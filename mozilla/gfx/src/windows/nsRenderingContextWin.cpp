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
#include "nsRegionWin.h"
#include <math.h>
#include "libimg.h"
#include "nsDeviceContextWin.h"

#ifdef NGLAYOUT_DDRAW
#include "ddraw.h"
#endif

#define FLAG_CLIP_VALID       0x0001
#define FLAG_CLIP_CHANGED     0x0002
#define FLAG_LOCAL_CLIP_VALID 0x0004

#define FLAGS_ALL             (FLAG_CLIP_VALID | FLAG_CLIP_CHANGED | FLAG_LOCAL_CLIP_VALID)

// Macro for creating a palette relative color if you have a COLORREF instead
// of the reg, green, and blue values. The color is color-matches to the nearest
// in the current logical palette. This has no effect on a non-palette device
#define PALETTERGB_COLORREF(c)  (0x02000000 | (c))

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
  nscolor         mBrushColor;
  HBRUSH          mSolidBrush;
  nsIFontMetrics  *mFontMetrics;
  HFONT           mFont;
  nscolor         mPenColor;
  HPEN            mSolidPen;
  PRInt32         mFlags;
  nscolor         mTextColor;
};

GraphicsState :: GraphicsState()
{
  mNext = nsnull;
  mMatrix.SetToIdentity();  
  mLocalClip.x = mLocalClip.y = mLocalClip.width = mLocalClip.height = 0;
  mClipRegion = NULL;
  mBrushColor = NS_RGB(0, 0, 0);
  mSolidBrush = NULL;
  mFontMetrics = nsnull;
  mFont = NULL;
  mPenColor = NS_RGB(0, 0, 0);
  mSolidPen = NULL;
  mFlags = ~FLAGS_ALL;
  mTextColor = RGB(0, 0, 0);
}

GraphicsState :: GraphicsState(GraphicsState &aState) :
                               mMatrix(&aState.mMatrix),
                               mLocalClip(aState.mLocalClip)
{
  mNext = &aState;
  mClipRegion = NULL;
  mBrushColor = aState.mBrushColor;
  mSolidBrush = NULL;
  mFontMetrics = nsnull;
  mFont = NULL;
  mPenColor = aState.mPenColor;
  mSolidPen = NULL;
  mFlags = ~FLAGS_ALL;
  mTextColor = aState.mTextColor;
}

GraphicsState :: ~GraphicsState()
{
  if (NULL != mClipRegion)
  {
    ::DeleteObject(mClipRegion);
    mClipRegion = NULL;
  }

  if (NULL != mSolidBrush)
  {
    ::DeleteObject(mSolidBrush);
    mSolidBrush = NULL;
  }

  //don't delete this because it lives in the font metrics
  mFont = NULL;

  if (NULL != mSolidPen)
  {
    ::DeleteObject(mSolidPen);
    mSolidPen = NULL;
  }
}

nsDrawingSurfaceWin :: nsDrawingSurfaceWin()
{
  NS_INIT_REFCNT();

  mDC = NULL;
  mDCOwner = nsnull;
  mOrigBitmap = nsnull;

#ifdef NGLAYOUT_DDRAW
  mSurface = NULL;
#endif
}

nsDrawingSurfaceWin :: ~nsDrawingSurfaceWin()
{
  if ((nsnull != mDC) && (nsnull != mOrigBitmap))
  {
    HBITMAP bits = (HBITMAP)::SelectObject(mDC, mOrigBitmap);

    if (nsnull != bits)
      ::DeleteObject(bits);

    mOrigBitmap = nsnull;
  }

#ifdef NGLAYOUT_DDRAW
  if (NULL != mSurface)
  {
    if (NULL != mDC)
    {
      mSurface->ReleaseDC(mDC);
      mDC = NULL;
    }

    NS_RELEASE(mSurface);
    mSurface = NULL;
  }
  else
#endif
  {
    if (NULL != mDC)
    {
      if (nsnull != mDCOwner)
      {
        ::ReleaseDC((HWND)mDCOwner->GetNativeData(NS_NATIVE_WINDOW), mDC);
        NS_RELEASE(mDCOwner);
      }
      else
        ::DeleteDC(mDC);

      mDC = NULL;
    }
  }
}

//this isn't really a com object, so don't allow anyone to get anything

NS_IMETHODIMP nsDrawingSurfaceWin :: QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
  return NS_NOINTERFACE;
}

NS_IMPL_ADDREF(nsDrawingSurfaceWin)
NS_IMPL_RELEASE(nsDrawingSurfaceWin)

nsresult nsDrawingSurfaceWin :: Init(HDC aDC)
{
  mDC = aDC;

  if (nsnull != mDC)
    return NS_OK;
  else
    return NS_ERROR_FAILURE;
}

nsresult nsDrawingSurfaceWin :: Init(nsIWidget *aOwner)
{
  mDCOwner = aOwner;

  if (nsnull != mDCOwner)
  {
    NS_ADDREF(mDCOwner);
    mDC = (HDC)mDCOwner->GetNativeData(NS_NATIVE_GRAPHIC);

    return NS_OK;
  }

  return NS_ERROR_FAILURE;
}

#ifdef NGLAYOUT_DDRAW

nsresult nsDrawingSurfaceWin :: Init(LPDIRECTDRAWSURFACE aSurface)
{
  mSurface = aSurface;

  if (nsnull != aSurface)
  {
    NS_ADDREF(mSurface);
    mSurface->GetDC(&mDC);

    return NS_OK;
  }

  return NS_ERROR_FAILURE;
}

#endif

static NS_DEFINE_IID(kRenderingContextIID, NS_IRENDERING_CONTEXT_IID);

#ifdef NGLAYOUT_DDRAW
IDirectDraw *nsRenderingContextWin::mDDraw = NULL;
IDirectDraw2 *nsRenderingContextWin::mDDraw2 = NULL;
nsresult nsRenderingContextWin::mDDrawResult = NS_OK;
#endif

#define NOT_SETUP 0x33
static PRBool gIsWIN95 = NOT_SETUP;

nsRenderingContextWin :: nsRenderingContextWin()
{
  NS_INIT_REFCNT();

  // The first time in we initialize gIsWIN95 flag
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

#ifdef NGLAYOUT_DDRAW
  CreateDDraw();
#endif

  mDC = NULL;
  mMainDC = NULL;
  mDCOwner = nsnull;
  mFontMetrics = nsnull;
  mFontCache = nsnull;
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
  mNullPen = NULL;
  mCurrTextColor = RGB(0, 0, 0);
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
  NS_IF_RELEASE(mFontCache);

  //destroy the initial GraphicsState

  PopState();

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
      ::DeleteObject(mDefFont);
      mDefFont = NULL;
    }

    if (NULL != mOrigSolidPen)
    {
      ::SelectObject(mDC, mOrigSolidPen);
      mOrigSolidPen = NULL;
    }

    if (NULL != mCurrBrush)
      ::DeleteObject(mCurrBrush);

    if ((NULL != mBlackBrush) && (mBlackBrush != mCurrBrush))
      ::DeleteObject(mBlackBrush);

    mCurrBrush = NULL;
    mBlackBrush = NULL;

    //don't kill the font because the font cache/metrics owns it
    mCurrFont = NULL;

    if (NULL != mCurrPen)
      ::DeleteObject(mCurrPen);

    if ((NULL != mBlackPen) && (mBlackPen != mCurrPen))
      ::DeleteObject(mBlackPen);

    if ((NULL != mNullPen) && (mNullPen != mCurrPen))
      ::DeleteObject(mNullPen);

    mCurrPen = NULL;
    mBlackPen = NULL;
    mNullPen = NULL;
  }

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

  NS_IF_RELEASE(mSurface);
  NS_IF_RELEASE(mMainSurface);

  NS_IF_RELEASE(mDCOwner);

  mTMatrix = nsnull;
  mDC = NULL;
  mMainDC = NULL;
}

NS_IMPL_QUERY_INTERFACE(nsRenderingContextWin, kRenderingContextIID)
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
    NS_ADDREF(mSurface);
    mSurface->Init(aWindow);
    mDC = mSurface->mDC;
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
    mDC = mSurface->mDC;
  }

  mDCOwner = nsnull;

  return CommonInit();
}

nsresult nsRenderingContextWin :: SetupDC(HDC aOldDC, HDC aNewDC)
{
  ::SetTextColor(aNewDC, RGB(0, 0, 0));
  ::SetBkMode(aNewDC, TRANSPARENT);
  ::SetPolyFillMode(aNewDC, WINDING);
  ::SetStretchBltMode(aNewDC, COLORONCOLOR);

  if (nsnull != aOldDC)
  {
    if (nsnull != mOrigSolidBrush)
      ::SelectObject(aOldDC, mOrigSolidBrush);

    if (nsnull != mOrigFont)
      ::SelectObject(aOldDC, mOrigFont);

    if (nsnull != mOrigSolidPen)
      ::SelectObject(aOldDC, mOrigSolidPen);

    if (nsnull != mOrigPalette)
      ::SelectPalette(aOldDC, mOrigPalette, TRUE);
  }

  mOrigSolidBrush = (HBRUSH)::SelectObject(aNewDC, mBlackBrush);
  mOrigFont = (HFONT)::SelectObject(aNewDC, mDefFont);
  mOrigSolidPen = (HPEN)::SelectObject(aNewDC, mBlackPen);

  // If this is a palette device, then select and realize the palette
  nsPaletteInfo palInfo;
  mContext->GetPaletteInfo(palInfo);
  if (palInfo.isPaletteDevice && palInfo.palette) {
    // Select the palette in the background
    mOrigPalette = ::SelectPalette(aNewDC, (HPALETTE)palInfo.palette, TRUE);
    // Don't do the realization for an off-screen memory DC
    if (nsnull == aOldDC) {
      ::RealizePalette(aNewDC);
    }
  }

  return NS_OK;
}

nsresult nsRenderingContextWin :: CommonInit(void)
{
  float app2dev;
  mContext->GetAppUnitsToDevUnits(app2dev);
	mTMatrix->AddScale(app2dev, app2dev);
  mContext->GetDevUnitsToAppUnits(mP2T);
  mContext->GetFontCache(mFontCache);

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

NS_IMETHODIMP
nsRenderingContextWin :: SelectOffScreenDrawingSurface(nsDrawingSurface aSurface)
{
  NS_ASSERTION(!(nsnull != mMainDC), "offscreen surface already selected");

  mMainSurface = mSurface;
  mMainDC = mDC;

  mSurface = (nsDrawingSurfaceWin *)aSurface;

  if (nsnull != aSurface)
  {
    NS_ADDREF(mSurface);
    mDC = mSurface->mDC;
  }

  return SetupDC(mMainDC, mDC);
}

NS_IMETHODIMP
nsRenderingContextWin::GetHints(PRUint32& aResult)
{
  PRUint32 result = 0;
  if (gIsWIN95) {
    result |= NS_RENDERING_HINT_FAST_8BIT_TEXT;
  }
  aResult = result;
  return NS_OK;
}

void nsRenderingContextWin :: Reset()
{
}

nsIDeviceContext * nsRenderingContextWin :: GetDeviceContext(void)
{
  NS_IF_ADDREF(mContext);
  return mContext;
}

void nsRenderingContextWin :: PushState(void)
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
    state->mClipRegion = NULL;
    state->mBrushColor = mStates->mBrushColor;
    state->mSolidBrush = NULL;
    state->mFontMetrics = mStates->mFontMetrics;
    state->mFont = NULL;
    state->mPenColor = mStates->mPenColor;
    state->mSolidPen = NULL;
    state->mFlags = ~FLAGS_ALL;
    state->mTextColor = mStates->mTextColor;

    mStates = state;
  }

  mTMatrix = &mStates->mMatrix;
}

PRBool nsRenderingContextWin :: PopState(void)
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
      oldstate->mSolidBrush = NULL;
      oldstate->mFont = NULL;
      oldstate->mSolidPen = NULL;
    }
    else
      mTMatrix = nsnull;
  }

  return retval;
}

PRBool nsRenderingContextWin :: IsVisibleRect(const nsRect& aRect)
{
  return PR_TRUE;
}

PRBool nsRenderingContextWin :: SetClipRect(const nsRect& aRect, nsClipCombine aCombine)
{
  nsRect  trect = aRect;
  int     cliptype;

  mStates->mLocalClip = aRect;

	mTMatrix->TransformCoord(&trect.x, &trect.y,
                           &trect.width, &trect.height);

  mStates->mFlags |= FLAG_LOCAL_CLIP_VALID;

  //how we combine the new rect with the previous?

  if (aCombine == nsClipCombine_kIntersect)
  {
    PushClipState();

    cliptype = ::IntersectClipRect(mDC, trect.x,
                                   trect.y,
                                   trect.XMost(),
                                   trect.YMost());
  }
  else if (aCombine == nsClipCombine_kUnion)
  {
    PushClipState();

    HRGN  tregion = ::CreateRectRgn(trect.x,
                                    trect.y,
                                    trect.XMost(),
                                    trect.YMost());

    cliptype = ::ExtSelectClipRgn(mDC, tregion, RGN_OR);
    ::DeleteObject(tregion);
  }
  else if (aCombine == nsClipCombine_kSubtract)
  {
    PushClipState();

    cliptype = ::ExcludeClipRect(mDC, trect.x,
                                 trect.y,
                                 trect.XMost(),
                                 trect.YMost());
  }
  else if (aCombine == nsClipCombine_kReplace)
  {
    PushClipState();

    HRGN  tregion = ::CreateRectRgn(trect.x,
                                    trect.y,
                                    trect.XMost(),
                                    trect.YMost());
    cliptype = ::SelectClipRgn(mDC, tregion);
    ::DeleteObject(tregion);
  }
  else
    NS_ASSERTION(FALSE, "illegal clip combination");

  if (cliptype == NULLREGION)
    return PR_TRUE;
  else
    return PR_FALSE;
}

PRBool nsRenderingContextWin :: GetClipRect(nsRect &aRect)
{
  if (mStates->mFlags & FLAG_LOCAL_CLIP_VALID)
  {
    aRect = mStates->mLocalClip;
    return PR_TRUE;
  }
  else
    return PR_FALSE;
}

PRBool nsRenderingContextWin :: SetClipRegion(const nsIRegion& aRegion, nsClipCombine aCombine)
{
  nsRegionWin *pRegion = (nsRegionWin *)&aRegion;
  HRGN        hrgn = pRegion->GetHRGN();
  int         cmode, cliptype;

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
    return PR_TRUE;
  else
    return PR_FALSE;
}

void nsRenderingContextWin :: GetClipRegion(nsIRegion **aRegion)
{
  //XXX wow, needs to do something.
}

void nsRenderingContextWin :: SetColor(nscolor aColor)
{
  mCurrentColor = aColor;
  mColor = RGB(mGammaTable[NS_GET_R(aColor)],
               mGammaTable[NS_GET_G(aColor)],
               mGammaTable[NS_GET_B(aColor)]);
}

nscolor nsRenderingContextWin :: GetColor() const
{
  return mCurrentColor;
}

void nsRenderingContextWin :: SetFont(const nsFont& aFont)
{
  NS_IF_RELEASE(mFontMetrics);
  mFontCache->GetMetricsFor(aFont, mFontMetrics);
}

const nsFont& nsRenderingContextWin :: GetFont()
{
  const nsFont* font;
  mFontMetrics->GetFont(font);
  return *font;
}

nsIFontMetrics* nsRenderingContextWin :: GetFontMetrics()
{
  NS_IF_ADDREF(mFontMetrics);
  return mFontMetrics;
}

// add the passed in translation to the current translation
void nsRenderingContextWin :: Translate(nscoord aX, nscoord aY)
{
	mTMatrix->AddTranslation((float)aX,(float)aY);
}

// add the passed in scale to the current scale
void nsRenderingContextWin :: Scale(float aSx, float aSy)
{
	mTMatrix->AddScale(aSx, aSy);
}

nsTransform2D * nsRenderingContextWin :: GetCurrentTransform()
{
  return mTMatrix;
}

nsDrawingSurface nsRenderingContextWin :: CreateDrawingSurface(nsRect *aBounds)
{
  nsDrawingSurfaceWin *surf = new nsDrawingSurfaceWin();

  if (nsnull != surf)
  {
    NS_ADDREF(surf);

#ifdef NGLAYOUT_DDRAW
    LPDIRECTDRAWSURFACE ddsurf = nsnull;

    if ((NULL != mDDraw2) && (nsnull != aBounds))
    {
      DDSURFACEDESC ddsd;

      ddsd.dwSize = sizeof(ddsd);
      ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
      ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
      ddsd.dwWidth = aBounds->width;
      ddsd.dwHeight = aBounds->height;

      nsresult res = mDDraw2->CreateSurface(&ddsd, &ddsurf, NULL);
    }

    if (NULL != ddsurf)
    {
      surf->Init(ddsurf);
      NS_RELEASE(ddsurf);
    }
    else
#endif
      surf->Init(::CreateCompatibleDC(mDC));

    HBITMAP hBits;

    if (nsnull != aBounds)
      hBits = ::CreateCompatibleBitmap(mDC, aBounds->width, aBounds->height);
    else
    {
      //we do this to make sure that the memory DC knows what the
      //bitmap format of the original DC was. this way, later
      //operations to create bitmaps from the memory DC will create
      //bitmaps with the correct properties.

      hBits = ::CreateCompatibleBitmap(mDC, 2, 2);
    }

    surf->mOrigBitmap = (HBITMAP)::SelectObject(surf->mDC, hBits);
  }

  return (nsDrawingSurface)surf;
}

void nsRenderingContextWin :: DestroyDrawingSurface(nsDrawingSurface aDS)
{
  nsDrawingSurfaceWin *surf = (nsDrawingSurfaceWin *)aDS;

  NS_IF_RELEASE(surf);
}

void nsRenderingContextWin :: DrawLine(nscoord aX0, nscoord aY0, nscoord aX1, nscoord aY1)
{
	mTMatrix->TransformCoord(&aX0,&aY0);
	mTMatrix->TransformCoord(&aX1,&aY1);

  SetupSolidPen();

  ::MoveToEx(mDC, (int)(aX0), (int)(aY0), NULL);
  ::LineTo(mDC, (int)(aX1), (int)(aY1));
}

void nsRenderingContextWin :: DrawPolyline(const nsPoint aPoints[], PRInt32 aNumPoints)
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

  // Draw the polyline
  SetupSolidPen();
  ::Polyline(mDC, pp0, int(aNumPoints));

  // Release temporary storage if necessary
  if (pp0 != pts)
    delete pp0;
}

void nsRenderingContextWin :: DrawRect(const nsRect& aRect)
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
}

void nsRenderingContextWin :: DrawRect(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight)
{
  RECT nr;

	mTMatrix->TransformCoord(&aX,&aY,&aWidth,&aHeight);
	nr.left = aX;
	nr.top = aY;
	nr.right = aX+aWidth;
	nr.bottom = aY+aHeight;

  ::FrameRect(mDC, &nr, SetupSolidBrush());
}

void nsRenderingContextWin :: FillRect(const nsRect& aRect)
{
  RECT nr;
	nsRect	tr;

	tr = aRect;
	mTMatrix->TransformCoord(&tr.x,&tr.y,&tr.width,&tr.height);
	nr.left = tr.x;
	nr.top = tr.y;
	nr.right = tr.x+tr.width;
	nr.bottom = tr.y+tr.height;

  ::FillRect(mDC, &nr, SetupSolidBrush());
}

void nsRenderingContextWin :: FillRect(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight)
{
  RECT nr;
	nsRect	tr;

	mTMatrix->TransformCoord(&aX,&aY,&aWidth,&aHeight);
	nr.left = aX;
	nr.top = aY;
	nr.right = aX+aWidth;
	nr.bottom = aY+aHeight;

  ::FillRect(mDC, &nr, SetupSolidBrush());
}

void nsRenderingContextWin::DrawPolygon(const nsPoint aPoints[], PRInt32 aNumPoints)
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

  // Outline the polygon
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
}

void nsRenderingContextWin::FillPolygon(const nsPoint aPoints[], PRInt32 aNumPoints)
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
}

void nsRenderingContextWin :: DrawEllipse(const nsRect& aRect)
{
  DrawEllipse(aRect.x, aRect.y, aRect.width, aRect.height);
}

void nsRenderingContextWin :: DrawEllipse(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight)
{
  mTMatrix->TransformCoord(&aX, &aY, &aWidth, &aHeight);

  SetupSolidPen();
  HBRUSH oldBrush = (HBRUSH)::SelectObject(mDC, ::GetStockObject(NULL_BRUSH));
  
  ::Ellipse(mDC, aX, aY, aX + aWidth, aY + aHeight);
  ::SelectObject(mDC, oldBrush);
}

void nsRenderingContextWin :: FillEllipse(const nsRect& aRect)
{
  FillEllipse(aRect.x, aRect.y, aRect.width, aRect.height);
}

void nsRenderingContextWin :: FillEllipse(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight)
{
  mTMatrix->TransformCoord(&aX, &aY, &aWidth, &aHeight);

  SetupSolidPen();
  SetupSolidBrush();
  
  ::Ellipse(mDC, aX, aY, aX + aWidth, aY + aHeight);
}

void nsRenderingContextWin :: DrawArc(const nsRect& aRect,
                                 float aStartAngle, float aEndAngle)
{
  this->DrawArc(aRect.x,aRect.y,aRect.width,aRect.height,aStartAngle,aEndAngle);
}

void nsRenderingContextWin :: DrawArc(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight,
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

  // this just makes it consitent, on windows 95 arc will always draw CC, nt this sets direction
  ::SetArcDirection(mDC, AD_COUNTERCLOCKWISE);

  ::Arc(mDC, aX, aY, aX + aWidth, aY + aHeight, sx, sy, ex, ey); 
}

void nsRenderingContextWin :: FillArc(const nsRect& aRect,
                                 float aStartAngle, float aEndAngle)
{
  this->FillArc(aRect.x, aRect.y, aRect.width, aRect.height, aStartAngle, aEndAngle);
}

void nsRenderingContextWin :: FillArc(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight,
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
}

void nsRenderingContextWin :: DrawString(const char *aString, PRUint32 aLength,
                                    nscoord aX, nscoord aY,
                                    nscoord aWidth)
{
	int	x = aX;
  int y = aY;

  SetupFontAndColor();
	mTMatrix->TransformCoord(&x,&y);
  ::ExtTextOut(mDC,x,y,0,NULL,aString,aLength,NULL);

  if (GetFont().decorations & NS_FONT_DECORATION_OVERLINE)
    DrawLine(aX, aY, aX + aWidth, aY);
}

void nsRenderingContextWin :: DrawString(const PRUnichar *aString, PRUint32 aLength,
                                         nscoord aX, nscoord aY, nscoord aWidth)
{
	int		x = aX;
  int   y = aY;

  SetupFontAndColor();
	mTMatrix->TransformCoord(&x,&y);
  ::ExtTextOutW(mDC,x,y,0,NULL,aString,aLength,NULL);

  if (GetFont().decorations & NS_FONT_DECORATION_OVERLINE)
    DrawLine(aX, aY, aX + aWidth, aY);
}

void nsRenderingContextWin :: DrawString(const nsString& aString,
                                         nscoord aX, nscoord aY, nscoord aWidth)
{
  DrawString(aString.GetUnicode(), aString.Length(), aX, aY, aWidth);
}

void nsRenderingContextWin :: DrawImage(nsIImage *aImage, nscoord aX, nscoord aY)
{
  NS_PRECONDITION(PR_TRUE == mInitialized, "!initialized");

  nscoord width, height;

  width = NSToCoordRound(mP2T * aImage->GetWidth());
  height = NSToCoordRound(mP2T * aImage->GetHeight());

  this->DrawImage(aImage, aX, aY, width, height);
}

void nsRenderingContextWin :: DrawImage(nsIImage *aImage, nscoord aX, nscoord aY,
                                        nscoord aWidth, nscoord aHeight) 
{
  nsRect  tr;

  tr.x = aX;
  tr.y = aY;
  tr.width = aWidth;
  tr.height = aHeight;

  this->DrawImage(aImage, tr);
}

void nsRenderingContextWin :: DrawImage(nsIImage *aImage, const nsRect& aSRect, const nsRect& aDRect)
{
  nsRect	sr,dr;

	sr = aSRect;
	mTMatrix->TransformCoord(&sr.x, &sr.y, &sr.width, &sr.height);

  dr = aDRect;
	mTMatrix->TransformCoord(&dr.x, &dr.y, &dr.width, &dr.height);

  ((nsImageWin *)aImage)->Draw(*this, mSurface, sr.x, sr.y, sr.width, sr.height, dr.x, dr.y, dr.width, dr.height);
}

void nsRenderingContextWin :: DrawImage(nsIImage *aImage, const nsRect& aRect)
{
  nsRect	tr;

	tr = aRect;
	mTMatrix->TransformCoord(&tr.x, &tr.y, &tr.width, &tr.height);

  ((nsImageWin *)aImage)->Draw(*this, mSurface, tr.x, tr.y, tr.width, tr.height);
}

NS_IMETHODIMP nsRenderingContextWin :: CopyOffScreenBits(nsRect &aBounds)
{
  if ((nsnull != mDC) && (nsnull != mMainDC))
  {
    HRGN  tregion = ::CreateRectRgn(0, 0, 0, 0);

    if (::GetClipRgn(mDC, tregion) == 1)
      ::SelectClipRgn(mMainDC, tregion);
//    else
//      ::SelectClipRgn(mMainDC, NULL);

    ::DeleteObject(tregion);

#if 0
    GraphicsState *pstate = mStates;

    //look for a cliprect somewhere in the stack...

    while ((nsnull != pstate) && !(pstate->mFlags & FLAG_CLIP_VALID))
      pstate = pstate->mNext;

    if (nsnull != pstate)
      ::SelectClipRgn(mMainDC, pstate->mClipRegion);
    else
      ::SelectClipRgn(mMainDC, NULL);
#endif

    // If there's a palette make sure it's selected.
    // XXX This doesn't seem like the best place to be doing this...
    nsPaletteInfo palInfo;
    HPALETTE      oldPalette;
    mContext->GetPaletteInfo(palInfo);
    if (palInfo.isPaletteDevice && palInfo.palette) {
      oldPalette = ::SelectPalette(mMainDC, (HPALETTE)palInfo.palette, TRUE);
    }
    ::BitBlt(mMainDC, 0, 0, aBounds.width, aBounds.height, mDC, 0, 0, SRCCOPY);
    if (palInfo.isPaletteDevice && palInfo.palette) {
      ::SelectPalette(mMainDC, oldPalette, TRUE);
    }
  }
  else
    NS_ASSERTION(0, "attempt to blit with bad DCs");

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
    // XXX In 16-bit mode we need to use GetNearestColor() to get a solid
    // color; otherwise, we'll end up with a dithered brush...
    HBRUSH  tbrush = ::CreateSolidBrush(PALETTERGB_COLORREF(mColor));

    ::SelectObject(mDC, tbrush);

    if (NULL != mCurrBrush)
      ::DeleteObject(mCurrBrush);

    mStates->mSolidBrush = mCurrBrush = tbrush;
    mStates->mBrushColor = mCurrBrushColor = mCurrentColor;
//printf("brushes: %d\n", ++numbrush);
  }

  return mCurrBrush;
}

void nsRenderingContextWin :: SetupFontAndColor(void)
{
  if ((mFontMetrics != mCurrFontMetrics) || (NULL == mCurrFontMetrics))
  {
    nsFontHandle  fontHandle;
    mFontMetrics->GetFontHandle(fontHandle);
    HFONT         tfont = (HFONT)fontHandle;
    
    ::SelectObject(mDC, tfont);

    mStates->mFont = mCurrFont = tfont;
    mStates->mFontMetrics = mCurrFontMetrics = mFontMetrics;
//printf("fonts: %d\n", ++numfont);
  }

  if (mCurrentColor != mCurrTextColor) {
    ::SetTextColor(mDC, PALETTERGB_COLORREF(mColor));
    mStates->mTextColor = mCurrTextColor = mCurrentColor;
  }
}

HPEN nsRenderingContextWin :: SetupSolidPen(void)
{
  if ((mCurrentColor != mCurrPenColor) || (NULL == mCurrPen))
  {
    HPEN  tpen = ::CreatePen(PS_SOLID, 0, PALETTERGB_COLORREF(mColor));

    ::SelectObject(mDC, tpen);

    if (NULL != mCurrPen)
      ::DeleteObject(mCurrPen);

    mStates->mSolidPen = mCurrPen = tpen;
    mStates->mPenColor = mCurrPenColor = mCurrentColor;
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

#ifdef NGLAYOUT_DDRAW

nsresult nsRenderingContextWin :: CreateDDraw()
{
  if ((mDDraw2 == NULL) && (mDDrawResult == NS_OK))
  {
    CoInitialize(NULL);

    mDDrawResult = DirectDrawCreate(NULL, &mDDraw, NULL);

    if (mDDrawResult == NS_OK)
      mDDrawResult = mDDraw->QueryInterface(IID_IDirectDraw2, (LPVOID *)&mDDraw2);

    if (mDDrawResult == NS_OK)
    {
      mDDraw2->SetCooperativeLevel(NULL, DDSCL_NORMAL);

#ifdef NS_DEBUG
      printf("using DirectDraw (%08X)\n", mDDraw2);

      DDSCAPS ddscaps;
      DWORD   totalmem, freemem;
    
      ddscaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY;
      nsresult res = mDDraw2->GetAvailableVidMem(&ddscaps, &totalmem, &freemem);

      if (NS_SUCCEEDED(res))
      {
        printf("total video memory: %d\n", totalmem);
        printf("free video memory: %d\n", freemem);
      }
      else
      {
        printf("GetAvailableVidMem() returned %08x: %s\n", res,
               (res == DDERR_NODIRECTDRAWHW) ?
               "no hardware ddraw driver available" : "unknown error code");
      }
#endif
    }
  }

  return mDDrawResult;
}

#endif
