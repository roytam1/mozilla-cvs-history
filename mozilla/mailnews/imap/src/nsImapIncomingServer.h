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

#ifndef __nsImapIncomingServer_h
#define __nsImapIncomingServer_h

#include "msgCore.h"
#include "nsIImapIncomingServer.h"
#include "nsISupportsArray.h"
#include "nsMsgIncomingServer.h"
#include "nsIImapServerSink.h"
#include "nsIStringBundle.h"
#include "nsIMsgLogonRedirector.h"
#include "nsISubscribableServer.h"
#include "nsIUrlListener.h"

/* get some implementation from nsMsgIncomingServer */
class nsImapIncomingServer : public nsMsgIncomingServer,
                             public nsIImapIncomingServer,
							              public nsIImapServerSink,
							              public nsIMsgLogonRedirectionRequester,
										  public nsISubscribableServer,
										  public nsIUrlListener
                             
{
public:
    NS_DECL_ISUPPORTS_INHERITED

    nsImapIncomingServer();
    virtual ~nsImapIncomingServer();

    // overriding nsMsgIncomingServer methods
	NS_IMETHOD SetKey(const char * aKey);  // override nsMsgIncomingServer's implementation...
	NS_IMETHOD GetLocalStoreType(char * *type);

	NS_DECL_NSIIMAPINCOMINGSERVER
	NS_DECL_NSIIMAPSERVERSINK
	NS_DECL_NSIMSGLOGONREDIRECTIONREQUESTER
    NS_DECL_NSISUBSCRIBABLESERVER
	NS_DECL_NSIURLLISTENER

	NS_IMETHOD PerformBiff();
	NS_IMETHOD PerformExpand(nsIMsgWindow *aMsgWindow);
	NS_IMETHOD CloseCachedConnections();
    NS_IMETHOD GetConstructedPrettyName(PRUnichar **retval);
        NS_IMETHOD GetCanBeDefaultServer(PRBool *canBeDefaultServer);
        NS_IMETHOD GetCanSearchMessages(PRBool *canSearchMessages);
    NS_IMETHOD GetOfflineSupportLevel(PRInt32 *aSupportLevel);
    NS_IMETHOD GeneratePrettyNameForMigration(PRUnichar **aPrettyName);
        NS_IMETHOD GetSupportsDiskSpace(PRBool *aSupportsDiskSpace);
    NS_IMETHOD GetCanCreateFoldersOnServer(PRBool *aCanCreateFoldersOnServer);
    NS_IMETHOD GetCanFileMessagesOnServer(PRBool *aCanFileMessagesOnServer);
    NS_IMETHOD GetFilterScope(nsMsgSearchScopeValue *filterScope);
    NS_IMETHOD GetSearchScope(nsMsgSearchScopeValue *searchScope);
  NS_IMETHOD GetServerRequiresPasswordForBiff(PRBool *_retval);
  NS_IMETHOD GetNumIdleConnections(PRInt32 *aNumIdleConnections);
protected:
	nsresult GetFolder(const char* name, nsIMsgFolder** pFolder);
    nsresult ResetFoldersToUnverified(nsIFolder *parentFolder);
	nsresult GetUnverifiedSubFolders(nsIFolder *parentFolder, nsISupportsArray *aFoldersArray, PRInt32 *aNumUnverifiedFolders);
	nsresult GetUnverifiedFolders(nsISupportsArray *aFolderArray, PRInt32 *aNumUnverifiedFolders);

	nsresult DeleteNonVerifiedFolders(nsIFolder *parentFolder);
	PRBool NoDescendentsAreVerified(nsIFolder *parentFolder);
	PRBool AllDescendentsAreNoSelect(nsIFolder *parentFolder);

  nsresult GetStringBundle();
  const char *GetPFCName();
  nsresult GetPFCForStringId(PRBool createIfMissing, PRInt32 stringId, nsIMsgFolder **aFolder);
private:
  nsresult SetDelimiterFromHierarchyDelimiter();
  nsresult SubscribeToFolder(const PRUnichar *aName, PRBool subscribe);
  nsresult CreateImapConnection (nsIEventQueue* aEventQueue,
                                   nsIImapUrl* aImapUrl,
                                   nsIImapProtocol** aImapConnection);
  nsresult CreateProtocolInstance(nsIEventQueue *aEventQueue, 
                                           nsIImapProtocol ** aImapConnection);
  nsresult RequestOverrideInfo(nsIMsgWindow *aMsgWindow);
  nsresult CreateHostSpecificPrefName(const char *prefPrefix, nsCAutoString &prefName);

  PRBool ConnectionTimeOut(nsIImapProtocol* aImapConnection);
  nsresult GetFormattedName(const PRUnichar *constructedPrettyName, PRUnichar **formattedPrettyName);  
  nsresult CreatePrefNameWithRedirectorType(const char *prefSuffix, nsCAutoString &prefName);
  nsCOMPtr<nsISupportsArray> m_connectionCache;
  nsCOMPtr<nsISupportsArray> m_urlQueue;
	nsCOMPtr<nsIStringBundle>	m_stringBundle;
  nsVoidArray       m_urlConsumers;
  PRUint32          m_capability;
  nsCString         m_manageMailAccountUrl;
  PRBool            m_readPFCName;
  PRBool            m_userAuthenticated;
  nsCString         m_pfcName;
  PRBool						m_waitingForConnectionInfo;
  PRInt32						m_redirectedLogonRetries;
  nsCOMPtr<nsIMsgLogonRedirector> m_logonRedirector;
  
  // subscribe dialog stuff
  PRBool	mDoingSubscribeDialog;
  PRBool	mDoingLsub;
  nsresult AddFolderToSubscribeDialog(const char *parentUri, const char *uri,const char *folderName);

  nsCOMPtr <nsISubscribableServer> mInner;
    nsresult EnsureInner();
    nsresult ClearInner();
};


#endif
