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

#include "nsIFactory.h"
#include "nsIComponentManager.h"
#include "nsIServiceManager.h"
#include "nsNetService.h"
#include "nsFileTransportService.h"
#include "nsSocketTransportService.h"
#include "nscore.h"
#include "nsStandardUrl.h"

static NS_DEFINE_CID(kComponentManagerCID,      NS_COMPONENTMANAGER_CID);
static NS_DEFINE_CID(kNetServiceCID,            NS_NETSERVICE_CID);
static NS_DEFINE_CID(kFileTransportServiceCID,  NS_FILETRANSPORTSERVICE_CID);
static NS_DEFINE_CID(kStandardUrlCID,           NS_STANDARDURL_CID);
static NS_DEFINE_CID(kSocketTransportServiceCID, NS_SOCKETTRANSPORTSERVICE_CID);

////////////////////////////////////////////////////////////////////////////////

class nsNetFactory : public nsIFactory
{
public:
    nsNetFactory(const nsCID &aClass);

    // nsISupports methods
    NS_DECL_ISUPPORTS

    // nsIFactory methods
    NS_IMETHOD CreateInstance(nsISupports *aOuter,
                              const nsIID &aIID,
                              void **aResult);

    NS_IMETHOD LockFactory(PRBool aLock);

protected:
    virtual ~nsNetFactory();

protected:
    nsCID       mClassID;
};

////////////////////////////////////////////////////////////////////////

nsNetFactory::nsNetFactory(const nsCID &aClass)
    : mClassID(aClass)
{
    NS_INIT_REFCNT();
}

nsNetFactory::~nsNetFactory()
{
    NS_ASSERTION(mRefCnt == 0, "non-zero refcnt at destruction");
}

NS_IMPL_ISUPPORTS(nsNetFactory, nsIFactory::GetIID());

NS_IMETHODIMP
nsNetFactory::CreateInstance(nsISupports *aOuter,
                             const nsIID &aIID,
                             void **aResult)
{
    nsresult rv;

    if (aResult == nsnull)
        return NS_ERROR_NULL_POINTER;

    nsISupports *inst = nsnull;
    if (mClassID.Equals(kNetServiceCID)) {
        if (aOuter) return NS_ERROR_NO_AGGREGATION;

        nsNetService* net = new nsNetService();
        if (net == nsnull)
            return NS_ERROR_OUT_OF_MEMORY;
        rv = net->Init();
        if (NS_FAILED(rv)) {
            delete net;
            return rv;
        }
        inst = net;
    }
    else if (mClassID.Equals(kFileTransportServiceCID)) {
        if (aOuter) return NS_ERROR_NO_AGGREGATION;

        nsFileTransportService* trans = new nsFileTransportService();
        if (trans == nsnull)
            return NS_ERROR_OUT_OF_MEMORY;
        rv = trans->Init();
        if (NS_FAILED(rv)) {
            delete trans;
            return rv;
        }
        inst = trans;
    }
    else if (mClassID.Equals(kSocketTransportServiceCID)) {
        nsSocketTransportService* trans = new nsSocketTransportService();
        if (trans == nsnull)
            return NS_ERROR_OUT_OF_MEMORY;
        rv = trans->Init();
        if (NS_FAILED(rv)) {
            delete trans;
            return rv;
        }
        inst = NS_STATIC_CAST(nsISocketTransportService*, trans); 
    }
    else if (mClassID.Equals(kStandardUrlCID)) {
        nsStandardUrl* url = new nsStandardUrl(aOuter);
        if (url == nsnull)
            return NS_ERROR_OUT_OF_MEMORY;
        inst = NS_STATIC_CAST(nsIUrl*, url);
    }
    else {
        return NS_ERROR_NO_INTERFACE;
    }

    NS_ADDREF(inst);
    *aResult = inst;
    return rv;
}

nsresult nsNetFactory::LockFactory(PRBool aLock)
{
    // Not implemented in simplest case.
    return NS_OK;
}

////////////////////////////////////////////////////////////////////////////////

// return the proper factory to the caller
extern "C" PR_IMPLEMENT(nsresult)
NSGetFactory(nsISupports* aServMgr,
             const nsCID &aClass,
             const char *aClassName,
             const char *aProgID,
             nsIFactory **aFactory)
{
    if (aFactory == nsnull)
        return NS_ERROR_NULL_POINTER;

    nsNetFactory* factory = new nsNetFactory(aClass);
    if (factory == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(factory);
    *aFactory = factory;
    return NS_OK;
}

extern "C" PR_IMPLEMENT(nsresult)
NSRegisterSelf(nsISupports* aServMgr , const char* aPath)
{
    nsresult rv;

    NS_WITH_SERVICE1(nsIComponentManager, compMgr, aServMgr, kComponentManagerCID, &rv);
    if (NS_FAILED(rv)) return rv;

    rv = compMgr->RegisterComponent(kNetServiceCID,  
                                    "Network Service",
                                    "component://netscape/network/net-service",
                                    aPath, PR_TRUE, PR_TRUE);
    if (NS_FAILED(rv)) return rv;

    rv = compMgr->RegisterComponent(kFileTransportServiceCID, 
                                    "File Transport Service",
                                    "component://netscape/network/file-transport-service",
                                    aPath, PR_TRUE, PR_TRUE);
    if (NS_FAILED(rv)) return rv;

    rv = compMgr->RegisterComponent(kSocketTransportServiceCID, 
                                    "Socket Transport Service",
                                    "component://netscape/network/socket-transport-service",
                                    aPath, PR_TRUE, PR_TRUE);
    if (NS_FAILED(rv)) return rv;;

    rv = compMgr->RegisterComponent(kStandardUrlCID, 
                                    "Standard URL Implementation",
                                    "component://netscape/network/standard-url",
                                    aPath, PR_TRUE, PR_TRUE);
    return rv;
}

extern "C" PR_IMPLEMENT(nsresult)
NSUnregisterSelf(nsISupports* aServMgr, const char* aPath)
{
    nsresult rv;

    NS_WITH_SERVICE1(nsIComponentManager, compMgr, aServMgr, kComponentManagerCID, &rv);
    if (NS_FAILED(rv)) return rv;

    rv = compMgr->UnregisterComponent(kNetServiceCID, aPath);
    if (NS_FAILED(rv)) return rv;

    rv = compMgr->UnregisterComponent(kFileTransportServiceCID, aPath);
    if (NS_FAILED(rv)) return rv;

    rv = compMgr->UnregisterComponent(kSocketTransportServiceCID, aPath);
    if (NS_FAILED(rv)) return rv;;

    rv = compMgr->UnregisterComponent(kStandardUrlCID, aPath);
    return rv;
}

////////////////////////////////////////////////////////////////////////////////
