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

#include "nspr.h"
#include "nsHTTPHandler.h"
#include "nsHTTPChannel.h"
//#include "nsITimer.h" 
#include "nsIProxy.h"
#include "plstr.h" // For PL_strcasecmp maybe DEBUG only... TODO check
#include "nsIURL.h"
#include "nsSocketKey.h"
#include "nsIChannel.h"
#include "nsISocketTransportService.h"
#include "nsIServiceManager.h"
#include "nsIEventSinkGetter.h"
#include "nsIHttpEventSink.h"

#if defined(PR_LOGGING)
//
// Log module for HTTP Protocol logging...
//
// To enable logging (see prlog.h for full details):
//
//    set NSPR_LOG_MODULES=nsHTTPProtocol:5
//    set NSPR_LOG_FILE=nspr.log
//
// this enables PR_LOG_DEBUG level information and places all output in
// the file nspr.log
//
PRLogModuleInfo* gHTTPLog = nsnull;

#endif /* PR_LOGGING */

static NS_DEFINE_CID(kStandardUrlCID, NS_STANDARDURL_CID);
static NS_DEFINE_CID(kSocketTransportServiceCID, NS_SOCKETTRANSPORTSERVICE_CID);

NS_METHOD CreateOrGetHTTPHandler(nsIHTTPHandler* *o_HTTPHandler)
{
#if defined(PR_LOGGING)
    //
    // Initialize the global PRLogModule for HTTP Protocol logging 
    // if necessary...
    //
    if (nsnull == gHTTPLog) {
        gHTTPLog = PR_NewLogModule("nsHTTPProtocol");
    }
#endif /* PR_LOGGING */

    if (o_HTTPHandler)
    {
        *o_HTTPHandler = nsHTTPHandler::GetInstance();
        return NS_OK;
    }
    return NS_ERROR_NULL_POINTER;
}

nsHTTPHandler::nsHTTPHandler():
    m_pTransportTable(new nsHashtable())
{
    NS_INIT_REFCNT();

    PR_LOG(gHTTPLog, PR_LOG_DEBUG, 
           ("Creating nsHTTPHandler [this=%x].\n", this));

    if (NS_FAILED(NS_NewISupportsArray(getter_AddRefs(m_pConnections)))) {
        NS_ERROR("unable to create new ISupportsArray");
    }
    if (!m_pTransportTable)
        NS_ERROR("Failed to create a new transport table");
}

nsHTTPHandler::~nsHTTPHandler()
{
    PR_LOG(gHTTPLog, PR_LOG_DEBUG, 
           ("Deleting nsHTTPHandler [this=%x].\n", this));

    if (m_pTransportTable)
    {
        delete m_pTransportTable;
        m_pTransportTable = 0;
    }
}

NS_IMPL_ADDREF(nsHTTPHandler);

NS_METHOD
nsHTTPHandler::NewChannel(const char* verb, nsIURI* i_URL,
                          nsIEventSinkGetter *eventSinkGetter,
                          nsIEventQueue *i_eventQueue,
                          nsIChannel **o_Instance)
{
    nsresult rv;
    nsHTTPChannel* pChannel = nsnull;
    char* scheme = 0;

    // Initial checks...
    if (!i_URL || !o_Instance) {
        return NS_ERROR_NULL_POINTER;
    }

    i_URL->GetScheme(&scheme);
    if (0 == PL_strcasecmp(scheme, "http")) {
        nsCOMPtr<nsIURI> channelURI;
        PRUint32 count;
        PRInt32 index;

        //Check to see if an instance already exists in the active list
        m_pConnections->Count(&count);
        for (index=count-1; index >= 0; --index) {
            //switch to static_cast...
            pChannel = (nsHTTPChannel*)((nsIHTTPChannel*) m_pConnections->ElementAt(index));
            //Do other checks here as well... TODO
            rv = pChannel->GetURI(getter_AddRefs(channelURI));
            if (NS_SUCCEEDED(rv) && (channelURI == i_URL))
            {
                NS_ADDREF(pChannel);
                *o_Instance = pChannel;
                return NS_OK; // TODO return NS_USING_EXISTING... or NS_DUPLICATE_REQUEST something like that.
            }
        }

        // Verify that the event sink is http
        nsCOMPtr<nsIHTTPEventSink>  httpEventSink;

        if (eventSinkGetter) {
            rv = eventSinkGetter->GetEventSink(verb, nsIHTTPEventSink::GetIID(),
                                              (nsISupports**)(nsIHTTPEventSink**)getter_AddRefs(httpEventSink));
            if (NS_FAILED(rv)) return rv;
        }
        // Create one
        pChannel = new nsHTTPChannel(i_URL, 
                                     httpEventSink,
                                     this);
        if (pChannel) {
            NS_ADDREF(pChannel);
            pChannel->Init();
            rv = pChannel->QueryInterface(nsIChannel::GetIID(), (void**)o_Instance);
            // add this instance to the active list of connections
            // TODO!
            NS_RELEASE(pChannel);
        } else {
            rv = NS_ERROR_OUT_OF_MEMORY;
        }
        return rv;
    }

    NS_ERROR("Non-HTTP request coming to HTTP Handler!!!");
    //return NS_ERROR_MISMATCHED_URL;
    return NS_ERROR_FAILURE;
}

nsresult
nsHTTPHandler::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
    if (NULL == aInstancePtr)
        return NS_ERROR_NULL_POINTER;

    *aInstancePtr = NULL;
    
    static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);

    if (aIID.Equals(nsIProtocolHandler::GetIID())) {
        *aInstancePtr = (void*) ((nsIProtocolHandler*)this);
        NS_ADDREF_THIS();
        return NS_OK;
    }
    if (aIID.Equals(nsIHTTPHandler::GetIID())) {
        *aInstancePtr = (void*) ((nsIHTTPHandler*)this);
        NS_ADDREF_THIS();
        return NS_OK;
    }
    if (aIID.Equals(nsIProxy::GetIID())) {
        *aInstancePtr = (void*) ((nsIProxy*)this);
        NS_ADDREF_THIS();
        return NS_OK;
    }
    if (aIID.Equals(kISupportsIID)) {
        *aInstancePtr = (void*) ((nsISupports*)(nsIProtocolHandler*)this);
        NS_ADDREF_THIS();
        return NS_OK;
    }
    return NS_NOINTERFACE;
}
 
NS_IMPL_RELEASE(nsHTTPHandler);

NS_METHOD
nsHTTPHandler::MakeAbsolute(const char *aRelativeSpec, nsIURI *aBaseURI,
                            char **_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_METHOD
nsHTTPHandler::NewURI(const char *aSpec, nsIURI *aBaseURI,
                      nsIURI **result)
{
    //todo clean this up...
    nsresult rv;

    nsIURI* url;
    if (aBaseURI)
        rv = aBaseURI->Clone(&url);
    else
        rv = nsComponentManager::CreateInstance(kStandardUrlCID, nsnull, nsIURI::GetIID(), (void**)&url);
    if (NS_FAILED(rv)) return rv;

    rv = url->SetSpec((char*)aSpec);

    nsIURI* realUrl = nsnull;
    
    rv = url->QueryInterface(nsIURI::GetIID(), (void**)&realUrl);
    if (NS_FAILED(rv)) return rv;

    *result= realUrl;
    NS_ADDREF(*result);

    return rv;
}

NS_METHOD
nsHTTPHandler::GetTransport(const char* i_Host, 
                            PRUint32& i_Port, 
                            nsIChannel** o_pTrans)
{
#if 0
    // Check in the table...
    nsSocketKey key(i_Host, i_Port);
    nsIChannel* trans = (nsIChannel*) m_pTransportTable->Get(&key);
    if (trans)
    {
        *o_pTrans = trans;
        return NS_OK;
    }
#endif /* 0 */

    // Create a new one...
    nsresult rv;
    nsIChannel* trans;

    NS_WITH_SERVICE(nsISocketTransportService, sts, kSocketTransportServiceCID, &rv);
    if (NS_FAILED(rv)) return rv;

    rv = sts->CreateTransport(i_Host, i_Port, &trans);
    if (NS_FAILED(rv)) return rv;

#if 0
    // Put it in the table...
    void* oldValue = m_pTransportTable->Put(&key, trans);
    NS_ASSERTION(oldValue == nsnull, "Race condition in transport table!");
    NS_ADDREF(trans);
#endif /* 0 */

    *o_pTrans = trans;

    return rv;
}

NS_METHOD
nsHTTPHandler::ReleaseTransport(const char* i_Host, 
                                PRUint32& i_Port, 
                                nsIChannel* i_pTrans)
{
#if 0
    nsSocketKey key(i_Host, i_Port);
    nsIChannel* value = (nsIChannel*) m_pTransportTable->Remove(&key);
    if (value == nsnull)
        return NS_ERROR_FAILURE;
    NS_ASSERTION(i_pTrans == value, "m_pTransportTable is out of sync");
#endif /* 0 */

    return NS_OK;
}

NS_METHOD
nsHTTPHandler::FollowRedirects(PRBool bFollow)
{
    //m_bFollowRedirects = bFollow;
    return NS_OK;
}
