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

#include "nsIOService.h"
#include "nsIProtocolHandler.h"
#include "nscore.h"
#include "nsString2.h"
#include "nsIServiceManager.h"
#include "nsIEventQueueService.h"
#include "nsIFileTransportService.h"
#include "nsIURI.h"
#include "nsIStreamListener.h"
#include "nsCOMPtr.h"
#include "prprf.h"
#include "prmem.h"      // for PR_Malloc
#include <ctype.h>      // for isalpha
#include "nsIFileProtocolHandler.h"     // for NewChannelFromNativePath
#include "nsLoadGroup.h"
#include "nsIFileChannel.h"
#include "nsInputStreamChannel.h"
#include "nsXPIDLString.h" 

static NS_DEFINE_CID(kFileTransportService, NS_FILETRANSPORTSERVICE_CID);
static NS_DEFINE_CID(kEventQueueService, NS_EVENTQUEUESERVICE_CID);

////////////////////////////////////////////////////////////////////////////////

nsIOService::nsIOService()
{
    NS_INIT_REFCNT();
}

nsresult
nsIOService::Init()
{
    return NS_OK;
}

nsIOService::~nsIOService()
{
}

NS_METHOD
nsIOService::Create(nsISupports *aOuter, REFNSIID aIID, void **aResult)
{
    if (aOuter)
        return NS_ERROR_NO_AGGREGATION;

    nsIOService* _ios = new nsIOService();
    if (_ios == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(_ios);
    nsresult rv = _ios->QueryInterface(aIID, aResult);
    NS_RELEASE(_ios);
    return rv;
}

NS_IMPL_ISUPPORTS(nsIOService, nsCOMTypeInfo<nsIIOService>::GetIID());

////////////////////////////////////////////////////////////////////////////////

#define MAX_SCHEME_LENGTH       64      // XXX big enough?

#define MAX_NET_PROGID_LENGTH   (MAX_SCHEME_LENGTH + NS_NETWORK_PROTOCOL_PROGID_PREFIX_LENGTH + 1)

NS_IMETHODIMP
nsIOService::GetProtocolHandler(const char* scheme, nsIProtocolHandler* *result)
{
    nsresult rv;

    NS_ASSERTION(NS_NETWORK_PROTOCOL_PROGID_PREFIX_LENGTH
                 == nsCRT::strlen(NS_NETWORK_PROTOCOL_PROGID_PREFIX),
                 "need to fix NS_NETWORK_PROTOCOL_PROGID_PREFIX_LENGTH");

    // XXX we may want to speed this up by introducing our own protocol 
    // scheme -> protocol handler mapping, avoiding the string manipulation
    // and service manager stuff

    char buf[MAX_NET_PROGID_LENGTH];
    nsAutoString2 progID(NS_NETWORK_PROTOCOL_PROGID_PREFIX);
    progID += scheme;
    progID.ToCString(buf, MAX_NET_PROGID_LENGTH);

    NS_WITH_SERVICE(nsIProtocolHandler, handler, buf, &rv);
    if (NS_FAILED(rv)) return rv;

    *result = handler;
	NS_ADDREF(handler);
    return NS_OK;
}

static nsresult
GetScheme(const char* inURI, char* *scheme)
{
    // search for something up to a colon, and call it the scheme
    NS_ASSERTION(inURI, "null pointer");
    if (!inURI) return NS_ERROR_NULL_POINTER;
    char c;
    const char* URI = inURI;
    PRUint32 i = 0;
    PRUint32 length = 0;
    // skip leading white space
    while (isspace(*URI))
        URI++;
    while ((c = *URI++) != '\0') {
        if (c == ':') {
            char* newScheme = (char *)PR_Malloc(length+1);
            if (newScheme == nsnull)
                return NS_ERROR_OUT_OF_MEMORY;

            nsCRT::memcpy(newScheme, inURI, length);
            newScheme[length] = '\0';
            *scheme = newScheme;
            return NS_OK;
        }
        else if (isalpha(c)) {
            length++;
        }
        else 
            break;
    }
    return NS_ERROR_MALFORMED_URI;
}

NS_IMETHODIMP
nsIOService::NewURI(const char* aSpec, nsIURI* aBaseURI,
                    nsIURI* *result)
{
    nsresult rv;
    char* scheme;
    rv = GetScheme(aSpec, &scheme);
    if (NS_FAILED(rv)) {
        if (aBaseURI)
            rv = aBaseURI->GetScheme(&scheme);
        if (NS_FAILED(rv)) return rv;
    }
    
    nsCOMPtr<nsIProtocolHandler> handler;
    rv = GetProtocolHandler(scheme, getter_AddRefs(handler));
    nsCRT::free(scheme);
    if (NS_FAILED(rv)) return rv;

    return handler->NewURI(aSpec, aBaseURI, result);
}

NS_IMETHODIMP
nsIOService::NewChannelFromURI(const char* verb, nsIURI *aURI,
                               nsIEventSinkGetter *eventSinkGetter,
                               nsIChannel **result)
{
    nsresult rv;

    nsXPIDLCString scheme;
    rv = aURI->GetScheme(getter_Copies(scheme));
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIProtocolHandler> handler;
    rv = GetProtocolHandler((const char*)scheme, getter_AddRefs(handler));
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIEventQueue> eventQ;
    NS_WITH_SERVICE(nsIEventQueueService, eventQService, kEventQueueService, &rv);
    if (NS_FAILED(rv)) return rv;
    rv = eventQService->GetThreadEventQueue(PR_CurrentThread(), 
                                            getter_AddRefs(eventQ));
    if (NS_FAILED(rv)) return rv;

    nsIChannel* channel;
    rv = handler->NewChannel(verb, aURI, eventSinkGetter, eventQ,
                             &channel);
    if (NS_FAILED(rv)) return rv;

    *result = channel;
    return rv;
}

NS_IMETHODIMP
nsIOService::NewChannel(const char* verb, const char *aSpec,
                        nsIURI *aBaseURI,
                        nsIEventSinkGetter *eventSinkGetter,
                        nsIChannel **result)
{
    nsresult rv;
    nsIURI* uri;
    rv = NewURI(aSpec, aBaseURI, &uri);
    if (NS_FAILED(rv)) return rv;
    rv = NewChannelFromURI(verb, uri, eventSinkGetter, result);
    NS_RELEASE(uri);
    return rv;
}

NS_IMETHODIMP
nsIOService::MakeAbsolute(const char *aSpec,
                          nsIURI *aBaseURI,
                          char **result)
{
    nsresult rv;
    NS_ASSERTION(aBaseURI, "It doesn't make sense to not supply a base URI");

    if (aSpec == nsnull)
        return aBaseURI->GetSpec(result);
	
    char* scheme;
    rv = GetScheme(aSpec, &scheme);
    if (NS_SUCCEEDED(rv)) {
        // if aSpec has a scheme, then it's already absolute
        *result = nsCRT::strdup(aSpec);
        return (*result == nsnull) ? NS_ERROR_OUT_OF_MEMORY : NS_OK;
    }

    // else ask the protocol handler for the base URI to deal with it
    rv = aBaseURI->GetScheme(&scheme);
    if (NS_FAILED(rv)) return rv;
    
    nsCOMPtr<nsIProtocolHandler> handler;
    rv = GetProtocolHandler(scheme, getter_AddRefs(handler));
    nsCRT::free(scheme);
    if (NS_FAILED(rv)) return rv;

    return handler->MakeAbsolute(aSpec, aBaseURI, result);
}

NS_IMETHODIMP
nsIOService::GetAppCodeName(PRUnichar* *aAppCodeName)
{
    *aAppCodeName = mAppCodeName.ToNewUnicode();
    return NS_OK;
}

NS_IMETHODIMP
nsIOService::GetAppVersion(PRUnichar* *aAppVersion)
{
    *aAppVersion = mAppVersion.ToNewUnicode();
    return NS_OK;
}

NS_IMETHODIMP
nsIOService::GetAppName(PRUnichar* *aAppName)
{
    *aAppName = mAppName.ToNewUnicode();
    return NS_OK;
}

NS_IMETHODIMP
nsIOService::GetLanguage(PRUnichar* *aLanguage)
{
    *aLanguage = mAppLanguage.ToNewUnicode();
    return NS_OK;
}

NS_IMETHODIMP
nsIOService::GetPlatform(PRUnichar* *aPlatform)
{
    *aPlatform = mAppPlatform.ToNewUnicode();
    return NS_OK;
}

NS_IMETHODIMP
nsIOService::GetUserAgent(PRUnichar* *aUserAgent)
{
    // XXX this should load the http module and ask for the user agent string from it.
    char buf[200];
    PR_snprintf(buf, 200, "%.100s/%.90s", mAppCodeName.GetBuffer(), mAppVersion.GetBuffer());
    nsAutoString2 aUA(buf);
    *aUserAgent = aUA.ToNewUnicode();
    return NS_OK;
}

////////////////////////////////////////////////////////////////////////////////

NS_IMETHODIMP
nsIOService::NewAsyncStreamObserver(nsIStreamObserver *receiver, nsIEventQueue *eventQueue,
                                    nsIStreamObserver **result)
{
    return NS_NewAsyncStreamObserver(result, eventQueue, receiver);
    
}

NS_IMETHODIMP
nsIOService::NewAsyncStreamListener(nsIStreamListener *receiver, nsIEventQueue *eventQueue,
                                    nsIStreamListener **result)
{
    return NS_NewAsyncStreamListener(result, eventQueue, receiver);

}

NS_IMETHODIMP
nsIOService::NewSyncStreamListener(nsIInputStream **inStream, 
                                   nsIBufferOutputStream **outStream,
                                   nsIStreamListener **listener)
{
    return NS_NewSyncStreamListener(inStream, outStream, listener);

}

NS_IMETHODIMP
nsIOService::NewChannelFromNativePath(const char *nativePath, nsIFileChannel **result)
{
    nsresult rv;
    nsCOMPtr<nsIProtocolHandler> handler;
    rv = GetProtocolHandler("file", getter_AddRefs(handler));
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIFileProtocolHandler> fileHandler = do_QueryInterface(handler, &rv);
    if (NS_FAILED(rv)) return rv;
    
    nsCOMPtr<nsIFileChannel> channel;
    rv = fileHandler->NewChannelFromNativePath(nativePath, getter_AddRefs(channel));
    if (NS_FAILED(rv)) return rv;
    
    *result = channel;
    return NS_OK;
}

NS_IMETHODIMP
nsIOService::NewLoadGroup(nsISupports* outer, nsIStreamObserver* observer,
                          nsILoadGroup* parent, nsILoadGroup **result)
{
    nsresult rv;
    nsILoadGroup* group;
    rv = nsLoadGroup::Create(outer, nsCOMTypeInfo<nsILoadGroup>::GetIID(), 
                             (void**)&group);
    if (NS_FAILED(rv)) return rv;

    rv = group->Init(observer, parent);
    if (NS_FAILED(rv)) {
        NS_RELEASE(group);
        return rv;
    }

    *result = group;
    return NS_OK;
}

NS_IMETHODIMP
nsIOService::NewInputStreamChannel(nsIURI* uri, const char *contentType,
                                   nsIInputStream *inStr, nsIChannel **result)
{
    nsresult rv;
    nsInputStreamChannel* channel;
    rv = nsInputStreamChannel::Create(nsnull, nsCOMTypeInfo<nsIChannel>::GetIID(),
                                      (void**)&channel);
    if (NS_FAILED(rv)) return rv;
    rv = channel->Init(uri, contentType, inStr);
    if (NS_FAILED(rv)) {
        NS_RELEASE(channel);
        return rv;
    }
    *result = channel;
    return NS_OK;
}

////////////////////////////////////////////////////////////////////////////////
