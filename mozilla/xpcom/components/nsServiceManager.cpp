/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

#include "nsIServiceManager.h"
#include "nsVector.h"
#include "nsHashtable.h"
#include "prcmon.h"
#include "prthread.h" /* XXX: only used for the NSPR initialization hack (rick) */

static NS_DEFINE_CID(kComponentManagerCID, NS_COMPONENTMANAGER_CID);

class nsServiceEntry {
public:

    nsServiceEntry(const nsCID& cid, nsISupports* service);
    ~nsServiceEntry();

    nsresult AddListener(nsIShutdownListener* listener);
    nsresult RemoveListener(nsIShutdownListener* listener);
    nsresult NotifyListeners(void);

    const nsCID& mClassID;
    nsISupports* mService;
    nsVector* mListeners;        // nsVector<nsIShutdownListener>
    PRBool mShuttingDown;

};

nsServiceEntry::nsServiceEntry(const nsCID& cid, nsISupports* service)
    : mClassID(cid), mService(service), mListeners(NULL), mShuttingDown(PR_FALSE)
{
}

nsServiceEntry::~nsServiceEntry()
{
    if (mListeners) {
        NS_ASSERTION(mListeners->GetSize() == 0, "listeners not removed or notified");
#if 0
        PRUint32 size = mListeners->GetSize();
        for (PRUint32 i = 0; i < size; i++) {
            nsIShutdownListener* listener = (nsIShutdownListener*)(*mListeners)[i];
            NS_RELEASE(listener);
        }
#endif
        delete mListeners;
    }
}

nsresult
nsServiceEntry::AddListener(nsIShutdownListener* listener)
{
    if (listener == NULL)
        return NS_OK;
    if (mListeners == NULL) {
        mListeners = new nsVector();
        if (mListeners == NULL)
            return NS_ERROR_OUT_OF_MEMORY;
    }
    PRInt32 rv = mListeners->Add(listener);
    NS_ADDREF(listener);
    return rv == -1 ? NS_ERROR_FAILURE : NS_OK;
}

nsresult
nsServiceEntry::RemoveListener(nsIShutdownListener* listener)
{
    if (listener == NULL)
        return NS_OK;
    NS_ASSERTION(mListeners, "no listeners added yet");
    PRUint32 size = mListeners->GetSize();
    for (PRUint32 i = 0; i < size; i++) {
        if ((*mListeners)[i] == listener) {
            mListeners->Remove(i);
            NS_RELEASE(listener);
            return NS_OK;
        }
    }
    NS_ASSERTION(0, "unregistered shutdown listener");
    return NS_ERROR_FAILURE;
}

nsresult
nsServiceEntry::NotifyListeners(void)
{
    if (mListeners) {
        PRUint32 size = mListeners->GetSize();
        for (PRUint32 i = 0; i < size; i++) {
            nsIShutdownListener* listener = (nsIShutdownListener*)(*mListeners)[0];
            nsresult rv = listener->OnShutdown(mClassID, mService);
            if (NS_FAILED(rv)) return rv;
            NS_RELEASE(listener);
            mListeners->Remove(0);
        }
        NS_ASSERTION(mListeners->GetSize() == 0, "failed to notify all listeners");
        delete mListeners;
        mListeners = NULL;
    }
    return NS_OK;
}

////////////////////////////////////////////////////////////////////////////////

class nsServiceManagerImpl : public nsIServiceManager {
public:

    NS_IMETHOD
    RegisterService(const nsCID& aClass, nsISupports* aService);

    NS_IMETHOD
    UnregisterService(const nsCID& aClass);

    NS_IMETHOD
    GetService(const nsCID& aClass, const nsIID& aIID,
               nsISupports* *result,
               nsIShutdownListener* shutdownListener = NULL);

    NS_IMETHOD
    ReleaseService(const nsCID& aClass, nsISupports* service,
                   nsIShutdownListener* shutdownListener = NULL);

    NS_IMETHOD
    GetService(const char* aProgID, const nsIID& aIID,
               nsISupports* *result,
               nsIShutdownListener* shutdownListener = NULL);

    NS_IMETHOD
    ReleaseService(const char* aProgID, nsISupports* service,
                   nsIShutdownListener* shutdownListener = NULL);

    nsServiceManagerImpl(void);

    NS_DECL_ISUPPORTS

protected:

    virtual ~nsServiceManagerImpl(void);

    nsHashtable/*<nsServiceEntry>*/* mServices;
};

nsServiceManagerImpl::nsServiceManagerImpl(void)
{
    NS_INIT_REFCNT();
    mServices = new nsHashtable();
    NS_ASSERTION(mServices, "out of memory already?");
}

static PRBool
DeleteEntry(nsHashKey *aKey, void *aData, void* closure)
{
    nsServiceEntry* entry = (nsServiceEntry*)aData;
    NS_RELEASE(entry->mService);
    delete entry;
    return PR_TRUE;
}

nsServiceManagerImpl::~nsServiceManagerImpl(void)
{
    if (mServices) {
        mServices->Enumerate(DeleteEntry);
        delete mServices;
    }
}

static NS_DEFINE_IID(kIServiceManagerIID, NS_ISERVICEMANAGER_IID);
static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);

NS_IMPL_ADDREF(nsServiceManagerImpl);
NS_IMPL_RELEASE(nsServiceManagerImpl);

NS_IMETHODIMP
nsServiceManagerImpl::QueryInterface(const nsIID& aIID, void* *aInstancePtr)
{
    if (NULL == aInstancePtr) {
        return NS_ERROR_NULL_POINTER; 
    } 
    *aInstancePtr = NULL; 
    if (aIID.Equals(kIServiceManagerIID) ||
        aIID.Equals(kISupportsIID)) {
        *aInstancePtr = (void*) this; 
        AddRef(); 
        return NS_OK; 
    } 
    return NS_NOINTERFACE; 
}

NS_IMETHODIMP
nsServiceManagerImpl::GetService(const nsCID& aClass, const nsIID& aIID,
                                 nsISupports* *result,
                                 nsIShutdownListener* shutdownListener)
{
    nsresult rv = NS_OK;
    /* XXX: This is a hack to force NSPR initialization..  This should be 
     *      removed once PR_CEnterMonitor(...) initializes NSPR... (rick)
     */
    (void)PR_GetCurrentThread();
    PR_CEnterMonitor(this);

    nsIDKey key(aClass);
    nsServiceEntry* entry = (nsServiceEntry*)mServices->Get(&key);

    if (entry) {
        nsISupports* service;
        rv = entry->mService->QueryInterface(aIID, (void**)&service);
        if (NS_SUCCEEDED(rv)) {
            rv = entry->AddListener(shutdownListener);
            if (NS_SUCCEEDED(rv)) {
                *result = service;

                // If someone else requested the service to be shut down, 
                // and we just asked to get it again before it could be 
                // released, then cancel their shutdown request:
                if (entry->mShuttingDown) {
                    entry->mShuttingDown = PR_FALSE;
                    NS_ADDREF(service);      // Released in UnregisterService
                }
            }
        }
    }
    else {
        nsISupports* service;
        rv = nsComponentManager::CreateInstance(aClass, NULL, aIID, (void**)&service);
        if (NS_SUCCEEDED(rv)) {
            entry = new nsServiceEntry(aClass, service);
            if (entry == NULL) {
                NS_RELEASE(service);
                rv = NS_ERROR_OUT_OF_MEMORY;
            }
            else {
                rv = entry->AddListener(shutdownListener);
                if (NS_SUCCEEDED(rv)) {
                    mServices->Put(&key, entry);
                    *result = service;
                    NS_ADDREF(service);      // Released in UnregisterService
                }
                else {
                    NS_RELEASE(service);
                    delete entry;
                }
            }
        }
    }

    PR_CExitMonitor(this);
    return rv;
}

NS_IMETHODIMP
nsServiceManagerImpl::ReleaseService(const nsCID& aClass, nsISupports* service,
                                     nsIShutdownListener* shutdownListener)
{
    nsresult rv = NS_OK;
    PR_CEnterMonitor(this);

    nsIDKey key(aClass);
    nsServiceEntry* entry = (nsServiceEntry*)mServices->Get(&key);

    NS_ASSERTION(entry, "service not found");
    NS_ASSERTION(entry->mService == service, "service looked failed");

    if (entry) {
        rv = entry->RemoveListener(shutdownListener);
        nsrefcnt cnt;
        NS_RELEASE2(service, cnt);
        if (NS_SUCCEEDED(rv) && cnt == 0) {
            mServices->Remove(&key);
            delete entry;
            rv = nsComponentManager::FreeLibraries();
        }
    }

    PR_CExitMonitor(this);
    return rv;
}

NS_IMETHODIMP
nsServiceManagerImpl::GetService(const char* aProgID, const nsIID& aIID,
                                 nsISupports* *result,
                                 nsIShutdownListener* shutdownListener)
{
    nsCID aClass;
    nsresult rv;
    rv = nsComponentManager::ProgIDToCLSID(aProgID, &aClass);
    if (NS_FAILED(rv)) return rv;
    return GetService(aClass, aIID, result, shutdownListener);
}

NS_IMETHODIMP
nsServiceManagerImpl::ReleaseService(const char* aProgID, nsISupports* service,
                                     nsIShutdownListener* shutdownListener)
{
    nsCID aClass;
    nsresult rv;
    rv = nsComponentManager::ProgIDToCLSID(aProgID, &aClass);
    if (NS_FAILED(rv)) return rv;
    return ReleaseService(aClass, service, shutdownListener);
}

NS_IMETHODIMP
nsServiceManagerImpl::RegisterService(const nsCID& aClass, nsISupports* aService)
{
    nsresult rv = NS_OK;
    PR_CEnterMonitor(this);

    nsIDKey key(aClass);
    nsServiceEntry* entry = (nsServiceEntry*)mServices->Get(&key);
    if (entry) {
        rv = NS_ERROR_FAILURE;
    }
    else {
        nsServiceEntry* entry = new nsServiceEntry(aClass, aService);
        if (entry == NULL) 
            rv = NS_ERROR_OUT_OF_MEMORY;
        else {
            mServices->Put(&key, entry);
            NS_ADDREF(aService);      // Released in UnregisterService
        }
    }
    PR_CExitMonitor(this);
    return rv;
}

NS_IMETHODIMP
nsServiceManagerImpl::UnregisterService(const nsCID& aClass)
{
    nsresult rv = NS_OK;
    PR_CEnterMonitor(this);

    nsIDKey key(aClass);
    nsServiceEntry* entry = (nsServiceEntry*)mServices->Get(&key);

    if (entry == NULL) {
        rv = NS_ERROR_SERVICE_NOT_FOUND;
    }
    else {
        rv = entry->NotifyListeners();         // break the cycles
        entry->mShuttingDown = PR_TRUE;
        nsrefcnt cnt;
        NS_RELEASE2(entry->mService, cnt);       // AddRef in GetService
        if (NS_SUCCEEDED(rv) && cnt == 0) {
            mServices->Remove(&key);
            delete entry;
            rv = nsComponentManager::FreeLibraries();
        }
        else
            rv = NS_ERROR_SERVICE_IN_USE;
    }

    PR_CExitMonitor(this);
    return rv;
}

////////////////////////////////////////////////////////////////////////////////

nsresult
NS_NewServiceManager(nsIServiceManager* *result)
{
    nsServiceManagerImpl* servMgr = new nsServiceManagerImpl();
    if (servMgr == NULL)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(servMgr);
    *result = servMgr;
    return NS_OK;
}

////////////////////////////////////////////////////////////////////////////////
// Global service manager interface (see nsIServiceManager.h)

nsIServiceManager* nsServiceManager::mGlobalServiceManager = NULL;

nsresult
nsServiceManager::GetGlobalServiceManager(nsIServiceManager* *result)
{
    if (mGlobalServiceManager == NULL) {
        nsIServiceManager* servMgr;
        nsresult rv = NS_NewServiceManager(&servMgr);
        if (NS_FAILED(rv)) return rv;

        // The global service manager always has a global component manager service:
        nsIComponentManager* compMgr;
        rv = NS_GetGlobalComponentManager(&compMgr);
        if (NS_FAILED(rv)) {
            NS_RELEASE(servMgr);
            return rv;
        }
        rv = servMgr->RegisterService(kComponentManagerCID, compMgr);
        if (NS_FAILED(rv)) {
            NS_RELEASE(servMgr);
            return rv;
        }

        mGlobalServiceManager = servMgr;
    }

    *result = mGlobalServiceManager;
    return NS_OK;
}

nsresult
nsServiceManager::GetService(const nsCID& aClass, const nsIID& aIID,
                             nsISupports* *result,
                             nsIShutdownListener* shutdownListener)
{
    nsIServiceManager* mgr;
    nsresult rv = GetGlobalServiceManager(&mgr);
    if (NS_FAILED(rv)) return rv;
    return mgr->GetService(aClass, aIID, result, shutdownListener);
}

nsresult
nsServiceManager::ReleaseService(const nsCID& aClass, nsISupports* service,
                                 nsIShutdownListener* shutdownListener)
{
    nsIServiceManager* mgr;
    nsresult rv = GetGlobalServiceManager(&mgr);
    if (NS_FAILED(rv)) return rv;
    return mgr->ReleaseService(aClass, service, shutdownListener);
}

nsresult
nsServiceManager::RegisterService(const nsCID& aClass, nsISupports* aService)
{
    nsIServiceManager* mgr;
    nsresult rv = GetGlobalServiceManager(&mgr);
    if (NS_FAILED(rv)) return rv;
    return mgr->RegisterService(aClass, aService);
}

nsresult
nsServiceManager::UnregisterService(const nsCID& aClass)
{
    nsIServiceManager* mgr;
    nsresult rv = GetGlobalServiceManager(&mgr);
    if (NS_FAILED(rv)) return rv;
    return mgr->UnregisterService(aClass);
}

////////////////////////////////////////////////////////////////////////////////
