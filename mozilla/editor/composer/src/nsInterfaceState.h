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


#ifndef nsInterfaceState_h__
#define nsInterfaceState_h__


#include "nsIDOMSelectionListener.h"
#include "nsIDocumentStateListener.h"
#include "nsITransactionListener.h"
#include "nsIWebShell.h"
#include "nsITimer.h"
#include "nsITimerCallback.h"

class nsIHTMLEditor;
class nsIDOMDocument;

// class responsible for communicating changes in local state back to the UI.
// This is currently somewhat tied to a given XUL UI implementation

class nsInterfaceState : public nsIDOMSelectionListener,
                         public nsIDocumentStateListener,
                         public nsITransactionListener,
                         public nsITimerCallback
{
public:

                nsInterfaceState();
  virtual       ~nsInterfaceState();
  
  NS_DECL_ISUPPORTS
  
  NS_IMETHOD    Init(nsIHTMLEditor* aEditor, nsIDOMDocument *aChromeDoc);

  // force an update of the UI. At some point, we could pass flags
  // here to target certain things for updating.
  NS_IMETHOD    ForceUpdate(const PRUnichar *tagToUpdate);
  
  // nsIDOMSelectionListener interface
  NS_IMETHOD    NotifySelectionChanged(nsIDOMDocument *aDoc, nsIDOMSelection *aSel, short aReason);

  NS_DECL_NSIDOCUMENTSTATELISTENER
  
  // nsITimerCallback interfaces
  NS_IMETHOD_(void) Notify(nsITimer *timer);

  /** nsITransactionListener interfaces
    */
  
  NS_IMETHOD WillDo(nsITransactionManager *aManager, nsITransaction *aTransaction, PRBool *aInterrupt);
  NS_IMETHOD DidDo(nsITransactionManager *aManager, nsITransaction *aTransaction, nsresult aDoResult);
  NS_IMETHOD WillUndo(nsITransactionManager *aManager, nsITransaction *aTransaction, PRBool *aInterrupt);
  NS_IMETHOD DidUndo(nsITransactionManager *aManager, nsITransaction *aTransaction, nsresult aUndoResult);
  NS_IMETHOD WillRedo(nsITransactionManager *aManager, nsITransaction *aTransaction, PRBool *aInterrupt);
  NS_IMETHOD DidRedo(nsITransactionManager *aManager, nsITransaction *aTransaction, nsresult aRedoResult);
  NS_IMETHOD WillBeginBatch(nsITransactionManager *aManager, PRBool *aInterrupt);
  NS_IMETHOD DidBeginBatch(nsITransactionManager *aManager, nsresult aResult);
  NS_IMETHOD WillEndBatch(nsITransactionManager *aManager, PRBool *aInterrupt);
  NS_IMETHOD DidEndBatch(nsITransactionManager *aManager, nsresult aResult);
  NS_IMETHOD WillMerge(nsITransactionManager *aManager, nsITransaction *aTopTransaction,
                       nsITransaction *aTransactionToMerge, PRBool *aInterrupt);
  NS_IMETHOD DidMerge(nsITransactionManager *aManager, nsITransaction *aTopTransaction,
                      nsITransaction *aTransactionToMerge,
                      PRBool aDidMerge, nsresult aMergeResult);

protected:

  enum {
    eStateUninitialized   = -1,
    eStateOff             = PR_FALSE,
    eStateOn              = PR_TRUE
  };
  
  PRBool        SelectionIsCollapsed();
  
  PRBool        XULNodeExists(const char* nodeID);
  
  nsresult      SetNodeAttribute(const char* nodeID, const char* attributeName, const nsString& newValue);
  nsresult      UnsetNodeAttribute(const char* nodeID, const char* attributeName);

  nsresult      UpdateParagraphState(const char* observerName, const char* attributeName);
  nsresult      UpdateListState(const char* observerName);
  nsresult      UpdateTextState(const char* tagName, const char* observerName, const char* attributeName, PRInt8& ioState);
  nsresult      UpdateFontFace(const char* observerName, const char* attributeName);
  nsresult      UpdateDirtyState(PRBool aNowDirty);
  
  nsresult      CallUpdateCommands(const nsString& aCommand);
  
  nsresult      PrimeUpdateTimer();
  void          TimerCallback();

  // this class should not hold references to the editor or editorShell. Doing
  // so would result in cirular reference chains.
  
  nsIHTMLEditor*      mEditor;		 // the HTML editor
  nsIDOMDocument*  mChromeDoc;  // XUL document for the chrome area

  nsIDOMWindow*       mDOMWindow;   // nsIDOMWindow used for calling UpdateCommands
  
  nsCOMPtr<nsITimer>  mUpdateTimer;
  
  // what we need to update
  PRPackedBool  mUpdateParagraph;
  PRPackedBool  mUpdateFont;
  PRPackedBool  mUpdateList;
  
  PRPackedBool  mUpdateBold;
  PRPackedBool  mUpdateItalics;
  PRPackedBool  mUpdateUnderline;
    
  // current state
  PRInt8        mBoldState;
  PRInt8        mItalicState;
  PRInt8        mUnderlineState;
  
  PRInt8        mDirtyState;
  
  PRInt8        mSelectionCollapsed;
  
  PRPackedBool  mFirstDoOfFirstUndo;
  
  nsString      mParagraphFormat;
  nsString      mFontString;
  nsString      mListTag;				// contains "" for none, "ol" or "ul"
  
};

extern "C" nsresult NS_NewInterfaceState(nsIHTMLEditor* aEditor, nsIDOMDocument* aChromeDoc, nsIDOMSelectionListener** aInstancePtrResult);

#endif // nsInterfaceState_h__
