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

#ifndef nsBlenderWin_h___
#define nsBlenderWin_h___

#include "nsIBlender.h"
#include "nsPoint.h"
#include "nsRect.h"
#include "nsIImage.h"
#include "nsRenderingContextWin.h"

#ifdef NGLAYOUT_DDRAW
#include "ddraw.h"
#endif

//----------------------------------------------------------------------

// Blender interface
class nsBlenderWin : public nsIBlender
{
public:

  NS_DECL_ISUPPORTS
  
  nsBlenderWin();
  ~nsBlenderWin();

  virtual nsresult Init(nsDrawingSurface aSrc,nsDrawingSurface aDst);
  virtual nsresult Blend(PRInt32 aSX, PRInt32 aSY, PRInt32 aWidth, PRInt32 aHeight,
                          nsDrawingSurface aDest, PRInt32 aDX, PRInt32 aDY, float aSrcOpacity,PRBool aSaveBlendArea);

  nsDrawingSurface GetSrcDS() {return(mSrcDS);}
  nsDrawingSurface GetDstDS() {return(mDstDS);}

  PRBool  RestoreImage(nsDrawingSurface aDst);

 private:
  PRBool CalcAlphaMetrics(BITMAP *aSrcInfo,BITMAP *aDestInfo,nsPoint *ASrcUL,
                              BITMAP  *aMapInfo,nsPoint *aMaskUL,
                              PRInt32 aWidth,PRInt32 aHeight,
                              PRInt32 *aNumlines,
                              PRInt32 *aNumbytes,PRUint8 **aSImage,PRUint8 **aDImage,
                              PRUint8 **aMImage,PRInt32 *aSLSpan,PRInt32 *aDLSpan,PRInt32 *aMLSpan);

  PRInt32  CalcBytesSpan(PRUint32  aWidth,PRUint32  aBitsPixel);

  /**
   * Create a DIB header and bits for a bitmap
   * @param aBHead  information header for the DIB
   * @param aBits   a handle to the 8 bit pointer for the data bits
   * @param aWidth  width of the bitmap to create
   * @param aHeight Height of the bitmap to create
   * @param aDepth  Bits per pixel of the bitmap to create
   */
  nsresult BuildDIB(LPBITMAPINFOHEADER  *aBHead,unsigned char **aBits,PRInt32 aWidth, PRInt32 aHeight, PRInt32 aDepth);

  /**
   * Delete the DIB header and bits created from BuildDIB
   * @param aBHead  information header for the DIB
   * @param aBits   a handle to the 8 bit pointer for the data bits
   */
  void DeleteDIB(LPBITMAPINFOHEADER  *aBHead,unsigned char **aBits);

  void Do32Blend(PRUint8 aBlendVal,PRInt32 aNumlines,PRInt32 aNumbytes,PRUint8 *aSImage,PRUint8 *aDImage,
                PRInt32 aSLSpan,PRInt32 aDLSpan,nsBlendQuality aBlendQuality,PRBool aSaveBlendArea);

  /** 
   * Blend two 24 bit image arrays using an 8 bit alpha mask
   * @param aNumlines  Number of lines to blend
   * @param aNumberBytes Number of bytes per line to blend
   * @param aSImage Pointer to beginning of the source bytes
   * @param aDImage Pointer to beginning of the destination bytes
   * @param aMImage Pointer to beginning of the mask bytes
   * @param aSLSpan number of bytes per line for the source bytes
   * @param aDLSpan number of bytes per line for the destination bytes
   * @param aMLSpan number of bytes per line for the Mask bytes
   * @param aBlendQuality The quality of this blend, this is for tweening if neccesary
   * @param aSaveBlendArea informs routine if the area affected area will be save first
   */
  void Do24BlendWithMask(PRInt32 aNumlines,PRInt32 aNumbytes,PRUint8 *aSImage,PRUint8 *aDImage,
                PRUint8 *aMImage,PRInt32 aSLSpan,PRInt32 aDLSpan,PRInt32 aMLSpan,nsBlendQuality aBlendQuality,PRBool aSaveBlendArea);

  /** 
   * Blend two 24 bit image arrays using a passed in blend value
   * @param aNumlines  Number of lines to blend
   * @param aNumberBytes Number of bytes per line to blend
   * @param aSImage Pointer to beginning of the source bytes
   * @param aDImage Pointer to beginning of the destination bytes
   * @param aMImage Pointer to beginning of the mask bytes
   * @param aSLSpan number of bytes per line for the source bytes
   * @param aDLSpan number of bytes per line for the destination bytes
   * @param aMLSpan number of bytes per line for the Mask bytes
   * @param aBlendQuality The quality of this blend, this is for tweening if neccesary
   * @param aSaveBlendArea informs routine if the area affected area will be save first
   */

  void Do24Blend(PRUint8 aBlendVal,PRInt32 aNumlines,PRInt32 aNumbytes,PRUint8 *aSImage,PRUint8 *aDImage,
                PRInt32 aSLSpan,PRInt32 aDLSpan,nsBlendQuality aBlendQuality,PRBool aSaveBlendArea);


  /** 
   * Blend two 16 bit image arrays using a passed in blend value
   * @param aNumlines  Number of lines to blend
   * @param aNumberBytes Number of bytes per line to blend
   * @param aSImage Pointer to beginning of the source bytes
   * @param aDImage Pointer to beginning of the destination bytes
   * @param aMImage Pointer to beginning of the mask bytes
   * @param aSLSpan number of bytes per line for the source bytes
   * @param aDLSpan number of bytes per line for the destination bytes
   * @param aMLSpan number of bytes per line for the Mask bytes
   * @param aBlendQuality The quality of this blend, this is for tweening if neccesary
   * @param aSaveBlendArea informs routine if the area affected area will be save first
   */
  void Do16Blend(PRUint8 aBlendVal,PRInt32 aNumlines,PRInt32 aNumbytes,PRUint8 *aSImage,PRUint8 *aDImage,
                PRInt32 aSLSpan,PRInt32 aDLSpan,nsBlendQuality aBlendQuality,PRBool aSaveBlendArea);

    /** 
   * Blend two 8 bit image arrays using an 8 bit alpha mask
   * @param aNumlines  Number of lines to blend
   * @param aNumberBytes Number of bytes per line to blend
   * @param aSImage Pointer to beginning of the source bytes
   * @param aDImage Pointer to beginning of the destination bytes
   * @param aMImage Pointer to beginning of the mask bytes
   * @param aSLSpan number of bytes per line for the source bytes
   * @param aDLSpan number of bytes per line for the destination bytes
   * @param aMLSpan number of bytes per line for the Mask bytes
   * @param aBlendQuality The quality of this blend, this is for tweening if neccesary
   * @param aSaveBlendArea informs routine if the area affected area will be save first
   */
  void Do8BlendWithMask(PRInt32 aNumlines,PRInt32 aNumbytes,PRUint8 *aSImage,PRUint8 *aDImage,
                PRUint8 *aMImage,PRInt32 aSLSpan,PRInt32 aDLSpan,PRInt32 aMLSpan,nsBlendQuality aBlendQuality,PRBool aSaveBlendArea);

  /** 
   * Blend two 8 bit image arrays using a passed in blend value
   * @param aNumlines  Number of lines to blend
   * @param aNumberBytes Number of bytes per line to blend
   * @param aSImage Pointer to beginning of the source bytes
   * @param aDImage Pointer to beginning of the destination bytes
   * @param aMImage Pointer to beginning of the mask bytes
   * @param aSLSpan number of bytes per line for the source bytes
   * @param aDLSpan number of bytes per line for the destination bytes
   * @param aMLSpan number of bytes per line for the Mask bytes
   * @param aBlendQuality The quality of this blend, this is for tweening if neccesary
   * @param aSaveBlendArea informs routine if the area affected area will be save first
   */
  void Do8Blend(PRUint8 aBlendVal,PRInt32 aNumlines,PRInt32 aNumbytes,PRUint8 *aSImage,PRUint8 *aDImage,
                PRInt32 aSLSpan,PRInt32 aDLSpan,nsColorMap *aColorMap,nsBlendQuality aBlendQuality,PRBool aSaveBlendArea);

#ifdef NGLAYOUT_DDRAW
  PRBool LockSurface(IDirectDrawSurface *aSurface, DDSURFACEDESC *aDesc, BITMAP *aBitmap, RECT *aRect, DWORD aLockFlags);
#endif

  private:
  BITMAPINFOHEADER    *mDstbinfo, *mSrcbinfo;
  PRUint8             *mSrcBytes;
  PRUint8             *mDstBytes;
  BITMAP              mSrcInfo, mDstInfo;
  nsDrawingSurfaceWin *mSrcDS, *mDstDS;

  PRInt32             mSRowBytes;
  PRInt32             mDRowBytes;

  PRInt32             mSaveLS;
  PRInt32             mSaveNumLines;
  PRInt32             mSaveNumBytes;
  PRUint8             *mSaveBytes;    // place to save bits
  PRUint8             *mRestorePtr;   // starting area of save dst
  PRUint32            mResLS;         // line span for restore area

#ifdef NGLAYOUT_DDRAW
  DDSURFACEDESC       mSrcSurf;
  DDSURFACEDESC       mDstSurf;
#endif
};

#endif