/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License.  You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are Copyright (C) 1998
 * Netscape Communications Corporation.  All Rights Reserved.
 */
#ifndef nsListControlFrame_h___
#define nsListControlFrame_h___

#include "nsScrollFrame.h"
#include "nsIFormControlFrame.h"
#include "nsIListControlFrame.h"

class nsIDOMHTMLSelectElement;
class nsIDOMHTMLCollection;
class nsIDOMHTMLOptionElement;
class nsIComboboxControlFrame;

/**
 * Frame-based listbox.
 */

class nsListControlFrame : public nsScrollFrame, 
                           public nsIFormControlFrame, 
                           public nsIListControlFrame
{
public:
  friend nsresult NS_NewListControlFrame(nsIFrame** aNewFrame);

   // nsISupports
  NS_DECL_ISUPPORTS

  NS_IMETHOD GetFrameForPoint(const nsPoint& aPoint, nsIFrame** aFrame);

  NS_IMETHOD  HandleEvent(nsIPresContext& aPresContext,
                          nsGUIEvent* aEvent,
                          nsEventStatus& aEventStatus);
  // nsIFrame
 
  NS_IMETHOD SetInitialChildList(nsIPresContext& aPresContext,
                                 nsIAtom*        aListName,
                                 nsIFrame*       aChildList);

  NS_IMETHOD Reflow(nsIPresContext&          aCX,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus);

  NS_IMETHOD  Init(nsIPresContext&  aPresContext,
                   nsIContent*      aContent,
                   nsIFrame*        aParent,
                   nsIStyleContext* aContext,
                   nsIFrame*        aPrevInFlow);

  NS_IMETHOD Deselect();

      // nsIFormControlFrame
  NS_IMETHOD SetProperty(nsIAtom* aName, const nsString& aValue);
  NS_IMETHOD GetProperty(nsIAtom* aName, nsString& aValue); 

  NS_METHOD GetMultiple(PRBool* aResult, nsIDOMHTMLSelectElement* aSelect = nsnull);

  virtual nscoord GetVerticalInsidePadding(float aPixToTwip,
                                           nscoord aInnerHeight) const;
  virtual nscoord GetHorizontalInsidePadding(nsIPresContext& aPresContext,
                                             float aPixToTwip, 
                                             nscoord aInnerWidth,
                                             nscoord aCharWidth) const;

  virtual nsresult RequiresWidget(PRBool &aRequiresWidget);

  NS_IMETHOD GetFont(nsIPresContext* aPresContext, 
                    nsFont&         aFont);
  NS_IMETHOD GetFormContent(nsIContent*& aContent) const;


  /////////////////////////
  // nsHTMLContainerFrame
  /////////////////////////
  virtual PRIntn GetSkipSides() const;

  /////////////////////////
  // nsIFormControlFrame
  /////////////////////////
  NS_IMETHOD GetType(PRInt32* aType) const;
  NS_IMETHOD GetName(nsString* aName);

  virtual void SetFocus(PRBool aOn = PR_TRUE, PRBool aRepaint = PR_FALSE);
  virtual void MouseClicked(nsIPresContext* aPresContext);
  virtual void Reset();
  virtual PRBool IsSuccessful(nsIFormControlFrame* aSubmitter);
  virtual PRInt32 GetMaxNumValues();
  virtual PRBool  GetNamesValues(PRInt32 aMaxNumValues, PRInt32& aNumValues,
                                 nsString* aValues, nsString* aNames);
  virtual void SetFormFrame(nsFormFrame* aFrame);
 
  // nsIListControlFrame
  NS_IMETHOD SetComboboxFrame(nsIFrame* aComboboxFrame);
  NS_IMETHOD GetSelectedItem(nsString & aStr);
  NS_IMETHOD AboutToDropDown();
  NS_IMETHOD CaptureMouseEvents(PRBool aGrabMouseEvents);

  // Static Methods
  static nsIDOMHTMLSelectElement* GetSelect(nsIContent * aContent);
  static nsIDOMHTMLCollection*    GetOptions(nsIContent * aContent, nsIDOMHTMLSelectElement* aSelect = nsnull);
  static nsIDOMHTMLOptionElement* GetOption(nsIDOMHTMLCollection& aOptions, PRUint32 aIndex);
  static nsIContent* GetOptionAsContent(nsIDOMHTMLCollection* aCollection,PRUint32 aIndex);
  static PRBool                   GetOptionValue(nsIDOMHTMLCollection& aCollecton, PRUint32 aIndex, nsString& aValue);

  nsIContent* GetOptionContent(PRUint32 aIndex);
  PRBool IsContentSelected(nsIContent* aContent);
  PRBool IsFrameSelected(PRUint32 aIndex);
  void   SetFrameSelected(PRUint32 aIndex, PRBool aSelected);
 
protected:
   // nsScrollFrame overrides
   // Override the widget created for the list box so a Borderless top level widget is created
   // for drop-down lists.
  virtual nsresult CreateScrollingViewWidget(nsIView* aView,const nsStylePosition* aPosition);

  nsListControlFrame();
  virtual ~nsListControlFrame();

  PRInt32 GetNumberOfOptions();

  nsIFrame * GetOptionFromChild(nsIFrame* aParentFrame);

  nsresult GetFrameForPointUsing(const nsPoint& aPoint,
                                 nsIAtom*       aList,
                                 nsIFrame**     aFrame);

  // Utility methods
  PRBool IsInDropDownMode();
  PRBool IsOptionElement(nsIContent* aContent);
  PRBool IsOptionElementFrame(nsIFrame *aFrame);
  nsIFrame *GetSelectableFrame(nsIFrame *aFrame);
  void DisplaySelected(nsIContent* aContent); 
  void DisplayDeselected(nsIContent* aContent); 
  void ForceRedraw();
  PRBool IsOptionGroup(nsIFrame* aFrame);
  void SingleSelection();
  void MultipleSelection(PRBool aIsShift, PRBool aIsControl);
  void SelectIndex(PRInt32 aIndex); 
  void ToggleSelected(PRInt32 aIndex);
  void ClearSelection();
  void InitializeFromContent();
  void ExtendedSelection(PRInt32 aStartIndex, PRInt32 aEndIndex, PRBool aDoInvert, PRBool aSetValue);

  NS_IMETHOD HandleLikeDropDownListEvent(nsIPresContext& aPresContext, 
                                         nsGUIEvent*     aEvent,
                                         nsEventStatus&  aEventStatus);
  PRBool HasSameContent(nsIFrame* aFrame1, nsIFrame* aFrame2);
  void HandleListSelection(nsIPresContext& aPresContext, 
                           nsGUIEvent*     aEvent,
                           nsEventStatus&  aEventStatus);
  NS_IMETHOD HandleLikeListEvent(nsIPresContext& aPresContext, 
                                 nsGUIEvent*     aEvent,
                                 nsEventStatus&  aEventStatus);
  PRInt32 GetSelectedIndex(nsIFrame *aHitFrame);

  // Data Members
  nscoord      mBorderOffsetY;
  nsFormFrame* mFormFrame;
  PRInt32      mNumRows;
  PRInt32      mNumSelections;
  PRBool       mMultipleSelections;
  PRInt32      mSelectedIndex;
  PRInt32      mStartExtendedIndex;
  PRInt32      mEndExtendedIndex;
  nsIFrame*    mHitFrame;
  PRBool       mIsInitializedFromContent;
  nsIFrame*    mContentFrame;
  nsIComboboxControlFrame *mComboboxFrame;
  PRBool       mDisplayed;
  PRBool       mButtonDown;
  nsIFrame*    mLastFrame;
};

#endif /* nsListControlFrame_h___ */

