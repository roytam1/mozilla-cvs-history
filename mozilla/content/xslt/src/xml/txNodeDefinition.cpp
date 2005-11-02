/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * (C) Copyright The MITRE Corporation 1999  All rights reserved.
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * The program provided "as is" without any warranty express or
 * implied, including the warranty of non-infringement and the implied
 * warranties of merchantibility and fitness for a particular purpose.
 * The Copyright owner will not be liable for any damages suffered by
 * you as a result of using the Program. In no event will the Copyright
 * owner be liable for any special, indirect or consequential damages or
 * lost profits even if the Copyright owner has been advised of the
 * possibility of their occurrence.
 *
 * Please see release.txt distributed with this file for more information.
 *
 */
// Tom Kneeland (3/29/99)
//
//  Implementation of the Document Object Model Level 1 Core
//    Implementation of the NodeDefinition Class
//
// Modification History:
// Who  When      What
// TK   03/29/99  Created
//

#include "dom.h"
#include "ArrayList.h"
#include "txURIUtils.h"
#include "txAtoms.h"
#include <string.h>

NodeDefinition::NodeDefinition(NodeType type, const String& name,
                               const String& value, Document* owner)
{

  nodeName = name;
  nodeValue = value;
  nodeType = type;

  parentNode = NULL;
  previousSibling = NULL;
  nextSibling = NULL;;
  firstChild = NULL;
  lastChild = NULL;

  ownerDocument = owner;
  length = 0;

  mOrderInfo = 0;
}

//
// This node is being destroyed, so loop through and destroy all the children.
//
NodeDefinition::~NodeDefinition()
{
  DeleteChildren();
  delete mOrderInfo;
}

//
//Remove and delete all children of this node
//
void NodeDefinition::DeleteChildren()
{
  NodeDefinition* pCurrent = firstChild;
  NodeDefinition* pDestroyer;

  while (pCurrent)
    {
      pDestroyer = pCurrent;
      pCurrent = pCurrent->nextSibling;
      delete pDestroyer;
    }

  length = 0;
  firstChild = NULL;
  lastChild = NULL;
}

const String& NodeDefinition::getNodeName() const
{
  return nodeName;
}

const String& NodeDefinition::getNodeValue()
{
  return nodeValue;
}

unsigned short NodeDefinition::getNodeType() const
{
  return nodeType;
}

Node* NodeDefinition::getParentNode() const
{
  return parentNode;
}

NodeList* NodeDefinition::getChildNodes()
{
  return this;
}

Node* NodeDefinition::getFirstChild() const
{
  return firstChild;
}

Node* NodeDefinition::getLastChild() const
{
  return lastChild;
}

Node* NodeDefinition::getPreviousSibling() const
{
  return previousSibling;
}

Node* NodeDefinition::getNextSibling() const
{
  return nextSibling;
}

NamedNodeMap* NodeDefinition::getAttributes()
{
  return 0;
}

Document* NodeDefinition::getOwnerDocument() const
{
  return ownerDocument;
}

Node* NodeDefinition::item(PRUint32 index)
{
  PRUint32 selectLoop;
  NodeDefinition* pSelectNode = firstChild;

  if (index < length)
    {
      for (selectLoop=0;selectLoop<index;selectLoop++)
        pSelectNode = pSelectNode->nextSibling;

      return pSelectNode;
    }

  return NULL;
}

PRUint32 NodeDefinition::getLength()
{
  return length;
}

void NodeDefinition::setNodeValue(const String& newNodeValue)
{
  nodeValue = newNodeValue;
}

//
//Insert the "newChild" node before the "refChild" node.  Return a pointer to
//the inserted child.  If the node to insert is a document fragment, then
//insert each child of the document fragment, and return the document fragment
//which should be empty if all the inserts suceeded.
//This function's responsibility is to check for and handle document fragments
//vs. plain nodes.
//     *** NOTE: Need to check the document types before inserting.
//
//               The decision to return the possibly empty document fragment
//               was an implementation choice.  The spec did not dictate what
//               whould occur.
//
Node* NodeDefinition::insertBefore(Node* newChild,
                                   Node* refChild)
{
  NodeDefinition* pCurrentNode = NULL;
  NodeDefinition* pNextNode = NULL;

  //Convert to a NodeDefinition Pointer
  NodeDefinition* pNewChild = (NodeDefinition*)newChild;
  NodeDefinition* pRefChild = (NodeDefinition*)refChild;

  //Check to see if the reference node is a child of this node
  if ((refChild != NULL) && (pRefChild->parentNode != this))
    return NULL;

  if (newChild->getNodeType() == Node::DOCUMENT_FRAGMENT_NODE)
    {
      pCurrentNode = pNewChild->firstChild;
      while (pCurrentNode)
        {
          pNextNode = pCurrentNode->nextSibling;
          pCurrentNode = (NodeDefinition*)pNewChild->removeChild(pCurrentNode);
          implInsertBefore(pCurrentNode, pRefChild);
          pCurrentNode = pNextNode;
        }
      return newChild;
    }
  else
    return implInsertBefore(pNewChild, pRefChild);
}

//
//The code that actually insert one node before another.
//
Node* NodeDefinition::implInsertBefore(NodeDefinition* pNewChild,
                                       NodeDefinition* pRefChild)
{
  //Remove the "newChild" if it is already a child of this node
  if (pNewChild->parentNode == this)
    pNewChild = (NodeDefinition*)removeChild(pNewChild);

  //The new child should not be a child of any other node
  if ((pNewChild->previousSibling == NULL) &&
      (pNewChild->nextSibling == NULL) &&
      (pNewChild->parentNode == NULL))
      {
        if (pRefChild == NULL)
          {
            //Append
            pNewChild->previousSibling = lastChild;

            if (lastChild)
              lastChild->nextSibling = pNewChild;

            lastChild = pNewChild;
          }
        else
          {
            //Insert before the reference node
            if (pRefChild->previousSibling)
              pRefChild->previousSibling->nextSibling = pNewChild;
            pNewChild->nextSibling = pRefChild;
            pNewChild->previousSibling = pRefChild->previousSibling;
            pRefChild->previousSibling = pNewChild;
          }

        pNewChild->parentNode = this;

        if (pNewChild->previousSibling == NULL)
            firstChild = pNewChild;

        length++;

        return pNewChild;
      }

  return NULL;
}


//
//Replace "oldChild" with "newChild".  Return the replaced node, or NULL
//otherwise.
//    *** NOTE:  Need to check that the documents match ***
//
Node* NodeDefinition::replaceChild(Node* newChild,
                                         Node* oldChild)
{
  NodeDefinition* pOldChild = (NodeDefinition*)oldChild;
  NodeDefinition* pNextSibling = NULL;

  //If the newChild is replacing itself then we don't need to do anything
  if (pOldChild == newChild)
      return pOldChild;

  //If "oldChild" is a child of this node, remove it from the list.
  pOldChild = (NodeDefinition*)removeChild(oldChild);

  //If the removal was successful... Else, return null
  if (pOldChild)
    {
      //Try to insert the new node before the old node's next sibling.  If
      //successful, just returned the replaced child.  If not succesful,
      //reinsert the old node, and return NULL.
      pNextSibling = pOldChild->nextSibling;
      if (!insertBefore(newChild, pNextSibling))
        {
        insertBefore(pOldChild, pNextSibling);
        pOldChild = NULL;
        }
    }

  return pOldChild;
}

//
//Remove the specified "oldChild" from this node's children.  First make sure
//the specified node is a child of this node.  Return the removed node, NULL
//otherwise.
//
Node* NodeDefinition::removeChild(Node* oldChild)
{
  NodeDefinition* pOldChild = (NodeDefinition*)oldChild;

  //If "oldChild" is a child of this node, adjust pointers to remove it, and
  //clear "oldChild"'s sibling and parent pointers.
  if (pOldChild->parentNode == this)
    {
      if (pOldChild != firstChild)
        pOldChild->previousSibling->nextSibling = pOldChild->nextSibling;
      else
        firstChild = pOldChild->nextSibling;

      if (pOldChild != lastChild)
        pOldChild->nextSibling->previousSibling = pOldChild->previousSibling;
      else
        lastChild = pOldChild->previousSibling;

      pOldChild->nextSibling = NULL;
      pOldChild->previousSibling = NULL;
      pOldChild->parentNode = NULL;

      length--;

      return pOldChild;
    }

  return NULL;
}

//
//Append a new child node.  First make sure the new child is not already a
//child of another node.  Return the appended node.
//  *** NOTE *** Need to eventually check to make sure the documents match ***
//
Node* NodeDefinition::appendChild(Node* newChild)
{
  return insertBefore(newChild, NULL);
}

Node* NodeDefinition::cloneNode(MBool deep, Node* dest)
{
    return 0;
}

MBool NodeDefinition::hasChildNodes() const
{
  if (firstChild != NULL)
    return MB_TRUE;
  else
    return MB_FALSE;
}

MBool NodeDefinition::getLocalName(txAtom** aLocalName)
{
  if (!aLocalName)
    return MB_FALSE;
  *aLocalName = 0;
  return MB_TRUE;
}

const String& NodeDefinition::getNamespaceURI()
{
  return txNamespaceManager::getNamespaceURI(getNamespaceID());
}

PRInt32 NodeDefinition::getNamespaceID()
{
  return kNameSpaceID_None;
}

//
// Looks up the Namespace associated with a certain prefix in the context of
// this node.
//
// @return namespace associated with prefix
//
PRInt32 NodeDefinition::lookupNamespaceID(txAtom* aPrefix)
{
  // this is http://www.w3.org/2000/xmlns/,
  // ID = kNameSpaceID_XMLNS, see txNamespaceManager::Init
  if (aPrefix == txXMLAtoms::xmlns)
    return kNameSpaceID_XMLNS; 
  // this is http://www.w3.org/XML/1998/namespace,
  // ID = kNameSpaceID_XML, see txNamespaceManager::Init
  if (aPrefix == txXMLAtoms::xml)
    return kNameSpaceID_XML; 

  Node* node = this;
  if (node->getNodeType() != Node::ELEMENT_NODE)
    node = node->getXPathParent();

  String name("xmlns:");
  if (aPrefix && (aPrefix != txXMLAtoms::_empty)) {
      //  We have a prefix, search for xmlns:prefix attributes.
      String prefixString;
      TX_GET_ATOM_STRING(aPrefix, prefixString);
      name.append(prefixString);
  }
  else {
      // No prefix, look up the default namespace by searching for xmlns
      // attributes. Remove the trailing :, set length to 5 (xmlns).
      name.truncate(5);
  }
  Attr* xmlns;
  while (node && node->getNodeType() == Node::ELEMENT_NODE) {
    String nsURI;
    if ((xmlns = ((Element*)node)->getAttributeNode(name))) {
      /*
       * xmlns:foo = "" makes "" a valid URI, so get that.
       * xmlns = "" resolves to 0 (null Namespace) (caught above)
       * in Element::getNamespaceID()
       */
      return txNamespaceManager::getNamespaceID(xmlns->getValue());
    }
    node = node->getXPathParent();
  }
  if (!aPrefix || (aPrefix == txXMLAtoms::_empty))
      return kNameSpaceID_None;
  return kNameSpaceID_Unknown;
}

Node* NodeDefinition::getXPathParent()
{
  return parentNode;
}

//
// Returns the base URI of the node. Acccounts for xml:base
// attributes.
//
// @return base URI for the node
//
String NodeDefinition::getBaseURI()
{
  Node* node = this;
  ArrayList baseUrls;
  String url;
  String attValue;

  while (node) {
    switch (node->getNodeType()) {
      case Node::ELEMENT_NODE :
        if (((Element*)node)->getAttr(txXMLAtoms::base, kNameSpaceID_XML,
                                      attValue))
          baseUrls.add(new String(attValue));
        break;

      case Node::DOCUMENT_NODE :
        baseUrls.add(new String(((Document*)node)->getBaseURI()));
        break;
    
      default:
        break;
    }
    node = node->getParentNode();
  }

  if (baseUrls.size()) {
    url = *((String*)baseUrls.get(baseUrls.size()-1));

    for (int i=baseUrls.size()-2;i>=0;i--) {
      String dest;
      URIUtils::resolveHref(*(String*)baseUrls.get(i), url, dest);
      url = dest;
    }
  }

  baseUrls.clear(MB_TRUE);
  
  return url;
} // getBaseURI

/*
 * Compares document position of this node relative to another node
 */
PRInt32 NodeDefinition::compareDocumentPosition(Node* aOther)
{
  OrderInfo* myOrder = getOrderInfo();
  OrderInfo* otherOrder = ((NodeDefinition*)aOther)->getOrderInfo();
  if (!myOrder || !otherOrder)
      return -1;

  if (myOrder->mRoot == otherOrder->mRoot) {
    int c = 0;
    while (c < myOrder->mSize && c < otherOrder->mSize) {
      if (myOrder->mOrder[c] < otherOrder->mOrder[c])
        return -1;
      if (myOrder->mOrder[c] > otherOrder->mOrder[c])
        return 1;
      ++c;
    }
    if (c < myOrder->mSize)
      return 1;
    if (c < otherOrder->mSize)
      return -1;
    return 0;
  }

  if (myOrder->mRoot < otherOrder->mRoot)
    return -1;

  return 1;
}

/*
 * Get order information for node
 */
NodeDefinition::OrderInfo* NodeDefinition::getOrderInfo()
{
  if (mOrderInfo)
    return mOrderInfo;

  mOrderInfo = new OrderInfo;
  if (!mOrderInfo)
    return 0;

  Node* parent = getXPathParent();
  if (!parent) {
    mOrderInfo->mOrder = 0;
    mOrderInfo->mSize = 0;
    mOrderInfo->mRoot = this;
    return mOrderInfo;
  }

  OrderInfo* parentOrder = ((NodeDefinition*)parent)->getOrderInfo();
  mOrderInfo->mSize = parentOrder->mSize + 1;
  mOrderInfo->mRoot = parentOrder->mRoot;
  mOrderInfo->mOrder = new PRUint32[mOrderInfo->mSize];
  if (!mOrderInfo->mOrder) {
    delete mOrderInfo;
    mOrderInfo = 0;
    return 0;
  }
  memcpy(mOrderInfo->mOrder,
         parentOrder->mOrder,
         parentOrder->mSize * sizeof(PRUint32*));

  // Get childnumber of this node
  int lastElem = parentOrder->mSize;
  switch (getNodeType()) {
    case Node::ATTRIBUTE_NODE:
    {
      NS_ASSERTION(parent->getNodeType() == Node::ELEMENT_NODE,
                   "parent to attribute is not an element");

      Element* elem = (Element*)parent;
      PRUint32 i;
      NamedNodeMap* attrs = elem->getAttributes();
      for (i = 0; i < attrs->getLength(); ++i) {
        if (attrs->item(i) == this) {
          mOrderInfo->mOrder[lastElem] = i + kTxAttrIndexOffset;
          return mOrderInfo;
        }
      }
      break;
    }
    // XXX Namespace: need to take care of namespace nodes here
    default:
    {
      PRUint32 i = 0;
      Node * child = parent->getFirstChild();
      while (child) {
        if (child == this) {
          mOrderInfo->mOrder[lastElem] = i + kTxChildIndexOffset;
          return mOrderInfo;
        }
        ++i;
        child = child->getNextSibling();
      }
      break;
    }
  }

  NS_ASSERTION(0, "unable to get childnumber");
  mOrderInfo->mOrder[lastElem] = 0;
  return mOrderInfo;
}

/*
 * OrderInfo destructor
 */
NodeDefinition::OrderInfo::~OrderInfo()
{
    delete [] mOrder;
}
