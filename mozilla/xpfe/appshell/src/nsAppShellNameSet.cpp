
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

#include "nsAppShellNameSet.h"
#include "nsIScriptContext.h"
#include "nsIScriptObjectOwner.h"
#include "nsIScriptNameSpaceManager.h"
#include "nsIDOMXPConnectFactory.h"
#include "nsAppShellCIDs.h" 


static NS_DEFINE_CID(kXPConnectFactoryCID,       NS_XPCONNECTFACTORY_CID);
static NS_DEFINE_IID(kIScriptObjectOwnerIID,     NS_ISCRIPTOBJECTOWNER_IID);

nsAppShellNameSet::nsAppShellNameSet()
{
  NS_INIT_REFCNT();
}

nsAppShellNameSet::~nsAppShellNameSet()
{
}

NS_IMPL_ISUPPORTS1(nsAppShellNameSet, nsIScriptExternalNameSet);




NS_IMETHODIMP
nsAppShellNameSet::InitializeClasses(nsIScriptContext* aScriptContext)
{
  nsresult rv;

  rv = NS_InitXPConnectFactoryClass(aScriptContext, nsnull);

  return rv;
}




NS_IMETHODIMP
nsAppShellNameSet::AddNameSet(nsIScriptContext* aScriptContext)
{
  nsresult rv;
  nsIScriptNameSpaceManager* manager;

  rv = aScriptContext->GetNameSpaceManager(&manager);
  if (NS_SUCCEEDED(rv)) {
    rv = manager->RegisterGlobalName(NS_ConvertASCIItoUCS2("XPComFactory"), 
				     kIScriptObjectOwnerIID,
                                     kXPConnectFactoryCID, 
                                     PR_FALSE);
    NS_RELEASE(manager);
  }
  return rv;
}

