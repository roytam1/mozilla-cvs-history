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

#include <locale.h>
#include "plstr.h"
#include "nsIServiceManager.h"
#include "nsICharsetConverterManager.h"
#include "nsDateTimeFormatUnix.h"
#include "nsIComponentManager.h"
#include "nsLocaleCID.h"
#include "nsILocaleService.h"
#include "nsIPlatformCharset.h"
#include "nsIPosixLocale.h"
#include "nsCOMPtr.h"
#include "nsCRT.h"

static NS_DEFINE_IID(kIDateTimeFormatIID, NS_IDATETIMEFORMAT_IID);
static NS_DEFINE_CID(kPosixLocaleFactoryCID, NS_POSIXLOCALEFACTORY_CID);
static NS_DEFINE_CID(kCharsetConverterManagerCID, NS_ICHARSETCONVERTERMANAGER_CID);
static NS_DEFINE_IID(kICharsetConverterManagerIID, NS_ICHARSETCONVERTERMANAGER_IID);
static NS_DEFINE_CID(kLocaleServiceCID, NS_LOCALESERVICE_CID); 

NS_IMPL_ISUPPORTS(nsDateTimeFormatUnix, kIDateTimeFormatIID);

// init this interface to a specified locale
nsresult nsDateTimeFormatUnix::Initialize(nsILocale* locale)
{
  PRUnichar *aLocaleUnichar = NULL;
  nsString aCategory("NSILOCALE_TIME");
  nsresult res = NS_OK;

  // use cached info if match with stored locale
  if (NULL == locale) {
    if (mLocale.Length() && mLocale.EqualsIgnoreCase(mAppLocale)) {
      return NS_OK;
    }
  }
  else {
    res = locale->GetCategory(aCategory.GetUnicode(), &aLocaleUnichar);
    if (NS_SUCCEEDED(res) && NULL != aLocaleUnichar) {
      if (mLocale.Length() && mLocale.EqualsIgnoreCase(aLocaleUnichar)) {
        nsAllocator::Free(aLocaleUnichar);
        return NS_OK;
      }
      nsAllocator::Free(aLocaleUnichar);
    }
  }

  mCharset.SetString("ISO-8859-1");
  PL_strncpy(mPlatformLocale, "en_US", kPlatformLocaleLength+1);

  // get locale name string, use app default if no locale specified
  if (NULL == locale) {
    nsILocaleService *localeService;

    res = nsComponentManager::CreateInstance(kLocaleServiceCID, NULL, 
                                             nsILocaleService::GetIID(), (void**)&localeService);
    if (NS_SUCCEEDED(res)) {
      nsILocale *appLocale;
      res = localeService->GetApplicationLocale(&appLocale);
	    localeService->Release();
      if (NS_SUCCEEDED(res)) {
        res = appLocale->GetCategory(aCategory.GetUnicode(), &aLocaleUnichar);
        if (NS_SUCCEEDED(res) && NULL != aLocaleUnichar) {
          mAppLocale.SetString(aLocaleUnichar); // cache app locale name
        }
        appLocale->Release();
      }
    }
  }
  else {
    res = locale->GetCategory(aCategory.GetUnicode(), &aLocaleUnichar);
  }

  if (NS_SUCCEEDED(res) && NULL != aLocaleUnichar) {
    mLocale.SetString(aLocaleUnichar); // cache locale name
    nsAllocator::Free(aLocaleUnichar);

    nsCOMPtr <nsIPosixLocale> posixLocale;
    res = nsComponentManager::CreateInstance(kPosixLocaleFactoryCID, NULL, 
                                             nsIPosixLocale::GetIID(), getter_AddRefs(posixLocale));
    if (NS_SUCCEEDED(res)) {
      res = posixLocale->GetPlatformLocale(&mLocale, mPlatformLocale, kPlatformLocaleLength+1);
    }

    nsCOMPtr <nsIPlatformCharset> platformCharset;
    res = nsComponentManager::CreateInstance(kPlatformCharsetCID, NULL, 
                                             nsIPlatformCharset::GetIID(), getter_AddRefs(platformCharset));
    if (NS_SUCCEEDED(res)) {
      PRUnichar* mappedCharset = NULL;
      res = platformCharset->GetDefaultCharsetForLocale(mLocale.GetUnicode(), &mappedCharset);
      if (NS_SUCCEEDED(res) && mappedCharset) {
        mCharset.SetString(mappedCharset);
        nsAllocator::Free(mappedCharset);
      }
    }
  }

  return res;
}

nsresult nsDateTimeFormatUnix::FormatTime(nsILocale* locale, 
                                      const nsDateFormatSelector  dateFormatSelector, 
                                      const nsTimeFormatSelector timeFormatSelector, 
                                      const time_t  timetTime, 
                                      nsString& stringOut) 
{
  return FormatTMTime(locale, dateFormatSelector, timeFormatSelector, localtime(&timetTime), stringOut);
}

// performs a locale sensitive date formatting operation on the struct tm parameter
nsresult nsDateTimeFormatUnix::FormatTMTime(nsILocale* locale, 
                                        const nsDateFormatSelector  dateFormatSelector, 
                                        const nsTimeFormatSelector timeFormatSelector, 
                                        const struct tm*  tmTime, 
                                        nsString& stringOut) 
{
#define NSDATETIME_FORMAT_BUFFER_LEN  80
  char strOut[NSDATETIME_FORMAT_BUFFER_LEN];
  char fmtD[NSDATETIME_FORMAT_BUFFER_LEN], fmtT[NSDATETIME_FORMAT_BUFFER_LEN];
  nsresult res;

  
  // set up locale data
  (void) Initialize(locale);

  // set date format
  switch (dateFormatSelector) {
    case kDateFormatNone:
      PL_strncpy(fmtD, "", NSDATETIME_FORMAT_BUFFER_LEN);
      break; 
    case kDateFormatLong:
      PL_strncpy(fmtD, "%c", NSDATETIME_FORMAT_BUFFER_LEN);
      break; 
    case kDateFormatShort:
      PL_strncpy(fmtD, "%x", NSDATETIME_FORMAT_BUFFER_LEN);
      break; 
    case kDateFormatYearMonth:
      PL_strncpy(fmtD, "%y/%m", NSDATETIME_FORMAT_BUFFER_LEN);
      break; 
    case kDateFormatWeekday:
      PL_strncpy(fmtD, "%a", NSDATETIME_FORMAT_BUFFER_LEN);
      break;
    default:
      PL_strncpy(fmtD, "", NSDATETIME_FORMAT_BUFFER_LEN); 
  }

  // set time format
  switch (timeFormatSelector) {
    case kTimeFormatNone:
      PL_strncpy(fmtT, "", NSDATETIME_FORMAT_BUFFER_LEN); 
      break;
    case kTimeFormatSeconds:
      PL_strncpy(fmtT, "%I:%M:%S %p", NSDATETIME_FORMAT_BUFFER_LEN);
      break;
    case kTimeFormatNoSeconds:
      PL_strncpy(fmtT, "%I:%M %p", NSDATETIME_FORMAT_BUFFER_LEN);
      break;
    case kTimeFormatSecondsForce24Hour:
      PL_strncpy(fmtT, "%H:%M:%S", NSDATETIME_FORMAT_BUFFER_LEN);
      break;
    case kTimeFormatNoSecondsForce24Hour:
      PL_strncpy(fmtT, "%H:%M", NSDATETIME_FORMAT_BUFFER_LEN);
      break;
    default:
      PL_strncpy(fmtT, "", NSDATETIME_FORMAT_BUFFER_LEN); 
  }

  // generate data/time string
  char *old_locale = setlocale(LC_TIME, "");
  (void) setlocale(LC_TIME, mPlatformLocale);
  if (PL_strlen(fmtD) && PL_strlen(fmtT)) {
    PL_strncat(fmtD, " ", NSDATETIME_FORMAT_BUFFER_LEN);
    PL_strncat(fmtD, fmtT, NSDATETIME_FORMAT_BUFFER_LEN);
    strftime(strOut, NSDATETIME_FORMAT_BUFFER_LEN, fmtD, tmTime);
  }
  else if (PL_strlen(fmtD) && !PL_strlen(fmtT)) {
    strftime(strOut, NSDATETIME_FORMAT_BUFFER_LEN, fmtD, tmTime);
  }
  else if (!PL_strlen(fmtD) && PL_strlen(fmtT)) {
    strftime(strOut, NSDATETIME_FORMAT_BUFFER_LEN, fmtT, tmTime);
  }
  else {
    PL_strncpy(strOut, "", NSDATETIME_FORMAT_BUFFER_LEN);
  }
  (void) setlocale(LC_TIME, old_locale);

  // convert result to unicode
  NS_WITH_SERVICE(nsICharsetConverterManager, ccm, kCharsetConverterManagerCID, &res);
  if(NS_SUCCEEDED(res) && ccm) {
    nsCOMPtr<nsIUnicodeDecoder> decoder;
    res = ccm->GetUnicodeDecoder(&mCharset, getter_AddRefs(decoder));
    if (NS_SUCCEEDED(res) && decoder) {
      PRInt32 unicharLength = 0;
      PRInt32 srcLength = (PRInt32) PL_strlen(strOut);
      res = decoder->GetMaxLength(strOut, srcLength, &unicharLength);
      PRUnichar *unichars = new PRUnichar [ unicharLength ];
  
      if (nsnull != unichars) {
        res = decoder->Convert(strOut, &srcLength,
                               unichars, &unicharLength);
        if (NS_SUCCEEDED(res)) {
          stringOut.SetString(unichars, unicharLength);
        }
      }
      delete [] unichars;
    }    
  }
  
  return res;
}

// performs a locale sensitive date formatting operation on the PRTime parameter
nsresult nsDateTimeFormatUnix::FormatPRTime(nsILocale* locale, 
                                           const nsDateFormatSelector  dateFormatSelector, 
                                           const nsTimeFormatSelector timeFormatSelector, 
                                           const PRTime  prTime, 
                                           nsString& stringOut)
{
  PRExplodedTime explodedTime;
  PR_ExplodeTime(prTime, PR_LocalTimeParameters, &explodedTime);

  return FormatPRExplodedTime(locale, dateFormatSelector, timeFormatSelector, &explodedTime, stringOut);
}

// performs a locale sensitive date formatting operation on the PRExplodedTime parameter
nsresult nsDateTimeFormatUnix::FormatPRExplodedTime(nsILocale* locale, 
                                                   const nsDateFormatSelector  dateFormatSelector, 
                                                   const nsTimeFormatSelector timeFormatSelector, 
                                                   const PRExplodedTime*  explodedTime, 
                                                   nsString& stringOut)
{
  struct tm  tmTime;
  /* be safe and set all members of struct tm to zero
   *
   * there are other fields in the tm struct that we aren't setting
   * (tm_isdst, tm_gmtoff, tm_zone, should we set these?) and since
   * tmTime is on the stack, it may be filled with garbage, but
   * the garbage may vary.  (this may explain why some saw bug #10412, and
   * others did not.
   *
   * when tmTime is passed to strftime() with garbage bad things may happen. 
   * see bug #10412
   */
  nsCRT::memset( &tmTime, 0, sizeof(tmTime) );

  tmTime.tm_yday = explodedTime->tm_yday;
  tmTime.tm_wday = explodedTime->tm_wday;
  tmTime.tm_year = explodedTime->tm_year;
  tmTime.tm_year -= 1900;
  tmTime.tm_mon = explodedTime->tm_month;
  tmTime.tm_mday = explodedTime->tm_mday;
  tmTime.tm_hour = explodedTime->tm_hour;
  tmTime.tm_min = explodedTime->tm_min;
  tmTime.tm_sec = explodedTime->tm_sec;

  return FormatTMTime(locale, dateFormatSelector, timeFormatSelector, &tmTime, stringOut);
}

