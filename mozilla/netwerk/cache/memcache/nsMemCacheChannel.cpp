/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
 * The Original Code is Mozilla Communicator client code, released
 * March 31, 1998.
 * 
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation. Portions created by Netscape are
 * Copyright (C) 1998-1999 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 */

#include "nsMemCache.h"
#include "nsMemCacheChannel.h"
#include "nsIStreamListener.h"
#include "nsIChannel.h"
#include "nsIStorageStream.h"
#include "nsIOutputStream.h"
#include "nsIServiceManager.h"
#include "nsIEventQueueService.h"
#include "nsNetUtil.h"
#include "nsILoadGroup.h"

static NS_DEFINE_CID(kIOServiceCID, NS_IOSERVICE_CID);
static NS_DEFINE_CID(kEventQueueService, NS_EVENTQUEUESERVICE_CID);

NS_IMPL_ISUPPORTS(nsMemCacheChannel, NS_GET_IID(nsIChannel))

void
nsMemCacheChannel::NotifyStorageInUse(PRInt32 aBytesUsed)
{
    mRecord->mCache->mOccupancy += aBytesUsed;
}

/**
 * This class acts as an adaptor around a synchronous input stream to add async
 * read capabilities.  It adds methods for initiating, suspending, resuming and
 * cancelling async reads. 
 */
class AsyncReadStreamAdaptor : public nsIInputStream {
public:
    AsyncReadStreamAdaptor(nsMemCacheChannel* aChannel, nsIInputStream *aSyncStream):
        mSyncStream(aSyncStream), mDataAvailCursor(0),
        mRemaining(0), mChannel(aChannel), mAvailable(0), mAborted(false), mSuspended(false)
        {
            NS_INIT_REFCNT();
            NS_ADDREF(mChannel);
        }

    virtual ~AsyncReadStreamAdaptor() { 
        mChannel->mAsyncReadStream = 0;
        NS_RELEASE(mChannel);
    }
    
    NS_DECL_ISUPPORTS

    nsresult
    IsPending(PRBool* aIsPending) {
        *aIsPending = (mRemaining != 0) && !mAborted;
        return NS_OK;
    }

    nsresult
    Cancel(void) {
        mAborted = true;
        return mStreamListener->OnStopRequest(mChannel, mContext, NS_BINDING_ABORTED, nsnull);
    }

    nsresult
    Suspend(void) { mSuspended = true; return NS_OK; }

    nsresult
    Resume(void) {
        if (!mSuspended)
            return NS_ERROR_FAILURE;
        mSuspended = false;
        return NextListenerEvent();
    }

    NS_IMETHOD
    Available(PRUint32 *aNumBytes) { return mAvailable; }

    NS_IMETHOD
    Read(char* aBuf, PRUint32 aCount, PRUint32 *aBytesRead) {
        if (mAborted)
            return NS_ERROR_ABORT;

        *aBytesRead = 0;
        aCount = PR_MIN(aCount, mAvailable);
        nsresult rv = mSyncStream->Read(aBuf, aCount, aBytesRead);
        mAvailable -= *aBytesRead;

        if (NS_FAILED(rv) && (rv != NS_BASE_STREAM_WOULD_BLOCK)) {
            Fail();
            return rv;
        }

        if (!mSuspended && !mAvailable) {
            rv = NextListenerEvent();
            if (NS_FAILED(rv)) {
                Fail();
                return rv;
            }
        }

        return NS_OK;
    }

    NS_IMETHOD
    Close() {
        nsresult rv = mSyncStream->Close();
        mSyncStream = 0;
        mContext = 0;
        mStreamListener = 0;
        return rv;
    }

    nsresult
    AsyncRead(PRUint32 aStartPosition, PRInt32 aReadCount,
              nsISupports* aContext, nsIStreamListener* aListener) {

        nsresult rv;
        nsIEventQueue *eventQ;

        mContext = aContext;
        mStreamListener = aListener;
        mRemaining = aReadCount;

        NS_WITH_SERVICE(nsIIOService, serv, kIOServiceCID, &rv);
        if (NS_FAILED(rv)) return rv;

        NS_WITH_SERVICE(nsIEventQueueService, eventQService, kEventQueueService, &rv);
        if (NS_FAILED(rv)) return rv;

        rv = eventQService->GetThreadEventQueue(PR_CurrentThread(), &eventQ);
        if (NS_FAILED(rv)) return rv;

        rv = NS_NewAsyncStreamListener(aListener, eventQ, 
                                       getter_AddRefs(mStreamListener));
        NS_RELEASE(eventQ);
        if (NS_FAILED(rv)) return rv;

        rv = mStreamListener->OnStartRequest(mChannel, aContext);
        if (NS_FAILED(rv)) return rv;

        return NextListenerEvent();
    }

protected:

    nsresult
    Fail(void) {
        mAborted = true;
        return mStreamListener->OnStopRequest(mChannel, mContext, NS_BINDING_FAILED, nsnull);
    }

    nsresult
    NextListenerEvent() {
        PRUint32 available;
        nsresult rv = mSyncStream->Available(&available);
        if (NS_FAILED(rv)) return rv;
        available -= mAvailable;
        available = PR_MIN(available, mRemaining);

        if (available) {
            PRUint32 size = PR_MIN(available, MEM_CACHE_SEGMENT_SIZE);
            rv = mStreamListener->OnDataAvailable(mChannel, mContext, this,
                                                  mDataAvailCursor, size);
            mDataAvailCursor += size;
            mRemaining -= size;
            mAvailable += size;
            return rv;
        } else {
            rv = mStreamListener->OnStopRequest(mChannel, mContext, NS_OK, nsnull);
            AsyncReadStreamAdaptor* thisAlias = this;
            NS_RELEASE(thisAlias);
            return rv;
        }
    }
    
private:
    nsCOMPtr<nsISupports>       mContext;        // Opaque context passed to AsyncRead()
    nsCOMPtr<nsIStreamListener> mStreamListener; // Stream listener that has been proxied
    nsCOMPtr<nsIInputStream>    mSyncStream;     // Underlying synchronous stream that is
                                                 //   being converted to an async stream
    PRUint32                    mDataAvailCursor;
    PRUint32                    mRemaining;      // Size of AsyncRead request less bytes for
                                                 //   consumer OnDataAvailable's that were fired
    PRUint32                    mAvailable;      // Number of bytes for which OnDataAvailable fired
    nsMemCacheChannel*          mChannel;        // Associated memory cache channel, strong link
                                                 //   but can not use nsCOMPtr
    bool                        mAborted;        // Abort() has been called
    bool                        mSuspended;      // Suspend() has been called
};

NS_IMPL_ISUPPORTS(AsyncReadStreamAdaptor,  NS_GET_IID(nsIInputStream))

// The only purpose of this output stream wrapper is to adjust the cache's
// overall occupancy as new data flows into the cache entry.
class MemCacheWriteStreamWrapper : public nsIOutputStream {
public:
    MemCacheWriteStreamWrapper(nsMemCacheChannel* aChannel, nsIOutputStream *aBaseStream):
        mBaseStream(aBaseStream), mChannel(aChannel)
        {
            NS_INIT_REFCNT();
            NS_ADDREF(mChannel);
        }

    virtual ~MemCacheWriteStreamWrapper() { NS_RELEASE(mChannel); };
    
    static nsresult
    Create(nsMemCacheChannel* aChannel, nsIOutputStream *aBaseStream, nsIOutputStream* *aWrapper) {
        MemCacheWriteStreamWrapper *wrapper =
            new MemCacheWriteStreamWrapper(aChannel, aBaseStream);
        if (!wrapper) return NS_ERROR_OUT_OF_MEMORY;
        NS_ADDREF(wrapper);
        *aWrapper = wrapper;
        return NS_OK;
    }
    
    NS_DECL_ISUPPORTS

    NS_IMETHOD
    Write(const char *aBuffer, PRUint32 aCount, PRUint32 *aNumWritten) {
        *aNumWritten = 0;
        nsresult rv = mBaseStream->Write(aBuffer, aCount, aNumWritten);
        mChannel->NotifyStorageInUse(*aNumWritten);
        return rv;
    }

    NS_IMETHOD
    Flush() { return mBaseStream->Flush(); }

    NS_IMETHOD
    Close() { return mBaseStream->Close(); }

private:
    nsCOMPtr<nsIOutputStream>   mBaseStream;
    nsMemCacheChannel*          mChannel;
};

NS_IMPL_ISUPPORTS(MemCacheWriteStreamWrapper,  NS_GET_IID(nsIOutputStream))

nsMemCacheChannel::nsMemCacheChannel(nsMemCacheRecord *aRecord, nsILoadGroup *aLoadGroup)
    : mRecord(aRecord)
{
    NS_INIT_REFCNT();
    mRecord->mNumChannels++;
}

nsMemCacheChannel::~nsMemCacheChannel()
{
    mRecord->mNumChannels--;
}

NS_IMETHODIMP
nsMemCacheChannel::IsPending(PRBool* aIsPending)
{
    *aIsPending = PR_FALSE;
    if (!mAsyncReadStream)
        return NS_OK;
    return mAsyncReadStream->IsPending(aIsPending);
}

NS_IMETHODIMP
nsMemCacheChannel::Cancel(void)
{
    if (!mAsyncReadStream)
        return NS_ERROR_FAILURE;
    return mAsyncReadStream->Cancel();
}

NS_IMETHODIMP
nsMemCacheChannel::Suspend(void)
{
    if (!mAsyncReadStream)
        return NS_ERROR_FAILURE;
    return mAsyncReadStream->Suspend();
}

NS_IMETHODIMP
nsMemCacheChannel::Resume(void)
{
    if (!mAsyncReadStream)
        return NS_ERROR_FAILURE;
    return mAsyncReadStream->Resume();
}

NS_IMETHODIMP
nsMemCacheChannel::GetOriginalURI(nsIURI * *aURI)
{
    // Not required
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsMemCacheChannel::GetURI(nsIURI * *aURI)
{
    // Not required to be implemented, since it is implemented by cache manager
    NS_ASSERTION(0, "nsMemCacheChannel method unexpectedly called");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsMemCacheChannel::OpenInputStream(PRUint32 aStartPosition, PRInt32 aReadCount,
                                   nsIInputStream* *aResult)
{
    nsresult rv;
    NS_ENSURE_ARG(aResult);
    if (mInputStream)
        return NS_ERROR_NOT_AVAILABLE;

    rv = mRecord->mStorageStream->NewInputStream(aStartPosition, getter_AddRefs(mInputStream));
    *aResult = mInputStream;
    NS_ADDREF(*aResult);
    return rv;
}

NS_IMETHODIMP
nsMemCacheChannel::OpenOutputStream(PRUint32 startPosition, nsIOutputStream* *aResult)
{
    nsresult rv;
    NS_ENSURE_ARG(aResult);

    nsCOMPtr<nsIOutputStream> outputStream;

    PRUint32 oldLength;
    mRecord->mStorageStream->GetLength(&oldLength);
    rv = mRecord->mStorageStream->GetOutputStream(startPosition, getter_AddRefs(outputStream));
    if (NS_FAILED(rv)) return rv;
    if (startPosition < oldLength)
        NotifyStorageInUse(startPosition - oldLength);

    return MemCacheWriteStreamWrapper::Create(this, outputStream, aResult);
}

NS_IMETHODIMP
nsMemCacheChannel::AsyncOpen(nsIStreamObserver *observer, nsISupports *ctxt)
{
    // Not required
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsMemCacheChannel::AsyncRead(PRUint32 aStartPosition, PRInt32 aReadCount,
                             nsISupports *aContext, nsIStreamListener *aListener)
{
    nsCOMPtr<nsIInputStream> inputStream;
    nsresult rv = OpenInputStream(aStartPosition, aReadCount, getter_AddRefs(inputStream));
    if (NS_FAILED(rv)) return rv;
    
    AsyncReadStreamAdaptor *asyncReadStreamAdaptor;
    asyncReadStreamAdaptor = new AsyncReadStreamAdaptor(this, inputStream);
    if (!asyncReadStreamAdaptor)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(asyncReadStreamAdaptor);
    mAsyncReadStream = asyncReadStreamAdaptor;

    rv = asyncReadStreamAdaptor->AsyncRead(aStartPosition, aReadCount, aContext, aListener);
    if (NS_FAILED(rv))
        delete asyncReadStreamAdaptor;
    return rv;
}

NS_IMETHODIMP
nsMemCacheChannel::AsyncWrite(nsIInputStream *fromStream, PRUint32 startPosition,
                              PRInt32 writeCount, nsISupports *ctxt,
                              nsIStreamObserver *observer)
{
    // Not required to be implemented
    NS_ASSERTION(0, "nsMemCacheChannel method unexpectedly called");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsMemCacheChannel::GetLoadAttributes(nsLoadFlags *aLoadAttributes)
{
    // Not required to be implemented, since it is implemented by cache manager
    NS_ASSERTION(0, "nsMemCacheChannel method unexpectedly called");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsMemCacheChannel::SetLoadAttributes(nsLoadFlags aLoadAttributes)
{
    // Not required to be implemented, since it is implemented by cache manager
    NS_ASSERTION(0, "nsMemCacheChannel method unexpectedly called");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsMemCacheChannel::GetContentType(char* *aContentType)
{
    // Not required to be implemented, since it is implemented by cache manager
    // NS_ASSERTION(0, "nsMemCacheChannel method unexpectedly called");
    // FIXME - lying for the purpose of testing
    *aContentType = strdup("text/html");
    return NS_OK;
}

NS_IMETHODIMP
nsMemCacheChannel::GetContentLength(PRInt32 *aContentLength)
{
    // Not required to be implemented, since it is implemented by cache manager
    NS_ASSERTION(0, "nsMemCacheChannel method unexpectedly called");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsMemCacheChannel::GetOwner(nsISupports* *aOwner)
{
    *aOwner = mOwner.get();
    NS_IF_ADDREF(*aOwner);
    return NS_OK;
}

NS_IMETHODIMP
nsMemCacheChannel::SetOwner(nsISupports* aOwner)
{
    // Not required to be implemented, since it is implemented by cache manager
    mOwner = aOwner;
    return NS_OK;
}

NS_IMETHODIMP
nsMemCacheChannel::GetLoadGroup(nsILoadGroup* *aLoadGroup)
{
    // Not required to be implemented, since it is implemented by cache manager
    NS_ASSERTION(0, "nsMemCacheChannel method unexpectedly called");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsMemCacheChannel::SetLoadGroup(nsILoadGroup* aLoadGroup)
{
    // Not required to be implemented, since it is implemented by cache manager
    NS_ASSERTION(0, "nsMemCacheChannel method unexpectedly called");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsMemCacheChannel::GetNotificationCallbacks(nsIInterfaceRequestor* *aNotificationCallbacks)
{
    // Not required to be implemented, since it is implemented by cache manager
    NS_ASSERTION(0, "nsMemCacheChannel method unexpectedly called");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsMemCacheChannel::SetNotificationCallbacks(nsIInterfaceRequestor* aNotificationCallbacks)
{
    // Not required to be implemented, since it is implemented by cache manager
    NS_ASSERTION(0, "nsMemCacheChannel method unexpectedly called");
    return NS_ERROR_NOT_IMPLEMENTED;
}
