/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

#ifndef nsIEditorSupport_h__
#define nsIEditorSupport_h__
#include "nsISupports.h"

class nsIDOMNode;

/*
Private Editor interface for a class that can provide helper functions
*/

#define NS_IEDITORSUPPORT_IID \
{/* 89b999b0-c529-11d2-86da-000064657374*/ \
0x89b999b0, 0xc529, 0x11d2, \
{0x86, 0xda, 0x0, 0x0, 0x64, 0x65, 0x73, 0x74} }


/**
 */
class nsIEditorSupport  : public nsISupports {

public:

  static const nsIID& IID() { static nsIID iid = NS_IEDITORSUPPORT_IID; return iid; }

  /** 
   * SplitNode() creates a new node identical to an existing node, and split the contents between the two nodes
   * @param aExistingRightNode   the node to split.  It will become the new node's next sibling.
   * @param aOffset              the offset of aExistingRightNode's content|children to do the split at
   * @param aNewLeftNode         [OUT] the new node resulting from the split, becomes aExistingRightNode's previous sibling.
   * @param aParent              the parent of aExistingRightNode
   */
  virtual nsresult SplitNodeImpl(nsIDOMNode * aExistingRightNode,
                                 PRInt32      aOffset,
                                 nsIDOMNode * aNewLeftNode,
                                 nsIDOMNode * aParent)=0;

  /** 
   * JoinNodes() takes 2 nodes and merge their content|children.
   * @param aNodeToKeep   The node that will remain after the join.
   * @param aNodeToJoin   The node that will be joined with aNodeToKeep.
   *                      There is no requirement that the two nodes be of the same type.
   * @param aParent       The parent of aExistingRightNode
   * @param aNodeToKeepIsFirst  if PR_TRUE, the contents|children of aNodeToKeep come before the
   *                            contents|children of aNodeToJoin, otherwise their positions are switched.
   */
  virtual nsresult JoinNodesImpl(nsIDOMNode *aNodeToKeep,
                                 nsIDOMNode  *aNodeToJoin,
                                 nsIDOMNode  *aParent,
                                 PRBool       aNodeToKeepIsFirst)=0;
  


};

#endif //nsIEditorSupport_h__

