/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: true; c-basic-offset: 4 -*-
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
 * Contributor(s):  Patrick C. Beard <beard@netscape.com>
 *                  Kevin McCluskey  <kmcclusk@netscape.com>
 */

#include "nsViewManager2.h"
#include "nsUnitConversion.h"
#include "nsIRenderingContext.h"
#include "nsIDeviceContext.h"
#include "nsGfxCIID.h"
#include "nsIScrollableView.h"
#include "nsView.h"
#include "nsIScrollbar.h"
#include "nsIClipView.h"
#include "nsISupportsArray.h"
#include "nsICompositeListener.h"
#include "nsCOMPtr.h"

static NS_DEFINE_IID(kBlenderCID, NS_BLENDER_CID);
static NS_DEFINE_IID(kRegionCID, NS_REGION_CID);
static NS_DEFINE_IID(kRenderingContextCID, NS_RENDERING_CONTEXT_CID);

#define UPDATE_QUANTUM  1000 / 40


//#define NO_DOUBLE_BUFFER

// if defined widget changes like moves and resizes are defered until and done
// all in one pass.
//#define CACHE_WIDGET_CHANGES

// display list flags
#define VIEW_RENDERED		0x00000001
#define PUSH_CLIP			0x00000002
#define POP_CLIP			0x00000004
#define VIEW_TRANSPARENT	0x00000008
#define VIEW_TRANSLUCENT	0x00000010
#define VIEW_CLIPPED 	 0x00000020

// Uncomment the following to use the nsIBlender. Turned off for now,
// so that we won't crash.
#define SUPPORT_TRANSLUCENT_VIEWS

// display list elements
struct DisplayListElement2 {
	nsIView*			mView;
	nsRect				mBounds;
	nsRect				mDirty;
	PRUint32			mFlags;
};

inline nscoord max(nscoord x, nscoord y) { return (x > y ? x : y); }
inline nscoord min(nscoord x, nscoord y) { return (x < y ? x : y); }

#ifdef NS_VM_PERF_METRICS
#include "nsITimeRecorder.h"
#endif

#ifdef NS_VIEWMANAGER_NEEDS_TIMER

static void vm_timer_callback(nsITimer *aTimer, void *aClosure)
{
	nsViewManager2 *vm = (nsViewManager2 *)aClosure;

  printf("ViewManager2 timer callback\n");

	//restart the timer
  
	if (vm->mTrueFrameRate == vm->mFrameRate)
		{
			PRUint32 fr = vm->mFrameRate;

			vm->mFrameRate = 0;
			vm->SetFrameRate(fr);
		}
	//printf("timer composite...\n");
#ifndef XP_MAC
	//XXX temporary: The Mac doesn't need the timer to repaint but
	// obviously this is not the good method to disable the thing.
	// It's that way for now because the proper solutions
	// (set UPDATE_QUANTUM to 0, or simply not create the timer)
	// don't work for now. We'll fix that and then disable the
	// Mac timers as we should.
	vm->Composite();
#endif
}
#endif

#if 0
static void blinkRect(nsIRenderingContext* context, const nsRect& r)
{
	context->InvertRect(r);
	::PR_Sleep(::PR_MillisecondsToInterval(100));
	context->InvertRect(r);
}
#endif

PRInt32 nsViewManager2::mVMCount = 0;
nsDrawingSurface nsViewManager2::mDrawingSurface = nsnull;
nsRect nsViewManager2::mDSBounds = nsRect(0, 0, 0, 0);

nsDrawingSurface nsViewManager2::gOffScreen = nsnull;
nsDrawingSurface nsViewManager2::gRed = nsnull;
nsDrawingSurface nsViewManager2::gBlue = nsnull;
nsSize nsViewManager2::gBlendSize = nsSize(0, 0);
nsSize nsViewManager2::gOffScreenSize = nsSize(0, 0);
nsSize nsViewManager2::gLargestRequestedSize = nsSize(0, 0);


// Weakly held references to all of the view managers
nsVoidArray* nsViewManager2::gViewManagers = nsnull;

static NS_DEFINE_IID(knsViewManagerIID, NS_IVIEWMANAGER_IID);

nsViewManager2::nsViewManager2()
{
	NS_INIT_REFCNT();

  if (mVMCount == 0) {
    //Create a vector to hold each view manager
    gViewManagers = new nsVoidArray;
  }
  gViewManagers->AppendElement(this);

	mVMCount++;
	// NOTE:  we use a zeroing operator new, so all data members are
	// assumed to be cleared here.
  mX = 0;
  mY = 0;
  mCachingWidgetChanges = 0;
  
}

nsViewManager2::~nsViewManager2()
{
#ifdef NS_VIEWMANAGER_NEEDS_TIMER
	if (nsnull != mTimer) {
		mTimer->Cancel();     //XXX this should not be necessary. MMP
		NS_RELEASE(mTimer);
	}
#endif

	NS_IF_RELEASE(mRootWindow);

	mRootScrollable = nsnull;

	NS_ASSERTION((mVMCount > 0), "underflow of viewmanagers");
	--mVMCount;

  PRBool removed = gViewManagers->RemoveElement(this);
  NS_ASSERTION(removed, "Viewmanager instance not was not in the global list of viewmanagers");

	if ((0 == mVMCount) &&
		((nsnull != mDrawingSurface) || (nsnull != gOffScreen) ||
		 (nsnull != gRed) || (nsnull != gBlue)))
		{
      delete gViewManagers;
      gViewManagers = nsnull;

			nsCOMPtr<nsIRenderingContext> rc;
			nsresult rv = nsComponentManager::CreateInstance(kRenderingContextCID, 
															 nsnull, 
															 NS_GET_IID(nsIRenderingContext), 
															 getter_AddRefs(rc));

			if (NS_OK == rv)
				{
					if (nsnull != mDrawingSurface)
						rc->DestroyDrawingSurface(mDrawingSurface);

					if (nsnull != gOffScreen)
						rc->DestroyDrawingSurface(gOffScreen);

					if (nsnull != gRed)
						rc->DestroyDrawingSurface(gRed);

					if (nsnull != gBlue)
						rc->DestroyDrawingSurface(gBlue);
				}
    
			mDrawingSurface = nsnull;
			gOffScreen = nsnull;
			gRed = nsnull;
			gBlue = nsnull;
			gBlendSize.SizeTo(0, 0);
			gOffScreenSize.SizeTo(0, 0);
		}

	mObserver = nsnull;
	mContext = nsnull;

	if (nsnull != mDisplayList)
		{
			PRInt32 count = mDisplayList->Count();
			for (PRInt32 index = 0; index < count; index++) {
				DisplayListElement2* element = (DisplayListElement2*) mDisplayList->ElementAt(index);
				if (element != nsnull)
					delete element;
			}

			delete mDisplayList;
			mDisplayList = nsnull;
		}
  
	if (nsnull != mTransRgn) {
		if (nsnull != mTransRects)
			mTransRgn->FreeRects(mTransRects);

		NS_RELEASE(mTransRgn);
	}

	NS_IF_RELEASE(mBlender);
	NS_IF_RELEASE(mOpaqueRgn);
	NS_IF_RELEASE(mTRgn);
	NS_IF_RELEASE(mRCRgn);

	NS_IF_RELEASE(mOffScreenCX);
	NS_IF_RELEASE(mRedCX);
	NS_IF_RELEASE(mBlueCX);

	if (nsnull != mCompositeListeners) {
		mCompositeListeners->Clear();
		NS_RELEASE(mCompositeListeners);
	}
}

NS_IMPL_QUERY_INTERFACE(nsViewManager2, knsViewManagerIID)

	NS_IMPL_ADDREF(nsViewManager2);

nsrefcnt nsViewManager2::Release(void)
{
	/* Note funny ordering of use of mRefCnt. We were seeing a problem
	   during the deletion of a view hierarchy where child views,
	   while being destroyed, referenced this view manager and caused
	   the Destroy part of this method to be re-entered. Waiting until
	   the destruction has finished to decrement the refcount
	   prevents that.
  */
	NS_LOG_RELEASE(this, mRefCnt - 1, "nsViewManager2");
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

static nsresult CreateRegion(nsIComponentManager* componentManager, nsIRegion* *result)
{
	*result = nsnull;
	nsIRegion* region = nsnull;
	nsresult rv = componentManager->CreateInstance(kRegionCID, nsnull, NS_GET_IID(nsIRegion), (void**)&region);
	if (NS_SUCCEEDED(rv)) {
		rv = region->Init();
		*result = region;
	}
	return rv;
}

// We don't hold a reference to the presentation context because it
// holds a reference to us.
NS_IMETHODIMP nsViewManager2::Init(nsIDeviceContext* aContext, nscoord aX, nscoord aY)
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

#ifdef NS_VIEWMANAGER_NEEDS_TIMER
	mTimer = nsnull;
#endif
	mFrameRate = 0;
	mTrueFrameRate = 0;
	mTransCnt = 0;

	rv = SetFrameRate(UPDATE_QUANTUM);

	mLastRefresh = PR_IntervalNow();

	mRefreshEnabled = PR_TRUE;

	mMouseGrabber = nsnull;
	mKeyGrabber = nsnull;

	// create regions
	nsIComponentManager* componentManager = nsnull;
	rv = NS_GetGlobalComponentManager(&componentManager);
	if (NS_SUCCEEDED(rv)) {
		rv = CreateRegion(componentManager, &mTransRgn);
		rv = CreateRegion(componentManager, &mOpaqueRgn);
		rv = CreateRegion(componentManager, &mTRgn);
		rv = CreateRegion(componentManager, &mRCRgn);
	}

  mX = aX;
  mY = aY;
  
	return rv;
}

NS_IMETHODIMP nsViewManager2::GetRootView(nsIView *&aView)
{
	aView = mRootView;
	return NS_OK;
}

NS_IMETHODIMP nsViewManager2 :: SetRootView(nsIView *aView, nsIWidget* aWidget)
{
	UpdateTransCnt(mRootView, aView);
	// Do NOT destroy the current root view. It's the caller's responsibility
	// to destroy it
	mRootView = aView;

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

NS_IMETHODIMP nsViewManager2::GetFrameRate(PRUint32 &aRate)
{
	aRate = mTrueFrameRate;
	return NS_OK;
}

NS_IMETHODIMP nsViewManager2::SetFrameRate(PRUint32 aFrameRate)
{
	nsresult  rv = NS_OK;

	if (aFrameRate != mFrameRate)
	{
#ifdef NS_VIEWMANAGER_NEEDS_TIMER
			if (nsnull != mTimer)
			{
					mTimer->Cancel();     //XXX this should not be necessary. MMP
					NS_RELEASE(mTimer);
			}
#endif

			mFrameRate = aFrameRate;
			mTrueFrameRate = aFrameRate;

			if (mFrameRate != 0)
			{
#ifdef NS_VIEWMANAGER_NEEDS_TIMER
					rv = NS_NewTimer(&mTimer);

					if (NS_OK == rv)
						mTimer->Init(vm_timer_callback, this, 1000 / mFrameRate);
#endif
			}
			
	}


	return rv;
}

NS_IMETHODIMP nsViewManager2::GetWindowDimensions(nscoord *width, nscoord *height)
{
	if (nsnull != mRootView)
		mRootView->GetDimensions(width, height);
	else
		{
			*width = 0;
			*height = 0;
		}
	return NS_OK;
}

NS_IMETHODIMP nsViewManager2::SetWindowDimensions(nscoord width, nscoord height)
{
	// Resize the root view
	if (nsnull != mRootView)
		mRootView->SetDimensions(width, height);

//printf("new dims: %d %d\n", width, height);
  // Inform the presentation shell that we've been resized
	if (nsnull != mObserver)
		mObserver->ResizeReflow(mRootView, width, height);
	//printf("reflow done\n");

	return NS_OK;
}

NS_IMETHODIMP nsViewManager2::ResetScrolling(void)
{
	if (nsnull != mRootScrollable)
		mRootScrollable->ComputeScrollOffsets(PR_TRUE);

	return NS_OK;
}

void nsViewManager2::Refresh(nsIView *aView, nsIRenderingContext *aContext, nsIRegion *region, PRUint32 aUpdateFlags)
{
	nsRect              wrect;
	nsCOMPtr<nsIRenderingContext> localcx;
	nsDrawingSurface    ds = nsnull;

	if (PR_FALSE == mRefreshEnabled)
		return;

#ifdef NS_VM_PERF_METRICS
  MOZ_TIMER_DEBUGLOG(("Reset nsViewManager::Refresh(region), this=%p\n", this));
  MOZ_TIMER_RESET(mWatch);

  MOZ_TIMER_DEBUGLOG(("Start: nsViewManager::Refresh(region)\n"));
  MOZ_TIMER_START(mWatch);
#endif

	NS_ASSERTION(!(PR_TRUE == mPainting), "recursive painting not permitted");

	mPainting = PR_TRUE;

	//printf("refreshing region...\n");
	//force double buffering because of non-opaque views?

	if (mTransCnt > 0)
		aUpdateFlags |= NS_VMREFRESH_DOUBLE_BUFFER;

#ifdef NO_DOUBLE_BUFFER
    aUpdateFlags &= ~NS_VMREFRESH_DOUBLE_BUFFER;
#endif

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
					listener->WillRefreshRegion(this, aView, aContext, region, aUpdateFlags);
				}
			}
		}
	}

	if (aUpdateFlags & NS_VMREFRESH_DOUBLE_BUFFER)
		{
			nsCOMPtr<nsIWidget> widget;
			aView->GetWidget(*getter_AddRefs(widget));
			widget->GetClientBounds(wrect);
			wrect.x = wrect.y = 0;
			ds = GetDrawingSurface(*localcx, wrect);
		}

	PRBool  result;
	nsRect  trect;

	if (nsnull != region)
		localcx->SetClipRegion(*region, nsClipCombine_kUnion, result);

	aView->GetBounds(trect);

	localcx->SetClipRect(trect, nsClipCombine_kIntersect, result);

	RenderViews(aView, *localcx, trect, result);

	if ((aUpdateFlags & NS_VMREFRESH_DOUBLE_BUFFER) && ds)
		localcx->CopyOffScreenBits(ds, wrect.x, wrect.y, wrect, NS_COPYBITS_USE_SOURCE_CLIP_REGION);

	// Subtract the area we just painted from the dirty region
	if ((nsnull != region) && !region->IsEmpty()) {
		nsRect  pixrect = trect;
		float   t2p;
		
		mContext->GetAppUnitsToDevUnits(t2p);
		
		pixrect.ScaleRoundIn(t2p);
		region->Subtract(pixrect.x, pixrect.y, pixrect.width, pixrect.height);
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
					listener->DidRefreshRegion(this, aView, aContext, region, aUpdateFlags);
				}
			}
		}
	}

#ifdef NS_VM_PERF_METRICS
  MOZ_TIMER_DEBUGLOG(("Stop: nsViewManager2::Refresh(region), this=%p\n", this));
  MOZ_TIMER_STOP(mWatch);
  MOZ_TIMER_LOG(("vm2 Paint time (this=%p): ", this));
  MOZ_TIMER_PRINT(mWatch);
#endif

}

void nsViewManager2::Refresh(nsIView *aView, nsIRenderingContext *aContext, const nsRect *rect, PRUint32 aUpdateFlags)
{
	nsRect              wrect, brect;
	nsCOMPtr<nsIRenderingContext> localcx;
	nsDrawingSurface    ds = nsnull;

	if (PR_FALSE == mRefreshEnabled)
		return;

#ifdef NS_VM_PERF_METRICS
  MOZ_TIMER_DEBUGLOG(("Reset nsViewManager::Refresh(region), this=%p\n", this));
  MOZ_TIMER_RESET(mWatch);

  MOZ_TIMER_DEBUGLOG(("Start: nsViewManager::Refresh(region)\n"));
  MOZ_TIMER_START(mWatch);
#endif

	NS_ASSERTION(!(PR_TRUE == mPainting), "recursive painting not permitted");

	mPainting = PR_TRUE;

	//force double buffering because of non-opaque views?

	//printf("refreshing rect... ");
	//stdout << *rect;
	//printf("\n");
	if (mTransCnt > 0)
		aUpdateFlags |= NS_VMREFRESH_DOUBLE_BUFFER;

#ifdef NO_DOUBLE_BUFFER
    aUpdateFlags &= ~NS_VMREFRESH_DOUBLE_BUFFER;
#endif

	if (nsnull == aContext)
		{
			localcx = getter_AddRefs(CreateRenderingContext(*aView));

			//couldn't get rendering context. this is ok if at startup
			if (nsnull == localcx) {
				mPainting = PR_FALSE;
				return;
			}
		} else {
			// plain assignment adds a ref
			localcx = aContext;
		}

	// notify the listeners.
	if (nsnull != mCompositeListeners) {
		PRUint32 listenerCount;
		if (NS_SUCCEEDED(mCompositeListeners->Count(&listenerCount))) {
			nsCOMPtr<nsICompositeListener> listener;
			for (PRUint32 i = 0; i < listenerCount; i++) {
				if (NS_SUCCEEDED(mCompositeListeners->QueryElementAt(i, NS_GET_IID(nsICompositeListener), getter_AddRefs(listener)))) {
					listener->WillRefreshRect(this, aView, aContext, rect, aUpdateFlags);
				}
			}
		}
	}

	if (aUpdateFlags & NS_VMREFRESH_DOUBLE_BUFFER)
		{
			nsCOMPtr<nsIWidget> widget;
			aView->GetWidget(*getter_AddRefs(widget));
			widget->GetClientBounds(wrect);
			brect = wrect;
			wrect.x = wrect.y = 0;
			ds = GetDrawingSurface(*localcx, wrect);
		}

	nsRect trect = *rect;

	PRBool  result;

	localcx->SetClipRect(trect, nsClipCombine_kReplace, result);

	RenderViews(aView, *localcx, trect, result);

	if ((aUpdateFlags & NS_VMREFRESH_DOUBLE_BUFFER) && ds)
		localcx->CopyOffScreenBits(ds, wrect.x, wrect.y, wrect, NS_COPYBITS_USE_SOURCE_CLIP_REGION);

#if 0
	// Subtract the area we just painted from the dirty region
	nsIRegion* dirtyRegion;
	aView->GetDirtyRegion(dirtyRegion);

	if ((nsnull != dirtyRegion) && !dirtyRegion->IsEmpty())
		{
			nsRect  pixrect = trect;
			float   t2p;

			mContext->GetAppUnitsToDevUnits(t2p);

			pixrect.ScaleRoundIn(t2p);
			dirtyRegion->Subtract(pixrect.x, pixrect.y, pixrect.width, pixrect.height);
			NS_RELEASE(dirtyRegion);
		}
#endif

	mLastRefresh = PR_IntervalNow();

	mPainting = PR_FALSE;

	// notify the listeners.
	if (nsnull != mCompositeListeners) {
		PRUint32 listenerCount;
		if (NS_SUCCEEDED(mCompositeListeners->Count(&listenerCount))) {
			nsCOMPtr<nsICompositeListener> listener;
			for (PRUint32 i = 0; i < listenerCount; i++) {
				if (NS_SUCCEEDED(mCompositeListeners->QueryElementAt(i, NS_GET_IID(nsICompositeListener), getter_AddRefs(listener)))) {
					listener->DidRefreshRect(this, aView, aContext, rect, aUpdateFlags);
				}
			}
		}
	}

#ifdef NS_VM_PERF_METRICS
  MOZ_TIMER_DEBUGLOG(("Stop: nsViewManager2::Refresh(region), this=%p\n", this));
  MOZ_TIMER_STOP(mWatch);
  MOZ_TIMER_LOG(("vm2 Paint time (this=%p): ", this));
  MOZ_TIMER_PRINT(mWatch);
#endif
}

void nsViewManager2::RenderViews(nsIView *aRootView, nsIRenderingContext& aRC, const nsRect& aRect, PRBool &aResult)
{
	PRBool isFloatingView = PR_FALSE;
	if (PR_FALSE && NS_SUCCEEDED(aRootView->GetFloating(isFloatingView)) && isFloatingView) {
		// floating views are rendered locally (and act globally).
		// Paint the view. The clipping rect was set above set don't clip again.
		aRootView->Paint(aRC, aRect, NS_VIEW_FLAG_CLIP_SET, aResult);
	} else {
		// otherwise, use display list to render non-floating views.
		PRBool clipEmpty;

		// compute this view's origin, and mark it and all parent views.
		nsPoint origin(0, 0);
		ComputeViewOffset(aRootView, &origin, 1);
		
		// create the display list.
		if (mDisplayList == nsnull) {
			mDisplayList = new nsVoidArray(8);
			NS_ASSERTION((mDisplayList != nsnull), "couldn't create display list.");
			if (mDisplayList == nsnull) {
				// not enough memory for a display list, punt and draw the view recursively.
				aRootView->Paint(aRC, aRect, NS_VIEW_FLAG_CLIP_SET, aResult);
				return;
			}
		}
		
		// initialize various counters.
		mDisplayListCount = 0;
		mOpaqueViewCount = 0;
		mTranslucentViewCount = 0;
		mTranslucentArea.SetRect(0, 0, 0, 0);
		mTranslucentSize.SizeTo(0, 0);
		
		CreateDisplayList(mRootView, &mDisplayListCount, origin.x, origin.y, aRootView, &aRect);
		
		// now, partition this display list into "front-to-back" bundles, and then draw each bundle
		// with successively more and more restrictive clipping.
		if (mOpaqueViewCount > 0)
			OptimizeDisplayList(aRect);

		// create blending buffers, if necessary.
		if (mTranslucentViewCount > 0) {
			nsresult rv = CreateBlendingBuffers(aRC);
			NS_ASSERTION((rv == NS_OK), "not enough memory to blend");
			if (NS_FAILED(rv)) {
				// fall back by just rendering with transparency.
				mTranslucentViewCount = 0;
				for (PRInt32 i = mDisplayListCount - 1; i>= 0; --i) {
					DisplayListElement2* element = NS_STATIC_CAST(DisplayListElement2*, mDisplayList->ElementAt(i));
					element->mFlags &= ~VIEW_TRANSLUCENT;
				}
			}

			// DEBUGGING:  fill in complete offscreen image in green, to see if we've got a blending bug.
			//mOffScreenCX->SetColor(NS_RGB(0, 255, 0));
			//mOffScreenCX->FillRect(nsRect(0, 0, gOffScreenSize.width, gOffScreenSize.height));
		}

		// draw all views in the display list, from back to front.
		for (PRInt32 i = mDisplayListCount - 1; i>= 0; --i) {
			DisplayListElement2* element = NS_STATIC_CAST(DisplayListElement2*, mDisplayList->ElementAt(i));
			if (element->mFlags & VIEW_RENDERED) {
				// typical case, just rendering a view.
				// RenderView(element->mView, aRC, aRect, element->mBounds, aResult);
        if (element->mFlags & VIEW_CLIPPED) {
            //Render the view using the clip rect set by it's ancestors
          aRC.PushState();
          aRC.SetClipRect(element->mBounds, nsClipCombine_kIntersect, clipEmpty);
          RenderDisplayListElement(element, aRC);
          aRC.PopState(clipEmpty);
        } else {
          RenderDisplayListElement(element, aRC);
        }
        
			} else {
				// special case, pushing or popping clipping.
				if (element->mFlags & PUSH_CLIP) {
					aRC.PushState();
					aRC.SetClipRect(element->mBounds, nsClipCombine_kIntersect, clipEmpty);
				} else {
					if (element->mFlags & POP_CLIP) {
						aRC.PopState(clipEmpty);
					}
				}
			}
		}
		
    	ComputeViewOffset(aRootView, nsnull, 0);
	}
}

void nsViewManager2::RenderView(nsIView *aView, nsIRenderingContext &aRC, const nsRect &aDamageRect, nsRect &aGlobalRect, PRBool &aResult)
{
	nsRect  drect;

	NS_ASSERTION((nsnull != aView), "no view");

	aRC.PushState();

	aRC.Translate(aGlobalRect.x, aGlobalRect.y);

	drect.IntersectRect(aDamageRect, aGlobalRect);

	drect.x -= aGlobalRect.x;
	drect.y -= aGlobalRect.y;

	// should use blender here if opacity < 1.0

	aView->Paint(aRC, drect, NS_VIEW_FLAG_JUST_PAINT, aResult);

	aRC.PopState(aResult);
}

void nsViewManager2::RenderDisplayListElement(DisplayListElement2* element, nsIRenderingContext &aRC)
{
	PRBool isTranslucent = (element->mFlags & VIEW_TRANSLUCENT) != 0, clipEmpty;
	if (!isTranslucent) {
		aRC.PushState();

		nscoord x = element->mBounds.x, y = element->mBounds.y;
		aRC.Translate(x, y);

		nsRect drect(element->mDirty.x - x, element->mDirty.y - y,
		             element->mDirty.width, element->mDirty.height);

		element->mView->Paint(aRC, drect, NS_VIEW_FLAG_JUST_PAINT, clipEmpty);
		
		aRC.PopState(clipEmpty);
	}

#if defined(SUPPORT_TRANSLUCENT_VIEWS)	
	if (mTranslucentViewCount > 0 && (isTranslucent || mTranslucentArea.Intersects(element->mDirty))) {
		// transluscency case. if this view is transluscent, have to use the nsIBlender, otherwise, just
		// render in the offscreen. when we reach the last transluscent view, then we flush the bits
		// to the onscreen rendering context.
		
		// compute the origin of the view, relative to the offscreen buffer, which has the
		// same dimensions as mTranslucentArea.
		nscoord viewX = element->mBounds.x - mTranslucentArea.x, viewY = element->mBounds.y - mTranslucentArea.y;

		nsRect damageRect(element->mDirty);
		damageRect.IntersectRect(damageRect, mTranslucentArea);
		damageRect.x -= element->mBounds.x, damageRect.y -= element->mBounds.y;
		
		if (element->mFlags & VIEW_TRANSLUCENT) {
			nsIView* view = element->mView;

			// paint the view twice, first in the red buffer, then the blue;
			// the blender will pick up the touched pixels only.
			mRedCX->SetColor(NS_RGB(255, 0, 0));
			mRedCX->FillRect(damageRect);
			PaintView(view, *mRedCX, 0, 0, damageRect);
			// DEBUGGING ONLY
			//aRC.CopyOffScreenBits(gRed, 0, 0, element->mDirty,
            //					  NS_COPYBITS_XFORM_DEST_VALUES | NS_COPYBITS_TO_BACK_BUFFER);

			mBlueCX->SetColor(NS_RGB(0, 0, 255));
			mBlueCX->FillRect(damageRect);
			PaintView(view, *mBlueCX, 0, 0, damageRect);
			// DEBUGGING ONLY
			//aRC.CopyOffScreenBits(gBlue, 0, 0, element->mDirty,
			//					  NS_COPYBITS_XFORM_DEST_VALUES | NS_COPYBITS_TO_BACK_BUFFER);
			//mOffScreenCX->CopyOffScreenBits(gBlue, 0, 0, nsRect(viewX, viewY, damageRect.width, damageRect.height),
			//					  NS_COPYBITS_XFORM_DEST_VALUES | NS_COPYBITS_TO_BACK_BUFFER);

			float opacity;
			view->GetOpacity(opacity);

			// perform the blend itself.
			damageRect *= mTwipsToPixels;
			if (damageRect.width > 0 && damageRect.height > 0) {
    			mBlender->Blend(damageRect.x, damageRect.y, damageRect.width, damageRect.height,
    							mRedCX, mOffScreenCX,
    							(PRInt32)(viewX * mTwipsToPixels),(PRInt32)(viewY * mTwipsToPixels),
    							opacity, mBlueCX,
    							NS_RGB(255, 0, 0), NS_RGB(0, 0, 255));
    		}
			
			--mTranslucentViewCount;
		} else {
			PaintView(element->mView, *mOffScreenCX, viewX, viewY, damageRect);
		}
		
		// flush the bits back to screen.
		if (mTranslucentViewCount == 0) {
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
	}
#endif
}

void nsViewManager2::PaintView(nsIView *aView, nsIRenderingContext &aRC, nscoord x, nscoord y,
							   const nsRect &aDamageRect)
{
	aRC.PushState();
	aRC.Translate(x, y);
	PRBool unused;
	aView->Paint(aRC, aDamageRect, NS_VIEW_FLAG_JUST_PAINT, unused);
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

nsresult nsViewManager2::CreateBlendingBuffers(nsIRenderingContext &aRC)
{
	nsresult rv;

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
    	if (nsnull != gOffScreen) {
    		aRC.DestroyDrawingSurface(gOffScreen);
    		gOffScreen = nsnull;
    	}

    	rv = aRC.CreateDrawingSurface(&offscreenBounds, NS_CREATEDRAWINGSURFACE_FOR_PIXEL_ACCESS, gOffScreen);
    	if (NS_FAILED(rv))
    		return rv;

		offscreenBounds.ScaleRoundIn(mPixelsToTwips);
        gOffScreenSize.width = offscreenBounds.width;
        gOffScreenSize.height = offscreenBounds.height;
    }

	// ensure that the global blending drawing surfaces are large enough.
	if (mTranslucentSize.width > gBlendSize.width || mTranslucentSize.height > gBlendSize.height) {
    	nsRect blenderBounds(0, 0, mTranslucentSize.width, mTranslucentSize.height);
    	blenderBounds.ScaleRoundOut(mTwipsToPixels);
    	blenderBounds.width = nextPowerOf2(blenderBounds.width);
    	blenderBounds.height = nextPowerOf2(blenderBounds.height);
    	
    	NS_IF_RELEASE(mRedCX);
    	NS_IF_RELEASE(mBlueCX);

    	if (nsnull != gRed) {
    		aRC.DestroyDrawingSurface(gRed);
    		gRed = nsnull;
    	}
    	aRC.CreateDrawingSurface(&blenderBounds, NS_CREATEDRAWINGSURFACE_FOR_PIXEL_ACCESS, gRed);
    	if (NS_FAILED(rv))
    		return rv;

    	if (nsnull != gBlue) {
    		aRC.DestroyDrawingSurface(gBlue);
    		gBlue = nsnull;
    	}
    	aRC.CreateDrawingSurface(&blenderBounds, NS_CREATEDRAWINGSURFACE_FOR_PIXEL_ACCESS, gBlue);
    	if (NS_FAILED(rv))
    		return rv;

		blenderBounds.ScaleRoundIn(mPixelsToTwips);
        gBlendSize.width = blenderBounds.width;
        gBlendSize.height = blenderBounds.height;
    }

	// recreate local offscreen & blending contexts, if necessary.
	if (mOffScreenCX == nsnull) {
		rv = NewOffscreenContext(mContext, gOffScreen, gOffScreenSize, &mOffScreenCX);
    	if (NS_FAILED(rv))
    		return rv;
	}

	if (mRedCX == nsnull) {
		rv = NewOffscreenContext(mContext, gRed, gBlendSize, &mRedCX);
    	if (NS_FAILED(rv))
    		return rv;
	}
	if (mBlueCX == nsnull) {
		rv = NewOffscreenContext(mContext, gBlue, gBlendSize, &mBlueCX);
    	if (NS_FAILED(rv))
    		return rv;
	}

	return NS_OK;
}

void nsViewManager2::InvalidateChildWidgets(nsIView *aView, nsRect& aDirtyRect) const
{
	nsRect bounds;
	aView->GetBounds(bounds);

	// translate dirty rect into view coordinates.
	aDirtyRect.MoveBy(-bounds.x, -bounds.y);

	nsRect invalidRect(0, 0, bounds.width, bounds.height);
	invalidRect.IntersectRect(invalidRect, aDirtyRect);
	if (!invalidRect.IsEmpty()) {
		nsCOMPtr<nsIWidget> widget;
		aView->GetWidget(*getter_AddRefs(widget));
		if (nsnull != widget) {
			float scale;
			mContext->GetAppUnitsToDevUnits(scale);
			invalidRect.ScaleRoundOut(scale);

			//printf("invalidating: view %x (pix) %d, %d\n", aView, pixrect.width, pixrect.height);
			widget->Invalidate(invalidRect, PR_FALSE);
		}
	}

	// invalidate any child widgets that intersect this rectangle.
	nsIView *child;
	aView->GetChild(0, child);
	while (nsnull != child) {
		InvalidateChildWidgets(child, aDirtyRect);
		child->GetNextSibling(child);
	}
	
	// back the transformation out.
	aDirtyRect.MoveBy(bounds.x, bounds.y);
}

void nsViewManager2::ProcessPendingUpdates(nsIView* aView)
{
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
	nsIView* childView = nsnull;
	aView->GetChild(0, childView);
	while (nsnull != childView)	{
		ProcessPendingUpdates(childView);
		childView->GetNextSibling(childView);
	}
}

NS_IMETHODIMP nsViewManager2::Composite()
{
	if (mUpdateCnt > 0)
		{
			if (nsnull != mRootWindow)
				mRootWindow->Update();

			mUpdateCnt = 0;
			PauseTimer();
		}

	return NS_OK;
}

NS_IMETHODIMP nsViewManager2::UpdateView(nsIView *aView, PRUint32 aUpdateFlags)
{
	// Mark the entire view as damaged
	nsRect bounds;
	aView->GetBounds(bounds);
	bounds.x = bounds.y = 0;
	return UpdateView(aView, bounds, aUpdateFlags);
}

NS_IMETHODIMP nsViewManager2::UpdateView(nsIView *aView, const nsRect &aRect, PRUint32 aUpdateFlags)
{
	NS_PRECONDITION(nsnull != aView, "null view");

   // If the rectangle is not visible then abort
   // without invalidating. This is a performance 
   // enhancement since invalidating a native widget
   // can be expensive.
   // This also checks for silly request like aRect.width = 0 or aRect.height = 0
  if (! IsRectVisible(aView, aRect)) {
    return NS_OK;
  }

	if (!mRefreshEnabled) {
		// accumulate this rectangle in the view's dirty region, so we can process it later.
		AddRectToDirtyRegion(aView, aRect);
		++mUpdateCnt;
		return NS_OK;
	}


	// Find the nearest view (including this view) that has a widget
	nsIView *widgetView = GetWidgetView(aView);
	if (nsnull != widgetView) {
		if (0 == mUpdateCnt)
			RestartTimer();

		mUpdateCnt++;

#if 0
		// Transform damaged rect to widgetView's coordinate system.
		nsRect  widgetRect = aRect;
		nsIView *parentView = aView;
		while (parentView != widgetView) {
			nscoord x, y;
			parentView->GetPosition(&x, &y);
			widgetRect.x += x;
			widgetRect.y += y;
			parentView->GetParent(parentView);
		}

		// Add this rect to the widgetView's dirty region.
		if (nsnull != widgetView)
			InvalidateChildWidgets(widgetView, widgetRect);
#else
		// Go ahead and invalidate the entire rectangular area.
		// regardless of parentage.
		nsRect  widgetRect = aRect;
		ViewToWidget(aView, widgetView, widgetRect);
		nsCOMPtr<nsIWidget> widget;
		widgetView->GetWidget(*getter_AddRefs(widget));
		widget->Invalidate(widgetRect, PR_FALSE);

#if 0
		// invalidate all child views that could possibly intersect this view.
		nsIView *child;
		widgetView->GetChild(0, child);
		while (nsnull != child) {
			InvalidateChildWidgets(child, widgetRect);
			child->GetNextSibling(child);
		}
#endif

#endif

		// See if we should do an immediate refresh or wait
		if (aUpdateFlags & NS_VMREFRESH_IMMEDIATE) {
			Composite();
		} else if ((mTrueFrameRate > 0) && !(aUpdateFlags & NS_VMREFRESH_NO_SYNC)) {
			// or if a sync paint is allowed and it's time for the compositor to
			// do a refresh
			PRInt32 deltams = PR_IntervalToMilliseconds(PR_IntervalNow() - mLastRefresh);
			if (deltams > (1000 / (PRInt32)mTrueFrameRate))
				Composite();
		}
	}

	return NS_OK;
}

NS_IMETHODIMP nsViewManager2::UpdateAllViews(PRUint32 aUpdateFlags)
{
	UpdateViews(mRootView, aUpdateFlags);
	return NS_OK;
}

void nsViewManager2::UpdateViews(nsIView *aView, PRUint32 aUpdateFlags)
{
	// update this view.
	UpdateView(aView, aUpdateFlags);

	// update all children as well.
	nsIView* childView = nsnull;
	aView->GetChild(0, childView);
	while (nsnull != childView)	{
		UpdateViews(childView, aUpdateFlags);
		childView->GetNextSibling(childView);
	}
}

NS_IMETHODIMP nsViewManager2::DispatchEvent(nsGUIEvent *aEvent, nsEventStatus *aStatus)
{
	*aStatus = nsEventStatus_eIgnore;

	switch(aEvent->message)
		{
		case NS_SIZE:
			{
				nsIView*      view = nsView::GetViewFor(aEvent->widget);

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
				nsIView *view = nsView::GetViewFor(aEvent->widget);

				if (nsnull != view)
					{
						// The rect is in device units, and it's in the coordinate space of its
						// associated window.
						nsRect  damrect = *((nsPaintEvent*)aEvent)->rect;

						float   p2t;
						mContext->GetDevUnitsToAppUnits(p2t);
						damrect.ScaleRoundOut(p2t);

						// Do an immediate refresh
						if (nsnull != mContext)
							{
								nsRect  viewrect;
								float   varea;

								// Check that there's actually something to paint
								view->GetBounds(viewrect);
								viewrect.x = viewrect.y = 0;
								varea = (float)viewrect.width * viewrect.height;

								if (varea > 0.0000001f)
									{
										// nsRect     arearect;
										PRUint32   updateFlags = 0;

										// Auto double buffering logic.
										// See if the paint region is greater than .25 the area of our view.
										// If so, enable double buffered painting.
             
										// XXX These two lines cause a lot of flicker for drag-over re-drawing - rods
										//arearect.IntersectRect(damrect, viewrect);
  
										//if ((((float)arearect.width * arearect.height) / varea) >  0.25f)
										// XXX rods
										updateFlags |= NS_VMREFRESH_DOUBLE_BUFFER;

										//printf("refreshing: view: %x, %d, %d, %d, %d\n", view, damrect.x, damrect.y, damrect.width, damrect.height);
										// Refresh the view
										Refresh(view, ((nsPaintEvent*)aEvent)->renderingContext, &damrect, updateFlags);
									}
							}

						*aStatus = nsEventStatus_eConsumeNoDefault;
					}

				break;
			}

		case NS_DESTROY:
			*aStatus = nsEventStatus_eConsumeNoDefault;
			break;

		default:
			{
				nsIView* baseView;
				nsIView* view;
				nsPoint offset;
				nsIScrollbar* sb;

				//Find the view whose coordinates system we're in.
				baseView = nsView::GetViewFor(aEvent->widget);
        
				//Find the view to which we're initially going to send the event 
				//for hittesting.
				if (nsnull != mMouseGrabber && NS_IS_MOUSE_EVENT(aEvent)) {
					view = mMouseGrabber;
				}
				else if (nsnull != mKeyGrabber && NS_IS_KEY_EVENT(aEvent)) {
					view = mKeyGrabber;
				}
				else if (NS_OK == aEvent->widget->QueryInterface(NS_GET_IID(nsIScrollbar), (void**)&sb)) {
					view = baseView;
					NS_RELEASE(sb);
				}
				else {
					view = mRootView;
				}

				if (nsnull != view) {
					//Calculate the proper offset for the view we're going to
					offset.x = offset.y = 0;
					if (baseView != view) {
						//Get offset from root of baseView
						nsIView *parent;
						nsRect bounds;

						parent = baseView;
						while (nsnull != parent) {
							parent->GetBounds(bounds);
							offset.x += bounds.x;
							offset.y += bounds.y;
							parent->GetParent(parent);
						}

						//Subtract back offset from root of view
						parent = view;
						while (nsnull != parent) {
							parent->GetBounds(bounds);
							offset.x -= bounds.x;
							offset.y -= bounds.y;
							parent->GetParent(parent);
						}
      
					}

					//Dispatch the event
					float p2t, t2p;

					mContext->GetDevUnitsToAppUnits(p2t);
					mContext->GetAppUnitsToDevUnits(t2p);

					//Before we start mucking with coords, make sure we know our baseline
					aEvent->refPoint.x = aEvent->point.x;
					aEvent->refPoint.y = aEvent->point.y;

					aEvent->point.x = NSIntPixelsToTwips(aEvent->point.x, p2t);
					aEvent->point.y = NSIntPixelsToTwips(aEvent->point.y, p2t);

					aEvent->point.x += offset.x;
					aEvent->point.y += offset.y;
 
					PRBool handled = PR_FALSE;
					view->HandleEvent(aEvent, NS_VIEW_FLAG_CHECK_CHILDREN | 
									  NS_VIEW_FLAG_CHECK_PARENT |
									  NS_VIEW_FLAG_CHECK_SIBLINGS,
									  aStatus,
									  handled);

					aEvent->point.x -= offset.x;
					aEvent->point.y -= offset.y;

					aEvent->point.x = NSTwipsToIntPixels(aEvent->point.x, t2p);
					aEvent->point.y = NSTwipsToIntPixels(aEvent->point.y, t2p);

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

NS_IMETHODIMP nsViewManager2::GrabMouseEvents(nsIView *aView, PRBool &aResult)
{
#ifdef DEBUG_mjudge
  if (aView)
  {
    printf("capturing mouse events for view %x\n",aView);
  }
  printf("removing mouse capture from view %x\n",mMouseGrabber);
#endif

	mMouseGrabber = aView;
	aResult = PR_TRUE;
	return NS_OK;
}

NS_IMETHODIMP nsViewManager2::GrabKeyEvents(nsIView *aView, PRBool &aResult)
{
	mKeyGrabber = aView;
	aResult = PR_TRUE;
	return NS_OK;
}

NS_IMETHODIMP nsViewManager2::GetMouseEventGrabber(nsIView *&aView)
{
	aView = mMouseGrabber;
	return NS_OK;
}

NS_IMETHODIMP nsViewManager2::GetKeyEventGrabber(nsIView *&aView)
{
	aView = mKeyGrabber;
	return NS_OK;
}

NS_IMETHODIMP nsViewManager2::InsertChild(nsIView *parent, nsIView *child, nsIView *sibling,
										  PRBool above)
{
	NS_PRECONDITION(nsnull != parent, "null ptr");
	NS_PRECONDITION(nsnull != child, "null ptr");

	if ((nsnull != parent) && (nsnull != child))
		{
			nsIView *kid;
			nsIView *prev = nsnull;

			//verify that the sibling exists...

			parent->GetChild(0, kid);

			while (nsnull != kid)
				{
					if (kid == sibling)
						break;

					//get the next sibling view

					prev = kid;
					kid->GetNextSibling(kid);
				}

			if (nsnull != kid)
				{
					//it's there, so do the insertion

					if (PR_TRUE == above)
						parent->InsertChild(child, prev);
					else
						parent->InsertChild(child, sibling);
				}

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

NS_IMETHODIMP nsViewManager2::InsertChild(nsIView *parent, nsIView *child, PRInt32 zindex)
{
	NS_PRECONDITION(nsnull != parent, "null ptr");
	NS_PRECONDITION(nsnull != child, "null ptr");

	if ((nsnull != parent) && (nsnull != child))
		{
			nsIView *kid;
			nsIView *prev = nsnull;

			//find the right insertion point...

			parent->GetChild(0, kid);

			while (nsnull != kid)
				{
					PRInt32 idx;

					kid->GetZIndex(idx);

					if (zindex >= idx)
						break;

					//get the next sibling view

					prev = kid;
					kid->GetNextSibling(kid);
				}

			//in case this hasn't been set yet... maybe we should not do this? MMP

			child->SetZIndex(zindex);
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

NS_IMETHODIMP nsViewManager2::RemoveChild(nsIView *parent, nsIView *child)
{
	NS_PRECONDITION(nsnull != parent, "null ptr");
	NS_PRECONDITION(nsnull != child, "null ptr");

	if ((nsnull != parent) && (nsnull != child))
		{
			UpdateTransCnt(child, nsnull);
			UpdateView(child, NS_VMREFRESH_NO_SYNC);
			parent->RemoveChild(child);
		}

	return NS_OK;
}

NS_IMETHODIMP nsViewManager2::MoveViewBy(nsIView *aView, nscoord aX, nscoord aY)
{
	nscoord x, y;

	aView->GetPosition(&x, &y);
	MoveViewTo(aView, aX + x, aY + y);
	return NS_OK;
}

NS_IMETHODIMP nsViewManager2::MoveViewTo(nsIView *aView, nscoord aX, nscoord aY)
{
	nscoord oldX, oldY;
	aView->GetPosition(&oldX, &oldY);
	aView->SetPosition(aX, aY);

	// only do damage control if the view is visible

	if ((aX != oldX) || (aY != oldY)) {
		nsViewVisibility  visibility;
		aView->GetVisibility(visibility);
		if (visibility != nsViewVisibility_kHide) {
			nsRect  bounds;
			aView->GetBounds(bounds);
			nsRect oldArea(oldX, oldY, bounds.width, bounds.height);
			nsIView* parentView;
			aView->GetParent(parentView);
			UpdateView(parentView, oldArea, NS_VMREFRESH_NO_SYNC);
			nsRect newArea(aX, aY, bounds.width, bounds.height); 
			UpdateView(parentView, newArea, NS_VMREFRESH_NO_SYNC);
		}
	}
	return NS_OK;
}

NS_IMETHODIMP nsViewManager2::ResizeView(nsIView *aView, nscoord width, nscoord height)
{
	nscoord oldWidth, oldHeight;
	aView->GetDimensions(&oldWidth, &oldHeight);
	if ((width != oldWidth) || (height != oldHeight)) {
		nscoord x = 0, y = 0;
		nsIView* parentView = nsnull;
	    aView->GetParent(parentView);
	    if (parentView != nsnull)
			aView->GetPosition(&x, &y);
		else
			parentView = aView;

     // resize the view.
     nsViewVisibility  visibility;
     aView->GetVisibility(visibility);

     // Prevent Invalidation of hidden views 
     if (visibility == nsViewVisibility_kHide) {  
       aView->SetDimensions(width, height, PR_FALSE);
     } else {
		   aView->SetDimensions(width, height, PR_TRUE);
       nscoord maxWidth = (oldWidth < width ? width : oldWidth);
		   nscoord maxHeight = (oldHeight < height ? height : oldHeight);
		   nsRect boundingArea(x, y, maxWidth, maxHeight);
		   UpdateView(parentView, boundingArea, NS_VMREFRESH_NO_SYNC);
     }
	}
	
	return NS_OK;
}

NS_IMETHODIMP nsViewManager2::SetViewChildClip(nsIView *aView, nsRect *aRect)
{
	NS_ASSERTION(!(nsnull == aView), "no view");
	NS_ASSERTION(!(nsnull == aRect), "no clip");

	aView->SetChildClip(aRect->x, aRect->y, aRect->XMost(), aRect->YMost());

	UpdateView(aView, *aRect, NS_VMREFRESH_NO_SYNC);

	return NS_OK;
}

NS_IMETHODIMP nsViewManager2::SetViewVisibility(nsIView *aView, nsViewVisibility aVisible)
{
	nsViewVisibility  oldVisible;
	aView->GetVisibility(oldVisible);
	if (aVisible != oldVisible) {
		aView->SetVisibility(aVisible);
		if (nsViewVisibility_kHide == aVisible) {
			nsIView* parentView = nsnull;
			aView->GetParent(parentView);
			if (parentView) {
				nsRect  bounds;
				aView->GetBounds(bounds);
				UpdateView(parentView, bounds, NS_VMREFRESH_NO_SYNC);
			}
		}
		else {
			UpdateView(aView, NS_VMREFRESH_NO_SYNC);
		}
	}
	return NS_OK;
}

NS_IMETHODIMP nsViewManager2::SetViewZIndex(nsIView *aView, PRInt32 aZIndex)
{
	nsresult  rv = NS_OK;

	NS_ASSERTION((aView != nsnull), "no view");

#if 0
	// a little hack to check out a theory:  don't let floating views have any other z-index.
	PRBool isFloating = PR_FALSE;
	aView->GetFloating(isFloating);
	if (isFloating) {
		NS_ASSERTION((aZIndex == 0x7FFFFFFF), "floating view's z-index messed up");
		aZIndex = 0x7FFFFFFF;
	}
#endif

	PRInt32 oldidx;
	aView->GetZIndex(oldidx);

	if (oldidx != aZIndex) {
		nsIView *parent;
		aView->GetParent(parent);
		if (nsnull != parent) {
			//we don't just call the view manager's RemoveChild()
			//so that we can avoid two trips trough the UpdateView()
			//code (one for removal, one for insertion). MMP
			parent->RemoveChild(aView);
			UpdateTransCnt(aView, nsnull);
			rv = InsertChild(parent, aView, aZIndex);
		}
	}

	return rv;
}

NS_IMETHODIMP nsViewManager2::SetViewAutoZIndex(nsIView *aView, PRBool aAutoZIndex)
{
	return aView->SetAutoZIndex(aAutoZIndex);
}

NS_IMETHODIMP nsViewManager2::MoveViewAbove(nsIView *aView, nsIView *aOther)
{
	nsresult  rv;

	NS_ASSERTION(!(nsnull == aView), "no view");
	NS_ASSERTION(!(nsnull == aOther), "no view");

	nsIView *nextview;

	aView->GetNextSibling(nextview);
 
	if (nextview != aOther)
		{
			nsIView *parent;

			aView->GetParent(parent);

			if (nsnull != parent)
				{
					//we don't just call the view manager's RemoveChild()
					//so that we can avoid two trips trough the UpdateView()
					//code (one for removal, one for insertion). MMP

					parent->RemoveChild(aView);
					UpdateTransCnt(aView, nsnull);
					rv = InsertChild(parent, aView, aOther, PR_TRUE);
				}
			else
				rv = NS_OK;
		}
	else
		rv = NS_OK;

	return rv;
}

NS_IMETHODIMP nsViewManager2::MoveViewBelow(nsIView *aView, nsIView *aOther)
{
	nsresult  rv;

	NS_ASSERTION(!(nsnull == aView), "no view");
	NS_ASSERTION(!(nsnull == aOther), "no view");

	nsIView *nextview;

	aOther->GetNextSibling(nextview);
 
	if (nextview != aView)
		{
			nsIView *parent;

			aView->GetParent(parent);

			if (nsnull != parent)
				{
					//we don't just call the view manager's RemoveChild()
					//so that we can avoid two trips trough the UpdateView()
					//code (one for removal, one for insertion). MMP

					parent->RemoveChild(aView);
					UpdateTransCnt(aView, nsnull);
					rv = InsertChild(parent, aView, aOther, PR_FALSE);
				}
			else
				rv = NS_OK;
		}
	else
		rv = NS_OK;

	return rv;
}

NS_IMETHODIMP nsViewManager2::IsViewShown(nsIView *aView, PRBool &aResult)
{
	aResult = PR_TRUE;
	return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsViewManager2::GetViewClipAbsolute(nsIView *aView, nsRect *rect, PRBool &aResult)
{
	aResult = PR_TRUE;
	return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsViewManager2::SetViewContentTransparency(nsIView *aView, PRBool aTransparent)
{
	PRBool trans;

	aView->HasTransparency(trans);

	if (trans != aTransparent)
		{
			UpdateTransCnt(aView, nsnull);
			aView->SetContentTransparency(aTransparent);
			UpdateTransCnt(nsnull, aView);
			UpdateView(aView, NS_VMREFRESH_NO_SYNC);
		}

	return NS_OK;
}

NS_IMETHODIMP nsViewManager2::SetViewOpacity(nsIView *aView, float aOpacity)
{
	float opacity;

	aView->GetOpacity(opacity);

	if (opacity != aOpacity)
		{
			UpdateTransCnt(aView, nsnull);
			aView->SetOpacity(aOpacity);
			UpdateTransCnt(nsnull, aView);
			UpdateView(aView, NS_VMREFRESH_NO_SYNC);
		}

	return NS_OK;
}

NS_IMETHODIMP nsViewManager2::SetViewObserver(nsIViewObserver *aObserver)
{
	mObserver = aObserver;
	return NS_OK;
}

NS_IMETHODIMP nsViewManager2::GetViewObserver(nsIViewObserver *&aObserver)
{
	if (nsnull != mObserver) {
		aObserver = mObserver;
		NS_ADDREF(mObserver);
		return NS_OK;
	} else
		return NS_ERROR_NO_INTERFACE;
}

NS_IMETHODIMP nsViewManager2::GetDeviceContext(nsIDeviceContext *&aContext)
{
	NS_IF_ADDREF(mContext);
	aContext = mContext;
	return NS_OK;
}

void nsViewManager2::GetMaxWidgetBounds(nsRect& aMaxWidgetBounds) const
{
   // Go through the list of viewmanagers and get the maximum width and 
   // height of their widgets
  aMaxWidgetBounds.width = 0;
  aMaxWidgetBounds.height = 0;
  PRInt32 index = 0;
  for (index = 0; index < mVMCount; index++) {

    nsIViewManager* vm = (nsIViewManager*)gViewManagers->ElementAt(index);
    nsCOMPtr<nsIWidget> rootWidget;

    if(NS_SUCCEEDED(vm->GetWidget(getter_AddRefs(rootWidget))) && rootWidget)
    {
      nsRect widgetBounds;
      rootWidget->GetBounds(widgetBounds);
      aMaxWidgetBounds.width = max(aMaxWidgetBounds.width, widgetBounds.width);
      aMaxWidgetBounds.height = max(aMaxWidgetBounds.height, widgetBounds.height);
    }
  }

//   printf("WIDGET BOUNDS %d %d\n", aMaxWidgetBounds.width, aMaxWidgetBounds.height);
}

PRBool nsViewManager2::RectFitsInside(nsRect& aRect, PRInt32 aWidth, PRInt32 aHeight) const
{
  if (aRect.width > aWidth)
    return (PR_FALSE);

  if (aRect.height > aHeight)
    return (PR_FALSE);

  return PR_TRUE;
}

PRBool nsViewManager2::BothRectsFitInside(nsRect& aRect1, nsRect& aRect2, PRInt32 aWidth, PRInt32 aHeight, nsRect& aNewSize) const
{
  if (PR_FALSE == RectFitsInside(aRect1, aWidth, aHeight)) {
    return PR_FALSE;
  }

  if (PR_FALSE == RectFitsInside(aRect2, aWidth, aHeight)) {
    return PR_FALSE;
  }

  aNewSize.width = aWidth;
  aNewSize.height = aHeight;

  return PR_TRUE;
}


void nsViewManager2::CalculateDiscreteSurfaceSize(nsRect& aRequestedSize, nsRect& aSurfaceSize) const
{
  nsRect aMaxWidgetSize;
  GetMaxWidgetBounds(aMaxWidgetSize);
 
    // Get the height and width of the screen
	PRInt32 height;
  PRInt32 width;
  NS_ASSERTION(mContext != nsnull, "The device context is null");
  mContext->GetDeviceSurfaceDimensions(width, height);

	float devUnits;
 	mContext->GetDevUnitsToAppUnits(devUnits);
	PRInt32 screenHeight = NSToIntRound(float( height) / devUnits );
  PRInt32 screenWidth = NSToIntRound(float( width) / devUnits );

  // These tests must go from smallest rectangle to largest rectangle.

  // 1/8 screen
  if (BothRectsFitInside(aRequestedSize, aMaxWidgetSize, screenWidth / 8, screenHeight / 8, aSurfaceSize)) {
    return;
  }

  // 1/4 screen
  if (BothRectsFitInside(aRequestedSize, aMaxWidgetSize, screenWidth / 4, screenHeight / 4, aSurfaceSize)) {
    return;
  }

  // 1/2 screen
  if (BothRectsFitInside(aRequestedSize, aMaxWidgetSize, screenWidth / 2, screenHeight / 2, aSurfaceSize)) {
    return;
  }

  // 3/4 screen
  if (BothRectsFitInside(aRequestedSize, aMaxWidgetSize, (screenWidth * 3) / 4, (screenHeight * 3) / 4, aSurfaceSize)) {
    return;
  }

  // 3/4 screen width full screen height
  if (BothRectsFitInside(aRequestedSize, aMaxWidgetSize, (screenWidth * 3) / 4, screenHeight, aSurfaceSize)) {
    return;
  }

  // Full screen
  if (BothRectsFitInside(aRequestedSize, aMaxWidgetSize, screenWidth, screenHeight, aSurfaceSize)) {
    return;
  }

   // Bigger than Full Screen use the largest request every made.
  if (BothRectsFitInside(aRequestedSize, aMaxWidgetSize, gLargestRequestedSize.width, gLargestRequestedSize.height, aSurfaceSize)) {
    return;
  } else {
     gLargestRequestedSize.width = PR_MAX(aRequestedSize.width, aMaxWidgetSize.width);
     gLargestRequestedSize.height = PR_MAX(aRequestedSize.height, aMaxWidgetSize.height);
     aSurfaceSize.width = gLargestRequestedSize.width;
     aSurfaceSize.height = gLargestRequestedSize.height;
  //   printf("Expanding the largested requested size to %d %d\n", gLargestRequestedSize.width, gLargestRequestedSize.height);
  }
}

void nsViewManager2::GetDrawingSurfaceSize(nsRect& aRequestedSize, nsRect& aNewSize) const
{ 
  CalculateDiscreteSurfaceSize(aRequestedSize, aNewSize);
  aNewSize.MoveTo(aRequestedSize.x, aRequestedSize.y);
}


nsDrawingSurface nsViewManager2::GetDrawingSurface(nsIRenderingContext &aContext, nsRect& aBounds) 
{
  nsRect newBounds;
  GetDrawingSurfaceSize(aBounds, newBounds);

	if ((nsnull == mDrawingSurface)
		|| (mDSBounds.width != newBounds.width)
		|| (mDSBounds.height != newBounds.height))
		{
			if (mDrawingSurface) {
				//destroy existing DS
				aContext.DestroyDrawingSurface(mDrawingSurface);
				mDrawingSurface = nsnull;
			}

			nsresult rv = aContext.CreateDrawingSurface(&newBounds, 0, mDrawingSurface);
   //   printf("Allocating a new drawing surface %d %d\n", newBounds.width, newBounds.height);
			if (NS_SUCCEEDED(rv)) {
				mDSBounds = newBounds;
				aContext.SelectOffScreenDrawingSurface(mDrawingSurface);
			} else {
				mDSBounds.SetRect(0,0,0,0);
				mDrawingSurface = nsnull;
			}
		} else {
			aContext.SelectOffScreenDrawingSurface(mDrawingSurface);

			float p2t;
			mContext->GetDevUnitsToAppUnits(p2t);
			nsRect bounds = aBounds;
			bounds *= p2t;

			PRBool clipEmpty;
			aContext.SetClipRect(bounds, nsClipCombine_kReplace, clipEmpty);

      // This is not be needed. Only the part of the offscreen that has been
      // rendered to should be displayed so there no need to
      // clear it out.
			//nscolor col = NS_RGB(255,255,255);
			//aContext.SetColor(col);
			//aContext.FillRect(bounds);
		}

	return mDrawingSurface;
}

NS_IMETHODIMP nsViewManager2::ShowQuality(PRBool aShow)
{
	if (nsnull != mRootScrollable)
		mRootScrollable->ShowQuality(aShow);

	return NS_OK;
}

NS_IMETHODIMP nsViewManager2::GetShowQuality(PRBool &aResult)
{
	if (nsnull != mRootScrollable)
		mRootScrollable->GetShowQuality(aResult);

	return NS_OK;
}

NS_IMETHODIMP nsViewManager2::SetQuality(nsContentQuality aQuality)
{
	if (nsnull != mRootScrollable)
		mRootScrollable->SetQuality(aQuality);

	return NS_OK;
}

nsIRenderingContext * nsViewManager2::CreateRenderingContext(nsIView &aView)
{
	nsIView             *par = &aView;
	nsCOMPtr<nsIWidget> win;
	nsIRenderingContext *cx = nsnull;
	nscoord             x, y, ax = 0, ay = 0;

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
					par->GetPosition(&x, &y);

					ax += x;
					ay += y;
				}

			par->GetParent(par);
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

void nsViewManager2::AddRectToDirtyRegion(nsIView* aView, const nsRect &aRect) const
{
	// Find a view with an associated widget. We'll transform this rect from the
	// current view's coordinate system to a "heavyweight" parent view, then convert
	// the rect to pixel coordinates, and accumulate the rect into that view's dirty region.
	nsIView* widgetView = GetWidgetView(aView);
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

void nsViewManager2::UpdateTransCnt(nsIView *oldview, nsIView *newview)
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

NS_IMETHODIMP nsViewManager2::DisableRefresh(void)
{
	if (mUpdateBatchCnt > 0)
		return NS_OK;

	mRefreshEnabled = PR_FALSE;
	return NS_OK;
}

NS_IMETHODIMP nsViewManager2::EnableRefresh(PRUint32 aUpdateFlags)
{
	if (mUpdateBatchCnt > 0)
		return NS_OK;

	mRefreshEnabled = PR_TRUE;

	if (mUpdateCnt > 0)
		ProcessPendingUpdates(mRootView);

	if (aUpdateFlags & NS_VMREFRESH_IMMEDIATE) {
   
		if (mTrueFrameRate > 0)
			{
				PRInt32 deltams = PR_IntervalToMilliseconds(PR_IntervalNow() - mLastRefresh);

				if (deltams > (1000 / (PRInt32)mTrueFrameRate))
					Composite();
			}
	}


	return NS_OK;
}

NS_IMETHODIMP nsViewManager2::BeginUpdateViewBatch(void)
{
	nsresult result = NS_OK;
  
	if (mUpdateBatchCnt == 0)
		result = DisableRefresh();

	if (NS_SUCCEEDED(result))
		++mUpdateBatchCnt;

	return result;
}

NS_IMETHODIMP nsViewManager2::EndUpdateViewBatch(PRUint32 aUpdateFlags)
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

NS_IMETHODIMP nsViewManager2::SetRootScrollableView(nsIScrollableView *aScrollable)
{
	mRootScrollable = aScrollable;

	//XXX this needs to go away when layout start setting this bit on it's own. MMP
	if (mRootScrollable)
		mRootScrollable->SetScrollProperties(NS_SCROLL_PROPERTY_ALWAYS_BLIT);

	return NS_OK;
}

NS_IMETHODIMP nsViewManager2::GetRootScrollableView(nsIScrollableView **aScrollable)
{
	*aScrollable = mRootScrollable;
	return NS_OK;
}

NS_IMETHODIMP nsViewManager2::Display(nsIView* aView)
{
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

	aView->GetBounds(trect);
	nscoord x = trect.x, y = trect.y;

	// XXX Temporarily reset the position to (0, 0), that way when we paint
	// we won't end up translating incorrectly
	aView->SetPosition(0, 0);

	PRBool  result;

	trect.x = trect.y = 0;
	localcx->SetClipRect(trect, nsClipCombine_kReplace, result);

	// Paint the view. The clipping rect was set above set don't clip again.
	aView->Paint(*localcx, trect, NS_VIEW_FLAG_CLIP_SET, result);

	// XXX Reset the view's origin
	aView->SetPosition(x, y);

	NS_RELEASE(localcx);

	mPainting = PR_FALSE;

	return NS_OK;
}

NS_IMETHODIMP nsViewManager2::AddCompositeListener(nsICompositeListener* aListener)
{
	if (nsnull == mCompositeListeners) {
		nsresult rv = NS_NewISupportsArray(&mCompositeListeners);
		if (NS_FAILED(rv))
			return rv;
	}
	return mCompositeListeners->AppendElement(aListener);
}

NS_IMETHODIMP nsViewManager2::RemoveCompositeListener(nsICompositeListener* aListener)
{
	if (nsnull != mCompositeListeners) {
		return mCompositeListeners->RemoveElement(aListener);
	}
	return NS_ERROR_FAILURE;
}

NS_IMETHODIMP nsViewManager2::GetWidgetForView(nsIView *aView, nsIWidget **aWidget)
{
	*aWidget = nsnull;
	nsIView *view = aView;
	PRBool hasWidget = PR_FALSE;
	while (!hasWidget && view)
		{
			view->HasWidget(&hasWidget);
			if (!hasWidget)
				view->GetParent(view);
		}

	if (hasWidget) {
		// Widget was found in the view hierarchy
		view->GetWidget(*aWidget);
	} else {
		// No widget was found in the view hierachy, so use try to use the mRootWindow
		if (nsnull != mRootWindow) {
#ifdef NS_DEBUG
			nsCOMPtr<nsIViewManager> vm;
			nsCOMPtr<nsIViewManager> thisInstance(this);
			aView->GetViewManager(*getter_AddRefs(vm));
			NS_ASSERTION(thisInstance == vm, "Must use the view instances view manager when calling GetWidgetForView");
#endif
			*aWidget = mRootWindow;
			NS_ADDREF(mRootWindow);
		}
	}

	return NS_OK;
}


NS_IMETHODIMP nsViewManager2::GetWidget(nsIWidget **aWidget)
{
	NS_IF_ADDREF(mRootWindow);
	*aWidget = mRootWindow;
	return NS_OK;
}

NS_IMETHODIMP nsViewManager2::ForceUpdate()
{
	if (mRootWindow) {
		mRootWindow->Update();
	}
	return NS_OK;
}

NS_IMETHODIMP nsViewManager2::GetOffset(nscoord *aX, nscoord *aY)
{
  NS_ASSERTION(aX != nsnull, "aX pointer is null");
  NS_ASSERTION(aY != nsnull, "aY pointer is null");
  *aX = mX;
  *aY = mY;
  return NS_OK;
}


PRBool nsViewManager2::CreateDisplayList(nsIView *aView, PRInt32 *aIndex,
                                         nscoord aOriginX, nscoord aOriginY, nsIView *aRealView,
                                         const nsRect *aDamageRect, nsIView *aTopView,
                                         nscoord aX, nscoord aY)
{
	PRBool retval = PR_FALSE;

	NS_ASSERTION((aView != nsnull), "no view");
	NS_ASSERTION((aIndex != nsnull), "no index");

	if (!aTopView)
		aTopView = aView;

	nsRect bounds;
  aView->GetBounds(bounds);


	if (aView == aTopView) {
		bounds.x = 0;
		bounds.y = 0;
	}

	bounds.x += aX;
	bounds.y += aY;

	// is this a clip view?
	PRBool isClipView = IsClipView(aView);
	
	// is this view above the currently compositing root view?
	PRUint32 isParentView = 0;
	aView->GetCompositorFlags(&isParentView);

	// does the view have a widget?
	PRBool hasWidget = DoesViewHaveNativeWidget(aView);

	nsIView *childView = nsnull;
	PRInt32 childCount;
	aView->GetChildCount(childCount);
	if (childCount > 0) {
		if (isClipView && (!hasWidget || (hasWidget && isParentView))) {
			bounds.x -= aOriginX;
			bounds.y -= aOriginY;

			retval = AddToDisplayList(aIndex, aView, bounds, bounds, POP_CLIP, aX, aY);

			if (retval)
				return retval;

			bounds.x += aOriginX;
			bounds.y += aOriginY;
		}

		if (!hasWidget || (hasWidget && isParentView))
			//    if ((aView == aTopView) || (aView == aRealView))
			//    if ((aView == aTopView) || !hasWidget || (aView == aRealView))
			//    if ((aView == aTopView) || !(hasWidget && clipper) || (aView == aRealView))
			{
				for (aView->GetChild(0, childView); nsnull != childView; childView->GetNextSibling(childView)) {
					PRInt32 zindex;
					childView->GetZIndex(zindex);
					if (zindex < 0)
						break;
					retval = CreateDisplayList(childView, aIndex, aOriginX, aOriginY, aRealView, aDamageRect, aTopView, bounds.x, bounds.y);
					if (retval)
						break;
				}
			}
	}

	bounds.x -= aOriginX;
	bounds.y -= aOriginY;

	//  if (clipper)
	if (isClipView && (!hasWidget || (hasWidget && isParentView))) {
    if (childCount > 0) {
			retval = AddToDisplayList(aIndex, aView, bounds, bounds, PUSH_CLIP, aX, aY);
    }
	} else if (!retval)	{
		nsViewVisibility  visibility;
		float             opacity;
		PRBool            overlap;
		PRBool            transparent;
		nsRect            irect;

		aView->GetVisibility(visibility);
		aView->GetOpacity(opacity);
		aView->HasTransparency(transparent);

		if (aDamageRect)
			overlap = irect.IntersectRect(bounds, *aDamageRect);
		else
			overlap = PR_TRUE;

		if ((nsViewVisibility_kShow == visibility) && (opacity > 0.0f) && overlap) {
			PRUint32 flags = VIEW_RENDERED;
			if (transparent)
				flags |= VIEW_TRANSPARENT;
#if defined(SUPPORT_TRANSLUCENT_VIEWS)
			if (opacity < 1.0f)
				flags |= VIEW_TRANSLUCENT;
#endif
			retval = AddToDisplayList(aIndex, aView, bounds, irect, flags, aX, aY);

			if (retval || !transparent && (opacity == 1.0f) && (irect == *aDamageRect))
				retval = PR_TRUE;
		}

		// any children with negative z-indices?
		if (!retval && nsnull != childView) {
			bounds.x += aOriginX;
			bounds.y += aOriginY;
			for (; nsnull != childView; childView->GetNextSibling(childView)) {
				retval = CreateDisplayList(childView, aIndex, aOriginX, aOriginY, aRealView, aDamageRect, aTopView, bounds.x, bounds.y);
				if (retval)
					break;
			}
		}
	}

	return retval;
}

PRBool nsViewManager2::AddToDisplayList(PRInt32 *aIndex, nsIView *aView, nsRect &aClipRect, nsRect& aDirtyRect, PRUint32 aFlags,nscoord aAbsX, nscoord aAbsY)
{
  PRBool empty;
  PRBool clipped;
  nsRect clipRect;

	PRInt32 index = (*aIndex)++;
	DisplayListElement2* element = (DisplayListElement2*) mDisplayList->ElementAt(index);
	if (element == nsnull) {
		element = new DisplayListElement2;
		if (element == nsnull) {
			*aIndex = index;
			return PR_TRUE;
		}
		mDisplayList->ReplaceElementAt(element, index);
	}

  // Calculate absolute clipped position of view
  aView->GetClippedRect(clipRect, clipped, empty);  
  clipRect.x += aAbsX;
  clipRect.y += aAbsY;

	element->mView = aView;
	element->mDirty = aDirtyRect;
  if (clipped) { 
    // Use bounds calculated by above and indicate
    // it should be clipped
    element->mBounds = clipRect;
	  element->mFlags = aFlags | VIEW_CLIPPED;
  } else {
    // Use the bounds passed in
    element->mBounds = aClipRect;
	  element->mFlags = aFlags;
  }
	
	// count number of opaque views.
	if (aFlags == VIEW_RENDERED)
		++mOpaqueViewCount;

	// count number of transluscent views, and
	// accumulate a rectangle of all transluscent
	// views. this will be used to determine which
	// views need to be rendered into the blending
	// buffers.
	if (aFlags & VIEW_TRANSLUCENT) {
		if (mTranslucentViewCount++ == 0) {
			mTranslucentArea = aDirtyRect;
			mTranslucentSize.width = aDirtyRect.width;
			mTranslucentSize.height = aDirtyRect.height;
		} else {
			mTranslucentArea.UnionRect(mTranslucentArea, aDirtyRect);
			mTranslucentSize.width = max(mTranslucentSize.width, aDirtyRect.width);
			mTranslucentSize.height = max(mTranslucentSize.height, aDirtyRect.height);
		}
	}
	
	return PR_FALSE;
}

nsresult nsViewManager2::OptimizeDisplayList(const nsRect& aDamageRect)
{
	// walk the display list, looking for opaque views, and remove any views that are behind them and totally occluded.
	PRInt32 count = mDisplayListCount;
	PRInt32 opaqueCount = mOpaqueViewCount;
	for (PRInt32 i = 0; i < count; ++i) {
		DisplayListElement2* element = NS_STATIC_CAST(DisplayListElement2*, mDisplayList->ElementAt(i));
		if (element->mFlags & VIEW_RENDERED) {
			// a view is opaque if it is neither transparent nor transluscent
			if (!(element->mFlags & (VIEW_TRANSPARENT | VIEW_TRANSLUCENT))) {
				nsRect opaqueRect;
				opaqueRect.IntersectRect(element->mBounds, aDamageRect);
				nscoord top = opaqueRect.y, left = opaqueRect.x;
				nscoord bottom = top + opaqueRect.height, right = left + opaqueRect.width;
				// search for views behind this one, that are completely obscured by it.
				for (PRInt32 j = i + 1; j < count; ++j) {
					DisplayListElement2* lowerElement = NS_STATIC_CAST(DisplayListElement2*, mDisplayList->ElementAt(j));
					if (lowerElement->mFlags & VIEW_RENDERED) {
						nsRect lowerRect;
						lowerRect.IntersectRect(lowerElement->mBounds, aDamageRect);
						if (left <= lowerRect.x && top <= lowerRect.y &&
						    right >= (lowerRect.x + lowerRect.width) &&
						    bottom >= (lowerRect.y + lowerRect.height))
							{
								// remove this element from the display list, by clearing its VIEW_RENDERED flag.
								lowerElement->mFlags &= ~VIEW_RENDERED;
							}
					}
				}
			}
		}
		if (--opaqueCount == 0)
			break;
	}
	
	return NS_OK;
}

void nsViewManager2::ShowDisplayList(PRInt32 flatlen)
{
	char     nest[400];
	PRInt32  newnestcnt, nestcnt = 0, cnt;

	for (cnt = 0; cnt < 400; cnt++)
		nest[cnt] = ' ';

	float t2p;
	mContext->GetAppUnitsToDevUnits(t2p);

	printf("### display list length=%d ###\n", flatlen);

	for (cnt = 0; cnt < flatlen; cnt++) {
		nsIView   *view, *parent;
		nsRect    rect;
		PRUint32  flags;
		PRInt32   zindex;

		DisplayListElement2* element = (DisplayListElement2*) mDisplayList->ElementAt(cnt);
		view = element->mView;
		rect = element->mBounds;
		flags = element->mFlags;

		nest[nestcnt << 1] = 0;

		view->GetParent(parent);
		view->GetZIndex(zindex);
		rect *= t2p;
		printf("%snsIView@%p [z=%d, x=%d, y=%d, w=%d, h=%d, p=%p]\n",
			   nest, view, zindex,
			   rect.x, rect.y, rect.width, rect.height, parent);

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

void nsViewManager2::ComputeViewOffset(nsIView *aView, nsPoint *aOrigin, PRInt32 aFlag)
{
	while (aView != nsnull) {
		// Mark the view with specified flags.
		aView->SetCompositorFlags(aFlag);

		// compute the view's global position in the view hierarchy.
		if (aOrigin) {
			nsRect bounds;
			aView->GetBounds(bounds);
			aOrigin->x += bounds.x;
			aOrigin->y += bounds.y;
		}

		nsIView *parent;
		aView->GetParent(parent);
		aView = parent;
	}
}

PRBool nsViewManager2::DoesViewHaveNativeWidget(nsIView* aView)
{
	nsCOMPtr<nsIWidget> widget;
	aView->GetWidget(*getter_AddRefs(widget));
	if (nsnull != widget)
		return (nsnull != widget->GetNativeData(NS_NATIVE_WIDGET));
	return PR_FALSE;
}

PRBool nsViewManager2::IsClipView(nsIView* aView)
{
	nsIClipView *clipView = nsnull;
	nsresult rv = aView->QueryInterface(NS_GET_IID(nsIClipView), (void **)&clipView);
	return (rv == NS_OK && clipView != nsnull);
}

void nsViewManager2::PauseTimer(void)
{
	PRUint32 oldframerate = mTrueFrameRate;
	SetFrameRate(0);
	mTrueFrameRate = oldframerate;
}

void nsViewManager2::RestartTimer(void)
{
	SetFrameRate(mTrueFrameRate);
}

nsIView* nsViewManager2::GetWidgetView(nsIView *aView) const
{
	while (aView != nsnull) {
		PRBool hasWidget;
		aView->HasWidget(&hasWidget);
		if (hasWidget)
			return aView;
		aView->GetParent(aView);
	}
	return nsnull;
}

void nsViewManager2::ViewToWidget(nsIView *aView, nsIView* aWidgetView, nsRect &aRect) const
{
	while (aView != aWidgetView) {
		nscoord x, y;
		aView->GetPosition(&x, &y);
		aRect.MoveBy(x, y);
		aView->GetParent(aView);
	}
	
	// intersect aRect with bounds of aWidgetView, to prevent generating any illegal rectangles.
	nsRect bounds;
	aWidgetView->GetBounds(bounds);
	bounds.x = bounds.y = 0;
	aRect.IntersectRect(aRect, bounds);
	
	// finally, convert to device coordinates.
	float t2p;
	mContext->GetAppUnitsToDevUnits(t2p);
	aRect.ScaleRoundOut(t2p);
}

nsresult nsViewManager2::GetVisibleRect(nsRect& aVisibleRect)
{
  nsresult rv = NS_OK;

  // Get the viewport scroller
  nsIScrollableView* scrollingView;
  GetRootScrollableView(&scrollingView);

  if (scrollingView) {     
    // Determine the visible rect in the scrolled view's coordinate space.
    // The size of the visible area is the clip view size
    const nsIView*  clipView;

    scrollingView->GetScrollPosition(aVisibleRect.x, aVisibleRect.y);
    scrollingView->GetClipView(&clipView);
    clipView->GetDimensions(&aVisibleRect.width, &aVisibleRect.height);
  } else {
    rv = NS_ERROR_FAILURE;
  }

  return rv;
}

nsresult nsViewManager2::GetAbsoluteRect(nsIView *aView, const nsRect &aRect, 
                                         nsRect& aAbsRect)
{
  nsIScrollableView* scrollingView = nsnull;
  nsIView* scrolledView = nsnull;
  GetRootScrollableView(&scrollingView);
  if (nsnull == scrollingView) { 
    return NS_ERROR_FAILURE;
  }

  scrollingView->GetScrolledView(scrolledView);

   // Calculate the absolute coordinates of the aRect passed in.
   // aRects values are relative to aView
  aAbsRect = aRect;
  nsIView *parentView = aView;
  while ((parentView != nsnull) && (parentView != scrolledView)) {
    nscoord x, y;
    parentView->GetPosition(&x, &y);
    aAbsRect.MoveBy(x, y);
    parentView->GetParent(parentView);
  }

  if (parentView != scrolledView) {
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}


PRBool nsViewManager2::IsRectVisible(nsIView *aView, const nsRect &aRect)
{
  if (aRect.width == 0 || aRect.height == 0) {
    return PR_FALSE;
  }

	// is this view even visible?
	nsViewVisibility  visibility;
	aView->GetVisibility(visibility);
  if (visibility == nsViewVisibility_kHide) {
		return PR_FALSE; 
  }

   // Calculate the absolute coordinates for the visible rectangle   
  nsRect visibleRect;
  if (GetVisibleRect(visibleRect) == NS_ERROR_FAILURE) {
    return PR_TRUE;
  }

   // Calculate the absolute coordinates of the aRect passed in.
   // aRects values are relative to aView
  nsRect absRect;
  if ((GetAbsoluteRect(aView, aRect, absRect)) == NS_ERROR_FAILURE) {
    return PR_TRUE;
  }
 
    // Compare the visible rect against the rect passed in.
  PRBool overlaps = absRect.IntersectRect(absRect, visibleRect);

#if 0
  // Debugging code
  static int toggle = 0;
  for (int i = 0; i < toggle; i++) {
    printf(" ");
  }
  if (toggle == 10) {
    toggle = 0;
  } else {
   toggle++;
  }
  printf("***overlaps %d\n", overlaps);
#endif

  return overlaps;
}


NS_IMETHODIMP
nsViewManager2::IsCachingWidgetChanges(PRBool* aCaching)
{
#ifdef CACHE_WIDGET_CHANGES
  *aCaching = (mCachingWidgetChanges > 0);
#else
  *aCaching = PR_FALSE;
#endif

  return NS_OK;
}

NS_IMETHODIMP
nsViewManager2::CacheWidgetChanges(PRBool aCache)
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

nsresult
nsViewManager2::ProcessWidgetChanges(nsIView* aView)
{
  nsresult rv = aView->SynchWidgetSizePosition();
  if (NS_FAILED(rv))
      return rv;

	nsIView *child;
	aView->GetChild(0, child);
	while (nsnull != child) {
		rv = ProcessWidgetChanges(child);
    if (NS_FAILED(rv))
      return rv;

		child->GetNextSibling(child);
	}

  return NS_OK;
}


