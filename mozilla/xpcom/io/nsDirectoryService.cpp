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
 *     IBM Corp.
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

#include "nsCOMPtr.h"
#include "nsDirectoryService.h"
#include "nsDirectoryServiceDefs.h"
#include "nsLocalFile.h"
#include "nsDebug.h"

#if defined(XP_MAC)
#include <Folders.h>
#include <Files.h>
#include <Memory.h>
#include <Processes.h>
#include <Gestalt.h>
#elif defined(XP_WIN)
#include <windows.h>
#include <shlobj.h>
#include <stdlib.h>
#include <stdio.h>
#elif defined(XP_UNIX) || defined(XP_MACOSX)
#include <unistd.h>
#include <stdlib.h>
#include <sys/param.h>
#include "prenv.h"
#ifdef XP_MACOSX
#include <CoreServices/CoreServices.h>
#include <Folders.h>
#include <Files.h>
#include <Memory.h>
#include <Processes.h>
#include <Gestalt.h>
#include <CFURL.h>
#include <InternetConfig.h>
#endif
#elif defined(XP_OS2)
#define MAX_PATH _MAX_PATH
#elif defined(XP_BEOS)
#include <FindDirectory.h>
#include <Path.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/param.h>
#include <OS.h>
#include <image.h>
#include "prenv.h"
#endif

#include "nsSpecialSystemDirectory.h"
#include "nsAppFileLocationProvider.h"

#if defined(XP_MAC)
#define COMPONENT_REGISTRY_NAME NS_LITERAL_CSTRING("Component Registry")
#define COMPONENT_DIRECTORY     NS_LITERAL_CSTRING("Components")
#else
#define COMPONENT_REGISTRY_NAME NS_LITERAL_CSTRING("component.reg")
#define COMPONENT_DIRECTORY     NS_LITERAL_CSTRING("components")
#endif 

#if defined(XP_MAC)
#define APP_REGISTRY_NAME "Application Registry"
#elif defined(XP_WIN) || defined(XP_OS2)
#define APP_REGISTRY_NAME "registry.dat"
#else
#define APP_REGISTRY_NAME "appreg"
#endif 

// define home directory
// For Windows platform, We are choosing Appdata folder as HOME
#if defined (XP_WIN)
#define HOME_DIR NS_WIN_APPDATA_DIR
#elif defined (XP_MAC) || defined (XP_MACOSX)
#define HOME_DIR NS_MAC_HOME_DIR
#elif defined (XP_UNIX)
#define HOME_DIR NS_UNIX_HOME_DIR
#elif defined (XP_OS2)
#define HOME_DIR NS_OS2_HOME_DIR
#elif defined (XP_BEOS)
#define HOME_DIR NS_BEOS_HOME_DIR
#endif

// define default product directory
#if defined(XP_WIN) || defined(XP_MAC) || defined(XP_OS2) || defined(XP_BEOS)
#define DEFAULT_PRODUCT_DIR "Mozilla"
#elif defined (XP_UNIX) || defined(XP_MACOSX)
#define DEFAULT_PRODUCT_DIR ".mozilla"
#endif

//----------------------------------------------------------------------------------------
nsresult 
nsDirectoryService::GetCurrentProcessDirectory(nsILocalFile** aFile)
//----------------------------------------------------------------------------------------
{
    NS_ENSURE_ARG_POINTER(aFile);
    *aFile = nsnull;
    
   //  Set the component registry location:
    if (!mService)
        return NS_ERROR_FAILURE;

    nsresult rv; 
 
    nsCOMPtr<nsIProperties> dirService;
    rv = nsDirectoryService::Create(nsnull, 
                                    NS_GET_IID(nsIProperties), 
                                    getter_AddRefs(dirService));  // needs to be around for life of product

    if (dirService)
    {
      nsCOMPtr <nsILocalFile> aLocalFile;
      dirService->Get(NS_XPCOM_INIT_CURRENT_PROCESS_DIR, NS_GET_IID(nsILocalFile), getter_AddRefs(aLocalFile));
      if (aLocalFile)
      {
        *aFile = aLocalFile;
        NS_ADDREF(*aFile);
        return NS_OK;
      }
    }

    nsLocalFile* localFile = new nsLocalFile;

    if (localFile == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(localFile);



#ifdef XP_WIN
    char buf[MAX_PATH];
    if ( ::GetModuleFileName(0, buf, sizeof(buf)) ) {
        // chop of the executable name by finding the rightmost backslash
        char* lastSlash = PL_strrchr(buf, '\\');
        if (lastSlash)
            *(lastSlash + 1) = '\0';
        
        localFile->InitWithNativePath(nsDependentCString(buf));
        *aFile = localFile;
        return NS_OK;
    }

#elif defined(XP_MAC)
    // get info for the the current process to determine the directory
    // its located in
    OSErr err;
    ProcessSerialNumber psn = {kNoProcess, kCurrentProcess};
    ProcessInfoRec pInfo;
    FSSpec         tempSpec;

    // initialize ProcessInfoRec before calling
    // GetProcessInformation() or die horribly.
    pInfo.processName = nil;
    pInfo.processAppSpec = &tempSpec;
    pInfo.processInfoLength = sizeof(ProcessInfoRec);

    err = GetProcessInformation(&psn, &pInfo);
    if (!err)
    {
        // create an FSSpec from the volume and dirid of the app.
        FSSpec appFSSpec;
        ::FSMakeFSSpec(pInfo.processAppSpec->vRefNum, pInfo.processAppSpec->parID, 0, &appFSSpec);
        
        nsCOMPtr<nsILocalFileMac> localFileMac = do_QueryInterface((nsIFile*)localFile);
        if (localFileMac) 
        {
            localFileMac->InitWithFSSpec(&appFSSpec);
            *aFile = localFile;
            return NS_OK;
        }
    }
#elif defined(XP_MACOSX)
    // Works even if we're not bundled.
    CFBundleRef appBundle = CFBundleGetMainBundle();
    if (appBundle != nsnull)
    {
        CFURLRef bundleURL = CFBundleCopyExecutableURL(appBundle);
        if (bundleURL != nsnull)
        {
            CFURLRef parentURL = CFURLCreateCopyDeletingLastPathComponent(kCFAllocatorDefault, bundleURL);
            if (parentURL)
            {
                CFStringRef path = CFURLCopyFileSystemPath(parentURL, kCFURLPOSIXPathStyle);
                if (path)
                {
                    char buffer[512];
                    if (CFStringGetCString(path, buffer, sizeof(buffer), kCFStringEncodingUTF8))
                    {
#ifdef DEBUG_conrad
                        printf("nsDirectoryService - CurrentProcessDir is: %s\n", buffer);
#endif
                        rv = localFile->InitWithNativePath(nsDependentCString(buffer));
                        if (NS_SUCCEEDED(rv))
                            *aFile = localFile;
                    }
                    CFRelease(path);
                }
                CFRelease(parentURL);
            }
            CFRelease(bundleURL);
        }
    }
    
    NS_ASSERTION(*aFile, "nsDirectoryService - Could not determine CurrentProcessDir.\n");  
    if (*aFile)
        return NS_OK;

#elif defined(XP_UNIX)

    // In the absence of a good way to get the executable directory let
    // us try this for unix:
    //	- if MOZILLA_FIVE_HOME is defined, that is it
    //	- else give the current directory
    char buf[MAXPATHLEN];

    // The MOZ_DEFAULT_MOZILLA_FIVE_HOME variable can be set at configure time with
    // a --with-default-mozilla-five-home=foo autoconf flag.
    // 
    // The idea here is to allow for builds that have a default MOZILLA_FIVE_HOME
    // regardless of the environment.  This makes it easier to write apps that
    // embed mozilla without having to worry about setting up the environment 
    //
    // We do this py putenv()ing the default value into the environment.  Note that
    // we only do this if it is not already set.
#ifdef MOZ_DEFAULT_MOZILLA_FIVE_HOME
    if (PR_GetEnv("MOZILLA_FIVE_HOME") == nsnull)
    {
        putenv("MOZILLA_FIVE_HOME=" MOZ_DEFAULT_MOZILLA_FIVE_HOME);
    }
#endif

    char *moz5 = PR_GetEnv("MOZILLA_FIVE_HOME");

    if (moz5)
    {
        localFile->InitWithNativePath(nsDependentCString(moz5));
        localFile->Normalize();
        *aFile = localFile;
        return NS_OK;
    }
    else
    {
        static PRBool firstWarning = PR_TRUE;

        if(firstWarning) {
            // Warn that MOZILLA_FIVE_HOME not set, once.
            printf("Warning: MOZILLA_FIVE_HOME not set.\n");
            firstWarning = PR_FALSE;
        }

        // Fall back to current directory.
        if (getcwd(buf, sizeof(buf)))
        {
            localFile->InitWithNativePath(nsDependentCString(buf));
            *aFile = localFile;
            return NS_OK;
        }
    }

#elif defined(XP_OS2)
    PPIB ppib;
    PTIB ptib;
    char buffer[CCHMAXPATH];
    DosGetInfoBlocks( &ptib, &ppib);
    DosQueryModuleName( ppib->pib_hmte, CCHMAXPATH, buffer);
    *strrchr( buffer, '\\') = '\0'; // XXX DBCS misery
    localFile->InitWithNativePath(nsDependentCString(buffer));
    *aFile = localFile;
    return NS_OK;

#elif defined(XP_BEOS)

    char *moz5 = getenv("MOZILLA_FIVE_HOME");
    if (moz5)
    {
        localFile->InitWithNativePath(nsDependentCString(moz5));
        localFile->Normalize();
        *aFile = localFile;
        return NS_OK;
    }
    else
    {
      static char buf[MAXPATHLEN];
      int32 cookie = 0;
      image_info info;
      char *p;
      *buf = 0;
      if(get_next_image_info(0, &cookie, &info) == B_OK)
      {
        strcpy(buf, info.name);
        if((p = strrchr(buf, '/')) != 0)
        {
          *p = 0;
          localFile->InitWithNativePath(nsDependentCString(buf));
          *aFile = localFile;
          return NS_OK;
        }
      }
    }

#endif
    
    if (localFile)
       delete localFile;

    NS_ERROR("unable to get current process directory");
    return NS_ERROR_FAILURE;
} // GetCurrentProcessDirectory()


nsIAtom*  nsDirectoryService::sCurrentProcess = nsnull;
nsIAtom*  nsDirectoryService::sComponentRegistry = nsnull;
nsIAtom*  nsDirectoryService::sComponentDirectory = nsnull;
nsIAtom*  nsDirectoryService::sGRE_Directory = nsnull;
nsIAtom*  nsDirectoryService::sGRE_ComponentDirectory = nsnull;
nsIAtom*  nsDirectoryService::sOS_DriveDirectory = nsnull;
nsIAtom*  nsDirectoryService::sOS_TemporaryDirectory = nsnull;
nsIAtom*  nsDirectoryService::sOS_CurrentProcessDirectory = nsnull;
nsIAtom*  nsDirectoryService::sOS_CurrentWorkingDirectory = nsnull;
#if defined(XP_MAC)
nsIAtom*  nsDirectoryService::sDirectory = nsnull;
nsIAtom*  nsDirectoryService::sDesktopDirectory = nsnull;
nsIAtom*  nsDirectoryService::sTrashDirectory = nsnull;
nsIAtom*  nsDirectoryService::sStartupDirectory = nsnull;
nsIAtom*  nsDirectoryService::sShutdownDirectory = nsnull;
nsIAtom*  nsDirectoryService::sAppleMenuDirectory = nsnull;
nsIAtom*  nsDirectoryService::sControlPanelDirectory = nsnull;
nsIAtom*  nsDirectoryService::sExtensionDirectory = nsnull;
nsIAtom*  nsDirectoryService::sFontsDirectory = nsnull;
nsIAtom*  nsDirectoryService::sClassicPreferencesDirectory = nsnull;
nsIAtom*  nsDirectoryService::sPreferencesDirectory = nsnull;
nsIAtom*  nsDirectoryService::sDocumentsDirectory = nsnull;
nsIAtom*  nsDirectoryService::sInternetSearchDirectory = nsnull;
nsIAtom*  nsDirectoryService::sHomeDirectory = nsnull;
nsIAtom*  nsDirectoryService::sDefaultDownloadDirectory = nsnull;
nsIAtom*  nsDirectoryService::sUserLibDirectory = nsnull;
#elif defined (XP_MACOSX)
nsIAtom*  nsDirectoryService::sHomeDirectory = nsnull;
nsIAtom*  nsDirectoryService::sDefaultDownloadDirectory = nsnull;
#elif defined (XP_WIN) 
nsIAtom*  nsDirectoryService::sSystemDirectory = nsnull;
nsIAtom*  nsDirectoryService::sWindowsDirectory = nsnull;
nsIAtom*  nsDirectoryService::sHomeDirectory = nsnull;
nsIAtom*  nsDirectoryService::sDesktop = nsnull;
nsIAtom*  nsDirectoryService::sPrograms = nsnull;
nsIAtom*  nsDirectoryService::sControls = nsnull;
nsIAtom*  nsDirectoryService::sPrinters = nsnull;
nsIAtom*  nsDirectoryService::sPersonal = nsnull;
nsIAtom*  nsDirectoryService::sFavorites = nsnull;
nsIAtom*  nsDirectoryService::sStartup = nsnull;
nsIAtom*  nsDirectoryService::sRecent = nsnull;
nsIAtom*  nsDirectoryService::sSendto = nsnull;
nsIAtom*  nsDirectoryService::sBitbucket = nsnull;
nsIAtom*  nsDirectoryService::sStartmenu = nsnull;
nsIAtom*  nsDirectoryService::sDesktopdirectory = nsnull;
nsIAtom*  nsDirectoryService::sDrives = nsnull;
nsIAtom*  nsDirectoryService::sNetwork = nsnull;
nsIAtom*  nsDirectoryService::sNethood = nsnull;
nsIAtom*  nsDirectoryService::sFonts = nsnull;
nsIAtom*  nsDirectoryService::sTemplates = nsnull;
nsIAtom*  nsDirectoryService::sCommon_Startmenu = nsnull;
nsIAtom*  nsDirectoryService::sCommon_Programs = nsnull;
nsIAtom*  nsDirectoryService::sCommon_Startup = nsnull;
nsIAtom*  nsDirectoryService::sCommon_Desktopdirectory = nsnull;
nsIAtom*  nsDirectoryService::sAppdata = nsnull;
nsIAtom*  nsDirectoryService::sPrinthood = nsnull;
#elif defined (XP_UNIX)
nsIAtom*  nsDirectoryService::sLocalDirectory = nsnull;
nsIAtom*  nsDirectoryService::sLibDirectory = nsnull;
nsIAtom*  nsDirectoryService::sHomeDirectory = nsnull;
#elif defined (XP_OS2)
nsIAtom*  nsDirectoryService::sSystemDirectory = nsnull;
nsIAtom*  nsDirectoryService::sOS2Directory = nsnull;
nsIAtom*  nsDirectoryService::sHomeDirectory = nsnull;
nsIAtom*  nsDirectoryService::sDesktopDirectory = nsnull;
#elif defined (XP_BEOS)
nsIAtom*  nsDirectoryService::sSettingsDirectory = nsnull;
nsIAtom*  nsDirectoryService::sHomeDirectory = nsnull;
nsIAtom*  nsDirectoryService::sDesktopDirectory = nsnull;
nsIAtom*  nsDirectoryService::sSystemDirectory = nsnull;
#endif


nsDirectoryService* nsDirectoryService::mService = nsnull;

nsDirectoryService::nsDirectoryService()
{
    NS_INIT_ISUPPORTS();
}

NS_METHOD
nsDirectoryService::Create(nsISupports *outer, REFNSIID aIID, void **aResult)
{
    NS_ENSURE_ARG_POINTER(aResult);
    if (mService == nsnull)
    {
        mService = new nsDirectoryService();
        if (mService == NULL)
            return NS_ERROR_OUT_OF_MEMORY;
    }
    return mService->QueryInterface(aIID, aResult);
}

nsresult
nsDirectoryService::Init()
{
    nsresult rv;
    mHashtable = new nsSupportsHashtable(256, PR_TRUE);
    if (mHashtable == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;
      
    rv = NS_NewISupportsArray(getter_AddRefs(mProviders));
    if (NS_FAILED(rv)) return rv;
    
    nsDirectoryService::sCurrentProcess             = NS_NewAtom(NS_XPCOM_CURRENT_PROCESS_DIR);
    nsDirectoryService::sComponentRegistry          = NS_NewAtom(NS_XPCOM_COMPONENT_REGISTRY_FILE);
    nsDirectoryService::sComponentDirectory         = NS_NewAtom(NS_XPCOM_COMPONENT_DIR);
    nsDirectoryService::sGRE_Directory              = NS_NewAtom(NS_GRE_DIR);
    nsDirectoryService::sGRE_ComponentDirectory     = NS_NewAtom(NS_GRE_COMPONENT_DIR);

    nsDirectoryService::sOS_DriveDirectory          = NS_NewAtom(NS_OS_DRIVE_DIR);
    nsDirectoryService::sOS_TemporaryDirectory      = NS_NewAtom(NS_OS_TEMP_DIR);
    nsDirectoryService::sOS_CurrentProcessDirectory = NS_NewAtom(NS_OS_CURRENT_PROCESS_DIR);
    nsDirectoryService::sOS_CurrentWorkingDirectory = NS_NewAtom(NS_OS_CURRENT_WORKING_DIR);
#if defined(XP_MAC)
    nsDirectoryService::sDirectory                  = NS_NewAtom(NS_OS_SYSTEM_DIR);
    nsDirectoryService::sDesktopDirectory           = NS_NewAtom(NS_MAC_DESKTOP_DIR);
    nsDirectoryService::sTrashDirectory             = NS_NewAtom(NS_MAC_TRASH_DIR);
    nsDirectoryService::sStartupDirectory           = NS_NewAtom(NS_MAC_STARTUP_DIR);
    nsDirectoryService::sShutdownDirectory          = NS_NewAtom(NS_MAC_SHUTDOWN_DIR);
    nsDirectoryService::sAppleMenuDirectory         = NS_NewAtom(NS_MAC_APPLE_MENU_DIR);
    nsDirectoryService::sControlPanelDirectory      = NS_NewAtom(NS_MAC_CONTROL_PANELS_DIR);
    nsDirectoryService::sExtensionDirectory         = NS_NewAtom(NS_MAC_EXTENSIONS_DIR);
    nsDirectoryService::sFontsDirectory             = NS_NewAtom(NS_MAC_FONTS_DIR);
    nsDirectoryService::sClassicPreferencesDirectory= NS_NewAtom(NS_MAC_CLASSICPREFS_DIR);
    nsDirectoryService::sPreferencesDirectory       = NS_NewAtom(NS_MAC_PREFS_DIR);
    nsDirectoryService::sDocumentsDirectory         = NS_NewAtom(NS_MAC_DOCUMENTS_DIR);
    nsDirectoryService::sInternetSearchDirectory    = NS_NewAtom(NS_MAC_INTERNET_SEARCH_DIR);
    nsDirectoryService::sHomeDirectory              = NS_NewAtom(NS_MAC_HOME_DIR);
    nsDirectoryService::sDefaultDownloadDirectory   = NS_NewAtom(NS_MAC_DEFAULT_DOWNLOAD_DIR);
    nsDirectoryService::sUserLibDirectory           = NS_NewAtom(NS_MAC_USER_LIB_DIR);
#elif defined (XP_MACOSX)
    nsDirectoryService::sHomeDirectory              = NS_NewAtom(NS_MAC_HOME_DIR);
    nsDirectoryService::sDefaultDownloadDirectory   = NS_NewAtom(NS_MAC_DEFAULT_DOWNLOAD_DIR);
#elif defined (XP_WIN) 
    nsDirectoryService::sSystemDirectory            = NS_NewAtom(NS_OS_SYSTEM_DIR);
    nsDirectoryService::sWindowsDirectory           = NS_NewAtom(NS_WIN_WINDOWS_DIR);
    nsDirectoryService::sHomeDirectory              = NS_NewAtom(NS_WIN_HOME_DIR);
    nsDirectoryService::sDesktop                    = NS_NewAtom(NS_WIN_DESKTOP_DIR);
    nsDirectoryService::sPrograms                   = NS_NewAtom(NS_WIN_PROGRAMS_DIR);
    nsDirectoryService::sControls                   = NS_NewAtom(NS_WIN_CONTROLS_DIR);
    nsDirectoryService::sPrinters                   = NS_NewAtom(NS_WIN_PRINTERS_DIR);
    nsDirectoryService::sPersonal                   = NS_NewAtom(NS_WIN_PERSONAL_DIR);
    nsDirectoryService::sFavorites                  = NS_NewAtom(NS_WIN_FAVORITES_DIR);
    nsDirectoryService::sStartup                    = NS_NewAtom(NS_WIN_STARTUP_DIR);
    nsDirectoryService::sRecent                     = NS_NewAtom(NS_WIN_RECENT_DIR);
    nsDirectoryService::sSendto                     = NS_NewAtom(NS_WIN_SEND_TO_DIR);
    nsDirectoryService::sBitbucket                  = NS_NewAtom(NS_WIN_BITBUCKET_DIR);
    nsDirectoryService::sStartmenu                  = NS_NewAtom(NS_WIN_STARTMENU_DIR);
    nsDirectoryService::sDesktopdirectory           = NS_NewAtom(NS_WIN_DESKTOP_DIRECTORY);
    nsDirectoryService::sDrives                     = NS_NewAtom(NS_WIN_DRIVES_DIR);
    nsDirectoryService::sNetwork                    = NS_NewAtom(NS_WIN_NETWORK_DIR);
    nsDirectoryService::sNethood                    = NS_NewAtom(NS_WIN_NETHOOD_DIR);
    nsDirectoryService::sFonts                      = NS_NewAtom(NS_WIN_FONTS_DIR);
    nsDirectoryService::sTemplates                  = NS_NewAtom(NS_WIN_TEMPLATES_DIR);
    nsDirectoryService::sCommon_Startmenu           = NS_NewAtom(NS_WIN_COMMON_STARTMENU_DIR);
    nsDirectoryService::sCommon_Programs            = NS_NewAtom(NS_WIN_COMMON_PROGRAMS_DIR);
    nsDirectoryService::sCommon_Startup             = NS_NewAtom(NS_WIN_COMMON_STARTUP_DIR);
    nsDirectoryService::sCommon_Desktopdirectory    = NS_NewAtom(NS_WIN_COMMON_DESKTOP_DIRECTORY);
    nsDirectoryService::sAppdata                    = NS_NewAtom(NS_WIN_APPDATA_DIR);
    nsDirectoryService::sPrinthood                  = NS_NewAtom(NS_WIN_PRINTHOOD);
#elif defined (XP_UNIX)
    nsDirectoryService::sLocalDirectory             = NS_NewAtom(NS_UNIX_LOCAL_DIR);
    nsDirectoryService::sLibDirectory               = NS_NewAtom(NS_UNIX_LIB_DIR);
    nsDirectoryService::sHomeDirectory              = NS_NewAtom(NS_UNIX_HOME_DIR);
#elif defined (XP_OS2)
    nsDirectoryService::sSystemDirectory            = NS_NewAtom(NS_OS_SYSTEM_DIR);
    nsDirectoryService::sOS2Directory               = NS_NewAtom(NS_OS2_DIR);  
    nsDirectoryService::sHomeDirectory              = NS_NewAtom(NS_OS2_HOME_DIR);
    nsDirectoryService::sDesktopDirectory           = NS_NewAtom(NS_OS2_DESKTOP_DIR);
#elif defined (XP_BEOS)
    nsDirectoryService::sSystemDirectory            = NS_NewAtom(NS_OS_SYSTEM_DIR);
    nsDirectoryService::sSettingsDirectory          = NS_NewAtom(NS_BEOS_SETTINGS_DIR);
    nsDirectoryService::sHomeDirectory              = NS_NewAtom(NS_BEOS_HOME_DIR);
    nsDirectoryService::sDesktopDirectory           = NS_NewAtom(NS_BEOS_DESKTOP_DIR);
#endif

    // Let the list hold the only reference to the provider.
    nsAppFileLocationProvider *defaultProvider = new nsAppFileLocationProvider;
    if (!defaultProvider)
        return NS_ERROR_OUT_OF_MEMORY;
    // AppendElement returns PR_TRUE for success.
    rv = mProviders->AppendElement(defaultProvider) ? NS_OK : NS_ERROR_FAILURE;

    return rv;
}

PRBool
nsDirectoryService::ReleaseValues(nsHashKey* key, void* data, void* closure)
{
    nsISupports* value = (nsISupports*)data;
    NS_IF_RELEASE(value);
    return PR_TRUE;
}

nsDirectoryService::~nsDirectoryService()
{
     delete mHashtable;

     NS_IF_RELEASE(nsDirectoryService::sCurrentProcess);
     NS_IF_RELEASE(nsDirectoryService::sComponentRegistry);
     NS_IF_RELEASE(nsDirectoryService::sComponentDirectory);
     NS_IF_RELEASE(nsDirectoryService::sGRE_Directory);
     NS_IF_RELEASE(nsDirectoryService::sGRE_ComponentDirectory);
     NS_IF_RELEASE(nsDirectoryService::sOS_DriveDirectory);
     NS_IF_RELEASE(nsDirectoryService::sOS_TemporaryDirectory);
     NS_IF_RELEASE(nsDirectoryService::sOS_CurrentProcessDirectory);
     NS_IF_RELEASE(nsDirectoryService::sOS_CurrentWorkingDirectory);
#if defined(XP_MAC)
     NS_IF_RELEASE(nsDirectoryService::sDirectory);
     NS_IF_RELEASE(nsDirectoryService::sDesktopDirectory);
     NS_IF_RELEASE(nsDirectoryService::sTrashDirectory);
     NS_IF_RELEASE(nsDirectoryService::sStartupDirectory);
     NS_IF_RELEASE(nsDirectoryService::sShutdownDirectory);
     NS_IF_RELEASE(nsDirectoryService::sAppleMenuDirectory);
     NS_IF_RELEASE(nsDirectoryService::sControlPanelDirectory);
     NS_IF_RELEASE(nsDirectoryService::sExtensionDirectory);
     NS_IF_RELEASE(nsDirectoryService::sFontsDirectory);
     NS_IF_RELEASE(nsDirectoryService::sClassicPreferencesDirectory);
     NS_IF_RELEASE(nsDirectoryService::sPreferencesDirectory);
     NS_IF_RELEASE(nsDirectoryService::sDocumentsDirectory);
     NS_IF_RELEASE(nsDirectoryService::sInternetSearchDirectory);
     NS_IF_RELEASE(nsDirectoryService::sHomeDirectory);
     NS_IF_RELEASE(nsDirectoryService::sDefaultDownloadDirectory);
     NS_IF_RELEASE(nsDirectoryService::sUserLibDirectory);
#elif defined (XP_MACOSX)
     NS_IF_RELEASE(nsDirectoryService::sHomeDirectory);
     NS_IF_RELEASE(nsDirectoryService::sDefaultDownloadDirectory);
#elif defined (XP_WIN)
     NS_IF_RELEASE(nsDirectoryService::sSystemDirectory);
     NS_IF_RELEASE(nsDirectoryService::sWindowsDirectory);
     NS_IF_RELEASE(nsDirectoryService::sHomeDirectory);
     NS_IF_RELEASE(nsDirectoryService::sDesktop);
     NS_IF_RELEASE(nsDirectoryService::sPrograms);
     NS_IF_RELEASE(nsDirectoryService::sControls);
     NS_IF_RELEASE(nsDirectoryService::sPrinters);
     NS_IF_RELEASE(nsDirectoryService::sPersonal);
     NS_IF_RELEASE(nsDirectoryService::sFavorites);
     NS_IF_RELEASE(nsDirectoryService::sStartup);
     NS_IF_RELEASE(nsDirectoryService::sRecent);
     NS_IF_RELEASE(nsDirectoryService::sSendto);
     NS_IF_RELEASE(nsDirectoryService::sBitbucket);
     NS_IF_RELEASE(nsDirectoryService::sStartmenu);
     NS_IF_RELEASE(nsDirectoryService::sDesktopdirectory);
     NS_IF_RELEASE(nsDirectoryService::sDrives);
     NS_IF_RELEASE(nsDirectoryService::sNetwork);
     NS_IF_RELEASE(nsDirectoryService::sNethood);
     NS_IF_RELEASE(nsDirectoryService::sFonts);
     NS_IF_RELEASE(nsDirectoryService::sTemplates);
     NS_IF_RELEASE(nsDirectoryService::sCommon_Startmenu);
     NS_IF_RELEASE(nsDirectoryService::sCommon_Programs);
     NS_IF_RELEASE(nsDirectoryService::sCommon_Startup);
     NS_IF_RELEASE(nsDirectoryService::sCommon_Desktopdirectory);
     NS_IF_RELEASE(nsDirectoryService::sAppdata);
     NS_IF_RELEASE(nsDirectoryService::sPrinthood);
#elif defined (XP_UNIX)
     NS_IF_RELEASE(nsDirectoryService::sLocalDirectory);
     NS_IF_RELEASE(nsDirectoryService::sLibDirectory);
     NS_IF_RELEASE(nsDirectoryService::sHomeDirectory);
#elif defined (XP_OS2)
     NS_IF_RELEASE(nsDirectoryService::sSystemDirectory);
     NS_IF_RELEASE(nsDirectoryService::sOS2Directory);
     NS_IF_RELEASE(nsDirectoryService::sHomeDirectory);
     NS_IF_RELEASE(nsDirectoryService::sDesktopDirectory);
#elif defined (XP_BEOS)
     NS_IF_RELEASE(nsDirectoryService::sSettingsDirectory);
     NS_IF_RELEASE(nsDirectoryService::sHomeDirectory);
     NS_IF_RELEASE(nsDirectoryService::sDesktopDirectory);
     NS_IF_RELEASE(nsDirectoryService::sSystemDirectory);
#endif

     // clear the global
     mService = nsnull;

}

NS_IMPL_THREADSAFE_ISUPPORTS4(nsDirectoryService, nsIProperties, nsIDirectoryService, nsIDirectoryServiceProvider, nsIDirectoryServiceProvider2)


NS_IMETHODIMP
nsDirectoryService::Undefine(const char* prop)
{
    nsCStringKey key(prop);
    if (!mHashtable->Exists(&key))
        return NS_ERROR_FAILURE;

    mHashtable->Remove (&key);
    return NS_OK;
 }

NS_IMETHODIMP
nsDirectoryService::GetKeys(PRUint32 *count, char ***keys)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

struct FileData
{
  FileData(const char* aProperty,
           const nsIID& aUUID) :
    property(aProperty),
    data(nsnull),
    persistent(PR_TRUE),
    uuid(aUUID) {}
    
  const char*   property;
  nsISupports*  data;
  PRBool        persistent;
  const nsIID&  uuid;
};

static PRBool FindProviderFile(nsISupports* aElement, void *aData)
{
  nsresult rv;
  FileData* fileData = (FileData*)aData;
  if (fileData->uuid.Equals(NS_GET_IID(nsISimpleEnumerator)))
  {
      // Not all providers implement this iface
      nsCOMPtr<nsIDirectoryServiceProvider2> prov2 = do_QueryInterface(aElement);
      if (prov2)
      {
          rv = prov2->GetFiles(fileData->property, (nsISimpleEnumerator **)&fileData->data);
          if (NS_SUCCEEDED(rv) && fileData->data) {
          	  fileData->persistent = PR_FALSE; // Enumerators can never be peristent
              return PR_FALSE;
          }
      }
  }
  else
  {
      nsCOMPtr<nsIDirectoryServiceProvider> prov = do_QueryInterface(aElement);
      if (!prov)
          return PR_FALSE;
      rv = prov->GetFile(fileData->property, &fileData->persistent, (nsIFile **)&fileData->data);
      if (NS_SUCCEEDED(rv) && fileData->data)
          return PR_FALSE;
  }

  return PR_TRUE;
}

NS_IMETHODIMP
nsDirectoryService::Get(const char* prop, const nsIID & uuid, void* *result)
{
    nsCStringKey key(prop);
    
    nsCOMPtr<nsISupports> value = dont_AddRef(mHashtable->Get(&key));
    
    if (value)
    {
        nsCOMPtr<nsIFile> cloneFile;
        nsCOMPtr<nsIFile> cachedFile = do_QueryInterface(value);
        
        if (!cachedFile)
            return NS_ERROR_NULL_POINTER;

        cachedFile->Clone(getter_AddRefs(cloneFile));
        return cloneFile->QueryInterface(uuid, result);
    }

    // it is not one of our defaults, lets check any providers
    FileData fileData(prop, uuid);

    mProviders->EnumerateBackwards(FindProviderFile, &fileData);
    if (fileData.data)
    {
        if (fileData.persistent)
        {
            Set(prop, NS_STATIC_CAST(nsIFile*, fileData.data));
        }
        nsresult rv = (fileData.data)->QueryInterface(uuid, result);
        NS_RELEASE(fileData.data);  // addref occurs in FindProviderFile()
        return rv;
    }

    FindProviderFile(NS_STATIC_CAST(nsIDirectoryServiceProvider*, this), &fileData);
    if (fileData.data)
    {
        if (fileData.persistent)
        {
            Set(prop, NS_STATIC_CAST(nsIFile*, fileData.data));
        }
        nsresult rv = (fileData.data)->QueryInterface(uuid, result);
        NS_RELEASE(fileData.data);  // addref occurs in FindProviderFile()
        return rv;
    }

    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsDirectoryService::Set(const char* prop, nsISupports* value)
{
    nsCStringKey key(prop);
    if (mHashtable->Exists(&key) || value == nsnull)
        return NS_ERROR_FAILURE;

    nsCOMPtr<nsIFile> ourFile;
    value->QueryInterface(NS_GET_IID(nsIFile), getter_AddRefs(ourFile));
    if (ourFile)
    {
      nsCOMPtr<nsIFile> cloneFile;
      ourFile->Clone (getter_AddRefs (cloneFile));
      mHashtable->Put(&key, cloneFile);

      return NS_OK;
    }

    return NS_ERROR_FAILURE;   
}

NS_IMETHODIMP
nsDirectoryService::Has(const char *prop, PRBool *_retval)
{
    *_retval = PR_FALSE;
    nsCOMPtr<nsIFile> value;
    nsresult rv = Get(prop, NS_GET_IID(nsIFile), getter_AddRefs(value));
    if (NS_FAILED(rv)) 
        return rv;
    
    if (value)
    {
        *_retval = PR_TRUE;
    }
    
    return rv;
}

NS_IMETHODIMP
nsDirectoryService::RegisterProvider(nsIDirectoryServiceProvider *prov)
{
    nsresult rv;
    if (!prov)
        return NS_ERROR_FAILURE;
    if (!mProviders)
        return NS_ERROR_NOT_INITIALIZED;

    nsCOMPtr<nsISupports> supports = do_QueryInterface(prov, &rv);
    if (NS_FAILED(rv)) return rv;

    // AppendElement returns PR_TRUE for success.
    return mProviders->AppendElement(supports) ? NS_OK : NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsDirectoryService::UnregisterProvider(nsIDirectoryServiceProvider *prov)
{
    nsresult rv;
    if (!prov)
        return NS_ERROR_FAILURE;
    if (!mProviders)
        return NS_ERROR_NOT_INITIALIZED;

    nsCOMPtr<nsISupports> supports = do_QueryInterface(prov, &rv);
    if (NS_FAILED(rv)) return rv;

    // RemoveElement returns PR_TRUE for success.
    return mProviders->RemoveElement(supports) ? NS_OK : NS_ERROR_FAILURE;
}

// DO NOT ADD ANY LOCATIONS TO THIS FUNCTION UNTIL YOU TALK TO: dougt@netscape.com.
// This is meant to be a place of xpcom or system specific file locations, not
// application specific locations.  If you need the later, register a callback for
// your application.  

NS_IMETHODIMP
nsDirectoryService::GetFile(const char *prop, PRBool *persistent, nsIFile **_retval)
{
	nsCOMPtr<nsILocalFile> localFile;
	nsresult rv = NS_ERROR_FAILURE;

	*_retval = nsnull;
	*persistent = PR_TRUE;

    nsIAtom* inAtom = NS_NewAtom(prop);

	// check to see if it is one of our defaults
        
    if (inAtom == nsDirectoryService::sCurrentProcess)
    {
        rv = GetCurrentProcessDirectory(getter_AddRefs(localFile));
    }
    else if (inAtom == nsDirectoryService::sComponentRegistry)
    {
        rv = GetCurrentProcessDirectory(getter_AddRefs(localFile));
        if (localFile)
    		localFile->AppendNative(COMPONENT_REGISTRY_NAME);           
    }
    else if (inAtom == nsDirectoryService::sGRE_Directory)
    {
        rv = GetCurrentProcessDirectory(getter_AddRefs(localFile));
    }
    else if (inAtom == nsDirectoryService::sGRE_ComponentDirectory)
    {
        rv = GetCurrentProcessDirectory(getter_AddRefs(localFile));
        if (localFile)
             localFile->AppendNative(COMPONENT_DIRECTORY);
     }
    else if (inAtom == nsDirectoryService::sComponentDirectory)
    {
        rv = GetCurrentProcessDirectory(getter_AddRefs(localFile));
        if (localFile)
		    localFile->AppendNative(COMPONENT_DIRECTORY);           
    }
    else if (inAtom == nsDirectoryService::sOS_DriveDirectory)
    {
        nsSpecialSystemDirectory fileSpec(nsSpecialSystemDirectory::OS_DriveDirectory);
        rv = NS_FileSpecToIFile(&fileSpec, getter_AddRefs(localFile));  
    }
    else if (inAtom == nsDirectoryService::sOS_TemporaryDirectory)
    {
        nsSpecialSystemDirectory fileSpec(nsSpecialSystemDirectory::OS_TemporaryDirectory); 
        rv = NS_FileSpecToIFile(&fileSpec, getter_AddRefs(localFile));  
    }
    else if (inAtom == nsDirectoryService::sOS_CurrentProcessDirectory)
    {
        nsSpecialSystemDirectory fileSpec(nsSpecialSystemDirectory::OS_CurrentProcessDirectory); 
        rv = NS_FileSpecToIFile(&fileSpec, getter_AddRefs(localFile));  
    }
    else if (inAtom == nsDirectoryService::sOS_CurrentWorkingDirectory)
    {
        nsSpecialSystemDirectory fileSpec(nsSpecialSystemDirectory::OS_CurrentWorkingDirectory); 
        rv = NS_FileSpecToIFile(&fileSpec, getter_AddRefs(localFile));  
    }
       
#if defined(XP_MAC)
    else if (inAtom == nsDirectoryService::sDirectory)
    {
        nsSpecialSystemDirectory fileSpec(nsSpecialSystemDirectory::Mac_SystemDirectory); 
        rv = NS_FileSpecToIFile(&fileSpec, getter_AddRefs(localFile));  
    }
    else if (inAtom == nsDirectoryService::sDesktopDirectory)
    {
        nsSpecialSystemDirectory fileSpec(nsSpecialSystemDirectory::Mac_DesktopDirectory); 
        rv = NS_FileSpecToIFile(&fileSpec, getter_AddRefs(localFile));  
    }
    else if (inAtom == nsDirectoryService::sHomeDirectory)
    {
        nsSpecialSystemDirectory fileSpec(nsSpecialSystemDirectory::Mac_DocumentsDirectory); 
        rv = NS_FileSpecToIFile(&fileSpec, getter_AddRefs(localFile));  
    }
    else if (inAtom == nsDirectoryService::sTrashDirectory)
    {
        nsSpecialSystemDirectory fileSpec(nsSpecialSystemDirectory::Mac_TrashDirectory); 
        rv = NS_FileSpecToIFile(&fileSpec, getter_AddRefs(localFile));  
    }
    else if (inAtom == nsDirectoryService::sStartupDirectory)
    {
        nsSpecialSystemDirectory fileSpec(nsSpecialSystemDirectory::Mac_StartupDirectory); 
        rv = NS_FileSpecToIFile(&fileSpec, getter_AddRefs(localFile));  
    }
    else if (inAtom == nsDirectoryService::sShutdownDirectory)
    {
        nsSpecialSystemDirectory fileSpec(nsSpecialSystemDirectory::Mac_ShutdownDirectory); 
        rv = NS_FileSpecToIFile(&fileSpec, getter_AddRefs(localFile));  
    }
    else if (inAtom == nsDirectoryService::sAppleMenuDirectory)
    {
        nsSpecialSystemDirectory fileSpec(nsSpecialSystemDirectory::Mac_AppleMenuDirectory); 
        rv = NS_FileSpecToIFile(&fileSpec, getter_AddRefs(localFile));  
    }
    else if (inAtom == nsDirectoryService::sControlPanelDirectory)
    {
        nsSpecialSystemDirectory fileSpec(nsSpecialSystemDirectory::Mac_ControlPanelDirectory); 
        rv = NS_FileSpecToIFile(&fileSpec, getter_AddRefs(localFile));  
    }
    else if (inAtom == nsDirectoryService::sExtensionDirectory)
    {
        nsSpecialSystemDirectory fileSpec(nsSpecialSystemDirectory::Mac_ExtensionDirectory);
        rv = NS_FileSpecToIFile(&fileSpec, getter_AddRefs(localFile));  
    }
    else if (inAtom == nsDirectoryService::sFontsDirectory)
    {
        nsSpecialSystemDirectory fileSpec(nsSpecialSystemDirectory::Mac_FontsDirectory); 
        rv = NS_FileSpecToIFile(&fileSpec, getter_AddRefs(localFile));  
    }
    else if (inAtom == nsDirectoryService::sClassicPreferencesDirectory)
    {
        nsSpecialSystemDirectory fileSpec(nsSpecialSystemDirectory::Mac_ClassicPreferencesDirectory); 
        rv = NS_FileSpecToIFile(&fileSpec, getter_AddRefs(localFile));  
    }
    else if (inAtom == nsDirectoryService::sPreferencesDirectory)
    {
        nsSpecialSystemDirectory fileSpec(nsSpecialSystemDirectory::Mac_PreferencesDirectory); 
        rv = NS_FileSpecToIFile(&fileSpec, getter_AddRefs(localFile));  
    }
    else if (inAtom == nsDirectoryService::sDocumentsDirectory)
    {
        nsSpecialSystemDirectory fileSpec(nsSpecialSystemDirectory::Mac_DocumentsDirectory);
        rv = NS_FileSpecToIFile(&fileSpec, getter_AddRefs(localFile));  
    }
    else if (inAtom == nsDirectoryService::sInternetSearchDirectory)
    {
        nsSpecialSystemDirectory fileSpec(nsSpecialSystemDirectory::Mac_InternetSearchDirectory); 
        rv = NS_FileSpecToIFile(&fileSpec, getter_AddRefs(localFile));  
    }   
    else if (inAtom == nsDirectoryService::sDefaultDownloadDirectory)
    {
        nsSpecialSystemDirectory fileSpec(nsSpecialSystemDirectory::Mac_DefaultDownloadDirectory); 
        rv = NS_FileSpecToIFile(&fileSpec, getter_AddRefs(localFile));  
    }
    else if (inAtom == nsDirectoryService::sUserLibDirectory)
    {
        nsSpecialSystemDirectory fileSpec(nsSpecialSystemDirectory::Mac_UserLibDirectory); 
        rv = NS_FileSpecToIFile(&fileSpec, getter_AddRefs(localFile));  
    }
#elif defined (XP_MACOSX)
    else if (inAtom == nsDirectoryService::sHomeDirectory)
    {
        OSErr err;
        FSRef fsRef;
        err = ::FSFindFolder(kUserDomain, kDomainTopLevelFolderType, kCreateFolder, &fsRef);
        if (err == noErr)
        {
            NS_NewLocalFile(nsString(), PR_TRUE, getter_AddRefs(localFile));
            nsCOMPtr<nsILocalFileMac> localMacFile(do_QueryInterface(localFile));
            if (localMacFile)
                rv = localMacFile->InitWithFSRef(&fsRef);
        }
    }
    else if (inAtom == nsDirectoryService::sDefaultDownloadDirectory)
    {
        NS_NewLocalFile(nsString(), PR_TRUE, getter_AddRefs(localFile));
        nsCOMPtr<nsILocalFileMac> localMacFile(do_QueryInterface(localFile));

        if (localMacFile)
        {
            OSErr err;
            ICInstance icInstance;

            err = ::ICStart(&icInstance, 'XPCM');
            if (err == noErr)
            {
                ICAttr attrs;
                ICFileSpec icFileSpec;
                long size = kICFileSpecHeaderSize;
                err = ::ICGetPref(icInstance, kICDownloadFolder, &attrs, &icFileSpec, &size);
                if (err == noErr || (err == icTruncatedErr && size >= kICFileSpecHeaderSize))
                {
                    rv = localMacFile->InitWithFSSpec(&icFileSpec.fss);
                }
                ::ICStop(icInstance);
            }
            
            if NS_FAILED(rv)
            { // We got an error getting the DL folder from IC so try finding the user's Desktop folder
                FSRef fsRef;
                err = ::FSFindFolder(kUserDomain, kSystemDesktopFolderType, kCreateFolder, &fsRef);
                if (err == noErr)
                    rv = localMacFile->InitWithFSRef(&fsRef);
            }
        }
    }
#elif defined (XP_WIN)
    else if (inAtom == nsDirectoryService::sSystemDirectory)
    {
        nsSpecialSystemDirectory fileSpec(nsSpecialSystemDirectory::Win_SystemDirectory); 
        rv = NS_FileSpecToIFile(&fileSpec, getter_AddRefs(localFile));  
    }
    else if (inAtom == nsDirectoryService::sWindowsDirectory)
    {
        nsSpecialSystemDirectory fileSpec(nsSpecialSystemDirectory::Win_WindowsDirectory); 
        rv = NS_FileSpecToIFile(&fileSpec, getter_AddRefs(localFile));  
    }
    else if (inAtom == nsDirectoryService::sHomeDirectory)
    {
        nsSpecialSystemDirectory fileSpec(nsSpecialSystemDirectory::Win_HomeDirectory); 
        rv = NS_FileSpecToIFile(&fileSpec, getter_AddRefs(localFile));  
    }
    else if (inAtom == nsDirectoryService::sDesktop)
    {
        nsSpecialSystemDirectory fileSpec(nsSpecialSystemDirectory::Win_Desktop); 
        rv = NS_FileSpecToIFile(&fileSpec, getter_AddRefs(localFile));  
    }
    else if (inAtom == nsDirectoryService::sPrograms)
    {
        nsSpecialSystemDirectory fileSpec(nsSpecialSystemDirectory::Win_Programs); 
        rv = NS_FileSpecToIFile(&fileSpec, getter_AddRefs(localFile));  
    }
    else if (inAtom == nsDirectoryService::sControls)
    {
        nsSpecialSystemDirectory fileSpec(nsSpecialSystemDirectory::Win_Controls); 
        rv = NS_FileSpecToIFile(&fileSpec, getter_AddRefs(localFile));  
    }
    else if (inAtom == nsDirectoryService::sPrinters)
    {
        nsSpecialSystemDirectory fileSpec(nsSpecialSystemDirectory::Win_Printers); 
        rv = NS_FileSpecToIFile(&fileSpec, getter_AddRefs(localFile));  
    }
    else if (inAtom == nsDirectoryService::sPersonal)
    {
        nsSpecialSystemDirectory fileSpec(nsSpecialSystemDirectory::Win_Personal); 
        rv = NS_FileSpecToIFile(&fileSpec, getter_AddRefs(localFile));  
    }
    else if (inAtom == nsDirectoryService::sFavorites)
    {
        nsSpecialSystemDirectory fileSpec(nsSpecialSystemDirectory::Win_Favorites); 
        rv = NS_FileSpecToIFile(&fileSpec, getter_AddRefs(localFile));  
    }
    else if (inAtom == nsDirectoryService::sStartup)
    {
        nsSpecialSystemDirectory fileSpec(nsSpecialSystemDirectory::Win_Startup);
        rv = NS_FileSpecToIFile(&fileSpec, getter_AddRefs(localFile));  
    }
    else if (inAtom == nsDirectoryService::sRecent)
    {
        nsSpecialSystemDirectory fileSpec(nsSpecialSystemDirectory::Win_Recent); 
        rv = NS_FileSpecToIFile(&fileSpec, getter_AddRefs(localFile));  
    }
    else if (inAtom == nsDirectoryService::sSendto)
    {
        nsSpecialSystemDirectory fileSpec(nsSpecialSystemDirectory::Win_Sendto); 
        rv = NS_FileSpecToIFile(&fileSpec, getter_AddRefs(localFile));  
    }
    else if (inAtom == nsDirectoryService::sBitbucket)
    {
        nsSpecialSystemDirectory fileSpec(nsSpecialSystemDirectory::Win_Bitbucket); 
        rv = NS_FileSpecToIFile(&fileSpec, getter_AddRefs(localFile));  
    }
    else if (inAtom == nsDirectoryService::sStartmenu)
    {
        nsSpecialSystemDirectory fileSpec(nsSpecialSystemDirectory::Win_Startmenu); 
        rv = NS_FileSpecToIFile(&fileSpec, getter_AddRefs(localFile));  
    }
    else if (inAtom == nsDirectoryService::sDesktopdirectory)
    {
        nsSpecialSystemDirectory fileSpec(nsSpecialSystemDirectory::Win_Desktopdirectory); 
        rv = NS_FileSpecToIFile(&fileSpec, getter_AddRefs(localFile));  
    }
    else if (inAtom == nsDirectoryService::sDrives)
    {
        nsSpecialSystemDirectory fileSpec(nsSpecialSystemDirectory::Win_Drives);
        rv = NS_FileSpecToIFile(&fileSpec, getter_AddRefs(localFile));  
    }
    else if (inAtom == nsDirectoryService::sNetwork)
    {
        nsSpecialSystemDirectory fileSpec(nsSpecialSystemDirectory::Win_Network); 
        rv = NS_FileSpecToIFile(&fileSpec, getter_AddRefs(localFile));  
    }
    else if (inAtom == nsDirectoryService::sNethood)
    {
        nsSpecialSystemDirectory fileSpec(nsSpecialSystemDirectory::Win_Nethood); 
        rv = NS_FileSpecToIFile(&fileSpec, getter_AddRefs(localFile));  
    }
    else if (inAtom == nsDirectoryService::sFonts)
    {
        nsSpecialSystemDirectory fileSpec(nsSpecialSystemDirectory::Win_Fonts);
        rv = NS_FileSpecToIFile(&fileSpec, getter_AddRefs(localFile));  
    }
    else if (inAtom == nsDirectoryService::sTemplates)
    {
        nsSpecialSystemDirectory fileSpec(nsSpecialSystemDirectory::Win_Templates); 
        rv = NS_FileSpecToIFile(&fileSpec, getter_AddRefs(localFile));  
    }
    else if (inAtom == nsDirectoryService::sCommon_Startmenu)
    {
        nsSpecialSystemDirectory fileSpec(nsSpecialSystemDirectory::Win_Common_Startmenu); 
        rv = NS_FileSpecToIFile(&fileSpec, getter_AddRefs(localFile));  
    }
    else if (inAtom == nsDirectoryService::sCommon_Programs)
    {
        nsSpecialSystemDirectory fileSpec(nsSpecialSystemDirectory::Win_Common_Programs); 
        rv = NS_FileSpecToIFile(&fileSpec, getter_AddRefs(localFile));  
    }
    else if (inAtom == nsDirectoryService::sCommon_Startup)
    {
        nsSpecialSystemDirectory fileSpec(nsSpecialSystemDirectory::Win_Common_Startup); 
        rv = NS_FileSpecToIFile(&fileSpec, getter_AddRefs(localFile));  
    }
    else if (inAtom == nsDirectoryService::sCommon_Desktopdirectory)
    {
        nsSpecialSystemDirectory fileSpec(nsSpecialSystemDirectory::Win_Common_Desktopdirectory); 
        rv = NS_FileSpecToIFile(&fileSpec, getter_AddRefs(localFile));  
    }
    else if (inAtom == nsDirectoryService::sAppdata)
    {
        nsSpecialSystemDirectory fileSpec(nsSpecialSystemDirectory::Win_Appdata); 
        rv = NS_FileSpecToIFile(&fileSpec, getter_AddRefs(localFile));  
    }
    else if (inAtom == nsDirectoryService::sPrinthood)
    {
        nsSpecialSystemDirectory fileSpec(nsSpecialSystemDirectory::Win_Printhood); 
        rv = NS_FileSpecToIFile(&fileSpec, getter_AddRefs(localFile));  
    }
#elif defined (XP_UNIX)

    else if (inAtom == nsDirectoryService::sLocalDirectory)
    {
        nsSpecialSystemDirectory fileSpec(nsSpecialSystemDirectory::Unix_LocalDirectory); 
        rv = NS_FileSpecToIFile(&fileSpec, getter_AddRefs(localFile));  
    }
    else if (inAtom == nsDirectoryService::sLibDirectory)
    {
        nsSpecialSystemDirectory fileSpec(nsSpecialSystemDirectory::Unix_LibDirectory);
        rv = NS_FileSpecToIFile(&fileSpec, getter_AddRefs(localFile));  
    }
    else if (inAtom == nsDirectoryService::sHomeDirectory)
    {
        nsSpecialSystemDirectory fileSpec(nsSpecialSystemDirectory::Unix_HomeDirectory); 
        rv = NS_FileSpecToIFile(&fileSpec, getter_AddRefs(localFile));  
    }
#elif defined (XP_OS2)
    else if (inAtom == nsDirectoryService::sSystemDirectory)
    {
        nsSpecialSystemDirectory fileSpec(nsSpecialSystemDirectory::OS2_SystemDirectory); 
        rv = NS_FileSpecToIFile(&fileSpec, getter_AddRefs(localFile));  
    }
    else if (inAtom == nsDirectoryService::sOS2Directory)
    {
        nsSpecialSystemDirectory fileSpec(nsSpecialSystemDirectory::OS2_OS2Directory); 
        rv = NS_FileSpecToIFile(&fileSpec, getter_AddRefs(localFile));  
    }
    else if (inAtom == nsDirectoryService::sHomeDirectory)
    {
        nsSpecialSystemDirectory fileSpec(nsSpecialSystemDirectory::OS2_HomeDirectory); 
        rv = NS_FileSpecToIFile(&fileSpec, getter_AddRefs(localFile));  
    }
    else if (inAtom == nsDirectoryService::sDesktopDirectory)
    {
        nsSpecialSystemDirectory fileSpec(nsSpecialSystemDirectory::OS2_DesktopDirectory); 
        rv = NS_FileSpecToIFile(&fileSpec, getter_AddRefs(localFile));  
    }
#elif defined (XP_BEOS)
    else if (inAtom == nsDirectoryService::sSettingsDirectory)
    {
        nsSpecialSystemDirectory fileSpec(nsSpecialSystemDirectory::BeOS_SettingsDirectory); 
        rv = NS_FileSpecToIFile(&fileSpec, getter_AddRefs(localFile));  
    }
    else if (inAtom == nsDirectoryService::sHomeDirectory)
    {
        nsSpecialSystemDirectory fileSpec(nsSpecialSystemDirectory::BeOS_HomeDirectory);
        rv = NS_FileSpecToIFile(&fileSpec, getter_AddRefs(localFile));  
    }
    else if (inAtom == nsDirectoryService::sDesktopDirectory)
    {
        nsSpecialSystemDirectory fileSpec(nsSpecialSystemDirectory::BeOS_DesktopDirectory); 
        rv = NS_FileSpecToIFile(&fileSpec, getter_AddRefs(localFile));  
    }
    else if (inAtom == nsDirectoryService::sSystemDirectory)
    {
        nsSpecialSystemDirectory fileSpec(nsSpecialSystemDirectory::BeOS_SystemDirectory); 
        rv = NS_FileSpecToIFile(&fileSpec, getter_AddRefs(localFile));  
    }
#endif


    NS_RELEASE(inAtom);

	if (localFile && NS_SUCCEEDED(rv))
		return localFile->QueryInterface(NS_GET_IID(nsIFile), (void**)_retval);

	return rv;
}

NS_IMETHODIMP
nsDirectoryService::GetFiles(const char *prop, nsISimpleEnumerator **_retval)
{
    NS_ENSURE_ARG_POINTER(_retval);
    *_retval = nsnull;
        
    return NS_ERROR_FAILURE;
}
