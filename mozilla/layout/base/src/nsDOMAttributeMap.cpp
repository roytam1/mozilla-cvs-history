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

#include "nsDOMAttributeMap.h"
#include "nsDOMAttribute.h"
#include "nsGenericElement.h"
#include "nsIContent.h"
#include "nsIDOMScriptObjectFactory.h"
#include "nsINameSpaceManager.h"
#include "nsDOMError.h"


//----------------------------------------------------------------------

nsDOMAttributeMap::nsDOMAttributeMap(nsIContent* aContent)
  : mContent(aContent)
{
  NS_INIT_REFCNT();
  mScriptObject = nsnull;
  // We don't add a reference to our content. If it goes away,
  // we'll be told to drop our reference
}

nsDOMAttributeMap::~nsDOMAttributeMap()
{
}

void
nsDOMAttributeMap::DropReference()
{
  mContent = nsnull;
}

NS_INTERFACE_MAP_BEGIN(nsDOMAttributeMap)
   NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMNamedNodeMap)
   NS_INTERFACE_MAP_ENTRY(nsIDOMNamedNodeMap)
   NS_INTERFACE_MAP_ENTRY(nsIScriptObjectOwner)
NS_INTERFACE_MAP_END

NS_IMPL_ADDREF(nsDOMAttributeMap)
NS_IMPL_RELEASE(nsDOMAttributeMap)

nsresult
nsDOMAttributeMap::GetScriptObject(nsIScriptContext *aContext,
                                   void** aScriptObject)
{
  nsresult res = NS_OK;
  if (nsnull == mScriptObject) {
    nsIDOMScriptObjectFactory *factory;

    res = nsGenericElement::GetScriptObjectFactory(&factory);
    if (NS_OK != res) {
      return res;
    }

    res = factory->NewScriptNamedNodeMap(aContext,
                                         (nsISupports *)(nsIDOMNamedNodeMap *)this,
                                         (nsISupports *)mContent,
                                         (void**)&mScriptObject);
    NS_RELEASE(factory);
  }
  *aScriptObject = mScriptObject;
  return res;
}

nsresult
nsDOMAttributeMap::SetScriptObject(void *aScriptObject)
{
  mScriptObject = aScriptObject;
  return NS_OK;
}

nsresult
nsDOMAttributeMap::GetNamedItem(const nsAReadableString& aAttrName,
                                nsIDOMNode** aAttribute)
{
  NS_ENSURE_ARG_POINTER(aAttribute);
  *aAttribute = nsnull;

  nsresult rv = NS_OK;
  if (mContent) {
    nsCOMPtr<nsINodeInfo> ni;
    mContent->NormalizeAttributeString(aAttrName, *getter_AddRefs(ni));
    NS_ENSURE_TRUE(ni, NS_ERROR_FAILURE);

    PRInt32 nsid;
    nsCOMPtr<nsIAtom> nameAtom;

    ni->GetNamespaceID(nsid);
    ni->GetNameAtom(*getter_AddRefs(nameAtom));

    nsresult attrResult;

    nsAutoString value;
    attrResult = mContent->GetAttribute(nsid, nameAtom, value);

    if (NS_CONTENT_ATTR_NOT_THERE != attrResult && NS_SUCCEEDED(attrResult)) {
      nsDOMAttribute* domAttribute;
      domAttribute = new nsDOMAttribute(mContent, ni, value);
      NS_ENSURE_TRUE(domAttribute, NS_ERROR_OUT_OF_MEMORY);

      rv = domAttribute->QueryInterface(NS_GET_IID(nsIDOMAttr),
                                        (void **)aAttribute);
    }
  }

  return rv;
}

nsresult
nsDOMAttributeMap::SetNamedItem(nsIDOMNode *aNode, nsIDOMNode **aReturn)
{
  NS_ENSURE_ARG_POINTER(aReturn);

  if (!aNode) {
    return NS_ERROR_NULL_POINTER;
  }

  nsresult rv = NS_OK;
  *aReturn = nsnull;

  if (mContent) {
    nsCOMPtr<nsIDOMAttr> attribute(do_QueryInterface(aNode));

    if (!attribute) {
      return NS_ERROR_DOM_HIERARCHY_REQUEST_ERR;
    }

    nsAutoString name, value;

    attribute->GetName(name);

    nsCOMPtr<nsINodeInfo> ni;
    mContent->NormalizeAttributeString(name, *getter_AddRefs(ni));
    NS_ENSURE_TRUE(ni, NS_ERROR_FAILURE);

    nsCOMPtr<nsIAtom> nameAtom;
    PRInt32 nsid;

    ni->GetNamespaceID(nsid);
    ni->GetNameAtom(*getter_AddRefs(nameAtom));

    nsresult attrResult = mContent->GetAttribute(nsid, nameAtom, value);

    if (NS_CONTENT_ATTR_NOT_THERE != attrResult && NS_SUCCEEDED(attrResult)) {
      nsDOMAttribute* domAttribute;
      // We pass a null content here since the attr node we return isn't
      // tied to this content anymore.
      domAttribute = new nsDOMAttribute(nsnull, ni, value);
      if (!domAttribute) {
        return NS_ERROR_OUT_OF_MEMORY;
      }

      rv = domAttribute->QueryInterface(NS_GET_IID(nsIDOMAttr),
                                        (void **)aReturn);
    }

    attribute->GetValue(value);

    rv = mContent->SetAttribute(ni, value, PR_TRUE);
  }

  return rv;
}

NS_IMETHODIMP
nsDOMAttributeMap::RemoveNamedItem(const nsAReadableString& aName,
                                   nsIDOMNode** aReturn)
{
  NS_ENSURE_ARG_POINTER(aReturn);
  *aReturn = nsnull;

  nsresult rv = NS_OK;

  if (mContent) {
    nsCOMPtr<nsINodeInfo> ni;
    mContent->NormalizeAttributeString(aName, *getter_AddRefs(ni));
    NS_ENSURE_TRUE(ni, NS_ERROR_FAILURE);

    nsCOMPtr<nsIAtom> nameAtom;
    PRInt32 nsid;

    ni->GetNamespaceID(nsid);
    ni->GetNameAtom(*getter_AddRefs(nameAtom));

    nsCOMPtr<nsIDOMNode> attribute;

    nsresult attrResult;
    nsAutoString value;
    attrResult = mContent->GetAttribute(nsid, nameAtom, value);

    if (NS_CONTENT_ATTR_NOT_THERE != attrResult && NS_SUCCEEDED(attrResult)) {
      nsDOMAttribute* domAttribute;
      domAttribute = new nsDOMAttribute(nsnull, ni, value);
      if (!domAttribute) {
        return NS_ERROR_OUT_OF_MEMORY;
      }

      rv = domAttribute->QueryInterface(NS_GET_IID(nsIDOMAttr),
                                        (void **)aReturn);
    } else {
      return NS_ERROR_DOM_NOT_FOUND_ERR;
    }

    rv = mContent->UnsetAttribute(nsid, nameAtom, PR_TRUE);
  }

  return rv;
}


nsresult
nsDOMAttributeMap::Item(PRUint32 aIndex, nsIDOMNode** aReturn)
{
  NS_ENSURE_ARG_POINTER(aReturn);
  PRInt32 nameSpaceID;
  nsCOMPtr<nsIAtom> nameAtom, prefix;

  nsresult rv = NS_OK;
  if (mContent &&
      NS_SUCCEEDED(mContent->GetAttributeNameAt(aIndex,
                                                nameSpaceID,
                                                *getter_AddRefs(nameAtom),
                                                *getter_AddRefs(prefix)))) {
    nsAutoString value, name;
    mContent->GetAttribute(nameSpaceID, nameAtom, value);

    nsCOMPtr<nsINodeInfo> ni;
    mContent->GetNodeInfo(*getter_AddRefs(ni));
    NS_ENSURE_TRUE(ni, NS_ERROR_FAILURE);

    nsCOMPtr<nsINodeInfoManager> nimgr;
    ni->GetNodeInfoManager(*getter_AddRefs(nimgr));
    NS_ENSURE_TRUE(nimgr, NS_ERROR_FAILURE);

    nimgr->GetNodeInfo(nameAtom, prefix, nameSpaceID, *getter_AddRefs(ni));
    NS_ENSURE_TRUE(ni, NS_ERROR_FAILURE);

    nsDOMAttribute* domAttribute = new nsDOMAttribute(mContent, ni, value);
    NS_ENSURE_TRUE(domAttribute, NS_ERROR_OUT_OF_MEMORY);

    rv = domAttribute->QueryInterface(NS_GET_IID(nsIDOMAttr),
                                      (void **)aReturn);
  }
  else {
    *aReturn = nsnull;
  }

  return rv;
}

nsresult
nsDOMAttributeMap::GetLength(PRUint32 *aLength)
{
  NS_ENSURE_ARG_POINTER(aLength);

  PRInt32 n;
  nsresult rv = NS_OK;

  if (nsnull != mContent) {
    rv = mContent->GetAttributeCount(n);
    *aLength = PRUint32(n);
  } else {
    *aLength = 0;
  }
  return rv;
}

nsresult
nsDOMAttributeMap::GetNamedItemNS(const nsAReadableString& aNamespaceURI,
                                  const nsAReadableString& aLocalName,
                                  nsIDOMNode** aReturn)
{
  NS_ENSURE_ARG_POINTER(aReturn);
  *aReturn = nsnull;

  nsresult rv = NS_OK;
  if (mContent) {
    nsCOMPtr<nsIAtom> nameAtom(dont_AddRef(NS_NewAtom(aLocalName)));
    PRInt32 nameSpaceID = kNameSpaceID_None;
    nsCOMPtr<nsIAtom> prefix;

    nsCOMPtr<nsINodeInfo> ni;
    mContent->GetNodeInfo(*getter_AddRefs(ni));
    NS_ENSURE_TRUE(ni, NS_ERROR_FAILURE);

    nsCOMPtr<nsINodeInfoManager> nimgr;
    ni->GetNodeInfoManager(*getter_AddRefs(nimgr));
    NS_ENSURE_TRUE(nimgr, NS_ERROR_FAILURE);

    if (aNamespaceURI.Length()) {
      nsCOMPtr<nsINameSpaceManager> nsmgr;
      nimgr->GetNamespaceManager(*getter_AddRefs(nsmgr));
      NS_ENSURE_TRUE(nsmgr, NS_ERROR_FAILURE);

      nsmgr->GetNameSpaceID(aNamespaceURI, nameSpaceID);

      if (nameSpaceID == kNameSpaceID_Unknown)
        return NS_OK;
    }

    nsresult attrResult;
    nsAutoString value;

    attrResult = mContent->GetAttribute(nameSpaceID, nameAtom,
                                        *getter_AddRefs(prefix), value);

    if (NS_CONTENT_ATTR_NOT_THERE != attrResult && NS_SUCCEEDED(attrResult)) {
      nimgr->GetNodeInfo(nameAtom, prefix, nameSpaceID, *getter_AddRefs(ni));
      NS_ENSURE_TRUE(ni, NS_ERROR_FAILURE);

      nsDOMAttribute* domAttribute;
      domAttribute = new nsDOMAttribute(mContent, ni, value);
      NS_ENSURE_TRUE(domAttribute, NS_ERROR_OUT_OF_MEMORY);

      rv = domAttribute->QueryInterface(NS_GET_IID(nsIDOMAttr),
                                        (void **)aReturn);
    }
  }

  return rv;
}

nsresult
nsDOMAttributeMap::SetNamedItemNS(nsIDOMNode* aArg, nsIDOMNode** aReturn)
{
  NS_ENSURE_ARG_POINTER(aReturn);

  nsresult rv = NS_OK;
  *aReturn = nsnull;

  if (mContent) {
    nsCOMPtr<nsIDOMAttr> attribute(do_QueryInterface(aArg));

    if (!attribute) {
      return NS_ERROR_DOM_HIERARCHY_REQUEST_ERR;
    }

    nsAutoString name, nsURI, value;
    nsCOMPtr<nsIAtom> nameAtom;
    PRInt32 nameSpaceID;

    attribute->GetName(name);
    attribute->GetPrefix(name);
    attribute->GetNamespaceURI(nsURI);

    nsCOMPtr<nsINodeInfo> ni;
    mContent->GetNodeInfo(*getter_AddRefs(ni));
    NS_ENSURE_TRUE(ni, NS_ERROR_FAILURE);

    nsCOMPtr<nsINodeInfoManager> nimgr;
    ni->GetNodeInfoManager(*getter_AddRefs(nimgr));
    NS_ENSURE_TRUE(nimgr, NS_ERROR_FAILURE);

    nimgr->GetNodeInfo(name, nsURI, *getter_AddRefs(ni));
    NS_ENSURE_TRUE(ni, NS_ERROR_FAILURE);

    ni->GetNameAtom(*getter_AddRefs(nameAtom));
    ni->GetNamespaceID(nameSpaceID);

    nsresult attrResult = mContent->GetAttribute(nameSpaceID, nameAtom, value);

    if (NS_CONTENT_ATTR_NOT_THERE != attrResult && NS_SUCCEEDED(attrResult)) {
      nsDOMAttribute* domAttribute;
      // We pass a null content here since the attr node we return isn't
      // tied to this content anymore.
      domAttribute = new nsDOMAttribute(nsnull, ni, value);
      if (!domAttribute) {
        return NS_ERROR_OUT_OF_MEMORY;
      }

      rv = domAttribute->QueryInterface(NS_GET_IID(nsIDOMAttr),
                                        (void **)aReturn);
    }

    attribute->GetValue(value);

    rv = mContent->SetAttribute(ni, value, PR_TRUE);
  }

  return rv;
}

nsresult
nsDOMAttributeMap::RemoveNamedItemNS(const nsAReadableString& aNamespaceURI,
                                     const nsAReadableString& aLocalName,
                                     nsIDOMNode** aReturn)
{
  NS_ENSURE_ARG_POINTER(aReturn);
  *aReturn = nsnull;

  nsresult rv = NS_OK;

  if (mContent) {
    nsCOMPtr<nsIAtom> nameAtom(dont_AddRef(NS_NewAtom(aLocalName)));
    PRInt32 nameSpaceID = kNameSpaceID_None;
    nsCOMPtr<nsIDOMNode> attribute;
    nsCOMPtr<nsIAtom> prefix;

    nsCOMPtr<nsINodeInfo> ni;
    mContent->GetNodeInfo(*getter_AddRefs(ni));
    NS_ENSURE_TRUE(ni, NS_ERROR_FAILURE);

    nsCOMPtr<nsINodeInfoManager> nimgr;
    ni->GetNodeInfoManager(*getter_AddRefs(nimgr));
    NS_ENSURE_TRUE(nimgr, NS_ERROR_FAILURE);

    if (aNamespaceURI.Length()) {
      nsCOMPtr<nsINameSpaceManager> nsmgr;
      nimgr->GetNamespaceManager(*getter_AddRefs(nsmgr));
      NS_ENSURE_TRUE(nsmgr, NS_ERROR_FAILURE);

      nsmgr->GetNameSpaceID(aNamespaceURI, nameSpaceID);

      if (nameSpaceID == kNameSpaceID_Unknown)
        return NS_ERROR_DOM_NOT_FOUND_ERR;
    }

    nsresult attrResult;
    nsAutoString value;
    attrResult = mContent->GetAttribute(nameSpaceID, nameAtom,
                                        *getter_AddRefs(prefix), value);

    if (NS_CONTENT_ATTR_NOT_THERE != attrResult && NS_SUCCEEDED(attrResult)) {
      nimgr->GetNodeInfo(nameAtom, prefix, nameSpaceID, *getter_AddRefs(ni));
      NS_ENSURE_TRUE(ni, NS_ERROR_FAILURE);

      nsDOMAttribute* domAttribute;
      domAttribute = new nsDOMAttribute(nsnull, ni, value);
      if (!domAttribute) {
        return NS_ERROR_OUT_OF_MEMORY;
      }

      rv = domAttribute->QueryInterface(NS_GET_IID(nsIDOMAttr),
                                        (void **)aReturn);
    } else {
      return NS_ERROR_DOM_NOT_FOUND_ERR;
    }

    rv = mContent->UnsetAttribute(nameSpaceID, nameAtom, PR_TRUE);
  }

  return rv;
}


#ifdef DEBUG
nsresult
nsDOMAttributeMap::SizeOfNamedNodeMap(nsIDOMNamedNodeMap* aMap,
                                      nsISizeOfHandler* aSizer,
                                      PRUint32* aResult)
{
  if (!aResult) return NS_ERROR_NULL_POINTER;
  nsDOMAttributeMap* map = (nsDOMAttributeMap*) aMap;
  PRUint32 sum = sizeof(nsDOMAttributeMap);
  *aResult = sum;
  return NS_OK;
}
#endif
