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

#include <math.h>

#include "nspr.h"
#include "nsIPref.h"
#include "nsIServiceManager.h"
#include "il_util.h"

#include "nsDeviceContextGTK.h"
#include "nsGfxCIID.h"

#include "nsGfxPSCID.h"
#include "nsIDeviceContextPS.h"

#include <gdk/gdk.h>
#include <gdk/gdkx.h>

#define NS_TO_GDK_RGB(ns) (ns & 0xff) << 16 | (ns & 0xff00) | ((ns >> 16) & 0xff)

#define GDK_COLOR_TO_NS_RGB(c) \
  ((nscolor) NS_RGB(c.red, c.green, c.blue))

static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);
static NS_DEFINE_IID(kDeviceContextIID, NS_IDEVICE_CONTEXT_IID);

nsDeviceContextGTK::nsDeviceContextGTK()
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

nsDeviceContextGTK::~nsDeviceContextGTK()
{
}

NS_IMPL_QUERY_INTERFACE(nsDeviceContextGTK, kDeviceContextIID)
NS_IMPL_ADDREF(nsDeviceContextGTK)
NS_IMPL_RELEASE(nsDeviceContextGTK)

static NS_DEFINE_CID(kPrefCID, NS_PREF_CID);
static NS_DEFINE_IID(kIPrefIID, NS_IPREF_IID);

NS_IMETHODIMP nsDeviceContextGTK::Init(nsNativeWidget aNativeWidget)
{
  GdkVisual *vis;
  GtkRequisition req;
  GtkWidget *sb;

  mWidget = aNativeWidget;

  static nscoord dpi = 96;
  static int initialized = 0;
  if (!initialized) {
    initialized = 1;
    nsIPref* prefs = nsnull;
    nsresult res = nsServiceManager::GetService(kPrefCID, kIPrefIID,
      (nsISupports**) &prefs);
    if (NS_SUCCEEDED(res) && prefs) {
      PRInt32 intVal = 96;
      res = prefs->GetIntPref("browser.screen_resolution", &intVal);
      if (NS_SUCCEEDED(res)) {
        if (intVal) {
          dpi = intVal;
        }
        else {
          // Compute dpi of display
          float screenWidth = float(::gdk_screen_width());
          float screenWidthIn = float(::gdk_screen_width_mm()) / 25.4f;
          dpi = nscoord(screenWidth / screenWidthIn);
        }
      }
      nsServiceManager::ReleaseService(kPrefCID, prefs);
    }
  }

#if 0
  // Now for some wacky heuristics. 
  if (dpi < 84) dpi = 72;
  else if (dpi < 108) dpi = 96;
  else if (dpi < 132) dpi = 120;
#endif

  mTwipsToPixels = float(dpi) / float(NSIntPointsToTwips(72));
  mPixelsToTwips = 1.0f / mTwipsToPixels;

#if 0
  mTwipsToPixels = ( ((float)::gdk_screen_width()) /
                     ((float)::gdk_screen_width_mm()) * 25.4) /
		     (float)NSIntPointsToTwips(72);

  mPixelsToTwips = 1.0f / mTwipsToPixels;
#endif

  vis = gdk_rgb_get_visual();
  mDepth = vis->depth;

  sb = gtk_vscrollbar_new(NULL);
  gtk_widget_ref(sb);
  gtk_object_sink(GTK_OBJECT(sb));
  gtk_widget_size_request(sb,&req);
  mScrollbarWidth = req.width;
  gtk_widget_destroy(sb);
  gtk_widget_unref(sb);
  
  sb = gtk_hscrollbar_new(NULL);
  gtk_widget_ref(sb);
  gtk_object_sink(GTK_OBJECT(sb));
  gtk_widget_size_request(sb,&req);
  mScrollbarHeight = req.height;
  gtk_widget_destroy(sb);
  gtk_widget_unref(sb);

  mWidthFloat = (float) gdk_screen_width();
  mHeightFloat = (float) gdk_screen_height();

#ifdef DEBUG
  static PRBool once = PR_TRUE;
  if (once) {
    printf("GFX: dpi=%d t2p=%g p2t=%g depth=%d\n", dpi, mTwipsToPixels, mPixelsToTwips,mDepth);
    once = PR_FALSE;
  }
#endif

  DeviceContextImpl::CommonInit();

  return NS_OK;
}

NS_IMETHODIMP nsDeviceContextGTK::CreateRenderingContext(nsIRenderingContext *&aContext)
{
  nsIRenderingContext *pContext;
  nsresult             rv;
  nsDrawingSurfaceGTK  *surf;
  GtkWidget *w;

  w = (GtkWidget*)mWidget;

  // to call init for this, we need to have a valid nsDrawingSurfaceGTK created
  pContext = new nsRenderingContextGTK();

  if (nsnull != pContext)
  {
    NS_ADDREF(pContext);

    // create the nsDrawingSurfaceGTK
    surf = new nsDrawingSurfaceGTK();

    if (surf && w)
      {
        GdkDrawable *gwin = nsnull;
        GdkDrawable *win = nsnull;
        // FIXME
        if (GTK_IS_LAYOUT(w))
          gwin = (GdkDrawable*)GTK_LAYOUT(w)->bin_window;
        else
          gwin = (GdkDrawable*)(w)->window;

        // window might not be realized... ugh
        if (gwin)
          gdk_window_ref(gwin);
        else
          win = gdk_pixmap_new(nsnull,
                               w->allocation.width,
                               w->allocation.height,
                               gdk_rgb_get_visual()->depth);

        GdkGC *gc = gdk_gc_new(win);

        // init the nsDrawingSurfaceGTK
        rv = surf->Init(win,gc);

        if (NS_OK == rv)
          // Init the nsRenderingContextGTK
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

NS_IMETHODIMP nsDeviceContextGTK::SupportsNativeWidgets(PRBool &aSupportsWidgets)
{
  //XXX it is very critical that this not lie!! MMP
  // read the comments in the mac code for this
  aSupportsWidgets = PR_TRUE;

  return NS_OK;
}

NS_IMETHODIMP nsDeviceContextGTK::GetScrollBarDimensions(float &aWidth, float &aHeight) const
{
  aWidth = mScrollbarWidth * mPixelsToTwips;
  aHeight = mScrollbarHeight * mPixelsToTwips;
 
  return NS_OK;
}

NS_IMETHODIMP nsDeviceContextGTK::GetSystemAttribute(nsSystemAttrID anID, SystemAttrStruct * aInfo) const
{
  nsresult status = NS_OK;
  GtkStyle *style = gtk_style_new();  // get the default styles

  switch (anID) {
    //---------
    // Colors
    //---------
    case eSystemAttr_Color_WindowBackground:
        *aInfo->mColor = GDK_COLOR_TO_NS_RGB(style->bg[GTK_STATE_NORMAL]);
        break;
    case eSystemAttr_Color_WindowForeground:
        *aInfo->mColor = GDK_COLOR_TO_NS_RGB(style->fg[GTK_STATE_NORMAL]);
        break;
    case eSystemAttr_Color_WidgetBackground:
        *aInfo->mColor = GDK_COLOR_TO_NS_RGB(style->bg[GTK_STATE_NORMAL]);
        break;
    case eSystemAttr_Color_WidgetForeground:
        *aInfo->mColor = GDK_COLOR_TO_NS_RGB(style->fg[GTK_STATE_NORMAL]);
        break;
    case eSystemAttr_Color_WidgetSelectBackground:
        *aInfo->mColor = GDK_COLOR_TO_NS_RGB(style->bg[GTK_STATE_SELECTED]);
        break;
    case eSystemAttr_Color_WidgetSelectForeground:
        *aInfo->mColor = GDK_COLOR_TO_NS_RGB(style->fg[GTK_STATE_SELECTED]);
        break;
    case eSystemAttr_Color_Widget3DHighlight:
        *aInfo->mColor = NS_RGB(0xa0,0xa0,0xa0);
        break;
    case eSystemAttr_Color_Widget3DShadow:
        *aInfo->mColor = NS_RGB(0x40,0x40,0x40);
        break;
    case eSystemAttr_Color_TextBackground:
        *aInfo->mColor = GDK_COLOR_TO_NS_RGB(style->bg[GTK_STATE_NORMAL]);
        break;
    case eSystemAttr_Color_TextForeground: 
        *aInfo->mColor = GDK_COLOR_TO_NS_RGB(style->fg[GTK_STATE_NORMAL]);
        break;
    case eSystemAttr_Color_TextSelectBackground:
        *aInfo->mColor = GDK_COLOR_TO_NS_RGB(style->bg[GTK_STATE_SELECTED]);
        break;
    case eSystemAttr_Color_TextSelectForeground:
        *aInfo->mColor = GDK_COLOR_TO_NS_RGB(style->text[GTK_STATE_SELECTED]);
        break;
    //---------
    // Size
    //---------
    case eSystemAttr_Size_ScrollbarHeight:
        aInfo->mSize = mScrollbarHeight;
        break;
    case eSystemAttr_Size_ScrollbarWidth: 
        aInfo->mSize = mScrollbarWidth;
        break;
    case eSystemAttr_Size_WindowTitleHeight:
        aInfo->mSize = 0;
        break;
    case eSystemAttr_Size_WindowBorderWidth:
        aInfo->mSize = style->klass->xthickness;
        break;
    case eSystemAttr_Size_WindowBorderHeight:
        aInfo->mSize = style->klass->ythickness;
        break;
    case eSystemAttr_Size_Widget3DBorder:
        aInfo->mSize = 4;
        break;
    //---------
    // Fonts
    //---------
    case eSystemAttr_Font_Caption:
    case eSystemAttr_Font_Icon:
    case eSystemAttr_Font_Menu:
    case eSystemAttr_Font_MessageBox:
    case eSystemAttr_Font_SmallCaption:
    case eSystemAttr_Font_StatusBar:
    case eSystemAttr_Font_Tooltips:
    case eSystemAttr_Font_Widget:
      status = NS_ERROR_FAILURE;
      break;
  } // switch

  gtk_style_unref(style);

  return NS_OK;
}

NS_IMETHODIMP nsDeviceContextGTK::GetDrawingSurface(nsIRenderingContext &aContext, 
                                                    nsDrawingSurface &aSurface)
{
  aContext.CreateDrawingSurface(nsnull, 0, aSurface);
  return nsnull == aSurface ? NS_ERROR_OUT_OF_MEMORY : NS_OK;  
}

NS_IMETHODIMP nsDeviceContextGTK::ConvertPixel(nscolor aColor, 
                                               PRUint32 & aPixel)
{
  aPixel = ::gdk_rgb_xpixel_from_rgb (NS_TO_GDK_RGB(aColor));

  return NS_OK;
}


NS_IMETHODIMP nsDeviceContextGTK::CheckFontExistence(const nsString& aFontName)
{
  char        **fnames = nsnull;
  PRInt32     namelen = aFontName.Length() + 1;
  char        *wildstring = (char *)PR_Malloc(namelen + 200);
  float       t2d;
  GetTwipsToDevUnits(t2d);
  PRInt32     dpi = NSToIntRound(t2d * 1440);
  int         numnames = 0;
  XFontStruct *fonts;
  nsresult    rv = NS_ERROR_FAILURE;
  
  if (nsnull == wildstring)
    return NS_ERROR_UNEXPECTED;
  
  if (abs(dpi - 75) < abs(dpi - 100))
    dpi = 75;
  else
    dpi = 100;
  
  char* fontName = aFontName.ToNewCString();
  PR_snprintf(wildstring, namelen + 200,
             "-*-%s-*-*-normal-*-*-*-%d-%d-*-*-*-*",
             fontName, dpi, dpi);
  delete [] fontName;
  
  fnames = ::XListFontsWithInfo(GDK_DISPLAY(), wildstring, 1, &numnames, &fonts);
  
  if (numnames > 0)
  {
    ::XFreeFontInfo(fnames, fonts, numnames);
    rv = NS_OK;
  }
  
  PR_Free(wildstring);
  
  return rv;
}

NS_IMETHODIMP nsDeviceContextGTK::GetDeviceSurfaceDimensions(PRInt32 &aWidth, PRInt32 &aHeight)
{
  if (mWidth == -1)
    mWidth = NSToIntRound(mWidthFloat * mDevUnitsToAppUnits);

  if (mHeight == -1)
    mHeight = NSToIntRound(mHeightFloat * mDevUnitsToAppUnits);

  aWidth = mWidth;
  aHeight = mHeight;

  return NS_OK;
}

NS_IMETHODIMP nsDeviceContextGTK::GetDeviceContextFor(nsIDeviceContextSpec *aDevice,
                                                      nsIDeviceContext *&aContext)
{
  static NS_DEFINE_CID(kCDeviceContextPS, NS_DEVICECONTEXTPS_CID);
  
  // Create a Postscript device context 
  nsresult rv;
  nsIDeviceContextPS *dcps;
  
  rv = nsComponentManager::CreateInstance(kCDeviceContextPS,
                                          nsnull,
                                          nsIDeviceContextPS::GetIID(),
                                          (void **)&dcps);

  NS_ASSERTION(NS_SUCCEEDED(rv), "Couldn't create PS Device context");
  
  dcps->SetSpec(aDevice);
  dcps->InitDeviceContextPS((nsIDeviceContext*)aContext,
                            (nsIDeviceContext*)this);

  rv = dcps->QueryInterface(nsIDeviceContext::GetIID(),
                            (void **)&aContext);

  NS_RELEASE(dcps);
  
  return rv;
}

NS_IMETHODIMP nsDeviceContextGTK::BeginDocument(void)
{
  return NS_OK;
}

NS_IMETHODIMP nsDeviceContextGTK::EndDocument(void)
{
  return NS_OK;
}

NS_IMETHODIMP nsDeviceContextGTK::BeginPage(void)
{
  return NS_OK;
}

NS_IMETHODIMP nsDeviceContextGTK::EndPage(void)
{
  return NS_OK;
}

NS_IMETHODIMP nsDeviceContextGTK::GetDepth(PRUint32& aDepth)
{
  GdkVisual * rgb_visual = gdk_rgb_get_visual();

  gint rgb_depth = rgb_visual->depth;

  aDepth = (PRUint32) rgb_depth;

  return NS_OK;
}
