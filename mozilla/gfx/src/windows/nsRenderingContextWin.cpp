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

#include "nsRenderingContextWin.h"
#include "nsFontMetricsWin.h"
#include "nsRegionWin.h"
#include <math.h>
#include "libimg.h"
#include "nsDeviceContextWin.h"
#include "prprf.h"
#include "nsDrawingSurfaceWin.h"
#include "nsGfxCIID.h"

//#define GFX_DEBUG

#ifdef GFX_DEBUG
  #define BREAK_TO_DEBUGGER           DebugBreak()
#else   
  #define BREAK_TO_DEBUGGER
#endif  

#ifdef GFX_DEBUG
  #define VERIFY(exp)                 ((exp) ? 0: (GetLastError(), BREAK_TO_DEBUGGER))
#else   // !_DEBUG
  #define VERIFY(exp)                 (exp)
#endif  // !_DEBUG

#define  ARABIC_HEBREW_RENDERING

static NS_DEFINE_IID(kIRenderingContextIID, NS_IRENDERING_CONTEXT_IID);
static NS_DEFINE_IID(kIRenderingContextWinIID, NS_IRENDERING_CONTEXT_WIN_IID);
static NS_DEFINE_IID(kIDrawingSurfaceIID, NS_IDRAWING_SURFACE_IID);
static NS_DEFINE_IID(kDrawingSurfaceCID, NS_DRAWING_SURFACE_CID);

#define FLAG_CLIP_VALID       0x0001
#define FLAG_CLIP_CHANGED     0x0002
#define FLAG_LOCAL_CLIP_VALID 0x0004

#define FLAGS_ALL             (FLAG_CLIP_VALID | FLAG_CLIP_CHANGED | FLAG_LOCAL_CLIP_VALID)

// Macro for creating a palette relative color if you have a COLORREF instead
// of the reg, green, and blue values. The color is color-matches to the nearest
// in the current logical palette. This has no effect on a non-palette device
#define PALETTERGB_COLORREF(c)  (0x02000000 | (c))

/*
 * This is actually real fun.  Windows does not draw dotted lines with Pen's
 * directly (Go ahead, try it, you'll get dashes).
 *
 * the trick is to install a callback and actually put the pixels in
 * directly. This function will get called for each pixel in the line.
 *
 */

static PRBool gFastDDASupport = PR_FALSE;

typedef struct lineddastructtag
{
   int   nDottedPixel;
   HDC   dc;
   COLORREF crColor;
} lineddastruct;


void CALLBACK LineDDAFunc(int x,int y,LONG lData)
{
  lineddastruct * dda_struct = (lineddastruct *) lData;
  
  dda_struct->nDottedPixel ^= 1; 

  if (dda_struct->nDottedPixel)
    SetPixel(dda_struct->dc, x, y, dda_struct->crColor);
}   



class GraphicsState
{
public:
  GraphicsState();
  GraphicsState(GraphicsState &aState);
  ~GraphicsState();

  GraphicsState   *mNext;
  nsTransform2D   mMatrix;
  nsRect          mLocalClip;
  HRGN            mClipRegion;
  nscolor         mColor;
  COLORREF        mColorREF;
  nsIFontMetrics  *mFontMetrics;
  HPEN            mSolidPen;
  HPEN            mDashedPen;
  HPEN            mDottedPen;
  PRInt32         mFlags;
  nsLineStyle     mLineStyle;
};

GraphicsState :: GraphicsState()
{
  mNext = nsnull;
  mMatrix.SetToIdentity();  
  mLocalClip.x = mLocalClip.y = mLocalClip.width = mLocalClip.height = 0;
  mClipRegion = NULL;
  mColor = NS_RGB(0, 0, 0);
  mColorREF = RGB(0, 0, 0);
  mFontMetrics = nsnull;
  mSolidPen = NULL;
  mDashedPen = NULL;
  mDottedPen = NULL;
  mFlags = ~FLAGS_ALL;
  mLineStyle = nsLineStyle_kSolid;
}

GraphicsState :: GraphicsState(GraphicsState &aState) :
                               mMatrix(&aState.mMatrix),
                               mLocalClip(aState.mLocalClip)
{
  mNext = &aState;
  mClipRegion = NULL;
  mColor = NS_RGB(0, 0, 0);
  mColorREF = RGB(0, 0, 0);
  mFontMetrics = nsnull;
  mSolidPen = NULL;
  mDashedPen = NULL;
  mDottedPen = NULL;
  mFlags = ~FLAGS_ALL;
  mLineStyle = aState.mLineStyle;
}

GraphicsState :: ~GraphicsState()
{
  if (NULL != mClipRegion)
  {
    VERIFY(::DeleteObject(mClipRegion));
    mClipRegion = NULL;
  }

  //these are killed by the rendering context...
  mSolidPen = NULL;
  mDashedPen = NULL;
  mDottedPen = NULL;
}

#define NOT_SETUP 0x33
static PRBool gIsWIN95 = NOT_SETUP;

nsRenderingContextWin :: nsRenderingContextWin()
{
  NS_INIT_REFCNT();

  // The first time in we initialize gIsWIN95 flag & gFastDDASupport flag
  if (NOT_SETUP == gIsWIN95) {
    OSVERSIONINFO os;
    os.dwOSVersionInfoSize = sizeof(os);
    ::GetVersionEx(&os);
    // XXX This may need tweaking for win98
    if (VER_PLATFORM_WIN32_NT == os.dwPlatformId) {
      gIsWIN95 = PR_FALSE;
    }
    else {
      gIsWIN95 = PR_TRUE;
    }
  }

  mDC = NULL;
  mMainDC = NULL;
  mDCOwner = nsnull;
  mFontMetrics = nsnull;
  mOrigSolidBrush = NULL;
  mBlackBrush = NULL;
  mOrigFont = NULL;
  mDefFont = NULL;
  mOrigSolidPen = NULL;
  mBlackPen = NULL;
  mOrigPalette = NULL;
  mCurrBrushColor = NULL;
  mCurrFontMetrics = nsnull;
  mCurrPenColor = NULL;
  mCurrPen = NULL;
  mNullPen = NULL;
  mCurrTextColor = NS_RGB(0, 0, 0);
  mCurrLineStyle = nsLineStyle_kSolid;
#ifdef NS_DEBUG
  mInitialized = PR_FALSE;
#endif
  mSurface = nsnull;
  mMainSurface = nsnull;

  mStateCache = new nsVoidArray();

  //create an initial GraphicsState

  PushState();

  mP2T = 1.0f;
}

nsRenderingContextWin :: ~nsRenderingContextWin()
{
  NS_IF_RELEASE(mContext);
  NS_IF_RELEASE(mFontMetrics);

  //destroy the initial GraphicsState

  PRBool clipState;
  PopState(clipState);

  //cleanup the DC so that we can just destroy objects
  //in the graphics state without worrying that we are
  //ruining the dc

  if (NULL != mDC)
  {
    if (NULL != mOrigSolidBrush)
    {
      ::SelectObject(mDC, mOrigSolidBrush);
      mOrigSolidBrush = NULL;
    }

    if (NULL != mOrigFont)
    {
      ::SelectObject(mDC, mOrigFont);
      mOrigFont = NULL;
    }

    if (NULL != mDefFont)
    {
      VERIFY(::DeleteObject(mDefFont));
      mDefFont = NULL;
    }

    if (NULL != mOrigSolidPen)
    {
      ::SelectObject(mDC, mOrigSolidPen);
      mOrigSolidPen = NULL;
    }
  }

  if (NULL != mCurrBrush)
    VERIFY(::DeleteObject(mCurrBrush));

  if ((NULL != mBlackBrush) && (mBlackBrush != mCurrBrush))
    VERIFY(::DeleteObject(mBlackBrush));

  mCurrBrush = NULL;
  mBlackBrush = NULL;

  //don't kill the font because the font cache/metrics owns it
  mCurrFont = NULL;

  if (NULL != mCurrPen)
    VERIFY(::DeleteObject(mCurrPen));

  if ((NULL != mBlackPen) && (mBlackPen != mCurrPen))
    VERIFY(::DeleteObject(mBlackPen));

  if ((NULL != mNullPen) && (mNullPen != mCurrPen))
    VERIFY(::DeleteObject(mNullPen));

  mCurrPen = NULL;
  mBlackPen = NULL;
  mNullPen = NULL;

  if (nsnull != mStateCache)
  {
    PRInt32 cnt = mStateCache->Count();

    while (--cnt >= 0)
    {
      GraphicsState *state = (GraphicsState *)mStateCache->ElementAt(cnt);
      mStateCache->RemoveElementAt(cnt);

      if (nsnull != state)
        delete state;
    }

    delete mStateCache;
    mStateCache = nsnull;
  }

  if (nsnull != mSurface)
  {
    mSurface->ReleaseDC();
    NS_RELEASE(mSurface);
  }

  if (nsnull != mMainSurface)
  {
    mMainSurface->ReleaseDC();
    NS_RELEASE(mMainSurface);
  }

  if (nsnull != mDCOwner)
  {
    ::ReleaseDC((HWND)mDCOwner->GetNativeData(NS_NATIVE_WINDOW), mMainDC);
    NS_RELEASE(mDCOwner);
  }

  mTMatrix = nsnull;
  mDC = NULL;
  mMainDC = NULL;
}

nsresult
nsRenderingContextWin :: QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
  if (nsnull == aInstancePtr)
    return NS_ERROR_NULL_POINTER;

  if (aIID.Equals(kIRenderingContextIID))
  {
    nsIRenderingContext* tmp = this;
    *aInstancePtr = (void*) tmp;
    NS_ADDREF_THIS();
    return NS_OK;
  }

  if (aIID.Equals(kIRenderingContextWinIID))
  {
    nsIRenderingContextWin* tmp = this;
    *aInstancePtr = (void*) tmp;
    NS_ADDREF_THIS();
    return NS_OK;
  }

  static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);

  if (aIID.Equals(kISupportsIID))
  {
    nsIRenderingContext* tmp = this;
    nsISupports* tmp2 = tmp;
    *aInstancePtr = (void*) tmp2;
    NS_ADDREF_THIS();
    return NS_OK;
  }

  return NS_NOINTERFACE;
}

NS_IMPL_ADDREF(nsRenderingContextWin)
NS_IMPL_RELEASE(nsRenderingContextWin)

NS_IMETHODIMP
nsRenderingContextWin :: Init(nsIDeviceContext* aContext,
                              nsIWidget *aWindow)
{
  NS_PRECONDITION(PR_FALSE == mInitialized, "double init");

  mContext = aContext;
  NS_IF_ADDREF(mContext);

  mSurface = (nsDrawingSurfaceWin *)new nsDrawingSurfaceWin();

  if (nsnull != mSurface)
  {
    HDC tdc = (HDC)aWindow->GetNativeData(NS_NATIVE_GRAPHIC);

    NS_ADDREF(mSurface);
    mSurface->Init(tdc);
    mDC = tdc;

    mMainDC = mDC;
    mMainSurface = mSurface;
    NS_ADDREF(mMainSurface);
  }

  mDCOwner = aWindow;

  NS_IF_ADDREF(mDCOwner);

  return CommonInit();
}

NS_IMETHODIMP
nsRenderingContextWin :: Init(nsIDeviceContext* aContext,
                              nsDrawingSurface aSurface)
{
  NS_PRECONDITION(PR_FALSE == mInitialized, "double init");

  mContext = aContext;
  NS_IF_ADDREF(mContext);

  mSurface = (nsDrawingSurfaceWin *)aSurface;

  if (nsnull != mSurface)
  {
    NS_ADDREF(mSurface);
    mSurface->GetDC(&mDC);

    mMainDC = mDC;
    mMainSurface = mSurface;
    NS_ADDREF(mMainSurface);
  }

  mDCOwner = nsnull;

  return CommonInit();
}

nsresult nsRenderingContextWin :: SetupDC(HDC aOldDC, HDC aNewDC)
{
  HBRUSH  prevbrush;
  HFONT   prevfont;
  HPEN    prevpen;

  ::SetTextColor(aNewDC, PALETTERGB_COLORREF(mColor));
  mCurrTextColor = mCurrentColor;
  ::SetBkMode(aNewDC, TRANSPARENT);
  ::SetPolyFillMode(aNewDC, WINDING);
  ::SetStretchBltMode(aNewDC, COLORONCOLOR);

  if (nsnull != aOldDC)
  {
    if (nsnull != mOrigSolidBrush)
      prevbrush = (HBRUSH)::SelectObject(aOldDC, mOrigSolidBrush);

    if (nsnull != mOrigFont)
      prevfont = (HFONT)::SelectObject(aOldDC, mOrigFont);

    if (nsnull != mOrigSolidPen)
      prevpen = (HPEN)::SelectObject(aOldDC, mOrigSolidPen);

    if (nsnull != mOrigPalette)
      ::SelectPalette(aOldDC, mOrigPalette, TRUE);
  }
  else
  {
    prevbrush = mBlackBrush;
    prevfont = mDefFont;
    prevpen = mBlackPen;
  }

  mOrigSolidBrush = (HBRUSH)::SelectObject(aNewDC, prevbrush);
  mOrigFont = (HFONT)::SelectObject(aNewDC, prevfont);
  mOrigSolidPen = (HPEN)::SelectObject(aNewDC, prevpen);

#if 0
  GraphicsState *pstate = mStates;

  while ((nsnull != pstate) && !(pstate->mFlags & FLAG_CLIP_VALID))
    pstate = pstate->mNext;

  if (nsnull != pstate)
    ::SelectClipRgn(aNewDC, pstate->mClipRegion);
#endif

  // If this is a palette device, then select and realize the palette
  nsPaletteInfo palInfo;
  mContext->GetPaletteInfo(palInfo);

  if (palInfo.isPaletteDevice && palInfo.palette)
  {
    // Select the palette in the background
    mOrigPalette = ::SelectPalette(aNewDC, (HPALETTE)palInfo.palette, TRUE);
    // Don't do the realization for an off-screen memory DC
    if (nsnull == aOldDC)
      ::RealizePalette(aNewDC);
  }

  if (GetDeviceCaps(aNewDC, RASTERCAPS) & (RC_BITBLT))
    gFastDDASupport = PR_TRUE;

  return NS_OK;
}

nsresult nsRenderingContextWin :: CommonInit(void)
{
  float app2dev;

  mContext->GetAppUnitsToDevUnits(app2dev);
	mTMatrix->AddScale(app2dev, app2dev);
  mContext->GetDevUnitsToAppUnits(mP2T);

#ifdef NS_DEBUG
  mInitialized = PR_TRUE;
#endif

  mBlackBrush = (HBRUSH)::GetStockObject(BLACK_BRUSH);
  mDefFont = ::CreateFont(12, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE,
                          ANSI_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
                          DEFAULT_QUALITY, FF_ROMAN | VARIABLE_PITCH, "Times New Roman");
  mBlackPen = ::CreatePen(PS_SOLID, 0, RGB(0, 0, 0));

  mContext->GetGammaTable(mGammaTable);

  return SetupDC(nsnull, mDC);
}

NS_IMETHODIMP nsRenderingContextWin :: LockDrawingSurface(PRInt32 aX, PRInt32 aY,
                                                          PRUint32 aWidth, PRUint32 aHeight,
                                                          void **aBits, PRInt32 *aStride,
                                                          PRInt32 *aWidthBytes, PRUint32 aFlags)
{
  PRBool  destructive;

  PushState();

  mSurface->IsReleaseDCDestructive(&destructive);

  if (destructive)
  {
    PushClipState();

    if (nsnull != mOrigSolidBrush)
      mCurrBrush = (HBRUSH)::SelectObject(mDC, mOrigSolidBrush);

    if (nsnull != mOrigFont)
      mCurrFont = (HFONT)::SelectObject(mDC, mOrigFont);

    if (nsnull != mOrigSolidPen)
      mCurrPen = (HPEN)::SelectObject(mDC, mOrigSolidPen);

    if (nsnull != mOrigPalette)
      ::SelectPalette(mDC, mOrigPalette, TRUE);
  }

  mSurface->ReleaseDC();

  return mSurface->Lock(aX, aY, aWidth, aHeight, aBits, aStride, aWidthBytes, aFlags);
}

NS_IMETHODIMP nsRenderingContextWin :: UnlockDrawingSurface(void)
{
  PRBool  clipstate;

  mSurface->Unlock();
  mSurface->GetDC(&mDC);

  PopState(clipstate);

  mSurface->IsReleaseDCDestructive(&clipstate);

  if (clipstate)
  {
    ::SetTextColor(mDC, PALETTERGB_COLORREF(mColor));
    mCurrTextColor = mCurrentColor;

    ::SetBkMode(mDC, TRANSPARENT);
    ::SetPolyFillMode(mDC, WINDING);
    ::SetStretchBltMode(mDC, COLORONCOLOR);

    mOrigSolidBrush = (HBRUSH)::SelectObject(mDC, mCurrBrush);
    mOrigFont = (HFONT)::SelectObject(mDC, mCurrFont);
    mOrigSolidPen = (HPEN)::SelectObject(mDC, mCurrPen);

    // If this is a palette device, then select and realize the palette
    nsPaletteInfo palInfo;
    mContext->GetPaletteInfo(palInfo);

    if (palInfo.isPaletteDevice && palInfo.palette)
    {
      PRBool  offscr;
      // Select the palette in the background
      mOrigPalette = ::SelectPalette(mDC, (HPALETTE)palInfo.palette, TRUE);

      mSurface->IsOffscreen(&offscr);

      // Don't do the realization for an off-screen memory DC

      if (PR_FALSE == offscr)
        ::RealizePalette(mDC);
    }

    if (GetDeviceCaps(mDC, RASTERCAPS) & (RC_BITBLT))
      gFastDDASupport = PR_TRUE;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsRenderingContextWin :: SelectOffScreenDrawingSurface(nsDrawingSurface aSurface)
{
  nsresult  rv;

  //XXX this should reset the data in the state stack.

  if (aSurface != mSurface)
  {
    if (nsnull != aSurface)
    {
      HDC tdc;

      //get back a DC
      ((nsDrawingSurfaceWin *)aSurface)->GetDC(&tdc);

      rv = SetupDC(mDC, tdc);

      //kill the DC
      mSurface->ReleaseDC();

      NS_IF_RELEASE(mSurface);
      mSurface = (nsDrawingSurfaceWin *)aSurface;
    }
    else
    {
      if (NULL != mDC)
      {
        rv = SetupDC(mDC, mMainDC);

        //kill the DC
        mSurface->ReleaseDC();

        NS_IF_RELEASE(mSurface);
        mSurface = mMainSurface;
      }
    }

    NS_ADDREF(mSurface);
    mSurface->GetDC(&mDC);
  }
  else
    rv = NS_OK;

  return rv;
}

NS_IMETHODIMP
nsRenderingContextWin :: GetDrawingSurface(nsDrawingSurface *aSurface)
{
  *aSurface = mSurface;
  return NS_OK;
}

NS_IMETHODIMP
nsRenderingContextWin :: GetHints(PRUint32& aResult)
{
  PRUint32 result = 0;

  if (gIsWIN95)
    result |= NS_RENDERING_HINT_FAST_8BIT_TEXT;

  aResult = result;

  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextWin :: Reset()
{
  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextWin :: GetDeviceContext(nsIDeviceContext *&aContext)
{
  NS_IF_ADDREF(mContext);
  aContext = mContext;
  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextWin :: PushState(void)
{
  PRInt32 cnt = mStateCache->Count();

  if (cnt == 0)
  {
    if (nsnull == mStates)
      mStates = new GraphicsState();
    else
      mStates = new GraphicsState(*mStates);
  }
  else
  {
    GraphicsState *state = (GraphicsState *)mStateCache->ElementAt(cnt - 1);
    mStateCache->RemoveElementAt(cnt - 1);

    state->mNext = mStates;

    //clone state info

    state->mMatrix = mStates->mMatrix;
    state->mLocalClip = mStates->mLocalClip;
// we don't want to NULL this out since we reuse the region
// from state to state. if we NULL it, we need to also delete it,
// which means we'll just re-create it when we push the clip state. MMP
//    state->mClipRegion = NULL;
    state->mSolidPen = NULL;
    state->mDashedPen = NULL;
    state->mDottedPen = NULL;
    state->mFlags = ~FLAGS_ALL;
    state->mLineStyle = mStates->mLineStyle;

    mStates = state;
  }

  if (nsnull != mStates->mNext)
  {
    mStates->mNext->mColor = mCurrentColor;
    mStates->mNext->mColorREF = mColor;
    mStates->mNext->mFontMetrics = mFontMetrics;
    NS_IF_ADDREF(mStates->mNext->mFontMetrics);
  }

  mTMatrix = &mStates->mMatrix;

  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextWin :: PopState(PRBool &aClipEmpty)
{
  PRBool  retval = PR_FALSE;

  if (nsnull == mStates)
  {
    NS_ASSERTION(!(nsnull == mStates), "state underflow");
  }
  else
  {
    GraphicsState *oldstate = mStates;

    mStates = mStates->mNext;

    mStateCache->AppendElement(oldstate);

    if (nsnull != mStates)
    {
      mTMatrix = &mStates->mMatrix;

      GraphicsState *pstate;

      if (oldstate->mFlags & FLAG_CLIP_CHANGED)
      {
        pstate = mStates;

        //the clip rect has changed from state to state, so
        //install the previous clip rect

        while ((nsnull != pstate) && !(pstate->mFlags & FLAG_CLIP_VALID))
          pstate = pstate->mNext;

        if (nsnull != pstate)
        {
          int cliptype = ::SelectClipRgn(mDC, pstate->mClipRegion);

          if (cliptype == NULLREGION)
            retval = PR_TRUE;
        }
      }

      oldstate->mFlags &= ~FLAGS_ALL;
      oldstate->mSolidPen = NULL;
      oldstate->mDashedPen = NULL;
      oldstate->mDottedPen = NULL;

      NS_IF_RELEASE(mFontMetrics);
      mFontMetrics = mStates->mFontMetrics;

      mCurrentColor = mStates->mColor;
      mColor = mStates->mColorREF;

      SetLineStyle(mStates->mLineStyle);
    }
    else
      mTMatrix = nsnull;
  }

  aClipEmpty = retval;

  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextWin :: IsVisibleRect(const nsRect& aRect, PRBool &aVisible)
{
  aVisible = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextWin :: SetClipRect(const nsRect& aRect, nsClipCombine aCombine, PRBool &aClipEmpty)
{
  nsRect  trect = aRect;
  int     cliptype;

  mStates->mLocalClip = aRect;

	mTMatrix->TransformCoord(&trect.x, &trect.y,
                           &trect.width, &trect.height);

  RECT nr;
  ConditionRect(trect, nr);

  mStates->mFlags |= FLAG_LOCAL_CLIP_VALID;

  //how we combine the new rect with the previous?

  if (aCombine == nsClipCombine_kIntersect)
  {
    PushClipState();

    cliptype = ::IntersectClipRect(mDC, nr.left,
                                   nr.top,
                                   nr.right,
                                   nr.bottom);
  }
  else if (aCombine == nsClipCombine_kUnion)
  {
    PushClipState();

    HRGN  tregion = ::CreateRectRgn(nr.left,
                                    nr.top,
                                    nr.right,
                                    nr.bottom);

    cliptype = ::ExtSelectClipRgn(mDC, tregion, RGN_OR);
    ::DeleteObject(tregion);
  }
  else if (aCombine == nsClipCombine_kSubtract)
  {
    PushClipState();

    cliptype = ::ExcludeClipRect(mDC, nr.left,
                                 nr.top,
                                 nr.right,
                                 nr.bottom);
  }
  else if (aCombine == nsClipCombine_kReplace)
  {
    PushClipState();

    HRGN  tregion = ::CreateRectRgn(nr.left,
                                    nr.top,
                                    nr.right,
                                    nr.bottom);
    cliptype = ::SelectClipRgn(mDC, tregion);
    ::DeleteObject(tregion);
  }
  else
    NS_ASSERTION(FALSE, "illegal clip combination");

  if (cliptype == NULLREGION)
    aClipEmpty = PR_TRUE;
  else
    aClipEmpty = PR_FALSE;

  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextWin :: GetClipRect(nsRect &aRect, PRBool &aClipValid)
{
  if (mStates->mFlags & FLAG_LOCAL_CLIP_VALID)
  {
    aRect = mStates->mLocalClip;
    aClipValid = PR_TRUE;
  }
  else
    aClipValid = PR_FALSE;

  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextWin :: SetClipRegion(const nsIRegion& aRegion, nsClipCombine aCombine, PRBool &aClipEmpty)
{
  HRGN        hrgn;
  int         cmode, cliptype;

  aRegion.GetNativeRegion((void *&)hrgn);

  switch (aCombine)
  {
    case nsClipCombine_kIntersect:
      cmode = RGN_AND;
      break;

    case nsClipCombine_kUnion:
      cmode = RGN_OR;
      break;

    case nsClipCombine_kSubtract:
      cmode = RGN_DIFF;
      break;

    default:
    case nsClipCombine_kReplace:
      cmode = RGN_COPY;
      break;
  }

  if (NULL != hrgn)
  {
    mStates->mFlags &= ~FLAG_LOCAL_CLIP_VALID;
    PushClipState();
    cliptype = ::ExtSelectClipRgn(mDC, hrgn, cmode);
  }
  else
    return PR_FALSE;

  if (cliptype == NULLREGION)
    aClipEmpty = PR_TRUE;
  else
    aClipEmpty = PR_FALSE;

  return NS_OK;
}

/**
 * Fills in |aRegion| with a copy of the current clip region.
 */
NS_IMETHODIMP nsRenderingContextWin::CopyClipRegion(nsIRegion &aRegion)
{
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP nsRenderingContextWin :: GetClipRegion(nsIRegion **aRegion)
{
  nsresult  rv = NS_OK;

  NS_ASSERTION(!(nsnull == aRegion), "no region ptr");

  if (nsnull == *aRegion)
  {
    nsRegionWin *rgn = new nsRegionWin();

    if (nsnull != rgn)
    {
      NS_ADDREF(rgn);

      rv = rgn->Init();

      if (NS_OK != rv)
        NS_RELEASE(rgn);
      else
        *aRegion = rgn;
    }
    else
      rv = NS_ERROR_OUT_OF_MEMORY;
  }

  if (rv == NS_OK)
  {
    int rstat = ::GetClipRgn(mDC, (*(nsRegionWin **)aRegion)->mRegion);

    if (rstat == 1)
    {
      //i can't find a way to get the actual complexity
      //of the region without actually messing with it, so
      //if the region is non-empty, we'll call it complex. MMP

      (*(nsRegionWin **)aRegion)->mRegionType = eRegionComplexity_complex;
    }
  }

  return rv;
}

NS_IMETHODIMP nsRenderingContextWin :: SetColor(nscolor aColor)
{
  mCurrentColor = aColor;
  mColor = RGB(mGammaTable[NS_GET_R(aColor)],
               mGammaTable[NS_GET_G(aColor)],
               mGammaTable[NS_GET_B(aColor)]);
  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextWin :: GetColor(nscolor &aColor) const
{
  aColor = mCurrentColor;
  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextWin :: SetLineStyle(nsLineStyle aLineStyle)
{
  mCurrLineStyle = aLineStyle;
  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextWin :: GetLineStyle(nsLineStyle &aLineStyle)
{
  aLineStyle = mCurrLineStyle;
  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextWin :: SetFont(const nsFont& aFont)
{
  NS_IF_RELEASE(mFontMetrics);
  mContext->GetMetricsFor(aFont, mFontMetrics);

  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextWin :: SetFont(nsIFontMetrics *aFontMetrics)
{
  NS_IF_RELEASE(mFontMetrics);
  mFontMetrics = aFontMetrics;
  NS_IF_ADDREF(mFontMetrics);

  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextWin :: GetFontMetrics(nsIFontMetrics *&aFontMetrics)
{
  NS_IF_ADDREF(mFontMetrics);
  aFontMetrics = mFontMetrics;

  return NS_OK;
}

// add the passed in translation to the current translation
NS_IMETHODIMP nsRenderingContextWin :: Translate(nscoord aX, nscoord aY)
{
	mTMatrix->AddTranslation((float)aX,(float)aY);
  return NS_OK;
}

// add the passed in scale to the current scale
NS_IMETHODIMP nsRenderingContextWin :: Scale(float aSx, float aSy)
{
	mTMatrix->AddScale(aSx, aSy);
  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextWin :: GetCurrentTransform(nsTransform2D *&aTransform)
{
  aTransform = mTMatrix;
  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextWin :: CreateDrawingSurface(nsRect *aBounds, PRUint32 aSurfFlags, nsDrawingSurface &aSurface)
{
  nsDrawingSurfaceWin *surf = new nsDrawingSurfaceWin();

  if (nsnull != surf)
  {
    NS_ADDREF(surf);

    if (nsnull != aBounds)
      surf->Init(mMainDC, aBounds->width, aBounds->height, aSurfFlags);
    else
      surf->Init(mMainDC, 0, 0, aSurfFlags);
  }

  aSurface = (nsDrawingSurface)surf;

  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextWin :: DestroyDrawingSurface(nsDrawingSurface aDS)
{
  nsDrawingSurfaceWin *surf = (nsDrawingSurfaceWin *)aDS;

  //are we using the surface that we want to kill?
  if (surf == mSurface)
  {
    //remove our local ref to the surface
    NS_IF_RELEASE(mSurface);

    mDC = mMainDC;
    mSurface = mMainSurface;

    //two pointers: two refs
    NS_IF_ADDREF(mSurface);
  }

  //release it...
  NS_IF_RELEASE(surf);

  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextWin :: DrawLine(nscoord aX0, nscoord aY0, nscoord aX1, nscoord aY1)
{
  if (nsLineStyle_kNone == mCurrLineStyle)
    return NS_OK;

	mTMatrix->TransformCoord(&aX0,&aY0);
	mTMatrix->TransformCoord(&aX1,&aY1);

  SetupPen();

  if ((nsLineStyle_kDotted == mCurrLineStyle) && (PR_TRUE == gFastDDASupport))
  {
    lineddastruct dda_struct;

    dda_struct.nDottedPixel = 1;
    dda_struct.dc = mDC;
    dda_struct.crColor = mColor;

    LineDDA((int)(aX0),(int)(aY0),(int)(aX1),(int)(aY1),(LINEDDAPROC) LineDDAFunc,(long)&dda_struct);
  }
  else
  {
    ::MoveToEx(mDC, (int)(aX0), (int)(aY0), NULL);
    ::LineTo(mDC, (int)(aX1), (int)(aY1));
  }

  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextWin :: DrawPolyline(const nsPoint aPoints[], PRInt32 aNumPoints)
{
  if (nsLineStyle_kNone == mCurrLineStyle)
    return NS_OK;

  // First transform nsPoint's into POINT's; perform coordinate space
  // transformation at the same time
  POINT pts[20];
  POINT* pp0 = pts;

  if (aNumPoints > 20)
    pp0 = new POINT[aNumPoints];

  POINT* pp = pp0;
  const nsPoint* np = &aPoints[0];

	for (PRInt32 i = 0; i < aNumPoints; i++, pp++, np++)
  {
		pp->x = np->x;
		pp->y = np->y;
		mTMatrix->TransformCoord((int*)&pp->x,(int*)&pp->y);
	}

  // Draw the polyline
  SetupPen();
  ::Polyline(mDC, pp0, int(aNumPoints));

  // Release temporary storage if necessary
  if (pp0 != pts)
    delete pp0;

  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextWin :: DrawRect(const nsRect& aRect)
{
  RECT nr;
	nsRect	tr;

	tr = aRect;
	mTMatrix->TransformCoord(&tr.x,&tr.y,&tr.width,&tr.height);
	nr.left = tr.x;
	nr.top = tr.y;
	nr.right = tr.x+tr.width;
	nr.bottom = tr.y+tr.height;

  ::FrameRect(mDC, &nr, SetupSolidBrush());

  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextWin :: DrawRect(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight)
{
  RECT nr;

	mTMatrix->TransformCoord(&aX,&aY,&aWidth,&aHeight);
	nr.left = aX;
	nr.top = aY;
	nr.right = aX+aWidth;
	nr.bottom = aY+aHeight;

  ::FrameRect(mDC, &nr, SetupSolidBrush());

  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextWin :: FillRect(const nsRect& aRect)
{
  RECT nr;
	nsRect tr;

	tr = aRect;
	mTMatrix->TransformCoord(&tr.x,&tr.y,&tr.width,&tr.height);
  ConditionRect(tr, nr);
  ::FillRect(mDC, &nr, SetupSolidBrush());

  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextWin :: FillRect(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight)
{
  RECT nr;
	nsRect	tr;

	mTMatrix->TransformCoord(&aX,&aY,&aWidth,&aHeight);
	nr.left = aX;
	nr.top = aY;
	nr.right = aX+aWidth;
	nr.bottom = aY+aHeight;

  ::FillRect(mDC, &nr, SetupSolidBrush());

  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextWin :: InvertRect(const nsRect& aRect)
{
  RECT nr;
	nsRect tr;

	tr = aRect;
	mTMatrix->TransformCoord(&tr.x,&tr.y,&tr.width,&tr.height);
  ConditionRect(tr, nr);
  ::InvertRect(mDC, &nr);

  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextWin :: InvertRect(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight)
{
  RECT nr;
	nsRect	tr;

	mTMatrix->TransformCoord(&aX,&aY,&aWidth,&aHeight);
	nr.left = aX;
	nr.top = aY;
	nr.right = aX+aWidth;
	nr.bottom = aY+aHeight;

  ::InvertRect(mDC, &nr);

  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextWin :: DrawPolygon(const nsPoint aPoints[], PRInt32 aNumPoints)
{
  // First transform nsPoint's into POINT's; perform coordinate space
  // transformation at the same time
  POINT pts[20];
  POINT* pp0 = pts;

  if (aNumPoints > 20)
    pp0 = new POINT[aNumPoints];

  POINT* pp = pp0;
  const nsPoint* np = &aPoints[0];

	for (PRInt32 i = 0; i < aNumPoints; i++, pp++, np++)
  {
		pp->x = np->x;
		pp->y = np->y;
		mTMatrix->TransformCoord((int*)&pp->x,(int*)&pp->y);
	}

  // Outline the polygon - note we are implicitly ignoring the linestyle here
  LOGBRUSH lb;
  lb.lbStyle = BS_NULL;
  lb.lbColor = 0;
  lb.lbHatch = 0;
  SetupSolidPen();
  HBRUSH brush = ::CreateBrushIndirect(&lb);
  HBRUSH oldBrush = (HBRUSH)::SelectObject(mDC, brush);
  ::Polygon(mDC, pp0, int(aNumPoints));
  ::SelectObject(mDC, oldBrush);
  ::DeleteObject(brush);

  // Release temporary storage if necessary
  if (pp0 != pts)
    delete pp0;

  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextWin :: FillPolygon(const nsPoint aPoints[], PRInt32 aNumPoints)
{
  // First transform nsPoint's into POINT's; perform coordinate space
  // transformation at the same time

  POINT pts[20];
  POINT* pp0 = pts;

  if (aNumPoints > 20)
    pp0 = new POINT[aNumPoints];

  POINT* pp = pp0;
  const nsPoint* np = &aPoints[0];

	for (PRInt32 i = 0; i < aNumPoints; i++, pp++, np++)
	{
		pp->x = np->x;
		pp->y = np->y;
		mTMatrix->TransformCoord((int*)&pp->x,(int*)&pp->y);
	}

  // Fill the polygon
  SetupSolidBrush();

  if (NULL == mNullPen)
    mNullPen = ::CreatePen(PS_NULL, 0, 0);

  HPEN oldPen = (HPEN)::SelectObject(mDC, mNullPen);
  ::Polygon(mDC, pp0, int(aNumPoints));
  ::SelectObject(mDC, oldPen);

  // Release temporary storage if necessary
  if (pp0 != pts)
    delete pp0;

  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextWin :: DrawEllipse(const nsRect& aRect)
{
  return DrawEllipse(aRect.x, aRect.y, aRect.width, aRect.height);
}

NS_IMETHODIMP nsRenderingContextWin :: DrawEllipse(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight)
{
  if (nsLineStyle_kNone == mCurrLineStyle)
    return NS_OK;

  mTMatrix->TransformCoord(&aX, &aY, &aWidth, &aHeight);

  SetupPen();

  HBRUSH oldBrush = (HBRUSH)::SelectObject(mDC, ::GetStockObject(NULL_BRUSH));
  
  ::Ellipse(mDC, aX, aY, aX + aWidth, aY + aHeight);
  ::SelectObject(mDC, oldBrush);

  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextWin :: FillEllipse(const nsRect& aRect)
{
  return FillEllipse(aRect.x, aRect.y, aRect.width, aRect.height);
}

NS_IMETHODIMP nsRenderingContextWin :: FillEllipse(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight)
{
  mTMatrix->TransformCoord(&aX, &aY, &aWidth, &aHeight);

  SetupSolidPen();
  SetupSolidBrush();
  
  ::Ellipse(mDC, aX, aY, aX + aWidth, aY + aHeight);

  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextWin :: DrawArc(const nsRect& aRect,
                                 float aStartAngle, float aEndAngle)
{
  return DrawArc(aRect.x,aRect.y,aRect.width,aRect.height,aStartAngle,aEndAngle);
}

NS_IMETHODIMP nsRenderingContextWin :: DrawArc(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight,
                                 float aStartAngle, float aEndAngle)
{
  if (nsLineStyle_kNone == mCurrLineStyle)
    return NS_OK;

  PRInt32 quad1, quad2, sx, sy, ex, ey, cx, cy;
  float   anglerad, distance;

  mTMatrix->TransformCoord(&aX, &aY, &aWidth, &aHeight);

  SetupPen();
  SetupSolidBrush();

  // figure out the the coordinates of the arc from the angle
  distance = (float)sqrt((float)(aWidth * aWidth + aHeight * aHeight));
  cx = aX + aWidth / 2;
  cy = aY + aHeight / 2;

  anglerad = (float)(aStartAngle / (180.0 / 3.14159265358979323846));
  quad1 = (PRInt32)(aStartAngle / 90.0);
  sx = (PRInt32)(distance * cos(anglerad) + cx);
  sy = (PRInt32)(cy - distance * sin(anglerad));

  anglerad = (float)(aEndAngle / (180.0 / 3.14159265358979323846));
  quad2 = (PRInt32)(aEndAngle / 90.0);
  ex = (PRInt32)(distance * cos(anglerad) + cx);
  ey = (PRInt32)(cy - distance * sin(anglerad));

  // this just makes it consitent, on windows 95 arc will always draw CC, nt this sets direction
  ::SetArcDirection(mDC, AD_COUNTERCLOCKWISE);

  ::Arc(mDC, aX, aY, aX + aWidth, aY + aHeight, sx, sy, ex, ey); 

  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextWin :: FillArc(const nsRect& aRect,
                                 float aStartAngle, float aEndAngle)
{
  return FillArc(aRect.x, aRect.y, aRect.width, aRect.height, aStartAngle, aEndAngle);
}

NS_IMETHODIMP nsRenderingContextWin :: FillArc(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight,
                                 float aStartAngle, float aEndAngle)
{
  PRInt32 quad1, quad2, sx, sy, ex, ey, cx, cy;
  float   anglerad, distance;

  mTMatrix->TransformCoord(&aX, &aY, &aWidth, &aHeight);

  SetupSolidPen();
  SetupSolidBrush();

  // figure out the the coordinates of the arc from the angle
  distance = (float)sqrt((float)(aWidth * aWidth + aHeight * aHeight));
  cx = aX + aWidth / 2;
  cy = aY + aHeight / 2;

  anglerad = (float)(aStartAngle / (180.0 / 3.14159265358979323846));
  quad1 = (PRInt32)(aStartAngle / 90.0);
  sx = (PRInt32)(distance * cos(anglerad) + cx);
  sy = (PRInt32)(cy - distance * sin(anglerad));

  anglerad = (float)(aEndAngle / (180.0 / 3.14159265358979323846));
  quad2 = (PRInt32)(aEndAngle / 90.0);
  ex = (PRInt32)(distance * cos(anglerad) + cx);
  ey = (PRInt32)(cy - distance * sin(anglerad));

  // this just makes it consistent, on windows 95 arc will always draw CC,
  // on NT this sets direction
  ::SetArcDirection(mDC, AD_COUNTERCLOCKWISE);

  ::Pie(mDC, aX, aY, aX + aWidth, aY + aHeight, sx, sy, ex, ey); 

  return NS_OK;
}


#ifdef ARABIC_HEBREW_RENDERING

#define CHAR_IS_HEBREW(c) ((0x0590 <= (c)) && ((c)<= 0x05FF))
#define CHAR_IS_ARABIC(c) ((0x0600 <= (c)) && ((c)<= 0x06FF))
#define HAS_ARABIC_PRESENTATION_FORM_B(font) (FONT_HAS_GLYPH((font)->map, 0xFE81))
#define HAS_HEBREW_GLYPH(font)               (FONT_HAS_GLYPH((font)->map, 0x05D0))

static void HebrewReordering(const PRUnichar *aString, PRUint32 aLen,
        PRUnichar* aBuf, PRUint32 &aBufLen)
{
   const PRUnichar* src=aString + aLen - 1;
   PRUnichar* dest= aBuf;
   while(src>=aString)
       *dest++ =  *src--;
   aBufLen = aLen;
}
//============ Begin Arabic Basic to Presentation Form B Code ============

PRUint8 gArabicMap1[] = {
            0x81, 0x83, 0x85, 0x87, 0x89, 0x8D, // 0622-0627
0x8F, 0x93, 0x95, 0x99, 0x9D, 0xA1, 0xA5, 0xA9, // 0628-062F
0xAB, 0xAD, 0xAF, 0xB1, 0xB5, 0xB9, 0xBD, 0xC1, // 0630-0637
0xC5, 0xC9, 0xCD                                // 0638-063A
};

PRUint8 gArabicMap2[] = {
      0xD1, 0xD5, 0xD9, 0xDD, 0xE1, 0xE5, 0xE9, // 0641-0647
0xED, 0xEF, 0xF1                                // 0648-064A
};

#define PresentationFormB(c, form)                           \
  (((0x0622<=(c)) && ((c)<=0x063A)) ?                        \
    (0xFE00|(gArabicMap1[(c)-0x0622] + (form))) :            \
     (((0x0641<=(c)) && ((c)<=0x064A)) ?                     \
      (0xFE00|(gArabicMap2[(c)-0x0641] + (form))) : (c)))

enum {
   eIsolated,  // or Char N
   eFinal,     // or Char R
   eInitial,   // or Char L
   eMedial,    // or Char M
} eArabicForm;
enum {
   eTr = 0, // Transparent
   eRJ = 1, // Right-Joining
   eLJ = 2, // Left-Joining
   eDJ = 3, // Dual-Joining
   eNJ  = 4,// Non-Joining
   eJC = 7, // Joining Causing
   eRightJCMask = 2, // bit of Right-Join Causing 
   eLeftJCMask = 1   // bit of Left-Join Causing 
} eArabicJoiningClass;

#define RightJCClass(j) (eRightJCMask&(j))
#define LeftJCClass(j)  (eLeftJCMask&(j))

#define DecideForm(jl,j,jr)                                 \
  (((eRJ == (j)) && RightJCClass(jr)) ? eFinal              \
                                      :                     \
   ((eDJ == (j)) ?                                          \
    ((RightJCClass(jr)) ?                                   \
     (((LeftJCClass(jl)) ? eMedial                          \
                         : eFinal))                         \
                        :                                   \
     (((LeftJCClass(jl)) ? eInitial                         \
                         : eIsolated))                      \
    )                     : eIsolated))                     \
  

PRInt8 gJoiningClass[] = {
          eRJ, eRJ, eRJ, eRJ, eDJ, eRJ, // 0620-0627
eDJ, eRJ, eDJ, eDJ, eDJ, eDJ, eDJ, eRJ, // 0628-062F
eRJ, eRJ, eRJ, eDJ, eDJ, eDJ, eDJ, eDJ, // 0630-0637
eDJ, eDJ, eDJ, eNJ, eNJ, eNJ, eNJ, eNJ, // 0638-063F
eJC, eDJ, eDJ, eDJ, eDJ, eDJ, eDJ, eDJ, // 0640-0647
eRJ, eRJ, eDJ, eTr, eTr, eTr, eTr, eTr, // 0648-064F
eTr, eTr, eTr                           // 0650-0652
};

#define GetJoiningClass(c)                   \
  (((0x0622 <= (c)) && ((c) <= 0x0652)) ?    \
       (gJoiningClass[(c) - 0x0622]) :       \
      ((0x200D == (c)) ? eJC : eTr))

PRUint16 gArabicLigatureMap[] = 
{
0x82DF, // 0xFE82 0xFEDF -> 0xFEF5
0x82E0, // 0xFE82 0xFEE0 -> 0xFEF6
0x84DF, // 0xFE84 0xFEDF -> 0xFEF7
0x84E0, // 0xFE84 0xFEE0 -> 0xFEF8
0x88DF, // 0xFE88 0xFEDF -> 0xFEF9
0x88E0, // 0xFE88 0xFEE0 -> 0xFEFA
0x8EDF, // 0xFE8E 0xFEDF -> 0xFEFB
0x8EE0  // 0xFE8E 0xFEE0 -> 0xFEFC
};
static void ArabicShaping(const PRUnichar* aString, PRUint32 aLen,
             PRUnichar* aBuf, PRUint32 &aBufLen, PRUint8* map)
{
   const PRUnichar* src = aString+aLen-1;
   const PRUnichar* p;
   PRUnichar* dest = aBuf;
   
   PRUnichar formB;
   PRInt8 leftJ, thisJ, rightJ;
   PRInt8 leftNoTrJ, rightNoTrJ;
   thisJ = eNJ;
   rightJ = GetJoiningClass(*(src)) ;
   while(src>aString) {
      leftJ = thisJ;
      if(eTr != thisJ)
        leftNoTrJ = thisJ;
      thisJ = rightJ;
      rightJ = rightNoTrJ = GetJoiningClass(*(src-1)) ;
      for(p=src-2; (eTr == rightNoTrJ) && (p >= src); p--) 
          rightNoTrJ = GetJoiningClass(*(p)) ;
      formB = PresentationFormB(*src, DecideForm(leftNoTrJ, thisJ, rightNoTrJ));
      if(FONT_HAS_GLYPH(map,formB))
          *dest++ = formB;
      else
          *dest++ = PresentationFormB(*src, eIsolated);
//printf("%x %d %d %d %x\n" ,*src,leftJ, thisJ, rightJ, 
//PresentationFormB(*src, DecideForm(leftJ, thisJ, rightJ)));
      src--;
   }
   if(eTr != thisJ)
     leftNoTrJ = thisJ;
   formB = PresentationFormB(*src, DecideForm(leftNoTrJ, rightJ, eNJ));
   if(FONT_HAS_GLYPH(map,formB))
       *dest++ = formB;
   else
       *dest++ = PresentationFormB(*src, eIsolated);
//printf("%x %d %d %d %x\n" ,*src, thisJ, rightJ, eNJ,
//PresentationFormB(*src, DecideForm( thisJ, rightJ, eNJ)));
   src--;
   PRUnichar *lSrc = aBuf;
   PRUnichar *lDest = aBuf;
   while(lSrc < (dest-1))
   {
      PRUnichar next = *(lSrc+1);
      if(((0xFEDF == next) || (0xFEE0 == next)) && 
         (0xFE80 == (0xFFF1 & *lSrc))) 
      {
         PRBool done = PR_FALSE;
         PRUint16 key = ((*lSrc) << 8) | ( 0x00FF & next);
         PRUint16 i;
         for(i=0;i<8;i++)
         {
             if(key == gArabicLigatureMap[i])
             {
                done = PR_TRUE;
                *lDest++ = 0xFEF5 + i;
                lSrc+=2;
                break;
             }
         }
         if(! done)
             *lDest++ = *lSrc++; 
      } else {
        *lDest++ = *lSrc++; 
      }
   }
   if(lSrc < dest)
      *lDest++ = *lSrc++; 
   aBufLen = lDest - aBuf; 
#if 0
printf("[");
for(PRUint32 k=0;k<aBufLen;k++)
  printf("%x ", aBuf[k]);
printf("]\n");
#endif
}
//============ End of Arabic Basic to Presentation Form B Code ============

static BOOL NeedComplexScriptHandling(const PRUnichar *aString, PRUint32 aLen,
       BOOL bFontSupportHebrew, BOOL* oHebrew,
       BOOL bFontSupportArabic, BOOL* oArabic)
{
  PRUint32 i;
  *oHebrew = *oArabic = FALSE;
  if(bFontSupportArabic && bFontSupportHebrew)
  {
     for(i=0;i<aLen;i++)
     {
       if(CHAR_IS_HEBREW(aString[i])) {
          *oHebrew=TRUE;
          break;
       } else if(CHAR_IS_ARABIC(aString[i])) {
          *oArabic=TRUE;
          break;
       }
     }
  } else if(bFontSupportHebrew) {
     for(i=0;i<aLen;i++)
     {
       if(CHAR_IS_HEBREW(aString[i])) {
          *oHebrew=TRUE;
          break;
       }
     }
  } else if(bFontSupportArabic) {
     for(i=0;i<aLen;i++)
     {
       if(CHAR_IS_ARABIC(aString[i])) {
          *oArabic=TRUE;
          break;
       }
     }
  }
  return *oArabic || *oHebrew;
}
#endif // ARABIC_HEBREW_RENDERING


NS_IMETHODIMP nsRenderingContextWin :: GetWidth(char ch, nscoord& aWidth)
{
  char buf[1];
  buf[0] = ch;
  return GetWidth(buf, 1, aWidth);
}

NS_IMETHODIMP nsRenderingContextWin :: GetWidth(PRUnichar ch, nscoord &aWidth, PRInt32 *aFontID)
{
  PRUnichar buf[1];
  buf[0] = ch;
  return GetWidth(buf, 1, aWidth, aFontID);
}

NS_IMETHODIMP nsRenderingContextWin :: GetWidth(const char* aString, nscoord& aWidth)
{
  return GetWidth(aString, strlen(aString), aWidth);
}

NS_IMETHODIMP nsRenderingContextWin :: GetWidth(const char* aString,
                                                PRUint32 aLength,
                                                nscoord& aWidth)
{

  if (nsnull != mFontMetrics)
  {
      // Check for the very common case of trying to get the width of a single
      // space.
    if ((1 == aLength) && (aString[0] == ' '))
    {
      nsFontMetricsWin* fontMetricsWin = (nsFontMetricsWin*)mFontMetrics;
      return fontMetricsWin->GetSpaceWidth(aWidth);
    }

    SIZE  size;

    SetupFontAndColor();
    ::GetTextExtentPoint32(mDC, aString, aLength, &size);
    aWidth = NSToCoordRound(float(size.cx) * mP2T);

    return NS_OK;
  }
  else
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP nsRenderingContextWin :: GetWidth(const nsString& aString, nscoord& aWidth, PRInt32 *aFontID)
{
  return GetWidth(aString.GetUnicode(), aString.Length(), aWidth, aFontID);
}

NS_IMETHODIMP nsRenderingContextWin :: GetWidth(const PRUnichar *aString,
                                                PRUint32 aLength,
                                                nscoord &aWidth,
                                                PRInt32 *aFontID)
{
  if (nsnull != mFontMetrics)
  {
    nsFontMetricsWin* metrics = (nsFontMetricsWin*) mFontMetrics;
    nsFontWin* prevFont = nsnull;
    SIZE size;

    SetupFontAndColor();
#ifdef ARABIC_HEBREW_RENDERING
    PRUnichar buf[8192];
    PRUint32 len;
#endif  // ARABIC_HEBREW_RENDERING

    LONG width = 0;
    PRUint32 start = 0;
    for (PRUint32 i = 0; i < aLength; i++) {
      PRUnichar c = aString[i];
      nsFontWin* currFont = nsnull;
      nsFontWin** font = metrics->mLoadedFonts;
      nsFontWin** end = &metrics->mLoadedFonts[metrics->mLoadedFontsCount];
      while (font < end) {
        if (FONT_HAS_GLYPH((*font)->map, c)) {
          currFont = *font;
          goto FoundFont; // for speed -- avoid "if" statement
        }
        font++;
      }
      currFont = metrics->FindFont(mDC, c);
FoundFont:
      // XXX avoid this test by duplicating code
      if (prevFont) {
        if (currFont != prevFont) {
          ::SelectObject(mDC, prevFont->font);
#ifdef ARABIC_HEBREW_RENDERING
          BOOL bArabic=FALSE;
          BOOL bHebrew=FALSE;
          if(NeedComplexScriptHandling(&aString[start],i-start,
                HAS_HEBREW_GLYPH(prevFont), &bHebrew,
                HAS_ARABIC_PRESENTATION_FORM_B(prevFont), &bArabic ) )
          {
             len = 8192;
             if(bHebrew) {
                HebrewReordering(&aString[start], i-start, buf, len);
             } else if (bArabic) {
                ArabicShaping(&aString[start], i-start, buf, len, prevFont->map);
            }
            ::GetTextExtentPoint32W(mDC, buf, len, &size);
          } 
          else 
#endif // ARABIC_HEBREW_RENDERING
          {
            ::GetTextExtentPoint32W(mDC, &aString[start], i - start, &size);
          }
          width += size.cx;
          prevFont = currFont;
          start = i;
        }
      }
      else {
        prevFont = currFont;
        start = i;
      }
    }

    if (prevFont) {
      ::SelectObject(mDC, prevFont->font);
#ifdef ARABIC_HEBREW_RENDERING
      BOOL bArabic=FALSE;
      BOOL bHebrew=FALSE;
      if(NeedComplexScriptHandling(&aString[start],i-start,
                HAS_HEBREW_GLYPH(prevFont), &bHebrew,
            HAS_ARABIC_PRESENTATION_FORM_B(prevFont), &bArabic ) )
      {
         len = 8192;
         if(bHebrew) {
            HebrewReordering(&aString[start], i-start, buf, len);
         } else if (bArabic) {
            ArabicShaping(&aString[start], i-start, buf, len, prevFont->map);
        }
        ::GetTextExtentPoint32W(mDC, buf, len, &size);
      } 
      else 
#endif // ARABIC_HEBREW_RENDERING
      {
         ::GetTextExtentPoint32W(mDC, &aString[start], i - start, &size);
      }
      width += size.cx;
    }

    aWidth = NSToCoordRound(float(width) * mP2T);

    ::SelectObject(mDC, mCurrFont);

    if (nsnull != aFontID)
      *aFontID = 0;

    return NS_OK;
  }
  else
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP nsRenderingContextWin :: DrawString(const char *aString, PRUint32 aLength,
                                                  nscoord aX, nscoord aY,
                                                  const nscoord* aSpacing)
{
	PRInt32	x = aX;
  PRInt32 y = aY;

  SetupFontAndColor();

  INT dxMem[500];
  INT* dx0;
  if (nsnull != aSpacing) {
    dx0 = dxMem;
    if (aLength > 500) {
      dx0 = new INT[aLength];
    }
    mTMatrix->ScaleXCoords(aSpacing, aLength, dx0);
  }

	mTMatrix->TransformCoord(&x, &y);
  ::ExtTextOut(mDC, x, y, 0, NULL, aString, aLength, aSpacing ? dx0 : NULL);

  if ((nsnull != aSpacing) && (dx0 != dxMem)) {
    delete [] dx0;
  }

  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextWin :: DrawString(const PRUnichar *aString, PRUint32 aLength,
                                                  nscoord aX, nscoord aY,
                                                  PRInt32 aFontID,
                                                  const nscoord* aSpacing)
{
  if (nsnull != mFontMetrics)
  {
    PRInt32 x = aX;
    PRInt32 y = aY;
    mTMatrix->TransformCoord(&x, &y);
    nsFontMetricsWin* metrics = (nsFontMetricsWin*) mFontMetrics;
    nsFontWin* prevFont = nsnull;
    SIZE size;

    SetupFontAndColor();
#ifdef ARABIC_HEBREW_RENDERING
    PRUnichar buf[8192];
    PRUint32 len;
#endif  // ARABIC_HEBREW_RENDERING

    PRUint32 start = 0;
    for (PRUint32 i = 0; i < aLength; i++) {
      PRUnichar c = aString[i];
      nsFontWin* currFont = nsnull;
      nsFontWin** font = metrics->mLoadedFonts;
      nsFontWin** end = &metrics->mLoadedFonts[metrics->mLoadedFontsCount];
      while (font < end) {
        if (FONT_HAS_GLYPH((*font)->map, c)) {
          currFont = *font;
          goto FoundFont; // for speed -- avoid "if" statement
        }
        font++;
      }
      currFont = metrics->FindFont(mDC, c);
FoundFont:
      if (prevFont) {
        if (currFont != prevFont) {
          ::SelectObject(mDC, prevFont->font);
          if (aSpacing) {
            // XXX Fix path to use a twips transform in the DC and use the
            // spacing values directly and let windows deal with the sub-pixel
            // positioning.

            // Slow, but accurate rendering
            const PRUnichar* str = &aString[start];
            const PRUnichar* end = &aString[i];
            while (str < end) {
              // XXX can shave some cycles by inlining a version of transform
              // coord where y is constant and transformed once
              x = aX;
              y = aY;
              mTMatrix->TransformCoord(&x, &y);
              ::ExtTextOutW(mDC, x, y, 0, NULL, str, 1, NULL);
              aX += *aSpacing++;
              str++;
            }
          }
          else {
#ifdef ARABIC_HEBREW_RENDERING
            BOOL bArabic=FALSE;
            BOOL bHebrew=FALSE;
            if(NeedComplexScriptHandling(&aString[start],i-start,
                HAS_HEBREW_GLYPH(prevFont), &bHebrew,
                HAS_ARABIC_PRESENTATION_FORM_B(prevFont), &bArabic ) )
            {
               len = 8192;
               if(bHebrew) {
                  HebrewReordering(&aString[start], i-start, buf, len);
               } else if (bArabic) {
                  ArabicShaping(&aString[start], i-start, buf, len, prevFont->map);
              }
              ::ExtTextOutW(mDC, x, y, 0, NULL, buf, len, NULL);
              ::GetTextExtentPoint32W(mDC, buf, len, &size);
            } 
            else 
#endif // ARABIC_HEBREW_RENDERING
            {
               ::ExtTextOutW(mDC, x, y, 0, NULL, &aString[start], i - start, NULL);
               ::GetTextExtentPoint32W(mDC, &aString[start], i - start, &size);
            }
            x += size.cx;
          }
          prevFont = currFont;
          start = i;
        }
      }
      else {
        prevFont = currFont;
        start = i;
      }
    }

    if (prevFont) {
      ::SelectObject(mDC, prevFont->font);
      if (aSpacing) {
        // XXX Fix path to use a twips transform in the DC and use the
        // spacing values directly and let windows deal with the sub-pixel
        // positioning.

        // Slow, but accurate rendering
        const PRUnichar* str = &aString[start];
        const PRUnichar* end = &aString[i];
        while (str < end) {
          // XXX can shave some cycles by inlining a version of transform
          // coord where y is constant and transformed once
          x = aX;
          y = aY;
          mTMatrix->TransformCoord(&x, &y);
          ::ExtTextOutW(mDC, x, y, 0, NULL, str, 1, NULL);
          aX += *aSpacing++;
          str++;
        }
      }
      else {
#ifdef ARABIC_HEBREW_RENDERING
        BOOL bArabic=FALSE;
        BOOL bHebrew=FALSE;
        if(NeedComplexScriptHandling(&aString[start],i-start,
            HAS_HEBREW_GLYPH(prevFont), &bHebrew,
            HAS_ARABIC_PRESENTATION_FORM_B(prevFont), &bArabic ) )
        {
            len = 8192;
            if(bHebrew) {
               HebrewReordering(&aString[start], i-start, buf, len);
            } else if (bArabic) {
               ArabicShaping(&aString[start], i-start, buf, len, prevFont->map);
            }
            ::ExtTextOutW(mDC, x, y, 0, NULL, buf, len, NULL);
        } 
        else 
#endif // ARABIC_HEBREW_RENDERING
        {
           ::ExtTextOutW(mDC, x, y, 0, NULL, &aString[start], i - start, NULL);
        }
      }
    }

    ::SelectObject(mDC, mCurrFont);

    return NS_OK;
  }
  else
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP nsRenderingContextWin :: DrawString(const nsString& aString,
                                                  nscoord aX, nscoord aY,
                                                  PRInt32 aFontID,
                                                  const nscoord* aSpacing)
{
  return DrawString(aString.GetUnicode(), aString.Length(), aX, aY, aFontID, aSpacing);
}

NS_IMETHODIMP nsRenderingContextWin :: DrawImage(nsIImage *aImage, nscoord aX, nscoord aY)
{
  NS_PRECONDITION(PR_TRUE == mInitialized, "!initialized");

  nscoord width, height;

  width = NSToCoordRound(mP2T * aImage->GetWidth());
  height = NSToCoordRound(mP2T * aImage->GetHeight());

  return DrawImage(aImage, aX, aY, width, height);
}

NS_IMETHODIMP nsRenderingContextWin :: DrawImage(nsIImage *aImage, nscoord aX, nscoord aY,
                                        nscoord aWidth, nscoord aHeight) 
{
  nsRect  tr;

  tr.x = aX;
  tr.y = aY;
  tr.width = aWidth;
  tr.height = aHeight;

  return DrawImage(aImage, tr);
}

NS_IMETHODIMP nsRenderingContextWin :: DrawImage(nsIImage *aImage, const nsRect& aSRect, const nsRect& aDRect)
{
  nsRect	sr,dr;

	sr = aSRect;
	mTMatrix->TransformCoord(&sr.x, &sr.y, &sr.width, &sr.height);

  dr = aDRect;
	mTMatrix->TransformCoord(&dr.x, &dr.y, &dr.width, &dr.height);

  return aImage->Draw(*this, mSurface, sr.x, sr.y, sr.width, sr.height, dr.x, dr.y, dr.width, dr.height);
}

NS_IMETHODIMP nsRenderingContextWin :: DrawImage(nsIImage *aImage, const nsRect& aRect)
{
  nsRect	tr;

	tr = aRect;
	mTMatrix->TransformCoord(&tr.x, &tr.y, &tr.width, &tr.height);

  return aImage->Draw(*this, mSurface, tr.x, tr.y, tr.width, tr.height);
}

NS_IMETHODIMP nsRenderingContextWin :: CopyOffScreenBits(nsDrawingSurface aSrcSurf,
                                                         PRInt32 aSrcX, PRInt32 aSrcY,
                                                         const nsRect &aDestBounds,
                                                         PRUint32 aCopyFlags)
{

  if ((nsnull != aSrcSurf) && (nsnull != mMainDC))
  {
    PRInt32 x = aSrcX;
    PRInt32 y = aSrcY;
    nsRect  drect = aDestBounds;
    HDC     destdc, srcdc;

    //get back a DC

    ((nsDrawingSurfaceWin *)aSrcSurf)->GetDC(&srcdc);

    if (nsnull != srcdc)
    {
      if (aCopyFlags & NS_COPYBITS_TO_BACK_BUFFER)
      {
        NS_ASSERTION(!(nsnull == mDC), "no back buffer");
        destdc = mDC;
      }
      else
        destdc = mMainDC;

      if (aCopyFlags & NS_COPYBITS_USE_SOURCE_CLIP_REGION)
      {
        HRGN  tregion = ::CreateRectRgn(0, 0, 0, 0);

        if (::GetClipRgn(srcdc, tregion) == 1)
          ::SelectClipRgn(destdc, tregion);

        ::DeleteObject(tregion);
      }

      // If there's a palette make sure it's selected.
      // XXX This doesn't seem like the best place to be doing this...

      nsPaletteInfo palInfo;
      HPALETTE      oldPalette;

      mContext->GetPaletteInfo(palInfo);

      if (palInfo.isPaletteDevice && palInfo.palette)
        oldPalette = ::SelectPalette(destdc, (HPALETTE)palInfo.palette, TRUE);

      if (aCopyFlags & NS_COPYBITS_XFORM_SOURCE_VALUES)
        mTMatrix->TransformCoord(&x, &y);

      if (aCopyFlags & NS_COPYBITS_XFORM_DEST_VALUES)
        mTMatrix->TransformCoord(&drect.x, &drect.y, &drect.width, &drect.height);

      ::BitBlt(destdc, drect.x, drect.y,
               drect.width, drect.height,
               srcdc, x, y, SRCCOPY);

      if (palInfo.isPaletteDevice && palInfo.palette)
        ::SelectPalette(destdc, oldPalette, TRUE);

      //kill the DC
      ((nsDrawingSurfaceWin *)aSrcSurf)->ReleaseDC();
    }
    else
      NS_ASSERTION(0, "attempt to blit with bad DCs");
  }
  else
    NS_ASSERTION(0, "attempt to blit with bad DCs");

  return NS_OK;
}

//~~~
NS_IMETHODIMP nsRenderingContextWin::RetrieveCurrentNativeGraphicData(PRUint32 * ngd)
{
  if(ngd != nsnull)
    *ngd = (PRUint32)mDC;
  return NS_OK;
}

#ifdef NS_DEBUG
//these are used with the routines below
//to see how our state caching is working... MMP
static numpen = 0;
static numbrush = 0;
static numfont = 0;
#endif

HBRUSH nsRenderingContextWin :: SetupSolidBrush(void)
{
  if ((mCurrentColor != mCurrBrushColor) || (NULL == mCurrBrush))
  {
    HBRUSH tbrush = ::CreateSolidBrush(PALETTERGB_COLORREF(mColor));

    ::SelectObject(mDC, tbrush);

    if (NULL != mCurrBrush)
      VERIFY(::DeleteObject(mCurrBrush));

    mCurrBrush = tbrush;
    mCurrBrushColor = mCurrentColor;
//printf("brushes: %d\n", ++numbrush);
  }

  return mCurrBrush;
}

void nsRenderingContextWin :: SetupFontAndColor(void)
{
  if (((mFontMetrics != mCurrFontMetrics) || (NULL == mCurrFontMetrics)) &&
      (nsnull != mFontMetrics))
  {
    nsFontHandle  fontHandle;
    mFontMetrics->GetFontHandle(fontHandle);
    HFONT         tfont = (HFONT)fontHandle;
    
    ::SelectObject(mDC, tfont);

    mCurrFont = tfont;
    mCurrFontMetrics = mFontMetrics;
//printf("fonts: %d\n", ++numfont);
  }

  if (mCurrentColor != mCurrTextColor)
  {
    ::SetTextColor(mDC, PALETTERGB_COLORREF(mColor));
    mCurrTextColor = mCurrentColor;
  }
}

HPEN nsRenderingContextWin :: SetupPen()
{
  HPEN pen;

  switch(mCurrLineStyle)
  {
    case nsLineStyle_kSolid:
      pen = SetupSolidPen();
      break;

    case nsLineStyle_kDashed:
      pen = SetupDashedPen();
      break;

    case nsLineStyle_kDotted:
      pen = SetupDottedPen();
      break;

    case nsLineStyle_kNone:
      pen = NULL;
      break;

    default:
      pen = SetupSolidPen();
      break;
  }

  return pen;
}


HPEN nsRenderingContextWin :: SetupSolidPen(void)
{
  if ((mCurrentColor != mCurrPenColor) || (NULL == mCurrPen) || (mCurrPen != mStates->mSolidPen))
  {
    HPEN  tpen = ::CreatePen(PS_SOLID, 0, PALETTERGB_COLORREF(mColor));

    ::SelectObject(mDC, tpen);

    if (NULL != mCurrPen)
      VERIFY(::DeleteObject(mCurrPen));

    mStates->mSolidPen = mCurrPen = tpen;
    mCurrPenColor = mCurrentColor;
//printf("pens: %d\n", ++numpen);
  }

  return mCurrPen;
}

HPEN nsRenderingContextWin :: SetupDashedPen(void)
{
  if ((mCurrentColor != mCurrPenColor) || (NULL == mCurrPen) || (mCurrPen != mStates->mDashedPen))
  {
    HPEN  tpen = ::CreatePen(PS_DASH, 0, PALETTERGB_COLORREF(mColor));

    ::SelectObject(mDC, tpen);

    if (NULL != mCurrPen)
      VERIFY(::DeleteObject(mCurrPen));

    mStates->mDashedPen = mCurrPen = tpen;
    mCurrPenColor = mCurrentColor;
//printf("pens: %d\n", ++numpen);
  }

  return mCurrPen;
}

HPEN nsRenderingContextWin :: SetupDottedPen(void)
{
  if ((mCurrentColor != mCurrPenColor) || (NULL == mCurrPen) || (mCurrPen != mStates->mDottedPen))
  {
    HPEN  tpen = ::CreatePen(PS_DOT, 0, PALETTERGB_COLORREF(mColor));

    ::SelectObject(mDC, tpen);

    if (NULL != mCurrPen)
      VERIFY(::DeleteObject(mCurrPen));

    mStates->mDottedPen = mCurrPen = tpen;
    mCurrPenColor = mCurrentColor;
//printf("pens: %d\n", ++numpen);
  }

  return mCurrPen;
}

void nsRenderingContextWin :: PushClipState(void)
{
  if (!(mStates->mFlags & FLAG_CLIP_CHANGED))
  {
    GraphicsState *tstate = mStates->mNext;

    //we have never set a clip on this state before, so
    //remember the current clip state in the next state on the
    //stack. kind of wacky, but avoids selecting stuff in the DC
    //all the damned time.

    if (nsnull != tstate)
    {
      if (NULL == tstate->mClipRegion)
        tstate->mClipRegion = ::CreateRectRgn(0, 0, 0, 0);

      if (::GetClipRgn(mDC, tstate->mClipRegion) == 1)
        tstate->mFlags |= FLAG_CLIP_VALID;
      else
        tstate->mFlags &= ~FLAG_CLIP_VALID;
    }
  
    mStates->mFlags |= FLAG_CLIP_CHANGED;
  }
}

NS_IMETHODIMP nsRenderingContextWin :: CreateDrawingSurface(HDC aDC, nsDrawingSurface &aSurface)
{
  nsDrawingSurfaceWin *surf = new nsDrawingSurfaceWin();

  if (nsnull != surf)
  {
    NS_ADDREF(surf);
    surf->Init(aDC);
  }

  aSurface = (nsDrawingSurface)surf;

  return NS_OK;
}


// The following is a workaround for a Japanese Windows 95 problem.

NS_IMETHODIMP nsRenderingContextWinA :: GetWidth(const PRUnichar *aString,
                                                PRUint32 aLength,
                                                nscoord &aWidth,
                                                PRInt32 *aFontID)
{
  if (nsnull != mFontMetrics)
  {
    nsFontMetricsWinA* metrics = (nsFontMetricsWinA*) mFontMetrics;
    nsFontSubset* prevFont = nsnull;
    SIZE size;

    SetupFontAndColor();

    LONG width = 0;
    PRUint32 start = 0;
    for (PRUint32 i = 0; i < aLength; i++) {
      PRUnichar c = aString[i];
      nsFontSubset* currFont = nsnull;
      nsFontWinA** font = (nsFontWinA**) metrics->mLoadedFonts;
      nsFontWinA** end = (nsFontWinA**) &metrics->mLoadedFonts[metrics->mLoadedFontsCount];
      while (font < end) {
        if (FONT_HAS_GLYPH((*font)->mMap, c)) {
          nsFontSubset* subset = (*font)->mSubsets;
          nsFontSubset* endSubsets = &((*font)->mSubsets[(*font)->mSubsetsCount]);
          while (subset < endSubsets) {
            if (!subset->mMap) {
              if (!subset->Load(*font)) {
                subset++;
                continue;
              }
            }
            if (FONT_HAS_GLYPH(subset->mMap, c)) {
              currFont = subset;
              goto FoundFont; // for speed -- avoid "if" statement
            }
            subset++;
          }
        }
        font++;
      }
      currFont = (nsFontSubset*) metrics->FindFont(mDC, c);
FoundFont:
      // XXX avoid this test by duplicating code
      if (prevFont) {
        if (currFont != prevFont) {
          ::SelectObject(mDC, prevFont->mFont);
          char str[1024];
          int len = WideCharToMultiByte(prevFont->mCodePage, 0, &aString[start],
            i - start, str, sizeof(str), nsnull, nsnull);
          if (len) {
            ::GetTextExtentPoint32A(mDC, str, len, &size);
            width += size.cx;
          }
          else {
            // XXX failed
            printf("%d: WideCharToMultiByte failed\n", prevFont->mCodePage);
          }
          prevFont = currFont;
          start = i;
        }
      }
      else {
        prevFont = currFont;
        start = i;
      }
    }

    if (prevFont) {
      ::SelectObject(mDC, prevFont->mFont);
      char str[1024];
      int len = WideCharToMultiByte(prevFont->mCodePage, 0, &aString[start],
        i - start, str, sizeof(str), nsnull, nsnull);
      if (len) {
        ::GetTextExtentPoint32A(mDC, str, len, &size);
        width += size.cx;
      }
      else {
        // XXX failed
        printf("%d: WideCharToMultiByte failed\n", prevFont->mCodePage);
      }
    }

    aWidth = NSToCoordRound(float(width) * mP2T);

    ::SelectObject(mDC, mCurrFont);

    if (nsnull != aFontID)
      *aFontID = 0;

    return NS_OK;
  }
  else
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP nsRenderingContextWinA :: DrawString(const PRUnichar *aString, PRUint32 aLength,
                                                  nscoord aX, nscoord aY,
                                                  PRInt32 aFontID,
                                                  const nscoord* aSpacing)
{
  if (nsnull != mFontMetrics)
  {
    PRInt32 x = aX;
    PRInt32 y = aY;
    mTMatrix->TransformCoord(&x, &y);
    nsFontMetricsWinA* metrics = (nsFontMetricsWinA*) mFontMetrics;
    nsFontSubset* prevFont = nsnull;
    SIZE size;

    SetupFontAndColor();

    PRUint32 start = 0;
    for (PRUint32 i = 0; i < aLength; i++) {
      PRUnichar c = aString[i];
      nsFontSubset* currFont = nsnull;
      nsFontWinA** font = (nsFontWinA**) metrics->mLoadedFonts;
      nsFontWinA** end = (nsFontWinA**) &metrics->mLoadedFonts[metrics->mLoadedFontsCount];
      while (font < end) {
        if (FONT_HAS_GLYPH((*font)->mMap, c)) {
          nsFontSubset* subset = (*font)->mSubsets;
          nsFontSubset* endSubsets = &((*font)->mSubsets[(*font)->mSubsetsCount]);
          while (subset < endSubsets) {
            if (!subset->mMap) {
              if (!subset->Load(*font)) {
                subset++;
                continue;
              }
            }
            if (FONT_HAS_GLYPH(subset->mMap, c)) {
              currFont = subset;
              goto FoundFont; // for speed -- avoid "if" statement
            }
            subset++;
          }
        }
        font++;
      }
      currFont = (nsFontSubset*) metrics->FindFont(mDC, c);
FoundFont:
      if (prevFont) {
        if (currFont != prevFont) {
          ::SelectObject(mDC, prevFont->mFont);
          if (aSpacing) {
            // XXX Fix path to use a twips transform in the DC and use the
            // spacing values directly and let windows deal with the sub-pixel
            // positioning.

            // Slow, but accurate rendering
            const PRUnichar* str = &aString[start];
            const PRUnichar* end = &aString[i];
            while (str < end) {
              // XXX can shave some cycles by inlining a version of transform
              // coord where y is constant and transformed once
              x = aX;
              y = aY;
              mTMatrix->TransformCoord(&x, &y);
              char mb[1024];
              int len = WideCharToMultiByte(prevFont->mCodePage, 0, str, 1, mb,
                sizeof(mb), nsnull, nsnull);
              if (len) {
                ::ExtTextOutA(mDC, x, y, 0, NULL, mb, len, NULL);
              }
              else {
                // XXX failed
                printf("%d: WideCharToMultiByte failed\n", prevFont->mCodePage);
              }
              aX += *aSpacing++;
              str++;
            }
          }
          else {
            char mb[1024];
            int len = WideCharToMultiByte(prevFont->mCodePage, 0,
              &aString[start], i - start, mb, sizeof(mb), nsnull, nsnull);
            if (len) {
              ::ExtTextOutA(mDC, x, y, 0, NULL, mb, len, NULL);
              ::GetTextExtentPoint32A(mDC, mb, len, &size);
              x += size.cx;
            }
            else {
              // XXX failed
              printf("%d: WideCharToMultiByte failed\n", prevFont->mCodePage);
            }
          }
          prevFont = currFont;
          start = i;
        }
      }
      else {
        prevFont = currFont;
        start = i;
      }
    }

    if (prevFont) {
      ::SelectObject(mDC, prevFont->mFont);
      if (aSpacing) {
        // XXX Fix path to use a twips transform in the DC and use the
        // spacing values directly and let windows deal with the sub-pixel
        // positioning.

        // Slow, but accurate rendering
        const PRUnichar* str = &aString[start];
        const PRUnichar* end = &aString[i];
        while (str < end) {
          // XXX can shave some cycles by inlining a version of transform
          // coord where y is constant and transformed once
          x = aX;
          y = aY;
          mTMatrix->TransformCoord(&x, &y);
          char mb[1024];
          int len = WideCharToMultiByte(prevFont->mCodePage, 0, str, 1, mb,
            sizeof(mb), nsnull, nsnull);
          if (len) {
            ::ExtTextOutA(mDC, x, y, 0, NULL, mb, len, NULL);
          }
          else {
            // XXX failed
            printf("%d: WideCharToMultiByte failed\n", prevFont->mCodePage);
          }
          aX += *aSpacing++;
          str++;
        }
      }
      else {
        char mb[1024];
        int len = WideCharToMultiByte(prevFont->mCodePage, 0,
          &aString[start], i - start, mb, sizeof(mb), nsnull, nsnull);
        if (len) {
          ::ExtTextOutA(mDC, x, y, 0, NULL, mb, len, NULL);
        }
        else {
          // XXX failed
          printf("%d: WideCharToMultiByte failed\n", prevFont->mCodePage);
        }
      }
    }

    ::SelectObject(mDC, mCurrFont);

    return NS_OK;
  }
  else
    return NS_ERROR_FAILURE;
}


// ConditionRect is used to fix a coordinate overflow problem under WIN95. 
// Convert nsRect to RECT with cooordinates modified to acceptable ranges for WIN95.
// XXX: TODO find all calls under WIN95 that will fail if passed large coordinate values
// and make calls to this method to fix them.

void 
nsRenderingContextWin::ConditionRect(nsRect aSrcRect, RECT& aDestRect)
{
  aDestRect.left = aSrcRect.x;
  aDestRect.right = aSrcRect.x + aSrcRect.width; 

   // XXX: TODO find the exact values for the and bottom limits. These limits were determined by
   // printing out the RECT coordinates and noticing when they failed. There must be an offical
   // document that describes what the coordinate limits are for calls
   // such as ::FillRect and ::IntersectClipRect under WIN95 which fail when large negative and
   // position values are passed.

   // The following is for WIN95. If range of legal values for the rectangles passed for 
   // clipping and drawing is smaller on WIN95 than under WINNT.                              
  const nscoord kBottomLimit = 16384;
  const nscoord kTopLimit = -8192;

  if (aSrcRect.y < kTopLimit)
    aDestRect.top = kTopLimit;
  else 
    aDestRect.top = aSrcRect.y;

  if ((aSrcRect.y + aSrcRect.height) > kBottomLimit) 
     aDestRect.bottom = kBottomLimit;
  else
     aDestRect.bottom = aSrcRect.y + aSrcRect.height;

}



