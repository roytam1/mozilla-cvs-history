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

/*
 * some platforms (like Windows and Mac) use a map file, because of
 * file name length limitations. 
 */
#ifndef XP_UNIX
#define USE_NEWSRC_MAP_FILE

#if defined(XP_PC)
#define FAT_FILE_NAME "fat"
/* 
 * on the PC, the fat file stores absolute paths to the newsrc files
 * on the Mac, the fat file stores relative paths to the newsrc files
 */
#define FAT_STORES_ABSOLUTE_NEWSRC_FILE_PATHS 1
#elif defined(XP_MAC)
#define FAT_FILE_NAME "NewsFAT"
#else
#error dont_know_what_your_fat_file_is
#endif 

#endif /* ! XP_UNIX */

class nsMsgNewsFolder : public nsMsgDBFolder, public nsIMsgNewsFolder
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

	NS_IMETHOD GetUsersName(char** userName);
	NS_IMETHOD GetHostName(char** hostName);
	NS_IMETHOD UserNeedsToAuthenticateForFolder(PRBool displayOnly, PRBool *authenticate);
	NS_IMETHOD RememberPassword(const char *password);
	NS_IMETHOD GetRememberedPassword(char ** password);

  virtual nsresult GetDBFolderInfoAndDB(nsIDBFolderInfo **folderInfo, nsIMsgDatabase **db);

 	NS_IMETHOD DeleteMessages(nsISupportsArray *messages);
	NS_IMETHOD CreateMessageFromMsgDBHdr(nsIMsgDBHdr *msgDBHdr, nsIMessage **message);
  NS_IMETHOD GetNewMessages();

	// nsIMsgNewsFolder
  NS_IMETHOD GetPath(nsNativeFileSpec& aPathName);

protected:
	nsresult ParseFolder(nsFileSpec& path);
	nsresult CreateSubFolders(nsFileSpec &path);
	nsresult AddDirectorySeparator(nsFileSpec &path);
	nsresult GetDatabase();

	/* Finds the directory associated with this folder.  That is if the path is
	c:\Inbox, it will return c:\Inbox.sbd if it succeeds.  If that path doesn't
	currently exist then it will create it
	*/
	nsresult CreateDirectoryForFolder(nsFileSpec &path);

	//Creates a subfolder with the name 'name' and adds it to the list of children.
	//Returns the child as well.
	nsresult AddSubfolder(nsAutoString name, nsIMsgFolder **child);

  PRBool isNewsHost(void);
  nsresult LoadNewsrcFileAndCreateNewsgroups(nsFileSpec &newsrcFile);
  PRInt32 ProcessLine(char *line, PRUint32 line_size);
  PRInt32 RememberLine(char *line);
  static PRInt32 ProcessLine_s(char *line, PRUint32 line_size, void *closure);
  nsresult GetNewsrcFile(char *newshostname, nsFileSpec &path, nsFileSpec &newsrcFile);
#ifdef USE_NEWSRC_MAP_FILE
  nsresult MapHostToNewsrcFile(char *newshostname, nsFileSpec &fatFile, nsFileSpec &newsrcFile);
#endif
  virtual const nsIID& GetIncomingServerType() {return nsINntpIncomingServer::GetIID();}

protected:
	nsNativeFileSpec *mPath;
	PRUint32  mExpungedBytes;
	PRBool		mHaveReadNameFromDB;
	PRBool		mGettingNews;
	PRBool		mInitialized;
	nsISupportsArray *mMessages;
  char      *m_optionLines;
};

#endif // nsMsgNewsFolder_h__
