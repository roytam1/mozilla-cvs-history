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
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */
#ifndef nsListControlFrame_h___
#define nsListControlFrame_h___

#ifdef DEBUG_evaughan
//#define DEBUG_rods
#endif

#ifdef DEBUG_rods
#define DO_REFLOW_DEBUG
#define DO_REFLOW_COUNTER
//#define DO_UNCONSTRAINED_CHECK
//#define DO_PIXELS
#endif

#include "nsScrollFrame.h"
#include "nsIFormControlFrame.h"
#include "nsIListControlFrame.h"
#include "nsISelectControlFrame.h"
#include "nsIDOMMouseListener.h"
#include "nsIDOMMouseMotionListener.h"
#include "nsIDOMKeyListener.h"
#include "nsIStatefulFrame.h"
#include "nsIPresState.h"

class nsIDOMHTMLSelectElement;
class nsIDOMHTMLCollection;
class nsIDOMHTMLOptionElement;
class nsIComboboxControlFrame;
class nsIViewManager;
class nsIPresContext;
class nsVoidArray;
class nsIScrollableView;

/**
 * Frame-based listbox.
 */

class nsListControlFrame : public nsScrollFrame, 
                           public nsIFormControlFrame, 
                           public nsIListControlFrame,
                           public nsIDOMMouseListener,
                           public nsIDOMMouseMotionListener,
                           public nsIDOMKeyListener,
                           public nsISelectControlFrame
{
public:
  friend nsresult NS_NewListControlFrame(nsIPresShell* aPresShell, nsIFrame** aNewFrame);

   // nsISupports
  NS_IMETHOD QueryInterface(const nsIID& aIID, void** aInstancePtr);

    // nsIFrame
  NS_IMETHOD HandleEvent(nsIPresContext* aPresContext,
                         nsGUIEvent* aEvent,
                         nsEventStatus* aEventStatus);
  
  NS_IMETHOD SetInitialChildList(nsIPresContext* aPresContext,
                                 nsIAtom*        aListName,
                                 nsIFrame*       aChildList);

  NS_IMETHOD Reflow(nsIPresContext*          aCX,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus);

  NS_IMETHOD Init(nsIPresContext*  aPresContext,
                   nsIContent*      aContent,
                   nsIFrame*        aParent,
                   nsIStyleContext* aContext,
                   nsIFrame*        aPrevInFlow);

  NS_IMETHOD DidReflow(nsIPresContext* aPresContext, nsDidReflowStatus aStatus);
  NS_IMETHOD MoveTo(nsIPresContext* aPresContext, nscoord aX, nscoord aY);
  NS_IMETHOD Destroy(nsIPresContext *aPresContext);

#ifdef IBMBIDI
  /**
   * Get the "type" of the frame
   *
   * @see nsLayoutAtoms::scrollFrame
   */
  NS_IMETHOD GetFrameType(nsIAtom** aType) const;
#endif // IBMBIDI

    // nsIFormControlFrame
  NS_IMETHOD GetType(PRInt32* aType) const;
  NS_IMETHOD GetName(nsString* aName);
  NS_IMETHOD SetProperty(nsIPresContext* aPresContext, nsIAtom* aName, const nsString& aValue);
  NS_IMETHOD GetProperty(nsIAtom* aName, nsString& aValue); 
  NS_IMETHOD GetMultiple(PRBool* aResult, nsIDOMHTMLSelectElement* aSelect = nsnull);
  NS_IMETHOD GetFont(nsIPresContext* aPresContext, 
                     const nsFont*&  aFont);
  NS_IMETHOD GetFormContent(nsIContent*& aContent) const;

  virtual void SetFocus(PRBool aOn = PR_TRUE, PRBool aRepaint = PR_FALSE);
  virtual void ScrollIntoView(nsIPresContext* aPresContext);
  virtual void MouseClicked(nsIPresContext* aPresContext);
  virtual void Reset(nsIPresContext* aPresContext);
  virtual PRBool IsSuccessful(nsIFormControlFrame* aSubmitter);
  virtual PRInt32 GetMaxNumValues();
  virtual PRBool  GetNamesValues(PRInt32 aMaxNumValues, PRInt32& aNumValues,
                                 nsString* aValues, nsString* aNames);
  virtual void SetFormFrame(nsFormFrame* aFrame);
  virtual nscoord GetVerticalInsidePadding(nsIPresContext* aPresContext,
                                           float aPixToTwip,
                                           nscoord aInnerHeight) const;
  virtual nscoord GetHorizontalInsidePadding(nsIPresContext* aPresContext,
                                             float aPixToTwip, 
                                             nscoord aInnerWidth,
                                             nscoord aCharWidth) const;
  virtual nsresult RequiresWidget(PRBool &aRequiresWidget);

    // nsHTMLContainerFrame
  virtual PRIntn GetSkipSides() const;

    // nsIListControlFrame
  NS_IMETHOD SetComboboxFrame(nsIFrame* aComboboxFrame);
  NS_IMETHOD GetSelectedIndex(PRInt32* aIndex); 
  NS_IMETHOD GetSelectedItem(nsString & aStr);
  NS_IMETHOD CaptureMouseEvents(nsIPresContext* aPresContext, PRBool aGrabMouseEvents);
  NS_IMETHOD GetMaximumSize(nsSize &aSize);
  NS_IMETHOD SetSuggestedSize(nscoord aWidth, nscoord aHeight);
  NS_IMETHOD GetNumberOfOptions(PRInt32* aNumOptions);  
  NS_IMETHOD SyncViewWithFrame(nsIPresContext* aPresContext);
  NS_IMETHOD AboutToDropDown();
  NS_IMETHOD AboutToRollup();
  NS_IMETHOD UpdateSelection(PRBool aDoDispatchEvent, PRBool aForceUpdate, nsIContent* aContent);
  NS_IMETHOD SetPresState(nsIPresState * aState) { mPresState = aState; return NS_OK;}
  NS_IMETHOD SetOverrideReflowOptimization(PRBool aValue) { mOverrideReflowOpt = aValue; return NS_OK; }
  NS_IMETHOD GetOptionsContainer(nsIPresContext* aPresContext, nsIFrame** aFrame);

  NS_IMETHOD SaveStateInternal(nsIPresContext* aPresContext, nsIPresState** aState);
  NS_IMETHOD RestoreStateInternal(nsIPresContext* aPresContext, nsIPresState* aState);

  // nsISelectControlFrame
  NS_IMETHOD AddOption(nsIPresContext* aPresContext, PRInt32 index);
  NS_IMETHOD RemoveOption(nsIPresContext* aPresContext, PRInt32 index);
  NS_IMETHOD SetOptionSelected(PRInt32 aIndex, PRBool aValue);
  NS_IMETHOD GetOptionSelected(PRInt32 aIndex, PRBool* aValue);
  NS_IMETHOD DoneAddingContent(PRBool aIsDone);
  NS_IMETHOD OptionDisabled(nsIContent * aContent);

  //nsIStatefulFrame
  NS_IMETHOD GetStateType(nsIPresContext* aPresContext, nsIStatefulFrame::StateType* aStateType);
  NS_IMETHOD SaveState(nsIPresContext* aPresContext, nsIPresState** aState);
  NS_IMETHOD RestoreState(nsIPresContext* aPresContext, nsIPresState* aState);

  //nsIDOMEventListener
  virtual nsresult MouseDown(nsIDOMEvent* aMouseEvent);
  virtual nsresult MouseUp(nsIDOMEvent* aMouseEvent);
  virtual nsresult MouseClick(nsIDOMEvent* aMouseEvent)    { return NS_OK; }
  virtual nsresult MouseDblClick(nsIDOMEvent* aMouseEvent) { return NS_OK; }
  virtual nsresult MouseOver(nsIDOMEvent* aMouseEvent)     { return NS_OK; }
  virtual nsresult MouseOut(nsIDOMEvent* aMouseEvent)      { return NS_OK; }
  virtual nsresult HandleEvent(nsIDOMEvent* aEvent)        { return NS_OK; }

  //nsIDOMEventMotionListener
  virtual nsresult MouseMove(nsIDOMEvent* aMouseEvent);
  virtual nsresult DragMove(nsIDOMEvent* aMouseEvent);

  //nsIDOMKeyListener
  virtual nsresult KeyDown(nsIDOMEvent* aKeyEvent);
  virtual nsresult KeyUp(nsIDOMEvent* aKeyEvent)    { return NS_OK; }
  virtual nsresult KeyPress(nsIDOMEvent* aKeyEvent);

    // Static Methods
  static nsIDOMHTMLSelectElement* GetSelect(nsIContent * aContent);
  static nsIDOMHTMLCollection*    GetOptions(nsIContent * aContent, nsIDOMHTMLSelectElement* aSelect = nsnull);
  static nsIDOMHTMLOptionElement* GetOption(nsIDOMHTMLCollection& aOptions, PRInt32 aIndex);
  static nsIContent* GetOptionAsContent(nsIDOMHTMLCollection* aCollection,PRInt32 aIndex);
  static PRBool                   GetOptionValue(nsIDOMHTMLCollection& aCollecton, PRInt32 aIndex, nsString& aValue);

protected:

  NS_IMETHOD GetSelectedIndexFromDOM(PRInt32* aIndex); // from DOM
  NS_IMETHOD IsTargetOptionDisabled(PRBool &aIsDisabled);
  nsresult   ScrollToFrame(nsIContent * aOptElement);
  PRBool     IsClickingInCombobox(nsIDOMEvent* aMouseEvent);

  nsListControlFrame();
  virtual ~nsListControlFrame();

   // nsScrollFrame overrides
   // Override the widget created for the list box so a Borderless top level widget is created
   // for drop-down lists.
  virtual  nsresult CreateScrollingViewWidget(nsIView* aView,const nsStylePosition* aPosition);
  virtual  nsresult GetScrollingParentView(nsIPresContext* aPresContext,
                                           nsIFrame* aParent,
                                           nsIView** aParentView);
  PRInt32  GetNumberOfOptions();

    // Utility methods
  nsresult GetSizeAttribute(PRInt32 *aSize);
  PRInt32  GetNumberOfSelections();
  nsIContent* GetOptionFromContent(nsIContent *aContent);
  nsresult GetIndexFromDOMEvent(nsIDOMEvent* aMouseEvent, PRInt32& aOldIndex, PRInt32& aCurIndex);
  PRInt32  GetSelectedIndexFromContent(nsIContent *aContent);
  nsIContent* GetOptionContent(PRInt32 aIndex);
  PRBool   IsContentSelected(nsIContent* aContent);
  PRBool   IsContentSelectedByIndex(PRInt32 aIndex);
  void     SetContentSelected(PRInt32 aIndex, PRBool aSelected);
  void     GetViewOffset(nsIViewManager* aManager, nsIView* aView, nsPoint& aPoint);
  nsresult Deselect();
  nsIFrame *GetOptionFromChild(nsIFrame* aParentFrame);
  PRBool   IsAncestor(nsIView* aAncestor, nsIView* aChild);
  nsIView* GetViewFor(nsIWidget* aWidget);
  PRBool   IsInDropDownMode();
  PRBool   IsOptionElement(nsIContent* aContent);
  PRBool   IsOptionElementFrame(nsIFrame *aFrame);
  nsIFrame *GetSelectableFrame(nsIFrame *aFrame);
  void     DisplaySelected(nsIContent* aContent); 
  void     DisplayDeselected(nsIContent* aContent); 
  void     ForceRedraw(nsIPresContext* aPresContext);
  PRBool   IsOptionGroup(nsIFrame* aFrame);
  void     SingleSelection();
  void     MultipleSelection(PRBool aIsShift, PRBool aIsControl);
  void     SelectIndex(PRInt32 aIndex); 
  void     ToggleSelected(PRInt32 aIndex);
  void     ClearSelection();
  void     ExtendedSelection(PRInt32 aStartIndex, PRInt32 aEndIndex, PRBool aDoInvert, PRBool aSetValue);
  void     ResetSelectedItem();
  PRBool   CheckIfAllFramesHere();

  PRBool   HasSameContent(nsIFrame* aFrame1, nsIFrame* aFrame2);
  void     HandleListSelection(nsIDOMEvent * aDOMEvent);
  PRInt32  GetSelectedIndexFromFrame(nsIFrame *aHitFrame);
  PRBool   IsLeftButton(nsIDOMEvent* aMouseEvent);

  void     GetScrollableView(nsIScrollableView*& aScrollableView);

  // onChange detection
  nsresult SelectionChanged(nsIContent* aContent);
  
  // Data Members
  nsFormFrame* mFormFrame;
  PRInt32      mSelectedIndex;
  PRInt32      mOldSelectedIndex;
  PRInt32      mSelectedIndexWhenPoppedDown;
  PRInt32      mStartExtendedIndex;
  PRInt32      mEndExtendedIndex;
  PRBool       mIsInitializedFromContent;
  nsIComboboxControlFrame *mComboboxFrame;
  PRBool       mButtonDown;
  nscoord      mMaxWidth;
  nscoord      mMaxHeight;
  PRBool       mIsCapturingMouseEvents;
  PRInt32      mNumDisplayRows;

  nsVoidArray  * mSelectionCache;
  PRInt32        mSelectionCacheLength;

  PRBool       mIsAllContentHere;
  PRBool       mIsAllFramesHere;
  PRBool       mHasBeenInitialized;

  PRBool       mOverrideReflowOpt;

  PRInt32      mDelayedIndexSetting;
  PRBool       mDelayedValueSetting;

  nsIPresContext* mPresContext;             // XXX: Remove the need to cache the pres context.

  nsCOMPtr<nsIPresState> mPresState;        // Need cache state when list is null

  // XXX temprary only until full system mouse capture works
  PRBool mIsScrollbarVisible;

  //Resize Reflow OpitmizationSize;
  nsSize       mCacheSize;
  nsSize       mCachedMaxElementSize;
  nsSize       mCachedUnconstrainedSize;
  nsSize       mCachedAvailableSize;

#ifdef DO_REFLOW_COUNTER
  PRInt32 mReflowId;
#endif

private:
  NS_IMETHOD_(nsrefcnt) AddRef() { return NS_OK; }
  NS_IMETHOD_(nsrefcnt) Release() { return NS_OK; }
};

#endif /* nsListControlFrame_h___ */

