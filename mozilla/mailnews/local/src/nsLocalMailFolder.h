/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

/********************************************************************************************************
 
   Interface for representing Messenger folders.
 
*********************************************************************************************************/

#ifndef nsMsgLocalMailFolder_h__
#define nsMsgLocalMailFolder_h__

#include "nsMsgFolder.h" /* include the interface we are going to support */
#include "nsMailDatabase.h"
#include "nsFileSpec.h"
#include "nsIDBChangeListener.h"
#include "nsICopyMessageListener.h"
#include "nsFileStream.h"

typedef struct {
	nsOutputFileStream *fileStream;
	nsIMessage *message;
	nsMsgKey dstKey;
} nsLocalMailCopyState;

class nsMsgLocalMailFolder : public nsMsgFolder, public nsIMsgLocalMailFolder,
							public nsIDBChangeListener, public nsICopyMessageListener
{
public:
	nsMsgLocalMailFolder(void);
	virtual ~nsMsgLocalMailFolder(void);

  NS_DECL_ISUPPORTS_INHERITED
#if 0
  static nsresult GetRoot(nsIMsgFolder* *result);
#endif
  // nsICollection methods:
  NS_IMETHOD Enumerate(nsIEnumerator* *result);

  // nsIFolder methods:
  NS_IMETHOD GetSubFolders(nsIEnumerator* *result);

  // nsIMsgFolder methods:
  NS_IMETHOD AddUnique(nsISupports* element);
  NS_IMETHOD ReplaceElement(nsISupports* element, nsISupports* newElement);
  NS_IMETHOD GetMessages(nsIEnumerator* *result);

#ifdef HAVE_DB	
	virtual nsresult BeginCopyingMessages(MSG_FolderInfo *dstFolder, 
																				MessageDB *sourceDB,
																				IDArray *srcArray, 
																				MSG_UrlQueue *urlQueue,
																				int32 srcCount,
																				MessageCopyInfo *copyInfo);


	virtual int FinishCopyingMessages (MWContext *context,
																			MSG_FolderInfo * srcFolder, 
																			MSG_FolderInfo *dstFolder, 
																			MessageDB *sourceDB,
																			IDArray **ppSrcArray, 
																			int32 srcCount,
																			msg_move_state *state);
#endif

	NS_IMETHOD CreateSubfolder(char *leafNameFromUser, nsIMsgFolder **outFolder, PRUint32 *outPos);

	NS_IMETHOD RemoveSubFolder (nsIMsgFolder *which);
	NS_IMETHOD Delete ();
	NS_IMETHOD Rename (const char *newName);
	NS_IMETHOD Adopt(nsIMsgFolder *srcFolder, PRUint32 *outPos);

  NS_IMETHOD GetChildNamed(nsString& name, nsISupports ** aChild);

  // this override pulls the value from the db
	NS_IMETHOD GetName(char ** name);   // Name of this folder (as presented to user).
	NS_IMETHOD GetPrettyName(nsString& prettyName);	// Override of the base, for top-level mail folder

  NS_IMETHOD BuildFolderURL(char **url);

	NS_IMETHOD UpdateSummaryTotals() ;

	NS_IMETHOD GetExpungedBytesCount(PRUint32 *count);
	NS_IMETHOD GetDeletable (PRBool *deletable); 
	NS_IMETHOD GetCanCreateChildren (PRBool *canCreateChildren) ;
	NS_IMETHOD GetCanBeRenamed (PRBool *canBeRenamed);
	NS_IMETHOD GetRequiresCleanup(PRBool *requiresCleanup);

	NS_IMETHOD GetSizeOnDisk(PRUint32 size);

	NS_IMETHOD GetUsersName(char** userName);
	NS_IMETHOD GetHostName(char** hostName);
	NS_IMETHOD UserNeedsToAuthenticateForFolder(PRBool displayOnly, PRBool *authenticate);
	NS_IMETHOD RememberPassword(char *password);
	NS_IMETHOD GetRememberedPassword(char ** password);

  virtual nsresult GetDBFolderInfoAndDB(nsIDBFolderInfo **folderInfo, nsMsgDatabase **db);

 	NS_IMETHOD DeleteMessage(nsIMessage *message);

	// nsIMsgMailFolder
  NS_IMETHOD GetPath(nsNativeFileSpec& aPathName);

	//nsIDBChangeListener
	NS_IMETHOD OnKeyChange(nsMsgKey aKeyChanged, int32 aFlags, 
                         nsIDBChangeListener * aInstigator);
	NS_IMETHOD OnKeyDeleted(nsMsgKey aKeyChanged, int32 aFlags, 
                          nsIDBChangeListener * aInstigator);
	NS_IMETHOD OnKeyAdded(nsMsgKey aKeyChanged, int32 aFlags, 
                        nsIDBChangeListener * aInstigator);
	NS_IMETHOD OnAnnouncerGoingAway(nsIDBChangeAnnouncer * instigator);

	//nsICopyMessageListener
	NS_IMETHOD BeginCopy(nsIMessage *message);
	NS_IMETHOD CopyData(nsIInputStream *aIStream, PRInt32 aLength);
	NS_IMETHOD EndCopy(PRBool copySucceeded);


protected:
	nsresult ParseFolder(nsFileSpec& path);
	nsresult CreateSubFolders(nsFileSpec &path);
	nsresult AddDirectorySeparator(nsFileSpec &path);


protected:
  nsNativeFileSpec mPath;
	PRUint32  mExpungedBytes;
	PRBool		mHaveReadNameFromDB;
	PRBool		mGettingMail;
	PRBool		mInitialized;
	nsISupportsArray *mMessages;
	nsMailDatabase* mMailDatabase;
	nsLocalMailCopyState *mCopyState; //We will only allow one of these at a time
};

#endif // nsMsgLocalMailFolder_h__
