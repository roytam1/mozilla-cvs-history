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
 */

#include "nsIServiceManager.h"
#include "nsIComponentManager.h"
#include "nsIGenericFactory.h"

#include "nsIURI.h"
#include "nsNetUtil.h"
#include "nsIPref.h"
#include "nsIPrefService.h"
#include "plevent.h"
#include "prmem.h"
#include "prnetdb.h"

#include "nsCOMPtr.h"
#include "nsIAppShell.h"
#include "nsICmdLineService.h"
#include "nsIAppShellService.h"
#include "nsIAppShellComponent.h"
#include "nsIAppStartupNotifier.h"
#include "nsIObserverService.h"
#include "nsAppShellCIDs.h"
#include "prprf.h"
#include "nsCRT.h"
#include "nsIDirectoryService.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsSpecialSystemDirectory.h"
#include "nsIWalletService.h"
#include "nsIWindowMediator.h"
#include "nsIDOMWindowInternal.h"
#include "nsIClipboard.h"
#include "nsISupportsPrimitives.h"
#include "nsICmdLineHandler.h"
#include "nsICategoryManager.h"
#include "nsXPIDLString.h"
#include "nsIXULWindow.h"
#include "nsIChromeRegistry.h"
#include "nsIContentHandler.h"
#include "nsIBrowserInstance.h"
#include "nsIEventQueueService.h"
#include "nsMPFileLocProvider.h" 
#include "nsDirectoryServiceDefs.h" 
#include "nsIHttpProtocolHandler.h"
#include "nsBuildID.h"
#include "nsWindowCreator.h"
#include "nsIWindowWatcher.h"
#include "nsProcess.h"

#include "InstallCleanupDefines.h"
#include "nsISoftwareUpdate.h"

// Interfaces Needed
#include "nsIXULWindow.h"
#include "nsIWebBrowserChrome.h"
#include "nsIDocShell.h"

// for X remote support
#ifdef MOZ_ENABLE_XREMOTE
#include "nsXRemoteClientCID.h"
#include "nsIXRemoteClient.h"
#endif

#ifdef NS_TRACE_MALLOC
#include "nsTraceMalloc.h"
#endif

#if defined(DEBUG_sspitzer) || defined(DEBUG_seth)
#define DEBUG_CMD_LINE
#endif

// Standalone App defines
#define STANDALONE_APP_PREF        "profile.standalone_app.enable"
#define STANDALONE_APP_DIR_PREF    "profile.standalone_app.directory"

static NS_DEFINE_CID(kWindowMediatorCID, NS_WINDOWMEDIATOR_CID);
static NS_DEFINE_CID(kIProcessCID, NS_PROCESS_CID); 

extern "C" void ShowOSAlert(char* aMessage);

#define HELP_SPACER_1   "\t"
#define HELP_SPACER_2   "\t\t"
#define HELP_SPACER_4   "\t\t\t\t"
#define PREF_HOMEPAGE_OVERRIDE "browser.startup.homepage_override.1"

#ifdef DEBUG
#include "prlog.h"
#endif

#ifdef MOZ_JPROF
#include "jprof.h"
#endif

// header file for profile manager
#include "nsIProfileInternal.h"

#ifdef _BUILD_STATIC_BIN
#include "nsStaticComponent.h"
nsresult
apprunner_getModuleInfo(nsStaticModuleInfo **info, PRUint32 *count);
#endif

#if defined(XP_UNIX)
  extern void InstallUnixSignalHandlers(const char *ProgramName);
#endif

#if defined(XP_MAC)

#include "macstdlibextras.h"
#include <TextServices.h>

// Set up the toolbox and (if DEBUG) the console.  Do this in a static initializer,
// to make it as unlikely as possible that somebody calls printf() before we get initialized.
static struct MacInitializer { MacInitializer() { InitializeMacToolbox(); } } gInitializer;

// Initialize profile services for both standalone and regular profiles
static nsresult InitializeProfileService(nsICmdLineService *cmdLineArgs);

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

#ifdef MOZ_WIDGET_GTK
#include <gtk/gtk.h>
#endif //MOZ_WIDGET_GTK

/* Define Class IDs */
static NS_DEFINE_CID(kAppShellServiceCID,   NS_APPSHELL_SERVICE_CID);
static NS_DEFINE_CID(kCmdLineServiceCID,    NS_COMMANDLINE_SERVICE_CID);
static char *sWatcherServiceContractID = "@mozilla.org/embedcomp/window-watcher;1";


#include "nsNativeAppSupport.h"

/*********************************************/
// Default implemenations for nativeAppSupport
// If your platform implements these functions if def out this code.
#if !defined (XP_MAC ) && !defined(NTO) && !defined( XP_PC )

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
#if !defined( XP_PC )

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

static nsresult GetNativeAppSupport(nsINativeAppSupport** aNativeApp)
{
    NS_ENSURE_ARG_POINTER(aNativeApp);
    *aNativeApp = nsnull;
    
    nsCOMPtr<nsIAppShellService> appShellService(do_GetService(kAppShellServiceCID));
    if (appShellService)
        appShellService->GetNativeAppSupport(aNativeApp);
    
    return *aNativeApp ? NS_OK : NS_ERROR_FAILURE;
}

static PRBool IsAppInServerMode()
{
    PRBool serverMode = PR_FALSE;
    nsCOMPtr<nsINativeAppSupport> nativeApp;
    GetNativeAppSupport(getter_AddRefs(nativeApp));
    if (nativeApp)
        nativeApp->GetIsServerMode(&serverMode);
        
    return serverMode;
}


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

static nsresult OpenWindow(const char *urlstr, const PRUnichar *args)
{
#ifdef DEBUG_CMD_LINE
  printf("OpenWindow(%s,?)\n",urlstr);
#endif /* DEBUG_CMD_LINE */

  nsCOMPtr<nsIWindowWatcher> wwatch(do_GetService("@mozilla.org/embedcomp/window-watcher;1"));
  nsCOMPtr<nsISupportsWString> sarg(do_CreateInstance(NS_SUPPORTS_WSTRING_CONTRACTID));
  if (!wwatch || !sarg)
    return NS_ERROR_FAILURE;

  sarg->SetData(args);

  nsCOMPtr<nsIDOMWindow> newWindow;
  nsresult rv;
  rv = wwatch->OpenWindow(0, urlstr, "_blank",
                 "chrome,dialog=no,all", sarg,
                 getter_AddRefs(newWindow));

  return rv;
}

static nsresult
OpenChromeURL( const char * urlstr,
               PRInt32 height = NS_SIZETOCONTENT,
               PRInt32 width = NS_SIZETOCONTENT )
{
#ifdef DEBUG_CMD_LINE
    printf("OpenChromeURL(%s,%d,%d)\n",urlstr,height,width);
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
                                      PR_TRUE, PR_TRUE,
                                      nsIWebBrowserChrome::CHROME_ALL,
                                      width, height,
                                      getter_AddRefs(newWindow));
  return rv;
}


static void DumpArbitraryHelp()
{
  nsresult rv;
  nsCOMPtr<nsICategoryManager> catman(do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv));
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

        nsXPIDLCString contractidString;
        rv = catman->GetCategoryEntry(COMMAND_LINE_ARGUMENT_HANDLERS,(const char *)entryString, getter_Copies(contractidString));
        if (NS_FAILED(rv) || !((const char *)contractidString)) break;

#ifdef DEBUG_CMD_LINE
        printf("cmd line handler contractid = %s\n", (const char *)contractidString);
#endif /* DEBUG_CMD_LINE */

        nsCOMPtr <nsICmdLineHandler> handler(do_GetService((const char *)contractidString, &rv));

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
nsresult LaunchApplication(const char *aParam, PRInt32 height, PRInt32 width)
{
  nsresult rv = NS_OK;

  nsCOMPtr <nsICmdLineService> cmdLine =
    do_GetService("@mozilla.org/appshell/commandLineService;1", &rv);
  if (NS_FAILED(rv)) return rv;

  nsCOMPtr <nsICmdLineHandler> handler;
  rv = cmdLine->GetHandlerForParam(aParam, getter_AddRefs(handler));
  if (NS_FAILED(rv)) return rv;

  nsXPIDLCString chromeUrlForTask;
  rv = handler->GetChromeUrlForTask(getter_Copies(chromeUrlForTask));
  if (NS_FAILED(rv)) return rv;

  PRBool handlesArgs = PR_FALSE;
  rv = handler->GetHandlesArgs(&handlesArgs);
  if (handlesArgs) {
    nsXPIDLString defaultArgs;

    /* GetDefaultArgs will always set the override pref to false but we don't want that
       to happen yet (otherwise the setup page won't get displayed when a new
       profile is used for the first time).  So we will fetch the override pref
       before calling GetDefaultArgs and, if it is true, we will restore it to back
       to true after calling GetDefaultArgs
     */
    PRBool override = PR_FALSE;
    nsCOMPtr<nsIPrefBranch> prefBranch;
    nsCOMPtr<nsIPrefService> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
    if (prefs) {
      rv = prefs->GetBranch(nsnull, getter_AddRefs(prefBranch));
      if (NS_SUCCEEDED(rv)) {
        prefBranch->GetBoolPref(PREF_HOMEPAGE_OVERRIDE, &override);
      }
    }

    rv = handler->GetDefaultArgs(getter_Copies(defaultArgs));

    if (override) {
      prefBranch->SetBoolPref(PREF_HOMEPAGE_OVERRIDE, PR_TRUE);
    }

    if (NS_FAILED(rv)) return rv;
    rv = OpenWindow(chromeUrlForTask, defaultArgs);
  }
  else {
    rv = OpenChromeURL(chromeUrlForTask, height, width);
  }

  return rv;
}

static nsresult
LaunchApplicationWithArgs(const char *commandLineArg,
                          nsICmdLineService *cmdLineArgs,
                          const char *aParam,
                          PRInt32 height, PRInt32 width)
{
  NS_ENSURE_ARG(commandLineArg);
  NS_ENSURE_ARG(cmdLineArgs);
  NS_ENSURE_ARG(aParam);

  nsresult rv;
  
  nsCOMPtr<nsICmdLineService> cmdLine =
    do_GetService("@mozilla.org/appshell/commandLineService;1",&rv);
  if (NS_FAILED(rv)) return rv;

  nsCOMPtr <nsICmdLineHandler> handler;
  rv = cmdLine->GetHandlerForParam(aParam, getter_AddRefs(handler));
  if (NS_FAILED(rv)) return rv;

  if (!handler) return NS_ERROR_FAILURE;

  nsXPIDLCString chromeUrlForTask;
  rv = handler->GetChromeUrlForTask(getter_Copies(chromeUrlForTask));
  if (NS_FAILED(rv)) return rv;

#ifdef DEBUG_CMD_LINE
  printf("XXX got this one:\t%s\n\t%s\n\n",commandLineArg,(const char *)chromeUrlForTask);
#endif /* DEBUG_CMD_LINE */

  nsXPIDLCString cmdResult;
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
          nsString cmdArgs; cmdArgs.AssignWithConversion(cmdResult);
#ifdef DEBUG_CMD_LINE
          printf("opening %s with %s\n",(const char *)chromeUrlForTask,"OpenWindow");
#endif /* DEBUG_CMD_LINE */
          rv = OpenWindow(chromeUrlForTask, cmdArgs.GetUnicode());
        }
        else {
#ifdef DEBUG_CMD_LINE
          printf("opening %s with %s\n",(const char *)cmdResult,"OpenChromeURL");
#endif /* DEBUG_CMD_LINE */
          rv = OpenChromeURL(cmdResult,height, width);
          if (NS_FAILED(rv)) return rv;
        }
      }
      else {
        nsXPIDLString defaultArgs;
        rv = handler->GetDefaultArgs(getter_Copies(defaultArgs));
        if (NS_FAILED(rv)) return rv;

        rv = OpenWindow(chromeUrlForTask, defaultArgs);
        if (NS_FAILED(rv)) return rv;
      }
    }
  }
  else {
    if (NS_SUCCEEDED(rv) && (const char*)cmdResult) {
      if (PL_strcmp("1",cmdResult) == 0) {
        rv = OpenChromeURL(chromeUrlForTask,height, width);
        if (NS_FAILED(rv)) return rv;
      }
      else {
        rv = OpenChromeURL(cmdResult, height, width);
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
    // skip past the "general.startup." part of the string
    const char *param = prefName + prefixLen;

#ifdef DEBUG_CMD_LINE
    printf("cmd line parameter = %s\n", param);
#endif /* DEBUG_CMD_LINE */
    rv = LaunchApplication(param, closure->height, closure->width);
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

        // skip over the - (or / on windows)
        char *command = argv[i] + 1;
#ifdef XP_UNIX
        // unix allows -mail and --mail
        if ((argv[i][0] == '-') && (argv[i][1] == '-')) {
          command = argv[i] + 2;
        }
#endif /* XP_UNIX */

        // this can fail, as someone could do -foo, where -foo is not handled
        rv = LaunchApplicationWithArgs((const char *)(argv[i]),
                                       cmdLineArgs, command,
                                       height, width);
      }
    }
  }

  return NS_OK;
}

// This should be done by app shell enumeration someday
static nsresult DoCommandLines(nsICmdLineService* cmdLine, PRBool heedGeneralStartupPrefs)
{
  nsresult rv;
	
  nsCOMPtr<nsIPref> prefs(do_GetService(NS_PREF_CONTRACTID, &rv));
  if (NS_FAILED(rv)) return rv;

  rv = HandleArbitraryStartup(cmdLine, prefs, heedGeneralStartupPrefs);
  return rv;
}

static nsresult DoOnShutdown()
{
  nsresult rv;

  // save the prefs, in case they weren't saved
  {
    // scoping this in a block to force release
    nsCOMPtr<nsIPref> prefs(do_GetService(NS_PREF_CONTRACTID, &rv));
    NS_ASSERTION(NS_SUCCEEDED(rv), "failed to get prefs, so unable to save them");
    if (NS_SUCCEEDED(rv))
      prefs->SavePrefFile(nsnull);
  }

  // at this point, all that is on the clipboard is a proxy object, but that object
  // won't be valid once the app goes away. As a result, we need to force the data
  // out of that proxy and properly onto the clipboard. This can't be done in the
  // clipboard service's shutdown routine because it requires the parser/etc which
  // has already been shutdown by the time the clipboard is shut down.
  {
    // scoping this in a block to force release
    nsCOMPtr<nsIClipboard> clipService(do_GetService("@mozilla.org/widget/clipboard;1", &rv));
    if (NS_SUCCEEDED(rv))
      clipService->ForceDataToClipboard(nsIClipboard::kGlobalClipboard);
  }

  return rv;
}


static nsresult OpenBrowserWindow(PRInt32 height, PRInt32 width)
{
    nsresult rv;
    nsCOMPtr<nsICmdLineHandler> handler(do_GetService(NS_BROWSERSTARTUPHANDLER_CONTRACTID, &rv));
    if (NS_FAILED(rv)) return rv;

    nsXPIDLCString chromeUrlForTask;
    rv = handler->GetChromeUrlForTask(getter_Copies(chromeUrlForTask));
    if (NS_FAILED(rv)) return rv;

    rv = OpenChromeURL(chromeUrlForTask, height, width);
    if (NS_FAILED(rv)) return rv;

    return rv;
}


static void InitCachePrefs()
{
  const char * const CACHE_DIR_PREF = "browser.cache.directory";
  nsresult rv;
  PRBool isDir = PR_FALSE;
  nsCOMPtr<nsIPref> prefs(do_GetService(NS_PREF_CONTRACTID, &rv));
  if (NS_FAILED(rv)) return;
		
  // If the pref is already set don't do anything
  nsCOMPtr<nsILocalFile> cacheDir;
  rv = prefs->GetFileXPref(CACHE_DIR_PREF, getter_AddRefs(cacheDir));
  if (NS_SUCCEEDED(rv) && cacheDir.get()) {
    rv = cacheDir->IsDirectory(&isDir);
    if (NS_SUCCEEDED(rv) && isDir)
      return;
  }
  // Set up the new pref

  nsCOMPtr<nsIFile> profileDir;   
  rv = NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR, getter_AddRefs(profileDir));
  NS_ASSERTION(profileDir, "NS_APP_USER_PROFILE_50_DIR is not defined");
  if (NS_FAILED(rv)) return;

  cacheDir = do_QueryInterface(profileDir);
  NS_ASSERTION(cacheDir, "Cannot get nsILocalFile from cache dir");
    
  PRBool exists;
  cacheDir->Append("Cache");
  rv = cacheDir->Exists(&exists);
  if (NS_SUCCEEDED(rv) && !exists)
    rv = cacheDir->Create(nsIFile::DIRECTORY_TYPE, 0775);
  if (NS_FAILED(rv)) return;

  prefs->SetFileXPref(CACHE_DIR_PREF, cacheDir);
}


static nsresult Ensure1Window( nsICmdLineService* cmdLineArgs)
{
  nsresult rv;
  nsCOMPtr<nsIWindowMediator> windowMediator(do_GetService(kWindowMediatorCID, &rv));

  if (NS_FAILED(rv))
    return rv;

  nsCOMPtr<nsISimpleEnumerator> windowEnumerator;

  if (NS_SUCCEEDED(windowMediator->GetEnumerator(nsnull, getter_AddRefs(windowEnumerator))))
  {
    PRBool more;
	
    windowEnumerator->HasMoreElements(&more);
    if ( !more )
    {
      // If starting up in server mode, then we do things differently.
      nsCOMPtr<nsINativeAppSupport> nativeApp;
      PRBool serverMode;
      rv = GetNativeAppSupport(getter_AddRefs(nativeApp));
      if (NS_SUCCEEDED(rv) && NS_SUCCEEDED(nativeApp->GetIsServerMode(&serverMode)) && serverMode) {
        // Create special Nav window.
        nativeApp->StartServerMode();
        return NS_OK;
      }

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

// Do the righe thing to provide locations depending on whether an
// application is standalone or not 
static nsresult InitializeProfileService(nsICmdLineService *cmdLineArgs)
{
    nsresult rv;

    PRBool standaloneApp = PR_FALSE; 
    nsCOMPtr<nsIPref> prefs(do_GetService(NS_PREF_CONTRACTID, &rv));
    if (NS_FAILED(rv)) return rv; 
    rv = prefs->GetBoolPref(STANDALONE_APP_PREF, &standaloneApp); 
    if (NS_SUCCEEDED(rv) && standaloneApp) 
    { 
        nsMPFileLocProvider *fileLocProvider = new nsMPFileLocProvider; 
        NS_ENSURE_TRUE(fileLocProvider, NS_ERROR_OUT_OF_MEMORY); 
        // Specify the dir that standalone app will use for its "profile" dir 
        nsCOMPtr<nsIFile> parentDir; 

        PRBool exists = PR_FALSE;
#ifdef XP_OS2
        rv = NS_GetSpecialDirectory(NS_OS2_HOME_DIR, getter_AddRefs(parentDir)); 
      
        if (NS_SUCCEEDED(rv))
          rv = parentDir->Exists(&exists);
        if (NS_FAILED(rv) || !exists)
          return rv;
#elif defined(XP_PC)
        rv = NS_GetSpecialDirectory(NS_WIN_APPDATA_DIR, getter_AddRefs(parentDir)); 
        if (NS_SUCCEEDED(rv))
          rv = parentDir->Exists(&exists);
        if (NS_FAILED(rv) || !exists)
          {
            parentDir = nsnull;
            rv = NS_GetSpecialDirectory(NS_WIN_WINDOWS_DIR, getter_AddRefs(parentDir));
          }
        
        if (NS_FAILED(rv)) 
          return rv;
#else
      rv = NS_GetSpecialDirectory(NS_OS_HOME_DIR, getter_AddRefs(parentDir)); 
      
      if (NS_SUCCEEDED(rv))
        rv = parentDir->Exists(&exists);
      if (NS_FAILED(rv) || !exists)
        return rv;
#endif

        // Get standalone app dir name from prefs and initialize mpfilelocprovider
        nsXPIDLCString appDir;
        rv = prefs->CopyCharPref(STANDALONE_APP_DIR_PREF, getter_Copies(appDir));
        if (NS_FAILED(rv) || (nsCRT::strlen(appDir) == 0)) 
            return NS_ERROR_FAILURE; 
        rv = fileLocProvider->Initialize(parentDir, appDir); 
        if (NS_FAILED(rv)) return rv; 

        rv = prefs->ResetPrefs(); 
        if (NS_FAILED(rv)) return rv; 
        rv = prefs->ReadConfigFile(); 
        if (NS_FAILED(rv)) return rv; 
        rv = prefs->ReadUserPrefs(nsnull); 
        if (NS_FAILED(rv)) return rv; 
    } 
    else 
    {
        rv = prefs->ReadConfigFile(); 
        if (NS_FAILED(rv)) return rv; 

        nsCOMPtr<nsIProfileInternal> profileMgr(do_GetService(NS_PROFILE_CONTRACTID, &rv));
        NS_ASSERTION(NS_SUCCEEDED(rv), "failed to get profile manager");
        if (NS_FAILED(rv)) return rv;

        nsCOMPtr<nsINativeAppSupport> nativeApp;
        PRBool serverMode = PR_FALSE;
        GetNativeAppSupport(getter_AddRefs(nativeApp));
        if (nativeApp)
            nativeApp->GetIsServerMode(&serverMode);
            
        // If we are in server mode, profile mgr cannot show UI
        rv = profileMgr->StartupWithArgs(cmdLineArgs, !serverMode);
        NS_ASSERTION(NS_SUCCEEDED(rv), "StartupWithArgs failed\n");
        if (serverMode && rv == NS_ERROR_PROFILE_REQUIRES_INTERACTION) {
            nativeApp->SetNeedsProfileUI(PR_TRUE);
            rv = NS_OK;
        } 
        else if (NS_FAILED(rv)) return rv;

        // if we get here, and we don't have a current profile, return a failure so we will exit
        // this can happen, if the user hits Cancel or Exit in the profile manager dialogs
        nsXPIDLString currentProfileStr;
        rv = profileMgr->GetCurrentProfile(getter_Copies(currentProfileStr));
        if (NS_FAILED(rv) || !((const PRUnichar *)currentProfileStr) ||
                            (nsCRT::strlen((const PRUnichar *)currentProfileStr) == 0)) {
  	    return NS_ERROR_FAILURE;
        }
    }
    return rv;
}

static nsresult InitializeWindowCreator()
{
  // create an nsWindowCreator and give it to the WindowWatcher service
  nsWindowCreator *creatorCallback = new nsWindowCreator();
  if (!creatorCallback)
    return NS_ERROR_OUT_OF_MEMORY;

  nsCOMPtr<nsIWindowCreator> windowCreator(NS_STATIC_CAST(nsIWindowCreator *, creatorCallback));
  if (windowCreator) {
    nsCOMPtr<nsIWindowWatcher> wwatch(do_GetService(sWatcherServiceContractID));
    if (wwatch) {
      wwatch->SetWindowCreator(windowCreator);
      return NS_OK;
    }
  }
  return NS_ERROR_FAILURE;
}

static nsresult VerifyInstallation(int argc, char **argv)
{
  nsresult rv;
  nsCOMPtr<nsILocalFile> registryFile;

  NS_WITH_SERVICE(nsIProperties, directoryService, NS_DIRECTORY_SERVICE_CONTRACTID, &rv);
  if (NS_FAILED(rv))
    return NS_OK;
  rv = directoryService->Get(NS_OS_CURRENT_PROCESS_DIR, 
                             NS_GET_IID(nsIFile), 
                             getter_AddRefs(registryFile));
  if (NS_FAILED(rv) || !registryFile)
    return NS_OK;

#ifdef XP_MAC
  registryFile->Append(ESSENTIAL_FILES);
#endif
  registryFile->Append(CLEANUP_REGISTRY);

  PRBool exists;
  registryFile->Exists(&exists);
  if (exists)
  {
    nsCOMPtr<nsIFile> binPath;
    char cleanupMessage[256];
    char* lastResortMessage = "A previous install did not complete correctly.  Finishing install.";
    PRInt32 numRead;

    registryFile->Clone(getter_AddRefs(binPath));
    nsCOMPtr<nsILocalFile>cleanupMessageFile = do_QueryInterface(binPath, &rv);
#ifdef XP_MAC
    nsCOMPtr<nsIFile> messageFileParent;
    cleanupMessageFile->GetParent(getter_AddRefs(messageFileParent));
    cleanupMessageFile = do_QueryInterface(messageFileParent, &rv);
#endif
    cleanupMessageFile->SetLeafName("res");
    cleanupMessageFile->Append(CLEANUP_MESSAGE_FILENAME);
  
    PRFileDesc* fd;
    cleanupMessageFile->OpenNSPRFileDesc(PR_RDONLY, 0664, &fd);
    if (fd)
    {
      numRead = PR_Read(fd, cleanupMessage, sizeof(cleanupMessage));
      if (numRead > 0)
        cleanupMessage[numRead] = 0;
      else
      {
        //Something was wrong with the translated message file. empty? 
        strcpy(cleanupMessage, lastResortMessage);
      }
    }
    else
    {
      //Couldn't open the translated message file
      strcpy(cleanupMessage, lastResortMessage);
    }
    //The cleanup registry file exists so we have cleanup work to do
#ifdef MOZ_WIDGET_GTK
    gtk_init(&argc, &argv);
#endif
    ShowOSAlert(cleanupMessage);

    nsCOMPtr<nsIFile> cleanupUtility;
    registryFile->Clone(getter_AddRefs(cleanupUtility));
    cleanupUtility->SetLeafName(CLEANUP_UTIL);

    //Create the process framework to run the cleanup utility
    nsCOMPtr<nsIProcess> cleanupProcess = do_CreateInstance(kIProcessCID);
    rv = cleanupProcess->Init(cleanupUtility);
    if (NS_SUCCEEDED(rv))
      rv = cleanupProcess->Run(PR_FALSE,nsnull, 0, nsnull);
    
    //We must exit because all open files must be released by the system
    return NS_ERROR_FAILURE;
  }
  return NS_OK;
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

  //----------------------------------------------------------------
  // First we need to check if a previous installation occured and
  // if so, make sure it finished and cleaned up correctly.
  //
  // If there is an xpicleanup.dat file left around, that means the
  // previous installation did not finish correctly. We must cleanup
  // before a valid mozilla can run.
  //
  // Show the user a platform-specific Alert message, then spawn the
  // xpicleanup utility, then exit.
  //----------------------------------------------------------------
  rv = VerifyInstallation(argc, argv);
  if (NS_FAILED(rv))
    return NS_ERROR_FAILURE;

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

  nsCOMPtr<nsIEventQueueService> eventQService(do_GetService(NS_EVENTQUEUESERVICE_CONTRACTID, &rv));
  if (NS_SUCCEEDED(rv)) {
    // XXX: What if this fails?
    rv = eventQService->CreateThreadEventQueue();
  }


  // Setup an autoreg obserer, so that we can update a progress
  // string in the splash screen
  nsCOMPtr<nsIObserverService> obsService(do_GetService(NS_OBSERVERSERVICE_CONTRACTID));
  if (obsService)
  {
    nsCOMPtr<nsIObserver> splashScreenObserver(do_QueryInterface(nativeApp));
    if (splashScreenObserver)
    {
      obsService->AddObserver(splashScreenObserver, NS_ConvertASCIItoUCS2(NS_XPCOM_AUTOREGISTRATION_OBSERVER_ID).get());
    }
  }

#if XP_MAC
  stTSMCloser  tsmCloser;

  rv = InitializeMacCommandLine( argc, argv);
  NS_ASSERTION(NS_SUCCEEDED(rv), "Initializing AppleEvents failed");
#endif

  // Ask XPInstall if we need to autoregister anything new.
  PRBool needAutoReg = NS_SoftwareUpdateNeedsAutoReg();

#ifdef DEBUG
  // _Always_ autoreg if we're in a debug build, under the assumption
  // that people are busily modifying components and will be angry if
  // their changes aren't noticed.
  needAutoReg = PR_TRUE;
#endif

  NS_SetupRegistry_1(needAutoReg);

  if (needAutoReg)  // XXX ...and autoreg was successful?
    NS_SoftwareUpdateDidAutoReg();

  // remove the nativeApp as an XPCOM autoreg observer
  if (obsService)
  {
    nsCOMPtr<nsIObserver> splashScreenObserver(do_QueryInterface(nativeApp));
    if (splashScreenObserver)
    {
      obsService->RemoveObserver(splashScreenObserver, NS_ConvertASCIItoUCS2(NS_XPCOM_AUTOREGISTRATION_OBSERVER_ID).get());
    }
  }

  // Start up the core services:

  // Please do not add new things to main1() - please hook into the
  // nsIAppStartupNotifier service.
  nsCOMPtr<nsIObserver> startupNotifier = do_CreateInstance(NS_APPSTARTUPNOTIFIER_CONTRACTID, &rv);
  if(NS_FAILED(rv))
    return rv;
  startupNotifier->Observe(nsnull, APPSTARTUP_TOPIC, nsnull);

  
  // Initialize the cmd line service
  nsCOMPtr<nsICmdLineService> cmdLineArgs(do_GetService(kCmdLineServiceCID, &rv));
  NS_ASSERTION(NS_SUCCEEDED(rv), "failed to get command line service");

  if (NS_FAILED(rv)) {
    NS_ASSERTION(PR_FALSE, "Could not obtain CmdLine processing service\n");
    return rv;
  }

  rv = cmdLineArgs->Initialize(argc, argv);
  NS_ASSERTION(NS_SUCCEEDED(rv), "failed to initialize command line args");
  if (rv == NS_ERROR_INVALID_ARG) {
    PrintUsage();
    return rv;
  }

  nsCOMPtr<nsIAppShellService> appShell(do_GetService(kAppShellServiceCID, &rv));
  NS_ASSERTION(NS_SUCCEEDED(rv), "failed to get the appshell service");

  /* if we couldn't get the nsIAppShellService service, then we should hide the
     splash screen and return */
  if (NS_FAILED(rv)) {
    // See if platform supports nsINativeAppSupport.
    nsCOMPtr<nsINativeAppSupport> nativeAppSupport(do_QueryInterface(nativeApp));
    if (nativeAppSupport) {
      // Use that interface to remove splash screen.
      nativeAppSupport->HideSplashScreen();
    } else {
      // See if platform supports nsISplashScreen, instead.
      nsCOMPtr<nsISplashScreen> splashScreen(do_QueryInterface(nativeApp));
      if (splashScreen) {
        splashScreen->Hide();
      }
    }

    // Release argument object.
    NS_IF_RELEASE(nativeApp);
    return rv;
  }


  // Create the Application Shell instance...
  rv = appShell->Initialize(cmdLineArgs, nativeApp);

  // We are done with the native app (or splash screen) object here;
  // the app shell owns it now.
  NS_IF_RELEASE(nativeApp);

  NS_ASSERTION(NS_SUCCEEDED(rv), "failed to initialize appshell");
  if (NS_FAILED(rv)) return rv;
  
  rv = InitializeWindowCreator();
  NS_ASSERTION(NS_SUCCEEDED(rv), "failed to initialize window creator");
  if (NS_FAILED(rv)) return rv;

  // So we can open and close windows during startup
  appShell->SetQuitOnLastWindowClosing(PR_FALSE);

  // Initialize Profile Service here.
  rv = InitializeProfileService(cmdLineArgs);
  if (NS_FAILED(rv)) return rv;

  // Enumerate AppShellComponenets
  appShell->EnumerateAndInitializeComponents();

  // rjc: now must explicitly call appshell's CreateHiddenWindow() function AFTER profile manager.
  //      if the profile manager ever switches to using nsIDOMWindowInternal stuff, this might have to change
  appShell->CreateHiddenWindow();

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

  // Startup wallet service so it registers for notifications
  nsCOMPtr<nsIWalletService> walletService(do_GetService(NS_WALLETSERVICE_CONTRACTID, &rv));
                  
  InitCachePrefs();

  // From this point on, should be true
  appShell->SetQuitOnLastWindowClosing(PR_TRUE);	
  // Start main event loop
  rv = appShell->Run();
  NS_ASSERTION(NS_SUCCEEDED(rv), "failed to run appshell");

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

#ifdef MOZ_WIDGET_GTK
  /* insert gtk options above moz options, like any other gtk app
   *
   * note: this isn't a very cool way to do things -- i'd rather get
   * these straight from a user's gtk version -- but it seems to be
   * what most gtk apps do. -dr
   */

  printf("GTK options\n");
  printf("%s--gdk-debug=FLAGS%sGdk debugging flags to set\n", HELP_SPACER_1, HELP_SPACER_2);
  printf("%s--gdk-no-debug=FLAGS%sGdk debugging flags to unset\n", HELP_SPACER_1, HELP_SPACER_2);
  printf("%s--gtk-debug=FLAGS%sGtk+ debugging flags to set\n", HELP_SPACER_1, HELP_SPACER_2);
  printf("%s--gtk-no-debug=FLAGS%sGtk+ debugging flags to unset\n", HELP_SPACER_1, HELP_SPACER_2);
  printf("%s--gtk-module=MODULE%sLoad an additional Gtk module\n", HELP_SPACER_1, HELP_SPACER_2);

  /* end gtk options */
#endif
#ifdef MOZ_X11
  printf("X11 options\n");
  printf("%s--display=DISPLAY%sX display to use\n", HELP_SPACER_1, HELP_SPACER_2);
  printf("%s--sync%sMake X calls synchronous\n", HELP_SPACER_1, HELP_SPACER_2);
  printf("%s--no-xshm%sDon't use X shared memory extension\n", HELP_SPACER_1, HELP_SPACER_2);
  printf("%s--xim-preedit=STYLE\n", HELP_SPACER_1);
  printf("%s--xim-status=STYLE\n", HELP_SPACER_1);
#endif
#ifdef XP_UNIX
  printf("%s--g-fatal-warnings%sMake all warnings fatal\n", HELP_SPACER_1, HELP_SPACER_2);

  printf("\nMozilla options\n");
#endif

  printf("%s-height <value>%sSet height of startup window to <value>.\n",HELP_SPACER_1,HELP_SPACER_2);
  printf("%s-h or -help%sPrint this message.\n",HELP_SPACER_1,HELP_SPACER_2);
  printf("%s-installer%sStart with 4.x migration window.\n",HELP_SPACER_1,HELP_SPACER_2);
  printf("%s-width <value>%sSet width of startup window to <value>.\n",HELP_SPACER_1,HELP_SPACER_2);
  printf("%s-v or -version%sPrint %s version.\n",HELP_SPACER_1,HELP_SPACER_2, appname);
  printf("%s-CreateProfile <profile>%sCreate <profile>.\n",HELP_SPACER_1,HELP_SPACER_2);
  printf("%s-P <profile>%sStart with <profile>.\n",HELP_SPACER_1,HELP_SPACER_2);
  printf("%s-ProfileWizard%sStart with profile wizard.\n",HELP_SPACER_1,HELP_SPACER_2);
  printf("%s-ProfileManager%sStart with profile manager.\n",HELP_SPACER_1,HELP_SPACER_2);
  printf("%s-SelectProfile%sStart with profile selection dialog.\n",HELP_SPACER_1,HELP_SPACER_2);
  printf("%s-lang <lang-region>%sStart with <lang-region> resources.\n",HELP_SPACER_1,HELP_SPACER_2);
#ifdef XP_WIN32
  printf("%s-console%sStart Mozilla with a debugging console.\n",HELP_SPACER_1,HELP_SPACER_2);
#endif
#ifdef MOZ_ENABLE_XREMOTE
  printf("%s-remote <command>%sExecute <command> in an already running\n"
         "%sMozilla process.  For more info, see:\n"
         "\n%shttp://www.mozilla.org/unix/remote.html\n\n",
         HELP_SPACER_1,HELP_SPACER_1,HELP_SPACER_4,HELP_SPACER_2);
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


// Print out user agent from the HTTP Handler service,
// and the Build ID from nsBuildID.h.
static nsresult DumpVersion(char *appname)
{
  nsresult rv = NS_OK;
  long buildID = NS_BUILD_ID;  // 10-digit number

  // Get httpHandler service.
  nsCOMPtr <nsIHttpProtocolHandler> httpHandler(do_GetService("@mozilla.org/network/protocol;1?name=http", &rv));
  NS_ENSURE_SUCCESS(rv,rv);

  nsXPIDLCString agent;
  httpHandler->GetUserAgent(getter_Copies(agent));
  
  printf("%s", agent.get());

  if(buildID) {
    printf(", build %u\n", (unsigned int)buildID);
  } else {
    printf(" <developer build>\n");
  }
  
  return rv;
}

#ifdef MOZ_ENABLE_XREMOTE
// use int here instead of a PR type since it will be returned
// from main - just to keep types consistent
static int HandleRemoteArguments(int argc, char* argv[], PRBool *aArgUsed)
{
  int i = 0;
  for (i=1; i < argc; i++) {
    if (PL_strcasecmp(argv[i], "-remote") == 0) {
      // someone used a -remote flag
      *aArgUsed = PR_TRUE;
      // check to make sure there's another arg
      if (argc-1 == i) {
        PR_fprintf(PR_STDERR, "-remote requires an argument\n");
        return 1;
      }
      // try to get the X remote client
      nsCOMPtr<nsIXRemoteClient> client (do_CreateInstance(NS_XREMOTECLIENT_CONTRACTID));
      if (!client)
        return 1;
      nsresult rv;
      // try to init - connects to the X server and stuff
      rv = client->Init();
      if (NS_FAILED(rv)) {
        PR_fprintf(PR_STDERR, "Failed to connect to X server.\n");
        return 1;
      }
      PRBool success = PR_FALSE;
      rv = client->SendCommand(argv[i+1], &success);
      // did the command fail?
      if (NS_FAILED(rv)) {
        PR_fprintf(PR_STDERR, "Failed to send command.\n");
        return 1;
      }
      // was there a window not running?
      if (!success) {
        PR_fprintf(PR_STDERR, "No running window found.\n");
        return 2;
      }
      client->Shutdown();
      // success
      return 0;
    }
  }
  return 0;
}
#endif /* XP_UNIX */

static PRBool HandleDumpArguments(int argc, char* argv[])
{
  for (int i=1; i<argc; i++) {
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
  // They should% return quick, so we deal with them here.
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

#ifdef _BUILD_STATIC_BIN
  // Initialize XPCOM's module info table
  NSGetStaticModuleInfo = apprunner_getModuleInfo;
#endif

  rv = NS_InitXPCOM(NULL, NULL);
  NS_ASSERTION( NS_SUCCEEDED(rv), "NS_InitXPCOM failed" );

#ifdef MOZ_ENABLE_XREMOTE
  // handle -remote now that xpcom is fired up
  int remoterv;
  PRBool argused = PR_FALSE;
  // argused will be true if someone tried to use a -remote flag.  We
  // always exit in that case.
  remoterv = HandleRemoteArguments(argc, argv, &argused);
  if (argused) {
    NS_ShutdownXPCOM(NULL);
    return remoterv;
  }
#endif

  nsresult mainResult = main1(argc, argv, nativeApp ? (nsISupports*)nativeApp : (nsISupports*)splash);

  /* if main1() didn't succeed, then don't bother trying to shut down clipboard, etc */
  if (NS_SUCCEEDED(mainResult)) {
    rv = DoOnShutdown();
    NS_ASSERTION(NS_SUCCEEDED(rv), "DoOnShutdown failed");
  }

  rv = NS_ShutdownXPCOM( NULL );
  NS_ASSERTION(NS_SUCCEEDED(rv), "NS_ShutdownXPCOM failed");

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
