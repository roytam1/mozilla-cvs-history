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

//
// Eric Vaughan
// Netscape Communications
//
// See documentation in associated header file
//

#include "nsSliderFrame.h"
#include "nsIStyleContext.h"
#include "nsIPresContext.h"
#include "nsIContent.h"
#include "nsCOMPtr.h"
#include "nsHTMLIIDs.h"
#include "nsUnitConversion.h"
#include "nsINameSpaceManager.h"
#include "nsXULAtoms.h"
#include "nsHTMLAtoms.h"
#include "nsIReflowCommand.h"
#include "nsHTMLParts.h"
#include "nsIPresShell.h"
#include "nsStyleChangeList.h"
#include "nsCSSRendering.h"
#include "nsHTMLAtoms.h"
#include "nsIDOMEventReceiver.h"
#include "nsIViewManager.h"
#include "nsIDOMMouseEvent.h"
#include "nsDocument.h"
#include "nsTitledButtonFrame.h"
#include "nsScrollbarButtonFrame.h"
#include "nsIScrollbarListener.h"
#include "nsISupportsArray.h"
#include "nsIXMLContent.h"
#include "nsXULAtoms.h"
#include "nsHTMLAtoms.h"
#include "nsINameSpaceManager.h"
#include "nsIScrollableView.h"
#include "nsRepeatService.h"
#include "nsBoxLayoutState.h"

#define DEBUG_SLIDER PR_FALSE


nsresult
NS_NewSliderFrame ( nsIPresShell* aPresShell, nsIFrame** aNewFrame)
{
  NS_PRECONDITION(aNewFrame, "null OUT ptr");
  if (nsnull == aNewFrame) {
    return NS_ERROR_NULL_POINTER;
  }
  nsSliderFrame* it = new (aPresShell) nsSliderFrame(aPresShell);
  if (nsnull == it)
    return NS_ERROR_OUT_OF_MEMORY;

  *aNewFrame = it;
  return NS_OK;
  
} // NS_NewSliderFrame

nsSliderFrame::nsSliderFrame(nsIPresShell* aPresShell):nsBoxFrame(aPresShell),
 mCurPos(0), mScrollbarListener(nsnull),mChange(0)
{
}

// stop timer
nsSliderFrame::~nsSliderFrame()
{
   mRedrawImmediate = PR_FALSE;
}

NS_IMETHODIMP
nsSliderFrame::Init(nsIPresContext*  aPresContext,
              nsIContent*      aContent,
              nsIFrame*        aParent,
              nsIStyleContext* aContext,
              nsIFrame*        aPrevInFlow)
{
  nsresult  rv = nsBoxFrame::Init(aPresContext, aContent, aParent, aContext, aPrevInFlow);
  CreateViewForFrame(aPresContext,this,aContext,PR_TRUE);
  nsIView* view;
  GetView(aPresContext, &view);
  view->SetContentTransparency(PR_TRUE);
  // XXX Hack
  mPresContext = aPresContext;
  return rv;
}

PRInt32
nsSliderFrame::GetCurrentPosition(nsIContent* content)
{
  return GetIntegerAttribute(content, nsXULAtoms::curpos, 0);
}

PRInt32
nsSliderFrame::GetMaxPosition(nsIContent* content)
{
  return GetIntegerAttribute(content, nsXULAtoms::maxpos, 100);
}

PRInt32
nsSliderFrame::GetIncrement(nsIContent* content)
{
  return GetIntegerAttribute(content, nsXULAtoms::increment, 1);
}


PRInt32
nsSliderFrame::GetPageIncrement(nsIContent* content)
{
  return GetIntegerAttribute(content, nsXULAtoms::pageincrement, 10);
}

PRInt32
nsSliderFrame::GetIntegerAttribute(nsIContent* content, nsIAtom* atom, PRInt32 defaultValue)
{
    nsAutoString value;
    if (NS_CONTENT_ATTR_HAS_VALUE == content->GetAttribute(kNameSpaceID_None, atom, value))
    {
      PRInt32 error;

      // convert it to an integer
      defaultValue = value.ToInteger(&error);
    }

    return defaultValue;
}


NS_IMETHODIMP
nsSliderFrame::AttributeChanged(nsIPresContext* aPresContext,
                               nsIContent* aChild,
                               PRInt32 aNameSpaceID,
                               nsIAtom* aAttribute,
                               PRInt32 aHint)
{
  nsresult rv = nsBoxFrame::AttributeChanged(aPresContext, aChild,
                                              aNameSpaceID, aAttribute, aHint);
  // if the current position changes
  if (aAttribute == nsXULAtoms::curpos) {
     rv = CurrentPositionChanged(aPresContext);
     NS_ASSERTION(NS_SUCCEEDED(rv), "failed to change position");
     if (NS_FAILED(rv))
        return rv;
  } else if (aAttribute == nsXULAtoms::maxpos) {
      // bounds check it.

      nsIBox* scrollbarBox = GetScrollbar();
      nsCOMPtr<nsIContent> scrollbar;
      GetContentOf(scrollbarBox, getter_AddRefs(scrollbar));
      PRInt32 current = GetCurrentPosition(scrollbar);      
      PRInt32 max = GetMaxPosition(scrollbar);
      if (current < 0 || current > max)
      {
          if (current < 0)
              current = 0;
          else if (current > max) 
              current = max;

          char ch[100];
          sprintf(ch,"%d", current);
 
          // set the new position but don't notify anyone. We already know
          scrollbar->SetAttribute(kNameSpaceID_None, nsXULAtoms::curpos, NS_ConvertASCIItoUCS2(ch), PR_FALSE);
      }
  }
  
  if ((aHint != NS_STYLE_HINT_REFLOW) && 
             (aAttribute == nsXULAtoms::maxpos || 
             aAttribute == nsXULAtoms::pageincrement ||
             aAttribute == nsXULAtoms::increment)) {
      nsCOMPtr<nsIPresShell> shell;
      aPresContext->GetShell(getter_AddRefs(shell));
 
      nsBoxLayoutState state(aPresContext);
      MarkDirtyChildren(state);
  }

  return rv;
}

NS_IMETHODIMP
nsSliderFrame::Paint(nsIPresContext* aPresContext,
                                nsIRenderingContext& aRenderingContext,
                                const nsRect& aDirtyRect,
                                nsFramePaintLayer aWhichLayer)
{
  // if we are too small to have a thumb don't paint it.
  nsIBox* thumb;
  GetChildBox(&thumb);

  NS_ASSERTION(thumb,"Slider does not have a thumb!!!!");

  nsRect thumbRect;
  thumb->GetBounds(thumbRect);
  nsMargin m;
  thumb->GetMargin(m);
  thumbRect.Inflate(m);

  nsRect rect;
  GetClientRect(rect);

  if (rect.width < thumbRect.width || rect.height < thumbRect.height)
  {
    if (NS_FRAME_PAINT_LAYER_BACKGROUND == aWhichLayer) {
    const nsStyleDisplay* disp = (const nsStyleDisplay*)
    mStyleContext->GetStyleData(eStyleStruct_Display);
    if (disp->IsVisibleOrCollapsed()) {
      const nsStyleColor* myColor = (const nsStyleColor*)
      mStyleContext->GetStyleData(eStyleStruct_Color);
      const nsStyleSpacing* mySpacing = (const nsStyleSpacing*)
      mStyleContext->GetStyleData(eStyleStruct_Spacing);
      nsRect rect(0, 0, mRect.width, mRect.height);
      nsCSSRendering::PaintBackground(aPresContext, aRenderingContext, this,
                                  aDirtyRect, rect, *myColor, *mySpacing, 0, 0);
      nsCSSRendering::PaintBorder(aPresContext, aRenderingContext, this,
                              aDirtyRect, rect, *mySpacing, mStyleContext, 0);
      }
    }
    return NS_OK;
  }
  
  return nsBoxFrame::Paint(aPresContext, aRenderingContext, aDirtyRect, aWhichLayer);
}

NS_IMETHODIMP
nsSliderFrame::Layout(nsBoxLayoutState& aState)
{
  EnsureOrient();

  if (mState & NS_STATE_DEBUG_WAS_SET) {
      if (mState & NS_STATE_SET_TO_DEBUG)
          SetDebug(aState, PR_TRUE);
      else
          SetDebug(aState, PR_FALSE);
  }

  // get the content area inside our borders
  nsRect clientRect(0,0,0,0);
  GetClientRect(clientRect);

  // get the scrollbar
  nsIBox* scrollbarBox = GetScrollbar();
  nsCOMPtr<nsIContent> scrollbar;
  GetContentOf(scrollbarBox, getter_AddRefs(scrollbar));
  PRBool isHorizontal = IsHorizontal();

  // get the thumb should be our only child
  nsIBox* thumbBox = nsnull;
  GetChildBox(&thumbBox);

  NS_ASSERTION(thumbBox,"Slider does not have a thumb!!!!");

  // get the thumb's pref size
  nsSize thumbSize(0,0);
  thumbBox->GetPrefSize(aState, thumbSize);

  if (isHorizontal)
    thumbSize.height = clientRect.height;
  else
    thumbSize.width = clientRect.width;

  // get our current position and max position from our content node
  PRInt32 curpospx = GetCurrentPosition(scrollbar);
  PRInt32 maxpospx = GetMaxPosition(scrollbar);

  if (curpospx < 0)
     curpospx = 0;
  else if (curpospx > maxpospx)
     curpospx = maxpospx;

  float p2t;
  aState.GetPresContext()->GetScaledPixelsToTwips(&p2t);
  nscoord onePixel = NSIntPixelsToTwips(1, p2t);

  /*
  if (aReflowState.mComputedHeight == NS_INTRINSICSIZE) 
    aDesiredSize.height = isHorizontal ? thumbSize.height : 200*onePixel;
  else {
    aDesiredSize.height = aReflowState.mComputedHeight;
   // if (aDesiredSize.height < thumbSize.height)
   //   aDesiredSize.height = thumbSize.height;
  }

  // set the width to the computed or if intrinsic then the width of the thumb.
  if (aReflowState.mComputedWidth == NS_INTRINSICSIZE) 
    aDesiredSize.width = isHorizontal ? 200*onePixel : thumbSize.width;
  else {
    aDesiredSize.width = aReflowState.mComputedWidth;
   // if (aDesiredSize.width < thumbSize.width)
   //   aDesiredSize.width = thumbSize.width;
  }
  */

  // get max pos in twips
  nscoord maxpos = maxpospx*onePixel;

  // get our maxpos in twips. This is the space we have left over in the scrollbar
  // after the height of the thumb has been removed
  nscoord& desiredcoord = isHorizontal ? clientRect.width : clientRect.height;
  nscoord& thumbcoord = isHorizontal ? thumbSize.width : thumbSize.height;

  nscoord ourmaxpos = desiredcoord; 

  mRatio = float(ourmaxpos)/float(maxpos + ourmaxpos);

  // if there is more room than the thumb need stretch the
  // thumb

  nscoord thumbsize = nscoord(ourmaxpos * mRatio);

  if (thumbsize > thumbcoord) {
    nscoord flex = 0;
    thumbBox->GetFlex(aState, flex);

    // if the thumb is flexible make the thumb bigger.
    if (flex > 0) {
       if (isHorizontal)
          thumbSize.width = nscoord(ourmaxpos * mRatio);
       else
          thumbSize.height = nscoord(ourmaxpos * mRatio);
    }    
  } else {
    ourmaxpos -= thumbcoord;
    mRatio = float(ourmaxpos)/float(maxpos);
  }

  nscoord curpos = curpospx*onePixel;

  // set the thumbs y coord to be the current pos * the ratio.
  nscoord pos = nscoord(float(curpos)*mRatio);
  nsRect thumbRect(clientRect.x, clientRect.y, thumbSize.width, thumbSize.height);
  
  if (isHorizontal)
    thumbRect.x += pos;
  else
    thumbRect.y += pos;

  LayoutChildAt(aState, thumbBox, thumbRect);

  SyncLayout(aState);

  if (DEBUG_SLIDER) {
     PRInt32 c = GetCurrentPosition(scrollbar);
     PRInt32 m = GetMaxPosition(scrollbar);
     printf("Current=%d, max=%d\n",c,m);
  }
  
  return NS_OK;
}


NS_IMETHODIMP
nsSliderFrame::HandleEvent(nsIPresContext* aPresContext, 
                                      nsGUIEvent* aEvent,
                                      nsEventStatus* aEventStatus)
{
  nsIBox* scrollbarBox = GetScrollbar();
  nsCOMPtr<nsIContent> scrollbar;
  GetContentOf(scrollbarBox, getter_AddRefs(scrollbar));
  PRBool isHorizontal = IsHorizontal();

  if (isDraggingThumb(aPresContext))
  {
      // we want to draw immediately if the user doing it directly with the
      // mouse that makes redrawing much faster.
      mRedrawImmediate = PR_TRUE;

    switch (aEvent->message) {
    case NS_MOUSE_MOVE: {
       // convert coord to pixels
      nscoord pos = isHorizontal ? aEvent->point.x : aEvent->point.y;

       // mDragStartPx is in pixels and is in our client areas coordinate system. 
       // so we need to first convert it so twips and then get it into our coordinate system.

       // convert start to twips
       nscoord startpx = mDragStartPx;
              
       float p2t;
       aPresContext->GetScaledPixelsToTwips(&p2t);
       nscoord onePixel = NSIntPixelsToTwips(1, p2t);
       nscoord start = startpx*onePixel;

       nsIFrame* thumbFrame = mFrames.FirstChild();


       // get it into our coordintate system by subtracting our parents offsets.
       nsIFrame* parent = this;
       while(parent != nsnull)
       {
          // if we hit a scrollable view make sure we take into account
          // how much we are scrolled.
          nsIScrollableView* scrollingView;
          nsIView*           view;
          parent->GetView(aPresContext, &view);
          if (view) {
            nsresult result = view->QueryInterface(NS_GET_IID(nsIScrollableView), (void**)&scrollingView);
            if (NS_SUCCEEDED(result)) {
                nscoord xoff = 0;
                nscoord yoff = 0;
                scrollingView->GetScrollPosition(xoff, yoff);
                isHorizontal ? start += xoff : start += yoff;         
            }
          }
       
         nsRect r;
         parent->GetRect(r);
         isHorizontal ? start -= r.x : start -= r.y;
         parent->GetParent(&parent);
       }

      //printf("Translated to start=%d\n",start);

       start -= mThumbStart;

       // take our current position and substract the start location
       pos -= start;

       // convert to pixels
       nscoord pospx = pos/onePixel;

       // convert to our internal coordinate system
       pospx = nscoord(pospx/mRatio);

       // set it
       SetCurrentPosition(scrollbar, thumbFrame, pospx);

    } 
    break;

    case NS_MOUSE_RIGHT_BUTTON_UP:
    case NS_MOUSE_LEFT_BUTTON_UP:
       // stop capturing
      //printf("stop capturing\n");
      AddListener();
      DragThumb(aPresContext, PR_FALSE);
    }

    // we want to draw immediately if the user doing it directly with the
    // mouse that makes redrawing much faster. Switch it back now.
    mRedrawImmediate = PR_FALSE;

    //return nsFrame::HandleEvent(aPresContext, aEvent, aEventStatus);
    return NS_OK;
  }

  // XXX hack until handle release is actually called in nsframe.
  if (aEvent->message == NS_MOUSE_EXIT_SYNTH || aEvent->message == NS_MOUSE_RIGHT_BUTTON_UP || aEvent->message == NS_MOUSE_LEFT_BUTTON_UP)
     HandleRelease(aPresContext, aEvent, aEventStatus);

  return nsFrame::HandleEvent(aPresContext, aEvent, aEventStatus);
}



nsIBox*
nsSliderFrame::GetScrollbar()
{
  // if we are in a scrollbar then return the scrollbar's content node
  // if we are not then return ours.
   nsIFrame* scrollbar;
   nsScrollbarButtonFrame::GetParentWithTag(nsXULAtoms::scrollbar, this, scrollbar);

   if (scrollbar == nsnull)
       return this;

   nsIBox* ibox = nsnull;
   scrollbar->QueryInterface(NS_GET_IID(nsIBox), (void**)&ibox);

   if (ibox == nsnull)
       return this;

   return ibox;
}

void
nsSliderFrame::GetContentOf(nsIBox* aBox, nsIContent** aContent)
{
   nsIFrame* frame = nsnull;
   aBox->GetFrame(&frame);
   frame->GetContent(aContent);
}

void
nsSliderFrame::PageUpDown(nsIFrame* aThumbFrame, nscoord change)
{ 
  // on a page up or down get our page increment. We get this by getting the scrollbar we are in and
  // asking it for the current position and the page increment. If we are not in a scrollbar we will
  // get the values from our own node.
  nsIBox* scrollbarBox = GetScrollbar();
  nsCOMPtr<nsIContent> scrollbar;
  GetContentOf(scrollbarBox, getter_AddRefs(scrollbar));
  
  if (mScrollbarListener)
    mScrollbarListener->PagedUpDown(); // Let the listener decide our increment.

  nscoord pageIncrement = GetPageIncrement(scrollbar);
  PRInt32 curpos = GetCurrentPosition(scrollbar);
  SetCurrentPosition(scrollbar, aThumbFrame, curpos + change*pageIncrement);
}

// called when the current position changed and we need to update the thumb's location
nsresult
nsSliderFrame::CurrentPositionChanged(nsIPresContext* aPresContext)
{
  nsIBox* scrollbarBox = GetScrollbar();
  nsCOMPtr<nsIContent> scrollbar;
  GetContentOf(scrollbarBox, getter_AddRefs(scrollbar));

  PRBool isHorizontal = IsHorizontal();

    // get the current position
    PRInt32 curpos = GetCurrentPosition(scrollbar);

    // do nothing if the position did not change
    if (mCurPos == curpos)
        return NS_OK;

    // get our current position and max position from our content node
    PRInt32 maxpos = GetMaxPosition(scrollbar);

    if (curpos < 0)
      curpos = 0;
         else if (curpos > maxpos)
      curpos = maxpos;

    // convert to pixels
    float p2t;
    aPresContext->GetScaledPixelsToTwips(&p2t);
    nscoord onePixel = NSIntPixelsToTwips(1, p2t);

    nscoord curpospx = curpos*onePixel;

    // get the thumb's rect
    nsIFrame* thumbFrame = mFrames.FirstChild();
    nsRect thumbRect;
    thumbFrame->GetRect(thumbRect);

    // get our border and padding
    const nsStyleSpacing* spacing;
    nsresult rv = GetStyleData(eStyleStruct_Spacing,
                   (const nsStyleStruct*&) spacing);

    NS_ASSERTION(NS_SUCCEEDED(rv), "failed to get spacing");
    if (NS_FAILED(rv))
        return rv;

    nsMargin borderPadding(0,0,0,0);
    spacing->GetBorderPadding(borderPadding);
    
    // figure out the new rect
    nsRect newThumbRect(thumbRect);

    if (isHorizontal)
       newThumbRect.x = borderPadding.left + nscoord(float(curpospx)*mRatio);
    else
       newThumbRect.y = borderPadding.top + nscoord(float(curpospx)*mRatio);

    // set the rect
    thumbFrame->SetRect(aPresContext, newThumbRect);
    
    // figure out the union of the rect so we know what to redraw
    nsRect changeRect;
    changeRect.UnionRect(thumbRect, newThumbRect);

    // redraw just the change
    Invalidate(aPresContext, changeRect, mRedrawImmediate);

    if (mScrollbarListener)
      mScrollbarListener->PositionChanged(aPresContext, mCurPos, curpos);
    
    mCurPos = curpos;

    return NS_OK;
}

void
nsSliderFrame::SetCurrentPosition(nsIContent* scrollbar, nsIFrame* aThumbFrame, nscoord newpos)
{
  
   // get our current position and max position from our content node
  PRInt32 maxpos = GetMaxPosition(scrollbar);

  // get the new position and make sure it is in bounds
  if (newpos > maxpos)
      newpos = maxpos;
  else if (newpos < 0) 
      newpos = 0;

  char ch[100];
  sprintf(ch,"%d", newpos);

  // set the new position
  scrollbar->SetAttribute(kNameSpaceID_None, nsXULAtoms::curpos, NS_ConvertASCIItoUCS2(ch), PR_TRUE);

  if (DEBUG_SLIDER)
     printf("Current Pos=%s\n",ch);
  
}

NS_IMETHODIMP  nsSliderFrame::GetFrameForPoint(nsIPresContext* aPresContext,
                                             const nsPoint& aPoint, 
                                             nsFramePaintLayer aWhichLayer,
                                             nsIFrame**     aFrame)
{ 
  if (isDraggingThumb(aPresContext))
  {
    // XXX I assume it's better not to test for visibility here.
    *aFrame = this;
    return NS_OK;
  }

  if (!mRect.Contains(aPoint))
    return NS_ERROR_FAILURE;


  if (NS_SUCCEEDED(nsBoxFrame::GetFrameForPoint(aPresContext, aPoint, aWhichLayer, aFrame)))
    return NS_OK;

  // always return us (if visible)
  const nsStyleDisplay* disp = (const nsStyleDisplay*)
    mStyleContext->GetStyleData(eStyleStruct_Display);
  if (disp->IsVisible()) {
    *aFrame = this;
    return NS_OK;
  }

  return NS_ERROR_FAILURE;
}



NS_IMETHODIMP
nsSliderFrame::SetInitialChildList(nsIPresContext* aPresContext,
                                              nsIAtom*        aListName,
                                              nsIFrame*       aChildList)
{
  nsresult r = nsBoxFrame::SetInitialChildList(aPresContext, aListName, aChildList);

  AddListener();

  return r;
}

nsresult
nsSliderFrame::MouseDown(nsIDOMEvent* aMouseEvent)
{
  //printf("Begin dragging\n");
  
  PRBool isHorizontal = IsHorizontal();

  nsCOMPtr<nsIDOMMouseEvent> mouseEvent(do_QueryInterface(aMouseEvent));

  RemoveListener();
  DragThumb(mPresContext, PR_TRUE);
  PRInt32 c = 0;
  if (isHorizontal)
     mouseEvent->GetClientX(&c);
  else
     mouseEvent->GetClientY(&c);

  mDragStartPx = c;
  nsIFrame* thumbFrame = mFrames.FirstChild();
  nsRect thumbRect;
  thumbFrame->GetRect(thumbRect);

  if (isHorizontal)
     mThumbStart = thumbRect.x;
  else
     mThumbStart = thumbRect.y;
     
  //printf("Pressed mDragStartPx=%d\n",mDragStartPx);
  
  return NS_OK;
}

nsresult
nsSliderFrame::MouseUp(nsIDOMEvent* aMouseEvent)
{
 // printf("Finish dragging\n");

  return NS_OK;
}

NS_IMETHODIMP
nsSliderFrame :: DragThumb(nsIPresContext* aPresContext, PRBool aGrabMouseEvents)
{
    // get its view
  nsIView* view = nsnull;
  GetView(aPresContext, &view);
  nsCOMPtr<nsIViewManager> viewMan;
  PRBool result;

  if (view) {
    view->GetViewManager(*getter_AddRefs(viewMan));

    if (viewMan) {
      if (aGrabMouseEvents) {
        viewMan->GrabMouseEvents(view,result);
      } else {
        viewMan->GrabMouseEvents(nsnull,result);
      }
    }
  }

  return NS_OK;
}

PRBool
nsSliderFrame :: isDraggingThumb(nsIPresContext* aPresContext)
{
    // get its view
  nsIView* view = nsnull;
  GetView(aPresContext, &view);
  nsCOMPtr<nsIViewManager> viewMan;
  
  if (view) {
    view->GetViewManager(*getter_AddRefs(viewMan));

    if (viewMan) {
        nsIView* grabbingView;
        viewMan->GetMouseEventGrabber(grabbingView);
        if (grabbingView == view)
          return PR_TRUE;
    }
  }

  return PR_FALSE;
}

void
nsSliderFrame::AddListener()
{
  nsIFrame* thumbFrame = mFrames.FirstChild();
  nsCOMPtr<nsIContent> content;
  thumbFrame->GetContent(getter_AddRefs(content));

  nsCOMPtr<nsIDOMEventReceiver> reciever(do_QueryInterface(content));

  reciever->AddEventListenerByIID(this,NS_GET_IID(nsIDOMMouseListener));
}

void
nsSliderFrame::RemoveListener()
{
  nsIFrame* thumbFrame = mFrames.FirstChild();
  nsCOMPtr<nsIContent> content;
  thumbFrame->GetContent(getter_AddRefs(content));

  nsCOMPtr<nsIDOMEventReceiver> reciever(do_QueryInterface(content));

  reciever->RemoveEventListenerByIID(this,NS_GET_IID(nsIDOMMouseListener));
}


NS_INTERFACE_MAP_BEGIN(nsSliderFrame)
  NS_INTERFACE_MAP_ENTRY(nsIDOMMouseListener)
  NS_INTERFACE_MAP_ENTRY(nsITimerCallback)
NS_INTERFACE_MAP_END_INHERITING(nsBoxFrame)


NS_IMETHODIMP_(nsrefcnt) 
nsSliderFrame::AddRef(void)
{
  return NS_OK;
}

NS_IMETHODIMP_(nsrefcnt) 
nsSliderFrame::Release(void)
{
    return NS_OK;
}

NS_IMETHODIMP
nsSliderFrame::HandlePress(nsIPresContext* aPresContext, 
                     nsGUIEvent*     aEvent,
                     nsEventStatus*  aEventStatus)
{
  PRBool isHorizontal = IsHorizontal();

  nsIFrame* thumbFrame = mFrames.FirstChild();
  nsRect thumbRect;
  thumbFrame->GetRect(thumbRect);

    nscoord change = 1;
    
    if ((isHorizontal && aEvent->point.x < thumbRect.x) || (!isHorizontal && aEvent->point.y < thumbRect.y)) 
        change = -1;

    mChange = change;
    mClickPoint = aEvent->point;
    PageUpDown(thumbFrame, change);
    nsRepeatService::GetInstance()->Start(this);

  return NS_OK;
}

NS_IMETHODIMP 
nsSliderFrame::HandleRelease(nsIPresContext* aPresContext, 
                                 nsGUIEvent*     aEvent,
                                 nsEventStatus*  aEventStatus)
{
  nsRepeatService::GetInstance()->Stop();

  return NS_OK;
}

NS_IMETHODIMP
nsSliderFrame::Destroy(nsIPresContext* aPresContext)
{
  // Ensure our repeat service isn't going... it's possible that a scrollbar can disappear out
  // from under you while you're in the process of scrolling.
  nsRepeatService::GetInstance()->Stop();

  // XXX: HACK!  WORKAROUND FOR BUG 21571
  /*
    the root cause of the crash is that nsSliderFrame implements nsIDOMEventListener and passes 
    itself to nsEventListenerManager::AddEventListener().  
    nsEventListenerManager::AddEventListener() assumes it is passed an 
    object that is governed by ref-counting.  But nsSliderFrame is **not** a 
    ref-counted object, and it's lifetime is implicitly controlled by the lifetime 
    of the frame model.  By passing itself to 
    nsEventListenerManager::AddEventListener(), the slider is passing in a pointer 
    that can be yanked out from underneath the event listener manager.  When the 
    event listener manager is destroyed, it correctly tries to clean up any objects 
    still under it's control, including the already-deleted slider.

    This bug is only evident when a slider is the last focused object before deletion.  
    Calling RemoveListener() removes *this* from nsEventListenerManager,
    removing the worst symptom of the bug.

    The real solution is to create a ref-counted listener object for the 
    slider to hand off to nsEventListenerManager::AddEventListener().
    Part of that fix should be removing nsSliderFrame::AddRef and
    nsSliderFrame::Release, which were masking this problem.  Without those
    methods, we would have gotten assertions as soon as the first slider was passed
    to any interface that tried to refcount it.
  */
  RemoveListener();   // remove this line when 21571 is fixed properly

  // call base class Destroy()
  return nsBoxFrame::Destroy(aPresContext);
}

NS_IMETHODIMP
nsSliderFrame::GetPrefSize(nsBoxLayoutState& aState, nsSize& aSize)
{
  EnsureOrient();
  return nsBoxFrame::GetPrefSize(aState, aSize);
}

NS_IMETHODIMP
nsSliderFrame::GetMinSize(nsBoxLayoutState& aState, nsSize& aSize)
{
  EnsureOrient();

  // our min size is just our borders and padding
  return nsBox::GetMinSize(aState, aSize);
}

NS_IMETHODIMP
nsSliderFrame::GetMaxSize(nsBoxLayoutState& aState, nsSize& aSize)
{
  EnsureOrient();
  return nsBoxFrame::GetMaxSize(aState, aSize);
}

void
nsSliderFrame::EnsureOrient()
{
  nsIBox* scrollbarBox = GetScrollbar();

  nsIFrame* frame = nsnull;
  scrollbarBox->GetFrame(&frame);
  nsFrameState state;
  frame->GetFrameState(&state);

  PRBool isHorizontal = state & NS_STATE_IS_HORIZONTAL;
  if (isHorizontal)
      mState |= NS_STATE_IS_HORIZONTAL;
  else
      mState &= ~NS_STATE_IS_HORIZONTAL;
}


void 
nsSliderFrame::SetScrollbarListener(nsIScrollbarListener* aListener)
{
  // Don't addref/release this, since it's actually a frame.
  mScrollbarListener = aListener;
}

NS_IMETHODIMP_(void) nsSliderFrame::Notify(nsITimer *timer)
{ 
    PRBool stop = PR_FALSE;

    nsIFrame* thumbFrame = mFrames.FirstChild();
    nsRect thumbRect;
    thumbFrame->GetRect(thumbRect);

    PRBool isHorizontal = IsHorizontal();

    // see if the thumb has moved passed our original click point.
    // if it has we want to stop.
    if (isHorizontal) {
        if (mChange < 0) {
            if (thumbRect.x < mClickPoint.x) 
                stop = PR_TRUE;
        } else {
            if (thumbRect.x + thumbRect.width > mClickPoint.x)
                stop = PR_TRUE;
        }
    } else {
         if (mChange < 0) {
            if (thumbRect.y < mClickPoint.y) 
                stop = PR_TRUE;
        } else {
            if (thumbRect.y + thumbRect.height > mClickPoint.y)
                stop = PR_TRUE;
        }
    }


    if (stop) {
       nsRepeatService::GetInstance()->Stop();
    } else {
      PageUpDown(thumbFrame, mChange);
    }
}

class nsThumbFrame : public nsTitledButtonFrame
{
public:

  friend nsresult NS_NewThumbFrame(nsIPresShell* aPresShell, nsIFrame** aNewFrame);

  NS_IMETHOD HandlePress(nsIPresContext* aPresContext,
                         nsGUIEvent *    aEvent,
                         nsEventStatus*  aEventStatus) { return NS_OK; }

  NS_IMETHOD HandleMultiplePress(nsIPresContext* aPresContext,
                         nsGUIEvent *    aEvent,
                         nsEventStatus*  aEventStatus)  { return NS_OK; }

  NS_IMETHOD HandleDrag(nsIPresContext* aPresContext,
                        nsGUIEvent *    aEvent,
                        nsEventStatus*  aEventStatus)  { return NS_OK; }

  NS_IMETHOD HandleRelease(nsIPresContext* aPresContext,
                           nsGUIEvent *    aEvent,
                           nsEventStatus*  aEventStatus)  { return NS_OK; }

  nsThumbFrame(nsIPresShell* aPresShell):nsTitledButtonFrame(aPresShell) {}
};


nsresult
NS_NewThumbFrame ( nsIPresShell* aPresShell, nsIFrame** aNewFrame)
{
  NS_PRECONDITION(aNewFrame, "null OUT ptr");
  if (nsnull == aNewFrame) {
    return NS_ERROR_NULL_POINTER;
  }
  nsThumbFrame* it = new (aPresShell) nsThumbFrame(aPresShell);
  if (nsnull == it)
    return NS_ERROR_OUT_OF_MEMORY;

  *aNewFrame = it;
  return NS_OK;
  
} // NS_NewSliderFrame

