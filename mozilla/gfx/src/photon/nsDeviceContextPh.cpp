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

#include <math.h>

#include "nspr.h"
#include "nsIPref.h"
#include "nsIServiceManager.h"
#include "nsCRT.h"
#include "nsReadableUtils.h"

#include "nsDeviceContextPh.h"
#include "nsRenderingContextPh.h"
#include "nsDeviceContextSpecPh.h"
#include "nsHashtable.h"

#include "nsPhGfxLog.h"

static NS_DEFINE_CID(kPrefCID, NS_PREF_CID);

#define NS_TO_PH_RGB(ns) (ns & 0xff) << 16 | (ns & 0xff00) | ((ns >> 16) & 0xff)

nscoord nsDeviceContextPh::mDpi = 96;

static nsHashtable* mFontLoadCache = nsnull;

nsDeviceContextPh :: nsDeviceContextPh( )
  : DeviceContextImpl()
  {
  mTwipsToPixels = 1.0;
  mPixelsToTwips = 1.0;
  mDepth = 0 ;
  mSurface = NULL;
  mPixelScale = 1.0f;
  mWidthFloat = 0.0f;
  mHeightFloat = 0.0f;

  /* These *MUST* be -1 */
  mWidth = -1;
  mHeight = -1;

  mSpec = nsnull;
  mDC = nsnull;

  mIsPrinting = 0;
	}

nsDeviceContextPh :: ~nsDeviceContextPh( ) {
  nsDrawingSurfacePh *surf = (nsDrawingSurfacePh *)mSurface;

  NS_IF_RELEASE(surf);    //this clears the surf pointer...
  mSurface = nsnull;

	if( mFontLoadCache ) { 
		delete mFontLoadCache;
		mFontLoadCache = nsnull;
		}
	NS_IF_RELEASE( mSpec );
	}

NS_IMETHODIMP nsDeviceContextPh :: Init( nsNativeWidget aWidget ) {
  float newscale, origscale;
  float a2d,t2d;
    
  CommonInit(NULL);
 
  GetTwipsToDevUnits(newscale);
  GetAppUnitsToDevUnits(origscale);

  GetTwipsToDevUnits(t2d);
  GetAppUnitsToDevUnits(a2d);

  // Call my base class
  return DeviceContextImpl::Init( aWidget );
	}


/* Called for Printing */
nsresult nsDeviceContextPh :: Init( nsNativeDeviceContext aContext, nsIDeviceContext *aOrigContext ) {
  float                  origscale, newscale, t2d, a2d;
    
  mDC = aContext;

  CommonInit(mDC);

  GetTwipsToDevUnits(newscale);
  aOrigContext->GetAppUnitsToDevUnits(origscale);
  
  mPixelScale = newscale / origscale;

  aOrigContext->GetTwipsToDevUnits(t2d);
  aOrigContext->GetAppUnitsToDevUnits(a2d);

  mAppUnitsToDevUnits = (a2d / t2d) * mTwipsToPixels;
  mDevUnitsToAppUnits = 1.0f / mAppUnitsToDevUnits;

	// for printers
	const PhDim_t *psize;
	PhDim_t dim;
	PpPrintContext_t *pc = ((nsDeviceContextSpecPh *)mSpec)->GetPrintContext();

	PpPrintGetPC(pc, Pp_PC_PAPER_SIZE, (const void **)&psize );
	mWidthFloat = (float)(psize->w / 10);
	mHeightFloat = (float)(psize->h / 10);
	dim.w = psize->w / 10;
	dim.h = psize->h / 10;
	PpPrintSetPC(pc, INITIAL_PC, 0 , Pp_PC_SOURCE_SIZE, &dim );
	return NS_OK;
	}

void nsDeviceContextPh :: GetPrinterRect( int *width, int *height ) {
	PhDim_t dim;
	const PhDim_t *psize;
	const PhRect_t 	*non_print;
	PhRect_t	rect, margins;
	const char *orientation = 0;
	int			tmp;
	PpPrintContext_t *pc = ((nsDeviceContextSpecPh *)mSpec)->GetPrintContext();
	
	memset( &rect, 0, sizeof(rect));
	memset( &margins, 0, sizeof(margins));

	PpPrintGetPC(pc, Pp_PC_PAPER_SIZE, (const void **)&psize );
	PpPrintGetPC(pc, Pp_PC_NONPRINT_MARGINS, (const void **)&non_print );
	dim.w = (psize->w - ( non_print->ul.x + non_print->lr.x )) * 100 / 1000;
	dim.h = (psize->h - ( non_print->ul.x + non_print->lr.x )) * 100 / 1000;

	PpPrintGetPC(pc, Pp_PC_ORIENTATION, (const void **)&orientation );

	if( *orientation ) {
		tmp = dim.w;
		dim.w = dim.h;
		dim.h = tmp;
		}

	/* set these to 0 since we do the margins */
	PpPrintSetPC(pc, INITIAL_PC, 0 , Pp_PC_MARGINS, &rect ); 
	PpPrintSetPC(pc, INITIAL_PC, 0 , Pp_PC_SOURCE_SIZE, &dim );

	*width = dim.w;
	*height = dim.h;
	}


void nsDeviceContextPh :: CommonInit( nsNativeDeviceContext aDC ) {
  PRInt32           aWidth, aHeight;
  static int        initialized = 0;


	if( !mScreenManager ) mScreenManager = do_GetService("@mozilla.org/gfx/screenmanager;1");

  if( !initialized ) {
    initialized = 1;
    // Set prefVal the value of the preference "browser.display.screen_resolution"
    // or -1 if we can't get it.
    // If it's negative, we pretend it's not set.
    // If it's 0, it means force use of the operating system's logical resolution.
    // If it's positive, we use it as the logical resolution
    PRInt32 prefVal = -1;
    nsresult res;

    nsCOMPtr<nsIPref> prefs(do_GetService(kPrefCID, &res));
    if( NS_SUCCEEDED( res ) && prefs ) {
      res = prefs->GetIntPref("browser.display.screen_resolution", &prefVal);
      if( NS_FAILED( res ) ) {
        prefVal = 96;
      	}

      prefs->RegisterCallback( "browser.display.screen_resolution", prefChanged, (void *)this );
      if( prefVal >0 ) mDpi = prefVal;
    	}
  	}

  SetDPI( mDpi ); 

	GetDisplayInfo(aWidth, aHeight, mDepth);

	/* Turn off virtual console support... */
	mWidthFloat  = (float) aWidth;
	mHeightFloat = (float) aHeight;
    
  /* Revisit: the scroll bar sizes is a gross guess based on Phab */
  mScrollbarHeight = 17;
  mScrollbarWidth  = 17;
  
  /* Call Base Class */
  DeviceContextImpl::CommonInit( );
	}

NS_IMETHODIMP nsDeviceContextPh :: CreateRenderingContext( nsIRenderingContext *&aContext ) {
  nsIRenderingContext *pContext;
  nsresult             rv;
  nsDrawingSurfacePh  *surf;
   
	pContext = new nsRenderingContextPh();
	
	if( nsnull != pContext ) {
	  NS_ADDREF(pContext);

	  surf = new nsDrawingSurfacePh();
	  if( nsnull != surf ) {
			rv = surf->Init();
			if( NS_OK == rv ) rv = pContext->Init(this, surf);
			else rv = NS_ERROR_OUT_OF_MEMORY;
			}
		}
	else rv = NS_ERROR_OUT_OF_MEMORY;

	if( NS_OK != rv ) NS_IF_RELEASE( pContext );

	aContext = pContext;
	NS_ADDREF( pContext ); // otherwise it's crashing after printing
	return rv;
	}

NS_IMETHODIMP nsDeviceContextPh :: SupportsNativeWidgets( PRBool &aSupportsWidgets ) {
/* REVISIT: These needs to return FALSE if we are printing! */
  if( nsnull == mDC ) aSupportsWidgets = PR_TRUE;
	else aSupportsWidgets = PR_FALSE;		/* while printing! */
  return NS_OK;
	}

NS_IMETHODIMP nsDeviceContextPh :: GetScrollBarDimensions( float &aWidth, float &aHeight ) const {
  /* Revisit: the scroll bar sizes is a gross guess based on Phab */
  float scale;
  GetCanonicalPixelScale(scale);
  aWidth = mScrollbarWidth * mPixelsToTwips * scale;
  aHeight = mScrollbarHeight * mPixelsToTwips * scale;
  return NS_OK;
	}

NS_IMETHODIMP nsDeviceContextPh :: GetSystemFont( nsSystemFontID aID, nsFont *aFont) const {
  switch (aID) {
    case eSystemFont_Caption:      // css2
	case eSystemFont_Icon:
	case eSystemFont_Menu:
	case eSystemFont_MessageBox:
	case eSystemFont_SmallCaption:
	case eSystemFont_StatusBar:
	case eSystemFont_Window:       // css3
	case eSystemFont_Document:
	case eSystemFont_Workspace:
	case eSystemFont_Desktop:
	case eSystemFont_Info:
	case eSystemFont_Dialog:
	case eSystemFont_Button:
	case eSystemFont_PullDownMenu:
	case eSystemFont_List:
	case eSystemFont_Field:
	case eSystemFont_Tooltips:     // moz
	case eSystemFont_Widget:
	  aFont->style       = NS_FONT_STYLE_NORMAL;
	  aFont->weight      = NS_FONT_WEIGHT_NORMAL;
	  aFont->decorations = NS_FONT_DECORATION_NONE;
	  aFont->size = NSIntPointsToTwips(8);
	  aFont->name.Assign(NS_LITERAL_STRING("TextFont"));
	  switch(aID) {
		  case eSystemFont_MessageBox:
		     aFont->name.Assign(NS_LITERAL_STRING("MessageFont"));
			 break;
		  case eSystemFont_Tooltips:
		     aFont->name.Assign(NS_LITERAL_STRING("BalloonFont"));
		     break;
		  case eSystemFont_Menu:
		     aFont->name.Assign(NS_LITERAL_STRING("MenuFont"));
		     break;
	  }
	  break;
  }

  return NS_OK;
}

NS_IMETHODIMP nsDeviceContextPh :: GetDrawingSurface( nsIRenderingContext &aContext, nsDrawingSurface &aSurface ) {
  aContext.CreateDrawingSurface(nsnull, 0, aSurface);
  return nsnull == aSurface ? NS_ERROR_OUT_OF_MEMORY : NS_OK;
	}

NS_IMETHODIMP nsDeviceContextPh :: GetClientRect( nsRect &aRect ) {
	nsresult rv = NS_OK;
	if( mIsPrinting ) { //( mSpec )
	  // we have a printer device
	  aRect.x = 0;
	  aRect.y = 0;
	  aRect.width = NSToIntRound(mWidth * mDevUnitsToAppUnits);
	  aRect.height = NSToIntRound(mHeight * mDevUnitsToAppUnits);
		}
	else rv = GetRect ( aRect );
	return rv;
	}

/* I need to know the requested font size to finish this function */
NS_IMETHODIMP nsDeviceContextPh :: CheckFontExistence( const nsString& aFontName ) {
  nsresult    ret_code = NS_ERROR_FAILURE;
  char        *fontName = ToNewCString(aFontName);

  if( fontName ) {
		FontID *id = NULL;

#if (Ph_LIB_VERSION > 200) // a header changed in RTP 6.2
		if( ( id = PfFindFont( (char *)fontName, 0, 0 ) ) ) {
#else
		if( ( id = PfFindFont( (uchar_t *)fontName, 0, 0 ) ) ) {
#endif
			if( !mFontLoadCache ) mFontLoadCache = new nsHashtable();

			nsCStringKey key((char *)(PfConvertFontID(id)));
			if( !mFontLoadCache->Exists( &key ) ) {
				char FullFontName[MAX_FONT_TAG];
#if (Ph_LIB_VERSION > 200) // a header changed in RTP 6.2
				PfGenerateFontName((char  *)fontName, nsnull, 8, (char *)FullFontName);
#else
				PfGenerateFontName((uchar_t  *)fontName, nsnull, 8, (uchar_t *)FullFontName);
#endif
				PfLoadFont(FullFontName, PHFONT_LOAD_METRICS, nsnull);
				PfLoadMetrics(FullFontName);
				// add this font to the table
				mFontLoadCache->Put(&key, nsnull);
				}
	
			ret_code = NS_OK;
			PfFreeFont(id);
			}
		delete [] fontName;
		}

	/* Return ok and we will map it to some other font later */  
	return ret_code;
	}

NS_IMETHODIMP nsDeviceContextPh::GetDepth( PRUint32& aDepth ) {
  aDepth = mDepth; // 24;
  return NS_OK;
	}


NS_IMETHODIMP nsDeviceContextPh :: ConvertPixel( nscolor aColor, PRUint32 & aPixel ) {
  aPixel = NS_TO_PH_RGB(aColor);
  return NS_OK;
	}

NS_IMETHODIMP nsDeviceContextPh :: GetDeviceSurfaceDimensions( PRInt32 &aWidth, PRInt32 &aHeight ) {
	if( mIsPrinting ) { //(mSpec)
		aWidth = NSToIntRound(mWidthFloat * mDevUnitsToAppUnits);
		aHeight = NSToIntRound(mHeightFloat * mDevUnitsToAppUnits);
		return NS_OK;
		}

  if( mWidth == -1 ) mWidth = NSToIntRound(mWidthFloat * mDevUnitsToAppUnits);
  if( mHeight == -1 ) mHeight = NSToIntRound(mHeightFloat * mDevUnitsToAppUnits);

  aWidth = mWidth;
  aHeight = mHeight;

  return NS_OK;
	}

NS_IMETHODIMP nsDeviceContextPh::GetRect( nsRect &aRect ) {
	if( mScreenManager ) {
    nsCOMPtr<nsIScreen> screen;
    mScreenManager->GetPrimaryScreen( getter_AddRefs( screen ) );
    screen->GetRect(&aRect.x, &aRect.y, &aRect.width, &aRect.height);
    aRect.x = NSToIntRound(mDevUnitsToAppUnits * aRect.x);
    aRect.y = NSToIntRound(mDevUnitsToAppUnits * aRect.y);
    aRect.width = NSToIntRound(mDevUnitsToAppUnits * aRect.width);
    aRect.height = NSToIntRound(mDevUnitsToAppUnits * aRect.height);
		}
	else {
  	PRInt32 width, height;
  	GetDeviceSurfaceDimensions( width, height );
  	aRect.x = 0;
  	aRect.y = 0;
  	aRect.width = width;
  	aRect.height = height;
		}
  return NS_OK;
	}

NS_IMETHODIMP nsDeviceContextPh :: GetDeviceContextFor( nsIDeviceContextSpec *aDevice, nsIDeviceContext *&aContext ) {
	nsDeviceContextPh* devConPh = new nsDeviceContextPh(); //ref count 0
	if (devConPh != nsnull) {
  		// this will ref count it
    	nsresult rv = devConPh->QueryInterface(NS_GET_IID(nsIDeviceContext), (void**)&aContext);
	  	NS_ASSERTION(NS_SUCCEEDED(rv), "This has to support nsIDeviceContext");
	} else {
	    return NS_ERROR_OUT_OF_MEMORY;
	}

	devConPh->mSpec = aDevice;
	NS_ADDREF(aDevice);
	return devConPh->Init(NULL, this);
	}

nsresult nsDeviceContextPh::SetDPI( PRInt32 aDpi ) {
  const int pt2t = 72;

  mDpi = aDpi;
    
  // make p2t a nice round number - this prevents rounding problems
  mPixelsToTwips = float(NSToIntRound(float(NSIntPointsToTwips(pt2t)) / float(aDpi)));
  mTwipsToPixels = 1.0f / mPixelsToTwips;
  // XXX need to reflow all documents
  return NS_OK;
	}

int nsDeviceContextPh::prefChanged( const char *aPref, void *aClosure ) {
  nsDeviceContextPh *context = (nsDeviceContextPh*)aClosure;
  nsresult rv;

  if( nsCRT::strcmp(aPref, "browser.display.screen_resolution")==0 )  {
    PRInt32 dpi;
    nsCOMPtr<nsIPref> prefs(do_GetService(kPrefCID, &rv));
    rv = prefs->GetIntPref(aPref, &dpi);
    if( NS_SUCCEEDED( rv ) ) context->SetDPI( dpi ); 
		}
  return 0;
	}

NS_IMETHODIMP nsDeviceContextPh :: BeginDocument( PRUnichar *t, PRUnichar* aPrintToFileName, PRInt32 aStartPage, PRInt32 aEndPage ) {
  PpPrintContext_t *pc = ((nsDeviceContextSpecPh *)mSpec)->GetPrintContext();
  PpStartJob(pc);
    mIsPrinting = 1;
    mIsPrintingStart = 1;
  return NS_OK;
    }

NS_IMETHODIMP nsDeviceContextPh :: EndDocument( void ) {
  PpPrintContext_t *pc = ((nsDeviceContextSpecPh *)mSpec)->GetPrintContext();
  PpEndJob(pc);
	mIsPrinting = 0;
  return NS_OK;
	}

NS_IMETHODIMP nsDeviceContextPh :: AbortDocument( void ) {
  return EndDocument();
	}

NS_IMETHODIMP nsDeviceContextPh :: BeginPage( void ) {
	PpPrintContext_t *pc = ((nsDeviceContextSpecPh *)mSpec)->GetPrintContext();
	if( !mIsPrintingStart ) PpPrintNewPage( pc );
	PpContinueJob( pc );
	mIsPrintingStart = 0;
	return NS_OK;
    }

NS_IMETHODIMP nsDeviceContextPh :: EndPage( void ) {
	PpPrintContext_t *pc = ((nsDeviceContextSpecPh *)mSpec)->GetPrintContext();
	PpSuspendJob(pc);
	return NS_OK;
    }

int nsDeviceContextPh :: IsPrinting( void ) { return mIsPrinting ? 1 : 0; }

/*
 Get the size and color depth of the display
 */
nsresult nsDeviceContextPh :: GetDisplayInfo( PRInt32 &aWidth, PRInt32 &aHeight, PRUint32 &aDepth ) {
  nsresult    			res = NS_ERROR_FAILURE;
  PhSysInfo_t       SysInfo;
  PhRect_t          rect;
  char              *p = NULL;
  int               inp_grp;
  PhRid_t           rid;

  /* Initialize variables */
  aWidth  = 0;
  aHeight = 0;
  aDepth  = 0;
  
	/* Get the Screen Size and Depth*/
	p = getenv("PHIG");
	if( p ) inp_grp = atoi( p );
	else inp_grp = 1;

	PhQueryRids( 0, 0, inp_grp, Ph_GRAFX_REGION, 0, 0, 0, &rid, 1 );
	PhWindowQueryVisible( Ph_QUERY_INPUT_GROUP | Ph_QUERY_EXACT, 0, inp_grp, &rect );
	aWidth  = rect.lr.x - rect.ul.x + 1;
	aHeight = rect.lr.y - rect.ul.y + 1;  

	/* Get the System Info for the RID */
	if( PhQuerySystemInfo( rid, NULL, &SysInfo ) ) {
		/* Make sure the "color_bits" field is valid */
		if( SysInfo.gfx.valid_fields & Ph_GFX_COLOR_BITS ) {
			aDepth = SysInfo.gfx.color_bits;
			res = NS_OK;
			}
		}
  return res;
	}
