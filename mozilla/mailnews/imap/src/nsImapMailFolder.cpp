/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998, 1999 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

#include "msgCore.h"
#include "nsMsgImapCID.h"
#include "nsIMessage.h"
#include "nsImapMailFolder.h"
#include "nsIEnumerator.h"
#include "nsIFolderListener.h"
#include "nsCOMPtr.h"
#include "nsIRDFService.h"
#include "nsIRDFDataSource.h"
#include "nsRDFCID.h"
#include "nsFileStream.h"
#include "nsMsgDBCID.h"
#include "nsMsgFolderFlags.h"
#include "nsLocalFolderSummarySpec.h"
#include "nsImapFlagAndUidState.h"
#include "nsIEventQueueService.h"
#include "nsIImapUrl.h"
#include "nsImapUtils.h"
#include "nsMsgUtils.h"
#include "nsIMsgMailSession.h"
#include "nsImapMessage.h"
#include "nsIWebShell.h"
#include "nsMsgBaseCID.h"
#include "nsMsgLocalCID.h"
#include "nsImapUndoTxn.h"
#include "nsIIMAPHostSessionList.h"
#include "nsIMsgCopyService.h"
#include "nsICopyMsgStreamListener.h"
#include "nsImapStringBundle.h"

#include "nsIMsgStatusFeedback.h"

#include "nsIMsgFilter.h"
#include "nsIMsgFilterService.h"
#include "nsImapMoveCoalescer.h"
#include "nsIPrompt.h"
#include "nsINetSupportDialogService.h"
#include "nsSpecialSystemDirectory.h"

static NS_DEFINE_CID(kNetSupportDialogCID, NS_NETSUPPORTDIALOG_CID);
static NS_DEFINE_CID(kMsgFilterServiceCID, NS_MSGFILTERSERVICE_CID);
static NS_DEFINE_CID(kRDFServiceCID, NS_RDFSERVICE_CID);
static NS_DEFINE_CID(kCMailDB, NS_MAILDB_CID);
static NS_DEFINE_CID(kImapProtocolCID, NS_IMAPPROTOCOL_CID);
static NS_DEFINE_CID(kCImapDB, NS_IMAPDB_CID);
static NS_DEFINE_CID(kCImapService, NS_IMAPSERVICE_CID);
static NS_DEFINE_CID(kEventQueueServiceCID, NS_EVENTQUEUESERVICE_CID);
static NS_DEFINE_CID(kMsgMailSessionCID, NS_MSGMAILSESSION_CID);
static NS_DEFINE_CID(kParseMailMsgStateCID, NS_PARSEMAILMSGSTATE_CID);
static NS_DEFINE_CID(kCImapHostSessionList, NS_IIMAPHOSTSESSIONLIST_CID);
static NS_DEFINE_CID(kMsgCopyServiceCID,		NS_MSGCOPYSERVICE_CID);
static NS_DEFINE_CID(kCopyMessageStreamListenerCID, NS_COPYMESSAGESTREAMLISTENER_CID);

#define FOUR_K 4096

nsImapMailFolder::nsImapMailFolder() :
    m_initialized(PR_FALSE),m_haveDiscoveredAllFolders(PR_FALSE),
    m_haveReadNameFromDB(PR_FALSE), 
    m_curMsgUid(0), m_nextMessageByteLength(0),
    m_urlRunning(PR_FALSE),
	m_verifiedAsOnlineFolder(PR_FALSE),
	m_explicitlyVerify(PR_FALSE) 
{
	m_pathName = nsnull;
    m_appendMsgMonitor = nsnull;	// since we're not using this (yet?) make it null.
								// if we do start using it, it should be created lazily

	nsresult rv;

    // Get current thread envent queue

	NS_WITH_SERVICE(nsIEventQueueService, pEventQService, kEventQueueServiceCID, &rv); 
    if (NS_SUCCEEDED(rv) && pEventQService)
        pEventQService->GetThreadEventQueue(PR_GetCurrentThread(),
                                            getter_AddRefs(m_eventQueue));
	m_moveCoalescer = nsnull;

}

nsImapMailFolder::~nsImapMailFolder()
{
	if (m_pathName)
		delete m_pathName;
    if (m_appendMsgMonitor)
        PR_DestroyMonitor(m_appendMsgMonitor);

	if (m_moveCoalescer)
		delete m_moveCoalescer;
}

NS_IMPL_ADDREF_INHERITED(nsImapMailFolder, nsMsgDBFolder)
NS_IMPL_RELEASE_INHERITED(nsImapMailFolder, nsMsgDBFolder)

NS_IMETHODIMP nsImapMailFolder::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
	if (!aInstancePtr) return NS_ERROR_NULL_POINTER;
	*aInstancePtr = nsnull;

    if (aIID.Equals(nsIMsgImapMailFolder::GetIID()))
	{
		*aInstancePtr = NS_STATIC_CAST(nsIMsgImapMailFolder*, this);
	}              
	else if(aIID.Equals(nsICopyMessageListener::GetIID()))
	{
		*aInstancePtr = NS_STATIC_CAST(nsICopyMessageListener*, this);
	}
	else if (aIID.Equals(nsIImapMailFolderSink::GetIID()))
	{
		*aInstancePtr = NS_STATIC_CAST(nsIImapMailFolderSink*, this);
	}
	else if (aIID.Equals(nsIImapMessageSink::GetIID()))
	{
		*aInstancePtr = NS_STATIC_CAST(nsIImapMessageSink*, this);
	}
	else if (aIID.Equals(nsIImapExtensionSink::GetIID()))
	{
		*aInstancePtr = NS_STATIC_CAST(nsIImapExtensionSink*, this);
	}
	else if (aIID.Equals(nsIImapMiscellaneousSink::GetIID()))
	{
		*aInstancePtr = NS_STATIC_CAST(nsIImapMiscellaneousSink*, this);
	}
	else if (aIID.Equals(nsIUrlListener::GetIID()))
	{
		*aInstancePtr = NS_STATIC_CAST(nsIUrlListener *, this);
	}
	else if (aIID.Equals(nsIMsgFilterHitNotify::GetIID()))
	{
		*aInstancePtr = NS_STATIC_CAST(nsIMsgFilterHitNotify *, this);
	}
	if(*aInstancePtr)
	{
		AddRef();
		return NS_OK;
	}

	return nsMsgFolder::QueryInterface(aIID, aInstancePtr);
}

NS_IMETHODIMP nsImapMailFolder::GetPath(nsIFileSpec** aPathName)
{
	nsresult rv;
    if (! m_pathName) 
    {
    	m_pathName = new nsNativeFileSpec("");
    	if (! m_pathName)
    		return NS_ERROR_OUT_OF_MEMORY;
   
        rv = nsImapURI2Path(kImapRootURI, mURI, *m_pathName);
//		printf("constructing path %s\n", (const char *) *m_pathName);
        if (NS_FAILED(rv)) return rv;
    }
	rv = NS_NewFileSpecWithSpec(*m_pathName, aPathName);

	return NS_OK;
}

NS_IMETHODIMP nsImapMailFolder::Enumerate(nsIEnumerator* *result)
{
#if 0
    nsresult rv = NS_OK;
    nsIEnumerator* folders;
    nsIEnumerator* messages;
    rv = GetSubFolders(&folders);
    if (NS_FAILED(rv)) return rv;
    rv = GetMessages(&messages);
    if (NS_FAILED(rv)) return rv;
    return NS_NewConjoiningEnumerator(folders, messages, 
                                      (nsIBidirectionalEnumerator**)result);
#endif
	NS_ASSERTION(PR_FALSE, "obsolete, right?");
	return NS_ERROR_FAILURE;
}

nsresult nsImapMailFolder::AddDirectorySeparator(nsFileSpec &path)
{
	nsresult rv = NS_OK;
	if (nsCRT::strcmp(mURI, kImapRootURI) == 0) 
	{
      // don't concat the full separator with .sbd
    }
    else 
	{
      nsAutoString sep;
      rv = nsGetMailFolderSeparator(sep);
      if (NS_FAILED(rv)) return rv;

      // see if there's a dir with the same name ending with .sbd
      // unfortunately we can't just say:
      //          path += sep;
      // here because of the way nsFileSpec concatenates
      nsAutoString str((nsFilePath)path);
      str += sep;
      path = nsFilePath(str);
    }

	return rv;
}

static PRBool
nsShouldIgnoreFile(nsString& name)
{
    PRInt32 len = name.Length();
    if (len > 4 && name.RFind(".msf", PR_TRUE) == len -4)
    {
        name.SetLength(len-4); // truncate the string
        return PR_FALSE;
    }
    return PR_TRUE;
}

nsresult nsImapMailFolder::AddSubfolder(nsAutoString name, 
                                        nsIMsgFolder **child)
{
	if(!child)
		return NS_ERROR_NULL_POINTER;

	nsresult rv = NS_OK;
	NS_WITH_SERVICE(nsIRDFService, rdf, kRDFServiceCID, &rv); 

	if(NS_FAILED(rv))
		return rv;

	nsAutoString uri;
	uri.Append(mURI);
	uri.Append('/');

	uri.Append(name);
	char* uriStr = uri.ToNewCString();
	if (uriStr == nsnull) 
		return NS_ERROR_OUT_OF_MEMORY;

	nsCOMPtr<nsIRDFResource> res;
	rv = rdf->GetResource(uriStr, getter_AddRefs(res));
	if (NS_FAILED(rv))
		return rv;

	nsCOMPtr<nsIMsgFolder> folder(do_QueryInterface(res, &rv));
	if (NS_FAILED(rv))
		return rv;        

	nsAllocator::Free(uriStr);
	folder->SetFlag(MSG_FOLDER_FLAG_MAIL);

	PRBool isServer;
    rv = GetIsServer(&isServer);

	//Only set these is these are top level children.
	if(NS_SUCCEEDED(rv) && isServer)
	{

		if(name.Compare("Inbox", PR_TRUE) == 0)
			folder->SetFlag(MSG_FOLDER_FLAG_INBOX);
		else if(name.Compare("Trash", PR_TRUE) == 0)
			folder->SetFlag(MSG_FOLDER_FLAG_TRASH);
		else if(name.Compare("Sent", PR_TRUE) == 0)
			folder->SetFlag(MSG_FOLDER_FLAG_SENTMAIL);
		else if(name.Compare("Drafts", PR_TRUE) == 0)
			folder->SetFlag(MSG_FOLDER_FLAG_DRAFTS);
		else if (name.Compare("Templates", PR_TRUE) == 0)
			folder->SetFlag(MSG_FOLDER_FLAG_TEMPLATES);
	}
	//at this point we must be ok and we don't want to return failure in case GetIsServer failed.
	rv = NS_OK;

	nsCOMPtr <nsISupports> supports = do_QueryInterface(folder);
	NS_ASSERTION(supports, "couldn't get isupports from imap folder");
	if (supports)
		mSubFolders->AppendElement(supports);
    folder->SetParent(this);
	*child = folder;
	NS_IF_ADDREF(*child);
	return rv;
}

nsresult nsImapMailFolder::CreateSubFolders(nsFileSpec &path)
{
	nsresult rv = NS_OK;
	nsAutoString currentFolderNameStr;
	nsCOMPtr<nsIMsgFolder> child;
	char *folderName;
	for (nsDirectoryIterator dir(path, PR_FALSE); dir.Exists(); dir++) 
	{
		nsFileSpec currentFolderPath = (nsFileSpec&)dir;

		folderName = currentFolderPath.GetLeafName();
		currentFolderNameStr = folderName;
		if (nsShouldIgnoreFile(currentFolderNameStr))
		{
			PL_strfree(folderName);
			continue;
		}

		AddSubfolder(currentFolderNameStr, getter_AddRefs(child));
		PL_strfree(folderName);
    }
	return rv;
}

NS_IMETHODIMP nsImapMailFolder::GetSubFolders(nsIEnumerator* *result)
{
    if (!m_initialized)
    {
        nsresult rv = NS_OK;
		nsCOMPtr<nsIFileSpec> pathSpec;
		rv = GetPath(getter_AddRefs(pathSpec));
		if (NS_FAILED(rv)) return rv;

		nsFileSpec path;
		rv = pathSpec->GetFileSpec(&path);
		if (NS_FAILED(rv)) return rv;

		// host directory does not need .sbd tacked on
        PRBool isServer;
        rv = GetIsServer(&isServer);
		if (NS_SUCCEEDED(rv) && !isServer)
			rv = AddDirectorySeparator(path);

        if(NS_FAILED(rv)) return rv;
        
        // we have to treat the root folder specially, because it's name
        // doesn't end with .sbd

        PRInt32 newFlags = MSG_FOLDER_FLAG_MAIL;
        if (path.IsDirectory()) {
            newFlags |= (MSG_FOLDER_FLAG_DIRECTORY | MSG_FOLDER_FLAG_ELIDED);
            SetFlag(newFlags);
            rv = CreateSubFolders(path);
        }
		PRUint32 count;
		if (isServer && NS_SUCCEEDED(mSubFolders->Count(&count)) && count == 0)
		{
			// create an inbox...
			CreateClientSubfolderInfo("INBOX");
		}
        UpdateSummaryTotals(PR_FALSE);

        if (NS_FAILED(rv)) return rv;
        m_initialized = PR_TRUE;      // XXX do this on failure too?
    }
    return mSubFolders->Enumerate(result);
}

NS_IMETHODIMP nsImapMailFolder::AddUnique(nsISupports* element)
{
    nsresult rv = NS_ERROR_FAILURE;
    return rv;
}

NS_IMETHODIMP nsImapMailFolder::ReplaceElement(nsISupports* element,
                                               nsISupports* newElement)
{
    nsresult rv = NS_ERROR_FAILURE;
    return rv;
}

NS_IMETHODIMP
nsImapMailFolder::GetMsgDatabase(nsIMsgDatabase** aMsgDatabase)
{
    GetDatabase();
    return nsMsgDBFolder::GetMsgDatabase(aMsgDatabase);
}

//Makes sure the database is open and exists.  If the database is valid then
//returns NS_OK.  Otherwise returns a failure error value.
nsresult nsImapMailFolder::GetDatabase()
{
	nsresult folderOpen = NS_OK;
	if (!mDatabase)
	{
		nsCOMPtr<nsIFileSpec> pathSpec;
		nsresult rv = GetPath(getter_AddRefs(pathSpec));
		if (NS_FAILED(rv)) return rv;

		nsCOMPtr<nsIMsgDatabase> mailDBFactory;

		rv = nsComponentManager::CreateInstance(kCImapDB, nsnull, nsIMsgDatabase::GetIID(), (void **) getter_AddRefs(mailDBFactory));
		if (NS_SUCCEEDED(rv) && mailDBFactory)
			folderOpen = mailDBFactory->Open(pathSpec, PR_TRUE, PR_TRUE, getter_AddRefs(mDatabase));

		if(mDatabase)
		{
			if(mAddListener)
				mDatabase->AddListener(this);

			// if we have to regenerate the folder, run the parser url.
			if(folderOpen == NS_MSG_ERROR_FOLDER_SUMMARY_MISSING || folderOpen == NS_MSG_ERROR_FOLDER_SUMMARY_OUT_OF_DATE)
			{
			}
			else
			{
				//Otherwise we have a valid database so lets extract necessary info.
				UpdateSummaryTotals(PR_TRUE);
			}
		}
		else
			folderOpen = rv;
	}
	return folderOpen;
}

NS_IMETHODIMP
nsImapMailFolder::UpdateFolder()
{
    nsresult rv = NS_ERROR_NULL_POINTER;
	PRBool selectFolder = PR_FALSE;

	NS_WITH_SERVICE(nsIImapService, imapService, kCImapService, &rv); 

	if (NS_FAILED(rv)) return rv;

	selectFolder = PR_TRUE;

    PRBool isServer;
    rv = GetIsServer(&isServer);
    if (NS_SUCCEEDED(rv) && isServer)
    {
        if (!m_haveDiscoveredAllFolders)
        {
            PRBool hasSubFolders = PR_FALSE;
            GetHasSubFolders(&hasSubFolders);
            if (!hasSubFolders)
            {
                rv = CreateClientSubfolderInfo("Inbox");
                if (NS_FAILED(rv)) 
                    return rv;
            }
            m_haveDiscoveredAllFolders = PR_TRUE;
        }
        selectFolder = PR_FALSE;
    }
    rv = GetDatabase();

	// don't run select if we're already running a url/select...
	if (NS_SUCCEEDED(rv) && !m_urlRunning && selectFolder)
	{
		nsCOMPtr <nsIEventQueue> eventQ;
		NS_WITH_SERVICE(nsIEventQueueService, pEventQService, kEventQueueServiceCID, &rv); 
		if (NS_SUCCEEDED(rv) && pEventQService)
			pEventQService->GetThreadEventQueue(PR_GetCurrentThread(),
												getter_AddRefs(eventQ));
		rv = imapService->SelectFolder(eventQ, this, this, nsnull, nsnull);
		m_urlRunning = PR_TRUE;
	}
	return rv;
}


NS_IMETHODIMP nsImapMailFolder::GetMessages(nsISimpleEnumerator* *result)
{
	if (result)
		*result = nsnull;
	nsresult rv;
  nsCOMPtr<nsISimpleEnumerator> msgHdrEnumerator;
  nsMessageFromMsgHdrEnumerator *messageEnumerator = nsnull;
  rv = NS_ERROR_UNEXPECTED;
  if (mDatabase)
      rv = mDatabase->EnumerateMessages(getter_AddRefs(msgHdrEnumerator));
  if(NS_SUCCEEDED(rv))
      rv = NS_NewMessageFromMsgHdrEnumerator(msgHdrEnumerator,
                                             this,
                                             &messageEnumerator);
  *result = messageEnumerator;
  return rv;
}

NS_IMETHODIMP nsImapMailFolder::CreateSubfolder(const char* folderName)
{
    nsresult rv = NS_ERROR_NULL_POINTER;
    if (!folderName) return rv;
    NS_WITH_SERVICE(nsIImapService, imapService, kCImapService, &rv);
    if (NS_SUCCEEDED(rv))
        rv = imapService->CreateFolder(m_eventQueue, this, 
                                       folderName, this, nsnull);
    return rv;
}

NS_IMETHODIMP nsImapMailFolder::CreateClientSubfolderInfo(const char *folderName)
{
	nsresult rv = NS_OK;
    
	//Get a directory based on our current path.
	nsCOMPtr<nsIFileSpec> pathSpec;
	rv = GetPath(getter_AddRefs(pathSpec));
	if (NS_FAILED(rv)) return rv;

	nsFileSpec path;
	rv = pathSpec->GetFileSpec(&path);
	if (NS_FAILED(rv)) return rv;

//	if (!path.Exists())
//	{
//		path.CreateDir();
//	}

	rv = CreateDirectoryForFolder(path);
	if(NS_FAILED(rv))
		return rv;

    nsAutoString leafName (folderName);
    nsAutoString folderNameStr;
    nsAutoString parentName = leafName;
    PRInt32 folderStart = leafName.FindChar('/');
    if (folderStart > 0)
    {
        NS_WITH_SERVICE(nsIRDFService, rdf, kRDFServiceCID, &rv);
        nsCOMPtr<nsIRDFResource> res;
        nsCOMPtr<nsIMsgImapMailFolder> parentFolder;
        nsCAutoString uri (mURI);
        parentName.Right(leafName, leafName.Length() - folderStart - 1);
        parentName.Truncate(folderStart);
        path += parentName;
        rv = CreateDirectoryForFolder(path);
        if (NS_FAILED(rv)) return rv;
        uri.Append('/');
        uri.Append(parentName);
        rv = rdf->GetResource(uri,
                              getter_AddRefs(res));
        if (NS_FAILED(rv)) return rv;
        parentFolder = do_QueryInterface(res, &rv);
        if (NS_FAILED(rv)) return rv;
	    return parentFolder->CreateClientSubfolderInfo(nsCAutoString(leafName));
    }
    
    folderNameStr = leafName;
    
    path += folderNameStr;

	// Create an empty database for this mail folder, set its name from the user  
	nsCOMPtr<nsIMsgDatabase> mailDBFactory;
    nsCOMPtr<nsIMsgFolder> child;

	rv = nsComponentManager::CreateInstance(kCMailDB, nsnull, nsIMsgDatabase::GetIID(), (void **) getter_AddRefs(mailDBFactory));
	if (NS_SUCCEEDED(rv) && mailDBFactory)
	{
        nsCOMPtr<nsIMsgDatabase> unusedDB;
		nsCOMPtr <nsIFileSpec> dbFileSpec;
		NS_NewFileSpecWithSpec(path, getter_AddRefs(dbFileSpec));
		rv = mailDBFactory->Open(dbFileSpec, PR_TRUE, PR_TRUE, (nsIMsgDatabase **) getter_AddRefs(unusedDB));

        if (NS_SUCCEEDED(rv) && unusedDB)
        {
			//need to set the folder name
			nsCOMPtr <nsIDBFolderInfo> folderInfo;
			rv = unusedDB->GetDBFolderInfo(getter_AddRefs(folderInfo));
			if(NS_SUCCEEDED(rv))
			{
				// ### DMB used to be leafNameFromUser?
				folderInfo->SetMailboxName(&folderNameStr);
			}

			//Now let's create the actual new folder
			rv = AddSubfolder(folderNameStr, getter_AddRefs(child));

			nsCOMPtr <nsIMsgImapMailFolder> imapFolder = do_QueryInterface(child);
			if (imapFolder)
				imapFolder->SetVerifiedAsOnlineFolder(PR_TRUE);

            unusedDB->SetSummaryValid(PR_TRUE);
			unusedDB->Commit(nsMsgDBCommitType::kLargeCommit);
            unusedDB->Close(PR_TRUE);
        }
	}
	if(NS_SUCCEEDED(rv) && child)
	{
		nsCOMPtr<nsISupports> folderSupports = do_QueryInterface(child, &rv);
		if(NS_SUCCEEDED(rv))
			NotifyItemAdded(folderSupports);
	}

	return rv;
}
    
NS_IMETHODIMP nsImapMailFolder::RemoveSubFolder (nsIMsgFolder *which)
{
    nsCOMPtr<nsISupportsArray> folders;
    nsresult rv = NS_NewISupportsArray(getter_AddRefs(folders));
    if (NS_FAILED(rv)) return rv;
    nsCOMPtr<nsISupports> folderSupport = do_QueryInterface(which, &rv);
    if (NS_FAILED(rv)) return rv;
    folders->AppendElement(folderSupport);
    which->Delete();
    return nsMsgFolder::DeleteSubFolders(folders);
}


NS_IMETHODIMP nsImapMailFolder::GetVerifiedAsOnlineFolder(PRBool *aVerifiedAsOnlineFolder)
{

	if (!aVerifiedAsOnlineFolder)
		return NS_ERROR_NULL_POINTER;

	*aVerifiedAsOnlineFolder = m_verifiedAsOnlineFolder;
	return NS_OK;
}

NS_IMETHODIMP nsImapMailFolder::SetVerifiedAsOnlineFolder(PRBool aVerifiedAsOnlineFolder)
{
	m_verifiedAsOnlineFolder = aVerifiedAsOnlineFolder;
	return NS_OK;
}

NS_IMETHODIMP nsImapMailFolder::GetExplicitlyVerify(PRBool *aExplicitlyVerify)
{
	if (!aExplicitlyVerify)
		return NS_ERROR_NULL_POINTER;
	*aExplicitlyVerify = m_explicitlyVerify;
	return NS_OK;
}

NS_IMETHODIMP nsImapMailFolder::SetExplicitlyVerify(PRBool aExplicitlyVerify)
{
	m_explicitlyVerify = aExplicitlyVerify;
	return NS_OK;
}


NS_IMETHODIMP nsImapMailFolder::Compact()
{
    nsresult rv;
    NS_WITH_SERVICE(nsIImapService, imapService, kCImapService, &rv);
    if (NS_SUCCEEDED(rv) && imapService)
    {
        rv = imapService->Expunge(m_eventQueue, this, nsnull, nsnull);
    }
    return rv;
}

NS_IMETHODIMP nsImapMailFolder::EmptyTrash()
{
    nsresult rv;
    nsCOMPtr<nsIMsgFolder> trashFolder;
    rv = GetTrashFolder(getter_AddRefs(trashFolder));
    if (NS_SUCCEEDED(rv))
    {
        nsCOMPtr<nsIMsgDatabase> trashDB;

        rv = trashFolder->Delete(); // delete summary spec
        rv = trashFolder->GetMsgDatabase(getter_AddRefs(trashDB));

        nsCOMPtr<nsIUrlListener> urlListener =
            do_QueryInterface(trashFolder);

        NS_WITH_SERVICE (nsIImapService, imapService, kCImapService, &rv);
        if (NS_SUCCEEDED(rv))
            rv = imapService->DeleteAllMessages(m_eventQueue, trashFolder,
                                                urlListener, nsnull);
    }
    return rv;
}

NS_IMETHODIMP nsImapMailFolder::Delete ()
{
    nsresult rv = NS_ERROR_FAILURE;
    if (mDatabase)
    {
        mDatabase->ForceClosed();
        mDatabase = null_nsCOMPtr();
    }

    nsCOMPtr<nsIFileSpec> pathSpec;
    rv = GetPath(getter_AddRefs(pathSpec));
    if (NS_SUCCEEDED(rv))
    {
        nsFileSpec fileSpec;
        rv = pathSpec->GetFileSpec(&fileSpec);
        if (NS_SUCCEEDED(rv))
        {
            nsLocalFolderSummarySpec summarySpec(fileSpec);
            if (summarySpec.Exists())
                summarySpec.Delete(PR_FALSE);
        }
    }
    return rv;
}

NS_IMETHODIMP nsImapMailFolder::Rename (const char *newName)
{
    nsresult rv = NS_ERROR_FAILURE;
    NS_WITH_SERVICE (nsIImapService, imapService, kCImapService, &rv);
    if (NS_SUCCEEDED(rv))
        rv = imapService->RenameLeaf(m_eventQueue, this, newName, this, nsnull);
    return rv;
}

NS_IMETHODIMP nsImapMailFolder::GetPrettyName(PRUnichar ** prettyName)
{
	return GetName(prettyName);
}
    
NS_IMETHODIMP nsImapMailFolder::BuildFolderURL(char **url)
{
    nsresult rv = NS_ERROR_FAILURE;
    return rv;
}
    
NS_IMETHODIMP nsImapMailFolder::UpdateSummaryTotals(PRBool force) 
{
	// could we move this into nsMsgDBFolder, or do we need to deal
	// with the pending imap counts?
	nsresult rv = NS_OK;

	PRInt32 oldUnreadMessages = mNumUnreadMessages;
	PRInt32 oldTotalMessages = mNumTotalMessages;
	//We need to read this info from the database
	ReadDBFolderInfo(force);

	// If we asked, but didn't get any, stop asking
	if (mNumUnreadMessages == -1)
		mNumUnreadMessages = -2;

	//Need to notify listeners that total count changed.
	if(oldTotalMessages != mNumTotalMessages)
	{
		char *oldTotalMessagesStr = PR_smprintf("%d", oldTotalMessages);
		char *totalMessagesStr = PR_smprintf("%d",mNumTotalMessages);
		NotifyPropertyChanged("TotalMessages", oldTotalMessagesStr, totalMessagesStr);
		PR_smprintf_free(totalMessagesStr);
		PR_smprintf_free(oldTotalMessagesStr);
	}

	if(oldUnreadMessages != mNumUnreadMessages)
	{
		char *oldUnreadMessagesStr = PR_smprintf("%d", oldUnreadMessages);
		char *totalUnreadMessages = PR_smprintf("%d",mNumUnreadMessages);
		NotifyPropertyChanged("TotalUnreadMessages", oldUnreadMessagesStr, totalUnreadMessages);
		PR_smprintf_free(totalUnreadMessages);
		PR_smprintf_free(oldUnreadMessagesStr);
	}

    return rv;
}
    
NS_IMETHODIMP nsImapMailFolder::GetExpungedBytesCount(PRUint32 *count)
{
    nsresult rv = NS_ERROR_FAILURE;
    return rv;
}

NS_IMETHODIMP nsImapMailFolder::GetDeletable (PRBool *deletable)
{
    nsresult rv = NS_ERROR_FAILURE;
    return rv;
}

NS_IMETHODIMP nsImapMailFolder::GetCanCreateChildren (PRBool
                                                      *canCreateChildren) 
{
    nsresult rv = NS_ERROR_FAILURE;
    return rv;
}

NS_IMETHODIMP nsImapMailFolder::GetCanBeRenamed (PRBool *canBeRenamed)
{
    nsresult rv = NS_ERROR_FAILURE;
    return rv;
}

NS_IMETHODIMP nsImapMailFolder::GetRequiresCleanup(PRBool *requiresCleanup)
{
    nsresult rv = NS_ERROR_FAILURE;
    return rv;
}

    
NS_IMETHODIMP nsImapMailFolder::GetSizeOnDisk(PRUint32 * size)
{
    nsresult rv = NS_ERROR_FAILURE;
    return rv;
}

nsresult nsImapMailFolder::GetServerKey(char **serverKey)
{
	// look for matching imap folders, then pop folders
	nsCOMPtr<nsIMsgIncomingServer> server;
	nsresult rv = GetServer(getter_AddRefs(server));
	if (NS_SUCCEEDED(rv) && server)
		return server->GetKey(serverKey);
	return rv;
}

NS_IMETHODIMP nsImapMailFolder::GetUsername(char** userName)
{
    nsresult rv = NS_ERROR_NULL_POINTER;
    NS_PRECONDITION (userName, "Oops ... null userName pointer");
    if (!userName)
        return rv;

    *userName = nsnull;
    
    char *uri = nsnull;
    rv = GetURI(&uri);
    if (NS_FAILED(rv)) return rv;
    
    nsAutoString aName = uri;
    PR_FREEIF(uri);
    if (aName.Find(kImapRootURI) != 0)
        return NS_ERROR_FAILURE;
    
    aName.Cut(0, PL_strlen(kImapRootURI));
    while (aName[0] == '/')
        aName.Cut(0, 1);
    PRInt32 userEnd = aName.FindChar('@');
    if (userEnd < 1)
        return NS_ERROR_NULL_POINTER;

    aName.SetLength(userEnd);
    char *tmpCString = aName.ToNewCString();
    if (tmpCString && *tmpCString)
    {
        *userName = PL_strdup(tmpCString);
        rv = NS_OK;
		nsAllocator::Free(tmpCString);
    }
    return rv;
}

NS_IMETHODIMP nsImapMailFolder::GetHostname(char** hostName)
{
    nsresult rv = NS_ERROR_NULL_POINTER;

    NS_PRECONDITION (hostName, "Oops ... null hostName pointer");
    if (!hostName)
        return rv;
    else
        *hostName = nsnull;

    nsCOMPtr<nsIFolder> aFolder = do_QueryInterface((nsIMsgFolder *) this, &rv);
    if (NS_FAILED(rv)) return rv;
    char *uri = nsnull;
    rv = aFolder->GetURI(&uri);
    if (NS_FAILED(rv)) return rv;
    nsAutoString aName = uri;
    PR_FREEIF(uri);
    if (aName.Find(kImapRootURI) == 0)
        aName.Cut(0, PL_strlen(kImapRootURI));
    else
        return NS_ERROR_FAILURE;
    while (aName[0] == '/')
        aName.Cut(0, 1);
	// cut out user name ### alec, when you clean up url parsing, please get this too!
	PRInt32 userNameEnd = aName.FindChar('@');
	if (userNameEnd > 0)
		aName.Cut(0, userNameEnd + 1);

    PRInt32 hostEnd = aName.FindChar('/');
    if (hostEnd > 0) // must have at least one valid charater
        aName.SetLength(hostEnd);
    char *tmpCString = aName.ToNewCString();
    if (tmpCString && *tmpCString)
    {
        *hostName = PL_strdup(tmpCString);
        rv = NS_OK;
		nsAllocator::Free(tmpCString);
    }
    return rv;
}

NS_IMETHODIMP nsImapMailFolder::UserNeedsToAuthenticateForFolder(PRBool
                                                                 displayOnly,
                                                                 PRBool
                                                                 *authenticate)
{
    nsresult rv = NS_ERROR_FAILURE;
    return rv;
}

NS_IMETHODIMP nsImapMailFolder::RememberPassword(const char *password)
{
    nsresult rv = NS_ERROR_FAILURE;
    return rv;
}

NS_IMETHODIMP nsImapMailFolder::GetRememberedPassword(char ** password)
{
    nsresult rv = NS_ERROR_FAILURE;
    return rv;
}


NS_IMETHODIMP
nsImapMailFolder::MarkMessagesRead(nsISupportsArray *messages, PRBool markRead)
{
	nsresult rv;

	// tell the folder to do it, which will mark them read in the db.
	rv = nsMsgFolder::MarkMessagesRead(messages, markRead);
	if (NS_SUCCEEDED(rv))
	{
		nsCString messageIds;
        nsMsgKeyArray keysToMarkRead;
		rv = BuildIdsAndKeyArray(messages, messageIds, keysToMarkRead);
		if (NS_FAILED(rv)) return rv;

		rv = StoreImapFlags(kImapMsgSeenFlag, markRead,  keysToMarkRead);
		mDatabase->Commit(nsMsgDBCommitType::kLargeCommit);
	}
	return rv;
}

NS_IMETHODIMP
nsImapMailFolder::MarkAllMessagesRead(void)
{
	nsresult rv = GetDatabase();
	
	if(NS_SUCCEEDED(rv))
	{
		nsMsgKeyArray thoseMarked;
		rv = mDatabase->MarkAllRead(&thoseMarked);
		if (NS_SUCCEEDED(rv))
		{
			rv = StoreImapFlags(kImapMsgSeenFlag, PR_TRUE, thoseMarked);
			mDatabase->Commit(nsMsgDBCommitType::kLargeCommit);
		}
	}

	return rv;
}

NS_IMETHODIMP
nsImapMailFolder::MarkMessagesFlagged(nsISupportsArray *messages, PRBool markFlagged)
{
	nsresult rv;

	// tell the folder to do it, which will mark them read in the db.
	rv = nsMsgFolder::MarkMessagesFlagged(messages, markFlagged);
	if (NS_SUCCEEDED(rv))
	{
		nsCString messageIds;
        nsMsgKeyArray keysToMarkFlagged;
		rv = BuildIdsAndKeyArray(messages, messageIds, keysToMarkFlagged);
		if (NS_FAILED(rv)) return rv;

		rv = StoreImapFlags(kImapMsgFlaggedFlag, markFlagged,  keysToMarkFlagged);
		mDatabase->Commit(nsMsgDBCommitType::kLargeCommit);
	}
	return rv;
}


NS_IMETHODIMP nsImapMailFolder::Adopt(nsIMsgFolder *srcFolder, 
                                      PRUint32 *outPos)
{
    nsresult rv = NS_ERROR_FAILURE;
    return rv;
}

nsresult nsImapMailFolder::GetDBFolderInfoAndDB(
    nsIDBFolderInfo **folderInfo, nsIMsgDatabase **db)
{
    nsresult openErr=NS_ERROR_UNEXPECTED;
    if(!db || !folderInfo)
		return NS_ERROR_NULL_POINTER;	//ducarroz: should we use NS_ERROR_INVALID_ARG?

	nsCOMPtr<nsIMsgDatabase> mailDBFactory;
	nsCOMPtr<nsIMsgDatabase> mailDB;

	nsresult rv = nsComponentManager::CreateInstance(kCImapDB, nsnull, nsIMsgDatabase::GetIID(), getter_AddRefs(mailDBFactory));
	if (NS_SUCCEEDED(rv) && mailDBFactory)
	{

		nsCOMPtr<nsIFileSpec> pathSpec;
		rv = GetPath(getter_AddRefs(pathSpec));
		if (NS_FAILED(rv)) return rv;

		openErr = mailDBFactory->Open(pathSpec, PR_FALSE, PR_FALSE, (nsIMsgDatabase **) &mailDB);
	}

    *db = mailDB;
	NS_IF_ADDREF(*db);
    if (NS_SUCCEEDED(openErr)&& *db)
        openErr = (*db)->GetDBFolderInfo(folderInfo);
    return openErr;
}

nsresult
nsImapMailFolder::BuildIdsAndKeyArray(nsISupportsArray* messages,
                                      nsCString& msgIds,
                                      nsMsgKeyArray& keyArray)
{
    nsresult rv = NS_ERROR_NULL_POINTER;
    PRUint32 count = 0;
    PRUint32 i;
    nsCOMPtr<nsISupports> msgSupports;
    nsCOMPtr<nsIMessage> message;

    if (!messages) return rv;

    rv = messages->Count(&count);
    if (NS_FAILED(rv)) return rv;

    // build up message keys.
    for (i = 0; i < count; i++)
    {
        msgSupports = getter_AddRefs(messages->ElementAt(i));
        message = do_QueryInterface(msgSupports);
        if (message)
        {
            nsMsgKey key;
            rv = message->GetMessageKey(&key);
            if (NS_SUCCEEDED(rv))
                keyArray.Add(key);
        }
    }
    
	return AllocateUidStringFromKeyArray(keyArray, msgIds);
}

/* static */ nsresult
nsImapMailFolder::AllocateUidStringFromKeyArray(nsMsgKeyArray &keyArray, nsCString &msgIds)
{
    nsresult rv = NS_OK;
	PRInt32 startSequence = (keyArray.GetSize() > 0) ? keyArray[0] : -1;
	PRInt32 curSequenceEnd = startSequence;
	PRUint32 total = keyArray.GetSize();
	// sort keys and then generate ranges instead of singletons!
	keyArray.QuickSort();
    for (PRUint32 keyIndex = 0; keyIndex < total; keyIndex++)
	{
		PRUint32 curKey = keyArray[keyIndex];
		PRUint32 nextKey = (keyIndex + 1 < total) ? keyArray[keyIndex + 1] : 0xFFFFFFFF;
		PRBool lastKey = (nextKey == 0xFFFFFFFF);

		if (lastKey)
			curSequenceEnd = curKey;
		if (nextKey == (PRUint32) curSequenceEnd + 1 && !lastKey)
		{
			curSequenceEnd = nextKey;
			continue;
		}
		else if (curSequenceEnd > startSequence)
		{
			msgIds.Append(startSequence, 10);
			msgIds += ':';
			msgIds.Append(curSequenceEnd, 10);
			if (!lastKey)
				msgIds += ',';
			startSequence = nextKey;
			curSequenceEnd = startSequence;
		}
		else
		{
			startSequence = nextKey;
			curSequenceEnd = startSequence;
			msgIds.Append(keyArray[keyIndex], 10);
			if (!lastKey)
				msgIds += ',';
		}
	}
    return rv;
}

NS_IMETHODIMP nsImapMailFolder::DeleteMessages(nsISupportsArray *messages,
                                               nsITransactionManager *txnMgr,
                                               PRBool deleteStorage)
{
    nsresult rv = NS_ERROR_FAILURE;
    // *** jt - assuming delete is move to the trash folder for now
    nsCOMPtr<nsIEnumerator> aEnumerator;
    nsCOMPtr<nsIRDFResource> res;
    nsCString uri;
    PRBool isTrashFolder = PR_FALSE;
    nsCString messageIds;
    nsMsgKeyArray srcKeyArray;

    rv = BuildIdsAndKeyArray(messages, messageIds, srcKeyArray);
    if (NS_FAILED(rv)) return rv;

    rv = GetFlag(MSG_FOLDER_FLAG_TRASH, &isTrashFolder);
    if (NS_SUCCEEDED(rv) && isTrashFolder)
    {
        return StoreImapFlags(kImapMsgDeletedFlag, PR_TRUE, srcKeyArray);
    }
    else
    {
        SetTransactionManager(txnMgr);
        
        nsCOMPtr<nsIMsgFolder> rootFolder;
        rv = GetRootFolder(getter_AddRefs(rootFolder));
        if (NS_SUCCEEDED(rv) && rootFolder)
        {
            nsCOMPtr<nsIMsgFolder> trashFolder;
            PRUint32 numFolders = 0;
            rv = rootFolder->GetFoldersWithFlag(MSG_FOLDER_FLAG_TRASH,
                                                getter_AddRefs(trashFolder),
                                                1, &numFolders);

            if(NS_SUCCEEDED(rv) && trashFolder)
            {
                nsCOMPtr<nsIMsgFolder> srcFolder;
                nsCOMPtr<nsISupports>srcSupport;
                PRUint32 count = 0;
                rv = messages->Count(&count);
                
                rv = QueryInterface(nsIMsgFolder::GetIID(),
                                    getter_AddRefs(srcFolder));
                if (NS_SUCCEEDED(rv))
                    srcSupport = do_QueryInterface(srcFolder, &rv);
                
                rv = InitCopyState(srcSupport, messages, PR_TRUE, PR_TRUE,
                                   nsnull); 
                if (NS_FAILED(rv)) return rv;
                nsCOMPtr<nsISupports> copySupport =
                    do_QueryInterface(m_copyState); 
                m_copyState->m_curIndex = m_copyState->m_totalCount;
                NS_WITH_SERVICE(nsIImapService, imapService, kCImapService,
                                &rv); 
                if (NS_SUCCEEDED(rv) && imapService)
                    rv = imapService->OnlineMessageCopy(
                        m_eventQueue, this, messageIds.GetBuffer(),
                        trashFolder, PR_TRUE, PR_TRUE, this, nsnull,
                        copySupport);

                if (NS_SUCCEEDED(rv))
                {
                    nsImapMoveCopyMsgTxn* undoMsgTxn = new
                        nsImapMoveCopyMsgTxn( 
                            this, &srcKeyArray, messageIds.GetBuffer(),
                            trashFolder, PR_TRUE, PR_TRUE, m_eventQueue, this);

                    if (undoMsgTxn)
                        rv = undoMsgTxn->QueryInterface(
                            nsCOMTypeInfo<nsImapMoveCopyMsgTxn>::GetIID(), 
                            getter_AddRefs(m_copyState->m_undoMsgTxn) );
                    if (undoMsgTxn)
                    {
                        nsString undoString = count > 1 ? 
                            "Undo Delete Messages" :
                            "Undo Delete Message";
                        nsString redoString = count > 1 ? 
                            "Redo Delete Messages" :
                            "Redo Delete Message";
                        rv = undoMsgTxn->SetUndoString(&undoString);
                        rv = undoMsgTxn->SetRedoString(&redoString);
                    }
                }
            }
        }
    }
    return rv;
}

PRBool
nsImapMailFolder::InTrash(nsIMsgFolder* folder)
{
    nsCOMPtr<nsIMsgFolder> parent;
    nsCOMPtr<nsIFolder> iFolder;
    nsCOMPtr<nsIMsgFolder> curFolder;
    nsresult rv;
    PRUint32 flags = 0;

    if (!folder) return PR_FALSE;
    curFolder = do_QueryInterface(folder, &rv);
    if (NS_FAILED(rv)) return PR_FALSE;

    do 
    {
        rv = curFolder->GetParent(getter_AddRefs(iFolder));
        if (NS_FAILED(rv)) return PR_FALSE;
        parent = do_QueryInterface(iFolder, &rv);
        if (NS_FAILED(rv)) return PR_FALSE;
        rv = parent->GetFlags(&flags);
        if (NS_FAILED(rv)) return PR_FALSE;
        if (flags & MSG_FOLDER_FLAG_TRASH)
            return PR_TRUE;
        curFolder = do_QueryInterface(parent, &rv);
    } while (NS_SUCCEEDED(rv) && curFolder);

    return PR_FALSE;
}
NS_IMETHODIMP
nsImapMailFolder::DeleteSubFolders(nsISupportsArray* folders)
{
    nsCOMPtr<nsIMsgFolder> curFolder;
    nsCOMPtr<nsISupports> folderSupport;
    nsCOMPtr<nsIUrlListener> urlListener;
    nsCOMPtr<nsIMsgFolder> trashFolder;
    PRUint32 i, folderCount = 0;
    nsresult rv;

    NS_WITH_SERVICE (nsIImapService, imapService, kCImapService, &rv);
    if (NS_SUCCEEDED(rv))
    {
        rv = folders->Count(&folderCount);
        if (NS_SUCCEEDED(rv))
        {
            rv = GetTrashFolder(getter_AddRefs(trashFolder));
            for (i = 0; i < folderCount; i++)
            {
                folderSupport = getter_AddRefs(folders->ElementAt(i));
                curFolder = do_QueryInterface(folderSupport, &rv);
                if (NS_SUCCEEDED(rv))
                {
                    urlListener = do_QueryInterface(curFolder);
                    if (InTrash(curFolder))
                        rv = imapService->DeleteFolder(m_eventQueue,
                                                       curFolder,
                                                       urlListener,
                                                       nsnull);
                    else
                        rv = imapService->MoveFolder(m_eventQueue,
                                                     curFolder,
                                                     trashFolder,
                                                     urlListener,
                                                     nsnull);
                }
            }
        }
    }
        
    return nsMsgFolder::DeleteSubFolders(folders);
}

NS_IMETHODIMP nsImapMailFolder::GetNewMessages()
{
    nsresult rv = NS_ERROR_FAILURE;
    NS_WITH_SERVICE(nsIImapService, imapService, kCImapService, &rv);
    if (NS_FAILED(rv)) return rv;
	nsCOMPtr<nsIMsgFolder> inbox;
	nsCOMPtr<nsIMsgFolder> rootFolder;
	rv = GetRootFolder(getter_AddRefs(rootFolder));
	if(NS_SUCCEEDED(rv) && rootFolder)
	{
		PRUint32 numFolders;
		rv = rootFolder->GetFoldersWithFlag(MSG_FOLDER_FLAG_INBOX, getter_AddRefs(inbox), 1, &numFolders);
	}
		nsCOMPtr <nsIEventQueue> eventQ;
		NS_WITH_SERVICE(nsIEventQueueService, pEventQService, kEventQueueServiceCID, &rv); 
		if (NS_SUCCEEDED(rv) && pEventQService)
			pEventQService->GetThreadEventQueue(PR_GetCurrentThread(),
												getter_AddRefs(eventQ));
    rv = imapService->SelectFolder(eventQ, inbox, this, nsnull, nsnull);
    return rv;
}

NS_IMETHODIMP nsImapMailFolder::UpdateImapMailboxInfo(
	nsIImapProtocol* aProtocol,	mailbox_spec* aSpec)
{
	nsresult rv = NS_ERROR_FAILURE;
    nsCOMPtr<nsIMsgDatabase> mailDBFactory;

	nsCOMPtr<nsIFileSpec> pathSpec;
	rv = GetPath(getter_AddRefs(pathSpec));
	if (NS_FAILED(rv)) return rv;

    nsFileSpec dbName;
	rv = pathSpec->GetFileSpec(&dbName);
	if (NS_FAILED(rv)) return rv;

    rv = nsComponentManager::CreateInstance(kCImapDB, nsnull,
                                            nsIMsgDatabase::GetIID(),
                                            (void **) getter_AddRefs(mailDBFactory));
    if (NS_FAILED(rv)) return rv;

    if (!mDatabase)
    {
        // if we pass in PR_TRUE for upgrading, the db code will ignore the
        // summary out of date problem for now.
        rv = mailDBFactory->Open(pathSpec, PR_TRUE, PR_TRUE, (nsIMsgDatabase **)
                                 getter_AddRefs(mDatabase));
        if (NS_FAILED(rv))
            return rv;
        if (!mDatabase) 
            return NS_ERROR_NULL_POINTER;
        if (mDatabase && mAddListener)
            mDatabase->AddListener(this);
    }
    if (aSpec->folderSelected)
    {
     	nsMsgKeyArray existingKeys;
    	nsMsgKeyArray keysToDelete;
    	nsMsgKeyArray keysToFetch;
		nsCOMPtr<nsIDBFolderInfo> dbFolderInfo;
		PRInt32 imapUIDValidity = 0;

        rv = NS_ERROR_UNEXPECTED;
        if (mDatabase)
            rv = mDatabase->GetDBFolderInfo(getter_AddRefs(dbFolderInfo));

		if (NS_SUCCEEDED(rv) && dbFolderInfo)
			dbFolderInfo->GetImapUidValidity(&imapUIDValidity);

        if (mDatabase) {
            mDatabase->ListAllKeys(existingKeys);
            if (mDatabase->ListAllOfflineDeletes(&existingKeys) > 0)
                existingKeys.QuickSort();
        }
    	if ((imapUIDValidity != aSpec->folder_UIDVALIDITY)	/* && // if UIDVALIDITY Changed 
    		!NET_IsOffline() */)
    	{

#if TRANSFER_INFO
			TNeoFolderInfoTransfer *originalInfo = NULL;
			originalInfo = new TNeoFolderInfoTransfer(dbFolderInfo);
#endif // 0
            if (mDatabase)
                mDatabase->ForceClosed();
			mDatabase = null_nsCOMPtr();
				
			nsLocalFolderSummarySpec	summarySpec(dbName);
			// Remove summary file.
			summarySpec.Delete(PR_FALSE);
			
			// Create a new summary file, update the folder message counts, and
			// Close the summary file db.
			rv = mailDBFactory->Open(pathSpec, PR_TRUE, PR_TRUE, getter_AddRefs(mDatabase));

			// ********** Important *************
			// David, help me here I don't know this is right or wrong
			if (rv == NS_MSG_ERROR_FOLDER_SUMMARY_MISSING)
					rv = NS_OK;

			if (NS_FAILED(rv) && mDatabase)
			{
				mDatabase->ForceClosed();
				mDatabase = null_nsCOMPtr();
			}
			else if (NS_SUCCEEDED(rv) && mDatabase)
			{
#if TRANSFER_INFO
				if (originalInfo && mDatabase)
				{
					originalInfo->TransferFolderInfo(mDatabase->m_dbFolderInfo);
					delete originalInfo;
				}
#endif
				SummaryChanged();
                rv = NS_ERROR_UNEXPECTED;
                if (mDatabase) {
					if(mAddListener)
	                    mDatabase->AddListener(this);
                    rv = mDatabase->GetDBFolderInfo(getter_AddRefs(dbFolderInfo));
                }
			}
			// store the new UIDVALIDITY value

			if (NS_SUCCEEDED(rv) && dbFolderInfo)
    			dbFolderInfo->SetImapUidValidity(aSpec->folder_UIDVALIDITY);
    										// delete all my msgs, the keys are bogus now
											// add every message in this folder
			existingKeys.RemoveAll();
//			keysToDelete.CopyArray(&existingKeys);

			if (aSpec->flagState)
			{
				nsMsgKeyArray no_existingKeys;
	  			FindKeysToAdd(no_existingKeys, keysToFetch, aSpec->flagState);
    		}
    	}		
    	else if (!aSpec->flagState /*&& !NET_IsOffline() */)	// if there are no messages on the server
    	{
			keysToDelete.CopyArray(&existingKeys);
    	}
    	else /* if ( !NET_IsOffline()) */
    	{
    		FindKeysToDelete(existingKeys, keysToDelete, aSpec->flagState);
            
			// if this is the result of an expunge then don't grab headers
			if (!(aSpec->box_flags & kJustExpunged))
				FindKeysToAdd(existingKeys, keysToFetch, aSpec->flagState);
    	}
    	
    	
    	if (keysToDelete.GetSize())
    	{
			PRUint32 total;
            
			// It would be nice to notify RDF or whoever of a mass delete here.
            if (mDatabase) {
                mDatabase->DeleteMessages(&keysToDelete,NULL);
                total = keysToDelete.GetSize();
            }
		}
		// if this is the INBOX, tell the stand-alone biff about the new high water mark
		if (mFlags & MSG_FOLDER_FLAG_INBOX)
		{
			PRInt32 numRecentMessages = 0;

			if (keysToFetch.GetSize() > 0)
			{
				SetBiffState(nsMsgBiffState_NewMail);
				if (aSpec->flagState)
				{
					aSpec->flagState->GetNumberOfRecentMessages(&numRecentMessages);
					SetNumNewMessages(numRecentMessages);
				}
			}
		}
	   	if (keysToFetch.GetSize())
    	{			
            PrepareToAddHeadersToMailDB(aProtocol, keysToFetch, aSpec);
			if (aProtocol)
				aProtocol->NotifyBodysToDownload(NULL, 0/*keysToFetch.GetSize() */);
    	}
    	else 
    	{
            // let the imap libnet module know that we don't need headers
			if (aProtocol)
				aProtocol->NotifyHdrsToDownload(NULL, 0);
			// wait until we can get body id monitor before continuing.
//			IMAP_BodyIdMonitor(adoptedBoxSpec->connection, PR_TRUE);
			// I think the real fix for this is to seperate the header ids from body id's.
			// this is for fetching bodies for offline use
			if (aProtocol)
				aProtocol->NotifyBodysToDownload(NULL, 0/*keysToFetch.GetSize() */);
//			NotifyFetchAnyNeededBodies(aSpec->connection, mailDB);
//			IMAP_BodyIdMonitor(adoptedBoxSpec->connection, PR_FALSE);
    	}
    }

    if (NS_FAILED(rv))
        dbName.Delete(PR_FALSE);

	return rv;
}

NS_IMETHODIMP nsImapMailFolder::UpdateImapMailboxStatus(
	nsIImapProtocol* aProtocol,	mailbox_spec* aSpec)
{
	nsresult rv = NS_ERROR_FAILURE;
	return rv;
}

NS_IMETHODIMP nsImapMailFolder::ChildDiscoverySucceeded(
	nsIImapProtocol* aProtocol)
{
	nsresult rv = NS_ERROR_FAILURE;
	return rv;
}

NS_IMETHODIMP nsImapMailFolder::PromptUserForSubscribeUpdatePath(
	nsIImapProtocol* aProtocol,	PRBool* aBool)
{
	nsresult rv = NS_ERROR_FAILURE;
	return rv;
}

NS_IMETHODIMP nsImapMailFolder::SetupHeaderParseStream(
    nsIImapProtocol* aProtocol, StreamInfo* aStreamInfo)
{
    nsresult rv = NS_ERROR_FAILURE;

	if (!mDatabase)
		GetDatabase();

	if (mFlags & MSG_FOLDER_FLAG_INBOX && !m_filterList)
	{
		NS_WITH_SERVICE(nsIMsgFilterService, filterService, kMsgFilterServiceCID, &rv);
		if (NS_SUCCEEDED(rv))
		{

			nsCOMPtr <nsIMsgFolder> rootFolder;
			rv = GetRootFolder(getter_AddRefs(rootFolder));

			if (NS_SUCCEEDED(rv))
			{

				nsCOMPtr <nsIFileSpec> rootDir;

				rv = rootFolder->GetPath(getter_AddRefs(rootDir));

				if (NS_SUCCEEDED(rv))
				{
					nsFileSpec		filterFile;

					rootDir->GetFileSpec(&filterFile);
					// need a file spec for filters...
					filterFile += "rules.dat";
					nsresult res;
                    res = filterService->OpenFilterList(&filterFile, getter_AddRefs(m_filterList));
				}
			}
		}

	}
	m_nextMessageByteLength = aStreamInfo->size;
	if (!m_msgParser)
	{
		rv = nsComponentManager::CreateInstance(kParseMailMsgStateCID, nsnull, 
			nsIMsgParseMailMsgState::GetIID(), (void **) getter_AddRefs(m_msgParser));
		if (NS_SUCCEEDED(rv))
			m_msgParser->SetMailDB(mDatabase);
	}
	else
		m_msgParser->Clear();
	
	if (m_msgParser)
		return m_msgParser->SetState(nsIMsgParseMailMsgState::ParseHeadersState);
	else
		return NS_ERROR_OUT_OF_MEMORY;
    return rv;
}

NS_IMETHODIMP nsImapMailFolder::ParseAdoptedHeaderLine(
    nsIImapProtocol* aProtocol, msg_line_info* aMsgLineInfo)
{
    // we can get blocks that contain more than one line, 
    // but they never contain partial lines
	char *str = aMsgLineInfo->adoptedMessageLine;
	m_curMsgUid = aMsgLineInfo->uidOfMessage;
	m_msgParser->SetEnvelopePos(m_curMsgUid);	// OK, this is silly (but
                                                // we'll fix
                                                // it). m_envelope_pos, for
                                                // local folders, 
    // is the msg key. Setting this will set the msg key for the new header.

	PRInt32 len = nsCRT::strlen(str);
    char *currentEOL  = PL_strstr(str, MSG_LINEBREAK);
    const char *currentLine = str;
    while (currentLine < (str + len))
    {
        if (currentEOL)
        {
            m_msgParser->ParseAFolderLine(currentLine, 
                                         (currentEOL + MSG_LINEBREAK_LEN) -
                                         currentLine);
            currentLine = currentEOL + MSG_LINEBREAK_LEN;
            currentEOL  = PL_strstr(currentLine, MSG_LINEBREAK);
        }
        else
        {
			m_msgParser->ParseAFolderLine(currentLine, PL_strlen(currentLine));
            currentLine = str + len + 1;
        }
    }
    return NS_OK;
}
    
NS_IMETHODIMP nsImapMailFolder::NormalEndHeaderParseStream(nsIImapProtocol*
                                                           aProtocol)
{
	nsCOMPtr<nsIMsgDBHdr> newMsgHdr;
	nsresult rv = NS_OK;

	if (m_msgParser)
		m_msgParser->GetNewMsgHdr(getter_AddRefs(newMsgHdr));
	if (NS_SUCCEEDED(rv) && newMsgHdr)
	{
		char *headers;
		PRInt32 headersSize;

		newMsgHdr->SetMessageKey(m_curMsgUid);
		TweakHeaderFlags(aProtocol, newMsgHdr);
		m_msgMovedByFilter = PR_FALSE;
		// If this is the inbox, try to apply filters.
		if (mFlags & MSG_FOLDER_FLAG_INBOX)
		{
			rv = m_msgParser->GetAllHeaders(&headers, &headersSize);

			if (NS_SUCCEEDED(rv) && headers)
			{
				if (m_filterList)
					m_filterList->ApplyFiltersToHdr(nsMsgFilterType::InboxRule, newMsgHdr, this, mDatabase, 
						headers, headersSize, this);
			}
		}
		// here we need to tweak flags from uid state..
		if (!m_msgMovedByFilter)
			mDatabase->AddNewHdrToDB(newMsgHdr, PR_TRUE);
		// I don't think we want to do this - it does bad things like set the size incorrectly.
//		m_msgParser->FinishHeader();
	}
    return NS_OK;
}
    
NS_IMETHODIMP nsImapMailFolder::AbortHeaderParseStream(nsIImapProtocol*
                                                       aProtocol)
{
    nsresult rv = NS_ERROR_FAILURE;
    return rv;
}
 
NS_IMETHODIMP nsImapMailFolder::CreateMessageFromMsgDBHdr(nsIMsgDBHdr *msgDBHdr, nsIMessage **message)
{
	
    nsresult rv; 
    NS_WITH_SERVICE(nsIRDFService, rdfService, kRDFServiceCID, &rv); 
    if (NS_FAILED(rv)) return rv;

	char* msgURI = nsnull;
	nsFileSpec path;
	nsMsgKey key;
    nsCOMPtr<nsIRDFResource> res;

	rv = msgDBHdr->GetMessageKey(&key);

	if(NS_SUCCEEDED(rv))
		rv = nsBuildImapMessageURI(mURI, key, &msgURI);


	if(NS_SUCCEEDED(rv))
		rv = rdfService->GetResource(msgURI, getter_AddRefs(res));

	if(msgURI)
		PR_smprintf_free(msgURI);

	if(NS_SUCCEEDED(rv))
	{
		nsCOMPtr<nsIDBMessage> messageResource = do_QueryInterface(res);
		if(messageResource)
		{
			messageResource->SetMsgDBHdr(msgDBHdr);
			*message = messageResource;
			NS_IF_ADDREF(*message);
		}
	}
	return rv;
}

  
NS_IMETHODIMP nsImapMailFolder::BeginCopy(nsIMessage *message)
{
	nsresult rv = NS_ERROR_NULL_POINTER;
    if (!m_copyState) return rv;
    if (m_copyState->m_tmpFileSpec) // leftover file spec nuke it
    {
        PRBool isOpen = PR_FALSE;
        rv = m_copyState->m_tmpFileSpec->IsStreamOpen(&isOpen);
        if (isOpen)
            m_copyState->m_tmpFileSpec->CloseStream();
        nsFileSpec fileSpec;
        m_copyState->m_tmpFileSpec->GetFileSpec(&fileSpec);
        if (fileSpec.Valid())
            fileSpec.Delete(PR_FALSE);
        m_copyState->m_tmpFileSpec = null_nsCOMPtr();
    }
    if (message)
        m_copyState->m_message = do_QueryInterface(message, &rv);

    nsFileSpec tmpFileSpec("nscpmsg.txt");
    rv = NS_NewFileSpecWithSpec(tmpFileSpec,
                                getter_AddRefs(m_copyState->m_tmpFileSpec));
    if (NS_SUCCEEDED(rv) && m_copyState->m_tmpFileSpec)
        rv = m_copyState->m_tmpFileSpec->OpenStreamForWriting();
    if (!m_copyState->m_dataBuffer)
    {
        m_copyState->m_dataBuffer = (char*) PR_CALLOC(FOUR_K+1);
        if (!m_copyState->m_dataBuffer)
            rv = NS_ERROR_OUT_OF_MEMORY;
    }
	return rv;
}

NS_IMETHODIMP nsImapMailFolder::CopyData(nsIInputStream *aIStream,
										 PRInt32 aLength)
{
	nsresult rv = NS_ERROR_NULL_POINTER;
    NS_ASSERTION(m_copyState && m_copyState->m_dataBuffer &&
                 m_copyState->m_tmpFileSpec, "Fatal copy operation error\n");
    if (!m_copyState || !m_copyState->m_dataBuffer ||
        !m_copyState->m_tmpFileSpec) return rv;

    PRUint32 readCount, maxReadCount = FOUR_K;
    PRInt32 writeCount;
    char *start, *end;

    while (aLength > 0)
    {
        if (aLength < (PRInt32) maxReadCount)
            maxReadCount = aLength;
        rv = aIStream->Read(m_copyState->m_dataBuffer, maxReadCount,
                            &readCount);
        if (NS_FAILED(rv)) return rv;

        m_copyState->m_dataBuffer[readCount] = '\0';

        start = m_copyState->m_dataBuffer;
        end = PL_strstr(start, CRLF);

        while (start && end)
        {
            if (PL_strncasecmp(start, "X-Mozilla-Status:", 17) &&
                PL_strncasecmp(start, "X-Mozilla-Status2:", 18) &&
                PL_strncmp(start, "From - ", 7))
                rv = m_copyState->m_tmpFileSpec->Write(start,
                                                       end-start+2,
                                                       &writeCount);
            start = end+2;
            if (start >= m_copyState->m_dataBuffer+readCount)
                break;
            end = PL_strstr(start, CRLF);
        }
        if (NS_FAILED(rv)) return rv;
        aLength -= readCount;
    }
	return rv;
}

NS_IMETHODIMP nsImapMailFolder::EndCopy(PRBool copySucceeded)
{
	nsresult rv = copySucceeded ? NS_OK : NS_ERROR_FAILURE;
    if (copySucceeded && m_copyState && m_copyState->m_tmpFileSpec)
    {
        nsCOMPtr<nsIUrlListener> urlListener;
        m_copyState->m_tmpFileSpec->Flush();
        m_copyState->m_tmpFileSpec->CloseStream();
        NS_WITH_SERVICE(nsIImapService, imapService, kCImapService, &rv);
        if (NS_FAILED(rv)) return rv;
        rv = QueryInterface(nsIUrlListener::GetIID(),
                            getter_AddRefs(urlListener));
        nsCOMPtr<nsISupports> copySupport;
        if (m_copyState)
            copySupport = do_QueryInterface(m_copyState);
        rv = imapService->AppendMessageFromFile(m_eventQueue,
                                                m_copyState->m_tmpFileSpec,
                                                this, "", PR_TRUE,
                                                m_copyState->m_selectedState,
                                                urlListener, nsnull,
                                                copySupport);
    }
	return rv;
}

NS_IMETHODIMP nsImapMailFolder::ApplyFilterHit(nsIMsgFilter *filter, PRBool *applyMore)
{
	nsMsgRuleActionType actionType;
	void				*value = nsnull;
	PRUint32	newFlags;
	nsresult rv = NS_OK;

	if (!applyMore)
	{
		NS_ASSERTION(PR_FALSE, "need to return status!");
		return NS_ERROR_NULL_POINTER;
	}
	// look at action - currently handle move
#ifdef DEBUG_bienvenu
	printf("got a rule hit!\n");
#endif
	if (NS_SUCCEEDED(filter->GetAction(&actionType, &value)))
	{
		nsCOMPtr<nsIMsgDBHdr> msgHdr;

		if (m_msgParser)
			m_msgParser->GetNewMsgHdr(getter_AddRefs(msgHdr));
		if (NS_SUCCEEDED(rv) && msgHdr)

		{
			PRUint32 msgFlags;
			nsMsgKey		msgKey;
			nsCAutoString trashNameVal;

			msgHdr->GetFlags(&msgFlags);
			msgHdr->GetMessageKey(&msgKey);
			PRBool isRead = (msgFlags & MSG_FLAG_READ);
			switch (actionType)
			{
			case nsMsgFilterAction::Delete:
			{
				PRBool deleteToTrash = DeleteIsMoveToTrash();
				if (deleteToTrash)
				{
					// set value to trash folder
					nsCOMPtr <nsIMsgFolder> mailTrash;
					rv = GetTrashFolder(getter_AddRefs(mailTrash));;
					if (NS_SUCCEEDED(rv) && mailTrash)
					{
						// this sucks - but we need value to live past this scope
						// so we use an nsString from above.
						PRUnichar *folderName = nsnull;
						rv = mailTrash->GetName(&folderName);
						trashNameVal = nsCAutoString(folderName);
						nsCRT::free(folderName);
						value = (void *) trashNameVal.GetBuffer();
					}

					msgHdr->OrFlags(MSG_FLAG_READ, &newFlags);	// mark read in trash.
				}
				else	// (!deleteToTrash)
				{
					msgHdr->OrFlags(MSG_FLAG_READ | MSG_FLAG_IMAP_DELETED, &newFlags);
					nsMsgKeyArray	keysToFlag;

					keysToFlag.Add(msgKey);
					StoreImapFlags(kImapMsgSeenFlag | kImapMsgDeletedFlag, PR_TRUE, keysToFlag);
//					if (!showDeletedMessages)
//						msgMoved = PR_TRUE;	// this will prevent us from adding the header to the db.

				}
			}
			// note that delete falls through to move.
			case nsMsgFilterAction::MoveToFolder:
			{
				// if moving to a different file, do it.
				PRUnichar *folderName = nsnull;
				rv = GetName(&folderName);

				if (value && nsCRT::strcasecmp(folderName, (char *) value))
				{
					msgHdr->GetFlags(&msgFlags);

					if (msgFlags & MSG_FLAG_MDN_REPORT_NEEDED &&
						!isRead)
					{

#if DOING_MDN	// leave it to the user aciton
						struct message_header to;
						struct message_header cc;
						GetAggregateHeader (m_toList, &to);
						GetAggregateHeader (m_ccList, &cc);
						msgHdr->SetFlags(msgFlags & ~MSG_FLAG_MDN_REPORT_NEEDED);
						msgHdr->OrFlags(MSG_FLAG_MDN_REPORT_SENT, &newFlags);
						if (actionType == nsMsgFilterActionDelete)
						{
							MSG_ProcessMdnNeededState processMdnNeeded
								(MSG_ProcessMdnNeededState::eDeleted,
								 m_pane, m_folder, msgHdr->GetMessageKey(),
								 &state->m_return_path, &state->m_mdn_dnt, 
								 &to, &cc, &state->m_subject, 
								 &state->m_date, &state->m_mdn_original_recipient,
								 &state->m_message_id, state->m_headers, 
								 (PRInt32) state->m_headers_fp, PR_TRUE);
						}
						else
						{
							MSG_ProcessMdnNeededState processMdnNeeded
								(MSG_ProcessMdnNeededState::eProcessed,
								 m_pane, m_folder, msgHdr->GetMessageKey(),
								 &state->m_return_path, &state->m_mdn_dnt, 
								 &to, &cc, &state->m_subject, 
								 &state->m_date, &state->m_mdn_original_recipient,
								 &state->m_message_id, state->m_headers, 
								 (PRInt32) state->m_headers_fp, PR_TRUE);
						}
						char *tmp = (char*) to.value;
						PR_FREEIF(tmp);
						tmp = (char*) cc.value;
						PR_FREEIF(tmp);
#endif
					}
					nsresult err = MoveIncorporatedMessage(msgHdr, mDatabase, (char *) value, filter);
					if (NS_SUCCEEDED(err))
					{
						m_msgMovedByFilter = PR_TRUE;
						*applyMore = PR_FALSE;
					}

				}
				nsAllocator::Free(folderName);
			}
				break;
			case nsMsgFilterAction::MarkRead:
				{
					nsMsgKeyArray	keysToFlag;

					keysToFlag.Add(msgKey);
					StoreImapFlags(kImapMsgSeenFlag, PR_TRUE, keysToFlag);
				}
//				MarkFilteredMessageRead(msgHdr);
				break;
			case nsMsgFilterAction::KillThread:
				// for ignore and watch, we will need the db
				// to check for the flags in msgHdr's that
				// get added, because only then will we know
				// the thread they're getting added to.
				msgHdr->OrFlags(MSG_FLAG_IGNORED, &newFlags);
				break;
			case nsMsgFilterAction::WatchThread:
				msgHdr->OrFlags(MSG_FLAG_WATCHED, &newFlags);
				break;
			case nsMsgFilterAction::ChangePriority:
				msgHdr->SetPriority(*(nsMsgPriority *) &value);
				break;
			default:
				break;
			}
		}
	}
	return rv;
}


nsresult nsImapMailFolder::StoreImapFlags(imapMessageFlagsType flags, PRBool addFlags, nsMsgKeyArray &keysToFlag)
{
	nsresult rv = NS_OK;
	if (PR_TRUE/* !NET_IsOffline() */)
	{
	    NS_WITH_SERVICE(nsIImapService, imapService, kCImapService, &rv);
        if (NS_SUCCEEDED(rv))
        {
            if (addFlags)
            {
                nsCString msgIds;
                
                AllocateUidStringFromKeyArray(keysToFlag, msgIds);
                imapService->AddMessageFlags(m_eventQueue, this, nsnull,
                                             nsnull, msgIds, flags, PR_TRUE);
            }
            else
            {
                nsCString msgIds;
                
                AllocateUidStringFromKeyArray(keysToFlag, msgIds);
                imapService->SubtractMessageFlags(m_eventQueue, this, nsnull,
                                                  nsnull, msgIds, flags,
                                                  PR_TRUE);
            }
            // force to update the thread pane view
            rv = imapService->SelectFolder(m_eventQueue, this, this, nsnull, nsnull);
        }
	}
	else
	{
#ifdef OFFLINE_IMAP
		MailDB *mailDb = NULL; // change flags offline
		PRBool wasCreated=PR_FALSE;

		ImapMailDB::Open(GetPathname(), PR_TRUE, &mailDb, GetMaster(), &wasCreated);
		if (mailDb)
		{
			UndoManager *undoManager = NULL;
			uint32 total = keysToFlag.GetSize();

			for (int keyIndex=0; keyIndex < total; keyIndex++)
			{
				OfflineImapOperation	*op = mailDb->GetOfflineOpForKey(keysToFlag[keyIndex], PR_TRUE);
				if (op)
				{
					MailDB *originalDB = NULL;
					if (op->GetOperationFlags() & kMoveResult)
					{
						// get the op in the source db and change the flags there
						OfflineImapOperation	*originalOp = GetOriginalOp(op, &originalDB);
						if (originalOp)
						{
							if (undoManager && undoManager->GetState() == UndoIdle && NET_IsOffline()) {
								OfflineIMAPUndoAction *undoAction = new 
										OfflineIMAPUndoAction(paneForFlagUrl, (MSG_FolderInfo*) this, op->GetMessageKey(), kFlagsChanged,
										this, NULL, flags, NULL, addFlags);
								if (undoAction)
									undoManager->AddUndoAction(undoAction);
							}
							op->unrefer();
							op = originalOp;
						}
					}
						
					if (addFlags)
						op->SetImapFlagOperation(op->GetNewMessageFlags() | flags);
					else
						op->SetImapFlagOperation(op->GetNewMessageFlags() & ~flags);
					op->unrefer();

					if (originalDB)
					{
						originalDB->Close();
						originalDB = NULL;
					}
				}
			}
			mailDb->Commit();	// flush offline flags
			mailDb->Close();
			mailDb = NULL;
		}
#endif // OFFLINE_IMAP
	}
	return rv;
}

nsresult nsImapMailFolder::MoveIncorporatedMessage(nsIMsgDBHdr *mailHdr, 
											   nsIMsgDatabase *sourceDB, 
											   char *destFolder,
											   nsIMsgFilter *filter)
{
	nsresult err = NS_OK;
	
	if (!m_moveCoalescer)
		m_moveCoalescer = new nsImapMoveCoalescer(this);
	if (m_moveCoalescer)
	{

	 	// look for matching imap folders, then pop folders
		nsCOMPtr<nsIMsgIncomingServer> server;
		nsresult rv = GetServer(getter_AddRefs(server));
 
		nsCOMPtr<nsIFolder> rootFolder;
		if (NS_SUCCEEDED(rv)) 
			rv = server->GetRootFolder(getter_AddRefs(rootFolder));

		if (!NS_SUCCEEDED(rv))
			return rv;

		nsCOMPtr <nsIFolder> destIFolder;
		rootFolder->FindSubFolder (destFolder, getter_AddRefs(destIFolder));

		nsCOMPtr <nsIMsgFolder> msgFolder;
		msgFolder = do_QueryInterface(destIFolder);

	 	if (destIFolder)
	 	{
			// put the header into the source db, since it needs to be there when we copy it
			// and we need a valid header to pass to StartAsyncCopyMessagesInto
			nsMsgKey keyToFilter;
			mailHdr->GetMessageKey(&keyToFilter);

			if (sourceDB && msgFolder)
			{
				PRBool imapDeleteIsMoveToTrash = DeleteIsMoveToTrash();

				m_moveCoalescer->AddMove (msgFolder, keyToFilter);
				// For each folder, we need to keep track of the ids we want to move to that
				// folder - we used to store them in the MSG_FolderInfo and then when we'd finished
				// downloading headers, we'd iterate through all the folders looking for the ones
				// that needed messages moved into them - perhaps instead we could
				// keep track of nsIMsgFolder, nsMsgKeyArray pairs here in the imap code.
//				nsMsgKeyArray *idsToMoveFromInbox = msgFolder->GetImapIdsToMoveFromInbox();
//				idsToMoveFromInbox->Add(keyToFilter);

				// this is our last best chance to log this
//				if (m_filterList->LoggingEnabled())
//					filter->LogRuleHit(GetLogFile(), mailHdr);

				if (imapDeleteIsMoveToTrash)
				{
				}
				
				msgFolder->SetFlag(MSG_FOLDER_FLAG_GOT_NEW);
				
				if (imapDeleteIsMoveToTrash)	
					err = 0;
			}
	 	}
	}
	
	
	// we have to return an error because we do not actually move the message
	// it is done async and that can fail
	return err;
}


// both of these algorithms assume that key arrays and flag states are sorted by increasing key.
void nsImapMailFolder::FindKeysToDelete(const nsMsgKeyArray &existingKeys, nsMsgKeyArray &keysToDelete, nsImapFlagAndUidState *flagState)
{
	PRBool imapDeleteIsMoveToTrash = DeleteIsMoveToTrash();
	PRUint32 total = existingKeys.GetSize();
	PRInt32 messageIndex;

	int onlineIndex=0; // current index into flagState
	for (PRUint32 keyIndex=0; keyIndex < total; keyIndex++)
	{
		PRUint32 uidOfMessage;

		flagState->GetNumberOfMessages(&messageIndex);
		while ((onlineIndex < messageIndex) && 
			   (flagState->GetUidOfMessage(onlineIndex, &uidOfMessage), (existingKeys[keyIndex] > uidOfMessage) ))
		{
			onlineIndex++;
		}
		
		imapMessageFlagsType flags;
		flagState->GetUidOfMessage(onlineIndex, &uidOfMessage);
		flagState->GetMessageFlags(onlineIndex, &flags);
		// delete this key if it is not there or marked deleted
		if ( (onlineIndex >= messageIndex ) ||
			 (existingKeys[keyIndex] != uidOfMessage) ||
			 ((flags & kImapMsgDeletedFlag) && imapDeleteIsMoveToTrash) )
		{
			nsMsgKey doomedKey = existingKeys[keyIndex];
			if ((PRInt32) doomedKey < 0 && doomedKey != nsMsgKey_None)
				continue;
			else
				keysToDelete.Add(existingKeys[keyIndex]);
		}
		
		flagState->GetUidOfMessage(onlineIndex, &uidOfMessage);
		if (existingKeys[keyIndex] == uidOfMessage) 
			onlineIndex++;
	}
}

void nsImapMailFolder::FindKeysToAdd(const nsMsgKeyArray &existingKeys, nsMsgKeyArray &keysToFetch, nsImapFlagAndUidState *flagState)
{
	PRBool showDeletedMessages = ShowDeletedMessages();

	int dbIndex=0; // current index into existingKeys
	PRInt32 existTotal, numberOfKnownKeys;
	PRInt32 messageIndex;
	
	existTotal = numberOfKnownKeys = existingKeys.GetSize();
	flagState->GetNumberOfMessages(&messageIndex);
	for (PRInt32 flagIndex=0; flagIndex < messageIndex; flagIndex++)
	{
		PRUint32 uidOfMessage;
		flagState->GetUidOfMessage(flagIndex, &uidOfMessage);
		while ( (flagIndex < numberOfKnownKeys) && (dbIndex < existTotal) &&
				existingKeys[dbIndex] < uidOfMessage) 
			dbIndex++;
		
		if ( (flagIndex >= numberOfKnownKeys)  || 
			 (dbIndex >= existTotal) ||
			 (existingKeys[dbIndex] != uidOfMessage ) )
		{
			numberOfKnownKeys++;

			imapMessageFlagsType flags;
			flagState->GetMessageFlags(flagIndex, &flags);
			if (showDeletedMessages || ! (flags & kImapMsgDeletedFlag))
			{
				keysToFetch.Add(uidOfMessage);
			}
		}
	}
}

void nsImapMailFolder::PrepareToAddHeadersToMailDB(nsIImapProtocol* aProtocol, const nsMsgKeyArray &keysToFetch,
                                                mailbox_spec *boxSpec)
{
    PRUint32 *theKeys = (PRUint32 *) PR_Malloc( keysToFetch.GetSize() * sizeof(PRUint32) );
    if (theKeys)
    {
		PRUint32 total = keysToFetch.GetSize();

        for (PRUint32 keyIndex=0; keyIndex < total; keyIndex++)
        	theKeys[keyIndex] = keysToFetch[keyIndex];
        
//        m_DownLoadState = kDownLoadingAllMessageHeaders;

        nsresult res = NS_OK; /*ImapMailDB::Open(m_pathName,
                                         PR_TRUE, // create if necessary
                                         &mailDB,
                                         m_master,
                                         &dbWasCreated); */

		// don't want to download headers in a composition pane
        if (NS_SUCCEEDED(res))
        {
#if 0
			SetParseMailboxState(new ParseIMAPMailboxState(m_master, m_host, this,
														   urlQueue,
														   boxSpec->flagState));
	        boxSpec->flagState = NULL;		// adopted by ParseIMAPMailboxState
			GetParseMailboxState()->SetPane(url_pane);

            GetParseMailboxState()->SetDB(mailDB);
            GetParseMailboxState()->SetIncrementalUpdate(PR_TRUE);
	        GetParseMailboxState()->SetMaster(m_master);
	        GetParseMailboxState()->SetContext(url_pane->GetContext());
	        GetParseMailboxState()->SetFolder(this);
	        
	        GetParseMailboxState()->BeginParsingFolder(0);
#endif // 0 hook up parsing later.
	        // the imap libnet module will start downloading message headers imap.h
			if (aProtocol)
				aProtocol->NotifyHdrsToDownload(theKeys, total /*keysToFetch.GetSize() */);
			// now, tell it we don't need any bodies.
			if (aProtocol)
				aProtocol->NotifyBodysToDownload(NULL, 0);
        }
        else
        {
			if (aProtocol)
				aProtocol->NotifyHdrsToDownload(NULL, 0);
        }
    }
}


void nsImapMailFolder::TweakHeaderFlags(nsIImapProtocol* aProtocol, nsIMsgDBHdr *tweakMe)
{
	if (mDatabase && aProtocol && tweakMe)
	{
		tweakMe->SetMessageKey(m_curMsgUid);
		tweakMe->SetMessageSize(m_nextMessageByteLength);
		
		PRBool foundIt = PR_FALSE;
		imapMessageFlagsType imap_flags;
		nsresult res = aProtocol->GetFlagsForUID(m_curMsgUid, &foundIt, &imap_flags);
		if (NS_SUCCEEDED(res) && foundIt)
		{
			// make a mask and clear these message flags
			PRUint32 mask = MSG_FLAG_READ | MSG_FLAG_REPLIED | MSG_FLAG_MARKED | MSG_FLAG_IMAP_DELETED;
			PRUint32 dbHdrFlags;

			tweakMe->GetFlags(&dbHdrFlags);
			tweakMe->AndFlags(~mask, &dbHdrFlags);
			
			// set the new value for these flags
			PRUint32 newFlags = 0;
			if (imap_flags & kImapMsgSeenFlag)
				newFlags |= MSG_FLAG_READ;
			else // if (imap_flags & kImapMsgRecentFlag)
				newFlags |= MSG_FLAG_NEW;

			// Okay here is the MDN needed logic (if DNT header seen):
			/* if server support user defined flag:
					MDNSent flag set => clear kMDNNeeded flag
					MDNSent flag not set => do nothing, leave kMDNNeeded on
			   else if 
					not MSG_FLAG_NEW => clear kMDNNeeded flag
					MSG_FLAG_NEW => do nothing, leave kMDNNeeded on
			 */
			PRUint16 userFlags;
			res = aProtocol->GetSupportedUserFlags(&userFlags);
			if (NS_SUCCEEDED(res) && (userFlags & (kImapMsgSupportUserFlag |
													  kImapMsgSupportMDNSentFlag)))
			{
				if (imap_flags & kImapMsgMDNSentFlag)
				{
					newFlags |= MSG_FLAG_MDN_REPORT_SENT;
					if (dbHdrFlags & MSG_FLAG_MDN_REPORT_NEEDED)
						tweakMe->AndFlags(~MSG_FLAG_MDN_REPORT_NEEDED, &dbHdrFlags);
				}
			}
			else
			{
				if (!(imap_flags & kImapMsgRecentFlag) && 
					dbHdrFlags & MSG_FLAG_MDN_REPORT_NEEDED)
					tweakMe->AndFlags(~MSG_FLAG_MDN_REPORT_NEEDED, &dbHdrFlags);
			}

			if (imap_flags & kImapMsgAnsweredFlag)
				newFlags |= MSG_FLAG_REPLIED;
			if (imap_flags & kImapMsgFlaggedFlag)
				newFlags |= MSG_FLAG_MARKED;
			if (imap_flags & kImapMsgDeletedFlag)
				newFlags |= MSG_FLAG_IMAP_DELETED;
			if (imap_flags & kImapMsgForwardedFlag)
				newFlags |= MSG_FLAG_FORWARDED;

			if (newFlags)
				tweakMe->OrFlags(newFlags, &dbHdrFlags);
		}
	}
}    

NS_IMETHODIMP
//nsImapMailFolder::SetupMsgWriteStream(nsIFileSpec * aFileSpec, PRBool addDummyEnvelope)
nsImapMailFolder::SetupMsgWriteStream(const char * aNativeString, PRBool addDummyEnvelope)
{
//    if (!aFileSpec)
//        return NS_ERROR_NULL_POINTER;
    nsFileSpec fileSpec (aNativeString);
//    aFileSpec->GetFileSpec(&fileSpec);
	fileSpec.Delete(PR_FALSE);
	nsCOMPtr<nsISupports>  supports;
	NS_NewIOFileStream(getter_AddRefs(supports), fileSpec, PR_WRONLY | PR_CREATE_FILE | PR_TRUNCATE, 00700);
	m_tempMessageStream = do_QueryInterface(supports);
    return NS_OK;
}

NS_IMETHODIMP 
nsImapMailFolder::ParseAdoptedMsgLine(const char *adoptedMessageLine, nsMsgKey uidOfMessage)
{
	PRUint32 count = 0;
	// remember the uid of the message we're downloading.
	m_curMsgUid = uidOfMessage;
	if (m_tempMessageStream)
	   m_tempMessageStream->Write(adoptedMessageLine, 
								  PL_strlen(adoptedMessageLine), &count);
                                                                                                                                                  
     return NS_OK;                                                               
}
    
NS_IMETHODIMP
nsImapMailFolder::NormalEndMsgWriteStream(nsMsgKey uidOfMessage)
{
	nsresult res = NS_OK;
	if (m_tempMessageStream)
		m_tempMessageStream->Close();

	nsCOMPtr<nsIMsgDBHdr> msgHdr;

	m_curMsgUid = uidOfMessage;
	res = GetMessageHeader(getter_AddRefs(msgHdr));
	if (NS_SUCCEEDED(res))
		msgHdr->MarkRead(PR_TRUE);

	return res;
}

NS_IMETHODIMP
nsImapMailFolder::AbortMsgWriteStream()
{
    return NS_ERROR_FAILURE;
}

nsresult nsImapMailFolder::GetMessageHeader(nsIMsgDBHdr ** aMsgHdr)
{
	nsresult rv = NS_OK;
	if (aMsgHdr)
	{
		rv = GetDatabase();
		// In theory, there shouldn't be contention over
		// m_curMsgUid, but it currently describes both the most
		// recent header we downloaded, and most recent message we've
		// downloaded. We may want to break this up.
		if (NS_SUCCEEDED(rv) && mDatabase) // did we get a db back?
			rv = mDatabase->GetMsgHdrForKey(m_curMsgUid, aMsgHdr);
	}
	else
		rv = NS_ERROR_NULL_POINTER;

	return rv;
}

    
    // message move/copy related methods
NS_IMETHODIMP 
nsImapMailFolder::OnlineCopyReport(ImapOnlineCopyState aCopyState)
{
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsImapMailFolder::BeginMessageUpload()
{
    return NS_ERROR_FAILURE;
}


    // message flags operation
NS_IMETHODIMP
nsImapMailFolder::NotifyMessageFlags(PRUint32 flags, nsMsgKey msgKey)
{
	if (NS_SUCCEEDED(GetDatabase()) && mDatabase)
    {
        mDatabase->MarkRead(msgKey, (flags & kImapMsgSeenFlag) != 0, nsnull);
        mDatabase->MarkReplied(msgKey, (flags & kImapMsgAnsweredFlag) != 0, nsnull);
        mDatabase->MarkMarked(msgKey, (flags & kImapMsgFlaggedFlag) != 0, nsnull);
    }
    return NS_OK;
}

NS_IMETHODIMP
nsImapMailFolder::NotifyMessageDeleted(const char *onlineFolderName,PRBool deleteAllMsgs, const char *msgIdString)
{
	const char *doomedKeyString = msgIdString;

	PRBool showDeletedMessages = ShowDeletedMessages();

	if (deleteAllMsgs)
	{
#ifdef HAVE_PORT		
		TNeoFolderInfoTransfer *originalInfo = NULL;
		nsIMsgDatabase *folderDB;
		if (ImapMailDB::Open(GetPathname(), PR_FALSE, &folderDB, GetMaster(), &wasCreated) == eSUCCESS)
		{
			originalInfo = new TNeoFolderInfoTransfer(*folderDB->m_neoFolderInfo);
			folderDB->ForceClosed();
		}
			
		// Remove summary file.
		XP_FileRemove(GetPathname(), xpMailFolderSummary);
		
		// Create a new summary file, update the folder message counts, and
		// Close the summary file db.
		if (ImapMailDB::Open(GetPathname(), PR_TRUE, &folderDB, GetMaster(), &wasCreated) == eSUCCESS)
		{
			if (originalInfo)
			{
				originalInfo->TransferFolderInfo(*folderDB->m_neoFolderInfo);
				delete originalInfo;
			}
			SummaryChanged();
			folderDB->Close();
		}
#endif
		// ### DMB - how to do this? Reload any thread pane because it's invalid now.
		return NS_OK;
	}

	char *keyTokenString = PL_strdup(doomedKeyString);
	nsMsgKeyArray affectedMessages;
	ParseUidString(keyTokenString, affectedMessages);

	if (doomedKeyString && !showDeletedMessages)
	{
		if (affectedMessages.GetSize() > 0)	// perhaps Search deleted these messages
		{
			GetDatabase();
			if (mDatabase)
				mDatabase->DeleteMessages(&affectedMessages, nsnull);
		}
		
	}
	else if (doomedKeyString)	// && !imapDeleteIsMoveToTrash
	{
		GetDatabase();
		if (mDatabase)
			SetIMAPDeletedFlag(mDatabase, affectedMessages, nsnull);
	}
	PR_FREEIF(keyTokenString);
	return NS_OK;
}

PRBool nsImapMailFolder::ShowDeletedMessages()
{
	nsresult err;
//	return (m_host->GetIMAPDeleteModel() == MSG_IMAPDeleteIsIMAPDelete);
    NS_WITH_SERVICE(nsIImapHostSessionList, hostSession,
                    kCImapHostSessionList, &err);
	PRBool rv = PR_FALSE;

    if (NS_SUCCEEDED(err) && hostSession)
	{
        char *serverKey = nsnull;
        GetServerKey(&serverKey);
        err = hostSession->GetShowDeletedMessagesForHost(serverKey, rv);
        PR_FREEIF(serverKey);
	}
	return rv;
}


PRBool nsImapMailFolder::DeleteIsMoveToTrash()
{
//	return (m_host->GetIMAPDeleteModel() == MSG_IMAPDeleteIsIMAPDelete);
	nsresult err;
    NS_WITH_SERVICE(nsIImapHostSessionList, hostSession,
                    kCImapHostSessionList, &err);
	PRBool rv = PR_TRUE;

    if (NS_SUCCEEDED(err) && hostSession)
	{
        char *serverKey = nsnull;
        GetServerKey(&serverKey);
        err = hostSession->GetDeleteIsMoveToTrashForHost(serverKey, rv);
        PR_FREEIF(serverKey);
	}
	return rv;
}

nsresult nsImapMailFolder::GetTrashFolder(nsIMsgFolder **pTrashFolder)
{
	if (!pTrashFolder)
		return NS_ERROR_NULL_POINTER;

	nsCOMPtr<nsIMsgFolder> rootFolder;
	nsresult rv = GetRootFolder(getter_AddRefs(rootFolder));
	if(NS_SUCCEEDED(rv))
	{
		PRUint32 numFolders;
		rv = rootFolder->GetFoldersWithFlag(MSG_FOLDER_FLAG_TRASH, pTrashFolder, 1, &numFolders);
		if (*pTrashFolder)
			NS_ADDREF(*pTrashFolder);
	}
	return rv;
}

void nsImapMailFolder::ParseUidString(char *uidString, nsMsgKeyArray &keys)
{
	// This is in the form <id>,<id>, or <id1>:<id2>
	char curChar = *uidString;
	PRBool isRange = PR_FALSE;
	int32	curToken;
	int32	saveStartToken=0;

	for (char *curCharPtr = uidString; curChar && *curCharPtr;)
	{
		char *currentKeyToken = curCharPtr;
		curChar = *curCharPtr;
		while (curChar != ':' && curChar != ',' && curChar != '\0')
			curChar = *curCharPtr++;
		*(curCharPtr - 1) = '\0';
		curToken = atoi(currentKeyToken);
		if (isRange)
		{
			while (saveStartToken < curToken)
				keys.Add(saveStartToken++);
		}
		keys.Add(curToken);
		isRange = (curChar == ':');
		if (isRange)
			saveStartToken = curToken + 1;
	}
}


// store MSG_FLAG_IMAP_DELETED in the specified mailhdr records
void nsImapMailFolder::SetIMAPDeletedFlag(nsIMsgDatabase *mailDB, const nsMsgKeyArray &msgids, PRBool markDeleted)
{
	nsresult markStatus = 0;
	PRUint32 total = msgids.GetSize();

	for (PRUint32 msgIndex=0; !markStatus && (msgIndex < total); msgIndex++)
	{
		markStatus = mailDB->MarkImapDeleted(msgids[msgIndex], markDeleted, nsnull);
	}
}


NS_IMETHODIMP
nsImapMailFolder::GetMessageSizeFromDB(const char *id, const char *folderName, PRBool idIsUid, PRUint32 *size)
{
	nsresult rv = NS_ERROR_FAILURE;
	if (id && mDatabase)
	{
		PRUint32 key = atoi(id);
		nsCOMPtr<nsIMsgDBHdr> mailHdr;
		NS_ASSERTION(idIsUid, "ids must be uids to get message size");
		if (idIsUid)
			rv = mDatabase->GetMsgHdrForKey(key, getter_AddRefs(mailHdr));
		if (NS_SUCCEEDED(rv) && mailHdr)
			rv = mailHdr->GetMessageSize(size);
	}
    return rv;
}

NS_IMETHODIMP
nsImapMailFolder::OnStartRunningUrl(nsIURI *aUrl)
{
	NS_PRECONDITION(aUrl, "just a sanity check since this is a test program");
	m_urlRunning = PR_TRUE;
    return NS_OK;
}

NS_IMETHODIMP
nsImapMailFolder::OnStopRunningUrl(nsIURI *aUrl, nsresult aExitCode)
{
	NS_PRECONDITION(aUrl, "just a sanity check since this is a test program");
	nsresult rv = NS_OK;
	m_urlRunning = PR_FALSE;
	if (aUrl)
	{
        nsCOMPtr<nsIImapUrl> imapUrl = do_QueryInterface(aUrl);
        if (imapUrl)
        {
            nsIImapUrl::nsImapAction imapAction = nsIImapUrl::nsImapTest;
            imapUrl->GetImapAction(&imapAction);
            switch(imapAction)
            {
            case nsIImapUrl::nsImapDeleteMsg:
            case nsIImapUrl::nsImapOnlineMove:
            case nsIImapUrl::nsImapOnlineCopy:
                if (m_copyState)
                {
                    if (NS_SUCCEEDED(aExitCode))
                    {
                        if (m_copyState->m_isMove)
                        {
                            nsCOMPtr<nsIMsgFolder> srcFolder;
                            srcFolder =
                                do_QueryInterface(m_copyState->m_srcSupport,
                                                  &rv);
                            nsCOMPtr<nsIMsgDatabase> srcDB;
                            if (NS_SUCCEEDED(rv))
                                rv = srcFolder->GetMsgDatabase(
                                    getter_AddRefs(srcDB));
                            if (NS_SUCCEEDED(rv) && srcDB)
                            {
                                nsCOMPtr<nsImapMoveCopyMsgTxn> msgTxn;
                                nsMsgKeyArray srcKeyArray;
                                msgTxn =
                                    do_QueryInterface(m_copyState->m_undoMsgTxn); 
                                if (msgTxn)
                                    msgTxn->GetSrcKeyArray(srcKeyArray);
                                srcDB->DeleteMessages(&srcKeyArray, nsnull);
                            }
                        }
                        if (m_transactionManager)
                            m_transactionManager->Do(m_copyState->m_undoMsgTxn);
                    }
                    ClearCopyState(aExitCode);
                }
                break;
            case nsIImapUrl::nsImapAppendMsgFromFile:
            case nsIImapUrl::nsImapAppendDraftFromFile:
                if (m_copyState)
                {
                    m_copyState->m_curIndex++;
                    if (m_copyState->m_curIndex >= m_copyState->m_totalCount)
                    {
                        if (m_transactionManager && m_copyState->m_undoMsgTxn)
                            m_transactionManager->Do(m_copyState->m_undoMsgTxn);
                        ClearCopyState(aExitCode);
                    }
                }
                break;
            default:
                break;
            }
        }
		// give base class a chance to send folder loaded notification...
		rv = nsMsgDBFolder::OnStopRunningUrl(aUrl, aExitCode);
		// query it for a mailnews interface for now....
		nsCOMPtr<nsIMsgMailNewsUrl> mailUrl = do_QueryInterface(aUrl);
		if (mailUrl)
			rv = mailUrl->UnRegisterListener(this);
	}
	return rv;
}

    // nsIImapExtensionSink methods

NS_IMETHODIMP
nsImapMailFolder::ClearFolderRights(nsIImapProtocol* aProtocol,
                                    nsIMAPACLRightsInfo* aclRights)
{
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsImapMailFolder::AddFolderRights(nsIImapProtocol* aProtocol,
                                  nsIMAPACLRightsInfo* aclRights)
{
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsImapMailFolder::RefreshFolderRights(nsIImapProtocol* aProtocol,
                                      nsIMAPACLRightsInfo* aclRights)
{
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsImapMailFolder::FolderNeedsACLInitialized(nsIImapProtocol* aProtocol,
                                            nsIMAPACLRightsInfo* aclRights)
{
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsImapMailFolder::SetCopyResponseUid(nsIImapProtocol* aProtocol,
                                     nsMsgKeyArray* aKeyArray,
                                     const char* msgIdString,
                                     nsISupports* copyState)
{   // CopyMessages() only
    nsresult rv = NS_OK;
    nsCOMPtr<nsImapMoveCopyMsgTxn> msgTxn;

    if (copyState)
    {
        nsCOMPtr<nsImapMailCopyState> mailCopyState =
            do_QueryInterface(copyState, &rv);
        if (NS_FAILED(rv)) return rv;
        if (mailCopyState->m_undoMsgTxn)
            msgTxn = do_QueryInterface(mailCopyState->m_undoMsgTxn, &rv);
    }
    if (msgTxn)
        msgTxn->SetCopyResponseUid(aKeyArray, msgIdString);
    
    return NS_OK;
}    

NS_IMETHODIMP
nsImapMailFolder::SetAppendMsgUid(nsIImapProtocol* aProtocol,
                                  nsMsgKey aKey,
                                  nsISupports* copyState)
{
    nsresult rv = NS_OK;
    if (copyState)
    {
        nsCOMPtr<nsImapMailCopyState> mailCopyState =
            do_QueryInterface(copyState, &rv);
        if (NS_FAILED(rv)) return rv;
        if (mailCopyState->m_undoMsgTxn) // CopyMessages()
        {
            //            nsImapMailCopyState* mailCopyState = 
            //                (nsImapMailCopyState*) copyState;
            nsCOMPtr<nsImapMoveCopyMsgTxn> msgTxn;
            msgTxn = do_QueryInterface(mailCopyState->m_undoMsgTxn, &rv);
            if (NS_SUCCEEDED(rv))
                msgTxn->AddDstKey(aKey);
        }
        else if (mailCopyState->m_listener) // CopyFileMessage();
                                            // Draft/Template goes here
            mailCopyState->m_listener->SetMessageKey(aKey);
    }
    return NS_OK;
}    

NS_IMETHODIMP
nsImapMailFolder::GetMessageId(nsIImapProtocol* aProtocl,
                               nsCString* messageId,
                               nsISupports* copyState)
{
    nsresult rv = NS_OK;
    if (copyState)
    {
        nsCOMPtr<nsImapMailCopyState> mailCopyState =
            do_QueryInterface(copyState, &rv);
        if (NS_FAILED(rv)) return rv;
        if (mailCopyState->m_listener)
            rv = mailCopyState->m_listener->GetMessageId(messageId);
    }
    return rv;
}
    // nsIImapMiscellaneousSink methods
NS_IMETHODIMP
nsImapMailFolder::AddSearchResult(nsIImapProtocol* aProtocol, 
                                  const char* searchHitLine)
{
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsImapMailFolder::GetArbitraryHeaders(nsIImapProtocol* aProtocol,
                                      GenericInfo* aInfo)
{
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsImapMailFolder::GetShouldDownloadArbitraryHeaders(nsIImapProtocol* aProtocol,
                                                    GenericInfo* aInfo)
{
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsImapMailFolder::GetShowAttachmentsInline(nsIImapProtocol* aProtocol,
                                           PRBool* aBool)
{
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsImapMailFolder::HeaderFetchCompleted(nsIImapProtocol* aProtocol)
{
	nsresult rv;
	if (mDatabase)
		mDatabase->Commit(nsMsgDBCommitType::kLargeCommit);
	if (m_moveCoalescer)
	{
		nsCOMPtr <nsIEventQueue> eventQ;
		NS_WITH_SERVICE(nsIEventQueueService, pEventQService, kEventQueueServiceCID, &rv); 
		if (NS_SUCCEEDED(rv) && pEventQService)
			pEventQService->GetThreadEventQueue(PR_GetCurrentThread(),
												getter_AddRefs(eventQ));
		m_moveCoalescer->PlaybackMoves (eventQ);
		delete m_moveCoalescer;
		m_moveCoalescer = nsnull;
	}
    return NS_OK;
}

NS_IMETHODIMP
nsImapMailFolder::UpdateSecurityStatus(nsIImapProtocol* aProtocol)
{
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsImapMailFolder::SetBiffStateAndUpdate(nsIImapProtocol* aProtocol,
                                        nsMsgBiffState biffState)
{
	SetBiffState(biffState);
    return NS_OK;
}

NS_IMETHODIMP
nsImapMailFolder::GetStoredUIDValidity(nsIImapProtocol* aProtocol,
                                       uid_validity_info* aInfo)
{
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsImapMailFolder::LiteSelectUIDValidity(nsIImapProtocol* aProtocol,
                                        PRUint32 uidValidity)
{
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsImapMailFolder::ProgressStatus(nsIImapProtocol* aProtocol,
                                 PRUint32 aMsgId, const char *extraInfo)
{
	PRUnichar *progressMsg = nsnull;

	nsCOMPtr<nsIMsgIncomingServer> server;
	nsresult rv = GetServer(getter_AddRefs(server));
	if (NS_SUCCEEDED(rv) && server)
	{
		nsCOMPtr<nsIImapServerSink> serverSink = do_QueryInterface(server);
		if (serverSink)
			serverSink->GetImapStringByID(aMsgId, &progressMsg);
	}
	if (!progressMsg)
		progressMsg = IMAPGetStringByID(aMsgId);

	if (aProtocol && progressMsg)
	{
		nsCOMPtr <nsIImapUrl> imapUrl;
		aProtocol->GetRunningImapURL(getter_AddRefs(imapUrl));
		if (imapUrl)
		{
			nsCOMPtr <nsIMsgMailNewsUrl> mailnewsUrl = do_QueryInterface(imapUrl);
			if (mailnewsUrl)
			{
				nsCOMPtr <nsIMsgStatusFeedback> feedback;
				mailnewsUrl->GetStatusFeedback(getter_AddRefs(feedback));

				if (extraInfo)
				{
					// lossy, but what can we do?
					nsCAutoString cProgressString(progressMsg);

					char *printfString = PR_smprintf(cProgressString, extraInfo);
					if (printfString)
					{
						nsString formattedString(printfString);
						PR_FREEIF(progressMsg);
						progressMsg = nsCRT::strdup(formattedString.GetUnicode());	
						PR_smprintf_free(printfString);

					}
				}
				if (feedback)
					feedback->ShowStatusString(progressMsg);
			}
		}
	}
	PR_FREEIF(progressMsg);

    return NS_OK;
}

NS_IMETHODIMP
nsImapMailFolder::PercentProgress(nsIImapProtocol* aProtocol,
                                  ProgressInfo* aInfo)
{
#ifdef DEBUG_bienvenu1
	nsCString message(aInfo->message);
	printf("progress: %d %s\n", aInfo->percent, message.GetBuffer());
#endif
	if (aProtocol)
	{
		nsCOMPtr <nsIImapUrl> imapUrl;
		aProtocol->GetRunningImapURL(getter_AddRefs(imapUrl));
		if (imapUrl)
		{
			nsCOMPtr <nsIMsgMailNewsUrl> mailnewsUrl = do_QueryInterface(imapUrl);
			if (mailnewsUrl)
			{
				nsCOMPtr <nsIMsgStatusFeedback> feedback;
				mailnewsUrl->GetStatusFeedback(getter_AddRefs(feedback));
				if (feedback && aInfo->message)
				{
					feedback->ShowProgress(aInfo->percent);
					feedback->ShowStatusString(aInfo->message);
				}
			}
		}
	}

    return NS_OK;
}

NS_IMETHODIMP
nsImapMailFolder::TunnelOutStream(nsIImapProtocol* aProtocol,
                                  msg_line_info* aInfo)
{
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsImapMailFolder::ProcessTunnel(nsIImapProtocol* aProtocol,
                                TunnelInfo *aInfo)
{
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsImapMailFolder::CopyNextStreamMessage(nsIImapProtocol* aProtocol,
                                        nsISupports* copyState)
{
    nsresult rv = NS_ERROR_NULL_POINTER;
    if (!copyState) return rv;
    nsCOMPtr<nsImapMailCopyState> mailCopyState = do_QueryInterface(copyState,
                                                                    &rv);
    if (NS_FAILED(rv)) return rv;

    if (!mailCopyState->m_streamCopy) return NS_OK;
    if (mailCopyState->m_curIndex < mailCopyState->m_totalCount)
    {
        nsCOMPtr<nsISupports> aSupport =
            getter_AddRefs(mailCopyState->m_messages->ElementAt
                           (mailCopyState->m_curIndex));
        mailCopyState->m_message = do_QueryInterface(aSupport,
                                                     &rv);
        if (NS_SUCCEEDED(rv))
        {
            rv = CopyStreamMessage(mailCopyState->m_message,
                                   this, mailCopyState->m_isMove);
        }
    }
    else if (mailCopyState->m_isMove)
    {
        nsCOMPtr<nsIMsgFolder> srcFolder =
            do_QueryInterface(mailCopyState->m_srcSupport, &rv);
        if (NS_SUCCEEDED(rv) && srcFolder)
        {
            srcFolder->DeleteMessages(mailCopyState->m_messages, nsnull,
                                      PR_FALSE);
        }
    }
    return rv;
}

NS_IMETHODIMP
nsImapMailFolder::SetUrlState(nsIImapProtocol* aProtocol,
                              nsIMsgMailNewsUrl* aUrl,
                              PRBool isRunning,
                              nsresult statusCode)
{
	if (!isRunning)
		ProgressStatus(aProtocol, IMAP_DONE, nsnull);

    if (aUrl)
        return aUrl->SetUrlState(isRunning, statusCode);
    return statusCode;
}

nsresult
nsImapMailFolder::CreateDirectoryForFolder(nsFileSpec &path) //** dup
{
	nsresult rv = NS_OK;

	if(!path.IsDirectory())
	{
		//If the current path isn't a directory, add directory separator
		//and test it out.
		rv = AddDirectorySeparator(path);
		if(NS_FAILED(rv))
			return rv;

		nsFileSpec tempPath(path.GetNativePathCString(), PR_TRUE);	// create incoming directories.

		//If that doesn't exist, then we have to create this directory
		if(!path.IsDirectory())
		{
			//If for some reason there's a file with the directory separator
			//then we are going to fail.
			if(path.Exists())
			{
				return NS_MSG_COULD_NOT_CREATE_DIRECTORY;
			}
			//otherwise we need to create a new directory.
			else
			{
				path.CreateDirectory();
				//Above doesn't return an error value so let's see if
				//it was created.
				if(!path.IsDirectory())
					return NS_MSG_COULD_NOT_CREATE_DIRECTORY;
			}
		}
	}

	return rv;
}

nsresult
nsImapMailFolder::CopyMessagesWithStream(nsIMsgFolder* srcFolder,
                                nsISupportsArray* messages,
                                PRBool isMove,
                                nsITransactionManager* txnMsg,
                                nsIMsgCopyServiceListener* listener)
{
    nsresult rv = NS_ERROR_NULL_POINTER;
    if (!srcFolder || !messages) return rv;

    nsCOMPtr<nsISupports> aSupport(do_QueryInterface(srcFolder, &rv));
    if (NS_FAILED(rv)) return rv;
    rv = InitCopyState(aSupport, messages, isMove, PR_TRUE, listener);
    if(NS_FAILED(rv)) return rv;

    m_copyState->m_streamCopy = PR_TRUE;

    // ** jt - needs to create server to server move/copy undo msg txn
    nsCString messageIds;
    nsMsgKeyArray srcKeyArray;
    nsCOMPtr<nsIUrlListener> urlListener;

	rv = QueryInterface(nsIUrlListener::GetIID(), getter_AddRefs(urlListener));
    rv = BuildIdsAndKeyArray(messages, messageIds, srcKeyArray);

    nsImapMoveCopyMsgTxn* undoMsgTxn = new nsImapMoveCopyMsgTxn(
        srcFolder, &srcKeyArray, messageIds.GetBuffer(), this,
        PR_TRUE, isMove, m_eventQueue, urlListener);

    rv = undoMsgTxn->QueryInterface(
        nsCOMTypeInfo<nsImapMoveCopyMsgTxn>::GetIID(), 
        getter_AddRefs(m_copyState->m_undoMsgTxn) );
    
    nsCOMPtr<nsISupports> msgSupport;
    msgSupport = getter_AddRefs(messages->ElementAt(0));
    if (msgSupport)
    {
        nsCOMPtr<nsIMessage> aMessage;
        aMessage = do_QueryInterface(msgSupport, &rv);
        if (NS_SUCCEEDED(rv))
            CopyStreamMessage(aMessage, this, isMove);
        else
            ClearCopyState(rv);
    }
    else
    {
        rv = NS_ERROR_FAILURE;
    }
    return rv;
}

NS_IMETHODIMP
nsImapMailFolder::CopyMessages(nsIMsgFolder* srcFolder,
                               nsISupportsArray* messages,
                               PRBool isMove,
                               nsITransactionManager* txnMgr,
                               nsIMsgCopyServiceListener* listener)
{
    nsresult rv = NS_OK;
    char *uri = nsnull;
    char *hostname1 = nsnull, *hostname2 = nsnull, *username1 = nsnull,
        *username2 = nsnull;
    nsAutoString protocolType;
    nsCString messageIds;
    nsMsgKeyArray srcKeyArray;
    nsCOMPtr<nsIUrlListener> urlListener;
    nsCOMPtr<nsISupports> srcSupport;
    nsCOMPtr<nsISupports> copySupport;

    SetTransactionManager(txnMgr);

    NS_WITH_SERVICE(nsIImapService, imapService, kCImapService, &rv);

    if (!srcFolder || !messages) return NS_ERROR_NULL_POINTER;
    rv = srcFolder->GetURI(&uri);
    if (NS_FAILED(rv)) goto done;
    rv = nsURI2ProtocolType(uri, protocolType);
    if (NS_FAILED(rv)) goto done;
    if (!protocolType.EqualsIgnoreCase("imap"))
    {
        rv = CopyMessagesWithStream(srcFolder, messages, isMove, txnMgr, listener);
        goto done;
    }
    rv = srcFolder->GetHostname(&hostname1);
    if (NS_FAILED(rv)) goto done;
    rv = srcFolder->GetUsername(&username1);
    if (NS_FAILED(rv)) goto done;
    rv = GetHostname(&hostname2);
    if(NS_FAILED(rv)) goto done;
    rv = GetUsername(&username2);
    if (NS_FAILED(rv)) goto done;
    if (PL_strcmp(hostname1, hostname2) || // *** different server or account
        PL_strcmp(username1, username2))   // do stream base copy
    {
        rv = CopyMessagesWithStream(srcFolder, messages, isMove, txnMgr, listener);
        goto done;
    }

    rv = BuildIdsAndKeyArray(messages, messageIds, srcKeyArray);
    if(NS_FAILED(rv)) goto done;

    srcSupport = do_QueryInterface(srcFolder);
	rv = QueryInterface(nsIUrlListener::GetIID(), getter_AddRefs(urlListener));

    rv = InitCopyState(srcSupport, messages, isMove, PR_TRUE, listener);
    if (NS_FAILED(rv)) goto done;

    m_copyState->m_curIndex = m_copyState->m_totalCount;

    copySupport = do_QueryInterface(m_copyState);
    if (imapService)
        rv = imapService->OnlineMessageCopy(m_eventQueue,
                                            srcFolder, messageIds.GetBuffer(),
                                            this, PR_TRUE, isMove,
                                            urlListener, nsnull,
                                            copySupport);
    if (NS_SUCCEEDED(rv))
    {
        nsImapMoveCopyMsgTxn* undoMsgTxn = new nsImapMoveCopyMsgTxn(
            srcFolder, &srcKeyArray, messageIds.GetBuffer(), this,
            PR_TRUE, isMove, m_eventQueue, urlListener);
        rv = undoMsgTxn->QueryInterface(
            nsCOMTypeInfo<nsImapMoveCopyMsgTxn>::GetIID(), 
            getter_AddRefs(m_copyState->m_undoMsgTxn) );
    }

done:
    
    PR_FREEIF(uri);
    PR_FREEIF(username1);
    PR_FREEIF(username2);
    PR_FREEIF(hostname1);
    PR_FREEIF(hostname2);
    return rv;
}

nsresult
nsImapMailFolder::SetTransactionManager(nsITransactionManager* txnMgr)
{
    nsresult rv = NS_OK;
    if (txnMgr && !m_transactionManager)
        m_transactionManager = do_QueryInterface(txnMgr, &rv);
    return rv;
}

NS_IMETHODIMP
nsImapMailFolder::CopyFileMessage(nsIFileSpec* fileSpec,
                                  nsIMessage* msgToReplace,
                                  PRBool isDraftOrTemplate,
                                  nsITransactionManager* txnMgr,
                                  nsIMsgCopyServiceListener* listener)
{
    nsresult rv = NS_ERROR_NULL_POINTER;
    nsMsgKey key = 0xffffffff;
    nsCString messageId;
    nsCOMPtr<nsIUrlListener> urlListener;
    nsCOMPtr<nsISupports> srcSupport;
    nsCOMPtr<nsISupportsArray> messages;

    if (!fileSpec) return rv;

    srcSupport = do_QueryInterface(fileSpec, &rv);
    if (NS_FAILED(rv)) return rv;

    rv = NS_NewISupportsArray(getter_AddRefs(messages));
    if (NS_FAILED(rv)) return rv;

    NS_WITH_SERVICE(nsIImapService, imapService, kCImapService, &rv);
    if (NS_FAILED(rv)) return rv;

    rv = QueryInterface(nsIUrlListener::GetIID(), getter_AddRefs(urlListener));

    if (msgToReplace)
    {
        rv = msgToReplace->GetMessageKey(&key);
        if (NS_SUCCEEDED(rv))
            messageId.Append((PRInt32) key);
    }

    rv = InitCopyState(srcSupport, messages, PR_FALSE, isDraftOrTemplate,
                       listener);
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsISupports> copySupport;
    if( m_copyState ) 
        copySupport = do_QueryInterface(m_copyState);
    rv = imapService->AppendMessageFromFile(m_eventQueue, fileSpec, this,
                                            messageId.GetBuffer(),
                                            PR_TRUE, isDraftOrTemplate,
                                            urlListener, nsnull,
                                            copySupport);
    if (NS_SUCCEEDED(rv))
        imapService->SelectFolder(m_eventQueue, this, this, nsnull, nsnull);

    return rv;
}

nsresult 
nsImapMailFolder::CopyStreamMessage(nsIMessage* message,
                                    nsIMsgFolder* dstFolder, // should be this
                                    PRBool isMove)
{
    nsresult rv = NS_ERROR_NULL_POINTER;
    if (!m_copyState) return rv;

    nsCOMPtr<nsICopyMessageStreamListener> copyStreamListener;

    rv = nsComponentManager::CreateInstance(kCopyMessageStreamListenerCID,
               NULL, nsICopyMessageStreamListener::GetIID(),
			   getter_AddRefs(copyStreamListener)); 
	if(NS_FAILED(rv))
		return rv;

    nsCOMPtr<nsICopyMessageListener>
        copyListener(do_QueryInterface(dstFolder, &rv));
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIMsgFolder>
        srcFolder(do_QueryInterface(m_copyState->m_srcSupport, &rv));
    if (NS_FAILED(rv)) return rv;
    rv = copyStreamListener->Init(srcFolder, copyListener, nsnull);
    if (NS_FAILED(rv)) return rv;
       
    nsCOMPtr<nsIRDFResource> messageNode(do_QueryInterface(message));
    if (!messageNode) return NS_ERROR_FAILURE;
    char *uri;
    messageNode->GetValue(&uri);

    if (!m_copyState->m_msgService)
    {
        rv = GetMessageServiceFromURI(uri, &m_copyState->m_msgService);
    }

    if (NS_SUCCEEDED(rv) && m_copyState->m_msgService)
    {
        nsIURI * url = nsnull;
		nsCOMPtr<nsIStreamListener>
            streamListener(do_QueryInterface(copyStreamListener, &rv));
		if(NS_FAILED(rv) || !streamListener)
			return NS_ERROR_NO_INTERFACE;

        rv = m_copyState->m_msgService->CopyMessage(uri, streamListener,
                                                     isMove, nsnull, &url);
	}
    return rv;
}

nsImapMailCopyState::nsImapMailCopyState() : m_msgService(nsnull),
    m_isMove(PR_FALSE), m_selectedState(PR_FALSE), m_curIndex(0),
    m_totalCount(0), m_streamCopy(PR_FALSE), m_dataBuffer(nsnull)
{
    NS_INIT_REFCNT();
}

nsImapMailCopyState::~nsImapMailCopyState()
{
    PR_FREEIF(m_dataBuffer);
    if (m_msgService && m_message)
    {
        nsCOMPtr<nsIRDFResource> msgNode(do_QueryInterface(m_message));
        if (msgNode)
        {
            char* uri;
            msgNode->GetValue(&uri);
            if (uri)
                ReleaseMessageServiceFromURI(uri, m_msgService);
        }
    }
    if (m_tmpFileSpec)
    {
        PRBool isOpen = PR_FALSE;
        nsFileSpec  fileSpec;
        if (isOpen)
            m_tmpFileSpec->CloseStream();
        m_tmpFileSpec->GetFileSpec(&fileSpec);
        if (fileSpec.Valid())
            fileSpec.Delete(PR_FALSE);
    }
}

NS_IMPL_ADDREF(nsImapMailCopyState)
NS_IMPL_RELEASE(nsImapMailCopyState)

NS_IMETHODIMP 
nsImapMailCopyState::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
  if (NULL == aInstancePtr) {
    return NS_ERROR_NULL_POINTER;
  }

  *aInstancePtr = NULL;

  static NS_DEFINE_IID(kClassIID, nsImapMailCopyState::GetIID());
  if (aIID.Equals(kClassIID)) {
    *aInstancePtr = (void*) this;
    NS_ADDREF_THIS();
    return NS_OK;
  }               
  if (aIID.Equals(nsCOMTypeInfo<nsISupports>::GetIID())) {
    *aInstancePtr = (void*) ((nsISupports*)this);
    NS_ADDREF_THIS();
    return NS_OK;
  }
  return NS_NOINTERFACE;
}

nsresult
nsImapMailFolder::InitCopyState(nsISupports* srcSupport,
                                nsISupportsArray* messages,
                                PRBool isMove,
                                PRBool selectedState,
                                nsIMsgCopyServiceListener* listener)
{
    nsresult rv = NS_ERROR_NULL_POINTER;

    if (!srcSupport || !messages) return rv;
    if (m_copyState) return NS_ERROR_FAILURE;

    nsImapMailCopyState* copyState = new nsImapMailCopyState();
    m_copyState = do_QueryInterface(copyState);

    if (!m_copyState) return NS_ERROR_OUT_OF_MEMORY;

    if (srcSupport)
        m_copyState->m_srcSupport = do_QueryInterface(srcSupport, &rv);

    if (NS_SUCCEEDED(rv))
    {
        m_copyState->m_messages = do_QueryInterface(messages, &rv);
        if (NS_SUCCEEDED(rv))
            rv = messages->Count(&m_copyState->m_totalCount);
    }
    m_copyState->m_isMove = isMove;
    m_copyState->m_selectedState = selectedState;

    if (listener)
        m_copyState->m_listener = do_QueryInterface(listener, &rv);
        
    return rv;
}

void
nsImapMailFolder::ClearCopyState(nsresult rv)
{
    if (m_copyState)
    {
        nsresult result;
        NS_WITH_SERVICE(nsIMsgCopyService, copyService, 
                        kMsgCopyServiceCID, &result); 
        if (NS_SUCCEEDED(result))
            copyService->NotifyCompletion(m_copyState->m_srcSupport, this, rv);
      
        m_copyState = null_nsCOMPtr();
    }
}

NS_IMETHODIMP nsImapMailFolder::MatchName(nsString *name, PRBool *matches)
{
	if (!matches)
		return NS_ERROR_NULL_POINTER;
    PRBool isInbox = mName.EqualsIgnoreCase("inbox");
	*matches = mName.Equals(*name, isInbox);
	return NS_OK;
}

NS_IMETHODIMP NS_NewImapMailFolder(nsISupports * aOuter, REFNSIID iid, void ** aResult)
{
  if (!aResult) return NS_ERROR_NULL_POINTER;

  if (aOuter)
  {
      *aResult = nsnull;
      return NS_ERROR_NO_AGGREGATION;
  }

  nsImapMailFolder *imapFolder = new nsImapMailFolder();
  if (!imapFolder) return NS_ERROR_OUT_OF_MEMORY;
  return imapFolder->QueryInterface(iid, aResult); 
}
