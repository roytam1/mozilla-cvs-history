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
 *   Tony Tsui <tony@igelaus.com.au>
 *   Roland Mainz <roland.mainz@informatik.med.uni-giessen,de>
 */

#ifndef nsFontMetricsXp_h__
#define nsFontMetricsXp_h__ 1

#include "nsICharRepresentable.h"
#include "nsIDeviceContext.h"
#include "nsIFontMetrics.h"
#include "nsIFontEnumerator.h"

#include "nsCRT.h"
#include "nsCOMPtr.h"
#include "nsDeviceContextXP.h"
#ifdef XPRINT_NOT_NOW
#include "nsDrawingSurfaceXP.h"
#else
#include "nsXPrintContext.h"
#endif
#include "nsFont.h"
#include "nsRenderingContextXP.h"
#include "nsString.h"
#include "nsUnitConversion.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>

// Remove this eventually after I remove all usage from implementation file. TonyT
#define FONT_SWITCHING

#undef FONT_HAS_GLYPH
#define FONT_HAS_GLYPH(map, char) IS_REPRESENTABLE(map, char)

typedef struct nsFontCharSetInfo nsFontCharSetInfo;

typedef int (*nsFontCharSetConverter)(nsFontCharSetInfo* aSelf,
             XFontStruct* aFont, const PRUnichar* aSrcBuf, PRInt32 aSrcLen, 
                   char* aDestBuf, PRInt32 aDestLen);

struct nsFontCharSet;
struct nsFontFamily;
struct nsFontNode;
struct nsFontStretch;


class nsFontXpUserDefined;
class nsFontMetricsXp;

class nsFontXp
{
public:
  nsFontXp();
  virtual ~nsFontXp();
  NS_DECL_AND_IMPL_ZEROING_OPERATOR_NEW

  void LoadFont(void);

  inline int SupportsChar(PRUnichar aChar)
    { return mFont && FONT_HAS_GLYPH(mMap, aChar); };
    

  virtual int GetWidth(const PRUnichar* aString, PRUint32 aLength) = 0;
#ifdef XPRINT_NOT_NOW  
  virtual int DrawString(nsRenderingContextXp* aContext,
                         nsDrawingSurfaceXp*   aSurface,
                         nscoord aX, nscoord aY,
                         const PRUnichar* aString, PRUint32 aLength) = 0;
#else
  virtual int DrawString(nsXPrintContext* aSurface,
                         nscoord aX,
                         nscoord aY, const PRUnichar* aString,
                         PRUint32 aLength) = 0;                          
#endif /* XPRINT_NOT_NOW */                     

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
  nsFontCharSetInfo     *mCharSetInfo;
  char                  *mName;
  nsFontXpUserDefined   *mUserDefinedFont;
  PRUint16               mSize;
  PRInt16                mBaselineAdjust;
};

//typedef struct nsFontSearch nsFontSearch;

class nsFontMetricsXp : public nsIFontMetrics
{
public:
  nsFontMetricsXp();
  virtual ~nsFontMetricsXp();

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

  nsFontXp* FindFont(PRUnichar aChar);
  nsFontXp* FindUserDefinedFont(PRUnichar aChar);
  nsFontXp* FindLocalFont(PRUnichar aChar);
  nsFontXp* FindGenericFont(PRUnichar aChar);
  nsFontXp* FindGlobalFont(PRUnichar aChar);
  nsFontXp* FindSubstituteFont(PRUnichar aChar);

  nsFontXp* SearchNode(nsFontNode* aNode, PRUnichar aChar);
  nsFontXp* TryAliases(nsCString* aName, PRUnichar aChar); 
  nsFontXp* TryFamily(nsCString* aName, PRUnichar aChar);
  nsFontXp* TryNode(nsCString* aName, PRUnichar aChar);

  nsFontXp* PickASizeAndLoad(nsFontStretch* aStretch,
                               nsFontCharSetInfo* aCharSet,
                               PRUnichar aChar);

  static nsresult FamilyExists(const nsString& aFontName);

  nsCAutoString      mDefaultFont;

  nsCStringArray     mFonts;
  PRUint16           mFontsAlloc;
  PRUint16           mFontsCount;
  PRUint16           mFontsIndex;
  nsVoidArray        mFontIsGeneric;

  nsCString          *mGeneric;
  PRUint8            mIsUserDefined;
  nsCOMPtr<nsIAtom>  mLangGroup;

  nsFontXp         **mLoadedFonts;
  PRUint16           mLoadedFontsAlloc;
  PRUint16           mLoadedFontsCount;

  PRUint8            mTriedAllGenerics;
  nsFontXp         *mSubstituteFont;

  nsCAutoString      mUserDefined;
   
protected:
  void RealizeFont();

  nsIDeviceContext    *mDeviceContext;
  nsFont              *mFont;
  XFontStruct         *mFontHandle;
  XFontStruct         *mFontStruct;
  nsFontXp            *mWesternFont;

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
};

class nsFontEnumeratorXp : public nsIFontEnumerator
{
public:
  nsFontEnumeratorXp();
  NS_DECL_ISUPPORTS
  NS_DECL_NSIFONTENUMERATOR
};



#endif /* !nsFontMetricsXp_h__ */

