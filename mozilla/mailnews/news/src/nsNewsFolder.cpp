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

#define NS_IMPL_IDS
#include "nsIPref.h"
#include "prlog.h"

#include "msgCore.h"    // precompiled header...

#include "nsNewsFolder.h"	 
#include "nsMsgFolderFlags.h"
#include "prprf.h"
#include "prsystem.h"
#include "nsISupportsArray.h"
#include "nsIServiceManager.h"
#include "nsIEnumerator.h"
#include "nsINntpService.h"
#include "nsIFolderListener.h"
#include "nsCOMPtr.h"
#include "nsIRDFService.h"
#include "nsIRDFDataSource.h"
#include "nsRDFCID.h"
#include "nsFileStream.h"
#include "nsMsgDBCID.h"
#include "nsMsgNewsCID.h"
#include "nsNewsMessage.h"
#include "nsMsgUtils.h"
#include "nsNewsUtils.h"

#include "nsCOMPtr.h"
#include "nsIMsgMailSession.h"
#include "nsIMsgIncomingServer.h"
#include "nsINntpIncomingServer.h"
#include "nsINewsDatabase.h"
#include "nsMsgBaseCID.h"
#include "nsFileStream.h"

#ifdef DEBUG_seth
#define DEBUG_NEWS 1
#endif

// we need this because of an egcs 1.0 (and possibly gcc) compiler bug
// that doesn't allow you to call ::nsISupports::GetIID() inside of a class
// that multiply inherits from nsISupports
static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);
static NS_DEFINE_CID(kRDFServiceCID, NS_RDFSERVICE_CID);
static NS_DEFINE_CID(kNntpServiceCID,	NS_NNTPSERVICE_CID);
static NS_DEFINE_CID(kCNewsDB, NS_NEWSDB_CID);
static NS_DEFINE_CID(kMsgMailSessionCID, NS_MSGMAILSESSION_CID);
static NS_DEFINE_CID(kPrefServiceCID, NS_PREF_CID);

#define PREF_NEWS_MAX_HEADERS_TO_SHOW "news.max_headers_to_show"
#define PREF_NEWS_ABBREVIATE_PRETTY_NAMES "news.abbreviate_pretty_name"
#define NEWSRC_FILE_BUFFER_SIZE 1024

////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////

nsMsgNewsFolder::nsMsgNewsFolder(void) : nsMsgLineBuffer(nsnull, PR_FALSE),
    mPath(nsnull), mExpungedBytes(0), mGettingNews(PR_FALSE),
    mInitialized(PR_FALSE), mOptionLines(nsnull)
{
  /* we're parsing the newsrc file, and the line breaks are platform specific.
   * if MSG_LINEBREAK != CRLF, then we aren't looking for CRLF 
   */
  if (PL_strcmp(MSG_LINEBREAK, CRLF)) {
    SetLookingForCRLF(PR_FALSE);
  }
//  NS_INIT_REFCNT(); done by superclass
}

nsMsgNewsFolder::~nsMsgNewsFolder(void)
{
	if (mPath) {
		delete mPath;
    mPath = nsnull;
  }

  PR_FREEIF(mOptionLines);
  mOptionLines = nsnull;
}

NS_IMPL_ADDREF_INHERITED(nsMsgNewsFolder, nsMsgDBFolder)
NS_IMPL_RELEASE_INHERITED(nsMsgNewsFolder, nsMsgDBFolder)

NS_IMETHODIMP nsMsgNewsFolder::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
	if (!aInstancePtr) return NS_ERROR_NULL_POINTER;
	*aInstancePtr = nsnull;
	if (aIID.Equals(nsIMsgNewsFolder::GetIID()))
	{
		*aInstancePtr = NS_STATIC_CAST(nsIMsgNewsFolder*, this);
	}              

	if(*aInstancePtr)
	{
		AddRef();
		return NS_OK;
	}

	return nsMsgDBFolder::QueryInterface(aIID, aInstancePtr);
}

////////////////////////////////////////////////////////////////////////////////

nsresult
nsMsgNewsFolder::CreateSubFolders(nsFileSpec &path)
{
  nsresult rv = NS_OK;

  char *hostname;
  rv = GetHostname(&hostname);
  if (NS_FAILED(rv)) return rv;

  PRBool isNewsServer = PR_FALSE;
  rv = GetIsServer(&isNewsServer);
  if (NS_FAILED(rv)) return rv;

  if (isNewsServer) {  
#ifdef DEBUG_NEWS
    printf("CreateSubFolders:  %s = %s\n", mURI, (const char *)path);
#endif

    //Are we assured this is the server for this folder?
    nsCOMPtr<nsIMsgIncomingServer> server;
    rv = GetServer(getter_AddRefs(server));
    if (NS_FAILED(rv)) return rv;
  
    nsCOMPtr<nsINntpIncomingServer> nntpServer;
    rv = server->QueryInterface(nsINntpIncomingServer::GetIID(),
                                getter_AddRefs(nntpServer));
    if (NS_FAILED(rv)) return rv;
    
    rv = nntpServer->GetNewsrcFilePath(getter_AddRefs(mNewsrcFilePath));
    if (NS_FAILED(rv)) return rv;
      
    rv = LoadNewsrcFileAndCreateNewsgroups();
  }
  else {
#ifdef DEBUG_NEWS
    printf("%s is not a host, so it has no newsgroups.  (what about categories??)\n", mURI);
#endif
    rv = NS_OK;
  }
  
  PR_FREEIF(hostname);

  return rv;
}

nsresult
nsMsgNewsFolder::AddSubfolder(nsAutoString name, nsIMsgFolder **child, char *setStr)
{
	if (!child)
		return NS_ERROR_NULL_POINTER;

  if (!setStr)
    return NS_ERROR_NULL_POINTER;
  
#ifdef DEBUG_NEWS
  nsCString nameStr(name);
  printf("AddSubfolder(%s,??,%s)\n",nameStr.GetBuffer(),setStr);
#endif
  
	nsresult rv = NS_OK;
	NS_WITH_SERVICE(nsIRDFService, rdf, kRDFServiceCID, &rv); 

	if(NS_FAILED(rv))
		return rv;

	nsCString uri(mURI);
	uri.Append('/');
	uri.Append(name);

	nsCOMPtr<nsIRDFResource> res;
	rv = rdf->GetResource(uri.GetBuffer(), getter_AddRefs(res));
	if (NS_FAILED(rv))
		return rv;
  
  nsCOMPtr<nsIMsgFolder> folder(do_QueryInterface(res, &rv));
	if (NS_FAILED(rv))
		return rv;
  
	folder->SetParent(this);

	rv = folder->SetFlag(MSG_FOLDER_FLAG_NEWSGROUP);
  if (NS_FAILED(rv))
    return rv;        
  
  nsCOMPtr<nsIMsgNewsFolder> newsFolder(do_QueryInterface(res, &rv));
  if (NS_FAILED(rv))
    return rv;        
  
  rv = newsFolder->SetUnreadSetStr(setStr);
  if (NS_FAILED(rv))
    return rv;
  
#ifdef DEBUG_NEWS
  char *testStr = nsnull;
  rv = newsFolder->GetUnreadSetStr(&testStr);
  if (NS_FAILED(rv)) return NS_ERROR_FAILURE;
  printf("set str = %s\n",testStr);
  if (testStr) {
    delete [] testStr;
    testStr = nsnull;
  }
#endif
 
	//convert to an nsISupports before appending
	nsCOMPtr<nsISupports> folderSupports(do_QueryInterface(folder));
	if(folderSupports)
		mSubFolders->AppendElement(folderSupports);
	*child = folder;
	folder->SetParent(this);
	NS_ADDREF(*child);

	return rv;
}


nsresult nsMsgNewsFolder::ParseFolder(nsFileSpec& path)
{
  NS_ASSERTION(0,"ParseFolder not implemented");
 	return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsMsgNewsFolder::Enumerate(nsIEnumerator **result)
{
#if 0
  nsresult rv = NS_OK;
  // for now, news folders contain both messages and folders
  // server is a folder, and it contains folders
  // newsgroup is a folder, and it contains messages
  //
  // eventually the top level server will not be a folder
  // and news folders will only contain messages
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

nsresult
nsMsgNewsFolder::AddDirectorySeparator(nsFileSpec &path)
{
	nsresult rv = NS_OK;
	if (PL_strcmp(mURI, kNewsRootURI) == 0) {
      // don't concat the full separator with .sbd
    }
    else {
      nsAutoString sep;
#if 0
      rv = nsGetNewsFolderSeparator(sep);
#else
      rv = NS_OK;
#endif
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

NS_IMETHODIMP
nsMsgNewsFolder::GetSubFolders(nsIEnumerator* *result)
{
  if (!mInitialized) {
	nsresult rv;
	nsCOMPtr<nsIFileSpec> pathSpec;
	rv = GetPath(getter_AddRefs(pathSpec));
	if (NS_FAILED(rv)) return rv;

	nsFileSpec path;
	rv = pathSpec->GetFileSpec(&path);
	if (NS_FAILED(rv)) return rv;
	
    rv = CreateSubFolders(path);

	// force ourselves to get initialized from cache
    UpdateSummaryTotals(PR_FALSE); 
    if (NS_FAILED(rv)) return rv;

    mInitialized = PR_TRUE;      // XXX do this on failure too?
  }
  return mSubFolders->Enumerate(result);
}

NS_IMETHODIMP
nsMsgNewsFolder::AddUnique(nsISupports* element)
{
  NS_ASSERTION(0,"AddUnique not implemented");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsMsgNewsFolder::ReplaceElement(nsISupports* element, nsISupports* newElement)
{
  NS_ASSERTION(0,"ReplaceElement not implemented");
  return NS_ERROR_NOT_IMPLEMENTED;
}

//Makes sure the database is open and exists.  If the database is valid then
//returns NS_OK.  Otherwise returns a failure error value.
nsresult nsMsgNewsFolder::GetDatabase()
{
		nsresult rv;
	if (!mDatabase)
	{
		nsCOMPtr<nsIFileSpec> pathSpec;
		rv = GetPath(getter_AddRefs(pathSpec));
		if (NS_FAILED(rv)) return rv;

		nsresult folderOpen = NS_OK;
		nsCOMPtr <nsIMsgDatabase> newsDBFactory;

		rv = nsComponentManager::CreateInstance(kCNewsDB, nsnull, nsIMsgDatabase::GetIID(), getter_AddRefs(newsDBFactory));
		if (NS_SUCCEEDED(rv) && newsDBFactory)
		{
			folderOpen = newsDBFactory->Open(pathSpec, PR_TRUE, PR_FALSE, getter_AddRefs(mDatabase));
#ifdef DEBUG_NEWS
        if (NS_SUCCEEDED(folderOpen)) {
          printf ("newsDBFactory->Open() succeeded\n");
		}
        else {
          printf ("newsDBFactory->Open() failed\n");
          return rv;
		}
#endif
		}

		if (mDatabase) {
			if(mAddListener)
				rv = mDatabase->AddListener(this);
		    nsCOMPtr<nsINewsDatabase> db(do_QueryInterface(mDatabase, &rv));
		    if (NS_FAILED(rv))
				return rv;        

		    rv = db->SetUnreadSet(m_unreadSet.GetBuffer());
		}
        if (NS_FAILED(rv)) return rv;
       
        rv = UpdateSummaryTotals(PR_TRUE);
        if (NS_FAILED(rv)) return rv;
	}
	return NS_OK;
}


NS_IMETHODIMP
nsMsgNewsFolder::UpdateFolder(nsIMsgWindow *aWindow)
{
  GetDatabase();	// want this cached...
  return GetNewMessages(aWindow);
}

NS_IMETHODIMP
nsMsgNewsFolder::GetCanSubscribe(PRBool *aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);
  *aResult = PR_FALSE;

  PRBool isNewsServer = PR_FALSE;
  nsresult rv = GetIsServer(&isNewsServer);
  if (NS_FAILED(rv)) return rv;
 
  // you can only subscribe to news servers, not news groups
  *aResult = isNewsServer;
  return NS_OK;
}

NS_IMETHODIMP
nsMsgNewsFolder::GetCanFileMessages(PRBool *aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);
  // you can't file messages into a news server or news group
  *aResult = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP
nsMsgNewsFolder::GetCanCreateSubfolders(PRBool *aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);
  *aResult = PR_FALSE;
  // you can't create subfolders on a news server or a news group
  return NS_OK;
}

NS_IMETHODIMP
nsMsgNewsFolder::GetCanRename(PRBool *aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);
  *aResult = PR_FALSE;
  // you can't rename a news server or a news group
  return NS_OK;
}


NS_IMETHODIMP
nsMsgNewsFolder::GetMessages(nsISimpleEnumerator* *result)
{
#ifdef DEBUG_NEWS
  printf("nsMsgNewsFolder::GetMessages(%s)\n",mURI);
#endif

  nsresult rv = NS_OK;

  rv = GetDatabase();
    *result = nsnull;
    
  if(NS_SUCCEEDED(rv)) {
		nsCOMPtr<nsISimpleEnumerator> msgHdrEnumerator;
		nsMessageFromMsgHdrEnumerator *messageEnumerator = nsnull;
		rv = mDatabase->EnumerateMessages(getter_AddRefs(msgHdrEnumerator));
		if(NS_SUCCEEDED(rv))
		  rv = NS_NewMessageFromMsgHdrEnumerator(msgHdrEnumerator, this, &messageEnumerator);
		*result = messageEnumerator;
  }

  return rv;
}

NS_IMETHODIMP nsMsgNewsFolder::GetFolderURL(char **url)
{
  const char *urlScheme = "news:";

  if(!url)
    return NS_ERROR_NULL_POINTER;

  nsCOMPtr<nsIFileSpec> pathSpec;
  nsresult rv = GetPath(getter_AddRefs(pathSpec));
  if (NS_FAILED(rv)) return rv;

  nsFileSpec path;
  rv = pathSpec->GetFileSpec(&path);
  if (NS_FAILED(rv)) return rv;
#if defined(XP_MAC)
  nsCAutoString tmpPath((nsFilePath)path); //ducarroz: please don't cast a nsFilePath to char* on Mac
  *url = PR_smprintf("%s%s", urlScheme, tmpPath.GetBuffer());
#else
  const char *pathName = path;
  *url = PR_smprintf("%s%s", urlScheme, pathName);
#endif
  return NS_OK;

}


NS_IMETHODIMP nsMsgNewsFolder::CreateSubfolder(const char *newsgroupname)
{
	nsresult rv = NS_OK;
  
	if (!newsgroupname) return NS_ERROR_NULL_POINTER;
	if (PL_strlen(newsgroupname) == 0) return NS_ERROR_FAILURE;

    nsFileSpec path;
	nsCOMPtr<nsIFileSpec> pathSpec;
	rv = GetPath(getter_AddRefs(pathSpec));
	if (NS_FAILED(rv)) return rv;

	rv = pathSpec->GetFileSpec(&path);

    nsCOMPtr<nsIMsgFolder> child;
   
	// Create an empty database for this mail folder, set its name from the user  
	nsCOMPtr<nsIMsgDatabase> newsDBFactory;
	nsCOMPtr <nsIMsgDatabase> newsDB;

	//Now we have a valid directory or we have returned.
	//Make sure the new folder name is valid
	// do we need to hash newsgroup name if it is too big?
	path += newsgroupname;
	
	rv = AddNewsgroupToNewsrcFile(newsgroupname);
	if (NS_FAILED(rv)) return rv;

	rv = nsComponentManager::CreateInstance(kCNewsDB, nsnull, nsIMsgDatabase::GetIID(), getter_AddRefs(newsDBFactory));
	if (NS_SUCCEEDED(rv) && newsDBFactory) {
		nsCOMPtr <nsIFileSpec> dbFileSpec;
		NS_NewFileSpecWithSpec(path, getter_AddRefs(dbFileSpec));
		rv = newsDBFactory->Open(dbFileSpec, PR_TRUE, PR_FALSE, getter_AddRefs(newsDB));
		if (NS_SUCCEEDED(rv) && newsDB) {
			//Now let's create the actual new folder
			char *setStr = PR_smprintf("");
			rv = AddSubfolder(newsgroupname, getter_AddRefs(child), setStr);
			PR_FREEIF(setStr);
            newsDB->SetSummaryValid(PR_TRUE);
            newsDB->Close(PR_TRUE);
        }
        else
        {
            rv = NS_MSG_CANT_CREATE_FOLDER;
        }
	}
	if(NS_SUCCEEDED(rv) && child)
	{
		nsCOMPtr<nsISupports> childSupports(do_QueryInterface(child));
		nsCOMPtr<nsISupports> folderSupports;
		rv = QueryInterface(nsCOMTypeInfo<nsISupports>::GetIID(), getter_AddRefs(folderSupports));
		if(childSupports && NS_SUCCEEDED(rv))
		{

			NotifyItemAdded(folderSupports, childSupports, "folderView");
		}
	}
	return rv;
}

NS_IMETHODIMP nsMsgNewsFolder::Delete()
{
  NS_ASSERTION(0,"Delete not implemented");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsMsgNewsFolder::Rename(const char *newName)
{
  NS_ASSERTION(0,"Rename not implemented");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsMsgNewsFolder::Adopt(nsIMsgFolder *srcFolder, PRUint32 *outPos)
{
  NS_ASSERTION(0,"Adopt not implemented");  
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP 
nsMsgNewsFolder::GetChildNamed(const char *name, nsISupports ** aChild)
{
  NS_ASSERTION(aChild, "NULL child");

  // will return nsnull if we can't find it
  *aChild = nsnull;

  nsCOMPtr <nsIMsgFolder> folder;

  PRUint32 cnt;
  nsresult rv = mSubFolders->Count(&cnt);
  if (NS_FAILED(rv)) return rv;
  PRUint32 count = cnt;

  for (PRUint32 i = 0; i < count; i++) {
    nsCOMPtr <nsISupports> supports;
    supports = mSubFolders->ElementAt(i);

    if(NS_SUCCEEDED(supports->QueryInterface(kISupportsIID, getter_AddRefs(folder)))) {
      PRUnichar *folderName;
      
      folder->GetName(&folderName);
      
      // case-insensitive compare is probably LCD across OS filesystems
      if (folderName && nsCRT::strcasecmp(folderName, name)!=0) {
        *aChild = folder;
        PR_FREEIF(folderName);
        return NS_OK;
      }
      PR_FREEIF(folderName);
    }
  }
  return NS_OK;
}

NS_IMETHODIMP nsMsgNewsFolder::GetAbbreviatedName(PRUnichar * *aAbbreviatedName)
{
  nsresult rv = NS_OK;

  if (!aAbbreviatedName)
    return NS_ERROR_NULL_POINTER;

  rv = nsMsgFolder::GetPrettyName(aAbbreviatedName);
  if(NS_FAILED(rv)) return rv;

  // only do this for newsgroup names, not for newsgroup hosts.
  PRBool isNewsServer = PR_FALSE;
  rv = GetIsServer(&isNewsServer);
  if (NS_FAILED(rv)) return rv;
  
  if (!isNewsServer) {  
	NS_WITH_SERVICE(nsIPref, prefs, kPrefServiceCID, &rv);
    if (NS_FAILED(rv)) return rv;
    
    PRInt32 numFullWords;
    rv = prefs->GetIntPref(PREF_NEWS_ABBREVIATE_PRETTY_NAMES, &numFullWords);
    if (NS_FAILED(rv))
	  numFullWords = 1;
    
    if (numFullWords != 0) { 
      rv = AbbreviatePrettyName(aAbbreviatedName, numFullWords);
	}
  }

  return rv;
}


// original code from Oleg Rekutin
// rekusha@asan.com
// Public domain, created by Oleg Rekutin
//
// takes a newsgroup name, number of words from the end to leave unabberviated
// the newsgroup name, will get reset to the following format:
// x.x.x, where x is the first letter of each word and with the
// exception of last 'fullwords' words, which are left intact.
// If a word has a dash in it, it is abbreviated as a-b, where
// 'a' is the first letter of the part of the word before the
// dash and 'b' is the first letter of the part of the word after
// the dash
nsresult nsMsgNewsFolder::AbbreviatePrettyName(PRUnichar ** prettyName, PRInt32 fullwords)
{
  if (!prettyName)
		return NS_ERROR_NULL_POINTER;
  
  nsString name(*prettyName);
  PRInt32 totalwords = 0; // total no. of words

  // get the total no. of words
  PRInt32 pos = 0;
  while(1)
  {
	  pos = name.FindChar('.', PR_FALSE, pos);
	  if(pos == -1)
	  {
		  totalwords++;
		  break;
	  }
	  else
	  {
		  totalwords++;
		  pos++;
	  }
  }

  // get the no. of words to abbreviate
  PRInt32 abbrevnum = totalwords - fullwords;
  if (abbrevnum < 1)
    return NS_OK; // nothing to abbreviate
  
  // build the ellipsis
  nsString out;
  
  out += name[0];
  
  PRInt32    length = name.Length();
  PRInt32    newword = 0;     // == 2 if done with all abbreviated words
  
  fullwords = 0;
  for (PRInt32 i = 1; i < length; i++) {
    if (newword < 2) {
      switch (name[i]) {
      case '.':
        fullwords++;
        // check if done with all abbreviated words...
        if (fullwords == abbrevnum)
          newword = 2;
        else
          newword = 1;
        break;
      case '-':
        newword = 1;
        break;
      default:
        if (newword)
          newword = 0;
        else
          continue;
      }
    }
    out += name[i];
  }

  if (!prettyName)
		return NS_ERROR_NULL_POINTER;
  // we are going to set *prettyName to something else, so free what was there
  
  PR_FREEIF(*prettyName);
  *prettyName = out.ToNewUnicode();
  
  return (*prettyName) ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}


nsresult  nsMsgNewsFolder::GetDBFolderInfoAndDB(nsIDBFolderInfo **folderInfo, nsIMsgDatabase **db)
{
  nsresult openErr=NS_ERROR_UNEXPECTED;
  if(!db || !folderInfo)
		return NS_ERROR_NULL_POINTER;	//ducarroz: should we use NS_ERROR_INVALID_ARG?
		
	if (!mPath)
		return NS_ERROR_NULL_POINTER;

	nsCOMPtr <nsIMsgDatabase> newsDBFactory;
	nsIMsgDatabase *newsDB;

	nsresult rv = nsComponentManager::CreateInstance(kCNewsDB, nsnull, nsIMsgDatabase::GetIID(), getter_AddRefs(newsDBFactory));
	if (NS_SUCCEEDED(rv) && newsDBFactory) {
		nsCOMPtr <nsIFileSpec> dbFileSpec;
		NS_NewFileSpecWithSpec(*mPath, getter_AddRefs(dbFileSpec));
		openErr = newsDBFactory->Open(dbFileSpec, PR_FALSE, PR_FALSE, (nsIMsgDatabase **) &newsDB);
	}
  else {
    return rv;
  }

  *db = newsDB;
  if (NS_SUCCEEDED(openErr)&& *db)
    openErr = (*db)->GetDBFolderInfo(folderInfo);
  return openErr;
}

NS_IMETHODIMP nsMsgNewsFolder::UpdateSummaryTotals(PRBool force)
{
#ifdef DEBUG_NEWS
	printf("nsMsgNewsFolder::UpdateSummaryTotals(%s)\n",mURI);
#endif

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
		NotifyIntPropertyChanged("TotalMessages", oldTotalMessages, mNumTotalMessages);
	}

	if(oldUnreadMessages != mNumUnreadMessages)
	{
		NotifyIntPropertyChanged("TotalUnreadMessages", oldUnreadMessages, mNumUnreadMessages);
	}

	return NS_OK;
}

NS_IMETHODIMP nsMsgNewsFolder::GetExpungedBytesCount(PRUint32 *count)
{
  if(!count)
    return NS_ERROR_NULL_POINTER;

  *count = mExpungedBytes;

  return NS_OK;
}

NS_IMETHODIMP nsMsgNewsFolder::GetDeletable(PRBool *deletable)
{
#if 0
  if(!deletable)
    return NS_ERROR_NULL_POINTER;

  // These are specified in the "Mail/News Windows" UI spec

  if (mFlags & MSG_FOLDER_FLAG_TRASH)
  {
    PRBool moveToTrash;
    GetDeleteIsMoveToTrash(&moveToTrash);
    if(moveToTrash)
      *deletable = PR_TRUE;  // allow delete of trash if we don't use trash
  }
  else if (mDepth == 1)
    *deletable = PR_FALSE;
  else if (mFlags & MSG_FOLDER_FLAG_INBOX || 
    mFlags & MSG_FOLDER_FLAG_DRAFTS || 
    mFlags & MSG_FOLDER_FLAG_TRASH ||
    mFlags & MSG_FOLDER_FLAG_TEMPLATES)
    *deletable = PR_FALSE;
  else *deletable =  PR_TRUE;

  return NS_OK;
#else
  NS_ASSERTION(0,"GetDeletable() not implemented");
  return NS_ERROR_NOT_IMPLEMENTED;
#endif
}
 
NS_IMETHODIMP nsMsgNewsFolder::GetCanCreateChildren(PRBool *canCreateChildren)
{  
  if(!canCreateChildren)
    return NS_ERROR_NULL_POINTER;

  *canCreateChildren = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP nsMsgNewsFolder::GetCanBeRenamed(PRBool *canBeRenamed)
{
#if 0
  if(!canBeRenamed)
    return NS_ERROR_NULL_POINTER;

    // The root mail folder can't be renamed
  if (mDepth < 2)
    *canBeRenamed = PR_FALSE;

  // Here's a weird case necessitated because we don't have a separate
  // preference for any folder name except the FCC folder (Sent). Others
  // are known by name, and as such, can't be renamed. I guess.
  else if (mFlags & MSG_FOLDER_FLAG_TRASH ||
      mFlags & MSG_FOLDER_FLAG_DRAFTS ||
      mFlags & MSG_FOLDER_FLAG_QUEUE ||
      mFlags & MSG_FOLDER_FLAG_INBOX ||
      mFlags & MSG_FOLDER_FLAG_TEMPLATES)
      *canBeRenamed = PR_FALSE;
  else 
    *canBeRenamed = PR_TRUE;

  return NS_OK;
#else
  NS_ASSERTION(0,"GetCanBeRenamed() not implemented");
  return NS_ERROR_NOT_IMPLEMENTED;
#endif
}

NS_IMETHODIMP nsMsgNewsFolder::GetRequiresCleanup(PRBool *requiresCleanup)
{
  NS_ASSERTION(0,"GetRequiresCleanup not implemented");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsMsgNewsFolder::GetSizeOnDisk(PRUint32 *size)
{
  NS_ASSERTION(0, "GetSizeOnDisk not implemented");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsMsgNewsFolder::UserNeedsToAuthenticateForFolder(PRBool displayOnly, PRBool *authenticate)
{
  return NS_OK;
}

NS_IMETHODIMP nsMsgNewsFolder::RememberPassword(const char *password)
{
#ifdef HAVE_DB
  NewsDB *newsDb = nsnull;
  NewsDB::Open(m_pathName, PR_TRUE, &newsDb);
  if (newsDb)
  {
    newsDb->SetCachedPassword(password);
    newsDb->Close();
  }
#endif
  return NS_OK;
}

NS_IMETHODIMP nsMsgNewsFolder::GetRememberedPassword(char ** password)
{
  return NS_OK;
}

NS_IMETHODIMP nsMsgNewsFolder::GetPath(nsIFileSpec** aPathName)
{
  nsresult rv;
  if (! mPath) {
    mPath = new nsNativeFileSpec("");
    if (! mPath)
    	return NS_ERROR_OUT_OF_MEMORY;

    rv = nsNewsURI2Path(kNewsRootURI, mURI, *mPath);
    if (NS_FAILED(rv)) return rv;
  }
  rv = NS_NewFileSpecWithSpec(*mPath, aPathName);
  return rv;
}

/* this is news, so remember that DeleteMessage is really CANCEL */
NS_IMETHODIMP nsMsgNewsFolder::DeleteMessages(nsISupportsArray *messages,
                                              nsIMsgWindow *msgWindow, PRBool deleteStorage)
{
  nsresult rv = NS_OK;
  
  if (!messages) {
    // nothing to CANCEL
    return NS_ERROR_NULL_POINTER;
  }

  NS_WITH_SERVICE(nsINntpService, nntpService, kNntpServiceCID, &rv);
  
  if (NS_SUCCEEDED(rv) && nntpService) {
    char *hostname;
    rv = GetHostname(&hostname);
    if (NS_FAILED(rv)) return rv;
    PRUnichar *newsgroupname;
    rv = GetName(&newsgroupname);
	nsCString asciiName(newsgroupname);
    if (NS_FAILED(rv)) {
      PR_FREEIF(hostname);
      return rv;
    }
    
    rv = nntpService->CancelMessages(hostname, asciiName.GetBuffer(), messages, nsnull, nsnull, nsnull);
    
    PR_FREEIF(hostname);
    PR_FREEIF(newsgroupname);
  }
  
  return rv;
}

NS_IMETHODIMP nsMsgNewsFolder::GetNewMessages(nsIMsgWindow *aWindow)
{
  nsresult rv = NS_OK;

#ifdef DEBUG_NEWS
  printf("GetNewMessages (for news)  uri = %s\n",mURI);
#endif

  PRBool isNewsServer = PR_FALSE;
  rv = GetIsServer(&isNewsServer);
  if (NS_FAILED(rv)) return rv;
  
  if (isNewsServer) {
    // get new messages only works on a newsgroup, not a news server
		return NS_OK;
  }

#ifdef DEBUG_NEWS
  char *setStr = nsnull;
  // caller needs to use delete [] to free
  rv = GetUnreadSetStr(&setStr);
  if (NS_FAILED(rv)) return rv;
  if (setStr) {
    printf("GetNewMessage with setStr = %s\n", setStr);
  }
  if (setStr) {
    delete [] setStr;
    setStr = nsnull;
  }
#endif
  
  NS_WITH_SERVICE(nsINntpService, nntpService, kNntpServiceCID, &rv);
  if (NS_FAILED(rv)) return rv;
  
  //Are we assured this is the server for this folder?
  nsCOMPtr<nsIMsgIncomingServer> server;
  rv = GetServer(getter_AddRefs(server));
  if (NS_FAILED(rv)) return rv;
  
  nsCOMPtr<nsINntpIncomingServer> nntpServer;
  rv = server->QueryInterface(nsINntpIncomingServer::GetIID(),
                              getter_AddRefs(nntpServer));
  if (NS_FAILED(rv)) return rv;
  
#ifdef DEBUG_NEWS
  printf("Getting new news articles....\n");
#endif
  rv = nntpService->GetNewNews(nntpServer, mURI, this, nsnull);
  return rv;
}

NS_IMETHODIMP nsMsgNewsFolder::CreateMessageFromMsgDBHdr(nsIMsgDBHdr *msgDBHdr, nsIMessage **message)
{
  nsresult rv; 
  NS_WITH_SERVICE(nsIRDFService, rdfService, kRDFServiceCID, &rv); 
  if (NS_FAILED(rv)) return rv;

	char* msgURI = nsnull;
	nsFileSpec path;
	nsMsgKey key;
	nsCOMPtr <nsIRDFResource> res;

	rv = msgDBHdr->GetMessageKey(&key);
  if (NS_FAILED(rv)) return rv;
  
  rv = nsBuildNewsMessageURI(mURI, key, &msgURI);
  if (NS_FAILED(rv)) return rv;
  
  rv = rdfService->GetResource(msgURI, getter_AddRefs(res));
  
  PR_FREEIF(msgURI);
  msgURI = nsnull;
  
  if (NS_FAILED(rv)) return rv;
  
  nsCOMPtr<nsIDBMessage> messageResource = do_QueryInterface(res);
  if(messageResource) {
    messageResource->SetMsgDBHdr(msgDBHdr);
    *message = messageResource;
    NS_IF_ADDREF(*message);
  }
  
	return rv;
}

nsresult
nsMsgNewsFolder::AddNewsgroupToNewsrcFile(const char *newsgroupname)
{
	nsresult rv;
	if (!mNewsrcFilePath) return NS_ERROR_FAILURE; 
	nsFileSpec newsrcFile;
	rv = mNewsrcFilePath->GetFileSpec(&newsrcFile);
	if (NS_FAILED(rv)) return rv;

	nsOutputFileStream newsrcStream(newsrcFile, (PR_WRONLY | PR_CREATE_FILE | PR_APPEND));
	if (!newsrcStream.is_open()) {
		return NS_ERROR_FAILURE;
	}
	newsrcStream << newsgroupname;
	newsrcStream << ":";
	newsrcStream << nsEndl;

	newsrcStream.close();
	return NS_OK;
}

nsresult 
nsMsgNewsFolder::LoadNewsrcFileAndCreateNewsgroups()
{
  if (!mNewsrcFilePath) return NS_ERROR_FAILURE;

  nsInputFileStream newsrcStream(mNewsrcFilePath); 
  nsresult rv = NS_OK;
  PRInt32 numread = 0;

  if (NS_FAILED(m_inputStream.GrowBuffer(NEWSRC_FILE_BUFFER_SIZE))) {
#ifdef DEBUG_NEWS
    printf("GrowBuffer failed\n");
#endif
    return NS_ERROR_FAILURE;
  }
	
  while (1) {
    numread = newsrcStream.read(m_inputStream.GetBuffer(), NEWSRC_FILE_BUFFER_SIZE);
#ifdef DEBUG_NEWS
    printf("numread == %d\n", numread);
#endif
    if (numread == 0) {
      break;
    }
    else {
      rv = BufferInput(m_inputStream.GetBuffer(), numread);
      if (NS_FAILED(rv)) {
#ifdef DEBUG_NEWS
        printf("bufferInput did not return NS_OK\n");
#endif
        break;
      }
    }
  }

  newsrcStream.close();
  
  return rv;
}


PRInt32
nsMsgNewsFolder::HandleLine(char* line, PRUint32 line_size)
{
	/* guard against blank line lossage */
	if (line[0] == '#' || line[0] == CR || line[0] == LF) return 0;

	line[line_size] = 0;

	if ((line[0] == 'o' || line[0] == 'O') &&
		!PL_strncasecmp (line, "options", 7)) {
		return RememberLine(line);
	}

	char *s = nsnull;
  char *setStr = nsnull;
	char *end = line + line_size;

	for (s = line; s < end; s++)
		if (*s == ':' || *s == '!')
			break;
	
	if (*s == 0) {
		/* What is this?? Well, don't just throw it away... */
		return RememberLine(line);
	}
    
	PRBool subscribed = (*s == ':');
  setStr = s+1;
	*s = '\0';
  
	if (PL_strlen(line) == 0) {
		return 0;
	}
 
  // previous versions of Communicator poluted the
  // newsrc files with articles
  // (this would happen when you clicked on a link like
  // news://news.mozilla.org/3746EF3F.6080309@netscape.com)
  //
  // legal newsgroup names can't contain @ or %
  // 
  // News group names are structured into parts separated by dots, 
  // for example "netscape.public.mozilla.mail-news". 
  // Each part may be up to 14 characters long, and should consist 
  // only of letters, digits, "+" and "-", with at least one letter
  //
  // @ indicates an article and %40 is @ escaped.
  // previous versions of Communicator also dumped
  // the escaped version into the newsrc file
  //
  // So lines like this in a newsrc file should be ignored:
  // 3746EF3F.6080309@netscape.com:
  // 3746EF3F.6080309%40netscape.com:
  if (PL_strstr(line,"@") || PL_strstr(line,"%40")) {
#ifdef DEBUG_NEWS
    printf("skipping %s.  it contains @ or %%40\n",line);
#endif
  	subscribed = PR_FALSE;
  }

  if (subscribed) {
#ifdef DEBUG_NEWS
    printf("subscribed: %s\n", line);
#endif

    // we're subscribed, so add it
    nsCOMPtr <nsIMsgFolder> child;
    nsAutoString currentFolderNameStr(line);
    
    nsresult rv = AddSubfolder(currentFolderNameStr,getter_AddRefs(child), setStr);
    
    if (NS_FAILED(rv)) return -1;
  }
  else {
#ifdef DEBUG_NEWS
    printf("NOT subscribed: %s\n", line);
#endif
  }

#ifdef HAVE_PORT
	MSG_FolderInfoNews* info;

	if (subscribed && IsCategoryContainer(line))
	{
		info = new MSG_FolderInfoCategoryContainer(line, set, subscribed,
												   this,
												   m_hostinfo->GetDepth() + 1);
		msg_GroupRecord* group = FindOrCreateGroup(line);
		// Go add all of our categories to the newsrc.
		AssureAllDescendentsLoaded(group);
		msg_GroupRecord* end = group->GetSiblingOrAncestorSibling();
		msg_GroupRecord* child;
		for (child = group->GetNextAlphabetic() ;
			 child != end ;
			 child = child->GetNextAlphabetic()) {
			NS_ASSERTION(child,"null ptr");
			if (!child) break;
			char* fullname = child->GetFullName();
			if (!fullname) break;
			MSG_FolderInfoNews* info = FindGroup(fullname);
			if (!info) {	// autosubscribe, if we haven't seen this one.
				char* groupLine = PR_smprintf("%s:", fullname);
				if (groupLine) {
					HandleLine(groupLine, PL_strlen(groupLine));
					PR_FREEIF(groupLine);
          groupLine = nsnull;
				}
			}
			delete [] fullname;
		}
	}
	else
		info = new MSG_FolderInfoNews(line, set, subscribed, this,
							   m_hostinfo->GetDepth() + 1);

	if (!info) return -1; // NS_ERROR_OUT_OF_MEMORY;

	// for now, you can't subscribe to category by itself.
	if (! info->IsCategory())
	{
		XPPtrArray* infolist = (XPPtrArray*) m_hostinfo->GetSubFolders();
		infolist->Add(info);
	}

	m_groups->Add(info);

	// prime the folder info from the folder cache while it's still around.
	// Except this might disable the update of new counts - check it out...
	m_master->InitFolderFromCache (info);
#endif /* HAVE_PORT */
  
  return 0;
}

PRInt32
nsMsgNewsFolder::RememberLine(char* line)
{
	char* new_data;
	if (mOptionLines) {
		new_data =
			(char *) PR_Realloc(mOptionLines,
								PL_strlen(mOptionLines)
								+ PL_strlen(line) + 4);
	} else {
		new_data = (char *) PR_Malloc(PL_strlen(line) + 3);
	}
	if (!new_data) return -1; // NS_ERROR_OUT_OF_MEMORY;
	PL_strcpy(new_data, line);
	PL_strcat(new_data, MSG_LINEBREAK);

	mOptionLines = new_data;

	return 0;

}

nsresult nsMsgNewsFolder::ForgetLine()
{
  PR_FREEIF(mOptionLines);
  mOptionLines = nsnull;
  return NS_OK;
}

// caller needs to use delete [] to free
NS_IMETHODIMP nsMsgNewsFolder::GetUnreadSetStr(char * *aUnreadSetStr)
{
  nsresult rv;
  
  if (!aUnreadSetStr) return NS_ERROR_NULL_POINTER;

  rv = GetDatabase();
  if (NS_FAILED(rv)) return rv;

  NS_ASSERTION(mDatabase, "no database!");
  if (!mDatabase) return NS_ERROR_NULL_POINTER;

  nsMsgKeySet * set = nsnull;
  
  nsCOMPtr<nsINewsDatabase> db(do_QueryInterface(mDatabase, &rv));
  if (NS_FAILED(rv))
	return rv;        

  rv = db->GetUnreadSet(&set);
  if (NS_FAILED(rv)) return rv;
      
  *aUnreadSetStr = set->Output();

  if (!*aUnreadSetStr) return NS_ERROR_OUT_OF_MEMORY;

  return NS_OK;
}

NS_IMETHODIMP nsMsgNewsFolder::SetUnreadSetStr(const char * aUnreadSetStr)
{
  if (!aUnreadSetStr) return NS_ERROR_NULL_POINTER;

  m_unreadSet = aUnreadSetStr;

  return NS_OK;
}
