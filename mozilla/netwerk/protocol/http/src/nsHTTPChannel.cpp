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

#include "nspr.h"
#include "nsHTTPChannel.h"
#include "netCore.h"
#include "nsIHttpEventSink.h"
#include "nsIHTTPProtocolHandler.h"
#include "nsHTTPRequest.h"
#include "nsHTTPResponse.h"
#include "nsIEventSinkGetter.h"
#include "nsIChannel.h"
#include "nsIInputStream.h"
#include "nsIStreamListener.h"
#include "nsIIOService.h"
#include "nsXPIDLString.h"
#include "nsHTTPAtoms.h"

#include "nsIHttpNotify.h"
#include "nsINetModRegEntry.h"
#include "nsProxyObjectManager.h"
#include "nsIServiceManager.h"
#include "nsINetModuleMgr.h"
#include "nsIEventQueueService.h"
#include "nsIMIMEService.h"
#include "nsIEnumerator.h"

// Once other kinds of auth are up change TODO
#include "nsBasicAuth.h" 
// This will go away once the dialog box starts popping off the window that triggered the load. 
#include "nsAppShellCIDs.h" // TODO remove later
#include "nsIAppShellService.h" // TODO remove later
#include "nsIWebShellWindow.h" // TODO remove later
static NS_DEFINE_CID(kAppShellServiceCID, NS_APPSHELL_SERVICE_CID);
static NS_DEFINE_CID(kProxyObjectManagerCID, NS_PROXYEVENT_MANAGER_CID);
#include "nsINetPrompt.h"
#include "nsProxiedService.h"

#if defined(PR_LOGGING)
extern PRLogModuleInfo* gHTTPLog;
#endif /* PR_LOGGING */

static NS_DEFINE_CID(kMIMEServiceCID, NS_MIMESERVICE_CID);
static NS_DEFINE_IID(kEventQueueServiceCID, NS_EVENTQUEUESERVICE_CID);

nsHTTPChannel::nsHTTPChannel(nsIURI* i_URL, 
                             const char *i_Verb,
                             nsIEventSinkGetter* i_EventSinkGetter,
                             nsIURI* originalURI,
                             nsHTTPHandler* i_Handler): 
    mOriginalURI(dont_QueryInterface(originalURI ? originalURI : i_URL)),
    mURI(dont_QueryInterface(i_URL)),
    mConnected(PR_FALSE),
    mHandler(dont_QueryInterface(i_Handler)),
    mState(HS_IDLE),
    mEventSinkGetter(dont_QueryInterface(i_EventSinkGetter)),
    mResponse(nsnull),
    mResponseDataListener(nsnull),
    mLoadAttributes(LOAD_NORMAL),
    mResponseContext(nsnull),
    mLoadGroup(nsnull),
    mAuthTriedWithPrehost(PR_FALSE),
    mUsingProxy(PR_FALSE)
{
    NS_INIT_REFCNT();

    PR_LOG(gHTTPLog, PR_LOG_ALWAYS, 
           ("Creating nsHTTPChannel [this=%x].\n", this));

    // The content length is unknown...
    mContentLength = -1;

    mVerb = i_Verb;

    // Verify that the event sink is http
    if (i_EventSinkGetter) {
        nsIHTTPEventSink *sink = nsnull;

        (void) i_EventSinkGetter->GetEventSink(i_Verb, NS_GET_IID(nsIHTTPEventSink),
                                               (nsISupports**)&sink);
        mEventSink = sink;
        NS_IF_RELEASE(sink);
    }

}

nsHTTPChannel::~nsHTTPChannel()
{
    PR_LOG(gHTTPLog, PR_LOG_ALWAYS, 
           ("Deleting nsHTTPChannel [this=%x].\n", this));

    //TODO if we keep our copy of mURI, then delete it too.
    NS_RELEASE(mRequest);
    NS_IF_RELEASE(mResponse);
    NS_IF_RELEASE(mResponseDataListener);

    mHandler         = null_nsCOMPtr();
    mEventSink       = null_nsCOMPtr();
    mResponseContext = null_nsCOMPtr();
    mLoadGroup       = null_nsCOMPtr();
}

NS_IMETHODIMP
nsHTTPChannel::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
    if (NULL == aInstancePtr)
        return NS_ERROR_NULL_POINTER;

    *aInstancePtr = NULL;
    
    if (aIID.Equals(NS_GET_IID(nsIHTTPChannel)) ||
        aIID.Equals(NS_GET_IID(nsIChannel)) ||
        aIID.Equals(NS_GET_IID(nsISupports))) {
        *aInstancePtr = NS_STATIC_CAST(nsIHTTPChannel*, this);
        NS_ADDREF_THIS();
        return NS_OK;
    }
    return NS_NOINTERFACE;
}
 
NS_IMPL_ADDREF(nsHTTPChannel);
NS_IMPL_RELEASE(nsHTTPChannel);

////////////////////////////////////////////////////////////////////////////////
// nsIRequest methods:

NS_IMETHODIMP
nsHTTPChannel::IsPending(PRBool *result)
{
  return mRequest->IsPending(result);
}

NS_IMETHODIMP
nsHTTPChannel::Cancel(void)
{
  nsresult rv;

  //
  // If this channel is currently waiting for a transport to become available.
  // Notify the HTTPHandler that this request has been cancelled...
  //
  if (!mConnected && (HS_WAITING_FOR_OPEN == mState)) {
    rv = mHandler->CancelPendingChannel(this);
  }

  return mRequest->Cancel();
}

NS_IMETHODIMP
nsHTTPChannel::Suspend(void)
{
  return mRequest->Suspend();
}

NS_IMETHODIMP
nsHTTPChannel::Resume(void)
{
  return mRequest->Resume();
}

////////////////////////////////////////////////////////////////////////////////
// nsIChannel methods:

NS_IMETHODIMP
nsHTTPChannel::GetOriginalURI(nsIURI* *o_URL)
{
    *o_URL = mOriginalURI;
    NS_IF_ADDREF(*o_URL);
    return NS_OK;
}

NS_IMETHODIMP
nsHTTPChannel::GetURI(nsIURI* *o_URL)
{
    *o_URL = mURI;
    NS_IF_ADDREF(*o_URL);
    return NS_OK;
}

NS_IMETHODIMP
nsHTTPChannel::OpenInputStream(PRUint32 startPosition, PRInt32 readCount,
                               nsIInputStream **o_Stream)
{
#if 0
    nsresult rv;

    if (!mConnected)
        Open();

    nsIInputStream* inStr; // this guy gets passed out to the user
    rv = NS_NewSyncStreamListener(&mResponseDataListener, &inStr);
    if (NS_FAILED(rv)) return rv;

    *o_Stream = inStr;
    return NS_OK;

#else
    return NS_ERROR_NOT_IMPLEMENTED;
#endif // if 0

}

NS_IMETHODIMP
nsHTTPChannel::OpenOutputStream(PRUint32 startPosition, nsIOutputStream **_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsHTTPChannel::AsyncOpen(nsIStreamObserver *observer, nsISupports* ctxt)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsHTTPChannel::AsyncRead(PRUint32 startPosition, PRInt32 readCount,
                         nsISupports *aContext,
                         nsIStreamListener *listener)
{
    nsresult rv = NS_OK;

    // Initial parameter checks...
    if (mResponseDataListener) {
        rv = NS_ERROR_IN_PROGRESS;
    } 
    else if (!listener) {
        rv = NS_ERROR_NULL_POINTER;
    }

    // Initiate the loading of the URL...
    if (NS_SUCCEEDED(rv)) {
        mResponseDataListener = listener;
        NS_ADDREF(mResponseDataListener);

        mResponseContext = do_QueryInterface(aContext);

        rv = Open();
    }

    return rv;
}

NS_IMETHODIMP
nsHTTPChannel::AsyncWrite(nsIInputStream *fromStream,
                          PRUint32 startPosition,
                          PRInt32 writeCount,
                          nsISupports *ctxt,
                          nsIStreamObserver *observer)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsHTTPChannel::GetLoadAttributes(PRUint32 *aLoadAttributes)
{
    *aLoadAttributes = mLoadAttributes;
    return NS_OK;
}

NS_IMETHODIMP
nsHTTPChannel::SetLoadAttributes(PRUint32 aLoadAttributes)
{
    mLoadAttributes = aLoadAttributes;
    return NS_OK;
}

#define DUMMY_TYPE "text/html"

NS_IMETHODIMP
nsHTTPChannel::GetContentType(char * *aContentType)
{
    nsresult rv = NS_OK;

    // Parameter validation...
    if (!aContentType) {
        return NS_ERROR_NULL_POINTER;
    }
    *aContentType = nsnull;

    //
    // If the content type has been returned by the server then return that...
    //
    if (mContentType.Length()) {
        *aContentType = mContentType.ToNewCString();
        if (!*aContentType) {
            rv = NS_ERROR_OUT_OF_MEMORY;
        }
        return rv;
    }

    //
    // No response yet...  Try to determine the content-type based
    // on the file extension of the URI...
    //
    NS_WITH_SERVICE(nsIMIMEService, MIMEService, kMIMEServiceCID, &rv);
    if (NS_SUCCEEDED(rv)) {
        rv = MIMEService->GetTypeFromURI(mURI, aContentType);
        if (NS_SUCCEEDED(rv)) return rv;
        // XXX we should probably set the content-type for this http response at this stage too.
    }

    // if all else fails treat it as text/html?
    if (!*aContentType) 
        *aContentType = nsCRT::strdup(DUMMY_TYPE);
    if (!*aContentType) {
        rv = NS_ERROR_OUT_OF_MEMORY;
    } else {
        rv = NS_OK;
    }

    return rv;
}

NS_IMETHODIMP
nsHTTPChannel::GetContentLength(PRInt32 *aContentLength)
{
    *aContentLength = mContentLength;
    return NS_OK;
}


NS_IMETHODIMP
nsHTTPChannel::GetLoadGroup(nsILoadGroup * *aLoadGroup)
{
    *aLoadGroup = mLoadGroup;
    NS_IF_ADDREF(*aLoadGroup);
    return NS_OK;
}

NS_IMETHODIMP
nsHTTPChannel::GetOwner(nsISupports * *aOwner)
{
    *aOwner = mOwner.get();
    NS_IF_ADDREF(*aOwner);
    return NS_OK;
}

NS_IMETHODIMP
nsHTTPChannel::SetOwner(nsISupports * aOwner)
{
    mOwner = aOwner;
    return NS_OK;
}

////////////////////////////////////////////////////////////////////////////////
// nsIHTTPChannel methods:

NS_IMETHODIMP
nsHTTPChannel::GetRequestHeader(nsIAtom* i_Header, char* *o_Value)
{
    return mRequest->GetHeader(i_Header, o_Value);
}

NS_IMETHODIMP
nsHTTPChannel::SetRequestHeader(nsIAtom* i_Header, const char* i_Value)
{
    return mRequest->SetHeader(i_Header, i_Value);
}

NS_IMETHODIMP
nsHTTPChannel::GetRequestHeaderEnumerator(nsISimpleEnumerator** aResult)
{
    return mRequest->GetHeaderEnumerator(aResult);
}


NS_IMETHODIMP
nsHTTPChannel::GetResponseHeader(nsIAtom* i_Header, char* *o_Value)
{
    if (!mConnected)
        Open();
    if (mResponse)
        return mResponse->GetHeader(i_Header, o_Value);
    else
        return NS_ERROR_FAILURE ; // NS_ERROR_NO_RESPONSE_YET ? 
}


NS_IMETHODIMP
nsHTTPChannel::GetResponseHeaderEnumerator(nsISimpleEnumerator** aResult)
{
    nsresult rv;

    if (!mConnected) {
        Open();
    }

    if (mResponse) {
        rv = mResponse->GetHeaderEnumerator(aResult);
    } else {
        *aResult = nsnull;
        rv = NS_ERROR_FAILURE ; // NS_ERROR_NO_RESPONSE_YET ? 
    }
    return rv;
}

NS_IMETHODIMP
nsHTTPChannel::SetResponseHeader(nsIAtom* i_Header, const char* i_HeaderValue) {
    nsresult rv = NS_OK;

    if (!mConnected) {
        rv = Open();
        if (NS_FAILED(rv)) return rv;
    }

    if (mResponse) {
        // we need to set the header and ensure that observers are notified again.
        rv = mResponse->SetHeader(i_Header, i_HeaderValue);
        if (NS_FAILED(rv)) return rv;

        rv = OnHeadersAvailable();

    } else {
        rv = NS_ERROR_FAILURE;
    }
    return rv;
}

NS_IMETHODIMP
nsHTTPChannel::GetResponseStatus(PRUint32  *o_Status)
{
    if (!mConnected) 
        Open();
    if (mResponse)
        return mResponse->GetStatus(o_Status);
    return NS_ERROR_FAILURE; // NS_ERROR_NO_RESPONSE_YET ? 
}

NS_IMETHODIMP
nsHTTPChannel::GetResponseString(char* *o_String) 
{
    if (!mConnected) 
        Open();
    if (mResponse)
        return mResponse->GetStatusString(o_String);
    return NS_ERROR_FAILURE; // NS_ERROR_NO_RESPONSE_YET ? 
}

NS_IMETHODIMP
nsHTTPChannel::GetEventSink(nsIHTTPEventSink* *o_EventSink) 
{
    nsresult rv;

    if (o_EventSink) {
        *o_EventSink = mEventSink;
        NS_IF_ADDREF(*o_EventSink);
        rv = NS_OK;
    } else {
        rv = NS_ERROR_NULL_POINTER;
    }
    return rv;
}


NS_IMETHODIMP
nsHTTPChannel::SetRequestMethod(PRUint32/*HTTPMethod*/ i_Method)
{
    return mRequest->SetMethod((HTTPMethod)i_Method);
}

NS_IMETHODIMP
nsHTTPChannel::GetResponseDataListener(nsIStreamListener* *aListener)
{
    nsresult rv = NS_OK;

    if (aListener) {
        *aListener = mResponseDataListener;
        NS_IF_ADDREF(mResponseDataListener);
    } else {
        rv = NS_ERROR_NULL_POINTER;
    }

    return rv;
}

NS_IMETHODIMP
nsHTTPChannel::GetCharset(char* *o_String)
{
  nsresult rv = NS_OK;

  *o_String = mCharset.ToNewCString();
  if (!*o_String) {
    rv = NS_ERROR_OUT_OF_MEMORY;
  }

  return rv;
}

NS_IMETHODIMP
nsHTTPChannel::SetPostDataStream(nsIInputStream* aPostStream)
{
  return mRequest->SetPostDataStream(aPostStream);
}

NS_IMETHODIMP
nsHTTPChannel::GetPostDataStream(nsIInputStream **o_postStream)
{ 
  return mRequest->GetPostDataStream(o_postStream);
}

NS_IMETHODIMP
nsHTTPChannel::SetAuthTriedWithPrehost(PRBool iTried)
{
    mAuthTriedWithPrehost = iTried;
    return NS_OK;
}

NS_IMETHODIMP
nsHTTPChannel::GetAuthTriedWithPrehost(PRBool* oTried)
{
    if (oTried)
    {
        *oTried = mAuthTriedWithPrehost;
        return NS_OK;
    }
    else
        return NS_ERROR_NULL_POINTER;
}


static NS_DEFINE_IID(kProxyObjectManagerIID, NS_IPROXYEVENT_MANAGER_IID);
static NS_DEFINE_CID(kEventQueueService, NS_EVENTQUEUESERVICE_CID);
static NS_DEFINE_CID(kNetModuleMgrCID, NS_NETMODULEMGR_CID);
static NS_DEFINE_CID(kIOServiceCID, NS_IOSERVICE_CID);


////////////////////////////////////////////////////////////////////////////////
// nsHTTPChannel methods:

nsresult
nsHTTPChannel::Init(nsILoadGroup *aGroup)
{
    //TODO think if we need to make a copy of the URL and keep it here
    //since it might get deleted off the creators thread. And the
    //stream listener could be elsewhere...

    /* 
        Set up a request object - later set to a clone of a default 
        request from the handler. TODO
    */
    nsresult rv;

    //
    // Initialize the load group and copy any default load attributes...
    //
    mLoadGroup = aGroup;
    if (mLoadGroup) {
        mLoadGroup->GetDefaultLoadAttributes(&mLoadAttributes);
    }

    mRequest = new nsHTTPRequest(mURI);
    if (mRequest == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(mRequest);
    mRequest->SetConnection(this);
    NS_WITH_SERVICE(nsIIOService, service, kIOServiceCID, &rv);
    if (NS_FAILED(rv)) return rv;
    PRUnichar * ua = nsnull;
    rv = service->GetUserAgent(&ua);
    if (NS_FAILED(rv)) return rv;
    nsCString uaString(ua);
    nsCRT::free(ua);
    mRequest->SetHeader(nsHTTPAtoms::User_Agent, uaString.GetBuffer());
    return NS_OK;
}

nsresult
nsHTTPChannel::Open(void)
{
    if (mConnected || (mState > HS_WAITING_FOR_OPEN))
        return NS_ERROR_ALREADY_CONNECTED;

    // Set up a new request observer and a response listener and pass 
    // to the transport
    nsresult rv = NS_OK;
    nsCOMPtr<nsIChannel> transport;

    // If this is the first time, then add the channel to its load group
    if (mState == HS_IDLE) {
      if (mLoadGroup) {
        nsCOMPtr<nsILoadGroupListenerFactory> factory;
        //
        // Create a load group "proxy" listener...
        //
        rv = mLoadGroup->GetGroupListenerFactory(getter_AddRefs(factory));
        if (factory) {
          nsIStreamListener *newListener;
          rv = factory->CreateLoadGroupListener(mResponseDataListener, 
                                                &newListener);
          if (NS_SUCCEEDED(rv)) {
            NS_RELEASE(mResponseDataListener);
            // Already AddRefed from the factory...
            mResponseDataListener = newListener;
          }
        }
        mLoadGroup->AddChannel(this, nsnull);
      }
    }
     
    rv = mHandler->RequestTransport(mURI, 
                        this, 
                        mEventSinkGetter, 
                        getter_AddRefs(transport));

    if (NS_ERROR_BUSY == rv) {
        mState = HS_WAITING_FOR_OPEN;
        return NS_OK;
    }
    if (NS_FAILED(rv)) {
      // Unable to create a transport...  End the request...
      (void) ResponseCompleted(nsnull, rv, nsnull);
      return rv;
    }
    // Check for any modules that want to set headers before we
    // send out a request.
    NS_WITH_SERVICE(nsINetModuleMgr, pNetModuleMgr, kNetModuleMgrCID, &rv);
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsISimpleEnumerator> pModules;
    rv = pNetModuleMgr->EnumerateModules(NS_NETWORK_MODULE_MANAGER_HTTP_REQUEST_PROGID, getter_AddRefs(pModules));
    if (NS_FAILED(rv)) return rv;

    // Go through the external modules and notify each one.
    nsISupports *supEntry;
    rv = pModules->GetNext(&supEntry);
    while (NS_SUCCEEDED(rv)) 
    {
        nsCOMPtr<nsINetModRegEntry> entry = do_QueryInterface(supEntry, &rv);
        if (NS_FAILED(rv)) 
            return rv;

        nsCOMPtr<nsINetNotify> syncNotifier;
        entry->GetSyncProxy(getter_AddRefs(syncNotifier));
        nsCOMPtr<nsIHTTPNotify> pNotify = do_QueryInterface(syncNotifier, &rv);

        if (NS_SUCCEEDED(rv)) 
        {
            // send off the notification, and block.
            // make the nsIHTTPNotify api call
            pNotify->ModifyRequest(this);
            // we could do something with the return code from the external
            // module, but what????            
        }
        rv = pModules->GetNext(&supEntry); // go around again
    }

    if (transport) {
        rv = mRequest->WriteRequest(transport, mUsingProxy);
        if (NS_FAILED(rv)) return rv;

        mState = HS_WAITING_FOR_RESPONSE;
        mConnected = PR_TRUE;
    }
    else
        NS_ERROR("Failed to create/get a transport!");

    return rv;
}


nsresult nsHTTPChannel::Redirect(const char *aNewLocation, 
                                 nsIChannel **aResult)
{
  nsresult rv;
  nsCOMPtr<nsIURI> newURI;
  nsCOMPtr<nsIChannel> channel;

  *aResult = nsnull;

  //
  // Create a new URI using the Location header and the current URL 
  // as a base ...
  //
  NS_WITH_SERVICE(nsIIOService, serv, kIOServiceCID, &rv);
  if (NS_FAILED(rv)) return rv;
    
  rv = serv->NewURI(aNewLocation, mURI, getter_AddRefs(newURI));
  if (NS_FAILED(rv)) return rv;

  //
  // Move the Reference of the old location to the new one 
  // if the new one has none
  //

  nsXPIDLCString   newref;
  nsCOMPtr<nsIURL> newurl;
  
  newurl = do_QueryInterface(newURI, &rv);
  if (NS_SUCCEEDED(rv)) {
    rv = newurl->GetRef(getter_Copies(newref));
    if (NS_SUCCEEDED(rv) && !newref) {
      nsXPIDLCString   baseref;
      nsCOMPtr<nsIURL> baseurl;

      baseurl = do_QueryInterface(mURI, &rv);
      if (NS_SUCCEEDED(rv)) {
        rv = baseurl->GetRef(getter_Copies(baseref));
        if (NS_SUCCEEDED(rv) && baseref) {
          // If the old URL had a reference and the new URL does not,
          // then move it to the new URL...
          newurl->SetRef(baseref);                
        }
      }
    }
  }

#if defined(PR_LOGGING)
  char *newURLSpec;

  newURLSpec = nsnull;
  newURI->GetSpec(&newURLSpec);
  PR_LOG(gHTTPLog, PR_LOG_ALWAYS, 
         ("ProcessRedirect [this=%x].\tRedirecting to: %s.\n",
          this, newURLSpec));
#endif /* PR_LOGGING */

  rv = serv->NewChannelFromURI(mVerb.GetBuffer(), newURI, mLoadGroup, mEventSinkGetter, 
                               mOriginalURI, getter_AddRefs(channel));
  if (NS_FAILED(rv)) return rv;

  // Copy the load attributes into the new channel...
  channel->SetLoadAttributes(mLoadAttributes);

  // Start the redirect...
  rv = channel->AsyncRead(0, -1, mResponseContext, mResponseDataListener);

  //
  // Fire the OnRedirect(...) notification.
  //
  if (mEventSink) {
    mEventSink->OnRedirect(this, newURI);
  }


  *aResult = channel;
  NS_ADDREF(*aResult);

  return rv;
}


nsresult nsHTTPChannel::ResponseCompleted(nsIChannel* aTransport, 
                                          nsresult aStatus,
                                          const PRUnichar* aMsg)
{
  nsresult rv = NS_OK;

  PRBool bPropogateOnStop = PR_TRUE;

  // Don't fire OnStops for redirects-- confuses imagelib. See #17393
  if (mResponse && (aStatus == NS_OK))
  {
    PRUint32 resp;
    mResponse->GetStatus(&resp);
    if (3 == (int)(resp/100)) // 3* is the redirects
        bPropogateOnStop = PR_FALSE;
  }

  // Call the consumer OnStopRequest(...) to end the request...
  if (bPropogateOnStop && mResponseDataListener) {
    rv = mResponseDataListener->OnStopRequest(this, 
                                              mResponseContext, 
                                              aStatus, 
                                              aMsg);

    if (NS_FAILED(rv)) {
      PR_LOG(gHTTPLog, PR_LOG_ERROR, 
             ("nsHTTPChannel::ResponseCompleted(...) [this=%x]."
              "\tOnStopRequest to consumer failed! Status:%x\n",
             this, rv));
    }
  }

  // Null out pointers that are no longer needed...
  mResponseContext = null_nsCOMPtr();
  NS_IF_RELEASE(mResponseDataListener);

  // Release the transport...
  if (aTransport) {
    (void)mHandler->ReleaseTransport(aTransport);
  }

  // Remove the channel from its load group...
  if (mLoadGroup) {
    (void)mLoadGroup->RemoveChannel(this, nsnull, aStatus, nsnull);
  }
  return rv;
}

nsresult nsHTTPChannel::SetResponse(nsHTTPResponse* i_pResp)
{ 
  NS_IF_RELEASE(mResponse);
  mResponse = i_pResp;
  NS_IF_ADDREF(mResponse);

  return NS_OK;
}

nsresult nsHTTPChannel::GetResponseContext(nsISupports** aContext)
{
  if (aContext) {
    *aContext = mResponseContext;
    NS_IF_ADDREF(*aContext);
    return NS_OK;
  }

  return NS_ERROR_NULL_POINTER;
}

nsresult nsHTTPChannel::SetContentLength(PRInt32 aContentLength)
{
    mContentLength = aContentLength;
    return NS_OK;
}

nsresult nsHTTPChannel::SetContentType(const char* aContentType)
{
    nsCAutoString cType(aContentType);
    cType.ToLowerCase();
    mContentType = cType.GetBuffer();
    return NS_OK;
}


nsresult nsHTTPChannel::SetCharset(const char *aCharset)
{
  mCharset = aCharset;
  return NS_OK;
}

nsresult nsHTTPChannel::OnHeadersAvailable()
{
    nsresult rv = NS_OK;

    // Notify the event sink that response headers are available...
    if (mEventSink) {
        rv = mEventSink->OnHeadersAvailable(this);
        if (NS_FAILED(rv)) return rv;
    }

    // Check for any modules that want to receive headers once they've arrived.
    NS_WITH_SERVICE(nsINetModuleMgr, pNetModuleMgr, kNetModuleMgrCID, &rv);
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsISimpleEnumerator> pModules;
    rv = pNetModuleMgr->EnumerateModules(NS_NETWORK_MODULE_MANAGER_HTTP_REQUEST_PROGID, getter_AddRefs(pModules));
    if (NS_FAILED(rv)) return rv;

    // Go through the external modules and notify each one.
    nsISupports *supEntry;
    rv = pModules->GetNext(&supEntry);
    while (NS_SUCCEEDED(rv)) 
    {
        nsCOMPtr<nsINetModRegEntry> entry = do_QueryInterface(supEntry, &rv);
        if (NS_FAILED(rv)) 
            return rv;

        nsCOMPtr<nsINetNotify> syncNotifier;
        entry->GetSyncProxy(getter_AddRefs(syncNotifier));
        nsCOMPtr<nsIHTTPNotify> pNotify = do_QueryInterface(syncNotifier, &rv);

        if (NS_SUCCEEDED(rv)) 
        {
            // send off the notification, and block.
            // make the nsIHTTPNotify api call
            pNotify->AsyncExamineResponse(this);
            // we could do something with the return code from the external
            // module, but what????            
        }
        rv = pModules->GetNext(&supEntry); // go around again
    }
    return NS_OK;
}


nsresult 
nsHTTPChannel::Authenticate(const char *iChallenge, nsIChannel **oChannel)
{
    nsresult rv = NS_ERROR_FAILURE;
    nsCOMPtr <nsIChannel> channel;
    if (!oChannel || !iChallenge)
        return NS_ERROR_NULL_POINTER;
    
    *oChannel = nsnull; // Initialize...

    // Determine the new username password combination to use 
    char* newUserPass = nsnull;
    if (!mAuthTriedWithPrehost)
    {
        nsXPIDLCString prehost;
        
        if (NS_SUCCEEDED(rv = mURI->GetPreHost(getter_Copies(prehost))))
        {
            if (!(newUserPass = nsCRT::strdup(prehost)))
                return NS_ERROR_OUT_OF_MEMORY;
        }
    }

    // Couldnt get one from prehost or has already been tried so...ask
    if (!newUserPass || (0==PL_strlen(newUserPass)))
    {
        /*
            Throw a modal dialog box asking for 
            username, password. Prefill (!?!)
        */
        /* 
            Currently this is being thrown from here itself. 
            The correct way to do this is to push this on the 
            HTTPEventSink and let that notify the window that
            triggered this load to throw the userpass dialog
        */
      nsresult rv;
      NS_WITH_SERVICE(nsIAppShellService, appshellservice, kAppShellServiceCID, &rv);
      if(NS_FAILED(rv))
          return rv;
      nsCOMPtr<nsIWebShellWindow> webshellwindow;
      appshellservice->GetHiddenWindow(getter_AddRefs( webshellwindow ) );
      nsCOMPtr<nsINetPrompt> prompter( do_QueryInterface( webshellwindow ) );
      
      NS_WITH_SERVICE(nsIProxyObjectManager, pIProxyObjectManager, kProxyObjectManagerCID, &rv);
      if(NS_FAILED(rv))
        return rv;
      nsINetPrompt* proxyprompter = NULL;
      rv = pIProxyObjectManager->GetProxyObject(nsnull, 
                                                              nsINetPrompt::GetIID(), 
                                                              prompter, PROXY_SYNC,
                                                              (void**)&proxyprompter);

        PRUnichar *user, *passwd;
        PRBool retval;

        nsAutoString message = "Enter username for "; //TODO localize it!
		message += iChallenge; // later on change to only show realm and then host's info. 
		const PRUnichar* msg = message.GetUnicode();
		
		// Get url
		 nsXPIDLCString urlCString; 
        mURI->GetHost(getter_Copies(urlCString));
		
		rv = proxyprompter->PromptUsernameAndPassword(urlCString, NULL, 
            msg, &user, &passwd, &retval);
            
   	 	proxyprompter->Release();  // Must be done as not managed for you.
       
        if (retval)
        {
            nsAutoString temp(user);
            temp += ':';
            temp += passwd;
            CRTFREEIF(newUserPass);
            newUserPass = temp.ToNewCString();
        }
    }

    // Construct the auth string request header based on info provided. 
    nsXPIDLCString authString;
    // change this later to include other kinds of authentication. TODO 
    if (NS_FAILED(rv = nsBasicAuth::Authenticate(
                        mURI, 
                        NS_STATIC_CAST(const char*, iChallenge), 
                        NS_STATIC_CAST(const char*, newUserPass),
                        getter_Copies(authString))))
        return rv; // Failed to construct an authentication string.

    // Construct a new channel
    NS_WITH_SERVICE(nsIIOService, serv, kIOServiceCID, &rv);
    if (NS_FAILED(rv)) return rv;

    // This smells like a clone function... maybe there is a benefit in doing that, think. TODO.
    rv = serv->NewChannelFromURI(mVerb.GetBuffer(), mURI, 
                                mLoadGroup, mEventSinkGetter, mOriginalURI,
                                getter_AddRefs(channel));
    if (NS_FAILED(rv)) return rv; 

    // Copy the load attributes into the new channel...
    channel->SetLoadAttributes(mLoadAttributes);

    nsCOMPtr<nsIHTTPChannel> httpChannel(do_QueryInterface(channel));
    NS_ASSERTION(httpChannel, "Something terrible happened..!");
    if (!httpChannel)
        return rv;

    // Add the authentication header.
    httpChannel->SetRequestHeader(nsHTTPAtoms::Authorization, authString);

    // Let it know that we have already tried prehost stuff...
    httpChannel->SetAuthTriedWithPrehost(PR_TRUE);

    // Fire the new request...
    rv = channel->AsyncRead(0, -1, mResponseContext, mResponseDataListener);
    *oChannel = channel;
    NS_ADDREF(*oChannel);
    return rv;
}

nsresult
nsHTTPChannel::SetUsingProxy(PRBool i_UsingProxy)
{
    mUsingProxy = i_UsingProxy;
    return NS_OK;
}
