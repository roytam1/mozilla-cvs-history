/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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

#include "xp_core.h"
#include "nsFontMetricsQT.h"
#include "nspr.h"

#undef NOISY_FONTS
#undef REALLY_NOISY_FONTS

static NS_DEFINE_IID(kIFontMetricsIID, NS_IFONT_METRICS_IID);

nsFontMetricsQT::nsFontMetricsQT()
{
printf("JCG: nsFontMetricsQT CTOR.\n");

    PR_LOG(QtGfxLM, PR_LOG_DEBUG, ("nsFontMetricsQT::nsFontMetricsQT\n"));
    NS_INIT_REFCNT();
    mDeviceContext = nsnull;
    mFont = nsnull;
    mFontHandle = nsnull;

    mHeight = 0;
    mAscent = 0;
    mDescent = 0;
    mLeading = 0;
    mMaxAscent = 0;
    mMaxDescent = 0;
    mMaxAdvance = 0;
    mXHeight = 0;
    mSuperscriptOffset = 0;
    mSubscriptOffset = 0;
    mStrikeoutSize = 0;
    mStrikeoutOffset = 0;
    mUnderlineSize = 0;
    mUnderlineOffset = 0;
}

nsFontMetricsQT::~nsFontMetricsQT()
{
printf("JCG: nsFontMetricsQT DTOR.\n");

    PR_LOG(QtGfxLM, PR_LOG_DEBUG, ("nsFontMetricsQT::~nsFontMetricsQT\n"));
    if (nsnull != mFont) {
        delete mFont;
        mFont = nsnull;
    }
    if (nsnull != mFontHandle) {
        delete mFontHandle;
        mFontHandle = nsnull;
    }
}

NS_IMPL_ISUPPORTS1(nsFontMetricsQT, nsIFontMetrics)

NS_IMETHODIMP nsFontMetricsQT::Init(const nsFont& aFont, nsIAtom* aLangGroup,
                                    nsIDeviceContext* aContext)
{
    PR_LOG(QtGfxLM, PR_LOG_DEBUG, ("nsFontMetricsQT::Init\n"));
    NS_ASSERTION(!(nsnull == aContext), 
                 "attempt to init fontmetrics with null device context");

    char * fontname = nsnull;
    float t2d;
    float a2d;
    float d2a;
    float cps;

    aContext->GetTwipsToDevUnits(t2d);
    aContext->GetAppUnitsToDevUnits(a2d);
    aContext->GetDevUnitsToAppUnits(d2a);
    aContext->GetCanonicalPixelScale(cps);

    PR_LOG(QtGfxLM, 
           PR_LOG_DEBUG, 
           ("nsFontMetricsQT::Init: twipstodevunits=%f, apptodevunits=%f, devtoappunits=%f, pixelscale=%f\n", 
            t2d, 
            a2d, 
            d2a, 
            cps));

    PRInt32 size = (PRInt32)(aFont.size * a2d);

    nsAutoString firstFace;
    if (NS_OK != aContext->FirstExistingFont(aFont, firstFace)) 
        aFont.GetFirstFamily(firstFace);

    mFont = new nsFont(aFont);
    mLangGroup = aLangGroup;
    mDeviceContext = aContext;
    mFontHandle = nsnull;

    fontname = firstFace.ToNewCString();

    int weight = (aFont.weight <= NS_FONT_WEIGHT_NORMAL) ? QFont::Normal : QFont::Bold;
    bool italic = (aFont.style == NS_FONT_STYLE_ITALIC);
    QString fontstring(fontname);

    PR_LOG(QtGfxLM, 
           PR_LOG_DEBUG, 
           ("nsFontMetricsQT::Init: Creating font: %s, %d pt\n", 
            (const char *)fontstring, 
            size));

printf("JCG nsFontMetricsQt Init. FontName: %s, Size: %d, Weight: %d, Italic: %s\n",(const char *)fontstring,size,weight,italic?"yes":"no");

    mFontHandle = new QFont(fontstring, size, weight, italic);

    RealizeFont();

    if (fontname)
        delete [] fontname;

    return NS_OK;
}

NS_IMETHODIMP nsFontMetricsQT::Destroy()
{
    PR_LOG(QtGfxLM, PR_LOG_DEBUG, ("nsFontMetricsQT::Destroy\n"));
    return NS_OK;
}

void nsFontMetricsQT::RealizeFont()
{
    PR_LOG(QtGfxLM, PR_LOG_DEBUG, ("nsFontMetricsQT::RealizeFont\n"));
    QFontMetrics fm(*mFontHandle);

    float f;
    mDeviceContext->GetDevUnitsToAppUnits(f);

    mAscent = nscoord(fm.ascent() * f);
    mDescent = nscoord(fm.descent() * f);
    mMaxAscent = nscoord(fm.ascent() * f) ;
    mMaxDescent = nscoord(fm.descent() * f);

    mHeight = nscoord(fm.height() * f);
    mMaxAdvance = nscoord(fm.maxWidth() * f);

    mEmHeight = mHeight;
    mEmAscent = mMaxAscent;  
    mEmDescent = mMaxDescent;
 
    // 56% of ascent, best guess for non-true type
    mXHeight = NSToCoordRound((float) fm.ascent() * f * 0.56f);

    mUnderlineOffset = nscoord(fm.underlinePos() * f);

    /* mHeight is already multipled by f */
    mUnderlineSize = NSToIntRound(fm.lineWidth() * f);

    mSuperscriptOffset = mXHeight;
    mSubscriptOffset = mXHeight;

    mStrikeoutOffset = nscoord(fm.strikeOutPos() * f);
    mStrikeoutSize = mUnderlineSize;

    mLeading = nscoord(fm.leading() * f);
}

NS_IMETHODIMP nsFontMetricsQT::GetXHeight(nscoord& aResult)
{
    PR_LOG(QtGfxLM, PR_LOG_DEBUG, ("nsFontMetricsQT::GetXHeight\n"));
    aResult = mXHeight;
    return NS_OK;
}

NS_IMETHODIMP nsFontMetricsQT::GetSuperscriptOffset(nscoord& aResult)
{
    PR_LOG(QtGfxLM, PR_LOG_DEBUG, ("nsFontMetricsQT::GetSuperscriptOffset\n"));
    aResult = mSuperscriptOffset;
    return NS_OK;
}

NS_IMETHODIMP nsFontMetricsQT::GetSubscriptOffset(nscoord& aResult)
{
    PR_LOG(QtGfxLM, PR_LOG_DEBUG, ("nsFontMetricsQT::GetSubscriptOffset\n"));
    aResult = mSubscriptOffset;
    return NS_OK;
}

NS_IMETHODIMP nsFontMetricsQT::GetStrikeout(nscoord& aOffset, nscoord& aSize)
{
    PR_LOG(QtGfxLM, PR_LOG_DEBUG, ("nsFontMetricsQT::GetStrikeout\n"));
    aOffset = mStrikeoutOffset;
    aSize = mStrikeoutSize;
    return NS_OK;
}

NS_IMETHODIMP nsFontMetricsQT::GetUnderline(nscoord& aOffset, nscoord& aSize)
{
    PR_LOG(QtGfxLM, PR_LOG_DEBUG, ("nsFontMetricsQT::GetUnderline\n"));
    aOffset = mUnderlineOffset;
    aSize = mUnderlineSize;
    return NS_OK;
}

NS_IMETHODIMP nsFontMetricsQT::GetHeight(nscoord &aHeight)
{
    PR_LOG(QtGfxLM, PR_LOG_DEBUG, ("nsFontMetricsQT::GetHeight\n"));
    aHeight = mHeight;
    return NS_OK;
}

NS_IMETHODIMP  nsFontMetricsQT::GetNormalLineHeight(nscoord &aHeight)
{
  aHeight = mEmHeight + mLeading;
  return NS_OK;
}
 
NS_IMETHODIMP nsFontMetricsQT::GetLeading(nscoord &aLeading)
{
    PR_LOG(QtGfxLM, PR_LOG_DEBUG, ("nsFontMetricsQT::GetLeading\n"));
    aLeading = mLeading;
    return NS_OK;
}

NS_IMETHODIMP  nsFontMetricsQT::GetEmHeight(nscoord &aHeight)
{
  aHeight = mEmHeight;
  return NS_OK;
}
 
NS_IMETHODIMP  nsFontMetricsQT::GetEmAscent(nscoord &aAscent)
{
  aAscent = mEmAscent;
  return NS_OK;
}
 
NS_IMETHODIMP  nsFontMetricsQT::GetEmDescent(nscoord &aDescent)
{
  aDescent = mEmDescent;
  return NS_OK;
}
 
NS_IMETHODIMP  nsFontMetricsQT::GetMaxHeight(nscoord &aHeight)
{
  aHeight = mMaxHeight;
  return NS_OK;
}
 
NS_IMETHODIMP nsFontMetricsQT::GetMaxAscent(nscoord &aAscent)
{
    PR_LOG(QtGfxLM, PR_LOG_DEBUG, ("nsFontMetricsQT::GetMaxAscent\n"));
    aAscent = mMaxAscent;
    return NS_OK;
}

NS_IMETHODIMP nsFontMetricsQT::GetMaxDescent(nscoord &aDescent)
{
    PR_LOG(QtGfxLM, PR_LOG_DEBUG, ("nsFontMetricsQT::GetMaxDescent\n"));
    aDescent = mMaxDescent;
    return NS_OK;
}

NS_IMETHODIMP nsFontMetricsQT::GetMaxAdvance(nscoord &aAdvance)
{
    PR_LOG(QtGfxLM, PR_LOG_DEBUG, ("nsFontMetricsQT::GetMaxAdvance\n"));
    aAdvance = mMaxAdvance;
    return NS_OK;
}

NS_IMETHODIMP nsFontMetricsQT::GetFont(const nsFont*& aFont)
{
    PR_LOG(QtGfxLM, PR_LOG_DEBUG, ("nsFontMetricsQT::GetFont\n"));
    aFont = mFont;
    return NS_OK;
}

NS_IMETHODIMP nsFontMetricsQT::GetLangGroup(nsIAtom** aLangGroup)
{
  if (!aLangGroup) {
    return NS_ERROR_NULL_POINTER;
  }
  *aLangGroup = mLangGroup;
  NS_IF_ADDREF(*aLangGroup);

  return NS_OK;
}


NS_IMETHODIMP nsFontMetricsQT::GetFontHandle(nsFontHandle &aHandle)
{
    PR_LOG(QtGfxLM, PR_LOG_DEBUG, ("nsFontMetricsQT::GetFontHandle\n"));
    aHandle = (nsFontHandle)mFontHandle;
    return NS_OK;
}

// The Font Enumerator
nsFontEnumeratorQT::nsFontEnumeratorQT()
{
printf("JCG: nsFontEnumeratorQT CTOR.\n");
  NS_INIT_REFCNT();
}
 
NS_IMPL_ISUPPORTS(nsFontEnumeratorQT,
                  NS_GET_IID(nsIFontEnumerator));

NS_IMETHODIMP
nsFontEnumeratorQT::EnumerateAllFonts(PRUint32* aCount, PRUnichar*** aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);
  *aResult = nsnull;
  NS_ENSURE_ARG_POINTER(aCount);
  *aCount = 0;

#ifdef DEBUG
printf("JCG: nsFontEnumeratorQT::EnumerateAllFonts not implemented!\n");
#endif

 return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsFontEnumeratorQT::EnumerateFonts(const char* aLangGroup, const char* aGeneric,
                                   PRUint32* aCount, PRUnichar*** aResult)
{ 
  NS_ENSURE_ARG_POINTER(aResult);
  *aResult = nsnull;
  NS_ENSURE_ARG_POINTER(aCount);
  *aCount = 0;
  NS_ENSURE_ARG_POINTER(aGeneric);
  NS_ENSURE_ARG_POINTER(aLangGroup);

  nsCOMPtr<nsIAtom> langGroup = getter_AddRefs(NS_NewAtom(aLangGroup));

#ifdef DEBUG
printf("JCG: nsFontEnumeratorQT::EnumerateFonts not implemented!\n");
#endif

 return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsFontEnumeratorQT::HaveFontFor(const char* aLangGroup, PRBool* aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);
  *aResult = PR_FALSE;
  NS_ENSURE_ARG_POINTER(aLangGroup);

  *aResult = PR_TRUE; // always return true for now.
  // Finish me - ftang
  return NS_OK;
}
