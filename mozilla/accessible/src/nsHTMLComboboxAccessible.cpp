/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
 * Original Author: Eric Vaughan (evaughan@netscape.com)
 *
 * Contributor(s): 
 */

#include "nsHTMLComboboxAccessible.h"

#include "nsCOMPtr.h"
#include "nsIComboboxControlFrame.h"
#include "nsIDOMEventReceiver.h"
#include "nsIDOMHTMLInputElement.h"
#include "nsIDOMHTMLSelectElement.h"
#include "nsIFrame.h"
#include "nsLayoutAtoms.h"

/** ----- nsHTMLComboboxAccessible ----- */

/**
  * Constructor -- create the nsHTMLAccessible and set initial state
  *                closed and not registered
  */
nsHTMLComboboxAccessible::nsHTMLComboboxAccessible(nsIDOMNode* aDOMNode, 
                                                   nsIWeakReference* aShell)
                         :nsAccessible(aDOMNode, aShell)
{
  mRegistered = PR_FALSE;
  mOpen = PR_FALSE;
  SetupMenuListener();
}

/**
  * Destructor -- If we are registered, remove ourselves as a listener.
  */
nsHTMLComboboxAccessible::~nsHTMLComboboxAccessible()
{
  if (mRegistered) {
    nsCOMPtr<nsIDOMEventReceiver> eventReceiver(do_QueryInterface(mDOMNode));
    if (eventReceiver) 
      eventReceiver->RemoveEventListener(NS_LITERAL_STRING("create"), this, PR_TRUE);   
  }
}

/** Inherit the ISupports impl from nsAccessible -- handle nsIDOMMenuListener ourself */
NS_IMPL_ISUPPORTS_INHERITED(nsHTMLComboboxAccessible, nsAccessible, nsIDOMMenuListener)

/** 
  * Tell our caller we are a combobox 
  */
NS_IMETHODIMP nsHTMLComboboxAccessible::GetAccRole(PRUint32 *_retval)
{
  *_retval = ROLE_COMBOBOX;
  return NS_OK;
}

/** 
  * Through the arg, pass back our last child, a nsHTMLComboboxWindowAccessible object
  */
NS_IMETHODIMP nsHTMLComboboxAccessible::GetAccLastChild(nsIAccessible **_retval)
{
  *_retval = new nsHTMLComboboxWindowAccessible(this, mDOMNode, mPresShell);
  if (! *_retval)
    return NS_ERROR_FAILURE;
  NS_ADDREF(*_retval);
  return NS_OK;
}

/** 
  * Through the arg, pass back our first child, a nsHTMLComboboxTextFieldAccessible object
  */
NS_IMETHODIMP nsHTMLComboboxAccessible::GetAccFirstChild(nsIAccessible **_retval)
{
  *_retval = new nsHTMLComboboxTextFieldAccessible(this, mDOMNode, mPresShell);
  if (! *_retval)
    return NS_ERROR_FAILURE;
  NS_ADDREF(*_retval);
  return NS_OK;
}

/**
  * We always have 3 children: TextField, Button, Window. In that order
  */
NS_IMETHODIMP nsHTMLComboboxAccessible::GetAccChildCount(PRInt32 *_retval)
{
  *_retval = 3;
  return NS_OK;
}

/**
  * nsIAccessibleSelectable method. No-op because our selection is returned through
  *     GetValue(). This _may_ change just to provide additional info for the vendors
  *     and another option for them to get at stuff.
  */
NS_IMETHODIMP nsHTMLComboboxAccessible::GetSelectedChildren(nsISupportsArray **_retval)
{
  *_retval = nsnull;
  return NS_OK;
}

/**
  * Our value is the value of our ( first ) selected child. SelectElement
  *     returns this by default with GetValue().
  */
NS_IMETHODIMP nsHTMLComboboxAccessible::GetAccValue(nsAWritableString& _retval)
{
  nsCOMPtr<nsIDOMHTMLSelectElement> select (do_QueryInterface(mDOMNode));
  if (select) {
    select->GetValue(_retval);  
    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}

/**
  * As a nsHTMLComboboxAccessible we can have the following states:
  *     STATE_FOCUSED
  *     STATE_READONLY
  *     STATE_FOCUSABLE
  *     STATE_HASPOPUP
  *     STATE_EXPANDED
  *     STATE_COLLAPSED
  */
NS_IMETHODIMP nsHTMLComboboxAccessible::GetAccState(PRUint32 *_retval)
{
  // this sets either STATE_FOCUSED or 0
  nsAccessible::GetAccState(_retval);

  if (mOpen)
    *_retval |= STATE_EXPANDED;
  else
    *_retval |= STATE_COLLAPSED;

  *_retval |= STATE_HASPOPUP | STATE_READONLY | STATE_FOCUSABLE;

  return NS_OK;
}

/**
  * Set our state to open and (TBD) fire an event to MSAA saying our state
  *     has changed.
  */
NS_IMETHODIMP nsHTMLComboboxAccessible::Create(nsIDOMEvent* aEvent)
{ 
  mOpen = PR_TRUE;

  /* TBD send state change event */ 

  return NS_OK; 
}

/**
  * Set our state to not open and (TDB) fire an event to MSAA saying
  *     our state has changed.
  */
NS_IMETHODIMP nsHTMLComboboxAccessible::Destroy(nsIDOMEvent* aEvent)
{ 
  mOpen = PR_FALSE;

  /* TBD send state change event */ 

  return NS_OK; 
}

/**
  * Set our state to not open and (TDB) fire an event to MSAA saying
  *     our state has changed.
  */
NS_IMETHODIMP nsHTMLComboboxAccessible::Close(nsIDOMEvent* aEvent)
{ 
  mOpen = PR_FALSE;

  /* TBD send state change event */ 

  return NS_OK; 
}

/**
  * If we aren't already registered, register ourselves as a
  *     listener to "create" events on our DOM node. Set our
  *     state to registered, but don't notify MSAA as they 
  *     don't need to know about this state.
  */
void
nsHTMLComboboxAccessible::SetupMenuListener()
{
  // if not already registered as a popup listener, register ourself
  if (!mRegistered) {
    nsCOMPtr<nsIDOMEventReceiver> eventReceiver(do_QueryInterface(mDOMNode));
    if (eventReceiver && NS_SUCCEEDED(eventReceiver->AddEventListener(NS_LITERAL_STRING("create"), this, PR_TRUE)))
      mRegistered = PR_TRUE;
  }
}


/** ----- nsHTMLComboboxTextFieldAccessible ----- */

/**
  * Constructor -- create the nsLeafAccessible and set our parent
  */
nsHTMLComboboxTextFieldAccessible::nsHTMLComboboxTextFieldAccessible(nsIAccessible* aParent, 
                                                                 nsIDOMNode* aDOMNode, 
                                                                 nsIWeakReference* aShell)
                                  :nsLeafAccessible(aDOMNode, aShell)
{
  mParent = aParent;
}

/**
  * Currently gets the text from the first option, needs to check for selection
  *     and then return that text.
  *     Walks the Frame tree and checks for proper frames.
  */
NS_IMETHODIMP nsHTMLComboboxTextFieldAccessible::GetAccValue(nsAWritableString& _retval)
{
  nsIFrame* frame = nsAccessible::GetBoundsFrame();
  nsCOMPtr<nsIPresContext> context;
  GetPresContext(context);
  if (!frame || !context)
    return NS_ERROR_FAILURE;

  frame->FirstChild(context, nsnull, &frame);
#ifdef DEBUG
  if (! nsAccessible::IsCorrectFrameType(frame, nsLayoutAtoms::blockFrame))
    return NS_ERROR_FAILURE;
#endif

  frame->FirstChild(context, nsnull, &frame);
#ifdef DEBUG
  if (! nsAccessible::IsCorrectFrameType(frame, nsLayoutAtoms::textFrame))
    return NS_ERROR_FAILURE;
#endif

  nsCOMPtr<nsIContent> content;
  frame->GetContent(getter_AddRefs(content));

  if (!content) 
    return NS_ERROR_FAILURE;

  AppendFlatStringFromSubtree(content, &_retval);

  return NS_OK;
}

/**
  * Gets the bounds for the AreaFrame around the BlockFrame.
  *     Walks the Frame tree and checks for proper frames.
  */
void nsHTMLComboboxTextFieldAccessible::GetBounds(nsRect& aBounds, nsIFrame** aRelativeFrame)
{
  // get our first child's frame
  nsIFrame* frame = nsAccessible::GetBoundsFrame();
  nsCOMPtr<nsIPresContext> context;
  GetPresContext(context);
  if (!frame || !context)
    return;

  frame->FirstChild(context, nsnull, &frame);
#ifdef DEBUG
  if (! nsAccessible::IsCorrectFrameType(frame, nsLayoutAtoms::blockFrame))
    return;
#endif

  frame->GetParent(aRelativeFrame);
#ifdef DEBUG
  if (! nsAccessible::IsCorrectFrameType(*aRelativeFrame, nsLayoutAtoms::areaFrame))
    return;
#endif

  frame->GetRect(aBounds);
}

/**
  * Getter for our parent
  */
NS_IMETHODIMP nsHTMLComboboxTextFieldAccessible::GetAccParent(nsIAccessible **_retval)
{   
    *_retval = mParent;
    NS_IF_ADDREF(*_retval);
    return NS_OK;
}

/**
  * Through the arg, pass back our next sibling, a 
  *     nsHTMLComboboxButtonAccessible object
  */
NS_IMETHODIMP nsHTMLComboboxTextFieldAccessible::GetAccNextSibling(nsIAccessible **_retval)
{ 
  nsCOMPtr<nsIAccessible> parent;
  GetAccParent(getter_AddRefs(parent));

  *_retval = new nsHTMLComboboxButtonAccessible(parent, mDOMNode, mPresShell);
  if (! *_retval)
    return NS_ERROR_FAILURE;
  NS_ADDREF(*_retval);
  return NS_OK;
} 

/**
  * We are the first child of our parent, no previous sibling
  */
NS_IMETHODIMP nsHTMLComboboxTextFieldAccessible::GetAccPreviousSibling(nsIAccessible **_retval)
{ 
  *_retval = nsnull;
  return NS_OK;
} 

/**
  * Our role is currently only static text, but we should be able to have
  *     editable text here and we need to check that case.
  */
NS_IMETHODIMP nsHTMLComboboxTextFieldAccessible::GetAccRole(PRUint32 *_retval)
{
  *_retval = ROLE_STATICTEXT;
  return NS_OK;
}

/**
  * As a nsHTMLComboboxTextFieldAccessible we can have the following states:
  *     STATE_READONLY
  *     STATE_FOCUSED
  *     STATE_FOCUSABLE
  */
NS_IMETHODIMP nsHTMLComboboxTextFieldAccessible::GetAccState(PRUint32 *_retval)
{
  // this sets either STATE_FOCUSED or 0
  nsAccessible::GetAccState(_retval);

  *_retval |= STATE_READONLY | STATE_FOCUSABLE;

  return NS_OK;
}


/** -----SelectButtonAccessible ----- */

/**
  * Constructor -- create the nsMenuListenerAccessible and set our parent
  */
nsHTMLComboboxButtonAccessible::nsHTMLComboboxButtonAccessible(nsIAccessible* aParent, 
                                                           nsIDOMNode* aDOMNode, 
                                                           nsIWeakReference* aShell)
                               :nsAccessible(aDOMNode, aShell)
{
  mParent = aParent;
}

/**
  * Programmaticaly click on the button, causing either the display or
  *     the hiding of the drop down box ( window ).
  *     Walks the Frame tree and checks for proper frames.
  */
NS_IMETHODIMP nsHTMLComboboxButtonAccessible::AccDoAction(PRUint8 index)
{
  nsIFrame* frame = nsAccessible::GetBoundsFrame();
  nsCOMPtr<nsIPresContext> context;
  GetPresContext(context);
  if (!context)
    return NS_ERROR_FAILURE;

  frame->FirstChild(context, nsnull, &frame);
#ifdef DEBUG
  if (! nsAccessible::IsCorrectFrameType(frame, nsLayoutAtoms::blockFrame))
    return NS_ERROR_FAILURE;
#endif

  frame->GetNextSibling(&frame);
#ifdef DEBUG
  if (! nsAccessible::IsCorrectFrameType(frame, nsLayoutAtoms::gfxButtonControlFrame))
    return NS_ERROR_FAILURE;
#endif

  nsCOMPtr<nsIContent> content;
  frame->GetContent(getter_AddRefs(content));

  // We only have one action, click. Any other index is meaningless(wrong)
  if (index == eAction_Click) {
    nsCOMPtr<nsIDOMHTMLInputElement> element(do_QueryInterface(content));
    if (element)
    {
       element->Click();
       return NS_OK;
    }
    return NS_ERROR_FAILURE;
  }
  return NS_ERROR_NOT_IMPLEMENTED;
}

/**
  * Just one action ( click ).
  */
NS_IMETHODIMP nsHTMLComboboxButtonAccessible::GetAccNumActions(PRUint8 *_retval)
{
  *_retval = 1;
  return NS_OK;
}

/**
  * Gets the bounds for the AreaFrame around the gfxButtonControlFrame.
  *     Walks the Frame tree and checks for proper frames.
  */
void nsHTMLComboboxButtonAccessible::GetBounds(nsRect& aBounds, nsIFrame** aRelativeFrame)
{
  // get our second child's frame
  nsIFrame* frame = nsAccessible::GetBoundsFrame();
  nsCOMPtr<nsIPresContext> context;
  GetPresContext(context);
  if (!context)
    return;

  frame->FirstChild(context, nsnull, &frame);
#ifdef DEBUG
  if (! nsAccessible::IsCorrectFrameType(frame, nsLayoutAtoms::blockFrame))
    return;
#endif

  frame->GetNextSibling(&frame);
#ifdef DEBUG
  if (! nsAccessible::IsCorrectFrameType(frame, nsLayoutAtoms::gfxButtonControlFrame))
    return;
#endif

  frame->GetParent(aRelativeFrame);
#ifdef DEBUG
  if (! nsAccessible::IsCorrectFrameType(*aRelativeFrame, nsLayoutAtoms::areaFrame))
    return;
#endif

  frame->GetRect(aBounds);
}

/**
  * Tell our caller we are a button.
  */
NS_IMETHODIMP nsHTMLComboboxButtonAccessible::GetAccRole(PRUint32 *_retval)
{
  *_retval = ROLE_PUSHBUTTON;
  return NS_OK;
}

/**
  * Getter for our parent
  */
NS_IMETHODIMP nsHTMLComboboxButtonAccessible::GetAccParent(nsIAccessible **_retval)
{   
  *_retval = mParent;
  NS_IF_ADDREF(*_retval);
  return NS_OK;
}

/**
  * Gets the name from GetAccActionName()
  */
NS_IMETHODIMP nsHTMLComboboxButtonAccessible::GetAccName(nsAWritableString& _retval)
{
  return GetAccActionName(eAction_Click, _retval);
}

/**
  * Our action name is the reverse of our state: 
  *     if we are closed -> open is our name.
  *     if we are open -> closed is our name.
  */
NS_IMETHODIMP nsHTMLComboboxButtonAccessible::GetAccActionName(PRUint8 index, nsAWritableString& _retval)
{
  // we are open or closed
  PRBool isOpen = PR_FALSE;
  nsIFrame *boundsFrame = GetBoundsFrame();
  nsIComboboxControlFrame* comboFrame;
  nsresult rv = QueryInterface(NS_GET_IID(nsIComboboxControlFrame), (void**)&comboFrame);
  if (NS_FAILED(rv))
    return NS_ERROR_FAILURE;
  comboFrame->IsDroppedDown(&isOpen);
  if (isOpen)
    _retval = NS_LITERAL_STRING("Close");
  else
    _retval = NS_LITERAL_STRING("Open");

  return NS_OK;
}

/**
  * Through the arg, pass back our next sibling, a 
  *     nsHTMLComboboxWindowAccessible object
  */
NS_IMETHODIMP nsHTMLComboboxButtonAccessible::GetAccNextSibling(nsIAccessible **_retval)
{ 
  nsCOMPtr<nsIAccessible> parent;
  GetAccParent(getter_AddRefs(parent));

  *_retval = new nsHTMLComboboxWindowAccessible(parent, mDOMNode, mPresShell);
  if (! *_retval)
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(*_retval);
  return NS_OK;
} 

/**
  * Through the arg, pass back our previous sibling, a 
  *     nsHTMLComboboxTextFieldAccessible object
  */
NS_IMETHODIMP nsHTMLComboboxButtonAccessible::GetAccPreviousSibling(nsIAccessible **_retval)
{ 
  nsCOMPtr<nsIAccessible> parent;
  GetAccParent(getter_AddRefs(parent));

  *_retval = new nsHTMLComboboxTextFieldAccessible(parent, mDOMNode, mPresShell);
  if (! *_retval)
    return NS_ERROR_FAILURE;
  NS_ADDREF(*_retval);
  return NS_OK;
} 

/**
  * No Children. Just a button ( over-riding nsAccessible )
  */
NS_IMETHODIMP nsHTMLComboboxButtonAccessible::GetAccLastChild(nsIAccessible **_retval)
{
  *_retval = nsnull;
  return NS_OK;
}

/**
  * No Children. Just a button ( over-riding nsAccessible )
  */
NS_IMETHODIMP nsHTMLComboboxButtonAccessible::GetAccFirstChild(nsIAccessible **_retval)
{
  *_retval = nsnull;
  return NS_OK;
}

/**
  * No Children. Just a button ( over-riding nsAccessible )
  */
NS_IMETHODIMP nsHTMLComboboxButtonAccessible::GetAccChildCount(PRInt32 *_retval)
{
  *_retval = 0;
  return NS_OK;
}

/**
  * As a nsHTMLComboboxButtonAccessible we can have the following states:
  *     STATE_PRESSED
  *     STATE_FOCUSED
  *     STATE_FOCUSABLE
  */
NS_IMETHODIMP nsHTMLComboboxButtonAccessible::GetAccState(PRUint32 *_retval)
{
  // this sets either STATE_FOCUSED or 0
  nsAccessible::GetAccState(_retval);

  // we are open or closed
  PRBool isOpen = PR_FALSE;
  nsIFrame *boundsFrame = GetBoundsFrame();
  nsIComboboxControlFrame* comboFrame;
  nsresult rv = QueryInterface(NS_GET_IID(nsIComboboxControlFrame), (void**)&comboFrame);
  if (NS_FAILED(rv))
    return NS_ERROR_FAILURE;
  comboFrame->IsDroppedDown(&isOpen);
  if (isOpen)
  *_retval |= STATE_PRESSED;

  *_retval |= STATE_FOCUSABLE;

  return NS_OK;
}


/** ----- nsHTMLComboboxWindowAccessible ----- */

/**
  * Constructor -- create the nsMenuListener and set our parent
  */
nsHTMLComboboxWindowAccessible::nsHTMLComboboxWindowAccessible(nsIAccessible* aParent, 
                                                           nsIDOMNode* aDOMNode, 
                                                           nsIWeakReference* aShell)
                               :nsAccessible(aDOMNode, aShell)
{
  mParent = aParent;
}

/**
  * As a nsHTMLComboboxWindowAccessible we can have the following states:
  *     STATE_FOCUSED
  *     STATE_FOCUSABLE
  *     STATE_INVISIBLE
  *     STATE_FLOATING
  */
NS_IMETHODIMP nsHTMLComboboxWindowAccessible::GetAccState(PRUint32 *_retval)
{
  // this sets either STATE_FOCUSED or 0
  nsAccessible::GetAccState(_retval);

  // we are open or closed
  PRBool isOpen = PR_FALSE;
  nsIFrame *boundsFrame = GetBoundsFrame();
  nsIComboboxControlFrame* comboFrame;
  nsresult rv = QueryInterface(NS_GET_IID(nsIComboboxControlFrame), (void**)&comboFrame);
  if (NS_FAILED(rv))
    return NS_ERROR_FAILURE;
  comboFrame->IsDroppedDown(&isOpen);
  if (! isOpen)
    *_retval |= STATE_INVISIBLE;

  *_retval |= STATE_FOCUSABLE | STATE_FLOATING;

  return NS_OK;
}

/**
  * Tell our caller we are a window
  */
NS_IMETHODIMP nsHTMLComboboxWindowAccessible::GetAccRole(PRUint32 *_retval)
{
  *_retval = ROLE_WINDOW;
  return NS_OK;
}

/**
  * Getter for our parent
  */
NS_IMETHODIMP nsHTMLComboboxWindowAccessible::GetAccParent(nsIAccessible **_retval)
{   
    *_retval = mParent;
    NS_IF_ADDREF(*_retval);
    return NS_OK;
}
 
/**
  * Through the arg, pass back our previous sibling, a 
  *     nsHTMLComboboxButtonAccessible object
  */
NS_IMETHODIMP nsHTMLComboboxWindowAccessible::GetAccPreviousSibling(nsIAccessible **_retval)
{ 
  nsCOMPtr<nsIAccessible> parent;
  GetAccParent(getter_AddRefs(parent));

  *_retval = new nsHTMLComboboxButtonAccessible(parent, mDOMNode, mPresShell);
  if (! *_retval)
    return NS_ERROR_FAILURE;
  NS_ADDREF(*_retval);
  return NS_OK;
} 

/**
  * We are the last sibling of our parent.
  */
NS_IMETHODIMP nsHTMLComboboxWindowAccessible::GetAccNextSibling(nsIAccessible **_retval)
{
  *_retval = nsnull;
  return NS_OK;
}

/**
  * We only have one child, a list
  */
NS_IMETHODIMP nsHTMLComboboxWindowAccessible::GetAccLastChild(nsIAccessible **_retval)
{
  *_retval = new nsHTMLSelectListAccessible(this, mDOMNode, mPresShell);
  if (! *_retval)
    return NS_ERROR_FAILURE;
  NS_ADDREF(*_retval);
  return NS_OK;
}

/**
  * We only have one child, a list
  */
NS_IMETHODIMP nsHTMLComboboxWindowAccessible::GetAccFirstChild(nsIAccessible **_retval)
{
  *_retval = new nsHTMLSelectListAccessible(this, mDOMNode, mPresShell);
  if (! *_retval)
    return NS_ERROR_FAILURE;
  NS_ADDREF(*_retval);
  return NS_OK;
}

/**
  * We only have one child, a list
  */
NS_IMETHODIMP nsHTMLComboboxWindowAccessible::GetAccChildCount(PRInt32 *_retval)
{
  *_retval = 1;
  return NS_OK;
}

/**
  * Find the bounds of the window, the window may be invisible.
  */
void nsHTMLComboboxWindowAccessible::GetBounds(nsRect& aBounds, nsIFrame** aRelativeFrame)
{
   // get our first option
  nsCOMPtr<nsIDOMNode> child;
  mDOMNode->GetFirstChild(getter_AddRefs(child));

  // now get its frame
  nsCOMPtr<nsIPresShell> shell(do_QueryReferent(mPresShell));
  if (!shell) {
    *aRelativeFrame = nsnull;
    return;
  }

  nsIFrame* frame = nsnull;
  nsCOMPtr<nsIContent> content(do_QueryInterface(child));
  shell->GetPrimaryFrameFor(content, &frame);
#ifdef DEBUG
  if (! nsAccessible::IsCorrectFrameType(frame, nsLayoutAtoms::blockFrame))
    return;
#endif

  frame->GetParent(&frame);
#ifdef DEBUG
  if (! nsAccessible::IsCorrectFrameType(frame, nsLayoutAtoms::areaFrame))
    return;
#endif

  frame->GetParent(aRelativeFrame);
#ifdef DEBUG
  if (! nsAccessible::IsCorrectFrameType(*aRelativeFrame, nsLayoutAtoms::listControlFrame))
    return;
#endif

  frame->GetRect(aBounds);
}
