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

#ifndef nsTextEditRules_h__
#define nsTextEditRules_h__

#include "nsCOMPtr.h"

#include "nsHTMLEditor.h"
#include "nsIDOMNode.h"

#include "nsEditRules.h"
#include "TypeInState.h"

/** Object that encapsulates HTML text-specific editing rules.
  *  
  * To be a good citizen, edit rules must live by these restrictions:
  * 1. All data manipulation is through the editor.  
  *    Content nodes in the document tree must <B>not</B> be manipulated directly.
  *    Content nodes in document fragments that are not part of the document itself
  *    may be manipulated at will.  Operations on document fragments must <B>not</B>
  *    go through the editor.
  * 2. Selection must not be explicitly set by the rule method.  
  *    Any manipulation of Selection must be done by the editor.
  */
class nsTextEditRules : public nsEditRules
{
public:

              nsTextEditRules();
  virtual     ~nsTextEditRules();

  // nsEditRules methods
  NS_IMETHOD Init(nsHTMLEditor *aEditor, PRUint32 aFlags);
  NS_IMETHOD WillDoAction(nsIDOMSelection *aSelection, nsRulesInfo *aInfo, PRBool *aCancel, PRBool *aHandled);
  NS_IMETHOD DidDoAction(nsIDOMSelection *aSelection, nsRulesInfo *aInfo, nsresult aResult);
  NS_IMETHOD GetFlags(PRUint32 *aFlags);
  NS_IMETHOD SetFlags(PRUint32 aFlags);
  NS_IMETHOD DocumentIsEmpty(PRBool *aDocumentIsEmpty);

  // nsTextEditRules action id's
  enum 
  {
    kDefault             = 0,
    // any editor that has a txn mgr
    kUndo                = 1000,
    kRedo                = 1001,
    // text actions
    kInsertText          = 2000,
    kInsertTextIME       = 2001,
    kDeleteSelection     = 2002,
    kSetTextProperty     = 2003,
    kRemoveTextProperty  = 2004,
    kOutputText          = 2005,
    // html only action
    kInsertBreak         = 3000,
    kMakeList            = 3001,
    kIndent              = 3002,
    kOutdent             = 3003,
    kAlign               = 3004,
    kMakeBasicBlock      = 3005,
    kRemoveList          = 3006,
    kInsertElement       = 3008
  };
  
protected:

  // nsTextEditRules implementation methods
  nsresult WillInsertText(nsIDOMSelection  *aSelection, 
                            PRBool         *aCancel,
                            PRBool         *aHandled,
                            const nsString *inString,
                            nsString       *outString,
                            TypeInState    typeInState,
                            PRInt32         aMaxLength);
  nsresult DidInsertText(nsIDOMSelection *aSelection, nsresult aResult);
  nsresult GetTopEnclosingPre(nsIDOMNode *aNode, nsIDOMNode** aOutPreNode);
  nsresult CreateStyleForInsertText(nsIDOMSelection *aSelection, TypeInState &aTypeInState);

  nsresult WillInsertBreak(nsIDOMSelection *aSelection, PRBool *aCancel, PRBool *aHandled);
  nsresult DidInsertBreak(nsIDOMSelection *aSelection, nsresult aResult);

  nsresult WillInsert(nsIDOMSelection *aSelection, PRBool *aCancel);
  nsresult DidInsert(nsIDOMSelection *aSelection, nsresult aResult);

  nsresult WillDeleteSelection(nsIDOMSelection *aSelection, 
                               nsIEditor::ESelectionCollapseDirection aCollapsedAction, 
                               PRBool *aCancel,
                               PRBool *aHandled);
  nsresult DidDeleteSelection(nsIDOMSelection *aSelection, 
                              nsIEditor::ESelectionCollapseDirection aCollapsedAction, 
                              nsresult aResult);

  nsresult WillSetTextProperty(nsIDOMSelection *aSelection, PRBool *aCancel, PRBool *aHandled);
  nsresult DidSetTextProperty(nsIDOMSelection *aSelection, nsresult aResult);

  nsresult WillRemoveTextProperty(nsIDOMSelection *aSelection, PRBool *aCancel, PRBool *aHandled);
  nsresult DidRemoveTextProperty(nsIDOMSelection *aSelection, nsresult aResult);

  nsresult WillUndo(nsIDOMSelection *aSelection, PRBool *aCancel, PRBool *aHandled);
  nsresult DidUndo(nsIDOMSelection *aSelection, nsresult aResult);

  nsresult WillRedo(nsIDOMSelection *aSelection, PRBool *aCancel, PRBool *aHandled);
  nsresult DidRedo(nsIDOMSelection *aSelection, nsresult aResult);

  /** called prior to nsIEditor::OutputToString
    * @param aSelection
    * @param aInFormat  the format requested for the output, a MIME type
    * @param aOutText   the string to use for output, if aCancel is set to true
    * @param aOutCancel if set to PR_TRUE, the caller should cancel the operation
    *                   and use aOutText as the result.
    */
  nsresult WillOutputText(nsIDOMSelection *aSelection,
                          const nsString  *aInFormat,
                          nsString *aOutText, 
                          PRBool   *aOutCancel, 
                          PRBool *aHandled);

  nsresult DidOutputText(nsIDOMSelection *aSelection, nsresult aResult);


  // helper functions
  
  /** insert aNode into a new style node of type aTag.
    * aSelection is optional.  If provided, aSelection is set to (aNode, 0)
    * if aNode was successfully placed in a new style node
    * @param aNewStyleNode   [OUT] The newly created style node, if result is successful
    *                              undefined if result is a failure.
    */
  nsresult InsertStyleNode(nsIDOMNode      *aNode, 
                           nsIAtom         *aTag, 
                           nsIDOMSelection *aSelection,
                           nsIDOMNode     **aNewStyleNode);

  /** inserts a new <FONT> node and sets the aAttr attribute to aValue */
  nsresult CreateFontStyleForInsertText(nsIDOMNode      *aNewTextNode,
                                        const nsString  &aAttr, 
                                        const nsString  &aValue, 
                                        nsIDOMSelection *aInOutSelection);

  /** create a new style node of type aTag in aParentNode at aOffset, 
    * and create a new text node in the new style node.  
    *
    * @param aParentNode   the node that will be the parent of the new style node
    * @param aOffset       the positoin in aParentNode to put the new style node
    * @param aTag          the type of style node to create
    *                      no validation of aTag is done, caller is responsible 
    *                      for passing in a reasonable tag name
    * @param aAttr         optional attribute to set on new style node
    *                      ignored if it is an empty string
    * @param aValue        optional value for aAttr.  Ignored if aAttr is an empty string
    * @param aInOutSelection    optional.  If provided and if it is collapsed to a text node, 
    *                           we use the text node and wrap a style node around it.
    *                           If provided, aSelection is collapsed to (newTextNode, 0)
    * if newTextNode was successfully created.
    */
  nsresult InsertStyleAndNewTextNode(nsIDOMNode *aParentNode, 
                                     PRInt32     aOffset,
                                     nsIAtom    *aTag,
                                     const nsString  &aAttr,
                                     const nsString  &aValue,
                                     nsIDOMSelection *aSelection);

  /** creates a bogus text node if the document has no editable content */
  nsresult CreateBogusNodeIfNeeded(nsIDOMSelection *aSelection);

  /** returns a truncated insertion string if insertion would place us
      over aMaxLength */
  nsresult TruncateInsertionIfNeeded(nsIDOMSelection *aSelection, 
                                           const nsString  *aInString,
                                           nsString        *aOutString,
                                           PRInt32          aMaxLength);
  
  /** Echo's the insertion text into the password buffer, and converts
      insertion text to '*'s */                                        
  nsresult EchoInsertionToPWBuff(nsIDOMSelection *aSelection, nsString *aOutString);

  /** do the actual text insertion */
  nsresult DoTextInsertion(nsIDOMSelection *aSelection, 
                           PRBool          *aCancel,
                           const nsString  *aInString,
                           TypeInState      aTypeInState);
  
  nsresult GetPriorHTMLSibling(nsIDOMNode *inNode, nsCOMPtr<nsIDOMNode> *outNode);
  nsresult GetNextHTMLSibling(nsIDOMNode *inNode, nsCOMPtr<nsIDOMNode> *outNode);
  nsresult GetPriorHTMLNode(nsIDOMNode *inNode, nsCOMPtr<nsIDOMNode> *outNode);
  nsresult GetPriorHTMLNode(nsIDOMNode *inParent, PRInt32 inOffset, nsCOMPtr<nsIDOMNode> *outNode);
  nsresult GetNextHTMLNode(nsIDOMNode *inNode, nsCOMPtr<nsIDOMNode> *outNode);
  nsresult GetNextHTMLNode(nsIDOMNode *inParent, PRInt32 inOffset, nsCOMPtr<nsIDOMNode> *outNode);

  nsresult IsFirstEditableChild( nsIDOMNode *aNode, PRBool *aOutIsFirst);
  nsresult IsLastEditableChild( nsIDOMNode *aNode, PRBool *aOutIsLast);
  nsresult GetFirstEditableChild( nsIDOMNode *aNode, nsCOMPtr<nsIDOMNode> *aOutFirstChild);
  nsresult GetLastEditableChild( nsIDOMNode *aNode, nsCOMPtr<nsIDOMNode> *aOutLastChild);

  static PRBool IsBody(nsIDOMNode *aNode);
  static PRBool IsBreak(nsIDOMNode *aNode);
  static PRBool IsMozBR(nsIDOMNode *aNode);
  static PRBool HasMozAttr(nsIDOMNode *aNode);

  static PRBool InBody(nsIDOMNode *aNode);

  nsresult CreateMozBR(nsIDOMNode *inParent, PRInt32 inOffset, nsCOMPtr<nsIDOMNode> *outBRNode);


  // data
  nsHTMLEditor *mEditor;  // note that we do not refcount the editor
  nsString      mPasswordText;  // a buffer we use to store the real value of password editors
  nsCOMPtr<nsIDOMNode> mBogusNode;  // magic node acts as placeholder in empty doc
  PRUint32 mFlags;
};



class nsTextRulesInfo : public nsRulesInfo
{
 public:
 
  nsTextRulesInfo(int aAction) : 
    nsRulesInfo(aAction),
    inString(0),
    outString(0),
    outputFormat(0),
    typeInState(),
    maxLength(-1),
    collapsedAction(nsIEditor::eDeleteNext),
    bOrdered(PR_FALSE),
    alignType(0),
    blockType(0),
    insertElement(0)
    {};

  virtual ~nsTextRulesInfo() {};
  
  // kInsertText
  const nsString *inString;
  nsString *outString;
  const nsString *outputFormat;
  TypeInState typeInState;
  PRInt32 maxLength;
  
  // kDeleteSelection
  nsIEditor::ESelectionCollapseDirection collapsedAction;
  
  // kMakeList
  PRBool bOrdered;
  
  // kAlign
  const nsString *alignType;
  
  // kMakeBasicBlock
  const nsString *blockType;
  
  // kInsertElement
  const nsIDOMElement* insertElement;
};

#endif //nsTextEditRules_h__

