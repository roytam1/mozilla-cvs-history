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
#include "nsBasicStreamGenerator.h"

// Define the constructor function for the nsWalletlibService
NS_GENERIC_FACTORY_CONSTRUCTOR(nsWalletlibService)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsBasicStreamGenerator)


// The list of components we register
static nsModuleComponentInfo components[] = {
    { NS_WALLETSERVICE_CLASSNAME, NS_WALLETSERVICE_CID,
      NS_WALLETSERVICE_PROGID, nsWalletlibServiceConstructor },
    { NS_BASIC_STREAM_GENERATOR_CLASSNAME, NS_BASIC_STREAM_GENERATOR_CID,
      NS_BASIC_STREAM_GENERATOR_PROGID, nsBasicStreamGeneratorConstructor },
};

NS_IMPL_NSGETMODULE("nsWalletModule", components)
