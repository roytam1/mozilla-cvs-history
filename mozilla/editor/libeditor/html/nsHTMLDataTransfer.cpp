/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: NPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is 
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the NPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */
#include "nsICaret.h"


#include "nsHTMLEditor.h"
#include "nsHTMLEditRules.h"
#include "nsTextEditUtils.h"
#include "nsHTMLEditUtils.h"
#include "nsWSRunObject.h"

#include "nsEditorEventListeners.h"

#include "nsIDOMText.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMDocument.h"
#include "nsIDOMAttr.h"
#include "nsIDocument.h"
#include "nsIDOMEventReceiver.h" 
#include "nsIDOMNSEvent.h"
#include "nsIDOMKeyEvent.h"
#include "nsIDOMKeyListener.h" 
#include "nsIDOMMouseListener.h"
#include "nsIDOMMouseEvent.h"
#include "nsISelection.h"
#include "nsISelectionPrivate.h"
#include "nsIDOMHTMLAnchorElement.h"
#include "nsIDOMHTMLImageElement.h"
#include "nsISelectionController.h"
#include "nsIFileChannel.h"
#include "nsIFrameSelection.h"  // For TABLESELECTION_ defines
#include "nsIIndependentSelection.h" //domselections answer to frameselection


#include "nsICSSLoader.h"
#include "nsICSSStyleSheet.h"
#include "nsIHTMLContentContainer.h"
#include "nsIStyleSet.h"
#include "nsIDocumentObserver.h"
#include "nsIDocumentStateListener.h"

#include "nsIStyleContext.h"

#include "nsIEnumerator.h"
#include "nsIContent.h"
#include "nsIContentIterator.h"
#include "nsEditorCID.h"
#include "nsLayoutCID.h"
#include "nsIDOMRange.h"
#include "nsIDOMNSRange.h"
#include "nsISupportsArray.h"
#include "nsVoidArray.h"
#include "nsFileSpec.h"
#include "nsIFile.h"
#include "nsIURL.h"
#include "nsIComponentManager.h"
#include "nsIServiceManager.h"
#include "nsWidgetsCID.h"
#include "nsIDocumentEncoder.h"
#include "nsIDOMDocumentFragment.h"
#include "nsIPresShell.h"
#include "nsIPresContext.h"
#include "nsIParser.h"
#include "nsParserCIID.h"
#include "nsIImage.h"
#include "nsAOLCiter.h"
#include "nsInternetCiter.h"
#include "nsXPCOM.h"
#include "nsISupportsPrimitives.h"

// netwerk
#include "nsIURI.h"
#include "nsNetUtil.h"

// Drag & Drop, Clipboard
#include "nsWidgetsCID.h"
#include "nsIClipboard.h"
#include "nsITransferable.h"
#include "nsIDragService.h"
#include "nsIDOMNSUIEvent.h"

// Misc
#include "TextEditorTest.h"
#include "nsEditorUtils.h"
#include "nsIPref.h"
const PRUnichar nbsp = 160;

static NS_DEFINE_CID(kCRangeCID,      NS_RANGE_CID);
static NS_DEFINE_CID(kPrefServiceCID, NS_PREF_CID);

// Drag & Drop, Clipboard Support
static NS_DEFINE_CID(kCClipboardCID,    NS_CLIPBOARD_CID);
static NS_DEFINE_CID(kCTransferableCID, NS_TRANSFERABLE_CID);
static NS_DEFINE_CID(kCHTMLFormatConverterCID, NS_HTMLFORMATCONVERTER_CID);
// private clipboard data flavors for html copy/paste
#define kHTMLContext   "text/_moz_htmlcontext"
#define kHTMLInfo      "text/_moz_htmlinfo"


#if defined(NS_DEBUG) && defined(DEBUG_buster)
static PRBool gNoisy = PR_FALSE;
#else
static const PRBool gNoisy = PR_FALSE;
#endif
static nsCOMPtr<nsIDOMNode> GetListParent(nsIDOMNode* aNode)
{
  if (!aNode) return nsnull;
  nsCOMPtr<nsIDOMNode> parent, tmp;
  aNode->GetParentNode(getter_AddRefs(parent));
  while (parent)
  {
    if (nsHTMLEditUtils::IsList(parent)) return parent;
    parent->GetParentNode(getter_AddRefs(tmp));
    parent = tmp;
  }
  return nsnull;
}

static nsCOMPtr<nsIDOMNode> GetTableParent(nsIDOMNode* aNode)
{
  if (!aNode) return nsnull;
  nsCOMPtr<nsIDOMNode> parent, tmp;
  aNode->GetParentNode(getter_AddRefs(parent));
  while (parent)
  {
    if (nsHTMLEditUtils::IsTable(parent)) return parent;
    parent->GetParentNode(getter_AddRefs(tmp));
    parent = tmp;
  }
  return nsnull;
}


NS_IMETHODIMP nsHTMLEditor::LoadHTML(const nsAString & aInString)
{
  nsAutoString charset;
  return LoadHTMLWithCharset(aInString, charset);
}

NS_IMETHODIMP nsHTMLEditor::LoadHTMLWithCharset(const nsAString & aInputString, const nsAString & aCharset)
{
  nsresult res = NS_OK;
  if (!mRules) return NS_ERROR_NOT_INITIALIZED;

  // force IME commit; set up rules sniffing and batching
  ForceCompositionEnd();
  nsAutoEditBatch beginBatching(this);
  nsAutoRules beginRulesSniffing(this, kOpLoadHTML, nsIEditor::eNext);
  
  // Get selection
  nsCOMPtr<nsISelection>selection;
  res = GetSelection(getter_AddRefs(selection));
  if (NS_FAILED(res)) return res;
  
  nsTextRulesInfo ruleInfo(nsTextEditRules::kLoadHTML);
  PRBool cancel, handled;
  res = mRules->WillDoAction(selection, &ruleInfo, &cancel, &handled);
  if (NS_FAILED(res)) return res;
  if (cancel) return NS_OK; // rules canceled the operation
  if (!handled)
  {
    PRBool isCollapsed;
    res = selection->GetIsCollapsed(&isCollapsed);
    if (NS_FAILED(res)) return res;

    // Delete Selection, but only if it isn't collapsed, see bug #106269
    if (!isCollapsed) 
    {
      res = DeleteSelection(eNone);
      if (NS_FAILED(res)) return res;
    }

    // Get the first range in the selection, for context:
    nsCOMPtr<nsIDOMRange> range, clone;
    res = selection->GetRangeAt(0, getter_AddRefs(range));
    NS_ENSURE_SUCCESS(res, res);
    if (!range)
      return NS_ERROR_NULL_POINTER;
    nsCOMPtr<nsIDOMNSRange> nsrange (do_QueryInterface(range));
    if (!nsrange)
      return NS_ERROR_NO_INTERFACE;

    // create fragment for pasted html
    nsCOMPtr<nsIDOMDocumentFragment> docfrag;
    {
      res = nsrange->CreateContextualFragment(aInputString, getter_AddRefs(docfrag));
    NS_ENSURE_SUCCESS(res, res);
    }
    // put the fragment into the document
    nsCOMPtr<nsIDOMNode> parent, junk;
    res = range->GetStartContainer(getter_AddRefs(parent));
    NS_ENSURE_SUCCESS(res, res);
    if (!parent)
      return NS_ERROR_NULL_POINTER;
    PRInt32 childOffset;
    res = range->GetStartOffset(&childOffset);
    NS_ENSURE_SUCCESS(res, res);

    nsCOMPtr<nsIDOMNode> nodeToInsert;
    docfrag->GetFirstChild(getter_AddRefs(nodeToInsert));
    while (nodeToInsert)
    {
      res = InsertNode(nodeToInsert, parent, childOffset++);
      NS_ENSURE_SUCCESS(res, res);
      docfrag->GetFirstChild(getter_AddRefs(nodeToInsert));
    }
  }

  return mRules->DidDoAction(selection, &ruleInfo, res);
}


NS_IMETHODIMP nsHTMLEditor::InsertHTML(const nsAString & aInString)
{
  nsAutoString charset;
  return InsertHTMLWithCharset(aInString, charset);
}

nsresult
nsHTMLEditor::InsertHTMLWithContext(const nsAString & aInputString,
                                    const nsAString & aContextStr,
                                    const nsAString & aInfoStr)
{
  nsAutoString charset;
  return InsertHTMLWithCharsetAndContext(aInputString, charset,
                                         aContextStr, aInfoStr);
}


NS_IMETHODIMP
nsHTMLEditor::InsertHTMLWithCharset(const nsAString & aInputString,
                                    const nsAString & aCharset)
{
  return InsertHTMLWithCharsetAndContext(aInputString, aCharset,
                                         nsAutoString(), nsAutoString());
}


nsresult
nsHTMLEditor::InsertHTMLWithCharsetAndContext(const nsAString & aInputString,
                                              const nsAString & aCharset,
                                              const nsAString & aContextStr,
                                              const nsAString & aInfoStr)
{
  if (!mRules) return NS_ERROR_NOT_INITIALIZED;

  // force IME commit; set up rules sniffing and batching
  ForceCompositionEnd();
  nsAutoEditBatch beginBatching(this);
  nsAutoRules beginRulesSniffing(this, kOpHTMLPaste, nsIEditor::eNext);
  
  // Get selection
  nsresult res;
  nsCOMPtr<nsISelection>selection;
  res = GetSelection(getter_AddRefs(selection));
  if (NS_FAILED(res)) return res;
  
  // Get the first range in the selection, for context:
  nsCOMPtr<nsIDOMRange> range, clone;
  res = selection->GetRangeAt(0, getter_AddRefs(range));
  NS_ENSURE_SUCCESS(res, res);
  res = range->CloneRange(getter_AddRefs(clone));
  NS_ENSURE_SUCCESS(res, res);
  nsCOMPtr<nsIDOMNSRange> nsrange (do_QueryInterface(clone));
  if (!nsrange)
    return NS_ERROR_NO_INTERFACE;

  // create a dom document fragment that represents the structure to paste
  nsCOMPtr<nsIDOMNode> fragmentAsNode;
  PRInt32 rangeStartHint, rangeEndHint;
  res = CreateDOMFragmentFromPaste(nsrange, aInputString, aContextStr, aInfoStr, 
                                            address_of(fragmentAsNode),
                                            &rangeStartHint, &rangeEndHint);
  NS_ENSURE_SUCCESS(res, res);

  // make a list of what nodes in docFrag we need to move
  nsCOMPtr<nsISupportsArray> nodeList;
  res = CreateListOfNodesToPaste(fragmentAsNode, address_of(nodeList), rangeStartHint, rangeEndHint);
  NS_ENSURE_SUCCESS(res, res);
  
  PRUint32 cc;

  nodeList->Count(&cc);
  
  // are there any table elements in the list?  
  // node and offset for insertion
  nsCOMPtr<nsIDOMNode> parentNode;
  PRInt32 offsetOfNewNode;
  
  // check for table cell selection mode
  PRBool cellSelectionMode = PR_FALSE;
  nsCOMPtr<nsIDOMElement> cell;
  res = GetFirstSelectedCell(nsnull, getter_AddRefs(cell));
  if (NS_SUCCEEDED(res) && cell)
  {
    cellSelectionMode = PR_TRUE;
  }
  
  if (cellSelectionMode)
  {
    // do we have table content to paste?  If so, we want to delete
    // the selected table cells and replace with new table elements;
    // but if not we want to delete _contents_ of cells and replace
    // with non-table elements.  Use cellSelectionMode bool to 
    // indicate results.
    nsCOMPtr<nsISupports> isupports = dont_AddRef(nodeList->ElementAt(0));
    nsCOMPtr<nsIDOMNode> firstNode( do_QueryInterface(isupports) );
    if (!nsHTMLEditUtils::IsTableElement(firstNode))
      cellSelectionMode = PR_FALSE;
  }

  if (!cellSelectionMode)
  {
    res = DeleteSelectionAndPrepareToCreateNode(parentNode, offsetOfNewNode);
    NS_ENSURE_SUCCESS(res, res);

    // pasting does not inherit local inline styles
    res = RemoveAllInlineProperties();
    NS_ENSURE_SUCCESS(res, res);
  }
  else
  {
    // delete whole cells: we will replace with new table content
    if (1)
    {
      // Save current selection since DeleteTableCell perturbs it
      nsAutoSelectionReset selectionResetter(selection, this);
      res = DeleteTableCell(1);
      NS_ENSURE_SUCCESS(res, res);
    }
    // colapse selection to beginning of deleted table content
    selection->CollapseToStart();
  }
  
  // give rules a chance to handle or cancel
  nsTextRulesInfo ruleInfo(nsTextEditRules::kInsertElement);
  PRBool cancel, handled;
  res = mRules->WillDoAction(selection, &ruleInfo, &cancel, &handled);
  if (NS_FAILED(res)) return res;
  if (cancel) return NS_OK; // rules canceled the operation
  if (!handled)
  {
    // The rules code (WillDoAction above) might have changed the selection.  
    // refresh our memory...
    res = GetStartNodeAndOffset(selection, address_of(parentNode), &offsetOfNewNode);
    if (!parentNode) res = NS_ERROR_FAILURE;
    if (NS_FAILED(res)) return res;
    
    // if there are any invisible br's after our insertion point, remove them.
    // this is because if there is a br at end of what we paste, it will make
    // the invisible br visible.
    nsWSRunObject wsObj(this, parentNode, offsetOfNewNode);
    if ( nsTextEditUtils::IsBreak(wsObj.mEndReasonNode) && 
        !IsVisBreak(wsObj.mEndReasonNode) )
    {
      res = DeleteNode(wsObj.mEndReasonNode);
      if (NS_FAILED(res)) return res;
    }
    
    // remeber if we are in a link.  
    PRBool bStartedInLink = IsInLink(parentNode);
  
    // are we in a text node?  If so, split it.
    if (IsTextNode(parentNode))
    {
      nsCOMPtr<nsIDOMNode> temp;
      res = SplitNodeDeep(parentNode, parentNode, offsetOfNewNode, &offsetOfNewNode);
      if (NS_FAILED(res)) return res;
      res = parentNode->GetParentNode(getter_AddRefs(temp));
      if (NS_FAILED(res)) return res;
      parentNode = temp;
    }

    // build up list of parents of first node in list that are either
    // lists or tables.  First examine front of paste node list.
    nsCOMPtr<nsISupportsArray> startListAndTableArray;
    res = GetListAndTableParents(PR_FALSE, nodeList, address_of(startListAndTableArray));
    NS_ENSURE_SUCCESS(res, res);
    
    // remember number of lists and tables above us
    PRInt32 highWaterMark = -1;
    if (startListAndTableArray->ElementAt(0))
    {
      res = DiscoverPartialListsAndTables(nodeList, startListAndTableArray, &highWaterMark);
      NS_ENSURE_SUCCESS(res, res);
    }

    // if we have pieces of tables or lists to be inserted, let's force the paste 
    // to deal with table elements right away, so that it doesn't orphan some 
    // table or list contents outside the table or list.
    if (highWaterMark >= 0)
    {
      res = ReplaceOrphanedStructure(PR_FALSE, nodeList, startListAndTableArray, highWaterMark);
      NS_ENSURE_SUCCESS(res, res);
    }
    
    // Now go through the same process again for the end of the paste node list.
    nsCOMPtr<nsISupportsArray> endListAndTableArray;
    res = GetListAndTableParents(PR_TRUE, nodeList, address_of(endListAndTableArray));
    NS_ENSURE_SUCCESS(res, res);
    highWaterMark = -1;
   
    // remember number of lists and tables above us
    if (endListAndTableArray->ElementAt(0))
    {
      res = DiscoverPartialListsAndTables(nodeList, endListAndTableArray, &highWaterMark);
      NS_ENSURE_SUCCESS(res, res);
    }
    
    // don't orphan partial list or table structure
    if (highWaterMark >= 0)
    {
      res = ReplaceOrphanedStructure(PR_TRUE, nodeList, endListAndTableArray, highWaterMark);
      NS_ENSURE_SUCCESS(res, res);
    }

    // Loop over the node list and paste the nodes:
    PRBool bDidInsert = PR_FALSE;
    nsCOMPtr<nsIDOMNode> parentBlock, lastInsertNode, insertedContextParent;
    PRUint32 listCount, j;
    nodeList->Count(&listCount);
    if (IsBlockNode(parentNode))
      parentBlock = parentNode;
    else
      parentBlock = GetBlockNodeParent(parentNode);
      
    for (j=0; j<listCount; j++)
    {
      nsCOMPtr<nsISupports> isupports = dont_AddRef(nodeList->ElementAt(j));
      nsCOMPtr<nsIDOMNode> curNode( do_QueryInterface(isupports) );

      nsString namestr;
      curNode->GetNodeName(namestr);

      NS_ENSURE_TRUE(curNode, NS_ERROR_FAILURE);
      NS_ENSURE_TRUE(curNode != fragmentAsNode, NS_ERROR_FAILURE);
      NS_ENSURE_TRUE(!nsTextEditUtils::IsBody(curNode), NS_ERROR_FAILURE);
      
      if (insertedContextParent)
      {
        // if we had to insert something higher up in the paste heirarchy, we want to 
        // skip any further paste nodes that descend from that.  Else we will paste twice.
        if (nsHTMLEditUtils::IsDescendantOf(curNode, insertedContextParent))
          continue;
      }
      
      // give the user a hand on table element insertion.  if they have
      // a table or table row on the clipboard, and are trying to insert
      // into a table or table row, insert the appropriate children instead.
      if (  (nsHTMLEditUtils::IsTableRow(curNode) && nsHTMLEditUtils::IsTableRow(parentNode))
         && (nsHTMLEditUtils::IsTable(curNode)    || nsHTMLEditUtils::IsTable(parentNode)) )
      {
        nsCOMPtr<nsIDOMNode> child;
        curNode->GetFirstChild(getter_AddRefs(child));
        while (child)
        {
          res = InsertNodeAtPoint(child, address_of(parentNode), &offsetOfNewNode, PR_TRUE);
          if (NS_SUCCEEDED(res)) 
          {
            bDidInsert = PR_TRUE;
            lastInsertNode = child;
            offsetOfNewNode++;
          }
          curNode->GetFirstChild(getter_AddRefs(child));
        }
      }
      // give the user a hand on list insertion.  if they have
      // a list on the clipboard, and are trying to insert
      // into a list or list item, insert the appropriate children instead,
      // ie, merge the lists instead of pasting in a sublist.
      else if (nsHTMLEditUtils::IsList(curNode) && 
              (nsHTMLEditUtils::IsList(parentNode)  || nsHTMLEditUtils::IsListItem(parentNode)) )
      {
        nsCOMPtr<nsIDOMNode> child, tmp;
        curNode->GetFirstChild(getter_AddRefs(child));
        while (child)
        {
          if (nsHTMLEditUtils::IsListItem(child) || nsHTMLEditUtils::IsList(child))
          {
            // check if we are pasting into empty list item. If so
            // delete it and paste into parent list instead.
            if (nsHTMLEditUtils::IsListItem(parentNode))
            {
              PRBool isEmpty;
              res = IsEmptyNode(parentNode, &isEmpty, PR_TRUE);
              if ((NS_SUCCEEDED(res)) && isEmpty)
              {
                nsCOMPtr<nsIDOMNode> listNode;
                PRInt32 newOffset;
                GetNodeLocation(parentNode, address_of(listNode), &newOffset);
                if (listNode)
                {
                  DeleteNode(parentNode);
                  parentNode = listNode;
                  offsetOfNewNode = newOffset;
                }
              }
            } 
            res = InsertNodeAtPoint(child, address_of(parentNode), &offsetOfNewNode, PR_TRUE);
            if (NS_SUCCEEDED(res)) 
            {
              bDidInsert = PR_TRUE;
              lastInsertNode = child;
              offsetOfNewNode++;
            }
          }
          else
          {
            curNode->RemoveChild(child, getter_AddRefs(tmp));
          }
          curNode->GetFirstChild(getter_AddRefs(child));
        }
        
      }
      // check for pre's going into pre's.  
      else if (nsHTMLEditUtils::IsPre(parentBlock) && nsHTMLEditUtils::IsPre(curNode))
      {
        nsCOMPtr<nsIDOMNode> child, tmp;
        curNode->GetFirstChild(getter_AddRefs(child));
        while (child)
        {
          res = InsertNodeAtPoint(child, address_of(parentNode), &offsetOfNewNode, PR_TRUE);
          if (NS_SUCCEEDED(res)) 
          {
            bDidInsert = PR_TRUE;
            lastInsertNode = child;
            offsetOfNewNode++;
          }
          curNode->GetFirstChild(getter_AddRefs(child));
        }
      }
      else
      {
        
        // try to insert
        res = InsertNodeAtPoint(curNode, address_of(parentNode), &offsetOfNewNode, PR_TRUE);
        if (NS_SUCCEEDED(res)) 
        {
          bDidInsert = PR_TRUE;
          lastInsertNode = curNode;
        }
          
        // assume failure means no legal parent in the document heirarchy.
        // try again with the parent of curNode in the paste heirarchy.
        nsCOMPtr<nsIDOMNode> parent;
        while (NS_FAILED(res) && curNode)
        {
          curNode->GetParentNode(getter_AddRefs(parent));
          if (parent && !nsTextEditUtils::IsBody(parent))
          {
            res = InsertNodeAtPoint(parent, address_of(parentNode), &offsetOfNewNode, PR_TRUE);
            if (NS_SUCCEEDED(res)) 
            {
              bDidInsert = PR_TRUE;
              insertedContextParent = parent;
              lastInsertNode = parent;
            }
          }
          curNode = parent;
        }
      }
      if (bDidInsert)
      {
        res = GetNodeLocation(lastInsertNode, address_of(parentNode), &offsetOfNewNode);
        NS_ENSURE_SUCCESS(res, res);
        offsetOfNewNode++;
      }
    }

    // Now collapse the selection to the end of what we just inserted:
    if (lastInsertNode) 
    {
      // set selection to the end of what we just pasted.
      nsCOMPtr<nsIDOMNode> selNode, tmp, visNode, highTable;
      PRInt32 selOffset;
      
      // but dont cross tables
      if (!nsHTMLEditUtils::IsTable(lastInsertNode))
      {
        res = GetLastEditableLeaf(lastInsertNode, address_of(selNode));
        if (NS_FAILED(res)) return res;
        tmp = selNode;
        while (tmp && (tmp != lastInsertNode))
        {
          if (nsHTMLEditUtils::IsTable(tmp))
            highTable = tmp;
          nsCOMPtr<nsIDOMNode> parent = tmp;
          tmp->GetParentNode(getter_AddRefs(parent));
          tmp = parent;
        }
        if (highTable)
          selNode = highTable;
      }
      if (!selNode) 
        selNode = lastInsertNode;
      if (IsTextNode(selNode) || (IsContainer(selNode) && !nsHTMLEditUtils::IsTable(selNode)))  
      {
        res = GetLengthOfDOMNode(selNode, (PRUint32&)selOffset);
        if (NS_FAILED(res)) return res;
      }
      else // we need to find a container for selection.  Look up.
      {
        tmp = selNode;
        res = GetNodeLocation(tmp, address_of(selNode), &selOffset);
        ++selOffset;  // want to be *after* last leaf node in paste
        if (NS_FAILED(res)) return res;
      }
      
      // make sure we dont end up with selection collapsed after an invisible break node
      nsWSRunObject wsRunObj(this, selNode, selOffset);
      PRInt32 outVisOffset=0;
      PRInt16 visType=0;
      res = wsRunObj.PriorVisibleNode(selNode, selOffset, address_of(visNode), &outVisOffset, &visType);
      if (NS_FAILED(res)) return res;
      if (visType == nsWSRunObject::eBreak)
      {
        // we are after a break.  Is it visible?  Despite the name, 
        // PriorVisibleNode does not make that determination for breaks.
        // It also may not return the break in visNode.  We have to pull it
        // out of the nsWSRunObject's state.
        if (!IsVisBreak(wsRunObj.mStartReasonNode))
        {
          // dont leave selection past an invisible break;
          // reset {selNode,selOffset} to point before break
          res = GetNodeLocation(wsRunObj.mStartReasonNode, address_of(selNode), &selOffset);
          // we want to be inside any inline style prior to break
          nsWSRunObject wsRunObj(this, selNode, selOffset);
          res = wsRunObj.PriorVisibleNode(selNode, selOffset, address_of(visNode), &outVisOffset, &visType);
          if (NS_FAILED(res)) return res;
          if (visType == nsWSRunObject::eText ||
              visType == nsWSRunObject::eNormalWS)
          {
            selNode = visNode;
            selOffset = outVisOffset;  // PriorVisibleNode already set offset to _after_ the text or ws
          }
          else if (visType == nsWSRunObject::eSpecial)
          {
            // prior visible thing is an image or some other non-text thingy.  
            // We want to be right after it.
            res = GetNodeLocation(wsRunObj.mStartReasonNode, address_of(selNode), &selOffset);
            ++selOffset;
          }
        }
      }
      selection->Collapse(selNode, selOffset);
      
      // if we just pasted a link, discontinue link style
      nsCOMPtr<nsIDOMNode> link;
      if (!bStartedInLink && IsInLink(selNode, address_of(link)))
      {
        // so, if we just pasted a link, I split it.  Why do that instead of just
        // nudging selection point beyond it?  Because it might have ended in a BR
        // that is not visible.  If so, the code above just placed selection
        // inside that.  So I split it instead.
        nsCOMPtr<nsIDOMNode> leftLink;
        PRInt32 linkOffset;
        res = SplitNodeDeep(link, selNode, selOffset, &linkOffset, PR_TRUE, address_of(leftLink));
        if (NS_FAILED(res)) return res;
        res = GetNodeLocation(leftLink, address_of(selNode), &selOffset);
        if (NS_FAILED(res)) return res;
        selection->Collapse(selNode, selOffset+1);
      }
    }
  }
  
  res = mRules->DidDoAction(selection, &ruleInfo, res);
  return res;
}


PRBool
nsHTMLEditor::IsInLink(nsIDOMNode *aNode, nsCOMPtr<nsIDOMNode> *outLink)
{
  if (!aNode) 
    return PR_FALSE;
  if (outLink)
    *outLink = nsnull;
  nsCOMPtr<nsIDOMNode> tmp, node = aNode;
  while (node)
  {
    if (nsHTMLEditUtils::IsLink(node)) 
    {
      if (outLink)
        *outLink = node;
      return PR_TRUE;
    }
    tmp = node;
    tmp->GetParentNode(getter_AddRefs(node));
  }
  return PR_FALSE;
}


nsresult
nsHTMLEditor::StripFormattingNodes(nsIDOMNode *aNode, PRBool aListOnly)
{
  NS_ENSURE_TRUE(aNode, NS_ERROR_NULL_POINTER);

  nsresult res = NS_OK;
  nsCOMPtr<nsIContent> content = do_QueryInterface(aNode);
  if (IsEmptyTextContent(content))
  {
    nsCOMPtr<nsIDOMNode> parent, ignored;
    aNode->GetParentNode(getter_AddRefs(parent));
    if (parent)
    {
      if (!aListOnly || nsHTMLEditUtils::IsList(parent))
        res = parent->RemoveChild(aNode, getter_AddRefs(ignored));
      return res;
    }
  }
  
  if (!nsHTMLEditUtils::IsPre(aNode))
  {
    nsCOMPtr<nsIDOMNode> child;
    aNode->GetLastChild(getter_AddRefs(child));
  
    while (child)
    {
      nsCOMPtr<nsIDOMNode> tmp;
      child->GetPreviousSibling(getter_AddRefs(tmp));
      res = StripFormattingNodes(child, aListOnly);
      NS_ENSURE_SUCCESS(res, res);
      child = tmp;
    }
  }
  return res;
}

NS_IMETHODIMP nsHTMLEditor::PrepareTransferable(nsITransferable **transferable)
{
  // Create generic Transferable for getting the data
  nsresult rv = nsComponentManager::CreateInstance(kCTransferableCID, nsnull, 
                                          NS_GET_IID(nsITransferable), 
                                          (void**)transferable);
  if (NS_FAILED(rv))
    return rv;

  // Get the nsITransferable interface for getting the data from the clipboard
  if (transferable)
  {
    // Create the desired DataFlavor for the type of data
    // we want to get out of the transferable
    if ((mFlags & eEditorPlaintextMask) == 0)  // This should only happen in html editors, not plaintext
    {
      (*transferable)->AddDataFlavor(kHTMLMime);
      (*transferable)->AddDataFlavor(kFileMime);
      //(*transferable)->AddDataFlavor(kJPEGImageMime);
    }
    (*transferable)->AddDataFlavor(kUnicodeMime);
  }
  
  return NS_OK;
}

NS_IMETHODIMP nsHTMLEditor::InsertFromTransferable(nsITransferable *transferable, 
                                                   const nsAString & aContextStr,
                                                   const nsAString & aInfoStr)
{
  nsresult rv = NS_OK;
  char* bestFlavor = nsnull;
  nsCOMPtr<nsISupports> genericDataObj;
  PRUint32 len = 0;
  if ( NS_SUCCEEDED(transferable->GetAnyTransferData(&bestFlavor, getter_AddRefs(genericDataObj), &len)) )
  {
    nsAutoTxnsConserveSelection dontSpazMySelection(this);
    nsAutoString flavor, stuffToPaste;
    flavor.AssignWithConversion( bestFlavor );   // just so we can use flavor.Equals()
#ifdef DEBUG_clipboard
    printf("Got flavor [%s]\n", bestFlavor);
#endif
    if (flavor.Equals(NS_LITERAL_STRING(kHTMLMime)))
    {
      nsCOMPtr<nsISupportsString> textDataObj ( do_QueryInterface(genericDataObj) );
      if (textDataObj && len > 0)
      {
        nsAutoString text;

        textDataObj->GetData ( text );

        NS_ASSERTION(text.Length() <= (len/2), "Invalid length!");
        stuffToPaste.Assign ( text.get(), len / 2 );
        nsAutoEditBatch beginBatching(this);
        rv = InsertHTMLWithContext(stuffToPaste, aContextStr, aInfoStr);
      }
    }
    else if (flavor.Equals(NS_LITERAL_STRING(kUnicodeMime)))
    {
      nsCOMPtr<nsISupportsString> textDataObj ( do_QueryInterface(genericDataObj) );
      if (textDataObj && len > 0)
      {
        nsAutoString text;
        textDataObj->GetData ( text );
        NS_ASSERTION(text.Length() <= (len/2), "Invalid length!");
        stuffToPaste.Assign ( text.get(), len / 2 );
        nsAutoEditBatch beginBatching(this);
        // pasting does not inherit local inline styles
        RemoveAllInlineProperties();
        rv = InsertText(stuffToPaste);
      }
    }
    else if (flavor.Equals(NS_LITERAL_STRING(kFileMime)))
    {
      nsCOMPtr<nsIFile> fileObj ( do_QueryInterface(genericDataObj) );
      if (fileObj && len > 0)
      {
        
        nsCOMPtr<nsIURI> uri;
        rv = NS_NewFileURI(getter_AddRefs(uri), fileObj);
        if (NS_FAILED(rv))
          return rv;
        
        nsCOMPtr<nsIURL> fileURL(do_QueryInterface(uri));
        if ( fileURL )
        {
          PRBool insertAsImage = PR_FALSE;
          nsCAutoString fileextension;
          rv = fileURL->GetFileExtension( fileextension );
          if ( NS_SUCCEEDED(rv) && !fileextension.IsEmpty() )
          {
            if ( (nsCRT::strcasecmp( fileextension.get(), "jpg" ) == 0 )
              || (nsCRT::strcasecmp( fileextension.get(), "jpeg" ) == 0 )
              || (nsCRT::strcasecmp( fileextension.get(), "gif" ) == 0 )
              || (nsCRT::strcasecmp( fileextension.get(), "png" ) == 0 ) )
            {
              insertAsImage = PR_TRUE;
            }
          }
          
          nsCAutoString urltext;
          rv = fileURL->GetSpec( urltext );
          if ( NS_SUCCEEDED(rv) && !urltext.IsEmpty() )
          {
            if ( insertAsImage )
            {
              stuffToPaste.Assign(NS_LITERAL_STRING("<IMG src=\""));
              stuffToPaste.Append(NS_ConvertUTF8toUCS2(urltext));
              stuffToPaste.Append(NS_LITERAL_STRING("\" alt=\"\" >"));
            }
            else /* insert as link */
            {
              stuffToPaste.Assign(NS_LITERAL_STRING("<A href=\""));
              stuffToPaste.Append(NS_ConvertUTF8toUCS2(urltext));
              stuffToPaste.Append(NS_LITERAL_STRING("\">"));
              stuffToPaste.Append(NS_ConvertUTF8toUCS2(urltext));
              stuffToPaste.Append(NS_LITERAL_STRING("</A>"));
            }
            nsAutoEditBatch beginBatching(this);
            rv = InsertHTML(stuffToPaste);
          }
        }
      }
    }
    else if (flavor.Equals(NS_LITERAL_STRING(kJPEGImageMime)))
    {
      // Insert Image code here
      printf("Don't know how to insert an image yet!\n");
      //nsIImage* image = (nsIImage *)data;
      //NS_RELEASE(image);
      rv = NS_ERROR_NOT_IMPLEMENTED; // for now give error code
    }
  }
  nsCRT::free(bestFlavor);
      
  // Try to scroll the selection into view if the paste/drop succeeded
  if (NS_SUCCEEDED(rv))
    ScrollSelectionIntoView(PR_FALSE);

  return rv;
}

NS_IMETHODIMP nsHTMLEditor::InsertFromDrop(nsIDOMEvent* aDropEvent)
{
  ForceCompositionEnd();
  
  nsresult rv;
  nsCOMPtr<nsIDragService> dragService = 
           do_GetService("@mozilla.org/widget/dragservice;1", &rv);
  if (NS_FAILED(rv)) return rv;

  nsCOMPtr<nsIDragSession> dragSession;
  dragService->GetCurrentSession(getter_AddRefs(dragSession)); 
  if (!dragSession) return NS_OK;

  // Get the nsITransferable interface for getting the data from the drop
  nsCOMPtr<nsITransferable> trans;
  rv = PrepareTransferable(getter_AddRefs(trans));
  if (NS_FAILED(rv)) return rv;
  if (!trans) return NS_OK;  // NS_ERROR_FAILURE; SHOULD WE FAIL?

  PRUint32 numItems = 0; 
  rv = dragSession->GetNumDropItems(&numItems);
  if (NS_FAILED(rv)) return rv;

  // Combine any deletion and drop insertion into one transaction
  nsAutoEditBatch beginBatching(this);

  PRUint32 i; 
  PRBool doPlaceCaret = PR_TRUE;
  for (i = 0; i < numItems; ++i)
  {
    rv = dragSession->GetData(trans, i);
    if (NS_FAILED(rv)) return rv;
    if (!trans) return NS_OK; // NS_ERROR_FAILURE; Should we fail?

    // get additional html copy hints, if present
    nsAutoString contextStr, infoStr;
    nsCOMPtr<nsISupports> contextDataObj, infoDataObj;
    PRUint32 contextLen, infoLen;
    nsCOMPtr<nsISupportsString> textDataObj;
    
    nsCOMPtr<nsITransferable> contextTrans = do_CreateInstance(kCTransferableCID);
    NS_ENSURE_TRUE(contextTrans, NS_ERROR_NULL_POINTER);
    contextTrans->AddDataFlavor(kHTMLContext);
    dragSession->GetData(contextTrans, i);
    contextTrans->GetTransferData(kHTMLContext, getter_AddRefs(contextDataObj), &contextLen);

    nsCOMPtr<nsITransferable> infoTrans = do_CreateInstance(kCTransferableCID);
    NS_ENSURE_TRUE(infoTrans, NS_ERROR_NULL_POINTER);
    infoTrans->AddDataFlavor(kHTMLInfo);
    dragSession->GetData(infoTrans, i);
    infoTrans->GetTransferData(kHTMLInfo, getter_AddRefs(infoDataObj), &infoLen);
    
    if (contextDataObj)
    {
      nsAutoString text;
      textDataObj = do_QueryInterface(contextDataObj);
      textDataObj->GetData ( text );
      NS_ASSERTION(text.Length() <= (contextLen/2), "Invalid length!");
      contextStr.Assign ( text.get(), contextLen / 2 );
    }
    
    if (infoDataObj)
    {
      nsAutoString text;
      textDataObj = do_QueryInterface(infoDataObj);
      textDataObj->GetData ( text );
      NS_ASSERTION(text.Length() <= (infoLen/2), "Invalid length!");
      infoStr.Assign ( text.get(), infoLen / 2 );
    }

    if ( doPlaceCaret )
    {
      // check if the user pressed the key to force a copy rather than a move
      // if we run into problems here, we'll just assume the user doesn't want a copy
      PRBool userWantsCopy = PR_FALSE;

      nsCOMPtr<nsIDOMNSUIEvent> nsuiEvent (do_QueryInterface(aDropEvent));
      if (!nsuiEvent) return NS_ERROR_FAILURE;

      nsCOMPtr<nsIDOMMouseEvent> mouseEvent ( do_QueryInterface(aDropEvent) );
      if (mouseEvent)

#ifdef XP_MAC
        mouseEvent->GetAltKey(&userWantsCopy);
#else
        mouseEvent->GetCtrlKey(&userWantsCopy);
#endif
      // Source doc is null if source is *not* the current editor document
      nsCOMPtr<nsIDOMDocument> srcdomdoc;
      rv = dragSession->GetSourceDocument(getter_AddRefs(srcdomdoc));
      if (NS_FAILED(rv)) return rv;

      // Current doc is destination
      nsCOMPtr<nsIDOMDocument>destdomdoc; 
      rv = GetDocument(getter_AddRefs(destdomdoc)); 
      if (NS_FAILED(rv)) return rv;

      nsCOMPtr<nsISelection> selection;
      rv = GetSelection(getter_AddRefs(selection));
      if (NS_FAILED(rv)) return rv;
      if (!selection) return NS_ERROR_FAILURE;

      PRBool isCollapsed;
      rv = selection->GetIsCollapsed(&isCollapsed);
      if (NS_FAILED(rv)) return rv;
      
      // Parent and offset under the mouse cursor
      nsCOMPtr<nsIDOMNode> newSelectionParent;
      PRInt32 newSelectionOffset = 0;
      rv = nsuiEvent->GetRangeParent(getter_AddRefs(newSelectionParent));
      if (NS_FAILED(rv)) return rv;
      if (!newSelectionParent) return NS_ERROR_FAILURE;

      rv = nsuiEvent->GetRangeOffset(&newSelectionOffset);
      if (NS_FAILED(rv)) return rv;
      /* Creating a range to store insert position because when
         we delete the selection, range gravity will make sure the insertion
         point is in the correct place */
      nsCOMPtr<nsIDOMRange> destinationRange;
      rv = CreateRange(newSelectionParent, newSelectionOffset,newSelectionParent, newSelectionOffset, getter_AddRefs(destinationRange));
      if (NS_FAILED(rv))
        return rv;
      if(!destinationRange)
        return NS_ERROR_FAILURE;

      // We never have to delete if selection is already collapsed
      PRBool deleteSelection = PR_FALSE;
      PRBool cursorIsInSelection = PR_FALSE;

      // Check if mouse is in the selection
      if (!isCollapsed)
      {
        PRInt32 rangeCount;
        rv = selection->GetRangeCount(&rangeCount);
        if (NS_FAILED(rv)) 
          return rv;

        for (PRInt32 j = 0; j < rangeCount; j++)
        {
          nsCOMPtr<nsIDOMRange> range;

          rv = selection->GetRangeAt(j, getter_AddRefs(range));
          if (NS_FAILED(rv) || !range) 
            continue;//dont bail yet, iterate through them all

          nsCOMPtr<nsIDOMNSRange> nsrange(do_QueryInterface(range));
          if (NS_FAILED(rv) || !nsrange) 
            continue;//dont bail yet, iterate through them all

          rv = nsrange->IsPointInRange(newSelectionParent, newSelectionOffset, &cursorIsInSelection);
          if(cursorIsInSelection)
            break;
        }
        if (cursorIsInSelection)
        {
          // Dragging within same doc can't drop on itself -- leave!
          // (We shouldn't get here - drag event shouldn't have started if over selection)
          if (srcdomdoc == destdomdoc)
            return NS_OK;
          
          // Dragging from another window onto a selection
          // XXX Decision made to NOT do this,
          //     note that 4.x does replace if dropped on
          //deleteSelection = PR_TRUE;
        }
        else 
        {
          // We are NOT over the selection
          if (srcdomdoc == destdomdoc)
          {
            // Within the same doc: delete if user doesn't want to copy
            deleteSelection = !userWantsCopy;
          }
          else
          {
            // Different source doc: Don't delete
            deleteSelection = PR_FALSE;
          }
        }
      }

      if (deleteSelection)
      {
        rv = DeleteSelection(eNone);
        if (NS_FAILED(rv)) return rv;
      }

      // If we deleted the selection because we dropped from another doc,
      //  then we don't have to relocate the caret (insert at the deletion point)
      if (!(deleteSelection && srcdomdoc != destdomdoc))
      {
        // Move the selection to the point under the mouse cursor
        rv = destinationRange->GetStartContainer(getter_AddRefs(newSelectionParent));
        if (NS_FAILED(rv))
          return rv;
        if(!newSelectionParent)
          return NS_ERROR_FAILURE;
       
        rv = destinationRange->GetStartOffset(&newSelectionOffset);
        if (NS_FAILED(rv))
          return rv;
        selection->Collapse(newSelectionParent, newSelectionOffset);
      }      
      // We have to figure out whether to delete and relocate caret only once
      doPlaceCaret = PR_FALSE;
    }
    
    rv = InsertFromTransferable(trans, contextStr, infoStr);
  }

  return rv;
}

NS_IMETHODIMP nsHTMLEditor::CanDrag(nsIDOMEvent *aDragEvent, PRBool *aCanDrag)
{
  if (!aCanDrag)
    return NS_ERROR_NULL_POINTER;

  /* we really should be checking the XY coordinates of the mouseevent and ensure that
   * that particular point is actually within the selection (not just that there is a selection)
   */
  *aCanDrag = PR_FALSE;
 
  // KLUDGE to work around bug 50703
  // After double click and object property editing, 
  //  we get a spurious drag event
  if (mIgnoreSpuriousDragEvent)
  {
#ifdef DEBUG_cmanske
    printf(" *** IGNORING SPURIOUS DRAG EVENT!\n");
#endif
    mIgnoreSpuriousDragEvent = PR_FALSE;
    return NS_OK;
  }
   
  nsCOMPtr<nsISelection> selection;
  nsresult res = GetSelection(getter_AddRefs(selection));
  if (NS_FAILED(res)) return res;
    
  PRBool isCollapsed;
  res = selection->GetIsCollapsed(&isCollapsed);
  if (NS_FAILED(res)) return res;
  
  // if we are collapsed, we have no selection so nothing to drag
  if ( isCollapsed )
    return NS_OK;

  nsCOMPtr<nsIDOMEventTarget> eventTarget;

  nsCOMPtr<nsIDOMNSEvent> nsevent(do_QueryInterface(aDragEvent));

  if (nsevent) {
    res = nsevent->GetOriginalTarget(getter_AddRefs(eventTarget));
    if (NS_FAILED(res)) {
      return res;
    }
  }

  if ( eventTarget )
  {
    nsCOMPtr<nsIDOMNode> eventTargetDomNode = do_QueryInterface(eventTarget);
    if ( eventTargetDomNode )
    {
      PRBool isTargetedCorrectly = PR_FALSE;
      res = selection->ContainsNode(eventTargetDomNode, PR_FALSE, &isTargetedCorrectly);
      if (NS_FAILED(res)) return res;

      *aCanDrag = isTargetedCorrectly;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP nsHTMLEditor::DoDrag(nsIDOMEvent *aDragEvent)
{
  nsresult rv;

  nsCOMPtr<nsIDOMEventTarget> eventTarget;
  rv = aDragEvent->GetTarget(getter_AddRefs(eventTarget));
  if (NS_FAILED(rv)) return rv;
  nsCOMPtr<nsIDOMNode> domnode = do_QueryInterface(eventTarget);

  /* get the selection to be dragged */
  nsCOMPtr<nsISelection> selection;
  rv = GetSelection(getter_AddRefs(selection));
  if (NS_FAILED(rv)) return rv;

  /* create an array of transferables */
  nsCOMPtr<nsISupportsArray> transferableArray;
  NS_NewISupportsArray(getter_AddRefs(transferableArray));
  if (transferableArray == nsnull)
    return NS_ERROR_OUT_OF_MEMORY;

  /* get the drag service */
  nsCOMPtr<nsIDragService> dragService = 
           do_GetService("@mozilla.org/widget/dragservice;1", &rv);
  if (NS_FAILED(rv)) return rv;

  /* create html flavor transferable */
  nsCOMPtr<nsITransferable> trans = do_CreateInstance(kCTransferableCID);
  NS_ENSURE_TRUE(trans, NS_ERROR_FAILURE);

  nsCOMPtr<nsIDOMDocument> domdoc;
  rv = GetDocument(getter_AddRefs(domdoc));
  if (NS_FAILED(rv)) return rv;
  
  nsCOMPtr<nsIDocument> doc = do_QueryInterface(domdoc);
  if (doc)
  {
    // find out if we're a plaintext control or not
    PRUint32 editorFlags = 0;
    rv = GetFlags(&editorFlags);
    if (NS_FAILED(rv)) return rv;

    PRBool bIsPlainTextControl = ((editorFlags & eEditorPlaintextMask) != 0);
    
    // get correct mimeType and document encoder flags set
    nsAutoString mimeType;
    PRUint32 docEncoderFlags = 0;
    if (bIsPlainTextControl)
    {
      docEncoderFlags |= nsIDocumentEncoder::OutputBodyOnly | nsIDocumentEncoder::OutputPreformatted;
      mimeType = NS_LITERAL_STRING(kUnicodeMime);
    }
    else
      mimeType = NS_LITERAL_STRING(kHTMLMime);
    
    // set up docEncoder
    nsCOMPtr<nsIDocumentEncoder> docEncoder = do_CreateInstance(NS_HTMLCOPY_ENCODER_CONTRACTID);
    NS_ENSURE_TRUE(docEncoder, NS_ERROR_FAILURE);

    rv = docEncoder->Init(doc, mimeType, docEncoderFlags);
    if (NS_FAILED(rv)) return rv;
    
    rv = docEncoder->SetSelection(selection);
    if (NS_FAILED(rv)) return rv;

    // grab a string
    nsAutoString buffer, parents, info;

    if (!bIsPlainTextControl)
    {
      // encode the selection as html with contextual info
      rv = docEncoder->EncodeToStringWithContext(buffer, parents, info);
      if (NS_FAILED(rv)) return rv;
    }
    else
    {
      // encode the selection
      rv = docEncoder->EncodeToString(buffer);
      if (NS_FAILED(rv)) return rv;
    }

    // if we have an empty string, we're done; otherwise continue
    if ( !buffer.IsEmpty() )
    {
      nsCOMPtr<nsISupportsString> dataWrapper, contextWrapper, infoWrapper;

      dataWrapper = do_CreateInstance(NS_SUPPORTS_STRING_CONTRACTID);
      NS_ENSURE_TRUE(dataWrapper, NS_ERROR_FAILURE);
      rv = dataWrapper->SetData(buffer);
      if (NS_FAILED(rv)) return rv;

      if (bIsPlainTextControl)
      {
         // Add the unicode flavor to the transferable
        rv = trans->AddDataFlavor(kUnicodeMime);
        if (NS_FAILED(rv)) return rv;

        // QI the data object an |nsISupports| so that when the transferable holds
        // onto it, it will addref the correct interface.
        nsCOMPtr<nsISupports> genericDataObj ( do_QueryInterface(dataWrapper) );
        rv = trans->SetTransferData(kUnicodeMime, genericDataObj, buffer.Length() * 2);
        if (NS_FAILED(rv)) return rv;
      }
      else
      {
        contextWrapper = do_CreateInstance(NS_SUPPORTS_STRING_CONTRACTID);
        NS_ENSURE_TRUE(contextWrapper, NS_ERROR_FAILURE);
        infoWrapper = do_CreateInstance(NS_SUPPORTS_STRING_CONTRACTID);
        NS_ENSURE_TRUE(infoWrapper, NS_ERROR_FAILURE);

        contextWrapper->SetData ( parents );
        infoWrapper->SetData ( info );

        rv = trans->AddDataFlavor(kHTMLMime);
        if (NS_FAILED(rv)) return rv;

        nsCOMPtr<nsIFormatConverter> htmlConverter = do_CreateInstance(kCHTMLFormatConverterCID);
        NS_ENSURE_TRUE(htmlConverter, NS_ERROR_FAILURE);

        rv = trans->SetConverter(htmlConverter);
        if (NS_FAILED(rv)) return rv;

        nsCOMPtr<nsISupports> genericDataObj ( do_QueryInterface(dataWrapper) );
        rv = trans->SetTransferData(kHTMLMime, genericDataObj, buffer.Length() * 2);
        if (NS_FAILED(rv)) return rv;

        if (parents.Length())
        {
          // Add the htmlcontext DataFlavor to the transferable
          trans->AddDataFlavor(kHTMLContext);
          genericDataObj = do_QueryInterface(contextWrapper);
          trans->SetTransferData(kHTMLContext, genericDataObj, parents.Length()*2);
        }
        if (info.Length())
        {
          // Add the htmlinfo DataFlavor to the transferable
          trans->AddDataFlavor(kHTMLInfo);
          genericDataObj = do_QueryInterface(infoWrapper);
          trans->SetTransferData(kHTMLInfo, genericDataObj, info.Length()*2);
        }
      }

      /* add the transferable to the array */
      rv = transferableArray->AppendElement(trans);
      if (NS_FAILED(rv)) return rv;

      /* invoke drag */
      unsigned int flags;
      // in some cases we'll want to cut rather than copy... hmmmmm...
      flags = nsIDragService::DRAGDROP_ACTION_COPY + nsIDragService::DRAGDROP_ACTION_MOVE;
      
      rv = dragService->InvokeDragSession( domnode, transferableArray, nsnull, flags);
      if (NS_FAILED(rv)) return rv;

      nsCOMPtr<nsIDOMNSEvent> nsevent(do_QueryInterface(aDragEvent));

      if (nsevent) {
        nsevent->PreventBubble();
      }
    }
  }

  return rv;
}

NS_IMETHODIMP nsHTMLEditor::Paste(PRInt32 aSelectionType)
{
  ForceCompositionEnd();

  // Get Clipboard Service
  nsresult rv;
  nsCOMPtr<nsIClipboard> clipboard( do_GetService( kCClipboardCID, &rv ) );
  if ( NS_FAILED(rv) )
    return rv;
    
  // Get the nsITransferable interface for getting the data from the clipboard
  nsCOMPtr<nsITransferable> trans;
  rv = PrepareTransferable(getter_AddRefs(trans));
  if (NS_SUCCEEDED(rv) && trans)
  {
    // Get the Data from the clipboard  
    if (NS_SUCCEEDED(clipboard->GetData(trans, aSelectionType)) && IsModifiable())
    {
      // also get additional html copy hints, if present
      nsAutoString contextStr, infoStr;
      nsCOMPtr<nsISupports> contextDataObj, infoDataObj;
      PRUint32 contextLen, infoLen;
      nsCOMPtr<nsISupportsString> textDataObj;
      
      nsCOMPtr<nsITransferable> contextTrans = do_CreateInstance(kCTransferableCID);
      NS_ENSURE_TRUE(contextTrans, NS_ERROR_NULL_POINTER);
      contextTrans->AddDataFlavor(kHTMLContext);
      clipboard->GetData(contextTrans, aSelectionType);
      contextTrans->GetTransferData(kHTMLContext, getter_AddRefs(contextDataObj), &contextLen);

      nsCOMPtr<nsITransferable> infoTrans = do_CreateInstance(kCTransferableCID);
      NS_ENSURE_TRUE(infoTrans, NS_ERROR_NULL_POINTER);
      infoTrans->AddDataFlavor(kHTMLInfo);
      clipboard->GetData(infoTrans, aSelectionType);
      infoTrans->GetTransferData(kHTMLInfo, getter_AddRefs(infoDataObj), &infoLen);
      
      if (contextDataObj)
      {
        nsAutoString text;
        textDataObj = do_QueryInterface(contextDataObj);
        textDataObj->GetData ( text );
        NS_ASSERTION(text.Length() <= (contextLen/2), "Invalid length!");
        contextStr.Assign ( text.get(), contextLen / 2 );
      }
      
      if (infoDataObj)
      {
        nsAutoString text;
        textDataObj = do_QueryInterface(infoDataObj);
        textDataObj->GetData ( text );
        NS_ASSERTION(text.Length() <= (infoLen/2), "Invalid length!");
        infoStr.Assign ( text.get(), infoLen / 2 );
      }
      rv = InsertFromTransferable(trans, contextStr, infoStr);
    }
  }

  return rv;
}

// 
// HTML PasteNoFormatting. Ignore any HTML styles and formating in paste source
//
NS_IMETHODIMP nsHTMLEditor::PasteNoFormatting(PRInt32 aSelectionType)
{
  ///XXX Joe Francis will write this part: bug 64647
  return Paste(aSelectionType);
}

NS_IMETHODIMP nsHTMLEditor::CanPaste(PRInt32 aSelectionType, PRBool *aCanPaste)
{
  if (!aCanPaste)
    return NS_ERROR_NULL_POINTER;
  *aCanPaste = PR_FALSE;
  
  // can't paste if readonly
  if (!IsModifiable())
    return NS_OK;
    
  nsresult rv;
  nsCOMPtr<nsIClipboard> clipboard(do_GetService(kCClipboardCID, &rv));
  if (NS_FAILED(rv)) return rv;
  
  // the flavors that we can deal with
  const char* const textEditorFlavors[] = { kUnicodeMime, nsnull };
  const char* const htmlEditorFlavors[] = { kHTMLMime, kJPEGImageMime, nsnull };

  nsCOMPtr<nsISupportsArray> flavorsList;
  rv = nsComponentManager::CreateInstance(NS_SUPPORTSARRAY_CONTRACTID, nsnull, 
         NS_GET_IID(nsISupportsArray), getter_AddRefs(flavorsList));
  if (NS_FAILED(rv)) return rv;
  
  PRUint32 editorFlags;
  GetFlags(&editorFlags);
  
  // add the flavors for all editors
  for (const char* const* flavor = textEditorFlavors; *flavor; flavor++)
  {
    nsCOMPtr<nsISupportsCString> flavorString;            
    nsComponentManager::CreateInstance(NS_SUPPORTS_CSTRING_CONTRACTID, nsnull, 
         NS_GET_IID(nsISupportsCString), getter_AddRefs(flavorString));
    if (flavorString)
    {
      flavorString->SetData(nsDependentCString(*flavor));
      flavorsList->AppendElement(flavorString);
    }
  }
  
  // add the HTML-editor only flavors
  if ((editorFlags & eEditorPlaintextMask) == 0)
  {
    for (const char* const* htmlFlavor = htmlEditorFlavors;
         *htmlFlavor;
         htmlFlavor++)
    {
      nsCOMPtr<nsISupportsCString> flavorString;            
      nsComponentManager::CreateInstance(NS_SUPPORTS_CSTRING_CONTRACTID, nsnull, 
           NS_GET_IID(nsISupportsCString), getter_AddRefs(flavorString));
      if (flavorString)
      {
        flavorString->SetData(nsDependentCString(*htmlFlavor));
        flavorsList->AppendElement(flavorString);
      }
    }
  }
  
  PRBool haveFlavors;
  rv = clipboard->HasDataMatchingFlavors(flavorsList, aSelectionType, &haveFlavors);
  if (NS_FAILED(rv)) return rv;
  
  *aCanPaste = haveFlavors;
  return NS_OK;
}


// 
// HTML PasteAsQuotation: Paste in a blockquote type=cite
//
NS_IMETHODIMP nsHTMLEditor::PasteAsQuotation(PRInt32 aSelectionType)
{
  if (mFlags & eEditorPlaintextMask)
    return PasteAsPlaintextQuotation(aSelectionType);

  nsAutoString citation;
  return PasteAsCitedQuotation(citation, aSelectionType);
}

NS_IMETHODIMP nsHTMLEditor::PasteAsCitedQuotation(const nsAString & aCitation,
                                                  PRInt32 aSelectionType)
{
  nsAutoEditBatch beginBatching(this);
  nsAutoRules beginRulesSniffing(this, kOpInsertQuotation, nsIEditor::eNext);

  // get selection
  nsCOMPtr<nsISelection> selection;
  nsresult res = GetSelection(getter_AddRefs(selection));
  if (NS_FAILED(res)) return res;
  if (!selection) return NS_ERROR_NULL_POINTER;

  // give rules a chance to handle or cancel
  nsTextRulesInfo ruleInfo(nsTextEditRules::kInsertElement);
  PRBool cancel, handled;
  res = mRules->WillDoAction(selection, &ruleInfo, &cancel, &handled);
  if (NS_FAILED(res)) return res;
  if (cancel) return NS_OK; // rules canceled the operation
  if (!handled)
  {
    nsCOMPtr<nsIDOMNode> newNode;
    res = DeleteSelectionAndCreateNode(NS_LITERAL_STRING("blockquote"), getter_AddRefs(newNode));
    if (NS_FAILED(res)) return res;
    if (!newNode) return NS_ERROR_NULL_POINTER;

    // Try to set type=cite.  Ignore it if this fails.
    nsCOMPtr<nsIDOMElement> newElement (do_QueryInterface(newNode));
    if (newElement)
    {
      newElement->SetAttribute(NS_LITERAL_STRING("type"), NS_LITERAL_STRING("cite"));
    }

    // Set the selection to the underneath the node we just inserted:
    res = selection->Collapse(newNode, 0);
    if (NS_FAILED(res))
    {
#ifdef DEBUG_akkana
      printf("Couldn't collapse");
#endif
      // XXX: error result:  should res be returned here?
    }

    res = Paste(aSelectionType);
  }
  return res;
}

//
// Paste a plaintext quotation
//
NS_IMETHODIMP nsHTMLEditor::PasteAsPlaintextQuotation(PRInt32 aSelectionType)
{
  // Get Clipboard Service
  nsresult rv;
  nsCOMPtr<nsIClipboard> clipboard(do_GetService(kCClipboardCID, &rv));
  if (NS_FAILED(rv)) return rv;

  // Create generic Transferable for getting the data
  nsCOMPtr<nsITransferable> trans;
  rv = nsComponentManager::CreateInstance(kCTransferableCID, nsnull, 
                                          NS_GET_IID(nsITransferable), 
                                          (void**) getter_AddRefs(trans));
  if (NS_SUCCEEDED(rv) && trans)
  {
    // We only handle plaintext pastes here
    trans->AddDataFlavor(kUnicodeMime);

    // Get the Data from the clipboard
    clipboard->GetData(trans, aSelectionType);

    // Now we ask the transferable for the data
    // it still owns the data, we just have a pointer to it.
    // If it can't support a "text" output of the data the call will fail
    nsCOMPtr<nsISupports> genericDataObj;
    PRUint32 len = 0;
    char* flav = 0;
    rv = trans->GetAnyTransferData(&flav, getter_AddRefs(genericDataObj),
                                   &len);
    if (NS_FAILED(rv))
    {
#ifdef DEBUG_akkana
      printf("PasteAsPlaintextQuotation: GetAnyTransferData failed, %d\n", rv);
#endif
      return rv;
    }
#ifdef DEBUG_clipboard
    printf("Got flavor [%s]\n", flav);
#endif
    nsAutoString flavor; flavor.AssignWithConversion(flav);
    nsAutoString stuffToPaste;
    if (flavor.Equals(NS_LITERAL_STRING(kUnicodeMime)))
    {
      nsCOMPtr<nsISupportsString> textDataObj ( do_QueryInterface(genericDataObj) );
      if (textDataObj && len > 0)
      {
        nsAutoString text;
        textDataObj->GetData ( text );
        NS_ASSERTION(text.Length() <= (len/2), "Invalid length!");
        stuffToPaste.Assign ( text.get(), len / 2 );
        nsAutoEditBatch beginBatching(this);
        rv = InsertAsPlaintextQuotation(stuffToPaste, PR_TRUE, 0);
      }
    }
    nsCRT::free(flav);
  }

  return rv;
}

NS_IMETHODIMP
nsHTMLEditor::InsertTextWithQuotations(const nsAString &aStringToInsert)
{
  if (mWrapToWindow)
    return InsertText(aStringToInsert);

  // The whole operation should be undoable in one transaction:
  BeginTransaction();

  // We're going to loop over the string, collecting up a "hunk"
  // that's all the same type (quoted or not),
  // Whenever the quotedness changes (or we reach the string's end)
  // we will insert the hunk all at once, quoted or non.

  static const PRUnichar cite('>');
  PRBool curHunkIsQuoted = (aStringToInsert.First() == cite);

  nsAString::const_iterator hunkStart, strEnd;
  aStringToInsert.BeginReading(hunkStart);
  aStringToInsert.EndReading(strEnd);

  // In the loop below, we only look for DOM newlines (\n),
  // because we don't have a FindChars method that can look
  // for both \r and \n.  \r is illegal in the dom anyway,
  // but in debug builds, let's take the time to verify that
  // there aren't any there:
#ifdef DEBUG
  nsAString::const_iterator dbgStart (hunkStart);
  if (FindCharInReadable('\r', dbgStart, strEnd))
    NS_ASSERTION(PR_FALSE,
            "Return characters in DOM! InsertTextWithQuotations may be wrong");
#endif /* DEBUG */

  // Loop over lines:
  nsresult rv = NS_OK;
  nsAString::const_iterator lineStart (hunkStart);
  while (1)   // we will break from inside when we run out of newlines
  {
    // Search for the end of this line (dom newlines, see above):
    PRBool found = FindCharInReadable('\n', lineStart, strEnd);
    PRBool quoted = PR_FALSE;
    if (found)
    {
      // if there's another newline, lineStart now points there.
      // Loop over any consecutive newline chars:
      nsAString::const_iterator firstNewline (lineStart);
      while (*lineStart == '\n')
        ++lineStart;
      quoted = (*lineStart == cite);
      if (quoted == curHunkIsQuoted)
        continue;
      // else we're changing state, so we need to insert
      // from curHunk to lineStart then loop around.

      // But if the current hunk is quoted, then we want to make sure
      // that any extra newlines on the end do not get included in
      // the quoted section: blank lines flaking a quoted section
      // should be considered unquoted, so that if the user clicks
      // there and starts typing, the new text will be outside of
      // the quoted block.
      if (curHunkIsQuoted)
        lineStart = firstNewline;
    }

    // If no newline found, lineStart is now strEnd and we can finish up,
    // inserting from curHunk to lineStart then returning.
    const nsAString &curHunk = Substring(hunkStart, lineStart);
    nsCOMPtr<nsIDOMNode> dummyNode;
#ifdef DEBUG_akkana_verbose
    printf("==== Inserting text as %squoted: ---\n%s---\n",
           curHunkIsQuoted ? "" : "non-",
           NS_LossyConvertUCS2toASCII(curHunk).get());
#endif
    if (curHunkIsQuoted)
      rv = InsertAsPlaintextQuotation(curHunk, PR_FALSE,
                                      getter_AddRefs(dummyNode));
    else
      rv = InsertText(curHunk);

    if (!found)
      break;

    curHunkIsQuoted = quoted;
    hunkStart = lineStart;
  }

  EndTransaction();

  return rv;
}

NS_IMETHODIMP nsHTMLEditor::InsertAsQuotation(const nsAString & aQuotedText,
                                              nsIDOMNode **aNodeInserted)
{
  if (mFlags & eEditorPlaintextMask)
    return InsertAsPlaintextQuotation(aQuotedText, PR_TRUE, aNodeInserted);

  nsAutoString citation;
  nsAutoString charset;
  return InsertAsCitedQuotation(aQuotedText, citation, PR_FALSE,
                                charset, aNodeInserted);
}

// Insert plaintext as a quotation, with cite marks (e.g. "> ").
// This differs from its corresponding method in nsPlaintextEditor
// in that here, quoted material is enclosed in a <pre> tag
// in order to preserve the original line wrapping.
NS_IMETHODIMP
nsHTMLEditor::InsertAsPlaintextQuotation(const nsAString & aQuotedText,
                                         PRBool aAddCites,
                                         nsIDOMNode **aNodeInserted)
{
  if (mWrapToWindow)
    return nsPlaintextEditor::InsertAsQuotation(aQuotedText, aNodeInserted);

  nsresult rv;

  // The quotesPreformatted pref is a temporary measure. See bug 69638.
  // Eventually we'll pick one way or the other.
  PRBool quotesInPre;
  nsCOMPtr<nsIPref> prefs = do_GetService(kPrefServiceCID, &rv);
  if (NS_SUCCEEDED(rv) && prefs)
    prefs->GetBoolPref("editor.quotesPreformatted", &quotesInPre);

  nsCOMPtr<nsIDOMNode> preNode;
  // get selection
  nsCOMPtr<nsISelection> selection;
  rv = GetSelection(getter_AddRefs(selection));
  if (NS_FAILED(rv)) return rv;
  if (!selection) return NS_ERROR_NULL_POINTER;
  else
  {
    nsAutoEditBatch beginBatching(this);
    nsAutoRules beginRulesSniffing(this, kOpInsertQuotation, nsIEditor::eNext);

    // give rules a chance to handle or cancel
    nsTextRulesInfo ruleInfo(nsTextEditRules::kInsertElement);
    PRBool cancel, handled;
    rv = mRules->WillDoAction(selection, &ruleInfo, &cancel, &handled);
    if (NS_FAILED(rv)) return rv;
    if (cancel) return NS_OK; // rules canceled the operation
    if (!handled)
    {
      // Wrap the inserted quote in a <pre> so it won't be wrapped:
      nsAutoString tag;
      if (quotesInPre)
        tag.Assign(NS_LITERAL_STRING("pre"));
      else
        tag.Assign(NS_LITERAL_STRING("span"));

      rv = DeleteSelectionAndCreateNode(tag, getter_AddRefs(preNode));
      
      // If this succeeded, then set selection inside the pre
      // so the inserted text will end up there.
      // If it failed, we don't care what the return value was,
      // but we'll fall through and try to insert the text anyway.
      if (NS_SUCCEEDED(rv) && preNode)
      {
        // Add an attribute on the pre node so we'll know it's a quotation.
        // Do this after the insertion, so that 
        nsCOMPtr<nsIDOMElement> preElement (do_QueryInterface(preNode));
        if (preElement)
        {
          preElement->SetAttribute(NS_LITERAL_STRING("_moz_quote"),
                                   NS_LITERAL_STRING("true"));
          if (quotesInPre)
          {
            // set style to not have unwanted vertical margins
            preElement->SetAttribute(NS_LITERAL_STRING("style"),
                                     NS_LITERAL_STRING("margin: 0 0 0 0px;"));
          }
          else
          {
            // turn off wrapping on spans
            preElement->SetAttribute(NS_LITERAL_STRING("style"),
                                     NS_LITERAL_STRING("white-space: pre;"));
          }
        }

        // and set the selection inside it:
        selection->Collapse(preNode, 0);
      }

      if (aAddCites)
        rv = nsPlaintextEditor::InsertAsQuotation(aQuotedText, aNodeInserted);
      else
        rv = nsPlaintextEditor::InsertText(aQuotedText);
      // Note that if !aAddCites, aNodeInserted isn't set.
      // That's okay because the routines that use aAddCites
      // don't need to know the inserted node.

      if (aNodeInserted && NS_SUCCEEDED(rv))
      {
        *aNodeInserted = preNode;
        NS_IF_ADDREF(*aNodeInserted);
      }
    }
  }
    
  // Set the selection to just after the inserted node:
  if (NS_SUCCEEDED(rv) && preNode)
  {
    nsCOMPtr<nsIDOMNode> parent;
    PRInt32 offset;
    if (NS_SUCCEEDED(GetNodeLocation(preNode, address_of(parent), &offset)) && parent)
      selection->Collapse(parent, offset+1);
  }
  return rv;
}

NS_IMETHODIMP
nsHTMLEditor::InsertAsCitedQuotation(const nsAString & aQuotedText,
                                     const nsAString & aCitation,
                                     PRBool aInsertHTML,
                                     const nsAString & aCharset,
                                     nsIDOMNode **aNodeInserted)
{
  // Don't let anyone insert html into a "plaintext" editor:
  if (mFlags & eEditorPlaintextMask)
  {
    NS_ASSERTION(!aInsertHTML, "InsertAsCitedQuotation: trying to insert html into plaintext editor");
    return InsertAsPlaintextQuotation(aQuotedText, PR_TRUE, aNodeInserted);
  }

  nsCOMPtr<nsIDOMNode> newNode;
  nsresult res = NS_OK;

  // get selection
  nsCOMPtr<nsISelection> selection;
  res = GetSelection(getter_AddRefs(selection));
  if (NS_FAILED(res)) return res;
  if (!selection) return NS_ERROR_NULL_POINTER;
  else
  {
    nsAutoEditBatch beginBatching(this);
    nsAutoRules beginRulesSniffing(this, kOpInsertQuotation, nsIEditor::eNext);

    // give rules a chance to handle or cancel
    nsTextRulesInfo ruleInfo(nsTextEditRules::kInsertElement);
    PRBool cancel, handled;
    res = mRules->WillDoAction(selection, &ruleInfo, &cancel, &handled);
    if (NS_FAILED(res)) return res;
    if (cancel) return NS_OK; // rules canceled the operation
    if (!handled)
    {
      res = DeleteSelectionAndCreateNode(NS_LITERAL_STRING("blockquote"), getter_AddRefs(newNode));
      if (NS_FAILED(res)) return res;
      if (!newNode) return NS_ERROR_NULL_POINTER;

      // Try to set type=cite.  Ignore it if this fails.
      nsCOMPtr<nsIDOMElement> newElement (do_QueryInterface(newNode));
      if (newElement)
      {
        NS_NAMED_LITERAL_STRING(citestr, "cite");
        newElement->SetAttribute(NS_LITERAL_STRING("type"), citestr);

        if (aCitation.Length() > 0)
          newElement->SetAttribute(citestr, aCitation);

        // Set the selection inside the blockquote so aQuotedText will go there:
        selection->Collapse(newNode, 0);
      }

      if (aInsertHTML)
        res = LoadHTMLWithCharset(aQuotedText, aCharset);

      else
        res = InsertText(aQuotedText);  // XXX ignore charset

      if (aNodeInserted)
      {
        if (NS_SUCCEEDED(res))
        {
          *aNodeInserted = newNode;
          NS_IF_ADDREF(*aNodeInserted);
        }
      }
    }
  }

  // Set the selection to just after the inserted node:
  if (NS_SUCCEEDED(res) && newNode)
  {
    nsCOMPtr<nsIDOMNode> parent;
    PRInt32 offset;
    if (NS_SUCCEEDED(GetNodeLocation(newNode, address_of(parent), &offset)) && parent)
      selection->Collapse(parent, offset+1);
  }
  return res;
}

nsresult nsHTMLEditor::CreateDOMFragmentFromPaste(nsIDOMNSRange *aNSRange,
                                                  const nsAString & aInputString,
                                                  const nsAString & aContextStr,
                                                  const nsAString & aInfoStr,
                                                  nsCOMPtr<nsIDOMNode> *outFragNode,
                                                  PRInt32 *outRangeStartHint,
                                                  PRInt32 *outRangeEndHint)
{
  if (!outFragNode || !outRangeStartHint || !outRangeEndHint || !aNSRange) 
    return NS_ERROR_NULL_POINTER;
  nsCOMPtr<nsIDOMDocumentFragment> docfrag;
  nsCOMPtr<nsIDOMNode> contextAsNode;  
  nsresult res = NS_OK;
  
  // if we have context info, create a fragment for that
  nsCOMPtr<nsIDOMDocumentFragment> contextfrag;
  nsCOMPtr<nsIDOMNode> contextLeaf;
  PRInt32 contextDepth = 0;
  if (aContextStr.Length())
  {
    res = aNSRange->CreateContextualFragment(aContextStr, getter_AddRefs(contextfrag));
    NS_ENSURE_SUCCESS(res, res);
    contextAsNode = do_QueryInterface(contextfrag);
    res = StripFormattingNodes(contextAsNode);
    NS_ENSURE_SUCCESS(res, res);
    // cache the deepest leaf in the context
    nsCOMPtr<nsIDOMNode> junk, child, tmp = contextAsNode;
    while (tmp)
    {
      contextDepth++;
      contextLeaf = tmp;
      contextLeaf->GetFirstChild(getter_AddRefs(tmp));
    }
    // tweak aNSRange to point inside contextAsNode
    nsCOMPtr<nsIDOMRange> range(do_QueryInterface(aNSRange));
    if (range)
    {
      aNSRange->NSDetach();
      range->SetStart(contextLeaf,0);
      range->SetEnd(contextLeaf,0);
    }
  }
  
  // create fragment for pasted html
  res = aNSRange->CreateContextualFragment(aInputString, getter_AddRefs(docfrag));
  NS_ENSURE_SUCCESS(res, res);
  *outFragNode = do_QueryInterface(docfrag);
  
  if (contextfrag)
  {
    nsCOMPtr<nsIDOMNode> junk;
    // unite the two trees
    contextLeaf->AppendChild(*outFragNode, getter_AddRefs(junk));
    *outFragNode = contextAsNode;
    // no longer have fragmentAsNode in tree
    contextDepth--;
  }
 
  res = StripFormattingNodes(*outFragNode, PR_TRUE);
  NS_ENSURE_SUCCESS(res, res);
 
  // get the infoString contents
  nsAutoString numstr1, numstr2;
  if (aInfoStr.Length())
  {
    PRInt32 err, sep;
    sep = aInfoStr.FindChar((PRUnichar)',');
    numstr1 = Substring(aInfoStr, 0, sep);
    numstr2 = Substring(aInfoStr, sep+1, aInfoStr.Length() - (sep+1));
    *outRangeStartHint = numstr1.ToInteger(&err) + contextDepth;
    *outRangeEndHint   = numstr2.ToInteger(&err) + contextDepth;
  }
  else
  {
    *outRangeStartHint = contextDepth;
    *outRangeEndHint = contextDepth;
  }
  return res;
}

nsresult nsHTMLEditor::CreateListOfNodesToPaste(nsIDOMNode  *aFragmentAsNode,
                                                nsCOMPtr<nsISupportsArray> *outNodeList,
                                                PRInt32 aRangeStartHint,
                                                PRInt32 aRangeEndHint)
{
  if (!outNodeList || !aFragmentAsNode) 
    return NS_ERROR_NULL_POINTER;

  // First off create a range over the portion of docFrag indicated by
  // the range hints.
  nsCOMPtr<nsIDOMRange> docFragRange;
  docFragRange = do_CreateInstance(kCRangeCID);
  nsCOMPtr<nsIDOMNode> startParent, endParent, tmp;
  PRInt32 endOffset;
  startParent = aFragmentAsNode;
  while (aRangeStartHint > 0)
  {
    startParent->GetFirstChild(getter_AddRefs(tmp));
    startParent = tmp;
    aRangeStartHint--;
    NS_ENSURE_TRUE(startParent, NS_ERROR_FAILURE);
  }
  endParent = aFragmentAsNode;
  while (aRangeEndHint > 0)
  {
    endParent->GetLastChild(getter_AddRefs(tmp));
    endParent = tmp;
    aRangeEndHint--;
    NS_ENSURE_TRUE(endParent, NS_ERROR_FAILURE);
  }
  nsresult res = GetLengthOfDOMNode(endParent, (PRUint32&)endOffset);
  NS_ENSURE_SUCCESS(res, res);

  res = docFragRange->SetStart(startParent, 0);
  NS_ENSURE_SUCCESS(res, res);
  res = docFragRange->SetEnd(endParent, endOffset);
  NS_ENSURE_SUCCESS(res, res);

  // now use a subtree iterator over the range to create a list of nodes
  nsTrivialFunctor functor;
  nsDOMSubtreeIterator iter;
  res = NS_NewISupportsArray(getter_AddRefs(*outNodeList));
  NS_ENSURE_SUCCESS(res, res);
  res = iter.Init(docFragRange);
  NS_ENSURE_SUCCESS(res, res);
  res = iter.AppendList(functor, *outNodeList);

  return res;
}

nsresult 
nsHTMLEditor::GetListAndTableParents(PRBool aEnd, 
                                     nsISupportsArray *aListOfNodes,
                                     nsCOMPtr<nsISupportsArray> *outArray)
{
  NS_ENSURE_TRUE(aListOfNodes, NS_ERROR_NULL_POINTER);
  NS_ENSURE_TRUE(outArray, NS_ERROR_NULL_POINTER);
  
  PRUint32 listCount;
  aListOfNodes->Count(&listCount);
  if (listCount <= 0)
    return NS_ERROR_FAILURE;  // no empty lists, please
    
  // build up list of parents of first (or last) node in list 
  // that are either lists, or tables.  
  PRUint32 idx = 0;
  if (aEnd) idx = listCount-1;
  
  nsCOMPtr<nsISupports> isup = dont_AddRef(aListOfNodes->ElementAt(idx));
  nsCOMPtr<nsIDOMNode>  pNode( do_QueryInterface(isup) );
  nsCOMPtr<nsISupportsArray> listAndTableArray;
  nsresult res = NS_NewISupportsArray(getter_AddRefs(listAndTableArray));
  NS_ENSURE_SUCCESS(res, res);
  while (pNode)
  {
    if (nsHTMLEditUtils::IsList(pNode) || nsHTMLEditUtils::IsTable(pNode))
    {
      isup = do_QueryInterface(pNode);
      listAndTableArray->AppendElement(isup);
    }
    nsCOMPtr<nsIDOMNode> parent;
    pNode->GetParentNode(getter_AddRefs(parent));
    pNode = parent;
  }
  *outArray = listAndTableArray;
  return NS_OK;
}

nsresult
nsHTMLEditor::DiscoverPartialListsAndTables(nsISupportsArray *aPasteNodes,
                                            nsISupportsArray *aListsAndTables,
                                            PRInt32 *outHighWaterMark)
{
  NS_ENSURE_TRUE(aPasteNodes, NS_ERROR_NULL_POINTER);
  NS_ENSURE_TRUE(aListsAndTables, NS_ERROR_NULL_POINTER);
  NS_ENSURE_TRUE(outHighWaterMark, NS_ERROR_NULL_POINTER);
  
  *outHighWaterMark = -1;
  PRUint32 listAndTableParents;
  aListsAndTables->Count(&listAndTableParents);
  
  // scan insertion list for table elements (other than table).
  PRUint32 listCount, j;  
  aPasteNodes->Count(&listCount);
  for (j=0; j<listCount; j++)
  {
    nsCOMPtr<nsISupports> isupports = dont_AddRef(aPasteNodes->ElementAt(j));
    nsCOMPtr<nsIDOMNode> curNode( do_QueryInterface(isupports) );

    NS_ENSURE_TRUE(curNode, NS_ERROR_FAILURE);
    if (nsHTMLEditUtils::IsTableElement(curNode) && !nsHTMLEditUtils::IsTable(curNode))
    {
      nsCOMPtr<nsIDOMNode> theTable = GetTableParent(curNode);
      if (theTable)
      {
        nsCOMPtr<nsISupports> isupTable(do_QueryInterface(theTable));
        PRInt32 indexT = aListsAndTables->IndexOf(isupTable);
        if (indexT >= 0)
        {
          *outHighWaterMark = indexT;
          if ((PRUint32)*outHighWaterMark == listAndTableParents-1) break;
        }
        else
        {
          break;
        }
      }
    }
    if (nsHTMLEditUtils::IsListItem(curNode))
    {
      nsCOMPtr<nsIDOMNode> theList = GetListParent(curNode);
      if (theList)
      {
        nsCOMPtr<nsISupports> isupList(do_QueryInterface(theList));
        PRInt32 indexL = aListsAndTables->IndexOf(isupList);
        if (indexL >= 0)
        {
          *outHighWaterMark = indexL;
          if ((PRUint32)*outHighWaterMark == listAndTableParents-1) break;
        }
        else
        {
          break;
        }
      }
    }
  }
  return NS_OK;
}

nsresult
nsHTMLEditor::ScanForListAndTableStructure( PRBool aEnd,
                                            nsISupportsArray *aNodes,
                                            nsIDOMNode *aListOrTable,
                                            nsCOMPtr<nsIDOMNode> *outReplaceNode)
{
  NS_ENSURE_TRUE(aNodes, NS_ERROR_NULL_POINTER);
  NS_ENSURE_TRUE(aListOrTable, NS_ERROR_NULL_POINTER);
  NS_ENSURE_TRUE(outReplaceNode, NS_ERROR_NULL_POINTER);

  *outReplaceNode = 0;
  
  // look upward from first/last paste node for a piece of this list/table
  PRUint32 listCount, idx = 0;
  aNodes->Count(&listCount);
  if (aEnd) idx = listCount-1;
  PRBool bList = nsHTMLEditUtils::IsList(aListOrTable);
  
  nsCOMPtr<nsISupports> isup  = dont_AddRef(aNodes->ElementAt(idx));
  nsCOMPtr<nsIDOMNode>  pNode = do_QueryInterface(isup);
  nsCOMPtr<nsIDOMNode>  originalNode = pNode;
  while (pNode)
  {
    if ( (bList && nsHTMLEditUtils::IsListItem(pNode)) ||
         (!bList && (nsHTMLEditUtils::IsTableElement(pNode) && !nsHTMLEditUtils::IsTable(pNode))) )
    {
      nsCOMPtr<nsIDOMNode> structureNode;
      if (bList) structureNode = GetListParent(pNode);
      else structureNode = GetTableParent(pNode);
      if (structureNode == aListOrTable)
      {
        if (bList)
          *outReplaceNode = structureNode;
        else
          *outReplaceNode = pNode;
        break;
      }
    }
    nsCOMPtr<nsIDOMNode> parent;
    pNode->GetParentNode(getter_AddRefs(parent));
    pNode = parent;
  }
  return NS_OK;
}    

nsresult
nsHTMLEditor::ReplaceOrphanedStructure(PRBool aEnd,
                                       nsISupportsArray *aNodeArray,
                                       nsISupportsArray *aListAndTableArray,
                                       PRInt32 aHighWaterMark)
{
  NS_ENSURE_TRUE(aNodeArray, NS_ERROR_NULL_POINTER);
  NS_ENSURE_TRUE(aListAndTableArray, NS_ERROR_NULL_POINTER);

  nsCOMPtr<nsISupports> isupports = dont_AddRef(aListAndTableArray->ElementAt(aHighWaterMark));
  nsCOMPtr<nsIDOMNode> curNode( do_QueryInterface(isupports) );
  NS_ENSURE_TRUE(curNode, NS_ERROR_NULL_POINTER);
  
  nsCOMPtr<nsIDOMNode> replaceNode, originalNode, tmp;
  
  // find substructure of list or table that must be included in paste.
  nsresult res = ScanForListAndTableStructure(aEnd, aNodeArray, 
                                 curNode, address_of(replaceNode));
  NS_ENSURE_SUCCESS(res, res);
  
  // if we found substructure, paste it instead of it's descendants
  if (replaceNode)
  {
    // postprocess list to remove any descendants of this node
    // so that we dont insert them twice.
    do
    {
      isupports = GetArrayEndpoint(aEnd, aNodeArray);
      if (!isupports) break;
      tmp = do_QueryInterface(isupports);
      if (tmp && nsHTMLEditUtils::IsDescendantOf(tmp, replaceNode))
        aNodeArray->RemoveElement(isupports);
      else
        break;
    } while(tmp);
    
    // now replace the removed nodes with the structural parent
    isupports = do_QueryInterface(replaceNode);
    if (aEnd) aNodeArray->AppendElement(isupports);
    else aNodeArray->InsertElementAt(isupports, 0);
  }
  return NS_OK;
}

nsISupports* nsHTMLEditor::GetArrayEndpoint(PRBool aEnd, nsISupportsArray *aNodeArray)
{
  if (aEnd)
  {
    PRUint32 listCount;
    aNodeArray->Count(&listCount);
    if (listCount <= 0) return nsnull;
    else return aNodeArray->ElementAt(listCount-1);
  }
  else
  {
    return aNodeArray->ElementAt(0);
  }
}
