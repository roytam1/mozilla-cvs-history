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

#ifndef nsFontMetricsMac_h__
#define nsFontMetricsMac_h__

#include "nsIFontMetrics.h"
#include "nsFont.h"
#include "nsString.h"
#include "nsUnitConversion.h"
#include "nsIDeviceContext.h"
#include "nsCRT.h"

class nsFontMetricsMac : public nsIFontMetrics
{
public:
  nsFontMetricsMac();
  ~nsFontMetricsMac();

  void* operator new(size_t sz) {
    void* rv = new char[sz];
    nsCRT::zero(rv, sz);
    return rv;
  }

  NS_DECL_ISUPPORTS

  NS_IMETHOD  Init(const nsFont& aFont, nsIDeviceContext* aContext);
  NS_IMETHOD  Destroy();

  NS_IMETHOD  GetXHeight(nscoord& aResult);
  NS_IMETHOD  GetSuperscriptOffset(nscoord& aResult);
  NS_IMETHOD  GetSubscriptOffset(nscoord& aResult);
  NS_IMETHOD  GetWidth(char aC, nscoord &aWidth);
  NS_IMETHOD  GetWidth(PRUnichar aC, nscoord &aWidth);
  NS_IMETHOD  GetWidth(const nsString& aString, nscoord &aWidth);
  NS_IMETHOD  GetWidth(const char *aString, nscoord &aWidth);
  NS_IMETHOD  GetWidth(const char* aString, PRUint32 aLength, nscoord& aWidth);
  NS_IMETHOD  GetWidth(const PRUnichar *aString, PRUint32 aLength, nscoord &aWidth);
  NS_IMETHOD  GetWidth(nsIDeviceContext *aContext, const nsString& aString, nscoord &aWidth);
  NS_IMETHOD  GetHeight(nscoord &aHeight);
  NS_IMETHOD  GetLeading(nscoord &aLeading);
  NS_IMETHOD  GetMaxAscent(nscoord &aAscent);
  NS_IMETHOD  GetMaxDescent(nscoord &aDescent);
  NS_IMETHOD  GetMaxAdvance(nscoord &aAdvance);
  NS_IMETHOD  GetWidths(const nscoord *&aWidths);
  NS_IMETHOD  GetFont(const nsFont *&aFont);
  NS_IMETHOD  GetFontHandle(nsFontHandle& aHandle);

/*protected:
  void RealizeFont();
  static const char* MapFamilyToFont(const nsString& aLogicalFontName);

protected:
  void QueryFont();
  char *PickAppropriateSize(char **names, XFontStruct *fonts, int cnt, nscoord desired);

  nsFont            *mFont;
  nsIDeviceContext  *mContext;
  XFontStruct       *mFontInfo;
  Font              mFontHandle;
  nscoord           mCharWidths[256];
  nscoord           mHeight;
  nscoord           mAscent;
  nscoord           mDescent;
  nscoord           mLeading;
  nscoord           mMaxAscent;
  nscoord           mMaxDescent;
  nscoord           mMaxAdvance;*/

  nsFont            *mFont;
  nsIDeviceContext  *mContext;
};

#endif




