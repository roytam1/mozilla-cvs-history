/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: NPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is 
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 * Contributor(s):  Patrick C. Beard <beard@netscape.com>
 *                  Kevin McCluskey  <kmcclusk@netscape.com>
 *                  Robert O'Callahan <roc+@cs.cmu.edu>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or 
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the NPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "nsViewManager.h"
#include "nsUnitConversion.h"
#include "nsIRenderingContext.h"
#include "nsIDeviceContext.h"
#include "nsGfxCIID.h"
#include "nsIScrollableView.h"
#include "nsView.h"
#include "nsIScrollbar.h"
#include "nsISupportsArray.h"
#include "nsICompositeListener.h"
#include "nsCOMPtr.h"
#include "nsIEventQueue.h"
#include "nsIEventQueueService.h"
#include "nsIServiceManager.h"
#include "nsGUIEvent.h"
#include "nsIPref.h"

static NS_DEFINE_IID(kBlenderCID, NS_BLENDER_CID);
static NS_DEFINE_IID(kRegionCID, NS_REGION_CID);
static NS_DEFINE_IID(kRenderingContextCID, NS_RENDERING_CONTEXT_CID);
static NS_DEFINE_CID(kEventQueueServiceCID, NS_EVENTQUEUESERVICE_CID);

/**
   XXX TODO XXX

   DeCOMify newly private methods
   Move event handling into nsViewManager
   Make event handling use CreateDisplayList
   Reverse storage order of views so that LAST view in document order is the LAST child
     of its parent view
   Audit users of nsIView::GetPosition and nsIView::GetBounds, then 
     fix nsContainerFrame::SyncFrameViewAfterReflow to size views to contain
     left-or-above content
   Remove nsIClipView stuff and just use the CLIPCHILDREN flag
   Put in support for hierarchy of viewmanagers (handle nsViewManager::SetRootView
     case where aWidget == null and aView has a non-null parent with a different view
     manager)
   Fix opacity model to conform to SVG (requires backbuffer stack)
   Optimize view storage
*/

/**
   A note about platform assumptions:

   We assume all native widgets are opaque.
   
   We assume that a widget is z-ordered on top of its parent.
   
   We do NOT assume anything about the relative z-ordering of sibling widgets. Even though
   we ask for a specific z-order, we don't assume that widget z-ordering actually works.
*/

//#define NO_DOUBLE_BUFFER

// if defined widget changes like moves and resizes are defered until and done
// all in one pass.
//#define CACHE_WIDGET_CHANGES

// display list flags
#define VIEW_RENDERED     0x00000001
#define PUSH_CLIP         0x00000002
#define POP_CLIP          0x00000004
#define VIEW_TRANSPARENT  0x00000008
#define VIEW_TRANSLUCENT  0x00000010
#define VIEW_CLIPPED      0x00000020

#define SUPPORT_TRANSLUCENT_VIEWS

// A DisplayListElement2 records the information needed to paint one view.
// Note that child views get their own DisplayListElement2s; painting a view
// paints that view's frame and all its child frames EXCEPT for the child frames
// that have their own views.
struct DisplayListElement2 {
  nsView*       mView;        
  nsRect        mBounds;      // coordinates relative to the view manager root
  nscoord       mAbsX, mAbsY; // coordinates relative to the view that we're Refreshing 
  PRUint32      mFlags;       // see above
  PRInt32       mZIndex;      // temporary used during z=index processing (see below)
};

/*
  IMPLEMENTING Z-INDEX

  Implementing z-index:auto and negative z-indices properly is hard. Here's how we do it.

  In CreateDisplayList, the display list elements above are inserted into a tree rather
  than a simple list. The tree structure mimics the view hierarchy (except for fixed-position
  stuff, see below for more about that), except that in the tree, only leaf nodes can do
  actual painting (i.e., correspond to display list elements). Therefore every leaf view
  (i.e., no child views) has one corresponding leaf tree node containing its
  DisplayListElement2. OTOH, every non-leaf view corresponds to an interior tree node
  which contains the tree nodes for the child views, and which also contains a leaf tree
  node which does the painting for the non-leaf view. Initially sibling tree nodes are
  ordered in the same order as their views, which Layout should have arranged in document
  order. (Actually CreateDisplayList ensures that the tree is created to have elements in the
  order "<children-with-negative-z-index> <this-view> <children-with-nonnegative-z-index>".
  This makes sure that this-view gets processed in the right place.)

  For example, given the view hierarchy and z-indices, and assuming lower-
  numbered views precede higher-numbered views in the document order,
    V0(auto) --> V1(0) --> V2(auto) --> V3(0)
     |            |         +---------> V4(2)
     |            +------> V5(1)
     +---------> V6(1)
  CreateDisplayList would create the following z-tree (z-order increases from top to bottom)
    Ta(V0) --> Tb*(V0)
     +-------> Tc(V1) --> Td*(V1)
     |          +-------> Te(V2) --> Tf*(V2)
     |          |          +-------> Tg*(V3)
     |          |          +-------> Th*(V4)
     |          +-------> Ti*(V5)
     +-------> Tj*(V6)
  (* indicates leaf nodes marked with display list elements)

  Once the Z-tree has been built we call SortByZOrder to compute a real linear display list.
  It recursively computes a display list for each tree node, by computing the display lists
  for all child nodes, then concatenating those lists and sorting them by z-index. The trick
  is that the z-indices for display list elements are updated during the process; after
  a display list is calculated for a tree node, the elements of the display list are all
  assigned the z-index specified for the tree node's view (unless the view has z-index
  'auto'). This ensures that a tree node's display list elements will be sorted correctly
  relative to the siblings of the tree node.

  The above example is processed as follows:
  The child nodes of Te(V2) are display list elements [ Tf*(V2)(0), Tg*(V3)(0), Th*(V4)(2) ].
  (The display list element for the frames of a non-leaf view always has z-index 0 relative
  to the children of the view.)
  Te(V2) is 'auto' so its display list is [ Tf*(V2)(0), Tg*(V3)(0), Th*(V4)(2) ].
  Tc(V1)'s child display list elements are [ Td*(V1)(0), Tf*(V2)(0), Tg*(V3)(0), Th*(V4)(2),
  Ti*(V5)(1) ].
  The nodes are sorted and then reassigned z-index 0, so Tc(V1) is replaced with the list
  [ Td*(V1)(0), Tf*(V2)(0), Tg*(V3)(0), Ti*(V5)(0), Th*(V4)(0) ].
  Finally we collect the elements for Ta(V0):
  [ Tb*(V0), Td*(V1)(0), Tf*(V2)(0), Tg*(V3)(0), Ti*(V5)(0), Th*(V4)(0), Tj*(V6)(1) ].
*/

struct DisplayZTreeNode {
  nsView*              mView;            // Null for tree leaf nodes
  DisplayZTreeNode*    mZSibling;

  // We can't have BOTH an mZChild and an mDisplayElement
  DisplayZTreeNode*    mZChild;          // tree interior nodes
  DisplayListElement2* mDisplayElement;  // tree leaf nodes
};

void nsViewManager::DestroyZTreeNode(DisplayZTreeNode* aNode) 
{
  if (aNode) {
    if (mMapPlaceholderViewToZTreeNode.Count() > 0) {
       nsVoidKey key(aNode->mView);
       mMapPlaceholderViewToZTreeNode.Remove(&key);
    }
  
    DestroyZTreeNode(aNode->mZChild);
    DestroyZTreeNode(aNode->mZSibling);
    delete aNode->mDisplayElement;
    delete aNode;
  }
}


#ifdef NS_VM_PERF_METRICS
#include "nsITimeRecorder.h"
#endif

//-------------- Begin Invalidate Event Definition ------------------------

struct nsInvalidateEvent : public PLEvent {
  nsInvalidateEvent(nsViewManager* aViewManager);
  ~nsInvalidateEvent() { }

  void HandleEvent() {  
    NS_ASSERTION(nsnull != mViewManager,"ViewManager is null");
    // Search for valid view manager before trying to access it
    // This is just a safety check. We should never have a circumstance
    // where the view manager has been destroyed and the invalidate event
    // which it owns is still around. The invalidate event should be destroyed
    // by the RevokeEvent in the viewmanager's destructor.
    PRBool found = PR_FALSE;
    PRInt32 index;
    PRInt32 count = nsViewManager::GetViewManagerCount();
    const nsVoidArray* viewManagers = nsViewManager::GetViewManagerArray();
    for (index = 0; index < count; index++) {
      nsViewManager* vm = (nsViewManager*)viewManagers->ElementAt(index);
      if (vm == mViewManager) {
        found = PR_TRUE;
      }
    }

    if (found) {
      mViewManager->ProcessInvalidateEvent();
    } else {
      NS_ASSERTION(PR_FALSE, "bad view manager asked to process invalidate event");
    }
  };
 
  nsViewManager* mViewManager; // Weak Reference. The viewmanager will destroy any pending
  // invalidate events in it's destructor.
};

static void PR_CALLBACK HandlePLEvent(nsInvalidateEvent* aEvent)
{
  NS_ASSERTION(nsnull != aEvent,"Event is null");
  aEvent->HandleEvent();
}

static void PR_CALLBACK DestroyPLEvent(nsInvalidateEvent* aEvent)
{
  NS_ASSERTION(nsnull != aEvent,"Event is null");
  delete aEvent;
}

nsInvalidateEvent::nsInvalidateEvent(nsViewManager* aViewManager)
{
  NS_ASSERTION(aViewManager, "null parameter");  
  mViewManager = aViewManager; // Assign weak reference
  PL_InitEvent(this, aViewManager,
               (PLHandleEventProc) ::HandlePLEvent,
               (PLDestroyEventProc) ::DestroyPLEvent);  
}

//-------------- End Invalidate Event Definition ---------------------------


void
nsViewManager::PostInvalidateEvent()
{
  if (!mPendingInvalidateEvent) {
    nsInvalidateEvent* ev = new nsInvalidateEvent(this);
    NS_ASSERTION(nsnull != ev,"InvalidateEvent is null");
    NS_ASSERTION(nsnull != mEventQueue,"Event queue is null");
    mEventQueue->PostEvent(ev);
    mPendingInvalidateEvent = PR_TRUE;  
  }
}

PRInt32 nsViewManager::mVMCount = 0;
nsIRenderingContext* nsViewManager::gCleanupContext = nsnull;
nsDrawingSurface nsViewManager::gOffScreen = nsnull;
nsDrawingSurface nsViewManager::gBlack = nsnull;
nsDrawingSurface nsViewManager::gWhite = nsnull;
nsSize nsViewManager::gOffScreenSize = nsSize(0, 0);

// Weakly held references to all of the view managers
nsVoidArray* nsViewManager::gViewManagers = nsnull;
PRUint32 nsViewManager::gLastUserEventTime = 0;

nsViewManager::nsViewManager()
{
  NS_INIT_REFCNT();

  if (gViewManagers == nsnull) {
    NS_ASSERTION(mVMCount == 0, "View Manager count is incorrect");
    // Create an array to hold a list of view managers
    gViewManagers = new nsVoidArray;
  }
 
  if (gCleanupContext == nsnull) {
    nsComponentManager::CreateInstance(kRenderingContextCID, 
                                       nsnull, NS_GET_IID(nsIRenderingContext), (void**)&gCleanupContext);
    NS_ASSERTION(gCleanupContext != nsnull, "Wasn't able to create a graphics context for cleanup");
  }

  gViewManagers->AppendElement(this);

  mVMCount++;
  // NOTE:  we use a zeroing operator new, so all data members are
  // assumed to be cleared here.
  mX = 0;
  mY = 0;
  mCachingWidgetChanges = 0;
  mDefaultBackgroundColor = NS_RGBA(0, 0, 0, 0);
  mAllowDoubleBuffering = PR_TRUE; 
  mHasPendingInvalidates = PR_FALSE;
  mPendingInvalidateEvent = PR_FALSE;
  mRecursiveRefreshPending = PR_FALSE;
}

nsViewManager::~nsViewManager()
{
  // Revoke pending invalidate events
  if (mPendingInvalidateEvent) {
    NS_ASSERTION(nsnull != mEventQueue,"Event queue is null"); 
    mPendingInvalidateEvent = PR_FALSE;
    mEventQueue->RevokeEvents(this);
  }

  NS_IF_RELEASE(mRootWindow);

  mRootScrollable = nsnull;

  NS_ASSERTION((mVMCount > 0), "underflow of viewmanagers");
  --mVMCount;

  PRBool removed = gViewManagers->RemoveElement(this);
  NS_ASSERTION(removed, "Viewmanager instance not was not in the global list of viewmanagers");

  if (0 == mVMCount) {
    // There aren't any more view managers so
    // release the global array of view managers
   
    NS_ASSERTION(gViewManagers != nsnull, "About to delete null gViewManagers");
    delete gViewManagers;
    gViewManagers = nsnull;

    // Cleanup all of the offscreen drawing surfaces if the last view manager
    // has been destroyed and there is something to cleanup

    // Note: A global rendering context is needed because it is not possible 
    // to create a nsIRenderingContext during the shutdown of XPCOM. The last
    // viewmanager is typically destroyed during XPCOM shutdown.

    if (gCleanupContext) {

      gCleanupContext->DestroyCachedBackbuffer();

      if (nsnull != gOffScreen)
        gCleanupContext->DestroyDrawingSurface(gOffScreen);

      if (nsnull != gWhite)
        gCleanupContext->DestroyDrawingSurface(gWhite);

      if (nsnull != gBlack)
        gCleanupContext->DestroyDrawingSurface(gBlack);

    } else {
      NS_ASSERTION(PR_FALSE, "Cleanup of drawing surfaces + offscreen buffer failed");
    }

    gOffScreen = nsnull;
    gWhite = nsnull;
    gBlack = nsnull;
    gOffScreenSize.SizeTo(0, 0);

    NS_IF_RELEASE(gCleanupContext);
  }

  mObserver = nsnull;
  mContext = nsnull;

  NS_IF_RELEASE(mBlender);
  NS_IF_RELEASE(mOpaqueRgn);
  NS_IF_RELEASE(mTmpRgn);

  NS_IF_RELEASE(mOffScreenCX);
  NS_IF_RELEASE(mBlackCX);
  NS_IF_RELEASE(mWhiteCX);

  if (nsnull != mCompositeListeners) {
    mCompositeListeners->Clear();
    NS_RELEASE(mCompositeListeners);
  }
}

NS_IMPL_QUERY_INTERFACE1(nsViewManager, nsIViewManager)

  NS_IMPL_ADDREF(nsViewManager);

nsrefcnt nsViewManager::Release(void)
{
  /* Note funny ordering of use of mRefCnt. We were seeing a problem
     during the deletion of a view hierarchy where child views,
     while being destroyed, referenced this view manager and caused
     the Destroy part of this method to be re-entered. Waiting until
     the destruction has finished to decrement the refcount
     prevents that.
  */
  NS_LOG_RELEASE(this, mRefCnt - 1, "nsViewManager");
  if (mRefCnt == 1)
    {
      if (nsnull != mRootView) {
        // Destroy any remaining views
        mRootView->Destroy();
        mRootView = nsnull;
      }
      delete this;
      return 0;
    }
  mRefCnt--;
  return mRefCnt;
}

nsresult 
nsViewManager::CreateRegion(nsIRegion* *result)
{
  nsresult rv;
  
  if (!mRegionFactory) {
    nsCOMPtr<nsIComponentManager> compMgr;
    rv = NS_GetComponentManager(getter_AddRefs(compMgr));
        
    if (NS_SUCCEEDED(rv))
      rv = compMgr->GetClassObject(kRegionCID, 
                                   NS_GET_IID(nsIFactory), 
                                   getter_AddRefs(mRegionFactory));
    
    if (!mRegionFactory) {
      *result = nsnull;
      return NS_ERROR_FAILURE;
        }
  }
  
  
  nsIRegion* region = nsnull;
  rv = mRegionFactory->CreateInstance(nsnull, NS_GET_IID(nsIRegion), (void**)&region);
  if (NS_SUCCEEDED(rv)) {
    rv = region->Init();
    *result = region;
  }
  return rv;
}

// We don't hold a reference to the presentation context because it
// holds a reference to us.
NS_IMETHODIMP nsViewManager::Init(nsIDeviceContext* aContext)
{
  nsresult rv;

  NS_PRECONDITION(nsnull != aContext, "null ptr");

  if (nsnull == aContext) {
    return NS_ERROR_NULL_POINTER;
  }
  if (nsnull != mContext) {
    return NS_ERROR_ALREADY_INITIALIZED;
  }
  mContext = aContext;
  mContext->GetAppUnitsToDevUnits(mTwipsToPixels);
  mContext->GetDevUnitsToAppUnits(mPixelsToTwips);

  mTransCnt = 0;

  mLastRefresh = PR_IntervalNow();

  mRefreshEnabled = PR_TRUE;

  mMouseGrabber = nsnull;
  mKeyGrabber = nsnull;

  // create regions
  mOpaqueRgn = nsnull;
  mTmpRgn = nsnull;
  
  CreateRegion(&mOpaqueRgn);
  CreateRegion(&mTmpRgn);
  
  if (nsnull == mEventQueue) {
    // Cache the event queue of the current UI thread
    nsCOMPtr<nsIEventQueueService> eventService = 
      do_GetService(kEventQueueServiceCID, &rv);
    if (NS_SUCCEEDED(rv) && (nsnull != eventService)) {                  // XXX this implies that the UI is the current thread.
      rv = eventService->GetThreadEventQueue(NS_CURRENT_THREAD, getter_AddRefs(mEventQueue));
    }

    NS_ASSERTION(nsnull != mEventQueue, "event queue is null");
  }
  
  return rv;
}

NS_IMETHODIMP nsViewManager::GetRootView(nsIView *&aView)
{
  aView = mRootView;
  return NS_OK;
}

NS_IMETHODIMP nsViewManager::SetRootView(nsIView *aView, nsIWidget* aWidget)
{
  nsView* view = NS_STATIC_CAST(nsView*, aView);

  UpdateTransCnt(mRootView, view);
  // Do NOT destroy the current root view. It's the caller's responsibility
  // to destroy it
  mRootView = view;

  //now get the window too.
  NS_IF_RELEASE(mRootWindow);

  // The window must be specified through one of the following:
  //* a) The aView has a nsIWidget instance or
  //* b) the aWidget parameter is an nsIWidget instance to render into 
  //*    that is not owned by a view.
  //* c) aView has a parent view managed by a different view manager or

  if (nsnull != aWidget) {
    mRootWindow = aWidget;
    NS_ADDREF(mRootWindow);
    return NS_OK;
  }

  // case b) The aView has a nsIWidget instance
  if (nsnull != mRootView) {
    mRootView->GetWidget(mRootWindow);
    if (nsnull != mRootWindow) {
      return NS_OK;
    }
  }

  // case c)  aView has a parent view managed by a different view manager

  return NS_OK;
}

NS_IMETHODIMP nsViewManager::GetWindowOffset(nscoord *aX, nscoord *aY)
{
  NS_ASSERTION(aX != nsnull, "aX pointer is null");
  NS_ASSERTION(aY != nsnull, "aY pointer is null");

  *aX = mX;
  *aY = mY;
  return NS_OK;
}

NS_IMETHODIMP nsViewManager::SetWindowOffset(nscoord aX, nscoord aY)
{
  mX = aX;
  mY = aY;
  return NS_OK;
}

NS_IMETHODIMP nsViewManager::GetWindowDimensions(nscoord *aWidth, nscoord *aHeight)
{
  if (nsnull != mRootView) {
    nsRect dim;
    mRootView->GetDimensions(dim);
    *aWidth = dim.width;
    *aHeight = dim.height;
  }
  else
    {
      *aWidth = 0;
      *aHeight = 0;
    }
  return NS_OK;
}

NS_IMETHODIMP nsViewManager::SetWindowDimensions(nscoord aWidth, nscoord aHeight)
{
  // Resize the root view
  if (nsnull != mRootView) {
    nsRect dim(0, 0, aWidth, aHeight);
    mRootView->SetDimensions(dim);
  }

  //printf("new dims: %d %d\n", aWidth, aHeight);
  // Inform the presentation shell that we've been resized
  if (nsnull != mObserver)
    mObserver->ResizeReflow(mRootView, aWidth, aHeight);
  //printf("reflow done\n");

  return NS_OK;
}

NS_IMETHODIMP nsViewManager::ResetScrolling(void)
{
  if (nsnull != mRootScrollable)
    mRootScrollable->ComputeScrollOffsets(PR_TRUE);

  return NS_OK;
}

/**
   aRegion is given in device coordinates!!
*/
void nsViewManager::Refresh(nsView *aView, nsIRenderingContext *aContext, nsIRegion *aRegion, PRUint32 aUpdateFlags)
{
  nsCOMPtr<nsIRenderingContext> localcx;
  nsDrawingSurface    ds = nsnull;

  NS_ASSERTION(aRegion != nsnull, "Null aRegion");

  if (PR_FALSE == mRefreshEnabled)
    return;

#ifdef NS_VM_PERF_METRICS
  MOZ_TIMER_DEBUGLOG(("Reset nsViewManager::Refresh(region), this=%p\n", this));
  MOZ_TIMER_RESET(mWatch);

  MOZ_TIMER_DEBUGLOG(("Start: nsViewManager::Refresh(region)\n"));
  MOZ_TIMER_START(mWatch);
#endif

  NS_ASSERTION(!(PR_TRUE == mPainting), "recursive painting not permitted");
  if (mPainting) {
    mRecursiveRefreshPending = PR_TRUE;
    return;
  }  

  mPainting = PR_TRUE;

  //printf("refreshing region...\n");
  //force double buffering because of non-opaque views?

  if (mTransCnt > 0)
    aUpdateFlags |= NS_VMREFRESH_DOUBLE_BUFFER;

#ifdef NO_DOUBLE_BUFFER
  aUpdateFlags &= ~NS_VMREFRESH_DOUBLE_BUFFER;
#endif

  if (PR_FALSE == mAllowDoubleBuffering) {
    // Turn off double-buffering of the display
    aUpdateFlags &= ~NS_VMREFRESH_DOUBLE_BUFFER;
  }

  if (nsnull == aContext)
    {
      localcx = getter_AddRefs(CreateRenderingContext(*aView));

      //couldn't get rendering context. this is ok at init time atleast
      if (nsnull == localcx) {
        mPainting = PR_FALSE;
        return;
      }
    } else {
      // plain assignment grabs another reference.
      localcx = aContext;
    }

  // notify the listeners.
  if (nsnull != mCompositeListeners) {
    PRUint32 listenerCount;
    if (NS_SUCCEEDED(mCompositeListeners->Count(&listenerCount))) {
      nsCOMPtr<nsICompositeListener> listener;
      for (PRUint32 i = 0; i < listenerCount; i++) {
        if (NS_SUCCEEDED(mCompositeListeners->QueryElementAt(i, NS_GET_IID(nsICompositeListener), getter_AddRefs(listener)))) {
          listener->WillRefreshRegion(this, aView, aContext, aRegion, aUpdateFlags);
        }
      }
    }
  }

  nsRect damageRectInPixels;
  aRegion->GetBoundingBox(&damageRectInPixels.x, &damageRectInPixels.y, &damageRectInPixels.width, &damageRectInPixels.height);

  if (aUpdateFlags & NS_VMREFRESH_DOUBLE_BUFFER)
  {
    nsRect maxWidgetSize;
    GetMaxWidgetBounds(maxWidgetSize);
    if NS_FAILED(localcx->GetBackbuffer(nsRect(0, 0, damageRectInPixels.width, damageRectInPixels.height), maxWidgetSize, ds)) {
      //Failed to get backbuffer so turn off double buffering
      aUpdateFlags &= ~NS_VMREFRESH_DOUBLE_BUFFER;
    }
  }

  nsRect viewRect;
  aView->GetDimensions(viewRect);

  nsRect damageRect = damageRectInPixels;
  nsRect paintRect;
  float  p2t;
  mContext->GetDevUnitsToAppUnits(p2t);
  damageRect.ScaleRoundOut(p2t);

  // move the view rect into widget coordinates
  viewRect.x = 0;
  viewRect.y = 0;

  if (paintRect.IntersectRect(damageRect, viewRect)) {

    if ((aUpdateFlags & NS_VMREFRESH_DOUBLE_BUFFER) && ds) {  
      // backbuffer's (0,0) is mapped to damageRect.x, damageRect.y
      localcx->Translate(-damageRect.x, -damageRect.y);
      aRegion->Offset(-damageRectInPixels.x, -damageRectInPixels.y);
    }

    PRBool result;
    // Note that nsIRenderingContext::SetClipRegion always works in pixel coordinates,
    // and nsIRenderingContext::SetClipRect always works in app coordinates. Stupid huh?
    localcx->SetClipRegion(*aRegion, nsClipCombine_kReplace, result);
    localcx->SetClipRect(paintRect, nsClipCombine_kIntersect, result);

    // pass in a damage rectangle in aView's coordinates.
    nsRect r = paintRect;
    nsRect dims;
    aView->GetDimensions(dims);
    r.x += dims.x;
    r.y += dims.y;

    // painting will be done in aView's coordinates, so shift them back to widget coordinates
    localcx->Translate(-dims.x, -dims.y);
    RenderViews(aView, *localcx, r, result);
    localcx->Translate(dims.x, dims.y);

    if ((aUpdateFlags & NS_VMREFRESH_DOUBLE_BUFFER) && ds) {
      // Setup the region relative to the destination's coordinates 
      aRegion->Offset(damageRectInPixels.x, damageRectInPixels.y);
      localcx->SetClipRegion(*aRegion, nsClipCombine_kReplace, result);
      localcx->Translate(damageRect.x, damageRect.y);
      localcx->SetClipRect(paintRect, nsClipCombine_kIntersect, result);
      localcx->CopyOffScreenBits(ds, 0, 0, damageRectInPixels, NS_COPYBITS_USE_SOURCE_CLIP_REGION);
    }
  } else {
#ifdef DEBUG
    printf("XXX Damage rectangle (%d,%d,%d,%d) does not intersect the widget's view (%d,%d,%d,%d)!\n",
           damageRect.x, damageRect.y, damageRect.width, damageRect.height,
           viewRect.x, viewRect.y, viewRect.width, viewRect.height);
#endif
  }

  mLastRefresh = PR_IntervalNow();

  mPainting = PR_FALSE;

  // notify the listeners.
  if (nsnull != mCompositeListeners) {
    PRUint32 listenerCount;
    if (NS_SUCCEEDED(mCompositeListeners->Count(&listenerCount))) {
      nsCOMPtr<nsICompositeListener> listener;
      for (PRUint32 i = 0; i < listenerCount; i++) {
        if (NS_SUCCEEDED(mCompositeListeners->QueryElementAt(i, NS_GET_IID(nsICompositeListener), getter_AddRefs(listener)))) {
          listener->DidRefreshRegion(this, aView, aContext, aRegion, aUpdateFlags);
        }
      }
    }
  }

  if (mRecursiveRefreshPending) {
    UpdateAllViews(aUpdateFlags);
    mRecursiveRefreshPending = PR_FALSE;
  }

  localcx->ReleaseBackbuffer();

#ifdef NS_VM_PERF_METRICS
  MOZ_TIMER_DEBUGLOG(("Stop: nsViewManager::Refresh(region), this=%p\n", this));
  MOZ_TIMER_STOP(mWatch);
  MOZ_TIMER_LOG(("vm2 Paint time (this=%p): ", this));
  MOZ_TIMER_PRINT(mWatch);
#endif

}

void nsViewManager::DefaultRefresh(nsView* aView, const nsRect* aRect)
{
  nsCOMPtr<nsIWidget> widget;
  GetWidgetForView(aView, getter_AddRefs(widget));
  if (! widget)
    return;

  nsCOMPtr<nsIRenderingContext> context
    = getter_AddRefs(CreateRenderingContext(*aView));

  if (! context)
    return;

  nscolor bgcolor = mDefaultBackgroundColor;

  if (NS_GET_A(mDefaultBackgroundColor) == 0) {
    NS_WARNING("nsViewManager: Asked to paint a default background, but no default background color is set!");
    return;
  }

  context->SetColor(bgcolor);
  context->FillRect(*aRect);
}

// Perform a *stable* sort of the buffer by increasing Z-index. The common case is
// when many or all z-indices are equal and the list is mostly sorted; make sure
// that's fast (should be linear time if all z-indices are equal).
static void ApplyZOrderStableSort(nsVoidArray &aBuffer, nsVoidArray &aMergeTmp, PRInt32 aStart, PRInt32 aEnd) {
  if (aEnd - aStart <= 6) {
    // do a fast bubble sort for the small sizes
    for (PRInt32 i = aEnd - 1; i > aStart; i--) {
      PRBool sorted = PR_TRUE;
      for (PRInt32 j = aStart; j < i; j++) {
        DisplayListElement2* e1 = NS_STATIC_CAST(DisplayListElement2*, aBuffer.ElementAt(j));
        DisplayListElement2* e2 = NS_STATIC_CAST(DisplayListElement2*, aBuffer.ElementAt(j + 1));
        if (e1->mZIndex > e2->mZIndex) {
          sorted = PR_FALSE;
          // We could use aBuffer.MoveElement(), but it wouldn't be much of
          // a win if any for swapping two elements.
          aBuffer.ReplaceElementAt(e2, j);
          aBuffer.ReplaceElementAt(e1, j + 1);
        }
      }
      if (sorted) {
        return;
      }
    }
  } else {
    // merge sort for the rest
    PRInt32 mid = (aEnd + aStart)/2;

    ApplyZOrderStableSort(aBuffer, aMergeTmp, aStart, mid);
    ApplyZOrderStableSort(aBuffer, aMergeTmp, mid, aEnd);

    DisplayListElement2* e1 = NS_STATIC_CAST(DisplayListElement2*, aBuffer.ElementAt(mid - 1));
    DisplayListElement2* e2 = NS_STATIC_CAST(DisplayListElement2*, aBuffer.ElementAt(mid));

    // fast common case: the list is already completely sorted
    if (e1->mZIndex <= e2->mZIndex) {
      return;
    }
    // we have some merging to do.

    PRInt32 i1 = aStart;
    PRInt32 i2 = mid;

    e1 = NS_STATIC_CAST(DisplayListElement2*, aBuffer.ElementAt(i1));
    e2 = NS_STATIC_CAST(DisplayListElement2*, aBuffer.ElementAt(i2));
    while (i1 < mid || i2 < aEnd) {
      if (i1 < mid && (i2 == aEnd || e1->mZIndex <= e2->mZIndex)) {
        aMergeTmp.AppendElement(e1);
        i1++;
        if (i1 < mid) {
          e1 = NS_STATIC_CAST(DisplayListElement2*, aBuffer.ElementAt(i1));
        }
      } else {
        aMergeTmp.AppendElement(e2);
        i2++;
        if (i2 < aEnd) {
          e2 = NS_STATIC_CAST(DisplayListElement2*, aBuffer.ElementAt(i2));
        }
      }
    }

    for (PRInt32 i = aStart; i < aEnd; i++) {
      aBuffer.ReplaceElementAt(aMergeTmp.ElementAt(i - aStart), i);
    }

    aMergeTmp.Clear();
  }
}

// The display-element (indirect) children of aNode are extracted and appended to aBuffer in
// z-order, with the bottom-most elements first.
// Their z-index is set to the z-index they will have in aNode's parent.
// I.e. if aNode's view has "z-index: auto", the nodes will keep their z-index, otherwise
// their z-indices will all be equal to the z-index value of aNode's view.
static void SortByZOrder(DisplayZTreeNode *aNode, nsVoidArray &aBuffer, nsVoidArray &aMergeTmp, PRBool aForceSort) {
  PRBool autoZIndex = PR_TRUE;
  PRInt32 explicitZIndex = 0;

  if (nsnull != aNode->mView) {
    autoZIndex = aNode->mView->GetZIndexIsAuto();
    if (!autoZIndex) {
      explicitZIndex = aNode->mView->GetZIndex();
    }
  }

  if (nsnull == aNode->mZChild) {
    if (nsnull != aNode->mDisplayElement) {
      aBuffer.AppendElement(aNode->mDisplayElement);
      aNode->mDisplayElement->mZIndex = explicitZIndex;
      aNode->mDisplayElement = nsnull;
    }
    return;
  }

  DisplayZTreeNode *child;
  PRInt32 childStartIndex = aBuffer.Count();
  for (child = aNode->mZChild; nsnull != child; child = child->mZSibling) {
    SortByZOrder(child, aBuffer, aMergeTmp, PR_FALSE);
  }
  PRInt32 childEndIndex = aBuffer.Count();
  PRBool hasClip = PR_FALSE;
  
  if (childEndIndex - childStartIndex >= 2) {
    DisplayListElement2* firstChild = NS_STATIC_CAST(DisplayListElement2*, aBuffer.ElementAt(childStartIndex));
    if (0 != (firstChild->mFlags & PUSH_CLIP) && firstChild->mView == aNode->mView) {
      hasClip = PR_TRUE;
    }
  }

  if (hasClip) {
    ApplyZOrderStableSort(aBuffer, aMergeTmp, childStartIndex + 1, childEndIndex - 1);
    
    if (autoZIndex && childEndIndex - childStartIndex >= 3) {
      // If we're an auto-z-index, then we have to worry about the possibility that some of
      // our children may be moved by the z-sorter beyond the bounds of the PUSH...POP clip
      // instructions. So basically, we ensure that around every group of children of
      // equal z-index, there is a PUSH...POP element pair with the same z-index. The stable
      // z-sorter will not break up such a group.
      // Note that if we're not an auto-z-index set, then our children will never be broken
      // up so we don't need to do this.
      // We also don't have to worry if we have no real children.
      DisplayListElement2* ePush = NS_STATIC_CAST(DisplayListElement2*, aBuffer.ElementAt(childStartIndex));
      DisplayListElement2* eFirstChild = NS_STATIC_CAST(DisplayListElement2*, aBuffer.ElementAt(childStartIndex + 1));

      ePush->mZIndex = eFirstChild->mZIndex;

      DisplayListElement2* ePop = NS_STATIC_CAST(DisplayListElement2*, aBuffer.ElementAt(childEndIndex - 1));
      DisplayListElement2* eLastChild = NS_STATIC_CAST(DisplayListElement2*, aBuffer.ElementAt(childEndIndex - 2));

      ePop->mZIndex = eLastChild->mZIndex;

      DisplayListElement2* e = eFirstChild;
      for (PRInt32 i = childStartIndex + 1; i < childEndIndex - 2; i++) {
        DisplayListElement2* eNext = NS_STATIC_CAST(DisplayListElement2*, aBuffer.ElementAt(i + 1));
        NS_ASSERTION(e->mZIndex <= eNext->mZIndex, "Display Z-list is not sorted!!");
        if (e->mZIndex != eNext->mZIndex) {
          // need to insert a POP for the last sequence and a PUSH for the next sequence
          DisplayListElement2* newPop = new DisplayListElement2;
          DisplayListElement2* newPush = new DisplayListElement2;

          *newPop = *ePop;
          newPop->mZIndex = e->mZIndex;
          *newPush = *ePush;
          newPush->mZIndex = eNext->mZIndex;
          aBuffer.InsertElementAt(newPop, i + 1);
          aBuffer.InsertElementAt(newPush, i + 2);
          i += 2;
          childEndIndex += 2;
        }
        e = eNext;
      }
    }
  } else if (aForceSort || !autoZIndex) {
    ApplyZOrderStableSort(aBuffer, aMergeTmp, childStartIndex, childEndIndex);
  }

  if (!autoZIndex) {
    for (PRInt32 i = childStartIndex; i < childEndIndex; i++) {
      DisplayListElement2* element = NS_STATIC_CAST(DisplayListElement2*, aBuffer.ElementAt(i));
      element->mZIndex = explicitZIndex;
    }
  }
}

static void PushStateAndClip(nsIRenderingContext **aRCs, PRInt32 aRCCount, nsRect &aRect,
                             PRInt32 aDX, PRInt32 aDY) {
  PRBool clipEmpty;
  nsRect rect = aRect;
  for (PRInt32 i = 0; i < aRCCount; i++) {
    aRCs[i]->PushState();
    if (i == 1) {
      rect.x -= aDX;
      rect.y -= aDY;
    }
    aRCs[i]->SetClipRect(rect, nsClipCombine_kIntersect, clipEmpty);
  }
}

static void PopState(nsIRenderingContext **aRCs, PRInt32 aRCCount) {
  PRBool clipEmpty;
  for (PRInt32 i = 0; i < aRCCount; i++) {
    aRCs[i]->PopState(clipEmpty);
  }
}

void nsViewManager::AddCoveringWidgetsToOpaqueRegion(nsIRegion* aRgn, nsIDeviceContext* aContext,
                                                     nsView* aRootView) {
  // We accumulate the bounds of widgets obscuring aRootView's widget into mOpaqueRgn.
  // In OptimizeDisplayList, display list elements which lie behind obscuring native
  // widgets are dropped.
  // This shouldn't really be necessary, since the GFX/Widget toolkit should remove these
  // covering widgets from the clip region passed into the paint command. But right now
  // they only give us a paint rect and not a region, so we can't access that information.
  // It's important to identifying areas that are covered by native widgets to avoid
  // painting their views many times as we process invalidates from the root widget all the
  // way down to the nested widgets.
  // 
  // NB: we must NOT add widgets that correspond to floating views!
  // We may be required to paint behind them
  if (aRgn) {
    aRgn->SetTo(0, 0, 0, 0);
    nsCOMPtr<nsIWidget> widget;
    GetWidgetForView(aRootView, getter_AddRefs(widget));
    if (widget) {
      nsCOMPtr<nsIEnumerator> children(dont_AddRef(widget->GetChildren()));
      if (children) {
        children->First();
        do {
          nsCOMPtr<nsISupports> child;
          if (NS_SUCCEEDED(children->CurrentItem(getter_AddRefs(child)))) {
            nsCOMPtr<nsIWidget> childWidget = do_QueryInterface(child);
            if (childWidget) {
              nsView* view = nsView::GetViewFor(childWidget);
              if (view) {
                nsViewVisibility visible = nsViewVisibility_kHide;
                view->GetVisibility(visible);
                if (visible == nsViewVisibility_kShow) {
                  PRBool floating = PR_FALSE;
                  view->GetFloating(floating);
                  if (!floating) {
                    nsRect bounds;
                    view->GetBounds(bounds);
                    if (bounds.width > 0 && bounds.height > 0) {
                      nsView* viewParent = view->GetParent();

                      while (viewParent && viewParent != aRootView) {
                        viewParent->ConvertToParentCoords(&bounds.x, &bounds.y);
                        viewParent = viewParent->GetParent();
                      }

                      // maybe we couldn't get the view into the coordinate
                      // system of aRootView (maybe it's not a descendant
                      // view of aRootView?); if so, don't use it
                      if (viewParent) {
                        aRgn->Union(bounds.x, bounds.y, bounds.width, bounds.height);
                      }
                    }
                  }
                }
              }
            }
          }
        } while (NS_SUCCEEDED(children->Next()));
      }
    }
  }
}

void nsViewManager::RenderViews(nsView *aRootView, nsIRenderingContext& aRC, const nsRect& aRect, PRBool &aResult)
{
  BuildDisplayList(aRootView, aRect, PR_FALSE, PR_FALSE);
  nsRect fakeClipRect;
  PRInt32 index = 0;
  PRBool anyRendered;
  nsRect finalTransparentRect;

  ReapplyClipInstructions(PR_FALSE, fakeClipRect, index);
    
  OptimizeDisplayList(aRect, finalTransparentRect);

  if (!finalTransparentRect.IsEmpty()) {
    // There are some bits here that aren't going to be completely painted unless we do it now.
    // XXX Which color should we use for these bits?
    aRC.SetColor(NS_RGB(128, 128, 128));
    aRC.FillRect(finalTransparentRect);
#ifdef DEBUG_roc
    printf("XXX: Using final transparent rect, x=%d, y=%d, width=%d, height=%d\n",
           finalTransparentRect.x, finalTransparentRect.y, finalTransparentRect.width, finalTransparentRect.height);
#endif
  }
    
  // initialize various counters. These are updated in OptimizeDisplayListClipping.
  mTranslucentViewCount = 0;
  mTranslucentArea.SetRect(0, 0, 0, 0);
    
  index = 0;
  OptimizeDisplayListClipping(PR_FALSE, fakeClipRect, index, anyRendered);
    
  // We keep a list of all the rendering contexts whose clip rects
  // need to be updated.
  nsIRenderingContext* RCList[4];
  PRInt32 RCCount = 1;
  RCList[0] = &aRC;
    
  // create blending buffers, if necessary.
  if (mTranslucentViewCount > 0) {
    nsresult rv = CreateBlendingBuffers(aRC);
    NS_ASSERTION((rv == NS_OK), "not enough memory to blend");
    if (NS_FAILED(rv)) {
      // fall back by just rendering with transparency.
      mTranslucentViewCount = 0;
      for (PRInt32 i = mDisplayListCount - 1; i>= 0; --i) {
        DisplayListElement2* element = NS_STATIC_CAST(DisplayListElement2*, mDisplayList.ElementAt(i));
        element->mFlags &= ~VIEW_TRANSLUCENT;
      }
    } else {
      RCCount = 4;
      RCList[1] = mBlackCX;
      RCList[2] = mWhiteCX;
      RCList[3] = mOffScreenCX;
    }
      
    if (!finalTransparentRect.IsEmpty()) {
      // There are some bits that aren't going to be completely painted, so
      // make sure we don't leave garbage in the offscreen context 
      mOffScreenCX->SetColor(NS_RGB(128, 128, 128));
      mOffScreenCX->FillRect(nsRect(0, 0, gOffScreenSize.width, gOffScreenSize.height));
    }
    // DEBUGGING:  fill in complete offscreen image in green, to see if we've got a blending bug.
    //mOffScreenCX->SetColor(NS_RGB(0, 255, 0));
    //mOffScreenCX->FillRect(nsRect(0, 0, gOffScreenSize.width, gOffScreenSize.height));
  }

  // draw all views in the display list, from back to front.
  for (PRInt32 i = 0; i < mDisplayListCount; i++) {
    DisplayListElement2* element = NS_STATIC_CAST(DisplayListElement2*, mDisplayList.ElementAt(i));
    if (element->mFlags & VIEW_RENDERED) {
      // typical case, just rendering a view.
      // RenderView(element->mView, aRC, aRect, element->mBounds, aResult);
      if (element->mFlags & VIEW_CLIPPED) {
        //Render the view using the clip rect set by it's ancestors
        PushStateAndClip(RCList, RCCount, element->mBounds, mTranslucentArea.x, mTranslucentArea.y);
        RenderDisplayListElement(element, aRC);
        PopState(RCList, RCCount);
      } else {
        RenderDisplayListElement(element, aRC);
      }
        
    } else {
      // special case, pushing or popping clipping.
      if (element->mFlags & PUSH_CLIP) {
        PushStateAndClip(RCList, RCCount, element->mBounds, mTranslucentArea.x, mTranslucentArea.y);
      } else {
        if (element->mFlags & POP_CLIP) {
          PopState(RCList, RCCount);
        }
      }
    }
      
    delete element;
  }
    
  // flush bits back to screen.
  // Must flush back when no clipping is in effect.
  if (mTranslucentViewCount > 0) {
    // DEBUG: is this getting through?
    // mOffScreenCX->SetColor(NS_RGB(0, 0, 0));
    // mOffScreenCX->DrawRect(nsRect(0, 0, mTranslucentArea.width, mTranslucentArea.height));
    aRC.CopyOffScreenBits(gOffScreen, 0, 0, mTranslucentArea,
                          NS_COPYBITS_XFORM_DEST_VALUES |
                          NS_COPYBITS_TO_BACK_BUFFER);
    // DEBUG: what rectangle are we blitting?
    // aRC.SetColor(NS_RGB(0, 0, 0));
    // aRC.DrawRect(mTranslucentArea);
  }
    
  mDisplayList.Clear();
}

void nsViewManager::RenderDisplayListElement(DisplayListElement2* element, nsIRenderingContext &aRC)
{
  PRBool isTranslucent = (element->mFlags & VIEW_TRANSLUCENT) != 0;
  PRBool clipEmpty;
  nsRect r;
  nsView* view = element->mView;

  view->GetDimensions(r);

  if (!isTranslucent) {
    aRC.PushState();

    nscoord x = element->mAbsX - r.x, y = element->mAbsY - r.y;
    aRC.Translate(x, y);

    nsRect drect(element->mBounds.x - x, element->mBounds.y - y,
                 element->mBounds.width, element->mBounds.height);

    element->mView->Paint(aRC, drect, 0, clipEmpty);
    
    aRC.PopState(clipEmpty);
  }

#if defined(SUPPORT_TRANSLUCENT_VIEWS)  
  if (mTranslucentViewCount > 0 && (isTranslucent || mTranslucentArea.Intersects(element->mBounds))) {
    // transluscency case. if this view is transluscent, have to use the nsIBlender, otherwise, just
    // render in the offscreen. when we reach the last transluscent view, then we flush the bits
    // to the onscreen rendering context.
    
    // compute the origin of the view, relative to the offscreen buffer, which has the
    // same dimensions as mTranslucentArea.
    nscoord x = element->mAbsX - r.x, y = element->mAbsY - r.y;
    nscoord viewX = x - mTranslucentArea.x, viewY = y - mTranslucentArea.y;

    nsRect damageRect(element->mBounds);
    damageRect.IntersectRect(damageRect, mTranslucentArea);
    // -> coordinates relative to element->mView origin
    damageRect.x -= x, damageRect.y -= y;
    
    if (element->mFlags & VIEW_TRANSLUCENT) {
      // paint the view twice, first in the black buffer, then the white;
      // the blender will pick up the touched pixels only.
      PaintView(view, *mBlackCX, viewX, viewY, damageRect);
      // DEBUGGING ONLY
      //aRC.CopyOffScreenBits(gBlack, 0, 0, element->mBounds,
      //            NS_COPYBITS_XFORM_DEST_VALUES | NS_COPYBITS_TO_BACK_BUFFER);

      PaintView(view, *mWhiteCX, viewX, viewY, damageRect);
      // DEBUGGING ONLY
      //aRC.CopyOffScreenBits(gWhite, 0, 0, element->mBounds,
      //            NS_COPYBITS_XFORM_DEST_VALUES | NS_COPYBITS_TO_BACK_BUFFER);
      //mOffScreenCX->CopyOffScreenBits(gWhite, 0, 0, nsRect(viewX, viewY, damageRect.width, damageRect.height),
      //            NS_COPYBITS_XFORM_DEST_VALUES | NS_COPYBITS_TO_BACK_BUFFER);

      float opacity;
      view->GetOpacity(opacity);

      // -> coordinates relative to mTranslucentArea origin
      damageRect.x += viewX, damageRect.y += viewY;

      // perform the blend itself.
      nsRect damageRectInPixels = damageRect;
      damageRectInPixels *= mTwipsToPixels;
      if (damageRectInPixels.width > 0 && damageRectInPixels.height > 0) {
        nsresult rv = mBlender->Blend(damageRectInPixels.x, damageRectInPixels.y,
                                      damageRectInPixels.width, damageRectInPixels.height,
                                      mBlackCX, mOffScreenCX,
                                      damageRectInPixels.x, damageRectInPixels.y,
                                      opacity, mWhiteCX,
                                      NS_RGB(0, 0, 0), NS_RGB(255, 255, 255));
        if (NS_FAILED(rv)) {
          NS_WARNING("Blend failed!");
          // let's paint SOMETHING. Paint opaquely
          PaintView(view, *mOffScreenCX, viewX, viewY, damageRect);
        }
      }
 
      // Set the contexts back to their default colors
      // We do that here because we know that whatever the clip region is,
      // everything we just painted is within the clip region so we are
      // sure to be able to overwrite it now.
      mBlackCX->SetColor(NS_RGB(0, 0, 0));
      mBlackCX->FillRect(damageRect);
      mWhiteCX->SetColor(NS_RGB(255, 255, 255));
      mWhiteCX->FillRect(damageRect);
    } else {
      PaintView(view, *mOffScreenCX, viewX, viewY, damageRect);
    }
  }
#endif
}

void nsViewManager::PaintView(nsView *aView, nsIRenderingContext &aRC, nscoord x, nscoord y,
                              const nsRect &aDamageRect)
{
  aRC.PushState();
  aRC.Translate(x, y);
  PRBool unused;
  aView->Paint(aRC, aDamageRect, 0, unused);
  aRC.PopState(unused);
}

inline PRInt32 nextPowerOf2(PRInt32 value)
{
  PRInt32 result = 1;
  while (value > result)
    result <<= 1;
  return result;
}

static nsresult NewOffscreenContext(nsIDeviceContext* deviceContext, nsDrawingSurface surface,
                                    const nsSize& size, nsIRenderingContext* *aResult)
{
  nsresult rv;
  nsIRenderingContext* context;
  rv = nsComponentManager::CreateInstance(kRenderingContextCID, nsnull,
                                          NS_GET_IID(nsIRenderingContext),
                                          (void **)&context);
  if (NS_FAILED(rv))
    return rv;
  rv = context->Init(deviceContext, surface);
  if (NS_FAILED(rv))
    return rv;

  // always initialize clipping, linux won't draw images otherwise.
  PRBool clipEmpty;
  nsRect clip(0, 0, size.width, size.height);
  context->SetClipRect(clip, nsClipCombine_kReplace, clipEmpty);
  
  *aResult = context;
  return NS_OK;
}

nsresult nsViewManager::CreateBlendingBuffers(nsIRenderingContext &aRC)
{
  nsresult rv = NS_OK;

  // create a blender, if none exists already.
  if (nsnull == mBlender) {
    rv = nsComponentManager::CreateInstance(kBlenderCID, nsnull, NS_GET_IID(nsIBlender), (void **)&mBlender);
    if (NS_FAILED(rv))
      return rv;
    rv = mBlender->Init(mContext);
    if (NS_FAILED(rv))
      return rv;
  }

  // ensure that the global drawing surfaces are large enough.
  if (mTranslucentArea.width > gOffScreenSize.width || mTranslucentArea.height > gOffScreenSize.height) {
    nsRect offscreenBounds(0, 0, mTranslucentArea.width, mTranslucentArea.height);
    offscreenBounds.ScaleRoundOut(mTwipsToPixels);
    offscreenBounds.width = nextPowerOf2(offscreenBounds.width);
    offscreenBounds.height = nextPowerOf2(offscreenBounds.height);

    NS_IF_RELEASE(mOffScreenCX);
    NS_IF_RELEASE(mBlackCX);
    NS_IF_RELEASE(mWhiteCX);

    if (nsnull != gOffScreen) {
      aRC.DestroyDrawingSurface(gOffScreen);
      gOffScreen = nsnull;
    }
    rv = aRC.CreateDrawingSurface(&offscreenBounds, NS_CREATEDRAWINGSURFACE_FOR_PIXEL_ACCESS, gOffScreen);
    if (NS_FAILED(rv))
      return rv;

    if (nsnull != gBlack) {
      aRC.DestroyDrawingSurface(gBlack);
      gBlack = nsnull;
    }
    rv = aRC.CreateDrawingSurface(&offscreenBounds, NS_CREATEDRAWINGSURFACE_FOR_PIXEL_ACCESS, gBlack);
    if (NS_FAILED(rv))
      return rv;

    if (nsnull != gWhite) {
      aRC.DestroyDrawingSurface(gWhite);
      gWhite = nsnull;
    }
    rv = aRC.CreateDrawingSurface(&offscreenBounds, NS_CREATEDRAWINGSURFACE_FOR_PIXEL_ACCESS, gWhite);
    if (NS_FAILED(rv))
      return rv;

    offscreenBounds.ScaleRoundIn(mPixelsToTwips);
    gOffScreenSize.width = offscreenBounds.width;
    gOffScreenSize.height = offscreenBounds.height;
  }

  // recreate local offscreen & blending contexts, if necessary.
  if (mOffScreenCX == nsnull) {
    rv = NewOffscreenContext(mContext, gOffScreen, gOffScreenSize, &mOffScreenCX);
    if (NS_FAILED(rv))
      return rv;
  }
  if (mBlackCX == nsnull) {
    rv = NewOffscreenContext(mContext, gBlack, gOffScreenSize, &mBlackCX);
    if (NS_FAILED(rv))
      return rv;
  }
  if (mWhiteCX == nsnull) {
    rv = NewOffscreenContext(mContext, gWhite, gOffScreenSize, &mWhiteCX);
    if (NS_FAILED(rv))
      return rv;
  }

  nsRect fillArea = mTranslucentArea;
  fillArea.x = 0;
  fillArea.y = 0;

  mBlackCX->SetColor(NS_RGB(0, 0, 0));
  mBlackCX->FillRect(fillArea);
  mWhiteCX->SetColor(NS_RGB(255, 255, 255));
  mWhiteCX->FillRect(fillArea);

  return NS_OK;
}

void nsViewManager::ProcessPendingUpdates(nsView* aView)
{
  // Protect against a null-view.
  if (nsnull == aView) {
    return;
  }
  PRBool hasWidget;
  aView->HasWidget(&hasWidget);
  if (hasWidget) {
    nsCOMPtr<nsIRegion> dirtyRegion;
    aView->GetDirtyRegion(*getter_AddRefs(dirtyRegion));
    if (dirtyRegion != nsnull && !dirtyRegion->IsEmpty()) {
      nsCOMPtr<nsIWidget> widget;
      aView->GetWidget(*getter_AddRefs(widget));
      if (widget) {
        widget->InvalidateRegion(dirtyRegion, PR_FALSE);
      }
      dirtyRegion->Init();
    }
  }

  // process pending updates in child view.
  nsView* childView = aView->GetFirstChild();
  while (nsnull != childView)  {
    ProcessPendingUpdates(childView);
    childView = childView->GetNextSibling();
  }

}

NS_IMETHODIMP nsViewManager::Composite()
{
  if (mUpdateCnt > 0)
    {
      if (nsnull != mRootWindow)
        mRootWindow->Update();

      mUpdateCnt = 0;
    }

  return NS_OK;
}

NS_IMETHODIMP nsViewManager::UpdateView(nsIView *aView, PRUint32 aUpdateFlags)
{
  // Mark the entire view as damaged
  nsView* view = NS_STATIC_CAST(nsView*, aView);

  nsRect bounds;
  view->GetBounds(bounds);
  view->ConvertFromParentCoords(&bounds.x, &bounds.y);
  return UpdateView(view, bounds, aUpdateFlags);
}


// Invalidate all widgets which overlap the view, other than the view's own widgets.
NS_IMETHODIMP
nsViewManager::UpdateViewAfterScroll(nsIView *aView, PRInt32 aDX, PRInt32 aDY)
{
  nsView* view = NS_STATIC_CAST(nsView*, aView);

  nsPoint origin(0, 0);
  ComputeViewOffset(view, &origin);
  nsRect damageRect;
  view->GetBounds(damageRect);
  view->ConvertFromParentCoords(&damageRect.x, &damageRect.y);
  damageRect.x += origin.x;
  damageRect.y += origin.y;

  // if this is a floating view, it isn't covered by any widgets other than
  // its children, which are handled by the widget scroller.
  PRBool viewIsFloating = PR_FALSE;
  view->GetFloating(viewIsFloating);
  if (viewIsFloating) {
    return NS_OK;
  }

  UpdateAllCoveringWidgets(mRootView, view, damageRect, PR_FALSE);
  Composite();
  return NS_OK;
}

// Returns true if this view's widget(s) completely cover the rectangle
// The specified rectangle, relative to aView, is invalidated in every widget child of aView.
// If non-null, aTarget and its children are ignored and only widgets above aTarget's widget
// in Z-order are invalidated (if possible).
PRBool nsViewManager::UpdateAllCoveringWidgets(nsView *aView, nsView *aTarget,
                                               nsRect &aDamagedRect, PRBool aRepaintOnlyUnblittableViews)
{
  if (aView == aTarget) {
    aRepaintOnlyUnblittableViews = PR_TRUE;
  }

  nsRect bounds;
  aView->GetBounds(bounds);
  aView->ConvertFromParentCoords(&bounds.x, &bounds.y);
  PRBool overlap = bounds.IntersectRect(bounds, aDamagedRect);
    
  if (!overlap) {
    return PR_FALSE;
  }

  PRBool noCropping = bounds == aDamagedRect;
    
  PRBool hasWidget = PR_FALSE;
  if (mRootView == aView) {
    hasWidget = PR_TRUE;
  } else {
    aView->HasWidget(&hasWidget);
  }

  PRUint32 flags = 0;
  aView->GetViewFlags(&flags);
  PRBool isBlittable = (flags & NS_VIEW_FLAG_DONT_BITBLT) == 0;
    
  nsView* childView = aView->GetFirstChild();
  PRBool childCovers = PR_FALSE;
  while (nsnull != childView) {
    nsRect childRect = bounds;
    childView->ConvertFromParentCoords(&childRect.x, &childRect.y);
    if (UpdateAllCoveringWidgets(childView, aTarget, childRect, aRepaintOnlyUnblittableViews)) {
      childCovers = PR_TRUE;
      // we can't stop here. We're not making any assumptions about how the child
      // widgets are z-ordered, and we can't risk failing to invalidate the top-most
      // one.
    }
    childView = childView->GetNextSibling();
  }

  if (!childCovers && (!isBlittable || (hasWidget && !aRepaintOnlyUnblittableViews))) {
    ++mUpdateCnt;

    if (!mRefreshEnabled) {
      // accumulate this rectangle in the view's dirty region, so we can process it later.
      AddRectToDirtyRegion(aView, bounds);
      mHasPendingInvalidates = PR_TRUE;
    } else {
      nsView* widgetView = GetWidgetView(aView);
      if (widgetView != nsnull) {
        ViewToWidget(aView, widgetView, bounds);

        nsCOMPtr<nsIWidget> widget;
        GetWidgetForView(widgetView, getter_AddRefs(widget));
        widget->Invalidate(bounds, PR_FALSE);
      }
    }
  }

  PRBool hasVisibleWidget = PR_FALSE;
  if (hasWidget) {
    nsViewVisibility visible;
    aView->GetVisibility(visible);
    if (visible == nsViewVisibility_kShow) {
      hasVisibleWidget = PR_TRUE;
    }
  }

  return noCropping && (hasVisibleWidget || childCovers);
}

NS_IMETHODIMP nsViewManager::UpdateView(nsIView *aView, const nsRect &aRect, PRUint32 aUpdateFlags)
{
  NS_PRECONDITION(nsnull != aView, "null view");

  nsView* view = NS_STATIC_CAST(nsView*, aView);

  // Only Update the rectangle region of the rect that intersects the view's non clipped rectangle
  nsRect clippedRect;
  PRBool isClipped;
  PRBool isEmpty;
  view->GetClippedRect(clippedRect, isClipped, isEmpty);
  if (isEmpty) {
    return NS_OK;
  }

  nsRect damagedRect;
  damagedRect.x = aRect.x;
  damagedRect.y = aRect.y;
  damagedRect.width = aRect.width;
  damagedRect.height = aRect.height;
  clippedRect.x = 0;
  clippedRect.y = 0;
  damagedRect.IntersectRect(aRect, clippedRect);

   // If the rectangle is not visible then abort
   // without invalidating. This is a performance 
   // enhancement since invalidating a native widget
   // can be expensive.
   // This also checks for silly request like damagedRect.width = 0 or damagedRect.height = 0
  PRBool isVisible;
  IsRectVisible(view, damagedRect, 0, &isVisible);
  if (!isVisible) {
    return NS_OK;
  }

  // if this is a floating view, it isn't covered by any widgets other than
  // its children. In that case we walk up to its parent widget and use
  // that as the root to update from. This also means we update areas that
  // may be outside the parent view(s), which is necessary for floaters.
  PRBool viewIsFloating = PR_FALSE;
  view->GetFloating(viewIsFloating);
  if (viewIsFloating) {
    nsView* widgetParent = view;
    PRBool hasWidget = PR_FALSE;
    widgetParent->HasWidget(&hasWidget);

    while (!hasWidget) {
      widgetParent->ConvertToParentCoords(&damagedRect.x, &damagedRect.y);

      widgetParent = widgetParent->GetParent();
      widgetParent->HasWidget(&hasWidget);
    }

    UpdateAllCoveringWidgets(widgetParent, nsnull, damagedRect, PR_FALSE);
  } else {
    nsPoint origin(damagedRect.x, damagedRect.y);
    ComputeViewOffset(view, &origin);
    damagedRect.x = origin.x;
    damagedRect.y = origin.y;

    UpdateAllCoveringWidgets(mRootView, nsnull, damagedRect, PR_FALSE);
  }

  ++mUpdateCnt;

  if (!mRefreshEnabled) {
    return NS_OK;
  }

  // See if we should do an immediate refresh or wait
  if (aUpdateFlags & NS_VMREFRESH_IMMEDIATE) {
    Composite();
  } 

  return NS_OK;
}

NS_IMETHODIMP nsViewManager::UpdateAllViews(PRUint32 aUpdateFlags)
{
  UpdateViews(mRootView, aUpdateFlags);
  return NS_OK;
}

void nsViewManager::UpdateViews(nsView *aView, PRUint32 aUpdateFlags)
{
  // update this view.
  UpdateView(aView, aUpdateFlags);

  // update all children as well.
  nsView* childView = aView->GetFirstChild();
  while (nsnull != childView)  {
    UpdateViews(childView, aUpdateFlags);
    childView = childView->GetNextSibling();
  }
}

NS_IMETHODIMP nsViewManager::DispatchEvent(nsGUIEvent *aEvent, nsEventStatus *aStatus)
{
  *aStatus = nsEventStatus_eIgnore;

  switch(aEvent->message)
    {
    case NS_SIZE:
      {
        nsView* view = nsView::GetViewFor(aEvent->widget);

        if (nsnull != view)
          {
            nscoord width = ((nsSizeEvent*)aEvent)->windowSize->width;
            nscoord height = ((nsSizeEvent*)aEvent)->windowSize->height;
            width = ((nsSizeEvent*)aEvent)->mWinWidth;
            height = ((nsSizeEvent*)aEvent)->mWinHeight;

            // The root view may not be set if this is the resize associated with
            // window creation

            if (view == mRootView)
              {
                // Convert from pixels to twips
                float p2t;
                mContext->GetDevUnitsToAppUnits(p2t);

                //printf("resize: (pix) %d, %d\n", width, height);
                SetWindowDimensions(NSIntPixelsToTwips(width, p2t),
                                    NSIntPixelsToTwips(height, p2t));
                *aStatus = nsEventStatus_eConsumeNoDefault;
              }
          }

        break;
      }

    case NS_PAINT:
      {
        nsView *view = nsView::GetViewFor(aEvent->widget);

        if (nsnull != view)
          {
            // Do an immediate refresh
            if (nsnull != mContext)
              {
                // The rect is in device units, and it's in the coordinate space of its
                // associated window.
                nsRect& damrect = *((nsPaintEvent*)aEvent)->rect;

                if (damrect.width > 0 && damrect.height > 0)
                  {
                    PRUint32   updateFlags = NS_VMREFRESH_DOUBLE_BUFFER;
                    PRBool     doDefault = PR_TRUE;

                    // printf("refreshing: view: %x, %d, %d, %d, %d\n", view, damrect.x, damrect.y, damrect.width, damrect.height);
                    // Refresh the view
                    if (mRefreshEnabled) {
                      nsCOMPtr<nsIRegion> rgn;
                      nsresult rv = CreateRegion(getter_AddRefs(rgn));
                      if (NS_SUCCEEDED(rv)) {
                          // Eventually we would like the platform paint event to include a region
                          // we can use. This could improve paint performance when the invalid area
                          // is more complicated than a rectangle. Right now the event's region field
                          // just contains garbage on some platforms so we can't trust it at all.
                          // When that gets fixed, we can just change the code right here.
                          rgn->SetTo(damrect.x, damrect.y, damrect.width, damrect.height);
                          Refresh(view, ((nsPaintEvent*)aEvent)->renderingContext, rgn, updateFlags);
                          doDefault = PR_FALSE;
                      }
                    }
                    

                    // since we got an NS_PAINT event, we need to
                    // draw something so we don't get blank areas.
                    if (doDefault) {
                      float p2t;
                      mContext->GetDevUnitsToAppUnits(p2t);
                      damrect.ScaleRoundOut(p2t);
                      DefaultRefresh(view, &damrect);

                      // Clients like the editor can trigger multiple
                      // reflows during what the user perceives as a single
                      // edit operation, so it disables view manager
                      // refreshing until the edit operation is complete
                      // so that users don't see the intermediate steps.
                      // 
                      // Unfortunately some of these reflows can trigger
                      // nsScrollPortView and nsScrollingView Scroll() calls
                      // which in most cases force an immediate BitBlt and
                      // synchronous paint to happen even if the view manager's
                      // refresh is disabled. (Bug 97674)
                      //
                      // Calling UpdateView() here, is neccessary to add
                      // the exposed region specified in the synchronous paint
                      // event to  the view's damaged region so that it gets
                      // painted properly when refresh is enabled.
                      //
                      // Note that calling UpdateView() here was deemed
                      // to have the least impact on performance, since the
                      // other alternative was to make Scroll() post an
                      // async paint event for the *entire* ScrollPort or
                      // ScrollingView's viewable area. (See bug 97674 for this
                      // alternate patch.)

                      UpdateView(view, damrect, NS_VMREFRESH_NO_SYNC);
                    }
                  }
              }
            *aStatus = nsEventStatus_eConsumeNoDefault;
          }

        break;
      }

    case NS_CREATE:
    case NS_DESTROY:
    case NS_SETZLEVEL:
    case NS_MOVE:
      /* Don't pass these events through. Passing them through
         causes performance problems on pages with lots of views/frames 
         @see bug 112861 */
      *aStatus = nsEventStatus_eConsumeNoDefault;
      break;


    case NS_DISPLAYCHANGED:

      //Destroy the cached backbuffer to force a new backbuffer
      //be constructed with the appropriate display depth.
      //@see bugzilla bug 6061
      *aStatus = nsEventStatus_eConsumeDoDefault;
      if (gCleanupContext) {
        gCleanupContext->DestroyCachedBackbuffer();
      }
      break;



    default:
      {
        nsView* baseView;
        nsView* view;
        nsPoint offset;
        nsIScrollbar* sb;
        PRBool capturedEvent = PR_FALSE;

        if (NS_IS_MOUSE_EVENT(aEvent) || NS_IS_KEY_EVENT(aEvent)) {
          gLastUserEventTime = PR_IntervalToMicroseconds(PR_IntervalNow());
        }

        //Find the view whose coordinates system we're in.
        baseView = nsView::GetViewFor(aEvent->widget);

        //Find the view to which we're initially going to send the event 
        //for hittesting.
        if (nsnull != mMouseGrabber && (NS_IS_MOUSE_EVENT(aEvent) || (NS_IS_DRAG_EVENT(aEvent)))) {
          view = mMouseGrabber;
          capturedEvent = PR_TRUE;
        }
        else if (nsnull != mKeyGrabber && NS_IS_KEY_EVENT(aEvent)) {
          view = mKeyGrabber;
          capturedEvent = PR_TRUE;
        }
        else if (NS_OK == aEvent->widget->QueryInterface(NS_GET_IID(nsIScrollbar), (void**)&sb)) {
          view = baseView;
          capturedEvent = PR_TRUE;
          NS_RELEASE(sb);
        }
        else {
          view = baseView;
        }

        if (nsnull != view) {
          //Calculate the proper offset for the view we're going to
          offset.x = offset.y = 0;
          if (baseView != view) {
            //Get offset from root of baseView
            nsView *parent;

            parent = baseView;
            while (nsnull != parent) {
              parent->ConvertToParentCoords(&offset.x, &offset.y);
              parent = parent->GetParent();
            }

            //Subtract back offset from root of view
            parent = view;
            while (nsnull != parent) {
              parent->ConvertFromParentCoords(&offset.x, &offset.y);
              parent = parent->GetParent();
            }
      
          }

          //Dispatch the event
          //Before we start mucking with coords, make sure we know our baseline
          aEvent->refPoint.x = aEvent->point.x;
          aEvent->refPoint.y = aEvent->point.y;

          nsRect baseViewDimensions;
          if (baseView != nsnull) {
            baseView->GetDimensions(baseViewDimensions);
          }

          float t2p;
          mContext->GetAppUnitsToDevUnits(t2p);
          float p2t;
          mContext->GetDevUnitsToAppUnits(p2t);

          aEvent->point.x = baseViewDimensions.x + NSIntPixelsToTwips(aEvent->point.x, p2t);
          aEvent->point.y = baseViewDimensions.y + NSIntPixelsToTwips(aEvent->point.y, p2t);

          aEvent->point.x += offset.x;
          aEvent->point.y += offset.y;

          *aStatus = view->HandleEvent(this, aEvent, capturedEvent);

          // From here on out, "this" could have been deleted!!!

          // From here on out, "this" could have been deleted!!!

          aEvent->point.x -= offset.x;
          aEvent->point.y -= offset.y;

          aEvent->point.x = NSTwipsToIntPixels(aEvent->point.x - baseViewDimensions.x, t2p);
          aEvent->point.y = NSTwipsToIntPixels(aEvent->point.y - baseViewDimensions.y, t2p);

          //
          // if the event is an nsTextEvent, we need to map the reply back into platform coordinates
          //
          if (aEvent->message==NS_TEXT_EVENT) {
            ((nsTextEvent*)aEvent)->theReply.mCursorPosition.x=NSTwipsToIntPixels(((nsTextEvent*)aEvent)->theReply.mCursorPosition.x, t2p);
            ((nsTextEvent*)aEvent)->theReply.mCursorPosition.y=NSTwipsToIntPixels(((nsTextEvent*)aEvent)->theReply.mCursorPosition.y, t2p);
            ((nsTextEvent*)aEvent)->theReply.mCursorPosition.width=NSTwipsToIntPixels(((nsTextEvent*)aEvent)->theReply.mCursorPosition.width, t2p);
            ((nsTextEvent*)aEvent)->theReply.mCursorPosition.height=NSTwipsToIntPixels(((nsTextEvent*)aEvent)->theReply.mCursorPosition.height, t2p);
          }
          if((aEvent->message==NS_COMPOSITION_START) ||
             (aEvent->message==NS_COMPOSITION_QUERY)) {
            ((nsCompositionEvent*)aEvent)->theReply.mCursorPosition.x=NSTwipsToIntPixels(((nsCompositionEvent*)aEvent)->theReply.mCursorPosition.x,t2p);
            ((nsCompositionEvent*)aEvent)->theReply.mCursorPosition.y=NSTwipsToIntPixels(((nsCompositionEvent*)aEvent)->theReply.mCursorPosition.y,t2p);
            ((nsCompositionEvent*)aEvent)->theReply.mCursorPosition.width=NSTwipsToIntPixels(((nsCompositionEvent*)aEvent)->theReply.mCursorPosition.width,t2p);
            ((nsCompositionEvent*)aEvent)->theReply.mCursorPosition.height=NSTwipsToIntPixels(((nsCompositionEvent*)aEvent)->theReply.mCursorPosition.height,t2p);
          }
        }
    
        break;
      }
    }

  return NS_OK;
}

void nsViewManager::BuildDisplayList(nsView* aView, const nsRect& aRect, PRBool aEventProcessing,
  PRBool aCaptured) {
  // compute this view's origin
  nsPoint origin(0, 0);
  ComputeViewOffset(aView, &origin);
    
  nsView *displayRoot = aView;
  if (!aCaptured) {
    for (;;) {
      nsView *displayParent = displayRoot->GetParent();

      if (nsnull == displayParent) {
        break;
      }
      PRBool isFloating = PR_FALSE;
      displayRoot->GetFloating(isFloating);
      PRBool isParentFloating = PR_FALSE;
      displayParent->GetFloating(isParentFloating);

      if (isFloating && !isParentFloating) {
        break;
      }
      displayRoot = displayParent;
    }
  }
    
  DisplayZTreeNode *zTree;

  if (!aEventProcessing && nsnull != mOpaqueRgn) {
    mOpaqueRgn->SetTo(0, 0, 0, 0);
    AddCoveringWidgetsToOpaqueRegion(mOpaqueRgn, mContext, aView);
  }

  nsPoint displayRootOrigin(0, 0);
  ComputeViewOffset(displayRoot, &displayRootOrigin);
    
  // Create the Z-ordered view tree
  PRBool paintFloaters;
  if (aEventProcessing) {
    paintFloaters = PR_TRUE;
  } else {
    displayRoot->GetFloating(paintFloaters);
  }
  CreateDisplayList(displayRoot, PR_FALSE, zTree, PR_FALSE, origin.x, origin.y,
                    aView, &aRect, nsnull, displayRootOrigin.x, displayRootOrigin.y, paintFloaters, aEventProcessing);
  mMapPlaceholderViewToZTreeNode.Reset();
    
  if (nsnull != zTree) {
    // Apply proper Z-order handling
    nsAutoVoidArray mergeTmp;

    SortByZOrder(zTree, mDisplayList, mergeTmp, PR_TRUE);
  }
    
  mDisplayListCount = mDisplayList.Count();
    
  DestroyZTreeNode(zTree);
}

void nsViewManager::BuildEventTargetList(nsAutoVoidArray &aTargets, nsView* aView, nsGUIEvent* aEvent,
  PRBool aCaptured) {
  NS_ASSERTION(!mPainting, "View manager cannot handle events during a paint");
  if (mPainting) {
    return;
  }

  nsRect eventRect(aEvent->point.x, aEvent->point.y, 1, 1);

  BuildDisplayList(aView, eventRect, PR_TRUE, aCaptured);

  // ShowDisplayList(mDisplayListCount);

  // The display list is in order from back to front. We return the target list in order from
  // front to back.
  for (PRInt32 i = mDisplayListCount - 1; i >= 0; --i) {
    DisplayListElement2* element = NS_STATIC_CAST(DisplayListElement2*, mDisplayList.ElementAt(i));
    if (element->mFlags & VIEW_RENDERED) {
      aTargets.AppendElement(element);
    } else {
      delete element;
    }
  }
    
  mDisplayList.Clear();
}

nsEventStatus nsViewManager::HandleEvent(nsView* aView, nsGUIEvent* aEvent, PRBool aCaptured) {
//printf(" %d %d %d %d (%d,%d) \n", this, event->widget, event->widgetSupports, 
//       event->message, event->point.x, event->point.y);

  // Hold a refcount to the observer. The continued existence of the observer will
  // delay deletion of this view hierarchy should the event want to cause its
  // destruction in, say, some JavaScript event handler.
  nsCOMPtr<nsIViewObserver> obs;
  GetViewObserver(*getter_AddRefs(obs));

  // if accessible event pass directly to the view observer
  if (aEvent->eventStructType == NS_ACCESSIBLE_EVENT || aEvent->message == NS_CONTEXTMENU_KEY) {
    nsEventStatus status = nsEventStatus_eIgnore;
    if (obs) {
       PRBool handled;
       obs->HandleEvent(aView, aEvent, &status, PR_TRUE, handled);
    }
    return status;
  }

  nsAutoVoidArray targetViews;

  // In fact, we only need to take this expensive path when the event is a mouse event ... riiiight?
  BuildEventTargetList(targetViews, aView, aEvent, aCaptured);

  nsEventStatus status = nsEventStatus_eIgnore;

  for (PRInt32 i = 0; i < targetViews.Count(); i++) {
    DisplayListElement2* element = NS_STATIC_CAST(DisplayListElement2*, targetViews.ElementAt(i));
    nsView* v = element->mView;

    if (nsnull != v->GetClientData() && nsnull != obs) {
      PRBool handled = PR_FALSE;
      nsRect r;
      v->GetDimensions(r);

      nscoord x = element->mAbsX - r.x;
      nscoord y = element->mAbsY - r.y;

      aEvent->point.x -= x;
      aEvent->point.y -= y;

      obs->HandleEvent(v, aEvent, &status, i == targetViews.Count() - 1, handled);

      aEvent->point.x += x;
      aEvent->point.y += y;

      if (handled) {
        while (i < targetViews.Count()) {
          DisplayListElement2* e = NS_STATIC_CAST(DisplayListElement2*, targetViews.ElementAt(i));
          delete e;
          i++;
        }
        break;
      }
      // if the child says "not handled" but did something which deleted the entire view hierarchy,
      // we'll crash in the next iteration. Oh well. The old code would have crashed too.
    }

    delete element;
  }

  return status;
}

NS_IMETHODIMP nsViewManager::GrabMouseEvents(nsIView *aView, PRBool &aResult)
{
#ifdef DEBUG_mjudge
  if (aView)
    {
      printf("capturing mouse events for view %x\n",aView);
    }
  printf("removing mouse capture from view %x\n",mMouseGrabber);
#endif

  mMouseGrabber = NS_STATIC_CAST(nsView*, aView);
  aResult = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP nsViewManager::GrabKeyEvents(nsIView *aView, PRBool &aResult)
{
  mKeyGrabber = NS_STATIC_CAST(nsView*, aView);
  aResult = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP nsViewManager::GetMouseEventGrabber(nsIView *&aView)
{
  aView = mMouseGrabber;
  return NS_OK;
}

NS_IMETHODIMP nsViewManager::GetKeyEventGrabber(nsIView *&aView)
{
  aView = mKeyGrabber;
  return NS_OK;
}

NS_IMETHODIMP nsViewManager::InsertChild(nsIView *aParent, nsIView *aChild, nsIView *aSibling,
                                         PRBool aAfter)
{
  nsView* parent = NS_STATIC_CAST(nsView*, aParent);
  nsView* child = NS_STATIC_CAST(nsView*, aChild);
  nsView* sibling = NS_STATIC_CAST(nsView*, aSibling);
  
  NS_PRECONDITION(nsnull != parent, "null ptr");
  NS_PRECONDITION(nsnull != child, "null ptr");
  NS_ASSERTION(sibling == nsnull || sibling->GetParent() == parent,
               "tried to insert view with invalid sibling");
  NS_ASSERTION(!IsViewInserted(child), "tried to insert an already-inserted view");

  if ((nsnull != parent) && (nsnull != child))
    {
      nsView *kid = parent->GetFirstChild();
      nsView *prev = nsnull;

      //verify that the sibling exists...

#if 0 // This is the correct code, but we can't activate it yet without breaking things.
      // We will turn this on when event handling and everything else has been
      // brainfixed to understand z-indexes.
      while (nsnull != kid)
        {
          if (kid == sibling)
            break;

          //get the next sibling view

          prev = kid;
          kid = kid->GetNextSibling();
        }

      // either kid == sibling and prev is the child before sibling, or null
      // if sibling is the first child,
      // OR kid == null, the sibling was null or not there, and prev == last child view
      // The following code works in both cases.

      if (PR_TRUE == aAfter)
        // the child views are ordered in REVERSE document order;
        // LAST view in document order is the FIRST child
        // so we insert the new view just in front of sibling, or as the first child
        // if sibling is null
        parent->InsertChild(child, prev);
      else
        parent->InsertChild(child, kid);
#else // for now, don't keep consistent document order, but order things by z-index instead
      // essentially we're emulating the old InsertChild(parent, child, zindex)
      PRInt32 zIndex = child->GetZIndex();
      while (nsnull != kid)
        {
          PRInt32 idx = kid->GetZIndex();

          if (zIndex >= idx)
            break;

          prev = kid;
          kid = kid->GetNextSibling();
        }

      parent->InsertChild(child, prev);
#endif

      UpdateTransCnt(nsnull, child);

      // if the parent view is marked as "floating", make the newly added view float as well.
      PRBool isFloating = PR_FALSE;
      parent->GetFloating(isFloating);
      if (isFloating)
        child->SetFloating(isFloating);

      //and mark this area as dirty if the view is visible...

      nsViewVisibility  visibility;
      child->GetVisibility(visibility);

      if (nsViewVisibility_kHide != visibility)
        UpdateView(child, NS_VMREFRESH_NO_SYNC);
    }
  return NS_OK;
}

NS_IMETHODIMP nsViewManager::InsertZPlaceholder(nsIView *aParent, nsIView *aChild,
                                                nsIView *aSibling, PRBool aAfter)
{
  nsView* parent = NS_STATIC_CAST(nsView*, aParent);
  nsView* child = NS_STATIC_CAST(nsView*, aChild);

  NS_PRECONDITION(nsnull != parent, "null ptr");
  NS_PRECONDITION(nsnull != child, "null ptr");

  nsZPlaceholderView* placeholder = new nsZPlaceholderView();
  nsRect bounds(0, 0, 0, 0);
  placeholder->Init(this, bounds, parent, nsViewVisibility_kHide);
  placeholder->SetReparentedView(child);
  placeholder->SetZIndex(child->GetZIndexIsAuto(), child->GetZIndex());
  child->SetZParent(placeholder);
  
  return InsertChild(parent, placeholder, aSibling, aAfter);
}

NS_IMETHODIMP nsViewManager::InsertChild(nsIView *aParent, nsIView *aChild, PRInt32 aZIndex)
{
  nsView* parent = NS_STATIC_CAST(nsView*, aParent);
  nsView* child = NS_STATIC_CAST(nsView*, aChild);

  NS_PRECONDITION(nsnull != parent, "null ptr");
  NS_PRECONDITION(nsnull != child, "null ptr");

  if ((nsnull != parent) && (nsnull != child))
    {
      nsView *kid = parent->GetFirstChild();
      nsView *prev = nsnull;

      //find the right insertion point...

      while (nsnull != kid)
        {
          PRInt32 idx = kid->GetZIndex();

          if (aZIndex >= idx)
            break;

          //get the next sibling view

          prev = kid;
          kid = kid->GetNextSibling();
        }

      //in case this hasn't been set yet... maybe we should not do this? MMP

      child->SetZIndex(child->GetZIndexIsAuto(), aZIndex);
      parent->InsertChild(child, prev);

      UpdateTransCnt(nsnull, child);

      // if the parent view is marked as "floating", make the newly added view float as well.
      PRBool isFloating = PR_FALSE;
      parent->GetFloating(isFloating);
      if (isFloating)
        child->SetFloating(isFloating);

      //and mark this area as dirty if the view is visible...
      nsViewVisibility  visibility;
      child->GetVisibility(visibility);

      if (nsViewVisibility_kHide != visibility)
        UpdateView(child, NS_VMREFRESH_NO_SYNC);
    }
  return NS_OK;
}

NS_IMETHODIMP nsViewManager::RemoveChild(nsIView *aChild)
{
  nsView* child = NS_STATIC_CAST(nsView*, aChild);

  NS_PRECONDITION(nsnull != child, "null ptr");

  nsView* parent = child->GetParent();

  if ((nsnull != parent) && (nsnull != child))
    {
      UpdateTransCnt(child, nsnull);
      UpdateView(child, NS_VMREFRESH_NO_SYNC);
      parent->RemoveChild(child);
    }

  return NS_OK;
}

NS_IMETHODIMP nsViewManager::MoveViewBy(nsIView *aView, nscoord aX, nscoord aY)
{
  nscoord x, y;
  nsView* view = NS_STATIC_CAST(nsView*, aView);

  view->GetPosition(&x, &y);
  MoveViewTo(view, aX + x, aY + y);
  return NS_OK;
}

NS_IMETHODIMP nsViewManager::MoveViewTo(nsIView *aView, nscoord aX, nscoord aY)
{
  nscoord oldX, oldY;
  nsView* view = NS_STATIC_CAST(nsView*, aView);
  nsRect oldArea;
  view->GetPosition(&oldX, &oldY);
  view->GetBounds(oldArea);
  view->SetPosition(aX, aY);

  // only do damage control if the view is visible

  if ((aX != oldX) || (aY != oldY)) {
    nsViewVisibility  visibility;
    view->GetVisibility(visibility);
    if (visibility != nsViewVisibility_kHide) {
      nsView* parentView = view->GetParent();
      UpdateView(parentView, oldArea, NS_VMREFRESH_NO_SYNC);
      nsRect newArea;
      view->GetBounds(newArea);
      UpdateView(parentView, newArea, NS_VMREFRESH_NO_SYNC);
    }
  }
  return NS_OK;
}

void nsViewManager::InvalidateHorizontalBandDifference(nsView *aView, const nsRect& aRect, const nsRect& aCutOut,
  PRUint32 aUpdateFlags, nscoord aY1, nscoord aY2, PRBool aInCutOut) {
  nscoord height = aY2 - aY1;
  if (aRect.x < aCutOut.x) {
    nsRect r(aRect.x, aY1, aCutOut.x - aRect.x, height);
    UpdateView(aView, r, aUpdateFlags);
  }
  if (!aInCutOut && aCutOut.x < aCutOut.XMost()) {
    nsRect r(aCutOut.x, aY1, aCutOut.width, height);
    UpdateView(aView, r, aUpdateFlags);
  }
  if (aCutOut.XMost() < aRect.XMost()) {
    nsRect r(aCutOut.XMost(), aY1, aRect.XMost() - aCutOut.XMost(), height);
    UpdateView(aView, r, aUpdateFlags);
  }
}

void nsViewManager::InvalidateRectDifference(nsView *aView, const nsRect& aRect, const nsRect& aCutOut,
  PRUint32 aUpdateFlags) {
  if (aRect.y < aCutOut.y) {
    InvalidateHorizontalBandDifference(aView, aRect, aCutOut, aUpdateFlags, aRect.y, aCutOut.y, PR_FALSE);
  }
  if (aCutOut.y < aCutOut.YMost()) {
    InvalidateHorizontalBandDifference(aView, aRect, aCutOut, aUpdateFlags, aCutOut.y, aCutOut.YMost(), PR_TRUE);
  }
  if (aCutOut.YMost() < aRect.YMost()) {
    InvalidateHorizontalBandDifference(aView, aRect, aCutOut, aUpdateFlags, aCutOut.YMost(), aRect.YMost(), PR_FALSE);
  }
}

NS_IMETHODIMP nsViewManager::ResizeView(nsIView *aView, const nsRect &aRect, PRBool aRepaintExposedAreaOnly)
{
  nsView* view = NS_STATIC_CAST(nsView*, aView);
  nsRect oldDimensions;

  view->GetDimensions(oldDimensions);
  if (oldDimensions != aRect) {
    nsView* parentView = view->GetParent();
    if (parentView == nsnull)
      parentView = view;

    // resize the view.
    nsViewVisibility  visibility;
    view->GetVisibility(visibility);

    // Prevent Invalidation of hidden views 
    if (visibility == nsViewVisibility_kHide) {  
      view->SetDimensions(aRect, PR_FALSE);
    } else {
      if (!aRepaintExposedAreaOnly) {
        //Invalidate the union of the old and new size
        view->SetDimensions(aRect, PR_TRUE);

        UpdateView(view, aRect, NS_VMREFRESH_NO_SYNC);
        view->ConvertToParentCoords(&oldDimensions.x, &oldDimensions.y);
        UpdateView(parentView, oldDimensions, NS_VMREFRESH_NO_SYNC);
      } else {
        view->SetDimensions(aRect, PR_FALSE);

        InvalidateRectDifference(view, aRect, oldDimensions, NS_VMREFRESH_NO_SYNC);
        nsRect r = aRect;
        view->ConvertToParentCoords(&r.x, &r.y);
        view->ConvertToParentCoords(&oldDimensions.x, &oldDimensions.y);
        InvalidateRectDifference(parentView, oldDimensions, r, NS_VMREFRESH_NO_SYNC);
      } 
    }
  }
  
  return NS_OK;
}

NS_IMETHODIMP nsViewManager::SetViewChildClipRegion(nsIView *aView, nsIRegion *aRegion)
{
  nsView* view = NS_STATIC_CAST(nsView*, aView);
 
  NS_ASSERTION(!(nsnull == view), "no view");
   
  // XXX Shouldn't we repaint the view here?

  if (aRegion != nsnull) {
    nsRect newClip;
    aRegion->GetBoundingBox(&newClip.x, &newClip.y, &newClip.width, &newClip.height);
    view->SetViewFlags(view->GetViewFlags() | NS_VIEW_FLAG_CLIPCHILDREN);
    view->SetChildClip(newClip.x, newClip.y, newClip.XMost(), newClip.YMost());
  } else {
    view->SetViewFlags(view->GetViewFlags() & ~NS_VIEW_FLAG_CLIPCHILDREN);
  }
 
  return NS_OK;
}

NS_IMETHODIMP nsViewManager::SetViewBitBltEnabled(nsIView *aView, PRBool aEnable)
{
  nsView* view = NS_STATIC_CAST(nsView*, aView);

  NS_ASSERTION(!(nsnull == view), "no view");

  if (aEnable) {
    view->SetViewFlags(view->GetViewFlags() & ~NS_VIEW_FLAG_DONT_BITBLT);
  } else {
    view->SetViewFlags(view->GetViewFlags() | NS_VIEW_FLAG_DONT_BITBLT);
  }

  return NS_OK;
}

NS_IMETHODIMP nsViewManager::SetViewCheckChildEvents(nsIView *aView, PRBool aEnable)
{
  nsView* view = NS_STATIC_CAST(nsView*, aView);

  NS_ASSERTION(!(nsnull == view), "no view");

  if (aEnable) {
    view->SetViewFlags(view->GetViewFlags() & ~NS_VIEW_FLAG_DONT_CHECK_CHILDREN);
  } else {
    view->SetViewFlags(view->GetViewFlags() | NS_VIEW_FLAG_DONT_CHECK_CHILDREN);
  }

  return NS_OK;
}

NS_IMETHODIMP nsViewManager::SetViewFloating(nsIView *aView, PRBool aFloating)
{
  nsView* view = NS_STATIC_CAST(nsView*, aView);

  NS_ASSERTION(!(nsnull == view), "no view");

  view->SetFloating(aFloating);

  return NS_OK;
}

NS_IMETHODIMP nsViewManager::SetViewVisibility(nsIView *aView, nsViewVisibility aVisible)
{
  nsView* view = NS_STATIC_CAST(nsView*, aView);

  nsViewVisibility  oldVisible;
  view->GetVisibility(oldVisible);
  if (aVisible != oldVisible) {
    view->SetVisibility(aVisible);

    if (IsViewInserted(view)) {
      PRBool hasWidget = PR_FALSE;
      view->HasWidget(&hasWidget);
      if (!hasWidget) {
        if (nsViewVisibility_kHide == aVisible) {
          nsView* parentView = view->GetParent();
          if (parentView) {
            nsRect  bounds;
            view->GetBounds(bounds);
            UpdateView(parentView, bounds, NS_VMREFRESH_NO_SYNC);
          }
        }
        else {
          UpdateView(view, NS_VMREFRESH_NO_SYNC);
        }
      }
    }
  }
  return NS_OK;
}

PRBool nsViewManager::IsViewInserted(nsView *aView)
{
  if (mRootView == aView) {
    return PR_TRUE;
  } else if (aView->GetParent() == nsnull) {
    return PR_FALSE;
  } else {
    nsView* view = aView->GetParent()->GetFirstChild();
    while (view != nsnull) {
      if (view == aView) {
        return PR_TRUE;
      }        
      view = view->GetNextSibling();
    }
    return PR_FALSE;
  }
}

NS_IMETHODIMP nsViewManager::SetViewZIndex(nsIView *aView, PRBool aAutoZIndex, PRInt32 aZIndex)
{
  nsView* view = NS_STATIC_CAST(nsView*, aView);
  nsresult  rv = NS_OK;

  NS_ASSERTION((view != nsnull), "no view");

  if (aAutoZIndex) {
    aZIndex = 0;
  }

  PRInt32 oldidx = view->GetZIndex();

  view->SetZIndex(aAutoZIndex, aZIndex);

  if (IsViewInserted(view)) {
    if (oldidx != aZIndex) {
      nsView *parent = view->GetParent();
      if (nsnull != parent) {
        //we don't just call the view manager's RemoveChild()
        //so that we can avoid two trips trough the UpdateView()
        //code (one for removal, one for insertion). MMP
        parent->RemoveChild(view);
        UpdateTransCnt(view, nsnull);
        rv = InsertChild(parent, view, aZIndex);
      }
      
      // XXX The following else block is a workaround and should be cleaned up (bug 43410)
    } else {
      nsCOMPtr<nsIWidget> widget;
      view->GetWidget(*getter_AddRefs(widget));
      if (widget) {
        widget->SetZIndex(aZIndex);
      }
    }

    nsZPlaceholderView* zParentView = view->GetZParent();
    if (nsnull != zParentView) {
      SetViewZIndex(zParentView, aAutoZIndex, aZIndex);
    }
  }

  return rv;
}

NS_IMETHODIMP nsViewManager::SetViewContentTransparency(nsIView *aView, PRBool aTransparent)
{
  nsView* view = NS_STATIC_CAST(nsView*, aView);
  PRBool trans;

  view->HasTransparency(trans);

  if (trans != aTransparent && IsViewInserted(view))
    {
      UpdateTransCnt(view, nsnull);
      view->SetContentTransparency(aTransparent);
      UpdateTransCnt(nsnull, view);
      UpdateView(view, NS_VMREFRESH_NO_SYNC);
    }

  return NS_OK;
}

NS_IMETHODIMP nsViewManager::SetViewOpacity(nsIView *aView, float aOpacity)
{
  nsView* view = NS_STATIC_CAST(nsView*, aView);
  float opacity;

  view->GetOpacity(opacity);

  if (opacity != aOpacity && IsViewInserted(view))
    {
      UpdateTransCnt(view, nsnull);
      view->SetOpacity(aOpacity);
      UpdateTransCnt(nsnull, view);
      UpdateView(view, NS_VMREFRESH_NO_SYNC);
    }

  return NS_OK;
}

NS_IMETHODIMP nsViewManager::SetViewObserver(nsIViewObserver *aObserver)
{
  mObserver = aObserver;
  return NS_OK;
}

NS_IMETHODIMP nsViewManager::GetViewObserver(nsIViewObserver *&aObserver)
{
  if (nsnull != mObserver) {
    aObserver = mObserver;
    NS_ADDREF(mObserver);
    return NS_OK;
  } else
    return NS_ERROR_NO_INTERFACE;
}

NS_IMETHODIMP nsViewManager::GetDeviceContext(nsIDeviceContext *&aContext)
{
  NS_IF_ADDREF(mContext);
  aContext = mContext;
  return NS_OK;
}

void nsViewManager::GetMaxWidgetBounds(nsRect& aMaxWidgetBounds) const
{
  // Go through the list of viewmanagers and get the maximum width and 
  // height of their widgets
  aMaxWidgetBounds.width = 0;
  aMaxWidgetBounds.height = 0;
  PRInt32 index = 0;
  for (index = 0; index < mVMCount; index++) {

    nsViewManager* vm = (nsViewManager*)gViewManagers->ElementAt(index);
    nsCOMPtr<nsIWidget> rootWidget;

    if(NS_SUCCEEDED(vm->GetWidget(getter_AddRefs(rootWidget))) && rootWidget)
      {
        nsRect widgetBounds;
        rootWidget->GetBounds(widgetBounds);
        aMaxWidgetBounds.width = PR_MAX(aMaxWidgetBounds.width, widgetBounds.width);
        aMaxWidgetBounds.height = PR_MAX(aMaxWidgetBounds.height, widgetBounds.height);
      }
  }

  //   printf("WIDGET BOUNDS %d %d\n", aMaxWidgetBounds.width, aMaxWidgetBounds.height);
}

PRInt32 nsViewManager::GetViewManagerCount()
{
  return mVMCount;
}

const nsVoidArray* nsViewManager::GetViewManagerArray() 
{
  return gViewManagers;
}

NS_IMETHODIMP nsViewManager::ShowQuality(PRBool aShow)
{
  if (nsnull != mRootScrollable)
    mRootScrollable->ShowQuality(aShow);

  return NS_OK;
}

NS_IMETHODIMP nsViewManager::GetShowQuality(PRBool &aResult)
{
  if (nsnull != mRootScrollable)
    mRootScrollable->GetShowQuality(aResult);

  return NS_OK;
}

NS_IMETHODIMP nsViewManager::SetQuality(nsContentQuality aQuality)
{
  if (nsnull != mRootScrollable)
    mRootScrollable->SetQuality(aQuality);

  return NS_OK;
}

nsIRenderingContext * nsViewManager::CreateRenderingContext(nsView &aView)
{
  nsView              *par = &aView;
  nsCOMPtr<nsIWidget> win;
  nsIRenderingContext *cx = nsnull;
  nscoord             ax = 0, ay = 0;

  do
    {
      par->GetWidget(*getter_AddRefs(win));
      if (nsnull != win)
        break;

      //get absolute coordinates of view, but don't
      //add in view pos since the first thing you ever
      //need to do when painting a view is to translate
      //the rendering context by the views pos and other parts
      //of the code do this for us...

      if (par != &aView)
        {
          par->ConvertToParentCoords(&ax, &ay);
        }

      par = par->GetParent();
    }
  while (nsnull != par);

  if (nsnull != win)
    {
      mContext->CreateRenderingContext(&aView, cx);

      if (nsnull != cx)
        cx->Translate(ax, ay);
    }

  return cx;
}

void nsViewManager::AddRectToDirtyRegion(nsView* aView, const nsRect &aRect) const
{
  // Find a view with an associated widget. We'll transform this rect from the
  // current view's coordinate system to a "heavyweight" parent view, then convert
  // the rect to pixel coordinates, and accumulate the rect into that view's dirty region.
  nsView* widgetView = GetWidgetView(aView);
  if (widgetView != nsnull) {
    nsRect widgetRect = aRect;
    ViewToWidget(aView, widgetView, widgetRect);

    // Get the dirty region associated with the widget view
    nsCOMPtr<nsIRegion> dirtyRegion;
    if (NS_SUCCEEDED(widgetView->GetDirtyRegion(*getter_AddRefs(dirtyRegion)))) {
      // add this rect to the widget view's dirty region.
      dirtyRegion->Union(widgetRect.x, widgetRect.y, widgetRect.width, widgetRect.height);
    }
  }
}

void nsViewManager::UpdateTransCnt(nsView *oldview, nsView *newview)
{
  if (nsnull != oldview)
    {
      PRBool  hasTransparency;
      float   opacity;

      oldview->HasTransparency(hasTransparency);
      oldview->GetOpacity(opacity);

      if (hasTransparency || (1.0f != opacity))
        mTransCnt--;
    }

  if (nsnull != newview)
    {
      PRBool  hasTransparency;
      float   opacity;

      newview->HasTransparency(hasTransparency);
      newview->GetOpacity(opacity);

      if (hasTransparency || (1.0f != opacity))
        mTransCnt++;
    }
}

NS_IMETHODIMP nsViewManager::DisableRefresh(void)
{
  if (mUpdateBatchCnt > 0)
    return NS_OK;

  mRefreshEnabled = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP nsViewManager::EnableRefresh(PRUint32 aUpdateFlags)
{
  if (mUpdateBatchCnt > 0)
    return NS_OK;

  mRefreshEnabled = PR_TRUE;

  if (aUpdateFlags & NS_VMREFRESH_IMMEDIATE) {
    ProcessPendingUpdates(mRootView);
    mHasPendingInvalidates = PR_FALSE;
  } else {
    PostInvalidateEvent();
  }

  if (aUpdateFlags & NS_VMREFRESH_IMMEDIATE) {
    Composite();
  }

  return NS_OK;
}

NS_IMETHODIMP nsViewManager::BeginUpdateViewBatch(void)
{
  nsresult result = NS_OK;
  
  if (mUpdateBatchCnt == 0)
    result = DisableRefresh();

  if (NS_SUCCEEDED(result))
    ++mUpdateBatchCnt;

  return result;
}

NS_IMETHODIMP nsViewManager::EndUpdateViewBatch(PRUint32 aUpdateFlags)
{
  nsresult result = NS_OK;

  --mUpdateBatchCnt;

  NS_ASSERTION(mUpdateBatchCnt >= 0, "Invalid batch count!");

  if (mUpdateBatchCnt < 0)
    {
      mUpdateBatchCnt = 0;
      return NS_ERROR_FAILURE;
    }

  if (mUpdateBatchCnt == 0)
    result = EnableRefresh(aUpdateFlags);

  return result;
}

NS_IMETHODIMP nsViewManager::SetRootScrollableView(nsIScrollableView *aScrollable)
{
  mRootScrollable = aScrollable;

  //XXX this needs to go away when layout start setting this bit on it's own. MMP
  if (mRootScrollable)
    mRootScrollable->SetScrollProperties(NS_SCROLL_PROPERTY_ALWAYS_BLIT);

  return NS_OK;
}

NS_IMETHODIMP nsViewManager::GetRootScrollableView(nsIScrollableView **aScrollable)
{
  *aScrollable = mRootScrollable;
  return NS_OK;
}

NS_IMETHODIMP nsViewManager::Display(nsIView* aView, nscoord aX, nscoord aY, const nsRect& aClipRect)
{
  nsView              *view = NS_STATIC_CAST(nsView*, aView);
  nsIRenderingContext *localcx = nsnull;
  nsRect              trect;

  if (PR_FALSE == mRefreshEnabled)
    return NS_OK;

  NS_ASSERTION(!(PR_TRUE == mPainting), "recursive painting not permitted");

  mPainting = PR_TRUE;

  mContext->CreateRenderingContext(localcx);

  //couldn't get rendering context. this is ok if at startup
  if (nsnull == localcx)
    {
      mPainting = PR_FALSE;
      return NS_ERROR_FAILURE;
    }

  view->GetBounds(trect);
  view->ConvertFromParentCoords(&trect.x, &trect.y);

  localcx->Translate(aX, aY);

  PRBool  result;

  localcx->SetClipRect(aClipRect, nsClipCombine_kReplace, result);

  // Paint the view. The clipping rect was set above set don't clip again.
  //aView->Paint(*localcx, trect, NS_VIEW_FLAG_CLIP_SET, result);
  RenderViews(view, *localcx, trect, result);

  NS_RELEASE(localcx);

  mPainting = PR_FALSE;

  return NS_OK;

}

NS_IMETHODIMP nsViewManager::AddCompositeListener(nsICompositeListener* aListener)
{
  if (nsnull == mCompositeListeners) {
    nsresult rv = NS_NewISupportsArray(&mCompositeListeners);
    if (NS_FAILED(rv))
      return rv;
  }
  return mCompositeListeners->AppendElement(aListener);
}

NS_IMETHODIMP nsViewManager::RemoveCompositeListener(nsICompositeListener* aListener)
{
  if (nsnull != mCompositeListeners) {
    return mCompositeListeners->RemoveElement(aListener);
  }
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP nsViewManager::GetWidgetForView(nsIView *aView, nsIWidget **aWidget)
{
  nsView *view = NS_STATIC_CAST(nsView*, aView);

  *aWidget = nsnull;
  PRBool hasWidget = PR_FALSE;
  while (!hasWidget && view)
    {
      view->HasWidget(&hasWidget);
      if (!hasWidget)
        view = view->GetParent();
    }

  if (hasWidget) {
    // Widget was found in the view hierarchy
    view->GetWidget(*aWidget);
  } else {
    // No widget was found in the view hierachy, so use try to use the mRootWindow
    if (nsnull != mRootWindow) {
#ifdef NS_DEBUG
      nsViewManager* vm = view->GetViewManager();
      NS_ASSERTION(this == vm, "Must use the view instances view manager when calling GetWidgetForView");
#endif
      *aWidget = mRootWindow;
      NS_ADDREF(mRootWindow);
    }
  }

  return NS_OK;
}


NS_IMETHODIMP nsViewManager::GetWidget(nsIWidget **aWidget)
{
  NS_IF_ADDREF(mRootWindow);
  *aWidget = mRootWindow;
  return NS_OK;
}

NS_IMETHODIMP nsViewManager::ForceUpdate()
{
  if (mRootWindow) {
    mRootWindow->Update();
  }
  return NS_OK;
}

static nsresult EnsureZTreeNodeCreated(nsView* aView, DisplayZTreeNode* &aNode) {
  if (nsnull == aNode) {
    aNode = new DisplayZTreeNode;

    if (nsnull == aNode) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    aNode->mView = aView;
    aNode->mDisplayElement = nsnull;
    aNode->mZChild = nsnull;
    aNode->mZSibling = nsnull;
  }
  return NS_OK;
}

PRBool nsViewManager::CreateDisplayList(nsView *aView, PRBool aReparentedViewsPresent,
                                        DisplayZTreeNode* &aResult, PRBool aInsideRealView,
                                        nscoord aOriginX, nscoord aOriginY, nsView *aRealView,
                                        const nsRect *aDamageRect, nsView *aTopView,
                                        nscoord aX, nscoord aY, PRBool aPaintFloaters,
                                        PRBool aEventProcessing)
{
  PRBool retval = PR_FALSE;

  aResult = nsnull;

  NS_ASSERTION((aView != nsnull), "no view");

  if (!aTopView)
    aTopView = aView;

  nsRect bounds;
  aView->GetBounds(bounds);
  nscoord posX, posY;
  aView->GetPosition(&posX, &posY);

  if (aView == aTopView) {
    aView->ConvertFromParentCoords(&bounds.x, &bounds.y);
    posX = posY = 0;
  }

  aInsideRealView = aInsideRealView || aRealView == aView,

  // -> to global coordinates (relative to aTopView)
  bounds.x += aX;
  bounds.y += aY;
  posX += aX;
  posY += aY;

  // is this a clip view?
  PRBool isClipView = IsClipView(aView);
  PRBool overlap;
  nsRect irect;
    
  // -> to refresh-frame coordinates (relative to aRealView)
  bounds.x -= aOriginX;
  bounds.y -= aOriginY;
  if (aDamageRect) {
    overlap = irect.IntersectRect(bounds, *aDamageRect);
    if (isClipView) {
      aDamageRect = &irect;
    }
    if (aEventProcessing && aRealView == aView) {
      // Always deliver an event somewhere, at least to the top-level target.
      // There may be mouse capturing going on.
      overlap = PR_TRUE;
    }
  }
  else
    overlap = PR_TRUE;

  // -> to global coordinates (relative to aTopView)
  bounds.x += aOriginX;
  bounds.y += aOriginY;

  if (!overlap && isClipView) {
    return PR_FALSE;
  }

  // Don't paint floating views unless the root view being painted is a floating view.
  // This is important because we may be asked to paint
  // a window that's behind a transient floater; in this case we must paint the real window
  // contents, not the floater contents (bug 63496)
  if (!aPaintFloaters) {
    PRBool isFloating = PR_FALSE;
    aView->GetFloating(isFloating);
    if (isFloating) {
      return PR_FALSE;
    }
  }

  if (!aReparentedViewsPresent) {
    for (nsView* childView = aView->GetFirstChild(); nsnull != childView;
         childView = childView->GetNextSibling()) {
      nsZPlaceholderView *zParent = childView->GetZParent();
      if (nsnull != zParent) {
        aReparentedViewsPresent = PR_TRUE;
        break;
      }
    }

    if (!overlap && !aReparentedViewsPresent) {
      return PR_FALSE;
    }
  }

  PRInt32 childCount = aView->GetChildCount();
  nsView *childView = nsnull;

  if (aEventProcessing
      && (aView->GetViewFlags() & NS_VIEW_FLAG_DONT_CHECK_CHILDREN) != 0) {
    childCount = 0;
  }

  if (childCount > 0) {
    if (isClipView) {
      // -> to refresh-frame coordinates (relative to aRealView)
      bounds.x -= aOriginX;
      bounds.y -= aOriginY;

      retval = AddToDisplayList(aView, aResult, bounds, bounds, POP_CLIP, aX - aOriginX, aY - aOriginY, PR_FALSE);

      if (retval)
        return retval;

      // -> to global coordinates (relative to aTopView)
      bounds.x += aOriginX;
      bounds.y += aOriginY;
    }

    for (childView = aView->GetFirstChild(); nsnull != childView;
         childView = childView->GetNextSibling()) {
      PRInt32 zindex = childView->GetZIndex();
      if (zindex < 0)
        break;

      DisplayZTreeNode* createdNode;
      retval = CreateDisplayList(childView, aReparentedViewsPresent, createdNode,
                                 aInsideRealView,
                                 aOriginX, aOriginY, aRealView, aDamageRect, aTopView, posX, posY, aPaintFloaters,
                                 aEventProcessing);
      if (createdNode != nsnull) {
        EnsureZTreeNodeCreated(aView, aResult);
        createdNode->mZSibling = aResult->mZChild;
        aResult->mZChild = createdNode;
      }

      if (retval)
        break;
    }
  }

  if (!retval) {
    if (overlap) {
      // -> to refresh-frame coordinates (relative to aRealView)
      bounds.x -= aOriginX;
      bounds.y -= aOriginY;

      nsViewVisibility  visibility;
      float             opacity;
      PRBool            transparent;

      aView->GetVisibility(visibility);
      aView->GetOpacity(opacity);
      aView->HasTransparency(transparent);

      if ((nsViewVisibility_kShow == visibility) && (aEventProcessing || opacity > 0.0f)) {
        PRUint32 flags = VIEW_RENDERED;
        if (transparent)
          flags |= VIEW_TRANSPARENT;
#if defined(SUPPORT_TRANSLUCENT_VIEWS)
        if (opacity < 1.0f)
          flags |= VIEW_TRANSLUCENT;
#endif
        retval = AddToDisplayList(aView, aResult, bounds, irect, flags, aX - aOriginX, aY - aOriginY,
                                  aEventProcessing && aRealView == aView);
      }

      // -> to global coordinates (relative to aTopView)
      bounds.x += aOriginX;
      bounds.y += aOriginY;
    } else {
      PRUint32 compositorFlags = 0;
      aView->GetCompositorFlags(&compositorFlags);

      if (0 != (compositorFlags & IS_Z_PLACEHOLDER_VIEW)) {
        EnsureZTreeNodeCreated(aView, aResult);
        mMapPlaceholderViewToZTreeNode.Put(new nsVoidKey(aView), aResult);
      }
    }

    // any children with negative z-indices?
    if (!retval && nsnull != childView) {
      for (; nsnull != childView; childView = childView->GetNextSibling()) {
        DisplayZTreeNode* createdNode;
        retval = CreateDisplayList(childView, aReparentedViewsPresent, createdNode,
                                   aInsideRealView,
                                   aOriginX, aOriginY, aRealView, aDamageRect, aTopView, posX, posY, aPaintFloaters,
                                   aEventProcessing);
        if (createdNode != nsnull) {
          EnsureZTreeNodeCreated(aView, aResult);
          createdNode->mZSibling = aResult->mZChild;
          aResult->mZChild = createdNode;
        }
              
        if (retval)
          break;
      }
    }
  }

  if (childCount > 0 && isClipView) {
    // -> to refresh-frame coordinates (relative to aRealView)
    bounds.x -= aOriginX;
    bounds.y -= aOriginY;

    if (AddToDisplayList(aView, aResult, bounds, bounds, PUSH_CLIP, aX - aOriginX, aY - aOriginY, PR_FALSE)) {
      retval = PR_TRUE;
    }
  }

  // Reparent any views that need reparenting in the Z-order tree
  if (nsnull != aResult) {
    DisplayZTreeNode* child;
    DisplayZTreeNode** prev = &aResult->mZChild;
    for (child = aResult->mZChild; nsnull != child; child = *prev) {
      nsZPlaceholderView *zParent = nsnull;
      if (nsnull != child->mView) {
        zParent = child->mView->GetZParent();
      }
      if (nsnull != zParent) {
        // unlink the child from the tree
        *prev = child->mZSibling;
        child->mZSibling = nsnull;

        nsVoidKey key(zParent);
        DisplayZTreeNode* placeholder = (DisplayZTreeNode *)mMapPlaceholderViewToZTreeNode.Remove(&key);

        if (nsnull != placeholder) {
          NS_ASSERTION((placeholder->mDisplayElement == nsnull), "placeholder already has elements?");
          NS_ASSERTION((placeholder->mZChild == nsnull), "placeholder already has Z-children?");
          placeholder->mDisplayElement = child->mDisplayElement;
          placeholder->mView = child->mView;
          placeholder->mZChild = child->mZChild;
          delete child;
        } else {
          // the placeholder was never added to the display list ...
          // we don't need to display this then
          DestroyZTreeNode(child);
        }
      } else {
        prev = &child->mZSibling;
      }
    }
  }
 
  return retval;
}

PRBool nsViewManager::AddToDisplayList(nsView *aView, DisplayZTreeNode* &aParent,
  nsRect &aClipRect, nsRect& aDirtyRect, PRUint32 aFlags,nscoord aAbsX, nscoord aAbsY,
  PRBool aAssumeIntersection)
{
  PRBool empty;
  PRBool clipped;
  nsRect clipRect;

  aView->GetClippedRect(clipRect, clipped, empty);
  if (empty) {
    return PR_FALSE;
  }
  clipRect.x += aAbsX;
  clipRect.y += aAbsY;

  if (!clipped) {
    clipRect = aClipRect;
  }

  PRBool overlap = clipRect.IntersectRect(clipRect, aDirtyRect);
  if (!overlap && !aAssumeIntersection) {
    return PR_FALSE;
  }

  DisplayListElement2* element = new DisplayListElement2;
  if (element == nsnull) {
    return PR_TRUE;
  }
  DisplayZTreeNode* node = new DisplayZTreeNode;
  if (nsnull == node) {
    delete element;
    return PR_TRUE;
  }

  EnsureZTreeNodeCreated(aView, aParent);

  node->mDisplayElement = element;
  node->mView = nsnull;
  node->mZChild = nsnull;
  node->mZSibling = aParent->mZChild;
  aParent->mZChild = node;

  element->mView = aView;
  element->mBounds = clipRect;
  element->mAbsX = aClipRect.x;
  element->mAbsY = aClipRect.y;
  element->mFlags = aFlags;
  if (clipped) { 
    element->mFlags |= VIEW_CLIPPED;
  }
  
  return PR_FALSE;
}

// Make sure that all PUSH_CLIP/POP_CLIP pairs are honored.
// They might not be because of the Z-reparenting mess: a fixed-position view might have
// created a display element with bounds that do not reflect the clipping instructions that now
// surround the element. This would cause problems in the optimizer.
void nsViewManager::ReapplyClipInstructions(PRBool aHaveClip, nsRect& aClipRect, PRInt32& aIndex)
{   
  while (aIndex < mDisplayListCount) {
    DisplayListElement2* element = NS_STATIC_CAST(DisplayListElement2*, mDisplayList.ElementAt(aIndex));
    aIndex++;

    if (element->mFlags & VIEW_RENDERED) {
      if (aHaveClip && !element->mBounds.IntersectRect(aClipRect, element->mBounds)) {
        element->mFlags &= ~VIEW_RENDERED;
      }
    }

    if (element->mFlags & PUSH_CLIP) {
      nsRect newClip;
      if (aHaveClip) {
        newClip.IntersectRect(aClipRect, element->mBounds);
      } else {
        newClip = element->mBounds;
      }

      ReapplyClipInstructions(PR_TRUE, newClip, aIndex);
    }

    if (element->mFlags & POP_CLIP) {
      return;
    }
  }
}

/**
   Walk the display list, looking for opaque views, and remove any views that are behind them
   and totally occluded.
   We rely on a good region implementation. If nsIRegion doesn't cut it, we can disable this
   optimization ... or better still, fix nsIRegion on that platform.
   It seems to be good on Windows.

   @param aFinalTransparentRect
       Receives a rectangle enclosing all pixels in the damage rectangle
       which will not be opaquely painted over by the display list.
       Usually this will be empty, but nothing really prevents someone
       from creating a set of views that are (for example) all transparent.
*/
nsresult nsViewManager::OptimizeDisplayList(const nsRect& aDamageRect, nsRect& aFinalTransparentRect)
{
  aFinalTransparentRect = aDamageRect;

  if (nsnull == mOpaqueRgn || nsnull == mTmpRgn) {
    return NS_OK;
  }

  PRInt32 count = mDisplayListCount;
  for (PRInt32 i = count - 1; i >= 0; i--) {
    DisplayListElement2* element = NS_STATIC_CAST(DisplayListElement2*, mDisplayList.ElementAt(i));
    if (element->mFlags & VIEW_RENDERED) {
      mTmpRgn->SetTo(element->mBounds.x, element->mBounds.y, element->mBounds.width, element->mBounds.height);
      mTmpRgn->Subtract(*mOpaqueRgn);

      if (mTmpRgn->IsEmpty()) {
        element->mFlags &= ~VIEW_RENDERED;
      } else {
        mTmpRgn->GetBoundingBox(&element->mBounds.x, &element->mBounds.y,
                                &element->mBounds.width, &element->mBounds.height);

        // a view is opaque if it is neither transparent nor transluscent
        if (!(element->mFlags & (VIEW_TRANSPARENT | VIEW_TRANSLUCENT))) {
          mOpaqueRgn->Union(element->mBounds.x, element->mBounds.y, element->mBounds.width, element->mBounds.height);
        }
      }
    }
  }

  mTmpRgn->SetTo(aDamageRect.x, aDamageRect.y, aDamageRect.width, aDamageRect.height);
  mTmpRgn->Subtract(*mOpaqueRgn);
  mTmpRgn->GetBoundingBox(&aFinalTransparentRect.x, &aFinalTransparentRect.y,
                          &aFinalTransparentRect.width, &aFinalTransparentRect.height);
  
  return NS_OK;
}

// Remove redundant PUSH/POP_CLIP pairs. These could be expensive.
// We also count the translucent views and compute the translucency area in
// this pass.
void nsViewManager::OptimizeDisplayListClipping(PRBool aHaveClip, nsRect& aClipRect, PRInt32& aIndex,
                                                PRBool& aAnyRendered)
{   
  aAnyRendered = PR_FALSE;

  while (aIndex < mDisplayListCount) {
    DisplayListElement2* element = NS_STATIC_CAST(DisplayListElement2*, mDisplayList.ElementAt(aIndex));
    aIndex++;

    if (element->mFlags & VIEW_RENDERED) {
      // count number of translucent views, and
      // accumulate a rectangle of all translucent
      // views. this will be used to determine which
      // views need to be rendered into the blending
      // buffers.
      if (element->mFlags & VIEW_TRANSLUCENT) {
        if (mTranslucentViewCount++ == 0) {
          mTranslucentArea = element->mBounds;
        } else {
          mTranslucentArea.UnionRect(mTranslucentArea, element->mBounds);
        }
      }
  
      aAnyRendered = PR_TRUE;

      if (aHaveClip && (element->mFlags & VIEW_CLIPPED)) {
        nsRect newClip;
        newClip.IntersectRect(aClipRect, element->mBounds);
        // no need to clip if the clip rect doesn't change
        if (newClip == aClipRect) {
          element->mFlags &= ~VIEW_CLIPPED;
        }
      }
    }

    if (element->mFlags & PUSH_CLIP) {
      nsRect newClip;
      if (aHaveClip) {
        newClip.IntersectRect(aClipRect, element->mBounds);
      } else {
        newClip = element->mBounds;
      }

      PRBool anyRenderedViews = PR_FALSE;
      OptimizeDisplayListClipping(PR_TRUE, newClip, aIndex, anyRenderedViews);
      DisplayListElement2* popElement = NS_STATIC_CAST(DisplayListElement2*, mDisplayList.ElementAt(aIndex - 1));
      NS_ASSERTION(popElement->mFlags & POP_CLIP, "Must end with POP!");

      if (anyRenderedViews) {
        aAnyRendered = PR_TRUE;
      }
      if (!anyRenderedViews || (aHaveClip && newClip == aClipRect)) {
        // no need to clip if nothing's going to be rendered
        // ... or if the clip rect didn't change
        element->mFlags &= ~PUSH_CLIP;
        popElement->mFlags &= ~POP_CLIP;
      }
    }

    if (element->mFlags & POP_CLIP) {
      return;
    }
  }
}

#ifdef NS_DEBUG
void nsViewManager::ShowDisplayList(PRInt32 flatlen)
{
  char     nest[400];
  PRInt32  newnestcnt, nestcnt = 0, cnt;

  for (cnt = 0; cnt < 400; cnt++)
    nest[cnt] = ' ';

  printf("### display list length=%d ###\n", flatlen);

  for (cnt = 0; cnt < flatlen; cnt++) {
    DisplayListElement2* element = (DisplayListElement2*) mDisplayList.ElementAt(cnt);
    nsView*              view = element->mView;
    nsRect               rect = element->mBounds;
    PRUint32             flags = element->mFlags;
    nsRect               dim;
    nscoord              vx, vy;

    view->GetDimensions(dim);
    view->GetPosition(&vx, &vy);
    nsView* parent = view->GetParent();
    PRInt32 zindex = view->GetZIndex();

    nest[nestcnt << 1] = 0;

    printf("%snsIView@%p{%d,%d,%d,%d @ %d,%d; p=%p, z=%d} [x=%d, y=%d, w=%d, h=%d, absX=%d, absY=%d]\n",
           nest, (void*)view,
           dim.x, dim.y, dim.width, dim.height,
           vx, vy,
           (void*)parent, zindex,
           rect.x, rect.y, rect.width, rect.height,
           element->mAbsX, element->mAbsY);

    newnestcnt = nestcnt;

    if (flags)
      {
        printf("%s", nest);

        if (flags & POP_CLIP) {
          printf("POP_CLIP ");
          newnestcnt--;
        }

        if (flags & PUSH_CLIP) {
          printf("PUSH_CLIP ");
          newnestcnt++;
        }

        if (flags & VIEW_RENDERED)
          printf("VIEW_RENDERED ");

        printf("\n");
      }

    nest[nestcnt << 1] = ' ';

    nestcnt = newnestcnt;
  }
}
#endif

void nsViewManager::ComputeViewOffset(nsView *aView, nsPoint *aOrigin)
{
  if (aOrigin) {
    while (aView != nsnull) {
      // compute the view's global position in the view hierarchy.
      aView->ConvertToParentCoords(&aOrigin->x, &aOrigin->y);
      aView = aView->GetParent();
    }
  }
}

PRBool nsViewManager::DoesViewHaveNativeWidget(nsView* aView)
{
  nsCOMPtr<nsIWidget> widget;
  aView->GetWidget(*getter_AddRefs(widget));
  if (nsnull != widget)
    return (nsnull != widget->GetNativeData(NS_NATIVE_WIDGET));
  return PR_FALSE;
}

PRBool nsViewManager::IsClipView(nsView* aView)
{
  nsIClipView *clipView = nsnull;
  nsresult rv = aView->QueryInterface(NS_GET_IID(nsIClipView), (void **)&clipView);
  return (rv == NS_OK && clipView != nsnull);
}


nsView* nsViewManager::GetWidgetView(nsView *aView) const
{
  while (aView != nsnull) {
    PRBool hasWidget;
    aView->HasWidget(&hasWidget);
    if (hasWidget)
      return aView;
    aView = aView->GetParent();
  }
  return nsnull;
}

void nsViewManager::ViewToWidget(nsView *aView, nsView* aWidgetView, nsRect &aRect) const
{
  while (aView != aWidgetView) {
    aView->ConvertToParentCoords(&aRect.x, &aRect.y);
    aView = aView->GetParent();
  }
  
  // intersect aRect with bounds of aWidgetView, to prevent generating any illegal rectangles.
  nsRect bounds;
  aWidgetView->GetDimensions(bounds);
  aRect.IntersectRect(aRect, bounds);
  // account for the view's origin not lining up with the widget's
  aRect.x -= bounds.x;
  aRect.y -= bounds.y;
  
  // finally, convert to device coordinates.
  float t2p;
  mContext->GetAppUnitsToDevUnits(t2p);
  aRect.ScaleRoundOut(t2p);
}

nsresult nsViewManager::GetVisibleRect(nsRect& aVisibleRect)
{
  nsresult rv = NS_OK;

  // Get the viewport scroller
  nsIScrollableView* scrollingView;
  GetRootScrollableView(&scrollingView);

  if (scrollingView) {     
    // Determine the visible rect in the scrolled view's coordinate space.
    // The size of the visible area is the clip view size
    const nsIView*  clipViewI;
    scrollingView->GetClipView(&clipViewI);

    const nsView* clipView = NS_STATIC_CAST(const nsView*, clipViewI);
    clipView->GetDimensions(aVisibleRect);

    scrollingView->GetScrollPosition(aVisibleRect.x, aVisibleRect.y);
  } else {
    rv = NS_ERROR_FAILURE;
  }

  return rv;
}

nsresult nsViewManager::GetAbsoluteRect(nsView *aView, const nsRect &aRect, 
                                        nsRect& aAbsRect)
{
  nsIScrollableView* scrollingView = nsnull;
  GetRootScrollableView(&scrollingView);
  if (nsnull == scrollingView) { 
    return NS_ERROR_FAILURE;
  }

  nsIView* scrolledIView = nsnull;
  scrollingView->GetScrolledView(scrolledIView);
  
  nsView* scrolledView = NS_STATIC_CAST(nsView*, scrolledIView);

  // Calculate the absolute coordinates of the aRect passed in.
  // aRects values are relative to aView
  aAbsRect = aRect;
  nsView *parentView = aView;
  while ((parentView != nsnull) && (parentView != scrolledView)) {
    parentView->ConvertToParentCoords(&aAbsRect.x, &aAbsRect.y);
    parentView = parentView->GetParent();
  }

  if (parentView != scrolledView) {
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}


NS_IMETHODIMP nsViewManager::IsRectVisible(nsIView *aView, const nsRect &aRect, PRUint16 aMinTwips, PRBool *aIsVisible)
{
  nsView* view = NS_STATIC_CAST(nsView*, aView);

  // The parameter aMinTwips determines how many rows/cols of pixels must be visible on each side of the element,
  // in order to be counted as visible

  *aIsVisible = PR_FALSE;
  if (aRect.width == 0 || aRect.height == 0) {
    return NS_OK;
  }

  // is this view even visible?
  nsViewVisibility  visibility;
  view->GetVisibility(visibility);
  if (visibility == nsViewVisibility_kHide) {
    return NS_OK; 
  }

  // Calculate the absolute coordinates for the visible rectangle   
  nsRect visibleRect;
  if (GetVisibleRect(visibleRect) == NS_ERROR_FAILURE) {
    *aIsVisible = PR_TRUE;
    return NS_OK;
  }

  // Calculate the absolute coordinates of the aRect passed in.
  // aRects values are relative to aView
  nsRect absRect;
  if ((GetAbsoluteRect(view, aRect, absRect)) == NS_ERROR_FAILURE) {
    *aIsVisible = PR_TRUE;
    return NS_OK;
  }
 
  /*
   * If aMinTwips > 0, ensure at least aMinTwips of space around object is visible
   * The object is visible if:
   * ((objectTop     >= windowTop    || objectBottom >= windowTop) &&
   *  (objectLeft   >= windowLeft   || objectRight  >= windowLeft) &&
   *  (objectBottom <= windowBottom || objectTop    <= windowBottom) &&
   *  (objectRight  <= windowRight  || objectLeft   <= windowRight))
   */
  *aIsVisible = ((absRect.y >= visibleRect.y  ||  absRect.y + absRect.height >= visibleRect.y + aMinTwips) &&
                 (absRect.x >= visibleRect.x  ||  absRect.x + absRect.width  >=  visibleRect.x + aMinTwips) &&
                 (absRect.y + absRect.height <= visibleRect.y  + visibleRect.height  ||  absRect.y <= visibleRect.y + visibleRect.height - aMinTwips) &&
                 (absRect.x + absRect.width <= visibleRect.x  + visibleRect.width    ||  absRect.x <= visibleRect.x + visibleRect.width - aMinTwips));

  return NS_OK;
}


NS_IMETHODIMP
nsViewManager::IsCachingWidgetChanges(PRBool* aCaching)
{
#ifdef CACHE_WIDGET_CHANGES
  *aCaching = (mCachingWidgetChanges > 0);
#else
  *aCaching = PR_FALSE;
#endif

  return NS_OK;
}

NS_IMETHODIMP
nsViewManager::CacheWidgetChanges(PRBool aCache)
{

#ifdef CACHE_WIDGET_CHANGES
  if (aCache == PR_TRUE)
    mCachingWidgetChanges++;
  else
    mCachingWidgetChanges--;

  NS_ASSERTION(mCachingWidgetChanges >= 0, "One too many decrements");

  // if we turned it off. Then move and size all the widgets.
  if (mCachingWidgetChanges == 0)
    ProcessWidgetChanges(mRootView);
#endif

  return NS_OK;
}

NS_IMETHODIMP
nsViewManager::AllowDoubleBuffering(PRBool aDoubleBuffer)
{
  mAllowDoubleBuffering = aDoubleBuffer;
  return NS_OK;
}

NS_IMETHODIMP
nsViewManager::IsPainting(PRBool& aIsPainting)
{
  aIsPainting = mPainting;
  return NS_OK;
}

NS_IMETHODIMP
nsViewManager::FlushPendingInvalidates()
{
  if (mHasPendingInvalidates) {
    ProcessPendingUpdates(mRootView);
    mHasPendingInvalidates = PR_FALSE;
  }
  return NS_OK;
}

nsresult
nsViewManager::ProcessInvalidateEvent() {
  FlushPendingInvalidates();
  mPendingInvalidateEvent = PR_FALSE;
  return NS_OK;
}

nsresult
nsViewManager::ProcessWidgetChanges(nsView* aView)
{
  //printf("---------Begin Sync----------\n");
  nsresult rv = aView->SynchWidgetSizePosition();
  if (NS_FAILED(rv))
    return rv;

  nsView *child = aView->GetFirstChild();
  while (nsnull != child) {
    rv = ProcessWidgetChanges(child);
    if (NS_FAILED(rv))
      return rv;

    child = child->GetNextSibling();
  }

  //printf("---------End Sync----------\n");

  return NS_OK;
}

NS_IMETHODIMP
nsViewManager::SetDefaultBackgroundColor(nscolor aColor)
{
  mDefaultBackgroundColor = aColor;
  return NS_OK;
}

NS_IMETHODIMP
nsViewManager::GetDefaultBackgroundColor(nscolor* aColor)
{
  *aColor = mDefaultBackgroundColor;
  return NS_OK;
}


NS_IMETHODIMP
nsViewManager::GetLastUserEventTime(PRUint32& aTime)
{
  aTime = gLastUserEventTime;
  return NS_OK;
}
