/* -*- Mode: IDL; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
 * The Original Code is the Mozilla browser.
 * 
 * The Initial Developer of the Original Code is Netscape
 * Communications, Inc.  Portions created by Netscape are
 * Copyright (C) 1999, Mozilla.  All Rights Reserved.
 * 
 * Contributor(s):
 *   Travis Bogard <travis@netscape.com>
 */

//#include "nsIComponentManager.h"

#include "nsWebBrowserSetup.h"

//*****************************************************************************
//***    nsWebBrowserSetup: Object Management
//*****************************************************************************

nsWebBrowserSetup::nsWebBrowserSetup()
{
	NS_INIT_REFCNT();
}

nsWebBrowserSetup::~nsWebBrowserSetup()
{
}

NS_IMETHODIMP nsWebBrowserSetup::Create(nsISupports* aOuter, const nsIID& aIID, 
	void** ppv)
{
	NS_ENSURE_ARG_POINTER(ppv);
	NS_ENSURE_NO_AGGREGATION(aOuter);

	nsWebBrowserSetup* setup = new  nsWebBrowserSetup();
	NS_ENSURE(setup, NS_ERROR_OUT_OF_MEMORY);

	NS_ADDREF(setup);
	nsresult rv = setup->QueryInterface(aIID, ppv);
	NS_RELEASE(setup);
	return rv;
}

//*****************************************************************************
// nsWebBrowser::nsISupports
//*****************************************************************************   

NS_IMPL_ISUPPORTS1(nsWebBrowserSetup, nsIWebBrowserSetup)

//*****************************************************************************
// nsWebBrowserSetup::nsIWebBrowserSetup
//*****************************************************************************   

NS_IMETHODIMP nsWebBrowserSetup::ChangeCurrentUser(const PRUnichar* newUser)
{
   NS_ENSURE_ARG(newUser);

   //XXX
   //Load up the Profile Manager and Change the current user.

   //XXX Implement
   return NS_ERROR_FAILURE;
}

NS_IMETHODIMP nsWebBrowserSetup::GetUserList(nsIEnumerator** enumerator)
{
   NS_ENSURE_ARG_POINTER(enumerator);

   //XXX
   // Load up the profile manager and get a list of users.  Create an 
   // enumeration object to stuff these users into.  Return this object.

   //XXX Implement
   return NS_ERROR_FAILURE;
}