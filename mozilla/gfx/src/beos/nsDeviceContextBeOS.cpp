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
 *   Pierre Phaneuf <pp@ludusdesign.com>
 */

#include <math.h>

#include "nspr.h"
#include "nsIPref.h"
#include "nsIServiceManager.h"
#include "il_util.h"
#include "nsCRT.h"

#include "nsDeviceContextBeOS.h"
#include "nsFontMetricsBeOS.h"
#include "nsGfxCIID.h"

#include "nsGfxPSCID.h"
#include "nsIDeviceContextPS.h"

#include <ScrollBar.h>
#include <Screen.h>

#include "nsIScreenManager.h"

static NS_DEFINE_CID(kPrefCID, NS_PREF_CID); 
 
nscoord nsDeviceContextBeOS::mDpi = 96; 
 
NS_IMPL_ISUPPORTS1(nsDeviceContextBeOS, nsIDeviceContext) 
 
nsDeviceContextBeOS::nsDeviceContextBeOS()
{
  NS_INIT_REFCNT();
  mTwipsToPixels = 1.0;
  mPixelsToTwips = 1.0;
  mDepth = 0 ;
  mPaletteInfo.isPaletteDevice = PR_FALSE;
  mPaletteInfo.sizePalette = 0;
  mPaletteInfo.numReserved = 0;
  mPaletteInfo.palette = NULL;
  mNumCells = 0;
  
  mWidthFloat = 0.0f; 
  mHeightFloat = 0.0f; 
  mWidth = -1; 
  mHeight = -1; 
}

nsDeviceContextBeOS::~nsDeviceContextBeOS()
{
  nsresult rv; 
  nsCOMPtr<nsIPref> prefs = do_GetService(kPrefCID, &rv); 
  if (NS_SUCCEEDED(rv)) { 
    prefs->UnregisterCallback("browser.display.screen_resolution", 
                              prefChanged, (void *)this); 
  } 
}

NS_IMETHODIMP nsDeviceContextBeOS::Init(nsNativeWidget aNativeWidget)
{
  // get the screen object and its width/height 
  // XXXRight now this will only get the primary monitor. 

  nsresult ignore; 
  nsCOMPtr<nsIScreenManager> sm ( do_GetService("@mozilla.org/gfx/screenmanager;1", &ignore) ); 
  if ( sm ) { 
    nsCOMPtr<nsIScreen> screen; 
    sm->GetPrimaryScreen ( getter_AddRefs(screen) ); 
    if ( screen ) { 
      PRInt32 x, y, width, height, depth; 
      screen->GetAvailRect ( &x, &y, &width, &height ); 
      screen->GetPixelDepth ( &depth ); 
      mWidthFloat = float(width); 
      mHeightFloat = float(height); 
      mDepth = NS_STATIC_CAST ( PRUint32, depth ); 
    } 
  } 
  
  static int initialized = 0; 
  if (!initialized) { 
    initialized = 1; 
 
    // Set prefVal the value of the preference "browser.display.screen_resolution" 
    // or -1 if we can't get it. 
    // If it's negative, we pretend it's not set. 
    // If it's 0, it means force use of the operating system's logical resolution. 
    // If it's positive, we use it as the logical resolution 
    PRInt32 prefVal = -1; 
    nsresult res; 
 
    NS_WITH_SERVICE(nsIPref, prefs, kPrefCID, &res); 
    if (NS_SUCCEEDED(res) && prefs) { 
      res = prefs->GetIntPref("browser.display.screen_resolution", &prefVal); 
      if (! NS_SUCCEEDED(res)) { 
        prefVal = -1; 
      } 
      prefs->RegisterCallback("browser.display.screen_resolution", prefChanged, 
                              (void *)this); 
    } 
 
    // Set OSVal to what the operating system thinks the logical resolution is. 
    PRInt32 OSVal = 72; 
    
    if (prefVal > 0) { 
      // If there's a valid pref value for the logical resolution, 
      // use it. 
      mDpi = prefVal; 
    } else if ((prefVal == 0) || (OSVal > 96)) { 
      // Either if the pref is 0 (force use of OS value) or the OS 
      // value is bigger than 96, use the OS value. 
      mDpi = OSVal; 
    } else { 
      // if we couldn't get the pref or it's negative, and the OS 
      // value is under 96ppi, then use 96. 
      mDpi = 96; 
    } 
  } 
 
  SetDPI(mDpi); 
  
  mScrollbarHeight = PRInt16(B_H_SCROLL_BAR_HEIGHT); 
  mScrollbarWidth = PRInt16(B_V_SCROLL_BAR_WIDTH); 
  
#ifdef DEBUG 
  static PRBool once = PR_TRUE; 
  if (once) { 
    printf("GFX: dpi=%d t2p=%g p2t=%g depth=%d\n", mDpi, mTwipsToPixels, mPixelsToTwips,mDepth); 
    once = PR_FALSE; 
  } 
#endif 

  DeviceContextImpl::CommonInit();
  
  return NS_OK;
}

NS_IMETHODIMP nsDeviceContextBeOS::CreateRenderingContext(nsIRenderingContext *&aContext) 
{ 
  nsIRenderingContext *pContext; 
  nsresult             rv; 
  nsDrawingSurfaceBeOS  *surf; 
  BView *w; 

  w = (BView*)mWidget;

  // to call init for this, we need to have a valid nsDrawingSurfaceBeOS created 
  pContext = new nsRenderingContextBeOS(); 
 
  if (nsnull != pContext) 
  { 
    NS_ADDREF(pContext); 
 
    // create the nsDrawingSurfaceBeOS 
    surf = new nsDrawingSurfaceBeOS(); 
 
    if (surf && w) 
      { 
 
        // init the nsDrawingSurfaceBeOS 
        rv = surf->Init(w); 
 
        if (NS_OK == rv) 
          // Init the nsRenderingContextBeOS 
          rv = pContext->Init(this, surf); 
      } 
    else 
      rv = NS_ERROR_OUT_OF_MEMORY; 
  }
  else 
    rv = NS_ERROR_OUT_OF_MEMORY;

  if (NS_OK != rv)
  {
    NS_IF_RELEASE(pContext); 
  }

  aContext = pContext; 
 
  return rv; 
} 
 
NS_IMETHODIMP nsDeviceContextBeOS::SupportsNativeWidgets(PRBool &aSupportsWidgets)
{
  //XXX it is very critical that this not lie!! MMP
  // read the comments in the mac code for this
  aSupportsWidgets = PR_TRUE;

  return NS_OK;
}

NS_IMETHODIMP nsDeviceContextBeOS::GetScrollBarDimensions(float &aWidth, float &aHeight) const
{
  aWidth = mScrollbarWidth * mPixelsToTwips; 
  aHeight = mScrollbarHeight * mPixelsToTwips; 

  return NS_OK;
}

NS_IMETHODIMP nsDeviceContextBeOS::GetSystemAttribute(nsSystemAttrID anID, SystemAttrStruct * aInfo) const
{
  nsresult status = NS_OK;

  switch (anID) {
    //---------
    // Colors
    //---------
    case eSystemAttr_Color_WindowBackground:
        *aInfo->mColor = NS_RGB(0xdd,0xdd,0xdd);
        break;
    case eSystemAttr_Color_WindowForeground:
        *aInfo->mColor = NS_RGB(0x00,0x00,0x00);        
        break;
    case eSystemAttr_Color_WidgetBackground:
        *aInfo->mColor = NS_RGB(0xdd,0xdd,0xdd);
        break;
    case eSystemAttr_Color_WidgetForeground:
        *aInfo->mColor = NS_RGB(0x00,0x00,0x00);        
        break;
    case eSystemAttr_Color_WidgetSelectBackground:
        *aInfo->mColor = NS_RGB(0x80,0x80,0x80);
        break;
    case eSystemAttr_Color_WidgetSelectForeground:
        *aInfo->mColor = NS_RGB(0x00,0x00,0x80);
        break;
    case eSystemAttr_Color_Widget3DHighlight:
        *aInfo->mColor = NS_RGB(0xa0,0xa0,0xa0);
        break;
    case eSystemAttr_Color_Widget3DShadow:
        *aInfo->mColor = NS_RGB(0x40,0x40,0x40);
        break;
    case eSystemAttr_Color_TextBackground:
        *aInfo->mColor = NS_RGB(0xff,0xff,0xff);
        break;
    case eSystemAttr_Color_TextForeground: 
        *aInfo->mColor = NS_RGB(0x00,0x00,0x00);
        break;
    case eSystemAttr_Color_TextSelectBackground:
        *aInfo->mColor = NS_RGB(0x00,0x00,0x80);
        break;
    case eSystemAttr_Color_TextSelectForeground:
        *aInfo->mColor = NS_RGB(0xff,0xff,0xff);
        break;
    //---------
    // Size
    //---------
    case eSystemAttr_Size_ScrollbarHeight:
        aInfo->mSize = mScrollbarHeight;
        break;
    case eSystemAttr_Size_ScrollbarWidth : 
        aInfo->mSize = mScrollbarWidth;
        break;
    case eSystemAttr_Size_WindowTitleHeight:
        aInfo->mSize = 0;
        break;
    case eSystemAttr_Size_WindowBorderWidth:
        aInfo->mSize = 5;      // need to be checked!
        break;
    case eSystemAttr_Size_WindowBorderHeight:
        aInfo->mSize = 5;      // need to be checked! 
        break;
    case eSystemAttr_Size_Widget3DBorder:
        aInfo->mSize = 4;
        break;
    //---------
    // Fonts
    //---------
    case eSystemAttr_Font_Caption:             // css2
    case eSystemAttr_Font_Icon : 
    case eSystemAttr_Font_Menu : 
    case eSystemAttr_Font_MessageBox : 
    case eSystemAttr_Font_SmallCaption : 
    case eSystemAttr_Font_StatusBar : 
    case eSystemAttr_Font_Window:                      // css3 
    case eSystemAttr_Font_Document: 
    case eSystemAttr_Font_Workspace: 
    case eSystemAttr_Font_Desktop: 
    case eSystemAttr_Font_Info: 
    case eSystemAttr_Font_Dialog: 
    case eSystemAttr_Font_Button: 
    case eSystemAttr_Font_PullDownMenu: 
    case eSystemAttr_Font_List: 
    case eSystemAttr_Font_Field: 
    case eSystemAttr_Font_Tooltips:            // moz 
    case eSystemAttr_Font_Widget: 
      status = GetSystemFontInfo(be_plain_font, anID, aInfo->mFont);  
      break;
  } // switch 

  return status;
}

NS_IMETHODIMP nsDeviceContextBeOS::GetDrawingSurface(nsIRenderingContext &aContext, 
                                                    nsDrawingSurface &aSurface)
{
  aContext.CreateDrawingSurface(nsnull, 0, aSurface);
  return nsnull == aSurface ? NS_ERROR_OUT_OF_MEMORY : NS_OK;  
}

NS_IMETHODIMP nsDeviceContextBeOS::ConvertPixel(nscolor aColor, 
                                               PRUint32 & aPixel)
{
       // koehler@mythrium.com: 
       // I think this fill the 32 bits pixel with the desired color 
       // It's possible that the nscolor is not representing that color 
       // if you just dump it inside a 32 bits value. 
  aPixel = aColor;

  return NS_OK;
}


NS_IMETHODIMP nsDeviceContextBeOS::CheckFontExistence(const nsString& aFontName)
{
  return nsFontMetricsBeOS::FamilyExists(aFontName); 
} 

/* 
NS_IMETHODIMP nsDeviceContextBeOS::CheckFontExistence(const nsString& aFontName) 
{
  PRBool  isthere = PR_FALSE;

  char* cStr = aFontName.ToNewCString();

	int32 numFamilies = count_font_families();
	for(int32 i = 0; i < numFamilies; i++)
	{
		font_family family; 
		uint32 flags; 
		if(get_font_family(i, &family, &flags) == B_OK)
		{
			if(strcmp(family, cStr) == 0)
			{
				isthere = PR_TRUE;
				break;
			}
		} 
	}

	//printf("%s there? %s\n", cStr, isthere?"Yes":"No" );
	
  delete[] cStr;

  if (PR_TRUE == isthere)
    return NS_OK;
  else
    return NS_ERROR_FAILURE;
}
*/

NS_IMETHODIMP nsDeviceContextBeOS::GetDeviceSurfaceDimensions(PRInt32 &aWidth, PRInt32 &aHeight)
{
  if (mWidth == -1) 
    mWidth = NSToIntRound(mWidthFloat * mDevUnitsToAppUnits);

  if (mHeight == -1) 
    mHeight = NSToIntRound(mHeightFloat * mDevUnitsToAppUnits); 
 
  aWidth = mWidth; 
  aHeight = mHeight; 
 
  return NS_OK; 
}

NS_IMETHODIMP nsDeviceContextBeOS::GetRect(nsRect &aRect)
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
 
NS_IMETHODIMP nsDeviceContextBeOS::GetClientRect(nsRect &aRect) 
{ 
//XXX do we know if the client rect should ever differ from the screen rect? 
  return GetRect ( aRect );
}

NS_IMETHODIMP nsDeviceContextBeOS::GetDeviceContextFor(nsIDeviceContextSpec *aDevice,
                                                      nsIDeviceContext *&aContext)
{
  static NS_DEFINE_CID(kCDeviceContextPS, NS_DEVICECONTEXTPS_CID);
  
  // Create a Postscript device context 
  nsresult rv;
  nsIDeviceContextPS *dcps;
  
  rv = nsComponentManager::CreateInstance(kCDeviceContextPS,
                                          nsnull,
                                          NS_GET_IID(nsIDeviceContextPS),
                                          (void **)&dcps);

  NS_ASSERTION(NS_SUCCEEDED(rv), "Couldn't create PS Device context");
  
  dcps->SetSpec(aDevice);
  dcps->InitDeviceContextPS((nsIDeviceContext*)aContext,
                            (nsIDeviceContext*)this);

  rv = dcps->QueryInterface(NS_GET_IID(nsIDeviceContext),
                            (void **)&aContext);

  NS_RELEASE(dcps);
  
  return rv;
}

NS_IMETHODIMP nsDeviceContextBeOS::BeginDocument(PRUnichar * aTitle)
{
  return NS_OK;
}

NS_IMETHODIMP nsDeviceContextBeOS::EndDocument(void)
{
  return NS_OK;
}

NS_IMETHODIMP nsDeviceContextBeOS::BeginPage(void)
{
  return NS_OK;
}

NS_IMETHODIMP nsDeviceContextBeOS::EndPage(void)
{
  return NS_OK;
} 
 
NS_IMETHODIMP nsDeviceContextBeOS::GetDepth(PRUint32& aDepth) 
{ 
  aDepth = mDepth; 
  return NS_OK; 
} 
 
nsresult 
nsDeviceContextBeOS::SetDPI(PRInt32 aDpi) 
{ 
  mDpi = aDpi; 
  
  int pt2t = 72; 
 
  // make p2t a nice round number - this prevents rounding problems 
  mPixelsToTwips = float(NSToIntRound(float(NSIntPointsToTwips(pt2t)) / float(aDpi))); 
  mTwipsToPixels = 1.0f / mPixelsToTwips; 
 
  // XXX need to reflow all documents 
  return NS_OK; 
} 
 
int nsDeviceContextBeOS::prefChanged(const char *aPref, void *aClosure) 
{ 
  nsDeviceContextBeOS *context = (nsDeviceContextBeOS*)aClosure; 
  nsresult rv; 
  
  if (nsCRT::strcmp(aPref, "browser.display.screen_resolution")==0) { 
    PRInt32 dpi; 
    NS_WITH_SERVICE(nsIPref, prefs, kPrefCID, &rv); 
    rv = prefs->GetIntPref(aPref, &dpi); 
    if (NS_SUCCEEDED(rv)) 
      context->SetDPI(dpi); 
  } 
  
  return 0; 
} 
 
nsresult 
nsDeviceContextBeOS::GetSystemFontInfo(const BFont *theFont, nsSystemAttrID anID, nsFont* aFont) const 
{ 
  nsresult status = NS_OK; 
 
  aFont->style       = NS_FONT_STYLE_NORMAL; 
  aFont->weight      = NS_FONT_WEIGHT_NORMAL; 
  aFont->decorations = NS_FONT_DECORATION_NONE; 
  
  // do we have the default_font defined by BeOS, if not then 
  // we error out. 
  if( !theFont ) 
    theFont = be_plain_font; // BeOS default font 
   
  if( !theFont ) 
  { 
    status = NS_ERROR_FAILURE; 
  } 
  else 
  { 
    font_family family; 
    font_style style; 
       font_height height; 
 
    theFont->GetFamilyAndStyle(&family, &style);   
    aFont->name.AssignWithConversion( family ); 
 
       // No weight 
           
       theFont->GetHeight(&height); 
    aFont->size = NSIntPixelsToTwips(uint32(height.ascent+height.descent+height.leading), mPixelsToTwips); 
 
       // no style 
    
       // no decoration 
           
    status = NS_OK; 
  } 
  return (status); 
}
