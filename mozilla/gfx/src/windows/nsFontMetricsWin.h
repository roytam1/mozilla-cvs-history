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

  static void SetFontWeight(PRInt32 aWeight, PRUint16* aWeightTable);
  static PRBool IsFontWeightAvailable(PRInt32 aWeight, PRUint16 aWeightTable);

protected:
  // @description Font Weights
    // Each available font weight is stored as as single bit inside a PRUint16.
    // e.g. The binary value 0000000000001000 indcates font weight 400 is available.
    // while the binary value 0000000000001001 indicates both font weight 100 and 400 are available
    // The font weights which will be represented include {100, 200, 300, 400, 500, 600, 700, 800, 900}
    // The font weight specified in the mFont->weight may include values which are not an even multiple of 100.
    // If so, the font weight mod 100 indicates the number steps to lighten are make bolder.
    // This corresponds to the CSS lighter and bolder property values. If bolder is applied twice to the font which has
    // a font weight of 400 then the mFont->weight will contain the value 402.
    // If lighter is applied twice to a font of weight 400 then the mFont->weight will contain the value 398.
    // Only nine steps of bolder or lighter are allowed by the CSS XPCODE.
    // The font weight table is used in conjuction with the mFont->weight to determine
    // what font weight to pass in the LOGFONT structure.


    // Utility methods for managing font weights.
  PRUint16 LookForFontWeightTable(HDC aDc, nsString* aName);
  PRInt32  GetBolderWeight(PRInt32 aWeight, PRInt32 aDistance, PRUint16 aWeightTable);
  PRInt32  GetLighterWeight(PRInt32 aWeight, PRInt32 aDistance, PRUint16 aWeightTable);
  PRInt32  GetFontWeight(PRInt32 aWeight, PRUint16 aWeightTable);
  PRInt32  GetClosestWeight(PRInt32 aWeight, PRUint16 aWeightTable);
  PRUint16 GetFontWeightTable(HDC aDC, nsString* aFontName);
  
  void FillLogFont(LOGFONT* aLogFont, PRInt32 aWeight);
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
  static PLHashTable* gFontWeights;

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
