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
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Seth Spitzer <sspitzer@netscape.com>
 *   Pierre Phaneuf <pp@ludusdesign.com>
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

#include "nsAbCardProperty.h"	 
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
#include "nsReadableUtils.h"
#include "nsUnicharUtils.h"
#include "nsIPref.h"
#include "nsIAbDirectory.h"

nsAbCardProperty::nsAbCardProperty(void)
{
	NS_INIT_REFCNT();

	m_LastModDate = 0;

	m_PreferMailFormat = nsIAbPreferMailFormat::unknown;
	m_IsMailList = PR_FALSE;
	m_MailListURI = nsnull;
}

nsAbCardProperty::~nsAbCardProperty(void)
{
  CRTFREEIF(m_MailListURI);
}

NS_IMPL_ISUPPORTS1(nsAbCardProperty, nsIAbCard)

////////////////////////////////////////////////////////////////////////////////

nsresult nsAbCardProperty::GetAttributeName(PRUnichar **aName, nsString& value)
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

nsresult nsAbCardProperty::SetAttributeName(const PRUnichar *aName, nsString& arrtibute)
{
	if (aName)
		arrtibute = aName;
	return NS_OK;
}

NS_IMETHODIMP nsAbCardProperty::GetPreferMailFormat(PRUint32 *aFormat)
{
	*aFormat = m_PreferMailFormat;	
	return NS_OK;
}

NS_IMETHODIMP nsAbCardProperty::SetPreferMailFormat(PRUint32 aFormat)
{
	m_PreferMailFormat = aFormat;
	return NS_OK;
}

NS_IMETHODIMP nsAbCardProperty::GetIsMailList(PRBool *aIsMailList)
{
	*aIsMailList = m_IsMailList;
	return NS_OK;
}

NS_IMETHODIMP nsAbCardProperty::SetIsMailList(PRBool aIsMailList)
{
	m_IsMailList = aIsMailList;
	return NS_OK;
}

NS_IMETHODIMP nsAbCardProperty::GetMailListURI(char **aMailListURI)
{
	if (aMailListURI)
	{
		if (m_MailListURI)
			*aMailListURI = nsCRT::strdup(m_MailListURI);
		else
			*aMailListURI = nsCRT::strdup("");

		return NS_OK;
	}
	else
		return NS_ERROR_NULL_POINTER;
}

NS_IMETHODIMP nsAbCardProperty::SetMailListURI(const char *aMailListURI)
{
	if (aMailListURI)
	{
		nsCRT::free (m_MailListURI);
		m_MailListURI = nsCRT::strdup(aMailListURI);
		return NS_OK;
	}
	else
		return NS_ERROR_NULL_POINTER;
}

NS_IMETHODIMP nsAbCardProperty::GetCardValue(const char *attrname, PRUnichar **value)
{
  NS_ENSURE_ARG_POINTER(attrname);
  NS_ENSURE_ARG_POINTER(value);

  nsresult rv = NS_OK;

  switch (attrname[0]) {
    case 'A':
      // kAddressCharSetColumn?
      rv = NS_ERROR_UNEXPECTED;
      break;
    case 'B':
      // BirthYear, BirthMonth, BirthDay
      switch (attrname[5]) {
        case 'Y':
          rv = GetBirthYear(value);
          break;
        case 'M':
          rv = GetBirthMonth(value);
          break;
        case 'D':
          rv = GetBirthDay(value);
          break;
        default:
          rv = NS_ERROR_UNEXPECTED;
          break;
      }
      break;
    case 'C':
      switch (attrname[1]) {
        case 'o':
          rv = GetCompany(value);
          break;
        case 'e':
          rv = GetCellularNumber(value);
          break;
        case 'u':
          switch (attrname[6]) {
            case '1':
              rv = GetCustom1(value);
              break;
            case '2':
              rv = GetCustom2(value);
              break;
            case '3':
              rv = GetCustom3(value);
              break;
            case '4':
              rv = GetCustom4(value);
              break;
            default:
              rv = NS_ERROR_UNEXPECTED;
              break;
          }
          break;
        default:
          rv = NS_ERROR_UNEXPECTED;
          break;
      }
      break;
    case 'D':
      if (attrname[1] == 'i') 
        rv = GetDisplayName(value);
      else 
        rv = GetDepartment(value);
      break;
    case 'F':
      if (attrname[1] == 'i') 
        rv = GetFirstName(value);
      else
        rv = GetFaxNumber(value);
      break;
    case 'H':
      switch (attrname[4]) {
        case 'A':
          if (attrname[11] == '\0')
            rv = GetHomeAddress(value);
          else 
            rv = GetHomeAddress2(value);
          break;
        case 'C':
          if (attrname[5] == 'i')
            rv = GetHomeCity(value);
          else 
            rv = GetHomeCountry(value);
          break;
        case 'P':
          rv = GetHomePhone(value);
          break;
        case 'S':
          rv = GetHomeState(value);
          break;
        case 'Z':
          rv = GetHomeZipCode(value);
          break;
        default:
          rv = NS_ERROR_UNEXPECTED;
          break;
      }
      break;
    case 'J':
      rv = GetJobTitle(value);
      break;
    case 'L':
      if (attrname[1] == 'a') {
        if (attrname[4] == 'N') 
          rv = GetLastName(value);
        else {
          // XXX todo
          // fix me?  LDAP code gets here
          PRUint32 lastModifiedDate;
          rv = GetLastModifiedDate(&lastModifiedDate);
          *value = nsCRT::strdup(NS_LITERAL_STRING("0Z").get()); 
        }
      }
      else
        rv = NS_ERROR_UNEXPECTED;
      break;
    case 'N':
      if (attrname[1] == 'o')
        rv = GetNotes(value);
      else 
        rv = GetNickName(value);
      break;
    case 'P':
      switch (attrname[2]) { 
        case 'e':
	  PRUint32 format;
          rv = GetPreferMailFormat(&format);
          const PRUnichar *formatStr;

          switch (format) {
            case nsIAbPreferMailFormat::html:
              formatStr = NS_LITERAL_STRING("html").get();
              break;
            case nsIAbPreferMailFormat::plaintext :
              formatStr = NS_LITERAL_STRING("plaintext").get();
              break;
            case nsIAbPreferMailFormat::unknown:
            default :
              formatStr = NS_LITERAL_STRING("unknown").get();
              break;
          }
          *value = nsCRT::strdup(formatStr);
          break;
        case 'g':
          rv = GetPagerNumber(value);
          break;
        case 'i':
          rv = GetPrimaryEmail(value);
          break;
        default:
          rv = NS_ERROR_UNEXPECTED;
          break;
      }
      break;
    case 'S':
      rv = GetSecondEmail(value);
      break;
    case 'W': 
      if (attrname[1] == 'e') {
        if (attrname[7] == '1')
          rv = GetWebPage1(value);
        else 
          rv = GetWebPage2(value);
      }
      else {
        switch (attrname[4]) {
          case 'A':
            if (attrname[11] == '\0')
              rv = GetWorkAddress(value);
            else 
              rv = GetWorkAddress2(value);
            break;
          case 'C':
            if (attrname[5] == 'i')
              rv = GetWorkCity(value);
            else 
              rv = GetWorkCountry(value);
            break;
          case 'P':
            rv = GetWorkPhone(value);
            break;
          case 'S':
            rv = GetWorkState(value);
            break;
          case 'Z':
            rv = GetWorkZipCode(value);
            break;
          default:
            rv = NS_ERROR_UNEXPECTED;
            break;
        }
      }
      break;
    default:
      rv = NS_ERROR_UNEXPECTED;
      break;
  }

  // don't assert here, as failure is expected in certain cases
  // we call GetCardValue() from nsAbView::Init() to determine if the 
  // saved sortColumn is valid or not.
  return rv;
}

NS_IMETHODIMP nsAbCardProperty::SetCardValue(const char *attrname, const PRUnichar *value)
{
  NS_ENSURE_ARG_POINTER(attrname);
  NS_ENSURE_ARG_POINTER(value);

  nsresult rv = NS_OK;

  switch (attrname[0]) {
    case 'A':
      // kAddressCharSetColumn?
      rv = NS_ERROR_UNEXPECTED;
      break;
    case 'B':
      // BirthYear, BirthMonth, BirthDay
      switch (attrname[5]) {
        case 'Y':
          rv = SetBirthYear(value);
          break;
        case 'M':
          rv = SetBirthMonth(value);
          break;
        case 'D':
          rv = SetBirthDay(value);
          break;
        default:
          rv = NS_ERROR_UNEXPECTED;
          break;
      }
      break;
    case 'C':
      switch (attrname[1]) {
        case 'o':
          rv = SetCompany(value);
          break;
        case 'e':
          rv = SetCellularNumber(value);
          break;
        case 'u':
          switch (attrname[6]) {
            case '1':
              rv = SetCustom1(value);
              break;
            case '2':
              rv = SetCustom2(value);
              break;
            case '3':
              rv = SetCustom3(value);
              break;
            case '4':
              rv = SetCustom4(value);
              break;
            default:
              rv = NS_ERROR_UNEXPECTED;
              break;
          }
          break;
        default:
          rv = NS_ERROR_UNEXPECTED;
          break;
      }
      break;
    case 'D':
      if (attrname[1] == 'i') 
        rv = SetDisplayName(value);
      else 
        rv = SetDepartment(value);
      break;
    case 'F':
      if (attrname[1] == 'i') 
        rv = SetFirstName(value);
      else
        rv = SetFaxNumber(value);
      break;
    case 'H':
      switch (attrname[4]) {
        case 'A':
          if (attrname[11] == '\0')
            rv = SetHomeAddress(value);
          else 
            rv = SetHomeAddress2(value);
          break;
        case 'C':
          if (attrname[5] == 'i')
            rv = SetHomeCity(value);
          else 
            rv = SetHomeCountry(value);
          break;
        case 'P':
          rv = SetHomePhone(value);
          break;
        case 'S':
          rv = SetHomeState(value);
          break;
        case 'Z':
          rv = SetHomeZipCode(value);
          break;
        default:
          rv = NS_ERROR_UNEXPECTED;
          break;
      }
      break;
    case 'J':
      rv = SetJobTitle(value);
      break;
    case 'L':
      if (attrname[1] == 'a') {
        if (attrname[4] == 'N') 
          rv = SetLastName(value);
        else {
          // XXX todo 
          // fix me?  LDAP code gets here
          rv = SetLastModifiedDate(0);
        }
      }
      else
        rv = NS_ERROR_UNEXPECTED;
      break;
    case 'N':
      if (attrname[1] == 'o')
        rv = SetNotes(value);
      else 
        rv = SetNickName(value);
      break;
    case 'P':
      switch (attrname[2]) { 
        case 'e':
          switch (value[0]) {
            case 'h':
            case 'H':
              rv = SetPreferMailFormat(nsIAbPreferMailFormat::html);
              break;
            case 'p':
            case 'P':
              rv = SetPreferMailFormat(nsIAbPreferMailFormat::plaintext);
              break;
            default:
              rv = SetPreferMailFormat(nsIAbPreferMailFormat::unknown);
              break;
          }
          break;
        case 'g':
          rv = SetPagerNumber(value);
          break;
        case 'i':
          rv = SetPrimaryEmail(value);
          break;
        default:
          rv = NS_ERROR_UNEXPECTED;
          break;
      }
      break;
    case 'S':
      rv = SetSecondEmail(value);
      break;
    case 'W': 
      if (attrname[1] == 'e') {
        if (attrname[7] == '1')
          rv = SetWebPage1(value);
        else 
          rv = SetWebPage2(value);
      }
      else {
        switch (attrname[4]) {
          case 'A':
            if (attrname[11] == '\0')
              rv = SetWorkAddress(value);
            else 
              rv = SetWorkAddress2(value);
            break;
          case 'C':
            if (attrname[5] == 'i')
              rv = SetWorkCity(value);
            else 
              rv = SetWorkCountry(value);
            break;
          case 'P':
            rv = SetWorkPhone(value);
            break;
          case 'S':
            rv = SetWorkState(value);
            break;
          case 'Z':
            rv = SetWorkZipCode(value);
            break;
          default:
            rv = NS_ERROR_UNEXPECTED;
            break;
        }
      }
      break;
    default:
      rv = NS_ERROR_UNEXPECTED;
      break;
  }

  NS_ENSURE_SUCCESS(rv,rv);
  return rv;
}

NS_IMETHODIMP
nsAbCardProperty::GetFirstName(PRUnichar * *aFirstName)
{ return GetAttributeName(aFirstName, m_FirstName); }

NS_IMETHODIMP
nsAbCardProperty::GetLastName(PRUnichar * *aLastName)
{ return GetAttributeName(aLastName, m_LastName); }

NS_IMETHODIMP
nsAbCardProperty::GetDisplayName(PRUnichar * *aDisplayName)
{ return GetAttributeName(aDisplayName, m_DisplayName); }

NS_IMETHODIMP
nsAbCardProperty::GetNickName(PRUnichar * *aNickName)
{ return GetAttributeName(aNickName, m_NickName); }

NS_IMETHODIMP
nsAbCardProperty::GetPrimaryEmail(PRUnichar * *aPrimaryEmail)
{ return GetAttributeName(aPrimaryEmail, m_PrimaryEmail); }

NS_IMETHODIMP
nsAbCardProperty::GetSecondEmail(PRUnichar * *aSecondEmail)
{ return GetAttributeName(aSecondEmail, m_SecondEmail); }

NS_IMETHODIMP
nsAbCardProperty::GetWorkPhone(PRUnichar * *aWorkPhone)
{ return GetAttributeName(aWorkPhone, m_WorkPhone); }

NS_IMETHODIMP
nsAbCardProperty::GetHomePhone(PRUnichar * *aHomePhone)
{ return GetAttributeName(aHomePhone, m_HomePhone); }

NS_IMETHODIMP
nsAbCardProperty::GetFaxNumber(PRUnichar * *aFaxNumber)
{ return GetAttributeName(aFaxNumber, m_FaxNumber); }

NS_IMETHODIMP
nsAbCardProperty::GetPagerNumber(PRUnichar * *aPagerNumber)
{ return GetAttributeName(aPagerNumber, m_PagerNumber); }

NS_IMETHODIMP
nsAbCardProperty::GetCellularNumber(PRUnichar * *aCellularNumber)
{ return GetAttributeName(aCellularNumber, m_CellularNumber); }

NS_IMETHODIMP
nsAbCardProperty::GetHomeAddress(PRUnichar * *aHomeAddress)
{ return GetAttributeName(aHomeAddress, m_HomeAddress); }

NS_IMETHODIMP
nsAbCardProperty::GetHomeAddress2(PRUnichar * *aHomeAddress2)
{ return GetAttributeName(aHomeAddress2, m_HomeAddress2); }

NS_IMETHODIMP
nsAbCardProperty::GetHomeCity(PRUnichar * *aHomeCity)
{ return GetAttributeName(aHomeCity, m_HomeCity); }

NS_IMETHODIMP
nsAbCardProperty::GetHomeState(PRUnichar * *aHomeState)
{ return GetAttributeName(aHomeState, m_HomeState); }

NS_IMETHODIMP
nsAbCardProperty::GetHomeZipCode(PRUnichar * *aHomeZipCode)
{ return GetAttributeName(aHomeZipCode, m_HomeZipCode); }

NS_IMETHODIMP
nsAbCardProperty::GetHomeCountry(PRUnichar * *aHomecountry)
{ return GetAttributeName(aHomecountry, m_HomeCountry); }

NS_IMETHODIMP
nsAbCardProperty::GetWorkAddress(PRUnichar * *aWorkAddress)
{ return GetAttributeName(aWorkAddress, m_WorkAddress); }

NS_IMETHODIMP
nsAbCardProperty::GetWorkAddress2(PRUnichar * *aWorkAddress2)
{ return GetAttributeName(aWorkAddress2, m_WorkAddress2); }

NS_IMETHODIMP
nsAbCardProperty::GetWorkCity(PRUnichar * *aWorkCity)
{ return GetAttributeName(aWorkCity, m_WorkCity); }

NS_IMETHODIMP
nsAbCardProperty::GetWorkState(PRUnichar * *aWorkState)
{ return GetAttributeName(aWorkState, m_WorkState); }

NS_IMETHODIMP
nsAbCardProperty::GetWorkZipCode(PRUnichar * *aWorkZipCode)
{ return GetAttributeName(aWorkZipCode, m_WorkZipCode); }

NS_IMETHODIMP
nsAbCardProperty::GetWorkCountry(PRUnichar * *aWorkCountry)
{ return GetAttributeName(aWorkCountry, m_WorkCountry); }

NS_IMETHODIMP
nsAbCardProperty::GetJobTitle(PRUnichar * *aJobTitle)
{ return GetAttributeName(aJobTitle, m_JobTitle); }

NS_IMETHODIMP
nsAbCardProperty::GetDepartment(PRUnichar * *aDepartment)
{ return GetAttributeName(aDepartment, m_Department); }

NS_IMETHODIMP
nsAbCardProperty::GetCompany(PRUnichar * *aCompany)
{ return GetAttributeName(aCompany, m_Company); }

NS_IMETHODIMP
nsAbCardProperty::GetWebPage1(PRUnichar * *aWebPage1)
{ return GetAttributeName(aWebPage1, m_WebPage1); }

NS_IMETHODIMP
nsAbCardProperty::GetWebPage2(PRUnichar * *aWebPage2)
{ return GetAttributeName(aWebPage2, m_WebPage2); }

NS_IMETHODIMP
nsAbCardProperty::GetBirthYear(PRUnichar * *aBirthYear)
{ return GetAttributeName(aBirthYear, m_BirthYear); }

NS_IMETHODIMP
nsAbCardProperty::GetBirthMonth(PRUnichar * *aBirthMonth)
{ return GetAttributeName(aBirthMonth, m_BirthMonth); }

NS_IMETHODIMP
nsAbCardProperty::GetBirthDay(PRUnichar * *aBirthDay)
{ return GetAttributeName(aBirthDay, m_BirthDay); }

NS_IMETHODIMP
nsAbCardProperty::GetCustom1(PRUnichar * *aCustom1)
{ return GetAttributeName(aCustom1, m_Custom1); }

NS_IMETHODIMP
nsAbCardProperty::GetCustom2(PRUnichar * *aCustom2)
{ return GetAttributeName(aCustom2, m_Custom2); }

NS_IMETHODIMP
nsAbCardProperty::GetCustom3(PRUnichar * *aCustom3)
{ return GetAttributeName(aCustom3, m_Custom3); }

NS_IMETHODIMP
nsAbCardProperty::GetCustom4(PRUnichar * *aCustom4)
{ return GetAttributeName(aCustom4, m_Custom4); }

NS_IMETHODIMP
nsAbCardProperty::GetNotes(PRUnichar * *aNotes)
{ return GetAttributeName(aNotes, m_Note); }

NS_IMETHODIMP
nsAbCardProperty::GetLastModifiedDate(PRUint32 *aLastModifiedDate)
{ *aLastModifiedDate = m_LastModDate; return NS_OK; }

NS_IMETHODIMP
nsAbCardProperty::SetFirstName(const PRUnichar * aFirstName)
{ return SetAttributeName(aFirstName, m_FirstName); }

NS_IMETHODIMP
nsAbCardProperty::SetLastName(const PRUnichar * aLastName)
{ return SetAttributeName(aLastName, m_LastName); }

NS_IMETHODIMP
nsAbCardProperty::SetDisplayName(const PRUnichar * aDisplayName)
{ return SetAttributeName(aDisplayName, m_DisplayName); }

NS_IMETHODIMP
nsAbCardProperty::SetNickName(const PRUnichar * aNickName)
{ return SetAttributeName(aNickName, m_NickName); }

NS_IMETHODIMP
nsAbCardProperty::SetPrimaryEmail(const PRUnichar * aPrimaryEmail)
{ return SetAttributeName(aPrimaryEmail, m_PrimaryEmail); }

NS_IMETHODIMP
nsAbCardProperty::SetSecondEmail(const PRUnichar * aSecondEmail)
{ return SetAttributeName(aSecondEmail, m_SecondEmail); }

NS_IMETHODIMP
nsAbCardProperty::SetWorkPhone(const PRUnichar * aWorkPhone)
{ return SetAttributeName(aWorkPhone, m_WorkPhone); }

NS_IMETHODIMP
nsAbCardProperty::SetHomePhone(const PRUnichar * aHomePhone)
{ return SetAttributeName(aHomePhone, m_HomePhone); }

NS_IMETHODIMP
nsAbCardProperty::SetFaxNumber(const PRUnichar * aFaxNumber)
{ return SetAttributeName(aFaxNumber, m_FaxNumber); }

NS_IMETHODIMP
nsAbCardProperty::SetPagerNumber(const PRUnichar * aPagerNumber)
{ return SetAttributeName(aPagerNumber, m_PagerNumber); }

NS_IMETHODIMP
nsAbCardProperty::SetCellularNumber(const PRUnichar * aCellularNumber)
{ return SetAttributeName(aCellularNumber, m_CellularNumber); }

NS_IMETHODIMP
nsAbCardProperty::SetHomeAddress(const PRUnichar * aHomeAddress)
{ return SetAttributeName(aHomeAddress, m_HomeAddress); }

NS_IMETHODIMP
nsAbCardProperty::SetHomeAddress2(const PRUnichar * aHomeAddress2)
{ return SetAttributeName(aHomeAddress2, m_HomeAddress2); }

NS_IMETHODIMP
nsAbCardProperty::SetHomeCity(const PRUnichar * aHomeCity)
{ return SetAttributeName(aHomeCity, m_HomeCity); }

NS_IMETHODIMP
nsAbCardProperty::SetHomeState(const PRUnichar * aHomeState)
{ return SetAttributeName(aHomeState, m_HomeState); }

NS_IMETHODIMP
nsAbCardProperty::SetHomeZipCode(const PRUnichar * aHomeZipCode)
{ return SetAttributeName(aHomeZipCode, m_HomeZipCode); }

NS_IMETHODIMP
nsAbCardProperty::SetHomeCountry(const PRUnichar * aHomecountry)
{ return SetAttributeName(aHomecountry, m_HomeCountry); }

NS_IMETHODIMP
nsAbCardProperty::SetWorkAddress(const PRUnichar * aWorkAddress)
{ return SetAttributeName(aWorkAddress, m_WorkAddress); }

NS_IMETHODIMP
nsAbCardProperty::SetWorkAddress2(const PRUnichar * aWorkAddress2)
{ return SetAttributeName(aWorkAddress2, m_WorkAddress2); }

NS_IMETHODIMP
nsAbCardProperty::SetWorkCity(const PRUnichar * aWorkCity)
{ return SetAttributeName(aWorkCity, m_WorkCity); }

NS_IMETHODIMP
nsAbCardProperty::SetWorkState(const PRUnichar * aWorkState)
{ return SetAttributeName(aWorkState, m_WorkState); }

NS_IMETHODIMP
nsAbCardProperty::SetWorkZipCode(const PRUnichar * aWorkZipCode)
{ return SetAttributeName(aWorkZipCode, m_WorkZipCode); }

NS_IMETHODIMP
nsAbCardProperty::SetWorkCountry(const PRUnichar * aWorkCountry)
{ return SetAttributeName(aWorkCountry, m_WorkCountry); }

NS_IMETHODIMP
nsAbCardProperty::SetJobTitle(const PRUnichar * aJobTitle)
{ return SetAttributeName(aJobTitle, m_JobTitle); }

NS_IMETHODIMP
nsAbCardProperty::SetDepartment(const PRUnichar * aDepartment)
{ return SetAttributeName(aDepartment, m_Department); }

NS_IMETHODIMP
nsAbCardProperty::SetCompany(const PRUnichar * aCompany)
{ return SetAttributeName(aCompany, m_Company); }

NS_IMETHODIMP
nsAbCardProperty::SetWebPage1(const PRUnichar * aWebPage1)
{ return SetAttributeName(aWebPage1, m_WebPage1); }

NS_IMETHODIMP
nsAbCardProperty::SetWebPage2(const PRUnichar * aWebPage2)
{ return SetAttributeName(aWebPage2, m_WebPage2); }

NS_IMETHODIMP
nsAbCardProperty::SetBirthYear(const PRUnichar * aBirthYear)
{ return SetAttributeName(aBirthYear, m_BirthYear); }

NS_IMETHODIMP
nsAbCardProperty::SetBirthMonth(const PRUnichar * aBirthMonth)
{ return SetAttributeName(aBirthMonth, m_BirthMonth); }

NS_IMETHODIMP
nsAbCardProperty::SetBirthDay(const PRUnichar * aBirthDay)
{ return SetAttributeName(aBirthDay, m_BirthDay); }

NS_IMETHODIMP
nsAbCardProperty::SetCustom1(const PRUnichar * aCustom1)
{ return SetAttributeName(aCustom1, m_Custom1); }

NS_IMETHODIMP
nsAbCardProperty::SetCustom2(const PRUnichar * aCustom2)
{ return SetAttributeName(aCustom2, m_Custom2); }

NS_IMETHODIMP
nsAbCardProperty::SetCustom3(const PRUnichar * aCustom3)
{ return SetAttributeName(aCustom3, m_Custom3); }

NS_IMETHODIMP
nsAbCardProperty::SetCustom4(const PRUnichar * aCustom4)
{ return SetAttributeName(aCustom4, m_Custom4); }

NS_IMETHODIMP
nsAbCardProperty::SetNotes(const PRUnichar * aNotes)
{ return SetAttributeName(aNotes, m_Note); }

NS_IMETHODIMP
nsAbCardProperty::SetLastModifiedDate(PRUint32 aLastModifiedDate)
{ return m_LastModDate = aLastModifiedDate; }

NS_IMETHODIMP nsAbCardProperty::Copy(nsIAbCard* srcCard)
{
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

	PRUint32 format;
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

	return NS_OK;
}

NS_IMETHODIMP nsAbCardProperty::EditCardToDatabase(const char *uri)
{
	return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsAbCardProperty::Equals(nsIAbCard *card, PRBool *result)
{
  *result = (card == this);
  return NS_OK;
}
