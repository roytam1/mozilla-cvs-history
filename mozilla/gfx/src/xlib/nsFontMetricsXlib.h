/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 * vim:ts=2:et:sw=2
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
 * Tony Tsui <tony@igelaus.com.au>
 */

#ifndef nsFontMetricsXlib_h__
#define nsFontMetricsXlib_h__

#include "nsICharRepresentable.h"
#include "nsIDeviceContext.h"
#include "nsIFontMetrics.h"
#include "nsIFontEnumerator.h"

#include "nsCRT.h"
#include "nsCOMPtr.h"
#include "nsDeviceContextXlib.h"
#include "nsDrawingSurfaceXlib.h"
#ifdef USE_XPRINT
#include "nsDeviceContextXP.h"
#include "nsXPrintContext.h"
#include "nsRenderingContextXP.h"
#endif /* USE_XPRINT */
#include "nsFont.h"
#include "nsRenderingContextXlib.h"
#include "nsString.h"
#include "nsUnitConversion.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>

// Remove this eventually after I remove all usage from implementation file. TonyT
#define FONT_SWITCHING

#undef FONT_HAS_GLYPH
#define FONT_HAS_GLYPH(map, char) IS_REPRESENTABLE(map, char)

typedef struct nsFontCharSetXlibInfo nsFontCharSetXlibInfo;

typedef int (*nsFontCharSetXlibConverter)(nsFontCharSetXlibInfo* aSelf,
             XFontStruct* aFont, const PRUnichar* aSrcBuf, PRInt32 aSrcLen, 
             char* aDestBuf, PRInt32 aDestLen);

struct nsFontCharSetXlib;
struct nsFontFamilyXlib;
struct nsFontNodeXlib;
struct nsFontStretchXlib;

class nsFontXlibUserDefined;
class nsFontMetricsXlib;

class nsFontXlib
{
public:
  nsFontXlib();
  virtual ~nsFontXlib();
  NS_DECL_AND_IMPL_ZEROING_OPERATOR_NEW

  void LoadFont(void);

  inline int SupportsChar(PRUnichar aChar)
    { return mFont && FONT_HAS_GLYPH(mMap, aChar); };
    

  virtual int GetWidth(const PRUnichar* aString, PRUint32 aLength) = 0;
  virtual int DrawString(nsRenderingContextXlib* aContext,
                         nsDrawingSurfaceXlib* aSurface,
                         nscoord aX, nscoord aY,
                         const PRUnichar* aString, PRUint32 aLength) = 0;
#ifdef USE_XPRINT
   virtual int DrawString(nsRenderingContextXp* aContext,
                          nsXPrintContext* aSurface,
                          nscoord aX,
                          nscoord aY, const PRUnichar* aString,
                          PRUint32 aLength) = 0;      
#endif /* USE_XPRINT */

#ifdef MOZ_MATHML
  // bounding metrics for a string 
  // remember returned values are not in app units 
  // - to emulate GetWidth () above
  virtual nsresult
  GetBoundingMetrics(const PRUnichar*   aString,
                     PRUint32           aLength,
                     nsBoundingMetrics& aBoundingMetrics) = 0;
#endif /* MOZ_MATHML */

  XFontStruct           *mFont;
  PRUint32              *mMap;
  nsFontCharSetXlibInfo     *mCharSetInfo;
  char                  *mName;
  nsFontXlibUserDefined *mUserDefinedFont;
  PRUint16               mSize;
  PRInt16                mBaselineAdjust;
};

class nsFontMetricsXlib : public nsIFontMetrics
{
public:
  nsFontMetricsXlib();
  virtual ~nsFontMetricsXlib();

#ifdef USE_XPRINT
  static void     EnablePrinterMode(PRBool printermode);
#endif /* USE_XPRINT */

  static nsresult InitGlobals(void);
  static void     FreeGlobals(void);
  
  NS_DECL_AND_IMPL_ZEROING_OPERATOR_NEW

  NS_DECL_ISUPPORTS

  NS_IMETHOD  Init(const nsFont& aFont, nsIAtom* aLangGroup,
                   nsIDeviceContext* aContext);
  NS_IMETHOD  Destroy();

  NS_IMETHOD  GetXHeight(nscoord& aResult);
  NS_IMETHOD  GetSuperscriptOffset(nscoord& aResult);
  NS_IMETHOD  GetSubscriptOffset(nscoord& aResult);
  NS_IMETHOD  GetStrikeout(nscoord& aOffset, nscoord& aSize);
  NS_IMETHOD  GetUnderline(nscoord& aOffset, nscoord& aSize);

  NS_IMETHOD  GetHeight(nscoord &aHeight);
  NS_IMETHOD  GetNormalLineHeight(nscoord &aHeight);
  NS_IMETHOD  GetLeading(nscoord &aLeading);
  NS_IMETHOD  GetEmHeight(nscoord &aHeight);
  NS_IMETHOD  GetEmAscent(nscoord &aAscent);
  NS_IMETHOD  GetEmDescent(nscoord &aDescent);
  NS_IMETHOD  GetMaxHeight(nscoord &aHeight);
  NS_IMETHOD  GetMaxAscent(nscoord &aAscent);
  NS_IMETHOD  GetMaxDescent(nscoord &aDescent);
  NS_IMETHOD  GetMaxAdvance(nscoord &aAdvance);
  NS_IMETHOD  GetFont(const nsFont *&aFont);
  NS_IMETHOD  GetLangGroup(nsIAtom** aLangGroup);
  NS_IMETHOD  GetFontHandle(nsFontHandle &aHandle);
  
  virtual nsresult GetSpaceWidth(nscoord &aSpaceWidth);

  nsFontXlib* FindFont(PRUnichar aChar);
  nsFontXlib* FindUserDefinedFont(PRUnichar aChar);
  nsFontXlib* FindStyleSheetSpecificFont(PRUnichar aChar);
  nsFontXlib* FindStyleSheetGenericFont(PRUnichar aChar);
  nsFontXlib* FindLangGroupPrefFont(nsIAtom* aLangGroup, PRUnichar aChar);
  nsFontXlib* FindLangGroupFont(nsIAtom* aLangGroup, PRUnichar aChar, nsCString* aName);
  nsFontXlib* FindAnyFont(PRUnichar aChar);
  nsFontXlib* FindSubstituteFont(PRUnichar aChar);

  nsFontXlib* SearchNode(nsFontNodeXlib* aNode, PRUnichar aChar);
  nsFontXlib* TryAliases(nsCString* aName, PRUnichar aChar);
  nsFontXlib* TryFamily(nsCString* aName, PRUnichar aChar);
  nsFontXlib* TryNode(nsCString* aName, PRUnichar aChar);
  nsFontXlib* TryNodes(nsAWritableCString &aFFREName, PRUnichar aChar);
  nsFontXlib* TryLangGroup(nsIAtom* aLangGroup, nsCString* aName, PRUnichar aChar);

  nsFontXlib* PickASizeAndLoad(nsFontStretchXlib* aStretch,
                               nsFontCharSetXlibInfo* aCharSet,
                               PRUnichar aChar);

  static nsresult FamilyExists(const nsString& aFontName);

  nsCAutoString      mDefaultFont;
  nsCString          *mGeneric;
  PRUint8            mIsUserDefined;
  nsCOMPtr<nsIAtom>  mLangGroup;
  nsCAutoString      mUserDefined;

  nsCStringArray     mFonts;
  PRUint16           mFontsAlloc;
  PRUint16           mFontsCount;
  PRUint16           mFontsIndex;
  nsVoidArray        mFontIsGeneric;

  nsFontXlib         **mLoadedFonts;
  PRUint16           mLoadedFontsAlloc;
  PRUint16           mLoadedFontsCount;

  PRUint8            mTriedAllGenerics;
  nsFontXlib         *mSubstituteFont;

   
protected:
  void RealizeFont();

  nsIDeviceContext    *mDeviceContext;
  nsFont              *mFont;
  XFontStruct         *mFontHandle;
  XFontStruct         *mFontStruct;
  nsFontXlib          *mWesternFont;

  nscoord             mAscent;
  nscoord             mDescent;
  
  nscoord             mLeading;
  nscoord             mEmHeight;
  nscoord             mEmAscent;
  nscoord             mEmDescent;
  nscoord             mMaxHeight;
  nscoord             mMaxAscent;
  nscoord             mMaxDescent;
  nscoord             mMaxAdvance;
  nscoord             mXHeight;
  nscoord             mSuperscriptOffset;
  nscoord             mSubscriptOffset;
  nscoord             mStrikeoutSize;
  nscoord             mStrikeoutOffset;
  nscoord             mUnderlineSize;
  nscoord             mUnderlineOffset;
  nscoord             mSpaceWidth;

  PRUint16            mPixelSize;
  PRUint8             mStretchIndex;
  PRUint8             mStyleIndex;

#ifdef USE_XPRINT
public:  
  static PRPackedBool mPrinterMode;
#endif /* USE_XPRINT */  
};

class nsFontEnumeratorXlib : public nsIFontEnumerator
{
public:
  nsFontEnumeratorXlib();
  NS_DECL_ISUPPORTS
  NS_DECL_NSIFONTENUMERATOR
};

#endif
