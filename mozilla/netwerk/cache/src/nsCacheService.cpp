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
 * The Original Code is nsCacheService.cpp, released February 10, 2001.
 * 
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are
 * Copyright (C) 2001 Netscape Communications Corporation.  All
 * Rights Reserved.
 * 
 * Contributor(s): 
 *    Gordon Sheridan, 10-February-2001
 */


#include "nsCache.h"
#include "nsCacheService.h"
#include "nsCacheRequest.h"
#include "nsCacheEntry.h"
#include "nsCacheEntryDescriptor.h"
#include "nsCacheDevice.h"
#include "nsDiskCacheDevice.h"
#include "nsMemoryCacheDevice.h"
#include "nsICacheVisitor.h"

#include "nsAutoLock.h"
#include "nsIEventQueue.h"
#include "nsIObserverService.h"
#include "nsIPref.h"

#if 0
static NS_DEFINE_CID(kEventQueueServiceCID, NS_EVENTQUEUESERVICE_CID);
static NS_DEFINE_CID(kProxyObjectManagerCID, NS_PROXYEVENT_MANAGER_CID);
#endif



/******************************************************************************
 * nsCachePrefObserver
 *****************************************************************************/

#define ENABLE_MEMORY_CACHE_PREF    "browser.cache.enable"
#define ENABLE_DISK_CACHE_PREF      "browser.cache.disk.enable"

class nsCachePrefObserver : public nsIObserver
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIOBSERVER

    nsCachePrefObserver() :
        mDiskCacheEnabled(PR_TRUE),
        mMemoryCacheEnabled(PR_TRUE)
    {
        NS_INIT_ISUPPORTS();
    }

    virtual ~nsCachePrefObserver() {}
    
    nsresult  Install();
    nsresult  Remove();

private:
    PRBool  mDiskCacheEnabled;
    PRBool  mMemoryCacheEnabled;
};

NS_IMPL_ISUPPORTS1(nsCachePrefObserver, nsIObserver);


nsresult
nsCachePrefObserver::Install()
{
    nsresult rv, rv2;
    nsCOMPtr<nsIPref> prefs = do_GetService(NS_PREF_CONTRACTID, &rv);
    if (NS_FAILED(rv)) return rv;

    rv = prefs->AddObserver(ENABLE_MEMORY_CACHE_PREF, this);
    if (NS_FAILED(rv)) rv2 = rv;

    rv = prefs->AddObserver(ENABLE_DISK_CACHE_PREF, this);
    if (NS_FAILED(rv)) rv2 = rv;

    rv = prefs->GetBoolPref(ENABLE_DISK_CACHE_PREF, &mDiskCacheEnabled);
    if (NS_FAILED(rv)) rv2 = rv;

    rv = prefs->GetBoolPref(ENABLE_MEMORY_CACHE_PREF, &mMemoryCacheEnabled);
    // if (NS_FAILED(rv)) rv2 = rv;

    if (NS_SUCCEEDED(rv)) rv = rv2;
    return rv;
}


nsresult
nsCachePrefObserver::Remove()
{
    nsresult rv, rv2;

    nsCOMPtr<nsIPref> prefs = do_GetService(NS_PREF_CONTRACTID, &rv);
    if (NS_FAILED(rv)) return rv;

    rv  = prefs->RemoveObserver(ENABLE_DISK_CACHE_PREF, this);
    rv2 = prefs->RemoveObserver(ENABLE_MEMORY_CACHE_PREF, this);

    if (NS_SUCCEEDED(rv)) rv = rv2;
    return rv;
}


NS_IMETHODIMP
nsCachePrefObserver::Observe(nsISupports *     subject,
                             const PRUnichar * topic,
                             const PRUnichar * data)
{
    nsresult rv;

    if (NS_LITERAL_STRING("nsPref:changed").Equals(topic)) {
        nsCOMPtr<nsIPref> prefs = do_QueryInterface(subject, &rv);
        if (NS_FAILED(rv)) return rv;

        // which preference changed?
        if (NS_LITERAL_STRING(ENABLE_DISK_CACHE_PREF).Equals(data)) {

            rv = prefs->GetBoolPref(ENABLE_DISK_CACHE_PREF, &mDiskCacheEnabled);

        } else if (NS_LITERAL_STRING(ENABLE_MEMORY_CACHE_PREF).Equals(data)) {

            rv = prefs->GetBoolPref(ENABLE_MEMORY_CACHE_PREF, &mMemoryCacheEnabled);
        }

        if (NS_FAILED(rv)) return rv;
        nsCacheService::GlobalInstance()->SetCacheDevicesEnabled(mDiskCacheEnabled,
                                                                 mMemoryCacheEnabled);
    }
    
    return NS_OK;
}


/******************************************************************************
 * nsCacheService
 *****************************************************************************/

nsCacheService *   nsCacheService::gService = nsnull;

NS_IMPL_THREADSAFE_ISUPPORTS2(nsCacheService, nsICacheService, nsIObserver)

nsCacheService::nsCacheService()
    : mCacheServiceLock(nsnull),
      mEnableMemoryDevice(PR_TRUE),
      mEnableDiskDevice(PR_TRUE),
      mMemoryDevice(nsnull),
      mDiskDevice(nsnull),
      mTotalEntries(0),
      mCacheHits(0),
      mCacheMisses(0),
      mMaxKeyLength(0),
      mMaxDataSize(0),
      mMaxMetaSize(0),
      mDeactivateFailures(0),
      mDeactivatedUnboundEntries(0)
{
  NS_INIT_REFCNT();

  NS_ASSERTION(gService==nsnull, "multiple nsCacheService instances!");
  gService = this;

  // create list of cache devices
  PR_INIT_CLIST(&mDoomedEntries);
}

nsCacheService::~nsCacheService()
{
    if (mCacheServiceLock) // Shutdown hasn't been called yet.
        (void) Shutdown();

    gService = nsnull;
#if DEBUG
    printf("### nsCacheService is now destroyed.\n");
#endif
}


NS_IMETHODIMP
nsCacheService::Init()
{
    nsresult  rv;

    NS_ASSERTION(mCacheServiceLock== nsnull, "nsCacheService already initialized.");
    if (mCacheServiceLock)
        return NS_ERROR_ALREADY_INITIALIZED;

    CACHE_LOG_INIT();

    mCacheServiceLock = PR_NewLock();
    if (mCacheServiceLock == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;

    // initialize hashtable for active cache entries
    rv = mActiveEntries.Init();
    if (NS_FAILED(rv)) goto error;
    
    // get references to services we'll be using frequently
    mEventQService = do_GetService(NS_EVENTQUEUESERVICE_CONTRACTID, &rv);
    if (NS_FAILED(rv)) return rv;
    
    mProxyObjectManager = do_GetService(NS_XPCOMPROXY_CONTRACTID, &rv);
    if (NS_FAILED(rv)) return rv;

    { // scope nsCOMPtr<nsIPrefService> and  nsCOMPtr<nsIPrefBranch>
        nsCOMPtr<nsIPrefService> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
        if (NS_FAILED(rv)) return rv;

        nsCOMPtr<nsIPrefBranch> prefBranch = do_QueryInterface(prefs, &rv);
        if (NS_SUCCEEDED(rv)) {
            rv = prefBranch->GetBoolPref(ENABLE_DISK_CACHE_PREF, &mEnableDiskDevice);
            rv = prefBranch->GetBoolPref(ENABLE_MEMORY_CACHE_PREF, &mEnableMemoryDevice);
            // ignore errors
        }
    }

    if (mEnableMemoryDevice) {   // create memory cache
        mMemoryDevice = new nsMemoryCacheDevice;
        if (!mMemoryDevice) {
            rv = NS_ERROR_OUT_OF_MEMORY;
            goto error;
        }
        rv = mMemoryDevice->Init();
        if (NS_FAILED(rv)) {
            // XXX log error
            delete mMemoryDevice;
            mMemoryDevice = nsnull;
        }
    }

#if EAGER_DISK_INIT
    if (mEnableDiskCache) {   // create disk cache
        mDiskDevice = new nsDiskCacheDevice;
        if (!mDiskDevice) {
            rv = NS_ERROR_OUT_OF_MEMORY;
            goto error;
        }
        rv = mDiskDevice->Init();
        if (NS_FAILED(rv)) goto error;
    }
#endif

    // observer XPCOM shutdown.
    {
        nsCOMPtr<nsIObserverService> observerService = do_GetService(NS_OBSERVERSERVICE_CONTRACTID, &rv);
        if (NS_SUCCEEDED(rv)) {
            observerService->AddObserver(this, NS_LITERAL_STRING(NS_XPCOM_SHUTDOWN_OBSERVER_ID).get());
        }
    }
    
    return NS_OK;

 error:
    (void)Shutdown();

    return rv;
}


NS_IMETHODIMP
nsCacheService::Shutdown()
{
    NS_ASSERTION(mCacheServiceLock != nsnull, 
                 "can't shutdown nsCacheService unless it has been initialized.");

    if (mCacheServiceLock) {
        // XXX this is not sufficient
        PRLock * tempLock = mCacheServiceLock;
        mCacheServiceLock = nsnull;
#if DEBUG
        printf("### beging nsCacheService::Shutdown()\n");
#endif

#if defined(PR_LOGGING)
        LogCacheStatistics();
#endif
        // Clear entries
        ClearDoomList();
        ClearActiveEntries();

        // deallocate memory and disk caches
        delete mMemoryDevice;
        mMemoryDevice = nsnull;

        delete mDiskDevice;
        mDiskDevice = nsnull;

        PR_DestroyLock(tempLock);
    }
    return NS_OK;
}


NS_METHOD
nsCacheService::Create(nsISupports* aOuter, const nsIID& aIID, void* *aResult)
{
    nsresult  rv;

    if (aOuter != nsnull)
        return NS_ERROR_NO_AGGREGATION;

    nsCacheService* cacheService = new nsCacheService();
    if (cacheService == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(cacheService);
    rv = cacheService->Init();
    if (NS_SUCCEEDED(rv)) {
        rv = cacheService->QueryInterface(aIID, aResult);
    }
    NS_RELEASE(cacheService);
    return rv;
}


NS_IMETHODIMP
nsCacheService::CreateSession(const char *          clientID,
                              nsCacheStoragePolicy  storagePolicy, 
                              PRBool                streamBased,
                              nsICacheSession     **result)
{
    *result = nsnull;

    if ((this == nsnull) || !(mEnableDiskDevice || mEnableMemoryDevice))
        return NS_ERROR_NOT_AVAILABLE;

    nsCacheSession * session = new nsCacheSession(clientID, storagePolicy, streamBased);
    if (!session)  return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(*result = session);

    return NS_OK;
}


nsresult
nsCacheService::EvictEntriesForSession(nsCacheSession * session)
{
    return EvictEntriesForClient(session->ClientID()->get(),
                                 session->StoragePolicy());
}


nsresult
nsCacheService::EvictEntriesForClient(const char *          clientID,
                                      nsCacheStoragePolicy  storagePolicy)
{
    if (this == nsnull) return NS_ERROR_NOT_AVAILABLE;
    nsAutoLock lock(mCacheServiceLock);
    nsresult rv = NS_OK;

    if (storagePolicy == nsICache::STORE_ANYWHERE ||
        storagePolicy == nsICache::STORE_ON_DISK) {

        if (mEnableDiskDevice) {
            if (!mDiskDevice) {
                rv = CreateDiskDevice();
                if (NS_FAILED(rv)) return rv;
            }
            rv = mDiskDevice->EvictEntries(clientID);
            if (NS_FAILED(rv)) return rv;
        }
    }

    if (storagePolicy == nsICache::STORE_ANYWHERE ||
        storagePolicy == nsICache::STORE_IN_MEMORY) {

        if (mEnableMemoryDevice) {
            rv = mMemoryDevice->EvictEntries(clientID);
            if (NS_FAILED(rv)) return rv;
        }
    }

    return NS_OK;
}


NS_IMETHODIMP nsCacheService::VisitEntries(nsICacheVisitor *visitor)
{
    nsAutoLock lock(mCacheServiceLock);

    if (!(mEnableDiskDevice || mEnableMemoryDevice))
        return NS_ERROR_NOT_AVAILABLE;

    // XXX record the fact that a visitation is in progress, i.e. keep
    // list of visitors in progress.
    
    nsresult rv = NS_OK;
    if (mEnableMemoryDevice) {
        rv = mMemoryDevice->Visit(visitor);
        if (NS_FAILED(rv)) return rv;
    }

    if (mEnableDiskDevice) {
        if (!mDiskDevice) {
            rv = CreateDiskDevice();
            if (NS_FAILED(rv)) return rv;
        }
        rv = mDiskDevice->Visit(visitor);
        if (NS_FAILED(rv)) return rv;
    }

    // XXX notify any shutdown process that visitation is complete for THIS visitor.
    // XXX keep queue of visitors

    return NS_OK;
}


NS_IMETHODIMP nsCacheService::EvictEntries(nsCacheStoragePolicy storagePolicy)
{
    return  EvictEntriesForClient(nsnull, storagePolicy);
}


/**
 * Internal Methods
 */
nsresult
nsCacheService::CreateDiskDevice()
{
    if (!mEnableDiskDevice) return NS_ERROR_NOT_AVAILABLE;
    if (mDiskDevice)        return NS_OK;

    mDiskDevice = new nsDiskCacheDevice;
    if (!mDiskDevice)       return NS_ERROR_OUT_OF_MEMORY;

    nsresult rv = mDiskDevice->Init();
    if (NS_FAILED(rv)) {
#if DEBUG
        printf("###\n");
        printf("### mDiskDevice->Init() failed (0x%.8x)\n", rv);
        printf("###    - disabling disk cache for this session.\n");
        printf("###\n");
#endif        
        mEnableDiskDevice = PR_FALSE;
        delete mDiskDevice;
        mDiskDevice = nsnull;
    }
    return rv;
}


nsresult
nsCacheService::CreateMemoryDevice()
{
    if (!mEnableMemoryDevice) return NS_ERROR_NOT_AVAILABLE;
    if (mMemoryDevice)        return NS_OK;

    mMemoryDevice = new nsMemoryCacheDevice;
    if (!mMemoryDevice)       return NS_ERROR_OUT_OF_MEMORY;

    nsresult rv = mMemoryDevice->Init();
    if (NS_FAILED(rv)) {
        // XXX log error
        delete mMemoryDevice;
        mMemoryDevice = nsnull;
    }
    return rv;
}


nsresult
nsCacheService::CreateRequest(nsCacheSession *   session,
                              const char *       clientKey,
                              nsCacheAccessMode  accessRequested,
                              nsICacheListener * listener,
                              nsCacheRequest **  request)
{
    NS_ASSERTION(request, "CommonOpenCacheEntry: request or entry is null");
     
    nsCString * key = new nsCString(*session->ClientID());
    if (!key)
        return NS_ERROR_OUT_OF_MEMORY;
    key->Append(":");
    key->Append(clientKey);

    if (mMaxKeyLength < key->Length()) mMaxKeyLength = key->Length();

    // create request
    *request = new  nsCacheRequest(key, listener, accessRequested, session);    
    if (!*request) {
        delete key;
        return NS_ERROR_OUT_OF_MEMORY;
    }

    if (!listener)  return NS_OK;  // we're sync, we're done.

    // get the nsIEventQueue for the request's thread
    nsresult  rv;
#if 0
    // XXX can we just keep a reference so we don't have to do this everytime?
    NS_WITH_SERVICE(nsIEventQueueService, eventQService, kEventQueueServiceCID, &rv);
    if (NS_FAILED(rv))  goto error;
#endif
    
    rv = mEventQService->ResolveEventQueue(NS_CURRENT_EVENTQ,
                                          getter_AddRefs((*request)->mEventQ));
    if (NS_FAILED(rv))  goto error;
    
    
    if (!(*request)->mEventQ) {
        rv = NS_ERROR_UNEXPECTED; // XXX what is the right error?
        goto error;
    }

    return NS_OK;

 error:
    delete *request;
    *request = nsnull;
    return rv;
}


nsresult
nsCacheService::NotifyListener(nsCacheRequest *          request,
                               nsICacheEntryDescriptor * descriptor,
                               nsCacheAccessMode         accessGranted,
                               nsresult                  error)
{
    nsresult rv;

#if 0
    // XXX can we hold onto the proxy object manager?
    NS_WITH_SERVICE(nsIProxyObjectManager, proxyObjMgr, kProxyObjectManagerCID, &rv);
    if (NS_FAILED(rv)) return rv;
#endif

    nsCOMPtr<nsICacheListener> listenerProxy;
    NS_ASSERTION(request->mEventQ, "no event queue for async request!");
    rv = mProxyObjectManager->GetProxyForObject(request->mEventQ,
                                                NS_GET_IID(nsICacheListener),
                                                request->mListener,
                                                PROXY_ASYNC|PROXY_ALWAYS,
                                                getter_AddRefs(listenerProxy));
    if (NS_FAILED(rv)) return rv;

    return listenerProxy->OnCacheEntryAvailable(descriptor, accessGranted, error);
}


nsresult
nsCacheService::ProcessRequest(nsCacheRequest * request,
                               nsICacheEntryDescriptor ** result)
{
    // !!! must be called with mCacheServiceLock held !!!
    nsresult           rv;
    nsCacheEntry *     entry = nsnull;
    nsCacheAccessMode  accessGranted = nsICache::ACCESS_NONE;
    if (result) *result = nsnull;

    while(1) {  // Activate entry loop
        rv = ActivateEntry(request, &entry);  // get the entry for this request
        if (NS_FAILED(rv))  break;

        while(1) { // Request Access loop
            NS_ASSERTION(entry, "no entry in Request Access loop!");
            // entry->RequestAccess queues request on entry
            rv = entry->RequestAccess(request, &accessGranted);
            if (rv != NS_ERROR_CACHE_WAIT_FOR_VALIDATION) break;
            
            if (request->mListener) // async exits - validate, doom, or close will resume
                return rv; 
            
            // XXX allocate condvar for request if necessary
            PR_Unlock(mCacheServiceLock);
            rv = request->WaitForValidation();
            PR_Lock(mCacheServiceLock);

            PR_REMOVE_AND_INIT_LINK(request);
            if (NS_FAILED(rv)) break;
            // okay, we're ready to process this request, request access again
        }
        if (rv != NS_ERROR_CACHE_ENTRY_DOOMED)  break;

        if (entry->IsNotInUse()) {
            // this request was the last one keeping it around, so get rid of it
            DeactivateEntry(entry);
        }
        // loop back around to look for another entry
    }

    nsCOMPtr<nsICacheEntryDescriptor> descriptor;
    
    if (NS_SUCCEEDED(rv))
        rv = entry->CreateDescriptor(request, accessGranted, getter_AddRefs(descriptor));

    if (request->mListener) {  // Asynchronous
        // call listener to report error or descriptor
        nsresult rv2 = NotifyListener(request, descriptor, accessGranted, rv);
        if (NS_FAILED(rv2) && NS_SUCCEEDED(rv)) {
            rv = rv2;  // trigger delete request
        }
    } else {        // Synchronous
        NS_IF_ADDREF(*result = descriptor);
    }
    return rv;
}


nsresult
nsCacheService::OpenCacheEntry(nsCacheSession *           session,
                                const char *               key,
                                nsCacheAccessMode          accessRequested,
                                nsICacheListener *         listener,
                                nsICacheEntryDescriptor ** result)
{
    if (this == nsnull)  return NS_ERROR_NOT_AVAILABLE;
    if (result)
        *result = nsnull;

    nsCacheRequest * request = nsnull;

    nsresult rv = CreateRequest(session, key, accessRequested, listener, &request);
    if (NS_FAILED(rv))  return rv;

    nsAutoLock lock(mCacheServiceLock);
    rv = ProcessRequest(request, result);

    // delete requests that have completed
    if (!(listener && (rv == NS_ERROR_CACHE_WAIT_FOR_VALIDATION)))
        delete request;

    return rv;
}


nsresult
nsCacheService::ActivateEntry(nsCacheRequest * request, 
                              nsCacheEntry ** result)
{
    nsresult        rv = NS_OK;

    NS_ASSERTION(request != nsnull, "ActivateEntry called with no request");
    if (result) *result = nsnull;
    if ((!request) || (!result))  return NS_ERROR_NULL_POINTER;


    // search active entries (including those not bound to device)
    nsCacheEntry *entry = mActiveEntries.GetEntry(request->mKey);

    if (!entry) {
        // search cache devices for entry
        entry = SearchCacheDevices(request->mKey, request->StoragePolicy());
        if (entry)  entry->MarkInitialized();
    }

    if (entry) {
        ++mCacheHits;
        entry->Fetched();
    } else {
        ++mCacheMisses;
    }
    if (!entry && !(request->AccessRequested() & nsICache::ACCESS_WRITE)) {
        // this is a READ-ONLY request
        rv = NS_ERROR_CACHE_KEY_NOT_FOUND;
        goto error;
    }

    if (entry &&
        ((request->AccessRequested() == nsICache::ACCESS_WRITE) ||
         (entry->mExpirationTime &&
          entry->mExpirationTime < SecondsFromPRTime(PR_Now()) &&
          request->WillDoomEntriesIfExpired())))
    {
        // this is FORCE-WRITE request or the entry has expired
        rv = DoomEntry_Locked(entry);
        if (NS_FAILED(rv)) {
            // XXX what to do?  Increment FailedDooms counter?
        }
        entry = nsnull;
    }

    if (!entry) {
        entry = new nsCacheEntry(request->mKey,
                                 request->IsStreamBased(),
                                 request->StoragePolicy());
        if (!entry)
            return NS_ERROR_OUT_OF_MEMORY;
        
        entry->Fetched();
        ++mTotalEntries;
        
        // XXX  we could perform an early bind in some cases based on storage policy
    }

    if (!entry->IsActive()) {
        rv = mActiveEntries.AddEntry(entry);
        if (NS_FAILED(rv)) goto error;
        entry->MarkActive();  // mark entry active, because it's now in mActiveEntries
    }
    *result = entry;
    return NS_OK;
    
 error:
    *result = nsnull;
    if (entry) {
        // XXX clean up
        delete entry;
    }
    return rv;
}


nsCacheEntry *
nsCacheService::SearchCacheDevices(nsCString * key, nsCacheStoragePolicy policy)
{
    nsCacheEntry * entry = nsnull;

    if ((policy == nsICache::STORE_ANYWHERE) || (policy == nsICache::STORE_IN_MEMORY)) {
        if (mEnableMemoryDevice)
            entry = mMemoryDevice->FindEntry(key);
    }

    if (!entry && 
        ((policy == nsICache::STORE_ANYWHERE) || (policy == nsICache::STORE_ON_DISK))) {

        if (mEnableDiskDevice) {
            if (!mDiskDevice) {
                nsresult rv = CreateDiskDevice();
                if (NS_FAILED(rv))
                    return nsnull;
            }
            
            entry = mDiskDevice->FindEntry(key);
        }
    }

    return entry;
}


nsCacheDevice *
nsCacheService::EnsureEntryHasDevice(nsCacheEntry * entry)
{
    nsCacheDevice * device = entry->CacheDevice();
    if (device)  return device;
    nsresult rv = NS_OK;

    if (entry->IsStreamData() && entry->IsAllowedOnDisk() && mEnableDiskDevice) {
        // this is the default
        if (!mDiskDevice) {
            rv = CreateDiskDevice();  // ignore the error (check for mDiskDevice instead)
        }

        if (mDiskDevice) {
            entry->MarkBinding();  // XXX
            rv = mDiskDevice->BindEntry(entry);
            entry->ClearBinding(); // XXX
            if (NS_SUCCEEDED(rv))
                device = mDiskDevice;
        }
    }
     
    // if we can't use mDiskDevice, try mMemoryDevice
    if (!device && mEnableMemoryDevice) {
        NS_ASSERTION(entry->IsAllowedInMemory(), "oops.. bad flags");
        
        entry->MarkBinding();  // XXX
        rv = mMemoryDevice->BindEntry(entry);
        entry->ClearBinding(); // XXX
        if (NS_SUCCEEDED(rv))
            device = mMemoryDevice;
    }

    if (device == nsnull)  return nsnull;

    entry->SetCacheDevice(device);
    return device;
}


nsresult
nsCacheService::ValidateEntry(nsCacheEntry * entry)
{
    if (this == nsnull)  return NS_ERROR_NOT_AVAILABLE;

    nsAutoLock lock(mCacheServiceLock);
    nsCacheDevice * device = EnsureEntryHasDevice(entry);
    if (!device)  return  NS_ERROR_UNEXPECTED; // XXX need better error here

    entry->MarkValid();
    nsresult rv = ProcessPendingRequests(entry);
    NS_ASSERTION(rv == NS_OK, "ProcessPendingRequests failed.");
    // XXX what else can be done?

    return rv;
}


nsresult
nsCacheService::DoomEntry(nsCacheEntry * entry)
{
    if (this == nsnull)  return NS_ERROR_NOT_AVAILABLE;
    nsAutoLock lock(mCacheServiceLock);
    return DoomEntry_Locked(entry);
}


nsresult
nsCacheService::DoomEntry_Locked(nsCacheEntry * entry)
{
    if (this == nsnull)  return NS_ERROR_NOT_AVAILABLE;
    if (entry->IsDoomed())  return NS_OK;
    
    nsresult  rv = NS_OK;
    entry->MarkDoomed();
    
    NS_ASSERTION(!entry->IsBinding(), "Dooming entry while binding device.");
    nsCacheDevice * device = entry->CacheDevice();
    if (device)  device->DoomEntry(entry);

    if (entry->IsActive()) {
        // remove from active entries
        mActiveEntries.RemoveEntry(entry);
        entry->MarkInactive();
     }

    // put on doom list to wait for descriptors to close
    NS_ASSERTION(PR_CLIST_IS_EMPTY(entry), "doomed entry still on device list");
    PR_APPEND_LINK(entry, &mDoomedEntries);

    // tell pending requests to get on with their lives...
    rv = ProcessPendingRequests(entry);
    
    // All requests have been removed, but there may still be open descriptors
    if (entry->IsNotInUse()) {
        DeactivateEntry(entry); // tell device to get rid of it
    }
    return rv;
}


void
nsCacheService::SetCacheDevicesEnabled(PRBool  enableDisk, PRBool  enableMemory)
{
    if (this == nsnull) return;  // NS_ERROR_NOT_AVAILABLE;
    nsAutoLock lock(mCacheServiceLock);
    
    if (enableDisk && !mDiskDevice) {
        // disk device requires lazy activation
    } else if (!enableDisk && mDiskDevice) {
        // XXX deactivate disk
    }

    if (enableMemory && !mMemoryDevice) {
        // XXX enable memory cache device
    } else if (!enableMemory && mMemoryDevice) {
        // XXX disable memory cache device
    }
}


nsresult
nsCacheService::SetCacheElement(nsCacheEntry * entry, nsISupports * element)
{
    nsresult rv;
    nsIEventQueue *  eventQ;
    rv = mEventQService->ResolveEventQueue(NS_CURRENT_EVENTQ, &eventQ);
    if (NS_FAILED(rv))  return rv;

    entry->SetEventQ(eventQ);
    entry->SetData(element);
    entry->TouchData();
    return NS_OK;
}


nsresult
nsCacheService::OnDataSizeChange(nsCacheEntry * entry, PRInt32 deltaSize)
{
    if (this == nsnull)  return NS_ERROR_NOT_AVAILABLE;
    nsAutoLock lock(mCacheServiceLock);

    nsCacheDevice * device = EnsureEntryHasDevice(entry);
    if (!device)  return  NS_ERROR_UNEXPECTED; // XXX need better error here

    return device->OnDataSizeChange(entry, deltaSize);
}


nsresult
nsCacheService::GetTransportForEntry(nsCacheEntry *     entry,
                                     nsCacheAccessMode  mode,
                                     nsITransport    ** result)
{
    if (this == nsnull)  return NS_ERROR_NOT_AVAILABLE;
    nsAutoLock lock(mCacheServiceLock);

    nsCacheDevice * device = EnsureEntryHasDevice(entry);
    if (!device)  return  NS_ERROR_UNEXPECTED; // XXX need better error here

    return device->GetTransportForEntry(entry, mode, result);
}


void
nsCacheService::CloseDescriptor(nsCacheEntryDescriptor * descriptor)
{
    NS_ASSERTION(this != nsnull, "CloseDescriptor called with no cache service!");
    if (this == nsnull)  return;
    nsAutoLock lock(mCacheServiceLock);

    // ask entry to remove descriptor
    nsCacheEntry * entry       = descriptor->CacheEntry();
    PRBool         stillActive = entry->RemoveDescriptor(descriptor);
    nsresult       rv          = NS_OK;

    if (!entry->IsValid()) {
        rv = ProcessPendingRequests(entry);
    }

    if (!stillActive) {
        DeactivateEntry(entry);
    }
}


nsresult        
nsCacheService::GetFileForEntry(nsCacheEntry *         entry,
                                nsIFile **             result)
{
    if (this == nsnull)  return NS_ERROR_NOT_AVAILABLE;
    nsAutoLock lock(mCacheServiceLock);
    nsCacheDevice * device = EnsureEntryHasDevice(entry);
    if (!device)  return  NS_ERROR_UNEXPECTED; // XXX need better error here
    return device->GetFileForEntry(entry, result);
}

void
nsCacheService::DeactivateEntry(nsCacheEntry * entry)
{
    nsresult  rv = NS_OK;
    NS_ASSERTION(entry->IsNotInUse(), "### deactivating an entry while in use!");

    if (mMaxDataSize < entry->DataSize() )     mMaxDataSize = entry->DataSize();
    if (mMaxMetaSize < entry->MetaDataSize() ) mMaxMetaSize = entry->MetaDataSize();

    if (entry->IsDoomed()) {
        // remove from Doomed list
        PR_REMOVE_AND_INIT_LINK(entry);
    } else {
        if (entry->IsActive()) {
            // remove from active entries
            mActiveEntries.RemoveEntry(entry);
            entry->MarkInactive();
        } else {
            // XXX bad state
        }
    }

    nsCacheDevice * device = entry->CacheDevice();
    if (device) {
        rv = device->DeactivateEntry(entry);
        if (NS_FAILED(rv)) {
            // increment deactivate failure count
            ++mDeactivateFailures;
        }
    } else {
        // increment deactivating unbound entry statistic
        ++mDeactivatedUnboundEntries;
        delete entry; // because no one else will
    }
}


nsresult
nsCacheService::ProcessPendingRequests(nsCacheEntry * entry)
{
    nsresult  rv = NS_OK;

    if (!entry->IsDoomed() && entry->IsInvalid()) {
        // 1st descriptor closed w/o MarkValid()
        // XXX assert no open descriptors
        // XXX find best requests to promote to 1st Writer
        // XXX process ACCESS_WRITE (shouldn't have any of these)
        // XXX ACCESS_READ_WRITE, ACCESS_READ
        NS_ASSERTION(PR_TRUE,
                     "closing descriptors w/o calling MarkValid is not supported yet.");
    }

    nsCacheRequest *   request = (nsCacheRequest *)PR_LIST_HEAD(&entry->mRequestQ);
    nsCacheRequest *   nextRequest;
    nsCacheAccessMode  accessGranted = nsICache::ACCESS_NONE;

    while (request != &entry->mRequestQ) {
        nextRequest = (nsCacheRequest *)PR_NEXT_LINK(request);

        if (request->mListener) {

            // Async request
            PR_REMOVE_AND_INIT_LINK(request);

            if (entry->IsDoomed()) {
                rv = ProcessRequest(request, nsnull);
                if (rv == NS_ERROR_CACHE_WAIT_FOR_VALIDATION)
                    rv = NS_OK;
                else
                    delete request;

                if (NS_FAILED(rv)) {
                    // XXX what to do?
                }
            } else if (entry->IsValid()) {
                rv = entry->RequestAccess(request, &accessGranted);
                NS_ASSERTION(NS_SUCCEEDED(rv),
                             "if entry is valid, RequestAccess must succeed.");

                // entry->CreateDescriptor dequeues request, and queues descriptor
                nsCOMPtr<nsICacheEntryDescriptor> descriptor;
                rv = entry->CreateDescriptor(request,
                                             accessGranted,
                                             getter_AddRefs(descriptor));
                
                // post call to listener to report error or descriptor
                rv = NotifyListener(request, descriptor, accessGranted, rv);
                delete request;
                if (NS_FAILED(rv)) {
                    // XXX what to do?
                }
                
            } else {
                // XXX bad state
            }
        } else {

            // Synchronous request
            request->WakeUp();
        }
        request = nextRequest;
    }

    return NS_OK;
}


void
nsCacheService::ClearPendingRequests(nsCacheEntry * entry)
{
    nsCacheRequest * request = (nsCacheRequest *)PR_LIST_HEAD(&entry->mRequestQ);
    
    while (request != &entry->mRequestQ) {
        nsCacheRequest * next = (nsCacheRequest *)PR_NEXT_LINK(request);

        // XXX we're just dropping these on the floor for now...definitely wrong.
        PR_REMOVE_AND_INIT_LINK(request);
        delete request;
        request = next;
    }
}


void
nsCacheService::ClearDoomList()
{
    nsCacheEntry * entry = (nsCacheEntry *)PR_LIST_HEAD(&mDoomedEntries);

    while (entry != &mDoomedEntries) {
        nsCacheEntry * next = (nsCacheEntry *)PR_NEXT_LINK(entry);
        
         entry->DetachDescriptors();
         DeactivateEntry(entry);
         entry = next;
    }        
}


void
nsCacheService::ClearActiveEntries()
{
    // XXX really we want a different finalize callback for mActiveEntries
    PL_DHashTableEnumerate(&mActiveEntries.table, DeactivateAndClearEntry, nsnull);
    mActiveEntries.Shutdown();
}


PLDHashOperator PR_CALLBACK
nsCacheService::DeactivateAndClearEntry(PLDHashTable *    table,
                                        PLDHashEntryHdr * hdr,
                                        PRUint32          number,
                                        void *            arg)
{
    nsCacheEntry * entry = ((nsCacheEntryHashTableEntry *)hdr)->cacheEntry;
    NS_ASSERTION(entry, "### active entry = nsnull!");
    gService->ClearPendingRequests(entry);
    entry->DetachDescriptors();
    gService->DeactivateEntry(entry);
    
    return PL_DHASH_REMOVE; // and continue enumerating
}


NS_IMETHODIMP nsCacheService::Observe(nsISupports *aSubject, const PRUnichar *aTopic, const PRUnichar *someData)
{
    return Shutdown();
}

#if defined(PR_LOGGING)
void
nsCacheService::LogCacheStatistics()
{
    PRUint32 hitPercentage = (PRUint32)((((double)mCacheHits) /
        ((double)(mCacheHits + mCacheMisses))) * 100);
    CACHE_LOG_ALWAYS(("\nCache Service Statistics:\n\n"));
    CACHE_LOG_ALWAYS(("    TotalEntries   = %d\n", mTotalEntries));
    CACHE_LOG_ALWAYS(("    Cache Hits     = %d\n", mCacheHits));
    CACHE_LOG_ALWAYS(("    Cache Misses   = %d\n", mCacheMisses));
    CACHE_LOG_ALWAYS(("    Cache Hit %%    = %d%%\n", hitPercentage));
    CACHE_LOG_ALWAYS(("    Max Key Length = %d\n", mMaxKeyLength));
    CACHE_LOG_ALWAYS(("    Max Meta Size  = %d\n", mMaxMetaSize));
    CACHE_LOG_ALWAYS(("    Max Data Size  = %d\n", mMaxDataSize));
    CACHE_LOG_ALWAYS(("\n"));
    CACHE_LOG_ALWAYS(("    Deactivate Failures         = %d\n", mDeactivateFailures));
    CACHE_LOG_ALWAYS(("    Deactivated Unbound Entries = %d\n", mDeactivatedUnboundEntries));
}
#endif
