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
 * Communications Corporation. Portions created by Netscape are
 * Copyright (C) 2001 Netscape Communications Corporation. All
 * Rights Reserved.
 * 
 * Contributor(s): 
 *   Stuart Parmenter <pavlov@netscape.com>
 *   Chris Saari <saari@netscape.com>
 */

#include "nsImageContainer.h"
#include "nsIServiceManager.h"
#include "nsIImageFrame.h"

NS_IMPL_ISUPPORTS1(nsImageContainer, nsIImageContainer)

nsImageContainer::nsImageContainer()
{
  NS_INIT_ISUPPORTS();
  /* member initializers and constructor code */
  mCurrentFrame = 0;
  mCurrentAnimationFrame = 0;
  mCurrentFrameIsFinishedDecoding = PR_FALSE;
  mDoneDecoding = PR_FALSE;
}

nsImageContainer::~nsImageContainer()
{
  /* destructor code */
  mFrames.Clear();
}



/* void init (in nscoord aWidth, in nscoord aHeight, in nsIImageContainerObserver aObserver); */
NS_IMETHODIMP nsImageContainer::Init(nscoord aWidth, nscoord aHeight, nsIImageContainerObserver *aObserver)
{
  if (aWidth <= 0 || aHeight <= 0) {
    printf("error - negative image size\n");
    return NS_ERROR_FAILURE;
  }

  mSize.SizeTo(aWidth, aHeight);

  mObserver = aObserver;

  return NS_OK;
}

/* readonly attribute gfx_format preferredAlphaChannelFormat; */
NS_IMETHODIMP nsImageContainer::GetPreferredAlphaChannelFormat(gfx_format *aFormat)
{
  *aFormat = nsIGFXFormat::RGB_A8;
  return NS_OK;
}

/* readonly attribute nscoord width; */
NS_IMETHODIMP nsImageContainer::GetWidth(nscoord *aWidth)
{
  *aWidth = mSize.width;
  return NS_OK;
}

/* readonly attribute nscoord height; */
NS_IMETHODIMP nsImageContainer::GetHeight(nscoord *aHeight)
{
  *aHeight = mSize.height;
  return NS_OK;
}


/* readonly attribute nsIImageFrame currentFrame; */
NS_IMETHODIMP nsImageContainer::GetCurrentFrame(nsIImageFrame * *aCurrentFrame)
{
  return this->GetFrameAt(mCurrentFrame, aCurrentFrame);
}

/* readonly attribute unsigned long numFrames; */
NS_IMETHODIMP nsImageContainer::GetNumFrames(PRUint32 *aNumFrames)
{
  return mFrames.Count(aNumFrames);
}

/* nsIImageFrame getFrameAt (in unsigned long index); */
NS_IMETHODIMP nsImageContainer::GetFrameAt(PRUint32 index, nsIImageFrame **_retval)
{
  nsISupports *sup = mFrames.ElementAt(index);
  if (!sup)
    return NS_ERROR_FAILURE;

  *_retval = NS_REINTERPRET_CAST(nsIImageFrame *, sup);
  return NS_OK;
}

/* void appendFrame (in nsIImageFrame item); */
NS_IMETHODIMP nsImageContainer::AppendFrame(nsIImageFrame *item)
{
  // If this is our second frame, init a timer so we don't display
  // the next frame until the delay timer has expired for the current
  // frame.
  PRUint32 numFrames;
  this->GetNumFrames(&numFrames);
  if(!mTimer){
    if(numFrames) {
      // Since we have more than one frame we need a timer
      mTimer = do_CreateInstance("@mozilla.org/timer;1");
      
      PRInt32 timeout;
      nsCOMPtr<nsIImageFrame> currentFrame;
      this->GetFrameAt(mCurrentFrame, getter_AddRefs(currentFrame));
      currentFrame->GetTimeout(&timeout);
      if(timeout != -1 &&
         timeout >= 0) { // -1 means display this frame forever
        
        mTimer->Init(
          sAnimationTimerCallback, this, timeout, 
          NS_PRIORITY_NORMAL, NS_TYPE_ONE_SHOT);
      }
    }
  }
  
  if(numFrames) mCurrentFrame++;
  
  mCurrentFrameIsFinishedDecoding = PR_FALSE;
  return mFrames.AppendElement(NS_REINTERPRET_CAST(nsISupports*, item));
}

/* void removeFrame (in nsIImageFrame item); */
NS_IMETHODIMP nsImageContainer::RemoveFrame(nsIImageFrame *item)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* nsIEnumerator enumerate (); */
NS_IMETHODIMP nsImageContainer::Enumerate(nsIEnumerator **_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void clear (); */
NS_IMETHODIMP nsImageContainer::Clear()
{
  return mFrames.Clear();
}

/* attribute long loopCount; */
NS_IMETHODIMP nsImageContainer::GetLoopCount(PRInt32 *aLoopCount)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP nsImageContainer::SetLoopCount(PRInt32 aLoopCount)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void endFrameDecode (in nsIImageFrame item, in unsigned long timeout); */
NS_IMETHODIMP nsImageContainer::EndFrameDecode(PRUint32 aFrameNum, PRUint32 aTimeout)
{
  // It is now okay to start the timer for the next frame in the animation
  mCurrentFrameIsFinishedDecoding = PR_TRUE;
  return NS_OK;
}

/* void decodingComplete (); */
NS_IMETHODIMP nsImageContainer::DecodingComplete(void)
{
  mDoneDecoding = PR_TRUE;
  return NS_OK;
}

void nsImageContainer::sAnimationTimerCallback(nsITimer *aTimer, void *aImageContainer)
{
  nsImageContainer* self = NS_STATIC_CAST(nsImageContainer*, aImageContainer);
  nsCOMPtr<nsIImageFrame> nextFrame;
  PRInt32 timeout = 100;
      
  printf("timer callback ");
  
  // If we're done decoding the next frame, go ahead and display it now and reinit
  // the timer with the next frame's delay time.
  if(self->mCurrentFrameIsFinishedDecoding && !self->mDoneDecoding) {
    // If we have the next frame in the sequence set the timer callback from it
    self->GetFrameAt(self->mCurrentAnimationFrame + 1, getter_AddRefs(nextFrame));
    if(nextFrame) {
      // Go to next frame in sequence
      nextFrame->GetTimeout(&timeout);
      self->mCurrentAnimationFrame++;
    } else if (self->mDoneDecoding) {
      // Go back to the beginning of the loop
      self->GetFrameAt(0, getter_AddRefs(nextFrame));
      nextFrame->GetTimeout(&timeout);
      self->mCurrentAnimationFrame = 0;
    } else {
      // twiddle our thumbs
      self->GetFrameAt(self->mCurrentAnimationFrame, getter_AddRefs(nextFrame));
      nextFrame->GetTimeout(&timeout);
    }
  } else if(self->mDoneDecoding){
    PRUint32 numFrames;
    self->GetNumFrames(&numFrames);
    if(numFrames == self->mCurrentAnimationFrame) {
      self->GetFrameAt(0, getter_AddRefs(nextFrame));
      self->mCurrentAnimationFrame = 0;
      self->mCurrentFrame = 0;
      nextFrame->GetTimeout(&timeout);
    } else {
      self->GetFrameAt(self->mCurrentAnimationFrame++, getter_AddRefs(nextFrame));
      self->mCurrentFrame = self->mCurrentAnimationFrame;
      nextFrame->GetTimeout(&timeout);
    }
  } else {
      self->GetFrameAt(self->mCurrentFrame, getter_AddRefs(nextFrame));
      nextFrame->GetTimeout(&timeout);
  }
  
  printf(" mCurrentAnimationFrame = %d\n", self->mCurrentAnimationFrame);
  
  // XXX do notification to FE to draw this frame
  nsRect* dirtyRect;
  nextFrame->GetRect(&dirtyRect);
  
  printf("x=%d, y=%d, w=%d, h=%d\n", dirtyRect->x, dirtyRect->y, dirtyRect->width, dirtyRect->height);
  self->mObserver->FrameChanged(
    self, nsnull, 
    nextFrame, dirtyRect);
  
  
  self->mTimer->Init(
    sAnimationTimerCallback, aImageContainer, timeout, 
    NS_PRIORITY_NORMAL, NS_TYPE_ONE_SHOT);
}