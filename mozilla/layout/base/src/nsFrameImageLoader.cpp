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
#include "nsFrameImageLoader.h"
#include "nsIPresShell.h"
#include "nsIViewManager.h"
#include "nsIView.h"
#include "nsIFrame.h"
#include "nsIURL.h"
#include "nsIImageGroup.h"
#include "nsIImageRequest.h"
#include "nsIStyleContext.h"
#include "nsCOMPtr.h"
#include "nsIDeviceContext.h"
#include "nsXPIDLString.h"
#include "nslog.h"

#ifdef NOISY_IMAGE_LOADING
NS_IMPL_LOG_ENABLED(nsFrameImageLoaderLog)
#else
NS_IMPL_LOG(nsFrameImageLoaderLog)
#endif
#define PRINTF(args) NS_LOG_PRINTF(nsFrameImageLoaderLog, args)
#define FLUSH()      NS_LOG_FLUSH(nsFrameImageLoaderLog)

#ifdef DEBUG
#undef NOISY_IMAGE_LOADING
#else
#undef NOISY_IMAGE_LOADING
#endif

static NS_DEFINE_IID(kIFrameImageLoaderIID, NS_IFRAME_IMAGE_LOADER_IID);
static NS_DEFINE_IID(kIImageRequestObserverIID, NS_IIMAGEREQUESTOBSERVER_IID);
static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);

NS_LAYOUT nsresult
NS_NewFrameImageLoader(nsIFrameImageLoader** aResult)
{
  NS_PRECONDITION(nsnull != aResult, "null ptr");
  if (nsnull == aResult) {
    return NS_ERROR_NULL_POINTER;
  }
  nsFrameImageLoader* it = new nsFrameImageLoader();
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  return it->QueryInterface(kIFrameImageLoaderIID, (void**) aResult);
}

nsFrameImageLoader::nsFrameImageLoader()
  : mPresContext(nsnull),
    mImageRequest(nsnull),
    mImage(nsnull),
    mHaveBackgroundColor(PR_FALSE),
    mHaveDesiredSize(PR_FALSE),
    mBackgroundColor(0),
    mDesiredSize(0, 0),
    mImageLoadStatus(NS_IMAGE_LOAD_STATUS_NONE),
    mImageLoadError( nsImageError(-1) ),
    mNotifyLockCount(0),
    mFrames(nsnull),
    mCurNotifiedFrame(nsnull)
{
  NS_INIT_REFCNT();
}

nsFrameImageLoader::~nsFrameImageLoader()
{
  PerFrameData* pfd = mFrames;
  while (nsnull != pfd) {
    PerFrameData* next = pfd->mNext;
    delete pfd;
    pfd = next;
  }
  NS_IF_RELEASE(mImageRequest);
  NS_IF_RELEASE(mPresContext);
  NS_IF_RELEASE(mImage);
}

NS_IMPL_ADDREF(nsFrameImageLoader)
NS_IMPL_RELEASE(nsFrameImageLoader)

NS_IMETHODIMP
nsFrameImageLoader::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
  if (NULL == aInstancePtr) {
    return NS_ERROR_NULL_POINTER;
  }
  if (aIID.Equals(kIFrameImageLoaderIID)) {
    nsIFrameImageLoader* tmp = this;
    *aInstancePtr = (void*) tmp;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  if (aIID.Equals(kIImageRequestObserverIID)) {
    nsIImageRequestObserver* tmp = this;
    *aInstancePtr = (void*) tmp;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  if (aIID.Equals(kISupportsIID)) {
    nsIFrameImageLoader* tmp1 = this;
    nsISupports* tmp2 = tmp1;
    *aInstancePtr = (void*) tmp2;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  return NS_NOINTERFACE;
}

NS_IMETHODIMP
nsFrameImageLoader::Init(nsIPresContext* aPresContext,
                         nsIImageGroup* aGroup,
                         const nsString& aURL,
                         const nscolor* aBackgroundColor,
                         const nsSize* aDesiredSize,
                         nsIFrame* aTargetFrame,
                         nsImageAnimation aAnimationMode,
                         nsIFrameImageLoaderCB aCallBack,
                         void* aClosure,
                         void* aKey)
{
  NS_PRECONDITION(nsnull != aPresContext, "null ptr");
  if (nsnull == aPresContext) {
    return NS_ERROR_NULL_POINTER;
  }
  NS_PRECONDITION(nsnull == mPresContext, "double init");
  if (nsnull != mPresContext) {
    return NS_ERROR_ALREADY_INITIALIZED;
  }
  
  mPresContext = aPresContext;
  NS_IF_ADDREF(aPresContext);
  mURL = aURL;
  if (aBackgroundColor) {
    mHaveBackgroundColor = PR_TRUE;
    mBackgroundColor = *aBackgroundColor;
  }

  // Computed desired size, in pixels
  nscoord desiredWidth = 0;
  nscoord desiredHeight = 0;
  if (aDesiredSize) {
    mHaveDesiredSize = PR_TRUE;
    mDesiredSize = *aDesiredSize;

    float t2p,devScale;
    nsIDeviceContext  *theDC;

    aPresContext->GetTwipsToPixels(&t2p);
    aPresContext->GetDeviceContext(&theDC);
    theDC->GetCanonicalPixelScale(devScale);
    NS_RELEASE(theDC);
    
    desiredWidth = NSToCoordRound((mDesiredSize.width * t2p)/devScale);
    desiredHeight = NSToCoordRound((mDesiredSize.height * t2p)/devScale);
  }

  if (aTargetFrame || aCallBack) {
    PerFrameData* pfd = new PerFrameData;
    if (nsnull == pfd) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    pfd->mFrame = aTargetFrame;
    pfd->mKey = aKey;
    pfd->mCallBack = aCallBack;
    pfd->mClosure = aClosure;
    pfd->mNext = mFrames;
    pfd->mNeedSizeUpdate = aTargetFrame ? PR_TRUE : PR_FALSE;
    mFrames = pfd;
  }

  // Start image load request
  char* cp = aURL.ToNewCString();


  mImageRequest = aGroup->GetImage(cp, this, aBackgroundColor,
                                   desiredWidth, desiredHeight, 0);

  nsCRT::free(cp);

  return NS_OK;
}


NS_IMETHODIMP
nsFrameImageLoader::AddFrame(nsIFrame* aFrame,
                             nsIFrameImageLoaderCB aCallBack,
                             void* aClosure,
                             void* aKey)
{
  PerFrameData* pfd = mFrames;
  while (pfd) {
    if (pfd->mKey == aKey) {
      pfd->mCallBack = aCallBack;
      pfd->mClosure = aClosure;
      return NS_OK;
    }
    pfd = pfd->mNext;
  }

  pfd = new PerFrameData;
  if (nsnull == pfd) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  pfd->mFrame = aFrame;
  pfd->mKey = aKey;
  pfd->mCallBack = aCallBack;
  pfd->mClosure = aClosure;
  pfd->mNext = mFrames;
  pfd->mNeedSizeUpdate = PR_TRUE;
  mFrames = pfd;
  if (aCallBack && mPresContext &&
      ((NS_IMAGE_LOAD_STATUS_SIZE_AVAILABLE |
        NS_IMAGE_LOAD_STATUS_ERROR) & mImageLoadStatus)) {
    // Fire notification callback right away so that caller doesn't
    // miss it...
    PRINTF(("%p: AddFrame %p: notify frame=%p status=%x\n",
           this, pfd, pfd->mFrame, mImageLoadStatus));
    (*aCallBack)(mPresContext, this, pfd->mFrame, pfd->mClosure,
                 mImageLoadStatus);
    pfd->mNeedSizeUpdate = PR_FALSE;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsFrameImageLoader::RemoveFrame(void* aKey)
{
  PerFrameData** pfdp = &mFrames;
  PerFrameData* pfd;
  while (nsnull != (pfd = *pfdp)) {
    if (pfd->mKey == aKey) {
      *pfdp = pfd->mNext;
      if (pfd == mCurNotifiedFrame)
        mCurNotifiedFrame = pfd->mNext;
      delete pfd;
      return NS_OK;
    }
    pfdp = &pfd->mNext;
  }
  return NS_OK;
}

NS_IMETHODIMP
nsFrameImageLoader::StopImageLoad(PRBool aStopChrome)
{
  // don't stop chrome
  if (!aStopChrome) {
      if (mURL.EqualsWithConversion("chrome:", PR_TRUE, 7))
          return NS_ERROR_FAILURE;
  }

  nsCAutoString tmp; tmp.AssignWithConversion(mURL);
  PRINTF(("    %p: stopping %s\n", this, tmp.GetBuffer()));
  if (nsnull != mImageRequest) {
    mImageRequest->RemoveObserver(this);
    NS_RELEASE(mImageRequest);
  }
  NS_IF_RELEASE(mPresContext);
  return NS_OK;
}

NS_IMETHODIMP
nsFrameImageLoader::AbortImageLoad()
{
  if (nsnull != mImageRequest) {
    mImageRequest->Interrupt();
  }
  return NS_OK;
}

NS_IMETHODIMP
nsFrameImageLoader::IsSameImageRequest(const nsString& aURL,
                                       const nscolor* aBackgroundColor,
                                       const nsSize* aDesiredSize,
                                       PRBool* aResult)
{
  NS_PRECONDITION(nsnull != aResult, "null ptr");
  if (nsnull == aResult) {
    return NS_ERROR_NULL_POINTER;
  }

  // URL's must match (XXX: not quite right; need real comparison here...)
  if (!aURL.Equals(mURL)) {
    *aResult = PR_FALSE;
    return NS_OK;
  }

  // XXX Temporary fix to deal with the case of animated
  // images that are preloaded. If we've completed the image
  // and there aren't any frames associated with it, then we
  // interrupted the load of the image. The good things is that
  // the image is still in the image cache. The bad thing is that
  // we don't share the image loader.
  if ((nsnull == mFrames) && (mImageLoadStatus & NS_IMAGE_LOAD_STATUS_IMAGE_READY)) {
    *aResult = PR_FALSE;
    return NS_OK;
  }

  // Background colors must match
  if (aBackgroundColor) {
    if (!mHaveBackgroundColor) {
      *aResult = PR_FALSE;
      return NS_OK;
    }
    if (mBackgroundColor != *aBackgroundColor) {
      *aResult = PR_FALSE;
      return NS_OK;
    }
  }
  else if (mHaveBackgroundColor) {
    *aResult = PR_FALSE;
    return NS_OK;
  }

  // Desired sizes must match
  if (aDesiredSize) {
    if (!mHaveDesiredSize) {
      *aResult = PR_FALSE;
      return NS_OK;
    }
    if ((mDesiredSize.width != aDesiredSize->width) ||
        (mDesiredSize.height != aDesiredSize->height)) {
      *aResult = PR_FALSE;
      return NS_OK;
    }
  }
  else if (mHaveDesiredSize) {
    *aResult = PR_FALSE;
    return NS_OK;
  }

  *aResult = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP
nsFrameImageLoader::SafeToDestroy(PRBool* aResult)
{
  NS_PRECONDITION(nsnull != aResult, "null ptr");
  if (nsnull == aResult) {
    return NS_ERROR_NULL_POINTER;
  }

  *aResult = PR_FALSE;
  if ((nsnull == mFrames) && (0 == mNotifyLockCount)) {
    *aResult = PR_TRUE;
  }
  return NS_OK;
}

NS_IMETHODIMP
nsFrameImageLoader::GetURL(nsString& aResult)
{
  aResult = mURL;
  return NS_OK;
}

NS_IMETHODIMP
nsFrameImageLoader::GetPresContext(nsIPresContext** aPresContext)
{
  NS_IF_ADDREF(*aPresContext = mPresContext);
  return NS_OK;
}

NS_IMETHODIMP
nsFrameImageLoader::GetImage(nsIImage** aResult)
{
  NS_PRECONDITION(nsnull != aResult, "null ptr");
  if (nsnull == aResult) {
    return NS_ERROR_NULL_POINTER;
  }
  *aResult = mImage;
  NS_IF_ADDREF(mImage);
  return NS_OK;
}

// Return the size of the image, in twips
NS_IMETHODIMP
nsFrameImageLoader::GetSize(nsSize& aResult)
{
  aResult = mDesiredSize;
  return NS_OK;
}

NS_IMETHODIMP
nsFrameImageLoader::GetImageLoadStatus(PRUint32* aResult)
{
  NS_PRECONDITION(nsnull != aResult, "null ptr");
  if (nsnull == aResult) {
    return NS_ERROR_NULL_POINTER;
  }
  *aResult = mImageLoadStatus;
  return NS_OK;
}

NS_IMETHODIMP
nsFrameImageLoader::GetIntrinsicSize(nsSize& aResult)
{
  if (mImageRequest) {
    PRUint32  width = 0, height = 0;
    float     p2t;

    mPresContext->GetScaledPixelsToTwips(&p2t);
    aResult.width = NSIntPixelsToTwips(width, p2t);
    aResult.height = NSIntPixelsToTwips(height, p2t);
  } else {
    aResult.SizeTo(0, 0);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsFrameImageLoader::GetNaturalImageSize(PRUint32* naturalWidth, 
                                        PRUint32 *naturalHeight)
{
  if(mImage){  
    *naturalWidth = mImage->GetNaturalWidth();
    *naturalHeight = mImage->GetNaturalHeight();
  }else{
    *naturalWidth = 0;
    *naturalHeight = 0;
  }
  return NS_OK;
}

void
nsFrameImageLoader::Notify(nsIImageRequest *aImageRequest,
                           nsIImage *aImage,
                           nsImageNotification aNotificationType,
                           PRInt32 aParam1, PRInt32 aParam2,
                           void *aParam3)
{
  nsIView*      view;
  nsRect        damageRect;
  float         p2t;
  const nsRect* changeRect;
  PerFrameData* pfd;

  if (!mPresContext) {
    return;
  }

  mNotifyLockCount++;

  nsCAutoString tmp; tmp.AssignWithConversion(mURL);
  PRINTF(("%p: loading %s", this, tmp.GetBuffer()));
  PRINTF((" notification=%d params=%d,%d,%p\n", aNotificationType,
         aParam1, aParam2, aParam3));
  switch (aNotificationType) {
  case nsImageNotification_kDimensions:
    mPresContext->GetScaledPixelsToTwips(&p2t);
    mDesiredSize.width = NSIntPixelsToTwips(aParam1, p2t);
    mDesiredSize.height = NSIntPixelsToTwips(aParam2, p2t);
    mImageLoadStatus |= NS_IMAGE_LOAD_STATUS_SIZE_AVAILABLE;
    NotifyFrames(PR_TRUE);
    break;

  case nsImageNotification_kPixmapUpdate:
    if ((nsnull == mImage) && (nsnull != aImage)) {
      mImage = aImage;
      NS_ADDREF(aImage);
    }
    mImageLoadStatus |= NS_IMAGE_LOAD_STATUS_IMAGE_READY;

    // Convert the rect from pixels to twips
    mPresContext->GetScaledPixelsToTwips(&p2t);
    changeRect = (const nsRect*) aParam3;
    damageRect.x = NSIntPixelsToTwips(changeRect->x, p2t);
    damageRect.y = NSIntPixelsToTwips(changeRect->y, p2t);
    damageRect.width = NSIntPixelsToTwips(changeRect->width, p2t);
    damageRect.height = NSIntPixelsToTwips(changeRect->height, p2t);
    DamageRepairFrames(&damageRect);
    break;

  case nsImageNotification_kImageComplete:
    // XXX Temporary fix to deal with the case of animated
    // images that are preloaded. If there are no frames
    // registered, then stop loading this image.
    if (nsnull == mFrames) {
      AbortImageLoad();
    }
    if ((nsnull == mImage) && (nsnull != aImage)) {
      mImage = aImage;
      NS_ADDREF(aImage);
      mImageLoadStatus |= NS_IMAGE_LOAD_STATUS_IMAGE_READY;
      DamageRepairFrames(nsnull);
    }
    NotifyFrames(PR_FALSE);
    break;

  case nsImageNotification_kFrameComplete:
    // New frame of a GIF animation
    // XXX Image library sends this for all images, and not just for animated
    // images. You bastards. It's a waste to re-draw the image if it's not an
    // animated image, but unfortunately we don't currently have a way to tell
    // whether the image is animated
    if (mImage) {
      DamageRepairFrames(nsnull);
    }
    break;

  case nsImageNotification_kIsTransparent:
    // Mark the frame's view as having transparent areas
    // XXX this is pretty vile; change this so that we set another frame status bit and then pass on a notification *or* lets just start passing on the notifications directly to the frames and eliminate all of this code.
    pfd = mFrames;
    while (pfd) {
      if (pfd->mFrame) {
        pfd->mFrame->GetView(mPresContext, &view);
        if (view) {
          view->SetContentTransparency(PR_TRUE);
        }
      }
      pfd = pfd->mNext;
    }
    break;

  case nsImageNotification_kAborted:
    // Treat this like an error
    mImageLoadError = nsImageError_kNoData;
    mImageLoadStatus |= NS_IMAGE_LOAD_STATUS_ERROR;
    NotifyFrames(PR_FALSE);
    break;

  default:
    break;
  }

  mNotifyLockCount--;
}

void
nsFrameImageLoader::NotifyError(nsIImageRequest *aImageRequest,
                                nsImageError aErrorType)
{
  mNotifyLockCount++;

  mImageLoadError = aErrorType;
  mImageLoadStatus |= NS_IMAGE_LOAD_STATUS_ERROR;
  NotifyFrames(PR_FALSE);

  mNotifyLockCount--;
}

void
nsFrameImageLoader::NotifyFrames(PRBool aIsSizeUpdate)
{
  mCurNotifiedFrame = mFrames;
  PerFrameData* pfd; // Initialized on the next line
  while (nsnull != (pfd = mCurNotifiedFrame)) {
    if ((aIsSizeUpdate && pfd->mNeedSizeUpdate) || !aIsSizeUpdate) {
      if (pfd->mCallBack) {
        PRINTF(("  notify pfd = %p frame=%p status=%x\n", pfd, pfd->mFrame, mImageLoadStatus));
        (*pfd->mCallBack)(mPresContext, this, pfd->mFrame, pfd->mClosure,
                          mImageLoadStatus);
      }
      if (pfd != mCurNotifiedFrame) {
        continue; //pfd has been deleted in the callback, don't chain to next
      }
      if (aIsSizeUpdate) {
        pfd->mNeedSizeUpdate = PR_FALSE;
      }
    }
    mCurNotifiedFrame = pfd->mNext;
  }
  
}

void
nsFrameImageLoader::DamageRepairFrames(const nsRect* aDamageRect)
{
  // Determine damaged area and tell view manager to redraw it
  nsPoint offset;
  nsRect bounds;
  nsIView* view;

  PerFrameData* pfd = mFrames;
  while (nsnull != pfd) {
    nsIFrame* frame = pfd->mFrame;

    if (frame) {
      // NOTE: It is not sufficient to invalidate only the size of the image:
      //       the image may be tiled! 
      //       The best option is to call into the frame, however lacking this
      //       we have to at least invalidate the frame's bounds, hence
      //       as long as we have a frame we'll use its size.
      //
      // XXX - Add a NotifyImageLoaded to nsIFrame and call that, passing the 
      //       damage rect (image size)

      // Invalidate the entire frame
      // XXX We really only need to invalidate the client area of the frame...
      frame->GetRect(bounds);
      bounds.x = bounds.y = 0;

      // XXX We should tell the frame the damage area and let it invalidate
      // itself. Add some API calls to nsIFrame to allow a caller to invalidate
      // parts of the frame...
      frame->GetView(mPresContext, &view);
      if (nsnull == view) {
        frame->GetOffsetFromView(mPresContext, offset, &view);
        bounds.x += offset.x;
        bounds.y += offset.y;
      }

      nsCOMPtr<nsIViewManager> vm = nsnull;
      nsresult rv = NS_OK;
      rv = view->GetViewManager(*getter_AddRefs(vm));
      if (NS_SUCCEEDED(rv) && nsnull != vm) {
        vm->UpdateView(view, bounds, NS_VMREFRESH_NO_SYNC);    
      }
    }

    pfd = pfd->mNext;
  }
}
