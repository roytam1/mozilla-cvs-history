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

#include "nsIImageRequest2.h"

#include "nsPIImageRequest.h"

#include "nsIRunnable.h"

#include "nsIChannel.h"
#include "nsIImageContainer.h"
#include "nsIImageDecoder.h"
#include "nsIImageDecoderObserver.h"
#include "nsIStreamListener.h"
#include "nsCOMPtr.h"

#include "nsVoidArray.h"

#define NS_IMAGEREQUEST_CID \
{ /* 9f733dd6-1dd1-11b2-8cdf-effb70d1ea71 */         \
     0x9f733dd6,                                     \
     0x1dd1,                                         \
     0x11b2,                                         \
    {0x8c, 0xdf, 0xef, 0xfb, 0x70, 0xd1, 0xea, 0x71} \
}


enum {
  onStartDecode = 0x1,
  onStartContainer = 0x2,
  onStopContainer = 0x4,
  onStopDecode = 0x8
};

class nsImageRequest : public nsIImageRequest,
                       public nsPIImageRequest,
                       public nsIImageDecoderObserver, 
                       public nsIStreamListener, public nsIRunnable
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIIMAGEREQUEST
  NS_DECL_NSPIIMAGEREQUEST
  NS_DECL_NSIIMAGEDECODEROBSERVER
  NS_DECL_NSISTREAMLISTENER
  NS_DECL_NSISTREAMOBSERVER
  NS_DECL_NSIRUNNABLE

  nsImageRequest();
  virtual ~nsImageRequest();
  /* additional members */

private:
  nsCOMPtr<nsIChannel> mChannel;
  nsCOMPtr<nsIImageContainer> mImage;
  nsCOMPtr<nsIImageDecoder> mDecoder;

  nsVoidArray mObservers;

  PRBool mProcessing;
  PRUint32 mStatus;
  PRUint32 mState;
};
