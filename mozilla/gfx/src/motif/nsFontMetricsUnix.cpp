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
#include "nsFontMetricsUnix.h"
#include "nsDeviceContextUnix.h"

#include "nspr.h"
#include "nsCRT.h"

static NS_DEFINE_IID(kIFontMetricsIID, NS_IFONT_METRICS_IID);

//#define NOISY_FONTS

nsFontMetricsUnix :: nsFontMetricsUnix()
{
  NS_INIT_REFCNT();
  mFont = nsnull;
  mFontHandle = nsnull;
  mFontInfo = nsnull;
  mXstring = nsnull;
  mXstringSize = 0;
}
  
nsFontMetricsUnix :: ~nsFontMetricsUnix()
{
  if (nsnull != mFont) {    
    delete mFont;
    mFont = nsnull;
  }
  
  if (nsnull != mXstring)
    PR_Free(mXstring);
  
  if (nsnull != mFontHandle)
    ::XUnloadFont(XtDisplay((Widget)mContext->GetNativeWidget()), mFontHandle);  
 
}

NS_IMPL_ISUPPORTS(nsFontMetricsUnix, kIFontMetricsIID)

nsresult nsFontMetricsUnix :: Init(const nsFont& aFont, nsIDeviceContext* aCX)
{
  NS_ASSERTION(!(nsnull == aCX), "attempt to init fontmetrics with null device context");

  nsAutoString  firstFace;
  if (NS_OK != aCX->FirstExistingFont(aFont, firstFace)) {
    aFont.GetFirstFamily(firstFace);
  }

  char        **fnames = nsnull;
  PRInt32     namelen = firstFace.Length() + 1;
  char	      *wildstring = (char *)PR_Malloc((namelen << 1) + 200);
  int         numnames = 0;
  char        altitalicization = 0;
  XFontStruct *fonts;
  PRInt32     dpi = NSToIntRound(aCX->GetTwipsToDevUnits() * 1440);
  Display     *dpy = XtDisplay((Widget)aCX->GetNativeWidget());

  if (nsnull == wildstring)
    return NS_ERROR_NOT_INITIALIZED;

  mFont = new nsFont(aFont);
  mContext = aCX;
  mFontHandle = nsnull;

  firstFace.ToCString(wildstring, namelen);

  if (abs(dpi - 75) < abs(dpi - 100))
    dpi = 75;
  else
    dpi = 100;

#ifdef NOISY_FONTS
  fprintf(stderr, "looking for font %s (%d)", wildstring, aFont.size / 20);
#endif

  //font properties we care about:
  //name
  //weight (bold, medium)
  //slant (r = normal, i = italic, o = oblique)
  //size in nscoords >> 1

  PR_snprintf(&wildstring[namelen + 1], namelen + 200,
             "*-%s-%s-%c-normal--*-*-%d-%d-*-*-*",
             wildstring,
             (aFont.weight <= NS_FONT_WEIGHT_NORMAL) ? "medium" : "bold",
             (aFont.style == NS_FONT_STYLE_NORMAL) ? 'r' :
             ((aFont.style == NS_FONT_STYLE_ITALIC) ? 'i' : 'o'), dpi, dpi);

  fnames = ::XListFontsWithInfo(dpy, &wildstring[namelen + 1], 200, &numnames, &fonts);

  if (aFont.style == NS_FONT_STYLE_ITALIC)
    altitalicization = 'o';
  else if (aFont.style == NS_FONT_STYLE_OBLIQUE)
    altitalicization = 'i';

  if ((numnames <= 0) && altitalicization)
  {
    PR_snprintf(&wildstring[namelen + 1], namelen + 200,
               "*-%s-%s-%c-normal--*-*-%d-%d-*-*-*",
	             wildstring,
               (aFont.weight <= NS_FONT_WEIGHT_NORMAL) ? "medium" : "bold",
               altitalicization, dpi, dpi);

    fnames = ::XListFontsWithInfo(dpy, &wildstring[namelen + 1], 200, &numnames, &fonts);
  }


  if (numnames <= 0)
  {
    //we were not able to match the font name at all...

    const char *newname = firstFace.ToNewCString();

    PR_snprintf(&wildstring[namelen + 1], namelen + 200,
               "*-%s-%s-%c-normal--*-*-%d-%d-*-*-*",
               newname,
               (aFont.weight <= NS_FONT_WEIGHT_NORMAL) ? "medium" : "bold",
               (aFont.style == NS_FONT_STYLE_NORMAL) ? 'r' :
               ((aFont.style == NS_FONT_STYLE_ITALIC) ? 'i' : 'o'), dpi, dpi);

    fnames = ::XListFontsWithInfo(dpy, &wildstring[namelen + 1], 200, &numnames, &fonts);

    if ((numnames <= 0) && altitalicization)
    {
      PR_snprintf(&wildstring[namelen + 1], namelen + 200,
                 "*-%s-%s-%c-normal--*-*-%d-%d-*-*-*",
                 newname,
                 (aFont.weight <= NS_FONT_WEIGHT_NORMAL) ? "medium" : "bold",
                 altitalicization, dpi, dpi);

      fnames = ::XListFontsWithInfo(dpy, &wildstring[namelen + 1], 200, &numnames, &fonts);
    }

    delete [] newname;
  }

  if (numnames > 0)
  {
    char *nametouse = PickAppropriateSize(fnames, fonts, numnames, aFont.size);
    
    mFontHandle = ::XLoadFont(dpy, nametouse);

#ifdef NOISY_FONTS
    fprintf(stderr, " is: %s\n", nametouse);
#endif

    ::XFreeFontInfo(fnames, fonts, numnames);
  }
  else
  {
    //ack. we're in real trouble, go for fixed...

#ifdef NOISY_FONTS
    fprintf(stderr, " is: %s\n", "fixed (final fallback)");
#endif

    mFontHandle = ::XLoadFont(dpy, "fixed");
  }

  RealizeFont();

  PR_Free(wildstring);

  return NS_OK;
}

char * nsFontMetricsUnix::PickAppropriateSize(char **names, XFontStruct *fonts, int cnt, nscoord desired)
{
  int         idx;
  PRInt32     desiredpix = NSToIntRound(mContext->GetAppUnitsToDevUnits() * desired);
  XFontStruct *curfont;
  PRInt32     closestmin = -1, minidx;

  //first try an exact or closest smaller match...

  for (idx = 0, curfont = fonts; idx < cnt; idx++, curfont++)
  {
    PRInt32 height = curfont->ascent + curfont->descent;

    if (height == desiredpix)
      break;

    if ((height < desiredpix) && (height > closestmin))
    {
      closestmin = height;
      minidx = idx;
    }
  }

  if (idx < cnt)
    return names[idx];
  else if (closestmin >= 0)
    return names[minidx];
  else
  {
    closestmin = 2000000;

    for (idx = 0, curfont = fonts; idx < cnt; idx++, curfont++)
    {
      PRInt32 height = curfont->ascent + curfont->descent;

      if ((height > desiredpix) && (height < closestmin))
      {
        closestmin = height;
        minidx = idx;
      }
    }

    return names[minidx];
  }
}

void nsFontMetricsUnix::RealizeFont()
{
  mFontInfo = ::XQueryFont(XtDisplay((Widget)mContext->GetNativeWidget()), mFontHandle);

  float f  = mContext->GetDevUnitsToAppUnits();
  
  mAscent = nscoord(mFontInfo->ascent * f);
  mDescent = nscoord(mFontInfo->descent * f);
  mMaxAscent = nscoord(mFontInfo->ascent * f) ;
  mMaxDescent = nscoord(mFontInfo->descent * f);
  
  mHeight = nscoord((mFontInfo->ascent + mFontInfo->descent) * f) ;
  mMaxAdvance = nscoord(mFontInfo->max_bounds.width * f);

  PRUint32 i;

  for (i = 0; i < 256; i++)
  {
    if ((i < mFontInfo->min_char_or_byte2) || (i > mFontInfo->max_char_or_byte2))
      mCharWidths[i] = mMaxAdvance;
    else
      mCharWidths[i] = nscoord((mFontInfo->per_char[i - mFontInfo->min_char_or_byte2].width) * f);
  }

  mLeading = 0;
}

nscoord nsFontMetricsUnix :: GetWidth(char ch)
{
  if (ch < 256)
    return mCharWidths[ch];
  else
    return 0; //XXX
}

nscoord nsFontMetricsUnix :: GetWidth(PRUnichar ch)
{
  if (ch < 256)
    return mCharWidths[PRUint8(ch)];
  else
    return 0;/* XXX */
}

nscoord nsFontMetricsUnix :: GetWidth(const nsString& aString)
{
  return GetWidth(aString.GetUnicode(), aString.Length());
}

nscoord nsFontMetricsUnix :: GetWidth(const char *aString)
{
  PRInt32 rc = 0 ;
  
  rc = (PRInt32) ::XTextWidth(mFontInfo, aString, nsCRT::strlen(aString));

  return (nscoord(rc * mContext->GetDevUnitsToAppUnits()));
}

nscoord nsFontMetricsUnix :: GetWidth(const PRUnichar *aString, PRUint32 aLength)
{
  XChar2b * thischar ;
  PRUint16 aunichar;
  nscoord width ;
  PRUint32 i ;
  PRUint32 desiredSize = sizeof(XChar2b) * aLength;

   // Make the temporary buffer larger if needed.
  if (nsnull == mXstring) {
    mXstring = (XChar2b *) PR_Malloc(desiredSize);
    mXstringSize = desiredSize;
  }
  else {
    if (mXstringSize < desiredSize) {
      mXstring = (XChar2b *) PR_Realloc(mXstring, desiredSize);
      mXstringSize = desiredSize;
    }
  }

  for (i=0; i<aLength; i++) {
    thischar = (mXstring+i);
    aunichar = (PRUint16)(*(aString+i));
    thischar->byte2 = (aunichar & 0xff);
    thischar->byte1 = (aunichar & 0xff00) >> 8;      
  }
  
  width = ::XTextWidth16(mFontInfo, mXstring, aLength);

  return (nscoord(width * mContext->GetDevUnitsToAppUnits()));
}

// XXX this needs to be implemented
nscoord nsFontMetricsUnix :: GetWidth(nsIDeviceContext *aContext, const nsString& aString)
{
  return GetWidth(aString.GetUnicode(), aString.Length());
}

nscoord nsFontMetricsUnix :: GetHeight()
{
  return mHeight;
}

nscoord nsFontMetricsUnix :: GetLeading()
{
  return mLeading;
}

nscoord nsFontMetricsUnix :: GetMaxAscent()
{
  return mMaxAscent;
}

nscoord nsFontMetricsUnix :: GetMaxDescent()
{
  return mMaxDescent;
}

nscoord nsFontMetricsUnix :: GetMaxAdvance()
{
  return mMaxAdvance;
}

const nscoord * nsFontMetricsUnix :: GetWidths()
{
  return mCharWidths;
}

const nsFont& nsFontMetricsUnix :: GetFont()
{
  return *mFont;
}

nsFontHandle nsFontMetricsUnix :: GetFontHandle()
{
  return (nsFontHandle)mFontHandle;
}


static void MapGenericFamilyToFont(const nsString& aGenericFamily, nsIDeviceContext* aDC,
                                   nsString& aFontFace)
{
  // the CSS generic names (conversions from Nav for now)
  // XXX this  need to check availability with the dc
  PRBool  aliased;
  if (aGenericFamily.EqualsIgnoreCase("serif")) {
    aDC->GetLocalFontName("times", aFontFace, aliased);
  }
  else if (aGenericFamily.EqualsIgnoreCase("sans-serif")) {
    aDC->GetLocalFontName("helvetica", aFontFace, aliased);
  }
  else if (aGenericFamily.EqualsIgnoreCase("cursive")) {
    aDC->GetLocalFontName("script", aFontFace, aliased);  // XXX ???
  }
  else if (aGenericFamily.EqualsIgnoreCase("fantasy")) {
    aDC->GetLocalFontName("helvetica", aFontFace, aliased);
  }
  else if (aGenericFamily.EqualsIgnoreCase("monospace")) {
    aDC->GetLocalFontName("fixed", aFontFace, aliased);
  }
  else {
    aFontFace.Truncate();
  }
}


