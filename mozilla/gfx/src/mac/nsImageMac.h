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
 * Communications Corporation.	Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

#ifndef nsImageMac_h___
#define nsImageMac_h___

#include "nsIImage.h"
#include <QDOffscreen.h>

class nsImageMac : public nsIImage
{
public:
											nsImageMac();
	virtual							~nsImageMac();

	NS_DECL_ISUPPORTS

	/**
	@see nsIImage.h
	*/
	virtual nsresult		Init(PRInt32 aWidth, PRInt32 aHeight, PRInt32 aDepth, nsMaskRequirements aMaskRequirements);
	virtual PRInt32			GetBytesPix()					{ return mBytesPerPixel; }
	virtual PRBool			GetIsRowOrderTopToBottom() { return mIsTopToBottom; }

	virtual PRInt32			GetWidth()						{ return mWidth; }
	virtual PRInt32			GetHeight()						{ return mHeight;}

	virtual PRUint8*		GetBits();
	virtual PRInt32			GetLineStride()				{ return mRowBytes; }
	virtual PRBool			GetHasAlphaMask()     { return mAlphaGWorld != nsnull; }

    NS_IMETHOD     SetNaturalWidth(PRInt32 naturalwidth) { mNaturalWidth= naturalwidth; return NS_OK;}
    NS_IMETHOD     SetNaturalHeight(PRInt32 naturalheight) { mNaturalHeight= naturalheight; return NS_OK;}
    virtual PRInt32     GetNaturalWidth() {return mNaturalWidth; }
    virtual PRInt32     GetNaturalHeight() {return mNaturalHeight; }

    NS_IMETHOD          SetDecodedRect(PRInt32 x1, PRInt32 y1, PRInt32 x2, PRInt32 y2);        
    virtual PRInt32     GetDecodedX1() { return mDecodedX1;}
    virtual PRInt32     GetDecodedY1() { return mDecodedY1;}
    virtual PRInt32     GetDecodedX2() { return mDecodedX2;}
    virtual PRInt32     GetDecodedY2() { return mDecodedY2;}


	virtual PRUint8*		GetAlphaBits();
	virtual PRInt32			GetAlphaWidth() 			{ return mAlphaWidth; }
	virtual PRInt32			GetAlphaHeight()	 		{ return mAlphaHeight; }
	virtual PRInt32			GetAlphaLineStride()	{ return mARowBytes; }

	virtual void				ImageUpdated(nsIDeviceContext *aContext, PRUint8 aFlags, nsRect *aUpdateRect);
	virtual PRBool			IsOptimized()					{ return PR_FALSE; }
	virtual nsresult		Optimize(nsIDeviceContext* aContext);
	virtual nsColorMap*	GetColorMap() 				{ return nsnull;}

	NS_IMETHOD 					Draw(nsIRenderingContext &aContext, nsDrawingSurface aSurface,
															PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight);

	NS_IMETHOD 					Draw(nsIRenderingContext &aContext, nsDrawingSurface aSurface,
															PRInt32 aSX, PRInt32 aSY, PRInt32 aSWidth, PRInt32 aSHeight,
															PRInt32 aDX, PRInt32 aDY, PRInt32 aDWidth, PRInt32 aDHeight);

	virtual void				SetAlphaLevel(PRInt32 aAlphaLevel);
	virtual PRInt32			GetAlphaLevel();
	virtual void*				GetBitInfo()					{ return nsnull; }

	NS_IMETHOD					LockImagePixels(PRBool aMaskPixels);
	NS_IMETHOD					UnlockImagePixels(PRBool aMaskPixels);

protected:
	
	void							ClearGWorld(GWorldPtr);
	OSErr							MakeGrayscaleColorTable(PRInt16 numColors, CTabHandle *outColorTable);
	OSErr							AllocateGWorld(PRInt16 depth, CTabHandle colorTable, const Rect& bounds, GWorldPtr *outGWorld);
	
private:

	GWorldPtr				mImageGWorld;
	
	PRInt32					mWidth;
	PRInt32					mHeight;

	PRInt32					mRowBytes;
	PRInt32					mBytesPerPixel;
		
	// alpha layer members
	GWorldPtr				mAlphaGWorld;

	PRInt16					mAlphaDepth;		// alpha layer depth
	PRInt16					mAlphaWidth;		// alpha layer width
	PRInt16					mAlphaHeight;		// alpha layer height
	PRInt32					mARowBytes;			// alpha row bytes

    PRInt32	                mNaturalWidth;
    PRInt32		            mNaturalHeight;

    PRInt32             mDecodedX1;       //Keeps track of what part of image
    PRInt32             mDecodedY1;       // has been decoded.
    PRInt32             mDecodedX2; 
    PRInt32             mDecodedY2;    

    
	//nsPoint					mLocation;			// alpha mask location

	//PRInt8					mImageCache;		// place to save off the old image for fast animation
	//PRInt16					mAlphaLevel;		// an alpha level every pixel uses

	PRIntn					mPixelDataSize;
	PRBool					mIsTopToBottom;

};

#endif
