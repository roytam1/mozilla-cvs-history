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

#ifndef nsPPMDecoder_h__
#define nsPPMDecoder_h__

#include "imgIDecoder.h"

#include "nsCOMPtr.h"

#include "imgIContainer.h"
#include "imgIDecoderObserver.h"
#include "gfxIImageFrame.h"
#include "imgILoad.h"

#define NS_PPMDECODER_CID \
{ /* e90bfa06-1dd1-11b2-8217-f38fe5d431a2 */         \
     0xe90bfa06,                                     \
     0x1dd1,                                         \
     0x11b2,                                         \
    {0x82, 0x17, 0xf3, 0x8f, 0xe5, 0xd4, 0x31, 0xa2} \
}

class nsPPMDecoder : public imgIDecoder
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_IMGIDECODER

  nsPPMDecoder();
  virtual ~nsPPMDecoder();

private:
  NS_METHOD checkSendRow();

  nsCOMPtr<imgIContainer> mImage;
  nsCOMPtr<gfxIImageFrame> mFrame;
  nsCOMPtr<imgILoad> mImageLoad;
  nsCOMPtr<imgIDecoderObserver> mObserver; // this is just qi'd from mRequest for speed

  char *mBuffer;
  PRUint32 mBufferSize;

  PRUint32 mState;
  PRUint32 mSkip;
  PRUint32 mOldSkip;
  char mDigits[5];
  PRUint32 mDigitCount;

  PRUint32 mType;
  nscoord mWidth;
  nscoord mHeight;
  PRUint32 mMaxValue;

  PRUint32 mRow;
  PRUint32 mBytesPerRow;
  PRUint32 mFrameBytesPerRow;
  PRUint8 *mRowData;
  PRUint32 mRowDataFill;
};

#endif // nsPPMDecoder_h__
