/*
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
 * License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is the Mozilla OS/2 libraries.
 *
 * The Initial Developer of the Original Code is John Fairhurst,
 * <john_fairhurst@iname.com>.  Portions created by John Fairhurst are
 * Copyright (C) 1999 John Fairhurst. All Rights Reserved.
 *
 * Contributor(s):
 * Henry Sobotka <sobotka@axess.com> Jan. 2000 review and update
 * Pierre Phaneuf <pp@ludusdesign.com>
 *
 * This Original Code has been modified by IBM Corporation.
 * Modifications made by IBM described herein are
 * Copyright (c) International Business Machines
 * Corporation, 2000
 *
 * Modifications to Mozilla code or documentation
 * identified per MPL Section 3.3
 *
 * Date             Modified by     Description of modification
 * 03/23/2000       IBM Corp.      Fixed InvertRect.
 * 05/08/2000       IBM Corp.      Fix for trying to us an already freed mGammaTable.
 * 05/31/2000       IBM Corp.      Fix background color on PS printers.
 *
 */

//#define PROFILE_GSTATE // be noisy about graphicsstate-usage
#define GSTATE_CACHESIZE 50

// ToDo: Unicode text draw-er
#include "nsGfxDefs.h"

#include "nsRenderingContextOS2.h"
#include "nsIComponentManager.h"
#include "nsDeviceContextOS2.h"
#include "nsFontMetricsOS2.h"
#include "nsIFontMetrics.h"
#include "nsTransform2D.h"
#include "nsRegionOS2.h"
#include "nsGfxCIID.h"
#include "nsString.h"
#include "nsFont.h"
#include "libimg.h"
#include "prprf.h"

// helper clip region functions - defined at the bottom of this file.
LONG OS2_CombineClipRegion( HPS hps, HRGN hrgnCombine, LONG lMode);
HRGN OS2_CopyClipRegion( HPS hps);
#define OS2_SetClipRegion(hps,hrgn) OS2_CombineClipRegion(hps, hrgn, CRGN_COPY)

// Use these instead of native GpiSave/RestorePS because: need to store ----
// more information, and need to be able to push from onscreen & pop onto
// offscreen.  Potentially.
struct GraphicsState
{
   GraphicsState() { Construct(); }
  ~GraphicsState() { Destruct(); }

   void Construct();
   void Destruct();

   nsTransform2D   mMatrix;
   HRGN            mClipRegion;
   nscolor         mColor;
   nsIFontMetrics *mFontMetrics;
   nsLineStyle     mLineStyle;

   GraphicsState  *mNext;
};

void GraphicsState::Construct()
{
   mClipRegion = 0;
   mColor = NS_RGB( 0, 0, 0);
   mFontMetrics = nsnull;
   mLineStyle = nsLineStyle_kSolid;
   mNext = nsnull;
}

void GraphicsState::Destruct()
{
   if( mClipRegion)
   {
      printf( "oops, leaked a region from rc-gs\n");
      mClipRegion = 0;
   }
   NS_IF_RELEASE( mFontMetrics);
}

class GraphicsStateCache
{
   GraphicsState *mFirst;
   UINT           mSize;
#ifdef PROFILE_GSTATE
   UINT           mDeleted;
   UINT           mCount;
   UINT           mPeak;
#endif

 public:
   GraphicsStateCache() : mFirst(nsnull), mSize(0)
#ifdef PROFILE_GSTATE
                         , mDeleted(0), mCount(0), mPeak(0)
#endif
   {}

  ~GraphicsStateCache()
   {
#ifdef PROFILE_GSTATE
      printf( "---- Graphics-State Stats -----\n");
      printf( "  GStates requested:       %d\n", mCount);
      printf( "  Actual GStates created:  %d\n", mPeak);
      double d = mCount ? (double)(mCount - mPeak) / (double)mCount : 0;
      printf( "  Gstates recycled:        %d (%d%%)\n", mCount - mPeak, (int)(d*100.0));
      UINT i = mSize+mDeleted;
      printf( "  Cached+Deleted:          %d\n", i);
      if( i != mPeak)
         printf( "  WARNING: GStates leaked: %d\n", mPeak - i);
      printf( "------------------------------\n\n");
#endif

      // Clear up the cache
      GraphicsState *pTemp, *pNext = mFirst;
      while( pNext)
      {
         pTemp = pNext->mNext;
         delete pNext;
         pNext = pTemp;
      }
   }

   GraphicsState *NewState()
   {
      GraphicsState *state = nsnull;
      if( mFirst)
      {
         state = mFirst;
         mFirst = mFirst->mNext;
         state->Construct();
         mSize--;
      }
      else
      {
         state = new GraphicsState;
#ifdef PROFILE_GSTATE
         mPeak++;
#endif
      }
#ifdef PROFILE_GSTATE
      mCount++;
#endif
      return state;
   }

   void DisposeState( GraphicsState *aState)
   {
      if( GSTATE_CACHESIZE == mSize)
      {
         delete aState;
#ifdef PROFILE_GSTATE
         mDeleted++;
#endif
      }
      else
      {
         aState->Destruct();
         aState->mNext = mFirst;
         mFirst = aState;
         mSize++;
      }
   }

} GStateCache; // XXX make this less static, I suppose

// Rendering context -------------------------------------------------------

NS_IMPL_ADDREF(nsRenderingContextOS2)
NS_IMPL_RELEASE(nsRenderingContextOS2)

nsresult
nsRenderingContextOS2::QueryInterface( REFNSIID aIID, void **aInstancePtr)
{
   if( !aInstancePtr)
      return NS_ERROR_NULL_POINTER;

   if( aIID.Equals( nsIRenderingContext::GetIID()))
      *aInstancePtr = (void *) (nsIRenderingContext*) this;
   else if( aIID.Equals( ((nsIRenderingContext*)this)->GetIID()))
      *aInstancePtr = (void *) (nsIRenderingContext*)this;

   if( !*aInstancePtr)
      return NS_NOINTERFACE;

   NS_ADDREF_THIS();

   return NS_OK;
}

// Init-term stuff ---------------------------------------------------------

nsRenderingContextOS2::nsRenderingContextOS2()
{
   NS_INIT_REFCNT();

   mContext = nsnull;
   mSurface = nsnull;
   mFrontSurface = nsnull;
   mColor = NS_RGB( 0, 0, 0);
   mP2T = 1.0f;
   mStateStack = nsnull;
   mFontMetrics = nsnull;
   mCurrFontMetrics = nsnull;
   mCurrDrawingColor = NS_RGB( 0, 0, 0);
   mAlreadySetDrawingColor = PR_FALSE;
   mCurrTextColor = NS_RGB( 0, 0, 0);
   mAlreadySetTextColor = PR_FALSE;
   mLineStyle = nsLineStyle_kSolid;
   mCurrLineStyle = nsLineStyle_kSolid;

   // Other impls want to do a PushState() here.  I can't see why...
   // PushState();
}

nsRenderingContextOS2::~nsRenderingContextOS2()
{
   NS_IF_RELEASE(mContext);

   // clear state stack
   GraphicsState *pTemp, *pNext = mStateStack;
   while( pNext)
   {
      if( pNext->mClipRegion)
      {
         GFX (::GpiDestroyRegion (mSurface->mPS, pNext->mClipRegion), FALSE);
         pNext->mClipRegion = 0;
      }
      pTemp = pNext->mNext;
      GStateCache.DisposeState( pNext);
      pNext = pTemp;
   }

   // Release surfaces and the palette
   NS_IF_RELEASE(mFrontSurface);
   NS_IF_RELEASE(mSurface);
   NS_IF_RELEASE(mFontMetrics);
}

// I've NS_ADDREF'd as opposed to NS_IF_ADDREF'd 'cos if certain things
// are null, then we want to know about it straight away.
// References rule, sigh.
nsresult nsRenderingContextOS2::Init( nsIDeviceContext *aContext,
                                      nsIWidget *aWindow)
{
   mContext = aContext;
   NS_ADDREF(mContext);

   // Create & remember an on-screen surface
   nsWindowSurface *surf = new nsWindowSurface;
   if (!surf)
     return NS_ERROR_OUT_OF_MEMORY;

   surf->Init(aWindow);

   mSurface = surf;
   NS_ADDREF(mSurface);

   mDCOwner = aWindow;

   // Grab another reference to the onscreen for later uniformity
   mFrontSurface = mSurface;
   NS_ADDREF(mFrontSurface);


   return CommonInit();
}

nsresult nsRenderingContextOS2::Init( nsIDeviceContext *aContext,
                                      nsDrawingSurface aSurface)
{
   mContext = aContext;
   NS_IF_ADDREF(mContext);

   // Add a couple of references to the onscreen (or print, more likely)
   mSurface = (nsDrawingSurfaceOS2 *) aSurface;
   NS_ADDREF(mSurface);
   mFrontSurface = mSurface;
   NS_ADDREF(mFrontSurface);

   return CommonInit();
}

// Presentation space page units (& so world coords) are PU_PELS.
// We have a matrix, mTMatrix, which converts from the passed in app units
// to pels.  Note there is *no* guarantee that app units == twips.
nsresult nsRenderingContextOS2::CommonInit()
{
   mContext->GetGammaTable(mGammaTable);
   float app2dev = 0;
   mContext->GetAppUnitsToDevUnits( app2dev);
   mTMatrix.AddScale( app2dev, app2dev);
   mContext->GetDevUnitsToAppUnits( mP2T);

  // If this is a palette device, then select and realize the palette
  nsPaletteInfo palInfo;
  mContext->GetPaletteInfo(palInfo);

  // Set image foreground and background colors. These are used in transparent images for blitting 1-bit masks.
  // To invert colors on ROP_SRCAND we map 1 to black and 0 to white
  // map 1 in mask to 0x000000 (black) in destination
  // map 0 in mask to 0xFFFFFF (white) in destination
  IMAGEBUNDLE ib;

  if (palInfo.isPaletteDevice && palInfo.palette)
  {
    ULONG cclr;
 
    // Select the palette in the background
    GFX (::GpiSelectPalette (mSurface->mPS, (HPAL)palInfo.palette), PAL_ERROR);
    if (mDCOwner) {
      ::WinRealizePalette((HWND)mDCOwner->GetNativeData(NS_NATIVE_WINDOW),mSurface->mPS, &cclr);
    } /* endif */
    ib.lColor     = GFX (::GpiQueryColorIndex (mSurface->mPS, 0, MK_RGB (0x00, 0x00, 0x00)), GPI_ALTERROR); // CLR_BLACK
    ib.lBackColor = GFX (::GpiQueryColorIndex (mSurface->mPS, 0, MK_RGB (0xFF, 0xFF, 0xFF)), GPI_ALTERROR); // CLR_WHITE
  }
  else
  {
    GFX (::GpiCreateLogColorTable (mSurface->mPS, 0, LCOLF_RGB, 0, 0, 0), FALSE);
    ib.lColor     = MK_RGB (0x00, 0x00, 0x00); // CLR_BLACK
    ib.lBackColor = MK_RGB (0xFF, 0xFF, 0xFF); // CLR_WHITE
  }

  ib.usMixMode  = FM_OVERPAINT;
  ib.usBackMixMode = BM_OVERPAINT;
  GFX (::GpiSetAttrs (mSurface->mPS, PRIM_IMAGE, IBB_COLOR | IBB_BACK_COLOR | IBB_MIX_MODE | IBB_BACK_MIX_MODE, 0, (PBUNDLE)&ib), FALSE);

  return NS_OK;
}

// PS & drawing surface management -----------------------------------------

nsresult nsRenderingContextOS2::SelectOffScreenDrawingSurface( nsDrawingSurface aSurface)
{
   nsresult rc = NS_ERROR_FAILURE;

   if( aSurface)
   {
      NS_IF_RELEASE(mSurface);
      mSurface = (nsDrawingSurfaceOS2 *) aSurface;
      // If this is a palette device, then select and realize the palette
      nsPaletteInfo palInfo;
      mContext->GetPaletteInfo(palInfo);
    
      if (palInfo.isPaletteDevice && palInfo.palette)
      {
        ULONG cclr;
        // Select the palette in the background
        GFX (::GpiSelectPalette (mSurface->mPS, (HPAL)palInfo.palette), PAL_ERROR);
        if (mDCOwner) {
          ::WinRealizePalette((HWND)mDCOwner->GetNativeData(NS_NATIVE_WINDOW),mSurface->mPS, &cclr);
        } /* endif */
      }
      else
      {
        GFX (::GpiCreateLogColorTable (mSurface->mPS, 0, LCOLF_RGB, 0, 0, 0), FALSE);
      }
   }
   else // deselect current offscreen...
   {
      NS_IF_RELEASE(mSurface);
      mSurface = mFrontSurface;
      rc = NS_OK;
   }

   // Set image foreground and background colors. These are used in transparent images for blitting 1-bit masks.
   // To invert colors on ROP_SRCAND we map 1 to black and 0 to white
   IMAGEBUNDLE ib;
   ib.lColor     = GFX (::GpiQueryColorIndex (mSurface->mPS, 0, MK_RGB (0, 0, 0)), GPI_ALTERROR);        // map 1 in mask to 0x000000 (black) in destination
   ib.lBackColor = GFX (::GpiQueryColorIndex (mSurface->mPS, 0, MK_RGB (255, 255, 255)), GPI_ALTERROR);  // map 0 in mask to 0xFFFFFF (white) in destination
   ib.usMixMode  = FM_OVERPAINT;
   ib.usBackMixMode = BM_OVERPAINT;
   GFX (::GpiSetAttrs (mSurface->mPS, PRIM_IMAGE, IBB_COLOR | IBB_BACK_COLOR | IBB_MIX_MODE | IBB_BACK_MIX_MODE, 0, (PBUNDLE)&ib), FALSE);

   // need to force a state refresh because the offscreen is something of
   // an unknown quantity.
   SetupDrawingColor( TRUE);
   SetupFontAndColor( TRUE);

   // Flush the offscreen's font cache because it's being associated with
   // a new onscreen.
//
// Actually I don't think this is necessary.
//
//   mSurface->FlushFontCache();
//

   NS_ADDREF(mSurface);

   return rc;
}

// !! This is a bit dodgy; nsDrawingSurface *really* needs to be XP-comified
// !! properly...
nsresult nsRenderingContextOS2::GetDrawingSurface( nsDrawingSurface *aSurface)
{
   if( !aSurface)
      return NS_ERROR_NULL_POINTER;

#if 0
   NS_IF_RELEASE( *aSurface);
#endif

   *aSurface = (void*)((nsDrawingSurfaceOS2 *) mSurface);
   // !! probably don't want to addref this 'cos client can't release it...

   return NS_OK;
}

// The idea here is to create an offscreen surface for blitting with.
// I can't find any way for people to resize the bitmap created here,
// so I guess this gets called quite often.
//
// I'm reliably told that 'aBounds' is in device units, and that the
// position oughtn't to be ignored, but for all intents & purposes can be.
//
nsresult nsRenderingContextOS2::CreateDrawingSurface( nsRect *aBounds,
                             PRUint32 aSurfFlags, nsDrawingSurface &aSurface)
{
   // We're gonna ignore this `aSurfFlags' due to not understanding it...

   nsresult rc = NS_ERROR_FAILURE;

   nsOffscreenSurface *surf = new nsOffscreenSurface;
   if (!surf)
     return NS_ERROR_OUT_OF_MEMORY;

   rc = surf->Init( mFrontSurface->mPS, aBounds->width, aBounds->height);

   if(NS_SUCCEEDED(rc))
   {
      NS_ADDREF(surf);
      aSurface = (void*)((nsDrawingSurfaceOS2 *) surf);
   }
   else
      delete surf;

   return rc;
}

nsresult nsRenderingContextOS2::DestroyDrawingSurface( nsDrawingSurface aDS)
{
   nsDrawingSurfaceOS2 *surf = (nsDrawingSurfaceOS2 *) aDS;
   nsresult rc = NS_ERROR_NULL_POINTER;

   // If the surface being destroyed is the one we're currently using for
   // offscreen, then lose our reference to it & hook back to the onscreen.
   if( surf && surf == mSurface)
   {
      NS_RELEASE(mSurface);    // ref. from SelectOffscreen
      mSurface = mFrontSurface;
      NS_ADDREF(mSurface);
   }

   if( surf)
   {
      NS_RELEASE(surf);        // ref. from CreateSurface
      rc = NS_OK;
   }

   return rc;
}

#if 0 // now inlined in the header
void nsRenderingContextOS2::GetTargetHeight( PRUint32 &ht)
{
   PRUint32 on, dummy, off;
   mSurface->GetDimensions( &dummy, &on);
   if( mSurface != mFrontSurface)
   {
      mFrontSurface->GetDimensions( &dummy, &off);
      if( off < on) on = off;
   }
   ht = on;
}
#endif

nsresult nsRenderingContextOS2::LockDrawingSurface( PRInt32 aX, PRInt32 aY,
                                       PRUint32 aWidth, PRUint32 aHeight,
                                       void **aBits,
                                       PRInt32 *aStride, PRInt32 *aWidthBytes,
                                       PRUint32 aFlags)
{
   return mSurface->Lock( aX, aY, aWidth, aHeight, aBits,
                          aStride, aWidthBytes, aFlags);
}

nsresult nsRenderingContextOS2::UnlockDrawingSurface()
{
   return mSurface->Unlock();
}

// State stack -------------------------------------------------------------
nsresult nsRenderingContextOS2::PushState()
{
   GraphicsState *state = GStateCache.NewState();

   // copy matrix into state
   state->mMatrix.SetMatrix( &mTMatrix);
   // copy color & font
   state->mColor = mColor;
   state->mFontMetrics = mFontMetrics;
   state->mLineStyle = mLineStyle;

   // add a new ref to the fontmetrics
   NS_IF_ADDREF( mFontMetrics);

   // clip region: get current & copy it.
   state->mClipRegion = OS2_CopyClipRegion( mSurface->mPS);

   // push state onto stack
   state->mNext = mStateStack;
   mStateStack = state;
   return NS_OK;
}

// RC here is true if clip region is now empty.
nsresult nsRenderingContextOS2::PopState( PRBool &aClipEmpty)
{
   PRBool rc = PR_TRUE;

   NS_ASSERTION( mStateStack, "state underflow");
   if( !mStateStack) return NS_ERROR_FAILURE;

   GraphicsState *state = mStateStack;
   mStateStack = state->mNext;

   // update xform matrix
   mTMatrix.SetMatrix( &state->mMatrix);

   // color & font
   SetColor( state->mColor);
   SetLineStyle( state->mLineStyle);
   NS_IF_RELEASE(mFontMetrics);
   mFontMetrics = state->mFontMetrics;
   state->mFontMetrics = nsnull;

   // Clip region
   OS2_SetClipRegion( mSurface->mPS, state->mClipRegion);

   if( state->mClipRegion != 0)
   {
      state->mClipRegion = 0;
      rc = PR_FALSE;
   }

   GStateCache.DisposeState( state);

   aClipEmpty = rc;

   return NS_OK;
}

nsresult nsRenderingContextOS2::Reset()
{
   // okay, what's this supposed to do?  Empty the state stack?
   printf( "nsRenderingContext::Reset() -- hmm\n");
   return NS_OK;
}

// OS/2 - XP coord conversion ----------------------------------------------

// get inclusive-inclusive rect
void nsRenderingContextOS2::NS2PM_ININ( const nsRect &in, RECTL &rcl)
{
   PRUint32 ulHeight;
   GetTargetHeight( ulHeight);

   rcl.xLeft = in.x;
   rcl.xRight = in.x + in.width - 1;
   rcl.yTop = ulHeight - in.y - 1;
   rcl.yBottom = rcl.yTop - in.height + 1;
}

void nsRenderingContextOS2::PM2NS_ININ( const RECTL &in, nsRect &out)
{
   PRUint32 ulHeight;
   GetTargetHeight( ulHeight);

   out.x = in.xLeft;
   out.width = in.xRight - in.xLeft + 1;
   out.y = ulHeight - in.yTop - 1;
   out.height = in.yTop - in.yBottom + 1;
}

// get in-ex rect
void nsRenderingContextOS2::NS2PM_INEX( const nsRect &in, RECTL &rcl)
{
   NS2PM_ININ( in, rcl);
   rcl.xRight++;
   rcl.yTop++;
}

void nsRenderingContextOS2::NS2PM( PPOINTL aPointl, ULONG cPointls)
{
   PRUint32 ulHeight;
   GetTargetHeight( ulHeight);

   for( ULONG i = 0; i < cPointls; i++)
      aPointl[ i].y = ulHeight - aPointl[ i].y - 1;
}

// Clip region management --------------------------------------------------

nsresult nsRenderingContextOS2::IsVisibleRect( const nsRect &aRect,
                                               PRBool &aIsVisible)
{
   nsRect trect( aRect);
   mTMatrix.TransformCoord( &trect.x, &trect.y,
                            &trect.width, &trect.height);
   RECTL rcl;
   NS2PM_ININ( trect, rcl);

   LONG rc = GFX (::GpiRectVisible( mSurface->mPS, &rcl), RVIS_ERROR);

   aIsVisible = (rc == RVIS_PARTIAL || rc == RVIS_VISIBLE) ? PR_TRUE : PR_FALSE;


   return NS_OK;
}

// Return PR_TRUE if clip region is now empty
nsresult nsRenderingContextOS2::SetClipRect( const nsRect& aRect, nsClipCombine aCombine, PRBool &aClipEmpty)
{
   nsRect trect = aRect;
   mTMatrix.TransformCoord( &trect.x, &trect.y, &trect.width, &trect.height);
   long lrc = RGN_ERROR;

   if( trect.width == 0 || trect.height == 0)
   {
      // Mozilla assumes that clip region with zero width or height does not produce any output - everything is clipped.
      // GPI does not support clip regions with zero width or height. We cheat by creating 1x1 clip region far outside
      // of real drawing region limits.

      if( aCombine == nsClipCombine_kIntersect || aCombine == nsClipCombine_kReplace)
      {
         RECTL rcl;
         rcl.xLeft   = -1;
         rcl.xRight  = 0;
         rcl.yBottom = -1;
         rcl.yTop    = 0;

         HRGN hrgn = GFX (::GpiCreateRegion( mSurface->mPS, 1, &rcl), RGN_ERROR);
         OS2_SetClipRegion (mSurface->mPS, hrgn);

         lrc = PR_TRUE;      // Should pretend that clipping region is empty
      } else
      {
         // Clipping region is already correct. Just need to obtain it's complexity
         POINTL Offset = { 0, 0 };

         lrc = GFX (::GpiOffsetClipRegion (mSurface->mPS, &Offset), RGN_ERROR);
      }
   }
   else
   {
      RECTL rcl;

      switch( aCombine)
      {
         case nsClipCombine_kIntersect:
         case nsClipCombine_kSubtract:
            NS2PM_ININ( trect, rcl);
            if( aCombine == nsClipCombine_kIntersect)
               lrc = GpiIntersectClipRectangle( mSurface->mPS, &rcl);
            else
               lrc = GpiExcludeClipRectangle( mSurface->mPS, &rcl);
            break;

         case nsClipCombine_kUnion:
         case nsClipCombine_kReplace:
         {
            // need to create a new region & fiddle with it
            NS2PM_INEX( trect, rcl);
            HRGN hrgn = GpiCreateRegion( mSurface->mPS, 1, &rcl);
            if( hrgn && aCombine == nsClipCombine_kReplace)
               lrc = OS2_SetClipRegion( mSurface->mPS, hrgn);
            else if( hrgn)
               lrc = OS2_CombineClipRegion( mSurface->mPS, hrgn, CRGN_OR);
            break;
         }
         default:
            // compiler informational...
            NS_ASSERTION( 0, "illegal clip combination");
            break;
      }
   }

   aClipEmpty = (lrc == RGN_NULL) ? PR_TRUE : PR_FALSE;

   return NS_OK;
}

// rc is whether there is a cliprect to return
// !! Potential problems here: I think we ought to return the rect
// !! transformed by the inverse of our xformation matrix.  Which,
// !! frankly, I can't be bothered to write the code for - it ought
// !! to be a method in nsTransform2D.
nsresult nsRenderingContextOS2::GetClipRect( nsRect &aRect, PRBool &aHasLocalClip)
{
   RECTL rcl;
   long rc = GFX (::GpiQueryClipBox (mSurface->mPS, &rcl), RGN_ERROR);

   PRBool brc = PR_FALSE;

   if( rc != RGN_NULL && rc != RGN_ERROR)
   {
      PM2NS_ININ( rcl, aRect);
      brc = PR_TRUE;
   }

   aHasLocalClip = brc;

   return NS_OK;
}

// Return PR_TRUE if clip region is now empty
nsresult nsRenderingContextOS2::SetClipRegion( const nsIRegion &aRegion, nsClipCombine aCombine, PRBool &aClipEmpty)
{
   nsRegionOS2 *pRegion = (nsRegionOS2 *) &aRegion;
   PRUint32     ulHeight;

   GetTargetHeight( ulHeight);

   HRGN hrgn = pRegion->GetHRGN( ulHeight, mSurface->mPS);
   long cmode = 0;

   switch( aCombine)
   {
      case nsClipCombine_kIntersect:
         cmode = CRGN_AND;
         break;
      case nsClipCombine_kUnion:
         cmode = CRGN_OR;
         break;
      case nsClipCombine_kSubtract:
         cmode = CRGN_DIFF;
         break;
      case nsClipCombine_kReplace:
         cmode = CRGN_COPY;
         break;
      default:
         // Compiler informational...
         NS_ASSERTION( 0, "illegal clip combination");
         break;
   }

   long lrc = OS2_CombineClipRegion( mSurface->mPS, hrgn, cmode);

   aClipEmpty = (lrc == RGN_NULL) ? PR_TRUE : PR_FALSE;

   return NS_OK;
}

// Somewhat dubious & rather expensive
nsresult nsRenderingContextOS2::GetClipRegion( nsIRegion **aRegion)
{
   if( !aRegion)
      return NS_ERROR_NULL_POINTER;

   *aRegion = 0;

   nsRegionOS2 *pRegion = new nsRegionOS2;
   if (!pRegion)
     return NS_ERROR_OUT_OF_MEMORY;
   NS_ADDREF(pRegion);

   // Get current clip region
   HRGN hrgnClip = 0;

   GFX (::GpiSetClipRegion (mSurface->mPS, 0, &hrgnClip), RGN_ERROR);
   
   if( hrgnClip && hrgnClip != HRGN_ERROR)
   {
      // There was a clip region, so get it & init.
      HRGN hrgnDummy = 0;
      PRUint32 ulHeight;
      GetTargetHeight( ulHeight);
      pRegion->Init( hrgnClip, ulHeight, mSurface->mPS);
      GFX (::GpiSetClipRegion (mSurface->mPS, hrgnClip, &hrgnDummy), RGN_ERROR);
   }
   else
      pRegion->Init();

   *aRegion = pRegion;

   return NS_OK;
}

/**
 * Fills in |aRegion| with a copy of the current clip region.
 */
nsresult nsRenderingContextOS2::CopyClipRegion(nsIRegion &aRegion)
{
  HRGN hr = OS2_CopyClipRegion(mSurface->mPS);

  if (hr == HRGN_ERROR)
    return NS_ERROR_FAILURE;
  else
    return NS_OK;
}

// Setters & getters -------------------------------------------------------

nsresult nsRenderingContextOS2::GetDeviceContext( nsIDeviceContext *&aDeviceContext)
{
// NS_IF_RELEASE(aDeviceContext);
   aDeviceContext = mContext;
   NS_IF_ADDREF( mContext);
   return NS_OK;
}

nsresult nsRenderingContextOS2::SetColor( nscolor aColor)
{
   mColor = aColor;
   return NS_OK;
}

nsresult nsRenderingContextOS2::GetColor( nscolor &aColor) const
{
   aColor = mColor;
   return NS_OK;
}

nsresult nsRenderingContextOS2::SetLineStyle( nsLineStyle aLineStyle)
{
   mLineStyle = aLineStyle;
   return NS_OK;
}

nsresult nsRenderingContextOS2::GetLineStyle( nsLineStyle &aLineStyle)
{
   aLineStyle = mLineStyle;
   return NS_OK;
}

nsresult nsRenderingContextOS2::SetFont( const nsFont &aFont)
{
   NS_IF_RELEASE( mFontMetrics);
   mContext->GetMetricsFor( aFont, mFontMetrics);
   return NS_OK;
}

nsresult nsRenderingContextOS2::SetFont( nsIFontMetrics *aFontMetrics)
{
   NS_IF_RELEASE( mFontMetrics);
   mFontMetrics = aFontMetrics;
   NS_IF_ADDREF( mFontMetrics);
   return NS_OK;
}

nsresult nsRenderingContextOS2::GetFontMetrics( nsIFontMetrics*& aFontMetrics)
{
// Commented out because clients misuse the interface.
//
// NS_IF_RELEASE(aFontMetrics);
   aFontMetrics = mFontMetrics;
   NS_IF_ADDREF( mFontMetrics);
   return NS_OK;
}

// add the passed in translation to the current translation
nsresult nsRenderingContextOS2::Translate( nscoord aX, nscoord aY)
{
   mTMatrix.AddTranslation( (float) aX, (float) aY);
   return NS_OK;
}

// add the passed in scale to the current scale
nsresult nsRenderingContextOS2::Scale( float aSx, float aSy)
{
   mTMatrix.AddScale(aSx, aSy);
   return NS_OK;
}

nsresult nsRenderingContextOS2::GetCurrentTransform( nsTransform2D *&aTransform)
{
/* JSK0126 - Assign mTMatrix into aTransform, since aTransform is not a valid object yet */
   aTransform = &mTMatrix;

   /*
   NS_PRECONDITION(aTransform,"Null transform ptr");
   if( !aTransform)
      return NS_ERROR_NULL_POINTER;
   aTransform->SetMatrix( &mTMatrix);
   */
   return NS_OK;
}


// Drawing methods ---------------------------------------------------------

// !! this method could do with a rename...
void nsRenderingContextOS2::SetupDrawingColor( BOOL bForce)
{
   if( bForce || mColor != mCurrDrawingColor || !mAlreadySetDrawingColor)
   {
      AREABUNDLE areaBundle;
      LINEBUNDLE lineBundle;


      long gcolor = MK_RGB( mGammaTable[NS_GET_R(mColor)],
                            mGammaTable[NS_GET_G(mColor)],
                            mGammaTable[NS_GET_B(mColor)]);

      nsPaletteInfo palInfo;
      mContext->GetPaletteInfo(palInfo);

      long lColor;
      if (palInfo.palette)
         lColor = GFX (::GpiQueryColorIndex (mSurface->mPS, 0, gcolor), GPI_ALTERROR);
      else
         lColor = gcolor;

      long lLineFlags = LBB_COLOR;
      long lAreaFlags = ABB_COLOR;

      areaBundle.lColor = lColor;
      lineBundle.lColor = lColor;

      if (((nsDeviceContextOS2 *) mContext)->mPrintDC )
      {
         areaBundle.lBackColor = GFX (::GpiQueryColorIndex (mSurface->mPS, 0, CLR_BACKGROUND), GPI_ALTERROR);
         lineBundle.lBackColor = GFX (::GpiQueryColorIndex (mSurface->mPS, 0, CLR_BACKGROUND), GPI_ALTERROR);

         areaBundle.usMixMode     = FM_OVERPAINT;
         areaBundle.usBackMixMode = BM_OVERPAINT;


         lLineFlags = lLineFlags | LBB_BACK_COLOR ;
         lAreaFlags = lAreaFlags | ABB_BACK_COLOR | ABB_MIX_MODE | ABB_BACK_MIX_MODE;
      }

      GFX (::GpiSetAttrs (mSurface->mPS, PRIM_LINE,lLineFlags, 0, (PBUNDLE)&lineBundle), FALSE);
      GFX (::GpiSetAttrs (mSurface->mPS, PRIM_AREA,lAreaFlags, 0, (PBUNDLE)&areaBundle), FALSE);

      mCurrDrawingColor = mColor;
      mAlreadySetDrawingColor = PR_TRUE;
   }

   if( bForce || mLineStyle != mCurrLineStyle)
   {
      long ltype = 0;
      switch( mLineStyle)
      {
         case nsLineStyle_kNone:   ltype = LINETYPE_INVISIBLE; break;
         case nsLineStyle_kSolid:  ltype = LINETYPE_SOLID; break;
         case nsLineStyle_kDashed: ltype = LINETYPE_SHORTDASH; break;
         case nsLineStyle_kDotted: ltype = LINETYPE_DOT; break;
         default:
            NS_ASSERTION(0, "Unexpected line style");
            break;
      }
      GFX (::GpiSetLineType (mSurface->mPS, ltype), FALSE);
      mCurrLineStyle = mLineStyle;
   }
}

void nsRenderingContextOS2::SetupFontAndColor( BOOL bForce)
{
   if( bForce || mFontMetrics != mCurrFontMetrics)
   {
      // select font
      mCurrFontMetrics = mFontMetrics;
      if( mCurrFontMetrics)
         mSurface->SelectFont( mCurrFontMetrics);
   }

   if( bForce || mColor != mCurrTextColor || !mAlreadySetTextColor)
   {

      CHARBUNDLE cBundle;

      long gcolor = MK_RGB( mGammaTable[NS_GET_R(mColor)],
                            mGammaTable[NS_GET_G(mColor)],
                            mGammaTable[NS_GET_B(mColor)]);

      nsPaletteInfo palInfo;
      mContext->GetPaletteInfo(palInfo);

      if (palInfo.palette)
         cBundle.lColor = GFX (::GpiQueryColorIndex (mSurface->mPS, 0, gcolor), GPI_ALTERROR);
      else
         cBundle.lColor = gcolor;

      cBundle.usMixMode = FM_OVERPAINT;
      cBundle.usBackMixMode = BM_LEAVEALONE;

      GFX (::GpiSetAttrs (mSurface->mPS, PRIM_CHAR,
                          CBB_COLOR | CBB_MIX_MODE | CBB_BACK_MIX_MODE,
                          0, &cBundle),
           FALSE);

      mCurrTextColor = mColor;
      mAlreadySetTextColor = PR_TRUE;
   }
}

nsresult nsRenderingContextOS2::DrawLine( nscoord aX0, nscoord aY0, nscoord aX1, nscoord aY1)
{
   mTMatrix.TransformCoord( &aX0, &aY0);
   mTMatrix.TransformCoord( &aX1, &aY1);

   POINTL ptls[] = { { (long) aX0, (long) aY0 },
                     { (long) aX1, (long) aY1 } };
   NS2PM( ptls, 2);

   if (ptls[0].x > ptls[1].x)
      ptls[0].x--;
   else if (ptls[1].x > ptls[0].x)
      ptls[1].x--;

   if (ptls[0].y < ptls[1].y)
      ptls[0].y++;
   else if (ptls[1].y < ptls[0].y)
      ptls[1].y++;

   SetupDrawingColor();

   GFX (::GpiMove (mSurface->mPS, ptls), FALSE);
   GFX (::GpiLine (mSurface->mPS, ptls + 1), GPI_ERROR);

   return NS_OK;
}

nsresult nsRenderingContextOS2::DrawPolyline(const nsPoint aPoints[], PRInt32 aNumPoints)
{
   PMDrawPoly( aPoints, aNumPoints, PR_FALSE);
   return NS_OK;
}

nsresult nsRenderingContextOS2::DrawPolygon( const nsPoint aPoints[], PRInt32 aNumPoints)
{
   PMDrawPoly( aPoints, aNumPoints, PR_FALSE);
   return NS_OK;
}

nsresult nsRenderingContextOS2::FillPolygon( const nsPoint aPoints[], PRInt32 aNumPoints)
{
   PMDrawPoly( aPoints, aNumPoints, PR_TRUE);
   return NS_OK;
}

void nsRenderingContextOS2::PMDrawPoly( const nsPoint aPoints[], PRInt32 aNumPoints, PRBool bFilled)
{
   if( aNumPoints > 1)
   {
      // Xform coords
      POINTL  aptls[ 20];
      PPOINTL pts = aptls;

      if( aNumPoints > 20)
         pts = new POINTL [ aNumPoints];

      PPOINTL pp = pts;
      const nsPoint *np = &aPoints[0];

      for( PRInt32 i = 0; i < aNumPoints; i++, pp++, np++)
      {
         pp->x = np->x;
         pp->y = np->y;
         mTMatrix.TransformCoord( (int*)&pp->x, (int*)&pp->y);
      }

      // go to os2
      NS2PM( pts, aNumPoints);

      SetupDrawingColor();

      // We draw closed pgons using polyline to avoid filling it.  This works
      // because the API to this class specifies that the last point must
      // be the same as the first one...

      GFX (::GpiMove (mSurface->mPS, pts), FALSE);

      if( bFilled == PR_TRUE)
      {
         POLYGON pgon = { aNumPoints - 1, pts + 1 };
         //IBM-AKR changed from boundary and inclusive to be noboundary and 
         //        exclusive to fix bug with text fields, buttons, etc. borders 
         //        being 1 pel too thick.  Bug 56853
         GFX (::GpiPolygons (mSurface->mPS, 1, &pgon, POLYGON_NOBOUNDARY, POLYGON_EXCL), GPI_ERROR);
      }
      else
      {
         GFX (::GpiPolyLine (mSurface->mPS, aNumPoints - 1, pts + 1), GPI_ERROR);
      }

      if( aNumPoints > 20)
         delete [] pts;
   }
}

nsresult nsRenderingContextOS2::DrawRect( const nsRect& aRect)
{
   nsRect tr = aRect;
   PMDrawRect( tr, FALSE);
   return NS_OK;
}

nsresult nsRenderingContextOS2::DrawRect( nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight)
{
   nsRect tr( aX, aY, aWidth, aHeight);
   PMDrawRect( tr, FALSE);
   return NS_OK;
}

nsresult nsRenderingContextOS2::FillRect( const nsRect& aRect)
{
   nsRect tr = aRect;
   PMDrawRect( tr, TRUE);
   return NS_OK;
}

nsresult nsRenderingContextOS2::FillRect( nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight)
{
   nsRect tr( aX, aY, aWidth, aHeight);
   PMDrawRect( tr, TRUE);
   return NS_OK;
}

NS_IMETHODIMP
nsRenderingContextOS2 :: InvertRect(const nsRect& aRect)
{
  return InvertRect(aRect.x, aRect.y, aRect.width, aRect.height);
}

NS_IMETHODIMP
nsRenderingContextOS2 :: InvertRect(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight)
{
//  mTMatrix.TransformCoord(&aX, &aY, &aWidth, &aHeight);
//  ConditionRect(aX, aY, aWidth, aHeight);
  nsRect tr(aX, aY, aWidth, aHeight);
  GFX (::GpiSetMix (mSurface->mPS, FM_XOR), FALSE);
  PMDrawRect(tr, FALSE);
  GFX (::GpiSetMix (mSurface->mPS, FM_DEFAULT), FALSE);
  return NS_OK;
}

void nsRenderingContextOS2::PMDrawRect( nsRect &rect, BOOL fill)
{
   mTMatrix.TransformCoord( &rect.x, &rect.y, &rect.width, &rect.height);

   RECTL rcl;
   NS2PM_ININ( rect, rcl);

   SetupDrawingColor();

   GFX (::GpiMove (mSurface->mPS, (PPOINTL) &rcl), FALSE);

   if (rcl.xLeft == rcl.xRight || rcl.yTop == rcl.yBottom)
   {
      GFX (::GpiLine (mSurface->mPS, ((PPOINTL)&rcl) + 1), GPI_ERROR);
   }
   else 
   {
      long lOps = (fill) ? DRO_FILL : DRO_OUTLINE;
   
      GFX (::GpiBox (mSurface->mPS, lOps, ((PPOINTL)&rcl) + 1, 0, 0), GPI_ERROR);
   }
}

// Arc-drawing methods, all proxy on to PMDrawArc
nsresult nsRenderingContextOS2::DrawEllipse( const nsRect& aRect)
{
   nsRect tRect( aRect);
   PMDrawArc( tRect, PR_FALSE, PR_TRUE);
   return NS_OK;
}

nsresult nsRenderingContextOS2::DrawEllipse( nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight)
{
   nsRect tRect( aX, aY, aWidth, aHeight);
   PMDrawArc( tRect, PR_FALSE, PR_TRUE);
   return NS_OK;
}

nsresult nsRenderingContextOS2::FillEllipse( const nsRect& aRect)
{
   nsRect tRect( aRect);
   PMDrawArc( tRect, PR_TRUE, PR_TRUE);
   return NS_OK;
}

nsresult nsRenderingContextOS2::FillEllipse( nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight)
{
   nsRect tRect( aX, aY, aWidth, aHeight);
   PMDrawArc( tRect, PR_TRUE, PR_TRUE);
   return NS_OK;
}

nsresult nsRenderingContextOS2::DrawArc( const nsRect& aRect,
                                         float aStartAngle, float aEndAngle)
{
   nsRect tRect( aRect);
   PMDrawArc( tRect, PR_FALSE, PR_FALSE, aStartAngle, aEndAngle);
   return NS_OK;
}

nsresult nsRenderingContextOS2::DrawArc( nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight,
                                         float aStartAngle, float aEndAngle)
{
   nsRect tRect( aX, aY, aWidth, aHeight);
   PMDrawArc( tRect, PR_FALSE, PR_FALSE, aStartAngle, aEndAngle);
   return NS_OK;
}

nsresult nsRenderingContextOS2::FillArc( const nsRect& aRect,
                                         float aStartAngle, float aEndAngle)
{
   nsRect tRect( aRect);
   PMDrawArc( tRect, PR_TRUE, PR_FALSE, aStartAngle, aEndAngle);
   return NS_OK;
}

nsresult nsRenderingContextOS2::FillArc( nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight,
                                         float aStartAngle, float aEndAngle)
{
   nsRect tRect( aX, aY, aWidth, aHeight);
   PMDrawArc( tRect, PR_TRUE, PR_FALSE, aStartAngle, aEndAngle);
   return NS_OK;
}

void nsRenderingContextOS2::PMDrawArc( nsRect &rect, PRBool bFilled, PRBool bFull,
                                       float start, float end)
{
   // convert coords
   mTMatrix.TransformCoord( &rect.x, &rect.y, &rect.width, &rect.height);

   RECTL rcl;
   NS2PM_ININ( rect, rcl);

   SetupDrawingColor();

   // set arc params.
   long lWidth = rect.width / 2;
   long lHeight = rect.height / 2;
   ARCPARAMS arcparams = { lWidth, lHeight, 0, 0 };
   GFX (::GpiSetArcParams (mSurface->mPS, &arcparams), FALSE);

   // move to center
   rcl.xLeft += lWidth;
   rcl.yBottom += lHeight;
   GFX (::GpiMove (mSurface->mPS, (PPOINTL)&rcl), FALSE);

   if (bFull)
   {
      long lOps = (bFilled) ? DRO_FILL : DRO_OUTLINE;

      // draw ellipse
      GFX (::GpiFullArc (mSurface->mPS, lOps, MAKEFIXED(1,0)), GPI_ERROR);
   }
   else
   {
      FIXED StartAngle = (FIXED)(start * 65536.0) % MAKEFIXED (360, 0);
      FIXED EndAngle   = (FIXED)(end * 65536.0) % MAKEFIXED (360, 0);
      FIXED SweepAngle = EndAngle - StartAngle;

      if (SweepAngle < 0) SweepAngle += MAKEFIXED (360, 0);

      // draw an arc or a pie
      if (bFilled)
      {
         GFX (::GpiBeginArea (mSurface->mPS, BA_NOBOUNDARY), FALSE);
         GFX (::GpiPartialArc (mSurface->mPS, (PPOINTL)&rcl, MAKEFIXED(1,0), StartAngle, SweepAngle), GPI_ERROR);
         GFX (::GpiEndArea (mSurface->mPS), GPI_ERROR);
      }
      else
      {
         // draw an invisible partialarc to get to the start of the arc.
         long lLineType = GFX (::GpiQueryLineType (mSurface->mPS), LINETYPE_ERROR);
         GFX (::GpiSetLineType (mSurface->mPS, LINETYPE_INVISIBLE), FALSE);
         GFX (::GpiPartialArc (mSurface->mPS, (PPOINTL)&rcl, MAKEFIXED(1,0), StartAngle, MAKEFIXED (0,0)), GPI_ERROR);
         // now draw a real arc
         GFX (::GpiSetLineType (mSurface->mPS, lLineType), FALSE);
         GFX (::GpiPartialArc (mSurface->mPS, (PPOINTL)&rcl, MAKEFIXED(1,0), StartAngle, SweepAngle), GPI_ERROR);
      }
   }
}

NS_IMETHODIMP nsRenderingContextOS2::GetHints(PRUint32& aResult)
{
  PRUint32 result = 0;

  aResult = result;

  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextOS2 :: GetWidth(char ch, nscoord& aWidth)
{
  char buf[1];
  buf[0] = ch;
  return GetWidth(buf, 1, aWidth);
}

NS_IMETHODIMP nsRenderingContextOS2 :: GetWidth(PRUnichar ch, nscoord &aWidth, PRInt32 *aFontID)
{
  PRUnichar buf[1];
  buf[0] = ch;
  return GetWidth(buf, 1, aWidth, aFontID);
}

NS_IMETHODIMP nsRenderingContextOS2 :: GetWidth(const char* aString, nscoord& aWidth)
{
  return GetWidth(aString, strlen(aString), aWidth);
}

NS_IMETHODIMP nsRenderingContextOS2 :: GetWidth(const char* aString,
                                                PRUint32 aLength,
                                                nscoord& aWidth)
{
  if (nsnull != mFontMetrics)
  {
    // Check for the very common case of trying to get the width of a single
    // space.
    if ((1 == aLength) && (aString[0] == ' '))
    {
      nsFontMetricsOS2* fontMetricsOS2 = (nsFontMetricsOS2*)mFontMetrics;
      return fontMetricsOS2->GetSpaceWidth(aWidth);
    }

    SIZEL size;

    SetupFontAndColor();
    ::GetTextExtentPoint32(mSurface->mPS, aString, aLength, &size);
    aWidth = NSToCoordRound(float(size.cx) * mP2T);

    return NS_OK;
  }
  else
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsRenderingContextOS2::GetWidth(const char *aString,
                                PRInt32     aLength,
                                PRInt32     aAvailWidth,
                                PRInt32*    aBreaks,
                                PRInt32     aNumBreaks,
                                nscoord&    aWidth,
                                PRInt32&    aNumCharsFit,
                                PRInt32*    aFontID = nsnull)
{
  NS_PRECONDITION(aBreaks[aNumBreaks - 1] == aLength, "invalid break array");

  if (nsnull != mFontMetrics) {
    // If we need to back up this state represents the last place we could
    // break. We can use this to avoid remeasuring text
    struct PrevBreakState {
      PRInt32   mBreakIndex;
      nscoord   mWidth;    // accumulated width to this point

      PrevBreakState() {
        mBreakIndex = -1;  // not known (hasn't been computed)
        mWidth = 0;
      }
    };

    // Initialize OUT parameter
    aNumCharsFit = 0;

    // Setup the font and foreground color
    SetupFontAndColor();

    // Iterate each character in the string and determine which font to use
    nsFontMetricsOS2* metrics = (nsFontMetricsOS2*)mFontMetrics;
    PrevBreakState    prevBreakState;
    nscoord           width = 0;
    PRInt32           start = 0;
    nscoord           aveCharWidth;
    metrics->GetAveCharWidth(aveCharWidth);

    while (start < aLength) {
      // Estimate how many characters will fit. Do that by diving the available
      // space by the average character width. Make sure the estimated number
      // of characters is at least 1
      PRInt32 estimatedNumChars = 0;
      if (aveCharWidth > 0) {
        estimatedNumChars = (aAvailWidth - width) / aveCharWidth;
      }
      if (estimatedNumChars < 1) {
        estimatedNumChars = 1;
      }

      // Find the nearest break offset
      PRInt32 estimatedBreakOffset = start + estimatedNumChars;
      PRInt32 breakIndex;
      nscoord numChars;

      // Find the nearest place to break that is less than or equal to
      // the estimated break offset
      if (aLength < estimatedBreakOffset) {
        // All the characters should fit
        numChars = aLength - start;
        breakIndex = aNumBreaks - 1;

      } else {
        breakIndex = prevBreakState.mBreakIndex;
        while (((breakIndex + 1) < aNumBreaks) &&
               (aBreaks[breakIndex + 1] <= estimatedBreakOffset)) {
          breakIndex++;
        }
        if (breakIndex == prevBreakState.mBreakIndex) {
          breakIndex++; // make sure we advanced past the previous break index
        }
        numChars = aBreaks[breakIndex] - start;
      }

      // Measure the text
      nscoord twWidth;
      if ((1 == numChars) && (aString[start] == ' ')) {
        metrics->GetSpaceWidth(twWidth);

      } else {
        SIZEL size;
        ::GetTextExtentPoint32(mSurface->mPS, &aString[start], numChars, &size);
        twWidth = NSToCoordRound(float(size.cx) * mP2T);
      }

      // See if the text fits
      PRBool  textFits = (twWidth + width) <= aAvailWidth;

      // If the text fits then update the width and the number of
      // characters that fit
      if (textFits) {
        aNumCharsFit += numChars;
        width += twWidth;
        start += numChars;

        // This is a good spot to back up to if we need to so remember
        // this state
        prevBreakState.mBreakIndex = breakIndex;
        prevBreakState.mWidth = width;

      } else {
        // See if we can just back up to the previous saved state and not
        // have to measure any text
        if (prevBreakState.mBreakIndex > 0) {
          // If the previous break index is just before the current break index
          // then we can use it
          if (prevBreakState.mBreakIndex == (breakIndex - 1)) {
            aNumCharsFit = aBreaks[prevBreakState.mBreakIndex];
            width = prevBreakState.mWidth;
            break;
          }
        }
          
        // We can't just revert to the previous break state
        if (0 == breakIndex) {
          // There's no place to back up to so even though the text doesn't fit
          // return it anyway
          aNumCharsFit += numChars;
          width += twWidth;
          break;
        }

        // Repeatedly back up until we get to where the text fits or we're all
        // the way back to the first word
        width += twWidth;
        while ((breakIndex >= 1) && (width > aAvailWidth)) {
          start = aBreaks[breakIndex - 1];
          numChars = aBreaks[breakIndex] - start;
          
          if ((1 == numChars) && (aString[start] == ' ')) {
            metrics->GetSpaceWidth(twWidth);

          } else {
            SIZEL size;
            ::GetTextExtentPoint32(mSurface->mPS, &aString[start], numChars, &size);
            twWidth = NSToCoordRound(float(size.cx) * mP2T);
          }

          width -= twWidth;
          aNumCharsFit = start;
          breakIndex--;
        }
        break;
      }
    }

    aWidth = width;
    return NS_OK;
  }

  return NS_ERROR_FAILURE;
}


NS_IMETHODIMP nsRenderingContextOS2::GetWidth( const nsString &aString,
                                               nscoord &aWidth,
                                               PRInt32 *aFontID)
{
   return GetWidth( aString.GetUnicode(), aString.Length(), aWidth, aFontID);
}

NS_IMETHODIMP nsRenderingContextOS2::GetWidth( const PRUnichar *aString,
                                               PRUint32 aLength,
                                               nscoord &aWidth,
                                               PRInt32 *aFontID)
{
  nsresult temp;
  char buf[1024];

  int newLength = WideCharToMultiByte( ((nsFontMetricsOS2*)mFontMetrics)->mCodePage, aString, aLength, buf, sizeof(buf));
  temp = GetWidth( buf, newLength, aWidth);
  return temp;
}

NS_IMETHODIMP nsRenderingContextOS2 :: DrawString(const char *aString, PRUint32 aLength,
                                                  nscoord aX, nscoord aY,
                                                  const nscoord* aSpacing)
{
  NS_PRECONDITION(mFontMetrics,"Something is wrong somewhere");

  // Take care of ascent and specifies the drawing on the baseline
  nscoord ascent;
  mFontMetrics->GetMaxAscent(ascent);
  aY += ascent;

  PRInt32 x = aX;
  PRInt32 y = aY;

  SetupFontAndColor();

  INT dxMem[500];
  INT* dx0;
  if (nsnull != aSpacing) {
    dx0 = dxMem;
    if (aLength > 500) {
      dx0 = new INT[aLength];
    }
    mTMatrix.ScaleXCoords(aSpacing, aLength, dx0);
  }
  mTMatrix.TransformCoord(&x, &y);

  POINTL ptl = { x, y };
  NS2PM( &ptl, 1);

  ::ExtTextOut(mSurface->mPS, ptl.x, ptl.y, 0, NULL, aString, aLength, aSpacing ? dx0 : NULL);

  if ((nsnull != aSpacing) && (dx0 != dxMem)) {
    delete [] dx0;
  }

  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextOS2 :: DrawString(const PRUnichar *aString, PRUint32 aLength,
                                                  nscoord aX, nscoord aY,
                                                  PRInt32 aFontID,
                                                  const nscoord* aSpacing)
{
  char buf[1024];

  int newLength = WideCharToMultiByte( ((nsFontMetricsOS2*)mFontMetrics)->mCodePage, aString, aLength, buf, sizeof(buf));

  return DrawString( buf, newLength, aX, aY, aSpacing);
}

NS_IMETHODIMP nsRenderingContextOS2 :: DrawString(const nsString& aString,
                                                  nscoord aX, nscoord aY,
                                                  PRInt32 aFontID,
                                                  const nscoord* aSpacing)
{
  return DrawString(aString.GetUnicode(), aString.Length(), aX, aY, aFontID, aSpacing);
}


// Image drawing: just proxy on to the image object, so no worries yet.
nsresult nsRenderingContextOS2::DrawImage( nsIImage *aImage, nscoord aX, nscoord aY)
{
   nscoord width, height;

   width = NSToCoordRound( mP2T * aImage->GetWidth());
   height = NSToCoordRound( mP2T * aImage->GetHeight());

   return this->DrawImage( aImage, aX, aY, width, height);
}

nsresult nsRenderingContextOS2::DrawImage( nsIImage *aImage, nscoord aX, nscoord aY,
                                           nscoord aWidth, nscoord aHeight)
{
   nsRect  tr;

   tr.x = aX;
   tr.y = aY;
   tr.width = aWidth;
   tr.height = aHeight;

   return this->DrawImage( aImage, tr);
}

nsresult nsRenderingContextOS2::DrawImage( nsIImage *aImage, const nsRect& aSRect, const nsRect& aDRect)
{
   nsRect sr,dr;

   sr = aSRect;
   mTMatrix.TransformCoord( &sr.x, &sr.y, &sr.width, &sr.height);

   dr = aDRect;
   mTMatrix.TransformCoord( &dr.x, &dr.y, &dr.width, &dr.height);

   return aImage->Draw( *this, mSurface, sr.x, sr.y, sr.width, sr.height, dr.x, dr.y, dr.width, dr.height);
}

nsresult nsRenderingContextOS2::DrawImage( nsIImage *aImage, const nsRect& aRect)
{
   nsRect tr( aRect);
   mTMatrix.TransformCoord( &tr.x, &tr.y, &tr.width, &tr.height);

   return aImage->Draw( *this, mSurface, tr.x, tr.y, tr.width, tr.height);
}

/** ---------------------------------------------------
 *  See documentation in nsIRenderingContext.h
 *	@update 3/16/00 dwc
 */
NS_IMETHODIMP
nsRenderingContextOS2::DrawTile(nsIImage *aImage,nscoord aX0,nscoord aY0,nscoord aX1,nscoord aY1,
                                                    nscoord aWidth,nscoord aHeight)
{
  nscoord     orgX,orgY,orgWidth,orgHeight;
  PRBool      didtile = FALSE;

  // convert output platform, but no translation.. just scale
  orgX = aX0;
  orgY = aY0;
  orgWidth = aX1 - aX0;
  orgHeight = aY1 - aY0;
  mTMatrix.TransformCoord(&aX0,&aY0,&aWidth,&aHeight);
  mTMatrix.TransformCoord(&orgX,&orgY,&orgWidth,&orgHeight);
  aX1 = aX0 + orgWidth;
  aY1 = aY0 + orgHeight;

//  if ( PR_TRUE==CanTile(aWidth,aHeight) ) {    
//    didtile = ((nsImageOS2*)aImage)->PatBltTile(*this,mSurface,aX0,aY0,aX1,aY1,aWidth,aHeight);
//  }
      
//  if (PR_FALSE ==didtile){
    // rely on the slower tiler supported in nsRenderingContextWin.. don't have 
    // to use xplatform which is really slow (slowest is the only one that supports transparency
    didtile = ((nsImageOS2*)aImage)->DrawTile(*this,mSurface,aX0,aY0,aX1,aY1,aWidth,aHeight);
//  }

  return NS_OK;
}


nsresult nsRenderingContextOS2::CopyOffScreenBits(
                     nsDrawingSurface aSrcSurf, PRInt32 aSrcX, PRInt32 aSrcY,
                             const nsRect &aDestBounds, PRUint32 aCopyFlags)
{
   NS_ASSERTION( aSrcSurf && mSurface && mFrontSurface, "bad surfaces");

   nsDrawingSurfaceOS2 *theSurf = (nsDrawingSurfaceOS2 *) aSrcSurf;

   nsRect drect( aDestBounds);
   HPS    hpsTarget = mFrontSurface->mPS;

   if( aCopyFlags & NS_COPYBITS_TO_BACK_BUFFER)
      hpsTarget = mSurface->mPS;

// When the bitmap-tiling code calls us, we must respect the SOURCE_CLIP_REGION
// flag (ie. not do it).  But we must put it on for the compositor.
//
// Finding out *why* this is so is a definite task.
//
#if 0
   // this is what we ought to do!
   if( aCopyFlags & NS_COPYBITS_USE_SOURCE_CLIP_REGION)
#else
   // compositor doesn't use this flag
// if( !(aCopyFlags & NS_COPYBITS_TO_BACK_BUFFER))
#endif
   {
      // Set clip region on dest surface to be that from the hps
      // in the passed-in drawing surface.
      OS2_SetClipRegion( hpsTarget, OS2_CopyClipRegion( theSurf->mPS));
   }

   // Windows wants to select palettes here.  I don't think I do.

   if( aCopyFlags & NS_COPYBITS_XFORM_SOURCE_VALUES)
      mTMatrix.TransformCoord( &aSrcX, &aSrcY);

   if( aCopyFlags & NS_COPYBITS_XFORM_DEST_VALUES)
      mTMatrix.TransformCoord( &drect.x, &drect.y,
                               &drect.width, &drect.height);

   // Note rects for GpiBitBlt are in-ex.
   // Doing coord-conversion by hand to avoid calling GetClientBounds() twice.
   RECTL   rcls[ 2];
   PRUint32 ulHeight, ulDummy;

   GetTargetHeight( ulHeight);

   rcls[0].xLeft = drect.x;
   rcls[0].xRight = drect.x + drect.width;
   rcls[0].yTop = ulHeight - drect.y;
   rcls[0].yBottom = rcls[0].yTop - drect.height;

   if( aCopyFlags & NS_COPYBITS_TO_BACK_BUFFER)
      theSurf->GetDimensions( &ulDummy, &ulHeight);

   rcls[1].xLeft = aSrcX;
   rcls[1].yBottom = ulHeight - aSrcY - drect.height;

   GFX (::GpiBitBlt (hpsTarget, theSurf->mPS, 3, (PPOINTL)rcls, ROP_SRCCOPY, BBO_OR), GPI_ERROR);

   return NS_OK;
}

nsresult nsRenderingContextOS2::RetrieveCurrentNativeGraphicData(PRUint32* ngd)
{
  if(ngd != nsnull)
    *ngd = (PRUint32)mDC;
  return NS_OK;
}

// Clip region helper functions --------------------------------------------

/* hrgnCombine becomes the clip region.  Any current clip region is       */
/* dealt with.  hrgnCombine may be NULLHANDLE.                            */
/* Return value is lComplexity.                                           */
/* lMode should be CRGN_*                                                 */
LONG OS2_CombineClipRegion( HPS hps, HRGN hrgnCombine, LONG lMode)
{
   if (!hps) return RGN_ERROR;

   HRGN hrgnClip = 0;
   LONG rc = RGN_NULL;

   GFX (::GpiSetClipRegion (hps, 0, &hrgnClip), RGN_ERROR); // Get the current clip region and deselect it

   if (hrgnClip && hrgnClip != HRGN_ERROR)
   {
      if (lMode != CRGN_COPY)    // If necessarry combine with previous clip region
         GFX (::GpiCombineRegion (hps, hrgnCombine, hrgnClip, hrgnCombine, lMode), RGN_ERROR);
      
      GFX (::GpiDestroyRegion (hps, hrgnClip), FALSE);
   }

   if (hrgnCombine)
      rc = GFX (::GpiSetClipRegion (hps, hrgnCombine, NULL), RGN_ERROR);  // Set new clip region

   return rc;
}

/* Return value is HRGN_ */
HRGN OS2_CopyClipRegion( HPS hps)
{
  if (!hps) return HRGN_ERROR;

  HRGN hrgn = 0, hrgnClip;

  // Get current clip region
  GFX (::GpiSetClipRegion (hps, 0, &hrgnClip), RGN_ERROR);

  if (hrgnClip && hrgnClip != HRGN_ERROR)
  {
     hrgn = GFX (::GpiCreateRegion (hps, 0, NULL), RGN_ERROR);     // Create empty region and combine with current
     GFX (::GpiCombineRegion (hps, hrgn, hrgnClip, 0, CRGN_COPY), RGN_ERROR);
     GFX (::GpiSetClipRegion (hps, hrgnClip, NULL), RGN_ERROR);    // restore current clip region
  }

  return hrgn;
}

// Keep coordinate within 32-bit limits
NS_IMETHODIMP nsRenderingContextOS2::ConditionRect(nscoord &x, nscoord &y, nscoord &w, nscoord &h)
{
  if (y < -134217728)
    y = -134217728;

  if (y + h > 134217727)
    h  = 134217727 - y;

  if (x < -134217728)
    x = -134217728;

  if (x + w > 134217727)
    w  = 134217727 - x;

  return NS_OK;
}
