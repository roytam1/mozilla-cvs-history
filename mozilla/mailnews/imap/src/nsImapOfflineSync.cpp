/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 2001 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

#include "msgCore.h"
#include "nsImapOfflineSync.h"
#include "nsImapMailFolder.h"
#include "nsMsgFolderFlags.h"
#include "nsXPIDLString.h"
#include "nsIRDFService.h"
#include "nsMsgBaseCID.h"
#include "nsRDFCID.h"
#include "nsIMsgAccountManager.h"
#include "nsINntpIncomingServer.h"

static NS_DEFINE_CID(kMsgAccountManagerCID, NS_MSGACCOUNTMANAGER_CID);
static NS_DEFINE_CID(kRDFServiceCID, NS_RDFSERVICE_CID);

NS_IMPL_ISUPPORTS1(nsImapOfflineSync, nsIUrlListener)

nsImapOfflineSync::nsImapOfflineSync(nsIMsgWindow *window, nsIMsgFolder *singleFolderOnly)
{
  NS_INIT_REFCNT();
  m_singleFolderToUpdate = singleFolderOnly;
  m_window = window;
  mCurrentPlaybackOpType = nsIMsgOfflineImapOperation::kFlagsChanged;
  m_mailboxupdatesStarted = PR_FALSE;
  m_createdOfflineFolders = PR_FALSE;
  m_pseudoOffline = PR_FALSE;
  m_KeyIndex = 0;
  mCurrentUIDValidity = nsMsgKey_None;
}

nsImapOfflineSync::~nsImapOfflineSync()
{
}

void      nsImapOfflineSync::SetWindow(nsIMsgWindow *window)
{
  m_window = window;
}

NS_IMETHODIMP nsImapOfflineSync::OnStartRunningUrl(nsIURI* url)
{
    return NS_OK;
}

NS_IMETHODIMP
nsImapOfflineSync::OnStopRunningUrl(nsIURI* url, nsresult exitCode)
{
  nsresult rv = exitCode;
  if (NS_SUCCEEDED(exitCode))
    rv = ProcessNextOperation();

  return rv;
}

// leaves m_currentServer at the next imap or local mail "server" that
// might have offline events to playback. If no more servers,
// m_currentServer will be left at nsnull.
// Also, sets up m_serverEnumerator to enumerate over the server
nsresult nsImapOfflineSync::AdvanceToNextServer()
{
  nsresult rv;

  if (!m_allServers)
  {
    NS_WITH_SERVICE(nsIMsgAccountManager, accountManager,
                      NS_MSGACCOUNTMANAGER_CONTRACTID, &rv);
    NS_ASSERTION(accountManager && NS_SUCCEEDED(rv), "couldn't get account mgr");
    if (!accountManager || NS_FAILED(rv)) return rv;

    rv = accountManager->GetAllServers(getter_AddRefs(m_allServers));
    NS_ENSURE_SUCCESS(rv, rv);
  }
  PRUint32 serverIndex = (m_currentServer) ? m_allServers->IndexOf(m_currentServer) + 1 : 0;
  m_currentServer = nsnull;
  PRUint32 numServers; 
  m_allServers->Count(&numServers);
  nsCOMPtr <nsIFolder> rootFolder;

  while (serverIndex < numServers)
  {
    nsISupports* serverSupports = m_allServers->ElementAt(serverIndex);
    serverIndex++;

    nsCOMPtr<nsIMsgIncomingServer> server = do_QueryInterface(serverSupports);
    NS_RELEASE(serverSupports);
    nsCOMPtr <nsINntpIncomingServer> newsServer = do_QueryInterface(server);
    if (newsServer) // news servers aren't involved in offline imap
      continue;
    if (server)
    {
      m_currentServer = server;
      server->GetRootFolder(getter_AddRefs(rootFolder));
      if (rootFolder)
      {
        NS_NewISupportsArray(getter_AddRefs(m_allFolders));
        rv = rootFolder->ListDescendents(m_allFolders);
        if (NS_SUCCEEDED(rv))
          m_allFolders->Enumerate(getter_AddRefs(m_serverEnumerator));
        if (NS_SUCCEEDED(rv) && m_serverEnumerator)
        {
          rv = m_serverEnumerator->First();
          if (NS_SUCCEEDED(rv))
            break;
        }
      }
    }
  }
  return rv;
}

nsresult nsImapOfflineSync::AdvanceToNextFolder()
{
  nsresult rv;
	// we always start by changing flags
  mCurrentPlaybackOpType = nsIMsgOfflineImapOperation::kFlagsChanged;
	
  m_currentFolder = nsnull;

  if (!m_currentServer)
     rv = AdvanceToNextServer();
  else
    rv = m_serverEnumerator->Next();
  if (!NS_SUCCEEDED(rv))
    rv = AdvanceToNextServer();

  if (NS_SUCCEEDED(rv))
  {
    // ### argh, this doesn't go into sub-folders of each folder.
    nsCOMPtr <nsISupports> supports;
    rv = m_serverEnumerator->CurrentItem(getter_AddRefs(supports));
    m_currentFolder = do_QueryInterface(supports);
  }
  return rv;
}

void nsImapOfflineSync::AdvanceToFirstIMAPFolder()
{
  m_currentServer = nsnull;
  nsresult rv = AdvanceToNextFolder();
  nsCOMPtr <nsIMsgImapMailFolder> imapFolder;
  do
  {
    if (m_currentFolder)
      imapFolder = do_QueryInterface(m_currentFolder);
  }
	while (m_currentFolder && !imapFolder);
}

void nsImapOfflineSync::ProcessFlagOperation(nsIMsgOfflineImapOperation *currentOp)
{
	nsMsgKeyArray matchingFlagKeys;
	PRUint32 currentKeyIndex = m_KeyIndex;
	imapMessageFlagsType matchingFlags;
  currentOp->GetNewFlags(&matchingFlags);
  imapMessageFlagsType flagOperation;
  imapMessageFlagsType newFlags;
	
	do
  {	// loop for all messsages with the same flags
    nsMsgKey curKey;
    currentOp->GetMessageKey(&curKey);
		matchingFlagKeys.Add(curKey);
    currentOp->ClearOperation(nsIMsgOfflineImapOperation::kFlagsChanged);
    currentOp = nsnull;
    if (++currentKeyIndex < m_CurrentKeys.GetSize())
      m_currentDB->GetOfflineOpForKey(m_CurrentKeys[currentKeyIndex], PR_FALSE,
        &currentOp);
    if (currentOp)
    {
      currentOp->GetFlagOperation(&flagOperation);
      currentOp->GetNewFlags(&newFlags);
    }
	} while (currentOp && (flagOperation & nsIMsgOfflineImapOperation::kFlagsChanged) && (newFlags == matchingFlags) );
	
  currentOp = nsnull;
	
  if (matchingFlagKeys.GetSize() > 0)
  {
    nsCAutoString uids;
	  nsImapMailFolder::AllocateUidStringFromKeyArray(matchingFlagKeys, uids);
    PRUint32 curFolderFlags;
    m_currentFolder->GetFlags(&curFolderFlags);

	  if (uids.get() && (curFolderFlags & MSG_FOLDER_FLAG_IMAPBOX)) 
	  {
	    nsresult rv = NS_OK;
      nsCOMPtr <nsIMsgImapMailFolder> imapFolder = do_QueryInterface(m_currentFolder);
      nsCOMPtr <nsIURI> uriToSetFlags;
      if (imapFolder)
      {
        rv = imapFolder->SetImapFlags(uids.get(), matchingFlags, getter_AddRefs(uriToSetFlags));
        if (NS_SUCCEEDED(rv) && uriToSetFlags)
        {
          nsCOMPtr <nsIMsgMailNewsUrl> mailnewsUrl = do_QueryInterface(uriToSetFlags);
          if (mailnewsUrl)
            mailnewsUrl->RegisterListener(this);
        }
      }

	  }
  }
  else
    ProcessNextOperation();
}

void
nsImapOfflineSync::ProcessAppendMsgOperation(nsIMsgOfflineImapOperation *currentOp, PRInt32 opType)
{
	nsCOMPtr <nsIMsgDBHdr> mailHdr;
  nsMsgKey msgKey;
  currentOp->GetMessageKey(&msgKey);
  nsresult rv = m_currentDB->GetMsgHdrForKey(msgKey, getter_AddRefs(mailHdr)); 
#ifdef NOT_IMPL_YET
	if (NS_SUCCEEDED(rv) && mailHdr)
	{
		char *msg_file_name = WH_TempName (xpFileToPost, "nsmail");
		if (msg_file_name)
		{
			XP_File msg_file = XP_FileOpen(msg_file_name, xpFileToPost,
										   XP_FILE_WRITE_BIN);
			if (msg_file)
			{
				mailHdr->WriteOfflineMessage(msg_file, m_currentDB->GetDB(), PR_FALSE);
				XP_FileClose(msg_file);
				nsCAutoString moveDestination;
				currentOp->GetMoveDestination(moveDestination);
				MSG_IMAPFolderInfoMail *currentIMAPFolder = m_currentFolder->GetIMAPFolderInfoMail();

				MSG_IMAPFolderInfoMail *mailFolderInfo = currentIMAPFolder
					? m_workerPane->GetMaster()->FindImapMailFolder(currentIMAPFolder->GetHostName(), moveDestination, nsnsnull, PR_FALSE)
					: m_workerPane->GetMaster()->FindImapMailFolder(moveDestination);
				char *urlString = 
					CreateImapAppendMessageFromFileUrl(
						mailFolderInfo->GetHostName(),
						mailFolderInfo->GetOnlineName(),
						mailFolderInfo->GetOnlineHierarchySeparator(),
						opType == kAppendDraft);
				if (urlString)
				{
					URL_Struct *url = NET_CreateURLStruct(urlString,
														  NET_NORMAL_RELOAD); 
					if (url)
					{
						url->post_data = XP_STRDUP(msg_file_name);
						url->post_data_size = XP_STRLEN(msg_file_name);
						url->post_data_is_file = PR_TRUE;
						url->method = URL_POST_METHOD;
						url->fe_data = (void *) new
							AppendMsgOfflineImapState(
								mailFolderInfo,
								currentOp->GetMessageKey(), msg_file_name);
						url->internal_url = PR_TRUE;
						url->msg_pane = m_workerPane;
						m_workerPane->GetContext()->imapURLPane = m_workerPane;
						MSG_UrlQueue::AddUrlToPane (url,
													PostAppendMsgExitFunction,
													m_workerPane, PR_TRUE);
						currentOp->ClearAppendMsgOperation(opType);
					}
					XP_FREEIF(urlString);
				}
			}
			XP_FREEIF(msg_file_name);
		}
		mailHdr->unrefer();
	}
#endif // NOT_IMPL_YET
}


void nsImapOfflineSync::ProcessMoveOperation(nsIMsgOfflineImapOperation *currentOp)
{
	nsMsgKeyArray matchingFlagKeys ;
	PRUint32 currentKeyIndex = m_KeyIndex;
	nsXPIDLCString moveDestination;
	currentOp->GetDestinationFolderURI(getter_Copies(moveDestination));
	PRBool moveMatches = PR_TRUE;
	
	do 
  {	// loop for all messsages with the same destination
		if (moveMatches)
		{
      nsMsgKey curKey;
      currentOp->GetMessageKey(&curKey);
			matchingFlagKeys.Add(curKey);
      currentOp->ClearOperation(nsIMsgOfflineImapOperation::kMsgMoved);
		}
		currentOp = nsnull;
		
		if (++currentKeyIndex < m_CurrentKeys.GetSize())
		{
			nsXPIDLCString nextDestination;
			nsresult rv = m_currentDB->GetOfflineOpForKey(m_CurrentKeys[currentKeyIndex], PR_FALSE, &currentOp);
      moveMatches = PR_FALSE;
			if (NS_SUCCEEDED(rv) && currentOp)
      {
        nsOfflineImapOperationType opType; 
        currentOp->GetOperation(&opType);
        if (opType & nsIMsgOfflineImapOperation::kMsgMoved)
        {
          currentOp->GetDestinationFolderURI(getter_Copies(nextDestination));
          moveMatches = nsCRT::strcmp(moveDestination, nextDestination) == 0;
        }
      }
		}
	} 
  while (currentOp);
	
  nsCAutoString uids;
	nsImapMailFolder::AllocateUidStringFromKeyArray(matchingFlagKeys, uids);

  nsresult rv;

  nsCOMPtr<nsIRDFResource> res;
  NS_WITH_SERVICE(nsIRDFService, rdf, kRDFServiceCID, &rv);
  if (NS_FAILED(rv)) return ; // ### return error code.
  rv = rdf->GetResource(moveDestination, getter_AddRefs(res));
  if (NS_SUCCEEDED(rv))
  {
    nsCOMPtr<nsIMsgFolder> destFolder(do_QueryInterface(res, &rv));
    if (NS_SUCCEEDED(rv) && destFolder)
    {
      nsCOMPtr <nsIMsgImapMailFolder> imapFolder = do_QueryInterface(m_currentFolder);
      if (imapFolder)
        rv = imapFolder->ReplayOfflineMoveCopy(uids.get(), PR_TRUE, destFolder,
                       this, m_window);
    }
	}
}


void nsImapOfflineSync::ProcessCopyOperation(nsIMsgOfflineImapOperation *currentOp)
{
	nsMsgKeyArray matchingFlagKeys;
	PRUint32 currentKeyIndex = m_KeyIndex;
	nsXPIDLCString copyDestination;
	currentOp->GetCopyDestination(0, getter_Copies(copyDestination));
	PRBool copyMatches = PR_TRUE;
	
	do {	// loop for all messsages with the same destination
			if (copyMatches)
			{
        nsMsgKey curKey;
        currentOp->GetMessageKey(&curKey);
			  matchingFlagKeys.Add(curKey);
        currentOp->ClearOperation(nsIMsgOfflineImapOperation::kMsgCopy);
			}
			currentOp = nsnull;
			
		if (++currentKeyIndex < m_CurrentKeys.GetSize())
		{
			nsXPIDLCString nextDestination;
			nsresult rv = m_currentDB->GetOfflineOpForKey(m_CurrentKeys[currentKeyIndex], PR_FALSE, &currentOp);
      copyMatches = PR_FALSE;
			if (NS_SUCCEEDED(rv) && currentOp)
      {
        nsOfflineImapOperationType opType; 
        currentOp->GetOperation(&opType);
        if (opType & nsIMsgOfflineImapOperation::kMsgCopy)
        {
        	currentOp->GetCopyDestination(0, getter_Copies(copyDestination));
          copyMatches = nsCRT::strcmp(copyDestination, nextDestination) == 0;
        }
      }
		}
	} 
  while (currentOp);
	
  nsCAutoString uids;
	nsImapMailFolder::AllocateUidStringFromKeyArray(matchingFlagKeys, uids);

  nsresult rv;

  nsCOMPtr<nsIRDFResource> res;
  NS_WITH_SERVICE(nsIRDFService, rdf, kRDFServiceCID, &rv);
  if (NS_FAILED(rv)) return ; // ### return error code.
  rv = rdf->GetResource(copyDestination, getter_AddRefs(res));
  if (NS_SUCCEEDED(rv))
  {
    nsCOMPtr<nsIMsgFolder> destFolder(do_QueryInterface(res, &rv));
    if (NS_SUCCEEDED(rv) && destFolder)
    {
      nsCOMPtr <nsIMsgImapMailFolder> imapFolder = do_QueryInterface(destFolder);
      if (imapFolder)
        rv = imapFolder->ReplayOfflineMoveCopy(uids.get(), PR_FALSE, destFolder,
                       this, m_window);
    }
	}
}

void nsImapOfflineSync::ProcessEmptyTrash(nsIMsgOfflineImapOperation *currentOp)
{
#ifdef NOT_IMPL_YET
	currentOp->unrefer();
	MSG_IMAPFolderInfoMail *currentIMAPFolder = m_currentFolder->GetIMAPFolderInfoMail();
	char *trashUrl = CreateImapDeleteAllMessagesUrl(currentIMAPFolder->GetHostName(), 
		                                            currentIMAPFolder->GetOnlineName(),
		                                            currentIMAPFolder->GetOnlineHierarchySeparator());
	// we're not going to delete sub-folders, since that prompts the user, a no-no while synchronizing.
	if (trashUrl)
	{
		queue->AddUrl(trashUrl, OfflineOpExitFunction);
		if (!alreadyRunningQueue)
			queue->GetNextUrl();	
		m_currentDB->DeleteOfflineOp(currentOp->GetMessageKey());

		m_currentDB = nsnull;	// empty trash deletes the database?
	}
#endif // NOT_IMPL_YET
}

// returns PR_TRUE if we found a folder to create, PR_FALSE if we're done creating folders.
PRBool nsImapOfflineSync::CreateOfflineFolders()
{
	while (m_currentFolder)
	{
		PRUint32 flags;
    m_currentFolder->GetFlags(&flags);
		PRBool offlineCreate = (flags & MSG_FOLDER_FLAG_CREATED_OFFLINE) != 0;
		if (offlineCreate)
		{
			if (CreateOfflineFolder(m_currentFolder))
				return PR_TRUE;
		}
		AdvanceToNextFolder();
	}
	return PR_FALSE;
}

PRBool nsImapOfflineSync::CreateOfflineFolder(nsIMsgFolder *folder)
{
  nsCOMPtr<nsIFolder> parent;
  folder->GetParent(getter_AddRefs(parent));

  nsCOMPtr <nsIMsgImapMailFolder> imapFolder = do_QueryInterface(parent);
  nsCOMPtr <nsIURI> createFolderURI;
   nsXPIDLCString onlineName;
  imapFolder->GetOnlineName(getter_Copies(onlineName));

  NS_ConvertASCIItoUCS2 folderName(onlineName);
//  folderName.AssignWithConversion(onlineName);
	nsresult rv = imapFolder->PlaybackOfflineFolderCreate(folderName.get(), nsnull,  getter_AddRefs(createFolderURI));
  if (createFolderURI && NS_SUCCEEDED(rv))
  {
    nsCOMPtr <nsIMsgMailNewsUrl> mailnewsUrl = do_QueryInterface(createFolderURI);
    if (mailnewsUrl)
      mailnewsUrl->RegisterListener(this);
  }
  return NS_SUCCEEDED(rv) ? PR_TRUE : PR_FALSE;	// this is asynch, we have to return and be called again by the OfflineOpExitFunction
}

// Playing back offline operations is one giant state machine that runs through ProcessNextOperation.
// The first state is creating online any folders created offline (we do this first, so we can play back
// any operations in them in the next pass)

nsresult nsImapOfflineSync::ProcessNextOperation()
{
  nsresult rv;
	// find a folder that needs to process operations
	nsIMsgFolder	*deletedAllOfflineEventsInFolder = nsnull;

	// if we haven't created offline folders, and we're updating all folders,
	// first, find offline folders to create.
	if (!m_createdOfflineFolders)
	{
		if (m_singleFolderToUpdate)
		{
			if (!m_pseudoOffline)
			{
				AdvanceToFirstIMAPFolder();
				if (CreateOfflineFolders())
					return NS_OK;
			}	
		}
		else
		{
			if (CreateOfflineFolders())
				return NS_OK;
			AdvanceToFirstIMAPFolder();
		}
		m_createdOfflineFolders = PR_TRUE;
	}
	// if updating one folder only, restore m_currentFolder to that folder
	if (m_singleFolderToUpdate)
		m_currentFolder = m_singleFolderToUpdate;

  PRUint32 folderFlags;
  nsCOMPtr <nsIDBFolderInfo> folderInfo;
	while (m_currentFolder && !m_currentDB)
	{
    m_currentFolder->GetFlags(&folderFlags);
		// need to check if folder has offline events, or is configured for offline
		if (folderFlags & (MSG_FOLDER_FLAG_OFFLINEEVENTS | MSG_FOLDER_FLAG_OFFLINE))
		{
      m_currentFolder->GetDBFolderInfoAndDB(getter_AddRefs(folderInfo), getter_AddRefs(m_currentDB));
		}
		if (m_currentDB)
		{
			m_CurrentKeys.RemoveAll();
			m_KeyIndex = 0;
			if ((m_currentDB->ListAllOfflineOpIds(&m_CurrentKeys) != 0) || !m_CurrentKeys.GetSize())
			{
				m_currentDB = nsnull;
        m_currentFolder->ClearFlag(MSG_FOLDER_FLAG_OFFLINEEVENTS);
			}
			else
			{
				// trash any ghost msgs
				PRBool deletedGhostMsgs = PR_FALSE;
				for (PRUint32 fakeIndex=0; fakeIndex < m_CurrentKeys.GetSize(); fakeIndex++)
				{
					nsCOMPtr <nsIMsgOfflineImapOperation> currentOp; 
          m_currentDB->GetOfflineOpForKey(m_CurrentKeys[fakeIndex], PR_FALSE, getter_AddRefs(currentOp));
					if (currentOp)
          {
            nsOfflineImapOperationType opType; 
            currentOp->GetOperation(&opType);

            if (opType == nsIMsgOfflineImapOperation::kMoveResult)
					  {
						  m_currentDB->RemoveOfflineOp(currentOp);
						  deletedGhostMsgs = PR_TRUE;
						  
						  nsCOMPtr <nsIMsgDBHdr> mailHdr;
              nsMsgKey curKey;
              currentOp->GetMessageKey(&curKey);
              m_currentDB->DeleteMessage(curKey, nsnull, PR_FALSE);
					  }
          }
				}
				
				if (deletedGhostMsgs)
					m_currentFolder->SummaryChanged();
				
				m_CurrentKeys.RemoveAll();
				if ( (m_currentDB->ListAllOfflineOpIds(&m_CurrentKeys) != 0) || !m_CurrentKeys.GetSize() )
				{
					m_currentDB = nsnull;
					if (deletedGhostMsgs)
						deletedAllOfflineEventsInFolder = m_currentFolder;
				}
				else if (folderFlags & MSG_FOLDER_FLAG_IMAPBOX)
				{
//					if (imapFolder->GetHasOfflineEvents())
//						XP_ASSERT(PR_FALSE);

					if (!m_pseudoOffline)	// if pseudo offline, falls through to playing ops back.
					{
						// there are operations to playback so check uid validity
						SetCurrentUIDValidity(0);	// force initial invalid state
            // ### do a lite select here and hook ourselves up as a listener.
  					return NS_OK;	// this is asynch, we have to return as be called again by the OfflineOpExitFunction
					}
				}
			}
		}
		
		if (!m_currentDB)
		{
				// only advance if we are doing all folders
			if (!m_singleFolderToUpdate)
				AdvanceToNextFolder();
			else
				m_currentFolder = nsnull;	// force update of this folder now.
		}
			
	}
	
  if (m_currentFolder)
    m_currentFolder->GetFlags(&folderFlags);
	// do the current operation
	if (m_currentDB)
	{	
		PRBool currentFolderFinished = PR_FALSE;
    if (!folderInfo)
      m_currentDB->GetDBFolderInfo(getter_AddRefs(folderInfo));
														// user canceled the lite select! if GetCurrentUIDValidity() == 0
		if ((m_KeyIndex < m_CurrentKeys.GetSize()) && (m_pseudoOffline || (GetCurrentUIDValidity() != 0) || !(folderFlags & MSG_FOLDER_FLAG_IMAPBOX)) )
		{
      PRInt32 curFolderUidValidity;
      folderInfo->GetImapUidValidity(&curFolderUidValidity);
			PRBool uidvalidityChanged = (!m_pseudoOffline && folderFlags & MSG_FOLDER_FLAG_IMAPBOX) && (GetCurrentUIDValidity() != curFolderUidValidity);
			nsIMsgOfflineImapOperation *currentOp = nsnull;
			if (uidvalidityChanged)
				DeleteAllOfflineOpsForCurrentDB();
			else
			  m_currentDB->GetOfflineOpForKey(m_CurrentKeys[m_KeyIndex], PR_FALSE, &currentOp);
			
			if (currentOp)
			{
        nsOfflineImapOperationType opType; 

        if (currentOp)
          currentOp->GetOperation(&opType);
				// loop until we find the next db record that matches the current playback operation
				while (currentOp && !(opType & mCurrentPlaybackOpType))
        {
          currentOp = nsnull;
          ++m_KeyIndex;
					if (m_KeyIndex < m_CurrentKeys.GetSize())
						m_currentDB->GetOfflineOpForKey(m_CurrentKeys[m_KeyIndex], PR_FALSE, &currentOp);
          if (currentOp)
            currentOp->GetOperation(&opType);
        }
				
				// if we did not find a db record that matches the current playback operation,
				// then move to the next playback operation and recurse.  
				if (!currentOp)
				{
					// we are done with the current type
					if (mCurrentPlaybackOpType == nsIMsgOfflineImapOperation::kFlagsChanged)
					{
						mCurrentPlaybackOpType = nsIMsgOfflineImapOperation::kMsgCopy;
						// recurse to deal with next type of operation
						m_KeyIndex = 0;
						ProcessNextOperation();
					}
					else if (mCurrentPlaybackOpType == nsIMsgOfflineImapOperation::kMsgCopy)
					{
						mCurrentPlaybackOpType = nsIMsgOfflineImapOperation::kMsgMoved;
						// recurse to deal with next type of operation
						m_KeyIndex = 0;
						ProcessNextOperation();
					}
					else if (mCurrentPlaybackOpType == nsIMsgOfflineImapOperation::kMsgMoved)
					{
						mCurrentPlaybackOpType = nsIMsgOfflineImapOperation::kAppendDraft;
						// recurse to deal with next type of operation
						m_KeyIndex = 0;
						ProcessNextOperation();
					}
					else if (mCurrentPlaybackOpType == nsIMsgOfflineImapOperation::kAppendDraft)
					{
						mCurrentPlaybackOpType = nsIMsgOfflineImapOperation::kAppendTemplate;
						// recurse to deal with next type of operation
						m_KeyIndex = 0;
						ProcessNextOperation();
					}
					else if (mCurrentPlaybackOpType == nsIMsgOfflineImapOperation::kAppendTemplate)
					{
						mCurrentPlaybackOpType = nsIMsgOfflineImapOperation::kDeleteAllMsgs;
						m_KeyIndex = 0;
						ProcessNextOperation();
					}
					else
					{
						DeleteAllOfflineOpsForCurrentDB();
						currentFolderFinished = PR_TRUE;
					}
					
				}
				else
				{
					if (mCurrentPlaybackOpType == nsIMsgOfflineImapOperation::kFlagsChanged)
						ProcessFlagOperation(currentOp);
					else if (mCurrentPlaybackOpType == nsIMsgOfflineImapOperation::kMsgCopy)
						ProcessCopyOperation(currentOp);
					else if (mCurrentPlaybackOpType == nsIMsgOfflineImapOperation::kMsgMoved)
						ProcessMoveOperation(currentOp);
					else if (mCurrentPlaybackOpType == nsIMsgOfflineImapOperation::kAppendDraft)
						ProcessAppendMsgOperation(currentOp, nsIMsgOfflineImapOperation::kAppendDraft);
					else if (mCurrentPlaybackOpType == nsIMsgOfflineImapOperation::kAppendTemplate)
						ProcessAppendMsgOperation(currentOp, nsIMsgOfflineImapOperation::kAppendTemplate);
					else if (mCurrentPlaybackOpType == nsIMsgOfflineImapOperation::kDeleteAllMsgs)
						ProcessEmptyTrash(currentOp);
					else
						NS_ASSERTION(PR_FALSE, "invalid playback op type");
					// currentOp was unreferred by one of the Process functions
					// so do not reference it again!
					currentOp = nsnull;
				}
			}
			else
				currentFolderFinished = PR_TRUE;
		}
		else
			currentFolderFinished = PR_TRUE;
			
		if (currentFolderFinished)
		{
			m_currentDB = nsnull;
			if (!m_singleFolderToUpdate)
			{
				AdvanceToNextFolder();
				ProcessNextOperation();
				return NS_OK;
			}
			else
				m_currentFolder = nsnull;
		}
	}
	
	if (!m_currentFolder && !m_mailboxupdatesStarted)
	{
		m_mailboxupdatesStarted = PR_TRUE;
		
		// if we are updating more than one folder then we need the iterator
		if (!m_singleFolderToUpdate)
			AdvanceToFirstIMAPFolder();
    if (m_singleFolderToUpdate)
    {
      m_singleFolderToUpdate->ClearFlag(MSG_FOLDER_FLAG_OFFLINEEVENTS);
			m_singleFolderToUpdate->UpdateFolder(m_window);
      // do we have to do anything? Old code would do a start update...
    }
    else
    {
			// this means that we are updating all of the folders.  Update the INBOX first so the updates on the remaining
			// folders pickup the results of any filter moves.
//			nsIMsgFolder *inboxFolder;
			if (!m_pseudoOffline )
			{
 	      NS_WITH_SERVICE(nsIMsgAccountManager, accountManager, kMsgAccountManagerCID, &rv);
	      if (NS_FAILED(rv)) return rv;
	      nsCOMPtr<nsISupportsArray> servers;
	      
	      rv = accountManager->GetAllServers(getter_AddRefs(servers));
	      if (NS_FAILED(rv)) return rv;
        // ### for each imap server, call get new messages.
      // get next folder...
      }
    }

//		MSG_FolderIterator *updateFolderIterator = m_singleFolderToUpdate ? (MSG_FolderIterator *) 0 : m_folderIterator;
		
			
			// we are done playing commands back, now queue up the sync with each imap folder
			// If we're using the iterator, m_currentFolder will be set correctly
//			nsIMsgFolder * folder = m_singleFolderToUpdate ? m_singleFolderToUpdate : m_currentFolder;
//			while (folder)
//			{            
//				PRBool loadingFolder = m_workerPane->GetLoadingImapFolder() == folder;
//				if ((folder->GetType() == FOLDER_IMAPMAIL) && (deletedAllOfflineEventsInFolder == folder || (folder->GetFolderPrefFlags() & MSG_FOLDER_FLAG_OFFLINE)
//					|| loadingFolder) 
//					&& !(folder->GetFolderPrefFlags() & MSG_FOLDER_PREF_IMAPNOSELECT) )
//				{
//					PRBool lastChance = ((deletedAllOfflineEventsInFolder == folder) && m_singleFolderToUpdate) || loadingFolder;
					// if deletedAllOfflineEventsInFolder == folder and we're only updating one folder, then we need to update newly selected folder
					// I think this also means that we're really opening the folder...so we tell StartUpdate that we're loading a folder.
//					if (!updateFolderIterator || !(imapMail->GetFlags() & MSG_FOLDER_FLAG_INBOX))		// avoid queueing the inbox twice
//						imapMail->StartUpdateOfNewlySelectedFolder(m_workerPane, lastChance, queue, nsnsnull, PR_FALSE, PR_FALSE);
//				}
//				folder= m_singleFolderToUpdate ? (MSG_FolderInfo *)nsnull : updateFolderIterator->Next();
//       }
	}
  return rv;
}


void nsImapOfflineSync::DeleteAllOfflineOpsForCurrentDB()
{
	m_KeyIndex = 0;
	nsCOMPtr <nsIMsgOfflineImapOperation> currentOp;
  m_currentDB->GetOfflineOpForKey(m_CurrentKeys[m_KeyIndex], PR_FALSE, getter_AddRefs(currentOp));
	while (currentOp)
	{
//		NS_ASSERTION(currentOp->GetOperationFlags() == 0);
		// delete any ops that have already played back
		m_currentDB->RemoveOfflineOp(currentOp);
		currentOp = nsnull;
		
		if (++m_KeyIndex < m_CurrentKeys.GetSize())
			m_currentDB->GetOfflineOpForKey(m_CurrentKeys[m_KeyIndex], PR_FALSE, getter_AddRefs(currentOp));
	}
	// turn off MSG_FOLDER_PREF_OFFLINEEVENTS
	if (m_currentFolder)
		m_currentFolder->ClearFlag(MSG_FOLDER_FLAG_OFFLINEEVENTS);
}
