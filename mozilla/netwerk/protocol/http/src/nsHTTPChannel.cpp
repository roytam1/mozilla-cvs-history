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

#include "nsHTTPChannel.h"
#include "netCore.h"
#include "nsIHttpEventSink.h"
#include "nsIHTTPHandler.h"
#include "nsHTTPRequest.h"
#include "nsHTTPResponse.h"
#include "nsIChannel.h"
#include "nsIInputStream.h"
#include "nsIStreamListener.h"

#include "nsIHttpNotify.h"
#include "nsINetModRegEntry.h"
#include "nsProxyObjectManager.h"
#include "nsIServiceManager.h"
#include "nsINetModuleMgr.h"
#include "nsIEventQueueService.h"
static NS_DEFINE_IID(kEventQueueServiceCID, NS_EVENTQUEUESERVICE_CID);

#if 0
#include "nsCookieModTest.h"
#endif //0



nsHTTPChannel::nsHTTPChannel(nsIURI* i_URL, 
                             nsIHTTPEventSink* i_HTTPEventSink,
                             nsIHTTPHandler* i_Handler): 
    m_URI(dont_QueryInterface(i_URL)),
    m_bConnected(PR_FALSE),
    m_State(HS_IDLE),
    mRefCnt(0),
    m_pHandler(dont_QueryInterface(i_Handler)),
    m_pEventSink(dont_QueryInterface(i_HTTPEventSink)),
    m_pResponse(nsnull),
    m_pResponseDataListener(nsnull),
    mLoadAttributes(LOAD_NORMAL),
    mResponseContext(nsnull)
{
    NS_INIT_REFCNT();
}

nsHTTPChannel::~nsHTTPChannel()
{
    //TODO if we keep our copy of m_URI, then delete it too.
    NS_IF_RELEASE(m_pRequest);
    NS_IF_RELEASE(m_pResponse);

    NS_IF_RELEASE(m_pResponseDataListener);
}

NS_IMETHODIMP
nsHTTPChannel::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
    if (NULL == aInstancePtr)
        return NS_ERROR_NULL_POINTER;

    *aInstancePtr = NULL;
    
    if (aIID.Equals(nsCOMTypeInfo<nsIHTTPChannel>::GetIID()) ||
        aIID.Equals(nsCOMTypeInfo<nsIChannel>::GetIID()) ||
        aIID.Equals(nsCOMTypeInfo<nsISupports>::GetIID())) {
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
nsHTTPChannel::Cancel(void)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsHTTPChannel::Suspend(void)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsHTTPChannel::Resume(void)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

////////////////////////////////////////////////////////////////////////////////
// nsIChannel methods:

NS_IMETHODIMP
nsHTTPChannel::GetURI(nsIURI* *o_URL)
{
    if (o_URL) {
        *o_URL = m_URI;
        NS_IF_ADDREF(*o_URL);
        return NS_OK;
    } else {
        return NS_ERROR_NULL_POINTER;
    }
}

NS_IMETHODIMP
nsHTTPChannel::OpenInputStream(PRUint32 startPosition, PRInt32 readCount,
                               nsIInputStream **o_Stream)
{
#if 0
    nsresult rv;

    if (!m_bConnected)
        Open();

    nsIInputStream* inStr; // this guy gets passed out to the user
    rv = NS_NewSyncStreamListener(&m_pResponseDataListener, &inStr);
    if (NS_FAILED(rv)) return rv;

    *o_Stream = inStr;
    return NS_OK;

#else

    if (m_pResponse)
        return m_pResponse->GetInputStream(o_Stream);
    NS_ERROR("No response!");
    return NS_OK; // change to error ? or block till response is set up?
#endif // if 0

}

NS_IMETHODIMP
nsHTTPChannel::OpenOutputStream(PRUint32 startPosition, nsIOutputStream **_retval)
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
    if (m_pResponseDataListener) {
        rv = NS_ERROR_IN_PROGRESS;
    } 
    else if (!listener) {
        rv = NS_ERROR_NULL_POINTER;
    }

    // Initiate the loading of the URL...
    if (NS_SUCCEEDED(rv)) {
        m_pResponseDataListener = listener;
        NS_ADDREF(m_pResponseDataListener);

        mResponseContext = aContext;

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
    nsresult rv = NS_ERROR_FAILURE;

    if (m_pResponse) {
        rv = m_pResponse->GetContentType(aContentType);
    }

    // XXX temporary hack until we have a contenttype strategy
    if (NS_FAILED(rv)) {
        *aContentType = nsCRT::strdup(DUMMY_TYPE);
        if (!*aContentType) {
            rv = NS_ERROR_OUT_OF_MEMORY;
        } else {
            rv = NS_OK;
        }
    }
    return rv;

    //return NS_ERROR_NOT_IMPLEMENTED;
}

////////////////////////////////////////////////////////////////////////////////
// nsIHTTPChannel methods:

NS_IMETHODIMP
nsHTTPChannel::GetRequestHeader(const char* i_Header, char* *o_Value)
{
    NS_ASSERTION(m_pRequest, "The request object vanished from underneath the connection!");
    return m_pRequest->GetHeader(i_Header, o_Value);
}

NS_IMETHODIMP
nsHTTPChannel::SetRequestHeader(const char* i_Header, const char* i_Value)
{
    NS_ASSERTION(m_pRequest, "The request object vanished from underneath the connection!");
    return m_pRequest->SetHeader(i_Header, i_Value);
}

NS_IMETHODIMP
nsHTTPChannel::GetResponseHeader(const char* i_Header, char* *o_Value)
{
    if (!m_bConnected)
        Open();
    if (m_pResponse)
        return m_pResponse->GetHeader(i_Header, o_Value);
    else
        return NS_ERROR_NOT_IMPLEMENTED; // NS_ERROR_NO_RESPONSE_YET ? 
}

NS_IMETHODIMP
nsHTTPChannel::GetResponseStatus(nsresult *o_Status)
{
    PRInt32 status = -1;
    if (!m_bConnected) 
        Open();
    *o_Status = status;

    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsHTTPChannel::GetResponseString(char* *o_String) 
{
    if (!m_bConnected) 
        Open();
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsHTTPChannel::GetEventSink(nsIHTTPEventSink* *o_EventSink) 
{
    nsresult rv;

    if (o_EventSink) {
        *o_EventSink = m_pEventSink;
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
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsHTTPChannel::GetResponseDataListener(nsIStreamListener* *aListener)
{
    nsresult rv = NS_OK;

    if (aListener) {
        *aListener = m_pResponseDataListener;
        NS_IF_ADDREF(m_pResponseDataListener);
    } else {
        rv = NS_ERROR_NULL_POINTER;
    }

    return rv;
}

static NS_DEFINE_IID(kProxyObjectManagerIID, NS_IPROXYEVENT_MANAGER_IID);
static NS_DEFINE_CID(kEventQueueService, NS_EVENTQUEUESERVICE_CID);
static NS_DEFINE_CID(kNetModuleMgrCID, NS_NETMODULEMGR_CID);

#if 0
static NS_DEFINE_CID(kCookieTestCID, NS_COOKIEMODTEST_CID);
#endif
////////////////////////////////////////////////////////////////////////////////
// nsHTTPChannel methods:

nsresult
nsHTTPChannel::Init()
{
    //TODO think if we need to make a copy of the URL and keep it here
    //since it might get deleted off the creators thread. And the
    //stream listener could be elsewhere...

    /* 
        Set up a request object - later set to a clone of a default 
        request from the handler
    */
    m_pRequest = new nsHTTPRequest(m_URI);
    if (m_pRequest == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(m_pRequest);
    m_pRequest->SetConnection(this);
    return NS_OK;
}

nsresult
nsHTTPChannel::Open(void)
{
    if (m_bConnected || (m_State > HS_IDLE))
        return NS_ERROR_ALREADY_CONNECTED;

    // Set up a new request observer and a response listener and pass to teh transport
    nsresult rv = NS_OK;

    char* host;
    PRInt32 port;
    nsCOMPtr<nsIChannel> channel;

    rv = m_URI->GetHost(&host);
    if (NS_FAILED(rv)) return rv;

    rv = m_URI->GetPort(&port);
    if (NS_FAILED(rv)) return rv;
    if (port == -1)
    {
        m_pHandler->GetDefaultPort(&port);
    }

    NS_ASSERTION(port>0, "Bad port setting!");
    PRUint32 unsignedPort = port;

    // Check for any modules that want to set headers before we
    // send out a request.
    NS_WITH_SERVICE(nsINetModuleMgr, pNetModuleMgr, kNetModuleMgrCID, &rv);
    if (NS_FAILED(rv)) return rv;

#if 0
    nsCookieModTest *cookieTest = new nsCookieModTest();
    nsIHTTPNotify *cookieNot = nsnull;
    rv = cookieTest->QueryInterface(nsCOMTypeInfo<nsIHTTPNotify>::GetIID(), (void**)&cookieNot);
    if (NS_FAILED(rv)) return rv;

    nsIEventQueue* eventQa; 
    NS_WITH_SERVICE(nsIEventQueueService, eventQServicea, kEventQueueServiceCID, &rv); 
    if (NS_SUCCEEDED(rv)) {
        rv = eventQServicea->CreateThreadEventQueue();
        if (NS_FAILED(rv)) return rv;

        rv = eventQServicea->GetThreadEventQueue(PR_CurrentThread(), &eventQa); 
    } 
    if (NS_FAILED(rv)) return rv; 

    rv = pNetModuleMgr->RegisterModule("http-request", eventQa, cookieNot, &kCookieTestCID);
    if (NS_FAILED(rv)) return rv; 
#endif //0

    nsISimpleEnumerator* pModules = nsnull;
    rv = pNetModuleMgr->EnumerateModules("http-request", &pModules);
    if (NS_FAILED(rv)) return rv;

    nsIProxyObjectManager*  proxyObjectManager = nsnull; 
    rv = nsServiceManager::GetService( NS_XPCOMPROXY_PROGID, 
                                        kProxyObjectManagerIID,
                                        (nsISupports **)&proxyObjectManager);
    if (NS_FAILED(rv)) {
        NS_RELEASE(pModules);
        return rv;
    }

    nsISupports *supEntry = nsnull;

    // Go through the external modules and notify each one.
    rv = pModules->GetNext(&supEntry);
    while (NS_SUCCEEDED(rv)) {
        nsINetModRegEntry *entry = nsnull;
        rv = supEntry->QueryInterface(nsCOMTypeInfo<nsINetModRegEntry>::GetIID(), (void**)&entry);
        NS_RELEASE(supEntry);
        if (NS_FAILED(rv)) {
            NS_RELEASE(pModules);
            NS_RELEASE(proxyObjectManager);
            return rv;
        }

        nsCID *lCID;
        nsIEventQueue* lEventQ = nsnull;

        rv = entry->GetMCID(&lCID);
        if (NS_FAILED(rv)) {
            NS_RELEASE(pModules);
            NS_RELEASE(proxyObjectManager);
            return rv;
        }

        rv = entry->GetMEventQ(&lEventQ);
        if (NS_FAILED(rv)) {
            NS_RELEASE(pModules);
            NS_RELEASE(proxyObjectManager);            
            return rv;
        }

        nsIHTTPNotify *pNotify = nsnull;
        // if this call fails one of the following happened.
        // a) someone registered an object for this topic but didn't
        //    implement the nsIHTTPNotify interface on that object.
        // b) someone registered an object for this topic bud didn't
        //    put the .xpt lib for that object in the components dir
        rv = proxyObjectManager->GetProxyObject(lEventQ, 
                                           *lCID,
                                           nsnull,
                                           nsCOMTypeInfo<nsIHTTPNotify>::GetIID(),
                                           PROXY_SYNC,
                                           (void**)&pNotify);
        NS_RELEASE(proxyObjectManager);
        
        NS_RELEASE(lEventQ);

        if (NS_SUCCEEDED(rv)) {
            // send off the notification, and block.

            // make the nsIHTTPNotify api call
            pNotify->ModifyRequest(this);
            NS_RELEASE(pNotify);
            // we could do something with the return code from the external
            // module, but what????            
        }

        NS_RELEASE(entry);
        rv = pModules->GetNext(&supEntry); // go around again
    }
    NS_RELEASE(pModules);
    NS_IF_RELEASE(proxyObjectManager);

    rv = m_pHandler->GetTransport(host, unsignedPort, getter_AddRefs(channel));
    nsCRT::free(host);
    if (NS_SUCCEEDED(rv) && channel)
    {
        nsCOMPtr<nsIInputStream> stream;

        m_pRequest->SetTransport(channel);

        //Get the stream where it will read the request data from
        rv = m_pRequest->GetInputStream(getter_AddRefs(stream));
        if (NS_SUCCEEDED(rv) && stream) {
            PRUint32 count;

            // Write the request to the server...
            rv = stream->GetLength(&count);
            rv = channel->AsyncWrite(stream, 0, count, this , m_pRequest);
            if (NS_FAILED(rv)) return rv;

            m_State = HS_WAITING_FOR_RESPONSE;
            m_bConnected = PR_TRUE;
        } else {
            NS_ERROR("Failed to get request Input stream.");
            return NS_ERROR_FAILURE;
        }
    }
    else
        NS_ERROR("Failed to create/get a transport!");

    return rv;
}

nsresult
nsHTTPChannel::SetResponse(nsHTTPResponse* i_pResp)
{ 
  NS_IF_RELEASE(m_pResponse);
  m_pResponse = i_pResp;
  NS_IF_ADDREF(m_pResponse);

  return NS_OK;
}

nsresult
nsHTTPChannel::GetResponseContext(nsISupports** aContext)
{
  if (aContext) {
    *aContext = mResponseContext;
    NS_IF_ADDREF(*aContext);
    return NS_OK;
  }

  return NS_ERROR_NULL_POINTER;
}

////////////////////////////////////////////////////////////////////////////////
