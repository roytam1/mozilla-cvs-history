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

#include "nsScrollingView.h"
#include "nsIWidget.h"
#include "nsUnitConversion.h"
#include "nsIViewManager.h"
#include "nsIPresContext.h"
#include "nsIScrollbar.h"
#include "nsIDeviceContext.h"
#include "nsGUIEvent.h"
#include "nsWidgetsCID.h"
#include "nsViewsCID.h"
#include "nsIScrollableView.h"
#include "nsIFrame.h"

static inline PRBool
ViewIsShowing(nsIView *aView)
{
  nsViewVisibility  visibility;

  aView->GetVisibility(visibility);
  return nsViewVisibility_kShow == visibility;
}

static NS_DEFINE_IID(kIScrollbarIID, NS_ISCROLLBAR_IID);
static NS_DEFINE_IID(kIScrollableViewIID, NS_ISCROLLABLEVIEW_IID);
static NS_DEFINE_IID(kWidgetCID, NS_CHILD_CID);
static NS_DEFINE_IID(kIViewIID, NS_IVIEW_IID);

//----------------------------------------------------------------------

class ScrollBarView : public nsView
{
public:
  ScrollBarView(nsScrollingView *aScrollingView);
  ~ScrollBarView();
  NS_IMETHOD  HandleEvent(nsGUIEvent *aEvent, PRUint32 aEventFlags, nsEventStatus &aStatus);
  NS_IMETHOD  SetPosition(nscoord x, nscoord y);
  NS_IMETHOD  SetDimensions(nscoord width, nscoord height, PRBool aPaint = PR_TRUE);

public:
  nsScrollingView *mScrollingView;
};

ScrollBarView :: ScrollBarView(nsScrollingView *aScrollingView)
{
  mScrollingView = aScrollingView;
}

ScrollBarView :: ~ScrollBarView()
{
}

NS_IMETHODIMP ScrollBarView :: HandleEvent(nsGUIEvent *aEvent, PRUint32 aEventFlags,
                                           nsEventStatus &aStatus)
{
  aStatus = nsEventStatus_eIgnore;

  switch (aEvent->message)
  {
    case NS_SCROLLBAR_POS:
    case NS_SCROLLBAR_PAGE_NEXT:
    case NS_SCROLLBAR_PAGE_PREV:
    case NS_SCROLLBAR_LINE_NEXT:
    case NS_SCROLLBAR_LINE_PREV:
      NS_ASSERTION((nsnull != mScrollingView), "HandleEvent() called after the ScrollingView has been destroyed.");
      if (nsnull != mScrollingView)
        mScrollingView->HandleScrollEvent(aEvent, aEventFlags);
      aStatus = nsEventStatus_eConsumeNoDefault;
      break;

    default:
      break;
  }

  return NS_OK;
}

NS_IMETHODIMP ScrollBarView :: SetPosition(nscoord x, nscoord y)
{
  mBounds.MoveTo(x, y);

  if (nsnull != mWindow)
  {
    nsIDeviceContext  *dx;
    float             twipToPix;
    nscoord           parx = 0, pary = 0;
    nsIWidget         *pwidget = nsnull;

    mViewManager->GetDeviceContext(dx);
    dx->GetAppUnitsToDevUnits(twipToPix);  

    GetOffsetFromWidget(&parx, &pary, pwidget);
    NS_IF_RELEASE(pwidget);
    
    mWindow->Move(NSTwipsToIntPixels((x + parx), twipToPix),
                  NSTwipsToIntPixels((y + pary), twipToPix));

    NS_RELEASE(dx);
  }
  return NS_OK;
}

NS_IMETHODIMP ScrollBarView :: SetDimensions(nscoord width, nscoord height, PRBool aPaint)
{
  mBounds.SizeTo(width, height);

  if (nsnull != mWindow)
  {
    nsIDeviceContext  *dx;
    float             t2p;
  
    mViewManager->GetDeviceContext(dx);
    dx->GetAppUnitsToDevUnits(t2p);

    mWindow->Resize(NSTwipsToIntPixels(width, t2p), NSTwipsToIntPixels(height, t2p),
                    aPaint);

    NS_RELEASE(dx);
  }
  return NS_OK;
}

//----------------------------------------------------------------------

#if 0
class nsICornerWidget : public nsISupports {
public:
  NS_IMETHOD Init(nsIWidget* aParent, const nsRect& aBounds) = 0;
  NS_IMETHOD MoveTo(PRInt32 aX, PRInt32 aY) = 0;
  NS_IMETHOD Show() = 0;
  NS_IMETHOD Hide() = 0;
  NS_IMETHOD Start() = 0;
  NS_IMETHOD Stop() = 0;
};
#endif

class CornerView : public nsView
{
public:
  CornerView();
  ~CornerView();
  NS_IMETHOD  ShowQuality(PRBool aShow);
  NS_IMETHOD  SetQuality(nsContentQuality aQuality);
  NS_IMETHOD  Paint(nsIRenderingContext& rc, const nsRect& rect,
                    PRUint32 aPaintFlags, PRBool &Result);

  void  Show(PRBool aShow);

  PRBool            mShowQuality;
  nsContentQuality  mQuality;
  PRBool            mShow;
};

CornerView :: CornerView()
{
  mShowQuality = PR_FALSE;
  mQuality = nsContentQuality_kGood;
  mShow = PR_FALSE;
}

CornerView :: ~CornerView()
{
}

NS_IMETHODIMP CornerView :: ShowQuality(PRBool aShow)
{
  if (mShowQuality != aShow)
  {
    mShowQuality = aShow;

    if (mShow == PR_FALSE)
    {
      if (mVis == nsViewVisibility_kShow)
        mViewManager->SetViewVisibility(this, nsViewVisibility_kHide);
      else
        mViewManager->SetViewVisibility(this, nsViewVisibility_kShow);

      nscoord dimx, dimy;

      //this will force the scrolling view to recalc the scrollbar sizes... MMP

      mParent->GetDimensions(&dimx, &dimy);
      mParent->SetDimensions(dimx, dimy);
    }

    mViewManager->UpdateView(this, nsnull, NS_VMREFRESH_IMMEDIATE);
  }
  return NS_OK;
}

NS_IMETHODIMP CornerView :: SetQuality(nsContentQuality aQuality)
{
  if (mQuality != aQuality)
  {
    mQuality = aQuality;

    if (mVis == nsViewVisibility_kShow)
      mViewManager->UpdateView(this, nsnull, NS_VMREFRESH_IMMEDIATE);
  }
  return NS_OK;
}

void CornerView :: Show(PRBool aShow)
{
  if (mShow != aShow)
  {
    mShow = aShow;

    if (mShow == PR_TRUE)
      mViewManager->SetViewVisibility(this, nsViewVisibility_kShow);
    else if (mShowQuality == PR_FALSE)
      mViewManager->SetViewVisibility(this, nsViewVisibility_kHide);

    nscoord dimx, dimy;

    //this will force the scrolling view to recalc the scrollbar sizes... MMP

    mParent->GetDimensions(&dimx, &dimy);
    mParent->SetDimensions(dimx, dimy);
  }
}

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

NS_IMETHODIMP CornerView :: Paint(nsIRenderingContext& rc, const nsRect& rect,
                                  PRUint32 aPaintFlags, PRBool &aResult)
{
  PRBool  clipres = PR_FALSE;

  if (mVis == nsViewVisibility_kShow)
  {
    nsRect  brect;

    rc.PushState();
    GetBounds(brect);

    rc.SetClipRect(brect, nsClipCombine_kIntersect, clipres);

    if (clipres == PR_FALSE)
    {
      rc.SetColor(NS_RGB(192, 192, 192));
      rc.FillRect(brect);

      if (PR_TRUE == mShowQuality)
      {
        nscolor tcolor, bcolor;

        //display quality indicator

        rc.Translate(brect.x, brect.y);

        rc.SetColor(NS_RGB(0, 0, 0));

        rc.FillEllipse(NSToCoordFloor(brect.width * 0.15f),
                       NSToCoordFloor(brect.height * 0.15f),
                       NSToCoordRound(brect.width * 0.7f),    // XXX should use NSToCoordCeil ??
                       NSToCoordRound(brect.height * 0.7f));  // XXX should use NSToCoordCeil ??

        if (mQuality == nsContentQuality_kGood)
          rc.SetColor(NS_RGB(0, 255, 0));
        else if (mQuality == nsContentQuality_kFair)
          rc.SetColor(NS_RGB(255, 176, 0));
        else
          rc.SetColor(NS_RGB(255, 0, 0));

        //hey, notice that these numbers don't add up... that's because
        //something funny happens on windows when the *right* numbers are
        //used. MMP

        rc.FillEllipse(NSToCoordRound(brect.width * 0.23f),  // XXX should use NSToCoordCeil ??
                       NSToCoordRound(brect.height * 0.23f), // XXX should use NSToCoordCeil ??
                       nscoord(brect.width * 0.46f),
                       nscoord(brect.height * 0.46f));

        rc.GetColor(bcolor);
        tcolor = bcolor;

        //this is inefficient, but compact...

        tcolor = NS_RGB((int)min(NS_GET_R(bcolor) + 40, 255), 
                        (int)min(NS_GET_G(bcolor) + 40, 255),
                        (int)min(NS_GET_B(bcolor) + 40, 255));

        rc.SetColor(tcolor);

        rc.FillEllipse(NSToCoordRound(brect.width * 0.34f),  // XXX should use NSToCoordCeil ??
                       NSToCoordRound(brect.height * 0.34f), // XXX should use NSToCoordCeil ??
                       nscoord(brect.width * 0.28f),
                       nscoord(brect.height * 0.28f));

        tcolor = NS_RGB((int)min(NS_GET_R(bcolor) + 120, 255), 
                        (int)min(NS_GET_G(bcolor) + 120, 255),
                        (int)min(NS_GET_B(bcolor) + 120, 255));

        rc.SetColor(tcolor);

        rc.FillEllipse(NSToCoordRound(brect.width * 0.32f),  // XXX should use NSToCoordCeil ??
                       NSToCoordRound(brect.height * 0.32f), // XXX should use NSToCoordCeil ??
                       nscoord(brect.width * 0.17f),
                       nscoord(brect.height * 0.17f));
      }
    }

    rc.PopState(clipres);

    if (clipres == PR_FALSE)
      rc.SetClipRect(brect, nsClipCombine_kSubtract, clipres);
  }

  aResult = clipres;
  return NS_OK;
}

//----------------------------------------------------------------------

nsScrollingView :: nsScrollingView()
  : mInsets(0, 0, 0, 0)
{
  mSizeX = mSizeY = 0;
  mOffsetX = mOffsetY = 0;
  mClipView = nsnull;
  mVScrollBarView = nsnull;
  mHScrollBarView = nsnull;
  mCornerView = nsnull;
  mScrollPref = nsScrollPreference_kAuto;
  mScrollingTimer = nsnull;
}

nsScrollingView :: ~nsScrollingView()
{
  if (nsnull != mVScrollBarView)
  {
    // Clear the back-pointer from the scrollbar...
    ((ScrollBarView*)mVScrollBarView)->mScrollingView = nsnull;
  }

  if (nsnull != mHScrollBarView)
  {
    // Clear the back-pointer from the scrollbar...
    ((ScrollBarView*)mHScrollBarView)->mScrollingView = nsnull;
  }

  mClipView = nsnull;
  mCornerView = nsnull;

  if (nsnull != mScrollingTimer)
  {
    mScrollingTimer->Cancel();
    NS_RELEASE(mScrollingTimer);
  }
}

nsresult nsScrollingView :: QueryInterface(const nsIID& aIID, void** aInstancePtr)
{
  if (nsnull == aInstancePtr) {
    return NS_ERROR_NULL_POINTER;
  }
  
  if (aIID.Equals(kIScrollableViewIID)) {
    *aInstancePtr = (void*)(nsIScrollableView*)this;
    return NS_OK;
  }

  return nsView::QueryInterface(aIID, aInstancePtr);
}

nsrefcnt nsScrollingView :: AddRef()
{
  NS_WARNING("not supported for views");
  return 1;
}

nsrefcnt nsScrollingView :: Release()
{
  NS_WARNING("not supported for views");
  return 1;
}

NS_IMETHODIMP nsScrollingView :: SetDimensions(nscoord width, nscoord height, PRBool aPaint)
{
  nsIDeviceContext  *dx;
  mViewManager->GetDeviceContext(dx);
  float             scrollWidth, scrollHeight;
  dx->GetScrollBarDimensions(scrollWidth, scrollHeight);
  nscoord           showHorz = 0, showVert = 0;
  nsRect            clipRect;

  // Set our bounds and size our widget if we have one
  nsView::SetDimensions(width, height, aPaint);

  // Determine how much space is actually taken up by the scrollbars
  if (mHScrollBarView && ViewIsShowing(mHScrollBarView))
    showHorz = NSToCoordRound(scrollHeight);
  if (mVScrollBarView && ViewIsShowing(mVScrollBarView))
    showVert = NSToCoordRound(scrollWidth);

  // Compute the clip view rect
  clipRect.SetRect(0, 0, width - showVert, height - showHorz);
  clipRect.Deflate(mInsets);

  // Size and position the clip view
  if (nsnull != mClipView)
    mClipView->SetBounds(clipRect, aPaint);

  UpdateScrollControls(aPaint);

  //this will fix the size of the thumb when we resize the root window,
  //but unfortunately it will also cause scrollbar flashing. so long as
  //all resize operations happen through the viewmanager, this is not
  //an issue. we'll see. MMP
  ComputeContainerSize();

  NS_RELEASE(dx);
  return NS_OK;
}

NS_IMETHODIMP nsScrollingView :: SetPosition(nscoord aX, nscoord aY)
{
  // If we have a widget then there's no need to adjust child widgets,
  // because they're relative to our window
  if (nsnull != mWindow)
  {
    nsView::SetPosition(aX, aY);
  }
  else
  {
    nsIDeviceContext  *dx;
    nsIWidget         *thiswin;
    GetWidget(thiswin);
    float             t2p;
  
    if (nsnull == thiswin)
      GetOffsetFromWidget(nsnull, nsnull, thiswin);
  
    if (nsnull != thiswin)
      thiswin->BeginResizingChildren();
  
    nsView::SetPosition(aX, aY);
  
    mViewManager->GetDeviceContext(dx);
    dx->GetAppUnitsToDevUnits(t2p);
  
    // Adjust the positions of the scrollbars and clip view's widget
    AdjustChildWidgets(this, this, 0, 0, t2p);
  
    if (nsnull != thiswin)
    {
      thiswin->EndResizingChildren();
      NS_RELEASE(thiswin);
    }
  
    NS_RELEASE(dx);
  }
  return NS_OK;
}

NS_IMETHODIMP nsScrollingView :: Paint(nsIRenderingContext& rc, const nsRect& rect,
                                       PRUint32 aPaintFlags, PRBool &aResult)
{
  PRBool  clipres = PR_FALSE;
  nsRect  brect;

  rc.PushState();

  GetBounds(brect);

  //don't clip if we have a widget
  if ((mVis == nsViewVisibility_kShow) && (nsnull == mWindow))
    rc.SetClipRect(brect, nsClipCombine_kIntersect, clipres);

  if (clipres == PR_FALSE)
  {
    nsView::Paint(rc, rect, aPaintFlags | NS_VIEW_FLAG_CLIP_SET, clipres);
  }

  rc.PopState(clipres);

  if ((clipres == PR_FALSE) && (mVis == nsViewVisibility_kShow) && (nsnull == mWindow))
    rc.SetClipRect(brect, nsClipCombine_kSubtract, clipres);

  aResult = clipres;
  return NS_OK;
}

void nsScrollingView :: HandleScrollEvent(nsGUIEvent *aEvent, PRUint32 aEventFlags)
{
  nsIView           *scview = nsView::GetViewFor(aEvent->widget);
  nsIDeviceContext  *px;
  float             t2p, p2t;
  nscoord           dx = 0, dy = 0;  // in device units
  nsSize            clipSize;

  mViewManager->GetDeviceContext(px);
  px->GetAppUnitsToDevUnits(t2p);
  px->GetDevUnitsToAppUnits(p2t);
  NS_RELEASE(px);

  // Get the size of the clip view
  mClipView->GetDimensions(&clipSize.width, &clipSize.height);

  // Is it a vertical scroll event or a horizontal scroll event?
  if ((nsnull != mVScrollBarView) && (scview == mVScrollBarView))
  {
    nscoord oldOffsetY = mOffsetY;
    nscoord newPos;

    // The new scrollbar position is in app units
    newPos = ((nsScrollbarEvent *)aEvent)->position;

    // Don't allow a scroll below the bottom of the scrolled view
    if ((newPos + clipSize.height) > mSizeY)
      newPos = mSizeY - clipSize.height;

    // Snap the new scrollbar position to the nearest pixel. This ensures that
    // as we scroll the view a pixel at a time the scrollbar position
    // is at the same pixel as the top edge of the scrolled view
    mOffsetY = NSIntPixelsToTwips(NSTwipsToIntPixels(newPos, t2p), p2t);

    // Compute the delta in device units. We need device units when scrolling
    // the window
    dy = NSTwipsToIntPixels((oldOffsetY - mOffsetY), t2p);
    if (dy != 0)
    {
      // Update the scrollbar position passed in with the scrollbar event.
      // This value will be used to update the scrollbar thumb, and we want
      // to make sure the scrollbar thumb is in sync with the offset we came
      // up with here.
      ((nsScrollbarEvent *)aEvent)->position = mOffsetY;
    }
  }
  else if ((nsnull != mHScrollBarView) && (scview == mHScrollBarView))
  {
    nscoord oldOffsetX = mOffsetX;
    nscoord newPos;

    // The new scrollbar position is in app units
    newPos = ((nsScrollbarEvent *)aEvent)->position;

    // Don't allow a scroll beyond the width of the scrolled view
    if ((newPos + clipSize.width) > mSizeX)
      newPos = mSizeX - clipSize.width;

    // Snap the new scrollbar position to the nearest pixel. This ensures that
    // as we scroll the view a pixel at a time the scrollbar position
    // is at the same pixel as the left edge of the scrolled view
    mOffsetX = NSIntPixelsToTwips(NSTwipsToIntPixels(newPos, t2p), p2t);

    // Compute the delta in device units. We need device units when scrolling
    // the window
    dx = NSTwipsToIntPixels((oldOffsetX - mOffsetX), t2p);
    if (dx != 0)
    {
      // Update the scrollbar position passed in with the scrollbar event.
      // This value will be used to update the scrollbar thumb, and we want
      // to make sure the scrollbar thumb is in sync with the offset we came
      // up with here.
      ((nsScrollbarEvent *)aEvent)->position = mOffsetX;
    }
  }

  // Position the scrolled view
  nsIView *scrolledView;
  GetScrolledView(scrolledView);
  scrolledView->SetPosition(-mOffsetX, -mOffsetY);
  
  // If we actually scrolled by at least one pixel then scroll the contents
  // of the scrolled view
  if ((dx != 0) || (dy != 0))
  {
    nsIWidget *clipWidget;
    mClipView->GetWidget(clipWidget);
    if (nsnull == clipWidget)
    {
      // XXX Repainting is really slow. The widget's Scroll() member function
      // needs an argument that specifies whether child widgets are scrolled,
      // and we need to be able to specify the rect to be scrolled...
      mViewManager->UpdateView(mClipView, nsnull, 0);
    }
    else
    {
      // Scroll the contents of the widget by the specfied amount, and scroll
      // the child widgets
      clipWidget->Scroll(dx, dy, nsnull);
    }
  }
}

void nsScrollingView :: Notify(nsITimer * aTimer)
{
  nscoord xoff, yoff;
  nsIView *view;
  GetScrolledView(view);

  // First do the scrolling of the view
  xoff = mOffsetX;
  yoff = mOffsetY;

  nscoord newPos = yoff + mScrollingDelta;

  if (newPos < 0)
    newPos = 0;

  ScrollTo(0, newPos, 0);

  // Now fake a mouse event so the frames can process the selection event

  nsRect        rect;
  nsGUIEvent    event;
  nsEventStatus retval;

  event.message = NS_MOUSE_MOVE;

  GetBounds(rect);

  event.point.x = rect.x;
  event.point.y = (mScrollingDelta > 0) ? (rect.height - rect.y - 1) : 135;

  //printf("timer %d %d\n", event.point.x, event.point.y);

  nsIViewObserver *obs;

  if (NS_OK == mViewManager->GetViewObserver(obs))
  {
    obs->HandleEvent((nsIView *)this, &event, retval);
    NS_RELEASE(obs);
  }
  
  NS_RELEASE(mScrollingTimer);

  if (NS_OK == NS_NewTimer(&mScrollingTimer))
    mScrollingTimer->Init(this, 25);
}

NS_IMETHODIMP nsScrollingView :: HandleEvent(nsGUIEvent *aEvent, PRUint32 aEventFlags,
                                             nsEventStatus &aStatus)
{
  nsIWidget *win;

  switch (aEvent->message)
  {
    case NS_MOUSE_LEFT_BUTTON_DOWN:
    case NS_MOUSE_MIDDLE_BUTTON_DOWN:
    case NS_MOUSE_RIGHT_BUTTON_DOWN: 
      GetWidget(win);
      if (nsnull != win) 
      {
        win->SetFocus();
        NS_RELEASE(win);
      }
      break;

    case NS_KEY_DOWN:
    {
      nsKeyEvent * keyEvent = (nsKeyEvent *)aEvent;
      switch (keyEvent->keyCode)
      {
        case NS_VK_PAGE_DOWN: 
        case NS_VK_PAGE_UP: {
          nsIScrollbar  *scrollv = nsnull, *scrollh = nsnull;
          nsIWidget     *win;
          mVScrollBarView->GetWidget(win);

          if (NS_OK == win->QueryInterface(kIScrollbarIID, (void **)&scrollv))
          {
            PRUint32  oldPos = 0;
            scrollv->GetPosition(oldPos);
            nsSize    clipSize;
            mClipView->GetDimensions(&clipSize.width, &clipSize.height);
            nscoord   newPos = 0;
            if (keyEvent->keyCode == NS_VK_PAGE_DOWN) {
              newPos = oldPos + clipSize.height;
            } else {
              newPos = oldPos - clipSize.height;
              if (newPos < 0)
                newPos = 0;
            }
            ScrollTo(0, newPos, 0);
          }

        } break;

        case NS_VK_DOWN: 
        case NS_VK_UP: {
          nsIScrollbar  *scrollv = nsnull, *scrollh = nsnull;
          mVScrollBarView->GetWidget(win);

          if (NS_OK == win->QueryInterface(kIScrollbarIID, (void **)&scrollv))
          {
            PRUint32  oldPos  = 0;
            scrollv->GetPosition(oldPos);
            PRUint32  lineInc = 0;
            scrollv->GetLineIncrement(lineInc);
            nscoord   newPos = 0;
            if (keyEvent->keyCode == NS_VK_DOWN) {
              newPos = oldPos + lineInc;
            } else {
              newPos = oldPos - lineInc;
              if (newPos < 0)
                newPos = 0;
            }
            ScrollTo(0, newPos, 0);
          }

        } break;

        default:
          break;

      } // switch
    } break;

    case NS_MOUSE_MOVE:
    {
#if 0
      nsRect  brect;
      nscoord lx, ly;

      GetWidget(win);
		
      GetBounds(brect);
      
      // XXX Huh. We shouldn't just be doing this for any mouse move.
      // If this is for auto-scrolling then only on mouse press and drag...
      lx = aEvent->point.x - (brect.x);
      ly = aEvent->point.y - (brect.y);

      //nscoord         xoff, yoff;
      //GetScrolledView()->GetScrollOffset(&xoff, &yoff);
      //printf("%d %d   %d\n", trect.y, trect.height, yoff);
      //printf("mouse %d %d \n", aEvent->point.x, aEvent->point.y);

      if (!brect.Contains(lx, ly))
      {
        if (mScrollingTimer == nsnull)
        {
          if (nsnull != mClientData)
          {
            if (ly < 0 || ly > brect.y)
            {
              mScrollingDelta = ly < 0 ? -100 : 100;
              NS_NewTimer(&mScrollingTimer);
              mScrollingTimer->Init(this, 25);
            }
          }
        }
      }
      else if (mScrollingTimer != nsnull)
      {
        mScrollingTimer->Cancel();
        NS_RELEASE(mScrollingTimer);
      }
#endif
      break;
    }

    case NS_MOUSE_LEFT_BUTTON_UP:
    case NS_MOUSE_MIDDLE_BUTTON_UP:
    case NS_MOUSE_RIGHT_BUTTON_UP: 
    {
      if (mScrollingTimer != nsnull)
      {
        mScrollingTimer->Cancel();
        NS_RELEASE(mScrollingTimer);
        mScrollingTimer = nsnull;
      }

      nsRect  trect;
      nscoord lx, ly;

      GetBounds(trect);

      //GetWidget(win);
      //mViewManager->GetDeviceContext(dx);
      //dx->GetDevUnitsToAppUnits(p2t);
      //offX = offY = 0;
      //win->ConvertToDeviceCoordinates(offX,offY);
      //offX = NSIntPixelsToTwips(offX, p2t);
      //offY = NSIntPixelsToTwips(offY, p2t);

      lx = aEvent->point.x - (trect.x);
      ly = aEvent->point.y - (trect.y);

      if (!trect.Contains(lx, ly))
      {
        nsEventStatus retval;

        if (nsnull != mClientData)
        {
          nsIViewObserver *obs;

          if (NS_OK == mViewManager->GetViewObserver(obs))
          {
            obs->HandleEvent((nsIView *)this, aEvent, retval);
            NS_RELEASE(obs);
          }
        }
      }
      break;
    }

    default:
      break;
  }

  return nsView::HandleEvent(aEvent, aEventFlags, aStatus);
}

NS_IMETHODIMP nsScrollingView :: CreateScrollControls(nsNativeWidget aNative)
{
  nsIDeviceContext  *dx;
  mViewManager->GetDeviceContext(dx);
  nsresult rv = NS_ERROR_FAILURE;

  // Create a clip view
  mClipView = new nsView;

  if (nsnull != mClipView)
  {
    // The clip view needs a widget to clip any of the scrolled view's
    // child views with widgets. Note that the clip view has an opacity
    // of 0.0f (completely transparent)
    // XXX The clip widget should be created on demand only...
    rv = mClipView->Init(mViewManager, mBounds, this);
    mViewManager->InsertChild(this, mClipView, -1);
    mViewManager->SetViewOpacity(mClipView, 0.0f);
    rv = mClipView->CreateWidget(kWidgetCID, nsnull,
                                 mWindow ? nsnull : aNative);
  }

  // Create a view for a corner cover
  mCornerView = new CornerView;

  if (nsnull != mCornerView)
  {
    nsRect trect;
    float  sbWidth, sbHeight;

    dx->GetScrollBarDimensions(sbWidth, sbHeight);
    trect.width = NSToCoordRound(sbWidth);
    trect.x = mBounds.x + mBounds.XMost() - trect.width;
    trect.height = NSToCoordRound(sbHeight);
    trect.y = mBounds.y + mBounds.YMost() - trect.height;

    rv = mCornerView->Init(mViewManager, trect, this,
                           nsnull, nsViewVisibility_kHide);
    mViewManager->InsertChild(this, mCornerView, -1);
  }

  // Create a view for a vertical scrollbar
  mVScrollBarView = new ScrollBarView(this);

  if (nsnull != mVScrollBarView)
  {
    nsRect  trect = mBounds;
    float   sbWidth, sbHeight;

    dx->GetScrollBarDimensions(sbWidth, sbHeight);
    trect.width = NSToCoordRound(sbWidth);
    trect.x += mBounds.XMost() - trect.width;
    trect.height -= NSToCoordRound(sbHeight);

    static NS_DEFINE_IID(kCScrollbarIID, NS_VERTSCROLLBAR_CID);

    rv = mVScrollBarView->Init(mViewManager, trect, this);
    mViewManager->InsertChild(this, mVScrollBarView, -3);
    rv = mVScrollBarView->CreateWidget(kCScrollbarIID, nsnull,
                                       mWindow ? nsnull : aNative);
  }

  // Create a view for a horizontal scrollbar
  mHScrollBarView = new ScrollBarView(this);

  if (nsnull != mHScrollBarView)
  {
    nsRect  trect = mBounds;
    float   sbWidth, sbHeight;

    dx->GetScrollBarDimensions(sbWidth, sbHeight);
    trect.height = NSToCoordRound(sbHeight);
    trect.y += mBounds.YMost() - trect.height;
    trect.width -= NSToCoordRound(sbWidth);

    static NS_DEFINE_IID(kCHScrollbarIID, NS_HORZSCROLLBAR_CID);

    rv = mHScrollBarView->Init(mViewManager, trect, this);
    mViewManager->InsertChild(this, mHScrollBarView, -3);
    rv = mHScrollBarView->CreateWidget(kCHScrollbarIID, nsnull,
                                       mWindow ? nsnull : aNative);
  }

  NS_RELEASE(dx);

  return rv;
}

NS_IMETHODIMP nsScrollingView :: SetWidget(nsIWidget *aWidget)
{
  NS_ASSERTION(PR_FALSE, "please don't try and set a widget here");
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP nsScrollingView :: ComputeContainerSize()
{
  nsIView       *scrolledView;
  GetScrolledView(scrolledView);
  nsIScrollbar  *scrollv = nsnull, *scrollh = nsnull;
  nsIWidget     *win;

  if (nsnull != scrolledView)
  {
    nscoord           dx = 0, dy = 0;
    nsIDeviceContext  *px;
    nscoord           hwidth, hheight;
    nscoord           vwidth, vheight;
    PRUint32          oldsizey = mSizeY, oldsizex = mSizeX;
    nsRect            area(0, 0, 0, 0);
    nscoord           offx, offy;
    float             scale;
    nsRect            controlRect(0, 0, mBounds.width, mBounds.height);
    controlRect.Deflate(mInsets);

    mViewManager->GetDeviceContext(px);
    px->GetAppUnitsToDevUnits(scale);

    scrolledView->GetDimensions(&mSizeX, &mSizeY);

    if (nsnull != mHScrollBarView)
    {
      mHScrollBarView->GetDimensions(&hwidth, &hheight);
      mHScrollBarView->GetWidget(win);

      if (NS_OK == win->QueryInterface(kIScrollbarIID, (void **)&scrollh))
      {
        if (((mSizeX > controlRect.width) &&
            (mScrollPref != nsScrollPreference_kNeverScroll)) ||
            (mScrollPref == nsScrollPreference_kAlwaysScroll))
        {
          scrollh->Release(); //DO NOT USE NS_RELEASE()! MMP
        }
        else
        {
          NS_RELEASE(scrollh); //MUST USE NS_RELEASE()! MMP
        }
      }

      NS_RELEASE(win);
    }

    if (nsnull != mVScrollBarView)
    {
      mVScrollBarView->GetDimensions(&vwidth, &vheight);
      offy = mOffsetY;

      mVScrollBarView->GetWidget(win);

      if (NS_OK == win->QueryInterface(kIScrollbarIID, (void **)&scrollv))
      {
        // XXX Shouldn't this check also take into account the height of the
        // horizontal scrollbar if we need one?
        if ((mSizeY > controlRect.height) &&
            (mScrollPref != nsScrollPreference_kNeverScroll))
        {
          //we need to be able to scroll

          mVScrollBarView->SetVisibility(nsViewVisibility_kShow);
          win->Enable(PR_TRUE);

          //now update the scroller position for the new size

          PRUint32  oldpos = 0;
          scrollv->GetPosition(oldpos);
          float     p2t;

          px->GetDevUnitsToAppUnits(p2t);

          // XXX Check for 0 initial size. This is really indicative
          // of a problem. 
          if (0 == oldsizey) {
            mOffsetY = 0;
          }
          else {
            mOffsetY = NSIntPixelsToTwips(NSTwipsToIntPixels(nscoord(((float)oldpos * mSizeY) / oldsizey), scale), p2t);
          }

          dy = NSTwipsToIntPixels((offy - mOffsetY), scale);

          scrollv->SetParameters(mSizeY, controlRect.height - ((nsnull != scrollh) ? hheight : 0),
                                 mOffsetY, NSIntPointsToTwips(12));
        }
        else
        {
          // The scrolled view is entirely visible vertically. Either hide the
          // vertical scrollbar or disable it
          mOffsetY = 0;
          dy = NSTwipsToIntPixels(offy, scale);

          if (mScrollPref == nsScrollPreference_kAlwaysScroll)
          {
            mVScrollBarView->SetVisibility(nsViewVisibility_kShow);
            win->Enable(PR_FALSE);
          }
          else
          {
            mVScrollBarView->SetVisibility(nsViewVisibility_kHide);
            win->Enable(PR_TRUE);
            NS_RELEASE(scrollv);
          }
        }

        //don't release the vertical scroller here because if we need to
        //create a horizontal one, it will need to know that there is a vertical one
//        //create a horizontal one, it will need to tweak the vertical one
      }

      NS_RELEASE(win);
    }

    if (nsnull != mHScrollBarView)
    {
      offx = mOffsetX;

      mHScrollBarView->GetWidget(win);

      if (NS_OK == win->QueryInterface(kIScrollbarIID, (void **)&scrollh))
      {
        // XXX Shouldn't this check also take into account the width of the
        // vertical scrollbar if we have one?
        if ((mSizeX > controlRect.width) &&
            (mScrollPref != nsScrollPreference_kNeverScroll))
        {
          //we need to be able to scroll

          mHScrollBarView->SetVisibility(nsViewVisibility_kShow);
          win->Enable(PR_TRUE);

          //now update the scroller position for the new size

          PRUint32  oldpos = 0;
          scrollh->GetPosition(oldpos);
          float     p2t;

          px->GetDevUnitsToAppUnits(p2t);

          // XXX Check for 0 initial size. This is really indicative
          // of a problem. 
          if (0 == oldsizex) {
            mOffsetX = 0;
          }
          else {
            mOffsetX = NSIntPixelsToTwips(NSTwipsToIntPixels(nscoord(((float)oldpos * mSizeX) / oldsizex), scale), p2t);
          }

          dx = NSTwipsToIntPixels((offx - mOffsetX), scale);

          scrollh->SetParameters(mSizeX, controlRect.width - ((nsnull != scrollv) ? vwidth : 0),
                                 mOffsetX, NSIntPointsToTwips(12));

//          //now make the vertical scroll region account for this scrollbar
//
//          if (nsnull != scrollv)
//            scrollv->SetParameters(mSizeY, mBounds.height - hheight, mOffsetY, NSIntPointsToTwips(12));
        }
        else
        {
          // The scrolled view is entirely visible horizontally. Either hide the
          // horizontal scrollbar or disable it
          mOffsetX = 0;
          dx = NSTwipsToIntPixels(offx, scale);

          if (mScrollPref == nsScrollPreference_kAlwaysScroll)
          {
            mHScrollBarView->SetVisibility(nsViewVisibility_kShow);
            win->Enable(PR_FALSE);
          }
          else
          {
            mHScrollBarView->SetVisibility(nsViewVisibility_kHide);
            win->Enable(PR_TRUE);
          }
        }

        NS_RELEASE(scrollh);
      }

      NS_RELEASE(win);
    }

    // Adjust the size of the clip view to account for scrollbars that are
    // showing
    if (mHScrollBarView && ViewIsShowing(mHScrollBarView))
    {
      controlRect.height -= hheight;
    }
    if (mVScrollBarView && ViewIsShowing(mVScrollBarView))
    {
      controlRect.width -= vwidth;
    }

    mClipView->SetDimensions(controlRect.width, controlRect.height, PR_FALSE);

    // Position the scrolled view
    scrolledView->SetPosition(-mOffsetX, -mOffsetY);

    if (mCornerView)
    {
      if (mHScrollBarView && ViewIsShowing(mHScrollBarView) &&
          mVScrollBarView && ViewIsShowing(mVScrollBarView))
        ((CornerView *)mCornerView)->Show(PR_TRUE);
      else
        ((CornerView *)mCornerView)->Show(PR_FALSE);
    }

    // now we can release the vertical scroller if there was one...

    NS_IF_RELEASE(scrollv);

//    if ((dx != 0) || (dy != 0))
//      AdjustChildWidgets(this, this, 0, 0, px->GetTwipsToPixels());

    NS_RELEASE(px);
  }
  else
  {
    // There's no scrolled view so hide the scrollbars and corner view
    if (nsnull != mHScrollBarView)
    {
      mHScrollBarView->SetVisibility(nsViewVisibility_kHide);

      mHScrollBarView->GetWidget(win);
      if (NS_OK == win->QueryInterface(kIScrollbarIID, (void **)&scrollh))
      {
        scrollh->SetParameters(0, 0, 0, 0);
        NS_RELEASE(scrollh);
      }
      NS_RELEASE(win);
    }

    if (nsnull != mVScrollBarView)
    {
      mVScrollBarView->SetVisibility(nsViewVisibility_kHide);

      mVScrollBarView->GetWidget(win);
      if (NS_OK == win->QueryInterface(kIScrollbarIID, (void **)&scrollv))
      {
        scrollv->SetParameters(0, 0, 0, 0);
        NS_RELEASE(scrollv);
      }
      NS_RELEASE(win);
    }

    if (nsnull != mCornerView)
      ((CornerView *)mCornerView)->Show(PR_FALSE);

    mOffsetX = mOffsetY = 0;
    mSizeX = mSizeY = 0;
  }

  UpdateScrollControls(PR_TRUE);

  return NS_OK;
}

NS_IMETHODIMP nsScrollingView :: GetContainerSize(nscoord *aWidth, nscoord *aHeight) const
{
  *aWidth = mSizeX;
  *aHeight = mSizeY;
  return NS_OK;
}

NS_IMETHODIMP nsScrollingView :: ShowQuality(PRBool aShow)
{
  ((CornerView *)mCornerView)->ShowQuality(aShow);
  return NS_OK;
}

NS_IMETHODIMP nsScrollingView :: GetShowQuality(PRBool &aShow) const
{
  aShow = ((CornerView *)mCornerView)->mShowQuality;
  return NS_OK;
}

NS_IMETHODIMP nsScrollingView :: SetQuality(nsContentQuality aQuality)
{
  ((CornerView *)mCornerView)->SetQuality(aQuality);
  return NS_OK;
}

NS_IMETHODIMP nsScrollingView :: SetScrollPreference(nsScrollPreference aPref)
{
  mScrollPref = aPref;
  ComputeContainerSize();
  return NS_OK;
}

NS_IMETHODIMP nsScrollingView :: GetScrollPreference(nsScrollPreference &aScrollPreference) const
{
  aScrollPreference = mScrollPref;
  return NS_OK;
}

// XXX This doesn't do X scrolling yet

// XXX This doesn't let the scrolling code slide the bits on the
// screen and damage only the appropriate area

// XXX doesn't smooth scroll

NS_IMETHODIMP
nsScrollingView :: ScrollTo(nscoord aX, nscoord aY, PRUint32 aUpdateFlags)
{
  nsIDeviceContext  *dx;
  float             t2p;
  float             p2t;

  mViewManager->GetDeviceContext(dx);
  dx->GetAppUnitsToDevUnits(t2p);
  dx->GetDevUnitsToAppUnits(p2t);

  NS_RELEASE(dx);

  nsIWidget*      win;
  mVScrollBarView->GetWidget(win);
  if (nsnull != win)
  {
    nsIScrollbar* scrollv;
    if (NS_OK == win->QueryInterface(kIScrollbarIID, (void **)&scrollv))
    {
      // Clamp aY
      nsSize  clipSize;
      mClipView->GetDimensions(&clipSize.width, &clipSize.height);
      if (aY + clipSize.height > mSizeY) {
        aY = mSizeY - clipSize.height;
        if (aY < 0) {
          aY = 0;
        }
      }

      // Move the scrollbar's thumb

      PRUint32  oldpos = mOffsetY;
      nscoord dy;

      PRUint32 newpos =
        NSIntPixelsToTwips(NSTwipsToIntPixels(aY, t2p), p2t);
      scrollv->SetPosition(newpos);

      dy = oldpos - newpos;

      // Update the scrolled view's position
      nsIView* scrolledView;
      GetScrolledView(scrolledView);
      if (nsnull != scrolledView)
      {
        scrolledView->SetPosition(-aX, -aY);
        mOffsetX = aX;
        mOffsetY = aY;
      }

      AdjustChildWidgets(this, scrolledView, 0, 0, t2p);

      // Damage the updated area
      nsRect  r;

      r.x = 0;
      r.y = aY;
      mClipView->GetDimensions(&r.width, &r.height);
      if (nsnull != scrolledView)
      {
        mViewManager->UpdateView(scrolledView, r, aUpdateFlags);
      }

      NS_RELEASE(scrollv);
    }
    NS_RELEASE(win);
  }
  return NS_OK;
}

NS_IMETHODIMP nsScrollingView :: SetControlInsets(const nsMargin &aInsets)
{
  mInsets = aInsets;
  return NS_OK;
}

NS_IMETHODIMP nsScrollingView :: GetControlInsets(nsMargin &aInsets) const
{
  aInsets = mInsets;
  return NS_OK;
}

void nsScrollingView :: AdjustChildWidgets(nsScrollingView *aScrolling, nsIView *aView, nscoord aDx, nscoord aDy, float scale)
{
  PRInt32           numkids;
  aView->GetChildCount(numkids);
  nscoord           offx, offy;
  PRBool            isscroll = PR_FALSE;

  if (aScrolling == aView)
  {
    nsIWidget *widget;
    aScrolling->GetOffsetFromWidget(&aDx, &aDy, widget);
    NS_IF_RELEASE(widget);
  }

  aView->GetPosition(&offx, &offy);

  aDx += offx;
  aDy += offy;

  for (PRInt32 cnt = 0; cnt < numkids; cnt++)
  {
    nsIView   *kid;
    aView->GetChild(cnt, kid);
    nsIWidget *win;
    kid->GetWidget(win);

    if (nsnull != win)
    {
      nsRect  bounds;

#if 0
      win->BeginResizingChildren();
#endif
      kid->GetBounds(bounds);

      if (!isscroll ||
          (isscroll &&
          (kid != ((nsScrollingView *)aView)->mVScrollBarView) &&
          (kid != ((nsScrollingView *)aView)->mHScrollBarView)))
        win->Move(NSTwipsToIntPixels((bounds.x + aDx), scale), NSTwipsToIntPixels((bounds.y + aDy), scale));
      else
        win->Move(NSTwipsToIntPixels((bounds.x + aDx + offx), scale), NSTwipsToIntPixels((bounds.y + aDy + offy), scale));
    }

    // Don't recurse if the view has a widget, because we adjusted the view's
    // widget position, and its child widgets are relative to its positon
    if (nsnull == win)
    {
      AdjustChildWidgets(aScrolling, kid, aDx, aDy, scale);
    }

    if (nsnull != win)
    {
#if 0
      win->EndResizingChildren();
#endif
      NS_RELEASE(win);
    }
  }
}

void nsScrollingView :: UpdateScrollControls(PRBool aPaint)
{
  nsRect  clipRect;

  if (nsnull != mClipView)
  {
    mClipView->GetBounds(clipRect);

    // Position the corner view
    if (nsnull != mCornerView)
    {
      nsSize  cornerSize;

      mCornerView->GetDimensions(&cornerSize.width, &cornerSize.height);
      mCornerView->SetBounds(clipRect.XMost(), clipRect.YMost(), cornerSize.width,
                             cornerSize.height, aPaint);
    }

    // Size and position the vertical scrollbar
    if (nsnull != mVScrollBarView)
    {
      nsSize  sbSize;

      mVScrollBarView->GetDimensions(&sbSize.width, &sbSize.height);
      mVScrollBarView->SetBounds(clipRect.XMost(), clipRect.y, sbSize.width, 
                                 clipRect.height, aPaint);
    }

    // Size and position the horizontal scrollbar
    if (nsnull != mHScrollBarView)
    {
      nsSize  sbSize;
    
      mHScrollBarView->GetDimensions(&sbSize.width, &sbSize.height);
      mHScrollBarView->SetBounds(clipRect.x, clipRect.YMost(), clipRect.width,
                                 sbSize.height, aPaint);
    }
  }
}

NS_IMETHODIMP nsScrollingView :: SetScrolledView(nsIView *aScrolledView)
{
  return mViewManager->InsertChild(mClipView, aScrolledView, 0);
}

NS_IMETHODIMP nsScrollingView :: GetScrolledView(nsIView *&aScrolledView) const
{
  if (nsnull != mClipView)
    return mClipView->GetChild(0, aScrolledView);
  else
  {
    aScrolledView = nsnull;
    return NS_OK;
  }
}

NS_IMETHODIMP nsScrollingView :: GetScrollPosition(nscoord &aX, nscoord &aY) const
{
  aX = mOffsetX;
  aY = mOffsetY;
  return NS_OK;
}

