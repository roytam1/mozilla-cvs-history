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
 * Contributor(s):
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
#ifndef nsRenderingContextPh_h___
#define nsRenderingContextPh_h___

#include "nsRenderingContextImpl.h"
#include "nsUnitConversion.h"
#include "nsFont.h"
#include "nsIFontMetrics.h"
#include "nsPoint.h"
#include "nsString.h"
#include "nsCRT.h"
#include "nsTransform2D.h"
#include "nsIViewManager.h"
#include "nsIWidget.h"
#include "nsRect.h"
#include "nsImagePh.h"
#include "nsIDeviceContext.h"
#include "nsVoidArray.h"
#include "nsGfxCIID.h"

#include "nsDrawingSurfacePh.h"
#include "nsRegionPh.h"

class nsRenderingContextPh : public nsRenderingContextImpl
{
public:
   nsRenderingContextPh();
   virtual ~nsRenderingContextPh();
   
   NS_DECL_AND_IMPL_ZEROING_OPERATOR_NEW
	   
   NS_DECL_ISUPPORTS
	   
   NS_IMETHOD Init(nsIDeviceContext* aContext, nsIWidget *aWindow);
   NS_IMETHOD Init(nsIDeviceContext* aContext, nsDrawingSurface aSurface);
   
   NS_IMETHOD Reset(void);
   
   NS_IMETHOD GetDeviceContext(nsIDeviceContext *&aContext);
   
   NS_IMETHOD LockDrawingSurface(PRInt32 aX, PRInt32 aY, PRUint32 aWidth, PRUint32 aHeight,
								 void **aBits, PRInt32 *aStride, PRInt32 *aWidthBytes,
								 PRUint32 aFlags);
   NS_IMETHOD UnlockDrawingSurface(void);
   
   NS_IMETHOD SelectOffScreenDrawingSurface(nsDrawingSurface aSurface);
   NS_IMETHOD GetDrawingSurface(nsDrawingSurface *aSurface);
   NS_IMETHOD GetHints(PRUint32& aResult);
   
   NS_IMETHOD PushState(void);
   NS_IMETHOD PopState(PRBool &aClipState);
   
   NS_IMETHOD IsVisibleRect(const nsRect& aRect, PRBool &aClipState);
   
   NS_IMETHOD SetClipRect(const nsRect& aRect, nsClipCombine aCombine, PRBool &aCilpState);
   NS_IMETHOD GetClipRect(nsRect &aRect, PRBool &aClipState);
   NS_IMETHOD SetClipRegion(const nsIRegion& aRegion, nsClipCombine aCombine, PRBool &aClipState);
   NS_IMETHOD CopyClipRegion(nsIRegion &aRegion);
   NS_IMETHOD GetClipRegion(nsIRegion **aRegion);
   
   NS_IMETHOD SetLineStyle(nsLineStyle aLineStyle);
   NS_IMETHOD GetLineStyle(nsLineStyle &aLineStyle);
   
   NS_IMETHOD SetColor(nscolor aColor);
   NS_IMETHOD GetColor(nscolor &aColor) const;
   
   NS_IMETHOD SetFont(const nsFont& aFont, nsIAtom* aLangGroup);
   NS_IMETHOD SetFont(nsIFontMetrics *aFontMetrics);
   
   NS_IMETHOD GetFontMetrics(nsIFontMetrics *&aFontMetrics);
   
   NS_IMETHOD Translate(nscoord aX, nscoord aY);
   NS_IMETHOD Scale(float aSx, float aSy);
   NS_IMETHOD GetCurrentTransform(nsTransform2D *&aTransform);
   
   NS_IMETHOD CreateDrawingSurface(nsRect *aBounds, PRUint32 aSurfFlags, nsDrawingSurface &aSurface);
   NS_IMETHOD DestroyDrawingSurface(nsDrawingSurface aDS);
   
   NS_IMETHOD DrawLine(nscoord aX0, nscoord aY0, nscoord aX1, nscoord aY1);
   NS_IMETHOD DrawStdLine(nscoord aX0, nscoord aY0, nscoord aX1, nscoord aY1);
   NS_IMETHOD DrawPolyline(const nsPoint aPoints[], PRInt32 aNumPoints);
   
   NS_IMETHOD DrawRect(const nsRect& aRect);
   NS_IMETHOD DrawRect(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight);
   
   NS_IMETHOD FillRect(const nsRect& aRect);
   NS_IMETHOD FillRect(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight);
   
   NS_IMETHOD InvertRect(const nsRect& aRect);
   NS_IMETHOD InvertRect(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight);
   
   NS_IMETHOD DrawPolygon(const nsPoint aPoints[], PRInt32 aNumPoints);
   NS_IMETHOD FillPolygon(const nsPoint aPoints[], PRInt32 aNumPoints);
   
   NS_IMETHOD DrawEllipse(const nsRect& aRect);
   NS_IMETHOD DrawEllipse(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight);
   NS_IMETHOD FillEllipse(const nsRect& aRect);
   NS_IMETHOD FillEllipse(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight);
   
   NS_IMETHOD DrawArc(const nsRect& aRect,
					  float aStartAngle, float aEndAngle);
   NS_IMETHOD DrawArc(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight,
					  float aStartAngle, float aEndAngle);
   NS_IMETHOD FillArc(const nsRect& aRect,
					  float aStartAngle, float aEndAngle);
   NS_IMETHOD FillArc(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight,
					  float aStartAngle, float aEndAngle);

   NS_IMETHOD GetWidth(char aC, nscoord& aWidth);
   NS_IMETHOD GetWidth(PRUnichar aC, nscoord& aWidth,
					   PRInt32 *aFontID);
   NS_IMETHOD GetWidth(const nsString& aString, nscoord& aWidth,
					   PRInt32 *aFontID);
   NS_IMETHOD GetWidth(const char* aString, nscoord& aWidth);
   NS_IMETHOD GetWidth(const char* aString, PRUint32 aLength, nscoord& aWidth);
   NS_IMETHOD GetWidth(const PRUnichar* aString, PRUint32 aLength,
					   nscoord& aWidth, PRInt32 *aFontID);
   
   NS_IMETHOD DrawString(const char *aString, PRUint32 aLength,
						 nscoord aX, nscoord aY,
						 const nscoord* aSpacing);
   NS_IMETHOD DrawString(const PRUnichar *aString, PRUint32 aLength,
						 nscoord aX, nscoord aY,
						 PRInt32 aFontID,
						 const nscoord* aSpacing);
   NS_IMETHOD DrawString(const nsString& aString, nscoord aX, nscoord aY,
						 PRInt32 aFontID,
						 const nscoord* aSpacing);
   NS_IMETHOD GetTextDimensions(const char* aString, PRUint32 aLength,
								nsTextDimensions& aDimensions);
   NS_IMETHOD GetTextDimensions(const PRUnichar *aString, PRUint32 aLength,
								nsTextDimensions& aDimensions, PRInt32 *aFontID);
   
   NS_IMETHOD DrawImage(nsIImage *aImage, nscoord aX, nscoord aY);
   NS_IMETHOD DrawImage(nsIImage *aImage, nscoord aX, nscoord aY,
						nscoord aWidth, nscoord aHeight); 
   NS_IMETHOD DrawImage(nsIImage *aImage, const nsRect& aRect);
   NS_IMETHOD DrawImage(nsIImage *aImage, const nsRect& aSRect, const nsRect& aDRect);
   NS_IMETHOD DrawTile(nsIImage *aImage,nscoord aX0,nscoord aY0,nscoord aX1,nscoord aY1,
					   nscoord aWidth,nscoord aHeight);
   NS_IMETHOD DrawTile(nsIImage *aImage, nscoord aSrcXOffset, nscoord aSrcYOffset,
					   const nsRect &aTileRect);
   
   NS_IMETHOD CopyOffScreenBits(nsDrawingSurface aSrcSurf, PRInt32 aSrcX, PRInt32 aSrcY,
								const nsRect &aDestBounds, PRUint32 aCopyFlags);
   NS_IMETHOD RetrieveCurrentNativeGraphicData(PRUint32 * ngd);

#ifdef MOZ_MATHML
  /**
   * Returns metrics (in app units) of an 8-bit character string
   */
  NS_IMETHOD GetBoundingMetrics(const char*        aString,
                                PRUint32           aLength,
                                nsBoundingMetrics& aBoundingMetrics);

  /**
   * Returns metrics (in app units) of a Unicode character string
   */
  NS_IMETHOD GetBoundingMetrics(const PRUnichar*   aString,
                                PRUint32           aLength,
                                nsBoundingMetrics& aBoundingMetrics,
                                PRInt32*           aFontID = nsnull);

#endif /* MOZ_MATHML */

   
   // nsIRenderingContextPh
   // NS_IMETHOD CreateDrawingSurface(PhGC_t *aGC, nsDrawingSurface &aSurface);

//   NS_IMETHOD SetClipRegion(PhTile_t *aTileList, nsClipCombine aCombine, PRBool &aClipState);
   NS_IMETHOD CommonInit();
   
   void CreateClipRegion() {
	   static NS_DEFINE_CID(kRegionCID, NS_REGION_CID);
	   if (mClipRegion)
		   return;
	   
	   PRUint32 w, h;
	   mSurface->GetSize(&w, &h);
	   
	   mClipRegion = do_CreateInstance(kRegionCID);
	   if (mClipRegion) {
		   mClipRegion->Init();
		   mClipRegion->SetTo(0,0,w,h);
	   }
   }
private:
   void ApplyClipping( PhGC_t *);
#if 0
   void holdSetGC();
   void SetGC();
   void RestoreGC();
   void StartDrawing(PhDrawContext_t *dc, PhGC_t *gc);
   void StopDrawing();
#endif   
protected:
   PhDrawContext_t    *mPtDC;
   PhGC_t             *mGC;
   PhGC_t             *mholdGC;
   nscolor            mCurrentColor;
   nsLineStyle        mCurrentLineStyle;
   unsigned char      mLineStyle[2];
   nsIFontMetrics     *mFontMetrics;
   nsDrawingSurfacePh *mOffscreenSurface;
   nsDrawingSurfacePh *mSurface;
   nsDrawingSurface   mMainSurface;
   nsIWidget          *mDCOwner;
   nsIDeviceContext   *mContext;
   float              mP2T;
   nsCOMPtr<nsIRegion>   mClipRegion;
   PtWidget_t         *mWidget;
   char               *mPhotonFontName;
   nsRegionPh         *mGlobalClip;
   PhDrawContext_t 	*mOldDC;
   PhGC_t			*mOldGC;
   
   //default objects
   //state management
   nsVoidArray       *mStateCache;
   
   static PhGC_t     *mPtGC;				/* Default Photon Graphics Context */
   PRBool            mInitialized;
   void UpdateGC();
   // ConditionRect is used to fix coordinate overflow problems for
   // rectangles after they are transformed to screen coordinates
   
   void ConditionRect(nscoord &x, nscoord &y, nscoord &w, nscoord &h) {
	   if ( y < -32766 )
		   y = -32766;
	   if ( y + h > 32766 )
		   h  = 32766 - y;
	   if ( x < -32766 )
		   x = -32766;
	   if ( x + w > 32766 ) 
		   w  = 32766 - x;
   }
};

#endif /* nsRenderingContextPh_h___ */
