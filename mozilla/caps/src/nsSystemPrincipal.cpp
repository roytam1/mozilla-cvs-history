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
 * Copyright (C) 1999-2000 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 * Norris Boyd
 */

/* The privileged system principal. */

#include "nsSystemPrincipal.h"
#include "nsIComponentManager.h"
#include "nsIServiceManager.h"
#include "nsIURL.h"
#include "nsCOMPtr.h"
#include "nsXPIDLString.h"


NS_IMPL_QUERY_INTERFACE1(nsSystemPrincipal, nsIPrincipal)

NSBASEPRINCIPALS_ADDREF(nsSystemPrincipal);
NSBASEPRINCIPALS_RELEASE(nsSystemPrincipal);


///////////////////////////////////////
// Methods implementing nsIPrincipal //
///////////////////////////////////////

NS_IMETHODIMP
nsSystemPrincipal::ToString(char **result)
{
    nsAutoString buf("[System]");
    *result = buf.ToNewCString();
    return *result ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

NS_IMETHODIMP
nsSystemPrincipal::ToUserVisibleString(char **result)
{
    return ToString(result);
}

NS_IMETHODIMP
nsSystemPrincipal::Equals(nsIPrincipal *other, PRBool *result)
{
    *result = (other == this);
    return NS_OK;
}

NS_IMETHODIMP
nsSystemPrincipal::HashValue(PRUint32 *result)
{
    *result = (PRUint32) this;
    return NS_OK;
}

NS_IMETHODIMP 
nsSystemPrincipal::CanEnableCapability(const char *capability, 
                                       PRInt16 *result)
{
    // System principal can enable all capabilities.
    *result = nsIPrincipal::ENABLE_GRANTED;
    return NS_OK;
}

NS_IMETHODIMP 
nsSystemPrincipal::SetCanEnableCapability(const char *capability, 
                                          PRInt16 canEnable)
{
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP 
nsSystemPrincipal::IsCapabilityEnabled(const char *capability, 
                                       void *annotation, 
                                       PRBool *result)
{
    *result = PR_TRUE;
    return NS_OK;
}

NS_IMETHODIMP 
nsSystemPrincipal::EnableCapability(const char *capability, void **annotation)
{
    return NS_OK;
}

NS_IMETHODIMP 
nsSystemPrincipal::RevertCapability(const char *capability, void **annotation)
{
    return NS_OK;
}

NS_IMETHODIMP 
nsSystemPrincipal::DisableCapability(const char *capability, void **annotation) 
{
    // Can't disable the capabilities of the system principal.
    // XXX might be handy to be able to do so!
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsSystemPrincipal::Save(nsSupportsHashtable* aPrincipals, nsIPref* prefs)
{
    // The system principal should never be streamed out
    return NS_ERROR_FAILURE; 
}


/////////////////////////////////////////////
// Constructor, Destructor, initialization //
/////////////////////////////////////////////

nsSystemPrincipal::nsSystemPrincipal()
{
    NS_INIT_ISUPPORTS();
}

NS_IMETHODIMP
nsSystemPrincipal::Init()
{
    char *codebase = nsCRT::strdup("[System Principal]");
    if (!codebase)
        return NS_ERROR_OUT_OF_MEMORY;
    if (NS_FAILED(mJSPrincipals.Init(codebase))) 
        return NS_ERROR_FAILURE;
    return NS_OK;
}

nsSystemPrincipal::~nsSystemPrincipal(void)
{
}
