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

    nsIOService* ios = new nsIOService();
    if (ios == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(ios);
    nsresult rv = ios->QueryInterface(aIID, aResult);
    NS_RELEASE(ios);
    return rv;
}

NS_IMPL_ISUPPORTS(nsIOService, nsIIOService::GetIID());

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

    nsIProtocolHandler* handler;
    rv = nsServiceManager::GetService(buf, nsIProtocolHandler::GetIID(),
                                      (nsISupports**)&handler);
    
    if (NS_FAILED(rv)) return rv;

    *result = handler;
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
    }
    return NS_ERROR_FAILURE;    // no colon
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

    rv = handler->NewURI(aSpec, aBaseURI, result);
    //NS_RELEASE(handler);
    return rv;
}

NS_IMETHODIMP
nsIOService::NewChannelFromURI(const char* verb, nsIURI *aURI,
                               nsIEventSinkGetter *eventSinkGetter,
                               nsIChannel **result)
{
    nsresult rv;

    char* scheme;
    rv = aURI->GetScheme(&scheme);
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIProtocolHandler> handler;
    rv = GetProtocolHandler(scheme, getter_AddRefs(handler));
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

    char* scheme;
    rv = GetScheme(aSpec, &scheme);
    if (NS_SUCCEEDED(rv)) {
        // if aSpec has a scheme, then it's already absolute
        *result = nsCRT::strdup(aSpec);
        if (*result == nsnull)
            return NS_ERROR_OUT_OF_MEMORY;
        return NS_OK;
    }

    // else ask the protocol handler for the base URI to deal with it
    rv = aBaseURI->GetScheme(&scheme);
    if (NS_FAILED(rv)) return rv;
    
    nsCOMPtr<nsIProtocolHandler> handler;
    rv = GetProtocolHandler(scheme, getter_AddRefs(handler));
    nsCRT::free(scheme);
    if (NS_FAILED(rv)) return rv;

    rv = handler->MakeAbsolute(aSpec, aBaseURI, result);
    return rv;
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
    nsIProtocolHandler* handler;
    rv = GetProtocolHandler("file", &handler);
    if (NS_FAILED(rv)) return rv;

    nsIFileProtocolHandler* fileHandler = nsnull;
    rv = handler->QueryInterface(nsIFileProtocolHandler::GetIID(), 
                                 (void**)&fileHandler);
    NS_RELEASE(handler);
    if (NS_FAILED(rv)) return rv;
    
    nsIFileChannel* channel;
    rv = fileHandler->NewChannelFromNativePath(nativePath, &channel);
    NS_RELEASE(fileHandler);
    if (NS_FAILED(rv)) return rv;
    
    *result = channel;
    return NS_OK;
}

NS_IMETHODIMP
nsIOService::NewLoadGroup(nsILoadGroup **result)
{
    return nsLoadGroup::Create(nsnull, nsILoadGroup::GetIID(), 
                               (void**)result);
}

////////////////////////////////////////////////////////////////////////////////

    
