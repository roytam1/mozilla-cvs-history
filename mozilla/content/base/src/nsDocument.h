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
#ifndef nsDocument_h___
#define nsDocument_h___

#include "nsIDocument.h"
#include "nsVoidArray.h"
#include "nsIDOMDocument.h"
#include "nsIScriptObjectOwner.h"
#include "nsIScriptContextOwner.h"
#include "nsIDOMEventCapturer.h"
#include "nsXIFConverter.h"
#include "nsIJSScriptObject.h"

class nsISelection;
class nsIEventListenerManager;

class nsPostData : public nsIPostData {
public:
  nsPostData(PRBool aIsFile, char* aData);

  NS_DECL_ISUPPORTS

  virtual PRBool      IsFile();
  virtual const char* GetData();
  virtual PRInt32     GetDataLength();

protected:
  virtual ~nsPostData();

  PRBool  mIsFile;
  char*   mData;
  PRInt32 mDataLen;
};


// Base class for our document implementations
class nsDocument : public nsIDocument, public nsIDOMDocument, public nsIScriptObjectOwner, 
                   public nsIDOMEventCapturer, public nsIJSScriptObject {
public:
  NS_DECL_ISUPPORTS

  virtual nsIArena* GetArena();

  /**
   * Return the title of the document. May return null.
   */
  virtual const nsString* GetDocumentTitle() const;

  /**
   * Return the URL for the document. May return null.
   */
  virtual nsIURL* GetDocumentURL() const;

  /**
   * Return a standard name for the document's character set. This will
   * trigger a startDocumentLoad if necessary to answer the question.
   */
  virtual nsCharSetID GetDocumentCharacterSet() const;
  virtual void SetDocumentCharacterSet(nsCharSetID aCharSetID);

  /**
   * Create a new presentation shell that will use aContext for
   * it's presentation context (presentation context's <b>must not</b> be
   * shared among multiple presentation shell's).
   */
  virtual nsresult CreateShell(nsIPresContext* aContext,
                               nsIViewManager* aViewManager,
                               nsIStyleSet* aStyleSet,
                               nsIPresShell** aInstancePtrResult);
  virtual PRBool DeleteShell(nsIPresShell* aShell);
  virtual PRInt32 GetNumberOfShells();
  virtual nsIPresShell* GetShellAt(PRInt32 aIndex);

  /**
   * Return the parent document of this document. Will return null
   * unless this document is within a compound document and has a parent.
   */
  virtual nsIDocument* GetParentDocument();
  virtual void SetParentDocument(nsIDocument* aParent);
  virtual void AddSubDocument(nsIDocument* aSubDoc);
  virtual PRInt32 GetNumberOfSubDocuments();
  virtual nsIDocument* GetSubDocumentAt(PRInt32 aIndex);

  /**
   * Return the root content object for this document.
   */
  virtual nsIContent* GetRootContent();
  virtual void SetRootContent(nsIContent* aRoot);

  /**
   * Get the style sheets owned by this document.
   */
  virtual PRInt32 GetNumberOfStyleSheets();
  virtual nsIStyleSheet* GetStyleSheetAt(PRInt32 aIndex);
  virtual void AddStyleSheet(nsIStyleSheet* aSheet);

  /**
   * Set the object from which a document can get a script context.
   * This is the context within which all scripts (during document 
   * creation and during event handling) will run.
   */
  virtual nsIScriptContextOwner *GetScriptContextOwner();
  virtual void SetScriptContextOwner(nsIScriptContextOwner *aScriptContextOwner);

  /**
   * Add a new observer of document change notifications. Whenever
   * content is changed, appended, inserted or removed the observers are
   * informed.
   */
  virtual void AddObserver(nsIDocumentObserver* aObserver);

  /**
   * Remove an observer of document change notifications. This will
   * return false if the observer cannot be found.
   */
  virtual PRBool RemoveObserver(nsIDocumentObserver* aObserver);

  // Observation hooks used by content nodes to propagate
  // notifications to document observers.
  NS_IMETHOD BeginLoad();
  NS_IMETHOD EndLoad();
  virtual void ContentChanged(nsIContent* aContent,
                              nsISupports* aSubContent);

  virtual void ContentAppended(nsIContent* aContainer);

  virtual void ContentInserted(nsIContent* aContainer,
                               nsIContent* aChild,
                               PRInt32 aIndexInContainer);

  virtual void ContentReplaced(nsIContent* aContainer,
                               nsIContent* aOldChild,
                               nsIContent* aNewChild,
                               PRInt32 aIndexInContainer);

  virtual void ContentWillBeRemoved(nsIContent* aContainer,
                                    nsIContent* aChild,
                                    PRInt32 aIndexInContainer);

  virtual void ContentHasBeenRemoved(nsIContent* aContainer,
                                     nsIContent* aChild,
                                     PRInt32 aIndexInContainer);

  /**
    * Returns the Selection Object
   */
  virtual nsISelection * GetSelection();

  /**
    * Selects all the Content
   */
  virtual void SelectAll();

  /**
    * Copies all text from the selection
   */
  virtual void GetSelectionText(nsString & aText);

  void TraverseTree(nsString   & aText,  
                    nsIContent * aContent, 
                    nsIContent * aStart, 
                    nsIContent * aEnd, 
                    PRBool     & aInRange);

  /**
    * Converts the document or a selection of the 
    * document to XIF (XML Interchange Format)
    * and places the result in aBuffer.
    */
  virtual void CreateXIF(nsString & aBuffer, PRBool aUseSelection);
  virtual void ToXIF(nsXIFConverter& aConverter, nsIDOMNode* aNode);
  virtual void BeginConvertToXIF(nsXIFConverter& aConverter, nsIDOMNode* aNode);
  virtual void ConvertChildrenToXIF(nsXIFConverter& aConverter, nsIDOMNode* aNode);
  virtual void FinishConvertToXIF(nsXIFConverter& aConverter, nsIDOMNode* aNode);


public:
  
  NS_IMETHOD GetScriptObject(nsIScriptContext *aContext, void** aScriptObject);
  NS_IMETHOD ResetScriptObject();

  // nsIDOMDocument interface
  NS_IMETHOD    GetMasterDoc(nsIDOMDocument **aDocument);
  NS_IMETHOD    GetDocumentType(nsIDOMDocumentType** aDocumentType);
  NS_IMETHOD    GetProlog(nsIDOMNodeList** aProlog);
  NS_IMETHOD    GetEpilog(nsIDOMNodeList** aEpilog);
  NS_IMETHOD    GetDocumentElement(nsIDOMElement** aDocumentElement);
  NS_IMETHOD    CreateDocumentFragment(nsIDOMDocumentFragment** aReturn);
  NS_IMETHOD    CreateComment(const nsString& aData, nsIDOMComment** aReturn);
  NS_IMETHOD    CreateProcessingInstruction(const nsString& aTarget, const nsString& aData, nsIDOMProcessingInstruction** aReturn);
  NS_IMETHOD    CreateAttribute(const nsString& aName, nsIDOMNode* aValue, nsIDOMAttribute** aReturn);
  NS_IMETHOD    CreateElement(const nsString& aTagName, 
                              nsIDOMNamedNodeMap* aAttributes, 
                              nsIDOMElement** aReturn);
  NS_IMETHOD    CreateTextNode(const nsString& aData, nsIDOMText** aReturn);
  NS_IMETHOD    GetElementsByTagName(const nsString& aTagname, nsIDOMNodeList** aReturn);

  // nsIDOMNode interface
  NS_IMETHOD    GetNodeName(nsString& aNodeName);
  NS_IMETHOD    GetNodeValue(nsString& aNodeValue);
  NS_IMETHOD    SetNodeValue(const nsString& aNodeValue);
  NS_IMETHOD    GetNodeType(PRInt32* aNodeType);
  NS_IMETHOD    GetParentNode(nsIDOMNode** aParentNode);
  NS_IMETHOD    GetChildNodes(nsIDOMNodeList** aChildNodes);
  NS_IMETHOD    GetHasChildNodes(PRBool* aHasChildNodes);
  NS_IMETHOD    GetFirstChild(nsIDOMNode** aFirstChild);
  NS_IMETHOD    GetLastChild(nsIDOMNode** aLastChild);
  NS_IMETHOD    GetPreviousSibling(nsIDOMNode** aPreviousSibling);
  NS_IMETHOD    GetNextSibling(nsIDOMNode** aNextSibling);
  NS_IMETHOD    GetAttributes(nsIDOMNamedNodeMap** aAttributes);
  NS_IMETHOD    InsertBefore(nsIDOMNode* aNewChild, nsIDOMNode* aRefChild, nsIDOMNode** aReturn);
  NS_IMETHOD    ReplaceChild(nsIDOMNode* aNewChild, nsIDOMNode* aOldChild, nsIDOMNode** aReturn);
  NS_IMETHOD    RemoveChild(nsIDOMNode* aOldChild, nsIDOMNode** aReturn);
  NS_IMETHOD    CloneNode(nsIDOMNode** aReturn);
  NS_IMETHOD    Equals(nsIDOMNode* aNode, PRBool aDeep, PRBool* aReturn);

  // nsIDOMEventCapturer interface
  NS_IMETHOD CaptureEvent(nsIDOMEventListener *aListener);
  NS_IMETHOD ReleaseEvent(nsIDOMEventListener *aListener);
  NS_IMETHOD AddEventListener(nsIDOMEventListener *aListener, const nsIID& aIID);
  NS_IMETHOD RemoveEventListener(nsIDOMEventListener *aListener, const nsIID& aIID);
  NS_IMETHOD GetNewListenerManager(nsIEventListenerManager **aInstancePtrResult);

  NS_IMETHOD HandleDOMEvent(nsIPresContext& aPresContext, 
                            nsEvent* aEvent, 
                            nsIDOMEvent** aDOMEvent,
                            PRUint32 aFlags,
                            nsEventStatus& aEventStatus);

  // nsIJSScriptObject interface
  virtual PRBool    AddProperty(JSContext *aContext, jsval aID, jsval *aVp);
  virtual PRBool    DeleteProperty(JSContext *aContext, jsval aID, jsval *aVp);
  virtual PRBool    GetProperty(JSContext *aContext, jsval aID, jsval *aVp);
  virtual PRBool    SetProperty(JSContext *aContext, jsval aID, jsval *aVp);
  virtual PRBool    EnumerateProperty(JSContext *aContext);
  virtual PRBool    Resolve(JSContext *aContext, jsval aID);
  virtual PRBool    Convert(JSContext *aContext, jsval aID);
  virtual void      Finalize(JSContext *aContext);

  virtual PRBool IsInRange(nsIContent *aStartContent, nsIContent* aEndContent, nsIContent* aContent) const;
  virtual PRBool IsBefore(nsIContent *aNewContent, nsIContent* aCurrentContent) const;
  virtual nsIContent* GetPrevContent(nsIContent *aContent) const;
  virtual nsIContent* GetNextContent(nsIContent *aContent) const;

protected:
  nsIContent* FindContent( nsIContent* aStartNode,
                          nsIContent* aTest1, 
                          nsIContent* aTest2) const;

protected:
  virtual void AddStyleSheetToSet(nsIStyleSheet* aSheet, nsIStyleSet* aSet);  // subclass hook
  NS_IMETHOD GetListenerManager(nsIEventListenerManager** aInstancePtrResult);
  nsresult SetScriptEventListener(JSContext *aContext, REFNSIID aListenerTypeIID);

  nsDocument();
  virtual ~nsDocument(); 
  nsresult Init();

  nsISelection * mSelection;

  nsIArena* mArena;
  nsString* mDocumentTitle;
  nsIURL* mDocumentURL;
  nsCharSetID mCharacterSet;
  nsIDocument* mParentDocument;
  nsVoidArray mSubDocuments;
  nsVoidArray mPresShells;
  nsIContent* mRootContent;
  nsVoidArray mStyleSheets;
  nsVoidArray mObservers;
  void* mScriptObject;
  nsIScriptContextOwner *mScriptContextOwner;
  nsIEventListenerManager* mListenerManager;
  PRBool mInDestructor;
};

#endif /* nsDocument_h___ */
