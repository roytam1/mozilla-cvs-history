/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

#include "nsIResProtocolHandler.h"
#include "nsIServiceManager.h"
#include "nsIIOService.h"
#include "nsIInputStream.h"
#include "nsIComponentManager.h"
#include "nsIStreamListener.h"
#include "nsIEventQueueService.h"
#include "nsIURI.h"
#include "nsCRT.h"

static NS_DEFINE_CID(kIOServiceCID, NS_IOSERVICE_CID);
static NS_DEFINE_CID(kEventQueueServiceCID, NS_EVENTQUEUESERVICE_CID);

////////////////////////////////////////////////////////////////////////////////

nsresult
SetupMapping()
{
    nsresult rv;

    NS_WITH_SERVICE(nsIIOService, serv, kIOServiceCID, &rv);
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIProtocolHandler> ph;
    rv = serv->GetProtocolHandler("res", getter_AddRefs(ph));
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIResProtocolHandler> resPH = do_QueryInterface(ph, &rv);
    if (NS_FAILED(rv)) return rv;

    rv = resPH->AppendSubstitution("foo", "file://y|/");
    if (NS_FAILED(rv)) return rv;

    rv = resPH->AppendSubstitution("foo", "file://y|/mozilla/dist/win32_D.OBJ/bin/");
    if (NS_FAILED(rv)) return rv;

    return rv;
}

////////////////////////////////////////////////////////////////////////////////

nsresult
TestOpenInputStream(const char* url)
{
    nsresult rv;

    NS_WITH_SERVICE(nsIIOService, serv, kIOServiceCID, &rv);
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIChannel> channel;
    rv = serv->NewChannel(url,
                          nsnull, // base uri
                          getter_AddRefs(channel));
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIInputStream> in;
    rv = channel->OpenInputStream(getter_AddRefs(in));
    if (NS_FAILED(rv)) {
        fprintf(stdout, "failed to OpenInputStream for %s\n", url);
        return NS_OK;
    }

    char buf[1024];
    while (1) {
        PRUint32 amt;
        rv = in->Read(buf, sizeof(buf), &amt);
        if (NS_FAILED(rv)) return rv;
        if (amt == 0) break;    // eof
        
        char* str = buf;
        while (amt-- > 0) {
            fputc(*str++, stdout);
        }
    }
    nsCOMPtr<nsIURI> uri;
    char* str;

    rv = channel->GetOriginalURI(getter_AddRefs(uri));
    if (NS_FAILED(rv)) return rv;
    rv = uri->GetSpec(&str);
    if (NS_FAILED(rv)) return rv;
    fprintf(stdout, "%s resolved to ", str);
    nsCRT::free(str);

    rv = channel->GetURI(getter_AddRefs(uri));
    if (NS_FAILED(rv)) return rv;
    rv = uri->GetSpec(&str);
    if (NS_FAILED(rv)) return rv;
    fprintf(stdout, "%s\n", str);
    nsCRT::free(str);

    return NS_OK;
}

////////////////////////////////////////////////////////////////////////////////

PRBool gDone = PR_FALSE;
nsIEventQueue* gEventQ = nsnull;

class Listener : public nsIStreamListener 
{
public:
    NS_DECL_ISUPPORTS

    Listener() { NS_INIT_REFCNT(); }
    virtual ~Listener() {}

    NS_IMETHOD OnStartRequest(nsIChannel *channel, nsISupports *ctxt) {
        nsresult rv;
        nsCOMPtr<nsIURI> uri;
        rv = channel->GetURI(getter_AddRefs(uri));
        if (NS_SUCCEEDED(rv)) {
            char* str;
            rv = uri->GetSpec(&str);
            if (NS_SUCCEEDED(rv)) {
                fprintf(stdout, "Starting to load %s\n", str);
                nsCRT::free(str);
            }
        }
        return NS_OK;
    }
    
    NS_IMETHOD OnStopRequest(nsIChannel *channel, nsISupports *ctxt, 
                             nsresult aStatus, const PRUnichar* aStatusArg) {
        nsresult rv;
        nsCOMPtr<nsIURI> uri;
        rv = channel->GetURI(getter_AddRefs(uri));
        if (NS_SUCCEEDED(rv)) {
            char* str;
            rv = uri->GetSpec(&str);
            if (NS_SUCCEEDED(rv)) {
                fprintf(stdout, "Ending load %s, status=%x\n", str, aStatus);
                nsCRT::free(str);
            }
        }
        gDone = PR_TRUE;
        return NS_OK;
    }

    NS_IMETHOD OnDataAvailable(nsIChannel *channel, nsISupports *ctxt, 
                               nsIInputStream *inStr,
                               PRUint32 sourceOffset, PRUint32 count) {
        nsresult rv;
        char buf[1024];
        while (count > 0) {
            PRUint32 amt;
            rv = inStr->Read(buf, sizeof(buf), &amt);
            count -= amt;
            char* c = buf;
            while (amt-- > 0) {
                fputc(*c++, stdout);
            }
        }
        return NS_OK;
    }
};

NS_IMPL_ISUPPORTS2(Listener, nsIStreamListener, nsIStreamObserver)

nsresult
TestAsyncRead(const char* url)
{
    nsresult rv;

    NS_WITH_SERVICE(nsIEventQueueService, eventQService, kEventQueueServiceCID, &rv);
    if (NS_FAILED(rv)) return rv;

    rv = eventQService->CreateThreadEventQueue();
    if (NS_FAILED(rv)) return rv;

    rv = eventQService->GetThreadEventQueue(NS_CURRENT_THREAD, &gEventQ);
    if (NS_FAILED(rv)) return rv;

    NS_WITH_SERVICE(nsIIOService, serv, kIOServiceCID, &rv);
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIChannel> channel;
    rv = serv->NewChannel(url,
                          nsnull, // base uri
                          getter_AddRefs(channel));
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIStreamListener> listener = new Listener();
    if (listener == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;

    rv = channel->AsyncRead(nsnull, listener);
    if (NS_FAILED(rv)) return rv;

    while (!gDone) {
        PLEvent* event;
        rv = gEventQ->GetEvent(&event);
        if (NS_FAILED(rv)) return rv;
        rv = gEventQ->HandleEvent(event);
        if (NS_FAILED(rv)) return rv;
    }

    return rv;
}

////////////////////////////////////////////////////////////////////////////////

nsresult
NS_AutoregisterComponents()
{
    nsresult rv = nsComponentManager::AutoRegister(nsIComponentManager::NS_Startup,
                                                   NULL /* default */);
    return rv;
}

int
main(int argc, char* argv[])
{
    nsresult rv;

    rv = NS_AutoregisterComponents();
    NS_ASSERTION(NS_SUCCEEDED(rv), "AutoregisterComponents failed");

    if (argc < 2) {
        printf("usage: %s resource://foo/<path-to-resolve>\n", argv[0]);
        return -1;
    }

    rv = SetupMapping();
    NS_ASSERTION(NS_SUCCEEDED(rv), "SetupMapping failed");
    if (NS_FAILED(rv)) return rv;

    rv = TestOpenInputStream(argv[1]);
    NS_ASSERTION(NS_SUCCEEDED(rv), "TestOpenInputStream failed");

    rv = TestAsyncRead(argv[1]);
    NS_ASSERTION(NS_SUCCEEDED(rv), "TestAsyncRead failed");

    return rv;
}

////////////////////////////////////////////////////////////////////////////////
