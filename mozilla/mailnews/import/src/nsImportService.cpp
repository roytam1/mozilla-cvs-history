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


#define NS_IMPL_IDS
#include "nsICharsetConverterManager.h"
#include "nsICharsetAlias.h"
#include "nsIPlatformCharset.h"
#undef NS_IMPL_IDS

#include "nsCRT.h"
#include "nsString.h"
#include "nsIComponentManager.h"
#include "nsIServiceManager.h"
#include "nsMemory.h"
#include "nsIEnumerator.h"
#include "nsIImportModule.h"
#include "nsIImportService.h"
#include "nsImportMailboxDescriptor.h"
#include "nsImportABDescriptor.h"
#include "nsIImportGeneric.h"
#include "nsImportFieldMap.h"
#include "nsICategoryManager.h"
#include "nsXPIDLString.h"
#include "nsISupportsPrimitives.h"
#include "nsImportStringBundle.h"
#include "plstr.h"
#include "prmem.h"
#include "ImportDebug.h"


static NS_DEFINE_CID(kComponentManagerCID, 	NS_COMPONENTMANAGER_CID);
static NS_DEFINE_CID(kImportServiceCID,		NS_IMPORTSERVICE_CID);
static NS_DEFINE_IID(kImportModuleIID,		NS_IIMPORTMODULE_IID);
static NS_DEFINE_CID(kCharsetConverterManagerCID, NS_ICHARSETCONVERTERMANAGER_CID);


static nsIImportService *	gImportService = nsnull;

static const char *	kWhitespace = "\b\t\r\n ";


class nsImportModuleList;

class nsImportService : public nsIImportService
{
public:

	nsImportService();
	virtual ~nsImportService();
	
	NS_DECL_ISUPPORTS

	NS_DECL_NSIIMPORTSERVICE

private:
	nsresult	LoadModuleInfo( const char *pClsId, const char *pSupports);
	nsresult	DoDiscover( void);

private:
	nsImportModuleList *	m_pModules;
	PRBool					m_didDiscovery;	
	nsString				m_sysCharset;
	nsIUnicodeDecoder *		m_pDecoder;
};

// extern nsresult NS_NewImportService(nsIImportService** aImportService);

class ImportModuleDesc {
public:
	ImportModuleDesc() { m_pModule = nsnull;}
	~ImportModuleDesc() { ReleaseModule();	}
	
	void	SetCID( const nsCID& cid) { m_cid = cid;}
	void	SetName( const PRUnichar *pName) { m_name = pName;}
	void	SetDescription( const PRUnichar *pDesc) { m_description = pDesc;}
	void	SetSupports( const char *pSupports) { m_supports = pSupports;}
	
	nsCID			GetCID( void) { return( m_cid);}
	const PRUnichar *GetName( void) { return( m_name.GetUnicode());}
	const PRUnichar *GetDescription( void) { return( m_description.GetUnicode());}
	const char *	GetSupports( void) { return( (const char *)m_supports);}
	
	nsIImportModule *	GetModule( PRBool keepLoaded = PR_FALSE); // Adds ref
	void				ReleaseModule( void);
	
	PRBool			SupportsThings( const char *pThings);

private:
	nsCID				m_cid;
	nsString			m_name;
	nsString			m_description;
	nsCString			m_supports;
	nsIImportModule *	m_pModule;
};

class nsImportModuleList {
public:
	nsImportModuleList() { m_pList = nsnull; m_alloc = 0; m_count = 0;}
	~nsImportModuleList() { ClearList(); }

	void	AddModule( const nsCID& cid, const char *pSupports, const PRUnichar *pName, const PRUnichar *pDesc);

	void	ClearList( void);
	
	PRInt32	GetCount( void) { return( m_count);}
	
	ImportModuleDesc *	GetModuleDesc( PRInt32 idx) 
		{ if ((idx < 0) || (idx >= m_count)) return( nsnull); else return( m_pList[idx]);}
	
private:
	
private:
	ImportModuleDesc **	m_pList;
	PRInt32				m_alloc;
	PRInt32				m_count;
};

////////////////////////////////////////////////////////////////////////
NS_IMETHODIMP NS_NewImportService( nsISupports* aOuter, REFNSIID aIID, void **aResult)
{
    NS_PRECONDITION(aResult != nsnull, "null ptr");
    if (! aResult)
        return( NS_ERROR_NULL_POINTER);
    if (aOuter) {                                                    
        *aResult = nsnull;
        return( NS_ERROR_NO_AGGREGATION);
    }

	if (!gImportService) {	
    	gImportService = new nsImportService();
    	if (! gImportService)
        	return( NS_ERROR_OUT_OF_MEMORY);
	}       

    NS_ADDREF( gImportService);

	nsresult rv = gImportService->QueryInterface(aIID, aResult);
    if (NS_FAILED(rv)) {
        *aResult = nsnull;
    }
    NS_RELEASE( gImportService);
	
	nsImportStringBundle::GetStringBundle();

	return( rv);
}


////////////////////////////////////////////////////////////////////////


nsImportService::nsImportService() : m_pModules( nsnull)
{
    NS_INIT_REFCNT();

	IMPORT_LOG0( "* nsImport Service Created\n");

	m_didDiscovery = PR_FALSE;
	m_pDecoder = nsnull;

	// Go ahead an initialize the charset converter to avoid any 
	// thread issues later.
	nsString	str;
	SystemStringToUnicode( "Dummy", str);
}


nsImportService::~nsImportService()
{
	if (m_pDecoder)
		NS_RELEASE( m_pDecoder);

	gImportService = nsnull;

    if (m_pModules != nsnull)
        delete m_pModules;

	IMPORT_LOG0( "* nsImport Service Deleted\n");

}



NS_IMPL_THREADSAFE_ISUPPORTS(nsImportService, NS_GET_IID(nsIImportService));


NS_IMETHODIMP nsImportService::DiscoverModules( void)
{
	m_didDiscovery = PR_FALSE;
	return( DoDiscover());
}

NS_IMETHODIMP nsImportService::CreateNewFieldMap( nsIImportFieldMap **_retval)
{
  nsresult rv;
  rv = nsImportFieldMap::Create( nsnull, NS_GET_IID(nsIImportFieldMap), (void**)_retval);
  return rv;
}

NS_IMETHODIMP nsImportService::CreateNewMailboxDescriptor( nsIImportMailboxDescriptor **_retval)
{
  nsresult rv;
  rv = nsImportMailboxDescriptor::Create( nsnull, NS_GET_IID(nsIImportMailboxDescriptor), (void**)_retval);
  return rv;
}

NS_IMETHODIMP nsImportService::CreateNewABDescriptor(nsIImportABDescriptor **_retval)
{
  nsresult rv;
  rv = nsImportABDescriptor::Create( nsnull, NS_GET_IID(nsIImportABDescriptor), (void**)_retval);
  return rv;
}

NS_IMETHODIMP nsImportService::SystemStringToUnicode(const char *sysStr, nsString & uniStr)
{
  
	nsresult	rv;
	if (m_sysCharset.IsEmpty()) {
		nsCOMPtr <nsIPlatformCharset> platformCharset = do_GetService(NS_PLATFORMCHARSET_PROGID, &rv);
		if (NS_SUCCEEDED(rv)) 
			rv = platformCharset->GetCharset(kPlatformCharsetSel_FileName, m_sysCharset);

		if (NS_FAILED(rv)) 
			m_sysCharset.AssignWithConversion("ISO-8859-1");
	}

	if (!sysStr) {
		uniStr.Truncate();
		return( NS_OK);
	}
  
	if (*sysStr == '\0') {
		uniStr.Truncate();
		return( NS_OK);
	}
	

	if (m_sysCharset.IsEmpty() ||
		m_sysCharset.EqualsIgnoreCase("us-ascii") ||
		m_sysCharset.EqualsIgnoreCase("ISO-8859-1")) {
		uniStr.AssignWithConversion( sysStr);
		return( NS_OK);
	}
	


	
	if (!m_pDecoder) {
		nsAutoString convCharset;

		// Resolve charset alias
		NS_WITH_SERVICE( nsICharsetAlias, calias, kCharsetAliasCID, &rv); 
		if (NS_SUCCEEDED( rv)) {
			nsAutoString aAlias( m_sysCharset);
			if (aAlias.Length()) {
				rv = calias->GetPreferred( aAlias, convCharset);
			}
		}

		if (NS_FAILED( rv)) {
			IMPORT_LOG0( "*** Error getting charset alias to convert to uinicode\n");
			uniStr.AssignWithConversion( sysStr);
			return( rv);
		}

		NS_WITH_SERVICE( nsICharsetConverterManager, ccm, kCharsetConverterManagerCID, &rv); 

		if (NS_SUCCEEDED( rv) && (nsnull != ccm)) {
			// get an unicode converter
			rv = ccm->GetUnicodeDecoder( &convCharset, &m_pDecoder);
		}	    
	}

	if (m_pDecoder) {
		PRInt32		srcLen = PL_strlen( sysStr);
		PRUnichar *	unichars;
		PRInt32 unicharLength = 0;
		rv = m_pDecoder->GetMaxLength( sysStr, srcLen, &unicharLength);
		// allocale an output buffer
		unichars = (PRUnichar *) PR_Malloc(unicharLength * sizeof(PRUnichar));
		if (unichars != nsnull) {
			// convert to unicode
			rv = m_pDecoder->Convert( sysStr, &srcLen, unichars, &unicharLength);
			uniStr.Assign(unichars, unicharLength);
			PR_Free(unichars);
		}
		else
			rv = NS_ERROR_OUT_OF_MEMORY;
	}
	
	if (NS_FAILED( rv))
		uniStr.AssignWithConversion( sysStr);

	return( rv);
}


extern nsresult NS_NewGenericMail(nsIImportGeneric** aImportGeneric);

NS_IMETHODIMP nsImportService::CreateNewGenericMail(nsIImportGeneric **_retval)
{
    NS_PRECONDITION(_retval != nsnull, "null ptr");
    if (! _retval)
        return NS_ERROR_NULL_POINTER;

	return( NS_NewGenericMail( _retval));
}

extern nsresult NS_NewGenericAddressBooks(nsIImportGeneric** aImportGeneric);

NS_IMETHODIMP nsImportService::CreateNewGenericAddressBooks(nsIImportGeneric **_retval)
{
    NS_PRECONDITION(_retval != nsnull, "null ptr");
    if (! _retval)
        return NS_ERROR_NULL_POINTER;

	return( NS_NewGenericAddressBooks( _retval));
}


NS_IMETHODIMP nsImportService::GetModuleCount( const char *filter, PRInt32 *_retval)
{
    NS_PRECONDITION(_retval != nsnull, "null ptr");
    if (! _retval)
        return NS_ERROR_NULL_POINTER;

	DoDiscover();
	
	if (m_pModules != nsnull) {
		ImportModuleDesc *	pDesc;
		PRInt32	count = 0;
		for (PRInt32 i = 0; i < m_pModules->GetCount(); i++) {
			pDesc = m_pModules->GetModuleDesc( i);
			if (pDesc->SupportsThings( filter))
				count++;
		}
		*_retval = count;
	}
	else
		*_retval = 0;
		
	return( NS_OK);
}
	
NS_IMETHODIMP nsImportService::GetModuleWithCID( const nsCID& cid, nsIImportModule **ppModule)
{
    NS_PRECONDITION(ppModule != nsnull, "null ptr");
    if (! ppModule)
        return NS_ERROR_NULL_POINTER;
	
	*ppModule = nsnull;
	nsresult rv = DoDiscover();
	if (NS_FAILED( rv)) return( rv);
	if (m_pModules == nsnull) return( NS_ERROR_FAILURE);
	PRInt32	cnt = m_pModules->GetCount();
	ImportModuleDesc *pDesc;
	for (PRInt32 i = 0; i < cnt; i++) {
		pDesc = m_pModules->GetModuleDesc( i);
		if (!pDesc) return( NS_ERROR_FAILURE);
		if (pDesc->GetCID().Equals( cid)) {
			*ppModule = pDesc->GetModule();
			
			IMPORT_LOG0( "* nsImportService::GetSpecificModule - attempted to load module\n");
			
			if (*ppModule == nsnull)
				return( NS_ERROR_FAILURE);
			else
				return( NS_OK);
		}
	}
	
	IMPORT_LOG0( "* nsImportService::GetSpecificModule - module not found\n");
	
	return( NS_ERROR_NOT_AVAILABLE);
}

NS_IMETHODIMP nsImportService::GetModuleInfo( const char *filter, PRInt32 index, PRUnichar **name, PRUnichar **moduleDescription)
{
    NS_PRECONDITION(name != nsnull, "null ptr");
    NS_PRECONDITION(moduleDescription != nsnull, "null ptr");
    if (!name || !moduleDescription)
        return NS_ERROR_NULL_POINTER;
    
	*name = nsnull;
	*moduleDescription = nsnull;

    DoDiscover();
    if (!m_pModules)
		return( NS_ERROR_FAILURE);
	
	if ((index < 0) || (index >= m_pModules->GetCount()))
		return( NS_ERROR_FAILURE);
	
	ImportModuleDesc *	pDesc;
	PRInt32	count = 0;
	for (PRInt32 i = 0; i < m_pModules->GetCount(); i++) {
		pDesc = m_pModules->GetModuleDesc( i);
		if (pDesc->SupportsThings( filter)) {
			if (count == index) {
				*name = nsCRT::strdup( pDesc->GetName());
				*moduleDescription = nsCRT::strdup( pDesc->GetDescription());
				return( NS_OK);
			}
			else
				count++;
		}
	}

	return( NS_ERROR_FAILURE);
}

NS_IMETHODIMP nsImportService::GetModuleName(const char *filter, PRInt32 index, PRUnichar **_retval)
{
    NS_PRECONDITION(_retval != nsnull, "null ptr");
    if (!_retval)
        return NS_ERROR_NULL_POINTER;
	
	*_retval = nsnull;
	    
    DoDiscover();
    if (!m_pModules)
		return( NS_ERROR_FAILURE);
	
	if ((index < 0) || (index >= m_pModules->GetCount()))
		return( NS_ERROR_FAILURE);
	
	ImportModuleDesc *	pDesc;
	PRInt32	count = 0;
	for (PRInt32 i = 0; i < m_pModules->GetCount(); i++) {
		pDesc = m_pModules->GetModuleDesc( i);
		if (pDesc->SupportsThings( filter)) {
			if (count == index) {
				*_retval = nsCRT::strdup( pDesc->GetName());
				return( NS_OK);
			}
			else
				count++;
		}
	}
	
	return( NS_ERROR_FAILURE);
}


NS_IMETHODIMP nsImportService::GetModuleDescription(const char *filter, PRInt32 index, PRUnichar **_retval)
{
    NS_PRECONDITION(_retval != nsnull, "null ptr");
    if (!_retval)
        return NS_ERROR_NULL_POINTER;
    
	*_retval = nsnull;

    DoDiscover();
    if (!m_pModules)
		return( NS_ERROR_FAILURE);
	
	if ((index < 0) || (index >= m_pModules->GetCount()))
		return( NS_ERROR_FAILURE);
	
	ImportModuleDesc *	pDesc;
	PRInt32	count = 0;
	for (PRInt32 i = 0; i < m_pModules->GetCount(); i++) {
		pDesc = m_pModules->GetModuleDesc( i);
		if (pDesc->SupportsThings( filter)) {
			if (count == index) {
				*_retval = nsCRT::strdup( pDesc->GetDescription());
				return( NS_OK);
			}
			else
				count++;
		}
	}
	
	return( NS_ERROR_FAILURE);
}



NS_IMETHODIMP nsImportService::GetModule( const char *filter, PRInt32 index, nsIImportModule **_retval)
{
    NS_PRECONDITION(_retval != nsnull, "null ptr");
    if (!_retval)
        return NS_ERROR_NULL_POINTER;
	*_retval = nsnull;

    DoDiscover();
    if (!m_pModules)
		return( NS_ERROR_FAILURE);
		
	if ((index < 0) || (index >= m_pModules->GetCount()))
		return( NS_ERROR_FAILURE);
	
	ImportModuleDesc *	pDesc;
	PRInt32	count = 0;
	for (PRInt32 i = 0; i < m_pModules->GetCount(); i++) {
		pDesc = m_pModules->GetModuleDesc( i);
		if (pDesc->SupportsThings( filter)) {
			if (count == index) {
				*_retval = pDesc->GetModule();
				break;
			}
			else
				count++;
		}
	}
	if (! (*_retval))
		return( NS_ERROR_FAILURE);

	return( NS_OK);
}


nsresult nsImportService::DoDiscover( void)
{	
	if (m_didDiscovery)
		return( NS_OK);
	
	if (m_pModules != nsnull)
		m_pModules->ClearList();
		    
    nsresult rv;
	
	nsCOMPtr<nsICategoryManager> catMan = do_GetService( NS_CATEGORYMANAGER_PROGID, &rv);
	if (NS_FAILED( rv)) return( rv);
    
	nsCOMPtr<nsISimpleEnumerator> e;
	rv = catMan->EnumerateCategory( "mailnewsimport", getter_AddRefs( e));
	if (NS_FAILED( rv)) return( rv);
	nsCOMPtr<nsISupportsString> progid;
	rv = e->GetNext( getter_AddRefs( progid));
	while (NS_SUCCEEDED( rv) && progid) {
		nsXPIDLCString	progIdStr;
		progid->ToString( getter_Copies( progIdStr));
		nsXPIDLCString	supportsStr;
		rv = catMan->GetCategoryEntry( "mailnewsimport", progIdStr, getter_Copies( supportsStr));
		if (NS_SUCCEEDED( rv)) {
			LoadModuleInfo( progIdStr, supportsStr);
		}
		rv = e->GetNext( getter_AddRefs( progid));
	}

	m_didDiscovery = PR_TRUE;
	
    return NS_OK;
}

nsresult nsImportService::LoadModuleInfo( const char *pClsId, const char *pSupports)
{
	if (!pClsId || !pSupports)
		return( NS_OK);

	if (m_pModules == nsnull)
		m_pModules = new nsImportModuleList();
		
	// load the component and get all of the info we need from it....
	// then call AddModule
	nsresult	rv;
   	NS_WITH_SERVICE( nsIComponentManager, compMgr, kComponentManagerCID, &rv);
	if (NS_FAILED(rv)) return rv;
	
	nsCID				clsId;
	clsId.Parse( pClsId);
	nsIImportModule *	module;
	rv = compMgr->CreateInstance( clsId, nsnull, kImportModuleIID, (void **) &module);
	if (NS_FAILED(rv)) return rv;
	
	nsString	theTitle;	
	nsString	theDescription;
	PRUnichar *	pName;
	rv = module->GetName( &pName);
	if (NS_SUCCEEDED( rv)) {
		theTitle = pName;
		delete [] pName;
	}
	else
		theTitle.AssignWithConversion("Unknown");
		
	rv = module->GetDescription( &pName);
	if (NS_SUCCEEDED( rv)) {
		theDescription = pName;
		delete [] pName;
	}
	else
		theDescription.AssignWithConversion("Unknown description");
	
	// call the module to get the info we need
	m_pModules->AddModule( clsId, pSupports, theTitle.GetUnicode(), theDescription.GetUnicode());
	
	module->Release();
	
	return NS_OK;
}


nsIImportModule *ImportModuleDesc::GetModule( PRBool keepLoaded)
{
	if (m_pModule) {
		m_pModule->AddRef();
		return( m_pModule);
	}
	
	nsresult	rv;
   	NS_WITH_SERVICE( nsIComponentManager, compMgr, kComponentManagerCID, &rv);
	if (NS_FAILED(rv)) return nsnull;
	
	rv = compMgr->CreateInstance( m_cid, nsnull, kImportModuleIID, (void **) &m_pModule);
	if (NS_FAILED(rv)) {
		m_pModule = nsnull;
		return nsnull;
	}
	
	if (keepLoaded) {
		m_pModule->AddRef();
		return( m_pModule);
	}
	else {
		nsIImportModule *pModule = m_pModule;
		m_pModule = nsnull;
		return( pModule);
	}
}

void ImportModuleDesc::ReleaseModule( void)
{
	if (m_pModule) {
		m_pModule->Release();
		m_pModule = nsnull;
	}
}

PRBool ImportModuleDesc::SupportsThings( const char *pThings)
{
	if (!pThings)
		return( PR_TRUE);
	if (!(*pThings))
		return( PR_TRUE);

	nsCString	thing = pThings;
	nsCString	item;
	PRInt32		idx;
	
	while ((idx = thing.FindChar( ',')) != -1) {
		thing.Left( item, idx);
		item.Trim( kWhitespace);
		item.ToLowerCase();
		if (item.Length() && (m_supports.Find( item) == -1))
			return( PR_FALSE);
		thing.Right( item, thing.Length() - idx - 1);
		thing = item;
	}
	thing.Trim( kWhitespace);
	thing.ToLowerCase();
	if (thing.Length() && (m_supports.Find( thing) == -1))
		return( PR_FALSE);
	
	return( PR_TRUE);
}

void nsImportModuleList::ClearList( void)
{
	if (m_pList != nsnull) {
		for (int i = 0; i < m_count; i++) {
			if (m_pList[i] != nsnull)
				delete m_pList[i];	
			m_pList[i] = nsnull;
		}
		m_count = 0;
		nsMemory::Free( m_pList);
		m_pList = nsnull;
		m_alloc = 0;
	}
	
}

void nsImportModuleList::AddModule( const nsCID& cid, const char *pSupports, const PRUnichar *pName, const PRUnichar *pDesc)
{
	if (m_pList == nsnull) {
		m_alloc = 10;
		m_pList = new ImportModuleDesc *[m_alloc];
		m_count = 0;
		nsCRT::memset( m_pList, 0, sizeof( ImportModuleDesc *) * m_alloc);
	}
	
	if (m_count == m_alloc) {
		ImportModuleDesc **pList = new ImportModuleDesc *[m_alloc + 10];
		nsCRT::memset( &(pList[m_alloc]), 0, sizeof( ImportModuleDesc *) * 10);
		nsCRT::memcpy( pList, m_pList, sizeof( ImportModuleDesc *) * m_alloc);
		delete [] m_pList;
		m_pList = pList;
		m_alloc += 10;
	}
	
	m_pList[m_count] = new ImportModuleDesc();
	m_pList[m_count]->SetCID( cid);
	m_pList[m_count]->SetSupports( pSupports);
	m_pList[m_count]->SetName( pName);
	m_pList[m_count]->SetDescription( pDesc);
	
	m_count++;
#ifdef IMPORT_DEBUG
	nsCString 	name; name.AssignWithConversion( pName);
	nsCString	desc; desc.AssignWithConversion( pDesc);
	IMPORT_LOG3( "* nsImportService registered import module: %s, %s, %s\n", (const char *)name, (const char *)desc, pSupports);
#endif
}
