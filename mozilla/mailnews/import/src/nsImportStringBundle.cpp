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
 * Copyright (C) 1999 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */
#include "prprf.h"
#include "prmem.h"
#include "nsCOMPtr.h"
#include "nsIStringBundle.h"
#include "nsImportStringBundle.h"
#include "nsIServiceManager.h"
#include "nsProxyObjectManager.h"
#include "nsIURI.h"

/* This is the next generation string retrieval call */
static NS_DEFINE_CID(kStringBundleServiceCID, NS_STRINGBUNDLESERVICE_CID);
static NS_DEFINE_IID(kProxyObjectManagerCID, NS_PROXYEVENT_MANAGER_CID);

#define IMPORT_MSGS_URL       "chrome://messenger/locale/importMsgs.properties"

nsIStringBundle *	nsImportStringBundle::m_pBundle = nsnull;

nsIStringBundle *nsImportStringBundle::GetStringBundle( void)
{
	if (m_pBundle)
		return( m_pBundle);

	nsresult			rv;
	char*				propertyURL = IMPORT_MSGS_URL;
	nsIStringBundle*	sBundle = nsnull;


	NS_WITH_SERVICE(nsIStringBundleService, sBundleService, kStringBundleServiceCID, &rv); 
	if (NS_SUCCEEDED(rv) && (nsnull != sBundleService)) {
		nsILocale   *		locale = nsnull;
		rv = sBundleService->CreateBundle(propertyURL, locale, &sBundle);
	}
	
	m_pBundle = sBundle;
	return( sBundle);
}

nsIStringBundle *nsImportStringBundle::GetStringBundleProxy( void)
{
	if (!m_pBundle)
		return( nsnull);

	nsIStringBundle *strProxy = nsnull;
	nsresult rv;
	// create a proxy object if we aren't on the same thread?
	NS_WITH_SERVICE( nsIProxyObjectManager, proxyMgr, kProxyObjectManagerCID, &rv);
	if (NS_SUCCEEDED(rv)) {
		rv = proxyMgr->GetProxyObject( NS_UI_THREAD_EVENTQ, NS_GET_IID(nsIStringBundle),
										m_pBundle, PROXY_SYNC | PROXY_ALWAYS, (void **) &strProxy);
	}

	return( strProxy);
}

void nsImportStringBundle::GetStringByID( PRInt32 stringID, nsString& result, nsIStringBundle *pBundle)
{
	
	PRUnichar *ptrv = GetStringByID( stringID, pBundle);	
	result = ptrv;
	FreeString( ptrv);
}

PRUnichar *nsImportStringBundle::GetStringByID(PRInt32 stringID, nsIStringBundle *pBundle)
{
	if (!pBundle) {
		pBundle = GetStringBundle();
	}
	
	if (pBundle) {
		PRUnichar *ptrv = nsnull;
		nsresult rv = pBundle->GetStringFromID(stringID, &ptrv);
				
		if (NS_SUCCEEDED( rv) && ptrv)
			return( ptrv);
	}

	nsString resultString( "[StringID ");
	resultString.Append(stringID, 10);
	resultString += "?]";

	return( resultString.ToNewUnicode());
}

void nsImportStringBundle::Cleanup( void)
{
	if (m_pBundle)
		m_pBundle->Release();
	m_pBundle = nsnull;
}

