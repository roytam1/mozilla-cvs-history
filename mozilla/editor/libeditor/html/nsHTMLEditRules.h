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

#ifndef nsHTMLEditRules_h__
#define nsHTMLEditRules_h__

#include "nsTextEditRules.h"
#include "nsIHTMLEditRules.h"
#include "nsIEditActionListener.h"
#include "nsISupportsArray.h"
#include "nsCOMPtr.h"
#include "nsString.h"

class nsISupportsArray;
class nsVoidArray;
class nsIDOMElement;

class nsHTMLEditRules : public nsIHTMLEditRules, nsTextEditRules, nsIEditActionListener
{
public:

  NS_DECL_ISUPPORTS_INHERITED
  
            nsHTMLEditRules();
  virtual   ~nsHTMLEditRules();


  // nsIEditRules methods
  NS_IMETHOD Init(nsHTMLEditor *aEditor, PRUint32 aFlags);
  NS_IMETHOD BeforeEdit(PRInt32 action, nsIEditor::EDirection aDirection);
  NS_IMETHOD AfterEdit(PRInt32 action, nsIEditor::EDirection aDirection);
  NS_IMETHOD WillDoAction(nsIDOMSelection *aSelection, nsRulesInfo *aInfo, PRBool *aCancel, PRBool *aHandled);
  NS_IMETHOD DidDoAction(nsIDOMSelection *aSelection, nsRulesInfo *aInfo, nsresult aResult);

  // nsIHTMLEditRules methods
  NS_IMETHOD GetListState(PRBool &aMixed, PRBool &aOL, PRBool &aUL);
  NS_IMETHOD GetIndentState(PRBool &aCanIndent, PRBool &aCanOutdent);

  // nsIEditActionListener methods
  
  NS_IMETHOD WillCreateNode(const nsString& aTag, nsIDOMNode *aParent, PRInt32 aPosition);
  NS_IMETHOD DidCreateNode(const nsString& aTag, nsIDOMNode *aNode, nsIDOMNode *aParent, PRInt32 aPosition, nsresult aResult);
  NS_IMETHOD WillInsertNode(nsIDOMNode *aNode, nsIDOMNode *aParent, PRInt32 aPosition);
  NS_IMETHOD DidInsertNode(nsIDOMNode *aNode, nsIDOMNode *aParent, PRInt32 aPosition, nsresult aResult);
  NS_IMETHOD WillDeleteNode(nsIDOMNode *aChild);
  NS_IMETHOD DidDeleteNode(nsIDOMNode *aChild, nsresult aResult);
  NS_IMETHOD WillSplitNode(nsIDOMNode *aExistingRightNode, PRInt32 aOffset);
  NS_IMETHOD DidSplitNode(nsIDOMNode *aExistingRightNode, PRInt32 aOffset, nsIDOMNode *aNewLeftNode, nsresult aResult);
  NS_IMETHOD WillJoinNodes(nsIDOMNode *aLeftNode, nsIDOMNode *aRightNode, nsIDOMNode *aParent);
  NS_IMETHOD DidJoinNodes(nsIDOMNode  *aLeftNode, nsIDOMNode *aRightNode, nsIDOMNode *aParent, nsresult aResult);
  NS_IMETHOD WillInsertText(nsIDOMCharacterData *aTextNode, PRInt32 aOffset, const nsString &aString);
  NS_IMETHOD DidInsertText(nsIDOMCharacterData *aTextNode, PRInt32 aOffset, const nsString &aString, nsresult aResult);
  NS_IMETHOD WillDeleteText(nsIDOMCharacterData *aTextNode, PRInt32 aOffset, PRInt32 aLength);
  NS_IMETHOD DidDeleteText(nsIDOMCharacterData *aTextNode, PRInt32 aOffset, PRInt32 aLength, nsresult aResult);
  NS_IMETHOD WillDeleteSelection(nsIDOMSelection *aSelection);
  NS_IMETHOD DidDeleteSelection(nsIDOMSelection *aSelection);

protected:

  enum RulesEndpoint
  {
    kStart,
    kEnd
  };


  // nsHTMLEditRules implementation methods
  nsresult WillInsertText(  PRInt32          aAction,
                            nsIDOMSelection *aSelection, 
                            PRBool          *aCancel,
                            PRBool          *aHandled,
                            const nsString  *inString,
                            nsString        *outString,
                            PRInt32          aMaxLength);
  nsresult WillInsertBreak(nsIDOMSelection *aSelection, PRBool *aCancel, PRBool *aHandled);
  nsresult WillDeleteSelection(nsIDOMSelection *aSelection, nsIEditor::EDirection aAction, 
                               PRBool *aCancel, PRBool *aHandled);
  nsresult WillMakeList(nsIDOMSelection *aSelection, PRBool aOrderd, PRBool *aCancel, PRBool *aHandled);
  nsresult WillRemoveList(nsIDOMSelection *aSelection, PRBool aOrderd, PRBool *aCancel, PRBool *aHandled);
  nsresult WillIndent(nsIDOMSelection *aSelection, PRBool *aCancel, PRBool *aHandled);
  nsresult WillOutdent(nsIDOMSelection *aSelection, PRBool *aCancel, PRBool *aHandled);
  nsresult WillAlign(nsIDOMSelection *aSelection, const nsString *alignType, PRBool *aCancel, PRBool *aHandled);
  nsresult WillMakeBasicBlock(nsIDOMSelection *aSelection, const nsString *aBlockType, PRBool *aCancel, PRBool *aHandled);
  nsresult DidMakeBasicBlock(nsIDOMSelection *aSelection, nsRulesInfo *aInfo, nsresult aResult);

  nsresult AlignTableElement(nsIDOMNode *aNode, const nsString *alignType);
  nsresult AlignTableCellContents(nsIDOMNode *aNode, const nsString *alignType);
  nsresult GetTableContent(nsIDOMNode *aNode, nsCOMPtr<nsISupportsArray> *outArrayOfNodes);

  nsresult InsertTab(nsIDOMSelection *aSelection, nsString *outString);

  nsresult ReturnInHeader(nsIDOMSelection *aSelection, nsIDOMNode *aHeader, nsIDOMNode *aTextNode, PRInt32 aOffset);
  nsresult ReturnInParagraph(nsIDOMSelection *aSelection, nsIDOMNode *aHeader, nsIDOMNode *aTextNode, PRInt32 aOffset, PRBool *aCancel, PRBool *aHandled);
  nsresult ReturnInListItem(nsIDOMSelection *aSelection, nsIDOMNode *aHeader, nsIDOMNode *aTextNode, PRInt32 aOffset);
  
  nsresult CreateStyleForInsertText(nsIDOMSelection *aSelection, nsIDOMDocument *aDoc);
  nsresult IsEmptyBlock(nsIDOMNode *aNode, 
                        PRBool *outIsEmptyBlock, 
                        PRBool aMozBRDoesntCount = PR_FALSE,
                        PRBool aListItemsNotEmpty = PR_FALSE);
  nsresult IsEmptyNode(nsIDOMNode *aNode, 
                       PRBool *outIsEmptyBlock, 
                       PRBool aMozBRDoesntCount = PR_FALSE,
                       PRBool aListItemsNotEmpty = PR_FALSE);
  PRBool IsFirstNode(nsIDOMNode *aNode);
  PRBool IsLastNode(nsIDOMNode *aNode);
  PRBool AtStartOfBlock(nsIDOMNode *aNode, PRInt32 aOffset, nsIDOMNode *aBlock);
  PRBool AtEndOfBlock(nsIDOMNode *aNode, PRInt32 aOffset, nsIDOMNode *aBlock);

  nsresult GetPromotedPoint(RulesEndpoint aWhere, nsIDOMNode *aNode, PRInt32 aOffset, 
                            PRInt32 actionID, nsCOMPtr<nsIDOMNode> *outNode, PRInt32 *outOffset);
  nsresult GetPromotedRanges(nsIDOMSelection *inSelection, 
                             nsCOMPtr<nsISupportsArray> *outArrayOfRanges, 
                             PRInt32 inOperationType);
  nsresult PromoteRange(nsIDOMRange *inRange, PRInt32 inOperationType);
  nsresult GetNodesForOperation(nsISupportsArray *inArrayOfRanges, 
                                   nsCOMPtr<nsISupportsArray> *outArrayOfNodes, 
                                   PRInt32 inOperationType);
  nsresult GetChildNodesForOperation(nsIDOMNode *inNode, 
                                   nsCOMPtr<nsISupportsArray> *outArrayOfNodes);
  nsresult GetListActionNodes(nsCOMPtr<nsISupportsArray> *outArrayOfNodes);
  nsresult BustUpInlinesAtBRs(nsIDOMNode *inNode, 
                                   nsCOMPtr<nsISupportsArray> *outArrayOfNodes);
  nsresult MakeTransitionList(nsISupportsArray *inArrayOfNodes, 
                                   nsVoidArray *inTransitionArray);
                                   
  nsresult ShouldMakeEmptyBlock(nsIDOMSelection *aSelection, const nsString *blockTag, PRBool *outMakeEmpty);
  nsresult ApplyBlockStyle(nsISupportsArray *arrayOfNodes, const nsString *aBlockTag);
  nsresult MakeBlockquote(nsISupportsArray *arrayOfNodes);
  nsresult SplitAsNeeded(const nsString *aTag, nsCOMPtr<nsIDOMNode> *inOutParent, PRInt32 *inOutOffset);
  nsresult AddTerminatingBR(nsIDOMNode *aBlock);

  nsresult JoinNodesSmart( nsIDOMNode *aNodeLeft, 
                           nsIDOMNode *aNodeRight, 
                           nsCOMPtr<nsIDOMNode> *aOutMergeParent, 
                           PRInt32 *aOutMergeOffset);

  nsresult GetTopEnclosingMailCite(nsIDOMNode *aNode, nsCOMPtr<nsIDOMNode> *aOutCiteNode);
  nsresult PopListItem(nsIDOMNode *aListItem, PRBool *aOutOfList);

  nsresult AdjustSpecialBreaks();
  nsresult AdjustWhitespace(nsIDOMSelection *aSelection);
  nsresult AdjustSelection(nsIDOMSelection *aSelection, nsIEditor::EDirection aAction);
  nsresult FindNearSelectableNode(nsIDOMNode *aSelNode, 
                                  PRInt32 aSelOffset, 
                                  nsIEditor::EDirection aDirection,
                                  nsCOMPtr<nsIDOMNode> *outSelectableNode);
  nsresult RemoveEmptyNodes();
  nsresult SelectionEndpointInNode(nsIDOMNode *aNode, PRBool *aResult);
  nsresult DoTextNodeWhitespace(nsIDOMCharacterData *aTextNode, PRInt32 aStart, PRInt32 aEnd);
  nsresult UpdateDocChangeRange(nsIDOMRange *aRange);

  nsresult ConvertWhitespace(const nsString & inString, nsString & outString);
  nsresult ConfirmSelectionInBody();

  PRBool   IsDescendantOfBody(nsIDOMNode *inNode);
  nsresult InsertMozBRIfNeeded(nsIDOMNode *aNode);

// data members
protected:
  nsCOMPtr<nsIDOMRange> mDocChangeRange;
  PRBool                mListenerEnabled;
  nsCOMPtr<nsIDOMRange> mUtilRange;
  nsCOMPtr<nsIDOMNode>  mBody;
  PRUint32              mJoinOffset;  // need to remember an int across willJoin/didJoin...
  
};

nsresult NS_NewHTMLEditRules(nsIEditRules** aInstancePtrResult);

#endif //nsHTMLEditRules_h__

