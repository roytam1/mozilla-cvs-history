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
 * Contributor(s): 
 */

#ifndef __selectionstate_h__
#define __selectionstate_h__

#include "nsCOMPtr.h"
#include "nsVoidArray.h"
#include "nsIDOMNode.h"

class nsIDOMCharacterData;
class nsIDOMRange;
class nsISelection;

/***************************************************************************
 * class for recording selection info.  stores selection as collection of
 * { {startnode, startoffset} , {endnode, endoffset} } tuples.  Cant store
 * ranges since dom gravity will possibly change the ranges.
 */

// first a helper struct for saving/setting ranges
struct nsRangeStore 
{
  nsRangeStore();
  ~nsRangeStore();
  nsresult StoreRange(nsIDOMRange *aRange);
  nsresult GetRange(nsCOMPtr<nsIDOMRange> *outRange);
        
  nsCOMPtr<nsIDOMNode> startNode;
  PRInt32              startOffset;
  nsCOMPtr<nsIDOMNode> endNode;
  PRInt32              endOffset;
  // DEBUG:   static PRInt32 n;
};

class nsSelectionState
{
  public:
      
    nsSelectionState();
    ~nsSelectionState();
  
    nsresult SaveSelection(nsISelection *aSel);
    nsresult RestoreSelection(nsISelection *aSel);
    PRBool   IsCollapsed();
    PRBool   IsEqual(nsSelectionState *aSelState);
    void     MakeEmpty();
    PRBool   IsEmpty();
  protected:    
    nsVoidArray mArray;
    
    friend class nsRangeUpdater;
};

class nsRangeUpdater
{
  public:    
  
    nsRangeUpdater();
    ~nsRangeUpdater();
  
    void* RegisterRange(nsIDOMRange *aRange);
    nsCOMPtr<nsIDOMRange> ReclaimRange(void *aCookie);
    void DropRange(void *aCookie);
    void RegisterRangeItem(nsRangeStore *aRangeItem);
    void DropRangeItem(nsRangeStore *aRangeItem);
    nsresult RegisterSelectionState(nsSelectionState &aSelState);
    nsresult DropSelectionState(nsSelectionState &aSelState);
    
    // editor selection gravity routines.  Note that we can't always depend on
    // DOM Range gravity to do what we want to the "real" selection.  For instance,
    // if you move a node, that corresponds to deleting it and reinserting it.
    // DOM Range gravity will promote the selection out of the node on deletion,
    // which is not what you want if you know you are reinserting it.
    nsresult SelAdjCreateNode(nsIDOMNode *aParent, PRInt32 aPosition);
    nsresult SelAdjInsertNode(nsIDOMNode *aParent, PRInt32 aPosition);
    nsresult SelAdjDeleteNode(nsIDOMNode *aNode, nsIDOMNode *aParent, PRInt32 aOffset);
    nsresult SelAdjSplitNode(nsIDOMNode *aOldRightNode, PRInt32 aOffset, nsIDOMNode *aNewLeftNode);
    nsresult SelAdjJoinNodes(nsIDOMNode *aLeftNode, 
                             nsIDOMNode *aRightNode, 
                             nsIDOMNode *aParent, 
                             PRInt32 aOffset,
                             PRInt32 aOldLeftNodeLength);
    nsresult SelAdjInsertText(nsIDOMCharacterData *aTextNode, PRInt32 aOffset, const nsString &aString);
    nsresult SelAdjDeleteText(nsIDOMCharacterData *aTextNode, PRInt32 aOffset, PRInt32 aLength);
    // the following gravity routines need will/did sandwiches, because the other gravity
    // routines will be called inside of these sandwiches, but should be ignored.
    nsresult WillReplaceContainer();
    nsresult DidReplaceContainer(nsIDOMNode *aOriginalNode, nsIDOMNode *aNewNode);
    nsresult WillRemoveContainer();
    nsresult DidRemoveContainer(nsIDOMNode *aNode, nsIDOMNode *aParent, PRInt32 aOffset, PRUint32 aNodeOrigLen);
    nsresult WillInsertContainer();
    nsresult DidInsertContainer();
    nsresult WillMoveNode();
    nsresult DidMoveNode(nsIDOMNode *aOldParent, PRInt32 aOldOffset, nsIDOMNode *aNewParent, PRInt32 aNewOffset);
  protected:    
    nsVoidArray mArray;
    PRBool mLock;
};


/***************************************************************************
 * another helper class for nsSelectionState.  stack based class for doing
 * Will/DidReplaceContainer()
 */

class nsAutoReplaceContainerSelNotify
{
  private:
    nsRangeUpdater &mRU;
    nsIDOMNode *mOriginalNode;
    nsIDOMNode *mNewNode;

  public:
    nsAutoReplaceContainerSelNotify(nsRangeUpdater &aRangeUpdater, nsIDOMNode *aOriginalNode, nsIDOMNode *aNewNode) :
    mRU(aRangeUpdater)
    ,mOriginalNode(aOriginalNode)
    ,mNewNode(aNewNode)
    {
      mRU.WillReplaceContainer();
    }
    
    ~nsAutoReplaceContainerSelNotify()
    {
      mRU.DidReplaceContainer(mOriginalNode, mNewNode);
    }
};


/***************************************************************************
 * another helper class for nsSelectionState.  stack based class for doing
 * Will/DidRemoveContainer()
 */

class nsAutoRemoveContainerSelNotify
{
  private:
    nsRangeUpdater &mRU;
    nsIDOMNode *mNode;
    nsIDOMNode *mParent;
    PRInt32    mOffset;
    PRUint32   mNodeOrigLen;

  public:
    nsAutoRemoveContainerSelNotify(nsRangeUpdater &aRangeUpdater, 
                                   nsIDOMNode *aNode, 
                                   nsIDOMNode *aParent, 
                                   PRInt32 aOffset, 
                                   PRUint32 aNodeOrigLen) :
    mRU(aRangeUpdater)
    ,mNode(aNode)
    ,mParent(aParent)
    ,mOffset(aOffset)
    ,mNodeOrigLen(aNodeOrigLen)
    {
      mRU.WillRemoveContainer();
    }
    
    ~nsAutoRemoveContainerSelNotify()
    {
      mRU.DidRemoveContainer(mNode, mParent, mOffset, mNodeOrigLen);
    }
};

/***************************************************************************
 * another helper class for nsSelectionState.  stack based class for doing
 * Will/DidInsertContainer()
 */

class nsAutoInsertContainerSelNotify
{
  private:
    nsRangeUpdater &mRU;

  public:
    nsAutoInsertContainerSelNotify(nsRangeUpdater &aRangeUpdater) :
    mRU(aRangeUpdater)
    {
      mRU.WillInsertContainer();
    }
    
    ~nsAutoInsertContainerSelNotify()
    {
      mRU.DidInsertContainer();
    }
};


/***************************************************************************
 * another helper class for nsSelectionState.  stack based class for doing
 * Will/DidMoveNode()
 */

class nsAutoMoveNodeSelNotify
{
  private:
    nsRangeUpdater &mRU;
    nsIDOMNode *mOldParent;
    nsIDOMNode *mNewParent;
    PRInt32    mOldOffset;
    PRInt32    mNewOffset;

  public:
    nsAutoMoveNodeSelNotify(nsRangeUpdater &aRangeUpdater, 
                            nsIDOMNode *aOldParent, 
                            PRInt32 aOldOffset, 
                            nsIDOMNode *aNewParent, 
                            PRInt32 aNewOffset) :
    mRU(aRangeUpdater)
    ,mOldParent(aOldParent)
    ,mNewParent(aNewParent)
    ,mOldOffset(aOldOffset)
    ,mNewOffset(aNewOffset)
    {
      mRU.WillMoveNode();
    }
    
    ~nsAutoMoveNodeSelNotify()
    {
      mRU.DidMoveNode(mOldParent, mOldOffset, mNewParent, mNewOffset);
    }
};

#endif


