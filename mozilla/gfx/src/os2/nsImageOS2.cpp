/*
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
 * License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is the Mozilla OS/2 libraries.
 *
 * The Initial Developer of the Original Code is John Fairhurst,
 * <john_fairhurst@iname.com>.  Portions created by John Fairhurst are
 * Copyright (C) 1999 John Fairhurst. All Rights Reserved.
 *
 * Contributor(s): 
 *   Pierre Phaneuf <pp@ludusdesign.com>
 */

#include "nsGfxDefs.h"
#include <stdlib.h>

#include "nsImageOS2.h"
#include "nsIDeviceContext.h"
#include "nsRenderingContextOS2.h"

NS_IMPL_ISUPPORTS(nsImageOS2, NS_GET_IID(nsIImage));

//------------------------------------------------------------
nsImageOS2::nsImageOS2()
{
   NS_INIT_REFCNT();
    
   mInfo      = 0;
   mStride    = 0;
   mImageBits = 0;
   mBitmap    = 0;

   mAStride    = 0;
   mAImageBits = 0;
   mABitmap    = 0;
   mAlphaDepth = 0;

   mColorMap = 0;

   mOptimized = PR_FALSE;

   mDeviceDepth = 0;
}

nsImageOS2::~nsImageOS2()
{
   Cleanup();
}

nsresult nsImageOS2::Init( PRInt32 aWidth, PRInt32 aHeight, PRInt32 aDepth,
                           nsMaskRequirements aMaskRequirements)
{
   // Guard against memory leak in multiple init
   Cleanup();

   // (copying windows code - what about monochrome?  Oh well.)
   NS_ASSERTION( aDepth == 24 || aDepth == 8, "Bad image depth");

   // Work out size of bitmap to allocate
   mStride = RASWIDTH(aWidth,aDepth);

// mStride = aWidth * aDepth;
// if( aDepth < 8)
//    mStride += 7;
// mStride /= 8;
//
// // Make sure image width is 4byte aligned
// mStride = (mStride + 3) & ~0x3;

  SetDecodedRect(0,0,0,0);  //init

   mImageBits = new PRUint8 [ aHeight * mStride ];

   // Set up bitmapinfo header
   int cols = -1;
   if( aDepth == 8) cols = COLOR_CUBE_SIZE;
   else if( aDepth <= 32) cols = 0;

   int szStruct = sizeof( BITMAPINFOHEADER2) + cols * sizeof( RGB2);

   mInfo = (PBITMAPINFO2) calloc( szStruct, 1);
   mInfo->cbFix = sizeof( BITMAPINFOHEADER2);
   mInfo->cx = aWidth;
   mInfo->cy = aHeight;
   mInfo->cPlanes = 1;
   mInfo->cBitCount = (USHORT) aDepth;

   // We can't set up the bitmap colour table yet.

   // init color map.
   // XP will update the color map & then call ImageUpdated(), at which
   // point we can change the color table in the bitmapinfo.
   if( aDepth == 8)
   {
      mColorMap = new nsColorMap;
      mColorMap->NumColors = COLOR_CUBE_SIZE;
      mColorMap->Index = new PRUint8[3 * mColorMap->NumColors];
   }

   // Allocate stuff for mask bitmap
   if( aMaskRequirements != nsMaskRequirements_kNoMask)
   {
      if( aMaskRequirements == nsMaskRequirements_kNeeds1Bit)
      {
         mAStride = (aWidth + 7) / 8;
         mAlphaDepth = 1;
      }
      else
      {
         NS_ASSERTION( nsMaskRequirements_kNeeds8Bit == aMaskRequirements,
                       "unexpected mask depth");
         mAStride = aWidth;
         mAlphaDepth = 8;
      }

      // 32-bit align each row
      mAStride = (mAStride + 3) & ~0x3;

      mAImageBits = new PRUint8 [ aHeight * mAStride];
   }

   return NS_OK;
}

void nsImageOS2::Cleanup()
{
   if( mImageBits) {
      delete [] mImageBits; mImageBits = 0;
   }
   if( mInfo) {
      free( mInfo); mInfo = 0;
   }
   if( mColorMap) {
      if( mColorMap->Index)
         delete [] mColorMap->Index;
      delete mColorMap;
      mColorMap = 0;
   }
   if( mAImageBits) {
      delete [] mAImageBits; mAImageBits = 0;
   }
   if( mBitmap) {
      GpiDeleteBitmap( mBitmap);
      mBitmap = 0;
   }
   if( mABitmap) {
      GpiDeleteBitmap( mABitmap);
      mABitmap = 0;
   }
}

void nsImageOS2::ImageUpdated( nsIDeviceContext *aContext,
                               PRUint8 aFlags, nsRect *aUpdateRect)
{
   // This is where we can set the bitmap colour table, as the XP code
   // has filled in the colour map.  It would be cute to be able to alias
   // the bitmap colour table as the mColorMap->Index thing, but the formats
   // are unfortunately different.  Rats.

   if( aFlags & nsImageUpdateFlags_kColorMapChanged && mInfo->cBitCount == 8)
   {
      PRGB2 pBmpEntry  = mInfo->argbColor;
      PRUint8 *pMapByte = mColorMap->Index;

      for( PRInt32 i = 0; i < mColorMap->NumColors; i++, pBmpEntry++)
      {
         pBmpEntry->bRed   = *pMapByte++;
         pBmpEntry->bGreen = *pMapByte++;
         pBmpEntry->bBlue  = *pMapByte++;
      }

      aContext->GetDepth( mDeviceDepth);
   }
   else if( aFlags & nsImageUpdateFlags_kBitsChanged)
   {
      // jolly good...
   }
}

nsresult nsImageOS2::Draw( nsIRenderingContext &aContext,
                           nsDrawingSurface aSurface,
                           PRInt32 aX, PRInt32 aY,
                           PRInt32 aWidth, PRInt32 aHeight)
{
   return Draw( aContext, aSurface,
                0, 0, mInfo->cx, mInfo->cy,
                aX, aY, aWidth, aHeight);
}

nsresult nsImageOS2::Draw( nsIRenderingContext &aContext,
                           nsDrawingSurface aSurface,
                           PRInt32 aSX, PRInt32 aSY, PRInt32 aSW, PRInt32 aSH,
                           PRInt32 aDX, PRInt32 aDY, PRInt32 aDW, PRInt32 aDH)
{
   // Find target rect in OS/2 coords.
   nsRect trect( aDX, aDY, aDW, aDH);
   RECTL  rcl;
   ((nsRenderingContextOS2 &)aContext).NS2PM_ININ( trect, rcl); // !! !! !!

   nsDrawingSurfaceOS2 *surf = (nsDrawingSurfaceOS2*) aSurface;

   // Set up blit coord array
   POINTL aptl[ 4] = { { rcl.xLeft, rcl.yBottom },
                       { rcl.xRight, rcl.yTop },
                       { aSX, mInfo->cy - aSY - aSH},
                       { aSX + aSW, mInfo->cy - aSY } };

   // Don't bother creating HBITMAPs, just use the pel data to GpiDrawBits
   // at all times.  This (a) makes printing work 'cos bitmaps are
   //                         device-independent.
   //                     (b) is allegedly more efficient...
   //
#if 0
   if( mBitmap == 0 && mOptimized)
   {
      // moz has asked us to optimize this image, but we haven't got
      // round to actually doing it yet.  So do it now.
      CreateBitmaps( surf);
   }
#endif

   if( mAlphaDepth == 0)
   {
      // no transparency, just blit it
      DrawBitmap( surf->mPS, 4, aptl, ROP_SRCCOPY, PR_FALSE);
   }
   else
   {
      // from os2fe/cxdc1.cpp:
      // > the transparent areas of the pixmap are coloured black.
      // > Note this does *not* mean that all black pels are transparent!
      // >
      // > Thus all we need to do is AND the mask onto the target, taking
      // > out pels that are not transparent, and then OR the image onto
      // > the target.
      // >
      // > Note that GPI *ignores* the colour in monochrome bitmaps when
      // > blitting, but uses the actual pel values (indices into cmap)
      // > to do things with.  For 8bpp palette surface, the XP mask is
      // > backwards, so we need a custom ROP.
      // > 
      // > There's probably a really good reason why ROP_SRCAND does the
      // > right thing in true colour...

      #define ROP_NOTSRCAND 0x22 // NOT(SRC) AND DST

      PRBool aBool;
      surf->RequiresInvertedMask( &aBool);

      long lRop = aBool ? ROP_NOTSRCAND : ROP_SRCAND;

      // Apply mask to target, clear pels we will fill in from the image
      DrawBitmap( surf->mPS, 4, aptl, lRop, PR_TRUE);
      // Now combine image with target
      DrawBitmap( surf->mPS, 4, aptl, ROP_SRCPAINT, PR_FALSE);
   }

   return NS_OK;
}

nsresult nsImageOS2::Optimize( nsIDeviceContext* aContext)
{
   // Defer this until we have a PS...
   mOptimized = PR_TRUE;
   return NS_OK;
}

// From os2fe/cxdc1.cpp

static RGB2 rgb2White = { 0xff, 0xff, 0xff, 0 };
static RGB2 rgb2Black = { 0, 0, 0, 0 };

struct MASKBMPINFO
{
   BITMAPINFOHEADER2 bmpInfo;
   RGB2              rgbZero;
   RGB2              rgbOne;

   operator PBITMAPINFO2 ()       { return (PBITMAPINFO2) &bmpInfo; }
   operator PBITMAPINFOHEADER2 () { return &bmpInfo; }

   MASKBMPINFO( PBITMAPINFO2 pBI)
   {
      memcpy( &bmpInfo, pBI, sizeof( BITMAPINFOHEADER2));
      bmpInfo.cBitCount = 1;
      rgbZero = rgb2Black;
      rgbOne = rgb2White;
   }
};

void nsImageOS2::CreateBitmaps( nsDrawingSurfaceOS2 *surf)
{
   mBitmap = GpiCreateBitmap( surf->mPS,
                              (PBITMAPINFOHEADER2) mInfo,
                              CBM_INIT,
                              (PBYTE) mImageBits,
                              mInfo);
   if( mBitmap == GPI_ERROR)
      PMERROR("GpiCreateBitmap");

   if( mAImageBits)
   {
      if( mAlphaDepth == 1)
      {
         MASKBMPINFO maskInfo( mInfo);
         mABitmap = GpiCreateBitmap( surf->mPS,
                                     maskInfo,
                                     CBM_INIT,
                                     (PBYTE) mAImageBits,
                                     maskInfo);
         if( mABitmap == GPI_ERROR)
            PMERROR( "GpiCreateBitmap (mask)");
      }
      else
         printf( "8 bit alpha mask, no chance...\n");
   }
}

void nsImageOS2::DrawBitmap( HPS hps, LONG lCount, PPOINTL pPoints,
                             LONG lRop, PRBool bIsMask)
{
   HBITMAP hBmp = bIsMask ? mABitmap : mBitmap;

#if 0
   if( hBmp)
   {
      if( GPI_ERROR == GpiWCBitBlt( hps, hBmp, lCount, pPoints, lRop, BBO_OR))
         PMERROR( "GpiWCBitBlt");
   }
   else
#endif
   {
      MASKBMPINFO *pMaskInfo = 0;
      PBITMAPINFO2 pBmp2 = mInfo;

      if( PR_TRUE == bIsMask)
      {
         pMaskInfo = new MASKBMPINFO( pBmp2);
         pBmp2 = *pMaskInfo;
      }

      void *pBits = bIsMask ? mAImageBits : mImageBits;

      if( GPI_ERROR == GpiDrawBits( hps, pBits, pBmp2,
                                    lCount, pPoints, lRop, BBO_OR))
         PMERROR( "GpiDrawBits");

      delete pMaskInfo;
   }
}

//------------------------------------------------------------
// lock the image pixels. implement this if you need it
NS_IMETHODIMP
nsImageOS2::LockImagePixels(PRBool aMaskPixels)
{
  return NS_OK;
}

//------------------------------------------------------------
// unlock the image pixels. implement this if you need it
NS_IMETHODIMP
nsImageOS2::UnlockImagePixels(PRBool aMaskPixels)
{
  return NS_OK;
} 

// ---------------------------------------------------
//	Set the decoded dimens of the image
//
NS_IMETHODIMP
nsImageOS2::SetDecodedRect(PRInt32 x1, PRInt32 y1, PRInt32 x2, PRInt32 y2 )
{
    
  mDecodedX1 = x1; 
  mDecodedY1 = y1; 
  mDecodedX2 = x2; 
  mDecodedY2 = y2; 
  return NS_OK;
}

