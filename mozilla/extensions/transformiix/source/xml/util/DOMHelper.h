/*
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is TransforMiiX XSLT processor.
 * 
 * The Initial Developer of the Original Code is Keith Visco.
 * Portions created by Keith Visco (C) 1999 Keith Visco.
 * All Rights Reserved..
 *
 * Contributor(s): 
 * Keith Visco, kvisco@ziplink.net
 *    -- original author.
 *
 * $Id$
 */

#ifndef TRANSFRMX_DOMHELPER_H
#define TRANSFRMX_DOMHELPER_H

#include "baseutils.h"
#include "TxString.h"
#include "dom.h"
#include "Map.h"
#include "TxObject.h"


//----------------------/
//- Class declarations -/
//----------------------/

/**
 * A class used by DOMHelper to hold document order information
 * for DOM Nodes
**/
class OrderInfo : public TxObject {

 public:

  OrderInfo();
  virtual ~OrderInfo();

  int compareTo(OrderInfo* orderInfo);

  int* order;
  int size;

}; //-- OrderInfo

/**
 * A class used to overcome DOM 1.0 deficiencies
 * @author <a href="mailto:kvisco@ziplink.net">Keith Visco</a>
 * @version $Revision$ $Date$
**/
class DOMHelper {

public:

    /**
     * Creates a new DOMHelper
    **/
    DOMHelper();

    /**
     * Deletes this DOMHelper
    **/
    virtual ~DOMHelper();

   /**
    * Returns the node which appears first in the document
    * (ie. has lower document order).
    * If this method is called with nodes from a different
    * document, node1 will be returned.
    * @return the node which appears first in document order
   **/
    Node* appearsFirst(Node* node1, Node* node2);

    /**
     * Generates a unique ID for the given node and places the result in
     * dest
    **/
    void generateId(Node* node, String& dest);

    /**
     * Returns the child number of the given node. Numbering
     * starts at 1 for all nodes except the Document node and 
     * attribute nodes which has child numbers of 0. The child
     * number is calculated by counting the number of times
     * Node#getPreviousSibling can be called.
     * @param node a pointer to the node in which to return the
     * child number of.
     * @return the child number for the given node
    **/
    int getChildNumber(Node* node);
 
    /**
     * Returns the parent of the given node. This method is available
     * mainly to compensate for the fact that Attr nodes in DOM 1.0
     * do not have parents. (Why??)
     * @param node the Node to return the parent of
    **/
    Node* getParentNode(Node* node);

    
private:


    /**
     * Adds the given child/parent mapping to the list of parents
    **/
    void addParentReference(Node* child, Node* parent);

    /**
     * Indexes until the given node has been indexed
    **/
    void continueIndexing(Node* node);


    /**
     * Returns the DocumentOrder for the given Node
     * @param node a pointer to the Node in which to return the 
     * DocumentOrder of
     * @return the DocumentOrder for the given Node
    **/
    OrderInfo* getDocumentOrder(Node* node);


    /**
     * A Hashtable of attribute's parent nodes
    **/
    Map parents;

    /**
     * A Hashtable of Node/OrderInfo mappings
    **/
    Map orders;

    /**
     * A list of IndexState objects (one for each Document)
    **/
    List  indexes;

    
}; //-- DOMHelper

/**
 * A class which holds information about the current state of
 * indexing
**/
class IndexState {

 public:

  IndexState();
  ~IndexState();
 
  /**
   * The Document that this IndexState is for
  **/
  Document* document;

  /**
   * The next node to index
  **/
  Node*    next;

  /**
   * A boolean indicating if the indexing has completed
  **/
  MBool    done;

}; //-- IndexState


#endif

