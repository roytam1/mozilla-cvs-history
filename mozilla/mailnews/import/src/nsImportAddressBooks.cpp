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
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 *   Pierre Phaneuf <pp@ludusdesign.com>
 */

#include "prthread.h"
#include "prprf.h"
#include "nscore.h"
#include "nsCOMPtr.h"
#include "nsIImportService.h"
#include "nsISupportsArray.h"
#include "nsIImportAddressBooks.h"
#include "nsIImportGeneric.h"
#include "nsISupportsPrimitives.h"
#include "nsIImportABDescriptor.h"
#include "nsIImportFieldMap.h"
#include "nsCRT.h"
#include "nsString.h"
#include "nsIURL.h"
#include "nsNetCID.h"

#include "nsIFileSpec.h"

#include "nsIAddrDatabase.h"
#include "nsIAddrBookSession.h"
#include "nsIRDFService.h"
#include "nsRDFCID.h"
#include "nsAbBaseCID.h"
#include "nsIAbDirectory.h"
#include "nsImportStringBundle.h"
#include "nsTextFormatter.h"
#include "nsIProxyObjectManager.h"
#include "nsProxiedService.h"

#include "ImportDebug.h"

static NS_DEFINE_CID(kImportServiceCID, NS_IMPORTSERVICE_CID);
static NS_DEFINE_CID(kAddressBookDBCID, NS_ADDRDATABASE_CID);
static NS_DEFINE_CID(kAbDirectoryCID, NS_ABDIRECTORY_CID);
static NS_DEFINE_CID(kStandardUrlCID, NS_STANDARDURL_CID);
static NS_DEFINE_CID(kAddrBookSessionCID, NS_ADDRBOOKSESSION_CID);
static NS_DEFINE_CID(kRDFServiceCID, NS_RDFSERVICE_CID);
static NS_DEFINE_CID(kSupportsWStringCID, NS_SUPPORTS_WSTRING_CID);
static NS_DEFINE_CID(kProxyObjectManagerCID, NS_PROXYEVENT_MANAGER_CID);


////////////////////////////////////////////////////////////////////////

PR_STATIC_CALLBACK( void) ImportAddressThread( void *stuff);


class AddressThreadData;

class nsImportGenericAddressBooks : public nsIImportGeneric
{
public:

	nsImportGenericAddressBooks();
	virtual ~nsImportGenericAddressBooks();
	
	NS_DECL_ISUPPORTS

	/* nsISupports GetData (in string dataId); */
	NS_IMETHOD GetData(const char *dataId, nsISupports **_retval);

	NS_IMETHOD SetData(const char *dataId, nsISupports *pData);

	NS_IMETHOD GetStatus( const char *statusKind, PRInt32 *_retval);

	/* boolean WantsProgress (); */
	NS_IMETHOD WantsProgress(PRBool *_retval);

	/* boolean BeginImport (in nsIOutputStream successLog, in nsIOutputStream errorLog); */
	NS_IMETHOD BeginImport(nsISupportsWString *successLog, nsISupportsWString *errorLog, PRBool *_retval);

	/* boolean ContinueImport (); */
	NS_IMETHOD ContinueImport(PRBool *_retval);

	/* long GetProgress (); */
	NS_IMETHOD GetProgress(PRInt32 *_retval);

	/* void CancelImport (); */
	NS_IMETHOD CancelImport(void);

private:
	void	GetDefaultLocation( void);
	void	GetDefaultBooks( void);
	void	GetDefaultFieldMap( void);

public:
	static void	SetLogs( nsString& success, nsString& error, nsISupportsWString *pSuccess, nsISupportsWString *pError);
	static void ReportError( PRUnichar *pName, nsString *pStream);

private:
	nsIImportAddressBooks *		m_pInterface;
	nsISupportsArray *			m_pBooks;
	nsIFileSpec *				m_pLocation;
	nsIImportFieldMap *			m_pFieldMap;
	PRBool						m_autoFind;
	PRUnichar *					m_description;
	PRBool						m_gotLocation;
	PRBool						m_found;
	PRBool						m_userVerify;
	nsISupportsWString *		m_pSuccessLog;
	nsISupportsWString *		m_pErrorLog;
	PRUint32					m_totalSize;
	PRBool						m_doImport;
	AddressThreadData *			m_pThreadData;
	char *						m_pDestinationUri;
};

class AddressThreadData {
public:
	PRBool						driverAlive;
	PRBool						threadAlive;
	PRBool						abort;
	PRBool						fatalError;
	PRUint32					currentTotal;
	PRUint32					currentSize;
	nsISupportsArray *			books;
	nsIImportAddressBooks *		addressImport;
	nsIImportFieldMap *			fieldMap;
	nsISupportsWString *		successLog;
	nsISupportsWString *		errorLog;
	char *						pDestinationUri;

	AddressThreadData();
	~AddressThreadData();
	void DriverDelete();
	void ThreadDelete();
	void DriverAbort();
};


nsresult NS_NewGenericAddressBooks(nsIImportGeneric** aImportGeneric)
{
    NS_PRECONDITION(aImportGeneric != nsnull, "null ptr");
    if (! aImportGeneric)
        return NS_ERROR_NULL_POINTER;
	
	nsImportGenericAddressBooks *pGen = new nsImportGenericAddressBooks();

	if (pGen == nsnull)
		return NS_ERROR_OUT_OF_MEMORY;

	NS_ADDREF( pGen);
	nsresult rv = pGen->QueryInterface( NS_GET_IID(nsIImportGeneric), (void **)aImportGeneric);
	NS_RELEASE( pGen);
    
    return( rv);
}

nsImportGenericAddressBooks::nsImportGenericAddressBooks()
{
    NS_INIT_REFCNT();
	m_pInterface = nsnull;
	m_pBooks = nsnull;
	m_pSuccessLog = nsnull;
	m_pErrorLog = nsnull;
	m_totalSize = 0;
	m_doImport = PR_FALSE;
	m_pThreadData = nsnull;
	m_pDestinationUri = nsnull;
	m_pFieldMap = nsnull;

	m_pLocation = nsnull;
	m_autoFind = PR_FALSE;
	m_description = nsnull;
	m_gotLocation = PR_FALSE;
	m_found = PR_FALSE;
	m_userVerify = PR_FALSE;
}


nsImportGenericAddressBooks::~nsImportGenericAddressBooks()
{
	if (m_pThreadData) {
		m_pThreadData->DriverAbort();
		m_pThreadData = nsnull;
	}
	
	if (m_pDestinationUri)
		nsCRT::free( m_pDestinationUri);
	
	if (m_description)
		nsCRT::free( m_description);

	NS_IF_RELEASE( m_pFieldMap);
	NS_IF_RELEASE( m_pLocation);
	NS_IF_RELEASE( m_pInterface);
	NS_IF_RELEASE( m_pBooks);
	NS_IF_RELEASE( m_pSuccessLog);
	NS_IF_RELEASE( m_pErrorLog);
}



NS_IMPL_THREADSAFE_ISUPPORTS1(nsImportGenericAddressBooks, nsIImportGeneric)


NS_IMETHODIMP nsImportGenericAddressBooks::GetData(const char *dataId, nsISupports **_retval)
{
    NS_PRECONDITION(_retval != nsnull, "null ptr");
	if (!_retval)
		return NS_ERROR_NULL_POINTER;
	
	nsresult	rv;
	*_retval = nsnull;
	if (!nsCRT::strcasecmp( dataId, "addressInterface")) {
		*_retval = m_pInterface;
		NS_IF_ADDREF( m_pInterface);
	}
	
	if (!nsCRT::strcasecmp( dataId, "addressLocation")) {
		if (!m_pLocation)
			GetDefaultLocation();
		*_retval = m_pLocation;
		NS_IF_ADDREF( m_pLocation);
	}
	
	if (!nsCRT::strcasecmp( dataId, "addressBooks")) {
		if (!m_pLocation)
			GetDefaultLocation();
		if (!m_pBooks)
			GetDefaultBooks();
		*_retval = m_pBooks;
		NS_IF_ADDREF( m_pBooks);
	}
	
	if (!nsCRT::strcasecmp( dataId, "addressDestination")) {
		if (m_pDestinationUri) {
			nsCOMPtr<nsIURL>	url;
			rv = nsComponentManager::CreateInstance( kStandardUrlCID, nsnull, NS_GET_IID(nsIURL), getter_AddRefs( url));
			if (NS_SUCCEEDED( rv)) {
				url->SetSpec( m_pDestinationUri);
				*_retval = url;
				NS_IF_ADDREF( *_retval);
			}
		}
	}

	if (!nsCRT::strcasecmp( dataId, "fieldMap")) {
		if (m_pFieldMap) {
			*_retval = m_pFieldMap;
			m_pFieldMap->AddRef();
		}
		else {
			if (m_pInterface && m_pLocation) {
				PRBool	needsIt = PR_FALSE;
				m_pInterface->GetNeedsFieldMap( m_pLocation, &needsIt);
				if (needsIt) {
					GetDefaultFieldMap();
					if (m_pFieldMap) {
						*_retval = m_pFieldMap;
						m_pFieldMap->AddRef();
					}
				}
			}
		}
	}

	if (!nsCRT::strncasecmp( dataId, "sampleData-", 11)) {
		// extra the record number
		const char *pNum = dataId + 11;
		PRInt32	rNum = 0;
		while (*pNum) {
			rNum *= 10;
			rNum += (*pNum - '0');
			pNum++;
		}
		IMPORT_LOG1( "Requesting sample data #: %ld\n", (long)rNum);
		if (m_pInterface) {
			nsCOMPtr<nsISupportsWString>	data;
			rv = nsComponentManager::CreateInstance( kSupportsWStringCID, nsnull, NS_GET_IID(nsISupportsWString), getter_AddRefs( data));
			if (NS_FAILED( rv))
				return( rv);
			PRUnichar *	pData = nsnull;
			PRBool		found = PR_FALSE;
			rv = m_pInterface->GetSampleData( rNum, &found, &pData);
			if (NS_FAILED( rv))
				return( rv);
			if (found) {
				data->SetData( pData);
				*_retval = data;
				NS_ADDREF( *_retval);
			}
			nsCRT::free( pData);
		}
	}

	return( NS_OK);
}


NS_IMETHODIMP nsImportGenericAddressBooks::SetData( const char *dataId, nsISupports *item)
{
	NS_PRECONDITION(dataId != nsnull, "null ptr");
    if (!dataId)
        return NS_ERROR_NULL_POINTER;

	if (!nsCRT::strcasecmp( dataId, "addressInterface")) {
		NS_IF_RELEASE( m_pInterface);
		if (item)
			item->QueryInterface( NS_GET_IID(nsIImportAddressBooks), (void **) &m_pInterface);
	}
	if (!nsCRT::strcasecmp( dataId, "addressBooks")) {
		NS_IF_RELEASE( m_pBooks);
		if (item)
			item->QueryInterface( NS_GET_IID(nsISupportsArray), (void **) &m_pBooks);
	}
	
	if (!nsCRT::strcasecmp( dataId, "addressLocation")) {
		NS_IF_RELEASE( m_pLocation);
		if (item)
			item->QueryInterface( NS_GET_IID(nsIFileSpec), (void **) &m_pLocation);
		if (m_pInterface)
			m_pInterface->SetSampleLocation( m_pLocation);
	}

	if (!nsCRT::strcasecmp( dataId, "addressDestination")) {
		if (item) {
			nsCOMPtr<nsIURL> url;
			item->QueryInterface( NS_GET_IID(nsIURL), getter_AddRefs( url));
			if (url) {
				if (m_pDestinationUri)
					nsCRT::free( m_pDestinationUri);
				m_pDestinationUri = nsnull;
				url->GetSpec( &m_pDestinationUri);
			}
		}
	}

	if (!nsCRT::strcasecmp( dataId, "fieldMap")) {
		NS_IF_RELEASE( m_pFieldMap);
		if (item)
			item->QueryInterface( NS_GET_IID(nsIImportFieldMap), (void **) &m_pFieldMap);
	}
	
	return( NS_OK);
}

NS_IMETHODIMP nsImportGenericAddressBooks::GetStatus( const char *statusKind, PRInt32 *_retval)
{
	NS_PRECONDITION(statusKind != nsnull, "null ptr");
	NS_PRECONDITION(_retval != nsnull, "null ptr");
	if (!statusKind || !_retval)
		return( NS_ERROR_NULL_POINTER);
	
	*_retval = 0;

	if (!nsCRT::strcasecmp( statusKind, "isInstalled")) {
		GetDefaultLocation();
		*_retval = (PRInt32) m_found;
	}

	if (!nsCRT::strcasecmp( statusKind, "canUserSetLocation")) {
		GetDefaultLocation();
		*_retval = (PRInt32) m_userVerify;
	}
	
	if (!nsCRT::strcasecmp( statusKind, "autoFind")) {
		GetDefaultLocation();
		*_retval = (PRInt32) m_autoFind;
	}

	if (!nsCRT::strcasecmp( statusKind, "supportsMultiple")) {
		PRBool		multi = PR_FALSE;
		if (m_pInterface)
			m_pInterface->GetSupportsMultiple( &multi);
		*_retval = (PRInt32) multi;
	}
	
	if (!nsCRT::strcasecmp( statusKind, "needsFieldMap")) {
		PRBool		needs = PR_FALSE;
		if (m_pInterface && m_pLocation)
			m_pInterface->GetNeedsFieldMap( m_pLocation, &needs);
		*_retval = (PRInt32) needs;
	}

	return( NS_OK);
}

void nsImportGenericAddressBooks::GetDefaultLocation( void)
{
	if (!m_pInterface)
		return;
	
	if ((m_pLocation && m_gotLocation) || m_autoFind)
		return;
	
	if (m_description)
		nsCRT::free( m_description);
	m_description = nsnull;
	m_pInterface->GetAutoFind( &m_description, &m_autoFind);
	m_gotLocation = PR_TRUE;
	if (m_autoFind) {
		m_found = PR_TRUE;
		m_userVerify = PR_FALSE;
		return;
	}

	nsIFileSpec *	pLoc = nsnull;
	m_pInterface->GetDefaultLocation( &pLoc, &m_found, &m_userVerify);
	if (!m_pLocation)
		m_pLocation = pLoc;
	else {
		NS_IF_RELEASE( pLoc);
	}
}

void nsImportGenericAddressBooks::GetDefaultBooks( void)
{
	if (!m_pInterface || m_pBooks)
		return;
	
	if (!m_pLocation && !m_autoFind)
		return;

	nsresult rv = m_pInterface->FindAddressBooks( m_pLocation, &m_pBooks);
	if (NS_FAILED( rv)) {
		IMPORT_LOG0( "*** Error: FindAddressBooks failed\n");
	}
}

void nsImportGenericAddressBooks::GetDefaultFieldMap( void)
{
	if (!m_pInterface || !m_pLocation)
		return;
	
	NS_IF_RELEASE( m_pFieldMap);
	
	nsresult	rv;
	NS_WITH_SERVICE(nsIImportService, impSvc, kImportServiceCID, &rv);
	if (NS_FAILED(rv)) {
		IMPORT_LOG0( "*** Unable to get nsIImportService.\n");
		return;
	}

	rv = impSvc->CreateNewFieldMap( &m_pFieldMap);
	if (NS_FAILED( rv))
		return;
	
	PRInt32	sz = 0;
	rv = m_pFieldMap->GetNumMozFields( &sz);
	if (NS_SUCCEEDED( rv))
		rv = m_pFieldMap->DefaultFieldMap( sz);
	if (NS_SUCCEEDED( rv))
		rv = m_pInterface->InitFieldMap( m_pLocation, m_pFieldMap);
	if (NS_FAILED( rv)) {
		IMPORT_LOG0( "*** Error: Unable to initialize field map\n");
		NS_IF_RELEASE( m_pFieldMap);
	}
}


NS_IMETHODIMP nsImportGenericAddressBooks::WantsProgress(PRBool *_retval)
{
	NS_PRECONDITION(_retval != nsnull, "null ptr");
    if (!_retval)
        return NS_ERROR_NULL_POINTER;
	
	if (m_pThreadData) {
		m_pThreadData->DriverAbort();
		m_pThreadData = nsnull;
	}

	GetDefaultLocation();
	GetDefaultBooks();

	PRUint32		totalSize = 0;
	PRBool			result = PR_FALSE;

	if (m_pBooks) {
		PRUint32		count = 0;
		nsresult 		rv = m_pBooks->Count( &count);
		nsISupports *	pSupports;
		PRUint32		i;
		PRBool			import;
		PRUint32		size;
		
		for (i = 0; i < count; i++) {
			pSupports = m_pBooks->ElementAt( i);
			if (pSupports) {
				nsCOMPtr<nsISupports> interface( dont_AddRef( pSupports));
				nsCOMPtr<nsIImportABDescriptor> book( do_QueryInterface( pSupports));
				if (book) {
					import = PR_FALSE;
					size = 0;
					rv = book->GetImport( &import);
					if (import) {
						rv = book->GetSize( &size);
						result = PR_TRUE;
					}
					totalSize += size;
				}
			}
		}

		m_totalSize = totalSize;
	}
	
	m_doImport = result;

	*_retval = result;	

	return( NS_OK);
}

void nsImportGenericAddressBooks::SetLogs( nsString& success, nsString& error, nsISupportsWString *pSuccess, nsISupportsWString *pError)
{
	nsString	str;
	PRUnichar *	pStr = nsnull;
	if (pSuccess) {
		pSuccess->GetData( &pStr);
		if (pStr) {
			str = pStr;
			nsCRT::free( pStr);
			pStr = nsnull;
			str.Append( success);
			pSuccess->SetData( str.get());
		}
		else {
			pSuccess->SetData( success.get());			
		}
	}
	if (pError) {
		pError->GetData( &pStr);
		if (pStr) {
			str = pStr;
			nsCRT::free( pStr);
			str.Append( error);
			pError->SetData( str.get());
		}
		else {
			pError->SetData( error.get());			
		}
	}	
}

NS_IMETHODIMP nsImportGenericAddressBooks::BeginImport(nsISupportsWString *successLog, nsISupportsWString *errorLog, PRBool *_retval)
{
	NS_PRECONDITION(_retval != nsnull, "null ptr");
    if (!_retval)
        return NS_ERROR_NULL_POINTER;

	nsString	success;
	nsString	error;
	
	if (!m_doImport) {
		*_retval = PR_TRUE;
		nsImportStringBundle::GetStringByID( IMPORT_NO_ADDRBOOKS, success);
		SetLogs( success, error, successLog, errorLog);
		return( NS_OK);		
	}
	
	if (!m_pInterface || !m_pBooks) {
		nsImportStringBundle::GetStringByID( IMPORT_ERROR_AB_NOTINITIALIZED, error);
		SetLogs( success, error, successLog, errorLog);
		*_retval = PR_FALSE;
		return( NS_OK);
	}

	if (m_pThreadData) {
		m_pThreadData->DriverAbort();
		m_pThreadData = nsnull;
	}

	NS_IF_RELEASE( m_pSuccessLog);
	NS_IF_RELEASE( m_pErrorLog);
	m_pSuccessLog = successLog;
	m_pErrorLog = errorLog;
	NS_IF_ADDREF( m_pSuccessLog);
	NS_IF_ADDREF( m_pErrorLog);
	
	
	// kick off the thread to do the import!!!!
	m_pThreadData = new AddressThreadData();
	m_pThreadData->books = m_pBooks;
	NS_ADDREF( m_pBooks);
	m_pThreadData->addressImport = m_pInterface;
	NS_ADDREF( m_pInterface);
	m_pThreadData->fieldMap = m_pFieldMap;
	NS_IF_ADDREF( m_pFieldMap);
	m_pThreadData->errorLog = m_pErrorLog;
	NS_IF_ADDREF( m_pErrorLog);
	m_pThreadData->successLog = m_pSuccessLog;
	NS_IF_ADDREF( m_pSuccessLog);
	if (m_pDestinationUri)
		m_pThreadData->pDestinationUri = nsCRT::strdup( m_pDestinationUri);
				
	PRThread *pThread = PR_CreateThread( PR_USER_THREAD, &ImportAddressThread, m_pThreadData, 
									PR_PRIORITY_NORMAL, 
									PR_LOCAL_THREAD, 
									PR_UNJOINABLE_THREAD,
									0);
	if (!pThread) {
		m_pThreadData->ThreadDelete();
		m_pThreadData->DriverDelete();
		m_pThreadData = nsnull;
		*_retval = PR_FALSE;
		nsImportStringBundle::GetStringByID( IMPORT_ERROR_AB_NOTHREAD, error);
		SetLogs( success, error, successLog, errorLog);
	}
	else
		*_retval = PR_TRUE;
					
	return( NS_OK);

}


NS_IMETHODIMP nsImportGenericAddressBooks::ContinueImport(PRBool *_retval)
{
    NS_PRECONDITION(_retval != nsnull, "null ptr");
    if (!_retval)
        return NS_ERROR_NULL_POINTER;
	
	*_retval = PR_TRUE;
	if (m_pThreadData) {
		if (m_pThreadData->fatalError)
			*_retval = PR_FALSE;
	}

	return( NS_OK);
}


NS_IMETHODIMP nsImportGenericAddressBooks::GetProgress(PRInt32 *_retval)
{
	// This returns the progress from the the currently
	// running import mail or import address book thread.
    NS_PRECONDITION(_retval != nsnull, "null ptr");
    if (!_retval)
        return NS_ERROR_NULL_POINTER;

	if (!m_pThreadData || !(m_pThreadData->threadAlive)) {
		*_retval = 100;				
		return( NS_OK);	
	}
	
	PRUint32 sz = 0;
	if (m_pThreadData->currentSize && m_pInterface) {
		if (NS_FAILED( m_pInterface->GetImportProgress( &sz)))
			sz = 0;
	}
	
	if (m_totalSize)
		*_retval = ((m_pThreadData->currentTotal + sz) * 100) / m_totalSize;
	else
		*_retval = 0;
	
	// never return less than 5 so it looks like we are doing something!
	if (*_retval < 5)
		*_retval = 5;

	// as long as the thread is alive don't return completely
	// done.
	if (*_retval > 99)
		*_retval = 99;

	return( NS_OK);
}


NS_IMETHODIMP nsImportGenericAddressBooks::CancelImport(void)
{
	if (m_pThreadData) {
		m_pThreadData->abort = PR_TRUE;
		m_pThreadData->DriverAbort();
		m_pThreadData = nsnull;
	}

	return( NS_OK);
}


AddressThreadData::AddressThreadData()
{
	fatalError = PR_FALSE;
	driverAlive = PR_TRUE;
	threadAlive = PR_TRUE;
	abort = PR_FALSE;
	currentTotal = 0;
	currentSize = 0;
	books = nsnull;
	addressImport = nsnull;
	successLog = nsnull;
	errorLog = nsnull;
	pDestinationUri = nsnull;
	fieldMap = nsnull;
}

AddressThreadData::~AddressThreadData()
{
	if (pDestinationUri)
		nsCRT::free( pDestinationUri);

	NS_IF_RELEASE( books);
	NS_IF_RELEASE( addressImport);
	NS_IF_RELEASE( errorLog);
	NS_IF_RELEASE( successLog);
	NS_IF_RELEASE( fieldMap);
}

void AddressThreadData::DriverDelete( void)
{
	driverAlive = PR_FALSE;
	if (!driverAlive && !threadAlive)
		delete this;
}

void AddressThreadData::ThreadDelete()
{
	threadAlive = PR_FALSE;
	if (!driverAlive && !threadAlive)
		delete this;
}

void AddressThreadData::DriverAbort()
{
	if (abort && !threadAlive) {
		// FIXME: Do whatever is necessary to abort what has already been imported!
	}
	else
		abort = PR_TRUE;
	DriverDelete();
}


nsIAddrDatabase *GetAddressBookFromUri( const char *pUri)
{
	return( nsnull);
}

nsIAddrDatabase *GetAddressBook( const PRUnichar *name, PRBool makeNew)
{
	nsresult			rv = NS_OK;

	if (!makeNew) {
		// FIXME: How do I get the list of address books and look for a
		// specific name.  Major bogosity!
		// For now, assume we didn't find anything with that name
	}
	
	IMPORT_LOG0( "In GetAddressBook\n");

	NS_WITH_SERVICE( nsIProxyObjectManager, proxyMgr, kProxyObjectManagerCID, &rv);
	if (NS_FAILED( rv)) {
		IMPORT_LOG0( "*** Error: Unable to get proxy manager\n");
		return( nsnull);
	}

	nsIAddrDatabase *	pDatabase = nsnull;

	/* Get the profile directory */
	// Note to Candice: This should return an nsIFileSpec, not a nsFileSpec
	nsFileSpec *					dbPath = nsnull;

	NS_WITH_PROXIED_SERVICE(nsIAddrBookSession, abSession, kAddrBookSessionCID, NS_UI_THREAD_EVENTQ, &rv); 
	
	if (NS_SUCCEEDED(rv))
		abSession->GetUserProfileDirectory(&dbPath);
	if (dbPath) {
		// Create a new address book file - we don't care what the file
		// name is, as long as it's unique
		(*dbPath) += "impab.mab";
		(*dbPath).MakeUnique();
		
		IMPORT_LOG0( "Getting the address database factory\n");

		NS_WITH_PROXIED_SERVICE(nsIAddrDatabase, addrDBFactory, kAddressBookDBCID, NS_UI_THREAD_EVENTQ, &rv);
		if (NS_SUCCEEDED(rv) && addrDBFactory) {
			
			IMPORT_LOG0( "Opening the new address book\n");
			rv = addrDBFactory->Open( dbPath, PR_TRUE, &pDatabase, PR_TRUE);
		}
	}
	else {
		IMPORT_LOG0( "Failed to get the user profile directory from the address book session\n");
	}

	
	if (pDatabase) {
		// We made a database, add it to the UI?!?!?!?!?!?!
		// This is major bogosity again!  Why doesn't the address book
		// just handle this properly for me?  Uggggg...
		
		NS_WITH_PROXIED_SERVICE(nsIRDFService, rdfService, kRDFServiceCID, NS_UI_THREAD_EVENTQ, &rv);
		if (NS_SUCCEEDED(rv)) {
			nsCOMPtr<nsIRDFResource>	parentResource;
			char *parentUri = PR_smprintf( "%s", kAllDirectoryRoot);
			rv = rdfService->GetResource( parentUri, getter_AddRefs(parentResource));
			nsCOMPtr<nsIAbDirectory> parentDir;
			rv = proxyMgr->GetProxyForObject( NS_UI_THREAD_EVENTQ, NS_GET_IID( nsIAbDirectory),
										parentResource, PROXY_SYNC | PROXY_ALWAYS, getter_AddRefs( parentDir));
			if (parentUri)
				PR_smprintf_free(parentUri);
			if (parentDir)
		       	{
				PRUint32 prefCount = 2;
				char **prefNames = (char **) nsMemory::Alloc(prefCount * (sizeof (char *)));
				if (!prefNames)
					return nsnull;

				PRUnichar ** prefValues = (PRUnichar **) nsMemory::Alloc(prefCount * (sizeof(PRUnichar *)));
				if (!prefValues)
					return nsnull;
					
				prefNames[0] = PR_smprintf("description");
				prefNames[1] = PR_smprintf("fileName");
					
				prefValues[0] = (PRUnichar *)name;
	
				char *fileName = (*dbPath).GetLeafName();
				PRInt32 descLength = PL_strlen(fileName);
				NS_ConvertUTF8toUCS2 temp((const char *)fileName, descLength);
				prefValues[1] = nsCRT::strdup(temp.get());

				parentDir->CreateNewDirectory((unsigned int )prefCount, (const char** )prefNames, (const PRUnichar** )prefValues);

          delete dbPath;

				if (fileName)
					nsCRT::free(fileName);
				if (prefNames[0])
					PR_smprintf_free(prefNames[0]);
				if (prefNames[1])
					PR_smprintf_free(prefNames[1]);
				if (prefNames)
					nsMemory::Free(prefNames);
				if (prefValues)
					nsMemory::Free(prefValues);
				
			}

			IMPORT_LOG0( "Added new address book to the UI\n");
		}
	}
	
	return( pDatabase);
}

void nsImportGenericAddressBooks::ReportError( PRUnichar *pName, nsString *pStream)
{
	if (!pStream)
		return;
	// load the error string
	PRUnichar *pFmt = nsImportStringBundle::GetStringByID( IMPORT_ERROR_GETABOOK);
	PRUnichar *pText = nsTextFormatter::smprintf( pFmt, pName);
	pStream->Append( pText);
	nsTextFormatter::smprintf_free( pText);
	nsImportStringBundle::FreeString( pFmt);
	pStream->AppendWithConversion( NS_LINEBREAK);
}

PR_STATIC_CALLBACK( void) ImportAddressThread( void *stuff)
{
	IMPORT_LOG0( "In Begin ImportAddressThread\n");

	AddressThreadData *pData = (AddressThreadData *)stuff;
	PRUint32	count = 0;
	nsresult 	rv = pData->books->Count( &count);
	
	nsISupports *				pSupports;
	PRUint32					i;
	PRBool						import;
	PRUint32					size;
	nsCOMPtr<nsIAddrDatabase>	destDB( getter_AddRefs( GetAddressBookFromUri( pData->pDestinationUri)));
	nsIAddrDatabase *			pDestDB = nsnull;
	
	nsString					success;
	nsString					error;

	for (i = 0; (i < count) && !(pData->abort); i++) {
		pSupports = pData->books->ElementAt( i);
		if (pSupports) {
			nsCOMPtr<nsISupports> interface( dont_AddRef( pSupports));
			nsCOMPtr<nsIImportABDescriptor> book( do_QueryInterface( pSupports));
			if (book) {
				import = PR_FALSE;
				size = 0;
				rv = book->GetImport( &import);
				if (import)
					rv = book->GetSize( &size);
				
				if (size && import) {
					PRUnichar *pName;
					book->GetPreferredName( &pName);
					if (destDB) {
						destDB->QueryInterface( NS_GET_IID(nsIAddrDatabase), (void **)&pDestDB);
					}
					else {
						pDestDB = GetAddressBook( pName, PR_TRUE);
					}


					PRBool fatalError = PR_FALSE;
					pData->currentSize = size;
					if (pDestDB) {
						PRUnichar *pSuccess = nsnull;
						PRUnichar *pError = nsnull;

						/*
						if (pData->fieldMap) {
							PRInt32		sz = 0;
							PRInt32		mapIndex;
							PRBool		active;
							pData->fieldMap->GetMapSize( &sz);
							IMPORT_LOG1( "**** Field Map Size: %d\n", (int) sz);
							for (PRInt32 i = 0; i < sz; i++) {
								pData->fieldMap->GetFieldMap( i, &mapIndex);
								pData->fieldMap->GetFieldActive( i, &active);
								IMPORT_LOG3( "Field map #%d: index=%d, active=%d\n", (int) i, (int) mapIndex, (int) active);
							}
						}
						*/

						rv = pData->addressImport->ImportAddressBook(	book, 
																	pDestDB, // destination
																	pData->fieldMap, // fieldmap
																	&pError,
																	&pSuccess,
																	&fatalError);
						if (pSuccess) {
							success.Append( pSuccess);
							nsCRT::free( pSuccess);
						}
						if (pError) {
							error.Append( pError);
							nsCRT::free( pError);
						}
					}
					else {
						nsImportGenericAddressBooks::ReportError( pName, &error);
					}

					nsCRT::free( pName);

					pData->currentSize = 0;
					pData->currentTotal += size;
					
					if (!destDB) {
						pDestDB->Close( PR_TRUE);
					}

					NS_IF_RELEASE( pDestDB);

					if (fatalError) {
						pData->fatalError = PR_TRUE;
						break;
					}
				}
			}
		}

		if (destDB) {
			destDB->Close( PR_TRUE);
		}
	}

	
	nsImportGenericAddressBooks::SetLogs( success, error, pData->successLog, pData->errorLog);

	if (pData->abort || pData->fatalError) {
		// FIXME: do what is necessary to get rid of what has been imported so far.
		// Nothing if we went into an existing address book!  Otherwise, delete
		// the ones we created?
	}

	pData->ThreadDelete();	
}
