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

#include "nsString2.h"

#include "nsIImapIncomingServer.h"
#include "nsIMAPHostSessionList.h"
#include "nsMsgIncomingServer.h"
#include "nsImapIncomingServer.h"
#include "nsIImapUrl.h"
#include "nsIUrlListener.h"
#include "nsIEventQueue.h"
#include "nsIImapProtocol.h"
#include "nsISupportsArray.h"
#include "nsVoidArray.h"
#include "nsCOMPtr.h"

#include "nsIPref.h"

#include "prmem.h"
#include "plstr.h"

static NS_DEFINE_CID(kCImapHostSessionList, NS_IIMAPHOSTSESSIONLIST_CID);
static NS_DEFINE_CID(kImapProtocolCID, NS_IMAPPROTOCOL_CID);

/* get some implementation from nsMsgIncomingServer */
class nsImapIncomingServer : public nsMsgIncomingServer,
                             public nsIImapIncomingServer
                             
{
public:
    NS_DECL_ISUPPORTS_INHERITED

    nsImapIncomingServer();
    virtual ~nsImapIncomingServer();

    // overriding nsMsgIncomingServer methods
	NS_IMETHOD SetKey(char * aKey);  // override nsMsgIncomingServer's implementation...
	NS_IMETHOD GetServerURI(char * *aServerURI);

	// we support the nsIImapIncomingServer interface
    NS_IMETHOD GetMaximumConnectionsNumber(PRInt32* maxConnections);
    NS_IMETHOD SetMaximumConnectionsNumber(PRInt32 maxConnections);

    NS_IMETHOD GetTimeOutLimits(PRInt32* minutes);
    NS_IMETHOD SetTimeOutLimits(PRInt32 minutes);
    
    NS_IMETHOD GetImapConnectionAndLoadUrl(nsIEventQueue* aClientEventQueue,
                                           nsIImapUrl* aImapUrl,
                                           nsIUrlListener* aUrlListener = 0,
                                           nsISupports* aConsumer = 0,
                                           nsIURL** aURL = 0);
    NS_IMETHOD LoadNextQueuedUrl();
    NS_IMETHOD RemoveConnection(nsIImapProtocol* aImapConnection);

private:
    nsresult CreateImapConnection (nsIEventQueue* aEventQueue,
                                   nsIImapUrl* aImapUrl,
                                   nsIImapProtocol** aImapConnection);
    PRBool ConnectionTimeOut(nsIImapProtocol* aImapConnection);
	char *m_rootFolderPath;
    nsCOMPtr<nsISupportsArray> m_connectionCache;
    nsCOMPtr<nsISupportsArray> m_urlQueue;
    nsVoidArray m_urlConsumers;
};

NS_IMPL_ISUPPORTS_INHERITED(nsImapIncomingServer,
                            nsMsgIncomingServer,
                            nsIImapIncomingServer);

                            
nsImapIncomingServer::nsImapIncomingServer() : m_rootFolderPath(nsnull)
{    
    NS_INIT_REFCNT();
    nsresult rv;
	rv = NS_NewISupportsArray(getter_AddRefs(m_connectionCache));
    rv = NS_NewISupportsArray(getter_AddRefs(m_urlQueue));
}

nsImapIncomingServer::~nsImapIncomingServer()
{
	PR_FREEIF(m_rootFolderPath);
}

NS_IMETHODIMP nsImapIncomingServer::SetKey(char * aKey)  // override nsMsgIncomingServer's implementation...
{
	nsMsgIncomingServer::SetKey(aKey);

	// okay now that the key has been set, we need to add ourselves to the
	// host session list...

	// every time we create an imap incoming server, we need to add it to the
	// host session list!! 

	// get the user and host name and the fields to the host session list.
	char * userName = nsnull;
	char * hostName = nsnull;
	
	nsresult rv = GetHostName(&hostName);
	rv = GetUsername(&userName);

	NS_WITH_SERVICE(nsIImapHostSessionList, hostSession, kCImapHostSessionList, &rv);
    if (NS_FAILED(rv)) return rv;

	hostSession->AddHostToList(hostName, userName);
	PR_FREEIF(userName);
	PR_FREEIF(hostName);

	return rv;
}

NS_IMETHODIMP nsImapIncomingServer::GetServerURI(char ** aServerURI)
{
	nsresult rv = NS_OK;
	nsString2 serverUri("imap://", eOneByte);
	char * hostName = nsnull;
	rv = GetHostName(&hostName);
	if (NS_FAILED(rv))
		return rv;

	serverUri += hostName;
	if (aServerURI)
		*aServerURI = PL_strdup(serverUri.GetBuffer());


	PR_FREEIF(hostName);

	return rv;
}

NS_IMPL_SERVERPREF_INT(nsImapIncomingServer, MaximumConnectionsNumber,
                       "max_cached_connections");

NS_IMPL_SERVERPREF_INT(nsImapIncomingServer, TimeOutLimits,
                       "timeout");

NS_IMETHODIMP
nsImapIncomingServer::GetImapConnectionAndLoadUrl(nsIEventQueue*
                                                  aClientEventQueue,
                                                  nsIImapUrl* aImapUrl,
                                                  nsIUrlListener*
                                                  aUrlListener,
                                                  nsISupports* aConsumer,
                                                  nsIURL** aURL)
{
    nsresult rv = NS_OK;
    nsIImapProtocol* aProtocol = nsnull;
    
    rv = CreateImapConnection(aClientEventQueue, aImapUrl, &aProtocol);
    if (NS_FAILED(rv)) return rv;

    if (aUrlListener)
        aImapUrl->RegisterListener(aUrlListener);

    if (aProtocol)
    {
        rv = aProtocol->LoadUrl(aImapUrl, aConsumer);
        // *** jt - in case of the time out situation or the connection gets
        // terminated by some unforseen problems let's give it a second chance
        // to run the url
        if (NS_FAILED(rv))
        {
            rv = aProtocol->LoadUrl(aImapUrl, aConsumer);
        }
        else
        {
            // *** jt - alert user that error has occurred
        }   
    }
    else
    {   // unable to get an imap connection to run the url; add to the url
        // queue
        PR_CEnterMonitor(this);
        m_urlQueue->AppendElement(aImapUrl);
        m_urlConsumers.AppendElement((void*)aConsumer);
        NS_IF_ADDREF(aConsumer);
        PR_CExitMonitor(this);
    }
    if (aURL)
    {
        *aURL = aImapUrl;
        NS_IF_RELEASE(*aURL);
    }

    return rv;
}

NS_IMETHODIMP
nsImapIncomingServer::LoadNextQueuedUrl()
{
    PRUint32 cnt = 0;
    nsresult rv = NS_OK;

    PR_CEnterMonitor(this);
    m_urlQueue->Count(&cnt);
    if (cnt > 0)
    {
        nsCOMPtr<nsISupports>
            aSupport(getter_AddRefs(m_urlQueue->ElementAt(0)));
        nsCOMPtr<nsIImapUrl>
            aImapUrl(do_QueryInterface(aSupport, &rv));

        if (aImapUrl)
        {
            nsISupports *aConsumer =
                (nsISupports*)m_urlConsumers.ElementAt(0);

            NS_IF_ADDREF(aConsumer);
            
            nsIImapProtocol * protocolInstance = nsnull;
            rv = CreateImapConnection(nsnull, aImapUrl,
                                               &protocolInstance);
            if (NS_SUCCEEDED(rv) && protocolInstance)
            {
                rv = protocolInstance->LoadUrl(aImapUrl, aConsumer);
                m_urlQueue->RemoveElementAt(0);
                m_urlConsumers.RemoveElementAt(0);
            }

            NS_IF_RELEASE(aConsumer);
        }
    }
    PR_CExitMonitor(this);
    return rv;
}


NS_IMETHODIMP
nsImapIncomingServer::RemoveConnection(nsIImapProtocol* aImapConnection)
{
    PR_CEnterMonitor(this);

    if (aImapConnection)
        m_connectionCache->RemoveElement(aImapConnection);

    PR_CExitMonitor(this);
    return NS_OK;
}

PRBool
nsImapIncomingServer::ConnectionTimeOut(nsIImapProtocol* aConnection)
{
    PRBool retVal = PR_FALSE;
    if (!aConnection) return retVal;
    nsresult rv;

    PR_CEnterMonitor(this);
    PRInt32 timeoutInMinutes = 0;
    rv = GetTimeOutLimits(&timeoutInMinutes);
    if (NS_FAILED(rv) || timeoutInMinutes <= 0 || timeoutInMinutes > 29)
    {
        timeoutInMinutes = 29;
        SetTimeOutLimits(timeoutInMinutes);
    }

    PRTime cacheTimeoutLimits;

    LL_I2L(cacheTimeoutLimits, timeoutInMinutes * 60 * 1000000); // in
                                                              // microseconds
    PRTime lastActiveTimeStamp;
    rv = aConnection->GetLastActiveTimeStamp(&lastActiveTimeStamp);

    PRTime elapsedTime;
    LL_SUB(elapsedTime, PR_Now(), lastActiveTimeStamp);
    PRTime t;
    LL_SUB(t, elapsedTime, cacheTimeoutLimits);
    if (LL_GE_ZERO(t))
    {
        nsCOMPtr<nsIImapProtocol> aProtocol(do_QueryInterface(aConnection,
                                                              &rv));
        if (NS_SUCCEEDED(rv) && aProtocol)
        {
            m_connectionCache->RemoveElement(aConnection);
            aProtocol->TellThreadToDie(PR_TRUE);
            retVal = PR_TRUE;
        }
    }
    PR_CExitMonitor(this);
    return retVal;
}

nsresult
nsImapIncomingServer::CreateImapConnection(nsIEventQueue *aEventQueue, 
                                           nsIImapUrl * aImapUrl, 
                                           nsIImapProtocol ** aImapConnection)
{
	nsresult rv = NS_OK;
	PRBool canRunUrl = PR_FALSE;
    PRBool hasToWait = PR_FALSE;
	nsCOMPtr<nsIImapProtocol> connection;
    nsCOMPtr<nsIImapProtocol> freeConnection;
    PRBool isBusy = PR_FALSE;
    PRBool isInboxConnection = PR_FALSE;

    PR_CEnterMonitor(this);

    PRInt32 maxConnections = 5; // default to be five
    rv = GetMaximumConnectionsNumber(&maxConnections);
    if (NS_FAILED(rv) || maxConnections == 0)
    {
        maxConnections = 5;
        rv = SetMaximumConnectionsNumber(maxConnections);
    }
    else if (maxConnections < 2)
    {   // forced to use at least 2
        maxConnections = 2;
        rv = SetMaximumConnectionsNumber(maxConnections);
    }

    *aImapConnection = nsnull;
	// iterate through the connection cache for a connection that can handle this url.
	PRUint32 cnt;
    nsCOMPtr<nsISupports> aSupport;

    rv = m_connectionCache->Count(&cnt);
    if (NS_FAILED(rv)) return rv;
    for (PRUint32 i = 0; i < cnt && !canRunUrl && !hasToWait; i++) 
	{
        aSupport = getter_AddRefs(m_connectionCache->ElementAt(i));
        connection = do_QueryInterface(aSupport);
		if (connection)
			rv = connection->CanHandleUrl(aImapUrl, canRunUrl, hasToWait);
        if (NS_FAILED(rv)) 
        {
            connection = null_nsCOMPtr();
            continue;
        }
        if (!freeConnection && !canRunUrl && !hasToWait && connection)
        {
            rv = connection->IsBusy(isBusy, isInboxConnection);
            if (NS_FAILED(rv)) continue;
            if (!isBusy && !isInboxConnection)
                freeConnection = connection;
        }
	}
    
    if (ConnectionTimeOut(connection))
        connection = null_nsCOMPtr();
    if (ConnectionTimeOut(freeConnection))
        freeConnection = null_nsCOMPtr();

	// if we got here and we have a connection, then we should return it!
	if (canRunUrl && connection)
	{
		*aImapConnection = connection;
		NS_IF_ADDREF(*aImapConnection);
	}
    else if (hasToWait)
    {
        // do nothing; return NS_OK; for queuing
    }
	else if (cnt < maxConnections && aEventQueue)
	{	
		// create a new connection and add it to the connection cache
		// we may need to flag the protocol connection as busy so we don't get
        // a race 
		// condition where someone else goes through this code 
		nsIImapProtocol * protocolInstance = nsnull;
		rv = nsComponentManager::CreateInstance(kImapProtocolCID, nsnull,
                                                nsIImapProtocol::GetIID(),
                                                (void **) &protocolInstance);
		if (NS_SUCCEEDED(rv) && protocolInstance)
        {
            NS_WITH_SERVICE(nsIImapHostSessionList, hostSession,
                            kCImapHostSessionList, &rv);
            if (NS_SUCCEEDED(rv))
                rv = protocolInstance->Initialize(hostSession, aEventQueue);
        }
		
		// take the protocol instance and add it to the connectionCache
		if (protocolInstance)
			m_connectionCache->AppendElement(protocolInstance);
		*aImapConnection = protocolInstance; // this is already ref counted.

	}
    else if (freeConnection)
    {
        *aImapConnection = freeConnection;
        NS_IF_ADDREF(*aImapConnection);
    }
    else // cannot get anyone to handle the url queue it
    {
        // queue the url
    }

    PR_CExitMonitor(this);
	return rv;
}

nsresult NS_NewImapIncomingServer(const nsIID& iid,
                                  void **result)
{
    nsImapIncomingServer *server;
    if (!result) return NS_ERROR_NULL_POINTER;
    server = new nsImapIncomingServer();
    return server->QueryInterface(iid, result);
}


