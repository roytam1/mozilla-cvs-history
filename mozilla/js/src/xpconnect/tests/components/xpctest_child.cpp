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

/* implement nsIChild for testing. */

#include "xpctest_private.h"

#define USE_MI 0

#if USE_MI
/***************************************************************************/

class xpctestOther : public nsIXPCTestOther
{
public:
    NS_DECL_ISUPPORTS

    NS_IMETHOD method3(PRInt16 i, PRInt16 j, PRInt16 k);

    xpctestOther();
};

class xpctestChild : public nsIXPCTestChild, public xpctestOther
{
public:
    NS_DECL_ISUPPORTS_INHERITED

    NS_IMETHOD method1(PRInt16 i);
    NS_IMETHOD method1a(nsIXPCTestParent *foo);
    NS_IMETHOD method2(PRInt16 i, PRInt16 j);

    xpctestChild();
};

static NS_DEFINE_IID(kxpctestOtherIID, NS_IXPCTESTOTHER_IID);
NS_IMPL_ISUPPORTS(xpctestOther, kxpctestOtherIID);

xpctestOther::xpctestOther()
{
    NS_INIT_REFCNT();
    NS_ADDREF_THIS();
}

NS_IMETHODIMP xpctestOther::method3(PRInt16 i, PRInt16 j, PRInt16 k)
{
    printf("method3 called on inherited other\n");
    return NS_OK;
}

NS_IMPL_ISUPPORTS_INHERITED(xpctestChild, xpctestOther, nsIXPCTestChild)

xpctestChild::xpctestChild()
{
}

NS_IMETHODIMP xpctestChild::method1(PRInt16 i)
{
    printf("method1 called on child\n");
    return NS_OK;
}

NS_IMETHODIMP xpctestChild::method1a(nsIXPCTestParent *foo)
{
    printf("method1a called on child\n");
    return NS_OK;
}


NS_IMETHODIMP xpctestChild::method2(PRInt16 i, PRInt16 j)
{
    printf("method2 called on child\n");
    return NS_OK;
}

#if 0
class xpctestParent : public nsIXPCTestParent
{
public:
    NS_DECL_ISUPPORTS

    NS_IMETHOD method1(PRInt16 i);
    NS_IMETHOD method1a(nsIXPCTestParent *foo);
    xpctestParent();
};


class xpctestChild : public xpctestParent, public nsIXPCTestChild
{
public:
    NS_DECL_ISUPPORTS

    NS_IMETHOD method2(PRInt16 i, PRInt16 j);

    xpctestChild();
};

NS_IMETHODIMP xpctestParent::method1(PRInt16 i)
{
    printf("method1 called on parent via child\n");
    return NS_OK;
}
NS_IMETHODIMP xpctestParent::method1a(nsIXPCTestParent *foo)
{
    printf("method1a called on parent via child\n");
    return NS_OK;
}

#endif

/***************************************************************************/
#else
/***************************************************************************/
class xpctestChild : public nsIXPCTestChild
{
public:
    NS_DECL_ISUPPORTS

    NS_IMETHOD method1(PRInt16 i);
    NS_IMETHOD method1a(nsIXPCTestParent *foo);
    NS_IMETHOD method2(PRInt16 i, PRInt16 j);

    xpctestChild();
};

NS_IMPL_ADDREF(xpctestChild);
NS_IMPL_RELEASE(xpctestChild);

NS_IMETHODIMP
xpctestChild::QueryInterface(REFNSIID iid, void** result)
{
    if (! result)
        return NS_ERROR_NULL_POINTER;

    if (iid.Equals(nsIXPCTestChild::GetIID()) ||
        iid.Equals(nsIXPCTestParent::GetIID()) ||
        iid.Equals(nsISupports::GetIID())) {
        *result = NS_STATIC_CAST(nsIXPCTestChild*, this);
        NS_ADDREF(this);
        return NS_OK;
    }
    else {
        *result = nsnull;
        return NS_NOINTERFACE;
    }
}

xpctestChild::xpctestChild()
{
    NS_INIT_REFCNT();
    NS_ADDREF_THIS();
}

NS_IMETHODIMP xpctestChild::method1(PRInt16 i)
{
    printf("method1 called on child\n");
    return NS_OK;
}

NS_IMETHODIMP xpctestChild::method1a(nsIXPCTestParent *foo)
{
    printf("method1a called on child\n");
    return NS_OK;
}

NS_IMETHODIMP xpctestChild::method2(PRInt16 i, PRInt16 j)
{
    printf("method2 called on child\n");
    return NS_OK;
}
#endif
/***************************************************************************/

// static
NS_IMETHODIMP
xpctest::ConstructChild(nsISupports *aOuter, REFNSIID aIID, void **aResult)
{
    nsresult rv;
    NS_ASSERTION(aOuter == nsnull, "no aggregation");
    xpctestChild* obj = new xpctestChild();
    rv = obj->QueryInterface(aIID, aResult);
    NS_ASSERTION(NS_SUCCEEDED(rv), "unable to find correct interface");
    NS_RELEASE(obj);
    return rv;
}
/***************************************************************************/




