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
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

#define NS_IMPL_IDS

#include "nspr.h"
#include "nsString.h"
#include "pratom.h"
#include "nsCOMPtr.h"
#include "nsIFactory.h"
#include "nsIRegistry.h"
#include "nsIServiceManager.h"
#include "nsIModule.h"
#include "nsUCvCnCID.h"
#include "nsUCvCnDll.h"

#include "nsHZToUnicode.h"
#include "nsUnicodeToHZ.h"
#include "nsGBKToUnicode.h"
#include "nsUnicodeToGBK.h"
#include "nsGB2312ToUnicodeV2.h"
#include "nsUnicodeToGB2312V2.h"
#include "nsGB2312ToUnicode.h"
#include "nsUnicodeToGB2312.h"
#include "nsUnicodeToGB2312GL.h"

//----------------------------------------------------------------------------
// Global functions and data [declaration]

static NS_DEFINE_CID(kComponentManagerCID, NS_COMPONENTMANAGER_CID);

#define DECODER_NAME_BASE "Unicode Decoder-"
#define ENCODER_NAME_BASE "Unicode Encoder-"

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

static FactoryData g_FactoryData[] =
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
  {
    &kCP936ToUnicodeCID,
    nsGB2312ToUnicodeV2::CreateInstance,
    "windows-936",
    "Unicode"
  },
  {
    &kUnicodeToCP936CID,
    nsUnicodeToGB2312V2::CreateInstance,
    "Unicode",
    "windows-936"
  },
  {
    &kGBKToUnicodeCID,
    nsGBKToUnicode::CreateInstance,
    "x-gbk",
    "Unicode"
  },
  {
    &kUnicodeToGBKCID,
    nsUnicodeToGBK::CreateInstance,
    "Unicode",
    "x-gbk"
  },
  {
    &kHZToUnicodeCID,
    nsHZToUnicode::CreateInstance,
    "HZ-GB-2312",
    "Unicode"
  },
  {
    &kUnicodeToHZCID,
    nsUnicodeToHZ::CreateInstance,
    "Unicode",
    "HZ-GB-2312"
  },
  {
    &kUnicodeToGB2312GLCID,
    nsUnicodeToGB2312GL::CreateInstance,
    "Unicode",
    "gb_2312-80"
  }
};

#define ARRAY_SIZE(_array)                                      \
     (sizeof(_array) / sizeof(_array[0]))

//----------------------------------------------------------------------------
// Class nsConverterFactory [declaration]

/**
 * General factory class for converter objects.
 * 
 * @created         24/Feb/1998
 * @author  Catalin Rotaru [CATA]
 */
class nsConverterFactory : public nsIFactory
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

  //--------------------------------------------------------------------------
  // Interface nsIFactory [declaration]

  NS_IMETHOD CreateInstance(nsISupports *aDelegate, const nsIID &aIID,
                            void **aResult);
  NS_IMETHOD LockFactory(PRBool aLock);
};

//----------------------------------------------------------------------------
// Class nsConverterModule [declaration]

class nsConverterModule : public nsIModule 
{
  NS_DECL_ISUPPORTS
  NS_DECL_NSIMODULE

private:

  PRBool mInitialized;

  void Shutdown();

public:

  nsConverterModule();

  virtual ~nsConverterModule();

  nsresult Initialize();

};

//----------------------------------------------------------------------------
// Global functions and data [implementation]

static nsConverterModule * gModule = NULL;

extern "C" NS_EXPORT nsresult NSGetModule(nsIComponentManager * compMgr,
                                          nsIFile* location,
                                          nsIModule** return_cobj)
{
  nsresult rv = NS_OK;

  NS_ENSURE_ARG_POINTER(return_cobj);
  NS_ENSURE_FALSE(gModule, NS_ERROR_FAILURE);

  // Create an initialize the module instance
  nsConverterModule * m = new nsConverterModule();
  if (!m) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  // Increase refcnt and store away nsIModule interface to m in return_cobj
  rv = m->QueryInterface(nsIModule::GetIID(), (void**)return_cobj);
  if (NS_FAILED(rv)) {
    delete m;
    m = nsnull;
  }
  gModule = m;                  // WARNING: Weak Reference
  return rv;
}

//----------------------------------------------------------------------------
// Class nsConverterFactory [implementation]

NS_IMPL_ISUPPORTS(nsConverterFactory, nsCOMTypeInfo<nsIFactory>::GetIID());

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

//----------------------------------------------------------------------------
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
  
  NS_ADDREF(t);  // Stabilize
  
  nsresult res = t->QueryInterface(aIID, aResult);
  
  NS_RELEASE(t); // Destabilize and avoid leaks. Avoid calling delete <interface pointer>  

  return res;
}

NS_IMETHODIMP nsConverterFactory::LockFactory(PRBool aLock)
{
  if (aLock) PR_AtomicIncrement(&g_LockCount);
  else PR_AtomicDecrement(&g_LockCount);

  return NS_OK;
}

//----------------------------------------------------------------------------
// Class nsConverterModule [implementation]

NS_IMPL_ISUPPORTS(nsConverterModule, nsIModule::GetIID())

nsConverterModule::nsConverterModule()
: mInitialized(PR_FALSE)
{
  NS_INIT_ISUPPORTS();
}

nsConverterModule::~nsConverterModule()
{
  Shutdown();
}

nsresult nsConverterModule::Initialize()
{
  return NS_OK;
}

void nsConverterModule::Shutdown()
{
}

//----------------------------------------------------------------------------
// Interface nsIModule [implementation]

NS_IMETHODIMP nsConverterModule::GetClassObject(nsIComponentManager *aCompMgr,
                                                const nsCID& aClass,
                                                const nsIID& aIID,
                                                void ** r_classObj)
{
  nsresult rv;

  // Defensive programming: Initialize *r_classObj in case of error below
  if (!r_classObj) {
    return NS_ERROR_INVALID_POINTER;
  }
  *r_classObj = NULL;

  if (!mInitialized) {
    rv = Initialize();
    if (NS_FAILED(rv)) {
      return rv;
    }
    mInitialized = PR_TRUE;
  }

  FactoryData * data;
  nsConverterFactory * fact;

  // XXX cache these factories
  for (PRUint32 i=0; i<ARRAY_SIZE(g_FactoryData); i++) {
    data = &(g_FactoryData[i]);
    if (aClass.Equals(*(data->mCID))) {
      fact = new nsConverterFactory(data);
      if (fact == NULL) {
        return NS_ERROR_OUT_OF_MEMORY;
      }
      rv = fact->QueryInterface(aIID, (void **) r_classObj);
      if (NS_FAILED(rv)) delete fact;

      return rv;
    }
  }

  return NS_ERROR_FACTORY_NOT_REGISTERED;
}

NS_IMETHODIMP nsConverterModule::RegisterSelf(nsIComponentManager *aCompMgr,
                                              nsIFile* aPath,
                                              const char* registryLocation,
                                              const char* componentType)
{
  nsresult res;
  PRUint32 i;
  nsIRegistry * registry = NULL;
  nsRegistryKey key;
  char buff[1024];

  // get the registry
  res = nsServiceManager::GetService(NS_REGISTRY_PROGID, 
      nsIRegistry::GetIID(), (nsISupports**)&registry);
  if (NS_FAILED(res)) goto done;

  // open the registry
  res = registry->OpenWellKnownRegistry(
      nsIRegistry::ApplicationComponentRegistry);
  if (NS_FAILED(res)) goto done;

  char name[128];
  char progid[128];
  char * cid_string;
  for (i=0; i<ARRAY_SIZE(g_FactoryData); i++) {
    if(0==PL_strcmp(g_FactoryData[i].mCharsetSrc,"Unicode"))
    {
       PL_strcpy(name, ENCODER_NAME_BASE);
       PL_strcat(name, g_FactoryData[i].mCharsetDest);
       PL_strcpy(progid, NS_UNICODEENCODER_PROGID_BASE);
       PL_strcat(progid, g_FactoryData[i].mCharsetDest);
    } else {
       PL_strcpy(name, DECODER_NAME_BASE);
       PL_strcat(name, g_FactoryData[i].mCharsetSrc);
       PL_strcpy(progid, NS_UNICODEDECODER_PROGID_BASE);
       PL_strcat(progid, g_FactoryData[i].mCharsetSrc);
    }
    // register component
    res = aCompMgr->RegisterComponentSpec(*(g_FactoryData[i].mCID), name, 
      progid, aPath, PR_TRUE, PR_TRUE);
    if(NS_FAILED(res) && (NS_ERROR_FACTORY_EXISTS != res)) goto done;

    // register component info
    // XXX take these KONSTANTS out of here; refine this code
    cid_string = g_FactoryData[i].mCID->ToString();
    sprintf(buff, "%s/%s", "software/netscape/intl/uconv", cid_string);
    nsCRT::free(cid_string);
    res = registry -> AddSubtree(nsIRegistry::Common, buff, &key);
    if (NS_FAILED(res)) goto done;
    res = registry -> SetString(key, "source", g_FactoryData[i].mCharsetSrc);
    if (NS_FAILED(res)) goto done;
    res = registry -> SetString(key, "destination", g_FactoryData[i].mCharsetDest);
    if (NS_FAILED(res)) goto done;
  }

done:
  if (registry != NULL) {
    registry -> Close();
    nsServiceManager::ReleaseService(NS_REGISTRY_PROGID, registry);
  }

  return res;
}

NS_IMETHODIMP nsConverterModule::UnregisterSelf(nsIComponentManager *aCompMgr,
                                                nsIFile* aPath,
                                                const char* registryLocation)
{
  // XXX also delete the stuff I added to the registry
  nsresult rv;

  for (PRUint32 i=0; i<ARRAY_SIZE(g_FactoryData); i++) {
    rv = aCompMgr->UnregisterComponentSpec(*(g_FactoryData[i].mCID), aPath);
  }

  return NS_OK;
}

NS_IMETHODIMP nsConverterModule::CanUnload(nsIComponentManager *aCompMgr, 
                                           PRBool *okToUnload)
{
  if (!okToUnload) {
    return NS_ERROR_INVALID_POINTER;
  }
  *okToUnload = (g_InstanceCount == 0 && g_LockCount == 0);
  return NS_OK;
}
