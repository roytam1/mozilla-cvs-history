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

#ifndef nsFontMetricsWin_h__
#define nsFontMetricsWin_h__

#include <windows.h>

#include "plhash.h"
#include "nsIFontMetrics.h"
#include "nsFont.h"
#include "nsString.h"
#include "nsUnitConversion.h"
#include "nsIDeviceContext.h"
#include "nsCRT.h"
#include "nsDeviceContextWin.h"

#ifdef FONT_HAS_GLYPH
#undef FONT_HAS_GLYPH
#endif
#define FONT_HAS_GLYPH(map, g) (((map)[(g) >> 3] >> ((g) & 7)) & 1)

#ifdef ADD_GLYPH
#undef ADD_GLYPH
#endif
#define ADD_GLYPH(map, g) (map)[(g) >> 3] |= (1 << ((g) & 7))

typedef struct nsFontWin
{
  HFONT    font;
  PRUint8* map;
} nsFontWin;

typedef struct nsGlobalFont
{
  nsString*     name;
  LOGFONT       logFont;
  PRUint8*      map;
  PRUint8       skip;
} nsGlobalFont;

class nsFontMetricsWin : public nsIFontMetrics
{
public:
  nsFontMetricsWin();
  virtual ~nsFontMetricsWin();

  NS_DECL_AND_IMPL_ZEROING_OPERATOR_NEW

  NS_DECL_ISUPPORTS

  NS_IMETHOD  Init(const nsFont& aFont, nsIDeviceContext* aContext);
  NS_IMETHOD  Destroy();

  NS_IMETHOD  GetXHeight(nscoord& aResult);
  NS_IMETHOD  GetSuperscriptOffset(nscoord& aResult);
  NS_IMETHOD  GetSubscriptOffset(nscoord& aResult);
  NS_IMETHOD  GetStrikeout(nscoord& aOffset, nscoord& aSize);
  NS_IMETHOD  GetUnderline(nscoord& aOffset, nscoord& aSize);

  NS_IMETHOD  GetHeight(nscoord &aHeight);
  NS_IMETHOD  GetLeading(nscoord &aLeading);
  NS_IMETHOD  GetMaxAscent(nscoord &aAscent);
  NS_IMETHOD  GetMaxDescent(nscoord &aDescent);
  NS_IMETHOD  GetMaxAdvance(nscoord &aAdvance);
  NS_IMETHOD  GetFont(const nsFont *&aFont);
  NS_IMETHOD  GetFontHandle(nsFontHandle &aHandle);

  virtual nsresult   GetSpaceWidth(nscoord &aSpaceWidth);
  virtual nsFontWin* FindGlobalFont(HDC aDC, PRUnichar aChar);
  virtual nsFontWin* FindLocalFont(HDC aDC, PRUnichar aChar);
  nsFontWin*         FindFont(HDC aDC, PRUnichar aChar);
  virtual nsFontWin* LoadFont(HDC aDC, nsString* aName);

  int SameAsPreviousMap(int aIndex);

  nsFontWin           **mLoadedFonts;
  PRUint16            mLoadedFontsAlloc;
  PRUint16            mLoadedFontsCount;

  nsString            *mFonts;
  PRUint16            mFontsAlloc;
  PRUint16            mFontsCount;
  PRUint16            mFontsIndex;
  nscoord						  mSpaceWidth;

  static nsGlobalFont* gGlobalFonts;
  static int gGlobalFontsCount;

protected:
  void FillLogFont(LOGFONT* aLogFont);
  void RealizeFont();

  nsDeviceContextWin  *mDeviceContext;
  nsFont              *mFont;
  nscoord             mHeight;
  nscoord             mAscent;
  nscoord             mDescent;
  nscoord             mLeading;
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
  HFONT               mFontHandle;

  static PLHashTable* InitializeFamilyNames(void);
  static PLHashTable* gFamilyNames;

  static nsGlobalFont* InitializeGlobalFonts(HDC aDC);

  static PRUint8* GetCMAP(HDC aDC);
};


// The following is a workaround for a Japanse Windows 95 problem.

typedef struct nsFontWinA nsFontWinA;

// must start with same fields as nsFontWin
struct nsFontSubset
{
  int Load(nsFontWinA* aFont);

  HFONT    mFont;
  PRUint8* mMap;
  BYTE     mCharSet;
  PRUint16 mCodePage;
};

struct nsFontWinA
{
  int GetSubsets(HDC aDC);

  LOGFONT       mLogFont;
  HFONT         mFont;
  PRUint8*      mMap;
  nsFontSubset* mSubsets;
  PRUint16      mSubsetsCount;
};

class nsFontMetricsWinA : public nsFontMetricsWin
{
public:
  virtual ~nsFontMetricsWinA();

  virtual nsFontWin* FindLocalFont(HDC aDC, PRUnichar aChar);
  virtual nsFontWin* FindGlobalFont(HDC aDC, PRUnichar aChar);
  virtual nsFontWin* LoadFont(HDC aDC, nsString* aName);
};

#endif /* nsFontMetricsWin_h__ */
