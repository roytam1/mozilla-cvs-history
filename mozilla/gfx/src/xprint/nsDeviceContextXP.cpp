/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
 * Roland Mainz <roland.mainz@informatik.med.uni-giessen.de>
 * Bradley Baetz <bbaetz@cs.mcgill.ca>
 *
 */
 
#include <strings.h>
#include <unistd.h>
#include <X11/Xatom.h>

#include "nsDeviceContextXP.h"
#include "nsRenderingContextXP.h"
#include "nsString.h"
#include "nsFontMetricsXP.h"
#include "xlibrgb.h"
#include "nsIDeviceContext.h"
#include "nsIDeviceContextSpecXPrint.h"
#include "il_util.h"
#include "nspr.h"
#include "nsXPrintContext.h"

#ifdef PR_LOGGING
static PRLogModuleInfo *nsDeviceContextXpLM = PR_NewLogModule("nsDeviceContextXp");
#endif /* PR_LOGGING */

static NS_DEFINE_IID(kIDeviceContextSpecXPIID, NS_IDEVICE_CONTEXT_SPEC_XP_IID);

/** ---------------------------------------------------
 *  See documentation in nsIDeviceContext.h
 */
nsDeviceContextXp :: nsDeviceContextXp()
{
  NS_INIT_REFCNT();
  /* Inherited from xlib device context code */
  //mTwipsToPixels               = 1.0;
  //mPixelsToTwips               = 1.0;
  mPrintContext                = nsnull;
  mSpec                        = nsnull; 
  mParentDeviceContext         = nsnull;

  NS_NewISupportsArray(getter_AddRefs(mFontMetrics));
}

/** ---------------------------------------------------
 *  See documentation in nsIDeviceContext.h
 *        @update 12/21/98 dwc
 */
nsDeviceContextXp :: ~nsDeviceContextXp() 
{ 
  NS_IF_RELEASE(mParentDeviceContext);
}


NS_IMETHODIMP
nsDeviceContextXp :: SetSpec(nsIDeviceContextSpec* aSpec)
{
  nsresult  rv = NS_ERROR_FAILURE;

  PR_LOG(nsDeviceContextXpLM, PR_LOG_DEBUG, ("nsDeviceContextXp::SetSpec()\n"));

  nsCOMPtr<nsIDeviceContextSpecXp> xpSpec;

  mSpec = aSpec;

  if(mPrintContext) delete mPrintContext; // we cannot reuse that...
    
  mPrintContext = new nsXPrintContext();
  xpSpec = do_QueryInterface(mSpec);
  if(xpSpec) {
     rv = mPrintContext->Init(xpSpec);
  }
 
  return rv;
}

NS_IMPL_ISUPPORTS_INHERITED(nsDeviceContextXp,
                            DeviceContextImpl,
                            nsIDeviceContextXp)

/** ---------------------------------------------------
 *  See documentation in nsDeviceContextXp.h
 * 
 * This method is called after SetSpec
 */
NS_IMETHODIMP
nsDeviceContextXp::InitDeviceContextXP(nsIDeviceContext *aCreatingDeviceContext,nsIDeviceContext *aParentContext)
{
  // Initialization moved to SetSpec to be done after creating the Print Context
  float origscale, newscale;
  float t2d, a2d;
  int   print_resolution;

  mPrintContext->GetPrintResolution(print_resolution);  
  mScreen  = mPrintContext->GetScreen();
  mDisplay = mPrintContext->GetDisplay();

  mPixelsToTwips = (float)NSIntPointsToTwips(72) / (float)print_resolution;
  mTwipsToPixels = 1.0f / mPixelsToTwips;

  GetTwipsToDevUnits(newscale);
  aParentContext->GetTwipsToDevUnits(origscale);

  mCPixelScale = newscale / origscale;

  aParentContext->GetTwipsToDevUnits(t2d);
  aParentContext->GetAppUnitsToDevUnits(a2d);

  mAppUnitsToDevUnits = (a2d / t2d) * mTwipsToPixels;
  mDevUnitsToAppUnits = 1.0f / mAppUnitsToDevUnits;

  // mPrintContext->GetAppUnitsToDevUnits(mAppUnitsToDevUnits);
  // mDevUnitsToAppUnits = 1.0f / mAppUnitsToDevUnits;

  mParentDeviceContext = aParentContext;
  NS_ASSERTION(mParentDeviceContext, "aCreatingDeviceContext cannot be NULL!!!");
  NS_ADDREF(mParentDeviceContext);
  
  return NS_OK;
}

/** ---------------------------------------------------
 */
NS_IMETHODIMP nsDeviceContextXp :: CreateRenderingContext(nsIRenderingContext *&aContext)
{
   nsresult  rv = NS_ERROR_OUT_OF_MEMORY;

   nsCOMPtr<nsRenderingContextXp> xpContext;

   xpContext = new nsRenderingContextXp();
   if (xpContext){
      rv = xpContext->Init(this);
   }

   if (NS_SUCCEEDED(rv)) {
     aContext = xpContext;
     NS_ADDREF(aContext);
   }
   return rv;
}

/** ---------------------------------------------------
 *  See documentation in nsIDeviceContext.h
 */
NS_IMETHODIMP nsDeviceContextXp :: SupportsNativeWidgets(PRBool &aSupportsWidgets)
{
  aSupportsWidgets = PR_FALSE;
  return NS_OK;
}

/** ---------------------------------------------------
 *  See documentation in nsIDeviceContext.h
 */
NS_IMETHODIMP nsDeviceContextXp :: GetScrollBarDimensions(float &aWidth, 
                                        float &aHeight) const
{
  // XXX Oh, yeah.  These are hard coded.
  aWidth  = 15 * mPixelsToTwips;
  aHeight = 15 * mPixelsToTwips;

  return NS_OK;
}

void  nsDeviceContextXp :: SetDrawingSurface(nsDrawingSurface  aSurface) 
{ 
}

/** ---------------------------------------------------
 *  See documentation in nsIDeviceContext.h
 */
NS_IMETHODIMP nsDeviceContextXp :: GetDrawingSurface(nsIRenderingContext &aContext, nsDrawingSurface &aSurface)
{
  aSurface = nsnull;
  return NS_OK;
}

/** ---------------------------------------------------
 *  See documentation in nsIDeviceContext.h
 */
NS_IMETHODIMP nsDeviceContextXp::GetILColorSpace(IL_ColorSpace*& aColorSpace)
{
#ifdef NOTNOW
  if (nsnull == mColorSpace) {
    mColorSpace = IL_CreateGreyScaleColorSpace(1, 1);

    if (nsnull == mColorSpace) {
      aColorSpace = nsnull;
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }

  // Return the color space
  aColorSpace = mColorSpace;
  IL_AddRefToColorSpace(aColorSpace);
#endif /* NOTNOW */

  if(!mColorSpace) {
      IL_RGBBits colorRGBBits;
    
      // Create a 24-bit color space
      colorRGBBits.red_shift   = 16;  
      colorRGBBits.red_bits    =  8;
      colorRGBBits.green_shift =  8;
      colorRGBBits.green_bits  =  8; 
      colorRGBBits.blue_shift  =  0; 
      colorRGBBits.blue_bits   =  8;  
    
      mColorSpace = IL_CreateTrueColorSpace(&colorRGBBits, 24);

    if (nsnull == mColorSpace) {
      aColorSpace = nsnull;
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }


  // Return the color space
  aColorSpace = mColorSpace;
  IL_AddRefToColorSpace(aColorSpace);

  return NS_OK;
}

/** ---------------------------------------------------
 *  See documentation in nsIDeviceContext.h
 *        @update 12/21/98 dwc
 */
NS_IMETHODIMP nsDeviceContextXp :: CheckFontExistence(const nsString& aFontName)
{
  PR_LOG(nsDeviceContextXpLM, PR_LOG_DEBUG, ("nsDeviceContextXp::CheckFontExistence()\n"));
  return nsFontMetricsXp::FamilyExists(aFontName);
}

NS_IMETHODIMP nsDeviceContextXp :: GetSystemAttribute(nsSystemAttrID anID, 
                                                      SystemAttrStruct * aInfo) const
{
  if (mParentDeviceContext != nsnull) {
    return mParentDeviceContext->GetSystemAttribute(anID, aInfo);
  }
  return NS_ERROR_FAILURE;
}

/** ---------------------------------------------------
 *  See documentation in nsIDeviceContext.h
 */
NS_IMETHODIMP nsDeviceContextXp::GetDeviceSurfaceDimensions(PRInt32 &aWidth, 
                                                        PRInt32 &aHeight)
{
  float width, height;
  width  = (float) mPrintContext->GetWidth();
  height = (float) mPrintContext->GetHeight();

  aWidth  = NSToIntRound(width  * mDevUnitsToAppUnits);
  aHeight = NSToIntRound(height * mDevUnitsToAppUnits);

  return NS_OK;
}

NS_IMETHODIMP nsDeviceContextXp::GetRect(nsRect &aRect)
{
  PRInt32 width, height;
  nsresult rv;
  rv = GetDeviceSurfaceDimensions(width, height);
  aRect.x = 0;
  aRect.y = 0;
  aRect.width = width;
  aRect.height = height;
  return rv;
}

/** ---------------------------------------------------
 *  See documentation in nsIDeviceContext.h
 */
NS_IMETHODIMP nsDeviceContextXp::GetClientRect(nsRect &aRect)
{
  return GetRect(aRect);
}

/** ---------------------------------------------------
 *  See documentation in nsIDeviceContext.h
 */
NS_IMETHODIMP nsDeviceContextXp::GetDeviceContextFor(nsIDeviceContextSpec *aDevice, nsIDeviceContext *&aContext)
{
  return NS_OK;
}

/** ---------------------------------------------------
 *  See documentation in nsIDeviceContext.h
 */
NS_IMETHODIMP nsDeviceContextXp::BeginDocument(PRUnichar * aTitle)
{  
  nsresult  rv = NS_OK;
  if (mPrintContext != nsnull) {
      rv = mPrintContext->BeginDocument(aTitle);
  } 
  return rv;
}

/** ---------------------------------------------------
 *  See documentation in nsIDeviceContext.h
 */
NS_IMETHODIMP nsDeviceContextXp::EndDocument(void)
{
  nsresult  rv = NS_OK;
  if (mPrintContext != nsnull) {
      rv = mPrintContext->EndDocument();
      
      // gisburn: mPrintContext cannot be reused between to print 
      // tasks as the destination print server may be a different one 
      // or the printer used on the same print server has other 
      // properties (build-in fonts for example ) than the printer 
      // previously used
      delete mPrintContext;
      mPrintContext = nsnull;
  } 
  
  return rv;
}

/** ---------------------------------------------------
 *  See documentation in nsIDeviceContext.h
 */
NS_IMETHODIMP nsDeviceContextXp::BeginPage(void)
{
  nsresult  rv = NS_OK;
  if (mPrintContext != nsnull) {
      rv = mPrintContext->BeginPage();
  }
  return rv;
}

/** ---------------------------------------------------
 *  See documentation in nsIDeviceContext.h
 *        @update 12/21/98 dwc
 */
NS_IMETHODIMP nsDeviceContextXp::EndPage(void)
{
  nsresult  rv = NS_OK;
  if (mPrintContext != nsnull) {
      rv = mPrintContext->EndPage();
  }
  return rv;
}

/** ---------------------------------------------------
 *  See documentation in nsIDeviceContext.h
 *        @update 12/21/98 dwc
 */
NS_IMETHODIMP nsDeviceContextXp :: ConvertPixel(nscolor aColor, 
                                                        PRUint32 & aPixel)
{
  PR_LOG(nsDeviceContextXpLM, PR_LOG_DEBUG, ("nsDeviceContextXp::ConvertPixel()\n"));
  aPixel = xlib_rgb_xpixel_from_rgb(NS_RGB(NS_GET_B(aColor),
                                           NS_GET_G(aColor),
                                           NS_GET_R(aColor)));
  return NS_OK;
}

Display *
nsDeviceContextXp::GetDisplay() 
{
   if (mPrintContext != nsnull) {
      return mDisplay; 
   } else {
      return (Display *)nsnull;
   }
}

NS_IMETHODIMP nsDeviceContextXp::GetDepth(PRUint32& aDepth)
{
   if (mPrintContext != nsnull) {
      aDepth = mPrintContext->GetDepth(); 
   } else {
      aDepth = 0; 
   }
   return NS_OK;
}

/** ---------------------------------------------------
 *  See documentation in nsIDeviceContext.h
 */
NS_IMETHODIMP nsDeviceContextXp::GetMetricsFor(const nsFont& aFont, 
                        nsIAtom* aLangGroup, nsIFontMetrics  *&aMetrics)
{
    return GetMetricsFor(aFont, aMetrics);
}

NS_IMETHODIMP nsDeviceContextXp::GetMetricsFor(const nsFont& aFont, 
                                        nsIFontMetrics  *&aMetrics)
{
  PRUint32         n,cnt;
  nsresult        rv;

  // First check our cache
  rv = mFontMetrics->Count(&n);
  if (NS_FAILED(rv))
    return rv;
  
  nsCOMPtr<nsIFontMetrics> m;

  for (cnt = 0; cnt < n; cnt++) {
    if (NS_SUCCEEDED(mFontMetrics->QueryElementAt(cnt,
                                                  NS_GET_IID(nsIFontMetrics),
                                                  getter_AddRefs(m)))) {
      const nsFont* font;
      m->GetFont(font);
      if (aFont.Equals(*font)) {
        aMetrics = m;
        NS_ADDREF(aMetrics);
        return NS_OK;
      }
    }
  }

  // It's not in the cache. Get font metrics and then cache them.
  nsCOMPtr<nsIFontMetrics> fm = new nsFontMetricsXp();
  if (!fm) {
    aMetrics = nsnull;
    return NS_ERROR_FAILURE;
  }

  // XXX need to pass real lang group
  rv = fm->Init(aFont, nsnull, this);

  if (NS_FAILED(rv)) {
    aMetrics = nsnull;
    return rv;
  }

  mFontMetrics->AppendElement(fm);

  aMetrics = fm;
  NS_ADDREF(aMetrics);
  return NS_OK;
}

NS_IMETHODIMP
nsDeviceContextXp::GetPrintContext(nsXPrintContext*& aContext) {
  aContext = mPrintContext;
  //NS_ADDREF(aContext);
  return NS_OK;
}

