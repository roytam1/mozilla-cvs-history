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

#include "nsImageBoxFrame.h"
#include "nsIDeviceContext.h"
#include "nsIFontMetrics.h"
#include "nsHTMLAtoms.h"
#include "nsXULAtoms.h"
#include "nsIStyleContext.h"
#include "nsStyleConsts.h"
#include "nsCOMPtr.h"
#include "nsIPresContext.h"
#include "nsButtonFrameRenderer.h"
#include "nsBoxLayoutState.h"

#include "nsHTMLParts.h"
#include "nsString.h"
#include "nsLeafFrame.h"
#include "nsIPresContext.h"
#include "nsIRenderingContext.h"
#include "nsIPresShell.h"
#include "nsHTMLIIDs.h"
#include "nsIImage.h"
#include "nsIWidget.h"
#include "nsHTMLAtoms.h"
#include "nsIHTMLAttributes.h"
#include "nsIDocument.h"
#include "nsIHTMLDocument.h"
#include "nsIStyleContext.h"
#include "nsStyleConsts.h"
#include "nsImageMap.h"
#include "nsILinkHandler.h"
#include "nsIURL.h"
#include "nsILoadGroup.h"
#include "nsIView.h"
#include "nsIViewManager.h"
#include "nsHTMLContainerFrame.h"
#include "prprf.h"
#include "nsISizeOfHandler.h"
#include "nsIFontMetrics.h"
#include "nsCSSRendering.h"
#include "nsIDOMHTMLImageElement.h"
#include "nsIDeviceContext.h"
#include "nsINameSpaceManager.h"
#include "nsTextFragment.h"
#include "nsIDOMHTMLMapElement.h"
#include "nsIStyleSet.h"
#include "nsIStyleContext.h"
#include "nsBoxLayoutState.h"

#include "nsIServiceManager.h"
#include "nsIURI.h"
#include "nsNetUtil.h"

#include "nsFormControlHelper.h"

#define ONLOAD_CALLED_TOO_EARLY 1

nsresult
nsImageBoxFrame::UpdateImageFrame(nsIPresContext* aPresContext,
                                      nsHTMLImageLoader* aLoader,
                                      nsIFrame* aFrame,
                                      void* aClosure,
                                      PRUint32 aStatus)
{
  if (NS_IMAGE_LOAD_STATUS_SIZE_AVAILABLE & aStatus) {
    nsImageBoxFrame* us = (nsImageBoxFrame*)aFrame;
    nsBoxLayoutState state(aPresContext);
    us->MarkDirty(state);
  }
  return NS_OK;
}

//
// NS_NewToolbarFrame
//
// Creates a new Toolbar frame and returns it in |aNewFrame|
//
nsresult
NS_NewImageBoxFrame ( nsIPresShell* aPresShell, nsIFrame** aNewFrame )
{
  NS_PRECONDITION(aNewFrame, "null OUT ptr");
  if (nsnull == aNewFrame) {
    return NS_ERROR_NULL_POINTER;
  }
  nsImageBoxFrame* it = new (aPresShell) nsImageBoxFrame (aPresShell);
  if (nsnull == it)
    return NS_ERROR_OUT_OF_MEMORY;

  *aNewFrame = it;
  return NS_OK;
  
} // NS_NewTitledButtonFrame

NS_IMETHODIMP
nsImageBoxFrame::AttributeChanged(nsIPresContext* aPresContext,
                               nsIContent* aChild,
                               PRInt32 aNameSpaceID,
                               nsIAtom* aAttribute,
                               PRInt32 aHint)
{
  PRBool aResize;
  PRBool aRedraw;
  UpdateAttributes(aPresContext, aAttribute, aResize, aRedraw);

  nsBoxLayoutState state(aPresContext);

  if (aResize) {
    MarkDirty(state);
  } else if (aRedraw) {
    Redraw(state);
  }

  return NS_OK;
}

#ifdef USE_IMG2
nsImageBoxFrame::nsImageBoxFrame(nsIPresShell* aShell):nsLeafBoxFrame(aShell), mIntrinsicSize(0,0)
#else
nsImageBoxFrame::nsImageBoxFrame(nsIPresShell* aShell):nsLeafBoxFrame(aShell)
#endif
{
  mSizeFrozen = PR_FALSE;
  mHasImage = PR_FALSE;
  NeedsRecalc();
}

nsImageBoxFrame::~nsImageBoxFrame()
{
}


NS_IMETHODIMP
nsImageBoxFrame::NeedsRecalc()
{
  SizeNeedsRecalc(mImageSize);
  return NS_OK;
}

NS_METHOD
nsImageBoxFrame::Destroy(nsIPresContext* aPresContext)
{
  // Release image loader first so that it's refcnt can go to zero
#ifdef USE_IMG2
  if (mImageRequest)
    mImageRequest->Cancel(NS_ERROR_FAILURE);

  if (mListener)
    NS_REINTERPRET_CAST(nsImageBoxListener*, mListener.get())->SetFrame(nsnull); // set the frame to null so we don't send messages to a dead object.

#else
  mImageLoader.StopAllLoadImages(aPresContext);
#endif

  return nsLeafBoxFrame::Destroy(aPresContext);
}


NS_IMETHODIMP
nsImageBoxFrame::Init(nsIPresContext*  aPresContext,
                          nsIContent*      aContent,
                          nsIFrame*        aParent,
                          nsIStyleContext* aContext,
                          nsIFrame*        aPrevInFlow)
{
#ifdef USE_IMG2
  if (!mListener) {
    nsImageBoxListener *listener;
    NS_NEWXPCOM(listener, nsImageBoxListener);
    NS_ADDREF(listener);
    listener->SetFrame(this);
    listener->QueryInterface(NS_GET_IID(imgIDecoderObserver), getter_AddRefs(mListener));
    NS_RELEASE(listener);
  }
#endif

  nsresult  rv = nsLeafBoxFrame::Init(aPresContext, aContent, aParent, aContext, aPrevInFlow);
  
  mHasImage = PR_FALSE;

  // Always set the image loader's base URL, because someone may
  // decide to change a button _without_ an image to have an image
  // later.
  nsCOMPtr<nsIURI> baseURL;
  GetBaseURI(getter_AddRefs(baseURL));

  // Initialize the image loader. Make sure the source is correct so
  // that UpdateAttributes doesn't double start an image load.
  nsAutoString src;
  GetImageSource(src);

#ifdef USE_IMG2

  PRBool needLoad = PR_FALSE;

  if (!src.IsEmpty()) {
    mHasImage = PR_TRUE;
    needLoad = PR_TRUE;
  }

  nsCOMPtr<nsIURI> srcURI;
  NS_NewURI(getter_AddRefs(srcURI), src, baseURL);

  // if we don't have an image request, then we certainly need to load
  if (!mImageRequest) {
    needLoad = PR_TRUE;
  } else {
    // check to see if the uri we are about to load is the same as the
    // one the image request came from.
    nsCOMPtr<nsIURI> requestURI;
    rv = mImageRequest->GetURI(getter_AddRefs(requestURI));
    NS_ASSERTION(NS_SUCCEEDED(rv) && requestURI,"no request URI");
    if (NS_FAILED(rv) || !requestURI) return NS_ERROR_UNEXPECTED;

    PRBool eq;
    requestURI->Equals(srcURI, &eq);

    if (eq) {
      needLoad = PR_FALSE;
    } else {
      needLoad = PR_TRUE;
      mImageRequest->Cancel(NS_ERROR_FAILURE);
      mImageRequest = nsnull;
    }
  }

  if (mHasImage && needLoad) {
    nsCOMPtr<imgILoader> il(do_GetService("@mozilla.org/image/loader;1", &rv));
    if (NS_FAILED(rv))
      return rv;

    nsCOMPtr<nsILoadGroup> loadGroup;
    GetLoadGroup(aPresContext, getter_AddRefs(loadGroup));

    il->LoadImage(srcURI, loadGroup, mListener, aPresContext, getter_AddRefs(mImageRequest));
  }
#else
  if (!src.IsEmpty()) {
    mHasImage = PR_TRUE;
  }

  mImageLoader.Init(this, UpdateImageFrame, nsnull, baseURL, src);
#endif

  PRBool a,b;
  UpdateAttributes(aPresContext, nsnull, a, b);

  return rv;
}

void
nsImageBoxFrame::GetImageSource(nsString& aResult)
{
  // get the new image src
  mContent->GetAttribute(kNameSpaceID_None, nsHTMLAtoms::src, aResult);

  // if the new image is empty
  if (aResult.IsEmpty()) {
    // get the list-style-image
    const nsStyleList* myList =
      (const nsStyleList*)mStyleContext->GetStyleData(eStyleStruct_List);
  
    if (myList->mListStyleImage.Length() > 0) {
      aResult = myList->mListStyleImage;
    }
  }
}

void
nsImageBoxFrame::UpdateAttributes(nsIPresContext*  aPresContext, nsIAtom* aAttribute, PRBool& aResize, PRBool& aRedraw)
{
  aResize = PR_FALSE;
  aRedraw = PR_FALSE;

  if (aAttribute == nsnull || aAttribute == nsHTMLAtoms::src) {
    UpdateImage(aPresContext, aResize);
  }
}

void
nsImageBoxFrame::UpdateImage(nsIPresContext*  aPresContext, PRBool& aResize)
{
  aResize = PR_FALSE;

#ifdef USE_IMG2

  // get the new image src
  nsAutoString src;
  GetImageSource(src);

  if (src.IsEmpty()) {
    mSizeFrozen = PR_TRUE;
    mHasImage = PR_FALSE;
    aResize = PR_TRUE;

    mImageRequest = nsnull;
    
    return;
  }

  nsCOMPtr<nsIURI> baseURI;
  GetBaseURI(getter_AddRefs(baseURI));
  nsCOMPtr<nsIURI> srcURI;
  NS_NewURI(getter_AddRefs(srcURI), src, baseURI);

  if (mImageRequest) {
    nsCOMPtr<nsIURI> requestURI;
    nsresult rv = mImageRequest->GetURI(getter_AddRefs(requestURI));
    NS_ASSERTION(NS_SUCCEEDED(rv) && requestURI,"no request URI");
    if (NS_FAILED(rv) || !requestURI) return;

    PRBool eq;
    requestURI->Equals(srcURI, &eq);
    // if the source uri and the current one are the same, return
    if (eq)
      return;
  }

  mSizeFrozen = PR_FALSE;
  mHasImage = PR_TRUE;

  // otherwise, we need to load the new uri
  if (mImageRequest) {
    mImageRequest->Cancel(NS_ERROR_FAILURE);
    mImageRequest = nsnull;
  }

  nsresult rv;
  nsCOMPtr<imgILoader> il(do_GetService("@mozilla.org/image/loader;1", &rv));

  nsCOMPtr<nsILoadGroup> loadGroup;
  GetLoadGroup(aPresContext, getter_AddRefs(loadGroup));

  il->LoadImage(srcURI, loadGroup, mListener, aPresContext, getter_AddRefs(mImageRequest));

  aResize = PR_TRUE;

#else
  // see if the source changed
  // get the old image src
  nsAutoString oldSrc;
  mImageLoader.GetURLSpec(oldSrc);

  // get the new image src
  nsAutoString src;
  GetImageSource(src);

   // see if the images are different
  if (!oldSrc.Equals(src)) {      

    if (!src.IsEmpty()) {
      mSizeFrozen = PR_FALSE;
      mHasImage = PR_TRUE;
    } else {
      mSizeFrozen = PR_TRUE;
      mHasImage = PR_FALSE;
    }

    mImageLoader.UpdateURLSpec(aPresContext, src);  

    aResize = PR_TRUE;
  }
#endif
}

NS_IMETHODIMP
nsImageBoxFrame::Paint(nsIPresContext* aPresContext,
                                nsIRenderingContext& aRenderingContext,
                                const nsRect& aDirtyRect,
                                nsFramePaintLayer aWhichLayer)
{	
	const nsStyleDisplay* disp = (const nsStyleDisplay*)
	mStyleContext->GetStyleData(eStyleStruct_Display);
	if (!disp->IsVisible())
		return NS_OK;

  nsresult rv = nsLeafBoxFrame::Paint(aPresContext, aRenderingContext, aDirtyRect, aWhichLayer);

  PaintImage(aPresContext, aRenderingContext, aDirtyRect, aWhichLayer);

  return rv;
}


NS_IMETHODIMP
nsImageBoxFrame::PaintImage(nsIPresContext* aPresContext,
                                nsIRenderingContext& aRenderingContext,
                                const nsRect& aDirtyRect,
                                nsFramePaintLayer aWhichLayer)
{
  if ((0 == mRect.width) || (0 == mRect.height)) {
    // Do not render when given a zero area. This avoids some useless
    // scaling work while we wait for our image dimensions to arrive
    // asynchronously.
    return NS_OK;
  }

  nsRect rect;
  GetClientRect(rect);

  // don't draw if the image is not dirty
  if (!mHasImage || !aDirtyRect.Intersects(rect))
    return NS_OK;

  if (NS_FRAME_PAINT_LAYER_FOREGROUND != aWhichLayer)
    return NS_OK;

#ifdef USE_IMG2
  nsCOMPtr<imgIContainer> imgCon;
#ifdef DEBUG_pavlov
  NS_ASSERTION(mImageRequest, "pav, fix me!");
#endif
  if (!mImageRequest) return NS_ERROR_UNEXPECTED;
  mImageRequest->GetImage(getter_AddRefs(imgCon));

  if (imgCon) {
    nsPoint p(rect.x, rect.y);
    aRenderingContext.DrawImage(imgCon, &rect, &p);
  }

#else
  nsCOMPtr<nsIImage> image ( dont_AddRef(mImageLoader.GetImage()) );
  if ( !image ) {
  }
  else {
    // Now render the image into our content area (the area inside the
    // borders and padding)
    aRenderingContext.DrawImage(image, rect);
  }
#endif

  return NS_OK;
}


//
// DidSetStyleContext
//
// When the style context changes, make sure that all of our image is up to date.
//
NS_IMETHODIMP
nsImageBoxFrame :: DidSetStyleContext( nsIPresContext* aPresContext )
{
  // if list-style-image change we want to change the image
  PRBool aResize;
  UpdateImage(aPresContext, aResize);
  
  return NS_OK;
  
} // DidSetStyleContext

void
nsImageBoxFrame::GetImageSize(nsIPresContext* aPresContext)
{
  nsSize s(0,0);
  nsHTMLReflowMetrics desiredSize(&s);
  const PRInt32 kDefaultSize = 0;
  float p2t;
  aPresContext->GetScaledPixelsToTwips(&p2t);
  const PRInt32 kDefaultSizeInTwips = NSIntPixelsToTwips(kDefaultSize, p2t);

// not calculated? Get the intrinsic size
	if (mHasImage) {
	  // get the size of the image and set the desired size
	  if (mSizeFrozen) {
			mImageSize.width = kDefaultSizeInTwips;
			mImageSize.height = kDefaultSizeInTwips;
      return;
	  } else {
      // Ask the image loader for the *intrinsic* image size
#ifdef USE_IMG2
      if (mIntrinsicSize.width > 0 && mIntrinsicSize.height > 0) {
        mImageSize.width = mIntrinsicSize.width;
        mImageSize.height = mIntrinsicSize.height;
        return;
      } else {
#else
      mImageLoader.GetDesiredSize(aPresContext, nsnull, desiredSize);
      if (desiredSize.width == 1 || desiredSize.height == 1)
      {
#endif
        mImageSize.width = kDefaultSizeInTwips;
        mImageSize.height = kDefaultSizeInTwips;
        return;
      }
	  }
	}

  mImageSize.width = desiredSize.width;
  mImageSize.height = desiredSize.height;
}


/**
 * Ok return our dimensions
 */
NS_IMETHODIMP
nsImageBoxFrame::DoLayout(nsBoxLayoutState& aState)
{
  return nsLeafBoxFrame::DoLayout(aState);
}

/**
 * Ok return our dimensions
 */
NS_IMETHODIMP
nsImageBoxFrame::GetPrefSize(nsBoxLayoutState& aState, nsSize& aSize)
{
  if (DoesNeedRecalc(mImageSize)) {
     CacheImageSize(aState);
  }

  aSize = mImageSize;
  AddBorderAndPadding(aSize);
  AddInset(aSize);
  nsIBox::AddCSSPrefSize(aState, this, aSize);

  return NS_OK;
}

/**
 * Ok return our dimensions
 */
NS_IMETHODIMP
nsImageBoxFrame::GetMinSize(nsBoxLayoutState& aState, nsSize& aSize)
{
  if (DoesNeedRecalc(mImageSize)) {
     CacheImageSize(aState);
  }

  aSize = mImageSize;
  AddBorderAndPadding(aSize);
  AddInset(aSize);
  nsIBox::AddCSSMinSize(aState, this, aSize);

  return NS_OK;
}

NS_IMETHODIMP
nsImageBoxFrame::GetAscent(nsBoxLayoutState& aState, nscoord& aCoord)
{
  nsSize size(0,0);
  GetPrefSize(aState, size);
  aCoord = size.height;
  return NS_OK;
}

/**
 * Ok return our dimensions
 */
void
nsImageBoxFrame::CacheImageSize(nsBoxLayoutState& aState)
{
  nsIPresContext* presContext = aState.GetPresContext();
  GetImageSize(presContext);
}

NS_IMETHODIMP
nsImageBoxFrame::GetFrameName(nsString& aResult) const
{
  aResult.AssignWithConversion("ImageBox");
  return NS_OK;
}



void
nsImageBoxFrame::GetBaseURI(nsIURI **uri)
{
  nsresult rv;
  nsCOMPtr<nsIURI> baseURI;
  nsCOMPtr<nsIHTMLContent> htmlContent(do_QueryInterface(mContent, &rv));
  if (NS_SUCCEEDED(rv)) {
    htmlContent->GetBaseURL(*getter_AddRefs(baseURI));
  }
  else {
    nsCOMPtr<nsIDocument> doc;
    rv = mContent->GetDocument(*getter_AddRefs(doc));
    if (NS_SUCCEEDED(rv)) {
      doc->GetBaseURL(*getter_AddRefs(baseURI));
    }
  }
  *uri = baseURI;
  NS_IF_ADDREF(*uri);
}

#ifdef USE_IMG2
void
nsImageBoxFrame::GetLoadGroup(nsIPresContext *aPresContext, nsILoadGroup **aLoadGroup)
{
  nsCOMPtr<nsIPresShell> shell;
  aPresContext->GetShell(getter_AddRefs(shell));

  if (!shell)
    return;

  nsCOMPtr<nsIDocument> doc;
  shell->GetDocument(getter_AddRefs(doc));
  if (!doc)
    return;

  doc->GetDocumentLoadGroup(aLoadGroup);
}


NS_IMETHODIMP nsImageBoxFrame::OnStartDecode(imgIRequest *request, nsIPresContext *aPresContext)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsImageBoxFrame::OnStartContainer(imgIRequest *request, nsIPresContext *aPresContext, imgIContainer *image)
{ 
  nsCOMPtr<nsIPresShell> presShell;
  aPresContext->GetShell(getter_AddRefs(presShell));

#ifdef DEBUG_pavlov
  NS_ENSURE_ARG(image);
#else
  if (!image) return NS_ERROR_INVALID_ARG; 
#endif  

  mHasImage = PR_TRUE;
  mSizeFrozen = PR_FALSE;

  nscoord w, h;
  image->GetWidth(&w);
  image->GetHeight(&h);

  float p2t;
  aPresContext->GetPixelsToTwips(&p2t);

  mIntrinsicSize.SizeTo(NSIntPixelsToTwips(w, p2t), NSIntPixelsToTwips(h, p2t));

  nsBoxLayoutState state(aPresContext);

  this->MarkDirty(state);

  return NS_OK;
}

NS_IMETHODIMP nsImageBoxFrame::OnStartFrame(imgIRequest *request, nsIPresContext *aPresContext, gfxIImageFrame *frame)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsImageBoxFrame::OnDataAvailable(imgIRequest *request, nsIPresContext *aPresContext, gfxIImageFrame *frame, const nsRect * rect)
{
  nsCOMPtr<nsIPresShell> presShell;
  aPresContext->GetShell(getter_AddRefs(presShell));

  nsBoxLayoutState state(aPresContext);
  this->Redraw(state);

#if 0

  // XXX we need to make sure that the reflow from the OnContainerStart has been
  // processed before we start calling invalidate

  float p2t;
  aPresContext->GetPixelsToTwips(&p2t);
  nsRect r(*rect);
  r *= p2t; // convert to twips

  Invalidate(aPresContext, nsRect(r.x, r.y, r.width, r.height), PR_FALSE);
#endif

  return NS_OK;
}

NS_IMETHODIMP nsImageBoxFrame::OnStopFrame(imgIRequest *request, nsIPresContext *aPresContext, gfxIImageFrame *frame)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsImageBoxFrame::OnStopContainer(imgIRequest *request, nsIPresContext *aPresContext, imgIContainer *image)
{

  return NS_OK;
}

NS_IMETHODIMP nsImageBoxFrame::OnStopDecode(imgIRequest *request, nsIPresContext *aPresContext, nsresult status, const PRUnichar *statusArg)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsImageBoxFrame::FrameChanged(imgIContainer *container, nsIPresContext *aPresContext, gfxIImageFrame *newframe, nsRect * dirtyRect)
{
  nsCOMPtr<nsIPresShell> presShell;
  aPresContext->GetShell(getter_AddRefs(presShell));

  nsBoxLayoutState state(aPresContext);
  this->Redraw(state);

  return NS_OK;
}
#endif



#ifdef USE_IMG2
NS_IMPL_ISUPPORTS2(nsImageBoxListener, imgIDecoderObserver, imgIContainerObserver)

nsImageBoxListener::nsImageBoxListener()
{
  NS_INIT_ISUPPORTS();
}

nsImageBoxListener::~nsImageBoxListener()
{
}

NS_IMETHODIMP nsImageBoxListener::OnStartDecode(imgIRequest *request, nsISupports *cx)
{
  if (!mFrame)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIPresContext> pc(do_QueryInterface(cx));
  return mFrame->OnStartDecode(request, pc);
}

NS_IMETHODIMP nsImageBoxListener::OnStartContainer(imgIRequest *request, nsISupports *cx, imgIContainer *image)
{
  if (!mFrame)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIPresContext> pc(do_QueryInterface(cx));
  return mFrame->OnStartContainer(request, pc, image);
}

NS_IMETHODIMP nsImageBoxListener::OnStartFrame(imgIRequest *request, nsISupports *cx, gfxIImageFrame *frame)
{
  if (!mFrame)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIPresContext> pc(do_QueryInterface(cx));
  return mFrame->OnStartFrame(request, pc, frame);
}

NS_IMETHODIMP nsImageBoxListener::OnDataAvailable(imgIRequest *request, nsISupports *cx, gfxIImageFrame *frame, const nsRect * rect)
{
  if (!mFrame)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIPresContext> pc(do_QueryInterface(cx));
  return mFrame->OnDataAvailable(request, pc, frame, rect);
}

NS_IMETHODIMP nsImageBoxListener::OnStopFrame(imgIRequest *request, nsISupports *cx, gfxIImageFrame *frame)
{
  if (!mFrame)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIPresContext> pc(do_QueryInterface(cx));
  return mFrame->OnStopFrame(request, pc, frame);
}

NS_IMETHODIMP nsImageBoxListener::OnStopContainer(imgIRequest *request, nsISupports *cx, imgIContainer *image)
{
  if (!mFrame)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIPresContext> pc(do_QueryInterface(cx));
  return mFrame->OnStopContainer(request, pc, image);
}

NS_IMETHODIMP nsImageBoxListener::OnStopDecode(imgIRequest *request, nsISupports *cx, nsresult status, const PRUnichar *statusArg)
{
  if (!mFrame)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIPresContext> pc(do_QueryInterface(cx));
  return mFrame->OnStopDecode(request, pc, status, statusArg);
}

NS_IMETHODIMP nsImageBoxListener::FrameChanged(imgIContainer *container, nsISupports *cx, gfxIImageFrame *newframe, nsRect * dirtyRect)
{
  if (!mFrame)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIPresContext> pc(do_QueryInterface(cx));
  return mFrame->FrameChanged(container, pc, newframe, dirtyRect);
}

#endif


