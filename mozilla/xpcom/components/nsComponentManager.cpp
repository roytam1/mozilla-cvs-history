/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
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
 * This Original Code has been modified by IBM Corporation.
 * Modifications made by IBM described herein are
 * Copyright (c) International Business Machines
 * Corporation, 2000
 *
 * Modifications to Mozilla code or documentation
 * identified per MPL Section 3.3
 *
 * Date             Modified by     Description of modification
 * 04/20/2000       IBM Corp.      Added PR_CALLBACK for Optlink use in OS2
 */
#include <stdlib.h>
#include "nscore.h"
#include "nsISupports.h"
#include "nsCRT.h"
#include "nspr.h"

// this after nsISupports, to pick up IID
// so that xpt stuff doesn't try to define it itself...
#include "xptinfo.h"
#include "nsIInterfaceInfoManager.h"

#include "nsCOMPtr.h"
#include "nsComponentManager.h"
#include "nsComponentManagerObsolete.h"
#include "nsICategoryManager.h"

#include "nsIEnumerator.h"
#include "nsIModule.h"
#include "nsISupportsPrimitives.h"
#include "nsIComponentLoader.h"
#include "nsNativeComponentLoader.h"
#include "nsXPIDLString.h"

#include "nsIObserverService.h"

#include "nsLocalFile.h"
#include "nsDirectoryService.h"
#include "nsDirectoryServiceDefs.h"

#include "NSReg.h"

#include "prcmon.h"
#include "prthread.h" /* XXX: only used for the NSPR initialization hack (rick) */

#ifdef XP_BEOS
#include <FindDirectory.h>
#include <Path.h>
#endif

#include "nsRegistry.h"

// Logging of debug output
#ifdef MOZ_LOGGING
#define FORCE_PR_LOG /* Allow logging in the release build */
#endif
#include "prlog.h"
PRLogModuleInfo* nsComponentManagerLog = NULL;

#if defined(DEBUG)
#define SHOW_DENIED_ON_SHUTDOWN
#endif

#include "nsAutoLock.h"

// Enable printing of critical errors on screen even for release builds
#define PRINT_CRITICAL_ERROR_TO_SCREEN

// Loader Types
#define NS_LOADER_DATA_ALLOC_STEP 6

// Bloated registry buffer size to improve startup performance -- needs to
// be big enough to fit the entire file into memory or it'll thrash.
// 512K is big enough to allow for some future growth in the registry.
#define BIG_REGISTRY_BUFLEN   (512*1024)

// Common Key Names 
const char xpcomKeyName[]="software/mozilla/XPCOM";
const char classesKeyName[]="contractID";
const char classIDKeyName[]="classID";
const char componentsKeyName[]="components";
const char componentLoadersKeyName[]="componentLoaders";
const char xpcomComponentsKeyName[]="software/mozilla/XPCOM/components";

// Common Value Names
const char classIDValueName[]="ClassID";
const char versionValueName[]="VersionString";
const char lastModValueName[]="LastModTimeStamp";
const char fileSizeValueName[]="FileSize";
const char componentCountValueName[]="ComponentsCount";
const char contractIDValueName[]="ContractID";
const char classNameValueName[]="ClassName";
const char inprocServerValueName[]="InprocServer";
const char componentTypeValueName[]="ComponentType";
const char nativeComponentType[]="application/x-mozilla-native";
const char staticComponentType[]="application/x-mozilla-static";

const static char XPCOM_ABSCOMPONENT_PREFIX[] = "abs:";
const static char XPCOM_RELCOMPONENT_PREFIX[] = "rel:";
const char XPCOM_LIB_PREFIX[]          = "lib:";

// Nonexistent factory entry
// This is used to mark non-existent contractid mappings
static nsFactoryEntry * kNonExistentContractID = (nsFactoryEntry*) 1;



#define NS_EMPTY_IID                                 \
{                                                    \
    0x00000000,                                      \
    0x0000,                                          \
    0x0000,                                          \
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00} \
}

NS_DEFINE_CID(kEmptyCID, NS_EMPTY_IID);

extern PRBool gXPCOMShuttingDown;

// Build is using USE_NSREG to turn off xpcom using registry
// but internally we use USE_REGISTRY. Map them propertly.
#ifdef USE_NSREG
#define USE_REGISTRY
#endif /* USE_NSREG */

nsresult
nsCreateInstanceByCID::operator()( const nsIID& aIID, void** aInstancePtr ) const
    {
        nsresult status = nsComponentManager::CreateInstance(mCID, mOuter, aIID, aInstancePtr);
        if ( NS_FAILED(status) )
            *aInstancePtr = 0;

        if ( mErrorPtr )
            *mErrorPtr = status;
        return status;
    }

nsresult
nsCreateInstanceByContractID::operator()( const nsIID& aIID, void** aInstancePtr ) const
    {
        nsresult status;
        if ( mContractID )
            {
              if ( NS_FAILED(status = nsComponentManager::CreateInstance(mContractID, mOuter, aIID, aInstancePtr)) )
                  *aInstancePtr = 0;
          }
        else
          status = NS_ERROR_NULL_POINTER;

        if ( mErrorPtr )
            *mErrorPtr = status;
        return status;
    }

nsresult
nsCreateInstanceFromCategory::operator()( const nsIID& aIID,
                                          void** aInstancePtr ) const
{
    /*
     * If I were a real man, I would consolidate this with
     * nsGetServiceFromContractID::operator().
     */
    nsresult status;
    nsXPIDLCString value;
    nsCOMPtr<nsICategoryManager> catman =
        do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &status);

    if (NS_FAILED(status)) goto error;

    if (!mCategory || !mEntry) {
        // when categories have defaults, use that for null mEntry
        status = NS_ERROR_NULL_POINTER;
        goto error;
    }
    
    /* find the contractID for category.entry */
    status = catman->GetCategoryEntry(mCategory, mEntry,
                                      getter_Copies(value));
    if (NS_FAILED(status)) goto error;
    if (!value) {
        status = NS_ERROR_SERVICE_NOT_FOUND;
        goto error;
    }

    status = nsComponentManager::CreateInstance(value, mOuter, aIID,
                                                aInstancePtr);
    error:
    if (NS_FAILED(status)) {
        *aInstancePtr = 0;
    }

    *mErrorPtr = status;
    return status;
}


nsresult
nsGetServiceByCID::operator()( const nsIID& aIID, void** aInstancePtr ) const
  {
    nsresult status = NS_ERROR_FAILURE;
    if ( mServiceManager ) {
        status = mServiceManager->GetService(mCID, aIID, (void**)aInstancePtr);
    } else {
        nsCOMPtr<nsIServiceManager> mgr;
        NS_GetServiceManager(getter_AddRefs(mgr));
        if (mgr)
            status = mgr->GetService(mCID, aIID, (void**)aInstancePtr);
    }
    if ( NS_FAILED(status) )
        *aInstancePtr = 0;

    if ( mErrorPtr )
      *mErrorPtr = status;
    return status;
  }

nsresult
nsGetServiceByContractID::operator()( const nsIID& aIID, void** aInstancePtr ) const
  {
    nsresult status = NS_ERROR_FAILURE;
    if ( mServiceManager ) {
        status = mServiceManager->GetServiceByContractID(mContractID, aIID, (void**)aInstancePtr);
    } else {
        nsCOMPtr<nsIServiceManager> mgr;
        NS_GetServiceManager(getter_AddRefs(mgr));
        if (mgr)
            status = mgr->GetServiceByContractID(mContractID, aIID, (void**)aInstancePtr);
    }
 
    if ( NS_FAILED(status) )
        *aInstancePtr = 0;

    if ( mErrorPtr )
      *mErrorPtr = status;
    return status;
}

nsresult
nsGetServiceFromCategory::operator()( const nsIID& aIID, void** aInstancePtr)
  const
{
  nsresult status;
  nsXPIDLCString value;
  nsCOMPtr<nsICategoryManager> catman = 
    do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &status);
  
  if (NS_FAILED(status)) goto error;
  
  if (!mCategory || !mEntry) {
    // when categories have defaults, use that for null mEntry
    status = NS_ERROR_NULL_POINTER;
    goto error;
  }
  
  /* find the contractID for category.entry */
  status = catman->GetCategoryEntry(mCategory, mEntry,
                                    getter_Copies(value));
  if (NS_FAILED(status)) goto error;
  if (!value) {
    status = NS_ERROR_SERVICE_NOT_FOUND;
    goto error;
  }
  
  if ( mServiceManager ) {
    status = mServiceManager->GetServiceByContractID(value, aIID, (void**)aInstancePtr);
  } else {
    nsCOMPtr<nsIServiceManager> mgr;
    NS_GetServiceManager(getter_AddRefs(mgr));
    if (mgr)
        status = mgr->GetServiceByContractID(value, aIID, (void**)aInstancePtr);
    }

  if (NS_FAILED(status)) {
  error:
    *aInstancePtr = 0;
  }

  *mErrorPtr = status;
  return status;
}

/* prototypes for the Mac */
PRBool PR_CALLBACK
nsFactoryEntry_Destroy(nsHashKey *aKey, void *aData, void* closure);

PR_STATIC_CALLBACK(const void *)
factory_GetKey(PLDHashTable *aTable, PLDHashEntryHdr *aHdr)
{
    nsFactoryTableEntry* entry = NS_STATIC_CAST(nsFactoryTableEntry*, aHdr);

    return &entry->mFactoryEntry->cid;
}

PR_STATIC_CALLBACK(PLDHashNumber)
factory_HashKey(PLDHashTable *aTable, const void *aKey)
{
    const nsCID *cidp = NS_REINTERPRET_CAST(const nsCID*, aKey);

    return cidp->m0;
}

PR_STATIC_CALLBACK(PRBool)
factory_MatchEntry(PLDHashTable *aTable, const PLDHashEntryHdr *aHdr,
                   const void *aKey)
{
    const nsFactoryTableEntry* entry =
        NS_STATIC_CAST(const nsFactoryTableEntry*, aHdr);
    const nsCID *cidp = NS_REINTERPRET_CAST(const nsCID*, aKey);

    return (entry->mFactoryEntry->cid).Equals(*cidp);
}

PR_STATIC_CALLBACK(void)
factory_ClearEntry(PLDHashTable *aTable, PLDHashEntryHdr *aHdr)
{
    nsFactoryTableEntry* entry = NS_STATIC_CAST(nsFactoryTableEntry*, aHdr);

    delete entry->mFactoryEntry;
    PL_DHashClearEntryStub(aTable, aHdr);
}
  
static PLDHashTableOps factory_DHashTableOps = {
    PL_DHashAllocTable,
    PL_DHashFreeTable,
    factory_GetKey,
    factory_HashKey,
    factory_MatchEntry,
    PL_DHashMoveEntryStub,
    factory_ClearEntry,
    PL_DHashFinalizeStub,
};
 
PR_STATIC_CALLBACK(const void *)
contractID_GetKey(PLDHashTable *aTable, PLDHashEntryHdr *aHdr)
{
    nsContractIDTableEntry* entry = NS_STATIC_CAST(nsContractIDTableEntry*, aHdr);

    return entry->mContractID;
}
 
PR_STATIC_CALLBACK(PRBool)
contractID_MatchEntry(PLDHashTable *aTable, const PLDHashEntryHdr *aHdr,
                      const void *aKey)
{
    const nsContractIDTableEntry* entry =
        NS_STATIC_CAST(const nsContractIDTableEntry*, aHdr);

    const char* contractID = NS_REINTERPRET_CAST(const char*, aKey);

    return strcmp(entry->mContractID, contractID) == 0;
}

PR_STATIC_CALLBACK(void)
contractID_ClearEntry(PLDHashTable *aTable, PLDHashEntryHdr *aHdr)
{
    nsContractIDTableEntry* entry = NS_STATIC_CAST(nsContractIDTableEntry*, aHdr);
    
    if (entry->mFactoryEntry != kNonExistentContractID && 
        entry->mFactoryEntry->typeIndex == NS_COMPONENT_TYPE_SERVICE_ONLY && 
        entry->mFactoryEntry->cid.Equals(kEmptyCID)) {
        // this object is owned by the hash.  Time to delete it.
        delete entry->mFactoryEntry;
    }

    nsCRT::free(entry->mContractID);
   
    PL_DHashClearEntryStub(aTable, aHdr);
}

static PLDHashTableOps contractID_DHashTableOps = {
    PL_DHashAllocTable,
    PL_DHashFreeTable,
    contractID_GetKey,
    PL_DHashStringKey,
    contractID_MatchEntry,
    PL_DHashMoveEntryStub,
    contractID_ClearEntry,
    PL_DHashFinalizeStub,
};

////////////////////////////////////////////////////////////////////////////////
// nsFactoryEntry
////////////////////////////////////////////////////////////////////////////////

MOZ_DECL_CTOR_COUNTER(nsFactoryEntry)

nsFactoryEntry::nsFactoryEntry(const nsCID &aClass, 
                               const char *aLocation,
                               int aType)
    : cid(aClass), typeIndex(aType)
{
    MOZ_COUNT_CTOR(nsFactoryEntry);
    location = nsCRT::strdup(aLocation);
}

nsFactoryEntry::nsFactoryEntry(const nsCID &aClass, nsIFactory *aFactory)
    : cid(aClass), typeIndex(NS_COMPONENT_TYPE_FACTORY_ONLY)
{
    MOZ_COUNT_CTOR(nsFactoryEntry);
    factory = aFactory;
    location = nsnull;

}

nsFactoryEntry::~nsFactoryEntry(void)
{
    MOZ_COUNT_DTOR(nsFactoryEntry);
    if (location)
        nsCRT::free(location);
    factory = 0;
}

nsresult
nsFactoryEntry::ReInit(const nsCID &aClass, const char *aLocation, int aType)
{
    NS_ENSURE_TRUE(typeIndex != NS_COMPONENT_TYPE_FACTORY_ONLY, NS_ERROR_INVALID_ARG);
    // cid has to match
    // SERVICE_ONLY entries can be promoted to an entry of another type
    NS_ENSURE_TRUE((typeIndex == NS_COMPONENT_TYPE_SERVICE_ONLY || cid.Equals(aClass)),
                   NS_ERROR_INVALID_ARG);
    location = nsCRT::strdup(aLocation);
    typeIndex = aType;
    return NS_OK;
}

nsresult
nsFactoryEntry::ReInit(const nsCID &aClass, nsIFactory *aFactory)
{
    // cids has to match
    // SERVICE_ONLY entries can be promoted to an entry of another type
    NS_ENSURE_TRUE((typeIndex == NS_COMPONENT_TYPE_SERVICE_ONLY || cid.Equals(aClass)),
                   NS_ERROR_INVALID_ARG);
    // We are going to let native component be overridden by a factory.
    factory = aFactory;
    typeIndex = NS_COMPONENT_TYPE_FACTORY_ONLY;
    return NS_OK;
}
////////////////////////////////////////////////////////////////////////////////
// nsComponentManagerImpl
////////////////////////////////////////////////////////////////////////////////


nsComponentManagerImpl::nsComponentManagerImpl()
    : 
    mMon(NULL), 
    mRegistry(NULL), 
    mPrePopulationDone(PR_FALSE),
    mNativeComponentLoader(0),
    mStaticComponentLoader(0),
    mShuttingDown(NS_SHUTDOWN_NEVERHAPPENED), 
    mLoaderData(nsnull)
{
    NS_INIT_REFCNT();
    mFactories.ops = nsnull;
    mContractIDs.ops = nsnull;
}

nsresult nsComponentManagerImpl::Init(void) 
{
    PR_ASSERT(mShuttingDown != NS_SHUTDOWN_INPROGRESS);
    if (mShuttingDown == NS_SHUTDOWN_INPROGRESS)
        return NS_ERROR_FAILURE;

    mShuttingDown = NS_SHUTDOWN_NEVERHAPPENED;

    if (nsComponentManagerLog == NULL)
    {
        nsComponentManagerLog = PR_NewLogModule("nsComponentManager");
        PR_LOG(nsComponentManagerLog, PR_LOG_ALWAYS,
               ("xpcom-log-version : " NS_XPCOM_COMPONENT_MANAGER_VERSION_STRING));
    }

    if (!mFactories.ops) {
        if (!PL_DHashTableInit(&mFactories, &factory_DHashTableOps,
                               0, sizeof(nsFactoryTableEntry),
                               1024)) {
            mFactories.ops = nsnull;
            return NS_ERROR_OUT_OF_MEMORY;
        }

        // Minimum alpha uses k=3 because nsFactoryTableEntry saves three
        // words compared to what a chained hash table requires.
        PL_DHashTableSetAlphaBounds(&mFactories,
                                    0.875,
                                    PL_DHASH_MIN_ALPHA(&mFactories, 3));
    }

    if (!mContractIDs.ops) {
        if (!PL_DHashTableInit(&mContractIDs, &contractID_DHashTableOps,
                               0, sizeof(nsContractIDTableEntry),
                               1024)) {
            mContractIDs.ops = nsnull;
            return NS_ERROR_OUT_OF_MEMORY;
        }

         // Minimum alpha uses k=2 because nsContractIDTableEntry saves two
         // words compared to what a chained hash table requires.
         PL_DHashTableSetAlphaBounds(&mContractIDs,
                                     0.875,
                                     PL_DHASH_MIN_ALPHA(&mContractIDs, 2));
    }
    if (mMon == NULL) {
        mMon = PR_NewMonitor();
        if (mMon == NULL)
            return NS_ERROR_OUT_OF_MEMORY;
    }

    if (mNativeComponentLoader == nsnull) {
        /* Create the NativeComponentLoader */
        mNativeComponentLoader = new nsNativeComponentLoader();
        if (!mNativeComponentLoader)
            return NS_ERROR_OUT_OF_MEMORY;
        NS_ADDREF(mNativeComponentLoader);
    }

    // Add predefined loaders
    mLoaderData = (nsLoaderdata *) PR_Malloc(sizeof(nsLoaderdata) * NS_LOADER_DATA_ALLOC_STEP);
    if (!mLoaderData)
        return NS_ERROR_OUT_OF_MEMORY;
    mMaxNLoaderData = NS_LOADER_DATA_ALLOC_STEP;

    mNLoaderData = NS_COMPONENT_TYPE_NATIVE;
    mLoaderData[mNLoaderData].type = PL_strdup(nativeComponentType);
    mLoaderData[mNLoaderData].loader = mNativeComponentLoader;
    NS_ADDREF(mLoaderData[mNLoaderData].loader);
    mNLoaderData++;

#ifdef ENABLE_STATIC_COMPONENT_LOADER
    if (mStaticComponentLoader == nsnull) {
        extern nsresult NS_NewStaticComponentLoader(nsIComponentLoader **);
        NS_NewStaticComponentLoader(&mStaticComponentLoader);
        if (!mStaticComponentLoader)
            return NS_ERROR_OUT_OF_MEMORY;
    }

    mLoaderData[mNLoaderData].type = PL_strdup(staticComponentType);
    mLoaderData[mNLoaderData].loader = mStaticComponentLoader;
    NS_ADDREF(mLoaderData[mNLoaderData].loader);
    mNLoaderData++;
#endif

#ifdef USE_REGISTRY
        NR_StartupRegistry();
        {
            nsresult ret;
            ret = PlatformInit();
            if( NS_FAILED( ret ) ) {
                return ret;
            }
        }
#endif

    PR_LOG(nsComponentManagerLog, PR_LOG_ALWAYS,
           ("nsComponentManager: Initialized."));

    return NS_OK;
}

nsresult nsComponentManagerImpl::Shutdown(void) 
{
    PR_ASSERT(mShuttingDown == NS_SHUTDOWN_NEVERHAPPENED);
    if (mShuttingDown != NS_SHUTDOWN_NEVERHAPPENED)
        return NS_ERROR_FAILURE;

    mShuttingDown = NS_SHUTDOWN_INPROGRESS;

    // Shutdown the component manager
    PR_LOG(nsComponentManagerLog, PR_LOG_ALWAYS, ("nsComponentManager: Beginning Shutdown."));

    // Release all cached factories
    if (mContractIDs.ops) {
        PL_DHashTableFinish(&mContractIDs);
        mContractIDs.ops = nsnull;
    }
    if (mFactories.ops) {
        PL_DHashTableFinish(&mFactories);
        mFactories.ops = nsnull;
    }
    // Unload libraries
    UnloadLibraries(NULL, NS_Shutdown);

#ifdef USE_REGISTRY
    // Release registry
    NS_IF_RELEASE(mRegistry);
#endif /* USE_REGISTRY */

    // This is were the nsFileSpec was deleted, so I am 
    // going to assign zero to 
    mComponentsDir = 0;

    // Release all the component data - loaders and type strings
    for(int i=0; i < mNLoaderData; i++) {
        NS_IF_RELEASE(mLoaderData[i].loader);
        PL_strfree((char *)mLoaderData[i].type);
    }
    PR_Free(mLoaderData);
    mLoaderData = nsnull;

    // we have an extra reference on this one, which is probably a good thing
    NS_IF_RELEASE(mNativeComponentLoader);
#ifdef ENABLE_STATIC_COMPONENT_LOADER
    NS_IF_RELEASE(mStaticComponentLoader);
#endif
    
    if (mMon)
        PR_DestroyMonitor(mMon);

#ifdef USE_REGISTRY
    NR_ShutdownRegistry();
#endif /* USE_REGISTRY */

    mShuttingDown = NS_SHUTDOWN_COMPLETE;

    PR_LOG(nsComponentManagerLog, PR_LOG_ALWAYS, ("nsComponentManager: Shutdown complete."));

    return NS_OK;
}

nsComponentManagerImpl::~nsComponentManagerImpl()
{
    PR_LOG(nsComponentManagerLog, PR_LOG_ALWAYS, ("nsComponentManager: Beginning destruction."));

    if (mShuttingDown != NS_SHUTDOWN_COMPLETE)
        Shutdown();

    PR_LOG(nsComponentManagerLog, PR_LOG_ALWAYS, ("nsComponentManager: Destroyed."));
}

NS_IMPL_THREADSAFE_ISUPPORTS6(nsComponentManagerImpl, 
                              nsIComponentManager, 
                              nsIServiceManager,
                              nsISupportsWeakReference, 
                              nsIInterfaceRequestor,
                              nsIServiceManagerObsolete,
                              nsIComponentManagerObsolete)

////////////////////////////////////////////////////////////////////////////////
// nsComponentManagerImpl: Platform methods
////////////////////////////////////////////////////////////////////////////////

#ifdef USE_REGISTRY

nsresult
nsComponentManagerImpl::PlatformInit(void)
{
    nsresult rv = NS_ERROR_FAILURE;

    // We need to create our registry. Since we are in the constructor
    // we haven't gone as far as registering the registry factory.
    // Hence, we hand create a registry.
    if (mRegistry == NULL) {        
        nsIFactory *registryFactory = NULL;
        rv = NS_RegistryGetFactory(&registryFactory);
        if (NS_SUCCEEDED(rv))
        {
            rv = registryFactory->CreateInstance(NULL, NS_GET_IID(nsIRegistry),(void **)&mRegistry);
            if (NS_FAILED(rv)) return rv;
            NS_RELEASE(registryFactory);
        }
    }

    // Open the App Components registry. We will keep it open forever!
    rv = mRegistry->OpenWellKnownRegistry(nsIRegistry::ApplicationComponentRegistry);
    if (NS_FAILED(rv)) return rv;

    // Set larger-than-standard buffer size to speed startup.
    // This will be re-set at the end of PrePopulateRegistry()
    ((nsRegistry *)mRegistry)->SetBufferSize( BIG_REGISTRY_BUFLEN );

    // Check the version of registry. Nuke old versions.
    nsRegistryKey xpcomRoot;
    rv = PlatformVersionCheck(&xpcomRoot);
    if (NS_FAILED(rv)) return rv;

    // Open common registry keys here to speed access
    // Do this after PlatformVersionCheck as it may re-create our keys
    rv = mRegistry->AddSubtree(xpcomRoot, componentsKeyName, &mXPCOMKey);
    if (NS_FAILED(rv)) return rv;

    rv = mRegistry->AddSubtree(xpcomRoot, classesKeyName, &mClassesKey);
    if (NS_FAILED(rv)) return rv;

    rv = mRegistry->AddSubtree(xpcomRoot, classIDKeyName, &mCLSIDKey);
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIProperties> directoryService;
    rv = nsDirectoryService::Create(nsnull, 
                                    NS_GET_IID(nsIProperties), 
                                    getter_AddRefs(directoryService));  

    directoryService->Get(NS_XPCOM_COMPONENT_DIR, NS_GET_IID(nsIFile), getter_AddRefs(mComponentsDir));

    if (!mComponentsDir)
        return NS_ERROR_OUT_OF_MEMORY;

    char* componentDescriptor;
    mComponentsDir->GetPath(&componentDescriptor);
    if (!componentDescriptor)
        return NS_ERROR_NULL_POINTER;

    mComponentsOffset = strlen(componentDescriptor);

    if (componentDescriptor)
        nsMemory::Free(componentDescriptor);



    if (mNativeComponentLoader) {
        /* now that we have the registry, Init the native loader */
        rv = mNativeComponentLoader->Init(this, mRegistry);
    } else {
        PR_LOG(nsComponentManagerLog, PR_LOG_ERROR,
               ("no native component loader available for init"));
    }

#ifdef ENABLE_STATIC_COMPONENT_LOADER
    if (mStaticComponentLoader) {
        /* now that we have the registry, Init the static loader */
        rv = mStaticComponentLoader->Init(this, mRegistry);
    }
#endif

    return rv;
}

/**
 * PlatformVersionCheck()
 *
 * Checks to see if the XPCOM hierarchy in the registry is the same as that of
 * the software as defined by NS_XPCOM_COMPONENT_MANAGER_VERSION_STRING
 */
nsresult
nsComponentManagerImpl::PlatformVersionCheck(nsRegistryKey *aXPCOMRootKey)
{
    nsRegistryKey xpcomKey;
    nsresult rv;
    rv = mRegistry->AddSubtree(nsIRegistry::Common, xpcomKeyName, &xpcomKey);
    if (NS_FAILED(rv)) return rv;

    nsXPIDLCString buf;
    nsresult err = mRegistry->GetStringUTF8(xpcomKey, versionValueName, 
                                        getter_Copies(buf));

    // If there is a version mismatch or no version string, we got an old registry.
    // Delete the old repository hierarchies and recreate version string
    if (NS_FAILED(err) || PL_strcmp(buf, NS_XPCOM_COMPONENT_MANAGER_VERSION_STRING))
    {
        PR_LOG(nsComponentManagerLog, PR_LOG_ALWAYS,
               ("nsComponentManager: Registry version mismatch (old:%s vs new:%s)."
                "Nuking xpcom registry hierarchy.", (const char *)buf,
                NS_XPCOM_COMPONENT_MANAGER_VERSION_STRING));

        // Delete the XPCOM hierarchy
        rv = mRegistry->RemoveSubtree(nsIRegistry::Common, xpcomKeyName);
        if(NS_FAILED(rv))
        {
            PR_LOG(nsComponentManagerLog, PR_LOG_ALWAYS,
                   ("nsComponentManager: Failed To Nuke Subtree (%s)",xpcomKeyName));
            return rv;
        }

        // The top-level Classes and CLSID trees are from an early alpha version,
        // we can probably remove these two deletions after the second beta or so.
        (void) mRegistry->RemoveSubtree(nsIRegistry::Common, classIDKeyName);
        (void) mRegistry->RemoveSubtree(nsIRegistry::Common, classesKeyName);

        // Recreate XPCOM key and version
        rv = mRegistry->AddSubtree(nsIRegistry::Common,xpcomKeyName, &xpcomKey);
        if(NS_FAILED(rv))
        {
            PR_LOG(nsComponentManagerLog, PR_LOG_ALWAYS,
                   ("nsComponentManager: Failed To Add Subtree (%s)",xpcomKeyName));
            return rv;
        }

        rv = mRegistry->SetStringUTF8(xpcomKey,versionValueName, NS_XPCOM_COMPONENT_MANAGER_VERSION_STRING);
        if(NS_FAILED(rv))
        {
            PR_LOG(nsComponentManagerLog, PR_LOG_ALWAYS,
                   ("nsComponentManager: Failed To Set String (Version) Under (%s)",xpcomKeyName));
            return rv;
        }
    }
    else
    {
        PR_LOG(nsComponentManagerLog, PR_LOG_ALWAYS,
               ("nsComponentManager: platformVersionCheck() passed."));
    }


    // return the XPCOM key (null check deferred so cleanup always happens)
    if (!aXPCOMRootKey)
        return NS_ERROR_NULL_POINTER;
    else
        *aXPCOMRootKey = xpcomKey;

    return NS_OK;
}

#if 0
// If ever revived, this code is not fully updated to escape the dll location
void
nsComponentManagerImpl::PlatformSetFileInfo(nsRegistryKey key, PRUint32 lastModifiedTime, PRUint32 fileSize)
{
    mRegistry->SetInt(key, lastModValueName, lastModifiedTime);
    mRegistry->SetInt(key, fileSizeValueName, fileSize);
}

/**
 * PlatformMarkNoComponents(nsDll *dll)
 *
 * Stores the dll name, last modified time, size and 0 for number of
 * components in dll in the registry at location
 *        ROOTKEY_COMMON/Software/Mozilla/XPCOM/Components/dllname
 */
nsresult
nsComponentManagerImpl::PlatformMarkNoComponents(nsDll *dll)
{
    PR_ASSERT(mRegistry!=NULL);
    
    nsresult rv;

    nsRegistryKey dllPathKey;
    rv = mRegistry->AddSubtreeRaw(mXPCOMKey, dll->GetPersistentDescriptorString(), &dllPathKey);    
    if(NS_FAILED(rv))
    {
        return rv;
    }
        
    PlatformSetFileInfo(dllPathKey, dll->GetLastModifiedTime(), dll->GetSize());
    rv = mRegistry->SetInt(dllPathKey, componentCountValueName, 0);
      
    return rv;
}

nsresult
nsComponentManagerImpl::PlatformRegister(const char *cidString,
                                         const char *className,
                                         const char * contractID, nsDll *dll)
{
    // Preconditions
    PR_ASSERT(cidString != NULL);
    PR_ASSERT(dll != NULL);
    PR_ASSERT(mRegistry !=NULL);

    nsresult rv;
    
    nsRegistryKey IDkey;
    rv = mRegistry->AddSubtreeRaw(mCLSIDKey, cidString, &IDkey);
    if (NS_FAILED(rv)) return (rv);


    rv = mRegistry->SetStringUTF8(IDkey,classNameValueName, className);
    if (contractID)
    {
        rv = mRegistry->SetStringUTF8(IDkey,contractIDValueName, contractID);        
    }
    rv = mRegistry->SetBytesUTF8(IDkey, inprocServerValueName, 
            strlen(dll->GetPersistentDescriptorString()) + 1, 
            NS_REINTERPRET_CAST(char*, dll->GetPersistentDescriptorString()));
    
    if (contractID)
    {
        nsRegistryKey contractIDKey;
        rv = mRegistry->AddSubtreeRaw(mClassesKey, contractID, &contractIDKey);
        rv = mRegistry->SetStringUTF8(contractIDKey, classIDValueName, cidString);
    }

    nsRegistryKey dllPathKey;
    rv = mRegistry->AddSubtreeRaw(mXPCOMKey,dll->GetPersistentDescriptorString(), &dllPathKey);

    PlatformSetFileInfo(dllPathKey, dll->GetLastModifiedTime(), dll->GetSize());

    PRInt32 nComponents = 0;
    rv = mRegistry->GetInt(dllPathKey, componentCountValueName, &nComponents);
    nComponents++;
    rv = mRegistry->SetInt(dllPathKey,componentCountValueName, nComponents);

    return rv;
}
#endif

nsresult
nsComponentManagerImpl::PlatformUnregister(const char *cidString,
                                           const char *aLibrary)
{
    nsresult rv;
    PRUint32 length = strlen(aLibrary);
    char* eLibrary;
    rv = mRegistry->EscapeKey((PRUint8*)aLibrary, 1, &length, (PRUint8**)&eLibrary);
    if (rv != NS_OK)
    {
    return rv;
    }
    if (eLibrary == nsnull)    //  No escaping required
    eLibrary = (char*)aLibrary;


    PR_ASSERT(mRegistry!=NULL);


    nsRegistryKey cidKey;
    rv = mRegistry->AddSubtreeRaw(mCLSIDKey, cidString, &cidKey);

    char *contractID = NULL;
    rv = mRegistry->GetStringUTF8(cidKey, contractIDValueName, &contractID);
    if(NS_SUCCEEDED(rv))
    {
        mRegistry->RemoveSubtreeRaw(mClassesKey, contractID);
        PR_FREEIF(contractID);
    }

    mRegistry->RemoveSubtree(mCLSIDKey, cidString);
        
    nsRegistryKey libKey;
    rv = mRegistry->GetSubtreeRaw(mXPCOMKey, eLibrary, &libKey);
    if(NS_FAILED(rv)) return rv;

    // We need to reduce the ComponentCount by 1.
    // If the ComponentCount hits 0, delete the entire key.
    PRInt32 nComponents = 0;
    rv = mRegistry->GetInt(libKey, componentCountValueName, &nComponents);
    if(NS_FAILED(rv)) return rv;
    nComponents--;
    
    if (nComponents <= 0)
    {
        rv = mRegistry->RemoveSubtreeRaw(mXPCOMKey, eLibrary);
    }
    else
    {
        rv = mRegistry->SetInt(libKey, componentCountValueName, nComponents);
    }

    if (eLibrary != aLibrary)
    nsMemory::Free(eLibrary);

    return rv;
}

nsresult
nsComponentManagerImpl::PlatformFind(const nsCID &aCID, nsFactoryEntry* *result)
{
    PR_ASSERT(mRegistry!=NULL);

    nsresult rv;

    char *cidString = aCID.ToString();

    nsRegistryKey cidKey;
    rv = mRegistry->GetSubtreeRaw(mCLSIDKey, cidString, &cidKey);
    delete [] cidString;

    if (NS_FAILED(rv)) return rv;

    nsXPIDLCString library;
    PRUint32 tmp;
    rv = mRegistry->GetBytesUTF8(cidKey, inprocServerValueName,
                              &tmp, (PRUint8**)getter_Copies(library).operator char**());
    if (NS_FAILED(rv))
    {
        // Registry inconsistent. No File name for CLSID.
        return rv;
    }

    nsXPIDLCString componentTypeStr;
    rv = mRegistry->GetStringUTF8(cidKey, componentTypeValueName, 
                              getter_Copies(componentTypeStr));
    const char* componentType = componentTypeStr.get();
    int type = NS_COMPONENT_TYPE_FACTORY_ONLY;

    if (NS_FAILED(rv)) {
        if (rv == NS_ERROR_REG_NOT_FOUND) {
            /* missing componentType, we assume application/x-moz-native */
            type = NS_COMPONENT_TYPE_NATIVE;
        }
        else 
            return rv;              // XXX translate error code?
    }
    if (type < 0) {
        // Find the right loader type index
        type = GetLoaderType(componentType);
    }

    nsFactoryEntry *res = new nsFactoryEntry(aCID, library, type);
    if (res == NULL)
      return NS_ERROR_OUT_OF_MEMORY;

    *result = res;
    return NS_OK;
}

nsresult
nsComponentManagerImpl::PlatformContractIDToCLSID(const char *aContractID, nsCID *aClass) 
{
    PR_ASSERT(aClass != NULL);
    PR_ASSERT(mRegistry);

    nsresult rv;
        
    nsRegistryKey contractIDKey;
    rv = mRegistry->GetSubtreeRaw(mClassesKey, aContractID, &contractIDKey);
    if (NS_FAILED(rv)) return NS_ERROR_FACTORY_NOT_REGISTERED;

    char *cidString;
    rv = mRegistry->GetStringUTF8(contractIDKey, classIDValueName, &cidString);
    if(NS_FAILED(rv)) return rv;
    if (!(aClass->Parse(cidString)))
    {
        rv = NS_ERROR_FAILURE;
    }

    PR_FREEIF(cidString);
    return rv;
}

nsresult
nsComponentManagerImpl::PlatformCLSIDToContractID(const nsCID *aClass,
                                              char* *aClassName, char* *aContractID)
{
        
    PR_ASSERT(aClass);
    PR_ASSERT(mRegistry);

    nsresult rv;

    char* cidStr = aClass->ToString();
    nsRegistryKey cidKey;
    rv = mRegistry->GetSubtreeRaw(mCLSIDKey, cidStr, &cidKey);
    if(NS_FAILED(rv)) return rv;
    PR_FREEIF(cidStr);

    char* classnameString;
    rv = mRegistry->GetStringUTF8(cidKey, classNameValueName, &classnameString);
    if(NS_FAILED(rv)) return rv;
    *aClassName = classnameString;

    char* contractidString;
    rv = mRegistry->GetStringUTF8(cidKey,contractIDValueName,&contractidString);
    if (NS_FAILED(rv)) return rv;
    *aContractID = contractidString;

    return NS_OK;

}

nsresult nsComponentManagerImpl::PlatformPrePopulateRegistry()
{
    nsresult rv;
    char buf[MAXREGPATHLEN];
    PRUint32 bufLength = sizeof(buf);

    if (mPrePopulationDone)
        return NS_OK;

    (void)((nsRegistry *)mRegistry)->SetBufferSize( BIG_REGISTRY_BUFLEN );

    nsCOMPtr<nsIRegistryGetter> regGetter = do_QueryInterface(mRegistry);
    if (!regGetter.get())
        return NS_ERROR_FAILURE;

    // Read in all CID entries and populate the mFactories
    nsCOMPtr<nsIEnumerator> cidEnum;
    rv = mRegistry->EnumerateSubtrees( mCLSIDKey, getter_AddRefs(cidEnum));
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIRegistryEnumerator> regEnum(do_QueryInterface(cidEnum, &rv));
    if (NS_FAILED(rv)) return rv;

    for (rv = regEnum->First();
         NS_SUCCEEDED(rv) && (regEnum->IsDone() != NS_OK);
         rv = regEnum->Next())
    {
        const char *cidString;
        nsRegistryKey cidKey;
        /*
         * CurrentItemInPlaceUTF8 will give us back a _shared_ pointer in 
         * cidString.  This is bad XPCOM practice.  It is evil, and requires
         * great care with the relative lifetimes of cidString and regEnum.
         *
         * It is also faster, and less painful in the allocation department.
         */
        rv = regEnum->CurrentItemInPlaceUTF8(&cidKey, &cidString);
        if (NS_FAILED(rv))  continue;

        // Figure out the component type
        // Use the non allocating registry getter. Our buffer is big enough
        // We wont get NS_ERROR_REG_BUFFER_TOO_SMALL
        bufLength = sizeof(buf);
        rv = regGetter->GetStringUTF8IntoBuffer(cidKey, componentTypeValueName, buf, &bufLength);
        if (NS_FAILED(rv))
            continue;

        int loadertype = GetLoaderType(buf);
        if (loadertype < 0) {
            loadertype = AddLoaderType(buf);
        }

        // Create the CID entry
        bufLength = sizeof(buf);
        rv = regGetter->GetBytesUTF8IntoBuffer(cidKey, inprocServerValueName,
                                               (PRUint8 *) buf, &bufLength);
        if (NS_FAILED(rv))
            continue;
        nsCID aClass;
        if (!(aClass.Parse(cidString)))
            continue;

        nsFactoryEntry *entry = new nsFactoryEntry(aClass, buf, loadertype);
        if (!entry)
            return NS_ERROR_OUT_OF_MEMORY;

        nsAutoMonitor mon(mMon);

        nsFactoryTableEntry* factoryTableEntry =
            NS_STATIC_CAST(nsFactoryTableEntry*,
                           PL_DHashTableOperate(&mFactories, &aClass,
                                                PL_DHASH_ADD));

        if (!factoryTableEntry)
            return NS_ERROR_OUT_OF_MEMORY;

        factoryTableEntry->mFactoryEntry = entry;
    }

    // Finally read in CONTRACTID -> CID mappings
    nsCOMPtr<nsIEnumerator> contractidEnum;
    rv = mRegistry->EnumerateSubtrees( mClassesKey, getter_AddRefs(contractidEnum));
    if (NS_FAILED(rv)) return rv;

    regEnum = do_QueryInterface(contractidEnum, &rv);
    if (NS_FAILED(rv)) return rv;

    for (rv = regEnum->First();
         NS_SUCCEEDED(rv) && (regEnum->IsDone() != NS_OK);
         rv = regEnum->Next())
    {
        const char *contractidString;
        nsRegistryKey contractidKey;
        /*
         * CurrentItemInPlaceUTF8 will give us back a _shared_ pointer in 
         * contractidString.  This is bad XPCOM practice.  It is evil, and requires
         * great care with the relative lifetimes of contractidString and regEnum.
         *
         * It is also faster, and less painful in the allocation department.
         */
        rv = regEnum->CurrentItemInPlaceUTF8(&contractidKey, &contractidString);
        if (NS_FAILED(rv))
            continue;

        // Use the non allocating registry getter. Our buffer is big enough
        // We wont get NS_ERROR_REG_BUFFER_TOO_SMALL
        bufLength = sizeof(buf);
        rv = regGetter->GetStringUTF8IntoBuffer(contractidKey, classIDValueName, buf, &bufLength);
        if (NS_FAILED(rv))
            continue;
        nsCID aClass;
        if (!aClass.Parse(buf))
            continue;

        // put the {contractid, Cid} mapping into our map
        HashContractID(contractidString, aClass);
    }

    // Create the category manage to ensure prepopulation of categories
    nsCOMPtr<nsICategoryManager> catman = do_GetService(NS_CATEGORYMANAGER_CONTRACTID);

    // reset registry buffer to standard size
    (void)((nsRegistry *)mRegistry)->SetBufferSize( -1 );

    mPrePopulationDone = PR_TRUE;
    return NS_OK;
}

#endif /* USE_REGISTRY */

//
// HashContractID
//
nsresult 
nsComponentManagerImpl::HashContractID(const char *aContractID, const nsCID &aClass, nsFactoryEntry **pfe)
{
    nsIDKey cidKey(aClass);
    return HashContractID(aContractID, aClass, cidKey, pfe);
}


nsresult 
nsComponentManagerImpl::HashContractID(const char *aContractID, const nsCID &aClass, nsIDKey &cidKey, nsFactoryEntry **pfe)
{
    if(!aContractID)
    {
        return NS_ERROR_NULL_POINTER;
    }
    
    // Find the factory entry corresponding to the CID.
    nsFactoryEntry *entry = GetFactoryEntry(aClass, cidKey);
    if (!entry) {
        // Non existent. We use the special kNonExistentContractID to mark
        // that this contractid does not have a mapping.
        entry = kNonExistentContractID;
    }

    nsresult rv = HashContractID(aContractID, entry);
    if (NS_FAILED(rv))
        return rv;

    // Fill the entry out parameter
    if (pfe) *pfe = entry;
    return NS_OK;
}

nsresult 
nsComponentManagerImpl::HashContractID(const char *aContractID, nsFactoryEntry *fe)
{
    if(!aContractID)
        return NS_ERROR_NULL_POINTER;
    
    nsAutoMonitor mon(mMon);

    nsContractIDTableEntry* contractIDTableEntry =
        NS_STATIC_CAST(nsContractIDTableEntry*,
                       PL_DHashTableOperate(&mContractIDs, aContractID,
                                            PL_DHASH_ADD));
    if (!contractIDTableEntry)
        return NS_ERROR_OUT_OF_MEMORY;

    if (!contractIDTableEntry->mContractID)
        contractIDTableEntry->mContractID = nsCRT::strdup(aContractID);

    contractIDTableEntry->mFactoryEntry = fe;
 
    return NS_OK;
}

////////////////////////////////////////////////////////////////////////////////
// nsComponentManagerImpl: Public methods
////////////////////////////////////////////////////////////////////////////////

/**
 * LoadFactory()
 *
 * Given a FactoryEntry, this loads the dll if it has to, find the NSGetFactory
 * symbol, calls the routine to create a new factory and returns it to the
 * caller.
 *
 * No attempt is made to store the factory in any form anywhere.
 */
nsresult
nsComponentManagerImpl::LoadFactory(nsFactoryEntry *aEntry,
                                    nsIFactory **aFactory)
{

    if (!aFactory)
        return NS_ERROR_NULL_POINTER;
    *aFactory = NULL;

    nsresult rv;
    rv = aEntry->GetFactory(aFactory, this);
    if (NS_FAILED(rv)) {
        PR_LOG(nsComponentManagerLog, PR_LOG_ERROR,
               ("nsComponentManager: FAILED to load factory from %s (%s)\n",
                (const char *)aEntry->location, mLoaderData[aEntry->typeIndex].type));
        return rv;
    }
        
    return NS_OK;
}

nsFactoryEntry *
nsComponentManagerImpl::GetFactoryEntry(const char *aContractID, int checkRegistry)
{
    nsFactoryEntry *fe = nsnull;
    {
    
        nsAutoMonitor mon(mMon);

        nsContractIDTableEntry* contractIDTableEntry =
            NS_STATIC_CAST(nsContractIDTableEntry*,
                           PL_DHashTableOperate(&mContractIDs, aContractID,
                                                PL_DHASH_LOOKUP));
  
    
        if (PL_DHASH_ENTRY_IS_BUSY(contractIDTableEntry)) {
            fe = contractIDTableEntry->mFactoryEntry;
        }
    }   //exit monitor

#ifdef USE_REGISTRY
    if (!fe)
    {
        if (checkRegistry < 0) {
            // default
            checkRegistry = !mPrePopulationDone;
        }

        if (checkRegistry)
        {
            nsCID cid;
            nsresult rv = PlatformContractIDToCLSID(aContractID, &cid);
            if (NS_SUCCEEDED(rv)) {
                HashContractID(aContractID, cid, &fe);
            }
        }
    }
#endif /* USE_REGISTRY */

    // If no mapping found, add a special non-existent mapping
    // so the next time around, we dont have to waste time doing the
    // same mapping over and over again
    if (!fe) {
        fe = kNonExistentContractID;
        HashContractID(aContractID, fe);
    }

    return (fe);
}


nsFactoryEntry *
nsComponentManagerImpl::GetFactoryEntry(const nsCID &aClass, int checkRegistry)
{
    nsIDKey cidKey(aClass);
    return GetFactoryEntry(aClass, cidKey, checkRegistry);
}


nsFactoryEntry *
nsComponentManagerImpl::GetFactoryEntry(const nsCID &aClass, nsIDKey &cidKey, int checkRegistry)
{
    nsFactoryEntry *entry = nsnull;
    {
        nsAutoMonitor mon(mMon);
        
        nsFactoryTableEntry* factoryTableEntry =
            NS_STATIC_CAST(nsFactoryTableEntry*,
                           PL_DHashTableOperate(&mFactories, &aClass,
                                                PL_DHASH_LOOKUP));
         
        if (PL_DHASH_ENTRY_IS_BUSY(factoryTableEntry)) {
            entry = factoryTableEntry->mFactoryEntry;
        }
    }   // exit monitor
   

#ifdef USE_REGISTRY
    if (!entry)
    {
        if (checkRegistry < 0) {
            // default
            checkRegistry = !mPrePopulationDone;
        }

        if (checkRegistry) {
            nsresult rv = PlatformFind(aClass, &entry);

            // If we got one, cache it in our hashtable
            if (NS_SUCCEEDED(rv))
            {
                nsAutoMonitor mon(mMon);

                nsFactoryTableEntry* factoryTableEntry =
                    NS_STATIC_CAST(nsFactoryTableEntry*,
                                   PL_DHashTableOperate(&mFactories, &cidKey,
                                                        PL_DHASH_ADD));
                
                if (!factoryTableEntry) {
                    return nsnull;
                }
                           
                factoryTableEntry->mFactoryEntry = entry;
              
             }
         }
     }
#endif /* USE_REGISTRY */

    return (entry);
}


/**
 * FindFactory()
 *
 * Given a classID, this finds the factory for this CID by first searching the
 * local CID<->factory mapping. Next it searches for a Dll that implements
 * this classID and calls LoadFactory() to create the factory.
 *
 * Again, no attempt is made at storing the factory.
 */
nsresult
nsComponentManagerImpl::FindFactory(const nsCID &aClass,
                                    nsIFactory **aFactory) 
{
    PR_ASSERT(aFactory != NULL);

    nsFactoryEntry *entry = GetFactoryEntry(aClass);

    if (!entry)
        return NS_ERROR_FACTORY_NOT_REGISTERED;

    return entry->GetFactory(aFactory, this);
}


nsresult
nsComponentManagerImpl::FindFactory(const char *contractID,
                                    nsIFactory **aFactory) 
{
    PR_ASSERT(aFactory != NULL);

    nsFactoryEntry *entry = GetFactoryEntry(contractID);

    if (!entry || entry == kNonExistentContractID)
        return NS_ERROR_FACTORY_NOT_REGISTERED;

    return entry->GetFactory(aFactory, this);
}

/**
 * GetClassObject()
 *
 * Given a classID, this finds the singleton ClassObject that implements the CID.
 * Returns an interface of type aIID off the singleton classobject.
 */
nsresult
nsComponentManagerImpl::GetClassObject(const nsCID &aClass, const nsIID &aIID,
                                       void **aResult) 
{
    nsresult rv;

    nsCOMPtr<nsIFactory> factory;

    if (PR_LOG_TEST(nsComponentManagerLog, PR_LOG_ALWAYS))
    {
        char *buf = aClass.ToString();
        PR_LogPrint("nsComponentManager: GetClassObject(%s)", buf);
        delete [] buf;
    }

    PR_ASSERT(aResult != NULL);
    
    rv = FindFactory(aClass, getter_AddRefs(factory));
    if (NS_FAILED(rv)) return rv;

    rv = factory->QueryInterface(aIID, aResult);

    PR_LOG(nsComponentManagerLog, PR_LOG_WARNING,
           ("\t\tGetClassObject() %s", NS_SUCCEEDED(rv) ? "succeeded" : "FAILED"));
        
    return rv;
}


nsresult
nsComponentManagerImpl::GetClassObjectByContractID(const char *contractID, 
                                                   const nsIID &aIID,
                                                   void **aResult) 
{
    nsresult rv;

    nsCOMPtr<nsIFactory> factory;

    if (PR_LOG_TEST(nsComponentManagerLog, PR_LOG_ALWAYS))
    {
        PR_LogPrint("nsComponentManager: GetClassObject(%s)", contractID);
    }

    PR_ASSERT(aResult != NULL);
    
    rv = FindFactory(contractID, getter_AddRefs(factory));
    if (NS_FAILED(rv)) return rv;

    rv = factory->QueryInterface(aIID, aResult);

    PR_LOG(nsComponentManagerLog, PR_LOG_WARNING,
           ("\t\tGetClassObject() %s", NS_SUCCEEDED(rv) ? "succeeded" : "FAILED"));
        
    return rv;
}

/**
 * ContractIDToClassID()
 *
 * Mapping function from a ContractID to a classID. Directly talks to the registry.
 *
 */
nsresult
nsComponentManagerImpl::ContractIDToClassID(const char *aContractID, nsCID *aClass) 
{
    NS_PRECONDITION(aContractID != NULL, "null ptr");
    if (! aContractID)
        return NS_ERROR_NULL_POINTER;

    NS_PRECONDITION(aClass != NULL, "null ptr");
    if (! aClass)
        return NS_ERROR_NULL_POINTER;

    nsresult res = NS_ERROR_FACTORY_NOT_REGISTERED;

    nsFactoryEntry *fe = GetFactoryEntry(aContractID);
    if (fe && fe != kNonExistentContractID) {
        *aClass = fe->cid;
        res = NS_OK;
    }

    if (PR_LOG_TEST(nsComponentManagerLog, PR_LOG_ALWAYS)) {
        char *buf = 0;
        if (NS_SUCCEEDED(res))
            buf = aClass->ToString();
        PR_LOG(nsComponentManagerLog, PR_LOG_ALWAYS,
               ("nsComponentManager: ContractIDToClassID(%s)->%s", aContractID,
                NS_SUCCEEDED(res) ? buf : "[FAILED]"));
        if (NS_SUCCEEDED(res))
            delete [] buf;
    }

    return res;
}

/**
 * CLSIDToContractID()
 *
 * Translates a classID to a {ContractID, Class Name}. Does direct registry
 * access to do the translation.
 *
 * NOTE: Since this isn't heavily used, we arent caching this.
 */
nsresult
nsComponentManagerImpl::CLSIDToContractID(const nsCID &aClass,
                                      char* *aClassName,
                                      char* *aContractID)
{
    nsresult res = NS_ERROR_FACTORY_NOT_REGISTERED;

#ifdef USE_REGISTRY
    res = PlatformCLSIDToContractID(&aClass, aClassName, aContractID);
#endif /* USE_REGISTRY */

    if (PR_LOG_TEST(nsComponentManagerLog, PR_LOG_ALWAYS))
    {
        char *buf = aClass.ToString();
        PR_LOG(nsComponentManagerLog, PR_LOG_WARNING,
               ("nsComponentManager: CLSIDToContractID(%s)->%s", buf,
                NS_SUCCEEDED(res) ? *aContractID : "[FAILED]"));
        delete [] buf;
    }

    return res;
}

/**
 * CreateInstance()
 *
 * Create an instance of an object that implements an interface and belongs
 * to the implementation aClass using the factory. The factory is immediately
 * released and not held onto for any longer.
 */
nsresult 
nsComponentManagerImpl::CreateInstance(const nsCID &aClass, 
                                       nsISupports *aDelegate,
                                       const nsIID &aIID,
                                       void **aResult)
{
    // test this first, since there's no point in creating a component during
    // shutdown -- whether it's available or not would depend on the order it
    // occurs in the list
    if (gXPCOMShuttingDown) {
        // When processing shutdown, dont process new GetService() requests
#ifdef SHOW_DENIED_ON_SHUTDOWN
        nsXPIDLCString cid, iid;
        cid.Adopt(aClass.ToString());
        iid.Adopt(aIID.ToString());
        printf("Creating new instance on shutdown. Denied.\n"
               "         CID: %s\n         IID: %s\n", cid.get(), iid.get());
#endif /* SHOW_DENIED_ON_SHUTDOWN */
        return NS_ERROR_UNEXPECTED;
    }

    if (aResult == NULL)
    {
        return NS_ERROR_NULL_POINTER;
    }
    *aResult = NULL;
        
    nsIFactory *factory = NULL;
    nsresult res = FindFactory(aClass, &factory);
    if (NS_SUCCEEDED(res))
    {
        res = factory->CreateInstance(aDelegate, aIID, aResult);
        NS_RELEASE(factory);
    }
    else
    {
        // Translate error values
        res = NS_ERROR_FACTORY_NOT_REGISTERED;
    }

    if (PR_LOG_TEST(nsComponentManagerLog, PR_LOG_ALWAYS)) 
    {
        char *buf = aClass.ToString();
        PR_LOG(nsComponentManagerLog, PR_LOG_ALWAYS,
               ("nsComponentManager: CreateInstance(%s) %s", buf,
                NS_SUCCEEDED(res) ? "succeeded" : "FAILED"));
        delete [] buf;
    }

    return res;
}

/**
 * CreateInstanceByContractID()
 *
 * A variant of CreateInstance() that creates an instance of the object that
 * implements the interface aIID and whose implementation has a contractID aContractID.
 *
 * This is only a convenience routine that turns around can calls the
 * CreateInstance() with classid and iid.
 */
nsresult
nsComponentManagerImpl::CreateInstanceByContractID(const char *aContractID,
                                               nsISupports *aDelegate,
                                               const nsIID &aIID,
                                               void **aResult)
{
    // test this first, since there's no point in creating a component during
    // shutdown -- whether it's available or not would depend on the order it
    // occurs in the list
    if (gXPCOMShuttingDown) {
        // When processing shutdown, dont process new GetService() requests
#ifdef SHOW_DENIED_ON_SHUTDOWN
        nsXPIDLCString iid;
        iid.Adopt(aIID.ToString());
        printf("Creating new instance on shutdown. Denied.\n"
               "  ContractID: %s\n         IID: %s\n", aContractID, iid.get());
#endif /* SHOW_DENIED_ON_SHUTDOWN */
        return NS_ERROR_UNEXPECTED;
    }

    if (aResult == NULL)
    {
        return NS_ERROR_NULL_POINTER;
    }
    *aResult = NULL;
        
    nsIFactory *factory = NULL;
    nsresult res = FindFactory(aContractID, &factory);
    if (NS_SUCCEEDED(res))
    {
        res = factory->CreateInstance(aDelegate, aIID, aResult);
        NS_RELEASE(factory);
    }
    else
    {
        // Translate error values
        res = NS_ERROR_FACTORY_NOT_REGISTERED;
    }

    PR_LOG(nsComponentManagerLog, PR_LOG_ALWAYS,
           ("nsComponentManager: CreateInstanceByContractID(%s) %s", aContractID,
            NS_SUCCEEDED(res) ? "succeeded" : "FAILED"));
    
    return res;
}

// Service Manager Impl

PLDHashOperator PR_CALLBACK
FreeServiceFactoryEntryEnumerate(PLDHashTable *aTable,
                                 PLDHashEntryHdr *aHdr,
                                 PRUint32 aNumber,
                                 void *aData)
{
    nsFactoryTableEntry* entry = NS_STATIC_CAST(nsFactoryTableEntry*, aHdr);

    if (!entry->mFactoryEntry || entry->mFactoryEntry == kNonExistentContractID) 
        return PL_DHASH_NEXT;
    
    nsFactoryEntry* factoryEntry = entry->mFactoryEntry;
    factoryEntry->mServiceObject = nsnull;
    return PL_DHASH_NEXT;
}

PLDHashOperator PR_CALLBACK
FreeServiceContractIDEntryEnumerate(PLDHashTable *aTable,
                                    PLDHashEntryHdr *aHdr,
                                    PRUint32 aNumber,
                                    void *aData)
{
    nsContractIDTableEntry* entry = NS_STATIC_CAST(nsContractIDTableEntry*, aHdr);

    if (!entry->mFactoryEntry || entry->mFactoryEntry == kNonExistentContractID) 
        return PL_DHASH_NEXT;

    nsFactoryEntry* factoryEntry = entry->mFactoryEntry;
    factoryEntry->mServiceObject = nsnull;
    return PL_DHASH_NEXT;
}

nsresult 
nsComponentManagerImpl::FreeServices()
{
    NS_ASSERTION(gXPCOMShuttingDown, "Must be shutting down in order to free all services");

    if (!gXPCOMShuttingDown)
        return NS_ERROR_FAILURE;

    if (mFactories.ops) {
        PL_DHashTableEnumerate(&mFactories, FreeServiceFactoryEntryEnumerate, nsnull);
    }

    if (mContractIDs.ops) {
        PL_DHashTableEnumerate(&mContractIDs, FreeServiceContractIDEntryEnumerate, nsnull);
    }

    return NS_OK;
}
NS_IMETHODIMP
nsComponentManagerImpl::GetService(const nsCID& aClass, 
                                   const nsIID& aIID,
                                   void* *result)
{
    nsAutoMonitor mon(mMon);

    // test this first, since there's no point in returning a service during
    // shutdown -- whether it's available or not would depend on the order it
    // occurs in the list
    if (gXPCOMShuttingDown) {
        // When processing shutdown, dont process new GetService() requests
#ifdef SHOW_DENIED_ON_SHUTDOWN
        nsXPIDLCString cid, iid;
        cid.Adopt(aClass.ToString());
        iid.Adopt(aIID.ToString());
        printf("Getting service on shutdown. Denied.\n"
               "         CID: %s\n         IID: %s\n", cid.get(), iid.get());
#endif /* SHOW_DENIED_ON_SHUTDOWN */
        return NS_ERROR_UNEXPECTED;
    }

    nsresult rv = NS_OK;
    nsIDKey key(aClass);
    nsFactoryEntry* entry = nsnull;
    nsFactoryTableEntry* factoryTableEntry =
        NS_STATIC_CAST(nsFactoryTableEntry*,
                       PL_DHashTableOperate(&mFactories, &aClass,
                                            PL_DHASH_LOOKUP));
  
    if (PL_DHASH_ENTRY_IS_BUSY(factoryTableEntry)) {
        entry = factoryTableEntry->mFactoryEntry;
    }

    if (entry && entry->mServiceObject) {
         return entry->mServiceObject->QueryInterface(aIID, result);
    }

    nsCOMPtr<nsISupports> service;
    // We need to not be holding the service manager's monitor while calling 
    // CreateInstance, because it invokes user code which could try to re-enter
    // the service manager:
    mon.Exit();

    rv = CreateInstance(aClass, NULL, aIID, getter_AddRefs(service));

    mon.Enter();

    if (NS_FAILED(rv))
        return rv;
  
    if (!entry) { // second hash lookup for GetService
        nsFactoryTableEntry* factoryTableEntry =
            NS_STATIC_CAST(nsFactoryTableEntry*,
                           PL_DHashTableOperate(&mFactories, &aClass,
                                                PL_DHASH_LOOKUP));
        if (PL_DHASH_ENTRY_IS_BUSY(factoryTableEntry)) {
            entry = factoryTableEntry->mFactoryEntry;
        }
        NS_ASSERTION(entry, "we should have a factory entry since CI succeeded - we should not get here");
        if (!entry) return NS_ERROR_FAILURE; 
    }

    entry->mServiceObject = service; 
    *result = service.get();
    NS_ADDREF(NS_STATIC_CAST(nsISupports*, (*result))); 
    return rv;
}

NS_IMETHODIMP
nsComponentManagerImpl::RegisterService(const nsCID& aClass, nsISupports* aService)
{
    nsAutoMonitor mon(mMon);

    // check to see if we have a factory entry for the service
    nsIDKey key(aClass);
    nsFactoryEntry *entry = GetFactoryEntry(aClass, key, 0);

    if (!entry) { // XXXdougt - should we require that all services register factories??  probably not.
        entry = new nsFactoryEntry(aClass, nsnull);
        if (entry == nsnull)
            return NS_ERROR_OUT_OF_MEMORY;
        entry->typeIndex = NS_COMPONENT_TYPE_SERVICE_ONLY;
        nsFactoryTableEntry* factoryTableEntry =
            NS_STATIC_CAST(nsFactoryTableEntry*,
                           PL_DHashTableOperate(&mFactories, &aClass,
                                                PL_DHASH_ADD));
        if (!factoryTableEntry)
            return NS_ERROR_OUT_OF_MEMORY;
       
        factoryTableEntry->mFactoryEntry = entry;
    }
    else {
        if (entry->mServiceObject)
            return NS_ERROR_FAILURE;
    }

    entry->mServiceObject = aService;
    return NS_OK;
}

NS_IMETHODIMP
nsComponentManagerImpl::UnregisterService(const nsCID& aClass)
{
    nsresult rv = NS_OK;
    
    nsFactoryEntry* entry = nsnull;

    nsAutoMonitor mon(mMon);

    nsFactoryTableEntry* factoryTableEntry =
        NS_STATIC_CAST(nsFactoryTableEntry*,
                       PL_DHashTableOperate(&mFactories, &aClass,
                                            PL_DHASH_LOOKUP));

    if (PL_DHASH_ENTRY_IS_BUSY(factoryTableEntry)) {
        entry = factoryTableEntry->mFactoryEntry;
    }

    if (!entry || !entry->mServiceObject)
        return NS_ERROR_SERVICE_NOT_FOUND;
    
    entry->mServiceObject = nsnull;
    return rv;
}

NS_IMETHODIMP
nsComponentManagerImpl::RegisterService(const char* aContractID, nsISupports* aService)
{

    nsAutoMonitor mon(mMon);

    // check to see if we have a factory entry for the service
    nsFactoryEntry *entry = GetFactoryEntry(aContractID, 0);

    if (entry == kNonExistentContractID)
        entry = nsnull;

    if (!entry) { // XXXdougt - should we require that all services register factories??  probably not.
        entry = new nsFactoryEntry(kEmptyCID, nsnull);
        if (entry == nsnull)
            return NS_ERROR_OUT_OF_MEMORY;
        entry->typeIndex = NS_COMPONENT_TYPE_SERVICE_ONLY;
   
        nsContractIDTableEntry* contractIDTableEntry =
            NS_STATIC_CAST(nsContractIDTableEntry*,
                           PL_DHashTableOperate(&mContractIDs, aContractID,
                                                PL_DHASH_ADD));
        if (!contractIDTableEntry) {
            delete entry;
            return NS_ERROR_OUT_OF_MEMORY;
        }

        if (!contractIDTableEntry->mContractID)
            contractIDTableEntry->mContractID = nsCRT::strdup(aContractID);

        contractIDTableEntry->mFactoryEntry = entry;
    }
    else {
        if (entry->mServiceObject)
            return NS_ERROR_FAILURE;
    }

    entry->mServiceObject = aService;
    return NS_OK;
}


NS_IMETHODIMP 
nsComponentManagerImpl::IsServiceInstantiated(const nsCID & aClass,
                                              const nsIID& aIID,
                                              PRBool *result)
{
    // Now we want to get the service if we already got it. If not, we dont want
    // to create an instance of it. mmh!

    // test this first, since there's no point in returning a service during
    // shutdown -- whether it's available or not would depend on the order it
    // occurs in the list
    if (gXPCOMShuttingDown) {
        // When processing shutdown, dont process new GetService() requests
#ifdef SHOW_DENIED_ON_SHUTDOWN
        nsXPIDLCString cid, iid;
        cid.Adopt(aClass.ToString());
        iid.Adopt(aIID.ToString());
        printf("Checking for service on shutdown. Denied.\n"
               "         CID: %s\n         IID: %s\n", cid.get(), iid.get());
#endif /* SHOW_DENIED_ON_SHUTDOWN */
        return NS_ERROR_UNEXPECTED;
    }

    nsresult rv = NS_ERROR_SERVICE_NOT_FOUND;
    nsFactoryEntry* entry = nsnull;
    nsFactoryTableEntry* factoryTableEntry =
        NS_STATIC_CAST(nsFactoryTableEntry*,
                       PL_DHashTableOperate(&mFactories, &aClass,
                                            PL_DHASH_LOOKUP));
  
    if (PL_DHASH_ENTRY_IS_BUSY(factoryTableEntry)) {
        entry = factoryTableEntry->mFactoryEntry;
    }

    if (entry && entry->mServiceObject) {
        nsCOMPtr<nsISupports> service;
        rv = entry->mServiceObject->QueryInterface(aIID, getter_AddRefs(service));
        *result =(service!=nsnull);
    }
    return rv;

}

NS_IMETHODIMP nsComponentManagerImpl::IsServiceInstantiatedByContractID(const char *aContractID, 
                                                                        const nsIID& aIID,
                                                                        PRBool *result)
{
    // Now we want to get the service if we already got it. If not, we dont want
    // to create an instance of it. mmh!

    // test this first, since there's no point in returning a service during
    // shutdown -- whether it's available or not would depend on the order it
    // occurs in the list
    if (gXPCOMShuttingDown) {
        // When processing shutdown, dont process new GetService() requests
#ifdef SHOW_DENIED_ON_SHUTDOWN
        nsXPIDLCString iid;
        iid.Adopt(aIID.ToString());
        printf("Checking for service on shutdown. Denied.\n"
               "  ContractID: %s\n         IID: %s\n", aContractID, iid.get());
#endif /* SHOW_DENIED_ON_SHUTDOWN */
        return NS_ERROR_UNEXPECTED;
    }

    nsresult rv = NS_ERROR_SERVICE_NOT_FOUND;
    nsFactoryEntry *entry = nsnull;
    {
        nsAutoMonitor mon(mMon);

        nsContractIDTableEntry* contractIDTableEntry =
            NS_STATIC_CAST(nsContractIDTableEntry*,
                           PL_DHashTableOperate(&mContractIDs, aContractID,
                                                PL_DHASH_LOOKUP));
    
        if (PL_DHASH_ENTRY_IS_BUSY(contractIDTableEntry)) {
            entry = contractIDTableEntry->mFactoryEntry;
        }
    }   // exit monitor

    if (entry && entry != kNonExistentContractID && entry->mServiceObject) {
        nsCOMPtr<nsISupports> service;
        rv = entry->mServiceObject->QueryInterface(aIID, getter_AddRefs(service));
        *result =(service!=nsnull);
    }
    return rv;
}


NS_IMETHODIMP
nsComponentManagerImpl::UnregisterService(const char* aContractID)
{
    nsresult rv = NS_OK;

    nsAutoMonitor mon(mMon);

    nsFactoryEntry *entry = nsnull;
    nsContractIDTableEntry* contractIDTableEntry =
       NS_STATIC_CAST(nsContractIDTableEntry*,
                      PL_DHashTableOperate(&mContractIDs, aContractID,
                                           PL_DHASH_LOOKUP));
 
   if (PL_DHASH_ENTRY_IS_BUSY(contractIDTableEntry)) {
       entry = contractIDTableEntry->mFactoryEntry;
   }
   
   if (entry == nsnull || entry == kNonExistentContractID || entry->mServiceObject == nsnull)
        return NS_ERROR_SERVICE_NOT_FOUND;

   entry->mServiceObject = nsnull;
   return rv;
}

NS_IMETHODIMP
nsComponentManagerImpl::GetServiceByContractID(const char* aContractID, 
                                               const nsIID& aIID,
                                               void* *result)
{
    nsAutoMonitor mon(mMon);

    // test this first, since there's no point in returning a service during
    // shutdown -- whether it's available or not would depend on the order it
    // occurs in the list
    if (gXPCOMShuttingDown) {
        // When processing shutdown, dont process new GetService() requests
#ifdef SHOW_DENIED_ON_SHUTDOWN
        nsXPIDLCString iid;
        iid.Adopt(aIID.ToString());
        printf("Getting service on shutdown. Denied.\n"
               "  ContractID: %s\n         IID: %s\n", aContractID, iid.get());
#endif /* SHOW_DENIED_ON_SHUTDOWN */
        return NS_ERROR_UNEXPECTED;
    }

    nsresult rv = NS_OK;
    nsFactoryEntry *entry = nsnull;
    nsContractIDTableEntry* contractIDTableEntry =
        NS_STATIC_CAST(nsContractIDTableEntry*,
                       PL_DHashTableOperate(&mContractIDs, aContractID,
                                            PL_DHASH_LOOKUP));
  
    if (PL_DHASH_ENTRY_IS_BUSY(contractIDTableEntry)) {
        entry = contractIDTableEntry->mFactoryEntry;
    }

    if (entry && entry != kNonExistentContractID && entry->mServiceObject) {
        return entry->mServiceObject->QueryInterface(aIID, result);
    }

    nsCOMPtr<nsISupports> service;
    // We need to not be holding the service manager's monitor while calling 
    // CreateInstance, because it invokes user code which could try to re-enter
    // the service manager:
    mon.Exit();

    rv = CreateInstanceByContractID(aContractID, NULL, aIID, getter_AddRefs(service));

    mon.Enter();

    if (NS_FAILED(rv))
        return rv;
  
    if (!entry) { // second hash lookup for GetService
        nsContractIDTableEntry* contractIDTableEntry =
            NS_STATIC_CAST(nsContractIDTableEntry*,
                           PL_DHashTableOperate(&mContractIDs, aContractID,
                                                PL_DHASH_LOOKUP));
  
        if (PL_DHASH_ENTRY_IS_BUSY(contractIDTableEntry)) {
            entry = contractIDTableEntry->mFactoryEntry;
        }
        NS_ASSERTION(entry, "we should have a factory entry since CI succeeded - we should not get here");
        if (!entry) return NS_ERROR_FAILURE; 
    }
  
    entry->mServiceObject = service;
    *result = service.get();
    NS_ADDREF(NS_STATIC_CAST(nsISupports*, (*result)));
    return rv;
}

NS_IMETHODIMP 
nsComponentManagerImpl::GetService(const nsCID& aClass, const nsIID& aIID,
                                     nsISupports* *result,
                                     nsIShutdownListener* shutdownListener)
{
    return GetService(aClass, aIID, (void**)result);
};

NS_IMETHODIMP 
nsComponentManagerImpl::GetService(const char* aContractID, const nsIID& aIID,
                                     nsISupports* *result,
                                     nsIShutdownListener* shutdownListener )
{
    return GetServiceByContractID(aContractID, aIID, (void**)result);
};


NS_IMETHODIMP 
nsComponentManagerImpl::ReleaseService(const nsCID& aClass, nsISupports* service,
                                         nsIShutdownListener* shutdownListener )
{
    NS_IF_RELEASE(service);
    return NS_OK;
};

NS_IMETHODIMP 
nsComponentManagerImpl::ReleaseService(const char* aContractID, nsISupports* service,
                                         nsIShutdownListener* shutdownListener )
{
    NS_IF_RELEASE(service);
    return NS_OK;
};

/*
 * I want an efficient way to allocate a buffer to the right size
 * and stick the prefix and dllName in, then be able to hand that buffer
 * off to the FactoryEntry.  Is that so wrong?
 *
 * *regName is allocated on success.
 *
 * This should live in nsNativeComponentLoader.cpp, I think.
 */
static nsresult
MakeRegistryName(const char *aDllName, const char *prefix, char **regName)
{
    char *registryName;

    PRUint32 len = nsCRT::strlen(prefix);

    PRUint32 registryNameLen = nsCRT::strlen(aDllName) + len;
    registryName = (char *)nsMemory::Alloc(registryNameLen + 1);
    
    // from here on it, we want len sans terminating NUL

    if (!registryName)
        return NS_ERROR_OUT_OF_MEMORY;
    
    memcpy(registryName, prefix, len);
    strcpy(registryName + len, aDllName); // no nsCRT::strcpy? for shame!
    registryName[registryNameLen] = '\0';
    *regName = registryName;

#ifdef DEBUG_shaver_off
    fprintf(stderr, "MakeRegistryName(%s, %s, &[%s])\n",
            aDllName, prefix, *regName);
#endif

    return NS_OK;
}

nsresult
nsComponentManagerImpl::RegistryNameForLib(const char *aLibName,
                                           char **aRegistryName)
{
    return MakeRegistryName(aLibName, XPCOM_LIB_PREFIX, aRegistryName);
}

nsresult
nsComponentManagerImpl::RegistryLocationForSpec(nsIFile *aSpec,
                                                char **aRegistryName)
{
    nsresult rv;
    
    if (!mComponentsDir) 
        return NS_ERROR_NOT_INITIALIZED;

    if (!aSpec) {
        *aRegistryName = nsCRT::strdup("");
        return NS_OK;
    }

    PRBool containedIn;
    mComponentsDir->Contains(aSpec, PR_TRUE, &containedIn);

    char *persistentDescriptor;

    if (containedIn){
        
        rv = aSpec->GetPath(&persistentDescriptor);
        if (NS_FAILED(rv))
            return rv;
        
        char* relativeLocation = persistentDescriptor + mComponentsOffset + 1;
        
        rv = MakeRegistryName(relativeLocation, XPCOM_RELCOMPONENT_PREFIX, 
                              aRegistryName);
    } else {
        /* absolute names include volume info on Mac, so persistent descriptor */
        rv = aSpec->GetPath(&persistentDescriptor);
        if (NS_FAILED(rv))
            return rv;
        rv = MakeRegistryName(persistentDescriptor, XPCOM_ABSCOMPONENT_PREFIX,
                              aRegistryName);
    }

    if (persistentDescriptor)
        nsMemory::Free(persistentDescriptor);
        
    return rv;

}

nsresult
nsComponentManagerImpl::SpecForRegistryLocation(const char *aLocation,
                                                nsIFile **aSpec)
{
    nsresult rv;
    if (!aLocation || !aSpec)
        return NS_ERROR_NULL_POINTER;

    /* abs:/full/path/to/libcomponent.so */
    if (!nsCRT::strncmp(aLocation, XPCOM_ABSCOMPONENT_PREFIX, 4)) {

        nsLocalFile* file = new nsLocalFile;
        if (!file) return NS_ERROR_FAILURE;
        
        rv = file->InitWithPath(((char *)aLocation + 4));
        file->QueryInterface(NS_GET_IID(nsILocalFile), (void**)aSpec);
        return rv;
    }

    if (!nsCRT::strncmp(aLocation, XPCOM_RELCOMPONENT_PREFIX, 4)) {
        
        if (!mComponentsDir)
            return NS_ERROR_NOT_INITIALIZED;

        nsILocalFile* file = nsnull;
        rv = mComponentsDir->Clone((nsIFile**)&file);       
        
        if (NS_FAILED(rv)) return rv;
        
        rv = file->AppendRelativePath(aLocation + 4);
        *aSpec = file;
        return rv;
    }
    *aSpec = nsnull;
    return NS_ERROR_INVALID_ARG;
}

/**
 * RegisterFactory()
 *
 * Register a factory to be responsible for creation of implementation of
 * classID aClass. Plus creates as association of aClassName and aContractID
 * to the classID. If replace is PR_TRUE, we replace any existing registrations
 * with this one.
 *
 * Once registration is complete, we add the class to the factories cache
 * that we maintain. The factories cache is the ONLY place where these
 * registrations are ever kept.
 *
 * The other RegisterFunctions create a loader mapping and persistent
 * location, but we just slam it into the cache here.  And we don't call the
 * loader's OnRegister function, either.
 */
nsresult
nsComponentManagerImpl::RegisterFactory(const nsCID &aClass,
                                        const char *aClassName,
                                        const char *aContractID,
                                        nsIFactory *aFactory, 
                                        PRBool aReplace)
{
    nsFactoryEntry *entry = nsnull;
    nsIDKey key(aClass);

    nsAutoMonitor mon(mMon);

    entry = GetFactoryEntry(aClass, key,
                            0 /* dont check registry */);

    if (PR_LOG_TEST(nsComponentManagerLog, PR_LOG_ALWAYS))
    {
        char *buf = aClass.ToString();
        PR_LOG(nsComponentManagerLog, PR_LOG_ALWAYS,
               ("nsComponentManager: RegisterFactory(%s, %s)", buf,
                (aContractID ? aContractID : "(null)")));
        delete [] buf;

    }
    

    if (entry && !aReplace) {
        // Already registered
        PR_LOG(nsComponentManagerLog, PR_LOG_ERROR,
               ("\t\tFactory already registered."));
        return NS_ERROR_FACTORY_EXISTS;
    }

    if (entry) {
        entry->ReInit(aClass, aFactory);
    }
    else {
        entry = new nsFactoryEntry(aClass, aFactory);
        if (entry == nsnull)
            return NS_ERROR_OUT_OF_MEMORY;

        
        nsFactoryTableEntry* factoryTableEntry =
            NS_STATIC_CAST(nsFactoryTableEntry*,
                           PL_DHashTableOperate(&mFactories, &aClass,
                                                PL_DHASH_ADD));
        
        if (!factoryTableEntry)
            return NS_ERROR_OUT_OF_MEMORY;
        
        factoryTableEntry->mFactoryEntry = entry;
    }

    // Update the ContractID->CLSID Map
    if (aContractID) {
        nsresult rv = HashContractID(aContractID, entry);
        if(NS_FAILED(rv)) {
            PR_LOG(nsComponentManagerLog, PR_LOG_WARNING,
                   ("\t\tFactory register succeeded. "
                    "Hashing contractid (%s) FAILED.", aContractID));
            return rv;
        }
    }
        
    PR_LOG(nsComponentManagerLog, PR_LOG_WARNING,
           ("\t\tFactory register succeeded contractid=%s.",
            aContractID ? aContractID : "<none>"));
        
    return NS_OK;
}

nsresult
nsComponentManagerImpl::RegisterComponent(const nsCID &aClass,
                                          const char *aClassName,
                                          const char *aContractID,
                                          const char *aPersistentDescriptor,
                                          PRBool aReplace,
                                          PRBool aPersist)
{
    return RegisterComponentCommon(aClass, aClassName, aContractID,
                                   aPersistentDescriptor, aReplace, aPersist,
                                   nativeComponentType);
}

nsresult
nsComponentManagerImpl::RegisterComponentWithType(const nsCID &aClass,
                                                  const char *aClassName,
                                                  const char *aContractID,
                                                  nsIFile *aSpec,
                                                  const char *aLocation,
                                                  PRBool aReplace,
                                                  PRBool aPersist,
                                                  const char *aType)
{
    return RegisterComponentCommon(aClass, aClassName, aContractID, 
                                   aLocation,
                                   aReplace, aPersist,
                                   aType);
}

/*
 * Register a component, using whatever they stuck in the nsIFile.
 */
nsresult
nsComponentManagerImpl::RegisterComponentSpec(const nsCID &aClass,
                                              const char *aClassName,
                                              const char *aContractID,
                                              nsIFile *aLibrarySpec,
                                              PRBool aReplace,
                                              PRBool aPersist)
{
    nsXPIDLCString registryName;
    nsresult rv = RegistryLocationForSpec(aLibrarySpec, getter_Copies(registryName));
    if (NS_FAILED(rv))
        return rv;

    rv = RegisterComponentWithType(aClass, aClassName, aContractID, aLibrarySpec,
                                   registryName,
                                   aReplace, aPersist,
                                   nativeComponentType);
    return rv;
}

/*
 * Register a ``library'', which is a DLL location named by a simple filename
 * such as ``libnsappshell.so'', rather than a relative or absolute path.
 *
 * It implies application/x-moz-dll as the component type, and skips the
 * FindLoaderForType phase.
 */
nsresult
nsComponentManagerImpl::RegisterComponentLib(const nsCID &aClass,
                                             const char *aClassName,
                                             const char *aContractID,
                                             const char *aDllName,
                                             PRBool aReplace,
                                             PRBool aPersist)
{
    nsXPIDLCString registryName;
    nsresult rv = RegistryNameForLib(aDllName, getter_Copies(registryName));
    if (NS_FAILED(rv))
        return rv;
    return RegisterComponentCommon(aClass, aClassName, aContractID, registryName,
                                   aReplace, aPersist, nativeComponentType);
}

/*
 * Add a component to the known universe of components.

 * Once we enter this function, we own aRegistryName, and must free it
 * or hand it to nsFactoryEntry.  Common exit point ``out'' helps keep us
 * sane.
 */

nsresult
nsComponentManagerImpl::RegisterComponentCommon(const nsCID &aClass,
                                                const char *aClassName,
                                                const char *aContractID,
                                                const char *aRegistryName,
                                                PRBool aReplace,
                                                PRBool aPersist,
                                                const char *aType)
{
    nsresult rv = NS_OK;

    nsIDKey key(aClass);
    nsAutoMonitor mon(mMon);

    nsFactoryEntry *entry = GetFactoryEntry(aClass, !mPrePopulationDone);

    // Normalize proid and classname
    const char *contractID = (aContractID && *aContractID) ? aContractID : NULL;
    const char *className = (aClassName && *aClassName) ? aClassName : NULL;

    if (PR_LOG_TEST(nsComponentManagerLog, PR_LOG_ALWAYS))
    {
        char *buf = aClass.ToString();
        PR_LOG(nsComponentManagerLog, PR_LOG_DEBUG,
               ("nsComponentManager: RegisterComponentCommon(%s, %s, %s, %s)",
                buf,
                contractID ? contractID : "(null)",
                aRegistryName, aType));
        delete [] buf;
    }

    if (entry && !aReplace) {
        PR_LOG(nsComponentManagerLog, PR_LOG_ERROR,
               ("\t\tFactory already registered."));
        return NS_ERROR_FACTORY_EXISTS;
    }

#ifdef USE_REGISTRY
    if (aPersist) {
        /* Add to the registry */
        rv = AddComponentToRegistry(aClass, className, contractID,
                                    aRegistryName, aType);
        if (NS_FAILED(rv)) {
            PR_LOG(nsComponentManagerLog, PR_LOG_ERROR,
                   ("\t\tadding %s %s to registry FAILED", className, contractID));
            return rv;
        }
    }
#endif

    int typeIndex = GetLoaderType(aType);

    nsCOMPtr<nsIComponentLoader> loader;
    rv = GetLoaderForType(typeIndex, getter_AddRefs(loader));
    if (NS_FAILED(rv)) {
        PR_LOG(nsComponentManagerLog, PR_LOG_ERROR,
               ("\t\tgetting loader for %s FAILED\n", aType));
        return rv;
    }
    
    if (entry) {
        entry->ReInit(aClass, aRegistryName, typeIndex);
    }
    else {
        entry = new nsFactoryEntry(aClass, aRegistryName, typeIndex);
        if (!entry)
            return NS_ERROR_OUT_OF_MEMORY;

        nsFactoryTableEntry* factoryTableEntry =
            NS_STATIC_CAST(nsFactoryTableEntry*,
                           PL_DHashTableOperate(&mFactories, &aClass,
                                                PL_DHASH_ADD));
        
        if (!factoryTableEntry)
            return NS_ERROR_OUT_OF_MEMORY;
        
        factoryTableEntry->mFactoryEntry = entry;
    }

   // Update the ContractID->CLSID Map
    if (contractID
#ifdef USE_REGISTRY
        && (mPrePopulationDone || !aPersist)
#endif
        ) {
        rv = HashContractID(contractID, entry);
        if (NS_FAILED(rv)) {
            PR_LOG(nsComponentManagerLog, PR_LOG_ERROR,
                   ("\t\tHashContractID(%s) FAILED\n", contractID));
            return rv;
        }
    }

    // Let the loader do magic things now
    rv = loader->OnRegister(aClass, aType, className, contractID, aRegistryName,
                            aReplace, aPersist);
    if (NS_FAILED(rv)) {
        PR_LOG(nsComponentManagerLog, PR_LOG_ERROR,
               ("\t\tloader->OnRegister FAILED for %s \"%s\" %s %s", aType,
                className, contractID, aRegistryName));
        return rv;
    }
    
    PR_LOG(nsComponentManagerLog, PR_LOG_ALWAYS,
           ("\t\tRegisterComponentCommon() %s",
            NS_SUCCEEDED(rv) ? "succeeded" : "FAILED"));
    return rv;
}

nsresult
nsComponentManagerImpl::GetLoaderForType(int aType,
                                         nsIComponentLoader **aLoader)
{
    nsresult rv;

    // Make sure we have a valid type
    if (aType < 0 || aType >= mNLoaderData)
        return NS_ERROR_INVALID_ARG;

    *aLoader = mLoaderData[aType].loader;
    if (*aLoader) {
        NS_ADDREF(*aLoader);
        return NS_OK;
    }

    nsCOMPtr<nsIComponentLoader> loader;
    loader = do_GetServiceFromCategory("component-loader", mLoaderData[aType].type, &rv);
    if (NS_FAILED(rv))
        return rv;
    
    rv = loader->Init(this, mRegistry);

    if (NS_SUCCEEDED(rv)) {
        mLoaderData[aType].loader = loader;
        NS_ADDREF(mLoaderData[aType].loader);
        *aLoader = loader;
        NS_ADDREF(*aLoader);
    }
    return rv;
}

nsresult
nsComponentManagerImpl::AddComponentToRegistry(const nsCID &aClass,
                                               const char *aClassName,
                                               const char *aContractID,
                                               const char *aRegistryName,
                                               const char *aType)
{
    nsresult rv;
    PRUint32 length = strlen(aRegistryName);
    char* eRegistryName;
    rv = mRegistry->EscapeKey((PRUint8*)aRegistryName, 1, &length, (PRUint8**)&eRegistryName);
    if (rv != NS_OK)
    {
    return rv;
    }
    if (eRegistryName == nsnull)    //  No escaping required
    eRegistryName = (char*)aRegistryName;

    nsRegistryKey IDKey;
    PRInt32 nComponents = 0;
    
    /* so why do we use strings here rather than writing bytes, anyway? */
    char *cidString = aClass.ToString();
    if (!cidString)
        return NS_ERROR_OUT_OF_MEMORY;
    rv = mRegistry->AddSubtreeRaw(mCLSIDKey, cidString, &IDKey);
    if (NS_FAILED(rv))
        goto out;
    
    if (aClassName) {
        rv = mRegistry->SetStringUTF8(IDKey, classNameValueName, aClassName);
        if (NS_FAILED(rv))
            goto out;
    }

    rv = mRegistry->SetBytesUTF8(IDKey, inprocServerValueName, 
            strlen(aRegistryName) + 1, 
            (PRUint8*)aRegistryName);
    if (NS_FAILED(rv))
        goto out;

    rv = mRegistry->SetStringUTF8(IDKey, componentTypeValueName, aType);
    if (NS_FAILED(rv))
        goto out;

    if (aContractID) {
        rv = mRegistry->SetStringUTF8(IDKey, contractIDValueName, aContractID);
        if (NS_FAILED(rv))
            goto out;

        nsRegistryKey contractIDKey;
        rv = mRegistry->AddSubtreeRaw(mClassesKey, aContractID, &contractIDKey);
        if (NS_FAILED(rv))
            goto out;
        rv = mRegistry->SetStringUTF8(contractIDKey, classIDValueName, cidString);
        if (NS_FAILED(rv))
            goto out;
    }

    nsRegistryKey compKey;
    rv = mRegistry->AddSubtreeRaw(mXPCOMKey, eRegistryName, &compKey);
    
    // update component count
    rv = mRegistry->GetInt(compKey, componentCountValueName, &nComponents);
    nComponents++;
    rv = mRegistry->SetInt(compKey, componentCountValueName, nComponents);
    if (NS_FAILED(rv))
        goto out;

 out:
    // XXX if failed, undo registry adds or set invalid bit?  How?
    nsCRT::free(cidString);
    if (eRegistryName != aRegistryName)
    nsMemory::Free(eRegistryName);
    return rv;
}

nsresult
nsComponentManagerImpl::UnregisterFactory(const nsCID &aClass,
                                          nsIFactory *aFactory)
{
    if (PR_LOG_TEST(nsComponentManagerLog, PR_LOG_ALWAYS)) 
    {
        char *buf = aClass.ToString();
        PR_LOG(nsComponentManagerLog, PR_LOG_DEBUG,
               ("nsComponentManager: UnregisterFactory(%s)", buf));
        delete [] buf;
    }
        
    nsIDKey key(aClass);
    nsresult res = NS_ERROR_FACTORY_NOT_REGISTERED;
    nsFactoryEntry *old = GetFactoryEntry(aClass, key,
                                          0 /* dont check registry */);
    if (old != NULL)
    {
        if (old->factory.get() == aFactory)
        {
            nsAutoMonitor mon(mMon);

            PL_DHashTableOperate(&mFactories, &aClass, PL_DHASH_REMOVE);
            old = NULL;
            res = NS_OK;
        }

    }

    PR_LOG(nsComponentManagerLog, PR_LOG_WARNING,
           ("\t\tUnregisterFactory() %s",
            NS_SUCCEEDED(res) ? "succeeded" : "FAILED"));
    return res;
}

nsresult
nsComponentManagerImpl::UnregisterComponent(const nsCID &aClass,
                                            const char *registryName)
{
    nsresult rv = NS_OK;

    NS_ENSURE_ARG_POINTER(registryName);

    nsAutoMonitor mon(mMon);

    // Remove any stored factory entries
    nsIDKey key(aClass);
    nsFactoryEntry *entry = GetFactoryEntry(aClass, key,
                                            0 /* dont check registry */);
    if (entry && entry->location && !PL_strcasecmp(entry->location, registryName))
    {
        PL_DHashTableOperate(&mFactories, &aClass, PL_DHASH_REMOVE);
        entry = nsnull;
    }

#ifdef USE_REGISTRY
    // Remove registry entries for this cid
    char *cidString = aClass.ToString();
    rv = PlatformUnregister(cidString, registryName);
    delete [] cidString;
#endif

    PR_LOG(nsComponentManagerLog, PR_LOG_WARNING,
           ("nsComponentManager: Factory unregister(%s) %s.", registryName,
            NS_SUCCEEDED(rv) ? "succeeded" : "FAILED"));

    return rv;
}

nsresult
nsComponentManagerImpl::UnregisterComponentSpec(const nsCID &aClass,
                                                nsIFile *aLibrarySpec)
{
    nsXPIDLCString registryName;
    nsresult rv = RegistryLocationForSpec(aLibrarySpec, getter_Copies(registryName));
    if (NS_FAILED(rv)) return rv;
    return UnregisterComponent(aClass, registryName);
}

// XXX Need to pass in aWhen and servicemanager
nsresult
nsComponentManagerImpl::FreeLibraries(void) 
{
    return UnloadLibraries(NS_STATIC_CAST(nsIServiceManager*, this), NS_Timer); // XXX when
}

// Private implementation of unloading libraries
nsresult
nsComponentManagerImpl::UnloadLibraries(nsIServiceManager *serviceMgr, PRInt32 aWhen)
{
    nsresult rv = NS_OK;

    nsAutoMonitor mon(mMon);
        
    PR_LOG(nsComponentManagerLog, PR_LOG_ALWAYS, 
           ("nsComponentManager: Unloading Libraries."));

    // UnloadAll the loaders
    /* iterate over all known loaders and ask them to autoregister. */
    // Skip mNativeComponentLoader
    for (int i=NS_COMPONENT_TYPE_NATIVE + 1; i<mNLoaderData; i++) {
        if (mLoaderData[i].loader) {
            rv = mLoaderData[i].loader->UnloadAll(aWhen);
            if (NS_FAILED(rv))
                break;
        }
    }

    // UnloadAll the native loader
    rv = mNativeComponentLoader->UnloadAll(aWhen);
    return rv;
}

////////////////////////////////////////////////////////////////////////////////

/**
 * AutoRegister(RegistrationInstant, const char *directory)
 *
 * Given a directory in the following format, this will ensure proper registration
 * of all components. No default director is looked at.
 *
 *    Directory and fullname are what NSPR will accept. For eg.
 *         WIN    y:/home/dp/mozilla/dist/bin
 *      UNIX    /home/dp/mozilla/dist/bin
 *      MAC    /Hard drive/mozilla/dist/apprunner
 *
 * This will take care not loading already registered dlls, finding and
 * registering new dlls, re-registration of modified dlls
 *
 */

nsresult
nsComponentManagerImpl::AutoRegister(PRInt32 when, nsIFile *inDirSpec)
{
    nsresult rv;
    ((nsRegistry *)mRegistry)->SetBufferSize( BIG_REGISTRY_BUFLEN );
    rv = AutoRegisterImpl(when, inDirSpec);
    mRegistry->Flush();
    ((nsRegistry *)mRegistry)->SetBufferSize( -1 );
    return rv;
}

nsresult
nsComponentManagerImpl::AutoRegisterImpl(PRInt32 when, nsIFile *inDirSpec)
{
    nsCOMPtr<nsIFile> dir;
    nsresult rv;

#ifdef DEBUG
    // testing release behaviour
    if (getenv("XPCOM_NO_AUTOREG"))
        return NS_OK;
#endif
    if (inDirSpec) 
    {
        // Use supplied components' directory   
        dir = inDirSpec;
    
        // Set components' directory for AutoRegisterInterfces to query
        nsCOMPtr<nsIProperties> directoryService = 
                 do_GetService(NS_DIRECTORY_SERVICE_CONTRACTID, &rv);
        if (NS_FAILED(rv)) return rv;

        // Don't care if undefining fails
        directoryService->Undefine(NS_XPCOM_COMPONENT_DIR); 
        rv = directoryService->Define(NS_XPCOM_COMPONENT_DIR, dir);
        if (NS_FAILED(rv)) return rv;
    } 
    else 
    {
        // Do default components directory
        nsCOMPtr<nsIProperties> directoryService = 
                 do_GetService(NS_DIRECTORY_SERVICE_CONTRACTID, &rv);
        if (NS_FAILED(rv)) return rv;

        rv = directoryService->Get(NS_XPCOM_COMPONENT_DIR, NS_GET_IID(nsIFile), getter_AddRefs(dir));
        if (NS_FAILED(rv)) return rv; // XXX translate error code?
    }

    nsCOMPtr<nsIInterfaceInfoManager> iim = 
        dont_AddRef(XPTI_GetInterfaceInfoManager());

    if (!iim)
        return NS_ERROR_UNEXPECTED;    
    
    // Notify observers of xpcom autoregistration start
    NS_CreateServicesFromCategory(NS_XPCOM_AUTOREGISTRATION_OBSERVER_ID, nsnull,
                                  "start");

    /* do the native loader first, so we can find other loaders */
    rv = mNativeComponentLoader->AutoRegisterComponents((PRInt32)when, dir);
    if (NS_FAILED(rv)) return rv;

#ifdef ENABLE_STATIC_COMPONENT_LOADER
    rv = mStaticComponentLoader->AutoRegisterComponents((PRInt32)when, dir);
    if (NS_FAILED(rv)) return rv;
#endif

    /* do InterfaceInfoManager after native loader so it can use components. */
    rv = iim->AutoRegisterInterfaces();
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsICategoryManager> catman =
        do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsISimpleEnumerator> loaderEnum;
    rv = catman->EnumerateCategory("component-loader",
                                   getter_AddRefs(loaderEnum));
    if (NS_FAILED(rv)) return rv;

    PRBool hasMore;
    while (NS_SUCCEEDED(loaderEnum->HasMoreElements(&hasMore)) && hasMore) {
        nsCOMPtr<nsISupports> supports;
        if (NS_FAILED(loaderEnum->GetNext(getter_AddRefs(supports))))
            continue;

        nsCOMPtr<nsISupportsString> supStr = do_QueryInterface(supports);
        if (!supStr)
            continue;
        
        nsXPIDLCString loaderType;
        if (NS_FAILED(supStr->GetData(getter_Copies(loaderType))))
            continue;
        
        // We depend on the loader being created. Add the loader type and
        // create the loader object too.
        nsCOMPtr<nsIComponentLoader> loader;
        GetLoaderForType(AddLoaderType(loaderType), getter_AddRefs(loader));
    }

    /* iterate over all known loaders and ask them to autoregister. */
    /* XXX convert when to nsIComponentLoader::(when) properly */
    nsIFile *spec = dir.get();
    for (int i = NS_COMPONENT_TYPE_NATIVE + 1; i < mNLoaderData; i++) {
        if (!mLoaderData[i].loader) {
            rv = GetLoaderForType(i, &mLoaderData[i].loader);
            if (NS_FAILED(rv))
                continue;
        }
        rv = mLoaderData[i].loader->AutoRegisterComponents(when, spec);
        if (NS_FAILED(rv))
            break;
    }

    if (NS_SUCCEEDED(rv))
    {
        PRBool registered;
        do {
            registered = PR_FALSE;
            for (int i = NS_COMPONENT_TYPE_NATIVE; i < mNLoaderData; i++) {
                PRBool b = PR_FALSE;
                if (mLoaderData[i].loader) {
                    rv = mLoaderData[i].loader->RegisterDeferredComponents(when, &b);
                    if (NS_FAILED(rv))
                        continue;
                    registered |= b;
                }
            }
        } while (NS_SUCCEEDED(rv) && registered);
    }

    // Notify observers of xpcom autoregistration completion
    NS_CreateServicesFromCategory(NS_XPCOM_AUTOREGISTRATION_OBSERVER_ID, nsnull,
                                  "end");
    
    return rv;
}

nsresult
nsComponentManagerImpl::AutoRegisterComponent(PRInt32 when,
                                              nsIFile *component)
{
    nsresult rv = NS_OK;
    /*
     * Do we have to give the native loader first crack at it?
     * I vote ``no''.
     */
    for (int i = 0; i < mNLoaderData; i++) {
        PRBool didRegister;
        if (!mLoaderData[i].loader) {
            rv = GetLoaderForType(i, &mLoaderData[i].loader);
            if (NS_FAILED(rv))
                continue;
        }
        rv = mLoaderData[i].loader->AutoRegisterComponent((int)when, component, &didRegister);
        if (NS_SUCCEEDED(rv) && didRegister)
            break;
    }
    return NS_FAILED(rv) ? NS_ERROR_FACTORY_NOT_REGISTERED : NS_OK;

}

nsresult
nsComponentManagerImpl::AutoUnregisterComponent(PRInt32 when,
                                                nsIFile *component)
{
    nsresult rv = NS_OK;
    for (int i = 0; i < mNLoaderData; i++) {
        PRBool didUnRegister;
        if (!mLoaderData[i].loader) {
            rv = GetLoaderForType(i, &mLoaderData[i].loader);
            if (NS_FAILED(rv))
                continue;
        }
        rv = mLoaderData[i].loader->AutoUnregisterComponent(when, component, &didUnRegister);
        if (NS_SUCCEEDED(rv) && didUnRegister)
            break;
    }
    return NS_FAILED(rv) ? NS_ERROR_FACTORY_NOT_REGISTERED : NS_OK;
}

nsresult
nsComponentManagerImpl::IsRegistered(const nsCID &aClass,
                                     PRBool *aRegistered)
{
    if(!aRegistered)
    {
        NS_ASSERTION(0, "null ptr");
        return NS_ERROR_NULL_POINTER;
    }
    *aRegistered = (nsnull != GetFactoryEntry(aClass));
    return NS_OK;
}


typedef NS_CALLBACK(EnumeratorConverter)(PLDHashTable *table,
                                         const PLDHashEntryHdr *hdr,
                                         void *data,
                                         nsISupports **retval);

class PLDHashTableEnumeratorImpl : public nsIBidirectionalEnumerator
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIENUMERATOR
    NS_DECL_NSIBIDIRECTIONALENUMERATOR
    
    virtual ~PLDHashTableEnumeratorImpl();
    PLDHashTableEnumeratorImpl(PLDHashTable *table,
                               EnumeratorConverter converter,
                               void *converterData);
    PRInt32 Count() { return mCount; }
private:
    PLDHashTableEnumeratorImpl(); /* no implementation */
    NS_IMETHODIMP ReleaseElements();
    
    nsVoidArray   mElements;
    PRInt32       mCount, mCurrent;
    PRMonitor*    mMonitor;

    struct Closure {
        PRBool                        succeeded;
        EnumeratorConverter           converter;
        void                          *data;
        PLDHashTableEnumeratorImpl    *impl;
    };

    static PLDHashOperator PR_CALLBACK Enumerator(PLDHashTable *table,
                                      PLDHashEntryHdr *hdr, PRUint32 number,
                                      void *data);
};

// static
PLDHashOperator PR_CALLBACK
PLDHashTableEnumeratorImpl::Enumerator(PLDHashTable *table,
                                       PLDHashEntryHdr *hdr, PRUint32 number,
                                       void *data)
{
    Closure *c = NS_REINTERPRET_CAST(Closure *, data);
    nsISupports *converted;
    if (NS_FAILED(c->converter(table, hdr, c->data, &converted)) ||
        !c->impl->mElements.AppendElement(converted)) {
        c->succeeded = PR_FALSE;
        return PL_DHASH_STOP;
    }

    c->succeeded = PR_TRUE;
    return PL_DHASH_NEXT;
}

PLDHashTableEnumeratorImpl::PLDHashTableEnumeratorImpl
(PLDHashTable *table, EnumeratorConverter converter,
 void *converterData) :
    mCurrent(0)
{
    mMonitor = nsAutoMonitor::NewMonitor("PLDHashTableEnumeratorImpl");
    
    nsAutoMonitor mon(mMonitor);

    NS_INIT_REFCNT();
    Closure c = { PR_FALSE, converter, converterData, this };
    mCount = PL_DHashTableEnumerate(table, Enumerator, &c);
    if (!c.succeeded) {
        ReleaseElements();
        mCount = 0;
    }
}

NS_IMPL_ISUPPORTS2(PLDHashTableEnumeratorImpl, nsIBidirectionalEnumerator,
                   nsIEnumerator);

PLDHashTableEnumeratorImpl::~PLDHashTableEnumeratorImpl()
{
    (void) ReleaseElements();

    // Destroy the Lock
    if (mMonitor)
        PR_DestroyMonitor(mMonitor);
}

NS_IMETHODIMP
PLDHashTableEnumeratorImpl::ReleaseElements()
{
    for (PRInt32 i = 0; i < mCount; i++) {
        nsISupports *supports = NS_REINTERPRET_CAST(nsISupports *,
                                                    mElements[i]);
        NS_IF_RELEASE(supports);
    }
    return NS_OK;
}

NS_IMETHODIMP
PL_NewDHashTableEnumerator(PLDHashTable *table,
                           EnumeratorConverter converter,
                           void *converterData, nsIEnumerator **retval)
{
    PLDHashTableEnumeratorImpl *impl =
        new PLDHashTableEnumeratorImpl(table, converter, converterData);
    
    if (!impl)
        return NS_ERROR_OUT_OF_MEMORY;

    if (impl->Count() == -1) {
        // conversion failed
        delete impl;
        return NS_ERROR_FAILURE;
    }

    return impl->QueryInterface(NS_GET_IID(nsIEnumerator), (void **)retval);
}

NS_IMETHODIMP
PLDHashTableEnumeratorImpl::First()
{
    if (!mCount)
        return NS_ERROR_FAILURE;
    
    mCurrent = 0;
    return NS_OK;
}

NS_IMETHODIMP
PLDHashTableEnumeratorImpl::Last()
{
    if (!mCount)
        return NS_ERROR_FAILURE;
    mCurrent = mCount - 1;
    return NS_OK;
}

NS_IMETHODIMP
PLDHashTableEnumeratorImpl::Prev()
{
    if (!mCurrent)
        return NS_ERROR_FAILURE;

    mCurrent--;
    return NS_OK;
}

NS_IMETHODIMP
PLDHashTableEnumeratorImpl::Next()
{
    // If empty or we're past the end, or we are at the end return error
    if (!mCount || (mCurrent == mCount) || (++mCurrent == mCount))
        return NS_ERROR_FAILURE;
    
    return NS_OK;
}

NS_IMETHODIMP
PLDHashTableEnumeratorImpl::CurrentItem(nsISupports **retval)
{
    if (!mCount || mCurrent == mCount)
        return NS_ERROR_FAILURE;

    *retval = NS_REINTERPRET_CAST(nsISupports *, mElements[mCurrent]);
    if (*retval)
        NS_ADDREF(*retval);

    return NS_OK;
}

NS_IMETHODIMP
PLDHashTableEnumeratorImpl::IsDone()
{
    if (!mCount || (mCurrent == mCount)) {
        return NS_OK;
    }

    return NS_COMFALSE;
}



static NS_IMETHODIMP
ConvertFactoryEntryToCID(PLDHashTable *table,
                         const PLDHashEntryHdr *hdr,
                         void *data, nsISupports **retval)
{
    nsresult rv;

    nsCOMPtr<nsISupportsID> wrapper =
        do_CreateInstance(NS_SUPPORTS_ID_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    const nsFactoryTableEntry *entry = 
        NS_REINTERPRET_CAST(const nsFactoryTableEntry *, hdr);
    if(entry) {
        nsFactoryEntry *fe = entry->mFactoryEntry;
   
        wrapper->SetData(&fe->cid);
        *retval = wrapper;
        NS_ADDREF(*retval);
        return NS_OK;
    }
    else
        *retval = nsnull;

    return rv;

}

static NS_IMETHODIMP
ConvertContractIDKeyToString(PLDHashTable *table,
                             const PLDHashEntryHdr *hdr,
                             void *data, nsISupports **retval)
{
    nsresult rv;

    nsCOMPtr<nsISupportsString> wrapper =
        do_CreateInstance(NS_SUPPORTS_STRING_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    const nsContractIDTableEntry *entry = 
        NS_REINTERPRET_CAST(const nsContractIDTableEntry *, hdr);
    const char *contractID = entry->mContractID;
    nsCAutoString converted;

    converted.Append(contractID);

    wrapper->SetData(converted.get());
    *retval = wrapper;
    NS_ADDREF(*retval);
    return NS_OK;
    
}

nsresult
nsComponentManagerImpl::EnumerateCLSIDs(nsIEnumerator** aEnumerator)
{
    NS_ASSERTION(aEnumerator != nsnull, "null ptr");
    if(!aEnumerator)
    {
        return NS_ERROR_NULL_POINTER;
    }
    *aEnumerator = nsnull;

    nsresult rv;
    if(!mPrePopulationDone)
    {
        rv = PlatformPrePopulateRegistry();
        if(NS_FAILED(rv))
            return rv;
    }

    return PL_NewDHashTableEnumerator(&mFactories,
                                      ConvertFactoryEntryToCID,
                                      nsnull, aEnumerator);
}

nsresult
nsComponentManagerImpl::EnumerateContractIDs(nsIEnumerator** aEnumerator)
{
    NS_ASSERTION(aEnumerator != nsnull, "null ptr");
    if(!aEnumerator)
    {
        return NS_ERROR_NULL_POINTER;
    }

    *aEnumerator = nsnull;

    nsresult rv;
    if(!mPrePopulationDone)
    {
        rv = PlatformPrePopulateRegistry();
        if(NS_FAILED(rv))
            return rv;
    }

    return PL_NewDHashTableEnumerator(&mContractIDs,
                                      ConvertContractIDKeyToString,
                                      nsnull, aEnumerator);
}


nsresult
nsComponentManagerImpl::GetInterface(const nsIID & uuid, void **result)
{
    if (uuid.Equals(NS_GET_IID(nsIServiceManager)))
    {
        *result = NS_STATIC_CAST(nsIServiceManager*, this);
        NS_ADDREF_THIS(); // dougt? extra addrefs??? 
        return NS_OK;
    }
    
    // fall through to QI as anything QIable is a superset of what canbe
    // got via the GetInterface()
    return  QueryInterface(uuid, result);
}

// Convert a loader type string into an index into the component data
// array. Empty loader types are converted to NATIVE. Returns -1 if
// loader type cannot be determined.
int
nsComponentManagerImpl::GetLoaderType(const char *typeStr)
{
    if (!typeStr || !*typeStr) {
        // Empty type strings are NATIVE
        return NS_COMPONENT_TYPE_NATIVE;
    }

    for (int i=NS_COMPONENT_TYPE_NATIVE; i<mNLoaderData; i++) {
        if (!strcmp(typeStr, mLoaderData[i].type))
            return i;
    }
    // Not found
    return NS_COMPONENT_TYPE_FACTORY_ONLY;
}

// Add a loader type if not already known. Return the typeIndex
// if the loader type is either added or already there.
int
nsComponentManagerImpl::AddLoaderType(const char *typeStr)
{
    int typeIndex = GetLoaderType(typeStr);
    if (typeIndex >= 0) {
        return typeIndex;
    }

    // Add the loader type
    if (mNLoaderData >= mMaxNLoaderData) {
        NS_ASSERTION(mNLoaderData == mMaxNLoaderData,
                     "Memory corruption. nsComponentManagerImpl::mLoaderData array overrun.");
        // Need to increase our loader array
        nsLoaderdata *new_mLoaderData = (nsLoaderdata *) PR_Realloc(mLoaderData, (mMaxNLoaderData + NS_LOADER_DATA_ALLOC_STEP) * sizeof(nsLoaderdata));
        if (!new_mLoaderData)
            return NS_ERROR_OUT_OF_MEMORY;
        mLoaderData = new_mLoaderData;
        mMaxNLoaderData += NS_LOADER_DATA_ALLOC_STEP;
    }

    typeIndex = mNLoaderData;
    mLoaderData[typeIndex].type = PL_strdup(typeStr);
    if (!mLoaderData[typeIndex].type) {
        // mmh! no memory. return failure.
        return NS_ERROR_OUT_OF_MEMORY;
    }
    mLoaderData[typeIndex].loader = nsnull;
    mNLoaderData++;

    return typeIndex;
}

////////////////////////////////////////////////////////////////////////////////

NS_COM nsresult
NS_GetGlobalComponentManager(nsIComponentManager* *result)
{
#ifdef DEBUG_dougt
    //    NS_WARNING("DEPRECATED FUNCTION: Use NS_GetComponentManager");
#endif
    nsresult rv = NS_OK;

    if (nsComponentManagerImpl::gComponentManager == NULL)
    {
        // XPCOM needs initialization.
        rv = NS_InitXPCOM2(nsnull, nsnull, nsnull);
    }

    if (NS_SUCCEEDED(rv))
    {
        // NO ADDREF since this is never intended to be released.
        // See nsComponentManagerObsolete.h for the reason for such
        // casting uglyness  
        *result = (nsIComponentManager*)(void*)(nsIComponentManagerObsolete*) nsComponentManagerImpl::gComponentManager;
    }

    return rv;
}




NS_COM nsresult
NS_GetComponentManager(nsIComponentManager* *result)
{
    nsresult rv = NS_OK;

    if (nsComponentManagerImpl::gComponentManager == NULL)
    {
        // XPCOM needs initialization.
        rv = NS_InitXPCOM2(nsnull, nsnull, nsnull);
    }

    if (NS_FAILED(rv))
        return rv;
  
    
    *result = NS_STATIC_CAST(nsIComponentManager*, 
                             nsComponentManagerImpl::gComponentManager);
    NS_IF_ADDREF(*result);
    return NS_OK;
}

NS_COM nsresult
NS_GetServiceManager(nsIServiceManager* *result)
{
    nsresult rv = NS_OK;

    if (nsComponentManagerImpl::gComponentManager == NULL)
    {
        // XPCOM needs initialization.
        rv = NS_InitXPCOM2(nsnull, nsnull, nsnull);
    }

    if (NS_FAILED(rv))
        return rv;
  
  
    *result = NS_STATIC_CAST(nsIServiceManager*, 
                             nsComponentManagerImpl::gComponentManager);
    NS_IF_ADDREF(*result);
    return NS_OK;
}


