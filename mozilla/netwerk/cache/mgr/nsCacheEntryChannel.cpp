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
 * The Original Code is Mozilla Communicator client code, released
 * March 31, 1998.
 * 
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation. Portions created by Netscape are
 * Copyright (C) 1999 Netscape Communications Corporation. All
 * Rights Reserved.
 * 
 * Contributor(s): 
 *   Scott Furman, fur@netscape.com
 */

#include "nsCacheManager.h"
#include "nsCacheEntryChannel.h"
#include "nsIOutputStream.h"
#include "nsIIOService.h"
#include "nsIServiceManager.h"
#include "nsIStreamListener.h"
#include "nsMemory.h"

nsCacheEntryChannel::nsCacheEntryChannel(
        nsCachedNetData* aCacheEntry, 
        nsIChannel* aChannel,
        nsILoadGroup* aLoadGroup):
    nsChannelProxy(aChannel), 
    mCacheEntry(aCacheEntry), 
    mLoadGroup(aLoadGroup)
{
    NS_ASSERTION(aCacheEntry->mChannelCount < 0xFF, "Overflowed channel counter");
    mCacheEntry->mChannelCount++;
    NS_INIT_REFCNT();
}

nsCacheEntryChannel::~nsCacheEntryChannel()
{
    mCacheEntry->mChannelCount--;
}

NS_IMPL_THREADSAFE_ISUPPORTS3(nsCacheEntryChannel, 
                              nsITransport,
                              nsIChannel, 
                              nsIRequest)

// A proxy for nsIOutputStream
class CacheOutputStream : public nsIOutputStream {

public:
    CacheOutputStream(nsIOutputStream *aOutputStream, 
            nsCachedNetData *aCacheEntry):
        mOutputStream(aOutputStream), 
        mCacheEntry(aCacheEntry), 
        mStartTime(PR_IntervalNow())
        { NS_INIT_REFCNT(); }

    virtual ~CacheOutputStream() {
        mCacheEntry->NoteDownloadTime(mStartTime, PR_IntervalNow());
        mCacheEntry->ClearFlag(nsCachedNetData::UPDATE_IN_PROGRESS);
    }
    
    NS_DECL_ISUPPORTS

    NS_IMETHOD Close() {
        return mOutputStream->Close();
    }

    NS_IMETHOD Flush() { return mOutputStream->Flush(); }

    NS_IMETHOD
    Write(const char *aBuf, PRUint32 aCount, PRUint32 *aActualBytes) {
        nsresult rv;
    
        *aActualBytes = 0;
        rv = mOutputStream->Write(aBuf, aCount, aActualBytes);
        mCacheEntry->mLogicalLength += *aActualBytes;
        if (NS_FAILED(rv)) return rv;
        nsCacheManager::LimitCacheSize();
        return rv;
    }
    
    NS_IMETHOD
    WriteFrom(nsIInputStream *inStr, PRUint32 count, PRUint32 *_retval) {
        NS_NOTREACHED("WriteFrom");
        return NS_ERROR_NOT_IMPLEMENTED;
    }

    NS_IMETHOD
    WriteSegments(nsReadSegmentFun reader, void * closure, PRUint32 count, PRUint32 *_retval) {
        NS_NOTREACHED("WriteSegments");
        return NS_ERROR_NOT_IMPLEMENTED;
    }

    NS_IMETHOD
    GetNonBlocking(PRBool *aNonBlocking) {
        NS_NOTREACHED("GetNonBlocking");
        return NS_ERROR_NOT_IMPLEMENTED;
    }

    NS_IMETHOD
    SetNonBlocking(PRBool aNonBlocking) {
        NS_NOTREACHED("SetNonBlocking");
        return NS_ERROR_NOT_IMPLEMENTED;
    }

    NS_IMETHOD
    GetObserver(nsIOutputStreamObserver * *aObserver) {
        NS_NOTREACHED("GetObserver");
        return NS_ERROR_NOT_IMPLEMENTED;
    }

    NS_IMETHOD
    SetObserver(nsIOutputStreamObserver * aObserver) {
        NS_NOTREACHED("SetObserver");
        return NS_ERROR_NOT_IMPLEMENTED;
    }

protected:
    nsCOMPtr<nsIOutputStream> mOutputStream;
    nsCOMPtr<nsCachedNetData> mCacheEntry;

    // Time at which stream was opened
    PRIntervalTime mStartTime;
};

NS_IMPL_THREADSAFE_ISUPPORTS1(CacheOutputStream, nsIOutputStream)

NS_IMETHODIMP
nsCacheEntryChannel::Open(nsIInputStream **aInputStream)
{
    return OpenInputStream(0, -1, 0, aInputStream);
}

NS_IMETHODIMP
nsCacheEntryChannel::AsyncOpen(nsIStreamListener *aListener, nsISupports *aContext)
{
    nsCOMPtr<nsIRequest> req;
    return AsyncRead(aListener, aContext, 0, -1, 0, getter_AddRefs(req));
}

NS_IMETHODIMP
nsCacheEntryChannel::OpenOutputStream(PRUint32 offset, PRUint32 count, PRUint32 flags,
                                      nsIOutputStream* *aOutputStream)
{
    nsresult rv;
    nsCOMPtr<nsIOutputStream> baseOutputStream;
    
    nsCOMPtr<nsITransport> trans = do_QueryInterface(mChannel, &rv);
    if (NS_FAILED(rv)) return rv;

    rv = trans->OpenOutputStream(offset, count, flags,
                                 getter_AddRefs(baseOutputStream));
    if (NS_FAILED(rv)) return rv;

    mCacheEntry->NoteAccess();
    mCacheEntry->NoteUpdate();

    *aOutputStream = new CacheOutputStream(baseOutputStream, mCacheEntry);
    if (!*aOutputStream)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(*aOutputStream);
    return NS_OK;
}

NS_IMETHODIMP
nsCacheEntryChannel::OpenInputStream(PRUint32 offset, PRUint32 count, PRUint32 flags,
                                     nsIInputStream* *aInputStream)
{
    mCacheEntry->NoteAccess();

    nsresult rv;
    nsCOMPtr<nsITransport> trans = do_QueryInterface(mChannel, &rv);
    if (NS_FAILED(rv)) return rv;

    return trans->OpenInputStream(offset, count, flags, aInputStream);
}

NS_IMETHODIMP
nsCacheEntryChannel::AsyncRead(nsIStreamListener *aListener, nsISupports *aContext,
                               PRUint32 offset, PRUint32 count, PRUint32 flags,
                               nsIRequest **aResult)
{
    mCacheEntry->NoteAccess();

    nsresult rv;
    nsCOMPtr<nsITransport> trans = do_QueryInterface(mChannel, &rv);
    if (NS_FAILED(rv)) return rv;

    return trans->AsyncRead(aListener, aContext, offset, count, flags, aResult);
}

// No async writes allowed to the cache yet
NS_IMETHODIMP
nsCacheEntryChannel::AsyncWrite(nsIStreamProvider *aProvider, nsISupports *aContext,
                                PRUint32 offset, PRUint32 count, PRUint32 flags,
                                nsIRequest **aResult)
{
    NS_NOTREACHED("nsCacheEntryChannel::AsyncWrite");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsCacheEntryChannel::GetSecurityInfo(nsISupports **aResult)
{
    return mChannel->GetSecurityInfo(aResult);
}

NS_IMETHODIMP
nsCacheEntryChannel::GetNotificationCallbacks(nsIInterfaceRequestor** aCallbacks)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsCacheEntryChannel::SetNotificationCallbacks(nsIInterfaceRequestor* aCallbacks)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsCacheEntryChannel::SetNotificationCallbacks(nsIInterfaceRequestor* aCallbacks,
                                              PRBool isBackground)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsCacheEntryChannel::GetLoadGroup(nsILoadGroup* *aLoadGroup)
{
    *aLoadGroup = mLoadGroup;
    return NS_OK;
}

NS_IMETHODIMP
nsCacheEntryChannel::GetLoadAttributes(nsLoadFlags *aLoadAttributes)
{
    return mChannel->GetLoadAttributes(aLoadAttributes);
}

NS_IMETHODIMP
nsCacheEntryChannel::SetLoadAttributes(nsLoadFlags aLoadAttributes)
{
    return mChannel->SetLoadAttributes(aLoadAttributes);
}

static NS_DEFINE_CID(kIOServiceCID, NS_IOSERVICE_CID);

NS_IMETHODIMP
nsCacheEntryChannel::GetURI(nsIURI * *aURI)
{
    char* spec;
    nsresult rv;

    rv = mCacheEntry->GetUriSpec(&spec);
    if (NS_FAILED(rv)) return rv;

    NS_WITH_SERVICE(nsIIOService, serv, kIOServiceCID, &rv);
    if (NS_FAILED(rv)) return rv;
    
    rv = serv->NewURI(spec, nsnull, aURI);
    nsMemory::Free(spec);
    return rv;
}

NS_IMETHODIMP
nsCacheEntryChannel::GetOriginalURI(nsIURI * *aURI)
{
    // FIXME - should return original URI passed into NewChannel() ?
    NS_NOTREACHED("nsCacheEntryChannel::GetOriginalURI");
    return NS_ERROR_NOT_IMPLEMENTED;
}
