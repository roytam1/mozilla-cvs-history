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

#include "nsDeviceContextPh.h"
#include "nsRenderingContextPh.h"
#include "nsDeviceContextSpecPh.h"
#include "il_util.h"
#include "nsPhGfxLog.h"

nsDeviceContextPh :: nsDeviceContextPh()
  : DeviceContextImpl()
{
  mSurface = NULL;
  mPaletteInfo.isPaletteDevice = PR_FALSE;
  mPaletteInfo.sizePalette = 0;
  mPaletteInfo.numReserved = 0;
  mPaletteInfo.palette = NULL;
//  mDC = NULL;
  mPixelScale = 1.0f;
  mWidthFloat = 0.0f;
  mHeightFloat = 0.0f;
  mWidth = -1;
  mHeight = -1;
  mSpec = nsnull;
}

nsDeviceContextPh :: ~nsDeviceContextPh()
{
}

NS_IMETHODIMP nsDeviceContextPh :: Init(nsNativeWidget aWidget)
{
  PhSysInfo_t SysInfo;
  PhRect_t                        rect;
  PtArg_t                         arg[15];
  char                            c, *p;
  int                             inp_grp = 0;
  PhRid_t                         rid;
  PhRegion_t                      region;
														
  PR_LOG(PhGfxLog, PR_LOG_DEBUG,("nsDeviceContextPh::Init with aWidget\n"));
  
  // this is a temporary hack!
  mPixelsToTwips = 15.0f;		// from debug under windows
  mTwipsToPixels = 1 / mPixelsToTwips;  

 /* Get the Screen Size */
 p = getenv("PHIG");
 if (p)
   inp_grp = atoi(p);
 else
   abort();

 PhQueryRids( 0, 0, inp_grp, Ph_INPUTGROUP_REGION, 0, 0, 0, &rid, 1 );
 PhRegionQuery( rid, &region, &rect, NULL, 0 );
 inp_grp = region.input_group;
 PhWindowQueryVisible( Ph_QUERY_INPUT_GROUP | Ph_QUERY_EXACT, 0, inp_grp, &rect );
 mWidthFloat  = (float) rect.lr.x - rect.ul.x+1;
 mHeightFloat = (float) rect.lr.y - rect.ul.y+1;  

 PR_LOG(PhGfxLog, PR_LOG_DEBUG,("nsDeviceContextPh::Init with aWidget: Screen Size (%f,%f)\n", mWidthFloat,mHeightFloat));


 /* Get the System Info for the RID */
 if (!PhQuerySystemInfo(rid, NULL, &SysInfo))
 {
    PR_LOG(PhGfxLog, PR_LOG_ERROR,("nsDeviceContextPh::Init with aWidget: Error getting SystemInfo\n"));
 }
 else
 {
   /* Make sure the "color_bits" field is valid */
   if (SysInfo.gfx.valid_fields & Ph_GFX_COLOR_BITS)
   {
     mDepth = SysInfo.gfx.color_bits;
     PR_LOG(PhGfxLog, PR_LOG_DEBUG,("nsDeviceContextPh::Init with aWidget: Pixel Depth = %d\n", mDepth));
   }
 }	 

  /* Palette Information: just a guess!  REVISIT */
  mPaletteInfo.isPaletteDevice = 1;
  mPaletteInfo.sizePalette = 255;
  mPaletteInfo.numReserved = 16;

  // Call my base class
  return DeviceContextImpl::Init(aWidget);
}

//local method...

nsresult nsDeviceContextPh :: Init(nsNativeDeviceContext aContext, nsIDeviceContext *aOrigContext)
{
  PR_LOG(PhGfxLog, PR_LOG_DEBUG,("nsDeviceContextPh::Init with nsNativeDeviceContext\n"));
  return NS_OK;
}

NS_IMETHODIMP nsDeviceContextPh :: CreateRenderingContext(nsIRenderingContext *&aContext)
{
  PR_LOG(PhGfxLog, PR_LOG_DEBUG,("nsDeviceContextPh::CreateRenderingContext - Not Implemented\n"));
  return NS_OK;
}

NS_IMETHODIMP nsDeviceContextPh :: SupportsNativeWidgets(PRBool &aSupportsWidgets)
{
  PR_LOG(PhGfxLog, PR_LOG_DEBUG,("nsDeviceContextPh::SupportsNativeWidgets\n"));
//  aSupportsWidgets = PR_FALSE;	/* we think this is correct */
    aSupportsWidgets = PR_TRUE;		/* but you have to return this ... look at nsView class CreateWidget method */
  return NS_OK;
}

NS_IMETHODIMP nsDeviceContextPh :: GetCanonicalPixelScale(float &aScale) const
{
  PR_LOG(PhGfxLog, PR_LOG_DEBUG,("nsDeviceContextPh::GetCanonicalPixelScale <%f>\n", mPixelScale));

  aScale = mPixelScale;		// mPixelScale should be 1.0f
  
  return NS_OK;
}

NS_IMETHODIMP nsDeviceContextPh :: GetScrollBarDimensions(float &aWidth, float &aHeight) const
{
  PR_LOG(PhGfxLog, PR_LOG_DEBUG,("nsDeviceContextPh::GetScrollBarDimensions\n"));

  /* Revisit: the scroll bar sizes is a gross guess based on Phab */
  aWidth = 17.0f *  mPixelsToTwips;
  aHeight = 17.0f *  mPixelsToTwips;
  
  return NS_OK;
}

nsresult GetSysFontInfo( PhGC_t &aGC, nsSystemAttrID anID, nsFont * aFont) 
{
  PR_LOG(PhGfxLog, PR_LOG_DEBUG,("nsDeviceContextPh::GetSysFontInfo - Not Implemented\n"));
  return NS_OK;
}

NS_IMETHODIMP nsDeviceContextPh :: GetSystemAttribute(nsSystemAttrID anID, SystemAttrStruct * aInfo) const
{
  PR_LOG(PhGfxLog, PR_LOG_DEBUG,("nsDeviceContextPh::GetSystemAttribute - Not Implemented\n"));
  return NS_OK;
}

NS_IMETHODIMP nsDeviceContextPh :: GetDrawingSurface(nsIRenderingContext &aContext, nsDrawingSurface &aSurface)
{
  PR_LOG(PhGfxLog, PR_LOG_DEBUG,("nsDeviceContextPh::GetDrawingSurface - Not Implemented\n"));
/*
  if (NULL == mSurface)
  {
     aContext.CreateDrawingSurface(nsnull, 0, mSurface);
  }
		
  aSurface = mSurface;
*/
  return NS_OK;
}

/* I need to know the requested font size to finish this function */
NS_IMETHODIMP nsDeviceContextPh :: CheckFontExistence(const nsString& aFontName)
{
  char *fontName = aFontName.ToNewCString();
  PR_LOG(PhGfxLog, PR_LOG_DEBUG,("nsDeviceContextPh::CheckFontExistence  for <%s>\n", fontName));

  nsresult    ret_code = NS_ERROR_FAILURE;
  int         MAX_FONTDETAIL = 30;
  FontDetails fDetails[MAX_FONTDETAIL];
  int         fontcount;
  
  fontcount = PfQueryFonts('a', PHFONT_ALL_FONTS, fDetails, MAX_FONTDETAIL);
  if (fontcount >= MAX_FONTDETAIL)
  {
	printf("nsDeviceContextPh::CheckFontExistence Font Array should be increased!\n");
  }

  if (fontcount)
  {
    int index;
	for(index=0; index < fontcount; index++)
	{
//      PR_LOG(PhGfxLog, PR_LOG_DEBUG,("nsDeviceContextPh::CheckFontExistence  comparing <%s> with <%s>\n", fontName, fDetails[index].desc));
	  if (strncmp(fontName, fDetails[index].desc, strlen(fontName)) == 0)
	  {
        PR_LOG(PhGfxLog, PR_LOG_DEBUG,("nsDeviceContextPh::CheckFontExistence  Found the font <%s>\n", fDetails[index].desc));
		ret_code = NS_OK;
		break;	  
	  }
	}
  }

  if (ret_code == NS_ERROR_FAILURE)
  {
    PR_LOG(PhGfxLog, PR_LOG_DEBUG,("nsDeviceContextPh::CheckFontExistence  Did not find the font <%s>\n", fontName));
  }
  
  delete [] fontName;

  /* Return ok and we will map it to some other font later */  
  return ret_code;
}

NS_IMETHODIMP nsDeviceContextPh::GetDepth(PRUint32& aDepth)
{
  PR_LOG(PhGfxLog, PR_LOG_DEBUG,("nsDeviceContextPh::GetDepth - Not Implemented\n"));
  return NS_OK;
}

NS_IMETHODIMP nsDeviceContextPh::GetILColorSpace(IL_ColorSpace*& aColorSpace)
{
  PR_LOG(PhGfxLog, PR_LOG_DEBUG,("nsDeviceContextPh::GetILColorSpace - Calling base class method\n"));

  /* This used to return NS_OK but I chose to call my base class instead */
  return DeviceContextImpl::GetILColorSpace(aColorSpace);
}

NS_IMETHODIMP nsDeviceContextPh::GetPaletteInfo(nsPaletteInfo& aPaletteInfo)
{
  PR_LOG(PhGfxLog, PR_LOG_DEBUG,("nsDeviceContextPh::GetPaletteInfo - Not Implemented\n"));
  return NS_OK;
}

NS_IMETHODIMP nsDeviceContextPh :: ConvertPixel(nscolor aColor, PRUint32 & aPixel)
{
  PR_LOG(PhGfxLog, PR_LOG_DEBUG,("nsDeviceContextPh::ConvertPixel - Not Implemented\n"));
  return NS_OK;
}

NS_IMETHODIMP nsDeviceContextPh :: GetDeviceSurfaceDimensions(PRInt32 &aWidth, PRInt32 &aHeight)
{
  PR_LOG(PhGfxLog, PR_LOG_DEBUG,("nsDeviceContextPh::GetDeviceSurfaceDimensions - Not Implemented\n"));
  return NS_OK;
}


NS_IMETHODIMP nsDeviceContextPh :: GetDeviceContextFor(nsIDeviceContextSpec *aDevice,
                                                        nsIDeviceContext *&aContext)
{
  PR_LOG(PhGfxLog, PR_LOG_DEBUG,("nsDeviceContextPh::GetDeviceContextFor - Not Implemented\n"));
  return NS_OK;
}

NS_IMETHODIMP nsDeviceContextPh :: BeginDocument(void)
{
  PR_LOG(PhGfxLog, PR_LOG_DEBUG,("nsDeviceContextPh::BeginDocument - Not Implemented\n"));
  return NS_OK;
}

NS_IMETHODIMP nsDeviceContextPh :: EndDocument(void)
{
  PR_LOG(PhGfxLog, PR_LOG_DEBUG,("nsDeviceContextPh::EndDocument - Not Implemented\n"));
  return NS_OK;
}

NS_IMETHODIMP nsDeviceContextPh :: BeginPage(void)
{
  PR_LOG(PhGfxLog, PR_LOG_DEBUG,("nsDeviceContextPh::BeginPage - Not Implemented\n"));
  return NS_OK;
}

NS_IMETHODIMP nsDeviceContextPh :: EndPage(void)
{
  PR_LOG(PhGfxLog, PR_LOG_DEBUG,("nsDeviceContextPh::EndPage - Not Implemented\n"));
  return NS_OK;
}
