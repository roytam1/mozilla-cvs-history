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

#include "nsHTMLFormControlAccessible.h"
#include "nsWeakReference.h"
#include "nsIDOMHTMLInputElement.h"
#include "nsIDOMHTMLTextAreaElement.h"
#include "nsINameSpaceManager.h"
#include "nsHTMLAtoms.h"
#include "nsIDOMHTMLButtonElement.h"
#include "nsReadableUtils.h"
#include "nsAccessible.h"
#include "nsIFrame.h"
#include "nsIDOMHTMLLabelElement.h"
#include "nsIDOMHTMLFormElement.h"

nsHTMLFormControlAccessible::nsHTMLFormControlAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell):
nsLeafAccessible(aNode, aShell)
{ 
}

NS_IMETHODIMP nsHTMLFormControlAccessible::AppendLabelFor(nsIContent *aLookNode, nsAReadableString *aId, nsAWritableString *aLabel)
{
  PRInt32 numChildren = 0;

  nsCOMPtr<nsIDOMHTMLLabelElement> labelElement(do_QueryInterface(aLookNode));
  if (labelElement) {
    nsCOMPtr<nsIDOMElement> elt(do_QueryInterface(aLookNode));
    nsresult rv = NS_OK;

    if (elt) {
      nsAutoString labelIsFor;
      elt->GetAttribute(NS_LITERAL_STRING("for"),labelIsFor);
      if (labelIsFor.Equals(*aId))
        rv = AppendFlatStringFromSubtree(aLookNode, aLabel);
    }
    return rv;
  }

  aLookNode->ChildCount(numChildren);
  nsIContent *contentWalker;
  PRInt32 index;
  for (index = 0; index < numChildren; index++) {
    aLookNode->ChildAt(index, contentWalker);
    if (contentWalker)
      AppendLabelFor(contentWalker, aId, aLabel);
  }
  return NS_OK;
}

/* wstring getAccName (); */
NS_IMETHODIMP nsHTMLFormControlAccessible::GetAccName(PRUnichar **_retval)
{
  nsCOMPtr<nsIContent> walkUpContent(do_QueryInterface(mDOMNode));
  nsCOMPtr<nsIDOMHTMLLabelElement> labelElement;
  nsCOMPtr<nsIDOMHTMLFormElement> formElement;
  nsAutoString nameString;
  nsresult rv = NS_OK;

  
  // go up tree get name of ancestor label if there is one. Don't go up farther than form element
  while (walkUpContent && nameString.IsEmpty() && !formElement) {
    labelElement = do_QueryInterface(walkUpContent);
    if (labelElement) 
      rv = AppendFlatStringFromSubtree(walkUpContent, &nameString);
    formElement = do_QueryInterface(walkUpContent); // reached top ancestor in form
    nsCOMPtr<nsIContent> nextParent;
    walkUpContent->GetParent(*getter_AddRefs(nextParent));
    walkUpContent = nextParent;
  }
  

  // There can be a label targeted at this control using the for="control_id" attribute
  // To save computing time, only look for those inside of a form element
  walkUpContent = do_QueryInterface(formElement);
  
  if (walkUpContent) {
    nsCOMPtr<nsIDOMElement> elt(do_QueryInterface(mDOMNode));
    nsAutoString forId;
    elt->GetAttribute(NS_LITERAL_STRING("id"), forId);
    // Actually we'll be walking down the content this time, with a depth first search
    if (!forId.IsEmpty())
      AppendLabelFor(walkUpContent,&forId,&nameString); 
  } 
  
  nameString.CompressWhitespace();
  *_retval = nameString.ToNewUnicode();
  
  return NS_OK;
}

/* long getAccState (); */
NS_IMETHODIMP nsHTMLFormControlAccessible::GetAccState(PRUint32 *_retval)
{
  // can be
  // focusable, focused, checked, protected, unavailable
  nsCOMPtr<nsIDOMHTMLInputElement> element(do_QueryInterface(mDOMNode));

  nsAccessible::GetAccState(_retval);
  *_retval |= STATE_FOCUSABLE;

  PRBool checked = PR_FALSE;
  element->GetChecked(&checked);
  if (checked) *_retval |= STATE_CHECKED;

  PRBool disabled = PR_FALSE;
  element->GetDisabled(&disabled);
  if (disabled)
    *_retval |= STATE_UNAVAILABLE;

  nsAutoString typeString;
  element->GetType(typeString);
  if (typeString.EqualsIgnoreCase("password"))
    *_retval |= STATE_PROTECTED;

  return NS_OK;
}

// --- checkbox -----

nsHTMLCheckboxAccessible::nsHTMLCheckboxAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell):
nsHTMLFormControlAccessible(aNode, aShell)
{ 
}

/* unsigned long getAccRole (); */
NS_IMETHODIMP nsHTMLCheckboxAccessible::GetAccRole(PRUint32 *_retval)
{
  *_retval = ROLE_CHECKBUTTON;
  return NS_OK;
}

/* PRUint8 getAccNumActions (); */
NS_IMETHODIMP nsHTMLCheckboxAccessible::GetAccNumActions(PRUint8 *_retval)
{
  *_retval = 1;
  return NS_OK;
}

/* wstring getAccActionName (in PRUint8 index); */
NS_IMETHODIMP nsHTMLCheckboxAccessible::GetAccActionName(PRUint8 index, PRUnichar **_retval)
{
  if (index == 0) {
    // check or uncheck
    nsCOMPtr<nsIDOMHTMLInputElement> element(do_QueryInterface(mDOMNode));

    PRBool checked = PR_FALSE;
    if (element) 
      element->GetChecked(&checked);

    if (checked)
      *_retval = ToNewUnicode(NS_LITERAL_STRING("uncheck"));
    else
      *_retval = ToNewUnicode(NS_LITERAL_STRING("check"));

    return NS_OK;
  }
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* void accDoAction (in PRUint8 index); */
NS_IMETHODIMP nsHTMLCheckboxAccessible::AccDoAction(PRUint8 index)
{
  if (index == 0) {
    nsCOMPtr<nsIDOMHTMLInputElement> element(do_QueryInterface(mDOMNode));
    PRBool checked = PR_FALSE;
    element->GetChecked(&checked);
    element->SetChecked(checked ? PR_FALSE : PR_TRUE);
    return NS_OK;
  }
  return NS_ERROR_NOT_IMPLEMENTED;
}


//------ Radio button -------

nsHTMLRadioButtonAccessible::nsHTMLRadioButtonAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell):
nsHTMLFormControlAccessible(aNode, aShell)
{ 
}

/* PRUint8 getAccNumActions (); */
NS_IMETHODIMP nsHTMLRadioButtonAccessible::GetAccNumActions(PRUint8 *_retval)
{
  *_retval = 1;
  return NS_OK;
}

/* wstring getAccActionName (in PRUint8 index); */
NS_IMETHODIMP nsHTMLRadioButtonAccessible::GetAccActionName(PRUint8 index, PRUnichar **_retval)
{
  if (index == 0) {
    *_retval = ToNewUnicode(NS_LITERAL_STRING("select"));
    return NS_OK;
  }
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* void accDoAction (in PRUint8 index); */
NS_IMETHODIMP nsHTMLRadioButtonAccessible::AccDoAction(PRUint8 index)
{
  if (index == 0) {
    nsCOMPtr<nsIDOMHTMLInputElement> element(do_QueryInterface(mDOMNode));
    element->Click();
    return NS_OK;
  }
  return NS_ERROR_NOT_IMPLEMENTED;
}


/* unsigned long getAccRole (); */
NS_IMETHODIMP nsHTMLRadioButtonAccessible::GetAccRole(PRUint32 *_retval)
{
  *_retval = ROLE_RADIOBUTTON;

  return NS_OK;
}

// ----- Button -----

nsHTMLButtonAccessible::nsHTMLButtonAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell):
nsHTMLFormControlAccessible(aNode, aShell)
{ 
}

/* PRUint8 getAccNumActions (); */
NS_IMETHODIMP nsHTMLButtonAccessible::GetAccNumActions(PRUint8 *_retval)
{
  *_retval = 1;
  return NS_OK;;
}

/* wstring getAccActionName (in PRUint8 index); */
NS_IMETHODIMP nsHTMLButtonAccessible::GetAccActionName(PRUint8 index, PRUnichar **_retval)
{
  if (index == 0) {
    *_retval = ToNewUnicode(NS_LITERAL_STRING("press"));
    return NS_OK;
  }
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* void accDoAction (in PRUint8 index); */
NS_IMETHODIMP nsHTMLButtonAccessible::AccDoAction(PRUint8 index)
{
  if (index == 0) {
    nsCOMPtr<nsIDOMHTMLInputElement> element(do_QueryInterface(mDOMNode));
    element->Click();
    return NS_OK;
  }
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* unsigned long getAccRole (); */
NS_IMETHODIMP nsHTMLButtonAccessible::GetAccRole(PRUint32 *_retval)
{
  *_retval = ROLE_PUSHBUTTON;
  return NS_OK;
}

/* wstring getAccName (); */
NS_IMETHODIMP nsHTMLButtonAccessible::GetAccName(PRUnichar **_retval)
{
  *_retval = nsnull;
  nsCOMPtr<nsIDOMHTMLInputElement> button(do_QueryInterface(mDOMNode));

  if (!button)
    return NS_ERROR_FAILURE;

  nsAutoString name;
  button->GetValue(name);
  name.CompressWhitespace();

  *_retval = name.ToNewUnicode();

  return NS_OK;
}


// ----- HTML 4 Button: can contain arbitrary HTML content -----

nsHTML4ButtonAccessible::nsHTML4ButtonAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell):
nsHTMLBlockAccessible(aNode, aShell)
{ 
}

/* PRUint8 getAccNumActions (); */
NS_IMETHODIMP nsHTML4ButtonAccessible::GetAccNumActions(PRUint8 *_retval)
{
  *_retval = 1;
  return NS_OK;;
}

/* wstring getAccActionName (in PRUint8 index); */
NS_IMETHODIMP nsHTML4ButtonAccessible::GetAccActionName(PRUint8 index, PRUnichar **_retval)
{
  if (index == 0) {
    *_retval = ToNewUnicode(NS_LITERAL_STRING("press"));
    return NS_OK;
  }
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* void accDoAction (in PRUint8 index); */
NS_IMETHODIMP nsHTML4ButtonAccessible::AccDoAction(PRUint8 index)
{
  if (index == 0) {
    nsCOMPtr<nsIDOMHTMLInputElement> element(do_QueryInterface(mDOMNode));
    element->Click();
    return NS_OK;
  }
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* unsigned long getAccRole (); */
NS_IMETHODIMP nsHTML4ButtonAccessible::GetAccRole(PRUint32 *_retval)
{
  *_retval = ROLE_PUSHBUTTON;
  return NS_OK;
}

/* long getAccState (); */
NS_IMETHODIMP nsHTML4ButtonAccessible::GetAccState(PRUint32 *_retval)
{
  nsAccessible::GetAccState(_retval);
  *_retval |= STATE_FOCUSABLE;
  return NS_OK;
}


/* wstring getAccName (); */
NS_IMETHODIMP nsHTML4ButtonAccessible::GetAccName(PRUnichar **_retval)
{
  *_retval = nsnull;
  nsresult rv = NS_ERROR_FAILURE;
  nsAutoString name;
  nsCOMPtr<nsIContent> content(do_QueryInterface(mDOMNode));

  if (content)
    rv = AppendFlatStringFromSubtree(content, &name);

  if (NS_SUCCEEDED(rv))  {
    name.CompressWhitespace();
    *_retval = name.ToNewUnicode();
  }

  return rv;
}


// --- textfield -----

nsHTMLTextFieldAccessible::nsHTMLTextFieldAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell):
nsHTMLFormControlAccessible(aNode, aShell)
{ 
}

/* unsigned long getAccRole (); */
NS_IMETHODIMP nsHTMLTextFieldAccessible::GetAccRole(PRUint32 *_retval)
{
  *_retval = ROLE_TEXT;
  return NS_OK;
}

/* wstring getAccValue (); */
NS_IMETHODIMP nsHTMLTextFieldAccessible::GetAccValue(PRUnichar **_retval)
{
  nsCOMPtr<nsIDOMHTMLTextAreaElement> textArea(do_QueryInterface(mDOMNode));
  if (textArea) {
    nsAutoString valueString;
    textArea->GetValue(valueString);
    *_retval = ToNewUnicode(valueString);
    return NS_OK;
  }
  
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* long getAccState (); */
NS_IMETHODIMP nsHTMLTextFieldAccessible::GetAccState(PRUint32 *_retval)
{
  // can be
  // focusable, focused, protected. readonly, unavailable, selected

  nsAccessible::GetAccState(_retval);
  *_retval |= STATE_FOCUSABLE;

  nsCOMPtr<nsIDOMHTMLTextAreaElement> textArea(do_QueryInterface(mDOMNode));
  nsCOMPtr<nsIDOMHTMLInputElement> inputElement(do_QueryInterface(mDOMNode));

  nsCOMPtr<nsIDOMElement> elt(do_QueryInterface(mDOMNode));
  PRBool isReadOnly = PR_FALSE;
  elt->HasAttribute(NS_LITERAL_STRING("readonly"), &isReadOnly);
  if (isReadOnly)
    *_retval |= STATE_READONLY;

  // Get current selection and find out if current node is in it
  nsCOMPtr<nsIPresShell> shell(do_QueryReferent(mPresShell));
  if (!shell) {
     return NS_ERROR_FAILURE;  
  }

  nsCOMPtr<nsIPresContext> context;
  shell->GetPresContext(getter_AddRefs(context));
  nsCOMPtr<nsIContent> content(do_QueryInterface(mDOMNode));
  nsIFrame *frame;
  if (content && NS_SUCCEEDED(shell->GetPrimaryFrameFor(content, &frame))) {
    nsCOMPtr<nsISelectionController> selCon;
    frame->GetSelectionController(context,getter_AddRefs(selCon));
    if (selCon) {
      nsCOMPtr<nsISelection> domSel;
      selCon->GetSelection(nsISelectionController::SELECTION_NORMAL, getter_AddRefs(domSel));
      if (domSel) {
        PRBool isCollapsed = PR_TRUE;
        domSel->GetIsCollapsed(&isCollapsed);
        if (!isCollapsed)
          *_retval |=STATE_SELECTED;
      }
    }
  }


  if (!textArea) {
    if (inputElement) {
      /////// ====== Must be a password field, so it uses nsIDOMHTMLFormControl ==== ///////
      PRUint32 moreStates = 0;
      nsresult rv = nsHTMLFormControlAccessible::GetAccState(&moreStates);
      *_retval |= moreStates;
      return rv;
    }
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  PRBool disabled = PR_FALSE;
  textArea->GetDisabled(&disabled);
  if (disabled)
    *_retval |= STATE_UNAVAILABLE;

  return NS_OK;
}

