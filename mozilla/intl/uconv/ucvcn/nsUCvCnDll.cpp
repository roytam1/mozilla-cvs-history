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
#include "nsUCvCnCID.h"
#include "nsUCvCnDll.h"
#include "nsGB2312ToUnicode.h"
#include "nsUnicodeToGB2312.h"

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

PRUint16 g_AsciiMapping[] = {
  0x0001, 0x0004, 0x0005, 0x0008, 0x0000, 0x0000, 0x007F, 0x0000
};
PRUint16 g_utGB2312Mapping[] = {
#include "gb2312.ut"
};
PRUint16 g_ufGB2312Mapping[] = {
#include "gb2312.uf"
};


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
    &kGB2312ToUnicodeCID,
    nsGB2312ToUnicode::CreateInstance,
    "GB2312",
    "Unicode"
  },
  {
    &kUnicodeToGB2312CID,
    nsUnicodeToGB2312::CreateInstance,
    "Unicode",
    "GB2312"
  },
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
