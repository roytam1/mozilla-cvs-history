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

#include "JoinElementTxn.h"
#include "nsIDOMNodeList.h"
#include "nsIEditorSupport.h"

static NS_DEFINE_IID(kIEditorSupportIID,    NS_IEDITORSUPPORT_IID);


JoinElementTxn::JoinElementTxn()
  : EditTxn()
{
}

nsresult JoinElementTxn::Init(nsIEditor  *aEditor,
                              nsIDOMNode *aLeftNode,
                              nsIDOMNode *aRightNode)
{
  mEditor = aEditor;
  mLeftNode = aLeftNode;
  mRightNode = aRightNode;
  mOffset=0;
  return NS_OK;
}

JoinElementTxn::~JoinElementTxn()
{
}

nsresult JoinElementTxn::Do(void)
{
  nsresult result;

  if ((mLeftNode) && (mRightNode))
  { // get the parent node
    nsCOMPtr<nsIDOMNode>leftParent;
    result = mLeftNode->GetParentNode(getter_AddRefs(leftParent));
    if ((NS_SUCCEEDED(result)) && (leftParent))
    { // verify that mLeftNode and mRightNode have the same parent
      nsCOMPtr<nsIDOMNode>rightParent;
      result = mRightNode->GetParentNode(getter_AddRefs(rightParent));
      if ((NS_SUCCEEDED(result)) && (rightParent))
      {
        if (leftParent==rightParent)
        {
          mParent=leftParent; // set this instance mParent. 
                              // Other methods see a non-null mParent and know all is well
          nsCOMPtr<nsIDOMNodeList> childNodes;
          result = mLeftNode->GetChildNodes(getter_AddRefs(childNodes));
          if ((NS_SUCCEEDED(result)) && (childNodes))
          {
            childNodes->GetLength(&mOffset);
          }
          //XXX: WRONG! needs commented code below
/*
  result = mEditor->QueryInterface(kIEditorSupportIID, getter_AddRefs(editor));
  if (NS_SUCCEEDED(result) && editor) {
    result = editor->JoinNodesImpl(mLeftNode, mRightNode, mParent, PR_FALSE);
*/
          result = mEditor->JoinNodes(mLeftNode, mRightNode, mParent, PR_FALSE);
        }
      }
    }
  }
  return result;
}


nsresult JoinElementTxn::Undo(void)
{
  nsresult result;
  nsCOMPtr<nsIEditorSupport> editor;
  result = mEditor->QueryInterface(kIEditorSupportIID, getter_AddRefs(editor));
  if (NS_SUCCEEDED(result) && editor) {
    result = editor->SplitNodeImpl(mRightNode, mOffset, mLeftNode, mParent);
  }
  else {
    result = NS_ERROR_NOT_IMPLEMENTED;
  }
  return result;
}

nsresult JoinElementTxn::Redo(void)
{
  nsresult result = mEditor->JoinNodes(mLeftNode, mRightNode, mParent, PR_FALSE);
  return result;
}

nsresult JoinElementTxn::GetIsTransient(PRBool *aIsTransient)
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

nsresult JoinElementTxn::Write(nsIOutputStream *aOutputStream)
{
  return NS_OK;
}

nsresult JoinElementTxn::GetUndoString(nsString **aString)
{
  if (nsnull!=aString)
  {
    **aString="Join Element";
  }
  return NS_OK;
}

nsresult JoinElementTxn::GetRedoString(nsString **aString)
{
  if (nsnull!=aString)
  {
    **aString="Split Element";
  }
  return NS_OK;
}
