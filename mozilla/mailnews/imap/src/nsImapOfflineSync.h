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
 * Copyright (C) 1999 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

#ifndef _nsImapOfflineSync_H_
#define _nsImapOfflineSync_H_


#include "nsIMsgDatabase.h"
#include "nsIUrlListener.h"
#include "nsIMsgOfflineImapOperation.h"
#include "nsIMsgWindow.h"
#include "nsIMsgFolder.h"

class nsImapOfflineSync : public nsIUrlListener {
public:												// set to one folder to playback one folder only
	nsImapOfflineSync(nsIMsgWindow *window, nsIUrlListener *listener, nsIMsgFolder *singleFolderOnly = nsnull);
	virtual ~nsImapOfflineSync();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIURLLISTENER
	virtual nsresult		ProcessNextOperation(); // this kicks off playback
	
	PRInt32		GetCurrentUIDValidity();
	void		SetCurrentUIDValidity(PRInt32 uidvalidity) { mCurrentUIDValidity = uidvalidity; }
	
	void		SetPseudoOffline(PRBool pseudoOffline) {m_pseudoOffline = pseudoOffline;}
	PRBool		ProcessingStaleFolderUpdate() { return m_singleFolderToUpdate != nsnull; }

	PRBool		CreateOfflineFolder(nsIMsgFolder *folder);
  void      SetWindow(nsIMsgWindow *window);
protected:
	PRBool		CreateOfflineFolders();
  nsresult  AdvanceToNextServer();
	nsresult  AdvanceToNextFolder();
	void		AdvanceToFirstIMAPFolder();
	void 		DeleteAllOfflineOpsForCurrentDB();
	
	void		ProcessFlagOperation(nsIMsgOfflineImapOperation *currentOp);
	void		ProcessMoveOperation(nsIMsgOfflineImapOperation *currentOp);
	void		ProcessCopyOperation(nsIMsgOfflineImapOperation *currentOp);
	void		ProcessEmptyTrash(nsIMsgOfflineImapOperation *currentOp);
	void		ProcessAppendMsgOperation(nsIMsgOfflineImapOperation *currentOp,
										  nsOfflineImapOperationType opType);
	
	nsCOMPtr <nsIMsgFolder>	m_currentFolder;
	nsCOMPtr <nsIMsgFolder> m_singleFolderToUpdate;
  nsCOMPtr <nsIMsgWindow> m_window;
  nsCOMPtr <nsISupportsArray> m_allServers;
  nsCOMPtr <nsISupportsArray> m_allFolders;
  nsCOMPtr <nsIMsgIncomingServer> m_currentServer;
  nsCOMPtr <nsIEnumerator> m_serverEnumerator;

	nsMsgKeyArray				m_CurrentKeys;
	PRUint32					m_KeyIndex;
	nsCOMPtr <nsIMsgDatabase>				m_currentDB;
  nsCOMPtr <nsIUrlListener> m_listener;
	PRInt32				mCurrentUIDValidity;
	PRInt32				mCurrentPlaybackOpType;	// kFlagsChanged -> kMsgCopy -> kMsgMoved
	PRBool				m_mailboxupdatesStarted;
	PRBool				m_pseudoOffline;		// for queueing online events in offline db
	PRBool				m_createdOfflineFolders;

};

class nsImapOfflineDownloader : public nsImapOfflineSync
{
public:
  nsImapOfflineDownloader(nsIMsgWindow *window, nsIUrlListener *listener);
  virtual ~nsImapOfflineDownloader();
	virtual nsresult		ProcessNextOperation(); // this kicks off download
};

#endif
