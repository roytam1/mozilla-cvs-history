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
 * IBM Corporation.
 * Portions created by the Initial Developer are Copyright (C) 2004
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *  Brian Ryner <bryner@brianryner.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
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

#include "nsXFormsInstanceElement.h"
#include "nsIDOMElement.h"
#include "nsIDOMEventTarget.h"
#include "nsIDOM3Node.h"
#include "nsMemory.h"
#include "nsXFormsAtoms.h"
#include "nsString.h"
#include "nsIDOMEventReceiver.h"
#include "nsIDOMXMLDocument.h"
#include "nsIDOMDOMImplementation.h"

static const nsIID sScriptingIIDs[] = {
  NS_IDOMELEMENT_IID,
  NS_IDOMEVENTTARGET_IID,
  NS_IDOM3NODE_IID
};

NS_IMPL_ADDREF(nsXFormsInstanceElement)
NS_IMPL_RELEASE(nsXFormsInstanceElement)

NS_INTERFACE_MAP_BEGIN(nsXFormsInstanceElement)
  NS_INTERFACE_MAP_ENTRY(nsIXTFGenericElement)
  NS_INTERFACE_MAP_ENTRY(nsIXTFElement)
  NS_INTERFACE_MAP_ENTRY(nsIXTFPrivate)
  NS_INTERFACE_MAP_ENTRY(nsIDOMLoadListener)
  NS_INTERFACE_MAP_ENTRY(nsIDOMEventListener)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIXTFGenericElement)
NS_INTERFACE_MAP_END

NS_IMETHODIMP
nsXFormsInstanceElement::OnDestroyed()
{
  nsCOMPtr<nsIDOMEventReceiver> rec = do_QueryInterface(mDocument);
  if (rec)
    rec->RemoveEventListenerByIID(this, NS_GET_IID(nsIDOMLoadListener));

  return NS_OK;
}

NS_IMETHODIMP
nsXFormsInstanceElement::GetElementType(PRUint32 *aElementType)
{
  *aElementType = nsIXTFElement::ELEMENT_TYPE_GENERIC_ELEMENT;
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsInstanceElement::GetIsAttributeHandler(PRBool *aIsAttributeHandler)
{
  *aIsAttributeHandler = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsInstanceElement::GetScriptingInterfaces(PRUint32 *aCount,
                                                nsIID ***aArray)
{
  return CloneScriptingInterfaces(sScriptingIIDs,
                                  NS_ARRAY_LENGTH(sScriptingIIDs),
                                  aCount, aArray);
}

NS_IMETHODIMP
nsXFormsInstanceElement::GetNotificationMask(PRUint32 *aNotificationMask)
{
  *aNotificationMask = (nsIXTFElement::NOTIFY_PARENT_CHANGED |
                        nsIXTFElement::NOTIFY_ATTRIBUTE_SET |
                        nsIXTFElement::NOTIFY_ATTRIBUTE_UNSET);
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsInstanceElement::WillChangeDocument(nsISupports *aNewDocument)
{
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsInstanceElement::DocumentChanged(nsISupports *aNewDocument)
{
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsInstanceElement::WillChangeParent(nsISupports *aNewParent)
{
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsInstanceElement::ParentChanged(nsISupports *aNewParent)
{
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsInstanceElement::WillInsertChild(nsISupports *aChild, PRUint32 aIndex)
{
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsInstanceElement::ChildInserted(nsISupports *aChild, PRUint32 aIndex)
{
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsInstanceElement::WillAppendChild(nsISupports *aChild)
{
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsInstanceElement::ChildAppended(nsISupports *aChild)
{
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsInstanceElement::WillRemoveChild(PRUint32 aIndex)
{
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsInstanceElement::ChildRemoved(PRUint32 aIndex)
{
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsInstanceElement::WillSetAttribute(nsIAtom *aName,
                                          const nsAString &aNewValue)
{
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsInstanceElement::AttributeSet(nsIAtom *aName,
                                      const nsAString &aNewValue)
{
  if (aName == nsXFormsAtoms::src) {
    // Note that this will fail if encountered during document construction,
    // because we won't be in the document yet, so CreateInstanceDocument
    // won't find a document to work with.  That's ok, we'll fix things after
    // our children are appended and we're in the document (DoneAddingChildren)

    LoadExternalInstance(aNewValue);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsXFormsInstanceElement::WillUnsetAttribute(nsIAtom *aName)
{
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsInstanceElement::AttributeUnset(nsIAtom *aName)
{
  if (aName == nsXFormsAtoms::src) {
    // We no longer have an external instance to use.
    // Reset our instance document to whatever inline content we have.
    return CloneInlineInstance();
  }

  return NS_OK;
}

NS_IMETHODIMP
nsXFormsInstanceElement::DoneAddingChildren()
{
  // By the time this is called, we should be inserted in the document
  // and have all of our child elements, so this is our first opportunity
  // to create the instance document.

  nsCOMPtr<nsIDOMElement> element;
  mWrapper->GetElementNode(getter_AddRefs(element));
  NS_ASSERTION(element, "no wrapper element");

  nsAutoString src;
  element->GetAttribute(NS_LITERAL_STRING("src"), src);

  if (src.IsEmpty()) {
    // If we don't have a linked external instance, use our inline data.
    CloneInlineInstance();
  } else {
    LoadExternalInstance(src);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsXFormsInstanceElement::OnCreated(nsIXTFGenericElementWrapper *aWrapper)
{
  mWrapper = aWrapper;
  return NS_OK;
}

// nsIDOMLoadListener

NS_IMETHODIMP
nsXFormsInstanceElement::Load(nsIDOMEvent *aEvent)
{
  nsXFormsModelElement *model = GetModel();
  if (model)
    model->RemovePendingInstance();

  return NS_OK;
}

NS_IMETHODIMP
nsXFormsInstanceElement::BeforeUnload(nsIDOMEvent *aEvent)
{
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsInstanceElement::Unload(nsIDOMEvent *aEvent)
{
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsInstanceElement::Abort(nsIDOMEvent *aEvent)
{
  nsXFormsModelElement *model = GetModel();
  if (model) {
    model->RemovePendingInstance();
    model->DispatchEvent(eEvent_LinkException);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsXFormsInstanceElement::Error(nsIDOMEvent *aEvent)
{
  nsXFormsModelElement *model = GetModel();
  if (model) {
    model->RemovePendingInstance();
    model->DispatchEvent(eEvent_LinkException);
  }

  return NS_OK;
}

// nsIDOMEventListener

NS_IMETHODIMP
nsXFormsInstanceElement::HandleEvent(nsIDOMEvent *aEvent)
{
  return NS_OK;
}

// nsIXTFPrivate

NS_IMETHODIMP
nsXFormsInstanceElement::GetInner(nsISupports **aInner)
{
  NS_ENSURE_ARG_POINTER(aInner);

  NS_ADDREF(*aInner = NS_STATIC_CAST(nsIXTFGenericElement*, this));
  return NS_OK;
}


// private methods

nsresult
nsXFormsInstanceElement::CloneInlineInstance()
{
  // Clear out our existing instance data
  nsresult rv = CreateInstanceDocument();
  if (NS_FAILED(rv))
    return rv; // don't warn, we might just not be in the document yet

  // look for our first child element (skip over text nodes, etc.)
  nsCOMPtr<nsIDOMElement> element;
  mWrapper->GetElementNode(getter_AddRefs(element));
  nsCOMPtr<nsIDOMNode> child, temp;

  element->GetFirstChild(getter_AddRefs(child));
  while (child) {
    PRUint16 nodeType;
    child->GetNodeType(&nodeType);

    if (nodeType == nsIDOMNode::ELEMENT_NODE)
      break;

    temp.swap(child);
    temp->GetNextSibling(getter_AddRefs(child));
  }

  if (child) {
    nsCOMPtr<nsIDOMNode> newNode;
    rv = mDocument->ImportNode(child, PR_TRUE, getter_AddRefs(newNode));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIDOMNode> nodeReturn;
    rv = mDocument->AppendChild(newNode, getter_AddRefs(nodeReturn));
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "failed to append root instance node");
  }

  return rv;
}

void
nsXFormsInstanceElement::LoadExternalInstance(const nsAString &aSrc)
{
  // Clear out our existing instance data
  if (NS_FAILED(CreateInstanceDocument()))
    return;

  nsCOMPtr<nsIDOMEventReceiver> rec = do_QueryInterface(mDocument);
  rec->AddEventListenerByIID(this, NS_GET_IID(nsIDOMLoadListener));

  nsCOMPtr<nsIDOMXMLDocument> xmlDoc = do_QueryInterface(mDocument);
  NS_ASSERTION(xmlDoc, "we created a document but it's not an XMLDocument?");

  nsCOMPtr<nsIDOMElement> element;
  mWrapper->GetElementNode(getter_AddRefs(element));

  PRBool success;
  xmlDoc->Load(aSrc, &success);

  nsXFormsModelElement *model = GetModel();
  if (model) {
    if (success)
      model->AddPendingInstance();
    else
      model->DispatchEvent(eEvent_LinkException);
  }
}

nsresult
nsXFormsInstanceElement::CreateInstanceDocument()
{
  nsCOMPtr<nsIDOMElement> element;
  mWrapper->GetElementNode(getter_AddRefs(element));
  NS_ASSERTION(element, "no wrapper element");

  nsCOMPtr<nsIDOMDocument> doc;
  nsresult rv = element->GetOwnerDocument(getter_AddRefs(doc));
  NS_ENSURE_SUCCESS(rv, rv);

  if (!doc) // could be we just aren't inserted yet, so don't warn
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDOMDOMImplementation> domImpl;
  rv = doc->GetImplementation(getter_AddRefs(domImpl));
  NS_ENSURE_SUCCESS(rv, rv);

  return domImpl->CreateDocument(EmptyString(), EmptyString(), nsnull,
                                 getter_AddRefs(mDocument));
}

nsXFormsModelElement*
nsXFormsInstanceElement::GetModel()
{
  nsCOMPtr<nsIDOMElement> element;
  mWrapper->GetElementNode(getter_AddRefs(element));
  NS_ASSERTION(element, "no wrapper element");

  nsCOMPtr<nsIDOMNode> parentNode;
  element->GetParentNode(getter_AddRefs(parentNode));

  nsCOMPtr<nsIXFormsModelElement> modelElt = do_QueryInterface(parentNode);
  if (!modelElt)
    return nsnull;

  nsCOMPtr<nsIXTFPrivate> xtfPriv = do_QueryInterface(modelElt);
  NS_ENSURE_TRUE(xtfPriv, nsnull);

  nsCOMPtr<nsISupports> modelInner;
  xtfPriv->GetInner(getter_AddRefs(modelInner));
  NS_ENSURE_TRUE(modelInner, nsnull);

  nsISupports *isupp = NS_STATIC_CAST(nsISupports*, modelInner.get());
  return NS_STATIC_CAST(nsXFormsModelElement*,
                        NS_STATIC_CAST(nsIXFormsModelElement*, isupp));
}

nsresult
NS_NewXFormsInstanceElement(nsIXTFElement **aResult)
{
  *aResult = new nsXFormsInstanceElement();
  if (!*aResult)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(*aResult);
  return NS_OK;
}
