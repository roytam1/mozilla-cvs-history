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
 *   Pierre Phaneuf <pp@ludusdesign.com>
 */

#include "nsDateTimeFormatWin.h"
#include "nsIServiceManager.h"
#include "nsIComponentManager.h"
#include "nsICharsetConverterManager.h"
#include "nsLocaleCID.h"
#include "nsILocaleService.h"
#include "nsIPlatformCharset.h"
#include "nsIWin32Locale.h"
#include "nsCRT.h"
#include "nsCOMPtr.h"
#include "prmem.h"


#define NSDATETIMEFORMAT_BUFFER_LEN  80

static NS_DEFINE_CID(kWin32LocaleFactoryCID, NS_WIN32LOCALEFACTORY_CID);
static NS_DEFINE_IID(kIWin32LocaleIID, NS_IWIN32LOCALE_IID);
static NS_DEFINE_IID(kIDateTimeFormatIID, NS_IDATETIMEFORMAT_IID);

static NS_DEFINE_CID(kCharsetConverterManagerCID, NS_ICHARSETCONVERTERMANAGER_CID);
static NS_DEFINE_IID(kICharsetConverterManagerIID, NS_ICHARSETCONVERTERMANAGER_IID);

static NS_DEFINE_CID(kLocaleServiceCID, NS_LOCALESERVICE_CID); 
static NS_DEFINE_CID(kPlatformCharsetCID, NS_PLATFORMCHARSET_CID);

NS_IMPL_THREADSAFE_ISUPPORTS(nsDateTimeFormatWin, kIDateTimeFormatIID);


// init this interface to a specified locale
nsresult nsDateTimeFormatWin::Initialize(nsILocale* locale)
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

  // get os version
  OSVERSIONINFO os;
  os.dwOSVersionInfoSize = sizeof(os);
  ::GetVersionEx(&os);
  if (VER_PLATFORM_WIN32_NT == os.dwPlatformId &&
      os.dwMajorVersion >= 4) {
    mW_API = PR_TRUE;   // has W API
  }
  else {
    mW_API = PR_FALSE;
  }

  // default charset name
  mCharset.Assign("ISO-8859-1");
  
  // default LCID (en-US)
  mLCID = 1033;

  // get locale string, use app default if no locale specified
  if (NULL == locale) {
    NS_WITH_SERVICE(nsILocaleService, localeService, kLocaleServiceCID, &res);
    if (NS_SUCCEEDED(res)) {
      nsILocale *appLocale;
      res = localeService->GetApplicationLocale(&appLocale);
      if (NS_SUCCEEDED(res)) {
        res = appLocale->GetCategory(aCategory.GetUnicode(), &aLocaleUnichar);
        if (NS_SUCCEEDED(res) && NULL != aLocaleUnichar) {
          mAppLocale.Assign(aLocaleUnichar); // cache app locale name
        }
        appLocale->Release();
      }
    }
  }
  else {
    res = locale->GetCategory(aCategory.GetUnicode(), &aLocaleUnichar);
  }

  // Get LCID and charset name from locale, if available
  if (NS_SUCCEEDED(res) && NULL != aLocaleUnichar) {
    mLocale.Assign(aLocaleUnichar); // cache locale name
    nsAllocator::Free(aLocaleUnichar);

    nsCOMPtr <nsIWin32Locale> win32Locale;
    res = nsComponentManager::CreateInstance(kWin32LocaleFactoryCID, NULL, kIWin32LocaleIID, getter_AddRefs(win32Locale));
    if (NS_SUCCEEDED(res)) {
  	  res = win32Locale->GetPlatformLocale(&mLocale, (LCID *) &mLCID);
    }

    nsCOMPtr <nsIPlatformCharset> platformCharset;
    res = nsComponentManager::CreateInstance(kPlatformCharsetCID, NULL, 
                                             NS_GET_IID(nsIPlatformCharset), getter_AddRefs(platformCharset));
    if (NS_SUCCEEDED(res)) {
      PRUnichar* mappedCharset = NULL;
      res = platformCharset->GetDefaultCharsetForLocale(mLocale.GetUnicode(), &mappedCharset);
      if (NS_SUCCEEDED(res) && mappedCharset) {
        mCharset.Assign(mappedCharset);
        nsAllocator::Free(mappedCharset);
      }
    }
  }

  return res;
}

// performs a locale sensitive date formatting operation on the time_t parameter
nsresult nsDateTimeFormatWin::FormatTime(nsILocale* locale, 
                                         const nsDateFormatSelector  dateFormatSelector, 
                                         const nsTimeFormatSelector timeFormatSelector, 
                                         const time_t  timetTime, 
                                         nsString& stringOut)
{
  return FormatTMTime(locale, dateFormatSelector, timeFormatSelector, localtime( &timetTime ), stringOut);
}

// performs a locale sensitive date formatting operation on the struct tm parameter
nsresult nsDateTimeFormatWin::FormatTMTime(nsILocale* locale, 
                                           const nsDateFormatSelector  dateFormatSelector, 
                                           const nsTimeFormatSelector timeFormatSelector, 
                                           const struct tm*  tmTime, 
                                           nsString& stringOut)
{
	SYSTEMTIME system_time;
  DWORD dwFlags_Date = 0, dwFlags_Time = 0;
  int dateLen, timeLen;
  PRUnichar dateBuffer[NSDATETIMEFORMAT_BUFFER_LEN], timeBuffer[NSDATETIMEFORMAT_BUFFER_LEN];

  // set up locale data
  (void) Initialize(locale);

  // Map tm to SYSTEMTIME
	system_time.wYear = 1900 + tmTime->tm_year;
	system_time.wMonth = tmTime->tm_mon + 1;
	system_time.wDayOfWeek = tmTime->tm_wday;
	system_time.wDay = tmTime->tm_mday;
	system_time.wHour = tmTime->tm_hour;
	system_time.wMinute = tmTime->tm_min;
	system_time.wSecond = tmTime->tm_sec;
	system_time.wMilliseconds = 0;

  // Map to WinAPI date format
  switch (dateFormatSelector) {
  case kDateFormatLong:
    dwFlags_Date = DATE_LONGDATE;
    break;
  case kDateFormatShort:
    dwFlags_Date = DATE_SHORTDATE;
    break;
  case kDateFormatWeekday:
    dwFlags_Date = 0;
    break;
  case kDateFormatYearMonth:
    dwFlags_Date = 0;     // TODO:only availabe NT5
    break;
  }

  // Map to WinAPI time format
  switch (timeFormatSelector) {
  case kTimeFormatSeconds:
    dwFlags_Time = 0;
    break;
  case kTimeFormatNoSeconds:
    dwFlags_Time = TIME_NOSECONDS;
    break;
  case kTimeFormatSecondsForce24Hour:
    dwFlags_Time = TIME_FORCE24HOURFORMAT;
    break;
  case kTimeFormatNoSecondsForce24Hour:
    dwFlags_Time = TIME_NOSECONDS + TIME_FORCE24HOURFORMAT;
    break;
  }

  // Call GetDateFormatW
  if (dateFormatSelector == kDateFormatNone) {
    dateLen = 0;
  }
  else {
    if (dateFormatSelector == kDateFormatYearMonth) {
      dateLen = nsGetDateFormatW(0, &system_time, "yyyy/MM", 
                                 dateBuffer, NSDATETIMEFORMAT_BUFFER_LEN);
    }
    else if (dateFormatSelector == kDateFormatWeekday) {
      dateLen = nsGetDateFormatW(0, &system_time, "ddd", 
                                 dateBuffer, NSDATETIMEFORMAT_BUFFER_LEN);
    }
    else {
      dateLen = nsGetDateFormatW(dwFlags_Date, &system_time, NULL, 
                                 dateBuffer, NSDATETIMEFORMAT_BUFFER_LEN);
    }
    if (dateLen != 0) {
      dateLen--;  // Since the count includes the terminating null.
    }
  }

  // Call GetTimeFormatW
  if (timeFormatSelector == kTimeFormatNone) {
    timeLen = 0;
  }
  else {
    timeLen = nsGetTimeFormatW(dwFlags_Time, &system_time, NULL, 
                               timeBuffer, NSDATETIMEFORMAT_BUFFER_LEN);
    if (timeLen != 0) {
      timeLen--;  // Since the count includes the terminating null.
    }
  }

  NS_ASSERTION(NSDATETIMEFORMAT_BUFFER_LEN >= (PRUint32) (dateLen + 1), "internal date buffer is not large enough");
  NS_ASSERTION(NSDATETIMEFORMAT_BUFFER_LEN >= (PRUint32) (timeLen + 1), "internal time buffer is not large enough");

  // Copy the result
  stringOut.Assign("");
  if (dateLen != 0 && timeLen != 0) {
    stringOut.Assign(dateBuffer, dateLen);
    stringOut.Append((PRUnichar *)(L" "), 1);
    stringOut.Append(timeBuffer, timeLen);
  }
  else if (dateLen != 0 && timeLen == 0) {
    stringOut.Assign(dateBuffer, dateLen);
  }
  else if (dateLen == 0 && timeLen != 0) {
    stringOut.Assign(timeBuffer, timeLen);
  }

  return NS_OK;
}

// performs a locale sensitive date formatting operation on the PRTime parameter
nsresult nsDateTimeFormatWin::FormatPRTime(nsILocale* locale, 
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
nsresult nsDateTimeFormatWin::FormatPRExplodedTime(nsILocale* locale, 
                                                   const nsDateFormatSelector  dateFormatSelector, 
                                                   const nsTimeFormatSelector timeFormatSelector, 
                                                   const PRExplodedTime*  explodedTime, 
                                                   nsString& stringOut)
{
  struct tm  tmTime;
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

nsresult nsDateTimeFormatWin::ConvertToUnicode(const char *inString, const PRInt32 inLen, PRUnichar *outString, PRInt32 *outLen)
{
  nsresult res;
  // convert result to unicode
  NS_WITH_SERVICE(nsICharsetConverterManager, ccm, kCharsetConverterManagerCID, &res);

  *outLen = 0;
  if(NS_SUCCEEDED(res) && ccm) {
    nsCOMPtr <nsIUnicodeDecoder> decoder;
    res = ccm->GetUnicodeDecoder(&mCharset, getter_AddRefs(decoder));
    if(NS_SUCCEEDED(res) && decoder) {
      PRInt32 unicharLength = 0;
      PRInt32 srcLength = inLen;
      res = decoder->GetMaxLength(inString, srcLength, &unicharLength);
      PRUnichar *unichars = outString;

      if (nsnull != unichars) {
        res = decoder->Convert(inString, &srcLength,
                               unichars, &unicharLength);
        if (NS_SUCCEEDED(res)) {
          *outLen = unicharLength;
        }
      }
    }    
  }

  return res;
}

int nsDateTimeFormatWin::nsGetTimeFormatW(DWORD dwFlags, const SYSTEMTIME *lpTime,
                                          const char* format, PRUnichar *timeStr, int cchTime)
{
  int len = 0;
  nsresult res = NS_OK;

  if (mW_API) {
    nsString formatString(format ? format : "");
    LPCWSTR wstr = format ? (LPCWSTR) formatString.GetUnicode() : NULL;
    len = GetTimeFormatW(mLCID, dwFlags, lpTime, wstr, (LPWSTR) timeStr, cchTime);
  }
  else {
    char *cstr_time;
    cstr_time = new char[NSDATETIMEFORMAT_BUFFER_LEN];
    if (nsnull == cstr_time) {
      return 0;
    }
    len = GetTimeFormatA(mLCID, dwFlags, lpTime, (LPCSTR) format, 
                         (LPSTR) cstr_time, cchTime);

    // convert result to unicode
    res = ConvertToUnicode((const char *) cstr_time, (const PRInt32) len, timeStr, &len);

    delete [] cstr_time;
  }
  return NS_SUCCEEDED(res) ? len : 0;
}

int nsDateTimeFormatWin::nsGetDateFormatW(DWORD dwFlags, const SYSTEMTIME *lpDate,
                                          const char* format, PRUnichar *dataStr, int cchDate)
{
  int len = 0;
  nsresult res = NS_OK;

  if (mW_API) {
    nsString formatString(format ? format : "");
    LPCWSTR wstr = format ? (LPCWSTR) formatString.GetUnicode() : NULL;
    len = GetDateFormatW(mLCID, dwFlags, lpDate, wstr, (LPWSTR) dataStr, cchDate);
  }
  else {
    char *cstr_date;
    cstr_date = new char[NSDATETIMEFORMAT_BUFFER_LEN];
    if (nsnull == cstr_date) {
      return 0;
    }
    len = GetDateFormatA(mLCID, dwFlags, lpDate, (LPCSTR) format, 
                         (LPSTR) cstr_date, cchDate);

    // convert result to unicode
    res = ConvertToUnicode((const char *) cstr_date, (const PRInt32) len, dataStr, &len);

    delete [] cstr_date;
  }
  return NS_SUCCEEDED(res) ? len : 0;
}
