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

#include "InsertElementTxn.h"
#include "nsIDOMSelection.h"
#include "nsIContent.h"

#ifdef NS_DEBUG
static PRBool gNoisy = PR_FALSE;
#else
static const PRBool gNoisy = PR_FALSE;
#endif


InsertElementTxn::InsertElementTxn()
  : EditTxn()
{
  SetTransactionDescriptionID( kTransactionID );
  /* log description initialized in parent constructor */
}

NS_IMETHODIMP InsertElementTxn::Init(nsIDOMNode *aNode,
                                     nsIDOMNode *aParent,
                                     PRInt32     aOffset,
                                     nsIEditor  *aEditor)
{
  NS_ASSERTION(aNode && aParent && aEditor, "bad arg");
  if (!aNode || !aParent || !aEditor)
    return NS_ERROR_NULL_POINTER;

  mNode = do_QueryInterface(aNode);
  mParent = do_QueryInterface(aParent);
  mOffset = aOffset;
  mEditor = aEditor;
  if (!mNode || !mParent || !mEditor)
    return NS_ERROR_INVALID_ARG;
  return NS_OK;
}


InsertElementTxn::~InsertElementTxn()
{
}

NS_IMETHODIMP InsertElementTxn::Do(void)
{
  if (gNoisy) 
  { 
    nsCOMPtr<nsIContent>nodeAsContent = do_QueryInterface(mNode);
    nsCOMPtr<nsIContent>parentAsContent = do_QueryInterface(mParent);
    nsString namestr;
    mNode->GetNodeName(namestr);
    char* nodename = namestr.ToNewCString();
    printf("%p Do Insert Element of %p <%s> into parent %p at offset %d\n", 
           this, nodeAsContent.get(), nodename,
           parentAsContent.get(), mOffset); 
    nsAllocator::Free(nodename);
  }

  if (!mNode || !mParent) return NS_ERROR_NOT_INITIALIZED;

  nsresult result;
  nsCOMPtr<nsIDOMNode>refNode;
  //if (0!=mOffset)
  { // get a ref node
    PRInt32 i=0;
    result = mParent->GetFirstChild(getter_AddRefs(refNode));
    if (NS_SUCCEEDED(result) && refNode)
    {
      for (; i<mOffset; i++)
      {
        nsCOMPtr<nsIDOMNode>nextSib;
        result = refNode->GetNextSibling(getter_AddRefs(nextSib));  
        if (NS_FAILED(result)) {
          break;  // couldn't get a next sibling, so make aNode the first child
        }
        refNode = do_QueryInterface(nextSib);
        if (!refNode) {
          break;  // couldn't get a next sibling, so make aNode the first child
        }
      }
    }
  }

  nsCOMPtr<nsIDOMNode> resultNode;
  result = mParent->InsertBefore(mNode, refNode, getter_AddRefs(resultNode));
  if (NS_FAILED(result)) return result;
  if (!resultNode) return NS_ERROR_NULL_POINTER;

  // Try to insert formatting whitespace for the new node:
  mEditor->InsertFormattingForNode(resultNode);

  // only set selection to insertion point if editor gives permission
  PRBool bAdjustSelection;
  mEditor->ShouldTxnSetSelection(&bAdjustSelection);
  if (bAdjustSelection)
  {
    nsCOMPtr<nsIDOMSelection> selection;
    result = mEditor->GetSelection(getter_AddRefs(selection));
    if (NS_FAILED(result)) return result;
    if (!selection) return NS_ERROR_NULL_POINTER;
    // place the selection just after the inserted element
    selection->Collapse(mParent, mOffset+1);
  }
  else
  {
    // do nothing - dom range gravity will adjust selection
  }
  return result;
}

NS_IMETHODIMP InsertElementTxn::Undo(void)
{
  if (gNoisy) { printf("%p Undo Insert Element of %p into parent %p at offset %d\n", 
                       this, mNode.get(), mParent.get(), mOffset); }
  if (!mNode || !mParent) return NS_ERROR_NOT_INITIALIZED;

  nsCOMPtr<nsIDOMNode> resultNode;
  nsresult result = mParent->RemoveChild(mNode, getter_AddRefs(resultNode));
  return result;
}

NS_IMETHODIMP InsertElementTxn::Merge(PRBool *aDidMerge, nsITransaction *aTransaction)
{
  if (nsnull!=aDidMerge)
    *aDidMerge=PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP InsertElementTxn::Write(nsIOutputStream *aOutputStream)
{
  return NS_OK;
}

NS_IMETHODIMP InsertElementTxn::GetUndoString(nsString *aString)
{
  if (nsnull!=aString)
  {
    *aString="Remove Element: ";
  }
  return NS_OK;
}

NS_IMETHODIMP InsertElementTxn::GetRedoString(nsString *aString)
{
  if (nsnull!=aString)
  {
    *aString="Insert Element: ";
  }
  return NS_OK;
}
