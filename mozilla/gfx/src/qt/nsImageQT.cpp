/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is 
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *		John C. Griggs <johng@corel.com>
 *
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

#include "nsImageQT.h"
#include "nsRenderingContextQT.h"

#include "nspr.h"
#include "qtlog.h"

#define IsFlagSet(a,b) ((a) & (b))

#ifdef DEBUG
PRUint32 gImageCount = 0;
PRUint32 gImageID = 0;
#endif

NS_IMPL_ISUPPORTS1(nsImageQT, nsIImage)
 
//------------------------------------------------------------
nsImageQT::nsImageQT()
: mImageBits(nsnull)
, mWidth(0)
, mHeight(0)
, mDepth(0)
, mRequestDepth(0)
, mAlphaBits(nsnull)
, mAlphaPixmap(nsnull)
, mImagePixmap(nsnull)
, mAlphaDepth(0)
, mRowBytes(0)
, mSizeImage(0)
, mAlphaRowBytes(0)
, mNumBytesPixel(0)
{
#ifdef DEBUG
  gImageCount++;
  mID = gImageID++;
  PR_LOG(gQTLogModule, QT_BASIC,
      ("nsImageQT CTOR (%p) ID: %d, Count: %d\n", this, mID, gImageCount));
#endif
}

//------------------------------------------------------------
nsImageQT::~nsImageQT()
{
#ifdef DEBUG
  gImageCount--;
  PR_LOG(gQTLogModule, QT_BASIC,
      ("nsImageQT DTOR (%p) ID: %d, Count: %d\n", this, mID, gImageCount));
#endif
  if (nsnull != mImageBits) {
    delete[] (PRUint8*)mImageBits;
    mImageBits = nsnull;
  }
  if (nsnull != mAlphaBits) {
    delete[] (PRUint8*)mAlphaBits;
    mAlphaBits = nsnull;
  }
  if (nsnull != mAlphaPixmap) {
    delete mAlphaPixmap;
  }
  if (nsnull != mImagePixmap) {
    delete mImagePixmap;
  }
}

//------------------------------------------------------------
nsresult nsImageQT::Init(PRInt32 aWidth,PRInt32 aHeight,
                         PRInt32 aDepth, 
                         nsMaskRequirements aMaskRequirements)
{
    // gfxImageFrame forces only one nsImageQT::Init
    if (aWidth == 0 || aHeight == 0) {
        return NS_ERROR_FAILURE;
    }
    // mImagePixmap gets created once per unique image bits in Draw()
    // ImageUpdated(nsImageUpdateFlags_kBitsChanged) can cause the
    // image bits to change and mImagePixmap will be unrefed and nulled.
    if (nsnull != mImagePixmap) {
        delete mImagePixmap;
        mImagePixmap = nsnull;
    }
    if (32 == aDepth) {
        mNumBytesPixel = 4;
        mDepth = aDepth;
        mRequestDepth = mDepth;
    }
    else if (24 == aDepth) {
        mNumBytesPixel = 4;
        mDepth = 32;
        mRequestDepth = aDepth;
    }
    else {
        NS_NOTREACHED("unexpected image depth");
        return NS_ERROR_UNEXPECTED;
    }
    mWidth = aWidth;
    mHeight = aHeight;

    // create the memory for the image
    ComputeMetrics();

    mImageBits = (PRUint8*)new PRUint8[mSizeImage];

    switch (aMaskRequirements) {
      case nsMaskRequirements_kNeeds1Bit:
        mAlphaRowBytes = (aWidth + 7) / 8;
        mAlphaDepth = 1;

        // 32-bit align each row
        mAlphaRowBytes = (mAlphaRowBytes + 3) & ~0x3;

        mAlphaBits = new PRUint8[mAlphaRowBytes * aHeight];
        break;

      case nsMaskRequirements_kNeeds8Bit:
        mAlphaRowBytes = aWidth;
        mAlphaDepth = 8;
 
        // 32-bit align each row
        mAlphaRowBytes = (mAlphaRowBytes + 3) & ~0x3;
        mAlphaBits = new PRUint8[mAlphaRowBytes * aHeight];
        break;
    }
    PR_LOG(gQTLogModule, QT_BASIC, ("nsImageQT::Init succeeded"));
    return NS_OK;
}

//------------------------------------------------------------

void nsImageQT::ComputeMetrics()
{
    mRowBytes = CalcBytesSpan(mWidth);
    mSizeImage = mRowBytes * mHeight;
}

PRInt32 nsImageQT::GetHeight()
{
    return mHeight;
}

PRInt32 nsImageQT::GetWidth()
{
    return mWidth;
}

PRUint8 *nsImageQT::GetBits()
{
    return mImageBits;
}

void *nsImageQT::GetBitInfo()
{
    return nsnull;
}

PRInt32 nsImageQT::GetLineStride()
{
    return mRowBytes;
}

nsColorMap *nsImageQT::GetColorMap()
{
    return nsnull;
}

PRUint8 *nsImageQT::GetAlphaBits()
{
    return mAlphaBits;
}

PRInt32 nsImageQT::GetAlphaLineStride()
{
    return mAlphaRowBytes;
}

//------------------------------------------------------------

PRInt32 nsImageQT::CalcBytesSpan(PRUint32  aWidth)
{
    PRInt32 spanbytes;

    spanbytes = (aWidth * mDepth) >> 5;

    if (((PRUint32)aWidth * mDepth) & 0x1F) {
        spanbytes++;
    }
    spanbytes <<= 2;
    return(spanbytes);
}

//------------------------------------------------------------

// set up the palette to the passed in color array, RGB only in this array
void nsImageQT::ImageUpdated(nsIDeviceContext *aContext,
                             PRUint8 aFlags,nsRect *aUpdateRect)
{
    if (IsFlagSet(nsImageUpdateFlags_kBitsChanged,aFlags)) {
        if (nsnull != mAlphaPixmap) {
            delete mAlphaPixmap;
            mAlphaPixmap = nsnull;
        }
        // mImagePixmap gets created once per unique image bits in Draw()
        // ImageUpdated(nsImageUpdateFlags_kBitsChanged) can cause the
        // image bits to change and mImagePixmap will be unrefed and nulled.
        if (nsnull != mImagePixmap) {
            delete mImagePixmap;
            mImagePixmap = nsnull;
        }
    }

    mDecodedRect.UnionRect(mDecodedRect, *aUpdateRect);
}

// Draw the bitmap, this method has a source and destination coordinates
NS_IMETHODIMP nsImageQT::Draw(nsIRenderingContext &aContext, 
                              nsDrawingSurface aSurface,
                              PRInt32 aSX, PRInt32 aSY, 
                              PRInt32 aSWidth, PRInt32 aSHeight,
                              PRInt32 aDX, PRInt32 aDY, 
                              PRInt32 aDWidth, PRInt32 aDHeight)
{
    if (nsnull == aSurface) {
        return NS_ERROR_FAILURE;
    }
    // Render unique image bits onto an off screen pixmap only once
    // The image bits can change as a result of ImageUpdated() - for
    // example: animated GIFs.
    if (nsnull == mImagePixmap) {
      CreateImagePixmap();
    }
    if (nsnull == mImagePixmap)
        return NS_ERROR_FAILURE;

    // Copy data from mImageBits to the drawing surface specified by aSurface.
    // We only want a subset of the data in mImageBits, and the data might
    // need to be scaled.
    nsDrawingSurfaceQT *drawing = (nsDrawingSurfaceQT*)aSurface;

    nsRect sourceRect(aSX, aSY, aSWidth, aSHeight);
    if (!sourceRect.IntersectRect(sourceRect, mDecodedRect))
      return NS_OK;

    // Now get the part of the image that should be drawn
    // Copy into a new image so we can scale afterwards
    QImage image(mImagePixmap->copy(sourceRect.x, sourceRect.y,
                                    sourceRect.width, sourceRect.height));
    if (image.isNull())
      return NS_ERROR_FAILURE;

    // Find the scale factor
    float w_factor = (float)aDWidth / aSWidth;
    float h_factor = (float)aDHeight / aSHeight;

    // If we had to draw only part of the requested image, must adjust
    // destination coordinates
    aDX += PRInt32((sourceRect.x - aSX) * w_factor);
    aDY += PRInt32((sourceRect.y - aSY) * h_factor);

    image = image.scale(int(sourceRect.width * w_factor), int(sourceRect.height * h_factor));

    drawing->GetGC()->drawImage(QPoint(aDX, aDY), image);
    return NS_OK;
}

//------------------------------------------------------------

// Draw the bitmap, this draw just has destination coordinates
NS_IMETHODIMP nsImageQT::Draw(nsIRenderingContext &aContext,
                              nsDrawingSurface aSurface,
                              PRInt32 aX, PRInt32 aY,
                              PRInt32 aWidth, PRInt32 aHeight)
{
  return Draw(aContext, aSurface, 0, 0, mWidth, mHeight, aX, aY, aWidth, aHeight);
}

void nsImageQT::CreateOffscreenPixmap(PRInt32 aWidth,PRInt32 aHeight)
{
  if (mImagePixmap)
    delete mImagePixmap;
  mImagePixmap = new QImage(aWidth,aHeight,mDepth);
  if (mImagePixmap) {
    mImagePixmap->setAlphaBuffer(PR_TRUE);
    mImagePixmap->fill(0);
  }
}

void nsImageQT::CreateImagePixmap()
{
  mImagePixmap = new QImage(mWidth,mHeight,mDepth);

  if (mImagePixmap) {
    PRInt8 bytesPerPixel = mRequestDepth / 8;
    PRUint8 *alpha = mAlphaBits;
    PRUint8 *image = mImageBits;
    PRUint8 *imagePtr;
    PRUint32 pixel;
    PRInt32 i,j;
    QRgb *line;

    mImagePixmap->setAlphaBuffer(PR_TRUE);
    for (i = 0; i < mHeight; i++) {
      line = (QRgb*)mImagePixmap->scanLine(i);

      imagePtr = image;
      for (j = 0; j < mWidth; j++) {
        pixel = 0xFF000000 | *(imagePtr + bytesPerPixel - 1)
                 | (*(imagePtr + bytesPerPixel - 2) << 8)
                  | (*(imagePtr + bytesPerPixel - 3) << 16);
        if (mAlphaBits) {
          if (mAlphaDepth == 1) {
            if (!(alpha[j / 8] & (1 << (7 - (j % 8))))) {
              pixel &= 0x00FFFFFF;
            }
          }
          else {
            pixel &= (alpha[j] << 24);
          }
        }
        else if (bytesPerPixel == 4)
          pixel &= (*(imagePtr + bytesPerPixel - 4) << 24);
                 
        line[j] = pixel;
        imagePtr += bytesPerPixel;
      }
      alpha += mAlphaRowBytes;
      image += mRowBytes;
    }
  }
}

NS_IMETHODIMP nsImageQT::DrawTile(nsIRenderingContext &aContext,
                                  nsDrawingSurface aSurface,
                                  nsRect &aSrcRect, nsRect &aTileRect)
{
  nsDrawingSurfaceQT *drawing = (nsDrawingSurfaceQT*)aSurface;

  if (aTileRect.width <= 0 || aTileRect.height <= 0) {
    NS_ASSERTION(aTileRect.width > 0 && aTileRect.height > 0,
                 "Error: image has 0 width or height!");
    return NS_OK;
  }
  if (drawing->GetDepth() == 8 || mAlphaDepth == 8) {
    PRInt32 aY0 = aTileRect.y, aX0 = aTileRect.x;
    PRInt32 aY1 = aTileRect.y + aTileRect.height;
    PRInt32 aX1 = aTileRect.x + aTileRect.width;
 
    for (PRInt32 y = aY0; y < aY1; y += aSrcRect.height)
      for (PRInt32 x = aX0; x < aX1; x += aSrcRect.width)
        Draw(aContext,aSurface,x,y, PR_MIN(aSrcRect.width, aX1 - x),
             PR_MIN(aSrcRect.height, aY1 - y));
 
    return NS_OK;
  }
  // Render unique image bits onto an off screen pixmap only once
  // The image bits can change as a result of ImageUpdated() - for
  // example: animated GIFs.
  if (nsnull == mImagePixmap) {
    CreateImagePixmap();
  }
  if (nsnull == mImagePixmap)
    return NS_ERROR_FAILURE;

  QPixmap qPmap;

  qPmap.convertFromImage(*mImagePixmap);
  drawing->GetGC()->drawTiledPixmap(aTileRect.x,aTileRect.y,
                                    aTileRect.width,aTileRect.height,
                                    qPmap,aSrcRect.x,aSrcRect.y);
  return NS_OK;
}

NS_IMETHODIMP nsImageQT::DrawTile(nsIRenderingContext &aContext,
                                  nsDrawingSurface aSurface,
                                  PRInt32 aSXOffset, PRInt32 aSYOffset,
                                  const nsRect &aTileRect)
{
  nsRect srcRect(aSXOffset,aSYOffset,mWidth,mHeight);
  nsRect dstRect(aTileRect);

  return DrawTile(aContext, aSurface, srcRect, dstRect);
}

//------------------------------------------------------------

nsresult nsImageQT::Optimize(nsIDeviceContext* aContext)
{
    return NS_OK;
}

PRInt32 nsImageQT::GetBytesPix()
{
    return mNumBytesPixel;
}
 
//------------------------------------------------------------
// lock the image pixels. Implement this if you need it.
NS_IMETHODIMP
nsImageQT::LockImagePixels(PRBool aMaskPixels)
{
  return NS_OK;
}

//------------------------------------------------------------
// unlock the image pixels.  Implement this if you need it.
NS_IMETHODIMP
nsImageQT::UnlockImagePixels(PRBool aMaskPixels)
{
  return NS_OK;
}

NS_IMETHODIMP nsImageQT::DrawToImage(nsIImage *aDstImage,
                                     nscoord aDX,nscoord aDY,
                                     nscoord aDWidth,nscoord aDHeight)
{
  nsImageQT *dest = NS_STATIC_CAST(nsImageQT*,aDstImage);
 
  if (!dest)
    return NS_ERROR_FAILURE;
 
  if (!mImagePixmap)
    CreateImagePixmap();
 
  if (!mImagePixmap)
    return NS_ERROR_FAILURE;
 
  if (!dest->mImagePixmap)
    dest->CreateOffscreenPixmap(dest->mWidth,dest->mHeight);
 
  if (!dest->mImagePixmap)
    return NS_ERROR_FAILURE;
 
  bitBlt(dest->mImagePixmap,aDX,aDY,mImagePixmap,0,0,mWidth,mHeight,
         Qt::CopyROP);
 
  return NS_OK;
}

