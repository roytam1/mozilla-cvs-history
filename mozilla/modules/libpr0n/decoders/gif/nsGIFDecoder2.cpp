/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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

#include "nsGIFDecoder2.h"
#include "nsIInputStream.h"
#include "nsIComponentManager.h"
#include "nsIImageFrame.h"
#include "nsMemory.h"

#ifndef XP_MAC
#define FOO 1
#endif

#ifdef FOO
#include "nsRect.h"
#else
#include "nsRect2.h"
#endif

//////////////////////////////////////////////////////////////////////
// GIF Decoder Implementation
// This is an adaptor between GIF2 and nsIImageDecoder

NS_IMPL_ISUPPORTS2(nsGIFDecoder2, nsIImageDecoder, nsIOutputStream);

nsGIFDecoder2::nsGIFDecoder2()
{
  NS_INIT_ISUPPORTS();
  memset(&mGIFStruct, 0, sizeof(gif_struct));
}

nsGIFDecoder2::~nsGIFDecoder2(void)
{
}

//******************************************************************************
/** nsIImageDecoder methods **/
//******************************************************************************

//******************************************************************************
/* void init (in nsIImageRequest aRequest); */
NS_IMETHODIMP nsGIFDecoder2::Init(nsIImageRequest *aRequest)
{
  mImageRequest = aRequest;
  mObserver = do_QueryInterface(aRequest);  // we're holding 2 strong refs to the request.

  aRequest->GetImage(getter_AddRefs(mImageContainer));

  /* do gif init stuff */
  /* Always decode to 24 bit pixdepth */
  
  // Call GIF decoder init routine
  GIFInit(
    this,
    &mGIFStruct,
    NewPixmap,
    BeginGIF,
    EndGIF,
    BeginImageFrame,
    EndImageFrame,
    SetupColorspaceConverter,
    ResetPalette,
    InitTransparentPixel,
    DestroyTransparentPixel,
    HaveDecodedRow,
    HaveImageAll);
  return NS_OK;
}

//******************************************************************************
/* readonly attribute nsIImageRequest request; */
NS_IMETHODIMP nsGIFDecoder2::GetRequest(nsIImageRequest * *aRequest)
{
  *aRequest = mImageRequest;
  NS_IF_ADDREF(*aRequest);
      
  return NS_OK;
}


//******************************************************************************
/** nsIOutputStream methods **/
//******************************************************************************

//******************************************************************************
/* void close (); */
NS_IMETHODIMP nsGIFDecoder2::Close()
{
  //if (mPNG)
  //  png_destroy_read_struct(&mPNG, mInfo ? &mInfo : NULL, NULL);

  return NS_OK;
}

//******************************************************************************
/* void flush (); */
NS_IMETHODIMP nsGIFDecoder2::Flush()
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

//******************************************************************************
/* unsigned long write (in string buf, in unsigned long count); */
NS_IMETHODIMP nsGIFDecoder2::Write(const char *buf, PRUint32 count, PRUint32 *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

//******************************************************************************
/* static callback from nsIInputStream::ReadSegments */
static NS_METHOD ReadDataOut(nsIInputStream* in,
                             void* closure,
                             const char* fromRawSegment,
                             PRUint32 toOffset,
                             PRUint32 count,
                             PRUint32 *writeCount)
{
  nsGIFDecoder2 *decoder = NS_STATIC_CAST(nsGIFDecoder2*, closure);
  *writeCount = decoder->ProcessData((unsigned char*)fromRawSegment, count);
  return NS_OK;
}

//******************************************************************************
PRUint32 nsGIFDecoder2::ProcessData(unsigned char *data, PRUint32 count)
{
  // Push the data to the GIF decoder
  // png_process_data(mPNG, mInfo, data, count);
  
  // First we ask if the gif decoder is ready for more data, and if so, push it.
  // In the new decoder, we should always be able to process more data since
  // we don't wait to decode each frame in an animation now.
  if(gif_write_ready(&mGIFStruct)) {
    gif_write(&mGIFStruct, data, count);
  }
    

  return count; // we always consume all the data
}

//******************************************************************************
/* unsigned long writeFrom (in nsIInputStream inStr, in unsigned long count); */
NS_IMETHODIMP nsGIFDecoder2::WriteFrom(nsIInputStream *inStr, PRUint32 count, PRUint32 *_retval)
{
//  PRUint32 sourceOffset = *_retval;

  //if (setjmp(mPNG->jmpbuf)) {
    //png_destroy_read_struct(&mPNG, &mInfo, NULL);
    // is this NS_ERROR_FAILURE enough?

    //mRequest->Cancel(NS_BINDING_ABORTED); // XXX is this the correct error ?

    //return NS_ERROR_FAILURE;
  //}

  inStr->ReadSegments(
    ReadDataOut, // Callback
    this,     
    count, 
    _retval);

  return NS_OK;
}

//******************************************************************************
/* [noscript] unsigned long writeSegments (in nsReadSegmentFun reader, in voidPtr closure, in unsigned long count); */
NS_IMETHODIMP nsGIFDecoder2::WriteSegments(nsReadSegmentFun reader, void * closure, PRUint32 count, PRUint32 *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

//******************************************************************************
/* attribute boolean nonBlocking; */
NS_IMETHODIMP nsGIFDecoder2::GetNonBlocking(PRBool *aNonBlocking)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
//******************************************************************************
NS_IMETHODIMP nsGIFDecoder2::SetNonBlocking(PRBool aNonBlocking)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

//******************************************************************************
/* attribute nsIOutputStreamObserver observer; */
NS_IMETHODIMP nsGIFDecoder2::GetObserver(nsIOutputStreamObserver * *aObserver)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

//******************************************************************************
NS_IMETHODIMP nsGIFDecoder2::SetObserver(nsIOutputStreamObserver * aObserver)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}



//******************************************************************************
// GIF decoder callback methods. Part of pulic API for GIF2
//******************************************************************************

//******************************************************************************
int BeginGIF(
  void*    aClientData,
  PRUint32 aLogicalScreenWidth, 
  PRUint32 aLogicalScreenHeight,
  GIF_RGB* aBackgroundRGB,
  GIF_RGB* aTransparencyChromaKey)
{
  // copy GIF info into imagelib structs
  nsGIFDecoder2 *decoder = NS_STATIC_CAST(nsGIFDecoder2*, aClientData);

  if (decoder->mObserver)
    decoder->mObserver->OnStartDecode(nsnull, nsnull);

  decoder->mImageContainer->Init(aLogicalScreenWidth, aLogicalScreenHeight);

  if (decoder->mObserver)
    decoder->mObserver->OnStartContainer(nsnull, nsnull, decoder->mImageContainer);

  return 0;
}

//******************************************************************************
int EndGIF()
{
  return 0;
}

//******************************************************************************
int BeginImageFrame(
  void*    aClientData,
  PRUint32 aFrameNumber,   /* Frame number, 1-n */
  PRUint32 aFrameXOffset,  /* X offset in logical screen */
  PRUint32 aFrameYOffset,  /* Y offset in logical screen */
  PRUint32 aFrameWidth,    
  PRUint32 aFrameHeight,   
  GIF_RGB* aTransparencyChromaKey) /* don't have this info yet */
{
  nsGIFDecoder2* decoder = NS_STATIC_CAST(nsGIFDecoder2*, aClientData);
  
  decoder->mGIFStruct.x_offset = aFrameXOffset;
  decoder->mGIFStruct.y_offset = aFrameYOffset;
  decoder->mGIFStruct.width = aFrameWidth;
  decoder->mGIFStruct.height = aFrameHeight;

  return 0;
}

//******************************************************************************
int EndImageFrame(
  void*    aClientData, 
  PRUint32 aDelayTimeout)  /* Time this frame should be displayed before the next frame 
                              we can't have this in the image frame init because it doesn't
                              show up in the GIF frame header, it shows up in a sub control
                              block.*/
{
  nsGIFDecoder2* decoder = NS_STATIC_CAST(nsGIFDecoder2*, aClientData);

  return 0;
}
  


//******************************************************************************
// GIF decoder callback
int HaveImageAll(
  void* aClientData)
{
  nsGIFDecoder2* decoder = NS_STATIC_CAST(nsGIFDecoder2*, aClientData);
  if (decoder->mObserver) {
    decoder->mObserver->OnStopFrame(nsnull, nsnull, decoder->mImageFrame);
    decoder->mObserver->OnStopContainer(nsnull, nsnull, decoder->mImageContainer);
    decoder->mObserver->OnStopDecode(nsnull, nsnull, NS_OK, nsnull);
  }
  return 0;
}

//******************************************************************************
// GIF decoder callback notification that it has decoded a row
int HaveDecodedRow(
  void* aClientData,
  PRUint8* aRowBufPtr,   // Pointer to single scanline temporary buffer
  PRUint8* aRGBrowBufPtr,// Pointer to temporary storage for dithering/mapping
  int aXOffset,          // With respect to GIF logical screen origin
  int aLength,           // Length of the row?
  int aRowNumber,        // Row number?
  int aDuplicateCount,   // Number of times to duplicate the row?
  PRUint8 aDrawMode,     // il_draw_mode
  int aInterlacePass)    // interlace pass (1-4)
{
  nsGIFDecoder2* decoder = NS_STATIC_CAST(nsGIFDecoder2*, aClientData);
  PRUint32 bpr, abpr;
  // We have to delay allocation of the image frame until now because
  // we won't have control block info (transparency) until now. The conrol
  // block of a GIF stream shows up after the image header since transparency
  // is added in GIF89a and control blocks are how the extensions are done.
  // How annoying.
  if(! decoder->mImageFrame) {
      gfx_format format = nsIGFXFormat::RGB;
      if (decoder->mGIFStruct.is_transparent)
        format = nsIGFXFormat::RGB_A1;

#ifdef XP_PC
      // XXX this works...
      format += 1; // RGB to BGR
#endif

      // initalize the frame and append it to the container
      decoder->mImageFrame = do_CreateInstance("@mozilla.org/gfx/image/frame;2");
      decoder->mImageFrame->Init(
        decoder->mGIFStruct.x_offset, decoder->mGIFStruct.y_offset, 
        decoder->mGIFStruct.width, decoder->mGIFStruct.height, format);
        
      decoder->mImageContainer->AppendFrame(decoder->mImageFrame);

      if (decoder->mObserver)
        decoder->mObserver->OnStartFrame(nsnull, nsnull, decoder->mImageFrame);


      decoder->mImageFrame->GetImageBytesPerRow(&bpr);
      decoder->mImageFrame->GetAlphaBytesPerRow(&abpr);
      decoder->colorLine = (PRUint8 *)nsMemory::Alloc(bpr);
      if (format == nsIGFXFormat::RGB_A1)
        decoder->alphaLine = (PRUint8 *)nsMemory::Alloc(abpr);
  } else {
    decoder->mImageFrame->GetImageBytesPerRow(&bpr);
    decoder->mImageFrame->GetAlphaBytesPerRow(&abpr);
  }
  
  PRUint32 length;
  PRUint8 *bits;
  decoder->mImageFrame->GetImageData(&bits, &length);

  if (aRowBufPtr) {
#ifdef FOO
    nscoord width;
#else
    gfx_dimension width;
#endif
    decoder->mImageFrame->GetWidth(&width);
    PRUint32 iwidth = width;

    gfx_format format;
    decoder->mImageFrame->GetFormat(&format);
    PRUint8 *aptr, *cptr;

        // XXX map the data into colors
        int cmapsize;
        GIF_RGB* cmap;
        if(decoder->mGIFStruct.local_colormap) {
          cmapsize = decoder->mGIFStruct.local_colormap_size;
          cmap = decoder->mGIFStruct.local_colormap;
        } else {
          cmapsize = decoder->mGIFStruct.global_colormap_size;
          cmap = decoder->mGIFStruct.global_colormap;
        }
      
        PRUint8* rgbRowIndex = aRGBrowBufPtr;
        PRUint8* rowBufIndex = aRowBufPtr;
        
    switch (format) {
    case nsIGFXFormat::RGB:
    case nsIGFXFormat::BGR:
      {
        while(rowBufIndex != decoder->mGIFStruct.rowend) {
          *rgbRowIndex++ = cmap[PRUint8(*rowBufIndex)].blue; //XXX off by one at start, alignment I think
          *rgbRowIndex++ = cmap[PRUint8(*rowBufIndex)].red;
          *rgbRowIndex++ = cmap[PRUint8(*rowBufIndex)].green;
          *rgbRowIndex++ = 0;// pad XXX why do I need to pad data that I say is 3 bytes?
          ++rowBufIndex;
        }

        decoder->mImageFrame->SetImageData((PRUint8*)aRGBrowBufPtr, bpr, aRowNumber*bpr);
      }
      break;
    case nsIGFXFormat::RGB_A1:
    case nsIGFXFormat::BGR_A1:
      {
        memset(aptr, 0, abpr);
        for (PRUint32 x=0; x<iwidth; x++) {
          if(*rowBufIndex == decoder->mGIFStruct.tpixel) {
            // set mask bit
          }
          *rgbRowIndex++ = cmap[PRUint8(*rowBufIndex)].blue; //XXX off by one at start, alignment I think
          *rgbRowIndex++ = cmap[PRUint8(*rowBufIndex)].red;
          *rgbRowIndex++ = cmap[PRUint8(*rowBufIndex)].green;
        }
        decoder->mImageFrame->SetImageData(decoder->colorLine, bpr, aRowNumber*bpr);
        // decoder->mImageFrame->SetAlphaData(decoder->alphaLine, abpr, aRowNumber*abpr);
      }
      break;
    default:
      break;

    }

#ifdef FOO
    nsRect r(0, aRowNumber, width, 1);
#else
    nsRect2 r(0, aRowNumber, width, 1);
#endif
    decoder->mObserver->OnDataAvailable(nsnull, nsnull, decoder->mImageFrame, &r);
  }

  return 0;
}

//******************************************************************************
int ResetPalette()
{
  return 0;
}

//******************************************************************************
int SetupColorspaceConverter()
{
  return 0;
}

//******************************************************************************
int EndImageFrame()
{
  return 0;
}

//******************************************************************************
int NewPixmap()
{
  return 0;
}

//******************************************************************************
int InitTransparentPixel()
{
  return 0;
}

//******************************************************************************
int DestroyTransparentPixel()
{
  return 0;
}
