/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: NPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the NPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "nsXPCOM.h"
#include "nsXPCOMPrivate.h"
#include "nsIRegistry.h"
#include "nscore.h"
#include "nsCOMPtr.h"
#include "nsObserverList.h"
#include "nsObserver.h"
#include "nsObserverService.h"
#include "nsProperties.h"
#include "nsIProperties.h"
#include "nsPersistentProperties.h"
#include "nsScriptableInputStream.h"

#include "nsMemoryImpl.h"
#include "nsErrorService.h"
#include "nsArena.h"
#include "nsByteBuffer.h"

#include "nsSupportsArray.h"
#include "nsSupportsPrimitives.h"
#include "nsConsoleService.h"
#include "nsExceptionService.h"

#include "nsComponentManager.h"
#include "nsIServiceManager.h"
#include "nsGenericFactory.h"

#include "nsEventQueueService.h"
#include "nsEventQueue.h"

#include "nsIProxyObjectManager.h"
#include "nsProxyEventPrivate.h"  // access to the impl of nsProxyObjectManager for the generic factory registration.

#include "xptinfo.h"
#include "nsIInterfaceInfoManager.h"

#include "nsTimerImpl.h"
#include "TimerThread.h"

#include "nsThread.h"
#include "nsProcess.h"

#include "nsFileSpecImpl.h"
#include "nsSpecialSystemDirectory.h"
#include "nsEmptyEnumerator.h"

#include "nsILocalFile.h"
#include "nsLocalFile.h"
#include "nsDirectoryService.h"
#include "nsDirectoryServiceDefs.h"
#include "nsICategoryManager.h"
#include "nsStringStream.h"

#include "nsFastLoadService.h"

#include "nsAtomService.h"
#include "nsAtomTable.h"
#include "nsTraceRefcnt.h"
#include "nsTimelineService.h"

#include "nsVariant.h"

#ifdef GC_LEAK_DETECTOR
#include "nsLeakDetector.h"
#endif

static NS_DEFINE_CID(kComponentManagerCID, NS_COMPONENTMANAGER_CID);
static NS_DEFINE_CID(kMemoryCID, NS_MEMORY_CID);
static NS_DEFINE_CID(kEventQueueServiceCID, NS_EVENTQUEUESERVICE_CID);

NS_GENERIC_FACTORY_CONSTRUCTOR(nsProcess);

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
NS_GENERIC_FACTORY_CONSTRUCTOR(nsSupportsInterfacePointerImpl)

NS_GENERIC_FACTORY_CONSTRUCTOR(nsConsoleService);
NS_GENERIC_FACTORY_CONSTRUCTOR(nsAtomService);
NS_GENERIC_FACTORY_CONSTRUCTOR(nsExceptionService);
NS_GENERIC_FACTORY_CONSTRUCTOR(nsTimerImpl);

NS_GENERIC_FACTORY_CONSTRUCTOR(nsVariant);

#ifdef MOZ_TIMELINE
NS_GENERIC_FACTORY_CONSTRUCTOR(nsTimelineService);
#endif

static NS_METHOD
nsXPTIInterfaceInfoManagerGetSingleton(nsISupports* outer,
                                       const nsIID& aIID,
                                       void* *aInstancePtr)
{
    NS_ENSURE_ARG_POINTER(aInstancePtr);
    NS_ENSURE_TRUE(!outer, NS_ERROR_NO_AGGREGATION);

    nsCOMPtr<nsIInterfaceInfoManager> iim(dont_AddRef(XPTI_GetInterfaceInfoManager()));
    if (!iim) {
        return NS_ERROR_FAILURE;
    }

    return iim->QueryInterface(aIID, aInstancePtr);
}

////////////////////////////////////////////////////////////////////////////////
// XPCOM initialization
//
// To Control the order of initialization of these key components I am putting
// this function.
//
//  - nsServiceManager
//    - nsComponentManager
//      - nsRegistry
//
// Here are key points to remember:
//  - A global of all these need to exist. nsServiceManager is an independent object.
//    nsComponentManager uses both the globalServiceManager and its own registry.
//
//  - A static object of both the nsComponentManager and nsServiceManager
//    are in use. Hence InitXPCOM() gets triggered from both
//    NS_GetGlobale{Service/Component}Manager() calls.
//
//  - There exists no global Registry. Registry can be created from the component manager.
//

static nsresult
RegisterGenericFactory(nsIComponentManager* compMgr,
                       const nsModuleComponentInfo *info)
{
    nsresult rv;
    nsIGenericFactory* fact;
    rv = NS_NewGenericFactory(&fact, info);
    if (NS_FAILED(rv)) return rv;

    // what I want to do here is QI for a Component Registration Manager.  Since this
    // has not been invented yet, QI to the obsolete manager.  Kids, don't do this at home.
    nsCOMPtr<nsIComponentRegistrar> registrar = do_QueryInterface(compMgr, &rv);
    if (registrar)
        rv = registrar->RegisterFactory(info->mCID, 
                                        info->mDescription,
                                        info->mContractID, 
                                        fact);
    NS_RELEASE(fact);
    return rv;
}


nsComponentManagerImpl* nsComponentManagerImpl::gComponentManager = NULL;
nsIProperties     *gDirectoryService = NULL;
PRBool gXPCOMShuttingDown = PR_FALSE;

// For each class that wishes to support nsIClassInfo, add a line like this
// NS_DECL_CLASSINFO(nsMyClass)

#define COMPONENT(NAME, Ctor)                                                  \
 { NS_##NAME##_CLASSNAME, NS_##NAME##_CID, NS_##NAME##_CONTRACTID, Ctor }

#define COMPONENT_CI(NAME, Ctor, Class)                                        \
 { NS_##NAME##_CLASSNAME, NS_##NAME##_CID, NS_##NAME##_CONTRACTID, Ctor,       \
   NULL, NULL, NULL, NS_CI_INTERFACE_GETTER_NAME(Class), NULL,                 \
   &NS_CLASSINFO_NAME(Class) }

static const nsModuleComponentInfo components[] = {
// ugh
#define NS_MEMORY_CONTRACTID "@mozilla.org/xpcom/memory-service;1"
#define NS_MEMORY_CLASSNAME  "Global Memory Service"
    COMPONENT(MEMORY, nsMemoryImpl::Create),
#define NS_ERRORSERVICE_CLASSNAME NS_ERRORSERVICE_NAME
    COMPONENT(ERRORSERVICE, nsErrorService::Create),

    COMPONENT(ARENA, ArenaImpl::Create),
    COMPONENT(BYTEBUFFER, ByteBufferImpl::Create),
    COMPONENT(SCRIPTABLEINPUTSTREAM, nsScriptableInputStream::Create),

    COMPONENT(PROPERTIES, nsProperties::Create),

#define NS_PERSISTENTPROPERTIES_CID NS_IPERSISTENTPROPERTIES_CID /* sigh */
    COMPONENT(PERSISTENTPROPERTIES, nsPersistentProperties::Create),

    COMPONENT(SUPPORTSARRAY, nsSupportsArray::Create),
    COMPONENT(CONSOLESERVICE, nsConsoleServiceConstructor),
    COMPONENT(EXCEPTIONSERVICE, nsExceptionServiceConstructor),
    COMPONENT(ATOMSERVICE, nsAtomServiceConstructor),
#ifdef MOZ_TIMELINE
    COMPONENT(TIMELINESERVICE, nsTimelineServiceConstructor),
#endif
    COMPONENT(OBSERVER, nsObserver::Create),
    COMPONENT(OBSERVERSERVICE, nsObserverService::Create),
    COMPONENT(GENERICFACTORY, nsGenericFactory::Create),
    COMPONENT(EVENTQUEUESERVICE, nsEventQueueServiceImpl::Create),
    COMPONENT(EVENTQUEUE, nsEventQueueImpl::Create),
    COMPONENT(THREAD, nsThread::Create),
    COMPONENT(THREADPOOL, nsThreadPool::Create),

#define NS_XPCOMPROXY_CID NS_PROXYEVENT_MANAGER_CID
    COMPONENT(XPCOMPROXY, nsProxyObjectManager::Create),

    COMPONENT(TIMER, nsTimerImplConstructor),

#define COMPONENT_SUPPORTS(TYPE, Type)                                         \
  COMPONENT(SUPPORTS_##TYPE, nsSupports##Type##ImplConstructor)

    COMPONENT_SUPPORTS(ID, ID),
    COMPONENT_SUPPORTS(STRING, String),
    COMPONENT_SUPPORTS(WSTRING, WString),
    COMPONENT_SUPPORTS(PRBOOL, PRBool),
    COMPONENT_SUPPORTS(PRUINT8, PRUint8),
    COMPONENT_SUPPORTS(PRUINT16, PRUint16),
    COMPONENT_SUPPORTS(PRUINT32, PRUint32),
    COMPONENT_SUPPORTS(PRUINT64, PRUint64),
    COMPONENT_SUPPORTS(PRTIME, PRTime),
    COMPONENT_SUPPORTS(CHAR, Char),
    COMPONENT_SUPPORTS(PRINT16, PRInt16),
    COMPONENT_SUPPORTS(PRINT32, PRInt32),
    COMPONENT_SUPPORTS(PRINT64, PRInt64),
    COMPONENT_SUPPORTS(FLOAT, Float),
    COMPONENT_SUPPORTS(DOUBLE, Double),
    COMPONENT_SUPPORTS(VOID, Void),
    COMPONENT_SUPPORTS(INTERFACE_POINTER, InterfacePointer),

#undef COMPONENT_SUPPORTS

    COMPONENT(LOCAL_FILE, nsLocalFile::nsLocalFileConstructor),
    COMPONENT(DIRECTORY_SERVICE, nsDirectoryService::Create),
    COMPONENT(PROCESS, nsProcessConstructor),
    COMPONENT(FILESPEC, nsFileSpecImpl::Create),
    COMPONENT(DIRECTORYITERATOR, nsDirectoryIteratorImpl::Create),

    COMPONENT(STRINGINPUTSTREAM, nsStringInputStreamConstructor),

    COMPONENT(FASTLOADSERVICE, nsFastLoadService::Create),
    COMPONENT(VARIANT, nsVariantConstructor),
    COMPONENT(INTERFACEINFOMANAGER_SERVICE, nsXPTIInterfaceInfoManagerGetSingleton)
};

#undef COMPONENT

const int components_length = sizeof(components) / sizeof(components[0]);

// gMemory will be freed during shutdown.
static nsIMemory* gMemory = nsnull;
nsresult NS_COM NS_GetMemoryManager(nsIMemory* *result)
{
    nsresult rv = NS_OK;
    if (!gMemory)
    {
        rv = nsMemoryImpl::Create(nsnull,
                                  NS_GET_IID(nsIMemory),
                                  (void**)&gMemory);
    }
    NS_IF_ADDREF(*result = gMemory);
    return rv;
}

nsresult NS_COM NS_InitXPCOM(nsIServiceManager* *result,
                             nsIFile* binDirectory)
{
    return NS_InitXPCOM2(result, binDirectory, nsnull);
}


nsresult NS_COM NS_InitXPCOM2(nsIServiceManager* *result,
                              nsIFile* binDirectory,
                              nsIDirectoryServiceProvider* appFileLocationProvider)
{
    nsresult rv = NS_OK;

#ifdef NS_BUILD_REFCNT_LOGGING
    nsTraceRefcnt::Startup();
#endif

    // Establish the main thread here.
    rv = nsIThread::SetMainThread();
    if (NS_FAILED(rv)) return rv;

    // Startup the memory manager
    rv = nsMemoryImpl::Startup();
    if (NS_FAILED(rv)) return rv;

    NS_StartupLocalFile();

    StartupSpecialSystemDirectory();

    // Start the directory service so that the component manager init can use it.
    rv = nsDirectoryService::Create(nsnull,
                                    NS_GET_IID(nsIProperties),
                                    (void**)&gDirectoryService);
    if (NS_FAILED(rv))
        return rv;

    nsCOMPtr<nsIDirectoryService> dirService = do_QueryInterface(gDirectoryService, &rv);
    if (NS_FAILED(rv))
        return rv;
    rv = dirService->Init();
    if (NS_FAILED(rv))
        return rv;

    // Create the Component/Service Manager
    nsComponentManagerImpl *compMgr = NULL;

    if (nsComponentManagerImpl::gComponentManager == NULL)
    {
        compMgr = new nsComponentManagerImpl();
        if (compMgr == NULL)
            return NS_ERROR_OUT_OF_MEMORY;
        NS_ADDREF(compMgr);

        PRBool value;
        if (binDirectory)
        {
            rv = binDirectory->IsDirectory(&value);

            if (NS_SUCCEEDED(rv) && value)
                gDirectoryService->Define(NS_XPCOM_INIT_CURRENT_PROCESS_DIR, binDirectory);

            //Since people are still using the nsSpecialSystemDirectory, we should init it.
            char* path;
            binDirectory->GetPath(&path);
            nsFileSpec spec(path);
            nsMemory::Free(path);

            nsSpecialSystemDirectory::Set(nsSpecialSystemDirectory::Moz_BinDirectory, &spec);

        }
        if (appFileLocationProvider) {
            rv = dirService->RegisterProvider(appFileLocationProvider);
            if (NS_FAILED(rv)) return rv;
        }

        rv = compMgr->Init();
        if (NS_FAILED(rv))
        {
            NS_RELEASE(compMgr);
            return rv;
        }

        nsComponentManagerImpl::gComponentManager = compMgr;

        if (result) {
            nsIServiceManager *serviceManager =
                NS_STATIC_CAST(nsIServiceManager*, compMgr);

            NS_ADDREF(*result = serviceManager);
        }
    }

    nsCOMPtr<nsIMemory> memory;
    NS_GetMemoryManager(getter_AddRefs(memory));
    // dougt - these calls will be moved into a new interface when nsIComponentManager is frozen.
    rv = compMgr->RegisterService(kMemoryCID, memory);
    if (NS_FAILED(rv)) return rv;

    rv = compMgr->RegisterService(kComponentManagerCID, NS_STATIC_CAST(nsIComponentManager*, compMgr));
    if (NS_FAILED(rv)) return rv;

#ifdef GC_LEAK_DETECTOR
  rv = NS_InitLeakDetector();
    if (NS_FAILED(rv)) return rv;
#endif

    // 2. Register the global services with the component manager so that
    //    clients can create new objects.

    // Registry
    nsIFactory *registryFactory = NULL;
    rv = NS_RegistryGetFactory(&registryFactory);
    if (NS_FAILED(rv)) return rv;

    NS_DEFINE_CID(kRegistryCID, NS_REGISTRY_CID);

    rv = compMgr->RegisterFactory(kRegistryCID,
                                  NS_REGISTRY_CLASSNAME,
                                  NS_REGISTRY_CONTRACTID,
                                  registryFactory, PR_TRUE);
    NS_RELEASE(registryFactory);
    if (NS_FAILED(rv)) return rv;

    // Category Manager
    {
      nsCOMPtr<nsIFactory> categoryManagerFactory;
      if ( NS_FAILED(rv = NS_CategoryManagerGetFactory(getter_AddRefs(categoryManagerFactory))) )
        return rv;

      NS_DEFINE_CID(kCategoryManagerCID, NS_CATEGORYMANAGER_CID);

      rv = compMgr->RegisterFactory(kCategoryManagerCID,
                                    NS_CATEGORYMANAGER_CLASSNAME,
                                    NS_CATEGORYMANAGER_CONTRACTID,
                                    categoryManagerFactory,
                                    PR_TRUE);
      if ( NS_FAILED(rv) )
        return rv;
    }

    for (int i = 0; i < components_length; i++)
        RegisterGenericFactory(compMgr, &components[i]);

    // Prepopulate registry for performance
    // Ignore return value. It is ok if this fails.
    nsComponentManagerImpl::gComponentManager->PlatformPrePopulateRegistry();

    // Pay the cost at startup time of starting this singleton.
    nsIInterfaceInfoManager* iim = XPTI_GetInterfaceInfoManager();
    NS_IF_RELEASE(iim);

    return rv;
}

static nsVoidArray gExitRoutines;
static void CallExitRoutines()
{
    PRInt32 count = gExitRoutines.Count();
    for (PRInt32 i = 0; i < count; i++) {
        XPCOMExitRoutine func = (XPCOMExitRoutine) gExitRoutines.ElementAt(i);
        func();
    }
    gExitRoutines.Clear();
}

nsresult NS_COM
NS_RegisterXPCOMExitRoutine(XPCOMExitRoutine exitRoutine, PRUint32 priority)
{
    // priority are not used right now.  It will need to be implemented as more
    // classes are moved into the glue library --dougt
    PRBool okay = gExitRoutines.AppendElement((void*)exitRoutine);
    return okay ? NS_OK : NS_ERROR_FAILURE;
}

nsresult NS_COM
NS_UnregisterXPCOMExitRoutine(XPCOMExitRoutine exitRoutine)
{
    PRBool okay = gExitRoutines.RemoveElement((void*)exitRoutine);
    return okay ? NS_OK : NS_ERROR_FAILURE;
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
//   - Release Contractid Cache held by Component Manager
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
        nsCOMPtr<nsIObserverService> observerService =
                 do_GetService("@mozilla.org/observer-service;1", &rv);
        if (NS_SUCCEEDED(rv))
        {
            nsCOMPtr<nsIServiceManager> mgr;
            rv = NS_GetServiceManager(getter_AddRefs(mgr));
            if (NS_SUCCEEDED(rv))
            {
                (void) observerService->NotifyObservers(mgr,
                                                        NS_XPCOM_SHUTDOWN_OBSERVER_ID,
                                                        nsnull);
            }
        }
    }

    // grab the event queue so that we can process events one last time before exiting
    nsCOMPtr <nsIEventQueue> currentQ;
    {
        nsCOMPtr<nsIEventQueueService> eventQService =
                 do_GetService(kEventQueueServiceCID, &rv);

        if (eventQService) {
            eventQService->GetThreadEventQueue(NS_CURRENT_THREAD, getter_AddRefs(currentQ));
        }
    }
    // XPCOM is officially in shutdown mode NOW
    // Set this only after the observers have been notified as this
    // will cause servicemanager to become inaccessible.
    gXPCOMShuttingDown = PR_TRUE;

    // We may have AddRef'd for the caller of NS_InitXPCOM, so release it
    // here again:
    NS_IF_RELEASE(servMgr);

    // Shutdown global servicemanager
    nsComponentManagerImpl::gComponentManager->FreeServices();
    nsServiceManager::ShutdownGlobalServiceManager(nsnull);

    if (currentQ) {
        currentQ->ProcessPendingEvents();
        currentQ = 0;
    }

    // Release the directory service
    NS_IF_RELEASE(gDirectoryService);

    // Shutdown nsLocalFile string conversion
    NS_ShutdownLocalFile();

    // Shutdown the timer thread and all timers that might still be alive before
    // shutting down the component manager
    nsTimerImpl::Shutdown();

    // Shutdown xpcom. This will release all loaders and cause others holding
    // a refcount to the component manager to release it.
    rv = (nsComponentManagerImpl::gComponentManager)->Shutdown();
    NS_ASSERTION(NS_SUCCEEDED(rv), "Component Manager shutdown failed.");

    // Release our own singletons
    // Do this _after_ shutting down the component manager, because the
    // JS component loader will use XPConnect to call nsIModule::canUnload,
    // and that will spin up the InterfaceInfoManager again -- bad mojo
    XPTI_FreeInterfaceInfoManager();

    // Finally, release the component manager last because it unloads the
    // libraries:
    NS_RELEASE2(nsComponentManagerImpl::gComponentManager, cnt);
    NS_WARN_IF_FALSE(cnt == 0, "Component Manager being held past XPCOM shutdown.");
    nsComponentManagerImpl::gComponentManager = nsnull;

#ifdef DEBUG
    extern void _FreeAutoLockStatics();
    _FreeAutoLockStatics();
#endif

    ShutdownSpecialSystemDirectory();

    EmptyEnumeratorImpl::Shutdown();
    nsMemoryImpl::Shutdown();
    NS_IF_RELEASE(gMemory);

    nsThread::Shutdown();
    NS_PurgeAtomTable();

    CallExitRoutines();

#ifdef NS_BUILD_REFCNT_LOGGING
    nsTraceRefcnt::DumpStatistics();
    nsTraceRefcnt::ResetStatistics();
    nsTraceRefcnt::Shutdown();
#endif

#ifdef GC_LEAK_DETECTOR
    // Shutdown the Leak detector.
    NS_ShutdownLeakDetector();
#endif

    return NS_OK;
}



