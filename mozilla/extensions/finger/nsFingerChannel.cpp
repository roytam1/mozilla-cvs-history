/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
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

static NS_DEFINE_CID(kSocketTransportServiceCID, NS_SOCKETTRANSPORTSERVICE_CID);

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

NS_IMPL_THREADSAFE_ISUPPORTS4(nsFingerChannel, nsIChannel, nsIRequest,
                              nsIStreamListener, nsIStreamObserver)

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
    nsresult rv = NS_ERROR_FAILURE;

    mStatus = status;
    if (mTransport) {
      rv = mTransport->Cancel(status);
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
nsFingerChannel::OpenInputStream(nsIInputStream **_retval)
{
    nsresult rv = NS_OK;

    NS_WITH_SERVICE(nsISocketTransportService, socketService, kSocketTransportServiceCID, &rv);
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIChannel> channel;
    rv = socketService->CreateTransport(mHost, mPort, mHost, BUFFER_SEG_SIZE,
            BUFFER_MAX_SIZE, getter_AddRefs(channel));
    if (NS_FAILED(rv)) return rv;

    rv = channel->SetNotificationCallbacks(mCallbacks);
    if (NS_FAILED(rv)) return rv;

    return channel->OpenInputStream(_retval);
}

NS_IMETHODIMP
nsFingerChannel::OpenOutputStream(nsIOutputStream **_retval)
{
    NS_NOTREACHED("nsFingerChannel::OpenOutputStream");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsFingerChannel::AsyncOpen(nsIStreamObserver *observer, nsISupports* ctxt)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsFingerChannel::AsyncRead(nsIStreamListener *aListener, nsISupports *ctxt)
{
    nsresult rv = NS_OK;

    NS_WITH_SERVICE(nsISocketTransportService, socketService, kSocketTransportServiceCID, &rv);
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIChannel> channel;
    rv = socketService->CreateTransport(mHost, mPort, mHost, BUFFER_SEG_SIZE,
      BUFFER_MAX_SIZE, getter_AddRefs(channel));
    if (NS_FAILED(rv)) return rv;

    rv = channel->SetNotificationCallbacks(mCallbacks);
    if (NS_FAILED(rv)) return rv;

    mListener = aListener;
    mResponseContext = ctxt;
    mTransport = channel;

    return SendRequest(channel);
}

NS_IMETHODIMP
nsFingerChannel::AsyncWrite(nsIInputStream *fromStream,
                            nsIStreamObserver *observer,
                            nsISupports *ctxt)
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

#define FINGER_TYPE TEXT_PLAIN

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
nsFingerChannel::GetTransferOffset(PRUint32 *aTransferOffset)
{
    NS_NOTREACHED("nsFingerChannel::GetTransferOffset");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsFingerChannel::SetTransferOffset(PRUint32 aTransferOffset)
{
    NS_NOTREACHED("nsFingerChannel::SetTransferOffset");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsFingerChannel::GetTransferCount(PRInt32 *aTransferCount)
{
    NS_NOTREACHED("nsFingerChannel::GetTransferCount");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsFingerChannel::SetTransferCount(PRInt32 aTransferCount)
{
    NS_NOTREACHED("nsFingerChannel::SetTransferCount");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsFingerChannel::GetBufferSegmentSize(PRUint32 *aBufferSegmentSize)
{
    NS_NOTREACHED("nsFingerChannel::GetBufferSegmentSize");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsFingerChannel::SetBufferSegmentSize(PRUint32 aBufferSegmentSize)
{
    NS_NOTREACHED("nsFingerChannel::SetBufferSegmentSize");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsFingerChannel::GetBufferMaxSize(PRUint32 *aBufferMaxSize)
{
    NS_NOTREACHED("nsFingerChannel::GetBufferMaxSize");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsFingerChannel::SetBufferMaxSize(PRUint32 aBufferMaxSize)
{
    NS_NOTREACHED("nsFingerChannel::SetBufferMaxSize");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsFingerChannel::GetShouldCache(PRBool *aShouldCache)
{
    *aShouldCache = PR_FALSE;
    return NS_OK;
}

NS_IMETHODIMP
nsFingerChannel::GetPipeliningAllowed(PRBool *aPipeliningAllowed)
{
    *aPipeliningAllowed = PR_FALSE;
    return NS_OK;
}
 
NS_IMETHODIMP
nsFingerChannel::SetPipeliningAllowed(PRBool aPipeliningAllowed)
{
    NS_NOTREACHED("SetPipeliningAllowed");
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
nsFingerChannel::OnStartRequest(nsIChannel *aChannel, nsISupports *aContext) {
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
nsFingerChannel::OnStopRequest(nsIChannel* aChannel, nsISupports* aContext,
                                      nsresult aStatus, const PRUnichar* aMsg) {
#ifdef DEBUG_bryner
    printf("nsFingerChannel::OnStopRequest, mActAsObserver=%d\n",
            mActAsObserver);
    printf("  aChannel = %p\n", aChannel);
#endif
    nsresult rv = NS_OK;

    if (NS_FAILED(aStatus) || !mActAsObserver) {
        if (mLoadGroup) {
          rv = mLoadGroup->RemoveChannel(this, nsnull, aStatus, aMsg);
          if (NS_FAILED(rv)) return rv;
        }
        rv = mListener->OnStopRequest(this, aContext, aStatus, aMsg);
        mTransport = 0;
        return rv;
    } else {
        // at this point we know the request has been sent.
        // we're no longer acting as an observer.
 
        mActAsObserver = PR_FALSE;
        return aChannel->AsyncRead(this, mResponseContext);
    }

}


// nsIStreamListener method
NS_IMETHODIMP
nsFingerChannel::OnDataAvailable(nsIChannel* aChannel, nsISupports* aContext,
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
    mLoadGroup->AddChannel(this, nsnull);
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

  rv = aChannel->SetTransferCount(requestBuffer.Length());
  if (NS_FAILED(rv)) return rv;
  rv = aChannel->AsyncWrite(charstream, this, 0);
  return rv;
}


