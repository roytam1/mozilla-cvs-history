/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL") you may not use this file except in
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

#include "nsTextEditRules.h"
#include "nsTextEditor.h"
#include "nsEditor.h"
#include "PlaceholderTxn.h"
#include "InsertTextTxn.h"
#include "nsCOMPtr.h"
#include "nsIDOMNode.h"
#include "nsIDOMElement.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMSelection.h"
#include "nsIDOMRange.h"
#include "nsIDOMCharacterData.h"
#include "nsIEnumerator.h"
#include "nsIContent.h"
#include "nsIEditProperty.h"

static NS_DEFINE_IID(kPlaceholderTxnIID,  PLACEHOLDER_TXN_IID);

#define CANCEL_OPERATION_IF_READONLY_OR_DISABLED \
  if (mFlags & TEXT_EDITOR_FLAG_READONLY || mFlags & TEXT_EDITOR_FLAG_DISABLED) \
  {                     \
    *aCancel = PR_TRUE; \
    return NS_OK;       \
  };

/********************************************************
 *  Constructor/Destructor 
 ********************************************************/

nsTextEditRules::nsTextEditRules()
{
  mEditor = nsnull;
  mFlags=0;
}

nsTextEditRules::~nsTextEditRules()
{
   // do NOT delete mEditor here.  We do not hold a ref count to mEditor.  mEditor owns our lifespan.
}


/********************************************************
 *  Public methods 
 ********************************************************/

NS_IMETHODIMP
nsTextEditRules::Init(nsIEditor *aEditor)
{
  if (!aEditor) { return NS_ERROR_NULL_POINTER; }
  mEditor = (nsTextEditor*)aEditor;  // we hold a non-refcounted reference back to our editor
  nsCOMPtr<nsIDOMSelection> selection;
  mEditor->GetSelection(getter_AddRefs(selection));
  NS_ASSERTION(selection, "editor cannot get selection");
  nsresult result = CreateBogusNodeIfNeeded(selection);   // this method handles null selection, which should never happen anyway
  return result;
}

NS_IMETHODIMP
nsTextEditRules::GetFlags(PRUint32 *aFlags)
{
  if (!aFlags) { return NS_ERROR_NULL_POINTER; }
  *aFlags = mFlags;
  return NS_OK;
}

NS_IMETHODIMP
nsTextEditRules::SetFlags(PRUint32 aFlags)
{
  mFlags = aFlags;
  return NS_OK;
}

NS_IMETHODIMP 
nsTextEditRules::WillDoAction(nsIDOMSelection *aSelection, 
                              nsRulesInfo *aInfo, PRBool *aCancel)
{
  if (!aSelection || !aInfo || !aCancel) { return NS_ERROR_NULL_POINTER; }

  *aCancel = PR_FALSE;

  // my kingdom for dynamic cast
  nsTextRulesInfo *info = NS_STATIC_CAST(nsTextRulesInfo*, aInfo);
    
  switch (info->action)
  {
    case kInsertBreak:
      return WillInsertBreak(aSelection, aCancel);
    case kInsertText:
      return WillInsertText(aSelection, 
                            aCancel, 
                            info->placeTxn, 
                            info->inString,
                            info->outString,
                            info->typeInState,
                            info->maxLength);
    case kDeleteSelection:
      return WillDeleteSelection(aSelection, aCancel);
    case kUndo:
      return WillUndo(aSelection, aCancel);
    case kRedo:
      return WillRedo(aSelection, aCancel);
    case kSetTextProperty:
      return WillSetTextProperty(aSelection, aCancel);
    case kRemoveTextProperty:
      return WillRemoveTextProperty(aSelection, aCancel);
  }
  return NS_ERROR_FAILURE;
}
  
NS_IMETHODIMP 
nsTextEditRules::DidDoAction(nsIDOMSelection *aSelection,
                             nsRulesInfo *aInfo, nsresult aResult)
{
  if (!aSelection || !aInfo) 
    return NS_ERROR_NULL_POINTER;
    
  // my kingdom for dynamic cast
  nsTextRulesInfo *info = NS_STATIC_CAST(nsTextRulesInfo*, aInfo);

  switch (info->action)
  {
   case kInsertBreak:
     return DidInsertBreak(aSelection, aResult);
    case kInsertText:
      return DidInsertText(aSelection, aResult);
    case kDeleteSelection:
      return DidDeleteSelection(aSelection, aResult);
    case kUndo:
      return DidUndo(aSelection, aResult);
    case kRedo:
      return DidRedo(aSelection, aResult);
    case kSetTextProperty:
      return DidSetTextProperty(aSelection, aResult);
    case kRemoveTextProperty:
      return DidRemoveTextProperty(aSelection, aResult);
  }
  return NS_ERROR_FAILURE;
}
  

/********************************************************
 *  Protected methods 
 ********************************************************/


nsresult
nsTextEditRules::WillInsert(nsIDOMSelection *aSelection, PRBool *aCancel)
{
  if (!aSelection || !aCancel) { return NS_ERROR_NULL_POINTER; }
  CANCEL_OPERATION_IF_READONLY_OR_DISABLED

  // initialize out param
  *aCancel = PR_FALSE;
  
  // check for the magic content node and delete it if it exists
  if (mBogusNode)
  {
    mEditor->DeleteNode(mBogusNode);
    mBogusNode = do_QueryInterface(nsnull);
    // there is no longer any legit selection, so clear it.
    aSelection->ClearSelection();
  }

  return NS_OK;
}

nsresult
nsTextEditRules::DidInsert(nsIDOMSelection *aSelection, nsresult aResult)
{
  return NS_OK;
}

nsresult
nsTextEditRules::WillInsertBreak(nsIDOMSelection *aSelection, PRBool *aCancel)
{
  if (!aSelection || !aCancel) { return NS_ERROR_NULL_POINTER; }
  CANCEL_OPERATION_IF_READONLY_OR_DISABLED
  if (mFlags & TEXT_EDITOR_FLAG_SINGLELINE) {
    *aCancel = PR_TRUE;
  }
  else {
    *aCancel = PR_FALSE;
  }
  return NS_OK;
}

nsresult
nsTextEditRules::DidInsertBreak(nsIDOMSelection *aSelection, nsresult aResult)
{
  return NS_OK;
}


nsresult
nsTextEditRules::WillInsertText(nsIDOMSelection *aSelection, 
                                PRBool          *aCancel,
                                PlaceholderTxn **aTxn,
                                const nsString *inString,
                                nsString       *outString,
                                TypeInState    typeInState,
                                PRInt32         aMaxLength)
{
  if (!aSelection || !aCancel || !inString || !outString) {return NS_ERROR_NULL_POINTER;}
  CANCEL_OPERATION_IF_READONLY_OR_DISABLED

  if (-1 != aMaxLength && (mFlags&TEXT_EDITOR_FLAG_PLAINTEXT))
  {
    // get the current text length
    // get the length of inString
    // if l1 is at or over max, cancel the insert
    // if l1 + l2 > max, set outString to subset of inString so length = max
    
  }
    
  // initialize out params
  *aCancel = PR_FALSE;
  *outString = *inString;
  
  if (mBogusNode || (PR_TRUE==typeInState.IsAnySet()))
  {
    nsresult result = TransactionFactory::GetNewTransaction(kPlaceholderTxnIID, (EditTxn **)aTxn);
    if (NS_FAILED(result)) { return result; }
    if (!*aTxn) { return NS_ERROR_NULL_POINTER; }
    (*aTxn)->SetName(InsertTextTxn::gInsertTextTxnName);
    mEditor->Do(*aTxn);
  }
  nsresult result = WillInsert(aSelection, aCancel);
  if (NS_SUCCEEDED(result) && (PR_FALSE==*aCancel))
  {
    if (PR_TRUE==typeInState.IsAnySet())
    { // for every property that is set, insert a new inline style node
      result = CreateStyleForInsertText(aSelection, typeInState);
    }
  }
  return result;
}

nsresult
nsTextEditRules::DidInsertText(nsIDOMSelection *aSelection, 
                               nsresult aResult)
{
  return DidInsert(aSelection, aResult);
}

nsresult
nsTextEditRules::CreateStyleForInsertText(nsIDOMSelection *aSelection, TypeInState &aTypeInState)
{ 
  // private method, we know aSelection is not null, and that it is collapsed
  NS_ASSERTION(nsnull!=aSelection, "bad selection");

  // We know at least one style is set and we're about to insert at least one character.
  // If the selection is in a text node, split the node (even if we're at the beginning or end)
  // then put the text node inside new inline style parents.
  // Otherwise, create the text node and the new inline style parents.
  nsCOMPtr<nsIDOMNode>anchor;
  PRInt32 offset;
  nsresult result = aSelection->GetAnchorNode(getter_AddRefs(anchor));
  if (NS_SUCCEEDED(result) && NS_SUCCEEDED(aSelection->GetAnchorOffset(&offset)) && anchor)
  {
    nsCOMPtr<nsIDOMCharacterData>anchorAsText;
    anchorAsText = do_QueryInterface(anchor);
    if (anchorAsText)
    {
      nsCOMPtr<nsIDOMNode>newTextNode;
      // create an empty text node by splitting the selected text node according to offset
      if (0==offset)
      {
        result = mEditor->SplitNode(anchorAsText, offset, getter_AddRefs(newTextNode));
      }
      else
      {
        PRUint32 length;
        anchorAsText->GetLength(&length);
        if (length==offset)
        {
          // newTextNode will be the left node
          result = mEditor->SplitNode(anchorAsText, offset, getter_AddRefs(newTextNode));
          // but we want the right node in this case
          newTextNode = do_QueryInterface(anchor);
        }
        else
        {
          // splitting anchor twice sets newTextNode as an empty text node between 
          // two halves of the original text node
          result = mEditor->SplitNode(anchorAsText, offset, getter_AddRefs(newTextNode));
          result = mEditor->SplitNode(anchorAsText, 0, getter_AddRefs(newTextNode));
        }
      }
      // now we have the new text node we are going to insert into.  
      // create style nodes or move it up the content hierarchy as needed.
      if ((NS_SUCCEEDED(result)) && newTextNode)
      {
        nsCOMPtr<nsIDOMNode>newStyleNode;
        if (aTypeInState.IsSet(NS_TYPEINSTATE_BOLD))
        {
          if (PR_TRUE==aTypeInState.GetBold()) {
            result = InsertStyleNode(newTextNode, nsIEditProperty::b, aSelection, getter_AddRefs(newStyleNode));
          }
          else {
            printf("not yet implemented, make not bold in a bold context\n");
          }
        }
        if (aTypeInState.IsSet(NS_TYPEINSTATE_ITALIC))
        {
          if (PR_TRUE==aTypeInState.GetItalic()) { 
            result = InsertStyleNode(newTextNode, nsIEditProperty::i, aSelection, getter_AddRefs(newStyleNode));
          }
          else
          {
            printf("not yet implemented, make not italic in a italic context\n");
          }
        }
        if (aTypeInState.IsSet(NS_TYPEINSTATE_UNDERLINE))
        {
          if (PR_TRUE==aTypeInState.GetUnderline()) {
            result = InsertStyleNode(newTextNode, nsIEditProperty::u, aSelection, getter_AddRefs(newStyleNode));
          }
          else
          {
            printf("not yet implemented, make not underline in an underline context\n");
          }
        }
        if (aTypeInState.IsSet(NS_TYPEINSTATE_FONTCOLOR))
        {
          nsAutoString value;
          aTypeInState.GetFontColor(value);
          nsAutoString attr;
          nsIEditProperty::color->ToString(attr);
          result = CreateFontStyleForInsertText(newTextNode, attr, value, aSelection);
        }
        if (aTypeInState.IsSet(NS_TYPEINSTATE_FONTFACE))
        {
          nsAutoString value;
          aTypeInState.GetFontFace(value);
          nsAutoString attr;
          nsIEditProperty::face->ToString(attr);
          result = CreateFontStyleForInsertText(newTextNode, attr, value, aSelection); 
        }
        if (aTypeInState.IsSet(NS_TYPEINSTATE_FONTSIZE))
        {
          nsAutoString value;
          aTypeInState.GetFontSize(value);
          nsAutoString attr;
          nsIEditProperty::size->ToString(attr);
          result = CreateFontStyleForInsertText(newTextNode, attr, value, aSelection);
        }
      }
    }
    else
    {
      printf("not yet implemented.  selection is not text.\n");
    }
  }
  else  // we have no selection, so insert a style tag in the body
  {
    nsCOMPtr<nsIDOMDocument>doc;
    mEditor->GetDocument(getter_AddRefs(doc));  
    nsCOMPtr<nsIDOMNodeList>nodeList;
    nsAutoString bodyTag = "body";
    result = doc->GetElementsByTagName(bodyTag, getter_AddRefs(nodeList));
    if ((NS_SUCCEEDED(result)) && nodeList)
    {
      PRUint32 count;
      nodeList->GetLength(&count);
      NS_ASSERTION(1==count, "there is not exactly 1 body in the document!");
      nsCOMPtr<nsIDOMNode>bodyNode;
      result = nodeList->Item(0, getter_AddRefs(bodyNode));
      if ((NS_SUCCEEDED(result)) && bodyNode)
      { // now we've got the body tag.  insert the style tag
        if (aTypeInState.IsSet(NS_TYPEINSTATE_BOLD))
        {
          if (PR_TRUE==aTypeInState.GetBold()) { 
            InsertStyleAndNewTextNode(bodyNode, nsIEditProperty::b, aSelection);
          }
        }
        if (aTypeInState.IsSet(NS_TYPEINSTATE_ITALIC))
        {
          if (PR_TRUE==aTypeInState.GetItalic()) { 
            InsertStyleAndNewTextNode(bodyNode, nsIEditProperty::i, aSelection);
          }
        }
        if (aTypeInState.IsSet(NS_TYPEINSTATE_UNDERLINE))
        {
          if (PR_TRUE==aTypeInState.GetUnderline()) { 
            InsertStyleAndNewTextNode(bodyNode, nsIEditProperty::u, aSelection);
          }
        }
      }
    }
  }
  return result;
}

nsresult
nsTextEditRules::CreateFontStyleForInsertText(nsIDOMNode      *aNewTextNode,
                                              const nsString  &aAttr, 
                                              const nsString  &aValue,
                                              nsIDOMSelection *aSelection)
{
  nsresult result = NS_OK;
  nsCOMPtr<nsIDOMNode>newStyleNode;
  if (0!=aValue.Length()) 
  { 
    result = InsertStyleNode(aNewTextNode, nsIEditProperty::font, aSelection, getter_AddRefs(newStyleNode));
    if (NS_SUCCEEDED(result) && newStyleNode)
    {
      nsCOMPtr<nsIDOMElement>element = do_QueryInterface(newStyleNode);
      if (element) {
        result = mEditor->SetAttribute(element, aAttr, aValue);
      }
    }
  }
  else
  {
    printf("not yet implemented, undo font in an font context\n");
  }
  return result;
}

nsresult
nsTextEditRules::InsertStyleNode(nsIDOMNode      *aNode, 
                                 nsIAtom         *aTag, 
                                 nsIDOMSelection *aSelection,
                                 nsIDOMNode     **aNewNode)
{
  NS_ASSERTION(aNode && aTag, "bad args");
  if (!aNode || !aTag) { return NS_ERROR_NULL_POINTER; }

  nsresult result;
  nsCOMPtr<nsIDOMNode>parent;
  aNode->GetParentNode(getter_AddRefs(parent));
  PRInt32 offsetInParent;
  nsEditor::GetChildOffset(aNode, parent, offsetInParent);
  nsAutoString tag;
  aTag->ToString(tag);
  result = mEditor->CreateNode(tag, parent, offsetInParent, aNewNode);
  if ((NS_SUCCEEDED(result)) && *aNewNode)
  {
    result = mEditor->DeleteNode(aNode);
    if (NS_SUCCEEDED(result))
    {
      result = mEditor->InsertNode(aNode, *aNewNode, 0);
      if (NS_SUCCEEDED(result)) {
        if (aSelection) {
          aSelection->Collapse(aNode, 0);
        }
      }
    }
  }
  return result;
}


nsresult
nsTextEditRules::InsertStyleAndNewTextNode(nsIDOMNode *aParentNode, nsIAtom *aTag, nsIDOMSelection *aSelection)
{
  NS_ASSERTION(aParentNode && aTag, "bad args");
  if (!aParentNode || !aTag) { return NS_ERROR_NULL_POINTER; }

  nsresult result;
  // if the selection already points to a text node, just call InsertStyleNode()
  if (aSelection)
  {
    nsCOMPtr<nsIDOMNode>anchor;
    PRInt32 offset;
    result = aSelection->GetAnchorNode(getter_AddRefs(anchor));
    if (NS_SUCCEEDED(result) && NS_SUCCEEDED(aSelection->GetAnchorOffset(&offset)) && anchor)
    {
      nsCOMPtr<nsIDOMCharacterData>anchorAsText;
      anchorAsText = do_QueryInterface(anchor);
      if (anchorAsText)
      {
        nsCOMPtr<nsIDOMNode> newStyleNode;
        result = InsertStyleNode(anchor, aTag, aSelection, getter_AddRefs(newStyleNode));
        return result;
      }
    }
  }
  // if we get here, there is no selected text node so we create one.
  nsAutoString tag;
  aTag->ToString(tag);
  nsCOMPtr<nsIDOMNode>newStyleNode;
  nsCOMPtr<nsIDOMNode>newTextNode;
  result = mEditor->CreateNode(tag, aParentNode, 0, getter_AddRefs(newStyleNode));
  if (NS_SUCCEEDED(result)) 
  {
    result = mEditor->CreateNode(nsIEditor::GetTextNodeTag(), newStyleNode, 0, getter_AddRefs(newTextNode));
    if (NS_SUCCEEDED(result)) 
    {
      if (aSelection) {
        aSelection->Collapse(newTextNode, 0);
      }
    }
  }
  return result;
}

nsresult
nsTextEditRules::WillSetTextProperty(nsIDOMSelection *aSelection, PRBool *aCancel)
{
  nsresult result = NS_OK;

  // XXX: should probably return a success value other than NS_OK that means "not allowed"
  if (TEXT_EDITOR_FLAG_PLAINTEXT & mFlags) {
    *aCancel = PR_TRUE;
  }
  return result;
}

nsresult
nsTextEditRules::DidSetTextProperty(nsIDOMSelection *aSelection, nsresult aResult)
{
  return NS_OK;
}

nsresult
nsTextEditRules::WillRemoveTextProperty(nsIDOMSelection *aSelection, PRBool *aCancel)
{
  nsresult result = NS_OK;

  // XXX: should probably return a success value other than NS_OK that means "not allowed"
  if (TEXT_EDITOR_FLAG_PLAINTEXT & mFlags) {
    *aCancel = PR_TRUE;
  }
  return result;
}

nsresult
nsTextEditRules::DidRemoveTextProperty(nsIDOMSelection *aSelection, nsresult aResult)
{
  return NS_OK;
}

nsresult
nsTextEditRules::WillDeleteSelection(nsIDOMSelection *aSelection, PRBool *aCancel)
{
  if (!aSelection || !aCancel) { return NS_ERROR_NULL_POINTER; }
  CANCEL_OPERATION_IF_READONLY_OR_DISABLED

  // initialize out param
  *aCancel = PR_FALSE;
  
  // if there is only bogus content, cancel the operation
  if (mBogusNode) {
    *aCancel = PR_TRUE;
    return NS_OK;
  }
  return NS_OK;
}

// if the document is empty, insert a bogus text node with a &nbsp;
// if we ended up with consecutive text nodes, merge them
nsresult
nsTextEditRules::DidDeleteSelection(nsIDOMSelection *aSelection, nsresult aResult)
{
  nsresult result = aResult;  // if aResult is an error, we just return it
  if (!aSelection) { return NS_ERROR_NULL_POINTER; }
  PRBool isCollapsed;
  aSelection->GetIsCollapsed(&isCollapsed);
  NS_ASSERTION(PR_TRUE==isCollapsed, "selection not collapsed after delete selection.");
  // if the delete selection resulted in no content 
  // insert a special bogus text node with a &nbsp; character in it.
  if (NS_SUCCEEDED(result)) // only do this work if DeleteSelection completed successfully
  {
    result = CreateBogusNodeIfNeeded(aSelection);
    // if we don't have an empty document, check the selection to see if any collapsing is necessary
    if (!mBogusNode)
    {
      nsCOMPtr<nsIDOMNode>anchor;
      PRInt32 offset;
		  result = aSelection->GetAnchorNode(getter_AddRefs(anchor));
		  if (NS_SUCCEEDED(result) && NS_SUCCEEDED(aSelection->GetAnchorOffset(&offset)) && anchor)
      {
        nsCOMPtr<nsIDOMNodeList> anchorChildren;
        result = anchor->GetChildNodes(getter_AddRefs(anchorChildren));
        nsCOMPtr<nsIDOMNode> selectedNode;
        if ((NS_SUCCEEDED(result)) && anchorChildren) {              
          result = anchorChildren->Item(offset, getter_AddRefs(selectedNode));
        }
        else {
          selectedNode = do_QueryInterface(anchor);
        }
        if ((NS_SUCCEEDED(result)) && selectedNode)
        {
          nsCOMPtr<nsIDOMCharacterData>selectedNodeAsText;
          selectedNodeAsText = do_QueryInterface(selectedNode);
          if (selectedNodeAsText)
          {
            nsCOMPtr<nsIDOMNode> siblingNode;
            selectedNode->GetPreviousSibling(getter_AddRefs(siblingNode));
            if (siblingNode)
            {
              nsCOMPtr<nsIDOMCharacterData>siblingNodeAsText;
              siblingNodeAsText = do_QueryInterface(siblingNode);
              if (siblingNodeAsText)
              {
                PRUint32 siblingLength; // the length of siblingNode before the join
                siblingNodeAsText->GetLength(&siblingLength);
                nsCOMPtr<nsIDOMNode> parentNode;
                selectedNode->GetParentNode(getter_AddRefs(parentNode));
                result = mEditor->JoinNodes(siblingNode, selectedNode, parentNode);
                // selectedNode will remain after the join, siblingNode is removed
              }
            }
            selectedNode->GetNextSibling(getter_AddRefs(siblingNode));
            if (siblingNode)
            {
              nsCOMPtr<nsIDOMCharacterData>siblingNodeAsText;
              siblingNodeAsText = do_QueryInterface(siblingNode);
              if (siblingNodeAsText)
              {
                PRUint32 selectedNodeLength; // the length of siblingNode before the join
                selectedNodeAsText->GetLength(&selectedNodeLength);
                nsCOMPtr<nsIDOMNode> parentNode;
                selectedNode->GetParentNode(getter_AddRefs(parentNode));
                result = mEditor->JoinNodes(selectedNode, siblingNode, parentNode);
                // selectedNode will remain after the join, siblingNode is removed
                // set selection
                aSelection->Collapse(siblingNode, selectedNodeLength);
              }
            }
          }
        }
      }
    }
  }
  return result;
}

nsresult
nsTextEditRules::WillUndo(nsIDOMSelection *aSelection, PRBool *aCancel)
{
  if (!aSelection || !aCancel) { return NS_ERROR_NULL_POINTER; }
  CANCEL_OPERATION_IF_READONLY_OR_DISABLED
  // initialize out param
  *aCancel = PR_FALSE;
  return NS_OK;
}

/* the idea here is to see if the magic empty node has suddenly reappeared as the result of the undo.
 * if it has, set our state so we remember it.
 * There is a tradeoff between doing here and at redo, or doing it everywhere else that might care.
 * Since undo and redo are relatively rare, it makes sense to take the (small) performance hit here.
 */
nsresult
nsTextEditRules:: DidUndo(nsIDOMSelection *aSelection, nsresult aResult)
{
  nsresult result = aResult;  // if aResult is an error, we return it.
  if (!aSelection) { return NS_ERROR_NULL_POINTER; }
  if (NS_SUCCEEDED(result)) 
  {
    if (mBogusNode) {
      mBogusNode = do_QueryInterface(nsnull);
    }
    else
    {
      nsCOMPtr<nsIDOMNode>node;
      PRInt32 offset;
		  result = aSelection->GetAnchorNode(getter_AddRefs(node));
		  if (NS_SUCCEEDED(result) && NS_SUCCEEDED(aSelection->GetAnchorOffset(&offset)) && node)
      {
        nsCOMPtr<nsIDOMElement>element;
        element = do_QueryInterface(node);
        if (element)
        {
          nsAutoString att(nsEditor::kMOZEditorBogusNodeAttr);
          nsAutoString val;
          (void)element->GetAttribute(att, val);
          if (val.Equals(nsEditor::kMOZEditorBogusNodeValue)) {
            mBogusNode = do_QueryInterface(element);
          }
        }
        nsCOMPtr<nsIDOMNode> temp;
        result = node->GetParentNode(getter_AddRefs(temp));
        node = do_QueryInterface(temp);
      }
    }
  }
  return result;
}

nsresult
nsTextEditRules::WillRedo(nsIDOMSelection *aSelection, PRBool *aCancel)
{
  if (!aSelection || !aCancel) { return NS_ERROR_NULL_POINTER; }
  CANCEL_OPERATION_IF_READONLY_OR_DISABLED
  // initialize out param
  *aCancel = PR_FALSE;
  return NS_OK;
}

nsresult
nsTextEditRules::DidRedo(nsIDOMSelection *aSelection, nsresult aResult)
{
  nsresult result = aResult;  // if aResult is an error, we return it.
  if (!aSelection) { return NS_ERROR_NULL_POINTER; }
  if (NS_SUCCEEDED(result)) 
  {
    if (mBogusNode) {
      mBogusNode = do_QueryInterface(nsnull);
    }
    else
    {
      nsCOMPtr<nsIDOMNode>node;
      PRInt32 offset;
		  result = aSelection->GetAnchorNode(getter_AddRefs(node));
		  if (NS_SUCCEEDED(result) && NS_SUCCEEDED(aSelection->GetAnchorOffset(&offset)) && node)
      {
        nsCOMPtr<nsIDOMElement>element;
        element = do_QueryInterface(node);
        if (element)
        {
          nsAutoString att(nsEditor::kMOZEditorBogusNodeAttr);
          nsAutoString val;
          (void)element->GetAttribute(att, val);
          if (val.Equals(nsEditor::kMOZEditorBogusNodeValue)) {
            mBogusNode = do_QueryInterface(element);
          }
        }
        nsCOMPtr<nsIDOMNode> temp;
        result = node->GetParentNode(getter_AddRefs(temp));
        node = do_QueryInterface(temp);
      }
    }
  }
  return result;
}


nsresult
nsTextEditRules::CreateBogusNodeIfNeeded(nsIDOMSelection *aSelection)
{
  if (!aSelection) { return NS_ERROR_NULL_POINTER; }
  nsCOMPtr<nsIDOMDocument>doc;
  mEditor->GetDocument(getter_AddRefs(doc));  
  nsCOMPtr<nsIDOMNodeList>nodeList;
  nsAutoString bodyTag = "body";
  nsresult result = doc->GetElementsByTagName(bodyTag, getter_AddRefs(nodeList));
  if ((NS_SUCCEEDED(result)) && nodeList)
  {
    PRUint32 count;
    nodeList->GetLength(&count);
    NS_ASSERTION(1==count, "there is not exactly 1 body in the document!");
    nsCOMPtr<nsIDOMNode>bodyNode;
    result = nodeList->Item(0, getter_AddRefs(bodyNode));
    if ((NS_SUCCEEDED(result)) && bodyNode)
    { // now we've got the body tag.
      // iterate the body tag, looking for editable content
      // if no editable content is found, insert the bogus node
      PRBool needsBogusContent=PR_TRUE;
      nsCOMPtr<nsIDOMNode>bodyChild;
      result = bodyNode->GetFirstChild(getter_AddRefs(bodyChild));        
      while ((NS_SUCCEEDED(result)) && bodyChild)
      { 
        if (PR_TRUE==nsEditor::IsEditable(bodyChild))
        {
          needsBogusContent = PR_FALSE;
          break;
        }
        nsCOMPtr<nsIDOMNode>temp;
        bodyChild->GetNextSibling(getter_AddRefs(temp));
        bodyChild = do_QueryInterface(temp);
      }
      if (PR_TRUE==needsBogusContent)
      {
        // set mBogusNode to be the newly created <P>
        result = mEditor->CreateNode(nsAutoString("P"), bodyNode, 0, 
                                     getter_AddRefs(mBogusNode));
        if ((NS_SUCCEEDED(result)) && mBogusNode)
        {
          nsCOMPtr<nsIDOMNode>newTNode;
          result = mEditor->CreateNode(nsIEditor::GetTextNodeTag(), mBogusNode, 0, 
                                       getter_AddRefs(newTNode));
          if ((NS_SUCCEEDED(result)) && newTNode)
          {
            nsCOMPtr<nsIDOMCharacterData>newNodeAsText;
            newNodeAsText = do_QueryInterface(newTNode);
            if (newNodeAsText)
            {
              nsAutoString data;
              data += 160;
              newNodeAsText->SetData(data);
              aSelection->Collapse(newTNode, 0);
            }
          }
          // make sure we know the PNode is bogus
          nsCOMPtr<nsIDOMElement>newPElement;
          newPElement = do_QueryInterface(mBogusNode);
          if (newPElement)
          {
            nsAutoString att(nsEditor::kMOZEditorBogusNodeAttr);
            nsAutoString val(nsEditor::kMOZEditorBogusNodeValue);
            newPElement->SetAttribute(att, val);
          }
        }
      }
    }
  }
  return result;
}



