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
 *
 */

#include "nsJPEGDecoder.h"

#include "nsIInputStream.h"

#include "nspr.h"

#include "nsCRT.h"

#include "nsIComponentManager.h"


NS_IMPL_ISUPPORTS2(nsJPEGDecoder, nsIImageDecoder, nsIOutputStream)


void PR_CALLBACK init_source (j_decompress_ptr jd);
boolean PR_CALLBACK fill_input_buffer (j_decompress_ptr jd);
void PR_CALLBACK skip_input_data (j_decompress_ptr jd, long num_bytes);
void PR_CALLBACK term_source (j_decompress_ptr jd);
void PR_CALLBACK il_error_exit (j_common_ptr cinfo);

/* Normal JFIF markers can't have more bytes than this. */
#define MAX_JPEG_MARKER_LENGTH  (((PRUint32)1 << 16) - 1)

/*
 *  Implementation of a JPEG src object that understands our state machine
 */
typedef struct {
  /* public fields; must be first in this struct! */
  struct jpeg_source_mgr pub;

  nsJPEGDecoder *decoder;

} decoder_source_mgr;


nsJPEGDecoder::nsJPEGDecoder()
{
  NS_INIT_ISUPPORTS();

  mState = JPEG_HEADER;

  mDataLen = 0;

  mSamples = nsnull;
  mSamples3 = nsnull;

  mBytesToSkip = 0;
}

nsJPEGDecoder::~nsJPEGDecoder()
{

}


/** nsIImageDecoder methods **/

/* void init (in nsIImageRequest aRequest); */
NS_IMETHODIMP nsJPEGDecoder::Init(nsIImageRequest *aRequest)
{
  mRequest = aRequest;
  mObserver = do_QueryInterface(mRequest);

  aRequest->GetImage(getter_AddRefs(mImage));


  NS_NewPipe(getter_AddRefs(mInStream),
             getter_AddRefs(mOutStream),
             10240); // this could be a lot smaller (like 3-6k?)

  /* Step 1: allocate and initialize JPEG decompression object */

  /* Now we can initialize the JPEG decompression object. */
  jpeg_create_decompress(&mInfo);


  /* Step 2: specify data source (eg, a file) */
  decoder_source_mgr *src;

  if (mInfo.src == NULL) {
    src = PR_NEWZAP(decoder_source_mgr);
    if (!src) {
      return PR_FALSE;
    }
    mInfo.src = (struct jpeg_source_mgr *) src;
  }

  mInfo.err = jpeg_std_error(&mErr.pub);

  mErr.pub.error_exit = il_error_exit;

  /* Setup callback functions. */
  src->pub.init_source = init_source;
  src->pub.fill_input_buffer = fill_input_buffer;
  src->pub.skip_input_data = skip_input_data;
  src->pub.resync_to_restart = jpeg_resync_to_restart;
  src->pub.term_source = term_source;

  src->decoder = this;


#if 0
  /* We set up the normal JPEG error routines, then override error_exit. */
  mInfo.err = jpeg_std_error(&jerr.pub);
  jerr.pub.error_exit = my_error_exit;
  /* Establish the setjmp return context for my_error_exit to use. */
  if (setjmp(jerr.setjmp_buffer)) {
    /* If we get here, the JPEG code has signaled an error.
     * We need to clean up the JPEG object, close the input file, and return.
     */
    jpeg_destroy_decompress(&cinfo);
    fclose(infile);
    return 0;
  }

#endif


  return NS_OK;
}

/* readonly attribute nsIImageRequest request; */
NS_IMETHODIMP nsJPEGDecoder::GetRequest(nsIImageRequest * *aRequest)
{
  *aRequest = mRequest;
  NS_ADDREF(*aRequest);
  return NS_OK;
}






/** nsIOutputStream methods **/

/* void close (); */
NS_IMETHODIMP nsJPEGDecoder::Close()
{

  // XXX this should flush the data out


  // XXX progressive? ;)
//  OutputScanlines(mInfo.output_height);


  /* Step 8: Release JPEG decompression object */

  /* This is an important step since it will release a good deal of memory. */
  jpeg_destroy_decompress(&mInfo);

  return NS_OK;
}

/* void flush (); */
NS_IMETHODIMP nsJPEGDecoder::Flush()
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* unsigned long write (in string buf, in unsigned long count); */
NS_IMETHODIMP nsJPEGDecoder::Write(const char *buf, PRUint32 count, PRUint32 *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* unsigned long writeFrom (in nsIInputStream inStr, in unsigned long count); */
NS_IMETHODIMP nsJPEGDecoder::WriteFrom(nsIInputStream *inStr, PRUint32 count, PRUint32 *_retval)
{
  /* We use our private extension JPEG error handler.
   * Note that this struct must live as long as the main JPEG parameter
   * struct, to avoid dangling-pointer problems.
   */
  // XXX above what is this?

  if (inStr) {
    mOutStream->WriteFrom(inStr, count, _retval);
    mDataLen += *_retval;
  }
  
  


  /* Register new buffer contents with data source manager. */

  decoder_source_mgr *src = NS_REINTERPRET_CAST(decoder_source_mgr *, mInfo.src);

  int status;
  switch (mState) {
  case JPEG_HEADER:
    /* Step 3: read file parameters with jpeg_read_header() */
    if (jpeg_read_header(&mInfo, TRUE) == JPEG_SUSPENDED)
      return NS_OK;

    /*
     * Don't allocate a giant and superfluous memory buffer
     * when the image is a sequential JPEG.
     */
    mInfo.buffered_image = jpeg_has_multiple_scans(&mInfo);

    /* Used to set up image size so arrays can be allocated */
    jpeg_calc_output_dimensions(&mInfo);

    mObserver->OnStartDecode(nsnull, nsnull);

    mImage->Init(mInfo.image_width, mInfo.image_height); 
    mObserver->OnStartContainer(nsnull, nsnull, mImage);

    mFrame = do_CreateInstance("@mozilla.org/gfx/image/frame;2");
    mFrame->Init(0, 0, mInfo.image_width, mInfo.image_height, nsIGFXFormat::RGB);
    mImage->AppendFrame(mFrame);
    mObserver->OnStartFrame(nsnull, nsnull, mFrame);


    /*
     * Make a one-row-high sample array that will go away
     * when done with image. Always make it big enough to
     * hold an RGB row.  Since this uses the IJG memory
     * manager, it must be allocated before the call to
     * jpeg_start_compress().
     */
    int row_stride;
    row_stride = mInfo.output_width * mInfo.output_components;
    mSamples = (*mInfo.mem->alloc_sarray)((j_common_ptr) &mInfo,
                                           JPOOL_IMAGE,
                                           row_stride, 1);

    /* Allocate RGB buffer for conversion from greyscale. */
    if (mInfo.output_components != 3) {
      row_stride = mInfo.output_width * 3;
      mSamples3 = (*mInfo.mem->alloc_sarray)((j_common_ptr) &mInfo,
                                              JPOOL_IMAGE,
                                              row_stride, 1);
    }

    mState = JPEG_START_DECOMPRESS;

  case JPEG_START_DECOMPRESS:

    /* Step 4: set parameters for decompression */

    /* FIXME -- Should reset dct_method and dither mode
     * for final pass of progressive JPEG
     */
    mInfo.dct_method = JDCT_FASTEST;
    mInfo.dither_mode = JDITHER_ORDERED;
    mInfo.do_fancy_upsampling = FALSE;
    mInfo.enable_2pass_quant = FALSE;
    mInfo.do_block_smoothing = TRUE;

    /* In this example, we don't need to change any of the defaults set by
     * jpeg_read_header(), so we do nothing here.
     */

    /* Step 5: Start decompressor */
    if (jpeg_start_decompress(&mInfo) == FALSE)
      return NS_OK;

    mState = JPEG_DECOMPRESS_PROGRESSIVE;

  case JPEG_DECOMPRESS_PROGRESSIVE:
    do {
      status = jpeg_consume_input(&mInfo);
    } while (!((status == JPEG_SUSPENDED) ||
               (status == JPEG_REACHED_EOI)));

    if (status == JPEG_REACHED_EOI) {
      mState = JPEG_FINAL_PROGRESSIVE_SCAN_OUTPUT;
    } else {
      return NS_OK;
    }

  case JPEG_FINAL_PROGRESSIVE_SCAN_OUTPUT:
    jpeg_start_output(&mInfo, mInfo.input_scan_number);
    OutputScanlines(-1);
    jpeg_finish_output(&mInfo);
    mState = JPEG_DONE;

  case JPEG_DONE:
    /* Step 7: Finish decompression */

    if (jpeg_finish_decompress(&mInfo) == FALSE)
      return NS_OK;

    mState = JPEG_SINK_NON_JPEG_TRAILER;

    /* we're done dude */
    break;

  case JPEG_SINK_NON_JPEG_TRAILER:
    break;
  }

  /* We may need to do some setup of our own at this point before reading
   * the data.  After jpeg_start_decompress() we have the correct scaled
   * output image dimensions available, as well as the output colormap
   * if we asked for color quantization.
   * In this example, we need to make an output work buffer of the right size.
   */ 
  /* JSAMPLEs per row in output buffer */
#if 0
  int row_stride = mInfo.output_width * mInfo.output_components; /* physical row width in output buffer */

  /* Make a one-row-high sample array that will go away when done with image */
  JSAMPARRAY buffer = (*mInfo.mem->alloc_sarray)       /* Output row buffer */
		((j_common_ptr) &mInfo, JPOOL_IMAGE, row_stride, 1);

  /* Step 6: while (scan lines remain to be read) */
  /*           jpeg_read_scanlines(...); */

  /* Here we use the library's state variable cinfo.output_scanline as the
   * loop counter, so that we don't have to keep track ourselves.
   */
  while (mInfo.output_scanline < mInfo.output_height) {
    /* jpeg_read_scanlines expects an array of pointers to scanlines.
     * Here the array is only one element long, but you could ask for
     * more than one scanline at a time if that's more convenient.
     */
    (void) jpeg_read_scanlines(&mInfo, buffer, 1);
    /* Assume put_scanline_someplace wants a pointer and sample count. */
    mFrame->SetImageData(buffer[0], row_stride, row_stride*mInfo.output_scanline /* XXX ??? */);


    nsRect r(0, mInfo.output_scanline, mInfo.output_width, 1);
    mObserver->OnDataAvailable(nsnull, nsnull, mFrame, &r);

  }
#endif

  return NS_OK;
}


int
nsJPEGDecoder::OutputScanlines(int num_scanlines)
{
  int input_exhausted;
  int pass;
  
#ifdef DEBUG
  PRUintn start_scanline = mInfo.output_scanline;
#endif

  if (mState == JPEG_FINAL_PROGRESSIVE_SCAN_OUTPUT)
      pass = -1;
  else
      pass = mCompletedPasses + 1;

  while ((mInfo.output_scanline < mInfo.output_height) && num_scanlines--) {
      JSAMPROW samples;
      
      /* Request one scanline.  Returns 0 or 1 scanlines. */
      int ns = jpeg_read_scanlines(&mInfo, mSamples, 1);
#if 0
      ILTRACE(15,("il:jpeg: scanline %d, ns = %d",
                  mInfo.output_scanline, ns));
#endif
      if (ns != 1) {
//          ILTRACE(5,("il:jpeg: suspending scanline"));
          input_exhausted = TRUE;
          goto done;
      }

      /* If grayscale image ... */
      if (mInfo.output_components == 1) {
          JSAMPLE j, *j1, *j1end, *j3;

          /* Convert from grayscale to RGB. */
          j1 = mSamples[0];
          j1end = j1 + mInfo.output_width;
          j3 = mSamples3[0];
          while (j1 < j1end) {
              j = *j1++;
              j3[0] = j;
              j3[1] = j;
              j3[2] = j;
              j3 += 3;
          }
          samples = mSamples3[0];
      } else {        /* 24-bit color image */
          samples = mSamples[0];
      }


      mFrame->SetImageData(samples, strlen((const char *)samples), strlen((const char *)samples)*mInfo.output_scanline /* XXX ??? */);
#if 0
      ic->imgdcb->ImgDCBHaveRow( 0, samples, 0, mInfo.output_width, mInfo.output_scanline-1,
                  1, ilErase, pass);
#endif
  }

  input_exhausted = FALSE;

done:
  
  return input_exhausted;
}


/* [noscript] unsigned long writeSegments (in nsReadSegmentFun reader, in voidPtr closure, in unsigned long count); */
NS_IMETHODIMP nsJPEGDecoder::WriteSegments(nsReadSegmentFun reader, void * closure, PRUint32 count, PRUint32 *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute boolean nonBlocking; */
NS_IMETHODIMP nsJPEGDecoder::GetNonBlocking(PRBool *aNonBlocking)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP nsJPEGDecoder::SetNonBlocking(PRBool aNonBlocking)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute nsIOutputStreamObserver observer; */
NS_IMETHODIMP nsJPEGDecoder::GetObserver(nsIOutputStreamObserver * *aObserver)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP nsJPEGDecoder::SetObserver(nsIOutputStreamObserver * aObserver)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}





/* Override the standard error method in the IJG JPEG decoder code. */
void PR_CALLBACK
il_error_exit (j_common_ptr cinfo)
{
#if 0
    int error_code;
    il_error_mgr *err = (il_error_mgr *) cinfo->err;

#ifdef DEBUG
#if 0
  /*ptn fix later */
    if (il_debug >= 1) {
        char buffer[JMSG_LENGTH_MAX];

        /* Create the message */
        (*cinfo->err->format_message) (cinfo, buffer);

        ILTRACE(1,("%s\n", buffer));
    }
#endif
#endif

    /* Convert error to a browser error code */
    if (cinfo->err->msg_code == JERR_OUT_OF_MEMORY)
        error_code = MK_OUT_OF_MEMORY;
    else
        error_code = MK_IMAGE_LOSSAGE;
        
    /* Return control to the setjmp point. */
    longjmp(err->setjmp_buffer, error_code);

#endif
}



void PR_CALLBACK
init_source (j_decompress_ptr jd)
{
}



static NS_METHOD DiscardData(nsIInputStream* in,
                             void* closure,
                             const char* fromRawSegment,
                             PRUint32 toOffset,
                             PRUint32 count,
                             PRUint32 *writeCount)
{
  j_decompress_ptr jd = NS_STATIC_CAST(j_decompress_ptr, closure);
  decoder_source_mgr *src = NS_REINTERPRET_CAST(decoder_source_mgr *, jd->src);

  *writeCount = count;

  return NS_OK;
}

void PR_CALLBACK
skip_input_data (j_decompress_ptr jd, long num_bytes)
{
  decoder_source_mgr *src = (decoder_source_mgr *)jd->src;

  if (num_bytes > (long)src->pub.bytes_in_buffer) {
    /*
     * Can't skip it all right now until we get more data from
     * network stream. Set things up so that fill_input_buffer
     * will skip remaining amount.
     */

    PRUint32 _retval;
    src->decoder->mInStream->ReadSegments(DiscardData, NS_STATIC_CAST(void*, jd), 
                                          src->pub.bytes_in_buffer, &_retval);
    src->decoder->mDataLen -= _retval;

    src->decoder->mBytesToSkip = (size_t)num_bytes - src->pub.bytes_in_buffer;
    src->pub.next_input_byte += src->pub.bytes_in_buffer;
    src->pub.bytes_in_buffer = 0;

  } else {
    /* Simple case. Just advance buffer pointer */

    PRUint32 _retval;
    src->decoder->mInStream->ReadSegments(DiscardData, NS_STATIC_CAST(void*, jd), 
                                          num_bytes, &_retval);
    src->decoder->mDataLen -= _retval;

    src->decoder->mBytesToSkip = 0;
    src->pub.bytes_in_buffer -= (size_t)num_bytes;
    src->pub.next_input_byte += num_bytes;
  }
}



/*-----------------------------------------------------------------------------
 * This is the callback routine from the IJG JPEG library used to supply new
 * data to the decompressor when its input buffer is exhausted.  It juggles
 * multiple buffers in an attempt to avoid unnecessary copying of input data.
 *
 * (A simpler scheme is possible: It's much easier to use only a single
 * buffer; when fill_input_buffer() is called, move any unconsumed data
 * (beyond the current pointer/count) down to the beginning of this buffer and
 * then load new data into the remaining buffer space.  This approach requires
 * a little more data copying but is far easier to get right.)
 *
 * At any one time, the JPEG decompressor is either reading from the netlib
 * input buffer, which is volatile across top-level calls to the IJG library,
 * or the "backtrack" buffer.  The backtrack buffer contains the remaining
 * unconsumed data from the netlib buffer after parsing was suspended due
 * to insufficient data in some previous call to the IJG library.
 *
 * When suspending, the decompressor will back up to a convenient restart
 * point (typically the start of the current MCU). The variables
 * next_input_byte & bytes_in_buffer indicate where the restart point will be
 * if the current call returns FALSE.  Data beyond this point must be
 * rescanned after resumption, so it must be preserved in case the decompressor
 * decides to backtrack.
 *
 * Returns:
 *  TRUE if additional data is available, FALSE if no data present and
 *   the JPEG library should therefore suspend processing of input stream
 *---------------------------------------------------------------------------*/

static NS_METHOD ReadDataOut(nsIInputStream* in,
                             void* closure,
                             const char* fromRawSegment,
                             PRUint32 toOffset,
                             PRUint32 count,
                             PRUint32 *writeCount)
{
  j_decompress_ptr jd = NS_STATIC_CAST(j_decompress_ptr, closure);
  decoder_source_mgr *src = NS_REINTERPRET_CAST(decoder_source_mgr *, jd->src);

  src->pub.next_input_byte = NS_REINTERPRET_CAST(const unsigned char *, fromRawSegment);
  src->pub.bytes_in_buffer = count;

  *writeCount = 0; // pretend we didn't really read anything

  return NS_ERROR_FAILURE; // return error so that we can point to this buffer and exit the ReadSegments loop
}


boolean PR_CALLBACK
fill_input_buffer (j_decompress_ptr jd)
{
  decoder_source_mgr *src = (decoder_source_mgr *)jd->src;

  PRUint32 _retval;

  if (src->decoder->mBytesToSkip != 0) {
    if (src->decoder->mBytesToSkip > src->decoder->mDataLen){
      src->decoder->mInStream->ReadSegments(DiscardData, NS_STATIC_CAST(void*, jd), 
                                            src->decoder->mDataLen, &_retval);
    } else {
      src->decoder->mInStream->ReadSegments(DiscardData, NS_STATIC_CAST(void*, jd), 
                                            src->decoder->mBytesToSkip, &_retval);
    }
    src->decoder->mBytesToSkip -= _retval;
    src->decoder->mDataLen -= _retval;
  }

  if (src->decoder->mDataLen != 0) {
    src->decoder->mInStream->ReadSegments(ReadDataOut, NS_STATIC_CAST(void*, jd), 
                                          src->decoder->mDataLen, &_retval);
    src->decoder->mDataLen -= src->pub.bytes_in_buffer;
    return PR_TRUE;
  } else {
    return PR_FALSE;
  }

}


/*
 * Terminate source --- called by jpeg_finish_decompress() after all
 * data has been read to clean up JPEG source manager.
 */
void PR_CALLBACK
term_source (j_decompress_ptr jd)
{
  decoder_source_mgr *src = (decoder_source_mgr *)jd->src;

  if (src->decoder->mObserver) {
    src->decoder->mObserver->OnStopFrame(nsnull, nsnull, src->decoder->mFrame);
    src->decoder->mObserver->OnStopContainer(nsnull, nsnull, src->decoder->mImage);
    src->decoder->mObserver->OnStopDecode(nsnull, nsnull, NS_OK, nsnull);
  }

    /* No work necessary here */
}
