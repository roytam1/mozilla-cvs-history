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

#include "nsImageRequest.h"

#include "nsXPIDLString.h"

#include "nsIChannel.h"
#include "nsIInputStream.h"
#include "nsIImageLoader.h"
#include "nsIComponentManager.h"

#include "nsIComponentManager.h"
#include "nsIServiceManager.h"

#include "nsString.h"

#include "nsIEventQueueService.h"
#include "nsIEventQueue.h"

#include "nspr.h"

NS_IMPL_THREADSAFE_ISUPPORTS6(nsImageRequest, nsIImageRequest, nsIRequest,
                              nsIImageDecoderObserver, nsIStreamListener, nsIStreamObserver, nsIRunnable)

nsImageRequest::nsImageRequest()
{
  NS_INIT_ISUPPORTS();
  /* member initializers and constructor code */
  mProcessing = PR_TRUE;
  mStatus = nsIImageRequest::STATUS_NONE;
}

nsImageRequest::~nsImageRequest()
{
  /* destructor code */
}



/* void init (in nsIChannel aChannel, in nsIImageDecoderObserver aObserver, in nsISupports cx); */
NS_IMETHODIMP nsImageRequest::Init(nsIChannel *aChannel, nsIImageDecoderObserver *aObserver, nsISupports *cx)
{
  if (mImage)
    return NS_ERROR_FAILURE; // XXX

  mChannel = aChannel;

  mObserver = aObserver;
  // XXX we should save off the thread we are getting called on here so that we can proxy all calls to mDecoder to it.

  mContext = cx;

  // XXX do not init the image here.  this has to be done from the image decoder.
  mImage = do_CreateInstance("@mozilla.org/gfx/image;2");

  return NS_OK;
}

/* readonly attribute nsIImage image; */
NS_IMETHODIMP nsImageRequest::GetImage(nsIImageContainer * *aImage)
{
  *aImage = mImage;
  NS_IF_ADDREF(*aImage);
  return NS_OK;
}

/* readonly attribute unsigned long imageStatus; */
NS_IMETHODIMP nsImageRequest::GetImageStatus(PRUint32 *aStatus)
{
  *aStatus = mStatus;
  return NS_OK;
}





/** nsIRequest methods **/

/* readonly attribute wstring name; */
NS_IMETHODIMP nsImageRequest::GetName(PRUnichar * *aName)
{
  return mChannel->GetName(aName);
}

/* boolean isPending (); */
NS_IMETHODIMP nsImageRequest::IsPending(PRBool *_retval)
{
  return mChannel->IsPending(_retval);
}

/* readonly attribute nsresult status; */
NS_IMETHODIMP nsImageRequest::GetStatus(nsresult *aStatus)
{
  return mChannel->GetStatus(aStatus);
}

/* void cancel (in nsresult status); */
NS_IMETHODIMP nsImageRequest::Cancel(nsresult status)
{
  return mChannel->Cancel(status);
}

/* void suspend (); */
NS_IMETHODIMP nsImageRequest::Suspend()
{
  return mChannel->Suspend();
}

/* void resume (); */
NS_IMETHODIMP nsImageRequest::Resume()
{
  return mChannel->Resume();
}






/** nsIImageDecoderObserver methods **/

/* void onStartDecode (in nsIImageRequest request, in nsISupports cx); */
NS_IMETHODIMP nsImageRequest::OnStartDecode(nsIImageRequest *request, nsISupports *cx)
{
  if (mObserver)
    mObserver->OnStartDecode(this, mContext);

  return NS_OK;
}

/* void onStartContainer (in nsIImageRequest request, in nsISupports cx, in nsIImageContainer image); */
NS_IMETHODIMP nsImageRequest::OnStartContainer(nsIImageRequest *request, nsISupports *cx, nsIImageContainer *image)
{
  mStatus |= nsIImageRequest::STATUS_SIZE_AVAILABLE;

  if (mObserver)
    mObserver->OnStartContainer(this, mContext, image);

  return NS_OK;
}

/* void onStartFrame (in nsIImageRequest request, in nsISupports cx, in nsIImageFrame frame); */
NS_IMETHODIMP nsImageRequest::OnStartFrame(nsIImageRequest *request, nsISupports *cx, nsIImageFrame *frame)
{
  if (mObserver)
    mObserver->OnStartFrame(this, mContext, frame);

  return NS_OK;
}

/* [noscript] void onDataAvailable (in nsIImageRequest request, in nsISupports cx, in nsIImageFrame frame, [const] in nsRect2 rect); */
NS_IMETHODIMP nsImageRequest::OnDataAvailable(nsIImageRequest *request, nsISupports *cx, nsIImageFrame *frame, const nsRect2 * rect)
{
  if (mObserver)
    mObserver->OnDataAvailable(this, mContext, frame, rect);

  return NS_OK;
}

/* void onStopFrame (in nsIImageRequest request, in nsISupports cx, in nsIImageFrame frame); */
NS_IMETHODIMP nsImageRequest::OnStopFrame(nsIImageRequest *request, nsISupports *cx, nsIImageFrame *frame)
{
  if (mObserver)
    mObserver->OnStopFrame(this, mContext, frame);

  return NS_OK;
}

/* void onStopContainer (in nsIImageRequest request, in nsISupports cx, in nsIImageContainer image); */
NS_IMETHODIMP nsImageRequest::OnStopContainer(nsIImageRequest *request, nsISupports *cx, nsIImageContainer *image)
{
  if (mObserver)
    mObserver->OnStopContainer(this, mContext, image);

  return NS_OK;
}

/* void onStopDecode (in nsIImageRequest request, in nsISupports cx, in nsresult status, in wstring statusArg); */
NS_IMETHODIMP nsImageRequest::OnStopDecode(nsIImageRequest *request, nsISupports *cx, nsresult status, const PRUnichar *statusArg)
{

  if (NS_FAILED(status))
    mStatus = nsIImageRequest::STATUS_ERROR;

  if (mObserver)
    mObserver->OnStopDecode(this, mContext, status, statusArg);

  return NS_OK;
}






/** nsIStreamObserver methods **/

/* void onStartRequest (in nsIChannel channel, in nsISupports ctxt); */
NS_IMETHODIMP nsImageRequest::OnStartRequest(nsIChannel *channel, nsISupports *ctxt)
{
  nsXPIDLCString contentType;
  channel->GetContentType(getter_Copies(contentType));
  printf("content type is %s\n", contentType.get());

  nsCAutoString conid("@mozilla.org/image/decoder;2?type=");
  conid += contentType.get();

  mDecoder = do_CreateInstance(conid);

  if (!mDecoder) {
    // no image decoder for this mimetype :(
    channel->Cancel(NS_BINDING_ABORTED);

    // XXX notify the person that owns us now that wants the nsIImageContainer off of us?

    return NS_ERROR_FAILURE;
  }

  mDecoder->Init(NS_STATIC_CAST(nsIImageRequest*, this));
  return NS_OK;
}

/* void onStopRequest (in nsIChannel channel, in nsISupports ctxt, in nsresult status, in wstring statusArg); */
NS_IMETHODIMP nsImageRequest::OnStopRequest(nsIChannel *channel, nsISupports *ctxt, nsresult status, const PRUnichar *statusArg)
{
  mProcessing = PR_FALSE;

  if (!mDecoder) return NS_ERROR_FAILURE;

  nsresult rv = mDecoder->Close();

  mDecoder = nsnull; // release the decoder so that it can rest peacefully ;)

  return rv;
}




/** nsIStreamListener methods **/

/* void onDataAvailable (in nsIChannel channel, in nsISupports ctxt, in nsIInputStream inStr, in unsigned long sourceOffset, in unsigned long count); */
NS_IMETHODIMP nsImageRequest::OnDataAvailable(nsIChannel *channel, nsISupports *ctxt, nsIInputStream *inStr, PRUint32 sourceOffset, PRUint32 count)
{
  if (!mDecoder) return NS_ERROR_FAILURE;

  PRUint32 wrote;
  return mDecoder->WriteFrom(inStr, count, &wrote);
}





/** nsIRunnable methods **/

NS_IMETHODIMP nsImageRequest::Run()
{
  nsresult rv = NS_OK;
  if (!mChannel) return NS_ERROR_NOT_INITIALIZED;

  // create an event queue for this thread.
  nsCOMPtr<nsIEventQueueService> service = do_GetService(NS_EVENTQUEUESERVICE_CONTRACTID, &rv);
  if (NS_FAILED(rv)) return rv;

  rv = service->CreateThreadEventQueue();
  if (NS_FAILED(rv)) return rv;

  nsCOMPtr<nsIEventQueue> currentThreadQ;
  rv = service->GetThreadEventQueue(NS_CURRENT_THREAD, 
                                getter_AddRefs(currentThreadQ));
  if (NS_FAILED(rv)) return rv;

  // initiate the AsyncRead from this thread so events are
  // sent here for processing.
  rv = mChannel->AsyncRead(NS_STATIC_CAST(nsIStreamListener*, this), nsnull);
  if (NS_FAILED(rv)) return rv;

  // process events until we're finished.
  PLEvent *event;
  while (mProcessing) {
    rv = currentThreadQ->WaitForEvent(&event);
    if (NS_FAILED(rv)) return rv;

    rv = currentThreadQ->HandleEvent(event);
    if (NS_FAILED(rv)) return rv;
  }

  rv = service->DestroyThreadEventQueue();
  if (NS_FAILED(rv)) return rv;

  // XXX make sure cleanup happens on the calling thread.
  return NS_OK;
}




