/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
 * Communications Corporation.	Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */


#include "nsCOMPtr.h"
#include "nsIModule.h"
#include "nsIGenericFactory.h"
#include "nsWalletService.h"

// Define the constructor function for the nsWalletlibService
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsWalletlibService, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsSingleSignOnPrompt, Init)

// The list of components we register
static nsModuleComponentInfo components[] = {
    { NS_WALLETSERVICE_CLASSNAME, NS_WALLETSERVICE_CID,
      NS_WALLETSERVICE_CONTRACTID, nsWalletlibServiceConstructor },
    { NS_SINGLESIGNONPROMPT_CLASSNAME, NS_SINGLESIGNONPROMPT_CID,
      NS_SINGLESIGNONPROMPT_CONTRACTID, nsSingleSignOnPromptConstructor }
};

NS_IMPL_NSGETMODULE("nsWalletModule", components)

#ifdef XP_WIN32
  //in addition to returning a version number for this module,
  //this also provides a convenient hook for the preloader
  //to keep (some if not all) of the module resident.
extern "C" __declspec(dllexport) float GetVersionNumber(void) {
  return 1.0;
}
#endif

