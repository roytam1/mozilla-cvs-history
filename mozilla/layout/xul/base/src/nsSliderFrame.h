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

//
// nsSliderFrame
//

#ifndef nsSliderFrame_h__
#define nsSliderFrame_h__


#include "nsBoxFrame.h"
#include "prtypes.h"
#include "nsIAtom.h"
#include "nsCOMPtr.h"
#include "nsIAnonymousContentCreator.h"
#include "nsITimerCallback.h"
#include "nsIDOMMouseListener.h"

class nsString;
class nsIScrollbarListener;
class nsISupportsArray;
class nsITimer;
class nsSliderFrame;

#define INITAL_REPEAT_DELAY 500
#define REPEAT_DELAY        50

nsresult NS_NewSliderFrame(nsIPresShell* aPresShell, nsIFrame** aResult) ;


class nsSliderMediator : public nsIDOMMouseListener, 
                         public nsITimerCallback
{
public:

  NS_DECL_ISUPPORTS

  nsSliderFrame* mSlider;

  nsSliderMediator(nsSliderFrame* aSlider) {  mSlider = aSlider; NS_INIT_ISUPPORTS(); }
  virtual ~nsSliderMediator() {}

  virtual void SetSlider(nsSliderFrame* aSlider) { mSlider = aSlider; }

 /**
  * Processes a mouse down event
  * @param aMouseEvent @see nsIDOMEvent.h 
  * @returns whether the event was consumed or ignored. @see nsresult
  */
  virtual nsresult MouseDown(nsIDOMEvent* aMouseEvent);

  /**
   * Processes a mouse up event
   * @param aMouseEvent @see nsIDOMEvent.h 
   * @returns whether the event was consumed or ignored. @see nsresult
   */
  virtual nsresult MouseUp(nsIDOMEvent* aMouseEvent);

  /**
   * Processes a mouse click event
   * @param aMouseEvent @see nsIDOMEvent.h 
   * @returns whether the event was consumed or ignored. @see nsresult
   *
   */
  virtual nsresult MouseClick(nsIDOMEvent* aMouseEvent) { return NS_OK; }

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


  NS_IMETHOD_(void) Notify(nsITimer *timer);

}; // class nsSliderFrame

class nsSliderFrame : public nsBoxFrame
{
public:
  nsSliderFrame(nsIPresShell* aShell);
  virtual ~nsSliderFrame();

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsString& aResult) const {
    return MakeFrameName("SliderFrame", aResult);
  }
#endif

  // nsIBox
  NS_IMETHOD GetPrefSize(nsBoxLayoutState& aBoxLayoutState, nsSize& aSize);
  NS_IMETHOD GetMinSize(nsBoxLayoutState& aBoxLayoutState, nsSize& aSize);
  NS_IMETHOD GetMaxSize(nsBoxLayoutState& aBoxLayoutState, nsSize& aSize);
  NS_IMETHOD Layout(nsBoxLayoutState& aBoxLayoutState);

  // nsIFrame overrides
  NS_IMETHOD Destroy(nsIPresContext* aPresContext);

  NS_IMETHOD Paint(nsIPresContext* aPresContext,
                   nsIRenderingContext& aRenderingContext,
                   const nsRect& aDirtyRect,
                   nsFramePaintLayer aWhichLayer);
 
    NS_IMETHOD AttributeChanged(nsIPresContext* aPresContext,
                              nsIContent* aChild,
                              PRInt32 aNameSpaceID,
                              nsIAtom* aAttribute,
                              PRInt32 aHint);

    virtual nsresult CurrentPositionChanged(nsIPresContext* aPresContext);

     NS_IMETHOD  Init(nsIPresContext*  aPresContext,
                   nsIContent*      aContent,
                   nsIFrame*        aParent,
                   nsIStyleContext* aContext,
                   nsIFrame*        asPrevInFlow);


   NS_IMETHOD HandleEvent(nsIPresContext* aPresContext, 
                         nsGUIEvent* aEvent,
                         nsEventStatus* aEventStatus);

  NS_IMETHOD GetFrameForPoint(nsIPresContext* aPresContext,
                              const nsPoint& aPoint,
                              nsFramePaintLayer aWhichLayer,    
                              nsIFrame**     aFrame);

    NS_IMETHOD SetInitialChildList(nsIPresContext* aPresContext,
                                 nsIAtom*        aListName,
                                 nsIFrame*       aChildList);

  virtual nsresult MouseDown(nsIDOMEvent* aMouseEvent);
  virtual nsresult MouseUp(nsIDOMEvent* aMouseEvent);

  virtual nsresult HandleEvent(nsIDOMEvent* aEvent) { return NS_OK; }

  static PRInt32 GetCurrentPosition(nsIContent* content);
  static PRInt32 GetMaxPosition(nsIContent* content);
  static PRInt32 GetIncrement(nsIContent* content);
  static PRInt32 GetPageIncrement(nsIContent* content);
  static PRInt32 GetIntegerAttribute(nsIContent* content, nsIAtom* atom, PRInt32 defaultValue);
  void EnsureOrient();

  void SetScrollbarListener(nsIScrollbarListener* aListener);


  NS_IMETHOD HandlePress(nsIPresContext* aPresContext,
                         nsGUIEvent *    aEvent,
                         nsEventStatus*  aEventStatus);

  NS_IMETHOD HandleMultiplePress(nsIPresContext* aPresContext,
                         nsGUIEvent *    aEvent,
                         nsEventStatus*  aEventStatus)  { return NS_OK; }

  NS_IMETHOD HandleDrag(nsIPresContext* aPresContext,
                        nsGUIEvent *    aEvent,
                        nsEventStatus*  aEventStatus)  { return NS_OK; }

  NS_IMETHOD HandleRelease(nsIPresContext* aPresContext,
                           nsGUIEvent *    aEvent,
                           nsEventStatus*  aEventStatus);

  NS_IMETHOD_(void) Notify(nsITimer *timer);
  friend nsSliderMediator;

protected:

  virtual PRIntn GetSkipSides() const { return 0; }

 
private:

  nsIBox* GetScrollbar();
  void GetContentOf(nsIBox* aBox, nsIContent** aContent);

  void PageUpDown(nsIFrame* aThumbFrame, nscoord change);
  void SetCurrentPosition(nsIContent* scrollbar, nsIFrame* aThumbFrame, nscoord pos);
  NS_IMETHOD DragThumb(nsIPresContext* aPresContext, PRBool aGrabMouseEvents);
  void AddListener();
  void RemoveListener();
  PRBool isDraggingThumb(nsIPresContext* aPresContext);

  float mRatio;

  nscoord mDragStartPx;
  nscoord mThumbStart;

  PRInt32 mCurPos;

  nsIScrollbarListener* mScrollbarListener;

  // XXX Hack
  nsIPresContext* mPresContext;  // weak reference

  nscoord mChange;
  nsPoint mClickPoint;
  PRBool mRedrawImmediate;
  nsSliderMediator* mMediator;

}; // class nsSliderFrame

#endif
