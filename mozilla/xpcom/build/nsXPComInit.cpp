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
 */

#include "nsIRegistry.h"
#include "nscore.h"
#include "nsCOMPtr.h"
#include "nsObserverService.h"
#include "nsObserver.h"
#include "nsProperties.h"

#include "nsAllocator.h"
#include "nsArena.h"
#include "nsByteBuffer.h"
#ifdef PAGE_MANAGER
#include "nsPageMgr.h"
#endif
#include "nsSupportsArray.h"
#include "nsSupportsPrimitives.h"
#include "nsUnicharBuffer.h"

#include "nsComponentManager.h"
#include "nsIServiceManager.h"
#include "nsGenericFactory.h"

#include "nsEventQueueService.h"
#include "nsEventQueue.h"

#include "nsProxyObjectManager.h"
#include "nsProxyEventPrivate.h"  // access to the impl of nsProxyObjectManager for the generic factory registration.

#include "nsFileSpecImpl.h"
#include "xptinfo.h"

#include "nsThread.h"

#ifdef _NEW_NSIFILE
#include "nsIFileImpl.h"
#include "nsIDirEnumeratorImpl.h"
#endif

#ifdef GC_LEAK_DETECTOR
#include "nsLeakDetector.h"
#endif

// Include files that dont reside under xpcom
// Our goal was to make this zero. But... :-(
#include "nsICaseConversion.h"

// base
static NS_DEFINE_CID(kAllocatorCID, NS_ALLOCATOR_CID);
// ds
static NS_DEFINE_CID(kArenaCID, NS_ARENA_CID);
static NS_DEFINE_CID(kByteBufferCID, NS_BYTEBUFFER_CID);
#ifdef PAGE_MANAGER
static NS_DEFINE_CID(kPageManagerCID, NS_PAGEMANAGER_CID);
#endif
static NS_DEFINE_CID(kPropertiesCID, NS_PROPERTIES_CID);
static NS_DEFINE_CID(kSupportsArrayCID, NS_SUPPORTSARRAY_CID);
static NS_DEFINE_CID(kUnicharBufferCID, NS_UNICHARBUFFER_CID);
// ds/nsISupportsPrimitives
static NS_DEFINE_CID(kSupportsIDCID, NS_SUPPORTS_ID_CID);
static NS_DEFINE_CID(kSupportsStringCID, NS_SUPPORTS_STRING_CID);
static NS_DEFINE_CID(kSupportsWStringCID, NS_SUPPORTS_WSTRING_CID);
static NS_DEFINE_CID(kSupportsPRBoolCID, NS_SUPPORTS_PRBOOL_CID);
static NS_DEFINE_CID(kSupportsPRUint8CID, NS_SUPPORTS_PRUINT8_CID);
static NS_DEFINE_CID(kSupportsPRUint16CID, NS_SUPPORTS_PRUINT16_CID);
static NS_DEFINE_CID(kSupportsPRUint32CID, NS_SUPPORTS_PRUINT32_CID);
static NS_DEFINE_CID(kSupportsPRUint64CID, NS_SUPPORTS_PRUINT64_CID);
static NS_DEFINE_CID(kSupportsPRTimeCID, NS_SUPPORTS_PRTIME_CID);
static NS_DEFINE_CID(kSupportsCharCID, NS_SUPPORTS_CHAR_CID);
static NS_DEFINE_CID(kSupportsPRInt16CID, NS_SUPPORTS_PRINT16_CID);
static NS_DEFINE_CID(kSupportsPRInt32CID, NS_SUPPORTS_PRINT32_CID);
static NS_DEFINE_CID(kSupportsPRInt64CID, NS_SUPPORTS_PRINT64_CID);
static NS_DEFINE_CID(kSupportsFloatCID, NS_SUPPORTS_FLOAT_CID);
static NS_DEFINE_CID(kSupportsDoubleCID, NS_SUPPORTS_DOUBLE_CID);
static NS_DEFINE_CID(kSupportsVoidCID, NS_SUPPORTS_VOID_CID);
// io
static NS_DEFINE_CID(kFileSpecCID, NS_FILESPEC_CID);
static NS_DEFINE_CID(kDirectoryIteratorCID, NS_DIRECTORYITERATOR_CID);

// components
static NS_DEFINE_CID(kComponentManagerCID, NS_COMPONENTMANAGER_CID);
static NS_DEFINE_CID(kGenericFactoryCID, NS_GENERICFACTORY_CID);
// threads
static NS_DEFINE_CID(kEventQueueServiceCID, NS_EVENTQUEUESERVICE_CID);
static NS_DEFINE_CID(kEventQueueCID, NS_EVENTQUEUE_CID);
static NS_DEFINE_CID(kThreadCID, NS_THREAD_CID);
static NS_DEFINE_CID(kThreadPoolCID, NS_THREADPOOL_CID);
// proxy
static NS_DEFINE_CID(kProxyObjectManagerCID, NS_PROXYEVENT_MANAGER_CID);


// ds/nsISupportsPrimitives
NS_GENERIC_FACTORY_CONSTRUCTOR(nsSupportsIDImpl)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsSupportsStringImpl)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsSupportsWStringImpl)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsSupportsPRBoolImpl)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsSupportsPRUint8Impl)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsSupportsPRUint16Impl)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsSupportsPRUint32Impl)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsSupportsPRUint64Impl)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsSupportsPRTimeImpl)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsSupportsCharImpl)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsSupportsPRInt16Impl)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsSupportsPRInt32Impl)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsSupportsPRInt64Impl)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsSupportsFloatImpl)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsSupportsDoubleImpl)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsSupportsVoidImpl)

////////////////////////////////////////////////////////////////////////////////
// XPCOM initialization
//
// To Control the order of initialization of these key components I am putting
// this function.
//
//	- nsServiceManager
//		- nsComponentManager
//			- nsRegistry
//
// Here are key points to remember:
//	- A global of all these need to exist. nsServiceManager is an independent object.
//	  nsComponentManager uses both the globalServiceManager and its own registry.
//
//	- A static object of both the nsComponentManager and nsServiceManager
//	  are in use. Hence InitXPCOM() gets triggered from both
//	  NS_GetGlobale{Service/Component}Manager() calls.
//
//	- There exists no global Registry. Registry can be created from the component manager.
//

static nsresult
RegisterGenericFactory(nsIComponentManager* compMgr, const nsCID& cid, const char* className,
                       const char *progid, nsIGenericFactory::ConstructorProcPtr constr)
{
    nsresult rv;
    nsIGenericFactory* fact;
    rv = NS_NewGenericFactory(&fact, constr);
    if (NS_FAILED(rv)) return rv;
    rv = compMgr->RegisterFactory(cid, className, progid, fact, PR_TRUE);
    NS_RELEASE(fact);
    return rv;
}

// Globals in xpcom

nsIServiceManager* nsServiceManager::mGlobalServiceManager = NULL;
nsComponentManagerImpl* nsComponentManagerImpl::gComponentManager = NULL;
nsICaseConversion *gCaseConv = NULL;

nsresult NS_COM NS_InitXPCOM(nsIServiceManager* *result,
                             nsFileSpec *registryFile, nsFileSpec *componentDir)
{
    nsresult rv = NS_OK;

#ifdef GC_LEAK_DETECTOR
	// 0. Initialize the GC.
	rv = NS_InitGarbageCollector();
	if (NS_FAILED(rv)) return rv;
#endif

	// Establish the main thread here.
    rv = nsIThread::SetMainThread();
	if (NS_FAILED(rv)) return rv;

    // 1. Create the Global Service Manager
    nsIServiceManager* servMgr = NULL;
    if (nsServiceManager::mGlobalServiceManager == NULL)
    {
        rv = NS_NewServiceManager(&servMgr);
        if (NS_FAILED(rv)) return rv;
        nsServiceManager::mGlobalServiceManager = servMgr;
        if (result)
		{
	        NS_ADDREF(servMgr);
			*result = servMgr;
		}
    }

    // 2. Create the Component Manager and register with global service manager
    //    It is understood that the component manager can use the global service manager.
    nsComponentManagerImpl *compMgr = NULL;
    if (nsComponentManagerImpl::gComponentManager == NULL)
    {
        compMgr = new nsComponentManagerImpl();
        if (compMgr == NULL)
            return NS_ERROR_OUT_OF_MEMORY;
        NS_ADDREF(compMgr);

        // Set the registry file
        if (registryFile && registryFile->IsFile())
        {
            nsSpecialSystemDirectory::Set(nsSpecialSystemDirectory::XPCOM_CurrentProcessComponentRegistry,
                                          registryFile);
        }
        if (componentDir && componentDir->IsDirectory())
        {
            nsSpecialSystemDirectory::Set(nsSpecialSystemDirectory::XPCOM_CurrentProcessComponentDirectory,
                                          componentDir);
        }
        rv = compMgr->Init();
        if (NS_FAILED(rv))
        {
            NS_RELEASE(compMgr);
            return rv;
        }
        nsComponentManagerImpl::gComponentManager = compMgr;
    }
    
    rv = servMgr->RegisterService(kComponentManagerCID, NS_STATIC_CAST(nsIComponentManager*, compMgr));
    if (NS_FAILED(rv)) return rv;

    // 3. Register the global services with the component manager so that
    //    clients can create new objects.

    // Registry
    nsIFactory *registryFactory = NULL;
    rv = NS_RegistryGetFactory(&registryFactory);
    if (NS_FAILED(rv)) return rv;

    NS_DEFINE_CID(kRegistryCID, NS_REGISTRY_CID);

    rv = compMgr->RegisterFactory(kRegistryCID,
                                  NS_REGISTRY_CLASSNAME,
                                  NS_REGISTRY_PROGID,
                                  registryFactory, PR_TRUE);
    NS_RELEASE(registryFactory);
    if (NS_FAILED(rv)) return rv;

#ifdef GC_LEAK_DETECTOR
	rv = NS_InitLeakDetector();
    if (NS_FAILED(rv)) return rv;
#endif

    rv = RegisterGenericFactory(compMgr, kAllocatorCID,
                                NS_ALLOCATOR_CLASSNAME,
                                NS_ALLOCATOR_PROGID,
                                nsAllocatorImpl::Create);
    if (NS_FAILED(rv)) return rv;

    rv = RegisterGenericFactory(compMgr, kArenaCID,
                                NS_ARENA_CLASSNAME,
                                NS_ARENA_PROGID,
                                ArenaImpl::Create);
    if (NS_FAILED(rv)) return rv;

#if 0
    rv = RegisterGenericFactory(compMgr, kBufferCID,
                                NS_BUFFER_CLASSNAME,
                                NS_BUFFER_PROGID,
                                nsBuffer::Create);
    if (NS_FAILED(rv)) return rv;
#endif

    rv = RegisterGenericFactory(compMgr, kByteBufferCID,
                                NS_BYTEBUFFER_CLASSNAME,
                                NS_BYTEBUFFER_PROGID,
                                ByteBufferImpl::Create);
    if (NS_FAILED(rv)) return rv;

    rv = RegisterGenericFactory(compMgr, kFileSpecCID,
                                NS_FILESPEC_CLASSNAME,
                                NS_FILESPEC_PROGID,
                                nsFileSpecImpl::Create);
    if (NS_FAILED(rv)) return rv;

    rv = RegisterGenericFactory(compMgr, kDirectoryIteratorCID,
                                NS_DIRECTORYITERATOR_CLASSNAME,
                                NS_DIRECTORYITERATOR_PROGID,
                                nsDirectoryIteratorImpl::Create);
    if (NS_FAILED(rv)) return rv;

#ifdef PAGE_MANAGER
    rv = RegisterGenericFactory(compMgr, kPageManagerCID,
                                NS_PAGEMANAGER_CLASSNAME,
                                NS_PAGEMANAGER_PROGID,
                                nsPageMgr::Create);
    if (NS_FAILED(rv)) return rv;
#endif

    rv = RegisterGenericFactory(compMgr, kPropertiesCID,
                                NS_PROPERTIES_CLASSNAME,
                                NS_PROPERTIES_PROGID,
                                nsProperties::Create);
    if (NS_FAILED(rv)) return rv;

    rv = RegisterGenericFactory(compMgr, kPersistentPropertiesCID,
                                NS_PERSISTENTPROPERTIES_CLASSNAME,
                                NS_PERSISTENTPROPERTIES_PROGID,
                                nsPersistentProperties::Create);
    if (NS_FAILED(rv)) return rv;

    rv = RegisterGenericFactory(compMgr, kSupportsArrayCID,
                                NS_SUPPORTSARRAY_CLASSNAME,
                                NS_SUPPORTSARRAY_PROGID,
                                nsSupportsArray::Create);
    if (NS_FAILED(rv)) return rv;

    rv = RegisterGenericFactory(compMgr, nsObserver::GetCID(), 
                                NS_OBSERVER_CLASSNAME,
                                NS_OBSERVER_PROGID,
                                nsObserver::Create);
    if (NS_FAILED(rv)) return rv;

    rv = RegisterGenericFactory(compMgr, nsObserverService::GetCID(),
                                NS_OBSERVERSERVICE_CLASSNAME,
                                NS_OBSERVERSERVICE_PROGID,
                                nsObserverService::Create);
    if (NS_FAILED(rv)) return rv;

    rv = RegisterGenericFactory(compMgr, kGenericFactoryCID,
                                NS_GENERICFACTORY_CLASSNAME,
                                NS_GENERICFACTORY_PROGID,
                                nsGenericFactory::Create);
    if (NS_FAILED(rv)) return rv;

    rv = RegisterGenericFactory(compMgr, kEventQueueServiceCID,
                                NS_EVENTQUEUESERVICE_CLASSNAME,
                                NS_EVENTQUEUESERVICE_PROGID,
                                nsEventQueueServiceImpl::Create);
    if (NS_FAILED(rv)) return rv;

    rv = RegisterGenericFactory(compMgr, kEventQueueCID,
                                NS_EVENTQUEUE_CLASSNAME,
                                NS_EVENTQUEUE_PROGID,
                                nsEventQueueImpl::Create);
    if (NS_FAILED(rv)) return rv;

    rv = RegisterGenericFactory(compMgr, kThreadCID,
                                NS_THREAD_CLASSNAME,
                                NS_THREAD_PROGID,
                                nsThread::Create);
    if (NS_FAILED(rv)) return rv;

    rv = RegisterGenericFactory(compMgr, kThreadPoolCID,
                                NS_THREADPOOL_CLASSNAME,
                                NS_THREADPOOL_PROGID,
                                nsThreadPool::Create);
    if (NS_FAILED(rv)) return rv;

    rv = RegisterGenericFactory(compMgr, 
                                kProxyObjectManagerCID, 
                                NS_XPCOMPROXY_CLASSNAME,
                                NS_XPCOMPROXY_PROGID,
                                nsProxyObjectManager::Create);
    if (NS_FAILED(rv)) return rv;


    rv = RegisterGenericFactory(compMgr, kSupportsIDCID,
                                NS_SUPPORTS_ID_CLASSNAME,
                                NS_SUPPORTS_ID_PROGID,
                                nsSupportsIDImplConstructor);
    if (NS_FAILED(rv)) return rv;

    rv = RegisterGenericFactory(compMgr, kSupportsStringCID,
                                NS_SUPPORTS_STRING_CLASSNAME,
                                NS_SUPPORTS_STRING_PROGID,
                                nsSupportsStringImplConstructor);
    if (NS_FAILED(rv)) return rv;

    rv = RegisterGenericFactory(compMgr, kSupportsWStringCID,
                                NS_SUPPORTS_WSTRING_CLASSNAME,
                                NS_SUPPORTS_WSTRING_PROGID,
                                nsSupportsWStringImplConstructor);
    if (NS_FAILED(rv)) return rv;

    rv = RegisterGenericFactory(compMgr, kSupportsPRBoolCID,
                                NS_SUPPORTS_PRBOOL_CLASSNAME,
                                NS_SUPPORTS_PRBOOL_PROGID,
                                nsSupportsPRBoolImplConstructor);
    if (NS_FAILED(rv)) return rv;

    rv = RegisterGenericFactory(compMgr, kSupportsPRUint8CID,
                                NS_SUPPORTS_PRUINT8_CLASSNAME,
                                NS_SUPPORTS_PRUINT8_PROGID,
                                nsSupportsPRUint8ImplConstructor);
    if (NS_FAILED(rv)) return rv;

    rv = RegisterGenericFactory(compMgr, kSupportsPRUint16CID,
                                NS_SUPPORTS_PRUINT16_CLASSNAME,
                                NS_SUPPORTS_PRUINT16_PROGID,
                                nsSupportsPRUint16ImplConstructor);
    if (NS_FAILED(rv)) return rv;

    rv = RegisterGenericFactory(compMgr, kSupportsPRUint32CID,
                                NS_SUPPORTS_PRUINT32_CLASSNAME,
                                NS_SUPPORTS_PRUINT32_PROGID,
                                nsSupportsPRUint32ImplConstructor);
    if (NS_FAILED(rv)) return rv;

    rv = RegisterGenericFactory(compMgr, kSupportsPRUint64CID,
                                NS_SUPPORTS_PRUINT64_CLASSNAME,
                                NS_SUPPORTS_PRUINT64_PROGID,
                                nsSupportsPRUint64ImplConstructor);
    if (NS_FAILED(rv)) return rv;

    rv = RegisterGenericFactory(compMgr, kSupportsPRTimeCID,
                                NS_SUPPORTS_PRTIME_CLASSNAME,
                                NS_SUPPORTS_PRTIME_PROGID,
                                nsSupportsPRTimeImplConstructor);
    if (NS_FAILED(rv)) return rv;

    rv = RegisterGenericFactory(compMgr, kSupportsCharCID,
                                NS_SUPPORTS_CHAR_CLASSNAME,
                                NS_SUPPORTS_CHAR_PROGID,
                                nsSupportsCharImplConstructor);
    if (NS_FAILED(rv)) return rv;

    rv = RegisterGenericFactory(compMgr, kSupportsPRInt16CID,
                                NS_SUPPORTS_PRINT16_CLASSNAME,
                                NS_SUPPORTS_PRINT16_PROGID,
                                nsSupportsPRInt16ImplConstructor);
    if (NS_FAILED(rv)) return rv;

    rv = RegisterGenericFactory(compMgr, kSupportsPRInt32CID,
                                NS_SUPPORTS_PRINT32_CLASSNAME,
                                NS_SUPPORTS_PRINT32_PROGID,
                                nsSupportsPRInt32ImplConstructor);
    if (NS_FAILED(rv)) return rv;

    rv = RegisterGenericFactory(compMgr, kSupportsPRInt64CID,
                                NS_SUPPORTS_PRINT64_CLASSNAME,
                                NS_SUPPORTS_PRINT64_PROGID,
                                nsSupportsPRInt64ImplConstructor);
    if (NS_FAILED(rv)) return rv;

    rv = RegisterGenericFactory(compMgr, kSupportsFloatCID,
                                NS_SUPPORTS_FLOAT_CLASSNAME,
                                NS_SUPPORTS_FLOAT_PROGID,
                                nsSupportsFloatImplConstructor);
    if (NS_FAILED(rv)) return rv;

    rv = RegisterGenericFactory(compMgr, kSupportsDoubleCID,
                                NS_SUPPORTS_DOUBLE_CLASSNAME,
                                NS_SUPPORTS_DOUBLE_PROGID,
                                nsSupportsDoubleImplConstructor);
    if (NS_FAILED(rv)) return rv;

    rv = RegisterGenericFactory(compMgr, kSupportsVoidCID,
                                NS_SUPPORTS_VOID_CLASSNAME,
                                NS_SUPPORTS_VOID_PROGID,
                                nsSupportsVoidImplConstructor);
    if (NS_FAILED(rv)) return rv;

#ifdef _NEW_NSIFILE
    rv = RegisterGenericFactory(compMgr, nsIFileImpl::GetCID(),
                                NS_FILE_CLASSNAME,
                                NS_FILE_PROGID,
                                nsIFileImpl::Create);
    if (NS_FAILED(rv)) return rv;

    rv = RegisterGenericFactory(compMgr, nsIDirEnumeratorImpl::GetCID(),
                                NS_DIRECTORY_ENUMERATOR_CLASS,
                                NS_DIRECTORY_ENUMERATOR_PROGID,
                                nsIDirEnumeratorImpl::Create);
    if (NS_FAILED(rv)) return rv;
#endif


    // Prepopulate registry for performance
    // Ignore return value. It is ok if this fails.
    nsComponentManagerImpl::gComponentManager->PlatformPrePopulateRegistry();

    return rv;
}

//
// NS_ShutdownXPCOM()
//
// The shutdown sequence for xpcom would be
//
// - Release the Global Service Manager
//   - Release all service instances held by the global service manager
//   - Release the Global Service Manager itself
// - Release the Component Manager
//   - Release all factories cached by the Component Manager
//   - Unload Libraries
//   - Release Progid Cache held by Component Manager
//   - Release dll abstraction held by Component Manager
//   - Release the Registry held by Component Manager
//   - Finally, release the component manager itself
//
nsresult NS_COM NS_ShutdownXPCOM(nsIServiceManager* servMgr)
{
    nsrefcnt cnt;

    // Notify observers of xpcom shutting down
    nsresult rv = NS_OK;
    {
        // Block it so that the COMPtr will get deleted before we hit
        // servicemanager shutdown
        NS_WITH_SERVICE (nsIObserverService, observerService, NS_OBSERVERSERVICE_PROGID, &rv);
        if (NS_SUCCEEDED(rv))
        {
            nsIServiceManager *mgr;		// NO COMPtr as we dont release the service manager
            rv = nsServiceManager::GetGlobalServiceManager(&mgr);
            if (NS_SUCCEEDED(rv))
            {
                nsAutoString topic = NS_XPCOM_SHUTDOWN_OBSERVER_ID;
                (void) observerService->Notify(mgr, topic.GetUnicode(), nsnull);
            }
        }
    }

    // Release our own singletons...
    XPTI_FreeInterfaceInfoManager();

    // We may have AddRef'd for the caller of NS_InitXPCOM, so release it
    // here again:
    
    NS_IF_RELEASE(servMgr);
    NS_RELEASE2(nsServiceManager::mGlobalServiceManager, cnt);
    NS_ASSERTION(cnt == 0, "Service Manager being held past XPCOM shutdown.");

    // Release the global case converter
    NS_IF_RELEASE(gCaseConv);

#if defined(DEBUG_shaver) || defined(DEBUG_dp)
    /* shaver needs to fix this and turn this on for the world */

    // Shutdown xpcom. This will release all loaders and cause others holding
    // a refcount to the component manager to release it.
    rv = (nsComponentManagerImpl::gComponentManager)->Shutdown();
    NS_ASSERTION(NS_SUCCEEDED(rv), "Component Manager shutdown failed.");
#endif

    // Finally, release the component manager last because it unloads the
    // libraries:
    NS_RELEASE2(nsComponentManagerImpl::gComponentManager, cnt);
    NS_ASSERTION(cnt == 0, "Component Manager being held past XPCOM shutdown.");

#ifdef DEBUG
    extern void _FreeAutoLockStatics();
    _FreeAutoLockStatics();
#endif

    NS_PurgeAtomTable();

    nsTraceRefcnt::DumpStatistics();
    nsTraceRefcnt::ResetStatistics();

#ifdef GC_LEAK_DETECTOR
	// Shutdown the Leak detector.
	NS_ShutdownLeakDetector();
	// Shutdown the GC.
	NS_ShutdownGarbageCollector();
#endif

    return NS_OK;
}
