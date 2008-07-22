/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
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
 *   Pierre Phaneuf <pp@ludusdesign.com>
 *   Seth Spitzer <sspitzer@netscape.com>
 *   Lorenzo Colitti <lorenzo@colitti.com>
 *   Karsten Düsterloh <mnyromyr@tprac.de>
 *   Siddharth Agarwal <sid1337@gmail.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either of the GNU General Public License Version 2 or later (the "GPL"),
 * or the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "msgCore.h"
#include "prmem.h"
#include "nsMsgImapCID.h"
#include "nsImapMailFolder.h"
#include "nsIEnumerator.h"
#include "nsILocalFile.h"
#include "nsIFolderListener.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsIRDFService.h"
#include "nsRDFCID.h"
#include "nsMsgDBCID.h"
#include "nsMsgFolderFlags.h"
#include "nsImapFlagAndUidState.h"
#include "nsISeekableStream.h"
#include "nsThreadUtils.h"
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
#include "nsIPrefBranch.h"
#include "nsIPrefService.h"
#include "nsMsgI18N.h"
#include "nsICacheSession.h"
#include "nsEscape.h"
#include "nsIDOMWindowInternal.h"
#include "nsIMsgFilter.h"
#include "nsImapMoveCoalescer.h"
#include "nsIPrompt.h"
#include "nsIPromptService.h"
#include "nsIDocShell.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsReadableUtils.h"
#include "nsUnicharUtils.h"
#include "nsIImapFlagAndUidState.h"
#include "nsIImapHeaderXferInfo.h"
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
#include "nsIWebNavigation.h"
#include "nsNetUtil.h"
#include "nsIMAPNamespace.h"
#include "nsHashtable.h"
#include "nsIMsgFolderCompactor.h"
#include "nsMsgMessageFlags.h"
#include "nsIMimeHeaders.h"
#include "nsIMsgMdnGenerator.h"
#include "nsISpamSettings.h"
#include "nsInt64.h"
#include <time.h>
#include "nsIMsgMailNewsUrl.h"
#include "nsEmbedCID.h"
#include "nsIMsgComposeService.h"
#include "nsMsgCompCID.h"
#include "nsICacheEntryDescriptor.h"
#include "nsDirectoryServiceDefs.h"
#include "nsIMsgIdentity.h"
#include "nsIMsgFolderNotificationService.h"
#include "nsNativeCharsetUtils.h"
#include "nsIExternalProtocolService.h"
#include "nsCExternalHandlerService.h"
#include "prprf.h"
#include "nsIMutableArray.h"
#include "nsArrayUtils.h"
#include "nsArrayEnumerator.h"

static NS_DEFINE_CID(kRDFServiceCID, NS_RDFSERVICE_CID);
static NS_DEFINE_CID(kParseMailMsgStateCID, NS_PARSEMAILMSGSTATE_CID);
static NS_DEFINE_CID(kCImapHostSessionList, NS_IIMAPHOSTSESSIONLIST_CID);

nsIAtom* nsImapMailFolder::mImapHdrDownloadedAtom=nsnull;

#define FOUR_K 4096
#define MAILNEWS_CUSTOM_HEADERS "mailnews.customHeaders"

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
            nsAutoString leafName;
            dirEntry->GetLeafName(leafName);
            newChild->AppendRelativePath(leafName);
            rv = newChild->Exists(&exists);
            if (NS_SUCCEEDED(rv) && !exists)
              rv = newChild->Create(nsIFile::DIRECTORY_TYPE, 0775);
            rv = RecursiveCopy(dirEntry, newChild);
          }
        }
        else
          rv = dirEntry->CopyTo(destDir, EmptyString());
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
    m_folderIsNamespace(PR_FALSE),
    m_folderNeedsSubscribing(PR_FALSE),
    m_folderNeedsAdded(PR_FALSE),
    m_folderNeedsACLListed(PR_TRUE),
    m_performingBiff(PR_FALSE),
    m_folderQuotaCommandIssued(PR_FALSE),
    m_folderQuotaDataIsValid(PR_FALSE),
    m_updatingFolder(PR_FALSE),
    m_downloadMessageForOfflineUse(PR_FALSE),
    m_downloadingFolderForOfflineUse(PR_FALSE),
    m_folderQuotaUsedKB(0),
    m_folderQuotaMaxKB(0)
{
  MOZ_COUNT_CTOR(nsImapMailFolder); // double count these for now.

  if (mImapHdrDownloadedAtom == nsnull)
    mImapHdrDownloadedAtom = NS_NewAtom("ImapHdrDownloaded");
  m_appendMsgMonitor = nsnull;  // since we're not using this (yet?) make it null.
  // if we do start using it, it should be created lazily

  m_thread = do_GetCurrentThread();
  m_moveCoalescer = nsnull;
  m_boxFlags = 0;
  m_uidValidity = kUidUnknown;
  m_numStatusRecentMessages = 0;
  m_numStatusUnseenMessages = 0;
  m_hierarchyDelimiter = kOnlineHierarchySeparatorUnknown;
  m_folderACL = nsnull;
  m_aclFlags = 0;
  m_supportedUserFlags = 0;
  m_namespace = nsnull;
  m_numFilterClassifyRequests = 0;
  m_pendingPlaybackReq = nsnull;  
}

nsImapMailFolder::~nsImapMailFolder()
{
  MOZ_COUNT_DTOR(nsImapMailFolder);
  if (m_appendMsgMonitor)
      PR_DestroyMonitor(m_appendMsgMonitor);

  // I think our destructor gets called before the base class...
  if (mInstanceCount == 1)
    NS_IF_RELEASE(mImapHdrDownloadedAtom);
  NS_IF_RELEASE(m_moveCoalescer);
  delete m_folderACL;
    
  // cleanup any pending request
  delete m_pendingPlaybackReq;
}

NS_IMPL_ADDREF_INHERITED(nsImapMailFolder, nsMsgDBFolder)
NS_IMPL_RELEASE_INHERITED(nsImapMailFolder, nsMsgDBFolder)
NS_IMPL_QUERY_HEAD(nsImapMailFolder)
    NS_IMPL_QUERY_BODY(nsIMsgImapMailFolder)
    NS_IMPL_QUERY_BODY(nsICopyMessageListener)
    NS_IMPL_QUERY_BODY(nsIImapMailFolderSink)
    NS_IMPL_QUERY_BODY(nsIImapMessageSink)
    NS_IMPL_QUERY_BODY(nsIUrlListener)
    NS_IMPL_QUERY_BODY(nsIMsgFilterHitNotify)
    NS_IMPL_QUERY_BODY(nsIJunkMailClassificationListener)
NS_IMPL_QUERY_TAIL_INHERITING(nsMsgDBFolder)

nsresult nsImapMailFolder::AddDirectorySeparator(nsILocalFile *path)
{
  if (mURI.Equals(kImapRootURI))
  {
    // don't concat the full separator with .sbd
  }
  else
  {
    // see if there's a dir with the same name ending with .sbd
    // unfortunately we can't just say:
    //          path += sep;
    // here because of the way nsFileSpec concatenates
    nsAutoString leafName;
    path->GetLeafName(leafName);
    leafName.Append(NS_LITERAL_STRING(FOLDER_SUFFIX));
    path->SetLeafName(leafName);
  }

  return NS_OK;
}

static PRBool
nsShouldIgnoreFile(nsString& name)
{
  PRInt32 len = name.Length();
  if (len > 4 && name.RFind(SUMMARY_SUFFIX, PR_TRUE) == len - 4)
  {
    name.SetLength(len-4); // truncate the string
    return PR_FALSE;
  }
  return PR_TRUE;
}

// this is only called for virtual folders, currently.
NS_IMETHODIMP nsImapMailFolder::AddSubfolder(const nsAString& aName, nsIMsgFolder** aChild)
{
  NS_ENSURE_ARG_POINTER(aChild);

  PRInt32 flags = 0;
  nsresult rv;
  nsCOMPtr<nsIRDFService> rdf = do_GetService("@mozilla.org/rdf/rdf-service;1", &rv);
  NS_ENSURE_SUCCESS(rv,rv);

  nsCAutoString uri(mURI);
  uri.Append('/');

  // If AddSubFolder starts getting called for folders other than virtual folders,
  // we'll have to do convert those names to modified utf-7. For now, the account manager code
  // that loads the virtual folders for each account, expects utf8 not modified utf-7.
  nsCAutoString escapedName;
  rv = NS_MsgEscapeEncodeURLPath(aName, escapedName);
  NS_ENSURE_SUCCESS(rv, rv);

  uri += escapedName.get();

  nsCOMPtr <nsIMsgFolder> msgFolder;
  rv = GetChildWithURI(uri, PR_FALSE/*deep*/, PR_TRUE /*case Insensitive*/, getter_AddRefs(msgFolder));
  if (NS_SUCCEEDED(rv) && msgFolder)
    return NS_MSG_FOLDER_EXISTS;

  nsCOMPtr<nsIRDFResource> res;
  rv = rdf->GetResource(uri, getter_AddRefs(res));
  if (NS_FAILED(rv))
    return rv;

  nsCOMPtr<nsIMsgFolder> folder(do_QueryInterface(res, &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr <nsILocalFile> path;
  rv = CreateDirectoryForFolder(getter_AddRefs(path));
  NS_ENSURE_SUCCESS(rv, rv);

  folder->GetFlags((PRUint32 *)&flags);

  flags |= nsMsgFolderFlags::Mail;

  folder->SetParent(this);

  folder->SetFlags(flags);

  mSubFolders.AppendObject(folder);
  folder.swap(*aChild);

  nsCOMPtr <nsIMsgImapMailFolder> imapChild = do_QueryInterface(*aChild);
  if (imapChild)
  {
    imapChild->SetOnlineName(NS_LossyConvertUTF16toASCII(aName));
    imapChild->SetHierarchyDelimiter(m_hierarchyDelimiter);
  }
  return rv;
}

nsresult nsImapMailFolder::AddSubfolderWithPath(nsAString& name, nsILocalFile *dbPath,
                                             nsIMsgFolder **child)
{
  NS_ENSURE_ARG_POINTER(child);

  nsresult rv;
  nsCOMPtr<nsIRDFService> rdf(do_GetService(kRDFServiceCID, &rv));
  NS_ENSURE_SUCCESS(rv, rv);
  PRInt32 flags = 0;

  nsCAutoString uri(mURI);
  uri.Append('/');
  AppendUTF16toUTF8(name, uri);

  //will make sure mSubFolders does not have duplicates because of bogus msf files.
  nsCOMPtr <nsIMsgFolder> msgFolder;
  rv = GetChildWithURI(uri, PR_FALSE/*deep*/, PR_FALSE /*case Insensitive*/, getter_AddRefs(msgFolder));
  if (NS_SUCCEEDED(rv) && msgFolder)
    return NS_MSG_FOLDER_EXISTS;

  nsCOMPtr<nsIRDFResource> res;
  rv = rdf->GetResource(uri, getter_AddRefs(res));
  if (NS_FAILED(rv))
    return rv;

  nsCOMPtr<nsIMsgFolder> folder(do_QueryInterface(res, &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  folder->SetFilePath(dbPath);
  nsCOMPtr<nsIMsgImapMailFolder> imapFolder = do_QueryInterface(folder, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  folder->GetFlags((PRUint32 *)&flags);
  folder->SetParent(this);
  flags |= nsMsgFolderFlags::Mail;

  PRBool isServer;
  rv = GetIsServer(&isServer);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 pFlags;
  GetFlags ((PRUint32 *) &pFlags);
  PRBool isParentInbox = pFlags & nsMsgFolderFlags::Inbox;

  //Only set these if these are top level children or parent is inbox
  if(NS_SUCCEEDED(rv))
  {
    if(isServer && name.LowerCaseEqualsLiteral("inbox"))
      flags |= nsMsgFolderFlags::Inbox;
    else if (isServer || isParentInbox)
    {
      nsAutoString trashName;
      GetTrashFolderName(trashName);
      if (name.Equals(trashName))
        flags |= nsMsgFolderFlags::Trash;
    }
#if 0
    else if(name.LowerCaseEqualsLiteral("sent"))
      folder->SetFlag(nsMsgFolderFlags::SentMail);
    else if(name.LowerCaseEqualsLiteral("drafts"))
      folder->SetFlag(nsMsgFolderFlags::Drafts);
    else if (name.LowerCaseEqualsLiteral("templates"))
      folder->SetFlag(nsMsgFolderFlags::Templates);
#endif
  }

  folder->SetFlags(flags);
  //at this point we must be ok and we don't want to return failure in case GetIsServer failed.
  rv = NS_OK;

  if (folder)
    mSubFolders.AppendObject(folder);
  folder.swap(*child);
  return rv;
}

nsresult nsImapMailFolder::CreateSubFolders(nsILocalFile *path)
{
  nsresult rv = NS_OK;
  nsAutoString currentFolderNameStr;    // online name
  nsAutoString currentFolderDBNameStr;  // possibly munged name
  nsCOMPtr<nsIMsgFolder> child;
  nsCOMPtr<nsIMsgIncomingServer> server;
  nsCOMPtr<nsIImapIncomingServer> imapServer;
  rv = GetServer(getter_AddRefs(server));
  NS_ENSURE_SUCCESS(rv, rv);

  imapServer = do_QueryInterface(server, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool isServer;
  rv = GetIsServer(&isServer);

  nsCAutoString folderName;
  nsCOMPtr <nsISimpleEnumerator> children;
  rv = path->GetDirectoryEntries(getter_AddRefs(children));
  PRBool more = PR_FALSE;
  if (children)
    children->HasMoreElements(&more);
  nsCOMPtr<nsIFile> dirEntry;

  while (more)
  {
    rv = children->GetNext((nsISupports**) getter_AddRefs(dirEntry));
    if (NS_FAILED(rv))
      break;
    rv = children->HasMoreElements(&more);
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr <nsILocalFile> currentFolderPath = do_QueryInterface(dirEntry);
    currentFolderPath->GetNativeLeafName(folderName);
    NS_CopyNativeToUnicode(folderName, currentFolderNameStr);
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
      continue;

    // OK, here we need to get the online name from the folder cache if we can.
    // If we can, use that to create the sub-folder
    nsCOMPtr <nsIMsgFolderCacheElement> cacheElement;
    nsCOMPtr <nsILocalFile> curFolder = do_CreateInstance(NS_LOCAL_FILE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    nsCOMPtr <nsILocalFile> dbFile = do_CreateInstance(NS_LOCAL_FILE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    dbFile->InitWithFile(currentFolderPath);
    curFolder->InitWithFile(currentFolderPath);
    // don't strip off the .msf in currentFolderPath.
    currentFolderPath->SetNativeLeafName(folderName);
    currentFolderDBNameStr = currentFolderNameStr;
    nsAutoString utf7LeafName = currentFolderNameStr;

    if (curFolder)
    {
      rv = GetFolderCacheElemFromFile(dbFile, getter_AddRefs(cacheElement));
      if (NS_SUCCEEDED(rv) && cacheElement)
      {
        nsString unicodeName;
        nsCString onlineFullUtf7Name;

        PRUint32 folderFlags;
        rv = cacheElement->GetInt32Property("flags", (PRInt32 *) &folderFlags);
        if (NS_SUCCEEDED(rv) && folderFlags & nsMsgFolderFlags::Virtual) //ignore virtual folders
          continue;
        PRInt32 hierarchyDelimiter;
        rv = cacheElement->GetInt32Property("hierDelim", &hierarchyDelimiter);
        if (NS_SUCCEEDED(rv) && hierarchyDelimiter == kOnlineHierarchySeparatorUnknown)
        {
          currentFolderPath->Remove(PR_FALSE);
          continue; // blow away .msf files for folders with unknown delimiter.
        }
        rv = cacheElement->GetStringProperty("onlineName", onlineFullUtf7Name);
        if (NS_SUCCEEDED(rv) && !onlineFullUtf7Name.IsEmpty())
        {
          currentFolderNameStr.Assign(unicodeName);

          PRUnichar delimiter = 0;
          GetHierarchyDelimiter(&delimiter);
          PRInt32 leafPos = currentFolderNameStr.RFindChar(delimiter);
          if (leafPos > 0)
            currentFolderNameStr.Cut(0, leafPos + 1);

          // take the utf7 full online name, and determine the utf7 leaf name
          CopyASCIItoUTF16(onlineFullUtf7Name, utf7LeafName);
          leafPos = utf7LeafName.RFindChar(delimiter);
          if (leafPos > 0)
            utf7LeafName.Cut(0, leafPos + 1);
        }
      }
    }
      // make the imap folder remember the file spec it was created with.
    nsCAutoString leafName;
    NS_CopyUnicodeToNative(currentFolderDBNameStr, leafName);
    nsCOMPtr <nsILocalFile> msfFilePath = do_CreateInstance(NS_LOCAL_FILE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    msfFilePath->InitWithFile(currentFolderPath);
    if (NS_SUCCEEDED(rv) && msfFilePath)
    {
      // leaf name is the db name w/o .msf (nsShouldIgnoreFile strips it off)
      // so this trims the .msf off the file spec.
      msfFilePath->SetNativeLeafName(leafName);
    }
    // use the utf7 name as the uri for the folder.
    AddSubfolderWithPath(utf7LeafName, msfFilePath, getter_AddRefs(child));
    if (child)
    {
      // use the unicode name as the "pretty" name. Set it so it won't be
      // automatically computed from the URI, which is in utf7 form.
      if (!currentFolderNameStr.IsEmpty())
        child->SetPrettyName(currentFolderNameStr);
      child->SetMsgDatabase(nsnull);
    }
  }
  return rv;
}

NS_IMETHODIMP nsImapMailFolder::GetSubFolders(nsISimpleEnumerator **aResult)
{
  PRBool isServer;
  nsresult rv = GetIsServer(&isServer);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!m_initialized)
  {
    nsCOMPtr<nsILocalFile> pathFile;
    rv = GetFilePath(getter_AddRefs(pathFile));
    if (NS_FAILED(rv)) return rv;

    // host directory does not need .sbd tacked on
    if (!isServer)
    {
      rv = AddDirectorySeparator(pathFile);
      if(NS_FAILED(rv)) return rv;
    }

    m_initialized = PR_TRUE;      // need to set this here to avoid infinite recursion from CreateSubfolders.
    // we have to treat the root folder specially, because it's name
    // doesn't end with .sbd

    PRInt32 newFlags = nsMsgFolderFlags::Mail;
    PRBool isDirectory = PR_FALSE;
    pathFile->IsDirectory(&isDirectory);
    if (isDirectory)
    {
        newFlags |= (nsMsgFolderFlags::Directory | nsMsgFolderFlags::Elided);
        if (!mIsServer)
          SetFlag(newFlags);
        rv = CreateSubFolders(pathFile);
    }
    if (isServer)
    {
      nsCOMPtr <nsIMsgFolder> inboxFolder;

      GetFolderWithFlags(nsMsgFolderFlags::Inbox, getter_AddRefs(inboxFolder));
      if (!inboxFolder)
      {
        // create an inbox if we don't have one.
        CreateClientSubfolderInfo(NS_LITERAL_CSTRING("INBOX"), kOnlineHierarchySeparatorUnknown, 0, PR_TRUE);
      }
    }
    UpdateSummaryTotals(PR_FALSE);
    if (NS_FAILED(rv)) return rv;
  }

  return aResult ? NS_NewArrayEnumerator(aResult, mSubFolders) : NS_ERROR_NULL_POINTER;
}

//Makes sure the database is open and exists.  If the database is valid then
//returns NS_OK.  Otherwise returns a failure error value.
nsresult nsImapMailFolder::GetDatabase(nsIMsgWindow *aMsgWindow)
{
  nsresult folderOpen = NS_OK;
  if (!mDatabase)
  {
    nsresult rv;
    nsCOMPtr<nsIMsgDBService> msgDBService = do_GetService(NS_MSGDB_SERVICE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    folderOpen = msgDBService->OpenFolderDB(this, PR_TRUE, PR_FALSE, getter_AddRefs(mDatabase));

    if (NS_FAILED(folderOpen) && folderOpen != NS_MSG_ERROR_FOLDER_SUMMARY_MISSING)
      folderOpen = msgDBService->OpenFolderDB(this, PR_TRUE, PR_TRUE, getter_AddRefs(mDatabase));

    if (NS_FAILED(folderOpen) && folderOpen != NS_MSG_ERROR_FOLDER_SUMMARY_MISSING)
      return folderOpen;

    if(folderOpen == NS_MSG_ERROR_FOLDER_SUMMARY_MISSING)
      folderOpen = NS_OK;

    if(mDatabase)
    {
      UpdateNewMessages();
      if(mAddListener)
        mDatabase->AddListener(this);
      UpdateSummaryTotals(PR_TRUE);
    }
  }
  return folderOpen;
}

NS_IMETHODIMP nsImapMailFolder::UpdateFolder(nsIMsgWindow * inMsgWindow)
{
  return UpdateFolder(inMsgWindow, nsnull);
}

NS_IMETHODIMP nsImapMailFolder::UpdateFolder(nsIMsgWindow *msgWindow, nsIUrlListener *aUrlListener)
{
  nsresult rv;
  PRBool selectFolder = PR_FALSE;

  if (mFlags & nsMsgFolderFlags::Inbox && !m_filterList)
    rv = GetFilterList(msgWindow, getter_AddRefs(m_filterList));

  if (m_filterList)
  {
    nsCOMPtr<nsIMsgIncomingServer> server;
    rv = GetServer(getter_AddRefs(server));
    NS_ENSURE_SUCCESS(rv, rv);

    PRBool canFileMessagesOnServer = PR_TRUE;
    rv = server->GetCanFileMessagesOnServer(&canFileMessagesOnServer);
    // the mdn filter is for filing return receipts into the sent folder
    // some servers (like AOL mail servers)
    // can't file to the sent folder, so we don't add the filter for those servers
    if (canFileMessagesOnServer)
    {
      rv = server->ConfigureTemporaryFilters(m_filterList);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

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
        rv = CreateClientSubfolderInfo(NS_LITERAL_CSTRING("Inbox"), kOnlineHierarchySeparatorUnknown,0, PR_FALSE);
        NS_ENSURE_SUCCESS(rv, rv);
      }
      m_haveDiscoveredAllFolders = PR_TRUE;
    }
    selectFolder = PR_FALSE;
  }
  rv = GetDatabase(msgWindow);
  if (NS_FAILED(rv))
  {
    ThrowAlertMsg("errorGettingDB", msgWindow);
    return rv;
  }
  PRBool canOpenThisFolder = PR_TRUE;
  GetCanOpenFolder(&canOpenThisFolder);

  PRBool hasOfflineEvents = PR_FALSE;
  GetFlag(nsMsgFolderFlags::OfflineEvents, &hasOfflineEvents);

  if (!WeAreOffline())
  {
    if (hasOfflineEvents)
    {
      nsImapOfflineSync *goOnline = new nsImapOfflineSync(msgWindow, this, this);
      if (goOnline)
        return goOnline->ProcessNextOperation();
    }
  }
  else // we're offline - check if we're password protecting the offline store
  {
    nsCOMPtr<nsIMsgAccountManager> accountManager = do_GetService(NS_MSGACCOUNTMANAGER_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    PRBool userNeedsToAuthenticate = PR_FALSE;
    // if we're PasswordProtectLocalCache, then we need to find out if the server is authenticated.
    (void) accountManager->GetUserNeedsToAuthenticate(&userNeedsToAuthenticate);
    if (userNeedsToAuthenticate)
    {
      nsCOMPtr<nsIMsgIncomingServer> server;
      rv = GetServer(getter_AddRefs(server));
      if (NS_SUCCEEDED(rv))
      {
        PRBool passwordMatches = PR_FALSE;
        rv = PromptForCachePassword(server, msgWindow, passwordMatches);
        if (!passwordMatches)
          return NS_ERROR_FAILURE;
      }
    }
  }
  if (!canOpenThisFolder)
    selectFolder = PR_FALSE;
  // don't run select if we can't select the folder...
  if (NS_SUCCEEDED(rv) && !m_urlRunning && selectFolder)
  {
    nsCOMPtr<nsIImapService> imapService = do_GetService(NS_IMAPSERVICE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr <nsIURI> url;
    rv = imapService->SelectFolder(m_thread, this, m_urlListener, msgWindow, getter_AddRefs(url));
    if (NS_SUCCEEDED(rv))
    {
      m_urlRunning = PR_TRUE;
      m_updatingFolder = PR_TRUE;
    }
    if (url)
    {
      nsCOMPtr <nsIMsgMailNewsUrl> mailnewsUrl = do_QueryInterface(url, &rv);
      NS_ENSURE_SUCCESS(rv, rv);
      mailnewsUrl->RegisterListener(this);
      m_urlListener = aUrlListener;
    }
    switch (rv)
    {
      case NS_MSG_ERROR_OFFLINE:
        if (msgWindow)
          AutoCompact(msgWindow);
        // note fall through to next case.
      case NS_BINDING_ABORTED:
        rv = NS_OK;
        NotifyFolderEvent(mFolderLoadedAtom);
        break;
      default:
        break;
    }
  }
  else if (NS_SUCCEEDED(rv))  // tell the front end that the folder is loaded if we're not going to
  {                           // actually run a url.
    if (!m_updatingFolder)    // if we're already running an update url, we'll let that one send the folder loaded
      NotifyFolderEvent(mFolderLoadedAtom);
  }
  return rv;
}

NS_IMETHODIMP nsImapMailFolder::GetMessages(nsIMsgWindow *aMsgWindow, nsISimpleEnumerator* *result)
{
  NS_ENSURE_ARG_POINTER(result);
  if (!mDatabase)
    GetDatabase(nsnull);
  if (mDatabase)
    return mDatabase->EnumerateMessages(result);
  return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP nsImapMailFolder::CreateSubfolder(const nsAString& folderName, nsIMsgWindow *msgWindow)
{
  NS_ENSURE_TRUE(!folderName.IsEmpty(), NS_ERROR_FAILURE);
  nsresult rv;
  nsAutoString trashName;
  GetTrashFolderName(trashName);
  if ( folderName.Equals(trashName))   // Trash , a special folder
  {
    ThrowAlertMsg("folderExists", msgWindow);
    return NS_MSG_FOLDER_EXISTS;
  }
  else if (mIsServer && folderName.LowerCaseEqualsLiteral("inbox"))  // Inbox, a special folder
  {
    ThrowAlertMsg("folderExists", msgWindow);
    return NS_MSG_FOLDER_EXISTS;
  }

  nsCOMPtr<nsIImapService> imapService = do_GetService(NS_IMAPSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv,rv);

  return imapService->CreateFolder(m_thread, this, folderName, this, nsnull);
}

NS_IMETHODIMP nsImapMailFolder::CreateClientSubfolderInfo(const nsACString& folderName, PRUnichar hierarchyDelimiter, PRInt32 flags, PRBool suppressNotification)
{
  nsresult rv = NS_OK;

  //Get a directory based on our current path.
  nsCOMPtr <nsILocalFile> path;
  rv = CreateDirectoryForFolder(getter_AddRefs(path));
  if(NS_FAILED(rv))
    return rv;

    NS_ConvertASCIItoUTF16 leafName(folderName);
    nsAutoString folderNameStr;
    nsAutoString parentName = leafName;
    // use RFind, because folder can start with a delimiter and
    // not be a leaf folder.
    PRInt32 folderStart = leafName.RFindChar('/');
    if (folderStart > 0)
    {
        nsCOMPtr<nsIRDFService> rdf(do_GetService(kRDFServiceCID, &rv));
        NS_ENSURE_SUCCESS(rv, rv);
        nsCOMPtr<nsIRDFResource> res;
        nsCOMPtr<nsIMsgImapMailFolder> parentFolder;
        nsCAutoString uri (mURI);
        parentName.Right(leafName, leafName.Length() - folderStart - 1);
        parentName.Truncate(folderStart);

        // the parentName might be too long or have some illegal chars
        // so we make it safe.
        // XXX Here it's assumed that IMAP folder names are stored locally
        // in modified UTF-7 (ASCII-only) as is stored remotely.  If we ever change
        // this, we have to work with nsString instead of nsCString
        // (ref. bug 264071)
        nsCAutoString safeParentName;
        LossyCopyUTF16toASCII(parentName, safeParentName);
        NS_MsgHashIfNecessary(safeParentName);
        path->AppendNative(safeParentName);

        // path is an out parameter to CreateDirectoryForFolder - I'm not
        // sure why we're munging it above.
        rv = CreateDirectoryForFolder(getter_AddRefs(path));
        if (NS_FAILED(rv)) return rv;
        uri.Append('/');
        LossyAppendUTF16toASCII(parentName, uri);
        rv = rdf->GetResource(uri, getter_AddRefs(res));
        if (NS_FAILED(rv)) return rv;
        parentFolder = do_QueryInterface(res, &rv);
        NS_ENSURE_SUCCESS(rv, rv);
        nsCAutoString leafnameC;
        LossyCopyUTF16toASCII(leafName, leafnameC);
        return parentFolder->CreateClientSubfolderInfo(leafnameC, hierarchyDelimiter,flags, suppressNotification);
    }

  // if we get here, it's really a leaf, and "this" is the parent.
  folderNameStr = leafName;

  // Create an empty database for this mail folder, set its name from the user
  nsCOMPtr<nsIMsgDatabase> mailDBFactory;
  nsCOMPtr<nsIMsgFolder> child;

  nsCOMPtr<nsIMsgDBService> msgDBService = do_GetService(NS_MSGDB_SERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIMsgDatabase> unusedDB;
  nsCOMPtr <nsILocalFile> dbFile;

  // warning, path will be changed
  rv = CreateFileForDB(folderName, path, getter_AddRefs(dbFile));
  NS_ENSURE_SUCCESS(rv,rv);

  //Now let's create the actual new folder
  rv = AddSubfolderWithPath(folderNameStr, dbFile, getter_AddRefs(child));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = msgDBService->OpenMailDBFromFile(dbFile, PR_TRUE, PR_TRUE, (nsIMsgDatabase **) getter_AddRefs(unusedDB));
  if (rv == NS_MSG_ERROR_FOLDER_SUMMARY_MISSING)
    rv = NS_OK;

  if (NS_SUCCEEDED(rv) && unusedDB)
  {
  //need to set the folder name
    nsCOMPtr <nsIDBFolderInfo> folderInfo;
    rv = unusedDB->GetDBFolderInfo(getter_AddRefs(folderInfo));
    nsCOMPtr <nsIMsgImapMailFolder> imapFolder = do_QueryInterface(child, &rv);
    if (NS_SUCCEEDED(rv))
    {
      nsCAutoString onlineName(m_onlineFolderName);
      if (!onlineName.IsEmpty())
        onlineName.Append(char(hierarchyDelimiter));
      LossyAppendUTF16toASCII(folderNameStr, onlineName);
      imapFolder->SetVerifiedAsOnlineFolder(PR_TRUE);
      imapFolder->SetOnlineName(onlineName);
      imapFolder->SetHierarchyDelimiter(hierarchyDelimiter);
      imapFolder->SetBoxFlags(flags);

      child->SetFlag(nsMsgFolderFlags::Elided);
      nsString unicodeName;
      rv = CopyMUTF7toUTF16(nsCString(folderName), unicodeName);
      if (NS_SUCCEEDED(rv))
        child->SetPrettyName(unicodeName);

      // store the online name as the mailbox name in the db folder info
      // I don't think anyone uses the mailbox name, so we'll use it
      // to restore the online name when blowing away an imap db.
      if (folderInfo)
        folderInfo->SetMailboxName(NS_ConvertASCIItoUTF16(onlineName));
    }

    unusedDB->SetSummaryValid(PR_TRUE);
    unusedDB->Commit(nsMsgDBCommitType::kLargeCommit);
    unusedDB->Close(PR_TRUE);
    // don't want to hold onto this newly created db.
    child->SetMsgDatabase(nsnull);
  }

  if (!suppressNotification)
  {
    nsCOMPtr <nsIAtom> folderCreateAtom;
    if(NS_SUCCEEDED(rv) && child)
    {
      NotifyItemAdded(child);
      folderCreateAtom = do_GetAtom("FolderCreateCompleted");
      child->NotifyFolderEvent(folderCreateAtom);
    }
    else
    {
      folderCreateAtom = do_GetAtom("FolderCreateFailed");
      NotifyFolderEvent(folderCreateAtom);
    }
  }
  return rv;
}

NS_IMETHODIMP nsImapMailFolder::List()
{
  nsresult rv;
  nsCOMPtr<nsIImapService> imapService = do_GetService(NS_IMAPSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv,rv);
  return imapService->ListFolder(m_thread, this, this, nsnull);
}

NS_IMETHODIMP nsImapMailFolder::RemoveSubFolder (nsIMsgFolder *which)
{
  nsresult rv;
  nsCOMPtr<nsIMutableArray> folders(do_CreateInstance(NS_ARRAY_CONTRACTID, &rv));
  NS_ENSURE_TRUE(folders, rv);
  nsCOMPtr<nsISupports> folderSupport = do_QueryInterface(which, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  folders->AppendElement(folderSupport, PR_FALSE);
  rv = nsMsgDBFolder::DeleteSubFolders(folders, nsnull);
  which->Delete();
  return rv;
}

NS_IMETHODIMP nsImapMailFolder::CreateStorageIfMissing(nsIUrlListener* urlListener)
{
  nsresult rv = NS_OK;
  nsCOMPtr <nsIMsgFolder> msgParent;
  GetParentMsgFolder(getter_AddRefs(msgParent));

  // parent is probably not set because *this* was probably created by rdf
  // and not by folder discovery. So, we have to compute the parent.
  if (!msgParent)
  {
    nsCAutoString folderName(mURI);

    PRInt32 leafPos = folderName.RFindChar('/');
    nsCAutoString parentName(folderName);

    if (leafPos > 0)
    {
      // If there is a hierarchy, there is a parent.
      // Don't strip off slash if it's the first character
      parentName.Truncate(leafPos);
      // get the corresponding RDF resource
      // RDF will create the folder resource if it doesn't already exist
      nsCOMPtr<nsIRDFService> rdf(do_GetService(kRDFServiceCID, &rv));
      NS_ENSURE_SUCCESS(rv, rv);
      nsCOMPtr<nsIRDFResource> resource;
      rv = rdf->GetResource(parentName, getter_AddRefs(resource));
      if (NS_FAILED(rv)) return rv;
      msgParent = do_QueryInterface(resource, &rv);
    }
  }
  if (msgParent)
  {
    nsString folderName;
    GetName(folderName);
    nsresult rv;
    nsCOMPtr<nsIImapService> imapService = do_GetService(NS_IMAPSERVICE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    nsCOMPtr <nsIURI> uri;
    imapService->EnsureFolderExists(m_thread, msgParent, folderName, urlListener, getter_AddRefs(uri));
  }
  return rv;
}


NS_IMETHODIMP nsImapMailFolder::GetVerifiedAsOnlineFolder(PRBool *aVerifiedAsOnlineFolder)
{
  NS_ENSURE_ARG_POINTER(aVerifiedAsOnlineFolder);
  *aVerifiedAsOnlineFolder = m_verifiedAsOnlineFolder;
  return NS_OK;
}

NS_IMETHODIMP nsImapMailFolder::SetVerifiedAsOnlineFolder(PRBool aVerifiedAsOnlineFolder)
{
  m_verifiedAsOnlineFolder = aVerifiedAsOnlineFolder;
  // mark ancestors as verified as well
  if (aVerifiedAsOnlineFolder)
  {
    nsCOMPtr<nsIMsgFolder> parent;
    do
    {
      GetParent(getter_AddRefs(parent));
      if (parent)
      {
        nsCOMPtr<nsIMsgImapMailFolder> imapParent = do_QueryInterface(parent);
        if (imapParent)
        {
          PRBool verifiedOnline;
          imapParent->GetVerifiedAsOnlineFolder(&verifiedOnline);
          if (verifiedOnline)
            break;
          imapParent->SetVerifiedAsOnlineFolder(PR_TRUE);
        }
      }
    }
    while (parent);
  }
  return NS_OK;
}

NS_IMETHODIMP nsImapMailFolder::GetOnlineDelimiter(char** onlineDelimiter)
{
  NS_ENSURE_ARG_POINTER(onlineDelimiter);
  PRUnichar delimiter = 0;
  GetHierarchyDelimiter(&delimiter);
  nsAutoString str(delimiter);
  *onlineDelimiter = ToNewCString(str);
  return NS_OK;
}

NS_IMETHODIMP nsImapMailFolder::SetHierarchyDelimiter(PRUnichar aHierarchyDelimiter)
{
  m_hierarchyDelimiter = aHierarchyDelimiter;
  return NS_OK;
}

NS_IMETHODIMP nsImapMailFolder::GetHierarchyDelimiter(PRUnichar *aHierarchyDelimiter)
{
  NS_ENSURE_ARG_POINTER(aHierarchyDelimiter);
  if (mIsServer)
  {
    // if it's the root folder, we don't know the delimiter. So look at the
    // first child.
    PRInt32 count = mSubFolders.Count();
    if (count > 0)
    {
      nsCOMPtr<nsIMsgImapMailFolder> childFolder(do_QueryInterface(mSubFolders[0]));
      if (childFolder)
        return childFolder->GetHierarchyDelimiter(aHierarchyDelimiter);
    }
  }
  ReadDBFolderInfo(PR_FALSE); // update cache first.
  *aHierarchyDelimiter = m_hierarchyDelimiter;
  return NS_OK;
}

NS_IMETHODIMP nsImapMailFolder::SetBoxFlags(PRInt32 aBoxFlags)
{
  ReadDBFolderInfo(PR_FALSE);

  m_boxFlags = aBoxFlags;
  PRUint32 newFlags = mFlags;

  newFlags |= nsMsgFolderFlags::ImapBox;

  if (m_boxFlags & kNoinferiors)
    newFlags |= nsMsgFolderFlags::ImapNoinferiors;
  else
    newFlags &= ~nsMsgFolderFlags::ImapNoinferiors;
    if (m_boxFlags & kNoselect)
      newFlags |= nsMsgFolderFlags::ImapNoselect;
    else
      newFlags &= ~nsMsgFolderFlags::ImapNoselect;
    if (m_boxFlags & kPublicMailbox)
      newFlags |= nsMsgFolderFlags::ImapPublic;
    else
      newFlags &= ~nsMsgFolderFlags::ImapPublic;
    if (m_boxFlags & kOtherUsersMailbox)
      newFlags |= nsMsgFolderFlags::ImapOtherUser;
    else
      newFlags &= ~nsMsgFolderFlags::ImapOtherUser;
    if (m_boxFlags & kPersonalMailbox)
      newFlags |= nsMsgFolderFlags::ImapPersonal;
    else
      newFlags &= ~nsMsgFolderFlags::ImapPersonal;

    SetFlags(newFlags);
  return NS_OK;
}

NS_IMETHODIMP nsImapMailFolder::GetBoxFlags(PRInt32 *aBoxFlags)
{
  NS_ENSURE_ARG_POINTER(aBoxFlags);
  *aBoxFlags = m_boxFlags;
  return NS_OK;
}

NS_IMETHODIMP nsImapMailFolder::GetExplicitlyVerify(PRBool *aExplicitlyVerify)
{
  NS_ENSURE_ARG_POINTER(aExplicitlyVerify);
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
  return GetFlag(nsMsgFolderFlags::ImapNoselect, aResult);
}

NS_IMETHODIMP nsImapMailFolder::Compact(nsIUrlListener *aListener, nsIMsgWindow *aMsgWindow)
{
  nsresult rv;

  rv = GetDatabase(nsnull);
  // now's a good time to apply the retention settings. If we do delete any
  // messages, the expunge is going to have to wait until the delete to
  // finish before it can run, but the multiple-connection protection code
  // should handle that.
  if (mDatabase)
    ApplyRetentionSettings();

  // compact offline store, if folder configured for offline use.
  // for now, check aMsgWindow because not having aMsgWindow means
  // we're doing a compact at shut-down. TEMPORARY HACK
  if (aMsgWindow && mFlags & nsMsgFolderFlags::Offline)
    CompactOfflineStore(aMsgWindow);

  nsCOMPtr<nsIImapService> imapService = do_GetService(NS_IMAPSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv,rv);

  return  imapService->Expunge(m_thread, this, aListener, nsnull);
}

NS_IMETHODIMP nsImapMailFolder::CompactAll(nsIUrlListener *aListener,  nsIMsgWindow *aMsgWindow, nsISupportsArray *aFolderArray,
                                           PRBool aCompactOfflineAlso, nsISupportsArray *aOfflineFolderArray)
{
  NS_ASSERTION(!aOfflineFolderArray, "compacting automatically compacts offline stores");
  nsresult rv;
  nsCOMPtr<nsISupportsArray> folderArray;

  if (!aFolderArray)
  {
    nsCOMPtr<nsIMsgFolder> rootFolder;
    nsCOMPtr<nsISupportsArray> allDescendents;
    rv = GetRootFolder(getter_AddRefs(rootFolder));
    if (NS_SUCCEEDED(rv) && rootFolder)
    {
      allDescendents = do_CreateInstance(NS_SUPPORTSARRAY_CONTRACTID, &rv);
      NS_ENSURE_TRUE(allDescendents, rv);
      rootFolder->ListDescendents(allDescendents);
      PRUint32 cnt =0;
      rv = allDescendents->Count(&cnt);
      NS_ENSURE_SUCCESS(rv,rv);
      folderArray = do_CreateInstance(NS_SUPPORTSARRAY_CONTRACTID, &rv);
      NS_ENSURE_TRUE(folderArray, rv);
      for (PRUint32 i=0; i < cnt;i++)
      {
        nsCOMPtr<nsISupports> supports = getter_AddRefs(allDescendents->ElementAt(i));
        nsCOMPtr<nsIMsgFolder> folder = do_QueryInterface(supports, &rv);
        NS_ENSURE_SUCCESS(rv,rv);
        rv = folderArray->AppendElement(supports);
      }
      rv = folderArray->Count(&cnt);
      NS_ENSURE_SUCCESS(rv,rv);
      if (cnt == 0)
        return NotifyCompactCompleted();
    }
  }
  nsCOMPtr <nsIMsgFolderCompactor> folderCompactor =  do_CreateInstance(NS_MSGLOCALFOLDERCOMPACTOR_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  return folderCompactor->CompactAll(aFolderArray ? aFolderArray : folderArray.get(), aMsgWindow,
                                     aCompactOfflineAlso, aOfflineFolderArray);
}

NS_IMETHODIMP nsImapMailFolder::UpdateStatus(nsIUrlListener *aListener, nsIMsgWindow *aMsgWindow)
{
  nsresult rv;
  nsCOMPtr<nsIImapService> imapService = do_GetService(NS_IMAPSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv,rv);

  nsCOMPtr <nsIURI> uri;
  rv = imapService->UpdateFolderStatus(m_thread, this, aListener, getter_AddRefs(uri));
  if (uri && !aMsgWindow)
  {
    nsCOMPtr <nsIMsgMailNewsUrl> mailNewsUrl = do_QueryInterface(uri, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    // if no msg window, we won't put up error messages (this is almost certainly a biff-inspired status)
    mailNewsUrl->SetSuppressErrorMsgs(PR_TRUE);
  }
  return rv;
}

NS_IMETHODIMP nsImapMailFolder::EmptyTrash(nsIMsgWindow *aMsgWindow, nsIUrlListener *aListener)
{
  nsCOMPtr<nsIMsgFolder> trashFolder;
  nsresult rv = GetTrashFolder(getter_AddRefs(trashFolder));
  if (NS_SUCCEEDED(rv))
  {
    nsCOMPtr<nsIMsgAccountManager> accountManager = do_GetService(NS_MSGACCOUNTMANAGER_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    // if we are emptying trash on exit and we are an aol server then don't perform
    // this operation because it's causing a hang that we haven't been able to figure out yet
    // this is an rtm fix and we'll look for the right solution post rtm.
    PRBool empytingOnExit = PR_FALSE;
    accountManager->GetEmptyTrashInProgress(&empytingOnExit);
    if (empytingOnExit)
    {
      nsCOMPtr<nsIImapIncomingServer> imapServer;
      rv = GetImapIncomingServer(getter_AddRefs(imapServer));
      if (imapServer)
      {
        PRBool isAOLServer = PR_FALSE;
        imapServer->GetIsAOLServer(&isAOLServer);
        if (isAOLServer)
          return NS_ERROR_FAILURE;  // we will not be performing an empty trash....
      } // if we fetched an imap server
    } // if emptying trash on exit which is done through the account manager.

    nsCOMPtr<nsIMsgDatabase> trashDB;
    if (WeAreOffline())
    {
      nsCOMPtr <nsIMsgDatabase> trashDB;
      rv = trashFolder->GetMsgDatabase(nsnull, getter_AddRefs(trashDB));
      if (trashDB)
      {
        nsMsgKey fakeKey;
        trashDB->GetNextFakeOfflineMsgKey(&fakeKey);

        nsCOMPtr <nsIMsgOfflineImapOperation> op;
        rv = trashDB->GetOfflineOpForKey(fakeKey, PR_TRUE, getter_AddRefs(op));
        trashFolder->SetFlag(nsMsgFolderFlags::OfflineEvents);
        op->SetOperation(nsIMsgOfflineImapOperation::kDeleteAllMsgs);
      }
      return rv;
    }
    nsCOMPtr <nsIDBFolderInfo> transferInfo;
    rv = trashFolder->GetDBTransferInfo(getter_AddRefs(transferInfo));
    rv = trashFolder->Delete(); // delete summary spec
    trashFolder->SetDBTransferInfo(transferInfo);

    trashFolder->SetSizeOnDisk(0);
    nsCOMPtr<nsIImapService> imapService = do_GetService(NS_IMAPSERVICE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    PRBool hasSubfolders = PR_FALSE;
    rv = trashFolder->GetHasSubFolders(&hasSubfolders);
    if (hasSubfolders)
    {
      PRBool confirmDeletion;
      nsCOMPtr<nsIPrefBranch> prefBranch(do_GetService(NS_PREFSERVICE_CONTRACTID, &rv));
      NS_ENSURE_SUCCESS(rv, rv);

      prefBranch->GetBoolPref("mail.imap.confirm_emptyTrashFolderDeletion", &confirmDeletion);

      if (confirmDeletion)
      {
        nsCOMPtr<nsISimpleEnumerator> enumerator;
        rv = trashFolder->GetSubFolders(getter_AddRefs(enumerator));
        NS_ENSURE_SUCCESS(rv, rv);

        nsCOMPtr<nsIPromptService> promptService(do_GetService(NS_PROMPTSERVICE_CONTRACTID, &rv));
        NS_ENSURE_SUCCESS(rv, rv);

        nsCOMPtr<nsIDOMWindowInternal> parentWindow;
        if (aMsgWindow)
        {
          nsCOMPtr<nsIDocShell> docShell;
          (void) aMsgWindow->GetRootDocShell(getter_AddRefs(docShell));
          parentWindow = do_QueryInterface(docShell);
        }

        nsCOMPtr<nsIStringBundle> bundle;
        rv = IMAPGetStringBundle(getter_AddRefs(bundle));
        NS_ENSURE_SUCCESS(rv, rv);

        PRBool hasMore;
        while (NS_SUCCEEDED(enumerator->HasMoreElements(&hasMore)) && hasMore)
        {
          PRInt32 dlgResult = -1;
          nsCOMPtr<nsISupports> item;
          rv = enumerator->GetNext(getter_AddRefs(item));
          if (NS_FAILED(rv))
            continue;

          if (confirmDeletion)
          {
            nsCOMPtr<nsIMsgFolder> folder(do_QueryInterface(item, &rv));
            if (NS_FAILED(rv))
              continue;
            nsString confirmText;
            nsString folderName;
            folder->GetName(folderName);
            const PRUnichar *formatStrings[1] = { folderName.get() };
            rv = bundle->FormatStringFromID(IMAP_EMPTY_TRASH_CONFIRM,
                                              formatStrings, 1,
                                              getter_Copies(confirmText));
            // Got the text, now show dialog.
            rv = promptService->ConfirmEx(parentWindow, nsnull, confirmText.get(),
                                        (nsIPromptService::BUTTON_TITLE_OK * nsIPromptService::BUTTON_POS_0) +
                                        (nsIPromptService::BUTTON_TITLE_CANCEL * nsIPromptService::BUTTON_POS_1),
                                          nsnull, nsnull, nsnull, nsnull, nsnull, &dlgResult);
          }
          if (NS_SUCCEEDED(rv) && dlgResult == 1)
            return NS_BINDING_ABORTED;
        }
      }
    }
    if (aListener)
      rv = imapService->DeleteAllMessages(m_thread, trashFolder, aListener, nsnull);
    else
    {
      nsCOMPtr<nsIUrlListener> urlListener = do_QueryInterface(trashFolder);
      rv = imapService->DeleteAllMessages(m_thread, trashFolder, urlListener, nsnull);
    }
    // return an error if this failed. We want the empty trash on exit code
    // to know if this fails so that it doesn't block waiting for empty trash to finish.
    if (NS_FAILED(rv))
      return rv;

    if (hasSubfolders)
    {
      nsCOMPtr<nsISimpleEnumerator> enumerator;
      nsCOMPtr<nsISupports> item;
      nsCOMArray<nsIMsgFolder> array;

      rv = trashFolder->GetSubFolders(getter_AddRefs(enumerator));
      NS_ENSURE_SUCCESS(rv, rv);

      PRBool hasMore;
      while (NS_SUCCEEDED(enumerator->HasMoreElements(&hasMore)) && hasMore)
      {
        rv = enumerator->GetNext(getter_AddRefs(item));
        if (NS_SUCCEEDED(rv))
        {
          nsCOMPtr<nsIMsgFolder> folder(do_QueryInterface(item, &rv));
          if (NS_SUCCEEDED(rv))
            array.AppendObject(folder);
        }
      }
      for (PRInt32 i = array.Count() - 1; i >= 0; i--)
      {
        trashFolder->PropagateDelete(array[i], PR_TRUE, aMsgWindow);
        // Remove the object, presumably to free it up before we delete the next.
        array.RemoveObjectAt(i);
      }
    }

    // The trash folder has effectively been deleted
    nsCOMPtr<nsIMsgFolderNotificationService> notifier(do_GetService(NS_MSGNOTIFICATIONSERVICE_CONTRACTID));
    if (notifier)
      notifier->NotifyFolderDeleted(trashFolder);

    return NS_OK;
  }
  return rv;
}

NS_IMETHODIMP nsImapMailFolder::Delete()
{
  nsresult rv;
  if (mDatabase)
  {
    mDatabase->ForceClosed();
    mDatabase = nsnull;
  }

  nsCOMPtr<nsILocalFile> path;
  rv = GetFilePath(getter_AddRefs(path));
  if (NS_SUCCEEDED(rv))
  {
    nsCOMPtr<nsILocalFile> summaryLocation;
    rv = GetSummaryFileLocation(path, getter_AddRefs(summaryLocation));
    if (NS_SUCCEEDED(rv))
    {
      PRBool exists = PR_FALSE;
      rv = summaryLocation->Exists(&exists);
      if (NS_SUCCEEDED(rv) && exists)
      {
        rv = summaryLocation->Remove(PR_FALSE);
        if (NS_FAILED(rv))
          NS_WARNING("failed to remove imap summary file");
      }
    }
  }
  if (mPath)
    mPath->Remove(PR_FALSE);
  // should notify nsIMsgFolderListeners about the folder getting deleted...
  return rv;
}

NS_IMETHODIMP nsImapMailFolder::Rename (const nsAString& newName, nsIMsgWindow *msgWindow)
{
  if (mFlags & nsMsgFolderFlags::Virtual)
    return nsMsgDBFolder::Rename(newName, msgWindow);
  nsresult rv;
  nsAutoString newNameStr(newName);
  if (newNameStr.FindChar(m_hierarchyDelimiter, 0) != -1)
  {
    nsCOMPtr<nsIDocShell> docShell;
    if (msgWindow)
      msgWindow->GetRootDocShell(getter_AddRefs(docShell));
    if (docShell)
    {
      nsCOMPtr<nsIStringBundle> bundle;
      rv = IMAPGetStringBundle(getter_AddRefs(bundle));
      if (NS_SUCCEEDED(rv) && bundle)
      {
        const PRUnichar *formatStrings[] =
        {
           (const PRUnichar*) m_hierarchyDelimiter
        };
        nsString alertString;
        rv = bundle->FormatStringFromID(IMAP_SPECIAL_CHAR,
                                      formatStrings, 1,
                                      getter_Copies(alertString));
        nsCOMPtr<nsIPrompt> dialog(do_GetInterface(docShell));
        if (dialog && !alertString.IsEmpty())
          dialog->Alert(nsnull, alertString.get());
      }
    }
    return NS_ERROR_FAILURE;
  }
  nsCOMPtr <nsIImapIncomingServer> incomingImapServer;
  GetImapIncomingServer(getter_AddRefs(incomingImapServer));
  if (incomingImapServer)
    RecursiveCloseActiveConnections(incomingImapServer);

  nsCOMPtr<nsIImapService> imapService = do_GetService(NS_IMAPSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv,rv);
  return imapService->RenameLeaf(m_thread, this, newName, this, msgWindow, nsnull);
}

NS_IMETHODIMP nsImapMailFolder::RecursiveCloseActiveConnections(nsIImapIncomingServer *incomingImapServer)
{
  NS_ENSURE_ARG(incomingImapServer);

  nsCOMPtr<nsIMsgImapMailFolder> folder;
  PRInt32 count = mSubFolders.Count();
  for (PRInt32 i = 0; i < count; i++)
  {
    folder = do_QueryInterface(mSubFolders[i]);
    if (folder)
      folder->RecursiveCloseActiveConnections(incomingImapServer);

    incomingImapServer->CloseConnectionForFolder(mSubFolders[i]);
  }
  return NS_OK;
}

// this is called *after* we've done the rename on the server.
NS_IMETHODIMP nsImapMailFolder::PrepareToRename()
{
  nsCOMPtr<nsIMsgImapMailFolder> folder;
  PRInt32 count = mSubFolders.Count();
  for (PRInt32 i = 0; i < count; i++)
  {
    folder = do_QueryInterface(mSubFolders[i]);
    if (folder)
      folder->PrepareToRename();
  }

  SetOnlineName(EmptyCString());
  return NS_OK;
}

NS_IMETHODIMP nsImapMailFolder::RenameLocal(const nsACString& newName, nsIMsgFolder *parent)
{
  // XXX Here it's assumed that IMAP folder names are stored locally
  // in modified UTF-7 (ASCII-only) as is stored remotely.  If we ever change
  // this, we have to work with nsString instead of nsCString
  // (ref. bug 264071)
  nsCAutoString leafname(newName);
  nsCAutoString parentName;
  // newName always in the canonical form "greatparent/parentname/leafname"
  PRInt32 leafpos = leafname.RFindChar('/');
  if (leafpos >0)
      leafname.Cut(0, leafpos+1);
  m_msgParser = nsnull;
  PrepareToRename();
  ForceDBClosed();

  nsresult rv = NS_OK;
  nsCOMPtr<nsILocalFile> oldPathFile;
  rv = GetFilePath(getter_AddRefs(oldPathFile));
  if (NS_FAILED(rv)) return rv;

  nsCOMPtr<nsILocalFile> parentPathFile;
  rv = parent->GetFilePath(getter_AddRefs(parentPathFile));
  NS_ENSURE_SUCCESS(rv,rv);

  PRBool isDirectory = PR_FALSE;
  parentPathFile->IsDirectory(&isDirectory);
  if (!isDirectory)
  AddDirectorySeparator(parentPathFile);

  nsCOMPtr <nsILocalFile> dirFile;

  PRInt32 count = mSubFolders.Count();
  if (count > 0)
    rv = CreateDirectoryForFolder(getter_AddRefs(dirFile));

  nsCOMPtr <nsILocalFile> oldSummaryFile;
  rv = GetSummaryFileLocation(oldPathFile, getter_AddRefs(oldSummaryFile));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCAutoString newNameStr;
  oldSummaryFile->Remove(PR_FALSE);
  if (count > 0)
  {
    newNameStr = leafname;
    NS_MsgHashIfNecessary(newNameStr);
    newNameStr += ".sbd";
    nsCAutoString leafName;
    dirFile->GetNativeLeafName(leafName);
    if (!leafName.Equals(newNameStr))
      return dirFile->MoveToNative(nsnull, newNameStr);      // in case of rename operation leaf names will differ

    parentPathFile->AppendNative(newNameStr);    //only for move we need to progress further in case the parent differs
    PRBool isDirectory = PR_FALSE;
    parentPathFile->IsDirectory(&isDirectory);
    if (!isDirectory)
      parentPathFile->Create(nsIFile::DIRECTORY_TYPE, 0700);
    else
      NS_ASSERTION(0,"Directory already exists.");
    rv = RecursiveCopy(dirFile, parentPathFile);
    NS_ENSURE_SUCCESS(rv,rv);
    dirFile->Remove(PR_TRUE);                         // moving folders
  }
  return rv;
}

NS_IMETHODIMP nsImapMailFolder::GetPrettyName(nsAString& prettyName)
{
  return GetName(prettyName);
}

NS_IMETHODIMP nsImapMailFolder::UpdateSummaryTotals(PRBool force)
{
  // bug 72871 inserted the mIsServer check for IMAP
  return mIsServer? NS_OK : nsMsgDBFolder::UpdateSummaryTotals(force);
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
  NS_ENSURE_ARG_POINTER(size);
  *size = mFolderSize;
  return NS_OK;
}

NS_IMETHODIMP
nsImapMailFolder::GetCanCreateSubfolders(PRBool *aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);
  *aResult = !(mFlags & (nsMsgFolderFlags::ImapNoinferiors | nsMsgFolderFlags::Virtual));

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
      *aResult = (mFlags & nsMsgFolderFlags::ImapNoselect);
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

nsresult nsImapMailFolder::GetServerKey(nsACString& serverKey)
{
  // look for matching imap folders, then pop folders
  nsCOMPtr<nsIMsgIncomingServer> server;
  nsresult rv = GetServer(getter_AddRefs(server));
  if (NS_SUCCEEDED(rv))
    rv = server->GetKey(serverKey);
  return rv;
}

NS_IMETHODIMP
nsImapMailFolder::GetImapIncomingServer(nsIImapIncomingServer **aImapIncomingServer)
{
  NS_ENSURE_ARG(aImapIncomingServer);
  nsCOMPtr<nsIMsgIncomingServer> server;
  if (NS_SUCCEEDED(GetServer(getter_AddRefs(server))) && server)
  {
    nsCOMPtr <nsIImapIncomingServer> incomingServer = do_QueryInterface(server);
    incomingServer.swap(*aImapIncomingServer);
    return NS_OK;
  }
  return NS_ERROR_NULL_POINTER;
}

NS_IMETHODIMP
nsImapMailFolder::AddMessageDispositionState(nsIMsgDBHdr *aMessage, nsMsgDispositionState aDispositionFlag)
{
  nsMsgDBFolder::AddMessageDispositionState(aMessage, aDispositionFlag);

  // set the mark message answered flag on the server for this message...
  if (aMessage)
  {
    nsMsgKey msgKey;
    aMessage->GetMessageKey(&msgKey);

    if (aDispositionFlag == nsIMsgFolder::nsMsgDispositionState_Replied)
      StoreImapFlags(kImapMsgAnsweredFlag, PR_TRUE, &msgKey, 1, nsnull);
    else if (aDispositionFlag == nsIMsgFolder::nsMsgDispositionState_Forwarded)
      StoreImapFlags(kImapMsgForwardedFlag, PR_TRUE, &msgKey, 1, nsnull);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsImapMailFolder::MarkMessagesRead(nsIArray *messages, PRBool markRead)
{
  // tell the folder to do it, which will mark them read in the db.
  nsresult rv = nsMsgDBFolder::MarkMessagesRead(messages, markRead);
  if (NS_SUCCEEDED(rv))
  {
    nsCAutoString messageIds;
    nsTArray<nsMsgKey> keysToMarkRead;
    rv = BuildIdsAndKeyArray(messages, messageIds, keysToMarkRead);
    if (NS_FAILED(rv)) return rv;

    StoreImapFlags(kImapMsgSeenFlag, markRead,  keysToMarkRead.Elements(), keysToMarkRead.Length(), nsnull);
    rv = GetDatabase(nsnull);
    if (NS_SUCCEEDED(rv))
      mDatabase->Commit(nsMsgDBCommitType::kLargeCommit);
  }
  return rv;
}

NS_IMETHODIMP
nsImapMailFolder::SetLabelForMessages(nsIArray *aMessages, nsMsgLabelValue aLabel)
{
  NS_ENSURE_ARG(aMessages);

  nsresult rv = nsMsgDBFolder::SetLabelForMessages(aMessages, aLabel);
  if (NS_SUCCEEDED(rv))
  {
    nsCAutoString messageIds;
    nsTArray<nsMsgKey> keysToLabel;
    nsresult rv = BuildIdsAndKeyArray(aMessages, messageIds, keysToLabel);
    NS_ENSURE_SUCCESS(rv, rv);
    StoreImapFlags((aLabel << 9), PR_TRUE, keysToLabel.Elements(), keysToLabel.Length(), nsnull);
    rv = GetDatabase(nsnull);
    if (NS_SUCCEEDED(rv))
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
    nsTArray<nsMsgKey> thoseMarked;
    EnableNotifications(allMessageCountNotifications, PR_FALSE, PR_TRUE /*dbBatching*/);
    rv = mDatabase->MarkAllRead(&thoseMarked);
    EnableNotifications(allMessageCountNotifications, PR_TRUE, PR_TRUE /*dbBatching*/);
    if (NS_SUCCEEDED(rv))
    {
      rv = StoreImapFlags(kImapMsgSeenFlag, PR_TRUE, thoseMarked.Elements(),
                          thoseMarked.Length(), nsnull);
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
    nsTArray<nsMsgKey> thoseMarked;
    rv = mDatabase->MarkThreadRead(thread, nsnull, &thoseMarked);
    if (NS_SUCCEEDED(rv))
    {
      rv = StoreImapFlags(kImapMsgSeenFlag, PR_TRUE, thoseMarked.Elements(),
                          thoseMarked.Length(), nsnull);
      mDatabase->Commit(nsMsgDBCommitType::kLargeCommit);
    }
  }
  return rv;
}


NS_IMETHODIMP nsImapMailFolder::ReadFromFolderCacheElem(nsIMsgFolderCacheElement *element)
{
  nsresult rv = nsMsgDBFolder::ReadFromFolderCacheElem(element);
  PRInt32 hierarchyDelimiter = kOnlineHierarchySeparatorUnknown;
  nsCString onlineName;

  element->GetInt32Property("boxFlags", &m_boxFlags);
  if (NS_SUCCEEDED(element->GetInt32Property("hierDelim", &hierarchyDelimiter))
      && hierarchyDelimiter != kOnlineHierarchySeparatorUnknown)
    m_hierarchyDelimiter = (PRUnichar) hierarchyDelimiter;
  rv = element->GetStringProperty("onlineName", onlineName);
  if (NS_SUCCEEDED(rv) && !onlineName.IsEmpty())
    m_onlineFolderName.Assign(onlineName);

  m_aclFlags = -1; // init to invalid value.
  element->GetInt32Property("aclFlags", (PRInt32 *) &m_aclFlags);
  return rv;
}

NS_IMETHODIMP nsImapMailFolder::WriteToFolderCacheElem(nsIMsgFolderCacheElement *element)
{
  nsresult rv = nsMsgDBFolder::WriteToFolderCacheElem(element);
  element->SetInt32Property("boxFlags", m_boxFlags);
  element->SetInt32Property("hierDelim", (PRInt32) m_hierarchyDelimiter);
  element->SetStringProperty("onlineName", m_onlineFolderName);
  element->SetInt32Property("aclFlags", (PRInt32) m_aclFlags);
  return rv;
}

NS_IMETHODIMP
nsImapMailFolder::MarkMessagesFlagged(nsIArray *messages, PRBool markFlagged)
{
  nsresult rv;
  // tell the folder to do it, which will mark them read in the db.
  rv = nsMsgDBFolder::MarkMessagesFlagged(messages, markFlagged);
  if (NS_SUCCEEDED(rv))
  {
    nsCAutoString messageIds;
    nsTArray<nsMsgKey> keysToMarkFlagged;
    rv = BuildIdsAndKeyArray(messages, messageIds, keysToMarkFlagged);
    if (NS_FAILED(rv)) return rv;
    rv = StoreImapFlags(kImapMsgFlaggedFlag, markFlagged,  keysToMarkFlagged.Elements(),
                        keysToMarkFlagged.Length(), nsnull);
    mDatabase->Commit(nsMsgDBCommitType::kLargeCommit);
  }
  return rv;
}

NS_IMETHODIMP nsImapMailFolder::SetOnlineName(const nsACString& aOnlineFolderName)
{
  nsresult rv;
  nsCOMPtr<nsIMsgDatabase> db;
  nsCOMPtr<nsIDBFolderInfo> folderInfo;
  rv = GetDBFolderInfoAndDB(getter_AddRefs(folderInfo), getter_AddRefs(db));
  // do this after GetDBFolderInfoAndDB, because it crunches m_onlineFolderName (not sure why)
  m_onlineFolderName = aOnlineFolderName;
  if(NS_SUCCEEDED(rv) && folderInfo)
  {
    nsAutoString onlineName;
    CopyASCIItoUTF16(aOnlineFolderName, onlineName);
    rv = folderInfo->SetProperty("onlineName", onlineName);
    rv = folderInfo->SetMailboxName(onlineName);
    // so, when are we going to commit this? Definitely not every time!
    // We could check if the online name has changed.
    db->Commit(nsMsgDBCommitType::kLargeCommit);
  }
  folderInfo = nsnull;
  return rv;
}

NS_IMETHODIMP nsImapMailFolder::GetOnlineName(nsACString& aOnlineFolderName)
{
  ReadDBFolderInfo(PR_FALSE); // update cache first.
  aOnlineFolderName = m_onlineFolderName;
  return NS_OK;
}

NS_IMETHODIMP
nsImapMailFolder::GetDBFolderInfoAndDB(nsIDBFolderInfo **folderInfo, nsIMsgDatabase **db)
{
  NS_ENSURE_ARG_POINTER (folderInfo);
  NS_ENSURE_ARG_POINTER (db);

  nsresult rv = GetDatabase(nsnull);
  if (NS_FAILED(rv))
    return rv;

  NS_ADDREF(*db = mDatabase);

  rv = (*db)->GetDBFolderInfo(folderInfo);
  if (NS_FAILED(rv))
    return rv; //GetDBFolderInfo can't return NS_OK if !folderInfo

  nsCString onlineName;
  rv = (*folderInfo)->GetCharProperty("onlineName", onlineName);
  if (NS_FAILED(rv))
    return rv;

  if (!onlineName.IsEmpty())
    m_onlineFolderName.Assign(onlineName);
  else
  {
    nsAutoString autoOnlineName;
    (*folderInfo)->GetMailboxName(autoOnlineName);
    if (autoOnlineName.IsEmpty())
    {
      nsCString uri;
      rv = GetURI(uri);
      NS_ENSURE_SUCCESS(rv, rv);

      nsCString hostname;
      rv = GetHostname(hostname);
      NS_ENSURE_SUCCESS(rv, rv);

      nsCString onlineCName;
      rv = nsImapURI2FullName(kImapRootURI, hostname.get(), uri.get(), getter_Copies(onlineCName));
      if (m_hierarchyDelimiter != '/')
        onlineCName.ReplaceChar('/',  char(m_hierarchyDelimiter));
      m_onlineFolderName.Assign(onlineCName);
      CopyASCIItoUTF16(onlineCName, autoOnlineName);
    }
    (*folderInfo)->SetProperty("onlineName", autoOnlineName);
  }
  return rv;
}

nsresult
nsImapMailFolder::BuildIdsAndKeyArray(nsIArray* messages,
                                      nsCString& msgIds,
                                      nsTArray<nsMsgKey>& keyArray)
{
  NS_ENSURE_ARG_POINTER(messages);
  nsresult rv;
  PRUint32 count = 0;
  PRUint32 i;
  rv = messages->GetLength(&count);
  if (NS_FAILED(rv)) return rv;

  // build up message keys.
  for (i = 0; i < count; i++)
  {
    nsMsgKey key;
    nsCOMPtr <nsIMsgDBHdr> msgDBHdr = do_QueryElementAt(messages, i, &rv);
    if (msgDBHdr)
      rv = msgDBHdr->GetMessageKey(&key);
    if (NS_SUCCEEDED(rv))
      keyArray.AppendElement(key);
  }
  return AllocateUidStringFromKeys(keyArray.Elements(), keyArray.Length(), msgIds);
}

static int PR_CALLBACK CompareKey (const void *v1, const void *v2, void *)
{
  // QuickSort callback to compare array values
  nsMsgKey i1 = *(nsMsgKey *)v1;
  nsMsgKey i2 = *(nsMsgKey *)v2;
  return i1 - i2;
}

/* static */nsresult
nsImapMailFolder::AllocateUidStringFromKeys(nsMsgKey *keys, PRUint32 numKeys, nsCString &msgIds)
{
  if (!numKeys)
    return NS_ERROR_INVALID_ARG;
  nsresult rv = NS_OK;
  PRUint32 startSequence;
  startSequence = keys[0];
  PRUint32 curSequenceEnd = startSequence;
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
      AppendUid(msgIds, startSequence);
      msgIds += ':';
      AppendUid(msgIds,curSequenceEnd);
      if (!lastKey)
        msgIds += ',';
      startSequence = nextKey;
      curSequenceEnd = startSequence;
    }
    else
    {
      startSequence = nextKey;
      curSequenceEnd = startSequence;
      AppendUid(msgIds, keys[keyIndex]);
      if (!lastKey)
        msgIds += ',';
    }
  }
  return rv;
}

nsresult nsImapMailFolder::MarkMessagesImapDeleted(nsTArray<nsMsgKey> *keyArray, PRBool deleted, nsIMsgDatabase *db)
{
  for (PRUint32 kindex = 0; kindex < keyArray->Length(); kindex++)
  {
    nsMsgKey key = keyArray->ElementAt(kindex);
    db->MarkImapDeleted(key, deleted, nsnull);
  }
  return NS_OK;
}

NS_IMETHODIMP nsImapMailFolder::DeleteMessages(nsIArray *messages,
                                               nsIMsgWindow *msgWindow,
                                               PRBool deleteStorage, PRBool isMove,
                                               nsIMsgCopyServiceListener* listener,
                                               PRBool allowUndo)
{
  // *** jt - assuming delete is move to the trash folder for now
  nsCOMPtr<nsIRDFResource> res;
  nsCAutoString uri;
  PRBool deleteImmediatelyNoTrash = PR_FALSE;
  nsCAutoString messageIds;
  nsTArray<nsMsgKey> srcKeyArray;
  PRBool deleteMsgs = PR_TRUE;  //used for toggling delete status - default is true
  nsMsgImapDeleteModel deleteModel = nsMsgImapDeleteModels::MoveToTrash;
  imapMessageFlagsType messageFlags = kImapMsgDeletedFlag;

  nsCOMPtr<nsIImapIncomingServer> imapServer;
  nsresult rv = GetFlag(nsMsgFolderFlags::Trash, &deleteImmediatelyNoTrash);
  rv = GetImapIncomingServer(getter_AddRefs(imapServer));

  if (NS_SUCCEEDED(rv) && imapServer)
  {
    imapServer->GetDeleteModel(&deleteModel);
    if (deleteModel != nsMsgImapDeleteModels::MoveToTrash || deleteStorage)
      deleteImmediatelyNoTrash = PR_TRUE;
    // if we're deleting a message, we should pseudo-interrupt the msg
    //load of the current message.
    PRBool interrupted = PR_FALSE;
    imapServer->PseudoInterruptMsgLoad(this, msgWindow, &interrupted);
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
      rootFolder->GetFolderWithFlags(nsMsgFolderFlags::Trash,
                                     getter_AddRefs(trashFolder));
      NS_ASSERTION(trashFolder != 0, "couldn't find trash");
      // if we can't find the trash, we'll just have to do an imap delete and pretend this is the trash
      if (!trashFolder)
        deleteImmediatelyNoTrash = PR_TRUE;
    }
  }

  if ((NS_SUCCEEDED(rv) && deleteImmediatelyNoTrash) || deleteModel == nsMsgImapDeleteModels::IMAPDelete )
  {
    if (allowUndo)
    {
      //need to take care of these two delete models
      nsImapMoveCopyMsgTxn* undoMsgTxn = new nsImapMoveCopyMsgTxn;
      if (!undoMsgTxn || NS_FAILED(undoMsgTxn->Init(this, &srcKeyArray, messageIds.get(), nsnull,
                                                      PR_TRUE, isMove, m_thread, nsnull)))
      {
        delete undoMsgTxn;
        return NS_ERROR_OUT_OF_MEMORY;
      }
      undoMsgTxn->SetTransactionType(nsIMessenger::eDeleteMsg);
      // we're adding this undo action before the delete is successful. This is evil,
      // but 4.5 did it as well.
      nsCOMPtr <nsITransactionManager> txnMgr;
      if (msgWindow)
        msgWindow->GetTransactionManager(getter_AddRefs(txnMgr));
      if (txnMgr)
        txnMgr->DoTransaction(undoMsgTxn);
    }

    if (deleteModel == nsMsgImapDeleteModels::IMAPDelete && !deleteStorage)
    {
      PRUint32 cnt, flags;
      rv = messages->GetLength(&cnt);
      NS_ENSURE_SUCCESS(rv, rv);
      deleteMsgs = PR_FALSE;
      for (PRUint32 i=0; i <cnt; i++)
      {
        nsCOMPtr <nsIMsgDBHdr> msgHdr = do_QueryElementAt(messages, i);
        if (msgHdr)
        {
          msgHdr->GetFlags(&flags);
          if (!(flags & MSG_FLAG_IMAP_DELETED))
          {
            deleteMsgs = PR_TRUE;
            break;
          }
        }
      }
    }
    // if copy service listener is also a url listener, pass that
    // url listener into StoreImapFlags.
    nsCOMPtr <nsIUrlListener> urlListener = do_QueryInterface(listener);
    if (deleteMsgs)
      messageFlags |= kImapMsgSeenFlag;
    rv = StoreImapFlags(messageFlags, deleteMsgs, srcKeyArray.Elements(),
                        srcKeyArray.Length(), urlListener);

    if (NS_SUCCEEDED(rv))
    {
      if (mDatabase)
      {
        if (deleteModel == nsMsgImapDeleteModels::IMAPDelete)
          MarkMessagesImapDeleted(&srcKeyArray, deleteMsgs, mDatabase);
        else
        {
          EnableNotifications(allMessageCountNotifications, PR_FALSE, PR_TRUE /*dbBatching*/);  //"remove it immediately" model
          // Notify if this is an actual delete.
          if (!isMove)
          {
            nsCOMPtr<nsIMsgFolderNotificationService> notifier(do_GetService(NS_MSGNOTIFICATIONSERVICE_CONTRACTID));
            if (notifier)
              notifier->NotifyMsgsDeleted(messages);
          }
          mDatabase->DeleteMessages(&srcKeyArray,nsnull);
          EnableNotifications(allMessageCountNotifications, PR_TRUE, PR_TRUE /*dbBatching*/);
          NotifyFolderEvent(mDeleteOrMoveMsgCompletedAtom);
        }
      }
    }
    return rv;
  }
  else  // have to move the messages to the trash
  {
    if(trashFolder)
    {
      nsCOMPtr<nsIMsgFolder> srcFolder;
      nsCOMPtr<nsISupports>srcSupport;
      PRUint32 count = 0;
      rv = messages->GetLength(&count);

      rv = QueryInterface(NS_GET_IID(nsIMsgFolder), getter_AddRefs(srcFolder));
      nsCOMPtr<nsIMsgCopyService> copyService = do_GetService(NS_MSGCOPYSERVICE_CONTRACTID, &rv);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = copyService->CopyMessages(srcFolder, messages, trashFolder, PR_TRUE, listener, msgWindow, allowUndo);
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
  NS_ENSURE_TRUE(folder, PR_FALSE);
  nsCOMPtr<nsIMsgFolder> parent;
  nsCOMPtr<nsIMsgFolder> curFolder = folder;
  nsresult rv;
  PRUint32 flags = 0;
  do
  {
    rv = curFolder->GetFlags(&flags);
    if (NS_FAILED(rv)) return PR_FALSE;
    if (flags & nsMsgFolderFlags::Trash)
      return PR_TRUE;
    curFolder->GetParentMsgFolder(getter_AddRefs(parent));
    if (!parent) return PR_FALSE;
    curFolder = parent;
  } while (NS_SUCCEEDED(rv) && curFolder);
  return PR_FALSE;
}
NS_IMETHODIMP
nsImapMailFolder::DeleteSubFolders(nsIArray* folders, nsIMsgWindow *msgWindow)
{
  nsCOMPtr<nsIMsgFolder> curFolder;
  nsCOMPtr<nsIUrlListener> urlListener;
  nsCOMPtr<nsIMsgFolder> trashFolder;
  PRInt32 i;
  PRUint32 folderCount = 0;
  nsresult rv;
  // "this" is the folder we're deleting from
  PRBool deleteNoTrash = TrashOrDescendentOfTrash(this) || !DeleteIsMoveToTrash();
  PRBool confirmed = PR_FALSE;
  PRBool confirmDeletion = PR_TRUE;

  nsCOMPtr<nsIMutableArray> foldersRemaining(do_CreateInstance(NS_ARRAY_CONTRACTID));
  folders->GetLength(&folderCount);

  for (i = folderCount - 1; i >= 0; i--)
  {
    curFolder = do_QueryElementAt(folders, i, &rv);
    if (NS_SUCCEEDED(rv))
    {
      PRUint32 folderFlags;
      curFolder->GetFlags(&folderFlags);
      if (folderFlags & nsMsgFolderFlags::Virtual)
      {
        RemoveSubFolder(curFolder);
        // since the folder pane only allows single selection, we can do this
        deleteNoTrash = confirmed = PR_TRUE;
        confirmDeletion = PR_FALSE;
      }
      else
        foldersRemaining->InsertElementAt(curFolder, 0, PR_FALSE);
    }
  }

  foldersRemaining->GetLength(&folderCount);

  nsCOMPtr<nsIImapService> imapService = do_GetService(NS_IMAPSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!deleteNoTrash)
  {
    rv = GetTrashFolder(getter_AddRefs(trashFolder));
    //If we can't find the trash folder and we are supposed to move it to the trash
    //return failure.
    if(NS_FAILED(rv) || !trashFolder)
      return NS_ERROR_FAILURE;
    PRBool canHaveSubFoldersOfTrash = PR_TRUE;
    trashFolder->GetCanCreateSubfolders(&canHaveSubFoldersOfTrash);
    if (canHaveSubFoldersOfTrash) // UW server doesn't set NOINFERIORS - check dual use pref
    {
      nsCOMPtr<nsIImapIncomingServer> imapServer;
      rv = GetImapIncomingServer(getter_AddRefs(imapServer));
      NS_ENSURE_SUCCESS(rv, rv);
      PRBool serverSupportsDualUseFolders;
      imapServer->GetDualUseFolders(&serverSupportsDualUseFolders);
      if (!serverSupportsDualUseFolders)
        canHaveSubFoldersOfTrash = PR_FALSE;
    }
    if (!canHaveSubFoldersOfTrash)
      deleteNoTrash = PR_TRUE;
    nsCOMPtr<nsIPrefBranch> prefBranch(do_GetService(NS_PREFSERVICE_CONTRACTID, &rv));
    NS_ENSURE_SUCCESS(rv, rv);
    prefBranch->GetBoolPref("mailnews.confirm.moveFoldersToTrash", &confirmDeletion);
  }
  if (!confirmed && (confirmDeletion || deleteNoTrash)) //let us alert the user if we are deleting folder immediately
  {
    nsString confirmationStr;
    IMAPGetStringByID(((!deleteNoTrash) ? IMAP_MOVE_FOLDER_TO_TRASH : IMAP_DELETE_NO_TRASH),
    getter_Copies(confirmationStr));
    if (!msgWindow)
      return NS_ERROR_NULL_POINTER;
    nsCOMPtr<nsIDocShell> docShell;
    msgWindow->GetRootDocShell(getter_AddRefs(docShell));
    nsCOMPtr<nsIPrompt> dialog;
    if (docShell)
      dialog = do_GetInterface(docShell);
    if (dialog && !confirmationStr.IsEmpty())
      dialog->Confirm(nsnull, confirmationStr.get(), &confirmed);
  }
  else
    confirmed = PR_TRUE;

  if (confirmed)
  {
    for (i = 0; i < folderCount; i++)
    {
      curFolder = do_QueryElementAt(foldersRemaining, i, &rv);
      if (NS_SUCCEEDED(rv))
      {
        urlListener = do_QueryInterface(curFolder);
        if (deleteNoTrash)
          rv = imapService->DeleteFolder(m_thread,
                                         curFolder,
                                         urlListener,
                                         msgWindow,
                                         nsnull);
        else
        {
          PRBool confirm = PR_FALSE;
          PRBool match = PR_FALSE;
          rv = curFolder->MatchOrChangeFilterDestination(nsnull, PR_FALSE, &match);
          if (match)
          {
            curFolder->ConfirmFolderDeletionForFilter(msgWindow, &confirm);
            if (!confirm)
              return NS_OK;
          }
          rv = imapService->MoveFolder(m_thread,
                                       curFolder,
                                       trashFolder,
                                       urlListener,
                                       msgWindow,
                                       nsnull);
        }
      }
    }
  }
  //delete subfolders only if you are  deleting things from trash
  return confirmed && deleteNoTrash ? nsMsgDBFolder::DeleteSubFolders(foldersRemaining, msgWindow) : rv;
}

// Called by Biff, or when user presses GetMsg button.
NS_IMETHODIMP nsImapMailFolder::GetNewMessages(nsIMsgWindow *aWindow, nsIUrlListener *aListener)
{
  nsCOMPtr<nsIMsgFolder> rootFolder;
  nsresult rv = GetRootFolder(getter_AddRefs(rootFolder));
  if(NS_SUCCEEDED(rv) && rootFolder)
  {
    nsCOMPtr<nsIImapIncomingServer> imapServer;
    rv = GetImapIncomingServer(getter_AddRefs(imapServer));
    NS_ENSURE_SUCCESS(rv, rv);
    PRBool performingBiff = PR_FALSE;
    nsCOMPtr<nsIMsgIncomingServer> incomingServer = do_QueryInterface(imapServer, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    incomingServer->GetPerformingBiff(&performingBiff);

    // Check preferences to see if we should check all folders for new
    // messages, or just the inbox and marked ones
    PRBool checkAllFolders = PR_FALSE;
    nsCOMPtr<nsIPrefBranch> prefBranch = do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    // This pref might not exist, which is OK. We'll only check inbox and marked ones
    rv = prefBranch->GetBoolPref("mail.check_all_imap_folders_for_new", &checkAllFolders);
    m_urlListener = aListener;

    // Get new messages for inbox
    nsCOMPtr<nsIMsgFolder> inbox;
    rv = rootFolder->GetFolderWithFlags(nsMsgFolderFlags::Inbox,
                                        getter_AddRefs(inbox));
    if (inbox)
    {
      nsCOMPtr<nsIMsgImapMailFolder> imapFolder = do_QueryInterface(inbox, &rv);
      NS_ENSURE_SUCCESS(rv, rv);
      imapFolder->SetPerformingBiff(performingBiff);
      inbox->SetGettingNewMessages(PR_TRUE);
      rv = inbox->UpdateFolder(aWindow);
    }
    // Get new messages for other folders if marked, or all of them if the pref is set
    rv = imapServer->GetNewMessagesForNonInboxFolders(rootFolder, aWindow, checkAllFolders, performingBiff);
  }
  return rv;
}

NS_IMETHODIMP nsImapMailFolder::Shutdown(PRBool shutdownChildren)
{
  m_filterList = nsnull;
  m_initialized = PR_FALSE;
  // mPath is used to decide if folder pathname needs to be reconstructed in GetPath().
  mPath = nsnull;
  NS_IF_RELEASE(m_moveCoalescer);
  return nsMsgDBFolder::Shutdown(shutdownChildren);
}

nsresult nsImapMailFolder::GetBodysToDownload(nsTArray<nsMsgKey> *keysOfMessagesToDownload)
{
  NS_ENSURE_ARG(keysOfMessagesToDownload);
  NS_ENSURE_TRUE(mDatabase, NS_ERROR_FAILURE);

  nsCOMPtr <nsISimpleEnumerator> enumerator;
  nsresult rv = mDatabase->EnumerateMessages(getter_AddRefs(enumerator));
  if (NS_SUCCEEDED(rv) && enumerator)
  {
    PRBool hasMore;
    while (NS_SUCCEEDED(rv = enumerator->HasMoreElements(&hasMore)) && hasMore)
    {
      nsCOMPtr <nsIMsgDBHdr> pHeader;
      rv = enumerator->GetNext(getter_AddRefs(pHeader));
      NS_ENSURE_SUCCESS(rv, rv);
      PRBool shouldStoreMsgOffline = PR_FALSE;
      nsMsgKey msgKey;
      pHeader->GetMessageKey(&msgKey);
      // MsgFitsDownloadCriteria ignores nsMsgFolderFlags::Offline, which we want
      if (m_downloadingFolderForOfflineUse)
        MsgFitsDownloadCriteria(msgKey, &shouldStoreMsgOffline);
      else
        ShouldStoreMsgOffline(msgKey, &shouldStoreMsgOffline);
      if (shouldStoreMsgOffline)
        keysOfMessagesToDownload->AppendElement(msgKey);
    }
  }
  return rv;
}

NS_IMETHODIMP nsImapMailFolder::OnNewIdleMessages()
{
  PRBool checkAllFolders = PR_FALSE;

  nsresult rv;
  nsCOMPtr<nsIPrefBranch> prefBranch = do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // This pref might not exist, which is OK.
  (void) prefBranch->GetBoolPref("mail.check_all_imap_folders_for_new", &checkAllFolders);

  // only trigger biff if we're checking all new folders for new messages, or this particular folder,
  // but excluding trash,junk, sent, and no select folders, by default.
  if ((checkAllFolders &&
    !(mFlags & (nsMsgFolderFlags::Trash | nsMsgFolderFlags::Junk | nsMsgFolderFlags::SentMail | nsMsgFolderFlags::ImapNoselect)))
    || (mFlags & (nsMsgFolderFlags::CheckNew|nsMsgFolderFlags::Inbox)))
    SetPerformingBiff(PR_TRUE);
  return UpdateFolder(nsnull);
}

NS_IMETHODIMP nsImapMailFolder::UpdateImapMailboxInfo(nsIImapProtocol* aProtocol, nsIMailboxSpec* aSpec)
{
  nsresult rv;
  ChangeNumPendingTotalMessages(-GetNumPendingTotalMessages());
  ChangeNumPendingUnread(-GetNumPendingUnread());
  m_numStatusRecentMessages = 0; // clear this since we selected the folder.
  m_numStatusUnseenMessages = 0; // clear this since we selected the folder.

  if (!mDatabase)
    GetDatabase(nsnull);

  PRBool folderSelected;
  rv = aSpec->GetFolderSelected(&folderSelected);
  NS_ENSURE_SUCCESS(rv, rv);
  nsTArray<nsMsgKey> existingKeys;
  nsTArray<nsMsgKey> keysToDelete;
  nsTArray<nsMsgKey> keysToFetch;
  PRUint32 numNewUnread;
  nsCOMPtr<nsIDBFolderInfo> dbFolderInfo;
  PRInt32 imapUIDValidity = 0;

  if (mDatabase)
  {
    rv = mDatabase->GetDBFolderInfo(getter_AddRefs(dbFolderInfo));
    if (NS_SUCCEEDED(rv) && dbFolderInfo)
      dbFolderInfo->GetImapUidValidity(&imapUIDValidity);
    mDatabase->ListAllKeys(existingKeys);
    PRInt32 keyCount = existingKeys.Length();
    mDatabase->ListAllOfflineDeletes(&existingKeys);
    if (keyCount < existingKeys.Length())
      existingKeys.Sort();
  }
  PRInt32 folderValidity;
  aSpec->GetFolder_UIDVALIDITY(&folderValidity);
  nsCOMPtr <nsIImapFlagAndUidState> flagState;
  aSpec->GetFlagState(getter_AddRefs(flagState));

  // remember what the supported user flags are.
  PRUint32 supportedUserFlags;
  aSpec->GetSupportedUserFlags(&supportedUserFlags);
  SetSupportedUserFlags(supportedUserFlags);

  m_uidValidity = folderValidity;

  if ((imapUIDValidity != folderValidity) /* && // if UIDVALIDITY Changed
    !NET_IsOffline() */)
  {
    NS_ASSERTION(PR_FALSE, "uid validity seems to have changed, blowing away db");
    nsCOMPtr<nsILocalFile> pathFile;
    rv = GetFilePath(getter_AddRefs(pathFile));
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIMsgDBService> msgDBService = do_GetService(NS_MSGDB_SERVICE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr <nsIDBFolderInfo> transferInfo;
    if (dbFolderInfo)
      dbFolderInfo->GetTransferInfo(getter_AddRefs(transferInfo));
    if (mDatabase)
    {
      dbFolderInfo = nsnull;
      mDatabase->ForceClosed();
    }
    mDatabase = nsnull;

    nsCOMPtr <nsILocalFile> summaryFile;
    GetSummaryFileLocation(pathFile, getter_AddRefs(summaryFile));
    // Remove summary file.
    summaryFile->Remove(PR_FALSE);

    // Create a new summary file, update the folder message counts, and
    // Close the summary file db.
    rv = msgDBService->OpenFolderDB(this, PR_TRUE, PR_TRUE, getter_AddRefs(mDatabase));

    // ********** Important *************
    // David, help me here I don't know this is right or wrong
    if (rv == NS_MSG_ERROR_FOLDER_SUMMARY_MISSING)
      rv = NS_OK;

    if (NS_FAILED(rv) && mDatabase)
    {
      mDatabase->ForceClosed();
      mDatabase = nsnull;
    }
    else if (NS_SUCCEEDED(rv) && mDatabase)
    {
      if (transferInfo)
        SetDBTransferInfo(transferInfo);

      SummaryChanged();
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
    existingKeys.Clear();
    //      keysToDelete.CopyArray(&existingKeys);

    if (flagState)
    {
      nsTArray<nsMsgKey> no_existingKeys;
      FindKeysToAdd(no_existingKeys, keysToFetch, numNewUnread, flagState);
    }
    if (NS_FAILED(rv))
      pathFile->Remove(PR_FALSE);

  }
  else if (!flagState /*&& !NET_IsOffline() */) // if there are no messages on the server
    keysToDelete = existingKeys;
  else /* if ( !NET_IsOffline()) */
  {
    FindKeysToDelete(existingKeys, keysToDelete, flagState);
    PRUint32 boxFlags;
    aSpec->GetBox_flags(&boxFlags);
    // if this is the result of an expunge then don't grab headers
    if (!(boxFlags & kJustExpunged))
      FindKeysToAdd(existingKeys, keysToFetch, numNewUnread, flagState);
  }
  if (!keysToDelete.IsEmpty() && mDatabase)
  {
    PRUint32 total;

    // Notify nsIMsgFolderListeners of a mass delete.
    nsCOMPtr<nsIMutableArray> hdrsToDelete(do_CreateInstance(NS_ARRAY_CONTRACTID));
    MsgGetHeadersFromKeys(mDatabase, keysToDelete, hdrsToDelete);
    nsCOMPtr<nsIMsgFolderNotificationService> notifier(do_GetService(NS_MSGNOTIFICATIONSERVICE_CONTRACTID));
    if (notifier)
      notifier->NotifyMsgsDeleted(hdrsToDelete);

    // It would be nice to notify RDF or whoever of a mass delete here.
    mDatabase->DeleteMessages(&keysToDelete, nsnull);
    total = keysToDelete.Length();
  }
  // If we are performing biff for this folder, tell the
  // stand-alone biff about the new high water mark
  if (m_performingBiff && numNewUnread)
  {
    // We must ensure that the server knows that we are performing biff.
    // Otherwise the stand-alone biff won't fire.
    nsCOMPtr<nsIMsgIncomingServer> server;
    if (NS_SUCCEEDED(GetServer(getter_AddRefs(server))) && server)
      server->SetPerformingBiff(PR_TRUE);
     SetNumNewMessages(numNewUnread);
  }
  SyncFlags(flagState);
  PRInt32 numUnreadFromServer;
  aSpec->GetNumUnseenMessages(&numUnreadFromServer);
  if (mDatabase && mNumUnreadMessages + keysToFetch.Length() > numUnreadFromServer)
    mDatabase->SyncCounts();

  if (!keysToFetch.IsEmpty())
    PrepareToAddHeadersToMailDB(aProtocol, keysToFetch, aSpec);
  else
  {
    // let the imap libnet module know that we don't need headers
    if (aProtocol)
      aProtocol->NotifyHdrsToDownload(nsnull, 0);
    PRBool gettingNewMessages;
    GetGettingNewMessages(&gettingNewMessages);
    if (gettingNewMessages)
      ProgressStatus(aProtocol, IMAP_NO_NEW_MESSAGES, nsnull);
    SetPerformingBiff(PR_FALSE);
  }
  return rv;
}

NS_IMETHODIMP nsImapMailFolder::UpdateImapMailboxStatus(
  nsIImapProtocol* aProtocol, nsIMailboxSpec* aSpec)
{
  NS_ENSURE_ARG_POINTER(aSpec);
  PRInt32 numRecent, numUnread;
  aSpec->GetNumRecentMessages(&numRecent);
  aSpec->GetNumUnseenMessages(&numUnread);
  // If m_numStatusUnseenMessages is 0, it means
  // this is the first time we've done a Status.
  // In that case, we count all the previous pending unread messages we know about
  // as unread messages.
  // We may want to do similar things with total messages, but the total messages
  // include deleted messages if the folder hasn't been expunged.
  PRInt32 previousUnreadMessages = (m_numStatusUnseenMessages)
    ? m_numStatusUnseenMessages : GetNumPendingUnread() + mNumUnreadMessages;
  if (numUnread != previousUnreadMessages)
  {
    // we're going to assume that recent messages are unread.
    ChangeNumPendingUnread(numUnread - previousUnreadMessages);
    ChangeNumPendingTotalMessages(numUnread - previousUnreadMessages);
    if (numUnread > previousUnreadMessages)
    {
      SetHasNewMessages(PR_TRUE);
      SetNumNewMessages(numUnread - previousUnreadMessages);
      SetBiffState(nsMsgBiffState_NewMail);
    }
    SummaryChanged();
  }
  SetPerformingBiff(PR_FALSE);
  m_numStatusUnseenMessages = numUnread;
  return NS_OK;
}

NS_IMETHODIMP nsImapMailFolder::ParseMsgHdrs(nsIImapProtocol *aProtocol, nsIImapHeaderXferInfo *aHdrXferInfo)
{
  PRInt32 numHdrs;
  nsCOMPtr <nsIImapHeaderInfo> headerInfo;
  nsCOMPtr <nsIImapUrl> aImapUrl;
  nsImapAction imapAction = nsIImapUrl::nsImapTest; // unused value.
  if (!mDatabase)
    GetDatabase(nsnull);

  nsresult rv = aHdrXferInfo->GetNumHeaders(&numHdrs);
  if (aProtocol)
  {
    (void) aProtocol->GetRunningImapURL(getter_AddRefs(aImapUrl));
    if (aImapUrl)
      aImapUrl->GetImapAction(&imapAction);
  }
  for (PRUint32 i = 0; NS_SUCCEEDED(rv) && i < numHdrs; i++)
  {
    rv = aHdrXferInfo->GetHeader(i, getter_AddRefs(headerInfo));
    NS_ENSURE_SUCCESS(rv, rv);
    if (!headerInfo)
      break;
    PRInt32 msgSize;
    nsMsgKey msgKey;
    PRBool containsKey;
    const char *msgHdrs;
    headerInfo->GetMsgSize(&msgSize);
    headerInfo->GetMsgUid(&msgKey);
    if (msgKey == nsMsgKey_None) // not a valid uid.
      continue;
    if (imapAction == nsIImapUrl::nsImapMsgPreview)
    {
      nsCOMPtr <nsIMsgDBHdr> msgHdr;
      headerInfo->GetMsgHdrs(&msgHdrs);
      // create an input stream based on the hdr string.
      nsCOMPtr<nsIStringInputStream> inputStream =
            do_CreateInstance("@mozilla.org/io/string-input-stream;1", &rv);
      NS_ENSURE_SUCCESS(rv, rv);
      inputStream->ShareData(msgHdrs, strlen(msgHdrs));
      GetMessageHeader(msgKey, getter_AddRefs(msgHdr));
      if (msgHdr)
        GetMsgPreviewTextFromStream(msgHdr, inputStream);
      continue;
    }
    if (mDatabase && NS_SUCCEEDED(mDatabase->ContainsKey(msgKey, &containsKey)) && containsKey)
    {
      NS_ASSERTION(PR_FALSE, "downloading hdrs for hdr we already have");
      continue;
    }
    nsresult rv = SetupHeaderParseStream(msgSize, EmptyCString(), nsnull);
    NS_ENSURE_SUCCESS(rv, rv);
    headerInfo->GetMsgHdrs(&msgHdrs);
    rv = ParseAdoptedHeaderLine(msgHdrs, msgKey);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = NormalEndHeaderParseStream(aProtocol, aImapUrl);
  }
  return rv;
}

nsresult nsImapMailFolder::SetupHeaderParseStream(PRUint32 aSize,
                                                  const nsACString& content_type, nsIMailboxSpec *boxSpec)
{
  if (!mDatabase)
    GetDatabase(nsnull);
  m_nextMessageByteLength = aSize;
  if (!m_msgParser)
  {
    nsresult rv;
    m_msgParser = do_CreateInstance(kParseMailMsgStateCID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else
    m_msgParser->Clear();

  m_msgParser->SetMailDB(mDatabase);
  return m_msgParser->SetState(nsIMsgParseMailMsgState::ParseHeadersState);
}

nsresult nsImapMailFolder::ParseAdoptedHeaderLine(const char *aMessageLine, PRUint32 aMsgKey)
{
  // we can get blocks that contain more than one line,
  // but they never contain partial lines
  const char *str = aMessageLine;
  m_curMsgUid = aMsgKey;
  m_msgParser->SetEnvelopePos(m_curMsgUid);
  // m_envelope_pos, for local folders,
  // is the msg key. Setting this will set the msg key for the new header.

  PRInt32 len = strlen(str);
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

nsresult nsImapMailFolder::NormalEndHeaderParseStream(nsIImapProtocol *aProtocol, nsIImapUrl* imapUrl)
{
  nsCOMPtr<nsIMsgDBHdr> newMsgHdr;
  nsresult rv;
  NS_ENSURE_TRUE(m_msgParser, NS_ERROR_NULL_POINTER);

  nsMailboxParseState parseState;
  m_msgParser->GetState(&parseState);
  if (parseState == nsIMsgParseMailMsgState::ParseHeadersState)
    m_msgParser->ParseAFolderLine(CRLF, 2);
  rv = m_msgParser->GetNewMsgHdr(getter_AddRefs(newMsgHdr));
  NS_ENSURE_SUCCESS(rv, rv);

  char *headers;
  PRInt32 headersSize;

  nsCOMPtr <nsIMsgWindow> msgWindow;
  nsCOMPtr <nsIMsgMailNewsUrl> msgUrl;
  if (imapUrl)
  {
    msgUrl = do_QueryInterface(imapUrl, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    msgUrl->GetMsgWindow(getter_AddRefs(msgWindow));
  }

  nsCOMPtr<nsIMsgIncomingServer> server;
  rv = GetServer(getter_AddRefs(server));
  NS_ENSURE_SUCCESS(rv, rv);
  newMsgHdr->SetMessageKey(m_curMsgUid);
  TweakHeaderFlags(aProtocol, newMsgHdr);
  PRUint32 messageSize;
  if (NS_SUCCEEDED(newMsgHdr->GetMessageSize(&messageSize)))
    mFolderSize += messageSize;
  m_msgMovedByFilter = PR_FALSE;
  // If this is the inbox, try to apply filters.
  if (mFlags & nsMsgFolderFlags::Inbox)
  {
    PRUint32 msgFlags;
    newMsgHdr->GetFlags(&msgFlags);
    if (!(msgFlags & (MSG_FLAG_READ | MSG_FLAG_IMAP_DELETED))) // only fire on unread msgs that haven't been deleted
    {
      PRInt32 duplicateAction = nsIMsgIncomingServer::keepDups;
      if (server)
        server->GetIncomingDuplicateAction(&duplicateAction);
      if (duplicateAction != nsIMsgIncomingServer::keepDups)
      {
        PRBool isDup;
        server->IsNewHdrDuplicate(newMsgHdr, &isDup);
        if (isDup)
        {
          // we want to do something similar to applying filter hits.
          // if a dup is marked read, it shouldn't trigger biff.
          // Same for deleting it or moving it to trash.
          switch (duplicateAction)
          {
            case nsIMsgIncomingServer::deleteDups:
              {
                PRUint32 newFlags;
                newMsgHdr->OrFlags(MSG_FLAG_READ | MSG_FLAG_IMAP_DELETED, &newFlags);
                StoreImapFlags(kImapMsgSeenFlag | kImapMsgDeletedFlag, PR_TRUE,
                               &m_curMsgUid, 1, nsnull);
                m_msgMovedByFilter = PR_TRUE;
              }
              break;
            case nsIMsgIncomingServer::moveDupsToTrash:
              {
                nsCOMPtr <nsIMsgFolder> trash;
                GetTrashFolder(getter_AddRefs(trash));
                if (trash)
                {
                  nsCString trashUri;
                  trash->GetURI(trashUri);
                  nsresult err = MoveIncorporatedMessage(newMsgHdr, mDatabase, trashUri, nsnull, msgWindow);
                  if (NS_SUCCEEDED(err))
                    m_msgMovedByFilter = PR_TRUE;
                }
              }
              break;
            case nsIMsgIncomingServer::markDupsRead:
              {
                PRUint32 newFlags;
                newMsgHdr->OrFlags(MSG_FLAG_READ, &newFlags);
                StoreImapFlags(kImapMsgSeenFlag, PR_TRUE, &m_curMsgUid, 1, nsnull);
              }
              break;
          }
          PRInt32 numNewMessages;
          GetNumNewMessages(PR_FALSE, &numNewMessages);
          SetNumNewMessages(numNewMessages - 1);
        }
      }
      rv = m_msgParser->GetAllHeaders(&headers, &headersSize);

      if (NS_SUCCEEDED(rv) && headers && !m_msgMovedByFilter)
      {
        if (m_filterList)
        {
          GetMoveCoalescer();  // not sure why we're doing this here.
          m_filterList->ApplyFiltersToHdr(nsMsgFilterType::InboxRule, newMsgHdr, this, mDatabase,
                                          headers, headersSize, this, msgWindow, nsnull);
        }
      }
    }
  }
  // here we need to tweak flags from uid state..
  if (mDatabase && (!m_msgMovedByFilter || ShowDeletedMessages()))
  {
    mDatabase->AddNewHdrToDB(newMsgHdr, PR_TRUE);
    nsCOMPtr<nsIMsgFolderNotificationService> notifier(do_GetService(NS_MSGNOTIFICATIONSERVICE_CONTRACTID));
    if (notifier)
      notifier->NotifyMsgAdded(newMsgHdr);
  }
  m_msgParser->Clear(); // clear out parser, because it holds onto a msg hdr.
  m_msgParser->SetMailDB(nsnull); // tell it to let go of the db too.
  // I don't think we want to do this - it does bad things like set the size incorrectly.
  //    m_msgParser->FinishHeader();
    return NS_OK;
}

NS_IMETHODIMP nsImapMailFolder::AbortHeaderParseStream(nsIImapProtocol* aProtocol)
{
  nsresult rv = NS_ERROR_FAILURE;
  return rv;
}

NS_IMETHODIMP nsImapMailFolder::BeginCopy(nsIMsgDBHdr *message)
{
  NS_ENSURE_TRUE(m_copyState, NS_ERROR_NULL_POINTER);
  nsresult rv;
  if (m_copyState->m_tmpFile) // leftover file spec nuke it
  {
    m_copyState->m_tmpFile->Remove(PR_FALSE);
    m_copyState->m_tmpFile = nsnull;
  }
  if (message)
    m_copyState->m_message = do_QueryInterface(message, &rv);

  nsCOMPtr<nsIFile> msgFile;
  rv = GetSpecialDirectoryWithFileName(NS_OS_TEMP_DIR,
                                       "nscpmsg.txt",
                                        getter_AddRefs(m_copyState->m_tmpFile));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIOutputStream> fileOutputStream;
  nsCOMPtr <nsILocalFile> localFile = do_QueryInterface(m_copyState->m_tmpFile);
  rv = NS_NewLocalFileOutputStream(getter_AddRefs(fileOutputStream), localFile, -1, 00600);
  NS_ENSURE_SUCCESS(rv,rv);
  rv = NS_NewBufferedOutputStream(getter_AddRefs(m_copyState->m_msgFileStream), fileOutputStream, FOUR_K);
  NS_ENSURE_SUCCESS(rv,rv);

  if (!m_copyState->m_dataBuffer)
    m_copyState->m_dataBuffer = (char*) PR_CALLOC(COPY_BUFFER_SIZE+1);
  NS_ENSURE_TRUE(m_copyState->m_dataBuffer, NS_ERROR_OUT_OF_MEMORY);
  m_copyState->m_dataBufferSize = COPY_BUFFER_SIZE;
  return NS_OK;
}

NS_IMETHODIMP nsImapMailFolder::CopyDataToOutputStreamForAppend(nsIInputStream *aIStream,
                     PRInt32 aLength, nsIOutputStream *outputStream)
{
  PRUint32 readCount;
  PRUint32 writeCount;
  if (!m_copyState)
  {
    nsImapMailCopyState* copyState = new nsImapMailCopyState();
    m_copyState = do_QueryInterface(copyState);
  }
  if ( aLength + m_copyState->m_leftOver > m_copyState->m_dataBufferSize )
  {
    m_copyState->m_dataBuffer = (char *) PR_REALLOC(m_copyState->m_dataBuffer, aLength + m_copyState->m_leftOver+ 1);
    NS_ENSURE_TRUE(m_copyState->m_dataBuffer, NS_ERROR_OUT_OF_MEMORY);
    m_copyState->m_dataBufferSize = aLength + m_copyState->m_leftOver;
  }

  char *start, *end;
  PRUint32 linebreak_len = 1;

  nsresult rv = aIStream->Read(m_copyState->m_dataBuffer+m_copyState->m_leftOver, aLength, &readCount);
  if (NS_FAILED(rv))
    return rv;

  m_copyState->m_leftOver += readCount;
  m_copyState->m_dataBuffer[m_copyState->m_leftOver] = '\0';

  start = m_copyState->m_dataBuffer;
  if (m_copyState->m_eatLF)
  {
    if (*start == '\n')
      start++;
    m_copyState->m_eatLF = PR_FALSE;
  }
  end = PL_strpbrk(start, "\r\n");
  if (end && *end == '\r' && *(end+1) == '\n')
    linebreak_len = 2;

  while (start && end)
  {
    if (PL_strncasecmp(start, "X-Mozilla-Status:", 17) &&
        PL_strncasecmp(start, "X-Mozilla-Status2:", 18) &&
        PL_strncmp(start, "From - ", 7))
    {
      rv = outputStream->Write(start,
                                             end-start,
                                             &writeCount);
      rv = outputStream->Write(CRLF, 2, &writeCount);
    }
    start = end+linebreak_len;
    if (start >=
        m_copyState->m_dataBuffer+m_copyState->m_leftOver)
    {
       m_copyState->m_leftOver = 0;
       break;
    }
    linebreak_len = 1;

    end = PL_strpbrk(start, "\r\n");
    if (end && *end == '\r')
    {
      if (*(end+1) == '\n')
        linebreak_len = 2;
      else if (! *(end+1)) // block might have split CRLF so remember if
        m_copyState->m_eatLF = PR_TRUE; // we should eat LF
    }

    if (start && !end)
    {
      m_copyState->m_leftOver -= (start - m_copyState->m_dataBuffer);
      memcpy(m_copyState->m_dataBuffer, start, m_copyState->m_leftOver+1); // including null
    }
  }
  return rv;
}

NS_IMETHODIMP nsImapMailFolder::CopyDataDone()
{
  m_copyState = nsnull;
  return NS_OK;
}

// sICopyMessageListener methods, BeginCopy, CopyData, EndCopy, EndMove, StartMessage, EndMessage
NS_IMETHODIMP nsImapMailFolder::CopyData(nsIInputStream *aIStream, PRInt32 aLength)
{
  NS_ENSURE_TRUE(m_copyState && m_copyState->m_msgFileStream && m_copyState->m_dataBuffer, NS_ERROR_NULL_POINTER);
  return CopyDataToOutputStreamForAppend(aIStream, aLength, m_copyState->m_msgFileStream);
}

NS_IMETHODIMP nsImapMailFolder::EndCopy(PRBool copySucceeded)
{
  nsresult rv = copySucceeded ? NS_OK : NS_ERROR_FAILURE;
  if (copySucceeded && m_copyState && m_copyState->m_msgFileStream)
  {
    nsCOMPtr<nsIUrlListener> urlListener;
    m_copyState->m_msgFileStream->Close();
    nsCOMPtr<nsIImapService> imapService = do_GetService(NS_IMAPSERVICE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv,rv);

    rv = QueryInterface(NS_GET_IID(nsIUrlListener), getter_AddRefs(urlListener));
    nsCOMPtr<nsISupports> copySupport;
    if (m_copyState)
      copySupport = do_QueryInterface(m_copyState);
    rv = imapService->AppendMessageFromFile(m_thread,
                                            m_copyState->m_tmpFile,
                                            this, EmptyCString(), PR_TRUE,
                                            m_copyState->m_selectedState,
                                            urlListener, nsnull,
                                            copySupport,
                                            m_copyState->m_msgWindow);
  }
  return rv;
}

NS_IMETHODIMP nsImapMailFolder::EndMove(PRBool moveSucceeded)
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

NS_IMETHODIMP nsImapMailFolder::ApplyFilterHit(nsIMsgFilter *filter, nsIMsgWindow *msgWindow, PRBool *applyMore)
{
  NS_ENSURE_ARG_POINTER(applyMore);

  nsMsgRuleActionType actionType;
  nsCString actionTargetFolderUri;
  PRUint32  newFlags;
  nsresult rv = NS_OK;

  // look at action - currently handle move
#ifdef DEBUG_bienvenu
  printf("got a rule hit!\n");
#endif

  nsCOMPtr<nsIMsgDBHdr> msgHdr;
  if (m_msgParser)
    m_msgParser->GetNewMsgHdr(getter_AddRefs(msgHdr));
  if (!msgHdr)
    return NS_ERROR_NULL_POINTER; //fatal error, cannot apply filters

  PRBool deleteToTrash = DeleteIsMoveToTrash();
  nsCOMPtr<nsISupportsArray> filterActionList = do_CreateInstance(NS_SUPPORTSARRAY_CONTRACTID, &rv);
  NS_ENSURE_TRUE(filterActionList, rv);
  rv = filter->GetSortedActionList(filterActionList);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 numActions;
  rv = filterActionList->Count(&numActions);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool loggingEnabled = PR_FALSE;
  if (m_filterList && numActions)
    (void)m_filterList->GetLoggingEnabled(&loggingEnabled);

  PRBool msgIsNew = PR_TRUE;

  for (PRUint32 actionIndex = 0; actionIndex < numActions; actionIndex++)
  {
    nsCOMPtr<nsIMsgRuleAction> filterAction;
    filterActionList->QueryElementAt(actionIndex, NS_GET_IID(nsIMsgRuleAction), getter_AddRefs(filterAction));
    if (!filterAction)
      continue;
    if (NS_SUCCEEDED(filterAction->GetType(&actionType)))
    {
      if (actionType == nsMsgFilterAction::MoveToFolder ||
          actionType == nsMsgFilterAction::CopyToFolder)
      {
        filterAction->GetTargetFolderUri(actionTargetFolderUri);
        if (actionTargetFolderUri.IsEmpty())
        {
          NS_ASSERTION(PR_FALSE, "actionTargetFolderUri is empty");
          continue;
        }
      }

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
          if (deleteToTrash)
          {
            // set value to trash folder
            nsCOMPtr <nsIMsgFolder> mailTrash;
            rv = GetTrashFolder(getter_AddRefs(mailTrash));
            if (NS_SUCCEEDED(rv) && mailTrash)
              rv = mailTrash->GetURI(actionTargetFolderUri);
            // msgHdr->OrFlags(MSG_FLAG_READ, &newFlags);  // mark read in trash.
          }
          else  // (!deleteToTrash)
          {
            msgHdr->OrFlags(MSG_FLAG_READ | MSG_FLAG_IMAP_DELETED, &newFlags);
            StoreImapFlags(kImapMsgSeenFlag | kImapMsgDeletedFlag, PR_TRUE,
                           &msgKey, 1, nsnull);
            m_msgMovedByFilter = PR_TRUE; // this will prevent us from adding the header to the db.
          }
          msgIsNew = PR_FALSE;
        }
        // note that delete falls through to move.
        case nsMsgFilterAction::MoveToFolder:
        {
          // if moving to a different file, do it.
          nsCString uri;
          rv = GetURI(uri);

          if (!actionTargetFolderUri.Equals(uri))
          {
            msgHdr->GetFlags(&msgFlags);

            if (msgFlags & MSG_FLAG_MDN_REPORT_NEEDED && !isRead)
            {
               msgHdr->SetFlags(msgFlags & ~MSG_FLAG_MDN_REPORT_NEEDED);
               msgHdr->OrFlags(MSG_FLAG_MDN_REPORT_SENT, &newFlags);
            }
            nsresult err = MoveIncorporatedMessage(msgHdr, mDatabase, actionTargetFolderUri, filter, msgWindow);
            if (NS_SUCCEEDED(err))
              m_msgMovedByFilter = PR_TRUE;
          }
          // don't apply any more filters, even if it was a move to the same folder
          *applyMore = PR_FALSE; 
        }
        break;
        case nsMsgFilterAction::CopyToFolder:
        {
          nsCString uri;
          rv = GetURI(uri);

          if (!actionTargetFolderUri.Equals(uri))
          {
            // XXXshaver I'm not actually 100% what the right semantics are for
            // MDNs and copied messages, but I suspect deep down inside that
            // we probably want to suppress them only on the copies.
            msgHdr->GetFlags(&msgFlags);
            if (msgFlags & MSG_FLAG_MDN_REPORT_NEEDED && !isRead)
            {
               msgHdr->SetFlags(msgFlags & ~MSG_FLAG_MDN_REPORT_NEEDED);
               msgHdr->OrFlags(MSG_FLAG_MDN_REPORT_SENT, &newFlags);
            }

            nsCOMPtr<nsIMutableArray> messageArray(do_CreateInstance(NS_ARRAY_CONTRACTID, &rv));
            NS_ENSURE_TRUE(messageArray, rv);
            messageArray->AppendElement(msgHdr, PR_FALSE);

            nsCOMPtr<nsIMsgFolder> dstFolder;
            rv = GetExistingFolder(actionTargetFolderUri, getter_AddRefs(dstFolder));
            NS_ENSURE_SUCCESS(rv, rv);

            nsCOMPtr<nsIMsgCopyService> copyService =
              do_GetService(NS_MSGCOPYSERVICE_CONTRACTID, &rv);
            NS_ENSURE_SUCCESS(rv, rv);
            rv = copyService->CopyMessages(this, messageArray, dstFolder,
                                           PR_FALSE, nsnull, msgWindow, PR_FALSE);
            NS_ENSURE_SUCCESS(rv, rv);
          }
        }
        break;
        case nsMsgFilterAction::MarkRead:
        {
          mDatabase->MarkHdrRead(msgHdr, PR_TRUE, nsnull);
          StoreImapFlags(kImapMsgSeenFlag, PR_TRUE, &msgKey, 1, nsnull);
          msgIsNew = PR_FALSE;
        }
        break;
        case nsMsgFilterAction::MarkFlagged:
        {
          mDatabase->MarkHdrMarked(msgHdr, PR_TRUE, nsnull);
          StoreImapFlags(kImapMsgFlaggedFlag, PR_TRUE, &msgKey, 1, nsnull);
        }
        break;
        case nsMsgFilterAction::KillThread:
          msgHdr->SetUint32Property("ProtoThreadFlags", MSG_FLAG_IGNORED);
          break;
        case nsMsgFilterAction::KillSubthread:
          msgHdr->OrFlags(MSG_FLAG_IGNORED, &newFlags);
          break;
        case nsMsgFilterAction::WatchThread:
          msgHdr->OrFlags(MSG_FLAG_WATCHED, &newFlags);
        break;
        case nsMsgFilterAction::ChangePriority:
        {
          nsMsgPriorityValue filterPriority;
          filterAction->GetPriority(&filterPriority);
          msgHdr->SetPriority(filterPriority);
        }
        break;
        case nsMsgFilterAction::Label:
        {
          nsMsgLabelValue filterLabel;
          filterAction->GetLabel(&filterLabel);
          msgHdr->SetLabel(filterLabel);
          StoreImapFlags((filterLabel << 9), PR_TRUE, &msgKey, 1, nsnull);
        }
        break;
        case nsMsgFilterAction::AddTag:
        {
          nsCString keyword;
          filterAction->GetStrValue(keyword);
          nsCOMPtr<nsIMutableArray> messageArray(do_CreateInstance(NS_ARRAY_CONTRACTID, &rv));
          NS_ENSURE_TRUE(messageArray, rv);
          messageArray->AppendElement(msgHdr, PR_FALSE);
          AddKeywordsToMessages(messageArray, keyword);
          break;
        }
        case nsMsgFilterAction::JunkScore:
        {
          nsCAutoString junkScoreStr;
          PRInt32 junkScore;
          filterAction->GetJunkScore(&junkScore);
          junkScoreStr.AppendInt(junkScore);
          mDatabase->SetStringProperty(msgKey, "junkscore", junkScoreStr.get());
          mDatabase->SetStringProperty(msgKey, "junkscoreorigin", "filter");

          // If score is available, set up to store junk status on server.
          if (junkScore == nsIJunkMailPlugin::IS_SPAM_SCORE ||
              junkScore == nsIJunkMailPlugin::IS_HAM_SCORE)
          {
            nsTArray<nsMsgKey> *keysToClassify = m_moveCoalescer->GetKeyBucket(
                       (junkScore == nsIJunkMailPlugin::IS_SPAM_SCORE) ? 0 : 1);
            NS_ASSERTION(keysToClassify, "error getting key bucket");
            if (keysToClassify)
              keysToClassify->AppendElement(msgKey);
            if (junkScore == nsIJunkMailPlugin::IS_SPAM_SCORE)
              msgIsNew = PR_FALSE;
          }
        }
        break;
      case nsMsgFilterAction::Forward:
        {
          nsCString forwardTo;
          filterAction->GetStrValue(forwardTo);
          nsCOMPtr <nsIMsgIncomingServer> server;
          rv = GetServer(getter_AddRefs(server));
          NS_ENSURE_SUCCESS(rv, rv);
          if (!forwardTo.IsEmpty())
          {
            nsCOMPtr <nsIMsgComposeService> compService = do_GetService (NS_MSGCOMPOSESERVICE_CONTRACTID) ;
            if (compService)
              rv = compService->ForwardMessage(NS_ConvertASCIItoUTF16(forwardTo), msgHdr, msgWindow, server);
          }
        }
        break;

      case nsMsgFilterAction::Reply:
        {
          nsCString replyTemplateUri;
          filterAction->GetStrValue(replyTemplateUri);
          nsCOMPtr <nsIMsgIncomingServer> server;
          GetServer(getter_AddRefs(server));
          NS_ENSURE_SUCCESS(rv, rv);
          if (!replyTemplateUri.IsEmpty())
          {
            nsCOMPtr <nsIMsgComposeService> compService = do_GetService (NS_MSGCOMPOSESERVICE_CONTRACTID) ;
            if (compService)
              rv = compService->ReplyWithTemplate(msgHdr, replyTemplateUri.get(), msgWindow, server);
          }
        }
        break;

        case nsMsgFilterAction::StopExecution:
        {
          // don't apply any more filters
          *applyMore = PR_FALSE;
        }
        break;

        default:
          break;
      }
      if (loggingEnabled)
      {
        // only log if successful move, or non-move action
        if (m_msgMovedByFilter || (actionType != nsMsgFilterAction::MoveToFolder &&
             (actionType != nsMsgFilterAction::Delete || !deleteToTrash)))
          (void) filter->LogRuleHit(filterAction, msgHdr);
      }
    }
  }
  if (!msgIsNew)
  {
    PRInt32 numNewMessages;
    GetNumNewMessages(PR_FALSE, &numNewMessages);
    SetNumNewMessages(numNewMessages - 1);
  }
  return NS_OK;
}

NS_IMETHODIMP nsImapMailFolder::SetImapFlags(const char *uids, PRInt32 flags, nsIURI **url)
{
  nsresult rv;
  nsCOMPtr<nsIImapService> imapService = do_GetService(NS_IMAPSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv,rv);

  return imapService->SetMessageFlags(m_thread, this, this, url, nsCAutoString(uids), flags, PR_TRUE);
}

// "this" is the parent folder
NS_IMETHODIMP nsImapMailFolder::PlaybackOfflineFolderCreate(const nsAString& aFolderName, nsIMsgWindow *aWindow, nsIURI **url)
{
  nsresult rv;
  nsCOMPtr<nsIImapService> imapService = do_GetService(NS_IMAPSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv,rv);
  return imapService->CreateFolder(m_thread, this, aFolderName, this, url);
}

NS_IMETHODIMP nsImapMailFolder::ReplayOfflineMoveCopy(nsMsgKey *msgKeys, PRUint32 numKeys, PRBool isMove, nsIMsgFolder *aDstFolder,
                                                      nsIUrlListener *aUrlListener, nsIMsgWindow *aWindow)
{
  nsresult rv;
  nsCOMPtr<nsIImapService> imapService = do_GetService(NS_IMAPSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr <nsIURI> resultUrl;
  nsCAutoString uids;
  AllocateUidStringFromKeys(msgKeys, numKeys, uids);
  rv = imapService->OnlineMessageCopy(m_thread,
                       this,
                                      uids,
                       aDstFolder,
                       PR_TRUE,
                       isMove,
                       aUrlListener,
                       getter_AddRefs(resultUrl), nsnull, aWindow);
  if (resultUrl)
  {
    nsCOMPtr <nsIMsgMailNewsUrl> mailnewsUrl = do_QueryInterface(resultUrl, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    nsCOMPtr <nsIUrlListener> folderListener = do_QueryInterface(aDstFolder);
    if (folderListener)
      mailnewsUrl->RegisterListener(folderListener);
  }
  return rv;
}

NS_IMETHODIMP nsImapMailFolder::StoreImapFlags(PRInt32 flags, PRBool addFlags,
                                               nsMsgKey *keys, PRUint32 numKeys,
                                               nsIUrlListener *aUrlListener)
{
  nsresult rv;
  if (!WeAreOffline())
  {
    nsCOMPtr<nsIImapService> imapService = do_GetService(NS_IMAPSERVICE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    nsCAutoString msgIds;
    AllocateUidStringFromKeys(keys, numKeys, msgIds);
    if (addFlags)
      imapService->AddMessageFlags(m_thread, this, aUrlListener ? aUrlListener : this,
                                   nsnull, msgIds, flags, PR_TRUE);
    else
      imapService->SubtractMessageFlags(m_thread, this, aUrlListener ? aUrlListener : this,
                                        nsnull, msgIds, flags, PR_TRUE);
  }
  else
  {
    GetDatabase(nsnull);
    if (mDatabase)
    {
      PRUint32 total = numKeys;
      for (PRUint32 keyIndex = 0; keyIndex < total; keyIndex++)
      {
        nsCOMPtr <nsIMsgOfflineImapOperation> op;
        rv = mDatabase->GetOfflineOpForKey(keys[keyIndex], PR_TRUE, getter_AddRefs(op));
        SetFlag(nsMsgFolderFlags::OfflineEvents);
        if (NS_SUCCEEDED(rv) && op)
        {
          imapMessageFlagsType newFlags;
          op->GetNewFlags(&newFlags);
          op->SetFlagOperation(addFlags ? newFlags | flags : newFlags & ~flags);
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
  nsCOMPtr<nsIImapService> imapService = do_GetService(NS_IMAPSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv,rv);
  return imapService->LiteSelectFolder(m_thread, this, aUrlListener, nsnull);
}

nsresult nsImapMailFolder::GetFolderOwnerUserName(nsACString& userName)
{
  if ((mFlags & nsMsgFolderFlags::ImapPersonal) ||
    !(mFlags & (nsMsgFolderFlags::ImapPublic | nsMsgFolderFlags::ImapOtherUser)))
  {
    // this is one of our personal mail folders
    // return our username on this host
    nsCOMPtr<nsIMsgIncomingServer> server;
    nsresult rv = GetServer(getter_AddRefs(server));
    return NS_FAILED(rv) ? rv : server->GetUsername(userName);
  }

  // the only other type of owner is if it's in the other users' namespace
  if (!(mFlags & nsMsgFolderFlags::ImapOtherUser))
    return NS_OK;

  if (m_ownerUserName.IsEmpty())
  {
    nsCString onlineName;
    GetOnlineName(onlineName);
    m_ownerUserName = nsIMAPNamespaceList::GetFolderOwnerNameFromPath(GetNamespaceForFolder(), onlineName.get());
  }
  userName = m_ownerUserName;
  return NS_OK;
}

nsIMAPNamespace *nsImapMailFolder::GetNamespaceForFolder()
{
  if (!m_namespace)
  {
#ifdef DEBUG_bienvenu
    // Make sure this isn't causing us to open the database
    NS_ASSERTION(m_hierarchyDelimiter != kOnlineHierarchySeparatorUnknown, "haven't set hierarchy delimiter");
#endif
    nsCString serverKey;
    nsCString onlineName;
    GetServerKey(serverKey);
    GetOnlineName(onlineName);
    PRUnichar hierarchyDelimiter;
    GetHierarchyDelimiter(&hierarchyDelimiter);

    m_namespace = nsIMAPNamespaceList::GetNamespaceForFolder(serverKey.get(), onlineName.get(), (char) hierarchyDelimiter);
    NS_ASSERTION(m_namespace, "didn't get namespace for folder");
    if (m_namespace)
    {
      nsIMAPNamespaceList::SuggestHierarchySeparatorForNamespace(m_namespace, (char) hierarchyDelimiter);
      m_folderIsNamespace = nsIMAPNamespaceList::GetFolderIsNamespace(serverKey.get(), onlineName.get(), (char) hierarchyDelimiter, m_namespace);
    }
  }
  return m_namespace;
}

void nsImapMailFolder::SetNamespaceForFolder(nsIMAPNamespace *ns)
{
#ifdef DEBUG_bienvenu
  NS_ASSERTION(ns, "null namespace");
#endif
  m_namespace = ns;
}

NS_IMETHODIMP nsImapMailFolder::FolderPrivileges(nsIMsgWindow *window)
{
  NS_ENSURE_ARG_POINTER(window);
  nsresult rv ;  // if no window...
#ifdef DEBUG_bienvenu
  m_adminUrl.Assign("http://www.netscape.com");
#endif
  if (!m_adminUrl.IsEmpty())
  {
    nsCOMPtr<nsIExternalProtocolService> extProtService = do_GetService(NS_EXTERNALPROTOCOLSERVICE_CONTRACTID);
    if (extProtService) 
    {
      nsCAutoString scheme;
      nsCOMPtr<nsIURI> uri;
      if (NS_FAILED(rv = NS_NewURI(getter_AddRefs(uri), m_adminUrl.get())))
        return rv;
      uri->GetScheme(scheme);
      if (!scheme.IsEmpty()) 
      {
        // if the URL scheme does not correspond to an exposed protocol, then we
        // need to hand this link click over to the external protocol handler.
        PRBool isExposed;
        rv = extProtService->IsExposedProtocol(scheme.get(), &isExposed);
        if (NS_SUCCEEDED(rv) && !isExposed) 
          return extProtService->LoadUrl(uri);
      }
    }
  }
  else
  {
    nsCOMPtr<nsIImapService> imapService = do_GetService(NS_IMAPSERVICE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv,rv);
    rv = imapService->GetFolderAdminUrl(m_thread, this, window, this, nsnull);
    if (NS_SUCCEEDED(rv))
      m_urlRunning = PR_TRUE;
  }
  return rv;
}

NS_IMETHODIMP nsImapMailFolder::GetHasAdminUrl(PRBool *aBool)
{
  NS_ENSURE_ARG_POINTER(aBool);
  nsCOMPtr<nsIImapIncomingServer> imapServer;
  nsresult rv = GetImapIncomingServer(getter_AddRefs(imapServer));
  nsCString manageMailAccountUrl;
  if (NS_SUCCEEDED(rv) && imapServer)
    rv = imapServer->GetManageMailAccountUrl(manageMailAccountUrl);
  *aBool = (NS_SUCCEEDED(rv) && !manageMailAccountUrl.IsEmpty());
  return rv;
}

NS_IMETHODIMP nsImapMailFolder::GetAdminUrl(nsACString& aResult)
{
  aResult = m_adminUrl;
  return NS_OK;
}

NS_IMETHODIMP nsImapMailFolder::SetAdminUrl(const nsACString& adminUrl)
{
  m_adminUrl = adminUrl;
  return NS_OK;
}

NS_IMETHODIMP nsImapMailFolder::GetHdrParser(nsIMsgParseMailMsgState **aHdrParser)
{
  NS_ENSURE_ARG_POINTER(aHdrParser);
  NS_IF_ADDREF(*aHdrParser = m_msgParser);
  return NS_OK;
}

  // this is used to issue an arbitrary imap command on the passed in msgs.
  // It assumes the command needs to be run in the selected state.
NS_IMETHODIMP nsImapMailFolder::IssueCommandOnMsgs(const nsACString& command, const char *uids, nsIMsgWindow *aWindow, nsIURI **url)
{
  nsresult rv;
  nsCOMPtr<nsIImapService> imapService = do_GetService(NS_IMAPSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv,rv);
  return imapService->IssueCommandOnMsgs(m_thread, this, aWindow, command, nsDependentCString(uids), url);
}

NS_IMETHODIMP nsImapMailFolder::FetchCustomMsgAttribute(const nsACString& attribute, const char *uids, nsIMsgWindow *aWindow, nsIURI **url)
{
  nsresult rv;
  nsCOMPtr<nsIImapService> imapService = do_GetService(NS_IMAPSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv,rv);

  return imapService->FetchCustomMsgAttribute(m_thread, this, aWindow, attribute, nsDependentCString(uids), url);
}

nsresult nsImapMailFolder::MoveIncorporatedMessage(nsIMsgDBHdr *mailHdr,
                                                   nsIMsgDatabase *sourceDB,
                                                   const nsACString& destFolderUri,
                                                   nsIMsgFilter *filter,
                                                   nsIMsgWindow *msgWindow)
{
  nsresult rv;
  if (m_moveCoalescer)
  {
    nsCOMPtr<nsIRDFService> rdf(do_GetService(kRDFServiceCID, &rv));
    NS_ENSURE_SUCCESS(rv, rv);
    nsCOMPtr<nsIRDFResource> res;
    rv = rdf->GetResource(destFolderUri, getter_AddRefs(res));
    if (NS_FAILED(rv))
      return rv;

    nsCOMPtr<nsIMsgFolder> destIFolder(do_QueryInterface(res, &rv));
    if (NS_FAILED(rv))
      return rv;

    if (destIFolder)
    {
      // check if the destination is a real folder (by checking for null parent)
      // and if it can file messages (e.g., servers or news folders can't file messages).
      // Or read only imap folders...
      PRBool canFileMessages = PR_TRUE;
      nsCOMPtr<nsIMsgFolder> parentFolder;
      destIFolder->GetParent(getter_AddRefs(parentFolder));
      if (parentFolder)
        destIFolder->GetCanFileMessages(&canFileMessages);
      if (filter && (!parentFolder || !canFileMessages))
      {
        filter->SetEnabled(PR_FALSE);
        m_filterList->SaveToDefaultFile();
        destIFolder->ThrowAlertMsg("filterDisabled",msgWindow);
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
        // keep track of nsIMsgFolder, nsTArray<nsMsgKey> pairs here in the imap code.
        // nsTArray<nsMsgKey> *idsToMoveFromInbox = msgFolder->GetImapIdsToMoveFromInbox();
        // idsToMoveFromInbox->AppendElement(keyToFilter);
        if (imapDeleteIsMoveToTrash)
        {
        }
        PRBool isRead = PR_FALSE;
        mailHdr->GetIsRead(&isRead);
        if (!isRead)
          destIFolder->SetFlag(nsMsgFolderFlags::GotNew);
        if (imapDeleteIsMoveToTrash)
          rv = NS_OK;
      }
    }
  }

  // we have to return an error because we do not actually move the message
  // it is done async and that can fail
  return rv;
}

// both of these algorithms assume that key arrays and flag states are sorted by increasing key.
void nsImapMailFolder::FindKeysToDelete(const nsTArray<nsMsgKey> &existingKeys, nsTArray<nsMsgKey> &keysToDelete, nsIImapFlagAndUidState *flagState)
{
  PRBool showDeletedMessages = ShowDeletedMessages();
  PRUint32 total = existingKeys.Length();
  PRInt32 messageIndex;

  int onlineIndex=0; // current index into flagState
  for (PRUint32 keyIndex=0; keyIndex < total; keyIndex++)
  {
    PRUint32 uidOfMessage;

    flagState->GetNumberOfMessages(&messageIndex);
    while ((onlineIndex < messageIndex) &&
         (flagState->GetUidOfMessage(onlineIndex, &uidOfMessage), (existingKeys[keyIndex] > uidOfMessage) ))
      onlineIndex++;

    imapMessageFlagsType flags;
    flagState->GetUidOfMessage(onlineIndex, &uidOfMessage);
    flagState->GetMessageFlags(onlineIndex, &flags);
    // delete this key if it is not there or marked deleted
    if ( (onlineIndex >= messageIndex ) ||
       (existingKeys[keyIndex] != uidOfMessage) ||
       ((flags & kImapMsgDeletedFlag) && !showDeletedMessages) )
    {
      nsMsgKey doomedKey = existingKeys[keyIndex];
      if ((PRInt32) doomedKey <= 0 && doomedKey != nsMsgKey_None)
        continue;
      else
        keysToDelete.AppendElement(existingKeys[keyIndex]);
    }

    flagState->GetUidOfMessage(onlineIndex, &uidOfMessage);
    if (existingKeys[keyIndex] == uidOfMessage)
      onlineIndex++;
  }
}

void nsImapMailFolder::FindKeysToAdd(const nsTArray<nsMsgKey> &existingKeys, nsTArray<nsMsgKey> &keysToFetch, PRUint32 &numNewUnread, nsIImapFlagAndUidState *flagState)
{
  PRBool showDeletedMessages = ShowDeletedMessages();
  int dbIndex=0; // current index into existingKeys
  PRInt32 existTotal, numberOfKnownKeys;
  PRInt32 messageIndex;

  numNewUnread = 0;
  existTotal = numberOfKnownKeys = existingKeys.Length();
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
      NS_ASSERTION(uidOfMessage != nsMsgKey_None, "got invalid msg key");
      if (uidOfMessage && uidOfMessage != nsMsgKey_None && (showDeletedMessages || ! (flags & kImapMsgDeletedFlag)))
      {
        if (mDatabase)
        {
          PRBool dbContainsKey;
          if (NS_SUCCEEDED(mDatabase->ContainsKey(uidOfMessage, &dbContainsKey)) && dbContainsKey)
          {
            NS_ASSERTION(PR_FALSE, "db has key - flagState messed up?");
            continue;
          }
        }
        keysToFetch.AppendElement(uidOfMessage);
        if (! (flags & kImapMsgSeenFlag))
          numNewUnread++;
      }
    }
  }
}

void nsImapMailFolder::PrepareToAddHeadersToMailDB(nsIImapProtocol* aProtocol, const nsTArray<nsMsgKey> &keysToFetch,
                                                nsIMailboxSpec *boxSpec)
{
  PRUint32 *theKeys = (PRUint32 *) PR_Malloc( keysToFetch.Length() * sizeof(PRUint32) );
  if (theKeys)
  {
    PRUint32 total = keysToFetch.Length();
    for (PRUint32 keyIndex=0; keyIndex < total; keyIndex++)
      theKeys[keyIndex] = keysToFetch[keyIndex];
    // tell the imap thread which hdrs to download
    if (aProtocol)
    {
      aProtocol->NotifyHdrsToDownload(theKeys, total /*keysToFetch.Length() */);
      // now, tell it we don't need any bodies.
      aProtocol->NotifyBodysToDownload(nsnull, 0);
    }
  }
  else if (aProtocol)
    aProtocol->NotifyHdrsToDownload(nsnull, 0);
}

void nsImapMailFolder::TweakHeaderFlags(nsIImapProtocol* aProtocol, nsIMsgDBHdr *tweakMe)
{
  if (mDatabase && aProtocol && tweakMe)
  {
    tweakMe->SetMessageKey(m_curMsgUid);
    tweakMe->SetMessageSize(m_nextMessageByteLength);

    PRBool foundIt = PR_FALSE;
    imapMessageFlagsType imap_flags;

    nsCString customFlags;
    nsresult rv = aProtocol->GetFlagsForUID(m_curMsgUid, &foundIt, &imap_flags, getter_Copies(customFlags));
    if (NS_SUCCEEDED(rv) && foundIt)
    {
      // make a mask and clear these message flags
      PRUint32 mask = MSG_FLAG_READ | MSG_FLAG_REPLIED | MSG_FLAG_MARKED | MSG_FLAG_IMAP_DELETED | MSG_FLAG_LABELS;
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
      rv = aProtocol->GetSupportedUserFlags(&userFlags);
      if (NS_SUCCEEDED(rv) && (userFlags & (kImapMsgSupportUserFlag |
                            kImapMsgSupportMDNSentFlag)))
      {
        if (imap_flags & kImapMsgMDNSentFlag)
        {
          newFlags |= MSG_FLAG_MDN_REPORT_SENT;
          if (dbHdrFlags & MSG_FLAG_MDN_REPORT_NEEDED)
            tweakMe->AndFlags(~MSG_FLAG_MDN_REPORT_NEEDED, &dbHdrFlags);
        }
      }

      if (imap_flags & kImapMsgAnsweredFlag)
        newFlags |= MSG_FLAG_REPLIED;
      if (imap_flags & kImapMsgFlaggedFlag)
        newFlags |= MSG_FLAG_MARKED;
      if (imap_flags & kImapMsgDeletedFlag)
        newFlags |= MSG_FLAG_IMAP_DELETED;
      if (imap_flags & kImapMsgForwardedFlag)
        newFlags |= MSG_FLAG_FORWARDED;

      // db label flags are 0x0E000000 and imap label flags are 0x0E00
      // so we need to shift 16 bits to the left to convert them.
      if (imap_flags & kImapMsgLabelFlags)
      {
        // we need to set label attribute on header because the dbview code
        // does msgHdr->GetLabel when asked to paint a row
        tweakMe->SetLabel((imap_flags & kImapMsgLabelFlags) >> 9);
        newFlags |= (imap_flags & kImapMsgLabelFlags) << 16;
      }
      if (newFlags)
        tweakMe->OrFlags(newFlags, &dbHdrFlags);
      if (!customFlags.IsEmpty())
        (void) HandleCustomFlags(m_curMsgUid, tweakMe, customFlags);
    }
  }
}

NS_IMETHODIMP
nsImapMailFolder::SetupMsgWriteStream(nsIFile * aFile, PRBool addDummyEnvelope)
{
  nsresult rv;
  aFile->Remove(PR_FALSE);
  nsCOMPtr<nsILocalFile>  localFile = do_QueryInterface(aFile, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = NS_NewLocalFileOutputStream(getter_AddRefs(m_tempMessageStream), localFile, PR_WRONLY | PR_CREATE_FILE | PR_TRUNCATE, 00700);
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

    m_tempMessageStream->Write(result.get(), result.Length(), &writeCount);
    result = "X-Mozilla-Status: 0001";
    result += MSG_LINEBREAK;
    m_tempMessageStream->Write(result.get(), result.Length(), &writeCount);
    result =  "X-Mozilla-Status2: 00000000";
    result += MSG_LINEBREAK;
    m_tempMessageStream->Write(result.get(), result.Length(), &writeCount);
  }
  return rv;
}

NS_IMETHODIMP nsImapMailFolder::DownloadMessagesForOffline(nsIArray *messages, nsIMsgWindow *window)
{
  nsCAutoString messageIds;
  nsTArray<nsMsgKey> srcKeyArray;
  nsresult rv = BuildIdsAndKeyArray(messages, messageIds, srcKeyArray);
  if (NS_FAILED(rv) || messageIds.IsEmpty()) return rv;

  nsCOMPtr<nsIImapService> imapService = do_GetService(NS_IMAPSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv,rv);

  SetNotifyDownloadedLines(PR_TRUE);
  rv = AcquireSemaphore(static_cast<nsIMsgImapMailFolder*>(this));
  if (NS_FAILED(rv))
  {
    ThrowAlertMsg("operationFailedFolderBusy", window);
    return rv;
  }

  return imapService->DownloadMessagesForOffline(messageIds, this, this, window);
}

NS_IMETHODIMP nsImapMailFolder::DownloadAllForOffline(nsIUrlListener *listener, nsIMsgWindow *msgWindow)
{
  nsresult rv;
  nsCOMPtr <nsIURI> runningURI;
  PRBool noSelect;
  GetFlag(nsMsgFolderFlags::ImapNoselect, &noSelect);

  if (!noSelect)
  {
    nsCAutoString messageIdsToDownload;
    nsTArray<nsMsgKey> msgsToDownload;

    GetDatabase(msgWindow);
    m_downloadingFolderForOfflineUse = PR_TRUE;

    rv = AcquireSemaphore(static_cast<nsIMsgImapMailFolder*>(this));
    if (NS_FAILED(rv))
    {
      ThrowAlertMsg("operationFailedFolderBusy", msgWindow);
      return rv;
    }
    SetNotifyDownloadedLines(PR_TRUE);
    nsCOMPtr<nsIImapService> imapService = do_GetService(NS_IMAPSERVICE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv,rv);

    // selecting the folder with m_downloadingFolderForOfflineUse true will cause
    // us to fetch any message bodies we don't have.
    rv = imapService->SelectFolder(m_thread, this, listener, msgWindow, nsnull);
    if (NS_SUCCEEDED(rv))
      m_urlRunning = PR_TRUE;
  }
  else
    rv = NS_MSG_FOLDER_UNREADABLE;
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
  // ignore this if we're downloading the whole folder and someone says
  // to turn off downloading for offline use, which can happen if a 3rd party
  // app tries to stream a message while we're downloading for offline use.
  if (!notifyDownloadedLines && m_downloadingFolderForOfflineUse)
    return NS_OK;
  m_downloadMessageForOfflineUse = notifyDownloadedLines;
  return NS_OK;
}

NS_IMETHODIMP
nsImapMailFolder::ParseAdoptedMsgLine(const char *adoptedMessageLine, nsMsgKey uidOfMessage)
{
  PRUint32 count = 0;
  nsresult rv;
  // remember the uid of the message we're downloading.
  m_curMsgUid = uidOfMessage;
  if (m_downloadMessageForOfflineUse && !m_offlineHeader)
  {
    GetMessageHeader(uidOfMessage, getter_AddRefs(m_offlineHeader));
    rv = StartNewOfflineMessage();
  }
  // adoptedMessageLine is actually a string with a lot of message lines, separated by native line terminators
  // we need to count the number of MSG_LINEBREAK's to determine how much to increment m_numOfflineMsgLines by.
  if (m_downloadMessageForOfflineUse)
  {
    const char *nextLine = adoptedMessageLine;
    do
    {
      m_numOfflineMsgLines++;
      nextLine = PL_strstr(nextLine, MSG_LINEBREAK);
      if (nextLine)
        nextLine += MSG_LINEBREAK_LEN;
    }
    while (nextLine && *nextLine);
  }
  if (m_tempMessageStream)
  {
    nsCOMPtr <nsISeekableStream> seekable (do_QueryInterface(m_tempMessageStream));
    if (seekable)
      seekable->Seek(PR_SEEK_END, 0);
    rv = m_tempMessageStream->Write(adoptedMessageLine,
                PL_strlen(adoptedMessageLine), &count);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return NS_OK;
}

void nsImapMailFolder::EndOfflineDownload()
{
  if (m_tempMessageStream)
  {
    m_tempMessageStream->Close();
    m_tempMessageStream = nsnull;
    if (mDatabase)
      mDatabase->Commit(nsMsgDBCommitType::kLargeCommit);
  }
  m_offlineHeader = nsnull;
}

NS_IMETHODIMP
nsImapMailFolder::NormalEndMsgWriteStream(nsMsgKey uidOfMessage,
                                          PRBool markRead,
                                          nsIImapUrl *imapUrl)
{
  PRBool commit = PR_FALSE;
  if (m_offlineHeader)
    EndNewOfflineMessage();
  m_curMsgUid = uidOfMessage;
  if (commit && mDatabase)
    mDatabase->Commit(nsMsgDBCommitType::kLargeCommit);
  return NS_OK;
}

NS_IMETHODIMP
nsImapMailFolder::AbortMsgWriteStream()
{
  m_offlineHeader = nsnull;
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
    if (action != nsIImapUrl::nsImapOnlineToOfflineMove)
      return NS_ERROR_FAILURE; // don't assert here...
    nsCString messageIds;
    rv = imapUrl->CreateListOfMessageIdsString(getter_Copies(messageIds));
    if (NS_FAILED(rv)) return rv;
    nsCOMPtr<nsIImapService> imapService =  do_GetService(NS_IMAPSERVICE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv,rv);
    return imapService->AddMessageFlags(m_thread, this, nsnull, nsnull,
                                      messageIds,
                                      kImapMsgDeletedFlag,
                                      PR_TRUE);
  }
  /* unhandled copystate */
  else if (m_copyState) // whoops, this is the wrong folder - should use the source folder
  {
    nsCOMPtr<nsIMsgFolder> srcFolder;
    srcFolder = do_QueryInterface(m_copyState->m_srcSupport, &rv);
    if (srcFolder)
      srcFolder->NotifyFolderEvent(mDeleteOrMoveMsgCompletedAtom);
  }
  else
    rv = NS_ERROR_FAILURE;

  return rv;
}

NS_IMETHODIMP
nsImapMailFolder::CloseMockChannel(nsIImapMockChannel * aChannel)
{
  aChannel->Close();
  return NS_OK;
}

NS_IMETHODIMP
nsImapMailFolder::ReleaseUrlCacheEntry(nsIMsgMailNewsUrl *aUrl)
{
  NS_ENSURE_ARG_POINTER(aUrl);
  return aUrl->SetMemCacheEntry(nsnull);
}

NS_IMETHODIMP
nsImapMailFolder::BeginMessageUpload()
{
  return NS_ERROR_FAILURE;
}

nsresult nsImapMailFolder::HandleCustomFlags(nsMsgKey uidOfMessage, nsIMsgDBHdr *dbHdr, nsCString &keywords)
{
  ToLowerCase(keywords);
  PRBool messageClassified = PR_TRUE;
  // Mac Mail uses "NotJunk"
  if (keywords.Find("NonJunk", PR_TRUE /* ignore case */) != -1 || keywords.Find("NotJunk", PR_TRUE /* ignore case */) != -1)
  {
    nsCAutoString msgJunkScore;
    msgJunkScore.AppendInt(nsIJunkMailPlugin::IS_HAM_SCORE);
    mDatabase->SetStringProperty(uidOfMessage, "junkscore", msgJunkScore.get());
  }
  // ### TODO: we really should parse the keywords into space delimited keywords before checking
  else if (keywords.Find("Junk", PR_TRUE /* ignore case */) != -1)
  {
    PRUint32 newFlags;
    dbHdr->AndFlags(~MSG_FLAG_NEW, &newFlags);
    nsCAutoString msgJunkScore;
    msgJunkScore.AppendInt(nsIJunkMailPlugin::IS_SPAM_SCORE);
    mDatabase->SetStringProperty(uidOfMessage, "junkscore", msgJunkScore.get());
  }
  else
    messageClassified = PR_FALSE;
  if (messageClassified)
  {
    // only set the junkscore origin if it wasn't set before. 
    nsCString existingProperty;
    dbHdr->GetStringProperty("junkscoreorigin", getter_Copies(existingProperty));
    if (existingProperty.IsEmpty())
      dbHdr->SetStringProperty("junkscoreorigin", "imapflag");
  }
  return dbHdr->SetStringProperty("keywords", keywords.get());
}

// synchronize the message flags in the database with the server flags
nsresult nsImapMailFolder::SyncFlags(nsIImapFlagAndUidState *flagState)
{
  nsresult rv = GetDatabase(nsnull); // we need a database for this
  NS_ENSURE_SUCCESS(rv, rv);
    // update all of the database flags
  PRInt32 messageIndex;
  PRUint32 messageSize;
  PRUint32 oldFolderSize = mFolderSize;
  // take this opportunity to recalculate the folder size:
  mFolderSize = 0;
  flagState->GetNumberOfMessages(&messageIndex);

  for (PRInt32 flagIndex=0; flagIndex < messageIndex; flagIndex++)
  {
    PRUint32 uidOfMessage;
    flagState->GetUidOfMessage(flagIndex, &uidOfMessage);
    imapMessageFlagsType flags;
    flagState->GetMessageFlags(flagIndex, &flags);
    nsCOMPtr<nsIMsgDBHdr> dbHdr;
    PRBool containsKey;
    rv = mDatabase->ContainsKey(uidOfMessage , &containsKey);
    // if we don't have the header, don't diddle the flags.
    // GetMsgHdrForKey will create the header if it doesn't exist.
    if (NS_FAILED(rv) || !containsKey)
      continue;

    rv = mDatabase->GetMsgHdrForKey(uidOfMessage, getter_AddRefs(dbHdr));
    if (NS_SUCCEEDED(dbHdr->GetMessageSize(&messageSize)))
      mFolderSize += messageSize;

    if (flags & kImapMsgCustomKeywordFlag)
    {
      nsCString keywords;
      if (NS_SUCCEEDED(flagState->GetCustomFlags(uidOfMessage, getter_Copies(keywords))))
      {
        if (!keywords.IsEmpty() && dbHdr && NS_SUCCEEDED(rv))
          HandleCustomFlags(uidOfMessage, dbHdr, keywords);
      }
    }
    NotifyMessageFlagsFromHdr(dbHdr, uidOfMessage, flags);
  }
  if (oldFolderSize != mFolderSize)
    NotifyIntPropertyChanged(kFolderSizeAtom, oldFolderSize, mFolderSize);

  return NS_OK;
}

// helper routine to sync the flags on a given header
nsresult nsImapMailFolder::NotifyMessageFlagsFromHdr(nsIMsgDBHdr *dbHdr, nsMsgKey msgKey, PRUint32 flags)
{
  mDatabase->MarkHdrRead(dbHdr, (flags & kImapMsgSeenFlag) != 0, nsnull);
  mDatabase->MarkHdrReplied(dbHdr, (flags & kImapMsgAnsweredFlag) != 0, nsnull);
  mDatabase->MarkHdrMarked(dbHdr, (flags & kImapMsgFlaggedFlag) != 0, nsnull);
  mDatabase->MarkImapDeleted(msgKey, (flags & kImapMsgDeletedFlag) != 0, nsnull);
  // this turns on labels, but it doesn't handle the case where the user
  // unlabels a message on one machine, and expects it to be unlabeled
  // on their other machines. If I turn that on, I'll be removing all the labels
  // that were assigned before we started storing them on the server, which will
  // make some people very unhappy.
  if (flags & kImapMsgLabelFlags)
    mDatabase->SetLabel(msgKey, (flags & kImapMsgLabelFlags) >> 9);
  else
  {
    PRUint32 supportedFlags;
    GetSupportedUserFlags(&supportedFlags);
    if (supportedFlags & kImapMsgLabelFlags)
      mDatabase->SetLabel(msgKey, 0);
  }
  if (flags & kImapMsgMDNSentFlag)
    mDatabase->MarkMDNSent(msgKey, PR_TRUE, nsnull);

  return NS_OK;
}

// message flags operation - this is called (rarely) from the imap protocol,
// proxied over from the imap thread to the ui thread
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
    if (NS_FAILED(rv) || !containsKey)
      return rv;
    rv = mDatabase->GetMsgHdrForKey(msgKey, getter_AddRefs(dbHdr));
    if(NS_SUCCEEDED(rv) && dbHdr)
      NotifyMessageFlagsFromHdr(dbHdr, msgKey, flags);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsImapMailFolder::NotifyMessageDeleted(const char * onlineFolderName, PRBool deleteAllMsgs, const char * msgIdString)
{
  if (deleteAllMsgs)
    return NS_OK;

  nsTArray<nsMsgKey> affectedMessages;
  ParseUidString(msgIdString, affectedMessages);

  if (msgIdString && !ShowDeletedMessages())
  {
    GetDatabase(nsnull);
    NS_ENSURE_TRUE(mDatabase, NS_OK);
    if (!ShowDeletedMessages())
    {
      if (!affectedMessages.IsEmpty()) // perhaps Search deleted these messages
          mDatabase->DeleteMessages(&affectedMessages, nsnull);
    }
    else // && !imapDeleteIsMoveToTrash
      SetIMAPDeletedFlag(mDatabase, affectedMessages, nsnull);
  }
  return NS_OK;
}

PRBool nsImapMailFolder::ShowDeletedMessages()
{
  nsresult rv;
  nsCOMPtr<nsIImapHostSessionList> hostSession = do_GetService(kCImapHostSessionList, &rv);
  NS_ENSURE_SUCCESS(rv, PR_FALSE);

  PRBool showDeleted = PR_FALSE;
  nsCString serverKey;
  GetServerKey(serverKey);
  hostSession->GetShowDeletedMessagesForHost(serverKey.get(), showDeleted);

  return showDeleted;
}

PRBool nsImapMailFolder::DeleteIsMoveToTrash()
{
  nsresult err;
  nsCOMPtr<nsIImapHostSessionList> hostSession = do_GetService(kCImapHostSessionList, &err);
  NS_ENSURE_SUCCESS(err, PR_TRUE);
  PRBool rv = PR_TRUE;

  nsCString serverKey;
  GetServerKey(serverKey);
  hostSession->GetDeleteIsMoveToTrashForHost(serverKey.get(), rv);
  return rv;
}

nsresult nsImapMailFolder::GetTrashFolder(nsIMsgFolder **pTrashFolder)
{
  NS_ENSURE_ARG_POINTER(pTrashFolder);
  nsCOMPtr<nsIMsgFolder> rootFolder;
  nsresult rv = GetRootFolder(getter_AddRefs(rootFolder));
  if(NS_SUCCEEDED(rv) && rootFolder)
  {
    rv = rootFolder->GetFolderWithFlags(nsMsgFolderFlags::Trash, pTrashFolder);
    if (!*pTrashFolder)
      rv = NS_ERROR_FAILURE;
  }
  return rv;
}


// store MSG_FLAG_IMAP_DELETED in the specified mailhdr records
void nsImapMailFolder::SetIMAPDeletedFlag(nsIMsgDatabase *mailDB, const nsTArray<nsMsgKey> &msgids, PRBool markDeleted)
{
  nsresult markStatus = 0;
  PRUint32 total = msgids.Length();

  for (PRUint32 msgIndex=0; !markStatus && (msgIndex < total); msgIndex++)
    markStatus = mailDB->MarkImapDeleted(msgids[msgIndex], markDeleted, nsnull);
}

NS_IMETHODIMP
nsImapMailFolder::GetMessageSizeFromDB(const char * id, PRBool idIsUid, PRUint32 *size)
{
  NS_ENSURE_ARG_POINTER(size);
  nsresult rv;
  *size = 0;
  (void) GetDatabase(nsnull);
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

NS_IMETHODIMP
nsImapMailFolder::SetImageCacheSessionForUrl(nsIMsgMailNewsUrl *mailurl)
{
  nsresult rv;
  nsCOMPtr<nsIImapService> imapService = do_GetService(NS_IMAPSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv,rv);

  nsCOMPtr<nsICacheSession> cacheSession;
  rv = imapService->GetCacheSession(getter_AddRefs(cacheSession));
  if (NS_SUCCEEDED(rv) && cacheSession)
    rv = mailurl->SetImageCacheSession(cacheSession);
  return rv;
}

NS_IMETHODIMP nsImapMailFolder::GetCurMoveCopyMessageInfo(nsIImapUrl *runningUrl, PRTime *aDate, char **aKeywords, PRUint32 *aResult)
{
  nsCOMPtr <nsISupports> copyState;
  runningUrl->GetCopyState(getter_AddRefs(copyState));
  if (copyState)
  {
    nsCOMPtr<nsImapMailCopyState> mailCopyState = do_QueryInterface(copyState);
    if (mailCopyState && mailCopyState->m_message)
    {
      nsMsgLabelValue label;
      PRUint32 supportedFlags = 0;
      mailCopyState->m_message->GetFlags(aResult);
      GetSupportedUserFlags(&supportedFlags);
      if (supportedFlags & (kImapMsgSupportUserFlag | kImapMsgLabelFlags))
      {
        mailCopyState->m_message->GetLabel(&label);
        if (label != 0)
          *aResult |= label << 25;
      }
      if (aDate)
        mailCopyState->m_message->GetDate(aDate);
      if (aKeywords && (supportedFlags & (kImapMsgSupportUserFlag)))
        mailCopyState->m_message->GetStringProperty("keywords", aKeywords);
    }
    // if we don't have a source header, and it's not the drafts folder,
    // then mark the message read, since it must be an append to the
    // fcc or templates folder.
    else if (mailCopyState)
      *aResult = mailCopyState->m_newMsgFlags;
  }
  return NS_OK;
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
  NS_PRECONDITION(aUrl, "sanity check - need to be be running non-null url");
  nsCOMPtr<nsIMsgMailNewsUrl> mailUrl = do_QueryInterface(aUrl);
  if (mailUrl)
  {
    PRBool updatingFolder;
    mailUrl->GetUpdatingFolder(&updatingFolder);
    m_updatingFolder = updatingFolder;
  }
  m_urlRunning = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP
nsImapMailFolder::OnStopRunningUrl(nsIURI *aUrl, nsresult aExitCode)
{
  nsresult rv;
  PRBool endedOfflineDownload = PR_FALSE;
  m_urlRunning = PR_FALSE;
  m_updatingFolder = PR_FALSE;
  if (m_downloadingFolderForOfflineUse)
  {
    ReleaseSemaphore(static_cast<nsIMsgImapMailFolder*>(this));
    m_downloadingFolderForOfflineUse = PR_FALSE;
    endedOfflineDownload = PR_TRUE;
    EndOfflineDownload();
  }
  nsCOMPtr<nsIMsgMailSession> session = do_GetService(NS_MSGMAILSESSION_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  if (aUrl)
  {
    nsCOMPtr<nsIMsgWindow> msgWindow;
    nsCOMPtr<nsIMsgMailNewsUrl> mailUrl = do_QueryInterface(aUrl);
    nsCOMPtr<nsIImapUrl> imapUrl = do_QueryInterface(aUrl);
    PRBool folderOpen = PR_FALSE;
    if (mailUrl)
      mailUrl->GetMsgWindow(getter_AddRefs(msgWindow));
    if (session)
      session->IsFolderOpenInWindow(this, &folderOpen);
#ifdef DEBUG_bienvenu1
    nsCString urlSpec;
    aUrl->GetSpec(getter_Copies(urlSpec));
    printf("stop running url %s\n", urlSpec);
#endif

   if (imapUrl)
   {
      DisplayStatusMsg(imapUrl, EmptyString());
      nsImapAction imapAction = nsIImapUrl::nsImapTest;
      imapUrl->GetImapAction(&imapAction);
      if (imapAction == nsIImapUrl::nsImapMsgFetch || imapAction == nsIImapUrl::nsImapMsgDownloadForOffline)
      {
        ReleaseSemaphore(static_cast<nsIMsgImapMailFolder*>(this));
        SetNotifyDownloadedLines(PR_FALSE);
        if (!endedOfflineDownload)
          EndOfflineDownload();
      }

      // Notify move, copy or delete (online operations)
      // Not sure whether nsImapDeleteMsg is even used, deletes in all three models use nsImapAddMsgFlags.
      nsCOMPtr<nsIMsgFolderNotificationService> notifier(do_GetService(NS_MSGNOTIFICATIONSERVICE_CONTRACTID));
      if (notifier && m_copyState)
      {
        if (imapAction == nsIImapUrl::nsImapOnlineMove)
          notifier->NotifyMsgsMoveCopyCompleted(PR_TRUE, m_copyState->m_messages, this);
        else if (imapAction == nsIImapUrl::nsImapOnlineCopy)
          notifier->NotifyMsgsMoveCopyCompleted(PR_FALSE, m_copyState->m_messages, this);
        else if (imapAction == nsIImapUrl::nsImapDeleteMsg)
          notifier->NotifyMsgsDeleted(m_copyState->m_messages);
      }

      switch(imapAction)
      {
      case nsIImapUrl::nsImapDeleteMsg:
      case nsIImapUrl::nsImapOnlineMove:
      case nsIImapUrl::nsImapOnlineCopy:
        if (NS_SUCCEEDED(aExitCode))
        {
          if (folderOpen)
            UpdateFolder(msgWindow);
          else
            UpdatePendingCounts();
        }
        if (m_copyState)
        {
          nsCOMPtr<nsIMsgFolder> srcFolder = do_QueryInterface(m_copyState->m_srcSupport, &rv);
          if (m_copyState->m_isMove && !m_copyState->m_isCrossServerOp)
          {
            if (NS_SUCCEEDED(aExitCode))
            {
              nsCOMPtr<nsIMsgDatabase> srcDB;
              if (srcFolder)
                  rv = srcFolder->GetMsgDatabase(msgWindow, getter_AddRefs(srcDB));
              if (NS_SUCCEEDED(rv) && srcDB)
              {
                nsRefPtr<nsImapMoveCopyMsgTxn> msgTxn;
                nsTArray<nsMsgKey> srcKeyArray;
                if (m_copyState->m_allowUndo)
                {
                  rv = m_copyState->m_undoMsgTxn->QueryInterface(NS_GET_IID(nsImapMoveCopyMsgTxn), getter_AddRefs(msgTxn));
                  if (msgTxn)
                    msgTxn->GetSrcKeyArray(srcKeyArray);
                }
                else
                {
                  nsCAutoString messageIds;
                  rv = BuildIdsAndKeyArray(m_copyState->m_messages, messageIds, srcKeyArray);
                  NS_ENSURE_SUCCESS(rv,rv);
                }

                if (!ShowDeletedMessages())
                  srcDB->DeleteMessages(&srcKeyArray, nsnull);
                else
                  MarkMessagesImapDeleted(&srcKeyArray, PR_TRUE, srcDB);
              }
              srcFolder->EnableNotifications(allMessageCountNotifications, PR_TRUE, PR_TRUE/* dbBatching*/);
              // even if we're showing deleted messages,
              // we still need to notify FE so it will show the imap deleted flag
              srcFolder->NotifyFolderEvent(mDeleteOrMoveMsgCompletedAtom);
              // is there a way to see that we think we have new msgs?
              nsCOMPtr<nsIPrefBranch> prefBranch(do_GetService(NS_PREFSERVICE_CONTRACTID, &rv));
              if (NS_SUCCEEDED(rv))
              {
                PRBool showPreviewText;
                prefBranch->GetBoolPref("mail.biff.alert.show_preview", &showPreviewText);
                // if we're showing preview text, update ourselves if we got a new unread
                // message copied so that we can download the new headers and have a chance
                // to preview the msg bodies.
                if (!folderOpen && showPreviewText && m_copyState->m_unreadCount > 0
                    && ! (mFlags & (nsMsgFolderFlags::Trash | nsMsgFolderFlags::Junk)))
                  UpdateFolder(msgWindow);
              }
            }
            else
            {
              srcFolder->EnableNotifications(allMessageCountNotifications, PR_TRUE, PR_TRUE/* dbBatching*/);
              srcFolder->NotifyFolderEvent(mDeleteOrMoveMsgFailedAtom);
            }

          }
          if (m_copyState->m_msgWindow && NS_SUCCEEDED(aExitCode)) //we should do this only if move/copy succeeds
          {
            nsCOMPtr<nsITransactionManager> txnMgr;
            m_copyState->m_msgWindow->GetTransactionManager(getter_AddRefs(txnMgr));
            if (txnMgr)
            {
              nsresult rv2 = txnMgr->DoTransaction(m_copyState->m_undoMsgTxn);
              NS_ASSERTION(NS_SUCCEEDED(rv2), "doing transaction failed");
            }
          }
           (void) OnCopyCompleted(m_copyState->m_srcSupport, aExitCode);
        }
        // we're the dest folder of a move/copy - if we're not open in the ui,
        // then we should clear our nsMsgDatabase pointer. Otherwise, the db would
        // be open until the user selected it and then selected another folder.
        // but don't do this for the trash or inbox - we'll leave them open
        if (!folderOpen && ! (mFlags & (nsMsgFolderFlags::Trash | nsMsgFolderFlags::Inbox)))
          SetMsgDatabase(nsnull);
        break;
      case nsIImapUrl::nsImapSubtractMsgFlags:
        {
        // this isn't really right - we'd like to know we were
        // deleting a message to start with, but it probably
        // won't do any harm.
          imapMessageFlagsType flags = 0;
          imapUrl->GetMsgFlags(&flags);
          //we need to subtract the delete flag in db only in case when we show deleted msgs
          if (flags & kImapMsgDeletedFlag && ShowDeletedMessages())
          {
            nsCOMPtr<nsIMsgDatabase> db;
            rv = GetMsgDatabase(nsnull, getter_AddRefs(db));
            if (NS_SUCCEEDED(rv) && db)
            {
              nsTArray<nsMsgKey> keyArray;
              char *keyString;
              imapUrl->CreateListOfMessageIdsString(&keyString);
              if (keyString)
              {
                ParseUidString(keyString, keyArray);
                MarkMessagesImapDeleted(&keyArray, PR_FALSE, db);
                db->Commit(nsMsgDBCommitType::kLargeCommit);
                NS_Free(keyString);
              }
            }
          }
        }
        break;
      case nsIImapUrl::nsImapAddMsgFlags:
        {
          imapMessageFlagsType flags = 0;
          imapUrl->GetMsgFlags(&flags);
          if (flags & kImapMsgDeletedFlag)
          {
            // we need to delete headers from db only when we don't show deleted msgs
            if (!ShowDeletedMessages())
            {
              nsCOMPtr<nsIMsgDatabase> db;
              rv = GetMsgDatabase(nsnull, getter_AddRefs(db));
              if (NS_SUCCEEDED(rv) && db)
              {
                nsTArray<nsMsgKey> keyArray;
                char *keyString = nsnull;
                imapUrl->CreateListOfMessageIdsString(&keyString);
                if (keyString)
                {
                  ParseUidString(keyString, keyArray);
                  
                  // Notify listeners of delete.
                  if (notifier)
                  {
                    nsCOMPtr<nsIMutableArray> msgHdrs(do_CreateInstance(NS_ARRAY_CONTRACTID));
                    MsgGetHeadersFromKeys(db, keyArray, msgHdrs);

                    // XXX Currently, the DeleteMessages below gets executed twice on deletes.
                    // Once in DeleteMessages, once here. The second time, it silently fails
                    // to delete. This is why we're also checking whether the array is empty.
                    PRUint32 numHdrs;
                    msgHdrs->GetLength(&numHdrs);
                    if (numHdrs)
                      notifier->NotifyMsgsDeleted(msgHdrs);
                  }

                  db->DeleteMessages(&keyArray, nsnull);
                  db->SetSummaryValid(PR_TRUE);
                  db->Commit(nsMsgDBCommitType::kLargeCommit);
                  NS_Free(keyString);
                }
              }
            }
            // see bug #188051
            // only send the folder event only if we are deleting
            // (and not for other flag changes)
            NotifyFolderEvent(mDeleteOrMoveMsgCompletedAtom);
          }
        }
        break;
      case nsIImapUrl::nsImapAppendMsgFromFile:
      case nsIImapUrl::nsImapAppendDraftFromFile:
          if (m_copyState)
          {
            if (NS_SUCCEEDED(aExitCode))
            {
              UpdatePendingCounts();

              m_copyState->m_curIndex++;
              if (m_copyState->m_curIndex >= m_copyState->m_totalCount)
              {
                if (folderOpen)
                {
                  // This gives a way for the caller to get notified
                  // when the UpdateFolder url is done.
                  nsCOMPtr <nsIUrlListener> saveUrlListener = m_urlListener;
                  if (m_copyState->m_listener)
                    m_urlListener = do_QueryInterface(m_copyState->m_listener);

                  UpdateFolder(msgWindow);
                  m_urlListener = saveUrlListener;
                }
                if (m_copyState->m_msgWindow && m_copyState->m_undoMsgTxn)
                {
                  nsCOMPtr<nsITransactionManager> txnMgr;
                  m_copyState->m_msgWindow->GetTransactionManager(getter_AddRefs(txnMgr));
                  if (txnMgr)
                    txnMgr->DoTransaction(m_copyState->m_undoMsgTxn);
                }
                (void) OnCopyCompleted(m_copyState->m_srcSupport, aExitCode);
              }
            }
            else
              //clear the copyState if copy has failed
              (void) OnCopyCompleted(m_copyState->m_srcSupport, aExitCode);
          }
          break;
      case nsIImapUrl::nsImapRenameFolder:
        if (NS_FAILED(aExitCode))
        {
          nsCOMPtr <nsIAtom> folderRenameAtom;
          folderRenameAtom = do_GetAtom("RenameCompleted");
          NotifyFolderEvent(folderRenameAtom);
        }
        break;
      case nsIImapUrl::nsImapDeleteAllMsgs:
          if (NS_SUCCEEDED(aExitCode))
          {
            if (folderOpen)
              UpdateFolder(msgWindow);
            else
            {
              ChangeNumPendingTotalMessages(-mNumPendingTotalMessages);
              ChangeNumPendingUnread(-mNumPendingUnreadMessages);
              m_numStatusUnseenMessages = 0;
            }

          }
          break;
      case nsIImapUrl::nsImapListFolder:
          // check if folder is now verified - if not,
          // we should remove it?
          if (NS_SUCCEEDED(aExitCode) && !m_verifiedAsOnlineFolder)
          {
            nsCOMPtr<nsIMsgFolder> parent;
            rv = GetParent(getter_AddRefs(parent));

            if (NS_SUCCEEDED(rv) && parent)
            {
              nsCOMPtr<nsIMsgImapMailFolder> imapParent = do_QueryInterface(parent);
              if (imapParent)
                imapParent->RemoveSubFolder(this);
            }
          }
        break;
      case nsIImapUrl::nsImapRefreshFolderUrls:
        // we finished getting an admin url for the folder.
          if (!m_adminUrl.IsEmpty())
            FolderPrivileges(msgWindow);
          break;
      case nsIImapUrl::nsImapCreateFolder:
        if (NS_FAILED(aExitCode))  //if success notification already done
        {
          nsCOMPtr <nsIAtom> folderCreateAtom;
          folderCreateAtom = do_GetAtom("FolderCreateFailed");
          NotifyFolderEvent(folderCreateAtom);
        }
        break;
      case nsIImapUrl::nsImapSubscribe:
        if (NS_SUCCEEDED(aExitCode) && msgWindow)
        {
          nsCString canonicalFolderName;
          imapUrl->CreateCanonicalSourceFolderPathString(getter_Copies(canonicalFolderName));
          nsCOMPtr <nsIMsgFolder> rootFolder;
          nsresult rv = GetRootFolder(getter_AddRefs(rootFolder));
          if(NS_SUCCEEDED(rv) && rootFolder)
          {
            nsCOMPtr <nsIMsgImapMailFolder> imapRoot = do_QueryInterface(rootFolder);
            if (imapRoot)
            {
              nsCOMPtr <nsIMsgImapMailFolder> foundFolder;
              rv = imapRoot->FindOnlineSubFolder(canonicalFolderName, getter_AddRefs(foundFolder));
              if (NS_SUCCEEDED(rv) && foundFolder)
              {
                nsCString uri;
                nsCOMPtr <nsIMsgFolder> msgFolder = do_QueryInterface(foundFolder);
                if (msgFolder)
                {
                  msgFolder->GetURI(uri);
                  nsCOMPtr<nsIMsgWindowCommands> windowCommands;
                  msgWindow->GetWindowCommands(getter_AddRefs(windowCommands));
                  if (windowCommands)
                    windowCommands->SelectFolder(uri);
                }
              }
            }
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
    if (mailUrl)
      rv = mailUrl->UnRegisterListener(this);

  }
  SetGettingNewMessages(PR_FALSE); // if we're not running a url, we must not be getting new mail :-)
  if (m_urlListener)
  {
    m_urlListener->OnStopRunningUrl(aUrl, aExitCode);
    m_urlListener = nsnull;
  }
  return rv;
}

void nsImapMailFolder::UpdatePendingCounts()
{
  if (m_copyState)
  {
    ChangeNumPendingTotalMessages(m_copyState->m_isCrossServerOp ? 1 : m_copyState->m_totalCount);

    // count the moves that were unread
    int numUnread = m_copyState->m_unreadCount;
    if (numUnread)
    {
      m_numStatusUnseenMessages += numUnread; // adjust last status count by this delta.
      ChangeNumPendingUnread(numUnread);
    }
    SummaryChanged();
  }
}

NS_IMETHODIMP
nsImapMailFolder::ClearFolderRights()
{
  SetFolderNeedsACLListed(PR_FALSE);
  delete m_folderACL;
  m_folderACL = new nsMsgIMAPFolderACL(this);
  return NS_OK;
}

NS_IMETHODIMP
nsImapMailFolder::AddFolderRights(const nsACString& userName, const nsACString& rights)
{
  SetFolderNeedsACLListed(PR_FALSE);
  GetFolderACL()->SetFolderRightsForUser(userName, rights);
  return NS_OK;
}

NS_IMETHODIMP
nsImapMailFolder::RefreshFolderRights()
{
  if (GetFolderACL()->GetIsFolderShared())
    SetFlag(nsMsgFolderFlags::PersonalShared);
  else
    ClearFlag(nsMsgFolderFlags::PersonalShared);
  return NS_OK;
}

NS_IMETHODIMP
nsImapMailFolder::SetCopyResponseUid(const char* msgIdString,
                                     nsIImapUrl * aUrl)
{   // CopyMessages() only
  nsresult rv = NS_OK;
  nsRefPtr<nsImapMoveCopyMsgTxn> msgTxn;
  nsCOMPtr<nsISupports> copyState;

  if (aUrl)
    aUrl->GetCopyState(getter_AddRefs(copyState));

  if (copyState)
  {
    nsCOMPtr<nsImapMailCopyState> mailCopyState =
        do_QueryInterface(copyState, &rv);
    if (NS_FAILED(rv)) return rv;
    if (mailCopyState->m_undoMsgTxn)
      rv = mailCopyState->m_undoMsgTxn->QueryInterface(NS_GET_IID(nsImapMoveCopyMsgTxn), getter_AddRefs(msgTxn));
  }
  else if (aUrl && m_pendingOfflineMoves.Count())
  {
    nsCString urlSourceMsgIds, undoTxnSourceMsgIds;
    aUrl->CreateListOfMessageIdsString(getter_Copies(urlSourceMsgIds));
    nsCOMPtr <nsITransaction> firstTransaction = m_pendingOfflineMoves[0];
    nsRefPtr<nsImapMoveCopyMsgTxn> imapUndo;
    firstTransaction->QueryInterface(NS_GET_IID(nsImapMoveCopyMsgTxn), getter_AddRefs(imapUndo));
    if (imapUndo)
    {
      imapUndo->GetSrcMsgIds(undoTxnSourceMsgIds);
      if (undoTxnSourceMsgIds.Equals(urlSourceMsgIds))
        msgTxn = imapUndo;
      // ### we should handle batched moves, but lets keep it simple for a2.
      m_pendingOfflineMoves.Clear();
    }
  }
  if (msgTxn)
    msgTxn->SetCopyResponseUid(msgIdString);
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
  if (!mDatabase || NS_FAILED(rv))
    return rv;
  
  // expect search results in the form of "* SEARCH <hit> <hit> ..."
  // expect search results in the form of "* SEARCH <hit> <hit> ..."
  nsCString tokenString(searchHitLine);
  char *currentPosition = PL_strcasestr(tokenString.get(), "SEARCH");
  if (currentPosition)
  {
    currentPosition += strlen("SEARCH");
    PRBool shownUpdateAlert = PR_FALSE;
    char *hitUidToken = NS_strtok(WHITESPACE, &currentPosition);
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
      }

      hitUidToken = NS_strtok(WHITESPACE, &currentPosition);
    }
}
  return NS_OK;
}

NS_IMETHODIMP
nsImapMailFolder::SetAppendMsgUid(nsMsgKey aKey,
                                  nsIImapUrl * aUrl)
{
  nsresult rv;
  nsCOMPtr<nsISupports> copyState;
  if (aUrl)
    aUrl->GetCopyState(getter_AddRefs(copyState));
  if (copyState)
  {
    nsCOMPtr<nsImapMailCopyState> mailCopyState = do_QueryInterface(copyState, &rv);
    if (NS_FAILED(rv)) return rv;

    if (mailCopyState->m_undoMsgTxn) // CopyMessages()
    {
        nsRefPtr<nsImapMoveCopyMsgTxn> msgTxn;
        rv = mailCopyState->m_undoMsgTxn->QueryInterface(NS_GET_IID(nsImapMoveCopyMsgTxn), getter_AddRefs(msgTxn));
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
nsImapMailFolder::GetMessageId(nsIImapUrl * aUrl,
                               nsACString &messageId)
{
  nsresult rv = NS_OK;
  nsCOMPtr<nsISupports> copyState;

  if (aUrl)
    aUrl->GetCopyState(getter_AddRefs(copyState));
  if (copyState)
  {
    nsCOMPtr<nsImapMailCopyState> mailCopyState = do_QueryInterface(copyState, &rv);
    if (NS_FAILED(rv)) return rv;
    if (mailCopyState->m_listener)
      rv = mailCopyState->m_listener->GetMessageId(messageId);
  }
  if (NS_SUCCEEDED(rv) && messageId.Length() > 0)
  {
    if (messageId.First() == '<')
        messageId.Cut(0, 1);
    if (messageId.Last() == '>')
        messageId.SetLength(messageId.Length() -1);
  }
  return rv;
}

NS_IMETHODIMP
nsImapMailFolder::HeaderFetchCompleted(nsIImapProtocol* aProtocol)
{
  nsCOMPtr <nsIMsgWindow> msgWindow; // we might need this for the filter plugins.
  if (mDatabase)
    mDatabase->Commit(nsMsgDBCommitType::kLargeCommit);
  SetSizeOnDisk(mFolderSize);
  PRInt32 numNewBiffMsgs = 0;
  if (m_performingBiff)
    GetNumNewMessages(PR_FALSE, &numNewBiffMsgs);

  PRBool pendingMoves = m_moveCoalescer && m_moveCoalescer->HasPendingMoves();
  PlaybackCoalescedOperations();
  if (aProtocol)
  {
    // check if we should download message bodies because it's the inbox and
    // the server is specified as one where where we download msg bodies automatically.
    // Or if we autosyncing all offline folders.
    nsCOMPtr<nsIImapIncomingServer> imapServer;
    GetImapIncomingServer(getter_AddRefs(imapServer));

    PRBool autoDownloadNewHeaders = PR_FALSE;
    PRBool autoSyncOfflineStores = PR_FALSE;

    if (imapServer)
      imapServer->GetAutoSyncOfflineStores(&autoSyncOfflineStores);
    if (autoSyncOfflineStores || mFlags & nsMsgFolderFlags::Inbox)
    {
      if (imapServer && mFlags & nsMsgFolderFlags::Inbox && !autoSyncOfflineStores)
        imapServer->GetDownloadBodiesOnGetNewMail(&autoDownloadNewHeaders);
      // this isn't quite right - we only want to download bodies for new headers
      // but we don't know what the new headers are. We could query the inbox db
      // for new messages, if the above filter playback actually moves the filtered
      // messages before we get to this code.
      if (autoDownloadNewHeaders || autoSyncOfflineStores)
      {
          // acquire semaphore for offline store. If it fails, we won't download for offline use.
        if (NS_SUCCEEDED(AcquireSemaphore(static_cast<nsIMsgImapMailFolder*>(this))))
          m_downloadingFolderForOfflineUse = PR_TRUE;
      }
    }

    if (m_downloadingFolderForOfflineUse)
    {
      nsTArray<nsMsgKey> keysToDownload;
      GetBodysToDownload(&keysToDownload);
      if (!keysToDownload.IsEmpty())
        SetNotifyDownloadedLines(PR_TRUE);

      aProtocol->NotifyBodysToDownload(keysToDownload.Elements(), keysToDownload.Length());
    }
    else
      aProtocol->NotifyBodysToDownload(nsnull, 0/*keysToFetch.Length() */);

    nsCOMPtr <nsIURI> runningUri;
    aProtocol->GetRunningUrl(getter_AddRefs(runningUri));
    if (runningUri)
    {
      nsCOMPtr <nsIMsgMailNewsUrl> mailnewsUrl = do_QueryInterface(runningUri);
      if (mailnewsUrl)
        mailnewsUrl->GetMsgWindow(getter_AddRefs(msgWindow));
    }
  }

  PRBool filtersRun;
  CallFilterPlugins(msgWindow, &filtersRun);
  if (!filtersRun && m_performingBiff && mDatabase && numNewBiffMsgs > 0 &&
      (!pendingMoves || !ShowPreviewText()))
  {
    if (!pendingMoves)
      SetHasNewMessages(PR_TRUE);

    // If we are performing biff for this folder, tell the
    // stand-alone biff about the new high water mark
    // We must ensure that the server knows that we are performing biff.
    // Otherwise the stand-alone biff won't fire.
    nsCOMPtr<nsIMsgIncomingServer> server;
    if (NS_SUCCEEDED(GetServer(getter_AddRefs(server))) && server)
      server->SetPerformingBiff(PR_TRUE);

    SetBiffState(nsIMsgFolder::nsMsgBiffState_NewMail);
    if (server)
      server->SetPerformingBiff(PR_FALSE);
    m_performingBiff = PR_FALSE;
  }

  if (m_filterList)
    (void)m_filterList->FlushLogIfNecessary();

  return NS_OK;
}

NS_IMETHODIMP
nsImapMailFolder::SetBiffStateAndUpdate(nsMsgBiffState biffState)
{
  SetBiffState(biffState);
  return NS_OK;
}

NS_IMETHODIMP
nsImapMailFolder::GetUidValidity(PRInt32 *uidValidity)
{
  NS_ENSURE_ARG(uidValidity);
  if (m_uidValidity == kUidUnknown)
  {
    nsCOMPtr<nsIMsgDatabase> db;
    nsCOMPtr<nsIDBFolderInfo> dbFolderInfo;
    (void) GetDBFolderInfoAndDB(getter_AddRefs(dbFolderInfo), getter_AddRefs(db));
    if (db)
      db->GetDBFolderInfo(getter_AddRefs(dbFolderInfo));

    if (dbFolderInfo)
      dbFolderInfo->GetImapUidValidity((PRInt32 *) &m_uidValidity);
  }
  *uidValidity = m_uidValidity;
  return NS_OK;
}

NS_IMETHODIMP
nsImapMailFolder::SetUidValidity(PRInt32 uidValidity)
{
  m_uidValidity = uidValidity;
  return NS_OK;
}

NS_IMETHODIMP
nsImapMailFolder::FillInFolderProps(nsIMsgImapFolderProps *aFolderProps)
{
  NS_ENSURE_ARG(aFolderProps);
  PRUint32 folderTypeStringID;
  PRUint32 folderTypeDescStringID = 0;
  PRUint32 folderQuotaStatusStringID;
  nsString folderType;
  nsString folderTypeDesc;
  nsString folderQuotaStatusDesc;
  nsCOMPtr<nsIStringBundle> bundle;
  nsresult rv = IMAPGetStringBundle(getter_AddRefs(bundle));
  NS_ENSURE_SUCCESS(rv, rv);

  // get the host session list and get server capabilities.
  PRUint32 capability = kCapabilityUndefined;

  nsCOMPtr<nsIImapHostSessionList> hostSession = do_GetService(kCImapHostSessionList, &rv);
  // if for some bizarre reason this fails, we'll still fall through to the normal sharing code
  if (NS_SUCCEEDED(rv) && hostSession)
  {
    nsCString serverKey;
    GetServerKey(serverKey);
    hostSession->GetCapabilityForHost(serverKey.get(), capability);

    // Figure out what to display in the Quota tab of the folder properties.
    // Does the server support quotas?
    if(capability & kQuotaCapability)
    {
      // Have we asked the server for quota information?
      if(m_folderQuotaCommandIssued)
      {
        // Has the server replied with storage quota info?
        if(m_folderQuotaDataIsValid)
        {
          // If so, set quota data
          folderQuotaStatusStringID = 0;
          aFolderProps->SetQuotaData(m_folderQuotaRoot, m_folderQuotaUsedKB, m_folderQuotaMaxKB);
        }
        else
        {
          // If not, there is no storage quota set on this folder
          folderQuotaStatusStringID = IMAP_QUOTA_STATUS_NOQUOTA;
        }
      }
      else
      {
        // The folder is not open, so no quota information is available
        folderQuotaStatusStringID = IMAP_QUOTA_STATUS_FOLDERNOTOPEN;
      }
    }
    else
    {
      // Either the server doesn't support quotas, or we don't know if it does
      // (e.g., because we don't have a connection yet). If the latter, we fall back
      // to saying that no information is available because the folder is not open.
      folderQuotaStatusStringID = (capability == kCapabilityUndefined) ?
        IMAP_QUOTA_STATUS_FOLDERNOTOPEN : IMAP_QUOTA_STATUS_NOTSUPPORTED;
    }

    if(folderQuotaStatusStringID == 0)
    {
      // Display quota data
      aFolderProps->ShowQuotaData(PR_TRUE);
    }
    else
    {
      // Hide quota data and show reason why it is not available
      aFolderProps->ShowQuotaData(PR_FALSE);

      rv = IMAPGetStringByID(folderQuotaStatusStringID, getter_Copies(folderQuotaStatusDesc));
      if (NS_SUCCEEDED(rv))
        aFolderProps->SetQuotaStatus(folderQuotaStatusDesc);
    }

    // See if the server supports ACL.
    // If not, just set the folder description to a string that says
    // the server doesn't support sharing, and return.
    if (! (capability & kACLCapability))
    {
      rv = IMAPGetStringByID(IMAP_SERVER_DOESNT_SUPPORT_ACL, getter_Copies(folderTypeDesc));
      if (NS_SUCCEEDED(rv))
        aFolderProps->SetFolderTypeDescription(folderTypeDesc);
      aFolderProps->ServerDoesntSupportACL();
      return NS_OK;
    }
  }
  if (mFlags & nsMsgFolderFlags::ImapPublic)
  {
    folderTypeStringID = IMAP_PUBLIC_FOLDER_TYPE_NAME;
    folderTypeDescStringID = IMAP_PUBLIC_FOLDER_TYPE_DESCRIPTION;
  }
  else if (mFlags & nsMsgFolderFlags::ImapOtherUser)
  {
    folderTypeStringID = IMAP_OTHER_USERS_FOLDER_TYPE_NAME;
    nsCString owner;
    nsString uniOwner;
    GetFolderOwnerUserName(owner);
    if (owner.IsEmpty())
    {
      rv = IMAPGetStringByID(folderTypeStringID, getter_Copies(uniOwner));
      // Another user's folder, for which we couldn't find an owner name
      NS_ASSERTION(PR_FALSE, "couldn't get owner name for other user's folder");
    }
    else
    {
      // is this right? It doesn't leak, does it?
      CopyASCIItoUTF16(owner, uniOwner);
    }
    const PRUnichar *params[] = { uniOwner.get() };
    rv = bundle->FormatStringFromID(IMAP_OTHER_USERS_FOLDER_TYPE_DESCRIPTION, params, 1, getter_Copies(folderTypeDesc));
  }
  else if (GetFolderACL()->GetIsFolderShared())
  {
    folderTypeStringID = IMAP_PERSONAL_SHARED_FOLDER_TYPE_NAME;
    folderTypeDescStringID = IMAP_PERSONAL_SHARED_FOLDER_TYPE_DESCRIPTION;
  }
  else
  {
    folderTypeStringID = IMAP_PERSONAL_SHARED_FOLDER_TYPE_NAME;
    folderTypeDescStringID = IMAP_PERSONAL_FOLDER_TYPE_DESCRIPTION;
  }

  rv = IMAPGetStringByID(folderTypeStringID, getter_Copies(folderType));
  if (NS_SUCCEEDED(rv))
    aFolderProps->SetFolderType(folderType);

  if (folderTypeDesc.IsEmpty() && folderTypeDescStringID != 0)
    rv = IMAPGetStringByID(folderTypeDescStringID, getter_Copies(folderTypeDesc));
  if (!folderTypeDesc.IsEmpty())
    aFolderProps->SetFolderTypeDescription(folderTypeDesc);

  nsString rightsString;
  rv = CreateACLRightsStringForFolder(rightsString);
  if (NS_SUCCEEDED(rv))
    aFolderProps->SetFolderPermissions(rightsString);
  return NS_OK;
}

NS_IMETHODIMP nsImapMailFolder::SetAclFlags(PRUint32 aclFlags)
{
  nsresult rv = NS_OK;
  if (m_aclFlags != aclFlags)
  {
    nsCOMPtr<nsIDBFolderInfo> dbFolderInfo;
    PRBool dbWasOpen = (mDatabase != nsnull);
    rv = GetDatabase(nsnull);

    m_aclFlags = aclFlags;
    if (mDatabase)
    {
      rv = mDatabase->GetDBFolderInfo(getter_AddRefs(dbFolderInfo));
      if (NS_SUCCEEDED(rv) && dbFolderInfo)
        dbFolderInfo->SetUint32Property("aclFlags", aclFlags);
      // if setting the acl flags caused us to open the db, release the ref
      // because on startup, we might get acl on all folders,which will
      // leave a lot of db's open.
      if (!dbWasOpen)
      {
        mDatabase->Close(PR_TRUE /* commit changes */);
        mDatabase = nsnull;
      }
    }
  }
  return rv;
}

NS_IMETHODIMP nsImapMailFolder::GetAclFlags(PRUint32 *aclFlags)
{
  NS_ENSURE_ARG_POINTER(aclFlags);
  nsresult rv;
  ReadDBFolderInfo(PR_FALSE); // update cache first.
  if (m_aclFlags == -1) // -1 means invalid value, so get it from db.
  {
    nsCOMPtr<nsIDBFolderInfo> dbFolderInfo;
    PRBool dbWasOpen = (mDatabase != nsnull);
    rv = GetDatabase(nsnull);

    if (mDatabase)
    {
      rv = mDatabase->GetDBFolderInfo(getter_AddRefs(dbFolderInfo));
      if (NS_SUCCEEDED(rv) && dbFolderInfo)
      {
        rv = dbFolderInfo->GetUint32Property("aclFlags", 0, aclFlags);
        m_aclFlags = *aclFlags;
      }
      // if getting the acl flags caused us to open the db, release the ref
      // because on startup, we might get acl on all folders,which will
      // leave a lot of db's open.
      if (!dbWasOpen)
      {
        mDatabase->Close(PR_TRUE /* commit changes */);
        mDatabase = nsnull;
      }
    }
  }
  else
    *aclFlags = m_aclFlags;
  return NS_OK;
}

nsresult nsImapMailFolder::SetSupportedUserFlags(PRUint32 userFlags)
{
  nsCOMPtr<nsIDBFolderInfo> dbFolderInfo;
  nsresult rv = GetDatabase(nsnull);

  m_supportedUserFlags = userFlags;
  if (mDatabase)
  {
    rv = mDatabase->GetDBFolderInfo(getter_AddRefs(dbFolderInfo));
    if (NS_SUCCEEDED(rv) && dbFolderInfo)
      dbFolderInfo->SetUint32Property("imapFlags", userFlags);
  }
  return rv;
}

nsresult nsImapMailFolder::GetSupportedUserFlags(PRUint32 *userFlags)
{
  NS_ENSURE_ARG_POINTER(userFlags);

  nsresult rv = NS_OK;

  ReadDBFolderInfo(PR_FALSE); // update cache first.
  if (m_supportedUserFlags == 0) // 0 means invalid value, so get it from db.
  {
    nsCOMPtr<nsIDBFolderInfo> dbFolderInfo;
    rv = GetDatabase(nsnull);

    if (mDatabase)
    {
      rv = mDatabase->GetDBFolderInfo(getter_AddRefs(dbFolderInfo));
      if (NS_SUCCEEDED(rv) && dbFolderInfo)
      {
        rv = dbFolderInfo->GetUint32Property("imapFlags", 0, userFlags);
        m_supportedUserFlags = *userFlags;
      }
    }
  }
  else
    *userFlags = m_supportedUserFlags;
  return rv;
}

NS_IMETHODIMP nsImapMailFolder::GetCanOpenFolder(PRBool *aBool)
{
  NS_ENSURE_ARG_POINTER(aBool);
  PRBool noSelect;
  GetFlag(nsMsgFolderFlags::ImapNoselect, &noSelect);
  *aBool = (noSelect) ? PR_FALSE : GetFolderACL()->GetCanIReadFolder();
  return NS_OK;
}

///////// nsMsgIMAPFolderACL class ///////////////////////////////

// This string is defined in the ACL RFC to be "anyone"
#define IMAP_ACL_ANYONE_STRING "anyone"

nsMsgIMAPFolderACL::nsMsgIMAPFolderACL(nsImapMailFolder *folder)
{
  NS_ASSERTION(folder, "need folder");
  m_folder = folder;
  m_rightsHash.Init(24);
  m_aclCount = 0;
  BuildInitialACLFromCache();
}

nsMsgIMAPFolderACL::~nsMsgIMAPFolderACL()
{
}

// We cache most of our own rights in the MSG_FOLDER_PREF_* flags
void nsMsgIMAPFolderACL::BuildInitialACLFromCache()
{
  nsCAutoString myrights;

  PRUint32 startingFlags;
  m_folder->GetAclFlags(&startingFlags);

  if (startingFlags & IMAP_ACL_READ_FLAG)
    myrights += "r";
  if (startingFlags & IMAP_ACL_STORE_SEEN_FLAG)
    myrights += "s";
  if (startingFlags & IMAP_ACL_WRITE_FLAG)
    myrights += "w";
  if (startingFlags & IMAP_ACL_INSERT_FLAG)
    myrights += "i";
  if (startingFlags & IMAP_ACL_POST_FLAG)
    myrights += "p";
  if (startingFlags & IMAP_ACL_CREATE_SUBFOLDER_FLAG)
    myrights +="c";
  if (startingFlags & IMAP_ACL_DELETE_FLAG)
    myrights += "dt";
  if (startingFlags & IMAP_ACL_ADMINISTER_FLAG)
    myrights += "a";
  if (startingFlags & IMAP_ACL_EXPUNGE_FLAG)
    myrights += "e";

  if (!myrights.IsEmpty())
    SetFolderRightsForUser(EmptyCString(), myrights);
}

void nsMsgIMAPFolderACL::UpdateACLCache()
{
  PRUint32 startingFlags = 0;
  m_folder->GetAclFlags(&startingFlags);

  if (GetCanIReadFolder())
    startingFlags |= IMAP_ACL_READ_FLAG;
  else
    startingFlags &= ~IMAP_ACL_READ_FLAG;

  if (GetCanIStoreSeenInFolder())
    startingFlags |= IMAP_ACL_STORE_SEEN_FLAG;
  else
    startingFlags &= ~IMAP_ACL_STORE_SEEN_FLAG;

  if (GetCanIWriteFolder())
    startingFlags |= IMAP_ACL_WRITE_FLAG;
  else
    startingFlags &= ~IMAP_ACL_WRITE_FLAG;

  if (GetCanIInsertInFolder())
    startingFlags |= IMAP_ACL_INSERT_FLAG;
  else
    startingFlags &= ~IMAP_ACL_INSERT_FLAG;

  if (GetCanIPostToFolder())
    startingFlags |= IMAP_ACL_POST_FLAG;
  else
    startingFlags &= ~IMAP_ACL_POST_FLAG;

  if (GetCanICreateSubfolder())
    startingFlags |= IMAP_ACL_CREATE_SUBFOLDER_FLAG;
  else
    startingFlags &= ~IMAP_ACL_CREATE_SUBFOLDER_FLAG;

  if (GetCanIDeleteInFolder())
    startingFlags |= IMAP_ACL_DELETE_FLAG;
  else
    startingFlags &= ~IMAP_ACL_DELETE_FLAG;

  if (GetCanIAdministerFolder())
    startingFlags |= IMAP_ACL_ADMINISTER_FLAG;
  else
    startingFlags &= ~IMAP_ACL_ADMINISTER_FLAG;

  if (GetCanIExpungeFolder())
    startingFlags |= IMAP_ACL_EXPUNGE_FLAG;
  else
    startingFlags &= ~IMAP_ACL_EXPUNGE_FLAG;

  m_folder->SetAclFlags(startingFlags);
}

PRBool nsMsgIMAPFolderACL::SetFolderRightsForUser(const nsACString& userName, const nsACString& rights)
{
  PRBool ret = PR_FALSE;
  nsCString myUserName;
  nsCOMPtr <nsIMsgIncomingServer> server;
  nsresult rv = m_folder->GetServer(getter_AddRefs(server));
  if (NS_FAILED(rv))
    return ret;
  // we need the real user name to match with what the imap server returns
  // in the acl response.
  server->GetRealUsername(myUserName);

  nsCAutoString ourUserName;
  if (userName.IsEmpty())
    ourUserName.Assign(myUserName);
  else
    ourUserName.Assign(userName);

  if (ourUserName.IsEmpty())
    return ret;

  ToLowerCase(ourUserName);
  nsCString oldValue;
  m_rightsHash.Get(ourUserName, &oldValue);
  if (!oldValue.IsEmpty())
  {
    m_rightsHash.Remove(ourUserName);
    m_aclCount--;
    NS_ASSERTION(m_aclCount >= 0, "acl count can't go negative");
  }
  m_aclCount++;
  ret = (m_rightsHash.Put(ourUserName, PromiseFlatCString(rights)) == 0);

  if (myUserName.Equals(ourUserName) || ourUserName.EqualsLiteral(IMAP_ACL_ANYONE_STRING))
    // if this is setting an ACL for me, cache it in the folder pref flags
    UpdateACLCache();

  return ret;
}

nsresult nsMsgIMAPFolderACL::GetRightsStringForUser(const nsACString& inUserName, nsCString &rights)
{
  nsCString userName;
  userName.Assign(inUserName);
  if (userName.IsEmpty())
  {
    nsCOMPtr <nsIMsgIncomingServer> server;

    nsresult rv = m_folder->GetServer(getter_AddRefs(server));
    NS_ENSURE_SUCCESS(rv, rv);
    // we need the real user name to match with what the imap server returns
    // in the acl response.
    server->GetRealUsername(userName);
  }
  ToLowerCase(userName);
  m_rightsHash.Get(userName, &rights);
  return NS_OK;
}

// First looks for individual user;  then looks for 'anyone' if the user isn't found.
// Returns defaultIfNotFound, if neither are found.
PRBool nsMsgIMAPFolderACL::GetFlagSetInRightsForUser(const nsACString& userName, char flag, PRBool defaultIfNotFound)
{
  nsCString flags;
  nsresult rv = GetRightsStringForUser(userName, flags);
  NS_ENSURE_SUCCESS(rv, defaultIfNotFound);
  if (flags.IsEmpty())
  {
    nsCString anyoneFlags;
    GetRightsStringForUser(NS_LITERAL_CSTRING(IMAP_ACL_ANYONE_STRING), anyoneFlags);
    if (anyoneFlags.IsEmpty())
      return defaultIfNotFound;
    else
      return (anyoneFlags.FindChar(flag) != kNotFound);
  }
  else
    return (flags.FindChar(flag) != kNotFound);
}

PRBool nsMsgIMAPFolderACL::GetCanUserLookupFolder(const nsACString& userName)
{
  return GetFlagSetInRightsForUser(userName, 'l', PR_FALSE);
}

PRBool nsMsgIMAPFolderACL::GetCanUserReadFolder(const nsACString& userName)
{
  return GetFlagSetInRightsForUser(userName, 'r', PR_FALSE);
}

PRBool nsMsgIMAPFolderACL::GetCanUserStoreSeenInFolder(const nsACString& userName)
{
  return GetFlagSetInRightsForUser(userName, 's', PR_FALSE);
}

PRBool nsMsgIMAPFolderACL::GetCanUserWriteFolder(const nsACString& userName)
{
  return GetFlagSetInRightsForUser(userName, 'w', PR_FALSE);
}

PRBool nsMsgIMAPFolderACL::GetCanUserInsertInFolder(const nsACString& userName)
{
  return GetFlagSetInRightsForUser(userName, 'i', PR_FALSE);
}

PRBool nsMsgIMAPFolderACL::GetCanUserPostToFolder(const nsACString& userName)
{
  return GetFlagSetInRightsForUser(userName, 'p', PR_FALSE);
}

PRBool nsMsgIMAPFolderACL::GetCanUserCreateSubfolder(const nsACString& userName)
{
  return GetFlagSetInRightsForUser(userName, 'c', PR_FALSE);
}

PRBool nsMsgIMAPFolderACL::GetCanUserDeleteInFolder(const nsACString& userName)
{
  return GetFlagSetInRightsForUser(userName, 'd', PR_FALSE)
    || GetFlagSetInRightsForUser(userName, 't', PR_FALSE);
}

PRBool nsMsgIMAPFolderACL::GetCanUserAdministerFolder(const nsACString& userName)
{
  return GetFlagSetInRightsForUser(userName, 'a', PR_FALSE);
}

PRBool nsMsgIMAPFolderACL::GetCanILookupFolder()
{
  return GetFlagSetInRightsForUser(EmptyCString(), 'l', PR_TRUE);
}

PRBool nsMsgIMAPFolderACL::GetCanIReadFolder()
{
  return GetFlagSetInRightsForUser(EmptyCString(), 'r', PR_TRUE);
}

PRBool nsMsgIMAPFolderACL::GetCanIStoreSeenInFolder()
{
  return GetFlagSetInRightsForUser(EmptyCString(), 's', PR_TRUE);
}

PRBool nsMsgIMAPFolderACL::GetCanIWriteFolder()
{
  return GetFlagSetInRightsForUser(EmptyCString(), 'w', PR_TRUE);
}

PRBool nsMsgIMAPFolderACL::GetCanIInsertInFolder()
{
  return GetFlagSetInRightsForUser(EmptyCString(), 'i', PR_TRUE);
}

PRBool nsMsgIMAPFolderACL::GetCanIPostToFolder()
{
  return GetFlagSetInRightsForUser(EmptyCString(), 'p', PR_TRUE);
}

PRBool nsMsgIMAPFolderACL::GetCanICreateSubfolder()
{
  return GetFlagSetInRightsForUser(EmptyCString(), 'c', PR_TRUE);
}

PRBool nsMsgIMAPFolderACL::GetCanIDeleteInFolder()
{
  return GetFlagSetInRightsForUser(EmptyCString(), 'd', PR_TRUE) ||
    GetFlagSetInRightsForUser(EmptyCString(), 't', PR_TRUE);
}

PRBool nsMsgIMAPFolderACL::GetCanIAdministerFolder()
{
  return GetFlagSetInRightsForUser(EmptyCString(), 'a', PR_TRUE);
}

PRBool nsMsgIMAPFolderACL::GetCanIExpungeFolder()
{
  return GetFlagSetInRightsForUser(EmptyCString(), 'e', PR_TRUE) ||
    GetFlagSetInRightsForUser(EmptyCString(), 'd', PR_TRUE);
}

// We use this to see if the ACLs think a folder is shared or not.
// We will define "Shared" in 5.0 to mean:
// At least one user other than the currently authenticated user has at least one
// explicitly-listed ACL right on that folder.
PRBool nsMsgIMAPFolderACL::GetIsFolderShared()
{
  // If we have more than one ACL count for this folder, which means that someone
  // other than ourself has rights on it, then it is "shared."
  if (m_aclCount > 1)
    return PR_TRUE;

  // Or, if "anyone" has rights to it, it is shared.
  nsCString anyonesRights;
  m_rightsHash.Get(NS_LITERAL_CSTRING(IMAP_ACL_ANYONE_STRING), &anyonesRights);
  return (!anyonesRights.IsEmpty());
}

PRBool nsMsgIMAPFolderACL::GetDoIHaveFullRightsForFolder()
{
  return (GetCanIReadFolder() &&
    GetCanIWriteFolder() &&
    GetCanIInsertInFolder() &&
    GetCanIAdministerFolder() &&
    GetCanICreateSubfolder() &&
    GetCanIDeleteInFolder() &&
    GetCanILookupFolder() &&
    GetCanIStoreSeenInFolder() &&
    GetCanIExpungeFolder() &&
    GetCanIPostToFolder());
}

// Returns a newly allocated string describing these rights
nsresult nsMsgIMAPFolderACL::CreateACLRightsString(nsAString& aRightsString)
{
  nsString curRight;
  nsCOMPtr<nsIStringBundle> bundle;
  nsresult rv = IMAPGetStringBundle(getter_AddRefs(bundle));
  NS_ENSURE_SUCCESS(rv, rv);

  if (GetDoIHaveFullRightsForFolder())
    return bundle->GetStringFromID(IMAP_ACL_FULL_RIGHTS, getter_Copies(aRightsString));
  else
  {
    if (GetCanIReadFolder())
    {
      bundle->GetStringFromID(IMAP_ACL_READ_RIGHT, getter_Copies(curRight));
      aRightsString.Append(curRight);
    }
    if (GetCanIWriteFolder())
    {
      if (!aRightsString.IsEmpty()) aRightsString.AppendLiteral(", ");
      bundle->GetStringFromID(IMAP_ACL_WRITE_RIGHT, getter_Copies(curRight));
      aRightsString.Append(curRight);
    }
    if (GetCanIInsertInFolder())
    {
      if (!aRightsString.IsEmpty()) aRightsString.AppendLiteral(", ");
      bundle->GetStringFromID(IMAP_ACL_INSERT_RIGHT, getter_Copies(curRight));
      aRightsString.Append(curRight);
    }
    if (GetCanILookupFolder())
    {
      if (!aRightsString.IsEmpty()) aRightsString.AppendLiteral(", ");
      bundle->GetStringFromID(IMAP_ACL_LOOKUP_RIGHT, getter_Copies(curRight));
      aRightsString.Append(curRight);
    }
    if (GetCanIStoreSeenInFolder())
    {
      if (!aRightsString.IsEmpty()) aRightsString.AppendLiteral(", ");
      bundle->GetStringFromID(IMAP_ACL_SEEN_RIGHT, getter_Copies(curRight));
      aRightsString.Append(curRight);
    }
    if (GetCanIDeleteInFolder())
    {
      if (!aRightsString.IsEmpty()) aRightsString.AppendLiteral(", ");
      bundle->GetStringFromID(IMAP_ACL_DELETE_RIGHT, getter_Copies(curRight));
      aRightsString.Append(curRight);
    }
    if (GetCanIExpungeFolder())
    {
      if (!aRightsString.IsEmpty())
        aRightsString.AppendLiteral(", ");
      bundle->GetStringFromID(IMAP_ACL_EXPUNGE_RIGHT, getter_Copies(curRight));
      aRightsString.Append(curRight);
    }
    if (GetCanICreateSubfolder())
    {
      if (!aRightsString.IsEmpty()) aRightsString.AppendLiteral(", ");
      bundle->GetStringFromID(IMAP_ACL_CREATE_RIGHT, getter_Copies(curRight));
      aRightsString.Append(curRight);
    }
    if (GetCanIPostToFolder())
    {
      if (!aRightsString.IsEmpty()) aRightsString.AppendLiteral(", ");
      bundle->GetStringFromID(IMAP_ACL_POST_RIGHT, getter_Copies(curRight));
      aRightsString.Append(curRight);
    }
    if (GetCanIAdministerFolder())
    {
      if (!aRightsString.IsEmpty()) aRightsString.AppendLiteral(", ");
      bundle->GetStringFromID(IMAP_ACL_ADMINISTER_RIGHT, getter_Copies(curRight));
      aRightsString.Append(curRight);
    }
  }
  return rv;
}

NS_IMETHODIMP nsImapMailFolder::GetFilePath(nsILocalFile ** aPathName)
{
  nsresult rv;
  if (!mPath)
  {
    rv = nsImapURI2Path(kImapRootURI, mURI.get(), getter_AddRefs(mPath));
    if (NS_FAILED(rv)) return rv;
  }
  // this will return a copy of mPath, which is what we want.
  return nsMsgDBFolder::GetFilePath(aPathName);
}

NS_IMETHODIMP nsImapMailFolder::SetFilePath(nsILocalFile * aPathName)
{
  return nsMsgDBFolder::SetFilePath(aPathName);   // call base class so mPath will get set
}

nsresult nsImapMailFolder::DisplayStatusMsg(nsIImapUrl *aImapUrl, const nsAString& msg)
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
      progressSink->OnStatus(request, nsnull, NS_OK, PromiseFlatString(msg).get());      // XXX i18n message
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsImapMailFolder::ProgressStatus(nsIImapProtocol* aProtocol,
                                 PRUint32 aMsgId, const PRUnichar * extraInfo)
{
  nsString progressMsg;

  nsCOMPtr<nsIMsgIncomingServer> server;
  nsresult rv = GetServer(getter_AddRefs(server));
  if (NS_SUCCEEDED(rv) && server)
  {
    nsCOMPtr<nsIImapServerSink> serverSink = do_QueryInterface(server);
    if (serverSink)
      serverSink->GetImapStringByID(aMsgId, progressMsg);
  }
  if (progressMsg.IsEmpty())
    IMAPGetStringByID(aMsgId, getter_Copies(progressMsg));

  if (aProtocol && !progressMsg.IsEmpty())
  {
    nsCOMPtr <nsIImapUrl> imapUrl;
    aProtocol->GetRunningImapURL(getter_AddRefs(imapUrl));
    if (imapUrl)
    {
      if (extraInfo)
      {
        PRUnichar *printfString = nsTextFormatter::smprintf(progressMsg.get(), extraInfo);
        if (printfString)
          progressMsg.Adopt(printfString);
      }
      DisplayStatusMsg(imapUrl, progressMsg);
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsImapMailFolder::PercentProgress(nsIImapProtocol* aProtocol,
                                  const PRUnichar * aMessage,
                                  PRInt32 aCurrentProgress, PRInt32 aMaxProgress)
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
            // XXX handle 64-bit ints for real
            progressSink->OnProgress(request, nsnull,
                                     nsUint64(aCurrentProgress),
                                     nsUint64(aMaxProgress));
            if (aMessage)
              progressSink->OnStatus(request, nsnull, NS_OK, aMessage); // XXX i18n message
        }
      }
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsImapMailFolder::CopyNextStreamMessage(PRBool copySucceeded, nsISupports *copyState)
{
  //if copy has failed it could be either user interrupted it or for some other reason
  //don't do any subsequent copies or delete src messages if it is move
  if (!copySucceeded)
    return NS_OK;
  nsresult rv;
  nsCOMPtr<nsImapMailCopyState> mailCopyState = do_QueryInterface(copyState, &rv);
  if (NS_FAILED(rv)) return rv; // this can fail...

  if (!mailCopyState->m_streamCopy)
    return NS_OK;

  if (mailCopyState->m_curIndex < mailCopyState->m_totalCount)
  {
    mailCopyState->m_message = do_QueryElementAt(mailCopyState->m_messages,
                                                 mailCopyState->m_curIndex,
                                                 &rv);
    if (NS_SUCCEEDED(rv))
    {
      PRBool isRead;
      mailCopyState->m_message->GetIsRead(&isRead);
      mailCopyState->m_unreadCount = (isRead) ? 0 : 1;
      rv = CopyStreamMessage(mailCopyState->m_message,
                             this, mailCopyState->m_msgWindow, mailCopyState->m_isMove);
    }
  }
  else
  {
    // Notify of move/copy completion in case we have some source headers
    nsCOMPtr<nsIMsgFolderNotificationService> notifier(do_GetService(NS_MSGNOTIFICATIONSERVICE_CONTRACTID));
    if (notifier)
    {
      PRUint32 numHdrs;
      mailCopyState->m_messages->GetLength(&numHdrs);
      if (numHdrs)
        notifier->NotifyMsgsMoveCopyCompleted(mailCopyState->m_isMove, mailCopyState->m_messages, this);
    }


    if (mailCopyState->m_isMove)
    {
      nsCOMPtr<nsIMsgFolder> srcFolder(do_QueryInterface(mailCopyState->m_srcSupport, &rv));
      if (NS_SUCCEEDED(rv) && srcFolder)
      {
        srcFolder->DeleteMessages(mailCopyState->m_messages, nsnull,
          PR_TRUE, PR_TRUE, nsnull, PR_FALSE);
        // we want to send this notification after the source messages have
        // been deleted.
        nsCOMPtr<nsIMsgLocalMailFolder> popFolder(do_QueryInterface(srcFolder));
        if (popFolder)   //needed if move pop->imap to notify FE
          srcFolder->NotifyFolderEvent(mDeleteOrMoveMsgCompletedAtom);
      }
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
  {
    ProgressStatus(aProtocol, IMAP_DONE, nsnull);
    m_urlRunning = PR_FALSE;
    // if no protocol, then we're reading from the mem or disk cache
    // and we don't want to end the offline download just yet.
    if (aProtocol)
    {
      EndOfflineDownload();
      if (m_downloadingFolderForOfflineUse)
      {
        ReleaseSemaphore(static_cast<nsIMsgImapMailFolder*>(this));
        m_downloadingFolderForOfflineUse = PR_FALSE;
      }
    }
  }
  if (aUrl)
      return aUrl->SetUrlState(isRunning, statusCode);
  return statusCode;
}

// used when copying from local mail folder, or other imap server)
nsresult
nsImapMailFolder::CopyMessagesWithStream(nsIMsgFolder* srcFolder,
                                nsIArray* messages,
                                PRBool isMove,
                                PRBool isCrossServerOp,
                                nsIMsgWindow *msgWindow,
                                nsIMsgCopyServiceListener* listener,
                                PRBool allowUndo)
{
  NS_ENSURE_ARG_POINTER(srcFolder);
  NS_ENSURE_ARG_POINTER(messages);
  nsresult rv;
  nsCOMPtr<nsISupports> aSupport(do_QueryInterface(srcFolder, &rv));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = InitCopyState(aSupport, messages, isMove, PR_FALSE, isCrossServerOp,
    /* new message flags, not used */0, listener, msgWindow, allowUndo);
  if(NS_FAILED(rv))
    return rv;

  m_copyState->m_streamCopy = PR_TRUE;

  // ** jt - needs to create server to server move/copy undo msg txn
  if (m_copyState->m_allowUndo)
  {
    nsCAutoString messageIds;
    nsTArray<nsMsgKey> srcKeyArray;
    nsCOMPtr<nsIUrlListener> urlListener;
    rv = QueryInterface(NS_GET_IID(nsIUrlListener), getter_AddRefs(urlListener));
    rv = BuildIdsAndKeyArray(messages, messageIds, srcKeyArray);

    nsImapMoveCopyMsgTxn* undoMsgTxn = new nsImapMoveCopyMsgTxn;

    if (!undoMsgTxn || NS_FAILED(undoMsgTxn->Init(srcFolder, &srcKeyArray, messageIds.get(), this,
                                PR_TRUE, isMove, m_thread, urlListener)))
    {
      delete undoMsgTxn;
      return NS_ERROR_OUT_OF_MEMORY;
    }
    if (isMove)
    {
      if (mFlags & nsMsgFolderFlags::Trash)
        undoMsgTxn->SetTransactionType(nsIMessenger::eDeleteMsg);
      else
        undoMsgTxn->SetTransactionType(nsIMessenger::eMoveMsg);
    }
    else
      undoMsgTxn->SetTransactionType(nsIMessenger::eCopyMsg);
    rv = undoMsgTxn->QueryInterface(NS_GET_IID(nsImapMoveCopyMsgTxn), getter_AddRefs(m_copyState->m_undoMsgTxn));
  }
  nsCOMPtr<nsIMsgDBHdr> msg;
  msg = do_QueryElementAt(messages, 0, &rv);
  if (NS_SUCCEEDED(rv))
    CopyStreamMessage(msg, this, msgWindow, isMove);
  return rv; //we are clearing copy state in CopyMessages on failure
}

nsresult nsImapMailFolder::GetClearedOriginalOp(nsIMsgOfflineImapOperation *op, nsIMsgOfflineImapOperation **originalOp, nsIMsgDatabase **originalDB)
{
  nsCOMPtr<nsIMsgOfflineImapOperation> returnOp;
  nsOfflineImapOperationType opType;
  op->GetOperation(&opType);
  NS_ASSERTION(opType & nsIMsgOfflineImapOperation::kMoveResult, "not an offline move op");

  nsCString sourceFolderURI;
  op->GetSourceFolderURI(getter_Copies(sourceFolderURI));

  nsCOMPtr<nsIRDFResource> res;
  nsresult rv;
  nsCOMPtr<nsIRDFService> rdf(do_GetService(kRDFServiceCID, &rv));
  NS_ENSURE_SUCCESS(rv, rv);
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
          rv = (*originalDB)->GetOfflineOpForKey(originalKey, PR_FALSE, getter_AddRefs(returnOp));
          if (NS_SUCCEEDED(rv) && returnOp)
          {
            nsCString moveDestination;
            nsCString thisFolderURI;
            GetURI(thisFolderURI);
            returnOp->GetDestinationFolderURI(getter_Copies(moveDestination));
            if (moveDestination.Equals(thisFolderURI))
              returnOp->ClearOperation(nsIMsgOfflineImapOperation::kMoveResult);
          }
        }
      }
    }
  }
  returnOp.swap(*originalOp);
  return rv;
}

nsresult nsImapMailFolder::GetOriginalOp(nsIMsgOfflineImapOperation *op, nsIMsgOfflineImapOperation **originalOp, nsIMsgDatabase **originalDB)
{
  nsCOMPtr<nsIMsgOfflineImapOperation> returnOp;
  nsCString sourceFolderURI;
  op->GetSourceFolderURI(getter_Copies(sourceFolderURI));

  nsCOMPtr<nsIRDFResource> res;
  nsresult rv;

  nsCOMPtr<nsIRDFService> rdf(do_GetService(kRDFServiceCID, &rv));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = rdf->GetResource(sourceFolderURI, getter_AddRefs(res));
  if (NS_SUCCEEDED(rv))
  {
    nsCOMPtr<nsIMsgFolder> sourceFolder(do_QueryInterface(res, &rv));
    NS_ENSURE_SUCCESS(rv, rv);
    nsCOMPtr <nsIDBFolderInfo> folderInfo;
    sourceFolder->GetDBFolderInfoAndDB(getter_AddRefs(folderInfo), originalDB);
    if (*originalDB)
    {
      nsMsgKey originalKey;
      op->GetMessageKey(&originalKey);
      rv = (*originalDB)->GetOfflineOpForKey(originalKey, PR_FALSE, getter_AddRefs(returnOp));
    }
  }
  returnOp.swap(*originalOp);
  return rv;
}

nsresult nsImapMailFolder::CopyOfflineMsgBody(nsIMsgFolder *srcFolder, nsIMsgDBHdr *destHdr, nsIMsgDBHdr *origHdr)
{
  nsCOMPtr<nsIOutputStream> outputStream;
  nsresult rv = GetOfflineStoreOutputStream(getter_AddRefs(outputStream));
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr <nsISeekableStream> seekable (do_QueryInterface(outputStream, &rv));
  NS_ENSURE_SUCCESS(rv, rv);
  nsMsgKey messageOffset;
  PRUint32 messageSize;
  origHdr->GetMessageOffset(&messageOffset);
  origHdr->GetOfflineMessageSize(&messageSize);
  if (!messageSize)
  {
    nsCOMPtr<nsIMsgLocalMailFolder> localFolder = do_QueryInterface(srcFolder);
    if (localFolder)   //can just use regular message size
      origHdr->GetMessageSize(&messageSize);
  }
  PRInt64 tellPos;
  seekable->Tell(&tellPos);
  nsInt64 curStorePos = tellPos;
  destHdr->SetMessageOffset((PRUint32) curStorePos);
  nsCOMPtr <nsIInputStream> offlineStoreInputStream;
  rv = srcFolder->GetOfflineStoreInputStream(getter_AddRefs(offlineStoreInputStream));
  if (NS_SUCCEEDED(rv) && offlineStoreInputStream)
  {
    nsCOMPtr<nsISeekableStream> seekStream = do_QueryInterface(offlineStoreInputStream);
    NS_ASSERTION(seekStream, "non seekable stream - can't read from offline msg");
    if (seekStream)
    {
      rv = seekStream->Seek(nsISeekableStream::NS_SEEK_SET, messageOffset);
      if (NS_SUCCEEDED(rv))
      {
        // now, copy the dest folder offline store msg to the temp file
        PRInt32 inputBufferSize = 10240;
        char *inputBuffer = (char *) PR_Malloc(inputBufferSize);
        PRInt32 bytesLeft;
        PRUint32 bytesRead, bytesWritten;
        bytesLeft = messageSize;
        rv = (inputBuffer) ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
        while (bytesLeft > 0 && NS_SUCCEEDED(rv))
        {
          rv = offlineStoreInputStream->Read(inputBuffer, inputBufferSize, &bytesRead);
          if (NS_SUCCEEDED(rv) && bytesRead > 0)
          {
            rv = outputStream->Write(inputBuffer, PR_MIN((PRInt32) bytesRead, bytesLeft), &bytesWritten);
            NS_ASSERTION((PRInt32) bytesWritten == PR_MIN((PRInt32) bytesRead, bytesLeft), "wrote out incorrect number of bytes");
          }
          else
            break;
          bytesLeft -= bytesRead;
        }
        PR_FREEIF(inputBuffer);
        outputStream->Flush();
      }
    }
  }
  if (NS_SUCCEEDED(rv))
  {
    PRUint32 resultFlags;
    destHdr->OrFlags(MSG_FLAG_OFFLINE, &resultFlags);
    destHdr->SetOfflineMessageSize(messageSize);
  }
  return rv;
}

// this imap folder is the destination of an offline move/copy.
// We are either offline, or doing a pseudo-offline delete (where we do an offline
// delete, load the next message, then playback the offline delete).
nsresult nsImapMailFolder::CopyMessagesOffline(nsIMsgFolder* srcFolder,
                               nsIArray* messages,
                               PRBool isMove,
                               nsIMsgWindow *msgWindow,
                               nsIMsgCopyServiceListener* listener)
{
  NS_ENSURE_ARG(messages);
  nsresult rv;
  nsresult stopit = 0;
  nsCOMPtr <nsIMsgDatabase> sourceMailDB;
  nsCOMPtr <nsIDBFolderInfo> srcDbFolderInfo;
  srcFolder->GetDBFolderInfoAndDB(getter_AddRefs(srcDbFolderInfo), getter_AddRefs(sourceMailDB));
  PRBool deleteToTrash = PR_FALSE;
  PRBool deleteImmediately = PR_FALSE;
  PRUint32 srcCount;
  messages->GetLength(&srcCount);
  nsCOMPtr<nsIImapIncomingServer> imapServer;
  rv = GetImapIncomingServer(getter_AddRefs(imapServer));
  nsCOMPtr<nsIMutableArray> msgHdrsCopied(do_CreateInstance(NS_ARRAY_CONTRACTID));
  nsCString messageIds;

  if (NS_SUCCEEDED(rv) && imapServer)
  {
    nsMsgImapDeleteModel deleteModel;
    imapServer->GetDeleteModel(&deleteModel);
    deleteToTrash = (deleteModel == nsMsgImapDeleteModels::MoveToTrash);
    deleteImmediately = (deleteModel == nsMsgImapDeleteModels::DeleteNoTrash);
  }

  // This array is used only when we are actually removing the messages from the
  // source database.
  nsTArray<nsMsgKey> keysToDelete((isMove && (deleteToTrash || deleteImmediately)) ? srcCount : 0);

  if (sourceMailDB)
  {
    // save the future ops in the source DB, if this is not a imap->local copy/move
    nsCOMPtr <nsITransactionManager> txnMgr;
    if (msgWindow)
      msgWindow->GetTransactionManager(getter_AddRefs(txnMgr));
    if (txnMgr)
      txnMgr->BeginBatch();
    GetDatabase(nsnull);
    if (mDatabase)
    {
      // get the highest key in the dest db, so we can make up our fake keys
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
        PRBool messageReturningHome = PR_FALSE;
        nsCString originalSrcFolderURI;
        srcFolder->GetURI(originalSrcFolderURI);
        nsCOMPtr<nsIMsgDBHdr> message;
        message = do_QueryElementAt(messages, sourceKeyIndex);
        nsMsgKey originalKey;
        if (message)
          rv = message->GetMessageKey(&originalKey);
        else
        {
          NS_ASSERTION(PR_FALSE, "bad msg in src array");
          continue;
        }
        nsCOMPtr <nsIMsgOfflineImapOperation> sourceOp;
        rv = sourceMailDB->GetOfflineOpForKey(originalKey, PR_TRUE, getter_AddRefs(sourceOp));
        if (NS_SUCCEEDED(rv) && sourceOp)
        {
          srcFolder->SetFlag(nsMsgFolderFlags::OfflineEvents);
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
              nsCString srcFolderURI;
              srcFolder->GetURI(srcFolderURI);
              sourceOp->GetSourceFolderURI(getter_Copies(originalSrcFolderURI));
              sourceOp->GetMessageKey(&originalKey);
              if (isMove)
                sourceMailDB->RemoveOfflineOp(sourceOp);
              sourceOp = originalOp;
              if (originalSrcFolderURI.Equals(srcFolderURI))
              {
                messageReturningHome = PR_TRUE;
                originalDB->RemoveOfflineOp(originalOp);
              }
            }
          }
          if (!messageReturningHome)
          {
            nsCString folderURI;
            GetURI(folderURI);
            if (isMove)
            {
              PRUint32 msgSize;
              PRUint32 msgFlags;
              imapMessageFlagsType newImapFlags = 0;
              message->GetMessageSize(&msgSize);
              message->GetFlags(&msgFlags);
              sourceOp->SetDestinationFolderURI(folderURI.get()); // offline move
              sourceOp->SetOperation(nsIMsgOfflineImapOperation::kMsgMoved);
              sourceOp->SetMsgSize(msgSize);
              newImapFlags = msgFlags & 0x7;
              if (msgFlags & MSG_FLAG_FORWARDED)
                newImapFlags |=  kImapMsgForwardedFlag;
              sourceOp->SetNewFlags(newImapFlags);
            }
            else
              sourceOp->AddMessageCopyOperation(folderURI.get()); // offline copy

            nsTArray<nsMsgKey> srcKeyArray;
            nsCOMPtr<nsIUrlListener> urlListener;

            messageIds.Truncate();
            rv = BuildIdsAndKeyArray(messages, messageIds, srcKeyArray);
            sourceOp->GetOperation(&opType);
            srcKeyArray.AppendElement(originalKey);
            rv = QueryInterface(NS_GET_IID(nsIUrlListener), getter_AddRefs(urlListener));
            nsImapOfflineTxn *undoMsgTxn = new
              nsImapOfflineTxn(srcFolder, &srcKeyArray, messageIds.get(), this, 
                               isMove, opType, message, m_thread, urlListener);
            if (undoMsgTxn)
            {
              if (isMove)
              {
                undoMsgTxn->SetTransactionType(nsIMessenger::eMoveMsg);
                nsCOMPtr<nsIMsgImapMailFolder> srcIsImap(do_QueryInterface(srcFolder));
                // remember this undo transaction so we can hook up the result
                // msg ids in the undo transaction.
                if (srcIsImap)
                {
                  nsImapMailFolder *srcImapFolder = static_cast<nsImapMailFolder*>(srcFolder);
                  srcImapFolder->m_pendingOfflineMoves.AppendObject(undoMsgTxn);
                }
              }
              else
                undoMsgTxn->SetTransactionType(nsIMessenger::eCopyMsg);
              // we're adding this undo action before the delete is successful. This is evil,
              // but 4.5 did it as well.
              if (txnMgr)
                txnMgr->DoTransaction(undoMsgTxn);
            }
          }
          PRBool hasMsgOffline = PR_FALSE;
          srcFolder->HasMsgOffline(originalKey, &hasMsgOffline);
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

          nsCOMPtr <nsIMsgDBHdr> newMailHdr;
          rv = mDatabase->CopyHdrFromExistingHdr(fakeBase + sourceKeyIndex, mailHdr,
            PR_TRUE, getter_AddRefs(newMailHdr));
          if (!newMailHdr || NS_FAILED(rv))
          {
            NS_ASSERTION(PR_FALSE, "failed to copy hdr");
            stopit = rv;
          }

          if (NS_SUCCEEDED(stopit))
          {
            PRBool hasMsgOffline = PR_FALSE;
            srcFolder->HasMsgOffline(originalKey, &hasMsgOffline);
            if (hasMsgOffline)
              CopyOfflineMsgBody(srcFolder, newMailHdr, mailHdr);

            nsCOMPtr <nsIMsgOfflineImapOperation> destOp;
            mDatabase->GetOfflineOpForKey(fakeBase + sourceKeyIndex, PR_TRUE, getter_AddRefs(destOp));
            if (destOp)
            {
              // check if this is a move back to the original mailbox, in which case
              // we just delete the offline operation.
              if (messageReturningHome)
                mDatabase->RemoveOfflineOp(destOp);
              else
              {
                SetFlag(nsMsgFolderFlags::OfflineEvents);
                destOp->SetSourceFolderURI(originalSrcFolderURI.get());
                destOp->SetMessageKey(originalKey);
                {
                  nsCOMPtr<nsIUrlListener> urlListener;
                  QueryInterface(NS_GET_IID(nsIUrlListener), getter_AddRefs(urlListener));
                  nsTArray<nsMsgKey> keyArray;
                  keyArray.AppendElement(fakeBase + sourceKeyIndex);
                  nsImapOfflineTxn *undoMsgTxn = new
                    nsImapOfflineTxn(this, &keyArray, nsnull, this, isMove, nsIMsgOfflineImapOperation::kAddedHeader,
                      newMailHdr, m_thread, urlListener);
                  if (undoMsgTxn && txnMgr)
                     txnMgr->DoTransaction(undoMsgTxn);
                }
              }
            }
            else
              stopit = NS_ERROR_FAILURE;
          }
          successfulCopy = NS_SUCCEEDED(stopit);
          nsMsgKey msgKey;
          mailHdr->GetMessageKey(&msgKey);
          if (isMove && successfulCopy)
          {
            nsTArray<nsMsgKey> srcKeyArray;
            srcKeyArray.AppendElement(msgKey);
            nsCOMPtr<nsIUrlListener> urlListener;
            rv = QueryInterface(NS_GET_IID(nsIUrlListener), getter_AddRefs(urlListener));
            nsOfflineImapOperationType opType = nsIMsgOfflineImapOperation::kDeletedMsg;
            if (!deleteToTrash)
              opType = nsIMsgOfflineImapOperation::kMsgMarkedDeleted;
            srcKeyArray.AppendElement(msgKey);
            nsImapOfflineTxn *undoMsgTxn = new
              nsImapOfflineTxn(srcFolder, &srcKeyArray, messageIds.get(), this, isMove, opType, mailHdr,
                m_thread, urlListener);
            if (undoMsgTxn)
            {
              if (isMove)
              {
                if (mFlags & nsMsgFolderFlags::Trash)
                  undoMsgTxn->SetTransactionType(nsIMessenger::eDeleteMsg);
                else
                  undoMsgTxn->SetTransactionType(nsIMessenger::eMoveMsg);
              }
              else
                undoMsgTxn->SetTransactionType(nsIMessenger::eCopyMsg);
              if (txnMgr)
                 txnMgr->DoTransaction(undoMsgTxn);
            }
            if (deleteToTrash || deleteImmediately)
              keysToDelete.AppendElement(msgKey);
            else
              sourceMailDB->MarkImapDeleted(msgKey, PR_TRUE, nsnull); // offline delete
          }
          if (successfulCopy)
            // This is for both moves and copies
            msgHdrsCopied->AppendElement(mailHdr, PR_FALSE);
        }
      }

      if (isMove)
        sourceMailDB->Commit(nsMsgDBCommitType::kLargeCommit);
      mDatabase->Commit(nsMsgDBCommitType::kLargeCommit);
      SummaryChanged();
      srcFolder->SummaryChanged();
    }
    if (txnMgr)
      txnMgr->EndBatch();
  }

  // Do this before delete, as it destroys the messages
  PRUint32 numHdrs;
  msgHdrsCopied->GetLength(&numHdrs);
  if (numHdrs)
  {
    nsCOMPtr<nsIMsgFolderNotificationService> notifier(do_GetService(NS_MSGNOTIFICATIONSERVICE_CONTRACTID));
    if (notifier)
      notifier->NotifyMsgsMoveCopyCompleted(isMove, msgHdrsCopied, this);
  }

  if (isMove && (deleteToTrash || deleteImmediately))
    sourceMailDB->DeleteMessages(&keysToDelete, nsnull);

  nsCOMPtr<nsISupports> srcSupport = do_QueryInterface(srcFolder);
  OnCopyCompleted(srcSupport, rv);

  if (NS_SUCCEEDED(rv) && isMove)
    srcFolder->NotifyFolderEvent(mDeleteOrMoveMsgCompletedAtom);
  return rv;
}

NS_IMETHODIMP
nsImapMailFolder::CopyMessages(nsIMsgFolder* srcFolder,
                               nsIArray* messages,
                               PRBool isMove,
                               nsIMsgWindow *msgWindow,
                               nsIMsgCopyServiceListener* listener,
                               PRBool isFolder, //isFolder for future use when we do cross-server folder move/copy
                               PRBool allowUndo)
{
  if (!(mFlags & (nsMsgFolderFlags::Trash|nsMsgFolderFlags::Junk)))
    SetMRUTime();

  nsresult rv;
  nsCOMPtr <nsIMsgIncomingServer> srcServer;
  nsCOMPtr <nsIMsgIncomingServer> dstServer;
  nsCOMPtr<nsISupports> srcSupport = do_QueryInterface(srcFolder);
  PRBool sameServer = PR_FALSE;

  rv = srcFolder->GetServer(getter_AddRefs(srcServer));
  if(NS_FAILED(rv)) goto done;

  rv = GetServer(getter_AddRefs(dstServer));
  if(NS_FAILED(rv)) goto done;

  NS_ENSURE_TRUE(dstServer, NS_ERROR_NULL_POINTER);
  
  rv = dstServer->Equals(srcServer, &sameServer);
  if (NS_FAILED(rv)) goto done;

  // in theory, if allowUndo is true, then this is a user initiated
  // action, and we should do it pseudo-offline. If it's not
  // user initiated (e.g., mail filters firing), then allowUndo is
  // false, and we should just do the action.
  if (!WeAreOffline() && sameServer && allowUndo) 
  {
    // complete the copy operation as in offline mode
    rv = CopyMessagesOffline(srcFolder, messages, isMove, msgWindow, listener);
    
    NS_ENSURE_SUCCESS(rv,rv);
  
    // XXX ebirol
    // We make sure that the source folder is an imap folder by limiting pseudo-offline 
    // operations to the same imap server. If we extend the code to cover non imap folders 
    // in the future (i.e. imap folder->local folder), then the following downcast
    // will cause either a crash or compiler error. Do not forget to change it accordingly.
    nsImapMailFolder *srcImapFolder = static_cast<nsImapMailFolder*>(srcFolder);
    
    // lazily create playback timer if it is not already
    // created
    if (!srcImapFolder->m_playbackTimer) 
    {
      rv = srcImapFolder->CreatePlaybackTimer();
      NS_ENSURE_SUCCESS(rv,rv);
    }
    
    if (srcImapFolder->m_playbackTimer) 
    {
      // if there is no pending request, create a new one, and set the timer. Otherwise
      // use the existing one to reset the timer.
      // it is callback function's responsibility to delete the new request object
      if (!srcImapFolder->m_pendingPlaybackReq) 
      {
        srcImapFolder->m_pendingPlaybackReq = new nsPlaybackRequest(srcImapFolder, msgWindow);
        if (!srcImapFolder->m_pendingPlaybackReq)
          return NS_ERROR_OUT_OF_MEMORY;
      }
              
      srcImapFolder->m_playbackTimer->InitWithFuncCallback(PlaybackTimerCallback, (void *) srcImapFolder->m_pendingPlaybackReq, 
                                        PLAYBACK_TIMER_INTERVAL_IN_MS, nsITimer::TYPE_ONE_SHOT);
    }
    return rv;
  }
  else 
  {
    nsCAutoString messageIds;
    nsTArray<nsMsgKey> srcKeyArray;
    nsCOMPtr<nsIUrlListener> urlListener;
    nsCOMPtr<nsISupports> copySupport;
    PRUint32 count = 0; // needs to be inited before goto
    
    if (WeAreOffline())
      return CopyMessagesOffline(srcFolder, messages, isMove, msgWindow, listener);
    
    nsCOMPtr<nsIImapService> imapService = do_GetService(NS_IMAPSERVICE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv,rv);
    
  PRUint32 supportedUserFlags;
  GetSupportedUserFlags(&supportedUserFlags);

  PRUint32 i;

  rv = messages->GetLength(&count);
  if (NS_FAILED(rv)) return rv;

  // make sure database is open to set special flags below
  if (!mDatabase)
    GetDatabase(nsnull);

  // check if any msg hdr has special flags or properties set
  // that we need to set on the dest hdr
  for (i = 0; i < count; i++)
  {
    nsCOMPtr <nsIMsgDBHdr> msgDBHdr = do_QueryElementAt(messages, i, &rv);
    if (mDatabase && msgDBHdr)
    {
      if (!(supportedUserFlags & kImapMsgSupportUserFlag))
      {
        nsMsgLabelValue label;
        nsCString junkScore;
        msgDBHdr->GetStringProperty("junkscore", getter_Copies(junkScore));
        if (!junkScore.IsEmpty()) // ignore already scored messages.
          mDatabase->SetAttributesOnPendingHdr(msgDBHdr, "junkscore", junkScore.get(), 0);
        msgDBHdr->GetLabel(&label);
        if (label != 0)
        {
          nsCAutoString labelStr;
          labelStr.AppendInt(label);
          mDatabase->SetAttributesOnPendingHdr(msgDBHdr, "label", labelStr.get(), 0);
        }
        nsCString keywords;
        msgDBHdr->GetStringProperty("keywords", getter_Copies(keywords));
        if (!keywords.IsEmpty())
          mDatabase->SetAttributesOnPendingHdr(msgDBHdr, "keywords", keywords.get(), 0);
      } 
      // do this even if the server supports user-defined flags.
      
      nsCString junkScoreOrigin, junkPercent;
      msgDBHdr->GetStringProperty("junkscoreorigin", getter_Copies(junkScoreOrigin));
      msgDBHdr->GetStringProperty("junkpercent", getter_Copies(junkPercent));
      if (!junkScoreOrigin.IsEmpty())
        mDatabase->SetAttributesOnPendingHdr(msgDBHdr, "junkscoreorigin", junkScoreOrigin.get(), 0);
      if (!junkPercent.IsEmpty())
        mDatabase->SetAttributesOnPendingHdr(msgDBHdr, "junkpercent", junkPercent.get(), 0);             
      
      nsMsgPriorityValue priority;
      msgDBHdr->GetPriority(&priority);
      if(priority != 0)
      {
        nsCAutoString priorityStr;
        priorityStr.AppendInt(priority);
        mDatabase->SetAttributesOnPendingHdr(msgDBHdr, "priority", priorityStr.get(), 0);
      }
    }
  }

  // if the folders aren't on the same server, do a stream base copy
  if (!sameServer)
  {
    rv = CopyMessagesWithStream(srcFolder, messages, isMove, PR_TRUE, msgWindow, listener, allowUndo);
    goto done;
  }

  rv = BuildIdsAndKeyArray(messages, messageIds, srcKeyArray);
  if(NS_FAILED(rv)) goto done;

  rv = QueryInterface(NS_GET_IID(nsIUrlListener), getter_AddRefs(urlListener));
  rv = InitCopyState(srcSupport, messages, isMove, PR_TRUE, PR_FALSE,
    /* newMsgFlags, not used */0, listener, msgWindow, allowUndo);
  if (NS_FAILED(rv)) goto done;

  m_copyState->m_curIndex = m_copyState->m_totalCount;

  if (isMove)
    srcFolder->EnableNotifications(allMessageCountNotifications, PR_FALSE, PR_TRUE/* dbBatching*/);  //disable message count notification

  copySupport = do_QueryInterface(m_copyState);
  rv = imapService->OnlineMessageCopy(m_thread,
                                      srcFolder, messageIds,
                                      this, PR_TRUE, isMove,
                                      urlListener, nsnull,
                                      copySupport, msgWindow);
  if (NS_SUCCEEDED(rv) && m_copyState->m_allowUndo)
  {
    nsImapMoveCopyMsgTxn* undoMsgTxn = new nsImapMoveCopyMsgTxn;
    if (!undoMsgTxn || NS_FAILED(undoMsgTxn->Init(srcFolder, &srcKeyArray,
                                 messageIds.get(), this,
                                 PR_TRUE, isMove, m_thread, urlListener)))
    {
      delete undoMsgTxn;
      return NS_ERROR_OUT_OF_MEMORY;
    }
    if (isMove)
    {
      if (mFlags & nsMsgFolderFlags::Trash)
        undoMsgTxn->SetTransactionType(nsIMessenger::eDeleteMsg);
      else
        undoMsgTxn->SetTransactionType(nsIMessenger::eMoveMsg);
    }
    else
      undoMsgTxn->SetTransactionType(nsIMessenger::eCopyMsg);
    rv = undoMsgTxn->QueryInterface(NS_GET_IID(nsImapMoveCopyMsgTxn), getter_AddRefs(m_copyState->m_undoMsgTxn) );
  }

  }//endif
  
done:
  if (NS_FAILED(rv))
  {
    (void) OnCopyCompleted(srcSupport, PR_FALSE);
    if (isMove)
    {
      srcFolder->EnableNotifications(allMessageCountNotifications, PR_TRUE, PR_TRUE/* dbBatching*/);  //enable message count notification
      NotifyFolderEvent(mDeleteOrMoveMsgFailedAtom);
    }
  }
  return rv;
}

class nsImapFolderCopyState : public nsIUrlListener, public nsIMsgCopyServiceListener
{
public:
  nsImapFolderCopyState(nsIMsgFolder *destParent, nsIMsgFolder *srcFolder,
                    PRBool isMoveFolder, nsIMsgWindow *msgWindow, nsIMsgCopyServiceListener *listener);
  ~nsImapFolderCopyState();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIURLLISTENER
  NS_DECL_NSIMSGCOPYSERVICELISTENER

  nsresult StartNextCopy();
  nsresult AdvanceToNextFolder(nsresult aStatus);
protected:
  nsCOMPtr <nsIMsgFolder> m_destParent;
  nsCOMPtr <nsIMsgFolder> m_srcFolder;
  PRBool                  m_isMoveFolder;
  nsCOMPtr <nsIMsgCopyServiceListener> m_copySrvcListener;
  nsCOMPtr <nsIMsgWindow> m_msgWindow;
  PRInt32                 m_childIndex;
  nsCOMPtr <nsISupportsArray> m_srcChildFolders;
  nsCOMPtr <nsISupportsArray> m_destParents;

};

NS_IMPL_ISUPPORTS2(nsImapFolderCopyState, nsIUrlListener, nsIMsgCopyServiceListener)

nsImapFolderCopyState::nsImapFolderCopyState(nsIMsgFolder *destParent, nsIMsgFolder *srcFolder,
                                             PRBool isMoveFolder, nsIMsgWindow *msgWindow, nsIMsgCopyServiceListener *listener)
{
  m_destParent = destParent;
  m_srcFolder = srcFolder;
  m_isMoveFolder = isMoveFolder;
  m_msgWindow = msgWindow;
  m_copySrvcListener = listener;
  m_childIndex = -1;
  m_srcChildFolders = do_CreateInstance(NS_SUPPORTSARRAY_CONTRACTID);
  m_destParents = do_CreateInstance(NS_SUPPORTSARRAY_CONTRACTID);
}

nsImapFolderCopyState::~nsImapFolderCopyState()
{
}

nsresult
nsImapFolderCopyState::StartNextCopy()
{
  nsresult rv;
  // first make sure dest folder exists.
  nsCOMPtr <nsIImapService> imapService = do_GetService (NS_IMAPSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  nsString folderName;
  m_srcFolder->GetName(folderName);

  return imapService->EnsureFolderExists(NS_GetCurrentThread(),
                                       m_destParent,
                                       folderName,
                                       this, nsnull);
}

nsresult nsImapFolderCopyState::AdvanceToNextFolder(nsresult aStatus)
{
  nsresult rv;
  m_childIndex++;
  PRUint32 childCount = 0;
  if (m_srcChildFolders)
    m_srcChildFolders->Count(&childCount);
  if (m_childIndex >= childCount)
  {
    if (m_copySrvcListener)
      rv = m_copySrvcListener->OnStopCopy(aStatus);
    Release();
  }
  else
  {
    m_destParent = do_QueryElementAt(m_destParents, m_childIndex, &rv);
    m_srcFolder = do_QueryElementAt(m_srcChildFolders, m_childIndex, &rv);
    rv = StartNextCopy();
  }
  return rv;
}

NS_IMETHODIMP
nsImapFolderCopyState::OnStartRunningUrl(nsIURI *aUrl)
{
  NS_PRECONDITION(aUrl, "sanity check - need to be be running non-null url");
  return NS_OK;
}

NS_IMETHODIMP
nsImapFolderCopyState::OnStopRunningUrl(nsIURI *aUrl, nsresult aExitCode)
{
  if (NS_FAILED(aExitCode))
  {
    if (m_copySrvcListener)
      m_copySrvcListener->OnStopCopy(aExitCode);
    Release();
    return aExitCode; // or NS_OK???
  }
  nsresult rv = NS_OK;
  if (aUrl)
  {
    nsCOMPtr<nsIImapUrl> imapUrl = do_QueryInterface(aUrl);
    if (imapUrl)
    {
      nsImapAction imapAction = nsIImapUrl::nsImapTest;
      imapUrl->GetImapAction(&imapAction);

      switch(imapAction)
      {
        case nsIImapUrl::nsImapEnsureExistsFolder:
        {
          nsCOMPtr<nsIMsgFolder> newMsgFolder;
          nsString folderName;
          nsCString utf7LeafName;
          m_srcFolder->GetName(folderName);
          rv = CopyUTF16toMUTF7(folderName, utf7LeafName);
          rv = m_destParent->FindSubFolder(utf7LeafName, getter_AddRefs(newMsgFolder));
          NS_ENSURE_SUCCESS(rv,rv);

          // check if the source folder has children. If it does, list them
          // into m_srcChildFolders, and set m_destParents for the
          // corresponding indexes to the newly created folder.
          nsCOMPtr<nsISimpleEnumerator> enumerator;
          rv = m_srcFolder->GetSubFolders(getter_AddRefs(enumerator));
          NS_ENSURE_SUCCESS(rv, rv);

          PRBool hasMore;
          PRUint32 childIndex = 0;
          while (NS_SUCCEEDED(enumerator->HasMoreElements(&hasMore)) && hasMore)
          {
            nsCOMPtr<nsISupports> child;
            rv = enumerator->GetNext(getter_AddRefs(child));
            if (NS_SUCCEEDED(rv))
            {
              m_srcChildFolders->InsertElementAt(child, m_childIndex + childIndex + 1);
              m_destParents->InsertElementAt(newMsgFolder, m_childIndex + childIndex + 1);
            }
            ++childIndex;
          }

          nsCOMPtr<nsISimpleEnumerator> messages;
          rv = m_srcFolder->GetMessages(m_msgWindow, getter_AddRefs(messages));
          nsCOMPtr<nsIMutableArray> msgArray(do_CreateInstance(NS_ARRAY_CONTRACTID, &rv));
          NS_ENSURE_TRUE(msgArray, rv);
          PRBool hasMoreElements;
          nsCOMPtr<nsISupports> aSupport;

          if (messages)
            messages->HasMoreElements(&hasMoreElements);

          if (!hasMoreElements)
            return AdvanceToNextFolder(NS_OK);

          while (hasMoreElements && NS_SUCCEEDED(rv))
          {
            rv = messages->GetNext(getter_AddRefs(aSupport));
            rv = msgArray->AppendElement(aSupport, PR_FALSE);
            messages->HasMoreElements(&hasMoreElements);
          }

          nsCOMPtr<nsIMsgCopyService> copyService = do_GetService(NS_MSGCOPYSERVICE_CONTRACTID, &rv);
          NS_ENSURE_SUCCESS(rv, rv);
          rv = copyService->CopyMessages(m_srcFolder,
                             msgArray, newMsgFolder,
                             m_isMoveFolder,
                             this,
                             m_msgWindow,
                             PR_FALSE /* allowUndo */);
        }
        break;
      }
    }
  }
  return rv;
}

NS_IMETHODIMP nsImapFolderCopyState::OnStartCopy()
{
  return NS_OK;
}

/* void OnProgress (in PRUint32 aProgress, in PRUint32 aProgressMax); */
NS_IMETHODIMP nsImapFolderCopyState::OnProgress(PRUint32 aProgress, PRUint32 aProgressMax)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* void SetMessageKey (in PRUint32 aKey); */
NS_IMETHODIMP nsImapFolderCopyState::SetMessageKey(PRUint32 aKey)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* [noscript] void GetMessageId (in nsCString aMessageId); */
NS_IMETHODIMP nsImapFolderCopyState::GetMessageId(nsACString& messageId)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* void OnStopCopy (in nsresult aStatus); */
NS_IMETHODIMP nsImapFolderCopyState::OnStopCopy(nsresult aStatus)
{
  if (NS_SUCCEEDED(aStatus))
    return AdvanceToNextFolder(aStatus);
  if (m_copySrvcListener)
    (void) m_copySrvcListener->OnStopCopy(aStatus);
  delete this;
  return NS_OK;
}

// "this" is the parent of the copied folder.
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
    PRUint32 folderFlags = 0;
    if (srcFolder)
      srcFolder->GetFlags(&folderFlags);

    // if our source folder is a virtual folder
    if (folderFlags & nsMsgFolderFlags::Virtual)
    {
      nsCOMPtr<nsIMsgFolder> newMsgFolder;
      nsString folderName;
      srcFolder->GetName(folderName);

      nsCAutoString tempSafeFolderName;
      LossyCopyUTF16toASCII(folderName, tempSafeFolderName);
      NS_MsgHashIfNecessary(tempSafeFolderName);

      nsAutoString safeFolderName;
      CopyASCIItoUTF16(tempSafeFolderName, safeFolderName);
      srcFolder->ForceDBClosed();

      nsCOMPtr<nsILocalFile> oldPathFile;
      rv = srcFolder->GetFilePath(getter_AddRefs(oldPathFile));
      NS_ENSURE_SUCCESS(rv,rv);

      nsCOMPtr <nsILocalFile> summaryFile;
      GetSummaryFileLocation(oldPathFile, getter_AddRefs(summaryFile));

      nsCOMPtr<nsILocalFile> newPathFile;
      rv = GetFilePath(getter_AddRefs(newPathFile));
      NS_ENSURE_SUCCESS(rv,rv);

      PRBool isDirectory = PR_FALSE;
      newPathFile->IsDirectory(&isDirectory);
      if (!isDirectory)
      {
        AddDirectorySeparator(newPathFile);
        rv = newPathFile->Create(nsIFile::DIRECTORY_TYPE, 0700);
        NS_ENSURE_SUCCESS(rv, rv);
      }

      rv = CheckIfFolderExists(folderName, this, msgWindow);
      if(NS_FAILED(rv))
        return rv;

      rv = summaryFile->CopyTo(newPathFile, NS_LITERAL_STRING(""));
      NS_ENSURE_SUCCESS(rv, rv);

      rv = AddSubfolder(safeFolderName, getter_AddRefs(newMsgFolder));
      NS_ENSURE_SUCCESS(rv, rv);

      newMsgFolder->SetPrettyName(folderName);

      PRUint32 flags;
      srcFolder->GetFlags(&flags);
      newMsgFolder->SetFlags(flags);

      NotifyItemAdded(newMsgFolder);

      // now remove the old folder
      nsCOMPtr<nsIMsgFolder> msgParent;
      srcFolder->GetParentMsgFolder(getter_AddRefs(msgParent));
      srcFolder->SetParent(nsnull);
      if (msgParent)
      {
        msgParent->PropagateDelete(srcFolder, PR_FALSE, msgWindow);  // The files have already been moved, so delete storage PR_FALSE
        oldPathFile->Remove(PR_FALSE);  //berkeley mailbox
        nsCOMPtr <nsIMsgDatabase> srcDB; // we need to force closed the source db
        srcFolder->Delete();

        nsCOMPtr<nsILocalFile> parentPathFile;
        rv = msgParent->GetFilePath(getter_AddRefs(parentPathFile));
        NS_ENSURE_SUCCESS(rv,rv);

        AddDirectorySeparator(parentPathFile);
        nsCOMPtr <nsISimpleEnumerator> children;
        parentPathFile->GetDirectoryEntries(getter_AddRefs(children));
        PRBool more;
        // checks if the directory is empty or not
        if (children && NS_SUCCEEDED(children->HasMoreElements(&more)) && !more)
          parentPathFile->Remove(PR_TRUE);
      }
    }
    else // non-virtual folder
    {
      nsCOMPtr <nsIImapService> imapService = do_GetService (NS_IMAPSERVICE_CONTRACTID, &rv);
      NS_ENSURE_SUCCESS(rv, rv);
      nsCOMPtr <nsIUrlListener> urlListener = do_QueryInterface(srcFolder);
      PRBool match = PR_FALSE;
      PRBool confirmed = PR_FALSE;
      if (mFlags & nsMsgFolderFlags::Trash)
      {
        rv = srcFolder->MatchOrChangeFilterDestination(nsnull, PR_FALSE, &match);
        if (match)
        {
          srcFolder->ConfirmFolderDeletionForFilter(msgWindow, &confirmed);
          // should we return an error to copy service?
          // or send a notification?
          if (!confirmed)
            return NS_OK;
        }
      }
      rv = imapService->MoveFolder(m_thread,
                                   srcFolder,
                                   this,
                                   urlListener,
                                   msgWindow,
                                   nsnull);
    }
  }
  else // copying folder (should only be across server?)
  {
    nsImapFolderCopyState *folderCopier = new nsImapFolderCopyState(this, srcFolder, isMoveFolder, msgWindow, listener);
    NS_ADDREF(folderCopier); // it owns itself.
    return folderCopier->StartNextCopy();
  }
  return rv;
}

NS_IMETHODIMP
nsImapMailFolder::CopyFileMessage(nsIFile* file,
                                  nsIMsgDBHdr* msgToReplace,
                                  PRBool isDraftOrTemplate,
                                  PRUint32 aNewMsgFlags,
                                  nsIMsgWindow *msgWindow,
                                  nsIMsgCopyServiceListener* listener)
{
    nsresult rv = NS_ERROR_NULL_POINTER;
    nsMsgKey key = 0xffffffff;
    nsCAutoString messageId;
    nsCOMPtr<nsIUrlListener> urlListener;
    nsCOMPtr<nsIMutableArray> messages(do_CreateInstance(NS_ARRAY_CONTRACTID));
    nsCOMPtr<nsISupports> srcSupport = do_QueryInterface(file, &rv);

    if (!messages)
      return OnCopyCompleted(srcSupport, rv);

    nsCOMPtr<nsIImapService> imapService = do_GetService(NS_IMAPSERVICE_CONTRACTID, &rv);
    if (NS_FAILED(rv))
      return OnCopyCompleted(srcSupport, rv);

    rv = QueryInterface(NS_GET_IID(nsIUrlListener), getter_AddRefs(urlListener));

    if (msgToReplace)
    {
        rv = msgToReplace->GetMessageKey(&key);
        if (NS_SUCCEEDED(rv))
            messageId.AppendInt((PRInt32) key);
    }

    rv = InitCopyState(srcSupport, messages, PR_FALSE, isDraftOrTemplate,
                       PR_FALSE, aNewMsgFlags, listener, msgWindow, PR_FALSE);
    if (NS_FAILED(rv))
      return OnCopyCompleted(srcSupport, rv);

    m_copyState->m_streamCopy = PR_TRUE;

    nsCOMPtr<nsISupports> copySupport;
    if( m_copyState )
      copySupport = do_QueryInterface(m_copyState);
    if (!isDraftOrTemplate)
      m_copyState->m_totalCount = 1;
    rv = imapService->AppendMessageFromFile(m_thread, file, this,
                                            messageId,
                                            PR_TRUE, isDraftOrTemplate,
                                            urlListener, nsnull,
                                            copySupport,
                                            msgWindow);
    if (NS_FAILED(rv))
      return OnCopyCompleted(srcSupport, rv);

    return rv;
}

nsresult
nsImapMailFolder::CopyStreamMessage(nsIMsgDBHdr* message,
                                    nsIMsgFolder* dstFolder, // should be this
                                    nsIMsgWindow *aMsgWindow,
                                    PRBool isMove)
{
  NS_ENSURE_TRUE(m_copyState, NS_ERROR_NULL_POINTER);
  nsresult rv;
  nsCOMPtr<nsICopyMessageStreamListener> copyStreamListener = do_CreateInstance(NS_COPYMESSAGESTREAMLISTENER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv,rv);

  nsCOMPtr<nsICopyMessageListener> copyListener(do_QueryInterface(dstFolder, &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIMsgFolder> srcFolder(do_QueryInterface(m_copyState->m_srcSupport, &rv));
  if (NS_FAILED(rv)) return rv;
  rv = copyStreamListener->Init(srcFolder, copyListener, nsnull);
  if (NS_FAILED(rv)) return rv;

  nsCOMPtr<nsIMsgDBHdr> msgHdr(do_QueryInterface(message, &rv));
  if (NS_FAILED(rv)) return rv;

  nsCString uri;
  srcFolder->GetUriForMsg(msgHdr, uri);

  if (!m_copyState->m_msgService)
    rv = GetMessageServiceFromURI(uri, getter_AddRefs(m_copyState->m_msgService));

  if (NS_SUCCEEDED(rv) && m_copyState->m_msgService)
  {
    nsCOMPtr<nsIStreamListener> streamListener(do_QueryInterface(copyStreamListener, &rv));
    NS_ENSURE_SUCCESS(rv, rv);

    // put up status message here, if copying more than one message.
    if (m_copyState->m_totalCount > 1)
    {
      nsString dstFolderName, progressText;
      GetName(dstFolderName);
      nsAutoString curMsgString;
      nsAutoString totalMsgString;
      totalMsgString.AppendInt(m_copyState->m_totalCount);
      curMsgString.AppendInt(m_copyState->m_curIndex + 1);

      const PRUnichar *formatStrings[3] = {curMsgString.get(),
                                            totalMsgString.get(),
                                            dstFolderName.get()
                                            };

      nsCOMPtr <nsIStringBundle> bundle;
      rv = IMAPGetStringBundle(getter_AddRefs(bundle));
      NS_ENSURE_SUCCESS(rv, rv);
      rv = bundle->FormatStringFromID(IMAP_COPYING_MESSAGE_OF,
                                        formatStrings, 3,
                                        getter_Copies(progressText));
      nsCOMPtr <nsIMsgStatusFeedback> statusFeedback;
      if (m_copyState->m_msgWindow)
        m_copyState->m_msgWindow->GetStatusFeedback(getter_AddRefs(statusFeedback));
      if (statusFeedback)
      {
        statusFeedback->ShowStatusString(progressText);
        PRInt32 percent;
        percent = (100 * m_copyState->m_curIndex) / (PRInt32) m_copyState->m_totalCount;
          statusFeedback->ShowProgress(percent);
      }
    }
    rv = m_copyState->m_msgService->CopyMessage(uri.get(), streamListener,
                                                isMove && !m_copyState->m_isCrossServerOp, nsnull, aMsgWindow, nsnull);
  } 
  return rv;
}

nsImapMailCopyState::nsImapMailCopyState() :
    m_isMove(PR_FALSE), m_selectedState(PR_FALSE),
    m_isCrossServerOp(PR_FALSE), m_curIndex(0),
    m_totalCount(0), m_streamCopy(PR_FALSE), m_dataBuffer(nsnull),
    m_dataBufferSize(0), m_leftOver(0), m_allowUndo(PR_FALSE),
    m_eatLF(PR_FALSE), m_newMsgFlags(0)
{
}

nsImapMailCopyState::~nsImapMailCopyState()
{
  PR_Free(m_dataBuffer);
  if (m_msgService && m_message)
  {
    nsCOMPtr <nsIMsgFolder> srcFolder = do_QueryInterface(m_srcSupport);
    if (srcFolder)
    {
      nsCString uri;
      srcFolder->GetUriForMsg(m_message, uri);
    }
  }
  if (m_tmpFile)
    m_tmpFile->Remove(PR_FALSE);
}


NS_IMPL_THREADSAFE_ISUPPORTS1(nsImapMailCopyState, nsImapMailCopyState)

nsresult
nsImapMailFolder::InitCopyState(nsISupports* srcSupport,
                                nsIArray* messages,
                                PRBool isMove,
                                PRBool selectedState,
                                PRBool acrossServers,
                                PRUint32 newMsgFlags,
                                nsIMsgCopyServiceListener* listener,
                                nsIMsgWindow *msgWindow,
                                PRBool allowUndo)
{
  NS_ENSURE_ARG_POINTER(srcSupport);
  NS_ENSURE_ARG_POINTER(messages);
  NS_ENSURE_TRUE(!m_copyState, NS_ERROR_FAILURE);
  nsresult rv;

  nsImapMailCopyState* copyState = new nsImapMailCopyState();
  m_copyState = do_QueryInterface(copyState);
  NS_ENSURE_TRUE(m_copyState,NS_ERROR_OUT_OF_MEMORY);

  m_copyState->m_isCrossServerOp = acrossServers;
  m_copyState->m_srcSupport = do_QueryInterface(srcSupport, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  m_copyState->m_messages = messages;
  rv = messages->GetLength(&m_copyState->m_totalCount);
  if (!m_copyState->m_isCrossServerOp)
  {
    if (NS_SUCCEEDED(rv))
    {
        PRUint32 numUnread = 0;
        for (PRUint32 keyIndex=0; keyIndex < m_copyState->m_totalCount; keyIndex++)
        {
          nsCOMPtr<nsIMsgDBHdr> message = do_QueryElementAt(m_copyState->m_messages, keyIndex, &rv);
          // if the key is not there, then assume what the caller tells us to.
          PRBool isRead = PR_FALSE;
          PRUint32 flags;
          if (message )
          {
            message->GetFlags(&flags);
            isRead = flags & MSG_FLAG_READ;
          }
          if (!isRead)
            numUnread++;
        }
        m_copyState->m_unreadCount = numUnread;
    }
  }
  else
  {
    nsCOMPtr<nsIMsgDBHdr> message =
        do_QueryElementAt(m_copyState->m_messages,
                          m_copyState->m_curIndex, &rv);
      // if the key is not there, then assume what the caller tells us to.
    PRBool isRead = PR_FALSE;
    PRUint32 flags;
    if (message )
    {
      message->GetFlags(&flags);
      isRead = flags & MSG_FLAG_READ;
    }
    m_copyState->m_unreadCount = (isRead) ? 0 : 1;
  }

  m_copyState->m_isMove = isMove;
  m_copyState->m_newMsgFlags = newMsgFlags;
  m_copyState->m_allowUndo = allowUndo;
  m_copyState->m_selectedState = selectedState;
  m_copyState->m_msgWindow = msgWindow;
  if (listener)
    m_copyState->m_listener = do_QueryInterface(listener, &rv);
  return rv;
}

nsresult
nsImapMailFolder::OnCopyCompleted(nsISupports *srcSupport, nsresult rv)
{
  m_copyState = nsnull;
  nsresult result;
  nsCOMPtr<nsIMsgCopyService> copyService = do_GetService(NS_MSGCOPYSERVICE_CONTRACTID, &result);
  NS_ENSURE_SUCCESS(result, result);
  return copyService->NotifyCompletion(srcSupport, this, rv);
}

nsresult nsImapMailFolder::CreateBaseMessageURI(const nsACString& aURI)
{
  return nsCreateImapBaseMessageURI(aURI, mBaseMessageURI);
}

NS_IMETHODIMP nsImapMailFolder::GetFolderURL(nsACString& aFolderURL)
{
  nsCOMPtr<nsIMsgFolder> rootFolder;
  nsresult rv = GetRootFolder(getter_AddRefs(rootFolder));
  NS_ENSURE_SUCCESS(rv, rv);
  rootFolder->GetURI(aFolderURL);

  NS_ASSERTION(mURI.Length() > aFolderURL.Length(), "Should match with a folder name!");
  nsCString escapedName;
  escapedName.Adopt(nsEscape(mURI.get() + aFolderURL.Length(), url_Path));
  if (escapedName.IsEmpty())
    return NS_ERROR_OUT_OF_MEMORY;
  aFolderURL.Append(escapedName);
  return NS_OK;
}

NS_IMETHODIMP nsImapMailFolder::GetFolderNeedsSubscribing(PRBool *bVal)
{
  NS_ENSURE_ARG_POINTER(bVal);
  *bVal = m_folderNeedsSubscribing;
  return NS_OK;
}

NS_IMETHODIMP nsImapMailFolder::SetFolderNeedsSubscribing(PRBool bVal)
{
  m_folderNeedsSubscribing = bVal;
  return NS_OK;
}

nsMsgIMAPFolderACL * nsImapMailFolder::GetFolderACL()
{
  if (!m_folderACL)
    m_folderACL = new nsMsgIMAPFolderACL(this);
  return m_folderACL;
}

nsresult nsImapMailFolder::CreateACLRightsStringForFolder(nsAString& rightsString)
{
  GetFolderACL(); // lazy create
  NS_ENSURE_TRUE(m_folderACL, NS_ERROR_NULL_POINTER);
  return m_folderACL->CreateACLRightsString(rightsString);
}

NS_IMETHODIMP nsImapMailFolder::GetFolderNeedsACLListed(PRBool *bVal)
{
  NS_ENSURE_ARG_POINTER(bVal);
  PRBool dontNeedACLListed = !m_folderNeedsACLListed;
  // if we haven't acl listed, and it's not a no select folder or the inbox,
  //  then we'll list the acl if it's not a namespace.
  if (m_folderNeedsACLListed && !(mFlags & (nsMsgFolderFlags::ImapNoselect | nsMsgFolderFlags::Inbox)))
    GetIsNamespace(&dontNeedACLListed);
  *bVal = !dontNeedACLListed;
  return NS_OK;
}

NS_IMETHODIMP nsImapMailFolder::SetFolderNeedsACLListed(PRBool bVal)
{
  m_folderNeedsACLListed = bVal;
  return NS_OK;
}

NS_IMETHODIMP nsImapMailFolder::GetIsNamespace(PRBool *aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);
  nsresult rv = NS_OK;
  if (!m_namespace)
  {
#ifdef DEBUG_bienvenu
    // Make sure this isn't causing us to open the database
    NS_ASSERTION(m_hierarchyDelimiter != kOnlineHierarchySeparatorUnknown, "hierarchy delimiter not set");
#endif

    nsCString onlineName, serverKey;
    GetServerKey(serverKey);
    GetOnlineName(onlineName);
    PRUnichar hierarchyDelimiter;
    GetHierarchyDelimiter(&hierarchyDelimiter);

    nsCOMPtr<nsIImapHostSessionList> hostSession = do_GetService(kCImapHostSessionList, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    m_namespace = nsIMAPNamespaceList::GetNamespaceForFolder(serverKey.get(), onlineName.get(), (char) hierarchyDelimiter);
    if (m_namespace == nsnull)
    {
      if (mFlags & nsMsgFolderFlags::ImapOtherUser)
         rv = hostSession->GetDefaultNamespaceOfTypeForHost(serverKey.get(), kOtherUsersNamespace, m_namespace);
      else if (mFlags & nsMsgFolderFlags::ImapPublic)
        rv = hostSession->GetDefaultNamespaceOfTypeForHost(serverKey.get(), kPublicNamespace, m_namespace);
      else
        rv = hostSession->GetDefaultNamespaceOfTypeForHost(serverKey.get(), kPersonalNamespace, m_namespace);
    }
    NS_ASSERTION(m_namespace, "failed to get namespace");
    if (m_namespace)
    {
      nsIMAPNamespaceList::SuggestHierarchySeparatorForNamespace(m_namespace, (char) hierarchyDelimiter);
      m_folderIsNamespace = nsIMAPNamespaceList::GetFolderIsNamespace(serverKey.get(), onlineName.get(), (char) hierarchyDelimiter, m_namespace);
    }
  }
  *aResult = m_folderIsNamespace;
  return rv;
}

NS_IMETHODIMP nsImapMailFolder::SetIsNamespace(PRBool isNamespace)
{
  m_folderIsNamespace = isNamespace;
  return NS_OK;
}

NS_IMETHODIMP nsImapMailFolder::ResetNamespaceReferences()
{
  nsCString serverKey;
  nsCString onlineName;
  GetServerKey(serverKey);
  GetOnlineName(onlineName);
  PRUnichar hierarchyDelimiter;
  GetHierarchyDelimiter(&hierarchyDelimiter);
  m_namespace = nsIMAPNamespaceList::GetNamespaceForFolder(serverKey.get(), onlineName.get(), (char) hierarchyDelimiter);
//  NS_ASSERTION(m_namespace, "resetting null namespace");
  m_folderIsNamespace = m_namespace ? nsIMAPNamespaceList::GetFolderIsNamespace(serverKey.get(), onlineName.get(), 
                                                                                (char) hierarchyDelimiter, m_namespace) : PR_FALSE;

  nsCOMPtr<nsISimpleEnumerator> enumerator;
  GetSubFolders(getter_AddRefs(enumerator));
  if (!enumerator)
    return NS_OK;

  nsresult rv;
  PRBool hasMore;
  while (NS_SUCCEEDED(enumerator->HasMoreElements(&hasMore)) && hasMore)
  {
    nsCOMPtr<nsISupports> item;
    rv = enumerator->GetNext(getter_AddRefs(item));
    if (NS_FAILED(rv))
      break;

    nsCOMPtr<nsIMsgImapMailFolder> folder(do_QueryInterface(item, &rv));
    if (NS_FAILED(rv))
      return rv;

    folder->ResetNamespaceReferences();
  }
  return rv;
}

NS_IMETHODIMP nsImapMailFolder::FindOnlineSubFolder(const nsACString& targetOnlineName, nsIMsgImapMailFolder **aResultFolder)
{
  nsresult rv = NS_OK;

  nsCString onlineName;
  GetOnlineName(onlineName);

  if (onlineName.Equals(targetOnlineName))
    return QueryInterface(NS_GET_IID(nsIMsgImapMailFolder), (void **) aResultFolder);

  nsCOMPtr<nsISimpleEnumerator> enumerator;
  GetSubFolders(getter_AddRefs(enumerator));
  if (!enumerator)
    return NS_OK;

  PRBool hasMore;
  while (NS_SUCCEEDED(enumerator->HasMoreElements(&hasMore)) && hasMore)
  {
    nsCOMPtr<nsISupports> item;
    rv = enumerator->GetNext(getter_AddRefs(item));
    if (NS_FAILED(rv))
      break;

    nsCOMPtr<nsIMsgImapMailFolder> folder(do_QueryInterface(item, &rv));
    if (NS_FAILED(rv))
      return rv;

    rv = folder->FindOnlineSubFolder(targetOnlineName, aResultFolder);
    if (*aResultFolder)
     return rv;
  }
  return rv;
}

NS_IMETHODIMP nsImapMailFolder::GetFolderNeedsAdded(PRBool *bVal)
{
  NS_ENSURE_ARG_POINTER(*bVal);
  *bVal = m_folderNeedsAdded;
  return NS_OK;
}

NS_IMETHODIMP nsImapMailFolder::SetFolderNeedsAdded(PRBool bVal)
{
  m_folderNeedsAdded = bVal;
  return NS_OK;
}

NS_IMETHODIMP nsImapMailFolder::GetFolderQuotaCommandIssued(PRBool *aCmdIssued)
{
  NS_ENSURE_ARG_POINTER(aCmdIssued);
  *aCmdIssued = m_folderQuotaCommandIssued;
  return NS_OK;
}

NS_IMETHODIMP nsImapMailFolder::SetFolderQuotaCommandIssued(PRBool aCmdIssued)
{
  m_folderQuotaCommandIssued = aCmdIssued;
  return NS_OK;
}

NS_IMETHODIMP nsImapMailFolder::SetFolderQuotaData(const nsACString &aFolderQuotaRoot,
                                                   PRUint32 aFolderQuotaUsedKB,
                                                    PRUint32 aFolderQuotaMaxKB)
{
  m_folderQuotaDataIsValid = PR_TRUE;
  m_folderQuotaRoot = aFolderQuotaRoot;
  m_folderQuotaUsedKB = aFolderQuotaUsedKB;
  m_folderQuotaMaxKB = aFolderQuotaMaxKB;
  return NS_OK;
}

NS_IMETHODIMP nsImapMailFolder::GetQuota(PRBool* aValid,
                                         PRUint32* aUsed, PRUint32* aMax)
{
  NS_ENSURE_ARG_POINTER(aValid);
  NS_ENSURE_ARG_POINTER(aUsed);
  NS_ENSURE_ARG_POINTER(aMax);
  *aValid = m_folderQuotaDataIsValid;
  *aUsed = m_folderQuotaUsedKB;
  *aMax = m_folderQuotaMaxKB;
  return NS_OK;
}

NS_IMETHODIMP nsImapMailFolder::PerformExpand(nsIMsgWindow *aMsgWindow)
{
  nsresult rv;
  PRBool usingSubscription = PR_FALSE;
  nsCOMPtr<nsIImapIncomingServer> imapServer;
  rv = GetImapIncomingServer(getter_AddRefs(imapServer));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = imapServer->GetUsingSubscription(&usingSubscription);
  if (NS_SUCCEEDED(rv) && !usingSubscription)
  {
    nsCOMPtr<nsIImapService> imapService = do_GetService(NS_IMAPSERVICE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = imapService->DiscoverChildren(m_thread, this, this,
                                       m_onlineFolderName,
                                       nsnull);
  }
  return rv;
}

NS_IMETHODIMP nsImapMailFolder::RenameClient(nsIMsgWindow *msgWindow, nsIMsgFolder *msgFolder, const nsACString& oldName, const nsACString& newName)
{
  nsresult rv;
  nsCOMPtr<nsILocalFile> pathFile;
  rv = GetFilePath(getter_AddRefs(pathFile));
  if (NS_FAILED(rv)) return rv;

  nsCOMPtr<nsIMsgImapMailFolder> oldImapFolder = do_QueryInterface(msgFolder, &rv);
  if (NS_FAILED(rv)) return rv;

  PRUnichar hierarchyDelimiter = '/';
  oldImapFolder->GetHierarchyDelimiter(&hierarchyDelimiter);
  PRInt32 boxflags=0;
  oldImapFolder->GetBoxFlags(&boxflags);
  
  nsAutoString newLeafName;
  nsAutoString newNameString;
  CopyASCIItoUTF16(newName, newNameString);
  newLeafName = newNameString;
  nsAutoString parentName;
  nsAutoString folderNameStr;
  PRInt32 folderStart = newLeafName.RFindChar('/');  //internal use of hierarchyDelimiter is always '/'
  if (folderStart > 0)
  {
    newNameString.Right(newLeafName, newLeafName.Length() - folderStart - 1);
    CreateDirectoryForFolder(getter_AddRefs(pathFile));    //needed when we move a folder to a folder with no subfolders.
  }

  // if we get here, it's really a leaf, and "this" is the parent.
  folderNameStr = newLeafName;

  // Create an empty database for this mail folder, set its name from the user
  nsCOMPtr<nsIMsgDatabase> mailDBFactory;
  nsCOMPtr<nsIMsgFolder> child;
  nsCOMPtr <nsIMsgImapMailFolder> imapFolder;

  nsCOMPtr<nsIMsgDBService> msgDBService = do_GetService(NS_MSGDB_SERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIMsgDatabase> unusedDB;
  nsCOMPtr <nsILocalFile> dbFile;

  nsCAutoString proposedDBName;
  LossyCopyUTF16toASCII(newLeafName, proposedDBName);
  // warning, path will be changed
  rv = CreateFileForDB(proposedDBName, pathFile, getter_AddRefs(dbFile));
  NS_ENSURE_SUCCESS(rv,rv);

  // it's OK to use openMailDBFromFileSpec and not OpenFolderDB here, since we don't use the DB.
  rv = msgDBService->OpenMailDBFromFile(dbFile, PR_TRUE, PR_TRUE, (nsIMsgDatabase **) getter_AddRefs(unusedDB));
  if (NS_SUCCEEDED(rv) && unusedDB)
  {
    //need to set the folder name
    nsCOMPtr <nsIDBFolderInfo> folderInfo;
    rv = unusedDB->GetDBFolderInfo(getter_AddRefs(folderInfo));

    //Now let's create the actual new folder
    rv = AddSubfolderWithPath(folderNameStr, dbFile, getter_AddRefs(child));
    if (!child || NS_FAILED(rv)) return rv;
    nsString unicodeName;
    rv = CopyMUTF7toUTF16(proposedDBName, unicodeName);
    if (NS_SUCCEEDED(rv))
      child->SetName(unicodeName);
    imapFolder = do_QueryInterface(child);
    if (imapFolder)
    {
      nsCAutoString onlineName(m_onlineFolderName);
      if (!onlineName.IsEmpty())
      onlineName.Append(char(hierarchyDelimiter));
      LossyAppendUTF16toASCII(folderNameStr, onlineName);
      imapFolder->SetVerifiedAsOnlineFolder(PR_TRUE);
      imapFolder->SetOnlineName(onlineName);
      imapFolder->SetHierarchyDelimiter(hierarchyDelimiter);
      imapFolder->SetBoxFlags(boxflags);
      // store the online name as the mailbox name in the db folder info
      // I don't think anyone uses the mailbox name, so we'll use it
      // to restore the online name when blowing away an imap db.
      if (folderInfo)
      {
        nsAutoString unicodeOnlineName;
        CopyASCIItoUTF16(onlineName, unicodeOnlineName);
        folderInfo->SetMailboxName(unicodeOnlineName);
      }
      PRBool changed = PR_FALSE;
      msgFolder->MatchOrChangeFilterDestination(child, PR_FALSE /*caseInsensitive*/, &changed);
      if (changed)
        msgFolder->AlertFilterChanged(msgWindow);
    }
    unusedDB->SetSummaryValid(PR_TRUE);
    unusedDB->Commit(nsMsgDBCommitType::kLargeCommit);
    unusedDB->Close(PR_TRUE);
    child->RenameSubFolders(msgWindow, msgFolder);
    nsCOMPtr<nsIMsgFolder> msgParent;
    msgFolder->GetParentMsgFolder(getter_AddRefs(msgParent));
    msgFolder->SetParent(nsnull);
    msgParent->PropagateDelete(msgFolder, PR_TRUE, nsnull);

    // Reset online status now that the folder is renamed.
    nsCOMPtr <nsIMsgImapMailFolder> oldImapFolder = do_QueryInterface(msgFolder);
    if (oldImapFolder)
      oldImapFolder->SetVerifiedAsOnlineFolder(PR_FALSE);
    nsCOMPtr<nsIMsgFolderNotificationService> notifier(do_GetService(NS_MSGNOTIFICATIONSERVICE_CONTRACTID));
    if (notifier)
      notifier->NotifyFolderRenamed(msgFolder, child);   
    NotifyItemAdded(child);
  }
  return rv;
}

NS_IMETHODIMP nsImapMailFolder::RenameSubFolders(nsIMsgWindow *msgWindow, nsIMsgFolder *oldFolder)
{
  m_initialized = PR_TRUE;
  nsCOMPtr<nsISimpleEnumerator> enumerator;
  nsresult rv = oldFolder->GetSubFolders(getter_AddRefs(enumerator));
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool hasMore;
  while (NS_SUCCEEDED(enumerator->HasMoreElements(&hasMore)) && hasMore)
  {
    nsCOMPtr<nsISupports> item;
    if (enumerator->GetNext(getter_AddRefs(item)))
      continue;

    nsCOMPtr<nsIMsgFolder> msgFolder(do_QueryInterface(item, &rv));
    if (NS_FAILED(rv))
      return rv;

    nsCOMPtr<nsIMsgImapMailFolder> folder(do_QueryInterface(msgFolder, &rv));
    if (NS_FAILED(rv))
      return rv;

    PRUnichar hierarchyDelimiter = '/';
    folder->GetHierarchyDelimiter(&hierarchyDelimiter);

    PRInt32 boxflags;
    folder->GetBoxFlags(&boxflags);

    PRBool verified;
    folder->GetVerifiedAsOnlineFolder(&verified);

    nsCOMPtr<nsILocalFile> oldPathFile;
    rv = msgFolder->GetFilePath(getter_AddRefs(oldPathFile));
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsILocalFile> newParentPathFile;
    rv = GetFilePath(getter_AddRefs(newParentPathFile));
    if (NS_FAILED(rv)) return rv;

    rv = AddDirectorySeparator(newParentPathFile);
    nsCAutoString oldLeafName;
    oldPathFile->GetNativeLeafName(oldLeafName);
    newParentPathFile->AppendNative(oldLeafName);

    nsCString newPathStr;
    newParentPathFile->GetNativePath(newPathStr);

    nsCOMPtr<nsILocalFile> newPathFile = do_CreateInstance(NS_LOCAL_FILE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    newPathFile->InitWithFile(newParentPathFile);

    nsCOMPtr<nsILocalFile> dbFilePath = newPathFile;

    nsCOMPtr<nsIMsgFolder> child;

    nsString folderName;
    rv = msgFolder->GetName(folderName);
    if (folderName.IsEmpty() || NS_FAILED(rv)) return rv;

    nsCString utf7LeafName;
    rv = CopyUTF16toMUTF7(folderName, utf7LeafName);
    NS_ENSURE_SUCCESS(rv, rv);

    // XXX : Fix this non-sense by fixing AddSubfolderWithPath
    nsAutoString unicodeLeafName;
    CopyASCIItoUTF16(utf7LeafName, unicodeLeafName);

    rv = AddSubfolderWithPath(unicodeLeafName, dbFilePath, getter_AddRefs(child));
    if (!child || NS_FAILED(rv)) return rv;

    child->SetName(folderName);
    nsCOMPtr <nsIMsgImapMailFolder> imapFolder = do_QueryInterface(child);
    nsCString onlineName;
    GetOnlineName(onlineName);
    nsCAutoString onlineCName(onlineName);
    onlineCName.Append(char(hierarchyDelimiter));
    onlineCName.Append(utf7LeafName);
    if (imapFolder)
    {
     imapFolder->SetVerifiedAsOnlineFolder(verified);
     imapFolder->SetOnlineName(onlineCName);
     imapFolder->SetHierarchyDelimiter(hierarchyDelimiter);
     imapFolder->SetBoxFlags(boxflags);

     PRBool changed = PR_FALSE;
     msgFolder->MatchOrChangeFilterDestination(child, PR_FALSE /*caseInsensitive*/, &changed);
     if (changed)
       msgFolder->AlertFilterChanged(msgWindow);
     child->RenameSubFolders(msgWindow, msgFolder);
    }
  }
  return rv;
}

NS_IMETHODIMP nsImapMailFolder::IsCommandEnabled(const nsACString& command, PRBool *result)
{
  NS_ENSURE_ARG_POINTER(result);
  *result = !(WeAreOffline() && (command.EqualsLiteral("cmd_renameFolder") ||
                                 command.EqualsLiteral("cmd_compactFolder") ||
                                 command.EqualsLiteral("button_compact") ||
                                 command.EqualsLiteral("cmd_delete") ||
                                 command.EqualsLiteral("button_delete")));
  return NS_OK;
}

NS_IMETHODIMP
nsImapMailFolder::GetCanFileMessages(PRBool *aCanFileMessages)
{
  nsresult rv;
  *aCanFileMessages = PR_TRUE;

  nsCOMPtr<nsIMsgIncomingServer> server;
  rv = GetServer(getter_AddRefs(server));
  if (NS_SUCCEEDED(rv) && server)
    rv = server->GetCanFileMessagesOnServer(aCanFileMessages);

  if (*aCanFileMessages)
    rv = nsMsgDBFolder::GetCanFileMessages(aCanFileMessages);

  if (*aCanFileMessages)
  {
    PRBool noSelect;
    GetFlag(nsMsgFolderFlags::ImapNoselect, &noSelect);
    *aCanFileMessages = (noSelect) ? PR_FALSE : GetFolderACL()->GetCanIInsertInFolder();
    return NS_OK;
  }
  return rv;
}

NS_IMETHODIMP
nsImapMailFolder::GetCanDeleteMessages(PRBool *aCanDeleteMessages)
{
  NS_ENSURE_ARG_POINTER(aCanDeleteMessages);
  *aCanDeleteMessages = GetFolderACL()->GetCanIDeleteInFolder();
  return NS_OK;
}

NS_IMETHODIMP
nsImapMailFolder::GetPerformingBiff(PRBool *aPerformingBiff)
{
  NS_ENSURE_ARG_POINTER(aPerformingBiff);
  *aPerformingBiff = m_performingBiff;
  return NS_OK;
}

NS_IMETHODIMP
nsImapMailFolder::SetPerformingBiff(PRBool aPerformingBiff)
{
  m_performingBiff = aPerformingBiff;
  return NS_OK;
}

NS_IMETHODIMP
nsImapMailFolder::SetFilterList(nsIMsgFilterList *aMsgFilterList)
{
  m_filterList = aMsgFilterList;
  return nsMsgDBFolder::SetFilterList(aMsgFilterList);
}

nsresult nsImapMailFolder::GetMoveCoalescer()
{
  if (!m_moveCoalescer)
  {
    m_moveCoalescer = new nsImapMoveCoalescer(this, nsnull /* msgWindow */);
    NS_ENSURE_TRUE (m_moveCoalescer, NS_ERROR_OUT_OF_MEMORY);
    m_moveCoalescer->AddRef();
  }
  return NS_OK;
}

nsresult
nsImapMailFolder::SpamFilterClassifyMessage(const char *aURI, nsIMsgWindow *aMsgWindow, nsIJunkMailPlugin *aJunkMailPlugin)
{
  ++m_numFilterClassifyRequests;
  return aJunkMailPlugin->ClassifyMessage(aURI, aMsgWindow, this);
}

nsresult
nsImapMailFolder::SpamFilterClassifyMessages(const char **aURIArray, PRUint32 aURICount, nsIMsgWindow *aMsgWindow, nsIJunkMailPlugin *aJunkMailPlugin)
{
  m_numFilterClassifyRequests += aURICount;
  return aJunkMailPlugin->ClassifyMessages(aURICount, aURIArray, aMsgWindow, this);
}

NS_IMETHODIMP
nsImapMailFolder::StoreCustomKeywords(nsIMsgWindow *aMsgWindow, const nsACString& aFlagsToAdd,
                                      const nsACString& aFlagsToSubtract, nsMsgKey *aKeysToStore, PRUint32 aNumKeys, nsIURI **_retval)
{
  nsresult rv;
  if (WeAreOffline())
  {
    GetDatabase(nsnull);
    if (mDatabase)
    {
      for (PRUint32 keyIndex = 0; keyIndex < aNumKeys; keyIndex++)
      {
        nsCOMPtr <nsIMsgOfflineImapOperation> op;
        rv = mDatabase->GetOfflineOpForKey(aKeysToStore[keyIndex], PR_TRUE, getter_AddRefs(op));
        SetFlag(nsMsgFolderFlags::OfflineEvents);
        if (NS_SUCCEEDED(rv) && op)
        {
          if (!aFlagsToAdd.IsEmpty())
            op->AddKeywordToAdd(PromiseFlatCString(aFlagsToAdd).get());
          if (!aFlagsToSubtract.IsEmpty())
            op->AddKeywordToRemove(PromiseFlatCString(aFlagsToSubtract).get());
        }
      }
      mDatabase->Commit(nsMsgDBCommitType::kLargeCommit); // flush offline ops
      return rv;
    }
  }
  nsCOMPtr<nsIImapService> imapService(do_GetService(NS_IMAPSERVICE_CONTRACTID, &rv));
  NS_ENSURE_SUCCESS(rv, rv);
  nsCAutoString msgIds;
  AllocateUidStringFromKeys(aKeysToStore, aNumKeys, msgIds);
  return imapService->StoreCustomKeywords(m_thread, this, aMsgWindow, aFlagsToAdd,
                                          aFlagsToSubtract, msgIds, _retval);
}

NS_IMETHODIMP nsImapMailFolder::NotifyIfNewMail()
{
  return PerformBiffNotifications();
}

PRBool nsImapMailFolder::ShowPreviewText()
{
  PRBool showPreviewText = PR_FALSE;
  nsCOMPtr<nsIPrefBranch> prefBranch(do_GetService(NS_PREFSERVICE_CONTRACTID));
  if (prefBranch)
    prefBranch->GetBoolPref("mail.biff.alert.show_preview", &showPreviewText);
  return showPreviewText;
}

nsresult
nsImapMailFolder::PlaybackCoalescedOperations()
{
  if (m_moveCoalescer)
  {
    nsTArray<nsMsgKey> *junkKeysToClassify = m_moveCoalescer->GetKeyBucket(0);
    if (junkKeysToClassify && !junkKeysToClassify->IsEmpty())
      StoreCustomKeywords(m_moveCoalescer->GetMsgWindow(), NS_LITERAL_CSTRING("Junk"), EmptyCString(), junkKeysToClassify->Elements(), junkKeysToClassify->Length(), nsnull);
    junkKeysToClassify->Clear();
    nsTArray<nsMsgKey> *nonJunkKeysToClassify = m_moveCoalescer->GetKeyBucket(1);
    if (nonJunkKeysToClassify && !nonJunkKeysToClassify->IsEmpty())
      StoreCustomKeywords(m_moveCoalescer->GetMsgWindow(), NS_LITERAL_CSTRING("NonJunk"), EmptyCString(), nonJunkKeysToClassify->Elements(), nonJunkKeysToClassify->Length(), nsnull);
    nonJunkKeysToClassify->Clear();
    return m_moveCoalescer->PlaybackMoves(ShowPreviewText());
  }
  return NS_OK; // must not be any coalesced operations
}

NS_IMETHODIMP
nsImapMailFolder::SetJunkScoreForMessages(nsIArray *aMessages, const nsACString& aJunkScore)
{
  NS_ENSURE_ARG(aMessages);

  nsresult rv = nsMsgDBFolder::SetJunkScoreForMessages(aMessages, aJunkScore);
  if (NS_SUCCEEDED(rv))
  {
    nsCAutoString messageIds;
    nsTArray<nsMsgKey> keys;
    nsresult rv = BuildIdsAndKeyArray(aMessages, messageIds, keys);
    NS_ENSURE_SUCCESS(rv, rv);
    StoreCustomKeywords(nsnull, aJunkScore.Equals("0") ? NS_LITERAL_CSTRING("NonJunk") : NS_LITERAL_CSTRING("Junk"), EmptyCString(), keys.Elements(),
      keys.Length(), nsnull);
    if (mDatabase)
      mDatabase->Commit(nsMsgDBCommitType::kLargeCommit);
  }
  return rv;
}

NS_IMETHODIMP
nsImapMailFolder::OnMessageClassified(const char * aMsgURI,
  nsMsgJunkStatus aClassification,
  PRUint32 aJunkPercent)
{
  nsCString spamFolderURI;
  nsCOMPtr <nsIMsgIncomingServer> server;
  nsresult rv = GetServer(getter_AddRefs(server));
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr <nsIMsgDBHdr> msgHdr;
  rv = GetMsgDBHdrFromURI(aMsgURI, getter_AddRefs(msgHdr));
  NS_ENSURE_SUCCESS(rv, rv);

  nsMsgKey msgKey;
  rv = msgHdr->GetMessageKey(&msgKey);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCAutoString msgJunkScore;
  msgJunkScore.AppendInt(aClassification == nsIJunkMailPlugin::JUNK ?
        nsIJunkMailPlugin::IS_SPAM_SCORE:
        nsIJunkMailPlugin::IS_HAM_SCORE);
  mDatabase->SetStringProperty(msgKey, "junkscore", msgJunkScore.get());
  mDatabase->SetStringProperty(msgKey, "junkscoreorigin", "plugin");

  char* strScore = PR_smprintf("%u", aJunkPercent);
  mDatabase->SetStringProperty(msgKey, "junkpercent", strScore);
  PR_smprintf_free(strScore);

  GetMoveCoalescer();
  if (m_moveCoalescer)
  {
    nsTArray<nsMsgKey> *keysToClassify = m_moveCoalescer->GetKeyBucket((aClassification == nsIJunkMailPlugin::JUNK) ? 0 : 1);
    NS_ASSERTION(keysToClassify, "error getting key bucket");
    if (keysToClassify)
      keysToClassify->AppendElement(msgKey);
  }
  if (aClassification == nsIJunkMailPlugin::JUNK)
  {
    nsCOMPtr<nsISpamSettings> spamSettings;
    rv = GetServer(getter_AddRefs(server));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = server->GetSpamSettings(getter_AddRefs(spamSettings));
    NS_ENSURE_SUCCESS(rv, rv);

    PRBool markAsReadOnSpam;
    (void)spamSettings->GetMarkAsReadOnSpam(&markAsReadOnSpam);
    if (markAsReadOnSpam)
    {
      if (!m_junkMessagesToMarkAsRead)
        m_junkMessagesToMarkAsRead = do_CreateInstance(NS_ARRAY_CONTRACTID);
      m_junkMessagesToMarkAsRead->AppendElement(msgHdr, PR_FALSE);
    }

    PRBool willMoveMessage = PR_FALSE;

    // don't do the move when we are opening up
    // the junk mail folder or the trash folder
    // or when manually classifying messages in those folders
    if (!(mFlags & nsMsgFolderFlags::Junk || mFlags & nsMsgFolderFlags::Trash))
    {
      PRBool moveOnSpam;
      (void)spamSettings->GetMoveOnSpam(&moveOnSpam);
      if (moveOnSpam)
      {
        rv = spamSettings->GetSpamFolderURI(getter_Copies(spamFolderURI));
        NS_ENSURE_SUCCESS(rv,rv);

        if (!spamFolderURI.IsEmpty())
        {
          nsCOMPtr<nsIMsgFolder> folder;
          rv = GetExistingFolder(spamFolderURI, getter_AddRefs(folder));
          if (NS_SUCCEEDED(rv) && folder)
          {
            rv = folder->SetFlag(nsMsgFolderFlags::Junk);
            NS_ENSURE_SUCCESS(rv,rv);
            if (NS_SUCCEEDED(GetMoveCoalescer()))
            {
              m_moveCoalescer->AddMove(folder, msgKey);
              willMoveMessage = PR_TRUE;
            }
          }
          else
          {
            // XXX TODO
            // JUNK MAIL RELATED
            // the listener should do
            // rv = folder->SetFlag(nsMsgFolderFlags::Junk);
            // NS_ENSURE_SUCCESS(rv,rv);
            // if (NS_SUCCEEDED(GetMoveCoalescer())) {
            //   m_moveCoalescer->AddMove(folder, msgKey);
            //   willMoveMessage = PR_TRUE;
            // }
            rv = GetOrCreateFolder(spamFolderURI, nsnull /* aListener */);
            NS_ASSERTION(NS_SUCCEEDED(rv), "GetOrCreateFolder failed");
          }
        }
      }
    }
    rv = spamSettings->LogJunkHit(msgHdr, willMoveMessage);
    NS_ENSURE_SUCCESS(rv,rv);
  }
  if (--m_numFilterClassifyRequests == 0)
  {
    if (m_junkMessagesToMarkAsRead)
    {
      PRUint32 count;
      m_junkMessagesToMarkAsRead->GetLength(&count);
      if (count > 0)
      {
        rv = MarkMessagesRead(m_junkMessagesToMarkAsRead, true);
        NS_ENSURE_SUCCESS(rv,rv);
        m_junkMessagesToMarkAsRead->Clear();
      }
    }
    PRBool pendingMoves = m_moveCoalescer && m_moveCoalescer->HasPendingMoves();
    PlaybackCoalescedOperations();
    // If we are performing biff for this folder, tell the server object
    if ((!pendingMoves || !ShowPreviewText()) && m_performingBiff)
    {
      // we don't need to adjust the num new messages in this folder because
      // the playback moves code already did that.
      (void) PerformBiffNotifications();
      nsCOMPtr<nsIMsgIncomingServer> server;
      if (NS_SUCCEEDED(GetServer(getter_AddRefs(server))) && server)
        server->SetPerformingBiff(PR_FALSE);
      m_performingBiff = PR_FALSE;
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsImapMailFolder::GetShouldDownloadAllHeaders(PRBool *aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);
  *aResult = PR_FALSE;
  //for just the inbox, we check if the filter list has arbitary headers.
  //for all folders, check if we have a spam plugin that requires all headers
  if (mFlags & nsMsgFolderFlags::Inbox)
  {
    nsCOMPtr <nsIMsgFilterList> filterList;
    nsresult rv = GetFilterList(nsnull, getter_AddRefs(filterList));
    NS_ENSURE_SUCCESS(rv,rv);

    rv = filterList->GetShouldDownloadAllHeaders(aResult);
    if (*aResult)
      return rv;
  }
  nsCOMPtr <nsIMsgFilterPlugin> filterPlugin;
  nsCOMPtr<nsIMsgIncomingServer> server;

  if (NS_SUCCEEDED(GetServer(getter_AddRefs(server))))
    server->GetSpamFilterPlugin(getter_AddRefs(filterPlugin));

  return (filterPlugin) ? filterPlugin->GetShouldDownloadAllHeaders(aResult) : NS_OK;
}


void nsImapMailFolder::GetTrashFolderName(nsAString &aFolderName)
{
  nsCOMPtr<nsIMsgIncomingServer> server;
  nsCOMPtr<nsIImapIncomingServer> imapServer;
  nsresult rv;
  rv = GetServer(getter_AddRefs(server));
  if (NS_FAILED(rv)) return;
  imapServer = do_QueryInterface(server, &rv);
  if (NS_FAILED(rv)) return;
  imapServer->GetTrashFolderName(aFolderName);
  return;
}
NS_IMETHODIMP nsImapMailFolder::FetchMsgPreviewText(nsMsgKey *aKeysToFetch, PRUint32 aNumKeys,
                                                 PRBool aLocalOnly, nsIUrlListener *aUrlListener,
                                                 PRBool *aAsyncResults)
{
  NS_ENSURE_ARG_POINTER(aKeysToFetch);
  NS_ENSURE_ARG_POINTER(aAsyncResults);

  nsTArray<nsMsgKey> keysToFetchFromServer;

  *aAsyncResults = PR_FALSE;
  nsresult rv = NS_OK;

  nsCOMPtr<nsIImapService> imapService = do_GetService(NS_IMAPSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr <nsIMsgMessageService> msgService = do_QueryInterface(imapService, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  for (PRUint32 i = 0; i < aNumKeys; i++)
  {
    nsCOMPtr <nsIMsgDBHdr> msgHdr;
    nsCString prevBody;
    rv = GetMessageHeader(aKeysToFetch[i], getter_AddRefs(msgHdr));
    NS_ENSURE_SUCCESS(rv, rv);
    // ignore messages that already have a preview body.
    msgHdr->GetStringProperty("preview", getter_Copies(prevBody));
    if (!prevBody.IsEmpty())
      continue;

    /* check if message is in memory cache or offline store. */
    nsCOMPtr <nsIURI> url;
    nsCOMPtr<nsIInputStream> inputStream;
    nsCString messageUri;
    rv = GetUriForMsg(msgHdr, messageUri);
    NS_ENSURE_SUCCESS(rv,rv);
    rv = msgService->GetUrlForUri(messageUri.get(), getter_AddRefs(url), nsnull);
    NS_ENSURE_SUCCESS(rv,rv);
    nsCAutoString urlSpec;
    url->GetAsciiSpec(urlSpec);

    nsCOMPtr<nsICacheSession> cacheSession;
    rv = imapService->GetCacheSession(getter_AddRefs(cacheSession));
    NS_ENSURE_SUCCESS(rv, rv);
    PRInt32 uidValidity;
    GetUidValidity(&uidValidity);
    // stick the uid validity in front of the url, so that if the uid validity
    // changes, we won't re-use the wrong cache entries.
    nsCAutoString cacheKey;
    cacheKey.AppendInt(uidValidity, 16);
    cacheKey.Append(urlSpec);
    nsCOMPtr <nsICacheEntryDescriptor> cacheEntry;

    // if mem cache entry is broken or empty, go to next message.
    rv = cacheSession->OpenCacheEntry(cacheKey, nsICache::ACCESS_READ, PR_FALSE, getter_AddRefs(cacheEntry));
    if (cacheEntry)
    {
      rv = cacheEntry->OpenInputStream(0, getter_AddRefs(inputStream));
      if (NS_SUCCEEDED(rv))
      {
        PRUint32 bytesAvailable = 0;
        rv = inputStream->Available(&bytesAvailable);
        if (!bytesAvailable)
          continue;
        rv = GetMsgPreviewTextFromStream(msgHdr, inputStream);
      }
    }
    else // lets look in the offline store
    {
      PRUint32 msgFlags;
      msgHdr->GetFlags(&msgFlags);
      nsMsgKey msgKey;
      msgHdr->GetMessageKey(&msgKey);
      if (msgFlags & MSG_FLAG_OFFLINE)
      {
        nsMsgKey messageOffset;
        PRUint32 messageSize;
        GetOfflineFileStream(msgKey, &messageOffset, &messageSize, getter_AddRefs(inputStream));
        if (inputStream)
          rv = GetMsgPreviewTextFromStream(msgHdr, inputStream);
      }
      else if (!aLocalOnly)
        keysToFetchFromServer.AppendElement(msgKey);
    }
  }
  if (!keysToFetchFromServer.IsEmpty())
  {
    PRUint32 msgCount = keysToFetchFromServer.Length();
    nsCAutoString messageIds;
    AllocateImapUidString(keysToFetchFromServer.Elements(), msgCount,
                         nsnull, messageIds);
    rv = imapService->GetBodyStart(m_thread, this, aUrlListener,
                                   messageIds, 2048, nsnull);
    *aAsyncResults = PR_TRUE; // the preview text will be available async...
  }
  return NS_OK;
}

NS_IMETHODIMP nsImapMailFolder::AddKeywordsToMessages(nsIArray *aMessages, const nsACString& aKeywords)
{
  nsresult rv = nsMsgDBFolder::AddKeywordsToMessages(aMessages, aKeywords);
  if (NS_SUCCEEDED(rv))
  {
    nsCAutoString messageIds;
    nsTArray<nsMsgKey> keys;
    rv = BuildIdsAndKeyArray(aMessages, messageIds, keys);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = StoreCustomKeywords(nsnull, aKeywords, EmptyCString(), keys.Elements(), keys.Length(), nsnull);
    if (mDatabase)
      mDatabase->Commit(nsMsgDBCommitType::kLargeCommit);
  }
  return rv;
}

NS_IMETHODIMP nsImapMailFolder::RemoveKeywordsFromMessages(nsIArray *aMessages, const nsACString& aKeywords)
{
  nsresult rv = nsMsgDBFolder::RemoveKeywordsFromMessages(aMessages, aKeywords);
  if (NS_SUCCEEDED(rv))
  {
    nsCAutoString messageIds;
    nsTArray<nsMsgKey> keys;
    nsresult rv = BuildIdsAndKeyArray(aMessages, messageIds, keys);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = StoreCustomKeywords(nsnull, EmptyCString(), aKeywords, keys.Elements(), keys.Length(), nsnull);
    if (mDatabase)
      mDatabase->Commit(nsMsgDBCommitType::kLargeCommit);
  }
  return rv;
}

NS_IMETHODIMP nsImapMailFolder::GetCustomIdentity(nsIMsgIdentity **aIdentity)
{
  NS_ENSURE_ARG_POINTER(aIdentity);
  if (mFlags & nsMsgFolderFlags::ImapOtherUser)
  {
    nsresult rv;
    PRBool delegateOtherUsersFolders = PR_FALSE;
    nsCOMPtr<nsIPrefBranch> prefBranch(do_GetService(NS_PREFSERVICE_CONTRACTID, &rv));
    NS_ENSURE_SUCCESS(rv, rv);
    prefBranch->GetBoolPref("mail.imap.delegateOtherUsersFolders", &delegateOtherUsersFolders);
    // if we're automatically delegating other user's folders, we need to
    // cons up an e-mail address for the other user. We do that by
    // taking the other user's name and the current user's domain name,
    // assuming they'll be the same. So, <otherUsersName>@<ourDomain>
    if (delegateOtherUsersFolders)
    {
      nsCOMPtr<nsIMsgIncomingServer> server = do_QueryReferent(mServer, &rv);
      NS_ENSURE_SUCCESS(rv, rv);
      nsCOMPtr<nsIMsgAccountManager> accountManager = do_GetService(NS_MSGACCOUNTMANAGER_CONTRACTID, &rv);
      NS_ENSURE_SUCCESS(rv, rv);
      nsCOMPtr <nsIMsgIdentity> ourIdentity;
      nsCOMPtr <nsIMsgIdentity> retIdentity;
      nsCOMPtr <nsIMsgAccount> account;
      nsCString foldersUserName;
      nsCString ourEmailAddress;

      accountManager->FindAccountForServer(server, getter_AddRefs(account));
      NS_ENSURE_SUCCESS(rv, rv);
      account->GetDefaultIdentity(getter_AddRefs(ourIdentity));
      NS_ENSURE_SUCCESS(rv, rv);
      ourIdentity->GetEmail(ourEmailAddress);
      PRInt32 atPos = ourEmailAddress.FindChar('@');
      if (atPos != -1)
      {
        nsCString otherUsersEmailAddress;
        GetFolderOwnerUserName(otherUsersEmailAddress);
        otherUsersEmailAddress.Append(Substring(ourEmailAddress, atPos, ourEmailAddress.Length()));
        nsCOMPtr <nsISupportsArray> identities;
        rv = accountManager->GetIdentitiesForServer(server, getter_AddRefs(identities));
        NS_ENSURE_SUCCESS(rv, rv);
        PRUint32 numIdentities;
        rv = identities->Count(&numIdentities);
        NS_ENSURE_SUCCESS(rv, rv);
        for (PRUint32 identityIndex = 0; identityIndex < numIdentities; identityIndex++)
        {
          nsCOMPtr<nsIMsgIdentity> identity = do_QueryElementAt(identities, identityIndex);
          if (!identity)
            continue;
          nsCString identityEmail;
          identity->GetEmail(identityEmail);
          if (identityEmail.Equals(otherUsersEmailAddress))
          {
            retIdentity = identity;;
            break;
          }
        }
        if (!retIdentity)
        {
          // create the identity
          rv = accountManager->CreateIdentity(getter_AddRefs(retIdentity));
          NS_ENSURE_SUCCESS(rv, rv);
          retIdentity->SetEmail(otherUsersEmailAddress);
          nsCOMPtr <nsIMsgAccount> account;
          accountManager->FindAccountForServer(server, getter_AddRefs(account));
          NS_ENSURE_SUCCESS(rv, rv);
          account->AddIdentity(retIdentity);
        }
      }
      if (retIdentity)
      {
        retIdentity.swap(*aIdentity);
        return NS_OK;
      }
    }
  }
  return nsMsgDBFolder::GetCustomIdentity(aIdentity);
}

NS_IMETHODIMP nsImapMailFolder::ChangePendingTotal(PRInt32 aDelta)
{
  ChangeNumPendingTotalMessages(aDelta);
  return NS_OK;
}

/* void changePendingUnread (in long aDelta); */
NS_IMETHODIMP nsImapMailFolder::ChangePendingUnread(PRInt32 aDelta)
{
  ChangeNumPendingUnread(aDelta);
  return NS_OK;
}

nsresult nsImapMailFolder::CreatePlaybackTimer()
{
  nsresult rv = NS_OK;
  if (!m_playbackTimer)
  {
    m_playbackTimer = do_CreateInstance(NS_TIMER_CONTRACTID, &rv);
    NS_ASSERTION(NS_SUCCEEDED(rv),"failed to create pseudo-offline operation timer in nsImapMailFolder");
  }
  return rv;
}

void nsImapMailFolder::PlaybackTimerCallback(nsITimer *aTimer, void *aClosure)
{
  nsPlaybackRequest *request = static_cast<nsPlaybackRequest*>(aClosure);
  
  NS_ASSERTION(request->SrcFolder->m_pendingPlaybackReq == request, "wrong playback request pointer");
  
  nsImapOfflineSync *offlineSync = new nsImapOfflineSync(request->MsgWindow, nsnull, request->SrcFolder, PR_TRUE);
  if (offlineSync)
  {
    nsresult rv = offlineSync->ProcessNextOperation();
    NS_ASSERTION(NS_SUCCEEDED(rv), "pseudo-offline playback is not successful");
  }
  
  // release request struct
  request->SrcFolder->m_pendingPlaybackReq = nsnull;
  delete request;
}
