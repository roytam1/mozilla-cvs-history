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
 *   Pierre Phaneuf <pp@ludusdesign.com>
 */

/*

  A protocol handler for ``chrome:''

*/
 
#include "nsCOMPtr.h"
#include "nsCRT.h"
#include "nsChromeProtocolHandler.h"
#include "nsIChannel.h"
#include "nsIChromeRegistry.h"
#include "nsIComponentManager.h"
#include "nsIEventQueue.h"
#include "nsIEventQueueService.h"
#include "nsIIOService.h"
#include "nsILoadGroup.h"
#include "nsIScriptSecurityManager.h"
#include "nsIStreamListener.h"
#include "nsIServiceManager.h"
#include "nsIXULDocument.h"
#include "nsIXULPrototypeCache.h"
#include "nsIXULPrototypeDocument.h"
#include "nsRDFCID.h"
#include "nsXPIDLString.h"
#include "prlog.h"

//----------------------------------------------------------------------

static NS_DEFINE_CID(kChromeRegistryCID,         NS_CHROMEREGISTRY_CID);
static NS_DEFINE_CID(kEventQueueServiceCID,      NS_EVENTQUEUESERVICE_CID);
static NS_DEFINE_CID(kIOServiceCID,              NS_IOSERVICE_CID);
static NS_DEFINE_CID(kStandardURLCID,            NS_STANDARDURL_CID);
static NS_DEFINE_CID(kXULDocumentCID,            NS_XULDOCUMENT_CID);
static NS_DEFINE_CID(kXULPrototypeCacheCID,      NS_XULPROTOTYPECACHE_CID);

//----------------------------------------------------------------------
//
//  A channel that's used for loading cached chrome documents. Since a
//  cached chrome document really doesn't have anything to do to load,
//  this is just the puppeteer that pulls the webshell's strings at the
//  right time.
//
//  Specifically, it fires the listener's OnStartRequest() from
//  AsyncOpen() and adds the channel to the load group. This winds its
//  way through webshell and causes the XUL document's
//  StartDocumentLoad() method to be called.
//
//  It then queues an event, which will fire the listener's
//  OnStopRequest() and remove the channel from the load group. This
//  is done asynchronously to allow the stack to unwind back to the
//  main event loop, which avoids any weird re-entrancy that occurs if
//  we try to immediately fire the OnStopRequest(). (Some portion of
//  the embedding dance happens after AsyncRead() is called, this
//  allows that to proceed "normally".)
//
//  For logging information, NSPR_LOG_MODULES=nsCachedChromeChannel:5
//

class nsCachedChromeChannel : public nsIChannel
{
protected:
    nsCachedChromeChannel(nsIURI* aURI, nsILoadGroup* aLoadGroup);
    virtual ~nsCachedChromeChannel();

    nsCOMPtr<nsIURI>            mURI;
    nsCOMPtr<nsILoadGroup>      mLoadGroup;
    nsCOMPtr<nsIStreamListener> mListener;
    nsCOMPtr<nsISupports>       mContext;
    nsLoadFlags                 mLoadAttributes;
    nsCOMPtr<nsISupports>       mOwner;

    struct StopLoadEvent {
        PLEvent                mEvent;
        nsCachedChromeChannel* mChannel;
    };

    static void* HandleStopLoadEvent(PLEvent* aEvent);
    static void DestroyStopLoadEvent(PLEvent* aEvent);

#ifdef PR_LOGGING
    static PRLogModuleInfo* gLog;
#endif

public:
    static nsresult
    Create(nsIURI* aURI, nsILoadGroup* aLoadGroup, nsIChannel** aResult);
	
    NS_DECL_ISUPPORTS

    // nsIRequest
    NS_IMETHOD IsPending(PRBool *_retval) { *_retval = PR_TRUE; return NS_OK; }
    NS_IMETHOD Cancel(void)  { return NS_OK; }
    NS_IMETHOD Suspend(void) { return NS_OK; }
    NS_IMETHOD Resume(void)  { return NS_OK; }

    // nsIChannel    
    NS_DECL_NSICHANNEL
};

#ifdef PR_LOGGING
PRLogModuleInfo* nsCachedChromeChannel::gLog;
#endif

NS_IMPL_ADDREF(nsCachedChromeChannel);
NS_IMPL_RELEASE(nsCachedChromeChannel);
NS_IMPL_QUERY_INTERFACE2(nsCachedChromeChannel, nsIRequest, nsIChannel);

nsresult
nsCachedChromeChannel::Create(nsIURI* aURI, nsILoadGroup* aLoadGroup, nsIChannel** aResult)
{
    NS_PRECONDITION(aURI != nsnull, "null ptr");
    if (! aURI)
        return NS_ERROR_NULL_POINTER;

    nsCachedChromeChannel* channel = new nsCachedChromeChannel(aURI, aLoadGroup);
    if (! channel)
        return NS_ERROR_OUT_OF_MEMORY;

    *aResult = channel;
    NS_ADDREF(*aResult);
    return NS_OK;
}


nsCachedChromeChannel::nsCachedChromeChannel(nsIURI* aURI, nsILoadGroup* aLoadGroup)
    : mURI(aURI), mLoadGroup(aLoadGroup), mLoadAttributes (nsIChannel::LOAD_NORMAL)
{
    NS_INIT_REFCNT();

#ifdef PR_LOGGING
    if (! gLog)
        gLog = PR_NewLogModule("nsCachedChromeChannel");
#endif

    PR_LOG(gLog, PR_LOG_DEBUG,
           ("nsCachedChromeChannel[%p]: created", this));
}


nsCachedChromeChannel::~nsCachedChromeChannel()
{
    PR_LOG(gLog, PR_LOG_DEBUG,
           ("nsCachedChromeChannel[%p]: destroyed", this));
}


NS_IMETHODIMP
nsCachedChromeChannel::GetOriginalURI(nsIURI * *aOriginalURI)
{
    *aOriginalURI = mURI;
    NS_ADDREF(*aOriginalURI);
    return NS_OK;
}

NS_IMETHODIMP
nsCachedChromeChannel::GetURI(nsIURI * *aURI)
{
    *aURI = mURI;
    NS_ADDREF(*aURI);
    return NS_OK;
}

NS_IMETHODIMP
nsCachedChromeChannel::OpenInputStream(PRUint32 startPosition, PRInt32 readCount, nsIInputStream **_retval)
{
    NS_NOTREACHED("don't do that");
    *_retval = nsnull;
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsCachedChromeChannel::OpenOutputStream(PRUint32 startPosition, nsIOutputStream **_retval)
{
    NS_NOTREACHED("don't do that");
    *_retval = nsnull;
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsCachedChromeChannel::AsyncOpen(nsIStreamObserver *observer, nsISupports *ctxt)
{
    NS_NOTREACHED("don't do that");
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsCachedChromeChannel::AsyncRead(PRUint32 startPosition, PRInt32 readCount, nsISupports *ctxt, nsIStreamListener *listener)
{
    if (listener) {
        nsresult rv;

        if (mLoadGroup) {
            PR_LOG(gLog, PR_LOG_DEBUG,
                   ("nsCachedChromeChannel[%p]: adding self to load group %p",
                    this, mLoadGroup.get()));

            rv = mLoadGroup->AddChannel(this, nsnull);
            if (NS_FAILED(rv)) return rv;
        }

        // Fire the OnStartRequest(), which will cause the XUL
        // document to get embedded.
        PR_LOG(gLog, PR_LOG_DEBUG,
               ("nsCachedChromeChannel[%p]: firing OnStartRequest for %p",
                this, listener));

        rv = listener->OnStartRequest(this, ctxt);

        // Queue an event to ourselves to let the stack unwind before
        // calling OnStopRequest(). This allows embedding to occur
        // before we fire OnStopRequest().
        if (NS_SUCCEEDED(rv)) {
            nsCOMPtr<nsIEventQueueService> svc = do_GetService(kEventQueueServiceCID, &rv);
            if (svc) {
                nsCOMPtr<nsIEventQueue> queue;
                rv = svc->GetThreadEventQueue(NS_CURRENT_THREAD, getter_AddRefs(queue));
                if (NS_SUCCEEDED(rv) && queue) {
                    rv = NS_ERROR_OUT_OF_MEMORY;
                    StopLoadEvent* event = new StopLoadEvent;
                    if (event) {
                        PL_InitEvent(NS_REINTERPRET_CAST(PLEvent*, event),
                                     nsnull,
                                     HandleStopLoadEvent,
                                     DestroyStopLoadEvent);

                        event->mChannel = this;
                        NS_ADDREF(event->mChannel);

                        rv = queue->EnterMonitor();
                        if (NS_SUCCEEDED(rv)) {
                            mContext  = ctxt;
                            mListener = listener;

                            (void) queue->PostEvent(NS_REINTERPRET_CAST(PLEvent*, event));
                            (void) queue->ExitMonitor();
                            return NS_OK;
                        }

                        // If we get here, something bad happened. Clean up.
                        NS_RELEASE(event->mChannel);
                        delete event;
                    }
                }
            }
        }

        // Uh oh, something went wrong. Fire a balancing
        // OnStopRequest() and indicate an error occurred.
        PR_LOG(gLog, PR_LOG_DEBUG,
               ("nsCachedChromeChannel[%p]: error 0x%x! firing on OnStopRequest for %p",
                this, rv, mListener.get()));

        (void) listener->OnStopRequest(this, ctxt, rv, nsnull);

        if (mLoadGroup) {
            PR_LOG(gLog, PR_LOG_DEBUG,
                   ("nsCachedChromeChannel[%p]: removing self from load group %p",
                    this, mLoadGroup.get()));

            (void) mLoadGroup->RemoveChannel(this, nsnull, nsnull, nsnull);
        }
    }

    return NS_OK;
}

NS_IMETHODIMP
nsCachedChromeChannel::AsyncWrite(nsIInputStream *fromStream, PRUint32 startPosition, PRInt32 writeCount, nsISupports *ctxt, nsIStreamObserver *observer)
{
    NS_NOTREACHED("don't do that");
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsCachedChromeChannel::GetLoadAttributes(nsLoadFlags *aLoadAttributes)
{
    *aLoadAttributes = mLoadAttributes; 
    return NS_OK;
}

NS_IMETHODIMP
nsCachedChromeChannel::SetLoadAttributes(nsLoadFlags aLoadAttributes)
{
    mLoadAttributes = aLoadAttributes;
    return NS_OK;
}

NS_IMETHODIMP
nsCachedChromeChannel::GetContentType(char * *aContentType)
{
    *aContentType = nsXPIDLCString::Copy("text/cached-xul");
    return *aContentType ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

NS_IMETHODIMP
nsCachedChromeChannel::SetContentType(const char *aContentType)
{
    // Do not allow the content-type to be changed.
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsCachedChromeChannel::GetContentLength(PRInt32 *aContentLength)
{
    NS_NOTREACHED("don't do that");
    *aContentLength = 0;
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsCachedChromeChannel::GetOwner(nsISupports * *aOwner)
{
    *aOwner = mOwner.get();
    NS_IF_ADDREF(*aOwner);
    return NS_OK;
}

NS_IMETHODIMP
nsCachedChromeChannel::SetOwner(nsISupports * aOwner)
{
    mOwner = aOwner;
    return NS_OK;
}

NS_IMETHODIMP
nsCachedChromeChannel::GetLoadGroup(nsILoadGroup * *aLoadGroup)
{
    *aLoadGroup = mLoadGroup;
    NS_IF_ADDREF(*aLoadGroup);
    return NS_OK;
}

NS_IMETHODIMP
nsCachedChromeChannel::SetLoadGroup(nsILoadGroup * aLoadGroup)
{
    mLoadGroup = aLoadGroup;
    return NS_OK;
}

NS_IMETHODIMP
nsCachedChromeChannel::GetNotificationCallbacks(nsIInterfaceRequestor * *aNotificationCallbacks)
{
    NS_NOTREACHED("don't do that");
    *aNotificationCallbacks = nsnull;
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsCachedChromeChannel::SetNotificationCallbacks(nsIInterfaceRequestor * aNotificationCallbacks)
{
    NS_NOTREACHED("don't do that");
    return NS_ERROR_FAILURE;
}


void*
nsCachedChromeChannel::HandleStopLoadEvent(PLEvent* aEvent)
{
    // Fire the OnStopRequest() for the cached chrome channel, and
    // remove it from the load group.
    StopLoadEvent* event = NS_REINTERPRET_CAST(StopLoadEvent*, aEvent);
    nsCachedChromeChannel* channel = event->mChannel;

    PR_LOG(gLog, PR_LOG_DEBUG,
           ("nsCachedChromeChannel[%p]: firing OnStopRequest for %p",
            channel, channel->mListener.get()));

    (void) channel->mListener->OnStopRequest(channel, channel->mContext, NS_OK, nsnull);

    if (channel->mLoadGroup) {
        PR_LOG(gLog, PR_LOG_DEBUG,
               ("nsCachedChromeChannel[%p]: removing self from load group %p",
                channel, channel->mLoadGroup.get()));

        (void) channel->mLoadGroup->RemoveChannel(channel, nsnull, nsnull, nsnull);
    }

    channel->mListener = nsnull;
    channel->mContext  = nsnull;

    return nsnull;
}

void
nsCachedChromeChannel::DestroyStopLoadEvent(PLEvent* aEvent)
{
    StopLoadEvent* event = NS_REINTERPRET_CAST(StopLoadEvent*, aEvent);
    NS_RELEASE(event->mChannel);
    delete event;
}

////////////////////////////////////////////////////////////////////////////////

nsChromeProtocolHandler::nsChromeProtocolHandler()
{
    NS_INIT_REFCNT();
}

nsresult
nsChromeProtocolHandler::Init()
{
    return NS_OK;
}

nsChromeProtocolHandler::~nsChromeProtocolHandler()
{
}

NS_IMPL_ISUPPORTS(nsChromeProtocolHandler, NS_GET_IID(nsIProtocolHandler));

NS_METHOD
nsChromeProtocolHandler::Create(nsISupports *aOuter, REFNSIID aIID, void **aResult)
{
    if (aOuter)
        return NS_ERROR_NO_AGGREGATION;

    nsChromeProtocolHandler* ph = new nsChromeProtocolHandler();
    if (ph == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(ph);
    nsresult rv = ph->Init();
    if (NS_SUCCEEDED(rv)) {
        rv = ph->QueryInterface(aIID, aResult);
    }
    NS_RELEASE(ph);
    return rv;
}

////////////////////////////////////////////////////////////////////////////////
// nsIProtocolHandler methods:

NS_IMETHODIMP
nsChromeProtocolHandler::GetScheme(char* *result)
{
    *result = nsCRT::strdup("chrome");
    if (*result == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;
    return NS_OK;
}

NS_IMETHODIMP
nsChromeProtocolHandler::GetDefaultPort(PRInt32 *result)
{
    *result = -1;        // no port for chrome: URLs
    return NS_OK;
}

NS_IMETHODIMP
nsChromeProtocolHandler::GetUritype(PRInt16 *result)
{
    *result = url_std;
    return NS_OK;
}

NS_IMETHODIMP
nsChromeProtocolHandler::NewURI(const char *aSpec, nsIURI *aBaseURI,
                                nsIURI **result)
{
    nsresult rv;

    // Chrome: URLs (currently) have no additional structure beyond that provided by standard
    // URLs, so there is no "outer" given to CreateInstance 

    nsIURI* url;
    if (aBaseURI) {
        rv = aBaseURI->Clone(&url);
        if (NS_FAILED(rv)) return rv;
        rv = url->SetRelativePath(aSpec);
    }
    else {
        rv = nsComponentManager::CreateInstance(kStandardURLCID, nsnull,
                                                NS_GET_IID(nsIURI),
                                                (void**)&url);
        if (NS_FAILED(rv)) return rv;
        rv = url->SetSpec((char*)aSpec);
    }

    if (NS_FAILED(rv)) {
        NS_RELEASE(url);
        return rv;
    }

    *result = url;
    return rv;
}

NS_IMETHODIMP
nsChromeProtocolHandler::NewChannel(const char* aVerb, nsIURI* aURI,
                                    nsILoadGroup* aLoadGroup,
                                    nsIInterfaceRequestor* aNotificationCallbacks,
                                    nsLoadFlags aLoadAttributes,
                                    nsIURI* aOriginalURI,
                                    PRUint32 bufferSegmentSize,
                                    PRUint32 bufferMaxSize,
                                    nsIChannel* *aResult)
{
    nsresult rv;
    nsCOMPtr<nsIChannel> result;

    // Canonify the "chrome:" URL; e.g., so that we collapse
    // "chrome://navigator/content/" and "chrome://navigator/content"
    // and "chrome://navigator/content/navigator.xul".
    NS_WITH_SERVICE(nsIChromeRegistry, reg, kChromeRegistryCID, &rv);
    if (NS_FAILED(rv)) return rv;

    rv = reg->Canonify(aURI);
    if (NS_FAILED(rv)) return rv;

    // Check the prototype cache to see if we've already got the
    // document in the cache.
    NS_WITH_SERVICE(nsIXULPrototypeCache, cache, kXULPrototypeCacheCID, &rv);
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIXULPrototypeDocument> proto;
    rv = cache->GetPrototype(aURI, getter_AddRefs(proto));
    if (NS_FAILED(rv)) return rv;

    if (proto) {
        // ...in which case, we'll create a dummy stream that'll just
        // load the thing.
        rv = nsCachedChromeChannel::Create(aURI, aLoadGroup, getter_AddRefs(result));
        if (NS_FAILED(rv)) return rv;
    }
    else {
        // Miss. Resolve the chrome URL using the registry and do a
        // normal necko load.
        nsCOMPtr<nsIURI> chromeURI;
        rv = aURI->Clone(getter_AddRefs(chromeURI));        // don't mangle the original
        if (NS_FAILED(rv)) return rv;

        rv = reg->ConvertChromeURL(chromeURI);
        if (NS_FAILED(rv)) return rv;

        // now fetch the converted URI
        NS_WITH_SERVICE(nsIIOService, serv, kIOServiceCID, &rv);
        if (NS_FAILED(rv)) return rv;

        rv = serv->NewChannelFromURI(aVerb, chromeURI,
                                     aLoadGroup,
                                     aNotificationCallbacks,
                                     aLoadAttributes,
                                     aOriginalURI ? aOriginalURI : aURI,
                                     bufferSegmentSize, bufferMaxSize,
                                     getter_AddRefs(result));

        if (NS_FAILED(rv)) return rv;

        // Get a system principal for chrome and set the owner
        // property of the result
        NS_WITH_SERVICE(nsIScriptSecurityManager, securityManager, NS_SCRIPTSECURITYMANAGER_PROGID, &rv);
        if (NS_FAILED(rv)) return rv;

        nsCOMPtr<nsIPrincipal> principal;
        rv = securityManager->GetSystemPrincipal(getter_AddRefs(principal));
        if (NS_FAILED(rv)) return rv;

        nsCOMPtr<nsISupports> owner = do_QueryInterface(principal);
        result->SetOwner(owner);
    }

    *aResult = result;
    NS_ADDREF(*aResult);
    return NS_OK;
}

////////////////////////////////////////////////////////////////////////////////
