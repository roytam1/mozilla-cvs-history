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

#include "nsPrintOptionsImpl.h"
#include "nsCoord.h"
#include "nsUnitConversion.h"

// For Prefs
#include "nsIPref.h"
#include "nsIServiceManager.h"

//NS_IMPL_ISUPPORTS1(nsPrintOptions, nsIPrintOptions)
  NS_IMPL_ADDREF(nsPrintOptions)                       
  NS_IMPL_RELEASE(nsPrintOptions)                      
NS_IMETHODIMP nsPrintOptions::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
  if (NULL == aInstancePtr) {
    return NS_ERROR_NULL_POINTER;
  }

  *aInstancePtr = NULL;

  if (aIID.Equals(NS_GET_IID(nsIPrintOptions))) {
    *aInstancePtr = (void*) (nsIPrintOptions*)this;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  if (aIID.Equals(NS_GET_IID(nsISupports))) {
    *aInstancePtr = (void*) ((nsISupports*)this);
    NS_ADDREF_THIS();
    return NS_OK;
  }
  return NS_NOINTERFACE;
}

// Pref Constants
const char * kMarginTop       = "print.print_margin_top";
const char * kMarginLeft      = "print.print_margin_left";
const char * kMarginBottom    = "print.print_margin_bottom";
const char * kMarginRight     = "print.print_margin_right";

// Prefs for Print Options
const char * kPrintEvenPages  = "print.print_evenpages";
const char * kPrintOddPages   = "print.print_oddpages";
const char * kPrintDocTitle   = "print.print_doctitle";
const char * kPrintDocLoc     = "print.print_doclocation";
const char * kPageNums        = "print.print_pagenumbers";
const char * kPageNumsJust    = "print.print_pagenumjust";
const char * kPrintPageTotals = "print.print_pagetotals";
const char * kPrintDate       = "print.print_date";

// Additional Prefs
const char * kPrintReversed   = "print.print_reversed";
const char * kPrintColor      = "print.print_color";
const char * kPrintPaperSize  = "print.print_paper_size";
const char * kPrintCommand    = "print.print_command";
const char * kPrintFile       = "print.print_file";
const char * kPrintToFile     = "print.print_tofile";
const char * kPrintPageDelay  = "print.print_pagedelay";

// There are currently NOT supported
//const char * kPrintBevelLines    = "print.print_bevellines";
//const char * kPrintBlackText     = "print.print_blacktext";
//const char * kPrintBlackLines    = "print.print_blacklines";
//const char * kPrintLastPageFirst = "print.print_lastpagefirst";
//const char * kPrintBackgrounds   = "print.print_backgrounds";

const char * kLeftJust   = "left";
const char * kCenterJust = "center";
const char * kRightJust  = "right";

#define form_properties "chrome://communicator/locale/printing.properties"

/** ---------------------------------------------------
 *  See documentation in nsPrintOptionsImpl.h
 *	@update 6/21/00 dwc
 */
nsPrintOptions::nsPrintOptions() :
  mPrintRange(kRangeAllPages),
  mStartPageNum(1),
  mEndPageNum(1),
  mPrintOptions(0L),
  mPrintReversed(PR_FALSE),
  mPrintInColor(PR_TRUE),
  mPaperSize(kLetterPaperSize),
  mPrintToFile(PR_FALSE),
  mPrintFrameType(kFramesAsIs),
  mHowToEnableFrameUI(kFrameEnableNone),
  mPageNumJust(kJustLeft),
  mIsCancelled(PR_FALSE),
  mPrintPageDelay(500),
  mPrintSilent(PR_FALSE)
{
  NS_INIT_ISUPPORTS();

  /* member initializers and constructor code */
  nscoord halfInch = NS_INCHES_TO_TWIPS(0.5);
  mMargin.SizeTo(halfInch, halfInch, halfInch, halfInch);

  mPrintOptions = kOptPrintOddPages    |
                  kOptPrintEvenPages   |
                  kOptPrintDocTitle    |
                  kOptPrintDocLoc      |
                  kOptPrintPageNums    |
                  kOptPrintPageTotal   |
                  kOptPrintDatePrinted;

  mDefaultFont = new nsFont("Times", NS_FONT_STYLE_NORMAL,NS_FONT_VARIANT_NORMAL,
                             NS_FONT_WEIGHT_NORMAL,0,NSIntPointsToTwips(10));

  ReadPrefs();
}

/** ---------------------------------------------------
 *  See documentation in nsPrintOptionsImpl.h
 *	@update 6/21/00 dwc
 */
nsPrintOptions::~nsPrintOptions()
{
  if (mDefaultFont != nsnull) {
    delete mDefaultFont;
  }
}

/** ---------------------------------------------------
 *  See documentation in nsPrintOptionsImpl.h
 *	@update 1/12/01 rods
 */
NS_IMETHODIMP 
nsPrintOptions::SetFontNamePointSize(nsString& aFontName, PRInt32 aPointSize)
{
  if (mDefaultFont != nsnull && aFontName.Length() > 0 && aPointSize > 0) {
    mDefaultFont->name = aFontName;
    mDefaultFont->size = NSIntPointsToTwips(aPointSize);
  }
  return NS_OK;
}

/** ---------------------------------------------------
 *  See documentation in nsPrintOptionsImpl.h
 *	@update 1/12/01 rods
 */
NS_IMETHODIMP 
nsPrintOptions::SetDefaultFont(nsFont &aFont)
{
  if (mDefaultFont != nsnull) {
    delete mDefaultFont;
  }
  mDefaultFont = new nsFont(aFont);
  return NS_OK;
}

/** ---------------------------------------------------
 *  See documentation in nsPrintOptionsImpl.h
 *	@update 1/12/01 rods
 */
NS_IMETHODIMP 
nsPrintOptions::GetDefaultFont(nsFont &aFont)
{
  aFont = *mDefaultFont;
  return NS_OK;
}

/** ---------------------------------------------------
 *  See documentation in nsPrintOptionsImpl.h
 *	@update 6/21/00 dwc
 *	@update 1/12/01 rods
 */
NS_IMETHODIMP 
nsPrintOptions::SetMarginInTwips(nsMargin& aMargin)
{
  mMargin = aMargin;
  return NS_OK;
}

/** ---------------------------------------------------
 *  See documentation in nsPrintOptionsImpl.h
 *	@update 6/21/00 dwc
 */
NS_IMETHODIMP 
nsPrintOptions::GetMarginInTwips(nsMargin& aMargin)
{
  aMargin = mMargin;
  return NS_OK;
}

/** ---------------------------------------------------
 *  See documentation in nsPrintOptionsImpl.h
 *	@update 6/21/00 dwc
 */
NS_IMETHODIMP 
nsPrintOptions::ShowNativeDialog()
{

  return NS_OK;
}

/** ---------------------------------------------------
 *  See documentation in nsPrintOptionsImpl.h
 *	@update 1/12/01 rods
 */
NS_IMETHODIMP 
nsPrintOptions::SetPrintOptions(PRInt32 aType, PRBool aTurnOnOff)
{
  if (aTurnOnOff) {
    mPrintOptions |=  aType;
  } else {
    mPrintOptions &= ~aType;
  }
  return NS_OK;
}

/** ---------------------------------------------------
 *  See documentation in nsPrintOptionsImpl.h
 *	@update 1/12/01 rods
 */
NS_IMETHODIMP 
nsPrintOptions::GetPrintOptions(PRInt32 aType, PRBool *aTurnOnOff)
{
  NS_ENSURE_ARG_POINTER(aTurnOnOff);
  *aTurnOnOff = mPrintOptions & aType;
  return NS_OK;
}

/** ---------------------------------------------------
 *  See documentation in nsPrintOptionsImpl.h
 *	@update 1/12/01 rods
 */
NS_IMETHODIMP 
nsPrintOptions::GetPrintOptionsBits(PRInt32 *aBits)
{
  NS_ENSURE_ARG_POINTER(aBits);
  *aBits = mPrintOptions;
  return NS_OK;
}


/** ---------------------------------------------------
 *  See documentation in nsPrintOptionsImpl.h
 *	@update 1/12/01 rods
 */
NS_IMETHODIMP 
nsPrintOptions::ReadPrefs()
{
  nsCOMPtr<nsIPref> prefs = do_GetService(NS_PREF_CONTRACTID);
  if (prefs) {
    ReadInchesToTwipsPref(prefs, kMarginTop,    mMargin.top);
    ReadInchesToTwipsPref(prefs, kMarginLeft,   mMargin.left);
    ReadInchesToTwipsPref(prefs, kMarginBottom, mMargin.bottom);
    ReadInchesToTwipsPref(prefs, kMarginRight,  mMargin.right);

    ReadBitFieldPref(prefs, kPrintEvenPages,  kOptPrintEvenPages);
    ReadBitFieldPref(prefs, kPrintOddPages,   kOptPrintOddPages);
    ReadBitFieldPref(prefs, kPrintDocTitle,   kOptPrintDocTitle);
    ReadBitFieldPref(prefs, kPrintDocLoc,     kOptPrintDocLoc);
    ReadBitFieldPref(prefs, kPageNums,        kOptPrintPageNums);
    ReadBitFieldPref(prefs, kPrintPageTotals, kOptPrintPageTotal);
    ReadBitFieldPref(prefs, kPrintDate,       kOptPrintDatePrinted);

    ReadJustification(prefs, kPageNumsJust, mPageNumJust, kJustLeft);

    // Read Additional XP Prefs
    prefs->GetBoolPref(kPrintReversed,   &mPrintReversed);
    prefs->GetBoolPref(kPrintColor,      &mPrintInColor);
    prefs->GetIntPref(kPrintPaperSize,   &mPaperSize);
    ReadPrefString(prefs, kPrintCommand, mPrintCommand);
    prefs->GetBoolPref(kPrintFile,       &mPrintToFile);
    ReadPrefString(prefs, kPrintToFile,  mToFileName);
    prefs->GetIntPref(kPrintPageDelay,   &mPrintPageDelay);

    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}

/** ---------------------------------------------------
 *  See documentation in nsPrintOptionsImpl.h
 *	@update 1/12/01 rods
 */
NS_IMETHODIMP 
nsPrintOptions::WritePrefs()
{
  nsCOMPtr<nsIPref> prefs = do_GetService(NS_PREF_CONTRACTID);
  if (prefs) {
    WriteInchesFromTwipsPref(prefs, kMarginTop,    mMargin.top);
    WriteInchesFromTwipsPref(prefs, kMarginLeft,   mMargin.left);
    WriteInchesFromTwipsPref(prefs, kMarginBottom, mMargin.bottom);
    WriteInchesFromTwipsPref(prefs, kMarginRight,  mMargin.right);

    WriteBitFieldPref(prefs, kPrintEvenPages,  kOptPrintEvenPages);
    WriteBitFieldPref(prefs, kPrintOddPages,   kOptPrintOddPages);
    WriteBitFieldPref(prefs, kPrintDocTitle,   kOptPrintDocTitle);
    WriteBitFieldPref(prefs, kPrintDocLoc,     kOptPrintDocLoc);
    WriteBitFieldPref(prefs, kPageNums,        kOptPrintPageNums);
    WriteBitFieldPref(prefs, kPrintPageTotals, kOptPrintPageTotal);
    WriteBitFieldPref(prefs, kPrintDate,       kOptPrintDatePrinted);

    WriteJustification(prefs, kPageNumsJust, mPageNumJust);

    // Write Additional XP Prefs
    prefs->SetBoolPref(kPrintReversed,    mPrintReversed);
    prefs->SetBoolPref(kPrintColor,       mPrintInColor);
    prefs->SetIntPref(kPrintPaperSize,    mPaperSize);
    WritePrefString(prefs, kPrintCommand, mPrintCommand);
    prefs->SetBoolPref(kPrintFile,        mPrintToFile);
    WritePrefString(prefs, kPrintToFile,  mToFileName);
    prefs->SetIntPref(kPrintPageDelay,    mPrintPageDelay);

    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}

/* attribute long startPageRange; */
NS_IMETHODIMP nsPrintOptions::GetStartPageRange(PRInt32 *aStartPageRange)
{
  //NS_ENSURE_ARG_POINTER(aStartPageRange);
  *aStartPageRange = mStartPageNum;
  return NS_OK;
}
NS_IMETHODIMP nsPrintOptions::SetStartPageRange(PRInt32 aStartPageRange)
{
  mStartPageNum = aStartPageRange;
  return NS_OK;
}

/* attribute long endPageRange; */
NS_IMETHODIMP nsPrintOptions::GetEndPageRange(PRInt32 *aEndPageRange)
{
  //NS_ENSURE_ARG_POINTER(aEndPageRange);
  *aEndPageRange = mEndPageNum;
  return NS_OK;
}
NS_IMETHODIMP nsPrintOptions::SetEndPageRange(PRInt32 aEndPageRange)
{
  mEndPageNum = aEndPageRange;
  return NS_OK;
}

/* attribute boolean printReversed; */
NS_IMETHODIMP nsPrintOptions::GetPrintReversed(PRBool *aPrintReversed)
{
  //NS_ENSURE_ARG_POINTER(aPrintReversed);
  *aPrintReversed = mPrintReversed;
  return NS_OK;
}
NS_IMETHODIMP nsPrintOptions::SetPrintReversed(PRBool aPrintReversed)
{
  mPrintReversed = aPrintReversed;
  return NS_OK;
}

/* attribute boolean printInColor; */
NS_IMETHODIMP nsPrintOptions::GetPrintInColor(PRBool *aPrintInColor)
{
  //NS_ENSURE_ARG_POINTER(aPrintInColor);
  *aPrintInColor = mPrintInColor;
  return NS_OK;
}
NS_IMETHODIMP nsPrintOptions::SetPrintInColor(PRBool aPrintInColor)
{
  mPrintInColor = aPrintInColor;
  return NS_OK;
}

/* attribute short paperSize; */
NS_IMETHODIMP nsPrintOptions::GetPaperSize(PRInt32 *aPaperSize)
{
  //NS_ENSURE_ARG_POINTER(aPaperSize);
  *aPaperSize = mPaperSize;
  return NS_OK;
}
NS_IMETHODIMP nsPrintOptions::SetPaperSize(PRInt32 aPaperSize)
{
  mPaperSize = aPaperSize;
  return NS_OK;
}

/* attribute wstring printCommand; */
NS_IMETHODIMP nsPrintOptions::GetPrintCommand(PRUnichar * *aPrintCommand)
{
  //NS_ENSURE_ARG_POINTER(aPrintCommand);
  *aPrintCommand = mPrintCommand.ToNewUnicode();
  return NS_OK;
}
NS_IMETHODIMP nsPrintOptions::SetPrintCommand(const PRUnichar * aPrintCommand)
{
  mPrintCommand = aPrintCommand;
  return NS_OK;
}

/* attribute boolean printToFile; */
NS_IMETHODIMP nsPrintOptions::GetPrintToFile(PRBool *aPrintToFile)
{
  //NS_ENSURE_ARG_POINTER(aPrintToFile);
  *aPrintToFile = mPrintToFile;
  return NS_OK;
}
NS_IMETHODIMP nsPrintOptions::SetPrintToFile(PRBool aPrintToFile)
{
  mPrintToFile = aPrintToFile;
  return NS_OK;
}

/* attribute wstring toFileName; */
NS_IMETHODIMP nsPrintOptions::GetToFileName(PRUnichar * *aToFileName)
{
  //NS_ENSURE_ARG_POINTER(aToFileName);
  *aToFileName = mToFileName.ToNewUnicode();
  return NS_OK;
}
NS_IMETHODIMP nsPrintOptions::SetToFileName(const PRUnichar * aToFileName)
{
  mToFileName = aToFileName;
  return NS_OK;
}

/* attribute long printPageDelay; */
NS_IMETHODIMP nsPrintOptions::GetPrintPageDelay(PRInt32 *aPrintPageDelay)
{
  *aPrintPageDelay = mPrintPageDelay;
  return NS_OK;
}
NS_IMETHODIMP nsPrintOptions::SetPrintPageDelay(PRInt32 aPrintPageDelay)
{
  mPrintPageDelay = aPrintPageDelay;
  return NS_OK;
}

/* attribute double marginTop; */
NS_IMETHODIMP nsPrintOptions::GetMarginTop(double *aMarginTop)
{
  NS_ENSURE_ARG_POINTER(aMarginTop);
  *aMarginTop = NS_TWIPS_TO_INCHES(mMargin.top);
  return NS_OK;
}
NS_IMETHODIMP nsPrintOptions::SetMarginTop(double aMarginTop)
{
  mMargin.top = NS_INCHES_TO_TWIPS(float(aMarginTop));
  return NS_OK;
}

/* attribute double marginLeft; */
NS_IMETHODIMP nsPrintOptions::GetMarginLeft(double *aMarginLeft)
{
  NS_ENSURE_ARG_POINTER(aMarginLeft);
  *aMarginLeft = NS_TWIPS_TO_INCHES(mMargin.left);
  return NS_OK;
}
NS_IMETHODIMP nsPrintOptions::SetMarginLeft(double aMarginLeft)
{
  mMargin.left = NS_INCHES_TO_TWIPS(float(aMarginLeft));
  return NS_OK;
}

/* attribute double marginBottom; */
NS_IMETHODIMP nsPrintOptions::GetMarginBottom(double *aMarginBottom)
{
  NS_ENSURE_ARG_POINTER(aMarginBottom);
  *aMarginBottom = NS_TWIPS_TO_INCHES(mMargin.bottom);
  return NS_OK;
}
NS_IMETHODIMP nsPrintOptions::SetMarginBottom(double aMarginBottom)
{
  mMargin.bottom = NS_INCHES_TO_TWIPS(float(aMarginBottom));
  return NS_OK;
}

/* attribute double marginRight; */
NS_IMETHODIMP nsPrintOptions::GetMarginRight(double *aMarginRight)
{
  NS_ENSURE_ARG_POINTER(aMarginRight);
  *aMarginRight = NS_TWIPS_TO_INCHES(mMargin.right);
  return NS_OK;
}
NS_IMETHODIMP nsPrintOptions::SetMarginRight(double aMarginRight)
{
  mMargin.right = NS_INCHES_TO_TWIPS(float(aMarginRight));
  return NS_OK;
}

/* attribute long printRange; */
NS_IMETHODIMP nsPrintOptions::GetPrintRange(PRInt16 *aPrintRange)
{
  NS_ENSURE_ARG_POINTER(aPrintRange);
  *aPrintRange = mPrintRange;
  return NS_OK;
}
NS_IMETHODIMP nsPrintOptions::SetPrintRange(PRInt16 aPrintRange)
{
  mPrintRange = aPrintRange;
  return NS_OK;
}

/* attribute long pageNumJust; */
NS_IMETHODIMP nsPrintOptions::GetPageNumJust(PRInt16 *aPageNumJust)
{
  NS_ENSURE_ARG_POINTER(aPageNumJust);
  *aPageNumJust = mPageNumJust;
  return NS_OK;
}
NS_IMETHODIMP nsPrintOptions::SetPageNumJust(PRInt16 aPageNumJust)
{
  mPageNumJust = aPageNumJust;
  return NS_OK;
}

/* attribute wstring docTitle; */
NS_IMETHODIMP nsPrintOptions::GetTitle(PRUnichar * *aTitle)
{
  NS_ENSURE_ARG_POINTER(aTitle);
  *aTitle = mTitle.ToNewUnicode();
  return NS_OK;
}
NS_IMETHODIMP nsPrintOptions::SetTitle(const PRUnichar * aTitle)
{
  NS_ENSURE_ARG_POINTER(aTitle);
  mTitle = aTitle;
  return NS_OK;
}

/* attribute wstring docURL; */
NS_IMETHODIMP nsPrintOptions::GetDocURL(PRUnichar * *aDocURL)
{
  NS_ENSURE_ARG_POINTER(aDocURL);
  *aDocURL = mURL.ToNewUnicode();
  return NS_OK;
}
NS_IMETHODIMP nsPrintOptions::SetDocURL(const PRUnichar * aDocURL)
{
  NS_ENSURE_ARG_POINTER(aDocURL);
  mURL = aDocURL;
  return NS_OK;
}

/* attribute boolean isPrintFrame; */
NS_IMETHODIMP nsPrintOptions::GetHowToEnableFrameUI(PRInt16 *aHowToEnableFrameUI)
{
  NS_ENSURE_ARG_POINTER(aHowToEnableFrameUI);
  *aHowToEnableFrameUI = (PRInt32)mHowToEnableFrameUI;
  return NS_OK;
}
NS_IMETHODIMP nsPrintOptions::SetHowToEnableFrameUI(PRInt16 aHowToEnableFrameUI)
{
  mHowToEnableFrameUI = aHowToEnableFrameUI;
  return NS_OK;
}

/* attribute long printFrame; */
NS_IMETHODIMP nsPrintOptions::GetPrintFrameType(PRInt16 *aPrintFrameType)
{
  NS_ENSURE_ARG_POINTER(aPrintFrameType);
  *aPrintFrameType = (PRInt32)mPrintFrameType;
  return NS_OK;
}
NS_IMETHODIMP nsPrintOptions::SetPrintFrameType(PRInt16 aPrintFrameType)
{
  mPrintFrameType = aPrintFrameType;
  return NS_OK;
}

/* attribute long isCancelled; */
NS_IMETHODIMP nsPrintOptions::GetIsCancelled(PRBool *aIsCancelled)
{
  NS_ENSURE_ARG_POINTER(aIsCancelled);
  *aIsCancelled = mIsCancelled;
  return NS_OK;
}
NS_IMETHODIMP nsPrintOptions::SetIsCancelled(PRBool aIsCancelled)
{
  mIsCancelled = aIsCancelled;
  return NS_OK;
}


/* attribute long isCancelled; */
NS_IMETHODIMP nsPrintOptions::GetPrintSilent(PRBool *aPrintSilent)
{
  NS_ENSURE_ARG_POINTER(aPrintSilent);
  *aPrintSilent = mPrintSilent;
  return NS_OK;
}
NS_IMETHODIMP nsPrintOptions::SetPrintSilent(PRBool aPrintSilent)
{
  mPrintSilent = aPrintSilent;
  return NS_OK;
}


//-----------------------------------------------------
//-- Protected Methods
//-----------------------------------------------------
//---------------------------------------------------
nsresult nsPrintOptions::ReadPrefString(nsIPref *    aPref, 
                                        const char * aPrefId, 
                                        nsString&    aString)
{
  char * str = nsnull;
  nsresult rv = aPref->CopyCharPref(aPrefId, &str);
  if (NS_SUCCEEDED(rv) && str) {
    aString.AssignWithConversion(str);
    nsMemory::Free(str);
  }
  return rv;
}

nsresult nsPrintOptions::WritePrefString(nsIPref *    aPref, 
                                         const char * aPrefId, 
                                         nsString&    aString)
{
  NS_ENSURE_ARG_POINTER(aPref);
  NS_ENSURE_ARG_POINTER(aPrefId);

  PRUnichar * str = aString.ToNewUnicode();
  nsresult rv = aPref->SetUnicharPref(aPrefId, str);
  nsMemory::Free(str);

  return rv;
}

void nsPrintOptions::ReadBitFieldPref(nsIPref *    aPref, 
                                      const char * aPrefId, 
                                      PRInt32      anOption)
{
  PRBool b;
  if (NS_SUCCEEDED(aPref->GetBoolPref(aPrefId, &b))) {
    SetPrintOptions(anOption, b);
  }
}

//---------------------------------------------------
void nsPrintOptions::WriteBitFieldPref(nsIPref *    aPref, 
                                      const char * aPrefId, 
                                      PRInt32      anOption)
{
  PRBool b;
  GetPrintOptions(anOption, &b);
  aPref->SetBoolPref(aPrefId, b);
}

//---------------------------------------------------
void nsPrintOptions::ReadInchesToTwipsPref(nsIPref *    aPref, 
                                           const char * aPrefId, 
                                           nscoord&     aTwips)
{
  char * str = nsnull;
  nsresult rv = aPref->CopyCharPref(aPrefId, &str);
  if (NS_SUCCEEDED(rv) && str) {
    nsAutoString justStr;
    justStr.AssignWithConversion(str);
    PRInt32 errCode;
    float inches = justStr.ToFloat(&errCode);
    if (NS_SUCCEEDED(errCode)) {
      aTwips = NS_INCHES_TO_TWIPS(inches);
    } else {
      aTwips = 0;
    }
    nsMemory::Free(str);
  }
}

//---------------------------------------------------
void nsPrintOptions::WriteInchesFromTwipsPref(nsIPref *    aPref, 
                                              const char * aPrefId, 
                                              nscoord      aTwips)
{
  double inches = NS_TWIPS_TO_INCHES(aTwips);
  nsAutoString inchesStr;
  inchesStr.AppendFloat(inches);
  char * str = inchesStr.ToNewCString();
  if (str) {
    aPref->SetCharPref(aPrefId, str);
  } else {
    aPref->SetCharPref(aPrefId, "0.5");
  }
}

void nsPrintOptions::ReadJustification(nsIPref *    aPref, 
                                       const char * aPrefId, 
                                       PRInt16&     aJust,
                                       PRInt16      aInitValue)
{
  aJust = aInitValue;
  nsAutoString justStr;
  if (NS_SUCCEEDED(ReadPrefString(aPref, aPrefId, justStr))) {
    if (justStr.EqualsWithConversion(kRightJust)) {
      aJust = kJustRight;

    } else if (justStr.EqualsWithConversion(kCenterJust)) {
      aJust = kJustCenter;

    } else {
      aJust = kJustLeft;
    }
  }
}

//---------------------------------------------------
void nsPrintOptions::WriteJustification(nsIPref *    aPref, 
                                        const char * aPrefId, 
                                        PRInt16      aJust)
{
  switch (aJust) {
    case kJustLeft: 
      aPref->SetCharPref(aPrefId, kLeftJust);
      break;

    case kJustCenter: 
      aPref->SetCharPref(aPrefId, kCenterJust);
      break;

    case kJustRight: 
      aPref->SetCharPref(aPrefId, kRightJust);
      break;
  } //switch
}



