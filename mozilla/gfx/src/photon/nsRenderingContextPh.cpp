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

#include "nsRenderingContextPh.h"
#include "nsRegionPh.h"
#include <math.h>
#include "libimg.h"
#include "nsDeviceContextPh.h"
#include "nsIScriptGlobalObject.h"
#include "prprf.h"
#include "nsDrawingSurfacePh.h"
#include "nsGfxCIID.h"
#include <stdlib.h>

#include <mem.h>
#include <photon/PhRender.h>
#include <Pt.h>

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

static NS_DEFINE_IID(kIDOMRenderingContextIID, NS_IDOMRENDERINGCONTEXT_IID);
static NS_DEFINE_IID(kIRenderingContextIID, NS_IRENDERING_CONTEXT_IID);
static NS_DEFINE_IID(kIRenderingContextPhIID, NS_IRENDERING_CONTEXT_PH_IID);
static NS_DEFINE_IID(kIScriptObjectOwnerIID, NS_ISCRIPTOBJECTOWNER_IID);
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

// Macro for converting from nscolor to PtColor_t
// Photon RGB values are stored as 00 RR GG BB
// nscolor RGB values are 00 BB GG RR
#define NS_TO_PH_RGB(ns) (ns & 0xff) << 16 | (ns & 0xff00) | ((ns >> 16) & 0xff)
#define PH_TO_NS_RGB(ns) (ns & 0xff) << 16 | (ns & 0xff00) | ((ns >> 16) & 0xff)

/* This global int turns on/off double buffering, there is probably */
/* a better way but not sure what it is.. */
int double_buffer = 1;
#include <prlog.h>
PRLogModuleInfo *PhGfxLog = NULL;
unsigned char   PhGfxLogState = 0;		/* 0 == Not Enabled */
#include "nsPhGfxLog.h"

#ifdef DEBUG_kirkj 
#define ENABLE_PHOTON_FLUXING
#endif

class GraphicsState
{
public:
  GraphicsState();
  ~GraphicsState();

  GraphicsState   *mNext;

  /* Members of nsRenderingContextPh object */

//PhGC_t            *mGC;
  nscolor		     mCurrentColor;
  nsTransform2D     *mMatrix;		// transform that all the graphics drawn here will obey
  nsIFontMetrics	*mFontMetrics;
  nsRegionPh	    *mClipRegion;
};


GraphicsState :: GraphicsState()
{
  mNext = nsnull;

//  mGC = nsnull;
  mCurrentColor = NS_RGB(0, 0, 0);
  mMatrix = nsnull;
  mFontMetrics = nsnull;
  mClipRegion = nsnull;
}


GraphicsState :: ~GraphicsState()
{
}


#define NOT_SETUP 0x33

PhGC_t *nsRenderingContextPh::mPtGC = nsnull;

nsRenderingContextPh :: nsRenderingContextPh()
{
  NS_INIT_REFCNT();
  
//  mholdGC = nsnull;
  mGC = nsnull;
  mTMatrix = new nsTransform2D();
  mRegion = new nsRegionPh();
  mRegion->Init();
  mFontMetrics = nsnull;
  mSurface = nsnull;
  mMainSurface = nsnull;
  mDCOwner = nsnull;
  mContext = nsnull;
  mP2T = 1.0f;
  mWidget = nsnull;
  mPhotonFontName = nsnull;

  //default objects
  //state management

  mStates = nsnull;
  mStateCache = new nsVoidArray();
  mCurrFontMetrics = nsnull;
  mGammaTable = nsnull;

#ifdef NS_DEBUG
  mInitialized = PR_FALSE;
#endif

  mScriptObject = nsnull;
  
  if( mPtGC == nsnull )
    mPtGC = PgGetGC();
}


nsRenderingContextPh :: ~nsRenderingContextPh()
{
  PR_LOG(PhGfxLog, PR_LOG_DEBUG, ("nsRenderingContextPh(%p) destructor, mGC = %p\n", this, mGC ));

  if (mContext)
    NS_IF_RELEASE(mContext);

  if (mFontMetrics)
    NS_IF_RELEASE(mFontMetrics);

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

  if (mTMatrix)
    delete mTMatrix;

  if (!mSurface)
  {
    if( mGC )
    {
//      PgDestroyGC( mGC );

      PgSetGC( mPtGC );
      PgSetRegion( mPtGC->rid );

      PgDestroyGC( mGC );
      mGC = nsnull;
    }
  }
  else
  {
//    PmMemStop( (PmMemoryContext_t *)mGC);
  }

  if (mRegion)
  {
    delete mRegion;
  }

  if( mWidget )
  {
#ifdef ENABLE_PHOTON_FLUXING
    PtEndFlux( PtFindDisjoint( mWidget ));
#endif
//    PtContainerRelease(PtFindDisjoint( mWidget ));
//    PtFlush();
  }
  PgSetGC( mPtGC );
  PgSetRegion( mPtGC->rid );
}


nsresult nsRenderingContextPh :: QueryInterface(REFNSIID aIID, void** aInstancePtr)
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

  if (aIID.Equals(kIRenderingContextPhIID))
  {
    nsIRenderingContextPh* tmp = this;
    *aInstancePtr = (void*) tmp;
    NS_ADDREF_THIS();
    return NS_OK;
  }

  if (aIID.Equals(kIScriptObjectOwnerIID))
  {
    nsIScriptObjectOwner* tmp = this;
    *aInstancePtr = (void*) tmp;
    NS_ADDREF_THIS();
    return NS_OK;
  }

  if (aIID.Equals(kIDOMRenderingContextIID))
  {
    nsIDOMRenderingContext* tmp = this;
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


NS_IMPL_ADDREF(nsRenderingContextPh)
NS_IMPL_RELEASE(nsRenderingContextPh)


NS_IMETHODIMP nsRenderingContextPh :: Init(nsIDeviceContext* aContext,
                              nsIWidget *aWindow)
{
  PR_LOG(PhGfxLog, PR_LOG_DEBUG, ("nsRenderingContextPh::Init with a widget\n"));
  NS_PRECONDITION(PR_FALSE == mInitialized, "double init");

  mContext = aContext;
  NS_IF_ADDREF(mContext);

  mWidget = (PtWidget_t*) aWindow->GetNativeData( NS_NATIVE_WIDGET );

  if(!mWidget)
  {
    printf( "nsRenderingContext::Init (with a widget) mWidget is NULL!\n" );
    fflush( stdout );
    return NS_ERROR_FAILURE;
  }

  PhRid_t    rid = PtWidgetRid( mWidget );
//  PhRid_t    rid = PtWidgetRid( PtFindDisjoint(mWidget) );

  if( !rid )
    PR_LOG(PhGfxLog, PR_LOG_DEBUG, ("Widget (%p) does not have a Rid!\n", mWidget ));

  mGC = PgCreateGC( 4096 );

  if( !mGC )
    PR_LOG(PhGfxLog, PR_LOG_DEBUG, ("PgCreateGC() failed!\n" ));

  PgSetGC( mGC );
  PgDefaultGC( mGC );
  PgSetRegion( rid );

//  PgSetGC( mPtGC );
//  PgSetRegion( mPtGC->rid );

  mSurface = new nsDrawingSurfacePh();
  mSurface->Init(mGC);
//  mSurface->Init(mGC,640,480,0);
//  mSurface->Select();
//  ApplyClipping(mSurface->GetGC()->rid);
  mOffscreenSurface = mSurface;

  NS_IF_ADDREF(aWindow);
  NS_ADDREF(mSurface);
  return (CommonInit());
}

NS_IMETHODIMP nsRenderingContextPh::CommonInit()
{
  float app2dev;

  mContext->GetAppUnitsToDevUnits(app2dev);
  mTMatrix->AddScale(app2dev,app2dev);
  mContext->GetDevUnitsToAppUnits(mP2T);
  
  mContext->GetGammaTable(mGammaTable);

  return NS_OK;
}


NS_IMETHODIMP nsRenderingContextPh :: Init(nsIDeviceContext* aContext,
                              nsDrawingSurface aSurface)
{
printf ("kedl: init with a surface!!!!\n");
PR_LOG(PhGfxLog, PR_LOG_DEBUG, ("nsRenderingContextPh::Init with a Drawing Surface\n"));

  NS_PRECONDITION(PR_FALSE == mInitialized, "double init");

  mContext = aContext;
  NS_IF_ADDREF(mContext);

//  mGC = PgCreateGC( 0 );
//  PgDefaultGC( mGC );

  mSurface = (nsDrawingSurfacePh *)aSurface;
//  NS_ADDREF(mSurface);

//  printf( "abs clip = not set from surface!\n" );
//  PgSetClipping( 0, NULL );
//  PgClearTranslation();

  return (CommonInit());
}


NS_IMETHODIMP nsRenderingContextPh :: LockDrawingSurface(PRInt32 aX, PRInt32 aY,
                                                          PRUint32 aWidth, PRUint32 aHeight,
                                                          void **aBits, PRInt32 *aStride,
                                                          PRInt32 *aWidthBytes, PRUint32 aFlags)
{
  PR_LOG(PhGfxLog, PR_LOG_DEBUG, ("nsRenderingContextPh::LockDrawingSurface  - Not Implemented\n"));

  return NS_OK;
}


NS_IMETHODIMP nsRenderingContextPh :: UnlockDrawingSurface(void)
{
  PR_LOG(PhGfxLog, PR_LOG_DEBUG, ("nsRenderingContextPh::UnLockDrawingSurface  - Not Implemented\n"));
  return NS_OK;
}


NS_IMETHODIMP nsRenderingContextPh :: SelectOffScreenDrawingSurface(nsDrawingSurface aSurface)
{
  PR_LOG(PhGfxLog, PR_LOG_DEBUG, ("nsRenderingContextPh::SelectOffScreenDrawingSurface\n"));

//  printf ("kedl: surface select: %lu\n",aSurface);
  if (nsnull==aSurface)
    mSurface = mOffscreenSurface;
  else
    mSurface = (nsDrawingSurfacePh *) aSurface;

//  printf ("kedl2: select pixmap %p\n", ((nsDrawingSurfacePh *)mSurface)->mPixmap);
  mSurface->Select();

  PgSetClipping( 0, NULL );
  PgSetFillColor(Pg_BLACK);
  PgDrawIRect( 0, 0, 640,480, Pg_DRAW_FILL_STROKE );

  ApplyClipping(mSurface->GetGC()->rid);
  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextPh :: GetDrawingSurface(nsDrawingSurface *aSurface)
{
  PR_LOG(PhGfxLog, PR_LOG_DEBUG, ("nsRenderingContextPh::GetDrawingSurface  - Not Implemented\n"));
  return NS_OK;
}


NS_IMETHODIMP nsRenderingContextPh :: GetHints(PRUint32& aResult)
{
  PR_LOG(PhGfxLog, PR_LOG_DEBUG, ("nsRenderingContextPh::GetHints\n"));

  /* this flag indicates that the system prefers 8bit chars over wide chars */
  /* It may or may not be faster under photon... */
  
  aResult = NS_RENDERING_HINT_FAST_8BIT_TEXT;

  return NS_OK;
}


NS_IMETHODIMP nsRenderingContextPh :: Reset()
{
  PR_LOG(PhGfxLog, PR_LOG_DEBUG, ("nsRenderingContextPh::Reset  - Not Implemented\n"));
  return NS_OK;
}


NS_IMETHODIMP nsRenderingContextPh :: GetDeviceContext(nsIDeviceContext *&aContext)
{
//  PR_LOG(PhGfxLog, PR_LOG_DEBUG, ("nsRenderingContextPh::GetDeviceContext\n"));

  NS_IF_ADDREF( mContext );
  aContext = mContext;
  return NS_OK;
}


NS_IMETHODIMP nsRenderingContextPh :: PushState(void)
{
  PR_LOG(PhGfxLog, PR_LOG_DEBUG, ("nsRenderingContextPh::PushState\n"));

  GraphicsState * state = new GraphicsState();

  mStateCache->AppendElement(state);

  // Save current settings to new state...

  //state->mGC = mGC;
  state->mCurrentColor = mCurrentColor;
  state->mMatrix = mTMatrix;
  state->mFontMetrics = mFontMetrics;
  NS_IF_ADDREF( state->mFontMetrics );
  state->mClipRegion = mRegion;
  
  /* if the mRegion is not empty make a copy */
  if (mRegion != nsnull)
  {
    mRegion = new nsRegionPh();
    mRegion->Init();
    mRegion->SetTo(*state->mClipRegion);  
  }
    
  // Make new objects so we dont change the saved ones...
  // Can't make a new FontMetrics since there is no copy constructor

  mTMatrix = new nsTransform2D(mTMatrix);

  return NS_OK;
}


NS_IMETHODIMP nsRenderingContextPh :: PopState( PRBool &aClipEmpty )
{
  PR_LOG(PhGfxLog, PR_LOG_DEBUG, ("nsRenderingContextPh::PopState\n"));

  PRUint32 cnt = mStateCache->Count();
  PRBool bEmpty=PR_FALSE;
  
  if( cnt > 0)
  {
    GraphicsState * state = (GraphicsState *)mStateCache->ElementAt(cnt - 1);

    mStateCache->RemoveElementAt(cnt - 1);

    // Assign all local attributes from the state object just popped

    if( state->mCurrentColor != mCurrentColor )
      SetColor(state->mCurrentColor);

    if( mTMatrix )
      delete mTMatrix;
    mTMatrix = state->mMatrix;

    NS_IF_RELEASE( mFontMetrics );
    mFontMetrics = state->mFontMetrics;

    if (mRegion)
      delete mRegion;
	  
    mRegion = state->mClipRegion;
    if ((mRegion) && (mRegion->IsEmpty() == PR_TRUE))
    {
      bEmpty = PR_TRUE;
    }

    ApplyClipping(mGC->rid);

    // Delete this graphics state object
    delete state;
  }

  // REVISIT - fix when we get clipping working
  aClipEmpty = bEmpty;

  return NS_OK;
}


NS_IMETHODIMP nsRenderingContextPh :: IsVisibleRect(const nsRect& aRect, PRBool &aVisible)
{
  PR_LOG(PhGfxLog, PR_LOG_DEBUG, ("nsRenderingContextPh::IsVisibleRect - Not Implemented\n"));
  aVisible = PR_TRUE;
  return NS_OK;
}


NS_IMETHODIMP nsRenderingContextPh :: SetClipRect(const nsRect& aRect, nsClipCombine aCombine, PRBool &aClipEmpty)
{
  nsresult   res = NS_ERROR_FAILURE;
  nsRect     trect = aRect;
  PhRect_t  *rgn;

  PR_LOG(PhGfxLog, PR_LOG_DEBUG, ("nsRenderingContextPh::SetClipRect  (%ld,%ld,%ld,%ld)\n", aRect.x, aRect.y, aRect.width, aRect.height ));

  if ((mTMatrix) && (mRegion))
  {
    mTMatrix->TransformCoord(&trect.x, &trect.y,&trect.width, &trect.height);

    switch(aCombine)
    {
      case nsClipCombine_kIntersect:
        mRegion->Intersect(trect.x,trect.y,trect.width,trect.height);
        break;
      case nsClipCombine_kUnion:
        mRegion->Union(trect.x,trect.y,trect.width,trect.height);
        break;
      case nsClipCombine_kSubtract:
        mRegion->Subtract(trect.x,trect.y,trect.width,trect.height);
        break;
      case nsClipCombine_kReplace:
        mRegion->SetTo(trect.x,trect.y,trect.width,trect.height);
        break;
      default:
		PR_LOG(PhGfxLog, PR_LOG_ERROR, ("nsRenderingContextPh::SetClipRect  Unknown Combine type\n"));
  	    break;
     }

     aClipEmpty = mRegion->IsEmpty();
     ApplyClipping(mGC->rid);

// kirk    mRegion->GetNativeRegion((void*&)rgn);
// kirk    PR_LOG(PhGfxLog, PR_LOG_DEBUG, ("nsRenderingContextPh::SetClipRect Calling PgSetCliping (%ld,%ld,%ld,%ld)\n", rgn->ul.x, rgn->ul.y, rgn->lr.x, rgn->lr.y));
// kirk    PgSetClipping(1, rgn);
	 
    res = NS_OK;
  }
  else
  {
    PR_LOG(PhGfxLog, PR_LOG_ERROR, ("nsRenderingContextPh::SetClipRect  Invalid pointers!\n"));
  }
  																			
  return res;
}


NS_IMETHODIMP nsRenderingContextPh :: GetClipRect(nsRect &aRect, PRBool &aClipValid)
{
  PR_LOG(PhGfxLog, PR_LOG_DEBUG, ("nsRenderingContextPh::GetClipRect  - Not Implemented\n"));
  PRInt32 x, y, w, h;

  if (!mRegion->IsEmpty())
  {
    mRegion->GetBoundingBox(&x,&y,&w,&h);
    aRect.SetRect(x,y,w,h);
    aClipValid = PR_TRUE;
  }
  else
  {
    aRect.SetRect(0,0,0,0);
    aClipValid = PR_FALSE;
  }

  return NS_OK;
}


NS_IMETHODIMP nsRenderingContextPh :: SetClipRegion(const nsIRegion& aRegion, nsClipCombine aCombine, PRBool &aClipEmpty)
{
  PR_LOG(PhGfxLog, PR_LOG_DEBUG, ("nsRenderingContextPh::SetClipRegion  - Not Implemented\n"));

  switch(aCombine)
  {
  case nsClipCombine_kIntersect:
    mRegion->Intersect(aRegion);
    break;
  case nsClipCombine_kUnion:
    mRegion->Union(aRegion);
    break;
  case nsClipCombine_kSubtract:
    mRegion->Subtract(aRegion);
    break;
  case nsClipCombine_kReplace:
    mRegion->SetTo(aRegion);
    break;
  }

  aClipEmpty = mRegion->IsEmpty();
  ApplyClipping(mGC->rid);

  return NS_OK;
}


NS_IMETHODIMP nsRenderingContextPh :: GetClipRegion(nsIRegion **aRegion)
{
  PR_LOG(PhGfxLog, PR_LOG_DEBUG, ("nsRenderingContextPh::GetClipRegion  - Not Implemented\n"));
  nsresult  rv = NS_OK;
  
  NS_ASSERTION(!(nsnull == aRegion), "no region ptr");

  if (nsnull == *aRegion)
  {
   nsRegionPh *rgn = new nsRegionPh();
   if (nsnull != rgn)
   {
     NS_ADDREF(rgn);
     rv = rgn->Init();
     if (NS_OK == rv)
       *aRegion = rgn;
     else
       NS_RELEASE(rgn);
   }
   else
   {
     rv = NS_ERROR_OUT_OF_MEMORY;
   }
  }

  if (rv == NS_OK)
  {
    (*aRegion)->SetTo(*mRegion);
  }																				  

  return rv;
}


NS_IMETHODIMP nsRenderingContextPh :: SetColor(nscolor aColor)
{
  PR_LOG(PhGfxLog, PR_LOG_DEBUG, ("nsRenderingContextPh::SetColor (%i,%i,%i)\n", NS_GET_R(aColor), NS_GET_G(aColor), NS_GET_B(aColor) ));

  mCurrentColor = aColor;
  PgSetStrokeColor( NS_TO_PH_RGB( aColor ));
  PgSetFillColor( NS_TO_PH_RGB( aColor ));
  PgSetTextColor( NS_TO_PH_RGB( aColor ));

  return NS_OK;
}


NS_IMETHODIMP nsRenderingContextPh :: GetColor(nscolor &aColor) const
{
  PR_LOG(PhGfxLog, PR_LOG_DEBUG, ("nsRenderingContextPh::GetColor\n"));
  aColor = mCurrentColor;
  return NS_OK;
}


NS_IMETHODIMP nsRenderingContextPh :: SetLineStyle(nsLineStyle aLineStyle)
{
  PR_LOG(PhGfxLog, PR_LOG_DEBUG, ("nsRenderingContextPh::SetLineStyle  - Not Implemented\n"));
  return NS_OK;
}


NS_IMETHODIMP nsRenderingContextPh :: GetLineStyle(nsLineStyle &aLineStyle)
{
  PR_LOG(PhGfxLog, PR_LOG_DEBUG, ("nsRenderingContextPh::GetLineStyle  - Not Implemented\n"));
  return NS_OK;
}


NS_IMETHODIMP nsRenderingContextPh :: SetFont(const nsFont& aFont)
{
  PR_LOG(PhGfxLog, PR_LOG_DEBUG, ("nsRenderingContextPh::SetFont with nsFont\n"));

  if (mFontMetrics)
    NS_IF_RELEASE(mFontMetrics);

  if (mContext)
  {
    mContext->GetMetricsFor(aFont, mFontMetrics);
    return SetFont(mFontMetrics);
  }
  else
    return NS_ERROR_FAILURE;
}


NS_IMETHODIMP nsRenderingContextPh :: SetFont(nsIFontMetrics *aFontMetrics)
{
//  PR_LOG(PhGfxLog, PR_LOG_DEBUG, ("nsRenderingContextPh::SetFont with nsIFontMetrics mFontMetrics=<%p> aFontMetrics=<%p>\n", mFontMetrics, aFontMetrics));
	  
  nsFontHandle  fontHandle;			/* really a nsString */
  nsString      *pFontHandle;

  if (mFontMetrics)
    NS_IF_RELEASE(mFontMetrics);

//PR_LOG(PhGfxLog, PR_LOG_DEBUG, ("nsRenderingContextPh::SetFont after NS_IF_RELEASE(mFontMetrics)\n"));

  mFontMetrics = aFontMetrics;
  NS_IF_ADDREF(mFontMetrics);

//PR_LOG(PhGfxLog, PR_LOG_DEBUG, ("nsRenderingContextPh::SetFont after NS_IF_ADDREF(mFontMetrics)\n"));
  
  mFontMetrics->GetFontHandle(fontHandle);

//PR_LOG(PhGfxLog, PR_LOG_DEBUG, ("nsRenderingContextPh::SetFont after GetFontHandle <%p>\n", fontHandle));

  pFontHandle = (nsString *) fontHandle;
    
  if (pFontHandle)
  {  
    if( mPhotonFontName )
      delete [] mPhotonFontName;

    mPhotonFontName = pFontHandle->ToNewCString();
    PfLoadMetrics( mPhotonFontName );

    PR_LOG(PhGfxLog, PR_LOG_DEBUG, ("nsRenderingContextPh::SetFont with nsIFontMetrics Photon Font Name is <%s>\n", mPhotonFontName));

    PgSetFont( mPhotonFontName );
	}
	else
	{
    PR_LOG(PhGfxLog, PR_LOG_ERROR, ("nsRenderingContextPh::SetFont with nsIFontMetrics, INVALID Font Handle\n"));
	}
	
  return NS_OK;
}


NS_IMETHODIMP nsRenderingContextPh :: GetFontMetrics(nsIFontMetrics *&aFontMetrics)
{
  PR_LOG(PhGfxLog, PR_LOG_DEBUG, ("nsRenderingContextPh::GetFontMetrics mFontMetrics=<%p>\n", mFontMetrics));
  NS_IF_ADDREF(mFontMetrics);
  aFontMetrics = mFontMetrics;
  return NS_OK;
}


// add the passed in translation to the current translation
NS_IMETHODIMP nsRenderingContextPh :: Translate(nscoord aX, nscoord aY)
{
  PR_LOG(PhGfxLog, PR_LOG_DEBUG, ("nsRenderingContextPh::Translate (%i,%i)\n", aX, aY));
//  printf("nsRenderingContextPh::Translate (%i,%i)\n", aX, aY);
PtArg_t arg;
PhPoint_t *pos;
PtSetArg(&arg,Pt_ARG_POS,&pos,0);
PtGetResources(mWidget,1,&arg);
//printf ("translate widget: %p %d %d\n",mWidget,pos->x,pos->y);
//aX += pos->x*15;
//aY += pos->y*15;
  mTMatrix->AddTranslation((float)aX,(float)aY);
  return NS_OK;
}


// add the passed in scale to the current scale
NS_IMETHODIMP nsRenderingContextPh :: Scale(float aSx, float aSy)
{
  PR_LOG(PhGfxLog, PR_LOG_DEBUG, ("nsRenderingContextPh::Scale (%f,%f)\n", aSx, aSy ));
  mTMatrix->AddScale(aSx, aSy);
  return NS_OK;
}


NS_IMETHODIMP nsRenderingContextPh :: GetCurrentTransform(nsTransform2D *&aTransform)
{
  PR_LOG(PhGfxLog, PR_LOG_DEBUG, ("nsRenderingContextPh::GetCurrentTransform\n"));
  aTransform = mTMatrix;
  return NS_OK;
}


NS_IMETHODIMP nsRenderingContextPh :: CreateDrawingSurface(nsRect *aBounds, PRUint32 aSurfFlags, nsDrawingSurface &aSurface)
{
// REVISIT; what are the flags???

  if (nsnull==mSurface) {
    aSurface = nsnull;
    return NS_ERROR_FAILURE;
  }

  extern int double_buffer;
  if (!double_buffer) return NS_OK;

  PR_LOG(PhGfxLog, PR_LOG_DEBUG, ("nsRenderingContextPh::CreateDrawingSurface\n"));

 nsDrawingSurfacePh *surf = new nsDrawingSurfacePh();

 if (surf)
 {
   NS_ADDREF(surf);
   surf->Init(mSurface->GetGC(), aBounds->width, aBounds->height, aSurfFlags);
//   surf->Init(mGC, aBounds->width, aBounds->height, aSurfFlags);
   ApplyClipping(mSurface->GetGC()->rid);
 }

 aSurface = (nsDrawingSurface)surf;

  return NS_OK;
}


NS_IMETHODIMP nsRenderingContextPh :: DestroyDrawingSurface(nsDrawingSurface aDS)
{
  PhImage_t *image;
  void *gc;

   PR_LOG(PhGfxLog, PR_LOG_DEBUG, ("nsRenderingContextPh::DestroyDrawingSurface - Not Implemented\n"));

   nsDrawingSurfacePh *surf = (nsDrawingSurfacePh *) aDS;
   NS_IF_RELEASE(surf);

   return NS_OK;
}


NS_IMETHODIMP nsRenderingContextPh :: DrawLine(nscoord aX0, nscoord aY0, nscoord aX1, nscoord aY1)
{
  PR_LOG(PhGfxLog, PR_LOG_DEBUG, ("nsRenderingContextPh::DrawLine (%ld,%ld,%ld,%ld)\n", aX0, aY0, aX1, aY1 ));
  nscoord x0,y0,x1,y1;

  x0 = aX0;
  y0 = aY0;
  x1 = aX1;
  y1 = aY1;

  mTMatrix->TransformCoord(&x0,&y0);
  mTMatrix->TransformCoord(&x1,&y1);

  PR_LOG(PhGfxLog, PR_LOG_DEBUG, ("nsRenderingContextPh::DrawLine (%ld,%ld,%ld,%ld)\n", x0, y0, x1, y1 ));

  PgDrawILine( x0, y0, x1, y1 );

  return NS_OK;
}


NS_IMETHODIMP nsRenderingContextPh :: DrawPolyline(const nsPoint aPoints[], PRInt32 aNumPoints)
{
  PR_LOG(PhGfxLog, PR_LOG_DEBUG, ("untested nsRenderingContextPh::DrawPolyLine\n"));

  PhPoint_t *pts;

  if(( pts = new PhPoint_t [aNumPoints] ) != NULL )
  {
    PhPoint_t pos = {0,0};
    PRInt32 i;

    for(i=0;i<aNumPoints;i++)
    {
    int x,y;
      x = aPoints[i].x;
      y = aPoints[i].y;
      mTMatrix->TransformCoord(&x,&y);
      pts[i].x = x;
      pts[i].y = y;
    }

    PgDrawPolygon( pts, aNumPoints, &pos, Pg_DRAW_STROKE );

    delete [] pts;
  }
  return NS_OK;
}


NS_IMETHODIMP nsRenderingContextPh :: DrawRect(const nsRect& aRect)
{
  PR_LOG(PhGfxLog, PR_LOG_DEBUG, ("untested nsRenderingContextPh::DrawRect\n"));

  DrawRect( aRect.x, aRect.y, aRect.width, aRect.height );

  return NS_OK;
}


NS_IMETHODIMP nsRenderingContextPh :: DrawRect(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight)
{
  PR_LOG(PhGfxLog, PR_LOG_DEBUG, ("untested nsRenderingContextPh::DrawRect\n"));

  nscoord x,y,w,h;

  x = aX;
  y = aY;
  w = aWidth;
  h = aHeight;
  mTMatrix->TransformCoord(&x,&y,&w,&h);

  PgDrawIRect( x, y, x + w - 1, y + h - 1, Pg_DRAW_STROKE );

  return NS_OK;
}


NS_IMETHODIMP nsRenderingContextPh :: FillRect(const nsRect& aRect)
{
//  PR_LOG(PhGfxLog, PR_LOG_DEBUG, ("nsRenderingContextPh::FillRect (%i,%i,%i,%i)\n", aRect.x, aRect.y, aRect.width, aRect.height ));

  FillRect( aRect.x, aRect.y, aRect.width, aRect.height );

  return NS_OK;
}


NS_IMETHODIMP nsRenderingContextPh :: FillRect(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight)
{
  PR_LOG(PhGfxLog, PR_LOG_DEBUG, ("nsRenderingContextPh::FillRect (%i,%i,%i,%i)\n", aX, aY, aWidth, aHeight ));
  nscoord x,y,w,h;

  x = aX;
  y = aY;
  w = aWidth;
  h = aHeight;

  mTMatrix->TransformCoord(&x,&y,&w,&h);
//printf ("fill rect 2: %d %d %d %d\n",x,y,w,h);
PtArg_t arg;
PhPoint_t *pos;
PtSetArg(&arg,Pt_ARG_POS,&pos,0);
PtGetResources(mWidget,1,&arg);
//printf ("fill rect 3: %p %d %d\n",mWidget,pos->x,pos->y);
//x+=pos->x;
//y+=pos->y;
  PgDrawIRect( x, y, x + w - 1, y + h - 1, Pg_DRAW_FILL_STROKE );

  return NS_OK;
}


NS_IMETHODIMP nsRenderingContextPh :: DrawPolygon(const nsPoint aPoints[], PRInt32 aNumPoints)
{
  PR_LOG(PhGfxLog, PR_LOG_DEBUG, ("untested nsRenderingContextPh::DrawPolygon\n"));

  PhPoint_t *pts;

  if(( pts = new PhPoint_t [aNumPoints] ) != NULL )
  {
    PhPoint_t pos = {0,0};
    PRInt32 i;

    for(i=0;i<aNumPoints;i++)
    {
    int x,y;
      x = aPoints[i].x;
      y = aPoints[i].y;
      mTMatrix->TransformCoord(&x,&y);
      pts[i].x = x;
      pts[i].y = y;
    }

    PgDrawPolygon( pts, aNumPoints, &pos, Pg_DRAW_STROKE | Pg_CLOSED );

    delete [] pts;
  }
  return NS_OK;
}


NS_IMETHODIMP nsRenderingContextPh :: FillPolygon(const nsPoint aPoints[], PRInt32 aNumPoints)
{
  PR_LOG(PhGfxLog, PR_LOG_DEBUG, ("untested nsRenderingContextPh::FillPolygon\n"));

  PhPoint_t *pts;

  if(( pts = new PhPoint_t [aNumPoints] ) != NULL )
  {
    PhPoint_t pos = {0,0};
    PRInt32 i;

    for(i=0;i<aNumPoints;i++)
    {
    int x,y;
      x = aPoints[i].x;
      y = aPoints[i].y;
      mTMatrix->TransformCoord(&x,&y);
      pts[i].x = x;
      pts[i].y = y;
    }

    PgDrawPolygon( pts, aNumPoints, &pos, Pg_DRAW_FILL_STROKE | Pg_CLOSED );

    delete [] pts;
  }
  return NS_OK;
}


NS_IMETHODIMP nsRenderingContextPh :: DrawEllipse(const nsRect& aRect)
{
  PR_LOG(PhGfxLog, PR_LOG_DEBUG, ("nsRenderingContextPh::DrawEllipse.\n"));

  DrawEllipse( aRect.x, aRect.y, aRect.width, aRect.height );

  return NS_OK;
}


NS_IMETHODIMP nsRenderingContextPh :: DrawEllipse(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight)
{
  PR_LOG(PhGfxLog, PR_LOG_DEBUG, ("nsRenderingContextPh::DrawEllipse.\n"));
  nscoord x,y,w,h;
  PhPoint_t center;
  PhPoint_t radii;
  unsigned int flags;

  x = aX;
  y = aY;
  w = aWidth;
  h = aHeight;

  mTMatrix->TransformCoord(&x,&y,&w,&h);

  center.x = x;
  center.y = y;
  radii.x = x+w-1;
  radii.y = y+h-1;
  flags = Pg_EXTENT_BASED | Pg_DRAW_STROKE;
  PgDrawEllipse( &center, &radii, flags );

  return NS_OK;
}


NS_IMETHODIMP nsRenderingContextPh :: FillEllipse(const nsRect& aRect)
{
  PR_LOG(PhGfxLog, PR_LOG_DEBUG, ("nsRenderingContextPh::FillEllipse.\n"));

  FillEllipse( aRect.x, aRect.y, aRect.width, aRect.height );

  return NS_OK;
}


NS_IMETHODIMP nsRenderingContextPh :: FillEllipse(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight)
{
  PR_LOG(PhGfxLog, PR_LOG_DEBUG, ("nsRenderingContextPh::FillEllipse.\n"));
  nscoord x,y,w,h;
  PhPoint_t center;
  PhPoint_t radii;
  unsigned int flags;

  x = aX;
  y = aY;
  w = aWidth;
  h = aHeight;

  mTMatrix->TransformCoord(&x,&y,&w,&h);

  center.x = x;
  center.y = y;
  radii.x = x+w-1;
  radii.y = y+h-1;
  flags = Pg_EXTENT_BASED | Pg_DRAW_FILL_STROKE;
  PgDrawEllipse( &center, &radii, flags );

  return NS_OK;
}


NS_IMETHODIMP nsRenderingContextPh :: DrawArc(const nsRect& aRect,
                                 float aStartAngle, float aEndAngle)
{
  PR_LOG(PhGfxLog, PR_LOG_DEBUG, ("nsRenderingContextPh::DrawArc - Not implemented.\n"));

  return NS_OK;
}


NS_IMETHODIMP nsRenderingContextPh :: DrawArc(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight,
                                 float aStartAngle, float aEndAngle)
{
  PR_LOG(PhGfxLog, PR_LOG_DEBUG, ("nsRenderingContextPh::DrawArc - Not implemented.\n"));

  return NS_OK;
}


NS_IMETHODIMP nsRenderingContextPh :: FillArc(const nsRect& aRect,
                                 float aStartAngle, float aEndAngle)
{
  PR_LOG(PhGfxLog, PR_LOG_DEBUG, ("nsRenderingContextPh::FillArc - Not implemented.\n"));

  return NS_OK;
}


NS_IMETHODIMP nsRenderingContextPh :: FillArc(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight,
                                 float aStartAngle, float aEndAngle)
{
  PR_LOG(PhGfxLog, PR_LOG_DEBUG, ("nsRenderingContextPh::FillArc - Not implemented.\n"));

  return NS_OK;
}


NS_IMETHODIMP nsRenderingContextPh :: GetWidth(char ch, nscoord& aWidth)
{
  char buf[2];
  nsresult ret_code;

  /* turn it into a string */
  buf[0] = ch;
  buf[1] = nsnull;

  ret_code = GetWidth(buf, 1, aWidth);  
  PR_LOG(PhGfxLog, PR_LOG_DEBUG, ("nsRenderingContextPh::GetWidth1 for <%c> aWidth=<%d> ret_code=<%d>\n", ch, aWidth, ret_code));

  return ret_code;
}


NS_IMETHODIMP nsRenderingContextPh :: GetWidth(PRUnichar ch, nscoord &aWidth, PRInt32 *aFontID)
{
  PRUnichar buf[2];
  nsresult ret_code;

  /* turn it into a string */
  buf[0] = ch;
  buf[1] = nsnull;

  ret_code = GetWidth(buf, 1, aWidth, aFontID);  
  PR_LOG(PhGfxLog, PR_LOG_DEBUG, ("nsRenderingContextPh::GetWidth2 for <%c> aWidth=<%d> aFontId=<%p> ret_code=<%d>\n", (char) ch, aWidth, aFontID, ret_code));
  return ret_code;
}


NS_IMETHODIMP nsRenderingContextPh :: GetWidth(const char* aString, nscoord& aWidth)
{
  nsresult ret_code;

  ret_code = GetWidth(aString, strlen(aString), aWidth);  
  PR_LOG(PhGfxLog, PR_LOG_DEBUG, ("nsRenderingContextPh::GetWidth3 for <%s> aWidth=<%d> ret_code=<%d>\n", aString, aWidth, ret_code));

  return ret_code;
}


NS_IMETHODIMP nsRenderingContextPh :: GetWidth(const char* aString,
                                                PRUint32 aLength,
                                                nscoord& aWidth)
{
  nsresult ret_code = NS_ERROR_FAILURE;
  
  aWidth = 0;	// Initialize to zero in case we fail.

  if (nsnull != mFontMetrics)
  {
    PhRect_t      extent;
	
    if (PfExtentText(&extent, NULL, mPhotonFontName, aString, aLength))
    {
      aWidth = (int) ((extent.lr.x - extent.ul.x + 1) * mP2T);

      ret_code = NS_OK;
    }
  }
  else
  {
    PR_LOG(PhGfxLog, PR_LOG_DEBUG, ("nsRenderingContextPh::GetWidth4 FAILED = a NULL mFontMetrics detected\n"));
    ret_code = NS_ERROR_FAILURE;
  }  

  return ret_code;
}


NS_IMETHODIMP nsRenderingContextPh :: GetWidth(const nsString& aString, nscoord& aWidth, PRInt32 *aFontID)
{
#if 0
  /* DEBUG ONLY */
  char *str = aString.ToNewCString();
  PR_LOG(PhGfxLog, PR_LOG_DEBUG, ("nsRenderingContextPh::GetWidth5 aString=<%s>\n", str));
  delete [] str;
#endif

  nsresult ret_code;

  ret_code = GetWidth(aString.GetUnicode(), aString.Length(), aWidth, aFontID);  

  /* What the heck? I copied this from Windows */
  if (nsnull != aFontID)
    *aFontID = 0;

  PR_LOG(PhGfxLog, PR_LOG_DEBUG, ("nsRenderingContextPh::GetWidth5  aWidth=<%d> ret_code=<%d>\n", aWidth, ret_code));
  return ret_code;
}


NS_IMETHODIMP nsRenderingContextPh :: GetWidth(const PRUnichar *aString,
                                                PRUint32 aLength,
                                                nscoord &aWidth,
                                                PRInt32 *aFontID)
{
  nsresult ret_code = NS_ERROR_FAILURE;
  nscoord photonWidth;
  
  aWidth = 0;	// Initialize to zero in case we fail.

  if (nsnull != mFontMetrics)
  {
    PhRect_t      extent;
//    nsFontHandle  fontHandle;			/* really a (nsString  *) */
//    nsString      *pFontHandle = nsnull;
//    char          *PhotonFontName =  nsnull;

//    mFontMetrics->GetFontHandle(fontHandle);
//    pFontHandle = (nsString *) fontHandle;
//    PhotonFontName =  pFontHandle->ToNewCString();
	
    if (PfExtentWideText(&extent, NULL, mPhotonFontName, (wchar_t *) aString, (aLength*2)))
    {
//	  photonWidth = (extent.lr.x - extent.ul.x + 1);
// 	  aWidth = (int) ((float) photonWidth * mP2T);
      aWidth = (int) ((extent.lr.x - extent.ul.x + 1) * mP2T);
     
//      PR_LOG(PhGfxLog, PR_LOG_DEBUG, ("nsRenderingContextPh::GetWidth4 PhotonWidth=<%d> aWidth=<%d> PhotonFontName=<%s>\n",photonWidth, aWidth, PhotonFontName));

      ret_code = NS_OK;
//	  delete [] PhotonFontName;
    }
  }
  else
  {
    PR_LOG(PhGfxLog, PR_LOG_DEBUG, ("nsRenderingContextPh::GetWidth6 FAILED = a NULL mFontMetrics detected\n"));
    ret_code = NS_ERROR_FAILURE;
  }  

#if 1
/**************************************************************/
/* ---kirk this causes us to crash when placing the cursor --- */
/* --- Don't know why!                                     --- */

  /* What the heck? I copied this from Windows */
  if (nsnull != aFontID)
  {
    *aFontID = 0;
  }
  	

  PR_LOG(PhGfxLog, PR_LOG_DEBUG, ("nsRenderingContextPh::GetWidth6  aWidth=<%d> ret_code=<%d>\n", aWidth, ret_code));
#endif

  return ret_code;
}


NS_IMETHODIMP nsRenderingContextPh :: DrawString(const char *aString, PRUint32 aLength,
                                                  nscoord aX, nscoord aY,
                                                  const nscoord* aSpacing)
{
  PR_LOG(PhGfxLog, PR_LOG_DEBUG, ("nsRenderingContextPh::DrawString1 first aString=<%s> of %d at (%d,%d) aSpacing=<%p>.\n",aString, aLength, aX, aY, aSpacing));

  nscoord x = aX;
  nscoord y = aY;

  if (nsnull != aSpacing)
  {
/* REVISIT this code will break with an ACTUAL multi-byte multi-byte string */
    // Render the string, one character at a time...
    const char* end = aString + aLength;
	while (aString < end)
	{
	  char ch = *aString++;
      nscoord xx = x;
	  nscoord yy = y;
	  mTMatrix->TransformCoord(&xx, &yy);
      PhPoint_t pos = { xx, yy };
      PgDrawText( &ch, 1, &pos, (Pg_TEXT_LEFT | Pg_TEXT_TOP));
      x += *aSpacing++;
    }
  }
  else
  {
    mTMatrix->TransformCoord(&x,&y);
    PhPoint_t pos = { x, y };

    PgDrawText( aString, aLength, &pos, (Pg_TEXT_LEFT | Pg_TEXT_TOP));
  }

  return NS_OK;
}


NS_IMETHODIMP nsRenderingContextPh :: DrawString(const PRUnichar *aString, PRUint32 aLength,
                                                  nscoord aX, nscoord aY,
                                                  PRInt32 aFontID,
                                                  const nscoord* aSpacing)
{
#if 0
  /* DEBUG!! */
  char buffer[aLength*3];
  int i,x;

  for(i=0,x=0; i<aLength*2; i++)
  {
    if (*(aString+i) != NULL)
	  buffer[x++] = *(aString+i);
  }
  
  buffer[x] = NULL;
#endif

#if 0
  nscoord X0 = aX;
  nscoord Y0 = aY;

  mTMatrix->TransformCoord(&X0,&Y0);
 
  PhPoint_t pos = { X0, Y0 };

  PR_LOG(PhGfxLog, PR_LOG_DEBUG, ("nsRenderingContextPh::DrawString2 aString=<%s> of %d at (%d,%d) aSpacing=<%p>\n", (char *) buffer, aLength, pos.x, pos.y, aSpacing));

  PgDrawTextChars( (char *) aString, aLength, &pos, (Pg_TEXT_WIDECHAR | Pg_TEXT_LEFT | Pg_TEXT_TOP));
  
  return NS_OK;
#endif

#if 0
  PR_LOG(PhGfxLog, PR_LOG_DEBUG, ("nsRenderingContextPh::DrawString2 first aString=<%s> of %d at (%d,%d) aSpacing=<%p>.\n",aString, aLength, aX, aY, aSpacing));

  nscoord x = aX;
  nscoord y = aY;

  if (nsnull != aSpacing)
  {
    // Render the string, one character at a time...
    const PRUnichar* end = aString + aLength;
	while (aString < end)
	{
      unsigned char ch[2];
	  memcpy(ch, aString++,2);
	  unsigned char tmp;
	  tmp=ch[0];
	  ch[0]=ch[1];
	  ch[1]=tmp;
	  nscoord xx = x;
	  nscoord yy = y;
	  mTMatrix->TransformCoord(&xx, &yy);
      PhPoint_t pos = { xx, yy };
      PgDrawText( (char *) ch, 1, &pos, (Pg_TEXT_WIDECHAR | Pg_TEXT_LEFT | Pg_TEXT_TOP));
      x += *aSpacing++;
    }
  }
  else
  {
    mTMatrix->TransformCoord(&x,&y);
    PhPoint_t pos = { x, y };

    PgDrawText( (char *) aString, aLength, &pos, (Pg_TEXT_WIDECHAR | Pg_TEXT_LEFT | Pg_TEXT_TOP));
  }

  return NS_OK;

#endif

#if 1
  const int BUFFER_SIZE = (aLength * 3);
  char buffer[BUFFER_SIZE];
  int len;
  
  len = wcstombs(buffer, (wchar_t *) aString, BUFFER_SIZE);
  return DrawString( (char *) buffer, aLength, aX, aY, aSpacing);
#endif
}


NS_IMETHODIMP nsRenderingContextPh :: DrawString(const nsString& aString,
                                                  nscoord aX, nscoord aY,
                                                  PRInt32 aFontID,
                                                  const nscoord* aSpacing)
{
  PR_LOG(PhGfxLog, PR_LOG_DEBUG, ("nsRenderingContextPh::DrawString3 at (%d,%d) aSpacing=<%p>.\n", aX, aY, aSpacing));

  return DrawString(aString.GetUnicode(), aString.Length(),
                      aX, aY, aFontID, aSpacing);
}


NS_IMETHODIMP nsRenderingContextPh :: DrawImage(nsIImage *aImage, nscoord aX, nscoord aY)
{
  PR_LOG(PhGfxLog, PR_LOG_DEBUG, ("nsRenderingContextPh::DrawImage1\n"));

  nsresult res;
  nscoord x,y,w,h;

  x = aX;
  y = aY;
  w = NSToCoordRound( mP2T * aImage->GetWidth());
  h = NSToCoordRound( mP2T * aImage->GetHeight());

  mTMatrix->TransformCoord(&x,&y,&w,&h);

  res = aImage->Draw( *this, mSurface, x, y, w, h );

  return res;
}


NS_IMETHODIMP nsRenderingContextPh :: DrawImage(nsIImage *aImage, nscoord aX, nscoord aY,
                                        nscoord aWidth, nscoord aHeight) 
{
  PR_LOG(PhGfxLog, PR_LOG_DEBUG, ("nsRenderingContextPh::DrawImage2\n"));

  nsresult res;
  nscoord x,y,w,h;

  x = aX;
  y = aY;
  w = aWidth;
  h = aHeight;

  mTMatrix->TransformCoord(&x,&y,&w,&h);

  res = aImage->Draw( *this, mSurface, x, y, w, h );

  return res;
}


NS_IMETHODIMP nsRenderingContextPh :: DrawImage(nsIImage *aImage, const nsRect& aSRect, const nsRect& aDRect)
{
  PR_LOG(PhGfxLog, PR_LOG_DEBUG, ("nsRenderingContextPh::DrawImage3\n"));

  nsresult res;
  nsRect	sr,dr;

  sr = aSRect;
  mTMatrix->TransformCoord(&sr.x,&sr.y,&sr.width,&sr.height);

  dr = aDRect;
  mTMatrix->TransformCoord(&dr.x,&dr.y,&dr.width,&dr.height);

  res = aImage->Draw(*this,mSurface,sr.x,sr.y,sr.width,sr.height, dr.x,dr.y,dr.width,dr.height);

  return res;
}


NS_IMETHODIMP nsRenderingContextPh :: DrawImage(nsIImage *aImage, const nsRect& aRect)
{
  PR_LOG(PhGfxLog, PR_LOG_DEBUG, ("nsRenderingContextPh::DrawImage4\n"));

  nsresult res;
  nsRect	tr;

  tr = aRect;
  mTMatrix->TransformCoord(&tr.x,&tr.y,&tr.width,&tr.height);

  res = aImage->Draw(*this,mSurface,tr.x,tr.y,tr.width,tr.height);

  return res;
}

static int count=0;

NS_IMETHODIMP nsRenderingContextPh :: CopyOffScreenBits(nsDrawingSurface aSrcSurf,
                                                         PRInt32 aSrcX, PRInt32 aSrcY,
                                                         const nsRect &aDestBounds,
                                                         PRUint32 aCopyFlags)
{
  PR_LOG(PhGfxLog, PR_LOG_DEBUG, ("nsRenderingContextPh::CopyOffScreenBits.\n"));
  PhArea_t    area;

  area.pos.x=aDestBounds.x;
  area.pos.y=aDestBounds.y;
  area.size.w=aDestBounds.width;
  area.size.h=aDestBounds.height;
//  printf ("location: %p (%d %d) %d %d %d %d\n",mOffscreenSurface,aSrcX,aSrcY,area.pos.x,area.pos.y,area.size.w,area.size.h);

  ((nsDrawingSurfacePh *)aSrcSurf)->Stop();

/*
  PgSetGC( mGC );
  PgSetRegion( mGC->rid );
  ApplyClipping( mGC->rid );
*/

  PhImage_t *image;
  image = ((nsDrawingSurfacePh *)aSrcSurf)->mPixmap;
//printf ("kedl2: copy bits: %p\n",image);
  mOffscreenSurface->Select();
  ApplyClipping( mOffscreenSurface->GetGC()->rid );

  nscoord X0 = aSrcX;
  nscoord Y0 = aSrcY;

//  mTMatrix->TransformCoord(&X0,&Y0);
  PhPoint_t pos = { X0, Y0 };
  PhDim_t size = { area.size.w,area.size.h };

pos.x=0; pos.y=0;

//  PgDrawImagemx( image->image, image->type , &pos, &image->size, image->bpl, 0); 
  PgDrawImagemx( image->image, image->type , &pos, &size, image->bpl, 0); 
//sleep(1);
if (0)
{
FILE *f;
char buf[100];
sprintf (buf,"image.%02d",count++);
printf ("saving: %s %d\n",buf,image->size.h * image->bpl); fflush(stdout);
f = fopen(buf,"w");
fwrite(image->image,1,image->size.h * image->bpl,f);
fclose(f);
}

  PgSetGC( mPtGC );
  PgSetRegion( mPtGC->rid );

  return NS_OK;
}


void nsRenderingContextPh :: PushClipState(void)
{
  PR_LOG(PhGfxLog, PR_LOG_DEBUG, ("nsRenderingContextPh::PushClipState - Not implemented.\n"));
}

NS_IMETHODIMP nsRenderingContextPh::GetScriptObject(nsIScriptContext* aContext,
                                       void** aScriptObject)
{
  PR_LOG(PhGfxLog, PR_LOG_DEBUG, ("nsRenderingContextPh::GetScriptObject - Not implemented.\n"));
  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextPh::SetScriptObject(void* aScriptObject)
{
  PR_LOG(PhGfxLog, PR_LOG_DEBUG, ("nsRenderingContextPh::SetScriptObject - Not implemented.\n"));
  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextPh::GetColor(nsString& aColor)
{
  PR_LOG(PhGfxLog, PR_LOG_DEBUG, ("nsRenderingContextPh::GetColor - Not implemented.\n"));
  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextPh::SetColor(const nsString& aColor)
{
  PR_LOG(PhGfxLog, PR_LOG_DEBUG, ("nsRenderingContextPh::SetColor - Not implemented.\n"));
  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextPh::DrawLine2(PRInt32 aX0, PRInt32 aY0,
                                 PRInt32 aX1, PRInt32 aY1)
{
  PR_LOG(PhGfxLog, PR_LOG_DEBUG, ("nsRenderingContextPh::DrawLine2.\n"));

  DrawLine( (nscoord)aX0, (nscoord)aY0, (nscoord)aX1, (nscoord)aY1 );

  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextPh :: CreateDrawingSurface( PhGC_t *aGC, nsDrawingSurface &aSurface)
{
  PR_LOG(PhGfxLog, PR_LOG_DEBUG, ("nsRenderingContextPh::CreateDrawingSurface - Not implemented.\n"));

  return NS_OK;
}


void nsRenderingContextPh::ApplyClipping( int rid )
{
PtArg_t arg;
PhPoint_t *pos;

//  PgSetClipping( 0, NULL );
//return;

  if (mRegion)
  {
    PhRegion_t my_region;
    PhRect_t rect = {{0,0},{0,0}};
    int offset_x=0,offset_y=0;
    int err;
    nsRegionPh tmp_region;
    PhTile_t *tiles;
    PhRect_t *rects;
    int rect_count;

      err = PhRegionQuery(rid, &my_region, &rect, NULL, 0);
	   
//    err=PhRegionQuery(mGC->rid, &my_region, &rect, NULL, 0);
//    PR_LOG(PhGfxLog, PR_LOG_DEBUG, ("nsRenderingContextPh::SetGC err=<%d> rect=(%d,%d)-(%d,%d)\n",err, rect.ul.x, rect.ul.y, rect.lr.x, rect.lr.y));
//    PR_LOG(PhGfxLog, PR_LOG_DEBUG, ("nsRenderingContextPh::SetGC Origin of region is (%d,%d)\n", my_region.origin.x, my_region.origin.y));

PtSetArg(&arg,Pt_ARG_POS,&pos,0);
PtGetResources(mWidget,1,&arg);
//printf ("clip widget: %p %d %d\n",mWidget,pos->x,pos->y);

    if ((err == 0) && ( (my_region.origin.x!=0) || (my_region.origin.y!=0)))
    {
      offset_x = my_region.origin.x * -1;
      offset_y = my_region.origin.y * -1;

//printf ("jerry offset %d %d %d, %d %d %d %d\n",rid,offset_x,offset_y,rect.ul.x,rect.ul.y,rect.lr.x,rect.lr.y);
      tmp_region.SetTo(*mRegion);
//      tmp_region.Offset(offset_x, offset_y);
      tmp_region.GetNativeRegion((void*&)tiles);
    }
    else
    {
      /* no offset needed use the normal tile list */
      mRegion->GetNativeRegion((void*&)tiles);
    }

    if (tiles != nsnull)
    {
      rects = PhTilesToRects(tiles, &rect_count);
//      PR_LOG(PhGfxLog, PR_LOG_DEBUG, ("nsRenderingContextPh::SetGC Calling PgSetClipping with %d rects\n", rect_count));
      PgSetMultiClip(rect_count,rects);
      free(rects);
    }
  }
//  PgSetFillColor(Pg_GREEN);
//  PgDrawIRect( pos->x, pos->y, pos->x + 150 - 1, pos->y + 150 - 1, Pg_DRAW_FILL_STROKE );
}


