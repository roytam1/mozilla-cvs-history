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
 *   Pierre Phaneuf <pp@ludusdesign.com>
 */

#include "msgCore.h"
#include "nsMsgImapCID.h"

#include "nsString.h"

#include "nsIMAPHostSessionList.h"
#include "nsImapIncomingServer.h"
#include "nsIImapUrl.h"
#include "nsIUrlListener.h"
#include "nsIEventQueue.h"
#include "nsIImapProtocol.h"
#include "nsISupportsArray.h"
#include "nsVoidArray.h"
#include "nsCOMPtr.h"
#include "nsImapStringBundle.h"
#include "nsIPref.h"
#include "nsMsgFolderFlags.h"
#include "nsTextFormatter.h"
#include "prmem.h"
#include "plstr.h"
#include "nsXPIDLString.h"
#include "nsIMsgFolder.h"
#include "nsIMsgWindow.h"
#include "nsIMsgImapMailFolder.h"
#include "nsImapUtils.h"
#include "nsIRDFService.h"
#include "nsRDFCID.h"
#include "nsINetSupportDialogService.h"
#include "nsEnumeratorUtils.h"
static NS_DEFINE_CID(kCImapHostSessionList, NS_IIMAPHOSTSESSIONLIST_CID);
static NS_DEFINE_CID(kImapProtocolCID, NS_IMAPPROTOCOL_CID);
static NS_DEFINE_CID(kRDFServiceCID, NS_RDFSERVICE_CID);
static NS_DEFINE_CID(kNetSupportDialogCID, NS_NETSUPPORTDIALOG_CID);
static NS_DEFINE_CID(kPrefServiceCID, NS_PREF_CID);

NS_IMPL_ADDREF_INHERITED(nsImapIncomingServer, nsMsgIncomingServer)
NS_IMPL_RELEASE_INHERITED(nsImapIncomingServer, nsMsgIncomingServer)

NS_IMETHODIMP nsImapIncomingServer::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
	if (!aInstancePtr) return NS_ERROR_NULL_POINTER;
	*aInstancePtr = nsnull;

    if (aIID.Equals(NS_GET_IID(nsIImapServerSink)))
	{
		*aInstancePtr = NS_STATIC_CAST(nsIImapServerSink*, this);
	}              
	else if(aIID.Equals(NS_GET_IID(nsIImapIncomingServer)))
	{
		*aInstancePtr = NS_STATIC_CAST(nsIImapIncomingServer*, this);
	}
	if(*aInstancePtr)
	{
		AddRef();
		return NS_OK;
	}

	return nsMsgIncomingServer::QueryInterface(aIID, aInstancePtr);
}

                            
nsImapIncomingServer::nsImapIncomingServer()
{    
    NS_INIT_REFCNT();
    nsresult rv;
	rv = NS_NewISupportsArray(getter_AddRefs(m_connectionCache));
    rv = NS_NewISupportsArray(getter_AddRefs(m_urlQueue));
	m_capability = kCapabilityUndefined;
}

nsImapIncomingServer::~nsImapIncomingServer()
{
    CloseCachedConnections();
}

NS_IMETHODIMP nsImapIncomingServer::SetKey(const char * aKey)  // override nsMsgIncomingServer's implementation...
{
	nsMsgIncomingServer::SetKey(aKey);

	// okay now that the key has been set, we need to add ourselves to the
	// host session list...

	// every time we create an imap incoming server, we need to add it to the
	// host session list!! 

	nsresult rv;
	NS_WITH_SERVICE(nsIImapHostSessionList, hostSession, kCImapHostSessionList, &rv);
    if (NS_FAILED(rv)) return rv;

	hostSession->AddHostToList(aKey);

	char *personalNamespace = nsnull;
	char *publicNamespace = nsnull;
	char *otherUsersNamespace = nsnull;

	rv = GetPersonalNamespace(&personalNamespace);

	if (!personalNamespace && !publicNamespace && !otherUsersNamespace)
		personalNamespace = PL_strdup("\"\"");

	if (NS_SUCCEEDED(rv))
	{
		hostSession->SetNamespaceFromPrefForHost(aKey, personalNamespace, kPersonalNamespace);
		PR_FREEIF(personalNamespace);
	}

	rv = GetPublicNamespace(&publicNamespace);

	if (NS_SUCCEEDED(rv))
	{
		hostSession->SetNamespaceFromPrefForHost(aKey, publicNamespace, kPublicNamespace);
		PR_FREEIF(publicNamespace);
	}

	rv = GetOtherUsersNamespace(&otherUsersNamespace);

	if (NS_SUCCEEDED(rv))
	{
		hostSession->SetNamespaceFromPrefForHost(aKey, otherUsersNamespace, kOtherUsersNamespace);
		PR_FREEIF(otherUsersNamespace);
	}

	return rv;
}

NS_IMETHODIMP nsImapIncomingServer::GetLocalStoreType(char ** type)
{
    NS_ENSURE_ARG_POINTER(type);
    *type = nsCRT::strdup("imap");
    return NS_OK;
}

NS_IMPL_SERVERPREF_STR(nsImapIncomingServer, AdminUrl,
                       "admin_url");

NS_IMPL_SERVERPREF_STR(nsImapIncomingServer, ServerDirectory,
                       "server_sub_directory");

NS_IMPL_SERVERPREF_BOOL(nsImapIncomingServer, UsingSubscription,
                        "using_subscription");

NS_IMPL_SERVERPREF_BOOL(nsImapIncomingServer, CleanupInboxOnExit,
                        "cleanup_inbox_on_exit");
			
NS_IMPL_SERVERPREF_BOOL(nsImapIncomingServer, DualUseFolders,
                        "dual_use_folders");
			
NS_IMPL_SERVERPREF_BOOL(nsImapIncomingServer, EmptyTrashOnExit,
                        "empty_trash_on_exit");

NS_IMPL_SERVERPREF_BOOL(nsImapIncomingServer, OfflineDownload,
                        "offline_download");

NS_IMPL_SERVERPREF_BOOL(nsImapIncomingServer, OverrideNamespaces,
                        "override_namespaces");
			
NS_IMPL_SERVERPREF_INT(nsImapIncomingServer, MaximumConnectionsNumber,
                       "max_cached_connections");

NS_IMPL_SERVERPREF_INT(nsImapIncomingServer, EmptyTrashThreshhold,
                       "empty_trash_threshhold");

NS_IMPL_SERVERPREF_INT(nsImapIncomingServer, DeleteModel,
                       "delete_model");

NS_IMPL_SERVERPREF_INT(nsImapIncomingServer, TimeOutLimits,
                       "timeout");

NS_IMPL_SERVERPREF_INT(nsImapIncomingServer, CapabilityPref,
                       "capability");

NS_IMPL_SERVERPREF_STR(nsImapIncomingServer, PersonalNamespace,
                       "namespace.personal");

NS_IMPL_SERVERPREF_STR(nsImapIncomingServer, PublicNamespace,
                       "namespace.public");

NS_IMPL_SERVERPREF_STR(nsImapIncomingServer, OtherUsersNamespace,
                       "namespace.other_users");

NS_IMPL_SERVERPREF_BOOL(nsImapIncomingServer, FetchByChunks,
                       "fetch_by_chunks");

NS_IMPL_SERVERPREF_BOOL(nsImapIncomingServer, MimePartsOnDemand,
                       "mime_parts_on_demand");

NS_IMETHODIMP
nsImapIncomingServer::GetIsAOLServer(PRBool *aBool)
{
	if (!aBool)
		return NS_ERROR_NULL_POINTER;
	*aBool = ((m_capability & kAOLImapCapability) != 0);
	return NS_OK;
}

NS_IMETHODIMP
nsImapIncomingServer::SetIsAOLServer(PRBool aBool)
{
	if (aBool)
		m_capability |= kAOLImapCapability;
	else
		m_capability &= ~kAOLImapCapability;
	return NS_OK;
}


NS_IMETHODIMP
nsImapIncomingServer::GetImapConnectionAndLoadUrl(nsIEventQueue * aClientEventQueue,
                                                  nsIImapUrl* aImapUrl,
                                                  nsISupports* aConsumer)
{
    nsresult rv = NS_OK;
    nsCOMPtr <nsIImapProtocol> aProtocol;
    
    rv = CreateImapConnection(aClientEventQueue, aImapUrl, getter_AddRefs(aProtocol));
    if (NS_FAILED(rv)) return rv;

	nsCOMPtr<nsIMsgMailNewsUrl> mailnewsurl = do_QueryInterface(aImapUrl, &rv);
    if (aProtocol)
    {
        rv = aProtocol->LoadUrl(mailnewsurl, aConsumer);
        // *** jt - in case of the time out situation or the connection gets
        // terminated by some unforseen problems let's give it a second chance
        // to run the url
        if (NS_FAILED(rv))
        {
            rv = aProtocol->LoadUrl(mailnewsurl, aConsumer);
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
		nsCOMPtr <nsISupports> supports(do_QueryInterface(aImapUrl));
		if (supports)
			m_urlQueue->AppendElement(supports);
        m_urlConsumers.AppendElement((void*)aConsumer);
        NS_IF_ADDREF(aConsumer);
        PR_CExitMonitor(this);
    }

    return rv;
}

// checks to see if there are any queued urls on this incoming server,
// and if so, tries to run the oldest one.
NS_IMETHODIMP
nsImapIncomingServer::LoadNextQueuedUrl(PRBool *aResult)
{
    PRUint32 cnt = 0;
    nsresult rv = NS_OK;
	PRBool urlRun = PR_FALSE;

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
            
            nsCOMPtr <nsIImapProtocol>  protocolInstance ;
            rv = CreateImapConnection(nsnull, aImapUrl,
                                               getter_AddRefs(protocolInstance));
            if (NS_SUCCEEDED(rv) && protocolInstance)
            {
				nsCOMPtr<nsIURI> url = do_QueryInterface(aImapUrl, &rv);
				if (NS_SUCCEEDED(rv) && url)
				{
					rv = protocolInstance->LoadUrl(url, aConsumer);
					urlRun = PR_TRUE;
				}
                m_urlQueue->RemoveElementAt(0);
                m_urlConsumers.RemoveElementAt(0);
            }

            NS_IF_RELEASE(aConsumer);
        }
    }
	if (aResult)
		*aResult = urlRun;

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
            aProtocol->TellThreadToDie(PR_FALSE);
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
			rv = connection->CanHandleUrl(aImapUrl, &canRunUrl, &hasToWait);
        if (NS_FAILED(rv)) 
        {
            connection = null_nsCOMPtr();
            continue;
        }
        if (!freeConnection && !canRunUrl && !hasToWait && connection)
        {
            rv = connection->IsBusy(&isBusy, &isInboxConnection);
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
	else if (cnt < ((PRUint32)maxConnections) && aEventQueue)
	{	
		// create a new connection and add it to the connection cache
		// we may need to flag the protocol connection as busy so we don't get
        // a race 
		// condition where someone else goes through this code 
		nsIImapProtocol * protocolInstance = nsnull;
		rv = nsComponentManager::CreateInstance(kImapProtocolCID, nsnull,
                                                NS_GET_IID(nsIImapProtocol),
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

NS_IMETHODIMP nsImapIncomingServer::ResetConnection(const char* folderName)
{
    nsresult rv = NS_OK;
    nsCOMPtr<nsIImapProtocol> connection;
    nsCOMPtr<nsISupports> aSupport;
    PRBool isBusy = PR_FALSE, isInbox = PR_FALSE;
    PRUint32 cnt = 0;
    nsXPIDLCString curFolderName;

    rv = m_connectionCache->Count(&cnt);
    if (NS_FAILED(rv)) return rv;

    PR_CEnterMonitor(this);
    
    for (PRUint32 i=0; i < cnt; i++)
    {
        aSupport = getter_AddRefs(m_connectionCache->ElementAt(i));
        connection = do_QueryInterface(aSupport);
        if (connection)
        {
            rv = connection->GetSelectedMailboxName(getter_Copies(curFolderName));
            if (PL_strcmp(curFolderName,folderName) == 0)
            {
                rv = connection->IsBusy(&isBusy, &isInbox);
                if (!isBusy)
                    rv = connection->ResetToAuthenticatedState();
                break; // found it, end of the loop
            }
        }
    }

    PR_CExitMonitor(this);
    return rv;
}

NS_IMETHODIMP nsImapIncomingServer::PerformBiff()
{
	nsresult rv;

	nsCOMPtr<nsIFolder> rootFolder;
	rv = GetRootFolder(getter_AddRefs(rootFolder));
	if(NS_SUCCEEDED(rv))
	{
		nsCOMPtr<nsIMsgFolder> rootMsgFolder = do_QueryInterface(rootFolder);
		if(rootMsgFolder)
			rv = rootMsgFolder->GetNewMessages(nsnull);
	}

	return rv;
}
    

NS_IMETHODIMP
nsImapIncomingServer::CloseCachedConnections()
{

	nsCOMPtr<nsIImapProtocol> connection;
    PR_CEnterMonitor(this);

	// iterate through the connection cache closing open connections.
	PRUint32 cnt;
    nsCOMPtr<nsISupports> aSupport;

    nsresult rv = m_connectionCache->Count(&cnt);
    if (NS_FAILED(rv)) return rv;
    for (PRUint32 i = 0; i < cnt; i++) 
	{
        aSupport = getter_AddRefs(m_connectionCache->ElementAt(i));
        connection = do_QueryInterface(aSupport);
		if (connection)
			rv = connection->TellThreadToDie(PR_TRUE);
	}
    
    PR_CExitMonitor(this);
	return rv;
}

// nsIImapServerSink impl
NS_IMETHODIMP nsImapIncomingServer::PossibleImapMailbox(const char *folderPath, PRUnichar hierarchyDelimiter, PRInt32 boxFlags)
{
	nsresult rv;
    PRBool found = PR_FALSE;
	PRBool haveParent = PR_FALSE;
    nsCOMPtr<nsIMsgImapMailFolder> hostFolder;
    nsCOMPtr<nsIMsgFolder> aFolder;
    
    if (!folderPath || !*folderPath) return NS_ERROR_NULL_POINTER;
    nsCAutoString dupFolderPath = folderPath;
    if (dupFolderPath.Last() == '/')
        dupFolderPath.SetLength(dupFolderPath.Length()-1);

    nsCAutoString folderName = dupFolderPath;
        
    nsCAutoString uri;
	nsXPIDLCString serverUri;

	GetServerURI(getter_Copies(serverUri));

    uri = serverUri;

    PRInt32 leafPos = folderName.RFindChar('/');

    nsCAutoString parentName(folderName);
	nsCAutoString parentUri(uri);

    if (leafPos > 0)
	{
		// If there is a hierarchy, there is a parent.
		// Don't strip off slash if it's the first character
        parentName.Truncate(leafPos);
		haveParent = PR_TRUE;
		parentUri.Append('/');
		parentUri.Append(parentName);
		folderName.Cut(0, leafPos + 1);	// get rid of the parent name
	}

	nsCOMPtr<nsIFolder> rootFolder;
    rv = GetRootFolder(getter_AddRefs(rootFolder));

    if(NS_FAILED(rv))
        return rv;
    nsCOMPtr<nsIMsgFolder> a_nsIFolder(do_QueryInterface(rootFolder, &rv));

    if (NS_FAILED(rv))
		return rv;

	hostFolder = do_QueryInterface(a_nsIFolder, &rv);
	if (NS_FAILED(rv))
		return rv;

	nsCOMPtr <nsIMsgFolder> child;

//	nsCString possibleName(aSpec->allocatedPathName);

	uri.Append('/');
	uri.Append(dupFolderPath);

	a_nsIFolder->GetChildWithURI(uri, PR_TRUE, getter_AddRefs(child));

	if (child)
		found = PR_TRUE;
    if (!found)
    {
		// trying to find/discover the parent
		if (haveParent)
		{											
			nsCOMPtr <nsIMsgFolder> parent;
			a_nsIFolder->GetChildWithURI(parentUri, PR_TRUE, getter_AddRefs(parent));
			if (!parent /* || parentFolder->GetFolderNeedsAdded()*/)
			{
				PossibleImapMailbox(parentName, hierarchyDelimiter,kNoselect |		// be defensive
											((boxFlags &	// only inherit certain flags from the child
											(kPublicMailbox | kOtherUsersMailbox | kPersonalMailbox))));
			}
		}

        hostFolder->CreateClientSubfolderInfo(dupFolderPath, hierarchyDelimiter);
		a_nsIFolder->GetChildWithURI(uri, PR_TRUE, getter_AddRefs(child));
    }
	if (child)
	{
		nsCOMPtr <nsIMsgImapMailFolder> imapFolder = do_QueryInterface(child);
		if (imapFolder)
		{
			nsXPIDLCString onlineName;
			nsXPIDLString unicodeName;
			imapFolder->SetVerifiedAsOnlineFolder(PR_TRUE);
			imapFolder->SetHierarchyDelimiter(hierarchyDelimiter);
			imapFolder->SetBoxFlags(boxFlags);
			imapFolder->GetOnlineName(getter_Copies(onlineName));
			if (! ((const char*) onlineName) || nsCRT::strlen((const char *) onlineName) == 0
				|| nsCRT::strcmp((const char *) onlineName, dupFolderPath))
				imapFolder->SetOnlineName(dupFolderPath);
			if (NS_SUCCEEDED(CreatePRUnicharStringFromUTF7(folderName, getter_Copies(unicodeName))))
				child->SetName(unicodeName);

		}
    }
	return NS_OK;
}

nsresult nsImapIncomingServer::GetFolder(const char* name, nsIMsgFolder** pFolder)
{
    nsresult rv = NS_ERROR_NULL_POINTER;
    if (!name || !*name || !pFolder) return rv;
    *pFolder = nsnull;
    nsCOMPtr<nsIFolder> rootFolder;
    rv = GetRootFolder(getter_AddRefs(rootFolder));
    if (NS_SUCCEEDED(rv) && rootFolder)
    {
        char* uri = nsnull;
        rv = rootFolder->GetURI(&uri);
        if (NS_SUCCEEDED(rv) && uri)
        {
            nsAutoString uriAutoString = uri;
            uriAutoString.Append('/');
            uriAutoString.Append(name);
            NS_WITH_SERVICE(nsIRDFService, rdf, kRDFServiceCID, &rv);
            if (NS_FAILED(rv)) return rv;
            char* uriString = uriAutoString.ToNewCString();
            nsCOMPtr<nsIRDFResource> res;
            rv = rdf->GetResource(uriString, getter_AddRefs(res));
            if (NS_SUCCEEDED(rv))
            {
                nsCOMPtr<nsIMsgFolder> folder(do_QueryInterface(res, &rv));
                if (NS_SUCCEEDED(rv) && folder)
                {
                    *pFolder = folder;
                    NS_ADDREF(*pFolder);
                }
            }
            delete [] uriString;
        }
        PR_FREEIF(uri);
    }
    return rv;
}


NS_IMETHODIMP  nsImapIncomingServer::OnlineFolderDelete(const char *aFolderName) 
{
	return NS_OK;
}

NS_IMETHODIMP  nsImapIncomingServer::OnlineFolderCreateFailed(const char *aFolderName) 
{
	return NS_OK;
}

NS_IMETHODIMP nsImapIncomingServer::OnlineFolderRename(const char *oldName, const char *newName)
{
    nsresult rv = NS_ERROR_FAILURE;
    if (newName && *newName)
    {
        nsCOMPtr<nsIFolder> iFolder;
        nsCOMPtr<nsIMsgImapMailFolder> parent;
        nsCOMPtr<nsIMsgFolder> me;
        rv = GetFolder(oldName, getter_AddRefs(me));
        if (NS_FAILED(rv)) return rv;
        rv = me->GetParent(getter_AddRefs(iFolder));
        if (NS_SUCCEEDED(rv))
        {
            parent = do_QueryInterface(iFolder, &rv);
            if (NS_SUCCEEDED(rv))
                    parent->RemoveSubFolder(me);
        }
        nsCOMPtr<nsIFolder> rootFolder;
        rv = GetRootFolder(getter_AddRefs(rootFolder));
        if (NS_SUCCEEDED(rv))
        {
			PRUnichar hierarchyDelimiter = '/';
            if (parent)
                parent->GetHierarchyDelimiter(&hierarchyDelimiter);

            nsCOMPtr<nsIMsgImapMailFolder> imapRootFolder =
                do_QueryInterface(rootFolder, &rv);
            if (NS_SUCCEEDED(rv))
            rv = imapRootFolder->CreateClientSubfolderInfo(newName, hierarchyDelimiter);
        }
    }
	return rv;
}

NS_IMETHODIMP  nsImapIncomingServer::FolderIsNoSelect(const char *aFolderName, PRBool *result) 
{
	if (!result)
		return NS_ERROR_NULL_POINTER;

	*result = PR_FALSE;
	return NS_OK;
}

NS_IMETHODIMP nsImapIncomingServer::SetFolderAdminURL(const char *aFolderName, const char *aFolderAdminUrl)
{
    nsresult rv = NS_ERROR_FAILURE;
	return rv;
}

NS_IMETHODIMP  nsImapIncomingServer::SubscribeUpgradeFinished(PRBool bringUpSubscribeUI) 
{
	return NS_OK;
}

NS_IMETHODIMP nsImapIncomingServer::DiscoveryDone()
{
	nsresult rv = NS_ERROR_FAILURE;
	// first, we need some indication of whether this is the subscribe UI running or not.
	// The subscribe UI doesn't want to delete non-verified folders, or go through the
	// extra list process. I'll leave that as an exercise for Seth for now. I think 
	// we'll probably need to add some state to the imap url to indicate whether the
	// subscribe UI started the url, and need to pass that through to discovery done.

//	m_haveDiscoveredAllFolders = PR_TRUE;

	PRInt32 numUnverifiedFolders;
	nsCOMPtr<nsISupportsArray> unverifiedFolders;

	rv = NS_NewISupportsArray(getter_AddRefs(unverifiedFolders));
	if(NS_FAILED(rv))
		return rv;

	rv = GetUnverifiedFolders(unverifiedFolders, &numUnverifiedFolders);
	if (numUnverifiedFolders > 0)
	{
		for (PRInt32 k = 0; k < numUnverifiedFolders; k++)
		{
			PRBool explicitlyVerify = PR_FALSE;
			PRBool hasSubFolders = PR_FALSE;
			nsCOMPtr<nsISupports> element;
			unverifiedFolders->GetElementAt(k, getter_AddRefs(element));

			nsCOMPtr<nsIMsgImapMailFolder> currentImapFolder = do_QueryInterface(element, &rv);
			nsCOMPtr<nsIFolder> currentFolder = do_QueryInterface(element, &rv);
			if (NS_FAILED(rv))
				continue;
			if ((NS_SUCCEEDED(currentImapFolder->GetExplicitlyVerify(&explicitlyVerify)) && explicitlyVerify) ||
				((NS_SUCCEEDED(currentFolder->GetHasSubFolders(&hasSubFolders)) && hasSubFolders)
					&& !NoDescendentsAreVerified(currentFolder)))
			{
				// If there are no subfolders and this is unverified, we don't want to run
				// this url.  That is, we want to undiscover the folder.
				// If there are subfolders and no descendants are verified, we want to 
				// undiscover all of the folders.
				// Only if there are subfolders and at least one of them is verified do we want
				// to refresh that folder's flags, because it won't be going away.
				currentImapFolder->SetExplicitlyVerify(PR_FALSE);
				currentImapFolder->List();
			}
			else
			{
				DeleteNonVerifiedFolders(currentFolder);
			}
		}
	}

#if 0
	if (currentContext->imapURLPane && (currentContext->imapURLPane->GetPaneType() == MSG_SUBSCRIBEPANE))
	{
		// Finished discovering folders for the subscribe pane.
		((MSG_SubscribePane *)(currentContext->imapURLPane))->ReportIMAPFolderDiscoveryFinished();
	}
	else
	{
		// only do this if we're discovering folders for real (i.e. not subscribe UI)

  
		if (URL_s && URL_s->msg_pane && !URL_s->msg_pane->GetPreImapFolderVerifyUrlExitFunction())
		{
    		URL_s->msg_pane->SetPreImapFolderVerifyUrlExitFunction(URL_s->pre_exit_fn);
    		URL_s->pre_exit_fn = DeleteNonVerifiedExitFunction;
		}

	    PR_ASSERT(currentContext->imapURLPane);

		// Go through folders and find if there are still any that are left unverified.
		// If so, manually LIST them to see if we can find out any info about them.
		char *hostName = NET_ParseURL(URL_s->address, GET_HOST_PART);
		if (hostName && currentContext->mailMaster && currentContext->imapURLPane)
		{
			MSG_FolderInfoContainer *hostContainerInfo = currentContext->mailMaster->GetImapMailFolderTreeForHost(hostName);
			MSG_IMAPFolderInfoContainer *hostInfo = hostContainerInfo ? hostContainerInfo->GetIMAPFolderInfoContainer() : (MSG_IMAPFolderInfoContainer *)NULL;
			if (hostInfo)
			{
				// for each folder

				int32 numberOfUnverifiedFolders = hostInfo->GetUnverifiedFolders(NULL, 0);
				if (numberOfUnverifiedFolders > 0)
				{
					MSG_IMAPFolderInfoMail **folderList = (MSG_IMAPFolderInfoMail **)PR_Malloc(sizeof(MSG_IMAPFolderInfoMail*) * numberOfUnverifiedFolders);
					if (folderList)
					{
						int32 numUsed = hostInfo->GetUnverifiedFolders(folderList, numberOfUnverifiedFolders);
						for (int32 k = 0; k < numUsed; k++)
						{
							MSG_IMAPFolderInfoMail *currentFolder = folderList[k];
							if (currentFolder->GetExplicitlyVerify() ||
								((currentFolder->GetNumSubFolders() > 0) && !NoDescendantsAreVerified(currentFolder)))
							{
								// If there are no subfolders and this is unverified, we don't want to run
								// this url.  That is, we want to undiscover the folder.
								// If there are subfolders and no descendants are verified, we want to 
								// undiscover all of the folders.
								// Only if there are subfolders and at least one of them is verified do we want
								// to refresh that folder's flags, because it won't be going away.
								currentFolder->SetExplicitlyVerify(PR_FALSE);
								char *url = CreateIMAPListFolderURL(hostName, currentFolder->GetOnlineName(), currentFolder->GetOnlineHierarchySeparator());
								if (url)
								{
									MSG_UrlQueue::AddUrlToPane(url, NULL, currentContext->imapURLPane);
									XP_FREE(url);
								}
							}
						}
						XP_FREE(folderList);
					}
				}
			}
			XP_FREE(hostName);
		}
		else
		{
			PR_ASSERT(PR_FALSE);
		}
	}

#endif
	return rv;
}

nsresult nsImapIncomingServer::DeleteNonVerifiedFolders(nsIFolder *curFolder)
{
	PRBool autoUnsubscribeFromNoSelectFolders = PR_TRUE;
	nsresult rv;
	NS_WITH_SERVICE(nsIPref, prefs, kPrefServiceCID, &rv);
	if(NS_SUCCEEDED(rv))
	{
		rv = prefs->GetBoolPref("mail.imap.auto_unsubscribe_from_noselect_folders", &autoUnsubscribeFromNoSelectFolders);
	}
//	return rv;
	nsCOMPtr<nsIEnumerator> subFolders;

	rv = curFolder->GetSubFolders(getter_AddRefs(subFolders));
	if(NS_SUCCEEDED(rv))
	{
		nsAdapterEnumerator *simpleEnumerator =	new nsAdapterEnumerator(subFolders);
		if (simpleEnumerator == nsnull)
			return NS_ERROR_OUT_OF_MEMORY;
		PRBool moreFolders;

		while (NS_SUCCEEDED(simpleEnumerator->HasMoreElements(&moreFolders)) && moreFolders)
		{
			nsCOMPtr<nsISupports> child;
			rv = simpleEnumerator->GetNext(getter_AddRefs(child));
			if (NS_SUCCEEDED(rv) && child) 
			{
				PRBool childVerified = PR_FALSE;
				nsCOMPtr <nsIMsgImapMailFolder> childImapFolder = do_QueryInterface(child, &rv);
				if (NS_SUCCEEDED(rv) && childImapFolder)
				{
					PRUint32 flags;

					nsCOMPtr <nsIMsgFolder> childFolder = do_QueryInterface(child, &rv);
					rv = childImapFolder->GetVerifiedAsOnlineFolder(&childVerified);

					rv = childFolder->GetFlags(&flags);
					PRBool folderIsNoSelectFolder = NS_SUCCEEDED(rv) && ((flags & MSG_FOLDER_FLAG_IMAP_NOSELECT) != 0);

		           	PRBool usingSubscription = PR_TRUE;
					GetUsingSubscription(&usingSubscription);
					if (usingSubscription)
					{
						PRBool folderIsNameSpace = PR_FALSE;
						PRBool noDescendentsAreVerified = NoDescendentsAreVerified(childFolder);
						PRBool shouldDieBecauseNoSelect = (folderIsNoSelectFolder ? 
							((noDescendentsAreVerified || AllDescendentsAreNoSelect(childFolder)) && !folderIsNameSpace)
							: PR_FALSE);
						if (!childVerified && (noDescendentsAreVerified || shouldDieBecauseNoSelect))
						{
						}

					}
					else
					{
					}
				}
			}
		}
		delete simpleEnumerator;
	}

#if 0
            
            MSG_IMAPFolderInfoMail *parentImapFolder = (parentFolder->GetType() == FOLDER_IMAPMAIL) ? 
            												(MSG_IMAPFolderInfoMail *) parentFolder :
            												(MSG_IMAPFolderInfoMail *)NULL;
            
            // if the parent is the imap container or an imap folder whose children were listed, then this bool is true.
            // We only delete .snm files whose parent's children were listed											
            XP_Bool parentChildrenWereListed =	(parentImapFolder == NULL) || 
            									(LL_CMP(parentImapFolder->GetTimeStampOfLastList(), >= , IMAP_GetTimeStampOfNonPipelinedList()));

			MSG_IMAPHost *imapHost = currentImapFolder->GetIMAPHost();
           	PRBool usingSubscription = imapHost ? imapHost->GetIsHostUsingSubscription() : PR_TRUE;
			PRBool folderIsNoSelectFolder = (currentImapFolder->GetFolderPrefFlags() & MSG_FOLDER_FLAG_IMAP_NOSELECT) != 0;
			PRBool shouldDieBecauseNoSelect = usingSubscription ?
									(folderIsNoSelectFolder ? ((NoDescendantsAreVerified(currentImapFolder) || AllDescendantsAreNoSelect(currentImapFolder)) && !currentImapFolder->GetFolderIsNamespace()): PR_FALSE)
									: PR_FALSE;
			PRBool offlineCreate = (currentImapFolder->GetFolderPrefFlags() & MSG_FOLDER_FLAG_CREATED_OFFLINE) != 0;

            if (!currentImapFolder->GetExplicitlyVerify() && !offlineCreate &&
				((autoUnsubscribeFromNoSelectFolders && shouldDieBecauseNoSelect) ||
				((usingSubscription ? PR_TRUE : parentChildrenWereListed) && !currentImapFolder->GetIsOnlineVerified() && NoDescendantsAreVerified(currentImapFolder))))
            {
                // This folder is going away.
				// Give notification so that folder menus can be rebuilt.
				if (*url_pane)
				{
					XPPtrArray referringPanes;
					uint32 total;

					(*url_pane)->GetMaster()->FindPanesReferringToFolder(currentFolder,&referringPanes);
					total = referringPanes.GetSize();
					for (int i=0; i < total;i++)
					{
						MSG_Pane *currentPane = (MSG_Pane *) referringPanes.GetAt(i);
						if (currentPane)
						{
							if (currentPane->GetFolder() == currentFolder)
							{
								currentPane->SetFolder(NULL);
								FE_PaneChanged(currentPane, PR_TRUE, MSG_PaneNotifyFolderDeleted, (uint32)currentFolder);
							}
						}
					}

                	FE_PaneChanged(*url_pane, PR_TRUE, MSG_PaneNotifyFolderDeleted, (uint32)currentFolder);

					// If we are running the IMAP subscribe upgrade, and we are deleting the folder that we'd normally
					// try to load after the process completes, then tell the pane not to load that folder.
					if (((MSG_ThreadPane *)(*url_pane))->GetIMAPUpgradeFolder() == currentFolder)
						((MSG_ThreadPane *)(*url_pane))->SetIMAPUpgradeFolder(NULL);

                	if ((*url_pane)->GetFolder() == currentFolder)
						*url_pane = NULL;


					if (shouldDieBecauseNoSelect && autoUnsubscribeFromNoSelectFolders && usingSubscription)
					{
						char *unsubscribeUrl = CreateIMAPUnsubscribeMailboxURL(imapHost->GetHostName(), currentImapFolder->GetOnlineName(), currentImapFolder->GetOnlineHierarchySeparator());
						if (unsubscribeUrl)
						{
							if (url_pane)
								MSG_UrlQueue::AddUrlToPane(unsubscribeUrl, NULL, *url_pane);
							else if (folderPane)
								MSG_UrlQueue::AddUrlToPane(unsubscribeUrl, NULL, folderPane);
							XP_FREE(unsubscribeUrl);
						}

						if (AllDescendantsAreNoSelect(currentImapFolder) && (currentImapFolder->GetNumSubFolders() > 0))
						{
							// This folder has descendants, all of which are also \NoSelect.
							// We'd like to unsubscribe from all of these as well.
							if (url_pane)
								UnsubscribeFromAllDescendants(currentImapFolder, *url_pane);
							else if (folderPane)
								UnsubscribeFromAllDescendants(currentImapFolder, folderPane);
						}

					}
				}
				else
				{
#ifdef DEBUG_chrisf
					PR_ASSERT(PR_FALSE);
#endif
				}

				parentFolder->PropagateDelete(&currentFolder); // currentFolder is null on return
				numberOfSubFolders--;
				folderIndex--;
            }
            else
            {
                if (currentFolder->HasSubFolders())
                    DeleteNonVerifiedImapFolders(currentFolder, folderPane, url_pane);
            }
        }
        folderIndex++;  // not in for statement because we modify it
    }

#endif // 0

    nsCOMPtr<nsIFolder> parent;
	nsCOMPtr<nsIMsgFolder> msgFolder = do_QueryInterface(curFolder);
    rv = curFolder->GetParent(getter_AddRefs(parent));


    if (NS_SUCCEEDED(rv) && parent)
	{
		nsCOMPtr<nsIMsgImapMailFolder> imapParent = do_QueryInterface(parent);
		if (imapParent)
			imapParent->RemoveSubFolder(msgFolder);
	}

	return rv;
}

PRBool nsImapIncomingServer::NoDescendentsAreVerified(nsIFolder *parentFolder)
{
	PRBool nobodyIsVerified = PR_TRUE;
	
	nsCOMPtr<nsIEnumerator> subFolders;

	nsresult rv = parentFolder->GetSubFolders(getter_AddRefs(subFolders));
	if(NS_SUCCEEDED(rv))
	{
		nsAdapterEnumerator *simpleEnumerator =	new nsAdapterEnumerator(subFolders);
		if (simpleEnumerator == nsnull)
			return NS_ERROR_OUT_OF_MEMORY;
		PRBool moreFolders;

		while (NS_SUCCEEDED(simpleEnumerator->HasMoreElements(&moreFolders)) && moreFolders && nobodyIsVerified)
		{
			nsCOMPtr<nsISupports> child;
			rv = simpleEnumerator->GetNext(getter_AddRefs(child));
			if (NS_SUCCEEDED(rv) && child) 
			{
				PRBool childVerified = PR_FALSE;
				nsCOMPtr <nsIMsgImapMailFolder> childImapFolder = do_QueryInterface(child, &rv);
				if (NS_SUCCEEDED(rv) && childImapFolder)
				{
					nsCOMPtr <nsIFolder> childFolder = do_QueryInterface(child, &rv);
					rv = childImapFolder->GetVerifiedAsOnlineFolder(&childVerified);
					nobodyIsVerified = !childVerified && NoDescendentsAreVerified(childFolder);
				}
			}
		}
		delete simpleEnumerator;
	}

	return nobodyIsVerified;
}


PRBool nsImapIncomingServer::AllDescendentsAreNoSelect(nsIFolder *parentFolder)
{
	PRBool allDescendentsAreNoSelect = PR_TRUE;
	nsCOMPtr<nsIEnumerator> subFolders;

	nsresult rv = parentFolder->GetSubFolders(getter_AddRefs(subFolders));
	if(NS_SUCCEEDED(rv))
	{
		nsAdapterEnumerator *simpleEnumerator =	new nsAdapterEnumerator(subFolders);
		if (simpleEnumerator == nsnull)
			return NS_ERROR_OUT_OF_MEMORY;
		PRBool moreFolders;

		while (NS_SUCCEEDED(simpleEnumerator->HasMoreElements(&moreFolders)) && moreFolders && allDescendentsAreNoSelect)
		{
			nsCOMPtr<nsISupports> child;
			rv = simpleEnumerator->GetNext(getter_AddRefs(child));
			if (NS_SUCCEEDED(rv) && child) 
			{
				PRBool childIsNoSelect = PR_FALSE;
				nsCOMPtr <nsIMsgImapMailFolder> childImapFolder = do_QueryInterface(child, &rv);
				if (NS_SUCCEEDED(rv) && childImapFolder)
				{
					PRUint32 flags;

					nsCOMPtr <nsIMsgFolder> childFolder = do_QueryInterface(child, &rv);
					rv = childFolder->GetFlags(&flags);
					childIsNoSelect = NS_SUCCEEDED(rv) && (flags & MSG_FOLDER_FLAG_IMAP_NOSELECT);
					allDescendentsAreNoSelect = !childIsNoSelect && AllDescendentsAreNoSelect(childFolder);
				}
			}
		}
		delete simpleEnumerator;
	}
#if 0
	int numberOfSubfolders = parentFolder->GetNumSubFolders();
	
	for (int childIndex=0; allDescendantsAreNoSelect && (childIndex < numberOfSubfolders); childIndex++)
	{
		MSG_IMAPFolderInfoMail *currentChild = (MSG_IMAPFolderInfoMail *) parentFolder->GetSubFolder(childIndex);
		allDescendentsAreNoSelect = (currentChild->GetFolderPrefFlags() & MSG_FOLDER_PREF_IMAPNOSELECT) &&
									AllDescendentsAreNoSelect(currentChild);
	}
#endif // 0
	return allDescendentsAreNoSelect;
}


#if 0
void nsImapIncomingServer::UnsubscribeFromAllDescendents(nsIFolder *parentFolder)
{
	int numberOfSubfolders = parentFolder->GetNumSubFolders();
	
	for (int childIndex=0; childIndex < numberOfSubfolders; childIndex++)
	{
		MSG_IMAPFolderInfoMail *currentChild = (MSG_IMAPFolderInfoMail *) parentFolder->GetSubFolder(childIndex);
		char *unsubscribeUrl = CreateIMAPUnsubscribeMailboxURL(currentChild->GetHostName(), currentChild->GetOnlineName(), currentChild->GetOnlineHierarchySeparator());	// unsubscribe from child
		if (unsubscribeUrl)
		{
			MSG_UrlQueue::AddUrlToPane(unsubscribeUrl, NULL, pane);
			XP_FREE(unsubscribeUrl);
		}
		UnsubscribeFromAllDescendants(currentChild);	// unsubscribe from its children
	}
}
#endif // 0


NS_IMETHODIMP
nsImapIncomingServer::FEAlert(const PRUnichar* aString)
{
	nsresult rv;
	NS_WITH_SERVICE(nsIPrompt, dialog, kNetSupportDialogCID, &rv);

	rv = dialog->Alert(nsAutoString(aString).GetUnicode());
    return rv;
}

NS_IMETHODIMP  nsImapIncomingServer::FEAlertFromServer(const char *aString)
{
	nsresult rv = NS_OK;
	NS_WITH_SERVICE(nsIPrompt, dialog, kNetSupportDialogCID, &rv);

	const char *serverSaid = aString;
	if (serverSaid)
	{
		// skip over the first two words, I guess.
		char *whereRealMessage = PL_strchr(serverSaid, ' ');
		if (whereRealMessage)
			whereRealMessage++;
		if (whereRealMessage)
			whereRealMessage = PL_strchr(whereRealMessage, ' ');
		if (whereRealMessage)
			whereRealMessage++;

		PRUnichar *serverSaidPrefix = nsnull;
		GetImapStringByID(IMAP_SERVER_SAID, &serverSaidPrefix);
		if (serverSaidPrefix)
		{
			nsAutoString message(serverSaidPrefix);
			message += whereRealMessage ? whereRealMessage : serverSaid;
			rv = dialog->Alert(message.GetUnicode());

			PR_Free(serverSaidPrefix);
		}
	}

    return rv;
}

/* This is the next generation string retrieval call */
static NS_DEFINE_CID(kStringBundleServiceCID, NS_STRINGBUNDLESERVICE_CID);

#define IMAP_MSGS_URL       "chrome://messenger/locale/imapMsgs.properties"

NS_IMETHODIMP  nsImapIncomingServer::GetImapStringByID(PRInt32 aMsgId, PRUnichar **aString)
{
	nsAutoString	resultString = "???";
	nsresult res = NS_OK;

	if (!m_stringBundle)
	{
		char*       propertyURL = NULL;

		propertyURL = IMAP_MSGS_URL;

		NS_WITH_SERVICE(nsIStringBundleService, sBundleService, kStringBundleServiceCID, &res); 
		if (NS_SUCCEEDED(res) && (nsnull != sBundleService)) 
		{
			nsILocale   *locale = nsnull;

			res = sBundleService->CreateBundle(propertyURL, locale, getter_AddRefs(m_stringBundle));
		}
	}
	if (m_stringBundle)
	{
		PRUnichar *ptrv = nsnull;
		res = m_stringBundle->GetStringFromID(aMsgId, &ptrv);

		if (NS_FAILED(res)) 
		{
			resultString = "[StringID";
			resultString.Append(aMsgId, 10);
			resultString += "?]";
			*aString = resultString.ToNewUnicode();
		}
		else
		{
			*aString = ptrv;
		}
	}
	else
	{
		res = NS_OK;
		*aString = resultString.ToNewUnicode();
	}
	return res;
}


nsresult nsImapIncomingServer::GetUnverifiedFolders(nsISupportsArray *aFoldersArray, PRInt32 *aNumUnverifiedFolders)
{
	// can't have both be null, but one null is OK, since the caller
	// may just be trying to count the number of unverified folders.
	if (!aFoldersArray && !aNumUnverifiedFolders)
		return NS_ERROR_NULL_POINTER;

	if (aNumUnverifiedFolders)
		*aNumUnverifiedFolders = 0;
	nsCOMPtr<nsIFolder> rootFolder;
	nsresult rv = GetRootFolder(getter_AddRefs(rootFolder));
	if(NS_SUCCEEDED(rv) && rootFolder)
		rv = GetUnverifiedSubFolders(rootFolder, aFoldersArray, aNumUnverifiedFolders);
	return rv;
}

nsresult nsImapIncomingServer::GetUnverifiedSubFolders(nsIFolder *parentFolder, nsISupportsArray *aFoldersArray, PRInt32 *aNumUnverifiedFolders)
{
	nsresult rv = NS_OK;

	nsCOMPtr <nsIMsgImapMailFolder> imapFolder = do_QueryInterface(parentFolder);
	PRBool verified;
	if (imapFolder)
	{
		rv = imapFolder->GetVerifiedAsOnlineFolder(&verified);
		if (NS_SUCCEEDED(rv) && !verified)
		{
			if (aFoldersArray)
			{
				nsCOMPtr <nsISupports> supports = do_QueryInterface(imapFolder);
				aFoldersArray->AppendElement(supports);
			}
			if (aNumUnverifiedFolders)
				(*aNumUnverifiedFolders)++;
		}
	}
	nsCOMPtr<nsIEnumerator> subFolders;

	rv = parentFolder->GetSubFolders(getter_AddRefs(subFolders));
	if(NS_SUCCEEDED(rv))
	{
		nsAdapterEnumerator *simpleEnumerator =	new nsAdapterEnumerator(subFolders);
		if (simpleEnumerator == nsnull)
			return NS_ERROR_OUT_OF_MEMORY;
		PRBool moreFolders;

		while (NS_SUCCEEDED(simpleEnumerator->HasMoreElements(&moreFolders)) && moreFolders)
		{
			nsCOMPtr<nsISupports> child;
			rv = simpleEnumerator->GetNext(getter_AddRefs(child));
			if (NS_SUCCEEDED(rv) && child) 
			{
				nsCOMPtr <nsIFolder> childFolder = do_QueryInterface(child, &rv);
				if (NS_SUCCEEDED(rv) && childFolder)
				{
					rv = GetUnverifiedSubFolders(childFolder, aFoldersArray, aNumUnverifiedFolders);
					if (!NS_SUCCEEDED(rv))
						break;
				}
			}
		}
		delete simpleEnumerator;
	}
	return rv;
}

NS_IMETHODIMP nsImapIncomingServer::PromptForPassword(char ** aPassword,
                                                      nsIMsgWindow * aMsgWindow)
{
    PRUnichar *passwordTemplate = IMAPGetStringByID(IMAP_ENTER_PASSWORD_PROMPT);
    PRUnichar *passwordTitle = IMAPGetStringByID(IMAP_ENTER_PASSWORD_PROMPT_TITLE);
    PRUnichar *passwordText = nsnull;
    nsXPIDLCString hostName;
    nsXPIDLCString userName;

    GetHostName(getter_Copies(hostName));
    GetUsername(getter_Copies(userName));

    passwordText = nsTextFormatter::smprintf(passwordTemplate, (const char *) userName, (const char *) hostName);
    nsresult rv =  GetPasswordWithUI(passwordText, passwordTitle, aMsgWindow, aPassword);
    nsTextFormatter::smprintf_free(passwordText);
    nsCRT::free(passwordTemplate);
    nsCRT::free(passwordTitle);
    return rv;
}

// for the nsIImapServerSink interface
NS_IMETHODIMP  nsImapIncomingServer::SetCapability(PRUint32 capability)
{
	m_capability = capability;
	SetCapabilityPref(capability);
	return NS_OK;
}

NS_IMETHODIMP  nsImapIncomingServer::CommitNamespaces()
{

	nsresult rv;
	NS_WITH_SERVICE(nsIImapHostSessionList, hostSession, kCImapHostSessionList, &rv);
    if (NS_FAILED(rv)) 
		return rv;

	return hostSession->CommitNamespacesForHost(this);

}

NS_IMETHODIMP nsImapIncomingServer::PseudoInterruptMsgLoad(nsIImapUrl *aImapUrl, PRBool *interrupted)
{
	nsresult rv = NS_OK;
	PRBool canRunUrl = PR_FALSE;
    PRBool hasToWait = PR_FALSE;
	nsCOMPtr<nsIImapProtocol> connection;

    PR_CEnterMonitor(this);

	// iterate through the connection cache for a connection that is loading
	// a message in this folder and should be pseudo-interrupted.
	PRUint32 cnt;
    nsCOMPtr<nsISupports> aSupport;

    rv = m_connectionCache->Count(&cnt);
    if (NS_FAILED(rv)) return rv;
    for (PRUint32 i = 0; i < cnt && !canRunUrl && !hasToWait; i++) 
	{
        aSupport = getter_AddRefs(m_connectionCache->ElementAt(i));
        connection = do_QueryInterface(aSupport);
		if (connection)
			rv = connection->PseudoInterruptMsgLoad(aImapUrl, interrupted);
	}
    
    PR_CExitMonitor(this);
	return rv;
}

NS_IMETHODIMP nsImapIncomingServer::ResetNamespaceReferences()
{
	return NS_ERROR_NOT_IMPLEMENTED;
}

#if FINISHED_PORTED_NAMESPACE_STUFF
		int numberOfChildren = GetNumSubFolders();
	for (int childIndex = 0; childIndex < numberOfChildren; childIndex++)
	{
		MSG_IMAPFolderInfoMail *currentChild = (MSG_IMAPFolderInfoMail *) GetSubFolder(childIndex);
		currentChild->ResetNamespaceReferences();
	}

	void MSG_IMAPFolderInfoMail::SetFolderIsNamespace(XP_Bool isNamespace)
{
	m_folderIsNamespace = isNamespace;
}

void MSG_IMAPFolderInfoMail::InitializeFolderCreatedOffline()
{
	TIMAPNamespace *ns = IMAPNS_GetNamespaceForFolder(m_host->GetHostName(), GetOnlineName(), '/');
	SetOnlineHierarchySeparator(IMAPNS_GetDelimiterForNamespace(ns));
}

TIMAPNamespace *MSG_IMAPFolderInfoMail::GetNamespaceForFolder()
{
	if (!m_namespace)
	{
#ifdef DEBUG_bienvenu
		// Make sure this isn't causing us to open the database
		PR_ASSERT(m_OnlineHierSeparator != kOnlineHierarchySeparatorUnknown);
#endif
		m_namespace = IMAPNS_GetNamespaceForFolder(m_host->GetHostName(), GetOnlineName(), GetOnlineHierarchySeparator());
		PR_ASSERT(m_namespace);
		if (m_namespace)
		{
			IMAPNS_SuggestHierarchySeparatorForNamespace(m_namespace, GetOnlineHierarchySeparator());
			m_folderIsNamespace = IMAPNS_GetFolderIsNamespace(m_host->GetHostName(), GetOnlineName(), GetOnlineHierarchySeparator(), m_namespace);
		}
	}
	return m_namespace;
}

void MSG_IMAPFolderInfoMail::SetNamespaceForFolder(TIMAPNamespace *ns)
{
#ifdef DEBUG_bienvenu
	NS_ASSERTION(ns, "null namespace");
#endif
	m_namespace = ns;
}

void MSG_IMAPFolderInfoMail::ResetNamespaceReferences()
{
	// this
	m_namespace = IMAPNS_GetNamespaceForFolder(GetHostName(), GetOnlineName(), GetOnlineHierarchySeparator());
	NS_ASSERTION(m_namespace, "resetting null namespace");
	if (m_namespace)
		m_folderIsNamespace = IMAPNS_GetFolderIsNamespace(GetHostName(), GetOnlineName(), GetOnlineHierarchySeparator(), m_namespace);
	else
		m_folderIsNamespace = PR_FALSE;

	// children
	int numberOfChildren = GetNumSubFolders();
	for (int childIndex = 0; childIndex < numberOfChildren; childIndex++)
	{
		MSG_IMAPFolderInfoMail *currentChild = (MSG_IMAPFolderInfoMail *) GetSubFolder(childIndex);
		currentChild->ResetNamespaceReferences();
	}
}

#endif  //FINISHED_PORTED_NAMESPACE_STUFF

NS_IMETHODIMP  nsImapIncomingServer::SetUserAuthenticated(PRBool authenticated)
{
	return NS_OK;
}

/* void SetMailServerUrls (in string manageMailAccount, in string manageLists, in string manageFilters); */
NS_IMETHODIMP  nsImapIncomingServer::SetMailServerUrls(const char *manageMailAccount, const char *manageLists, const char *manageFilters)
{
	return SetManageMailAccountUrl((char *) manageMailAccount);
}

NS_IMETHODIMP nsImapIncomingServer::SetManageMailAccountUrl(const char *manageMailAccountUrl)
{
	m_manageMailAccountUrl = manageMailAccountUrl;
	return NS_OK;
}

NS_IMETHODIMP nsImapIncomingServer::GetManageMailAccountUrl(char **manageMailAccountUrl)
{
	if (!manageMailAccountUrl)
		return NS_ERROR_NULL_POINTER;

	*manageMailAccountUrl = m_manageMailAccountUrl.ToNewCString();
	return NS_OK;
}

NS_IMETHODIMP nsImapIncomingServer::RemoveChannelFromUrl(nsIMsgMailNewsUrl *aUrl, PRUint32 statusCode)
{
  nsresult rv = NS_OK;
  if (aUrl)
  {
    nsCOMPtr<nsIImapUrl> imapUrl = do_QueryInterface(aUrl);
    if (imapUrl)
      rv = imapUrl->RemoveChannel(statusCode);
  }

  return rv;
}

NS_IMETHODIMP nsImapIncomingServer::CreatePRUnicharStringFromUTF7(const char * aSourceString, PRUnichar **aUnicodeStr)
{
	return CreateUnicodeStringFromUtf7(aSourceString, aUnicodeStr);
}

