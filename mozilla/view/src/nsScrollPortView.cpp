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

#include "nsScrollPortView.h"
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
#include "nsILookAndFeel.h"
#include "nsIClipView.h"
#include "nsISupportsArray.h"
#include "nsIScrollPositionListener.h"

static NS_DEFINE_IID(kWidgetCID, NS_CHILD_CID);
static NS_DEFINE_IID(kLookAndFeelCID, NS_LOOKANDFEEL_CID);
static NS_DEFINE_IID(kIClipViewIID, NS_ICLIPVIEW_IID);

// the percentage of the page that is scrolled on a page up or down
#define PAGE_SCROLL_PERCENT 0.8

nsScrollPortView::nsScrollPortView()
{
  mOffsetX = mOffsetY = 0;
  mOffsetXpx = mOffsetYpx = 0;

  mListeners = nsnull;
}

nsScrollPortView::~nsScrollPortView()
{    
  if (nsnull != mListeners) {
    mListeners->Clear();
    NS_RELEASE(mListeners);
  }

}

nsresult nsScrollPortView::QueryInterface(const nsIID& aIID, void** aInstancePtr)
{
  if (nsnull == aInstancePtr) {
    return NS_ERROR_NULL_POINTER;
  }
  *aInstancePtr = nsnull;
  if (aIID.Equals(NS_GET_IID(nsIScrollableView))) {
    *aInstancePtr = (void*)(nsIScrollableView*)this;
    return NS_OK;
  }

  return nsView::QueryInterface(aIID, aInstancePtr);
}

nsrefcnt nsScrollPortView::AddRef()
{
  NS_WARNING("not supported for views");
  return 1;
}

nsrefcnt nsScrollPortView::Release()
{
  NS_WARNING("not supported for views");
  return 1;
}


NS_IMETHODIMP nsScrollPortView::Init(nsIViewManager* aManager,
                                      const nsRect &aBounds,
                                      const nsIView *aParent,
                                      const nsViewClip *aClip,
                                      nsViewVisibility aVisibilityFlag)
{
  return nsView::Init(aManager, aBounds, aParent, aClip, aVisibilityFlag);
}

NS_IMETHODIMP nsScrollPortView::SetDimensions(nscoord width, nscoord height, PRBool aPaint)
{
  return nsView::SetDimensions(width, height, aPaint);
}

NS_IMETHODIMP nsScrollPortView::SetPosition(nscoord aX, nscoord aY)
{

  return nsView::SetPosition(aX, aY);
}


NS_IMETHODIMP nsScrollPortView::SetVisibility(nsViewVisibility aVisibility)
{
  return nsView::SetVisibility(aVisibility);
}
  
NS_IMETHODIMP nsScrollPortView::GetClipView(const nsIView** aClipView) const
{
  *aClipView = this; 
  return NS_OK;
}

NS_IMETHODIMP nsScrollPortView::AddScrollPositionListener(nsIScrollPositionListener* aListener)
{
	if (nsnull == mListeners) {
		nsresult rv = NS_NewISupportsArray(&mListeners);
		if (NS_FAILED(rv))
			return rv;
	}
	return mListeners->AppendElement(aListener);
}

NS_IMETHODIMP nsScrollPortView::RemoveScrollPositionListener(nsIScrollPositionListener* aListener)
{
	if (nsnull != mListeners) {
		return mListeners->RemoveElement(aListener);
	}
	return NS_ERROR_FAILURE;
}

NS_IMETHODIMP nsScrollPortView::CreateScrollControls(nsNativeWidget aNative)
{
  nsWidgetInitData  initData;
  initData.clipChildren = PR_TRUE;
  initData.clipSiblings = PR_TRUE;

  CreateWidget(kWidgetCID, &initData,
               mWindow ? nsnull : aNative);
  
  return NS_OK;
}

NS_IMETHODIMP nsScrollPortView::SetWidget(nsIWidget *aWidget)
{
  if (nsnull != aWidget) {
    NS_ASSERTION(PR_FALSE, "please don't try and set a widget here");
    return NS_ERROR_FAILURE;
  }
  return NS_OK;
}

NS_IMETHODIMP nsScrollPortView::ComputeScrollOffsets(PRBool aAdjustWidgets)
{
  return NS_OK;
}

NS_IMETHODIMP nsScrollPortView::GetContainerSize(nscoord *aWidth, nscoord *aHeight) const
{
  if (!aWidth || !aHeight)
    return NS_ERROR_NULL_POINTER;

  *aWidth  = 0;
  *aHeight = 0;

  nsIView *scrolledView = 0;

  nsresult result = GetScrolledView(scrolledView);

  if (NS_FAILED(result))
    return result;
  
  if (!scrolledView)
    return NS_ERROR_FAILURE;

  return scrolledView->GetDimensions(aWidth, aHeight);
}

NS_IMETHODIMP nsScrollPortView::ShowQuality(PRBool aShow)
{
  return NS_OK;
}

NS_IMETHODIMP nsScrollPortView::GetShowQuality(PRBool &aShow) const
{
  return NS_OK;
}

NS_IMETHODIMP nsScrollPortView::SetQuality(nsContentQuality aQuality)
{
  return NS_OK;
}

NS_IMETHODIMP nsScrollPortView::SetScrollPreference(nsScrollPreference aPref)
{
  return NS_OK;
}

NS_IMETHODIMP nsScrollPortView::GetScrollPreference(nsScrollPreference &aScrollPreference) const
{
  return nsScrollPreference_kNeverScroll;
}

NS_IMETHODIMP nsScrollPortView::ScrollTo(nscoord aX, nscoord aY, PRUint32 aUpdateFlags)
{

    // do nothing if the we aren't scrolling.
    if (aX == mOffsetX && aY == mOffsetY)
        return NS_OK;

	nsSize            scrolledSize;
    

	PRInt32           dxPx = 0, dyPx = 0;
    nsIView           *scrolledView;

    // convert to pixels
    nsIDeviceContext  *dev;
	float             t2p;
	float             p2t;
 
	// notify the listeners.
	PRUint32 listenerCount;
        const nsIID& kScrollPositionListenerIID = NS_GET_IID(nsIScrollPositionListener);
	nsIScrollPositionListener* listener;
	if (nsnull != mListeners) {
		if (NS_SUCCEEDED(mListeners->Count(&listenerCount))) {
			for (PRUint32 i = 0; i < listenerCount; i++) {
				if (NS_SUCCEEDED(mListeners->QueryElementAt(i, kScrollPositionListenerIID, (void**)&listener))) {
					listener->ScrollPositionWillChange(this, aX, aY);
					NS_RELEASE(listener);
				}
			}
		}
	}

    mViewManager->GetDeviceContext(dev);
	dev->GetAppUnitsToDevUnits(t2p); 
	dev->GetDevUnitsToAppUnits(p2t);
  
  	NS_RELEASE(dev);

	// Update the scrolled view's position

    // make sure the new position in in bounds
	GetScrolledView(scrolledView);
	scrolledView->GetDimensions(&scrolledSize.width, &scrolledSize.height);

    nsSize portSize;
	GetDimensions(&portSize.width, &portSize.height);

    nscoord maxX = scrolledSize.width - portSize.width;
    nscoord maxY = scrolledSize.height - portSize.height;

    if (aX > maxX)
        aX = maxX;

    if (aY > maxY)
        aY = maxY;

    if (aX < 0)
        aX = 0;

    if (aY < 0)
        aY = 0;

    // convert aX and aY in pixels
    nscoord aXpx = NSTwipsToIntPixels(aX, t2p);
    nscoord aYpx = NSTwipsToIntPixels(aY, t2p);

    // figure out the diff by comparing old pos to new
    dxPx = mOffsetXpx - aXpx;
    dyPx = mOffsetYpx - aYpx;
    
	if (nsnull != scrolledView)
	{
        // move the scrolled view to the new location
		scrolledView->SetPosition(-aX, -aY);

        // store old position in pixels. We need to do this to make sure there is no
        // round off errors. This could cause weird scrolling.
		mOffsetXpx = aXpx;
		mOffsetYpx = aYpx;

        // store the new position
        mOffsetX = aX;
        mOffsetY = aY;
	}

    Scroll(scrolledView, dxPx, dyPx, t2p, 0);

	// notify the listeners.
	if (nsnull != mListeners) {
		if (NS_SUCCEEDED(mListeners->Count(&listenerCount))) {
			for (PRUint32 i = 0; i < listenerCount; i++) {
				if (NS_SUCCEEDED(mListeners->QueryElementAt(i, kScrollPositionListenerIID, (void**)&listener))) {
					listener->ScrollPositionDidChange(this, aX, aY);
					NS_RELEASE(listener);
				}
			}
		}
	}

	return NS_OK;
}

NS_IMETHODIMP nsScrollPortView::SetControlInsets(const nsMargin &aInsets)
{
    return NS_OK;
}

NS_IMETHODIMP nsScrollPortView::GetControlInsets(nsMargin &aInsets) const
{
  aInsets.left = aInsets.right = aInsets.top = aInsets.bottom = 0;
  return NS_OK;
}

NS_IMETHODIMP nsScrollPortView::GetScrollbarVisibility(PRBool *aVerticalVisible,
                                                        PRBool *aHorizontalVisible) const
{ 
  NS_WARNING("Attempt to get scrollbars visibility this is not xp!");
  return NS_OK;
}


void nsScrollPortView::AdjustChildWidgets(nsScrollPortView *aScrolling, nsIView *aView, nscoord aDx, nscoord aDy, float scale)
{

  PRInt32           numkids;
  aView->GetChildCount(numkids);
  nscoord           offx, offy;

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

        win->Move(NSTwipsToIntPixels((bounds.x + aDx), scale), NSTwipsToIntPixels((bounds.y + aDy), scale));
    }

    // Don't recurse if the view has a widget, because we adjusted the view's
    // widget position, and its child widgets are relative to its positon
    if (nsnull == win)
      AdjustChildWidgets(aScrolling, kid, aDx, aDy, scale);

    if (nsnull != win)
    {
      NS_RELEASE(win);
    }
  }

}


NS_IMETHODIMP nsScrollPortView::SetScrolledView(nsIView *aScrolledView)
{
  PRInt32 count = 0;
  GetChildCount(count);

  NS_ASSERTION(count <= 1,"Error scroll port has too many children");

  // if there is already a child so remove it
  if (count == 1)
  {
     nsIView* child = nsnull;
     GetChild(0, child);
     mViewManager->RemoveChild(this, child);
  }

  return mViewManager->InsertChild(this, aScrolledView, 0);

}

NS_IMETHODIMP nsScrollPortView::GetScrolledView(nsIView *&aScrolledView) const
{
  return GetChild(0, aScrolledView);
}

NS_IMETHODIMP nsScrollPortView::GetScrollPosition(nscoord &aX, nscoord &aY) const
{
  aX = mOffsetX;
  aY = mOffsetY;

  return NS_OK;
}

NS_IMETHODIMP nsScrollPortView::SetScrollProperties(PRUint32 aProperties)
{
  mScrollProperties = aProperties;
  return NS_OK;
}

NS_IMETHODIMP nsScrollPortView::GetScrollProperties(PRUint32 *aProperties)
{
  *aProperties = mScrollProperties;
  return NS_OK;
}

NS_IMETHODIMP nsScrollPortView::SetLineHeight(nscoord aHeight)
{
  mLineHeight = aHeight;
  return NS_OK;
}

NS_IMETHODIMP nsScrollPortView::GetLineHeight(nscoord *aHeight)
{
  *aHeight = mLineHeight;
  return NS_OK;
}

NS_IMETHODIMP nsScrollPortView::ScrollByLines(PRInt32 aNumLines)
{
    nscoord dy = mLineHeight*aNumLines;

    ScrollTo(mOffsetX, mOffsetY + dy, 0);

	return NS_OK;
}

NS_IMETHODIMP nsScrollPortView::ScrollByPages(PRInt32 aNumPages)
{
    nsSize size;
	GetDimensions(&size.width, &size.height);
    
    // scroll % of the window
    nscoord dy = nscoord(float(size.height)*PAGE_SCROLL_PERCENT);

    // put in the number of pages.
    dy *= aNumPages;

    ScrollTo(mOffsetX, mOffsetY + dy, 0);
    

	return NS_OK;
}

NS_IMETHODIMP nsScrollPortView::ScrollByWhole(PRBool aTop)
{
  nscoord   newPos = 0;

  if (!aTop) {
    nsSize scrolledSize;
    nsIView* scrolledView = nsnull;
    GetScrolledView(scrolledView);
	scrolledView->GetDimensions(&scrolledSize.width, &scrolledSize.height);
    newPos = scrolledSize.height;
  }

	ScrollTo(0, newPos, 0);

	return NS_OK;
}

PRBool nsScrollPortView::CannotBitBlt(nsIView* aScrolledView)
{
  
  PRBool    trans;
  float     opacity;
  PRUint32  scrolledViewFlags;

  HasTransparency(trans);
  GetOpacity(opacity);
  aScrolledView->GetViewFlags(&scrolledViewFlags);

  return ((trans || opacity) && !(mScrollProperties & NS_SCROLL_PROPERTY_ALWAYS_BLIT)) ||
         (mScrollProperties & NS_SCROLL_PROPERTY_NEVER_BLIT) ||
         (scrolledViewFlags & NS_VIEW_PUBLIC_FLAG_DONT_BITBLT);
}


void nsScrollPortView::Scroll(nsIView *aScrolledView, PRInt32 aDx, PRInt32 aDy, float scale, PRUint32 aUpdateFlags)
{
  if ((aDx != 0) || (aDy != 0))
  {
    
    nsIWidget *scrollWidget = nsnull;

    GetWidget(scrollWidget);
    
    if (nsnull == scrollWidget)
    {
      // if we don't have a scroll widget then we must just update.
      mViewManager->UpdateView(this, 0);

    } else if (CannotBitBlt(aScrolledView)) {
        // we can't blit for some reason just update the view and adjust any heavy weight widgets
        mViewManager->UpdateView(this, 0);
        AdjustChildWidgets(this, aScrolledView, 0, 0, scale);
    } else { // if we can blit and have a scrollwidget then scroll.
      // Scroll the contents of the widget by the specfied amount, and scroll
      // the child widgets
      scrollWidget->Scroll(aDx, aDy, nsnull);
    }
    
    NS_IF_RELEASE(scrollWidget);
  }
}

NS_IMETHODIMP nsScrollPortView::Paint(nsIRenderingContext& rc, const nsRect& rect,
                                  PRUint32 aPaintFlags, PRBool &aResult)
{
    nsresult rv = nsView::Paint(rc, rect, aPaintFlags, aResult);
    return rv;
}
