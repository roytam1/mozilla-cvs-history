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
 *   Pierre Phaneuf <pp@ludusdesign.com>
 */

#include "SetDocTitleTxn.h"
#include "nsEditor.h"
#include "nsHTMLEditor.h"
#include "nsIDOMNode.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMDocument.h"
#include "nsIDOMHTMLDocument.h"
#include "nsIDOMText.h"
#include "nsIDOMElement.h"

// note that aEditor is not refcounted
SetDocTitleTxn::SetDocTitleTxn()
  : EditTxn()
, mIsTransient(PR_FALSE)
{
  SetTransactionDescriptionID( kTransactionID );
  /* log description initialized in parent constructor */
}

NS_IMETHODIMP SetDocTitleTxn::Init(nsIHTMLEditor *aEditor,
                                   const nsString *aValue)

{
  NS_ASSERTION(aEditor && aValue, "null args");
  if (!aEditor || !aValue) { return NS_ERROR_NULL_POINTER; }

  mEditor = aEditor;
  mValue = *aValue;

  return NS_OK;
}

SetDocTitleTxn::~SetDocTitleTxn()
{
}

NS_IMETHODIMP SetDocTitleTxn::Do(void)
{
  nsresult res = SetDomTitle(mValue);
  if (NS_FAILED(res)) return res;

  return SetDocTitle(mValue);
}

NS_IMETHODIMP SetDocTitleTxn::Undo(void)
{
  return SetDocTitle(mUndoValue);
}

NS_IMETHODIMP SetDocTitleTxn::Redo(void)
{
  return SetDocTitle(mValue);
}

nsresult SetDocTitleTxn::SetDocTitle(nsString& aTitle)
{
  NS_ASSERTION(mEditor, "bad state");
  if (!mEditor) return NS_ERROR_NOT_INITIALIZED;

  nsCOMPtr<nsIDOMDocument>  domDoc;
  nsCOMPtr<nsIEditor> editor = do_QueryInterface(mEditor);
  if (!editor) return NS_ERROR_FAILURE;
  nsresult res = editor->GetDocument(getter_AddRefs(domDoc));
  if (!domDoc) return NS_ERROR_FAILURE;
  nsCOMPtr<nsIDOMHTMLDocument> HTMLDoc = do_QueryInterface(domDoc);
  if (!HTMLDoc) return NS_ERROR_FAILURE;

  return HTMLDoc->SetTitle(aTitle);
}

nsresult SetDocTitleTxn::SetDomTitle(nsString& aTitle)
{
  nsCOMPtr<nsIDOMDocument>  domDoc;
  nsCOMPtr<nsIEditor> editor = do_QueryInterface(mEditor);
  if (!editor) return NS_ERROR_FAILURE;
  nsresult res = editor->GetDocument(getter_AddRefs(domDoc));
  
  if (!domDoc) return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDOMNodeList> titleList;
  nsCOMPtr<nsIDOMNode>titleNode;
  nsCOMPtr<nsIDOMNode>headNode;
  nsCOMPtr<nsIDOMNode> resultNode;
  res = domDoc->GetElementsByTagName(NS_LITERAL_STRING("title"), getter_AddRefs(titleList));
  if (NS_FAILED(res)) return res;

  // First assume we will NOT really do anything
  // (transaction will not be pushed on stack)
  mIsTransient = PR_TRUE;

  if(titleList)
  {
    res = titleList->Item(0, getter_AddRefs(titleNode));
    if (NS_FAILED(res)) return res;
    if (titleNode)
    {
      // Delete existing child textnode of title node
      // (Note: all contents under a TITLE node are always in a single text node)
      nsCOMPtr<nsIDOMNode> child;
      res = titleNode->GetFirstChild(getter_AddRefs(child));
      if(NS_FAILED(res)) return res;
      if(child)
      {
        // Save current text as the undo value
        nsCOMPtr<nsIDOMCharacterData> textNode = do_QueryInterface(child);
        if(textNode)
        {
          textNode->GetData(mUndoValue);

          // If title text is identical to what already exists,
          // quit now (mIsTransient is now TRUE)
          if (mUndoValue == aTitle)
            return NS_OK;
        }
        res = editor->DeleteNode(child);
        if(NS_FAILED(res)) return res;
      }
    }
  }

  // We didn't return above, thus we really will be changing the title
  mIsTransient = PR_FALSE;

  // Get the <HEAD> node, create a <TITLE> and insert it under the HEAD
  nsCOMPtr<nsIDOMNodeList> headList;
  res = domDoc->GetElementsByTagName(NS_LITERAL_STRING("head"),getter_AddRefs(headList));
  if (NS_FAILED(res)) return res;
  if (!headList) return NS_ERROR_FAILURE;
  
  headList->Item(0, getter_AddRefs(headNode));
  if (!headNode) return NS_ERROR_FAILURE;

  PRBool   newTitleNode = PR_FALSE;
  PRUint32 newTitleIndex = 0;

  if (!titleNode)
  {
    // Didn't find one above: Create a new one
    nsCOMPtr<nsIDOMElement>titleElement;
    res = domDoc->CreateElement(NS_LITERAL_STRING("title"), getter_AddRefs(titleElement));
    if (NS_FAILED(res)) return res;
    if (!titleElement) return NS_ERROR_FAILURE;

    titleNode = do_QueryInterface(titleElement);
    newTitleNode = PR_TRUE;

    // Get index so we append new title node 
    // after all existing HEAD children
    nsCOMPtr<nsIDOMNodeList> children;
    res = headNode->GetChildNodes(getter_AddRefs(children));
    if (NS_FAILED(res)) return res;
    if (children)
      children->GetLength(&newTitleIndex);
  }

  // Append a text node under the TITLE
  //  only if the title text isn't empty
  if (titleNode && aTitle.Length() > 0)
  {
    nsCOMPtr<nsIDOMText> textNode;
    res = domDoc->CreateTextNode(aTitle, getter_AddRefs(textNode));
    if (NS_FAILED(res)) return res;
    if (!textNode) return NS_ERROR_FAILURE;
    nsCOMPtr<nsIDOMNode> newNode = do_QueryInterface(textNode);
    if (!newNode) return NS_ERROR_FAILURE;

    if (newTitleNode)
    {
      // Not undoable: We will insert newTitleNode below
      res = titleNode->AppendChild(newNode, getter_AddRefs(resultNode));
    } 
    else 
    {
      // This is an undoable transaction
      res = editor->InsertNode(newNode, titleNode, 0);
    }
    if (NS_FAILED(res)) return res;
  }

  if (newTitleNode)
  {
    // Undoable transaction to insert title+text together
    res = editor->InsertNode(titleNode, headNode, newTitleIndex);
  }
  return res;
}

NS_IMETHODIMP SetDocTitleTxn::Merge(PRBool *aDidMerge, nsITransaction *aTransaction)
{
  if (nsnull!=aDidMerge)
    *aDidMerge=PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP SetDocTitleTxn::Write(nsIOutputStream *aOutputStream)
{
  return NS_OK;
}

NS_IMETHODIMP SetDocTitleTxn::GetUndoString(nsString *aString)
{
  if (nsnull!=aString)
  {
    aString->AssignWithConversion("Restore Document Title: ");
    *aString += mUndoValue;
  }
  return NS_OK;
}

NS_IMETHODIMP SetDocTitleTxn::GetRedoString(nsString *aString)
{
  if (nsnull!=aString)
  {
    aString->AssignWithConversion("Set Document Title: ");
    *aString += mValue;
  }
  return NS_OK;
}

NS_IMETHODIMP SetDocTitleTxn::GetIsTransient(PRBool *aIsTransient)
{
  if (!aIsTransient) return NS_ERROR_NULL_POINTER;  
  *aIsTransient = mIsTransient;
  return NS_OK;
}
