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

#ifndef nsGfxTextControlFrame_h___
#define nsGfxTextControlFrame_h___

#include "nsIGfxTextControlFrame.h"
#include "nsCOMPtr.h"
#include "nsCWeakReference.h"
#include "nsFormControlFrame.h"
#include "nsTextControlFrame.h"
#include "nsIDocumentLoaderObserver.h"
#include "nsIEditor.h"
#include "nsIDocumentObserver.h"
#include "nsIDOMKeyListener.h"
#include "nsIDOMMouseListener.h"
#include "nsIDOMDragListener.h"
#include "nsIDOMFocusListener.h"
#include "nsIDOMSelectionListener.h"
#include "nsIDOMDocument.h"
#include "nsIPresContext.h"
#include "nsIContent.h"
#include "nsIDOMMouseEvent.h"
#include "nsIDOMKeyEvent.h"
#include "nsITextContent.h"
#include "nsHTMLValue.h"
#include "nsIWebShell.h"
#include "nsIViewManager.h"

#include "nsCSSFrameConstructor.h"

class nsIFrame;
class nsIDOMSelection;


class nsGfxTextControlFrame;

#define NS_IENDER_EVENT_LISTENER_IID \
{/* e4bf05b0-457f-11d3-86ea-000064657374*/ \
0xe4bf05b0, 0x457f, 0x11d3, \
{0x86, 0xea, 0x0, 0x0, 0x64, 0x65, 0x73, 0x74} }


/*******************************************************************************
 * EnderTempObserver 
 * This class is responsible for notifying mFrame when the document
 * has finished loading.
 ******************************************************************************/
class EnderTempObserver : public nsIDocumentLoaderObserver
{
public:
  EnderTempObserver();
  virtual ~EnderTempObserver();

  NS_IMETHOD SetFrame(nsGfxTextControlFrame *aFrame);

  // nsISupports
  NS_DECL_ISUPPORTS

  // nsIDocumentLoaderObserver
  NS_IMETHOD OnStartDocumentLoad(nsIDocumentLoader* loader, 
                                 nsIURI* aURL, 
                                 const char* aCommand);
  NS_IMETHOD OnEndDocumentLoad(nsIDocumentLoader* loader,
                               nsIChannel* channel,
                               nsresult aStatus,
							                 nsIDocumentLoaderObserver * aObserver);
  NS_IMETHOD OnStartURLLoad(nsIDocumentLoader* loader,
                            nsIChannel* channel, 
                            nsIContentViewer* aViewer);
  NS_IMETHOD OnProgressURLLoad(nsIDocumentLoader* loader,
                               nsIChannel* channel,
                               PRUint32 aProgress, 
                               PRUint32 aProgressMax);
  NS_IMETHOD OnStatusURLLoad(nsIDocumentLoader* loader,
                             nsIChannel* channel,
                             nsString& aMsg);
  NS_IMETHOD OnEndURLLoad(nsIDocumentLoader* loader,
                          nsIChannel* channel,
                          nsresult aStatus);
  NS_IMETHOD HandleUnknownContentType( nsIDocumentLoader* loader,
                                       nsIChannel* channel,
                                       const char *aContentType,
                                       const char *aCommand );

protected:
  nsGfxTextControlFrame *mFrame; // not ref counted
  PRBool mFirstCall;
};

/*******************************************************************************
 * nsEnderDocumentObserver
 * This class is responsible for notifying mFrame about document content changes.
 ******************************************************************************/
class nsEnderDocumentObserver : public nsIDocumentObserver
{
public:
  nsEnderDocumentObserver();

  NS_IMETHOD SetFrame(nsGfxTextControlFrame *aFrame);

  virtual ~nsEnderDocumentObserver(); 

  // nsISupports
  NS_DECL_ISUPPORTS

  NS_IMETHOD BeginUpdate(nsIDocument *aDocument);
  NS_IMETHOD EndUpdate(nsIDocument *aDocument);
  NS_IMETHOD BeginLoad(nsIDocument *aDocument);
  NS_IMETHOD EndLoad(nsIDocument *aDocument);
  NS_IMETHOD BeginReflow(nsIDocument *aDocument, nsIPresShell* aShell);
  NS_IMETHOD EndReflow(nsIDocument *aDocument, nsIPresShell* aShell);
  NS_IMETHOD ContentChanged(nsIDocument *aDocument,
                            nsIContent* aContent,
                            nsISupports* aSubContent);
  NS_IMETHOD ContentStatesChanged(nsIDocument* aDocument,
                                  nsIContent* aContent1,
                                  nsIContent* aContent2);
  NS_IMETHOD AttributeChanged(nsIDocument *aDocument,
                              nsIContent*  aContent,
                              PRInt32      aNameSpaceID,
                              nsIAtom*     aAttribute,
                              PRInt32      aHint);
  NS_IMETHOD ContentAppended(nsIDocument *aDocument,
                             nsIContent* aContainer,
                             PRInt32     aNewIndexInContainer);
  NS_IMETHOD ContentInserted(nsIDocument *aDocument,
                             nsIContent* aContainer,
                             nsIContent* aChild,
                             PRInt32 aIndexInContainer);
  NS_IMETHOD ContentReplaced(nsIDocument *aDocument,
                             nsIContent* aContainer,
                             nsIContent* aOldChild,
                             nsIContent* aNewChild,
                             PRInt32 aIndexInContainer);
  NS_IMETHOD ContentRemoved(nsIDocument *aDocument,
                            nsIContent* aContainer,
                            nsIContent* aChild,
                            PRInt32 aIndexInContainer);
  NS_IMETHOD StyleSheetAdded(nsIDocument *aDocument,
                             nsIStyleSheet* aStyleSheet);
  NS_IMETHOD StyleSheetRemoved(nsIDocument *aDocument,
                               nsIStyleSheet* aStyleSheet);
  NS_IMETHOD StyleSheetDisabledStateChanged(nsIDocument *aDocument,
                                            nsIStyleSheet* aStyleSheet,
                                            PRBool aDisabled);
  NS_IMETHOD StyleRuleChanged(nsIDocument *aDocument,
                              nsIStyleSheet* aStyleSheet,
                              nsIStyleRule* aStyleRule,
                              PRInt32 aHint);
  NS_IMETHOD StyleRuleAdded(nsIDocument *aDocument,
                            nsIStyleSheet* aStyleSheet,
                            nsIStyleRule* aStyleRule);
  NS_IMETHOD StyleRuleRemoved(nsIDocument *aDocument,
                              nsIStyleSheet* aStyleSheet,
                              nsIStyleRule* aStyleRule);
  NS_IMETHOD DocumentWillBeDestroyed(nsIDocument *aDocument);


protected:

  nsGfxTextControlFrame *mFrame; // not ref counted
};

/******************************************************************************
 * nsIEnderEventListener
 * Standard interface for event listeners that are attached to nsGfxTextControls
 ******************************************************************************/
 
class nsIEnderEventListener : public nsISupports
{
public:
 
  static const nsIID& GetIID() { static nsIID iid = NS_IENDER_EVENT_LISTENER_IID; return iid; }
 
  /** SetFrame sets the frame we send event messages to, when necessary
   *  @param aFrame -- the frame, can be null, not ref counted (guaranteed to outlive us!)
   */
  NS_IMETHOD SetFrame(nsGfxTextControlFrame *aFrame)=0;

  /** set the pres context associated with this listener instance */
  NS_IMETHOD SetPresContext(nsIPresContext *aCx)=0;

  /** set the view associated with this listener instance */
  NS_IMETHOD SetView(nsIView *aView)=0;

  NS_IMETHOD SetInnerPresShell(nsIPresShell* aPresShell)=0; // Weak ref
};

/******************************************************************************
 * nsEnderEventListener
 * This class is responsible for propogating events from the embedded webshell
 * of nsGfxTextControlFrame to the parent environment.  This enables DOM
 * event listeners on text controls.
 ******************************************************************************/

class nsEnderEventListener; // forward declaration for factory

/* factory for ender key listener */
nsresult NS_NewEnderEventListener(nsIEnderEventListener ** aInstancePtrResult);

class nsEnderEventListener : public nsIEnderEventListener,
                             public nsIDOMKeyListener, 
                             public nsIDOMMouseListener,
                             public nsIDOMFocusListener,
                             public nsIDOMSelectionListener
{
public:

  /** the default destructor */
  virtual ~nsEnderEventListener();

  /** interfaces for addref and release and queryinterface*/
  NS_DECL_ISUPPORTS

  /** nsIEnderEventListener interfaces 
    * @see nsIEnderEventListener
    */
  NS_IMETHOD SetFrame(nsGfxTextControlFrame *aFrame);
  NS_IMETHOD SetPresContext(nsIPresContext *aCx) {mContext = do_QueryInterface(aCx); return NS_OK;}
  NS_IMETHOD SetView(nsIView *aView) {mView = aView; return NS_OK;} // views are not ref counted
  NS_IMETHOD SetInnerPresShell(nsIPresShell* aPresShell) { mInnerShell = aPresShell; return NS_OK; }

  /** nsIDOMKeyListener interfaces 
    * @see nsIDOMKeyListener
    */
  virtual nsresult HandleEvent(nsIDOMEvent* aEvent);
  virtual nsresult KeyDown(nsIDOMEvent* aKeyEvent);
  virtual nsresult KeyUp(nsIDOMEvent* aKeyEvent);
  virtual nsresult KeyPress(nsIDOMEvent* aKeyEvent);
  /* END interfaces from nsIDOMKeyListener*/

  /** nsIDOMMouseListener interfaces 
    * @see nsIDOMMouseListener
    */
  virtual nsresult MouseDown(nsIDOMEvent* aMouseEvent);
  virtual nsresult MouseUp(nsIDOMEvent* aMouseEvent);
  virtual nsresult MouseClick(nsIDOMEvent* aMouseEvent);
  virtual nsresult MouseDblClick(nsIDOMEvent* aMouseEvent);
  virtual nsresult MouseOver(nsIDOMEvent* aMouseEvent);
  virtual nsresult MouseOut(nsIDOMEvent* aMouseEvent);
  /* END interfaces from nsIDOMMouseListener*/

  /** nsIDOMFocusListener interfaces 
    * used to propogate focus, blur, and change notifications
    * @see nsIDOMFocusListener
    */
  virtual nsresult Focus(nsIDOMEvent* aEvent);
  virtual nsresult Blur (nsIDOMEvent* aEvent);
  /* END interfaces from nsIDOMFocusListener*/

  /** nsIDOMSelectionListener interfaces 
    * @see nsIDOMSelectionListener
    */
  NS_IMETHOD NotifySelectionChanged();
  /*END interfaces from nsIDOMSelectionListener*/
 
  friend nsresult NS_NewEnderEventListener(nsIEnderEventListener ** aInstancePtrResult);

protected:
  /** the default constructor.  Protected, use the factory to create an instance.
    * @see NS_NewEnderEventListener
    */
  nsEnderEventListener();

  /** mouse event dispatch helper */
  nsresult DispatchMouseEvent(nsIDOMMouseEvent *aEvent, PRInt32 aEventType);


protected:
  nsCWeakReference<nsGfxTextControlFrame> mFrame;
  nsIView                  *mView;    // not ref counted
  nsCOMPtr<nsIPresContext>  mContext; // ref counted
  nsIPresShell*             mInnerShell; // not ref counted
  nsCOMPtr<nsIContent>      mContent; // ref counted
  nsString                  mTextValue; // the value of the text field at focus
  PRBool                    mSkipFocusDispatch; // On Mouse down we don't want to dispatch 
                                                // any focus events (they get dispatched
                                                // from the native widget
                            // note nsGfxTextControlFrame is held as a weak ptr
                            // because the frame can be deleted in the middle
                            // of event processing.  See the KeyUp handler
                            // for places where this is a problem, and see
                            // nsCWeakReference.h for notes on use.
};


/******************************************************************************
 * nsEnderFocusListenerForDisplayContent
 * used to watch for focus notifications on the text control frame's display content
 ******************************************************************************/

class nsEnderFocusListenerForDisplayContent : public nsIDOMFocusListener
{
public:

  /** the default destructor */
  virtual ~nsEnderFocusListenerForDisplayContent();

  /*interfaces for addref and release and queryinterface*/
  NS_DECL_ISUPPORTS

  /* nsIDOMFocusListener interfaces */
  virtual nsresult HandleEvent(nsIDOMEvent* aEvent);
  virtual nsresult Focus(nsIDOMEvent* aEvent);
  virtual nsresult Blur (nsIDOMEvent* aEvent);
  /* END interfaces from nsIDOMFocusListener*/

  NS_IMETHOD SetFrame(nsGfxTextControlFrame *aFrame);

  nsEnderFocusListenerForDisplayContent();

protected:
  nsGfxTextControlFrame    *mFrame;   // not ref counted
};


/******************************************************************************
 * nsEnderListenerForContent
 * used to watch for drag notifications on the text control frame's content
 ******************************************************************************/

class nsEnderListenerForContent : 
  public nsIDOMDragListener
{
public:

  /** the default destructor */
  virtual ~nsEnderListenerForContent();

  /*interfaces for addref and release and queryinterface*/
  NS_DECL_ISUPPORTS

  /** nsIDOMDragListener interfaces 
    * @see nsIDOMDragListener
    */
  virtual nsresult HandleEvent(nsIDOMEvent* aEvent);
  virtual nsresult DragEnter(nsIDOMEvent* aDragEvent);
  virtual nsresult DragOver(nsIDOMEvent* aDragEvent);
  virtual nsresult DragExit(nsIDOMEvent* aDragEvent);
  virtual nsresult DragDrop(nsIDOMEvent* aDragEvent);
  virtual nsresult DragGesture(nsIDOMEvent* aDragEvent);
  /* END interfaces from nsIDOMDragListener*/

  NS_IMETHOD SetFrame(nsGfxTextControlFrame *aFrame);

  nsEnderListenerForContent();

protected:
  nsGfxTextControlFrame    *mFrame;   // not ref counted
  nsCOMPtr<nsIViewManager>  mViewManager;
};


/******************************************************************************
 * nsGfxTextControlFrame
 * frame that represents an HTML text input element
 * handles both <input type=text> and <textarea>
 * For performance, nsGfxTextControl displays text with a simple block frame
 * until it first recieves focus.  At that point, it builds a subdocument (mWebShell)
 * and attaches an editor to the subdocument.
 ******************************************************************************/

class nsGfxTextControlFrame : public nsTextControlFrame,
                              public nsIGfxTextControlFrame
{
private:
	typedef nsFormControlFrame Inherited;

public:

  /** constructor */
  nsGfxTextControlFrame();

  /** destructor */
  virtual ~nsGfxTextControlFrame();

  /** needs it's own QI for nsIGfxTextControlFrame */
  NS_IMETHOD QueryInterface(const nsIID& aIID, void** aInstancePtr);

  /** nsIFrame override of Init.
    * all we do here is cache the pres context for later use
    */
  NS_IMETHOD  Init(nsIPresContext*  aPresContext,
                   nsIContent*      aContent,
                   nsIFrame*        aParent,
                   nsIStyleContext* aContext,
                   nsIFrame*        aPrevInFlow);

  NS_IMETHOD GetFrameType(nsIAtom** aType) const;

#ifdef NS_DEBUG
  /** debug method to dump frames 
    * @see nsIFrame
    */
  NS_IMETHOD  List(nsIPresContext* aPresContext, FILE* out, PRInt32 aIndent) const;
#endif

  /** create all the objects required for editing. */
  NS_IMETHOD CreateEditor();

  // nsIFormControlFrame
  NS_IMETHOD SetProperty(nsIPresContext* aPresContext, nsIAtom* aName, const nsString& aValue);
  NS_IMETHOD GetProperty(nsIAtom* aName, nsString& aValue); 
  virtual void SetFocus(PRBool aOn = PR_TRUE, PRBool aRepaint = PR_FALSE);
  virtual nsWidgetInitData* GetWidgetInitData(nsIPresContext* aPresContext);
  virtual void PostCreateWidget(nsIPresContext* aPresContext,
                                nscoord& aWidth,
                                nscoord& aHeight) {};

  /** handler for attribute changes to mContent */
  NS_IMETHOD AttributeChanged(nsIPresContext* aPresContext,
                              nsIContent*     aChild,
                              PRInt32         aNameSpaceID,
                              nsIAtom*        aAttribute,
                              PRInt32         aHint);


  /** fire up the webshell and editor 
    * @param aSizeOfSubdocContainer  if null, the size of this frame is used (normally null)
    *                                if not null, the size from which the subdoc size is calculated.
    */
  virtual nsresult CreateSubDoc(nsRect *aSizeOfSubdocContainer);

  /** @see nsTextControlFrame::GetText */
  NS_IMETHOD GetText(nsString* aValue, PRBool aInitialValue);

  /** 
    * Respond to a gui event
    * @see nsNativeFormControlFrame::HandleEvent
    */
  NS_IMETHOD HandleEvent(nsIPresContext* aPresContext, 
                         nsGUIEvent* aEvent,
                         nsEventStatus* aEventStatus);

  virtual void EnterPressed(nsIPresContext* aPresContext) ;

  virtual PRBool GetNamesValues(PRInt32 aMaxNumValues, PRInt32& aNumValues,
                                nsString* aValues, nsString* aNames);
  virtual void Reset(nsIPresContext* aPresContext);

  // override to interact with webshell
  NS_IMETHOD Reflow(nsIPresContext*          aPresContext,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus);

  NS_IMETHOD ReflowNavQuirks(nsIPresContext*          aPresContext,
                              nsHTMLReflowMetrics&     aDesiredSize,
                              const nsHTMLReflowState& aReflowState,
                              nsReflowStatus&          aStatus,
                              nsMargin&                aBorder,
                              nsMargin&                aPadding);
  NS_IMETHOD ReflowStandard(nsIPresContext*          aPresContext,
                            nsHTMLReflowMetrics&     aDesiredSize,
                            const nsHTMLReflowState& aReflowState,
                            nsReflowStatus&          aStatus,
                            nsMargin&                aBorder,
                            nsMargin&                aPadding);

  NS_IMETHOD Paint(nsIPresContext* aPresContext,
                   nsIRenderingContext& aRenderingContext,
                   const nsRect& aDirtyRect,
                   nsFramePaintLayer aWhichLayer);
 
  // Utility methods to get and set current widget state
  void GetTextControlFrameState(nsString& aValue);
  void SetTextControlFrameState(const nsString& aValue);
  
  // this method is called to notify the frame that it has gained focus
  void NotifyFocus();
  
  NS_IMETHOD InstallEditor();

  virtual nsresult RequiresWidget(PRBool &aRequiresWidget);

  NS_IMETHOD InternalContentChanged();
  NS_IMETHOD DoesAttributeExist(nsIAtom *aAtt);

  void RemoveNewlines(nsString &aString);

  nsCWeakReferent *WeakReferent()
    { return &mWeakReferent; }

  // The nsEnderEventListener needs to know if the focus
  // had been successfully set, if so then it won't reset it
  // See: nsGfxTextControlFrame::SetFocus
  //      nsEnderEventListener::Focus
  PRBool DidSetFocus() { return mDidSetFocus; }

  /* ============= nsIGfxTextControlFrame ================= */
  NS_IMETHOD GetEditor(nsIEditor **aEditor);
  NS_IMETHOD GetWebShell(nsIWebShell **aWebShell);
  NS_IMETHOD SetInnerFocus();

protected:

  /** calculate the inner region of the text control (size - border and padding) in pixels */
  virtual void CalcSizeOfSubDocInTwips(const nsMargin &aBorder, 
                                       const nsMargin &aPadding,  
                                       const nsSize &aFrameSize, 
                                       nsRect &aSubBounds);

  PRInt32 CalculateSizeNavQuirks (nsIPresContext*       aPresContext, 
                                  nsIRenderingContext*  aRendContext,
                                  nsIFormControlFrame*  aFrame,
                                  const nsSize&         aCSSSize, 
                                  nsInputDimensionSpec& aSpec, 
                                  nsSize&               aDesiredSize, 
                                  nsSize&               aMinSize, 
                                  PRBool&               aWidthExplicit, 
                                  PRBool&               aHeightExplicit, 
                                  nscoord&              aRowHeight,
                                  nsMargin&             aBorder,
                                  nsMargin&             aPadding);

  PRInt32 CalculateSizeStandard (nsIPresContext*       aPresContext, 
                                  nsIRenderingContext*  aRendContext,
                                  nsIFormControlFrame*  aFrame,
                                  const nsSize&         aCSSSize, 
                                  nsInputDimensionSpec& aSpec, 
                                  nsSize&               aDesiredSize, 
                                  nsSize&               aMinSize, 
                                  PRBool&               aWidthExplicit, 
                                  PRBool&               aHeightExplicit, 
                                  nscoord&              aRowHeight,
                                  nsMargin&             aBorder,
                                  nsMargin&             aPadding);

  nsresult GetColRowSizeAttr(nsIFormControlFrame*  aFrame,
                             nsIAtom *     aColSizeAttr,
                             nsHTMLValue & aColSize,
                             nsresult &    aColStatus,
                             nsIAtom *     aRowSizeAttr,
                             nsHTMLValue & aRowSize,
                             nsresult &    aRowStatus);
 
  NS_IMETHOD CreateWebShell(nsIPresContext* aPresContext,
                            const nsSize& aSize);

  NS_IMETHOD InitializeTextControl(nsIPresShell *aPresShell, nsIDOMDocument *aDoc);

  NS_IMETHOD InstallEventListeners();

  NS_IMETHOD GetPresShellFor(nsIWebShell* aWebShell, nsIPresShell** aPresShell);
  
  NS_IMETHOD GetFirstNodeOfType(const nsString& aTag, nsIDOMDocument *aDOMDoc, nsIDOMNode **aBodyNode);

  NS_IMETHOD GetFirstFrameForType(const nsString& aTag, nsIPresShell *aPresShell, nsIDOMDocument *aDOMDoc, nsIFrame **aResult);

  /** create an event based on aEvent and pass it to the sub document mWebShell
    * via the sub document's view manager.
    */
  NS_IMETHOD RedispatchMouseEventToSubDoc(nsIPresContext* aPresContext, 
                                          nsGUIEvent* aEvent,
                                          nsEventStatus* aEventStatus,
                                          PRBool aAdjustForView);

  virtual PRBool IsSingleLineTextControl() const;

  virtual PRBool IsPlainTextControl() const;

  virtual PRBool IsPasswordTextControl() const;

  virtual PRBool IsInitialized() const;

  virtual PRInt32 GetWidthInCharacters() const;


  virtual void PaintTextControlBackground(nsIPresContext* aPresContext,
                                          nsIRenderingContext& aRenderingContext,
                                          const nsRect& aDirtyRect,
                                          nsFramePaintLayer aWhichLayer);

  virtual void PaintTextControl(nsIPresContext* aPresContext,
                                nsIRenderingContext& aRenderingContext,
                                const nsRect& aDirtyRect, nsString& aText,
                                nsIStyleContext* aStyleContext,
                                nsRect& aRect);

  virtual void PaintChild(nsIPresContext*      aPresContext,
                          nsIRenderingContext& aRenderingContext,
                          const nsRect&        aDirtyRect,
                          nsIFrame*            aFrame,
                          nsFramePaintLayer    aWhichLayer);

  NS_IMETHOD FirstChild(nsIAtom *aListName, nsIFrame **aFirstChild) const;
  
  NS_IMETHOD GetAdditionalChildListName(PRInt32 aIndex,
                                        nsIAtom** aListName) const;
  NS_IMETHOD Destroy(nsIPresContext *aPresContext);

public:
  void SetShouldSetFocus() { mDidSetFocus = PR_FALSE; };
  void SetFrameConstructor(nsCSSFrameConstructor *aConstructor)
    { mFrameConstructor = aConstructor; } // not owner - do not addref!

protected:
  nsCOMPtr<nsIWebShell> mWebShell;
  PRBool mCreatingViewer;
  EnderTempObserver* mTempObserver;
  nsEnderDocumentObserver *mDocObserver;
  PRBool mNotifyOnInput;  // init false, 
    // when true this frame propogates notifications whenever the edited content is changed
  PRBool mIsProcessing;
  PRBool mNeedsStyleInit;
  nsIPresContext *mFramePresContext; // not ref counted
  nsString* mCachedState; // this is used for caching changed between frame creation
                          // and full initialization
  nsCWeakReferent mWeakReferent; // so this obj can be used as a weak ptr

  // listeners
  nsCOMPtr<nsIEnderEventListener> mEventListener;           // ref counted
  nsEnderFocusListenerForDisplayContent *mFocusListenerForDisplayContent; // ref counted
  nsEnderListenerForContent *mListenerForContent;  // ref counted

  nsCSSFrameConstructor *mFrameConstructor;
  nsIFrame *mDisplayFrame;
  nsCOMPtr<nsITextContent> mDisplayContent;
 
  // editing state
  nsCOMPtr<nsIEditor>       mEditor;  // ref counted
  nsCOMPtr<nsIDOMDocument>  mDoc;     // ref counted

  PRBool mDidSetFocus;  // init false, 

  // the PassThroughState is used to manage a tiny state machine so 
  // only the proper messages get passed through
  enum PassThroughState
  {
    eUninitialized,
    eGotDown, // with my bad self
    eGotUp,
    eGotClick
  };
  PassThroughState mPassThroughMouseEvents;

private:  // frames are not refcounted
  NS_IMETHOD_(nsrefcnt) AddRef() { return NS_OK; }
  NS_IMETHOD_(nsrefcnt) Release() { return NS_OK; }


};

#endif

