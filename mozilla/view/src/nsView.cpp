/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file arhandlee subject to the Netscape Public License
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

#include "nsView.h"
#include "nsIWidget.h"
#include "nsIViewManager.h"
#include "nsIFrame.h"
#include "nsIPresContext.h"
#include "nsIWidget.h"
#include "nsIButton.h"
#include "nsIScrollbar.h"
#include "nsGUIEvent.h"
#include "nsIDeviceContext.h"
#include "nsRepository.h"
#include "nsIRenderingContext.h"
#include "nsTransform2D.h"
#include "nsIScrollableView.h"

static NS_DEFINE_IID(kIViewIID, NS_IVIEW_IID);
static NS_DEFINE_IID(kIScrollableViewIID, NS_ISCROLLABLEVIEW_IID);

static nsEventStatus PR_CALLBACK HandleEvent(nsGUIEvent *aEvent);


//#define SHOW_VIEW_BORDERS
//#define HIDE_ALL_WIDGETS

//
// Main events handler
//
nsEventStatus PR_CALLBACK HandleEvent(nsGUIEvent *aEvent)
{ 
//printf(" %d %d %d (%d,%d) \n", aEvent->widget, aEvent->widgetSupports, 
//       aEvent->message, aEvent->point.x, aEvent->point.y);
  nsEventStatus result = nsEventStatus_eIgnore;
  
  nsIView*      view = nsView::GetViewFor(aEvent->widget);
  if (nsnull != view) {
    nsIViewManager*  vm;

    view->GetViewManager(vm);
    vm->DispatchEvent(aEvent, result);
    NS_RELEASE(vm);
  }

  return result;
}

nsView :: nsView()
{
  mVis = nsViewVisibility_kShow;
  mXForm = nsnull;
  mVFlags = ~ALL_VIEW_FLAGS;
}

nsView :: ~nsView()
{
  mVFlags |= VIEW_FLAG_DYING;

  PRInt32 numKids;
  GetChildCount(numKids);
  if (numKids > 0)
  {
    nsIView *kid;

    //nuke the kids
    do {
      GetChild(0, kid);
      if (nsnull != kid)
        kid->Destroy();
    } while (nsnull != kid);
  }

  if (mXForm != nsnull)
  {
    delete mXForm;
    mXForm = nsnull;
  }

  if (nsnull != mViewManager)
  {
    nsIView *rootView;
    mViewManager->GetRootView(rootView);

    if (nsnull != rootView)
    {
      if (rootView == this)
      {
        // Inform the view manager that the root view has gone away...
        mViewManager->SetRootView(nsnull);
      }
      else
      {
        if (nsnull != mParent)
        {
          mViewManager->RemoveChild(mParent, this);
        }
      }
    } 
    else if (nsnull != mParent)
    {
      mParent->RemoveChild(this);
    }

    mViewManager = nsnull;
  }
  else if (nsnull != mParent)
  {
    mParent->RemoveChild(this);
  }

  // Destroy and release the widget
  if (nsnull != mWindow) {
    mWindow->SetClientData(nsnull);
    mWindow->Destroy();
    NS_RELEASE(mWindow);
  }
}

nsresult nsView :: QueryInterface(const nsIID& aIID, void** aInstancePtr)
{
  if (nsnull == aInstancePtr) {
    return NS_ERROR_NULL_POINTER;
  }
  static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);
  static NS_DEFINE_IID(kClassIID, kIViewIID);

  if (aIID.Equals(kClassIID) || (aIID.Equals(kISupportsIID))) {
    *aInstancePtr = (void*)(nsIView*)this;
    return NS_OK;
  }

  return NS_NOINTERFACE;
}

nsrefcnt nsView::AddRef() 
{
  NS_WARNING("not supported for views");
  return 1;
}

nsrefcnt nsView::Release()
{
  NS_WARNING("not supported for views");
  return 1;
}

nsIView* nsView::GetViewFor(nsIWidget* aWidget)
{           
  nsIView*  view = nsnull;
  void*     clientData;

  // The widget's client data points back to the owning view
  if (NS_SUCCEEDED(aWidget->GetClientData(clientData))) {
    view = (nsIView*)clientData;

    if (nsnull != view) {
#ifdef NS_DEBUG
      // Verify the pointer is really a view
      nsView* widgetView;
      NS_ASSERTION((NS_SUCCEEDED(view->QueryInterface(kIViewIID, (void **)&widgetView))) &&
                   (widgetView == view), "bad client data");
#endif
    }  
  }

  return view;
}

NS_IMETHODIMP nsView :: Init(nsIViewManager* aManager,
                             const nsRect &aBounds,
                             nsIView *aParent,
                             const nsCID *aWindowCIID,
                             nsWidgetInitData *aWidgetInitData,
                             nsNativeWidget aNative,
                             PRInt32 aZIndex,
                             const nsViewClip *aClip,
                             float aOpacity,
                             nsViewVisibility aVisibilityFlag)
{
  //printf(" \n callback=%d data=%d", aWidgetCreateCallback, aCallbackData);
  NS_PRECONDITION(nsnull != aManager, "null ptr");
  if (nsnull == aManager) {
    return NS_ERROR_NULL_POINTER;
  }
  if (nsnull != mViewManager) {
    return NS_ERROR_ALREADY_INITIALIZED;
  }
  // we don't hold a reference to the view manager
  mViewManager = aManager;

  if (aClip != nsnull)
    mClip = *aClip;
  else
  {
    mClip.mLeft = 0;
    mClip.mRight = 0;
    mClip.mTop = 0;
    mClip.mBottom = 0;
  }

  mOpacity = aOpacity;
  mZindex = aZIndex;

  // assign the parent view
  SetParent(aParent);

  SetBounds(aBounds);

  // check if a real window has to be created
  if (aWindowCIID)
  {
    nsIDeviceContext  *dx;
    nsRect            trect = aBounds;
    float             scale;

    mViewManager->GetDeviceContext(dx);
    dx->GetAppUnitsToDevUnits(scale);

    trect *= scale;

    if (NS_OK == LoadWidget(*aWindowCIID))
    {
      if (aNative)
        mWindow->Create(aNative, trect, ::HandleEvent, dx, nsnull, nsnull, aWidgetInitData);
      else
      {
        nsIWidget *parent;
        GetOffsetFromWidget(nsnull, nsnull, parent);
        mWindow->Create(parent, trect, ::HandleEvent, dx, nsnull, nsnull, aWidgetInitData);
        NS_IF_RELEASE(parent);
      }
    }

    NS_RELEASE(dx);
  }

  SetVisibility(aVisibilityFlag);

  return NS_OK;
}

NS_IMETHODIMP nsView :: Destroy()
{
  delete this;
  return NS_OK;
}

NS_IMETHODIMP nsView :: GetViewManager(nsIViewManager *&aViewMgr)
{
  NS_IF_ADDREF(mViewManager);
  aViewMgr = mViewManager;
  return NS_OK;
}

NS_IMETHODIMP nsView :: GetWidget(nsIWidget *&aWidget)
{
  NS_IF_ADDREF(mWindow);
  aWidget = mWindow;
  return NS_OK;
}

NS_IMETHODIMP nsView :: Paint(nsIRenderingContext& rc, const nsRect& rect,
                              PRUint32 aPaintFlags, nsIView *aBackstop, PRBool &aResult)
{
  nsIView *pRoot;
  PRBool  clipres = PR_FALSE;
  PRBool  clipwasset = PR_FALSE;

  mViewManager->GetRootView(pRoot);
  rc.PushState();

  if (aPaintFlags & NS_VIEW_FLAG_CLIP_SET)
  {
    clipwasset = PR_TRUE;
    aPaintFlags &= ~NS_VIEW_FLAG_CLIP_SET;
  }
  else if (mVis == nsViewVisibility_kShow)
  {
    nsRect brect;

    GetBounds(brect);

    if ((mClip.mLeft != mClip.mRight) && (mClip.mTop != mClip.mBottom))
    {
      nsRect  crect;

      crect.x = mClip.mLeft + brect.x;
      crect.y = mClip.mTop + brect.y;
      crect.width = mClip.mRight - mClip.mLeft;
      crect.height = mClip.mBottom - mClip.mTop;

      clipres = rc.SetClipRect(crect, nsClipCombine_kIntersect);
    }
    else if (this != pRoot)
      clipres = rc.SetClipRect(brect, nsClipCombine_kIntersect);
  }

  if (nsnull != mXForm)
  {
    nsTransform2D *pXForm = rc.GetCurrentTransform();
    pXForm->Concatenate(mXForm);
  }

  if (clipres == PR_FALSE)
  {
    nscoord posx, posy;

    GetPosition(&posx, &posy);

    rc.Translate(posx, posy);

    PRInt32 numkids;
    GetChildCount(numkids);

    for (PRInt32 cnt = 0; cnt < numkids; cnt++)
    {
      nsIView *kid;
      GetChild(cnt, kid);

      if (nsnull != kid)
      {
        nsRect kidRect;
        kid->GetBounds(kidRect);
        nsRect damageArea;
        PRBool overlap = damageArea.IntersectRect(rect, kidRect);

        if (overlap == PR_TRUE)
        {
          // Translate damage area into kid's coordinate system
          nsRect kidDamageArea(damageArea.x - kidRect.x, damageArea.y - kidRect.y,
                               damageArea.width, damageArea.height);
          kid->Paint(rc, kidDamageArea, aPaintFlags, nsnull, clipres);

          if (clipres == PR_TRUE)
            break;
        }
      }
    }

    if ((clipres == PR_FALSE) && (mVis == nsViewVisibility_kShow))
    {
      float opacity;
      GetOpacity(opacity);

      if (opacity > 0.0f)
      {
        rc.PushState();

        PRBool  hasTransparency;
        HasTransparency(hasTransparency);

        if (hasTransparency || (opacity < 1.0f))
        {
          //overview of algorithm:
          //1. clip is set to intersection of this view and whatever is
          //   left of the damage region in the rc.
          //2. walk tree from this point down through the view list,
          //   rendering and clipping out opaque views encountered until
          //   there is nothing left in the clip area or the bottommost
          //   view is reached.
          //3. walk back up through view list restoring clips and painting
          //   or blending any non-opaque views encountered until we reach the
          //   view that started the whole process

          //walk down rendering only views within this clip

          nsIView *child, *prevchild = this;
          GetNextSibling(child);

          while (nsnull != child)
          {
            nsRect kidRect;
            child->GetBounds(kidRect);
            nsRect damageArea;
            PRBool overlap = damageArea.IntersectRect(rect, kidRect);

            //as we tell each kid to paint, we need to mark the kid as one that was hit
            //in the front to back rendering so that when we do the back to front pass,
            //we can re-add the child's rect back into the clip.

            if (overlap == PR_TRUE)
            {
              // Translate damage area into kid's coordinate system
              nsRect kidDamageArea(damageArea.x - kidRect.x, damageArea.y - kidRect.y,
                                   damageArea.width, damageArea.height);
              child->Paint(rc, kidDamageArea, aPaintFlags, nsnull, clipres);
            }

            prevchild = child;

            child->GetNextSibling(child);

            if (nsnull == child)
              child->GetParent(child);

            if (clipres == PR_TRUE)
              break;
          }

          if ((nsnull != prevchild) && (this != prevchild))
          {
            //walk backwards, rendering views
          }
        }

        if (nsnull != mClientData)
        {
          nsIViewObserver *obs;

          if (NS_OK == mViewManager->GetViewObserver(obs))
          {
            obs->Paint((nsIView *)this, rc, rect);
            NS_RELEASE(obs);
          }
        }

#ifdef SHOW_VIEW_BORDERS
        {
          nscoord x, y, w, h;

          if ((mClip.mLeft != mClip.mRight) && (mClip.mTop != mClip.mBottom))
          {
            x = mClip.mLeft;
            y = mClip.mTop;
            w = mClip.mRight - mClip.mLeft;
            h = mClip.mBottom - mClip.mTop;

            rc.SetColor(NS_RGB(255, 255, 0));
          }
          else
          {
            x = y = 0;

            GetDimensions(&w, &h);

            if (nsnull != mWindow)
              rc.SetColor(NS_RGB(0, 255, 0));
            else
              rc.SetColor(NS_RGB(0, 0, 255));
          }

          rc.DrawRect(x, y, w, h);
        }
#endif

        rc.PopState();
      }
    }
  }

  //XXX would be nice if we could have a version of pop that just removes the
  //state from the stack but doesn't change the state of the underlying graphics
  //context. MMP

  clipres = rc.PopState();

  //now we need to exclude this view from the rest of the
  //paint process. only do this if this view is actually
  //visible and if there is no widget (like a scrollbar) here.

//  if ((clipres == PR_FALSE) && (mVis == nsViewVisibility_kShow))
  if (!clipwasset && (clipres == PR_FALSE) && (mVis == nsViewVisibility_kShow) && (nsnull == mWindow))
  {
    nsRect  brect;

    GetBounds(brect);

    if ((mClip.mLeft != mClip.mRight) && (mClip.mTop != mClip.mBottom))
    {
      nsRect  crect;

      crect.x = mClip.mLeft + brect.x;
      crect.y = mClip.mTop + brect.y;
      crect.width = mClip.mRight - mClip.mLeft;
      crect.height = mClip.mBottom - mClip.mTop;

      clipres = rc.SetClipRect(crect, nsClipCombine_kSubtract);
    }
    else if (this != pRoot)
      clipres = rc.SetClipRect(brect, nsClipCombine_kSubtract);
  }

  aResult = clipres;
  return NS_OK;
}

NS_IMETHODIMP nsView :: Paint(nsIRenderingContext& rc, const nsIRegion& region,
                              PRUint32 aPaintFlags, PRBool &aResult)
{
  // XXX apply region to rc
  // XXX get bounding rect from region
  //if (nsnull != mClientData)
  //{
  //  nsIViewObserver *obs;
  //
  //  if (NS_OK == mViewManager->GetViewObserver(obs))
  //  {
  //    obs->Paint((nsIView *)this, rc, rect, aPaintFlags);
  //    NS_RELEASE(obs);
  //  }
  //}
  aResult = PR_FALSE;
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsView :: HandleEvent(nsGUIEvent *event, PRUint32 aEventFlags,
                                    nsEventStatus &aStatus)
{
//printf(" %d %d %d %d (%d,%d) \n", this, event->widget, event->widgetSupports, 
//       event->message, event->point.x, event->point.y);
  aStatus = nsEventStatus_eIgnore;

  //see if any of this view's children can process the event
  if (aStatus == nsEventStatus_eIgnore) {
    PRInt32 numkids;
    nsRect  trect;
    nscoord x, y;

    GetChildCount(numkids);
    x = event->point.x;
    y = event->point.y;

    for (PRInt32 cnt = 0; cnt < numkids; cnt++)
    {
      nsIView *pKid;
      nscoord lx, ly;

      GetChild(cnt, pKid);
      pKid->GetBounds(trect);

      lx = x - trect.x;
      ly = y - trect.y;

      if (trect.Contains(lx, ly))
      {
        //the x, y position of the event in question
        //is inside this child view, so give it the
        //opportunity to handle the event

        event->point.x -= trect.x;
        event->point.y -= trect.y;

        pKid->HandleEvent(event, NS_VIEW_FLAG_CHECK_CHILDREN, aStatus);

        event->point.x += trect.x;
        event->point.y += trect.y;

        if (aStatus != nsEventStatus_eIgnore)
          break;
      }
    }
  }

  //if the view's children didn't take the event, check the view itself.
  if ((aStatus == nsEventStatus_eIgnore) && (nsnull != mClientData))
  {
    nsIViewObserver *obs;

    if (NS_OK == mViewManager->GetViewObserver(obs))
    {
      nscoord xoff, yoff;

      GetScrollOffset(&xoff, &yoff);

      event->point.x += xoff;
      event->point.y += yoff;

      obs->HandleEvent((nsIView *)this, event, aStatus);

      event->point.x -= xoff;
      event->point.y -= yoff;

      NS_RELEASE(obs);
    }
  }

  return NS_OK;
}

NS_IMETHODIMP nsView :: SetPosition(nscoord x, nscoord y)
{
  mBounds.MoveTo(x, y);

  if (nsnull != mWindow)
  {
    nsIDeviceContext  *dx;
    nscoord           offx, offy, parx = 0, pary = 0;
    float             scale;
    nsIWidget         *pwidget = nsnull;
  
    mViewManager->GetDeviceContext(dx);
    dx->GetAppUnitsToDevUnits(scale);

    GetScrollOffset(&offx, &offy);

    GetOffsetFromWidget(&parx, &pary, pwidget);
    NS_IF_RELEASE(pwidget);
    
    mWindow->Move(NSTwipsToIntPixels((x + parx - offx), scale),
                  NSTwipsToIntPixels((y + pary - offy), scale));

    NS_RELEASE(dx);
  }

  return NS_OK;
}

NS_IMETHODIMP nsView :: GetPosition(nscoord *x, nscoord *y)
{
  nsIView *rootView;

  mViewManager->GetRootView(rootView);
  if (this == rootView)
    *x = *y = 0;
  else
  {
    *x = mBounds.x;
    *y = mBounds.y;
  }

  return NS_OK;
}

NS_IMETHODIMP nsView :: SetDimensions(nscoord width, nscoord height, PRBool aPaint)
{
  mBounds.SizeTo(width, height);

  if (nsnull != mParent)
  {
    nsIScrollableView *scroller;

    static NS_DEFINE_IID(kscroller, NS_ISCROLLABLEVIEW_IID);

    if (NS_OK == mParent->QueryInterface(kscroller, (void **)&scroller))
    {
      scroller->ComputeContainerSize();
    }
  }

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

NS_IMETHODIMP nsView :: GetDimensions(nscoord *width, nscoord *height)
{
  *width = mBounds.width;
  *height = mBounds.height;
  return NS_OK;
}

NS_IMETHODIMP nsView :: SetBounds(const nsRect &aBounds, PRBool aPaint)
{
  SetPosition(aBounds.x, aBounds.y);
  SetDimensions(aBounds.width, aBounds.height, aPaint);
  return NS_OK;
}

NS_IMETHODIMP nsView :: SetBounds(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight, PRBool aPaint)
{
  SetPosition(aX, aY);
  SetDimensions(aWidth, aHeight, aPaint);
  return NS_OK;
}

NS_IMETHODIMP nsView :: GetBounds(nsRect &aBounds) const
{
  nsIView *rootView;

  mViewManager->GetRootView(rootView);
  aBounds = mBounds;

  if ((nsIView *)this == rootView)
    aBounds.x = aBounds.y = 0;

  return NS_OK;
}

NS_IMETHODIMP nsView :: SetClip(nscoord aLeft, nscoord aTop, nscoord aRight, nscoord aBottom)
{
  mClip.mLeft = aLeft;
  mClip.mTop = aTop;
  mClip.mRight = aRight;
  mClip.mBottom = aBottom;
  return NS_OK;
}

NS_IMETHODIMP nsView :: GetClip(nscoord *aLeft, nscoord *aTop, nscoord *aRight, nscoord *aBottom,
                                PRBool &aResult)
{
  if ((mClip.mLeft == mClip.mRight) || (mClip.mTop == mClip.mBottom))
    aResult = PR_FALSE;
  else
  {
    *aLeft = mClip.mLeft;
    *aTop = mClip.mTop;
    *aRight = mClip.mRight;
    *aBottom = mClip.mBottom;
    aResult = PR_TRUE;
  }

  return NS_OK;
}

NS_IMETHODIMP nsView :: SetVisibility(nsViewVisibility aVisibility)
{
  mVis = aVisibility;

  if (nsnull != mWindow)
  {
#ifndef HIDE_ALL_WIDGETS
    if (mVis == nsViewVisibility_kShow)
      mWindow->Show(PR_TRUE);
    else
#endif
      mWindow->Show(PR_FALSE);
  }

  return NS_OK;
}

NS_IMETHODIMP nsView :: GetVisibility(nsViewVisibility &aVisibility)
{
  aVisibility = mVis;
  return NS_OK;
}

NS_IMETHODIMP nsView :: SetZIndex(PRInt32 zindex)
{
  mZindex = zindex;
  return NS_OK;
}

NS_IMETHODIMP nsView :: GetZIndex(PRInt32 &aZIndex)
{
  aZIndex = mZindex;
  return NS_OK;
}

NS_IMETHODIMP nsView :: SetParent(nsIView *aParent)
{
  mParent = aParent;
  return NS_OK;
}

NS_IMETHODIMP nsView :: GetParent(nsIView *&aParent)
{
  aParent = mParent;
  return NS_OK;
}

NS_IMETHODIMP nsView :: GetNextSibling(nsIView *&aNextSibling) const
{
  aNextSibling = mNextSibling;
  return NS_OK;
}

NS_IMETHODIMP nsView::SetNextSibling(nsIView* aView)
{
  mNextSibling = aView;
  return NS_OK;
}

NS_IMETHODIMP nsView :: InsertChild(nsIView *child, nsIView *sibling)
{
  NS_PRECONDITION(nsnull != child, "null ptr");
  if (nsnull != child)
  {
    if (nsnull != sibling)
    {
#ifdef NS_DEBUG
      nsIView*  siblingParent;
      sibling->GetParent(siblingParent);
      NS_ASSERTION(siblingParent == this, "tried to insert view with invalid sibling");
#endif
      //insert after sibling
      nsIView*  siblingNextSibling;
      sibling->GetNextSibling(siblingNextSibling);
      child->SetNextSibling(siblingNextSibling);
      sibling->SetNextSibling(child);
    }
    else
    {
      child->SetNextSibling(mFirstChild);
      mFirstChild = child;
    }
    child->SetParent(this);
    mNumKids++;
  }

  return NS_OK;
}

NS_IMETHODIMP nsView :: RemoveChild(nsIView *child)
{
  NS_PRECONDITION(nsnull != child, "null ptr");

  if (nsnull != child)
  {
    nsIView* prevKid = nsnull;
    nsIView* kid = mFirstChild;
    PRBool found = PR_FALSE;
    while (nsnull != kid) {
      if (kid == child) {
        if (nsnull != prevKid) {
          nsIView*  kidNextSibling;
          kid->GetNextSibling(kidNextSibling);
          prevKid->SetNextSibling(kidNextSibling);
        } else {
          kid->GetNextSibling(mFirstChild);
        }
        child->SetParent(nsnull);
        mNumKids--;
        found = PR_TRUE;
        break;
      }
      prevKid = kid;
	    kid->GetNextSibling(kid);
    }
    NS_ASSERTION(found, "tried to remove non child");
  }

  return NS_OK;
}

NS_IMETHODIMP nsView :: GetChildCount(PRInt32 &aCount)
{
  aCount = mNumKids;
  return NS_OK;
}

NS_IMETHODIMP nsView :: GetChild(PRInt32 index, nsIView *&aChild)
{ 
  NS_PRECONDITION(!(index > mNumKids), "bad index");

  aChild = nsnull;
  if (index < mNumKids)
  {
    aChild = mFirstChild;
    for (PRInt32 cnt = 0; (cnt < index) && (nsnull != aChild); cnt++) {
      aChild->GetNextSibling(aChild);
    }
  }

  return NS_OK;
}

NS_IMETHODIMP nsView :: SetTransform(nsTransform2D &aXForm)
{
  if (nsnull == mXForm)
    mXForm = new nsTransform2D(&aXForm);
  else
    *mXForm = aXForm;

  return NS_OK;
}

NS_IMETHODIMP nsView :: GetTransform(nsTransform2D &aXForm)
{
  if (nsnull != mXForm)
    aXForm = *mXForm;
  else
    aXForm.SetToIdentity();

  return NS_OK;
}

NS_IMETHODIMP nsView :: SetOpacity(float opacity)
{
  mOpacity = opacity;
  return NS_OK;
}

NS_IMETHODIMP nsView :: GetOpacity(float &aOpacity)
{
  aOpacity = mOpacity;
  return NS_OK;
}

NS_IMETHODIMP nsView :: HasTransparency(PRBool &aTransparent)
{
  aTransparent = (mVFlags & VIEW_FLAG_TRANSPARENT) ? PR_TRUE : PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP nsView :: SetContentTransparency(PRBool aTransparent)
{
  if (aTransparent == PR_TRUE)
    mVFlags |= VIEW_FLAG_TRANSPARENT;
  else
    mVFlags &= ~VIEW_FLAG_TRANSPARENT;

  return NS_OK;
}

NS_IMETHODIMP nsView :: SetClientData(void *aData)
{
  mClientData = aData;
  return NS_OK;
}

NS_IMETHODIMP nsView :: GetClientData(void *&aData)
{
  aData = mClientData;
  return NS_OK;
}

//
// internal window creation functions
//
nsresult nsView :: LoadWidget(const nsCID &aClassIID)
{
  nsISupports*  window;
  nsresult      rv;

  static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);
  rv = NSRepository::CreateInstance(aClassIID, nsnull, kISupportsIID, (void**)&window);

  if (NS_OK == rv) {
    // get a pointer to the nsIWidget* interface
    static NS_DEFINE_IID(kIWidgetIID, NS_IWIDGET_IID);
    rv = window->QueryInterface(kIWidgetIID, (void**)&mWindow);
    window->Release();

    // Set the widget's client data
    mWindow->SetClientData((void*)this);
  }

  return rv;
}

void nsView :: List(FILE* out, PRInt32 aIndent) const
{
  PRInt32 i;
  for (i = aIndent; --i >= 0; ) fputs("  ", out);
  fprintf(out, "%p ", this);
  if (nsnull != mWindow) {
    nsRect windowBounds;
    mWindow->GetBounds(windowBounds);
    fprintf(out, "(widget=%p pos={%d,%d,%d,%d}) ",
            mWindow,
            windowBounds.x, windowBounds.y,
            windowBounds.width, windowBounds.height);
  }
  nsRect brect;
  GetBounds(brect);
  out << brect;
  fprintf(out, " z=%d vis=%d opc=%1.3f <\n", mZindex, mVis, mOpacity);
  nsIView* kid = mFirstChild;
  while (nsnull != kid) {
    kid->List(out, aIndent + 1);
    kid->GetNextSibling(kid);
  }
  for (i = aIndent; --i >= 0; ) fputs("  ", out);
  fputs(">\n", out);
}

NS_IMETHODIMP nsView :: GetOffsetFromWidget(nscoord *aDx, nscoord *aDy, nsIWidget *&aWidget)
{
  nsIView   *ancestor;
  
  GetParent(ancestor);
  while (nsnull != ancestor)
  {
    ancestor->GetWidget(aWidget);
	  if (nsnull != aWidget)
	    return NS_OK;

    if ((nsnull != aDx) && (nsnull != aDy))
    {
      nscoord offx, offy;

      ancestor->GetPosition(&offx, &offy);

      *aDx += offx;
      *aDy += offy;
    }

	  ancestor->GetParent(ancestor);
  }

  aWidget = nsnull;
  return NS_OK;
}

NS_IMETHODIMP nsView :: GetScrollOffset(nscoord *aDx, nscoord *aDy)
{
  nsIWidget *window = nsnull;
  nsIView   *ancestor;
   
  GetParent(ancestor);
  while (nsnull != ancestor)
  {
    nsIScrollableView *sview;

    static NS_DEFINE_IID(kscroller, NS_ISCROLLABLEVIEW_IID);

    if (NS_OK == ancestor->QueryInterface(kscroller, (void **)&sview))
    {
      sview->GetVisibleOffset(aDx, aDy);
      return NS_OK;
    }

    ancestor->GetParent(ancestor);
  }

  *aDx = *aDy = 0;
  return NS_OK;
}
