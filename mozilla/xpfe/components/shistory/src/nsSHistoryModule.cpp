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
#include "nsISHEntry.h"
#include "nsISHTransaction.h"

extern NS_IMETHODIMP NS_NewSHTransaction(nsISupports * aOuter, REFNSIID aIID, void** aResult);
extern NS_IMETHODIMP NS_NewSHEntry(nsISupports * aOuter, REFNSIID aIID, void** aResult);
 

///////////////////////////////////////////////////////////////////////////////
// Module implementation for the history library

static nsModuleComponentInfo gSHistoryModuleInfo[] = {
  { "nsSHEntry",
    NS_SHENTRY_CID,
    NS_SHENTRY_PROGID, 
    NS_NewSHEntry },
  { "nsSHTransaction",
    NS_SHTRANSACTION_CID,
    NS_SHTRANSACTION_PROGID, 
    NS_NewSHTransaction }
};


NS_IMPL_NSGETMODULE("history", gSHistoryModuleInfo)
