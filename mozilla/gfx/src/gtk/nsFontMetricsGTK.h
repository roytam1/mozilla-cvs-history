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

#ifndef nsFontMetricsGTK_h__
#define nsFontMetricsGTK_h__

#include "nsDeviceContextGTK.h"
#include "nsIFontMetrics.h"
#include "nsIFontEnumerator.h"
#include "nsFont.h"
#include "nsString.h"
#include "nsUnitConversion.h"
#include "nsIDeviceContext.h"
#include "nsCRT.h"
#include "nsCOMPtr.h"
#include "nsRenderingContextGTK.h"
#include "nsICharRepresentable.h"

#include <gdk/gdk.h>
#include <gdk/gdkx.h>

#undef FONT_HAS_GLYPH
#define FONT_HAS_GLYPH(map, char) IS_REPRESENTABLE(map, char)

typedef struct nsFontCharSetInfo nsFontCharSetInfo;

typedef gint (*nsFontCharSetConverter)(nsFontCharSetInfo* aSelf,
  XFontStruct* aFont, const PRUnichar* aSrcBuf, PRInt32 aSrcLen,
  char* aDestBuf, PRInt32 aDestLen);

struct nsFontCharSet;
struct nsFontFamily;
struct nsFontNode;
struct nsFontStretch;

class nsFontGTKUserDefined;
class nsFontMetricsGTK;

class nsFontGTK
{
public:
  nsFontGTK();
  virtual ~nsFontGTK();
  NS_DECL_AND_IMPL_ZEROING_OPERATOR_NEW

  void LoadFont(void);

  inline int SupportsChar(PRUnichar aChar)
    { return mFont && FONT_HAS_GLYPH(mMap, aChar); };

  virtual GdkFont* GetGDKFont(void);
  virtual PRBool   GetGDKFontIs10646(void);
  virtual gint GetWidth(const PRUnichar* aString, PRUint32 aLength) = 0;
  virtual gint DrawString(nsRenderingContextGTK* aContext,
                          nsDrawingSurfaceGTK* aSurface, nscoord aX,
                          nscoord aY, const PRUnichar* aString,
                          PRUint32 aLength) = 0;
#ifdef MOZ_MATHML
  // bounding metrics for a string 
  // remember returned values are not in app units 
  // - to emulate GetWidth () above
  virtual nsresult
  GetBoundingMetrics(const PRUnichar*   aString,
                     PRUint32           aLength,
                     nsBoundingMetrics& aBoundingMetrics) = 0;
#endif

  PRUint32*              mMap;
  nsFontCharSetInfo*     mCharSetInfo;
  char*                  mName;
  nsFontGTKUserDefined*  mUserDefinedFont;
  PRUint16               mSize;
  PRInt16                mBaselineAdjust;

protected:
  GdkFont*               mFont;
};

class nsFontMetricsGTK : public nsIFontMetrics
{
public:
  nsFontMetricsGTK();
  virtual ~nsFontMetricsGTK();

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

  nsFontGTK*  FindFont(PRUnichar aChar);
  nsFontGTK*  FindUserDefinedFont(PRUnichar aChar);
  nsFontGTK*  FindLocalFont(PRUnichar aChar);
  nsFontGTK*  FindGenericFont(PRUnichar aChar);
  nsFontGTK*  FindGlobalFont(PRUnichar aChar);
  nsFontGTK*  FindSubstituteFont(PRUnichar aChar);

  nsFontGTK*  SearchNode(nsFontNode* aNode, PRUnichar aChar);
  nsFontGTK*  TryAliases(nsCString* aName, PRUnichar aChar);
  nsFontGTK*  TryFamily(nsCString* aName, PRUnichar aChar);
  nsFontGTK*  TryNode(nsCString* aName, PRUnichar aChar);

  nsFontGTK*  PickASizeAndLoad(nsFontStretch* aStretch,
                               nsFontCharSetInfo* aCharSet, PRUnichar aChar);

  static nsresult FamilyExists(const nsString& aFontName);

  //friend struct nsFontGTK;

  nsFontGTK   **mLoadedFonts;
  PRUint16    mLoadedFontsAlloc;
  PRUint16    mLoadedFontsCount;

  nsFontGTK   *mSubstituteFont;

  nsCStringArray mFonts;
  PRUint16       mFontsIndex;
  nsVoidArray    mFontIsGeneric;

  nsCAutoString     mDefaultFont;
  nsCString         *mGeneric;
  nsCOMPtr<nsIAtom> mLangGroup;
  nsCAutoString     mUserDefined;

  PRUint8 mTriedAllGenerics;
  PRUint8 mIsUserDefined;

protected:
  void RealizeFont();

  nsIDeviceContext    *mDeviceContext;
  nsFont              *mFont;
  nsFontGTK           *mWesternFont;

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

class nsFontEnumeratorGTK : public nsIFontEnumerator
{
public:
  nsFontEnumeratorGTK();
  NS_DECL_ISUPPORTS
  NS_DECL_NSIFONTENUMERATOR
};

#endif
