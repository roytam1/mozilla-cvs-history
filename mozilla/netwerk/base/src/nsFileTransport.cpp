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
 * Communications Corporation. Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s):
 *   Darin Fisher <darin@netscape.com>
 */

#include <limits.h>

#include "nsFileTransport.h"
#include "nsFileTransportService.h"
#include "nsIInterfaceRequestor.h"
#include "nsAutoLock.h"
#include "netCore.h"
#include "nsIFileStreams.h"
#include "nsCOMPtr.h"
#include "nsIProxyObjectManager.h"
#include "nsNetUtil.h"

static NS_DEFINE_CID(kFileTransportServiceCID, NS_FILETRANSPORTSERVICE_CID);
static NS_DEFINE_CID(kProxyObjectManagerCID, NS_PROXYEVENT_MANAGER_CID);

#define NS_OUTPUT_STREAM_BUFFER_SIZE    (64 * 1024)

//////////////////////////////////////////////////////////////////////////////////

#if defined(PR_LOGGING)
static PRLogModuleInfo *gFileTransportLog = nsnull;
#endif

#define LOG(args) PR_LOG(gFileTransportLog, PR_LOG_DEBUG, args)

//////////////////////////////////////////////////////////////////////////////////

//
// An nsFileTransportSourceWrapper captures the number of bytes read from an
// input stream.
//
class nsFileTransportSourceWrapper : public nsIInputStream
{
public:
    NS_DECL_ISUPPORTS

    nsFileTransportSourceWrapper() : mBytesRead(0) {NS_INIT_ISUPPORTS();}
    virtual ~nsFileTransportSourceWrapper() {}

    //
    // nsIInputStream implementation...
    //
    NS_IMETHOD Close() {
        return mSource->Close();
    }
    NS_IMETHOD Available(PRUint32 *aCount) {
        return mSource->Available(aCount);
    }
    NS_IMETHOD Read(char *aBuf, PRUint32 aCount, PRUint32 *aBytesRead) {
        nsresult rv = mSource->Read(aBuf, aCount, aBytesRead);
        if (NS_SUCCEEDED(rv))
            mBytesRead += *aBytesRead;
        return rv;
    }
    NS_IMETHOD ReadSegments(nsWriteSegmentFun aWriter, void *aClosure,
                            PRUint32 aCount, PRUint32 *aBytesRead) {
        nsresult rv = mSource->ReadSegments(aWriter, aClosure, aCount, aBytesRead);
        if (NS_SUCCEEDED(rv))
            mBytesRead += *aBytesRead;
        return rv;
    }
    NS_IMETHOD GetNonBlocking(PRBool *aValue) {
        return mSource->GetNonBlocking(aValue);
    }
    NS_IMETHOD GetObserver(nsIInputStreamObserver **aObserver) {
        return mSource->GetObserver(aObserver);
    }
    NS_IMETHOD SetObserver(nsIInputStreamObserver *aObserver) {
        return mSource->SetObserver(aObserver);
    }

    //
    // Helper functions
    //
    void SetSource(nsIInputStream *aSource) {
        mSource = aSource;
    }
    PRUint32 GetBytesRead() {
        return mBytesRead;
    }
    void ZeroBytesRead() {
        mBytesRead = 0;
    }

protected:
    // 
    // State variables
    //
    PRUint32                 mBytesRead;
    nsCOMPtr<nsIInputStream> mSource;
};

// This must be threadsafe since different threads can run the same transport
NS_IMPL_THREADSAFE_ISUPPORTS1(nsFileTransportSourceWrapper, nsIInputStream)

//////////////////////////////////////////////////////////////////////////////////

//
// An nsFileTransportSinkWrapper captures the number of bytes written to an
// output stream.
//
class nsFileTransportSinkWrapper : public nsIOutputStream
{
public:
    NS_DECL_ISUPPORTS

    nsFileTransportSinkWrapper() : mBytesWritten(0) {NS_INIT_ISUPPORTS();}
    virtual ~nsFileTransportSinkWrapper() {}

    //
    // nsIInputStream implementation...
    //
    NS_IMETHOD Close() {
        return mSink->Close();
    }
    NS_IMETHOD Flush() {
        return mSink->Flush();
    }
    NS_IMETHOD Write(const char *aBuf, PRUint32 aCount, PRUint32 *aBytesWritten) {
        nsresult rv = mSink->Write(aBuf, aCount, aBytesWritten);
        if (NS_SUCCEEDED(rv))
            mBytesWritten += *aBytesWritten;
        return rv;
    }
    NS_IMETHOD WriteFrom(nsIInputStream *aSource, PRUint32 aCount, PRUint32 *aBytesWritten) {
        nsresult rv = mSink->WriteFrom(aSource, aCount, aBytesWritten);
        if (NS_SUCCEEDED(rv))
            mBytesWritten += *aBytesWritten;
        return rv;
    }
    NS_IMETHOD WriteSegments(nsReadSegmentFun aReader, void *aClosure,
                            PRUint32 aCount, PRUint32 *aBytesWritten) {
        nsresult rv = mSink->WriteSegments(aReader, aClosure, aCount, aBytesWritten);
        if (NS_SUCCEEDED(rv))
            mBytesWritten += *aBytesWritten;
        return rv;
    }
    NS_IMETHOD GetNonBlocking(PRBool *aValue) {
        return mSink->GetNonBlocking(aValue);
    }
    NS_IMETHOD SetNonBlocking(PRBool aValue) {
        return mSink->SetNonBlocking(aValue);
    }
    NS_IMETHOD GetObserver(nsIOutputStreamObserver **aObserver) {
        return mSink->GetObserver(aObserver);
    }
    NS_IMETHOD SetObserver(nsIOutputStreamObserver *aObserver) {
        return mSink->SetObserver(aObserver);
    }

    //
    // Helper functions
    //
    void SetSink(nsIOutputStream *aSink) {
        mSink = aSink;
    }
    PRUint32 GetBytesWritten() {
        return mBytesWritten;
    }
    void ZeroBytesWritten() {
        mBytesWritten = 0;
    }

protected:
    // 
    // State variables
    //
    PRUint32                  mBytesWritten;
    nsCOMPtr<nsIOutputStream> mSink;
};

// This must be threadsafe since different threads can run the same transport
NS_IMPL_THREADSAFE_ISUPPORTS1(nsFileTransportSinkWrapper, nsIOutputStream)

//////////////////////////////////////////////////////////////////////////////////

nsFileTransport::nsFileTransport()
    : mContentType(nsnull),
      mBufferSegmentSize(NS_FILE_TRANSPORT_DEFAULT_SEGMENT_SIZE),
      mBufferMaxSize(NS_FILE_TRANSPORT_DEFAULT_BUFFER_SIZE),
      mXferState(CLOSED),
      mRunState(RUNNING),
      mCancelStatus(NS_OK),
      mSuspendCount(0),
      mLock(nsnull),
      mActive(PR_FALSE),
      mStatus(NS_OK),
      mOffset(0),
      mTotalAmount(-1),
      mTransferAmount(-1),
      mSourceWrapper(nsnull),
      mSinkWrapper(nsnull),
      mService(nsnull)
{
    NS_INIT_ISUPPORTS();

#if defined(PR_LOGGING)
    if (!gFileTransportLog)
        gFileTransportLog = PR_NewLogModule("nsFileTransport");
#endif
}

nsresult
nsFileTransport::Init(nsFileTransportService *aService, nsIFile* file, PRInt32 ioFlags, PRInt32 perm)
{
    nsresult rv;
    nsCOMPtr<nsIFileIO> io;
    rv = NS_NewFileIO(getter_AddRefs(io), file, ioFlags, perm);
    if (NS_FAILED(rv)) return rv;

    return Init(aService, io);
}

nsresult
nsFileTransport::Init(nsFileTransportService *aService, const char* name, nsIInputStream* inStr,
                      const char* contentType, PRInt32 contentLength)
{
    nsresult rv;
    nsCOMPtr<nsIInputStreamIO> io;
    rv = NS_NewInputStreamIO(getter_AddRefs(io),
                             name, inStr, contentType, contentLength);
    if (NS_FAILED(rv)) return rv;
    return Init(aService, io);
}

nsresult
nsFileTransport::Init(nsFileTransportService *aService, nsIStreamIO* io)
{
    nsresult rv = NS_OK;
    if (mLock == nsnull) {
        mLock = PR_NewLock();
        if (mLock == nsnull)
            return NS_ERROR_OUT_OF_MEMORY;
    }
    mStreamIO = io;
    nsXPIDLCString name;
    rv = mStreamIO->GetName(getter_Copies(name));
    mStreamName = NS_STATIC_CAST(const char*, name);
    NS_ASSERTION(NS_SUCCEEDED(rv), "GetName failed");

    NS_ADDREF(mService = aService);
    PR_AtomicIncrement(&mService->mTotalTransports);

    return rv;
}

nsFileTransport::~nsFileTransport()
{
    if (mXferState != CLOSED)
        DoClose();

    NS_ASSERTION(mSourceWrapper == nsnull, "transport not closed");
    NS_ASSERTION(mSink == nsnull, "transport not closed");
    NS_ASSERTION(mSinkWrapper == nsnull, "transport not closed");

    if (mLock) {
        PR_DestroyLock(mLock);
        mLock = nsnull;
    }
    if (mContentType) {
        nsCRT::free(mContentType);
        mContentType = nsnull;
    }

    NS_IF_RELEASE(mService);
}

NS_IMPL_THREADSAFE_ISUPPORTS4(nsFileTransport, 
                         nsITransport, 
                         nsITransportRequest,
                         nsIRequest, 
                         nsIRunnable)

NS_METHOD
nsFileTransport::Create(nsISupports* aOuter, const nsIID& aIID, void* *aResult)
{
    nsFileTransport* fc = new nsFileTransport();
    if (fc == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(fc);
    nsresult rv = fc->QueryInterface(aIID, aResult);
    NS_RELEASE(fc);
    return rv;
}

////////////////////////////////////////////////////////////////////////////////
// From nsIRequest
////////////////////////////////////////////////////////////////////////////////

NS_IMETHODIMP
nsFileTransport::GetName(PRUnichar* *result)
{
    nsString name;
    name.AppendWithConversion(mStreamName);
    *result = name.ToNewUnicode();
    return *result ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

NS_IMETHODIMP
nsFileTransport::IsPending(PRBool *result)
{
    *result = mXferState != CLOSED;
    return NS_OK;
}

NS_IMETHODIMP
nsFileTransport::GetStatus(nsresult *status)
{
    *status = mRunState == CANCELED ? mCancelStatus : mStatus;
    return NS_OK;
}

NS_IMETHODIMP
nsFileTransport::Cancel(nsresult status)
{
    nsresult rv = NS_OK;

    nsAutoLock lock(mLock);
    NS_ASSERTION(NS_FAILED(status), "shouldn't cancel with a success code");

    mCancelStatus = status;

    // Only dispatch a new thread, if there isn't one currently active.
    if (!mActive) {
#ifdef TIMING
        mStartTime = PR_IntervalNow();
#endif
        rv = mService->DispatchRequest(this);
    }

    LOG(("nsFileTransport: Cancel [this=%x %s]\n", this, mStreamName.get()));
    return rv;
}

NS_IMETHODIMP
nsFileTransport::Suspend()
{
    nsAutoLock lock(mLock);
    if (mRunState != CANCELED) {
        LOG(("nsFileTransport: Suspend [this=%x %s]\n", this, mStreamName.get()));
        PR_AtomicIncrement(&mSuspendCount);
        mService->AddSuspendedTransport(this);
    }
    return NS_OK;
}

NS_IMETHODIMP
nsFileTransport::Resume()
{
    nsAutoLock lock(mLock);
    if (mRunState != CANCELED) {
        LOG(("nsFileTransport: Resume [this=%x %s]\n", this, mStreamName.get()));
        // Allow negative suspend count
        PR_AtomicDecrement(&mSuspendCount);
        mService->RemoveSuspendedTransport(this);
        // Only dispatch a new thread, if there isn't one currently active.
        if (!mActive && (mSuspendCount == 0)) {
            mRunState = RUNNING;
#ifdef TIMING
            mStartTime = PR_IntervalNow();
#endif
            mStatus = mService->DispatchRequest(this);
        }
    }
    else
        LOG(("nsFileTransport: Resume ignored [this=%x %s] status=%x cancelstatus=%x\n",
            this, mStreamName.get(), mStatus, mCancelStatus));
    return NS_OK;
}

////////////////////////////////////////////////////////////////////////////////
// From nsITransportRequest
////////////////////////////////////////////////////////////////////////////////

NS_IMETHODIMP
nsFileTransport::GetTransport(nsITransport **result)
{
    NS_ENSURE_ARG_POINTER(result);
    NS_ADDREF(*result = this);
    return NS_OK;
}

////////////////////////////////////////////////////////////////////////////////
// From nsITransport
////////////////////////////////////////////////////////////////////////////////

NS_IMETHODIMP
nsFileTransport::OpenInputStream(PRUint32 aTransferOffset,
                                 PRUint32 aTransferCount,
                                 PRUint32 aFlags,
                                 nsIInputStream **aResult)
{
    NS_ENSURE_ARG_POINTER(aResult);
    NS_ASSERTION(aTransferCount == ULONG_MAX, "need to wrap input stream in one that truncates");
    
    nsresult rv = mStreamIO->GetInputStream(aResult);
    if (NS_SUCCEEDED(rv) && aTransferOffset) {
        nsCOMPtr<nsISeekableStream> seekable(do_QueryInterface(*aResult, &rv));
        if (NS_SUCCEEDED(rv) && seekable) {
            rv = seekable->Seek(nsISeekableStream::NS_SEEK_SET, aTransferOffset);
        }
    }
    
    return rv;
}

NS_IMETHODIMP
nsFileTransport::OpenOutputStream(PRUint32 aTransferOffset,
                                  PRUint32 aTransferCount,
                                  PRUint32 aFlags,
                                  nsIOutputStream **aResult)
{
    NS_ENSURE_ARG_POINTER(aResult);
    NS_ASSERTION(aTransferCount == ULONG_MAX, "need to wrap output stream in one that truncates");
    
    nsresult rv = mStreamIO->GetOutputStream(aResult);
    if (NS_SUCCEEDED(rv) && aTransferOffset) {
        nsCOMPtr<nsISeekableStream> seekable(do_QueryInterface(*aResult, &rv));
        if (NS_SUCCEEDED(rv) && seekable) {
            rv = seekable->Seek(nsISeekableStream::NS_SEEK_SET, aTransferOffset);
        }
    }
    
    return rv;
}

NS_IMETHODIMP
nsFileTransport::AsyncRead(nsIStreamListener *aListener,
                           nsISupports *aContext,
                           PRUint32 aTransferOffset,
                           PRUint32 aTransferCount,
                           PRUint32 aFlags,
                           nsIRequest **aResult)
{
    NS_ENSURE_ARG_POINTER(aResult);
    nsresult rv = NS_OK;

    if (mXferState != CLOSED)
        return NS_ERROR_IN_PROGRESS;

    NS_ASSERTION(aListener, "need to supply an nsIStreamListener");
    rv = NS_NewStreamListenerProxy(getter_AddRefs(mListener),
                                   aListener, nsnull,
                                   mBufferSegmentSize,
                                   mBufferMaxSize);
    if (NS_FAILED(rv)) return rv;

    NS_ASSERTION(mContext == nsnull, "context not released");
    mContext = aContext;
    mOffset = aTransferOffset;
    mTransferAmount = aTransferCount;
    mXferState = OPEN_FOR_READ;

    LOG(("nsFileTransport: AsyncRead [this=%x %s] mOffset=%d mTransferAmount=%d\n",
        this, mStreamName.get(), mOffset, mTransferAmount));

#ifdef TIMING
    mStartTime = PR_IntervalNow();
#endif
    rv = mService->DispatchRequest(this);
    if (NS_FAILED(rv)) return rv;

    NS_ADDREF(*aResult = this);
    return NS_OK;
}

NS_IMETHODIMP
nsFileTransport::AsyncWrite(nsIStreamProvider *aProvider,
                            nsISupports *aContext,
                            PRUint32 aTransferOffset,
                            PRUint32 aTransferCount,
                            PRUint32 aFlags,
                            nsIRequest **aResult)
{
    NS_ENSURE_ARG_POINTER(aResult);
    nsresult rv = NS_OK;

    LOG(("nsFileTransport: AsyncWrite [this=%x, provider=%x]\n",
        this, aProvider));

    if (mXferState != CLOSED)
        return NS_ERROR_IN_PROGRESS;

    NS_ASSERTION(aProvider, "need to supply an nsIStreamProvider");
    rv = NS_NewStreamProviderProxy(getter_AddRefs(mProvider),
                                   aProvider, nsnull,
                                   mBufferSegmentSize,
                                   mBufferMaxSize);
    if (NS_FAILED(rv)) return rv;

    NS_ASSERTION(mContext == nsnull, "context not released");
    mContext = aContext;
    mOffset = aTransferOffset;
    mTransferAmount = aTransferCount;
    mXferState = OPEN_FOR_WRITE;

    LOG(("nsFileTransport: AsyncWrite [this=%x %s] mOffset=%d mTransferAmount=%d\n",
        this, mStreamName.get(), mOffset, mTransferAmount));

#ifdef TIMING
    mStartTime = PR_IntervalNow();
#endif
    rv = mService->DispatchRequest(this);
    if (NS_FAILED(rv)) return rv;

    NS_ADDREF(*aResult = this);
    return NS_OK;
}
    
////////////////////////////////////////////////////////////////////////////////
// nsIRunnable methods:
////////////////////////////////////////////////////////////////////////////////

NS_IMETHODIMP
nsFileTransport::Run(void)
{
    PR_Lock(mLock);
    mActive = PR_TRUE;

    LOG(("nsFileTransport: Inside Run\n"));

#ifdef TIMING
    PRIntervalTime now = PR_IntervalNow();
    printf("nsFileTransport: latency=%u ticks\n", now - mStartTime);
#endif
    
    if (mRunState == SUSPENDED && NS_FAILED(mCancelStatus))
        mRunState = CANCELED;

    while (mXferState != CLOSED && mRunState != SUSPENDED) {
        //
        // Change transfer state if canceled.
        //
        if (mRunState == CANCELED) {
            if (mXferState == READING)
                mXferState = END_READ;
            else if (mXferState == WRITING)
                mXferState = END_WRITE;
            else
                mXferState = CLOSING;
            mStatus = mCancelStatus;
        }
        //
        // While processing, we allow Suspend, Resume, and Cancel.
        //
        PR_Unlock(mLock);
        Process();
        PR_Lock(mLock);

        //
        // Were we canceled ?
        //
        if (NS_FAILED(mCancelStatus))
            mRunState = CANCELED;
        //
        // Were we suspended ?
        //
        else if (mSuspendCount > 0) {
            mRunState = SUSPENDED;
            mService->AddSuspendedTransport(this);
        }
    }

    LOG(("nsFileTransport: Leaving Run [xferState=%d runState=%d]\n",
        mXferState, mRunState));

    mActive = PR_FALSE;
    PR_Unlock(mLock);
    return NS_OK;
}

void
nsFileTransport::Process(void)
{
    LOG(("nsFileTransport: Inside Process [this=%x state=%x status=%x]\n",
        this, mXferState, mStatus));

    switch (mXferState) {
      case OPEN_FOR_READ: { 
        mStatus = mStreamIO->Open(&mContentType, &mTotalAmount);
        LOG(("nsFileTransport: OPEN_FOR_READ [this=%x %s] status=%x\n", this, mStreamName.get(), mStatus));
        if (mListener) {
            nsresult rv = mListener->OnStartRequest(this, mContext);  // always send the start notification
            if (NS_SUCCEEDED(mStatus))
                mStatus = rv;
        }

        mXferState = NS_FAILED(mStatus) ? END_READ : START_READ;
        break;
      }

      case START_READ: {
        LOG(("nsFileTransport: START_READ [this=%x %s]\n", this, mStreamName.get()));

        PR_AtomicIncrement(&mService->mInUseTransports);
    
        nsCOMPtr<nsIInputStream> source;
        mStatus = mStreamIO->GetInputStream(getter_AddRefs(source));
        if (NS_FAILED(mStatus)) {
            LOG(("nsFileTransport: mStreamIO->GetInputStream() failed [this=%x rv=%x]\n",
                this, mStatus));
            mXferState = END_READ;
            return;
        }

        if (mOffset > 0) {
            // if we need to set a starting offset, QI for the nsISeekableStream
            // and set it
            nsCOMPtr<nsISeekableStream> ras = do_QueryInterface(source, &mStatus);
            if (NS_FAILED(mStatus)) {
                mXferState = END_READ;
                return;
            }
            // for now, assume the offset is always relative to the start of the
            // file (position 0) so use PR_SEEK_SET
            mStatus = ras->Seek(PR_SEEK_SET, mOffset);
            if (NS_FAILED(mStatus)) {
                mXferState = END_READ;
                return;
            }
        }

        if (!mSourceWrapper) {
            //
            // Allocate an input stream wrapper to capture the number of bytes
            // read from source.
            //
            NS_NEWXPCOM(mSourceWrapper, nsFileTransportSourceWrapper);
            if (!mSourceWrapper) {
                mStatus = NS_ERROR_OUT_OF_MEMORY;
                mXferState = END_READ;
                return;
            }
            NS_ADDREF(mSourceWrapper);
            mSourceWrapper->SetSource(source);
        }

        // capture the total amount for progress information
        if (mTransferAmount < 0) {
            mTransferAmount = mTotalAmount;
        } else
            mTotalAmount = mTransferAmount;

        mXferState = READING;
        break;
      }

      case READING: {
        //
        // Read at most mBufferMaxSize.
        //
        PRInt32 transferAmt = mBufferMaxSize;
        if (mTransferAmount >= 0)
            transferAmt = PR_MIN(transferAmt, mTransferAmount);

        LOG(("nsFileTransport: READING [this=%x %s] transferAmt=%u mBufferMaxSize=%u\n",
            this, mStreamName.get(), transferAmt, mBufferMaxSize));

        // Zero the number of bytes read on the source wrapper
        mSourceWrapper->ZeroBytesRead();

        //
        // Give the listener a chance to read at most transferAmt bytes from
        // the source input stream.
        //
        nsresult status = mListener->OnDataAvailable(this, mContext,
                                                     mSourceWrapper,
                                                     mOffset, transferAmt);

        //
        // Handle the various return codes.
        //
        if (status == NS_BASE_STREAM_WOULD_BLOCK) {
            LOG(("nsFileTransport: READING [this=%x %s] listener would block; suspending self.\n",
                this, mStreamName.get()));
            mStatus = NS_OK;
            PR_AtomicIncrement(&mSuspendCount);
        }
        else if (status == NS_BASE_STREAM_CLOSED) {
            LOG(("nsFileTransport: READING [this=%x %s] done reading file.\n",
                this, mStreamName.get()));
            mStatus = NS_OK;
            mXferState = END_READ;
        }
        else if (NS_FAILED(status)) {
            LOG(("nsFileTransport: READING [this=%x %s] error reading file.\n",
                this, mStreamName.get()));
            mStatus = status;
            mXferState = END_READ;
        }
        else {
            // 
            // get the number of bytes read
            //
            PRUint32 total = mSourceWrapper->GetBytesRead();
            mOffset += total;

            if (mTransferAmount > 0)
                mTransferAmount -= total;

            if (0 == total || 0 == mTransferAmount) {
                LOG(("nsFileTransport: READING [this=%x %s] done reading file.\n",
                    this, mStreamName.get()));
                mXferState = END_READ;
            }
            else
                LOG(("nsFileTransport: READING [this=%x %s] read %u bytes [offset=%u]\n",
                    this, mStreamName.get(), total, mOffset));

// what about check for background flags! dougt
            if (mProgress && (mTransferAmount >= 0)) {
                mProgress->OnProgress(this, mContext,
                                      mTotalAmount - mTransferAmount,
                                      mTotalAmount);
            }
        }
        break;
      }

      case END_READ: {
        
        PR_AtomicDecrement(&mService->mInUseTransports);

        LOG(("nsFileTransport: END_READ [this=%x %s] status=%x\n",
            this, mStreamName.get(), mStatus));

#if defined (DEBUG_dougt) || defined (DEBUG_warren)
        NS_ASSERTION(mTransferAmount <= 0 || NS_FAILED(mStatus), "didn't transfer all the data");
#endif 
        if (mTransferAmount > 0 && NS_SUCCEEDED(mStatus)) {
            //
            // This happens when the requested read amount is more than the amount
            // of the data in the stream/file, or if the listener returned 
            // NS_BASE_STREAM_CLOSED.
            //
            mStatus = NS_BASE_STREAM_CLOSED;
        }

        if (mListener) {
            mListener->OnStopRequest(this, mContext, mStatus, nsnull);
            mListener = 0;
        }
// what about check for background flag! dougt
        if (mProgress) {
            nsAutoString fileName;
            fileName.AssignWithConversion(mStreamName);
            mProgress->OnStatus(this, mContext, 
                                NS_NET_STATUS_READ_FROM, 
                                fileName.GetUnicode());
        }
        mContext = 0;

        // close the data source
        NS_IF_RELEASE(mSourceWrapper);
        mSourceWrapper = nsnull;
        mXferState = CLOSING;
        break;
      }

      case OPEN_FOR_WRITE: {
        LOG(("nsFileTransport: OPEN_FOR_WRITE [this=%x %s]\n",
            this, mStreamName.get()));
        mStatus = mStreamIO->Open(&mContentType, &mTotalAmount);
        
        if (mStatus == NS_ERROR_FILE_NOT_FOUND)
            mStatus = NS_OK;

        if (mProvider) {
            // always send the start notification
            nsresult rv = mProvider->OnStartRequest(this, mContext);
            if (NS_SUCCEEDED(mStatus))
                mStatus = rv;
        }

        mXferState = NS_FAILED(mStatus) ? END_WRITE : START_WRITE;
        break;
      }

      case START_WRITE: {
        LOG(("nsFileTransport: START_WRITE [this=%x %s]\n",
            this, mStreamName.get()));

        PR_AtomicIncrement(&mService->mInUseTransports);

        mStatus = mStreamIO->GetOutputStream(getter_AddRefs(mSink));
        if (NS_FAILED(mStatus)) {
            mXferState = END_WRITE;
            return;
        }

        if (mOffset > 0) {
            // If we need to set a starting offset, QI for the nsISeekableStream
            // and set it.
            nsCOMPtr<nsISeekableStream> ras = do_QueryInterface(mSink, &mStatus);
            if (NS_FAILED(mStatus)) {
                mXferState = END_WRITE;
                return;
            }
            // For now, assume the offset is always relative to the start of the
            // file (position 0) so use PR_SEEK_SET.
            mStatus = ras->Seek(PR_SEEK_SET, mOffset);
            if (NS_FAILED(mStatus)) {
                mXferState = END_WRITE;
                return;
            }
            mOffset = 0;
        }

        if (!mSinkWrapper) {
            //
            // Allocate an output stream wrapper to capture the number of bytes
            // written to mSink.
            //
            NS_NEWXPCOM(mSinkWrapper, nsFileTransportSinkWrapper);
            if (!mSinkWrapper) {
                mStatus = NS_ERROR_OUT_OF_MEMORY;
                mXferState = END_WRITE;
                return;
            }
            NS_ADDREF(mSinkWrapper);
            mSinkWrapper->SetSink(mSink);
        }

        mXferState = WRITING;
        break;
      }

      case WRITING: {
        //
        // Write at most mBufferMaxSize
        //
        PRUint32 transferAmt = mBufferMaxSize;
        if (mTransferAmount >= 0)
            transferAmt = PR_MIN(transferAmt, (PRUint32)mTransferAmount);

        // Zero the number of bytes written to the sink wrapper.
        mSinkWrapper->ZeroBytesWritten();

        // 
        // Ask the provider for data
        //
        nsresult status = mProvider->OnDataWritable(this, mContext,
                                                    mSinkWrapper,
                                                    mOffset, transferAmt);
        
        //
        // Handle the various return codes.
        //
        if (status == NS_BASE_STREAM_WOULD_BLOCK) {
            LOG(("nsFileTransport: WRITING [this=%x %s] provider would block; suspending self.\n",
                this, mStreamName.get()));
            mStatus = NS_OK;
            PR_AtomicIncrement(&mSuspendCount);
        }
        else if (status == NS_BASE_STREAM_CLOSED) {
            LOG(("nsFileTransport: WRITING [this=%x %s] no more data to be written.\n",
                this, mStreamName.get()));
            mStatus = NS_OK;
            mXferState = END_WRITE;
        }
        else if (NS_FAILED(status)) {
            LOG(("nsFileTransport: WRITING [this=%x %s] provider failed.\n",
                this, mStreamName.get()));
            mStatus = status;
            mXferState = END_WRITE;
        }
        else {
            //
            // Get the number of bytes written
            //
            PRUint32 total = mSinkWrapper->GetBytesWritten();
            mOffset += total;

            if (mTransferAmount > 0)
                mTransferAmount -= total;

            if (total == 0 || mTransferAmount == 0) {
                LOG(("nsFileTransport: WRITING [this=%x %s] done writing file.\n",
                    this, mStreamName.get()));
                mXferState = END_WRITE;
            }
            else 
                LOG(("nsFileTransport: WRITING [this=%x %s] wrote %u bytes [offset=%u]\n",
                    this, mStreamName.get(), total, mOffset));
// what about check for background flag dougt!
            if (mProgress && (mTransferAmount >= 0))
                mProgress->OnProgress(this, mContext,
                                      mTotalAmount - mTransferAmount,
                                      mTotalAmount);
        }
        break;
      }

      case END_WRITE: {
        LOG(("nsFileTransport: END_WRITE [this=%x %s] status=%x\n",
            this, mStreamName.get(), mStatus));

        PR_AtomicDecrement(&mService->mInUseTransports);

#if defined (DEBUG_dougt) || defined (DEBUG_warren)
        NS_ASSERTION(mTransferAmount <= 0 || NS_FAILED(mStatus), "didn't transfer all the data");
#endif 
        if (mTransferAmount > 0 && NS_SUCCEEDED(mStatus)) {
            // This happens when the requested write amount is more than the amount
            // of the data in the stream/file.
            mStatus = NS_BASE_STREAM_CLOSED;
        }

        if (mSink) {
            mSink->Flush();
            mSink = 0;
        }
        NS_IF_RELEASE(mSinkWrapper);
        mSinkWrapper = nsnull;

        if (mProvider) {
            mProvider->OnStopRequest(this, mContext, mStatus, nsnull);
            mProvider = 0;
        }
        if (mProgress) {
            nsAutoString fileName; fileName.AssignWithConversion(mStreamName);
            nsresult rv = mProgress->OnStatus(this, mContext,
                                              NS_NET_STATUS_WROTE_TO, 
                                              fileName.GetUnicode());
            NS_ASSERTION(NS_SUCCEEDED(rv), "unexpected OnStatus failure");
        }
        mContext = 0;

        mXferState = CLOSING;
        break;
      }

      case CLOSING: {
        DoClose();
        break;
      }

      case CLOSED: {
        NS_NOTREACHED("trying to continue a quiescent file transfer");
        break;
      }
    }

    LOG(("nsFileTransport: Leaving Process [this=%x state=%x status=%x]\n",
        this, mXferState, mStatus));
}

void
nsFileTransport::DoClose(void)
{
    LOG(("nsFileTransport: CLOSING [this=%x %s] status=%x\n",
        this, mStreamName.get(), mStatus));

    // XXX closing the stream io prevents this file transport from being reused
    if (mStreamIO) {
        nsresult rv = mStreamIO->Close(mStatus);
        NS_ASSERTION(NS_SUCCEEDED(rv), "unexpected Close failure");
        mStreamIO = 0;
    }
    mXferState = CLOSED;

    PR_AtomicDecrement(&mService->mConnectedTransports);
}

NS_IMETHODIMP 
nsFileTransport::GetSecurityInfo(nsISupports * *aSecurityInfo)
{
    *aSecurityInfo = nsnull;
    return NS_OK;
}

NS_IMETHODIMP
nsFileTransport::GetNotificationCallbacks(nsIInterfaceRequestor** aCallbacks) {
    NS_ENSURE_ARG_POINTER(aCallbacks);
    *aCallbacks = mNotificationCallbacks;
    NS_IF_ADDREF(*aCallbacks);
    return NS_OK;
}

NS_IMETHODIMP
nsFileTransport::SetNotificationCallbacks(nsIInterfaceRequestor* aCallbacks,
                                          PRBool isBackground) {
    mNotificationCallbacks = aCallbacks;
    mProgress = 0;
    if (!mNotificationCallbacks || isBackground) return NS_OK;

    nsCOMPtr<nsIProgressEventSink> sink(do_GetInterface(mNotificationCallbacks));
    if (!sink) return NS_ERROR_FAILURE;

    nsresult rv;
    NS_WITH_SERVICE(nsIProxyObjectManager,
                    proxyMgr, kProxyObjectManagerCID, &rv);
    if (NS_FAILED(rv)) return rv;
        
    return proxyMgr->GetProxyForObject(NS_UI_THREAD_EVENTQ, // primordial thread - should change?
                                       NS_GET_IID(nsIProgressEventSink),
                                       sink,
                                       PROXY_ASYNC | PROXY_ALWAYS,
                                       getter_AddRefs(mProgress));
}

////////////////////////////////////////////////////////////////////////////////
