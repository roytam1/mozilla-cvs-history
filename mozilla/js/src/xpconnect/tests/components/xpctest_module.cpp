/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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

/* module registration and factory code. */

#include "xpctest_private.h"

static NS_DEFINE_CID(kComponentManagerCID, NS_COMPONENTMANAGER_CID);
static NS_DEFINE_CID(kGenericFactoryCID, NS_GENERICFACTORY_CID);

extern "C" PR_IMPLEMENT(nsresult)
NSGetFactory(nsISupports* aServMgr,
             const nsCID &aClass,
             const char *aClassName,
             const char *aProgID,
             nsIFactory **aFactory)
{
    nsresult rv;
    NS_ASSERTION(aFactory != nsnull, "bad factory pointer");

    NS_WITH_SERVICE1(nsIComponentManager, compMgr,
                     aServMgr, kComponentManagerCID, &rv);
    if (NS_FAILED(rv)) return rv;

    nsIGenericFactory* factory;
    rv = compMgr->CreateInstance(kGenericFactoryCID, nsnull, 
                                 nsIGenericFactory::GetIID(), 
                                 (void**)&factory);
    if (NS_FAILED(rv)) return rv;

    // add more factories as 'if else's below...

    if(aClass.Equals(xpctest::GetEchoCID()))
        rv = factory->SetConstructor(xpctest::ConstructEcho);
    else if(aClass.Equals(xpctest::GetChildCID()))
        rv = factory->SetConstructor(xpctest::ConstructChild);
    else if(aClass.Equals(xpctest::GetNoisyCID()))
        rv = factory->SetConstructor(xpctest::ConstructNoisy);
    else if(aClass.Equals(xpctest::GetStringTestCID()))
        rv = factory->SetConstructor(xpctest::ConstructStringTest);
    else if(aClass.Equals(xpctest::GetOverloadedCID()))
        rv = factory->SetConstructor(xpctest::ConstructOverloaded);
    else if(aClass.Equals(xpctest::GetArrayTestCID()))
        rv = factory->SetConstructor(xpctest::ConstructArrayTest);
    else
    {
        NS_ASSERTION(0, "incorrectly registered");
        rv = NS_ERROR_NO_INTERFACE;
    }

    if (NS_FAILED(rv)) {
        NS_RELEASE(factory);
        return rv;
    }
    *aFactory = factory;
    return NS_OK;
}

extern "C" PR_IMPLEMENT(nsresult)
NSRegisterSelf(nsISupports* aServMgr , const char* aPath)
{
    nsresult rv;
#ifdef DEBUG
    printf("*** Register XPConnect test components\n");
#endif
    NS_WITH_SERVICE1(nsIComponentManager, compMgr,
                     aServMgr, kComponentManagerCID, &rv);
    if (NS_FAILED(rv)) return rv;

    rv = compMgr->RegisterComponent(xpctest::GetEchoCID(),
                                    "nsEcho", "nsEcho", aPath,
                                    PR_TRUE, PR_TRUE);

    rv = compMgr->RegisterComponent(xpctest::GetChildCID(),
                                    "nsChild", "nsChild", aPath,
                                    PR_TRUE, PR_TRUE);

    rv = compMgr->RegisterComponent(xpctest::GetNoisyCID(),
                                    "nsNoisy", "nsNoisy", aPath,
                                    PR_TRUE, PR_TRUE);

    rv = compMgr->RegisterComponent(xpctest::GetStringTestCID(),
                                    "nsStringTest", "nsStringTest", aPath,
                                    PR_TRUE, PR_TRUE);

    rv = compMgr->RegisterComponent(xpctest::GetOverloadedCID(),
                                    "nsOverloaded", "nsOverloaded", aPath,
                                    PR_TRUE, PR_TRUE);

    rv = compMgr->RegisterComponent(xpctest::GetArrayTestCID(),
                                    "nsArrayTest", "nsArrayTest", aPath,
                                    PR_TRUE, PR_TRUE);

    return rv;
}

extern "C" PR_IMPLEMENT(nsresult)
NSUnregisterSelf(nsISupports* aServMgr, const char* aPath)
{
    nsresult rv;
    NS_WITH_SERVICE1(nsIComponentManager, compMgr,
                     aServMgr, kComponentManagerCID, &rv);
    if (NS_FAILED(rv)) return rv;

    rv = compMgr->UnregisterComponent(xpctest::GetEchoCID(), aPath);
    rv = compMgr->UnregisterComponent(xpctest::GetChildCID(), aPath);
    rv = compMgr->UnregisterComponent(xpctest::GetNoisyCID(), aPath);
    rv = compMgr->UnregisterComponent(xpctest::GetStringTestCID(), aPath);
    rv = compMgr->UnregisterComponent(xpctest::GetOverloadedCID(), aPath);
    rv = compMgr->UnregisterComponent(xpctest::GetArrayTestCID(), aPath);

    return rv;
}

