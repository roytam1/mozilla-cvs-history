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
#include "nsIFactory.h"
#include "nscore.h"
#include "nsAppShellCIDs.h"
#include "nsICmdLineService.h"

/* extern the factory entry points... */
nsresult NS_NewAppShellServiceFactory(nsIFactory** aFactory);


static NS_DEFINE_IID(kAppShellServiceCID, NS_APPSHELL_SERVICE_CID);
static NS_DEFINE_IID(kCmdLineServiceCID,         NS_COMMANDLINE_SERVICE_CID);


#if defined(XP_MAC) && defined(MAC_STATIC)
extern "C" NS_APPSHELL nsresult NSGetFactory_APPSHELL_DLL(const nsCID& aClass, nsISupports* servMgr, nsIFactory** aFactory)
#else
extern "C" NS_APPSHELL nsresult NSGetFactory(const nsCID& aClass, nsISupports* servMgr, nsIFactory** aFactory)
#endif
{
  nsresult rv = NS_OK;

  if (nsnull == aFactory) {
    return NS_ERROR_NULL_POINTER;
  }

  if (aClass.Equals(kAppShellServiceCID)) {
    rv = NS_NewAppShellServiceFactory(aFactory);
  }
  else if (aClass.Equals(kCmdLineServiceCID)) {
     rv = NS_NewCmdLineServiceFactory(aFactory);
  }

  return rv;
}

