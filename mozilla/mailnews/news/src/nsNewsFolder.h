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
 
   Interface for representing News folders.
 
*********************************************************************************************************/

#ifndef nsMsgNewsFolder_h__
#define nsMsgNewsFolder_h__

#include "nsMsgDBFolder.h" 
#include "nsFileSpec.h"
#include "nsFileStream.h"
#include "nsINntpIncomingServer.h" // need this for the IID
#include "nsNewsUtils.h"
#include "nsMsgLineBuffer.h"
#include "nsMsgKeySet.h"

class nsMsgNewsFolder : public nsMsgDBFolder, public nsIMsgNewsFolder, public nsMsgLineBuffer
{
public:
	nsMsgNewsFolder(void);
	virtual ~nsMsgNewsFolder(void);

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

	NS_IMETHOD CreateSubfolder(const char *folderName);

	NS_IMETHOD RemoveSubFolder (nsIMsgFolder *which);
	NS_IMETHOD Delete ();
	NS_IMETHOD Rename (const char *newName);
	NS_IMETHOD Adopt(nsIMsgFolder *srcFolder, PRUint32 *outPos);

  NS_IMETHOD GetChildNamed(const char* name, nsISupports ** aChild);

  // this override pulls the value from the db
	NS_IMETHOD GetName(char ** name);   // Name of this folder (as presented to user).
	NS_IMETHOD GetPrettyName(char ** prettyName);	// Override of the base, for top-level news folder

  NS_IMETHOD BuildFolderURL(char **url);

	NS_IMETHOD UpdateSummaryTotals() ;

	NS_IMETHOD GetExpungedBytesCount(PRUint32 *count);
	NS_IMETHOD GetDeletable (PRBool *deletable); 
	NS_IMETHOD GetCanCreateChildren (PRBool *canCreateChildren) ;
	NS_IMETHOD GetCanBeRenamed (PRBool *canBeRenamed);
	NS_IMETHOD GetRequiresCleanup(PRBool *requiresCleanup);

	NS_IMETHOD GetSizeOnDisk(PRUint32 *size);

	NS_IMETHOD GetUsername(char** userName);
	NS_IMETHOD GetHostname(char** hostName);
	NS_IMETHOD UserNeedsToAuthenticateForFolder(PRBool displayOnly, PRBool *authenticate);
	NS_IMETHOD RememberPassword(const char *password);
	NS_IMETHOD GetRememberedPassword(char ** password);

  virtual nsresult GetDBFolderInfoAndDB(nsIDBFolderInfo **folderInfo, nsIMsgDatabase **db);

 	NS_IMETHOD DeleteMessages(nsISupportsArray *messages, 
                            nsITransactionManager *txnMgr, PRBool deleteStorage);
	NS_IMETHOD CreateMessageFromMsgDBHdr(nsIMsgDBHdr *msgDBHdr, nsIMessage **message);
  NS_IMETHOD GetNewMessages();

  NS_IMETHOD GetPath(nsIFileSpec** aPathName);
  
  NS_IMETHOD GetMsgKeySetStr(char * *aMsgKeySetStr);
  NS_IMETHOD SetMsgKeySetStr(char * aMsgKeySetStr);

protected:
	nsresult ParseFolder(nsFileSpec& path);
	nsresult CreateSubFolders(nsFileSpec &path);
	nsresult AddDirectorySeparator(nsFileSpec &path);
	nsresult GetDatabase();
  
	nsresult CreateDirectoryForFolder(nsFileSpec &path);

	//Creates a subfolder with the name 'name' and adds it to the list of children.
	//Returns the child as well.
	nsresult AddSubfolder(nsAutoString name, nsIMsgFolder **child, char *setStr);

  PRBool isNewsHost(void);
  nsresult LoadNewsrcFileAndCreateNewsgroups(nsFileSpec &newsrcFile);
  PRInt32 RememberLine(char *line);
  nsresult ForgetLine(void);

  PRInt32 HandleLine(char *line, PRUint32 line_size);
  nsresult GetNewsrcFile(char *newshostname, nsFileSpec &path, nsFileSpec &newsrcFile);
#ifdef USE_NEWSRC_MAP_FILE
  nsresult MapHostToNewsrcFile(char *newshostname, nsFileSpec &fatFile, nsFileSpec &newsrcFile);
#endif

  virtual const char *GetIncomingServerType() {return "nntp";}
  
  nsByteArray		m_inputStream;

protected:
	nsNativeFileSpec *mPath;
	PRUint32  mExpungedBytes;
	PRBool		mGettingNews;
	PRBool		mInitialized;
	nsISupportsArray *mMessages;
  char      *mOptionLines;
  char      *mHostname;

private:
  nsMsgKeySet *mSet;
};

#endif // nsMsgNewsFolder_h__
