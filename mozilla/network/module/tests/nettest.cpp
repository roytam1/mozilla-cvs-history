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
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

#include <stdio.h>
#include <assert.h>

#ifdef XP_PC
#include <windows.h>
#endif

#include "plstr.h"
#include "plevent.h"

#include "nsIStreamListener.h"
#include "nsIInputStream.h"
#include "nsIURL.h"
#include "nsINetService.h"
#include "nsIComponentManager.h"
#include "nsIServiceManager.h"
#include "nsIEventQueueService.h"
#include "nsString.h"


int urlLoaded;
PRBool bTraceEnabled;
PRBool bLoadAsync;

#include "nsIPostToServer.h"
#include "nsINetService.h"

#ifdef XP_PC
#define NETLIB_DLL "netlib.dll"
#define XPCOM_DLL  "xpcom32.dll"
#else
#ifdef XP_MAC
#include "nsMacRepository.h"
#else
#define NETLIB_DLL "libnetlib"MOZ_DLL_SUFFIX
#define XPCOM_DLL  "libxpcom"MOZ_DLL_SUFFIX
#endif
#endif

// Define CIDs...
static NS_DEFINE_IID(kNetServiceCID, NS_NETSERVICE_CID);
static NS_DEFINE_IID(kEventQueueServiceCID, NS_EVENTQUEUESERVICE_CID);

// Define IIDs...
static NS_DEFINE_IID(kIEventQueueServiceIID, NS_IEVENTQUEUESERVICE_IID);
NS_DEFINE_IID(kIPostToServerIID, NS_IPOSTTOSERVER_IID);


/* XXX: Don't include net.h... */
extern "C" {
extern void NET_ToggleTrace();
};

class TestConsumer : public nsIStreamListener
{

public:
    NS_DECL_ISUPPORTS
    
    TestConsumer();
    virtual ~TestConsumer();

    NS_IMETHOD GetBindInfo(nsIURI* aURL, nsStreamBindingInfo* info);
    NS_IMETHOD OnProgress(nsIURI* aURL, PRUint32 Progress, PRUint32 ProgressMax);
    NS_IMETHOD OnStatus(nsIURI* aURL, const PRUnichar* aMsg);
    NS_IMETHOD OnStartRequest(nsIURI* aURL, const char *aContentType);
    NS_IMETHOD OnDataAvailable(nsIURI* aURL, nsIInputStream *pIStream, PRUint32 length);
    NS_IMETHOD OnStopRequest(nsIURI* aURL, nsresult status, const PRUnichar* aMsg);
};


TestConsumer::TestConsumer()
{
    NS_INIT_REFCNT();
}


NS_DEFINE_IID(kIStreamListenerIID, NS_ISTREAMLISTENER_IID);
NS_IMPL_ISUPPORTS(TestConsumer,kIStreamListenerIID);


TestConsumer::~TestConsumer()
{
    if (bTraceEnabled) {
        printf("\n+++ TestConsumer is being deleted...\n");
    }
}


NS_IMETHODIMP TestConsumer::GetBindInfo(nsIURI* aURL, nsStreamBindingInfo* info)
{
    if (bTraceEnabled) {
        printf("\n+++ TestConsumer::GetBindInfo: URL: %p\n", aURL);
    }

    return 0;
}

NS_IMETHODIMP TestConsumer::OnProgress(nsIURI* aURL, PRUint32 Progress, 
                                       PRUint32 ProgressMax)
{
    if (bTraceEnabled) {
        printf("\n+++ TestConsumer::OnProgress: URL: %p - %d of total %d\n", aURL, Progress, ProgressMax);
    }

    return 0;
}

NS_IMETHODIMP TestConsumer::OnStatus(nsIURI* aURL, const PRUnichar* aMsg)
{
    if (bTraceEnabled) {
        printf("\n+++ TestConsumer::OnStatus: ");
        nsAutoString str(aMsg);
        char* c = str.ToNewCString();
        fputs(c, stdout);
        delete[] c;
        fputs("\n", stdout);
    }

    return 0;
}

NS_IMETHODIMP TestConsumer::OnStartRequest(nsIURI* aURL, const char *aContentType)
{
    if (bTraceEnabled) {
        printf("\n+++ TestConsumer::OnStartRequest: URL: %p, Content type: %s\n", aURL, aContentType);
    }

    return 0;
}


NS_IMETHODIMP TestConsumer::OnDataAvailable(nsIURI* aURL, nsIInputStream *pIStream, PRUint32 length) 
{
    PRUint32 len;

    if (bTraceEnabled) {
        printf("\n+++ TestConsumer::OnDataAvailable: URL: %p, %d bytes available...\n", aURL, length);
    }

    do {
        nsresult err;
        char buffer[80];
        PRUint32 i;

        err = pIStream->Read(buffer, 80, &len);
        if (err == NS_OK) {
            for (i=0; i<len; i++) {
                putchar(buffer[i]);
            }
        }
    } while (len > 0);

    return 0;
}


NS_IMETHODIMP TestConsumer::OnStopRequest(nsIURI* aURL, nsresult status, const PRUnichar* aMsg)
{
    if (bTraceEnabled) {
        printf("\n+++ TestConsumer::OnStopRequest... URL: %p status: %d\n", aURL, status);
    }

    if (NS_FAILED(status)) {
      const char* url;
      aURL->GetSpec(&url);

      printf("Unable to load URL %s\n", url);
    }
    /* The document has been loaded, so drop out of the message pump... */
    urlLoaded = 1;
    return 0;
}


nsresult ReadStreamSynchronously(nsIInputStream* aIn)
{
    nsresult rv;
    char buffer[1024];

    if (nsnull != aIn) {
        PRUint32 len;

        do {
            PRUint32 i;

            rv = aIn->Read(buffer, sizeof(buffer), &len);
            for (i=0; i<len; i++) {
                putchar(buffer[i]);
            }
        } while (len > 0);
    }
    return NS_OK;
}



int main(int argc, char **argv)
{
    nsAutoString url_address;
    char buf[256];
    nsIStreamListener *pConsumer;
    nsIEventQueueService* pEventQService;
    nsIURI *pURL;
    nsresult result;
    int i;

    if (argc < 2) {
        printf("test: [-trace] [-sync] <URL>\n");
        return 0;
    }

    nsComponentManager::RegisterComponent(kEventQueueServiceCID, NULL, NULL, XPCOM_DLL, PR_FALSE, PR_FALSE);
    nsComponentManager::RegisterComponent(kNetServiceCID, NULL, NULL, NETLIB_DLL, PR_FALSE, PR_FALSE);

    // Create the Event Queue for this thread...
    pEventQService = nsnull;
    result = nsServiceManager::GetService(kEventQueueServiceCID,
                                          kIEventQueueServiceIID,
                                          (nsISupports **)&pEventQService);
    if (NS_SUCCEEDED(result)) {
      // XXX: What if this fails?
      result = pEventQService->CreateThreadEventQueue();
    }

    bTraceEnabled = PR_FALSE;
    bLoadAsync    = PR_TRUE;

    for (i=1; i < argc; i++) {
        // Turn on netlib tracing...
        if (PL_strcasecmp(argv[i], "-trace") == 0) {
            NET_ToggleTrace();
            bTraceEnabled = PR_TRUE;
            continue;
        } 
        // Turn on synchronous URL loading...
        if (PL_strcasecmp(argv[i], "-sync") == 0) {
            bLoadAsync = PR_FALSE;
            continue;
        }

        urlLoaded = 0;

        url_address = argv[i];
        if (bTraceEnabled) {
            url_address.ToCString(buf, 256);
            printf("+++ loading URL: %s...\n", buf);
        }

        pConsumer = new TestConsumer;
        pConsumer->AddRef();
        
        // Create the URL object...
        pURL = NULL;
        result = NS_NewURL(&pURL, url_address);
        if (NS_OK != result) {
            if (bTraceEnabled) {
                printf("NS_NewURL() failed...\n");
            }
            return 1;
        }

#if 0
        nsIPostToServer *pPoster;
        result = pURL->QueryInterface(kIPostToServerIID, (void**)&pPoster);
        if (result == NS_OK) {
            pPoster->SendFile("foo.txt");
        }
        NS_IF_RELEASE(pPoster);
#endif
        
        // Start the URL load...
        if (PR_TRUE == bLoadAsync) {
            result = NS_OpenURL(pURL, pConsumer);

            /* If the open failed, then do not drop into the message loop... */
            if (NS_OK != result) {
                const char* url;
                pURL->GetSpec(&url);

                printf("Unable to load URL %s\n", url);
                urlLoaded = 1;
            }
        } 
        // Load the URL synchronously...
        else {
            nsIInputStream *in;

            result = NS_OpenURL(pURL, &in);
            ReadStreamSynchronously(in);
            NS_IF_RELEASE(in);
            urlLoaded = 1;
        }
        
        
        // Enter the message pump to allow the URL load to proceed.
        while ( !urlLoaded ) {
#ifdef XP_PC
            MSG msg;

            if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
#endif
        }
        
        pURL->Release();
    }
    
    if (nsnull != pEventQService) {
        pEventQService->DestroyThreadEventQueue();
        nsServiceManager::ReleaseService(kEventQueueServiceCID, pEventQService);
    }
    return 0;
}
