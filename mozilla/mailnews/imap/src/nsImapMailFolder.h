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
 */
#ifndef nsImapMailFolder_h__
#define nsImapMailFolder_h__

#include "nsImapCore.h"
#include "nsMsgDBFolder.h"
#include "nsIMessage.h"
#include "nsIImapMailFolderSink.h"
#include "nsIImapMessageSink.h"
#include "nsIImapExtensionSink.h"
#include "nsIImapMiscellaneousSink.h"
#include "nsICopyMessageListener.h"
#include "nsIImapService.h"
#include "nsIUrlListener.h"
#include "nsIImapIncomingServer.h" // we need this for its IID
#include "nsIMsgParseMailMsgState.h"
#include "nsITransactionManager.h"
#include "nsMsgTxn.h"
#include "nsIMsgMessageService.h"
#include "nsIMsgFilterHitNotify.h"
#include "nsIMsgFilterList.h"
#include "prmon.h"
#include "nsIEventQueue.h"
#include "nsIMsgImapMailFolder.h"
#include "nsIImapMailFolderSink.h"
#include "nsIImapServerSink.h"
#include "nsIStreamListener.h"
class nsImapMoveCoalescer;


#define FOUR_K 4096

/* b64534f0-3d53-11d3-ac2a-00805f8ac968 */

#define NS_IMAPMAILCOPYSTATE_IID \
{ 0xb64534f0, 0x3d53, 0x11d3, \
    { 0xac, 0x2a, 0x00, 0x80, 0x5f, 0x8a, 0xc9, 0x68 } }

class nsImapMailCopyState: public nsISupports
{
public:
    static const nsIID& GetIID()
    {
        static nsIID iid = NS_IMAPMAILCOPYSTATE_IID;
        return iid;
    }
    
    NS_DECL_ISUPPORTS

    nsImapMailCopyState();
    virtual ~nsImapMailCopyState();

    nsCOMPtr<nsISupports> m_srcSupport; // source file spec or folder
    nsCOMPtr<nsISupportsArray> m_messages; // array of source messages
    nsCOMPtr<nsMsgTxn> m_undoMsgTxn; // undo object with this copy operation
    nsCOMPtr<nsIMessage> m_message; // current message to be copied
    nsCOMPtr<nsIMsgCopyServiceListener> m_listener; // listener of this copy
                                                    // operation 
    nsCOMPtr<nsIFileSpec> m_tmpFileSpec; // temp file spec for copy operation
    nsCOMPtr<nsIMsgWindow> m_msgWindow; // msg window for copy operation

    nsIMsgMessageService* m_msgService; // source folder message service; can
                                        // be Nntp, Mailbox, or Imap
    PRBool m_isMove;             // is a move
    PRBool m_selectedState;      // needs to be in selected state; append msg
    PRBool m_isCrossServerOp; // are we copying between imap servers?
    PRUint32 m_curIndex; // message index to the message array which we are
                         // copying 
    PRUint32 m_totalCount;// total count of messages we have to do
    PRBool m_streamCopy;
    char *m_dataBuffer; // temporary buffer for this copy operation
    PRUint32 m_leftOver;
};

class nsImapMailFolder : public nsMsgDBFolder, 
                         public nsIMsgImapMailFolder,
                         public nsIImapMailFolderSink,
                         public nsIImapMessageSink,
                         public nsIImapExtensionSink,
                         public nsIImapMiscellaneousSink,
                         public nsICopyMessageListener,
						             public nsIMsgFilterHitNotify,
                         public nsIStreamListener
{
public:
	nsImapMailFolder();
	virtual ~nsImapMailFolder();

	NS_DECL_ISUPPORTS_INHERITED

    // nsICollection methods
	NS_IMETHOD Enumerate(nsIEnumerator **result);

  // nsIFolder methods:
  NS_IMETHOD GetSubFolders(nsIEnumerator* *result);
  
  // nsIMsgFolder methods:
  NS_IMETHOD AddUnique(nsISupports* element);
  NS_IMETHOD ReplaceElement(nsISupports* element, nsISupports* newElement);
  NS_IMETHOD GetMessages(nsIMsgWindow *aMsgWindow, nsISimpleEnumerator* *result);
	NS_IMETHOD UpdateFolder(nsIMsgWindow *aWindow);
    
	NS_IMETHOD CreateSubfolder(const PRUnichar *folderName,nsIMsgWindow *msgWindow );
	NS_IMETHOD AddSubfolderWithPath(nsAutoString *name, nsIFileSpec *dbPath, nsIMsgFolder **child);
  NS_IMETHODIMP CreateStorageIfMissing(nsIUrlListener* urlListener);
    
  NS_IMETHOD Compact(nsIUrlListener *aListener);
  NS_IMETHOD EmptyTrash(nsIMsgWindow *msgWindow, nsIUrlListener *aListener);
	NS_IMETHOD Delete ();
	NS_IMETHOD Rename (const PRUnichar *newName, nsIMsgWindow *msgWindow);
	NS_IMETHOD Adopt(nsIMsgFolder *srcFolder, PRUint32 *outPos);
    NS_IMETHOD GetNoSelect(PRBool *aResult);

	NS_IMETHOD GetPrettyName(PRUnichar ** prettyName);	// Override of the base, for top-level mail folder
    
  NS_IMETHOD GetFolderURL(char **url);
    
	NS_IMETHOD UpdateSummaryTotals(PRBool force) ;
    
	NS_IMETHOD GetDeletable (PRBool *deletable); 
	NS_IMETHOD GetRequiresCleanup(PRBool *requiresCleanup);
    
	NS_IMETHOD GetSizeOnDisk(PRUint32 * size);
        
  NS_IMETHOD GetCanCreateSubfolders(PRBool *aResult);
	NS_IMETHOD GetCanSubscribe(PRBool *aResult);	

	NS_IMETHOD UserNeedsToAuthenticateForFolder(PRBool displayOnly, PRBool *authenticate);
	NS_IMETHOD RememberPassword(const char *password);
	NS_IMETHOD GetRememberedPassword(char ** password);

  NS_IMETHOD AddMessageDispositionState(nsIMessage *aMessage, nsMsgDispositionState aDispositionFlag);
	NS_IMETHOD MarkMessagesRead(nsISupportsArray *messages, PRBool markRead);
	NS_IMETHOD MarkAllMessagesRead(void);
	NS_IMETHOD MarkMessagesFlagged(nsISupportsArray *messages, PRBool markFlagged);
  NS_IMETHOD MarkThreadRead(nsIMsgThread *thread);

  NS_IMETHOD DeleteSubFolders(nsISupportsArray *folders, nsIMsgWindow *msgWindow);
	NS_IMETHOD ReadFromFolderCacheElem(nsIMsgFolderCacheElement *element);
	NS_IMETHOD WriteToFolderCacheElem(nsIMsgFolderCacheElement *element);
    
  NS_IMETHOD GetDBFolderInfoAndDB(nsIDBFolderInfo **folderInfo,
                                          nsIMsgDatabase **db);
 	NS_IMETHOD DeleteMessages(nsISupportsArray *messages,
                              nsIMsgWindow *msgWindow, PRBool
                              deleteStorage, PRBool isMove);
  NS_IMETHOD CopyMessages(nsIMsgFolder *srcFolder, 
                            nsISupportsArray* messages,
                            PRBool isMove, nsIMsgWindow *msgWindow,
                            nsIMsgCopyServiceListener* listener);
  NS_IMETHOD CopyFileMessage(nsIFileSpec* fileSpec, 
                               nsIMessage* msgToReplace,
                               PRBool isDraftOrTemplate,
                               nsIMsgWindow *msgWindow,
                               nsIMsgCopyServiceListener* listener);
	NS_IMETHOD CreateMessageFromMsgDBHdr(nsIMsgDBHdr *msgHdr, nsIMessage
                                         **message);
  NS_IMETHOD GetNewMessages(nsIMsgWindow *aWindow);

  NS_IMETHOD GetPath(nsIFileSpec** aPathName);
	NS_IMETHOD SetPath(nsIFileSpec * aPath);

  NS_IMETHOD Shutdown(PRBool shutdownChildren);

  NS_IMETHOD DownloadMessagesForOffline(nsISupportsArray *messages);

    // nsIMsgImapMailFolder methods
	NS_DECL_NSIMSGIMAPMAILFOLDER

    // nsIImapMailFolderSink methods
	NS_DECL_NSIIMAPMAILFOLDERSINK

    // nsIImapMessageSink methods
	NS_DECL_NSIIMAPMESSAGESINK

    //nsICopyMessageListener
	NS_DECL_NSICOPYMESSAGELISTENER

	NS_DECL_NSISTREAMOBSERVER

	NS_DECL_NSISTREAMLISTENER
    // nsIUrlListener methods
	NS_IMETHOD OnStartRunningUrl(nsIURI * aUrl);
	NS_IMETHOD OnStopRunningUrl(nsIURI * aUrl, nsresult aExitCode);

  // nsIImapExtensionSink methods
  NS_IMETHOD ClearFolderRights(nsIImapProtocol* aProtocol,
                               nsIMAPACLRightsInfo* aclRights);

  NS_IMETHOD AddFolderRights(nsIImapProtocol* aProtocol,
                             nsIMAPACLRightsInfo* aclRights);
  NS_IMETHOD RefreshFolderRights(nsIImapProtocol* aProtocol,
                                 nsIMAPACLRightsInfo* aclRights);
  NS_IMETHOD FolderNeedsACLInitialized(nsIImapProtocol* aProtocol,
                                       nsIMAPACLRightsInfo* aclRights);
  NS_IMETHOD SetCopyResponseUid(nsIImapProtocol* aProtocol,
                                nsMsgKeyArray* keyArray,
                                const char* msgIdString,
                                nsIImapUrl * aUrl);
  NS_IMETHOD SetAppendMsgUid(nsIImapProtocol* aProtocol,
                             nsMsgKey aKey,
                             nsIImapUrl * aUrl);
  NS_IMETHOD GetMessageId(nsIImapProtocol* aProtocol,
                          nsCString* messageId,
                          nsIImapUrl * aUrl);
    
    // nsIImapMiscellaneousSink methods
	NS_IMETHOD AddSearchResult(nsIImapProtocol* aProtocol, 
                               const char* searchHitLine);
	NS_IMETHOD GetArbitraryHeaders(nsIImapProtocol* aProtocol,
                                   GenericInfo* aInfo);
	NS_IMETHOD GetShouldDownloadArbitraryHeaders(nsIImapProtocol* aProtocol,
                                                 GenericInfo* aInfo);
	NS_IMETHOD HeaderFetchCompleted(nsIImapProtocol* aProtocol);
	NS_IMETHOD UpdateSecurityStatus(nsIImapProtocol* aProtocol);
	// ****
	NS_IMETHOD SetBiffStateAndUpdate(nsIImapProtocol* aProtocol,
                                     nsMsgBiffState biffState);
	NS_IMETHOD GetStoredUIDValidity(nsIImapProtocol* aProtocol,
                                    uid_validity_info* aInfo);
	NS_IMETHOD LiteSelectUIDValidity(nsIImapProtocol* aProtocol,
                                     PRUint32 uidValidity);
	NS_IMETHOD ProgressStatus(nsIImapProtocol* aProtocol,
                              PRUint32 aMsgId, const PRUnichar *extraInfo);
	NS_IMETHOD PercentProgress(nsIImapProtocol* aProtocol,
                               ProgressInfo* aInfo);
	NS_IMETHOD TunnelOutStream(nsIImapProtocol* aProtocol,
                               msg_line_info* aInfo);
	NS_IMETHOD ProcessTunnel(nsIImapProtocol* aProtocol,
                             TunnelInfo *aInfo);

  NS_IMETHOD CopyNextStreamMessage(nsIImapProtocol* aProtocol,
                                   nsIImapUrl * aUrl);
  NS_IMETHOD SetUrlState(nsIImapProtocol* aProtocol,
                         nsIMsgMailNewsUrl* aUrl,
                         PRBool isRunning,
                         nsresult statusCode);

	NS_IMETHOD MatchName(nsString *name, PRBool *matches);
	// nsIMsgFilterHitNotification method(s)
	NS_IMETHOD ApplyFilterHit(nsIMsgFilter *filter, PRBool *applyMore);


	nsresult MoveIncorporatedMessage(nsIMsgDBHdr *mailHdr, 
									   nsIMsgDatabase *sourceDB, 
                                     const char *destFolder,
									   nsIMsgFilter *filter);
	nsresult StoreImapFlags(imapMessageFlagsType flags, PRBool addFlags, nsMsgKeyArray &msgKeys);
	static nsresult AllocateUidStringFromKeyArray(nsMsgKeyArray &keyArray, nsCString &msgIds);
protected:
    // Helper methods
	void FindKeysToAdd(const nsMsgKeyArray &existingKeys, nsMsgKeyArray
                       &keysToFetch, nsIImapFlagAndUidState *flagState);
	void FindKeysToDelete(const nsMsgKeyArray &existingKeys, nsMsgKeyArray
                          &keysToFetch, nsIImapFlagAndUidState *flagState);
	void PrepareToAddHeadersToMailDB(nsIImapProtocol* aProtocol, const
                                     nsMsgKeyArray &keysToFetch, 
                                     nsIMailboxSpec *boxSpec);
	void TweakHeaderFlags(nsIImapProtocol* aProtocol, nsIMsgDBHdr *tweakMe);

	nsresult SyncFlags(nsIImapFlagAndUidState *flagState);

  nsresult MarkMessagesImapDeleted(nsMsgKeyArray *keyArray, PRBool deleted, nsIMsgDatabase *db);

	void UpdatePendingCounts(PRBool countUnread, PRBool missingAreRead);
	void SetIMAPDeletedFlag(nsIMsgDatabase *mailDB, const nsMsgKeyArray &msgids, PRBool markDeleted);
	virtual PRBool ShowDeletedMessages();
	virtual PRBool DeleteIsMoveToTrash();
	void ParseUidString(char *uidString, nsMsgKeyArray &keys);
  nsresult GetFolder(const char *name, nsIMsgFolder **pFolder);
	nsresult GetTrashFolder(nsIMsgFolder **pTrashFolder);
  PRBool TrashOrDescendentOfTrash(nsIMsgFolder* folder);
	nsresult GetServerKey(char **serverKey);
  nsresult GetImapIncomingServer(nsIImapIncomingServer **aImapIncomingServer);

  nsresult DisplayStatusMsg(nsIImapUrl *aImapUrl, const PRUnichar *msg);

  //nsresult RenameLocal(const char *newName);
  nsresult AddDirectorySeparator(nsFileSpec &path);
  nsresult CreateDirectoryForFolder(nsFileSpec &path);
	nsresult CreateSubFolders(nsFileSpec &path);
	nsresult GetDatabase(nsIMsgWindow *aMsgWindow);
	virtual const char *GetIncomingServerType() {return "imap";}

  // Uber message copy service
  nsresult CopyMessagesWithStream(nsIMsgFolder* srcFolder,
                         nsISupportsArray* messages,
                         PRBool isMove,
                         PRBool isCrossServerOp,
                         nsIMsgWindow *msgWindow,
                         nsIMsgCopyServiceListener* listener);
  nsresult CopyStreamMessage(nsIMessage* message, nsIMsgFolder* dstFolder,
                             nsIMsgWindow *msgWindow, PRBool isMove);
  nsresult InitCopyState(nsISupports* srcSupport, 
                         nsISupportsArray* messages,
                         PRBool isMove,
                         PRBool selectedState,
                         nsIMsgCopyServiceListener* listener,
                         nsIMsgWindow *msgWindow);
  void ClearCopyState(nsresult exitCode);
  nsresult SetTransactionManager(nsITransactionManager* txnMgr);
  nsresult BuildIdsAndKeyArray(nsISupportsArray* messages,
                               nsCString& msgIds, nsMsgKeyArray& keyArray);

	virtual nsresult CreateBaseMessageURI(const char *aURI);

  PRBool m_initialized;
  PRBool m_haveDiscoveredAllFolders;
  PRBool m_haveReadNameFromDB;
	nsCOMPtr<nsIMsgParseMailMsgState> m_msgParser;
	nsCOMPtr<nsIMsgFilterList> m_filterList;
	PRBool				m_msgMovedByFilter;
	nsImapMoveCoalescer *m_moveCoalescer;
	nsMsgKey			m_curMsgUid;
	PRInt32			m_nextMessageByteLength;
  nsCOMPtr<nsIEventQueue> m_eventQueue;
  PRBool m_urlRunning;

  // *** jt - undo move/copy trasaction support
  nsCOMPtr<nsITransactionManager> m_transactionManager;
  nsCOMPtr<nsMsgTxn> m_pendingUndoTxn;
  nsCOMPtr<nsImapMailCopyState> m_copyState;
  PRMonitor *m_appendMsgMonitor;
  PRBool	m_verifiedAsOnlineFolder;
	PRBool	m_explicitlyVerify; // whether or not we need to explicitly verify this through LIST
	PRUnichar m_hierarchyDelimiter;
	PRInt32 m_boxFlags;
	nsCString m_onlineFolderName;
	nsFileSpec *m_pathName;

  PRBool m_folderNeedsSubscribing;
  PRBool m_folderNeedsAdded;
  PRBool m_folderNeedsACLListed;

  nsCOMPtr<nsIMsgMailNewsUrl> mUrlToRelease;

  // offline imap support
  PRBool m_downloadMessageForOfflineUse;
};

#endif
