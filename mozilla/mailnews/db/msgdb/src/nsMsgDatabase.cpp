/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
 * Copyright (C) 1999 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

// this file implements the nsMsgDatabase interface using the MDB Interface.

#include "msgCore.h"
#include "nsMsgDatabase.h"
#include "nsDBFolderInfo.h"
#include "nsMsgKeySet.h"
#include "nsIEnumerator.h"
#include "nsMsgThread.h"
#include "nsFileStream.h"
#include "nsString.h"
#include "nsIMsgHeaderParser.h"
#include "nsMsgBaseCID.h"
#include "nsMorkCID.h"
#include "nsIMdbFactoryFactory.h"
#include "nsIMimeConverter.h"

#include "nsILocale.h"
#include "nsLocaleCID.h"
#include "nsILocaleFactory.h"

static NS_DEFINE_CID(kCMorkFactory, NS_MORK_CID);

#if defined(XP_MAC) && defined(CompareString)
	#undef CompareString
#endif
#include "nsICollation.h"

#include "nsCollationCID.h"
#include "nsIPref.h"

#if defined(DEBUG_sspitzer) || defined(DEBUG_seth)
#define DEBUG_MSGKEYSET 1
#endif

static NS_DEFINE_IID(kIPrefIID, NS_IPREF_IID);
static NS_DEFINE_CID(kPrefCID, NS_PREF_CID);
static NS_DEFINE_CID(kCMimeConverterCID, NS_MIME_CONVERTER_CID);
static NS_DEFINE_CID(kLocaleFactoryCID, NS_LOCALEFACTORY_CID);
static NS_DEFINE_IID(kILocaleFactoryIID, NS_ILOCALEFACTORY_IID);
static NS_DEFINE_CID(kLocaleCID, NS_LOCALE_CID);
static NS_DEFINE_IID(kILocaleIID, NS_ILOCALE_IID);
static NS_DEFINE_CID(kCollationFactoryCID, NS_COLLATIONFACTORY_CID);
static NS_DEFINE_IID(kICollationIID, NS_ICOLLATION_IID);
static NS_DEFINE_IID(kICollationFactoryIID, NS_ICOLLATIONFACTORY_IID);
static NS_DEFINE_CID(kMsgHeaderParserCID, NS_MSGHEADERPARSER_CID); 


const int kMsgDBVersion = 1;


nsresult
nsMsgDatabase::CreateMsgHdr(nsIMdbRow* hdrRow, nsMsgKey key, nsIMsgDBHdr* *result)
{
	nsMsgHdr *msgHdr = new nsMsgHdr(this, hdrRow);
	if(!msgHdr)
		return NS_ERROR_OUT_OF_MEMORY;
    msgHdr->SetMessageKey(key);
	msgHdr->AddRef();
    *result = msgHdr;
  
	return NS_OK;
}

NS_IMETHODIMP nsMsgDatabase::AddListener(nsIDBChangeListener *listener)
{
    if (m_ChangeListeners == nsnull) 
	{
        m_ChangeListeners = new nsVoidArray();
        if (!m_ChangeListeners) 
			return NS_ERROR_OUT_OF_MEMORY;
    }
	return m_ChangeListeners->AppendElement(listener);
}

NS_IMETHODIMP nsMsgDatabase::RemoveListener(nsIDBChangeListener *listener)
{
    if (m_ChangeListeners == nsnull) 
		return NS_OK;
	for (PRInt32 i = 0; i < m_ChangeListeners->Count(); i++)
	{
		if ((nsIDBChangeListener *) m_ChangeListeners->ElementAt(i) == listener)
		{
			m_ChangeListeners->RemoveElementAt(i);
			return NS_OK;
		}
	}
	return NS_COMFALSE;
}

	// change announcer methods - just broadcast to all listeners.
NS_IMETHODIMP nsMsgDatabase::NotifyKeyChangeAll(nsMsgKey keyChanged, PRUint32 oldFlags, PRUint32 newFlags,
	nsIDBChangeListener *instigator)
{
    if (m_ChangeListeners == nsnull)
		return NS_OK;
	for (PRInt32 i = 0; i < m_ChangeListeners->Count(); i++)
	{
		nsIDBChangeListener *changeListener =
            (nsIDBChangeListener *) m_ChangeListeners->ElementAt(i);

		nsresult rv = changeListener->OnKeyChange(keyChanged, oldFlags, newFlags, instigator); 
        if (NS_FAILED(rv)) 
			return rv;
	}
    return NS_OK;
}

NS_IMETHODIMP nsMsgDatabase::NotifyKeyDeletedAll(nsMsgKey keyDeleted, PRInt32 flags, 
	nsIDBChangeListener *instigator)
{
    if (m_ChangeListeners == nsnull)
		return NS_OK;
	for (PRInt32 i = 0; i < m_ChangeListeners->Count(); i++)
	{
		nsIDBChangeListener *changeListener = 
            (nsIDBChangeListener *) m_ChangeListeners->ElementAt(i);

		nsresult rv = changeListener->OnKeyDeleted(keyDeleted, flags, instigator); 
        if (NS_FAILED(rv)) return rv;
	}
    return NS_OK;
}

NS_IMETHODIMP nsMsgDatabase::NotifyKeyAddedAll(nsMsgKey keyAdded, PRInt32 flags, 
	nsIDBChangeListener *instigator)
{
    if (m_ChangeListeners == nsnull) 
		return NS_OK;
	for (PRInt32 i = 0; i < m_ChangeListeners->Count(); i++)
	{
		nsIDBChangeListener *changeListener =
            (nsIDBChangeListener *) m_ChangeListeners->ElementAt(i);

		nsresult rv = changeListener->OnKeyAdded(keyAdded, flags, instigator); 
        if (NS_FAILED(rv)) return rv;
	}
    return NS_OK;
}

NS_IMETHODIMP nsMsgDatabase::NotifyAnnouncerGoingAway(void)
{
    if (m_ChangeListeners == nsnull)
		return NS_OK;
	// run loop backwards because listeners remove themselves from the list 
	// on this notification
	for (PRInt32 i = m_ChangeListeners->Count() - 1; i >= 0 ; i--)
	{
		nsIDBChangeListener *changeListener =
            (nsIDBChangeListener *) m_ChangeListeners->ElementAt(i);

		nsresult rv = changeListener->OnAnnouncerGoingAway(this); 
        if (NS_FAILED(rv)) 
			return rv;
	}
    return NS_OK;
}



nsVoidArray *nsMsgDatabase::m_dbCache = NULL;

//----------------------------------------------------------------------
// GetDBCache
//----------------------------------------------------------------------

nsVoidArray/*<nsMsgDatabase>*/*
nsMsgDatabase::GetDBCache()
{
	if (!m_dbCache)
		m_dbCache = new nsVoidArray();

	return m_dbCache;
	
}

void
nsMsgDatabase::CleanupCache()
{
	if (m_dbCache) // clean up memory leak
	{
		for (PRInt32 i = 0; i < GetDBCache()->Count(); i++)
		{
			nsMsgDatabase* pMessageDB = NS_STATIC_CAST(nsMsgDatabase*, GetDBCache()->ElementAt(i));
			if (pMessageDB)
			{
				pMessageDB->ForceClosed();
				i--;	// back up array index, since closing removes db from cache.
			}
		}
		NS_ASSERTION(GetNumInCache() == 0, "some msg dbs left open");	// better not be any open db's.
		delete m_dbCache;
	}
	m_dbCache = nsnull; // Need to reset to NULL since it's a
			  // static global ptr and maybe referenced 
			  // again in other places.
}

//----------------------------------------------------------------------
// FindInCache - this addrefs the db it finds.
//----------------------------------------------------------------------
nsMsgDatabase* nsMsgDatabase::FindInCache(nsFileSpec &dbName)
{
	for (PRInt32 i = 0; i < GetDBCache()->Count(); i++)
	{
		nsMsgDatabase* pMessageDB = NS_STATIC_CAST(nsMsgDatabase*, GetDBCache()->ElementAt(i));
		if (pMessageDB->MatchDbName(dbName))
		{
			NS_ADDREF(pMessageDB);
			return pMessageDB;
		}
	}
	return nsnull;
}

//----------------------------------------------------------------------
// FindInCache
//----------------------------------------------------------------------
int nsMsgDatabase::FindInCache(nsMsgDatabase* pMessageDB)
{
	for (PRInt32 i = 0; i < GetDBCache()->Count(); i++)
	{
		if (GetDBCache()->ElementAt(i) == pMessageDB)
		{
			return(i);
		}
	}
	return(-1);
}

PRBool nsMsgDatabase::MatchDbName(nsFileSpec &dbName)	// returns PR_TRUE if they match
{
	return (m_dbName == dbName); 
}

//----------------------------------------------------------------------
// RemoveFromCache
//----------------------------------------------------------------------
void nsMsgDatabase::RemoveFromCache(nsMsgDatabase* pMessageDB)
{
	int i = FindInCache(pMessageDB);
	if (i != -1)
	{
		GetDBCache()->RemoveElementAt(i);
	}
}


#ifdef DEBUG
void nsMsgDatabase::DumpCache()
{
    nsMsgDatabase* pMessageDB = nsnull;
	for (PRInt32 i = 0; i < GetDBCache()->Count(); i++)
	{
		pMessageDB = NS_STATIC_CAST(nsMsgDatabase*, GetDBCache()->ElementAt(i));
	}
}
#endif /* DEBUG */

nsMsgDatabase::nsMsgDatabase()
    : m_dbFolderInfo(nsnull), m_mdbEnv(nsnull), m_mdbStore(nsnull),
      m_mdbAllMsgHeadersTable(nsnull), m_mdbAllThreadsTable(nsnull), m_dbName(""), m_newSet(nsnull),
      m_mdbTokensInitialized(PR_FALSE), m_ChangeListeners(nsnull),
      m_hdrRowScopeToken(0),
      m_hdrTableKindToken(0),
	  m_threadTableKindToken(0),
      m_subjectColumnToken(0),
      m_senderColumnToken(0),
      m_messageIdColumnToken(0),
      m_referencesColumnToken(0),
      m_recipientsColumnToken(0),
      m_dateColumnToken(0),
      m_messageSizeColumnToken(0),
      m_flagsColumnToken(0),
      m_priorityColumnToken(0),
      m_statusOffsetColumnToken(0),
      m_numLinesColumnToken(0),
      m_ccListColumnToken(0),
	  m_threadFlagsColumnToken(0),
	  m_threadIdColumnToken(0),
	  m_threadChildrenColumnToken(0),
	  m_threadUnreadChildrenColumnToken(0),
	  m_messageThreadIdColumnToken(0),
	  m_threadSubjectColumnToken(0),
	  m_numReferencesColumnToken(0),
	  m_messageCharSetColumnToken(0),
	  m_threadParentColumnToken(0),
	  m_threadRootKeyColumnToken(0),
	  m_HeaderParser(nsnull)
{
	NS_INIT_REFCNT();
}

nsMsgDatabase::~nsMsgDatabase()
{
//	Close(FALSE);	// better have already been closed.
	if (m_HeaderParser)
	{
		NS_RELEASE(m_HeaderParser);
		m_HeaderParser = nsnull;
	}
    if (m_ChangeListeners) 
	{
        // better not be any listeners, because we're going away.
        NS_ASSERTION(m_ChangeListeners->Count() == 0, "shouldn't have any listeners");
        delete m_ChangeListeners;
    }

    if (m_newSet) {
#ifdef DEBUG_MSGKEYSET
        char *str = nsnull;
        str = m_newSet->Output();
        if (str) {
            printf("setStr = %s on destroy\n",str);
            delete [] str;
            str = nsnull;
        }
#endif
        delete m_newSet;
        m_newSet = nsnull;
    }
}

NS_IMPL_ADDREF(nsMsgDatabase)

NS_IMETHODIMP_(nsrefcnt) nsMsgDatabase::Release(void)                    
{                                                      
	NS_PRECONDITION(0 != mRefCnt, "dup release");     
	if (--mRefCnt == 0)	// OK, the cache is no longer holding onto this, so we really want to delete it, 
	{						// after removing it from the cache.
		RemoveFromCache(this);
#ifdef DEBUG_bienvenu1
		if (GetNumInCache() != 0)
		{
			XP_Trace("closing %s\n", m_dbName);
			DumpCache();
		}
#endif
		if (m_mdbStore)
		{
			m_mdbStore->CloseMdbObject(m_mdbEnv);
		}
		NS_DELETEXPCOM(this);                              
		return 0;                                          
	}
	return mRefCnt;                                      
}

// NS_IMPL_RELEASE(nsMsgDatabase)

NS_IMETHODIMP nsMsgDatabase::QueryInterface(REFNSIID aIID, void** aResult)
{   
    if (aResult == NULL)  
        return NS_ERROR_NULL_POINTER;  

    if (aIID.Equals(nsIMsgDatabase::GetIID()) ||
        aIID.Equals(nsIDBChangeAnnouncer::GetIID()) ||
        aIID.Equals(nsCOMTypeInfo<nsISupports>::GetIID())) {
        *aResult = NS_STATIC_CAST(nsIMsgDatabase*, this);   
        NS_ADDREF_THIS();
        return NS_OK;
    }
    return NS_NOINTERFACE;
}   

/* static */ nsIMdbFactory *nsMsgDatabase::GetMDBFactory()
{
	static nsIMdbFactory *gMDBFactory = nsnull;
	if (!gMDBFactory)
	{
		nsresult rv;
        rv = nsComponentManager::CreateInstance(kCMorkFactory, nsnull, nsIMdbFactoryFactory::GetIID(), (void **) &gMDBFactory);
	}
	return gMDBFactory;
}

#ifdef XP_PC
// this code is stolen from nsFileSpecWin. Since MDB requires a native path, for 
// the time being, we'll just take the Unix/Canonical form and munge it
void nsMsgDatabase::UnixToNative(char*& ioPath)
// This just does string manipulation.  It doesn't check reality, or canonify, or
// anything
//----------------------------------------------------------------------------------------
{
	// Allow for relative or absolute.  We can do this in place, because the
	// native path is never longer.
	
	if (!ioPath || !*ioPath)
		return;
		
	char* src = ioPath;
	if (*ioPath == '/')
    {
      // Strip initial slash for an absolute path
      src++;
    }
		
	// Convert the vertical slash to a colon
	char* cp = src + 1;
	
	// If it was an absolute path, check for the drive letter
	if (*ioPath == '/' && strstr(cp, "|/") == cp)
    *cp = ':';
	
	// Convert '/' to '\'.
	while (*++cp)
    {
      if (*cp == '/')
        *cp = '\\';
    }

	if (*ioPath == '/') {
    for (cp = ioPath; *cp; ++cp)
      *cp = *(cp + 1);
  }
}
#endif /* XP_PC */

#ifdef XP_MAC
// this code is stolen from nsFileSpecMac. Since MDB requires a native path, for 
// the time being, we'll just take the Unix/Canonical form and munge it
void nsMsgDatabase::UnixToNative(char*& ioPath)
// This just does string manipulation.  It doesn't check reality, or canonify, or
// anything
//----------------------------------------------------------------------------------------
{
	// Relying on the fact that the unix path is always longer than the mac path:
	size_t len = strlen(ioPath);
	char* result = new char[len + 2]; // ... but allow for the initial colon in a partial name
	if (result)
	{
		char* dst = result;
		const char* src = ioPath;
		if (*src == '/')		 	// * full path
			src++;
		else if (strchr(src, '/'))	// * partial path, and not just a leaf name
			*dst++ = ':';
		strcpy(dst, src);

		while ( *dst != 0)
		{
			if (*dst == '/')
				*dst++ = ':';
			else
				*dst++;
		}
		nsCRT::free(ioPath);
		ioPath = result;
	}
}

void nsMsgDatabase::NativeToUnix(char*& ioPath)
// This just does string manipulation.  It doesn't check reality, or canonify, or
// anything
//----------------------------------------------------------------------------------------
{
	size_t len = strlen(ioPath);
	char* result = new char[len + 2]; // ... but allow for the initial colon in a partial name
	if (result)
	{
		char* dst = result;
		const char* src = ioPath;
		if (*src == ':')		 	// * partial path, and not just a leaf name
			src++;
		else if (strchr(src, ':'))	// * full path
			*dst++ = '/';
		strcpy(dst, src);

		while ( *dst != 0)
		{
			if (*dst == ':')
				*dst++ = '/';
			else
				*dst++;
		}
		PR_Free(ioPath);
		ioPath = result;
	}
}
#endif /* XP_MAC */

NS_IMETHODIMP nsMsgDatabase::Open(nsIFileSpec *folderName, PRBool create, PRBool upgrading, nsIMsgDatabase** pMessageDB)
{
	NS_ASSERTION(FALSE, "must override");
	return NS_ERROR_NOT_IMPLEMENTED;
}

// Open the MDB database synchronously. If successful, this routine
// will set up the m_mdbStore and m_mdbEnv of the database object 
// so other database calls can work.
NS_IMETHODIMP nsMsgDatabase::OpenMDB(const char *dbName, PRBool create)
{
	nsresult ret = NS_OK;
	nsIMdbFactory *myMDBFactory = GetMDBFactory();
	if (myMDBFactory)
	{
		ret = myMDBFactory->MakeEnv(NULL, &m_mdbEnv);
		if (NS_SUCCEEDED(ret))
		{
			nsIMdbThumb *thumb = nsnull;
			struct stat st;
			char	*nativeFileName = nsCRT::strdup(dbName);

			if (!nativeFileName)
				return NS_ERROR_OUT_OF_MEMORY;

			if (m_mdbEnv)
				m_mdbEnv->SetAutoClear(PR_TRUE);
			m_dbName = dbName;
#if defined(XP_PC) || defined(XP_MAC)
			UnixToNative(nativeFileName);
#endif
			if (stat(nativeFileName, &st)) 
				ret = NS_MSG_ERROR_FOLDER_SUMMARY_MISSING;
			else
			{
				mdbOpenPolicy inOpenPolicy;
				mdb_bool	canOpen;
				mdbYarn		outFormatVersion;
				char		bufFirst512Bytes[512];
				mdbYarn		first512Bytes;

				first512Bytes.mYarn_Buf = bufFirst512Bytes;
				first512Bytes.mYarn_Size = 512;
				first512Bytes.mYarn_Fill = 512;
				first512Bytes.mYarn_Form = 0;	// what to do with this? we're storing csid in the msg hdr...

				{
					nsIOFileStream *dbStream = new nsIOFileStream(nsFileSpec(dbName));
					if (dbStream)
					{
						PRInt32 bytesRead = dbStream->read(bufFirst512Bytes, sizeof(bufFirst512Bytes));
						first512Bytes.mYarn_Fill = bytesRead;
						dbStream->close();
						delete dbStream;
					}
					else
						return NS_ERROR_OUT_OF_MEMORY;
				}
				ret = myMDBFactory->CanOpenFilePort(m_mdbEnv, nativeFileName, // the file to investigate
					&first512Bytes,	&canOpen, &outFormatVersion);
				if (ret == 0 && canOpen)
				{

					inOpenPolicy.mOpenPolicy_ScopePlan.mScopeStringSet_Count = 0;
					inOpenPolicy.mOpenPolicy_MinMemory = 0;
					inOpenPolicy.mOpenPolicy_MaxLazy = 0;

					ret = myMDBFactory->OpenFileStore(m_mdbEnv, NULL, nativeFileName, &inOpenPolicy, 
									&thumb); 
				}
				else
					ret = NS_MSG_ERROR_FOLDER_SUMMARY_OUT_OF_DATE;
			}
			if (NS_SUCCEEDED(ret) && thumb)
			{
				mdb_count outTotal;    // total somethings to do in operation
				mdb_count outCurrent;  // subportion of total completed so far
				mdb_bool outDone = PR_FALSE;      // is operation finished?
				mdb_bool outBroken;     // is operation irreparably dead and broken?
				do
				{
					ret = thumb->DoMore(m_mdbEnv, &outTotal, &outCurrent, &outDone, &outBroken);
					if (ret != 0)
					{// mork isn't really doing NS erorrs yet.
						outDone = PR_TRUE;
						break;
					}
				}
				while (NS_SUCCEEDED(ret) && !outBroken && !outDone);
//				m_mdbEnv->ClearErrors(); // ### temporary...
				if (NS_SUCCEEDED(ret) && outDone)
				{
					ret = myMDBFactory->ThumbToOpenStore(m_mdbEnv, thumb, &m_mdbStore);
					if (ret == NS_OK && m_mdbStore)
						ret = InitExistingDB();
				}
#ifdef DEBUG_bienvenu1
				DumpContents();
#endif
			}
			else if (create)	// ### need error code saying why open file store failed
			{
				mdbOpenPolicy inOpenPolicy;

				inOpenPolicy.mOpenPolicy_ScopePlan.mScopeStringSet_Count = 0;
				inOpenPolicy.mOpenPolicy_MinMemory = 0;
				inOpenPolicy.mOpenPolicy_MaxLazy = 0;

				ret = myMDBFactory->CreateNewFileStore(m_mdbEnv, NULL, dbName, &inOpenPolicy, &m_mdbStore);
				if (ret == NS_OK)
					ret = InitNewDB();
			}
			if(thumb)
			{
				thumb->CutStrongRef(m_mdbEnv);
			}
			nsCRT::free(nativeFileName);
		}
	}
	return ret;
}

NS_IMETHODIMP nsMsgDatabase::CloseMDB(PRBool commit)
{
	if (commit)
		Commit(nsMsgDBCommitType::kSessionCommit);
	return(NS_OK);
}

// force the database to close - this'll flush out anybody holding onto
// a database without having a listener!
// This is evil in the com world, but there are times we need to delete the file.
NS_IMETHODIMP nsMsgDatabase::ForceClosed()
{
	nsresult	err = NS_OK;
    nsCOMPtr<nsIMsgDatabase> aDb(do_QueryInterface(this, &err));

	// make sure someone has a reference so object won't get deleted out from under us.
	AddRef();	
	NotifyAnnouncerGoingAway();
	// OK, remove from cache first and close the store.
	RemoveFromCache(this);

	err = CloseMDB(PR_FALSE);	// since we're about to delete it, no need to commit.
	if (m_mdbStore)
	{
		m_mdbStore->CloseMdbObject(m_mdbEnv);
		m_mdbStore = nsnull;
	}
	Release();
	return err;
}

// caller must Release result.
NS_IMETHODIMP nsMsgDatabase::GetDBFolderInfo(nsIDBFolderInfo	**result)
{
	*result = m_dbFolderInfo;
	if (m_dbFolderInfo)
		m_dbFolderInfo->AddRef();
	return NS_OK;
}

NS_IMETHODIMP nsMsgDatabase::Commit(nsMsgDBCommit commitType)
{
	nsresult	err = NS_OK;
	nsIMdbThumb	*commitThumb = NULL;

//	commitType = nsMsgDBCommitType::kCompressCommit;	// ### until incremental writing works.

	if (m_mdbStore)
	{
		switch (commitType)
		{
		case nsMsgDBCommitType::kSmallCommit:
			err = m_mdbStore->SmallCommit(GetEnv());
			break;
		case nsMsgDBCommitType::kLargeCommit:
			err = m_mdbStore->LargeCommit(GetEnv(), &commitThumb);
			break;
		case nsMsgDBCommitType::kSessionCommit:
			// comment out until persistence works.
			err = m_mdbStore->SessionCommit(GetEnv(), &commitThumb);
			break;
		case nsMsgDBCommitType::kCompressCommit:
			err = m_mdbStore->CompressCommit(GetEnv(), &commitThumb);
			break;
		}
	}
	if (commitThumb)
	{
		mdb_count outTotal = 0;    // total somethings to do in operation
		mdb_count outCurrent = 0;  // subportion of total completed so far
		mdb_bool outDone = PR_FALSE;      // is operation finished?
		mdb_bool outBroken = PR_FALSE;     // is operation irreparably dead and broken?
		while (!outDone && !outBroken && err == NS_OK)
		{
			err = commitThumb->DoMore(GetEnv(), &outTotal, &outCurrent, &outDone, &outBroken);
		}
		NS_RELEASE(commitThumb);
	}
	// ### do something with error, but clear it now because mork errors out on commits.
	if (GetEnv())
		GetEnv()->ClearErrors();
	return err;
}

NS_IMETHODIMP nsMsgDatabase::Close(PRBool forceCommit /* = TRUE */)
{
	return CloseMDB(forceCommit);
}

const char *kMsgHdrsScope = "ns:msg:db:row:scope:msgs:all";	// scope for all headers table
const char *kMsgHdrsTableKind = "ns:msg:db:table:kind:msgs";
const char *kThreadTableKind = "ns:msg:db:table:kind:thread";
const char *kThreadHdrsScope = "ns:msg:db:row:scope:threads:all"; // scope for all threads table
const char *kAllThreadsTableKind = "ns:msg:db:table:kind:allthreads"; // kind for table of all threads
const char *kSubjectColumnName = "subject";
const char *kSenderColumnName = "sender";
const char *kMessageIdColumnName = "message-id";
const char *kReferencesColumnName = "references";
const char *kRecipientsColumnName = "recipients";
const char *kDateColumnName = "date";
const char *kMessageSizeColumnName = "size";
const char *kFlagsColumnName = "flags";
const char *kPriorityColumnName = "priority";
const char *kStatusOffsetColumnName = "statusOfset";
const char *kNumLinesColumnName = "numLines";
const char *kCCListColumnName = "ccList";
const char *kMessageThreadIdColumnName = "msgThreadId";
const char *kNumReferencesColumnName = "numRefs";
const char *kThreadFlagsColumnName = "threadFlags";
const char *kThreadIdColumnName = "threadId";
const char *kThreadChildrenColumnName = "children";
const char *kThreadUnreadChildrenColumnName = "unreadChildren";
const char *kThreadSubjectColumnName = "threadSubject";
const char *kMessageCharSetColumnName = "msgCharSet";
const char *kThreadParentColumnName = "threadParent";
const char *kThreadRootColumnName = "threadRoot";
struct mdbOid gAllMsgHdrsTableOID;
struct mdbOid gAllThreadsTableOID;

// set up empty tables, dbFolderInfo, etc.
nsresult nsMsgDatabase::InitNewDB()
{
	nsresult err = NS_OK;

	err = InitMDBInfo();
	if (err == NS_OK)
	{
		// why is this bad? dbFolderInfo is tightly tightly bound to nsMsgDatabase. It will
		// never be provided by someone else. It could be strictly embedded inside nsMsgDatabase
		// but I wanted to keep nsMsgDatabase a little bit smaller
		nsDBFolderInfo *dbFolderInfo = new nsDBFolderInfo(this); // this is bad!! Should go through component manager
		if (dbFolderInfo)
		{
			NS_ADDREF(dbFolderInfo); // mscott: shouldn't have to do this...go through c. manager
			err = dbFolderInfo->AddToNewMDB();
			dbFolderInfo->SetVersion(GetCurVersion());
			nsIMdbStore *store = GetStore();
			// create the unique table for the dbFolderInfo.
			mdb_err mdberr;

            mdberr = (nsresult) store->NewTable(GetEnv(), m_hdrRowScopeToken, 
				m_hdrTableKindToken, PR_FALSE, nsnull, &m_mdbAllMsgHeadersTable);
//			m_mdbAllMsgHeadersTable->BecomeContent(GetEnv(), &gAllMsgHdrsTableOID);
            mdberr = (nsresult) store->NewTable(GetEnv(), m_threadRowScopeToken, 
				m_allThreadsTableKindToken, PR_FALSE, nsnull, &m_mdbAllThreadsTable);

			m_dbFolderInfo = dbFolderInfo;

		}
		else
			err = NS_ERROR_OUT_OF_MEMORY;
	}
	return err;
}

nsresult nsMsgDatabase::InitExistingDB()
{
	nsresult err = NS_OK;

	err = InitMDBInfo();
	if (err == NS_OK)
	{
		err = GetStore()->GetTable(GetEnv(), &gAllMsgHdrsTableOID, &m_mdbAllMsgHeadersTable);
		if (err == NS_OK)
		{
			m_dbFolderInfo = new nsDBFolderInfo(this);	 // mscott: This is bad!!! Should be going through component manager
			if (m_dbFolderInfo)
			{
				NS_ADDREF(m_dbFolderInfo); // mscott: acquire a ref count. we shouldn't do this & should be going through
									       // the component manager instead.
				m_dbFolderInfo->InitFromExistingDB();
			}
		}
		err = GetStore()->GetTable(GetEnv(), &gAllThreadsTableOID, &m_mdbAllThreadsTable);
		// this may be a db without this table - let's just create one.
		if (!m_mdbAllThreadsTable)
            err = (nsresult) GetStore()->NewTable(GetEnv(), m_threadRowScopeToken, 
				m_allThreadsTableKindToken, PR_FALSE, nsnull, &m_mdbAllThreadsTable);

	}
	return err;
}

// initialize the various tokens and tables in our db's env
nsresult nsMsgDatabase::InitMDBInfo()
{
	nsresult err = NS_OK;

	if (!m_mdbTokensInitialized && GetStore())
	{
		m_mdbTokensInitialized = PR_TRUE;
		err	= GetStore()->StringToToken(GetEnv(), kMsgHdrsScope, &m_hdrRowScopeToken); 
		if (err == NS_OK)
		{
			GetStore()->StringToToken(GetEnv(),  kSubjectColumnName, &m_subjectColumnToken);
			GetStore()->StringToToken(GetEnv(),  kSenderColumnName, &m_senderColumnToken);
			GetStore()->StringToToken(GetEnv(),  kMessageIdColumnName, &m_messageIdColumnToken);
			// if we just store references as a string, we won't get any savings from the
			// fact there's a lot of duplication. So we may want to break them up into
			// multiple columns, r1, r2, etc.
			GetStore()->StringToToken(GetEnv(),  kReferencesColumnName, &m_referencesColumnToken);
			// similarly, recipients could be tokenized properties
			GetStore()->StringToToken(GetEnv(),  kRecipientsColumnName, &m_recipientsColumnToken);
			GetStore()->StringToToken(GetEnv(),  kDateColumnName, &m_dateColumnToken);
			GetStore()->StringToToken(GetEnv(),  kMessageSizeColumnName, &m_messageSizeColumnToken);
			GetStore()->StringToToken(GetEnv(),  kFlagsColumnName, &m_flagsColumnToken);
			GetStore()->StringToToken(GetEnv(),  kPriorityColumnName, &m_priorityColumnToken);
			GetStore()->StringToToken(GetEnv(),  kStatusOffsetColumnName, &m_statusOffsetColumnToken);
			GetStore()->StringToToken(GetEnv(),  kNumLinesColumnName, &m_numLinesColumnToken);
			GetStore()->StringToToken(GetEnv(),  kCCListColumnName, &m_ccListColumnToken);
			GetStore()->StringToToken(GetEnv(),  kMessageThreadIdColumnName, &m_messageThreadIdColumnToken);
			GetStore()->StringToToken(GetEnv(),  kThreadIdColumnName, &m_threadIdColumnToken);
			GetStore()->StringToToken(GetEnv(),  kThreadFlagsColumnName, &m_threadFlagsColumnToken);
			GetStore()->StringToToken(GetEnv(),  kThreadChildrenColumnName, &m_threadChildrenColumnToken);
			GetStore()->StringToToken(GetEnv(),  kThreadUnreadChildrenColumnName, &m_threadUnreadChildrenColumnToken);
			GetStore()->StringToToken(GetEnv(),  kThreadSubjectColumnName, &m_threadSubjectColumnToken);
			GetStore()->StringToToken(GetEnv(),  kNumReferencesColumnName, &m_numReferencesColumnToken);
			GetStore()->StringToToken(GetEnv(),  kMessageCharSetColumnName, &m_messageCharSetColumnToken);
			err = GetStore()->StringToToken(GetEnv(), kMsgHdrsTableKind, &m_hdrTableKindToken); 
			if (err == NS_OK)
				err = GetStore()->StringToToken(GetEnv(), kThreadTableKind, &m_threadTableKindToken);
			err = GetStore()->StringToToken(GetEnv(), kAllThreadsTableKind, &m_allThreadsTableKindToken); 
			err	= GetStore()->StringToToken(GetEnv(), kThreadHdrsScope, &m_threadRowScopeToken); 
			err	= GetStore()->StringToToken(GetEnv(), kThreadParentColumnName, &m_threadParentColumnToken);
			err	= GetStore()->StringToToken(GetEnv(), kThreadRootColumnName, &m_threadRootKeyColumnToken);
			if (err == NS_OK)
			{
				// The table of all message hdrs will have table id 1.
				gAllMsgHdrsTableOID.mOid_Scope = m_hdrRowScopeToken;
				gAllMsgHdrsTableOID.mOid_Id = 1;
				gAllThreadsTableOID.mOid_Scope = m_threadRowScopeToken;
				gAllThreadsTableOID.mOid_Id = 1;

			}
		}
	}
	return err;
}

// Returns if the db contains this key
NS_IMETHODIMP nsMsgDatabase::ContainsKey(nsMsgKey key, PRBool *containsKey)
{

	nsresult	err = NS_OK;
	mdb_bool	hasOid;
	mdbOid		rowObjectId;

	if (!containsKey || !m_mdbAllMsgHeadersTable)
		return NS_ERROR_NULL_POINTER;
	*containsKey = PR_FALSE;

	rowObjectId.mOid_Id = key;
	rowObjectId.mOid_Scope = m_hdrRowScopeToken;
	err = m_mdbAllMsgHeadersTable->HasOid(GetEnv(), &rowObjectId, &hasOid);
	if(NS_SUCCEEDED(err))
		*containsKey = hasOid;

	return err;
}

// get a message header for the given key. Caller must release()!
NS_IMETHODIMP nsMsgDatabase::GetMsgHdrForKey(nsMsgKey key, nsIMsgDBHdr **pmsgHdr)
{
	nsresult	err = NS_OK;
	mdb_bool	hasOid;
	mdbOid		rowObjectId;


	if (!pmsgHdr || !m_mdbAllMsgHeadersTable)
		return NS_ERROR_NULL_POINTER;

	*pmsgHdr = NULL;
	rowObjectId.mOid_Id = key;
	rowObjectId.mOid_Scope = m_hdrRowScopeToken;
	err = m_mdbAllMsgHeadersTable->HasOid(GetEnv(), &rowObjectId, &hasOid);
	if (err == NS_OK && m_mdbStore /* && hasOid */)
	{
		nsIMdbRow *hdrRow;
		err = m_mdbStore->GetRow(GetEnv(), &rowObjectId, &hdrRow);

		if (err == NS_OK && hdrRow)
		{
			err = CreateMsgHdr(hdrRow,  key, pmsgHdr);
		}
	}

	return err;
}

NS_IMETHODIMP nsMsgDatabase::DeleteMessage(nsMsgKey key, nsIDBChangeListener *instigator, PRBool commit)
{
	nsresult	err = NS_OK;
	nsIMsgDBHdr *msgHdr = NULL;

	err = GetMsgHdrForKey(key, &msgHdr);
	if (msgHdr == NULL)
		return NS_MSG_MESSAGE_NOT_FOUND;

	err = DeleteHeader(msgHdr, instigator, commit, PR_TRUE);
	NS_IF_RELEASE(msgHdr);
	return err;
}


NS_IMETHODIMP nsMsgDatabase::DeleteMessages(nsMsgKeyArray* nsMsgKeys, nsIDBChangeListener *instigator)
{
	nsresult	err = NS_OK;

    PRUint32 kindex;
	for (kindex = 0; kindex < nsMsgKeys->GetSize(); kindex++)
	{
		nsMsgKey key = nsMsgKeys->ElementAt(kindex);
		nsIMsgDBHdr *msgHdr = NULL;
		
		err = GetMsgHdrForKey(key, &msgHdr);
        if (NS_FAILED(err)) 
		{
			err = NS_MSG_MESSAGE_NOT_FOUND;
			break;
		}
		if (msgHdr)
			err = DeleteHeader(msgHdr, instigator, kindex % 300 == 0, PR_TRUE);
		NS_IF_RELEASE(msgHdr);
		if (err != NS_OK)
			break;
	}
	Commit(nsMsgDBCommitType::kSmallCommit);
	return err;
}


NS_IMETHODIMP nsMsgDatabase::DeleteHeader(nsIMsgDBHdr *msg, nsIDBChangeListener *instigator, PRBool commit, PRBool notify)
{
    nsMsgHdr* msgHdr = NS_STATIC_CAST(nsMsgHdr*, msg);  // closed system, so this is ok
	nsMsgKey key;
    (void)msg->GetMessageKey(&key);
	// only need to do this for mail - will this speed up news expiration? 
//	if (GetMailDB())
		SetHdrFlag(msg, PR_TRUE, MSG_FLAG_EXPUNGED);	// tell mailbox (mail)

	if (m_newSet)	// if it's in the new set, better get rid of it.
		m_newSet->Remove(key);

	if (m_dbFolderInfo != NULL)
	{
		PRBool isRead;
		m_dbFolderInfo->ChangeNumMessages(-1);
		m_dbFolderInfo->ChangeNumVisibleMessages(-1);
		IsRead(key, &isRead);
		if (!isRead)
			m_dbFolderInfo->ChangeNumNewMessages(-1);

        PRUint32 size;
        (void)msg->GetMessageSize(&size);
		m_dbFolderInfo->ChangeExpungedBytes (size);

	}	


//	if (!onlyRemoveFromThread)	// to speed up expiration, try this. But really need to do this in RemoveHeaderFromDB
	nsresult ret = RemoveHeaderFromDB(msgHdr);
	
	if (notify && NS_SUCCEEDED(ret))
	{
        PRUint32 flags;
        (void)msg->GetFlags(&flags);
		NotifyKeyDeletedAll(key, flags, instigator); // tell listeners
    }

	if (commit)
		Commit(nsMsgDBCommitType::kLargeCommit);			// ### dmb is this a good time to commit?
	return ret;
}

NS_IMETHODIMP
nsMsgDatabase::UndoDelete(nsIMsgDBHdr *msgHdr)
{
    if (msgHdr)
    {
        SetHdrFlag(msgHdr, FALSE, MSG_FLAG_EXPUNGED);
        // ** do we need to update folder info regarding the message size
    }
    return NS_OK;
}


// This is a lower level routine which doesn't send notifcations or
// update folder info. One use is when a rule fires moving a header
// from one db to another, to remove it from the first db.

nsresult nsMsgDatabase::RemoveHeaderFromDB(nsMsgHdr *msgHdr)
{
	if (!msgHdr)
		return NS_ERROR_NULL_POINTER;
	nsresult ret = NS_OK;
	 // turn this on when Scottip has rdf stuff worked out.
	nsCOMPtr <nsIMsgThread> thread ;
	ret = GetThreadContainingMsgHdr(msgHdr, getter_AddRefs(thread));
	if (NS_SUCCEEDED(ret))
		ret = thread->RemoveChildHdr(msgHdr);
	else
		NS_ASSERTION(PR_FALSE, "couldn't find thread containing deleted message");
	// even if we couldn't find the thread,we should try to remove the header.
	if (NS_SUCCEEDED(ret))
	{
		nsIMdbRow* row = msgHdr->GetMDBRow();
		ret = m_mdbAllMsgHeadersTable->CutRow(GetEnv(), msgHdr->GetMDBRow());
		row->CutAllColumns(GetEnv());
	}
	return ret;
}

nsresult nsMsgDatabase::IsRead(nsMsgKey key, PRBool *pRead)
{
	nsresult rv;
	nsIMsgDBHdr *msgHdr;

	rv = GetMsgHdrForKey(key, &msgHdr);
    if (NS_FAILED(rv) || !msgHdr) 
		return NS_MSG_MESSAGE_NOT_FOUND; // XXX return rv?
    rv = IsHeaderRead(msgHdr, pRead);
    NS_RELEASE(msgHdr);
    return rv;
}

PRUint32	nsMsgDatabase::GetStatusFlags(nsIMsgDBHdr *msgHdr)
{
	PRUint32	statusFlags;
    (void)msgHdr->GetFlags(&statusFlags);
	PRBool	isRead;

    nsMsgKey key;
    (void)msgHdr->GetMessageKey(&key);
	if (m_newSet && m_newSet->IsMember(key))
		statusFlags |= MSG_FLAG_NEW;
	if (IsRead(key, &isRead) == NS_OK && isRead)
		statusFlags |= MSG_FLAG_READ;
	return statusFlags;
}

NS_IMETHODIMP nsMsgDatabase::IsHeaderRead(nsIMsgDBHdr *hdr, PRBool *pRead)
{
	if (!hdr)
		return NS_MSG_MESSAGE_NOT_FOUND;

    PRUint32 flags;
    (void)hdr->GetFlags(&flags);
	*pRead = (flags & MSG_FLAG_READ) != 0;
	return NS_OK;
}

NS_IMETHODIMP nsMsgDatabase::IsMarked(nsMsgKey key, PRBool *pMarked)
{
	nsresult rv;
	nsIMsgDBHdr *msgHdr;

	rv = GetMsgHdrForKey(key, &msgHdr);
    if (NS_FAILED(rv))
		return NS_MSG_MESSAGE_NOT_FOUND; // XXX return rv?

    PRUint32 flags;
    (void)msgHdr->GetFlags(&flags);
    *pMarked = (flags & MSG_FLAG_MARKED) == MSG_FLAG_MARKED;
    NS_RELEASE(msgHdr);
    return rv;
}

NS_IMETHODIMP nsMsgDatabase::IsIgnored(nsMsgKey key, PRBool *pIgnored)
{
	PR_ASSERT(pIgnored != NULL);
	if (!pIgnored)
		return NS_ERROR_NULL_POINTER;
#ifdef WE_DO_THREADING_YET
	nsIMsgThread *threadHdr = GetnsThreadHdrForMsgID(nsMsgKey);
	// This should be very surprising, but we leave that up to the caller
	// to determine for now.
	if (threadHdr == NULL)
		return NS_MSG_MESSAGE_NOT_FOUND;
	*pIgnored = (threadHdr->GetFlags() & MSG_FLAG_IGNORED) ? PR_TRUE : PR_FALSE;
	NS_RELEASE(threadHdr);
#endif
	return NS_OK;
}

nsresult nsMsgDatabase::HasAttachments(nsMsgKey key, PRBool *pHasThem)
{
	nsresult rv;

	PR_ASSERT(pHasThem != NULL);
	if (!pHasThem)
		return NS_ERROR_NULL_POINTER;

	nsIMsgDBHdr *msgHdr;

	rv = GetMsgHdrForKey(key, &msgHdr);
    if (NS_FAILED(rv)) 
		return rv;

    PRUint32 flags;
    (void)msgHdr->GetFlags(&flags);
    *pHasThem = (flags & MSG_FLAG_ATTACHMENT) ? PR_TRUE : PR_FALSE;
    NS_RELEASE(msgHdr);
	return rv;
}

NS_IMETHODIMP nsMsgDatabase::MarkHdrReadInDB(nsIMsgDBHdr *msgHdr, PRBool bRead,
                                             nsIDBChangeListener *instigator)
{
    nsresult rv;
    nsMsgKey key;
	PRUint32 oldFlags;
    (void)msgHdr->GetMessageKey(&key);
	msgHdr->GetFlags(&oldFlags);
	SetHdrFlag(msgHdr, bRead, MSG_FLAG_READ);

	if (m_newSet)
		m_newSet->Remove(key);

	if (m_dbFolderInfo != NULL)
	{
		if (bRead)
			m_dbFolderInfo->ChangeNumNewMessages(-1);
		else
			m_dbFolderInfo->ChangeNumNewMessages(1);
	}

    PRUint32 flags;
    rv = msgHdr->GetFlags(&flags);
    if (NS_FAILED(rv)) return rv;
    
	return NotifyKeyChangeAll(key, oldFlags, flags, instigator);
}

NS_IMETHODIMP nsMsgDatabase::MarkRead(nsMsgKey key, PRBool bRead, 
						   nsIDBChangeListener *instigator)
{
	nsresult rv;
	nsIMsgDBHdr *msgHdr;
	
	rv = GetMsgHdrForKey(key, &msgHdr);
    if (NS_FAILED(rv)) 
		return NS_MSG_MESSAGE_NOT_FOUND; // XXX return rv?

	rv = MarkHdrRead(msgHdr, bRead, instigator);
	NS_RELEASE(msgHdr);
	return rv;
}

NS_IMETHODIMP nsMsgDatabase::MarkReplied(nsMsgKey key, PRBool bReplied, 
								nsIDBChangeListener *instigator /* = NULL */)
{
	return SetKeyFlag(key, bReplied, MSG_FLAG_REPLIED, instigator);
}

NS_IMETHODIMP nsMsgDatabase::MarkForwarded(nsMsgKey key, PRBool bForwarded, 
								nsIDBChangeListener *instigator /* = NULL */) 
{
	return SetKeyFlag(key, bForwarded, MSG_FLAG_FORWARDED, instigator);
}

NS_IMETHODIMP nsMsgDatabase::MarkHasAttachments(nsMsgKey key, PRBool bHasAttachments, 
								nsIDBChangeListener *instigator)
{
	return SetKeyFlag(key, bHasAttachments, MSG_FLAG_ATTACHMENT, instigator);
}

NS_IMETHODIMP
nsMsgDatabase::MarkThreadIgnored(nsIMsgThread *thread, nsMsgKey threadKey, PRBool bIgnored,
                                 nsIDBChangeListener *instigator)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsMsgDatabase::MarkThreadWatched(nsIMsgThread *thread, nsMsgKey threadKey, PRBool bWatched,
                                 nsIDBChangeListener *instigator)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsMsgDatabase::MarkMarked(nsMsgKey key, PRBool mark,
										nsIDBChangeListener *instigator)
{
	return SetKeyFlag(key, mark, MSG_FLAG_MARKED, instigator);
}

NS_IMETHODIMP nsMsgDatabase::MarkOffline(nsMsgKey key, PRBool offline,
										nsIDBChangeListener *instigator)
{
	return SetKeyFlag(key, offline, MSG_FLAG_OFFLINE, instigator);
}

NS_IMETHODIMP
nsMsgDatabase::AllMsgKeysImapDeleted(nsMsgKeyArray *keys)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsMsgDatabase::MarkImapDeleted(nsMsgKey key, PRBool deleted,
										nsIDBChangeListener *instigator)
{
	return SetKeyFlag(key, deleted, MSG_FLAG_IMAP_DELETED, instigator);
}

NS_IMETHODIMP nsMsgDatabase::MarkMDNNeeded(nsMsgKey key, PRBool bNeeded, 
								nsIDBChangeListener *instigator /* = NULL */)
{
	return SetKeyFlag(key, bNeeded, MSG_FLAG_MDN_REPORT_NEEDED, instigator);
}

NS_IMETHODIMP nsMsgDatabase::IsMDNNeeded(nsMsgKey key, PRBool *pNeeded)
{
	nsresult rv;
	nsIMsgDBHdr *msgHdr = nsnull;
	
    rv = GetMsgHdrForKey(key, &msgHdr);
    if (NS_FAILED(rv) || !msgHdr)
		return NS_MSG_MESSAGE_NOT_FOUND; // XXX return rv?

    PRUint32 flags;
    (void)msgHdr->GetFlags(&flags);
    *pNeeded = ((flags & MSG_FLAG_MDN_REPORT_NEEDED) == MSG_FLAG_MDN_REPORT_NEEDED);
    NS_RELEASE(msgHdr);
    return rv;
}


nsresult nsMsgDatabase::MarkMDNSent(nsMsgKey key, PRBool bSent, 
							  nsIDBChangeListener *instigator /* = NULL */)
{
	return SetKeyFlag(key, bSent, MSG_FLAG_MDN_REPORT_SENT, instigator);
}


nsresult nsMsgDatabase::IsMDNSent(nsMsgKey key, PRBool *pSent)
{
	nsresult rv;
	nsIMsgDBHdr *msgHdr = nsnull;
	
	rv = GetMsgHdrForKey(key, &msgHdr);
    if (NS_FAILED(rv) || !msgHdr) 
		return NS_MSG_MESSAGE_NOT_FOUND; // XXX return rv?

    PRUint32 flags;
    (void)msgHdr->GetFlags(&flags);
    *pSent = flags & MSG_FLAG_MDN_REPORT_SENT;
    NS_RELEASE(msgHdr);
    return rv;
}


nsresult	nsMsgDatabase::SetKeyFlag(nsMsgKey key, PRBool set, PRUint32 flag,
							  nsIDBChangeListener *instigator)
{
	nsresult rv;
	nsIMsgDBHdr *msgHdr = nsnull;
		
    rv = GetMsgHdrForKey(key, &msgHdr);
    if (NS_FAILED(rv) || !msgHdr) 
		return NS_MSG_MESSAGE_NOT_FOUND; // XXX return rv?

	PRUint32 oldFlags;
	msgHdr->GetFlags(&oldFlags);

	SetHdrFlag(msgHdr, set, flag);

    PRUint32 flags;
    (void)msgHdr->GetFlags(&flags);
	NotifyKeyChangeAll(key, oldFlags, flags, instigator);

	NS_RELEASE(msgHdr);
	return rv;
}

// Helper routine - lowest level of flag setting - returns PR_TRUE if flags change,
// PR_FALSE otherwise.
PRBool nsMsgDatabase::SetHdrFlag(nsIMsgDBHdr *msgHdr, PRBool bSet, MsgFlags flag)
{
//	PR_ASSERT(! (flag & kDirty));	// this won't do the right thing so don't.
	PRUint32 currentStatusFlags = GetStatusFlags(msgHdr);
	PRBool flagAlreadySet = (currentStatusFlags & flag) != 0;

	if ((flagAlreadySet && !bSet) || (!flagAlreadySet && bSet))
	{
        PRUint32 resultFlags;
		if (bSet)
		{
			msgHdr->OrFlags(flag, &resultFlags);
		}
		else
		{
			msgHdr->AndFlags(~flag, &resultFlags);
		}
		return PR_TRUE;
	}
	return PR_FALSE;
}


NS_IMETHODIMP nsMsgDatabase::MarkHdrRead(nsIMsgDBHdr *msgHdr, PRBool bRead, 
                                         nsIDBChangeListener *instigator)
{
    nsresult rv = NS_OK;
	PRBool	isRead;
	IsHeaderRead(msgHdr, &isRead);
	// if the flag is already correct in the db, don't change it
	if (!!isRead != !!bRead)
	{
#ifdef WE_DO_THREADING_YET
		nsIMsgThread *threadHdr = GetnsThreadHdrForMsgID(msgHdr->GetMessageKey());
		if (threadHdr != NULL)
		{
			threadHdr->MarkChildRead(bRead);
			NS_RELEASE(threadHdr);
		}
#endif
		rv = MarkHdrReadInDB(msgHdr, bRead, instigator);
	}
	return rv;
}

NS_IMETHODIMP nsMsgDatabase::MarkAllRead(nsMsgKeyArray *thoseMarked)
{
	nsresult		rv;
	nsMsgHdr		*pHeader;
	//ListContext		*listContext = NULL;
	PRInt32			numChanged = 0;

    nsIEnumerator* hdrs;
    rv = EnumerateMessages(&hdrs);
    if (NS_FAILED(rv))
		return rv;
	for (hdrs->First(); hdrs->IsDone() != NS_OK; hdrs->Next()) 
	{
        rv = hdrs->CurrentItem((nsISupports**)&pHeader);
        NS_ASSERTION(NS_SUCCEEDED(rv), "nsMsgDBEnumerator broken");
        if (NS_FAILED(rv)) 
			break;

		if (thoseMarked) 
		{
            nsMsgKey key;
            (void)pHeader->GetMessageKey(&key);
			thoseMarked->Add(key);
        }
		rv = MarkHdrRead(pHeader, PR_TRUE, NULL); 	// ### dmb - blow off error?
		numChanged++;
		NS_RELEASE(pHeader);
	}

	if (numChanged > 0)	// commit every once in a while
		Commit(nsMsgDBCommitType::kSmallCommit);
	// force num new to 0.
	PRInt32 numNewMessages;

	rv = m_dbFolderInfo->GetNumNewMessages(&numNewMessages);
	if (rv == NS_OK)
		m_dbFolderInfo->ChangeNumNewMessages(-numNewMessages);
	return rv;
}

NS_IMETHODIMP nsMsgDatabase::MarkReadByDate (PRTime startDate, PRTime endDate, nsMsgKeyArray *markedIds)
{
	nsresult rv;
	nsMsgHdr	*pHeader;
	//ListContext		*listContext = NULL;
	PRInt32			numChanged = 0;

    nsIEnumerator* hdrs;
    rv = EnumerateMessages(&hdrs);
    if (NS_FAILED(rv)) 
		return rv;
		
	nsTime t_startDate(startDate);
	nsTime t_endDate(endDate);
		
	for (hdrs->First(); hdrs->IsDone() != NS_OK; hdrs->Next()) 
	{
        rv = hdrs->CurrentItem((nsISupports**)&pHeader);
        NS_ASSERTION(NS_SUCCEEDED(rv), "nsMsgDBEnumerator broken");
        if (NS_FAILED(rv)) break;

		PRTime headerDate;
        (void)pHeader->GetDate(&headerDate);
        nsTime t_headerDate(headerDate);
        
		if (t_headerDate > t_startDate && t_headerDate <= t_endDate)
		{
			PRBool isRead;
            nsMsgKey key;
            (void)pHeader->GetMessageKey(&key);
			IsRead(key, &isRead);
			if (!isRead)
			{
				numChanged++;
				if (markedIds)
					markedIds->Add(key);
				rv = MarkHdrRead(pHeader, PR_TRUE, NULL);	// ### dmb - blow off error?
			}
		}
		NS_RELEASE(pHeader);
	}
	if (numChanged > 0)
		Commit(nsMsgDBCommitType::kSmallCommit);
	return rv;
}

NS_IMETHODIMP nsMsgDatabase::MarkLater(nsMsgKey key, PRTime until)
{
	PR_ASSERT(m_dbFolderInfo);
	if (m_dbFolderInfo != NULL)
	{
		m_dbFolderInfo->AddLaterKey(key, until);
	}
	return NS_OK;
}

NS_IMETHODIMP nsMsgDatabase::AddToNewList(nsMsgKey key)
{
    nsresult rv = NS_OK;

    if (!m_newSet) {
        m_newSet = nsMsgKeySet::Create("" /* , this */);
        if (!m_newSet) return NS_ERROR_OUT_OF_MEMORY;
    }

    rv = m_newSet->Add(key);
    
	return rv;
}


NS_IMETHODIMP nsMsgDatabase::ClearNewList(PRBool notify /* = FALSE */)
{
	nsresult			err = NS_OK;
	if (m_newSet)
	{
		if (notify)	// need to update view
		{
			PRInt32 firstMember;
			while ((firstMember = m_newSet->GetFirstMember()) != 0)
			{
				m_newSet->Remove(firstMember);	// this bites, since this will cause us to regen new list many times.
				nsIMsgDBHdr *msgHdr;
				err = GetMsgHdrForKey(firstMember, &msgHdr);
				if (NS_SUCCEEDED(err))
				{
                    nsMsgKey key;
                    (void)msgHdr->GetMessageKey(&key);
                    PRUint32 flags;
                    (void)msgHdr->GetFlags(&flags);
					NotifyKeyChangeAll(key, flags | MSG_FLAG_NEW, flags, NULL);
					NS_RELEASE(msgHdr);
				}
			}
		}
		delete m_newSet;
		m_newSet = NULL;
	}
    else {
        NS_ASSERTION(0, "no set!\n");
    }
    return err;
}

NS_IMETHODIMP nsMsgDatabase::HasNew()
{
	return (m_newSet && m_newSet->getLength() > 0) ? NS_OK : NS_COMFALSE;
}

NS_IMETHODIMP nsMsgDatabase::GetFirstNew(nsMsgKey *result)
{
	// even though getLength is supposedly for debugging only, it's the only
	// way I can tell if the set is empty (as opposed to having a member 0.
	if (NS_SUCCEEDED(HasNew()))
		*result = m_newSet->GetFirstMember();
	else
		*result = nsMsgKey_None;
    return NS_OK;
}


////////////////////////////////////////////////////////////////////////////////

class nsMsgDBEnumerator : public nsIEnumerator {
public:
    NS_DECL_ISUPPORTS

    // nsIEnumerator methods:
    NS_IMETHOD First(void);
    NS_IMETHOD Next(void);
    NS_IMETHOD CurrentItem(nsISupports **aItem);
    NS_IMETHOD IsDone(void);

    // nsMsgDBEnumerator methods:
    typedef nsresult (*nsMsgDBEnumeratorFilter)(nsIMsgDBHdr* hdr, void* closure);

    nsMsgDBEnumerator(nsMsgDatabase* db, 
                      nsMsgDBEnumeratorFilter filter, void* closure);
    virtual ~nsMsgDBEnumerator();

protected:
    nsMsgDatabase*              mDB;
	nsIMdbTableRowCursor*       mRowCursor;
    nsIMsgDBHdr*                 mResultHdr;
    PRBool                      mDone;
    nsMsgDBEnumeratorFilter     mFilter;
    void*                       mClosure;
};

nsMsgDBEnumerator::nsMsgDBEnumerator(nsMsgDatabase* db,
                                     nsMsgDBEnumeratorFilter filter, void* closure)
    : mDB(db), mRowCursor(nsnull), mResultHdr(nsnull), mDone(PR_FALSE),
      mFilter(filter), mClosure(closure)
{
    NS_INIT_REFCNT();
    NS_ADDREF(mDB);
}

nsMsgDBEnumerator::~nsMsgDBEnumerator()
{
    NS_RELEASE(mDB);
	NS_IF_RELEASE(mResultHdr);
}

NS_IMPL_ISUPPORTS(nsMsgDBEnumerator, nsIEnumerator::GetIID())

NS_IMETHODIMP nsMsgDBEnumerator::First(void)
{
	nsresult rv = 0;
	mDone = PR_FALSE;

	if (!mDB || !mDB->m_mdbAllMsgHeadersTable)
		return NS_ERROR_NULL_POINTER;
		
	mDB->m_mdbAllMsgHeadersTable->GetTableRowCursor(mDB->GetEnv(), -1, &mRowCursor);
	if (NS_FAILED(rv)) return rv;
    return Next();
}

NS_IMETHODIMP nsMsgDBEnumerator::Next(void)
{
	nsresult rv;
	nsIMdbRow* hdrRow;
	mdb_pos rowPos;
	PRUint32 flags;

    do {
        NS_IF_RELEASE(mResultHdr);
        mResultHdr = nsnull;
        rv = mRowCursor->NextRow(mDB->GetEnv(), &hdrRow, &rowPos);
		if (!hdrRow) {
            mDone = PR_TRUE;
			return NS_ERROR_FAILURE;
        }
        if (NS_FAILED(rv)) {
            mDone = PR_TRUE;
            return rv;
        }
		//Get key from row
		mdbOid outOid;
		nsMsgKey key=0;
		if (hdrRow->GetOid(mDB->GetEnv(), &outOid) == NS_OK)
            key = outOid.mOid_Id;

        rv = mDB->CreateMsgHdr(hdrRow, key, &mResultHdr);
        if (NS_FAILED(rv))
			return rv;

		if (mResultHdr)
			mResultHdr->GetFlags(&flags);
		else
			flags = 0;
    } 
	while (mFilter && mFilter(mResultHdr, mClosure) != NS_OK && !(flags & MSG_FLAG_EXPUNGED));
	return rv;
}

NS_IMETHODIMP nsMsgDBEnumerator::CurrentItem(nsISupports **aItem)
{
    if (mResultHdr) {
        *aItem = mResultHdr;
        NS_ADDREF(mResultHdr);
        return NS_OK;
    }
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP nsMsgDBEnumerator::IsDone(void)
{
    return mDone ? NS_OK : NS_COMFALSE;
}

////////////////////////////////////////////////////////////////////////////////

NS_IMETHODIMP 
nsMsgDatabase::EnumerateMessages(nsIEnumerator* *result)
{
    nsMsgDBEnumerator* e = new nsMsgDBEnumerator(this, nsnull, nsnull);
    if (e == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(e);
    *result = e;
    return NS_OK;
}


#if HAVE_INT_ENUMERATORS
NS_IMETHODIMP nsMsgDatabase::EnumerateKeys(nsIEnumerator* *result)
{
    nsISupportsArray* keys;
    nsresult rv = NS_NewISupportsArray(&keys);
    if (NS_FAILED(rv)) return rv;

	nsIMdbTableRowCursor *rowCursor;
	rv = m_mdbAllMsgHeadersTable->GetTableRowCursor(GetEnv(), -1, &rowCursor);

	while (NS_SUCCEEDED(rv)) {
		mdbOid outOid;
		mdb_pos	outPos;

		rv = rowCursor->NextRowOid(GetEnv(), &outOid, &outPos);
        if (NS_FAILED(rv)) return rv;
		if (outPos < 0)	// is this right?
			break;
        keys->AppendElement(outOid.mOid_Id);
	}
	return keys->Enumerate(result);
}
#else
NS_IMETHODIMP nsMsgDatabase::ListAllKeys(nsMsgKeyArray &outputKeys)
{
	nsresult	err = NS_OK;
	nsIMdbTableRowCursor *rowCursor;
	if (m_mdbAllMsgHeadersTable)
	{
		err = m_mdbAllMsgHeadersTable->GetTableRowCursor(GetEnv(), -1, &rowCursor);
		while (err == NS_OK && rowCursor)
		{
			mdbOid outOid;
			mdb_pos	outPos;

			err = rowCursor->NextRowOid(GetEnv(), &outOid, &outPos);
			// is this right? Mork is returning a 0 id, but that should valid.
			if (outPos < 0 || outOid.mOid_Id == -1)	
				break;
			if (err == NS_OK)
				outputKeys.Add(outOid.mOid_Id);
		}
	}
	return err;
}
#endif


class nsMsgDBThreadEnumerator : public nsIEnumerator
{
public:
    NS_DECL_ISUPPORTS

    // nsIEnumerator methods:
    NS_IMETHOD First(void);
    NS_IMETHOD Next(void);
    NS_IMETHOD CurrentItem(nsISupports **aItem);
    NS_IMETHOD IsDone(void);

    // nsMsgDBEnumerator methods:
    typedef nsresult (*nsMsgDBThreadEnumeratorFilter)(nsIMsgThread* hdr, void* closure);

    nsMsgDBThreadEnumerator(nsMsgDatabase* db, 
                      nsMsgDBThreadEnumeratorFilter filter, void* closure);
    virtual ~nsMsgDBThreadEnumerator();

protected:
    nsMsgDatabase*              mDB;
	nsIMdbPortTableCursor*       mTableCursor;
    nsIMsgThread*                 mResultThread;
    PRBool                      mDone;
    nsMsgDBThreadEnumeratorFilter     mFilter;
    void*                       mClosure;
};

nsMsgDBThreadEnumerator::nsMsgDBThreadEnumerator(nsMsgDatabase* db,
                                     nsMsgDBThreadEnumeratorFilter filter, void* closure)
    : mDB(db), mTableCursor(nsnull), mResultThread(nsnull), mDone(PR_FALSE),
      mFilter(filter), mClosure(closure)
{
    NS_INIT_REFCNT();
    NS_ADDREF(mDB);
}

nsMsgDBThreadEnumerator::~nsMsgDBThreadEnumerator()
{
	NS_IF_RELEASE(mTableCursor);
	NS_IF_RELEASE(mResultThread);
    NS_RELEASE(mDB);
}

NS_IMPL_ISUPPORTS(nsMsgDBThreadEnumerator, nsIEnumerator::GetIID())

NS_IMETHODIMP nsMsgDBThreadEnumerator::First(void)
{
	nsresult rv = 0;

	if (!mDB || !mDB->m_mdbStore)
		return NS_ERROR_NULL_POINTER;
		
	mDB->m_mdbStore->GetPortTableCursor(mDB->GetEnv(),   mDB->m_hdrRowScopeToken, mDB->m_threadTableKindToken,
	    &mTableCursor);

	if (NS_FAILED(rv)) 
		return rv;
    return Next();
}

NS_IMETHODIMP nsMsgDBThreadEnumerator::Next(void)
{
	nsresult rv;
	nsIMdbTable *table = nsnull;

    while (PR_TRUE) 
	{
        NS_IF_RELEASE(mResultThread);
        mResultThread = nsnull;
        rv = mTableCursor->NextTable(mDB->GetEnv(), &table);
		if (!table) 
		{
            mDone = PR_TRUE;
			return NS_ERROR_FAILURE;
        }
        if (NS_FAILED(rv)) 
		{
            mDone = PR_TRUE;
            return rv;
        }

        if (NS_FAILED(rv)) 
			return rv;

        mResultThread = new nsMsgThread(mDB, table);
		if(mResultThread)
		{
			PRUint32 numChildren = 0;
			NS_ADDREF(mResultThread);
			mResultThread->GetNumChildren(&numChildren);
			// we've got empty thread; don't tell caller about it.
			if (numChildren == 0)
				continue;
		}
		if (mFilter && mFilter(mResultThread, mClosure) != NS_OK)
			continue;
		else
			break;
    }
	return rv;
}

NS_IMETHODIMP nsMsgDBThreadEnumerator::CurrentItem(nsISupports **aItem)
{
    if (mResultThread) 
	{
        *aItem = mResultThread;
        NS_ADDREF(mResultThread);
        return NS_OK;
    }
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP nsMsgDBThreadEnumerator::IsDone(void)
{
    return mDone ? NS_OK : NS_COMFALSE;
}

NS_IMETHODIMP 
nsMsgDatabase::EnumerateThreads(nsIEnumerator* *result)
{
    nsMsgDBThreadEnumerator* e = new nsMsgDBThreadEnumerator(this, nsnull, nsnull);
    if (e == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(e);
    *result = e;
    return NS_OK;
}


#if 0
// convenience function to iterate through a db only looking for unread messages.
// It turns out to be possible to do this more efficiently in news, since read/unread
// status is kept in a much more compact format (the newsrc format).
// So this is the base implementation, which news databases override.
nsresult nsMsgDatabase::ListNextUnread(ListContext **pContext, nsMsgHdr **pResult)
{
	nsMsgHdr	*pHeader;
	nsresult			dbErr = NS_OK;
	PRBool			lastWasRead = TRUE;
	*pResult = NULL;

	while (PR_TRUE)
	{
		if (*pContext == NULL)
			dbErr = ListFirst (pContext, &pHeader);
		else
			dbErr = ListNext(*pContext, &pHeader);

		if (dbErr != NS_OK)
		{
			ListDone(*pContext);
			break;
		}

		else if (dbErr != NS_OK)	 
			break;
		if (IsHeaderRead(pHeader, &lastWasRead) == NS_OK && !lastWasRead)
			break;
		else
			NS_RELEASE(pHeader);
	}
	if (!lastWasRead)
		*pResult = pHeader;
	return dbErr;
}
#else
static nsresult
nsMsgUnreadFilter(nsIMsgDBHdr* msg, void* closure)
{
    nsMsgDatabase* db = (nsMsgDatabase*)closure;
    PRBool wasRead;
    nsresult rv = db->IsHeaderRead(msg, &wasRead);
    if (NS_FAILED(rv)) 
		return rv;
    return !wasRead ? NS_OK : NS_COMFALSE;
}

NS_IMETHODIMP 
nsMsgDatabase::EnumerateUnreadMessages(nsIEnumerator* *result)
{
    nsMsgDBEnumerator* e = new nsMsgDBEnumerator(this, nsMsgUnreadFilter, this);
    if (e == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(e);
    *result = e;
    return NS_OK;
}
#endif

NS_IMETHODIMP nsMsgDatabase::CreateNewHdr(nsMsgKey key, nsIMsgDBHdr **pnewHdr)
{
	nsresult	err = NS_OK;
	nsIMdbRow		*hdrRow;
	struct mdbOid allMsgHdrsTableOID;

	if (!pnewHdr || !m_mdbAllMsgHeadersTable || !m_mdbStore)
		return NS_ERROR_NULL_POINTER;

	allMsgHdrsTableOID.mOid_Scope = m_hdrRowScopeToken;
	allMsgHdrsTableOID.mOid_Id = key;	// presumes 0 is valid key value

	err = m_mdbStore->GetRow(GetEnv(), &allMsgHdrsTableOID, &hdrRow);
	if (!hdrRow)	
		err  = m_mdbStore->NewRowWithOid(GetEnv(), &allMsgHdrsTableOID, &hdrRow);

	if (NS_FAILED(err)) 
		return err;
    err = CreateMsgHdr(hdrRow, key, pnewHdr);
	return err;
}

NS_IMETHODIMP nsMsgDatabase::AddNewHdrToDB(nsIMsgDBHdr *newHdr, PRBool notify)
{
    nsMsgHdr* hdr = NS_STATIC_CAST(nsMsgHdr*, newHdr);          // closed system, cast ok
	PRBool newThread;

	nsresult err = ThreadNewHdr(hdr, newThread);
	// we thread header before we add it to the all headers table
	// so that subject threading will work (otherwise, when we try
	// to find the first header with the same subject, we get the
	// new header!
	if (NS_SUCCEEDED(err))
	{
		nsMsgKey key;
		PRUint32 flags;

		newHdr->GetMessageKey(&key);
		newHdr->GetFlags(&flags);
		if (flags & MSG_FLAG_NEW)
		{
			PRUint32 newFlags;
			newHdr->AndFlags(~MSG_FLAG_NEW, &newFlags);	// make sure not filed out
			AddToNewList(key);
		}
		if (m_dbFolderInfo != NULL)
		{
			m_dbFolderInfo->ChangeNumMessages(1);
			m_dbFolderInfo->ChangeNumVisibleMessages(1);
			if (! (flags & MSG_FLAG_READ))
				m_dbFolderInfo->ChangeNumNewMessages(1);
		}
		err = m_mdbAllMsgHeadersTable->AddRow(GetEnv(), hdr->GetMDBRow());
		if (notify)
		{
			NotifyKeyAddedAll(key, flags, NULL);
		}
	}
	return err;
}

NS_IMETHODIMP nsMsgDatabase::CopyHdrFromExistingHdr(nsMsgKey key, nsIMsgDBHdr *existingHdr, nsIMsgDBHdr **newHdr)
{
	nsresult	err = NS_OK;

	if (existingHdr)
	{
	    nsMsgHdr* sourceMsgHdr = NS_STATIC_CAST(nsMsgHdr*, existingHdr);      // closed system, cast ok
		nsMsgHdr *destMsgHdr = nsnull;
		CreateNewHdr(key, (nsIMsgDBHdr **) &destMsgHdr);
		nsIMdbRow	*sourceRow = sourceMsgHdr->GetMDBRow() ;
		nsIMdbRow	*destRow = destMsgHdr->GetMDBRow();
		err = destRow->SetRow(GetEnv(), sourceRow);
		if (NS_SUCCEEDED(err))
		{
			err = AddNewHdrToDB(destMsgHdr, PR_TRUE);
			if (NS_SUCCEEDED(err) && newHdr)
				*newHdr = destMsgHdr;
		}

	}
	return err;
}

nsresult nsMsgDatabase::RowCellColumnTonsString(nsIMdbRow *hdrRow, mdb_token columnToken, nsString &resultStr)
{
	nsresult	err = NS_OK;
	nsIMdbCell	*hdrCell;

	if (hdrRow)	// ### probably should be an error if hdrRow is NULL...
	{
		err = hdrRow->GetCell(GetEnv(), columnToken, &hdrCell);
		if (err == NS_OK && hdrCell)
		{
			struct mdbYarn yarn;
			hdrCell->AliasYarn(GetEnv(), &yarn);
			YarnTonsString(&yarn, &resultStr);
			hdrCell->CutStrongRef(GetEnv()); // always release ref
		}
	}
	return err;
}

nsresult nsMsgDatabase::RowCellColumnToMime2EncodedString(nsIMdbRow *row, mdb_token columnToken, nsString &resultStr)
{
	nsresult err;
	nsString nakedString;
	err = RowCellColumnTonsString(row, columnToken, nakedString);
	if (NS_SUCCEEDED(err) && nakedString.Length() > 0)
	{
		// apply mime decode
		nsIMimeConverter *converter;
		err = nsComponentManager::CreateInstance(kCMimeConverterCID, nsnull, 
                                            nsIMimeConverter::GetIID(), (void **)&converter);

		if (NS_SUCCEEDED(err) && nsnull != converter) 
		{
			nsString charset;
			nsString decodedStr;
			m_dbFolderInfo->GetCharacterSet(&charset);
			err = converter->DecodeMimePartIIStr(nakedString, charset, resultStr);
			NS_RELEASE(converter);
		}
	}
	return err;
}

nsresult nsMsgDatabase::RowCellColumnToCollationKey(nsIMdbRow *row, mdb_token columnToken, nsString &resultStr)
{
	nsString nakedString;
	nsresult err;

	err = RowCellColumnTonsString(row, columnToken, nakedString);
	if (NS_SUCCEEDED(err))
	{
		nsILocaleFactory* localeFactory; 
		nsILocale* locale; 
		nsString localeName; 

		// get a locale factory 
		err = nsComponentManager::FindFactory(kLocaleFactoryCID, (nsIFactory**)&localeFactory); 
		if (NS_SUCCEEDED(err) && localeFactory)
		{
			// do this for a new db if no UI to be provided for locale selection 
			err = localeFactory->GetApplicationLocale(&locale); 

			// or generate a locale from a stored locale name ("en_US", "fr_FR") 
			//err = localeFactory->NewLocale(&localeName, &locale); 

			// release locale factory
			NS_RELEASE(localeFactory);

			nsICollationFactory *f;

			err = nsComponentManager::CreateInstance(kCollationFactoryCID, NULL,
									  kICollationFactoryIID, (void**) &f); 
			if (NS_SUCCEEDED(err) && f)
			{
				nsICollation *inst;

				// get a collation interface instance 
				err = f->CreateCollation(locale, &inst);

				// release locale, collation factory
				NS_RELEASE(locale);
				NS_RELEASE(f);

				if (NS_SUCCEEDED(err) && inst)
				{
					err = inst->CreateSortKey( kCollationCaseInSensitive, nakedString, resultStr) ;
					NS_RELEASE(inst);
				}
			}
 		}
	}
	return err;
}

nsIMsgHeaderParser *nsMsgDatabase::GetHeaderParser()
{

	if (!m_HeaderParser)
	{
		nsresult rv = nsComponentManager::CreateInstance(kMsgHeaderParserCID, 
													NULL, 
													nsIMsgHeaderParser::GetIID(), 
													(void **) &m_HeaderParser);
		if (!NS_SUCCEEDED(rv))
			m_HeaderParser = nsnull;
	}
	return m_HeaderParser;
}


nsresult nsMsgDatabase::RowCellColumnToUInt32(nsIMdbRow *hdrRow, mdb_token columnToken, PRUint32 &uint32Result, PRUint32 defaultValue)
{
	return RowCellColumnToUInt32(hdrRow, columnToken, &uint32Result, defaultValue);
}

nsresult nsMsgDatabase::RowCellColumnToUInt32(nsIMdbRow *hdrRow, mdb_token columnToken, PRUint32 *uint32Result, PRUint32 defaultValue)
{
	nsresult	err = NS_OK;
	nsIMdbCell	*hdrCell;

	if (uint32Result)
		*uint32Result = defaultValue;
	if (hdrRow)	// ### probably should be an error if hdrRow is NULL...
	{
		err = hdrRow->GetCell(GetEnv(), columnToken, &hdrCell);
		if (err == NS_OK && hdrCell)
		{
			struct mdbYarn yarn;
			hdrCell->AliasYarn(GetEnv(), &yarn);
			YarnToUInt32(&yarn, uint32Result);
			hdrCell->CutStrongRef(GetEnv()); // always release ref
		}
	}
	return err;
}

nsresult nsMsgDatabase::UInt32ToRowCellColumn(nsIMdbRow *row, mdb_token columnToken, PRUint32 value)
{
	struct mdbYarn yarn;
	char	yarnBuf[100];

	yarn.mYarn_Buf = (void *) yarnBuf;
	yarn.mYarn_Size = sizeof(yarnBuf);
	yarn.mYarn_Fill = yarn.mYarn_Size;
	yarn.mYarn_Form = 0;
	yarn.mYarn_Grow = NULL;
	return row->AddColumn(GetEnv(),  columnToken, UInt32ToYarn(&yarn, value));
}

nsresult nsMsgDatabase::CharPtrToRowCellColumn(nsIMdbRow *row, mdb_token columnToken, const char *charPtr)
{
	struct mdbYarn yarn;
	yarn.mYarn_Buf = (void *) charPtr;
	yarn.mYarn_Size = PL_strlen((const char *) yarn.mYarn_Buf) + 1;
	yarn.mYarn_Fill = yarn.mYarn_Size - 1;
	yarn.mYarn_Form = 0;	// what to do with this? we're storing csid in the msg hdr...

	return row->AddColumn(GetEnv(),  columnToken, &yarn);
}

nsresult nsMsgDatabase::RowCellColumnToCharPtr(nsIMdbRow *row, mdb_token columnToken, char **result)
{
	nsresult	err = NS_ERROR_NULL_POINTER;
	nsIMdbCell	*hdrCell;

	if (row && result)
	{
		err = row->GetCell(GetEnv(), columnToken, &hdrCell);
		if (err == NS_OK && hdrCell)
		{
			struct mdbYarn yarn;
			hdrCell->AliasYarn(GetEnv(), &yarn);
			*result = (char *) PR_CALLOC(yarn.mYarn_Fill + 1);
			if (*result && yarn.mYarn_Fill > 0)
				nsCRT::memcpy(*result, yarn.mYarn_Buf, yarn.mYarn_Fill);
			else
				err = NS_ERROR_OUT_OF_MEMORY;

			hdrCell->CutStrongRef(GetEnv()); // always release ref
		}
	}
	return err;
}



/* static */struct mdbYarn *nsMsgDatabase::nsStringToYarn(struct mdbYarn *yarn, nsString *str)
{
	yarn->mYarn_Buf = str->ToNewCString();
	yarn->mYarn_Size = PL_strlen((const char *) yarn->mYarn_Buf) + 1;
	yarn->mYarn_Fill = yarn->mYarn_Size - 1;
	yarn->mYarn_Form = 0;	// what to do with this? we're storing csid in the msg hdr...
	return yarn;
}

/* static */struct mdbYarn *nsMsgDatabase::UInt32ToYarn(struct mdbYarn *yarn, PRUint32 i)
{
	PR_snprintf((char *) yarn->mYarn_Buf, yarn->mYarn_Size, "%lx", i);
	yarn->mYarn_Fill = PL_strlen((const char *) yarn->mYarn_Buf);
	yarn->mYarn_Form = 0;	// what to do with this? Should be parsed out of the mime2 header?
	return yarn;
}

/* static */void nsMsgDatabase::YarnTonsString(struct mdbYarn *yarn, nsString *str)
{
	str->SetString((const char *) yarn->mYarn_Buf, yarn->mYarn_Fill);
}

/* static */void nsMsgDatabase::YarnToUInt32(struct mdbYarn *yarn, PRUint32 *pResult)
{
	PRUint32 result;
	char *p = (char *) yarn->mYarn_Buf;
	PRInt32 numChars = MIN(8, yarn->mYarn_Fill);
	PRInt32 i;
	for (i=0, result = 0; i<numChars; i++, p++)
	{
		char C = *p;

		PRInt8 unhex = ((C >= '0' && C <= '9') ? C - '0' :
			((C >= 'A' && C <= 'F') ? C - 'A' + 10 :
			 ((C >= 'a' && C <= 'f') ? C - 'a' + 10 : -1)));
		if (unhex < 0)
			break;
		result = (result << 4) | unhex;
	}
    
	*pResult = result;
}

/* static */void nsMsgDatabase::PRTime2Seconds(PRTime prTime, PRUint32 *seconds)
{
	PRInt64 microSecondsPerSecond, intermediateResult;
	
	LL_I2L(microSecondsPerSecond, PR_USEC_PER_SEC);
	LL_DIV(intermediateResult, prTime, microSecondsPerSecond);
    LL_L2UI((*seconds), intermediateResult);
}

/* static */void nsMsgDatabase::Seconds2PRTime(PRUint32 seconds, PRTime *prTime)
{
	PRInt64 microSecondsPerSecond, intermediateResult;
	
	LL_I2L(microSecondsPerSecond, PR_USEC_PER_SEC);
    LL_UI2L(intermediateResult, seconds);
	LL_MUL((*prTime), intermediateResult, microSecondsPerSecond);
}


PRUint32 nsMsgDatabase::GetCurVersion()
{
	return kMsgDBVersion;
}

nsresult nsMsgDatabase::SetSummaryValid(PRBool valid /* = PR_TRUE */)
{
	// setting the version to -1 ought to make it pretty invalid.
	if (!valid)
		m_dbFolderInfo->SetVersion(-1);

	// for default db (and news), there's no nothing to set to make it it valid
	return NS_OK;
}

// protected routines

nsresult nsMsgDatabase::CreateNewThread(nsMsgKey threadId, const char *subject, nsMsgThread **pnewThread)
{
	nsresult	err = NS_OK;
	nsIMdbTable		*threadTable;
	struct mdbOid threadTableOID;
	struct mdbOid allThreadsTableOID;

	if (!pnewThread || !m_mdbAllThreadsTable || !m_mdbStore)
		return NS_ERROR_NULL_POINTER;

	threadTableOID.mOid_Scope = m_hdrRowScopeToken;
	threadTableOID.mOid_Id = threadId;

	err  = GetStore()->NewTableWithOid(GetEnv(), &threadTableOID, m_threadTableKindToken, 
                                     PR_FALSE, nsnull, &threadTable);
	if (NS_FAILED(err)) 
		return err;

	allThreadsTableOID.mOid_Scope = m_threadRowScopeToken;
	allThreadsTableOID.mOid_Id = threadId;	

	// add a row for this thread in the table of all threads that we'll use
	// to do our mapping between subject strings and threads.
	nsIMdbRow *threadRow = nsnull;

	err = m_mdbStore->GetRow(GetEnv(), &allThreadsTableOID, &threadRow);
	if (!threadRow)	
	{
		err  = m_mdbStore->NewRowWithOid(GetEnv(), &allThreadsTableOID, &threadRow);
		if (NS_SUCCEEDED(err) && threadRow)
		{
			err = CharPtrToRowCellColumn(threadRow, m_threadSubjectColumnToken, subject);
			threadRow->Release();
		}
	}

	*pnewThread = new nsMsgThread(this, threadTable);
	if (*pnewThread)
		(*pnewThread)->SetThreadKey(threadId);
	return err;
}


nsIMsgThread *nsMsgDatabase::GetThreadForReference(nsString2 &msgID, nsIMsgDBHdr **pMsgHdr)
{
	nsIMsgDBHdr	*msgHdr = GetMsgHdrForMessageID(msgID);  
	nsIMsgThread *thread = NULL;

	if (msgHdr != NULL)
	{
		nsMsgKey threadId;
		if (NS_SUCCEEDED(msgHdr->GetThreadId(&threadId)))
		{
			// find thread header for header whose message id we matched.
			thread = GetThreadForThreadId(threadId);
		}
		if (pMsgHdr)
			*pMsgHdr = msgHdr;
		else
			msgHdr->Release();
	}
	return thread;
}

nsIMsgThread *	nsMsgDatabase::GetThreadForSubject(nsString2 &subject)
{
//	NS_ASSERTION(PR_FALSE, "not implemented yet.");
	nsIMsgThread *thread = NULL;

	nsIMsgDBHdr	*msgHdr = nsnull;
    nsresult rv = NS_OK;
	mdbYarn	subjectYarn;

	subjectYarn.mYarn_Buf = (void*)subject.GetBuffer();
	subjectYarn.mYarn_Fill = PL_strlen(subject.GetBuffer());
	subjectYarn.mYarn_Form = 0;
	subjectYarn.mYarn_Size = subjectYarn.mYarn_Fill;

	nsIMdbRow	*threadRow;
	mdbOid		outRowId;
	mdb_err result = GetStore()->FindRow(GetEnv(), m_threadRowScopeToken,
		m_threadSubjectColumnToken, &subjectYarn,  &outRowId, &threadRow);
	if (NS_SUCCEEDED(result) && threadRow)
	{
		//Get key from row
		mdbOid outOid;
		nsMsgKey key = 0;
		if (threadRow->GetOid(GetEnv(), &outOid) == NS_OK)
			key = outOid.mOid_Id;
		// find thread header for header whose message id we matched.
		thread = GetThreadForThreadId(key);
	}

	return thread;
}

nsresult nsMsgDatabase::ThreadNewHdr(nsMsgHdr* newHdr, PRBool &newThread)
{
	nsresult result=NS_ERROR_UNEXPECTED;
	nsCOMPtr <nsIMsgThread> thread;
	nsCOMPtr <nsIMsgDBHdr> replyToHdr;
	nsMsgKey threadId = nsMsgKey_None;

	if (!newHdr)
		return NS_ERROR_NULL_POINTER;

	PRUint16 numReferences = 0;
	PRUint32 newHdrFlags = 0;

	newHdr->GetFlags(&newHdrFlags);
	newHdr->GetNumReferences(&numReferences);

#define SUBJ_THREADING 1// try reference threading first
	for (PRInt32 i = numReferences - 1; i >= 0;  i--)
	{
		nsString2 reference(eOneByte);

		newHdr->GetStringReference(i, reference);
		// first reference we have hdr for is best top-level hdr.
		// but we have to handle case of promoting new header to top-level
		// in case the top-level header comes after a reply.

		if (reference.Length() == 0)
			break;

		thread = getter_AddRefs(GetThreadForReference(reference, getter_AddRefs(replyToHdr))) ;
		if (thread)
		{
			thread->GetThreadKey(&threadId);
			newHdr->SetThreadId(threadId);
			result = AddToThread(newHdr, thread, replyToHdr, TRUE);
			break;
		}
	}
#ifdef SUBJ_THREADING
	// try subject threading if we couldn't find a reference and the subject starts with Re:
	nsAutoString subject (eOneByte);

	newHdr->GetSubject(subject);
	if ((ThreadBySubjectWithoutRe() || (newHdrFlags & MSG_FLAG_HAS_RE)) && (!thread))
	{
		thread = getter_AddRefs(GetThreadForSubject(subject));
		if(thread)
		{
			thread->GetThreadKey(&threadId);
			newHdr->SetThreadId(threadId);
			//TRACE("threading based on subject %s\n", (const char *) msgHdr->m_subject);
			// if we move this and do subject threading after, ref threading, 
			// don't thread within children, since we know it won't work. But for now, pass TRUE.
			result = AddToThread(newHdr, thread, nsnull, TRUE);     
		}
	}
#endif // SUBJ_THREADING

	if (!thread)
	{
		// couldn't find any parent articles - msgHdr is top-level thread, for now
		result = AddNewThread(newHdr);
		newThread = TRUE;
	}
	else
	{
		newThread = FALSE;
	}
	return result;
}

nsresult nsMsgDatabase::AddToThread(nsMsgHdr *newHdr, nsIMsgThread *thread, nsIMsgDBHdr *inReplyTo, PRBool threadInThread)
{
	// don't worry about real threading yet.
	return thread->AddChild(newHdr, inReplyTo, threadInThread);
}

nsMsgHdr	*	nsMsgDatabase::GetMsgHdrForReference(nsString2 &reference)
{
	NS_ASSERTION(PR_FALSE, "not implemented yet.");
	return nsnull;
}

nsIMsgDBHdr *nsMsgDatabase::GetMsgHdrForMessageID(nsString2 &msgID)
{
	nsIMsgDBHdr	*msgHdr = nsnull;
    nsresult rv = NS_OK;
	mdbYarn	messageIdYarn;

	messageIdYarn.mYarn_Buf = (void*)msgID.GetBuffer();
	messageIdYarn.mYarn_Fill = PL_strlen(msgID.GetBuffer());
	messageIdYarn.mYarn_Form = 0;
	messageIdYarn.mYarn_Size = messageIdYarn.mYarn_Fill;

	nsIMdbRow	*hdrRow;
	mdbOid		outRowId;
	mdb_err result = GetStore()->FindRow(GetEnv(), m_hdrRowScopeToken,
		m_messageIdColumnToken, &messageIdYarn,  &outRowId, 
		&hdrRow);
	if (NS_SUCCEEDED(result) && hdrRow)
	{
		//Get key from row
		mdbOid outOid;
		nsMsgKey key=0;
		if (hdrRow->GetOid(GetEnv(), &outOid) == NS_OK)
			key = outOid.mOid_Id;
		rv = CreateMsgHdr(hdrRow, key, &msgHdr);
	}
	return msgHdr;
}

nsIMsgDBHdr *nsMsgDatabase::GetMsgHdrForSubject(nsString2 &subject)
{
	nsIMsgDBHdr	*msgHdr = nsnull;
    nsresult rv = NS_OK;
	mdbYarn	subjectYarn;

	subjectYarn.mYarn_Buf = (void*)subject.GetBuffer();
	subjectYarn.mYarn_Fill = PL_strlen(subject.GetBuffer());
	subjectYarn.mYarn_Form = 0;
	subjectYarn.mYarn_Size = subjectYarn.mYarn_Fill;

	nsIMdbRow	*hdrRow;
	mdbOid		outRowId;
	mdb_err result = GetStore()->FindRow(GetEnv(), m_hdrRowScopeToken,
		m_subjectColumnToken, &subjectYarn,  &outRowId, 
		&hdrRow);
	if (NS_SUCCEEDED(result) && hdrRow)
	{
		//Get key from row
		mdbOid outOid;
		nsMsgKey key=0;
		if (hdrRow->GetOid(GetEnv(), &outOid) == NS_OK)
			key = outOid.mOid_Id;
		rv = CreateMsgHdr(hdrRow, key, &msgHdr);
	}
	return msgHdr;
}

NS_IMETHODIMP nsMsgDatabase::GetThreadContainingMsgHdr(nsIMsgDBHdr *msgHdr, nsIMsgThread **result)
{
	if (!result)
		return NS_ERROR_NULL_POINTER;

	*result = nsnull;
	nsMsgKey threadId = nsMsgKey_None;
	(void)msgHdr->GetThreadId(&threadId);
	if (threadId != nsMsgKey_None)
		*result = GetThreadForThreadId(threadId);

	return (*result) ? NS_OK : NS_ERROR_FAILURE;
}


NS_IMETHODIMP nsMsgDatabase::GetThreadForMsgKey(nsMsgKey msgKey, nsIMsgThread **result)
{
	nsresult ret = NS_OK;
	if (!result)
		return NS_ERROR_NULL_POINTER;

	*result = nsnull;

	nsIMsgDBHdr *msg = NULL;
    ret = GetMsgHdrForKey(msgKey, &msg);

	if (NS_SUCCEEDED(ret) && msg)
	{
		ret = GetThreadContainingMsgHdr(msg, result);
		NS_RELEASE(msg);
	}

	return ret;
}

// caller needs to unrefer.
nsIMsgThread *	nsMsgDatabase::GetThreadForThreadId(nsMsgKey threadId)
{

	nsMsgThread		*pThread = nsnull;
	if (m_mdbStore)
	{
		mdbOid tableId;
		tableId.mOid_Id = threadId;
		tableId.mOid_Scope = m_hdrRowScopeToken;

		nsIMdbTable *threadTable;
		mdb_err res = m_mdbStore->GetTable(GetEnv(), &tableId, &threadTable);
		
		if (NS_SUCCEEDED(res) && threadTable)
		{
			pThread = new nsMsgThread(this, threadTable);
			if(pThread)
				NS_ADDREF(pThread);
		}
	}
	return pThread;
}

// make the passed in header a thread header
nsresult nsMsgDatabase::AddNewThread(nsMsgHdr *msgHdr)
{

	if (!msgHdr)
		return NS_ERROR_NULL_POINTER;

	nsMsgThread *threadHdr = nsnull;
	
	nsString2 subject(eOneByte);

	nsresult err = msgHdr->GetSubject(subject);

	err = CreateNewThread(msgHdr->m_messageKey, subject.GetBuffer(), &threadHdr);
	msgHdr->SetThreadId(msgHdr->m_messageKey);
	if (threadHdr)
	{
//		nsString2 subject(eOneByte);

		threadHdr->AddRef();
//		err = msgHdr->GetSubject(subject);
//		threadHdr->SetThreadKey(msgHdr->m_messageKey);
//		threadHdr->SetSubject(subject.GetBuffer());

		// need to add the thread table to the db.
		AddToThread(msgHdr, threadHdr, nsnull, PR_FALSE);

		threadHdr->Release();
	}
	return err;
}


// should we thread messages with common subjects that don't start with Re: together?
// I imagine we might have separate preferences for mail and news, so this is a virtual method.
PRBool	nsMsgDatabase::ThreadBySubjectWithoutRe()
{
	return PR_TRUE;
}

nsresult nsMsgDatabase::GetBoolPref(const char *prefName, PRBool *result)
{
	PRBool prefValue = PR_FALSE;
	nsresult rv;
	NS_WITH_SERVICE(nsIPref, prefs, kPrefCID, &rv); 
    if (NS_SUCCEEDED(rv) && prefs)
	{
		rv = prefs->GetBoolPref(prefName, &prefValue);
		*result = prefValue;
	}
	return rv;
}

nsresult nsMsgDatabase::ListAllThreads(nsMsgKeyArray *threadIds)
{
	nsresult		rv;
	nsMsgThread		*pThread;

    nsIEnumerator* threads;
    rv = EnumerateThreads(&threads);
    if (NS_FAILED(rv)) 
		return rv;
	for (threads->First(); threads->IsDone() != NS_OK; threads->Next()) 
	{
        rv = threads->CurrentItem((nsISupports**)&pThread);
        NS_ASSERTION(NS_SUCCEEDED(rv), "nsMsgDBEnumerator broken");
        if (NS_FAILED(rv)) 
			break;

		if (threadIds)
		{
            nsMsgKey key;
            (void)pThread->GetThreadKey(&key);
			threadIds->Add(key);
        }
//		NS_RELEASE(pThread);
		pThread = nsnull;
	}
	NS_RELEASE(threads);
	return rv;
}

NS_IMETHODIMP nsMsgDatabase::ListAllOfflineOpIds(nsMsgKeyArray *offlineOpIds)
{
	nsresult ret = NS_OK;
	if (!offlineOpIds)
		return NS_ERROR_NULL_POINTER;
	// technically, notimplemented, but no one's putting offline ops in anyway.
	return ret;
}

NS_IMETHODIMP nsMsgDatabase::ListAllOfflineDeletes(nsMsgKeyArray *offlineDeletes)
{
	nsresult ret = NS_OK;
	if (!offlineDeletes)
		return NS_ERROR_NULL_POINTER;

	// technically, notimplemented, but no one's putting offline ops in anyway.
	return ret;
}
NS_IMETHODIMP nsMsgDatabase::GetHighWaterArticleNum(nsMsgKey *key)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP nsMsgDatabase::GetLowWaterArticleNum(nsMsgKey *key)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


#ifdef DEBUG
nsresult nsMsgDatabase::DumpContents()
{
    nsMsgKey key;
    PRUint32 i;

#ifdef HAVE_INT_ENUMERATORS
    nsIEnumerator* keys;
	nsresult rv = EnumerateKeys(&keys);
    if (NS_FAILED(rv)) return rv;
    for (keys->First(); keys->IsDone != NS_OK; keys->Next()) {
        rv = keys->CurrentItem((nsISupports**)&key);
        NS_ASSERTION(NS_SUCCEEDED(rv), "nsMsgDBEnumerator broken");
        if (NS_FAILED(rv)) break;
#else
    nsMsgKeyArray keys;
    nsresult rv = ListAllKeys(keys);
    for (i = 0; i < keys.GetSize(); i++) {
        key = keys[i];
#endif /* HAVE_INT_ENUMERATORS */
		nsIMsgDBHdr *msg = NULL;
        rv = GetMsgHdrForKey(key, &msg);
        nsMsgHdr* msgHdr = NS_STATIC_CAST(nsMsgHdr*, msg);      // closed system, cast ok
		if (NS_SUCCEEDED(rv))
		{
            nsAutoString author;
            nsAutoString subject;

			msgHdr->GetMessageKey(&key);
			msgHdr->GetAuthor(author);
			msgHdr->GetSubject(subject);
			char *authorStr = author.ToNewCString();
			char *subjectStr = subject.ToNewCString();
			printf("hdr key = %u, author = %s subject = %s\n", key, (authorStr) ? authorStr : "", (subjectStr) ? subjectStr : "");
			delete [] authorStr;
			delete [] subjectStr;
			NS_RELEASE(msgHdr);
		}
    }
	nsMsgKeyArray threads;
    rv = ListAllThreads(&threads);
    for ( i = 0; i < threads.GetSize(); i++) 
	{
        key = threads[i];
		printf("thread key = %u\n", key);
//		DumpThread(key);
    }


    return NS_OK;
}

nsresult nsMsgDatabase::DumpMsgChildren(nsIMsgDBHdr *msgHdr)
{
	return NS_OK;
}

nsresult	nsMsgDatabase::DumpThread(nsMsgKey threadId)
{
	nsresult ret = NS_OK;
	nsIMsgThread	*thread = nsnull;

	thread = GetThreadForThreadId(threadId);
	if (thread)
	{
		nsIEnumerator *enumerator = nsnull;

		ret = thread->EnumerateMessages(nsMsgKey_None, &enumerator);
		if (NS_SUCCEEDED(ret) && enumerator)
		{
			for (enumerator->First(); enumerator->IsDone() != NS_OK; enumerator->Next()) 
			{
				nsIMsgDBHdr *pMessage = nsnull;
				ret = enumerator->CurrentItem((nsISupports**)&pMessage);
				NS_ASSERTION(NS_SUCCEEDED(ret), "nsMsgDBEnumerator broken");
				if (NS_FAILED(ret)) 
					break;

#ifdef DEBUG_bienvenu                    
				if (pMessage)
				{
					nsMsgKey key;
					nsString subject;
					(void)pMessage->GetMessageKey(&key);
					pMessage->GetSubject(subject);

					printf("message in thread %u %s\n", key, (const char *) nsAutoCString(subject));
				}
#endif /* DEBUG_bienvenu */
		//		NS_RELEASE(pMessage);
				pMessage = nsnull;
			}
			NS_RELEASE(enumerator);

		}
	}
	return ret;
}
#endif /* DEBUG */


