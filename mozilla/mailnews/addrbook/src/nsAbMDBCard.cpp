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

#include "nsAbMDBCard.h"

nsAbMDBCard::nsAbMDBCard(void)
  : m_key(0),
    m_dbTableID(0),
    m_dbRowID(0)
{
}

nsAbMDBCard::~nsAbMDBCard(void)
{
}

NS_IMPL_ISUPPORTS_INHERITED1(nsAbMDBCard, nsAbCardProperty, nsIAbMDBCard)

NS_IMETHODIMP nsAbMDBCard::GetDbTableID(PRUint32 *aDbTableID)
{
  *aDbTableID = m_dbTableID;
  return NS_OK;
}

NS_IMETHODIMP nsAbMDBCard::SetDbTableID(PRUint32 aDbTableID)
{
  m_dbTableID = aDbTableID;
  return NS_OK;
}

NS_IMETHODIMP nsAbMDBCard::GetDbRowID(PRUint32 *aDbRowID)
{
  *aDbRowID = m_dbRowID;
  return NS_OK;
}

NS_IMETHODIMP nsAbMDBCard::SetDbRowID(PRUint32 aDbRowID)
{
  m_dbRowID = aDbRowID;
  return NS_OK;
}

NS_IMETHODIMP nsAbMDBCard::GetKey(PRUint32 *aKey)
{
  *aKey = m_key;
  return NS_OK;
}

NS_IMETHODIMP nsAbMDBCard::SetKey(PRUint32 key)
{
  m_key = key;
  return NS_OK;
}

NS_IMETHODIMP nsAbMDBCard::SetAbDatabase(nsIAddrDatabase* database)
{
  mCardDatabase = database;
  return NS_OK;
}

NS_IMETHODIMP nsAbMDBCard::SetStringAttribute(const char *name, const PRUnichar *value)
{
  NS_ASSERTION(mCardDatabase, "no db");
  if (!mCardDatabase)
    return NS_ERROR_UNEXPECTED;

  return mCardDatabase->SetCardValue(this, name, value, PR_TRUE /* notify */);
}  

NS_IMETHODIMP nsAbMDBCard::GetStringAttribute(const char *name, PRUnichar **value)
{
  NS_ASSERTION(mCardDatabase, "no db");
  if (!mCardDatabase)
    return NS_ERROR_UNEXPECTED;

  return mCardDatabase->GetCardValue(this, name, value);
}

NS_IMETHODIMP nsAbMDBCard::Equals(nsIAbCard *card, PRBool *result)
{
  nsresult rv;

  if (this == card) {
    *result = PR_TRUE;
    return NS_OK;
  }

  // the reason we need this card at all is that multiple nsIAbCards
  // can exist for a given mdbcard
  nsCOMPtr <nsIAbMDBCard> mdbcard = do_QueryInterface(card, &rv);
  if (NS_FAILED(rv) || !mdbcard) {
    // XXX using ldap can get us here, we need to fix how the listeners work
    *result = PR_FALSE;
    return NS_OK;
  }

  // XXX todo
  // optimize this code, key might be enough
  PRUint32 dbRowID;
  rv = mdbcard->GetDbRowID(&dbRowID);
  NS_ENSURE_SUCCESS(rv,rv);

  PRUint32 dbTableID;
  rv = mdbcard->GetDbTableID(&dbTableID);
  NS_ENSURE_SUCCESS(rv,rv);

  PRUint32 key;
  rv = mdbcard->GetKey(&key);
  NS_ENSURE_SUCCESS(rv,rv);

  if (dbRowID == m_dbRowID && dbTableID == m_dbTableID && key == m_key)
    *result = PR_TRUE;
  else
    *result = PR_FALSE;
  return NS_OK;
}

