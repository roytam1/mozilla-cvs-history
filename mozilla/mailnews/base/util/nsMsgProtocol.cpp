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

#include "msgCore.h"
#include "nsMsgProtocol.h"
#include "nsIMsgMailNewsUrl.h"
#include "nsISocketTransportService.h"
#include "nsXPIDLString.h"
#include "nsSpecialSystemDirectory.h"
#include "nsILoadGroup.h"
#include "nsIIOService.h"

static NS_DEFINE_CID(kSocketTransportServiceCID, NS_SOCKETTRANSPORTSERVICE_CID);
static NS_DEFINE_CID(kIOServiceCID, NS_IOSERVICE_CID);

NS_IMPL_ISUPPORTS3(nsMsgProtocol, nsIStreamListener, nsIStreamObserver, nsIChannel)

nsMsgProtocol::nsMsgProtocol(nsIURI * aURL, nsIURI* originalURI)
{
	NS_INIT_REFCNT();
	m_flags = 0;
	m_startPosition = 0;
	m_readCount = 0;
	m_socketIsOpen = PR_FALSE;
		
	m_tempMsgFileSpec = nsSpecialSystemDirectory(nsSpecialSystemDirectory::OS_TemporaryDirectory);
	m_tempMsgFileSpec += "tempMessage.eml";
    m_url = aURL;
    m_originalUrl = originalURI ? originalURI : aURL;
}

nsMsgProtocol::~nsMsgProtocol()
{}

nsresult nsMsgProtocol::OpenNetworkSocket(nsIURI * aURL) // open a connection on this url
{
	nsresult rv = NS_OK;
	nsXPIDLCString hostName;
	PRInt32 port = 0;

	m_readCount = -1; // with socket connections we want to read as much data as arrives
	m_startPosition = 0;

    NS_WITH_SERVICE(nsISocketTransportService, socketService, kSocketTransportServiceCID, &rv);

	if (NS_SUCCEEDED(rv) && aURL)
	{
		aURL->GetPort(&port);
		aURL->GetHost(getter_Copies(hostName));

		rv = socketService->CreateTransport(hostName, port, nsnull, nsnull, getter_AddRefs(m_channel));
		if (NS_SUCCEEDED(rv) && m_channel)
		{
			m_socketIsOpen = PR_FALSE;
			rv = SetupTransportState();
		}
	}

	return rv;
}

nsresult nsMsgProtocol::OpenFileSocket(nsIURI * aURL, const nsFileSpec * aFileSpec, PRUint32 aStartPosition, PRInt32 aReadCount)
{
	// mscott - file needs to be encoded directly into aURL. I should be able to get
	// rid of this method completely.

	nsresult rv = NS_OK;
	m_startPosition = aStartPosition;
	m_readCount = aReadCount;

    NS_WITH_SERVICE(nsIIOService, netService, kIOServiceCID, &rv);
	if (NS_SUCCEEDED(rv) && aURL)
	{
		// extract the file path from the uri...
		nsXPIDLCString filePath;
		aURL->GetPath(getter_Copies(filePath));
		char * urlSpec = PR_smprintf("file://%s", (const char *) filePath);

		rv = netService->NewChannel("Load", urlSpec, 
                                    nsnull,     // null base URI
                                    nsnull,     // null load group
                                    nsnull,     // null eventsink getter
                                    nsnull,     // originalURI
                                    getter_AddRefs(m_channel));
		PR_FREEIF(urlSpec);

		if (NS_SUCCEEDED(rv) && m_channel)
		{
			m_socketIsOpen = PR_FALSE;
			// rv = SetupTransportState();
		}
	}

	return rv;
}

nsresult nsMsgProtocol::SetupTransportState()
{
	nsresult rv = NS_OK;

	if (!m_socketIsOpen && m_channel)
	{
		rv = m_channel->OpenOutputStream(0 /* start position */, getter_AddRefs(m_outputStream));

		NS_ASSERTION(NS_SUCCEEDED(rv), "unable to create an output stream");
		// we want to open the stream 
	} // if m_transport

	return rv;
}

nsresult nsMsgProtocol::CloseSocket()
{
	// release all of our socket state
	m_socketIsOpen = PR_FALSE;
	m_outputStream = null_nsCOMPtr();
	m_channel = null_nsCOMPtr();

	return NS_OK;
}

/*
 * Writes the data contained in dataBuffer into the current output stream. It also informs
 * the transport layer that this data is now available for transmission.
 * Returns a positive number for success, 0 for failure (not all the bytes were written to the
 * stream, etc). We need to make another pass through this file to install an error system (mscott)
 */

PRInt32 nsMsgProtocol::SendData(nsIURI * aURL, const char * dataBuffer)
{
	PRUint32 writeCount = 0; 
	PRInt32 status = 0; 

//	NS_PRECONDITION(m_outputStream, "oops....we don't have an output stream...how did that happen?");
	if (dataBuffer && m_outputStream)
	{
		status = m_outputStream->Write(dataBuffer, PL_strlen(dataBuffer), &writeCount);
	}

	return status;
}

// Whenever data arrives from the connection, core netlib notifices the protocol by calling
// OnDataAvailable. We then read and process the incoming data from the input stream. 
NS_IMETHODIMP nsMsgProtocol::OnDataAvailable(nsIChannel * /* aChannel */, nsISupports *ctxt, nsIInputStream *inStr, PRUint32 sourceOffset, PRUint32 count)
{
	// right now, this really just means turn around and churn through the state machine
	nsCOMPtr<nsIURI> uri = do_QueryInterface(ctxt);
	return ProcessProtocolState(uri, inStr, sourceOffset, count);
}

NS_IMETHODIMP nsMsgProtocol::OnStartRequest(nsIChannel * aChannel, nsISupports *ctxt)
{
	nsresult rv = NS_OK;
	nsCOMPtr <nsIMsgMailNewsUrl> aMsgUrl = do_QueryInterface(ctxt, &rv);
	if (NS_SUCCEEDED(rv) && aMsgUrl)
		rv = aMsgUrl->SetUrlState(PR_TRUE, NS_OK);

	// if we are set up as a channel, we should notify our channel listener that we are starting...
	// so pass in ourself as the channel and not the underlying socket or file channel the protocol
	// happens to be using
	if (m_channelListener)
		rv = m_channelListener->OnStartRequest(this, m_channelContext);

	return rv;
}

// stop binding is a "notification" informing us that the stream associated with aURL is going away. 
NS_IMETHODIMP nsMsgProtocol::OnStopRequest(nsIChannel * aChannel, nsISupports *ctxt, nsresult aStatus, const PRUnichar* aMsg)
{
	nsresult rv = NS_OK;
	nsCOMPtr <nsIMsgMailNewsUrl> aMsgUrl = do_QueryInterface(ctxt, &rv);
	if (NS_SUCCEEDED(rv) && aMsgUrl)
		rv = aMsgUrl->SetUrlState(PR_FALSE, aStatus);

	// if we are set up as a channel, we should notify our channel listener that we are starting...
	// so pass in ourself as the channel and not the underlying socket or file channel the protocol
	// happens to be using
	if (m_channelListener)
		rv = m_channelListener->OnStopRequest(this, m_channelContext, aStatus, aMsg);

	return rv;
}

nsresult nsMsgProtocol::LoadUrl(nsIURI * aURL, nsISupports * aConsumer)
{
	// okay now kick us off to the next state...
	// our first state is a process state so drive the state machine...
	nsresult rv = NS_OK;
	nsCOMPtr <nsIMsgMailNewsUrl> aMsgUrl = do_QueryInterface(aURL);

	if (NS_SUCCEEDED(rv))
	{
		rv = aMsgUrl->SetUrlState(PR_TRUE, NS_OK); // set the url as a url currently being run...

    // if the url is given a stream consumer then we should use it to forward calls to...
    if (!m_channelListener && aConsumer) // if we don't have a registered listener already
    {
      m_channelListener = do_QueryInterface(aConsumer);
      if (!m_channelContext)
        m_channelContext = do_QueryInterface(aURL);
    }

		if (!m_socketIsOpen)
		{
			nsCOMPtr<nsISupports> urlSupports = do_QueryInterface(aURL);

			// put us in a state where we are always notified of incoming data
			m_channel->AsyncRead(m_startPosition, m_readCount, urlSupports ,this /* stream observer */);
			m_socketIsOpen = PR_TRUE; // mark the channel as open
		} // if we got an event queue service
		else  // the connection is already open so we should begin processing our new url...
			rv = ProcessProtocolState(aURL, nsnull, 0, 0); 
	}

	return rv;
}

///////////////////////////////////////////////////////////////////////
// The rest of this file is mostly nsIChannel mumbo jumbo stuff
///////////////////////////////////////////////////////////////////////

nsresult nsMsgProtocol::SetUrl(nsIURI * aURL)
{
	m_url = dont_QueryInterface(aURL);
	return NS_OK;
}

nsresult nsMsgProtocol::SetLoadGroup(nsILoadGroup * aLoadGroup)
{
	m_loadGroup = dont_QueryInterface(aLoadGroup);
	return NS_OK;
}

NS_IMETHODIMP nsMsgProtocol::GetOriginalURI(nsIURI * *aURI)
{
    *aURI = m_originalUrl;
    NS_IF_ADDREF(*aURI);
    return NS_OK;
}
 
NS_IMETHODIMP nsMsgProtocol::GetURI(nsIURI * *aURI)
{
    *aURI = m_url;
    NS_IF_ADDREF(*aURI);
    return NS_OK;
}
 
NS_IMETHODIMP nsMsgProtocol::OpenInputStream(PRUint32 startPosition, PRInt32 readCount, nsIInputStream **_retval)
{
	return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsMsgProtocol::OpenOutputStream(PRUint32 startPosition, nsIOutputStream **_retval)
{
	return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsMsgProtocol::AsyncOpen(nsIStreamObserver *observer, nsISupports* ctxt)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsMsgProtocol::AsyncRead(PRUint32 startPosition, PRInt32 readCount, nsISupports *ctxt, nsIStreamListener *listener)
{
	// set the stream listener and then load the url
	m_channelContext = ctxt;
	m_channelListener = listener;

	// the following load group code is completely bogus....
	nsresult rv = NS_OK;
	if (m_loadGroup)
	{
		nsCOMPtr<nsILoadGroupListenerFactory> factory;
		//
		// Create a load group "proxy" listener...
		//
		rv = m_loadGroup->GetGroupListenerFactory(getter_AddRefs(factory));
		if (factory) 
		{
			nsCOMPtr<nsIStreamListener> newListener;
			rv = factory->CreateLoadGroupListener(m_channelListener, getter_AddRefs(newListener));
			if (NS_SUCCEEDED(rv)) 
				m_channelListener = newListener;
		}
	} // if aLoadGroup

	return LoadUrl(m_url, nsnull);
}

NS_IMETHODIMP nsMsgProtocol::AsyncWrite(nsIInputStream *fromStream, PRUint32 startPosition, PRInt32 writeCount, nsISupports *ctxt, nsIStreamObserver *observer)
{
	return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsMsgProtocol::GetLoadAttributes(nsLoadFlags *aLoadAttributes)
{
	return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsMsgProtocol::SetLoadAttributes(nsLoadFlags aLoadAttributes)
{
	return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsMsgProtocol::GetContentType(char * *aContentType)
{
  // as url dispatching matures, we'll be intelligent and actually start
  // opening the url before specifying the content type. This will allow
  // us to optimize the case where the message url actual refers to  
  // a part in the message that has a content type that is not message/rfc822

	*aContentType = nsCRT::strdup("message/rfc822");
	return NS_OK;
}

NS_IMETHODIMP nsMsgProtocol::GetContentLength(PRInt32 * aContentLength)
{
  *aContentLength = -1;
  return NS_OK;
}

NS_IMETHODIMP nsMsgProtocol::GetOwner(nsISupports * *aPrincipal)
{
	return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsMsgProtocol::SetOwner(nsISupports * aPrincipal)
{
	return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsMsgProtocol::GetLoadGroup(nsILoadGroup * *aLoadGroup)
{
	return NS_ERROR_NOT_IMPLEMENTED;
}

////////////////////////////////////////////////////////////////////////////////
// From nsIRequest
////////////////////////////////////////////////////////////////////////////////

NS_IMETHODIMP nsMsgProtocol::IsPending(PRBool *result)
{
    *result = PR_TRUE;
    return NS_OK; 
}

NS_IMETHODIMP nsMsgProtocol::Cancel()
{
	return m_channel->Cancel();
}

NS_IMETHODIMP nsMsgProtocol::Suspend()
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsMsgProtocol::Resume()
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
