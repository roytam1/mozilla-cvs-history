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


#include "xp_core.h"			//this is a hack to get it to build. MMP
#include "nsRenderingContextUnix.h"
#include "nsDeviceContextUnix.h"

#include <math.h>
#include "nspr.h"

#include "nsRegionUnix.h"
#include "nsGfxCIID.h"

#include "X11/Xlib.h"
#include "X11/Xutil.h"



/*
  Some Implementation Notes

  REGIONS:  Regions are clipping rects associated with a GC. Since 
  multiple Drawable's can and do share GC's (they are hardware cached)
  In order to select clip rect's into GC's, they must be writeable. Thus, 
  any consumer of the 'gfx' library must assume that GC's created by them
  will be modified in gfx.

 */

class GraphicsState
{
public:
  GraphicsState();
  ~GraphicsState();

  nsTransform2D   *mMatrix;
  nsRect          mLocalClip;
  Region          mClipRegion;
  nscolor         mColor;
  nsLineStyle     mLineStyle;
  nsIFontMetrics  *mFontMetrics;
  Font            mFont;
};

GraphicsState :: GraphicsState()
{
  mMatrix = nsnull;  
  mLocalClip.x = mLocalClip.y = mLocalClip.width = mLocalClip.height = 0;
  mClipRegion = nsnull;
  mColor = NS_RGB(0, 0, 0);
  mLineStyle = nsLineStyle_kSolid;
  mFontMetrics = nsnull;
  mFont = nsnull;
}

GraphicsState :: ~GraphicsState()
{
  mFont = nsnull;
}

static NS_DEFINE_IID(kRenderingContextIID, NS_IRENDERING_CONTEXT_IID);

nsRenderingContextUnix :: nsRenderingContextUnix()
{
  NS_INIT_REFCNT();

  mFontMetrics = nsnull ;
  mContext = nsnull ;
  mFrontBuffer = nsnull ;
  mRenderingSurface = nsnull ;
  mCurrentColor = 0;
  mCurrentLineStyle = nsLineStyle_kSolid;
  mTMatrix = nsnull;
  mP2T = 1.0f;
  mStateCache = new nsVoidArray();
  mRegion = nsnull;
  mCurrFontHandle = 0;
  PushState();

#ifdef MITSHM
  mHasSharedMemory = PR_FALSE;
  mSupportsSharedPixmaps = PR_FALSE;
#endif

}

nsRenderingContextUnix :: ~nsRenderingContextUnix()
{
  if (mRegion) {
    ::XDestroyRegion(mRegion);
    mRegion = nsnull;
  }

  mTMatrix = nsnull;

  // Destroy the State Machine
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

  // Destroy the front buffer and it's GC if one was allocated for it
  if (nsnull != mFrontBuffer) {
    delete mFrontBuffer;
  }

  NS_IF_RELEASE(mFontMetrics);
  NS_IF_RELEASE(mContext);

  if (nsnull != mDrawStringBuf) {
    PR_Free(mDrawStringBuf);
  }

}

NS_IMPL_QUERY_INTERFACE(nsRenderingContextUnix, kRenderingContextIID)
NS_IMPL_ADDREF(nsRenderingContextUnix)
NS_IMPL_RELEASE(nsRenderingContextUnix)

NS_IMETHODIMP
nsRenderingContextUnix :: Init(nsIDeviceContext* aContext,
                               nsIWidget *aWindow)
{

  if (nsnull == aWindow->GetNativeData(NS_NATIVE_WINDOW))
    return NS_ERROR_NOT_INITIALIZED;

  mContext = aContext;
  NS_IF_ADDREF(mContext);

  mRenderingSurface = new nsDrawingSurfaceUnix();

#ifdef MITSHM
  mRenderingSurface->shmImage = nsnull;
#endif

  mRenderingSurface->display =  (Display *)aWindow->GetNativeData(NS_NATIVE_DISPLAY);
  mRenderingSurface->drawable = (Drawable)aWindow->GetNativeData(NS_NATIVE_WINDOW);
  mRenderingSurface->gc       = (GC)aWindow->GetNativeData(NS_NATIVE_GRAPHIC);

  XWindowAttributes wa;

  ::XGetWindowAttributes(mRenderingSurface->display,
			 mRenderingSurface->drawable,
			 &wa);

  mRenderingSurface->visual = wa.visual;
  mRenderingSurface->depth = wa.depth;
  
  mFrontBuffer = mRenderingSurface;

  return (CommonInit());
}

NS_IMETHODIMP
nsRenderingContextUnix :: Init(nsIDeviceContext* aContext,
                               nsDrawingSurface aSurface)
{

  mContext = aContext;
  NS_IF_ADDREF(mContext);

  mRenderingSurface = (nsDrawingSurfaceUnix *) aSurface;

  return (CommonInit());
}

nsresult nsRenderingContextUnix :: CommonInit()
{

#ifdef MITSHM
  PRInt32 shmMajor, shmMinor ;
#endif

  ((nsDeviceContextUnix *)mContext)->SetDrawingSurface(mRenderingSurface);
  ((nsDeviceContextUnix *)mContext)->InstallColormap();

  mContext->GetDevUnitsToAppUnits(mP2T);
  float app2dev;
  mContext->GetAppUnitsToDevUnits(app2dev);
  mTMatrix->AddScale(app2dev, app2dev);

#ifdef MITSHM

  // We need to query the extension first using straight XLib since
  // the Shared memory invocation prints an error message to stdout

  PRInt32 maj, evt, err;

  if (::XQueryExtension(mRenderingSurface->display,
			"MIT-SHM",
			&maj, &evt, &err) == True) {

    if (XShmQueryVersion(mRenderingSurface->display, 
			 &shmMajor, 
			 &shmMinor, 
			 &mSupportsSharedPixmaps) != 0) {
      
      mHasSharedMemory = PR_TRUE;
      mRenderingSurface->shmImage = nsnull;
      mRenderingSurface->shmInfo.shmaddr = nsnull;
    }
  }
#endif

  return NS_OK;
}

NS_IMETHODIMP
nsRenderingContextUnix :: SelectOffScreenDrawingSurface(nsDrawingSurface aSurface)
{  
  if (nsnull == aSurface)
    mRenderingSurface = mFrontBuffer;
  else
    mRenderingSurface = (nsDrawingSurfaceUnix *)aSurface;

  return NS_OK;
}

NS_IMETHODIMP
nsRenderingContextUnix::GetHints(PRUint32& aResult)
{
  PRUint32 result = 0;

  // Most X servers implement 8 bit text rendering alot faster than
  // XChar2b rendering. In addition, we can avoid the PRUnichar to
  // XChar2b conversion. So we set this bit...
  result |= NS_RENDERING_HINT_FAST_8BIT_TEXT;

  // XXX see if we are rendering to the local display or to a remote
  // dispaly and set the NS_RENDERING_HINT_REMOTE_RENDERING accordingly

  aResult = result;
  return NS_OK;
}

void nsRenderingContextUnix :: Reset()
{
}

nsIDeviceContext * nsRenderingContextUnix :: GetDeviceContext(void)
{
  NS_IF_ADDREF(mContext);
  return mContext;
}

void nsRenderingContextUnix :: PushState(void)
{

  nsRect rect;

  GraphicsState * state = new GraphicsState();

  // Push into this state object, add to vector
  state->mMatrix = mTMatrix;

  mStateCache->AppendElement(state);

  if (nsnull == mTMatrix)
    mTMatrix = new nsTransform2D();
  else
    mTMatrix = new nsTransform2D(mTMatrix);

  GetClipRect(state->mLocalClip);

  state->mClipRegion = mRegion;

  if (nsnull != state->mClipRegion) {
    mRegion = ::XCreateRegion();

    XRectangle xrect;
    
    xrect.x = state->mLocalClip.x;
    xrect.y = state->mLocalClip.y;
    xrect.width = state->mLocalClip.width;
    xrect.height = state->mLocalClip.height;
    
    ::XUnionRectWithRegion(&xrect, mRegion, mRegion);
  }

  state->mColor = mCurrentColor;
  state->mLineStyle = mCurrentLineStyle;

}

PRBool nsRenderingContextUnix :: PopState(void)
{

  PRBool bEmpty = PR_FALSE;

  PRUint32 cnt = mStateCache->Count();
  GraphicsState * state;

  if (cnt > 0) {
    state = (GraphicsState *)mStateCache->ElementAt(cnt - 1);
    mStateCache->RemoveElementAt(cnt - 1);

    // Assign all local attributes from the state object just popped
    if (mTMatrix)
      delete mTMatrix;
    mTMatrix = state->mMatrix;

    if (nsnull != mRegion)
      ::XDestroyRegion(mRegion);

    mRegion = state->mClipRegion;

    if (nsnull != mRegion && ::XEmptyRegion(mRegion) == True){
      bEmpty = PR_TRUE;
    }else{

      // Select in the old region.  We probably want to set a dirty flag and only 
      // do this IFF we need to draw before the next Pop.  We'd need to check the
      // state flag on every draw operation.
      if (nsnull != mRegion)
	::XSetRegion(mRenderingSurface->display,
		     mRenderingSurface->gc,
		     mRegion);

    }

    if (state->mColor != mCurrentColor)
      SetColor(state->mColor);

    if (state->mLineStyle != mCurrentLineStyle)
      SetLineStyle(state->mLineStyle);
    

    // Delete this graphics state object
    delete state;
  }

  return bEmpty;
}

PRBool nsRenderingContextUnix :: IsVisibleRect(const nsRect& aRect)
{
  return PR_TRUE;
}

PRBool nsRenderingContextUnix :: SetClipRectInPixels(const nsRect& aRect, nsClipCombine aCombine)
{
  PRBool bEmpty = PR_FALSE;

  nsRect  trect = aRect;

  XRectangle xrect;

  xrect.x = trect.x;
  xrect.y = trect.y;
  xrect.width = trect.width;
  xrect.height = trect.height;

  Region a = ::XCreateRegion();
  ::XUnionRectWithRegion(&xrect, a, a);

  if (aCombine == nsClipCombine_kIntersect)
  {
    Region tRegion = ::XCreateRegion();

    if (nsnull != mRegion) {
      ::XIntersectRegion(a, mRegion, tRegion);
      ::XDestroyRegion(mRegion);
      ::XDestroyRegion(a);
      mRegion = tRegion;
    } else {
      ::XDestroyRegion(tRegion);
      mRegion = a;
    }

  }
  else if (aCombine == nsClipCombine_kUnion)
  {
    if (nsnull != mRegion) {
      Region tRegion = ::XCreateRegion();
      ::XUnionRegion(a, mRegion, tRegion);
      ::XDestroyRegion(mRegion);
      ::XDestroyRegion(a);
      mRegion = tRegion;
    } else {
      mRegion = a;
    }

  }
  else if (aCombine == nsClipCombine_kSubtract)
  {

    if (nsnull != mRegion) {

      Region tRegion = ::XCreateRegion();
      ::XSubtractRegion(mRegion, a, tRegion);
      ::XDestroyRegion(mRegion);
      ::XDestroyRegion(a);
      mRegion = tRegion;

    } else {
      mRegion = a;
    }

  }
  else if (aCombine == nsClipCombine_kReplace)
  {

    if (nsnull != mRegion)
      ::XDestroyRegion(mRegion);

    mRegion = a;

  }
  else
    NS_ASSERTION(PR_FALSE, "illegal clip combination");

  if (::XEmptyRegion(mRegion) == True) {

    bEmpty = PR_TRUE;
    ::XSetClipMask(mRenderingSurface->display,
		   mRenderingSurface->gc,
		   None);

  } else {

    ::XSetRegion(mRenderingSurface->display,
		 mRenderingSurface->gc,
		 mRegion);

  }

  return bEmpty;
}

PRBool nsRenderingContextUnix :: SetClipRect(const nsRect& aRect, nsClipCombine aCombine)
{
  nsRect  trect = aRect;

  mTMatrix->TransformCoord(&trect.x, &trect.y,
                           &trect.width, &trect.height);
  return(SetClipRectInPixels(trect,aCombine));
}

PRBool nsRenderingContextUnix :: GetClipRect(nsRect &aRect)
{

  if (mRegion != nsnull) {
    XRectangle xrect;
    ::XClipBox(mRegion, &xrect);
    aRect.SetRect(xrect.x, xrect.y, xrect.width, xrect.height);
  } else {
    aRect.SetRect(0,0,0,0);
    return (PR_TRUE);
  }
  if (::XEmptyRegion(mRegion) == True)
    return PR_TRUE;
  else
    return PR_FALSE;

}

PRBool nsRenderingContextUnix :: SetClipRegion(const nsIRegion& aRegion, nsClipCombine aCombine)
{
  nsRect rect;
  XRectangle xrect;

  nsRegionUnix *pRegion = (nsRegionUnix *)&aRegion;
  Region xregion = pRegion->GetXRegion();
  
  ::XClipBox(xregion, &xrect);

  rect.x = xrect.x;
  rect.y = xrect.y;
  rect.width = xrect.width;
  rect.height = xrect.height;

  SetClipRectInPixels(rect, aCombine);

  if (::XEmptyRegion(mRegion) == True)
    return PR_TRUE;
  else
    return PR_FALSE;

}

void nsRenderingContextUnix :: GetClipRegion(nsIRegion **aRegion)
{

  nsIRegion * pRegion ;

  static NS_DEFINE_IID(kCRegionCID, NS_REGION_CID);
  static NS_DEFINE_IID(kIRegionIID, NS_IREGION_IID);

  nsresult rv = nsRepository::CreateInstance(kCRegionCID, 
					     nsnull, 
					     kIRegionIID, 
					     (void **)aRegion);

  if (NS_OK == rv) {
    nsRect rect;
    pRegion = (nsIRegion *)&aRegion;
    pRegion->Init();    
    this->GetClipRect(rect);
    pRegion->Union(rect.x,rect.y,rect.width,rect.height);
  }

  return ;

}

void nsRenderingContextUnix :: SetColor(nscolor aColor)
{
  if (nsnull == mContext) 
    return;

  XGCValues values ;
  mContext->ConvertPixel(aColor, mCurrentColor);

  values.foreground = mCurrentColor;
  values.background = mCurrentColor;

  ::XChangeGC(mRenderingSurface->display,
	            mRenderingSurface->gc,
	            GCForeground | GCBackground,
	            &values);

  mCurrentColor = aColor ;
  
}

nscolor nsRenderingContextUnix :: GetColor() const
{
  return mCurrentColor;
}

nsresult nsRenderingContextUnix :: SetLineStyle(nsLineStyle aLineStyle)
{

  if (aLineStyle != mCurrentLineStyle)
  {
    XGCValues values ;

    switch(aLineStyle)
    {
      case nsLineStyle_kSolid:
        values.line_style = LineSolid;
        ::XChangeGC(mRenderingSurface->display,
	                  mRenderingSurface->gc,
	                  GCLineStyle,
	                  &values);
        break;

      case nsLineStyle_kDashed: {
        static char dashed[2] = {4,4};

        ::XSetDashes(mRenderingSurface->display,
                     mRenderingSurface->gc,
                     0, dashed, 2);
        } break;

      case nsLineStyle_kDotted: {
        static char dotted[2] = {3,1};

        ::XSetDashes(mRenderingSurface->display,
                     mRenderingSurface->gc,
                     0, dotted, 2);
         }break;

      default:
        break;

    }

    mCurrentLineStyle = aLineStyle ;
  }

  return NS_OK;
}

nsresult nsRenderingContextUnix :: GetLineStyle(nsLineStyle &aLineStyle)
{
  aLineStyle = mCurrentLineStyle;
  return NS_OK;
}

void nsRenderingContextUnix :: SetFont(const nsFont& aFont)
{
  NS_IF_RELEASE(mFontMetrics);
  mContext->GetMetricsFor(aFont, mFontMetrics);

  if (mFontMetrics)
  {  
//    mCurrFontHandle = ::XLoadFont(mRenderingSurface->display, (char *)mFontMetrics->GetFontHandle());
    nsFontHandle  fontHandle;
    mFontMetrics->GetFontHandle(fontHandle);
    mCurrFontHandle = (Font)fontHandle;
    
    ::XSetFont(mRenderingSurface->display,
	             mRenderingSurface->gc,
	             mCurrFontHandle);
      
//    ::XFlushGC(mRenderingSurface->display,
//	             mRenderingSurface->gc);
  }
}

void nsRenderingContextUnix :: SetFont(nsIFontMetrics *aFontMetrics)
{
  NS_IF_RELEASE(mFontMetrics);
  mFontMetrics = aFontMetrics;
  NS_IF_ADDREF(mFontMetrics);

  //XXX this code and that in SetFont() above need to be factored
  //into a function. MMP.

  if (mFontMetrics)
  {  
//    mCurrFontHandle = ::XLoadFont(mRenderingSurface->display, (char *)mFontMetrics->GetFontHandle());
    nsFontHandle  fontHandle;
    mFontMetrics->GetFontHandle(fontHandle);
    mCurrFontHandle = (Font)fontHandle;
    
    ::XSetFont(mRenderingSurface->display,
	             mRenderingSurface->gc,
	             mCurrFontHandle);
      
//    ::XFlushGC(mRenderingSurface->display,
//	             mRenderingSurface->gc);
  }
}

const nsFont& nsRenderingContextUnix :: GetFont()
{
  const nsFont* font;
  mFontMetrics->GetFont(font);
  return *font;
}

nsIFontMetrics* nsRenderingContextUnix :: GetFontMetrics()
{
  NS_IF_ADDREF(mFontMetrics);
  return mFontMetrics;
}

// add the passed in translation to the current translation
void nsRenderingContextUnix :: Translate(nscoord aX, nscoord aY)
{
  mTMatrix->AddTranslation((float)aX,(float)aY);
}

// add the passed in scale to the current scale
void nsRenderingContextUnix :: Scale(float aSx, float aSy)
{
  mTMatrix->AddScale(aSx, aSy);
}

nsTransform2D * nsRenderingContextUnix :: GetCurrentTransform()
{
  return mTMatrix;
}

nsDrawingSurface nsRenderingContextUnix :: CreateDrawingSurface(nsRect *aBounds, PRUint32 aSurfFlags)
{
  if (nsnull == mRenderingSurface) {
    return nsnull;
  }
  // Must make sure this code never gets called when nsRenderingSurface is nsnull
  PRUint32 depth = DefaultDepth(mRenderingSurface->display,
				DefaultScreen(mRenderingSurface->display));
  Pixmap p = nsnull;

  PRInt32 w = 200, h = 200; // Use some reasonable defaults

  if (nsnull != aBounds) {
    w = aBounds->width;
    h = aBounds->height;
  }

#ifdef MITSHM

  if (mSupportsSharedPixmaps == PR_TRUE) {

    mRenderingSurface->shmImage = 
      ::XShmCreateImage(mRenderingSurface->display,
			mRenderingSurface->visual, 
			depth, 
			XShmPixmapFormat(mRenderingSurface->display), 
			0, 
			&(mRenderingSurface->shmInfo),
			w,h);
    
    mRenderingSurface->shmInfo.shmid = 
      shmget(IPC_PRIVATE,
	     mRenderingSurface->shmImage->bytes_per_line * 
	     mRenderingSurface->shmImage->height,
	     IPC_CREAT | 0777);
    
    if (mRenderingSurface->shmInfo.shmid >= 0) {
      
      mRenderingSurface->shmInfo.shmaddr = 
	(char *) shmat(mRenderingSurface->shmInfo.shmid, 0, 0);
      
      if (mRenderingSurface->shmInfo.shmaddr != ((char*)-1)) {
	
	mRenderingSurface->shmImage->data = mRenderingSurface->shmInfo.shmaddr;
	mRenderingSurface->shmInfo.readOnly = False;
	XShmAttach(mRenderingSurface->display, &(mRenderingSurface->shmInfo));
	
	p = ::XShmCreatePixmap(mRenderingSurface->display,
			       mRenderingSurface->drawable,
			       mRenderingSurface->shmInfo.shmaddr, 
			       &(mRenderingSurface->shmInfo),
			       mRenderingSurface->shmImage->width,
			       mRenderingSurface->shmImage->height,
			       mRenderingSurface->shmImage->depth);

	::XShmGetImage(mRenderingSurface->display,
		       mRenderingSurface->drawable,
		       mRenderingSurface->shmImage,
		       0,0,AllPlanes);
      }
    }
  }

  // If we failed along the way, just fall back on 
  // old sockets pixmap mechanism....
  if (nsnull == p) {

    if (mRenderingSurface->shmInfo.shmaddr != nsnull) {

      if (mRenderingSurface->shmInfo.shmaddr != ((char*)-1))
	shmdt(mRenderingSurface->shmInfo.shmaddr);

      shmctl(mRenderingSurface->shmInfo.shmid, IPC_RMID,0);

      ::XShmDetach(mRenderingSurface->display, 
		   &(mRenderingSurface->shmInfo));
    }

    if (mRenderingSurface->shmImage != nsnull)
      XDestroyImage(mRenderingSurface->shmImage);
    
    mRenderingSurface->shmImage = nsnull;   
    mRenderingSurface->shmInfo.shmaddr = nsnull;
    
  }

#endif

  if (nsnull == p)
    p  = ::XCreatePixmap(mRenderingSurface->display,
			 mRenderingSurface->drawable,
			 w, h, depth);


  nsDrawingSurfaceUnix * surface = new nsDrawingSurfaceUnix();

  surface->drawable = p ;
  surface->display  = mRenderingSurface->display;
  surface->gc       = mRenderingSurface->gc;
  surface->visual   = mRenderingSurface->visual;
  surface->depth    = mRenderingSurface->depth;

#ifdef MITSHM

  surface->shmInfo = mRenderingSurface->shmInfo;
  surface->shmImage = mRenderingSurface->shmImage;
  mRenderingSurface->shmInfo.shmaddr = nsnull;
  mRenderingSurface->shmImage = nsnull;

#endif

  return ((nsDrawingSurface)surface);
}

void nsRenderingContextUnix :: DestroyDrawingSurface(nsDrawingSurface aDS)
{
  nsDrawingSurfaceUnix * surface = (nsDrawingSurfaceUnix *) aDS;

#ifdef MITSHM
  if (surface->shmImage != nsnull) {

    shmdt(surface->shmInfo.shmaddr);
    shmctl(surface->shmInfo.shmid, IPC_RMID,0);
    ::XShmDetach(surface->display, &(surface->shmInfo));
    XDestroyImage(surface->shmImage);

    surface->shmImage = nsnull;   
    surface->shmInfo.shmaddr = nsnull;
  }
#endif

  ::XFreePixmap(surface->display, surface->drawable);

    if (mRenderingSurface == surface)
      mRenderingSurface = nsnull;

  delete aDS;
}

void nsRenderingContextUnix :: DrawLine(nscoord aX0, nscoord aY0, nscoord aX1, nscoord aY1)
{
  if (nsnull == mTMatrix || nsnull == mRenderingSurface) {
    return;
  }
  mTMatrix->TransformCoord(&aX0,&aY0);
  mTMatrix->TransformCoord(&aX1,&aY1);

  ::XDrawLine(mRenderingSurface->display, 
	      mRenderingSurface->drawable,
	      mRenderingSurface->gc,
	      aX0, aY0, aX1, aY1);
}

void nsRenderingContextUnix :: DrawPolyline(const nsPoint aPoints[], PRInt32 aNumPoints)
{
  if (nsnull == mTMatrix || nsnull == mRenderingSurface) {
    return;
  }
  PRUint32 i ;
  XPoint * xpoints;
  XPoint * thispoint;
  
  xpoints = (XPoint *) PR_Malloc(sizeof(XPoint) * aNumPoints);

  for (i = 0; i < aNumPoints; i++){
    thispoint = (xpoints+i);
    thispoint->x = aPoints[i].x;
    thispoint->y = aPoints[i].y;
    mTMatrix->TransformCoord((PRInt32*)&thispoint->x,(PRInt32*)&thispoint->y);
  }

  ::XDrawLines(mRenderingSurface->display,
	       mRenderingSurface->drawable,
	       mRenderingSurface->gc,
	       xpoints, aNumPoints, CoordModeOrigin);

  PR_Free((void *)xpoints);
}

void nsRenderingContextUnix :: DrawRect(const nsRect& aRect)
{
  DrawRect(aRect.x, aRect.y, aRect.width, aRect.height);
}

void nsRenderingContextUnix :: DrawRect(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight)
{
  if (nsnull == mTMatrix || nsnull == mRenderingSurface) {
    return;
  }

  nscoord x,y,w,h;

  x = aX;
  y = aY;
  w = aWidth;
  h = aHeight;

  mTMatrix->TransformCoord(&x,&y,&w,&h);

  ::XDrawRectangle(mRenderingSurface->display, 
		   mRenderingSurface->drawable,
		   mRenderingSurface->gc,
		   x,y,w,h);
}

void nsRenderingContextUnix :: FillRect(const nsRect& aRect)
{
  FillRect(aRect.x, aRect.y, aRect.width, aRect.height);
}

void nsRenderingContextUnix :: FillRect(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight)
{
  if (nsnull == mTMatrix || nsnull == mRenderingSurface) {
    return;
  }
  nscoord x,y,w,h;
  x = aX;
  y = aY;
  w = aWidth;
  h = aHeight;

  mTMatrix->TransformCoord(&x,&y,&w,&h);
  ::XFillRectangle(mRenderingSurface->display, 
		   mRenderingSurface->drawable,
		   mRenderingSurface->gc,
		   x,y,w,h);

}


void nsRenderingContextUnix::DrawPolygon(const nsPoint aPoints[], PRInt32 aNumPoints)
{
  if (nsnull == mTMatrix || nsnull == mRenderingSurface) {
    return;
  }
  PRUint32 i ;
  XPoint * xpoints;
  XPoint * thispoint;
  
  xpoints = (XPoint *) PR_Malloc(sizeof(XPoint) * aNumPoints);

  for (i = 0; i < aNumPoints; i++){
    thispoint = (xpoints+i);
    thispoint->x = aPoints[i].x;
    thispoint->y = aPoints[i].y;
    mTMatrix->TransformCoord((PRInt32*)&thispoint->x,(PRInt32*)&thispoint->y);
  }

  ::XDrawLines(mRenderingSurface->display,
	       mRenderingSurface->drawable,
	       mRenderingSurface->gc,
	       xpoints, aNumPoints, CoordModeOrigin);

  PR_Free((void *)xpoints);
}

void nsRenderingContextUnix::FillPolygon(const nsPoint aPoints[], PRInt32 aNumPoints)
{
  if (nsnull == mTMatrix || nsnull == mRenderingSurface) {
    return;
  }
  PRUint32 i ;
  XPoint * xpoints;
  XPoint * thispoint;
  nscoord x,y;
  
  xpoints = (XPoint *) PR_Malloc(sizeof(XPoint) * aNumPoints);

  for (i = 0; i < aNumPoints; i++){
    thispoint = (xpoints+i);
    x = aPoints[i].x;
    y = aPoints[i].y;
    mTMatrix->TransformCoord(&x,&y);
    thispoint->x = x;
    thispoint->y = y;
  }

  ::XFillPolygon(mRenderingSurface->display,
		 mRenderingSurface->drawable,
		 mRenderingSurface->gc,
		 xpoints, aNumPoints, Convex, CoordModeOrigin);

  PR_Free((void *)xpoints);
}

void nsRenderingContextUnix :: DrawEllipse(const nsRect& aRect)
{
  DrawEllipse(aRect.x, aRect.y, aRect.width, aRect.height);
}

void nsRenderingContextUnix :: DrawEllipse(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight)
{
  if (nsnull == mTMatrix || nsnull == mRenderingSurface) {
    return;
  }
  nscoord x,y,w,h;

  x = aX;
  y = aY;
  w = aWidth;
  h = aHeight;

  mTMatrix->TransformCoord(&x,&y,&w,&h);

  ::XDrawArc(mRenderingSurface->display, 
	     mRenderingSurface->drawable,
	     mRenderingSurface->gc,
	     x,y,w,h, 0, 360 * 64);
}

void nsRenderingContextUnix :: FillEllipse(const nsRect& aRect)
{
  FillEllipse(aRect.x, aRect.y, aRect.width, aRect.height);
}

void nsRenderingContextUnix :: FillEllipse(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight)
{
  if (nsnull == mTMatrix || nsnull == mRenderingSurface) {
    return;
  }
  nscoord x,y,w,h;

  x = aX;
  y = aY;
  w = aWidth;
  h = aHeight;

  mTMatrix->TransformCoord(&x,&y,&w,&h);

  ::XFillArc(mRenderingSurface->display, 
	     mRenderingSurface->drawable,
	     mRenderingSurface->gc,
	     x,y,w,h, 0, 360 * 64);
}

void nsRenderingContextUnix :: DrawArc(const nsRect& aRect,
                                 float aStartAngle, float aEndAngle)
{
  this->DrawArc(aRect.x,aRect.y,aRect.width,aRect.height,aStartAngle,aEndAngle);
}

void nsRenderingContextUnix :: DrawArc(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight,
                                 float aStartAngle, float aEndAngle)
{
  if (nsnull == mTMatrix || nsnull == mRenderingSurface) {
    return;
  }
  nscoord x,y,w,h;

  x = aX;
  y = aY;
  w = aWidth;
  h = aHeight;

  mTMatrix->TransformCoord(&x,&y,&w,&h);

  ::XDrawArc(mRenderingSurface->display, 
	     mRenderingSurface->drawable,
	     mRenderingSurface->gc,
	     x,y,w,h, NSToIntRound(aStartAngle * 64.0f),
             NSToIntRound(aEndAngle * 64.0f));
}

void nsRenderingContextUnix :: FillArc(const nsRect& aRect,
                                 float aStartAngle, float aEndAngle)
{
  this->FillArc(aRect.x, aRect.y, aRect.width, aRect.height, aStartAngle, aEndAngle);
}

void nsRenderingContextUnix :: FillArc(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight,
                                 float aStartAngle, float aEndAngle)
{
  if (nsnull == mTMatrix || nsnull == mRenderingSurface) {
    return;
  }
  nscoord x,y,w,h;

  x = aX;
  y = aY;
  w = aWidth;
  h = aHeight;

  mTMatrix->TransformCoord(&x,&y,&w,&h);

  ::XFillArc(mRenderingSurface->display, 
	     mRenderingSurface->drawable,
	     mRenderingSurface->gc,
	     x,y,w,h, NSToIntRound(aStartAngle * 64.0f),
             NSToIntRound(aEndAngle * 64.0f));
}

NS_IMETHODIMP nsRenderingContextUnix :: GetWidth(char ch, nscoord &aWidth)
{
  char buf[1];
  buf[0] = ch;
  return GetWidth(buf, 1, aWidth);
}

NS_IMETHODIMP nsRenderingContextUnix :: GetWidth(PRUnichar ch, nscoord &aWidth)
{
  PRUnichar buf[1];
  buf[0] = ch;
  return GetWidth(buf, 1, aWidth);
}

NS_IMETHODIMP nsRenderingContextUnix :: GetWidth(const nsString& aString, nscoord &aWidth)
{
  return GetWidth(aString.GetUnicode(), aString.Length(), aWidth);
}

NS_IMETHODIMP nsRenderingContextUnix :: GetWidth(const char *aString, nscoord &aWidth)
{
  return GetWidth(aString, strlen(aString), aWidth);
}

NS_IMETHODIMP nsRenderingContextUnix :: GetWidth(const char *aString,
                                            PRUint32 aLength, nscoord &aWidth)
{
  PRInt32     rc;
  XFontStruct *font;
  
  font = ::XQueryFont(mRenderingSurface->display, (Font)mCurrFontHandle);
  rc = (PRInt32) ::XTextWidth(font, aString, aLength);
  aWidth = nscoord(rc * mP2T);

  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextUnix :: GetWidth(const PRUnichar *aString,
                                            PRUint32 aLength,
                                            nscoord &aWidth)
{
  XChar2b * thischar ;
  PRUint16 aunichar;
  nscoord width ;
  PRUint32 i ;
  PRUint32 desiredSize = sizeof(XChar2b) * aLength;
  XFontStruct *font;

  // Make the temporary buffer larger if needed.
  if (nsnull == mDrawStringBuf) {
    mDrawStringBuf = (XChar2b *) PR_Malloc(desiredSize);
    mDrawStringSize = aLength;
  }
  else {
    if (mDrawStringSize < PRInt32(aLength)) {
      mDrawStringBuf = (XChar2b *) PR_Realloc(mDrawStringBuf, desiredSize);
      mDrawStringSize = aLength;
    }
  }

  // Translate the unicode data into XChar2b's
  XChar2b* xc = mDrawStringBuf;
  XChar2b* end = xc + aLength;
  while (xc < end) {
    PRUnichar ch = *aString++;
    xc->byte2 = (ch & 0xff);
    xc->byte1 = (ch & 0xff00) >> 8;
    xc++;
  }
  
  font = ::XQueryFont(mRenderingSurface->display, (Font)mCurrFontHandle);
  width = ::XTextWidth16(font, mDrawStringBuf, aLength);
  aWidth = nscoord(width * mP2T);

  return NS_OK;
}

void
nsRenderingContextUnix :: DrawString(const char *aString, PRUint32 aLength,
                                     nscoord aX, nscoord aY,
                                     nscoord aWidth,
                                     const nscoord* aSpacing)
{
  nscoord x = aX;
  nscoord y = aY;

  // Substract xFontStruct ascent since drawing specifies baseline
  if (mFontMetrics) {	  
	mFontMetrics->GetMaxAscent(y);
	y+=aY;
  }

  mTMatrix->TransformCoord(&x,&y);

  ::XDrawString(mRenderingSurface->display, 
		mRenderingSurface->drawable,
		mRenderingSurface->gc,
		x, y, aString, aLength);

  if (mFontMetrics)
  {
    PRUint8 deco = GetFont().decorations;

    if (deco & NS_FONT_DECORATION_OVERLINE)
      DrawLine(aX, aY, aX + aWidth, aY);

    if (deco & NS_FONT_DECORATION_UNDERLINE)
    {
      nscoord ascent,descent;

	  mFontMetrics->GetMaxAscent(ascent);
      mFontMetrics->GetMaxDescent(descent);

      DrawLine(aX, aY + ascent + (descent >> 1),
               aX + aWidth, aY + ascent + (descent >> 1));
    }

    if (deco & NS_FONT_DECORATION_LINE_THROUGH)
    {
      nscoord height;
	  
	  mFontMetrics->GetHeight(height);

      DrawLine(aX, aY + (height >> 1), aX + aWidth, aY + (height >> 1));
    }
  }
}

void
nsRenderingContextUnix :: DrawString(const PRUnichar *aString, PRUint32 aLength,
                                     nscoord aX, nscoord aY, nscoord aWidth,
                                     const nscoord* aSpacing)
{
  nscoord x = aX;
  nscoord y = aY;
  PRUint32 desiredSize = sizeof(XChar2b) * aLength;

  // Substract xFontStruct ascent since drawing specifies baseline
  if (mFontMetrics) {
    mFontMetrics->GetMaxAscent(y);
	y+=aY;
  }

  mTMatrix->TransformCoord(&x, &y);

  // Make the temporary buffer larger if needed.
  if (nsnull == mDrawStringBuf) {
    mDrawStringBuf = (XChar2b *) PR_Malloc(desiredSize);
    mDrawStringSize = aLength;
  }
  else {
    if (mDrawStringSize < PRInt32(aLength)) {
      mDrawStringBuf = (XChar2b *) PR_Realloc(mDrawStringBuf, desiredSize);
      mDrawStringSize = aLength;
    }
  }

  // Translate the unicode data into XChar2b's
  XChar2b* xc = mDrawStringBuf;
  XChar2b* end = xc + aLength;
  while (xc < end) {
    PRUnichar ch = *aString++;
    xc->byte2 = (ch & 0xff);
    xc->byte1 = (ch & 0xff00) >> 8;
    xc++;
  }

  ::XDrawString16(mRenderingSurface->display, 
                mRenderingSurface->drawable,
                mRenderingSurface->gc,
                x, y, mDrawStringBuf, aLength);

  if (mFontMetrics)
  {
    PRUint8 deco = GetFont().decorations;

    if (deco & NS_FONT_DECORATION_OVERLINE)
      DrawLine(aX, aY, aX + aWidth, aY);

    if (deco & NS_FONT_DECORATION_UNDERLINE)
    {
      nscoord ascent,descent;

	  mFontMetrics->GetMaxAscent(ascent);
      mFontMetrics->GetMaxDescent(descent);

      DrawLine(aX, aY + ascent + (descent >> 1),
               aX + aWidth, aY + ascent + (descent >> 1));
    }

    if (deco & NS_FONT_DECORATION_LINE_THROUGH)
    {
      nscoord height;

	  mFontMetrics->GetHeight(height);

      DrawLine(aX, aY + (height >> 1), aX + aWidth, aY + (height >> 1));
    }
  }
}

void
nsRenderingContextUnix :: DrawString(const nsString& aString,
                                     nscoord aX, nscoord aY, nscoord aWidth,
                                     const nscoord* aSpacing)
{
  DrawString(aString.GetUnicode(), aString.Length(), aX, aY, aWidth, aSpacing);
}

void nsRenderingContextUnix :: DrawImage(nsIImage *aImage, nscoord aX, nscoord aY)
{
  nscoord width,height;
  width = NSToCoordRound(mP2T * aImage->GetWidth());
  height = NSToCoordRound(mP2T * aImage->GetHeight());
  
  this->DrawImage(aImage,aX,aY,width,height);
}

void nsRenderingContextUnix :: DrawImage(nsIImage *aImage, nscoord aX, nscoord aY,
                                        nscoord aWidth, nscoord aHeight) 
{
  nsRect	tr;

  tr.x = aX;
  tr.y = aY;
  tr.width = aWidth;
  tr.height = aHeight;
  this->DrawImage(aImage,tr);
}

void nsRenderingContextUnix :: DrawImage(nsIImage *aImage, const nsRect& aSRect, const nsRect& aDRect)
{
  nsRect	sr,dr;
  
  sr = aSRect;
  mTMatrix ->TransformCoord(&sr.x,&sr.y,&sr.width,&sr.height);

  dr = aDRect;
  mTMatrix->TransformCoord(&dr.x,&dr.y,&dr.width,&dr.height);
  
  ((nsImageUnix*)aImage)->Draw(*this,mRenderingSurface,sr.x,sr.y,sr.width,sr.height,
                                 dr.x,dr.y,dr.width,dr.height);
}

void nsRenderingContextUnix :: DrawImage(nsIImage *aImage, const nsRect& aRect)
{
  nsRect	tr;

  tr = aRect;
  mTMatrix->TransformCoord(&tr.x,&tr.y,&tr.width,&tr.height);
  
  if (aImage != nsnull) {
    ((nsImageUnix*)aImage)->Draw(*this,mRenderingSurface,tr.x,tr.y,tr.width,tr.height);
  } 
}

NS_IMETHODIMP
nsRenderingContextUnix :: CopyOffScreenBits(nsDrawingSurface aSrcSurf,
                                            PRInt32 aSrcX, PRInt32 aSrcY,
                                            const nsRect &aDestBounds,
                                            PRUint32 aCopyFlags)
{
  PRInt32               x = aSrcX;
  PRInt32               y = aSrcY;
  nsRect                drect = aDestBounds;
  nsDrawingSurfaceUnix  *destsurf;

  if (aCopyFlags & NS_COPYBITS_TO_BACK_BUFFER)
  {
    NS_ASSERTION(!(nsnull == mRenderingSurface), "no back buffer");
    destsurf = mRenderingSurface;
  }
  else
    destsurf = mFrontBuffer;

  if (aCopyFlags & NS_COPYBITS_XFORM_SOURCE_VALUES)
    mTMatrix->TransformCoord(&x, &y);

  if (aCopyFlags & NS_COPYBITS_XFORM_DEST_VALUES)
    mTMatrix->TransformCoord(&drect.x, &drect.y, &drect.width, &drect.height);

  //XXX flags are unused. that would seem to mean that there is
  //inefficiency somewhere... MMP

  ::XCopyArea(((nsDrawingSurfaceUnix *)aSrcSurf)->display, 
	            ((nsDrawingSurfaceUnix *)aSrcSurf)->drawable,
	            destsurf->drawable,
	            ((nsDrawingSurfaceUnix *)aSrcSurf)->gc,
	            x, y, drect.width, drect.height,
              drect.x, drect.y);

  return NS_OK;
}















