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
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

#include "nsCOMPtr.h"
#include "nsIModule.h"
#include "nsIGenericFactory.h"

#include "nsIServiceManager.h"
#include "nsIComponentManager.h"
#include "nsIChromeRegistry.h"
#include "nscore.h"
#include "rdf.h"
#include "nsChromeProtocolHandler.h"
#include "nsChromeRegistry.h"


NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsChromeRegistry, Init)

// The list of components we register
static nsModuleComponentInfo components[] = 
{
    { "Chrome Registry", 
      NS_CHROMEREGISTRY_CID,
      "@mozilla.org/chrome/chrome-registry;1", 
      nsChromeRegistryConstructor
    },

    { "Chrome Protocol Handler", 
      NS_CHROMEPROTOCOLHANDLER_CID,
      NS_NETWORK_PROTOCOL_CONTRACTID_PREFIX "chrome", 
      nsChromeProtocolHandler::Create
    },
};

NS_IMPL_NSGETMODULE("nsChromeModule", components);

