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

#ifndef nsGfxTextControlFrame_h___
#define nsGfxTextControlFrame_h___

#include "nsCOMPtr.h"
#include "nsCWeakReference.h"
#include "nsFormControlFrame.h"
#include "nsTextControlFrame.h"
#include "nsIDocumentLoaderObserver.h"
#include "nsIEditor.h"
#include "nsIDocumentObserver.h"
#include "nsIDOMKeyListener.h"
#include "nsIDOMMouseListener.h"
#include "nsIDOMFocusListener.h"
#include "nsIDOMSelectionListener.h"
#include "nsIDOMDocument.h"
#include "nsIPresContext.h"
#include "nsIContent.h"
#include "nsIDOMUIEvent.h"
#include "nsHTMLValue.h"

class nsIFrame;
class nsIWebShell;
class nsIDOMSelection;


class nsGfxTextControlFrame;

#define NS_IENDER_EVENT_LISTENER_IID \
{/* e4bf05b0-457f-11d3-86ea-000064657374*/ \
0xe4bf05b0, 0x457f, 0x11d3, \
{0x86, 0xea, 0x0, 0x0, 0x64, 0x65, 0x73, 0x74} }


/*******************************************************************************
 * EnderTempObserver XXX temporary until doc manager/loader is in place
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
 * This class responds to document changes
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
};

/******************************************************************************
 * nsEnderEventListener
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
  nsresult DispatchMouseEvent(nsIDOMUIEvent *aEvent, PRInt32 aEventType);


protected:
  nsCWeakReference<nsGfxTextControlFrame> mFrame;
  nsIView                  *mView;    // not ref counted
  nsCOMPtr<nsIPresContext>  mContext; // ref counted
  nsCOMPtr<nsIContent>      mContent; // ref counted
  nsString                  mTextValue; // the value of the text field at focus

                            // note nsGfxTextControlFrame is held as a weak ptr
                            // because the frame can be deleted in the middle
                            // of event processing.  See the KeyUp handler
                            // for places where this is a problem, and see
                            // nsCWeakReference.h for notes on use.
};



/******************************************************************************
 * nsGfxTextControlFrame
 ******************************************************************************/

class nsGfxTextControlFrame : public nsTextControlFrame
{
private:
	typedef nsFormControlFrame Inherited;

public:
  nsGfxTextControlFrame();
  virtual ~nsGfxTextControlFrame();

  /** nsIFrame override of Init.
    * all we do here is cache the pres context for later use
    */
  NS_IMETHOD  Init(nsIPresContext&  aPresContext,
                   nsIContent*      aContent,
                   nsIFrame*        aParent,
                   nsIStyleContext* aContext,
                   nsIFrame*        aPrevInFlow);
  NS_IMETHOD  List(FILE* out, PRInt32 aIndent) const;

  NS_IMETHOD InitTextControl();

       // nsIFormControlFrame
  NS_IMETHOD SetProperty(nsIAtom* aName, const nsString& aValue);
  NS_IMETHOD GetProperty(nsIAtom* aName, nsString& aValue); 
  virtual void SetFocus(PRBool aOn = PR_TRUE, PRBool aRepaint = PR_FALSE);

  virtual nsWidgetInitData* GetWidgetInitData(nsIPresContext& aPresContext);

  NS_IMETHOD AttributeChanged(nsIPresContext* aPresContext,
                              nsIContent*     aChild,
                              PRInt32         aNameSpaceID,
                              nsIAtom*        aAttribute,
                              PRInt32         aHint);

  virtual void PostCreateWidget(nsIPresContext* aPresContext,
                                nscoord& aWidth,
                                nscoord& aHeight);

  NS_IMETHOD GetText(nsString* aValue, PRBool aInitialValue);

  /** 
    * Respond to a gui event
    * @see nsNativeFormControlFrame::HandleEvent
    */
  NS_IMETHOD HandleEvent(nsIPresContext& aPresContext, 
                         nsGUIEvent* aEvent,
                         nsEventStatus& aEventStatus);

  virtual void EnterPressed(nsIPresContext& aPresContext) ;

  virtual PRBool GetNamesValues(PRInt32 aMaxNumValues, PRInt32& aNumValues,
                                nsString* aValues, nsString* aNames);
  virtual void Reset();

  // override to interact with webshell
  NS_IMETHOD Reflow(nsIPresContext&          aPresContext,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus);

  NS_IMETHOD ReflowNavQuirks(nsIPresContext&          aPresContext,
                              nsHTMLReflowMetrics&     aDesiredSize,
                              const nsHTMLReflowState& aReflowState,
                              nsReflowStatus&          aStatus,
                              nsMargin&                aBorderPadding);
  NS_IMETHOD ReflowStandard(nsIPresContext&          aPresContext,
                            nsHTMLReflowMetrics&     aDesiredSize,
                            const nsHTMLReflowState& aReflowState,
                            nsReflowStatus&          aStatus,
                            nsMargin&                aBorderPadding);

  NS_IMETHOD Paint(nsIPresContext& aPresContext,
                   nsIRenderingContext& aRenderingContext,
                   const nsRect& aDirtyRect,
                   nsFramePaintLayer aWhichLayer);

  virtual void PaintTextControlBackground(nsIPresContext& aPresContext,
                                          nsIRenderingContext& aRenderingContext,
                                          const nsRect& aDirtyRect,
                                          nsFramePaintLayer aWhichLayer);

  virtual void PaintTextControl(nsIPresContext& aPresContext,
                                nsIRenderingContext& aRenderingContext,
                                const nsRect& aDirtyRect, nsString& aText,
                                nsIStyleContext* aStyleContext,
                                nsRect& aRect);
 
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

protected:
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
                                  nsMargin&             aBorderPadding);

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
                                  nsMargin&             aBorderPadding);

  nsresult GetColRowSizeAttr(nsIFormControlFrame*  aFrame,
                             nsIAtom *     aColSizeAttr,
                             nsHTMLValue & aColSize,
                             nsresult &    aColStatus,
                             nsIAtom *     aRowSizeAttr,
                             nsHTMLValue & aRowSize,
                             nsresult &    aRowStatus);
 
  NS_IMETHOD CreateWebShell(nsIPresContext& aPresContext,
                            const nsSize& aSize);

  NS_IMETHOD InitializeTextControl(nsIPresShell *aPresShell, nsIDOMDocument *aDoc);

  NS_IMETHOD InstallEventListeners();

  NS_IMETHOD GetPresShellFor(nsIWebShell* aWebShell, nsIPresShell** aPresShell);
  
  NS_IMETHOD GetFirstNodeOfType(const nsString& aTag, nsIDOMDocument *aDOMDoc, nsIDOMNode **aBodyNode);

  NS_IMETHOD GetFirstFrameForType(const nsString& aTag, nsIPresShell *aPresShell, nsIDOMDocument *aDOMDoc, nsIFrame **aResult);

  NS_IMETHOD SelectAllTextContent(nsIDOMNode *aBodyNode, nsIDOMSelection *aSelection);

  PRBool IsSingleLineTextControl() const;

  PRBool IsPlainTextControl() const;

  PRBool IsPasswordTextControl() const;

  PRBool IsInitialized() const;

  PRInt32 GetWidthInCharacters() const;


protected:
  nsIWebShell* mWebShell;
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
  nsCOMPtr<nsIEnderEventListener> mEventListener;  // ref counted

  // editing state
  nsCOMPtr<nsIEditor>       mEditor;  // ref counted
  nsCOMPtr<nsIDOMDocument>  mDoc;     // ref counted

};

#endif

