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

#include "nsHttpProtocolConnection.h"
#include "nscore.h"
#include "nsIUrl.h"
#include "nsIHttpEventSink.h"
#include "nsIComponentManager.h"
#include "nsIServiceManager.h"
//#include "nsISocketTransportService.h"
#include "nsIFileTransportService.h"    // XXX temporary

//static NS_DEFINE_CID(kSocketTransportServiceCID, NS_SOCKETTRANSPORTSERVICE_CID);
static NS_DEFINE_CID(kFileTransportServiceCID, NS_FILETRANSPORTSERVICE_CID);        // XXX temporary

////////////////////////////////////////////////////////////////////////////////

nsHttpProtocolConnection::nsHttpProtocolConnection()
    : mUrl(nsnull), mEventSink(nsnull), mConnected(PR_FALSE)
{
}

nsHttpProtocolConnection::~nsHttpProtocolConnection()
{
    NS_IF_RELEASE(mUrl);
    NS_IF_RELEASE(mEventSink);
}

NS_IMPL_ADDREF(nsHttpProtocolConnection);
NS_IMPL_RELEASE(nsHttpProtocolConnection);

NS_IMETHODIMP
nsHttpProtocolConnection::QueryInterface(const nsIID& aIID, void** aInstancePtr)
{
    NS_ASSERTION(aInstancePtr, "no instance pointer");
    if (aIID.Equals(nsIHttpProtocolConnection::GetIID()) ||
        aIID.Equals(nsIProtocolConnection::GetIID()) ||
        aIID.Equals(nsISupports::GetIID())) {
        *aInstancePtr = NS_STATIC_CAST(nsIHttpProtocolConnection*, this);
        NS_ADDREF_THIS();
        return NS_OK;
    }
    return NS_NOINTERFACE; 
}

nsresult 
nsHttpProtocolConnection::Init(nsIUrl* url, nsISupports* eventSink)
{
    nsresult rv;

    mUrl = url;
    NS_ADDREF(mUrl);

    rv = eventSink->QueryInterface(nsIHttpEventSink::GetIID(), (void**)&mEventSink);

    return rv;
}

////////////////////////////////////////////////////////////////////////////////
// nsICancelable methods:

NS_IMETHODIMP
nsHttpProtocolConnection::Cancel(void)
{
    if (!mConnected)
        return NS_ERROR_NOT_CONNECTED;
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsHttpProtocolConnection::Suspend(void)
{
    if (!mConnected)
        return NS_ERROR_NOT_CONNECTED;
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsHttpProtocolConnection::Resume(void)
{
    if (!mConnected)
        return NS_ERROR_NOT_CONNECTED;
    return NS_ERROR_NOT_IMPLEMENTED;
}

////////////////////////////////////////////////////////////////////////////////
// nsIProtocolConnection methods:

NS_IMETHODIMP
nsHttpProtocolConnection::GetContentType(char* *contentType)
{
    if (!mConnected)
        return NS_ERROR_NOT_CONNECTED;
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsHttpProtocolConnection::GetInputStream(nsIInputStream* *result)
{
    if (!mConnected)
        return NS_ERROR_NOT_CONNECTED;
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsHttpProtocolConnection::GetOutputStream(nsIOutputStream* *result)
{
    if (!mConnected)
        return NS_ERROR_NOT_CONNECTED;
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsHttpProtocolConnection::AsyncWrite(nsIInputStream* data, PRUint32 count,
                                     nsresult (*callback)(void* closure, PRUint32 count),
                                     void* closure)
{
    if (!mConnected)
        return NS_ERROR_NOT_CONNECTED;
    return NS_ERROR_NOT_IMPLEMENTED;
}

////////////////////////////////////////////////////////////////////////////////
// nsIHttpProtocolConnection methods:

NS_IMETHODIMP
nsHttpProtocolConnection::Get(void)
{
    nsresult rv;
//    NS_WITH_SERVICE(nsISocketTransportService, sts, kSocketTransportServiceCID, &rv);

    NS_WITH_SERVICE(nsIFileTransportService, sts, kFileTransportServiceCID, &rv);

//    rv = sts->AsyncWrite();

    return rv;
}

NS_IMETHODIMP
nsHttpProtocolConnection::GetByteRange(PRUint32 from, PRUint32 to)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsHttpProtocolConnection::Put(void)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsHttpProtocolConnection::Post(void)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

////////////////////////////////////////////////////////////////////////////////
