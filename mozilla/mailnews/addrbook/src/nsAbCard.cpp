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
 *   Pierre Phaneuf <pp@ludusdesign.com>
 */

#include "nsAbCard.h"	 
#include "nsIRDFService.h"
#include "nsIServiceManager.h"
#include "nsRDFCID.h"
#include "nsXPIDLString.h"
#include "nsCOMPtr.h"
#include "nsAbBaseCID.h"
#include "prmem.h"	 
#include "prlog.h"	 
#include "nsAddrDatabase.h"
#include "nsIAddrBookSession.h"

static NS_DEFINE_CID(kRDFServiceCID, NS_RDFSERVICE_CID);
static NS_DEFINE_CID(kAddrBookSessionCID, NS_ADDRBOOKSESSION_CID);

nsAbCard::nsAbCard(void)
  : nsAbRDFResource(), mListeners(nsnull)
{
}

nsAbCard::~nsAbCard(void)
{
	if (mCardDatabase)
	{
		nsIAddrDBListener* listener = this;
		mCardDatabase->RemoveListener(listener);
		mCardDatabase = null_nsCOMPtr();
	}

	if (mListeners) 
	{
		PRInt32 i;
		for (i = mListeners->Count() - 1; i >= 0; --i) 
			mListeners->RemoveElementAt(i);
		delete mListeners;
	}
}

NS_IMPL_ISUPPORTS_INHERITED(nsAbCard, nsAbRDFResource, nsIAbCard)

////////////////////////////////////////////////////////////////////////////////
NS_IMETHODIMP nsAbCard::OnCardEntryChange
(PRUint32 abCode, nsIAbCard *card, nsIAddrDBListener *instigator)
{
	if (abCode == AB_NotifyPropertyChanged && card)
	{
		PRUint32 tableID;
		PRUint32 rowID;
		PRBool bMailList;

		card->GetDbTableID(&tableID);
		card->GetDbRowID(&rowID);
		card->GetIsMailList(&bMailList);
		if (m_dbTableID == tableID && m_dbRowID == rowID && m_bIsMailList == bMailList)
		{
			nsXPIDLString pDisplayName;
			card->GetDisplayName(getter_Copies(pDisplayName));
			if (pDisplayName)
				NotifyPropertyChanged("DisplayName", nsnull, 
									  NS_CONST_CAST(PRUnichar*, (const PRUnichar*)pDisplayName));

			nsXPIDLString pName;
			card->GetName(getter_Copies(pName));
			if (pName)
				NotifyPropertyChanged("Name", nsnull, 
									  NS_CONST_CAST(PRUnichar*, (const PRUnichar*)pName));

			nsXPIDLString pNickName;
			card->GetNickName(getter_Copies(pNickName));
			if (pNickName)
				NotifyPropertyChanged("NickName", nsnull, 
									  NS_CONST_CAST(PRUnichar*, (const PRUnichar*)pNickName));

			nsXPIDLString pPrimaryEmail;
			card->GetPrimaryEmail(getter_Copies(pPrimaryEmail));
			if (pPrimaryEmail)
				NotifyPropertyChanged("PrimaryEmail", nsnull, 
									  NS_CONST_CAST(PRUnichar*, (const PRUnichar*)pPrimaryEmail));

			nsXPIDLString pSecondEmail;
			card->GetSecondEmail(getter_Copies(pSecondEmail));
			if (pSecondEmail)
				NotifyPropertyChanged("SecondEmail", nsnull, 
									  NS_CONST_CAST(PRUnichar*, (const PRUnichar*)pSecondEmail));

			nsXPIDLString pWorkPhone;
			card->GetWorkPhone(getter_Copies(pWorkPhone));
			if (pWorkPhone)
				NotifyPropertyChanged("WorkPhone", nsnull, 
									  NS_CONST_CAST(PRUnichar*, (const PRUnichar*)pWorkPhone));

			nsXPIDLString pHomePhone;
			card->GetHomePhone(getter_Copies(pHomePhone));
			if (pHomePhone)
				NotifyPropertyChanged("HomePhone", nsnull, 
									  NS_CONST_CAST(PRUnichar*, (const PRUnichar*)pHomePhone));

			nsXPIDLString pFaxNumber;
			card->GetFaxNumber(getter_Copies(pFaxNumber));
			if (pFaxNumber)
				NotifyPropertyChanged("FaxNumber", nsnull, 
									  NS_CONST_CAST(PRUnichar*, (const PRUnichar*)pFaxNumber));

			nsXPIDLString pPagerNumber;
			card->GetPagerNumber(getter_Copies(pPagerNumber));
			if (pPagerNumber)
				NotifyPropertyChanged("PagerNumber", nsnull, 
									  NS_CONST_CAST(PRUnichar*, (const PRUnichar*)pPagerNumber));

			nsXPIDLString pCellularNumber;
			card->GetCellularNumber(getter_Copies(pCellularNumber));
			if (pCellularNumber)
				NotifyPropertyChanged("CellularNumber", nsnull, 
									  NS_CONST_CAST(PRUnichar*, (const PRUnichar*)pCellularNumber));

			nsXPIDLString pJobTitle;
			card->GetJobTitle(getter_Copies(pJobTitle));
			if (pJobTitle)
				NotifyPropertyChanged("JobTitle", nsnull, 
									  NS_CONST_CAST(PRUnichar*, (const PRUnichar*)pJobTitle));

			nsXPIDLString pDepartment;
			card->GetDepartment(getter_Copies(pDepartment));
			if (pDepartment)
				NotifyPropertyChanged("Department", nsnull, 
									  NS_CONST_CAST(PRUnichar*, (const PRUnichar*)pDepartment));

			nsXPIDLString pCompany;
			card->GetCompany(getter_Copies(pCompany));
			if (pCompany)
				NotifyPropertyChanged("Company", nsnull, 
									  NS_CONST_CAST(PRUnichar*, (const PRUnichar*)pCompany));
		}
	}
	return NS_OK;
}

nsresult nsAbCard::NotifyPropertyChanged(char *property, PRUnichar* oldValue, PRUnichar* newValue)
{
	nsCOMPtr<nsISupports> supports;
	if(NS_SUCCEEDED(QueryInterface(NS_GET_IID(nsISupports), getter_AddRefs(supports))))
	{
		//Notify listeners who listen to every folder
		nsresult rv;
		NS_WITH_SERVICE(nsIAddrBookSession, abSession, kAddrBookSessionCID, &rv); 
		if(NS_SUCCEEDED(rv))
			abSession->NotifyItemPropertyChanged(supports, property, oldValue, newValue);
	}

	return NS_OK;
}

nsresult nsAbCard::AddSubNode(nsAutoString name, nsIAbCard **childCard)
{
	if(!childCard)
		return NS_ERROR_NULL_POINTER;

	nsresult rv = NS_OK;
	NS_WITH_SERVICE(nsIRDFService, rdf, kRDFServiceCID, &rv);

	if(NS_FAILED(rv))
		return rv;

  nsCAutoString uri;
  uri.Append(mURI);
  uri.Append('/');

  char *utf8Name = name.ToNewUTF8String();
  if (!utf8Name)
    return NS_ERROR_OUT_OF_MEMORY;
  uri.Append(utf8Name);
  nsMemory::Free(utf8Name);

	nsCOMPtr<nsIRDFResource> res;
  rv = rdf->GetResource(uri.GetBuffer(), getter_AddRefs(res));
	if (NS_FAILED(rv))
		return rv;
	nsCOMPtr<nsIAbCard> card(do_QueryInterface(res, &rv));
	if (NS_FAILED(rv))
		return rv;        

	*childCard = card;
	NS_IF_ADDREF(*childCard);

	return rv;
}

