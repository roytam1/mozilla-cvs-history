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

#include "nsISupports.h"
#include "nsIContent.h"
#include "nsIDOMDocumentFragment.h"
#include "nsGenericElement.h"
#include "nsINameSpaceManager.h"
#include "nsINodeInfo.h"
#include "nsNodeInfoManager.h"
#include "nsIDocument.h"
#include "nsIDOMDocument.h"
#include "nsDOMError.h"


class nsDocumentFragment : public nsGenericContainerElement,
                           public nsIDocumentFragment,
                           public nsIDOM3Node
{
public:
  nsDocumentFragment(nsIDocument* aOwnerDocument);
  virtual ~nsDocumentFragment();

  // nsISupports
  NS_DECL_ISUPPORTS_INHERITED

  // interface nsIDocumentFragment
  NS_IMETHOD DisconnectChildren();
  NS_IMETHOD ReconnectChildren();
  NS_IMETHOD DropChildReferences();

  // interface nsIDOMDocumentFragment
  NS_IMETHOD    GetNodeName(nsAString& aNodeName)
  { return nsGenericContainerElement::GetNodeName(aNodeName); }
  NS_IMETHOD    GetNodeValue(nsAString& aNodeValue)
  { return nsGenericContainerElement::GetNodeValue(aNodeValue); }
  NS_IMETHOD    SetNodeValue(const nsAString& aNodeValue)
  { return nsGenericContainerElement::SetNodeValue(aNodeValue); }
  NS_IMETHOD    GetNodeType(PRUint16* aNodeType);
  NS_IMETHOD    GetParentNode(nsIDOMNode** aParentNode)
  { return nsGenericContainerElement::GetParentNode(aParentNode); }
  NS_IMETHOD    GetChildNodes(nsIDOMNodeList** aChildNodes)
  { return nsGenericContainerElement::GetChildNodes(aChildNodes); }
  NS_IMETHOD    GetFirstChild(nsIDOMNode** aFirstChild)
  { return nsGenericContainerElement::GetFirstChild(aFirstChild); }
  NS_IMETHOD    GetLastChild(nsIDOMNode** aLastChild)
  { return nsGenericContainerElement::GetLastChild(aLastChild); }
  NS_IMETHOD    GetPreviousSibling(nsIDOMNode** aPreviousSibling)
  { return nsGenericContainerElement::GetPreviousSibling(aPreviousSibling); }
  NS_IMETHOD    GetNextSibling(nsIDOMNode** aNextSibling)
  { return nsGenericContainerElement::GetNextSibling(aNextSibling); }
  NS_IMETHOD    GetAttributes(nsIDOMNamedNodeMap** aAttributes)
    {
      *aAttributes = nsnull;
      return NS_OK;
    }
  NS_IMETHOD    GetOwnerDocument(nsIDOMDocument** aOwnerDocument);
  NS_IMETHOD    InsertBefore(nsIDOMNode* aNewChild, nsIDOMNode* aRefChild, 
                             nsIDOMNode** aReturn)
  { return nsGenericContainerElement::InsertBefore(aNewChild, aRefChild,
                                                   aReturn); }
  NS_IMETHOD    ReplaceChild(nsIDOMNode* aNewChild, nsIDOMNode* aOldChild, 
                             nsIDOMNode** aReturn)
  { return nsGenericContainerElement::ReplaceChild(aNewChild, aOldChild,
                                                   aReturn); }
  NS_IMETHOD    RemoveChild(nsIDOMNode* aOldChild, nsIDOMNode** aReturn)
  { return nsGenericContainerElement::RemoveChild(aOldChild, aReturn); }
  NS_IMETHOD    AppendChild(nsIDOMNode* aNewChild, nsIDOMNode** aReturn)
  { return nsGenericContainerElement::AppendChild(aNewChild, aReturn); }
  NS_IMETHOD    HasChildNodes(PRBool* aReturn)
  { return nsGenericContainerElement::HasChildNodes(aReturn); }
  NS_IMETHOD    HasAttributes(PRBool* aReturn)
  { return nsGenericContainerElement::HasAttributes(aReturn); }
  NS_IMETHOD    CloneNode(PRBool aDeep, nsIDOMNode** aReturn);
  NS_IMETHOD    GetPrefix(nsAString& aPrefix)
  { return nsGenericContainerElement::GetPrefix(aPrefix); }
  NS_IMETHOD    SetPrefix(const nsAString& aPrefix);
  NS_IMETHOD    GetNamespaceURI(nsAString& aNamespaceURI)
  { return nsGenericContainerElement::GetNamespaceURI(aNamespaceURI); }
  NS_IMETHOD    GetLocalName(nsAString& aLocalName)
  { return nsGenericContainerElement::GetLocalName(aLocalName); }
  NS_IMETHOD    Normalize()
  { return nsGenericContainerElement::Normalize(); }
  NS_IMETHOD    IsSupported(const nsAString& aFeature,
                            const nsAString& aVersion,
                            PRBool* aReturn)
  { return nsGenericContainerElement::IsSupported(aFeature, aVersion,
                                                  aReturn); }

  // nsIDOM3Node
  NS_IMETHOD    GetBaseURI(nsAString& aURI)
  { aURI.Truncate(); return NS_OK; }
  NS_IMETHOD    CompareDocumentPosition(nsIDOMNode *aOther,
                                        PRUint16* aReturn);
  NS_IMETHOD    IsSameNode(nsIDOMNode *aOther, PRBool* aReturn);
  NS_IMETHOD    LookupNamespacePrefix(const nsAString& aNamespaceURI,
                                      nsAString& aPrefix) {
    aPrefix.Truncate(); return NS_OK;
  }
  NS_IMETHOD    LookupNamespaceURI(const nsAString& aNamespacePrefix,
                                   nsAString& aNamespaceURI) {
    aNamespaceURI.Truncate(); return NS_OK;
  }

  // nsIContent
  NS_IMETHOD SetParent(nsIContent* aParent)
    { return NS_OK; }
  NS_IMETHOD SetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                     const nsAString& aValue,
                     PRBool aNotify)
    { return NS_OK; }
  NS_IMETHOD SetAttr(nsINodeInfo* aNodeInfo,
                     const nsAString& aValue,
                     PRBool aNotify)
    { return NS_OK; }
  NS_IMETHOD GetAttr(PRInt32 aNameSpaceID, nsIAtom* aName, 
                     nsAString& aResult) const
    { return NS_CONTENT_ATTR_NOT_THERE; }
  NS_IMETHOD GetAttr(PRInt32 aNameSpaceID, nsIAtom* aName, 
                     nsIAtom*& aPrefix, nsAString& aResult) const
    { return NS_CONTENT_ATTR_NOT_THERE; }
  NS_IMETHOD UnsetAttr(PRInt32 aNameSpaceID, nsIAtom* aAttribute, 
                       PRBool aNotify)
    { return NS_OK; }
  NS_IMETHOD GetAttrNameAt(PRInt32 aIndex,
                           PRInt32& aNameSpaceID, 
                           nsIAtom*& aName,
                           nsIAtom*& aPrefix) const
    {
      aName = nsnull;
      aPrefix = nsnull;
      return NS_ERROR_ILLEGAL_VALUE;
    }
  NS_IMETHOD HandleDOMEvent(nsIPresContext* aPresContext,
                            nsEvent* aEvent,
                            nsIDOMEvent** aDOMEvent,
                            PRUint32 aFlags,
                            nsEventStatus* aEventStatus)
    {
      NS_ENSURE_ARG_POINTER(aEventStatus);
      *aEventStatus = nsEventStatus_eIgnore;
      return NS_OK;
    }

protected:
  nsCOMPtr<nsIDocument> mOwnerDocument;
};

nsresult
NS_NewDocumentFragment(nsIDOMDocumentFragment** aInstancePtrResult,
                       nsIDocument* aOwnerDocument)
{
  NS_ENSURE_ARG_POINTER(aInstancePtrResult);

  nsCOMPtr<nsINodeInfoManager> nimgr;
  nsCOMPtr<nsINodeInfo> nodeInfo;

  nsresult rv;

  if (aOwnerDocument) {
    rv = aOwnerDocument->GetNodeInfoManager(*getter_AddRefs(nimgr));
  } else {
    rv = nsNodeInfoManager::GetAnonymousManager(*getter_AddRefs(nimgr));
  }
  NS_ENSURE_SUCCESS(rv, rv);

  rv = nimgr->GetNodeInfo(NS_LITERAL_STRING("#document-fragment"),
                          nsnull, kNameSpaceID_None,
                          *getter_AddRefs(nodeInfo));
  NS_ENSURE_SUCCESS(rv, rv);

  nsDocumentFragment* it = new nsDocumentFragment(aOwnerDocument);
  if (!it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  rv = it->Init(nodeInfo);

  if (NS_FAILED(rv)) {
    delete it;

    return rv;
  }

  *aInstancePtrResult = NS_STATIC_CAST(nsIDOMDocumentFragment *, it);

  NS_ADDREF(*aInstancePtrResult);

  return NS_OK;
}

nsDocumentFragment::nsDocumentFragment(nsIDocument* aOwnerDocument)
{
  mOwnerDocument = aOwnerDocument;
}

nsDocumentFragment::~nsDocumentFragment()
{
}


// QueryInterface implementation for nsDocumentFragment
NS_INTERFACE_MAP_BEGIN(nsDocumentFragment)
  NS_INTERFACE_MAP_ENTRY(nsIDocumentFragment)
  NS_INTERFACE_MAP_ENTRY(nsIDOMDocumentFragment)
  NS_INTERFACE_MAP_ENTRY(nsIDOMNode)
  NS_INTERFACE_MAP_ENTRY(nsIDOM3Node)
  NS_INTERFACE_MAP_ENTRY(nsIContent)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIContent)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(DocumentFragment)
NS_INTERFACE_MAP_END


NS_IMPL_ADDREF(nsDocumentFragment)
NS_IMPL_RELEASE(nsDocumentFragment)

NS_IMETHODIMP
nsDocumentFragment::DisconnectChildren()
{
  nsCOMPtr<nsIContent> child;
  PRInt32 i, count;

  ChildCount(count);

  for (i = 0; i < count; i++) {
    ChildAt(i, *getter_AddRefs(child));
    NS_ASSERTION(child, "Bad content container");

    child->SetParent(nsnull);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDocumentFragment::ReconnectChildren()
{
  nsCOMPtr<nsIContent> child, parent;
  PRInt32 i, count = 0;

  ChildCount(count);

  for (i = 0; i < count; i++) {
    ChildAt(i, *getter_AddRefs(child));
    NS_ASSERTION(child, "Bad content container");

    child->GetParent(*getter_AddRefs(parent));

    if (parent) {
      PRInt32 indx = -1;

      // This is potentially a O(n**2) operation, but it should only
      // happen in error cases (such as out of memory or something
      // similar) so we don't care for now.

      parent->IndexOf(child, indx);

      if (indx >= 0) {
        parent->RemoveChildAt(indx, PR_TRUE);
      }
    }

    child->SetParent(this);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDocumentFragment::DropChildReferences()
{
  PRInt32 count;

  ChildCount(count);

  for (PRInt32 index = 0; index < count; ++index) {
    nsIContent* kid = NS_STATIC_CAST(nsIContent*, mChildren.ElementAt(index));
    NS_RELEASE(kid);
  }

  mChildren.Clear();

  return NS_OK;
}

NS_IMETHODIMP    
nsDocumentFragment::GetNodeType(PRUint16* aNodeType)
{
  *aNodeType = (PRUint16)nsIDOMNode::DOCUMENT_FRAGMENT_NODE;
  return NS_OK;
}

NS_IMETHODIMP    
nsDocumentFragment::GetOwnerDocument(nsIDOMDocument** aOwnerDocument)
{
  NS_ENSURE_ARG_POINTER(aOwnerDocument);

  if (!mOwnerDocument) {
    *aOwnerDocument = nsnull;
    return NS_OK;
  }

  return CallQueryInterface(mOwnerDocument, aOwnerDocument);
}

NS_IMETHODIMP
nsDocumentFragment::SetPrefix(const nsAString& aPrefix)
{
  return NS_ERROR_DOM_NAMESPACE_ERR;
}

NS_IMETHODIMP    
nsDocumentFragment::CloneNode(PRBool aDeep, nsIDOMNode** aReturn)
{
  NS_ENSURE_ARG_POINTER(aReturn);
  *aReturn = nsnull;

  nsresult rv = NS_OK;
  nsCOMPtr<nsIDOMDocumentFragment> newFragment;

  rv = NS_NewDocumentFragment(getter_AddRefs(newFragment), mOwnerDocument);
  NS_ENSURE_SUCCESS(rv, rv);

  if (aDeep) {
    nsCOMPtr<nsIDOMNodeList> childNodes;

    GetChildNodes(getter_AddRefs(childNodes));
    if (childNodes) {
      PRUint32 index, count;
      childNodes->GetLength(&count);

      for (index = 0; index < count; ++index) {
        nsCOMPtr<nsIDOMNode> child;
        childNodes->Item(index, getter_AddRefs(child));
        if (child) {
          nsCOMPtr<nsIDOMNode> newChild;
          rv = child->CloneNode(PR_TRUE, getter_AddRefs(newChild));
          NS_ENSURE_SUCCESS(rv, rv);

          nsCOMPtr<nsIDOMNode> dummyNode;
          rv = newFragment->AppendChild(newChild,
                                        getter_AddRefs(dummyNode));
          NS_ENSURE_SUCCESS(rv, rv);
        }
      } // End of for loop
    } // if (childNodes)
  } // if (aDeep)

  return CallQueryInterface(newFragment, aReturn);
}

NS_IMETHODIMP
nsDocumentFragment::CompareDocumentPosition(nsIDOMNode* aOther,
                                            PRUint16* aReturn)
{
  NS_ENSURE_ARG_POINTER(aOther);
  NS_PRECONDITION(aReturn, "Must have an out parameter");

  if (this == aOther) {
    // If the two nodes being compared are the same node,
    // then no flags are set on the return.
    *aReturn = 0;
    return NS_OK;
  }

  PRUint16 mask = 0;

  nsCOMPtr<nsIDOMNode> other(aOther);
  do {
    nsCOMPtr<nsIDOMNode> tmp(other);
    tmp->GetParentNode(getter_AddRefs(other));
    if (!other) {
      // No parent.  Check to see if we're at an attribute node.
      PRUint16 nodeType = 0;
      tmp->GetNodeType(&nodeType);
      if (nodeType != nsIDOMNode::ATTRIBUTE_NODE) {
        // If there is no common container node, then the order
        // is based upon order between the root container of each
        // node that is in no container. In this case, the result
        // is disconnected and implementation-dependent.
        mask |= (nsIDOMNode::DOCUMENT_POSITION_DISCONNECTED |
                 nsIDOMNode::DOCUMENT_POSITION_IMPLEMENTATION_SPECIFIC);

        break;
      }

      // If we are, let's get the owner element and continue up the tree
      nsCOMPtr<nsIDOMAttr> attr(do_QueryInterface(tmp));
      nsCOMPtr<nsIDOMElement> owner;
      attr->GetOwnerElement(getter_AddRefs(owner));
      other = do_QueryInterface(owner);
    }

    if (NS_STATIC_CAST(nsIDOMNode*, this) == other) {
      // If the node being compared is contained by our node,
      // then it follows it.
      mask |= (nsIDOMNode::DOCUMENT_POSITION_IS_CONTAINED |
               nsIDOMNode::DOCUMENT_POSITION_FOLLOWING);
      break;
    }
  } while (other);

  *aReturn = mask;
  return NS_OK;
}

NS_IMETHODIMP
nsDocumentFragment::IsSameNode(nsIDOMNode* aOther,
                               PRBool* aReturn)
{
  PRBool sameNode = PR_FALSE;

  if (NS_STATIC_CAST(nsIDOMNode*, this) == aOther) {
    sameNode = PR_TRUE;
  }

  *aReturn = sameNode;
  return NS_OK;
}

