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

#include "nsHTMLIFrameRootAccessible.h"
#include "nsCOMPtr.h"
#include "nsIDocument.h"
#include "nsIPresShell.h"
#include "nsIContent.h"
#include "nsIFrame.h"
#include "nsIDOMDocument.h"
#include "nsReadableUtils.h"

NS_IMPL_ISUPPORTS_INHERITED0(nsHTMLIFrameRootAccessible, nsRootAccessible);
NS_IMPL_ISUPPORTS_INHERITED3(nsHTMLIFrameAccessible, nsBlockAccessible, nsIAccessibleDocument, nsIAccessibleHyperText, nsIAccessibleEventReceiver)

nsHTMLIFrameAccessible::nsHTMLIFrameAccessible(nsIDOMNode* aNode, nsIAccessible* aRoot, nsIWeakReference* aShell, nsIDocument *aDoc):
  nsBlockAccessible(aNode, aShell), nsDocAccessibleMixin(aDoc), mRootAccessible(aRoot)
{
}

  /* attribute wstring accName; */
NS_IMETHODIMP nsHTMLIFrameAccessible::GetAccName(nsAString& aAccName) 
{ 
  nsresult rv = GetTitle(aAccName);
  if (NS_FAILED(rv) || aAccName.IsEmpty())
    rv = GetURL(aAccName);
  return rv;
}

NS_IMETHODIMP nsHTMLIFrameAccessible::GetAccValue(nsAString& aAccValue) 
{ 
  return NS_OK;
}

/* nsIAccessible getAccFirstChild (); */
NS_IMETHODIMP nsHTMLIFrameAccessible::GetAccFirstChild(nsIAccessible **aChild)
{
  NS_IF_ADDREF(*aChild = mRootAccessible);
  return NS_OK;
}

/* nsIAccessible getAccLastChild (); */
NS_IMETHODIMP nsHTMLIFrameAccessible::GetAccLastChild(nsIAccessible **aChild)
{
  NS_IF_ADDREF(*aChild = mRootAccessible);
  return NS_OK;
}

/* long getAccChildCount (); */
NS_IMETHODIMP nsHTMLIFrameAccessible::GetAccChildCount(PRInt32 *aNumChildren)
{
  *aNumChildren = 1;
  return NS_OK;
}

/* unsigned long getAccRole (); */
NS_IMETHODIMP nsHTMLIFrameAccessible::GetAccRole(PRUint32 *_retval)
{
  *_retval = ROLE_CLIENT;
  return NS_OK;
}

NS_IMETHODIMP nsHTMLIFrameAccessible::GetAccState(PRUint32 *aAccState)
{
  return nsAccessible::GetAccState(aAccState);
}

NS_IMETHODIMP nsHTMLIFrameAccessible::GetURL(nsAString& aURL)
{
  return nsDocAccessibleMixin::GetURL(aURL);
}

NS_IMETHODIMP nsHTMLIFrameAccessible::GetTitle(nsAString& aTitle)
{
  return nsDocAccessibleMixin::GetTitle(aTitle);
}

NS_IMETHODIMP nsHTMLIFrameAccessible::GetMimeType(nsAString& aMimeType)
{
  return nsDocAccessibleMixin::GetMimeType(aMimeType);
}

NS_IMETHODIMP nsHTMLIFrameAccessible::GetDocType(nsAString& aDocType)
{
  return nsDocAccessibleMixin::GetDocType(aDocType);
}

NS_IMETHODIMP nsHTMLIFrameAccessible::GetNameSpaceURIForID(PRInt16 aNameSpaceID, nsAString& aNameSpaceURI)
{
  return nsDocAccessibleMixin::GetNameSpaceURIForID(aNameSpaceID, aNameSpaceURI);
}

NS_IMETHODIMP nsHTMLIFrameAccessible::GetDocument(nsIDocument **doc)
{
  return nsDocAccessibleMixin::GetDocument(doc);
}

NS_IMETHODIMP nsHTMLIFrameAccessible::GetCaretAccessible(nsIAccessibleCaret **aCaretAccessible)
{
  // Caret only owned by top level window's document
  *aCaretAccessible = nsnull;
  return NS_OK;
}

// ------- nsIAccessibleHyperText ---------------
/* readonly attribute long links; */
NS_IMETHODIMP nsHTMLIFrameAccessible::GetLinks(PRInt32 *aLinks)
{
  *aLinks = GetLinksFromAccNode(this);
  return NS_OK;
}

/* nsIAccessibleHyperLink getLink (in long index); */
NS_IMETHODIMP nsHTMLIFrameAccessible::GetLink(PRInt32 aIndex,
                                              nsIAccessibleHyperLink **_retval)
{
  return GetLinkFromAccNode(aIndex, this, _retval);
}

/* long getLinkIndex (in long charIndex); */
NS_IMETHODIMP nsHTMLIFrameAccessible::GetLinkIndex(PRInt32 aCharIndex,
                                                   PRInt32 *_retval)
{
  return GetLinkIndexFromAccNode(this, aCharIndex, _retval);
}

//helper function for nsIAccessibleHyperText
PRBool nsHTMLIFrameAccessible::IsHyperLink(nsIAccessible *aAccNode)
{
  nsCOMPtr<nsIAccessibleHyperLink> hyperlink(do_QueryInterface(aAccNode));
  return hyperlink? PR_TRUE: PR_FALSE;
}

PRInt32 nsHTMLIFrameAccessible::GetLinksFromAccNode(nsIAccessible *aAccNode)
{
  PRInt32 rv = IsHyperLink(aAccNode) ? 1 : 0;
  
  // Here, all the links of accChild should be summed up
  nsCOMPtr<nsIAccessible> childa;
  nsCOMPtr<nsIAccessible> child;

  aAccNode->GetAccFirstChild(getter_AddRefs(child));
  while (child) {
    rv += GetLinksFromAccNode(child);
    child->GetAccNextSibling(getter_AddRefs(childa));
    child = childa;
  }
  //end for summing up accChildren's links

  return rv;
}

nsresult nsHTMLIFrameAccessible::GetLinkFromAccNode(PRInt32 aIndex,
                                                    nsIAccessible *aAccNode,
                                                    nsIAccessibleHyperLink **_retval)
{
  PRInt32 links;

  //firstly, to see whether beginning node  is a hyperlink
  links = 0;
  if (aIndex < 0) {
    //of course, the aIndex  is not right.
    *_retval = nsnull;
    return NS_ERROR_INVALID_ARG;
  }
  if (IsHyperLink(aAccNode)) {
    links = 1;
    if (0 == aIndex) {
      return CallQueryInterface(aAccNode, _retval);
    }
  }

  //navigate all accChildren to getLink for the aIndex
  nsCOMPtr<nsIAccessible> child;
  nsCOMPtr<nsIAccessible> childa;
 
  aIndex = aIndex - links;
  aAccNode->GetAccFirstChild(getter_AddRefs(child));
  while (child) {
    links = GetLinksFromAccNode(child);
    if (aIndex < links) {
      return GetLinkFromAccNode(aIndex, child, _retval);
    }
    aIndex -= links;
    child->GetAccNextSibling(getter_AddRefs(childa));
    child = childa;
  }
  //end of navigate accChild

  *_retval = nsnull;
  return NS_ERROR_INVALID_ARG;
}

PRInt32 nsHTMLIFrameAccessible::GetAccNodeCharLength(nsIAccessible *aAccNode)
{
  PRUint32 role;
  PRInt32 childCharLength;
  nsAutoString tempAccName;

  PRInt32 rv = 0;
  role = ROLE_NOTHING;
  aAccNode->GetAccRole(&role);

  //ROLE_TEXT
  if (ROLE_TEXT == role) {
    aAccNode->GetAccName(tempAccName);
    rv = tempAccName.Length();
  }

  //sum up the number all accChildren's characters
  nsCOMPtr<nsIAccessible> childa;
  nsCOMPtr<nsIAccessible> child;

  aAccNode->GetAccFirstChild(getter_AddRefs(child));
  while (child) {
    childCharLength = GetAccNodeCharLength(child);
    rv += childCharLength;
    child->GetAccNextSibling(getter_AddRefs(childa));
    child = childa;
  }
  return rv;
}

// *_retval is zero-based index, so -1 for no index.
// be careful, charIndex is zero-index, but length is not.
nsresult nsHTMLIFrameAccessible::GetLinkIndexFromAccNode(nsIAccessible *aAccNode,
                                                         PRInt32 aCharIndex,
                                                         PRInt32 *_retval)
{
  nsresult rv;
  PRUint32 role;
  nsAutoString tempAccName;
  PRInt32 charLength;
  PRInt32 links;

  *_retval = -1;
  links = 0;
  charLength = 0;
  role = ROLE_NOTHING;
  aAccNode->GetAccRole(&role);

  // if the beginning node is hyperlink
  if (IsHyperLink(aAccNode)) {
    charLength = GetAccNodeCharLength(aAccNode);
    if (aCharIndex < charLength) {
      *_retval = 0;
      return NS_OK;
    }
    *_retval = -1;
    return  NS_OK;
  }

  //begin to count char length
  if (ROLE_TEXT == role) {
    aAccNode->GetAccName(tempAccName);
    charLength = tempAccName.Length();
    if (aCharIndex < charLength) {
      *_retval = -1;
      return NS_OK;
    }
    return NS_ERROR_INVALID_ARG;
  }
  if (IsHyperLink(aAccNode)) {
    links = 1;
  }
  //exam the beginning node  end

  //getLinkIndex from accChildren
  nsCOMPtr<nsIAccessible> childa;
  nsCOMPtr<nsIAccessible> child;
  PRInt32 childCharLength;
  PRInt32 childLinks;

  aAccNode->GetAccFirstChild(getter_AddRefs(child));
  while (child) {
    childCharLength = GetAccNodeCharLength(child);
    if (aCharIndex < charLength + childCharLength) {
      rv = GetLinkIndexFromAccNode(child, aCharIndex - charLength, _retval);
      *_retval += links;
      return rv;
    }
    charLength += childCharLength;
    childLinks = GetLinksFromAccNode(child);
    links += childLinks;
    child->GetAccNextSibling(getter_AddRefs(childa));
    child = childa;
  }

  return NS_ERROR_INVALID_ARG;
}

NS_IMETHODIMP nsHTMLIFrameAccessible::AddAccessibleEventListener(nsIAccessibleEventListener *aListener)
{
  nsCOMPtr<nsIAccessibleEventReceiver> rootReceiver(do_QueryInterface(mRootAccessible));
  NS_ENSURE_TRUE(rootReceiver, NS_OK);
  return rootReceiver->AddAccessibleEventListener(aListener);
}

NS_IMETHODIMP nsHTMLIFrameAccessible::RemoveAccessibleEventListener()
{
  nsCOMPtr<nsIAccessibleEventReceiver> rootReceiver(do_QueryInterface(mRootAccessible));
  NS_ENSURE_TRUE(rootReceiver, NS_OK);
  return rootReceiver->RemoveAccessibleEventListener();
}


//=============================//
// nsHTMLIFrameRootAccessible  //
//=============================//

//-----------------------------------------------------
// construction 
//-----------------------------------------------------
nsHTMLIFrameRootAccessible::nsHTMLIFrameRootAccessible(nsIWeakReference* aShell):
  nsRootAccessible(aShell)
{
}

//-----------------------------------------------------
// destruction
//-----------------------------------------------------
nsHTMLIFrameRootAccessible::~nsHTMLIFrameRootAccessible()
{
}

/* void addAccessibleEventListener (in nsIAccessibleEventListener aListener); */
NS_IMETHODIMP nsHTMLIFrameRootAccessible::AddAccessibleEventListener(nsIAccessibleEventListener *aListener)
{
  NS_ASSERTION(aListener, "Trying to add a null listener!");
  if (!mListener) {
    mListener = aListener;
    AddContentDocListeners();
  }
  return NS_OK;
}

/* void removeAccessibleEventListener (); */
NS_IMETHODIMP nsHTMLIFrameRootAccessible::RemoveAccessibleEventListener()
{
  if (mListener) {
    RemoveContentDocListeners();
    mListener = nsnull;
  }
  return NS_OK;
}

NS_IMETHODIMP nsHTMLIFrameRootAccessible::GetAccRole(PRUint32 *_retval)
{
  *_retval = ROLE_PANE;
  return NS_OK;
}

NS_IMETHODIMP nsHTMLIFrameRootAccessible::GetAccName(nsAString& aAccName)
{
  return NS_OK;
}

NS_IMETHODIMP nsHTMLIFrameRootAccessible::GetAccValue(nsAString& aAccValue)
{
  return GetURL(aAccValue);
}
