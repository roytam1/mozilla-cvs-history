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

#include "prprf.h"
#include "nsInstallFile.h"
#include "nsFileSpec.h"
#include "VerReg.h"
#include "ScheduledTasks.h"
#include "nsInstall.h"
#include "nsIDOMInstallVersion.h"
#include "nsInstallResources.h"

/* Public Methods */

/*	Constructor
        inInstall    - softUpdate object we belong to
        inComponentName	- full path of the registry component
        inVInfo	        - full version info
        inJarLocation   - location inside the JAR file
        inFinalFileSpec	- final	location on disk
*/

MOZ_DECL_CTOR_COUNTER(nsInstallFile);

nsInstallFile::nsInstallFile(nsInstall* inInstall,
                             const nsString& inComponentName,
                             const nsString& inVInfo,
                             const nsString& inJarLocation,
                             nsInstallFolder *folderSpec,
                             const nsString& inPartialPath,
                             PRBool forceInstall,
                             PRInt32 *error) 
  : nsInstallObject(inInstall),
    mVersionInfo(nsnull),
    mJarLocation(nsnull),
    mExtractedFile(nsnull),
    mFinalFile(nsnull),
    mVersionRegistryName(nsnull),
    mForceInstall(forceInstall),
    mReplaceFile(PR_FALSE),
    mChildFile(PR_TRUE),
    mUpgradeFile(PR_FALSE),
    mSkipInstall(PR_FALSE)
{
    MOZ_COUNT_CTOR(nsInstallFile);

    if ((folderSpec == nsnull) || (inInstall == NULL))
    {
        *error = nsInstall::INVALID_ARGUMENTS;
        return;
    }

    *error = nsInstall::SUCCESS;
    
    /* Check for existence of the newer	version	*/
    
    char* qualifiedRegNameString = inComponentName.ToNewCString();

    // --------------------------------------------------------------------
    // we always install if forceInstall is true, or the new file's
    // version is null, or the file doesn't previously exist.
    //
    // IFF it's not force, AND the new file has a version, AND it's been
    // previously installed, THEN we have to do the version comparing foo.
    // --------------------------------------------------------------------
    if ( (forceInstall == PR_FALSE ) && (inVInfo !=  "") && ( VR_ValidateComponent( qualifiedRegNameString ) == 0 ) ) 
    {
        nsInstallVersion *newVersion = new nsInstallVersion();
        
        if (newVersion == nsnull)
        {
            Recycle(qualifiedRegNameString);
            *error = nsInstall::OUT_OF_MEMORY;
            return;
        }

        newVersion->Init(inVInfo);
        
        VERSION versionStruct;
        
        VR_GetVersion( qualifiedRegNameString, &versionStruct );
        
        nsInstallVersion* oldVersion = new nsInstallVersion();
        
        if (newVersion == nsnull)
        {
            Recycle(qualifiedRegNameString);
            delete oldVersion;
            *error = nsInstall::OUT_OF_MEMORY;
            return;
        }

        oldVersion->Init(versionStruct.major,
                         versionStruct.minor,
                         versionStruct.release,
                         versionStruct.build);

        PRInt32 areTheyEqual;
        newVersion->CompareTo(oldVersion, &areTheyEqual);
        
        delete oldVersion;
        delete newVersion;

        if ( areTheyEqual < 0 )
        {
            // the file to be installed is OLDER than what is on disk.
            // Don't install it.
            Recycle(qualifiedRegNameString);
            mSkipInstall = PR_TRUE;
            return;
        }
    }

    Recycle(qualifiedRegNameString);

    nsFileSpec* tmp = folderSpec->GetFileSpec();
    if (!tmp)
    {
        *error = nsInstall::INVALID_ARGUMENTS;
        return;
    }

    mFinalFile = new nsFileSpec(*tmp);
    if (mFinalFile == nsnull)
    {
        *error = nsInstall::OUT_OF_MEMORY;
        return;
    }

    if ( mFinalFile->Exists() )
    {
        // is there a file with the same name as the proposed folder?
        if ( mFinalFile->IsFile() ) 
        {
            *error = nsInstall::FILENAME_ALREADY_USED;
            return;
        }
        // else this directory already exists, so do nothing
    }
    else
    {
        /* the nsFileSpecMac.cpp operator += requires "this" (the nsFileSpec)
         * to be an existing dir
         */
        int dirPermissions = 0755; // std default for UNIX, ignored otherwise
        mFinalFile->CreateDir(dirPermissions);
    }

    *mFinalFile += inPartialPath;
    
    mReplaceFile = mFinalFile->Exists();
    
    if (mReplaceFile == PR_FALSE)
    {
       /* although it appears that we are creating the dir _again_ it is necessary
        * when inPartialPath has arbitrary levels of nested dirs before the leaf
        */
        nsFileSpec parent;
        mFinalFile->GetParent(parent);
        nsFileSpec makeDirs(parent.GetCString(), PR_TRUE);
    }

    mVersionRegistryName    = new nsString(inComponentName);
    mJarLocation            = new nsString(inJarLocation);
    mVersionInfo	        = new nsString(inVInfo);
     
    if (mVersionRegistryName == nsnull ||
        mJarLocation         == nsnull ||
        mVersionInfo         == nsnull )
    {
        *error = nsInstall::OUT_OF_MEMORY;
        return;
    }

    nsString regPackageName;
    mInstall->GetRegPackageName(regPackageName);
    
    // determine Child status
    if ( regPackageName == "" ) 
    {
        // in the "current communicator package" absolute pathnames (start
        // with slash) indicate shared files -- all others are children
        mChildFile = ( mVersionRegistryName->CharAt(0) != '/' );
    } 
    else 
    {
        mChildFile = mVersionRegistryName->Equals( regPackageName,
                                                   PR_FALSE,
                                                   regPackageName.Length() );
    }
}


nsInstallFile::~nsInstallFile()
{
    if (mVersionRegistryName)
        delete mVersionRegistryName;
  
    if (mJarLocation)
        delete mJarLocation;
  
    if (mExtractedFile)
        delete mExtractedFile;

    if (mFinalFile)
        delete mFinalFile;
  
    if (mVersionInfo)
      delete mVersionInfo;

    MOZ_COUNT_DTOR(nsInstallFile);
}

/* Prepare
 * Extracts file out of the JAR archive
 */
PRInt32 nsInstallFile::Prepare()
{
    if (mInstall == nsnull || mFinalFile == nsnull || mJarLocation == nsnull )
        return nsInstall::INVALID_ARGUMENTS;

    if (mSkipInstall)
        return nsInstall::SUCCESS;

    return mInstall->ExtractFileFromJar(*mJarLocation, mFinalFile, &mExtractedFile);
}

/* Complete
 * Completes the install:
 * - move the downloaded file to the final location
 * - updates the registry
 */
PRInt32 nsInstallFile::Complete()
{
    PRInt32 err;

    if (mInstall == nsnull || mVersionRegistryName == nsnull || mFinalFile == nsnull ) 
    {
       return nsInstall::INVALID_ARGUMENTS;
    }
   
    if (mSkipInstall)
        return nsInstall::SUCCESS;

    err = CompleteFileMove();
    
    if ( 0 == err || nsInstall::REBOOT_NEEDED == err ) 
    {
        RegisterInVersionRegistry();
    }
    
    return err;

}

void nsInstallFile::Abort()
{
    if (mExtractedFile != nsnull)
        mExtractedFile->Delete(PR_FALSE);
}

#define RESBUFSIZE 1024
char* nsInstallFile::toString()
{
    char* buffer = new char[RESBUFSIZE];
    char* rsrcVal = nsnull;
    const char* fname = nsnull;

    if (buffer == nsnull || !mInstall)
        return nsnull;
    else
        buffer[0] = '\0';
    
    if (mReplaceFile)
    {
        rsrcVal = mInstall->GetResourcedString("ReplaceFile");
    }
    else if (mSkipInstall)
    {
        rsrcVal = mInstall->GetResourcedString("SkipFile");
    }
    else
    {
        rsrcVal = mInstall->GetResourcedString("InstallFile");
    }

    if (rsrcVal)
    {
        if (mFinalFile)
            fname = mFinalFile->GetCString();

        PR_snprintf( buffer, RESBUFSIZE, rsrcVal, fname );

        Recycle(rsrcVal);
    }

    return buffer;
}


PRInt32 nsInstallFile::CompleteFileMove()
{
    int result = 0;
    
    if (mExtractedFile == nsnull) 
    {
        return nsInstall::UNEXPECTED_ERROR;
    }
   	
    if ( *mExtractedFile == *mFinalFile ) 
    {
        /* No need to rename, they are the same */
        result = nsInstall::SUCCESS;
    } 
    else 
    {
        result = ReplaceFileNowOrSchedule(*mExtractedFile, *mFinalFile );
    }

  return result;  
}

PRInt32
nsInstallFile::RegisterInVersionRegistry()
{
    int refCount;
    nsString regPackageName;
    mInstall->GetRegPackageName(regPackageName);
    
  
    // Register file and log for Uninstall
  
    if (!mChildFile) 
    {
        int found;
        if (regPackageName != "") 
        {
            found = VR_UninstallFileExistsInList( (char*)(const char*)nsAutoCString(regPackageName) , 
                                                  (char*)(const char*)nsAutoCString(*mVersionRegistryName));
        } 
        else 
        {
            found = VR_UninstallFileExistsInList( "", (char*)(const char*)nsAutoCString(*mVersionRegistryName) );
        }
        
        if (found != REGERR_OK)
            mUpgradeFile = PR_FALSE;
        else
            mUpgradeFile = PR_TRUE;
    } 
    else if (REGERR_OK == VR_InRegistry( (char*)(const char*)nsAutoCString(*mVersionRegistryName))) 
    {
        mUpgradeFile = PR_TRUE;
    } 
    else 
    {
        mUpgradeFile = PR_FALSE;
    }

    if ( REGERR_OK != VR_GetRefCount( (char*)(const char*)nsAutoCString(*mVersionRegistryName), &refCount )) 
    {
        refCount = 0;
    }

    VR_Install( (char*)(const char*)nsAutoCString(*mVersionRegistryName), 
                (char*)(const char*)mFinalFile->GetNativePathCString(),  // DO NOT CHANGE THIS. 
                (char*)(const char*)nsAutoCString(*mVersionInfo), 
                PR_FALSE );

    if (mUpgradeFile) 
    {
        if (refCount == 0) 
            VR_SetRefCount( (char*)(const char*)nsAutoCString(*mVersionRegistryName), 1 );
        else 
            VR_SetRefCount( (char*)(const char*)nsAutoCString(*mVersionRegistryName), refCount );  //FIX?? what should the ref count be/
    }
    else
    {
        if (refCount != 0) 
        {
            VR_SetRefCount( (char*)(const char*)nsAutoCString(*mVersionRegistryName), refCount + 1 );
        } 
        else 
        {
            if (mReplaceFile)
                VR_SetRefCount( (char*)(const char*)nsAutoCString(*mVersionRegistryName), 2 );
            else
                VR_SetRefCount( (char*)(const char*)nsAutoCString(*mVersionRegistryName), 1 );
        }
    }
    
    if ( !mChildFile && !mUpgradeFile ) 
    {
        if (regPackageName != "") 
        { 
            VR_UninstallAddFileToList( (char*)(const char*)nsAutoCString(regPackageName), 
                                       (char*)(const char*)nsAutoCString(*mVersionRegistryName));
        } 
        else 
        {
            VR_UninstallAddFileToList( "", (char*)(const char*)nsAutoCString(*mVersionRegistryName) );
        }
    }

    return nsInstall::SUCCESS;
}

/* CanUninstall
* InstallFile() installs files which can be uninstalled,
* hence this function returns true. 
*/
PRBool
nsInstallFile::CanUninstall()
{
    return PR_TRUE;
}

/* RegisterPackageNode
* InstallFile() installs files which need to be registered,
* hence this function returns true.
*/
PRBool
nsInstallFile::RegisterPackageNode()
{
    return PR_TRUE;
}
