/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
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
 * The Original Code is Mozilla Communicator client code, 
 * released March 31, 1998. 
 *
 * The Initial Developer of the Original Code is Netscape Communications 
 * Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 *     Daniel Veditz <dveditz@netscape.com>
 *     Douglas Turner <dougt@netscape.com>
 *     Pierre Phaneuf <pp@ludusdesign.com>
 */



#include "nscore.h"
#include "nsIFactory.h"
#include "nsISupports.h"

#include "nsRepository.h"
#include "nsIServiceManager.h"

#include "nsHashtable.h"
#include "nsFileSpec.h"
#include "nsFileStream.h"
#include "nsSpecialSystemDirectory.h"
#include "nsDirectoryService.h"
#include "nsDirectoryServiceDefs.h"

#include "nsIPref.h"

#include "prmem.h"
#include "plstr.h"
#include "prprf.h"

#include "VerReg.h"

#include "nsInstall.h"
#include "nsInstallFolder.h"
#include "nsInstallVersion.h"
#include "nsInstallFile.h"
#include "nsInstallDelete.h"
#include "nsInstallExecute.h"
#include "nsInstallPatch.h"
#include "nsInstallUninstall.h"
#include "nsInstallResources.h"
#include "nsRegisterItem.h"
#include "nsNetUtil.h"

#include "nsProxiedService.h"
#include "nsICommonDialogs.h"
#include "nsIPrompt.h"

#ifdef _WINDOWS
#include "nsWinReg.h"
#include "nsWinProfile.h"
#endif

#include "nsInstallFileOpEnums.h"
#include "nsInstallFileOpItem.h"

#ifdef XP_MAC
#include "Gestalt.h"
#include "nsAppleSingleDecoder.h"
#include "nsILocalFileMac.h"
#endif 

#include "nsILocalFile.h"
#include "nsIURL.h"

static NS_DEFINE_IID(kEventQueueServiceCID, NS_EVENTQUEUESERVICE_CID);
static NS_DEFINE_IID(kIEventQueueServiceIID, NS_IEVENTQUEUESERVICE_IID);

static NS_DEFINE_CID(kCommonDialogsCID, NS_CommonDialog_CID);

static NS_DEFINE_CID(kStringBundleServiceCID, NS_STRINGBUNDLESERVICE_CID);
static NS_DEFINE_IID(kIStringBundleServiceIID, NS_ISTRINGBUNDLESERVICE_IID);
	
MOZ_DECL_CTOR_COUNTER(nsInstallInfo);

nsInstallInfo::nsInstallInfo(PRUint32           aInstallType,
                             nsIFile*           aFile, 
                             const PRUnichar*   aURL,
                             const PRUnichar*   aArgs, 
                             PRUint32           flags, 
                             nsIXPIListener*    aListener,
                             nsIDOMWindowInternal* aParentWindow,
                             nsIChromeRegistry* aChromeReg)
: mError(0),
  mType(aInstallType),
  mFlags(flags),
  mURL(aURL),
  mArgs(aArgs), 
  mFile(aFile), 
  mListener(aListener),
  mParent(aParentWindow),
  mChromeReg(aChromeReg)
{
    MOZ_COUNT_CTOR(nsInstallInfo);
}


nsInstallInfo::~nsInstallInfo()
{
  MOZ_COUNT_DTOR(nsInstallInfo);
}

static NS_DEFINE_CID(kStandardURLCID, NS_STANDARDURL_CID);
  
static NS_DEFINE_IID(kISoftwareUpdateIID, NS_ISOFTWAREUPDATE_IID);
static NS_DEFINE_IID(kSoftwareUpdateCID,  NS_SoftwareUpdate_CID);

static NS_DEFINE_IID(kIZipReaderIID, NS_IZIPREADER_IID);
static NS_DEFINE_IID(kZipReaderCID,  NS_ZIPREADER_CID);

MOZ_DECL_CTOR_COUNTER(nsInstall);

nsInstall::nsInstall(nsIZipReader * theJARFile)
{
    MOZ_COUNT_CTOR(nsInstall);

    mScriptObject           = nsnull;           // this is the jsobject for our context
    mVersionInfo            = nsnull;           // this is the version information passed to us in StartInstall()
    mInstalledFiles         = nsnull;           // the list of installed objects
//  mRegistryPackageName    = "";               // this is the name that we will add into the registry for the component we are installing
//  mUIName                 = "";               // this is the name that will be displayed in UI.
    mPatchList              = nsnull;
    mUninstallPackage       = PR_FALSE;
    mRegisterPackage        = PR_FALSE;
    mStatusSent             = PR_FALSE;
    mStartInstallCompleted  = PR_FALSE;
    mJarFileLocation        = nsnull;
    //mInstallArguments       = "";
    mPackageFolder          = nsnull;

    // mJarFileData is an opaque handle to the jarfile.
    mJarFileData = theJARFile;

    nsISoftwareUpdate *su;
    nsresult rv = nsServiceManager::GetService(kSoftwareUpdateCID, 
                                               kISoftwareUpdateIID,
                                               (nsISupports**) &su);
    
    if (NS_SUCCEEDED(rv))
    {
        su->GetMasterListener( getter_AddRefs(mListener) );
    }

    su->Release();

    // get the resourced xpinstall string bundle
    mStringBundle = nsnull;
    NS_WITH_PROXIED_SERVICE( nsIStringBundleService, 
                             service, 
                             kStringBundleServiceCID, 
                             NS_UI_THREAD_EVENTQ, 
                             &rv );

    if (NS_SUCCEEDED(rv) && service)
    {
        nsILocale* locale = nsnull;
        rv = service->CreateBundle( XPINSTALL_BUNDLE_URL, locale,
                                    getter_AddRefs(mStringBundle) );
    }
}

nsInstall::~nsInstall()
{
    if (mVersionInfo != nsnull)
        delete mVersionInfo;

    if (mPackageFolder)
        delete mPackageFolder;

    MOZ_COUNT_DTOR(nsInstall);
}

PRInt32
nsInstall::SetScriptObject(void *aScriptObject)
{
  mScriptObject = (JSObject*) aScriptObject;
  return NS_OK;
}

#ifdef _WINDOWS
PRInt32
nsInstall::SaveWinRegPrototype(void *aScriptObject)
{
  mWinRegObject = (JSObject*) aScriptObject;
  return NS_OK;
}
PRInt32
nsInstall::SaveWinProfilePrototype(void *aScriptObject)
{
  mWinProfileObject = (JSObject*) aScriptObject;
  return NS_OK;
}

JSObject*
nsInstall::RetrieveWinRegPrototype()
{
  return mWinRegObject;
}

JSObject*
nsInstall::RetrieveWinProfilePrototype()
{
  return mWinProfileObject;
}
#endif


PRInt32    
nsInstall::GetUserPackageName(nsString& aUserPackageName)
{
  aUserPackageName = mUIName;
  return NS_OK;
}

PRInt32    
nsInstall::GetRegPackageName(nsString& aRegPackageName)
{
  aRegPackageName = mRegistryPackageName;
  return NS_OK;
}

void
nsInstall::InternalAbort(PRInt32 errcode)
{
    if (mListener)
    {
        mListener->FinalStatus(mInstallURL.GetUnicode(), errcode);
        mStatusSent = PR_TRUE;
    }

    nsInstallObject* ie;
    if (mInstalledFiles != nsnull) 
    {
        for (PRInt32 i = mInstalledFiles->Count(); i >= 0; i--) 
        {
            ie = (nsInstallObject *)mInstalledFiles->ElementAt(i);
            if (ie) 
                ie->Abort();
        }
    }
    
    CleanUp();
}

PRInt32    
nsInstall::AbortInstall(PRInt32 aErrorNumber)
{
    InternalAbort(aErrorNumber);
    return NS_OK;
}

PRInt32    
nsInstall::AddDirectory(const nsString& aRegName, 
                        const nsString& aVersion, 
                        const nsString& aJarSource, 
                        nsInstallFolder *aFolder,
                        const nsString& aSubdir, 
                        PRInt32 aMode, 
                        PRInt32* aReturn)
{
    nsInstallFile* ie = nsnull;
    PRInt32 result;
    
    if ( aJarSource.IsEmpty() || aFolder == nsnull ) 
    {
        *aReturn = SaveError(nsInstall::INVALID_ARGUMENTS);
        return NS_OK;
    }

    result = SanityCheck();
    
    if (result != nsInstall::SUCCESS)
    {
        *aReturn = SaveError( result );
        return NS_OK;
    }
    
    nsString qualifiedRegName;
    
    if ( aRegName.IsEmpty()) 
    {
        // Default subName = location in jar file
        *aReturn = GetQualifiedRegName( aJarSource, qualifiedRegName);
    } 
    else 
    {
        *aReturn = GetQualifiedRegName( aRegName, qualifiedRegName );
    }

    if (*aReturn != SUCCESS)
    {
        return NS_OK;
    }
    
    nsString qualifiedVersion = aVersion;
    if (qualifiedVersion.IsEmpty())
    {
        // assume package version for overriden forms that don't take version info
        *aReturn = mVersionInfo->ToString(qualifiedVersion);

        if (NS_FAILED(*aReturn))
        {
            SaveError( nsInstall::UNEXPECTED_ERROR );
            return NS_OK;
        }
    }
	
    nsString subdirectory(aSubdir);

    if (!subdirectory.IsEmpty())
    {
        subdirectory.AppendWithConversion("/");
    }

    
    nsVoidArray *paths = new nsVoidArray();
    
    if (paths == nsnull)
    {
        *aReturn = SaveError(nsInstall::OUT_OF_MEMORY);
        return NS_OK;
    }

    result = ExtractDirEntries(aJarSource, paths);
    if (result != nsInstall::SUCCESS)
    {
        *aReturn = SaveError( result );
        return NS_OK;
    }
    
    PRInt32 count = paths->Count();
    
    if (count == 0)
    {
        *aReturn = SaveError( nsInstall::DOES_NOT_EXIST );
        return NS_OK;
    }

    for (PRInt32 i=0; i < count; i++)
    {
        nsString *thisPath = (nsString *)paths->ElementAt(i);

        nsString newJarSource = aJarSource;
        newJarSource.AppendWithConversion("/");
        newJarSource += *thisPath;
        
        nsString fullRegName = qualifiedRegName;
        fullRegName.AppendWithConversion("/");
        fullRegName += *thisPath;
        

        nsString newSubDir;

        if (!subdirectory.IsEmpty())
        {
            newSubDir = subdirectory;
        }
        
        newSubDir += *thisPath;

        ie = new nsInstallFile( this,
                                fullRegName,
                                qualifiedVersion,
                                newJarSource,
                                aFolder,
                                newSubDir,
                                aMode,
                                &result);
        
        if (ie == nsnull)
        {
            *aReturn = SaveError(nsInstall::OUT_OF_MEMORY);
                return NS_OK;
        }

        if (result == nsInstall::SUCCESS)
        {
            result = ScheduleForInstall( ie );
        }
        else
        {
            delete ie;
        }
    
    }
    
    DeleteVector(paths);

    *aReturn = SaveError( result );
    return NS_OK;
}

PRInt32    
nsInstall::AddDirectory(const nsString& aRegName, 
                        const nsString& aVersion, 
                        const nsString& aJarSource, 
                        nsInstallFolder *aFolder,
                        const nsString& aSubdir, 
                        PRInt32* aReturn)
{
    return AddDirectory(aRegName, 
                        aVersion, 
                        aJarSource, 
                        aFolder, 
                        aSubdir, 
                        INSTALL_NO_COMPARE,
                        aReturn);
}

PRInt32    
nsInstall::AddDirectory(const nsString& aRegName, 
                        const nsString& aJarSource, 
                        nsInstallFolder *aFolder,
                        const nsString& aSubdir, 
                        PRInt32* aReturn)
{
    return AddDirectory(aRegName, 
                        nsAutoString(), 
                        aJarSource, 
                        aFolder, 
                        aSubdir, 
                        INSTALL_NO_COMPARE,
                        aReturn);
}

PRInt32    
nsInstall::AddDirectory(const nsString& aJarSource,
                        PRInt32* aReturn)
{
    if(mPackageFolder == nsnull)
    {
        *aReturn = SaveError( nsInstall::PACKAGE_FOLDER_NOT_SET );
        return NS_OK;
    }
    
    return AddDirectory(nsAutoString(), 
                        nsAutoString(), 
                        aJarSource, 
                        mPackageFolder, 
                        nsAutoString(), 
                        INSTALL_NO_COMPARE,
                        aReturn);
}

PRInt32    
nsInstall::AddSubcomponent(const nsString& aRegName, 
                           const nsString& aVersion, 
                           const nsString& aJarSource, 
                           nsInstallFolder *aFolder, 
                           const nsString& aTargetName, 
                           PRInt32 aMode, 
                           PRInt32* aReturn)
{
    nsInstallFile*  ie;
    nsString        qualifiedRegName;
    nsString        qualifiedVersion = aVersion;
    nsString        tempTargetName   = aTargetName;
    
    PRInt32         errcode = nsInstall::SUCCESS;


    if(aJarSource.IsEmpty() || aFolder == nsnull ) 
    {
        *aReturn = SaveError( nsInstall::INVALID_ARGUMENTS );
        return NS_OK;
    }

    PRInt32 result = SanityCheck();

    if (result != nsInstall::SUCCESS)
    {
        *aReturn = SaveError( result );
        return NS_OK;
    }
    
    if( aTargetName.IsEmpty() )
    {
        PRInt32 pos = aJarSource.RFindChar('/');

        if ( pos == kNotFound )
            tempTargetName = aJarSource;
        else
            aJarSource.Mid(tempTargetName, pos+1, -1);
    }
    
    if (qualifiedVersion.IsEmpty())
        qualifiedVersion.AssignWithConversion("0.0.0.0");   	


    if ( aRegName.IsEmpty() ) 
    {
        // Default subName = location in jar file
        *aReturn = GetQualifiedRegName( aJarSource, qualifiedRegName);
    } 
    else 
    {
        *aReturn = GetQualifiedRegName( aRegName, qualifiedRegName );
    }

    if (*aReturn != SUCCESS)
    {
        return NS_OK;
    }
    
   
    ie = new nsInstallFile( this, 
                            qualifiedRegName, 
                            qualifiedVersion, 
                            aJarSource,
                            aFolder,
                            tempTargetName, 
                            aMode, 
                            &errcode );
    
    if (ie == nsnull)
    {
        *aReturn = SaveError(nsInstall::OUT_OF_MEMORY);
            return NS_OK;
    }

    if (errcode == nsInstall::SUCCESS) 
    {
        errcode = ScheduleForInstall( ie );
    }
    else
    {
        delete ie;
    }    

    *aReturn = SaveError( errcode );
    return NS_OK;
}

PRInt32    
nsInstall::AddSubcomponent(const nsString& aRegName, 
                           const nsString& aVersion, 
                           const nsString& aJarSource, 
                           nsInstallFolder* aFolder, 
                           const nsString& aTargetName, 
                           PRInt32* aReturn)
{
    return AddSubcomponent(aRegName, 
                           aVersion, 
                           aJarSource, 
                           aFolder, 
                           aTargetName, 
                           INSTALL_NO_COMPARE, 
                           aReturn);
}

PRInt32    
nsInstall::AddSubcomponent(const nsString& aRegName, 
                           const nsString& aJarSource, 
                           nsInstallFolder* aFolder,
                           const nsString& aTargetName, 
                           PRInt32* aReturn)
{
    PRInt32 result = SanityCheck();

    if (result != nsInstall::SUCCESS)
    {
        *aReturn = SaveError( result );
        return NS_OK;
    }

    nsString version;
    *aReturn = mVersionInfo->ToString(version);

    if (NS_FAILED(*aReturn))
    {
        SaveError( nsInstall::UNEXPECTED_ERROR );
        return NS_OK;
    }
    
    return AddSubcomponent(aRegName, 
                           version, 
                           aJarSource, 
                           aFolder, 
                           aTargetName, 
                           INSTALL_NO_COMPARE, 
                           aReturn);
}

PRInt32    
nsInstall::AddSubcomponent(const nsString& aJarSource,
                           PRInt32* aReturn)
{
    if(mPackageFolder == nsnull)
    {
        *aReturn = SaveError( nsInstall::PACKAGE_FOLDER_NOT_SET );
        return NS_OK;
    }
    PRInt32 result = SanityCheck();

    if (result != nsInstall::SUCCESS)
    {
        *aReturn = SaveError( result );
        return NS_OK;
    }
    
    nsString version;
    *aReturn = mVersionInfo->ToString(version);

    if (NS_FAILED(*aReturn))
    {
        SaveError( nsInstall::UNEXPECTED_ERROR );
        return NS_OK;
    }

    return AddSubcomponent(nsAutoString(), 
                           version, 
                           aJarSource, 
                           mPackageFolder, 
                           nsAutoString(),
                           INSTALL_NO_COMPARE, 
                           aReturn);
}

PRInt32    
nsInstall::DeleteComponent(const nsString& aRegistryName, PRInt32* aReturn)
{
    PRInt32 result = SanityCheck();

    if (result != nsInstall::SUCCESS)
    {
        *aReturn = SaveError( result );
        return NS_OK;
    }

    nsString qualifiedRegName;

    *aReturn = GetQualifiedRegName( aRegistryName, qualifiedRegName);
    
    if (*aReturn != SUCCESS)
    {
        return NS_OK;
    }
    
    nsInstallDelete* id = new nsInstallDelete(this, qualifiedRegName, &result);
    
    if (id == nsnull)
    {
        *aReturn = SaveError(nsInstall::OUT_OF_MEMORY);
        return NS_OK;
    }

    if (result == nsInstall::SUCCESS) 
    {
        result = ScheduleForInstall( id );
    }
    
    *aReturn = SaveError(result);

    return NS_OK;
}

PRInt32    
nsInstall::DeleteFile(nsInstallFolder* aFolder, const nsString& aRelativeFileName, PRInt32* aReturn)
{
    PRInt32 result = SanityCheck();

    if (result != nsInstall::SUCCESS)
    {
        *aReturn = SaveError( result );
        return NS_OK;
    }
   
    nsInstallDelete* id = new nsInstallDelete(this, aFolder, aRelativeFileName, &result);

    if (id == nsnull)
    {
        *aReturn = SaveError(nsInstall::OUT_OF_MEMORY);
        return NS_OK;
    }

    if (result == nsInstall::SUCCESS) 
    {
        result = ScheduleForInstall( id );
    }
        
    if (result == nsInstall::DOES_NOT_EXIST) 
    {
        result = nsInstall::SUCCESS;
    }

    *aReturn = SaveError(result);

    return NS_OK;
}

PRInt32    
nsInstall::DiskSpaceAvailable(const nsString& aFolder, PRInt64* aReturn)
{
    PRInt32 result = SanityCheck();

    if (result != nsInstall::SUCCESS)
    {
        double d = SaveError( result );
        LL_L2D(d, *aReturn);
        return NS_OK;
    }
    nsAutoCString temp(aFolder);
    nsCOMPtr<nsILocalFile> folder;
    NS_NewLocalFile(temp, PR_TRUE, getter_AddRefs(folder));

    result = folder->GetDiskSpaceAvailable(aReturn);
    return NS_OK;
}

PRInt32    
nsInstall::Execute(const nsString& aJarSource, const nsString& aArgs, PRInt32* aReturn)
{
    PRInt32 result = SanityCheck();

    if (result != nsInstall::SUCCESS)
    {
        *aReturn = SaveError( result );
        return NS_OK;
    }
   
    nsInstallExecute* ie = new nsInstallExecute(this, aJarSource, aArgs, &result);
    
    if (ie == nsnull)
    {
        *aReturn = SaveError(nsInstall::OUT_OF_MEMORY);
        return NS_OK;
    }

    if (result == nsInstall::SUCCESS) 
    {
        result = ScheduleForInstall( ie );
    }
        
    *aReturn = SaveError(result);
    return NS_OK;
}

PRInt32    
nsInstall::Execute(const nsString& aJarSource, PRInt32* aReturn)
{
    return Execute(aJarSource, nsAutoString(), aReturn);
}

PRInt32    
nsInstall::FinalizeInstall(PRInt32* aReturn)
{
    PRInt32 result = SUCCESS;
    PRBool  rebootNeeded = PR_FALSE;

    *aReturn = SanityCheck();

    if (*aReturn != nsInstall::SUCCESS)
    {
        SaveError( *aReturn );
        if (mListener)
        {
            mListener->FinalStatus(mInstallURL.GetUnicode(), *aReturn);
            mStatusSent = PR_TRUE;
        }
        return NS_OK;
    }
    

    if ( mInstalledFiles->Count() > 0 )
    {
        if ( mUninstallPackage )
        {
            VR_UninstallCreateNode( (char*)(const char*) nsAutoCString(mRegistryPackageName), 
                                    (char*)(const char*) nsAutoCString(mUIName));
        }

        // Install the Component into the Version Registry.
        if (mVersionInfo)
        {
            nsString versionString;
            nsCString path;

            mVersionInfo->ToString(versionString);

            if (mPackageFolder)
                mPackageFolder->GetDirectoryPath(path);

            VR_Install( (char*)(const char*)nsAutoCString(mRegistryPackageName), 
                        (char*)path.GetBuffer(),
                        (char*)(const char*)nsAutoCString(versionString), 
                        PR_FALSE );
        }

        nsInstallObject* ie = nsnull;

        for (PRInt32 i=0; i < mInstalledFiles->Count(); i++) 
        {
            ie = (nsInstallObject*)mInstalledFiles->ElementAt(i);
            NS_ASSERTION(ie, "NULL object in install queue!");
            if (ie == NULL)
                continue;

            if (mListener)
            {
                char *objString = ie->toString();
                if (objString)
                {
                    mListener->FinalizeProgress(NS_ConvertASCIItoUCS2(objString).GetUnicode(),
                                               (i+1), mInstalledFiles->Count());
                    delete [] objString;
                }
            }

            result = ie->Complete();

            if (result != nsInstall::SUCCESS) 
            {
                if ( result == REBOOT_NEEDED )
                {
                    rebootNeeded = PR_TRUE;
                    result = SUCCESS;
                }
                else
                {
                    InternalAbort( result );
                    break;
                }
            }
        }

        if ( result == SUCCESS )
        {
            if ( rebootNeeded )
                *aReturn = SaveError( REBOOT_NEEDED );

            // XXX for now all successful installs will trigger an Autoreg.
            // We eventually want to do this only when flagged.
            HREG reg;
            if ( REGERR_OK == NR_RegOpen("", &reg) )
            {
                RKEY xpiRoot;
                REGERR err;
                err = NR_RegAddKey(reg,ROOTKEY_COMMON,XPI_ROOT_KEY,&xpiRoot);
                if ( err == REGERR_OK )
                    NR_RegSetEntryString(reg, xpiRoot, XPI_AUTOREG_VAL, "yes");

                NR_RegClose(reg);
            }
        }
        else
            *aReturn = SaveError( result );

        if (mListener)
        {
            mListener->FinalStatus(mInstallURL.GetUnicode(), *aReturn);
            mStatusSent = PR_TRUE;
        }
    }
    else
    {
        // no actions queued: don't register the package version
        // and no need for user confirmation

        if (mListener)
        {
            mListener->FinalStatus(mInstallURL.GetUnicode(), *aReturn);
            mStatusSent = PR_TRUE;
        }
    }

    CleanUp();

    return NS_OK;
}

#ifdef XP_MAC
#define GESTALT_CHAR_CODE(x)          (((unsigned long) ((x[0]) & 0x000000FF)) << 24) \
                                    | (((unsigned long) ((x[1]) & 0x000000FF)) << 16) \
                                    | (((unsigned long) ((x[2]) & 0x000000FF)) << 8)  \
                                    | (((unsigned long) ((x[3]) & 0x000000FF)))
#endif /* XP_MAC */
								
PRInt32    
nsInstall::Gestalt(const nsString& aSelector, PRInt32* aReturn)
{
    *aReturn = nsnull;
    
    PRInt32 result = SanityCheck();

    if (result != nsInstall::SUCCESS)
    {
        *aReturn = SaveError( result );
        return NS_OK;
    }
#ifdef XP_MAC
	
    long    response = 0;
    char    selectorChars[4];
    int     i;
    OSErr   err = noErr;
    OSType  selector;
    
    if (aSelector.IsEmpty())
    {
        return NS_OK;
    }

    for (i=0; i<4; i++)
        selectorChars[i] = aSelector.CharAt(i);
    selector = GESTALT_CHAR_CODE(selectorChars);
    
    err = ::Gestalt(selector, &response);
    
    if (err != noErr)
        *aReturn = err;
    else
        *aReturn = response;
	
#endif    
    return NS_OK;    
}

PRInt32
nsInstall::GetComponentFolder(const nsString& aComponentName, const nsString& aSubdirectory, nsInstallFolder** aNewFolder)
{
    long        err;
    char*       componentCString;
    char        dir[MAXREGPATHLEN];
    nsFileSpec  nsfsDir;
    nsresult    res = NS_OK;

    if(!aNewFolder)
      return INVALID_ARGUMENTS;

    *aNewFolder = nsnull;
    
    
    nsString tempString;

    if ( GetQualifiedPackageName(aComponentName, tempString) != SUCCESS )
    {
        return NS_OK;
    }

    componentCString = tempString.ToNewCString();

    if((err = VR_GetDefaultDirectory( componentCString, MAXREGPATHLEN, dir )) != REGERR_OK)
    {
        if((err = VR_GetPath( componentCString, MAXREGPATHLEN, dir )) == REGERR_OK)
        {
            int i;

            nsString dirStr; dirStr.AssignWithConversion(dir);
            if (  (i = dirStr.RFindChar(FILESEP)) > 0 ) 
            {
                // i is the index in the string, not the total number of
                // characters in the string.  ToCString() requires the
                // total number of characters in the string to copy,
                // therefore add 1 to it.
                dirStr.Truncate(i + 1);
                dirStr.ToCString(dir, MAXREGPATHLEN);
            }
        }
        else
        {
            *dir = '\0';
        }
    }
    else
    {
        *dir = '\0';
    }

    if(*dir != '\0') 
    {
      nsInstallFolder * folder = new nsInstallFolder();
      if (!folder) return NS_ERROR_OUT_OF_MEMORY;
      res = folder->Init(NS_ConvertASCIItoUCS2(dir), aSubdirectory);
      if (NS_FAILED(res))
      {
        delete folder;
      }
      else
      {
        *aNewFolder = folder;
      }
    }

    if (componentCString)
        Recycle(componentCString);
    
    return res;
}

PRInt32    
nsInstall::GetComponentFolder(const nsString& aComponentName, nsInstallFolder** aNewFolder)
{
    return GetComponentFolder(aComponentName, nsAutoString(), aNewFolder);
}

PRInt32
nsInstall::GetFolder(const nsString& targetFolder, const nsString& aSubdirectory, nsInstallFolder** aNewFolder)
{
    /* This version of GetFolder takes an nsString object as the first param */
    if (!aNewFolder)
        return INVALID_ARGUMENTS;

    * aNewFolder = nsnull;
    
    nsInstallFolder* folder = new nsInstallFolder();   
    if (folder == nsnull)
    {
        return NS_ERROR_OUT_OF_MEMORY;
    }
    nsresult res = folder->Init(targetFolder, aSubdirectory);

    if (NS_FAILED(res))
    {
        delete folder;
        return res;
    }
    *aNewFolder = folder;
    return NS_OK;
}

PRInt32    
nsInstall::GetFolder(const nsString& targetFolder, nsInstallFolder** aNewFolder)
{
    /* This version of GetFolder takes an nsString object as the only param */
    return GetFolder(targetFolder, nsAutoString(), aNewFolder);
}

PRInt32
nsInstall::GetFolder( nsInstallFolder& aTargetFolderObj, const nsString& aSubdirectory, nsInstallFolder** aNewFolder )
{
    /* This version of GetFolder takes an nsString object as the first param */
    if (!aNewFolder)
        return INVALID_ARGUMENTS;

    * aNewFolder = nsnull;

    nsInstallFolder* folder = new nsInstallFolder();   
    if (folder == nsnull)
    {
        return NS_ERROR_OUT_OF_MEMORY;
    }
    nsresult res = folder->Init(aTargetFolderObj, aSubdirectory);

    if (NS_FAILED(res))
    {
        delete folder;
        return res;
    }
    *aNewFolder = folder;
    return NS_OK;
}




PRInt32    
nsInstall::GetLastError(PRInt32* aReturn)
{
    *aReturn = mLastError;
    return NS_OK;
}

PRInt32    
nsInstall::GetWinProfile(const nsString& aFolder, const nsString& aFile, JSContext* jscontext, JSClass* WinProfileClass, jsval* aReturn)
{
    *aReturn = JSVAL_NULL;
    
    PRInt32 result = SanityCheck();

    if (result != nsInstall::SUCCESS)
    {
        *aReturn = SaveError( result );
        return NS_OK;
    }

#ifdef _WINDOWS
    JSObject*     winProfileObject;
    nsWinProfile* nativeWinProfileObject = new nsWinProfile(this, aFolder, aFile);
    
    if (nativeWinProfileObject == nsnull)
        return NS_OK;
    
    JSObject*     winProfilePrototype    = this->RetrieveWinProfilePrototype();
    
    winProfileObject = JS_NewObject(jscontext, WinProfileClass, winProfilePrototype, NULL);
    if(winProfileObject == NULL)
        return NS_OK;
    
    JS_SetPrivate(jscontext, winProfileObject, nativeWinProfileObject);

    *aReturn = OBJECT_TO_JSVAL(winProfileObject);
#endif /* _WINDOWS */

    return NS_OK;
}

PRInt32    
nsInstall::GetWinRegistry(JSContext* jscontext, JSClass* WinRegClass, jsval* aReturn)
{
    *aReturn = JSVAL_NULL;
    
    PRInt32 result = SanityCheck();

    if (result != nsInstall::SUCCESS)
    {
        *aReturn = SaveError( result );
        return NS_OK;
    }

#ifdef _WINDOWS
    JSObject* winRegObject;
    nsWinReg* nativeWinRegObject = new nsWinReg(this);
    
    if (nativeWinRegObject == nsnull)
        return NS_OK;

    JSObject* winRegPrototype    = this->RetrieveWinRegPrototype();

    winRegObject = JS_NewObject(jscontext, WinRegClass, winRegPrototype, NULL);
    if(winRegObject == NULL)
        return NS_OK;
 
    JS_SetPrivate(jscontext, winRegObject, nativeWinRegObject);

    *aReturn = OBJECT_TO_JSVAL(winRegObject);
#endif /* _WINDOWS */

    return NS_OK;
}

PRInt32
nsInstall::LoadResources(JSContext* cx, const nsString& aBaseName, jsval* aReturn)
{
    PRInt32 result = SanityCheck();

    if (result != nsInstall::SUCCESS)
    {
        *aReturn = SaveError( result );
        return NS_OK;
    }
    nsresult ret;
    nsCOMPtr<nsIFile> resFile;
    nsCOMPtr<nsIFileURL> resFileURL = nsnull;
    nsIURI *url = nsnull;
    nsILocale* locale = nsnull;
    nsIStringBundleService* service = nsnull;
    nsIEventQueueService* pEventQueueService = nsnull;
    nsIStringBundle* bundle = nsnull;
    nsIBidirectionalEnumerator* propEnum = nsnull;
    *aReturn = JSVAL_NULL;
    jsval v = JSVAL_NULL;

    // set up JSObject to return
    JS_GetProperty( cx, JS_GetGlobalObject( cx ), "Object", &v );
    if (!v)
    {
        return NS_ERROR_NULL_POINTER;
    }
    JSClass *objclass = JS_GetClass( cx, JSVAL_TO_OBJECT(v) );
    JSObject *res = JS_NewObject( cx, objclass, JSVAL_TO_OBJECT(v), 0 );

    // extract properties file
    // XXX append locale info: lang code, country code, .properties suffix to aBaseName
    PRInt32 err = ExtractFileFromJar(aBaseName, nsnull, getter_AddRefs(resFile));
    if ( (!resFile) || (err != nsInstall::SUCCESS)  )
    {
        SaveError( err );
        return NS_OK;
    }
	
    // initialize string bundle and related services
    ret = nsServiceManager::GetService(kStringBundleServiceCID, 
                    kIStringBundleServiceIID, (nsISupports**) &service);
    if (NS_FAILED(ret)) 
        goto cleanup;
    ret = nsServiceManager::GetService(kEventQueueServiceCID,
                    kIEventQueueServiceIID, (nsISupports**) &pEventQueueService);
    if (NS_FAILED(ret)) 
        goto cleanup;
    ret = pEventQueueService->CreateThreadEventQueue();
    if (NS_FAILED(ret)) 
        goto cleanup;

    // construct properties file URL as required by StringBundle interface
    
    //nsCOMPtr<nsIFileURL> url;
    ret = nsComponentManager::CreateInstance(kStandardURLCID, nsnull,
                                             NS_GET_IID(nsIFileURL),
                                             getter_AddRefs(resFileURL));

    if (NS_FAILED(ret)) goto cleanup;

    ret = resFileURL->SetFile(resFile);

    if (NS_FAILED(ret)) goto cleanup;


    // get the string bundle using the extracted properties file
#if 1
    {
      char* spec = nsnull;
      ret = resFileURL->GetSpec(&spec);
      if (NS_FAILED(ret)) {
        printf("cannot get url spec\n");
        nsServiceManager::ReleaseService(kStringBundleServiceCID, service);
        nsCRT::free(spec);
        return ret;
      }
      ret = service->CreateBundle(spec, locale, &bundle);
      nsCRT::free(spec);
    }
#else
    ret = service->CreateBundle(url, locale, &bundle);
#endif
    if (NS_FAILED(ret)) 
        goto cleanup;
    ret = bundle->GetEnumeration(&propEnum);
    if (NS_FAILED(ret))
        goto cleanup;

    // set the variables of the JSObject to return using the StringBundle's
    // enumeration service
    ret = propEnum->First();
    if (NS_FAILED(ret))
        goto cleanup;
    while (NS_SUCCEEDED(ret))
    {
        nsIPropertyElement* propElem = nsnull;
        ret = propEnum->CurrentItem((nsISupports**)&propElem);
        if (NS_FAILED(ret))
            goto cleanup;

        PRUnichar *pKey = nsnull;
        PRUnichar *pVal = nsnull;

        ret = propElem->GetKey(&pKey);
        if (NS_FAILED(ret)) 
            goto cleanup;
        ret = propElem->GetValue(&pVal);
        if (NS_FAILED(ret))
            goto cleanup;

        nsAutoString keyAdjustedLengthBuff(pKey);
        nsAutoString valAdjustedLengthBuff(pVal);

        char* keyCStr = keyAdjustedLengthBuff.ToNewCString();
        PRUnichar* valCStr = valAdjustedLengthBuff.ToNewUnicode();
        if (keyCStr && valCStr) 
        {
            JSString* propValJSStr = JS_NewUCStringCopyZ(cx, (jschar*) valCStr);
            jsval propValJSVal = STRING_TO_JSVAL(propValJSStr);
            JS_SetProperty(cx, res, keyCStr, &propValJSVal);
            delete[] keyCStr;
            delete[] valCStr;
        }
        if (pKey)
            delete[] pKey;
        if (pVal)
            delete[] pVal;
        ret = propEnum->Next();
    }
	 
    *aReturn = OBJECT_TO_JSVAL(res);
    ret = nsInstall::SUCCESS;

cleanup:
    SaveError( ret );
	
    // release services
    NS_IF_RELEASE( service );
    NS_IF_RELEASE( pEventQueueService );

    // release file, URL, StringBundle, Enumerator
    NS_IF_RELEASE( url );
    NS_IF_RELEASE( bundle );
    NS_IF_RELEASE( propEnum );

    return NS_OK;
}

PRInt32    
nsInstall::Patch(const nsString& aRegName, const nsString& aVersion, const nsString& aJarSource, nsInstallFolder* aFolder, const nsString& aTargetName, PRInt32* aReturn)
{
    PRInt32 result = SanityCheck();

    if (result != nsInstall::SUCCESS)
    {
        *aReturn = SaveError( result );
        return NS_OK;
    }

    nsString qualifiedRegName;
    
    *aReturn = GetQualifiedRegName( aRegName, qualifiedRegName);
    
    if (*aReturn != SUCCESS)
    {
        return NS_OK;
    }

    if (!mPatchList)
    {
        mPatchList = new nsHashtable();
        if (mPatchList == nsnull)
        {
            *aReturn = SaveError(nsInstall::OUT_OF_MEMORY);
            return NS_OK;
        }
    }
 
    nsInstallPatch* ip = new nsInstallPatch( this,
                                             qualifiedRegName,
                                             aVersion,
                                             aJarSource,
                                             aFolder,
                                             aTargetName,
                                             &result);
    
    if (ip == nsnull)
    {
        *aReturn = SaveError(nsInstall::OUT_OF_MEMORY);
        return NS_OK;
    }

    if (result == nsInstall::SUCCESS) 
    {
        result = ScheduleForInstall( ip );
    }
        
    *aReturn = SaveError(result);
    return NS_OK;
}

PRInt32    
nsInstall::Patch(const nsString& aRegName, const nsString& aJarSource, nsInstallFolder* aFolder, const nsString& aTargetName, PRInt32* aReturn)
{
    return Patch(aRegName, nsAutoString(), aJarSource, aFolder, aTargetName, aReturn);
}

PRInt32
nsInstall::RegisterChrome(nsIFile* chrome, PRUint32 chromeType, const char* path)
{
    PRInt32 result = SanityCheck();
    if (result != SUCCESS)
        return SaveError( result );

    if (!chrome || !chromeType)
        return SaveError( INVALID_ARGUMENTS );

    nsRegisterItem* ri = new nsRegisterItem(this, chrome, chromeType, path);
    if (ri == nsnull)
        return SaveError(OUT_OF_MEMORY);
    else
        return SaveError(ScheduleForInstall( ri ));
}


PRInt32    
nsInstall::ResetError()
{
    mLastError = nsInstall::SUCCESS;
    return NS_OK;
}

PRInt32    
nsInstall::SetPackageFolder(nsInstallFolder& aFolder)
{
    if (mPackageFolder)
        delete mPackageFolder;

    nsInstallFolder* folder = new nsInstallFolder();   
    if (folder == nsnull)
    {
        return NS_ERROR_OUT_OF_MEMORY;
    }
    nsresult res = folder->Init(aFolder, nsAutoString());

    if (NS_FAILED(res))
    {
        delete folder;
        return res;
    }
    mPackageFolder = folder;
    return NS_OK;
}


PRInt32    
nsInstall::StartInstall(const nsString& aUserPackageName, const nsString& aRegistryPackageName, const nsString& aVersion, PRInt32* aReturn)
{
    if ( aUserPackageName.Length() == 0 )
    {
        // There must be some pretty name for the UI and the uninstall list
        *aReturn = SaveError(INVALID_ARGUMENTS);
        return NS_OK;
    }

    char szRegPackagePath[MAXREGPATHLEN];
    char* szRegPackageName = aRegistryPackageName.ToNewCString();
    
    if (szRegPackageName == nsnull)
    {
        *aReturn = SaveError(nsInstall::OUT_OF_MEMORY);
        return nsInstall::OUT_OF_MEMORY;
    }

    *szRegPackagePath = '0';
    *aReturn   = nsInstall::SUCCESS;
    
    ResetError();
        
    mUserCancelled = PR_FALSE; 
     
    mUIName = aUserPackageName;
    
    *aReturn = GetQualifiedPackageName( aRegistryPackageName, mRegistryPackageName );
    
    if (*aReturn != nsInstall::SUCCESS)
    {
        return NS_OK;
    }

    if(REGERR_OK == VR_GetDefaultDirectory(szRegPackageName, MAXREGPATHLEN, szRegPackagePath))
    {
        nsInstallFolder* folder = new nsInstallFolder();   
        if (folder == nsnull)
        {
            return NS_ERROR_OUT_OF_MEMORY;
        }
        nsresult res = folder->Init(NS_ConvertASCIItoUCS2(szRegPackagePath), nsAutoString());

        if (NS_FAILED(res))
        {
            delete folder;
        }
        else
        {
            mPackageFolder = folder;
        }
    }
    else
    {
      mPackageFolder = nsnull;
    }

    if(szRegPackageName)
      Recycle(szRegPackageName);

    if (mVersionInfo != nsnull)
        delete mVersionInfo;

    mVersionInfo    = new nsInstallVersion();
    if (mVersionInfo == nsnull)
    {
        *aReturn = nsInstall::OUT_OF_MEMORY;
        return SaveError(nsInstall::OUT_OF_MEMORY);
    }

    mVersionInfo->Init(aVersion);

    mInstalledFiles = new nsVoidArray();
    
    if (mInstalledFiles == nsnull)
    {
        *aReturn = nsInstall::OUT_OF_MEMORY;
        return SaveError(nsInstall::OUT_OF_MEMORY);
    }

    if (mListener)
            mListener->InstallStarted(mInstallURL.GetUnicode(), mUIName.GetUnicode());

    mStartInstallCompleted = PR_TRUE;
    return NS_OK;
}


PRInt32    
nsInstall::Uninstall(const nsString& aRegistryPackageName, PRInt32* aReturn)
{
    PRInt32 result = SanityCheck();

    if (result != nsInstall::SUCCESS)
    {
        *aReturn = SaveError( result );
        return NS_OK;
    }
    
    nsString qualifiedPackageName;

    *aReturn = GetQualifiedPackageName( aRegistryPackageName, qualifiedPackageName );
    
    if (*aReturn != SUCCESS)
    {
        return NS_OK;
    }

    nsInstallUninstall *ie = new nsInstallUninstall( this,
                                                     qualifiedPackageName,
                                                     &result );

    if (ie == nsnull)
    {
        *aReturn = SaveError(nsInstall::OUT_OF_MEMORY);
        return NS_OK;
    }
    
    if (result == nsInstall::SUCCESS) 
    {
        result = ScheduleForInstall( ie );
    }
    else
    {
        delete ie;
    }    

    *aReturn = SaveError(result);

    return NS_OK;
}

////////////////////////////////////////


void       
nsInstall::AddPatch(nsHashKey *aKey, nsIFile* fileName)
{
    if (mPatchList != nsnull)
    {
        mPatchList->Put(aKey, fileName);
    }
}

void       
nsInstall::GetPatch(nsHashKey *aKey, nsIFile** fileName)
{
    if (!fileName) 
        return;
    else
        *fileName = nsnull;

    if (mPatchList != nsnull)
    {
        *fileName = (nsIFile*) mPatchList->Get(aKey);
    }
}

PRInt32
nsInstall::FileOpDirCreate(nsInstallFolder& aTarget, PRInt32* aReturn)
{
  nsCOMPtr<nsIFile> localFile = aTarget.GetFileSpec();
  if (localFile == nsnull)
  {
     *aReturn = SaveError(nsInstall::OUT_OF_MEMORY);
     return NS_OK;
  }

  nsInstallFileOpItem* ifop = new nsInstallFileOpItem(this, NS_FOP_DIR_CREATE, localFile, aReturn);
  if (ifop == nsnull)
  {
      *aReturn = SaveError(nsInstall::OUT_OF_MEMORY);
      return NS_OK;
  }

  PRInt32 result = SanityCheck();
  if (result != nsInstall::SUCCESS)
  {
      delete ifop;
      *aReturn = SaveError( result );
      return NS_OK;
  }
  
  if (*aReturn == nsInstall::SUCCESS) 
  {
      *aReturn = ScheduleForInstall( ifop );
  }
  
  SaveError(*aReturn);

  return NS_OK;
}

PRInt32
nsInstall::FileOpDirGetParent(nsInstallFolder& aTarget, nsInstallFolder** theParentFolder)
{
  nsCOMPtr<nsIFile> parent;
  nsCOMPtr<nsIFile> localFile = aTarget.GetFileSpec();

  nsresult rv = localFile->GetParent(getter_AddRefs(parent));
  if (NS_SUCCEEDED(rv) && parent)
  {
    nsInstallFolder* folder = new nsInstallFolder();   
    if (folder == nsnull)
    {
        return NS_ERROR_OUT_OF_MEMORY;
    }
	  folder->Init(parent);
	  *theParentFolder = folder;
  }
  else
	  theParentFolder = nsnull;

  return NS_OK;
}

PRInt32
nsInstall::FileOpDirRemove(nsInstallFolder& aTarget, PRInt32 aFlags, PRInt32* aReturn)
{
  nsCOMPtr<nsIFile> localFile = aTarget.GetFileSpec();
  if (localFile == nsnull)
  {
     *aReturn = SaveError(nsInstall::OUT_OF_MEMORY);
     return NS_OK;
  }

  nsInstallFileOpItem* ifop = new nsInstallFileOpItem(this, NS_FOP_DIR_REMOVE, localFile, aFlags, aReturn);
  if (ifop == nsnull)
  {
      *aReturn = SaveError(nsInstall::OUT_OF_MEMORY);
      return NS_OK;
  }

  PRInt32 result = SanityCheck();
  if (result != nsInstall::SUCCESS)
  {
      delete ifop;
      *aReturn = SaveError( result );
      return NS_OK;
  }

  if (*aReturn == nsInstall::SUCCESS) 
  {
      *aReturn = ScheduleForInstall( ifop );
  }
  
  SaveError(*aReturn);

  return NS_OK;
}

PRInt32
nsInstall::FileOpDirRename(nsInstallFolder& aSrc, nsString& aTarget, PRInt32* aReturn)
{
  nsCOMPtr<nsIFile> localFile = aSrc.GetFileSpec();
  if (localFile == nsnull)
  {
     *aReturn = SaveError(nsInstall::OUT_OF_MEMORY);
     return NS_OK;
  }

  nsInstallFileOpItem* ifop = new nsInstallFileOpItem(this, NS_FOP_DIR_RENAME, localFile, aTarget, aReturn);
  if (ifop == nsnull)
  {
      *aReturn = SaveError(nsInstall::OUT_OF_MEMORY);
      return NS_OK;
  }

  PRInt32 result = SanityCheck();
  if (result != nsInstall::SUCCESS)
  {
      delete ifop;
      *aReturn = SaveError( result );
      return NS_OK;
  }

  if (*aReturn == nsInstall::SUCCESS) 
  {
      *aReturn = ScheduleForInstall( ifop );
  }
  
  SaveError(*aReturn);

  return NS_OK;
}

PRInt32
nsInstall::FileOpFileCopy(nsInstallFolder& aSrc, nsInstallFolder& aTarget, PRInt32* aReturn)
{
  nsCOMPtr<nsIFile> localSrcFile = aSrc.GetFileSpec();
  if (localSrcFile == nsnull)
  {
     *aReturn = SaveError(nsInstall::OUT_OF_MEMORY);
     return NS_OK;
  }

  nsCOMPtr<nsIFile>localTargetFile = aTarget.GetFileSpec();
  if (localTargetFile == nsnull)
  {
     *aReturn = SaveError(nsInstall::OUT_OF_MEMORY);
     return NS_OK;
  }

  nsInstallFileOpItem* ifop = new nsInstallFileOpItem(this, NS_FOP_FILE_COPY, localSrcFile, localTargetFile, aReturn);
  if (ifop == nsnull)
  {
      *aReturn = SaveError(nsInstall::OUT_OF_MEMORY);
      return NS_OK;
  }

  PRInt32 result = SanityCheck();
  if (result != nsInstall::SUCCESS)
  {
      delete ifop;
      *aReturn = SaveError( result );
      return NS_OK;
  }

  if (*aReturn == nsInstall::SUCCESS) 
  {
      *aReturn = ScheduleForInstall( ifop );
  }

  SaveError(*aReturn);

  return NS_OK;
}

PRInt32
nsInstall::FileOpFileDelete(nsInstallFolder& aTarget, PRInt32 aFlags, PRInt32* aReturn)
{
  nsCOMPtr<nsIFile> localFile = aTarget.GetFileSpec();
  if (localFile == nsnull)
  {
     *aReturn = SaveError(nsInstall::OUT_OF_MEMORY);
     return NS_OK;
  }

  nsInstallFileOpItem* ifop = new nsInstallFileOpItem(this, NS_FOP_FILE_DELETE, localFile, aFlags, aReturn);
  if (ifop == nsnull)
  {
      *aReturn = SaveError(nsInstall::OUT_OF_MEMORY);
      return NS_OK;
  }

  PRInt32 result = SanityCheck();
  if (result != nsInstall::SUCCESS)
  {
      delete ifop;
      *aReturn = SaveError( result );
      return NS_OK;
  }

  if (*aReturn == nsInstall::SUCCESS) 
  {
      *aReturn = ScheduleForInstall( ifop );
  }
  
  SaveError(*aReturn);

  return NS_OK;
}

PRInt32
nsInstall::FileOpFileExecute(nsInstallFolder& aTarget, nsString& aParams, PRInt32* aReturn)
{
  nsCOMPtr<nsIFile> localFile = aTarget.GetFileSpec();
  if (localFile == nsnull)
  {
     *aReturn = SaveError(nsInstall::OUT_OF_MEMORY);
     return NS_OK;
  }

  nsInstallFileOpItem* ifop = new nsInstallFileOpItem(this, NS_FOP_FILE_EXECUTE, localFile, aParams, aReturn);
  if (ifop == nsnull)
  {
      *aReturn = SaveError(nsInstall::OUT_OF_MEMORY);
      return NS_OK;
  }

  PRInt32 result = SanityCheck();
  if (result != nsInstall::SUCCESS)
  {
      delete ifop;
      *aReturn = SaveError( result );
      return NS_OK;
  }

  if (*aReturn == nsInstall::SUCCESS) 
  {
      *aReturn = ScheduleForInstall( ifop );
  }
  
  SaveError(*aReturn);

  return NS_OK;
}

PRInt32
nsInstall::FileOpFileExists(nsInstallFolder& aTarget, PRBool* aReturn)
{
  nsCOMPtr<nsIFile> localFile = aTarget.GetFileSpec();

  localFile->Exists(aReturn);
  return NS_OK;
}

PRInt32
nsInstall::FileOpFileGetNativeVersion(nsInstallFolder& aTarget, nsString* aReturn)
{
  return NS_OK;
}

PRInt32
nsInstall::FileOpFileGetDiskSpaceAvailable(nsInstallFolder& aTarget, PRInt64* aReturn)
{
  nsresult rv;
  nsCOMPtr<nsIFile> file = aTarget.GetFileSpec();
  nsCOMPtr<nsILocalFile> localFile = do_QueryInterface(file, &rv); 

  localFile->GetDiskSpaceAvailable(aReturn);  //nsIFileXXX: need to figure out how to call GetDiskSpaceAvailable
  return NS_OK;
}

//nsIFileXXX: need to get nsIFile equivalent to GetModDate
PRInt32
nsInstall::FileOpFileGetModDate(nsInstallFolder& aTarget, double* aReturn)
{
    * aReturn = 0;

    nsCOMPtr<nsIFile> localFile = aTarget.GetFileSpec();
  
    if (localFile)
    {
        double newStamp;
        PRInt64 lastModDate = LL_ZERO;
        localFile->GetLastModificationDate(&lastModDate);
    
        LL_L2D(newStamp, lastModDate);

        *aReturn = newStamp;
    }

  return NS_OK;
}

PRInt32
nsInstall::FileOpFileGetSize(nsInstallFolder& aTarget, PRInt64* aReturn)
{
  nsCOMPtr<nsIFile> localFile = aTarget.GetFileSpec();

  localFile->GetFileSize(aReturn);
  return NS_OK;
}

PRInt32
nsInstall::FileOpFileIsDirectory(nsInstallFolder& aTarget, PRBool* aReturn)
{
  nsCOMPtr<nsIFile> localFile = aTarget.GetFileSpec();

  localFile->IsDirectory(aReturn);
  return NS_OK;
}

PRInt32
nsInstall::FileOpFileIsFile(nsInstallFolder& aTarget, PRBool* aReturn)
{
  nsCOMPtr<nsIFile> localFile = aTarget.GetFileSpec();

  localFile->IsFile(aReturn);
  return NS_OK;
}

//nsIFileXXX: need to get the ModDateChanged equivalent for nsIFile
PRInt32
nsInstall::FileOpFileModDateChanged(nsInstallFolder& aTarget, double aOldStamp, PRBool* aReturn)
{
    *aReturn = PR_TRUE;

    nsCOMPtr<nsIFile> localFile = aTarget.GetFileSpec();
    if (localFile)
    {
        double newStamp;
        PRInt64 lastModDate = LL_ZERO;
        localFile->GetLastModificationDate(&lastModDate);
        
        LL_L2D(newStamp, lastModDate);

        *aReturn = !(newStamp == aOldStamp);
    }
    return NS_OK;
}

PRInt32
nsInstall::FileOpFileMove(nsInstallFolder& aSrc, nsInstallFolder& aTarget, PRInt32* aReturn)
{
  nsCOMPtr<nsIFile> localSrcFile = aSrc.GetFileSpec();
  if (localSrcFile == nsnull)
  {
     *aReturn = SaveError(nsInstall::OUT_OF_MEMORY);
     return NS_OK;
  }
  nsCOMPtr<nsIFile> localTargetFile = aTarget.GetFileSpec();
  if (localTargetFile == nsnull)
  {
     *aReturn = SaveError(nsInstall::OUT_OF_MEMORY);
     return NS_OK;
  }

  nsInstallFileOpItem* ifop = new nsInstallFileOpItem(this, NS_FOP_FILE_MOVE, localSrcFile, localTargetFile, aReturn);
  if (ifop == nsnull)
  {
      *aReturn = SaveError(nsInstall::OUT_OF_MEMORY);
      return NS_OK;
  }

  PRInt32 result = SanityCheck();
  if (result != nsInstall::SUCCESS)
  {
      delete ifop;
      *aReturn = SaveError( result );
      return NS_OK;
  }

  if (*aReturn == nsInstall::SUCCESS) 
  {
      *aReturn = ScheduleForInstall( ifop );
  }

  SaveError(*aReturn);

  return NS_OK;
}

PRInt32
nsInstall::FileOpFileRename(nsInstallFolder& aSrc, nsString& aTarget, PRInt32* aReturn)
{
  nsCOMPtr<nsIFile> localFile = aSrc.GetFileSpec();
  if (localFile == nsnull)
  {
     *aReturn = SaveError(nsInstall::OUT_OF_MEMORY);
     return NS_OK;
  }

  nsInstallFileOpItem* ifop = new nsInstallFileOpItem(this, NS_FOP_FILE_RENAME, localFile, aTarget, aReturn);
  if (ifop == nsnull)
  {
      *aReturn = SaveError(nsInstall::OUT_OF_MEMORY);
      return NS_OK;
  }

  PRInt32 result = SanityCheck();
  if (result != nsInstall::SUCCESS)
  {
      delete ifop;
      *aReturn = SaveError( result );
      return NS_OK;
  }

  if (*aReturn == nsInstall::SUCCESS) 
  {
      *aReturn = ScheduleForInstall( ifop );
  }
  
  SaveError(*aReturn);

  return NS_OK;
}

PRInt32
nsInstall::FileOpFileWindowsShortcut(nsIFile* aTarget, nsIFile* aShortcutPath, nsString& aDescription, nsIFile* aWorkingPath, nsString& aParams, nsIFile* aIcon, PRInt32 aIconId, PRInt32* aReturn)
{

  nsInstallFileOpItem* ifop = new nsInstallFileOpItem(this, NS_FOP_WIN_SHORTCUT, aTarget, aShortcutPath, aDescription, aWorkingPath, aParams, aIcon, aIconId, aReturn);
  if (ifop == nsnull)
  {
      *aReturn = SaveError(nsInstall::OUT_OF_MEMORY);
      return NS_OK;
  }

  PRInt32 result = SanityCheck();
  if (result != nsInstall::SUCCESS)
  {
      delete ifop;
      *aReturn = SaveError( result );
      return NS_OK;
  }

  if (*aReturn == nsInstall::SUCCESS) 
  {
      *aReturn = ScheduleForInstall( ifop );
  }
      
  SaveError(*aReturn);

  return NS_OK;
}

PRInt32
nsInstall::FileOpFileMacAlias(nsIFile *aSourceFile, nsIFile *aAliasFile, PRInt32* aReturn)
{

  *aReturn = nsInstall::SUCCESS;

#ifdef XP_MAC
  nsInstallFileOpItem* ifop = new nsInstallFileOpItem(this, NS_FOP_MAC_ALIAS, aSourceFile, aAliasFile, aReturn);
  if (!ifop)
  {
      *aReturn = SaveError(nsInstall::OUT_OF_MEMORY);
      return NS_OK;
  }
      
  PRInt32 result = SanityCheck();
  if (result != nsInstall::SUCCESS)
  {
      *aReturn = SaveError( result );
      return NS_OK;
  }

  if (ifop == nsnull)
  {
      *aReturn = SaveError(nsInstall::OUT_OF_MEMORY);
      return NS_OK;
  }

  if (*aReturn == nsInstall::SUCCESS) 
  {
      *aReturn = ScheduleForInstall( ifop );
  }
      
  SaveError(*aReturn);
#endif

  return NS_OK;
}

PRInt32
nsInstall::FileOpFileUnixLink(nsInstallFolder& aTarget, PRInt32 aFlags, PRInt32* aReturn)
{
  return NS_OK;
}

PRInt32
nsInstall::FileOpWinRegisterServer(nsInstallFolder& aTarget, PRInt32* aReturn)
{
  nsCOMPtr<nsIFile> localFile = aTarget.GetFileSpec();
  if (localFile == nsnull)
  {
     *aReturn = SaveError(nsInstall::OUT_OF_MEMORY);
     return NS_OK;
  }

  nsInstallFileOpItem* ifop = new nsInstallFileOpItem(this, NS_FOP_WIN_REGISTER_SERVER, localFile, aReturn);
  if (ifop == nsnull)
  {
      *aReturn = SaveError(nsInstall::OUT_OF_MEMORY);
      return NS_OK;
  }

  PRInt32 result = SanityCheck();
  if (result != nsInstall::SUCCESS)
  {
      delete ifop;
      *aReturn = SaveError( result );
      return NS_OK;
  }
  
  if (*aReturn == nsInstall::SUCCESS) 
  {
      *aReturn = ScheduleForInstall( ifop );
  }
  
  SaveError(*aReturn);

  return NS_OK;
}

void
nsInstall::LogComment(nsString& aComment)
{
  if(mListener)
    mListener->LogComment(aComment.GetUnicode());
}

/////////////////////////////////////////////////////////////////////////
// Private Methods
/////////////////////////////////////////////////////////////////////////

/**
 * ScheduleForInstall
 * call this to put an InstallObject on the install queue
 * Do not call installedFiles.addElement directly, because this routine also 
 * handles progress messages
 */
PRInt32 
nsInstall::ScheduleForInstall(nsInstallObject* ob)
{
    PRInt32 error = nsInstall::SUCCESS;

    char *objString = ob->toString();

    // flash current item

    if (mListener)
        mListener->ItemScheduled(NS_ConvertASCIItoUCS2(objString).GetUnicode());


    // do any unpacking or other set-up
    error = ob->Prepare();
    
    if (error == nsInstall::SUCCESS) 
    {
        // Add to installation list
        mInstalledFiles->AppendElement( ob );

        // turn on flags for creating the uninstall node and
        // the package node for each InstallObject

        if (ob->CanUninstall())
            mUninstallPackage = PR_TRUE;

        if (ob->RegisterPackageNode())
            mRegisterPackage = PR_TRUE;
    }
    else if ( mListener )
    {
        // error in preparation step -- log it
        char* errRsrc = GetResourcedString(NS_ConvertASCIItoUCS2("ERROR"));
        if (errRsrc)
        {
            char* errprefix = PR_smprintf("%s (%d): ", errRsrc, error);
            nsString errstr; errstr.AssignWithConversion(errprefix);
            errstr.AppendWithConversion(objString);

            mListener->LogComment( errstr.GetUnicode() );

            PR_smprintf_free(errprefix);
            nsCRT::free(errRsrc);
        }   
    }

    if (objString)
        delete [] objString;
    
    return error;
}


/**
 * SanityCheck
 *
 * This routine checks if the packageName is null. It also checks the flag if the user cancels
 * the install progress dialog is set and acccordingly aborts the install.
 */
PRInt32
nsInstall::SanityCheck(void)
{
    if ( mInstalledFiles == nsnull || mStartInstallCompleted == PR_FALSE ) 
    {
        return INSTALL_NOT_STARTED;	
    }

    if (mUserCancelled) 
    {
        InternalAbort(USER_CANCELLED);
        return USER_CANCELLED;
    }
	
	return 0;
}

/**
 * GetQualifiedPackageName
 *
 * This routine converts a package-relative component registry name
 * into a full name that can be used in calls to the version registry.
 */

PRInt32
nsInstall::GetQualifiedPackageName( const nsString& name, nsString& qualifiedName )
{
    nsString startOfName;
    name.Left(startOfName, 7);

    if ( startOfName.EqualsWithConversion( "=USER=/") )
    {
        CurrentUserNode(qualifiedName);
        qualifiedName += name;
    }
    else
    {
        qualifiedName = name;
    }
    
    if (BadRegName(qualifiedName)) 
    {
        return BAD_PACKAGE_NAME;
    }


    /* Check to see if the PackageName ends in a '/'.  If it does nuke it. */

    if (qualifiedName.Last() == '/')
    {
        PRInt32 index = qualifiedName.Length();
        qualifiedName.Truncate(--index);
    }

    return SUCCESS;
}


/**
 * GetQualifiedRegName
 *
 * This routine converts a package-relative component registry name
 * into a full name that can be used in calls to the version registry.
 */
PRInt32
nsInstall::GetQualifiedRegName(const nsString& name, nsString& qualifiedRegName )
{
    nsString startOfName;
    name.Left(startOfName, 7);

    nsString usr ();

    if ( startOfName.EqualsWithConversion("=COMM=/") || startOfName.EqualsWithConversion("=USER=/")) 
    {
        qualifiedRegName = name;
        qualifiedRegName.Cut( 0, 7 );
    }
    else if ( name.CharAt(0) != '/' )
    {
        if (!mRegistryPackageName.IsEmpty())
        {
            qualifiedRegName = mRegistryPackageName;
            qualifiedRegName.AppendWithConversion("/");
            qualifiedRegName += name;
        }
        else
        {
            qualifiedRegName = name;
        }
    }
    else
    {
       qualifiedRegName = name;
    }

    if (BadRegName(qualifiedRegName)) 
    {
        return BAD_PACKAGE_NAME;
    }
  
    return SUCCESS;
}


static NS_DEFINE_IID(kPrefsIID, NS_IPREF_IID);
static NS_DEFINE_IID(kPrefsCID,  NS_PREF_CID);

void
nsInstall::CurrentUserNode(nsString& userRegNode)
{    
    char *profname;
    nsIPref * prefs;
    
    nsresult rv = nsServiceManager::GetService(kPrefsCID, 
                                               kPrefsIID,
                                               (nsISupports**) &prefs);


    if ( NS_SUCCEEDED(rv) )
    {
        rv = prefs->CopyCharPref("profile.name", &profname);

        if ( NS_FAILED(rv) )
        {
            PR_FREEIF(profname); // Allocated by PREF_CopyCharPref
            profname = NULL;
        }

        NS_RELEASE(prefs);
    }
    else
    {
        profname = NULL;
    }
    
    userRegNode.AssignWithConversion("/Netscape/Users/");
    if (profname != nsnull)
    {
        userRegNode.AppendWithConversion(profname);
        userRegNode.AppendWithConversion("/");
        PR_FREEIF(profname);
    }
}

// catch obvious registry name errors proactively
// rather than returning some cryptic libreg error
PRBool 
nsInstall::BadRegName(const nsString& regName)
{
    if ((regName.First() == ' ' ) || (regName.Last() == ' ' ))
        return PR_TRUE;
        
    if ( regName.Find("//") != -1 )
        return PR_TRUE;
     
    if ( regName.Find(" /") != -1 )
        return PR_TRUE;

    if ( regName.Find("/ ") != -1  )
        return PR_TRUE;        
    
    if ( regName.Find("=") != -1 )
        return PR_TRUE;           

    return PR_FALSE;
}

PRInt32    
nsInstall::SaveError(PRInt32 errcode)
{
  if ( errcode != nsInstall::SUCCESS ) 
    mLastError = errcode;
  
  return errcode;
}


/*
 * CleanUp
 * call	it when	done with the install
 *
 */
void 
nsInstall::CleanUp(void)
{
    nsInstallObject* ie;
    
    if ( mInstalledFiles != nsnull ) 
    {
        for (PRInt32 i=0; i < mInstalledFiles->Count(); i++) 
        {
            ie = (nsInstallObject*)mInstalledFiles->ElementAt(i);
            if (ie)
                delete ie;
        }

        mInstalledFiles->Clear();
        delete (mInstalledFiles);
        mInstalledFiles = nsnull;
    }

    if (mPatchList != nsnull)
    {
        mPatchList->Reset();
        delete mPatchList;
        mPatchList = nsnull;
    }
    
    if (mPackageFolder != nsnull)
    {
      delete (mPackageFolder);
      mPackageFolder = nsnull;
    }

    mRegistryPackageName.SetLength(0); // used to see if StartInstall() has been called
    mStartInstallCompleted = PR_FALSE;
}


void       
nsInstall::SetJarFileLocation(nsIFile* aFile)
{
    mJarFileLocation = aFile;
}

void       
nsInstall::GetInstallArguments(nsString& args)
{
    args = mInstallArguments;
}

void       
nsInstall::SetInstallArguments(const nsString& args)
{
    mInstallArguments = args;
}


void nsInstall::GetInstallURL(nsString& url)        { url = mInstallURL; }
void nsInstall::SetInstallURL(const nsString& url)  { mInstallURL = url; }


PRInt32    
nsInstall::Alert(nsString& string, nsresult status)
{
    nsresult res;

    NS_WITH_PROXIED_SERVICE(nsICommonDialogs, dialog, kCommonDialogsCID, NS_UI_THREAD_EVENTQ, &res);
    if (NS_FAILED(res)) 
        return res;

    return dialog->Alert(mParent, nsnull, string.GetUnicode(), status);
}

PRInt32    
nsInstall::Confirm(nsString& string, PRBool* aReturn)
{
    *aReturn = PR_FALSE; /* default value */
    
    nsresult res;  
    NS_WITH_PROXIED_SERVICE(nsICommonDialogs, dialog, kCommonDialogsCID, NS_UI_THREAD_EVENTQ, &res);
    if (NS_FAILED(res)) 
        return res;
    
    return dialog->Confirm(mParent, nsnull, string.GetUnicode(), aReturn);
}


// aJarFile         - This is the filepath within the jar file.
// aSuggestedName   - This is the name that we should try to extract to.  If we can, we will create a new temporary file.
// aRealName        - This is the name that we did extract to.  This will be allocated by use and should be disposed by the caller.

PRInt32    
nsInstall::ExtractFileFromJar(const nsString& aJarfile, nsIFile* aSuggestedName, nsIFile** aRealName)
{
    PRInt32 extpos = 0;
    nsCOMPtr<nsIFile> extractHereSpec;
    nsCOMPtr<nsILocalFile> tempFile;
    nsresult rv;

    if (aSuggestedName == nsnull)
    {
        NS_WITH_SERVICE(nsIProperties, directoryService, NS_DIRECTORY_SERVICE_CONTRACTID, &rv);

        directoryService->Get(NS_OS_TEMP_DIR, NS_GET_IID(nsIFile), getter_AddRefs(tempFile));
  
        nsString tempFileName; tempFileName.AssignWithConversion("xpinstall");

        // Get the extension of the file in the JAR
        extpos = aJarfile.RFindChar('.');
        if (extpos != -1)
        {
            // We found the extension; add it to the tempFileName string
            nsString extension;
            aJarfile.Right(extension, (aJarfile.Length() - extpos) );
            tempFileName += extension;
        }
        nsAutoCString temp(tempFileName);
        tempFile->Append(temp);

        // Create a temporary file to extract to
        MakeUnique(tempFile);

        tempFile->Clone(getter_AddRefs(extractHereSpec));

        if (extractHereSpec == nsnull)
            return nsInstall::OUT_OF_MEMORY;
    }
    else
    {
        // extract to the final destination.
        nsCOMPtr<nsIFile> temp;
        aSuggestedName->Clone(getter_AddRefs(temp));
        tempFile = do_QueryInterface(temp, &rv);
        if (tempFile == nsnull)
            return nsInstall::OUT_OF_MEMORY;

        MakeUnique(tempFile);
        extractHereSpec = tempFile;
    }

    // We will overwrite what is in the way.  is this something that we want to do?  
    extractHereSpec->Delete(PR_FALSE);

    //nsCOMPtr<nsILocalFile> file;
    //rv = NS_NewLocalFile(*extractHereSpec, PR_TRUE, getter_AddRefs(file));
    //if (NS_SUCCEEDED(rv))
    rv = mJarFileData->Extract(nsAutoCString(aJarfile), extractHereSpec);
    if (NS_FAILED(rv)) 
    {
        return EXTRACTION_FAILED;
    }
    
#ifdef XP_MAC
	FSSpec finalSpec, extractedSpec;
    
    nsCOMPtr<nsILocalFileMac> tempExtractHereSpec;
    tempExtractHereSpec = do_QueryInterface(extractHereSpec, &rv);
    tempExtractHereSpec->GetResolvedFSSpec(&extractedSpec);
	
	if ( nsAppleSingleDecoder::IsAppleSingleFile(&extractedSpec) )
	{
		nsAppleSingleDecoder *asd = new nsAppleSingleDecoder(&extractedSpec, &finalSpec);
        OSErr decodeErr = fnfErr;

        if (asd)
            decodeErr = asd->Decode();
	
		if (decodeErr != noErr)
		{			
			if (asd)
				delete asd;
			return EXTRACTION_FAILED;
		}
	
		if ( !(extractedSpec.vRefNum == finalSpec.vRefNum) ||
			 !(extractedSpec.parID   == finalSpec.parID)   ||
			 !(nsAppleSingleDecoder::PLstrcmp(extractedSpec.name, finalSpec.name)) )
		{
			// delete the unique extracted file that got renamed in AS decoding
			FSpDelete(&extractedSpec);
			
			// "real name" in AppleSingle entry may cause file rename
			tempExtractHereSpec->InitWithFSSpec(&finalSpec);
			extractHereSpec = do_QueryInterface(tempExtractHereSpec, &rv);
		}
	}		
#endif

    extractHereSpec->Clone(getter_AddRefs(aRealName));

    return nsInstall::SUCCESS;
}

/**
 * GetResourcedString
 * 
 * Obtains the string resource for actions and messages that are displayed
 * in user interface confirmation and progress dialogs.
 *
 * @param   aResName    - property name/identifier of string resource
 * @return  rscdStr     - corresponding resourced value in the string bundle
 */
char*
nsInstall::GetResourcedString(const nsString& aResName)
{
    nsString rscdStr;
    PRBool bStrBdlSuccess = PR_FALSE;

    if (mStringBundle)
    {   
        const PRUnichar *ucResName = aResName.GetUnicode();
        PRUnichar *ucRscdStr = nsnull;
        nsresult rv = mStringBundle->GetStringFromName(ucResName, &ucRscdStr);
        if (NS_SUCCEEDED(rv))
        {
            bStrBdlSuccess = PR_TRUE;
            rscdStr = ucRscdStr;
        }
    }
    
    /*
    ** We don't have a string bundle, the necessary libs, or something went wrong
    ** so we failover to hardcoded english strings so we log something rather
    ** than nothing due to failure above: always the case for the Install Wizards.
    */
    if (!bStrBdlSuccess)
    {
        nsAutoCString temp(aResName);
        rscdStr.AssignWithConversion(nsInstallResources::GetDefaultVal(temp));
    }
    
    return rscdStr.ToNewCString();
}


PRInt32 
nsInstall::ExtractDirEntries(const nsString& directory, nsVoidArray *paths)
{
    char                *buf;
    nsISimpleEnumerator *jarEnum = nsnull;
    nsIZipEntry         *currZipEntry = nsnull;

    if ( paths )
    {
        nsString pattern(directory);
        pattern.AppendWithConversion("/*");
        PRInt32 prefix_length = directory.Length()+1; // account for slash

        nsresult rv = mJarFileData->FindEntries( nsAutoCString(pattern), &jarEnum );
        if (NS_FAILED(rv) || !jarEnum)
            goto handle_err;

        PRBool bMore;
        rv = jarEnum->HasMoreElements(&bMore);
        while (bMore && NS_SUCCEEDED(rv))
        {
            rv = jarEnum->GetNext( (nsISupports**) &currZipEntry );
            if (currZipEntry)
            {
                // expensive 'buf' callee malloc per iteration!
                rv = currZipEntry->GetName(&buf);
                if (NS_FAILED(rv)) 
                    goto handle_err;
                if (buf)
                {
                    PRInt32 namelen = PL_strlen(buf);
                    NS_ASSERTION( prefix_length <= namelen, "Match must be longer than pattern!" );

                    if ( buf[namelen-1] != '/' ) 
                    {
                        // XXX manipulation should be in caller
                        nsString* tempString = new nsString; tempString->AssignWithConversion(buf+prefix_length);
                        paths->AppendElement(tempString);
                    }

                    PR_FREEIF( buf );
                }
                NS_IF_RELEASE(currZipEntry);
            }
            rv = jarEnum->HasMoreElements(&bMore);
        }
    }

    NS_IF_RELEASE(jarEnum);
    return SUCCESS;

handle_err:    
    NS_IF_RELEASE(jarEnum);                         
    NS_IF_RELEASE(currZipEntry); 
    return EXTRACTION_FAILED;
}

void
nsInstall::DeleteVector(nsVoidArray* vector)
{
    if (vector != nsnull)
    {
        for (PRInt32 i=0; i < vector->Count(); i++) 
        {
            nsString* element = (nsString*)vector->ElementAt(i);
            if (element != nsnull)
                delete element;
        }

        vector->Clear();
        delete (vector);
        vector = nsnull;
    }
}

nsresult MakeUnique(nsILocalFile* file)
{
    PRBool flagExists;

    nsresult rv = file->Exists(&flagExists);

    if (NS_FAILED(rv)) return rv;
    if (!flagExists) return NS_ERROR_FAILURE;

    char* leafName;
    
    rv = file->GetLeafName(&leafName);
    if (NS_FAILED(rv)) return rv;

    char* lastDot = strrchr(leafName, '.');
    char* suffix = "";
    if (lastDot)
    {
        suffix = nsCRT::strdup(lastDot); // include '.'
        *lastDot = '\0'; // strip suffix and dot.
    }

    // 27 should work on Macintosh, Unix, and Win32. 
    const int maxRootLength = 27 - nsCRT::strlen(suffix) - 1;

    if ((int)nsCRT::strlen(leafName) > (int)maxRootLength)
        leafName[maxRootLength] = '\0';

    for (short indx = 1; indx < 1000 && flagExists; indx++)
    {
        // start with "Picture-1.jpg" after "Picture.jpg" exists
        char newName[32];
        sprintf(newName, "%s-%d%s", leafName, indx, suffix);
        file->SetLeafName(newName);

        rv = file->Exists(&flagExists);
        if (NS_FAILED(rv)) return rv;
    }
    return NS_OK;
}


