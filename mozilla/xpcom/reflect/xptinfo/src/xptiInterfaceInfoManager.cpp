/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1999 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

/* Implementation of xptiInterfaceInfoManager. */

#include "xptiprivate.h"

NS_IMPL_THREADSAFE_ISUPPORTS1(xptiInterfaceInfoManager, nsIInterfaceInfoManager)

static xptiInterfaceInfoManager* gInterfaceInfoManager = nsnull;

// static
xptiInterfaceInfoManager*
xptiInterfaceInfoManager::GetInterfaceInfoManagerNoAddRef()
{
    if(!gInterfaceInfoManager)
    {
        gInterfaceInfoManager = new xptiInterfaceInfoManager();
        if(gInterfaceInfoManager)
            NS_ADDREF(gInterfaceInfoManager);
        if(!gInterfaceInfoManager->IsValid())
        {
            NS_RELEASE(gInterfaceInfoManager);
        }
        else
        {
            PRBool mustAutoReg = 
                    !xptiManifest::Read(gInterfaceInfoManager, 
                                        &gInterfaceInfoManager->mWorkingSet);
#ifdef DEBUG
            {
            // This sets what will be returned by GetOpenLogFile().
            xptiAutoLog autoLog(gInterfaceInfoManager, 
                                gInterfaceInfoManager->mAutoRegLogFile, PR_TRUE);
            LOG_AUTOREG(("debug build forced autoreg after %s load of manifest\n", mustAutoReg ? "FAILED" : "successful"));
        
            mustAutoReg = PR_TRUE;
            }
#endif // DEBUG
            if(mustAutoReg)
                gInterfaceInfoManager->AutoRegisterInterfaces();
        }
    }
    return gInterfaceInfoManager;
}

void
xptiInterfaceInfoManager::FreeInterfaceInfoManager()
{
    if(gInterfaceInfoManager)
        gInterfaceInfoManager->LogStats();

    NS_IF_RELEASE(gInterfaceInfoManager);
}


PRBool 
xptiInterfaceInfoManager::IsValid()
{
    return mWorkingSet.IsValid() &&
           mResolveLock &&
           mAutoRegLock;
}        

xptiInterfaceInfoManager::xptiInterfaceInfoManager()
    :   mWorkingSet(),
        mOpenLogFile(nsnull),
        mResolveLock(PR_NewLock()),
        mAutoRegLock(PR_NewLock())
{
    NS_INIT_ISUPPORTS();

    nsCOMPtr<nsIPref> prefs = do_GetService(NS_PREF_PROGID);
    if(prefs)
    {
        char* statsFilename;
        if(NS_SUCCEEDED(prefs->CopyCharPref("xptinfo.logging.statsfilename",
                                            &statsFilename)) && statsFilename)
        {
            mStatsLogFile = do_CreateInstance(NS_LOCAL_FILE_PROGID);         
            if(mStatsLogFile && 
               NS_SUCCEEDED(mStatsLogFile->InitWithPath(statsFilename)))
            {
#ifdef DEBUG
                printf("***** logging xptinfo stats to: %s\n", statsFilename);
#endif
            }
            else
            {
                mStatsLogFile = nsnull;
            }
            nsCRT::free(statsFilename);
        }

        char* autoRegFilename;
        if(NS_SUCCEEDED(prefs->CopyCharPref("xptinfo.logging.autoregfilename",
                                            &autoRegFilename)) && autoRegFilename)
        {
            mAutoRegLogFile = do_CreateInstance(NS_LOCAL_FILE_PROGID);         
            if(mAutoRegLogFile && 
               NS_SUCCEEDED(mAutoRegLogFile->InitWithPath(autoRegFilename)))
            {
#ifdef DEBUG
                printf("***** logging xptinfo autoreg to: %s\n", autoRegFilename);
#endif
            }
            else
            {
                mAutoRegLogFile = nsnull;
            }
            nsCRT::free(autoRegFilename);
        }
    }
}

xptiInterfaceInfoManager::~xptiInterfaceInfoManager()
{
    // We only do this on shutdown of the service.
    mWorkingSet.InvalidateInterfaceInfos();

    if(mResolveLock)
        PR_DestroyLock(mResolveLock);
    if(mAutoRegLock)
        PR_DestroyLock(mAutoRegLock);
}

PRBool 
xptiInterfaceInfoManager::GetComponentsDir(nsILocalFile** aDir)
{
    NS_ASSERTION(aDir,"loser!");
    
    // Make a new one each time because caller *will* modify it.

    nsCOMPtr<nsILocalFile> dir = do_CreateInstance(NS_LOCAL_FILE_PROGID);
    if(!dir)
        return PR_FALSE;

    nsresult rv = dir->InitWithPath(
        nsSpecialSystemDirectory(
            nsSpecialSystemDirectory::XPCOM_CurrentProcessComponentDirectory));
    if(NS_FAILED(rv))
        return PR_FALSE;
    
    NS_ADDREF(*aDir = dir);
    return PR_TRUE;
}

PRBool 
xptiInterfaceInfoManager::BuildFileList(nsISupportsArray** aFileList)
{
    NS_ASSERTION(aFileList, "loser!");
    
    nsresult rv;

    nsCOMPtr<nsILocalFile> dir;
    
    rv = GetComponentsDir(getter_AddRefs(dir));
    if(NS_FAILED(rv) || !dir)
        return PR_FALSE;

    nsCOMPtr<nsISimpleEnumerator> entries;
    rv = dir->GetDirectoryEntries(getter_AddRefs(entries));
    if(NS_FAILED(rv) || !entries)
        return PR_FALSE;

    nsCOMPtr<nsISupportsArray> fileList = 
        do_CreateInstance(NS_SUPPORTSARRAY_PROGID);
    if(!fileList)
        return PR_FALSE;

    PRUint32 count = 0;
    PRBool hasMore;
    while(NS_SUCCEEDED(entries->HasMoreElements(&hasMore)) && hasMore)
    {
        nsCOMPtr<nsISupports> sup;
        entries->GetNext(getter_AddRefs(sup));
        if(!sup)
            return PR_FALSE;
        nsCOMPtr<nsIFile> file = do_QueryInterface(sup);
        if(!file)
            return PR_FALSE;

        PRBool isFile;
        if(NS_FAILED(file->IsFile(&isFile)) || !isFile)
        {
            continue;
        }
     
        char* name;
        if(NS_FAILED(file->GetLeafName(&name)))
            return PR_FALSE;

        if(xptiFileType::IsUnknown(name))
        {
            nsAllocator::Free(name);
            continue;
        }

        LOG_AUTOREG(("found file: %s\n", name));

        nsAllocator::Free(name);

        if(!fileList->InsertElementAt(file, count))
            return PR_FALSE;
        ++count;
    }

    NS_ADDREF(*aFileList = fileList); 
    return PR_TRUE;
}

XPTHeader* 
xptiInterfaceInfoManager::ReadXPTFile(nsILocalFile* aFile, 
                                      xptiWorkingSet* aWorkingSet)
{
    NS_ASSERTION(aFile, "loser!");

    XPTHeader *header = nsnull;
    char *whole = nsnull;
    PRFileDesc*   fd = nsnull;
    XPTState *state = nsnull;
    XPTCursor cursor;
    PRInt32 flen;
    PRInt64 fileSize;

    if(NS_FAILED(aFile->GetFileSize(&fileSize)) || !(flen = nsInt64(fileSize)))
    {
        return nsnull;
    }

    whole = new char[flen];
    if (!whole)
    {
        return nsnull;
    }

    // all exits from on here should be via 'goto out' 

    if(NS_FAILED(aFile->OpenNSPRFileDesc(PR_RDONLY, 0444, &fd)) || !fd)
    {
        goto out;
    }

    if(flen > PR_Read(fd, whole, flen))
    {
        goto out;
    }

    if(!(state = XPT_NewXDRState(XPT_DECODE, whole, flen)))
    {
        goto out;
    }
    
    if(!XPT_MakeCursor(state, XPT_HEADER, 0, &cursor))
    {
        goto out;
    }
    
    if (!XPT_DoHeader(aWorkingSet->GetStructArena(), &cursor, &header))
    {
        header = nsnull;
        goto out;
    }

 out:
    if(fd)
        PR_Close(fd);
    if(state)
        XPT_DestroyXDRState(state);
    if(whole)
        delete [] whole;
    return header;
}

PRBool 
xptiInterfaceInfoManager::LoadFile(const xptiTypelib& aTypelibRecord,
                                 xptiWorkingSet* aWorkingSet)
{
    if(!aWorkingSet)
        aWorkingSet = &mWorkingSet;

    if(!aWorkingSet->IsValid())
        return PR_FALSE;

    xptiFile* fileRecord = &aWorkingSet->GetFileAt(aTypelibRecord.GetFileIndex());
    xptiZipItem* zipItem = nsnull;

    nsCOMPtr<nsILocalFile> file;
    if(!GetComponentsDir(getter_AddRefs(file)) || !file)
        return PR_FALSE;

    if(NS_FAILED(file->Append(fileRecord->GetName())))
        return PR_FALSE;

    XPTHeader* header;

    if(aTypelibRecord.IsZip())
    {
        zipItem = &aWorkingSet->GetZipItemAt(aTypelibRecord.GetZipItemIndex());
        LOG_LOAD(("# loading zip item %s::%s\n", fileRecord->GetName(), zipItem->GetName()));
        header = xptiZipLoader::ReadXPTFileFromZip(file, zipItem->GetName(), aWorkingSet);
    } 
    else
    {
        LOG_LOAD(("^ loading file %s\n", fileRecord->GetName()));
        header = ReadXPTFile(file, aWorkingSet);
    } 

    if(!header)
        return PR_FALSE;


    if(aTypelibRecord.IsZip())
    {
        // This also allocs zipItem.GetGuts() used below.
        if(!zipItem->SetHeader(header))
            return PR_FALSE;
    }
    else
    {
        // This also allocs fileRecord.GetGuts() used below.
        if(!fileRecord->SetHeader(header))
            return PR_FALSE;
    }

    // For each interface in the header we want to find the xptiInterfaceInfo
    // object and set its resolution info.

    for(PRUint16 i = 0; i < header->num_interfaces; i++)
    {
        static const nsID zeroIID =
            { 0x0, 0x0, 0x0, { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 } };

        XPTInterfaceDirectoryEntry* iface = header->interface_directory + i;
        xptiInterfaceInfo* info;
        
        if(!iface->iid.Equals(zeroIID))
        {
            info = (xptiInterfaceInfo*)
                PL_HashTableLookup(aWorkingSet->mIIDTable, &iface->iid);
        }
        else
        {
            info = (xptiInterfaceInfo*)
                PL_HashTableLookup(aWorkingSet->mNameTable, iface->name);
        }

        if(!info)
        {
            // This one is just not resolved anywhere!
            continue;    
        }

        if(aTypelibRecord.IsZip())
            zipItem->GetGuts()->SetInfoAt(i, info);
        else
            fileRecord->GetGuts()->SetInfoAt(i, info);

        XPTInterfaceDescriptor* descriptor = iface->interface_descriptor;

        if(descriptor && aTypelibRecord.Equals(info->GetTypelibRecord()))
            info->PartiallyResolveLocked(descriptor, aWorkingSet);
    }
    return PR_TRUE;
}

static int
IndexOfFileWithName(const char* aName, const xptiWorkingSet* aWorkingSet)
{
    NS_ASSERTION(aName, "loser!");

    for(PRUint32 i = 0; i < aWorkingSet->GetFileCount(); ++i)
    {
        if(!PL_strcmp(aName, aWorkingSet->GetFileAt(i).GetName()))
            return i;     
    }
    return -1;        
}        

PR_STATIC_CALLBACK(int)
xptiSortFileList(const void * p1, const void *p2, void * closure)
{
    nsILocalFile* pFile1 = *((nsILocalFile**) p1);
    nsILocalFile* pFile2 = *((nsILocalFile**) p2);
    xptiWorkingSet* pWorkingSet = (xptiWorkingSet*) closure;
        
    char* name1;
    char* name2;
    
    if(NS_FAILED(pFile1->GetLeafName(&name1)))
    {
        NS_ASSERTION(0, "way bad, with no happy out!");
        return 0;    
    }    
    if(NS_FAILED(pFile2->GetLeafName(&name2)))
    {
        NS_ASSERTION(0, "way bad, with no happy out!");
        return 0;    
    }    

    int index1 = IndexOfFileWithName(name1, pWorkingSet); 
    int index2 = IndexOfFileWithName(name2, pWorkingSet); 
   
    // Get these now in case we need them later.
    PRBool isXPT1 = xptiFileType::IsXPT(name1);
    PRBool isXPT2 = xptiFileType::IsXPT(name2);
    int nameOrder = PL_strcmp(name1, name2);
    
    nsAllocator::Free(name1);
    nsAllocator::Free(name2);

    // XXX need to test with non-empty working sets to be sure this right

    // both in workingSet, preserve old order
    if(index1 != -1 && index2 != -1)
        return index1 - index2;

    if(index1 != -1)
        return 1;

    if(index2 != -1)
        return -1;

    // .xpt files come before archives (.zip, .jar, etc)
    if(isXPT1 &&!isXPT2)
        return -1;
        
    if(!isXPT1 && isXPT2)
        return 1;
    
    // neither element is in the workingSet and both are same type, sort by size

    PRInt64 size1;
    PRInt64 size2;

    if(NS_FAILED(pFile1->GetFileSize(&size1)))
    {
        NS_ASSERTION(0, "way bad, with no happy out!");
        return 0;    
    }    
    if(NS_FAILED(pFile2->GetFileSize(&size2)))
    {
        NS_ASSERTION(0, "way bad, with no happy out!");
        return 0;    
    }    

    // by size with largest first, or by name if size is the same
    int sizeDiff = int(PRInt32(nsInt64(size2) - nsInt64(size1)));
    return sizeDiff != 0  ? sizeDiff : nameOrder;
}        

nsILocalFile** 
xptiInterfaceInfoManager::BuildOrderedFileArray(nsISupportsArray* aFileList,
                                                xptiWorkingSet* aWorkingSet)
{
    // We want to end up with a file list that starts with the files from
    // aWorkingSet (but only those that are in aFileList) in the order in 
    // which they appeared in aWorkingSet-> Following those files will be those
    // files in aFileList which are not in aWorkingSet-> These additional
    // files will be ordered by file size (larger first) but all .xpt files
    // will preceed all zipfile of those files not already in the working set.
    // To do this we will do a fancy sort on a copy of aFileList.


    nsILocalFile** orderedFileList = nsnull;
    PRUint32 countOfFilesInFileList;
    PRUint32 i;

    NS_ASSERTION(aFileList, "loser!");
    NS_ASSERTION(aWorkingSet, "loser!");
    NS_ASSERTION(aWorkingSet->IsValid(), "loser!");

    if(NS_FAILED(aFileList->Count(&countOfFilesInFileList)) || 
       0 == countOfFilesInFileList)
        return nsnull;

    orderedFileList = (nsILocalFile**) 
        XPT_MALLOC(aWorkingSet->GetStructArena(),
                   sizeof(nsILocalFile*) * countOfFilesInFileList);
    
    if(!orderedFileList)
        return nsnull;

    // fill our list for sorting
    for(i = 0; i < countOfFilesInFileList; ++i)
    {
        nsCOMPtr<nsISupports> sup;
        aFileList->GetElementAt(i, getter_AddRefs(sup));
        NS_ASSERTION(sup, "loser!");
        nsCOMPtr<nsILocalFile> file = do_QueryInterface(sup);
        NS_ASSERTION(file, "loser!");

        // Intentionally NOT addref'd cuz we know these are pinned in aFileList.
        orderedFileList[i] = file.get();
    }

    // sort the filelist
    NS_QuickSort(orderedFileList, countOfFilesInFileList, sizeof(nsILocalFile*),
                 xptiSortFileList, aWorkingSet);
     
    return orderedFileList;
}

xptiInterfaceInfoManager::AutoRegMode 
xptiInterfaceInfoManager::DetermineAutoRegStrategy(nsISupportsArray* aFileList,
                                                   xptiWorkingSet* aWorkingSet)
{
    NS_ASSERTION(aFileList, "loser!");
    NS_ASSERTION(aWorkingSet, "loser!");
    NS_ASSERTION(aWorkingSet->IsValid(), "loser!");

    PRUint32 countOfFilesInWorkingSet = aWorkingSet->GetFileCount();
    PRUint32 countOfFilesInFileList;
    PRUint32 i;
    PRUint32 k;

    if(0 == countOfFilesInWorkingSet)
    {
        // Loading manifest might have failed. Better safe...     
        return FULL_VALIDATION_REQUIRED;
    }

    if(NS_FAILED(aFileList->Count(&countOfFilesInFileList)))
    {
        NS_ASSERTION(0, "unexpected!");
        return FULL_VALIDATION_REQUIRED;
    }       


    if(countOfFilesInFileList == countOfFilesInWorkingSet)
    {
        // try to determine if *no* files are new or changed.
     
        PRBool same = PR_TRUE;
        for(i = 0; i < countOfFilesInFileList && same; ++i)
        {
            nsCOMPtr<nsISupports> sup;
            aFileList->GetElementAt(i, getter_AddRefs(sup));
            NS_ASSERTION(sup, "loser!");
            nsCOMPtr<nsILocalFile> file = do_QueryInterface(sup);
            NS_ASSERTION(file, "loser!");
    
            char*   name;
            PRInt64 size;
            PRInt64 date;
            if(NS_FAILED(file->GetFileSize(&size)) ||
               NS_FAILED(file->GetLastModificationDate(&date)) ||
               NS_FAILED(file->GetLeafName(&name)))
            {
                NS_ASSERTION(0, "unexpected!");
                return FULL_VALIDATION_REQUIRED;
            }    

            for(k = 0; k < countOfFilesInWorkingSet; ++k)
            {
                xptiFile& target = aWorkingSet->GetFileAt(k);
                
                if(0 == PL_strcasecmp(name, target.GetName()))
                {
                    if(nsInt64(size) != target.GetSize() ||
                       nsInt64(date) != target.GetDate())
                        same = PR_FALSE;
                    break;        
                }
                // failed to find our file in the workingset?
                if(k == countOfFilesInWorkingSet)
                    same = PR_FALSE;
            }
            nsAllocator::Free(name);
        }
        if(same)
            return NO_FILES_CHANGED;
    }
    else if(countOfFilesInFileList > countOfFilesInWorkingSet)
    {
        // try to determine if the only changes are additional new files
        // XXX Wimping out and doing this as a separate walk through the lists.

        PRBool same = PR_TRUE;

        for(i = 0; i < countOfFilesInWorkingSet && same; ++i)
        {
            xptiFile& target = aWorkingSet->GetFileAt(i);
            
            for(k = 0; k < countOfFilesInFileList; ++k)
            {
                nsCOMPtr<nsISupports> sup;
                aFileList->GetElementAt(k, getter_AddRefs(sup));
                NS_ASSERTION(sup, "loser!");
                nsCOMPtr<nsILocalFile> file = do_QueryInterface(sup);
                NS_ASSERTION(file, "loser!");
                
                char*   name;
                PRInt64 size;
                PRInt64 date;
                if(NS_FAILED(file->GetFileSize(&size)) ||
                   NS_FAILED(file->GetLastModificationDate(&date)) ||
                   NS_FAILED(file->GetLeafName(&name)))
                {
                    NS_ASSERTION(0, "unexpected!");
                    return FULL_VALIDATION_REQUIRED;
                }    
            
                PRBool sameName = (0 == PL_strcasecmp(name, target.GetName()));
                nsAllocator::Free(name);
                if(sameName)
                {
                    if(nsInt64(size) != target.GetSize() ||
                       nsInt64(date) != target.GetDate())
                        same = PR_FALSE;
                    break;        
                }
            }
            // failed to find our file in the file list?
            if(k == countOfFilesInFileList)
                same = PR_FALSE;
        }
        if(same)
            return FILES_ADDED_ONLY;
    }

    return FULL_VALIDATION_REQUIRED; 
}

PRBool 
xptiInterfaceInfoManager::AddOnlyNewFileFromFileList(nsISupportsArray* aFileList,
                                                     xptiWorkingSet* aWorkingSet)
{
    nsILocalFile** orderedFileArray;
    PRUint32 countOfFilesInFileList;
    PRUint32 i;

    NS_ASSERTION(aFileList, "loser!");
    NS_ASSERTION(aWorkingSet, "loser!");
    NS_ASSERTION(aWorkingSet->IsValid(), "loser!");

    if(NS_FAILED(aFileList->Count(&countOfFilesInFileList)))
        return PR_FALSE;
    NS_ASSERTION(countOfFilesInFileList, "loser!");
    
    PRUint32 countOfFilesInWorkingSet = aWorkingSet->GetFileCount();
    NS_ASSERTION(countOfFilesInFileList > countOfFilesInWorkingSet,"loser!");

    orderedFileArray = BuildOrderedFileArray(aFileList, aWorkingSet);

    if(!orderedFileArray)
        return PR_FALSE;

    // Make enough space in aWorkingset for additions to xptiFile array.

    if(!aWorkingSet->ExtendFileArray(countOfFilesInFileList))   
        return PR_FALSE;

    // For each file that is not already in our working set, add any valid 
    // interfaces that don't conflict with previous interfaces added.
    for(i = 0; i < countOfFilesInFileList; i++)
    {
        nsILocalFile* file = orderedFileArray[i];

        char*   name;
        PRInt64 size;
        PRInt64 date;
        if(NS_FAILED(file->GetFileSize(&size)) ||
           NS_FAILED(file->GetLastModificationDate(&date)) ||
           NS_FAILED(file->GetLeafName(&name)))
        {
            return PR_FALSE;
        }    
    

        if(xptiWorkingSet::NOT_FOUND != aWorkingSet->FindFileWithName(name))
        {
            // This file was found in the working set, so skip it.       
            nsAllocator::Free(name);
            continue;
        }

        LOG_AUTOREG(("  finding interfaces in new file: %s\n", name));

        xptiFile fileRecord;
        fileRecord = xptiFile(nsInt64(size), nsInt64(date),
                              name, aWorkingSet);
        nsAllocator::Free(name);

        if(xptiFileType::IsXPT(fileRecord.GetName()))
        {
            XPTHeader* header = ReadXPTFile(file, aWorkingSet);
            if(!header)
            {
                // XXX do something!
                NS_ASSERTION(0,"");    
                continue;
            }

    
            xptiTypelib typelibRecord;
            typelibRecord.Init(aWorkingSet->GetFileCount());
    
            PRBool AddedFile = PR_FALSE;
            for(PRUint16 k = 0; k < header->num_interfaces; k++)
            {
                xptiInterfaceInfo* info = nsnull;
    
                if(!VerifyAndAddInterfaceIfNew(aWorkingSet,
                                               header->interface_directory + k,
                                               typelibRecord,
                                               &info))
                    return PR_FALSE;    
    
                if(!info)
                    continue;
                
                // If this is the first interface we found for this file then
                // setup the fileRecord for the header and infos.
                if(!AddedFile)
                {
                    if(!fileRecord.SetHeader(header))
                    {
                        // XXX that would be bad.
                        return PR_FALSE;    
                    }
                    AddedFile = PR_TRUE;
                }
                fileRecord.GetGuts()->SetInfoAt(k, info);
            }
            
            // This will correspond to typelibRecord above.
            aWorkingSet->AppendFile(fileRecord);
        }
        else // It is a zip file, Oh boy!
        {
            if(!xptiZipLoader::EnumerateZipEntries(file, 
                          NS_STATIC_CAST(xptiEntrySink*, this), 
                          aWorkingSet))
            {
                return PR_FALSE;    
            }
            // This will correspond to typelibRecord used in
            // xptiInterfaceInfoManager::FoundEntry.
            aWorkingSet->AppendFile(fileRecord);
        }
    }

    return PR_TRUE;
}        

PRBool 
xptiInterfaceInfoManager::DoFullValidationMergeFromFileList(nsISupportsArray* aFileList,
                                                          xptiWorkingSet* aWorkingSet)
{
    nsILocalFile** orderedFileArray;
    PRUint32 countOfFilesInFileList;
    PRUint32 i;

    NS_ASSERTION(aFileList, "loser!");

    if(!aWorkingSet->IsValid())
        return PR_FALSE;

    if(NS_FAILED(aFileList->Count(&countOfFilesInFileList)))
        return PR_FALSE;

    if(!countOfFilesInFileList)
    {
        // XXX do the right thing here!    
        return PR_FALSE;
    }

    orderedFileArray = BuildOrderedFileArray(aFileList, aWorkingSet);

    if(!orderedFileArray)
        return PR_FALSE;

    // DEBUG_DumpFileArray(orderedFileArray, countOfFilesInFileList);

    // Make space in aWorkingset for a new xptiFile array.

    if(!aWorkingSet->NewFileArray(countOfFilesInFileList))   
        return PR_FALSE;

    aWorkingSet->ClearZipItems();
    aWorkingSet->ClearHashTables();

    // For each file, add any valid interfaces that don't conflict with 
    // previous interfaces added.
    for(i = 0; i < countOfFilesInFileList; i++)
    {
        nsILocalFile* file = orderedFileArray[i];

        char*   name;
        PRInt64 size;
        PRInt64 date;
        if(NS_FAILED(file->GetFileSize(&size)) ||
           NS_FAILED(file->GetLastModificationDate(&date)) ||
           NS_FAILED(file->GetLeafName(&name)))
        {
            return PR_FALSE;
        }    

        LOG_AUTOREG(("  finding interfaces in file: %s\n", name));
    
        xptiFile fileRecord;
        fileRecord = xptiFile(nsInt64(size), nsInt64(date),
                              name, aWorkingSet);
        nsAllocator::Free(name);

//        printf("* found %s\n", fileRecord.GetName());


        if(xptiFileType::IsXPT(fileRecord.GetName()))
        {
            XPTHeader* header = ReadXPTFile(file, aWorkingSet);
            if(!header)
            {
                // XXX do something!
                NS_ASSERTION(0,"");    
                continue;
            }
    
            xptiTypelib typelibRecord;
            typelibRecord.Init(aWorkingSet->GetFileCount());
    
            PRBool AddedFile = PR_FALSE;
            for(PRUint16 k = 0; k < header->num_interfaces; k++)
            {
                xptiInterfaceInfo* info = nsnull;
    
                if(!VerifyAndAddInterfaceIfNew(aWorkingSet,
                                               header->interface_directory + k,
                                               typelibRecord,
                                               &info))
                    return PR_FALSE;    
    
                if(!info)
                    continue;
                
                // If this is the first interface we found for this file then
                // setup the fileRecord for the header and infos.
                if(!AddedFile)
                {
                    if(!fileRecord.SetHeader(header))
                    {
                        // XXX that would be bad.
                        return PR_FALSE;    
                    }
                    AddedFile = PR_TRUE;
                }
                fileRecord.GetGuts()->SetInfoAt(k, info);
            }
            
            // This will correspond to typelibRecord above.
            aWorkingSet->AppendFile(fileRecord);
        }
        else // It is a zip file, Oh boy!
        {
            if(!xptiZipLoader::EnumerateZipEntries(file, 
                          NS_STATIC_CAST(xptiEntrySink*, this), 
                          aWorkingSet))
            {
                return PR_FALSE;    
            }
            // This will correspond to typelibRecord used in
            // xptiInterfaceInfoManager::FoundEntry.
            aWorkingSet->AppendFile(fileRecord);
        }
    }
    return PR_TRUE;
}        

// implement xptiEntrySink
PRBool 
xptiInterfaceInfoManager::FoundEntry(const char* entryName,
                                     int index,
                                     XPTHeader* header,
                                     xptiWorkingSet* aWorkingSet)
{

    NS_ASSERTION(entryName, "loser!");
    NS_ASSERTION(header, "loser!");
    NS_ASSERTION(aWorkingSet, "loser!");

    int countOfInterfacesAddedForItem = 0;
    xptiZipItem zipItemRecord(entryName, aWorkingSet);
    
    LOG_AUTOREG(("    finding interfaces in file: %s\n", entryName));

    if(!header->num_interfaces)
    {
        // We are not interested in files without interfaces.
        return PR_TRUE;
    }
    
    xptiTypelib typelibRecord;
    typelibRecord.Init(aWorkingSet->GetFileCount(),
                       aWorkingSet->GetZipItemCount());

    for(PRUint16 k = 0; k < header->num_interfaces; k++)
    {
        xptiInterfaceInfo* info = nsnull;
    
        if(!VerifyAndAddInterfaceIfNew(aWorkingSet,
                                       header->interface_directory + k,
                                       typelibRecord,
                                       &info))
            return PR_FALSE;    
    
        if(!info)
            continue;

        // If this is the first interface we found for this item
        // then setup the zipItemRecord for the header and infos.
        if(!countOfInterfacesAddedForItem)
        {
            // XXX fix this!
            if(!zipItemRecord.SetHeader(header))
            {
                // XXX that would be bad.
                return PR_FALSE;    
            }
        }

        // zipItemRecord.GetGuts()->SetInfoAt(k, info);
        ++countOfInterfacesAddedForItem;
    }   

    if(countOfInterfacesAddedForItem)
    {
        if(!aWorkingSet->GetZipItemFreeSpace())
        {
            if(!aWorkingSet->ExtendZipItemArray(
                aWorkingSet->GetZipItemCount() + 20))
            {        
                // out of space!
                return PR_FALSE;    
            }
        }
        aWorkingSet->AppendZipItem(zipItemRecord);
    } 
    return PR_TRUE;
}

PRBool 
xptiInterfaceInfoManager::VerifyAndAddInterfaceIfNew(xptiWorkingSet* aWorkingSet,
                                                   XPTInterfaceDirectoryEntry* iface,
                                                   const xptiTypelib& typelibRecord,
                                                   xptiInterfaceInfo** infoAdded)
{
    NS_ASSERTION(iface, "loser!");
    NS_ASSERTION(infoAdded, "loser!");

    *infoAdded = nsnull;

    if(!iface->interface_descriptor)
    {
        // Not resolved, ignore this one.
        // XXX full logging might note this...
        return PR_TRUE;
    }
    
    xptiInterfaceInfo* info = (xptiInterfaceInfo*)
        PL_HashTableLookup(aWorkingSet->mIIDTable, &iface->iid);
    
    if(info)
    {
        // XXX validate this info to find possible inconsistencies
        LOG_AUTOREG(("      ignoring repeated interface: %s\n", iface->name));
        return PR_TRUE;
    }
    
    // Build a new xptiInterfaceInfo object and hook it up. 

    info = new xptiInterfaceInfo(iface->name, iface->iid,
                                 typelibRecord, aWorkingSet);
    if(!info)
    {
        // XXX bad!
        return PR_FALSE;    
    }

    NS_ADDREF(info);
    if(!info->IsValid())
    {
        // XXX bad!
        NS_RELEASE(info);
        return PR_FALSE;    
    }

    //XXX  We should SetHeader too as part of the validation, no?
    info->SetScriptableFlag(XPT_ID_IS_SCRIPTABLE(iface->interface_descriptor->flags));

    // The name table now owns the reference we AddRef'd above
    PL_HashTableAdd(aWorkingSet->mNameTable, iface->name, info);
    PL_HashTableAdd(aWorkingSet->mIIDTable, &iface->iid, info);
    
    *infoAdded = info;

    LOG_AUTOREG(("      added interface: %s\n", iface->name));

    return PR_TRUE;
}

PR_STATIC_CALLBACK(PRIntn)
xpti_Merger(PLHashEntry *he, PRIntn i, void *arg)
{
    xptiInterfaceInfo* srcInfo = (xptiInterfaceInfo*) he->value;
    xptiWorkingSet* aWorkingSet = (xptiWorkingSet*) arg;

    xptiInterfaceInfo* destInfo = (xptiInterfaceInfo*)
            PL_HashTableLookup(aWorkingSet->mIIDTable, srcInfo->GetTheIID());
    
    if(destInfo)
    {
        // XXX we could do some serious validation here!
    }
    else
    {
        // Clone the xptiInterfaceInfo into our WorkingSet.

        xptiTypelib typelibRecord;

        uint16 fileIndex = srcInfo->GetTypelibRecord().GetFileIndex();
        uint16 zipItemIndex = srcInfo->GetTypelibRecord().GetZipItemIndex();
        
        fileIndex += aWorkingSet->mFileMergeOffsetMap[fileIndex];
        
        // If it is not a zipItem, then the original index is fine.
        if(srcInfo->GetTypelibRecord().IsZip())
            zipItemIndex += aWorkingSet->mZipItemMergeOffsetMap[zipItemIndex];

        typelibRecord.Init(fileIndex, zipItemIndex);
                    
        destInfo = new xptiInterfaceInfo(*srcInfo, typelibRecord, aWorkingSet);

        if(!destInfo)
        {
            // XXX bad! should log
            return HT_ENUMERATE_NEXT;    
        }

        NS_ADDREF(destInfo);
        if(!destInfo->IsValid())
        {
            // XXX bad! should log
            NS_RELEASE(destInfo);
            return HT_ENUMERATE_NEXT;    
        }

        // The name table now owns the reference we AddRef'd above
        PL_HashTableAdd(aWorkingSet->mNameTable, destInfo->GetTheName(), destInfo);
        PL_HashTableAdd(aWorkingSet->mIIDTable, destInfo->GetTheIID(), destInfo);
    }

    return HT_ENUMERATE_NEXT;
}       


PRBool 
xptiInterfaceInfoManager::MergeWorkingSets(xptiWorkingSet* aDestWorkingSet,
                                           xptiWorkingSet* aSrcWorkingSet)
{

    PRUint32 i;

    // Combine file lists.

    PRUint32 originalFileCount = aDestWorkingSet->GetFileCount();
    PRUint32 additionalFileCount = aSrcWorkingSet->GetFileCount();

    // Create a new array big enough to hold both lists and copy existing files

    if(additionalFileCount)
    {
        if(!aDestWorkingSet->ExtendFileArray(originalFileCount +
                                             additionalFileCount))
            return PR_FALSE;
    
        // Now we are where we started, but we know we have enough space.
    
        // Prepare offset array for later fixups. 
        // NOTE: Storing with dest, but alloc'ing from src. This is intentional.
        aDestWorkingSet->mFileMergeOffsetMap = (PRUint32*)
            XPT_CALLOC(aSrcWorkingSet->GetStructArena(),
                       additionalFileCount * sizeof(PRUint32)); 
        if(!aDestWorkingSet->mFileMergeOffsetMap)
            return PR_FALSE;
    }

    for(i = 0; i < additionalFileCount; ++i)
    {
        xptiFile& srcFile = aSrcWorkingSet->GetFileAt(i);
        PRUint32 k;
        for(k = 0; k < originalFileCount; ++k)
        {
            // If file (with same name, date, and time) is in both lists
            // then reuse that record.
            xptiFile& destFile = aDestWorkingSet->GetFileAt(k);
            if(srcFile.Equals(destFile))
            {
                aDestWorkingSet->mFileMergeOffsetMap[i] = k - i;
                break;    
            }
        }
        if(k == originalFileCount)
        {
            // No match found, tack it on the end.

            PRUint32 newIndex = aDestWorkingSet->GetFileCount();

            aDestWorkingSet->AppendFile(
                    xptiFile(srcFile, aDestWorkingSet, PR_FALSE));

            // Fixup the merge offset map.
            aDestWorkingSet->mFileMergeOffsetMap[i] = newIndex - i;
        }
    }

    // Combine ZipItem lists.

    PRUint32 originalZipItemCount = aDestWorkingSet->GetZipItemCount();
    PRUint32 additionalZipItemCount = aSrcWorkingSet->GetZipItemCount();

    // Create a new array big enough to hold both lists and copy existing ZipItems

    if(additionalZipItemCount)
    {
        if(!aDestWorkingSet->ExtendZipItemArray(originalZipItemCount +
                                                additionalZipItemCount))
            return PR_FALSE;
    
        // Now we are where we started, but we know we have enough space.
    
        // Prepare offset array for later fixups. 
        // NOTE: Storing with dest, but alloc'ing from src. This is intentional.
        aDestWorkingSet->mZipItemMergeOffsetMap = (PRUint32*)
            XPT_CALLOC(aSrcWorkingSet->GetStructArena(),
                       additionalZipItemCount * sizeof(PRUint32)); 
        if(!aDestWorkingSet->mZipItemMergeOffsetMap)
            return PR_FALSE;
    }

    for(i = 0; i < additionalZipItemCount; ++i)
    {
        xptiZipItem& srcZipItem = aSrcWorkingSet->GetZipItemAt(i);
        PRUint32 k;
        for(k = 0; k < originalZipItemCount; ++k)
        {
            // If ZipItem (with same name) is in both lists
            // then reuse that record.
            xptiZipItem& destZipItem = aDestWorkingSet->GetZipItemAt(k);
            if(srcZipItem.Equals(destZipItem))
            {
                aDestWorkingSet->mZipItemMergeOffsetMap[i] = k - i;
                break;    
            }
        }
        if(k == originalZipItemCount)
        {
            // No match found, tack it on the end.

            PRUint32 newIndex = aDestWorkingSet->GetZipItemCount();

            aDestWorkingSet->AppendZipItem(
                    xptiZipItem(srcZipItem, aDestWorkingSet, PR_FALSE));

            // Fixup the merge offset map.
            aDestWorkingSet->mZipItemMergeOffsetMap[i] = newIndex - i;
        }
    }

    // Migrate xptiInterfaceInfos

    PL_HashTableEnumerateEntries(aSrcWorkingSet->mNameTable, xpti_Merger, 
                                 aDestWorkingSet);

    return PR_TRUE;
}        

PRBool 
xptiInterfaceInfoManager::DEBUG_DumpFileList(nsISupportsArray* aFileList)
{
    PRUint32 count;

    if(NS_FAILED(aFileList->Count(&count)))
        return PR_FALSE;
    
    for(PRUint32 i = 0; i < count; i++)
    {
        nsCOMPtr<nsISupports> sup;
        aFileList->GetElementAt(i, getter_AddRefs(sup));
        if(!sup)
            return PR_FALSE;
        nsCOMPtr<nsIFile> file = do_QueryInterface(sup);
        if(!file)
            return PR_FALSE;

        char* name;
        if(NS_FAILED(file->GetLeafName(&name)))
            return PR_FALSE;

        printf("* found %s\n", name);
        nsAllocator::Free(name);
    }
    return PR_TRUE;
}

PRBool 
xptiInterfaceInfoManager::DEBUG_DumpFileListInWorkingSet(xptiWorkingSet* aWorkingSet)
{
    for(PRUint16 i = 0; i < aWorkingSet->GetFileCount(); ++i)
    {
        xptiFile& record = aWorkingSet->GetFileAt(i);
    
        printf("! has %s\n", record.GetName());
    }        
    return PR_TRUE;
}        

PRBool 
xptiInterfaceInfoManager::DEBUG_DumpFileArray(nsILocalFile** aFileArray, 
                                              PRUint32 count)
{
    // dump the sorted list
    for(PRUint32 i = 0; i < count; ++i)
    {
        nsILocalFile* file = aFileArray[i];
    
        char* name;
        if(NS_FAILED(file->GetLeafName(&name)))
            return PR_FALSE;

        printf("found file: %s\n", name);
        nsAllocator::Free(name);
    }        
    return PR_TRUE;        
}        

/***************************************************************************/

// static 
void 
xptiInterfaceInfoManager::WriteToLog(const char *fmt, ...)
{
    if(!gInterfaceInfoManager)
        return;

    PRFileDesc* fd = gInterfaceInfoManager->GetOpenLogFile();
    if(fd)
    {
        va_list ap;
        va_start(ap, fmt);
        PR_vfprintf(fd, fmt, ap);
        va_end(ap);
    }
}        

PR_STATIC_CALLBACK(PRIntn)
xpti_ResolvedFileNameLogger(PLHashEntry *he, PRIntn i, void *arg)
{
    xptiInterfaceInfo* ii = (xptiInterfaceInfo*) he->value;
    xptiInterfaceInfoManager* mgr = (xptiInterfaceInfoManager*) arg;

    if(ii->IsFullyResolved())
    {
        xptiWorkingSet*  aWorkingSet = mgr->GetWorkingSet();
        PRFileDesc* fd = mgr->GetOpenLogFile();

        const xptiTypelib& typelib = ii->GetTypelibRecord();
        const char* filename = 
                aWorkingSet->GetFileAt(typelib.GetFileIndex()).GetName();
        
        if(typelib.IsZip())
        {
            const char* zipItemName = 
                aWorkingSet->GetZipItemAt(typelib.GetZipItemIndex()).GetName();
            PR_fprintf(fd, "xpti used interface: %s from %s::%s\n", 
                       ii->GetTheName(), filename, zipItemName);
        }    
        else
        {
            PR_fprintf(fd, "xpti used interface: %s from %s\n", 
                       ii->GetTheName(), filename);
        }
    }
    return HT_ENUMERATE_NEXT;
}

void   
xptiInterfaceInfoManager::LogStats()
{
    PRUint32 i;
    
    // This sets what will be returned by GetOpenLogFile().
    xptiAutoLog autoLog(this, mStatsLogFile, PR_FALSE);

    PRFileDesc* fd = GetOpenLogFile();
    if(!fd)
        return;

    // Show names of xpt (only) files from which at least one interface 
    // was resolved.

    PRUint32 fileCount = mWorkingSet.GetFileCount();
    for(i = 0; i < fileCount; ++i)
    {
        xptiFile& f = mWorkingSet.GetFileAt(i);
        if(f.GetGuts())
            PR_fprintf(fd, "xpti used file: %s\n", f.GetName());
    }

    PR_fprintf(fd, "\n");

    // Show names of xptfiles loaded from zips from which at least 
    // one interface was resolved.

    PRUint32 zipItemCount = mWorkingSet.GetZipItemCount();
    for(i = 0; i < zipItemCount; ++i)
    {
        xptiZipItem& zi = mWorkingSet.GetZipItemAt(i);
        if(zi.GetGuts())                           
            PR_fprintf(fd, "xpti used file from zip: %s\n", zi.GetName());
    }

    PR_fprintf(fd, "\n");

    // Show name of each interface that was fully resolved and the name
    // of the file and (perhaps) zip from which it was loaded.

    PL_HashTableEnumerateEntries(mWorkingSet.mNameTable, 
                                 xpti_ResolvedFileNameLogger, this);

} 

/***************************************************************************/

/* nsIInterfaceInfo getInfoForIID (in nsIIDPtr iid); */
NS_IMETHODIMP xptiInterfaceInfoManager::GetInfoForIID(const nsIID * iid, nsIInterfaceInfo **_retval)
{
    NS_ASSERTION(iid, "bad param");
    NS_ASSERTION(_retval, "bad param");

    xptiInterfaceInfo* info = (xptiInterfaceInfo*)
        PL_HashTableLookup(mWorkingSet.mIIDTable, iid);

    if(!info)
    {
        *_retval = nsnull;
        return NS_ERROR_FAILURE;    
    }

    NS_ADDREF(*_retval = NS_STATIC_CAST(nsIInterfaceInfo*, info));
    return NS_OK;    
}

/* nsIInterfaceInfo getInfoForName (in string name); */
NS_IMETHODIMP xptiInterfaceInfoManager::GetInfoForName(const char *name, nsIInterfaceInfo **_retval)
{
    NS_ASSERTION(name, "bad param");
    NS_ASSERTION(_retval, "bad param");

    xptiInterfaceInfo* info = (xptiInterfaceInfo*)
        PL_HashTableLookup(mWorkingSet.mNameTable, name);

    if(!info)
    {
        *_retval = nsnull;
        return NS_ERROR_FAILURE;    
    }

    NS_ADDREF(*_retval = NS_STATIC_CAST(nsIInterfaceInfo*, info));
    return NS_OK;    
}

/* nsIIDPtr getIIDForName (in string name); */
NS_IMETHODIMP xptiInterfaceInfoManager::GetIIDForName(const char *name, nsIID * *_retval)
{
    NS_ASSERTION(name, "bad param");
    NS_ASSERTION(_retval, "bad param");

    xptiInterfaceInfo* info = (xptiInterfaceInfo*)
        PL_HashTableLookup(mWorkingSet.mNameTable, name);

    if(!info)
    {
        *_retval = nsnull;
        return NS_ERROR_FAILURE;    
    }

    return info->GetIID(_retval);
}

/* string getNameForIID (in nsIIDPtr iid); */
NS_IMETHODIMP xptiInterfaceInfoManager::GetNameForIID(const nsIID * iid, char **_retval)
{
    NS_ASSERTION(iid, "bad param");
    NS_ASSERTION(_retval, "bad param");

    xptiInterfaceInfo* info = (xptiInterfaceInfo*)
        PL_HashTableLookup(mWorkingSet.mIIDTable, iid);

    if(!info)
    {
        *_retval = nsnull;
        return NS_ERROR_FAILURE;    
    }

    return info->GetName(_retval);
}

PR_STATIC_CALLBACK(PRIntn)
xpti_ArrayAppender(PLHashEntry *he, PRIntn i, void *arg)
{
    nsIInterfaceInfo* ii = (nsIInterfaceInfo*) he->value;
    nsISupportsArray* array = (nsISupportsArray*) arg;

    // XXX nsSupportsArray.h shows that this method returns the wrong value!
    array->AppendElement(ii);
    return HT_ENUMERATE_NEXT;
}


/* nsIEnumerator enumerateInterfaces (); */
NS_IMETHODIMP xptiInterfaceInfoManager::EnumerateInterfaces(nsIEnumerator **_retval)
{
    // I didn't want to incur the size overhead of using nsHashtable just to
    // make building an enumerator easier. So, this code makes a snapshot of 
    // the table using an nsISupportsArray and builds an enumerator for that.
    // We can afford this transient cost.

    nsCOMPtr<nsISupportsArray> array = 
        do_CreateInstance(NS_SUPPORTSARRAY_PROGID);
    if(!array)
        return NS_ERROR_UNEXPECTED;

    PL_HashTableEnumerateEntries(mWorkingSet.mNameTable, xpti_ArrayAppender, 
                                 array);
    
    return array->Enumerate(_retval);
}

/* void autoRegisterInterfaces (); */
NS_IMETHODIMP xptiInterfaceInfoManager::AutoRegisterInterfaces()
{
    nsCOMPtr<nsISupportsArray> fileList;
    xptiWorkingSet workingSet;
    AutoRegMode mode;
    PRBool ok;

    nsAutoLock lock(xptiInterfaceInfoManager::GetAutoRegLock());

    if(!workingSet.IsValid())
        return NS_ERROR_UNEXPECTED;

    // This sets what will be returned by GetOpenLogFile().
    xptiAutoLog autoLog(this, mAutoRegLogFile, PR_TRUE);

    LOG_AUTOREG(("start AutoRegister\n"));

    // We re-read the manifest rather than muck with the 'live' one.
    // It is OK if this fails.
    // XXX But we should track failure as a warning.
    ok = xptiManifest::Read(this, &workingSet);

    LOG_AUTOREG(("read of manifest %s\n", ok ? "successful" : "FAILED"));

    // Grovel for all the typelibs we can find (in .xpt or .zip, .jar,...).
    if(!BuildFileList(getter_AddRefs(fileList)) || !fileList)
        return NS_ERROR_UNEXPECTED;
    
    // DEBUG_DumpFileList(fileList);

    // Check to see how much work we need to do.
    mode = DetermineAutoRegStrategy(fileList, &workingSet);

    switch(mode)
    {
    case NO_FILES_CHANGED:
        LOG_AUTOREG(("autoreg strategy: no files changed\n"));
        LOG_AUTOREG(("successful end of AutoRegister\n"));
        return NS_OK;
    case FILES_ADDED_ONLY:
        LOG_AUTOREG(("autoreg strategy: files added only\n"));
        if(!AddOnlyNewFileFromFileList(fileList, &workingSet))
        {
            LOG_AUTOREG(("FAILED to add new files\n"));
            return NS_ERROR_UNEXPECTED;
        }
        break;
    case FULL_VALIDATION_REQUIRED:
        LOG_AUTOREG(("autoreg strategy: doing full validation merge\n"));
        if(!DoFullValidationMergeFromFileList(fileList, &workingSet))
        {
            LOG_AUTOREG(("FAILED to do full validation\n"));
            return NS_ERROR_UNEXPECTED;
        }
        break;
    default:
        NS_ASSERTION(0,"switch missing a case");
        return NS_ERROR_UNEXPECTED;
    }

    // XXX Is failure to write the file a good reason to not merge?
    if(!xptiManifest::Write(this, &workingSet))
    {
        LOG_AUTOREG(("FAILED to write manifest\n"));
        return NS_ERROR_UNEXPECTED;
    }
    
    if(!MergeWorkingSets(&mWorkingSet, &workingSet))
    {
        LOG_AUTOREG(("FAILED to merge into live workingset\n"));
        return NS_ERROR_UNEXPECTED;
    }

//    DEBUG_DumpFileListInWorkingSet(mWorkingSet);

    LOG_AUTOREG(("successful end of AutoRegister\n"));

    return NS_OK;
}

/***************************************************************************/

XPTI_PUBLIC_API(nsIInterfaceInfoManager*)
XPTI_GetInterfaceInfoManager()
{
    nsIInterfaceInfoManager* iim =
        xptiInterfaceInfoManager::GetInterfaceInfoManagerNoAddRef();
    NS_IF_ADDREF(iim);
    return iim;
}

XPTI_PUBLIC_API(void)
XPTI_FreeInterfaceInfoManager()
{
    xptiInterfaceInfoManager::FreeInterfaceInfoManager();
}

