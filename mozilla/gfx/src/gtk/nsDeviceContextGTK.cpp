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
 */

#include <math.h>

#include "nspr.h"
#include "nsIPref.h"
#include "nsIServiceManager.h"
#include "il_util.h"
#include "nsCRT.h"

#include "nsDeviceContextGTK.h"
#include "nsGfxCIID.h"

#include "nsGfxPSCID.h"
#include "nsIDeviceContextPS.h"

#include <gdk/gdk.h>
#include <gdk/gdkx.h>

#define NS_TO_GDK_RGB(ns) (ns & 0xff) << 16 | (ns & 0xff00) | ((ns >> 16) & 0xff)

#define GDK_COLOR_TO_NS_RGB(c) \
  ((nscolor) NS_RGB(c.red, c.green, c.blue))

static NS_DEFINE_CID(kPrefCID, NS_PREF_CID);


nscoord nsDeviceContextGTK::mDpi = 96;

NS_IMPL_ISUPPORTS1(nsDeviceContextGTK, nsIDeviceContext)

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
  nsresult rv;
  nsCOMPtr<nsIPref> prefs = do_GetService(kPrefCID, &rv);
  if (NS_SUCCEEDED(rv)) {
    prefs->UnregisterCallback("browser.screen_resolution",
                              prefChanged, (void *)this);
  }
}


NS_IMETHODIMP nsDeviceContextGTK::Init(nsNativeWidget aNativeWidget)
{
  GdkVisual *vis;
  GtkRequisition req;
  GtkWidget *sb;

  mWidget = aNativeWidget;

  static int initialized = 0;
  if (!initialized) {
    initialized = 1;

    // Set prefVal the value of the preference "browser.screen_resolution"
    // or -1 if we can't get it.
    // If it's negative, we pretend it's not set.
    // If it's 0, it means force use of the operating system's logical resolution.
    // If it's positive, we use it as the logical resolution
    PRInt32 prefVal = -1;
    nsresult res;

    NS_WITH_SERVICE(nsIPref, prefs, kPrefCID, &res);
    if (NS_SUCCEEDED(res) && prefs) {
      res = prefs->GetIntPref("browser.screen_resolution", &prefVal);
      if (! NS_SUCCEEDED(res)) {
        prefVal = -1;
      }
      prefs->RegisterCallback("browser.screen_resolution", prefChanged,
                              (void *)this);
    }

    // Set OSVal to what the operating system thinks the logical resolution is.
    float screenWidth = float(::gdk_screen_width());
    float screenWidthIn = float(::gdk_screen_width_mm()) / 25.4f;
    PRInt32 OSVal = nscoord(screenWidth / screenWidthIn);

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
    printf("GFX: dpi=%d t2p=%g p2t=%g depth=%d\n", mDpi, mTwipsToPixels, mPixelsToTwips,mDepth);
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
  nsCRT::free(fontName);
 
  fnames = ::XListFonts(GDK_DISPLAY(), wildstring, 1, &numnames);
 
  if (numnames > 0)
  {
    ::XFreeFontNames(fnames);
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

NS_IMETHODIMP nsDeviceContextGTK::GetClientRect(nsRect &aRect)
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

NS_IMETHODIMP nsDeviceContextGTK::GetDeviceContextFor(nsIDeviceContextSpec *aDevice,
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

nsresult
nsDeviceContextGTK::SetDPI(PRInt32 aDpi)
{
  mDpi = aDpi;
  
  int pt2t = 72;

  // make p2t a nice round number - this prevents rounding problems
  mPixelsToTwips = float(NSToIntRound(float(NSIntPointsToTwips(pt2t)) / float(aDpi)));
  mTwipsToPixels = 1.0f / mPixelsToTwips;

  // XXX need to reflow all documents
  return NS_OK;
}

int nsDeviceContextGTK::prefChanged(const char *aPref, void *aClosure)
{
  nsDeviceContextGTK *context = (nsDeviceContextGTK*)aClosure;
  nsresult rv;
  
  if (nsCRT::strcmp(aPref, "browser.screen_resolution")==0) {
    PRInt32 dpi;
    NS_WITH_SERVICE(nsIPref, prefs, kPrefCID, &rv);
    rv = prefs->GetIntPref(aPref, &dpi);
    if (NS_SUCCEEDED(rv))
      context->SetDPI(dpi);
  }
  
  return 0;
}
