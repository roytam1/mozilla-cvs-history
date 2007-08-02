/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
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
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Pierre Phaneuf <pp@ludusdesign.com>
 *   Seth Spitzer <sspitzer@netscape.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either of the GNU General Public License Version 2 or later (the "GPL"),
 * or the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "nsAbDirProperty.h"	 
#include "nsAbBaseCID.h"
#include "nsIAbCard.h"
#include "nsDirPrefs.h"
#include "nsIPrefService.h"
#include "nsIPrefLocalizedString.h"
#include "nsServiceManagerUtils.h"
#include "prmem.h"
#include "nsCRT.h"
#include "rdf.h"
#include "nsIAddrBookSession.h"

// From nsDirPrefs
#define kDefaultPosition 1

nsAbDirProperty::nsAbDirProperty(void)
  : m_LastModifiedDate(0)
{
	m_IsMailList = PR_FALSE;
}

nsAbDirProperty::~nsAbDirProperty(void)
{
#if 0
  // this code causes a regression #138647
  // don't turn it on until you figure it out
  if (m_AddressList) {
    PRUint32 count;
    nsresult rv;
    rv = m_AddressList->Count(&count);
    NS_ASSERTION(NS_SUCCEEDED(rv), "Count failed");
    PRInt32 i;
    for (i = count - 1; i >= 0; i--)
      m_AddressList->RemoveElementAt(i);
  }
#endif
}

NS_IMPL_ISUPPORTS1(nsAbDirProperty,nsIAbDirectory)

NS_IMETHODIMP nsAbDirProperty::GetOperations(PRInt32 *aOperations)
{
  // Default is to support all operations.
  // Inheriting implementations may override
  // to reduce supported operations
  NS_ENSURE_ARG_POINTER(aOperations);
	*aOperations = nsIAbDirectory::opRead |
		nsIAbDirectory::opWrite |
		nsIAbDirectory::opSearch;

	return NS_OK;
}

NS_IMETHODIMP nsAbDirProperty::GetDirName(nsAString &aDirName)
{
  if (m_IsMailList)
  {
    aDirName = m_ListDirName;
    return NS_OK;
  }

  nsCString dirName;
  nsresult rv = GetLocalizedStringValue("description", EmptyCString(),
                                        dirName);
  NS_ENSURE_SUCCESS(rv, rv);

  CopyUTF8toUTF16(dirName, aDirName);
  return NS_OK;
}

// XXX Although mailing lists could use the NotifyItemPropertyChanged
// mechanism here, it requires some rework on how we write/save data
// relating to mailing lists, so we're just using the old method of a
// local variable to store tha mailing list name.
NS_IMETHODIMP nsAbDirProperty::SetDirName(const nsAString &aDirName)
{
  if (m_IsMailList)
  {
    m_ListDirName = aDirName;
    return NS_OK;
  }

  // Store the old value.
  nsString oldDirName;
  nsresult rv = GetDirName(oldDirName);
  NS_ENSURE_SUCCESS(rv, rv);

  // Save the new value
  rv = SetStringValue("description", NS_ConvertUTF16toUTF8(aDirName));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIAddrBookSession> abSession =
    do_GetService(NS_ADDRBOOKSESSION_CONTRACTID, &rv);

  if (NS_SUCCEEDED(rv))
    abSession->NotifyItemPropertyChanged(this, "DirName", oldDirName.get(),
                                         nsString(aDirName).get());

  return NS_OK;
}

NS_IMETHODIMP nsAbDirProperty::GetDirType(PRInt32 *aDirType)
{
  return GetIntValue("dirType", LDAPDirectory, aDirType);
}

NS_IMETHODIMP nsAbDirProperty::GetFileName(nsACString &aFileName)
{
  return GetStringValue("filename", EmptyCString(), aFileName);
}

NS_IMETHODIMP nsAbDirProperty::GetURI(nsACString &aURI)
{
  // XXX Should we complete this for Mailing Lists?
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsAbDirProperty::GetPosition(PRInt32 *aPosition)
{
  return GetIntValue("position", kDefaultPosition, aPosition);
}

NS_IMETHODIMP nsAbDirProperty::GetLastModifiedDate(PRUint32 *aLastModifiedDate)
{
	if (aLastModifiedDate)
	{
		*aLastModifiedDate = m_LastModifiedDate;
		return NS_OK;
	}
	else
		return NS_RDF_NO_VALUE;
}

NS_IMETHODIMP nsAbDirProperty::SetLastModifiedDate(PRUint32 aLastModifiedDate)
{
	if (aLastModifiedDate)
	{
		m_LastModifiedDate = aLastModifiedDate;
	}
	return NS_OK;
}

nsresult nsAbDirProperty::GetAttributeName(PRUnichar **aName, nsString& value)
{
	if (aName)
	{
		*aName = ToNewUnicode(value);
		if (!(*aName)) 
			return NS_ERROR_OUT_OF_MEMORY;
		else
			return NS_OK;
	}
	else
		return NS_ERROR_NULL_POINTER;

}

nsresult nsAbDirProperty::SetAttributeName(const PRUnichar *aName, nsString& arrtibute)
{
	if (aName)
		arrtibute = aName;
	return NS_OK;
}

NS_IMETHODIMP nsAbDirProperty::GetListNickName(PRUnichar * *aListNickName)
{ return GetAttributeName(aListNickName, m_ListNickName); }

NS_IMETHODIMP nsAbDirProperty::SetListNickName(const PRUnichar * aListNickName)
{ return SetAttributeName(aListNickName, m_ListNickName); }

NS_IMETHODIMP nsAbDirProperty::GetDescription(PRUnichar * *aDescription)
{ return GetAttributeName(aDescription, m_Description); }

NS_IMETHODIMP nsAbDirProperty::SetDescription(const PRUnichar * aDescription)
{ return SetAttributeName(aDescription, m_Description); }

NS_IMETHODIMP nsAbDirProperty::GetIsMailList(PRBool *aIsMailList)
{
	*aIsMailList = m_IsMailList;
	return NS_OK;
}

NS_IMETHODIMP nsAbDirProperty::SetIsMailList(PRBool aIsMailList)
{
	m_IsMailList = aIsMailList;
	return NS_OK;
}

NS_IMETHODIMP nsAbDirProperty::GetAddressLists(nsISupportsArray * *aAddressLists)
{
	if (!m_AddressList)
	{
		NS_NewISupportsArray(getter_AddRefs(m_AddressList));
	}

	*aAddressLists = m_AddressList;
	NS_ADDREF(*aAddressLists);
	return NS_OK;
}

NS_IMETHODIMP nsAbDirProperty::SetAddressLists(nsISupportsArray * aAddressLists)
{
	m_AddressList = aAddressLists;
	return NS_OK;
}

NS_IMETHODIMP nsAbDirProperty::CopyMailList(nsIAbDirectory* srcList)
{
  SetIsMailList(PR_TRUE);

  nsString str;
  srcList->GetDirName(str);
  SetDirName(str);
  srcList->GetListNickName(getter_Copies(str));
  SetListNickName(str.get());
  srcList->GetDescription(getter_Copies(str));
  SetDescription(str.get());

  nsCOMPtr <nsISupportsArray> pAddressLists;
  srcList->GetAddressLists(getter_AddRefs(pAddressLists));
  SetAddressLists(pAddressLists);
  return NS_OK;
}

// nsIAbDirectory NOT IMPLEMENTED methods

NS_IMETHODIMP
nsAbDirProperty::GetChildNodes(nsISimpleEnumerator **childList)
{ return NS_ERROR_NOT_IMPLEMENTED; }

NS_IMETHODIMP
nsAbDirProperty::GetChildCards(nsISimpleEnumerator **childCards)
{ return NS_ERROR_NOT_IMPLEMENTED; }

NS_IMETHODIMP
nsAbDirProperty::DeleteDirectory(nsIAbDirectory *directory)
{ return NS_ERROR_NOT_IMPLEMENTED; }

NS_IMETHODIMP
nsAbDirProperty::HasCard(nsIAbCard *cards, PRBool *hasCard)
{ return NS_ERROR_NOT_IMPLEMENTED; }

NS_IMETHODIMP
nsAbDirProperty::HasDirectory(nsIAbDirectory *dir, PRBool *hasDir)
{ return NS_ERROR_NOT_IMPLEMENTED; }

NS_IMETHODIMP
nsAbDirProperty::CreateNewDirectory(const nsAString &aDirName,
                                    const nsACString &aURI,
                                    const PRUint32 aType, nsACString &aResult)
{ return NS_ERROR_NOT_IMPLEMENTED; }

NS_IMETHODIMP
nsAbDirProperty::CreateDirectoryByURI(const nsAString &aDisplayName,
                                      const nsACString &aURI,
                                      PRBool migrating)
{ return NS_ERROR_NOT_IMPLEMENTED; }

NS_IMETHODIMP nsAbDirProperty::AddMailList(nsIAbDirectory *list)
{ return NS_ERROR_NOT_IMPLEMENTED; }

NS_IMETHODIMP nsAbDirProperty::EditMailListToDatabase(const char *uri, nsIAbCard *listCard)
{ return NS_ERROR_NOT_IMPLEMENTED; }

NS_IMETHODIMP nsAbDirProperty::AddCard(nsIAbCard *childCard, nsIAbCard **addedCard)
{ return NS_ERROR_NOT_IMPLEMENTED; }

NS_IMETHODIMP nsAbDirProperty::ModifyCard(nsIAbCard *aModifiedCard)
{ return NS_ERROR_NOT_IMPLEMENTED; }

NS_IMETHODIMP nsAbDirProperty::DeleteCards(nsISupportsArray *cards)
{ return NS_ERROR_NOT_IMPLEMENTED; }

NS_IMETHODIMP nsAbDirProperty::DropCard(nsIAbCard *childCard, PRBool needToCopyCard)
{ return NS_ERROR_NOT_IMPLEMENTED; }

NS_IMETHODIMP nsAbDirProperty::GetSupportsMailingLists(PRBool *aSupportsMailingsLists)
{
  NS_ENSURE_ARG_POINTER(aSupportsMailingsLists);
  // We don't currently support nested mailing lists, so only return true if
  // we're not a mailing list.
  *aSupportsMailingsLists = !m_IsMailList;
  return NS_OK;
}

NS_IMETHODIMP nsAbDirProperty::GetIsRemote(PRBool *aIsRemote)
{
  NS_ENSURE_ARG_POINTER(aIsRemote);
  *aIsRemote = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP nsAbDirProperty::GetIsSecure(PRBool *aIsSecure)
{
  NS_ENSURE_ARG_POINTER(aIsSecure);
  *aIsSecure = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP nsAbDirProperty::GetSearchDuringLocalAutocomplete(PRBool *aSearchDuringLocalAutocomplete)
{
  NS_ENSURE_ARG_POINTER(aSearchDuringLocalAutocomplete);
  *aSearchDuringLocalAutocomplete = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP nsAbDirProperty::GetDirPrefId(nsACString &aDirPrefId)
{
  aDirPrefId = m_DirPrefId;
  return NS_OK;
}

NS_IMETHODIMP nsAbDirProperty::SetDirPrefId(const nsACString &aDirPrefId)
{
  if (!m_DirPrefId.Equals(aDirPrefId))
  {
    m_DirPrefId.Assign(aDirPrefId);
    // Clear the directory pref branch so that it is re-initialized next
    // time its required.
    m_DirectoryPrefs = nsnull;
  }
  return NS_OK;
}

nsresult nsAbDirProperty::InitDirectoryPrefs()
{
  if (m_DirPrefId.IsEmpty())
    return NS_ERROR_NOT_INITIALIZED;
    
  nsresult rv;
  nsCOMPtr<nsIPrefService> prefService(do_GetService(NS_PREFSERVICE_CONTRACTID, &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCString realPrefId(m_DirPrefId);
  realPrefId.Append('.');

  return prefService->GetBranch(realPrefId.get(), getter_AddRefs(m_DirectoryPrefs));
}

NS_IMETHODIMP nsAbDirProperty::GetIntValue(const char *aName,
                                          PRInt32 aDefaultValue,
                                          PRInt32 *aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);

  if (!m_DirectoryPrefs && NS_FAILED(InitDirectoryPrefs()))
    return NS_ERROR_NOT_INITIALIZED;

  if (NS_FAILED(m_DirectoryPrefs->GetIntPref(aName, aResult)))
    *aResult = aDefaultValue;

  return NS_OK;
}

NS_IMETHODIMP nsAbDirProperty::GetBoolValue(const char *aName,
                                           PRBool aDefaultValue,
                                           PRBool *aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);

  if (!m_DirectoryPrefs && NS_FAILED(InitDirectoryPrefs()))
    return NS_ERROR_NOT_INITIALIZED;

  if (NS_FAILED(m_DirectoryPrefs->GetBoolPref(aName, aResult)))
    *aResult = aDefaultValue;

  return NS_OK;
}

NS_IMETHODIMP nsAbDirProperty::GetStringValue(const char *aName,
                                              const nsACString &aDefaultValue, 
                                              nsACString &aResult)
{
  if (!m_DirectoryPrefs && NS_FAILED(InitDirectoryPrefs()))
    return NS_ERROR_NOT_INITIALIZED;

  nsCString value;

    /* unfortunately, there may be some prefs out there which look like (null) */
  if (NS_SUCCEEDED(m_DirectoryPrefs->GetCharPref(aName, getter_Copies(value))) &&
      !value.EqualsLiteral("(null"))
    aResult = value;
  else
    aResult = aDefaultValue;

  return NS_OK;
}
/*
 * Get localized unicode string pref from properties file, convert into an
 * UTF8 string since address book prefs store as UTF8 strings. So far there
 * are 2 default prefs stored in addressbook.properties.
 * "ldap_2.servers.pab.description"
 * "ldap_2.servers.history.description"
 */
NS_IMETHODIMP nsAbDirProperty::GetLocalizedStringValue(const char *aName,
                                                       const nsACString &aDefaultValue, 
                                                       nsACString &aResult)
{
  if (!m_DirectoryPrefs && NS_FAILED(InitDirectoryPrefs()))
    return NS_ERROR_NOT_INITIALIZED;

  nsString wvalue;
  nsCOMPtr<nsIPrefLocalizedString> locStr;

  nsresult rv = m_DirectoryPrefs->GetComplexValue(aName,
                                                  NS_GET_IID(nsIPrefLocalizedString),
                                                  getter_AddRefs(locStr));
  if (NS_SUCCEEDED(rv))
  {
    rv = locStr->ToString(getter_Copies(wvalue));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (wvalue.IsEmpty())
    aResult = aDefaultValue;
  else
    CopyUTF16toUTF8(wvalue, aResult);

  return NS_OK;
}

NS_IMETHODIMP nsAbDirProperty::SetIntValue(const char *aName,
                                          PRInt32 aValue)
{
  if (!m_DirectoryPrefs && NS_FAILED(InitDirectoryPrefs()))
    return NS_ERROR_NOT_INITIALIZED;

  return m_DirectoryPrefs->SetIntPref(aName, aValue);
}

NS_IMETHODIMP nsAbDirProperty::SetBoolValue(const char *aName,
                                           PRBool aValue)
{
  if (!m_DirectoryPrefs && NS_FAILED(InitDirectoryPrefs()))
    return NS_ERROR_NOT_INITIALIZED;

  return m_DirectoryPrefs->SetBoolPref(aName, aValue);
}

NS_IMETHODIMP nsAbDirProperty::SetStringValue(const char *aName,
                                              const nsACString &aValue)
{
  if (!m_DirectoryPrefs && NS_FAILED(InitDirectoryPrefs()))
    return NS_ERROR_NOT_INITIALIZED;

  return m_DirectoryPrefs->SetCharPref(aName, nsCString(aValue).get());
}
