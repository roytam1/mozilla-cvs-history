/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
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
#ifndef __nsIAppShellComponentImpl_h
#define __nsIAppShellComponentImpl_h

#include "nsIAppShellComponent.h"
#include "nsIAppShellService.h"
#include "nsAppShellCIDs.h"
#include "nsIComponentManager.h"
#include "nsIServiceManager.h"
#include "nsICmdLineService.h"
#include "nsISupports.h"
#include "nsIFactory.h"
#include "nsIModule.h"
#include "nsIGenericFactory.h"
#include "nsIRegistry.h"
#include "pratom.h"
#include "nsCOMPtr.h"
#include "prprf.h"

/*------------------------------------------------------------------------------
| This file contains macros to assist in the implementation of application     |
| shell components.  The macros of interest (that you would typically use      |
| in the implementation of your component) are:                                |
|                                                                              |
| NS_IMPL_IAPPSHELLCOMPONENT( className, interfaceName, progId ):              |
|                                                                              |
|   This macro will generate all the boilderplate app shell component          |
|   implementation details.                                                    |
|       "className" is the name of your implementation                         |
|           class.                                                             |
|       "interfaceName" is the name of the interface that the class            |
|           implements (sorry, currently only one is supported).               |
|       "progId" is the progId corresponding to the interface your             |
|           class implements.                                                  |
------------------------------------------------------------------------------*/

#if defined( NS_DEBUG ) && !defined( XP_MAC )
    #define DEBUG_PRINTF PR_fprintf
#else
    #define DEBUG_PRINTF (void)
#endif

// Declare nsInstanceCounter class.
class nsInstanceCounter {
public:
    // ctor increments (shared library) global instance counter.
    nsInstanceCounter() {
        PR_AtomicIncrement( &mInstanceCount );
    }
    // dtor decrements it.
    ~nsInstanceCounter() {
        PR_AtomicDecrement( &mInstanceCount );
    }
    // Tests whether shared library can be unloaded.
    static PRBool CanUnload() {
        return mInstanceCount == 0 && mLockCount == 0;
    }
    // Lock/unlock (a factory).
    static nsresult LockFactory( PRBool aLock ) {
        if (aLock)
            PR_AtomicIncrement( &mLockCount ); 
        else
            PR_AtomicDecrement( &mLockCount );
        return NS_OK;
    }
private:
    static PRInt32 mInstanceCount, mLockCount;
}; // nsInstanceCounter

#define NS_DEFINE_MODULE_INSTANCE_COUNTER() \
PRInt32 nsInstanceCounter::mInstanceCount = 0; \
PRInt32 nsInstanceCounter::mLockCount = 0; \
\
// Declare component-global class.
class nsAppShellComponentImpl {
public:
    // Override this and return PR_FALSE if your component
    // should not be registered with the Service Manager
    // when Initialize-d.
    virtual PRBool Is_Service() { return PR_TRUE; }
    // Override this to perform initialization for your component.
    NS_IMETHOD DoInitialization() { return NS_OK; }
    static nsIAppShellService *GetAppShell() {
        if ( !mAppShell ) {
            nsCID cid = NS_APPSHELL_SERVICE_CID;
            nsServiceManager::GetService( cid,
                                         nsIAppShellService::GetIID(),
                                         (nsISupports**)&mAppShell );
        }
        return mAppShell;
    }
    static nsIAppShellService *mAppShell;
    static nsICmdLineService  *mCmdLine;
    #ifdef NS_DEBUG
    nsAppShellComponentImpl();
    virtual ~nsAppShellComponentImpl();
    #endif
}; // nsAppShellComponent

#define NS_DEFINE_COMPONENT_GLOBALS() \
nsIAppShellService *nsAppShellComponentImpl::mAppShell   = 0; \
nsICmdLineService  *nsAppShellComponentImpl::mCmdLine    = 0;

// Macros to define ctor/dtor for implementation class.
// These differ in debug vs. non-debug situations.
#ifdef NS_DEBUG
#define NS_IMPL_IAPPSHELLCOMPONENTIMPL_CTORDTOR(className) \
nsAppShellComponentImpl::nsAppShellComponentImpl() { \
    DEBUG_PRINTF( PR_STDOUT, #className " component created\n" ); \
} \
nsAppShellComponentImpl::~nsAppShellComponentImpl() { \
    DEBUG_PRINTF( PR_STDOUT, #className " component destroyed\n" ); \
}
#else
#define NS_IMPL_IAPPSHELLCOMPONENTIMPL_CTORDTOR(className)
#endif

#define NS_IMPL_IAPPSHELLCOMPONENT( className, interfaceName, progId, autoInit ) \
/* Define instance counter implementation stuff. */\
NS_DEFINE_MODULE_INSTANCE_COUNTER() \
/* Define component globals. */\
NS_DEFINE_COMPONENT_GLOBALS() \
/* Component's implementation of Initialize. */\
NS_IMETHODIMP \
className::Initialize( nsIAppShellService *anAppShell, \
                       nsICmdLineService  *aCmdLineService ) { \
    nsresult rv = NS_OK; \
    mAppShell = anAppShell; \
    mCmdLine  = aCmdLineService; \
    if ( Is_Service() ) { \
        rv = nsServiceManager::RegisterService( progId, this ); \
    } \
    if ( NS_SUCCEEDED( rv ) ) { \
        rv = DoInitialization(); \
    } \
    return rv; \
} \
/* Component's implementation of Shutdown. */\
NS_IMETHODIMP \
className::Shutdown() { \
    nsresult rv = NS_OK; \
    if ( Is_Service() ) { \
        rv = nsServiceManager::UnregisterService( progId ); \
    } \
    return rv; \
} \
/* nsISupports Implementation for the class */\
NS_IMPL_ADDREF( className );  \
NS_IMPL_RELEASE( className ); \
/* QueryInterface implementation for this class. */\
NS_IMETHODIMP \
className::QueryInterface( REFNSIID anIID, void **anInstancePtr ) { \
    nsresult rv = NS_OK; \
    /* Check for place to return result. */\
    if ( !anInstancePtr ) { \
        rv = NS_ERROR_NULL_POINTER; \
    } else { \
        /* Initialize result. */\
        *anInstancePtr = 0; \
        /* Check for IIDs we support and cast this appropriately. */\
        if ( 0 ) { \
        } else if ( anIID.Equals( interfaceName::GetIID() ) ) { \
            *anInstancePtr = (void*) this; \
            NS_ADDREF_THIS(); \
        } else if ( anIID.Equals( nsIAppShellComponent::GetIID() ) ) { \
            *anInstancePtr = (void*) ( (nsIAppShellComponent*)this ); \
            NS_ADDREF_THIS(); \
        } else if ( anIID.Equals( nsCOMTypeInfo<nsISupports>::GetIID() ) ) { \
            *anInstancePtr = (void*) ( (nsISupports*)this ); \
            NS_ADDREF_THIS(); \
        } else { \
            /* Not an interface we support. */\
            rv = NS_NOINTERFACE; \
        } \
    } \
    return rv; \
} \
/* Factory class */\
struct className##Factory : public nsIFactory { \
    /* ctor/dtor */\
    className##Factory() { \
        NS_INIT_REFCNT(); \
    } \
    virtual ~className##Factory() { \
    } \
    /* This class implements the nsISupports interface functions. */\
	NS_DECL_ISUPPORTS \
    /* nsIFactory methods */\
    NS_IMETHOD CreateInstance( nsISupports *aOuter, \
                               const nsIID &aIID, \
                               void **aResult ); \
    NS_IMETHOD LockFactory( PRBool aLock ); \
private: \
    nsInstanceCounter instanceCounter; \
}; \
/* nsISupports interface implementation for the factory. */\
NS_IMPL_ADDREF( className##Factory ) \
NS_IMPL_RELEASE( className##Factory ) \
NS_IMETHODIMP \
className##Factory::QueryInterface( const nsIID &anIID, void **aResult ) { \
    nsresult rv = NS_OK; \
    if ( aResult ) { \
        *aResult = 0; \
        if ( 0 ) { \
        } else if ( anIID.Equals( nsIFactory::GetIID() ) ) { \
            *aResult = (void*) (nsIFactory*)this; \
            NS_ADDREF_THIS(); \
        } else if ( anIID.Equals( nsCOMTypeInfo<nsISupports>::GetIID() ) ) { \
            *aResult = (void*) (nsISupports*)this; \
            NS_ADDREF_THIS(); \
        } else { \
            rv = NS_ERROR_NO_INTERFACE; \
        } \
    } else { \
        rv = NS_ERROR_NULL_POINTER; \
    } \
    return rv; \
} \
/* Factory's CreateInstance implementation */\
NS_IMETHODIMP \
className##Factory::CreateInstance( nsISupports *anOuter, \
                                    const nsIID &anIID, \
                                    void*       *aResult ) { \
    nsresult rv = NS_OK; \
    if ( aResult ) { \
        /* Allocate new find component object. */\
        className *component = new className(); \
        if ( component ) { \
            /* Allocated OK, do query interface to get proper */\
            /* pointer and increment refcount.                */\
            rv = component->QueryInterface( anIID, aResult ); \
            if ( NS_FAILED( rv ) ) { \
                /* refcount still at zero, delete it here. */\
                delete component; \
            } \
        } else { \
            rv = NS_ERROR_OUT_OF_MEMORY; \
        } \
    } else { \
        rv = NS_ERROR_NULL_POINTER; \
    } \
    return rv; \
} \
/* Factory's LockFactory implementation */\
NS_IMETHODIMP \
className##Factory::LockFactory(PRBool aLock) { \
      return nsInstanceCounter::LockFactory( aLock ); \
} \
class className##Module : public nsIModule { \
public: \
    className##Module() { \
        NS_INIT_REFCNT(); \
    } \
    virtual ~className##Module() { \
    } \
    NS_DECL_ISUPPORTS \
    NS_DECL_NSIMODULE \
}; \
NS_IMPL_ISUPPORTS1(className##Module, nsIModule) \
/* NSRegisterSelf implementation */\
NS_IMETHODIMP \
className##Module::RegisterSelf(nsIComponentManager *compMgr, \
                                nsIFileSpec *path, \
                                const char *registryLocation, \
                                const char *componentType) \
{ \
    nsresult rv = NS_OK; \
    /* WARNING: Dont remember service manager. */ \
    /* Get the component manager service. */ \
 \
    if (NS_FAILED(rv)) return rv; \
    /* Register our component. */ \
    rv = compMgr->RegisterComponentSpec( className::GetCID(), #className, \
                                          progId, path, PR_TRUE, PR_TRUE ); \
    if ( NS_SUCCEEDED( rv ) ) { \
        DEBUG_PRINTF( PR_STDOUT, #className " registration successful\n" ); \
        if ( autoInit ) { \
            /* Add to appshell component list. */ \
            nsIRegistry *registry; \
            rv = nsServiceManager::GetService( NS_REGISTRY_PROGID, \
                                      nsIRegistry::GetIID(), \
                                      (nsISupports**)&registry ); \
            if ( NS_SUCCEEDED( rv ) ) { \
                registry->OpenWellKnownRegistry(nsIRegistry::ApplicationComponentRegistry); \
                char buffer[256]; \
                char *cidString = className::GetCID().ToString(); \
                PR_snprintf( buffer, sizeof buffer, "%s/%s", \
                             NS_IAPPSHELLCOMPONENT_KEY, \
                             cidString ? cidString : "unknown" ); \
                delete [] cidString; \
                nsRegistryKey key; \
                rv = registry->AddSubtree( nsIRegistry::Common, buffer, &key ); \
                if ( NS_SUCCEEDED( rv ) ) { \
                    DEBUG_PRINTF( PR_STDOUT, #className " added to appshell component list\n" ); \
                } else { \
                    DEBUG_PRINTF( PR_STDOUT, #className " not added to appshell component list, rv=0x%X\n", (int)rv ); \
                } \
            } else { \
                DEBUG_PRINTF( PR_STDOUT, #className " not added to appshell component list, rv=0x%X\n", (int)rv ); \
            } \
        } \
    } else { \
        DEBUG_PRINTF( PR_STDOUT, #className " registration failed, RegisterComponent rv=0x%X\n", (int)rv ); \
    } \
 \
    return rv; \
} \
/* UnregisterSelf implementation */ \
NS_IMETHODIMP \
className##Module::UnregisterSelf( nsIComponentManager *compMgr, \
                                   nsIFileSpec* path, \
                                   const char *registryLocation) { \
    nsresult rv = NS_OK; \
    if (NS_FAILED(rv)) \
    { \
        DEBUG_PRINTF( PR_STDOUT, #className " registration failed, GetService rv=0x%X\n", (int)rv ); \
        return rv; \
    } \
 \
    /* Unregister our component. */ \
    rv = compMgr->UnregisterComponentSpec( className::GetCID(), path ); \
    if ( NS_SUCCEEDED( rv ) ) { \
        DEBUG_PRINTF( PR_STDOUT, #className " unregistration successful\n" ); \
    } else { \
        DEBUG_PRINTF( PR_STDOUT, #className " unregistration failed, UnregisterComponent rv=0x%X\n", (int)rv ); \
    } \
 \
    return rv; \
} \
/* GetFactory implementation */\
NS_IMETHODIMP \
className##Module::GetClassObject( nsIComponentManager *compMgr, \
              const nsCID &aClass, \
              const nsIID &aIID, \
              void **aFactory ) { \
    nsresult rv = NS_OK; \
    if ( NS_SUCCEEDED( rv ) ) { \
        if ( aFactory ) { \
            className##Factory *factory = new className##Factory(); \
            if ( factory ) { \
                NS_ADDREF(factory); \
                rv = factory->QueryInterface( aIID, (void**)aFactory ); \
                NS_RELEASE(factory); \
            } else { \
                rv = NS_ERROR_OUT_OF_MEMORY; \
            } \
        } else { \
            rv = NS_ERROR_NULL_POINTER; \
        } \
    } \
    return rv; \
} \
NS_IMETHODIMP \
className##Module::CanUnload( nsIComponentManager*, PRBool* canUnload) { \
      if (!canUnload) return NS_ERROR_NULL_POINTER; \
      *canUnload = nsInstanceCounter::CanUnload(); \
} \
NS_IMPL_IAPPSHELLCOMPONENTIMPL_CTORDTOR( className ) \
NS_IMPL_NSGETMODULE(className##Module)
#endif
