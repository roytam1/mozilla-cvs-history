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

#include "SplitElementTxn.h"
#include "nsIDOMNode.h"
#include "nsIDOMSelection.h"
#include "nsIEditorSupport.h"

static NS_DEFINE_IID(kIEditorSupportIID,    NS_IEDITORSUPPORT_IID);

// note that aEditor is not refcounted
SplitElementTxn::SplitElementTxn()
  : EditTxn()
{
}

NS_IMETHODIMP SplitElementTxn::Init(nsIEditor  *aEditor,
                                    nsIDOMNode *aNode,
                                    PRInt32     aOffset)
{
  mEditor = do_QueryInterface(aEditor);
  mExistingRightNode = do_QueryInterface(aNode);
  mOffset = aOffset;
  return NS_OK;
}

SplitElementTxn::~SplitElementTxn()
{
}

NS_IMETHODIMP SplitElementTxn::Do(void)
{
  NS_ASSERTION(mExistingRightNode, "bad state");
  if (!mExistingRightNode) {
    return NS_ERROR_NOT_INITIALIZED;
  }
  // create a new node
  nsresult result = mExistingRightNode->CloneNode(PR_FALSE, getter_AddRefs(mNewLeftNode));
  NS_ASSERTION(((NS_SUCCEEDED(result)) && (mNewLeftNode)), "could not create element.");

  if ((NS_SUCCEEDED(result)) && (mNewLeftNode))
  {
    // get the parent node
    result = mExistingRightNode->GetParentNode(getter_AddRefs(mParent));
    // insert the new node
    if ((NS_SUCCEEDED(result)) && (mParent))
    {
      nsCOMPtr<nsIEditorSupport> editor;
      result = mEditor->QueryInterface(kIEditorSupportIID, getter_AddRefs(editor));
      if (NS_SUCCEEDED(result) && editor) 
      {
        result = editor->SplitNodeImpl(mExistingRightNode, mOffset, mNewLeftNode, mParent);
        if (NS_SUCCEEDED(result) && mNewLeftNode)
        {
          nsCOMPtr<nsIDOMSelection>selection;
          mEditor->GetSelection(getter_AddRefs(selection));
          if (selection)
          {
            selection->Collapse(mNewLeftNode, mOffset);
          }
        }
      }
      else {
        result = NS_ERROR_NOT_IMPLEMENTED;
      }
    }
  }
  return result;
}

NS_IMETHODIMP SplitElementTxn::Undo(void)
{
  NS_ASSERTION(mExistingRightNode && mNewLeftNode && mParent, "bad state");
  if (!mExistingRightNode || !mNewLeftNode || !mParent) {
    return NS_ERROR_NOT_INITIALIZED;
  }

#ifdef NS_DEBUG
  // sanity check
  nsCOMPtr<nsIDOMNode>parent;
  nsresult debugResult = mExistingRightNode->GetParentNode(getter_AddRefs(parent));
  NS_ASSERTION((NS_SUCCEEDED(debugResult)), "bad GetParentNode result for right child");
  NS_ASSERTION(parent, "bad GetParentNode for right child");
  NS_ASSERTION(parent==mParent, "bad GetParentNode for right child, parents don't match");
  
  debugResult = mNewLeftNode->GetParentNode(getter_AddRefs(parent));
  NS_ASSERTION((NS_SUCCEEDED(debugResult)), "bad GetParentNode result for left child");
  NS_ASSERTION(parent, "bad GetParentNode for left child");
  NS_ASSERTION(parent==mParent, "bad GetParentNode for right child, left don't match");

#endif

  // this assumes Do inserted the new node in front of the prior existing node
  nsresult result;
  nsCOMPtr<nsIEditorSupport> editor;
  result = mEditor->QueryInterface(kIEditorSupportIID, getter_AddRefs(editor));
  if (NS_SUCCEEDED(result) && editor) 
  {
    result = editor->JoinNodesImpl(mExistingRightNode, mNewLeftNode, mParent, PR_FALSE);
    if (NS_SUCCEEDED(result) && mNewLeftNode)
    {
      nsCOMPtr<nsIDOMSelection>selection;
      mEditor->GetSelection(getter_AddRefs(selection));
      if (selection)
      {
        selection->Collapse(mExistingRightNode, mOffset);
      }
    }
  }
  else {
    result = NS_ERROR_NOT_IMPLEMENTED;
  }
  return result;
}

/*
NS_IMETHODIMP SplitElementTxn::Redo(void)
{
  NS_ASSERTION(mExistingRightNode && mNewLeftNode && mParent, "bad state");
  if (!mExistingRightNode || !mNewLeftNode || !mParent) {
    return NS_ERROR_NOT_INITIALIZED;
  }
  nsresult result;
  nsCOMPtr<nsIEditorSupport> editor;
  result = mEditor->QueryInterface(kIEditorSupportIID, getter_AddRefs(editor));
  if (NS_SUCCEEDED(result) && editor) {
    result = editor->SplitNodeImpl(mExistingRightNode, mOffset, mNewLeftNode, mParent);
  }
  else {
    result = NS_ERROR_NOT_IMPLEMENTED;
  }
  return result;
}
*/

NS_IMETHODIMP SplitElementTxn::Merge(PRBool *aDidMerge, nsITransaction *aTransaction)
{
  if (nsnull!=aDidMerge)
    *aDidMerge=PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP SplitElementTxn::Write(nsIOutputStream *aOutputStream)
{
  return NS_OK;
}

NS_IMETHODIMP SplitElementTxn::GetUndoString(nsString **aString)
{
  if (nsnull!=aString)
  {
    **aString="Join Element";
  }
  return NS_OK;
}

NS_IMETHODIMP SplitElementTxn::GetRedoString(nsString **aString)
{
  if (nsnull!=aString)
  {
    **aString="Split Element";
  }
  return NS_OK;
}

NS_IMETHODIMP SplitElementTxn::GetNewNode(nsIDOMNode **aNewNode)
{
  if (!aNewNode)
    return NS_ERROR_NULL_POINTER;
  if (!mNewLeftNode)
    return NS_ERROR_NOT_INITIALIZED;
  *aNewNode = mNewLeftNode;
  NS_ADDREF(*aNewNode);
  return NS_OK;
}
