/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */
#include "nsImageMac.h"
#include "nsRenderingContextMac.h"
#include "nsDeviceContextMac.h"

#include "nspr.h"

#define IsFlagSet(a,b) (a & b)

static NS_DEFINE_IID(kIImageIID, NS_IIMAGE_IID);

//------------------------------------------------------------

nsImageMac :: nsImageMac()
{

	NS_INIT_REFCNT();
	mThePixelmap.baseAddr = nsnull;
	mImageBits = nsnull;
  mWidth = 0;
  mHeight = 0;	
}

//------------------------------------------------------------

nsImageMac :: ~nsImageMac()
{
	if(mImageBits)
		delete[] mImageBits;
}

NS_IMPL_ISUPPORTS(nsImageMac, kIImageIID);

//------------------------------------------------------------

PRUint8* nsImageMac::GetBits()
{ 
	return mImageBits; 
}

//------------------------------------------------------------

nsresult nsImageMac :: Init(PRInt32 aWidth, PRInt32 aHeight, PRInt32 aDepth,nsMaskRequirements aMaskRequirements)
{
PRInt32	bufferdepth;

	if(nsnull==mImageBits)
		{
		delete [] mImageBits;
		}

		switch(aDepth)
			{
			case 8:
				mThePixelmap.pixelType = chunky;
				mThePixelmap.cmpCount = 1;
				mThePixelmap.cmpSize = 8;
				mThePixelmap.pmTable = GetCTable(8);
				bufferdepth = 8;
				break;
			case 16:
				mThePixelmap.pixelType = chunky;
				mThePixelmap.cmpCount = 3;
				mThePixelmap.cmpSize = 5;
				mThePixelmap.pmTable = 0;
				bufferdepth = 16;
				break;
			case 24:			// 24 and 32 bit are basically the same
			case 32:
				mThePixelmap.pixelType = chunky;
				mThePixelmap.cmpCount = 3;
				mThePixelmap.cmpSize = 8;
				mThePixelmap.pmTable = 0;
				bufferdepth = 32;
				break;
			default:
				mThePixelmap.cmpCount = 0;
				break;
			}
	
	if(mThePixelmap.cmpCount)
		{
		mRowBytes = CalcBytesSpan(aWidth,bufferdepth);
		mSizeImage = mRowBytes*aHeight;
		mImageBits = new unsigned char[mSizeImage];
		}
		
	if(mImageBits)
		{
		// we are cool
		mThePixelmap.baseAddr = (char*) mImageBits;
		mThePixelmap.rowBytes = mRowBytes | 0x8000;
		mThePixelmap.bounds.top = 0;
		mThePixelmap.bounds.left = 0;
		mThePixelmap.bounds.bottom = aHeight;
		mThePixelmap.bounds.right = aWidth;
		mThePixelmap.pixelSize = bufferdepth;
		mThePixelmap.packType = 0;
		mThePixelmap.packSize = 0;
		mThePixelmap.hRes = 72<<16;
		mThePixelmap.vRes = 72<<16;
		mThePixelmap.planeBytes = 0;
		mThePixelmap.pmReserved = 0;
		mThePixelmap.pmVersion = 0;
		mWidth = aWidth;
		mHeight = aHeight;
		}


    // Allocate mask image bits if requested
    if (aMaskRequirements != nsMaskRequirements_kNoMask)
    {
      if (nsMaskRequirements_kNeeds1Bit == aMaskRequirements)
      {
        mARowBytes = (aWidth + 7) / 8;
        mAlphaDepth = 1;
      }
      else
      {
        NS_ASSERTION(nsMaskRequirements_kNeeds8Bit == aMaskRequirements,
                     "unexpected mask depth");
        mARowBytes = aWidth;
        mAlphaDepth = 8;
      }

      // 32-bit align each row
      mARowBytes = (mARowBytes + 3) & ~0x3;

      mAlphaBits = new unsigned char[mARowBytes * aHeight];
      mAlphaWidth = aWidth;
      mAlphaHeight = aHeight;
    }
    else
    {
      mAlphaBits = nsnull;
      mAlphaWidth = 0;
      mAlphaHeight = 0;
    }





  return NS_OK;
}

//------------------------------------------------------------

PRInt32  nsImageMac :: CalcBytesSpan(PRUint32  aWidth,PRUint32	aDepth)
{
PRInt32 spanbytes;

  spanbytes = (aWidth * aDepth) >> 5;

  if (((PRUint32)aWidth * aDepth) & 0x1F) 
    spanbytes++;
  spanbytes <<= 2;
  return(spanbytes);
  return 0;
}

//------------------------------------------------------------

// set up the pallete to the passed in color array, RGB only in this array
void nsImageMac :: ImageUpdated(nsIDeviceContext *aContext, PRUint8 aFlags, nsRect *aUpdateRect)
{

  /*if (nsnull == mImage)
    return;

  if (IsFlagSet(nsImageUpdateFlags_kBitsChanged, aFlags)){
  }
*/
}


//------------------------------------------------------------

// Draw the bitmap, this method has a source and destination coordinates
PRBool nsImageMac :: Draw(nsIRenderingContext &aContext, nsDrawingSurface aSurface, PRInt32 aSX, PRInt32 aSY, PRInt32 aSWidth, PRInt32 aSHeight,
                          PRInt32 aDX, PRInt32 aDY, PRInt32 aDWidth, PRInt32 aDHeight)
{
PixMapPtr	destpix;
RGBColor	rgbblack = {0x0000,0x0000,0x0000};
RGBColor	rgbwhite = {0xFFFF,0xFFFF,0xFFFF};
Rect			srcrect,dstrect;

  if (nsnull == mThePixelmap.baseAddr)
    return PR_FALSE;

	::SetRect(&srcrect,aSX,aSY,aSX+aSWidth,aSY+aSHeight);
	::SetRect(&dstrect,aDX,aDY,aDX+aDWidth,aDY+aDHeight);

	destpix = *((CGrafPtr)aSurface)->portPixMap;

	::RGBForeColor(&rgbblack);
	::RGBBackColor(&rgbwhite);
	
	::CopyBits((BitMap*)&mThePixelmap,(BitMap*)destpix,&srcrect,&dstrect,ditherCopy,0L);

  return PR_TRUE;
}

//------------------------------------------------------------

// Draw the bitmap, this draw just has destination coordinates
PRBool nsImageMac :: Draw(nsIRenderingContext &aContext, 
                       nsDrawingSurface aSurface,
                       PRInt32 aX, PRInt32 aY, 
                       PRInt32 aWidth, PRInt32 aHeight)
{

  return Draw(aContext,aSurface,0,0,mWidth,mHeight,aX,aY,aWidth,aHeight);
}

//------------------------------------------------------------

void nsImageMac::CompositeImage(nsIImage *aTheImage, nsPoint *aULLocation,nsBlendQuality aBlendQuality)
{

}

//------------------------------------------------------------

// lets build an alpha mask from this image
PRBool nsImageMac::SetAlphaMask(nsIImage *aTheMask)
{
  return(PR_FALSE);
}

//------------------------------------------------------------

nsresult nsImageMac::Optimize(nsIDeviceContext* aContext)
{
  return NS_OK;
}

//------------------------------------------------------------
