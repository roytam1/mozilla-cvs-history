/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ----- BEGIN LICENSE BLOCK -----
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is the Mozilla XTF project.
 *
 * The Initial Developer of the Original Code is 
 * Alex Fritze.
 * Portions created by the Initial Developer are Copyright (C) 2004
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *    Alex Fritze <alex@croczilla.com> (original author)
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
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ----- END LICENSE BLOCK ----- */

#include "nsXTFElementWrapper.h"
#include "nsIXTFElement.h"
#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsXTFInterfaceAggregator.h"
#include "nsIClassInfo.h"
#include "nsPIDOMWindow.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIDocument.h"
#include "nsHTMLAtoms.h" // XXX only needed for nsHTMLAtoms::id

nsXTFElementWrapper::nsXTFElementWrapper(nsINodeInfo* aNodeInfo)
    : nsXTFElementWrapperBase(aNodeInfo)
{
}

nsresult
nsXTFElementWrapper::Init()
{
  PRBool innerHandlesAttribs = PR_FALSE;
  GetXTFElement()->GetIsAttributeHandler(&innerHandlesAttribs);
  if (innerHandlesAttribs)
    mAttributeHandler = do_QueryInterface(GetXTFElement());
  GetXTFElement()->GetNotificationMask(&mNotificationMask);
  return NS_OK;
}

//----------------------------------------------------------------------
// nsISupports implementation

NS_IMPL_ADDREF_INHERITED(nsXTFElementWrapper,nsXTFElementWrapperBase)
NS_IMPL_RELEASE_INHERITED(nsXTFElementWrapper,nsXTFElementWrapperBase)

NS_IMETHODIMP
nsXTFElementWrapper::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
  nsresult rv;
  
  if(aIID.Equals(NS_GET_IID(nsIClassInfo))) {
    *aInstancePtr = NS_STATIC_CAST(nsIClassInfo*, this);
    NS_ADDREF_THIS();
    return NS_OK;
  }
  else if(aIID.Equals(NS_GET_IID(nsIXTFElementWrapper))) {
    *aInstancePtr = NS_STATIC_CAST(nsIXTFElementWrapper*, this);
    NS_ADDREF_THIS();
    return NS_OK;
  }
  else if (NS_SUCCEEDED(rv = nsXTFElementWrapperBase::QueryInterface(aIID, aInstancePtr))) {
    return rv;
  }
  else if (AggregatesInterface(aIID)) {
#ifdef DEBUG
//    printf("nsXTFElementWrapper::QueryInterface(): creating aggregation tearoff\n");
#endif
    return NS_NewXTFInterfaceAggregator(aIID, GetXTFElement(), (nsIContent*)this,
                                        (nsISupports**)aInstancePtr);
  }

  return NS_ERROR_NO_INTERFACE;
}

//----------------------------------------------------------------------
// nsIContent:

void
nsXTFElementWrapper::SetDocument(nsIDocument* aDocument, PRBool aDeep,
                                 PRBool aCompileEventHandlers)
{
  // XXX For some reason we often get 2 SetDocument notifications with
  // identical aDocument (one when expat encounters the element and
  // another when the element is appended to its parent). We want to
  // make sure that we only route notifications if the document has
  // actually changed.
  bool docReallyChanged = false;
  if (aDocument!=mDocument) docReallyChanged = true;

  if (docReallyChanged &&
      (mNotificationMask & nsIXTFElement::NOTIFY_WILL_CHANGE_DOCUMENT))
    GetXTFElement()->WillChangeDocument(aDocument);
  nsXTFElementWrapperBase::SetDocument(aDocument, aDeep, aCompileEventHandlers);
  if (docReallyChanged &&
      (mNotificationMask & nsIXTFElement::NOTIFY_DOCUMENT_CHANGED))
    GetXTFElement()->DocumentChanged(aDocument);
}

void
nsXTFElementWrapper::SetParent(nsIContent* aParent)
{
  if (mNotificationMask & nsIXTFElement::NOTIFY_WILL_CHANGE_PARENT)
    GetXTFElement()->WillChangeParent(aParent);
  nsXTFElementWrapperBase::SetParent(aParent);
  if (mNotificationMask & nsIXTFElement::NOTIFY_PARENT_CHANGED)
    GetXTFElement()->ParentChanged(aParent);
}

nsresult
nsXTFElementWrapper::InsertChildAt(nsIContent* aKid, PRUint32 aIndex,
                                   PRBool aNotify, PRBool aDeepSetDocument)
{
  nsresult rv;
  
  if (mNotificationMask & nsIXTFElement::NOTIFY_WILL_INSERT_CHILD)
    GetXTFElement()->WillInsertChild(aKid, aIndex);
  rv = nsXTFElementWrapperBase::InsertChildAt(aKid, aIndex, aNotify, aDeepSetDocument);
  if (mNotificationMask & nsIXTFElement::NOTIFY_CHILD_INSERTED)
    GetXTFElement()->ChildInserted(aKid, aIndex);
  return rv;
}

nsresult
nsXTFElementWrapper::AppendChildTo(nsIContent* aKid, PRBool aNotify,
                                   PRBool aDeepSetDocument)
{
  nsresult rv;
  
  if (mNotificationMask & nsIXTFElement::NOTIFY_WILL_APPEND_CHILD)
    GetXTFElement()->WillAppendChild(aKid);
  rv = nsXTFElementWrapperBase::AppendChildTo(aKid, aNotify, aDeepSetDocument);
  if (mNotificationMask & nsIXTFElement::NOTIFY_CHILD_APPENDED)
    GetXTFElement()->ChildAppended(aKid);
  return rv;
}

nsresult
nsXTFElementWrapper::RemoveChildAt(PRUint32 aIndex, PRBool aNotify)
{
  nsresult rv;
  if (mNotificationMask & nsIXTFElement::NOTIFY_WILL_REMOVE_CHILD)
    GetXTFElement()->WillRemoveChild(aIndex);
  rv = nsXTFElementWrapperBase::RemoveChildAt(aIndex, aNotify);
  if (mNotificationMask & nsIXTFElement::NOTIFY_CHILD_REMOVED)
    GetXTFElement()->ChildRemoved(aIndex);
  return rv;
}

nsIAtom *
nsXTFElementWrapper::GetIDAttributeName() const
{
  // XXX:
  return nsHTMLAtoms::id;
}

nsresult
nsXTFElementWrapper::SetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                             nsIAtom* aPrefix, const nsAString& aValue,
                             PRBool aNotify)
{
  nsresult rv;
  
  if (mNotificationMask & nsIXTFElement::NOTIFY_WILL_SET_ATTRIBUTE)
    GetXTFElement()->WillSetAttribute(aName, aValue);
  
  if (mAttributeHandler) {
    // XXX we don't do namespaced attributes yet
    if (aNameSpaceID != kNameSpaceID_None) {
      NS_WARNING("setattr: xtf elements don't do namespaced attribs yet!");
      return NS_ERROR_FAILURE;
    }  
    rv = mAttributeHandler->SetAttribute(aName, aValue);
    // XXX mutation events?
  }
  else { // wrapper handles attribs
    rv = nsXTFElementWrapperBase::SetAttr(aNameSpaceID, aName, aPrefix, aValue, aNotify);
  }
  
  if (mNotificationMask & nsIXTFElement::NOTIFY_ATTRIBUTE_SET)
    GetXTFElement()->AttributeSet(aName, aValue);
  
  return rv;
}

nsresult
nsXTFElementWrapper::GetAttr(PRInt32 aNameSpaceID, nsIAtom* aName, 
                             nsAString& aResult)const
{
  if (mAttributeHandler) {
    // XXX we don't do namespaced attributes yet
    if (aNameSpaceID != kNameSpaceID_None) {
      NS_WARNING("getattr: xtf elements don't do namespaced attribs yet!");
      return NS_CONTENT_ATTR_NOT_THERE;
    }
    nsresult rv = mAttributeHandler->GetAttribute(aName, aResult);
    if (NS_FAILED(rv)) return NS_CONTENT_ATTR_NOT_THERE;
    if (aResult.IsVoid()) return NS_CONTENT_ATTR_NO_VALUE;
    
    return NS_CONTENT_ATTR_HAS_VALUE;
  }
  else { // wrapper handles attribs
    return nsXTFElementWrapperBase::GetAttr(aNameSpaceID, aName, aResult);
  }
}

PRBool
nsXTFElementWrapper::HasAttr(PRInt32 aNameSpaceID, nsIAtom* aName) const
{
  if (mAttributeHandler) {
    PRBool rval = PR_FALSE;
    mAttributeHandler->HasAttribute(aName, &rval);
    return rval;
  }
  else { // wrapper handles attribs
    return nsXTFElementWrapperBase::HasAttr(aNameSpaceID, aName);
  }
}


nsresult
nsXTFElementWrapper::UnsetAttr(PRInt32 aNameSpaceID, nsIAtom* aAttr, 
                               PRBool aNotify)
{
  nsresult rv;

  if (mNotificationMask & nsIXTFElement::NOTIFY_WILL_UNSET_ATTRIBUTE)
    GetXTFElement()->WillUnsetAttribute(aAttr);
  
  if (mAttributeHandler) {
    // XXX we don't do namespaced attributes yet
    if (aNameSpaceID != kNameSpaceID_None) {
      NS_WARNING("setattr: xtf elements don't do namespaced attribs yet!");
      return NS_ERROR_FAILURE;
    }  
    rv = mAttributeHandler->UnsetAttribute(aAttr);
    // XXX mutation events?
  }
  else { // wrapper handles attribs
    rv = nsXTFElementWrapperBase::UnsetAttr(aNameSpaceID, aAttr, aNotify);
  }

  if (mNotificationMask & nsIXTFElement::NOTIFY_ATTRIBUTE_UNSET)
    GetXTFElement()->AttributeUnset(aAttr);

  return rv;
}

nsresult
nsXTFElementWrapper::GetAttrNameAt(PRUint32 aIndex, PRInt32* aNameSpaceID,
                                   nsIAtom** aName, nsIAtom** aPrefix) const
{
  if (mAttributeHandler) {
    *aNameSpaceID = kNameSpaceID_None;
    *aPrefix = nsnull;
    return mAttributeHandler->GetAttributeNameAt(aIndex, aName);
  }
  else { // wrapper handles attribs
    return nsXTFElementWrapperBase::GetAttrNameAt(aIndex, aNameSpaceID,
                                                  aName, aPrefix);
  }
}

PRUint32
nsXTFElementWrapper::GetAttrCount() const
{
  if (mAttributeHandler) {
    PRUint32 rval = 0;
    mAttributeHandler->GetAttributeCount(&rval);
    return rval;
  }
  else { // wrapper handles attribs
    return nsXTFElementWrapperBase::GetAttrCount();
  }
}


//----------------------------------------------------------------------
// nsIClassInfo implementation

/* void getInterfaces (out PRUint32 count, [array, size_is (count), retval] out nsIIDPtr array); */
NS_IMETHODIMP 
nsXTFElementWrapper::GetInterfaces(PRUint32 *count, nsIID * **array)
{
  return GetXTFElement()->GetScriptingInterfaces(count, array);
}

/* nsISupports getHelperForLanguage (in PRUint32 language); */
NS_IMETHODIMP 
nsXTFElementWrapper::GetHelperForLanguage(PRUint32 language, nsISupports **_retval)
{
  *_retval = nsnull;
  return NS_OK;
}

/* readonly attribute string contractID; */
NS_IMETHODIMP 
nsXTFElementWrapper::GetContractID(char * *aContractID)
{
  *aContractID = nsnull;
  return NS_OK;
}

/* readonly attribute string classDescription; */
NS_IMETHODIMP 
nsXTFElementWrapper::GetClassDescription(char * *aClassDescription)
{
  *aClassDescription = nsnull;
  return NS_OK;
}

/* readonly attribute nsCIDPtr classID; */
NS_IMETHODIMP 
nsXTFElementWrapper::GetClassID(nsCID * *aClassID)
{
  *aClassID = nsnull;
  return NS_OK;
}

/* readonly attribute PRUint32 implementationLanguage; */
NS_IMETHODIMP 
nsXTFElementWrapper::GetImplementationLanguage(PRUint32 *aImplementationLanguage)
{
  *aImplementationLanguage = nsIProgrammingLanguage::UNKNOWN;
  return NS_OK;
}

/* readonly attribute PRUint32 flags; */
NS_IMETHODIMP 
nsXTFElementWrapper::GetFlags(PRUint32 *aFlags)
{
  *aFlags = nsIClassInfo::DOM_OBJECT;
  return NS_OK;
}

/* [notxpcom] readonly attribute nsCID classIDNoAlloc; */
NS_IMETHODIMP 
nsXTFElementWrapper::GetClassIDNoAlloc(nsCID *aClassIDNoAlloc)
{
  return NS_ERROR_NOT_AVAILABLE;
}

//----------------------------------------------------------------------
// nsIXTFElementWrapper implementation:

/* readonly attribute nsIDOMElement elementNode; */
NS_IMETHODIMP
nsXTFElementWrapper::GetElementNode(nsIDOMElement * *aElementNode)
{
  *aElementNode = (nsIDOMElement*)this;
  NS_ADDREF(*aElementNode);
  return NS_OK;
}

/* readonly attribute nsIDOMElement documentFrameElement; */
NS_IMETHODIMP
nsXTFElementWrapper::GetDocumentFrameElement(nsIDOMElement * *aDocumentFrameElement)
{
  *aDocumentFrameElement = nsnull;
  
  if (!mDocument) {
    NS_WARNING("no document");
    return NS_OK;
  }
  nsCOMPtr<nsISupports> container = mDocument->GetContainer();
  if (!container) {
    NS_ERROR("no docshell");
    return NS_ERROR_FAILURE;
  }
  nsCOMPtr<nsPIDOMWindow> pidomwin = do_GetInterface(container);
  if (!pidomwin) {
    NS_ERROR("no nsPIDOMWindow interface on docshell");
    return NS_ERROR_FAILURE;
  }
  *aDocumentFrameElement = pidomwin->GetFrameElementInternal();
  NS_IF_ADDREF(*aDocumentFrameElement);
  return NS_OK;
}

//----------------------------------------------------------------------
// implementation helpers:
PRBool
nsXTFElementWrapper::AggregatesInterface(REFNSIID aIID)
{
  // We must ensure that the inner element has a distinct xpconnect
  // identity, so we mustn't aggregate nsIXPConnectWrappedJS:
  if (aIID.Equals(NS_GET_IID(nsIXPConnectWrappedJS))) return PR_FALSE;

  nsCOMPtr<nsISupports> inst;
  GetXTFElement()->QueryInterface(aIID, getter_AddRefs(inst));
  return (inst!=nsnull);
}
