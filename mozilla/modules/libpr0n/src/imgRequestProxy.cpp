/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
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
 * Copyright (C) 2001 Netscape Communications Corporation.
 * All Rights Reserved.
 * 
 * Contributor(s):
 *   Stuart Parmenter <pavlov@netscape.com>
 */

#include "imgRequestProxy.h"

#include "nsXPIDLString.h"

#include "nsIInputStream.h"
#include "imgILoader.h"
#include "nsIComponentManager.h"

#include "nsIComponentManager.h"
#include "nsIServiceManager.h"

#include "imgRequest.h"

#include "nsString.h"

#include "nsIChannel.h"

#include "nspr.h"

#include "ImageLogging.h"

NS_IMPL_ISUPPORTS3(imgRequestProxy, imgIRequest, imgIDecoderObserver, gfxIImageContainerObserver)

imgRequestProxy::imgRequestProxy() :
  mCanceled(PR_FALSE)
{
  NS_INIT_ISUPPORTS();
  /* member initializers and constructor code */
}

imgRequestProxy::~imgRequestProxy()
{
  /* destructor code */
  Cancel(NS_ERROR_FAILURE);
}



nsresult imgRequestProxy::Init(imgRequest *request, imgIDecoderObserver *aObserver, nsISupports *cx)
{
  PR_ASSERT(request);

  PR_LOG(gImgLog, PR_LOG_DEBUG,
         ("[this=%p] imgRequestProxy::Init (request=%p) {ENTER}\n",
          this, request));

  mOwner = NS_STATIC_CAST(imgIRequest*, request);

  mObserver = aObserver;
  // XXX we should save off the thread we are getting called on here so that we can proxy all calls to mDecoder to it.

  mContext = cx;

  request->AddObserver(this);

  PR_LOG(gImgLog, PR_LOG_DEBUG,
         ("[this=%p] imgRequestProxy::Init {EXIT}\n",
          this));

  return NS_OK;
}


/* void cancel (in nsresult status); */
NS_IMETHODIMP imgRequestProxy::Cancel(nsresult status)
{
  if (mCanceled)
    return NS_ERROR_FAILURE;

  LOG_SCOPE("imgRequestProxy::Cancel");

  mCanceled = PR_TRUE;
  nsresult rv = NS_REINTERPRET_CAST(imgRequest*, mOwner.get())->RemoveObserver(this, status);

  return rv;
}

/* readonly attribute gfxIImageContainer image; */
NS_IMETHODIMP imgRequestProxy::GetImage(gfxIImageContainer * *aImage)
{
  return mOwner->GetImage(aImage);
}

/* readonly attribute unsigned long imageStatus; */
NS_IMETHODIMP imgRequestProxy::GetImageStatus(PRUint32 *aStatus)
{
  return mOwner->GetImageStatus(aStatus);
}

/* readonly attribute nsIURI URI; */
NS_IMETHODIMP imgRequestProxy::GetURI(nsIURI **aURI)
{
  return mOwner->GetURI(aURI);
}

/** gfxIImageContainerObserver methods **/

/* [noscript] void frameChanged (in gfxIImageContainer container, in nsISupports cx, in gfxIImageFrame newframe, in nsRect dirtyRect); */
NS_IMETHODIMP imgRequestProxy::FrameChanged(gfxIImageContainer *container, nsISupports *cx, gfxIImageFrame *newframe, nsRect * dirtyRect)
{
  PR_LOG(gImgLog, PR_LOG_DEBUG,
         ("[this=%p] imgRequestProxy::FrameChanged\n", this));

  if (mObserver)
    mObserver->FrameChanged(container, mContext, newframe, dirtyRect);

  return NS_OK;
}

/** imgIDecoderObserver methods **/

/* void onStartDecode (in imgIRequest request, in nsISupports cx); */
NS_IMETHODIMP imgRequestProxy::OnStartDecode(imgIRequest *request, nsISupports *cx)
{
  PR_LOG(gImgLog, PR_LOG_DEBUG,
         ("[this=%p] imgRequestProxy::OnStartDecode\n", this));

  if (mObserver)
    mObserver->OnStartDecode(this, mContext);

  return NS_OK;
}

/* void onStartContainer (in imgIRequest request, in nsISupports cx, in gfxIImageContainer image); */
NS_IMETHODIMP imgRequestProxy::OnStartContainer(imgIRequest *request, nsISupports *cx, gfxIImageContainer *image)
{
  PR_LOG(gImgLog, PR_LOG_DEBUG,
         ("[this=%p] imgRequestProxy::OnStartContainer\n", this));

  if (mObserver)
    mObserver->OnStartContainer(this, mContext, image);

  return NS_OK;
}

/* void onStartFrame (in imgIRequest request, in nsISupports cx, in gfxIImageFrame frame); */
NS_IMETHODIMP imgRequestProxy::OnStartFrame(imgIRequest *request, nsISupports *cx, gfxIImageFrame *frame)
{
  PR_LOG(gImgLog, PR_LOG_DEBUG,
         ("[this=%p] imgRequestProxy::OnStartFrame\n", this));

  if (mObserver)
    mObserver->OnStartFrame(this, mContext, frame);

  return NS_OK;
}

/* [noscript] void onDataAvailable (in imgIRequest request, in nsISupports cx, in gfxIImageFrame frame, [const] in nsRect rect); */
NS_IMETHODIMP imgRequestProxy::OnDataAvailable(imgIRequest *request, nsISupports *cx, gfxIImageFrame *frame, const nsRect * rect)
{
  PR_LOG(gImgLog, PR_LOG_DEBUG,
         ("[this=%p] imgRequestProxy::OnDataAvailable\n", this));

  if (mObserver)
    mObserver->OnDataAvailable(this, mContext, frame, rect);

  return NS_OK;
}

/* void onStopFrame (in imgIRequest request, in nsISupports cx, in gfxIImageFrame frame); */
NS_IMETHODIMP imgRequestProxy::OnStopFrame(imgIRequest *request, nsISupports *cx, gfxIImageFrame *frame)
{
  PR_LOG(gImgLog, PR_LOG_DEBUG,
         ("[this=%p] imgRequestProxy::OnStopFrame\n", this));

  if (mObserver)
    mObserver->OnStopFrame(this, mContext, frame);

  return NS_OK;
}

/* void onStopContainer (in imgIRequest request, in nsISupports cx, in gfxIImageContainer image); */
NS_IMETHODIMP imgRequestProxy::OnStopContainer(imgIRequest *request, nsISupports *cx, gfxIImageContainer *image)
{
  PR_LOG(gImgLog, PR_LOG_DEBUG,
         ("[this=%p] imgRequestProxy::OnStopContainer\n", this));

  if (mObserver)
    mObserver->OnStopContainer(this, mContext, image);

  return NS_OK;
}

/* void onStopDecode (in imgIRequest request, in nsISupports cx, in nsresult status, in wstring statusArg); */
NS_IMETHODIMP imgRequestProxy::OnStopDecode(imgIRequest *request, nsISupports *cx, nsresult status, const PRUnichar *statusArg)
{
  PR_LOG(gImgLog, PR_LOG_DEBUG,
         ("[this=%p] imgRequestProxy::OnStopDecode\n", this));

  if (mObserver)
    mObserver->OnStopDecode(this, mContext, status, statusArg);

  return NS_OK;
}

