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

#include "pratom.h"
#include "nsRepository.h"
#include "nsIAppShellComponentImpl.h"
#include "nsIBrowserWindow.h"
#include "nsIComponentManager.h"
#include "nsIServiceManager.h"
#include "nsINetSupportDialogService.h"
#include "nsIWindowMediator.h"
#include "nsICommonDialogs.h"
#include "nsIScriptGlobalObject.h"
#include "nsSpecialSystemDirectory.h"
#include "nsFileStream.h"
#include "nsIFileSpec.h"
#include "nsCOMPtr.h"
#include "prio.h"
#include "prerror.h"
#include "prmem.h"
#include "nsIPref.h"
#include "plstr.h"
#include "prprf.h"

#include "nsProxiedService.h"

#include "nsNeckoUtil.h"

/* Network */

#include "net.h"

#include "nsPrefMigration.h"
#include "nsPrefMigrationFactory.h"
//#include "nsPMProgressDlg.h"

#define NEW_DIR_PERMISSIONS 00700

#define PREF_FILE_HEADER_STRING "# Mozilla User Preferences    " 

#if defined(XP_UNIX)
#define MAIL_FILTER_FILE_NAME_IN_4x "mailrule"
#define SUMMARY_SUFFIX_IN_4x ".summary"
#define COOKIES_FILE_NAME_IN_4x "cookies"
#define BOOKMARKS_FILE_NAME_IN_4x "bookmarks.html"
#elif defined(XP_MAC)
#define MAIL_FILTER_FILE_NAME_IN_4x "<hostname> Rules"
#define MAIL_FILTER_FILE_NAME_SUFFIX_IN_4x " Rules"   
#define SUMMARY_SUFFIX_IN_4x ".snm"
#define COOKIES_FILE_NAME_IN_4x "MagicCookie"
#define BOOKMARKS_FILE_NAME_IN_4x "Bookmarks.html"
#else /* XP_PC */
#define MAIL_FILTER_FILE_NAME_IN_4x "rules.dat"
#define SUMMARY_SUFFIX_IN_4x ".snm"
#define COOKIES_FILE_NAME_IN_4x "cookies.txt"
#define BOOKMARKS_FILE_NAME_IN_4x "bookmarks.htm"
#endif /* XP_UNIX */

#define SUMMARY_SUFFIX_IN_5x ".msf"
#define COOKIES_FILE_NAME_IN_5x "cookies.txt"
#define MAIL_FILTER_FILE_NAME_IN_5x "rules.dat"
#define BOOKMARKS_FILE_NAME_IN_5x "bookmarks.html"

#define PREMIGRATION_PREFIX "premigration"
#define PREF_MAIL_DIRECTORY "mail.directory"
#define PREF_NEWS_DIRECTORY "news.directory"
#define PREF_MAIL_IMAP_ROOT_DIR "mail.imap.root_dir"
#define PREF_NETWORK_HOSTS_POP_SERVER "network.hosts.pop_server"
#define PREF_MAIL_SERVER_TYPE	"mail.server_type"
#define POP_4X_MAIL_TYPE 0
#define IMAP_4X_MAIL_TYPE 1
#define MOVEMAIL_4X_MAIL_TYPE 2

#ifdef XP_UNIX
/* a 4.x profile on UNIX is rooted at something like
 * "/u/sspitzer/.netscape"
 * profile + OLD_MAIL_DIR_NAME = "/u/sspitzer/.netscape/../nsmail" = "/u/sspitzer/nsmail"
 * profile + OLD_NEWS_DIR_NAME = "/u/sspitzer/.netscape/xover-cache"
 * profile + OLD_IMAPMAIL_DIR_NAME = "/u/sspitzer/.netscape/../ns_imap" = "/u/sspitzer/ns_imap"
 * which is as good as we're going to get for defaults on UNIX.
 */
#define OLD_MAIL_DIR_NAME "/../nsmail"
#define OLD_NEWS_DIR_NAME "/xover-cache"
#define OLD_IMAPMAIL_DIR_NAME "/../ns_imap"
#else
#define OLD_MAIL_DIR_NAME "Mail"
#define OLD_NEWS_DIR_NAME "News"
#define OLD_IMAPMAIL_DIR_NAME "ImapMail"
#endif /* XP_UNIX */

#define NEW_DIR_SUFFIX "5"

/* these are the same for all platforms */
#define NEW_MAIL_DIR_NAME "Mail"
#define NEW_NEWS_DIR_NAME "News"
#define NEW_IMAPMAIL_DIR_NAME "ImapMail"
#define NEW_LOCAL_MAIL_DIR_NAME "Local Mail"

/* who's going to win the file name battle? */
#if defined(XP_UNIX)
#define PREF_FILE_NAME_IN_4x "preferences.js"
#elif defined(XP_MAC)
#define PREF_FILE_NAME_IN_4x "Netscape Preferences"
#elif defined(XP_PC)
#define PREF_FILE_NAME_IN_4x "prefs.js"
#else
/* this will cause a failure at run time, as it should, since we don't know
   how to migrate platforms other than Mac, Windows and UNIX */
#define PREF_FILE_NAME_IN_4x ""
#endif /* XP_UNIX */

/* and the winner is:  Windows */
#define PREF_FILE_NAME_IN_5x "prefs.js"

typedef struct
{
  const char* oldFile;
  const char* newFile;

} MigrateProfileItem;


/*-----------------------------------------------------------------
 * Globals
 *-----------------------------------------------------------------*/
static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);
static NS_DEFINE_IID(kIFactoryIID, NS_IFACTORY_IID);
static NS_DEFINE_IID(kAppShellServiceCID, NS_APPSHELL_SERVICE_CID );

static NS_DEFINE_IID(kIPrefMigrationIID, NS_IPREFMIGRATION_IID);
static NS_DEFINE_IID(kPrefMigrationCID,  NS_PREFMIGRATION_CID);

static NS_DEFINE_CID(kPrefServiceCID, NS_PREF_CID);
static NS_DEFINE_CID(kComponentManagerCID, NS_COMPONENTMANAGER_CID);
static NS_DEFINE_CID(kNetSupportDialogCID, NS_NETSUPPORTDIALOG_CID);
static NS_DEFINE_CID(kWindowMediatorCID, NS_WINDOWMEDIATOR_CID);
static NS_DEFINE_CID(kCommonDialogsCID, NS_CommonDialog_CID);
static NS_DEFINE_CID(kDialogParamBlockCID, NS_DialogParamBlock_CID);

static NS_DEFINE_IID(kProxyObjectManagerCID, NS_PROXYEVENT_MANAGER_CID);

static PRInt32 gInstanceCnt = 0;
static PRInt32 gLockCnt     = 0;




nsPrefMigration* nsPrefMigration::mInstance = nsnull;

nsPrefMigration *
nsPrefMigration::GetInstance()
{
    if (mInstance == NULL) 
    {
        mInstance = new nsPrefMigration();
    }
    return mInstance;
}



nsPrefMigration::nsPrefMigration() : m_prefs(0)
{
  NS_INIT_REFCNT();
}



PRBool ProfilesToMigrateCleanup(void* aElement, void *aData)
{
  if (aElement)
    delete (MigrateProfileItem*)aElement;

  return PR_TRUE;
}

nsPrefMigration::~nsPrefMigration()
{
  if (m_prefs) nsServiceManager::ReleaseService(kPrefServiceCID, m_prefs);
  mProfilesToMigrate.EnumerateForwards((nsVoidArrayEnumFunc)ProfilesToMigrateCleanup, nsnull);
}



nsresult
nsPrefMigration::getPrefService()
{
  // get the prefs service
  nsresult rv = NS_OK;

  NS_WITH_SERVICE(nsIPref, pIMyService, kPrefServiceCID, &rv);
  if(NS_FAILED(rv))
    return rv;

  NS_WITH_SERVICE(nsIProxyObjectManager, pIProxyObjectManager, kProxyObjectManagerCID, &rv);
  if(NS_FAILED(rv))
    return rv;
  
  rv = pIProxyObjectManager->GetProxyObject(nsnull, 
                                            nsIPref::GetIID(), 
                                            pIMyService, 
                                            PROXY_SYNC,
                                            (void**)&m_prefs);
  NS_RELEASE(pIMyService);

if (NS_FAILED(rv)) return rv;

  // m_prefs is good now
  return NS_OK;   
}

NS_IMETHODIMP 
nsPrefMigration::QueryInterface(const nsIID& iid,void** result)
{
  nsresult rv = NS_NOINTERFACE;
  if (! result)
    return NS_ERROR_NULL_POINTER;

  void *res = nsnull;
  if (iid.Equals(nsCOMTypeInfo<nsIPrefMigration>::GetIID()) ||
      iid.Equals(nsCOMTypeInfo<nsISupports>::GetIID()))
    res = NS_STATIC_CAST(nsIPrefMigration*, this);
  else if (iid.Equals(nsCOMTypeInfo<nsIShutdownListener>::GetIID()))
    res = NS_STATIC_CAST(nsIShutdownListener*, this);

  if (res) {
    NS_ADDREF(this);
    *result = res;
    rv = NS_OK;
  }

  return rv;
}

NS_IMPL_ADDREF(nsPrefMigration)
NS_IMPL_RELEASE(nsPrefMigration)


NS_IMETHODIMP
nsPrefMigration::AddProfilePaths(const char * oldProfilePathStr, const char * newProfilePathStr)
{
  MigrateProfileItem* item = new MigrateProfileItem();
  item->oldFile = oldProfilePathStr;
  item->newFile = newProfilePathStr;
  
  if (mProfilesToMigrate.AppendElement((void*)item));
    return NS_OK;

  return NS_ERROR_FAILURE;
}


NS_IMETHODIMP
nsPrefMigration::ProcessPrefs()
{
  nsresult rv;
  nsCOMPtr<nsIURI> pmprogressURL;
  PRInt32 pmWinWidth  = 300;
  PRInt32 pmWinHeight = 200;
  nsAutoString args;
  const char *pmprogressStr = "chrome://global/content/pmunprog.xul";


  NS_WITH_SERVICE(nsIAppShellService, PMProgressAppShell,
                  kAppShellServiceCID, &rv);
  if (NS_FAILED(rv))
    return rv;

  rv = NS_NewURI(getter_AddRefs(pmprogressURL), pmprogressStr);
  if (NS_FAILED(rv))
    return rv;

  rv = PMProgressAppShell->CreateTopLevelWindow(nsnull, pmprogressURL,
                                          PR_TRUE, PR_TRUE, NS_CHROME_ALL_CHROME,
                                          nsnull, pmWinWidth, pmWinHeight,
                                          getter_AddRefs(mPMProgressWindow));

  if (NS_FAILED(rv)) 
  {
     return rv;
  }

  //PMProgressAppShell->Run();
  mPMProgressWindow->ShowModal();
  
  return NS_OK;
}


static PRThread* gMigrationThread = nsnull;


extern "C" void ProfileMigrationController(void *data)
{
  if (!data) return;

  nsPrefMigration* migrator = (nsPrefMigration*)data;
  nsIPrefMigration* interfaceM = (nsIPrefMigration*)data;
  PRInt32 count = migrator->mProfilesToMigrate.Count();
  PRInt32 index = -1;
  nsresult rv = NS_OK;

  while (++index < count && NS_SUCCEEDED(rv)) 
  {

    MigrateProfileItem* item = (MigrateProfileItem*)migrator->mProfilesToMigrate.ElementAt(index);
    if (item)
      rv = migrator->ProcessPrefsCallback(item->oldFile, item->newFile);
  }

  
  NS_WITH_SERVICE(nsIProxyObjectManager, pIProxyObjectManager, kProxyObjectManagerCID, &rv);
  if(NS_FAILED(rv))
    return;
  
  nsIPrefMigration* prefProxy;
  nsCOMPtr<nsIPrefMigration> migratorInterface = do_QueryInterface(interfaceM);

  rv = pIProxyObjectManager->GetProxyObject(nsnull, 
                                            nsIPrefMigration::GetIID(), 
                                            migratorInterface, 
                                            PROXY_SYNC,
                                            (void**)&prefProxy);
  
  prefProxy->WindowCloseCallback(); 
  NS_RELEASE(prefProxy);

}

NS_IMETHODIMP
nsPrefMigration::WindowCloseCallback()
{

  if (mPMProgressWindow)
    mPMProgressWindow->Close();

  return NS_OK;
}



NS_IMETHODIMP
nsPrefMigration::ProcessPrefsFromJS()  // called via js so that we can have progress bar that show up.
{
  gMigrationThread = PR_CreateThread(PR_USER_THREAD,
                                     ProfileMigrationController,
                                     this, 
                                     PR_PRIORITY_NORMAL, 
                                     PR_GLOBAL_THREAD, 
                                     PR_UNJOINABLE_THREAD,
                                     0);  
  return NS_OK;
}


/*--------------------------------------------------------------------------
 * Process Prefs is the primary funtion for the class nsPrefMigration.
 *
 * Called by: The Profile Manager
 * INPUT: The specific profile path (prefPath) and the 5.0 installed path
 * OUTPUT: The modified 5.0 prefs files
 * RETURN: Success or a failure code
 *
 *-------------------------------------------------------------------------*/
nsresult
nsPrefMigration::ProcessPrefsCallback(const char* oldProfilePathStr, const char * newProfilePathStr)
{ 
  char *oldPOPMailPathStr = nsnull;
  char *oldIMAPMailPathStr= nsnull; 
  char *oldIMAPLocalMailPathStr = nsnull;
  char *oldNewsPathStr = nsnull;
  char *newPOPMailPathStr = nsnull;
  char *newIMAPMailPathStr = nsnull;
  char *newIMAPLocalMailPathStr = nsnull; 
  char *newNewsPathStr = nsnull;
  char *popServerName = nsnull;

  nsresult rv;

  nsFileSpec oldPOPMailPath, oldIMAPMailPath, oldIMAPLocalMailPath, oldNewsPath;
  nsFileSpec newPOPMailPath, newIMAPMailPath, newIMAPLocalMailPath, newNewsPath;

  PRInt32 serverType = POP_4X_MAIL_TYPE; 
  PRBool hasIMAP = PR_FALSE;

#if defined(NS_DEBUG)
  printf("*Entered Actual Migration routine*\n");
#endif

//  PRUnichar *testing1;
//  PRUnichar *testing2;
	
//  ShowProgressDialog(testing1, testing2);
  
  /* Create the new profile tree for 5.x */
  rv = CreateNewUser5Tree(oldProfilePathStr, newProfilePathStr);
  if (NS_FAILED(rv)) return rv;


  rv = getPrefService();
  if (NS_FAILED(rv)) return rv;

  /* Create the new mail directory from the setting in prefs.js or a default */
  rv = m_prefs->GetIntPref(PREF_MAIL_SERVER_TYPE, &serverType);
  if (NS_FAILED(rv)) return rv;
  
  if (serverType == POP_4X_MAIL_TYPE) 
    {
      if(NS_SUCCEEDED(GetDirFromPref(oldProfilePathStr,newProfilePathStr,NEW_MAIL_DIR_NAME, PREF_MAIL_DIRECTORY, &newPOPMailPathStr, &oldPOPMailPathStr)))
        {
          /* convert back to nsFileSpec */
          oldPOPMailPath = oldPOPMailPathStr;
          newPOPMailPath = newPOPMailPathStr;
        }
      else
        {
          /* use the default locations */
          oldPOPMailPath = oldProfilePathStr;
          oldPOPMailPath += OLD_MAIL_DIR_NAME;

	// if we are here, then PREF_MAIL_DIRECTORY was not set.
	// but we still need it in the actual pref migration
          rv = SetPremigratedFilePref(PREF_MAIL_DIRECTORY, oldPOPMailPath);
	  if (NS_FAILED(rv)) return rv;

          newPOPMailPath = newProfilePathStr;
          newPOPMailPath += NEW_MAIL_DIR_NAME;
        }
      PR_MkDir(nsNSPRPath(newPOPMailPath), NEW_DIR_PERMISSIONS);
      m_prefs->CopyCharPref(PREF_NETWORK_HOSTS_POP_SERVER, &popServerName);
      newPOPMailPath += popServerName;
      PR_MkDir(nsNSPRPath(newPOPMailPath), NEW_DIR_PERMISSIONS);
    }
  else if (serverType == IMAP_4X_MAIL_TYPE)
    {
      hasIMAP = PR_TRUE;
      /* First get the actual local mail files location */
      if(NS_SUCCEEDED(GetDirFromPref(oldProfilePathStr,newProfilePathStr, NEW_MAIL_DIR_NAME, PREF_MAIL_DIRECTORY, &newIMAPLocalMailPathStr, &oldIMAPLocalMailPathStr)))
        {
          /* convert back to nsFileSpec */
          oldIMAPLocalMailPath = oldIMAPLocalMailPathStr;
          newIMAPLocalMailPath = newIMAPLocalMailPathStr;
        }
      else  /* default paths */
        { 
          oldIMAPLocalMailPath = oldProfilePathStr;
          oldIMAPLocalMailPath += OLD_MAIL_DIR_NAME;

	// if we are here, then PREF_MAIL_DIRECTORY was not set.
	// but we still need it in the actual pref migration
          rv = SetPremigratedFilePref(PREF_MAIL_DIRECTORY, oldIMAPLocalMailPath);
	  if (NS_FAILED(rv)) return rv;

          newIMAPLocalMailPath = newProfilePathStr;
          newIMAPLocalMailPath += NEW_MAIL_DIR_NAME;
        }
      /* Now create the new directories */
      PR_MkDir(nsNSPRPath(newIMAPLocalMailPath), NEW_DIR_PERMISSIONS);
      newIMAPLocalMailPath += NEW_LOCAL_MAIL_DIR_NAME;
      PR_MkDir(nsNSPRPath(newIMAPLocalMailPath), NEW_DIR_PERMISSIONS);
      
      /* Next get IMAP mail summary files location */
      if(NS_SUCCEEDED(GetDirFromPref(oldProfilePathStr,newProfilePathStr, NEW_IMAPMAIL_DIR_NAME, PREF_MAIL_IMAP_ROOT_DIR, &newIMAPMailPathStr, &oldIMAPMailPathStr)))
        {
          /* convert back to nsFileSpec */
          oldIMAPMailPath = oldIMAPMailPathStr;
          newIMAPMailPath = newIMAPMailPathStr;
        }
      else  /* default paths */
        { 
          oldIMAPMailPath = oldProfilePathStr;
          oldIMAPMailPath += OLD_IMAPMAIL_DIR_NAME;
      
	// if we are here, then PREF_MAIL_IMAP_ROOT_DIR was not set.
	// but we still need it in the actual pref migration
          rv = SetPremigratedFilePref(PREF_MAIL_IMAP_ROOT_DIR, oldIMAPMailPath);
          if (NS_FAILED(rv)) return rv;   

          newIMAPMailPath = newProfilePathStr;
          newIMAPMailPath += NEW_IMAPMAIL_DIR_NAME;
        }
      PR_MkDir(nsNSPRPath(newIMAPMailPath), NEW_DIR_PERMISSIONS);
    }
  else
    {
      return NS_ERROR_UNEXPECTED;
    }
  
  
  /* Create the new News directory from the setting in prefs.js or a default */
  if(NS_SUCCEEDED(GetDirFromPref(oldProfilePathStr,newProfilePathStr, NEW_NEWS_DIR_NAME, PREF_NEWS_DIRECTORY, &newNewsPathStr, &oldNewsPathStr)))
    {
      oldNewsPath = oldNewsPathStr;
      newNewsPath = newNewsPathStr;
    }
  else /* default paths */
    {
    oldNewsPath = oldProfilePathStr;
    oldNewsPath += OLD_NEWS_DIR_NAME;
 
    // if we are here, then PREF_NEWS_DIRECTORY was not set.
    // but we still need it in the actual pref migration
    rv = SetPremigratedFilePref(PREF_NEWS_DIRECTORY, oldNewsPath);
    if (NS_FAILED(rv)) return rv; 

    newNewsPath = newProfilePathStr;
    newNewsPath += NEW_NEWS_DIR_NAME;
  }
  PR_MkDir(nsNSPRPath(newNewsPath), NEW_DIR_PERMISSIONS);


  nsFileSpec oldProfilePath(oldProfilePathStr); /* nsFileSpec version of the profile's 4.x root dir */
  nsFileSpec newProfilePath(newProfilePathStr); /* Ditto for the profile's new 5.x root dir         */

#if defined(XP_UNIX) || defined(XP_MAC)
  printf("TODO:  port the code that checks for space before copying.\n");
#else  
  PRUint32 totalMailSize = 0, 
           totalNewsSize = 0, 
           totalProfileSize = 0,
           totalRequired = 0;

  rv = GetSizes(oldProfilePath, PR_FALSE, &totalProfileSize);
  rv = GetSizes(oldNewsPath, PR_TRUE, &totalNewsSize);
  if(hasIMAP)
  {
    rv = GetSizes(oldIMAPLocalMailPath, PR_TRUE, &totalMailSize);
  }
  else
  {
    rv = GetSizes(oldPOPMailPath, PR_TRUE, &totalMailSize);
  }

  /* Get the drive name or letter for the profile tree */
  char *profile_hd_name = nsnull;
  GetDriveName(oldProfilePath, &profile_hd_name);
  
  /* Get the drive name or letter for the mail (IMAP-local or POP) tree */
  char *mail_hd_name = nsnull;
  if (hasIMAP)
    GetDriveName(oldIMAPLocalMailPath, &mail_hd_name);
  else
    GetDriveName(oldPOPMailPath, &mail_hd_name);

  /* Get the drive name or letter for the news tree */
  char *news_hd_name = nsnull;
  GetDriveName(oldNewsPath, &news_hd_name);

  
  /* Check to see if all the dirs are on the same drive (the default case) */
  if((PL_strcmp(profile_hd_name, mail_hd_name) == 0) &&
     (PL_strcmp(profile_hd_name, news_hd_name) == 0)) /* All dirs are on the same drive */
  {
    totalRequired = totalProfileSize + totalMailSize + totalNewsSize;
    if(NS_FAILED(CheckForSpace(newProfilePath, totalRequired)))
    {
	    return NS_ERROR_ABORT;  /* Need error code for not enough space */
    }
  }
  else
  {
    if(PL_strcmp(mail_hd_name, news_hd_name) == 0) /* Mail and news on same drive */
    {
      totalRequired = totalMailSize + totalNewsSize;
	  if (hasIMAP)
	  {
        if(NS_FAILED(CheckForSpace(newIMAPLocalMailPath, totalRequired)))
		{
        return NS_ERROR_ABORT; /* Need error code for not enough space */
		}
	  }
	  else
	  {
        if(NS_FAILED(CheckForSpace(newPOPMailPath, totalRequired)))
		{
        return NS_ERROR_ABORT; /* Need error code for not enough space */
		}
	  }
      if(NS_FAILED(CheckForSpace(newProfilePath, totalProfileSize)))
      {
        return NS_ERROR_ABORT; /* Need error code for not enough space */
      }
    }
    else
    {
      if(PL_strcmp(profile_hd_name, mail_hd_name) == 0) /* Mail and profile on same drive */
      {
        totalRequired = totalProfileSize + totalMailSize;
        if(NS_FAILED(CheckForSpace(newProfilePath, totalRequired)))
        {
          return NS_ERROR_ABORT;  /* Need error code for not enough space */
        }
        if(NS_FAILED(CheckForSpace(newNewsPath, totalNewsSize)))
        {
          return NS_ERROR_ABORT;  /* Need error code for not enough space */
        }
      }
      else
      {
        if(PL_strcmp(profile_hd_name, news_hd_name) == 0) /* News and profile on same drive */
        {
          totalRequired = totalProfileSize + totalNewsSize;
          if(NS_FAILED(CheckForSpace(newProfilePath, totalRequired)))
          {
            return NS_ERROR_ABORT;  /* Need error code for not enough space */
          }
		  if (hasIMAP)
		  {
            if(NS_FAILED(CheckForSpace(newIMAPMailPath, totalMailSize)))
			{
              return NS_ERROR_ABORT;  /* Need error code for not enough space */
			}
		  }
		  else
		  {
            if(NS_FAILED(CheckForSpace(newPOPMailPath, totalMailSize)))
			{
              return NS_ERROR_ABORT;  /* Need error code for not enough space */
			}
		  }
        }
        else
        {
          /* All the trees are on different drives */
		  if (hasIMAP)
		  {
            if(NS_FAILED(CheckForSpace(newIMAPMailPath, totalMailSize)))
			{
              return NS_ERROR_ABORT;  /* Need error code for not enough space */
			}
		  }
		  else
		  {
            if(NS_FAILED(CheckForSpace(newPOPMailPath, totalMailSize)))
			{
              return NS_ERROR_ABORT;  /* Need error code for not enough space */
			}
		  }
          if(NS_FAILED(CheckForSpace(newNewsPath, totalNewsSize)))
          {
            return NS_ERROR_ABORT;  /* Need error code for not enough space */
          }
          if(NS_FAILED(CheckForSpace(newProfilePath, totalProfileSize)))
          {
            return NS_ERROR_ABORT;  /* Need error code for not enough space */
          }
        }
      } /* else */
    } /* else */
  } /* else */

  PR_FREEIF(profile_hd_name);
  PR_FREEIF(mail_hd_name);
  PR_FREEIF(news_hd_name);
#endif /* XP_UNIX || XP_MAC */

  PRBool needToRenameFilterFiles;
  if (PL_strcmp(MAIL_FILTER_FILE_NAME_IN_4x,MAIL_FILTER_FILE_NAME_IN_5x)) {
#ifdef MAIL_FILTER_FILE_SUFFIX_IN_4x
    // if our filter files had a suffix (the weren't all named the same thing)
    // we don't need to rename them when we copy.
    // we do that later.
    needToRenameFilterFiles = PR_FALSE;
#else
    needToRenameFilterFiles = PR_TRUE;
#endif /* MAIL_FILTER_FILE_SUFFIX_IN_4x */
  }
  else {
    // if the name was the same in 4x as in 5x, no need to rename it
    needToRenameFilterFiles = PR_FALSE;
  }
  
  rv = DoTheCopy(oldProfilePath, newProfilePath, PR_FALSE);
  if (NS_FAILED(rv)) return rv;
  rv = DoTheCopy(oldNewsPath, newNewsPath, PR_TRUE);
  if (NS_FAILED(rv)) return rv;
#ifdef XP_UNIX
  printf("TODO: do we need to copy the .newsrc files?\n");
#endif /* XP_UNIX */
  if(hasIMAP)
  {
    rv = DoTheCopyAndRename(oldIMAPMailPath, newIMAPMailPath, PR_TRUE, needToRenameFilterFiles,MAIL_FILTER_FILE_NAME_IN_4x,MAIL_FILTER_FILE_NAME_IN_5x);
    if (NS_FAILED(rv)) return rv;
    rv = DoTheCopyAndRename(oldIMAPLocalMailPath, newIMAPLocalMailPath, PR_TRUE, needToRenameFilterFiles,MAIL_FILTER_FILE_NAME_IN_4x,MAIL_FILTER_FILE_NAME_IN_5x);
    if (NS_FAILED(rv)) return rv;
  }
  else {
    rv = DoTheCopyAndRename(oldPOPMailPath, newPOPMailPath, PR_TRUE,needToRenameFilterFiles,MAIL_FILTER_FILE_NAME_IN_4x,MAIL_FILTER_FILE_NAME_IN_5x);
    if (NS_FAILED(rv)) return rv;
  }

  rv=DoSpecialUpdates(newProfilePath);
  if (NS_FAILED(rv)) return rv;

//  m_progressWindow->Close(); /* Close the progress dialog window */

  PR_FREEIF(oldPOPMailPathStr);
  PR_FREEIF(oldIMAPMailPathStr);
  PR_FREEIF(oldIMAPLocalMailPathStr);
  PR_FREEIF(oldNewsPathStr);
  PR_FREEIF(newPOPMailPathStr);
  PR_FREEIF(newIMAPMailPathStr);
  PR_FREEIF(newIMAPLocalMailPathStr);
  PR_FREEIF(newNewsPathStr);
  PR_FREEIF(popServerName);

  rv=m_prefs->SavePrefFileAs(m_prefsFile);
  if (NS_FAILED(rv)) return rv;
  rv=m_prefs->ResetPrefs();
  if (NS_FAILED(rv)) return rv;

  return NS_OK;
}


/*----------------------------------------------------------------------------
 * CreateNewUsers5Tree creates the directory called users5 (parent of the
 * of the profile directories) and the profile directory itself
 *---------------------------------------------------------------------------*/

nsresult
nsPrefMigration::CreateNewUser5Tree(const char* oldProfilePath, const char* newProfilePath)
{
  nsresult rv;
  
  NS_ASSERTION((PL_strlen(PREF_FILE_NAME_IN_4x) > 0), "don't know how to migrate your platform");
  if (PL_strlen(PREF_FILE_NAME_IN_4x) == 0) {
    return NS_ERROR_UNEXPECTED;
  }
      
  /* Copy the old prefs file to the new profile directory for modification and reading.  after copying it, rename it to pref.js, the 5.x pref file name on all platforms */

  nsFileSpec oldPrefsFile(oldProfilePath);
  oldPrefsFile += PREF_FILE_NAME_IN_4x;

  nsFileSpec newPrefsFile(newProfilePath);
      
  if (!newPrefsFile.Exists())
  {
	  newPrefsFile.CreateDirectory();
  }

  oldPrefsFile.Copy(newPrefsFile);

  newPrefsFile += PREF_FILE_NAME_IN_4x;
  newPrefsFile.Rename(PREF_FILE_NAME_IN_5x);
 
  rv = getPrefService();
  if (NS_FAILED(rv)) return rv;

  /* initialize prefs with the new prefs.js file (which is a copy of the 4.x preferences file) */
  rv = NS_NewFileSpecWithSpec(newPrefsFile, getter_AddRefs(m_prefsFile));
  if (NS_FAILED(rv)) return rv;
  m_prefs->ReadUserPrefsFrom(m_prefsFile);

  return NS_OK;
}

/*---------------------------------------------------------------------------------
 * GetDirFromPref gets a directory based on a preference set in the 4.x
 * preferences file, adds a 5 and resets the preference.
 *
 * INPUT: 
 *         oldProfilePath - the path to the old 4.x profile directory.  
 *                          currently only used by UNIX
 *
 *         newProfilePath - the path to the 5.0 profile directory
 *                          currently only used by UNIX
 *
 *         newDirName     - the leaf name of the directory in the 5.0 world that corresponds to
 *                          this pref.  Examples:  "Mail", "ImapMail", "News".
 *                          only used on UNIX.
 *
 *         pref - the pref in the "dot" format (e.g. mail.directory)
 *
 * OUTPUT: newPath - The old path with a 5 added (on mac and windows)
 *                   the newProfilePath + "/" + newDirName (on UNIX)
 *         oldPath - The old path from the pref (if any)
 *
 *
 * RETURNS: NS_OK if the pref was successfully pulled from the prefs file
 *
 *--------------------------------------------------------------------------------*/
nsresult
nsPrefMigration::GetDirFromPref(const char *oldProfilePath, const char* newProfilePath, const char *newDirName, char* pref, char** newPath, char** oldPath)
{
  PRInt32 foundPref;
  nsresult rv;

  if (!oldProfilePath || !newProfilePath || !newDirName || !pref || !newPath || !oldPath) return NS_ERROR_NULL_POINTER;
  NS_ASSERTION(!*newPath,"*newPath should be null");
  NS_ASSERTION(!*oldPath,"*oldPath should be null");

  rv = getPrefService();
  if (NS_FAILED(rv)) return rv;   

  foundPref = m_prefs->CopyCharPref(pref, oldPath);
  if((foundPref == 0) && (*oldPath) && (PL_strlen(*oldPath) > 0))
  {
#ifdef XP_UNIX
    // what if they don't want to go to <profile>/<newDirName>?
    // what if unix users want "mail.directory" + "5" (like "~/ns_imap5")
    // or "mail.imap.root_dir" + "5" (like "~/nsmail5")?
    // should we let them? no.  let's migrate them to
    // <profile>/Mail and <profile>/ImapMail
    // let's make all three platforms the same.
    *newPath = PR_smprintf("%s/%s",newProfilePath,newDirName);
#else
    *newPath = PR_smprintf("%s%s",*oldPath,NEW_DIR_SUFFIX);
#endif /* XP_UNIX */

    rv = SetPremigratedCharPref(pref, *oldPath);
    if (NS_FAILED(rv)) return rv;

#ifdef XP_UNIX
    /* on UNIX, we kept the newsrc files in "news.directory", (which was usually ~)
     * and the summary files in ~/.netscape/xover-cache
     * oldPath should point to ~/.netscape/xover-cache, not "news.directory"
     * but we want to save the old "news.directory" in "premigration.news.directory"
     * later, again for UNIX only, 
     * we will copy the .newsrc files (from "news.directory") into the new <profile>/News directory.
     * isn't this fun?  
     */
    if (PL_strcmp(PREF_NEWS_DIRECTORY, pref) == 0) {
      PR_FREEIF(*oldPath);
      *oldPath = PR_smprintf("%s%s",oldProfilePath,OLD_NEWS_DIR_NAME);
    }
#endif /* XP_UNIX */

    m_prefs->SetCharPref(pref, *newPath); 
    return NS_OK;
  }
  else
    return NS_ERROR_ABORT;
  
}

static PRBool
nsStringEndsWith(nsString& name, const char *ending)
{
  if (!ending) return PR_FALSE;

  PRInt32 len = name.Length();
  if (len == 0) return PR_FALSE;

  PRInt32 endingLen = PL_strlen(ending);
  if (len > endingLen && name.RFind(ending, PR_TRUE) == len - endingLen) {
        return PR_TRUE;
  }
  else {
        return PR_FALSE;
  }
} 
/*---------------------------------------------------------------------------------
 * GetSizes reads the 4.x files in the profile tree and accumulates their sizes
 *
 * INPUT:
 *
 * OUPUT:
 *
 * RETURNS:
 *
 *--------------------------------------------------------------------------------*/
nsresult
nsPrefMigration::GetSizes(nsFileSpec inputPath, PRBool readSubdirs, PRUint32 *sizeTotal)
{
  char* folderName;
  nsAutoString fileOrDirNameStr;

  for (nsDirectoryIterator dir(inputPath, PR_FALSE); dir.Exists(); dir++)
  {
    nsFileSpec fileOrDirName = (nsFileSpec&)dir;
    folderName = fileOrDirName.GetLeafName();
    fileOrDirNameStr = folderName;
    if (nsStringEndsWith(fileOrDirNameStr, SUMMARY_SUFFIX_IN_4x) || nsStringEndsWith(fileOrDirNameStr, SUMMARY_SUFFIX_IN_5x)) /* Don't copy the summary files */
      continue;
    else
    {
      if (fileOrDirName.IsDirectory())
      {
        if(readSubdirs)
        {
          GetSizes(fileOrDirName, PR_TRUE, sizeTotal); /* re-enter the GetSizes function */
        }
        else
          continue;
      }
      else
        *sizeTotal += fileOrDirName.GetFileSize();
    }
  }

  return NS_OK;
}


/*--------------------------------------------------------------------------
 * GetDriveName gets the drive letter (on Windows) or the volume name (on Mac)
 *
 * INPUT: an nsFileSpec path
 * 
 * OUTPUT: the drive letter or volume name
 * 
 * RETURNS: NS_OK if found
 *          NS_ERROR_FAILURE if some error occurs while iterating through the
 *                           parent nodes.
 *
 *--------------------------------------------------------------------------*/

static
nsresult GetStringFromSpec(nsFileSpec inSpec, char **string)
{
	nsresult rv;
        nsCOMPtr<nsIFileSpec> spec;
        rv = NS_NewFileSpecWithSpec(inSpec, getter_AddRefs(spec));
        if (NS_SUCCEEDED(rv)) {
        		rv = spec->GetNativePath(string);
                if (NS_SUCCEEDED(rv)) {
                        return NS_OK;
                }
                else {
                        PR_FREEIF(*string);
                        return rv;
                }
        }
        else {
                *string = nsnull;
                return rv;
        }
}                       

nsresult
nsPrefMigration::GetDriveName(nsFileSpec inputPath, char**driveName)
{
  nsresult rv;
  if (!driveName) return NS_ERROR_NULL_POINTER;
  NS_ASSERTION(!*driveName,"*driveName should be null");
 
  nsFileSpec oldParent, newParent;
  PRBool foundIt = PR_FALSE;

  inputPath.GetParent(oldParent); /* do initial pass to optimize one directory case */
  while (!foundIt)
  {
    PR_FREEIF(*driveName);
    rv = GetStringFromSpec(oldParent,driveName);
    
    oldParent.GetParent(newParent);
    /* Since GetParent doesn't return an error if there's no more parents
     * I have to compare the results of the parent value (string) to see 
     * if they've changed. */
    if (oldParent == newParent) 
      foundIt = PR_TRUE;
    else
      oldParent = newParent;
  }
  return NS_OK;
}

/*--------------------------------------------------------------------------
 * CheckForSpace checks the target drive for enough space
 *
 * INPUT: newProfilePath - The path to the new profile
 *        requiredSpace - The space needed on the new profile drive
 *
 * RETURNS: NS_OK if enough space is available
 *          NS_ERROR_FAILURE if there is not enough space
 *
 * Todo: you may want to change this proto from a float to a int64.
 *--------------------------------------------------------------------------*/
nsresult
nsPrefMigration::CheckForSpace(nsFileSpec newProfilePath, PRFloat64 requiredSpace)
{
  nsFileSpec drive(newProfilePath);
  PRInt64 availSpace = newProfilePath.GetDiskSpaceAvailable();
  PRInt64 require64;
  
  LL_F2L(require64, requiredSpace);
  if (LL_CMP(availSpace, <, require64))
    return NS_ERROR_FAILURE;
  return NS_OK;
}

/*-------------------------------------------------------------------------
 * DoTheCopyAndRename copies the files listed in oldPath to newPath
 *                    and renames files, if necessary
 *
 * INPUT: oldPath - The old profile path plus the specific data type 
 *                  (e.g. mail or news)
 *        newPath - The new profile path plus the specific data type
 *
 *        readSubdirs
 *
 *        needToRenameFiles - do we need to search for files named oldFile
 *                            and rename them to newFile
 *
 *        oldFile           - old file name (used for renaming)
 *
 *        newFile           - new file name (used for renaming)
 *
 * RETURNS: NS_OK if successful
 *          NS_ERROR_FAILURE if failed
 *
 *--------------------------------------------------------------------------*/
nsresult
nsPrefMigration::DoTheCopyAndRename(nsFileSpec oldPath, nsFileSpec newPath, PRBool readSubdirs, PRBool needToRenameFiles, const char *oldName, const char *newName)
{
  char* folderName = nsnull;
  nsAutoString fileOrDirNameStr;
 
  for (nsDirectoryIterator dir(oldPath, PR_FALSE); dir.Exists(); dir++)
  {
    nsFileSpec fileOrDirName = (nsFileSpec&)dir; //set first file or dir to a nsFileSpec
    folderName = fileOrDirName.GetLeafName();    //get the filename without the full path
    fileOrDirNameStr = folderName;

    if (nsStringEndsWith(fileOrDirNameStr, SUMMARY_SUFFIX_IN_4x) || nsStringEndsWith(fileOrDirNameStr, SUMMARY_SUFFIX_IN_5x)) /* Don't copy the summary files */
      continue;
    else
    {
      if (fileOrDirName.IsDirectory())
      {
        if(readSubdirs)
        {
          nsFileSpec newPathExtended = newPath;
          newPathExtended += folderName;
          newPathExtended.CreateDirectory();
          DoTheCopyAndRename(fileOrDirName, newPathExtended, PR_TRUE, needToRenameFiles, oldName, newName); /* re-enter the DoTheCopyAndRename function */
        }
        else
          continue;
      }
      else {
        // copy the file
        fileOrDirName.Copy(newPath);

        if (needToRenameFiles) {
          // rename the file, if it matches
          if (fileOrDirNameStr == oldName) {
            nsFileSpec newFile = newPath;
            newFile += fileOrDirNameStr;
            newFile.Rename(newName);
          }
        }
      }
    }
  }  
  
  return NS_OK;
}


nsresult
nsPrefMigration::DoTheCopy(nsFileSpec oldPath, nsFileSpec newPath, PRBool readSubdirs)
{
  return DoTheCopyAndRename(oldPath, newPath, readSubdirs, PR_FALSE, "", "");
}
 
/*----------------------------------------------------------------------------
 * DoSpecialUpdates updates is a routine that does some miscellaneous updates 
 * like renaming certain files, etc.
 *
 *--------------------------------------------------------------------------*/
nsresult
nsPrefMigration::DoSpecialUpdates(nsFileSpec profilePath)
{
  nsresult rv;
  
#if defined(XP_MAC)
  printf("TODO: move and rename the mail filter files\n"); 
#endif /* XP_MAC*/

  nsFileSpec fs(profilePath);
  fs += PREF_FILE_NAME_IN_5x;
  
  nsOutputFileStream fsStream(fs, (PR_WRONLY | PR_CREATE_FILE | PR_APPEND));
  
  if (!fsStream.is_open())
  {
    return NS_ERROR_FAILURE;
  }

  /* Need to add a string to the top of the prefs.js file to prevent it
   * from being loaded as a standard javascript file which would be a
   * security hole.
   */
  fsStream << PREF_FILE_HEADER_STRING << nsEndl ;

  // rename the cookies file, but only if we need to.
  rv = Rename4xFileAfterMigration(profilePath,COOKIES_FILE_NAME_IN_4x,COOKIES_FILE_NAME_IN_5x);
  if (NS_FAILED(rv)) return rv;

  // rename the bookmarks file, but only if we need to.
  rv = Rename4xFileAfterMigration(profilePath,BOOKMARKS_FILE_NAME_IN_4x,BOOKMARKS_FILE_NAME_IN_5x);
  if (NS_FAILED(rv)) return rv;

#ifdef MAIL_FILTER_FILE_NAME_SUFFIX_IN_4x
  rv = RenameAndMoveFilterFiles(profilePath);
  if (NS_FAILED(rv)) return rv;
#endif /* MAIL_FILTER_FILE_NAME_SUFFIX_IN_4x */
     
  fsStream.close();

  return rv;
}

nsresult
nsPrefMigration::RenameAndMoveFilterFiles(nsFileSpec profilePath)
{
  // unlike windows and unix, in 4.x the mac stored the filter files in
  // <profile>/<hostname> Rules
  // instead of
  // <profile>/ImapMail/<hostname>/rules.dat
  //
  // find all files in profilePath that end with MAIL_FILTER_FILE_NAME_SUFFIX_IN_4x
  // hostname = filename - MAIL_FILTER_FILE_NAME_SUFFIX_IN_4x
  // copy file to Mail/<hostname>/MAIL_FILTER_NAME_IN_5x
  // or
  // copy file to ImapMail/<hostname>/MAIL_FILTER_NAME_IN_5x
  return NS_OK;
}

nsresult
nsPrefMigration::Rename4xFileAfterMigration(nsFileSpec profilePath, const char *oldFileName, const char *newFileName)
{
  // if they are the same, don't bother to rename the file.
  if (PL_strcmp(oldFileName, newFileName) == 0) {
    return NS_OK;
  }
               
  nsFileSpec file(profilePath);

  file += oldFileName;
  
  // make sure it exists before you try to rename it
  if (file.Exists()) {
    file.Rename(newFileName);
  }
  return NS_OK;
}

////////////////////////////////////////////////////////////////////////////////
// DLL Entry Points:
////////////////////////////////////////////////////////////////////////////////

extern "C" NS_EXPORT PRBool
NSCanUnload(nsISupports* serviceMgr)
{
  return PRBool (gInstanceCnt == 0 && gLockCnt == 0);
}

extern "C" NS_EXPORT nsresult
NSRegisterSelf(nsISupports* aServMgr, const char *path)
{
  nsresult rv;
  //nsRepository::RegisterComponent(kPrefMigrationCID, "Profile Migration", "component://netscape/prefmigration", path, PR_TRUE, PR_TRUE);
  NS_WITH_SERVICE1(nsIComponentManager, compMgr, aServMgr, kComponentManagerCID, &rv);
  if (NS_FAILED(rv)) return rv;

  rv = compMgr->RegisterComponent(kPrefMigrationCID, "Profile Migration",
                                  NS_PROFILEMIGRATION_PROGID,
                                  path,
                                  PR_TRUE,
                                  PR_TRUE);
  return rv;
}

extern "C" NS_EXPORT nsresult
NSUnregisterSelf(nsISupports* aServMgr, const char *path)
{
  nsresult rv;
  //nsRepository::UnregisterFactory(kPrefMigrationCID, path);

  NS_WITH_SERVICE1(nsIComponentManager, compMgr, aServMgr, kComponentManagerCID, &rv);
  if (NS_FAILED(rv))
    return rv;
  rv = compMgr->UnregisterComponent(kPrefMigrationCID, path);

  return rv;
}


extern "C" NS_EXPORT nsresult
NSGetFactory(nsISupports* serviceMgr,
             const nsCID &aClass,
             const char *aClassName,
             const char *aProgID,
             nsIFactory **aFactory)
{
  if (aFactory == NULL)
  {
    return NS_ERROR_NULL_POINTER;
  }

  *aFactory = NULL;
  nsISupports *inst;

  if ( aClass.Equals(kPrefMigrationCID) )
  {
    inst = new nsPrefMigrationFactory();        
  }
  else
  {
    return NS_ERROR_ILLEGAL_VALUE;
  }

  if (inst == NULL)
  {   
    return NS_ERROR_OUT_OF_MEMORY;
  }

  nsresult res = inst->QueryInterface(kIFactoryIID, (void**) aFactory);

  if (NS_FAILED(res))
  {   
    delete inst;
  }

  return res;
}

nsresult 
nsPrefMigration::SetPremigratedFilePref(const char *pref_name, nsFileSpec &filePath)
{
	nsresult rv;
	nsCOMPtr <nsIFileSpec> path;
	rv = NS_NewFileSpecWithSpec(filePath, getter_AddRefs(path));
	if (NS_FAILED(rv)) return rv; 

	// save off the old pref, prefixed with "premigration"
	// for example, we need the old "mail.directory" pref when
	// migrating the copies and folder prefs in nsMsgAccountManager.cpp
	//
	// note we do this for all platforms.
	char *premigration_pref = nsnull;
	premigration_pref = PR_smprintf("%s.%s", PREMIGRATION_PREFIX,pref_name);
	if (!premigration_pref) return NS_ERROR_FAILURE;
#ifdef NS_DEBUG
	printf("setting %s (from a nsFileSpec) for later...\n", premigration_pref);
#endif
	rv = m_prefs->SetFilePref(premigration_pref, path, PR_FALSE /* set default */);
	PR_FREEIF(premigration_pref);   
	return rv;
}



nsresult
nsPrefMigration::SetPremigratedCharPref(const char *pref_name, char *value)
{
	nsresult rv;
	// save off the old pref, prefixed with "premigration"
	// for example, we need the old "mail.directory" pref when
	// migrating the copies and folder prefs in nsMsgAccountManager.cpp
	//
	// note we do this for all platforms.
	char *premigration_pref = nsnull;
	premigration_pref = PR_smprintf("%s.%s", PREMIGRATION_PREFIX,pref_name);
	if (!premigration_pref) return NS_ERROR_FAILURE;
#ifdef NS_DEBUG
	printf("setting %s (from a char *) for later...\n", premigration_pref);
#endif
	rv = m_prefs->SetCharPref(premigration_pref, value);
	PR_FREEIF(premigration_pref);   
	return rv;
}

/* called if the prefs service goes offline */
NS_IMETHODIMP
nsPrefMigration::OnShutdown(const nsCID& aClass, nsISupports *service)
{
  if (aClass.Equals(kPrefServiceCID)) {
    if (m_prefs) nsServiceManager::ReleaseService(kPrefServiceCID, m_prefs);
    m_prefs = nsnull;
  }

  return NS_OK;
}

/* This works.  Saving for reference 

PRInt32    
nsPrefMigration::ShowProgressDialog(nsString& string)
{
    nsresult res;  

    NS_WITH_SERVICE(nsIPrompt, dialog, kNetSupportDialogCID,  &res);
    if (NS_FAILED(res)) 
        return res;
    
    return dialog->Alert(string.GetUnicode());
}
*/

#if 0

PRInt32
nsPrefMigration::ShowProgressDialog(const PRUnichar *inWindowTitle, const PRUnichar *inMsg)
{
    nsresult rv;
    nsString text;

    enum {  eMsg =0, eCheckboxMsg =1, eIconURL =2 , eTitleMessage =3, 
            eEditfield1Msg =4, eEditfield2Msg =5, eEditfield1Value = 6, 
            eEditfield2Value = 7,	eButton0Text = 8, eButton1Text = 9, 
            eButton2Text =10, eButton3Text = 11,eDialogTitle = 12 };

    enum { eButtonPressed = 0, eCheckboxState = 1, eNumberButtons = 2, 
           eNumberEditfields =3, eEditField1Password =4 };


    NS_WITH_SERVICE(nsIWindowMediator, windowMediator, kWindowMediatorCID, &rv);
    if ( NS_SUCCEEDED ( rv ) )
    {
      //nsCOMPtr< nsIDOMWindow> window;
      windowMediator->GetMostRecentWindow( NULL, getter_AddRefs( m_parentWindow ) );
	    
      nsCOMPtr< nsIDialogParamBlock> block;
	    rv = nsComponentManager::CreateInstance( kDialogParamBlockCID,
                                               0,
                                               nsIDialogParamBlock::GetIID(),
                                               (void**)&block );
      
	    if ( NS_FAILED( rv ) )
		    return rv;
	    block->SetInt( eNumberButtons, 1 );

      ShowPMDialogEngine(block, kProgressURL);

      //nsCOMPtr<nsICommonDialogs> dialogService;
      //rv = nsComponentManager::CreateInstance( kCommonDialogsCID,
      //                                         0, 
      //                                         nsICommonDialogs::GetIID(),
      //                                         (void**)&dialogService );

      //if( NS_SUCCEEDED ( rv ) )
      //  rv = dialogService->DoDialog( window, block, kProgressURL);
        //rv = dialogService->Alert( window, NULL, text.GetUnicode());
    
    }
    return rv;
}


nsresult
nsPrefMigration::ShowPMDialogEngine(nsIDialogParamBlock *ioParamBlock, const char *inChromeURL)
{
  nsresult rv = NS_OK;

    if ( m_parentWindow && ioParamBlock &&inChromeURL )
    {
        // Get JS context from parent window.
        nsCOMPtr<nsIScriptGlobalObject> sgo = do_QueryInterface( m_parentWindow, &rv );
        if ( NS_SUCCEEDED( rv ) && sgo )
        {
            nsCOMPtr<nsIScriptContext> context;
            sgo->GetContext( getter_AddRefs( context ) );
            if ( context )
            {
                JSContext *jsContext = (JSContext*)context->GetNativeContext();
                if ( jsContext ) {
                    void *stackPtr;
                    jsval *argv = JS_PushArguments( jsContext,
                                                    &stackPtr,
                                                    "svs%ip",
                                                    inChromeURL,
                                                    JSVAL_NULL,
                                                    "chrome",
                                                    (const nsIID*)(&nsIDialogParamBlock::GetIID()),
                                                    (nsISupports*)ioParamBlock
                                                  );
                    if ( argv ) {
                        //nsIDOMWindow *newWindow;
                        rv = m_parentWindow->Open( jsContext, argv, 4, &m_progressWindow );
                        if ( NS_SUCCEEDED( rv ) )
                        {
                            m_progressWindow->Release();
                        } else
                        {
                        }
                        JS_PopArguments( jsContext, stackPtr );
                    }
                    else
                    {
                    	
                        NS_WARNING( "JS_PushArguments failed\n" );
                        rv = NS_ERROR_FAILURE;
                    }
                }
                else
                {
                    NS_WARNING(" GetNativeContext failed\n" );
                    rv = NS_ERROR_FAILURE;
                }
            }
            else
            {
                NS_WARNING( "GetContext failed\n" );
                rv = NS_ERROR_FAILURE;
            }
        }
        else
        {
            NS_WARNING( "QueryInterface (for nsIScriptGlobalObject) failed \n" );
        }
    }
    else
    {
        NS_WARNING( " OpenDialogWithArg was passed a null pointer!\n" );
        rv = NS_ERROR_NULL_POINTER;
    }
    return rv;
 }

#endif /* 0 */