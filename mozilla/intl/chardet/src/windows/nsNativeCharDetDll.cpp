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

#define NS_IMPL_IDS
#include "nsICharsetAlias.h"
#undef NS_IMPL_IDS

#include "pratom.h"
#include "nsNativeCharDetDll.h"
#include "nsISupports.h"
#include "nsIComponentManager.h"
#include "nsIFactory.h"
#include "nsIServiceManager.h"
#include "nsCOMPtr.h"
#include "nsICharsetDetector.h"
#include "nsIStringCharsetDetector.h"
#include "nsNativeDetectors.h"


static NS_DEFINE_CID(kJANativeDetectorCID,         NS_JA_NATIVE_DETECTOR_CID);
static NS_DEFINE_CID(kJANativeStringDetectorCID,   NS_JA_NATIVE_STRING_DETECTOR_CID);
static NS_DEFINE_CID(kKONativeDetectorCID,         NS_KO_NATIVE_DETECTOR_CID);
static NS_DEFINE_CID(kKONativeStringDetectorCID,   NS_KO_NATIVE_STRING_DETECTOR_CID);
static NS_DEFINE_CID(kComponentManagerCID, NS_COMPONENTMANAGER_CID);
static NS_DEFINE_IID(kFactoryIID, NS_IFACTORY_IID);

PRInt32 g_InstanceCount = 0;
PRInt32 g_LockCount = 0;


extern "C" NS_EXPORT nsresult NSGetFactory(nsISupports* aServMgr,
                                           const nsCID &aClass,
                                           const char *aClassName,
                                           const char *aContractID,
                                           nsIFactory **aFactory)
{
  if (aFactory == NULL) {
    return NS_ERROR_NULL_POINTER;
  }

  nsIFactory *factory = nsnull;
  if (aClass.Equals(kJANativeDetectorCID)) {
    ;
    //bug#13844 disable this until find out the reason of the freeze
    //factory = NEW_JA_NATIVEDETECTOR_FACTORY();
  } else if (aClass.Equals(kJANativeStringDetectorCID)) {
    factory = NEW_JA_STRING_NATIVEDETECTOR_FACTORY();
  } else if (aClass.Equals(kKONativeDetectorCID)) {
    ;factory = NEW_KO_NATIVEDETECTOR_FACTORY();
  } else if (aClass.Equals(kKONativeStringDetectorCID)) {
    factory = NEW_KO_STRING_NATIVEDETECTOR_FACTORY();
  }

  if(nsnull != factory) {
    nsresult res = factory->QueryInterface(kFactoryIID, (void **) aFactory);
    if (NS_FAILED(res)) {
      *aFactory = NULL;
      delete factory;
    }
    return res;
  }
  return NS_NOINTERFACE;
}

extern "C" NS_EXPORT PRBool NSCanUnload(nsISupports* aServMgr) {
  return PRBool(g_InstanceCount == 0 && g_LockCount == 0);
}
extern "C" NS_EXPORT nsresult NSRegisterSelf(nsISupports* aServMgr, const char *path)
{
  nsresult rv;

  nsCOMPtr<nsIServiceManager> servMgr(do_QueryInterface(aServMgr, &rv));
  if (NS_FAILED(rv)) return rv;

  nsCOMPtr<nsIComponentManager> compMgr = do_GetService(kComponentManagerCID, &rv);
  if (NS_FAILED(rv)) return rv;

  rv = compMgr->RegisterComponent(kJANativeDetectorCID, 
                                  "Native JA Charset Detector", 
                                  NS_CHARSET_DETECTOR_CONTRACTID_BASE "jams", 
                                  path,
                                  PR_TRUE, PR_TRUE);

  rv = compMgr->RegisterComponent(kJANativeStringDetectorCID, 
                                  "Native JA String Charset Detector", 
                                  NS_STRCDETECTOR_CONTRACTID_BASE "jams", 
                                  path,
                                  PR_TRUE, PR_TRUE);

  rv = compMgr->RegisterComponent(kKONativeDetectorCID, 
                                  "Native KO Charset Detector", 
                                  NS_CHARSET_DETECTOR_CONTRACTID_BASE "koms", 
                                  path,
                                  PR_TRUE, PR_TRUE);

  rv = compMgr->RegisterComponent(kKONativeStringDetectorCID, 
                                  "Native KO String Charset Detector", 
                                  NS_STRCDETECTOR_CONTRACTID_BASE "koms", 
                                  path,
                                  PR_TRUE, PR_TRUE);
  return rv;
}

extern "C" NS_EXPORT nsresult NSUnregisterSelf(nsISupports* aServMgr, const char *path)
{
  nsresult rv;

  nsCOMPtr<nsIServiceManager> servMgr(do_QueryInterface(aServMgr, &rv));
  if (NS_FAILED(rv)) return rv;

  nsCOMPtr<nsIComponentManager> compMgr = do_GetService(kComponentManagerCID, &rv);
  if (NS_FAILED(rv)) return rv;

  rv = compMgr->UnregisterComponent(kJANativeDetectorCID, path);
  rv = compMgr->UnregisterComponent(kJANativeStringDetectorCID, path);
  rv = compMgr->UnregisterComponent(kKONativeDetectorCID, path);
  rv = compMgr->UnregisterComponent(kKONativeStringDetectorCID, path);

  return rv;
}

