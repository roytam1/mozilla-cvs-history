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

#include "nsAOLCiter.h"

#include "nsWrapUtils.h"

/** Mail citations using the AOL style >> This is a citation <<
  */

nsAOLCiter::nsAOLCiter()
{
  NS_INIT_REFCNT();
}

nsAOLCiter::~nsAOLCiter()
{
}

NS_IMPL_ADDREF(nsAOLCiter)

NS_IMPL_RELEASE(nsAOLCiter)

NS_IMETHODIMP
nsAOLCiter::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
  if (nsnull == aInstancePtr) {
    return NS_ERROR_NULL_POINTER;
  }
  if (aIID.Equals(NS_GET_IID(nsISupports)))
  {
    *aInstancePtr = (void*)this;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  if (aIID.Equals(NS_GET_IID(nsICiter))) {
    *aInstancePtr = (void*)(nsICiter*)this;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  return NS_NOINTERFACE;
}

NS_IMETHODIMP
nsAOLCiter::GetCiteString(const nsAReadableString& aInString, nsAWritableString& aOutString)
{
  aOutString = NS_LITERAL_STRING("\n\n>> ");
  aOutString += aInString;

  // See if the last char is a newline, and replace it if so
  PRUnichar newline ('\n');
  if (aOutString.Last() == newline)
  {
    aOutString.Append(PRUnichar(' '));
    aOutString.Append(NS_LITERAL_STRING("<<\n"));
  }
  else
  {
    aOutString.Append(NS_LITERAL_STRING(" <<\n"));
  }

  return NS_OK;
}

NS_IMETHODIMP
nsAOLCiter::StripCites(const nsAReadableString& aInString, nsAWritableString& aOutString)
{
  // Remove the beginning cites, if any:
  nsAutoString tInputString(aInString);//MJUDGE SCC NEED HELP
  nsAutoString tOutputString;
  if (tInputString.EqualsWithConversion(">>", PR_FALSE, 2))
  {
    PRInt32 i = 3;
    while (nsCRT::IsAsciiSpace(tInputString[i]))
      ++i;
    tOutputString.Append(tInputString.GetUnicode(), i);
  }
  else
    tOutputString = tInputString;

  // Remove the end cites, if any:
  tOutputString.Trim("<", PR_FALSE, PR_TRUE, PR_FALSE);
  aOutString.Assign(tOutputString);
  return NS_OK;
}

NS_IMETHODIMP
nsAOLCiter::Rewrap(const nsAReadableString& aInString,
                   PRUint32 aWrapCol, PRUint32 aFirstLineOffset,
                   PRBool aRespectNewlines,
                   nsAWritableString& aOutString)
{
  nsString citeString;
  return nsWrapUtils::Rewrap(aInString, aWrapCol, aFirstLineOffset,
                             aRespectNewlines, citeString,
                             aOutString);
}

