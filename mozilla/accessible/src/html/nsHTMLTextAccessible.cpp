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
 * Author: Eric Vaughan (evaughan@netscape.com)
 * Contributor(s): 
 */

#include "nsHTMLTextAccessible.h"
#include "nsWeakReference.h"
#include "nsIFrame.h"
#include "nsILink.h"
#include "nsILinkHandler.h"
#include "nsISelection.h"
#include "nsISelectionController.h"
#include "nsIPresContext.h"
#include "nsReadableUtils.h"

nsHTMLTextAccessible::nsHTMLTextAccessible(nsIPresShell* aShell, nsIDOMNode* aDomNode):
nsLinkableAccessible(aShell, aDomNode)
{ 
}

/* wstring getAccName (); */
NS_IMETHODIMP nsHTMLTextAccessible::GetAccName(PRUnichar **_retval)
{ 

  nsAutoString nameString;
  nsresult rv = NS_OK;
  //if (IsALink()) {
  //  rv = AppendFlatStringFromSubtree(mLinkContent, &nameString);
  //}
  //else 
  mNode->GetNodeValue(nameString);
  nameString.CompressWhitespace();
  *_retval = nameString.ToNewUnicode();
  return rv;
}

/* wstring getAccRole (); */
NS_IMETHODIMP nsHTMLTextAccessible::GetAccRole(PRUnichar **_retval)
{
  *_retval = ToNewUnicode(NS_LITERAL_STRING("static text"));

  return NS_OK;
}


/* nsIAccessible getAccFirstChild (); */
NS_IMETHODIMP nsHTMLTextAccessible::GetAccFirstChild(nsIAccessible **_retval)
{
  *_retval = nsnull;
  return NS_OK;
}

/* nsIAccessible getAccLastChild (); */
NS_IMETHODIMP nsHTMLTextAccessible::GetAccLastChild(nsIAccessible **_retval)
{
  *_retval = nsnull;
  return NS_OK;
}

/* long getAccChildCount (); */
NS_IMETHODIMP nsHTMLTextAccessible::GetAccChildCount(PRInt32 *_retval)
{
  *_retval = 0;
  return NS_OK;
}
