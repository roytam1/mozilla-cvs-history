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

#ifndef nsDrawingSurfaceXlib_h__
#define nsDrawingSurfaceXlib_h__

#include "nsIDrawingSurface.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>

class nsDrawingSurfaceXlib : public nsIDrawingSurface
{
public:
  nsDrawingSurfaceXlib();
  ~nsDrawingSurfaceXlib();

  NS_DECL_ISUPPORTS
  
  NS_IMETHOD Lock(PRInt32 aX, PRInt32 aY, PRUint32 aWidth, PRUint32 aHeight,
                  void **aBits, PRInt32 *aStride, PRInt32 *aWidthBytes,
                  PRUint32 aFlags);
  NS_IMETHOD Unlock(void);
  NS_IMETHOD GetDimensions(PRUint32 *aWidth, PRUint32 *aHeight);
  NS_IMETHOD IsOffscreen(PRBool *aOffScreen);
  NS_IMETHOD IsPixelAddressable(PRBool *aAddressable);
  NS_IMETHOD GetPixelFormat(nsPixelFormat *aFormat);
  NS_IMETHOD Init (Drawable aDrawable, GC aGC);
  NS_IMETHOD Init (GC aGC, PRUint32 aWidth, PRUint32 aHeight, PRUint32 aFlags);
  GC         GetGC(void) { return mGC; }
  Drawable   GetDrawable(void) { return mPixmap; }  

private:
  GC mGC;
  Pixmap mPixmap;
  XImage *mImage;
  nsPixelFormat mPixFormat;
  PRUint8 mDepth;
  // for locking
  PRInt32	mLockX;
  PRInt32	mLockY;
  PRUint32	mLockWidth;
  PRUint32	mLockHeight;
  PRUint32	mLockFlags;
  PRBool	mLocked;
  // dimensions
  PRUint32 mWidth;
  PRUint32 mHeight;
  // are we offscreen
  PRBool mIsOffscreen;
};

#endif
