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
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is 
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
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
#include "nsStyleContext.h"
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
#include "nsIImage.h"
#include "nsIWidget.h"
#include "nsHTMLAtoms.h"
#include "nsIHTMLContent.h"
#include "nsIDocument.h"
#include "nsIHTMLDocument.h"
#include "nsStyleConsts.h"
#include "nsImageMap.h"
#include "nsILinkHandler.h"
#include "nsIURL.h"
#include "nsILoadGroup.h"
#include "nsIView.h"
#include "nsIViewManager.h"
#include "nsHTMLContainerFrame.h"
#include "prprf.h"
#include "nsIFontMetrics.h"
#include "nsCSSRendering.h"
#include "nsIDOMHTMLImageElement.h"
#include "nsIDeviceContext.h"
#include "nsINameSpaceManager.h"
#include "nsTextFragment.h"
#include "nsIDOMHTMLMapElement.h"
#include "nsIStyleSet.h"
#include "nsBoxLayoutState.h"
#include "nsIDOMDocument.h"
#include "nsIEventQueueService.h"
#include "nsTransform2D.h"
#include "nsITheme.h"

#include "nsIServiceManager.h"
#include "nsIURI.h"
#include "nsNetUtil.h"
#include "nsGUIEvent.h"

#include "nsFormControlHelper.h"

#define ONLOAD_CALLED_TOO_EARLY 1

static void PR_CALLBACK
HandleImagePLEvent(nsIContent *aContent, PRUint32 aMessage, PRUint32 aFlags)
{
  nsCOMPtr<nsIDOMNode> node(do_QueryInterface(aContent));

  if (!node) {
    NS_ERROR("null or non-DOM node passed to HandleImagePLEvent!");

    return;
  }

  nsCOMPtr<nsIDOMDocument> dom_doc;
  node->GetOwnerDocument(getter_AddRefs(dom_doc));

  nsCOMPtr<nsIDocument> doc(do_QueryInterface(dom_doc));

  if (!doc) {
    return;
  }

  nsCOMPtr<nsIPresShell> pres_shell;
  doc->GetShellAt(0, getter_AddRefs(pres_shell));

  if (!pres_shell) {
    return;
  }

  nsCOMPtr<nsIPresContext> pres_context;
  pres_shell->GetPresContext(getter_AddRefs(pres_context));

  if (!pres_context) {
    return;
  }

  nsEventStatus status = nsEventStatus_eIgnore;
  nsEvent event;
  event.eventStructType = NS_EVENT;
  event.message = aMessage;

  aContent->HandleDOMEvent(pres_context, &event, nsnull, aFlags, &status);
}

static void PR_CALLBACK
HandleImageOnloadPLEvent(PLEvent *aEvent)
{
  nsIContent *content = (nsIContent *)PL_GetEventOwner(aEvent);

  HandleImagePLEvent(content, NS_IMAGE_LOAD,
                     NS_EVENT_FLAG_INIT | NS_EVENT_FLAG_CANT_BUBBLE);

  NS_RELEASE(content);
}

static void PR_CALLBACK
HandleImageOnerrorPLEvent(PLEvent *aEvent)
{
  nsIContent *content = (nsIContent *)PL_GetEventOwner(aEvent);

  HandleImagePLEvent(content, NS_IMAGE_ERROR, NS_EVENT_FLAG_INIT);

  NS_RELEASE(content);
}

static void PR_CALLBACK
DestroyImagePLEvent(PLEvent* aEvent)
{
  delete aEvent;
}

// Fire off a PLEvent that'll asynchronously call the image elements
// onload handler once handled. This is needed since the image library
// can't decide if it wants to call it's observer methods
// synchronously or asynchronously. If an image is loaded from the
// cache the notifications come back synchronously, but if the image
// is loaded from the netswork the notifications come back
// asynchronously.

void
FireDOMEvent(nsIContent* aContent, PRUint32 aMessage)
{
  static NS_DEFINE_CID(kEventQueueServiceCID,   NS_EVENTQUEUESERVICE_CID);

  nsCOMPtr<nsIEventQueueService> event_service =
    do_GetService(kEventQueueServiceCID);

  if (!event_service) {
    NS_WARNING("Failed to get event queue service");

    return;
  }

  nsCOMPtr<nsIEventQueue> event_queue;

  event_service->GetThreadEventQueue(NS_CURRENT_THREAD,
                                     getter_AddRefs(event_queue));

  if (!event_queue) {
    NS_WARNING("Failed to get event queue from service");

    return;
  }

  PLEvent *event = new PLEvent;

  if (!event) {
    // Out of memory, but none of our callers care, so just warn and
    // don't fire the event

    NS_WARNING("Out of memory?");

    return;
  }

  PLHandleEventProc f;

  switch (aMessage) {
  case NS_IMAGE_LOAD :
    f = (PLHandleEventProc)::HandleImageOnloadPLEvent;

    break;
  case NS_IMAGE_ERROR :
    f = (PLHandleEventProc)::HandleImageOnerrorPLEvent;

    break;
  default:
    NS_WARNING("Huh, I don't know how to fire this type of event?!");

    return;
  }

  PL_InitEvent(event, aContent, f, (PLDestroyEventProc)::DestroyImagePLEvent);

  // The event owns the content pointer now.
  NS_ADDREF(aContent);

  event_queue->PostEvent(event);
}

//
// NS_NewImageBoxFrame
//
// Creates a new image frame and returns it in |aNewFrame|
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
                               PRInt32 aModType, 
                               PRInt32 aHint)
{
  nsresult rv = nsLeafBoxFrame::AttributeChanged(aPresContext, aChild, aNameSpaceID, aAttribute, aModType, aHint);

  PRBool aResize;
  PRBool aRedraw;
  UpdateAttributes(aPresContext, aAttribute, aResize, aRedraw);

  nsBoxLayoutState state(aPresContext);

  if (aResize) {
    MarkDirty(state);
  } else if (aRedraw) {
    Redraw(state);
  }

  return rv;
}

nsImageBoxFrame::nsImageBoxFrame(nsIPresShell* aShell) :
  nsLeafBoxFrame(aShell),
  mUseSrcAttr(PR_FALSE),
  mSizeFrozen(PR_FALSE),
  mHasImage(PR_FALSE),
  mSuppressStyleCheck(PR_FALSE),
  mIntrinsicSize(0,0),
  mLoadFlags(nsIRequest::LOAD_NORMAL),
  mPresContext(nsnull)
{
  NeedsRecalc();
}

nsImageBoxFrame::~nsImageBoxFrame()
{
  NS_PRECONDITION(!mPresContext, "Never got destroyed properly!");
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
  if (mImageRequest)
    mImageRequest->Cancel(NS_ERROR_FAILURE);

  if (mListener)
    NS_REINTERPRET_CAST(nsImageBoxListener*, mListener.get())->SetFrame(nsnull); // set the frame to null so we don't send messages to a dead object.

  // Clear out weak ptr to prescontext
  mPresContext = nsnull;
  
  return nsLeafBoxFrame::Destroy(aPresContext);
}


NS_IMETHODIMP
nsImageBoxFrame::Init(nsIPresContext*  aPresContext,
                          nsIContent*      aContent,
                          nsIFrame*        aParent,
                          nsStyleContext*  aContext,
                          nsIFrame*        aPrevInFlow)
{
  if (!mListener) {
    nsImageBoxListener *listener;
    NS_NEWXPCOM(listener, nsImageBoxListener);
    NS_ADDREF(listener);
    listener->SetFrame(this);
    listener->QueryInterface(NS_GET_IID(imgIDecoderObserver), getter_AddRefs(mListener));
    NS_RELEASE(listener);
  }

  mPresContext = aPresContext;

  mSuppressStyleCheck = PR_TRUE;
  nsresult  rv = nsLeafBoxFrame::Init(aPresContext, aContent, aParent, aContext, aPrevInFlow);
  mSuppressStyleCheck = PR_FALSE;

  GetImageSource();
  UpdateLoadFlags();

  PRBool aResize;
  UpdateImage(aPresContext, aResize);

  return rv;
}

void
nsImageBoxFrame::GetImageSource()
{
  // get the new image src
  mContent->GetAttr(kNameSpaceID_None, nsHTMLAtoms::src, mSrc);

  // if the new image is empty
  if (mSrc.IsEmpty()) {
    mUseSrcAttr = PR_FALSE;

    // Only get the list-style-image if we aren't being drawn
    // by a native theme.
    const nsStyleDisplay* disp = GetStyleDisplay();
    if (disp->mAppearance && nsBox::gTheme && 
        nsBox::gTheme->ThemeSupportsWidget(nsnull, this, disp->mAppearance))
      return;

    // get the list-style-image
    const nsStyleList* myList = GetStyleList();
  
    if (myList->mListStyleImage.Length() > 0) {
      mSrc = myList->mListStyleImage;
    }
  }
  else
    mUseSrcAttr = PR_TRUE;
}

void
nsImageBoxFrame::UpdateAttributes(nsIPresContext*  aPresContext, nsIAtom* aAttribute, PRBool& aResize, PRBool& aRedraw)
{
  aResize = PR_FALSE;
  aRedraw = PR_FALSE;

  if (aAttribute == nsnull || aAttribute == nsHTMLAtoms::src) {
    GetImageSource();
    UpdateImage(aPresContext, aResize);
  }
  else if (aAttribute == nsXULAtoms::validate)
    UpdateLoadFlags();
}

void
nsImageBoxFrame::UpdateLoadFlags()
{
  nsAutoString loadPolicy;
  mContent->GetAttr(kNameSpaceID_None, nsXULAtoms::validate, loadPolicy);
  if (loadPolicy.EqualsIgnoreCase("always"))
    mLoadFlags = nsIRequest::VALIDATE_ALWAYS;
  else if (loadPolicy.EqualsIgnoreCase("never"))
    mLoadFlags = nsIRequest::VALIDATE_NEVER|nsIRequest::LOAD_FROM_CACHE; 
  else
    mLoadFlags = nsIRequest::LOAD_NORMAL;
}

void
nsImageBoxFrame::UpdateImage(nsIPresContext*  aPresContext, PRBool& aResize)
{
  aResize = PR_FALSE;

  // get the new image src
  if (mSrc.IsEmpty()) {
    mSizeFrozen = PR_TRUE;
    mHasImage = PR_FALSE;
    aResize = PR_TRUE;

    if (mImageRequest) {
      mImageRequest->Cancel(NS_ERROR_FAILURE);
      mImageRequest = nsnull;
    }

    return;
  }

  nsCOMPtr<nsIURI> baseURI;
  GetBaseURI(getter_AddRefs(baseURI));
  nsCOMPtr<nsIURI> srcURI;
  NS_NewURI(getter_AddRefs(srcURI), mSrc, nsnull, baseURI);

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
  if (NS_FAILED(rv)) return;

  nsCOMPtr<nsILoadGroup> loadGroup;
  GetLoadGroup(aPresContext, getter_AddRefs(loadGroup));

  // Get the document URI for the referrer...
  nsCOMPtr<nsIURI> documentURI;
  nsCOMPtr<nsIDocument> doc;
  if (mContent) {
    (void) mContent->GetDocument(*getter_AddRefs(doc));
    if (doc) {
      doc->GetDocumentURL(getter_AddRefs(documentURI));
    }
  }

  // XXX: initialDocumentURI is NULL!
  il->LoadImage(srcURI, nsnull, documentURI, loadGroup, mListener, doc,
                mLoadFlags, nsnull, nsnull, getter_AddRefs(mImageRequest));

  aResize = PR_TRUE;
}

NS_IMETHODIMP
nsImageBoxFrame::Paint(nsIPresContext*      aPresContext,
                       nsIRenderingContext& aRenderingContext,
                       const nsRect&        aDirtyRect,
                       nsFramePaintLayer    aWhichLayer,
                       PRUint32             aFlags)
{	
  if (!GetStyleVisibility()->IsVisible())
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

  if (!mImageRequest) return NS_ERROR_UNEXPECTED;

  nsCOMPtr<imgIContainer> imgCon;
  mImageRequest->GetImage(getter_AddRefs(imgCon));

  if (imgCon) {
    PRBool hasSubRect = !mUseSrcAttr && (mSubRect.width > 0 || mSubRect.height > 0);
    PRBool sizeMatch = hasSubRect ? 
                       mSubRect.width == rect.width && mSubRect.height == rect.height :
                       mImageSize.width == rect.width && mImageSize.height == rect.height;

    if (sizeMatch) {
      nsPoint p(rect.x, rect.y);
        
      if (hasSubRect)
        rect = mSubRect;
      else {
        rect.x = 0;
        rect.y = 0;
      }

      // XXXdwh do dirty rect intersection like the HTML image frame does,
      // so that we don't always repaint the entire image!
      aRenderingContext.DrawImage(imgCon, &rect, &p);
    }
    else {
      nsRect src(0, 0, mImageSize.width, mImageSize.height);
      if (hasSubRect)
        src = mSubRect;
      aRenderingContext.DrawScaledImage(imgCon, &src, &rect);
    }
  }

  return NS_OK;
}


//
// DidSetStyleContext
//
// When the style context changes, make sure that all of our image is up to date.
//
NS_IMETHODIMP
nsImageBoxFrame::DidSetStyleContext( nsIPresContext* aPresContext )
{
  // Fetch our subrect.
  const nsStyleList* myList = GetStyleList();
  mSubRect = myList->mImageRegion;

  if (mUseSrcAttr || mSuppressStyleCheck)
    return NS_OK; // No more work required, since the image isn't specified by style.

  // If we're using a native theme implementation, we shouldn't draw anything.
  const nsStyleDisplay* disp = GetStyleDisplay();
  if (disp->mAppearance && nsBox::gTheme && 
      nsBox::gTheme->ThemeSupportsWidget(nsnull, this, disp->mAppearance))
    return NS_OK;

  // If list-style-image changes, we have a new image.
  nsAutoString newSrc;
  if (myList->mListStyleImage.Equals(mSrc))
    return NS_OK;

  mSrc = myList->mListStyleImage;

  PRBool aResize;
  UpdateImage(aPresContext, aResize);
  return NS_OK;
} // DidSetStyleContext

void
nsImageBoxFrame::GetImageSize(nsIPresContext* aPresContext)
{
  nsHTMLReflowMetrics desiredSize(PR_TRUE);
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
      if (mIntrinsicSize.width > 0 && mIntrinsicSize.height > 0) {
        mImageSize.width = mIntrinsicSize.width;
        mImageSize.height = mIntrinsicSize.height;
        return;
      } else {
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

  if (!mUseSrcAttr && (mSubRect.width > 0 || mSubRect.height > 0))
    aSize = nsSize(mSubRect.width, mSubRect.height);
  else
    aSize = mImageSize;
  AddBorderAndPadding(aSize);
  AddInset(aSize);
  nsIBox::AddCSSPrefSize(aState, this, aSize);

  nsSize minSize(0,0);
  nsSize maxSize(0,0);
  GetMinSize(aState, minSize);
  GetMaxSize(aState, maxSize);

  BoundsCheck(minSize, aSize, maxSize);

  return NS_OK;
}

NS_IMETHODIMP
nsImageBoxFrame::GetMinSize(nsBoxLayoutState& aState, nsSize& aSize)
{
  // An image can always scale down to (0,0).
  aSize.width = aSize.height = 0;
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

#ifdef DEBUG
NS_IMETHODIMP
nsImageBoxFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("ImageBox"), aResult);
}
#endif


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
    mContent->GetDocument(*getter_AddRefs(doc));
    if (doc) {
      doc->GetBaseURL(*getter_AddRefs(baseURI));
    }
  }
  *uri = baseURI;
  NS_IF_ADDREF(*uri);
}

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


NS_IMETHODIMP nsImageBoxFrame::OnStartDecode(imgIRequest *request)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsImageBoxFrame::OnStartContainer(imgIRequest *request,
                                                imgIContainer *image)
{
  NS_ENSURE_ARG_POINTER(image);

  // If we have no prescontext, what's going on?
  NS_ENSURE_TRUE(mPresContext, NS_ERROR_UNEXPECTED);

  mHasImage = PR_TRUE;
  mSizeFrozen = PR_FALSE;

  nscoord w, h;
  image->GetWidth(&w);
  image->GetHeight(&h);

  float p2t;
  mPresContext->GetPixelsToTwips(&p2t);

  mIntrinsicSize.SizeTo(NSIntPixelsToTwips(w, p2t), NSIntPixelsToTwips(h, p2t));

  nsBoxLayoutState state(mPresContext);

  this->MarkDirty(state);

  return NS_OK;
}

NS_IMETHODIMP nsImageBoxFrame::OnStartFrame(imgIRequest *request,
                                            gfxIImageFrame *frame)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsImageBoxFrame::OnDataAvailable(imgIRequest *request,
                                               gfxIImageFrame *frame,
                                               const nsRect * rect)
{
  return NS_OK;
}

NS_IMETHODIMP nsImageBoxFrame::OnStopFrame(imgIRequest *request,
                                           gfxIImageFrame *frame)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsImageBoxFrame::OnStopContainer(imgIRequest *request,
                                               imgIContainer *image)
{
  // If we have no prescontext, what's going on?
  NS_ENSURE_TRUE(mPresContext, NS_ERROR_UNEXPECTED);

  nsBoxLayoutState state(mPresContext);
  this->Redraw(state);

  return NS_OK;
}

NS_IMETHODIMP nsImageBoxFrame::OnStopDecode(imgIRequest *request,
                                            nsresult aStatus,
                                            const PRUnichar *statusArg)
{
  if (NS_SUCCEEDED(aStatus))
    // Fire an onerror DOM event.
    FireDOMEvent(mContent, NS_IMAGE_LOAD);
  else // Fire an onload DOM event.
    FireDOMEvent(mContent, NS_IMAGE_ERROR);

  return NS_OK;
}

NS_IMETHODIMP nsImageBoxFrame::FrameChanged(imgIContainer *container,
                                            gfxIImageFrame *newframe,
                                            nsRect * dirtyRect)
{
  // If we have no prescontext, what's going on?
  NS_ENSURE_TRUE(mPresContext, NS_ERROR_UNEXPECTED);

  nsBoxLayoutState state(mPresContext);
  this->Redraw(state);

  return NS_OK;
}

NS_IMPL_ISUPPORTS2(nsImageBoxListener, imgIDecoderObserver, imgIContainerObserver)

nsImageBoxListener::nsImageBoxListener()
{
}

nsImageBoxListener::~nsImageBoxListener()
{
}

NS_IMETHODIMP nsImageBoxListener::OnStartDecode(imgIRequest *request)
{
  if (!mFrame)
    return NS_ERROR_FAILURE;

  return mFrame->OnStartDecode(request);
}

NS_IMETHODIMP nsImageBoxListener::OnStartContainer(imgIRequest *request,
                                                   imgIContainer *image)
{
  if (!mFrame)
    return NS_ERROR_FAILURE;

  return mFrame->OnStartContainer(request, image);
}

NS_IMETHODIMP nsImageBoxListener::OnStartFrame(imgIRequest *request,
                                               gfxIImageFrame *frame)
{
  if (!mFrame)
    return NS_ERROR_FAILURE;

  return mFrame->OnStartFrame(request, frame);
}

NS_IMETHODIMP nsImageBoxListener::OnDataAvailable(imgIRequest *request,
                                                  gfxIImageFrame *frame,
                                                  const nsRect * rect)
{
  if (!mFrame)
    return NS_ERROR_FAILURE;

  return mFrame->OnDataAvailable(request, frame, rect);
}

NS_IMETHODIMP nsImageBoxListener::OnStopFrame(imgIRequest *request,
                                              gfxIImageFrame *frame)
{
  if (!mFrame)
    return NS_ERROR_FAILURE;

  return mFrame->OnStopFrame(request, frame);
}

NS_IMETHODIMP nsImageBoxListener::OnStopContainer(imgIRequest *request,
                                                  imgIContainer *image)
{
  if (!mFrame)
    return NS_ERROR_FAILURE;

  return mFrame->OnStopContainer(request, image);
}

NS_IMETHODIMP nsImageBoxListener::OnStopDecode(imgIRequest *request,
                                               nsresult status,
                                               const PRUnichar *statusArg)
{
  if (!mFrame)
    return NS_ERROR_FAILURE;

  return mFrame->OnStopDecode(request, status, statusArg);
}

NS_IMETHODIMP nsImageBoxListener::FrameChanged(imgIContainer *container,
                                               gfxIImageFrame *newframe,
                                               nsRect * dirtyRect)
{
  if (!mFrame)
    return NS_ERROR_FAILURE;

  return mFrame->FrameChanged(container, newframe, dirtyRect);
}

