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
 * Copyright (C) 1998-1999 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

#include "JoinElementTxn.h"
#include "nsEditor.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMCharacterData.h"
#include "nslog.h"

NS_IMPL_LOG(JoinElementTxnLog)
#define PRINTF(args) NS_LOG_PRINTF(JoinElementTxnLog, args)
#define FLUSH()      NS_LOG_FLUSH(JoinElementTxnLog)

JoinElementTxn::JoinElementTxn()
  : EditTxn()
{
  SetTransactionDescriptionID( kTransactionID );
  /* log description initialized in parent constructor */
}

NS_IMETHODIMP JoinElementTxn::Init(nsEditor   *aEditor,
                                   nsIDOMNode *aLeftNode,
                                   nsIDOMNode *aRightNode)
{
  NS_PRECONDITION((aEditor && aLeftNode && aRightNode), "null arg");
  if (!aEditor || !aLeftNode || !aRightNode) { return NS_ERROR_NULL_POINTER; }
  mEditor = aEditor;
  mLeftNode = do_QueryInterface(aLeftNode);
  mRightNode = do_QueryInterface(aRightNode);
  mOffset=0;
  return NS_OK;
}

JoinElementTxn::~JoinElementTxn()
{
}

// After Do() and Redo(), the left node is removed from the content tree and right node remains.
NS_IMETHODIMP JoinElementTxn::Do(void)
{
  PRINTF(("%p Do Join of %p and %p\n", this, mLeftNode.get(), mRightNode.get()));
  NS_PRECONDITION((mEditor && mLeftNode && mRightNode), "null arg");
  if (!mEditor || !mLeftNode || !mRightNode) { return NS_ERROR_NOT_INITIALIZED; }

  // get the parent node
  nsCOMPtr<nsIDOMNode>leftParent;
  nsresult result = mLeftNode->GetParentNode(getter_AddRefs(leftParent));
  if (NS_FAILED(result)) return result;
  if (!leftParent) return NS_ERROR_NULL_POINTER;

  // verify that mLeftNode and mRightNode have the same parent
  nsCOMPtr<nsIDOMNode>rightParent;
  result = mRightNode->GetParentNode(getter_AddRefs(rightParent));
  if (NS_FAILED(result)) return result;
  if (!rightParent) return NS_ERROR_NULL_POINTER;

  if (leftParent==rightParent)
  {
    mParent= do_QueryInterface(leftParent); // set this instance mParent. 
                                            // Other methods will see a non-null mParent and know all is well
    nsCOMPtr<nsIDOMCharacterData> leftNodeAsText;
    leftNodeAsText = do_QueryInterface(mLeftNode);
    if (leftNodeAsText) 
    {
      leftNodeAsText->GetLength(&mOffset);
    }
    else 
    {
      nsCOMPtr<nsIDOMNodeList> childNodes;
      result = mLeftNode->GetChildNodes(getter_AddRefs(childNodes));
      if (NS_FAILED(result)) return result;
      if (childNodes) 
      {
        childNodes->GetLength(&mOffset);
      }
    }
    result = mEditor->JoinNodesImpl(mRightNode, mLeftNode, mParent, PR_FALSE);
    if (NS_SUCCEEDED(result))
    {
      PRINTF(("  left node = %p removed\n", mLeftNode.get()));
    }
  }
  else 
  {
    NS_ASSERTION(PR_FALSE, "2 nodes do not have same parent");
    return NS_ERROR_INVALID_ARG;
  }
  return result;
}

//XXX: what if instead of split, we just deleted the unneeded children of mRight
//     and re-inserted mLeft?
NS_IMETHODIMP JoinElementTxn::Undo(void)
{
  PRINTF(("%p Undo Join, right node = %p\n", this, mRightNode.get()));
  NS_ASSERTION(mRightNode && mLeftNode && mParent, "bad state");
  if (!mRightNode || !mLeftNode || !mParent) { return NS_ERROR_NOT_INITIALIZED; }
  nsresult result;
  nsCOMPtr<nsIDOMNode>resultNode;
  // first, massage the existing node so it is in its post-split state
  nsCOMPtr<nsIDOMCharacterData>rightNodeAsText;
  rightNodeAsText = do_QueryInterface(mRightNode);
  if (rightNodeAsText)
  {
    result = rightNodeAsText->DeleteData(0, mOffset);
  }
  else
  {
    nsCOMPtr<nsIDOMNode>child;
    nsCOMPtr<nsIDOMNode>nextSibling;
    result = mRightNode->GetFirstChild(getter_AddRefs(child));
    PRUint32 i;
    for (i=0; i<mOffset; i++)
    {
      if (NS_FAILED(result)) {return result;}
      if (!child) {return NS_ERROR_NULL_POINTER;}
      child->GetNextSibling(getter_AddRefs(nextSibling));
      result = mLeftNode->AppendChild(child, getter_AddRefs(resultNode));
      child = do_QueryInterface(nextSibling);
    }
  }
  // second, re-insert the left node into the tree 
  result = mParent->InsertBefore(mLeftNode, mRightNode, getter_AddRefs(resultNode));
  return result;

}

NS_IMETHODIMP JoinElementTxn::GetIsTransient(PRBool *aIsTransient)
{
  if (nsnull!=aIsTransient)
    *aIsTransient = PR_FALSE;
  return NS_OK;
}

nsresult JoinElementTxn::Merge(PRBool *aDidMerge, nsITransaction *aTransaction)
{
  if (nsnull!=aDidMerge)
    *aDidMerge=PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP JoinElementTxn::Write(nsIOutputStream *aOutputStream)
{
  return NS_OK;
}

NS_IMETHODIMP JoinElementTxn::GetUndoString(nsString *aString)
{
  if (nsnull!=aString)
  {
    aString->AssignWithConversion("Join Element");
  }
  return NS_OK;
}

NS_IMETHODIMP JoinElementTxn::GetRedoString(nsString *aString)
{
  if (nsnull!=aString)
  {
    aString->AssignWithConversion("Split Element");
  }
  return NS_OK;
}
