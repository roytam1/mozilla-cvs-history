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

#include "nsCollationMac.h"
#include <Resources.h>
#include <TextUtils.h>
#include <Script.h>
#include "prmem.h"
#include "prmon.h"
#include "nsIServiceManager.h"
#include "nsIComponentManager.h"
#include "nsILocaleService.h"
#include "nsLocaleCID.h"
#include "nsIPlatformCharset.h"
#include "nsIMacLocale.h"
#include "nsCOMPtr.h"

static NS_DEFINE_IID(kICollationIID, NS_ICOLLATION_IID);
static NS_DEFINE_IID(kMacLocaleFactoryCID, NS_MACLOCALEFACTORY_CID);
static NS_DEFINE_IID(kIMacLocaleIID, NS_IMACLOCALE_IID);
static NS_DEFINE_CID(kLocaleServiceCID, NS_LOCALESERVICE_CID); 
static NS_DEFINE_CID(kPlatformCharsetCID, NS_PLATFORMCHARSET_CID);


////////////////////////////////////////////////////////////////////////////////

/* Copy from FE_StrColl(), macfe/utility/locale.cp. */
static short mac_get_script_sort_id(const short scriptcode)
{
	short itl2num;
	ItlbRecord **ItlbRecordHandle;

	/* get itlb of the system script */
	ItlbRecordHandle = (ItlbRecord **) GetResource('itlb', scriptcode);
	
	/* get itl2 number of current system script from itlb if possible
	 * otherwise, try script manager (Script manager won't update 
	 * itl2 number when the change on the fly )
	 */
	if(ItlbRecordHandle != NULL)
	{
		if(*ItlbRecordHandle == NULL)
			LoadResource((Handle)ItlbRecordHandle);
			
		if(*ItlbRecordHandle != NULL)
			itl2num = (*ItlbRecordHandle)->itlbSort;
		else
			itl2num = GetScriptVariable(scriptcode, smScriptSort);
	} else {	/* Use this as fallback */
		itl2num = GetScriptVariable(scriptcode, smScriptSort);
	}
	
	return itl2num;
}

static Handle itl2Handle;

static int mac_sort_tbl_compare(const void* s1, const void* s2)
{
	return CompareText((Ptr) s1, (Ptr) s2, 1, 1, itl2Handle);
}

static int mac_sort_tbl_init(const short scriptcode, unsigned char *mac_sort_tbl)
{
	int i;
	unsigned char sort_tbl[256];
	
	for (i = 0; i < 256; i++)
		sort_tbl[i] = (unsigned char) i;

	/* Get itl2. */
	itl2Handle = GetResource('itl2', mac_get_script_sort_id(scriptcode));
	if (itl2Handle == NULL)
		return -1;
	
	/* qsort */
	PRMonitor* mon = PR_NewMonitor();
	PR_EnterMonitor(mon);
	qsort((void *) sort_tbl, 256, 1, mac_sort_tbl_compare);
	(void) PR_ExitMonitor(mon);
	PR_DestroyMonitor(mon);
	
	/* Put index to the table so we can map character code to sort oder. */
	for (i = 0; i < 256; i++)
		mac_sort_tbl[sort_tbl[i]] = (unsigned char) i;
		
	return 0;
}

inline unsigned char mac_sort_tbl_search(const unsigned char ch, const unsigned char* mac_sort_tbl)
{
	/* Map character code to sort order. */
	return mac_sort_tbl[ch];
}

////////////////////////////////////////////////////////////////////////////////

NS_IMPL_ISUPPORTS(nsCollationMac, kICollationIID);


nsCollationMac::nsCollationMac() 
{
  NS_INIT_REFCNT(); 
  mCollation = NULL;
}

nsCollationMac::~nsCollationMac() 
{
  if (mCollation != NULL)
    delete mCollation;
}

nsresult nsCollationMac::Initialize(nsILocale* locale) 
{
  NS_ASSERTION(mCollation == NULL, "Should only be initialized once.");
  nsresult res;
  mCollation = new nsCollation;
  if (mCollation == NULL) {
    NS_ASSERTION(0, "mCollation creation failed");
    return NS_ERROR_OUT_OF_MEMORY;
  }

  // locale -> script code + charset name
  m_scriptcode = smRoman;
  mCharset.AssignWithConversion("ISO-8859-1");

  PRUnichar *aLocaleUnichar; 
  nsAutoString aCategory; aCategory.AssignWithConversion("NSILOCALE_COLLATE");

  // get locale string, use app default if no locale specified
  if (locale == nsnull) {
    NS_WITH_SERVICE(nsILocaleService, localeService, kLocaleServiceCID, &res);
    if (NS_SUCCEEDED(res)) {
      nsILocale *appLocale;
      res = localeService->GetApplicationLocale(&appLocale);
      if (NS_SUCCEEDED(res)) {
        res = appLocale->GetCategory(aCategory.GetUnicode(), &aLocaleUnichar);
        appLocale->Release();
      }
    }
  }
  else {
    res = locale->GetCategory(aCategory.GetUnicode(), &aLocaleUnichar);
  }

  if (NS_SUCCEEDED(res)) {
    nsAutoString aLocale(aLocaleUnichar);
    nsAllocator::Free(aLocaleUnichar);

    short scriptcode, langcode, regioncode;
    nsCOMPtr <nsIMacLocale> macLocale = do_GetService(kMacLocaleFactoryCID, &res);
    if (NS_SUCCEEDED(res)) {
      if (NS_SUCCEEDED(res = macLocale->GetPlatformLocale(&aLocale, &scriptcode, &langcode, &regioncode))) {
        m_scriptcode = scriptcode;
      }
    }

    nsCOMPtr <nsIPlatformCharset> platformCharset = do_GetService(NS_PLATFORMCHARSET_PROGID, &res);
    if (NS_SUCCEEDED(res)) {
      PRUnichar* mappedCharset = NULL;
      res = platformCharset->GetDefaultCharsetForLocale(aLocale.GetUnicode(), &mappedCharset);
      if (NS_SUCCEEDED(res) && mappedCharset) {
        mCharset.Assign(mappedCharset);
        nsAllocator::Free(mappedCharset);
      }
    }
  }
  NS_ASSERTION(NS_SUCCEEDED(res), "initialization failed, use default values");

  // Initialize a mapping table for the script code.
  if (mac_sort_tbl_init(m_scriptcode, m_mac_sort_tbl) == -1) {
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
};
 

nsresult nsCollationMac::GetSortKeyLen(const nsCollationStrength strength, 
                           const nsString& stringIn, PRUint32* outLen)
{
  *outLen = stringIn.Length() * sizeof(PRUnichar);
  return NS_OK;
}

nsresult nsCollationMac::CreateRawSortKey(const nsCollationStrength strength, 
                           const nsString& stringIn, PRUint8* key, PRUint32* outLen)
{
  nsresult res = NS_OK;

  nsAutoString stringNormalized(stringIn);
  if (strength != kCollationCaseSensitive) {
    res = mCollation->NormalizeString(stringNormalized);
  }
  // convert unicode to charset
  char *str;
  int str_len;

  res = mCollation->UnicodeToChar(stringNormalized, &str, mCharset);
  if (NS_SUCCEEDED(res) && str != NULL) {
    str_len = strlen(str);
    NS_ASSERTION(str_len <= *outLen, "output buffer too small");
    
    // If no CJK then generate a collation key
    if (smJapanese != m_scriptcode && smKorean != m_scriptcode && 
        smTradChinese != m_scriptcode && smSimpChinese != m_scriptcode) {
      for (int i = 0; i < str_len; i++) {
        *key++ = (PRUint8) mac_sort_tbl_search((const unsigned char) str[i], m_mac_sort_tbl);
      }
    }
    else {
      // No CJK support, just copy the row string.
      strcpy((char *) key, str);
      while (*key) {
        if ((unsigned char) *key < 128) {
          key++;
        }
        else {
          // ShiftJIS specific, shift hankaku kana in front of zenkaku.
          if (smJapanese == m_scriptcode) {
            if (*key >= 0xA0 && *key < 0xE0) {
              *key -= (0xA0 - 0x81);
            }
            else if (*key >= 0x81 && *key < 0xA0) {
              *key += (0xE0 - 0xA0);
            } 
          }
          // advance 2 bytes if the API says so and not passing the end of the string
          key += ((smFirstByte == CharacterByteType((Ptr) key, 0, m_scriptcode)) &&
                  (*(key + 1))) ? 2 : 1;
        }
      }
    }
    *outLen = str_len;
    PR_Free(str);
  }

  return NS_OK;
}

