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
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */
#include "nsGenericXMLElement.h"

#include "nsIAtom.h"
#include "nsINodeInfo.h"
#include "nsIBindingManager.h"
#include "nsIXBLBinding.h"
#include "nsIDocument.h"
#include "nsIDOMAttr.h"
#include "nsIDOMNamedNodeMap.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMDocument.h"
#include "nsIDOMDocumentFragment.h"
#include "nsIDOMRange.h"
#include "nsRange.h"
#include "nsIScriptGlobalObject.h"
#include "nsIScriptObjectOwner.h"
#include "nsISizeOfHandler.h"
#include "nsISupportsArray.h"
#include "nsIURL.h"
#include "nsFrame.h"
#include "nsIPresShell.h"
#include "nsIView.h"
#include "nsIViewManager.h"
#include "nsString.h"
#include "nsDOMCID.h"
#include "nsIServiceManager.h"
#include "nsIDOMScriptObjectFactory.h"
#include "nsINameSpaceManager.h"
#include "nsLayoutAtoms.h"
#include "prprf.h"
#include "prmem.h"

class nsIWebShell;

static NS_DEFINE_IID(kIDOMAttrIID, NS_IDOMATTR_IID);
static NS_DEFINE_IID(kIDOMNamedNodeMapIID, NS_IDOMNAMEDNODEMAP_IID);
static NS_DEFINE_IID(kIDOMNodeListIID, NS_IDOMNODELIST_IID);
static NS_DEFINE_IID(kIDOMDocumentIID, NS_IDOMDOCUMENT_IID);
static NS_DEFINE_IID(kIDOMDocumentFragmentIID, NS_IDOMDOCUMENTFRAGMENT_IID);

nsGenericXMLElement::nsGenericXMLElement()
{
  mNameSpace = nsnull;
}

nsGenericXMLElement::~nsGenericXMLElement()
{
  NS_IF_RELEASE(mNameSpace);
}

nsresult
nsGenericXMLElement::CopyInnerTo(nsIContent* aSrcContent,
                                 nsGenericXMLElement* aDst,
                                 PRBool aDeep)
{
  nsresult result = nsGenericContainerElement::CopyInnerTo(aSrcContent, aDst, aDeep);
  if (NS_OK == result) {
    aDst->mNameSpace = mNameSpace;
    NS_IF_ADDREF(mNameSpace);
  }
  return NS_OK;
}

  nsresult GetNodeName(nsString& aNodeName);
  nsresult GetLocalName(nsString& aLocalName);

nsresult
nsGenericXMLElement::GetNodeName(nsString& aNodeName)
{
  return mNodeInfo->GetQualifiedName(aNodeName);
}

nsresult
nsGenericXMLElement::GetLocalName(nsString& aLocalName)
{
  return mNodeInfo->GetLocalName(aLocalName);
}

nsresult
nsGenericXMLElement::GetScriptObject(nsIScriptContext* aContext, 
                                     void** aScriptObject)
{
  nsresult res = NS_OK;

  // XXX Yuck! Reaching into the generic content object isn't good.
  nsDOMSlots *slots = GetDOMSlots();
  if (nsnull == slots->mScriptObject) {
    nsIDOMScriptObjectFactory *factory;
    
    res = nsGenericElement::GetScriptObjectFactory(&factory);
    if (NS_OK != res) {
      return res;
    }

    nsAutoString tag;
    mNodeInfo->GetQualifiedName(tag);

    res = factory->NewScriptXMLElement(tag, aContext, mContent,
                                       mParent, (void**)&slots->mScriptObject);
    NS_RELEASE(factory);
    
    if (nsnull != mDocument) {
      aContext->AddNamedReference((void *)&slots->mScriptObject,
                                  slots->mScriptObject,
                                  "nsGenericXMLElement::mScriptObject");
    }
  }

  void* object = nsnull;
  if (mDocument) {
    nsCOMPtr<nsIBindingManager> bindingManager;
    mDocument->GetBindingManager(getter_AddRefs(bindingManager));
    nsCOMPtr<nsIXBLBinding> binding;
    bindingManager->GetBinding(mContent, getter_AddRefs(binding));
    if (binding) {
      nsCOMPtr<nsIScriptObjectOwner> owner(do_QueryInterface(binding));
      owner->GetScriptObject(aContext, &object);
    }
  }
  *aScriptObject = object ? object : slots->mScriptObject;
  return res;
}

nsresult 
nsGenericXMLElement::ParseAttributeString(const nsString& aStr, 
                                          nsIAtom*& aName,
                                          PRInt32& aNameSpaceID)
{
  nsAutoString attrName(aStr);
  nsIAtom* nameSpaceAtom = nsGenericElement::CutNameSpacePrefix(attrName); 
  nsIAtom* nameAtom = NS_NewAtom(attrName);
  aNameSpaceID = kNameSpaceID_None;

  if (nsnull != nameSpaceAtom) {
    if (nameSpaceAtom == nsLayoutAtoms::xmlNameSpace) {
      aNameSpaceID = kNameSpaceID_XML;
    }
    else if (nsnull != mNameSpace) {
      mNameSpace->FindNameSpaceID(nameSpaceAtom, aNameSpaceID);
    }
  }

  aName = nameAtom;
  NS_IF_RELEASE(nameSpaceAtom);

  return NS_OK;
}

nsresult 
nsGenericXMLElement::GetNameSpacePrefixFromId(PRInt32 aNameSpaceID,
                                              nsIAtom*& aPrefix)
{
  if (nsnull != mNameSpace) {
    return mNameSpace->FindNameSpacePrefix(aNameSpaceID, aPrefix);
  }
  else {
    aPrefix = nsnull;
    return NS_OK;
  }
}

nsresult
nsGenericXMLElement::SetNameSpacePrefix(nsIAtom* aNameSpacePrefix)
{
  nsINodeInfo *newNodeInfo;

  nsresult rv = mNodeInfo->PrefixChanged(aNameSpacePrefix, newNodeInfo);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_RELEASE(mNodeInfo);
  mNodeInfo = newNodeInfo;

  return NS_OK;
}

nsresult
nsGenericXMLElement::GetNameSpacePrefix(nsIAtom*& aNameSpacePrefix) const
{
  return mNodeInfo->GetPrefixAtom(aNameSpacePrefix);
}

nsresult
nsGenericXMLElement::GetNameSpaceID(PRInt32& aNameSpaceID) const
{
  return mNodeInfo->GetNamespaceID(aNameSpaceID);
}

nsresult
nsGenericXMLElement::MaybeTriggerAutoLink(nsIWebShell *aShell)
{
  // Implement in subclass
  return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult 
nsGenericXMLElement::SetContainingNameSpace(nsINameSpace* aNameSpace)
{
  NS_IF_RELEASE(mNameSpace);
  
  mNameSpace = aNameSpace;
  
  NS_IF_ADDREF(mNameSpace);

  return NS_OK;
}

nsresult 
nsGenericXMLElement::GetContainingNameSpace(nsINameSpace*& aNameSpace) const
{
  aNameSpace = mNameSpace;
  NS_IF_ADDREF(aNameSpace);

  return NS_OK;  
}
