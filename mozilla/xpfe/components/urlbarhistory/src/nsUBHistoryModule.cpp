/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
 * Contributor(s):  Radha Kulkarni radha@netscape.com
 */

#include "nsIModule.h"
#include "nsIGenericFactory.h"
#include "nsIComponentManager.h"
#include "nsIServiceManager.h"
#include "nsIUrlbarHistory.h"

#include "nsUrlbarHistory.h"

// Factory Constructors

NS_GENERIC_FACTORY_CONSTRUCTOR(nsUrlbarHistory)

///////////////////////////////////////////////////////////////////////////////
// Module implementation for the history library

static nsModuleComponentInfo gUBHistoryModuleInfo[] = 
{
   { "nsUrlbarHistory", NS_URLBARHISTORY_CID,
      NS_URLBARHISTORY_CONTRACTID, nsUrlbarHistoryConstructor },
   { "nsUrlbarHistory", NS_URLBARHISTORY_CID,
      NS_URLBARAUTOCOMPLETE_CONTRACTID, nsUrlbarHistoryConstructor }
};

NS_IMPL_NSGETMODULE(Urlbar_History_Module, gUBHistoryModuleInfo)
