/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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
 * Contributor(s): Daniel Switkin and Mathias Agopian
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

#ifndef nsImageBeOS_h___
#define nsImageBeOS_h___

#include "nsIImage.h"
#include "nsPoint.h"
class BBitmap;
class BView;
class nsImageBeOS : public nsIImage
{
public:
	nsImageBeOS();
	virtual ~nsImageBeOS();
	
	NS_DECL_ISUPPORTS

	// Vital stats
	virtual PRInt32 GetBytesPix() { return mNumBytesPixel; } 
	virtual PRInt32 GetHeight() { return mHeight; }
	virtual PRInt32 GetWidth() { return mWidth; }
	virtual PRUint8 *GetBits() { return mImageBits; }
	virtual void *GetBitInfo() { return nsnull; }
	virtual PRBool GetIsRowOrderTopToBottom() { return PR_TRUE; }
	virtual PRInt32 GetLineStride() { return mRowBytes; }
	
	NS_IMETHOD SetNaturalWidth(PRInt32 naturalwidth);
	NS_IMETHOD SetNaturalHeight(PRInt32 naturalheight);
	virtual PRInt32 GetNaturalWidth() { return mNaturalWidth; } 
	virtual PRInt32 GetNaturalHeight() { return mNaturalHeight; } 
	
	NS_IMETHOD SetDecodedRect(PRInt32 x1, PRInt32 y1, PRInt32 x2, PRInt32 y2); 
	virtual PRInt32 GetDecodedX1() { return mDecodedX1; }
	virtual PRInt32 GetDecodedY1() { return mDecodedY1; }
	virtual PRInt32 GetDecodedX2() { return mDecodedX2; }
	virtual PRInt32 GetDecodedY2() { return mDecodedY2; }
	
	virtual nsColorMap *GetColorMap() { return nsnull; }

	// The heavy lifting
	NS_IMETHOD Draw(nsIRenderingContext &aContext, nsDrawingSurface aSurface,
		PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight);
	NS_IMETHOD Draw(nsIRenderingContext &aContext, nsDrawingSurface aSurface,
		PRInt32 aSX, PRInt32 aSY, PRInt32 aSWidth, PRInt32 aSHeight,
		PRInt32 aDX, PRInt32 aDY, PRInt32 aDWidth, PRInt32 aDHeight);

#ifdef USE_IMG2
	NS_IMETHOD DrawToImage(nsIImage *aDstImage, nscoord aDX, nscoord aDY,
		nscoord aDWidth, nscoord aDHeight);
#endif
 
	NS_IMETHOD DrawTile(nsIRenderingContext &aContext, nsDrawingSurface aSurface, 
		nsRect &aSrcRect, nsRect &aTileRect); 
	
	NS_IMETHOD DrawTile(nsIRenderingContext &aContext, nsDrawingSurface aSurface, 
		PRInt32 aSXOffset, PRInt32 aSYOffset, const nsRect &aTileRect); 
	
	virtual void ImageUpdated(nsIDeviceContext *aContext, PRUint8 aFlags,
		nsRect *aUpdateRect);
	virtual nsresult Init(PRInt32 aWidth, PRInt32 aHeight, PRInt32 aDepth,
		nsMaskRequirements aMaskRequirements);

	virtual PRBool IsOptimized() { return mOptimized; }
	virtual nsresult Optimize(nsIDeviceContext *aContext);
	
	virtual PRBool GetHasAlphaMask() { return mAlphaBits != nsnull; }
	virtual PRUint8 *GetAlphaBits() { return mAlphaBits; }
	virtual PRInt32 GetAlphaWidth() { return mAlphaWidth; }
	virtual PRInt32 GetAlphaHeight() { return mAlphaHeight; }
	virtual PRInt32 GetAlphaLineStride() { return mAlphaRowBytes; }
	virtual nsIImage *DuplicateImage();
	
	virtual void SetAlphaLevel(PRInt32 aAlphaLevel);
	virtual PRInt32 GetAlphaLevel() { return 0; }
	virtual PRInt8 GetAlphaDepth() { return mAlphaDepth; }
	virtual void MoveAlphaMask(PRInt32 aX, PRInt32 aY);
	
	NS_IMETHOD LockImagePixels(PRBool aMaskPixels);
	NS_IMETHOD UnlockImagePixels(PRBool aMaskPixels);

private:
	void ComputePaletteSize(PRIntn nBitCount);
	void DrawNoLock(BView *aView, PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight);
	
protected:
	void CreateImage(nsDrawingSurface aSurface);
	nsresult BuildImage(nsDrawingSurface aDrawingSurface);
	
private:
	BBitmap *mImage;
	PRUint8 *mImageBits;
	PRInt32 mWidth;
	PRInt32 mHeight;
	PRInt32 mDepth;
	PRInt32 mRowBytes;
	PRInt32 mSizeImage;
	
	PRInt32 mNaturalWidth;
	PRInt32 mNaturalHeight;
	
	// Keeps track of what part of image has been decoded
	PRInt32 mDecodedX1;
	PRInt32 mDecodedY1;
	PRInt32 mDecodedX2;
	PRInt32 mDecodedY2;
	
	// Alpha layer members
	PRUint8 *mAlphaBits;
	PRInt16 mAlphaWidth;
	PRInt16 mAlphaHeight;
	PRInt16 mAlphaRowBytes;
	PRInt8 mAlphaDepth;
	
	// Flags set by ImageUpdated
	PRUint8 mFlags;
	PRInt8 mNumBytesPixel;
	PRBool mImageCurrent;
	PRBool mOptimized;
};

#endif
