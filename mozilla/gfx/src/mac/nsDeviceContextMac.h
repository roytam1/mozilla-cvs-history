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

#ifndef nsDeviceContextMac_h___
#define nsDeviceContextMac_h___

#include "nsDeviceContext.h"
#include "nsUnitConversion.h"
#include "nsIWidget.h"
#include "nsIView.h"
#include "nsIRenderingContext.h"
#include "nsIFontEnumerator.h"
#include <Types.h>
#include <QuickDraw.h>

#include "nsIScreen.h"
#include "nsIScreenManager.h"


class nsDeviceContextMac : public DeviceContextImpl
{
public:
  nsDeviceContextMac();

  NS_IMETHOD  Init(nsNativeWidget aNativeWidget);  

  NS_IMETHOD  CreateRenderingContext(nsIRenderingContext *&aContext);
  NS_IMETHOD  SupportsNativeWidgets(PRBool &aSupportsWidgets);

  NS_IMETHOD  GetScrollBarDimensions(float &aWidth, float &aHeight) const;
  NS_IMETHOD  GetSystemAttribute(nsSystemAttrID anID, SystemAttrStruct * aInfo) const;

	void 				SetDrawingSurface(nsDrawingSurface  aSurface) { mSurface = aSurface; }
  NS_IMETHOD  GetDrawingSurface(nsIRenderingContext &aContext, nsDrawingSurface &aSurface);

  NS_IMETHOD 	CheckFontExistence(const nsString& aFontName);
  NS_IMETHOD 	CreateILColorSpace(IL_ColorSpace*& aColorSpace);
  NS_IMETHODIMP GetILColorSpace(IL_ColorSpace*& aColorSpace);
  NS_IMETHOD 	GetDepth(PRUint32& aDepth);
  NS_IMETHOD 	ConvertPixel(nscolor aColor, PRUint32 & aPixel);

  NS_IMETHOD 	GetDeviceSurfaceDimensions(PRInt32 &aWidth, PRInt32 &aHeight);
  NS_IMETHOD  GetRect(nsRect &aRect);
  NS_IMETHOD  GetClientRect(nsRect &aRect);

  NS_IMETHOD 	GetDeviceContextFor(nsIDeviceContextSpec *aDevice,
                                 nsIDeviceContext *&aContext);

  NS_IMETHOD 	BeginDocument(PRUnichar * aTitle);
  NS_IMETHOD 	EndDocument(void);

  NS_IMETHOD 	BeginPage(void);
  NS_IMETHOD 	EndPage(void);
  PRBool	IsPrinter() 	{if (nsnull != mSpec){return (PR_TRUE);}else{return (PR_FALSE);} };



protected:
  virtual 	~nsDeviceContextMac();
  
	virtual nsresult CreateFontAliasTable();

  void FindScreenForSurface ( nsIScreen** outScreen ) ;

  nsDrawingSurface 			mSurface;
  Rect									mPageRect;
  nsCOMPtr<nsIDeviceContextSpec> mSpec;
  GrafPtr								mOldPort;
  nsCOMPtr<nsIScreenManager> mScreenManager;
  nsCOMPtr<nsIScreen> mPrimaryScreen;         // cache the screen for single-monitor systems
  
public:
  // InitFontInfoList and nsHashTable are static because GetMacFontNumber is static
  static void InitFontInfoList();
  static nsHashtable* gFontInfoList;
public:
  static bool GetMacFontNumber(const nsString& aFontName, short &fontNum);

private:
	static PRUint32		mPixelsPerInch;
	static PRBool			mDisplayVerySmallFonts;
	static PRUint32   sNumberOfScreens;       // how many screens we have.
public:
	static PRUint32		GetScreenResolution();
	static PRBool			DisplayVerySmallFonts();
};


class nsFontEnumeratorMac : public nsIFontEnumerator
{
public:
  nsFontEnumeratorMac();
  NS_DECL_ISUPPORTS
  NS_DECL_NSIFONTENUMERATOR
};

#endif /* nsDeviceContextMac_h___ */
