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
 *   Pierre Phaneuf <pp@ludusdesign.com>
 */

/*
 * nsContentIterator.cpp: Implementation of the nsContentIterator object.
 * This ite
 */

#include "nsISupports.h"
#include "nsIDOMNodeList.h"
#include "nsIContentIterator.h"
#include "nsRange.h"
#include "nsIContent.h"
#include "nsIDOMText.h"
#include "nsISupportsArray.h"
#include "nsIFocusTracker.h"
#include "nsCOMPtr.h"
#include "nsIPresContext.h"
#include "nsIComponentManager.h"
#include "nsLayoutCID.h"

static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);
static NS_DEFINE_IID(kCContentIteratorCID, NS_CONTENTITERATOR_CID);
static NS_DEFINE_IID(kCSubtreeIteratorCID, NS_SUBTREEITERATOR_CID);

// couple of utility static functs

///////////////////////////////////////////////////////////////////////////
// GetNumChildren: returns the number of things inside aNode. 
//
static PRUint32
GetNumChildren(nsIDOMNode *aNode) 
{
  PRUint32 numChildren = 0;
  if (!aNode)
    return 0;

  PRBool hasChildNodes;
  aNode->HasChildNodes(&hasChildNodes);
  if (hasChildNodes)
  {
    nsCOMPtr<nsIDOMNodeList>nodeList;
    nsresult res = aNode->GetChildNodes(getter_AddRefs(nodeList));
    if (NS_SUCCEEDED(res) && nodeList) 
      nodeList->GetLength(&numChildren);
  }
  return numChildren;
}

///////////////////////////////////////////////////////////////////////////
// GetChildAt: returns the node at this position index in the parent
//
static nsCOMPtr<nsIDOMNode> 
GetChildAt(nsIDOMNode *aParent, PRInt32 aOffset)
{
  nsCOMPtr<nsIDOMNode> resultNode;
  
  if (!aParent) 
    return resultNode;
  
  PRBool hasChildNodes;
  aParent->HasChildNodes(&hasChildNodes);
  if (PR_TRUE==hasChildNodes)
  {
    nsCOMPtr<nsIDOMNodeList>nodeList;
    nsresult res = aParent->GetChildNodes(getter_AddRefs(nodeList));
    if (NS_SUCCEEDED(res) && nodeList) 
      nodeList->Item(aOffset, getter_AddRefs(resultNode));
  }
  
  return resultNode;
}
  



/*
 *  A simple iterator class for traversing the content in "close tag" order
 */
class nsContentIterator : public nsIContentIterator //, public nsIEnumerator
{
public:
  NS_DECL_ISUPPORTS

  nsContentIterator();
  virtual ~nsContentIterator();

  // nsIContentIterator interface methods ------------------------------

  NS_IMETHOD Init(nsIContent* aRoot);

  NS_IMETHOD Init(nsIDOMRange* aRange);

  NS_IMETHOD First();

  NS_IMETHOD Last();
  
  NS_IMETHOD Next();

  NS_IMETHOD Prev();

  NS_IMETHOD CurrentNode(nsIContent **aNode);

  NS_IMETHOD IsDone();

  NS_IMETHOD PositionAt(nsIContent* aCurNode);

  NS_IMETHOD MakePre();

  NS_IMETHOD MakePost();

  
  // nsIEnumertor interface methods ------------------------------
  
  //NS_IMETHOD CurrentItem(nsISupports **aItem);

protected:

  static nsCOMPtr<nsIContent> GetDeepFirstChild(nsCOMPtr<nsIContent> aRoot);
  static nsCOMPtr<nsIContent> GetDeepLastChild(nsCOMPtr<nsIContent> aRoot);
  
  nsresult GetNextSibling(nsCOMPtr<nsIContent> aNode, nsCOMPtr<nsIContent> *aSibling);
  nsresult GetPrevSibling(nsCOMPtr<nsIContent> aNode, nsCOMPtr<nsIContent> *aSibling);
  
  nsresult NextNode(nsCOMPtr<nsIContent> *ioNextNode);
  nsresult PrevNode(nsCOMPtr<nsIContent> *ioPrevNode);

  void MakeEmpty();
  
  nsCOMPtr<nsIContent> mCurNode;
  nsCOMPtr<nsIContent> mFirst;
  nsCOMPtr<nsIContent> mLast;
  nsCOMPtr<nsIContent> mCommonParent;

  PRBool mIsDone;
  PRBool mPre;
  
private:

  // no copy's or assigns  FIX ME
  nsContentIterator(const nsContentIterator&);
  nsContentIterator& operator=(const nsContentIterator&);

};

nsresult NS_NewContentIterator(nsIContentIterator** aInstancePtrResult);
nsresult NS_NewGeneratedContentIterator(nsIGeneratedContentIterator** aInstancePtrResult);

class nsGeneratedContentIterator : public nsContentIterator, nsIGeneratedContentIterator
{
public:
  NS_DECL_ISUPPORTS
  nsGeneratedContentIterator();
  virtual ~nsGeneratedContentIterator();

  // nsIGeneratedContentIterator interface methods ------------------------------

  NS_IMETHOD Init(nsIFocusTracker *aTracker, nsIDOMRange* aRange, PRBool aSelectBefore, PRBool aSelectAfter);
  NS_IMETHOD First();

  NS_IMETHOD Last();
  
  NS_IMETHOD Next();

  NS_IMETHOD Prev();

  NS_IMETHOD CurrentNode(nsIContent **aNode);

protected:
  PRBool mSelectBefore;//set at init time
  PRBool mSelectAfter;
  nsCOMPtr<nsIContentIterator> mGenIter;//reset for every before and after
  nsIFocusTracker *mTracker;//weak reference

  nsresult FillGenIter(PRInt8 aSide);
  PRInt8 mCurSideOfContent; //-1= before, 0 = current, 1=next
};


nsGeneratedContentIterator::nsGeneratedContentIterator()
:mSelectBefore(0),mSelectAfter(0), mCurSideOfContent(-1)
{
}

nsGeneratedContentIterator::~nsGeneratedContentIterator()
{
}


NS_IMPL_ADDREF_INHERITED(nsGeneratedContentIterator, nsContentIterator) 
NS_IMPL_RELEASE_INHERITED(nsGeneratedContentIterator, nsContentIterator) 

NS_IMETHODIMP
nsGeneratedContentIterator::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
  if (nsnull == aInstancePtr) {
    return NS_ERROR_NULL_POINTER;
  }
  if (aIID.Equals(nsIGeneratedContentIterator::GetIID())) {
    *aInstancePtr = NS_STATIC_CAST(nsIGeneratedContentIterator *,this);
    NS_ADDREF_THIS();
    return NS_OK;
  }
  return nsContentIterator::QueryInterface(aIID,aInstancePtr);
}


/******************************************************
 * repository cruft
 ******************************************************/

nsresult NS_NewContentIterator(nsIContentIterator** aInstancePtrResult)
{
  nsContentIterator * iter = new nsContentIterator();
  if (iter)
    return iter->QueryInterface(NS_GET_IID(nsIContentIterator), (void**) aInstancePtrResult);
  return NS_ERROR_OUT_OF_MEMORY;
}

nsresult NS_NewGeneratedContentIterator(nsIContentIterator** aInstancePtrResult)
{
  nsGeneratedContentIterator * iter = new nsGeneratedContentIterator();
  if (iter)
    return iter->QueryInterface(NS_GET_IID(nsIContentIterator), (void**) aInstancePtrResult);
  return NS_ERROR_OUT_OF_MEMORY;
}


/******************************************************
 * XPCOM cruft
 ******************************************************/
 
NS_IMPL_ADDREF(nsContentIterator)
NS_IMPL_RELEASE(nsContentIterator)

nsresult nsContentIterator::QueryInterface(const nsIID& aIID,
                                     void** aInstancePtrResult)
{
  NS_PRECONDITION(nsnull != aInstancePtrResult, "null pointer");
  if (nsnull == aInstancePtrResult) 
  {
    return NS_ERROR_NULL_POINTER;
  }
  if (aIID.Equals(kISupportsIID)) 
  {
    *aInstancePtrResult = (void*)(nsISupports*)(nsIContentIterator*)this;
    NS_ADDREF_THIS();
    return NS_OK;
  }
/*  if (aIID.Equals(NS_GET_IID(nsIEnumerator))) 
  {
    *aInstancePtrResult = (void*)(nsIEnumerator*)this;
    NS_ADDREF_THIS();
    return NS_OK;
  }  */
  if (aIID.Equals(NS_GET_IID(nsIContentIterator))) 
  {
    *aInstancePtrResult = (void*)(nsIContentIterator*)this;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  return NS_NOINTERFACE;
}


/******************************************************
 * constructor/destructor
 ******************************************************/

nsContentIterator::nsContentIterator() :
  // don't need to explicitly initialize |nsCOMPtr|s, they will automatically be NULL
  mIsDone(PR_FALSE), mPre(PR_FALSE)
{
  NS_INIT_REFCNT();
}


nsContentIterator::~nsContentIterator()
{
}


/******************************************************
 * Init routines
 ******************************************************/


nsresult nsContentIterator::Init(nsIContent* aRoot)
{
  if (!aRoot) 
    return NS_ERROR_NULL_POINTER; 
  mIsDone = PR_FALSE;
  nsCOMPtr<nsIContent> root( do_QueryInterface(aRoot) );
  mFirst = GetDeepFirstChild(root); 
  mLast = root;
  mCommonParent = root;
  mCurNode = mFirst;
  return NS_OK;
}


nsresult nsContentIterator::Init(nsIDOMRange* aRange)
{
  if (!aRange) 
    return NS_ERROR_NULL_POINTER; 

  nsCOMPtr<nsIDOMNode> dN;
  nsCOMPtr<nsIContent> cChild;
  
  nsCOMPtr<nsIContent> startCon;
  nsCOMPtr<nsIDOMNode> startDOM;
  nsCOMPtr<nsIContent> endCon;
  nsCOMPtr<nsIDOMNode> endDOM;
  PRInt32 startIndx;
  PRInt32 endIndx;
  
  mIsDone = PR_FALSE;

  // get common content parent
  if (NS_FAILED(aRange->GetCommonParent(getter_AddRefs(dN))) || !dN)
    return NS_ERROR_FAILURE;
  mCommonParent = do_QueryInterface(dN);

  // get the start node and offset, convert to nsIContent
  aRange->GetStartParent(getter_AddRefs(startDOM));
  if (!startDOM) 
    return NS_ERROR_ILLEGAL_VALUE;
  startCon = do_QueryInterface(startDOM);
  if (!startCon) 
    return NS_ERROR_FAILURE;
  
  aRange->GetStartOffset(&startIndx);
  
  // get the end node and offset, convert to nsIContent
  aRange->GetEndParent(getter_AddRefs(endDOM));
  if (!endDOM) 
    return NS_ERROR_ILLEGAL_VALUE;
  endCon = do_QueryInterface(endDOM);
  if (!endCon) 
    return NS_ERROR_FAILURE;

  aRange->GetEndOffset(&endIndx);
  
  // short circuit when start node == end node
  if (startDOM == endDOM)
  {
    startCon->ChildAt(0,*getter_AddRefs(cChild));
  
    if (!cChild) // no children, must be a text node or empty container
    {
      mFirst = startCon;
      mLast = startCon;
      mCurNode = startCon;
      return NS_OK;
    }
    else
    {
      if (startIndx == endIndx)  // collapsed range
      {
        MakeEmpty();
        return NS_OK;
      }
    }
  }
  
  // find first node in range
  startCon->ChildAt(0,*getter_AddRefs(cChild));
  
  if (!cChild) // no children, must be a text node
  {
    mFirst = startCon; 
  }
  else
  {
    startCon->ChildAt(startIndx,*getter_AddRefs(cChild));
    if (!cChild)  // offset after last child, parent is first node
    {
      mFirst = startCon;
    }
    else
    {
      mFirst = GetDeepFirstChild(cChild);
    }
    // Does that first node really intersect the range?
    // the range could be collapsed, or the range could be
    // 'degenerate', ie not collapsed but still containing
    // no content.  In this case, we want the iterator to
    // be empty
  
    if (!IsNodeIntersectsRange(mFirst, aRange))
    {
      MakeEmpty();
      return NS_OK;
    }
  }
  
  // find last node in range
  endCon->ChildAt(0,*getter_AddRefs(cChild));

  if (!cChild) // no children, must be a text node
  {
    mLast = endCon; 
  }
  else if (endIndx == 0) // before first child, parent is last node
  {
    mLast = endCon; 
  }
  else
  {
    endCon->ChildAt(--endIndx,*getter_AddRefs(cChild));
    if (!cChild)  // offset after last child, last child is last node
    {
      endCon->ChildCount(endIndx);
      endCon->ChildAt(--endIndx,*getter_AddRefs(cChild)); 
      if (!cChild)
      {
        NS_NOTREACHED("nsContentIterator::nsContentIterator");
        return NS_ERROR_FAILURE; 
      }
    }
    mLast = cChild;  
  }
  
  mCurNode = mFirst;
  return NS_OK;
}


/******************************************************
 * Helper routines
 ******************************************************/

void nsContentIterator::MakeEmpty()
{
  nsCOMPtr<nsIContent> noNode;
  mCurNode = noNode;
  mFirst = noNode;
  mLast = noNode;
  mCommonParent = noNode;
  mIsDone = PR_TRUE;
}

nsCOMPtr<nsIContent> nsContentIterator::GetDeepFirstChild(nsCOMPtr<nsIContent> aRoot)
{
  nsCOMPtr<nsIContent> deepFirstChild;
  
  if (aRoot) 
  {  
    nsCOMPtr<nsIContent> cN = aRoot;
    nsCOMPtr<nsIContent> cChild;
    cN->ChildAt(0,*getter_AddRefs(cChild));
    while ( cChild )
    {
      cN = cChild;
      cN->ChildAt(0,*getter_AddRefs(cChild));
    }
    deepFirstChild = cN;
  }
  
  return deepFirstChild;
}

nsCOMPtr<nsIContent> nsContentIterator::GetDeepLastChild(nsCOMPtr<nsIContent> aRoot)
{
  nsCOMPtr<nsIContent> deepFirstChild;
  
  if (aRoot) 
  {  
    nsCOMPtr<nsIContent> cN = aRoot;
    nsCOMPtr<nsIContent> cChild;
    PRInt32 numChildren;
  
    cN->ChildCount(numChildren);

    while ( numChildren )
    {
      cN->ChildAt(--numChildren,*getter_AddRefs(cChild));
      if (cChild)
      {
        cChild->ChildCount(numChildren);
        cN = cChild;
      }
      else
      {
        break;
      }
    }
    deepFirstChild = cN;
  }
  
  return deepFirstChild;
}

// Get the next sibling, or parents next sibling, or grandpa's next sibling...
nsresult nsContentIterator::GetNextSibling(nsCOMPtr<nsIContent> aNode, nsCOMPtr<nsIContent> *aSibling)
{
  if (!aNode) 
    return NS_ERROR_NULL_POINTER;
  if (!aSibling) 
    return NS_ERROR_NULL_POINTER;
  
  nsCOMPtr<nsIContent> sib;
  nsCOMPtr<nsIContent> parent;
  PRInt32              indx;
  
  if (NS_FAILED(aNode->GetParent(*getter_AddRefs(parent))) || !parent)
    return NS_ERROR_FAILURE;

  if (NS_FAILED(parent->IndexOf(aNode, indx)))
    return NS_ERROR_FAILURE;

  if (NS_SUCCEEDED(parent->ChildAt(++indx, *getter_AddRefs(sib))) && sib)
  {
    *aSibling = sib;
  }
  else if (parent != mCommonParent)
  {
    return GetNextSibling(parent, aSibling);
  }
  else
  {
    *aSibling = nsCOMPtr<nsIContent>();
  }
  
  return NS_OK;
}

// Get the prev sibling, or parents prev sibling, or grandpa's prev sibling...
nsresult nsContentIterator::GetPrevSibling(nsCOMPtr<nsIContent> aNode, nsCOMPtr<nsIContent> *aSibling)
{
  if (!aNode) 
    return NS_ERROR_NULL_POINTER;
  if (!aSibling) 
    return NS_ERROR_NULL_POINTER;
  
  nsCOMPtr<nsIContent> sib;
  nsCOMPtr<nsIContent> parent;
  PRInt32              indx;
  
  if (NS_FAILED(aNode->GetParent(*getter_AddRefs(parent))) || !parent)
    return NS_ERROR_FAILURE;

  if (NS_FAILED(parent->IndexOf(aNode, indx)))
    return NS_ERROR_FAILURE;

  if (indx && NS_SUCCEEDED(parent->ChildAt(--indx, *getter_AddRefs(sib))) && sib)
  {
    *aSibling = sib;
  }
  else if (parent != mCommonParent)
  {
    return GetPrevSibling(parent, aSibling);
  }
  else
  {
    *aSibling = nsCOMPtr<nsIContent>();
  }
  
  return NS_OK;
}

nsresult nsContentIterator::NextNode(nsCOMPtr<nsIContent> *ioNextNode)
{
  if (!ioNextNode)
    return NS_ERROR_NULL_POINTER;
    
  if (mPre)  // if we are a Pre-order iterator, use pre-order
  {
    nsCOMPtr<nsIContent> cN = *ioNextNode;
    nsCOMPtr<nsIContent> cFirstChild;
    PRInt32 numChildren;
  
    cN->ChildCount(numChildren);
  
    // if it has children then next node is first child
    if (numChildren)
    {
      if (NS_FAILED(cN->ChildAt(0,*getter_AddRefs(cFirstChild))))
        return NS_ERROR_FAILURE;
      if (!cFirstChild)
        return NS_ERROR_FAILURE;
      *ioNextNode = cFirstChild;
      return NS_OK;
    }
  
    // else next sibling is next
    return GetNextSibling(cN, ioNextNode);
  }
  else  // post-order
  {
    nsCOMPtr<nsIContent> cN = *ioNextNode;
    nsCOMPtr<nsIContent> cSibling;
    nsCOMPtr<nsIContent> parent;
    PRInt32              indx;
  
    // get next sibling if there is one
    if (NS_FAILED(cN->GetParent(*getter_AddRefs(parent))))
      return NS_ERROR_FAILURE;
    if (!parent || NS_FAILED(parent->IndexOf(cN, indx)))
    {
      // a little noise to catch some iterator usage bugs.
      NS_NOTREACHED("nsContentIterator::NextNode() : no parent found");
      return NS_ERROR_FAILURE;
    }
    if (NS_SUCCEEDED(parent->ChildAt(++indx,*getter_AddRefs(cSibling))) && cSibling)
    {
      // next node is siblings "deep left" child
      *ioNextNode = GetDeepFirstChild(cSibling); 
      return NS_OK;
    }
  
    // else it's the parent
    *ioNextNode = parent;
  }
  return NS_OK;
}

nsresult nsContentIterator::PrevNode(nsCOMPtr<nsIContent> *ioNextNode)
{
  if (!ioNextNode)
    return NS_ERROR_NULL_POINTER;
   
  if (mPre)  // if we are a Pre-order iterator, use pre-order
  {
    nsCOMPtr<nsIContent> cN = *ioNextNode;
    nsCOMPtr<nsIContent> cSibling;
    nsCOMPtr<nsIContent> parent;
    PRInt32              indx;
  
    // get prev sibling if there is one
    if (NS_FAILED(cN->GetParent(*getter_AddRefs(parent))))
      return NS_ERROR_FAILURE;
    if (!parent || NS_FAILED(parent->IndexOf(cN, indx)))
    {
      // a little noise to catch some iterator usage bugs.
      NS_NOTREACHED("nsContentIterator::PrevNode() : no parent found");
      return NS_ERROR_FAILURE;
    }
    if (indx && NS_SUCCEEDED(parent->ChildAt(--indx,*getter_AddRefs(cSibling))) && cSibling)
    {
      // prev node is siblings "deep right" child
      *ioNextNode = GetDeepLastChild(cSibling); 
      return NS_OK;
    }
  
    // else it's the parent
    *ioNextNode = parent;
  }
  else  // post-order
  {
    nsCOMPtr<nsIContent> cN = *ioNextNode;
    nsCOMPtr<nsIContent> cLastChild;
    PRInt32 numChildren;
  
    cN->ChildCount(numChildren);
  
    // if it has children then prev node is last child
    if (numChildren)
    {
      if (NS_FAILED(cN->ChildAt(--numChildren,*getter_AddRefs(cLastChild))))
        return NS_ERROR_FAILURE;
      if (!cLastChild)
        return NS_ERROR_FAILURE;
      *ioNextNode = cLastChild;
      return NS_OK;
    }
  
    // else prev sibling is previous
    return GetPrevSibling(cN, ioNextNode);
  }
  return NS_OK;
}

/******************************************************
 * ContentIterator routines
 ******************************************************/

nsresult nsContentIterator::First()
{
  if (!mFirst) 
    return NS_ERROR_FAILURE;
  mIsDone = PR_FALSE;
  if (mFirst == mCurNode) 
    return NS_OK;
  mCurNode = mFirst;
  return NS_OK;
}


nsresult nsContentIterator::Last()
{
  if (!mLast) 
    return NS_ERROR_FAILURE;
  mIsDone = PR_FALSE;
  if (mLast == mCurNode) 
    return NS_OK;
  mCurNode = mLast;
  return NS_OK;
}


nsresult nsContentIterator::Next()
{
  if (mIsDone) 
    return NS_OK;
  if (!mCurNode) 
    return NS_OK;
  if (mCurNode == mLast) 
  {
    mIsDone = PR_TRUE;
    return NS_OK;
  }
  
  return NextNode(&mCurNode);
}


nsresult nsContentIterator::Prev()
{
  if (mIsDone) 
    return NS_OK;
  if (!mCurNode) 
    return NS_OK;
  if (mCurNode == mFirst) 
  {
    mIsDone = PR_TRUE;
    return NS_OK;
  }
  
  return PrevNode(&mCurNode);
}


nsresult nsContentIterator::IsDone()
{
  if (mIsDone) 
    return NS_OK;
  else 
    return NS_ENUMERATOR_FALSE;
}


nsresult nsContentIterator::PositionAt(nsIContent* aCurNode)
{
  // XXX need to confirm that aCurNode is within range
  if (!aCurNode)
    return NS_ERROR_NULL_POINTER;
  mCurNode = do_QueryInterface(aCurNode);
  mIsDone = PR_FALSE;
  return NS_OK;
}

nsresult nsContentIterator::MakePre()
{
  // XXX need to confirm mCurNode is within range
  mPre = PR_TRUE;
  return NS_OK;
}

nsresult nsContentIterator::MakePost()
{
  // XXX need to confirm mCurNode is within range
  mPre = PR_FALSE;
  return NS_OK;
}


nsresult nsContentIterator::CurrentNode(nsIContent **aNode)
{
  if (!mCurNode) 
    return NS_ERROR_FAILURE;
  if (mIsDone) 
    return NS_ERROR_FAILURE;
  return mCurNode->QueryInterface(NS_GET_IID(nsIContent), (void**) aNode);
}





/*====================================================================================*/
/*====================================================================================*/






/******************************************************
 * nsContentSubtreeIterator
 ******************************************************/


/*
 *  A simple iterator class for traversing the content in "top subtree" order
 */
class nsContentSubtreeIterator : public nsContentIterator 
{
public:
  nsContentSubtreeIterator() {};
  virtual ~nsContentSubtreeIterator() {};

  // nsContentIterator overrides ------------------------------

  NS_IMETHOD Init(nsIContent* aRoot);

  NS_IMETHOD Init(nsIDOMRange* aRange);

  NS_IMETHOD Next();

  NS_IMETHOD Prev();

  NS_IMETHOD PositionAt(nsIContent* aCurNode);

  NS_IMETHOD MakePre();

  NS_IMETHOD MakePost();

protected:

  nsresult GetTopAncestorInRange( nsCOMPtr<nsIContent> aNode,
                                  nsCOMPtr<nsIContent> *outAnestor);
                                  
  // no copy's or assigns  FIX ME
  nsContentSubtreeIterator(const nsContentSubtreeIterator&);
  nsContentSubtreeIterator& operator=(const nsContentSubtreeIterator&);

  nsCOMPtr<nsIDOMRange> mRange;
};

nsresult NS_NewContentSubtreeIterator(nsIContentIterator** aInstancePtrResult);




/******************************************************
 * repository cruft
 ******************************************************/

nsresult NS_NewContentSubtreeIterator(nsIContentIterator** aInstancePtrResult)
{
  nsContentIterator * iter = new nsContentSubtreeIterator();
  if (iter)
    return iter->QueryInterface(NS_GET_IID(nsIContentIterator), (void**) aInstancePtrResult);
  return NS_ERROR_OUT_OF_MEMORY;
}



/******************************************************
 * Init routines
 ******************************************************/


nsresult nsContentSubtreeIterator::Init(nsIContent* aRoot)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


nsresult nsContentSubtreeIterator::Init(nsIDOMRange* aRange)
{
  if (!aRange) 
    return NS_ERROR_NULL_POINTER; 

  mIsDone = PR_FALSE;

  mRange = do_QueryInterface(aRange);
  
  // get the start node and offset, convert to nsIContent
  nsCOMPtr<nsIDOMNode> commonParent;
  nsCOMPtr<nsIDOMNode> startParent;
  nsCOMPtr<nsIDOMNode> endParent;
  nsCOMPtr<nsIContent> cStartP;
  nsCOMPtr<nsIContent> cEndP;
  nsCOMPtr<nsIContent> cN;
  nsCOMPtr<nsIContent> firstCandidate;
  nsCOMPtr<nsIContent> lastCandidate;
  nsCOMPtr<nsIDOMNode> dChild;
  nsCOMPtr<nsIContent> cChild;
  PRInt32 indx, startIndx, endIndx;
  PRInt32 numChildren;

  // get common content parent
  if (NS_FAILED(aRange->GetCommonParent(getter_AddRefs(commonParent))) || !commonParent)
    return NS_ERROR_FAILURE;
  mCommonParent = do_QueryInterface(commonParent);

  // get start content parent
  if (NS_FAILED(aRange->GetStartParent(getter_AddRefs(startParent))) || !startParent)
    return NS_ERROR_FAILURE;
  cStartP = do_QueryInterface(startParent);
  aRange->GetStartOffset(&startIndx);

  // get end content parent
  if (NS_FAILED(aRange->GetEndParent(getter_AddRefs(endParent))) || !endParent)
    return NS_ERROR_FAILURE;
  cEndP = do_QueryInterface(endParent);
  aRange->GetEndOffset(&endIndx);
  
  // short circuit when start node == end node
  if (startParent == endParent)
  {
    cStartP->ChildAt(0,*getter_AddRefs(cChild));
  
    if (!cChild) // no children, must be a text node or empty container
    {
      // all inside one text node - empty subtree iterator
      MakeEmpty();
      return NS_OK;
    }
    else
    {
      if (startIndx == endIndx)  // collapsed range
      {
        MakeEmpty();
        return NS_OK;
      }
    }
  }
  
  // find first node in range
  aRange->GetStartOffset(&indx);
  numChildren = GetNumChildren(startParent);
  
  if (!numChildren) // no children, must be a text node
  {
    cN = cStartP; 
  }
  else
  {
    dChild = GetChildAt(startParent, indx);
    cChild = do_QueryInterface(dChild);
    if (!cChild)  // offset after last child
    {
      cN = cStartP;
    }
    else
    {
      firstCandidate = cChild;
    }
  }
  
  if (!firstCandidate)
  {
    // then firstCandidate is next node after cN
    if (NS_FAILED(GetNextSibling(cN,  &firstCandidate)))
    {
      MakeEmpty();
      return NS_OK;
    }
  }
  
  firstCandidate = GetDeepFirstChild(firstCandidate);
  
  // confirm that this first possible contained node
  // is indeed contained.  Else we have a range that
  // does not fully contain any node.
  
  PRBool nodeBefore, nodeAfter;
  if (NS_FAILED(CompareNodeToRange(firstCandidate, aRange, &nodeBefore, &nodeAfter)))
    return NS_ERROR_FAILURE;
  if (nodeBefore || nodeAfter)
  {
    MakeEmpty();
    return NS_OK;
  }

  // cool, we have the first node in the range.  Now we walk
  // up it's ancestors to find the most senior that is still
  // in the range.  That's the real first node.
  if (NS_FAILED(GetTopAncestorInRange(firstCandidate, &mFirst)))
    return NS_ERROR_FAILURE;
  
  
  
  // now to find the last node
  aRange->GetEndOffset(&indx);
  numChildren = GetNumChildren(endParent);

  if (indx > numChildren) indx = numChildren;
  if (!indx)
  {
    cN = cEndP;
  }
  else
  {
    if (!numChildren) // no children, must be a text node
    {
      cN = cEndP; 
    }
    else
    {
      dChild = GetChildAt(endParent, --indx);
      cChild = do_QueryInterface(dChild);
      if (!cChild)  // shouldn't happen
      {
        NS_ASSERTION(0,"tree traversal trouble in nsContentSubtreeIterator::Init");
        return NS_ERROR_FAILURE;
      }
      else
      {
        lastCandidate = cChild;
      }
    }
  }
  
  if (!lastCandidate)
  {
    // then lastCandidate is prev node before cN
    if (NS_FAILED(GetPrevSibling(cN, &lastCandidate)))
    {
      MakeEmpty();
      return NS_OK;
    }
  }
  
  lastCandidate = GetDeepLastChild(lastCandidate);
  
  // confirm that this first possible contained node
  // is indeed contained.  Else we have a range that
  // does not fully contain any node.
  
  if (NS_FAILED(CompareNodeToRange(lastCandidate, aRange, &nodeBefore, &nodeAfter)))
    return NS_ERROR_FAILURE;
  if (nodeBefore || nodeAfter)
  {
    MakeEmpty();
    return NS_OK;
  }

  // cool, we have the last node in the range.  Now we walk
  // up it's ancestors to find the most senior that is still
  // in the range.  That's the real first node.
  if (NS_FAILED(GetTopAncestorInRange(lastCandidate, &mLast)))
    return NS_ERROR_FAILURE;
  
  mCurNode = mFirst;

  return NS_OK;
}


/****************************************************************
 * nsContentSubtreeIterator overrides of ContentIterator routines
 ****************************************************************/

nsresult nsContentSubtreeIterator::Next()
{
  if (mIsDone) 
    return NS_OK;
  if (!mCurNode) 
    return NS_OK;
  if (mCurNode == mLast) 
  {
    mIsDone = PR_TRUE;
    return NS_OK;
  }
  
  nsCOMPtr<nsIContent> nextNode;
  if (NS_FAILED(GetNextSibling(mCurNode, &nextNode)))
    return NS_OK;
  nextNode = GetDeepFirstChild(nextNode);
  return GetTopAncestorInRange(nextNode, &mCurNode);
}


nsresult nsContentSubtreeIterator::Prev()
{
  if (mIsDone) 
    return NS_OK;
  if (!mCurNode) 
    return NS_OK;
  if (mCurNode == mFirst) 
  {
    mIsDone = PR_TRUE;
    return NS_OK;
  }
  
  nsCOMPtr<nsIContent> prevNode;
  prevNode = GetDeepFirstChild(mCurNode);
  if (NS_FAILED(PrevNode(&prevNode)))
    return NS_OK;
  prevNode = GetDeepLastChild(prevNode);
  return GetTopAncestorInRange(prevNode, &mCurNode);
}

nsresult nsContentSubtreeIterator::PositionAt(nsIContent* aCurNode)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult nsContentSubtreeIterator::MakePre()
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult nsContentSubtreeIterator::MakePost()
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/****************************************************************
 * nsContentSubtreeIterator helper routines
 ****************************************************************/

nsresult nsContentSubtreeIterator::GetTopAncestorInRange(
                                       nsCOMPtr<nsIContent> aNode,
                                       nsCOMPtr<nsIContent> *outAnestor)
{
  if (!aNode) 
    return NS_ERROR_NULL_POINTER;
  if (!outAnestor) 
    return NS_ERROR_NULL_POINTER;
  
  
  // sanity check: aNode is itself in the range
  PRBool nodeBefore, nodeAfter;
  if (NS_FAILED(CompareNodeToRange(aNode, mRange, &nodeBefore, &nodeAfter)))
    return NS_ERROR_FAILURE;
  if (nodeBefore || nodeAfter)
    return NS_ERROR_FAILURE;
  
  nsCOMPtr<nsIContent> parent;
  while (aNode)
  {
    if (NS_FAILED(aNode->GetParent(*getter_AddRefs(parent))) || !parent)
      return NS_ERROR_FAILURE;
    if (NS_FAILED(CompareNodeToRange(parent, mRange, &nodeBefore, &nodeAfter)))
      return NS_ERROR_FAILURE;
    if (nodeBefore || nodeAfter)
    {
      *outAnestor = aNode;
      return NS_OK;
    }
    aNode = parent;
  }
  return NS_ERROR_FAILURE;
}

/******************************************************
 * nsGeneratedSubtreeIterator
 ******************************************************/

nsresult NS_NewGeneratedSubtreeIterator(nsIGeneratedContentIterator** aInstancePtrResult);

class nsGeneratedSubTreeIterator : public nsContentSubtreeIterator, nsIGeneratedContentIterator
{
public:
  NS_DECL_ISUPPORTS
  nsGeneratedSubTreeIterator();
  virtual ~nsGeneratedSubTreeIterator();

  // nsIGeneratedContentIterator interface methods ------------------------------

  NS_IMETHOD Init(nsIFocusTracker *aTracker, nsIContent* aRange, PRBool aSelectBefore, PRBool aSelectAfter);
  NS_IMETHOD First();

  NS_IMETHOD Last();
  
  NS_IMETHOD Next();

  NS_IMETHOD Prev();

  NS_IMETHOD CurrentNode(nsIContent **aNode);

protected:
  PRBool mSelectBefore;//set at init time
  PRBool mSelectAfter;
  nsCOMPtr<nsIContentIterator> mGenIter;//reset for every before and after
  nsIFocusTracker *mTracker;//weak reference

  nsresult FillGenIter(PRInt8 aSide);
  PRInt8 mCurSideOfContent; //-1= before, 0 = current, 1=next
};


nsGeneratedSubTreeIterator::nsGeneratedSubTreeIterator()
:mSelectBefore(0),mSelectAfter(0), mCurSideOfContent(-1)
{
}

nsGeneratedSubTreeIterator::~nsGeneratedSubTreeIterator()
{
}


NS_IMPL_ADDREF_INHERITED(nsGeneratedSubTreeIterator, nsContentIterator) 
NS_IMPL_RELEASE_INHERITED(nsGeneratedSubTreeIterator, nsContentIterator) 

NS_IMETHODIMP
nsGeneratedSubTreeIterator::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
  if (nsnull == aInstancePtr) {
    return NS_ERROR_NULL_POINTER;
  }
  if (aIID.Equals(nsIGeneratedContentIterator::GetIID())) {
    *aInstancePtr = NS_STATIC_CAST(nsIGeneratedContentIterator *,this);
    NS_ADDREF_THIS();
    return NS_OK;
  }
  return nsContentSubtreeIterator::QueryInterface(aIID,aInstancePtr);
}


/******************************************************
 * nsGeneratedSubTreeIterator
 ******************************************************/

nsresult nsGeneratedSubTreeIterator::Init(nsIFocusTracker *aTracker, nsIContent* aContent, PRBool aSelectBefore, PRBool aSelectAfter)
{
  mTracker = aTracker;
  mSelectBefore = aSelectBefore;
  mSelectAfter = aSelectAfter;
  return nsContentSubtreeIterator::Init(aContent);
}
  
  
nsresult nsGeneratedSubTreeIterator::First()
{
  nsContentSubtreeIterator::First();
  if (!mFirst) 
    return NS_ERROR_FAILURE;

  mIsDone = PR_FALSE;
  mGenIter = 0;
  if (mSelectBefore)
  {
    mCurSideOfContent = -1;//get generated from in FRONT of this node
    FillGenIter(mCurSideOfContent);
  }
  if (!mGenIter)
    mCurSideOfContent = 0;
  return NS_OK;
}

nsresult nsGeneratedSubTreeIterator::Next()
{
  if (mIsDone) 
    return NS_OK;
  if (!mCurNode) 
    return NS_OK;

  nsresult result = NS_OK;

  if (mGenIter)
  {
    result = mGenIter->Next();
    if (NS_FAILED(result) || (NS_SUCCEEDED(mGenIter->IsDone()) ))
    {
      mGenIter = 0;
    }
    else
      return result;
  }

  if (mCurNode == mLast && !mGenIter) 
  {
    mIsDone = PR_TRUE;
    return NS_OK;
  }

  mCurSideOfContent ++;
  if (mCurSideOfContent > 0)//current node currently
  {
    FillGenIter(mCurSideOfContent);
  }
  if (!mGenIter) //nothing after current node
  {
    mCurSideOfContent = -1;
    result = nsContentSubtreeIterator::NextNode(&mCurNode);
    FillGenIter(mCurSideOfContent);
    if (!mGenIter)//nothing before new node.
      mCurSideOfContent = 0;//advance to actual node
  }
  return result;
}

nsresult nsGeneratedSubTreeIterator::Prev()
{
  if (mIsDone) 
    return NS_OK;
  if (!mCurNode) 
    return NS_OK;

  nsresult result = NS_OK;

  if (mGenIter)
  {
    result = mGenIter->Prev();
    if (NS_FAILED(result) || (NS_SUCCEEDED(mGenIter->IsDone()) ))
    {
      mGenIter = 0;
    }
    else
      return result;
  }

  if (mCurNode == mFirst && !mGenIter) 
  {
    mIsDone = PR_TRUE;
    return NS_OK;
  }

  mCurSideOfContent --;
  if (mCurSideOfContent < 0)//current node currently
  {
    FillGenIter(mCurSideOfContent);
  }
  if (!mGenIter) //nothing after current node
  {
    mCurSideOfContent = 1;
    result = nsContentSubtreeIterator::PrevNode(&mCurNode);
    FillGenIter(mCurSideOfContent);
    if (!mGenIter)//nothing before new node.
      mCurSideOfContent = 0;//advance to actual node
  }
  return result;
}


nsresult nsGeneratedSubTreeIterator::Last()
{
  if (!mLast) 
    return NS_ERROR_FAILURE;
  nsContentSubtreeIterator::Last();
  mGenIter = 0;
  mIsDone = PR_FALSE;
  if (mSelectAfter)
  {
    mCurSideOfContent = 1;
    FillGenIter(mCurSideOfContent);
  }
  if (!mGenIter)
    mCurSideOfContent = 0;
  
  return NS_OK;
}

nsresult 
nsGeneratedSubTreeIterator::FillGenIter(PRInt8 aSide)
{
  if (!mCurNode || !mTracker)
    return NS_ERROR_NULL_POINTER;
  nsresult result;
  if (aSide == 0)
    return NS_OK;
  nsFrameState frameState;
  nsIFrame *genFrame;
  result = mTracker->GetPrimaryFrameFor(mCurNode, &genFrame);
  if (NS_FAILED(result))
    return result;
  if (!genFrame)
    return NS_ERROR_FAILURE;
  nsCOMPtr<nsIPresContext> context;
  result = mTracker->GetPresContext(getter_AddRefs(context));
  if (NS_FAILED(result) || !context)
    return result?result:NS_ERROR_FAILURE;
  result = genFrame->FirstChild(context,nsnull,&genFrame);
  if (NS_FAILED(result) || !genFrame)
    return NS_OK;//fine nothing to do here
  if (aSide < 0)
  {
    //we SHOULD now have a generated content frame if one exists. we need to check the flag for gen content
    result = genFrame->GetFrameState(&frameState);

    if (NS_FAILED(result))
      return result;

  }
  else if (aSide > 0)
  {
    nsIFrame *nextFrame = genFrame;
    while(nextFrame)
    {
      result = nextFrame->GetNextSibling(&nextFrame);
      if (NS_SUCCEEDED(result) && nextFrame)
        genFrame = nextFrame;
      else
        nextFrame = nsnull;
    }
  }
  if (frameState & NS_FRAME_GENERATED_CONTENT) {
    nsCOMPtr<nsIContent> content;
    result = genFrame->GetContent(getter_AddRefs(content));
    if (NS_FAILED(result) || !content)
      return result ? result:NS_ERROR_FAILURE;
    result = nsComponentManager::CreateInstance(kCSubtreeIteratorCID, nsnull,
                                            NS_GET_IID(nsIContentIterator), 
                                            getter_AddRefs(mGenIter));
    if (NS_FAILED(result) || !mGenIter)
      return result ? result:NS_ERROR_FAILURE;
    mGenIter->Init(content);
    if (aSide > 0)
      mGenIter->Last();
  }

    return NS_OK;
}

nsresult nsGeneratedSubTreeIterator::CurrentNode(nsIContent **aNode)
{
  if (!mCurNode) 
    return NS_ERROR_FAILURE;
  if (mIsDone) 
    return NS_ERROR_FAILURE;
  if (mGenIter)
    return mGenIter->CurrentNode(aNode);
  return mCurNode->QueryInterface(nsIContent::GetIID(), (void**) aNode);
}


/******************************************************
 * nsGeneratedContentIterator
 ******************************************************/

nsresult nsGeneratedContentIterator::Init(nsIFocusTracker *aTracker, nsIDOMRange* aRange, PRBool aSelectBefore, PRBool aSelectAfter)
{
  mTracker = aTracker;
  mSelectBefore = aSelectBefore;
  mSelectAfter = aSelectAfter;
  return nsContentIterator::Init(aRange);
}
  
  
nsresult nsGeneratedContentIterator::First()
{
  nsContentIterator::First();
  if (!mFirst) 
    return NS_ERROR_FAILURE;
  mIsDone = PR_FALSE;
  mGenIter = 0;
  if (mSelectBefore)
  {
    mCurSideOfContent = -1;//get generated from in FRONT of this node
    FillGenIter(mCurSideOfContent);
  }
  if (!mGenIter)
    mCurSideOfContent = 0;
  return NS_OK;
}

nsresult nsGeneratedContentIterator::Next()
{
  if (mIsDone) 
    return NS_OK;
  if (!mCurNode) 
    return NS_OK;

  nsresult result = NS_OK;

  if (mGenIter)
  {
    result = mGenIter->Next();
    if (NS_FAILED(result) || (NS_SUCCEEDED(mGenIter->IsDone()) ))
    {
      mGenIter = 0;
    }
    else
      return result;
  }

  if (mCurNode == mLast && !mGenIter) 
  {
    mIsDone = PR_TRUE;
    return NS_OK;
  }

  mCurSideOfContent ++;
  if (mCurSideOfContent > 0)//current node currently
  {
    FillGenIter(mCurSideOfContent);
  }
  if (!mGenIter) //nothing after current node
  {
    mCurSideOfContent = -1;
    result = nsContentIterator::NextNode(&mCurNode);
    FillGenIter(mCurSideOfContent);
    if (!mGenIter)//nothing before new node.
      mCurSideOfContent = 0;//advance to actual node
  }
  return result;
}

nsresult nsGeneratedContentIterator::Prev()
{
  if (mIsDone) 
    return NS_OK;
  if (!mCurNode) 
    return NS_OK;

  nsresult result = NS_OK;

  if (mGenIter)
  {
    result = mGenIter->Prev();
    if (NS_FAILED(result) || (NS_SUCCEEDED(mGenIter->IsDone()) ))
    {
      mGenIter = 0;
    }
    else
      return result;
  }

  if (mCurNode == mFirst && !mGenIter) 
  {
    mIsDone = PR_TRUE;
    return NS_OK;
  }

  mCurSideOfContent --;
  if (mCurSideOfContent < 0)//current node currently
  {
    FillGenIter(mCurSideOfContent);
  }
  if (!mGenIter) //nothing after current node
  {
    mCurSideOfContent = 1;
    result = nsContentIterator::PrevNode(&mCurNode);
    FillGenIter(mCurSideOfContent);
    if (!mGenIter)//nothing before new node.
      mCurSideOfContent = 0;//advance to actual node
  }
  return result;
}


nsresult nsGeneratedContentIterator::Last()
{
  nsContentIterator::Last();
  if (!mLast) 
    return NS_ERROR_FAILURE;
  mGenIter = 0;
  mIsDone = PR_FALSE;
  mCurNode = mLast;
  mCurSideOfContent = 1;
  FillGenIter(mCurSideOfContent);
  if (!mGenIter)
    mCurSideOfContent = 0;
  
  return NS_OK;
}

nsresult 
nsGeneratedContentIterator::FillGenIter(PRInt8 aSide)
{
  if (!mCurNode || !mTracker)
    return NS_ERROR_NULL_POINTER;
  nsresult result;
  if (aSide == 0)
    return NS_OK;
  nsFrameState frameState;
  nsIFrame *genFrame;
  result = mTracker->GetPrimaryFrameFor(mCurNode, &genFrame);
  if (NS_FAILED(result))
    return result;
  if (!genFrame)
    return NS_ERROR_FAILURE;
  nsCOMPtr<nsIPresContext> context;
  result = mTracker->GetPresContext(getter_AddRefs(context));
  if (NS_FAILED(result) || !context)
    return result?result:NS_ERROR_FAILURE;
  result = genFrame->FirstChild(context,nsnull,&genFrame);
  if (NS_FAILED(result) || !genFrame)
    return NS_OK;//fine nothing to do here
  if (aSide < 0)
  {
    //we SHOULD now have a generated content frame if one exists. we need to check the flag for gen content
    result = genFrame->GetFrameState(&frameState);

    if (NS_FAILED(result))
      return result;

  }
  else if (aSide > 0)
  {
    nsIFrame *nextFrame = genFrame;
    while(nextFrame)
    {
      result = nextFrame->GetNextSibling(&nextFrame);
      if (NS_SUCCEEDED(result) && nextFrame)
        genFrame = nextFrame;
      else
        nextFrame = nsnull;
    }
  }
  if (frameState & NS_FRAME_GENERATED_CONTENT) {
    nsCOMPtr<nsIContent> content;
    result = genFrame->GetContent(getter_AddRefs(content));
    if (NS_FAILED(result) || !content)
      return result ? result:NS_ERROR_FAILURE;
    result = nsComponentManager::CreateInstance(kCSubtreeIteratorCID, nsnull,
                                            NS_GET_IID(nsIContentIterator), 
                                            getter_AddRefs(mGenIter));
    if (NS_FAILED(result) || !mGenIter)
      return result ? result:NS_ERROR_FAILURE;
    mGenIter->Init(content);
    if (aSide > 0)
      mGenIter->Last();
  }

    return NS_OK;
}

nsresult nsGeneratedContentIterator::CurrentNode(nsIContent **aNode)
{
  if (!mCurNode) 
    return NS_ERROR_FAILURE;
  if (mIsDone) 
    return NS_ERROR_FAILURE;
  if (mGenIter)
    return mGenIter->CurrentNode(aNode);
  return mCurNode->QueryInterface(nsIContent::GetIID(), (void**) aNode);
}

