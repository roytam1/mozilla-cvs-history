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
 *          John Gaunt (jgaunt@netscape.com)
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

#include "nsBaseWidgetAccessible.h"
#include "nsIAccessibilityService.h"
#include "nsAccessibleWrap.h"
#include "nsGUIEvent.h"
#include "nsILink.h"
#include "nsIPresContext.h"
#include "nsIPresShell.h"
#include "nsIServiceManager.h"

// ------------
// nsBlockAccessible
// ------------

nsBlockAccessible::nsBlockAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell):nsAccessibleWrap(aNode, aShell)
{
}

NS_IMPL_ISUPPORTS_INHERITED0(nsBlockAccessible, nsAccessible)

/* nsIAccessible accGetAt (in long x, in long y); */
NS_IMETHODIMP nsBlockAccessible::AccGetAt(PRInt32 tx, PRInt32 ty, nsIAccessible **_retval)
{
  // We're going to find the child that contains coordinates (tx,ty)
  PRInt32 x,y,w,h;
  AccGetBounds(&x,&y,&w,&h);  // Get bounds for this accessible
  if (tx >= x && tx < x + w && ty >= y && ty < y + h)
  {
    // It's within this nsIAccessible, let's drill down
    nsCOMPtr<nsIAccessible> child;
    nsCOMPtr<nsIAccessible> smallestChild;
    PRInt32 smallestArea = -1;
    nsCOMPtr<nsIAccessible> next;
    GetAccFirstChild(getter_AddRefs(child));
    PRInt32 cx,cy,cw,ch;  // Child bounds

    while(child) {
      child->AccGetBounds(&cx,&cy,&cw,&ch);
      
      // ok if there are multiple frames the contain the point 
      // and they overlap then pick the smallest. We need to do this
      // for text frames.
      
      // For example, A point that's in block #2 is also in block #1, but we want to return #2:
      //
      // [[block #1 is long wrapped text that continues to
      // another line]]  [[here is a shorter block #2]]

      if (tx >= cx && tx < cx + cw && ty >= cy && ty < cy + ch) 
      {
        if (smallestArea == -1 || cw*ch < smallestArea) {
          smallestArea = cw*ch;
          smallestChild = child;
        }
      }
      child->GetAccNextSibling(getter_AddRefs(next));
      child = next;
    }

    if (smallestChild != nsnull)
    {
      *_retval = smallestChild;
      NS_ADDREF(*_retval);
      return NS_OK;
    }

    *_retval = this;
    NS_ADDREF(this);
    return NS_OK;
  }

  *_retval = nsnull;
  return NS_OK;
}

/**
  * nsContainerAccessible
  */
nsContainerAccessible::nsContainerAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell):
nsAccessibleWrap(aNode, aShell)
{
}

NS_IMPL_ISUPPORTS_INHERITED0(nsContainerAccessible, nsAccessible)

/** no actions */
NS_IMETHODIMP nsContainerAccessible::GetAccNumActions(PRUint8 *_retval)
{
  *_retval = eNo_Action;
  return NS_OK;
}

/** no actions */
NS_IMETHODIMP nsContainerAccessible::GetAccActionName(PRUint8 index, nsAString& _retval)
{
  return NS_OK;
}

/** no actions */
NS_IMETHODIMP nsContainerAccessible::AccDoAction(PRUint8 index)
{
  return NS_OK;
}

/** no state -- normal */
NS_IMETHODIMP nsContainerAccessible::GetAccState(PRUint32 *_retval)
{
  *_retval = 0;
  return NS_OK;
}

/** no value */
NS_IMETHODIMP nsContainerAccessible::GetAccValue(nsAString& _retval)
{
  return NS_OK;
}

/** no name*/
NS_IMETHODIMP nsContainerAccessible::GetAccName(nsAString& _retval)
{
  return NS_OK;
}


//-------------
// nsLeafFrameAccessible
//-------------

nsLeafAccessible::nsLeafAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell):
nsAccessibleWrap(aNode, aShell)
{
}

NS_IMPL_ISUPPORTS_INHERITED0(nsLeafAccessible, nsAccessible)

/* nsIAccessible getAccFirstChild (); */
NS_IMETHODIMP nsLeafAccessible::GetAccFirstChild(nsIAccessible **_retval)
{
  *_retval = nsnull;
  return NS_OK;
}

/* nsIAccessible getAccLastChild (); */
NS_IMETHODIMP nsLeafAccessible::GetAccLastChild(nsIAccessible **_retval)
{
  *_retval = nsnull;
  return NS_OK;
}

/* long getAccChildCount (); */
NS_IMETHODIMP nsLeafAccessible::GetAccChildCount(PRInt32 *_retval)
{
  *_retval = 0;
  return NS_OK;
}


//----------------
// nsLinkableAccessible
//----------------

nsLinkableAccessible::nsLinkableAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell) :
  nsAccessibleWrap(aNode, aShell),
  mLinkContent(nsnull),
  mIsALinkCached(PR_FALSE),
  mIsLinkVisited(PR_FALSE)
{ 
}

NS_IMPL_ISUPPORTS_INHERITED0(nsLinkableAccessible, nsAccessible)

NS_IMETHODIMP nsLinkableAccessible::AccTakeFocus()
{ 
  if (IsALink()) {
    mLinkContent->SetFocus(nsCOMPtr<nsIPresContext>(GetPresContext()));
  }
  
  return NS_OK;
}

/* long GetAccState (); */
NS_IMETHODIMP nsLinkableAccessible::GetAccState(PRUint32 *aState)
{
  nsAccessible::GetAccState(aState);
  if (IsALink()) {
    *aState |= STATE_LINKED;
    if (mIsLinkVisited)
      *aState |= STATE_TRAVERSED;
  }
  
  if (IsALink()) {
    // Make sure we also include all the states of the parent link, such as focusable, focused, etc.
    PRUint32 role;
    GetAccRole(&role);
    if (role != ROLE_LINK) {
      nsCOMPtr<nsIAccessible> parentAccessible;
      GetAccParent(getter_AddRefs(parentAccessible));
      if (parentAccessible) {
        PRUint32 orState = 0;
        parentAccessible->GetAccState(&orState);
        *aState |= orState;
      }
    }
  }

  nsCOMPtr<nsIAccessibleDocument> docAccessible(GetDocAccessible());
  if (docAccessible) {
    PRBool isEditable;
    docAccessible->GetIsEditable(&isEditable);
    if (isEditable) {
      *aState &= ~(STATE_FOCUSED | STATE_FOCUSABLE); // Links not focusable in editor
    }
  }
  return NS_OK;
}


NS_IMETHODIMP nsLinkableAccessible::GetAccValue(nsAString& _retval)
{
  if (IsALink()) {
    nsCOMPtr<nsIDOMNode> linkNode(do_QueryInterface(mLinkContent));
    nsCOMPtr<nsIPresShell> presShell(do_QueryReferent(mWeakShell));
    if (linkNode && presShell)
      return presShell->GetLinkLocation(linkNode, _retval);
  }
  return NS_ERROR_NOT_IMPLEMENTED;
}


/* PRUint8 getAccNumActions (); */
NS_IMETHODIMP nsLinkableAccessible::GetAccNumActions(PRUint8 *_retval)
{
  *_retval = eSingle_Action;
  return NS_OK;
}

/* wstring getAccActionName (in PRUint8 index); */
NS_IMETHODIMP nsLinkableAccessible::GetAccActionName(PRUint8 index, nsAString& _retval)
{
  // Action 0 (default action): Jump to link
  if (index == eAction_Jump) {   
    if (IsALink()) {
      nsAccessible::GetTranslatedString(NS_LITERAL_STRING("jump"), _retval); 
      return NS_OK;
    }
    return NS_ERROR_NOT_IMPLEMENTED;
  }
  return NS_ERROR_INVALID_ARG;
}

/* void accDoAction (in PRUint8 index); */
NS_IMETHODIMP nsLinkableAccessible::AccDoAction(PRUint8 index)
{
  // Action 0 (default action): Jump to link
  if (index == eAction_Jump) {
    if (IsALink()) {
      nsCOMPtr<nsIPresContext> presContext(GetPresContext());
      if (presContext) {
        nsMouseEvent linkClickEvent;
        linkClickEvent.eventStructType = NS_EVENT;
        linkClickEvent.message = NS_MOUSE_LEFT_CLICK;
        linkClickEvent.isShift = PR_FALSE;
        linkClickEvent.isControl = PR_FALSE;
        linkClickEvent.isAlt = PR_FALSE;
        linkClickEvent.isMeta = PR_FALSE;
        linkClickEvent.clickCount = 0;
        linkClickEvent.widget = nsnull;

        nsEventStatus eventStatus =  nsEventStatus_eIgnore;
        mLinkContent->HandleDOMEvent(presContext, 
                                     &linkClickEvent, 
                                     nsnull, 
                                     NS_EVENT_FLAG_INIT, 
                                     &eventStatus);
        return NS_OK;
      }
    }
  }
  return NS_ERROR_INVALID_ARG;
}

NS_IMETHODIMP nsLinkableAccessible::GetAccKeyboardShortcut(nsAString& _retval)
{
  if (IsALink()) {
    nsresult rv;
    nsCOMPtr<nsIDOMNode> linkNode(do_QueryInterface(mLinkContent));
    if (linkNode && mDOMNode != linkNode) {
      nsCOMPtr<nsIAccessible> linkAccessible;
      nsCOMPtr<nsIAccessibilityService> accService = 
        do_GetService("@mozilla.org/accessibilityService;1");
      rv = accService->GetAccessibleInWeakShell(linkNode, mWeakShell,
                                                getter_AddRefs(linkAccessible));
      if (NS_SUCCEEDED(rv) && linkAccessible)
        return linkAccessible->GetAccKeyboardShortcut(_retval);
      else
        return rv;
    }
  }
  return nsAccessible::GetAccKeyboardShortcut(_retval);;
}

PRBool nsLinkableAccessible::IsALink()
{
  if (mIsALinkCached)  // Cached answer?
    return mLinkContent? PR_TRUE: PR_FALSE;

  nsCOMPtr<nsIContent> walkUpContent(do_QueryInterface(mDOMNode));
  if (walkUpContent) {
    nsCOMPtr<nsIContent> tempContent = walkUpContent;
    while (walkUpContent) {
      nsCOMPtr<nsILink> link(do_QueryInterface(walkUpContent));
      if (link) {
        mLinkContent = tempContent;
        mIsALinkCached = PR_TRUE;
        nsLinkState linkState;
        link->GetLinkState(linkState);
        if (linkState == eLinkState_Visited)
          mIsLinkVisited = PR_TRUE;
        return PR_TRUE;
      }
      walkUpContent->GetParent(*getter_AddRefs(tempContent));
      walkUpContent = tempContent;
    }
  }
  mIsALinkCached = PR_TRUE;  // Cached that there is no link
  return PR_FALSE;
}

NS_IMETHODIMP nsLinkableAccessible::Shutdown()
{
  mLinkContent = nsnull;
  return nsAccessibleWrap::Shutdown();
}
