/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s):
 *   Pierre Phaneuf <pp@ludusdesign.com>
 */
#include "nsIServiceManager.h"
#include "nsIComponentManager.h"
#include "nsIURL.h"
#include "nsNetUtil.h"
#include "nsIWidget.h"
#include "nsIPref.h"
#include "plevent.h"
#include "prmem.h"
#include "prnetdb.h"

#include "nsIAppShell.h"
#include "nsICmdLineService.h"
#include "nsIThread.h"
#include "nsIAppShellService.h"
#include "nsIAppShellComponent.h"
#include "nsIObserverService.h"
#include "nsAppShellCIDs.h"
#include "prprf.h"
#include "nsCRT.h"
#include "nsFileSpec.h"
#include "nsIFileSpec.h"
#include "nsIFileLocator.h"
#include "nsFileLocations.h"
#include "nsFileStream.h"
#include "nsSpecialSystemDirectory.h"
#include "nsIWalletService.h"
#include "nsIWebShell.h"
#include "nsIWindowMediator.h"
#include "nsIDOMWindow.h"
#include "nsIClipboard.h"
#include "nsISoftwareUpdate.h"
#include "nsSoftwareUpdateIIDs.h"
#include "nsISupportsPrimitives.h"
#include "nsICmdLineHandler.h"
#include "nsICategoryManager.h"
#include "nsXPIDLString.h"
#include "nsIXULWindow.h"
#include "nsIChromeRegistry.h"
#include "nsIContentHandler.h"
#include "nsIBrowserInstance.h"

// Interfaces Needed
#include "nsIXULWindow.h"
#include "nsIWebBrowserChrome.h"

#ifdef NS_TRACE_MALLOC
#include "nsTraceMalloc.h"
#endif


#if !defined(XP_MAC) && !defined(RHAPSODY)
#include "nsTimeBomb.h"
#endif

#ifdef RHAPSODY
// XXX hack because we can't link against libraptorwebwidget (pav)
#undef DETECT_WEBSHELL_LEAKS
#endif

#if defined(DEBUG_sspitzer) || defined(DEBUG_seth)
#define DEBUG_CMD_LINE
#endif


static NS_DEFINE_CID(kSoftUpdateCID,     NS_SoftwareUpdate_CID);
static NS_DEFINE_IID(kIWindowMediatorIID,NS_IWINDOWMEDIATOR_IID);
static NS_DEFINE_CID(kWindowMediatorCID, NS_WINDOWMEDIATOR_CID);
static NS_DEFINE_CID(kWalletServiceCID,     NS_WALLETSERVICE_CID);
static NS_DEFINE_CID(kBrowserContentHandlerCID, NS_BROWSERCONTENTHANDLER_CID);


#define HELP_SPACER_1   "\t"
#define HELP_SPACER_2   "\t\t"

#ifdef DEBUG
#include "prlog.h"
#endif

#ifdef MOZ_JPROF
#include "jprof.h"
#endif

// header file for profile manager
#include "nsIProfile.h"

#if defined(XP_UNIX)
  extern void InstallUnixSignalHandlers(const char *ProgramName);
#endif

#if defined(XP_MAC)

#include "macstdlibextras.h"
#include <TextServices.h>

static nsresult CheckForNewChrome(void);

// Set up the toolbox and (if DEBUG) the console.  Do this in a static initializer,
// to make it as unlikely as possible that somebody calls printf() before we get initialized.
static struct MacInitializer { MacInitializer() { InitializeMacToolbox(); } } gInitializer;


class stTSMCloser
{
public:
	stTSMCloser()
  {
    // TSM is initialized in InitializeMacToolbox
  };
	
	~stTSMCloser()
	{
#if !TARGET_CARBON	
		(void)CloseTSMAwareApplication();
#endif
	}
};
#endif // XP_MAC

/* Define Class IDs */
static NS_DEFINE_CID(kAppShellServiceCID,   NS_APPSHELL_SERVICE_CID);
static NS_DEFINE_CID(kCmdLineServiceCID,    NS_COMMANDLINE_SERVICE_CID);
static NS_DEFINE_CID(kPrefCID,              NS_PREF_CID);
static NS_DEFINE_CID(kFileLocatorCID,       NS_FILELOCATOR_CID);
static NS_DEFINE_CID(kProfileCID,           NS_PROFILE_CID);

#include "nsNativeAppSupport.h"

/*********************************************/
// Default implemenations for nativeAppSupport
// If your platform implements these functions if def out this code.
#if !defined (XP_MAC ) && !defined(NTO) && ( !defined( XP_PC ) || !defined( WIN32 ) )

nsresult NS_CreateSplashScreen( nsISplashScreen **aResult )
{	
    nsresult rv = NS_OK;
    if ( aResult ) {
        *aResult = 0;
    } else {
        rv = NS_ERROR_NULL_POINTER;
    }
    return rv;
}

PRBool NS_CanRun()
{
	return PR_TRUE;
}
#endif

/*********************************************/
// Default implementation for new and improved
// native app support.  If your platform
// implements nsINativeAppSupport then implement
// this function and if def out this code.
//
// Note: For now, the default imiplementation returns 0 and
//       the code that calls this will defalt to use the old
//       nsISplashScreen interface directly.  At some point
//       this function will return an instance of
//       nsNativeAppSupportBase which will use the older
//       "splash screen" interface.  The code below will
//       then rely on nsINativeAppSupport and its use of
//       nsISplashScreen will be removed.
//
#if !defined( NS_WIN32 )

nsresult NS_CreateNativeAppSupport( nsINativeAppSupport **aResult )
{
    nsresult rv = NS_OK;
    if ( aResult ) {
        *aResult = 0;
    } else {
        rv = NS_ERROR_NULL_POINTER;
    }
    return rv;
}

#endif

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

#ifdef XP_MAC
#include "nsCommandLineServiceMac.h"
#endif

extern "C" void NS_SetupRegistry_1(PRBool aNeedAutoreg);

static void
PrintUsage(void)
{
  fprintf(stderr, "Usage: apprunner <url>\n");
  fprintf(stderr, "\t<url>:  a fully defined url string like http:// etc..\n");
}

static nsresult OpenWindow( const char*urlstr, const PRUnichar *args )
{
#ifdef DEBUG_CMD_LINE
    printf("OpenWindow(%s,?)\n",urlstr);
#endif /* DEBUG_CMD_LINE */
    nsresult rv;
    NS_WITH_SERVICE(nsIAppShellService, appShellService, kAppShellServiceCID, &rv)
    if ( NS_SUCCEEDED( rv ) ) {
        nsCOMPtr<nsIDOMWindow> hiddenWindow;
        JSContext *jsContext;
        rv = appShellService->GetHiddenWindowAndJSContext( getter_AddRefs( hiddenWindow ),
                                                           &jsContext );
        if ( NS_SUCCEEDED( rv ) ) {
            void *stackPtr;
            jsval *argv = JS_PushArguments( jsContext,
                                            &stackPtr,
                                            "sssW",
                                            urlstr,
                                            "_blank",
                                            "chrome,dialog=no,all",
                                            args );
            if( argv ) {
                nsCOMPtr<nsIDOMWindow> newWindow;
                rv = hiddenWindow->OpenDialog( jsContext,
                                               argv,
                                               4,
                                               getter_AddRefs( newWindow ) );
                JS_PopArguments( jsContext, stackPtr );
            }
        }
    }
    return rv;
}

static nsresult OpenChromURL( const char * urlstr, PRInt32 height = NS_SIZETOCONTENT, PRInt32 width = NS_SIZETOCONTENT )
{
#ifdef DEBUG_CMD_LINE
    printf("OpenChromURL(%s,%d,%d)\n",urlstr,height,width);
#endif /* DEBUG_CMD_LINE */

	nsCOMPtr<nsIURI> url;
	nsresult  rv;
	rv = NS_NewURI(getter_AddRefs(url), urlstr);
	if ( NS_FAILED( rv ) )
		return rv;

   nsCOMPtr<nsIAppShellService> appShell(do_GetService(kAppShellServiceCID));
   NS_ENSURE_TRUE(appShell, NS_ERROR_FAILURE);

   nsCOMPtr<nsIXULWindow> newWindow;
 	rv = appShell->CreateTopLevelWindow(nsnull, url,
                                      PR_TRUE, PR_TRUE, nsIWebBrowserChrome::allChrome,
                                      width, height,
                                      getter_AddRefs(newWindow));
  return rv;
}


static void DumpArbitraryHelp()
{
  nsresult rv;
  NS_WITH_SERVICE(nsICategoryManager, catman, "mozilla.categorymanager.1", &rv);
  if(NS_SUCCEEDED(rv) && catman) {
    nsCOMPtr<nsISimpleEnumerator> e;
    rv = catman->EnumerateCategory(COMMAND_LINE_ARGUMENT_HANDLERS, getter_AddRefs(e));
    if(NS_SUCCEEDED(rv) && e) {
      while (PR_TRUE) {
        nsCOMPtr<nsISupportsString> catEntry;
        rv = e->GetNext(getter_AddRefs(catEntry));
        if (NS_FAILED(rv) || !catEntry) break;

        nsXPIDLCString entryString;
        rv = catEntry->GetData(getter_Copies(entryString));
        if (NS_FAILED(rv) || !((const char *)entryString)) break;

		nsXPIDLCString progidString;
		rv = catman->GetCategoryEntry(COMMAND_LINE_ARGUMENT_HANDLERS,(const char *)entryString, getter_Copies(progidString));
        if (NS_FAILED(rv) || !((const char *)progidString)) break;

#ifdef DEBUG_CMD_LINE
        printf("cmd line handler progid = %s\n", (const char *)progidString);
#endif /* DEBUG_CMD_LINE */

        nsCOMPtr <nsICmdLineHandler> handler = do_GetService((const char *)progidString, &rv);

        if (handler) {
          nsXPIDLCString commandLineArg;
          rv = handler->GetCommandLineArgument(getter_Copies(commandLineArg));
          if (NS_FAILED(rv)) continue;

          nsXPIDLCString helpText;
          rv = handler->GetHelpText(getter_Copies(helpText));
          if (NS_FAILED(rv)) continue;

          if ((const char *)commandLineArg) {
            printf("%s%s", HELP_SPACER_1,(const char *)commandLineArg);

            PRBool handlesArgs = PR_FALSE;
            rv = handler->GetHandlesArgs(&handlesArgs);
            if (NS_SUCCEEDED(rv) && handlesArgs) {
                printf(" <url>");
            }
            if ((const char *)helpText) {
              printf("%s%s\n",HELP_SPACER_2,(const char *)helpText);
            }
          }
        }

      }
    }
  }
  return;
}

static
nsresult LaunchApplication(const char *progID, PRInt32 height, PRInt32 width)
{
  nsresult rv = NS_OK;

  nsCOMPtr <nsICmdLineHandler> handler = do_GetService(progID, &rv);
  if (NS_FAILED(rv)) return rv;

  if (!handler) return NS_ERROR_FAILURE;

  nsXPIDLCString chromeUrlForTask;
  rv = handler->GetChromeUrlForTask(getter_Copies(chromeUrlForTask));
  if (NS_FAILED(rv)) return rv;

  PRBool handlesArgs = PR_FALSE;
  rv = handler->GetHandlesArgs(&handlesArgs);
  if (handlesArgs) {
    PRUnichar *defaultArgs = nsnull;
    rv = handler->GetDefaultArgs(&defaultArgs);
    if (NS_FAILED(rv)) return rv;
    rv = OpenWindow((const char *)chromeUrlForTask, defaultArgs);
    Recycle(defaultArgs);
  }
  else {
    rv = OpenChromURL((const char *)chromeUrlForTask, height, width);
  }

  return rv;
}

static nsresult LaunchApplicationWithArgs(const char *commandLineArg, nsICmdLineService *cmdLineArgs, const char *progID, PRInt32 height, PRInt32 width)
{
  nsresult rv;
  if (!progID || !commandLineArg || !cmdLineArgs) return NS_ERROR_FAILURE;
	nsXPIDLCString cmdResult;

  nsCOMPtr <nsICmdLineHandler> handler = do_GetService(progID, &rv);
  if (NS_FAILED(rv)) return rv;

  if (!handler) return NS_ERROR_FAILURE;

  nsXPIDLCString chromeUrlForTask;
  rv = handler->GetChromeUrlForTask(getter_Copies(chromeUrlForTask));
  if (NS_FAILED(rv)) return rv;

#ifdef DEBUG_CMD_LINE
  printf("XXX got this one:\t%s\n\t%s\n\n",commandLineArg,(const char *)chromeUrlForTask);
#endif /* DEBUG_CMD_LINE */

  rv = cmdLineArgs->GetCmdLineValue(commandLineArg, getter_Copies(cmdResult));
  if (NS_FAILED(rv)) return rv;
#ifdef DEBUG_CMD_LINE
  printf("%s, cmdResult = %s\n",commandLineArg,(const char *)cmdResult);
#endif /* DEBUG_CMD_LINE */

  PRBool handlesArgs = PR_FALSE;
  rv = handler->GetHandlesArgs(&handlesArgs);
  if (handlesArgs) {
    if ((const char *)cmdResult) {
      if (PL_strcmp("1",(const char *)cmdResult)) {
        PRBool openWindowWithArgs = PR_TRUE;
        rv = handler->GetOpenWindowWithArgs(&openWindowWithArgs);
        if (NS_FAILED(rv)) return rv;

        if (openWindowWithArgs) {
          nsString cmdArgs; cmdArgs.AssignWithConversion(NS_STATIC_CAST(const char *, cmdResult));
#ifdef DEBUG_CMD_LINE
          printf("opening %s with %s\n",(const char *)chromeUrlForTask,"OpenWindow");
#endif /* DEBUG_CMD_LINE */
          rv = OpenWindow((const char *)chromeUrlForTask, cmdArgs.GetUnicode());
        }
        else {
#ifdef DEBUG_CMD_LINE
          printf("opening %s with %s\n",(const char *)cmdResult,"OpenChromURL");
#endif /* DEBUG_CMD_LINE */
          rv = OpenChromURL((const char *)cmdResult,height, width);
          if (NS_FAILED(rv)) return rv;
        }
      }
      else {
        PRUnichar *defaultArgs;
        rv = handler->GetDefaultArgs(&defaultArgs);
        if (NS_FAILED(rv)) return rv;

        rv = OpenWindow((const char *)chromeUrlForTask, defaultArgs);
        Recycle(defaultArgs);
        if (NS_FAILED(rv)) return rv;
      }
    }
  }
  else {
    if (NS_SUCCEEDED(rv) && (const char*)cmdResult) {
      if (PL_strcmp("1",(const char *)cmdResult) == 0) {
        rv = OpenChromURL((const char *)chromeUrlForTask,height, width);
        if (NS_FAILED(rv)) return rv;
      }
      else {
        rv = OpenChromURL((const char *)cmdResult, height, width);
        if (NS_FAILED(rv)) return rv;
      }
    }
  }

  return NS_OK;
}

typedef struct
{
  nsIPref *prefs;
  PRInt32 height;
  PRInt32 width;
} StartupClosure;

static
void startupPrefEnumerationFunction(const char *prefName, void *data)
{
  nsresult rv;
  PRBool prefValue = PR_FALSE;

  if (!data || !prefName) return;

  StartupClosure *closure = (StartupClosure *)data;

#ifdef DEBUG_CMD_LINE
  printf("getting %s\n", prefName);
#endif /* DEBUG_CMD_LINE */

  rv = closure->prefs->GetBoolPref(prefName, &prefValue);
  if (NS_FAILED(rv)) return;

#ifdef DEBUG_CMD_LINE
  printf("%s = %d\n", prefName, prefValue);
#endif /* DEBUG_CMD_LINE */

  PRUint32 prefixLen = PL_strlen(PREF_STARTUP_PREFIX);

  // if the pref is "general.startup.", ignore it.
  if (PL_strlen(prefName) <= prefixLen) return;

  if (prefValue) {
    // this is the progid prefix that all the command line handlers register
    nsCAutoString progID = "component://netscape/commandlinehandler/general-startup-";
    progID += (prefName + prefixLen);

#ifdef DEBUG_CMD_LINE
    printf("progid = %s\n", (const char *)progID);
#endif /* DEBUG_CMD_LINE */
    rv = LaunchApplication((const char *)progID, closure->height, closure->width);
  }
  return;
}

static PRBool IsStartupCommand(const char *arg)
{
  if (!arg) return PR_FALSE;

  if (PL_strlen(arg) <= 1) return PR_FALSE;

  // windows allows /mail or -mail
  if ((arg[0] == '-')
#ifdef XP_PC
      || (arg[0] == '/')
#endif /* XP_PC */
      ) {
    return PR_TRUE;
  }

  return PR_FALSE;
}

static nsresult HandleArbitraryStartup( nsICmdLineService* cmdLineArgs, nsIPref *prefs,  PRBool heedGeneralStartupPrefs)
{
	nsresult rv;
	PRInt32 height  = NS_SIZETOCONTENT;
	PRInt32 width  = NS_SIZETOCONTENT;
	nsXPIDLCString tempString;

	// Get the value of -width option
	rv = cmdLineArgs->GetCmdLineValue("-width", getter_Copies(tempString));
	if (NS_FAILED(rv)) return rv;
	
	if ((const char*)tempString) PR_sscanf(tempString, "%d", &width);
	
	// Get the value of -height option
	rv = cmdLineArgs->GetCmdLineValue("-height", getter_Copies(tempString));
	if (NS_FAILED(rv)) return rv;
	
	if ((const char*)tempString) PR_sscanf(tempString, "%d", &height);

  if (heedGeneralStartupPrefs) {
#ifdef DEBUG_CMD_LINE
    printf("XXX iterate over all the general.startup.* prefs\n");
#endif /* DEBUG_CMD_LINE */
    StartupClosure closure;

    closure.prefs = prefs;
    closure.height = height;
    closure.width = width;

    prefs->EnumerateChildren(PREF_STARTUP_PREFIX, startupPrefEnumerationFunction,(void *)(&closure));
  }
  else {
    PRInt32 argc = 0;
    rv = cmdLineArgs->GetArgc(&argc);
	if (NS_FAILED(rv)) return rv;

    NS_ASSERTION(argc > 1, "we shouldn't be here if there were no command line arguments");
    if (argc <= 1) return NS_ERROR_FAILURE;

    char **argv = nsnull;
    rv = cmdLineArgs->GetArgv(&argv);
    if (NS_FAILED(rv)) return rv;

    PRInt32 i = 0;
    for (i=1;i<argc;i++) {
#ifdef DEBUG_CMD_LINE
      printf("XXX argv[%d] = %s\n",i,argv[i]);
#endif /* DEBUG_CMD_LINE */
      if (IsStartupCommand(argv[i])) {
        nsCAutoString progID = "component://netscape/commandlinehandler/general-startup-";

        // skip over the - (or / on windows)
        char *command = argv[i] + 1;
#ifdef XP_UNIX
        // unix allows -mail and --mail
        if ((argv[i][0] == '-') && (argv[i][1] == '-')) {
          command = argv[i] + 2;
        }
#endif /* XP_UNIX */
        progID += (const char *)command;
        // this can fail, as someone could do -foo, where -foo is not handled
        rv = LaunchApplicationWithArgs((const char *)(argv[i]), cmdLineArgs, (const char *)progID, height, width);
      }
    }
  }

  return NS_OK;
}

// This should be done by app shell enumeration someday
static nsresult DoCommandLines( nsICmdLineService* cmdLine, PRBool heedGeneralStartupPrefs )
{
	nsresult  rv;
	
	NS_WITH_SERVICE(nsIPref, prefs, kPrefCID, &rv);
	if (NS_FAILED(rv)) return rv;

    rv = HandleArbitraryStartup( cmdLine, prefs, heedGeneralStartupPrefs);
	return rv;
}

static nsresult DoOnShutdown()
{
  nsresult rv;
  {
    // Scoping this in a block to force the pref service to be
    // released.
    //
  	// save the prefs, in case they weren't saved
  	NS_WITH_SERVICE(nsIPref, prefs, kPrefCID, &rv);
  	NS_ASSERTION(NS_SUCCEEDED(rv), "failed to get prefs, so unable to save them");
  	if (NS_SUCCEEDED(rv)) {
  		prefs->SavePrefFile();
  	}
  }

  // at this point, all that is on the clipboard is a proxy object, but that object
  // won't be valid once the app goes away. As a result, we need to force the data
  // out of that proxy and properly onto the clipboard. This can't be done in the
  // clipboard service's shutdown routine because it requires the parser/etc which
  // has already been shutdown by the time the clipboard is shut down.
  {
    // scoping this in a block to force release
    NS_WITH_SERVICE(nsIClipboard, clipService, "component://netscape/widget/clipboard", &rv);
    if ( clipService )
      clipService->ForceDataToClipboard(nsIClipboard::kGlobalClipboard);
  }
  return rv;
}


static nsresult OpenBrowserWindow(PRInt32 height, PRInt32 width)
{
    nsresult rv;
    NS_WITH_SERVICE(nsICmdLineHandler, handler, NS_BROWSERSTARTUPHANDLER_PROGID, &rv);
    if (NS_FAILED(rv)) return rv;

    nsXPIDLCString chromeUrlForTask;
    rv = handler->GetChromeUrlForTask(getter_Copies(chromeUrlForTask));
    if (NS_FAILED(rv)) return rv;

    rv = OpenChromURL((const char *)chromeUrlForTask, height, width );
    if (NS_FAILED(rv)) return rv;

    return rv;
}


static void	InitCachePrefs()
{
	const char * const CACHE_DIR_PREF   = "browser.cache.directory";
	nsresult rv;
	NS_WITH_SERVICE(nsIPref, prefs, NS_PREF_PROGID, &rv);
	if (NS_FAILED(rv)) return;
		
	// If the pref is already set don't do anything
	nsCOMPtr<nsIFileSpec> cacheSubDir;
	rv = prefs->GetFilePref(CACHE_DIR_PREF, getter_AddRefs(cacheSubDir));
	if (NS_SUCCEEDED(rv) && cacheSubDir.get()) return;	

// Set up the new pref
	rv = NS_NewFileSpec(getter_AddRefs(cacheSubDir));
	if(NS_FAILED(rv)) return;

	nsCOMPtr<nsIFileSpec> spec(dont_AddRef(NS_LocateFileOrDirectory(nsSpecialFileSpec::App_UserProfileDirectory50)));	
	rv = cacheSubDir->FromFileSpec(spec);
	if(NS_FAILED(rv)) return;
		
	rv = cacheSubDir->AppendRelativeUnixPath("Cache") ;
	if(NS_FAILED(rv))	return;
	
	prefs->SetFilePref(CACHE_DIR_PREF, cacheSubDir, PR_FALSE);
}


static nsresult Ensure1Window( nsICmdLineService* cmdLineArgs)
{
	nsresult rv;
	NS_WITH_SERVICE(nsIWindowMediator, windowMediator, kWindowMediatorCID, &rv);
	nsCOMPtr<nsISimpleEnumerator> windowEnumerator;

  if (NS_SUCCEEDED(windowMediator->GetEnumerator(nsnull, getter_AddRefs(windowEnumerator))))
  {
    PRBool more;
	
    windowEnumerator->HasMoreElements(&more);
    if ( !more )
    {
      // No window exists so lets create a browser one
      PRInt32 height  = NS_SIZETOCONTENT;
      PRInt32 width  = NS_SIZETOCONTENT;
				
      // Get the value of -width option
      nsXPIDLCString tempString;
      rv = cmdLineArgs->GetCmdLineValue("-width", getter_Copies(tempString));
      if (NS_FAILED(rv))
        return rv;
      if ((const char*)tempString)
        PR_sscanf(tempString, "%d", &width);
				
				
      // Get the value of -height option
      rv = cmdLineArgs->GetCmdLineValue("-height", getter_Copies(tempString));
      if (NS_FAILED(rv))
        return rv;

      if ((const char*)tempString)
        PR_sscanf(tempString, "%d", &height);
				
      rv = OpenBrowserWindow(height, width);
    }
	}
	return rv;
}

nsresult CheckForNewChrome(void) {

  nsCOMPtr <nsIChromeRegistry> chromeReg = do_GetService("component://netscape/chrome/chrome-registry");
  NS_ASSERTION(chromeReg, "chrome check couldn't get the chrome registry");

  if (chromeReg)
    return chromeReg->CheckForNewChrome();
  return NS_ERROR_FAILURE;
}

#ifdef DEBUG_warren
#ifdef XP_PC
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#endif

#if defined(FREEBSD)
// pick up fpsetmask prototype.
#include <floatingpoint.h>
#endif

// Note: nativeApp is an owning reference that this function has responsibility
//       to release.  This responsibility is delegated to the app shell service
//       (see nsAppShellService::Initialize call, below).
static nsresult main1(int argc, char* argv[], nsISupports *nativeApp )
{
  nsresult rv;

#ifdef DEBUG_warren
//  _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_CHECK_ALWAYS_DF);
#endif


#ifndef XP_MAC
  // Unbuffer debug output (necessary for automated QA performance scripts).
  setbuf( stdout, 0 );
#endif

#if defined(FREEBSD)
  // Disable all SIGFPE's on FreeBSD, as it has non-IEEE-conformant fp
  // trap behavior that trips up on floating-point tests performed by
  // the JS engine.  See bugzilla bug 9967 details.
  fpsetmask(0);
#endif

  // Setup an autoreg obserer, so that we can update a progress
  // string in the splash screen
  nsCOMPtr<nsIObserverService> obsService = do_GetService(NS_OBSERVERSERVICE_PROGID);
  if (obsService)
  {
    nsCOMPtr<nsIObserver> splashScreenObserver = do_QueryInterface(nativeApp);
    if (splashScreenObserver)
    {
      obsService->AddObserver(splashScreenObserver, NS_ConvertASCIItoUCS2(NS_XPCOM_AUTOREGISTRATION_OBSERVER_ID).GetUnicode());
    }
  }

  //----------------------------------------------------------------
  // XPInstall needs to clean up after any updates that couldn't
  // be completed because components were in use. This must be done
  // **BEFORE** any other libraries are loaded!
  //
  // Will also check to see if AutoReg is required due to version
  // change or installation of new components. If for some reason
  // XPInstall can't be loaded we assume Autoreg is required.
  //
  // (scoped in a block to force release of COMPtr)
  //----------------------------------------------------------------
  PRBool needAutoreg = PR_TRUE;
  {
    nsCOMPtr<nsISoftwareUpdate> su = do_GetService(kSoftUpdateCID,&rv);
    if (NS_SUCCEEDED(rv))
    {
      su->StartupTasks( &needAutoreg );
    }
  }
//  nsServiceManager::UnregisterService(kSoftUpdateCID);

#if XP_MAC
  stTSMCloser  tsmCloser;

  rv = InitializeMacCommandLine( argc, argv);
  NS_ASSERTION(NS_SUCCEEDED(rv), "Initializing AppleEvents failed");
#endif

  // XXX: This call will be replaced by a registry initialization...
  NS_SetupRegistry_1( needAutoreg );

  // remove the nativeApp as an XPCOM autoreg observer
  if (obsService)
  {
    nsCOMPtr<nsIObserver> splashScreenObserver = do_QueryInterface(nativeApp);
    if (splashScreenObserver)
    {
      obsService->RemoveObserver(splashScreenObserver, NS_ConvertASCIItoUCS2(NS_XPCOM_AUTOREGISTRATION_OBSERVER_ID).GetUnicode());
    }
  }

  // Start up the core services:

  // Initialize the cmd line service
  NS_WITH_SERVICE(nsICmdLineService, cmdLineArgs, kCmdLineServiceCID, &rv);
  NS_ASSERTION(NS_SUCCEEDED(rv), "failed to get command line service");

  if (NS_FAILED(rv)) {
    fprintf(stderr, "Could not obtain CmdLine processing service\n");
    return rv;
  }

  rv = cmdLineArgs->Initialize(argc, argv);
  NS_ASSERTION(NS_SUCCEEDED(rv), "failed to initialize command line args");
  if (rv  == NS_ERROR_INVALID_ARG) {
    PrintUsage();
    return rv;
  }

  CheckForNewChrome();

  // Create the Application Shell instance...
  NS_WITH_SERVICE(nsIAppShellService, appShell, kAppShellServiceCID, &rv);
  NS_ASSERTION(NS_SUCCEEDED(rv), "failed to get the appshell service");
  if (NS_FAILED(rv)) {
    // See if platform supports nsINativeAppSupport.
    nsCOMPtr<nsINativeAppSupport> nativeAppSupport = do_QueryInterface( nativeApp );
    if ( nativeAppSupport ) {
        // Use that interface to remove splash screen.
        nativeAppSupport->HideSplashScreen();
    } else {
        // See if platform supports nsISplashScreen, instead.
        nsCOMPtr<nsISplashScreen> splashScreen = do_QueryInterface( nativeApp );
        if ( splashScreen ) {
            splashScreen->Hide();
        }
    }

    // Release argument object.
    NS_IF_RELEASE( nativeApp );
    return rv;
  }

  rv = appShell->Initialize( cmdLineArgs, nativeApp );
  // We are done with the native app (or splash screen) object here;
  // the app shell owns it now.
  NS_IF_RELEASE( nativeApp );
  NS_ASSERTION(NS_SUCCEEDED(rv), "failed to initialize appshell");
  if ( NS_FAILED(rv) ) return rv;

  NS_WITH_SERVICE(nsIProfile, profileMgr, kProfileCID, &rv);
  NS_ASSERTION(NS_SUCCEEDED(rv), "failed to get profile manager");
  if ( NS_FAILED(rv) ) return rv;

  rv = profileMgr->StartupWithArgs(cmdLineArgs);
  if (NS_FAILED(rv)) return rv;

  // if we get here, and we don't have a current profile, return a failure so we will exit
  // this can happen, if the user hits Cancel or Exit in the profile manager dialogs
  nsXPIDLString currentProfileStr;
  rv = profileMgr->GetCurrentProfile(getter_Copies(currentProfileStr));
  if (NS_FAILED(rv) || !((const PRUnichar *)currentProfileStr) ||
                        (nsCRT::strlen((const PRUnichar *)currentProfileStr) == 0)) {
  	return NS_ERROR_FAILURE;
  }

  // rjc: now must explicitly call appshell's CreateHiddenWindow() function AFTER profile manager.
  //      if the profile manager ever switches to using nsIDOMWindow stuff, this might have to change
  appShell->CreateHiddenWindow();

	// Enumerate AppShellComponenets
	appShell->EnumerateAndInitializeComponents();
	
	// This will go away once Components are handling there own commandlines
	// if we have no command line arguments, we need to heed the
	// "general.startup.*" prefs
	// if we had no command line arguments, argc == 1.
#ifdef XP_MAC
	// if we do no command line args on the mac, it says argc is 0, and not 1
	rv = DoCommandLines( cmdLineArgs, ((argc == 1) || (argc == 0)) );
#else
	rv = DoCommandLines( cmdLineArgs, (argc == 1) );
#endif /* XP_MAC */
    NS_ASSERTION(NS_SUCCEEDED(rv), "failed to process command line");
	if ( NS_FAILED(rv) )
    return rv;

  // Make sure there exists at least 1 window.
  rv = Ensure1Window( cmdLineArgs );
  NS_ASSERTION(NS_SUCCEEDED(rv), "failed to Ensure1Window");
  if (NS_FAILED(rv)) return rv;

  // Fire up the walletService. Why the heck is this here?
  NS_WITH_SERVICE(nsIWalletService, walletService, kWalletServiceCID, &rv);
  NS_ASSERTION(NS_SUCCEEDED(rv), "wallet failed");
  if (NS_SUCCEEDED(rv))
  {
    // this is a no-op. What is going on?
    walletService->WALLET_FetchFromNetCenter();
  }

	InitCachePrefs();
	
  // Start main event loop
  rv = appShell->Run();
  NS_ASSERTION(NS_SUCCEEDED(rv), "failed to run appshell");

#ifdef NS_TRACE_MALLOC
  // XXX we crash somewhere under nsAppShellService::Shutdown...
  NS_TraceMallocShutdown();
#endif

  /*
   * Shut down the Shell instance...  This is done even if the Run(...)
   * method returned an error.
   */
  (void) appShell->Shutdown();

  return rv;
}


// English text needs to go into a dtd file.
// But when this is called we have no components etc. These strings must either be
// here, or in a native resource file.
static void DumpHelp(char *appname)
{
  printf("Usage: %s [ options ... ] [URL]\n", appname);
  printf("       where options include:\n");
  printf("\n");
  printf("%s-height <value>%sSet height of startup window to <value>.\n",HELP_SPACER_1,HELP_SPACER_2);
  printf("%s-h or -help%sPrint this message.\n",HELP_SPACER_1,HELP_SPACER_2);
  printf("%s-installer%sStart with 4.x migration window.\n",HELP_SPACER_1,HELP_SPACER_2);
  printf("%s-pref%sStart with pref window.\n",HELP_SPACER_1,HELP_SPACER_2);
  printf("%s-width <value>%sSet width of startup window to <value>.\n",HELP_SPACER_1,HELP_SPACER_2);
  printf("%s-v or -version%sPrint %s version.\n",HELP_SPACER_1,HELP_SPACER_2, appname);
  printf("%s-CreateProfile <profile>%sCreate and start with <profile>.\n",HELP_SPACER_1,HELP_SPACER_2);
  printf("%s-P <profile>%sStart with <profile>.\n",HELP_SPACER_1,HELP_SPACER_2);
  printf("%s-ProfileWizard%sStart with profile wizard.\n",HELP_SPACER_1,HELP_SPACER_2);
  printf("%s-ProfileManager%sStart with profile manager.\n",HELP_SPACER_1,HELP_SPACER_2);
  printf("%s-SelectProfile%sStart with profile selection dialog.\n",HELP_SPACER_1,HELP_SPACER_2);
#ifdef XP_UNIX
  printf("%s-splash%sEnable splash screen.\n",HELP_SPACER_1,HELP_SPACER_2);
#else
  printf("%s-nosplash%sDisable splash screen.\n",HELP_SPACER_1,HELP_SPACER_2);
#ifdef XP_PC
  printf("%s-quiet%sDisable splash screen.\n",HELP_SPACER_1,HELP_SPACER_2);
#endif
#endif

  // this works, but only after the components have registered.  so if you drop in a new command line handler, -help
  // won't not until the second run.
  // out of the bug, because we ship a component.reg file, it works correctly.
  DumpArbitraryHelp();
}

static void DumpVersion(char *appname)
{
	printf("%s: version info\n", appname);
}


static PRBool HandleDumpArguments(int argc, char* argv[])
{
  int i = 0;

  for (i=1; i<argc; i++) {
    if ((PL_strcasecmp(argv[i], "-h") == 0)
        || (PL_strcasecmp(argv[i], "-help") == 0)
#if defined(XP_UNIX) || defined(XP_BEOS)
        || (PL_strcasecmp(argv[i], "--help") == 0)
#endif /* XP_UNIX || XP_BEOS*/
#ifdef XP_PC
        || (PL_strcasecmp(argv[i], "/h") == 0)
        || (PL_strcasecmp(argv[i], "/help") == 0)
        || (PL_strcasecmp(argv[i], "/?") == 0)
#endif /* XP_PC */
      ) {
      DumpHelp(argv[0]);
      return PR_TRUE;
    }
    if ((PL_strcasecmp(argv[i], "-v") == 0)
        || (PL_strcasecmp(argv[i], "-version") == 0)
#if defined(XP_UNIX) || defined(XP_BEOS)
        || (PL_strcasecmp(argv[i], "--version") == 0)
#endif /* XP_UNIX || XP_BEOS */
#ifdef XP_PC
        || (PL_strcasecmp(argv[i], "/v") == 0)
        || (PL_strcasecmp(argv[i], "/version") == 0)
#endif /* XP_PC */
      ) {
      DumpVersion(argv[0]);
      return PR_TRUE;
    }
  }

  return PR_FALSE;
}


static PRBool GetWantSplashScreen(int argc, char* argv[])
{
  int i;
  PRBool dosplash;
  // We can't use the command line service here because it isn't running yet
#if defined(XP_UNIX) && !defined(NTO)
  dosplash = PR_FALSE;
  for (i=1; i<argc; i++)
    if ((PL_strcasecmp(argv[i], "-splash") == 0)
        || (PL_strcasecmp(argv[i], "--splash") == 0))
      dosplash = PR_TRUE;
#else
  dosplash = PR_TRUE;
  for (i=1; i<argc; i++)
    if ((PL_strcasecmp(argv[i], "-nosplash") == 0)
#ifdef XP_BEOS
		|| (PL_strcasecmp(argv[i], "--nosplash") == 0)
#endif /* XP_BEOS */
#ifdef XP_PC
        || (PL_strcasecmp(argv[i], "/nosplash") == 0)
#endif /* XP_PC */
	) {
      dosplash = PR_FALSE;
	}
#endif

  return dosplash;
}


int main(int argc, char* argv[])
{
#if defined(XP_UNIX)
  InstallUnixSignalHandlers(argv[0]);
#endif

  // Handle -help and -version command line arguments.
  // They should return quick, so we deal with them here.
  if (HandleDumpArguments(argc, argv))
    return 0;

#ifdef NS_TRACE_MALLOC
  argc = NS_TraceMallocStartupArgs(argc, argv);
#endif

  // Call the code to install our handler
#ifdef MOZ_JPROF
  setupProfilingStuff();
#endif

  // Try to allocate "native app support."
  // Note: this object is not released here.  It is passed to main1 which
  //       has responsibility to release it.
  nsINativeAppSupport *nativeApp = 0;
  nsresult rv = NS_CreateNativeAppSupport( &nativeApp );

  // See if we can run.
  if (nativeApp)
  {
    PRBool canRun = PR_FALSE;
    rv = nativeApp->Start( &canRun );
    if (!canRun) {
        return 1;
    }
  } else {
    // If platform doesn't implement nsINativeAppSupport, fall
    // back to old method.
    if (!NS_CanRun())
      return 1;
  }
  // Note: this object is not released here.  It is passed to main1 which
  //       has responsibility to release it.
  nsISplashScreen *splash = 0;
  PRBool dosplash = GetWantSplashScreen(argc, argv);

  if (dosplash && !nativeApp) {
      // If showing splash screen and platform doesn't implement
      // nsINativeAppSupport, then use older nsISplashScreen interface.
      rv = NS_CreateSplashScreen(&splash);
      NS_ASSERTION( NS_SUCCEEDED(rv), "NS_CreateSplashScreen failed" );
  }
  // If the platform has a splash screen, show it ASAP.
  if (dosplash && nativeApp) {
      nativeApp->ShowSplashScreen();
  } else if (splash) {
      splash->Show();
  }
  rv = NS_InitXPCOM(NULL, NULL);
  NS_ASSERTION( NS_SUCCEEDED(rv), "NS_InitXPCOM failed" );

  nsresult mainResult = main1(argc, argv, nativeApp ? (nsISupports*)nativeApp : (nsISupports*)splash);

  rv = DoOnShutdown();
  NS_ASSERTION(NS_SUCCEEDED(rv), "DoOnShutdown failed");

  rv = NS_ShutdownXPCOM( NULL );
  NS_ASSERTION(NS_SUCCEEDED(rv), "NS_ShutdownXPCOM failed");

#ifdef DETECT_WEBSHELL_LEAKS
  unsigned long count;
  count = NS_TotalWebShellsInExistence();
  if (count)  {
    printf("XXX WARNING: Number of webshells being leaked: %d \n", (int)count);
  }
#endif

  return TranslateReturnValue(mainResult);
}

#if defined( XP_PC ) && defined( WIN32 )
// We need WinMain in order to not be a console app.  This function is
// unused if we are a console application.
int WINAPI WinMain( HINSTANCE, HINSTANCE, LPSTR args, int )
{
    // Do the real work.
    return main( __argc, __argv );
}
#endif // XP_PC && WIN32
