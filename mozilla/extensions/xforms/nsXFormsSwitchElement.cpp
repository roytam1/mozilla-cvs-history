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
 * The Original Code is Mozilla XForms support.
 *
 * The Initial Developer of the Original Code is
 * Olli Pettay.
 * Portions created by the Initial Developer are Copyright (C) 2004
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Olli Pettay <Olli.Pettay@helsinki.fi> (original author)
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

#include "nsCOMPtr.h"
#include "nsIDOMElement.h"
#include "nsIDOMDocument.h"
#include "nsString.h"
#include "nsIDocument.h"
#include "nsIDOMNodeList.h"

// For focus control
#include "nsPIDOMWindow.h"
#include "nsIFocusController.h"
#include "nsIScriptGlobalObject.h"

#include "nsIXTFXMLVisualWrapper.h"

#include "nsXFormsUtils.h"
#include "nsXFormsControlStub.h"
#include "nsIModelElementPrivate.h"
#include "nsIXFormsSwitchElement.h"
#include "nsIXFormsCaseElement.h"
#include "nsIXFormsContextControl.h"

/**
 * Implementation of the XForms \<switch\> element.
 *
 * @see http://www.w3.org/TR/xforms/slice9.html#id2631571
 *
 * The implementation of the context control is based on
 * nsXFormsGroupElement.
 */

class nsXFormsSwitchElement : public nsIXFormsSwitchElement,
                              public nsIXFormsContextControl,
                              public nsXFormsControlStub
{
public:
  nsXFormsSwitchElement();

  NS_DECL_ISUPPORTS_INHERITED

  NS_IMETHOD ChildInserted(nsIDOMNode *aChild, PRUint32 aIndex);
  NS_IMETHOD ChildAppended(nsIDOMNode *aChild);
  NS_IMETHOD WillRemoveChild(PRUint32 aIndex);
  NS_IMETHOD DoneAddingChildren();
  NS_IMETHOD OnDestroyed();

  NS_IMETHOD OnCreated(nsIXTFXMLVisualWrapper *aWrapper);
  NS_IMETHOD GetVisualContent(nsIDOMElement **aContent);
  NS_IMETHOD GetInsertionPoint(nsIDOMElement **aPoint);

  NS_DECL_NSIXFORMSSWITCHELEMENT

  // nsIXFormsControl
  NS_IMETHOD Refresh();

  NS_DECL_NSIXFORMSCONTEXTCONTROL

private:
  /**
   * http://www.w3.org/TR/xforms/slice9.html#ui-case
   * "If multiple cases within a switch are marked as
   * selected="true", the first selected case remains and all
   * others are deselected. If none are selected, the first becomes selected."
   *
   * This method returns the first selected case.
   */
  already_AddRefed<nsIDOMElement>
    FindFirstSelectedCase(nsIDOMElement* aDeselected = nsnull);

  void Init(nsIDOMElement* aDeselected = nsnull);

  /**
   * This is called when a case is added or removed dynamically.
   */
  void CaseChanged(nsIDOMNode* aCase, PRBool aRemoved);

  /**
   * Checks if any element in aDeselected has focus and if yes
   * tries to focus first focusable element in aSelected.
   */
  void SetFocus(nsIDOMElement* aDeselected, nsIDOMElement* aSelected);

  nsresult Process();

  nsCOMPtr<nsIDOMElement> mVisual;
  nsCOMPtr<nsIDOMElement> mSelected;
  PRBool mDoneAddingChildren;
  nsString mModelID;
};

nsXFormsSwitchElement::nsXFormsSwitchElement() : mDoneAddingChildren(PR_FALSE)
{
}

NS_IMPL_ISUPPORTS_INHERITED3(nsXFormsSwitchElement,
                             nsXFormsXMLVisualStub,
                             nsIXFormsSwitchElement,
                             nsIXFormsControl,
                             nsIXFormsContextControl)

NS_IMETHODIMP
nsXFormsSwitchElement::OnCreated(nsIXTFXMLVisualWrapper *aWrapper)
{
  nsresult rv = nsXFormsControlStub::OnCreated(aWrapper);
  NS_ENSURE_SUCCESS(rv, rv);

  aWrapper->SetNotificationMask(kStandardNotificationMask |
                                nsIXTFElement::NOTIFY_DONE_ADDING_CHILDREN |
                                nsIXTFElement::NOTIFY_CHILD_APPENDED |
                                nsIXTFElement::NOTIFY_CHILD_INSERTED |
                                nsIXTFElement::NOTIFY_WILL_REMOVE_CHILD);

  nsCOMPtr<nsIDOMDocument> domDoc;
  mElement->GetOwnerDocument(getter_AddRefs(domDoc));

  rv = domDoc->CreateElementNS(NS_LITERAL_STRING(NS_NAMESPACE_XHTML),
                               NS_LITERAL_STRING("div"),
                               getter_AddRefs(mVisual));
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsSwitchElement::GetVisualContent(nsIDOMElement **aContent)
{
  NS_ADDREF(*aContent = mVisual);
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsSwitchElement::GetInsertionPoint(nsIDOMElement **aPoint)
{
  NS_ADDREF(*aPoint = mVisual);
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsSwitchElement::OnDestroyed()
{
  mVisual = nsnull;
  mElement = nsnull;
  mSelected = nsnull;

  return nsXFormsControlStub::OnDestroyed();
}

NS_IMETHODIMP
nsXFormsSwitchElement::ChildInserted(nsIDOMNode *aChild, PRUint32 aIndex)
{
  if (mDoneAddingChildren)
    CaseChanged(aChild, PR_FALSE);
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsSwitchElement::ChildAppended(nsIDOMNode *aChild)
{
  if (mDoneAddingChildren)
    CaseChanged(aChild, PR_FALSE);
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsSwitchElement::WillRemoveChild(PRUint32 aIndex)
{
  if (mDoneAddingChildren) {
    nsCOMPtr<nsIDOMNodeList> list;
    mElement->GetChildNodes(getter_AddRefs(list));
    if (list) {
      nsCOMPtr<nsIDOMNode> child;
      list->Item(aIndex, getter_AddRefs(child));
      CaseChanged(child, PR_TRUE);
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsSwitchElement::DoneAddingChildren()
{
  if (!mElement)
    return NS_OK;

  Init();
  mDoneAddingChildren = PR_TRUE;
  Refresh();
  return NS_OK;
}

// nsIXFormsControl

NS_IMETHODIMP
nsXFormsSwitchElement::Refresh()
{
  nsresult rv = NS_OK;
  if (mDoneAddingChildren) {
    rv = Process();
  }
  return rv;
}

// nsXFormsSwitchElement

nsresult
nsXFormsSwitchElement::Process()
{
  mModelID.Truncate();

  nsCOMPtr<nsIModelElementPrivate> modelNode;
  nsCOMPtr<nsIDOMXPathResult> result;
  nsresult rv =
    ProcessNodeBinding(NS_LITERAL_STRING("ref"),
                       nsIDOMXPathResult::FIRST_ORDERED_NODE_TYPE,
                       getter_AddRefs(result),
                       getter_AddRefs(modelNode));
  
  NS_ENSURE_SUCCESS(rv, rv);
  
  if (!result) {
    return NS_OK;
  }

  // Get model ID
  nsCOMPtr<nsIDOMElement> modelElement = do_QueryInterface(modelNode);
  NS_ENSURE_TRUE(modelElement, NS_ERROR_FAILURE);
  modelElement->GetAttribute(NS_LITERAL_STRING("id"), mModelID);

  // Get context node, if any  
  result->GetSingleNodeValue(getter_AddRefs(mBoundNode));

  return NS_OK;
}

NS_IMETHODIMP
nsXFormsSwitchElement::SetContextNode(nsIDOMNode *aContextNode)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult
nsXFormsSwitchElement::GetContext(nsAString&      aModelID,
                                 nsIDOMNode    **aContextNode,
                                 PRInt32        *aContextPosition,
                                 PRInt32        *aContextSize)
{
  NS_ENSURE_ARG(aContextSize);
  NS_ENSURE_ARG(aContextPosition);
  
  /** @todo Not too elegant to call Process() here, but DoneAddingChildren is,
   *        logically, called on children before us.  We need a notification
   *        that goes from the document node and DOWN, where the controls
   *        should Refresh().
   */
  *aContextPosition = 1;
  *aContextSize = 1;

  if (mBoundNode && aContextNode)
    CallQueryInterface(mBoundNode, aContextNode); // addrefs
  aModelID = mModelID;

  return NS_OK;
}

void
nsXFormsSwitchElement::Init(nsIDOMElement* aDeselected)
{
  nsCOMPtr<nsIDOMElement> firstCase = FindFirstSelectedCase(aDeselected);
  mSelected = firstCase;
  nsCOMPtr<nsIXFormsCaseElement> selected(do_QueryInterface(mSelected));
  if (selected)
    selected->SetSelected(PR_TRUE);
}

already_AddRefed<nsIDOMElement>
nsXFormsSwitchElement::FindFirstSelectedCase(nsIDOMElement* aDeselected)
{
  nsCOMPtr<nsIDOMNode> child;
  mElement->GetFirstChild(getter_AddRefs(child));
  nsCOMPtr<nsIDOMElement> firstCase;
  while (child) {
    nsCOMPtr<nsIDOMElement> childElement(do_QueryInterface(child));
    if (childElement  && childElement != aDeselected) {
      if (nsXFormsUtils::IsXFormsElement(child, NS_LITERAL_STRING("case"))) {
        if (!firstCase)
          firstCase = childElement;
        nsAutoString selected;
        childElement->GetAttribute(NS_LITERAL_STRING("selected"), selected);
        if (selected.EqualsLiteral("true")) {
          firstCase = childElement;
          break;
        }
      }
    }
    nsCOMPtr<nsIDOMNode> tmp;
    child->GetNextSibling(getter_AddRefs(tmp));
    child.swap(tmp);
  }
  nsIDOMElement* result;
  NS_IF_ADDREF(result = firstCase);
  return result;
}

NS_IMETHODIMP
nsXFormsSwitchElement::SetSelected(nsIDOMElement *aCase, PRBool aValue)
{
  if (!mElement)
    return NS_OK;

  // There is no need to change the selected case, If we are trying to select 
  // a case which is already selected or if deselecting a case,
  // which is not selected.
  if (mSelected &&
      (((mSelected == aCase) && aValue) || ((mSelected != aCase) && !aValue)))
    return NS_OK;

  // Swapping the status of the mSelected and aCase, dispatching events.
  if (aValue && mSelected) {
    nsCOMPtr<nsIDOMElement> oldSel = mSelected;
    mSelected = aCase;
    nsCOMPtr<nsIXFormsCaseElement> deselected(do_QueryInterface(oldSel));
    if (deselected)
      deselected->SetSelected(PR_FALSE);
    nsCOMPtr<nsIXFormsCaseElement> selected(do_QueryInterface(mSelected));
    if (selected)
      selected->SetSelected(PR_TRUE);
    SetFocus(oldSel, mSelected);
    nsXFormsUtils::DispatchEvent(oldSel, eEvent_Deselect);
    nsXFormsUtils::DispatchEvent(mSelected, eEvent_Select);
    return NS_OK;
  }

  nsCOMPtr<nsIDOMElement> firstCase = 
    FindFirstSelectedCase(aValue ? nsnull : aCase);

  // If a selectable case was found, bring it to front, set focus
  // and dispatch events.
  if (firstCase) {
    mSelected = firstCase;
    nsCOMPtr<nsIXFormsCaseElement> deselected(do_QueryInterface(aCase));
    if (deselected)
      deselected->SetSelected(PR_FALSE);
    nsCOMPtr<nsIXFormsCaseElement> selected(do_QueryInterface(mSelected));
    if (selected)
      selected->SetSelected(PR_TRUE);
    SetFocus(aCase, mSelected);
    nsXFormsUtils::DispatchEvent(aCase, eEvent_Deselect);
    nsXFormsUtils::DispatchEvent(mSelected, eEvent_Select);
  }

  // If there is only one case, we (re)select it. No need to
  // set focus.
  else {
    mSelected = aCase;
    nsCOMPtr<nsIXFormsCaseElement> selected(do_QueryInterface(mSelected));
    if (selected)
      selected->SetSelected(PR_TRUE);
    nsXFormsUtils::DispatchEvent(mSelected, eEvent_Deselect);
    nsXFormsUtils::DispatchEvent(mSelected, eEvent_Select);
  }
  return NS_OK;
}

void
nsXFormsSwitchElement::SetFocus(nsIDOMElement* aDeselected,
                               nsIDOMElement* aSelected)
{
  if (aDeselected == aSelected)
    return;

  nsCOMPtr<nsIDOMDocument> domDoc;
  mElement->GetOwnerDocument(getter_AddRefs(domDoc));
  nsCOMPtr<nsIDocument> doc(do_QueryInterface(domDoc));
  if (!doc)
    return;

  nsCOMPtr<nsPIDOMWindow> win = do_QueryInterface(doc->GetScriptGlobalObject());
  if (!win)
    return;

  nsIFocusController *focusController = win->GetRootFocusController();
  if (!focusController)
    return;

  nsCOMPtr<nsIDOMElement> focused;
  focusController->GetFocusedElement(getter_AddRefs(focused));
  if (!focused)
    return;
  
  PRBool hasFocus = PR_FALSE;
  nsCOMPtr<nsIDOMNode> current(do_QueryInterface(focused));
  do {
    nsCOMPtr<nsIDOMElement> currentElement(do_QueryInterface(current));
    if (aDeselected == current) {
      hasFocus = PR_TRUE;
      break;
    }
    nsCOMPtr<nsIDOMNode> parent;
    current->GetParentNode(getter_AddRefs(parent));
    current.swap(parent);
  } while(current);
  
  if (hasFocus) {
    // Flush layout before moving focus. Otherwise focus controller might
    // (re)focus something in the deselected <case>.
    doc->FlushPendingNotifications(Flush_Layout);
    focusController->MoveFocus(PR_TRUE, mElement);
  }
}

void
nsXFormsSwitchElement::CaseChanged(nsIDOMNode* aCase, PRBool aRemoved)
{
  if (!aCase)
    return;

  if (aRemoved) {
    if (aCase == mSelected) {
      nsXFormsUtils::DispatchEvent(mSelected, eEvent_Deselect);
      nsCOMPtr<nsIDOMElement> el(do_QueryInterface(aCase));
      Init(el);
      if (mSelected)
        nsXFormsUtils::DispatchEvent(mSelected, eEvent_Select);
    }
    return;
  }

  nsCOMPtr<nsIDOMElement> element(do_QueryInterface(aCase));
  if (!element)
    return;

  if (!mSelected) {
    Init();
    if (mSelected)
      nsXFormsUtils::DispatchEvent(mSelected, eEvent_Select);
    return;
  }

  if (!nsXFormsUtils::IsXFormsElement(aCase, NS_LITERAL_STRING("case")))
    return;

  nsAutoString sel;
  element->GetAttribute(NS_LITERAL_STRING("selected"), sel);
  if (sel.EqualsLiteral("true"))
    SetSelected(element, PR_TRUE);
}

NS_HIDDEN_(nsresult)
NS_NewXFormsSwitchElement(nsIXTFElement **aResult)
{
  *aResult = new nsXFormsSwitchElement();
  if (!*aResult)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(*aResult);
  return NS_OK;
}
