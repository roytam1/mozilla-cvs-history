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

// ftp implementation

#include "nsFtpProtocolConnection.h"
#include "nscore.h"
#include "nsIServiceManager.h"
#include "nsIByteBufferInputStream.h"
#include "nsFtpConnectionThread.h"

#include "prprf.h" // PR_sscanf

static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);

// There are actually two transport connections established for an 
// ftp connection. One is used for the command channel , and
// the other for the data channel. The command channel is the first
// connection made and is used to negotiate the second, data, channel.
// The data channel is driven by the command channel and is either
// initiated by the server (PORT command) or by the client (PASV command).
// Client initiation is the most command case and is attempted first.

nsFtpProtocolConnection::nsFtpProtocolConnection()
    : mUrl(nsnull), mConnected(PR_FALSE), mListener(nsnull) {

    mEventQueue = PL_CreateEventQueue("FTP Event Queue", PR_CurrentThread());
    NS_INIT_REFCNT();
}

nsFtpProtocolConnection::~nsFtpProtocolConnection() {
    NS_IF_RELEASE(mUrl);
    NS_IF_RELEASE(mListener);
}

NS_IMPL_ADDREF(nsFtpProtocolConnection);
NS_IMPL_RELEASE(nsFtpProtocolConnection);

NS_IMETHODIMP
nsFtpProtocolConnection::QueryInterface(const nsIID& aIID, void** aInstancePtr) {
    NS_ASSERTION(aInstancePtr, "no instance pointer");
    if (aIID.Equals(nsIFtpProtocolConnection::GetIID()) ||
        aIID.Equals(nsIProtocolConnection::GetIID()) ||
        aIID.Equals(kISupportsIID) ) {
        *aInstancePtr = NS_STATIC_CAST(nsIFtpProtocolConnection*, this);
        NS_ADDREF_THIS();
        return NS_OK;
    }
    if (aIID.Equals(nsIStreamListener::GetIID()) ||
        aIID.Equals(nsIStreamObserver::GetIID())) {
        *aInstancePtr = NS_STATIC_CAST(nsIStreamListener*, this);
        NS_ADDREF_THIS();
        return NS_OK;
    }
    return NS_NOINTERFACE; 
}

nsresult 
nsFtpProtocolConnection::Init(nsIUrl* aUrl, nsISupports* aEventSink, PLEventQueue* aEventQueue) {
 
    if (mConnected)
        return NS_ERROR_NOT_IMPLEMENTED;

    mUrl = aUrl;
    NS_ADDREF(mUrl);

    mEventQueue = aEventQueue;

    return NS_OK;
}

////////////////////////////////////////////////////////////////////////////////
// nsICancelable methods:

NS_IMETHODIMP
nsFtpProtocolConnection::Cancel(void) {
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsFtpProtocolConnection::Suspend(void) {
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsFtpProtocolConnection::Resume(void) {
    return NS_ERROR_NOT_IMPLEMENTED;
}

////////////////////////////////////////////////////////////////////////////////
// nsIProtocolConnection methods:


// establishes the connection and initiates the file transfer.
NS_IMETHODIMP
nsFtpProtocolConnection::Open(void) {
    nsresult rv;
    nsIThread* workerThread = nsnull;
    nsFtpConnectionThread* protocolInterpreter = 
        new nsFtpConnectionThread(mEventQueue, this);
    NS_ASSERTION(protocolInterpreter, "ftp protocol interpreter alloc failed");
    NS_ADDREF(protocolInterpreter);

    if (!protocolInterpreter)
        return NS_ERROR_OUT_OF_MEMORY;

    protocolInterpreter->Init(workerThread, mUrl);
    protocolInterpreter->SetUsePasv(PR_TRUE);

    rv = NS_NewThread(&workerThread, protocolInterpreter);
    NS_ASSERTION(NS_SUCCEEDED(rv), "new thread failed");
    
    // XXX this release should probably be in the destructor.
    NS_RELEASE(protocolInterpreter);
    return NS_OK;
}

NS_IMETHODIMP
nsFtpProtocolConnection::GetContentType(char* *contentType) {
    
    // XXX for ftp we need to do a file extension-to-type mapping lookup
    // XXX in some hash table/registry of mime-types
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsFtpProtocolConnection::GetInputStream(nsIInputStream* *result) {
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsFtpProtocolConnection::GetOutputStream(nsIOutputStream* *result) {
    return NS_ERROR_NOT_IMPLEMENTED;
}


////////////////////////////////////////////////////////////////////////////////
// nsIFtpProtocolConnection methods:

NS_IMETHODIMP
nsFtpProtocolConnection::Get(void) {
    return Open();
}

NS_IMETHODIMP
nsFtpProtocolConnection::Put(void) {
    nsresult rv;
    nsIThread* workerThread = nsnull;
    nsFtpConnectionThread* protocolInterpreter = 
        new nsFtpConnectionThread(mEventQueue, this);
    NS_ASSERTION(protocolInterpreter, "ftp protocol interpreter alloc failed");
    NS_ADDREF(protocolInterpreter);

    if (!protocolInterpreter)
        return NS_ERROR_OUT_OF_MEMORY;

    protocolInterpreter->Init(workerThread, mUrl);
    protocolInterpreter->SetAction(PUT);
    protocolInterpreter->SetUsePasv(PR_TRUE);

    rv = NS_NewThread(&workerThread, protocolInterpreter);
    NS_ASSERTION(NS_SUCCEEDED(rv), "new thread failed");
    
    // XXX this release should probably be in the destructor.
    NS_RELEASE(protocolInterpreter);
    return NS_OK;
}

////////////////////////////////////////////////////////////////////////////////
// nsIStreamObserver methods:

NS_IMETHODIMP
nsFtpProtocolConnection::OnStartBinding(nsISupports* context) {
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsFtpProtocolConnection::OnStopBinding(nsISupports* context,
                                        nsresult aStatus,
                                        nsIString* aMsg) {
    // Release the lock so the user get's the data stream
    return NS_ERROR_NOT_IMPLEMENTED;
}

////////////////////////////////////////////////////////////////////////////////
// nsIStreamListener methods:

NS_IMETHODIMP
nsFtpProtocolConnection::OnDataAvailable(nsISupports* context,
                                         nsIInputStream *aIStream, 
                                         PRUint32 aSourceOffset,
                                         PRUint32 aLength) {
    // Fill in the buffer w/ the new data.
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsFtpProtocolConnection::SetStreamListener(nsIStreamListener *aListener) {
    mListener = aListener;
    NS_ADDREF(mListener);
    return NS_OK;
}


////////////////////////////////////////////////////////////////////////////////
