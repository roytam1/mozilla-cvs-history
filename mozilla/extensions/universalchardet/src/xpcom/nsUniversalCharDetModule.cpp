/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ----- BEGIN LICENSE BLOCK -----
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications Corporation.
 * Portions created by Netscape Communications Corporation are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either of the GNU General Public License Version 2 or later (the "GPL"),
 * or the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the LGPL or the GPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ----- END LICENSE BLOCK ----- */

#define NS_IMPL_IDS
#include "nsICharsetAlias.h"
#undef NS_IMPL_IDS
#include "nsCOMPtr.h"
#include "nsIModule.h"

#include "nspr.h"
#include "nsString.h"
#include "pratom.h"
#include "nsUniversalCharDetDll.h"
#include "nsISupports.h"
#include "nsIRegistry.h"
#include "nsIComponentManager.h"
#include "nsIFactory.h"
#include "nsIServiceManager.h"
#include "nsICharsetDetector.h"
#include "nsIStringCharsetDetector.h"
#include "nsIGenericFactory.h"

#include "nsUniversalDetector.h"

static NS_DEFINE_CID(kUniversalDetectorCID,  NS_UNIVERSAL_DETECTOR_CID);
static NS_DEFINE_CID(kUniversalStringDetectorCID,  NS_UNIVERSAL_STRING_DETECTOR_CID);

NS_GENERIC_FACTORY_CONSTRUCTOR(nsUniversalXPCOMDetector);
NS_GENERIC_FACTORY_CONSTRUCTOR(nsUniversalXPCOMStringDetector);

//----------------------------------------
static NS_METHOD nsUniversalCharDetectorRegistrationProc(nsIComponentManager *aCompMgr,
                                          nsIFile *aPath,
                                          const char *registryLocation,
                                          const char *componentType,
                                          const nsModuleComponentInfo *info)
{
  nsRegistryKey key;
  nsresult rv = NS_OK;
  nsCOMPtr<nsIRegistry> registry = do_GetService(NS_REGISTRY_CONTRACTID, &rv);
  if (NS_FAILED(rv)) {
    return rv;
  }

  // open the registry
  rv = registry->OpenWellKnownRegistry(
    nsIRegistry::ApplicationComponentRegistry);
  if (NS_FAILED(rv)) {
    return rv;
  }

  rv = registry -> AddSubtree(nsIRegistry::Common, 
                              NS_CHARSET_DETECTOR_REG_BASE "universal_charset_detector" ,&key);
  if (NS_SUCCEEDED(rv)) {
    rv = registry-> SetStringUTF8(key, "type", "universal_charset_detector");
    rv = registry-> SetStringUTF8(key, "defaultEnglishText", "UniversalDetector");
  }

  return rv;
}

// Component Table
static nsModuleComponentInfo components[] = 
{
   { "Universal Charset Detector", NS_UNIVERSAL_DETECTOR_CID, 
    NS_CHARSET_DETECTOR_CONTRACTID_BASE "universal_charset_detector", nsUniversalXPCOMDetectorConstructor, 
    nsUniversalCharDetectorRegistrationProc, NULL},
   { "Universal String Charset Detector", NS_UNIVERSAL_STRING_DETECTOR_CID, 
    NS_STRCDETECTOR_CONTRACTID_BASE "universal_charset_detector", nsUniversalXPCOMStringDetectorConstructor, 
    NULL, NULL}
};

NS_IMPL_NSGETMODULE(nsUniversalCharDetModule, components)
