/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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

#include "nsImagePh.h"
#include "nsRenderingContextPh.h"
#include "nsPhGfxLog.h"
#include "nsDeviceContextPh.h"
#include "nspr.h"
#include "photon/PhRender.h"

//#define ALLOW_PHIMAGE_CACHEING
#define IsFlagSet(a,b) (a & b)

// static NS_DEFINE_IID(kIImageIID, NS_IIMAGE_IID);

NS_IMPL_ISUPPORTS1(nsImagePh, nsIImage)

#define IMAGE_SHMEM				0x1
#define IMAGE_SHMEM_THRESHOLD	4096

// ----------------------------------------------------------------
nsImagePh :: nsImagePh()
{
	NS_INIT_REFCNT();
	mImageBits = nsnull;
	mWidth = 0;
	mHeight = 0;
	mDepth = 0;
	mAlphaBits = nsnull;
	mAlphaDepth = 0;
	mRowBytes = 0;
	mSizeImage = 0;
	mAlphaHeight = 0;
	mAlphaWidth = 0;
	mConvertedBits = nsnull;
	mImageFlags = 0;
	mAlphaRowBytes = 0;
	mNaturalWidth = 0;
	mNaturalHeight = 0;
	mIsOptimized = PR_FALSE;
	memset(&mPhImage, 0, sizeof(PhImage_t));
	mPhImageCache=NULL;
	mPhImageZoom = NULL;
}

// ----------------------------------------------------------------
nsImagePh :: ~nsImagePh()
{
  if (mImageBits != nsnull)
  {
  	if (mImageFlags & IMAGE_SHMEM)
  		DestroySRamImage(mImageBits);
  	else 
		delete [] mImageBits;
    mImageBits = nsnull;
  }

  if (mConvertedBits != nsnull)
	DestroySRamImage(mConvertedBits);

  if (mPhImageCache)
  {
	PhDCRelease(mPhImageCache);
	mPhImageCache=NULL;
  }

  if (mAlphaBits != nsnull)
  {
    delete [] mAlphaBits;
    mAlphaBits = nsnull;
  }

	if( mPhImageZoom ) {
		PgShmemDestroy( mPhImageZoom->image );
		free( mPhImageZoom );
		mPhImageZoom = NULL;
		}

  memset(&mPhImage, 0, sizeof(PhImage_t));
}

/** ----------------------------------------------------------------
 * Initialize the nsImagePh object
 * @param aWidth - Width of the image
 * @param aHeight - Height of the image
 * @param aDepth - Depth of the image
 * @param aMaskRequirements - A mask used to specify if alpha is needed.
 * @result NS_OK if the image was initied ok
 */
nsresult nsImagePh :: Init(PRInt32 aWidth, PRInt32 aHeight, PRInt32 aDepth,nsMaskRequirements aMaskRequirements)
{
	int type = -1;

	if (mImageBits != nsnull)
	{
		if (mImageFlags & IMAGE_SHMEM)
			DestroySRamImage(mImageBits);
		else
			delete [] mImageBits;
		mImageBits = nsnull;
	}

	if (mAlphaBits != nsnull)
	{
		delete [] mAlphaBits;
		mAlphaBits = nsnull;
	}
  
  	SetDecodedRect(0,0,0,0);  //init
 
    switch (aDepth)
    {
        case 24:
            type = Pg_IMAGE_DIRECT_888;
            mNumBytesPixel = 3;
            break;
//      case 16:
//          type = Pg_IMAGE_DIRECT_555;
//          mNumBytesPixel = 2;
//          break;
      case 8:
//          type = Pg_IMAGE_PALETTE_BYTE;
//          mNumBytesPixel = 1;
//          break;
        default:
            NS_ASSERTION(PR_FALSE, "unexpected image depth");
            return NS_ERROR_UNEXPECTED;
            break;
    }
 
	mWidth = aWidth;
	mHeight = aHeight;
	mDepth = aDepth;
	mIsTopToBottom = PR_TRUE;

  	/* Allocate the Image Data */
	mSizeImage = mNumBytesPixel * mWidth * mHeight;

	/* TODO: don't allow shared memory contexts if the graphics driver isn't a local device */

  	if (mSizeImage >= IMAGE_SHMEM_THRESHOLD)
  	{
		mImageBits = CreateSRamImage(mSizeImage);
		mImageFlags |= IMAGE_SHMEM;
//	  	mImageBits = (PRUint8*) PgShmemCreate(mSizeImage,0);
  	}
  	else
  	{
	  	//mImageBits = (PRUint8*) new PRUint8[mSizeImage];
	  	mImageBits = new PRUint8[mSizeImage];
	  	memset(mImageBits, 0, mSizeImage);
	  	mImageFlags &= ~IMAGE_SHMEM;
	}

//	mImageCache = PdCreateOffscreenContext(0, aWidth, aHeight, 0);

	switch(aMaskRequirements)
	{
		default:
		case nsMaskRequirements_kNoMask:
			mAlphaBits = nsnull;
			mAlphaWidth = 0;
			mAlphaHeight = 0;
			mAlphaRowBytes = 0;
			break;

		case nsMaskRequirements_kNeeds1Bit:
			mAlphaRowBytes = (aWidth + 7) / 8;
			mAlphaDepth = 1;

			// 32-bit align each row
			mAlphaRowBytes = (mAlphaRowBytes + 3) & ~0x3;

			mAlphaBits = new PRUint8[mAlphaRowBytes * aHeight];
			mAlphaWidth = aWidth;
			mAlphaHeight = aHeight;
			break;

		case nsMaskRequirements_kNeeds8Bit:
			mAlphaRowBytes = aWidth;
			mAlphaDepth = 8;

			// 32-bit align each row
			mAlphaRowBytes = (mAlphaRowBytes + 3) & ~0x3;
			mAlphaBits = new PRUint8[mAlphaRowBytes * aHeight];
			mAlphaWidth = aWidth;
			mAlphaHeight = aHeight;
			break;
	}

	mPhImage.image_tag = PtCRC((char *)mImageBits, mSizeImage);
	mPhImage.image = (char *)mImageBits;
	mPhImage.size.w = mWidth;
	mPhImage.size.h = mHeight;
	mRowBytes = mPhImage.bpl = mNumBytesPixel * mWidth;
	mPhImage.type = type;
	if (aMaskRequirements == nsMaskRequirements_kNeeds1Bit)
	{
		mPhImage.mask_bm = (char *)mAlphaBits;
		mPhImage.mask_bpl = mAlphaRowBytes;
	}

  	return NS_OK;
}

//------------------------------------------------------------

/* Creates a shared memory image if allowed...otherwise mallocs it... */
PRUint8 * nsImagePh::CreateSRamImage (PRUint32 size)
{
	/* TODO: add code to check for remote drivers (no shmem then) */
	return (PRUint8 *) PgShmemCreate(size,NULL);
}

PRBool nsImagePh::DestroySRamImage (PRUint8 * ptr)
{
	PgShmemDestroy(ptr);
	return PR_TRUE;
}

PRInt32 nsImagePh::GetHeight()
{
  	return mHeight;
}

PRInt32 nsImagePh::GetWidth()
{
  	return mWidth;
}

PRUint8* nsImagePh::GetBits()
{
  	return mImageBits;
}

void* nsImagePh::GetBitInfo()
{
  	return nsnull;
}

PRInt32 nsImagePh::GetLineStride()
{
  	return mRowBytes;
}

nsColorMap* nsImagePh::GetColorMap()
{
  	return nsnull;
}

PRBool nsImagePh::IsOptimized()
{
  	return mIsOptimized;
}

PRUint8* nsImagePh::GetAlphaBits()
{
  	return mAlphaBits;
}

PRInt32 nsImagePh::GetAlphaWidth()
{
  	return mAlphaWidth;
}

PRInt32 nsImagePh::GetAlphaHeight()
{
  	return mAlphaHeight;
}

PRInt32 nsImagePh::GetAlphaLineStride()
{
  	return mAlphaRowBytes;
}

nsIImage *nsImagePh::DuplicateImage()
{
  	return nsnull;
}

void nsImagePh::SetAlphaLevel(PRInt32 aAlphaLevel)
{
  	mAlphaLevel=aAlphaLevel;
}

PRInt32 nsImagePh::GetAlphaLevel()
{
  	return(mAlphaLevel);
}

void nsImagePh::MoveAlphaMask(PRInt32 aX, PRInt32 aY)
{
}

void nsImagePh :: ImageUpdated(nsIDeviceContext *aContext, PRUint8 aFlags, nsRect *aUpdateRect)
{
	/* does this mean it's dirty? */
  	mFlags = aFlags; // this should be 0'd out by Draw()
}




/* set the mask_bm and mask_bpl to 0, replacing the transparency mask_bm based with the Ph_USE_TRANSPARENCY mechanism */
static void convert_mask_bm_to_transparency( PhImage_t *image ) {
	if( !image->mask_bpl || !image->mask_bm ) return;

	int one = 0, x, y, b;
	uchar_t bit;
	char *ptr;
	PgColor_t transparent = PgRGB( 255, 255, 0 ); /* Shawn's idea */

  for( ptr = image->mask_bm,y = 0; y<image->size.h; y++ ) {
    for( b=0; b<image->mask_bpl; b++ ) {

			x = b * 8;
			for( bit=0x1; bit; bit<<=1 ) {
				if( !( (*ptr) & bit ) ) {
					if( x<image->size.w ) PiSetPixel( image, x, y, transparent );
					one++;
					}
				x++;
				}

      ptr++;
    	}
  	}

	if( one ) {
		image->flags |= Ph_USE_TRANSPARENCY;
		image->transparent = transparent;
//		free( image->mask_bm );
		image->mask_bm = NULL;
		image->mask_bpl = 0;
		}
	}

/** ----------------------------------------------------------------
  * Draw the bitmap, this method has a source and destination coordinates
  * @update dc - 11/20/98
  * @param aContext - the rendering context to draw with
  * @param aSurface - The HDC in a nsDrawingsurfacePh to copy the bits to.
  * @param aSX - source horizontal location
  * @param aSY - source vertical location
  * @param aSWidth - source width
  * @param aSHeight - source height
  * @param aDX - destination location
  * @param aDY - destination location
  * @param aDWidth - destination width
  * @param aDHeight - destination height
  * @result NS_OK if the draw worked
  */
NS_IMETHODIMP nsImagePh :: Draw(nsIRenderingContext &aContext, nsDrawingSurface aSurface,
				 PRInt32 aSX, PRInt32 aSY, PRInt32 aSWidth, PRInt32 aSHeight,
				 PRInt32 aDX, PRInt32 aDY, PRInt32 aDWidth, PRInt32 aDHeight)
{
	PhRect_t clip = { {aDX, aDY}, {aDX + aDWidth, aDY + aDHeight} };
	PhPoint_t pos = { aDX - aSX, aDY - aSY};
	PRBool 	aOffScreen;

	nsDrawingSurfacePh* drawing = (nsDrawingSurfacePh*) aSurface;

	if( !aSWidth || !aSHeight || !aDWidth || !aDHeight ) return NS_OK;


#ifdef ALLOW_PHIMAGE_CACHEING
	drawing->IsOffscreen(&aOffScreen);
	if (!mPhImage.mask_bm && mIsOptimized && mPhImageCache && aOffScreen)
	{
		PhArea_t sarea, darea;

		sarea.pos.x = aSX;
		sarea.pos.y = aSY;
		darea.pos.x = aDX;
		darea.pos.y = aDY;
		darea.size.w=sarea.size.w=aDWidth;
		darea.size.h=sarea.size.h=aDHeight;
		
		PgContextBlitArea(mPhImageCache,&sarea,(PdOffscreenContext_t *)drawing->GetDC(),&darea);
		PgFlush();	
	}
	else
#endif
	{
		PhImage_t *pimage = &mPhImage;

		if( aSWidth != aDWidth || aSHeight != aDHeight ) {
			if( !mPhImageZoom )  {
				convert_mask_bm_to_transparency( &mPhImage );
				mPhImageZoom = PiResizeImage( &mPhImage, NULL, aDWidth, aDHeight, Pi_USE_COLORS|Pi_SHMEM );
				}
			pimage = mPhImageZoom;
			}

		PgSetMultiClip( 1, &clip );
		if ((mAlphaDepth == 1) || (mAlphaDepth == 0))
			PgDrawPhImagemx( &pos, pimage, 0 );
		else
			printf("DRAW IMAGE: with 8 bit alpha!!\n");
		PgSetMultiClip( 0, NULL );
	}


  	return NS_OK;
}

NS_IMETHODIMP nsImagePh :: Draw(nsIRenderingContext &aContext, nsDrawingSurface aSurface,
				 PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight)
{
  	PhPoint_t pos = { aX, aY };
  	int       err;
  	PRBool 	aOffScreen;

	if (!aSurface || !mImageBits)
		return (NS_ERROR_FAILURE);
  
  	nsDrawingSurfacePh* drawing = (nsDrawingSurfacePh*) aSurface;

#ifdef ALLOW_PHIMAGE_CACHEING
	drawing->IsOffscreen(&aOffScreen);
	if (!mPhImage.mask_bm && mIsOptimized && mPhImageCache && aOffScreen)
	{
		PhArea_t sarea, darea;

		sarea.pos.x=sarea.pos.y=0;
		darea.pos=pos;
		darea.size.w=sarea.size.w=aWidth;
		darea.size.h=sarea.size.h=aHeight;
		
		PgContextBlitArea(mPhImageCache,&sarea,(PdOffscreenContext_t *)drawing->GetDC(),&darea);
		PgFlush();	
	}
	else
#endif
	{
		if ((mAlphaDepth == 1) || (mAlphaDepth == 0))
		{
			PgDrawPhImagemx(&pos, &mPhImage, 0);
		}
		else if (mAlphaDepth == 8)
		{
		//	memset(&pg_alpha, 0, sizeof(PgAlpha_t));
		//	pg_alpha.dest_alpha_map = mAlphaBits;
		//	mPhImage.alpha = &pg_alpha;
		//	PgDrawPhImagemx(&pos, &mPhImage, 0);
		//	mPhImage.alpha = nsnull;
			printf("DRAW IMAGE: with 8 bit alpha!!\n");
		}
	}

  	return NS_OK;
}

/* New Tile code *********************************************************************/
NS_IMETHODIMP nsImagePh::DrawTile(nsIRenderingContext &aContext, nsDrawingSurface aSurface, nsRect &aSrcRect, nsRect &aTileRect ) {
	PhPoint_t pos, space, rep;
  	PRBool 	aOffScreen;

	pos.x = aTileRect.x;
	pos.y = aTileRect.y;

	space.x = aSrcRect.width;
	space.y = aSrcRect.height;
	rep.x = ( aTileRect.width + space.x - 1 ) / space.x;
	rep.y = ( aTileRect.height + space.y - 1 ) / space.y;
	PhRect_t clip = { aTileRect.x, aTileRect.y, aTileRect.x + aTileRect.width, aTileRect.y + aTileRect.height };
	PgSetMultiClip( 1, &clip );

 	nsDrawingSurfacePh* drawing = (nsDrawingSurfacePh*) aSurface;

#ifdef ALLOW_PHIMAGE_CACHEING
	drawing->IsOffscreen(&aOffScreen);
	if (!mPhImage.mask_bm && mIsOptimized && mPhImageCache && aOffScreen)
	{
		PhArea_t sarea, darea;
		int x,y;
		PdOffscreenContext_t *dc = (PdOffscreenContext_t *) drawing->GetDC(),*odc= (PdOffscreenContext_t *) PhDCSetCurrent(NULL);

		sarea.pos.x=sarea.pos.y=0;
		darea.pos=pos;
		darea.size.w=sarea.size.w=mPhImage.size.w;
		darea.size.h=sarea.size.h=mPhImage.size.h;

		for (y=0; y<rep.y; y++)
		{
			for (x=0; x<rep.x; x++)
			{
				PgContextBlitArea(mPhImageCache,&sarea,dc,&darea);
				darea.pos.x+=darea.size.w;
			}
			darea.pos.x=pos.x;
			darea.pos.y+=darea.size.h;
		}

		PgFlush();	
		PhDCSetCurrent(odc);
	}
	else
#endif
	{
		PgDrawRepPhImagemx( &mPhImage, 0, &pos, &rep, &space );
	}

	PgSetMultiClip( 0, NULL );
}

NS_IMETHODIMP nsImagePh::DrawTile( nsIRenderingContext &aContext, nsDrawingSurface aSurface, PRInt32 aSXOffset, PRInt32 aSYOffset, const nsRect &aTileRect ) 
{
	PhPoint_t pos, space, rep;
  	PRBool 	aOffScreen;

	// since there is an offset into the image and I only want to draw full
	// images, shift the position back and set clipping so that it looks right
	pos.x = aTileRect.x - aSXOffset;
	pos.y = aTileRect.y - aSYOffset;

	space.x = mPhImage.size.w;
	space.y = mPhImage.size.h;
	rep.x = ( aTileRect.width + aSXOffset + space.x - 1 ) / space.x;
	rep.y = ( aTileRect.height + aSYOffset + space.y - 1 ) / space.y;

 	nsDrawingSurfacePh* drawing = (nsDrawingSurfacePh*) aSurface;

	/* not sure if cliping is necessary */
	PhRect_t clip = { aTileRect.x, aTileRect.y, aTileRect.x + aTileRect.width, aTileRect.y + aTileRect.height };
	PgSetMultiClip( 1, &clip );

#ifdef ALLOW_PHIMAGE_CACHEING
	drawing->IsOffscreen(&aOffScreen);
	if (!mPhImage.mask_bm && mIsOptimized && mPhImageCache && aOffScreen)
	{
		PhArea_t sarea, darea;
		int x,y;
		PdOffscreenContext_t *dc = (PdOffscreenContext_t *) drawing->GetDC(),*odc= (PdOffscreenContext_t *) PhDCSetCurrent(NULL);

		sarea.pos.x=sarea.pos.y=0;
		darea.pos=pos;
		darea.size.w=sarea.size.w=mPhImage.size.w;
		darea.size.h=sarea.size.h=mPhImage.size.h;

		for (y=0; y<rep.y; y++)
		{
			for (x=0; x<rep.x; x++)
			{
				PgContextBlitArea(mPhImageCache,&sarea,dc,&darea);
				darea.pos.x+=darea.size.w;
			}
			darea.pos.x=pos.x;
			darea.pos.y+=darea.size.h;
		}

		PgFlush();	
		PhDCSetCurrent(odc);
	}
	else
#endif
	{
		PgDrawRepPhImagemx( &mPhImage, 0, &pos, &rep, &space );
	}

	PgSetMultiClip( 0, NULL );

	return NS_OK;
}
/* End New Tile code *****************************************************************/


//------------------------------------------------------------
nsresult nsImagePh :: Optimize(nsIDeviceContext* aContext)
{
	PhPoint_t pos={0,0};
	PhDrawContext_t *odc;

	/* TODO: need to invalidate the caches if the graphics system has changed somehow (ie: mode switch, dragged to a remote
			display, etc... */

	/* TODO: remote graphics drivers should always cache the images offscreen regardless of wether there is a mask or not,
			the Draw code will need to be updated for this */

#ifdef ALLOW_PHIMAGE_CACHEING
	if ((mSizeImage > IMAGE_SHMEM_THRESHOLD) && (mPhImage.mask_bm == NULL))
	{
		if (mPhImageCache == NULL)
		{
			/* good candidate for an offscreen cached image */
			if((mPhImageCache = PdCreateOffscreenContext(0,mPhImage.size.w,mPhImage.size.h,0)) != NULL)
			{	
				odc = PhDCSetCurrent (mPhImageCache);
				PgDrawPhImagemx (&pos, &mPhImage, 0);
				PgFlush();
				PhDCSetCurrent(odc);
				mIsOptimized = PR_TRUE;
				return NS_OK;
			}
		}
	}
	else
	{
/* it's late and this should be working but it doesn't so...I'm obviously not thinking straight...going home... */
#if 0	/* might want to change this to a real define to give the option of removing the dependancy on the render lib */

		/* Creates a new shared memory object and uses a memory context to convert the image down
			to the native screen format */

		nsDeviceContextPh * dev = (nsDeviceContextPh *) aContext;
		PRUint32 scr_depth;
		PRUint32 scr_type;
		PRUint32 cimgsize;

		dev->GetDepth(scr_depth);

		switch (scr_depth)
		{
			case 8 : scr_type = Pg_IMAGE_PALETTE_BYTE; break;
			case 15 : scr_type = Pg_IMAGE_DIRECT_555; break;
			case 16 : scr_type = Pg_IMAGE_DIRECT_565; break;
			case 24 : scr_type = Pg_IMAGE_DIRECT_888; break;
			case 32 : scr_type = Pg_IMAGE_DIRECT_8888; break;
			default :
				return NS_OK; /* some wierd pixel depth...not optimizing it... */
		}
		cimgsize = (((scr_depth + 1) & 0xFC) >> 3) * mPhImage.size.w * mPhImage.size.h;
		if (cimgsize > IMAGE_SHMEM_THRESHOLD)
		{
			PmMemoryContext_t *memctx;
			PhImage_t timage;
			PhPoint_t pos={0,0};
			
			if (mConvertedBits == NULL)
			{
				if ((mConvertedBits = CreateSRamImage (cimgsize)) == NULL)
				{
					fprintf(stderr,"memc failed\n");
					mIsOptimized = PR_FALSE;
					return NS_OK;
				}
			}
			memset (&timage, 0 , sizeof (PhImage_t));

			timage.size = mPhImage.size;
			timage.image = mConvertedBits;
			timage.bpl = cimgsize / timage.size.h;
			timage.type = scr_type;

			fprintf (stderr,"Creating memctx cache\n");
			if (memctx = PmMemCreateMC (&timage, &timage.size, &pos) == NULL)
			{
				DestroySRamImage (mConvertedBits);
				mIsOptimized = PR_FALSE;
				fprintf(stderr,"Error Creating Memctx\n");
				return NS_OK;
			}
			
			fprintf(stderr,"mPhImage.size = (%d,%d) timage.size = (%d,%d)\n",mPhImage.size.w, mPhImage.size.h, timage.size.w, timage.size.h);
			fprintf(stderr,"mPhImage.bpl = %d timage.bpl = %d\n",mPhImage.bpl, timage.bpl);
			fprintf(stderr,"mPhImage.type = %d timage.type = %d\n",mPhImage.type, timage.type);
			fprintf(stderr,"mPhImage.image = 0x%08X timage.image = 0x%08X\n", mPhImage.image, timage.image);


			fprintf(stderr,"staring to draw\n");

			PmMemStart(memctx);
			
			fprintf(stderr,"drawing\n");
			if (mPhImage.colors)
			{
				fprintf(stderr,"1\n");
				PgSetPalette (mPhImage.palette, 0, 0, mPhImage.colors, Pg_PALSET_SOFT, 0);
			}

			fprintf(stderr,"2\n");
			PgDrawImage(mPhImage.image, mPhImage.type, &pos, &mPhImage.size, mPhImage.bpl, 0);
			fprintf(stderr,"3\n");
			PmMemFlush(memctx, &timage);
			fprintf(stderr,"4\n");
			PmMemStop(memctx);
			fprintf(stderr,"we got our image drawn\n");
			mPhImage = timage;
			mPhImage.image_tag = PtCRC((char *)mPhImage.image, cimgsize);
			mPhImage.type = 0; /* now in native display format */
			mIsOptimized = PR_TRUE;
			fprintf(stderr,"done!\n");
		}
#endif
	}
#endif // ALLOW_PHIMAGE_CACHEING

	return NS_OK;
}


//------------------------------------------------------------
// lock the image pixels. Implement this if you need it.
NS_IMETHODIMP
nsImagePh::LockImagePixels(PRBool aMaskPixels)
{
  	return NS_OK;
}

//------------------------------------------------------------
// unlock the image pixels.  Implement this if you need it.
NS_IMETHODIMP
nsImagePh::UnlockImagePixels(PRBool aMaskPixels)
{
  	return NS_OK;
}

/** ---------------------------------------------------
 *	Set the decoded dimens of the image
 */
NS_IMETHODIMP
nsImagePh::SetDecodedRect(PRInt32 x1, PRInt32 y1, PRInt32 x2, PRInt32 y2 )
{
  	mDecodedX1 = x1; 
  	mDecodedY1 = y1; 
  	mDecodedX2 = x2; 
  	mDecodedY2 = y2; 
  	return NS_OK;
}

#ifdef USE_IMG2
 /**
  * BitBlit the entire (no cropping) nsIImage to another nsImage, the source and dest can be scaled
  * @update - saari 03/08/01
  * @param aDstImage  the nsImage to blit to
  * @param aDX The destination horizontal location
  * @param aDY The destination vertical location
  * @param aDWidth The destination width of the pixelmap
  * @param aDHeight The destination height of the pixelmap
  * @return if TRUE, no errors
  */
NS_IMETHODIMP nsImagePh::DrawToImage(nsIImage* aDstImage,
                                      nscoord aDX, nscoord aDY,
                                      nscoord aDWidth, nscoord aDHeight)
{
  nsImagePh *dest = NS_STATIC_CAST(nsImagePh *, aDstImage);

  if (!dest)
    return NS_ERROR_FAILURE;

  if (!dest->mPhImage.image)
    return NS_ERROR_FAILURE;

#ifdef ALLOW_PHIMAGE_CACHEING
    if (!mPhImage.mask_bm && mIsOptimized && mPhImageCache && dest->mPhImageCache)
    {
        PhArea_t sarea, darea;

        sarea.pos.x = sarea.pos.y = 0;
        darea.pos.x = aDX;
        darea.pos.y = aDY;
        darea.size.w = sarea.size.w = aDWidth;
        darea.size.h = sarea.size.h = aDHeight;

        PgContextBlitArea(mPhImageCache,&sarea,dest->mPhImageCache,&darea);
        PgFlush();
    }
	else
#endif
	{
		PhArea_t sarea, darea;
		int start, x, y;
		char mbit, mbyte;

		sarea.pos.x = sarea.pos.y = 0;
		darea.pos.x = aDX;
		darea.pos.y = aDY;
		darea.size.w = sarea.size.w = aDWidth;
		darea.size.h = sarea.size.h = aDHeight;
		start = (aDY * dest->mPhImage.bpl) + (aDX * mNumBytesPixel);
		for (y = 0; y < mPhImage.size.h; y++)
		{
			for (x = 0; x < mPhImage.size.w; x++)
			{
				if (mPhImage.mask_bm)
				{
					mbyte = *(mPhImage.mask_bm + (mPhImage.mask_bpl * y) + (x >> 3));
					mbit = x & 7;
					if (!(mbyte & (0x80 >> mbit)))
						continue;
				}
				memcpy(dest->mPhImage.image + start + (dest->mPhImage.bpl * y) + (x*mNumBytesPixel), \
					mPhImage.image + (mPhImage.bpl * y) + (x*mNumBytesPixel), mNumBytesPixel);
			}
		}
	}

  return NS_OK;
}
#endif
