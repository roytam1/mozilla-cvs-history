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

// this file implements the nsAddrDatabase interface using the MDB Interface.

#include "nsAddrDatabase.h"
#include "nsIEnumerator.h"
#include "nsFileStream.h"
#include "nsString.h"
#include "nsAbBaseCID.h"
#include "nsIAbCard.h"
#include "nsAbCard.h"
#include "nsIAddrBookSession.h"

#include "prmem.h"
#include "prprf.h"

#include "nsIServiceManager.h"
#include "nsRDFCID.h"

#include "nsICollation.h"

#include "nsCollationCID.h"
#include "nsMorkCID.h"
#include "nsIPref.h"
#include "nsIMdbFactoryFactory.h"
#include "nsXPIDLString.h"

static NS_DEFINE_CID(kCMorkFactory, NS_MORK_CID);
static NS_DEFINE_CID(kAddrBookSessionCID, NS_ADDRBOOKSESSION_CID);

/* The definition is nsAddressBook.cpp */
extern const char *kCardDataSourceRoot;

/* The definition is nsAddrDatabase.cpp */
extern const char *kMainPersonalAddressBook;

#define ID_PAB_TABLE		1
#define ID_ANONYMOUS_TABLE	2

const PRInt32 kAddressBookDBVersion = 1;

const char *kAnonymousTableKind = "ns:addrbk:db:table:kind:anonymous";
const char *kAnonymousRowScope = "ns:addrbk:db:row:scope:anonymous:all";

const char *kPabTableKind = "ns:addrbk:db:table:kind:pab";
const char *kMailListTableKind = "ns:addrbk:db:table:kind:maillist";

const char *kCardRowScope = "ns:addrbk:db:row:scope:card:all";

const char *kFirstNameColumn = "FirstName";
const char *kLastNameColumn = "LastName";
const char *kDisplayNameColumn = "DisplayName";
const char *kNicknameColumn = "NickName";
const char *kPriEmailColumn = "PrimaryEmail";
const char *k2ndEmailColumn = "SecondEmail";
const char *kPlainTextColumn = "SendPlainText";
const char *kWorkPhoneColumn = "WorkPhone";
const char *kHomePhoneColumn = "HomePhone";
const char *kFaxColumn = "FaxNumber";
const char *kPagerColumn = "PagerNumber";
const char *kCellularColumn = "CellularNumber";
const char *kHomeAddressColumn = "HomeAddress";
const char *kHomeAddress2Column = "HomeAddress2";
const char *kHomeCityColumn = "HomeCity";
const char *kHomeStateColumn = "HomeState";
const char *kHomeZipCodeColumn = "HomeZipCode";
const char *kHomeCountryColumn = "HomeCountry";
const char *kWorkAddressColumn = "WorkAddress";
const char *kWorkAddress2Column = "WorkAddress2";
const char *kWorkCityColumn = "WorkCity";
const char *kWorkStateColumn = "WorkState";
const char *kWorkZipCodeColumn = "WorkZipCode";
const char *kWorkCountryColumn = "WorkCountry";
const char *kJobTitleColumn = "JobTitle";
const char *kDepartmentColumn = "Department";
const char *kCompanyColumn = "Company";
const char *kWebPage1Column = "WebPage1";
const char *kWebPage2Column = "WebPage2";
const char *kBirthYearColumn = "BirthYear";
const char *kBirthMonthColumn = "BirthMonth";
const char *kBirthDayColumn = "BirthDay";
const char *kCustom1Column = "Custom1";
const char *kCustom2Column = "Custom2";
const char *kCustom3Column = "Custom3";
const char *kCustom4Column = "Custom4";
const char *kNotesColumn = "Notes";
const char *kLastModifiedDateColumn = "LastModifiedDate";

const char *kAddressCharSetColumn = "AddrCharSet";


struct mdbOid gAddressBookTableOID;
struct mdbOid gMailListTableOID;
struct mdbOid gAnonymousTableOID;

nsAddrDatabase::nsAddrDatabase()
    : m_mdbEnv(nsnull), m_mdbStore(nsnull),
      m_mdbPabTable(nsnull), m_mdbRow(nsnull),
	  m_dbName(""), m_mdbTokensInitialized(PR_FALSE), 
	  m_ChangeListeners(nsnull), m_mdbAnonymousTable(nsnull), 
	  m_AnonymousTableKind(0), m_pAnonymousStrAttributes(nsnull), 
	  m_pAnonymousStrValues(nsnull), m_pAnonymousIntAttributes(nsnull),
	  m_pAnonymousIntValues(nsnull), m_pAnonymousBoolAttributes(nsnull),
	  m_pAnonymousBoolValues(nsnull),
      m_PabTableKind(0),
      m_MailListTableKind(0),
      m_CardRowScopeToken(0),
      m_FirstNameColumnToken(0),
      m_LastNameColumnToken(0),
      m_DisplayNameColumnToken(0),
      m_NickNameColumnToken(0),
      m_PriEmailColumnToken(0),
      m_2ndEmailColumnToken(0),
      m_WorkPhoneColumnToken(0),
      m_HomePhoneColumnToken(0),
      m_FaxColumnToken(0),
      m_PagerColumnToken(0),
      m_CellularColumnToken(0),
      m_HomeAddressColumnToken(0),
      m_HomeAddress2ColumnToken(0),
      m_HomeCityColumnToken(0),
      m_HomeStateColumnToken(0),
      m_HomeZipCodeColumnToken(0),
      m_HomeCountryColumnToken(0),
      m_WorkAddressColumnToken(0),
      m_WorkAddress2ColumnToken(0),
      m_WorkCityColumnToken(0),
      m_WorkStateColumnToken(0),
      m_WorkZipCodeColumnToken(0),
      m_WorkCountryColumnToken(0),
      m_WebPage1ColumnToken(0),
      m_WebPage2ColumnToken(0),
      m_BirthYearColumnToken(0),
      m_BirthMonthColumnToken(0),
      m_BirthDayColumnToken(0),
      m_Custom1ColumnToken(0),
      m_Custom2ColumnToken(0),
      m_Custom3ColumnToken(0),
      m_Custom4ColumnToken(0),
      m_NotesColumnToken(0),
	  m_LastModDateColumnToken(0),
      m_PlainTextColumnToken(0),
      m_AddressCharSetColumnToken(0),
	  m_dbDirectory(nsnull)
{
	NS_INIT_REFCNT();
}

nsAddrDatabase::~nsAddrDatabase()
{
//	Close(FALSE);	// better have already been closed.
    if (m_ChangeListeners) 
	{
        // better not be any listeners, because we're going away.
        NS_ASSERTION(m_ChangeListeners->Count() == 0, "shouldn't have any listeners");
        delete m_ChangeListeners;
    }

	CleanupCache();

	if (m_pAnonymousStrAttributes)
		RemoveAnonymousList(m_pAnonymousStrAttributes);
	if (m_pAnonymousIntAttributes)
		RemoveAnonymousList(m_pAnonymousIntAttributes);
	if (m_pAnonymousBoolAttributes)
		RemoveAnonymousList(m_pAnonymousBoolAttributes);

	if (m_pAnonymousStrValues)
		RemoveAnonymousList(m_pAnonymousStrValues);
	if (m_pAnonymousIntValues)
		RemoveAnonymousList(m_pAnonymousIntValues);
	if (m_pAnonymousBoolValues)
		RemoveAnonymousList(m_pAnonymousBoolValues);
}

nsresult nsAddrDatabase::RemoveAnonymousList(nsVoidArray* pArray)
{
	if (pArray)
	{
		PRUint32 count = pArray->Count();
		for (int i = count - 1; i >= 0; i--)
		{
			void* pPtr = pArray->ElementAt(i);
			PR_FREEIF(pPtr);
			pArray->RemoveElementAt(i);
		}
		delete pArray;
	}
	return NS_OK;
}

NS_IMPL_ADDREF(nsAddrDatabase)
NS_IMPL_RELEASE(nsAddrDatabase)

NS_IMETHODIMP nsAddrDatabase::QueryInterface(REFNSIID aIID, void** aResult)
{   
    if (aResult == NULL)  
        return NS_ERROR_NULL_POINTER;  

    if (aIID.Equals(nsCOMTypeInfo<nsIAddrDatabase>::GetIID()) ||
        aIID.Equals(nsCOMTypeInfo<nsIAddrDBAnnouncer>::GetIID()) ||
        aIID.Equals(nsCOMTypeInfo<nsISupports>::GetIID())) {
        *aResult = NS_STATIC_CAST(nsIAddrDatabase*, this);   
        NS_ADDREF_THIS();
        return NS_OK;
    }
    return NS_NOINTERFACE;
}   

NS_IMETHODIMP nsAddrDatabase::AddListener(nsIAddrDBListener *listener)
{
    if (m_ChangeListeners == nsnull) 
	{
        m_ChangeListeners = new nsVoidArray();
        if (!m_ChangeListeners) 
			return NS_ERROR_OUT_OF_MEMORY;
    }
	return m_ChangeListeners->AppendElement(listener);
}

NS_IMETHODIMP nsAddrDatabase::RemoveListener(nsIAddrDBListener *listener)
{
    if (m_ChangeListeners == nsnull) 
		return NS_OK;

	PRInt32 count = m_ChangeListeners->Count();
	PRInt32 i;
	for (i = 0; i < count; i++)
	{
		nsIAddrDBListener *dbListener = (nsIAddrDBListener *)m_ChangeListeners->ElementAt(i);
		if (dbListener == listener)
		{
			m_ChangeListeners->RemoveElementAt(i);
			return NS_OK;
		}
	}
	return NS_ERROR_FAILURE;
}

NS_IMETHODIMP nsAddrDatabase::NotifyCardAttribChange(PRUint32 abCode, nsIAddrDBListener *instigator)
{
    if (m_ChangeListeners == nsnull)
		return NS_OK;
	PRInt32 i;
	for (i = 0; i < m_ChangeListeners->Count(); i++)
	{
		nsIAddrDBListener *changeListener =
            (nsIAddrDBListener *) m_ChangeListeners->ElementAt(i);

		nsresult rv = changeListener->OnCardAttribChange(abCode, instigator); 
        if (NS_FAILED(rv)) 
			return rv;
	}
    return NS_OK;
}

NS_IMETHODIMP nsAddrDatabase::NotifyCardEntryChange(PRUint32 abCode, nsIAbCard *card, nsIAddrDBListener *instigator)
{
    if (m_ChangeListeners == nsnull)
		return NS_OK;
	PRInt32 i;
	PRInt32 count = m_ChangeListeners->Count();
	for (i = 0; i < count; i++)
	{
		nsIAddrDBListener *changeListener = 
            (nsIAddrDBListener *) m_ChangeListeners->ElementAt(i);

		nsresult rv = changeListener->OnCardEntryChange(abCode, card, instigator); 
        if (NS_FAILED(rv)) return rv;
	}
    return NS_OK;
}

NS_IMETHODIMP nsAddrDatabase::NotifyAnnouncerGoingAway(void)
{
    if (m_ChangeListeners == nsnull)
		return NS_OK;
	// run loop backwards because listeners remove themselves from the list 
	// on this notification
	PRInt32 i;
	for (i = m_ChangeListeners->Count() - 1; i >= 0 ; i--)
	{
		nsIAddrDBListener *changeListener =
            (nsIAddrDBListener *) m_ChangeListeners->ElementAt(i);

		nsresult rv = changeListener->OnAnnouncerGoingAway(this); 
        if (NS_FAILED(rv)) 
			return rv;
	}
    return NS_OK;
}



nsVoidArray *nsAddrDatabase::m_dbCache = NULL;

//----------------------------------------------------------------------
// GetDBCache
//----------------------------------------------------------------------

nsVoidArray/*<nsAddrDatabase>*/*
nsAddrDatabase::GetDBCache()
{
	if (!m_dbCache)
		m_dbCache = new nsVoidArray();

	return m_dbCache;
	
}

void
nsAddrDatabase::CleanupCache()
{
	if (m_dbCache) // clean up memory leak
	{
		PRInt32 i;
		for (i = 0; i < GetDBCache()->Count(); i++)
		{
			nsAddrDatabase* pAddrDB = NS_STATIC_CAST(nsAddrDatabase*, GetDBCache()->ElementAt(i));
			if (pAddrDB)
			{
				pAddrDB->ForceClosed();
				i--;	// back up array index, since closing removes db from cache.
			}
		}
//		NS_ASSERTION(GetNumInCache() == 0, "some msg dbs left open");	// better not be any open db's.
		delete m_dbCache;
	}
	m_dbCache = nsnull; // Need to reset to NULL since it's a
			  // static global ptr and maybe referenced 
			  // again in other places.
}

//----------------------------------------------------------------------
// FindInCache - this addrefs the db it finds.
//----------------------------------------------------------------------
nsAddrDatabase* nsAddrDatabase::FindInCache(nsFileSpec *dbName)
{
	PRInt32 i;
	for (i = 0; i < GetDBCache()->Count(); i++)
	{
		nsAddrDatabase* pAddrDB = NS_STATIC_CAST(nsAddrDatabase*, GetDBCache()->ElementAt(i));
		if (pAddrDB->MatchDbName(dbName))
		{
			NS_ADDREF(pAddrDB);
			return pAddrDB;
		}
	}
	return nsnull;
}

//----------------------------------------------------------------------
// FindInCache
//----------------------------------------------------------------------
PRInt32 nsAddrDatabase::FindInCache(nsAddrDatabase* pAddrDB)
{
	PRInt32 i;
	for (i = 0; i < GetDBCache()->Count(); i++)
	{
		if (GetDBCache()->ElementAt(i) == pAddrDB)
		{
			return(i);
		}
	}
	return(-1);
}

PRBool nsAddrDatabase::MatchDbName(nsFileSpec* dbName)	// returns PR_TRUE if they match
{
	return (m_dbName == (*dbName)); 
}

//----------------------------------------------------------------------
// RemoveFromCache
//----------------------------------------------------------------------
void nsAddrDatabase::RemoveFromCache(nsAddrDatabase* pAddrDB)
{
	PRInt32 i = FindInCache(pAddrDB);
	if (i != -1)
	{
		GetDBCache()->RemoveElementAt(i);
	}
}

nsIMdbFactory *nsAddrDatabase::GetMDBFactory()
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
void nsAddrDatabase::UnixToNative(char*& ioPath)
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
void nsAddrDatabase::UnixToNative(char*& ioPath)
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
		else if (PL_strchr(src, '/'))	// * partial path, and not just a leaf name
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

void nsAddrDatabase::NativeToUnix(char*& ioPath)
// This just does string manipulation.  It doesn't check reality, or canonify, or
// anything
//----------------------------------------------------------------------------------------
{
	size_t len = PL_strlen(ioPath);
	char* result = new char[len + 2]; // ... but allow for the initial colon in a partial name
	if (result)
	{
		char* dst = result;
		const char* src = ioPath;
		if (*src == ':')		 	// * partial path, and not just a leaf name
			src++;
		else if (PL_strchr(src, ':'))	// * full path
			*dst++ = '/';
		PL_strcpy(dst, src);

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

/* caller need to delete *aDbPath */
NS_IMETHODIMP nsAddrDatabase::GetDbPath(nsFileSpec * *aDbPath)
{
	if (aDbPath)
	{
		nsFileSpec* pFilePath = new nsFileSpec();
		*pFilePath = m_dbName;
		*aDbPath = pFilePath;
		return NS_OK;
	}
	else
		return NS_ERROR_NULL_POINTER;
}

NS_IMETHODIMP nsAddrDatabase::SetDbPath(nsFileSpec * aDbPath)
{
	m_dbName = (*aDbPath);
	return NS_OK;
}

NS_IMETHODIMP nsAddrDatabase::Open
(nsFileSpec* pabName, PRBool create, nsIAddrDatabase** pAddrDB, PRBool upgrading)
{
	nsAddrDatabase	        *pAddressBookDB;
	nsresult                err = NS_OK;

	*pAddrDB = nsnull;

	pAddressBookDB = (nsAddrDatabase *) FindInCache(pabName);
	if (pAddressBookDB) {
		*pAddrDB = pAddressBookDB;
		return(NS_OK);
	}

	pAddressBookDB = new nsAddrDatabase();
	if (!pAddressBookDB) {
		return NS_ERROR_OUT_OF_MEMORY;
	}

	pAddressBookDB->AddRef();

	err = pAddressBookDB->OpenMDB(pabName, create);
	if (NS_SUCCEEDED(err)) 
	{
		pAddressBookDB->SetDbPath(pabName);
		*pAddrDB = pAddressBookDB;
		if (pAddressBookDB)  
			GetDBCache()->AppendElement(pAddressBookDB);
		NS_IF_ADDREF(*pAddrDB);
	}
	else 
	{
		*pAddrDB = nsnull;
		if (pAddressBookDB)  
			delete pAddressBookDB;
		pAddressBookDB = nsnull;
	}

	return err;
}

// Open the MDB database synchronously. If successful, this routine
// will set up the m_mdbStore and m_mdbEnv of the database object 
// so other database calls can work.
NS_IMETHODIMP nsAddrDatabase::OpenMDB(nsFileSpec *dbName, PRBool create)
{
	nsresult ret = NS_OK;
	nsIMdbFactory *myMDBFactory = GetMDBFactory();
	if (myMDBFactory)
	{
		ret = myMDBFactory->MakeEnv(NULL, &m_mdbEnv);
		if (NS_SUCCEEDED(ret))
		{
			nsIMdbThumb *thumb;
			const char *pFilename = dbName->GetCString(); /* do not free */
			char	*nativeFileName = PL_strdup(pFilename);
			nsIMdbHeap* dbHeap = 0;
			mdb_bool dbFrozen = mdbBool_kFalse; // not readonly, we want modifiable

			if (!nativeFileName)
				return NS_ERROR_OUT_OF_MEMORY;

			if (m_mdbEnv)
				m_mdbEnv->SetAutoClear(PR_TRUE);

#if defined(XP_PC) || defined(XP_MAC)
			UnixToNative(nativeFileName);
#endif
			if (!dbName->Exists()) 
				ret = NS_ERROR_FAILURE;  // check: use the right error code later
			else
			{
				mdbOpenPolicy inOpenPolicy;
				mdb_bool	canOpen;
				mdbYarn		outFormatVersion;
				// char		bufFirst512Bytes[512];
				// mdbYarn		first512Bytes;

				// first512Bytes.mYarn_Buf = bufFirst512Bytes;
				// first512Bytes.mYarn_Size = 512;
				// first512Bytes.mYarn_Fill = 512;
				// first512Bytes.mYarn_Form = 0;	// what to do with this? we're storing csid in the msg hdr...

				// {
				// 	nsFileSpec ioStream(dbName->GetCString());
				// 	nsIOFileStream *dbStream = new nsIOFileStream(ioStream);
				// 	if (dbStream) {
				// 		PRInt32 bytesRead = dbStream->read(bufFirst512Bytes, sizeof(bufFirst512Bytes));
				// 		first512Bytes.mYarn_Fill = bytesRead;
				// 		dbStream->close();
				// 		delete dbStream;
				// 	}
				// 	else {
				// 		PR_FREEIF(nativeFileName);
				// 		return NS_ERROR_OUT_OF_MEMORY;
				// 	}
				// }
				
				nsIMdbFile* oldFile = 0;
				ret = myMDBFactory->OpenOldFile(m_mdbEnv, dbHeap, nativeFileName,
					 dbFrozen, &oldFile);
				if ( oldFile )
				{
					if ( ret == NS_OK )
					{
						ret = myMDBFactory->CanOpenFilePort(m_mdbEnv, oldFile, // the file to investigate
							&canOpen, &outFormatVersion);
						if (ret == 0 && canOpen)
						{
							inOpenPolicy.mOpenPolicy_ScopePlan.mScopeStringSet_Count = 0;
							inOpenPolicy.mOpenPolicy_MinMemory = 0;
							inOpenPolicy.mOpenPolicy_MaxLazy = 0;

							ret = myMDBFactory->OpenFileStore(m_mdbEnv, dbHeap,
								oldFile, &inOpenPolicy, &thumb); 
						}
						else
							ret = NS_ERROR_FAILURE;  //check: use the right error code
					}
					oldFile->CutStrongRef(m_mdbEnv); // always release our file ref, store has own
				}
			}

			PR_FREEIF(nativeFileName);

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
					{ 
						outDone = PR_TRUE;
						break;
					}
				}
				while (NS_SUCCEEDED(ret) && !outBroken && !outDone);
				if (NS_SUCCEEDED(ret) && outDone)
				{
					ret = myMDBFactory->ThumbToOpenStore(m_mdbEnv, thumb, &m_mdbStore);
					if (ret == NS_OK && m_mdbStore)
					{
						ret = InitExistingDB();
						create = PR_FALSE;
					}
				}
			}
			else if (create)	// ### need error code saying why open file store failed
			{
				nsIMdbFile* newFile = 0;
				ret = myMDBFactory->CreateNewFile(m_mdbEnv, dbHeap, dbName->GetCString(), &newFile);
				if ( newFile )
				{
					if (ret == NS_OK)
					{
						mdbOpenPolicy inOpenPolicy;

						inOpenPolicy.mOpenPolicy_ScopePlan.mScopeStringSet_Count = 0;
						inOpenPolicy.mOpenPolicy_MinMemory = 0;
						inOpenPolicy.mOpenPolicy_MaxLazy = 0;

						ret = myMDBFactory->CreateNewFileStore(m_mdbEnv, dbHeap,
							newFile, &inOpenPolicy, &m_mdbStore);
						if (ret == NS_OK)
							ret = InitNewDB();
					}
					newFile->CutStrongRef(m_mdbEnv); // always release our file ref, store has own
				}
			}
		}
	}
	return ret;
}

NS_IMETHODIMP nsAddrDatabase::CloseMDB(PRBool commit)
{
	if (commit)
		Commit(kSessionCommit);
	return NS_OK;
}

NS_IMETHODIMP nsAddrDatabase::OpenAnonymousDB(nsIAddrDatabase **pCardDB)
{
	nsresult rv = NS_OK;
    nsCOMPtr<nsIAddrDatabase> database;

	NS_WITH_SERVICE(nsIAddrBookSession, abSession, kAddrBookSessionCID, &rv); 
	if (NS_SUCCEEDED(rv))
	{
		nsFileSpec* dbPath;
		abSession->GetUserProfileDirectory(&dbPath);

		(*dbPath) += kMainPersonalAddressBook;

		Open(dbPath, PR_TRUE, getter_AddRefs(database), PR_TRUE);

        *pCardDB = database;
		NS_IF_ADDREF(*pCardDB);
	}
	return rv;
}

NS_IMETHODIMP nsAddrDatabase::CloseAnonymousDB(PRBool forceCommit)
{
	return CloseMDB(forceCommit);
}

// force the database to close - this'll flush out anybody holding onto
// a database without having a listener!
// This is evil in the com world, but there are times we need to delete the file.
NS_IMETHODIMP nsAddrDatabase::ForceClosed()
{
	nsresult	err = NS_OK;
    nsCOMPtr<nsIAddrDatabase> aDb(do_QueryInterface(this, &err));

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

NS_IMETHODIMP nsAddrDatabase::Commit(PRUint32 commitType)
{
	nsresult	err = NS_OK;
	nsIMdbThumb	*commitThumb = NULL;

	if (m_mdbStore)
	{
		switch (commitType)
		{
		case kSmallCommit:
			err = m_mdbStore->SmallCommit(GetEnv());
			break;
		case kLargeCommit:
			err = m_mdbStore->LargeCommit(GetEnv(), &commitThumb);
			break;
		case kSessionCommit:
			// comment out until persistence works.
			err = m_mdbStore->SessionCommit(GetEnv(), &commitThumb);
			break;
		case kCompressCommit:
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

NS_IMETHODIMP nsAddrDatabase::Close(PRBool forceCommit /* = TRUE */)
{
	return CloseMDB(forceCommit);
}

// set up empty tablesetc.
nsresult nsAddrDatabase::InitNewDB()
{
	nsresult err = NS_OK;

	err = InitMDBInfo();
	if (NS_SUCCEEDED(err))
	{
		err = InitPabTable();
	}
	return err;
}

nsresult nsAddrDatabase::InitPabTable()
{
	nsIMdbStore *store = GetStore();

    mdb_err mdberr = (nsresult) store->NewTableWithOid(GetEnv(), &gAddressBookTableOID, 
		m_PabTableKind, PR_FALSE, (const mdbOid*)nsnull, &m_mdbPabTable);

	return mdberr;
}

nsresult nsAddrDatabase::InitAnonymousTable()
{
	nsIMdbStore *store = GetStore();

	nsresult err = store->StringToToken(GetEnv(), kAnonymousTableKind, &m_AnonymousTableKind); 
    err = (nsresult) store->NewTableWithOid(GetEnv(), &gAnonymousTableOID, 
		m_AnonymousTableKind, PR_FALSE, (const mdbOid*)nsnull, &m_mdbAnonymousTable);
	return err;
}

nsresult nsAddrDatabase::InitExistingDB()
{
	nsresult err = NS_OK;

	err = InitMDBInfo();
	if (err == NS_OK)
	{
		err = GetStore()->GetTable(GetEnv(), &gAddressBookTableOID, &m_mdbPabTable);

		err = GetStore()->StringToToken(GetEnv(), kAnonymousTableKind, &m_AnonymousTableKind); 
		err = GetStore()->GetTable(GetEnv(), &gAnonymousTableOID, &m_mdbAnonymousTable);
	}
	return err;
}

// initialize the various tokens and tables in our db's env
nsresult nsAddrDatabase::InitMDBInfo()
{
	nsresult err = NS_OK;

	if (!m_mdbTokensInitialized && GetStore())
	{
		m_mdbTokensInitialized = PR_TRUE;
		err	= GetStore()->StringToToken(GetEnv(), kCardRowScope, &m_CardRowScopeToken); 
		gAddressBookTableOID.mOid_Scope = m_CardRowScopeToken;
		gAddressBookTableOID.mOid_Id = ID_PAB_TABLE;
		gAnonymousTableOID.mOid_Scope = m_CardRowScopeToken;
		gAnonymousTableOID.mOid_Id = ID_ANONYMOUS_TABLE;
		if (NS_SUCCEEDED(err))
		{
			GetStore()->StringToToken(GetEnv(),  kFirstNameColumn, &m_FirstNameColumnToken);
			GetStore()->StringToToken(GetEnv(),  kLastNameColumn, &m_LastNameColumnToken);
			GetStore()->StringToToken(GetEnv(),  kDisplayNameColumn, &m_DisplayNameColumnToken);
			GetStore()->StringToToken(GetEnv(),  kNicknameColumn, &m_NickNameColumnToken);
			GetStore()->StringToToken(GetEnv(),  kPriEmailColumn, &m_PriEmailColumnToken);
			GetStore()->StringToToken(GetEnv(),  k2ndEmailColumn, &m_2ndEmailColumnToken);
			GetStore()->StringToToken(GetEnv(),  kWorkPhoneColumn, &m_WorkPhoneColumnToken);
			GetStore()->StringToToken(GetEnv(),  kHomePhoneColumn, &m_HomePhoneColumnToken);
			GetStore()->StringToToken(GetEnv(),  kFaxColumn, &m_FaxColumnToken);
			GetStore()->StringToToken(GetEnv(),  kPagerColumn, &m_PagerColumnToken);
			GetStore()->StringToToken(GetEnv(),  kCellularColumn, &m_CellularColumnToken);
			GetStore()->StringToToken(GetEnv(),  kHomeAddressColumn, &m_HomeAddressColumnToken);
			GetStore()->StringToToken(GetEnv(),  kHomeAddress2Column, &m_HomeAddress2ColumnToken);
			GetStore()->StringToToken(GetEnv(),  kHomeCityColumn, &m_HomeCityColumnToken);
			GetStore()->StringToToken(GetEnv(),  kHomeStateColumn, &m_HomeStateColumnToken);
			GetStore()->StringToToken(GetEnv(),  kHomeZipCodeColumn, &m_HomeZipCodeColumnToken);
			GetStore()->StringToToken(GetEnv(),  kHomeCountryColumn, &m_HomeCountryColumnToken);
			GetStore()->StringToToken(GetEnv(),  kWorkAddressColumn, &m_WorkAddressColumnToken);
			GetStore()->StringToToken(GetEnv(),  kWorkAddress2Column, &m_WorkAddress2ColumnToken);
			GetStore()->StringToToken(GetEnv(),  kWorkCityColumn, &m_WorkCityColumnToken);
			GetStore()->StringToToken(GetEnv(),  kWorkStateColumn, &m_WorkStateColumnToken);
			GetStore()->StringToToken(GetEnv(),  kWorkZipCodeColumn, &m_WorkZipCodeColumnToken);
			GetStore()->StringToToken(GetEnv(),  kWorkCountryColumn, &m_WorkCountryColumnToken);
			GetStore()->StringToToken(GetEnv(),  kJobTitleColumn, &m_JobTitleColumnToken);
			GetStore()->StringToToken(GetEnv(),  kDepartmentColumn, &m_DepartmentColumnToken);
			GetStore()->StringToToken(GetEnv(),  kCompanyColumn, &m_CompanyColumnToken);
			GetStore()->StringToToken(GetEnv(),  kWebPage1Column, &m_WebPage1ColumnToken);
			GetStore()->StringToToken(GetEnv(),  kWebPage2Column, &m_WebPage2ColumnToken);
			GetStore()->StringToToken(GetEnv(),  kBirthYearColumn, &m_BirthYearColumnToken);
			GetStore()->StringToToken(GetEnv(),  kBirthMonthColumn, &m_BirthMonthColumnToken);
			GetStore()->StringToToken(GetEnv(),  kBirthDayColumn, &m_BirthDayColumnToken);
			GetStore()->StringToToken(GetEnv(),  kCustom1Column, &m_Custom1ColumnToken);
			GetStore()->StringToToken(GetEnv(),  kCustom2Column, &m_Custom2ColumnToken);
			GetStore()->StringToToken(GetEnv(),  kCustom3Column, &m_Custom3ColumnToken);
			GetStore()->StringToToken(GetEnv(),  kCustom4Column, &m_Custom4ColumnToken);
			GetStore()->StringToToken(GetEnv(),  kNotesColumn, &m_NotesColumnToken);
			GetStore()->StringToToken(GetEnv(),  kLastModifiedDateColumn, &m_LastModDateColumnToken);

			GetStore()->StringToToken(GetEnv(),  kAddressCharSetColumn, &m_AddressCharSetColumnToken);
			err = GetStore()->StringToToken(GetEnv(), kPabTableKind, &m_PabTableKind); 
		}
	}
	return err;
}

////////////////////////////////////////////////////////////////////////////////

nsresult nsAddrDatabase::AddAttributeColumnsToRow(nsIAbCard *card, nsIMdbRow *cardRow)
{
	nsresult	err = NS_OK;

	if (!card && !cardRow )
		return NS_ERROR_NULL_POINTER;

	mdbOid rowOid, tableOid;
	m_mdbPabTable->GetOid(GetEnv(), &tableOid);
	cardRow->GetOid(GetEnv(), &rowOid);

	card->SetDbTableID(tableOid.mOid_Id);
	card->SetDbRowID(rowOid.mOid_Id);

	// add the row to the singleton table.
	if (NS_SUCCEEDED(err) && cardRow)
	{
		PRUnichar* pUnicodeStr = nsnull;
		PRInt32 unicharLength = 0;
		char* pUTF8Str = nsnull;
		card->GetFirstName(&pUnicodeStr);
		unicharLength = nsCRT::strlen(pUnicodeStr);
		if (pUnicodeStr && unicharLength)
		{
			INTL_ConvertFromUnicode(pUnicodeStr, unicharLength, (char**)&pUTF8Str);
			if (pUTF8Str)
			{
				AddFirstName(cardRow, pUTF8Str);
				PR_FREEIF(pUTF8Str);
			}
			PR_FREEIF(pUnicodeStr);
		}
		card->GetLastName(&pUnicodeStr);
		unicharLength = nsCRT::strlen(pUnicodeStr);
		if (pUnicodeStr && unicharLength)
		{
			INTL_ConvertFromUnicode(pUnicodeStr, unicharLength, (char**)&pUTF8Str);
			if (pUTF8Str)
			{
				AddLastName(cardRow, pUTF8Str);
				PR_FREEIF(pUTF8Str);
			}
			PR_FREEIF(pUnicodeStr);
		}
		card->GetDisplayName(&pUnicodeStr);
		unicharLength = nsCRT::strlen(pUnicodeStr);
		if (pUnicodeStr && unicharLength)
		{
			INTL_ConvertFromUnicode(pUnicodeStr, unicharLength, (char**)&pUTF8Str);
			if (pUTF8Str)
			{
				AddDisplayName(cardRow, pUTF8Str);
				PR_FREEIF(pUTF8Str);
			}
			PR_FREEIF(pUnicodeStr);
		}
		card->GetNickName(&pUnicodeStr);
		unicharLength = nsCRT::strlen(pUnicodeStr);
		if (pUnicodeStr && unicharLength)
		{
			INTL_ConvertFromUnicode(pUnicodeStr, unicharLength, (char**)&pUTF8Str);
			if (pUTF8Str)
			{
				AddNickName(cardRow, pUTF8Str);
				PR_FREEIF(pUTF8Str);
			}
			PR_FREEIF(pUnicodeStr);
		}
		card->GetPrimaryEmail(&pUnicodeStr);
		unicharLength = nsCRT::strlen(pUnicodeStr);
		if (pUnicodeStr && unicharLength)
		{
			INTL_ConvertFromUnicode(pUnicodeStr, unicharLength, (char**)&pUTF8Str);
			if (pUTF8Str)
			{
				AddPrimaryEmail(cardRow, pUTF8Str);
				PR_FREEIF(pUTF8Str);
			}
			PR_FREEIF(pUnicodeStr);
		}
		card->GetSecondEmail(&pUnicodeStr);
		unicharLength = nsCRT::strlen(pUnicodeStr);
		if (pUnicodeStr && unicharLength)
		{
			INTL_ConvertFromUnicode(pUnicodeStr, unicharLength, (char**)&pUTF8Str);
			if (pUTF8Str)
			{
				Add2ndEmail(cardRow, pUTF8Str);
				PR_FREEIF(pUTF8Str);
			}
			PR_FREEIF(pUnicodeStr);
		}
		card->GetWorkPhone(&pUnicodeStr);
		unicharLength = nsCRT::strlen(pUnicodeStr);
		if (pUnicodeStr && unicharLength)
		{
			INTL_ConvertFromUnicode(pUnicodeStr, unicharLength, (char**)&pUTF8Str);
			if (pUTF8Str)
			{
				AddWorkPhone(cardRow, pUTF8Str);
				PR_FREEIF(pUTF8Str);
			}
			PR_FREEIF(pUnicodeStr);
		}
		card->GetHomePhone(&pUnicodeStr);
		unicharLength = nsCRT::strlen(pUnicodeStr);
		if (pUnicodeStr && unicharLength)
		{
			INTL_ConvertFromUnicode(pUnicodeStr, unicharLength, (char**)&pUTF8Str);
			if (pUTF8Str)
			{
				AddHomePhone(cardRow, pUTF8Str);
				PR_FREEIF(pUTF8Str);
			}
			PR_FREEIF(pUnicodeStr);
		}
		card->GetFaxNumber(&pUnicodeStr);
		unicharLength = nsCRT::strlen(pUnicodeStr);
		if (pUnicodeStr && unicharLength)
		{
			INTL_ConvertFromUnicode(pUnicodeStr, unicharLength, (char**)&pUTF8Str);
			if (pUTF8Str)
			{
				AddFaxNumber(cardRow, pUTF8Str);
				PR_FREEIF(pUTF8Str);
			}
			PR_FREEIF(pUnicodeStr);
		}
		card->GetPagerNumber(&pUnicodeStr);
		unicharLength = nsCRT::strlen(pUnicodeStr);
		if (pUnicodeStr && unicharLength)
		{
			INTL_ConvertFromUnicode(pUnicodeStr, unicharLength, (char**)&pUTF8Str);
			if (pUTF8Str)
			{
				AddPagerNumber(cardRow,pUTF8Str);
				PR_FREEIF(pUTF8Str);
			}
			PR_FREEIF(pUnicodeStr);
		}
		card->GetCellularNumber(&pUnicodeStr);
		unicharLength = nsCRT::strlen(pUnicodeStr);
		if (pUnicodeStr && unicharLength)
		{
			INTL_ConvertFromUnicode(pUnicodeStr, unicharLength, (char**)&pUTF8Str);
			if (pUTF8Str)
			{
				AddCellularNumber(cardRow,pUTF8Str);
				PR_FREEIF(pUTF8Str);
			}
			PR_FREEIF(pUnicodeStr);
		}
		card->GetHomeAddress(&pUnicodeStr);
		unicharLength = nsCRT::strlen(pUnicodeStr);
		if (pUnicodeStr && unicharLength)
		{
			INTL_ConvertFromUnicode(pUnicodeStr, unicharLength, (char**)&pUTF8Str);
			if (pUTF8Str)
			{
				AddHomeAddress(cardRow, pUTF8Str);
				PR_FREEIF(pUTF8Str);
			}
			PR_FREEIF(pUnicodeStr);
		}
		card->GetHomeAddress2(&pUnicodeStr); 
		unicharLength = nsCRT::strlen(pUnicodeStr);
		if (pUnicodeStr && unicharLength)
		{
			INTL_ConvertFromUnicode(pUnicodeStr, unicharLength, (char**)&pUTF8Str);
			if (pUTF8Str)
			{
				AddHomeAddress2(cardRow, pUTF8Str);
				PR_FREEIF(pUTF8Str);
			}
			PR_FREEIF(pUnicodeStr);
		}
		card->GetHomeCity(&pUnicodeStr); 
		unicharLength = nsCRT::strlen(pUnicodeStr);
		if (pUnicodeStr && unicharLength)
		{
			INTL_ConvertFromUnicode(pUnicodeStr, unicharLength, (char**)&pUTF8Str);
			if (pUTF8Str)
			{
				AddHomeCity(cardRow, pUTF8Str);
				PR_FREEIF(pUTF8Str);
			}
			PR_FREEIF(pUnicodeStr);
		}
		card->GetHomeState(&pUnicodeStr); 
		unicharLength = nsCRT::strlen(pUnicodeStr);
		if (pUnicodeStr && unicharLength)
		{
			INTL_ConvertFromUnicode(pUnicodeStr, unicharLength, (char**)&pUTF8Str);
			if (pUTF8Str)
			{
				AddHomeState(cardRow, pUTF8Str);
				PR_FREEIF(pUTF8Str);
			}
			PR_FREEIF(pUnicodeStr);
		}
		card->GetHomeZipCode(&pUnicodeStr); 
		unicharLength = nsCRT::strlen(pUnicodeStr);
		if (pUnicodeStr && unicharLength)
		{
			INTL_ConvertFromUnicode(pUnicodeStr, unicharLength, (char**)&pUTF8Str);
			if (pUTF8Str)
			{
				AddHomeZipCode(cardRow, pUTF8Str);
				PR_FREEIF(pUTF8Str);
			}
			PR_FREEIF(pUnicodeStr);
		}
		card->GetHomeCountry(&pUnicodeStr); 
		unicharLength = nsCRT::strlen(pUnicodeStr);
		if (pUnicodeStr && unicharLength)
		{
			INTL_ConvertFromUnicode(pUnicodeStr, unicharLength, (char**)&pUTF8Str);
			if (pUTF8Str)
			{
				AddHomeCountry(cardRow, pUTF8Str);
				PR_FREEIF(pUTF8Str);
			}
			PR_FREEIF(pUnicodeStr);
		}
		card->GetWorkAddress(&pUnicodeStr);  
		unicharLength = nsCRT::strlen(pUnicodeStr);
		if (pUnicodeStr && unicharLength)
		{
			INTL_ConvertFromUnicode(pUnicodeStr, unicharLength, (char**)&pUTF8Str);
			if (pUTF8Str)
			{
				AddWorkAddress(cardRow, pUTF8Str);
				PR_FREEIF(pUTF8Str);
			}
			PR_FREEIF(pUnicodeStr);
		}
		card->GetWorkAddress2(&pUnicodeStr); 
		unicharLength = nsCRT::strlen(pUnicodeStr);
		if (pUnicodeStr && unicharLength)
		{
			INTL_ConvertFromUnicode(pUnicodeStr, unicharLength, (char**)&pUTF8Str);
			if (pUTF8Str)
			{
				AddWorkAddress2(cardRow, pUTF8Str);
				PR_FREEIF(pUTF8Str);
			}
			PR_FREEIF(pUnicodeStr);
		}
		card->GetWorkCity(&pUnicodeStr); 
		unicharLength = nsCRT::strlen(pUnicodeStr);
		if (pUnicodeStr && unicharLength)
		{
			INTL_ConvertFromUnicode(pUnicodeStr, unicharLength, (char**)&pUTF8Str);
			if (pUTF8Str)
			{
				AddWorkCity(cardRow, pUTF8Str);
				PR_FREEIF(pUTF8Str);
			}
			PR_FREEIF(pUnicodeStr);
		}
		card->GetWorkState(&pUnicodeStr); 
		unicharLength = nsCRT::strlen(pUnicodeStr);
		if (pUnicodeStr && unicharLength)
		{
			INTL_ConvertFromUnicode(pUnicodeStr, unicharLength, (char**)&pUTF8Str);
			if (pUTF8Str)
			{
				AddWorkState(cardRow, pUTF8Str);
				PR_FREEIF(pUTF8Str);
			}
			PR_FREEIF(pUnicodeStr);
		}
		card->GetWorkZipCode(&pUnicodeStr); 
		unicharLength = nsCRT::strlen(pUnicodeStr);
		if (pUnicodeStr && unicharLength)
		{
			INTL_ConvertFromUnicode(pUnicodeStr, unicharLength, (char**)&pUTF8Str);
			if (pUTF8Str)
			{
				AddWorkZipCode(cardRow, pUTF8Str);
				PR_FREEIF(pUTF8Str);
			}
			PR_FREEIF(pUnicodeStr);
		}
		card->GetWorkCountry(&pUnicodeStr); 
		unicharLength = nsCRT::strlen(pUnicodeStr);
		if (pUnicodeStr && unicharLength)
		{
			INTL_ConvertFromUnicode(pUnicodeStr, unicharLength, (char**)&pUTF8Str);
			if (pUTF8Str)
			{
				AddWorkCountry(cardRow, pUTF8Str);
				PR_FREEIF(pUTF8Str);
			}
			PR_FREEIF(pUnicodeStr);
		}
		card->GetJobTitle(&pUnicodeStr); 
		unicharLength = nsCRT::strlen(pUnicodeStr);
		if (pUnicodeStr && unicharLength)
		{
			INTL_ConvertFromUnicode(pUnicodeStr, unicharLength, (char**)&pUTF8Str);
			if (pUTF8Str)
			{
				AddJobTitle(cardRow, pUTF8Str);
				PR_FREEIF(pUTF8Str);
			}
			PR_FREEIF(pUnicodeStr);
		}
		card->GetDepartment(&pUnicodeStr); 
		unicharLength = nsCRT::strlen(pUnicodeStr);
		if (pUnicodeStr && unicharLength)
		{
			INTL_ConvertFromUnicode(pUnicodeStr, unicharLength, (char**)&pUTF8Str);
			if (pUTF8Str)
			{
				AddDepartment(cardRow, pUTF8Str);
				PR_FREEIF(pUTF8Str);
			}
			PR_FREEIF(pUnicodeStr);
		}
		card->GetCompany(&pUnicodeStr); 
		unicharLength = nsCRT::strlen(pUnicodeStr);
		if (pUnicodeStr && unicharLength)
		{
			INTL_ConvertFromUnicode(pUnicodeStr, unicharLength, (char**)&pUTF8Str);
			if (pUTF8Str)
			{
				AddCompany(cardRow, pUTF8Str);
				PR_FREEIF(pUTF8Str);
			}
			PR_FREEIF(pUnicodeStr);
		}
		card->GetWebPage1(&pUnicodeStr); 
		unicharLength = nsCRT::strlen(pUnicodeStr);
		if (pUnicodeStr && unicharLength)
		{
			INTL_ConvertFromUnicode(pUnicodeStr, unicharLength, (char**)&pUTF8Str);
			if (pUTF8Str)
			{
				AddWebPage1(cardRow,pUTF8Str);
				PR_FREEIF(pUTF8Str);
			}
			PR_FREEIF(pUnicodeStr);
		}
		card->GetWebPage2(&pUnicodeStr); 
		unicharLength = nsCRT::strlen(pUnicodeStr);
		if (pUnicodeStr && unicharLength)
		{
			INTL_ConvertFromUnicode(pUnicodeStr, unicharLength, (char**)&pUTF8Str);
			if (pUTF8Str)
			{
				AddWebPage2(cardRow, pUTF8Str);
				PR_FREEIF(pUTF8Str);
			}
			PR_FREEIF(pUnicodeStr);
		}
		card->GetBirthYear(&pUnicodeStr); 
		unicharLength = nsCRT::strlen(pUnicodeStr);
		if (pUnicodeStr && unicharLength)
		{
			INTL_ConvertFromUnicode(pUnicodeStr, unicharLength, (char**)&pUTF8Str);
			if (pUTF8Str)
			{
				AddBirthYear(cardRow, pUTF8Str);
				PR_FREEIF(pUTF8Str);
			}
			PR_FREEIF(pUnicodeStr);
		}
		card->GetBirthMonth(&pUnicodeStr); 
		unicharLength = nsCRT::strlen(pUnicodeStr);
		if (pUnicodeStr && unicharLength)
		{
			INTL_ConvertFromUnicode(pUnicodeStr, unicharLength, (char**)&pUTF8Str);
			if (pUTF8Str)
			{
				AddBirthMonth(cardRow, pUTF8Str);
				PR_FREEIF(pUTF8Str);
			}
			PR_FREEIF(pUnicodeStr);
		}
		card->GetBirthDay(&pUnicodeStr); 
		unicharLength = nsCRT::strlen(pUnicodeStr);
		if (pUnicodeStr && unicharLength)
		{
			INTL_ConvertFromUnicode(pUnicodeStr, unicharLength, (char**)&pUTF8Str);
			if (pUTF8Str)
			{
				AddBirthDay(cardRow, pUTF8Str);
				PR_FREEIF(pUTF8Str);
			}
			PR_FREEIF(pUnicodeStr);
		}
		card->GetCustom1(&pUnicodeStr); 
		unicharLength = nsCRT::strlen(pUnicodeStr);
		if (pUnicodeStr && unicharLength)
		{
			INTL_ConvertFromUnicode(pUnicodeStr, unicharLength, (char**)&pUTF8Str);
			if (pUTF8Str)
			{
				AddCustom1(cardRow,pUTF8Str);
				PR_FREEIF(pUTF8Str);
			}
			PR_FREEIF(pUnicodeStr);
		}
		card->GetCustom2(&pUnicodeStr); 
		unicharLength = nsCRT::strlen(pUnicodeStr);
		if (pUnicodeStr && unicharLength)
		{
			INTL_ConvertFromUnicode(pUnicodeStr, unicharLength, (char**)&pUTF8Str);
			if (pUTF8Str)
			{
				AddCustom2(cardRow, pUTF8Str);
				PR_FREEIF(pUTF8Str);
			}
			PR_FREEIF(pUnicodeStr);
		}
		card->GetCustom3(&pUnicodeStr); 
		unicharLength = nsCRT::strlen(pUnicodeStr);
		if (pUnicodeStr && unicharLength)
		{
			INTL_ConvertFromUnicode(pUnicodeStr, unicharLength, (char**)&pUTF8Str);
			if (pUTF8Str)
			{
				AddCustom3(cardRow, pUTF8Str);
				PR_FREEIF(pUTF8Str);
			}
			PR_FREEIF(pUnicodeStr);
		}
		card->GetCustom4(&pUnicodeStr); 
		unicharLength = nsCRT::strlen(pUnicodeStr);
		if (pUnicodeStr && unicharLength)
		{
			INTL_ConvertFromUnicode(pUnicodeStr, unicharLength, (char**)&pUTF8Str);
			if (pUTF8Str)
			{
				AddCustom4(cardRow, pUTF8Str);
				PR_FREEIF(pUTF8Str);
			}
			PR_FREEIF(pUnicodeStr);
		}
		card->GetNotes(&pUnicodeStr); 
		unicharLength = nsCRT::strlen(pUnicodeStr);
		if (pUnicodeStr && unicharLength)
		{
			INTL_ConvertFromUnicode(pUnicodeStr, unicharLength, (char**)&pUTF8Str);
			if (pUTF8Str)
			{
				AddNotes(cardRow, pUTF8Str);
				PR_FREEIF(pUTF8Str);
			}
			PR_FREEIF(pUnicodeStr);
		}
	}
	return NS_OK;
}

NS_IMETHODIMP nsAddrDatabase::CreateNewCardAndAddToDB(nsIAbCard *newCard, PRBool notify /* = FALSE */)
{
	nsresult	err = NS_OK;
	nsIMdbRow	*cardRow;

	if (!newCard || !m_mdbPabTable)
		return NS_ERROR_NULL_POINTER;

	err  = GetNewRow(&cardRow);

	if (NS_SUCCEEDED(err) && cardRow)
	{
		AddAttributeColumnsToRow(newCard, cardRow);
		err = m_mdbPabTable->AddRow(GetEnv(), cardRow);
	}
	if (NS_FAILED(err)) return err;

	//  do notification
	if (notify)
	{
		NotifyCardEntryChange(AB_NotifyInserted, newCard, NULL);
	}
	return err;
}

nsresult nsAddrDatabase::FindAttributeRow(nsIMdbTable* pTable, mdb_token columnToken, nsIMdbRow** row)
{
	nsIMdbTableRowCursor* rowCursor = nsnull;
	nsIMdbRow* findRow = nsnull;
	nsIMdbCell* valueCell = nsnull;
 	mdb_pos	rowPos = 0;
	nsresult err = NS_ERROR_FAILURE;

	err = pTable->GetTableRowCursor(GetEnv(), -1, &rowCursor);

	if (NS_FAILED(err) || !rowCursor)
		return NS_ERROR_FAILURE;
	do
	{
		err = rowCursor->NextRow(GetEnv(), &findRow, &rowPos);
		if (NS_SUCCEEDED(err) && findRow)
		{
			err = findRow->GetCell(GetEnv(), columnToken, &valueCell);
			if (NS_SUCCEEDED(err) && valueCell)
			{
				*row = findRow;
				return NS_OK;
			}
		}
	} while (findRow);

	return NS_ERROR_FAILURE;
}

nsresult nsAddrDatabase::DoStringAnonymousTransaction
(nsVoidArray* pAttributes, nsVoidArray* pValues, AB_NOTIFY_CODE code)
{
	nsresult err = NS_OK;

	if (pAttributes && pValues)
	{
		PRUint32 count, i;
		count = pAttributes->Count();
		for (i = 0; i < count; i++)
		{
			char* pAttrStr = (char*)pAttributes->ElementAt(i);
			mdb_token anonymousColumnToken;
			GetStore()->StringToToken(GetEnv(),  pAttrStr, &anonymousColumnToken);
			char* pValueStr = (char*)pValues->ElementAt(i);

			nsIMdbRow	*anonymousRow = nsnull;
			if (code == AB_NotifyInserted)
			{
				err  = GetNewRow(&anonymousRow);
				if (NS_SUCCEEDED(err) && anonymousRow)
				{
					AddCharStringColumn(anonymousRow, anonymousColumnToken, pValueStr);
					err = m_mdbAnonymousTable->AddRow(GetEnv(), anonymousRow);
				}
			}
			else if (code == AB_NotifyDeleted)
			{
				struct mdbYarn yarn;
				mdbOid rowOid;

				GetCharStringYarn(pValueStr, &yarn);
				err = GetStore()->FindRow(GetEnv(), m_CardRowScopeToken, anonymousColumnToken,
										&yarn, &rowOid, &anonymousRow);
				if (NS_SUCCEEDED(err) && anonymousRow)
					err = m_mdbAnonymousTable->CutRow(GetEnv(), anonymousRow);
			}
			else /* Edit */
			{
				err = FindAttributeRow(m_mdbAnonymousTable, anonymousColumnToken, &anonymousRow);
				if (NS_SUCCEEDED(err) && anonymousRow)
				{
					AddCharStringColumn(anonymousRow, anonymousColumnToken, pValueStr);
					err = m_mdbAnonymousTable->AddRow(GetEnv(), anonymousRow);
					return NS_OK;
				} 
				err = NS_ERROR_FAILURE;
			}
 		}
	}
	return err;
}

nsresult nsAddrDatabase::DoIntAnonymousTransaction
(nsVoidArray* pAttributes, nsVoidArray* pValues, AB_NOTIFY_CODE code)
{
	nsresult err = NS_OK;
	if (pAttributes && pValues)
	{
		PRUint32 count, i;
		count = pAttributes->Count();
		for (i = 0; i < count; i++)
		{
			char* pAttrStr = (char*)pAttributes->ElementAt(i);
			mdb_token anonymousColumnToken;
			GetStore()->StringToToken(GetEnv(),  pAttrStr, &anonymousColumnToken);
			PRUint32* pValue = (PRUint32*)pValues->ElementAt(i);
			PRUint32 value = *pValue;

			nsIMdbRow	*anonymousRow = nsnull;
			if (code == AB_NotifyInserted)
			{
				err  = GetNewRow(&anonymousRow);
				if (NS_SUCCEEDED(err) && anonymousRow)
				{
					AddIntColumn(anonymousRow, anonymousColumnToken, value);
					err = m_mdbAnonymousTable->AddRow(GetEnv(), anonymousRow);
				}
			}
			else if (code == AB_NotifyDeleted)
			{
				struct mdbYarn yarn;
				mdbOid rowOid;
				char yarnBuf[100];

				yarn.mYarn_Buf = (void *) yarnBuf;
				GetIntYarn(value, &yarn);
				err = GetStore()->FindRow(GetEnv(), m_CardRowScopeToken, anonymousColumnToken,
										&yarn, &rowOid, &anonymousRow);
				if (NS_SUCCEEDED(err) && anonymousRow)
					err = m_mdbAnonymousTable->CutRow(GetEnv(), anonymousRow);
			}
			else
			{
				err = FindAttributeRow(m_mdbAnonymousTable, anonymousColumnToken, &anonymousRow);
				if (NS_SUCCEEDED(err) && anonymousRow)
				{
					AddIntColumn(anonymousRow, anonymousColumnToken, value);
					err = m_mdbAnonymousTable->AddRow(GetEnv(), anonymousRow);
					return NS_OK;
				} 
				err = NS_ERROR_FAILURE;
			}
 		}
	}
	return err;
}

nsresult nsAddrDatabase::DoBoolAnonymousTransaction
(nsVoidArray* pAttributes, nsVoidArray* pValues, AB_NOTIFY_CODE code)
{
	nsresult err = NS_OK;
	if (pAttributes && pValues)
	{
		PRUint32 count, i;
		count = m_pAnonymousBoolAttributes->Count();
		for (i = 0; i < count; i++)
		{
			char* pAttrStr = (char*)pAttributes->ElementAt(i);
			mdb_token anonymousColumnToken;
			GetStore()->StringToToken(GetEnv(),  pAttrStr, &anonymousColumnToken);
			PRBool* pValue = (PRBool*)pValues->ElementAt(i);
			PRBool value = *pValue;
			PRUint32 nBoolValue = 0;
			if (value)
				nBoolValue = 1;
			else
				nBoolValue = 0;

			nsIMdbRow	*anonymousRow = nsnull;
			if (code == AB_NotifyInserted)
			{
				err  = GetNewRow(&anonymousRow);
				if (NS_SUCCEEDED(err) && anonymousRow)
				{
					AddIntColumn(anonymousRow, anonymousColumnToken, nBoolValue);
					err = m_mdbAnonymousTable->AddRow(GetEnv(), anonymousRow);
				}
			}
			else if (code == AB_NotifyDeleted)
			{
				struct mdbYarn yarn;
				mdbOid rowOid;
				char yarnBuf[100];

				yarn.mYarn_Buf = (void *) yarnBuf;
				GetIntYarn(nBoolValue, &yarn);
				err = GetStore()->FindRow(GetEnv(), m_CardRowScopeToken, anonymousColumnToken,
										&yarn, &rowOid, &anonymousRow);
				if (NS_SUCCEEDED(err) && anonymousRow)
					err = m_mdbAnonymousTable->CutRow(GetEnv(), anonymousRow);
			}
			else
			{
				err = FindAttributeRow(m_mdbAnonymousTable, anonymousColumnToken, &anonymousRow);
				if (NS_SUCCEEDED(err) && anonymousRow)
				{
					AddIntColumn(anonymousRow, anonymousColumnToken, nBoolValue);
					err = m_mdbAnonymousTable->AddRow(GetEnv(), anonymousRow);
					return NS_OK;
				} 
				err = NS_ERROR_FAILURE;
			}
 		}
	}
	return err;
}

nsresult nsAddrDatabase::DoAnonymousAttributesTransaction(AB_NOTIFY_CODE code)
{
	nsresult err = NS_OK;
		  
	if (!m_mdbAnonymousTable)
		err = InitAnonymousTable();

	if (NS_FAILED(err) || !m_mdbAnonymousTable)
		return NS_ERROR_FAILURE;

	DoStringAnonymousTransaction(m_pAnonymousStrAttributes, m_pAnonymousStrValues, code);
	DoIntAnonymousTransaction(m_pAnonymousIntAttributes, m_pAnonymousIntValues, code);
	DoBoolAnonymousTransaction(m_pAnonymousBoolAttributes, m_pAnonymousBoolValues, code);

	Commit(kSessionCommit);
	return err;
}

void nsAddrDatabase::GetAnonymousAttributesFromCard(nsIAbCard* card)
{
	nsresult err = NS_OK;
	RemoveAnonymousList(m_pAnonymousStrAttributes);
	RemoveAnonymousList(m_pAnonymousStrValues);
	RemoveAnonymousList(m_pAnonymousIntAttributes);
	RemoveAnonymousList(m_pAnonymousIntValues);
	RemoveAnonymousList(m_pAnonymousBoolAttributes);
	RemoveAnonymousList(m_pAnonymousBoolValues);
	err = card->GetAnonymousStrAttrubutesList(&m_pAnonymousStrAttributes);
	err = card->GetAnonymousStrValuesList(&m_pAnonymousStrValues);
	err = card->GetAnonymousIntAttrubutesList(&m_pAnonymousIntAttributes);
	err = card->GetAnonymousIntValuesList(&m_pAnonymousIntValues);
	err = card->GetAnonymousBoolAttrubutesList(&m_pAnonymousBoolAttributes);
	err = card->GetAnonymousBoolValuesList(&m_pAnonymousBoolValues);
}

NS_IMETHODIMP nsAddrDatabase::AddAnonymousAttributesFromCard(nsIAbCard* card)
{
	GetAnonymousAttributesFromCard(card);
	return DoAnonymousAttributesTransaction(AB_NotifyInserted);
}

NS_IMETHODIMP nsAddrDatabase::AddAnonymousAttributesToDB()
{
	return DoAnonymousAttributesTransaction(AB_NotifyInserted);
}

NS_IMETHODIMP nsAddrDatabase::RemoveAnonymousAttributesFromCard(nsIAbCard *card)
{
	GetAnonymousAttributesFromCard(card);
	return DoAnonymousAttributesTransaction(AB_NotifyDeleted);
}

NS_IMETHODIMP nsAddrDatabase::RemoveAnonymousAttributesFromDB()
{
	return DoAnonymousAttributesTransaction(AB_NotifyDeleted);
}

NS_IMETHODIMP nsAddrDatabase::EditAnonymousAttributesFromCard(nsIAbCard* card)
{
	GetAnonymousAttributesFromCard(card);
	return DoAnonymousAttributesTransaction(AB_NotifyPropertyChanged);
}

NS_IMETHODIMP nsAddrDatabase::EditAnonymousAttributesInDB()
{
	return DoAnonymousAttributesTransaction(AB_NotifyPropertyChanged);
}

NS_IMETHODIMP nsAddrDatabase::DeleteCard(nsIAbCard *card, PRBool notify)
{
	nsresult err = NS_OK;

	if (!card || !m_mdbPabTable)
		return NS_ERROR_NULL_POINTER;

	// get the right row
	nsIMdbRow* pCardRow = nsnull;
	mdbOid rowOid;
	rowOid.mOid_Scope = m_CardRowScopeToken;
	card->GetDbRowID((PRUint32*)&rowOid.mOid_Id);
	err = GetStore()->GetRow(GetEnv(), &rowOid, &pCardRow);
	if (pCardRow)
	{
		err = m_mdbPabTable->CutRow(GetEnv(), pCardRow);

		if (NS_SUCCEEDED(err))
		{
			
			nsCOMPtr<nsIAddrDBListener> listener(do_QueryInterface(card, &err));
			if (NS_FAILED(err)) 
				return NS_ERROR_NULL_POINTER;
			RemoveListener(listener);

			if (notify) 
				NotifyCardEntryChange(AB_NotifyDeleted, card, NULL);
		}
	}
	return NS_OK;
}

NS_IMETHODIMP nsAddrDatabase::EditCard(nsIAbCard *card, PRBool notify)
{
	if (!card || !m_mdbPabTable)
		return NS_ERROR_NULL_POINTER;

	nsresult err = NS_OK;

	nsIMdbRow* pCardRow = nsnull;
	mdbOid rowOid;
	rowOid.mOid_Scope = m_CardRowScopeToken;
	card->GetDbRowID((PRUint32*)&rowOid.mOid_Id);
	err = GetStore()->GetRow(GetEnv(), &rowOid, &pCardRow);
	if (pCardRow)
		err = AddAttributeColumnsToRow(card, pCardRow);
	if (NS_FAILED(err)) return err;

	if (notify) 
		NotifyCardEntryChange(AB_NotifyPropertyChanged, card, NULL);

	return NS_OK;
}

NS_IMETHODIMP nsAddrDatabase::ContainsCard(nsIAbCard *card, PRBool *hasCard)
{
	if (!card || !m_mdbPabTable)
		return NS_ERROR_NULL_POINTER;

	nsresult err = NS_OK;
	mdb_bool hasOid;
	mdbOid rowOid;

	rowOid.mOid_Scope = m_CardRowScopeToken;
	card->GetDbRowID((PRUint32*)&rowOid.mOid_Id);
	err = m_mdbPabTable->HasOid(GetEnv(), &rowOid, &hasOid);
	if (NS_SUCCEEDED(err))
		*hasCard = hasOid;

	return err;
}

NS_IMETHODIMP nsAddrDatabase::GetNewRow(nsIMdbRow * *newRow)
{
	nsresult err = NS_OK;
	nsIMdbRow *row = nsnull;
	err  = GetStore()->NewRow(GetEnv(), m_CardRowScopeToken, &row);
	*newRow = row;
	return err;
}

NS_IMETHODIMP nsAddrDatabase::AddCardRowToDB(nsIMdbRow *newRow)
{
	if (m_mdbPabTable)
		return m_mdbPabTable->AddRow(GetEnv(), newRow);
	else
		return NS_ERROR_FAILURE;
}
 
void nsAddrDatabase::GetCharStringYarn(char* str, struct mdbYarn* strYarn)
{
	strYarn->mYarn_Grow = NULL;
	strYarn->mYarn_Buf = str;
	strYarn->mYarn_Size = PL_strlen((const char *) strYarn->mYarn_Buf) + 1;
	strYarn->mYarn_Fill = strYarn->mYarn_Size - 1;
	strYarn->mYarn_Form = 0;
}

void nsAddrDatabase::GetStringYarn(nsString* str, struct mdbYarn* strYarn)
{
	strYarn->mYarn_Buf = str->ToNewCString();
	strYarn->mYarn_Size = PL_strlen((const char *) strYarn->mYarn_Buf) + 1;
	strYarn->mYarn_Fill = strYarn->mYarn_Size - 1;
	strYarn->mYarn_Form = 0;	 
}

void nsAddrDatabase::GetIntYarn(PRUint32 nValue, struct mdbYarn* intYarn)
{
	intYarn->mYarn_Size = sizeof(intYarn->mYarn_Buf);
	intYarn->mYarn_Fill = intYarn->mYarn_Size;
	intYarn->mYarn_Form = 0;
	intYarn->mYarn_Grow = NULL;

	PR_snprintf((char*)intYarn->mYarn_Buf, intYarn->mYarn_Size, "%lx", nValue);
	intYarn->mYarn_Fill = PL_strlen((const char *) intYarn->mYarn_Buf);
}

mdb_err nsAddrDatabase::AddCharStringColumn(nsIMdbRow* cardRow, mdb_column inColumn, const char* str)
{
	struct mdbYarn yarn;

	GetCharStringYarn((char *) str, &yarn);
	mdb_err err = cardRow->AddColumn(GetEnv(),  inColumn, &yarn);

	return err;
}

mdb_err nsAddrDatabase::AddStringColumn(nsIMdbRow* cardRow, mdb_column inColumn, nsString* str)
{
	struct mdbYarn yarn;

	GetStringYarn(str, &yarn);
	mdb_err err = cardRow->AddColumn(GetEnv(),  inColumn, &yarn);

	return err;
}

mdb_err nsAddrDatabase::AddIntColumn(nsIMdbRow* cardRow, mdb_column inColumn, PRUint32 nValue)
{
	struct mdbYarn yarn;
	char	yarnBuf[100];

	yarn.mYarn_Buf = (void *) yarnBuf;
	GetIntYarn(nValue, &yarn);
	return cardRow->AddColumn(GetEnv(),  inColumn, &yarn);
}

nsresult nsAddrDatabase::GetStringColumn(nsIMdbRow *cardRow, mdb_token outToken, nsString& str)
{
	nsresult	err = NS_ERROR_FAILURE;
	nsIMdbCell	*cardCell;

	if (cardRow)	
	{
		err = cardRow->GetCell(GetEnv(), outToken, &cardCell);
		if (err == NS_OK && cardCell)
		{
			struct mdbYarn yarn;
			cardCell->AliasYarn(GetEnv(), &yarn);
			str.SetString((const char *) yarn.mYarn_Buf, yarn.mYarn_Fill);
			cardCell->CutStrongRef(GetEnv()); // always release ref
		}
		else
			err = NS_ERROR_FAILURE;
	}
	return err;
}

void nsAddrDatabase::YarnToUInt32(struct mdbYarn *yarn, PRUint32 *pResult)
{
	PRUint32 i, result, numChars;
	char *p = (char *) yarn->mYarn_Buf;
	if (yarn->mYarn_Fill > 8)
		numChars = 8;
	else
		numChars = yarn->mYarn_Fill;
	for (i=0, result = 0; i < numChars; i++, p++)
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

nsresult nsAddrDatabase::GetIntColumn
(nsIMdbRow *cardRow, mdb_token outToken, PRUint32* pValue, PRUint32 defaultValue)
{
	nsresult	err = NS_ERROR_FAILURE;
	nsIMdbCell	*cardCell;

	if (pValue)
		*pValue = defaultValue;
	if (cardRow)
	{
		err = cardRow->GetCell(GetEnv(), outToken, &cardCell);
		if (err == NS_OK && cardCell)
		{
			struct mdbYarn yarn;
			cardCell->AliasYarn(GetEnv(), &yarn);
			YarnToUInt32(&yarn, pValue);
			cardCell->CutStrongRef(GetEnv());
		}
		else
			err = NS_ERROR_FAILURE;
	}
	return err;
}

nsresult nsAddrDatabase::GetBoolColumn(nsIMdbRow *cardRow, mdb_token outToken, PRBool* pValue)
{
	nsresult	err = NS_ERROR_FAILURE;
	nsIMdbCell	*cardCell;
	PRUint32 nValue = 0;

	if (cardRow)
	{
		err = cardRow->GetCell(GetEnv(), outToken, &cardCell);
		if (err == NS_OK && cardCell)
		{
			struct mdbYarn yarn;
			cardCell->AliasYarn(GetEnv(), &yarn);
			YarnToUInt32(&yarn, &nValue);
			cardCell->CutStrongRef(GetEnv());
		}
		else
			err = NS_ERROR_FAILURE;
	}
	if (nValue == 0)
		*pValue = PR_FALSE;
	else
		*pValue = PR_TRUE;
	return err;
}


nsresult nsAddrDatabase::SetAnonymousAttribute
(nsVoidArray** pAttrAray, nsVoidArray** pValueArray, void *attrname, void *value)
{
	nsresult rv = NS_OK;
	nsVoidArray* pAttributes = *pAttrAray;
	nsVoidArray* pValues = *pValueArray; 

	if (!pAttributes && !pValues)
	{
		pAttributes = new nsVoidArray();
		pValues = new nsVoidArray();
	}
	if (pAttributes && pValues)
	{
		if (attrname && value)
		{
			pAttributes->AppendElement(attrname);
			pValues->AppendElement(value);
			*pAttrAray = pAttributes;
			*pValueArray = pValues;
		}
		else
		{
			delete pAttributes;
			delete pValues;
		}
	}
	else
	{ 
		rv = NS_ERROR_FAILURE;
	}

	return rv;
}	


NS_IMETHODIMP nsAddrDatabase::SetAnonymousStringAttribute
(const char *attrname, const char *value)
{
	nsresult rv = NS_OK;

	char* pAttribute = PL_strdup(attrname);
	char* pValue = PL_strdup(value);
	if (pAttribute && pValue)
	{
		rv = SetAnonymousAttribute(&m_pAnonymousStrAttributes, 
			&m_pAnonymousStrValues, pAttribute, pValue);
	}
	else
	{
		PR_FREEIF(pAttribute);
		PR_FREEIF(pValue);
		rv = NS_ERROR_NULL_POINTER;
	}
	return rv;
}	

NS_IMETHODIMP nsAddrDatabase::SetAnonymousIntAttribute
(const char *attrname, PRUint32 value)
{
	nsresult rv = NS_OK;

	char* pAttribute = PL_strdup(attrname);
	PRUint32* pValue = (PRUint32 *)PR_Calloc(1, sizeof(PRUint32));
	*pValue = value;
	if (pAttribute && pValue)
	{
		rv = SetAnonymousAttribute(&m_pAnonymousIntAttributes, 
			&m_pAnonymousIntValues, pAttribute, pValue);
	}
	else
	{
		PR_FREEIF(pAttribute);
		PR_FREEIF(pValue);
		rv = NS_ERROR_NULL_POINTER;
	}
	return rv;
}	

NS_IMETHODIMP nsAddrDatabase::SetAnonymousBoolAttribute
(const char *attrname, PRBool value)
{
	nsresult rv = NS_OK;

	char* pAttribute = PL_strdup(attrname);
	PRBool* pValue = (PRBool *)PR_Calloc(1, sizeof(PRBool));
	*pValue = value;
	if (pAttribute && pValue)
	{
		rv = SetAnonymousAttribute(&m_pAnonymousBoolAttributes, 
			&m_pAnonymousBoolValues, pAttribute, pValue);
	}
	else
	{
		PR_FREEIF(pAttribute);
		PR_FREEIF(pValue);
		rv = NS_ERROR_NULL_POINTER;
	}
	return rv;
}

NS_IMETHODIMP nsAddrDatabase::GetAnonymousStringAttribute(const char *attrname, char** value)
{
	if (m_mdbAnonymousTable)
	{
		nsIMdbRow* cardRow;
		nsIMdbTableRowCursor* rowCursor;
		mdb_pos rowPos;
		nsAutoString tempString;
		char *tempCString = nsnull;

		mdb_token anonymousColumnToken;
		GetStore()->StringToToken(GetEnv(), attrname, &anonymousColumnToken);


		m_mdbAnonymousTable->GetTableRowCursor(GetEnv(), -1, &rowCursor);
		do 
		{
			mdb_err err = rowCursor->NextRow(GetEnv(), &cardRow, &rowPos);

			if (NS_SUCCEEDED(err) && cardRow)
			{
				err = GetStringColumn(cardRow, anonymousColumnToken, tempString);
				if (NS_SUCCEEDED(err) && tempString.Length())
				{
					tempCString = tempString.ToNewCString();
					*value = PL_strdup(tempCString);
					delete [] tempCString;
					return NS_OK;
				}
			}
		} while (cardRow);
	}
	return NS_ERROR_FAILURE;
}

NS_IMETHODIMP nsAddrDatabase::GetAnonymousIntAttribute(const char *attrname, PRUint32* value)
{
	if (m_mdbAnonymousTable)
	{
		nsIMdbRow* cardRow;
		nsIMdbTableRowCursor* rowCursor;
		mdb_pos rowPos;
		PRUint32 nValue;

		mdb_token anonymousColumnToken;
		GetStore()->StringToToken(GetEnv(), attrname, &anonymousColumnToken);


		m_mdbAnonymousTable->GetTableRowCursor(GetEnv(), -1, &rowCursor);
		do 
		{
			mdb_err err = rowCursor->NextRow(GetEnv(), &cardRow, &rowPos);

			if (NS_SUCCEEDED(err) && cardRow)
			{
				err = GetIntColumn(cardRow, anonymousColumnToken, &nValue, 0);
				if (NS_SUCCEEDED(err))
				{
					*value = nValue;
					return err;
				}
			}
		} while (cardRow);
	}
	return NS_ERROR_FAILURE;
}

NS_IMETHODIMP nsAddrDatabase::GetAnonymousBoolAttribute(const char *attrname, PRBool* value)
{
	if (m_mdbAnonymousTable)
	{
		nsIMdbRow* cardRow;
		nsIMdbTableRowCursor* rowCursor;
		mdb_pos rowPos;
		PRUint32 nValue;

		mdb_token anonymousColumnToken;
		GetStore()->StringToToken(GetEnv(), attrname, &anonymousColumnToken);


		m_mdbAnonymousTable->GetTableRowCursor(GetEnv(), -1, &rowCursor);
		do 
		{
			mdb_err err = rowCursor->NextRow(GetEnv(), &cardRow, &rowPos);

			if (NS_SUCCEEDED(err) && cardRow)
			{
				err = GetIntColumn(cardRow, anonymousColumnToken, &nValue, 0);
				if (NS_SUCCEEDED(err))
				{
					if (nValue)
						*value = PR_TRUE;
					else
						*value = PR_FALSE;
					return err;
				}
			}
		} while (cardRow);
	}
	return NS_ERROR_FAILURE;
}

nsresult nsAddrDatabase::GetCardFromDB(nsIAbCard *newCard, nsIMdbRow* cardRow)
{
	nsresult	err = NS_OK;
	if (!newCard || !cardRow)
		return NS_ERROR_NULL_POINTER;

    nsAutoString tempString;
	char *tempCString = nsnull;
	PRUnichar *unicodeStr = nsnull;
	PRInt32 unicharLength = 0;

	err = GetStringColumn(cardRow, m_FirstNameColumnToken, tempString);
	if (NS_SUCCEEDED(err) && tempString.Length())
	{
		tempCString = tempString.ToNewCString();
		INTL_ConvertToUnicode((const char *)tempCString, nsCRT::strlen(tempCString), (void**)&unicodeStr, &unicharLength);
		newCard->SetFirstName(unicodeStr);
		nsAllocator::Free(tempCString);
		PR_Free(unicodeStr);
	}

	err = GetStringColumn(cardRow, m_LastNameColumnToken, tempString);
	if (NS_SUCCEEDED(err) && tempString.Length())
	{
		tempCString = tempString.ToNewCString();
		INTL_ConvertToUnicode((const char *)tempCString, nsCRT::strlen(tempCString), (void**)&unicodeStr, &unicharLength);
		newCard->SetLastName(unicodeStr);
		delete [] tempCString;
		PR_Free(unicodeStr);
	}

	err = GetStringColumn(cardRow, m_DisplayNameColumnToken, tempString);
	if (NS_SUCCEEDED(err) && tempString.Length())
	{
		tempCString = tempString.ToNewCString();
		INTL_ConvertToUnicode((const char *)tempCString, nsCRT::strlen(tempCString), (void**)&unicodeStr, &unicharLength);
		newCard->SetDisplayName(unicodeStr);
		delete [] tempCString;
		PR_Free(unicodeStr);
	}

	err = GetStringColumn(cardRow, m_NickNameColumnToken, tempString);
	if (NS_SUCCEEDED(err) && tempString.Length())
	{
		tempCString = tempString.ToNewCString();
		INTL_ConvertToUnicode((const char *)tempCString, nsCRT::strlen(tempCString), (void**)&unicodeStr, &unicharLength);
		newCard->SetNickName(unicodeStr);
		delete [] tempCString;
		PR_Free(unicodeStr);
	}

	err = GetStringColumn(cardRow, m_PriEmailColumnToken, tempString);
	if (NS_SUCCEEDED(err) && tempString.Length())
	{
		tempCString = tempString.ToNewCString();
		INTL_ConvertToUnicode((const char *)tempCString, nsCRT::strlen(tempCString), (void**)&unicodeStr, &unicharLength);
		newCard->SetPrimaryEmail(unicodeStr);
		delete [] tempCString;
		PR_Free(unicodeStr);
	}

	err = GetStringColumn(cardRow, m_2ndEmailColumnToken, tempString);
	if (NS_SUCCEEDED(err) && tempString.Length())
	{
		tempCString = tempString.ToNewCString();
		INTL_ConvertToUnicode((const char *)tempCString, nsCRT::strlen(tempCString), (void**)&unicodeStr, &unicharLength);
		newCard->SetSecondEmail(unicodeStr);
		delete [] tempCString;
		PR_Free(unicodeStr);
	}

	err = GetStringColumn(cardRow, m_WorkPhoneColumnToken, tempString);
	if (NS_SUCCEEDED(err) && tempString.Length())
	{
		tempCString = tempString.ToNewCString();
		INTL_ConvertToUnicode((const char *)tempCString, nsCRT::strlen(tempCString), (void**)&unicodeStr, &unicharLength);
		newCard->SetWorkPhone(unicodeStr);
		delete [] tempCString;
		PR_Free(unicodeStr);
	}

	err = GetStringColumn(cardRow, m_HomePhoneColumnToken, tempString);
	if (NS_SUCCEEDED(err) && tempString.Length())
	{
		tempCString = tempString.ToNewCString();
		INTL_ConvertToUnicode((const char *)tempCString, nsCRT::strlen(tempCString), (void**)&unicodeStr, &unicharLength);
		newCard->SetHomePhone(unicodeStr);
		delete [] tempCString;
		PR_Free(unicodeStr);
	}

	err = GetStringColumn(cardRow, m_FaxColumnToken, tempString);
	if (NS_SUCCEEDED(err) && tempString.Length())
	{
		tempCString = tempString.ToNewCString();
		INTL_ConvertToUnicode((const char *)tempCString, nsCRT::strlen(tempCString), (void**)&unicodeStr, &unicharLength);
		newCard->SetFaxNumber(unicodeStr);
		delete [] tempCString;
		PR_Free(unicodeStr);
	}

	err = GetStringColumn(cardRow, m_PagerColumnToken, tempString);
	if (NS_SUCCEEDED(err) && tempString.Length())
	{
		tempCString = tempString.ToNewCString();
		INTL_ConvertToUnicode((const char *)tempCString, nsCRT::strlen(tempCString), (void**)&unicodeStr, &unicharLength);
		newCard->SetPagerNumber(unicodeStr);
		delete [] tempCString;
		PR_Free(unicodeStr);
	}

	err = GetStringColumn(cardRow, m_CellularColumnToken, tempString);
	if (NS_SUCCEEDED(err) && tempString.Length())
	{
		tempCString = tempString.ToNewCString();
		INTL_ConvertToUnicode((const char *)tempCString, nsCRT::strlen(tempCString), (void**)&unicodeStr, &unicharLength);
		newCard->SetCellularNumber(unicodeStr);
		delete [] tempCString;
		PR_Free(unicodeStr);
	}

	err = GetStringColumn(cardRow, m_HomeAddressColumnToken, tempString);
	if (NS_SUCCEEDED(err) && tempString.Length())
	{
		tempCString = tempString.ToNewCString();
		INTL_ConvertToUnicode((const char *)tempCString, nsCRT::strlen(tempCString), (void**)&unicodeStr, &unicharLength);
		newCard->SetHomeAddress(unicodeStr);
		delete [] tempCString;
		PR_Free(unicodeStr);
	}

	err = GetStringColumn(cardRow, m_HomeAddress2ColumnToken, tempString);
	if (NS_SUCCEEDED(err) && tempString.Length())
	{
		tempCString = tempString.ToNewCString();
		INTL_ConvertToUnicode((const char *)tempCString, nsCRT::strlen(tempCString), (void**)&unicodeStr, &unicharLength);
		newCard->SetHomeAddress2(unicodeStr);
		delete [] tempCString;
		PR_Free(unicodeStr);
	}

	err = GetStringColumn(cardRow, m_HomeCityColumnToken, tempString);
	if (NS_SUCCEEDED(err) && tempString.Length())
	{
		tempCString = tempString.ToNewCString();
		INTL_ConvertToUnicode((const char *)tempCString, nsCRT::strlen(tempCString), (void**)&unicodeStr, &unicharLength);
		newCard->SetHomeCity(unicodeStr);
		delete [] tempCString;
		PR_Free(unicodeStr);
	}

	err = GetStringColumn(cardRow, m_HomeStateColumnToken, tempString);
	if (NS_SUCCEEDED(err) && tempString.Length())
	{
		tempCString = tempString.ToNewCString();
		INTL_ConvertToUnicode((const char *)tempCString, nsCRT::strlen(tempCString), (void**)&unicodeStr, &unicharLength);
		newCard->SetHomeState(unicodeStr);
		delete [] tempCString;
		PR_Free(unicodeStr);
	}

	err = GetStringColumn(cardRow, m_HomeZipCodeColumnToken, tempString);
	if (NS_SUCCEEDED(err) && tempString.Length())
	{
		tempCString = tempString.ToNewCString();
		INTL_ConvertToUnicode((const char *)tempCString, nsCRT::strlen(tempCString), (void**)&unicodeStr, &unicharLength);
		newCard->SetHomeZipCode(unicodeStr);
		delete [] tempCString;
		PR_Free(unicodeStr);
	}

	err = GetStringColumn(cardRow, m_HomeCountryColumnToken, tempString);
	if (NS_SUCCEEDED(err) && tempString.Length())
	{
		tempCString = tempString.ToNewCString();
		INTL_ConvertToUnicode((const char *)tempCString, nsCRT::strlen(tempCString), (void**)&unicodeStr, &unicharLength);
		newCard->SetHomeCountry(unicodeStr);
		delete [] tempCString;
		PR_Free(unicodeStr);
	}

	err = GetStringColumn(cardRow, m_WorkAddressColumnToken, tempString);
	if (NS_SUCCEEDED(err) && tempString.Length())
	{
		tempCString = tempString.ToNewCString();
		INTL_ConvertToUnicode((const char *)tempCString, nsCRT::strlen(tempCString), (void**)&unicodeStr, &unicharLength);
		newCard->SetWorkAddress(unicodeStr);
		delete [] tempCString;
		PR_Free(unicodeStr);
	}

	err = GetStringColumn(cardRow, m_WorkAddress2ColumnToken, tempString);
	if (NS_SUCCEEDED(err) && tempString.Length())
	{
		tempCString = tempString.ToNewCString();
		INTL_ConvertToUnicode((const char *)tempCString, nsCRT::strlen(tempCString), (void**)&unicodeStr, &unicharLength);
		newCard->SetWorkAddress2(unicodeStr);
		delete [] tempCString;
		PR_Free(unicodeStr);
	}

	err = GetStringColumn(cardRow, m_WorkCityColumnToken, tempString);
	if (NS_SUCCEEDED(err) && tempString.Length())
	{
		tempCString = tempString.ToNewCString();
		INTL_ConvertToUnicode((const char *)tempCString, nsCRT::strlen(tempCString), (void**)&unicodeStr, &unicharLength);
		newCard->SetWorkCity(unicodeStr);
		delete [] tempCString;
		PR_Free(unicodeStr);
	}

	err = GetStringColumn(cardRow, m_WorkStateColumnToken, tempString);
	if (NS_SUCCEEDED(err) && tempString.Length())
	{
		tempCString = tempString.ToNewCString();
		INTL_ConvertToUnicode((const char *)tempCString, nsCRT::strlen(tempCString), (void**)&unicodeStr, &unicharLength);
		newCard->SetWorkState(unicodeStr);
		delete [] tempCString;
		PR_Free(unicodeStr);
	}

	err = GetStringColumn(cardRow, m_WorkZipCodeColumnToken, tempString);
	if (NS_SUCCEEDED(err) && tempString.Length())
	{
		tempCString = tempString.ToNewCString();
		INTL_ConvertToUnicode((const char *)tempCString, nsCRT::strlen(tempCString), (void**)&unicodeStr, &unicharLength);
		newCard->SetWorkZipCode(unicodeStr);
		delete [] tempCString;
		PR_Free(unicodeStr);
	}

	err = GetStringColumn(cardRow, m_WorkCountryColumnToken, tempString);
	if (NS_SUCCEEDED(err) && tempString.Length())
	{
		tempCString = tempString.ToNewCString();
		INTL_ConvertToUnicode((const char *)tempCString, nsCRT::strlen(tempCString), (void**)&unicodeStr, &unicharLength);
		newCard->SetWorkCountry(unicodeStr);
		delete [] tempCString;
		PR_Free(unicodeStr);
	}

	err = GetStringColumn(cardRow, m_JobTitleColumnToken, tempString);
	if (NS_SUCCEEDED(err) && tempString.Length())
	{
		tempCString = tempString.ToNewCString();
		INTL_ConvertToUnicode((const char *)tempCString, nsCRT::strlen(tempCString), (void**)&unicodeStr, &unicharLength);
		newCard->SetJobTitle(unicodeStr);
		delete [] tempCString;
		PR_Free(unicodeStr);
	}

	err = GetStringColumn(cardRow, m_DepartmentColumnToken, tempString);
	if (NS_SUCCEEDED(err) && tempString.Length())
	{
		tempCString = tempString.ToNewCString();
		INTL_ConvertToUnicode((const char *)tempCString, nsCRT::strlen(tempCString), (void**)&unicodeStr, &unicharLength);
		newCard->SetDepartment(unicodeStr);
		delete [] tempCString;
		PR_Free(unicodeStr);
	}

	err = GetStringColumn(cardRow, m_CompanyColumnToken, tempString);
	if (NS_SUCCEEDED(err) && tempString.Length())
	{
		tempCString = tempString.ToNewCString();
		INTL_ConvertToUnicode((const char *)tempCString, nsCRT::strlen(tempCString), (void**)&unicodeStr, &unicharLength);
		newCard->SetCompany(unicodeStr);
		delete [] tempCString;
		PR_Free(unicodeStr);
	}

	err = GetStringColumn(cardRow, m_WebPage1ColumnToken, tempString);
	if (NS_SUCCEEDED(err) && tempString.Length())
	{
		tempCString = tempString.ToNewCString();
		INTL_ConvertToUnicode((const char *)tempCString, nsCRT::strlen(tempCString), (void**)&unicodeStr, &unicharLength);
		newCard->SetWebPage1(unicodeStr);
		delete [] tempCString;
		PR_Free(unicodeStr);
	}

	err = GetStringColumn(cardRow, m_WebPage2ColumnToken, tempString);
	if (NS_SUCCEEDED(err) && tempString.Length())
	{
		tempCString = tempString.ToNewCString();
		INTL_ConvertToUnicode((const char *)tempCString, nsCRT::strlen(tempCString), (void**)&unicodeStr, &unicharLength);
		newCard->SetWebPage2(unicodeStr);
		delete [] tempCString;
		PR_Free(unicodeStr);
	}

	err = GetStringColumn(cardRow, m_BirthYearColumnToken, tempString);
	if (NS_SUCCEEDED(err) && tempString.Length())
	{
		tempCString = tempString.ToNewCString();
		INTL_ConvertToUnicode((const char *)tempCString, nsCRT::strlen(tempCString), (void**)&unicodeStr, &unicharLength);
		newCard->SetBirthYear(unicodeStr);
		delete [] tempCString;
		PR_Free(unicodeStr);
	}

	err = GetStringColumn(cardRow, m_BirthMonthColumnToken, tempString);
	if (NS_SUCCEEDED(err) && tempString.Length())
	{
		tempCString = tempString.ToNewCString();
		INTL_ConvertToUnicode((const char *)tempCString, nsCRT::strlen(tempCString), (void**)&unicodeStr, &unicharLength);
		newCard->SetBirthMonth(unicodeStr);
		delete [] tempCString;
		PR_Free(unicodeStr);
	}

	err = GetStringColumn(cardRow, m_BirthDayColumnToken, tempString);
	if (NS_SUCCEEDED(err) && tempString.Length())
	{
		tempCString = tempString.ToNewCString();
		INTL_ConvertToUnicode((const char *)tempCString, nsCRT::strlen(tempCString), (void**)&unicodeStr, &unicharLength);
		newCard->SetBirthDay(unicodeStr);
		delete [] tempCString;
		PR_Free(unicodeStr);
	}

	err = GetStringColumn(cardRow, m_Custom1ColumnToken, tempString);
	if (NS_SUCCEEDED(err) && tempString.Length())
	{
		tempCString = tempString.ToNewCString();
		INTL_ConvertToUnicode((const char *)tempCString, nsCRT::strlen(tempCString), (void**)&unicodeStr, &unicharLength);
		newCard->SetCustom1(unicodeStr);
		delete [] tempCString;
		PR_Free(unicodeStr);
	}

	err = GetStringColumn(cardRow, m_Custom2ColumnToken, tempString);
	if (NS_SUCCEEDED(err) && tempString.Length())
	{
		tempCString = tempString.ToNewCString();
		INTL_ConvertToUnicode((const char *)tempCString, nsCRT::strlen(tempCString), (void**)&unicodeStr, &unicharLength);
		newCard->SetCustom2(unicodeStr);
		delete [] tempCString;
		PR_Free(unicodeStr);
	}

	err = GetStringColumn(cardRow, m_Custom3ColumnToken, tempString);
	if (NS_SUCCEEDED(err) && tempString.Length())
	{
		tempCString = tempString.ToNewCString();
		INTL_ConvertToUnicode((const char *)tempCString, nsCRT::strlen(tempCString), (void**)&unicodeStr, &unicharLength);
		newCard->SetCustom3(unicodeStr);
		delete [] tempCString;
		PR_Free(unicodeStr);
	}

	err = GetStringColumn(cardRow, m_Custom4ColumnToken, tempString);
	if (NS_SUCCEEDED(err) && tempString.Length())
	{
		tempCString = tempString.ToNewCString();
		INTL_ConvertToUnicode((const char *)tempCString, nsCRT::strlen(tempCString), (void**)&unicodeStr, &unicharLength);
		newCard->SetCustom4(unicodeStr);
		delete [] tempCString;
		PR_Free(unicodeStr);
	}

	err = GetStringColumn(cardRow, m_NotesColumnToken, tempString);
	if (NS_SUCCEEDED(err) && tempString.Length())
	{
		tempCString = tempString.ToNewCString();
		INTL_ConvertToUnicode((const char *)tempCString, nsCRT::strlen(tempCString), (void**)&unicodeStr, &unicharLength);
		newCard->SetNotes(unicodeStr);
		delete [] tempCString;
		PR_Free(unicodeStr);
	}
	return err;
}

class nsAddrDBEnumerator : public nsIEnumerator 
{
public:
    NS_DECL_ISUPPORTS

    // nsIEnumerator methods:
    NS_DECL_NSIENUMERATOR

    // nsAddrDBEnumerator methods:

    nsAddrDBEnumerator(nsAddrDatabase* db, void* closure);
    virtual ~nsAddrDBEnumerator();

protected:
    nsCOMPtr<nsAddrDatabase>    mDB;
    nsCOMPtr<nsIAbCard>         mResultCard;
	nsIMdbTable*				mDbTable;
	nsIMdbTableRowCursor*       mRowCursor;
	nsIMdbRow*					mCurrentRow;
 	mdb_pos						mRowPos;
    PRBool                      mDone;
    void*                       mClosure;
};

nsAddrDBEnumerator::nsAddrDBEnumerator(nsAddrDatabase* db, void* closure)
    : mDB(db), mRowCursor(nsnull), mDone(PR_FALSE), mClosure(closure)
{
    NS_INIT_REFCNT();
}

nsAddrDBEnumerator::~nsAddrDBEnumerator()
{
}

NS_IMPL_ISUPPORTS(nsAddrDBEnumerator, nsCOMTypeInfo<nsIEnumerator>::GetIID())

NS_IMETHODIMP nsAddrDBEnumerator::First(void)
{
	nsresult rv = 0;
	mDone = PR_FALSE;

	if (!mDB || !mDB->GetPabTable() || !mDB->GetEnv())
		return NS_ERROR_NULL_POINTER;
		
	mDB->GetPabTable()->GetTableRowCursor(mDB->GetEnv(), -1, &mRowCursor);
	if (NS_FAILED(rv)) return rv;
    return Next();
}

NS_IMETHODIMP nsAddrDBEnumerator::Next(void)
{
	if (!mRowCursor)
	{
		mDone = PR_TRUE;
		return NS_ERROR_FAILURE;
	}
	nsresult rv = mRowCursor->NextRow(mDB->GetEnv(), &mCurrentRow, &mRowPos);
    if (mCurrentRow && NS_SUCCEEDED(rv))
		return NS_OK;
	else if (!mCurrentRow) 
	{
        mDone = PR_TRUE;
		return NS_ERROR_NULL_POINTER;
    }
    else if (NS_FAILED(rv)) {
        mDone = PR_TRUE;
        return NS_ERROR_FAILURE;
    }
	return NS_ERROR_FAILURE;
}

NS_IMETHODIMP nsAddrDBEnumerator::CurrentItem(nsISupports **aItem)
{
    if (mCurrentRow) 
	{
        nsresult rv;
        rv = mDB->CreateABCard(mCurrentRow, getter_AddRefs(mResultCard));
        *aItem = mResultCard;
		NS_IF_ADDREF(*aItem);
        return NS_OK;
    }
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP nsAddrDBEnumerator::IsDone(void)
{
    return mDone ? NS_OK : NS_ERROR_FAILURE;
}

////////////////////////////////////////////////////////////////////////////////

NS_IMETHODIMP nsAddrDatabase::EnumerateCards(nsIAbDirectory *directory, nsIEnumerator **result)
{
    nsAddrDBEnumerator* e = new nsAddrDBEnumerator(this, nsnull);
	m_dbDirectory = directory;
    if (e == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(e);
    *result = e;
    return NS_OK;
}

NS_IMETHODIMP nsAddrDatabase::EnumerateMailingLists(nsIAbDirectory *directory, nsIEnumerator **result)
{
    nsAddrDBEnumerator* e = new nsAddrDBEnumerator(this, nsnull);
	m_dbDirectory = directory;
    if (e == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(e);
    *result = e;
    return NS_OK;
}

nsresult nsAddrDatabase::CreateABCard(nsIMdbRow* cardRow, nsIAbCard **result)
{
    nsresult rv = NS_OK; 

	mdbOid outOid;
	mdb_id rowID=0;

	if (cardRow->GetOid(GetEnv(), &outOid) == NS_OK)
        rowID = outOid.mOid_Id;

	if(NS_SUCCEEDED(rv))
	{
		char* cardURI = nsnull;
		char* file = nsnull;

		file = m_dbName.GetLeafName();
		cardURI = PR_smprintf("%s%s/Card%ld", kCardDataSourceRoot, file, rowID);
		nsCOMPtr<nsIAbCard> personCard;
		rv = m_dbDirectory->AddChildCards(cardURI, getter_AddRefs(personCard));
		if (personCard)
		{
			GetCardFromDB(personCard, cardRow);
			mdbOid tableOid;
			m_mdbPabTable->GetOid(GetEnv(), &tableOid);
			personCard->SetDbTableID(tableOid.mOid_Id);
			personCard->SetDbRowID(rowID);
			personCard->SetAbDatabase(this);

			nsCOMPtr<nsIAddrDBListener> listener(do_QueryInterface(personCard, &rv));
			if (NS_FAILED(rv)) 
				return NS_ERROR_NULL_POINTER;

			AddListener(listener);
		}
		*result = personCard;
		NS_IF_ADDREF(*result);

		if (file)
			nsCRT::free(file);
		if (cardURI)
			PR_smprintf_free(cardURI);
	}

	return rv;
}


NS_IMETHODIMP nsAddrDatabase::GetCardForEmailAddress(nsIAbDirectory *directory, const char *emailAddress, nsIAbCard **cardResult)
{
	nsresult rv = NS_OK;
	if (!cardResult)
		return NS_ERROR_NULL_POINTER;

	m_dbDirectory = directory;

	mdbYarn	emailAddressYarn;

	emailAddressYarn.mYarn_Buf = (void *) emailAddress;
	emailAddressYarn.mYarn_Fill = PL_strlen(emailAddress);
	emailAddressYarn.mYarn_Form = 0;
	emailAddressYarn.mYarn_Size = emailAddressYarn.mYarn_Fill;

	nsIMdbRow	*cardRow;
	mdbOid		outRowId;
	mdb_err result = GetStore()->FindRow(GetEnv(), m_CardRowScopeToken,
		m_PriEmailColumnToken, &emailAddressYarn,  &outRowId, 
		&cardRow);
	if (NS_SUCCEEDED(result) && cardRow)
	{
		rv = CreateABCard(cardRow, cardResult);
	}
	else
		*cardResult = nsnull;
	return rv;
}

