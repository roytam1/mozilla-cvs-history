/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
 * Copyright (C) 1998, 1999 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 *   Pierre Phaneuf <pp@ludusdesign.com>
 */

#include "msgCore.h"

#include "nsMsgImapCID.h"
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
#include "nsMsgBaseCID.h"
#include "nsMsgLocalCID.h"
#include "nsImapUndoTxn.h"
#include "nsIIMAPHostSessionList.h"
#include "nsIMsgCopyService.h"
#include "nsICopyMsgStreamListener.h"
#include "nsImapStringBundle.h"
#include "nsIMsgFolderCacheElement.h"
#include "nsTextFormatter.h"
#include "nsIPref.h"

#include "nsIMsgFilter.h"
#include "nsImapMoveCoalescer.h"
#include "nsIPrompt.h"
#include "nsIDocShell.h"
#include "nsIInterfaceRequestor.h"
#include "nsSpecialSystemDirectory.h"
#include "nsXPIDLString.h"
#include "nsIImapFlagAndUidState.h"
#include "nsIMessenger.h"
#include "nsIMsgSearchAdapter.h"
#include "nsIImapMockChannel.h"
#include "nsIProgressEventSink.h"
#include "nsIMsgWindow.h"
#include "nsIMsgFolder.h" // TO include biffState enum. Change to bool later...
#include "nsIMsgOfflineImapOperation.h"
#include "nsImapOfflineSync.h"
#include "nsIMsgAccountManager.h"
#include "nsQuickSort.h"
#include "nsIImapMockChannel.h"

static NS_DEFINE_CID(kMsgAccountManagerCID, NS_MSGACCOUNTMANAGER_CID);
static NS_DEFINE_CID(kRDFServiceCID, NS_RDFSERVICE_CID);
static NS_DEFINE_CID(kCMailDB, NS_MAILDB_CID);
static NS_DEFINE_CID(kImapProtocolCID, NS_IMAPPROTOCOL_CID);
static NS_DEFINE_CID(kCImapDB, NS_IMAPDB_CID);
static NS_DEFINE_CID(kCImapService, NS_IMAPSERVICE_CID);
static NS_DEFINE_CID(kEventQueueServiceCID, NS_EVENTQUEUESERVICE_CID);
static NS_DEFINE_CID(kMsgMailSessionCID, NS_MSGMAILSESSION_CID);
static NS_DEFINE_CID(kParseMailMsgStateCID, NS_PARSEMAILMSGSTATE_CID);
static NS_DEFINE_CID(kCImapHostSessionList, NS_IIMAPHOSTSESSIONLIST_CID);
static NS_DEFINE_CID(kMsgCopyServiceCID,    NS_MSGCOPYSERVICE_CID);
static NS_DEFINE_CID(kCopyMessageStreamListenerCID, NS_COPYMESSAGESTREAMLISTENER_CID);
static NS_DEFINE_CID(kPrefCID, NS_PREF_CID);


#define FOUR_K 4096

/*
    Copies the contents of srcDir into destDir.
    destDir will be created if it doesn't exist.
*/

static
nsresult RecursiveCopy(nsIFile* srcDir, nsIFile* destDir)
{
    nsresult rv;
    PRBool isDir;
    
    rv = srcDir->IsDirectory(&isDir);
    if (NS_FAILED(rv)) return rv;
	if (!isDir) return NS_ERROR_INVALID_ARG;

    PRBool exists;
    rv = destDir->Exists(&exists);
	if (NS_SUCCEEDED(rv) && !exists)
		rv = destDir->Create(nsIFile::DIRECTORY_TYPE, 0775);
	if (NS_FAILED(rv)) return rv;

    PRBool hasMore = PR_FALSE;
    nsCOMPtr<nsISimpleEnumerator> dirIterator;
    rv = srcDir->GetDirectoryEntries(getter_AddRefs(dirIterator));
    if (NS_FAILED(rv)) return rv;
    
    rv = dirIterator->HasMoreElements(&hasMore);
    if (NS_FAILED(rv)) return rv;
    
    nsCOMPtr<nsIFile> dirEntry;
    
	while (hasMore)
	{
		rv = dirIterator->GetNext((nsISupports**)getter_AddRefs(dirEntry));
		if (NS_SUCCEEDED(rv))
		{
		    rv = dirEntry->IsDirectory(&isDir);
			if (NS_SUCCEEDED(rv))
		    {
		        if (isDir)
		        {
		            nsCOMPtr<nsIFile> destClone;
		            rv = destDir->Clone(getter_AddRefs(destClone));
		            if (NS_SUCCEEDED(rv))
		            {
		                nsCOMPtr<nsILocalFile> newChild(do_QueryInterface(destClone));
		                nsXPIDLCString leafName;
		                dirEntry->GetLeafName(getter_Copies(leafName));
		                newChild->AppendRelativePath(leafName);
						rv = newChild->Exists(&exists);
			            if (NS_SUCCEEDED(rv) && !exists)
				             rv = newChild->Create(nsIFile::DIRECTORY_TYPE, 0775);
		                rv = RecursiveCopy(dirEntry, newChild);
		            }
		        }
		        else
		            rv = dirEntry->CopyTo(destDir, nsnull);
		    }
		
		}
        rv = dirIterator->HasMoreElements(&hasMore);
        if (NS_FAILED(rv)) return rv;
	}

	return rv;
}

nsImapMailFolder::nsImapMailFolder() :
    m_initialized(PR_FALSE),m_haveDiscoveredAllFolders(PR_FALSE),
    m_haveReadNameFromDB(PR_FALSE), 
    m_curMsgUid(0), m_nextMessageByteLength(0),
    m_urlRunning(PR_FALSE),
  m_verifiedAsOnlineFolder(PR_FALSE),
  m_explicitlyVerify(PR_FALSE),
    m_folderNeedsSubscribing(PR_FALSE),
    m_folderNeedsAdded(PR_FALSE),
    m_folderNeedsACLListed(PR_TRUE),
    m_downloadMessageForOfflineUse(PR_FALSE),
    m_downloadingFolderForOfflineUse(PR_FALSE)
{
    m_appendMsgMonitor = nsnull;  // since we're not using this (yet?) make it null.
                // if we do start using it, it should be created lazily

  nsresult rv;

    // Get current thread envent queue

  NS_WITH_SERVICE(nsIEventQueueService, pEventQService, kEventQueueServiceCID, &rv); 
    if (NS_SUCCEEDED(rv) && pEventQService)
        pEventQService->GetThreadEventQueue(NS_CURRENT_THREAD,
                                            getter_AddRefs(m_eventQueue));
  m_moveCoalescer = nsnull;
  m_boxFlags = 0;
  m_hierarchyDelimiter = kOnlineHierarchySeparatorUnknown;
  m_pathName = nsnull;
}

nsImapMailFolder::~nsImapMailFolder()
{
    if (m_appendMsgMonitor)
        PR_DestroyMonitor(m_appendMsgMonitor);

  if (m_moveCoalescer)
    delete m_moveCoalescer;
}

NS_IMPL_ADDREF_INHERITED(nsImapMailFolder, nsMsgDBFolder)
NS_IMPL_RELEASE_INHERITED(nsImapMailFolder, nsMsgDBFolder)
NS_IMPL_QUERY_HEAD(nsImapMailFolder)
    NS_IMPL_QUERY_BODY(nsIMsgImapMailFolder)
    NS_IMPL_QUERY_BODY(nsICopyMessageListener)
    NS_IMPL_QUERY_BODY(nsIImapMailFolderSink)
    NS_IMPL_QUERY_BODY(nsIImapMessageSink)
    NS_IMPL_QUERY_BODY(nsIImapExtensionSink)
    NS_IMPL_QUERY_BODY(nsIImapMiscellaneousSink)
    NS_IMPL_QUERY_BODY(nsIUrlListener)
    NS_IMPL_QUERY_BODY(nsIMsgFilterHitNotify)
    NS_IMPL_QUERY_BODY(nsIStreamListener)
NS_IMPL_QUERY_TAIL_INHERITING(nsMsgDBFolder)


NS_IMETHODIMP nsImapMailFolder::Enumerate(nsIEnumerator* *result)
{
#if 0
    nsresult rv = NS_OK;
    nsIEnumerator* folders;
    nsIEnumerator* messages;
    rv = GetSubFolders(&folders);
    if (NS_FAILED(rv)) return rv;
    rv = GetMessages(nsnull, &messages);
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
      nsAutoString str; str.AssignWithConversion(NS_STATIC_CAST(nsFilePath, path));
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

NS_IMETHODIMP nsImapMailFolder::AddSubfolderWithPath(nsAutoString *name, nsIFileSpec *dbPath, 
                                             nsIMsgFolder **child)
{
  if(!child)
    return NS_ERROR_NULL_POINTER;

  nsresult rv = NS_OK;
  NS_WITH_SERVICE(nsIRDFService, rdf, kRDFServiceCID, &rv); 

  if(NS_FAILED(rv))
    return rv;

    PRInt32 flags = 0;
  nsAutoString uri;
  uri.AppendWithConversion(mURI);
  uri.AppendWithConversion('/');

  uri.Append(*name);
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

  folder->SetPath(dbPath);
    nsCOMPtr<nsIMsgImapMailFolder> imapFolder = do_QueryInterface(folder);

    folder->GetFlags((PRUint32 *)&flags);

    folder->SetParent(this);
  nsMemory::Free(uriStr);

  flags |= MSG_FOLDER_FLAG_MAIL;

  PRBool isServer;
    rv = GetIsServer(&isServer);

  //Only set these is these are top level children.
  if(NS_SUCCEEDED(rv) && isServer)
  {
    if(name->EqualsIgnoreCase(NS_ConvertASCIItoUCS2("Inbox")))
      flags |= MSG_FOLDER_FLAG_INBOX;
    else if(name->EqualsIgnoreCase(nsAutoString(kTrashName)))
      flags |= MSG_FOLDER_FLAG_TRASH;
#if 0
    else if(name->EqualsIgnoreCase(kSentName))
      folder->SetFlag(MSG_FOLDER_FLAG_SENTMAIL);
    else if(name->EqualsIgnoreCase(kDraftsName))
      folder->SetFlag(MSG_FOLDER_FLAG_DRAFTS);
    else if (name->EqualsIgnoreCase(kTemplatesName));
      folder->SetFlag(MSG_FOLDER_FLAG_TEMPLATES);
#endif 
  }

    folder->SetFlags(flags);
  //at this point we must be ok and we don't want to return failure in case GetIsServer failed.
  rv = NS_OK;

  nsCOMPtr <nsISupports> supports = do_QueryInterface(folder);
  NS_ASSERTION(supports, "couldn't get isupports from imap folder");
  if (supports)
    mSubFolders->AppendElement(supports);
  *child = folder;
  NS_IF_ADDREF(*child);
  return rv;
}

nsresult nsImapMailFolder::CreateSubFolders(nsFileSpec &path)
{
  nsresult rv = NS_OK;
  nsAutoString currentFolderNameStr;    // online name
  nsAutoString currentFolderDBNameStr;  // possibly munged name
  nsCOMPtr<nsIMsgFolder> child;
  nsCOMPtr<nsIMsgIncomingServer> server;
  nsCOMPtr<nsIImapIncomingServer> imapServer;

  if (NS_SUCCEEDED(GetServer(getter_AddRefs(server))) && server)
    imapServer = do_QueryInterface(server);

  PRBool isServer;
  rv = GetIsServer(&isServer);

  char *folderName;
  for (nsDirectoryIterator dir(path, PR_FALSE); dir.Exists(); dir++) 
  {
    nsFileSpec currentFolderPath = dir.Spec();
    folderName = currentFolderPath.GetLeafName();
    currentFolderNameStr.AssignWithConversion(folderName);
    if (isServer && imapServer)
    {
      PRBool isPFC;
      imapServer->GetIsPFC(folderName, &isPFC);
      if (isPFC)
      {
        nsCOMPtr <nsIMsgFolder> pfcFolder;
        imapServer->GetPFC(PR_TRUE, getter_AddRefs(pfcFolder));
        continue;
      }
      // should check if this is the PFC
    }
    if (nsShouldIgnoreFile(currentFolderNameStr))
    {
      PL_strfree(folderName);
      continue;
    }

       // OK, here we need to get the online name from the folder cache if we can.
    // If we can, use that to create the sub-folder

    nsCOMPtr <nsIMsgFolderCacheElement> cacheElement;
    nsCOMPtr <nsIFileSpec> curFolder;
    nsCOMPtr <nsIFileSpec> dbFile;

    NS_NewFileSpecWithSpec(currentFolderPath, getter_AddRefs(dbFile));
    // don't strip off the .msf in currentFolderPath.
    currentFolderPath.SetLeafName(currentFolderNameStr);
    rv = NS_NewFileSpecWithSpec(currentFolderPath, getter_AddRefs(curFolder));

    currentFolderDBNameStr = currentFolderNameStr;
    nsAutoString utf7LeafName = currentFolderNameStr;

    if (NS_SUCCEEDED(rv) && curFolder)
    {
      rv = GetFolderCacheElemFromFileSpec(dbFile, getter_AddRefs(cacheElement));

      if (NS_SUCCEEDED(rv) && cacheElement)
      {
        nsXPIDLString unicodeName;
        nsXPIDLCString onlineFullUtf7Name;

        rv = cacheElement->GetStringProperty("onlineName", getter_Copies(onlineFullUtf7Name));
        if (NS_SUCCEEDED(rv) && (const char *) onlineFullUtf7Name && nsCRT::strlen((const char *) onlineFullUtf7Name))
        {
          if (imapServer)

            imapServer->CreatePRUnicharStringFromUTF7(onlineFullUtf7Name, getter_Copies(unicodeName));


          // take the full unicode folder name and find the unicode leaf name.
          currentFolderNameStr.Assign(unicodeName);

          PRUnichar delimiter = 0;
          GetHierarchyDelimiter(&delimiter);
          PRInt32 leafPos = currentFolderNameStr.RFindChar(delimiter);
          if (leafPos > 0)
            currentFolderNameStr.Cut(0, leafPos + 1);

          // take the utf7 full online name, and determine the utf7 leaf name
          utf7LeafName.AssignWithConversion(onlineFullUtf7Name);
          leafPos = utf7LeafName.RFindChar(delimiter);
          if (leafPos > 0)
            utf7LeafName.Cut(0, leafPos + 1);
        }
      }
    }
      // make the imap folder remember the file spec it was created with.
    nsCAutoString leafName; leafName.AssignWithConversion(currentFolderDBNameStr);
    nsCOMPtr <nsIFileSpec> msfFileSpec;
    rv = NS_NewFileSpecWithSpec(currentFolderPath, getter_AddRefs(msfFileSpec));
    if (NS_SUCCEEDED(rv) && msfFileSpec)
    {
      // leaf name is the db name w/o .msf (nsShouldIgnoreFile strips it off)
      // so this trims the .msf off the file spec.
      msfFileSpec->SetLeafName(leafName);
    }
    // use the utf7 name as the uri for the folder.
    AddSubfolderWithPath(&utf7LeafName, msfFileSpec, getter_AddRefs(child));
    if (child)
    {
      // use the unicode name as the "pretty" name. Set it so it won't be
      // automatically computed from the URI, which is in utf7 form.
      if (currentFolderNameStr.Length() > 0)
        child->SetName(currentFolderNameStr.GetUnicode());

    }
    PL_strfree(folderName);
    }
  return rv;
}

NS_IMETHODIMP nsImapMailFolder::GetSubFolders(nsIEnumerator* *result)
{
  PRBool isServer;
  nsresult rv = GetIsServer(&isServer);

  if (!m_initialized)
  {
    nsCOMPtr<nsIFileSpec> pathSpec;
    rv = GetPath(getter_AddRefs(pathSpec));
    if (NS_FAILED(rv)) return rv;

    nsFileSpec path;
    rv = pathSpec->GetFileSpec(&path);
    if (NS_FAILED(rv)) return rv;

    // host directory does not need .sbd tacked on
    if (NS_SUCCEEDED(rv) && !isServer)
      rv = AddDirectorySeparator(path);

    if(NS_FAILED(rv)) return rv;
    
    m_initialized = PR_TRUE;      // need to set this here to avoid infinite recursion from CreateSubfolders.
    // we have to treat the root folder specially, because it's name
    // doesn't end with .sbd

    PRInt32 newFlags = MSG_FOLDER_FLAG_MAIL;
    if (path.IsDirectory()) 
    {
        newFlags |= (MSG_FOLDER_FLAG_DIRECTORY | MSG_FOLDER_FLAG_ELIDED);
        if (!mIsServer)
          SetFlag(newFlags);
        rv = CreateSubFolders(path);
    }
    if (isServer)
    {
      PRUint32 numFolders = 0;
      nsCOMPtr <nsIMsgFolder> inboxFolder;

      rv = GetFoldersWithFlag(MSG_FOLDER_FLAG_INBOX, 1, &numFolders, getter_AddRefs(inboxFolder));
      if (!NS_SUCCEEDED(rv) || numFolders == 0 || !inboxFolder)
      {
        // create an inbox if we don't have one.
        CreateClientSubfolderInfo("INBOX", kOnlineHierarchySeparatorUnknown,0);
      }
    }
    UpdateSummaryTotals(PR_FALSE);

    if (NS_FAILED(rv)) return rv;
  }
  rv = mSubFolders->Enumerate(result);
  return rv;
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

//Makes sure the database is open and exists.  If the database is valid then
//returns NS_OK.  Otherwise returns a failure error value.
nsresult nsImapMailFolder::GetDatabase(nsIMsgWindow *aMsgWindow)
{
  nsresult folderOpen = NS_OK;
  if (!mDatabase)
  {
    nsCOMPtr<nsIFileSpec> pathSpec;
    nsresult rv = GetPath(getter_AddRefs(pathSpec));
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIMsgDatabase> mailDBFactory;

    rv = nsComponentManager::CreateInstance(kCImapDB, nsnull, NS_GET_IID(nsIMsgDatabase), (void **) getter_AddRefs(mailDBFactory));
    if (NS_SUCCEEDED(rv) && mailDBFactory)
      folderOpen = mailDBFactory->OpenFolderDB(this, PR_TRUE, PR_TRUE, getter_AddRefs(mDatabase));

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
nsImapMailFolder::UpdateFolder(nsIMsgWindow *msgWindow)
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
              rv = CreateClientSubfolderInfo("Inbox", kOnlineHierarchySeparatorUnknown,0);
              if (NS_FAILED(rv)) 
                  return rv;
          }
          m_haveDiscoveredAllFolders = PR_TRUE;
      }
      selectFolder = PR_FALSE;
  }
  rv = GetDatabase(msgWindow);

  PRBool noSelect = PR_FALSE;
  GetFlag(MSG_FOLDER_FLAG_IMAP_NOSELECT, &noSelect);

  PRBool hasOfflineEvents = PR_FALSE;
  GetFlag(MSG_FOLDER_FLAG_OFFLINEEVENTS, &hasOfflineEvents);

  if (hasOfflineEvents && !WeAreOffline())
  {
    nsImapOfflineSync *goOnline = new nsImapOfflineSync(msgWindow, this, this);
    if (goOnline)
    {
    	return goOnline->ProcessNextOperation();
    }
  }
  if (noSelect) 
    selectFolder = PR_FALSE;
  // don't run select if we're already running a url/select...
  if (NS_SUCCEEDED(rv) && !m_urlRunning && selectFolder)
  {
    // check if we should download message bodies because it's the inbox and 
    // the server is specified as one where where we download msg bodies automatically.
    if (mFlags & MSG_FOLDER_FLAG_INBOX)
    {
      nsCOMPtr<nsIImapIncomingServer> imapServer;
      nsresult rv = GetImapIncomingServer(getter_AddRefs(imapServer));

      if (NS_SUCCEEDED(rv) && imapServer)
        imapServer->GetDownloadBodiesOnGetNewMail(&m_downloadingFolderForOfflineUse);
    }

    nsCOMPtr <nsIEventQueue> eventQ;
    NS_WITH_SERVICE(nsIEventQueueService, pEventQService, kEventQueueServiceCID, &rv); 
    if (NS_SUCCEEDED(rv) && pEventQService)
      pEventQService->GetThreadEventQueue(NS_CURRENT_THREAD,
                        getter_AddRefs(eventQ));
    rv = imapService->SelectFolder(eventQ, this, this, msgWindow, nsnull);
    if (NS_SUCCEEDED(rv))
      m_urlRunning = PR_TRUE;
    else if (rv == NS_MSG_ERROR_OFFLINE)
    {
      rv = NS_OK;
      NotifyFolderEvent(mFolderLoadedAtom);
    }
  }
  else if (NS_SUCCEEDED(rv))  // tell the front end that the folder is loaded if we're not going to 
  {                           // actually run a url.
    NotifyFolderEvent(mFolderLoadedAtom);
  }

  return rv;
}


NS_IMETHODIMP nsImapMailFolder::GetMessages(nsIMsgWindow *aMsgWindow, nsISimpleEnumerator* *result)
{
  if (result)
    *result = nsnull;
  if (mDatabase)
		return mDatabase->EnumerateMessages(result);
  return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP nsImapMailFolder::CreateSubfolder(const PRUnichar* folderName, nsIMsgWindow *msgWindow )
{
    nsresult rv = NS_ERROR_NULL_POINTER;
    if (!folderName) return rv;

    if ( nsCRT::strcasecmp(folderName,"Trash") == 0 )   // Trash , a special folder
    {
        AlertSpecialFolderExists(msgWindow);
        return NS_MSG_FOLDER_EXISTS;
    }
    else if ( nsCRT::strcasecmp(folderName,"Inbox") == 0 )  // Inbox, a special folder
    {
        AlertSpecialFolderExists(msgWindow);
        return NS_MSG_FOLDER_EXISTS;
    }

    NS_WITH_SERVICE(nsIImapService, imapService, kCImapService, &rv);
    if (NS_SUCCEEDED(rv))
        rv = imapService->CreateFolder(m_eventQueue, this, 
                                       folderName, this, nsnull);
    return rv;
}

nsresult
nsImapMailFolder::AlertSpecialFolderExists(nsIMsgWindow *msgWindow) 
{
    nsresult rv = NS_OK;
    nsCOMPtr<nsIDocShell> docShell;
    msgWindow->GetRootDocShell(getter_AddRefs(docShell));
    PRUnichar *alertString = IMAPGetStringByID(IMAP_MAILBOX_ALREADY_EXISTS);
    if (!alertString) return rv;
        if (docShell)
        {
            nsCOMPtr<nsIPrompt> dialog(do_GetInterface(docShell));
            if (dialog)
            {
                rv = dialog->Alert(nsnull, alertString);
                return rv;
            }
        }
    return rv;
}

NS_IMETHODIMP nsImapMailFolder::CreateClientSubfolderInfo(const char *folderName, PRUnichar hierarchyDelimiter, PRInt32 flags)
{
  nsresult rv = NS_OK;
    
  //Get a directory based on our current path.
  nsCOMPtr<nsIFileSpec> pathSpec;
  rv = GetPath(getter_AddRefs(pathSpec));
  if (NS_FAILED(rv)) return rv;

  nsFileSpec path;
  rv = pathSpec->GetFileSpec(&path);
  if (NS_FAILED(rv)) return rv;

//  if (!path.Exists())
//  {
//    path.CreateDir();
//  }

  rv = CreateDirectoryForFolder(path);
  if(NS_FAILED(rv))
    return rv;

    nsAutoString leafName; leafName.AssignWithConversion(folderName);
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
        uri.AppendWithConversion(parentName);
        rv = rdf->GetResource(uri,
                              getter_AddRefs(res));
        if (NS_FAILED(rv)) return rv;
        parentFolder = do_QueryInterface(res, &rv);
        if (NS_FAILED(rv)) return rv;
        nsCAutoString leafnameC;
        leafnameC.AssignWithConversion(leafName);
		return parentFolder->CreateClientSubfolderInfo(leafnameC, hierarchyDelimiter,flags);
    }
    
  // if we get here, it's really a leaf, and "this" is the parent.
    folderNameStr = leafName;
    
//    path += folderNameStr;

  // Create an empty database for this mail folder, set its name from the user  
  nsCOMPtr<nsIMsgDatabase> mailDBFactory;
    nsCOMPtr<nsIMsgFolder> child;

  rv = nsComponentManager::CreateInstance(kCMailDB, nsnull, NS_GET_IID(nsIMsgDatabase), (void **) getter_AddRefs(mailDBFactory));
  if (NS_SUCCEEDED(rv) && mailDBFactory)
  {
        nsCOMPtr<nsIMsgDatabase> unusedDB;
    nsCOMPtr <nsIFileSpec> dbFileSpec;

    nsXPIDLCString uniqueLeafName;
    nsCAutoString proposedDBName(folderName);
    proposedDBName += ".msf";

    rv = CreatePlatformLeafNameForDisk(proposedDBName, path, getter_Copies(uniqueLeafName));

    // take off the ".msf" on the end.
    proposedDBName = uniqueLeafName;
    proposedDBName.Truncate(proposedDBName.Length() - 4);

    path.SetLeafName(proposedDBName);

    NS_NewFileSpecWithSpec(path, getter_AddRefs(dbFileSpec));
    rv = mailDBFactory->Open(dbFileSpec, PR_TRUE, PR_TRUE, (nsIMsgDatabase **) getter_AddRefs(unusedDB));

        if (NS_SUCCEEDED(rv) && unusedDB)
        {
      //need to set the folder name
      nsCOMPtr <nsIDBFolderInfo> folderInfo;
      rv = unusedDB->GetDBFolderInfo(getter_AddRefs(folderInfo));
//      if(NS_SUCCEEDED(rv))
//      {
        // ### DMB used to be leafNameFromUser?
//        folderInfo->SetMailboxName(&folderNameStr);
//      }

      //Now let's create the actual new folder
      rv = AddSubfolderWithPath(&folderNameStr, dbFileSpec, getter_AddRefs(child));
//      if (NS_SUCCEEDED(rv) && child)
//        child->SetPath(dbFileSpec);

      nsCOMPtr <nsIMsgImapMailFolder> imapFolder = do_QueryInterface(child);
      if (imapFolder)
      {
        nsCAutoString onlineName(m_onlineFolderName); 
        if (onlineName.Length() > 0)
          onlineName.AppendWithConversion(hierarchyDelimiter);
        onlineName.AppendWithConversion(folderNameStr);
        imapFolder->SetVerifiedAsOnlineFolder(PR_TRUE);
        imapFolder->SetOnlineName(onlineName.get());
        imapFolder->SetHierarchyDelimiter(hierarchyDelimiter);
        imapFolder->SetBoxFlags(flags);
        // store the online name as the mailbox name in the db folder info
        // I don't think anyone uses the mailbox name, so we'll use it
        // to restore the online name when blowing away an imap db.
        if (folderInfo)
        {
          nsAutoString unicodeOnlineName; unicodeOnlineName.AssignWithConversion(onlineName);
          folderInfo->SetMailboxName(&unicodeOnlineName);
        }
      }

            unusedDB->SetSummaryValid(PR_TRUE);
      unusedDB->Commit(nsMsgDBCommitType::kLargeCommit);
            unusedDB->Close(PR_TRUE);
        }
  }
  if(NS_SUCCEEDED(rv) && child)
  {
    nsCOMPtr<nsISupports> childSupports(do_QueryInterface(child));
    nsCOMPtr<nsISupports> folderSupports;
    rv = QueryInterface(NS_GET_IID(nsISupports), getter_AddRefs(folderSupports));
    if(childSupports && NS_SUCCEEDED(rv))
    {

      NotifyItemAdded(folderSupports, childSupports, "folderView");
    }
  }
  return rv;
}
    
NS_IMETHODIMP nsImapMailFolder::List()
{
  nsresult rv;
  NS_WITH_SERVICE(nsIImapService, imapService, kCImapService, &rv);
  if (NS_FAILED(rv)) 
    return rv;
  rv = imapService->ListFolder(m_eventQueue, this, nsnull, nsnull);

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
    return nsMsgFolder::DeleteSubFolders(folders, nsnull);
}

NS_IMETHODIMP nsImapMailFolder::CreateStorageIfMissing(nsIUrlListener* urlListener)
{
	nsresult status = NS_OK;
  nsCOMPtr <nsIFolder> parent;
  GetParent(getter_AddRefs(parent));
  nsCOMPtr <nsIMsgFolder> msgParent;
  if (parent)
    msgParent = do_QueryInterface(parent);
  // parent is probably not set because *this* was probably created by rdf
  // and not by folder discovery. So, we have to compute the parent.
  if (!msgParent)
  {
    nsCAutoString folderName(mURI);
      
    nsCAutoString uri;

    PRInt32 leafPos = folderName.RFindChar('/');

    nsCAutoString parentName(folderName);

    if (leafPos > 0)
	  {
		// If there is a hierarchy, there is a parent.
		// Don't strip off slash if it's the first character
      parentName.Truncate(leafPos);
      // get the corresponding RDF resource
      // RDF will create the folder resource if it doesn't already exist
      NS_WITH_SERVICE(nsIRDFService, rdf, kRDFServiceCID, &status);
      if (NS_FAILED(status)) return status;
      nsCOMPtr<nsIRDFResource> resource;
      status = rdf->GetResource(parentName, getter_AddRefs(resource));
      if (NS_FAILED(status)) return status;

      msgParent = do_QueryInterface(resource, &status);
	  }
  }
  if (msgParent)
  {
    nsXPIDLString folderName;
    GetName(getter_Copies(folderName));
    nsresult rv;
	  NS_WITH_SERVICE(nsIImapService, imapService, kCImapService, &rv); 
	  if (NS_SUCCEEDED(rv) && imapService)
    {
      nsCOMPtr <nsIURI> uri;

      imapService->EnsureFolderExists(m_eventQueue, msgParent, folderName, urlListener, getter_AddRefs(uri));
    }
  }

  return status;
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

NS_IMETHODIMP nsImapMailFolder::GetOnlineDelimiter(char** onlineDelimiter)
{
    if (onlineDelimiter)
    {
        nsresult rv;
        PRUnichar delimiter = 0;
        rv = GetHierarchyDelimiter(&delimiter);
        nsAutoString aString(delimiter);
        *onlineDelimiter = aString.ToNewCString();
        return rv;
    }
    return NS_ERROR_NULL_POINTER;
}

NS_IMETHODIMP nsImapMailFolder::SetHierarchyDelimiter(PRUnichar aHierarchyDelimiter)
{
  m_hierarchyDelimiter = aHierarchyDelimiter;
  return NS_OK;
}

NS_IMETHODIMP nsImapMailFolder::GetHierarchyDelimiter(PRUnichar *aHierarchyDelimiter)
{
  if (!aHierarchyDelimiter)
    return NS_ERROR_NULL_POINTER;
   ReadDBFolderInfo(PR_FALSE); // update cache first.
  *aHierarchyDelimiter = m_hierarchyDelimiter;
  return NS_OK;
}

NS_IMETHODIMP nsImapMailFolder::SetBoxFlags(PRInt32 aBoxFlags)
{
  ReadDBFolderInfo(PR_FALSE);

  m_boxFlags = aBoxFlags;
  PRUint32 newFlags = mFlags;

    newFlags |= MSG_FOLDER_FLAG_IMAPBOX;

  if (m_boxFlags & kNoinferiors)
    newFlags |= MSG_FOLDER_FLAG_IMAP_NOINFERIORS;
  else
    newFlags &= ~MSG_FOLDER_FLAG_IMAP_NOINFERIORS;
    if (m_boxFlags & kNoselect)
        newFlags |= MSG_FOLDER_FLAG_IMAP_NOSELECT;
    else
        newFlags &= ~MSG_FOLDER_FLAG_IMAP_NOSELECT;
    if (m_boxFlags & kPublicMailbox)
        newFlags |= MSG_FOLDER_FLAG_IMAP_PUBLIC;
    else
        newFlags &= ~MSG_FOLDER_FLAG_IMAP_PUBLIC;
    if (m_boxFlags & kOtherUsersMailbox)
        newFlags |= MSG_FOLDER_FLAG_IMAP_OTHER_USER;
    else
        newFlags &= ~MSG_FOLDER_FLAG_IMAP_OTHER_USER;
    if (m_boxFlags & kPersonalMailbox)
        newFlags |= MSG_FOLDER_FLAG_IMAP_PERSONAL;
    else
        newFlags &= ~MSG_FOLDER_FLAG_IMAP_PERSONAL;

    SetFlags(newFlags);
  return NS_OK;
}

NS_IMETHODIMP nsImapMailFolder::GetBoxFlags(PRInt32 *aBoxFlags)
{
  if (!aBoxFlags)
    return NS_ERROR_NULL_POINTER;
  *aBoxFlags = m_boxFlags;
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

NS_IMETHODIMP nsImapMailFolder::GetNoSelect(PRBool *aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);
  return GetFlag(MSG_FOLDER_FLAG_IMAP_NOSELECT, aResult);
}
NS_IMETHODIMP nsImapMailFolder::Compact(nsIUrlListener *aListener, nsIMsgWindow *aMsgWindow)
{
  nsresult rv;
  // compact offline part purely for testing purposes
  if (WeAreOffline() && (mFlags & MSG_FOLDER_FLAG_OFFLINE))
  {
    rv = CompactOfflineStore(nsnull);
  }
  else
  {
    NS_WITH_SERVICE(nsIImapService, imapService, kCImapService, &rv);
    if (NS_SUCCEEDED(rv) && imapService)
    {
        rv = imapService->Expunge(m_eventQueue, this, aListener, nsnull);
    }
  }
  return rv;
}

NS_IMETHODIMP nsImapMailFolder::CompactAll(nsIUrlListener *aListener,  nsIMsgWindow *aMsgWindow)
{
  return Compact(aListener, aMsgWindow);  //for now
}

NS_IMETHODIMP nsImapMailFolder::EmptyTrash(nsIMsgWindow *msgWindow,
                                           nsIUrlListener *aListener)
{
    nsresult rv;
    nsCOMPtr<nsIMsgFolder> trashFolder;
    rv = GetTrashFolder(getter_AddRefs(trashFolder));
    if (NS_SUCCEEDED(rv))
    {
       NS_WITH_SERVICE(nsIMsgAccountManager, accountManager, kMsgAccountManagerCID, &rv);
       if (accountManager)
       {
         // if we are emptying trash on exit and we are an aol server then don't perform
         // this operation because it's causing a hang that we haven't been able to figure out yet
         // this is an rtm fix and we'll look for the right solution post rtm. 

         PRBool empytingOnExit = PR_FALSE;
         accountManager->GetEmptyTrashInProgress(&empytingOnExit);
         if (empytingOnExit)
         {
            nsCOMPtr<nsIImapIncomingServer> imapServer;
            rv = GetImapIncomingServer(getter_AddRefs(imapServer));

            if (NS_SUCCEEDED(rv) && imapServer) 
            {
              PRBool isAOLServer = PR_FALSE;
              imapServer->GetIsAOLServer(&isAOLServer);
              if (isAOLServer)
                return NS_ERROR_FAILURE;  // we will not be performing an empty trash....
            } // if we fetched an imap server
         } // if emptying trash on exit which is done through the account manager.
       }

        nsCOMPtr<nsIMsgDatabase> trashDB;

        rv = trashFolder->Delete(); // delete summary spec
        rv = trashFolder->GetMsgDatabase(msgWindow, getter_AddRefs(trashDB));

        NS_WITH_SERVICE (nsIImapService, imapService, kCImapService, &rv);
        if (NS_SUCCEEDED(rv))
        {
            if (aListener)
            {
                rv = imapService->DeleteAllMessages(m_eventQueue, trashFolder,
                                                    aListener, nsnull);
            }
            else
            {
                nsCOMPtr<nsIUrlListener> urlListener = 
                    do_QueryInterface(trashFolder);
                rv = imapService->DeleteAllMessages(m_eventQueue, trashFolder,
                                                    urlListener, nsnull);
            }
        }
        PRBool hasSubfolders = PR_FALSE;
        rv = trashFolder->GetHasSubFolders(&hasSubfolders);
        if (hasSubfolders)
        {
            nsCOMPtr<nsIEnumerator> aEnumerator;
            nsCOMPtr<nsISupports> aSupport;
            nsCOMPtr<nsIMsgFolder> aFolder;
            nsCOMPtr<nsISupportsArray> aSupportsArray;
            rv = NS_NewISupportsArray(getter_AddRefs(aSupportsArray));
            if (NS_FAILED(rv)) return rv;
            rv = trashFolder->GetSubFolders(getter_AddRefs(aEnumerator));
            rv = aEnumerator->First();
            while(NS_SUCCEEDED(rv))
            {
                rv = aEnumerator->CurrentItem(getter_AddRefs(aSupport));
                if (NS_SUCCEEDED(rv))
                {
                    aSupportsArray->AppendElement(aSupport);
                    rv = aEnumerator->Next();
                }
            }
            PRUint32 cnt = 0;
            aSupportsArray->Count(&cnt);
            for (PRInt32 i = cnt-1; i >= 0; i--)
            {
                aSupport = aSupportsArray->ElementAt(i);
                aSupportsArray->RemoveElementAt(i);
                aFolder = do_QueryInterface(aSupport);
                if (aFolder)
                    trashFolder->PropagateDelete(aFolder, PR_TRUE);
            }
        }
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

NS_IMETHODIMP nsImapMailFolder::Rename (const PRUnichar *newName, nsIMsgWindow *msgWindow )
{
    nsresult rv = NS_ERROR_FAILURE;
    nsCOMPtr <nsIImapIncomingServer> incomingImapServer;

    GetImapIncomingServer(getter_AddRefs(incomingImapServer));
    if (incomingImapServer)
      RecursiveCloseActiveConnections(incomingImapServer);

    NS_WITH_SERVICE (nsIImapService, imapService, kCImapService, &rv);
    if (NS_SUCCEEDED(rv))
        rv = imapService->RenameLeaf(m_eventQueue, this, newName, this,
                                     nsnull);
    return rv;
}

NS_IMETHODIMP nsImapMailFolder::RecursiveCloseActiveConnections(nsIImapIncomingServer *incomingImapServer)
{
  NS_ENSURE_ARG(incomingImapServer);
  PRUint32 cnt = 0, i;
  nsresult rv;
  if (mSubFolders)
  {
      nsCOMPtr<nsISupports> aSupport;
      nsCOMPtr<nsIMsgImapMailFolder> folder;
      mSubFolders->Count(&cnt);
      if (cnt > 0)
      {
          for (i = 0; i < cnt; i++)
          {
              aSupport = getter_AddRefs(mSubFolders->ElementAt(i));
              folder = do_QueryInterface(aSupport);
              if (folder)
                  folder->RecursiveCloseActiveConnections(incomingImapServer);
              nsCOMPtr<nsIMsgFolder> msgFolder = do_QueryInterface(folder, &rv);
              if (NS_SUCCEEDED(rv) && msgFolder)
                incomingImapServer->CloseConnectionForFolder(msgFolder);
          }
      }
  }
  return NS_OK;  
}

// this is called *after* we've done the rename on the server.
NS_IMETHODIMP nsImapMailFolder::PrepareToRename()
{
    PRUint32 cnt = 0, i;
    if (mSubFolders)
    {
        nsCOMPtr<nsISupports> aSupport;
        nsCOMPtr<nsIMsgImapMailFolder> folder;
        mSubFolders->Count(&cnt);
        if (cnt > 0)
        {
            for (i = 0; i < cnt; i++)
            {
                aSupport = getter_AddRefs(mSubFolders->ElementAt(i));
                folder = do_QueryInterface(aSupport);
                if (folder)
                    folder->PrepareToRename();
            }
        }
    }
    SetOnlineName("");
    return NS_OK;
}

NS_IMETHODIMP nsImapMailFolder::RenameLocal(const char *newName, nsIMsgFolder *parent)
{
    nsCAutoString leafname(newName);
    nsCAutoString parentName;
    // newName always in the canonical form "greatparent/parentname/leafname"
    PRInt32 leafpos = leafname.RFindChar('/');
    if (leafpos >0) 
        leafname.Cut(0, leafpos+1);
    m_msgParser = null_nsCOMPtr();
    PrepareToRename();
    NotifyStoreClosedAllHeaders();
    ForceDBClosed();

    nsresult rv = NS_OK;
    nsCOMPtr<nsIFileSpec> oldPathSpec;
    rv = GetPath(getter_AddRefs(oldPathSpec));
    if (NS_FAILED(rv)) return rv;

	nsCOMPtr<nsIFileSpec> parentPathSpec;
	rv = parent->GetPath(getter_AddRefs(parentPathSpec));
	NS_ENSURE_SUCCESS(rv,rv);

	nsFileSpec parentPath;
	rv = parentPathSpec->GetFileSpec(&parentPath);
	NS_ENSURE_SUCCESS(rv,rv);

    if (!parentPath.IsDirectory())
	  AddDirectorySeparator(parentPath);
    
    PRUint32 cnt = 0;
    nsFileSpec dirSpec;

    if (mSubFolders)
        mSubFolders->Count(&cnt);
    if (cnt > 0)
    {
        oldPathSpec->GetFileSpec(&dirSpec);
        rv = CreateDirectoryForFolder(dirSpec);
    }
    nsFileSpec fileSpec;
    oldPathSpec->GetFileSpec(&fileSpec);
    nsLocalFolderSummarySpec oldSummarySpec(fileSpec);
    nsCAutoString newNameStr = leafname;
    newNameStr += ".msf";
    oldSummarySpec.Delete(PR_FALSE);
    if (cnt > 0)
    {
       newNameStr = leafname;
       newNameStr += ".sbd";
	   char *leafName = dirSpec.GetLeafName();
	   if (nsCRT::strcmp(leafName, newNameStr) != 0 )
	   {
           dirSpec.Rename(newNameStr.get());      // in case of rename operation leaf names will differ
		   nsCRT::free(leafName);
		   return rv;
	   }
	   nsCRT::free(leafName);
                                               
	   parentPath += newNameStr;    //only for move we need to progress further in case the parent differs

	   if (!parentPath.IsDirectory())
		   parentPath.CreateDirectory();
	   else
		   NS_ASSERTION(0,"Directory already exists.");
	   
	   nsCOMPtr<nsILocalFile> srcDir = (do_CreateInstance(NS_LOCAL_FILE_CONTRACTID, &rv));
	   NS_ENSURE_SUCCESS(rv,rv);

	   nsCOMPtr<nsILocalFile> destDir = (do_CreateInstance(NS_LOCAL_FILE_CONTRACTID, &rv));
	   NS_ENSURE_SUCCESS(rv,rv);
	  
	   nsCString oldPathStr (dirSpec.GetNativePathCString());
       srcDir->InitWithPath(oldPathStr.get());
	   
       nsCString newPathStr (parentPath.GetNativePathCString());
       destDir->InitWithPath(newPathStr.get());
       
	   rv = RecursiveCopy(srcDir, destDir);
       
	   NS_ENSURE_SUCCESS(rv,rv);

	   dirSpec.Delete(PR_TRUE);                         // moving folders
    }
    return rv;
}

NS_IMETHODIMP nsImapMailFolder::GetPrettyName(PRUnichar ** prettyName)
{
  return GetName(prettyName);
}
    
NS_IMETHODIMP nsImapMailFolder::GetFolderURL(char **url)
{
    nsresult rv = NS_ERROR_FAILURE;
    return rv;
}
    
NS_IMETHODIMP nsImapMailFolder::UpdateSummaryTotals(PRBool force) 
{
  if (!mNotifyCountChanges || mIsServer)
    return NS_OK;

  // could we move this into nsMsgDBFolder, or do we need to deal
  // with the pending imap counts?
  nsresult rv = NS_OK;

  PRInt32 oldUnreadMessages = mNumUnreadMessages + mNumPendingUnreadMessages;
  PRInt32 oldTotalMessages = mNumTotalMessages + mNumPendingTotalMessages;
  //We need to read this info from the database
  ReadDBFolderInfo(force);

  PRInt32 newUnreadMessages = mNumUnreadMessages + mNumPendingUnreadMessages;
  PRInt32 newTotalMessages = mNumTotalMessages + mNumPendingTotalMessages;

  //Need to notify listeners that total count changed.
  if(oldTotalMessages != newTotalMessages)
  {
    NotifyIntPropertyChanged(kTotalMessagesAtom, oldTotalMessages, newTotalMessages);
  }

  if(oldUnreadMessages != newUnreadMessages)
  {
    NotifyIntPropertyChanged(kTotalUnreadMessagesAtom, oldUnreadMessages, newUnreadMessages);
  }

  FlushToFolderCache();
    return rv;
}
    
NS_IMETHODIMP nsImapMailFolder::GetDeletable (PRBool *deletable)
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

NS_IMETHODIMP
nsImapMailFolder::GetCanCreateSubfolders(PRBool *aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);
  *aResult = !(mFlags & MSG_FOLDER_FLAG_IMAP_NOINFERIORS);

  PRBool isServer = PR_FALSE;
  GetIsServer(&isServer);
  if (!isServer)
  {
      nsCOMPtr<nsIImapIncomingServer> imapServer;
      nsresult rv = GetImapIncomingServer(getter_AddRefs(imapServer));
      PRBool dualUseFolders = PR_TRUE;
      if (NS_SUCCEEDED(rv) && imapServer)
          imapServer->GetDualUseFolders(&dualUseFolders);
      if (!dualUseFolders && *aResult)
          *aResult = (mFlags & MSG_FOLDER_FLAG_IMAP_NOSELECT);
  }
  
  return NS_OK;
}

NS_IMETHODIMP
nsImapMailFolder::GetCanSubscribe(PRBool *aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);
  *aResult = PR_FALSE;

  PRBool isImapServer = PR_FALSE;
  nsresult rv = GetIsServer(&isImapServer);
  if (NS_FAILED(rv)) return rv;
 
  // you can only subscribe to imap servers, not imap folders
  *aResult = isImapServer;
  return NS_OK;
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

nsresult nsImapMailFolder::GetImapIncomingServer(nsIImapIncomingServer **aImapIncomingServer)
{
  NS_ENSURE_ARG(aImapIncomingServer);

  *aImapIncomingServer = nsnull;

  nsCOMPtr<nsIMsgIncomingServer> server;

  if (NS_SUCCEEDED(GetServer(getter_AddRefs(server))) && server)
  {
    nsCOMPtr <nsIImapIncomingServer> incomingServer = do_QueryInterface(server);
    *aImapIncomingServer = incomingServer;
    NS_IF_ADDREF(*aImapIncomingServer);
    return NS_OK;
  }
  return NS_ERROR_NULL_POINTER;
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
nsImapMailFolder::AddMessageDispositionState(nsIMsgDBHdr *aMessage, nsMsgDispositionState aDispositionFlag)
{
  nsMsgDBFolder::AddMessageDispositionState(aMessage, aDispositionFlag);

  // set the mark message answered flag on the server for this message...
  if (aMessage)
  {
    nsMsgKeyArray messageIDs;
    nsMsgKey msgKey;
    aMessage->GetMessageKey(&msgKey);
    messageIDs.Add(msgKey);

    if (aDispositionFlag == nsIMsgFolder::nsMsgDispositionState_Replied)
      StoreImapFlags(kImapMsgAnsweredFlag, PR_TRUE, messageIDs.GetArray(), messageIDs.GetSize());
    else if (aDispositionFlag == nsIMsgFolder::nsMsgDispositionState_Forwarded)
      StoreImapFlags(kImapMsgForwardedFlag, PR_TRUE, messageIDs.GetArray(), messageIDs.GetSize());
  }
  return NS_OK;
}

NS_IMETHODIMP
nsImapMailFolder::MarkMessagesRead(nsISupportsArray *messages, PRBool markRead)
{
  nsresult rv;

  // tell the folder to do it, which will mark them read in the db.
  rv = nsMsgFolder::MarkMessagesRead(messages, markRead);
  if (NS_SUCCEEDED(rv))
  {
    nsCAutoString messageIds;
    nsMsgKeyArray keysToMarkRead;
    rv = BuildIdsAndKeyArray(messages, messageIds, keysToMarkRead);
    if (NS_FAILED(rv)) return rv;

    rv = StoreImapFlags(kImapMsgSeenFlag, markRead,  keysToMarkRead.GetArray(), keysToMarkRead.GetSize());
    mDatabase->Commit(nsMsgDBCommitType::kLargeCommit);
  }
  return rv;
}

NS_IMETHODIMP
nsImapMailFolder::MarkAllMessagesRead(void)
{
  nsresult rv = GetDatabase(nsnull);
  
  if(NS_SUCCEEDED(rv))
  {
    nsMsgKeyArray thoseMarked;
    EnableNotifications(allMessageCountNotifications, PR_FALSE);
    rv = mDatabase->MarkAllRead(&thoseMarked);
    EnableNotifications(allMessageCountNotifications, PR_TRUE);
    if (NS_SUCCEEDED(rv))
    {
      rv = StoreImapFlags(kImapMsgSeenFlag, PR_TRUE, thoseMarked.GetArray(), thoseMarked.GetSize());
      mDatabase->Commit(nsMsgDBCommitType::kLargeCommit);
    }
  }

  return rv;
}

NS_IMETHODIMP nsImapMailFolder::MarkThreadRead(nsIMsgThread *thread)
{

	nsresult rv = GetDatabase(nsnull);
	if(NS_SUCCEEDED(rv))
  {
    nsMsgKeyArray thoseMarked;
		rv = mDatabase->MarkThreadRead(thread, nsnull, &thoseMarked);
    if (NS_SUCCEEDED(rv))
    {
      rv = StoreImapFlags(kImapMsgSeenFlag, PR_TRUE, thoseMarked.GetArray(), thoseMarked.GetSize());
      mDatabase->Commit(nsMsgDBCommitType::kLargeCommit);
    }
  }
	return rv;
}


NS_IMETHODIMP nsImapMailFolder::ReadFromFolderCacheElem(nsIMsgFolderCacheElement *element)
{
  nsresult rv = nsMsgDBFolder::ReadFromFolderCacheElem(element);
  PRInt32 hierarchyDelimiter = kOnlineHierarchySeparatorUnknown;
  nsXPIDLCString onlineName;

  element->GetInt32Property("boxFlags", &m_boxFlags);
  if (NS_SUCCEEDED(element->GetInt32Property("hierDelim", &hierarchyDelimiter)))
    m_hierarchyDelimiter = (PRUnichar) hierarchyDelimiter;
  rv = element->GetStringProperty("onlineName", getter_Copies(onlineName));
  if (NS_SUCCEEDED(rv) && (const char *) onlineName && nsCRT::strlen((const char *) onlineName))
    m_onlineFolderName.Assign(onlineName);
#ifdef DEBUG_bienvenu
  if (!nsCRT::strcasecmp((const char *) onlineName, "Sent"))
    printf("loading folder cache elem for %s flags = %lx", (const char *) onlineName, mFlags);
  else if (!nsCRT::strcasecmp((const char *) onlineName, "INBOX"))
    printf("loading folder cache elem for %s flags = %lx", (const char *) onlineName, mFlags);
#endif
  return rv;
}

NS_IMETHODIMP nsImapMailFolder::WriteToFolderCacheElem(nsIMsgFolderCacheElement *element)
{
  nsresult rv = nsMsgDBFolder::WriteToFolderCacheElem(element);
  element->SetInt32Property("boxFlags", m_boxFlags);
  element->SetInt32Property("hierDelim", (PRInt32) m_hierarchyDelimiter);
  element->SetStringProperty("onlineName", m_onlineFolderName.get());
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
    nsCAutoString messageIds;
        nsMsgKeyArray keysToMarkFlagged;
    rv = BuildIdsAndKeyArray(messages, messageIds, keysToMarkFlagged);
    if (NS_FAILED(rv)) return rv;

    rv = StoreImapFlags(kImapMsgFlaggedFlag, markFlagged,  keysToMarkFlagged.GetArray(), keysToMarkFlagged.GetSize());
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

NS_IMETHODIMP nsImapMailFolder::SetOnlineName(const char * aOnlineFolderName)
{
  nsresult rv;
  nsCOMPtr<nsIMsgDatabase> db; 
  nsCOMPtr<nsIDBFolderInfo> folderInfo;
  rv = GetDBFolderInfoAndDB(getter_AddRefs(folderInfo), getter_AddRefs(db));
  // do this after GetDBFolderInfoAndDB, because it crunches m_onlineFolderName (not sure why)
  m_onlineFolderName = aOnlineFolderName;
  if(NS_SUCCEEDED(rv) && folderInfo)
  {
    nsAutoString onlineName; onlineName.AssignWithConversion(aOnlineFolderName);
    rv = folderInfo->SetProperty("onlineName", &onlineName);
    rv = folderInfo->SetMailboxName(&onlineName);
    // so, when are we going to commit this? Definitely not every time!
    // We could check if the online name has changed.
    db->Commit(nsMsgDBCommitType::kLargeCommit);
  }
  folderInfo = null_nsCOMPtr();
  return rv;
}


NS_IMETHODIMP nsImapMailFolder::GetOnlineName(char ** aOnlineFolderName)
{
  if (!aOnlineFolderName)
    return NS_ERROR_NULL_POINTER;
  ReadDBFolderInfo(PR_FALSE); // update cache first.
  *aOnlineFolderName = m_onlineFolderName.ToNewCString();
  return (*aOnlineFolderName) ? NS_OK : NS_ERROR_OUT_OF_MEMORY;

  // ### do we want to read from folder cache first, or has that been done?
}


NS_IMETHODIMP
nsImapMailFolder::GetDBFolderInfoAndDB(nsIDBFolderInfo **folderInfo, nsIMsgDatabase **db)
{
  nsresult openErr=NS_ERROR_UNEXPECTED;
  if(!db || !folderInfo)
  return NS_ERROR_NULL_POINTER; //ducarroz: should we use NS_ERROR_INVALID_ARG?
  nsresult rv;

  openErr = GetDatabase(nsnull);

  *db = mDatabase;
  NS_IF_ADDREF(*db);
    if (NS_SUCCEEDED(openErr)&& *db)
  {
        openErr = (*db)->GetDBFolderInfo(folderInfo);
    if (NS_SUCCEEDED(openErr) && folderInfo)
    {
      nsXPIDLCString onlineName;
      if (NS_SUCCEEDED((*folderInfo)->GetCharPtrProperty("onlineName", getter_Copies(onlineName))))
      {
        if ((const char*) onlineName && nsCRT::strlen((const char *) onlineName) > 0)
          m_onlineFolderName.Assign(onlineName);
        else
        {
          nsAutoString autoOnlineName; 
          // autoOnlineName.AssignWithConversion(name);
          (*folderInfo)->GetMailboxName(&autoOnlineName);
          if (autoOnlineName.Length() == 0)
          {
            nsXPIDLCString uri;
            rv = GetURI(getter_Copies(uri));
            if (NS_FAILED(rv)) return rv;
            nsXPIDLCString hostname;
            rv = GetHostname(getter_Copies(hostname));
            if (NS_FAILED(rv)) return rv;
            nsXPIDLCString name;
            rv = nsImapURI2FullName(kImapRootURI, hostname, uri, getter_Copies(name));
            m_onlineFolderName.Assign(name);
            autoOnlineName.AssignWithConversion(name);
          }
          rv = (*folderInfo)->SetProperty("onlineName", &autoOnlineName);
        }
      }
    }
  }
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

    if (!messages) return rv;

    rv = messages->Count(&count);
    if (NS_FAILED(rv)) return rv;

    // build up message keys.
    for (i = 0; i < count; i++)
    {
      msgSupports = getter_AddRefs(messages->ElementAt(i));
      nsMsgKey key;
      nsCOMPtr <nsIMsgDBHdr> msgDBHdr = do_QueryInterface(msgSupports, &rv);
      if (msgDBHdr)
        rv = msgDBHdr->GetMessageKey(&key);
      if (NS_SUCCEEDED(rv))
        keyArray.Add(key);
    }
    
  return AllocateUidStringFromKeys(keyArray.GetArray(), keyArray.GetSize(), msgIds);
}

static int PR_CALLBACK CompareKey (const void *v1, const void *v2, void *)
{
	// QuickSort callback to compare array values
	nsMsgKey i1 = *(nsMsgKey *)v1;
	nsMsgKey i2 = *(nsMsgKey *)v2;
	return i1 - i2;
}

/* static */nsresult
nsImapMailFolder::AllocateUidStringFromKeys(nsMsgKey *keys, PRInt32 numKeys, nsCString &msgIds)
{
    nsresult rv = NS_OK;
  PRInt32 startSequence = -1;
    if (numKeys > 0)
        startSequence = keys[0];
  PRInt32 curSequenceEnd = startSequence;
  PRUint32 total = numKeys;
  // sort keys and then generate ranges instead of singletons!
  NS_QuickSort(keys, numKeys, sizeof(nsMsgKey), CompareKey, nsnull);
  for (PRUint32 keyIndex = 0; keyIndex < total; keyIndex++)
  {
    PRUint32 curKey = keys[keyIndex];
    PRUint32 nextKey = (keyIndex + 1 < total) ? keys[keyIndex + 1] : 0xFFFFFFFF;
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
      msgIds.AppendInt(startSequence, 10);
      msgIds += ':';
      msgIds.AppendInt(curSequenceEnd, 10);
      if (!lastKey)
        msgIds += ',';
      startSequence = nextKey;
      curSequenceEnd = startSequence;
    }
    else
    {
      startSequence = nextKey;
      curSequenceEnd = startSequence;
      msgIds.AppendInt(keys[keyIndex], 10);
      if (!lastKey)
        msgIds += ',';
    }
  }
    return rv;
}

nsresult nsImapMailFolder::MarkMessagesImapDeleted(nsMsgKeyArray *keyArray, PRBool deleted, nsIMsgDatabase *db)
{
  PRBool allKeysImapDeleted;
  db->AllMsgKeysImapDeleted(keyArray, &allKeysImapDeleted);
	for (PRUint32 kindex = 0; kindex < keyArray->GetSize(); kindex++)
	{
		nsMsgKey key = keyArray->ElementAt(kindex);
    db->MarkImapDeleted(key, deleted || !allKeysImapDeleted, nsnull);
  }
  return NS_OK;
}

NS_IMETHODIMP nsImapMailFolder::DeleteMessages(nsISupportsArray *messages,
                                               nsIMsgWindow *msgWindow,
                                               PRBool deleteStorage, PRBool isMove,
                                               nsIMsgCopyServiceListener* listener)
{
  nsresult rv = NS_ERROR_FAILURE;
  // *** jt - assuming delete is move to the trash folder for now
  nsCOMPtr<nsIEnumerator> aEnumerator;
  nsCOMPtr<nsIRDFResource> res;
  nsCAutoString uri;
  PRBool deleteImmediatelyNoTrash = PR_FALSE;
  nsCAutoString messageIds;
  nsMsgKeyArray srcKeyArray;

  nsMsgImapDeleteModel deleteModel = nsMsgImapDeleteModels::MoveToTrash;

  nsCOMPtr<nsIImapIncomingServer> imapServer;
  rv = GetFlag(MSG_FOLDER_FLAG_TRASH, &deleteImmediatelyNoTrash);
  rv = GetImapIncomingServer(getter_AddRefs(imapServer));

  if (NS_SUCCEEDED(rv) && imapServer)
  {
    imapServer->GetDeleteModel(&deleteModel);
    if (deleteModel != nsMsgImapDeleteModels::MoveToTrash || deleteStorage)
      deleteImmediatelyNoTrash = PR_TRUE;
  }

  rv = BuildIdsAndKeyArray(messages, messageIds, srcKeyArray);
  if (NS_FAILED(rv)) return rv;


  nsCOMPtr<nsIMsgFolder> rootFolder;
  nsCOMPtr<nsIMsgFolder> trashFolder;

	if (!deleteImmediatelyNoTrash)
	{
        rv = GetRootFolder(getter_AddRefs(rootFolder));
        if (NS_SUCCEEDED(rv) && rootFolder)
        {
            PRUint32 numFolders = 0;
            rv = rootFolder->GetFoldersWithFlag(MSG_FOLDER_FLAG_TRASH,
                                                1, &numFolders,
                                                getter_AddRefs(trashFolder));
      NS_ASSERTION(NS_SUCCEEDED(rv) && trashFolder != 0, "couldn't find trash");

			// if we can't find the trash, we'll just have to do an imap delete and pretend this is the trash
			if (NS_FAILED(rv) || !trashFolder)
				deleteImmediatelyNoTrash = PR_TRUE;
		}
	}
  if (msgWindow)
  {
    nsCOMPtr <nsITransactionManager> txnMgr;

    msgWindow->GetTransactionManager(getter_AddRefs(txnMgr));

    if (txnMgr) SetTransactionManager(txnMgr);
  }
    

  if ((NS_SUCCEEDED(rv) && deleteImmediatelyNoTrash) || deleteModel == nsMsgImapDeleteModels::IMAPDelete )
  {
    nsImapMoveCopyMsgTxn* undoMsgTxn = new nsImapMoveCopyMsgTxn(
        this, &srcKeyArray, messageIds.get(), nsnull,
        PR_TRUE, isMove, m_eventQueue, nsnull);
    if (!undoMsgTxn) return NS_ERROR_OUT_OF_MEMORY;
    undoMsgTxn->SetTransactionType(nsIMessenger::eDeleteMsg);
    // we're adding this undo action before the delete is successful. This is evil,
    // but 4.5 did it as well.
    if (m_transactionManager)
      m_transactionManager->DoTransaction(undoMsgTxn);

    rv = StoreImapFlags(kImapMsgDeletedFlag, PR_TRUE, srcKeyArray.GetArray(), srcKeyArray.GetSize());
    if (NS_SUCCEEDED(rv))
    {
        if (mDatabase)
        {
            if (deleteModel == nsMsgImapDeleteModels::IMAPDelete)
            {
                MarkMessagesImapDeleted(&srcKeyArray, PR_TRUE, mDatabase);
            }
            else
            {
                mDatabase->DeleteMessages(&srcKeyArray,NULL);
                NotifyFolderEvent(mDeleteOrMoveMsgCompletedAtom);
            }
        }
    }
        return rv;
    }

    // have to move the messages to the trash
  else
  {
    if(trashFolder)
	  {
      nsCOMPtr<nsIMsgFolder> srcFolder;
      nsCOMPtr<nsISupports>srcSupport;
      PRUint32 count = 0;
      rv = messages->Count(&count);

      rv = QueryInterface(NS_GET_IID(nsIMsgFolder),
					  getter_AddRefs(srcFolder));
      rv = trashFolder->CopyMessages(srcFolder, messages, PR_TRUE, msgWindow, listener,PR_FALSE);
	  }
  }
  return rv;
}

// check if folder is the trash, or a descendent of the trash
// so we can tell if the folders we're deleting from it should
// be *really* deleted.
PRBool
nsImapMailFolder::TrashOrDescendentOfTrash(nsIMsgFolder* folder)
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
        rv = curFolder->GetFlags(&flags);
        if (NS_FAILED(rv)) return PR_FALSE;
        if (flags & MSG_FOLDER_FLAG_TRASH)
            return PR_TRUE;
        rv = curFolder->GetParent(getter_AddRefs(iFolder));
        if (NS_FAILED(rv)) return PR_FALSE;
        parent = do_QueryInterface(iFolder, &rv);
        if (NS_FAILED(rv)) return PR_FALSE;
        curFolder = do_QueryInterface(parent, &rv);
    } while (NS_SUCCEEDED(rv) && curFolder);

    return PR_FALSE;
}
NS_IMETHODIMP
nsImapMailFolder::DeleteSubFolders(nsISupportsArray* folders, nsIMsgWindow *msgWindow)
{
    nsCOMPtr<nsIMsgFolder> curFolder;
    nsCOMPtr<nsISupports> folderSupport;
    nsCOMPtr<nsIUrlListener> urlListener;
    nsCOMPtr<nsIMsgFolder> trashFolder;
    PRUint32 i, folderCount = 0;
    nsresult rv;
    // "this" is the folder we're deleting from
    PRBool deleteNoTrash = TrashOrDescendentOfTrash(this);;
    PRBool confirmed = PR_FALSE;

    NS_WITH_SERVICE (nsIImapService, imapService, kCImapService, &rv);
    if (NS_SUCCEEDED(rv))
    {
        rv = folders->Count(&folderCount);
        if (NS_SUCCEEDED(rv))
        {
            rv = GetTrashFolder(getter_AddRefs(trashFolder));
            if (!msgWindow) return NS_ERROR_NULL_POINTER;
            nsCOMPtr<nsIDocShell> docShell;
            msgWindow->GetRootDocShell(getter_AddRefs(docShell));
			//If we can't find the trash folder and we are supposed to move it to the trash
			//return failure.
			if((NS_FAILED(rv) || !trashFolder) && !deleteNoTrash)
				return NS_ERROR_FAILURE;

            nsCOMPtr<nsIPrompt> dialog;
            if (docShell) dialog = do_GetInterface(docShell);
            if (!deleteNoTrash)
            {
              PRBool canHaveSubFoldersOfTrash = PR_TRUE;
              trashFolder->GetCanCreateSubfolders(&canHaveSubFoldersOfTrash);
              if (canHaveSubFoldersOfTrash) // UW server doesn't set NOINFERIORS - check dual use pref
              {
                nsCOMPtr<nsIImapIncomingServer> imapServer;
                rv = GetImapIncomingServer(getter_AddRefs(imapServer));

                if (NS_SUCCEEDED(rv) && imapServer) 
                {
                  PRBool serverSupportsDualUseFolders;
                  imapServer->GetDualUseFolders(&serverSupportsDualUseFolders);
                  if (!serverSupportsDualUseFolders)
                    canHaveSubFoldersOfTrash = PR_FALSE;
                }
              }
              if (!canHaveSubFoldersOfTrash)
                deleteNoTrash = PR_TRUE;
            }

            PRUnichar *confirmationStr = IMAPGetStringByID((!deleteNoTrash)
              ? IMAP_MOVE_FOLDER_TO_TRASH : IMAP_DELETE_NO_TRASH);

            if (dialog && confirmationStr) {
                dialog->Confirm(nsnull, confirmationStr, &confirmed);
            }
            if (confirmed)
            {
              for (i = 0; i < folderCount; i++)
              {
                  folderSupport = getter_AddRefs(folders->ElementAt(i));
                  curFolder = do_QueryInterface(folderSupport, &rv);
                  if (NS_SUCCEEDED(rv))
                  {
                      urlListener = do_QueryInterface(curFolder);
                      if (deleteNoTrash)
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
            else
            if (confirmationStr)
                nsCRT::free(confirmationStr);
        }
    }
    
    if (confirmed && deleteNoTrash)   //delete subfolders only if you are  deleting things from trash
        return nsMsgFolder::DeleteSubFolders(folders, nsnull);
    else
        return rv;
}

NS_IMETHODIMP nsImapMailFolder::GetNewMessages(nsIMsgWindow *aWindow)
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
    rv = rootFolder->GetFoldersWithFlag(MSG_FOLDER_FLAG_INBOX, 1, &numFolders, getter_AddRefs(inbox));
  }
  if (inbox)
  {
    nsCOMPtr <nsIEventQueue> eventQ;
    NS_WITH_SERVICE(nsIEventQueueService, pEventQService, kEventQueueServiceCID, &rv); 
    if (NS_SUCCEEDED(rv) && pEventQService)
      pEventQService->GetThreadEventQueue(NS_CURRENT_THREAD,
                        getter_AddRefs(eventQ));

    nsCOMPtr<nsIImapIncomingServer> imapServer;
    nsresult rv = GetImapIncomingServer(getter_AddRefs(imapServer));

    if (NS_SUCCEEDED(rv) && imapServer)
      imapServer->GetDownloadBodiesOnGetNewMail(&m_downloadingFolderForOfflineUse);

    inbox->SetGettingNewMessages(PR_TRUE);
    rv = imapService->SelectFolder(eventQ, inbox, this, aWindow, nsnull);
  }

  return rv;
}

NS_IMETHODIMP nsImapMailFolder::Shutdown(PRBool shutdownChildren)
{
  m_filterList = null_nsCOMPtr();
  return nsMsgDBFolder::Shutdown(shutdownChildren);
}

nsresult nsImapMailFolder::GetBodysToDownload(nsMsgKeyArray *keysOfMessagesToDownload)
{
  NS_ENSURE_ARG(keysOfMessagesToDownload);

  nsresult rv;

  if (mDatabase)
  {
    nsCOMPtr <nsISimpleEnumerator> enumerator;
    rv = mDatabase->EnumerateMessages(getter_AddRefs(enumerator));
    if (NS_SUCCEEDED(rv) && enumerator)
    {
      PRBool hasMore;

	    while (NS_SUCCEEDED(rv = enumerator->HasMoreElements(&hasMore)) && (hasMore == PR_TRUE)) 
	    {
        nsCOMPtr <nsIMsgDBHdr> pHeader;
        rv = enumerator->GetNext(getter_AddRefs(pHeader));
        NS_ASSERTION(NS_SUCCEEDED(rv), "nsMsgDBEnumerator broken");
        if (pHeader && NS_SUCCEEDED(rv))
        {
          PRBool shouldStoreMsgOffline = PR_FALSE;
          nsMsgKey msgKey;
          pHeader->GetMessageKey(&msgKey);
          // MsgFitsDownloadCriteria ignores MSG_FOLDER_FLAG_OFFLINE, which we want
          if (m_downloadingFolderForOfflineUse)
            MsgFitsDownloadCriteria(msgKey, &shouldStoreMsgOffline);
          else
            ShouldStoreMsgOffline(msgKey, &shouldStoreMsgOffline);
          if (shouldStoreMsgOffline)
            keysOfMessagesToDownload->Add(msgKey);
        }
      }
    }
  }
    return rv;
}

NS_IMETHODIMP nsImapMailFolder::UpdateImapMailboxInfo(
  nsIImapProtocol* aProtocol, nsIMailboxSpec* aSpec)
{
  nsresult rv = NS_ERROR_FAILURE;
  ChangeNumPendingTotalMessages(-GetNumPendingTotalMessages());
  ChangeNumPendingUnread(-GetNumPendingUnread());

  if (!mDatabase)
  {
    GetDatabase(nsnull);
    if (mDatabase && mAddListener)
        mDatabase->AddListener(this);
  }
  PRBool folderSelected;
  rv = aSpec->GetFolderSelected(&folderSelected);
  if (NS_SUCCEEDED(rv) && folderSelected)
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
    PRInt32 folderValidity;
    aSpec->GetFolder_UIDVALIDITY(&folderValidity);

    nsCOMPtr <nsIImapFlagAndUidState> flagState;

    aSpec->GetFlagState(getter_AddRefs(flagState));

    if ((imapUIDValidity != folderValidity) /* && // if UIDVALIDITY Changed 
      !NET_IsOffline() */)
    {

      nsCOMPtr<nsIMsgDatabase> mailDBFactory;

      nsCOMPtr<nsIFileSpec> pathSpec;
      rv = GetPath(getter_AddRefs(pathSpec));
      if (NS_FAILED(rv)) return rv;

      nsFileSpec dbName;
      rv = pathSpec->GetFileSpec(&dbName);
      if (NS_FAILED(rv)) return rv;

      rv = nsComponentManager::CreateInstance(kCImapDB, nsnull,
                                              NS_GET_IID(nsIMsgDatabase),
                                              (void **) getter_AddRefs(mailDBFactory));
      if (NS_FAILED(rv)) return rv;

      nsCOMPtr <nsIDBFolderInfo> transferInfo;
      dbFolderInfo->GetTransferInfo(getter_AddRefs(transferInfo));
      if (mDatabase)
      {
        dbFolderInfo = null_nsCOMPtr();
        NotifyStoreClosedAllHeaders();
        mDatabase->ForceClosed();
      }
      mDatabase = null_nsCOMPtr();
      
      nsLocalFolderSummarySpec  summarySpec(dbName);
      // Remove summary file.
      summarySpec.Delete(PR_FALSE);
    
      // Create a new summary file, update the folder message counts, and
      // Close the summary file db.
      rv = mailDBFactory->OpenFolderDB(this, PR_TRUE, PR_TRUE, getter_AddRefs(mDatabase));

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
        if (transferInfo && mDatabase)
        {
          rv = mDatabase->GetDBFolderInfo(getter_AddRefs(dbFolderInfo));
          if (dbFolderInfo)
            dbFolderInfo->InitFromTransferInfo(transferInfo);
        }
        SummaryChanged();
        rv = NS_ERROR_UNEXPECTED;
        if (mDatabase) 
        {
          if(mAddListener)
            mDatabase->AddListener(this);
          rv = mDatabase->GetDBFolderInfo(getter_AddRefs(dbFolderInfo));
        }
      }
      // store the new UIDVALIDITY value

      if (NS_SUCCEEDED(rv) && dbFolderInfo)
          dbFolderInfo->SetImapUidValidity(folderValidity);
                        // delete all my msgs, the keys are bogus now
                      // add every message in this folder
      existingKeys.RemoveAll();
  //      keysToDelete.CopyArray(&existingKeys);

      if (flagState)
      {
        nsMsgKeyArray no_existingKeys;
          FindKeysToAdd(no_existingKeys, keysToFetch, flagState);
      }
      if (NS_FAILED(rv))
          dbName.Delete(PR_FALSE);

    }   
    else if (!flagState /*&& !NET_IsOffline() */) // if there are no messages on the server
    {
      keysToDelete.CopyArray(&existingKeys);
    }
    else /* if ( !NET_IsOffline()) */
    {
      FindKeysToDelete(existingKeys, keysToDelete, flagState);
        
      PRUint32 boxFlags;

      aSpec->GetBox_flags(&boxFlags);
      // if this is the result of an expunge then don't grab headers
      if (!(boxFlags & kJustExpunged))
        FindKeysToAdd(existingKeys, keysToFetch, flagState);
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
        SetBiffState(nsIMsgFolder::nsMsgBiffState_NewMail);
        if (flagState)
        {
          flagState->GetNumberOfRecentMessages(&numRecentMessages);
          SetNumNewMessages(numRecentMessages);
        }
      }
    }
    SyncFlags(flagState);
    if (keysToFetch.GetSize())
    {     
          PrepareToAddHeadersToMailDB(aProtocol, keysToFetch, aSpec);
    }
    else 
    {
            // let the imap libnet module know that we don't need headers
      if (aProtocol)
        aProtocol->NotifyHdrsToDownload(NULL, 0);
      PRBool gettingNewMessages;
      GetGettingNewMessages(&gettingNewMessages);
      if (gettingNewMessages)
      {
        ProgressStatus(aProtocol,IMAP_NO_NEW_MESSAGES, nsnull);
      }

//      NotifyFetchAnyNeededBodies(aSpec->connection, mailDB);
//      IMAP_BodyIdMonitor(adoptedBoxSpec->connection, PR_FALSE);
    }
    if (aProtocol)
    {
      if (m_downloadingFolderForOfflineUse)
      {
        nsMsgKeyArray keysToDownload;
        GetBodysToDownload(&keysToDownload);
        if (keysToDownload.GetSize() > 0)
          SetNotifyDownloadedLines(PR_TRUE); // ### TODO need to clear this when we've finished

        aProtocol->NotifyBodysToDownload(keysToDownload.GetArray(), keysToDownload.GetSize());
      }
      else
        aProtocol->NotifyBodysToDownload(NULL, 0/*keysToFetch.GetSize() */);
    }
  }

  return rv;
}

NS_IMETHODIMP nsImapMailFolder::UpdateImapMailboxStatus(
  nsIImapProtocol* aProtocol, nsIMailboxSpec* aSpec)
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
  nsIImapProtocol* aProtocol, PRBool* aBool)
{
  nsresult rv = NS_ERROR_FAILURE;
  return rv;
}

NS_IMETHODIMP nsImapMailFolder::SetupHeaderParseStream(
    nsIImapProtocol* aProtocol, PRUint32 aSize, const char *content_type, nsIMailboxSpec *boxSpec)
{
    nsresult rv = NS_ERROR_FAILURE;

  if (!mDatabase)
    GetDatabase(nsnull);

  if (mFlags & MSG_FOLDER_FLAG_INBOX && !m_filterList)
  {
      nsCOMPtr<nsIMsgIncomingServer> server;
      rv = GetServer(getter_AddRefs(server));
      if (NS_SUCCEEDED(rv) && server)
          server->GetFilterList(getter_AddRefs(m_filterList));
  }
  m_nextMessageByteLength = aSize;
  if (!m_msgParser)
  {
    rv = nsComponentManager::CreateInstance(kParseMailMsgStateCID, nsnull, 
      NS_GET_IID(nsIMsgParseMailMsgState), (void **) getter_AddRefs(m_msgParser));
  }
  else
    m_msgParser->Clear();
  
  if (m_msgParser)
    {
        m_msgParser->SetMailDB(mDatabase);
    return
            m_msgParser->SetState(nsIMsgParseMailMsgState::ParseHeadersState);
    }
  else
    {
    return NS_ERROR_OUT_OF_MEMORY;
    }
    return rv;
}

NS_IMETHODIMP nsImapMailFolder::ParseAdoptedHeaderLine(
    nsIImapProtocol* aProtocol, const char *aMessageLine, PRUint32 aMsgKey)
{
    // we can get blocks that contain more than one line, 
    // but they never contain partial lines
  const char *str = aMessageLine;
  m_curMsgUid = aMsgKey;
  m_msgParser->SetEnvelopePos(m_curMsgUid); // OK, this is silly (but
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
  {
    nsMailboxParseState parseState;
    m_msgParser->GetState(&parseState);
    if (parseState == nsIMsgParseMailMsgState::ParseHeadersState)
    m_msgParser->ParseAFolderLine(CRLF, 2);
    m_msgParser->GetNewMsgHdr(getter_AddRefs(newMsgHdr));
  }
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
      PRUint32 msgFlags;

      newMsgHdr->GetFlags(&msgFlags);
      if (!(msgFlags & (MSG_FLAG_READ | MSG_FLAG_IMAP_DELETED))) // only fire on unread msgs that haven't been deleted
      {
        rv = m_msgParser->GetAllHeaders(&headers, &headersSize);

        if (NS_SUCCEEDED(rv) && headers)
        {
          if (m_filterList)
          {
            nsCOMPtr <nsIMsgWindow> msgWindow;
            if (aProtocol)
            {
              nsCOMPtr <nsIImapUrl> aImapUrl;
              nsCOMPtr <nsIMsgMailNewsUrl> msgUrl;
              rv = aProtocol->GetRunningImapURL(getter_AddRefs(aImapUrl));
              if (NS_SUCCEEDED(rv) && aImapUrl)
              {
                msgUrl = do_QueryInterface(aImapUrl);
                if (msgUrl)
                  msgUrl->GetMsgWindow(getter_AddRefs(msgWindow));
              }

            }
            if (!m_moveCoalescer)
              m_moveCoalescer = new nsImapMoveCoalescer(this, msgWindow);
            m_filterList->ApplyFiltersToHdr(nsMsgFilterType::InboxRule, newMsgHdr, this, mDatabase, 
                                            headers, headersSize, this);
          }
        }
      }
    }
    // here we need to tweak flags from uid state..
    if (mDatabase && (!m_msgMovedByFilter || ShowDeletedMessages()))
      mDatabase->AddNewHdrToDB(newMsgHdr, PR_TRUE);
    m_msgParser->Clear(); // clear out parser, because it holds onto a msg hdr.
    // I don't think we want to do this - it does bad things like set the size incorrectly.
//    m_msgParser->FinishHeader();
  }
    return NS_OK;
}
    
NS_IMETHODIMP nsImapMailFolder::AbortHeaderParseStream(nsIImapProtocol*
                                                       aProtocol)
{
    nsresult rv = NS_ERROR_FAILURE;
    return rv;
}

NS_IMETHODIMP nsImapMailFolder::BeginCopy(nsIMsgDBHdr *message)
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

  nsSpecialSystemDirectory tmpFileSpec(nsSpecialSystemDirectory::OS_TemporaryDirectory);
  
  tmpFileSpec += "nscpmsg.txt";
  tmpFileSpec.MakeUnique();
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

    PRUint32 readCount, maxReadCount = FOUR_K - m_copyState->m_leftOver;
    PRInt32 writeCount;
    char *start, *end;
    PRUint32 linebreak_len = 0;

    while (aLength > 0)
    {
        if (aLength < (PRInt32) maxReadCount)
            maxReadCount = aLength;
        rv = aIStream->Read(m_copyState->m_dataBuffer+m_copyState->m_leftOver,
                            maxReadCount,
                            &readCount);
        if (NS_FAILED(rv)) return rv;

        m_copyState->m_leftOver += readCount;
        m_copyState->m_dataBuffer[m_copyState->m_leftOver] = '\0';

        start = m_copyState->m_dataBuffer;
        end = PL_strstr(start, "\r");
        if (!end)
            end = PL_strstr(start, "\n");
        else if (*(end+1) == nsCRT::LF && linebreak_len == 0)
            linebreak_len = 2;

        if (linebreak_len == 0) // not initialize yet
            linebreak_len = 1;

        aLength -= readCount;
        maxReadCount = FOUR_K - m_copyState->m_leftOver;

        if (!end && aLength > (PRInt32) maxReadCount)
            // must be a very very long line; sorry cannot handle it
            return NS_ERROR_FAILURE;

        while (start && end)
        {
            if (PL_strncasecmp(start, "X-Mozilla-Status:", 17) &&
                PL_strncasecmp(start, "X-Mozilla-Status2:", 18) &&
                PL_strncmp(start, "From - ", 7))
            {
                rv = m_copyState->m_tmpFileSpec->Write(start,
                                                       end-start,
                                                       &writeCount);
                rv = m_copyState->m_tmpFileSpec->Write(CRLF, 2, &writeCount);
            }
            start = end+linebreak_len;
            if (start >=
                m_copyState->m_dataBuffer+m_copyState->m_leftOver)
            {
                maxReadCount = FOUR_K;
                m_copyState->m_leftOver = 0;
                break;
            }
            end = PL_strstr(start, "\r");
            if (!end)
                end = PL_strstr(start, "\n");
            if (start && !end)
            {
                m_copyState->m_leftOver -= (start - m_copyState->m_dataBuffer);
                nsCRT::memcpy(m_copyState->m_dataBuffer, start,
                              m_copyState->m_leftOver+1); // including null
                maxReadCount = FOUR_K - m_copyState->m_leftOver;
            }
        }
        if (NS_FAILED(rv)) return rv;
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
        rv = QueryInterface(NS_GET_IID(nsIUrlListener),
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

NS_IMETHODIMP nsImapMailFolder::EndMove()
{
  return NS_OK;
}
// this is the beginning of the next message copied
NS_IMETHODIMP nsImapMailFolder::StartMessage()
{
  return NS_OK;
}

// just finished the current message.
NS_IMETHODIMP nsImapMailFolder::EndMessage(nsMsgKey key)
{
  return NS_OK;
}

NS_IMETHODIMP nsImapMailFolder::ApplyFilterHit(nsIMsgFilter *filter, PRBool *applyMore)
{
  nsMsgRuleActionType actionType;
  nsXPIDLCString actionTargetFolderUri;
  PRUint32  newFlags;
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
  if (NS_SUCCEEDED(filter->GetAction(&actionType)))
  {

    if (actionType == nsMsgFilterAction::MoveToFolder)
        filter->GetActionTargetFolderUri(getter_Copies(actionTargetFolderUri));
    nsCOMPtr<nsIMsgDBHdr> msgHdr;

    if (m_msgParser)
      m_msgParser->GetNewMsgHdr(getter_AddRefs(msgHdr));
    if (NS_SUCCEEDED(rv) && msgHdr)

    {
      PRUint32 msgFlags;
      nsMsgKey    msgKey;
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
            rv = mailTrash->GetURI(getter_Copies(actionTargetFolderUri));

          msgHdr->OrFlags(MSG_FLAG_READ, &newFlags);  // mark read in trash.
        }
        else  // (!deleteToTrash)
        {
          msgHdr->OrFlags(MSG_FLAG_READ | MSG_FLAG_IMAP_DELETED, &newFlags);
          nsMsgKeyArray keysToFlag;

          keysToFlag.Add(msgKey);
          StoreImapFlags(kImapMsgSeenFlag | kImapMsgDeletedFlag, PR_TRUE, keysToFlag.GetArray(), keysToFlag.GetSize());
          m_msgMovedByFilter = PR_TRUE; // this will prevent us from adding the header to the db.
        }
      }
      // note that delete falls through to move.
      case nsMsgFilterAction::MoveToFolder:
      {
        // if moving to a different file, do it.
        nsXPIDLCString uri;
        rv = GetURI(getter_Copies(uri));

        if ((const char*)actionTargetFolderUri &&
            nsCRT::strcasecmp(uri, actionTargetFolderUri))
        {
          msgHdr->GetFlags(&msgFlags);

          if (msgFlags & MSG_FLAG_MDN_REPORT_NEEDED &&
            !isRead)
          {

#if DOING_MDN // leave it to the user aciton
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
          nsresult err = MoveIncorporatedMessage(msgHdr, mDatabase, actionTargetFolderUri, filter);
          if (NS_SUCCEEDED(err))
            m_msgMovedByFilter = PR_TRUE;
        }
      }
      // don't apply any more filters, even if it was a move to the same folder
        *applyMore = PR_FALSE; 
        break;
      case nsMsgFilterAction::MarkRead:
        {
          nsMsgKeyArray keysToFlag;

          keysToFlag.Add(msgKey);
          StoreImapFlags(kImapMsgSeenFlag, PR_TRUE, keysToFlag.GetArray(), keysToFlag.GetSize());
        }
//        MarkFilteredMessageRead(msgHdr);
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
          {
              nsMsgPriorityValue filterPriority;
              filter->GetActionPriority(&filterPriority);
              msgHdr->SetPriority(filterPriority);
          }
        break;
      default:
        break;
      }
    }
  }
  return rv;
}

NS_IMETHODIMP nsImapMailFolder::SetImapFlags(const char *uids, PRInt32 flags, nsIURI **url)
{
  nsresult rv;
  NS_WITH_SERVICE(nsIImapService, imapService, kCImapService, &rv);
  if (NS_SUCCEEDED(rv))
  {
    rv = imapService->SetMessageFlags(m_eventQueue, this, this,
                                 url, uids, flags, PR_TRUE);
  }
  return rv;
}

// "this" is the parent folder
NS_IMETHODIMP nsImapMailFolder::PlaybackOfflineFolderCreate(const PRUnichar *folderName, nsIMsgWindow *aWindow, nsIURI **url)
{
    nsresult rv = NS_ERROR_NULL_POINTER;
    if (!folderName) return rv;
    NS_WITH_SERVICE(nsIImapService, imapService, kCImapService, &rv);
    if (NS_SUCCEEDED(rv))
        rv = imapService->CreateFolder(m_eventQueue, this, 
                                       folderName, this, url);
    return rv;
}

NS_IMETHODIMP nsImapMailFolder::ReplayOfflineMoveCopy(nsMsgKey *msgKeys, PRInt32 numKeys, PRBool isMove, nsIMsgFolder *aDstFolder,
                         nsIUrlListener *aUrlListener, nsIMsgWindow *aWindow)
{
  nsresult rv;
  NS_WITH_SERVICE(nsIImapService, imapService, kCImapService, &rv);
  if (NS_SUCCEEDED(rv))
  {
    nsCAutoString uids;
    AllocateUidStringFromKeys(msgKeys, numKeys, uids);
    rv = imapService->OnlineMessageCopy(m_eventQueue, 
                         this,
                         uids,
                         aDstFolder,
                         PR_TRUE,
                         isMove,
                         aUrlListener,
                         nsnull, nsnull, aWindow);

  }
  return rv;
}

NS_IMETHODIMP nsImapMailFolder::StoreImapFlags(PRInt32 flags, PRBool addFlags, nsMsgKey *keys, PRInt32 numKeys)
{
  nsresult rv = NS_OK;
  if (!WeAreOffline())
  {
    NS_WITH_SERVICE(nsIImapService, imapService, kCImapService, &rv);
    if (NS_SUCCEEDED(rv))
    {
      nsCAutoString msgIds;
      AllocateUidStringFromKeys(keys, numKeys, msgIds);

      if (addFlags)
      {
        imapService->AddMessageFlags(m_eventQueue, this, this,
                                     nsnull, msgIds, flags, PR_TRUE);
      }
      else
      {
        imapService->SubtractMessageFlags(m_eventQueue, this, this,
                                          nsnull, msgIds, flags,
                                          PR_TRUE);
      }
    }
  }
  else
  {
    GetDatabase(nsnull);
    if (mDatabase)
    {
//      UndoManager *undoManager = NULL;
      PRUint32 total = numKeys;

      for (PRUint32 keyIndex=0; keyIndex < total; keyIndex++)
      {
        nsCOMPtr <nsIMsgOfflineImapOperation> op;
        rv = mDatabase->GetOfflineOpForKey(keys[keyIndex], PR_TRUE, getter_AddRefs(op));
        SetFlag(MSG_FOLDER_FLAG_OFFLINEEVENTS);
        if (NS_SUCCEEDED(rv) && op)
        {
#if 0
          MailDB *originalDB = NULL;
          if (op->GetOperationFlags() & kMoveResult)
          {
            // get the op in the source db and change the flags there
            OfflineImapOperation  *originalOp = GetOriginalOp(op, &originalDB);
            if (originalOp)
            {
              if (undoManager && undoManager->GetState() == UndoIdle && NET_IsOffline()) {
                OfflineIMAPUndoAction *undoAction = new 
                    OfflineIMAPUndoAction(paneForFlagUrl, (MSG_FolderInfo*) this, op->GetMessageKey(), kFlagsChanged,
                    this, NULL, flags, NULL, addFlags);
                if (undoAction)
                  undoManager->AddUndoAction(undoAction);
              }
              op = originalOp;
            }
          }
#endif   
          imapMessageFlagsType newFlags;
          op->GetNewFlags(&newFlags);

          if (addFlags)
            op->SetFlagOperation(newFlags | flags);
          else
            op->SetFlagOperation(newFlags & ~flags);
#if 0
          if (originalDB)
          {
            originalDB->Close();
            originalDB = NULL;
          }
#endif
        }
      }
      mDatabase->Commit(nsMsgDBCommitType::kLargeCommit); // flush offline flags
    }
  }
  return rv;
}

NS_IMETHODIMP nsImapMailFolder::LiteSelect(nsIUrlListener *aUrlListener)
{
  nsresult rv;
  NS_WITH_SERVICE(nsIImapService, imapService, kCImapService, &rv);
  if (NS_SUCCEEDED(rv))
    rv = imapService->LiteSelectFolder(m_eventQueue, this, aUrlListener, nsnull);
  return rv;
}

nsresult nsImapMailFolder::MoveIncorporatedMessage(nsIMsgDBHdr *mailHdr, 
                                                   nsIMsgDatabase *sourceDB, 
                                                   const char *destFolderUri,
                                                   nsIMsgFilter *filter)
{
  nsresult err = NS_OK;
  
  if (m_moveCoalescer)
  {

    NS_WITH_SERVICE(nsIRDFService, rdf, kRDFServiceCID, &err); 
    nsCOMPtr<nsIRDFResource> res;
    err = rdf->GetResource(destFolderUri, getter_AddRefs(res));
    if (NS_FAILED(err))
      return err;

    nsCOMPtr<nsIMsgFolder> destIFolder(do_QueryInterface(res, &err));
    if (NS_FAILED(err))
      return err;        

    if (destIFolder)
    {
      // check if the destination is a real folder (by checking for null parent)
      // and if it can file messages (e.g., servers or news folders can't file messages).
      // Or read only imap folders...
      PRBool canFileMessages = PR_TRUE;
      nsCOMPtr<nsIFolder> parentFolder;
      destIFolder->GetParent(getter_AddRefs(parentFolder));
      destIFolder->GetCanFileMessages(&canFileMessages);
      if (!parentFolder || !canFileMessages)
      {
        filter->SetEnabled(PR_FALSE);
        return NS_MSG_NOT_A_MAIL_FOLDER;
      }
      // put the header into the source db, since it needs to be there when we copy it
      // and we need a valid header to pass to StartAsyncCopyMessagesInto
      nsMsgKey keyToFilter;
      mailHdr->GetMessageKey(&keyToFilter);

      if (sourceDB && destIFolder)
      {
        PRBool imapDeleteIsMoveToTrash = DeleteIsMoveToTrash();

        m_moveCoalescer->AddMove (destIFolder, keyToFilter);
        // For each folder, we need to keep track of the ids we want to move to that
        // folder - we used to store them in the MSG_FolderInfo and then when we'd finished
        // downloading headers, we'd iterate through all the folders looking for the ones
        // that needed messages moved into them - perhaps instead we could
        // keep track of nsIMsgFolder, nsMsgKeyArray pairs here in the imap code.
//        nsMsgKeyArray *idsToMoveFromInbox = msgFolder->GetImapIdsToMoveFromInbox();
//        idsToMoveFromInbox->Add(keyToFilter);

        // this is our last best chance to log this
//        if (m_filterList->LoggingEnabled())
//          filter->LogRuleHit(GetLogFile(), mailHdr);

        if (imapDeleteIsMoveToTrash)
        {
        }
        
        destIFolder->SetFlag(MSG_FOLDER_FLAG_GOT_NEW);
        
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
void nsImapMailFolder::FindKeysToDelete(const nsMsgKeyArray &existingKeys, nsMsgKeyArray &keysToDelete, nsIImapFlagAndUidState *flagState)
{
  PRBool showDeletedMessages = ShowDeletedMessages();
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
       ((flags & kImapMsgDeletedFlag) && !showDeletedMessages) )
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

void nsImapMailFolder::FindKeysToAdd(const nsMsgKeyArray &existingKeys, nsMsgKeyArray &keysToFetch, nsIImapFlagAndUidState *flagState)
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
      if (uidOfMessage != nsMsgKey_None &&showDeletedMessages || ! (flags & kImapMsgDeletedFlag))
      {
        keysToFetch.Add(uidOfMessage);
      }
    }
  }
}

void nsImapMailFolder::PrepareToAddHeadersToMailDB(nsIImapProtocol* aProtocol, const nsMsgKeyArray &keysToFetch,
                                                nsIMailboxSpec *boxSpec)
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
          boxSpec->flagState = NULL;    // adopted by ParseIMAPMailboxState
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
    nsresult rv = NS_ERROR_FAILURE;
//    if (!aFileSpec)
//        return NS_ERROR_NULL_POINTER;
    nsFileSpec fileSpec (aNativeString);
//    aFileSpec->GetFileSpec(&fileSpec);
  fileSpec.Delete(PR_FALSE);
  nsCOMPtr<nsISupports>  supports;
  rv = NS_NewIOFileStream(getter_AddRefs(supports), fileSpec, PR_WRONLY | PR_CREATE_FILE | PR_TRUNCATE, 00700);
  m_tempMessageStream = do_QueryInterface(supports);
    if (m_tempMessageStream && addDummyEnvelope)
    {
        nsCAutoString result;
        char *ct;
        PRUint32 writeCount;
        time_t now = time ((time_t*) 0);
        ct = ctime(&now);
        ct[24] = 0;
        result = "From - ";
        result += ct;
        result += MSG_LINEBREAK;
        
        m_tempMessageStream->Write(result.get(), result.Length(),
                                   &writeCount);
        result = "X-Mozilla-Status: 0001";
        result += MSG_LINEBREAK;
        m_tempMessageStream->Write(result.get(), result.Length(),
                                   &writeCount);
        result =  "X-Mozilla-Status2: 00000000";
        result += MSG_LINEBREAK;
        m_tempMessageStream->Write(result.get(), result.Length(),
                                   &writeCount);
    }
    return rv;
}

NS_IMETHODIMP nsImapMailFolder::DownloadMessagesForOffline(nsISupportsArray *messages, nsIMsgWindow *window)
{
  nsCAutoString messageIds;
  nsMsgKeyArray srcKeyArray;
#ifdef DEBUG_bienvenu
//  return DownloadAllForOffline(nsnull, window);
#endif
  nsresult rv = BuildIdsAndKeyArray(messages, messageIds, srcKeyArray);
  if (NS_FAILED(rv)) return rv;
  NS_WITH_SERVICE(nsIImapService, imapService, kCImapService, &rv);
  if (NS_FAILED(rv)) return rv;

  SetNotifyDownloadedLines(PR_TRUE); // ### TODO need to clear this when we've finished
  rv = imapService->DownloadMessagesForOffline(messageIds, this, nsnull, window);
  return rv;
}

NS_IMETHODIMP nsImapMailFolder::DownloadAllForOffline(nsIUrlListener *listener, nsIMsgWindow *msgWindow)
{
  nsresult rv = NS_OK;
  nsCOMPtr <nsIURI> runningURI;
  PRBool noSelect;
  GetFlag(MSG_FOLDER_FLAG_IMAP_NOSELECT, &noSelect);

  if (!noSelect)
  {
    nsCAutoString messageIdsToDownload;
    nsMsgKeyArray msgsToDownload;

    GetDatabase(msgWindow);
    GetBodysToDownload(&msgsToDownload);
    if (msgsToDownload.GetSize() == 0)
    {
      if (listener)
        listener->OnStopRunningUrl(nsnull, NS_OK);
      return NS_OK;
    }
    rv = AllocateUidStringFromKeys(msgsToDownload.GetArray(), msgsToDownload.GetSize(), messageIdsToDownload);
    NS_ENSURE_SUCCESS(rv, rv);

    SetNotifyDownloadedLines(PR_TRUE); // ### TODO need to clear this when we've finished
     NS_WITH_SERVICE(nsIImapService, imapService, kCImapService, &rv);
    if (NS_FAILED(rv)) return rv;
    rv = imapService->DownloadMessagesForOffline(messageIdsToDownload, this, listener, msgWindow);
    if (NS_SUCCEEDED(rv))
      m_urlRunning = PR_TRUE;
  }
  return rv;
}

NS_IMETHODIMP
nsImapMailFolder::GetNotifyDownloadedLines(PRBool *notifyDownloadedLines)
{
  NS_ENSURE_ARG(notifyDownloadedLines);
  *notifyDownloadedLines = m_downloadMessageForOfflineUse;
  return NS_OK;
}

NS_IMETHODIMP
nsImapMailFolder::SetNotifyDownloadedLines(PRBool notifyDownloadedLines)
{
  m_downloadMessageForOfflineUse = notifyDownloadedLines;
  return NS_OK;
}

NS_IMETHODIMP 
nsImapMailFolder::ParseAdoptedMsgLine(const char *adoptedMessageLine, nsMsgKey uidOfMessage)
{
  PRUint32 count = 0;
  nsresult rv = NS_OK;
  // remember the uid of the message we're downloading.
  m_curMsgUid = uidOfMessage;
  if (m_downloadMessageForOfflineUse && !m_tempMessageStream)
  {
    GetMessageHeader(uidOfMessage, getter_AddRefs(m_offlineHeader));
    rv = StartNewOfflineMessage();
  }

  if (m_tempMessageStream)
  {
     rv = m_tempMessageStream->Write(adoptedMessageLine, 
                  PL_strlen(adoptedMessageLine), &count);
     NS_ASSERTION(NS_SUCCEEDED(rv), "failed to write to stream");
  }
                                                                                
  return rv;
}
    
NS_IMETHODIMP
nsImapMailFolder::NormalEndMsgWriteStream(nsMsgKey uidOfMessage, PRBool markRead)
{
  nsresult res = NS_OK;
  PRBool commit = PR_FALSE;

  if (m_offlineHeader)
  {
    EndNewOfflineMessage();
    commit = PR_TRUE;
  }
  if (m_tempMessageStream)
  {
    m_tempMessageStream->Close();
    m_tempMessageStream = nsnull;
  }
  if (markRead)
  {
    nsCOMPtr<nsIMsgDBHdr> msgHdr;

    m_curMsgUid = uidOfMessage;
    res = GetMessageHeader(m_curMsgUid, getter_AddRefs(msgHdr));
    if (NS_SUCCEEDED(res))
    {
      msgHdr->MarkRead(PR_TRUE);
      commit = PR_TRUE;
    }

  }
  if (commit && mDatabase)
    mDatabase->Commit(nsMsgDBCommitType::kLargeCommit);
  return res;
}

NS_IMETHODIMP
nsImapMailFolder::AbortMsgWriteStream()
{
    return NS_ERROR_FAILURE;
}

    // message move/copy related methods
NS_IMETHODIMP 
nsImapMailFolder::OnlineCopyCompleted(nsIImapProtocol *aProtocol, ImapOnlineCopyState aCopyState)
{
  NS_ENSURE_ARG_POINTER(aProtocol);

  nsresult rv;
  if (aCopyState == ImapOnlineCopyStateType::kSuccessfulCopy)
  {
      
    nsCOMPtr <nsIImapUrl> imapUrl;
    rv = aProtocol->GetRunningImapURL(getter_AddRefs(imapUrl));
        if (NS_FAILED(rv) || !imapUrl) return NS_ERROR_FAILURE;
    nsImapAction action;
    rv = imapUrl->GetImapAction(&action);
    if (NS_FAILED(rv)) return rv;
    
    if (action == nsIImapUrl::nsImapOnlineToOfflineMove)
    {
        nsXPIDLCString messageIds;
        rv = imapUrl->CreateListOfMessageIdsString(getter_Copies(messageIds));

        if (NS_FAILED(rv)) return rv;
        nsCOMPtr<nsIEventQueue> queue;  
        // get the Event Queue for this thread...
        NS_WITH_SERVICE(nsIEventQueueService, pEventQService,
                        kEventQueueServiceCID, &rv);
        if (NS_FAILED(rv)) return rv;

        rv = pEventQService->GetThreadEventQueue(NS_CURRENT_THREAD,
                                                 getter_AddRefs(queue));
        if (NS_FAILED(rv)) return rv;
        
        NS_WITH_SERVICE(nsIImapService, imapService, kCImapService, &rv);
        if (NS_FAILED(rv)) return rv;
   
        rv = imapService->AddMessageFlags(queue, this, nsnull, nsnull,
                                          messageIds,
                                          kImapMsgDeletedFlag,
                                          PR_TRUE);
      if (NS_SUCCEEDED(rv))
      {
        nsMsgKeyArray affectedMessages;
        char *keyTokenString = nsCRT::strdup(messageIds);
        ParseUidString(keyTokenString, affectedMessages);
        if (mDatabase) 
          mDatabase->DeleteMessages(&affectedMessages,NULL);
        nsCRT::free(keyTokenString);
        return rv;
      }
    }
    /* unhandled action */
    else return NS_ERROR_FAILURE;
  }

  /* unhandled copystate */
  else 
  {
    // whoops, this is the wrong folder - should use the source folder
    if (m_copyState)
    {
       nsCOMPtr<nsIMsgFolder> srcFolder;
       srcFolder = do_QueryInterface(m_copyState->m_srcSupport, &rv);
       if (srcFolder)
        srcFolder->NotifyFolderEvent(mDeleteOrMoveMsgCompletedAtom);
    }
    return NS_ERROR_FAILURE;
  }

  return rv;
}

NS_IMETHODIMP
nsImapMailFolder::PrepareToReleaseUrl(nsIMsgMailNewsUrl * aUrl)
{
  mUrlToRelease = aUrl;
  return NS_OK;
}

NS_IMETHODIMP
nsImapMailFolder::ReleaseUrl()
{
  mUrlToRelease = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
nsImapMailFolder::CloseMockChannel(nsIImapMockChannel * aChannel)
{
  aChannel->Close();
  return NS_OK;
}

NS_IMETHODIMP
nsImapMailFolder::BeginMessageUpload()
{
    return NS_ERROR_FAILURE;
}

// synchronize the message flags in the database with the server flags
nsresult nsImapMailFolder::SyncFlags(nsIImapFlagAndUidState *flagState)
{
    // update all of the database flags
  PRInt32 messageIndex;
  
  flagState->GetNumberOfMessages(&messageIndex);

  for (PRInt32 flagIndex=0; flagIndex < messageIndex; flagIndex++)
  {
    PRUint32 uidOfMessage;
    flagState->GetUidOfMessage(flagIndex, &uidOfMessage);
    imapMessageFlagsType flags;
    flagState->GetMessageFlags(flagIndex, &flags);
    // ### dmb need to do something about imap deleted flag;
    NotifyMessageFlags(flags, uidOfMessage);
    }
  return NS_OK;
}

    // message flags operation
NS_IMETHODIMP
nsImapMailFolder::NotifyMessageFlags(PRUint32 flags, nsMsgKey msgKey)
{
  if (NS_SUCCEEDED(GetDatabase(nsnull)) && mDatabase)
  {
		nsCOMPtr<nsIMsgDBHdr> dbHdr;
		nsresult rv;
		PRBool containsKey;

		rv = mDatabase->ContainsKey(msgKey , &containsKey);
		// if we don't have the header, don't diddle the flags.
		// GetMsgHdrForKey will create the header if it doesn't exist.
		if (!NS_SUCCEEDED(rv) || !containsKey)
			return rv;

		rv = mDatabase->GetMsgHdrForKey(msgKey, getter_AddRefs(dbHdr));

		if(NS_SUCCEEDED(rv) && dbHdr)
		{
	    mDatabase->MarkHdrRead(dbHdr, (flags & kImapMsgSeenFlag) != 0, nsnull);
		  mDatabase->MarkHdrReplied(dbHdr, (flags & kImapMsgAnsweredFlag) != 0, nsnull);
			mDatabase->MarkHdrMarked(dbHdr, (flags & kImapMsgFlaggedFlag) != 0, nsnull);
			mDatabase->MarkImapDeleted(msgKey, (flags & kImapMsgDeletedFlag) != 0, nsnull);
		}
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
    if (affectedMessages.GetSize() > 0) // perhaps Search deleted these messages
    {
      GetDatabase(nsnull);
      if (mDatabase)
        mDatabase->DeleteMessages(&affectedMessages, nsnull);
    }
    
  }
  else if (doomedKeyString) // && !imapDeleteIsMoveToTrash
  {
    GetDatabase(nsnull);
    if (mDatabase)
      SetIMAPDeletedFlag(mDatabase, affectedMessages, nsnull);
  }
  PR_FREEIF(keyTokenString);
  return NS_OK;
}

PRBool nsImapMailFolder::ShowDeletedMessages()
{
  nsresult err;
    NS_WITH_SERVICE(nsIImapHostSessionList, hostSession,
                    kCImapHostSessionList, &err);
  PRBool showDeleted = PR_FALSE;

  if (NS_SUCCEEDED(err) && hostSession)
  {
    nsXPIDLCString serverKey;
    GetServerKey(getter_Copies(serverKey));
    err = hostSession->GetShowDeletedMessagesForHost(serverKey, showDeleted);
  }
  // check for special folders that need to show deleted messages
  if (!showDeleted)
  {
    nsCOMPtr<nsIImapIncomingServer> imapServer;
    nsresult rv = GetImapIncomingServer(getter_AddRefs(imapServer));

    if (NS_SUCCEEDED(rv) && imapServer) 
    {
      PRBool isAOLServer = PR_FALSE;
      imapServer->GetIsAOLServer(&isAOLServer);
      nsCOMPtr<nsIMsgIncomingServer> incomingServer (do_QueryInterface(imapServer));
      nsXPIDLCString hostName;
      incomingServer->GetHostName(getter_Copies(hostName));
      
      if (isAOLServer && ((const char *) hostName) && !nsCRT::strcmp(hostName, "imap.mail.aol.com"))
      {
        nsXPIDLString folderName;
        GetName(getter_Copies(folderName));
        if (!nsCRT::strncasecmp(folderName, "Trash", 5))
          showDeleted = PR_TRUE;
      }
    }
  }
  return showDeleted;
}


PRBool nsImapMailFolder::DeleteIsMoveToTrash()
{
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
    rv = rootFolder->GetFoldersWithFlag(MSG_FOLDER_FLAG_TRASH, 1, &numFolders, pTrashFolder);
	if (numFolders != 1)
		rv = NS_ERROR_FAILURE;
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
  int32 curToken;
  int32 saveStartToken=0;

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
nsImapMailFolder::GetMessageSizeFromDB(const char *id, PRBool idIsUid, PRUint32 *size)
{
  nsresult rv = NS_ERROR_FAILURE;
  NS_ENSURE_ARG(size);
  *size = 0;
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
nsImapMailFolder::SetContentModified(nsIImapUrl *aImapUrl, nsImapContentModifiedType modified)
{
  return aImapUrl->SetContentModified(modified);
}

// we provide this interface (nsIStreamListener) because the nsImapProtocol wants us to, in order
// to hook up the mock channel and other stuff when downloading messages for offline use.
// But we don't really need to do anything with these notifications because we use 
// the nsIImapMesageSink interfaces ParseAdoptedMessageLine and NormalEndMsgWriteStream
NS_IMETHODIMP nsImapMailFolder::OnDataAvailable(nsIRequest * /* request */, nsISupports *ctxt, nsIInputStream *aIStream, PRUint32 sourceOffset, PRUint32 aLength)
{
	nsresult rv = NS_OK;
	return rv;
}

NS_IMETHODIMP nsImapMailFolder::OnStartRequest(nsIRequest *request, nsISupports *ctxt)
{
	return NS_OK;
}

NS_IMETHODIMP nsImapMailFolder::OnStopRequest(nsIRequest *request, nsISupports *ctxt, nsresult aStatus)
{
  return NS_OK;
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
  m_downloadingFolderForOfflineUse = PR_FALSE;
  SetNotifyDownloadedLines(PR_FALSE);
    NS_WITH_SERVICE(nsIMsgMailSession, session, kMsgMailSessionCID, &rv); 
  if (aUrl)
  {
    nsCOMPtr<nsIMsgWindow> aWindow;
    nsCOMPtr<nsIMsgMailNewsUrl> mailUrl = do_QueryInterface(aUrl);
        nsCOMPtr<nsIImapUrl> imapUrl = do_QueryInterface(aUrl);
    PRBool folderOpen = PR_FALSE;
    if (mailUrl)
      mailUrl->GetMsgWindow(getter_AddRefs(aWindow));
    if (session)
      session->IsFolderOpenInWindow(this, &folderOpen);
#ifdef DEBUG_bienvenu1
    nsXPIDLCString urlSpec;
    aUrl->GetSpec(getter_Copies(urlSpec));
   printf("stop running url %s\n", (const char *) urlSpec);
#endif

   if (imapUrl)
    {
        nsImapAction imapAction = nsIImapUrl::nsImapTest;
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
              if (folderOpen)
                UpdateFolder(aWindow);
              else
                UpdatePendingCounts(PR_TRUE, PR_FALSE);
              if (m_copyState->m_isMove && !m_copyState->m_isCrossServerOp)
              {
                nsCOMPtr<nsIMsgFolder> srcFolder;
                srcFolder =
                    do_QueryInterface(m_copyState->m_srcSupport,
                                      &rv);
                nsCOMPtr<nsIMsgDatabase> srcDB;
                if (NS_SUCCEEDED(rv))
                    rv = srcFolder->GetMsgDatabase(aWindow,
                        getter_AddRefs(srcDB));
                if (NS_SUCCEEDED(rv) && srcDB)
                {
                    nsCOMPtr<nsImapMoveCopyMsgTxn> msgTxn;
                    nsMsgKeyArray srcKeyArray;
                    msgTxn =
                        do_QueryInterface(m_copyState->m_undoMsgTxn); 
                    if (msgTxn)
                        msgTxn->GetSrcKeyArray(srcKeyArray);
                  if (!ShowDeletedMessages())
                  {
                    EnableNotifications(allMessageCountNotifications, PR_FALSE);
                    srcDB->DeleteMessages(&srcKeyArray, nsnull);
                    EnableNotifications(allMessageCountNotifications, PR_TRUE);
                  }
                  else
                    MarkMessagesImapDeleted(&srcKeyArray, PR_TRUE, srcDB);
                }
                // even if we're showing deleted messages, 
                // we still need to notify FE so it will show the imap deleted flag
                srcFolder->NotifyFolderEvent(mDeleteOrMoveMsgCompletedAtom);
              }
              if (m_transactionManager)
                m_transactionManager->DoTransaction(m_copyState->m_undoMsgTxn);
            }
            ClearCopyState(aExitCode);
          }
          break;
        case nsIImapUrl::nsImapSubtractMsgFlags:
          {
          // this isn't really right - we'd like to know we were 
          // deleting a message to start with, but it probably
          // won't do any harm.
            imapMessageFlagsType flags = 0;
            imapUrl->GetMsgFlags(&flags);
            if (flags & kImapMsgDeletedFlag && !DeleteIsMoveToTrash())
            {
              nsCOMPtr<nsIMsgDatabase> db;
              rv = GetMsgDatabase(nsnull, getter_AddRefs(db));
              if (NS_SUCCEEDED(rv) && db)
              {
                nsMsgKeyArray keyArray;
                char *keyString;
                imapUrl->CreateListOfMessageIdsString(&keyString);
                if (keyString)
                {
                  ParseUidString(keyString, keyArray);
                  MarkMessagesImapDeleted(&keyArray, PR_FALSE, db);
                  db->Commit(nsMsgDBCommitType::kLargeCommit);
                  nsCRT::free(keyString);
                }
              }
            }
            NotifyFolderEvent(mDeleteOrMoveMsgCompletedAtom);

          }

          break;
        case nsIImapUrl::nsImapAddMsgFlags:
        {
            imapMessageFlagsType flags = 0;
            imapUrl->GetMsgFlags(&flags);
            if (flags & kImapMsgDeletedFlag && DeleteIsMoveToTrash())
            {
                nsCOMPtr<nsIMsgDatabase> db;
                rv = GetMsgDatabase(nsnull, getter_AddRefs(db));
                if (NS_SUCCEEDED(rv) && db)
                {
                    nsMsgKeyArray keyArray;
                    char *keyString = nsnull;
                    imapUrl->CreateListOfMessageIdsString(&keyString);
                    if (keyString)
                    {
                      ParseUidString(keyString, keyArray);
                      db->DeleteMessages(&keyArray, nsnull);
                      db->SetSummaryValid(PR_TRUE);
                      db->Commit(nsMsgDBCommitType::kLargeCommit);
                      nsCRT::free(keyString);
                    }
                }
            }
            NotifyFolderEvent(mDeleteOrMoveMsgCompletedAtom);
        }
          break;
        case nsIImapUrl::nsImapAppendMsgFromFile:
        case nsIImapUrl::nsImapAppendDraftFromFile:
            if (m_copyState)
            {
              if (folderOpen)
                UpdateFolder(aWindow);
              else
                UpdatePendingCounts(PR_TRUE, PR_FALSE);
              m_copyState->m_curIndex++;
              if (m_copyState->m_curIndex >= m_copyState->m_totalCount)
              {
                  if (m_transactionManager && m_copyState->m_undoMsgTxn)
                      m_transactionManager->DoTransaction(m_copyState->m_undoMsgTxn);
                  ClearCopyState(aExitCode);
              }
              else
                NS_ASSERTION(PR_FALSE, "not clearing copy state");
            }
            break;
        case nsIImapUrl::nsImapRenameFolder:
            break;
        default:
            break;
        }
    }
    // give base class a chance to send folder loaded notification...
    rv = nsMsgDBFolder::OnStopRunningUrl(aUrl, aExitCode);
    // query it for a mailnews interface for now....
    if (mailUrl)
      rv = mailUrl->UnRegisterListener(this);
  }
  SetGettingNewMessages(PR_FALSE); // if we're not running a url, we must not be getting new mail :-)
  if (mCopyListener)
	  mCopyListener->OnStopCopy(aExitCode);
  return rv;
}

void nsImapMailFolder::UpdatePendingCounts(PRBool countUnread, PRBool missingAreRead)
{
  nsresult rv;
  if (m_copyState)
  {
    if (!m_copyState->m_isCrossServerOp)
      ChangeNumPendingTotalMessages(m_copyState->m_totalCount);
    else
      ChangeNumPendingTotalMessages(1);

    if (countUnread)
    {
      // count the moves that were unread
      int numUnread = 0;
      nsCOMPtr <nsIMsgFolder> srcFolder = do_QueryInterface(m_copyState->m_srcSupport);
      if (!m_copyState->m_isCrossServerOp)
        for (PRUint32 keyIndex=0; keyIndex < m_copyState->m_totalCount; keyIndex++)
        {
          nsCOMPtr<nsIMsgDBHdr> message;

          nsCOMPtr<nsISupports> aSupport =
          getter_AddRefs(m_copyState->m_messages->ElementAt(keyIndex));
          message = do_QueryInterface(aSupport, &rv);
          // if the key is not there, then assume what the caller tells us to.
          PRBool isRead = missingAreRead;
          PRUint32 flags;
          if (message )
          {
            message->GetFlags(&flags);
            isRead = flags & MSG_FLAG_READ;
          }

          if (!isRead)
            numUnread++;
        }
      else
      {
        nsCOMPtr<nsIMsgDBHdr> message;

        nsCOMPtr<nsISupports> aSupport =
          getter_AddRefs(m_copyState->m_messages->ElementAt(m_copyState->m_curIndex));
        message = do_QueryInterface(aSupport, &rv);
          // if the key is not there, then assume what the caller tells us to.
        PRBool isRead = missingAreRead;
        PRUint32 flags;
        if (message )
        {
          message->GetFlags(&flags);
          isRead = flags & MSG_FLAG_READ;
        }

        if (!isRead)
          numUnread++;
      }
    
      if (numUnread)
        ChangeNumPendingUnread(numUnread);
    }
    SummaryChanged();
  }
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
                                     nsIImapUrl * aUrl)
{   // CopyMessages() only
  nsresult rv = NS_OK;
  nsCOMPtr<nsImapMoveCopyMsgTxn> msgTxn;
  nsCOMPtr<nsISupports> copyState;
  
  if (aUrl)
    aUrl->GetCopyState(getter_AddRefs(copyState));

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
nsImapMailFolder::StartMessage(nsIMsgMailNewsUrl * aUrl)
{
  nsCOMPtr<nsIImapUrl> imapUrl (do_QueryInterface(aUrl));
  nsCOMPtr<nsISupports> copyState;
  NS_ENSURE_TRUE(imapUrl, NS_ERROR_FAILURE);
  
  imapUrl->GetCopyState(getter_AddRefs(copyState));
  if (copyState)
  {
    nsCOMPtr <nsICopyMessageStreamListener> listener = do_QueryInterface(copyState);
    if (listener)
      listener->StartMessage();
  }

  return NS_OK;
}

NS_IMETHODIMP
nsImapMailFolder::EndMessage(nsIMsgMailNewsUrl * aUrl, nsMsgKey uidOfMessage)
{
  nsCOMPtr<nsIImapUrl> imapUrl (do_QueryInterface(aUrl));
  nsCOMPtr<nsISupports> copyState;
  NS_ENSURE_TRUE(imapUrl, NS_ERROR_FAILURE);
  
  imapUrl->GetCopyState(getter_AddRefs(copyState));
  if (copyState)
  {
    nsCOMPtr <nsICopyMessageStreamListener> listener = do_QueryInterface(copyState);
    if (listener)
      listener->EndMessage(uidOfMessage);
  }

  return NS_OK;
}

#define WHITESPACE " \015\012"     // token delimiter

NS_IMETHODIMP
nsImapMailFolder::NotifySearchHit(nsIMsgMailNewsUrl * aUrl, 
                                  const char* searchHitLine)
{
  nsresult rv = GetDatabase(nsnull /* don't need msg window, that's more for local mbox parsing */);
  if (!mDatabase || !NS_SUCCEEDED(rv))
    return rv;
  // expect search results in the form of "* SEARCH <hit> <hit> ..."
                // expect search results in the form of "* SEARCH <hit> <hit> ..."
  char *tokenString = nsCRT::strdup(searchHitLine);
  if (tokenString)
  {
      char *currentPosition = PL_strcasestr(tokenString, "SEARCH");
      if (currentPosition)
      {
        currentPosition += nsCRT::strlen("SEARCH");
        char *newStr;
          
          PRBool shownUpdateAlert = PR_FALSE;
          char *hitUidToken = nsCRT::strtok(currentPosition, WHITESPACE, &newStr);
          while (hitUidToken)
          {
            long naturalLong; // %l is 64 bits on OSF1
            sscanf(hitUidToken, "%ld", &naturalLong);
            nsMsgKey hitUid = (nsMsgKey) naturalLong;
        
            nsCOMPtr <nsIMsgDBHdr> hitHeader;
            rv = mDatabase->GetMsgHdrForKey(hitUid, getter_AddRefs(hitHeader));
            if (NS_SUCCEEDED(rv) && hitHeader)
            {
              nsCOMPtr <nsIMsgSearchSession> searchSession;
              nsCOMPtr <nsIMsgSearchAdapter> searchAdapter;
              aUrl->GetSearchSession(getter_AddRefs(searchSession));
              if (searchSession)
              {
                searchSession->GetRunningAdapter(getter_AddRefs(searchAdapter));
                if (searchAdapter)
                  searchAdapter->AddResultElement(hitHeader);
              }
            }
            else if (!shownUpdateAlert)
            {
#if 0 // can't do this yet
            FE_Alert(context, XP_GetString(MK_MSG_SEARCH_HITS_NOT_IN_DB));
            shownUpdateAlert = PR_TRUE;
#endif
            }
          
            hitUidToken = nsCRT::strtok(newStr, WHITESPACE, &newStr);
        }
    }

    nsCRT::free(tokenString);
  }
  else
    return NS_ERROR_OUT_OF_MEMORY;

  return NS_OK;
}


NS_IMETHODIMP
nsImapMailFolder::SetAppendMsgUid(nsIImapProtocol* aProtocol,
                                  nsMsgKey aKey,
                                  nsIImapUrl * aUrl)
{
  nsresult rv = NS_OK;
  nsCOMPtr<nsISupports> copyState;

  if (aUrl)
    aUrl->GetCopyState(getter_AddRefs(copyState));
  if (copyState)
  {
    nsCOMPtr<nsImapMailCopyState> mailCopyState = do_QueryInterface(copyState, &rv);
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
                               nsIImapUrl * aUrl)
{
  nsresult rv = NS_OK;
  nsCOMPtr<nsISupports> copyState;

  if (aUrl)
    aUrl->GetCopyState(getter_AddRefs(copyState));
  if (copyState)
  {
    nsCOMPtr<nsImapMailCopyState> mailCopyState =
        do_QueryInterface(copyState, &rv);
    if (NS_FAILED(rv)) return rv;
    if (mailCopyState->m_listener)
        rv = mailCopyState->m_listener->GetMessageId(messageId);
  }
  if (NS_SUCCEEDED(rv) && messageId->Length() > 0)
  {
      if (messageId->First() == '<')
          messageId->Cut(0, 1);
      if (messageId->Last() == '>')
          messageId->SetLength(messageId->Length() -1);
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
      pEventQService->GetThreadEventQueue(NS_CURRENT_THREAD,
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

NS_IMETHODIMP nsImapMailFolder::GetPath(nsIFileSpec ** aPathName)
{
  nsresult rv;
  if (! m_pathName) 
  {
    m_pathName = new nsNativeFileSpec("");
    if (! m_pathName)
       return NS_ERROR_OUT_OF_MEMORY;

    rv = nsImapURI2Path(kImapRootURI, mURI, *m_pathName);
    //    printf("constructing path %s\n", (const char *) *m_pathName);
    if (NS_FAILED(rv)) return rv;
  }
  rv = NS_NewFileSpecWithSpec(*m_pathName, aPathName);
  return NS_OK;
}


NS_IMETHODIMP nsImapMailFolder::SetPath(nsIFileSpec * aPathName)                
{                                                                               
  nsMsgFolder::SetPath(aPathName);   // call base class so mPath will get set too
  if (!aPathName)
     return NS_ERROR_NULL_POINTER;

  // not sure why imap has m_pathName and doesn't just use mPath.
  if (!m_pathName)
  {
    m_pathName = new nsFileSpec("");
    if (! m_pathName)
      return NS_ERROR_OUT_OF_MEMORY;
  }
  return aPathName->GetFileSpec(m_pathName);                                  
}                                                                               
                                                                                 

nsresult nsImapMailFolder::DisplayStatusMsg(nsIImapUrl *aImapUrl, const PRUnichar *msg)
{
  nsCOMPtr<nsIImapMockChannel> mockChannel;
  aImapUrl->GetMockChannel(getter_AddRefs(mockChannel));
  if (mockChannel)
  {
    nsCOMPtr<nsIProgressEventSink> progressSink;
    mockChannel->GetProgressEventSink(getter_AddRefs(progressSink));
    if (progressSink)
    {
        nsCOMPtr<nsIRequest> request = do_QueryInterface(mockChannel);
        if (!request) return NS_ERROR_FAILURE;
      progressSink->OnStatus(request, nsnull, NS_OK, msg);      // XXX i18n message
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsImapMailFolder::ProgressStatus(nsIImapProtocol* aProtocol,
                                 PRUint32 aMsgId, const PRUnichar *extraInfo)
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
      if (extraInfo)
      {
        PRUnichar *printfString = nsTextFormatter::smprintf(progressMsg, extraInfo);
        if (printfString)
        {
          progressMsg = nsCRT::strdup(printfString);  
          nsTextFormatter::smprintf_free(printfString);
        }
      }
      DisplayStatusMsg(imapUrl, progressMsg);
    }
  }
  PR_FREEIF(progressMsg);
  return NS_OK;
}

NS_IMETHODIMP
nsImapMailFolder::PercentProgress(nsIImapProtocol* aProtocol,
                                  ProgressInfo* aInfo)
{
  if (aProtocol)
  {
    nsCOMPtr <nsIImapUrl> imapUrl;
    aProtocol->GetRunningImapURL(getter_AddRefs(imapUrl));
    if (imapUrl)
    {
      nsCOMPtr<nsIImapMockChannel> mockChannel;
      imapUrl->GetMockChannel(getter_AddRefs(mockChannel));
      if (mockChannel)
      {
        nsCOMPtr<nsIProgressEventSink> progressSink;
        mockChannel->GetProgressEventSink(getter_AddRefs(progressSink));
        if (progressSink)
        {
            nsCOMPtr<nsIRequest> request = do_QueryInterface(mockChannel);
            if (!request) return NS_ERROR_FAILURE;

          progressSink->OnProgress(request, nsnull, aInfo->currentProgress, aInfo->maxProgress);
          if (aInfo->message)
            progressSink->OnStatus(request, nsnull, NS_OK, aInfo->message);      // XXX i18n message

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
                                        nsIImapUrl * aUrl)
{
    nsresult rv = NS_ERROR_NULL_POINTER;
    if (!aUrl) return rv;
    nsCOMPtr<nsISupports> copyState;
    aUrl->GetCopyState(getter_AddRefs(copyState));
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
                                   this, mailCopyState->m_msgWindow, mailCopyState->m_isMove);
        }
    }
    else if (mailCopyState->m_isMove)
    {
        nsCOMPtr<nsIMsgFolder> srcFolder =
            do_QueryInterface(mailCopyState->m_srcSupport, &rv);
        if (NS_SUCCEEDED(rv) && srcFolder)
        {
            srcFolder->DeleteMessages(mailCopyState->m_messages, nsnull,
                                      PR_TRUE, PR_TRUE, nsnull);
		    nsCOMPtr<nsIMsgLocalMailFolder> popFolder = do_QueryInterface(srcFolder); 
		    if (popFolder)   //needed if move pop->imap to notify FE
		        srcFolder->NotifyFolderEvent(mDeleteOrMoveMsgCompletedAtom);
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

    nsFileSpec tempPath(path.GetNativePathCString(), PR_TRUE);  // create incoming directories.

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
// used when copying from local mail folder, or other imap server)
nsresult
nsImapMailFolder::CopyMessagesWithStream(nsIMsgFolder* srcFolder,
                                nsISupportsArray* messages,
                                PRBool isMove,
                                PRBool isCrossServerOp,
                                nsIMsgWindow *msgWindow,
                                nsIMsgCopyServiceListener* listener)
{
    nsresult rv = NS_ERROR_NULL_POINTER;
    if (!srcFolder || !messages) return rv;

    nsCOMPtr<nsISupports> aSupport(do_QueryInterface(srcFolder, &rv));
    if (NS_FAILED(rv)) return rv;
    rv = InitCopyState(aSupport, messages, isMove, PR_FALSE, listener, msgWindow);
    if(NS_FAILED(rv)) return rv;

    m_copyState->m_streamCopy = PR_TRUE;
    m_copyState->m_isCrossServerOp = isCrossServerOp;

    // ** jt - needs to create server to server move/copy undo msg txn
    nsCAutoString messageIds;
    nsMsgKeyArray srcKeyArray;
    nsCOMPtr<nsIUrlListener> urlListener;

  rv = QueryInterface(NS_GET_IID(nsIUrlListener), getter_AddRefs(urlListener));
    rv = BuildIdsAndKeyArray(messages, messageIds, srcKeyArray);

    nsImapMoveCopyMsgTxn* undoMsgTxn = new nsImapMoveCopyMsgTxn(
        srcFolder, &srcKeyArray, messageIds.get(), this,
        PR_TRUE, isMove, m_eventQueue, urlListener);

    if (!undoMsgTxn) return NS_ERROR_OUT_OF_MEMORY;
    if (isMove)
    {
        if (mFlags & MSG_FOLDER_FLAG_TRASH)
            undoMsgTxn->SetTransactionType(nsIMessenger::eDeleteMsg);
        else
            undoMsgTxn->SetTransactionType(nsIMessenger::eMoveMsg);
    }
    else
    {
        undoMsgTxn->SetTransactionType(nsIMessenger::eCopyMsg);
    }
    
    rv = undoMsgTxn->QueryInterface(
        NS_GET_IID(nsImapMoveCopyMsgTxn), 
        getter_AddRefs(m_copyState->m_undoMsgTxn) );
    
  nsCOMPtr<nsISupports> msgSupport;
  msgSupport = getter_AddRefs(messages->ElementAt(0));
  if (msgSupport)
  {
    nsCOMPtr<nsIMsgDBHdr> aMessage;
    aMessage = do_QueryInterface(msgSupport, &rv);
    if (NS_SUCCEEDED(rv))
      CopyStreamMessage(aMessage, this, msgWindow, isMove);
    else
      ClearCopyState(rv);
  }
  else
  {
    rv = NS_ERROR_FAILURE;
  }
    return rv;
}

nsresult nsImapMailFolder::GetClearedOriginalOp(nsIMsgOfflineImapOperation *op, nsIMsgOfflineImapOperation **originalOp, nsIMsgDatabase **originalDB)
{
	nsIMsgOfflineImapOperation *returnOp = nsnull;
  nsOfflineImapOperationType opType;
  op->GetOperation(&opType);
  NS_ASSERTION(opType & nsIMsgOfflineImapOperation::kMoveResult, "not an offline move op");
	
	nsXPIDLCString sourceFolderURI;
	op->GetSourceFolderURI(getter_Copies(sourceFolderURI));
	
  nsCOMPtr<nsIRDFResource> res;
  nsresult rv;

  NS_WITH_SERVICE(nsIRDFService, rdf, kRDFServiceCID, &rv);
  if (NS_FAILED(rv)) 
    return rv; 
  rv = rdf->GetResource(sourceFolderURI, getter_AddRefs(res));
  if (NS_SUCCEEDED(rv))
  {
    nsCOMPtr<nsIMsgFolder> sourceFolder(do_QueryInterface(res, &rv));
    if (NS_SUCCEEDED(rv) && sourceFolder)
    {
	    if (sourceFolder)
	    {
        nsCOMPtr <nsIDBFolderInfo> folderInfo;
        sourceFolder->GetDBFolderInfoAndDB(getter_AddRefs(folderInfo), originalDB);
		    if (*originalDB)
		    {
          nsMsgKey originalKey;
          op->GetMessageKey(&originalKey);
          rv = (*originalDB)->GetOfflineOpForKey(originalKey, PR_FALSE, &returnOp);
			    if (NS_SUCCEEDED(rv) && returnOp)
			    {
				    nsXPIDLCString moveDestination;
            nsXPIDLCString thisFolderURI;

            GetURI(getter_Copies(thisFolderURI));

				    returnOp->GetDestinationFolderURI(getter_Copies(moveDestination));
            if (!nsCRT::strcmp(moveDestination, thisFolderURI))
					    returnOp->ClearOperation(nsIMsgOfflineImapOperation::kMoveResult);
			    }
		    }
      }
    }
	}
  NS_IF_ADDREF(returnOp);
  *originalOp = returnOp;
  return rv;
}

nsresult nsImapMailFolder::GetOriginalOp(nsIMsgOfflineImapOperation *op, nsIMsgOfflineImapOperation **originalOp, nsIMsgDatabase **originalDB)
{
	nsIMsgOfflineImapOperation *returnOp = nsnull;
	
	nsXPIDLCString sourceFolderURI;
	op->GetSourceFolderURI(getter_Copies(sourceFolderURI));
	
  nsCOMPtr<nsIRDFResource> res;
  nsresult rv;

  NS_WITH_SERVICE(nsIRDFService, rdf, kRDFServiceCID, &rv);
  if (NS_FAILED(rv)) 
    return rv; 
  rv = rdf->GetResource(sourceFolderURI, getter_AddRefs(res));
  if (NS_SUCCEEDED(rv))
  {
    nsCOMPtr<nsIMsgFolder> sourceFolder(do_QueryInterface(res, &rv));
    if (NS_SUCCEEDED(rv) && sourceFolder)
    {
	    if (sourceFolder)
	    {
        nsCOMPtr <nsIDBFolderInfo> folderInfo;
        sourceFolder->GetDBFolderInfoAndDB(getter_AddRefs(folderInfo), originalDB);
		    if (*originalDB)
		    {
          nsMsgKey originalKey;
          op->GetMessageKey(&originalKey);
          rv = (*originalDB)->GetOfflineOpForKey(originalKey, PR_FALSE, &returnOp);
		    }
      }
    }
	}
  NS_IF_ADDREF(returnOp);
  *originalOp = returnOp;
  return rv;
}

// this imap folder is the destination of an offline move/copy.
// We are either offline, or doing a pseudo-offline delete (where we do an offline
// delete, load the next message, then playback the offline delete). 
nsresult nsImapMailFolder::CopyMessagesOffline(nsIMsgFolder* srcFolder,
                               nsISupportsArray* messages,
                               PRBool isMove,
                               nsIMsgWindow *msgWindow,
                               nsIMsgCopyServiceListener* listener)
{
  NS_ENSURE_ARG(messages);
  nsresult rv = NS_OK;
	nsresult stopit = 0;
	nsCOMPtr <nsIMsgDatabase> sourceMailDB;
  nsCOMPtr <nsIDBFolderInfo> srcDbFolderInfo;
  srcFolder->GetDBFolderInfoAndDB(getter_AddRefs(srcDbFolderInfo), getter_AddRefs(sourceMailDB));
	PRBool deleteToTrash = PR_FALSE;
  PRUint32 srcCount;
  messages->Count(&srcCount);
  nsCOMPtr<nsIImapIncomingServer> imapServer;
  rv = GetImapIncomingServer(getter_AddRefs(imapServer));

  if (NS_SUCCEEDED(rv) && imapServer)
  {
    nsMsgImapDeleteModel deleteModel;
    imapServer->GetDeleteModel(&deleteModel);
    deleteToTrash = (deleteModel == nsMsgImapDeleteModels::MoveToTrash);
  }	
	if (sourceMailDB)
	{
#ifdef DOING_OFFLINEUNDO_YET
		UndoManager *undoManager = NULL;

		if (state && state->sourcePane)
			undoManager = state->sourcePane->GetUndoManager();

		PRBool shouldUndoOffline = undoManager && NET_IsOffline();
		if (shouldUndoOffline)
			undoManager->StartBatch();
#endif 
		// save the future ops in the source DB, if this is not a imap->local copy/move
		
    GetDatabase(nsnull);
		if (mDatabase) 
		{
			// get the highest key in the dest db, so we can make up our fake keys
			PRBool highWaterDeleted = PR_FALSE;
			nsMsgKey fakeBase = 1;
      nsCOMPtr <nsIDBFolderInfo> folderInfo;
      rv = mDatabase->GetDBFolderInfo(getter_AddRefs(folderInfo));
      NS_ENSURE_SUCCESS(rv, rv);
      nsMsgKey highWaterMark = nsMsgKey_None;
      folderInfo->GetHighWater(&highWaterMark);

			fakeBase += highWaterMark;
			
			// put fake message in destination db, delete source if move
			for (PRUint32 sourceKeyIndex=0; !stopit && (sourceKeyIndex < srcCount); sourceKeyIndex++)
			{
				PRBool	messageReturningHome = PR_FALSE;
        nsXPIDLCString sourceFolderURI;
        srcFolder->GetURI(getter_Copies(sourceFolderURI));
        nsXPIDLCString originalSrcFolderURI;
        originalSrcFolderURI = sourceFolderURI.get();
        nsCOMPtr<nsISupports> msgSupports;
        nsCOMPtr<nsIMsgDBHdr> message;

        msgSupports = getter_AddRefs(messages->ElementAt(sourceKeyIndex));
        message = do_QueryInterface(msgSupports);
        nsMsgKey originalKey;
        if (message)
        {
          rv = message->GetMessageKey(&originalKey);
        }
        else
        {
          NS_ASSERTION(PR_FALSE, "bad msg in src array");
          continue;
        }
				nsCOMPtr <nsIMsgOfflineImapOperation> sourceOp;
        rv = sourceMailDB->GetOfflineOpForKey(originalKey, PR_TRUE, getter_AddRefs(sourceOp));
				if (NS_SUCCEEDED(rv) && sourceOp)
				{
          srcFolder->SetFlag(MSG_FOLDER_FLAG_OFFLINEEVENTS);
					nsCOMPtr <nsIMsgDatabase> originalDB;
          nsOfflineImapOperationType opType;
          sourceOp->GetOperation(&opType);
          // if we already have an offline op for this key, then we need to see if it was
          // moved into the source folder while offline
          if (opType == nsIMsgOfflineImapOperation::kMoveResult) // offline move
					{
						// gracious me, we are moving something we already moved while offline!
						// find the original operation and clear it!
						nsCOMPtr <nsIMsgOfflineImapOperation> originalOp;
            rv = GetClearedOriginalOp(sourceOp, getter_AddRefs(originalOp), getter_AddRefs(originalDB));
						if (originalOp)
						{
							nsXPIDLCString originalString;
              nsXPIDLCString srcFolderURI;

              srcFolder->GetURI(getter_Copies(srcFolderURI));
							sourceOp->GetSourceFolderURI(getter_Copies(originalString));
              sourceOp->GetMessageKey(&originalKey);
              originalSrcFolderURI = originalString.get();
							
              if (isMove)
	              sourceMailDB->RemoveOfflineOp(sourceOp);
							
							sourceOp = originalOp;
              if (!nsCRT::strcmp(originalSrcFolderURI, srcFolderURI))
							{
								messageReturningHome = PR_TRUE;
                originalDB->RemoveOfflineOp(originalOp);
							}
						}
					}
					
					if (!messageReturningHome)
					{
            nsXPIDLCString folderURI;
						GetURI(getter_Copies(folderURI));
							
						if (isMove)
						{
							sourceOp->SetDestinationFolderURI(folderURI); // offline move
              sourceOp->SetOperation(nsIMsgOfflineImapOperation::kMsgMoved);
						}
						else
							sourceOp->AddMessageCopyOperation(folderURI); // offline copy
#ifdef DOING_OFFLINEUNDO_YET
						if (shouldUndoOffline && undoManager->GetState() == UndoIdle)
						{	// only need undo if we're really offline and not pseudo offline
							OfflineIMAPUndoAction *undoAction = new 
									OfflineIMAPUndoAction(state->sourcePane, (MSG_FolderInfo*) this, sourceOp->GetMessageKey(), opType,
									sourceMailDB->GetFolderInfo(), dstFolder, 0, NULL, 0);
							if (undoAction)
								undoManager->AddUndoAction(undoAction);
						}
#endif
					}
				}
					else
						stopit = NS_ERROR_FAILURE;
				

				nsCOMPtr <nsIMsgDBHdr> mailHdr;
        rv = sourceMailDB->GetMsgHdrForKey(originalKey, getter_AddRefs(mailHdr));
				if (NS_SUCCEEDED(rv) && mailHdr)
				{
					PRBool successfulCopy = PR_FALSE;
          nsMsgKey srcDBhighWaterMark;
          srcDbFolderInfo->GetHighWater(&srcDBhighWaterMark);
					highWaterDeleted = !highWaterDeleted && isMove && deleteToTrash &&
										(originalKey == srcDBhighWaterMark);
					
					nsCOMPtr <nsIMsgDBHdr> newMailHdr;
  				rv = mDatabase->CopyHdrFromExistingHdr(fakeBase + sourceKeyIndex, mailHdr,
              PR_TRUE, getter_AddRefs(newMailHdr));
          if (!newMailHdr || !NS_SUCCEEDED(rv))
          {
            NS_ASSERTION(PR_FALSE, "failed to copy hdr");
            stopit = rv;
          }
					
					if (NS_SUCCEEDED(stopit))
					{
    				nsCOMPtr <nsIMsgOfflineImapOperation> destOp;
						mDatabase->GetOfflineOpForKey(fakeBase + sourceKeyIndex, PR_TRUE, getter_AddRefs(destOp));
						if (destOp)
						{
							// check if this is a move back to the original mailbox, in which case
							// we just delete the offline operation.
							if (messageReturningHome)
							{
								mDatabase->RemoveOfflineOp(destOp);
							}
							else
							{
                SetFlag(MSG_FOLDER_FLAG_OFFLINEEVENTS);
								destOp->SetSourceFolderURI(originalSrcFolderURI);
                destOp->SetMessageKey(originalKey);
#ifdef DOING_OFFLINEUNDO_YET
								if (shouldUndoOffline && undoManager->GetState() == UndoIdle)
								{
									OfflineIMAPUndoAction *undoAction = new 
										OfflineIMAPUndoAction(state->sourcePane, (MSG_FolderInfo*) this, destOp->GetMessageKey(), kAddedHeader,
											state->destDB->GetFolderInfo(), dstFolder, 0, newMailHdr);
									if (undoAction)
										undoManager->AddUndoAction(undoAction);
								}
#endif
							}
						}
						else
							stopit = NS_ERROR_FAILURE;
					}
					successfulCopy = NS_SUCCEEDED(stopit);
					
					
					if (isMove && successfulCopy)	
					{
						nsOfflineImapOperationType opType = nsIMsgOfflineImapOperation::kDeletedMsg;
						if (!deleteToTrash)
							opType = nsIMsgOfflineImapOperation::kMsgMarkedDeleted;
#ifdef DOING_OFFLINEUNDO_YET
						if (shouldUndoOffline && undoManager->GetState() == UndoIdle) 
            {
							OfflineIMAPUndoAction *undoAction = new 
								OfflineIMAPUndoAction(state->sourcePane, (MSG_FolderInfo*) this, mailHdr->GetMessageKey(), opType,
										sourceMailDB->GetFolderInfo(), dstFolder, 0, mailHdr);
							if (undoAction)
								undoManager->AddUndoAction(undoAction);
						}
#endif
            nsMsgKey msgKey;
            mailHdr->GetMessageKey(&msgKey);
						if (deleteToTrash)
							sourceMailDB->DeleteMessage(msgKey, nsnull, PR_FALSE);
						else
							sourceMailDB->MarkImapDeleted(msgKey,PR_TRUE,nsnull); // offline delete
					}
						
					
					if (!successfulCopy)
						highWaterDeleted = PR_FALSE;
				}
			}
			
      PRInt32 numVisibleMessages;
      srcDbFolderInfo->GetNumVisibleMessages(&numVisibleMessages);
      if (numVisibleMessages && highWaterDeleted)
      {
//                ListContext *listContext = nsnull;
//                nsCOMPtr <nsIMsgDBHdr> highHdr;
//                if (sourceMailDB->ListLast(&listContext, &highHdr) == NS_OK)
//                {   
//                    sourceMailDB->m_neoFolderInfo->m_LastMessageUID = highHdr->GetMessageKey();
//                  sourceMailDB->ListDone(listContext);
//                }
            }
            
			if (isMove)
				sourceMailDB->Commit(nsMsgDBCommitType::kLargeCommit);
      mDatabase->Commit(nsMsgDBCommitType::kLargeCommit);
      SummaryChanged();
      srcFolder->SummaryChanged();
		}

#ifdef DOING_OFFLINEUNDO_YET
    if (shouldUndoOffline)
      undoManager->EndBatch();
#endif
  }
  if (NS_SUCCEEDED(rv) && isMove)
    srcFolder->NotifyFolderEvent(mDeleteOrMoveMsgCompletedAtom);
  return rv;
}

NS_IMETHODIMP
nsImapMailFolder::CopyMessages(nsIMsgFolder* srcFolder,
                               nsISupportsArray* messages,
                               PRBool isMove,
                               nsIMsgWindow *msgWindow,
                               nsIMsgCopyServiceListener* listener,
							   PRBool isFolder)   //isFolder for future use when we do cross-server folder move/copy
{
  nsresult rv = NS_OK;
  nsCAutoString messageIds;
  nsMsgKeyArray srcKeyArray;
  nsCOMPtr<nsIUrlListener> urlListener;
  nsCOMPtr<nsISupports> srcSupport;
  nsCOMPtr<nsISupports> copySupport;

  mCopyListener = listener;

  if (msgWindow)
  {
    nsCOMPtr <nsITransactionManager> txnMgr;

    msgWindow->GetTransactionManager(getter_AddRefs(txnMgr));
    if (txnMgr) SetTransactionManager(txnMgr);
  }

  if (WeAreOffline())
    return CopyMessagesOffline(srcFolder, messages, isMove, msgWindow, listener);

  NS_WITH_SERVICE(nsIImapService, imapService, kCImapService, &rv);

  if (!srcFolder || !messages) return NS_ERROR_NULL_POINTER;
  nsCOMPtr <nsIMsgIncomingServer> srcServer;
  nsCOMPtr <nsIMsgIncomingServer> dstServer;

  rv = srcFolder->GetServer(getter_AddRefs(srcServer));
  if(NS_FAILED(rv)) goto done;

  rv = GetServer(getter_AddRefs(dstServer));
  if(NS_FAILED(rv)) goto done;

  NS_ENSURE_TRUE(dstServer, NS_ERROR_NULL_POINTER);
  PRBool sameServer;
  rv = dstServer->Equals(srcServer, &sameServer);
  if (NS_FAILED(rv)) goto done;

  // if the folders aren't on the same server, do a stream base copy
  if (!sameServer) 
  {
    rv = CopyMessagesWithStream(srcFolder, messages, isMove, PR_TRUE, msgWindow, listener);
    goto done;
  }

  rv = BuildIdsAndKeyArray(messages, messageIds, srcKeyArray);
  if(NS_FAILED(rv)) goto done;

  srcSupport = do_QueryInterface(srcFolder);
  rv = QueryInterface(NS_GET_IID(nsIUrlListener), getter_AddRefs(urlListener));

  rv = InitCopyState(srcSupport, messages, isMove, PR_TRUE, listener, msgWindow);
  if (NS_FAILED(rv)) goto done;

  m_copyState->m_curIndex = m_copyState->m_totalCount;

  copySupport = do_QueryInterface(m_copyState);
  if (imapService)
    rv = imapService->OnlineMessageCopy(m_eventQueue,
                                            srcFolder, messageIds.get(),
                                            this, PR_TRUE, isMove,
                                            urlListener, nsnull,
                                            copySupport, msgWindow);
  if (NS_SUCCEEDED(rv))
  {
    nsImapMoveCopyMsgTxn* undoMsgTxn = new nsImapMoveCopyMsgTxn(
        srcFolder, &srcKeyArray, messageIds.get(), this,
        PR_TRUE, isMove, m_eventQueue, urlListener);
    if (!undoMsgTxn) return NS_ERROR_OUT_OF_MEMORY;
    if (isMove)
    {
      if (mFlags & MSG_FOLDER_FLAG_TRASH)
          undoMsgTxn->SetTransactionType(nsIMessenger::eDeleteMsg);
      else
          undoMsgTxn->SetTransactionType(nsIMessenger::eMoveMsg);
    }
    else
    {
        undoMsgTxn->SetTransactionType(nsIMessenger::eCopyMsg);
    }
      rv = undoMsgTxn->QueryInterface(
        NS_GET_IID(nsImapMoveCopyMsgTxn), 
        getter_AddRefs(m_copyState->m_undoMsgTxn) );
  }
  else 
  {
    NS_ASSERTION(PR_FALSE, "online copy failed");
    ClearCopyState(rv);
  }

done:
    return rv;
}

NS_IMETHODIMP
nsImapMailFolder::CopyFolder(nsIMsgFolder* srcFolder,
                               PRBool isMoveFolder,
                               nsIMsgWindow *msgWindow,
                               nsIMsgCopyServiceListener* listener)
{

  NS_ENSURE_ARG_POINTER(srcFolder);
  
  nsresult rv = NS_OK;

  if (isMoveFolder)   //move folder permitted when dstFolder and the srcFolder are on same server
  {
	   nsCOMPtr <nsIImapService> imapService = do_GetService (kCImapService, &rv);
	   if (NS_SUCCEEDED(rv))
	   {
	       nsCOMPtr <nsIUrlListener> urlListener = do_QueryInterface(srcFolder);
	       rv = imapService->MoveFolder(m_eventQueue,
                                        srcFolder,
                                        this,
                                        urlListener,
                                        nsnull);
	   }

  }
  else
	  NS_ASSERTION(0,"isMoveFolder is false. Trying to copy to a different server.");

  return rv;
  
}

nsresult
nsImapMailFolder::SetTransactionManager(nsITransactionManager* txnMgr)
{
    nsresult rv = NS_OK;
    if (txnMgr && !m_transactionManager)
        m_transactionManager = txnMgr;
    return rv;
}

NS_IMETHODIMP
nsImapMailFolder::CopyFileMessage(nsIFileSpec* fileSpec,
                                  nsIMsgDBHdr* msgToReplace,
                                  PRBool isDraftOrTemplate,
                                  nsIMsgWindow *msgWindow,
                                  nsIMsgCopyServiceListener* listener)
{
    nsresult rv = NS_ERROR_NULL_POINTER;
    nsMsgKey key = 0xffffffff;
    nsCAutoString messageId;
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

    rv = QueryInterface(NS_GET_IID(nsIUrlListener), getter_AddRefs(urlListener));

    if (msgToReplace)
    {
        rv = msgToReplace->GetMessageKey(&key);
        if (NS_SUCCEEDED(rv))
            messageId.AppendInt((PRInt32) key);
    }

    rv = InitCopyState(srcSupport, messages, PR_FALSE, isDraftOrTemplate,
                       listener, msgWindow);
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsISupports> copySupport;
    if( m_copyState ) 
        copySupport = do_QueryInterface(m_copyState);
    rv = imapService->AppendMessageFromFile(m_eventQueue, fileSpec, this,
                                            messageId.get(),
                                            PR_TRUE, isDraftOrTemplate,
                                            urlListener, nsnull,
                                            copySupport);

    return rv;
}

nsresult 
nsImapMailFolder::CopyStreamMessage(nsIMsgDBHdr* message,
                                    nsIMsgFolder* dstFolder, // should be this
                                    nsIMsgWindow *aMsgWindow,
                                    PRBool isMove)
{
    nsresult rv = NS_ERROR_NULL_POINTER;
    if (!m_copyState) return rv;

    nsCOMPtr<nsICopyMessageStreamListener> copyStreamListener;

    rv = nsComponentManager::CreateInstance(kCopyMessageStreamListenerCID,
               NULL, NS_GET_IID(nsICopyMessageStreamListener),
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
       
    nsCOMPtr<nsIMsgDBHdr> msgHdr(do_QueryInterface(message));
    if (!msgHdr) return NS_ERROR_FAILURE;

    nsXPIDLCString uri;
    srcFolder->GetUriForMsg(msgHdr, getter_Copies(uri));

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
                                                     isMove && !m_copyState->m_isCrossServerOp, nsnull, aMsgWindow, &url);
  }
    return rv;
}

nsImapMailCopyState::nsImapMailCopyState() : m_msgService(nsnull),
    m_isMove(PR_FALSE), m_selectedState(PR_FALSE), m_curIndex(0),
    m_totalCount(0), m_streamCopy(PR_FALSE), m_dataBuffer(nsnull),
    m_leftOver(0), m_isCrossServerOp(PR_FALSE)
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
            nsXPIDLCString uri;
            msgNode->GetValue(getter_Copies(uri));
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


NS_IMPL_THREADSAFE_ISUPPORTS1(nsImapMailCopyState, nsImapMailCopyState)

nsresult
nsImapMailFolder::InitCopyState(nsISupports* srcSupport,
                                nsISupportsArray* messages,
                                PRBool isMove,
                                PRBool selectedState,
                                nsIMsgCopyServiceListener* listener,
                                nsIMsgWindow *msgWindow)
{
    nsresult rv = NS_OK;

    if (!srcSupport || !messages) return NS_ERROR_NULL_POINTER;
    NS_ASSERTION(!m_copyState, "move/copy already in progress");
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
    m_copyState->m_msgWindow = msgWindow;
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
  *matches = mName.EqualsWithConversion(*name, isInbox);
  return NS_OK;
}

nsresult nsImapMailFolder::CreateBaseMessageURI(const char *aURI)
{
  nsresult rv;

  rv = nsCreateImapBaseMessageURI(aURI, &mBaseMessageURI);
  return rv;
}

NS_IMETHODIMP nsImapMailFolder::GetFolderNeedsSubscribing(PRBool *bVal)
{
    if (!bVal)
        return NS_ERROR_NULL_POINTER;
    *bVal = m_folderNeedsSubscribing;
    return NS_OK;
}

NS_IMETHODIMP nsImapMailFolder::SetFolderNeedsSubscribing(PRBool bVal)
{
    m_folderNeedsSubscribing = bVal;
    return NS_OK;
}

NS_IMETHODIMP nsImapMailFolder::GetFolderNeedsACLListed(PRBool *bVal)
{
    if (!bVal)
        return NS_ERROR_NULL_POINTER;
    // *** jt -- come back later; still need to worry about if the folder
    // itself is a namespace
    *bVal = (m_folderNeedsACLListed && !(mFlags &
                                         MSG_FOLDER_FLAG_IMAP_NOSELECT) 
             /* && !GetFolderIsNamespace() */ );
    return NS_OK;
}

NS_IMETHODIMP nsImapMailFolder::SetFolderNeedsACLListed(PRBool bVal)
{
    m_folderNeedsACLListed = bVal;
    return NS_OK;
}

NS_IMETHODIMP nsImapMailFolder::GetFolderNeedsAdded(PRBool *bVal)
{
    if (!bVal)
        return NS_ERROR_NULL_POINTER;
    *bVal = m_folderNeedsAdded;
    return NS_OK;
}

NS_IMETHODIMP nsImapMailFolder::SetFolderNeedsAdded(PRBool bVal)
{
    m_folderNeedsAdded = bVal;
    return NS_OK;
}

NS_IMETHODIMP nsImapMailFolder::GetFolderVerifiedOnline(PRBool *bVal)
{
    if (!bVal)
        return NS_ERROR_NULL_POINTER;
    *bVal = m_verifiedAsOnlineFolder;
    return NS_OK;
}

NS_IMETHODIMP nsImapMailFolder::SetFolderVerifiedOnline(PRBool bVal)
{
    m_verifiedAsOnlineFolder = bVal;
    return NS_OK;
}

NS_IMETHODIMP nsImapMailFolder::ShouldStoreMsgOffline(nsMsgKey msgKey, PRBool *result)
{
  // check if we're storing mail we read in the inbox in a personal filing cabinet.
  // if not, just use base class implementation.
  if (mFlags & MSG_FOLDER_FLAG_INBOX)
  {
    PRBool hasMsgOffline = PR_FALSE;

    HasMsgOffline(msgKey, &hasMsgOffline);
    if (hasMsgOffline)
    {
      *result = PR_FALSE;
      return NS_OK;
    }
    nsCOMPtr<nsIImapIncomingServer> imapServer;
    nsresult rv = GetImapIncomingServer(getter_AddRefs(imapServer));
    if (NS_SUCCEEDED(rv) && imapServer)
    {
      PRBool storeReadMailInPFC;
      imapServer->GetStoreReadMailInPFC(&storeReadMailInPFC);
      if (storeReadMailInPFC)
      {
        *result = PR_TRUE;
        return NS_OK;
      }
    }
  }
  return nsMsgDBFolder::ShouldStoreMsgOffline(msgKey, result);
}

nsresult nsImapMailFolder::GetOfflineStoreOutputStream(nsIOutputStream **outputStream)
{
  // check if we're storing mail we read in the inbox in a personal filing cabinet.
  // if not, just use base class implementation.
  if (mFlags & MSG_FOLDER_FLAG_INBOX)
  {
    nsCOMPtr<nsIImapIncomingServer> imapServer;
    nsresult rv = GetImapIncomingServer(getter_AddRefs(imapServer));
    if (NS_SUCCEEDED(rv) && imapServer)
    {
      PRBool storeReadMailInPFC;
      imapServer->GetStoreReadMailInPFC(&storeReadMailInPFC);
      if (storeReadMailInPFC)
      {
        nsresult rv = NS_ERROR_NULL_POINTER;
        nsCOMPtr <nsIMsgFolder> outputPFC;

        imapServer->GetReadMailPFC(PR_TRUE, getter_AddRefs(outputPFC));
        if (outputPFC)
        {
          nsCOMPtr <nsIFileSpec> outputPFCPath;
          outputPFC->GetPath(getter_AddRefs(outputPFCPath));
          nsCOMPtr<nsISupports>  supports;
          nsFileSpec fileSpec;
          outputPFCPath->GetFileSpec(&fileSpec);
          rv = NS_NewIOFileStream(getter_AddRefs(supports), fileSpec, PR_WRONLY | PR_CREATE_FILE, 00700);
          supports->QueryInterface(NS_GET_IID(nsIOutputStream), (void **) outputStream);

          nsCOMPtr <nsIRandomAccessStore> randomStore = do_QueryInterface(supports);
          if (randomStore)
            randomStore->Seek(PR_SEEK_END, 0);
        }
        return rv;
      }
    }
  }
  return nsMsgDBFolder::GetOfflineStoreOutputStream(outputStream);
}

NS_IMETHODIMP nsImapMailFolder::PerformExpand(nsIMsgWindow *aMsgWindow)
{
    nsresult rv;
    PRBool usingSubscription = PR_FALSE;
    nsCOMPtr<nsIImapIncomingServer> imapServer;
    rv = GetImapIncomingServer(getter_AddRefs(imapServer));

    if (NS_FAILED(rv) || !imapServer) return NS_ERROR_FAILURE;
    rv = imapServer->GetUsingSubscription(&usingSubscription);
    if (NS_SUCCEEDED(rv) && !usingSubscription)
    {
        NS_WITH_SERVICE(nsIImapService, imapService, kCImapService, &rv);
        if (NS_SUCCEEDED(rv))
            rv = imapService->DiscoverChildren(m_eventQueue, this, this,
                                               m_onlineFolderName,
                                               nsnull);
    }
    return rv;
}

NS_IMETHODIMP nsImapMailFolder::RenameClient( nsIMsgFolder *msgFolder, const char* oldName, const char* newName )
{
    nsresult rv = NS_OK;
    nsCOMPtr<nsIFileSpec> pathSpec;
    rv = GetPath(getter_AddRefs(pathSpec));
    if (NS_FAILED(rv)) return rv;

    nsFileSpec path;
    rv = pathSpec->GetFileSpec(&path);
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIMsgImapMailFolder> oldImapFolder = do_QueryInterface(msgFolder, &rv);
    if (NS_FAILED(rv)) return rv;

    PRUnichar hierarchyDelimiter = '/';
    oldImapFolder->GetHierarchyDelimiter(&hierarchyDelimiter);
    PRInt32 boxflags=0;
    oldImapFolder->GetBoxFlags(&boxflags);

    nsAutoString newLeafName;
	nsAutoString newNameString;
    newNameString.AssignWithConversion(newName);
	newLeafName = newNameString;
	nsAutoString parentName; 
    nsAutoString folderNameStr;
    PRInt32 folderStart = newLeafName.RFindChar('/');  //internal use of hierarchyDelimiter is always '/'
    if (folderStart > 0)
    {
        newNameString.Right(newLeafName, newLeafName.Length() - folderStart - 1);
		CreateDirectoryForFolder(path);    //needed when we move a folder to a folder with no subfolders.
	}	

    // if we get here, it's really a leaf, and "this" is the parent.
    folderNameStr = newLeafName;
    
    // Create an empty database for this mail folder, set its name from the user  
    nsCOMPtr<nsIMsgDatabase> mailDBFactory;
    nsCOMPtr<nsIMsgFolder> child;
    nsCOMPtr <nsIMsgImapMailFolder> imapFolder;

    rv = nsComponentManager::CreateInstance(kCMailDB, nsnull, NS_GET_IID(nsIMsgDatabase), (void **) getter_AddRefs(mailDBFactory));
    if (NS_SUCCEEDED(rv) && mailDBFactory)
	{
      nsCOMPtr<nsIMsgDatabase> unusedDB;
      nsCOMPtr <nsIFileSpec> dbFileSpec;

      nsXPIDLCString uniqueLeafName;
      nsCAutoString proposedDBName(newLeafName.ToNewCString());
	  //nsCAutoString proposedDBName(folderName);
      proposedDBName += ".msf";

      rv = CreatePlatformLeafNameForDisk(proposedDBName, path, getter_Copies(uniqueLeafName));

      // take off the ".msf" on the end.
      proposedDBName = uniqueLeafName;
      proposedDBName.Truncate(proposedDBName.Length() - 4);

      path.SetLeafName(proposedDBName);

      NS_NewFileSpecWithSpec(path, getter_AddRefs(dbFileSpec));
      // it's OK to use Open and not OpenFolderDB here, since we don't use the DB.
      rv = mailDBFactory->Open(dbFileSpec, PR_TRUE, PR_TRUE, (nsIMsgDatabase **) getter_AddRefs(unusedDB));

      if (NS_SUCCEEDED(rv) && unusedDB)
      {
        //need to set the folder name
        nsCOMPtr <nsIDBFolderInfo> folderInfo;
        rv = unusedDB->GetDBFolderInfo(getter_AddRefs(folderInfo));

        //Now let's create the actual new folder
        rv = AddSubfolderWithPath(&folderNameStr, dbFileSpec, getter_AddRefs(child));

        imapFolder = do_QueryInterface(child);
        if (imapFolder)
		{
          nsCAutoString onlineName(m_onlineFolderName); 
          if (onlineName.Length() > 0)
          onlineName.AppendWithConversion(hierarchyDelimiter);
          onlineName.AppendWithConversion(folderNameStr);
          imapFolder->SetVerifiedAsOnlineFolder(PR_TRUE);
          imapFolder->SetOnlineName(onlineName.get());
          imapFolder->SetHierarchyDelimiter(hierarchyDelimiter);
          imapFolder->SetBoxFlags(boxflags);

        // store the online name as the mailbox name in the db folder info
        // I don't think anyone uses the mailbox name, so we'll use it
        // to restore the online name when blowing away an imap db.
           if (folderInfo)
           {
             nsAutoString unicodeOnlineName; unicodeOnlineName.AssignWithConversion(onlineName);
             folderInfo->SetMailboxName(&unicodeOnlineName);
           }
        }

        unusedDB->SetSummaryValid(PR_TRUE);
        unusedDB->Commit(nsMsgDBCommitType::kLargeCommit);
        unusedDB->Close(PR_TRUE);

	    imapFolder->RenameSubfolders(msgFolder);
	  }
    }

	nsCOMPtr<nsIFolder> parent;
	msgFolder->GetParent(getter_AddRefs(parent));
	nsCOMPtr<nsIMsgFolder> msgParent = do_QueryInterface(parent);
    msgFolder->SetParent(nsnull);
	msgParent->PropagateDelete(msgFolder,PR_FALSE);

    if(NS_SUCCEEDED(rv) && child)
    {
       nsCOMPtr<nsISupports> childSupports(do_QueryInterface(child));
       nsCOMPtr<nsISupports> folderSupports;
       rv = QueryInterface(NS_GET_IID(nsISupports), getter_AddRefs(folderSupports));
       if(childSupports && NS_SUCCEEDED(rv))
	   {
          NotifyItemAdded(folderSupports, childSupports, "folderView");
	   }
	}
       
  return rv;

}

NS_IMETHODIMP nsImapMailFolder::RenameSubfolders(nsIMsgFolder *oldFolder)
{
  nsresult rv = NS_OK;
  
  nsCOMPtr<nsIEnumerator> aEnumerator;
  oldFolder->GetSubFolders(getter_AddRefs(aEnumerator));
  nsCOMPtr<nsISupports> aSupport;
  rv = aEnumerator->First();
  while (NS_SUCCEEDED(rv))
  {
     rv = aEnumerator->CurrentItem(getter_AddRefs(aSupport));
			
     nsCOMPtr<nsIMsgFolder>msgFolder = do_QueryInterface(aSupport);
     nsCOMPtr<nsIMsgImapMailFolder> folder = do_QueryInterface(msgFolder, &rv);
     if (NS_FAILED(rv)) return rv;

     PRUnichar hierarchyDelimiter = '/';
     folder->GetHierarchyDelimiter(&hierarchyDelimiter);
				
     PRInt32 boxflags;
     folder->GetBoxFlags(&boxflags);

     PRBool verified;
     folder->GetVerifiedAsOnlineFolder(&verified);
				
     nsCOMPtr<nsIFileSpec> oldPathSpec;
     rv = msgFolder->GetPath(getter_AddRefs(oldPathSpec));
     if (NS_FAILED(rv)) return rv;

     nsFileSpec oldPath;
     rv = oldPathSpec->GetFileSpec(&oldPath);
     if (NS_FAILED(rv)) return rv;

     nsCOMPtr<nsIFileSpec> newParentPathSpec;
     rv = GetPath(getter_AddRefs(newParentPathSpec));
     if (NS_FAILED(rv)) return rv;

     nsFileSpec newParentPath;
     rv = newParentPathSpec->GetFileSpec(&newParentPath);
     if (NS_FAILED(rv)) return rv;

     rv = AddDirectorySeparator(newParentPath);
     newParentPath += oldPath.GetLeafName();
     nsCString newPathStr(newParentPath.GetNativePathCString());
     nsCOMPtr<nsIFileSpec> newPathSpec;
     rv = NS_NewFileSpec(getter_AddRefs(newPathSpec));
     if (NS_FAILED(rv)) return rv;
     rv = newPathSpec->SetNativePath(newPathStr.get());
	 	  
     nsFileSpec newPath;
     rv = newPathSpec->GetFileSpec(&newPath);
     if (NS_FAILED(rv)) return rv;

     nsCOMPtr<nsIFileSpec> dbFileSpec;
     NS_NewFileSpecWithSpec(newPath, getter_AddRefs(dbFileSpec));
     nsCOMPtr<nsIMsgFolder> child;
				
     char *leafName;
     leafName = newPath.GetLeafName();
     nsAutoString currentFolderNameStr;
     currentFolderNameStr.AssignWithConversion(leafName);
     nsAutoString utf7LeafName = currentFolderNameStr;

     AddSubfolderWithPath(&utf7LeafName, dbFileSpec, getter_AddRefs(child));
	 
     nsCOMPtr <nsIMsgImapMailFolder> imapFolder = do_QueryInterface(child);
     nsXPIDLCString onlineName;
     GetOnlineName(getter_Copies(onlineName));
     nsCAutoString onlineCName(onlineName);
     onlineCName.AppendWithConversion(hierarchyDelimiter);
     onlineCName.Append(leafName);
     imapFolder->SetVerifiedAsOnlineFolder(verified);
     imapFolder->SetOnlineName(onlineCName.get());
     imapFolder->SetHierarchyDelimiter(hierarchyDelimiter);
     imapFolder->SetBoxFlags(boxflags);

     rv = aEnumerator->Next();
	
     imapFolder->RenameSubfolders(msgFolder);
     m_initialized = PR_TRUE; 
     PL_strfree(leafName);

  }
  return rv;
}

