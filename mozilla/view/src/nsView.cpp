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
#include "nsIPresShell.h"
#include "nsIWidget.h"
#include "nsIButton.h"
#include "nsIScrollbar.h"
#include "nsGUIEvent.h"
#include "nsIDeviceContext.h"
#include "nsRepository.h"
#include "nsIRenderingContext.h"
#include "nsTransform2D.h"

static NS_DEFINE_IID(kIViewIID, NS_IVIEW_IID);

//
// Main events handler
//
nsEventStatus PR_CALLBACK HandleEvent(nsGUIEvent *aEvent)
{ 
  nsIView *view;
  nsEventStatus  result = nsEventStatus_eIgnore; 

  if (NS_OK == aEvent->widget->QueryInterface(kIViewIID, (void**)&view))
  {
    switch(aEvent->message)
    {
      case NS_SIZE:
      {
        nscoord width = ((nsSizeEvent*)aEvent)->windowSize->width;
        nscoord height = ((nsSizeEvent*)aEvent)->windowSize->height;

        // Inform the view manager that the root window has been resized
        nsIViewManager*  vm = view->GetViewManager();
        nsIPresContext* presContext = vm->GetPresContext();

        // The root view may not be set if this is the resize associated with
        // window creation

        nsIView* rootView = vm->GetRootView();

        if (view == rootView)
        {
          // Convert from pixels to twips
          float p2t = presContext->GetPixelsToTwips();
          vm->SetWindowDimensions(NS_TO_INT_ROUND(width * p2t),
                                  NS_TO_INT_ROUND(height * p2t));
          result = nsEventStatus_eConsumeNoDefault;
        }

        NS_RELEASE(rootView);
        NS_RELEASE(presContext);
        NS_RELEASE(vm);

        break;
      }

      case NS_PAINT:
      {
        nsIViewManager    *vm = view->GetViewManager();
        nsIPresContext    *px = vm->GetPresContext();
        float             convert = px->GetPixelsToTwips();
        nsRect            vrect, trect = *((nsPaintEvent*)aEvent)->rect;
        nsIDeviceContext  *dx = px->GetDeviceContext();

        trect.x = NS_TO_INT_ROUND(trect.x * convert);
        trect.y = NS_TO_INT_ROUND(trect.y * convert);
        trect.width = NS_TO_INT_ROUND(trect.width * convert);
        trect.height = NS_TO_INT_ROUND(trect.height * convert);

        //see if the paint region is greater than .75 the area of our root view.
        //if so, enable double buffered painting.

        view->GetBounds(vrect);

        PRBool db = PR_FALSE;

        if ((((float)trect.width * trect.height) / ((float)vrect.width * vrect.height)) >  0.75f)
          db = PR_TRUE;

        vm->Refresh(view, ((nsPaintEvent *)aEvent)->renderingContext, &trect,
                    ((db == PR_TRUE) ? NS_VMREFRESH_DOUBLE_BUFFER : 0) | NS_VMREFRESH_SCREEN_RECT);

        NS_RELEASE(dx);
        NS_RELEASE(px);
        NS_RELEASE(vm);

        result = nsEventStatus_eConsumeNoDefault;

        break;
      }

      case NS_DESTROY:
        result = nsEventStatus_eConsumeNoDefault;
        break;

      default:
        nsIViewManager *vm = view->GetViewManager();
        nsIPresContext  *cx = vm->GetPresContext();

        // pass on to view somewhere else to deal with

        aEvent->point.x = NS_TO_INT_ROUND(aEvent->point.x * cx->GetPixelsToTwips());
        aEvent->point.y = NS_TO_INT_ROUND(aEvent->point.y * cx->GetPixelsToTwips());

        result = view->HandleEvent(aEvent);

        aEvent->point.x = NS_TO_INT_ROUND(aEvent->point.x * cx->GetTwipsToPixels());
        aEvent->point.y = NS_TO_INT_ROUND(aEvent->point.y * cx->GetTwipsToPixels());

        NS_RELEASE(cx);
        NS_RELEASE(vm);

        break;
    }

    NS_RELEASE(view);
  }

  return result;
}

nsView :: nsView()
{
  mVis = nsViewVisibility_kShow;
}

nsView :: ~nsView()
{
  if (GetChildCount() > 0)
  {
    nsIView *kid;

    //nuke the kids

    while (kid = GetChild(0))
      kid->Destroy();
  }

  if (nsnull != mViewManager)
  {
    nsIView *rootView = mViewManager->GetRootView();
    if (rootView == this) {
      //This code should never be reached since there is a circular ref between nsView and nsViewManager\n");
      mViewManager->SetRootView(nsnull);  // this resets our ref count to 0
    }
    else
    {
      if (nsnull != mParent) {
        mViewManager->RemoveChild(mParent, this);
      }
      NS_RELEASE(rootView);
    }
    NS_RELEASE(mViewManager);
    mViewManager = nsnull;
  }
  if (nsnull != mFrame) {
    // Temporarily raise our refcnt because the frame is going to
    // Release us.
    mRefCnt = 99;
    mFrame->SetView(nsnull);
    mRefCnt = 0;
  }
  if (nsnull != mInnerWindow) {
    //if (nsnull != mWindow) {
	  //  mWindow->Destroy();
    //  mWindow = nsnull;
	  //}
	  NS_RELEASE(mInnerWindow); // this should destroy the widget and its native windows
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
    AddRef();
    return NS_OK;
  }

  if (nsnull != mInnerWindow)
    return mInnerWindow->QueryInterface(aIID, aInstancePtr);

  return NS_NOINTERFACE;
}

// this should be added to nsView
nsIWidget*
GetWindowTemp(nsIView *aView)
{
  nsIWidget *window = nsnull;

  nsIView *ancestor = aView;
  while (nsnull != ancestor) {
	  if (nsnull != (window = ancestor->GetWidget())) {
	    return window;
	  }
	  ancestor = ancestor->GetParent();
  }
  return nsnull;
}

nsrefcnt nsView::AddRef() 
{
  return ++mRefCnt;
}

nsrefcnt nsView::Release()
{
  if (--mRefCnt == 0)
  {
    delete this;
    return 0;
  }
  return mRefCnt;
}

nsresult nsView :: Init(nsIViewManager* aManager,
                        const nsRect &aBounds,
                        nsIView *aParent,
                        const nsCID *aWindowCIID,
                        nsNativeWindow aNative,
                        PRInt32 aZIndex,
                        const nsRect *aClipRect,
                        float aOpacity,
                        nsViewVisibility aVisibilityFlag)
{
  NS_PRECONDITION(nsnull != aManager, "null ptr");
  if (nsnull == aManager) {
    return NS_ERROR_NULL_POINTER;
  }
  if (nsnull != mViewManager) {
    return NS_ERROR_ALREADY_INITIALIZED;
  }
  mViewManager = aManager;

  NS_ADDREF(aManager);

  mBounds = aBounds;

  // assign the parent view
  SetParent(aParent);

  // check if a real window has to be created
  if (aWindowCIID)
  {
    nsIPresContext    *cx = mViewManager->GetPresContext();
    nsIDeviceContext  *dx = cx->GetDeviceContext();
    nsRect            trect = aBounds;

    trect *= cx->GetTwipsToPixels();

    if (NS_OK == LoadWidget(*aWindowCIID))
    {
      if (aNative)
        mWindow->Create(aNative, trect, ::HandleEvent, dx);
      else
      {
        nsIWidget *parent = GetWindowTemp(aParent); 
        mWindow->Create(parent, trect, ::HandleEvent, dx);
        NS_IF_RELEASE(parent);
      }
    }

    NS_RELEASE(dx);
    NS_RELEASE(cx);
  }

  SetVisibility(aVisibilityFlag);

  return NS_OK;
}

void nsView :: Destroy()
{
  delete this;
}

nsIViewManager * nsView :: GetViewManager()
{
  NS_IF_ADDREF(mViewManager);
  return mViewManager;
}

nsIWidget * nsView :: GetWidget()
{
  NS_IF_ADDREF(mWindow);
  return mWindow;
}

void nsView :: Paint(nsIRenderingContext& rc, const nsRect& rect)
{
  if (nsnull != mFrame)
  {
    nsIPresContext  *cx = mViewManager->GetPresContext();

    mFrame->Paint(*cx, rc, rect);

    NS_RELEASE(cx);
  }
}

void nsView :: Paint(nsIRenderingContext& rc, const nsRegion& region)
{
  // XXX apply region to rc
  // XXX get bounding rect from region
  //if (nsnull != mFrame)
  //  mFrame->Paint(rc, rect);
}

nsEventStatus nsView :: HandleEvent(nsGUIEvent *event, PRBool aCheckParent, PRBool aCheckChildren)
{
  nsIScrollbar  *scroll;
  nsEventStatus retval = nsEventStatus_eIgnore;

  if (nsnull != mWindow)
  {
    //if this is a scrollbar window that sent
    //us the event, do special processing

    static NS_DEFINE_IID(kscroller, NS_ISCROLLBAR_IID);

    if (NS_OK == mWindow->QueryInterface(kscroller, (void **)&scroll))
    {
      if (nsnull != mParent)
        retval = mParent->HandleEvent(event, PR_FALSE, PR_FALSE);

      NS_RELEASE(scroll);
    }
  }

  if ((retval == nsEventStatus_eIgnore) && (nsnull != mFrame))
  {
    nsIPresContext  *cx = mViewManager->GetPresContext();
    nscoord         xoff, yoff;

    mViewManager->GetWindowOffsets(&xoff, &yoff);

    event->point.x += xoff;
    event->point.y += yoff;

    mFrame->HandleEvent(*cx, event, retval);

    event->point.x -= xoff;
    event->point.y -= yoff;

    NS_RELEASE(cx);
  }

  //see if any of this view's children can process the event
  
  if ((PR_TRUE == aCheckChildren) && (retval == nsEventStatus_eIgnore))
  {
    PRInt32 numkids = GetChildCount();
    nsRect  trect;
    nscoord x, y;

    x = event->point.x;
    y = event->point.y;

    for (PRInt32 cnt = 0; cnt < numkids; cnt++)
    {
      nsIView *pKid = GetChild(cnt);
      nscoord lx, ly;
      
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

        retval = pKid->HandleEvent(event, PR_FALSE, PR_TRUE);

        event->point.x += trect.x;
        event->point.y += trect.y;

        if (retval != nsEventStatus_eIgnore)
          break;
      }
    }
  }

  //see if any of this views siblings can process this event
  //we only go from the next sibling since this is a z-ordered
  //list

  if (retval == nsEventStatus_eIgnore)
  {
    nsIView *pNext = GetNextSibling();

    while (pNext)
    {
      retval = pNext->HandleEvent(event, PR_FALSE, PR_TRUE);

      if (retval != PR_FALSE)
        break;

      pNext = pNext->GetNextSibling();
    }
  }
  
  //no-one has a clue what to do with this... so ask the
  //parents. kind of mimics life, huh?

  if ((PR_TRUE == aCheckParent) && (retval == PR_FALSE))
  {
    nsIView *pParent = GetParent();

    while (pParent)
    {
      retval = pParent->HandleEvent(event, PR_FALSE, PR_FALSE);

      if (retval != PR_FALSE)
        break;

      pParent = pParent->GetParent();
    }
  }

  return retval;
}

void nsView :: SetPosition(nscoord x, nscoord y)
{
  mBounds.MoveTo(x, y);

  if (nsnull != mWindow)
  {
    nsIPresContext  *px = mViewManager->GetPresContext();
    nscoord         offx, offy;
    float           scale = px->GetTwipsToPixels();

    mViewManager->GetWindowOffsets(&offx, &offy);
    
    mWindow->Move(NS_TO_INT_ROUND((x + offx) * scale), NS_TO_INT_ROUND((y + offy) * scale));

    NS_RELEASE(px);
  }
}

void nsView :: GetPosition(nscoord *x, nscoord *y)
{
  *x = mBounds.x;
  *y = mBounds.y;
}

#include "nsScrollingView.h"

void nsView :: SetDimensions(nscoord width, nscoord height)
{
  mBounds.SizeTo(width, height);

  //XXX this is a hack. pretend you don't see it.
  //it will go away soon, i promise. MMP

  if (nsnull != mParent)
  {
    nsScrollingView *root = (nsScrollingView *)mViewManager->GetRootView();

    if (mParent == root)
    {
      root->SetContainerSize(mBounds.height);
    }

    if (nsnull != mWindow)
    {
      nsIPresContext  *px = mViewManager->GetPresContext();
      float           t2p = px->GetTwipsToPixels();
    
      mWindow->Resize(NS_TO_INT_ROUND(t2p * width), NS_TO_INT_ROUND(t2p * height));

      NS_RELEASE(px);
    }

    NS_RELEASE(root);
  }
}

void nsView :: GetDimensions(nscoord *width, nscoord *height)
{
  *width = mBounds.width;
  *height = mBounds.height;
}

void nsView :: SetBounds(const nsRect &aBounds)
{
  mBounds = aBounds;

  SetPosition(aBounds.x, aBounds.y);
  SetDimensions(aBounds.width, aBounds.height);
}

void nsView :: SetBounds(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight)
{
  mBounds.x = aX;
  mBounds.y = aY;

  SetPosition(aX, aY);
  SetDimensions(aWidth, aHeight);
}

void nsView :: GetBounds(nsRect &aBounds)
{
  aBounds = mBounds;
}

void nsView :: SetClip(const nsRect &aClip)
{
}

void nsView :: SetClip(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight)
{
}

PRBool nsView :: GetClip(nsRect *aClip)
{
  return PR_FALSE;
}

void nsView :: SetVisibility(nsViewVisibility aVisibility)
{
  mVis = aVisibility;

  if (nsnull != mWindow)
  {
    if (mVis == nsViewVisibility_kShow)
      mWindow->Show(PR_TRUE);
    else
      mWindow->Show(PR_FALSE);
  }
}

nsViewVisibility nsView :: GetVisibility()
{
  return mVis;
}

void nsView :: SetZIndex(PRInt32 zindex)
{
  mZindex = zindex;
}

PRInt32 nsView :: GetZIndex()
{
  return mZindex;
}

void nsView :: SetParent(nsIView *aParent)
{
  mParent = aParent;
}

nsIView * nsView :: GetParent()
{
  return mParent;
}

nsIView * nsView :: GetNextSibling() const
{
  return mNextSibling;
}

void nsView::SetNextSibling(nsIView* aView)
{
  mNextSibling = aView;
}

void nsView :: InsertChild(nsIView *child, nsIView *sibling)
{
  NS_PRECONDITION(nsnull != child, "null ptr");
  if (nsnull != child)
  {
    if (nsnull != sibling)
    {
      NS_ASSERTION(sibling->GetParent() != this, "tried to insert view with invalid sibling");
      //insert after sibling
      child->SetNextSibling(sibling->GetNextSibling());
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
}

void nsView :: RemoveChild(nsIView *child)
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
          prevKid->SetNextSibling(kid->GetNextSibling());
        } else {
          mFirstChild = kid->GetNextSibling();
        }
        child->SetParent(nsnull);
        mNumKids--;
        found = PR_TRUE;
        break;
      }
	    kid = kid->GetNextSibling();
    }
    NS_ASSERTION(found, "tried to remove non child");
  }
}

PRInt32 nsView :: GetChildCount()
{
  return mNumKids;
}

nsIView * nsView :: GetChild(PRInt32 index)
{ 
  NS_PRECONDITION(!(index > mNumKids), "bad index");

  if (index < mNumKids)
  {
    nsIView *kid = mFirstChild;
    for (PRInt32 cnt = 0; (cnt < index) && (nsnull != kid); cnt++) {
      kid = kid->GetNextSibling();
    }
    return kid;
  }
  return nsnull;
}

void nsView :: SetTransform(nsTransform2D *transform)
{
}

nsTransform2D * nsView :: GetTransform()
{
  return nsnull;
}

void nsView :: SetOpacity(float opacity)
{
}

float nsView :: GetOpacity()
{
  return 1.0f;
}

PRBool nsView :: HasTransparency()
{
  return PR_FALSE;
}

// Frames have a pointer to the view, so don't AddRef the frame.
void nsView :: SetFrame(nsIFrame *aFrame)
{
  mFrame = aFrame;
}

nsIFrame * nsView :: GetFrame()
{
  return mFrame;
}

//
// internal window creation functions
//
nsresult nsView :: LoadWidget(const nsCID &aClassIID)
{
  nsresult rv;

  static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);
  rv = NSRepository::CreateInstance(aClassIID, this, kISupportsIID, (void**)&mInnerWindow);

  if (NS_OK == rv) {
    // load the convenience nsIWidget pointer.
    // NOTE: mWindow is released so not to create a circulare refcount
    static NS_DEFINE_IID(kIWidgetIID, NS_IWIDGET_IID);
    rv = mInnerWindow->QueryInterface(kIWidgetIID, (void**)&mWindow);
    if (NS_OK != rv) {
	    mInnerWindow->Release();
	    mInnerWindow = NULL;
    }
    else {
      mWindow->Release();
    }
  }

  return rv;
}

void nsView :: List(FILE* out, PRInt32 aIndent) const
{
  for (PRInt32 i = aIndent; --i >= 0; ) fputs("  ", out);
  fprintf(out, "%p win=%p ", this, mWindow);
  out << mBounds;
  fputs("<\n", out);
  nsIView* kid = mFirstChild;
  while (nsnull != kid) {
    kid->List(out, aIndent + 1);
    kid = kid->GetNextSibling();
  }
  for (i = aIndent; --i >= 0; ) fputs("  ", out);
  fputs(">\n", out);
}

void nsView :: AdjustChildWidgets(nscoord aDx, nscoord aDy)
{
  PRInt32 numkids = GetChildCount();

  for (PRInt32 cnt = 0; cnt < numkids; cnt++)
  {
    nsIView   *kid = GetChild(cnt);
    nsIWidget *win = kid->GetWidget();

    if (nsnull != win)
    {
      nsRect  bounds;

      win->GetBounds(bounds);
      win->Move(bounds.x + aDx, bounds.y + aDy);

      NS_RELEASE(win);
    }

    kid->AdjustChildWidgets(aDx, aDy);
  }
}
