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

#ifndef nsJPEGDecoder_h__
#define nsJPEGDecoder_h__

#include "nsIImageDecoder.h"

#include "nsCOMPtr.h"

#include "nsIImageContainer.h"
#include "nsIImageFrame.h"
#include "nsIImageDecoderObserver.h"
#include "nsIImageRequest2.h"
#include "nsIInputStream.h"

#include "jpeglib.h"

#define NS_JPEGDECODER_CID \
{ /* 5871a422-1dd2-11b2-ab3f-e2e56be5da9c */         \
     0x5871a422,                                     \
     0x1dd2,                                         \
     0x11b2,                                         \
    {0xab, 0x3f, 0xe2, 0xe5, 0x6b, 0xe5, 0xda, 0x9c} \
}

typedef struct {
    struct jpeg_error_mgr pub;  /* "public" fields for IJG library*/
    jmp_buf setjmp_buffer;      /* For handling catastropic errors */
} decoder_error_mgr;


typedef enum {
    JPEG_HEADER,                          /* Reading JFIF headers */
    JPEG_START_DECOMPRESS,
    JPEG_DECOMPRESS_PROGRESSIVE,          /* Output progressive pixels */
    JPEG_DECOMPRESS_SEQUENTIAL,           /* Output sequential pixels */
    JPEG_FINAL_PROGRESSIVE_SCAN_OUTPUT,
    JPEG_DONE,
    JPEG_SINK_NON_JPEG_TRAILER,          /* Some image files have a */
                                         /* non-JPEG trailer */
    JPEG_ERROR    
} jstate;

class nsJPEGDecoder : public nsIImageDecoder
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIIMAGEDECODER
  NS_DECL_NSIOUTPUTSTREAM

  nsJPEGDecoder();
  virtual ~nsJPEGDecoder();

  PRBool FillInput(j_decompress_ptr jd);

  PRUint32 mBytesToSkip;

protected:
  int OutputScanlines(int num_scanlines);

public:
  nsCOMPtr<nsIImageContainer> mImage;
  nsCOMPtr<nsIImageFrame> mFrame;
  nsCOMPtr<nsIImageRequest> mRequest;

  nsCOMPtr<nsIImageDecoderObserver> mObserver;

  struct jpeg_decompress_struct mInfo;
  decoder_error_mgr mErr;
  jstate mState;

  char *mBuf;
  PRUint32 mBufLen;
  PRUint32 mCurDataLen;

  JSAMPARRAY mSamples;
  JSAMPARRAY mSamples3;

  PRInt32 mCompletedPasses;
  PRInt32 mPasses;
};

#endif // nsJPEGDecoder_h__
