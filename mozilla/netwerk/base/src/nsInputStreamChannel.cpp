/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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

#include "nsInputStreamChannel.h"
#include "nsCOMPtr.h"
#include "nsIIOService.h"
#include "nsIServiceManager.h"
#include "nsIFileTransportService.h"
#include "netCore.h"
#include "nsXPIDLString.h"

static NS_DEFINE_CID(kIOServiceCID, NS_IOSERVICE_CID);
static NS_DEFINE_CID(kFileTransportServiceCID, NS_FILETRANSPORTSERVICE_CID);

////////////////////////////////////////////////////////////////////////////////
// nsInputStreamIO methods:

NS_IMPL_THREADSAFE_ISUPPORTS2(nsInputStreamIO,
                              nsIInputStreamIO,
                              nsIStreamIO)

nsInputStreamIO::nsInputStreamIO()
    : mName(nsnull), mContentType(nsnull), mContentLength(-1), mStatus(NS_OK)
{
    NS_INIT_REFCNT();
}

nsInputStreamIO::~nsInputStreamIO()
{
    (void)Close(NS_OK);
    if (mName) nsCRT::free(mName);
    if (mContentType) nsCRT::free(mContentType);
}

NS_METHOD
nsInputStreamIO::Create(nsISupports *aOuter, REFNSIID aIID, void **aResult)
{
    if (aOuter)
        return NS_ERROR_NO_AGGREGATION;
    nsInputStreamIO* io = new nsInputStreamIO();
    if (io == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(io);
    nsresult rv = io->QueryInterface(aIID, aResult);
    NS_RELEASE(io);
    return rv;
}

NS_IMETHODIMP
nsInputStreamIO::Init(const char* name, nsIInputStream* input,
                      const char* contentType, PRInt32 contentLength)
{
    mName = nsCRT::strdup(name);
    if (mName == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;
    mInputStream = input;
    if (contentType) { 
        mContentType = nsCRT::strdup(contentType);
        const char *constContentType = mContentType;
        if (!constContentType) return NS_ERROR_OUT_OF_MEMORY;
        char* semicolon = PL_strchr(constContentType, ';');
        CBufDescriptor cbd(constContentType,
                           PR_TRUE,
                           semicolon ? (semicolon-constContentType) + 1: PL_strlen(constContentType), // capacity 
                           semicolon ? (semicolon-constContentType) : PL_strlen(constContentType)); 
        nsCAutoString(cbd).ToLowerCase(); 
    } 
    mContentLength = contentLength;
    return NS_OK;
}

NS_IMETHODIMP
nsInputStreamIO::Open(char **contentType, PRInt32 *contentLength)
{
    *contentType = nsCRT::strdup(mContentType);
    if (*contentType == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;
    *contentLength = mContentLength;
    return NS_OK;
}

NS_IMETHODIMP
nsInputStreamIO::Close(nsresult status)
{
    mStatus = status;
    if (mInputStream)
        return mInputStream->Close();
    else
        return NS_OK;
}

NS_IMETHODIMP
nsInputStreamIO::GetInputStream(nsIInputStream * *aInputStream)
{
    *aInputStream = mInputStream;
    NS_ADDREF(*aInputStream);
    return NS_OK;
}

NS_IMETHODIMP
nsInputStreamIO::GetOutputStream(nsIOutputStream * *aOutputStream)
{
    // this method should never be called
    NS_NOTREACHED("nsInputStreamIO::GetOutputStream");
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsInputStreamIO::GetName(char* *aName)
{
    *aName = nsCRT::strdup(mName);
    return *aName ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

////////////////////////////////////////////////////////////////////////////////
// nsStreamIOChannel methods:

nsStreamIOChannel::nsStreamIOChannel()
    : mContentType(nsnull), mContentLength(-1),
      mBufferSegmentSize(0), mBufferMaxSize(0),
      mLoadAttributes(LOAD_NORMAL), mStatus(NS_OK)
{
    NS_INIT_REFCNT(); 
}

nsStreamIOChannel::~nsStreamIOChannel()
{
    if (mContentType) nsCRT::free(mContentType);
}

NS_METHOD
nsStreamIOChannel::Create(nsISupports *aOuter, REFNSIID aIID,
                             void **aResult)
{
    if (aOuter)
        return NS_ERROR_NO_AGGREGATION;
    nsStreamIOChannel* channel = new nsStreamIOChannel();
    if (channel == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(channel);
    nsresult rv = channel->QueryInterface(aIID, aResult);
    NS_RELEASE(channel);
    return rv;
}

NS_IMETHODIMP
nsStreamIOChannel::Init(nsIURI* uri, nsIStreamIO* io)
{
    mURI = uri;
    mStreamIO = io;
    return NS_OK;
}

////////////////////////////////////////////////////////////////////////////////
// nsISupports implementation:
////////////////////////////////////////////////////////////////////////////////

NS_IMPL_THREADSAFE_ADDREF(nsStreamIOChannel)
NS_IMPL_THREADSAFE_RELEASE(nsStreamIOChannel)

NS_INTERFACE_MAP_BEGIN(nsStreamIOChannel)
    NS_INTERFACE_MAP_ENTRY(nsIStreamIOChannel)
    NS_INTERFACE_MAP_ENTRY(nsIChannel)
    NS_INTERFACE_MAP_ENTRY(nsIRequest)
    NS_INTERFACE_MAP_ENTRY(nsIStreamContentInfo)
    NS_INTERFACE_MAP_ENTRY(nsIStreamListener)
    NS_INTERFACE_MAP_ENTRY(nsIStreamProvider)
    NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsIStreamObserver, nsIStreamListener)
    NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIStreamListener)
NS_INTERFACE_MAP_END_THREADSAFE

////////////////////////////////////////////////////////////////////////////////
// nsIRequest implementation:
////////////////////////////////////////////////////////////////////////////////

NS_IMETHODIMP
nsStreamIOChannel::GetName(PRUnichar* *result)
{
    nsresult rv;
    nsXPIDLCString urlStr;
    rv = mURI->GetSpec(getter_Copies(urlStr));
    if (NS_FAILED(rv)) return rv;
    nsString name;
    name.AppendWithConversion(urlStr);
    *result = name.ToNewUnicode();
    return *result ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

NS_IMETHODIMP
nsStreamIOChannel::IsPending(PRBool *result)
{
    if (mRequest)
        return mRequest->IsPending(result);
    *result = PR_FALSE;
    return NS_OK;
}

NS_IMETHODIMP
nsStreamIOChannel::GetStatus(nsresult *status)
{
    *status = mStatus;
    // if we don't have a status error of our own to report
    // then we should propogate the status error of the underlying
    // file transport (if we have one)
    if (NS_SUCCEEDED(mStatus) && mRequest)
      mRequest->GetStatus(status);

    return NS_OK;
}

NS_IMETHODIMP
nsStreamIOChannel::Cancel(nsresult status)
{
    NS_ASSERTION(NS_FAILED(status), "shouldn't cancel with a success code");
    mStatus = status;
    if (mRequest)
        return mRequest->Cancel(status);
    return NS_OK;
}

NS_IMETHODIMP
nsStreamIOChannel::Suspend(void)
{
    if (mRequest)
        return mRequest->Suspend();
    return NS_OK;
}

NS_IMETHODIMP
nsStreamIOChannel::Resume(void)
{
    if (mRequest)
        return mRequest->Resume();
    return NS_OK;
}

NS_IMETHODIMP
nsStreamIOChannel::GetParent(nsISupports **aParent)
{
    NS_ADDREF(*aParent=(nsISupports*)(nsIChannel*)this);
    return NS_OK;
}

NS_IMETHODIMP
nsStreamIOChannel::SetParent(nsISupports *aParent)
{
    NS_NOTREACHED("SetParent");
    return NS_ERROR_NOT_IMPLEMENTED;
}

////////////////////////////////////////////////////////////////////////////////
// nsIChannel implementation:
////////////////////////////////////////////////////////////////////////////////

NS_IMETHODIMP
nsStreamIOChannel::GetOriginalURI(nsIURI* *aURI)
{
    *aURI = mOriginalURI ? mOriginalURI : mURI;
    NS_IF_ADDREF(*aURI);
    return NS_OK;
}

NS_IMETHODIMP
nsStreamIOChannel::SetOriginalURI(nsIURI* aURI)
{
    mOriginalURI = aURI;
    return NS_OK;
}

NS_IMETHODIMP
nsStreamIOChannel::GetURI(nsIURI* *aURI)
{
    *aURI = mURI;
    NS_IF_ADDREF(*aURI);
    return NS_OK;
}

NS_IMETHODIMP
nsStreamIOChannel::SetURI(nsIURI* aURI)
{
    mURI = aURI;
    return NS_OK;
}

NS_IMETHODIMP
nsStreamIOChannel::OpenInputStream(PRUint32 transferOffset, PRUint32 transferCount, nsIInputStream **result)
{
    return mStreamIO->GetInputStream(result);
}

NS_IMETHODIMP
nsStreamIOChannel::OpenOutputStream(PRUint32 transferOffset, PRUint32 transferCount, nsIOutputStream **result)
{
    return mStreamIO->GetOutputStream(result);
}

NS_IMETHODIMP
nsStreamIOChannel::AsyncRead(nsIStreamListener *listener, nsISupports *ctxt,
                             PRUint32 transferOffset, PRUint32 transferCount, nsIRequest **_retval)
{
    nsresult rv;

    NS_ASSERTION(listener, "no listener");
    SetListener(listener);

    if (mLoadGroup) {
        nsCOMPtr<nsILoadGroupListenerFactory> factory;
        //
        // Create a load group "proxy" listener...
        //
        rv = mLoadGroup->GetGroupListenerFactory(getter_AddRefs(factory));
        if (factory) {
            nsIStreamListener *newListener;
            rv = factory->CreateLoadGroupListener(GetListener(), &newListener);
            if (NS_SUCCEEDED(rv)) {
                mUserObserver = newListener;
                NS_RELEASE(newListener);
            }
        }

        rv = mLoadGroup->AddRequest(this, nsnull);
        if (NS_FAILED(rv)) return rv;
    }

    if (mFileTransport == nsnull) {
        NS_WITH_SERVICE(nsIFileTransportService, fts, kFileTransportServiceCID, &rv);
        if (NS_FAILED(rv)) goto done;

        rv = fts->CreateTransportFromStreamIO(mStreamIO, getter_AddRefs(mFileTransport));
        if (NS_FAILED(rv)) goto done;
    }

#if 0
    if (mContentType == nsnull) {
        rv = mStreamIO->Open(&mContentType, &mContentLength);
        if (NS_FAILED(rv)) goto done;
    }
#endif
    rv = mFileTransport->AsyncRead(this, ctxt, 0, -1, getter_AddRefs(mRequest));

  done:
    if (NS_FAILED(rv)) {
        nsresult rv2;
        rv2 = mLoadGroup->RemoveRequest(this, ctxt, rv, nsnull);
        NS_ASSERTION(NS_SUCCEEDED(rv2), "RemoveRequest failed");
        // release the transport so that we don't think we're in progress
        mFileTransport = nsnull;
    }

    NS_IF_ADDREF(*_retval = mRequest);
    return rv;
}

NS_IMETHODIMP
nsStreamIOChannel::AsyncWrite(nsIStreamProvider *provider, nsISupports *ctxt,
                              PRUint32 transferOffset, PRUint32 transferCount,
                              nsIRequest **_retval)
{
    nsresult rv;

    NS_ASSERTION(provider, "no provider");
    SetProvider(provider);

    if (mLoadGroup) {
        nsCOMPtr<nsILoadGroupListenerFactory> factory;
        //
        // Create a load group "proxy" listener...
        //
        rv = mLoadGroup->GetGroupListenerFactory(getter_AddRefs(factory));
        if (factory) {
            NS_WARNING("load group proxy listener not implemented for AsyncWrite");
#if 0
            nsIStreamListener *newListener;
            rv = factory->CreateLoadGroupListener(GetListener(), &newListener);
            if (NS_SUCCEEDED(rv)) {
                mUserObserver = newListener;
                NS_RELEASE(newListener);
            }
#endif
        }

        rv = mLoadGroup->AddRequest(this, nsnull);
        if (NS_FAILED(rv)) return rv;
    }

    if (mFileTransport == nsnull) {
        NS_WITH_SERVICE(nsIFileTransportService, fts, kFileTransportServiceCID, &rv);
        if (NS_FAILED(rv)) goto done;

        rv = fts->CreateTransportFromStreamIO(mStreamIO, getter_AddRefs(mFileTransport));
        if (NS_FAILED(rv)) goto done;
    }

#if 0
    if (mContentType == nsnull) {
        rv = mStreamIO->Open(&mContentType, &mContentLength);
        if (NS_FAILED(rv)) goto done;
    }
#endif
    rv = mFileTransport->AsyncWrite(this, ctxt, 0, -1, getter_AddRefs(mRequest));

  done:
    if (NS_FAILED(rv)) {
        nsresult rv2;
        rv2 = mLoadGroup->RemoveRequest(this, ctxt, rv, nsnull);
        NS_ASSERTION(NS_SUCCEEDED(rv2), "RemoveRequest failed");
        // release the transport so that we don't think we're in progress
        mFileTransport = nsnull;
    }
    
    NS_IF_ADDREF(*_retval = mRequest);
    return rv;
}

NS_IMETHODIMP
nsStreamIOChannel::GetLoadAttributes(nsLoadFlags *aLoadAttributes)
{
    *aLoadAttributes = mLoadAttributes;
    return NS_OK;
}

NS_IMETHODIMP
nsStreamIOChannel::SetLoadAttributes(nsLoadFlags aLoadAttributes)
{
    mLoadAttributes = aLoadAttributes;
    return NS_OK;
}

NS_IMETHODIMP
nsStreamIOChannel::GetLoadGroup(nsILoadGroup* *aLoadGroup)
{
    *aLoadGroup = mLoadGroup.get();
    NS_IF_ADDREF(*aLoadGroup);
    return NS_OK;
}

NS_IMETHODIMP
nsStreamIOChannel::SetLoadGroup(nsILoadGroup* aLoadGroup)
{
    mLoadGroup = aLoadGroup;
    return NS_OK;
}

NS_IMETHODIMP
nsStreamIOChannel::GetOwner(nsISupports* *aOwner)
{
    *aOwner = mOwner.get();
    NS_IF_ADDREF(*aOwner);
    return NS_OK;
}

NS_IMETHODIMP
nsStreamIOChannel::SetOwner(nsISupports* aOwner)
{
    mOwner = aOwner;
    return NS_OK;
}

NS_IMETHODIMP
nsStreamIOChannel::GetNotificationCallbacks(nsIInterfaceRequestor* *aNotificationCallbacks)
{
  *aNotificationCallbacks = mCallbacks.get();
  NS_IF_ADDREF(*aNotificationCallbacks);
  return NS_OK;
}

NS_IMETHODIMP
nsStreamIOChannel::SetNotificationCallbacks(nsIInterfaceRequestor* aNotificationCallbacks)
{
  mCallbacks = aNotificationCallbacks;
  return NS_OK;
}


NS_IMETHODIMP 
nsStreamIOChannel::GetSecurityInfo(nsISupports * *aSecurityInfo)
{
    *aSecurityInfo = nsnull;
    return NS_OK;
}

////////////////////////////////////////////////////////////////////////////////
// nsIContentStreamInfo implementation:
////////////////////////////////////////////////////////////////////////////////

NS_IMETHODIMP
nsStreamIOChannel::GetContentType(char * *aContentType)
{
    nsresult rv;
    if (mContentType == nsnull) {
        rv = mStreamIO->Open(&mContentType, &mContentLength);
        if (NS_FAILED(rv)) return rv;
    }
    *aContentType = nsCRT::strdup(mContentType);
    if (*aContentType == nsnull) {
        return NS_ERROR_OUT_OF_MEMORY;
    }
    return NS_OK;
}

NS_IMETHODIMP
nsStreamIOChannel::SetContentType(const char *aContentType)
{
    mContentType = nsCRT::strdup(aContentType);
    if (*aContentType == nsnull) {
        return NS_ERROR_OUT_OF_MEMORY;
    }
    return NS_OK;
}

NS_IMETHODIMP
nsStreamIOChannel::GetContentLength(PRInt32 *aContentLength)
{
    nsresult rv;
    if (mContentType == nsnull) {
        rv = mStreamIO->Open(&mContentType, &mContentLength);
        if (NS_FAILED(rv)) return rv;
    }
    *aContentLength = mContentLength;
    return NS_OK;
}

NS_IMETHODIMP
nsStreamIOChannel::SetContentLength(PRInt32 aContentLength)
{
    mContentLength = aContentLength;
    return NS_OK;
}

////////////////////////////////////////////////////////////////////////////////
// nsIStreamObserver implementation:
////////////////////////////////////////////////////////////////////////////////

NS_IMETHODIMP
nsStreamIOChannel::OnStartRequest(nsIRequest *request, nsISupports* context)
{
    NS_ASSERTION(mUserObserver, "No listener...");
    return mUserObserver->OnStartRequest(this, context);
}

NS_IMETHODIMP
nsStreamIOChannel::OnStopRequest(nsIRequest *request, nsISupports* context,
                                 nsresult aStatus, const PRUnichar* aStatusArg)
{
    nsresult rv;

    if (mUserObserver) {
        rv = mUserObserver->OnStopRequest(this, context, aStatus, aStatusArg);
        if (NS_FAILED(rv)) return rv;
    }

    if (mLoadGroup) {
        if (NS_SUCCEEDED(rv)) {
            rv = mLoadGroup->RemoveRequest(this, context, aStatus, aStatusArg);
            if (NS_FAILED(rv)) return rv;
        }
    }

    // Release the reference to the consumer stream listener...
    mUserObserver = null_nsCOMPtr();
    mFileTransport = null_nsCOMPtr();

    return mStreamIO->Close(aStatus);
}

////////////////////////////////////////////////////////////////////////////////
// nsIStreamListener implementation:
////////////////////////////////////////////////////////////////////////////////

NS_IMETHODIMP
nsStreamIOChannel::OnDataAvailable(nsIRequest *request, nsISupports* context,
                                   nsIInputStream *aIStream, PRUint32 aSourceOffset,
                                   PRUint32 aLength)
{
    return GetListener()->OnDataAvailable(this, context, aIStream,
                                          aSourceOffset, aLength);
}

////////////////////////////////////////////////////////////////////////////////
// nsIStreamProvider implementation:
////////////////////////////////////////////////////////////////////////////////

NS_IMETHODIMP
nsStreamIOChannel::OnDataWritable(nsIRequest *request, nsISupports *context,
                                  nsIOutputStream *aOStream,
                                  PRUint32 aOffset, PRUint32 aLength)
{
    return GetProvider()->OnDataWritable(this, context, aOStream,
                                         aOffset, aLength);
}

////////////////////////////////////////////////////////////////////////////////
