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

#ifndef nsIEditor_h__
#define nsIEditor_h__
#include "nsISupports.h"

class nsIDOMElement;
class nsIDOMNode;
class nsITransaction;

/*
Editor interface to outside world
*/

#define NS_IEDITOR_IID \
{/* A3C5EE71-742E-11d2-8F2C-006008310194*/ \
0xa3c5ee71, 0x742e, 0x11d2, \
{0x8f, 0x2c, 0x0, 0x60, 0x8, 0x31, 0x1, 0x94} }

#define NS_IEDITORFACTORY_IID \
{ /* {E2F4C7F1-864A-11d2-8F38-006008310194}*/ \
0xe2f4c7f1, 0x864a, 0x11d2, \
{ 0x8f, 0x38, 0x0, 0x60, 0x8, 0x31, 0x1, 0x94 } }

#define NS_ITEXTEDITORFACTORY_IID \
{ /* {4a1f5ce0-c1f9-11d2-8f4c-006008159b0c}*/ \
0x4a1f5ce0, 0xc1f9, 0x11d2, \
{ 0x8f, 0x4c, 0x0, 0x60, 0x8, 0x15, 0x9b, 0xc } }

class nsIDOMDocument;
class nsIDOMSelection;
class nsIPresShell;
class nsString;

/**
 * A generic editor interface. 
 * <P>
 * nsIEditor is the base interface used by applications to communicate with the editor.  
 * It makes no assumptions about the kind of content it is editing, 
 * other than the content being a DOM tree. 
 */
class nsIEditor  : public nsISupports{
public:

  typedef enum {eLTR=0, eRTL=1} Direction;

  static const nsIID& IID() { static nsIID iid = NS_IEDITOR_IID; return iid; }

  /**
   * Init tells is to tell the implementation of nsIEditor to begin its services
   * @param aDomInterface   The dom interface being observed
   * @param aPresShell      TEMP: The presentation shell displaying the document
   *                        once events can tell us from what pres shell they originated, 
   *                        this will no longer be necessary and the editor will no longer be
   *                        linked to a single pres shell.
   */
  virtual nsresult Init(nsIDOMDocument *aDomInterface,
                        nsIPresShell   *aPresShell) = 0;

  /**
   * return the DOM Document this editor is associated with
   *
   * @param aDoc [OUT] the dom interface being observed, refcounted
   */
  virtual nsresult GetDocument(nsIDOMDocument **aDoc)=0;

  /** 
   * return the DOM Selection for the presentation shell that has focus
   * (or most recently had focus.)
   * @param aSelection [OUT] the dom interface for the selection
   */
  virtual nsresult GetSelection(nsIDOMSelection **aSelection)=0;

  /**
   * SetAttribute() sets the attribute of aElement.
   * No checking is done to see if aAttribute is a legal attribute of the node,
   * or if aValue is a legal value of aAttribute.
   *
   * @param aElement    the content element to operate on
   * @param aAttribute  the string representation of the attribute to set
   * @param aValue      the value to set aAttribute to
   */
  virtual nsresult SetAttribute(nsIDOMElement * aElement, 
                                const nsString& aAttribute, 
                                const nsString& aValue)=0;

  /**
   * GetAttributeValue() retrieves the attribute's value for aElement.
   *
   * @param aElement      the content element to operate on
   * @param aAttribute    the string representation of the attribute to get
   * @param aResultValue  the value of aAttribute.  only valid if aResultIsSet is PR_TRUE
   * @param aResultIsSet  PR_TRUE if aAttribute is set on the current node, PR_FALSE if it is not.
   */
  virtual nsresult GetAttributeValue(nsIDOMElement * aElement, 
                                     const nsString& aAttribute, 
                                     nsString&       aResultValue, 
                                     PRBool&         aResultIsSet)=0;

  /**
   * RemoveAttribute() deletes aAttribute from the attribute list of aElement.
   * If aAttribute is not an attribute of aElement, nothing is done.
   *
   * @param aElement      the content element to operate on
   * @param aAttribute    the string representation of the attribute to get
   */
  virtual nsresult RemoveAttribute(nsIDOMElement * aElement, 
                                   const nsString& aAttribute)=0;

  /** 
   * CreateNode instantiates a new element of type aTag and inserts it into aParent at aPosition.
   * @param aTag      The type of object to create
   * @param aParent   The node to insert the new object into
   * @param aPosition The place in aParent to insert the new node
   * @param aNewNode  [OUT] The node created.  Caller must release aNewNode.
   */
  virtual nsresult CreateNode(const nsString& aTag,
                              nsIDOMNode *    aParent,
                              PRInt32         aPosition,
                              nsIDOMNode **   aNewNode)=0;

  /** 
   * InsertNode inserts aNode into aParent at aPosition.
   * No checking is done to verify the legality of the insertion.  That is the 
   * responsibility of the caller.
   * @param aNode     The DOM Node to insert.
   * @param aParent   The node to insert the new object into
   * @param aPosition The place in aParent to insert the new node
   */
  virtual nsresult InsertNode(nsIDOMNode * aNode,
                              nsIDOMNode * aParent,
                              PRInt32      aPosition)=0;


  /**
   * InsertText() Inserts a string at the current location, given by the selection.
   * If the selection is not collapsed, the selection is deleted and the insertion
   * takes place at the resulting collapsed selection.
   *
   * NOTE: what happens if the string contains a CR?
   *
   * @param aString   the string to be inserted
   */
  virtual nsresult InsertText(const nsString& aStringToInsert)=0;

  /** 
   * DeleteNode removes aChild from aParent.
   * If aChild is not a child of aParent, nothing is done and an error is returned.
   * @param aChild    The node to delete
   */
  virtual nsresult DeleteNode(nsIDOMNode * aChild)=0;

  /** 
   * DeleteSelection removes all nodes in the current selection.
   * @param aDir  if eLTR, delete to the right (for example, the DEL key)
   *              if eRTL, delete to the left (for example, the BACKSPACE key)
   */
  virtual nsresult DeleteSelection(nsIEditor::Direction aDir)=0;

  /** 
   * SplitNode() creates a new node identical to an existing node, and split the contents between the two nodes
   * @param aExistingRightNode   the node to split.  It will become the new node's next sibling.
   * @param aOffset              the offset of aExistingRightNode's content|children to do the split at
   * @param aNewLeftNode         [OUT] the new node resulting from the split, becomes aExistingRightNode's previous sibling.
   */
  virtual nsresult SplitNode(nsIDOMNode * aExistingRightNode,
                             PRInt32      aOffset,
                             nsIDOMNode ** aNewLeftNode)=0;

  /** 
   * JoinNodes() takes 2 nodes and merge their content|children.
   * @param aNodeToKeep   The node that will remain after the join.
   * @param aNodeToJoin   The node that will be joined with aNodeToKeep.
   *                      There is no requirement that the two nodes be of the same type.
   * @param aParent       The parent of aExistingRightNode
   * @param aNodeToKeepIsFirst  if PR_TRUE, the contents|children of aNodeToKeep come before the
   *                            contents|children of aNodeToJoin, otherwise their positions are switched.
   */
  virtual nsresult JoinNodes(nsIDOMNode *aNodeToKeep,
                            nsIDOMNode  *aNodeToJoin,
                            nsIDOMNode  *aParent,
                            PRBool       aNodeToKeepIsFirst)=0;
  
  /**
   * The handler for RETURN keys and CTRL-RETURN keys.<br>
   * It may enter a character, split a node in the tree, etc.
   * @param aCtrlKey  was the CtrlKey down?
   */
  virtual nsresult InsertBreak(PRBool aCtrlKey)=0;

  /** turn the undo system on or off
    * @param aEnable  if PR_TRUE, the undo system is turned on if it is available
    *                 if PR_FALSE the undo system is turned off if it was previously on
    * @return         if aEnable is PR_TRUE, returns NS_OK if the undo system could be initialized properly
    *                 if aEnable is PR_FALSE, returns NS_OK.
    */
  virtual nsresult EnableUndo(PRBool aEnable)=0;

  /** Do() fires a transaction.  It is provided here so clients can create their own transactions.
    * If a transaction manager is present, it is used.  
    * Otherwise, the transaction is just executed directly.
    *
    * @param aTxn the transaction to execute
    */
  virtual nsresult Do(nsITransaction *aTxn)=0;

  /** Undo reverses the effects of the last Do operation, if Undo is enabled in the editor.
    * It is provided here so clients need no knowledge of whether the editor has a transaction manager or not.
    * If a transaction manager is present, it is told to undo and the result of
    * that undo is returned.  
    * Otherwise, the Undo request is ignored and an error NS_ERROR_NOT_AVAILABLE is returned.
    *
    */
  virtual nsresult Undo(PRUint32 aCount)=0;

  /** returns state information about the undo system.
    * @param aIsEnabled [OUT] PR_TRUE if undo is enabled
    * @param aCanUndo   [OUT] PR_TRUE if at least one transaction is currently ready to be undone.
    */
  virtual nsresult CanUndo(PRBool &aIsEnabled, PRBool &aCanUndo)=0;

  /** Redo reverses the effects of the last Undo operation
    * It is provided here so clients need no knowledge of whether the editor has a transaction manager or not.
    * If a transaction manager is present, it is told to redo and the result of the previously undone
    * transaction is reapplied to the document.
    * If no transaction is available for Redo, or if the document has no transaction manager,
    * the Redo request is ignored and an error NS_ERROR_NOT_AVAILABLE is returned.
    *
    */
  virtual nsresult Redo(PRUint32 aCount)=0;

  /** returns state information about the redo system.
    * @param aIsEnabled [OUT] PR_TRUE if redo is enabled
    * @param aCanRedo   [OUT] PR_TRUE if at least one transaction is currently ready to be redone.
    */
  virtual nsresult CanRedo(PRBool &aIsEnabled, PRBool &aCanRedo)=0;

  /** BeginTransaction is a signal from the caller to the editor that the caller will execute multiple updates
    * to the content tree that should be treated as a single logical operation,
    * in the most efficient way possible.<br>
    * All transactions executed between a call to BeginTransaction and EndTransaction will be undoable as
    * an atomic action.<br>
    * EndTransaction must be called after BeginTransaction.<br>
    * Calls to BeginTransaction can be nested, as long as EndTransaction is called once per BeginUpdate.
    */
  virtual nsresult BeginTransaction()=0;

  /** EndTransaction is a signal to the editor that the caller is finished updating the content model.<br>
    * BeginUpdate must be called before EndTransaction is called.<br>
    * Calls to BeginTransaction can be nested, as long as EndTransaction is called once per BeginTransaction.
    */
  virtual nsresult EndTransaction()=0;

  /** scroll the viewport so the selection is in view.
    * @param aScrollToBegin  PR_TRUE if the beginning of the selection is to be scrolled into view.
    *                        PR_FALSE if the end of the selection is to be scrolled into view
    */
  virtual nsresult ScrollIntoView(PRBool aScrollToBegin)=0;


};

#endif //nsIEditor_h__

