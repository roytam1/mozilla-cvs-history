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

#include "nsScrollBar.h"
#include "nsGfxCIID.h"

NS_IMPL_ADDREF(nsScrollbar)
NS_IMPL_RELEASE(nsScrollbar)

nsScrollbar::nsScrollbar(PRBool aIsVertical) : nsWidget(), nsIScrollbar()
{
  NS_INIT_REFCNT();
  mMaxRange = 0;
  mPosition = 0;
  mThumbSize = 0;
  mLineIncrement = 1;
  mIsVertical = aIsVertical;
  mBackground = NS_RGB(100,100,100);
  bg_pixel = xlib_rgb_xpixel_from_rgb(mBackground);
  border_pixel = xlib_rgb_xpixel_from_rgb(mBackground);
  mBar = 0;
  mBarBounds.x = mBarBounds.y = mBarBounds.width = mBarBounds.height = 0;
};

nsScrollbar::~nsScrollbar()
{
  if (mBar) {
    XDestroyWindow(gDisplay, mBar);
    DeleteWindowCallback(mBar);
  }
}

nsresult nsScrollbar::QueryInterface(const nsIID& aIID, void** aInstancePtr)
{
  nsresult result = nsWidget::QueryInterface(aIID, aInstancePtr);
  
  static NS_DEFINE_IID(kInsScrollbarIID, NS_ISCROLLBAR_IID);
  if (result == NS_NOINTERFACE && aIID.Equals(kInsScrollbarIID)) {
    *aInstancePtr = (void*) ((nsIScrollbar*)this);
    NS_ADDREF_THIS();
    result = NS_OK;
  }
  
  return result;
}

NS_METHOD nsScrollbar::SetMaxRange(PRUint32 aEndRange)
{
  printf("nsScrollbar::SetMaxRange()\n");
  printf("Max Range set to %d\n", aEndRange);
  mMaxRange = aEndRange;
  CalcBarBounds();
  LayoutBar();
  return NS_OK;
}

PRUint32 nsScrollbar::GetMaxRange(PRUint32& aRange)
{
  printf("nsScrollbar::GetMaxRange()\n");
  aRange = mMaxRange;
  return NS_OK;
}

NS_METHOD nsScrollbar::SetPosition(PRUint32 aPos)
{
  printf("nsScrollbar::SetPosition()\n");
  printf("Scroll to %d\n", aPos);
  mPosition = aPos;
  CalcBarBounds();
  LayoutBar();
  return NS_OK;
}

PRUint32 nsScrollbar::GetPosition(PRUint32& aPosition)
{
  printf("nsScrollbar::GetPosition()\n");
  aPosition = mPosition;
  return NS_OK;
}

NS_METHOD nsScrollbar::SetThumbSize(PRUint32 aSize)
{
  printf("nsScrollbar::SetThumbSize()\n");
  printf("Thumb size set to %d\n", aSize);
  mThumbSize = aSize;
  CalcBarBounds();
  LayoutBar();
  return NS_OK;
}

NS_METHOD nsScrollbar::GetThumbSize(PRUint32& aSize)
{
  printf("nsScrollbar::GetThumbSize()\n");
  aSize = mThumbSize;
  return NS_OK;
}

NS_METHOD nsScrollbar::SetLineIncrement(PRUint32 aSize)
{
  printf("nsScrollbar::SetLineIncrement()\n");
  printf("Set Line Increment to %d\n", aSize);
  mLineIncrement = aSize;
  CalcBarBounds();
  LayoutBar();
  return NS_OK;
}

NS_METHOD nsScrollbar::GetLineIncrement(PRUint32& aSize)
{
  printf("nsScrollbar::GetLineIncrement()\n");
  aSize = mLineIncrement;
  return NS_OK;
}

NS_METHOD nsScrollbar::SetParameters(PRUint32 aMaxRange, PRUint32 aThumbSize,
                                PRUint32 aPosition, PRUint32 aLineIncrement)
{
  printf("nsScrollbar::SetParameters()\n");
  printf("MaxRange = %d ThumbSize = %d aPosition = %d LineIncrement = %d\n",
         aMaxRange, aThumbSize, aPosition, aLineIncrement);
  SetMaxRange(aMaxRange);
  SetThumbSize(aThumbSize);
  SetPosition(aPosition);
  SetLineIncrement(aLineIncrement);
  CalcBarBounds();
  LayoutBar();
  return NS_OK;
}

PRBool nsScrollbar::OnScroll(PRUint32 scrollCode, int cPos)
{
  printf("nsScrollbar::OnScroll\n");
  PRBool result = PR_FALSE;
  switch (scrollCode) {
  case NS_SCROLLBAR_PAGE_NEXT:
    result = NextPage();
    break;
  case NS_SCROLLBAR_PAGE_PREV:
    result = PrevPage();
    break;
  default:
    break;
  }
  return result;
}

PRBool nsScrollbar::OnResize(nsSizeEvent &event)
{
  PRBool result;
  printf("nsScrollbar::OnResize\n");
  nsWidget::OnResize(event);
  CalcBarBounds();
  LayoutBar();
  result = PR_FALSE;
  return result;
}

PRBool nsScrollbar::DispatchMouseEvent(nsMouseEvent &aEvent)
{
  PRBool result;
  printf("nsScrollbar::DispatchMouseEvent\n");
  // check to see if this was on the main window.
  switch (aEvent.message) {
  case NS_MOUSE_LEFT_BUTTON_DOWN:
    if (mIsVertical == PR_TRUE) {
      if (aEvent.point.y < mBarBounds.y) {
        OnScroll(NS_SCROLLBAR_PAGE_PREV, 0);
      }
      else if (aEvent.point.y > mBarBounds.height) {
        OnScroll(NS_SCROLLBAR_PAGE_NEXT, 0);
      }
    }
    else {
      if (aEvent.point.x < mBarBounds.x) {
        OnScroll(NS_SCROLLBAR_PAGE_PREV, 0);
      }
      else if (aEvent.point.x > mBarBounds.width) {
        OnScroll(NS_SCROLLBAR_PAGE_NEXT, 0);
      }
    }
    break;
  default:
    break;
  }
  result = PR_FALSE;
  return result;
}


void nsScrollbar::CreateNative(Window aParent, nsRect aRect)
{
  XSetWindowAttributes attr;
  unsigned long attr_mask;
  
  // on a window resize, we don't want to window contents to
  // be discarded...
  attr.bit_gravity = SouthEastGravity;
  // make sure that we listen for events
  attr.event_mask = StructureNotifyMask | ButtonPressMask | ButtonReleaseMask;
  // set the default background color and border to that awful gray
  attr.background_pixel = bg_pixel;
  attr.border_pixel = border_pixel;
  // set the colormap
  attr.colormap = xlib_rgb_get_cmap();
  // here's what's in the struct
  attr_mask = CWBitGravity | CWEventMask | CWBackPixel | CWBorderPixel;
  // check to see if there was actually a colormap.
  if (attr.colormap)
    attr_mask |= CWColormap;

  CreateNativeWindow(aParent, mBounds, attr, attr_mask);
  CreateGC();
  // set up the scrolling bar.
  attr.event_mask = Button1MotionMask | ButtonPressMask | ButtonReleaseMask;
  attr.background_pixel = xlib_rgb_xpixel_from_rgb(NS_RGB(192,192,192));
  attr.border_pixel = xlib_rgb_xpixel_from_rgb(NS_RGB(100,100,100));
  // set up the size
  CalcBarBounds();
  mBar = XCreateWindow(gDisplay,
                       mBaseWindow,
                       mBarBounds.x, mBarBounds.y,
                       mBarBounds.width, mBarBounds.height,
                       2,  // border width
                       gDepth,
                       InputOutput,
                       gVisual,
                       attr_mask,
                       &attr);
  AddWindowCallback(mBar, this);
}

NS_IMETHODIMP nsScrollbar::Show(PRBool bState)
{
  nsWidget::Show(bState);
  if (mBar) {
    XMapWindow(gDisplay, mBar);
  }
  CalcBarBounds();
  LayoutBar();
  return NS_OK;
}

NS_IMETHODIMP nsScrollbar::Resize(PRUint32 aWidth,
                                  PRUint32 aHeight,
                                  PRBool   aRepaint)
{
  nsWidget::Resize(aWidth, aHeight, aRepaint);
  CalcBarBounds();
  LayoutBar();
  return NS_OK;
}

NS_IMETHODIMP nsScrollbar::Resize(PRUint32 aX,
                                  PRUint32 aY,
                                  PRUint32 aWidth,
                                  PRUint32 aHeight,
                                  PRBool   aRepaint)
{
  nsWidget::Resize(aX, aY, aWidth, aHeight, aRepaint);
  CalcBarBounds();
  LayoutBar();
  return NS_OK;
}

nsresult nsScrollbar::NextPage(void)
{
  PRUint32 max;
  nsresult result = PR_FALSE;

  // change it locally.
  max = mMaxRange - mThumbSize;
  mPosition += mThumbSize;
  if (mPosition > max)
    mPosition = max;
  
  // send the event
  if (mEventCallback) {
    nsScrollbarEvent sevent;
    sevent.message = NS_SCROLLBAR_POS;
    sevent.widget = (nsWidget *)this;
    sevent.eventStructType = NS_SCROLLBAR_EVENT;
    sevent.position = (mPosition);
    // send the event
    result = ConvertStatus((*mEventCallback) (&sevent));
    // the gtk code indicates that the callback can
    // modify the position.  how odd.
    mPosition = sevent.position;
  }
  CalcBarBounds();
  LayoutBar();
  return result;
}

nsresult nsScrollbar::PrevPage(void)
{
  nsresult result = PR_FALSE;
  // check to make sure we don't go backwards
  if (mThumbSize > mPosition) {
    mPosition = 0;
  }
  else {
    mPosition -= mThumbSize;
  }
  
  // send the event
  if (mEventCallback) {
    nsScrollbarEvent sevent;
    sevent.message = NS_SCROLLBAR_POS;
    sevent.widget = (nsWidget *)this;
    sevent.eventStructType = NS_SCROLLBAR_EVENT;
    sevent.position = (mPosition);
    // send the event
    result = ConvertStatus((*mEventCallback) (&sevent));
    // the gtk code indicates that the callback can
    // modify the position.  how odd.
    mPosition = sevent.position;
  }
  CalcBarBounds();
  LayoutBar();
  return result;
}

void nsScrollbar::CalcBarBounds(void)
{
  float bar_start;
  float bar_end;

  if (mMaxRange == 0) {
    bar_start = 0;
    bar_end = 0;
    printf("CalcBarBounds: max range is zero.\n");
  }
  else {
    printf("CalcBarBounds: position: %d max: %d thumb: %d\n",
           mPosition, mMaxRange, mThumbSize);
    bar_start = (float)mPosition / (float)mMaxRange;
    bar_end = ((float)mThumbSize + (float)mPosition ) / (float)mMaxRange;
    printf("CalcBarBounds: start: %f end: %f\n", bar_start, bar_end);
  }
  
  if (mIsVertical == PR_TRUE) {
    mBarBounds.x = 0;
    mBarBounds.y = (int)(bar_start * mBounds.height);
    mBarBounds.width = mBounds.width;
    mBarBounds.height = (int)(bar_end * mBounds.height);
  }
  else {
    mBarBounds.x = (int)(bar_start * mBounds.width);
    mBarBounds.y = 0;
    mBarBounds.width = (int)(bar_end * mBounds.width);
    mBarBounds.height = mBounds.height;
  }
  if (mBarBounds.height == 0) {
    mBarBounds.height = 1;
  }
  if (mBarBounds.width == 0) {
    mBarBounds.width = 1;
  }
  printf("CalcBarBounds: bar is (%s) %d %d %d %d\n", ((mIsVertical == PR_TRUE) ? "vertical" : "horizontal" ), mBarBounds.x, mBarBounds.y, mBarBounds.width, mBarBounds.height);
}

void nsScrollbar::LayoutBar(void)
{
  XMoveResizeWindow(gDisplay, mBar,
                    mBarBounds.x, mBarBounds.y,
                    mBarBounds.width, mBarBounds.height);
}
