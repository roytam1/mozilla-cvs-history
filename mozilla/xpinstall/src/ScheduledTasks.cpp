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

#include "nscore.h"
#include "nsXPIDLString.h"
#include "nsFileSpec.h"
#include "nsFileStream.h"
#include "nsInstall.h" // for error codes
#include "prmem.h"
#include "ScheduledTasks.h"
#include "InstallCleanupDefines.h"

#include "nsSpecialSystemDirectory.h"
#include "nsDirectoryService.h"
#include "nsDirectoryServiceDefs.h"

static nsresult 
GetPersistentStringFromSpec(nsIFile* inSpec, char **string)
{
    nsresult rv;

    if (!string) return NS_ERROR_NULL_POINTER;

    nsCOMPtr<nsILocalFile> LocalFile = do_QueryInterface(inSpec, &rv);

    if (NS_SUCCEEDED(rv)) {
        rv = LocalFile->GetPath(string);
    } 
    else {
        *string = nsnull;
    }
    return rv;
}





#ifdef _WINDOWS
#include <sys/stat.h>
#include <windows.h>

PRInt32 ReplaceWindowsSystemFile(nsIFile* currentSpec, nsIFile* finalSpec)
{
    PRInt32 err = -1;

    // Get OS version info 
    DWORD dwVersion = GetVersion();

    char *final;
    char *current;

    finalSpec->GetPath(&final);
    currentSpec->GetPath(&current);
 
    // Get build numbers for Windows NT or Win32s 

    if (dwVersion > 0x80000000)
    {
        // Windows 95 or Win16

        // Place an entry in the WININIT.INI file in the Windows directory
        // to delete finalName and rename currentName to be finalName at reboot

        int     strlen;
        char    Src[_MAX_PATH];   // 8.3 name
        char    Dest[_MAX_PATH];  // 8.3 name
        
        strlen = GetShortPathName( (LPCTSTR)current, (LPTSTR)Src, (DWORD)sizeof(Src) );
        if ( strlen > 0 ) 
        {
            free(current);
            current = strdup(Src);
        }

        strlen = GetShortPathName( (LPCTSTR) final, (LPTSTR) Dest, (DWORD) sizeof(Dest));
        if ( strlen > 0 ) 
        {
            free(final);
            final = strdup(Dest);
        }
        
        // NOTE: use OEM filenames! Even though it looks like a Windows
        //       .INI file, WININIT.INI is processed under DOS 
        
        AnsiToOem( final, final );
        AnsiToOem( current, current );

        if ( WritePrivateProfileString( "Rename", final, current, "WININIT.INI" ) )
            err = 0;
    }
    else
    {
       // Windows NT
        if ( MoveFileEx(final, current, MOVEFILE_DELAY_UNTIL_REBOOT) )
          err = 0;
    }

    free(final);
    free(current);
    
    return err;
}
#endif

char* GetRegFilePath()
{
    nsresult rv;
    nsCOMPtr<nsILocalFile> iFileUtilityPath;
    //Get the program directory
    nsCOMPtr<nsIProperties> directoryService = 
             do_GetService(NS_DIRECTORY_SERVICE_CONTRACTID, &rv);
    if (NS_FAILED(rv))
        return nsnull;

    directoryService->Get(NS_OS_CURRENT_PROCESS_DIR, 
                          NS_GET_IID(nsIFile), 
                          getter_AddRefs(iFileUtilityPath));
    if (!iFileUtilityPath)
        return nsnull;

#if defined (XP_MAC)
    iFileUtilityPath->Append(ESSENTIAL_FILES);
#endif
    iFileUtilityPath->Append(CLEANUP_REGISTRY);

    //Yes, we know using GetPath is buggy on the Mac.
    //When libreg is fixed to accept nsIFiles we'll change this to match.
    char* regFilePath;
    iFileUtilityPath->GetPath(&regFilePath);
  
    return regFilePath;
}


PRInt32 DeleteFileNowOrSchedule(nsIFile* filename)
{
    PRBool flagExists;  
    PRInt32 result = nsInstall::SUCCESS;

    filename->Remove(PR_FALSE);
    filename->Exists(&flagExists);
    if (flagExists)
        result = ScheduleFileForDeletion(filename);
 
    return result;
} 

PRInt32 ScheduleFileForDeletion(nsIFile *filename)
{
    // could not delete, schedule it for later

    RKEY newkey;
    HREG reg;
    REGERR  err;
    PRInt32 result = nsInstall::UNEXPECTED_ERROR;

    char* regFilePath = GetRegFilePath();
    err = NR_RegOpen(regFilePath, &reg);
    if (regFilePath)
      nsCRT::free(regFilePath);

    if ( err == REGERR_OK )
    {
        err = NR_RegAddKey(reg,ROOTKEY_PRIVATE,REG_DELETE_LIST_KEY,&newkey);
        if ( err == REGERR_OK )
        {
            char    valname[20];

            err = NR_RegGetUniqueName( reg, valname, sizeof(valname) );
            if ( err == REGERR_OK )
            {
                nsXPIDLCString nameowner;
                nsresult rv = GetPersistentStringFromSpec(
                                filename,getter_Copies(nameowner));
                if ( NS_SUCCEEDED(rv) && nameowner )
                {
                    const char *fnamestr = nameowner;
                    err = NR_RegSetEntry( reg, newkey, valname, 
                                          REGTYPE_ENTRY_BYTES, 
                                          (void*)fnamestr, 
                                          strlen(fnamestr)+sizeof('\0'));

                    if ( err == REGERR_OK )
                    {
                         result = nsInstall::REBOOT_NEEDED;
                         nsSoftwareUpdate::NeedCleanup();
                    }
                }
            }
        }

        NR_RegClose(reg);
    }

    return result;
}




PRInt32 ReplaceFileNow(nsIFile* replacementFile, nsIFile* doomedFile )
{
    PRBool flagExists, flagIsEqual;
    nsresult rv;

    // replacement file must exist, doomed file doesn't have to
    replacementFile->Exists(&flagExists);
    if ( !flagExists )
        return nsInstall::DOES_NOT_EXIST;

    // don't have to do anything if the files are the same
    replacementFile->Equals(doomedFile, &flagIsEqual);
    if ( flagIsEqual )
        return nsInstall::SUCCESS;


    PRInt32 result = nsInstall::ACCESS_DENIED;

    // first try to rename the doomed file out of the way (if it exists)
    nsCOMPtr<nsIFile>      renamedDoomedFile;
    nsCOMPtr<nsILocalFile> tmpLocalFile;
    nsCOMPtr<nsIFile> parent;
    nsXPIDLCString leafname;
    
    doomedFile->Clone(getter_AddRefs(renamedDoomedFile));
    renamedDoomedFile->Exists(&flagExists);
    if ( flagExists )
    {
        tmpLocalFile = do_QueryInterface(renamedDoomedFile, &rv); // Convert to an nsILocalFile

        //get the leafname so we can convert its extension to .old
        nsXPIDLCString uniqueLeafName;
        tmpLocalFile->GetLeafName(getter_Copies(leafname));
        nsCString newLeafName (leafname); 

        PRInt32 extpos = newLeafName.RFindChar('.');
        if (extpos != -1)
        {
            // We found the extension; 
            newLeafName.Truncate(extpos + 1); //strip off the old extension
        }
        newLeafName.Append("old");
        
        //Now reset the leafname
        tmpLocalFile->SetLeafName(newLeafName.get());
        
        MakeUnique(tmpLocalFile);                                 //  for the call to MakeUnique
        
        tmpLocalFile->GetParent(getter_AddRefs(parent)); //get the parent for later use in MoveTo
        tmpLocalFile->GetLeafName(getter_Copies(uniqueLeafName));//this is the new "unique" leafname

        rv = doomedFile->Clone(getter_AddRefs(renamedDoomedFile));// Reset renamedDoomed file so doomedfile isn't 
                                                                  //   changed during the MoveTo call
        if (NS_FAILED(rv)) result = nsInstall::UNEXPECTED_ERROR;
        rv = renamedDoomedFile->MoveTo(parent, uniqueLeafName);        

        if (NS_SUCCEEDED(rv))
        {
            renamedDoomedFile = parent;                //MoveTo on Mac doesn't reset the tmpFile object to 
            renamedDoomedFile->Append(uniqueLeafName); //the new name or location. That's why there's this 
                                                       //explict assignment and Append call.
        }

        if (result == nsInstall::UNEXPECTED_ERROR)
            return result;
    }


    // if doomedFile is gone move new file into place
    doomedFile->Exists(&flagExists);
    if ( !flagExists )
    {
        nsCOMPtr<nsIFile> parentofFinalFile;
        nsCOMPtr<nsIFile> parentofReplacementFile;

        doomedFile->GetParent(getter_AddRefs(parentofFinalFile));
        replacementFile->GetParent(getter_AddRefs(parentofReplacementFile));

        // XXX looks dangerous, the replacement file name may NOT be unique in the
        // target directory if we have to move it! Either we should never move the
        // files like this (i.e. error if not in the same dir) or we need to take
        // a little more care in the move.
        parentofReplacementFile->Equals(parentofFinalFile, &flagIsEqual);
        if(!flagIsEqual)
        {
            NS_WARNING("File unpacked into a non-dest dir" );
            replacementFile->GetLeafName(getter_Copies(leafname));
            rv = replacementFile->MoveTo(parentofFinalFile, leafname);
        }
        	
        rv = doomedFile->GetLeafName(getter_Copies(leafname));
        if ( NS_SUCCEEDED(rv))
            rv = replacementFile->MoveTo(parentofReplacementFile, leafname );

        if ( NS_SUCCEEDED(rv) ) 
        {
            // we replaced the old file OK, now we have to
            // get rid of it if it was renamed out of the way
            result = DeleteFileNowOrSchedule( renamedDoomedFile );
        }
        else
        {
            // couldn't rename file, try to put old file back
            renamedDoomedFile->GetParent(getter_AddRefs(parent));
            renamedDoomedFile->MoveTo(parent, leafname);
        }
    }

    return result;
}





PRInt32 ReplaceFileNowOrSchedule(nsIFile* replacementFile, nsIFile* doomedFile, PRInt32 aMode)
{
    PRInt32 result = ReplaceFileNow( replacementFile, doomedFile );

    if ( result == nsInstall::ACCESS_DENIED )
    {
        // if we couldn't replace the file schedule it for later
#ifdef _WINDOWS
        if ( (aMode & WIN_SYSTEM_FILE) && 
             (ReplaceWindowsSystemFile(replacementFile, doomedFile) == 0) )
                return nsInstall::REBOOT_NEEDED;
#endif

        RKEY    listkey;
        RKEY    filekey;
        HREG    reg;
        REGERR  err;

        char* regFilePath = GetRegFilePath();
        if ( REGERR_OK == NR_RegOpen(regFilePath, &reg) ) 
        {
            err = NR_RegAddKey( reg, ROOTKEY_PRIVATE, REG_REPLACE_LIST_KEY, &listkey );
            if ( err == REGERR_OK ) 
            {
                char     valname[20];
                REGERR   err2;

                err = NR_RegGetUniqueName( reg, valname, sizeof(valname) );
                if ( err == REGERR_OK )
                {
                    err = NR_RegAddKey( reg, listkey, valname, &filekey );
                    if ( REGERR_OK == err )
                    {
                        nsXPIDLCString srcowner;
                        nsXPIDLCString destowner;
                        nsresult rv = GetPersistentStringFromSpec(
                                replacementFile, getter_Copies(srcowner));
                        nsresult rv2 = GetPersistentStringFromSpec(
                                doomedFile, getter_Copies(destowner));
                        if ( NS_SUCCEEDED(rv) && NS_SUCCEEDED(rv2) )
                        {

                            const char *fsrc  = srcowner;
                            const char *fdest = destowner;
                            err = NR_RegSetEntry( reg, filekey, 
                                                  REG_REPLACE_SRCFILE,
                                                  REGTYPE_ENTRY_BYTES, 
                                                  (void*)fsrc, 
                                                  strlen(fsrc)+sizeof('\0'));

                            err2 = NR_RegSetEntry(reg, filekey,
                                                  REG_REPLACE_DESTFILE,
                                                  REGTYPE_ENTRY_BYTES,
                                                  (void*)fdest,
                                                  strlen(fdest)+sizeof('\0'));

                            if ( err == REGERR_OK && err2 == REGERR_OK )
                            {
                                result = nsInstall::REBOOT_NEEDED;
                                nsSoftwareUpdate::NeedCleanup();
                            }
                            else
                                NR_RegDeleteKey( reg, listkey, valname );
                        }
                    }
                }
            }
            NR_RegClose(reg);
        }
        if (regFilePath)
            nsCRT::free(regFilePath);
    }

    return result;
}




//-----------------------------------------------------------------------------
//
//          STARTUP: DO SCHEDULED ACTIONS
//
//-----------------------------------------------------------------------------

void DeleteScheduledFiles(HREG);
void ReplaceScheduledFiles(HREG);

void PerformScheduledTasks(HREG reg)
{
    DeleteScheduledFiles( reg );
    ReplaceScheduledFiles( reg );
}



void DeleteScheduledFiles( HREG reg )
{
    REGERR  err;
    RKEY    key;
    REGENUM state = 0;
    nsresult rv = NS_OK;

    // perform scheduled file deletions  
    if (REGERR_OK == NR_RegGetKey(reg,ROOTKEY_PRIVATE,REG_DELETE_LIST_KEY,&key))
    {
        // the delete key exists, so we loop through its children
        // and try to delete all the listed files

        char    namebuf[MAXREGNAMELEN];
        char    valbuf[MAXREGPATHLEN];

        nsCOMPtr<nsIFile>        doomedFile;
        nsCOMPtr<nsILocalFile>   spec;

        if (NS_SUCCEEDED(rv))
        {
            while (REGERR_OK == NR_RegEnumEntries( reg, key, &state, namebuf,
                                                   sizeof(namebuf), 0 ) )
            {
                uint32 bufsize = sizeof(valbuf); // gets changed, must reset
                err = NR_RegGetEntry( reg, key, namebuf, valbuf, &bufsize );
                if ( err == REGERR_OK )
                {
                    // no need to check return value of 
                    // SetPersistentDescriptorString, it's always NS_OK
                    //spec->SetPersistentDescriptorString(valbuf); //nsIFileXXX: Do we still need this instead of InitWithPath?
                    NS_NewLocalFile((char*)valbuf, PR_TRUE, getter_AddRefs(spec));
                    spec->Clone(getter_AddRefs(doomedFile));
                    if (NS_SUCCEEDED(rv)) 
                    {
                        PRBool flagExists;
                        doomedFile->Remove(PR_FALSE);
                        doomedFile->Exists(&flagExists);
                        if ( !flagExists )
                        {
                            // deletion successful, don't have to retry
                            NR_RegDeleteEntry( reg, key, namebuf );
                        }
                    }
                }
            }

            // delete list node if empty 
            state = 0;
            err = NR_RegEnumEntries(reg, key, &state, namebuf, sizeof(namebuf), 0);
            if ( err == REGERR_NOMORE )
            {
                NR_RegDeleteKey(reg, ROOTKEY_PRIVATE, REG_DELETE_LIST_KEY);
            }
        }
    }
}



void ReplaceScheduledFiles( HREG reg )
{
    RKEY    key;

    // replace files if any listed 
    if (REGERR_OK == NR_RegGetKey(reg,ROOTKEY_PRIVATE,REG_REPLACE_LIST_KEY,&key))
    {
        char keyname[MAXREGNAMELEN];
        char doomedFile[MAXREGPATHLEN];
        char srcFile[MAXREGPATHLEN];

        nsCOMPtr<nsIFile>       doomedSpec;
        nsCOMPtr<nsIFile>       srcSpec;
        nsCOMPtr<nsILocalFile>       src;
        nsCOMPtr<nsILocalFile>       dest;
        nsresult                rv1, rv2;

        uint32 bufsize;
        REGENUM state = 0;
        while (REGERR_OK == NR_RegEnumSubkeys( reg, key, &state, 
                               keyname, sizeof(keyname), REGENUM_CHILDREN))
        {
            bufsize = sizeof(srcFile);
            REGERR err1 = NR_RegGetEntry( reg, (RKEY)state,
                               REG_REPLACE_SRCFILE, srcFile, &bufsize);

            bufsize = sizeof(doomedFile);
            REGERR err2 = NR_RegGetEntry( reg, (RKEY)state,
                               REG_REPLACE_DESTFILE, doomedFile, &bufsize);

            if ( err1 == REGERR_OK && err2 == REGERR_OK )
            {
                rv1 = NS_NewLocalFile((char*)srcFile, PR_TRUE, getter_AddRefs(src));
                rv1 = src->Clone(getter_AddRefs(srcSpec));

                rv2 = NS_NewLocalFile((char*)doomedFile, PR_TRUE, getter_AddRefs(dest));
                rv2 = dest->Clone(getter_AddRefs(doomedSpec));

                if (NS_SUCCEEDED(rv1) && NS_SUCCEEDED(rv2))
                {
                    // finally now try to do the replace
                    PRInt32 result = ReplaceFileNow( srcSpec, doomedSpec );

                    if ( result == nsInstall::DOES_NOT_EXIST ||
                         result == nsInstall::SUCCESS )
                    {
                        // This one is done
                        NR_RegDeleteKey( reg, key, keyname );
                    }
                }
            }
        }


        // delete list node if empty 
        state = 0;
        if (REGERR_NOMORE == NR_RegEnumSubkeys( reg, key, &state, keyname,
                                     sizeof(keyname), REGENUM_CHILDREN ))
        {
            NR_RegDeleteKey(reg, ROOTKEY_PRIVATE, REG_REPLACE_LIST_KEY);
        }
    }
}


