/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: NPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is 
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 2000
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Jessica Blanco <jblanco@us.ibm.com>
 *
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the NPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "nsPrintOptionsImpl.h"
#include "nsCoord.h"
#include "nsUnitConversion.h"
#include "nsReadableUtils.h"
#include "nsPrintSettingsImpl.h"

#include "nsIDOMWindow.h"
#include "nsIServiceManager.h"
#include "nsIDialogParamBlock.h"
#include "nsXPCOM.h"
#include "nsISupportsPrimitives.h"
#include "nsIWindowWatcher.h"
#include "nsIDOMWindowInternal.h"
#include "nsVoidArray.h"
#include "nsSupportsArray.h"

// For Prefs
#include "nsIPref.h"
#include "nsIServiceManager.h"

#include "nsISimpleEnumerator.h"
#include "nsISupportsPrimitives.h"
#include "nsGfxCIID.h"
 
static NS_DEFINE_IID(kCPrinterEnumerator, NS_PRINTER_ENUMERATOR_CID);

NS_IMPL_ISUPPORTS2(nsPrintOptions, nsIPrintOptions, nsIPrintSettingsService)

// Pref Constants
const char kMarginTop[]       = "print_margin_top";
const char kMarginLeft[]      = "print_margin_left";
const char kMarginBottom[]    = "print_margin_bottom";
const char kMarginRight[]     = "print_margin_right";

// Prefs for Print Options
const char kPrintEvenPages[]       = "print_evenpages";
const char kPrintOddPages[]        = "print_oddpages";
const char kPrintHeaderStrLeft[]   = "print_headerleft";
const char kPrintHeaderStrCenter[] = "print_headercenter";
const char kPrintHeaderStrRight[]  = "print_headerright";
const char kPrintFooterStrLeft[]   = "print_footerleft";
const char kPrintFooterStrCenter[] = "print_footercenter";
const char kPrintFooterStrRight[]  = "print_footerright";

// Additional Prefs
const char kPrintPaperSize[]     = "print_paper_size"; // this has been deprecated

const char kPrintReversed[]      = "print_reversed";
const char kPrintInColor[]       = "print_in_color";
const char kPrintPaperName[]     = "print_paper_name";
const char kPrintPaperSizeType[] = "print_paper_size_type";
const char kPrintPaperData[]     = "print_paper_data";
const char kPrintPaperSizeUnit[] = "print_paper_size_unit";
const char kPrintPaperWidth[]    = "print_paper_width";
const char kPrintPaperHeight[]   = "print_paper_height";
const char kPrintOrientation[]   = "print_orientation";
const char kPrintCommand[]       = "print_command";
const char kPrinterName[]        = "print_printer";
const char kPrintToFile[]        = "print_to_file";
const char kPrintToFileName[]    = "print_to_filename";
const char kPrintPageDelay[]     = "print_pagedelay";
const char kPrintBGColors[]       = "print_bgcolor";
const char kPrintBGImages[]      = "print_bgimages";

const char kJustLeft[]   = "left";
const char kJustCenter[] = "center";
const char kJustRight[]  = "right";

static NS_DEFINE_IID(kPrinterEnumeratorCID, NS_PRINTER_ENUMERATOR_CID);

nsFont* nsPrintOptions::sDefaultFont = nsnull;

/** ---------------------------------------------------
 *  See documentation in nsPrintOptionsImpl.h
 *	@update 6/21/00 dwc
 */
nsPrintOptions::nsPrintOptions()
{
  NS_INIT_ISUPPORTS();

  /* member initializers and constructor code */
  if (sDefaultFont == nsnull) {
    sDefaultFont = new nsFont("Times", NS_FONT_STYLE_NORMAL,NS_FONT_VARIANT_NORMAL,
                               NS_FONT_WEIGHT_NORMAL,0,NSIntPointsToTwips(10));
  }
}

/** ---------------------------------------------------
 *  See documentation in nsPrintOptionsImpl.h
 *	@update 6/21/00 dwc
 */
nsPrintOptions::~nsPrintOptions()
{
  if (sDefaultFont != nsnull) {
    delete sDefaultFont;
    sDefaultFont = nsnull;
  }
}


//**************************************************************
//** PageList Enumerator
//**************************************************************
class
nsPrinterListEnumerator : public nsISimpleEnumerator
{
   public:
     nsPrinterListEnumerator();
     virtual ~nsPrinterListEnumerator();

     //nsISupports interface
     NS_DECL_ISUPPORTS

     //nsISimpleEnumerator interface
     NS_DECL_NSISIMPLEENUMERATOR

     NS_IMETHOD Init();

   protected:
     PRUnichar **mPrinters;
     PRUint32 mCount;
     PRUint32 mIndex;
};

nsPrinterListEnumerator::nsPrinterListEnumerator() :
  mPrinters(nsnull), mCount(0), mIndex(0)
{
   NS_INIT_ISUPPORTS();
}

nsPrinterListEnumerator::~nsPrinterListEnumerator()
{
   if (mPrinters) {
      PRUint32 i;
      for (i = 0; i < mCount; i++ ) {
         nsMemory::Free(mPrinters[i]);
      }
      nsMemory::Free(mPrinters);
   }
}

NS_IMPL_ISUPPORTS1(nsPrinterListEnumerator, nsISimpleEnumerator)

NS_IMETHODIMP nsPrinterListEnumerator::Init()
{
   nsresult rv;
   nsCOMPtr<nsIPrinterEnumerator> printerEnumerator;

   printerEnumerator = do_CreateInstance(kCPrinterEnumerator, &rv);
   if (NS_FAILED(rv))
      return rv;

   return printerEnumerator->EnumeratePrinters(&mCount, &mPrinters);
}

NS_IMETHODIMP nsPrinterListEnumerator::HasMoreElements(PRBool *result)
{
   *result = (mIndex < mCount);
    return NS_OK;
}

NS_IMETHODIMP nsPrinterListEnumerator::GetNext(nsISupports **aPrinter)
{
   NS_ENSURE_ARG_POINTER(aPrinter);
   *aPrinter = nsnull;
   if (mIndex >= mCount) {
     return NS_ERROR_UNEXPECTED;
   }

   PRUnichar *printerName = mPrinters[mIndex++];
   nsCOMPtr<nsISupportsString> printerNameWrapper;
   nsresult rv;

   rv = nsComponentManager::CreateInstance(NS_SUPPORTS_STRING_CONTRACTID, nsnull,
                                           NS_GET_IID(nsISupportsString), getter_AddRefs(printerNameWrapper));
   NS_ENSURE_SUCCESS(rv, rv);
   NS_ENSURE_TRUE(printerNameWrapper, NS_ERROR_OUT_OF_MEMORY);
   printerNameWrapper->SetData(nsDependentString(printerName));
   *aPrinter = NS_STATIC_CAST(nsISupports*, printerNameWrapper);
   NS_ADDREF(*aPrinter);
   return NS_OK;
}

/** ---------------------------------------------------
 *  See documentation in nsPrintOptionsImpl.h
 *	@update 1/12/01 rods
 */
NS_IMETHODIMP 
nsPrintOptions::SetFontNamePointSize(nsString& aFontName, PRInt32 aPointSize)
{
  if (sDefaultFont != nsnull && aFontName.Length() > 0 && aPointSize > 0) {
    sDefaultFont->name = aFontName;
    sDefaultFont->size = NSIntPointsToTwips(aPointSize);
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
  if (sDefaultFont != nsnull) {
    delete sDefaultFont;
  }
  sDefaultFont = new nsFont(aFont);
  return NS_OK;
}

/** ---------------------------------------------------
 *  See documentation in nsPrintOptionsImpl.h
 *	@update 1/12/01 rods
 */
NS_IMETHODIMP 
nsPrintOptions::GetDefaultFont(nsFont &aFont)
{
  aFont = *sDefaultFont;
  return NS_OK;
}

/** ---------------------------------------------------
 *  See documentation in nsPrintOptionsImpl.h
 *	@update 6/21/00 dwc
 */
NS_IMETHODIMP 
nsPrintOptions::ShowPrintSetupDialog(nsIPrintSettings *aPS)
{  
 NS_ASSERTION(aPS, "Can't have a null PrintSettings!");
 if (aPS == nsnull) return NS_OK;

 nsresult rv = NS_ERROR_FAILURE;

 // create a nsISupportsArray of the parameters
 // being passed to the window
 nsCOMPtr<nsISupportsArray> array;
 NS_NewISupportsArray(getter_AddRefs(array));
 if (!array) return NS_ERROR_FAILURE;

 nsCOMPtr<nsISupports> psSupports(do_QueryInterface(aPS));
 NS_ASSERTION(psSupports, "PrintSettings must be a supports");
 array->AppendElement(psSupports);

 nsCOMPtr<nsIDialogParamBlock> ioParamBlock(do_CreateInstance(NS_DIALOGPARAMBLOCK_CONTRACTID));
 if (ioParamBlock) {
   ioParamBlock->SetInt(0, 0);
   nsCOMPtr<nsISupports> blkSupps(do_QueryInterface(ioParamBlock));
   NS_ASSERTION(blkSupps, "IOBlk must be a supports");

   array->AppendElement(blkSupps);
   nsCOMPtr<nsISupports> arguments(do_QueryInterface(array));
   NS_ASSERTION(array, "array must be a supports");

   nsCOMPtr<nsIWindowWatcher> wwatch(do_GetService(NS_WINDOWWATCHER_CONTRACTID));
   if (wwatch) {
     nsCOMPtr<nsIDOMWindow> active;
     wwatch->GetActiveWindow(getter_AddRefs(active));           nsCOMPtr<nsIDOMWindowInternal> parent = do_QueryInterface(active);

     nsCOMPtr<nsIDOMWindow> newWindow;
     rv = wwatch->OpenWindow(parent, "chrome://communicator/content/printPageSetup.xul",
                   "_blank", "chrome,modal,centerscreen", array,
                   getter_AddRefs(newWindow));
   }
 }

 return rv;
  
}

/** ---------------------------------------------------
 *  Helper function - Creates the "prefix" for the pref
 *  It is either "print."
 *  or "print.printer_<print name>."
 */
const char* nsPrintOptions::GetPrefName(const char *    aPrefName, 
                                        const nsString& aPrinterName)
{
  if (!aPrefName || !*aPrefName) {
    NS_ASSERTION(0, "Must have a valid pref name!");
    return aPrefName;
  }

  mPrefName.AssignWithConversion(NS_LITERAL_STRING("print."));

  if (aPrinterName.Length()) {
    mPrefName.AppendWithConversion(NS_LITERAL_STRING("printer_"));
    mPrefName.AppendWithConversion(aPrinterName);
    mPrefName.AppendWithConversion(NS_LITERAL_STRING("."));
  }
  mPrefName += aPrefName;

  return mPrefName.get();

}

//----------------------------------------------------------------------
// Testing of read/write prefs
// This define controls debug output 
#ifdef DEBUG_rods_X
static void WriteDebugStr(const char* aArg1, const char* aArg2, const PRUnichar* aStr)
{
  nsString str(aStr);
  PRUnichar s = '&';
  PRUnichar r = '_';
  str.ReplaceChar(s, r);

  printf("%s %s = %s \n", aArg1, aArg2, ToNewUTF8String(str));
}
const char* kWriteStr = "Write Pref:";
const char* kReadStr  = "Read Pref:";
#define DUMP_STR(_a1, _a2, _a3)  WriteDebugStr((_a1), GetPrefName((_a2), aPrefName), (_a3));
#define DUMP_BOOL(_a1, _a2, _a3) printf("%s %s = %s \n", (_a1), GetPrefName((_a2), aPrefName), (_a3)?"T":"F");
#define DUMP_INT(_a1, _a2, _a3)  printf("%s %s = %d \n", (_a1), GetPrefName((_a2), aPrefName), (_a3));
#define DUMP_DBL(_a1, _a2, _a3)  printf("%s %s = %10.5f \n", (_a1), GetPrefName((_a2), aPrefName), (_a3));
#else
#define DUMP_STR(_a1, _a2, _a3) 
#define DUMP_BOOL(_a1, _a2, _a3)
#define DUMP_INT(_a1, _a2, _a3)
#define DUMP_DBL(_a1, _a2, _a3)
#endif
//----------------------------------------------------------------------

/** ---------------------------------------------------
 *  This will read in the generic prefs (not specific to a printer)
 *  or it will it can read the prefs in using the printer name to qualify
 *  It is either "print.attr_name"
 *  or "print.printer_HPLasr5.attr_name"
 *
*/
nsresult 
nsPrintOptions::ReadPrefs(nsIPrintSettings* aPS, const nsString& aPrefName, PRUint32 aFlags)
{
  nsCOMPtr<nsIPref> prefs = do_GetService(NS_PREF_CONTRACTID);
  if (prefs) {
    if (aFlags & nsIPrintSettings::kInitSaveMargins) {
      nscoord halfInch = NS_INCHES_TO_TWIPS(0.5);
      nsMargin margin;
      margin.SizeTo(halfInch, halfInch, halfInch, halfInch);
      ReadInchesToTwipsPref(prefs, GetPrefName(kMarginTop, aPrefName),    margin.top);
      DUMP_INT(kReadStr, kMarginTop, margin.top);
      ReadInchesToTwipsPref(prefs, GetPrefName(kMarginLeft, aPrefName),   margin.left);
      DUMP_INT(kReadStr, kMarginLeft, margin.left);
      ReadInchesToTwipsPref(prefs, GetPrefName(kMarginBottom, aPrefName), margin.bottom);
      DUMP_INT(kReadStr, kMarginBottom, margin.bottom);
      ReadInchesToTwipsPref(prefs, GetPrefName(kMarginRight, aPrefName),  margin.right);
      DUMP_INT(kReadStr, kMarginRight, margin.right);
      aPS->SetMarginInTwips(margin);
    }

    PRBool   b;
    nsString str;
    PRInt32  iVal;
    double   dbl;

    if (aFlags & nsIPrintSettings::kInitSaveOddEvenPages) {
      if (NS_SUCCEEDED(prefs->GetBoolPref(GetPrefName(kPrintEvenPages, aPrefName), &b)))  {
        aPS->SetPrintOptions(nsIPrintSettings::kPrintEvenPages, b);
        DUMP_BOOL(kReadStr, kPrintEvenPages, b);
      }
    }

    if (aFlags & nsIPrintSettings::kInitSaveOddEvenPages) {
      if (NS_SUCCEEDED(prefs->GetBoolPref(GetPrefName(kPrintOddPages, aPrefName), &b))) {
        aPS->SetPrintOptions(nsIPrintSettings::kPrintOddPages, b);
        DUMP_BOOL(kReadStr, kPrintOddPages, b);
      }
    }

    if (aFlags & nsIPrintSettings::kInitSaveHeaderLeft) {
      if (NS_SUCCEEDED(ReadPrefString(prefs, GetPrefName(kPrintHeaderStrLeft, aPrefName), str))) {
        aPS->SetHeaderStrLeft(str.get());
        DUMP_STR(kReadStr, kPrintHeaderStrLeft, str.get());
      }
    }

    if (aFlags & nsIPrintSettings::kInitSaveHeaderCenter) {
      if (NS_SUCCEEDED(ReadPrefString(prefs, GetPrefName(kPrintHeaderStrCenter, aPrefName), str))) {
        aPS->SetHeaderStrCenter(str.get());
        DUMP_STR(kReadStr, kPrintHeaderStrCenter, str.get());
      }
    }

    if (aFlags & nsIPrintSettings::kInitSaveHeaderRight) {
      if (NS_SUCCEEDED(ReadPrefString(prefs, GetPrefName(kPrintHeaderStrRight, aPrefName), str))) {
        aPS->SetHeaderStrRight(str.get());
        DUMP_STR(kReadStr, kPrintHeaderStrRight, str.get());
      }
    }

    if (aFlags & nsIPrintSettings::kInitSaveFooterLeft) {
      if (NS_SUCCEEDED(ReadPrefString(prefs, GetPrefName(kPrintFooterStrLeft, aPrefName), str))) {
        aPS->SetFooterStrLeft(str.get());
        DUMP_STR(kReadStr, kPrintFooterStrLeft, str.get());
      }
    }

    if (aFlags & nsIPrintSettings::kInitSaveFooterCenter) {
      if (NS_SUCCEEDED(ReadPrefString(prefs, GetPrefName(kPrintFooterStrCenter, aPrefName), str))) {
        aPS->SetFooterStrCenter(str.get());
        DUMP_STR(kReadStr, kPrintFooterStrCenter, str.get());
      }
    }

    if (aFlags & nsIPrintSettings::kInitSaveFooterRight) {
      if (NS_SUCCEEDED(ReadPrefString(prefs, GetPrefName(kPrintFooterStrRight, aPrefName), str))) {
        aPS->SetFooterStrRight(str.get());
        DUMP_STR(kReadStr, kPrintFooterStrRight, str.get());
      }
    }

    if (aFlags & nsIPrintSettings::kInitSaveBGColors) {
      if (NS_SUCCEEDED(prefs->GetBoolPref(GetPrefName(kPrintBGColors, aPrefName), &b))) {
        aPS->SetPrintBGColors(b);
        DUMP_BOOL(kReadStr, kPrintBGColors, b);
      }
    }

    if (aFlags & nsIPrintSettings::kInitSaveBGImages) {
      if (NS_SUCCEEDED(prefs->GetBoolPref(GetPrefName(kPrintBGImages, aPrefName), &b))) {
        aPS->SetPrintBGImages(b);
        DUMP_BOOL(kReadStr, kPrintBGImages, b);
      }
    }

    if (aFlags & nsIPrintSettings::kInitSavePaperSize) {
      if (NS_SUCCEEDED(prefs->GetIntPref(GetPrefName(kPrintPaperSize, aPrefName), &iVal))) { // this has been deprecated
        aPS->SetPaperSize(iVal);
        DUMP_INT(kReadStr, kPrintPaperSize, iVal);
      }
    }

    if (aFlags & nsIPrintSettings::kInitSaveReversed) {
      if (NS_SUCCEEDED(prefs->GetBoolPref(GetPrefName(kPrintReversed, aPrefName), &b))) {
        aPS->SetPrintReversed(b);
        DUMP_BOOL(kReadStr, kPrintReversed, b);
      }
    }

    if (aFlags & nsIPrintSettings::kInitSaveInColor) {
      if (NS_SUCCEEDED(prefs->GetBoolPref(GetPrefName(kPrintInColor, aPrefName), &b))) {
        aPS->SetPrintInColor(b);
        DUMP_BOOL(kReadStr, kPrintInColor, b);
      }
    }

    if (aFlags & nsIPrintSettings::kInitSavePaperName) {
      if (NS_SUCCEEDED(ReadPrefString(prefs, GetPrefName(kPrintPaperName, aPrefName), str))) {
        aPS->SetPaperName(str.get());
        DUMP_STR(kReadStr, kPrintPaperName, str.get());
      }
    }

    if (aFlags & nsIPrintSettings::kInitSavePaperSizeUnit) {
      if (NS_SUCCEEDED(prefs->GetIntPref(GetPrefName(kPrintPaperSizeUnit, aPrefName),  &iVal))) {
        aPS->SetPaperSizeUnit(iVal);
        DUMP_INT(kReadStr, kPrintPaperSizeUnit, iVal);
      }
    }

    if (aFlags & nsIPrintSettings::kInitSavePaperSizeType) {
      if (NS_SUCCEEDED(prefs->GetIntPref(GetPrefName(kPrintPaperSizeType, aPrefName),  &iVal))) {
        aPS->SetPaperSizeType(iVal);
        DUMP_INT(kReadStr, kPrintPaperSizeType, iVal);
      }
    }

    if (aFlags & nsIPrintSettings::kInitSavePaperData) {
      if (NS_SUCCEEDED(prefs->GetIntPref(GetPrefName(kPrintPaperData, aPrefName),  &iVal))) {
        aPS->SetPaperData(iVal);
        DUMP_INT(kReadStr, kPrintPaperData, iVal);
      }
    }

    if (aFlags & nsIPrintSettings::kInitSavePaperWidth) {
      if (NS_SUCCEEDED(ReadPrefDouble(prefs, GetPrefName(kPrintPaperWidth, aPrefName), dbl))) {
        aPS->SetPaperWidth(dbl);
        DUMP_DBL(kReadStr, kPrintPaperWidth, dbl);
      }
    }

    if (aFlags & nsIPrintSettings::kInitSavePaperHeight) {
      if (NS_SUCCEEDED(ReadPrefDouble(prefs, GetPrefName(kPrintPaperHeight, aPrefName),  dbl))) {
        aPS->SetPaperHeight(dbl);
        DUMP_DBL(kReadStr, kPrintPaperHeight, dbl);
      }
    }

    if (aFlags & nsIPrintSettings::kInitSaveOrientation) {
      if (NS_SUCCEEDED(prefs->GetIntPref(GetPrefName(kPrintOrientation, aPrefName), &iVal))) {
        aPS->SetOrientation(iVal);
        DUMP_INT(kReadStr, kPrintOrientation, iVal);
      }
    }

    if (aFlags & nsIPrintSettings::kInitSavePrintCommand) {
      if (NS_SUCCEEDED(ReadPrefString(prefs, GetPrefName(kPrintCommand, aPrefName), str))) {
        aPS->SetPrintCommand(str.get());
        DUMP_STR(kReadStr, kPrintCommand, str.get());
      }
    }

    if (aFlags & nsIPrintSettings::kInitSavePrinterName) {
      if (NS_SUCCEEDED(ReadPrefString(prefs, GetPrefName(kPrinterName, aPrefName), str))) {
        aPS->SetPrinterName(str.get());
        DUMP_STR(kReadStr, kPrinterName, str.get());
      }
    }

     if (aFlags & nsIPrintSettings::kInitSavePrintToFile) {
     if (NS_SUCCEEDED(prefs->GetBoolPref(GetPrefName(kPrintToFile, aPrefName), &b))) {
        aPS->SetPrintToFile(b);
        DUMP_BOOL(kReadStr, kPrintToFile, b);
      }
    }

    if (aFlags & nsIPrintSettings::kInitSaveToFileName) {
      if (NS_SUCCEEDED(ReadPrefString(prefs, GetPrefName(kPrintToFileName, aPrefName),  str))) {
        aPS->SetToFileName(str.get());
        DUMP_STR(kReadStr, kPrintToFileName, str.get());
      }
    }

    if (aFlags & nsIPrintSettings::kInitSavePageDelay) {
      if (NS_SUCCEEDED(prefs->GetIntPref(GetPrefName(kPrintPageDelay, aPrefName),   &iVal))) {
        aPS->SetPrintPageDelay(iVal);
        DUMP_INT(kReadStr, kPrintPageDelay, iVal);
      }
    }

    // Not Reading In:
    //   Scaling
    //   ShrinkToFit
    //   Number of Copies

    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}

/** ---------------------------------------------------
 *  See documentation in nsPrintOptionsImpl.h
 *	@update 1/12/01 rods
 */
nsresult 
nsPrintOptions::WritePrefs(nsIPrintSettings *aPS, const nsString& aPrefName, PRUint32 aFlags)
{
  NS_ENSURE_ARG_POINTER(aPS);

  nsCOMPtr<nsIPref> prefs = do_GetService(NS_PREF_CONTRACTID);
  if (prefs) {
    nsMargin margin;
    if (aFlags & nsIPrintSettings::kInitSaveMargins) {
      if (NS_SUCCEEDED(aPS->GetMarginInTwips(margin))) {
        WriteInchesFromTwipsPref(prefs, GetPrefName(kMarginTop, aPrefName),    margin.top);
        DUMP_INT(kWriteStr, kMarginTop, margin.top);
        WriteInchesFromTwipsPref(prefs, GetPrefName(kMarginLeft, aPrefName),   margin.left);
        DUMP_INT(kWriteStr, kMarginLeft, margin.top);
        WriteInchesFromTwipsPref(prefs, GetPrefName(kMarginBottom, aPrefName), margin.bottom);
        DUMP_INT(kWriteStr, kMarginBottom, margin.top);
        WriteInchesFromTwipsPref(prefs, GetPrefName(kMarginRight, aPrefName),  margin.right);
        DUMP_INT(kWriteStr, kMarginRight, margin.top);
      }
    }

    PRBool     b;
    PRUnichar* uStr;
    PRInt32    iVal;
    PRInt16    iVal16;
    double     dbl;

    if (aFlags & nsIPrintSettings::kInitSaveOddEvenPages) {
      if (NS_SUCCEEDED(aPS->GetPrintOptions(nsIPrintSettings::kPrintEvenPages, &b))) {
        DUMP_BOOL(kWriteStr, kPrintEvenPages, b);
        prefs->SetBoolPref(GetPrefName(kPrintEvenPages, aPrefName), b);
      }
    }

    if (aFlags & nsIPrintSettings::kInitSaveOddEvenPages) {
      if (NS_SUCCEEDED(aPS->GetPrintOptions(nsIPrintSettings::kPrintOddPages, &b))) {
        DUMP_BOOL(kWriteStr, kPrintOddPages, b);
        prefs->SetBoolPref(GetPrefName(kPrintOddPages, aPrefName), b);
      }
    }

    if (aFlags & nsIPrintSettings::kInitSaveHeaderLeft) {
      if (NS_SUCCEEDED(aPS->GetHeaderStrLeft(&uStr))) {
        DUMP_STR(kWriteStr, kPrintHeaderStrLeft, uStr);
        WritePrefString(prefs, uStr, GetPrefName(kPrintHeaderStrLeft, aPrefName));
      }
    }

    if (aFlags & nsIPrintSettings::kInitSaveHeaderCenter) {
      if (NS_SUCCEEDED(aPS->GetHeaderStrCenter(&uStr))) {
        DUMP_STR(kWriteStr, kPrintHeaderStrCenter, uStr);
        WritePrefString(prefs, uStr, GetPrefName(kPrintHeaderStrCenter, aPrefName));
      }
    }

    if (aFlags & nsIPrintSettings::kInitSaveHeaderRight) {
      if (NS_SUCCEEDED(aPS->GetHeaderStrRight(&uStr))) {
        DUMP_STR(kWriteStr, kPrintHeaderStrRight, uStr);
        WritePrefString(prefs, uStr, GetPrefName(kPrintHeaderStrRight, aPrefName));
      }
    }

    if (aFlags & nsIPrintSettings::kInitSaveFooterLeft) {
      if (NS_SUCCEEDED(aPS->GetFooterStrLeft(&uStr))) {
        DUMP_STR(kWriteStr, kPrintFooterStrLeft, uStr);
        WritePrefString(prefs, uStr, GetPrefName(kPrintFooterStrLeft, aPrefName));
      }
    }

    if (aFlags & nsIPrintSettings::kInitSaveFooterCenter) {
      if (NS_SUCCEEDED(aPS->GetFooterStrCenter(&uStr))) {
        DUMP_STR(kWriteStr, kPrintFooterStrCenter, uStr);
        WritePrefString(prefs, uStr, GetPrefName(kPrintFooterStrCenter, aPrefName));
      }
    }

    if (aFlags & nsIPrintSettings::kInitSaveFooterRight) {
      if (NS_SUCCEEDED(aPS->GetFooterStrRight(&uStr))) {
        DUMP_STR(kWriteStr, kPrintFooterStrRight, uStr);
        WritePrefString(prefs, uStr, GetPrefName(kPrintFooterStrRight, aPrefName));
      }
    }

    if (aFlags & nsIPrintSettings::kInitSaveBGColors) {
      if (NS_SUCCEEDED(aPS->GetPrintBGColors(&b))) {
        DUMP_BOOL(kWriteStr, kPrintBGColors, b);
        prefs->SetBoolPref(GetPrefName(kPrintBGColors, aPrefName), b);
      }
    }

    if (aFlags & nsIPrintSettings::kInitSaveBGImages) {
      if (NS_SUCCEEDED(aPS->GetPrintBGImages(&b))) {
        DUMP_BOOL(kWriteStr, kPrintBGImages, b);
        prefs->SetBoolPref(GetPrefName(kPrintBGImages, aPrefName), b);
      }
    }

    if (aFlags & nsIPrintSettings::kInitSavePaperSize) {
      if (NS_SUCCEEDED(aPS->GetPaperSize(&iVal))) {
        DUMP_INT(kWriteStr, kPrintPaperSize, iVal);
        prefs->SetIntPref(GetPrefName(kPrintPaperSize, aPrefName), iVal); // this has been deprecated
      }
    }

    if (aFlags & nsIPrintSettings::kInitSaveReversed) {
      if (NS_SUCCEEDED(aPS->GetPrintReversed(&b))) {
        DUMP_BOOL(kWriteStr, kPrintReversed, b);
        prefs->SetBoolPref(GetPrefName(kPrintReversed, aPrefName), b);
      }
    }

    if (aFlags & nsIPrintSettings::kInitSaveInColor) {
      if (NS_SUCCEEDED(aPS->GetPrintInColor(&b))) {
        DUMP_BOOL(kWriteStr, kPrintInColor, b);
        prefs->SetBoolPref(GetPrefName(kPrintInColor, aPrefName), b);
      }
    }

    if (aFlags & nsIPrintSettings::kInitSavePaperName) {
      if (NS_SUCCEEDED(aPS->GetPaperName(&uStr))) {
        DUMP_STR(kWriteStr, kPrintPaperName, uStr);
        WritePrefString(prefs, uStr, GetPrefName(kPrintPaperName, aPrefName));
      }
    }

    if (aFlags & nsIPrintSettings::kInitSavePaperSizeUnit) {
      if (NS_SUCCEEDED(aPS->GetPaperSizeUnit(&iVal16))) {
        DUMP_INT(kWriteStr, kPrintPaperSizeUnit, iVal16);
        prefs->SetIntPref(GetPrefName(kPrintPaperSizeUnit, aPrefName),  PRInt32(iVal16));
      }
    }

    if (aFlags & nsIPrintSettings::kInitSavePaperSizeType) {
      if (NS_SUCCEEDED(aPS->GetPaperSizeType(&iVal16))) {
        DUMP_INT(kWriteStr, kPrintPaperSizeType, iVal16);
        prefs->SetIntPref(GetPrefName(kPrintPaperSizeType, aPrefName),  PRInt32(iVal16));
      }
    }

    if (aFlags & nsIPrintSettings::kInitSavePaperData) {
      if (NS_SUCCEEDED(aPS->GetPaperData(&iVal16))) {
        DUMP_INT(kWriteStr, kPrintPaperData, iVal16);
        prefs->SetIntPref(GetPrefName(kPrintPaperData, aPrefName),      PRInt32(iVal16));
      }
    }

    if (aFlags & nsIPrintSettings::kInitSavePaperWidth) {
      if (NS_SUCCEEDED(aPS->GetPaperWidth(&dbl))) {
        DUMP_DBL(kWriteStr, kPrintPaperWidth, dbl);
        WritePrefDouble(prefs, GetPrefName(kPrintPaperWidth, aPrefName), dbl);
      }
    }

    if (aFlags & nsIPrintSettings::kInitSavePaperHeight) {
      if (NS_SUCCEEDED(aPS->GetPaperHeight(&dbl))) {
        DUMP_DBL(kWriteStr, kPrintPaperHeight, dbl);
        WritePrefDouble(prefs, GetPrefName(kPrintPaperHeight, aPrefName), dbl);
      }
    }

    if (aFlags & nsIPrintSettings::kInitSaveOrientation) {
      if (NS_SUCCEEDED(aPS->GetOrientation(&iVal))) {
        DUMP_INT(kWriteStr, kPrintOrientation, iVal);
        prefs->SetIntPref(GetPrefName(kPrintOrientation, aPrefName),  iVal);
      }
    }

    if (aFlags & nsIPrintSettings::kInitSavePrintCommand) {
      if (NS_SUCCEEDED(aPS->GetPrintCommand(&uStr))) {
        DUMP_STR(kWriteStr, kPrintCommand, uStr);
        WritePrefString(prefs, uStr, GetPrefName(kPrintCommand, aPrefName));
      }
    }

    if (aFlags & nsIPrintSettings::kInitSavePrinterName) {
      if (NS_SUCCEEDED(aPS->GetPrinterName(&uStr))) {
        DUMP_STR(kWriteStr, kPrinterName, uStr);
        WritePrefString(prefs, uStr, GetPrefName(kPrinterName, aPrefName));
      }
    }

    if (aFlags & nsIPrintSettings::kInitSavePrintToFile) {
      if (NS_SUCCEEDED(aPS->GetPrintToFile(&b))) {
        DUMP_BOOL(kWriteStr, kPrintToFile, b);
        prefs->SetBoolPref(GetPrefName(kPrintToFile, aPrefName), b);
      }
    }

    if (aFlags & nsIPrintSettings::kInitSaveToFileName) {
      if (NS_SUCCEEDED(aPS->GetToFileName(&uStr))) {
        DUMP_STR(kWriteStr, kPrintToFileName, uStr);
        WritePrefString(prefs, uStr, GetPrefName(kPrintToFileName, aPrefName));
      }
    }

    if (aFlags & nsIPrintSettings::kInitSavePageDelay) {
      if (NS_SUCCEEDED(aPS->GetPrintPageDelay(&iVal))) {
        DUMP_INT(kWriteStr, kPrintPageDelay, iVal);
        prefs->SetIntPref(GetPrefName(kPrintPageDelay, aPrefName), iVal);
      }
    }

    // Not Writing Out:
    //   Scaling
    //   ShrinkToFit
    //   Number of Copies

    return NS_OK;
  }

  return NS_ERROR_FAILURE;
}

/* create and return a new |nsPrinterListEnumerator| */
NS_IMETHODIMP nsPrintOptions::AvailablePrinters(nsISimpleEnumerator **aPrinterEnumerator)
{
   NS_ENSURE_ARG_POINTER(aPrinterEnumerator);
   *aPrinterEnumerator = nsnull;

   nsCOMPtr<nsPrinterListEnumerator> printerListEnum = new nsPrinterListEnumerator();
   NS_ENSURE_TRUE(printerListEnum.get(), NS_ERROR_OUT_OF_MEMORY);

   // if Init fails NS_OK is return (script needs NS_OK) but the enumerator will be null
   nsresult rv = printerListEnum->Init();
   if (NS_SUCCEEDED(rv)) {
     *aPrinterEnumerator = NS_STATIC_CAST(nsISimpleEnumerator*, printerListEnum);
     NS_ADDREF(*aPrinterEnumerator);
   }
   return NS_OK;
}

NS_IMETHODIMP nsPrintOptions::DisplayJobProperties( const PRUnichar *aPrinter, nsIPrintSettings* aPrintSettings, PRBool *aDisplayed)
{
   NS_ENSURE_ARG(aPrinter);
   *aDisplayed = PR_FALSE;

   nsresult  rv;
   nsCOMPtr<nsIPrinterEnumerator> propDlg;

   propDlg = do_CreateInstance(kCPrinterEnumerator, &rv);
   if (NS_FAILED(rv))
     return rv;

   if (NS_FAILED(propDlg->DisplayPropertiesDlg((PRUnichar*)aPrinter, aPrintSettings)))
     return rv;
   
   *aDisplayed = PR_TRUE;
      
   return NS_OK;
}   

/* [noscript] voidPtr GetNativeData (in short aDataType); */
NS_IMETHODIMP nsPrintOptions::GetNativeData(PRInt16 aDataType, void * *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = nsnull;

  return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult nsPrintOptions::_CreatePrintSettings(nsIPrintSettings **_retval)
{
  nsresult rv = NS_OK;
  nsPrintSettings* printSettings = new nsPrintSettings(); // does not initially ref count
  NS_ASSERTION(printSettings, "Can't be NULL!");

  rv = printSettings->QueryInterface(NS_GET_IID(nsIPrintSettings), (void**)_retval); // ref counts
  if (NS_FAILED(rv)) return NS_ERROR_FAILURE;

  InitPrintSettingsFromPrefs(*_retval, PR_FALSE, nsIPrintSettings::kInitSaveAll); // ignore return value

  return rv;
}

/* nsIPrintSettings CreatePrintSettings (); */
NS_IMETHODIMP nsPrintOptions::CreatePrintSettings(nsIPrintSettings **_retval)
{
  return _CreatePrintSettings(_retval);
}

//-----------------------------------------------------
//-----------------------------------------------------
//-- nsIPrintSettingsService
//-----------------------------------------------------
//-----------------------------------------------------
/* readonly attribute nsIPrintSettings globalPrintSettings; */
NS_IMETHODIMP nsPrintOptions::GetGlobalPrintSettings(nsIPrintSettings * *aGlobalPrintSettings)
{
  if (!mGlobalPrintSettings) {
    CreatePrintSettings(getter_AddRefs(mGlobalPrintSettings));
    NS_ASSERTION(mGlobalPrintSettings, "Can't be NULL!");
  }

  // If this still NULL, we have some very big problems going on
  if (!mGlobalPrintSettings) {
    return NS_ERROR_FAILURE;
  }

  *aGlobalPrintSettings = mGlobalPrintSettings.get();
  NS_ADDREF(*aGlobalPrintSettings);

  return NS_OK;
}

/* readonly attribute nsIPrintSettings newPrintSettings; */
NS_IMETHODIMP
nsPrintOptions::GetNewPrintSettings(nsIPrintSettings * *aNewPrintSettings)
{
    NS_ENSURE_ARG_POINTER(aNewPrintSettings);
    return CreatePrintSettings(aNewPrintSettings);
}

//-----------------------------------------------------------------------
NS_IMETHODIMP
nsPrintOptions::GetDefaultPrinterName(PRUnichar * *aDefaultPrinterName)
{
    NS_ENSURE_ARG_POINTER(aDefaultPrinterName);

    nsresult rv;
    nsCOMPtr<nsIPrinterEnumerator> prtEnum = do_GetService(kPrinterEnumeratorCID, &rv);
    if (prtEnum) {
        rv = prtEnum->GetDefaultPrinterName(aDefaultPrinterName);
    }
    return rv;
}

//-----------------------------------------------------------------------
NS_IMETHODIMP
nsPrintOptions::InitPrintSettingsFromPrinter(const PRUnichar *aPrinterName, nsIPrintSettings *aPrintSettings)
{
    NS_ENSURE_ARG_POINTER(aPrintSettings);
    NS_ENSURE_ARG_POINTER(aPrinterName);

#ifdef NS_DEBUG
    nsXPIDLString printerName;
    aPrintSettings->GetPrinterName(getter_Copies(printerName));
    if (!printerName.Equals(aPrinterName)) {
      NS_WARNING("Printer names should match!");
    }
#endif

    PRBool isInitialized;
    aPrintSettings->GetIsInitializedFromPrinter(&isInitialized);
    if (!isInitialized) {
        nsresult rv = NS_ERROR_FAILURE;
        nsCOMPtr<nsIPrinterEnumerator> prtEnum = do_GetService(kPrinterEnumeratorCID, &rv);
        if (prtEnum) {
            rv = prtEnum->InitPrintSettingsFromPrinter(aPrinterName, aPrintSettings);
            if (NS_SUCCEEDED(rv)) {
                aPrintSettings->SetIsInitializedFromPrinter(PR_TRUE);
            }
        }
        return rv;
    }

    return NS_OK;
}

/** ---------------------------------------------------
 *  Helper function - Returns either the name or sets the length to zero
 */
static void GetAdjustedPrinterName(nsIPrintSettings* aPS, PRBool aUsePNP, nsString& aPrinterName)
{
  aPrinterName.SetLength(0);

  // Get the Printer Name from the PrintSettings 
  // to use as a prefix for Pref Names
  PRUnichar* prtName = nsnull;
  if (aUsePNP && NS_SUCCEEDED(aPS->GetPrinterName(&prtName))) {
    if (prtName && !*prtName) {
      nsMemory::Free(prtName);
      prtName = nsnull;
    }
  }

  if (prtName) {
    aPrinterName = prtName;
    PRUnichar uc = '_';
    const char* replaceStr = " \n\r";
    for (PRInt32 i=0;i<(PRInt32)strlen(replaceStr);i++) {
      PRUnichar uChar = replaceStr[i];
      aPrinterName.ReplaceChar(uChar, uc);
    }
  }
}

/* PRInt32 getPrinterPrefInt (in nsIPrintSettings aPrintSettings, in wstring aPrefName); */
NS_IMETHODIMP nsPrintOptions::GetPrinterPrefInt(nsIPrintSettings *aPrintSettings, const PRUnichar *aPrefName, PRInt32 *_retval)
{
  nsString prtName;
  // Get the Printer Name from the PtinerSettings 
  // to use as a prefix for Pref Names
  GetAdjustedPrinterName(aPrintSettings, PR_TRUE, prtName);

  nsCOMPtr<nsIPref> prefs = do_GetService(NS_PREF_CONTRACTID);
  if (prefs) {
    PRInt32 iVal;
    if (NS_SUCCEEDED(prefs->GetIntPref(GetPrefName(NS_LossyConvertUCS2toASCII(aPrefName).get(), prtName), &iVal))) {
      *_retval = iVal;
      return NS_OK;
    }
  }
  return NS_ERROR_FAILURE;
}

/** ---------------------------------------------------
 *  See documentation in nsPrintOptionsImpl.h
 *	@update 1/12/01 rods
 */
NS_IMETHODIMP 
nsPrintOptions::InitPrintSettingsFromPrefs(nsIPrintSettings* aPS, PRBool aUsePNP, PRUint32 aFlags)
{
  NS_ENSURE_ARG_POINTER(aPS);

  PRBool isInitialized;
  aPS->GetIsInitializedFromPrefs(&isInitialized);
  if (!isInitialized) {
    nsString prtName;
    // read any non printer specific prefs
    // with empty printer name
    nsresult rv = ReadPrefs(aPS, prtName, aFlags);
    NS_ENSURE_SUCCESS(rv, rv);

    // Get the Printer Name from the PrintSettings 
    // to use as a prefix for Pref Names
    GetAdjustedPrinterName(aPS, aUsePNP, prtName);
    if (prtName.Length()) {
      // Now read any printer specific prefs
      rv = ReadPrefs(aPS, prtName, aFlags);
      if (NS_SUCCEEDED(rv)) {
        aPS->SetIsInitializedFromPrefs(PR_TRUE);
      }
    }
  }
  return NS_OK;
}

/** ---------------------------------------------------
 *  This will asve into prefs most all the PrintSettings either generically (not specified printer)
 *  or to a specific printer.
 */
nsresult 
nsPrintOptions::SavePrintSettingsToPrefs(nsIPrintSettings *aPS, PRBool aUsePrinterNamePrefix, PRUint32 aFlags)
{
  nsString prtName;
  // Get the Printer Name from the PtinerSettings 
  // to use as a prefix for Pref Names
  GetAdjustedPrinterName(aPS, aUsePrinterNamePrefix, prtName);

  // Now write any printer specific prefs
  return WritePrefs(aPS, prtName, aFlags);
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

/** ---------------------------------------------------
 *  Write PRUnichar* to Prefs and deletes the contents of the string
 */
nsresult nsPrintOptions::WritePrefString(nsIPref* aPref, PRUnichar*& aStr, const char* aPrefId)
{
  if (!aStr) return NS_ERROR_FAILURE;

  nsresult rv = NS_ERROR_FAILURE;
  if (aStr) {
    rv = aPref->SetUnicharPref(aPrefId, aStr);
    nsMemory::Free(aStr);
    aStr = nsnull;
  }
  return rv;
}

nsresult nsPrintOptions::WritePrefString(nsIPref *    aPref, 
                                         const char * aPrefId, 
                                         nsString&    aString)
{
  NS_ENSURE_ARG_POINTER(aPref);
  NS_ENSURE_ARG_POINTER(aPrefId);

  PRUnichar * str = ToNewUnicode(aString);
  nsresult rv = aPref->SetUnicharPref(aPrefId, str);
  nsMemory::Free(str);

  return rv;
}

nsresult nsPrintOptions::ReadPrefDouble(nsIPref *    aPref, 
                                        const char * aPrefId, 
                                        double&      aVal)
{
  char * str = nsnull;
  nsresult rv = aPref->CopyCharPref(aPrefId, &str);
  if (NS_SUCCEEDED(rv) && str) {
    float f;
    sscanf(str, "%f", &f);
    aVal = double(f);
    nsMemory::Free(str);
  }
  return rv;
}

nsresult nsPrintOptions::WritePrefDouble(nsIPref *    aPref, 
                                         const char * aPrefId, 
                                         double       aVal)
{
  NS_ENSURE_ARG_POINTER(aPref);
  NS_ENSURE_ARG_POINTER(aPrefId);

  char str[64];
  sprintf(str, "%6.2f", aVal);
  return aPref->SetCharPref(aPrefId, str);
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
  char * str = ToNewCString(inchesStr);
  if (str) {
    aPref->SetCharPref(aPrefId, str);
  } else {
    aPref->SetCharPref(aPrefId, "0.5");
  }
}

//---------------------------------------------------
void nsPrintOptions::ReadJustification(nsIPref *    aPref, 
                                       const char * aPrefId, 
                                       PRInt16&     aJust,
                                       PRInt16      aInitValue)
{
  aJust = aInitValue;
  nsAutoString justStr;
  if (NS_SUCCEEDED(ReadPrefString(aPref, aPrefId, justStr))) {
    if (justStr.EqualsWithConversion(kJustRight)) {
      aJust = nsIPrintSettings::kJustRight;

    } else if (justStr.EqualsWithConversion(kJustCenter)) {
      aJust = nsIPrintSettings::kJustCenter;

    } else {
      aJust = nsIPrintSettings::kJustLeft;
    }
  }
}

//---------------------------------------------------
void nsPrintOptions::WriteJustification(nsIPref *    aPref, 
                                        const char * aPrefId, 
                                        PRInt16      aJust)
{
  switch (aJust) {
    case nsIPrintSettings::kJustLeft: 
      aPref->SetCharPref(aPrefId, kJustLeft);
      break;

    case nsIPrintSettings::kJustCenter: 
      aPref->SetCharPref(aPrefId, kJustCenter);
      break;

    case nsIPrintSettings::kJustRight: 
      aPref->SetCharPref(aPrefId, kJustRight);
      break;
  } //switch
}

//----------------------------------------------------------------------
// Testing of read/write prefs
// This define turns on the testing module below
// so at start up it writes and reads the prefs.
#ifdef DEBUG_rods_X
class Tester {
public:
  Tester();
};
Tester::Tester()
{
  nsCOMPtr<nsIPrintSettings> ps;
  nsresult rv;
  nsCOMPtr<nsIPrintOptions> printService = do_GetService("@mozilla.org/gfx/printsettings-service;1", &rv);
  if (NS_SUCCEEDED(rv)) {
    rv = printService->CreatePrintSettings(getter_AddRefs(ps));
  }

  if (ps) {
    ps->SetPrintOptions(nsIPrintSettings::kPrintOddPages,  PR_TRUE);
    ps->SetPrintOptions(nsIPrintSettings::kPrintEvenPages,  PR_FALSE);
    ps->SetMarginTop(1.0);
    ps->SetMarginLeft(1.0);
    ps->SetMarginBottom(1.0);
    ps->SetMarginRight(1.0);
    ps->SetScaling(0.5);
    ps->SetPrintBGColors(PR_TRUE);
    ps->SetPrintBGImages(PR_TRUE);
    ps->SetPrintRange(15);
    ps->SetHeaderStrLeft(NS_ConvertUTF8toUCS2("Left").get());
    ps->SetHeaderStrCenter(NS_ConvertUTF8toUCS2("Center").get());
    ps->SetHeaderStrRight(NS_ConvertUTF8toUCS2("Right").get());
    ps->SetFooterStrLeft(NS_ConvertUTF8toUCS2("Left").get());
    ps->SetFooterStrCenter(NS_ConvertUTF8toUCS2("Center").get());
    ps->SetFooterStrRight(NS_ConvertUTF8toUCS2("Right").get());
    ps->SetPaperName(NS_ConvertUTF8toUCS2("Paper Name").get());
    ps->SetPaperSizeType(10);
    ps->SetPaperData(1);
    ps->SetPaperWidth(100.0);
    ps->SetPaperHeight(50.0);
    ps->SetPaperSizeUnit(nsIPrintSettings::kPaperSizeMillimeters);
    ps->SetPrintReversed(PR_TRUE);
    ps->SetPrintInColor(PR_TRUE);
    ps->SetPaperSize(5);
    ps->SetOrientation(nsIPrintSettings::kLandscapeOrientation);
    ps->SetPrintCommand(NS_ConvertUTF8toUCS2("Command").get());
    ps->SetNumCopies(2);
    ps->SetPrinterName(NS_ConvertUTF8toUCS2("Printer Name").get());
    ps->SetPrintToFile(PR_TRUE);
    ps->SetToFileName(NS_ConvertUTF8toUCS2("File Name").get());
    ps->SetPrintPageDelay(1000);

    struct SettingsType {
      const char* mName;
      PRUint32    mFlag;
    };
    SettingsType gSettings[] = {
      {"OddEven", nsIPrintSettings::kInitSaveOddEvenPages},
      {kPrintHeaderStrLeft, nsIPrintSettings::kInitSaveHeaderLeft},
      {kPrintHeaderStrCenter, nsIPrintSettings::kInitSaveHeaderCenter},
      {kPrintHeaderStrRight, nsIPrintSettings::kInitSaveHeaderRight},
      {kPrintFooterStrLeft, nsIPrintSettings::kInitSaveFooterLeft},
      {kPrintFooterStrCenter, nsIPrintSettings::kInitSaveFooterCenter},
      {kPrintFooterStrRight, nsIPrintSettings::kInitSaveFooterRight},
      {kPrintBGColors, nsIPrintSettings::kInitSaveBGColors},
      {kPrintBGImages, nsIPrintSettings::kInitSaveBGImages},
      {kPrintPaperSize, nsIPrintSettings::kInitSavePaperSize},
      {kPrintPaperName, nsIPrintSettings::kInitSavePaperName},
      {kPrintPaperSizeUnit, nsIPrintSettings::kInitSavePaperSizeUnit},
      {kPrintPaperSizeType, nsIPrintSettings::kInitSavePaperSizeType},
      {kPrintPaperData, nsIPrintSettings::kInitSavePaperData},
      {kPrintPaperWidth, nsIPrintSettings::kInitSavePaperWidth},
      {kPrintPaperHeight, nsIPrintSettings::kInitSavePaperHeight},
      {kPrintReversed, nsIPrintSettings::kInitSaveReversed},
      {kPrintInColor, nsIPrintSettings::kInitSaveInColor},
      {kPrintOrientation, nsIPrintSettings::kInitSaveOrientation},
      {kPrintCommand, nsIPrintSettings::kInitSavePrintCommand},
      {kPrinterName, nsIPrintSettings::kInitSavePrinterName},
      {kPrintToFile, nsIPrintSettings::kInitSavePrintToFile},
      {kPrintToFileName, nsIPrintSettings::kInitSaveToFileName},
      {kPrintPageDelay, nsIPrintSettings::kInitSavePageDelay},
      {"Margins", nsIPrintSettings::kInitSaveMargins},
      {"All", nsIPrintSettings::kInitSaveAll},
      {nsnull, 0}};

    nsString prefix; prefix.AssignWithConversion("Printer Name");
    PRInt32 i = 0;
    while (gSettings[i].mName != nsnull) {
      printf("------------------------------------------------\n");
      printf("%d) %s -> 0x%X\n", i, gSettings[i].mName, gSettings[i].mFlag);
      printService->SavePrintSettingsToPrefs(ps, PR_TRUE, gSettings[i].mFlag);
      printService->InitPrintSettingsFromPrefs(ps, PR_TRUE, gSettings[i].mFlag);
      i++;
    }
  }

}
Tester gTester;
#endif
