/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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

#include "nsNeckoUtil.h"
#include "nsIIOService.h"
#include "nsIServiceManager.h"
#include "nsIChannel.h"

static NS_DEFINE_CID(kIOServiceCID, NS_IOSERVICE_CID);

nsresult
NS_NewURI(nsIURI* *result, const char* spec, nsIURI* baseURI)
{
    nsresult rv;
    NS_WITH_SERVICE(nsIIOService, serv, kIOServiceCID, &rv);
    if (NS_FAILED(rv)) return rv;
    
    rv = serv->NewURI(spec, baseURI, result);
    return rv;
}

nsresult
NS_NewURI(nsIURI* *result, const nsString& spec, nsIURI* baseURI)
{
    char* specStr = spec.ToNewCString();
    nsresult rv = NS_NewURI(result, specStr, baseURI);
    nsCRT::free(specStr);
    return rv;
}

nsresult
NS_OpenURI(nsIURI* uri, nsIInputStream* *result)
{
    nsresult rv;
    NS_WITH_SERVICE(nsIIOService, serv, kIOServiceCID, &rv);
    if (NS_FAILED(rv)) return rv;
    
    nsIChannel* channel;
    rv = serv->NewChannelFromURI("load", uri, nsnull, &channel);
    if (NS_FAILED(rv)) return rv;

    nsIInputStream* inStr;
    rv = channel->OpenInputStream(0, -1, &inStr);
    NS_RELEASE(channel);
    if (NS_FAILED(rv)) return rv;

    *result = inStr;
    return rv;
}

nsresult
NS_OpenURI(nsIURI* uri, nsIStreamListener* aConsumer)
{
    nsresult rv;
    NS_WITH_SERVICE(nsIIOService, serv, kIOServiceCID, &rv);
    if (NS_FAILED(rv)) return rv;
    
    nsIChannel* channel;
    rv = serv->NewChannelFromURI("load", uri, nsnull, &channel);
    if (NS_FAILED(rv)) return rv;

    rv = channel->AsyncRead(0, -1, uri, // uri used as context
                            nsnull,     // XXX need event queue of current thread
                            aConsumer);
    NS_RELEASE(channel);
    return rv;
}

nsresult
NS_MakeAbsoluteURI(const char* spec, nsIURI* baseURI, char* *result)
{
    nsresult rv;
    NS_WITH_SERVICE(nsIIOService, serv, kIOServiceCID, &rv);
    if (NS_FAILED(rv)) return rv;
    
    return serv->MakeAbsolute(spec, baseURI, result);
}

nsresult
NS_MakeAbsoluteURI(const nsString& spec, nsIURI* baseURI, nsString& result)
{
    char* specStr = spec.ToNewCString();
    char* resultStr;
    nsresult rv = NS_MakeAbsoluteURI(specStr, baseURI, &resultStr);
    nsCRT::free(specStr);
    if (NS_FAILED(rv)) return rv;

    result = resultStr;
    return rv;
}

////////////////////////////////////////////////////////////////////////////////
