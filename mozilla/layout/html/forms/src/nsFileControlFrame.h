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

#ifndef nsFileControlFrame_h___
#define nsFileControlFrame_h___

#include "nsAreaFrame.h"
#include "nsIFormControlFrame.h"
#include "nsIDOMMouseListener.h"
#include "nsIAnonymousContentCreator.h"
#include "nsIStatefulFrame.h"

class nsButtonControlFrame;
class nsTextControlFrame;
class nsFormFrame;
class nsISupportsArray;
class nsIHTMLContent;

class nsFileControlFrame : public nsAreaFrame,
                           public nsIFormControlFrame,
                           public nsIDOMMouseListener,
                           public nsIAnonymousContentCreator,
			   public nsIStatefulFrame

{
public:
  nsFileControlFrame();
  virtual ~nsFileControlFrame();

  // XXX Hack so we can squirrel away the pres context pointer
  NS_IMETHOD Init(nsIPresContext&  aPresContext,
                  nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIStyleContext* aContext,
                  nsIFrame*        aPrevInFlow) {
    mPresContext = &aPresContext;
    return nsAreaFrame::Init(aPresContext, aContent, aParent, aContext, aPrevInFlow);
  }

  virtual void MouseClicked(nsIPresContext* aPresContext) {}
  virtual void SetFormFrame(nsFormFrame* aFormFrame) { mFormFrame = aFormFrame; }
  virtual nsFormFrame* GetFromFrame() { return mFormFrame; }

  NS_IMETHOD Paint(nsIPresContext& aPresContext,
                   nsIRenderingContext& aRenderingContext,
                   const nsRect& aDirtyRect,
                   nsFramePaintLayer aWhichLayer);

      // nsIFormControlFrame
  NS_IMETHOD SetProperty(nsIPresContext* aPresContext, nsIAtom* aName, const nsString& aValue);
  NS_IMETHOD GetProperty(nsIAtom* aName, nsString& aValue); 
  NS_IMETHOD QueryInterface(const nsIID& aIID, void** aInstancePtr);

  NS_IMETHOD Reflow(nsIPresContext&          aCX,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus);

#ifdef NS_DEBUG
  NS_IMETHOD GetFrameName(nsString& aResult) const;
#endif
  NS_IMETHOD SetSuggestedSize(nscoord aWidth, nscoord aHeight) { return NS_OK; };
  NS_IMETHOD GetFrameForPoint(nsIPresContext* aPresContext, const nsPoint& aPoint, nsIFrame** aFrame);
  NS_IMETHOD AttributeChanged(nsIPresContext* aPresContext,
                              nsIContent*     aChild,
                              PRInt32         aNameSpaceID,
                              nsIAtom*        aAttribute,
                              PRInt32         aHint);

  virtual PRInt32 GetMaxNumValues();

  virtual PRBool GetNamesValues(PRInt32 aMaxNumValues, PRInt32& aNumValues,
                                nsString* aValues, nsString* aNames);

  NS_IMETHOD     GetName(nsString* aName);
  virtual PRBool IsSuccessful(nsIFormControlFrame* aSubmitter);
  virtual void   Reset(nsIPresContext* aPresContext);
  NS_IMETHOD     GetType(PRInt32* aType) const;
  void           SetFocus(PRBool aOn, PRBool aRepaint);
  void           ScrollIntoView(nsIPresContext* aPresContext);

  NS_IMETHOD GetFont(nsIPresContext* aPresContext, 
                    nsFont&         aFont);

  NS_IMETHOD GetFormContent(nsIContent*& aContent) const;
  virtual nscoord GetVerticalInsidePadding(nsIPresContext& aPresContext,
                                           float aPixToTwip,
                                           nscoord aInnerHeight) const;
  virtual nscoord GetHorizontalInsidePadding(nsIPresContext& aPresContext,
                                             float aPixToTwip, 
                                             nscoord aInnerWidth,
                                             nscoord aCharWidth) const;

  virtual nsresult RequiresWidget(PRBool &aRequiresWidget);


  // from nsIAnonymousContentCreator
  NS_IMETHOD CreateAnonymousContent(nsISupportsArray& aChildList);


  // mouse events when out browse button is pressed
  /**
  * Processes a mouse down event
  * @param aMouseEvent @see nsIDOMEvent.h 
  * @returns whether the event was consumed or ignored. @see nsresult
  */
  virtual nsresult MouseDown(nsIDOMEvent* aMouseEvent) { return NS_OK; }

  /**
   * Processes a mouse up event
   * @param aMouseEvent @see nsIDOMEvent.h 
   * @returns whether the event was consumed or ignored. @see nsresult
   */
  virtual nsresult MouseUp(nsIDOMEvent* aMouseEvent) { return NS_OK; }

  /**
   * Processes a mouse click event
   * @param aMouseEvent @see nsIDOMEvent.h 
   * @returns whether the event was consumed or ignored. @see nsresult
   *
   */
  virtual nsresult MouseClick(nsIDOMEvent* aMouseEvent); // we only care when the button is clicked

  /**
   * Processes a mouse click event
   * @param aMouseEvent @see nsIDOMEvent.h 
   * @returns whether the event was consumed or ignored. @see nsresult
   *
   */
  virtual nsresult MouseDblClick(nsIDOMEvent* aMouseEvent) { return NS_OK; }

  /**
   * Processes a mouse enter event
   * @param aMouseEvent @see nsIDOMEvent.h 
   * @returns whether the event was consumed or ignored. @see nsresult
   */
  virtual nsresult MouseOver(nsIDOMEvent* aMouseEvent) { return NS_OK; }

  /**
   * Processes a mouse leave event
   * @param aMouseEvent @see nsIDOMEvent.h 
   * @returns whether the event was consumed or ignored. @see nsresult
   */
  virtual nsresult MouseOut(nsIDOMEvent* aMouseEvent) { return NS_OK; }

  virtual nsresult HandleEvent(nsIDOMEvent* aEvent) { return NS_OK; }

  //nsIStatefulFrame
  NS_IMETHOD GetStateType(nsIPresContext* aPresContext, nsIStatefulFrame::StateType* aStateType);
  NS_IMETHOD SaveState(nsIPresContext* aPresContext, nsISupports** aState);
  NS_IMETHOD RestoreState(nsIPresContext* aPresContext, nsISupports* aState);

protected:
  nsIWidget* GetWindowTemp(nsIView *aView); // XXX temporary

  virtual PRIntn GetSkipSides() const;

  nsTextControlFrame* mTextFrame;
  nsFormFrame*        mFormFrame;
  nsIHTMLContent*     mTextContent;
  nsString*           mCachedState;
  // XXX Hack: pres context needed by function MouseClick()
  nsIPresContext*     mPresContext;  // weak reference

private:
  nsTextControlFrame* GetTextControlFrame(nsIFrame* aStart);

  NS_IMETHOD_(nsrefcnt) AddRef() { return NS_OK; }
  NS_IMETHOD_(nsrefcnt) Release() { return NS_OK; }
};

#endif


