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

#ifndef nsPlaintextEditor_h__
#define nsPlaintextEditor_h__

#include "nsCOMPtr.h"

#include "nsIPlaintextEditor.h"

#include "nsEditor.h"
#include "nsIDOMElement.h"
#include "nsIDOMEventListener.h"

#include "TypeInState.h"
#include "nsEditRules.h"
 
class nsIDOMKeyEvent;
class nsITransferable;
class nsIDOMEventReceiver;
class nsIDocumentEncoder;

/**
 * The text editor implementation.
 * Use to edit text document represented as a DOM tree. 
 */
class nsPlaintextEditor : public nsEditor,
                          public nsIPlaintextEditor
{

public:

// Interfaces for addref and release and queryinterface
// NOTE macro used is for classes that inherit from 
// another class. Only the base class should use NS_DECL_ISUPPORTS
  NS_DECL_ISUPPORTS_INHERITED

  /* below used by TypedText() */
  enum {
    eTypedText,  /* user typed text */
    eTypedBR,    /* user typed shift-enter to get a br */
    eTypedBreak  /* user typed enter */
  };

           nsPlaintextEditor();
  virtual  ~nsPlaintextEditor();

  /* ------------ nsIPlaintextEditor methods -------------- */
  NS_DECL_NSIPLAINTEXTEDITOR

  /* ------------ nsIEditorIMESupport overrides -------------- */
  
  NS_IMETHOD SetCompositionString(const nsString& aCompositionString, nsIPrivateTextRangeList* aTextRangeList,nsTextEventReply* aReply);
  NS_IMETHOD GetReconversionString(nsReconversionEventReply* aReply);

  /* ------------ Overrides of nsEditor interface methods -------------- */

  /** prepare the editor for use */
  NS_IMETHOD Init(nsIDOMDocument *aDoc, nsIPresShell *aPresShell,  nsIContent *aRoot, nsISelectionController *aSelCon, PRUint32 aFlags);
  
  NS_IMETHOD GetDocumentIsEmpty(PRBool *aDocumentIsEmpty);

  NS_IMETHOD DeleteSelection(EDirection aAction);

  NS_IMETHOD SetDocumentCharacterSet(const PRUnichar* characterSet);

  /** we override this here to install event listeners */
  NS_IMETHOD PostCreate();

  NS_IMETHOD GetFlags(PRUint32 *aFlags);
  NS_IMETHOD SetFlags(PRUint32 aFlags);

  NS_IMETHOD Undo(PRUint32 aCount);
  NS_IMETHOD Redo(PRUint32 aCount);

  NS_IMETHOD Cut();
  NS_IMETHOD CanCut(PRBool &aCanCut);
  NS_IMETHOD Copy();
  NS_IMETHOD CanCopy(PRBool &aCanCopy);
  NS_IMETHOD Paste(PRInt32 aSelectionType);
  NS_IMETHOD CanPaste(PRInt32 aSelectionType, PRBool &aCanPaste);

  NS_IMETHOD CanDrag(nsIDOMEvent *aDragEvent, PRBool &aCanDrag);
  NS_IMETHOD DoDrag(nsIDOMEvent *aDragEvent);
  NS_IMETHOD InsertFromDrop(nsIDOMEvent* aDropEvent);

  NS_IMETHOD OutputToString(nsAWritableString& aOutputString,
                            const nsAReadableString& aFormatType,
                            PRUint32 aFlags);
                            
  NS_IMETHOD OutputToStream(nsIOutputStream* aOutputStream,
                            const nsAReadableString& aFormatType,
                            const nsAReadableString* aCharsetOverride,
                            PRUint32 aFlags);


  /** All editor operations which alter the doc should be prefaced
   *  with a call to StartOperation, naming the action and direction */
  NS_IMETHOD StartOperation(PRInt32 opID, nsIEditor::EDirection aDirection);

  /** All editor operations which alter the doc should be followed
   *  with a call to EndOperation */
  NS_IMETHOD EndOperation();

  /** make the given selection span the entire document */
  NS_IMETHOD SelectEntireDocument(nsISelection *aSelection);

  /* ------------ Utility Routines, not part of public API -------------- */
  NS_IMETHOD TypedText(const PRUnichar* aString, PRInt32 aAction);

  /** returns the absolute position of the end points of aSelection
    * in the document as a text stream.
    */
  nsresult GetTextSelectionOffsets(nsISelection *aSelection,
                                   PRInt32 &aStartOffset, 
                                   PRInt32 &aEndOffset);

  nsresult GetAbsoluteOffsetsForPoints(nsIDOMNode *aInStartNode,
                                       PRInt32 aInStartOffset,
                                       nsIDOMNode *aInEndNode,
                                       PRInt32 aInEndOffset,
                                       nsIDOMNode *aInCommonParentNode,
                                       PRInt32 &aOutStartOffset, 
                                       PRInt32 &aEndOffset);

protected:

  NS_IMETHOD  InitRules();
  void        BeginEditorInit();
  nsresult    EndEditorInit();
  
  /** install the event listeners for the editor 
    * used to be part of Init, but now broken out into a separate method
    * called by PostCreate, giving the caller the chance to interpose
    * their own listeners before we install our own backstops.
    */
  NS_IMETHOD InstallEventListeners();

  /** returns the layout object (nsIFrame in the real world) for aNode
    * @param aNode          the content to get a frame for
    * @param aLayoutObject  the "primary frame" for aNode, if one exists.  May be null
    * @return NS_OK whether a frame is found or not
    *         an error if some serious error occurs
    */
  NS_IMETHOD GetLayoutObject(nsIDOMNode *aInNode, nsISupports **aOutLayoutObject);
  NS_IMETHOD GetBodyStyleContext(nsIStyleContext** aStyleContext);

  // Helpers for output routines
  NS_IMETHOD GetAndInitDocEncoder(const nsAReadableString& aFormatType,
                                  PRUint32 aFlags,
                                  const nsAReadableString* aCharset,
                                  nsIDocumentEncoder** encoder);

  // key event helpers
  NS_IMETHOD CreateBR(nsIDOMNode *aNode, PRInt32 aOffset, 
                      nsCOMPtr<nsIDOMNode> *outBRNode, EDirection aSelect = eNone);
  NS_IMETHOD CreateBRImpl(nsCOMPtr<nsIDOMNode> *aInOutParent, 
                         PRInt32 *aInOutOffset, 
                         nsCOMPtr<nsIDOMNode> *outBRNode, 
                         EDirection aSelect);
  NS_IMETHOD InsertBR(nsCOMPtr<nsIDOMNode> *outBRNode);

  NS_IMETHOD IsRootTag(nsString &aTag, PRBool &aIsTag);


  // factored methods for handling insertion of data from transferables (drag&drop or clipboard)
  NS_IMETHOD PrepareTransferable(nsITransferable **transferable);
  NS_IMETHOD InsertTextFromTransferable(nsITransferable *transferable);

  /** simple utility to handle any error with event listener allocation or registration */
  void HandleEventListenerError();

  /* small utility routine to test the eEditorReadonly bit */
  PRBool IsModifiable();

  nsresult GetDOMEventReceiver(nsIDOMEventReceiver **aEventReceiver);

  //XXX Kludge: Used to suppress spurious drag/drop events (bug 50703)
  PRBool   mIgnoreSpuriousDragEvent;
  NS_IMETHOD IgnoreSpuriousDragEvent(PRBool aIgnoreSpuriousDragEvent) {mIgnoreSpuriousDragEvent = aIgnoreSpuriousDragEvent; return NS_OK;}

// Data members
protected:

  nsCOMPtr<nsIEditRules>        mRules;
  nsCOMPtr<nsIDOMEventListener> mKeyListenerP;
  nsCOMPtr<nsIDOMEventListener> mMouseListenerP;
  nsCOMPtr<nsIDOMEventListener> mTextListenerP;
  nsCOMPtr<nsIDOMEventListener> mCompositionListenerP;
  nsCOMPtr<nsIDOMEventListener> mDragListenerP;
  nsCOMPtr<nsIDOMEventListener> mFocusListenerP;
  PRBool 	mIsComposing;
  PRInt32 mMaxTextLength;
  PRInt32 mInitTriggerCounter;

public:
  static nsIAtom *gTypingTxnName;
  static nsIAtom *gIMETxnName;
  static nsIAtom *gDeleteTxnName;

// friends
friend class nsHTMLEditRules;
friend class nsTextEditRules;
friend class nsAutoEditInitRulesTrigger;

};

#endif //nsPlaintextEditor_h__

