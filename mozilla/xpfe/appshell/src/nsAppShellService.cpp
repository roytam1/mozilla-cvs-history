/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License.  You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are Copyright (C) 1998
 * Netscape Communications Corporation.  All Rights Reserved.
 */


#include "nsIAppShellService.h"
#include "nsISupportsArray.h"
#include "nsIComponentManager.h"
#include "nsIURL.h"
#include "nsIServiceManager.h"
#include "nsIEventQueueService.h"
#include "nsXPComFactory.h"    /* template implementation of a XPCOM factory */

#include "nsIAppShell.h"
#include "nsIWidget.h"
#include "nsIWebShellWindow.h"
#include "nsWebShellWindow.h"

#include "nsIWindowMediator.h"

#include "nsIAppShellComponent.h"
#include "nsIRegistry.h"
#include "nsIEnumerator.h"
#include "nsICmdLineService.h"
#include "nsCRT.h"
#ifdef NS_DEBUG
#include "prprf.h"    
#endif

/* For Javascript Namespace Access */
#include "nsDOMCID.h"
#include "nsIScriptNameSetRegistry.h"
#include "nsAppShellNameSet.h"

#include "nsWidgetsCID.h"
#include "nsIStreamObserver.h"

#ifdef MOZ_FULLCIRCLE
#include "fullsoft.h"
#endif

#ifdef XP_PC
#include "nsIPICS.h"
static NS_DEFINE_IID(kIPICSIID, NS_IPICS_IID);
static NS_DEFINE_IID(kPICSCID, NS_PICS_CID);
#endif

#include "nsMetaCharsetCID.h"
#include "nsIMetaCharsetService.h"

/* Define Class IDs */
static NS_DEFINE_IID(kAppShellCID,          NS_APPSHELL_CID);
static NS_DEFINE_IID(kEventQueueServiceCID, NS_EVENTQUEUESERVICE_CID);
static NS_DEFINE_IID(kCScriptNameSetRegistryCID, NS_SCRIPT_NAMESET_REGISTRY_CID);
static NS_DEFINE_CID(kWindowMediatorCID, NS_WINDOWMEDIATOR_CID);
static NS_DEFINE_IID(kMetaCharsetCID, NS_META_CHARSET_CID);


/* Define Interface IDs */

static NS_DEFINE_IID(kIFactoryIID,           NS_IFACTORY_IID);
static NS_DEFINE_IID(kIEventQueueServiceIID, NS_IEVENTQUEUESERVICE_IID);
static NS_DEFINE_IID(kIAppShellServiceIID,   NS_IAPPSHELL_SERVICE_IID);
static NS_DEFINE_IID(kIAppShellIID,          NS_IAPPSHELL_IID);
static NS_DEFINE_IID(kIWebShellWindowIID,    NS_IWEBSHELL_WINDOW_IID);
static NS_DEFINE_IID(kIScriptNameSetRegistryIID, NS_ISCRIPTNAMESETREGISTRY_IID);
static NS_DEFINE_IID(kIWindowMediatorIID,NS_IWINDOWMEDIATOR_IID);
static NS_DEFINE_IID(kIMetaCharsetServiceIID, NS_IMETA_CHARSET_SERVICE_IID);


class nsAppShellService : public nsIAppShellService
{
public:
  nsAppShellService(void);

  NS_DECL_ISUPPORTS

  NS_IMETHOD Initialize(nsICmdLineService*aCmdLineService);
  NS_IMETHOD Run(void);
  NS_IMETHOD Shutdown(void);

  NS_IMETHOD CreateTopLevelWindow(nsIWebShellWindow * aParent,
                                  nsIURL* aUrl, 
                                  PRBool showWindow,
                                  nsIWebShellWindow*& aResult, nsIStreamObserver* anObserver,
                                  nsIXULWindowCallbacks *aCallbacks,
                                  PRInt32 aInitialWidth, PRInt32 aInitialHeight);
  NS_IMETHOD CreateDialogWindow(  nsIWebShellWindow * aParent,
                                  nsIURL* aUrl, 
                                  PRBool showWindow,
                                  nsIWebShellWindow*& aResult, nsIStreamObserver* anObserver,
                                  nsIXULWindowCallbacks *aCallbacks,
                                  PRInt32 aInitialWidth, PRInt32 aInitialHeight);
  NS_IMETHOD RunModalDialog(      nsIWebShellWindow * aParent,
                                  nsIURL* aUrl, 
                                  nsIWebShellWindow*& aResult, nsIStreamObserver* anObserver,
                                  nsIXULWindowCallbacks *aCallbacks,
                                  PRInt32 aInitialWidth, PRInt32 aInitialHeight);
  NS_IMETHOD CloseTopLevelWindow(nsIWebShellWindow* aWindow);
  NS_IMETHOD RegisterTopLevelWindow(nsIWebShellWindow* aWindow);
  NS_IMETHOD UnregisterTopLevelWindow(nsIWebShellWindow* aWindow);


protected:
  virtual ~nsAppShellService();

  void InitializeComponent( const nsCID &aComponentCID );
  void ShutdownComponent( const nsCID &aComponentCID );
  void EnumerateComponents( void (nsAppShellService::*function)(const nsCID&) );

  nsIAppShell* mAppShell;
  nsISupportsArray* mWindowList;
  nsICmdLineService* mCmdLineService;
};


nsAppShellService::nsAppShellService()
{
  NS_INIT_REFCNT();

  mAppShell     = nsnull;
  mWindowList   = nsnull;
  mCmdLineService = nsnull;
}

nsAppShellService::~nsAppShellService()
{
  NS_IF_RELEASE(mAppShell);
  NS_IF_RELEASE(mWindowList);
  NS_IF_RELEASE(mCmdLineService);
}


/*
 * Implement the nsISupports methods...
 */
NS_IMPL_ISUPPORTS(nsAppShellService, kIAppShellServiceIID);


NS_IMETHODIMP
nsAppShellService::Initialize( nsICmdLineService *aCmdLineService )
{
  nsresult rv;
#ifdef XP_PC
  nsIPICS *pics = NULL;
#endif
  
#ifdef MOZ_FULLCIRCLE
  FCInitialize();
#endif

  // Remember cmd line service.
  mCmdLineService = aCmdLineService;
  NS_IF_ADDREF( mCmdLineService );

  // Create the Event Queue for the UI thread...
  nsIEventQueueService* eventQService;
  rv = nsServiceManager::GetService(kEventQueueServiceCID,
                                    kIEventQueueServiceIID,
                                    (nsISupports **)&eventQService);
  if (NS_OK == rv) {
    // XXX: What if this fails?
    rv = eventQService->CreateThreadEventQueue();
  }

  // Register the nsAppShellNameSet with the global nameset registry...
  nsIScriptNameSetRegistry *registry;
  rv = nsServiceManager::GetService(kCScriptNameSetRegistryCID,
                                    kIScriptNameSetRegistryIID,
                                    (nsISupports **)&registry);
  if (NS_FAILED(rv)) {
    goto done;
  }
  
  nsAppShellNameSet* nameSet;
  nameSet = new nsAppShellNameSet();
  registry->AddExternalNameSet(nameSet);
  /* XXX: do we need to release this service?  When we do, it get deleted,and our name is lost. */

  // Create the toplevel window list...
  rv = NS_NewISupportsArray(&mWindowList);
  if (NS_FAILED(rv)) {
    goto done;
  }

#ifdef XP_PC
  rv = nsComponentManager::CreateInstance(kPICSCID,
							   NULL,
							   kIPICSIID,
							  (void **) &pics);
 
#endif

  nsIMetaCharsetService* metacharset;
  rv = nsServiceManager::GetService(kMetaCharsetCID,
                                    kIMetaCharsetServiceIID,
                                     (nsISupports **) &metacharset);
   if(NS_FAILED(rv)) {
      goto done;
   }
   rv = metacharset->Start();
   if(NS_FAILED(rv)) {
      goto done;
   }
   rv = nsServiceManager::ReleaseService(kMetaCharsetCID, metacharset);

  // Create widget application shell
  rv = nsComponentManager::CreateInstance(kAppShellCID, nsnull, kIAppShellIID,
                                    (void**)&mAppShell);
  if (NS_FAILED(rv)) {
    goto done;
  }

  rv = mAppShell->Create(0, nsnull);

  if (NS_FAILED(rv)) {
      goto done;
  }

  // Initialize each registered component.
  EnumerateComponents( &nsAppShellService::InitializeComponent );

done:
  return rv;
}

// Apply function (Initialize/Shutdown) to each app shell component.
void
nsAppShellService::EnumerateComponents( void (nsAppShellService::*function)( const nsCID& ) ) {
    nsresult rv;
    nsIRegistry *registry = 0;
    nsIRegistry::Key key;
    nsIEnumerator *components = 0;
    const char *failed = "GetService";
    if ( NS_SUCCEEDED( ( rv = nsServiceManager::GetService( NS_REGISTRY_PROGID,
                                                          nsIRegistry::GetIID(),
                                                          (nsISupports**)&registry ) ) )
         &&
         ( failed = "Open" )
         &&
         NS_SUCCEEDED( ( rv = registry->OpenWellKnownRegistry(nsIRegistry::ApplicationComponentRegistry) ) )
         &&
         ( failed = "GetSubtree" )
         &&
         NS_SUCCEEDED( ( rv = registry->GetSubtree( nsIRegistry::Common,
                                                    NS_IAPPSHELLCOMPONENT_KEY,
                                                    &key ) ) )
         &&
         ( failed = "EnumerateSubtrees" )
         &&
         NS_SUCCEEDED( ( rv = registry->EnumerateSubtrees( key,
                                                           &components ) ) )
         &&
         ( failed = "First" )
         &&
         NS_SUCCEEDED( ( rv = components->First() ) ) ) {
        // Enumerate all subtrees
        while ( NS_SUCCEEDED( rv ) && !components->IsDone() ) {
            nsISupports *base;
            
            rv = components->CurrentItem( &base );
            if ( NS_SUCCEEDED( rv ) ) {
                // Get specific interface.
                nsIRegistryNode *node;
                nsIID nodeIID = NS_IREGISTRYNODE_IID;
                rv = base->QueryInterface( nodeIID, (void**)&node );
                // Test that result.
                if ( NS_SUCCEEDED( rv ) ) {
                    // Get node name.
                    char *name;
                    rv = node->GetName( &name );
                    if ( NS_SUCCEEDED( rv ) ) {
                        // If this is a CID of a component; apply function to it.
                        nsCID cid;
                        if ( cid.Parse( name ) ) {
                            (this->*function)( cid );
                        } else {
                            // Not a valid CID, ignore it.
                        }
                    } else {
                        // Unable to get subkey name, ignore it.
                    }
                    // Release the node.
                    nsCRT::free(name);
                    NS_RELEASE( node );
                } else {
                    // Unable to convert item to registry node, ignore it.
                }

                // Release the current (generic) item.
                NS_RELEASE( base );
            } else {
                // Unable to get current item, ignore it.
            }

            // Go on to next component, if this fails, we quit.
            rv = components->Next();
        }
    } else {
        // Unable to set up for subkey enumeration.
        #ifdef NS_DEBUG
            printf( "Unable to enumerator app shell components, %s rv=0x%08X\n",
                    failed, (int)rv );
        #endif
    }

    // Clean up.
    if ( registry ) {
        // Registry was accessed, close it.
        registry->Close();

        // Release enumerator (if necessary).
        NS_IF_RELEASE( components );

        // Release nsIRegistry service.
        nsServiceManager::ReleaseService( NS_REGISTRY_PROGID, registry );
    }

    return;
}

void
nsAppShellService::InitializeComponent( const nsCID &aComponentCID ) {
    // Attempt to create instance of the component.
    nsIAppShellComponent *component;
    nsresult rv = nsComponentManager::CreateInstance( aComponentCID,
                                                      0,
                                                      nsIAppShellComponent::GetIID(),
                                                      (void**)&component );
    if ( NS_SUCCEEDED( rv ) ) {
        // Then tell it to initialize (it may RegisterService itself).
        rv = component->Initialize( this, mCmdLineService );
        #ifdef NS_DEBUG
            char *name = aComponentCID.ToString();
            printf( "Initialized app shell component %s, rv=0x%08X\n",
                    name, (int)rv );
            delete [] name;
        #endif
        // Release it (will live on if it registered itself as service).
        component->Release();
    } else {
        // Error creating component.
        #ifdef NS_DEBUG
            char *name = aComponentCID.ToString();
            printf( "Error creating app shell component %s, rv=0x%08X\n",
                    name, (int)rv );
            delete [] name;
        #endif
    }

    return;
}

void
nsAppShellService::ShutdownComponent( const nsCID &aComponentCID ) {
    // Attempt to create instance of the component (must be a service).
    nsIAppShellComponent *component;
    nsresult rv = nsServiceManager::GetService( aComponentCID,
                                                nsIAppShellComponent::GetIID(),
                                                (nsISupports**)&component );
    if ( NS_SUCCEEDED( rv ) ) {
        // Instance accessed, tell it to shutdown.
        rv = component->Shutdown();
        #ifdef NS_DEBUG
            char *name = aComponentCID.ToString();
            printf( "Shut down app shell component %s, rv=0x%08X\n",
                    name, (int)rv );
            delete [] name;
        #endif
        // Release the service.
        nsServiceManager::ReleaseService( aComponentCID, component );
    } else {
        // Error getting component service (perhaps due to that component not being
        // a service).
        #ifdef NS_DEBUG
            char *name = aComponentCID.ToString();
            printf( "Unable to shut down app shell component %s, rv=0x%08X\n",
                    name, (int)rv );
            delete [] name;
        #endif
    }

    return;
}

NS_IMETHODIMP
nsAppShellService::Run(void)
{
  return mAppShell->Run();
}

NS_IMETHODIMP
nsAppShellService::Shutdown(void)
{

#if 1
  // Shutdown all components.
  EnumerateComponents( &nsAppShellService::ShutdownComponent );

  mAppShell->Exit();
#else
  while (mWindowList->Count() > 0) {
    nsISupports * winSupports = mWindowList->ElementAt(0);
    nsCOMPtr<nsIWidget> window(do_QueryInterface(winSupports));
    if (window) {
      mWindowList->RemoveElementAt(0);
      CloseTopLevelWindow(window);
    } else {
      nsCOMPtr<nsIWebShellWindow> webShellWin(do_QueryInterface(winSupports));
      if (webShellWin) {
        nsIWidget * win;
        webShellWin->GetWidget(win);
        CloseTopLevelWindow(win);
      }
      //nsCOMPtr<nsIWebShellContainer> wsc(do_QueryInterface(winSupports));
      //if (wsc) {
      //
      //}
      break;
    }
  }
#endif
  return NS_OK;
}

/*
 * Create a new top level window and display the given URL within it...
 *
 * @param aParent - parent for the window to be created (generally null;
 *                  included for compatibility with dialogs).
 *                  (currently unused).
 * @param aURL - location of XUL window contents description
 * @param aShowWindow - whether or not to show the window initially.
 * @param anObserver - a stream observer to give to the new window
 * @param aConstructionCallbacks - methods which will be called during
 *                                 window construction. can be null.
 * @param aInitialWidth - width of window, in pixels (currently unused)
 * @param aInitialHeight - height of window, in pixels (currently unused)
 */
NS_IMETHODIMP
nsAppShellService::CreateTopLevelWindow(nsIWebShellWindow *aParent,
                                        nsIURL* aUrl, PRBool showWindow,
                                        nsIWebShellWindow*& aResult, nsIStreamObserver* anObserver,
                                        nsIXULWindowCallbacks *aCallbacks,
                                        PRInt32 aInitialWidth, PRInt32 aInitialHeight)
{
  nsresult rv;
  nsWebShellWindow* window;

  aResult = nsnull;
  window = new nsWebShellWindow();
  if (nsnull == window) {
    rv = NS_ERROR_OUT_OF_MEMORY;
  } else {
    // temporarily disabling parentage because non-Windows platforms
    // seem to be interpreting it in unexpected ways.
    rv = window->Initialize((nsIWebShellWindow *) nsnull, mAppShell, aUrl,
                            anObserver, aCallbacks,
                            aInitialWidth, aInitialHeight);
    if (NS_SUCCEEDED(rv)) {
      rv = window->QueryInterface(kIWebShellWindowIID, (void **) &aResult);
      RegisterTopLevelWindow(window);
      if (showWindow)
				window->Show(PR_TRUE);
    }
  }

  return rv;
}


NS_IMETHODIMP
nsAppShellService::CloseTopLevelWindow(nsIWebShellWindow* aWindow)
{
  return aWindow->Close();
}

/*
 * Like CreateTopLevelWindow, but with dialog window borders.  This
 * method is necessary because of the current misfortune that the window
 * is created before its XUL description has been parsed, so the description
 * can't affect attributes like window type.
 */
NS_IMETHODIMP
nsAppShellService::CreateDialogWindow(nsIWebShellWindow * aParent,
                                      nsIURL* aUrl, PRBool showWindow,
                                      nsIWebShellWindow*& aResult, nsIStreamObserver* anObserver,
                                      nsIXULWindowCallbacks *aCallbacks,
                                      PRInt32 aInitialWidth, PRInt32 aInitialHeight)
{
  nsresult rv;
  nsWebShellWindow* window;

  aResult = nsnull;
  window = new nsWebShellWindow();
  if (nsnull == window) {
    rv = NS_ERROR_OUT_OF_MEMORY;
  } else {
    // temporarily disabling parentage because non-Windows platforms
    // seem to be interpreting it in unexpected ways.
    rv = window->Initialize((nsIWebShellWindow *) nsnull, mAppShell, aUrl,
                            anObserver, aCallbacks,
                            aInitialWidth, aInitialHeight);
    if (NS_SUCCEEDED(rv)) {
      rv = window->QueryInterface(kIWebShellWindowIID, (void **) &aResult);
      RegisterTopLevelWindow(window);
      if (showWindow)
				window->Show(PR_TRUE);
    }
  }

  return rv;
}


/* CreateDialogWindow, run it modally, and destroy it.  To make initial control
   settings or get information out of the dialog before dismissal, use
   event handlers.  This wrapper method is desirable because of the
   complications creeping in to the modal window story: there's a lot of setup.
   See the code..

   Note that the window created is returned in aResult.  By the time this function
   exits, that window has been partially destroyed.  We return it anyway, in the
   hopes that it may be queried for results, somehow.  This may be a mistake.
   It is returned addrefed (by the QueryInterface to nsIWebShellWindow in
   CreateDialogWindow).
*/
NS_IMETHODIMP
nsAppShellService::RunModalDialog(
                      nsIWebShellWindow *aParent, nsIURL* aUrl,
                      nsIWebShellWindow*& aResult, nsIStreamObserver *anObserver,
                      nsIXULWindowCallbacks *aCallbacks,
                      PRInt32 aInitialWidth, PRInt32 aInitialHeight)
{

  nsresult             rv;


#ifdef XP_PC // XXX: Won't work with any other platforms yet.
  // First push a nested event queue for event processing from netlib
  // onto our UI thread queue stack.
  // nsCOMPtr<nsIEventQueue> innerQueue;
  NS_WITH_SERVICE(nsIEventQueueService, eQueueService, kEventQueueServiceCID, &rv);
  if (NS_FAILED(rv)) {
    NS_ERROR("RunModalDialog unable to obtain eventqueue service.");
    return rv;
  }
  eQueueService->PushThreadEventQueue();
#endif

  rv = CreateDialogWindow(aParent, aUrl, PR_TRUE, aResult, anObserver, aCallbacks,
            aInitialWidth, aInitialHeight);

  if (NS_SUCCEEDED(rv)) {
    nsCOMPtr<nsIWidget> parentWindowWidgetThing;
    nsresult gotParent;
    gotParent = aParent ? aParent->GetWidget(*getter_AddRefs(parentWindowWidgetThing)) :
                          NS_ERROR_FAILURE;
    // Windows OS wants the parent disabled for modality
    if (NS_SUCCEEDED(gotParent))
      parentWindowWidgetThing->Enable(PR_FALSE);
    aResult->ShowModal();
    if (NS_SUCCEEDED(gotParent))
      parentWindowWidgetThing->Enable(PR_TRUE);
  }

	// Release the event queue 
#ifdef XP_PC // XXX Won't work on other platforms yet
  eQueueService->PopThreadEventQueue();
#endif

  return rv;
}


/*
 * Register a new top level window (created elsewhere)
 */
static NS_DEFINE_IID(kIWebShellContainerIID,  NS_IWEB_SHELL_CONTAINER_IID);
NS_IMETHODIMP
nsAppShellService::RegisterTopLevelWindow(nsIWebShellWindow* aWindow)
{
  nsresult rv;

  nsIWebShellContainer* wsc;
  rv = aWindow->QueryInterface(kIWebShellContainerIID, (void **) &wsc);
  if (NS_SUCCEEDED(rv)) {
    mWindowList->AppendElement(wsc);
    NS_RELEASE(wsc);
    
    nsIWindowMediator* service;
		if (NS_SUCCEEDED(nsServiceManager::GetService(kWindowMediatorCID, kIWindowMediatorIID, (nsISupports**) &service ) ) )
		{
			service->RegisterWindow( aWindow);
			nsServiceManager::ReleaseService(kWindowMediatorCID, service);
		}
  }
  return rv;
}


NS_IMETHODIMP
nsAppShellService::UnregisterTopLevelWindow(nsIWebShellWindow* aWindow)
{
  nsIWindowMediator* service;
	if (NS_SUCCEEDED(nsServiceManager::GetService(kWindowMediatorCID, kIWindowMediatorIID, (nsISupports**) &service ) ) )
	{
		service->RegisterWindow( aWindow);
		nsServiceManager::ReleaseService(kWindowMediatorCID, service);
	}
  
  nsresult rv;

  nsIWebShellContainer* wsc;
  rv = aWindow->QueryInterface(kIWebShellContainerIID, (void **) &wsc);
  if (NS_SUCCEEDED(rv)) {
    mWindowList->RemoveElement(wsc);
    NS_RELEASE(wsc);
  }
  PRUint32 cnt;
  rv = mWindowList->Count(&cnt);
  if (NS_FAILED(rv)) return rv;
  if (0 == cnt)
    mAppShell->Exit();
  return rv;
}


NS_EXPORT nsresult NS_NewAppShellService(nsIAppShellService** aResult)
{
  if (nsnull == aResult) {
    return NS_ERROR_NULL_POINTER;
  }
	
  *aResult = new nsAppShellService();
  if (nsnull == *aResult) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  NS_ADDREF(*aResult);
  return NS_OK;
}

//----------------------------------------------------------------------

// Entry point to create nsAppShellService factory instances...
NS_DEF_FACTORY(AppShellService,nsAppShellService)

nsresult NS_NewAppShellServiceFactory(nsIFactory** aResult)
{
  nsresult rv = NS_OK;
  nsIFactory* inst = new nsAppShellServiceFactory;
  if (nsnull == inst) {
    rv = NS_ERROR_OUT_OF_MEMORY;
  }
  else {
    NS_ADDREF(inst);
  }
  *aResult = inst;
  return rv;
}
