/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: NPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is 
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 2001
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or 
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the NPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "nsAbMDBCardProperty.h"	 
#include "nsIRDFService.h"
#include "nsIServiceManager.h"
#include "nsRDFCID.h"
#include "nsXPIDLString.h"
#include "nsReadableUtils.h"
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

nsAbMDBCardProperty::nsAbMDBCardProperty(void)
{
	m_Key = 0;
	m_dbTableID = 0;
	m_dbRowID = 0;
}

nsAbMDBCardProperty::~nsAbMDBCardProperty(void)
{
	if (mCardDatabase)
		mCardDatabase = nsnull;
}

NS_IMPL_ISUPPORTS_INHERITED1(nsAbMDBCardProperty, nsAbCardProperty, nsIAbMDBCard)

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

NS_IMETHODIMP nsAbMDBCardProperty::SetStringAttribute(const PRUnichar *name, const PRUnichar *value)
{
  NS_ASSERTION(mCardDatabase, "no db");
  if (!mCardDatabase)
    return NS_ERROR_UNEXPECTED;

  return mCardDatabase->SetCardValue(this, name, value, PR_TRUE /* notify */);
}

NS_IMETHODIMP nsAbMDBCardProperty::GetStringAttribute(const PRUnichar *name, PRUnichar **value)
{
  NS_ASSERTION(mCardDatabase, "no db");
  if (!mCardDatabase)
    return NS_ERROR_UNEXPECTED;

  return mCardDatabase->GetCardValue(this, name, value);
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

// protected class methods

nsresult nsAbMDBCardProperty::GetCardDatabase(const char *uri)
{
	nsresult rv = NS_OK;

	nsCOMPtr<nsIAddrBookSession> abSession = 
	         do_GetService(NS_ADDRBOOKSESSION_CONTRACTID, &rv); 
	if (NS_SUCCEEDED(rv))
	{
		nsFileSpec* dbPath;
		abSession->GetUserProfileDirectory(&dbPath);

		const char* file = nsnull;
		file = &(uri[PL_strlen(kMDBDirectoryRoot)]);
		(*dbPath) += file;
		
		if (dbPath->Exists())
		{
			nsCOMPtr<nsIAddrDatabase> addrDBFactory = 
			         do_GetService(NS_ADDRDATABASE_CONTRACTID, &rv);

			if (NS_SUCCEEDED(rv) && addrDBFactory)
				rv = addrDBFactory->Open(dbPath, PR_TRUE, getter_AddRefs(mCardDatabase), PR_TRUE);
		}
		else
			rv = NS_ERROR_FAILURE;
		delete dbPath;
	}
	return rv;
}


