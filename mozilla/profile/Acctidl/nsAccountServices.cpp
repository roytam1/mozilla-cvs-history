/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */


/*

  A sample of XPConnect. This file contains an implementation of
  nsIAccountServices.

*/
#include "nscore.h"
#include "nsIAccountServices.h"
#include "nsIAllocator.h"
#include "plstr.h"
#include "stdio.h"
#include "nsIAccount.h"
#include "nsIComponentManager.h"
#include "nsIServiceManager.h"


static NS_DEFINE_CID(kAccountCID, NS_Account_CID);

class AccountServicesImpl : public nsIAccountServices
{
public:
    AccountServicesImpl();
    virtual ~AccountServicesImpl();

    // nsISupports interface
    NS_DECL_ISUPPORTS

    // nsIAccount interface
	NS_IMETHOD SetDialerConfig(const char* aValue);

private:
	nsIAccount *mAccount;
};

////////////////////////////////////////////////////////////////////////

nsresult
NS_NewAccountServices(nsIAccountServices** aAccount)
{
    NS_PRECONDITION(aAccount != nsnull, "null ptr");
    if (! aAccount)
        return NS_ERROR_NULL_POINTER;

    *aAccount = new AccountServicesImpl();
    if (! *aAccount)
        return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(*aAccount);
    return NS_OK;
}

////////////////////////////////////////////////////////////////////////


AccountServicesImpl::AccountServicesImpl()
:	mAccount(nsnull)
{
    NS_INIT_REFCNT();

	nsresult rv = nsServiceManager::GetService(kAccountCID, 
                                    nsIAccount::GetIID(), 
                                    (nsISupports **)&mAccount);
}


AccountServicesImpl::~AccountServicesImpl()
{
}



NS_IMPL_ISUPPORTS(AccountServicesImpl, nsIAccountServices::GetIID());


NS_IMETHODIMP
AccountServicesImpl::SetDialerConfig(const char* aPrefix)
{
    NS_PRECONDITION(aPrefix != nsnull, "null ptr");
    if (! aPrefix)
        return NS_ERROR_NULL_POINTER;

    printf("Hey : You are here..........%s \n", aPrefix);
	mAccount->SetDialerConfig((char *)aPrefix);
    return NS_OK;
}

