/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License.  You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are Copyright (C) 1998
 * Netscape Communications Corporation.  All Rights Reserved.
 */

#define NS_IMPL_IDS

#include "pratom.h"
#include "nsIComponentManager.h"
#include "nsIServiceManager.h"
#include "nsIFactory.h"
#include "nsCOMPtr.h"
#include "nsICharsetConverterInfo.h"
#include "nsUCvLatinCID.h"
#include "nsUCvLatinDll.h"
#include "nsISO88591ToUnicode.h"
#include "nsISO88592ToUnicode.h"
#include "nsISO88593ToUnicode.h"
#include "nsISO88594ToUnicode.h"
#include "nsISO88595ToUnicode.h"
#include "nsISO88596ToUnicode.h"
#include "nsISO88597ToUnicode.h"
#include "nsISO88598ToUnicode.h"
#include "nsISO88599ToUnicode.h"
#include "nsISO885914ToUnicode.h"
#include "nsISO885915ToUnicode.h"
#include "nsCP1250ToUnicode.h"
#include "nsCP1251ToUnicode.h"
#include "nsCP1252ToUnicode.h"
#include "nsCP1253ToUnicode.h"
#include "nsCP1254ToUnicode.h"
#include "nsCP1257ToUnicode.h"
#include "nsCP1258ToUnicode.h"
#include "nsCP874ToUnicode.h"
#include "nsKOI8RToUnicode.h"
#include "nsKOI8UToUnicode.h"
#include "nsMacRomanToUnicode.h"
#include "nsMacCEToUnicode.h"
#include "nsMacGreekToUnicode.h"
#include "nsMacTurkishToUnicode.h"
#include "nsMacCroatianToUnicode.h"
#include "nsMacRomanianToUnicode.h"
#include "nsMacCyrillicToUnicode.h"
#include "nsMacUkrainianToUnicode.h"
#include "nsMacIcelandicToUnicode.h"
#include "nsARMSCII8ToUnicode.h"
#include "nsTCVN5712ToUnicode.h"
#include "nsVISCIIToUnicode.h"
#include "nsVPSToUnicode.h"
#include "nsUTF8ToUnicode.h"
#include "nsUnicodeToISO88591.h"
#include "nsUnicodeToISO88592.h"
#include "nsUnicodeToISO88593.h"
#include "nsUnicodeToISO88594.h"
#include "nsUnicodeToISO88595.h"
#include "nsUnicodeToISO88596.h"
#include "nsUnicodeToISO88597.h"
#include "nsUnicodeToISO88598.h"
#include "nsUnicodeToISO88599.h"
#include "nsUnicodeToISO885914.h"
#include "nsUnicodeToISO885915.h"
#include "nsUnicodeToCP1250.h"
#include "nsUnicodeToCP1251.h"
#include "nsUnicodeToCP1252.h"
#include "nsUnicodeToCP1253.h"
#include "nsUnicodeToCP1254.h"
#include "nsUnicodeToCP1257.h"
#include "nsUnicodeToCP1258.h"
#include "nsUnicodeToCP874.h"
#include "nsUnicodeToKOI8R.h"
#include "nsUnicodeToKOI8U.h"
#include "nsUnicodeToMacRoman.h"
#include "nsUnicodeToMacCE.h"
#include "nsUnicodeToMacGreek.h"
#include "nsUnicodeToMacTurkish.h"
#include "nsUnicodeToMacCroatian.h"
#include "nsUnicodeToMacRomanian.h"
#include "nsUnicodeToMacCyrillic.h"
#include "nsUnicodeToMacUkrainian.h"
#include "nsUnicodeToMacIcelandic.h"
#include "nsUnicodeToARMSCII8.h"
#include "nsUnicodeToTCVN5712.h"
#include "nsUnicodeToVISCII.h"
#include "nsUnicodeToVPS.h"
#include "nsUnicodeToUTF8.h"

// just for NS_IMPL_IDS; this is a good, central place to implement GUIDs
#include "nsIUnicodeDecoder.h"
#include "nsIUnicodeDecodeUtil.h"
#include "nsIUnicodeDecodeHelper.h"
#include "nsIUnicodeEncoder.h"
#include "nsIUnicodeEncodeHelper.h"
#include "nsICharsetConverterManager.h"

//----------------------------------------------------------------------
// Global functions and data [declaration]

static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);
static NS_DEFINE_IID(kIFactoryIID, NS_IFACTORY_IID);
static NS_DEFINE_CID(kComponentManagerCID, NS_COMPONENTMANAGER_CID);

PRInt32 g_InstanceCount = 0;
PRInt32 g_LockCount = 0;

typedef nsresult (* fpCreateInstance) (nsISupports **);

struct FactoryData
{
  const nsCID   * mCID;
  fpCreateInstance  CreateInstance;
  char    * mCharsetSrc;
  char    * mCharsetDest;
};

FactoryData g_FactoryData[] =
{
  {
    &kISO88591ToUnicodeCID,
    nsISO88591ToUnicode::CreateInstance,
    "ISO-8859-1",
    "Unicode"
  },
  {
    &kISO88592ToUnicodeCID,
    nsISO88592ToUnicode::CreateInstance,
    "ISO-8859-2",
    "Unicode"
  },
  {
    &kISO88593ToUnicodeCID,
    nsISO88593ToUnicode::CreateInstance,
    "ISO-8859-3",
    "Unicode"
  },
  {
    &kISO88594ToUnicodeCID,
    nsISO88594ToUnicode::CreateInstance,
    "ISO-8859-4",
    "Unicode"
  },
  {
    &kISO88595ToUnicodeCID,
    nsISO88595ToUnicode::CreateInstance,
    "ISO-8859-5",
    "Unicode"
  },
  {
    &kISO88596ToUnicodeCID,
    nsISO88596ToUnicode::CreateInstance,
    "ISO-8859-6",
    "Unicode"
  },
  {
    &kISO88597ToUnicodeCID,
    nsISO88597ToUnicode::CreateInstance,
    "ISO-8859-7",
    "Unicode"
  },
  {
    &kISO88598ToUnicodeCID,
    nsISO88598ToUnicode::CreateInstance,
    "ISO-8859-8",
    "Unicode"
  },
  {
    &kISO88599ToUnicodeCID,
    nsISO88599ToUnicode::CreateInstance,
    "ISO-8859-9",
    "Unicode"
  },
  {
    &kISO885914ToUnicodeCID,
    nsISO885914ToUnicode::CreateInstance,
    "ISO-8859-14",
    "Unicode"
  },
  {
    &kISO885915ToUnicodeCID,
    nsISO885915ToUnicode::CreateInstance,
    "ISO-8859-15",
    "Unicode"
  },
  {
    &kCP1250ToUnicodeCID,
    nsCP1250ToUnicode::CreateInstance,
    "windows-1250",
    "Unicode"
  },
  {
    &kCP1251ToUnicodeCID,
    nsCP1251ToUnicode::CreateInstance,
    "windows-1251",
    "Unicode"
  },
  {
    &kCP1252ToUnicodeCID,
    nsCP1252ToUnicode::CreateInstance,
    "windows-1252",
    "Unicode"
  },
  {
    &kCP1253ToUnicodeCID,
    nsCP1253ToUnicode::CreateInstance,
    "windows-1253",
    "Unicode"
  },
  {
    &kCP1254ToUnicodeCID,
    nsCP1254ToUnicode::CreateInstance,
    "windows-1254",
    "Unicode"
  },
  {
    &kCP1257ToUnicodeCID,
    nsCP1257ToUnicode::CreateInstance,
    "windows-1257",
    "Unicode"
  },
  {
    &kCP1258ToUnicodeCID,
    nsCP1258ToUnicode::CreateInstance,
    "windows-1258",
    "Unicode"
  },
  {
    &kCP874ToUnicodeCID,
    nsCP874ToUnicode::CreateInstance,
    "TIS-620",
    "Unicode"
  },
  {
    &kKOI8RToUnicodeCID,
    nsKOI8RToUnicode::CreateInstance,
    "KOI8-R",
    "Unicode"
  },
  {
    &kKOI8UToUnicodeCID,
    nsKOI8UToUnicode::CreateInstance,
    "KOI8-U",
    "Unicode"
  },
  {
    &kMacRomanToUnicodeCID,
    nsMacRomanToUnicode::CreateInstance,
    "x-mac-roman",
    "Unicode"
  },
  {
    &kMacCEToUnicodeCID,
    nsMacCEToUnicode::CreateInstance,
    "x-mac-ce",
    "Unicode"
  },
  {
    &kMacGreekToUnicodeCID,
    nsMacGreekToUnicode::CreateInstance,
    "x-mac-greek",
    "Unicode"
  },
  {
    &kMacTurkishToUnicodeCID,
    nsMacTurkishToUnicode::CreateInstance,
    "x-mac-turkish",
    "Unicode"
  },
  {
    &kMacCroatianToUnicodeCID,
    nsMacCroatianToUnicode::CreateInstance,
    "x-mac-croatian",
    "Unicode"
  },
  {
    &kMacRomanianToUnicodeCID,
    nsMacRomanianToUnicode::CreateInstance,
    "x-mac-romanian",
    "Unicode"
  },
  {
    &kMacCyrillicToUnicodeCID,
    nsMacCyrillicToUnicode::CreateInstance,
    "x-mac-cyrillic",
    "Unicode"
  },
  {
    &kMacUkrainianToUnicodeCID,
    nsMacUkrainianToUnicode::CreateInstance,
    "x-mac-ukrainian",
    "Unicode"
  },
  {
    &kMacIcelandicToUnicodeCID,
    nsMacIcelandicToUnicode::CreateInstance,
    "x-mac-icelandic",
    "Unicode"
  },
  {
    &kARMSCII8ToUnicodeCID,
    nsARMSCII8ToUnicode::CreateInstance,
    "armscii-8",
    "Unicode"
  },
  {
    &kTCVN5712ToUnicodeCID,
    nsTCVN5712ToUnicode::CreateInstance,
    "x-viet-tcvn",
    "Unicode"
  },
  {
    &kVISCIIToUnicodeCID,
    nsVISCIIToUnicode::CreateInstance,
    "VISCII",
    "Unicode"
  },
  {
    &kVPSToUnicodeCID,
    nsVPSToUnicode::CreateInstance,
    "x-viet-vps",
    "Unicode"
  },
  {
    &kUTF8ToUnicodeCID,
    nsUTF8ToUnicode::CreateInstance,
    "UTF-8",
    "Unicode"
  },
  {
    &kUnicodeToISO88591CID,
    nsUnicodeToISO88591::CreateInstance,
    "Unicode",
    "ISO-8859-1"
  },
  {
    &kUnicodeToISO88592CID,
    nsUnicodeToISO88592::CreateInstance,
    "Unicode",
    "ISO-8859-2"
  },
  {
    &kUnicodeToISO88593CID,
    nsUnicodeToISO88593::CreateInstance,
    "Unicode",
    "ISO-8859-3"
  },
  {
    &kUnicodeToISO88594CID,
    nsUnicodeToISO88594::CreateInstance,
    "Unicode",
    "ISO-8859-4"
  },
  {
    &kUnicodeToISO88595CID,
    nsUnicodeToISO88595::CreateInstance,
    "Unicode",
    "ISO-8859-5"
  },
  {
    &kUnicodeToISO88596CID,
    nsUnicodeToISO88596::CreateInstance,
    "Unicode",
    "ISO-8859-6"
  },
  {
    &kUnicodeToISO88597CID,
    nsUnicodeToISO88597::CreateInstance,
    "Unicode",
    "ISO-8859-7"
  },
  {
    &kUnicodeToISO88598CID,
    nsUnicodeToISO88598::CreateInstance,
    "Unicode",
    "ISO-8859-8"
  },
  {
    &kUnicodeToISO88599CID,
    nsUnicodeToISO88599::CreateInstance,
    "Unicode",
    "ISO-8859-9"
  },
  {
    &kUnicodeToISO885914CID,
    nsUnicodeToISO885914::CreateInstance,
    "Unicode",
    "ISO-8859-14"
  },
  {
    &kUnicodeToISO885915CID,
    nsUnicodeToISO885915::CreateInstance,
    "Unicode",
    "ISO-8859-15"
  },
  {
    &kUnicodeToCP1250CID,
    nsUnicodeToCP1250::CreateInstance,
    "Unicode",
    "windows-1250"
  },
  {
    &kUnicodeToCP1251CID,
    nsUnicodeToCP1251::CreateInstance,
    "Unicode",
    "windows-1251"
  },
  {
    &kUnicodeToCP1252CID,
    nsUnicodeToCP1252::CreateInstance,
    "Unicode",
    "windows-1252"
  },
  {
    &kUnicodeToCP1253CID,
    nsUnicodeToCP1253::CreateInstance,
    "Unicode",
    "windows-1253"
  },
  {
    &kUnicodeToCP1254CID,
    nsUnicodeToCP1254::CreateInstance,
    "Unicode",
    "windows-1254"
  },
  {
    &kUnicodeToCP1257CID,
    nsUnicodeToCP1257::CreateInstance,
    "Unicode",
    "windows-1257"
  },
  {
    &kUnicodeToCP1258CID,
    nsUnicodeToCP1258::CreateInstance,
    "Unicode",
    "windows-1258"
  },
  {
    &kUnicodeToCP874CID,
    nsUnicodeToCP874::CreateInstance,
    "Unicode",
    "TIS-620"
  },
  {
    &kUnicodeToKOI8RCID,
    nsUnicodeToKOI8R::CreateInstance,
    "Unicode",
    "KOI8-R"
  },
  {
    &kUnicodeToKOI8UCID,
    nsUnicodeToKOI8U::CreateInstance,
    "Unicode",
    "KOI8-U"
  },
  {
    &kUnicodeToMacRomanCID,
    nsUnicodeToMacRoman::CreateInstance,
    "Unicode",
    "x-mac-roman"
  },
  {
    &kUnicodeToMacCECID,
    nsUnicodeToMacCE::CreateInstance,
    "Unicode",
    "x-mac-ce"
  },
  {
    &kUnicodeToMacGreekCID,
    nsUnicodeToMacGreek::CreateInstance,
    "Unicode",
    "x-mac-greek"
  },
  {
    &kUnicodeToMacTurkishCID,
    nsUnicodeToMacTurkish::CreateInstance,
    "Unicode",
    "x-mac-turkish"
  },
  {
    &kUnicodeToMacCroatianCID,
    nsUnicodeToMacCroatian::CreateInstance,
    "Unicode",
    "x-mac-croatian"
  },
  {
    &kUnicodeToMacRomanianCID,
    nsUnicodeToMacRomanian::CreateInstance,
    "Unicode",
    "x-mac-romanian"
  },
  {
    &kUnicodeToMacCyrillicCID,
    nsUnicodeToMacCyrillic::CreateInstance,
    "Unicode",
    "x-mac-cyrillic"
  },
  {
    &kUnicodeToMacUkrainianCID,
    nsUnicodeToMacUkrainian::CreateInstance,
    "Unicode",
    "x-mac-ukrainian"
  },
  {
    &kUnicodeToMacIcelandicCID,
    nsUnicodeToMacIcelandic::CreateInstance,
    "Unicode",
    "x-mac-icelandic"
  },
  {
    &kUnicodeToARMSCII8CID,
    nsUnicodeToARMSCII8::CreateInstance,
    "Unicode",
    "armscii-8"
  },
  {
    &kUnicodeToTCVN5712CID,
    nsUnicodeToTCVN5712::CreateInstance,
    "Unicode",
    "x-viet-tcvn"
  },
  {
    &kUnicodeToVISCIICID,
    nsUnicodeToVISCII::CreateInstance,
    "Unicode",
    "VISCII"
  },
  {
    &kUnicodeToVPSCID,
    nsUnicodeToVPS::CreateInstance,
    "Unicode",
    "x-viet-vps"
  },
  {
    &kUnicodeToUTF8CID,
    nsUnicodeToUTF8::CreateInstance,
    "Unicode",
    "UTF-8"
  }
};

#define ARRAY_SIZE(_array)                                      \
     (sizeof(_array) / sizeof(_array[0]))

//----------------------------------------------------------------------
// Class nsConverterFactory [declaration]

/**
 * General factory class for converter objects.
 * 
 * @created         24/Feb/1998
 * @author  Catalin Rotaru [CATA]
 */
class nsConverterFactory : public nsIFactory, 
public nsICharsetConverterInfo
{
  NS_DECL_ISUPPORTS

private:

  FactoryData * mData;

public:

  /**
   * Class constructor.
   */
  nsConverterFactory(FactoryData * aData);

  /**
   * Class destructor.
   */
  virtual ~nsConverterFactory();

  //--------------------------------------------------------------------
  // Interface nsIFactory [declaration]

  NS_IMETHOD CreateInstance(nsISupports *aDelegate, const nsIID &aIID,
                            void **aResult);

  NS_IMETHOD LockFactory(PRBool aLock);

  //--------------------------------------------------------------------
  // Interface nsICharsetConverterInfo [declaration]

  NS_IMETHOD GetCharsetSrc(char ** aCharset);
  NS_IMETHOD GetCharsetDest(char ** aCharset);
};

//----------------------------------------------------------------------
// Global functions and data [implementation]

extern "C" NS_EXPORT PRBool NSCanUnload(nsISupports* aServMgr)
{
  return PRBool(g_InstanceCount == 0 && g_LockCount == 0);
}

extern "C" NS_EXPORT nsresult NSGetFactory(nsISupports* aServMgr,
                                           const nsCID &aClass,
                                           const char *aClassName,
                                           const char *aProgID,
                                           nsIFactory **aFactory)
{
  if (aFactory == NULL) return NS_ERROR_NULL_POINTER;

  nsresult res;
  nsConverterFactory * fac;
  FactoryData * data;

  for (PRUint32 i=0; i<ARRAY_SIZE(g_FactoryData); i++) {
    data = &(g_FactoryData[i]);
    if (aClass.Equals(*(data->mCID))) {
      fac = new nsConverterFactory(data);
      res = fac->QueryInterface(kIFactoryIID, (void **) aFactory);
      if (NS_FAILED(res)) {
        *aFactory = NULL;
        delete fac;
      }

      return res;
    }
  }

  return NS_NOINTERFACE;
}

extern "C" NS_EXPORT nsresult NSRegisterSelf(nsISupports* aServMgr, const char * path)
{
  nsresult rv;

  nsCOMPtr<nsIServiceManager> servMgr(do_QueryInterface(aServMgr, &rv));
  if (NS_FAILED(rv)) return rv;

  nsIComponentManager* compMgr;
  rv = servMgr->GetService(kComponentManagerCID, 
                           nsIComponentManager::GetIID(), 
                           (nsISupports**)&compMgr);
  if (NS_FAILED(rv)) return rv;

  for (PRUint32 i=0; i<ARRAY_SIZE(g_FactoryData); i++) {
    rv = compMgr->RegisterComponent(*(g_FactoryData[i].mCID), NULL, NULL,
      path, PR_TRUE, PR_TRUE);
    if(NS_FAILED(rv) && (NS_ERROR_FACTORY_EXISTS != rv)) goto done;
  }

  done:
  (void)servMgr->ReleaseService(kComponentManagerCID, compMgr);
  return rv;
}

extern "C" NS_EXPORT nsresult NSUnregisterSelf(nsISupports* aServMgr, const char * path)
{
  nsresult rv;

  nsCOMPtr<nsIServiceManager> servMgr(do_QueryInterface(aServMgr, &rv));
  if (NS_FAILED(rv)) return rv;

  nsIComponentManager* compMgr;
  rv = servMgr->GetService(kComponentManagerCID, 
                           nsIComponentManager::GetIID(), 
                           (nsISupports**)&compMgr);
  if (NS_FAILED(rv)) return rv;

  for (PRUint32 i=0; i<ARRAY_SIZE(g_FactoryData); i++) {
    rv = compMgr->UnregisterComponent(*(g_FactoryData[i].mCID), path);
    if(NS_FAILED(rv)) goto done;
  }

  done:
  (void)servMgr->ReleaseService(kComponentManagerCID, compMgr);
  return rv;
}

//----------------------------------------------------------------------
// Class nsConverterFactory [implementation]

nsConverterFactory::nsConverterFactory(FactoryData * aData) 
{
  mData = aData;

  NS_INIT_REFCNT();
  PR_AtomicIncrement(&g_InstanceCount);
}

nsConverterFactory::~nsConverterFactory() 
{
  PR_AtomicDecrement(&g_InstanceCount);
}

//----------------------------------------------------------------------
// Interface nsISupports [implementation]

NS_IMPL_ADDREF(nsConverterFactory);
NS_IMPL_RELEASE(nsConverterFactory);

nsresult nsConverterFactory::QueryInterface(REFNSIID aIID, 
                                            void** aInstancePtr)
{                                                                        
  if (NULL == aInstancePtr) {                                            
    return NS_ERROR_NULL_POINTER;                                        
  }                                                                      
                                                                         
  *aInstancePtr = NULL;                                                  

  if (aIID.Equals(kICharsetConverterInfoIID)) {
    *aInstancePtr = (void*) ((nsICharsetConverterInfo*)this);
  } else if (aIID.Equals(kIFactoryIID)) {
    *aInstancePtr = (void*) ((nsIFactory*)this);
  } else if (aIID.Equals(kISupportsIID)) {
    *aInstancePtr = (void*) ((nsISupports*)(nsIFactory*)this);
  } else {
    return NS_NOINTERFACE;
  }

  NS_ADDREF_THIS();                                                    
  return NS_OK;                                                        
}

//----------------------------------------------------------------------
// Interface nsIFactory [implementation]

NS_IMETHODIMP nsConverterFactory::CreateInstance(nsISupports *aDelegate,
                                                 const nsIID &aIID,
                                                 void **aResult)
{
  if (aResult == NULL) return NS_ERROR_NULL_POINTER;
  if (aDelegate != NULL) return NS_ERROR_NO_AGGREGATION;

  nsISupports * t;
  mData->CreateInstance(&t);
  if (t == NULL) return NS_ERROR_OUT_OF_MEMORY;
  
  nsresult res = t->QueryInterface(aIID, aResult);
  if (NS_FAILED(res)) delete t;

  return res;
}

NS_IMETHODIMP nsConverterFactory::LockFactory(PRBool aLock)
{
  if (aLock) PR_AtomicIncrement(&g_LockCount);
  else PR_AtomicDecrement(&g_LockCount);

  return NS_OK;
}

//----------------------------------------------------------------------
// Interface nsICharsetConverterInfo [implementation]

NS_IMETHODIMP nsConverterFactory::GetCharsetSrc(char ** aCharset)
{
  (*aCharset) = mData->mCharsetSrc;
  return NS_OK;
}

NS_IMETHODIMP nsConverterFactory::GetCharsetDest(char ** aCharset)
{
  (*aCharset) = mData->mCharsetDest;
  return NS_OK;
}
