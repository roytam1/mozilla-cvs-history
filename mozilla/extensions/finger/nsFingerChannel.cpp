/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Brian Ryner.
 * Portions created by Brian Ryner are Copyright (C) 2000 Brian Ryner.
 * All Rights Reserved.
 *
 * Contributor(s): 
 *  Brian Ryner <bryner@uiuc.edu>
 */

// finger implementation

#include "nsFingerChannel.h"
#include "nsIServiceManager.h"
#include "nsILoadGroup.h"
#include "nsIInterfaceRequestor.h"
#include "nsXPIDLString.h"
#include "nsISocketTransportService.h"
#include "nsIStringStream.h"
#include "nsMimeTypes.h"
#include "nsIStreamConverterService.h"
#include "nsITXTToHTMLConv.h"
#include "nsNetUtil.h"

static NS_DEFINE_CID(kSocketTransportServiceCID, NS_SOCKETTRANSPORTSERVICE_CID);
static NS_DEFINE_CID(kStreamConverterServiceCID, NS_STREAMCONVERTERSERVICE_CID);

#define BUFFER_SEG_SIZE (4*1024)
#define BUFFER_MAX_SIZE (64*1024)

// nsFingerChannel methods
nsFingerChannel::nsFingerChannel()
    : mContentLength(-1),
      mActAsObserver(PR_TRUE),
      mPort(-1),
      mStatus(NS_OK)
{
    NS_INIT_REFCNT();
}

nsFingerChannel::~nsFingerChannel() {
}

NS_IMPL_THREADSAFE_ISUPPORTS5(nsFingerChannel, 
                              nsIChannel, 
                              nsIRequest,
                              nsIStreamContentInfo,
                              nsIStreamListener, 
                              nsIStreamObserver)

nsresult
nsFingerChannel::Init(nsIURI* uri)
{
    nsresult rv;
    nsXPIDLCString autoBuffer;

    NS_ASSERTION(uri, "no uri");

    mUrl = uri;

//  For security reasons, we do not allow the user to specify a
//  non-default port for finger: URL's.

    mPort = FINGER_PORT;

    rv = mUrl->GetPath(getter_Copies(autoBuffer)); // autoBuffer = user@host
    if (NS_FAILED(rv)) return rv;

    nsCString cString(autoBuffer);
    nsCString tempBuf;

    PRUint32 i;

    // Now parse out the user and host
    for (i=0; cString[i] != '\0'; i++) {
      if (cString[i] == '@') {
        cString.Left(tempBuf, i);
        mUser = tempBuf;
        cString.Right(tempBuf, cString.Length() - i - 1);
        mHost = tempBuf;
        break;
       }
    }

    // Catch the case of just the host being given

    if (cString[i] == '\0') {
      mHost = cString;
    }

#ifdef DEBUG_bryner
    printf("Status:mUser = %s, mHost = %s\n", (const char*)mUser,
           (const char*)mHost);
#endif
    if (!*(const char *)mHost) return NS_ERROR_NOT_INITIALIZED;

    return NS_OK;
}

NS_METHOD
nsFingerChannel::Create(nsISupports* aOuter, const nsIID& aIID, void* *aResult)
{
    nsFingerChannel* fc = new nsFingerChannel();
    if (fc == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(fc);
    nsresult rv = fc->QueryInterface(aIID, aResult);
    NS_RELEASE(fc);
    return rv;
}

////////////////////////////////////////////////////////////////////////////////
// nsIRequest methods:

NS_IMETHODIMP
nsFingerChannel::GetName(PRUnichar* *result)
{
    NS_NOTREACHED("nsFingerChannel::GetName");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsFingerChannel::IsPending(PRBool *result)
{
    NS_NOTREACHED("nsFingerChannel::IsPending");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsFingerChannel::GetStatus(nsresult *status)
{
    *status = mStatus;
    return NS_OK;
}

NS_IMETHODIMP
nsFingerChannel::Cancel(nsresult status)
{
    NS_ASSERTION(NS_FAILED(status), "shouldn't cancel with a success code");
    nsresult rv = NS_ERROR_FAILURE;

    mStatus = status;
    if (mTransportRequest) {
      rv = mTransportRequest->Cancel(status);
    }
    return rv;
}

NS_IMETHODIMP
nsFingerChannel::Suspend(void)
{
    NS_NOTREACHED("nsFingerChannel::Suspend");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsFingerChannel::Resume(void)
{
    NS_NOTREACHED("nsFingerChannel::Resume");
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute nsISupports parent; */
NS_IMETHODIMP
nsFingerChannel::GetParent(nsISupports * *aParent)
{
    NS_ADDREF(*aParent=(nsISupports*)(nsIChannel*)this);
    return NS_OK;
}
NS_IMETHODIMP
nsFingerChannel::SetParent(nsISupports * aParent)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
////////////////////////////////////////////////////////////////////////////////
// nsIChannel methods:

NS_IMETHODIMP
nsFingerChannel::GetOriginalURI(nsIURI* *aURI)
{
    *aURI = mOriginalURI ? mOriginalURI : mUrl;
    NS_ADDREF(*aURI);
    return NS_OK;
}

NS_IMETHODIMP
nsFingerChannel::SetOriginalURI(nsIURI* aURI)
{
    mOriginalURI = aURI;
    return NS_OK;
}

NS_IMETHODIMP
nsFingerChannel::GetURI(nsIURI* *aURI)
{
    *aURI = mUrl;
    NS_IF_ADDREF(*aURI);
    return NS_OK;
}

NS_IMETHODIMP
nsFingerChannel::SetURI(nsIURI* aURI)
{
    mUrl = aURI;
    return NS_OK;
}

NS_IMETHODIMP
nsFingerChannel::OpenInputStream(PRUint32 transferOffset, PRUint32 transferCount, nsIInputStream **_retval)
{
    nsresult rv = NS_OK;

    NS_WITH_SERVICE(nsISocketTransportService, socketService, kSocketTransportServiceCID, &rv);
    if (NS_FAILED(rv)) return rv;

    rv = socketService->CreateTransport(mHost, mPort, nsnull, -1, BUFFER_SEG_SIZE,
            BUFFER_MAX_SIZE, getter_AddRefs(mTransport));
    if (NS_FAILED(rv)) return rv;

    rv = mTransport->SetNotificationCallbacks(mCallbacks);
    if (NS_FAILED(rv)) return rv;

    return mTransport->OpenInputStream(transferOffset, transferCount, _retval);
}

NS_IMETHODIMP
nsFingerChannel::OpenOutputStream(PRUint32 transferOffset, PRUint32 transferCount, nsIOutputStream **_retval)
{
    NS_NOTREACHED("nsFingerChannel::OpenOutputStream");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsFingerChannel::AsyncRead(nsIStreamListener *aListener, nsISupports *ctxt,
                           PRUint32 transferOffset, PRUint32 transferCount, nsIRequest **_retval)
{
    nsresult rv = NS_OK;

    NS_WITH_SERVICE(nsISocketTransportService, socketService, kSocketTransportServiceCID, &rv);
    if (NS_FAILED(rv)) return rv;

    rv = socketService->CreateTransport(mHost, mPort, nsnull, -1, BUFFER_SEG_SIZE,
      BUFFER_MAX_SIZE, getter_AddRefs(mTransport));
    if (NS_FAILED(rv)) return rv;

    rv = mTransport->SetNotificationCallbacks(mCallbacks);
    if (NS_FAILED(rv)) return rv;

    mListener = aListener;
    mResponseContext = ctxt;

    rv = SendRequest(mTransport);

    if (NS_SUCCEEDED(rv))
        NS_ADDREF(*_retval=this);
    return rv;
}

NS_IMETHODIMP
nsFingerChannel::AsyncWrite(nsIStreamProvider *provider,
                            nsISupports *ctxt,
                            PRUint32 transferOffset, 
                            PRUint32 transferCount, 
                            nsIRequest **_retval)
{
    NS_NOTREACHED("nsFingerChannel::AsyncWrite");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsFingerChannel::GetLoadAttributes(PRUint32 *aLoadAttributes)
{
    *aLoadAttributes = mLoadAttributes;
    return NS_OK;
}

NS_IMETHODIMP
nsFingerChannel::SetLoadAttributes(PRUint32 aLoadAttributes)
{
    mLoadAttributes = aLoadAttributes;
    return NS_OK;
}

#define FINGER_TYPE TEXT_HTML

NS_IMETHODIMP
nsFingerChannel::GetContentType(char* *aContentType) {
    if (!aContentType) return NS_ERROR_NULL_POINTER;

    *aContentType = nsCRT::strdup(FINGER_TYPE);
    if (!*aContentType) return NS_ERROR_OUT_OF_MEMORY;
    return NS_OK;
}

NS_IMETHODIMP
nsFingerChannel::SetContentType(const char *aContentType)
{
    //It doesn't make sense to set the content-type on this type
    // of channel...
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsFingerChannel::GetContentLength(PRInt32 *aContentLength)
{
    *aContentLength = mContentLength;
    return NS_OK;
}

NS_IMETHODIMP
nsFingerChannel::SetContentLength(PRInt32 aContentLength)
{
    NS_NOTREACHED("nsFingerChannel::SetContentLength");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsFingerChannel::GetLoadGroup(nsILoadGroup* *aLoadGroup)
{
    *aLoadGroup = mLoadGroup;
    NS_IF_ADDREF(*aLoadGroup);
    return NS_OK;
}

NS_IMETHODIMP
nsFingerChannel::SetLoadGroup(nsILoadGroup* aLoadGroup)
{
    mLoadGroup = aLoadGroup;
    return NS_OK;
}

NS_IMETHODIMP
nsFingerChannel::GetOwner(nsISupports* *aOwner)
{
    *aOwner = mOwner.get();
    NS_IF_ADDREF(*aOwner);
    return NS_OK;
}

NS_IMETHODIMP
nsFingerChannel::SetOwner(nsISupports* aOwner)
{
    mOwner = aOwner;
    return NS_OK;
}

NS_IMETHODIMP
nsFingerChannel::GetNotificationCallbacks(nsIInterfaceRequestor* *aNotificationCallbacks)
{
    *aNotificationCallbacks = mCallbacks.get();
    NS_IF_ADDREF(*aNotificationCallbacks);
    return NS_OK;
}

NS_IMETHODIMP
nsFingerChannel::SetNotificationCallbacks(nsIInterfaceRequestor* aNotificationCallbacks)
{
    mCallbacks = aNotificationCallbacks;
    return NS_OK;
}

NS_IMETHODIMP 
nsFingerChannel::GetSecurityInfo(nsISupports * *aSecurityInfo)
{
    *aSecurityInfo = nsnull;
    return NS_OK;
}

// nsIStreamObserver methods
NS_IMETHODIMP
nsFingerChannel::OnStartRequest(nsIRequest *aRequest, nsISupports *aContext) {
    if (!mActAsObserver) {
      // acting as a listener
      return mListener->OnStartRequest(this, aContext);
    } else {
      // we don't want to pass our AsyncWrite's OnStart through
      // we just ignore this
      return NS_OK;
    }
}


NS_IMETHODIMP
nsFingerChannel::OnStopRequest(nsIRequest *aRequest, nsISupports* aContext,
                               nsresult aStatus, const PRUnichar* aStatusArg)
{
#ifdef DEBUG_bryner
    printf("nsFingerChannel::OnStopRequest, mActAsObserver=%d\n",
            mActAsObserver);
    printf("  aChannel = %p\n", aChannel);
#endif
    nsresult rv = NS_OK;

    if (NS_FAILED(aStatus) || !mActAsObserver) {
        if (mLoadGroup) {
          rv = mLoadGroup->RemoveRequest(this, nsnull, aStatus, aStatusArg);
          if (NS_FAILED(rv)) return rv;
        }
        rv = mListener->OnStopRequest(this, aContext, aStatus, aStatusArg);
        mTransportRequest = 0;
        return rv;
    } else {
        // at this point we know the request has been sent.
        // we're no longer acting as an observer.
 
        mActAsObserver = PR_FALSE;
        nsCOMPtr<nsIStreamListener> converterListener;

        NS_WITH_SERVICE(nsIStreamConverterService, StreamConvService,
                         kStreamConverterServiceCID, &rv);
        if (NS_FAILED(rv)) return rv;

        nsAutoString fromStr; fromStr.AssignWithConversion("text/plain");
        nsAutoString toStr; toStr.AssignWithConversion("text/html");

        rv = StreamConvService->AsyncConvertData(fromStr.GetUnicode(),
              toStr.GetUnicode(), this, mResponseContext,
              getter_AddRefs(converterListener));
        if (NS_FAILED(rv)) return rv;

        nsCOMPtr<nsITXTToHTMLConv> converter(do_QueryInterface(converterListener));
        if (converter) {
          nsAutoString title; title.AssignWithConversion("Finger information for ");
          nsXPIDLCString userHost;
          rv = mUrl->GetPath(getter_Copies(userHost));
          title.AppendWithConversion(userHost);
          converter->SetTitle(title.GetUnicode());
          converter->PreFormatHTML(PR_TRUE);
        }


        return mTransport->AsyncRead(converterListener, mResponseContext, 0, -1, getter_AddRefs(mTransportRequest));
    }

}


// nsIStreamListener method
NS_IMETHODIMP
nsFingerChannel::OnDataAvailable(nsIRequest *aRequest, nsISupports* aContext,
                               nsIInputStream *aInputStream, PRUint32 aSourceOffset,
                               PRUint32 aLength) {
    mContentLength = aLength;
    return mListener->OnDataAvailable(this, aContext, aInputStream, aSourceOffset, aLength);
}

nsresult
nsFingerChannel::SendRequest(nsIChannel* aChannel) {
  // The text to send should already be in mUser

  nsresult rv = NS_OK;
  nsCOMPtr<nsISupports> result;
  nsCOMPtr<nsIInputStream> charstream;
  nsCString requestBuffer(mUser);

  if (mLoadGroup) {
    mLoadGroup->AddRequest(this, nsnull);
  }

  requestBuffer.Append(CRLF);

  mRequest = requestBuffer.ToNewCString();

  rv = NS_NewCharInputStream(getter_AddRefs(result), mRequest);
  if (NS_FAILED(rv)) return rv;

  charstream = do_QueryInterface(result, &rv);
  if (NS_FAILED(rv)) return rv;

#ifdef DEBUG_bryner
  printf("Sending: %s\n", requestBuffer.GetBuffer());
#endif
  
  rv = NS_AsyncWriteFromStream(getter_AddRefs(mTransportRequest),
                               aChannel, charstream,
                               0, requestBuffer.Length(),
                               this, nsnull);
  return rv;
}


