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
#include "nsIServiceManager.h"
#include "nsIURL.h"
#include "nsIWidget.h"
#include "plevent.h"

#include "nsIAppShell.h"
#include "nsICmdLineService.h"
#include "nsIAppShellService.h"
#include "nsAppShellCIDs.h"
#include "prprf.h"

#if defined(XP_MAC)
#include "macstdlibextras.h"
  // Set up the toolbox and (if DEBUG) the console.  Do this in a static initializer,
  // to make it as unlikely as possible that somebody calls printf() before we get initialized.
static struct MacInitializer { MacInitializer() { InitializeMacToolbox(); } } gInitializer;
#endif // XP_MAC

/* Define Class IDs */
static NS_DEFINE_IID(kAppShellServiceCID, NS_APPSHELL_SERVICE_CID);
static NS_DEFINE_IID(kCmdLineServiceCID, NS_COMMANDLINE_SERVICE_CID);

/* Define Interface IDs */
static NS_DEFINE_IID(kIAppShellServiceIID, NS_IAPPSHELL_SERVICE_IID);
static NS_DEFINE_IID(kICmdLineServiceIID, NS_ICOMMANDLINE_SERVICE_IID);

/*********************************************
 AppCores
*********************************************/

//#if defined(XP_PC) || defined(XP_MAC)
#include "nsAppCoresCIDs.h"
#include "nsIDOMAppCoresManager.h"

//static nsIDOMAppCoresManager *appCoresManager = nsnull;
static NS_DEFINE_IID(kIDOMAppCoresManagerIID, NS_IDOMAPPCORESMANAGER_IID);
static NS_DEFINE_IID(kAppCoresManagerCID,     NS_APPCORESMANAGER_CID);
//#endif
/*********************************************/

/*
 * This routine translates the nsresult into a platform specific return
 * code for the application...
 */
static int TranslateReturnValue(nsresult aResult)
{
  if (NS_SUCCEEDED(aResult)) {
    return 0;
  }
  return 1;
}


extern "C" void NS_SetupRegistry_1();

static void
PrintUsage(void)
{
   fprintf(stderr, "Usage: apprunner <url>\n");
   fprintf(stderr, "\t<url>:  a fully defined url string like http:// etc..\n");
}


int main(int argc, char* argv[])
{
  nsresult rv;
  nsString controllerCID;

  nsICmdLineService *  cmdLineArgs = nsnull;
  char *  urlstr=nsnull;
  char *   progname = nsnull;
  char *   width=nsnull, *height=nsnull;
  char *  iconic_state=nsnull;

  PRInt32 widthVal  = 615;
  PRInt32 heightVal = 650;

  nsIAppShellService* appShell = nsnull;

  /*
   * Initialize XPCOM.  Ultimately, this should be a function call such as
   * NS_XPCOM_Initialize(...).
   *
   * - PL_EventsLib(...)
   * - Repository
   * - ServiceManager
   */
  // XXX: This call will be replaced by a registry initialization...
  NS_SetupRegistry_1();

  /*
   * Start up the core services:
   *  - Command-line processor.
   */

  rv = nsServiceManager::GetService(kCmdLineServiceCID,
                                    kICmdLineServiceIID,
                                    (nsISupports **)&cmdLineArgs);
  if (!NS_SUCCEEDED(rv)) {
    fprintf(stderr, "Could not obtain CmdLine processing service\n");
    goto done;
  }

  // Initialize the cmd line service 
  rv = cmdLineArgs->Initialize(argc, argv);
  if (rv  == NS_ERROR_INVALID_ARG) {
    PrintUsage();
    goto done;
  }
  
  // Get the URL to load
  rv = cmdLineArgs->GetURLToLoad(&urlstr);
  if (rv  == NS_ERROR_INVALID_ARG) {
    PrintUsage();
    goto done;
  }
  // Default URL if one was not provided in the cmdline
  if (nsnull == urlstr)
      urlstr = "resource:/res/samples/navigator.xul";
  else
      fprintf(stderr, "URL to load is %s\n", urlstr);


  // Check if -iconic was set
  rv = cmdLineArgs->GetCmdLineValue("-iconic", &iconic_state);
  if (rv != NS_OK)
     goto done;
  else {
    if (nsnull == iconic_state)
      fprintf(stderr, "iconic  state not set\n");
    else
      fprintf(stderr, "iconic state set \n");
  }
  
 
  // Get the value of -width option
  rv = cmdLineArgs->GetCmdLineValue("-width", &width);
  if (rv != NS_OK)
     goto done;
  else {
    if (width) {
      PR_sscanf(width, "%d", &widthVal);
      fprintf(stderr, "Width is set to %d\n", widthVal);
    } else {
      fprintf(stderr, "width was not set\n");
    }
  }
  
  // Get the value of -height option
  rv = cmdLineArgs->GetCmdLineValue("-height", &height);
  if (rv != NS_OK)
     goto done;
  else {
    if (height) {
      PR_sscanf(height, "%d", &heightVal);
      fprintf(stderr, "height is set to %d\n", heightVal);
    } else {
      fprintf(stderr, "height was not set\n");
    }
  }
  
   

  /*
   * Create the Application Shell instance...
   */
  rv = nsServiceManager::GetService(kAppShellServiceCID,
                                    kIAppShellServiceIID,
                                    (nsISupports**)&appShell);
  if (!NS_SUCCEEDED(rv)) {
    goto done;
  }

  /*
   * Initialize the Shell...
   */
  rv = appShell->Initialize();
  if (!NS_SUCCEEDED(rv)) {
    goto done;
  }
 
  /*
   * Post an event to the shell instance to load the AppShell 
   * initialization routines...  
   * 
   * This allows the application to enter its event loop before having to 
   * deal with GUI initialization...
   */
  ///write me...
  nsIURL* url;
  nsIWidget* newWindow;
  
  rv = NS_NewURL(&url, urlstr);
  if (NS_FAILED(rv)) {
    goto done;
  }

  /* ********************************************************************* */
  /* This is a temporary hack to get mail working with autoregistration
   * This won't succeed unless messenger was found
   * through autoregistration, and will go away when the service manager
     * is accessable from JavaScript
     */
  /* Comments/questions to alecf@netscape.com */
  {
    nsIAppShellService *messenger;
    const char *messengerProgID = "component://netscape/messenger";
    nsresult result = nsRepository::CreateInstance(messengerProgID,
                                                   nsnull,
                                                   nsIAppShellService::GetIID(),
                                                   (void **)&messenger);
    if (NS_SUCCEEDED(result)) {
      result = messenger->Initialize();
    }
  }
  /* End of mailhack */
  /* ********************************************************************* */

  /* Kick off appcores */
  nsIDOMAppCoresManager *appCoresManager;
  rv = nsServiceManager::GetService(kAppCoresManagerCID,
                                    kIDOMAppCoresManagerIID,
                                    (nsISupports**)&appCoresManager);
	if (rv == NS_OK) {
		if (appCoresManager->Startup() != NS_OK) {
		  appCoresManager->Shutdown();
      nsServiceManager::ReleaseService(kAppCoresManagerCID, appCoresManager);
		}
  }

  /*
   * XXX: Currently, the CID for the "controller" is passed in as an argument 
   *      to CreateTopLevelWindow(...).  Once XUL supports "controller" 
   *      components this will be specified in the XUL description...
   */
  controllerCID = "43147b80-8a39-11d2-9938-0080c7cb1081";
  rv = appShell->CreateTopLevelWindow(nsnull, url, controllerCID, newWindow,
                   nsnull, nsnull, widthVal, heightVal);

  NS_RELEASE(url);
  if (NS_FAILED(rv)) goto done;

 
  /*
   * Start up the main event loop...
   */
  rv = appShell->Run();

  /*
   * Shut down the Shell instance...  This is done even if the Run(...)
   * method returned an error.
   */
  (void) appShell->Shutdown();

done:
  /* Release the services... */
#if 0
  if (nsnull != cmdLineArgs) {
    nsServiceManager::ReleaseService(kCmdLineProcessorCID, cmdLineArgs);
  }
#endif

  /* Release the shell... */
  if (nsnull != appShell) {
    nsServiceManager::ReleaseService(kAppShellServiceCID, appShell);
  }

  /* Release the AppCoresManager... */
  if (nsnull != appCoresManager) {
		appCoresManager->Shutdown();
    nsServiceManager::ReleaseService(kAppCoresManagerCID, appCoresManager);
  }

  /* 
   * Translate the nsresult into an appropriate platform-specific return code.
   */
  return TranslateReturnValue(rv);
}
