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
 * Contributor(s): Paul Sandoz
 */

#include "nsAbMDBCardProperty.h"	 
#include "nsIRDFService.h"
#include "nsIServiceManager.h"
#include "nsRDFCID.h"
#include "nsXPIDLString.h"
#include "nsAbBaseCID.h"
#include "prmem.h"	 
#include "prlog.h"	 
#include "prprf.h"	 
#include "rdf.h"
#include "nsCOMPtr.h"

#include "nsAddrDatabase.h"
#include "nsIAddrBookSession.h"
#include "nsIPref.h"
#include "nsIAddressBook.h"

static NS_DEFINE_CID(kAddressBookDBCID, NS_ADDRDATABASE_CID);
static NS_DEFINE_CID(kAddrBookSessionCID, NS_ADDRBOOKSESSION_CID);
static NS_DEFINE_CID(kAddrBookCID, NS_ADDRESSBOOK_CID);

/* The definition is nsAddressBook.cpp */
extern const char *kDirectoryDataSourceRoot;
extern const char *kCardDataSourceRoot;

nsAbMDBCardProperty::nsAbMDBCardProperty(void)
{
	m_Key = 0;
	m_dbTableID = 0;
	m_dbRowID = 0;

	m_pAnonymousStrAttributes = nsnull;
	m_pAnonymousStrValues = nsnull;
	m_pAnonymousIntAttributes = nsnull;
	m_pAnonymousIntValues = nsnull;
	m_pAnonymousBoolAttributes = nsnull;
	m_pAnonymousBoolValues = nsnull;

}

nsAbMDBCardProperty::~nsAbMDBCardProperty(void)
{
	
	if (mCardDatabase)
		mCardDatabase = null_nsCOMPtr();

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

NS_IMPL_ISUPPORTS_INHERITED(nsAbMDBCardProperty, nsAbCardProperty, nsIAbMDBCard)


// nsIAbMDBCard attributes

NS_IMETHODIMP nsAbMDBCardProperty::GetDbTableID(PRUint32 *aDbTableID)
{
	*aDbTableID = m_dbTableID;
	return NS_OK;
}

NS_IMETHODIMP nsAbMDBCardProperty::SetDbTableID(PRUint32 aDbTableID)
{
	m_dbTableID = aDbTableID;
	return NS_OK;
}

NS_IMETHODIMP nsAbMDBCardProperty::GetDbRowID(PRUint32 *aDbRowID)
{
	*aDbRowID = m_dbRowID;
	return NS_OK;
}

NS_IMETHODIMP nsAbMDBCardProperty::SetDbRowID(PRUint32 aDbRowID)
{
	m_dbRowID = aDbRowID;
	return NS_OK;
}

NS_IMETHODIMP nsAbMDBCardProperty::GetKey(PRUint32 *aKey)
{
	*aKey = m_Key;
	return NS_OK;
}

NS_IMETHODIMP nsAbMDBCardProperty::GetAnonymousStrAttrubutesList(nsVoidArray **attrlist)
{
	if (attrlist && m_pAnonymousStrAttributes)
	{
		*attrlist = m_pAnonymousStrAttributes;
		return NS_OK;
	}
	else
		return NS_ERROR_NULL_POINTER;
}

NS_IMETHODIMP nsAbMDBCardProperty::GetAnonymousStrValuesList(nsVoidArray **valuelist)
{
	if (valuelist && m_pAnonymousStrValues)
	{
		*valuelist = m_pAnonymousStrValues;
		return NS_OK;
	}
	else
		return NS_ERROR_NULL_POINTER;
}

NS_IMETHODIMP nsAbMDBCardProperty::GetAnonymousIntAttrubutesList(nsVoidArray **attrlist)
{
	if (attrlist && m_pAnonymousIntAttributes)
	{
		*attrlist = m_pAnonymousIntAttributes;
		return NS_OK;
	}
	else
		return NS_ERROR_NULL_POINTER;
}

NS_IMETHODIMP nsAbMDBCardProperty::GetAnonymousIntValuesList(nsVoidArray **valuelist)
{
	if (valuelist && m_pAnonymousIntValues)
	{
		*valuelist = m_pAnonymousIntValues;
		return NS_OK;
	}
	else
		return NS_ERROR_NULL_POINTER;
}

NS_IMETHODIMP nsAbMDBCardProperty::GetAnonymousBoolAttrubutesList(nsVoidArray **attrlist)
{
	if (attrlist && m_pAnonymousBoolAttributes)
	{
		*attrlist = m_pAnonymousBoolAttributes;
		return NS_OK;
	}
	else
		return NS_ERROR_NULL_POINTER;
}

NS_IMETHODIMP nsAbMDBCardProperty::GetAnonymousBoolValuesList(nsVoidArray **valuelist)
{
	if (valuelist && m_pAnonymousBoolValues)
	{
		*valuelist = m_pAnonymousBoolValues;
		return NS_OK;
	}
	else
		return NS_ERROR_NULL_POINTER;
}







// nsIAbMDBCard methods

NS_IMETHODIMP nsAbMDBCardProperty::SetRecordKey(PRUint32 key)
{
	m_Key = key;
	return NS_OK;
}

NS_IMETHODIMP nsAbMDBCardProperty::SetAbDatabase(nsIAddrDatabase* database)
{
	mCardDatabase = database;
	return NS_OK;
}


NS_IMETHODIMP nsAbMDBCardProperty::SetAnonymousStringAttribute
(const char *attrname, const char *value)
{
	nsresult rv = NS_OK;

	char* pAttribute = nsCRT::strdup(attrname);
	char* pValue = nsCRT::strdup(value);
	if (pAttribute && pValue)
	{
		rv = SetAnonymousAttribute(&m_pAnonymousStrAttributes, 
			&m_pAnonymousStrValues, pAttribute, pValue);
	}
	else
	{
		nsCRT::free(pAttribute);
		nsCRT::free(pValue);
		rv = NS_ERROR_NULL_POINTER;
	}
	return rv;
}	

NS_IMETHODIMP nsAbMDBCardProperty::SetAnonymousIntAttribute
(const char *attrname, PRUint32 value)
{
	nsresult rv = NS_OK;

	char* pAttribute = nsCRT::strdup(attrname);
	PRUint32* pValue = (PRUint32 *)PR_Calloc(1, sizeof(PRUint32));
	*pValue = value;
	if (pAttribute && pValue)
	{
		rv = SetAnonymousAttribute(&m_pAnonymousIntAttributes, 
			&m_pAnonymousIntValues, pAttribute, pValue);
	}
	else
	{
		nsCRT::free(pAttribute);
		PR_FREEIF(pValue);
		rv = NS_ERROR_NULL_POINTER;
	}
	return rv;
}	

NS_IMETHODIMP nsAbMDBCardProperty::SetAnonymousBoolAttribute
(const char *attrname, PRBool value)
{
	nsresult rv = NS_OK;

	char* pAttribute = nsCRT::strdup(attrname);
	PRBool* pValue = (PRBool *)PR_Calloc(1, sizeof(PRBool));
	*pValue = value;
	if (pAttribute && pValue)
	{
		rv = SetAnonymousAttribute(&m_pAnonymousBoolAttributes, 
			&m_pAnonymousBoolValues, pAttribute, pValue);
	}
	else
	{	
		nsCRT::free(pAttribute);
		PR_FREEIF(pValue);
		rv = NS_ERROR_NULL_POINTER;
	}
	return rv;
}

/* caller need to PR_smprintf_free *uri */
NS_IMETHODIMP nsAbMDBCardProperty::GetCardURI(char **uri)
{
	char* cardURI = nsnull;
	nsFileSpec  *filePath = nsnull;
	if (mCardDatabase)
	{
		mCardDatabase->GetDbPath(&filePath);
		if (filePath)
		{
			char* file = nsnull;
			file = filePath->GetLeafName();
			if (file && m_dbRowID)
			{
				if (m_bIsMailList)
					cardURI = PR_smprintf("%s%s/ListCard%ld", kCardDataSourceRoot, file, m_dbRowID);
				else
					cardURI = PR_smprintf("%s%s/Card%ld", kCardDataSourceRoot, file, m_dbRowID);
			}
			if (file)
				nsCRT::free(file);
			delete filePath;
		}
	}
	if (cardURI)
	{
		*uri = cardURI;
		return NS_OK;
	}
	else
		return NS_ERROR_NULL_POINTER;
}

NS_IMETHODIMP nsAbMDBCardProperty::CopyCard(nsIAbMDBCard* srcCardDB)
{
	nsresult rv = NS_OK;
	nsCOMPtr<nsIAbCard> srcCard(do_QueryInterface(srcCardDB, &rv));
	NS_ENSURE_SUCCESS(rv, rv);

	nsXPIDLString str;
	srcCard->GetFirstName(getter_Copies(str));
	SetFirstName(str);

	srcCard->GetLastName(getter_Copies(str));
	SetLastName(str);
	srcCard->GetDisplayName(getter_Copies(str));
	SetDisplayName(str);
	srcCard->GetNickName(getter_Copies(str));
	SetNickName(str);
	srcCard->GetPrimaryEmail(getter_Copies(str));
	SetPrimaryEmail(str);
	srcCard->GetSecondEmail(getter_Copies(str));
	SetSecondEmail(str);

  PRUint32 format = nsIAbPreferMailFormat::unknown;
  srcCard->GetPreferMailFormat(&format);
  SetPreferMailFormat(format);

	srcCard->GetWorkPhone(getter_Copies(str));
	SetWorkPhone(str);
	srcCard->GetHomePhone(getter_Copies(str));
	SetHomePhone(str);
	srcCard->GetFaxNumber(getter_Copies(str));
	SetFaxNumber(str);
	srcCard->GetPagerNumber(getter_Copies(str));
	SetPagerNumber(str);
	srcCard->GetCellularNumber(getter_Copies(str));
	SetCellularNumber(str);
	srcCard->GetHomeAddress(getter_Copies(str));
	SetHomeAddress(str);
	srcCard->GetHomeAddress2(getter_Copies(str));
	SetHomeAddress2(str);
	srcCard->GetHomeCity(getter_Copies(str));
	SetHomeCity(str);
	srcCard->GetHomeState(getter_Copies(str));
	SetHomeState(str);
	srcCard->GetHomeZipCode(getter_Copies(str));
	SetHomeZipCode(str);
	srcCard->GetHomeCountry(getter_Copies(str));
	SetHomeCountry(str);
	srcCard->GetWorkAddress(getter_Copies(str));
	SetWorkAddress(str);
	srcCard->GetWorkAddress2(getter_Copies(str));
	SetWorkAddress2(str);
	srcCard->GetWorkCity(getter_Copies(str));
	SetWorkCity(str);
	srcCard->GetWorkState(getter_Copies(str));
	SetWorkState(str);
	srcCard->GetWorkZipCode(getter_Copies(str));
	SetWorkZipCode(str);
	srcCard->GetWorkCountry(getter_Copies(str));
	SetWorkCountry(str);
	srcCard->GetJobTitle(getter_Copies(str));
	SetJobTitle(str);
	srcCard->GetDepartment(getter_Copies(str));
	SetDepartment(str);
	srcCard->GetCompany(getter_Copies(str));
	SetCompany(str);
	srcCard->GetWebPage1(getter_Copies(str));
	SetWebPage1(str);
	srcCard->GetWebPage2(getter_Copies(str));
	SetWebPage2(str);
	srcCard->GetBirthYear(getter_Copies(str));
	SetBirthYear(str);
	srcCard->GetBirthMonth(getter_Copies(str));
	SetBirthMonth(str);
	srcCard->GetBirthDay(getter_Copies(str));
	SetBirthDay(str);
	srcCard->GetCustom1(getter_Copies(str));
	SetCustom1(str);
	srcCard->GetCustom2(getter_Copies(str));
	SetCustom2(str);
	srcCard->GetCustom3(getter_Copies(str));
	SetCustom3(str);
	srcCard->GetCustom4(getter_Copies(str));
	SetCustom4(str);
	srcCard->GetNotes(getter_Copies(str));
	SetNotes(str);

	PRUint32 tableID, rowID;
	srcCardDB->GetDbTableID(&tableID);
	SetDbTableID(tableID);
	srcCardDB->GetDbRowID(&rowID);
	SetDbRowID(rowID);

	return NS_OK;
}

NS_IMETHODIMP nsAbMDBCardProperty::AddAnonymousAttributesToDB()
{
	nsresult rv = NS_OK;
	if (mCardDatabase)
		mCardDatabase = null_nsCOMPtr();
	rv = GetCardDatabase(kPersonalAddressbookUri);
	if (NS_SUCCEEDED(rv) && mCardDatabase)
		rv = mCardDatabase->AddAnonymousAttributesFromCard(this);
	return rv;
}

NS_IMETHODIMP nsAbMDBCardProperty::EditAnonymousAttributesInDB()
{
	nsresult rv = NS_OK;
	if (mCardDatabase)
		mCardDatabase = null_nsCOMPtr();
	rv = GetCardDatabase(kPersonalAddressbookUri);
	if (NS_SUCCEEDED(rv) && mCardDatabase)
		rv = mCardDatabase->EditAnonymousAttributesFromCard(this);
	return rv;
}









// nsIAbCard methods

NS_IMETHODIMP nsAbMDBCardProperty::GetPrintCardUrl(char * *aPrintCardUrl)
{
static const char *kAbPrintUrlFormat = "addbook:printone?email=%s&folder=%s";

	if (!aPrintCardUrl)
		return NS_OK;

  nsXPIDLString email;
	GetPrimaryEmail(getter_Copies(email));
	nsAutoString emailStr(email);

	if (emailStr.Length() == 0)
	{
		*aPrintCardUrl = PR_smprintf("");
		return NS_OK;
	}
  nsXPIDLString dirName;
	if (mCardDatabase)
		mCardDatabase->GetDirectoryName(getter_Copies(dirName));
	nsAutoString dirNameStr(dirName);
	if (dirNameStr.Length() == 0)
	{
		*aPrintCardUrl = PR_smprintf("");
		return NS_OK;
	}
	dirNameStr.ReplaceSubstring(NS_ConvertASCIItoUCS2(" "), NS_ConvertASCIItoUCS2("%20"));

  char *emailCharStr = emailStr.ToNewUTF8String();
  char *dirCharStr = dirNameStr.ToNewUTF8String();

	*aPrintCardUrl = PR_smprintf(kAbPrintUrlFormat, emailCharStr, dirCharStr);

	nsMemory::Free(emailCharStr);
	nsMemory::Free(dirCharStr);

	return NS_OK;
}



NS_IMETHODIMP nsAbMDBCardProperty::EditCardToDatabase(const char *uri)
{
	if (!mCardDatabase && uri)
		GetCardDatabase(uri);

	if (mCardDatabase)
	{
		mCardDatabase->EditCard(this, PR_TRUE);
		mCardDatabase->Commit(kLargeCommit);
		return NS_OK;
	}
	else
		return NS_ERROR_FAILURE;
}

NS_IMETHODIMP nsAbMDBCardProperty::GetCollationKey(const PRUnichar *str, PRUnichar **key)
{
	nsresult rv = NS_OK;
	nsAutoString resultStr;

	if (mCardDatabase)
	{
		rv = mCardDatabase->CreateCollationKey(str, resultStr);
		*key = resultStr.ToNewUnicode();
	}
	else
		rv = NS_ERROR_FAILURE;

	return rv;
}





// protected class methods

nsresult nsAbMDBCardProperty::GetCardDatabase(const char *uri)
{
	nsresult rv = NS_OK;

	NS_WITH_SERVICE(nsIAddrBookSession, abSession, kAddrBookSessionCID, &rv); 
	if (NS_SUCCEEDED(rv))
	{
		nsFileSpec* dbPath;
		abSession->GetUserProfileDirectory(&dbPath);

		const char* file = nsnull;
		file = &(uri[PL_strlen(kDirectoryDataSourceRoot)]);
		(*dbPath) += file;
		
		if (dbPath->Exists())
		{
			NS_WITH_SERVICE(nsIAddrDatabase, addrDBFactory, kAddressBookDBCID, &rv);

			if (NS_SUCCEEDED(rv) && addrDBFactory)
				rv = addrDBFactory->Open(dbPath, PR_TRUE, getter_AddRefs(mCardDatabase), PR_TRUE);
		}
		else
			rv = NS_ERROR_FAILURE;
		delete dbPath;
	}
	return rv;
}


nsresult nsAbMDBCardProperty::RemoveAnonymousList(nsVoidArray* pArray)
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

nsresult nsAbMDBCardProperty::SetAnonymousAttribute
(nsVoidArray** pAttrAray, nsVoidArray** pValueArray, void *attrname, void *value)
{
	nsresult rv = NS_OK;
	nsVoidArray* pAttributes = *pAttrAray;
	nsVoidArray* pValues = *pValueArray; 

	if (!pAttributes && !pValues)
	{
		pAttributes = new nsVoidArray();
		pValues = new nsVoidArray();
		*pAttrAray = pAttributes;
		*pValueArray = pValues;
	}
	if (pAttributes && pValues)
	{
		if (attrname && value)
		{
			pAttributes->AppendElement(attrname);
			pValues->AppendElement(value);
		}
	}
	else
	{ 
		rv = NS_ERROR_FAILURE;
	}

	return rv;
}	
