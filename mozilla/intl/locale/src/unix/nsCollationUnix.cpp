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

#include  <locale.h>
#include "prmem.h"
#include "nsCollationUnix.h"
#include "nsIComponentManager.h"
#include "nsLocaleCID.h"
#include "nsIPosixLocale.h"


static NS_DEFINE_IID(kICollationIID, NS_ICOLLATION_IID);
static NS_DEFINE_IID(kPosixLocaleFactoryCID, NS_POSIXLOCALEFACTORY_CID);
static NS_DEFINE_IID(kIPosixLocaleIID, NS_IPOSIXLOCALE_IID);

NS_IMPL_ISUPPORTS(nsCollationUnix, kICollationIID);


nsCollationUnix::nsCollationUnix() 
{
  NS_INIT_REFCNT(); 
  mCollation = NULL;
}

nsCollationUnix::~nsCollationUnix() 
{
  if (mCollation != NULL)
    delete mCollation;
}

nsresult nsCollationUnix::Initialize(nsILocale* locale) 
{
  NS_ASSERTION(mCollation == NULL, "Should only be initialized once");

  mCollation = new nsCollation;
  if (mCollation == NULL) {
    NS_ASSERTION(0, "mCollation creation failed");
    return NS_ERROR_OUT_OF_MEMORY;
  }

  // store local charset name
  mCharset.SetString("ISO-8859-1"); //TODO: need to get this from locale

  // store platform locale
  mLocale.SetString("C");

  if (locale != nsnull) {
    const PRUnichar *aLocaleUnichar;
    nsString aLocale;
    nsString aCategory("NSILOCALE_COLLATE");
    nsresult res = locale->GetCategory(aCategory.ToNewUnicode(), &aLocaleUnichar);
    if (NS_FAILED(res)) {
      return res;
    }
    aLocale.SetString(aLocaleUnichar);

    nsIPosixLocale* posixLocale;
    char locale[32];
    size_t length = 32;
    res = nsComponentManager::CreateInstance(kPosixLocaleFactoryCID, NULL, kIPosixLocaleIID, (void**)&posixLocale);
    if (NS_FAILED(res)) {
      return res;
    }
    if (NS_SUCCEEDED(res = posixLocale->GetPlatformLocale(&aLocale, locale, length))) {
      mLocale.SetString(locale);
    }
    posixLocale->Release();
  }

  return NS_OK;
};
 

nsresult nsCollationUnix::GetSortKeyLen(const nsCollationStrength strength, 
                           const nsString& stringIn, PRUint32* outLen)
{
  nsresult res = NS_OK;

  // this may not necessary because collation key length 
  // probably will not change by this normalization
  nsAutoString stringNormalized(stringIn);
  if (strength != kCollationCaseSensitive) {
    res = mCollation->NormalizeString(stringNormalized);
  }

  // convert unicode to charset
  char *str;

  res = mCollation->UnicodeToChar(stringNormalized, &str, mCharset);
  if (NS_SUCCEEDED(res) && str != NULL) {
    char *cstr = mLocale.ToNewCString();
    char *old_locale =  setlocale(LC_COLLATE, NULL);
    (void) setlocale(LC_COLLATE, cstr);
    // call strxfrm to calculate a key length 
    int len = strxfrm(NULL, str, 0) + 1;
    (void) setlocale(LC_COLLATE, old_locale);
    delete [] cstr;
    *outLen = (len == -1) ? 0 : (PRUint32)len;
    PR_Free(str);
  }

  return res;
}

nsresult nsCollationUnix::CreateRawSortKey(const nsCollationStrength strength, 
                           const nsString& stringIn, PRUint8* key, PRUint32* outLen)
{
  nsresult res = NS_OK;

  nsAutoString stringNormalized(stringIn);
  if (strength != kCollationCaseSensitive) {
    res = mCollation->NormalizeString(stringNormalized);
  }
  // convert unicode to charset
  char *str;

  res = mCollation->UnicodeToChar(stringNormalized, &str, mCharset);
  if (NS_SUCCEEDED(res) && str != NULL) {
    char *cstr = mLocale.ToNewCString();
    char *old_locale =  setlocale(LC_COLLATE, NULL);
    (void) setlocale(LC_COLLATE, cstr);
    // call strxfrm to generate a key 
    int len = strxfrm((char *) key, str, strlen(str));
    (void) setlocale(LC_COLLATE, old_locale);
    delete [] cstr;
    *outLen = (len == -1) ? 0 : (PRUint32)len;
    PR_Free(str);
  }

  return res;
}

