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

#ifndef nsRenderingContextUnix_h___
#define nsRenderingContextUnix_h___

#include "nsIRenderingContext.h"
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
#include "nsImageUnix.h"
#include "nsIDeviceContext.h"
#include "nsVoidArray.h"
#include "nsIRegion.h"
#include "nsDeviceContextUnix.h"

#include "X11/Xlib.h"
#include "X11/Xutil.h"

#ifdef MITSHM
#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>
#endif

class GraphicsState;

class nsRenderingContextUnix : public nsIRenderingContext
{
public:
  nsRenderingContextUnix();
  ~nsRenderingContextUnix();

  void* operator new(size_t sz) {
    void* rv = new char[sz];
    nsCRT::zero(rv, sz);
    return rv;
  }

  NS_DECL_ISUPPORTS

  NS_IMETHOD Init(nsIDeviceContext* aContext, nsIWidget *aWindow);
  NS_IMETHOD Init(nsIDeviceContext* aContext, nsDrawingSurface aSurface);
  virtual nsresult CommonInit();

  virtual void Reset();

  virtual nsIDeviceContext * GetDeviceContext(void);

  NS_IMETHOD SelectOffScreenDrawingSurface(nsDrawingSurface aSurface);
  NS_IMETHOD GetHints(PRUint32& aResult);

  virtual void PushState(void);
  virtual PRBool PopState(void);

  virtual PRBool IsVisibleRect(const nsRect& aRect);

  virtual PRBool SetClipRectInPixels(const nsRect& aRect, nsClipCombine aCombine);
  virtual PRBool SetClipRect(const nsRect& aRect, nsClipCombine aCombine);
  virtual PRBool GetClipRect(nsRect &aRect);
  virtual PRBool SetClipRegion(const nsIRegion& aRegion, nsClipCombine aCombine);
  virtual void GetClipRegion(nsIRegion **aRegion);

  NS_IMETHOD SetLineStyle(nsLineStyle aLineStyle);
  NS_IMETHOD GetLineStyle(nsLineStyle &aLineStyle);

  virtual void SetColor(nscolor aColor);
  virtual nscolor GetColor() const;

  virtual void SetFont(const nsFont& aFont);
  virtual const nsFont& GetFont();

  virtual nsIFontMetrics * GetFontMetrics();

  virtual void Translate(nscoord aX, nscoord aY);
  virtual void Scale(float aSx, float aSy);
  virtual nsTransform2D * GetCurrentTransform();

  virtual nsDrawingSurface CreateDrawingSurface(nsRect *aBounds);
  virtual void DestroyDrawingSurface(nsDrawingSurface aDS);

  virtual void DrawLine(nscoord aX0, nscoord aY0, nscoord aX1, nscoord aY1);
  virtual void DrawPolyline(const nsPoint aPoints[], PRInt32 aNumPoints);

  virtual void DrawRect(const nsRect& aRect);
  virtual void DrawRect(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight);
  virtual void FillRect(const nsRect& aRect);
  virtual void FillRect(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight);

  virtual void DrawPolygon(const nsPoint aPoints[], PRInt32 aNumPoints);
  virtual void FillPolygon(const nsPoint aPoints[], PRInt32 aNumPoints);

  virtual void DrawEllipse(const nsRect& aRect);
  virtual void DrawEllipse(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight);
  virtual void FillEllipse(const nsRect& aRect);
  virtual void FillEllipse(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight);

  virtual void DrawArc(const nsRect& aRect,
                       float aStartAngle, float aEndAngle);
  virtual void DrawArc(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight,
                       float aStartAngle, float aEndAngle);
  virtual void FillArc(const nsRect& aRect,
                       float aStartAngle, float aEndAngle);
  virtual void FillArc(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight,
                       float aStartAngle, float aEndAngle);

  virtual void DrawString(const char *aString, PRUint32 aLength,
                          nscoord aX, nscoord aY,
                          nscoord aWidth);
  virtual void DrawString(const PRUnichar *aString, PRUint32 aLength, nscoord aX, nscoord aY,
                          nscoord aWidth);
  virtual void DrawString(const nsString& aString, nscoord aX, nscoord aY,
                          nscoord aWidth);

  virtual void DrawImage(nsIImage *aImage, nscoord aX, nscoord aY);
  virtual void DrawImage(nsIImage *aImage, nscoord aX, nscoord aY,
                         nscoord aWidth, nscoord aHeight); 
  virtual void DrawImage(nsIImage *aImage, const nsRect& aRect);
  virtual void DrawImage(nsIImage *aImage, const nsRect& aSRect, const nsRect& aDRect);

  NS_IMETHOD CopyOffScreenBits(nsRect &aBounds);

protected:

  nscolor mCurrentColor ;
  nsTransform2D		  *mTMatrix;		// transform that all the graphics drawn here will obey
  float             mP2T;

  nsDrawingSurfaceUnix   *mRenderingSurface;  // Can be a BackBuffer if Selected in
  nsDrawingSurfaceUnix   *mFrontBuffer;
  nsIDeviceContext       *mContext;
  nsIFontMetrics         *mFontMetrics;
  Region                 mRegion;
  Font                   mCurrFontHandle;
  XChar2b*               mDrawStringBuf;
  PRInt32                mDrawStringSize;
  nsLineStyle            mCurrentLineStyle;

  //state management
  nsVoidArray       *mStateCache;

#ifdef MITSHM
private:
  PRBool  mHasSharedMemory;
  PRBool  mSupportsSharedPixmaps;
#endif

};

#endif /* nsRenderingContextUnix_h___ */
