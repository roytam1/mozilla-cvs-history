/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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
 */


#ifndef __NS_INSTALL_H__
#define __NS_INSTALL_H__

#include "nscore.h"
#include "nsISupports.h"

#include "jsapi.h"

#include "plevent.h"

#include "nsString.h"
#include "nsFileSpec.h"
#include "nsVoidArray.h"
#include "nsHashtable.h"
#include "nsCOMPtr.h"

#include "nsSoftwareUpdate.h"

#include "nsInstallObject.h"
#include "nsInstallVersion.h"
#include "nsInstallFolder.h"

#include "nsIXPINotifier.h"

#include "nsIStringBundle.h"
#include "nsILocale.h"
#include "nsIEventQueueService.h"
#include "nsIServiceManager.h"
#include "nsIComponentManager.h"
#include "nsIProperties.h"
#include "nsIEnumerator.h"
#include "nsIZipReader.h"


class nsInstallInfo
{
  public:
    
    nsInstallInfo( nsIFileSpec*     aFile, 
                   const PRUnichar* aURL, 
                   const PRUnichar* aArgs, 
                   long             aFlags, 
                   nsIXPINotifier*  aNotifier);

    virtual ~nsInstallInfo();

    nsresult GetLocalFile(nsFileSpec& aSpec);

    void GetURL(nsString& aURL) { aURL = mURL; }

    void GetArguments(nsString& aArgs) { aArgs = mArgs; }
    
    long GetFlags() { return mFlags; }

    nsIXPINotifier* GetNotifier() { return mNotifier; };
    
  private:
    
    nsresult  mError;

    long       mFlags;
    nsString   mURL;
    nsString   mArgs;

    nsCOMPtr<nsIFileSpec>       mFile;
    nsCOMPtr<nsIXPINotifier>    mNotifier;
};

#ifdef XP_PC
#define FILESEP '\\'
#elif defined XP_MAC
#define FILESEP ':'
#elif defined XP_BEOS
#define FILESEP '/'
#else
#define FILESEP '/'
#endif

class nsInstall
{
    friend class nsWinReg;
    friend class nsWinProfile;

    public:
       
        enum 
        {
            BAD_PACKAGE_NAME            = -200,
            UNEXPECTED_ERROR            = -201,
            ACCESS_DENIED               = -202,
            TOO_MANY_CERTIFICATES       = -203,
            NO_INSTALL_SCRIPT           = -204,
            NO_CERTIFICATE              = -205,
            NO_MATCHING_CERTIFICATE     = -206,
            CANT_READ_ARCHIVE           = -207,
            INVALID_ARGUMENTS           = -208,
            ILLEGAL_RELATIVE_PATH       = -209,
            USER_CANCELLED              = -210,
            INSTALL_NOT_STARTED         = -211,
            SILENT_MODE_DENIED          = -212,
            NO_SUCH_COMPONENT           = -213,
            DOES_NOT_EXIST              = -214,
            READ_ONLY                   = -215,
            IS_DIRECTORY                = -216,
            NETWORK_FILE_IS_IN_USE      = -217,
            APPLE_SINGLE_ERR            = -218,
            INVALID_PATH_ERR            = -219,
            PATCH_BAD_DIFF              = -220,
            PATCH_BAD_CHECKSUM_TARGET   = -221,
            PATCH_BAD_CHECKSUM_RESULT   = -222,
            UNINSTALL_FAILED            = -223,
            PACKAGE_FOLDER_NOT_SET      = -224,
            EXTRACTION_FAILED           = -225,
            FILENAME_ALREADY_USED       = -226,
            ABORT_INSTALL               = -227,
            DOWNLOAD_ERROR              = -228,
            SCRIPT_ERROR                = -229,

            ALREADY_EXISTS              = -230,
            IS_FILE                     = -231,
            SOURCE_DOES_NOT_EXIST       = -232,
            SOURCE_IS_DIRECTORY         = -233,
            SOURCE_IS_FILE              = -234,
            
            OUT_OF_MEMORY               = -299,

            GESTALT_UNKNOWN_ERR         = -5550,
            GESTALT_INVALID_ARGUMENT    = -5551,
            
            SUCCESS                     = 0,
            REBOOT_NEEDED               = 999,
            
            LIMITED_INSTALL             = 0,
            FULL_INSTALL                = 1,
            NO_STATUS_DLG               = 2,
            NO_FINALIZE_DLG             = 4,

            INSTALL_FILE_UNEXPECTED_MSG_ID = 0,
            DETAILS_REPLACE_FILE_MSG_ID = 1,
            DETAILS_INSTALL_FILE_MSG_ID = 2
        };


        nsInstall();
        virtual ~nsInstall();
        
        PRInt32    SetScriptObject(void* aScriptObject);

        PRInt32    SaveWinRegPrototype(void* aScriptObject);
        PRInt32    SaveWinProfilePrototype(void* aScriptObject);
        
        JSObject*  RetrieveWinRegPrototype(void);
        JSObject*  RetrieveWinProfilePrototype(void);

        PRInt32    GetUserPackageName(nsString& aUserPackageName);
        PRInt32    GetRegPackageName(nsString& aRegPackageName);

        PRInt32    AbortInstall(PRInt32 aErrorNumber);
        
        PRInt32    AddDirectory(const nsString& aRegName, const nsString& aVersion, const nsString& aJarSource, nsInstallFolder* aFolder, const nsString& aSubdir, PRBool aForceMode, PRInt32* aReturn);
        PRInt32    AddDirectory(const nsString& aRegName, const nsString& aVersion, const nsString& aJarSource, nsInstallFolder* aFolder, const nsString& aSubdir, PRInt32* aReturn);
        PRInt32    AddDirectory(const nsString& aRegName, const nsString& aJarSource, nsInstallFolder* aFolder, const nsString& aSubdir, PRInt32* aReturn);
        PRInt32    AddDirectory(const nsString& aJarSource, PRInt32* aReturn);
        
        PRInt32    AddSubcomponent(const nsString& aRegName, const nsString& aVersion, const nsString& aJarSource, nsInstallFolder *aFolder, const nsString& aTargetName, PRBool aForceMode, PRInt32* aReturn);
        PRInt32    AddSubcomponent(const nsString& aRegName, const nsString& aVersion, const nsString& aJarSource, nsInstallFolder *aFolder, const nsString& aTargetName, PRInt32* aReturn);
        PRInt32    AddSubcomponent(const nsString& aRegName, const nsString& aJarSource, nsInstallFolder *aFolder, const nsString& aTargetName, PRInt32* aReturn);
        PRInt32    AddSubcomponent(const nsString& aJarSource, PRInt32* aReturn);
        
        PRInt32    DeleteComponent(const nsString& aRegistryName, PRInt32* aReturn);
        PRInt32    DeleteFile(nsInstallFolder* aFolder, const nsString& aRelativeFileName, PRInt32* aReturn);
        PRInt32    DiskSpaceAvailable(const nsString& aFolder, PRInt64* aReturn);
        PRInt32    Execute(const nsString& aJarSource, const nsString& aArgs, PRInt32* aReturn);
        PRInt32    Execute(const nsString& aJarSource, PRInt32* aReturn);
        PRInt32    FinalizeInstall(PRInt32* aReturn);
        PRInt32    Gestalt(const nsString& aSelector, PRInt32* aReturn);
        
        PRInt32    GetComponentFolder(const nsString& aComponentName, const nsString& aSubdirectory, nsInstallFolder** aFolder);
        PRInt32    GetComponentFolder(const nsString& aComponentName, nsInstallFolder** aFolder);
        
        PRInt32    GetFolder(nsInstallFolder& aTargetFolder, const nsString& aSubdirectory, nsInstallFolder** aFolder);
        PRInt32    GetFolder(const nsString& aTargetFolder, const nsString& aSubdirectory, nsInstallFolder** aFolder);
        PRInt32    GetFolder(const nsString& aTargetFolder, nsInstallFolder** aFolder);
        
        PRInt32    GetLastError(PRInt32* aReturn);
        PRInt32    GetWinProfile(const nsString& aFolder, const nsString& aFile, JSContext* jscontext, JSClass* WinProfileClass, jsval* aReturn);
        PRInt32    GetWinRegistry(JSContext* jscontext, JSClass* WinRegClass, jsval* aReturn);
        PRInt32	   LoadResources(JSContext* cx, const nsString& aBaseName, jsval* aReturn);
        PRInt32    Patch(const nsString& aRegName, const nsString& aVersion, const nsString& aJarSource, nsInstallFolder* aFolder, const nsString& aTargetName, PRInt32* aReturn);
        PRInt32    Patch(const nsString& aRegName, const nsString& aJarSource, nsInstallFolder* aFolder, const nsString& aTargetName, PRInt32* aReturn);
        PRInt32    ResetError();
        PRInt32    SetPackageFolder(nsInstallFolder& aFolder);
        PRInt32    StartInstall(const nsString& aUserPackageName, const nsString& aPackageName, const nsString& aVersion, PRInt32* aReturn);
        PRInt32    Uninstall(const nsString& aPackageName, PRInt32* aReturn);
        
        PRInt32    FileOpDirCreate(nsFileSpec& aTarget, PRInt32* aReturn);
        PRInt32    FileOpDirGetParent(nsFileSpec& aTarget, nsFileSpec* aReturn);
        PRInt32    FileOpDirRemove(nsFileSpec& aTarget, PRInt32 aFlags, PRInt32* aReturn);
        PRInt32    FileOpDirRename(nsFileSpec& aSrc, nsString& aTarget, PRInt32* aReturn);
        PRInt32    FileOpFileCopy(nsFileSpec& aSrc, nsFileSpec& aTarget, PRInt32* aReturn);
        PRInt32    FileOpFileDelete(nsFileSpec& aTarget, PRInt32 aFlags, PRInt32* aReturn);
        PRInt32    FileOpFileExists(nsFileSpec& aTarget, PRBool* aReturn);
        PRInt32    FileOpFileExecute(nsFileSpec& aTarget, nsString& aParams, PRInt32* aReturn);
        PRInt32    FileOpFileGetNativeVersion(nsFileSpec& aTarget, nsString* aReturn);
        PRInt32    FileOpFileGetDiskSpaceAvailable(nsFileSpec& aTarget, PRInt64* aReturn);
        PRInt32    FileOpFileGetModDate(nsFileSpec& aTarget, nsFileSpec::TimeStamp* aReturn);
        PRInt32    FileOpFileGetSize(nsFileSpec& aTarget, PRUint32* aReturn);
        PRInt32    FileOpFileIsDirectory(nsFileSpec& aTarget, PRBool* aReturn);
        PRInt32    FileOpFileIsFile(nsFileSpec& aTarget, PRBool* aReturn);
        PRInt32    FileOpFileModDateChanged(nsFileSpec& aTarget, nsFileSpec::TimeStamp& aOldStamp, PRBool* aReturn);
        PRInt32    FileOpFileMove(nsFileSpec& aSrc, nsFileSpec& aTarget, PRInt32* aReturn);
        PRInt32    FileOpFileRename(nsFileSpec& aSrc, nsString& aTarget, PRInt32* aReturn);
        PRInt32    FileOpFileWindowsShortcut(nsFileSpec& aTarget, nsFileSpec& aShortcutPath, nsString& aDescription, nsFileSpec& aWorkingPath, nsString& aParams, nsFileSpec& aIcon, PRInt32 aIconId, PRInt32* aReturn);
        PRInt32    FileOpFileMacAlias(nsString& aSourcePath, nsString& aAliasPath, PRInt32* aReturn);
        PRInt32    FileOpFileUnixLink(nsFileSpec& aTarget, PRInt32 aFlags, PRInt32* aReturn);

        void       LogComment(nsString& aComment);

        PRInt32    ExtractFileFromJar(const nsString& aJarfile, nsFileSpec* aSuggestedName, nsFileSpec** aRealName);
        char*      GetResourcedString(const nsString& aResName);
        void       AddPatch(nsHashKey *aKey, nsFileSpec* fileName);
        void       GetPatch(nsHashKey *aKey, nsFileSpec** fileName);
        
        void       GetJarFileLocation(nsString& aFile);
        void       SetJarFileLocation(const nsFileSpec& aFile);

        void       GetInstallArguments(nsString& args);
        void       SetInstallArguments(const nsString& args);

        void       GetInstallURL(nsString& url);
        void       SetInstallURL(const nsString& url);

        PRBool     GetStatusSent() { return mStatusSent; }
        PRBool     InInstallTransaction(void) { return mInstalledFiles != nsnull; }
        
        PRInt32    Alert(nsString& string);
        PRInt32    Confirm(nsString& string, PRBool* aReturn);
        void       InternalAbort(PRInt32 errcode);



    private:
        JSObject*           mScriptObject;
        
        JSObject*           mWinRegObject;
        JSObject*           mWinProfileObject;

        
        nsFileSpec          mJarFileLocation;
        nsIZipReader*       mJarFileData;
        
        nsString            mInstallArguments;
        nsString            mInstallURL;
        nsInstallFolder*    mPackageFolder;

        PRBool              mUserCancelled;
        PRBool              mStatusSent;
        
        PRBool              mUninstallPackage;
        PRBool              mRegisterPackage;
        PRBool              mStartInstallCompleted;

        nsString            mRegistryPackageName;   /* Name of the package we are installing */
        nsString            mUIName;                /* User-readable package name */
        nsInstallVersion*   mVersionInfo;           /* Component version info */
        
        nsVoidArray*        mInstalledFiles;        
        nsHashtable*        mPatchList;
        
        nsIXPINotifier      *mNotifier;

        nsCOMPtr<nsIStringBundle>   mStringBundle;

        PRInt32             mLastError;

        void        ParseFlags(int flags);
        PRInt32     SanityCheck(void);
        void        GetTime(nsString &aString);

        
        PRInt32     GetQualifiedRegName(const nsString& name, nsString& qualifiedRegName );
        PRInt32     GetQualifiedPackageName( const nsString& name, nsString& qualifiedName );
        
        void        CurrentUserNode(nsString& userRegNode);
        PRBool      BadRegName(const nsString& regName);
        PRInt32     SaveError(PRInt32 errcode);

        void        CleanUp();

        PRInt32     OpenJARFile(void);
        void        CloseJARFile(void);
        PRInt32     ExtractDirEntries(const nsString& directory, nsVoidArray *paths);

        PRInt32     ScheduleForInstall(nsInstallObject* ob);

        static void DeleteVector(nsVoidArray* vector);    
};

#endif
