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

#ifndef nsComboboxControlFrame_h___
#define nsComboboxControlFrame_h___

#define OPTIMIZE_RESIZE_RELOW

#include "nsAreaFrame.h"
#include "nsIFormControlFrame.h"
#include "nsIComboboxControlFrame.h"
#include "nsVoidArray.h"
#include "nsIAnonymousContentCreator.h"
#include "nsISelectControlFrame.h"
#include "nsIStatefulFrame.h"
#include "nsIRollupListener.h"

class nsButtonControlFrame;
class nsTextControlFrame;
class nsFormFrame;
class nsIView;
class nsStyleContext;
class nsIHTMLContent;
class nsIListControlFrame;

/**
 * Child list name indices
 * @see #GetAdditionalChildListName()
 */
#define NS_COMBO_FRAME_POPUP_LIST_INDEX   (NS_AREA_FRAME_ABSOLUTE_LIST_INDEX + 1)

class nsComboboxControlFrame : public nsAreaFrame,
                               public nsIFormControlFrame,
                               public nsIComboboxControlFrame,
                               public nsIAnonymousContentCreator,
                               public nsISelectControlFrame,
			                         public nsIStatefulFrame,
                               public nsIRollupListener
{
public:
  friend nsresult NS_NewComboboxControlFrame(nsIPresShell* aPresShell, nsIFrame** aNewFrame, PRUint32 aFlags);

  nsComboboxControlFrame();
  ~nsComboboxControlFrame();

   // nsISupports
  NS_IMETHOD QueryInterface(const nsIID& aIID, void** aInstancePtr);
  
   // nsIAnonymousContentCreator
  NS_IMETHOD CreateAnonymousContent(nsISupportsArray& aChildList);

   // nsIFrame
  NS_IMETHOD Init(nsIPresContext*  aPresContext,
              nsIContent*      aContent,
              nsIFrame*        aParent,
              nsIStyleContext* aContext,
              nsIFrame*        aPrevInFlow);

  NS_IMETHOD Reflow(nsIPresContext*          aCX,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus);

  NS_IMETHOD HandleEvent(nsIPresContext* aPresContext,
                         nsGUIEvent* aEvent,
                         nsEventStatus* aEventStatus);

#ifdef NS_DEBUG
  NS_IMETHOD GetFrameName(nsString& aResult) const;
#endif
  NS_IMETHOD Destroy(nsIPresContext* aPresContext);
  NS_IMETHOD FirstChild(nsIAtom* aListName, nsIFrame** aFirstChild) const;
  NS_IMETHOD SetInitialChildList(nsIPresContext* aPresContext,
                               nsIAtom*        aListName,
                               nsIFrame*       aChildList);
  NS_IMETHOD GetAdditionalChildListName(PRInt32   aIndex,
                                        nsIAtom** aListName) const;

  NS_IMETHOD GetFrameForPoint(nsIPresContext* aPresContext, const nsPoint& aPoint, nsIFrame** aFrame);

     // nsIFormControlFrame
  NS_IMETHOD SetSuggestedSize(nscoord aWidth, nscoord aHeight);
  NS_IMETHOD GetName(nsString* aName);
  NS_IMETHOD GetType(PRInt32* aType) const;
  NS_IMETHOD SetProperty(nsIPresContext* aPresContext, nsIAtom* aName, const nsString& aValue);
  NS_IMETHOD GetProperty(nsIAtom* aName, nsString& aValue); 
  void       SetFocus(PRBool aOn, PRBool aRepaint);
  void       ScrollIntoView(nsIPresContext* aPresContext);
  virtual void PostCreateWidget(nsIPresContext* aPresContext,
                                nscoord& aWidth,
                                nscoord& aHeight);
  virtual PRBool IsSuccessful(nsIFormControlFrame* aSubmitter);
  virtual void   SetFormFrame(nsFormFrame* aFormFrame) { mFormFrame = aFormFrame; }
  virtual void   Reset(nsIPresContext* aPresContext);
  virtual PRInt32 GetMaxNumValues();
  virtual PRBool GetNamesValues(PRInt32 aMaxNumValues, PRInt32& aNumValues,
                                nsString* aValues, nsString* aNames);
  NS_IMETHOD GetFont(nsIPresContext* aPresContext, 
                    nsFont&         aFont);
  NS_IMETHOD GetFormContent(nsIContent*& aContent) const;
  virtual nscoord GetVerticalBorderWidth(float aPixToTwip) const;
  virtual nscoord GetHorizontalBorderWidth(float aPixToTwip) const;
  virtual nscoord GetVerticalInsidePadding(nsIPresContext* aPresContext,
                                           float aPixToTwip,
                                           nscoord aInnerHeight) const;
  virtual nscoord GetHorizontalInsidePadding(nsIPresContext* aPresContext,
                                             float aPixToTwip, 
                                             nscoord aInnerWidth,
                                             nscoord aCharWidth) const;
  virtual nsresult RequiresWidget(PRBool &aRequiresWidget);

  NS_IMETHOD SelectionChanged();// Called when the selection has changed. 
                                                       // If the the same item in the list is selected
                                                       // it is NOT called.

  // nsIFormMouseListener
  virtual void MouseClicked(nsIPresContext* aPresContext);

  //nsIComboboxControlFrame
  NS_IMETHOD IsDroppedDown(PRBool * aDoDropDown) { *aDoDropDown = mDroppedDown; return NS_OK; }
  NS_IMETHOD ShowDropDown(PRBool aDoDropDown);
  NS_IMETHOD GetDropDown(nsIFrame** aDropDownFrame);
  NS_IMETHOD SetDropDown(nsIFrame* aDropDownFrame);
  NS_IMETHOD ListWasSelected(nsIPresContext* aPresContext);
  NS_IMETHOD UpdateSelection(PRBool aDoDispatchEvent, PRBool aForceUpdate, PRInt32 aNewIndex);
  NS_IMETHOD AbsolutelyPositionDropDown();
  NS_IMETHOD GetAbsoluteRect(nsRect* aRect);

  // nsISelectControlFrame
  NS_IMETHOD AddOption(nsIPresContext* aPresContext, PRInt32 index);
  NS_IMETHOD RemoveOption(nsIPresContext* aPresContext, PRInt32 index);
  NS_IMETHOD SetOptionSelected(PRInt32 aIndex, PRBool aValue);
  NS_IMETHOD GetOptionSelected(PRInt32 aIndex, PRBool* aValue);
  NS_IMETHOD DoneAddingContent(PRBool aIsDone);

  //nsIStatefulFrame
  NS_IMETHOD GetStateType(nsIPresContext* aPresContext, nsIStatefulFrame::StateType* aStateType);
  NS_IMETHOD SaveState(nsIPresContext* aPresContext, nsISupports** aState);
  NS_IMETHOD RestoreState(nsIPresContext* aPresContext, nsISupports* aState);

  //nsIRollupListener
  NS_IMETHOD Rollup();


protected:

   // nsHTMLContainerFrame
  virtual PRIntn GetSkipSides() const;

   // Utilities
  nsresult ReflowComboChildFrame(nsIFrame*           aFrame, 
                            nsIPresContext*          aPresContext, 
                            nsHTMLReflowMetrics&     aDesiredSize,
                            const nsHTMLReflowState& aReflowState, 
                            nsReflowStatus&          aStatus,
                            nscoord                  aAvailableWidth,
                            nscoord                  aAvailableHeight);

  nsresult GetScreenHeight(nsIPresContext* aPresContext, nscoord& aHeight);
public:
  nsresult PositionDropdown(nsIPresContext* aPresContext,
                            nscoord aHeight, 
                            nsRect aAbsoluteTwipsRect, 
                            nsRect aAbsolutePixelRect);
  nsresult GetAbsoluteFramePosition(nsIPresContext* aPresContext,
                                    nsIFrame *aFrame, 
                                    nsRect& aAbsoluteTwipsRect, 
                                    nsRect& aAbsolutePixelRect);
  nsIFrame* GetDisplayFrame(nsIPresContext* aPresContext);
protected:
  void ShowPopup(PRBool aShowPopup);
  void ShowList(nsIPresContext* aPresContext, PRBool aShowList);
  void SetChildFrameSize(nsIFrame* aFrame, nscoord aWidth, nscoord aHeight);
  void InitTextStr(nsIPresContext* aPresContext, PRBool aUpdate);
  nsresult GetPrimaryComboFrame(nsIPresContext* aPresContext, nsIContent* aContent, nsIFrame** aFrame);
  nsIFrame* GetButtonFrame(nsIPresContext* aPresContext);
  nsIFrame* GetDropdownFrame();
  NS_IMETHOD ToggleList(nsIPresContext* aPresContext);
  NS_IMETHOD MakeSureSomethingIsSelected(nsIPresContext* aPresContext); // Default to option 0
  PRBool ShouldScrollFrameIntoView(nsIPresShell * aShell, nsIPresContext * aPresContext, nsIFrame *aFrame);

  nsFrameList mPopupFrames;                       // additional named child list
  nsIPresContext*       mPresContext;             // XXX: Remove the need to cache the pres context.
  nsFormFrame*          mFormFrame;               // Parent Form Frame
  nsString              mTextStr;                 // Current Combo box selection
  PRInt32               mSelectedIndex;           // current selected index
  nsIHTMLContent*       mDisplayContent;          // Anonymous content used to display the current selection
  nsIHTMLContent*       mButtonContent;           // Anonymous content used to popup the dropdown list
  PRBool                mDroppedDown;             // Current state of the dropdown list, PR_TRUE is dropped down
  nsIFrame*             mDisplayFrame;            // frame to display selection
  nsIFrame*             mButtonFrame;             // button frame
  nsIFrame*             mDropdownFrame;           // dropdown list frame
  nsIListControlFrame * mListControlFrame;        // ListControl Interface for the dropdown frame
  PRBool                mIgnoreFocus;             // Tells the combo to ignore all focus notifications

  // Resize Reflow Optimization
  nsSize                mCacheSize;
  nsSize                mCachedMaxElementSize;

private:
  NS_IMETHOD_(nsrefcnt) AddRef() { return NS_OK; }
  NS_IMETHOD_(nsrefcnt) Release() { return NS_OK; }
};

#endif


