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
 * Author: Eric Vaughan (evaughan@netscape.com)
 *
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

#include "nsGenericAccessible.h"
#include "nsCOMPtr.h"
#include "nsIWeakReference.h"
#include "nsReadableUtils.h"
#include "nsIContent.h"

/* Implementation file */
NS_IMPL_ISUPPORTS1(nsGenericAccessible, nsIAccessible)

nsGenericAccessible::nsGenericAccessible()
{
  NS_INIT_ISUPPORTS();
  /* member initializers and constructor code */
}

nsGenericAccessible::~nsGenericAccessible()
{
  /* destructor code */
}

/* nsIAccessible getAccParent (); */
NS_IMETHODIMP nsGenericAccessible::GetAccParent(nsIAccessible **_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* nsIAccessible getAccNextSibling (); */
NS_IMETHODIMP nsGenericAccessible::GetAccNextSibling(nsIAccessible **_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* nsIAccessible getAccPreviousSibling (); */
NS_IMETHODIMP nsGenericAccessible::GetAccPreviousSibling(nsIAccessible **_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* nsIAccessible getAccFirstChild (); */
NS_IMETHODIMP nsGenericAccessible::GetAccFirstChild(nsIAccessible **_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* nsIAccessible getAccLastChild (); */
NS_IMETHODIMP nsGenericAccessible::GetAccLastChild(nsIAccessible **_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* long getAccChildCount (); */
NS_IMETHODIMP nsGenericAccessible::GetAccChildCount(PRInt32 *_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* DOMString getAccName (); */
NS_IMETHODIMP nsGenericAccessible::GetAccName(nsAString& _retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* DOMString getAccValue (); */
NS_IMETHODIMP nsGenericAccessible::GetAccValue(nsAString& _retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute long accId; */
NS_IMETHODIMP nsGenericAccessible::GetAccId(PRInt32 *aAccId)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* void setAccName (in DOMString name); */
NS_IMETHODIMP nsGenericAccessible::SetAccName(const nsAString& name)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* DOMString getAccDescription (); */
NS_IMETHODIMP nsGenericAccessible::GetAccDescription(nsAString& _retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* DOMString getAccKeyboardShortcut (); */
NS_IMETHODIMP nsGenericAccessible::GetAccKeyboardShortcut(nsAString& _retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* unsigned long getAccRole (); */
NS_IMETHODIMP nsGenericAccessible::GetAccRole(PRUint32 *_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* DOMString getAccState (); */
NS_IMETHODIMP nsGenericAccessible::GetAccState(PRUint32 *_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* PRUint8 getAccNumActions (); */
NS_IMETHODIMP nsGenericAccessible::GetAccNumActions(PRUint8 *_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* DOMString getAccActionName (in PRUint8 index); */
NS_IMETHODIMP nsGenericAccessible::GetAccActionName(PRUint8 index, nsAString& _retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* void accDoAction (in PRUint8 index); */
NS_IMETHODIMP nsGenericAccessible::AccDoAction(PRUint8 index)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* nsIAccessible getAccFocused(); */
NS_IMETHODIMP nsGenericAccessible::GetAccFocused(nsIAccessible **_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* DOMString getAccHelp (); */
NS_IMETHODIMP nsGenericAccessible::GetAccHelp(nsAString& _retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* nsIAccessible accGetAt (in long x, in long y); */
NS_IMETHODIMP nsGenericAccessible::AccGetAt(PRInt32 x, PRInt32 y, nsIAccessible **_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* nsIAccessible accNavigateRight (); */
NS_IMETHODIMP nsGenericAccessible::AccNavigateRight(nsIAccessible **_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* nsIAccessible accNavigateLeft (); */
NS_IMETHODIMP nsGenericAccessible::AccNavigateLeft(nsIAccessible **_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* nsIAccessible accNavigateUp (); */
NS_IMETHODIMP nsGenericAccessible::AccNavigateUp(nsIAccessible **_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* nsIAccessible accNavigateDown (); */
NS_IMETHODIMP nsGenericAccessible::AccNavigateDown(nsIAccessible **_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* void accGetBounds (out long x, out long y, out long width, out long height); */
NS_IMETHODIMP nsGenericAccessible::AccGetBounds(PRInt32 *x, PRInt32 *y, PRInt32 *width, PRInt32 *height)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* void accAddSelection (); */
NS_IMETHODIMP nsGenericAccessible::AccAddSelection()
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* void accRemoveSelection (); */
NS_IMETHODIMP nsGenericAccessible::AccRemoveSelection()
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* void accExtendSelection (); */
NS_IMETHODIMP nsGenericAccessible::AccExtendSelection()
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* void accTakeSelection (); */
NS_IMETHODIMP nsGenericAccessible::AccTakeSelection()
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* void accTakeFocus (); */
NS_IMETHODIMP nsGenericAccessible::AccTakeFocus()
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* unsigned long getAccExtState (); */
NS_IMETHODIMP nsGenericAccessible::GetAccExtState(PRUint32 *_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsGenericAccessible::AccGetDOMNode(nsIDOMNode **_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsGenericAccessible::CacheOptimizations(nsIAccessible *aParent, PRInt32 aSiblingIndex, nsIDOMNodeList *aSiblingList)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}
