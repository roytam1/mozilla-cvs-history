/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

/********************************************************************************************************
 
   Interface for representing Messenger folders.
 
*********************************************************************************************************/

#ifndef nsMsgFolder_h__
#define nsMsgFolder_h__

#include "msgCore.h"
#include "nsIMsgFolder.h" /* include the interface we are going to support */
#include "nsRDFResource.h"
#include "nsIDBFolderInfo.h"
#include "nsIMsgDatabase.h"
#include "nsIMsgIncomingServer.h"
#include "nsCOMPtr.h"
#include "nsIURL.h"
 /* 
  * MsgFolder
  */ 

class NS_MSG_BASE nsMsgFolder: public nsRDFResource, public nsIMsgFolder
{
public: 
  nsMsgFolder(void);
  virtual ~nsMsgFolder(void);

  NS_DECL_ISUPPORTS_INHERITED

  NS_DECL_NSICOLLECTION
  NS_DECL_NSIFOLDER
  // eventually this will be an instantiable class, and we should
  // use this macro:
  // NS_DECL_NSIMSGFOLDER
  
  // right now a few of these methods are left abstract, and
  // are commented out below
  
  // begin NS_DECL_NSIMSGFOLDER
  NS_IMETHOD AddUnique(nsISupports *element);
  NS_IMETHOD ReplaceElement(nsISupports *element, nsISupports *newElement);
  NS_IMETHOD GetMessages(nsISimpleEnumerator **_retval);
  NS_IMETHOD GetThreads(nsISimpleEnumerator **_retval);
  NS_IMETHOD StartFolderLoading(void);
  NS_IMETHOD EndFolderLoading(void);
  NS_IMETHOD UpdateFolder(nsIMsgWindow *window);
  NS_IMETHOD GetThreadForMessage(nsIMessage *message, nsIMsgThread **_retval);
  NS_IMETHOD HasMessage(nsIMessage *message, PRBool *_retval);
  NS_IMETHOD GetVisibleSubFolders(nsIEnumerator **_retval);
  NS_IMETHOD GetPrettiestName(PRUnichar * *aPrettiestName);
  NS_IMETHOD GetFolderURL(char * *aFolderURL);
  NS_IMETHOD GetDeleteIsMoveToTrash(PRBool *aDeleteIsMoveToTrash);
  NS_IMETHOD GetShowDeletedMessages(PRBool *aShowDeletedMessages);
  NS_IMETHOD GetServer(nsIMsgIncomingServer * *aServer);
  NS_IMETHOD GetIsServer(PRBool *aIsServer);
  NS_IMETHOD GetCanSubscribe(PRBool *aCanSubscribe);
  NS_IMETHOD GetCanFileMessages(PRBool *aCanFileMessages);
  NS_IMETHOD GetCanCreateSubfolders(PRBool *aCanCreateSubfolders);
  NS_IMETHOD GetCanRename(PRBool *aCanRename);
  NS_IMETHOD OnCloseFolder(void);
  NS_IMETHOD Delete(void);
  NS_IMETHOD DeleteSubFolders(nsISupportsArray *folders);
  NS_IMETHOD PropagateDelete(nsIMsgFolder *folder, PRBool deleteStorage);
  NS_IMETHOD RecursiveDelete(PRBool deleteStorage);
  NS_IMETHOD CreateSubfolder(const char *folderName);
  NS_IMETHOD AddSubfolder(nsAutoString *folderName, nsIMsgFolder **newFolder);
  NS_IMETHOD Compact(void);
  NS_IMETHOD EmptyTrash(void);
  NS_IMETHOD Rename(const char *name);
  NS_IMETHOD Adopt(nsIMsgFolder *srcFolder, PRUint32 *outPos);
  NS_IMETHOD ContainsChildNamed(const char *name, PRBool *_retval);
  NS_IMETHOD IsAncestorOf(nsIMsgFolder *folder, PRBool *_retval);
  NS_IMETHOD GenerateUniqueSubfolderName(const char *prefix, nsIMsgFolder *otherFolder, char **_retval); 
  NS_IMETHOD UpdateSummaryTotals(PRBool force);
  NS_IMETHOD SummaryChanged(void);
  NS_IMETHOD GetNumUnread(PRBool deep, PRInt32 *_retval);
  NS_IMETHOD GetTotalMessages(PRBool deep, PRInt32 *_retval);
  NS_IMETHOD GetExpungedBytesCount(PRUint32 *aExpungedBytesCount);
  NS_IMETHOD GetDeletable(PRBool *aDeletable);
  NS_IMETHOD GetCanCreateChildren(PRBool *aCanCreateChildren);
  NS_IMETHOD GetCanBeRenamed(PRBool *aCanBeRenamed);
  NS_IMETHOD GetRequiresCleanup(PRBool *aRequiresCleanup);
  NS_IMETHOD ClearRequiresCleanup(void);
  NS_IMETHOD ManyHeadersToDownload(PRBool *_retval);
  NS_IMETHOD GetKnowsSearchNntpExtension(PRBool *aKnowsSearchNntpExtension);
  NS_IMETHOD GetAllowsPosting(PRBool *aAllowsPosting);
  NS_IMETHOD GetDisplayRecipients(PRBool *aDisplayRecipients);
  NS_IMETHOD GetRelativePathName(char * *aRelativePathName);
  NS_IMETHOD GetSizeOnDisk(PRUint32 *aSizeOnDisk);
  NS_IMETHOD RememberPassword(const char *password);
  NS_IMETHOD GetRememberedPassword(char * *aRememberedPassword);
  NS_IMETHOD UserNeedsToAuthenticateForFolder(PRBool displayOnly, PRBool *_retval);
  NS_IMETHOD GetUsername(char * *aUsername);
  NS_IMETHOD GetHostname(char * *aHostname);
  NS_IMETHOD SetFlag(PRUint32 flag);
  NS_IMETHOD ClearFlag(PRUint32 flag);
  NS_IMETHOD GetFlag(PRUint32 flag, PRBool *_retval);
  NS_IMETHOD ToggleFlag(PRUint32 flag);
  NS_IMETHOD OnFlagChange(PRUint32 flag);
  NS_IMETHOD GetFlags(PRUint32 *aFlags);
  NS_IMETHOD GetFoldersWithFlag(PRUint32 flags, nsIMsgFolder **result, PRUint32 resultsize, PRUint32 *numFolders);
  NS_IMETHOD GetExpansionArray(nsISupportsArray *expansionArray);
  // NS_IMETHOD DeleteMessages(nsISupportsArray *message, nsITransactionManager *txnMgr, PRBool deleteStorage);
  NS_IMETHOD CopyMessages(nsIMsgFolder *srcFolder, nsISupportsArray *messages, PRBool isMove, nsIMsgWindow *window, nsIMsgCopyServiceListener *listener);
  NS_IMETHOD CopyFileMessage(nsIFileSpec *fileSpec, nsIMessage *msgToReplace, PRBool isDraft, nsIMsgWindow *window, nsIMsgCopyServiceListener *listener);
  NS_IMETHOD AcquireSemaphore(nsISupports *semHolder);
  NS_IMETHOD ReleaseSemaphore(nsISupports *semHolder);
  NS_IMETHOD TestSemaphore(nsISupports *semHolder, PRBool *_retval);
  NS_IMETHOD GetLocked(PRBool *aLocked);
  // NS_IMETHOD CreateMessageFromMsgDBHdr(nsIMsgDBHdr *msgDBHdr, nsIMessage **_retval);
  NS_IMETHOD GetNewMessages(nsIMsgWindow *window);
  // NS_IMETHOD WriteToFolderCache(nsIMsgFolderCache *folderCache);
  // NS_IMETHOD GetCharset(PRUnichar * *aCharset);
  // NS_IMETHOD SetCharset(const PRUnichar * aCharset);
  NS_IMETHOD GetBiffState(PRUint32 *aBiffState);
  NS_IMETHOD SetBiffState(PRUint32 aBiffState);
  NS_IMETHOD GetNumNewMessages(PRInt32 *aNumNewMessages);
  NS_IMETHOD SetNumNewMessages(PRInt32 aNumNewMessages);
  NS_IMETHOD GetNewMessagesNotificationDescription(PRUnichar * *aNewMessagesNotificationDescription);
  NS_IMETHOD GetRootFolder(nsIMsgFolder * *aRootFolder);
  NS_IMETHOD GetMsgDatabase(nsIMsgDatabase * *aMsgDatabase);
  NS_IMETHOD GetPath(nsIFileSpec * *aPath);
  NS_IMETHOD MarkMessagesRead(nsISupportsArray *messages, PRBool markRead);
  NS_IMETHOD MarkAllMessagesRead(void);
  NS_IMETHOD MarkMessagesFlagged(nsISupportsArray *messages, PRBool markFlagged);
  NS_IMETHOD GetChildWithURI(const char *uri, PRBool deep, nsIMsgFolder **_retval); 

  // end NS_DECL_NSIMSGFOLDER
  
  // nsRDFResource overrides
  NS_IMETHOD Init(const char* aURI);
  
#if 0
  static nsresult GetRoot(nsIMsgFolder* *result);
#endif
  // Gets the URL that represents the given message.  Returns a newly
  // created string that must be free'd using XP_FREE().
  // If the db is NULL, then returns a URL that represents the entire
  // folder as a whole.
	// These functions are used for tricking the front end into thinking that we have more 
	// messages than are really in the DB.  This is usually after and IMAP message copy where
	// we don't want to do an expensive select until the user actually opens that folder
	// These functions are called when MSG_Master::GetFolderLineById is populating a MSG_FolderLine
	// struct used by the FE
  PRInt32 GetNumPendingUnread();
  PRInt32 GetNumPendingTotalMessages();
	
  void			ChangeNumPendingUnread(PRInt32 delta);
  void			ChangeNumPendingTotalMessages(PRInt32 delta);


#ifdef HAVE_DB
  NS_IMETHOD BuildUrl(nsMsgDatabase *db, nsMsgKey key, char ** url);
  
  // updates num messages and num unread - should be pure virtual
  // when I get around to implementing in all subclasses?
  NS_IMETHOD GetTotalMessagesInDB(PRUint32 *totalMessages) const;					// How many messages in database.
  NS_IMETHOD SetFolderPrefFlags(PRUint32 flags);
  NS_IMETHOD GetFolderPrefFlags(PRUint32 *flags);


  NS_IMETHOD SetLastMessageLoaded(nsMsgKey lastMessageLoaded);
  NS_IMETHOD GetLastMessageLoaded();
#endif


#ifdef HAVE_ADMINURL
  NS_IMETHOD GetAdminUrl(MWContext *context, MSG_AdminURLType type);
  NS_IMETHOD HaveAdminUrl(MSG_AdminURLType type, PRBool *hadAdminUrl);
#endif


#ifdef HAVE_PANE
  NS_IMETHOD	MarkAllRead(MSG_Pane *pane, PRBool deep);
  NS_IMETHOD SetFlagInAllFolderPanes(PRUint32 which);
#endif

#ifdef HAVE_NET
  NS_IMETHOD EscapeMessageId(const char *messageId, const char **escapeMessageID);
  NS_IMETHOD ShouldPerformOperationOffline(PRBool *performOffline);
#endif

#ifdef HAVE_CACHE
	virtual nsresult WriteToCache(XP_File);
	virtual nsresult ReadFromCache(char *);
	virtual PRBool IsCachable();
	void SkipCacheTokens(char **ppBuf, int numTokens);
#endif

#ifdef DOES_FOLDEROPERATIONS
	int DownloadToTempFileAndUpload(MessageCopyInfo *copyInfo, nsMsgKeyArray &keysToSave, MSG_FolderInfo *dstFolder, nsMsgDatabase *sourceDB);
	void UpdateMoveCopyStatus(MWContext *context, PRBool isMove, int32 curMsgCount, int32 totMessages);
#endif

	virtual nsresult GetDBFolderInfoAndDB(nsIDBFolderInfo **folderInfo, nsIMsgDatabase **db) = 0;


	NS_IMETHOD MatchName(nsString *name, PRBool *matches);


protected:
	nsresult NotifyPropertyChanged(char *property, char* oldValue, char* newValue);
	nsresult NotifyIntPropertyChanged(char *property, PRInt32 oldValue, PRInt32 newValue);
	nsresult NotifyBoolPropertyChanged(char *property, PRBool oldValue, PRBool newValue);
	nsresult NotifyPropertyFlagChanged(nsISupports *item, char *property, PRUint32 oldValue,
												PRUint32 newValue);
	nsresult NotifyItemAdded(nsISupports *parentItem, nsISupports *item, const char *viewString);
	nsresult NotifyItemDeleted(nsISupports *parentItem, nsISupports *item, const char* viewString);

	nsresult NotifyFolderLoaded();
	// this is a little helper function that is not part of the public interface. 
	// we use it to get the IID of the incoming server for the derived folder.
	// w/out a function like this we would have to implement GetServer in each
	// derived folder class.
	virtual const char* GetIncomingServerType() = 0;

protected:
  PRUint32 mFlags;
  nsIFolder *mParent;     //This won't be refcounted for ownership reasons.
  PRInt32 mNumUnreadMessages;        /* count of unread messages (-1 means
                                         unknown; -2 means unknown but we already
                                         tried to find out.) */
  PRInt32 mNumTotalMessages;         /* count of existing messages. */
  nsCOMPtr<nsISupportsArray> mSubFolders;
  nsVoidArray *mListeners; //This can't be an nsISupportsArray because due to
													 //ownership issues, listeners can't be AddRef'd

  PRInt32 mPrefFlags;       // prefs like MSG_PREF_OFFLINE, MSG_PREF_ONE_PANE, etc
  nsISupports *mSemaphoreHolder; // set when the folder is being written to
								//Due to ownership issues, this won't be AddRef'd.

  nsIMsgIncomingServer* m_server; //this won't be addrefed....ownership issue here

#ifdef HAVE_DB
  nsMsgKey	m_lastMessageLoaded;
#endif
  // These values are used for tricking the front end into thinking that we have more 
  // messages than are really in the DB.  This is usually after and IMAP message copy where
  // we don't want to do an expensive select until the user actually opens that folder
  PRInt32 mNumPendingUnreadMessages;
  PRInt32 mNumPendingTotalMessages;

  PRUint32	mBiffState;
  PRInt32	mNumNewBiffMessages;

  PRBool mIsCachable;

  //
  // stuff from the uri
  //
  
  PRBool mIsServer;
  nsString mName;
  
};

#endif
