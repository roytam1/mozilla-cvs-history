/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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

#include "nsDrawingSurfaceQT.h"
#include "nsRenderingContextQT.h"

static NS_DEFINE_IID(kIDrawingSurfaceIID, NS_IDRAWING_SURFACE_IID);
static NS_DEFINE_IID(kIDrawingSurfaceQTIID, NS_IDRAWING_SURFACE_QT_IID);

nsDrawingSurfaceQT::nsDrawingSurfaceQT()
{
    PR_LOG(QtGfxLM, PR_LOG_DEBUG, ("nsDrawingSurfaceQT::nsDrawingSurfaceQT\n"));
    NS_INIT_REFCNT();

    mPaintDevice = nsnull;
    mGC          = nsnull;
    mDepth       = 0;
    mWidth       = 0;
    mHeight      = 0;
    mFlags       = 0;
    mLockWidth   = 0;
    mLockHeight  = 0;
    mLockFlags   = 0;
    mLocked      = PR_FALSE;
    mCleanup     = PR_TRUE;

    // I have no idea how to compute these values.
/*
  mPixFormat.mRedMask = v->red_mask;
  mPixFormat.mGreenMask = v->green_mask;
  mPixFormat.mBlueMask = v->blue_mask;
  // FIXME
  mPixFormat.mAlphaMask = 0;
  
  mPixFormat.mRedShift = v->red_shift;
  mPixFormat.mGreenShift = v->green_shift;
  mPixFormat.mBlueShift = v->blue_shift;
  // FIXME
  mPixFormat.mAlphaShift = 0;
  mDepth = v->depth;
*/
}

nsDrawingSurfaceQT::~nsDrawingSurfaceQT()
{
    PR_LOG(QtGfxLM, PR_LOG_DEBUG, ("nsDrawingSurfaceQT::~nsDrawingSurfaceQT\n"));
    if (mPaintDevice)
    {
        // Should I really be destroying this? It's possible that this pixmap
        // wasn't created by this class.
        //delete mPixmap;
    }

    if (mGC && mCleanup)
    {
        PR_LOG(QtGfxLM, 
               PR_LOG_DEBUG, 
               ("nsDrawingSurfaceQT::~nsDrawingSurfaceQT: calling QPainter::end for %p\n",
                mPaintDevice));
        mGC->end();
    }
}

NS_IMETHODIMP nsDrawingSurfaceQT::QueryInterface(REFNSIID aIID, 
                                                 void** aInstancePtr)
{
    PR_LOG(QtGfxLM, PR_LOG_DEBUG, ("nsDrawingSurfaceQT::QueryInterface\n"));
    if (nsnull == aInstancePtr)
    {
        return NS_ERROR_NULL_POINTER;
    }

    if (aIID.Equals(kIDrawingSurfaceIID))
    {
        nsIDrawingSurface* tmp = this;
        *aInstancePtr = (void*) tmp;
        NS_ADDREF_THIS();
        return NS_OK;
    }

    if (aIID.Equals(kIDrawingSurfaceQTIID))
    {
        nsDrawingSurfaceQT* tmp = this;
        *aInstancePtr = (void*) tmp;
        NS_ADDREF_THIS();
        return NS_OK;
    }

    static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);

    if (aIID.Equals(kISupportsIID))
    {
        nsIDrawingSurface* tmp = this;
        nsISupports* tmp2 = tmp;
        *aInstancePtr = (void*) tmp2;
        NS_ADDREF_THIS();
        return NS_OK;
    }

    return NS_NOINTERFACE;
}

NS_IMPL_ADDREF(nsDrawingSurfaceQT)
NS_IMPL_RELEASE(nsDrawingSurfaceQT)

NS_IMETHODIMP nsDrawingSurfaceQT::Lock(PRInt32   aX, 
                                       PRInt32   aY,
                                       PRUint32  aWidth, 
                                       PRUint32  aHeight,
                                       void   ** aBits, 
                                       PRInt32 * aStride,
                                       PRInt32 * aWidthBytes, 
                                       PRUint32  aFlags)
{
    PR_LOG(QtGfxLM, PR_LOG_DEBUG, ("nsDrawingSurfaceQT::Lock\n"));
    if (mLocked)
    {
        NS_ASSERTION(0, "nested lock attempt");
        return NS_ERROR_FAILURE;
    }

    mLocked     = PR_TRUE;
    mLockX      = aX;
    mLockY      = aY;
    mLockWidth  = aWidth;
    mLockHeight = aHeight;
    mLockFlags  = aFlags;

    // For now, we will lock the drawing surface when it is initialized. This
    // will probably need to be improved.

    //mGC->begin(mPaintDevice);

    return NS_OK;
}

NS_IMETHODIMP nsDrawingSurfaceQT::Unlock(void)
{
    PR_LOG(QtGfxLM, PR_LOG_DEBUG, ("nsDrawingSurfaceQT::Unlock\n"));
    if (!mLocked)
    {
        NS_ASSERTION(0, "attempting to unlock an DS that isn't locked");
        return NS_ERROR_FAILURE;
    }

    if (!(mFlags & NS_LOCK_SURFACE_READ_ONLY))
    {
#if 0
        mGC->drawPixmap(0, 
                        0, 
                        *mPaintDevice, 
                        mLockY, 
                        mLockY, 
                        mLockWidth, 
                        mLockHeight);
#endif
    }

    delete mPaintDevice;
    mPaintDevice = nsnull;

    mLocked = PR_FALSE;

    return NS_OK;
}

NS_IMETHODIMP nsDrawingSurfaceQT::GetDimensions(PRUint32 *aWidth, 
                                                PRUint32 *aHeight)
{
    PR_LOG(QtGfxLM, PR_LOG_DEBUG, ("nsDrawingSurfaceQT::GetDimensions\n"));
    *aWidth = mWidth;
    *aHeight = mHeight;

    return NS_OK;
}

NS_IMETHODIMP nsDrawingSurfaceQT::IsOffscreen(PRBool *aOffScreen)
{
    PR_LOG(QtGfxLM, PR_LOG_DEBUG, ("nsDrawingSurfaceQT::IsOffscreen\n"));
    *aOffScreen = mIsOffscreen;
    return NS_OK;
}

NS_IMETHODIMP nsDrawingSurfaceQT::IsPixelAddressable(PRBool *aAddressable)
{
    PR_LOG(QtGfxLM, PR_LOG_DEBUG, ("nsDrawingSurfaceQT::IsPixelAddressable\n"));
    *aAddressable = PR_FALSE;
    return NS_OK;
}

NS_IMETHODIMP nsDrawingSurfaceQT::GetPixelFormat(nsPixelFormat *aFormat)
{
    PR_LOG(QtGfxLM, PR_LOG_DEBUG, ("nsDrawingSurfaceQT::GetPixelFormat\n"));
    *aFormat = mPixFormat;

    return NS_OK;
}

NS_IMETHODIMP nsDrawingSurfaceQT::Init(QPaintDevice * aPaintDevice, 
                                       QPainter *aGC)
{
    PR_LOG(QtGfxLM, 
           PR_LOG_DEBUG, 
           ("nsDrawingSurfaceQT::Init: pixmap=%p, painter=%p\n",
            aPaintDevice,
            aGC));
    mGC          = aGC;
    mPaintDevice = aPaintDevice;
    mIsOffscreen = PR_FALSE;

    return CommonInit();
}

NS_IMETHODIMP nsDrawingSurfaceQT::Init(QPainter *aGC, 
                                       PRUint32 aWidth,
                                       PRUint32 aHeight, 
                                       PRUint32 aFlags)
{
    PR_LOG(QtGfxLM, PR_LOG_DEBUG, ("nsDrawingSurfaceQT::Init: with no pixmap\n"));

    if (nsnull  == aGC ||
        aWidth  <= 0   ||
        aHeight <= 0)
    {
        return NS_ERROR_FAILURE;
    }

    mGC          = aGC;
    mWidth       = aWidth;
    mHeight      = aHeight;
    mFlags       = aFlags;
    mIsOffscreen = PR_TRUE;

    PR_LOG(QtGfxLM, 
           PR_LOG_DEBUG, 
           ("nsDrawingSurfaceQT::Init: creating pixmap w=%d, h=%d, d=%d\n", 
            mWidth, 
            mHeight, 
            mDepth));
    mPaintDevice = new QPixmap(mWidth, mHeight, mDepth);

    return CommonInit();
}


NS_IMETHODIMP nsDrawingSurfaceQT::CommonInit()
{
    PR_LOG(QtGfxLM, PR_LOG_DEBUG, ("nsDrawingSurfaceQT::CommonInit\n"));

    if (mGC->begin(mPaintDevice))
    {
        PR_LOG(QtGfxLM, 
               PR_LOG_DEBUG, 
               ("nsDrawingSurfaceQT::CommonInit: QPainter::begin succeeded for %p\n",
                mPaintDevice));
    }
    else
    {
        PR_LOG(QtGfxLM, 
               PR_LOG_DEBUG, 
               ("nsDrawingSurfaceQT::CommonInit: QPainter::begin failed for %p\n",
                mPaintDevice));
        mCleanup = PR_FALSE;
    }

    return NS_OK;
}

QPainter * nsDrawingSurfaceQT::GetGC()
{
    PR_LOG(QtGfxLM, PR_LOG_DEBUG, ("nsDrawingSurfaceQT::GetGC\n"));
    return mGC;
}

QPaintDevice * nsDrawingSurfaceQT::GetPaintDevice()
{
    PR_LOG(QtGfxLM, PR_LOG_DEBUG, ("nsDrawingSurfaceQT::GetPaintDevice\n"));
    return mPaintDevice;
}
