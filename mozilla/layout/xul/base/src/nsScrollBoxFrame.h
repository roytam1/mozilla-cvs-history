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
#ifndef nsScrollBoxFrame_h___
#define nsScrollBoxFrame_h___

#include "nsBoxFrame.h"
#include "nsIStatefulFrame.h"
#include "nsGUIEvent.h"

/**
 * The scroll frame creates and manages the scrolling view
 *
 * It only supports having a single child frame that typically is an area
 * frame, but doesn't have to be. The child frame must have a view, though
 *
 * Scroll frames don't support incremental changes, i.e. you can't replace
 * or remove the scrolled frame
 */
class nsScrollBoxFrame : public nsBoxFrame, public nsIStatefulFrame {
public:
  friend nsresult NS_NewScrollBoxFrame(nsIPresShell* aPresShell, nsIFrame** aNewFrame);

  NS_IMETHOD Init(nsIPresContext*  aPresContext,
                  nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIStyleContext* aContext,
                  nsIFrame*        aPrevInFlow);

  // Called to set the one and only child frame. Returns NS_ERROR_INVALID_ARG
  // if the child frame is NULL, and NS_ERROR_UNEXPECTED if the child list
  // contains more than one frame
  NS_IMETHOD SetInitialChildList(nsIPresContext* aPresContext,
                                 nsIAtom*        aListName,
                                 nsIFrame*       aChildList);

  // Because there can be only one child frame, these two function return
  // NS_ERROR_FAILURE
  NS_IMETHOD AppendFrames(nsIPresContext* aPresContext,
                          nsIPresShell&   aPresShell,
                          nsIAtom*        aListName,
                          nsIFrame*       aFrameList);
  NS_IMETHOD InsertFrames(nsIPresContext* aPresContext,
                          nsIPresShell&   aPresShell,
                          nsIAtom*        aListName,
                          nsIFrame*       aPrevFrame,
                          nsIFrame*       aFrameList);

  // This function returns NS_ERROR_NOT_IMPLEMENTED
  NS_IMETHOD RemoveFrame(nsIPresContext* aPresContext,
                         nsIPresShell&   aPresShell,
                         nsIAtom*        aListName,
                         nsIFrame*       aOldFrame);


  NS_IMETHOD Paint(nsIPresContext*      aPresContext,
                   nsIRenderingContext& aRenderingContext,
                   const nsRect&        aDirtyRect,
                   nsFramePaintLayer    aWhichLayer);

  /**
   * Get the "type" of the frame
   *
   * @see nsLayoutAtoms::scrollFrame
   */
  NS_IMETHOD GetFrameType(nsIAtom** aType) const;
  
#ifdef NS_DEBUG
  NS_IMETHOD GetFrameName(nsString& aResult) const;
#endif

  // nsIBox methods
  NS_DECL_ISUPPORTS

  NS_IMETHOD GetPrefSize(nsBoxLayoutState& aBoxLayoutState, nsSize& aSize);
  NS_IMETHOD GetMinSize(nsBoxLayoutState& aBoxLayoutState, nsSize& aSize);
  NS_IMETHOD GetMaxSize(nsBoxLayoutState& aBoxLayoutState, nsSize& aSize);
  NS_IMETHOD GetAscent(nsBoxLayoutState& aBoxLayoutState, nscoord& aAscent);
  NS_IMETHOD DoLayout(nsBoxLayoutState& aBoxLayoutState);
  NS_IMETHOD GetPadding(nsMargin& aMargin);
  NS_IMETHOD GetBorder(nsMargin& aMargin);
  NS_IMETHOD GetMargin(nsMargin& aMargin);

  virtual nsresult GetContentOf(nsIContent** aContent);
  

protected:
  nsScrollBoxFrame(nsIPresShell* aShell);
  virtual PRIntn GetSkipSides() const;

   // Creation of the widget for the scrolling view is factored into a virtual method so
   // that sub-classes may control widget creation.
  virtual nsresult CreateScrollingViewWidget(nsIView* aView,const nsStylePosition* aPosition);
   // Getting the view for scollframe may be overriden to provide a parent view for te scroll frame
  virtual nsresult GetScrollingParentView(nsIPresContext* aPresContext,
                                          nsIFrame* aParent,
                                          nsIView** aParentView);

  //nsIStatefulFrame
  NS_IMETHOD GetStateType(nsIPresContext* aPresContext, nsIStatefulFrame::StateType* aStateType);
  NS_IMETHOD SaveState(nsIPresContext* aPresContext, nsIPresState** aState);
  NS_IMETHOD RestoreState(nsIPresContext* aPresContext, nsIPresState* aState);

private:
  nsresult CreateScrollingView(nsIPresContext* aPresContext);
  PRPackedBool mVerticalOverflow;
  PRPackedBool mHorizontalOverflow;
  nsRect mRestoreRect;
  
protected:
  virtual PRBool NeedsClipWidget();
  virtual void PostScrollPortEvent(nsIPresShell* aShell, PRBool aOverflow, nsScrollPortEvent::orientType aType);
  virtual void SetUpScrolledFrame(nsIPresContext* aPresContext);
};

#endif /* nsScrollBoxFrame_h___ */
