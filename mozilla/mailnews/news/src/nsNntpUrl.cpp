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
 * Copyright (C) 1999 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

#include "msgCore.h"    // precompiled header...
#include "prlog.h"

#include "nsIURL.h"
#include "nsNntpUrl.h"

#include "nsString.h"
#include "prmem.h"
#include "plstr.h"
#include "prprf.h"
#include "nsCRT.h"
#include "nsNewsUtils.h"

#include "nntpCore.h"

#include "nsCOMPtr.h"
#include "nsXPIDLString.h"
#include "nsIMsgDatabase.h"
#include "nsMsgDBCID.h"

static NS_DEFINE_CID(kCNewsDB, NS_NEWSDB_CID);
    
nsNntpUrl::nsNntpUrl()
{
	// nsINntpUrl specific code...
	m_newsHost = nsnull;
	m_articleList = nsnull;
	m_newsgroup = nsnull;
	m_offlineNews = nsnull;
	m_newsgroupList = nsnull;
    m_newsgroupPost = nsnull;
    m_newsgroupName = nsnull;
    m_messageKey = nsMsgKey_None;
	m_newsAction = nsINntpUrl::ActionGetNewNews;
    m_addDummyEnvelope = PR_FALSE;
    m_canonicalLineEnding = PR_FALSE;
}
 
nsNntpUrl::~nsNntpUrl()
{
	NS_IF_RELEASE(m_newsHost);
	NS_IF_RELEASE(m_articleList);
	NS_IF_RELEASE(m_newsgroup);
	NS_IF_RELEASE(m_offlineNews);
	NS_IF_RELEASE(m_newsgroupList);
    PR_FREEIF(m_newsgroupPost);
    PR_FREEIF(m_newsgroupName);
}
  
NS_IMPL_ADDREF_INHERITED(nsNntpUrl, nsMsgMailNewsUrl)
NS_IMPL_RELEASE_INHERITED(nsNntpUrl, nsMsgMailNewsUrl)
  
nsresult nsNntpUrl::QueryInterface(const nsIID &aIID, void** aInstancePtr)
{
    if (NULL == aInstancePtr) 
	{
        return NS_ERROR_NULL_POINTER;
    }
 
    if (aIID.Equals(nsINntpUrl::GetIID()))
	{
        *aInstancePtr = (void*) ((nsINntpUrl*)this);
        NS_ADDREF_THIS();
        return NS_OK;
    }
	if (aIID.Equals(NS_GET_IID(nsIMsgMessageUrl)))
	{
		*aInstancePtr = (void *) ((nsIMsgMessageUrl *) this);
		NS_ADDREF_THIS();
		return NS_OK;
	}

    return nsMsgMailNewsUrl::QueryInterface(aIID, aInstancePtr);
}


////////////////////////////////////////////////////////////////////////////////////
// Begin nsINntpUrl specific support
////////////////////////////////////////////////////////////////////////////////////

NS_IMETHODIMP nsNntpUrl::GetNewsAction(nsNewsAction *aNewsAction)
{
	if (aNewsAction)
		*aNewsAction = m_newsAction;
	return NS_OK;
}


NS_IMETHODIMP nsNntpUrl::SetNewsAction(nsNewsAction aNewsAction)
{
	m_newsAction = aNewsAction;
	return NS_OK;
}

nsresult nsNntpUrl::SetNntpHost (nsINNTPHost * newsHost)
{
	NS_LOCK_INSTANCE();
	if (newsHost)
	{
		NS_IF_RELEASE(m_newsHost);
		m_newsHost = newsHost;
		NS_ADDREF(m_newsHost);
	}
	NS_UNLOCK_INSTANCE();
	return NS_OK;
}

nsresult nsNntpUrl::GetNntpHost (nsINNTPHost ** newsHost)
{
    NS_LOCK_INSTANCE();
	if (newsHost)
	{
		*newsHost = m_newsHost;
		NS_IF_ADDREF(m_newsHost);
	}
    NS_UNLOCK_INSTANCE();
    return NS_OK;
}

nsresult nsNntpUrl::SetNntpArticleList (nsINNTPArticleList * articleList)
{
	NS_LOCK_INSTANCE();
	if (articleList)
	{
		NS_IF_RELEASE(m_articleList);
		m_articleList = articleList;
		NS_ADDREF(m_articleList);
	}
	NS_UNLOCK_INSTANCE();
	return NS_OK;
}

nsresult nsNntpUrl::GetNntpArticleList (nsINNTPArticleList ** articleList)
{
	NS_LOCK_INSTANCE();
	if (articleList)
	{
		*articleList = m_articleList;
		NS_IF_ADDREF(m_articleList);
	}
    NS_UNLOCK_INSTANCE();
    return NS_OK;
}

nsresult nsNntpUrl::SetNewsgroup (nsINNTPNewsgroup * newsgroup)
{
	NS_LOCK_INSTANCE();
	if (newsgroup)
	{
		NS_IF_RELEASE(m_newsgroup);
		m_newsgroup = newsgroup;
		NS_ADDREF(m_newsgroup);
	}
	NS_UNLOCK_INSTANCE();
	return NS_OK;
}

nsresult nsNntpUrl::GetNewsgroup (nsINNTPNewsgroup ** newsgroup)
{
	NS_LOCK_INSTANCE();
	if (newsgroup)
	{
		*newsgroup = m_newsgroup;
		NS_IF_ADDREF(m_newsgroup);
	}
    NS_UNLOCK_INSTANCE();
    return NS_OK;
}

nsresult nsNntpUrl::SetOfflineNewsState (nsIMsgOfflineNewsState * offlineNews)
{
	NS_LOCK_INSTANCE();
	if (offlineNews)
	{
		NS_IF_RELEASE(m_offlineNews);
		m_offlineNews = offlineNews;
		NS_ADDREF(m_offlineNews);
	}
	NS_UNLOCK_INSTANCE();
	return NS_OK;
}

nsresult nsNntpUrl::GetOfflineNewsState (nsIMsgOfflineNewsState ** offlineNews) 
{
	NS_LOCK_INSTANCE();
	if (offlineNews)
	{
		*offlineNews = m_offlineNews;
		NS_IF_ADDREF(m_offlineNews);
	}
    NS_UNLOCK_INSTANCE();
    return NS_OK;
}

nsresult nsNntpUrl::SetNewsgroupList (nsINNTPNewsgroupList * newsgroupList)
{
	NS_LOCK_INSTANCE();
	if (newsgroupList)
	{
		NS_IF_RELEASE(m_newsgroupList);
		m_newsgroupList = newsgroupList;
		NS_IF_ADDREF(m_newsgroupList);
	}
	NS_UNLOCK_INSTANCE();
	return NS_OK;
}

nsresult nsNntpUrl::GetNewsgroupList (nsINNTPNewsgroupList ** newsgroupList) 
{
	NS_LOCK_INSTANCE();
	if (newsgroupList)
	{
		*newsgroupList = m_newsgroupList;
		NS_IF_ADDREF(m_newsgroupList);
	}
    NS_UNLOCK_INSTANCE();
    return NS_OK;
}

// from nsIMsgMessageUrl
NS_IMETHODIMP nsNntpUrl::GetURI(char ** aURI)
{	
	nsresult rv;
	if (aURI)
	{
		nsXPIDLCString spec;
		GetSpec(getter_Copies(spec));
		char * uri = nsnull;
		rv = nsBuildNewsMessageURI(spec, m_messageKey, &uri);
		if (NS_FAILED(rv)) return rv;
		*aURI = uri;
		return NS_OK;
	}
	else {
		return NS_ERROR_NULL_POINTER;
	}
}

NS_IMPL_GETSET(nsNntpUrl, AddDummyEnvelope, PRBool, m_addDummyEnvelope);
NS_IMPL_GETSET(nsNntpUrl, CanonicalLineEnding, PRBool, m_canonicalLineEnding);

NS_IMETHODIMP nsNntpUrl::SetMessageFile(nsIFileSpec * aFileSpec)
{
	m_messageFileSpec = dont_QueryInterface(aFileSpec);
	return NS_OK;
}

NS_IMETHODIMP nsNntpUrl::GetMessageFile(nsIFileSpec ** aFileSpec)
{
	if (aFileSpec)
	{
		*aFileSpec = m_messageFileSpec;
		NS_IF_ADDREF(*aFileSpec);
	}
	return NS_OK;
}


NS_IMETHODIMP nsNntpUrl::SetUsername(const char *aUserName)
{
	nsresult rv = NS_OK;
	if (aUserName)
		m_userName = aUserName;
	else
		rv = NS_ERROR_NULL_POINTER;

	return rv;
}

////////////////////////////////////////////////////////////////////////////////////
// End nsINntpUrl specific support
////////////////////////////////////////////////////////////////////////////////////

nsresult nsNntpUrl::SetMessageToPost(nsINNTPNewsgroupPost *post)
{
    NS_LOCK_INSTANCE();
    NS_IF_RELEASE(m_newsgroupPost);
    m_newsgroupPost=post;
    if (m_newsgroupPost) NS_ADDREF(m_newsgroupPost);
    NS_UNLOCK_INSTANCE();
    return NS_OK;
}

nsresult nsNntpUrl::GetMessageToPost(nsINNTPNewsgroupPost **aPost)
{
    NS_LOCK_INSTANCE();
    if (!aPost) return NS_ERROR_NULL_POINTER;
    *aPost = m_newsgroupPost;
    if (*aPost) NS_ADDREF(*aPost);
    NS_UNLOCK_INSTANCE();
    return NS_OK;
}

NS_IMETHODIMP nsNntpUrl::GetMessageHeader(nsIMsgDBHdr ** aMsgHdr)
{
    nsresult rv = NS_OK;
    nsFileSpec pathResult;
    
    if (!aMsgHdr) return NS_ERROR_NULL_POINTER;

    if (!m_newsgroupName) return NS_ERROR_FAILURE;

    nsXPIDLCString hostName;
    rv = GetHost(getter_Copies(hostName));
    if (NS_FAILED(rv)) return rv;

    nsXPIDLCString userName;
    rv = GetUsername(getter_Copies(userName));
    if (NS_FAILED(rv)) return rv; 

    nsCString newsgroupURI(kNewsMessageRootURI);
    newsgroupURI.Append("/");
    if (userName && (userName != (const char *)"")) {
	newsgroupURI.Append(userName);
	newsgroupURI.Append("@");
    }
    newsgroupURI.Append(hostName);
    newsgroupURI.Append("/");
    newsgroupURI.Append(m_newsgroupName);
    
    rv = nsNewsURI2Path(kNewsMessageRootURI, newsgroupURI.GetBuffer(), pathResult);
    if (NS_FAILED(rv)) {
        return rv;
    }
    
    nsCOMPtr<nsIMsgDatabase> newsDBFactory;
    nsCOMPtr<nsIMsgDatabase> newsDB;
    
    rv = nsComponentManager::CreateInstance(kCNewsDB, nsnull, nsIMsgDatabase::GetIID(), getter_AddRefs(newsDBFactory));
    if (NS_FAILED(rv) || (!newsDBFactory)) {
        return rv;
    }
    
	nsCOMPtr <nsIFileSpec> dbFileSpec;
	NS_NewFileSpecWithSpec(pathResult, getter_AddRefs(dbFileSpec));
    rv = newsDBFactory->Open(dbFileSpec, PR_TRUE, PR_FALSE, getter_AddRefs(newsDB));
    
    if (NS_FAILED(rv) || (!newsDB)) {
        return rv;
    }
    
    rv = newsDB->GetMsgHdrForKey(m_messageKey, aMsgHdr);
    if (NS_FAILED(rv) || (!aMsgHdr)) {
        return rv;
    }
  
	return NS_OK;
}

NS_IMETHODIMP nsNntpUrl::SetNewsgroupName(const char * aNewsgroupName)
{
    if (!aNewsgroupName) return NS_ERROR_NULL_POINTER;

    PR_FREEIF(m_newsgroupName);
    m_newsgroupName = nsnull;
    
    m_newsgroupName = PL_strdup(aNewsgroupName);
    if (!m_newsgroupName) {
        return NS_ERROR_OUT_OF_MEMORY;
    }
    else {
        return NS_OK;
    }    
}

NS_IMETHODIMP nsNntpUrl::GetNewsgroupName(char ** aNewsgroupName)
{
    if (!*aNewsgroupName) return NS_ERROR_NULL_POINTER;

    NS_ASSERTION(m_newsgroupName, "null ptr");
    if (!m_newsgroupName) return NS_ERROR_FAILURE;

    *aNewsgroupName = PL_strdup(m_newsgroupName);
    if (!aNewsgroupName) {
        return NS_ERROR_OUT_OF_MEMORY;
    }
    else {
        return NS_OK;
    }
}     

NS_IMETHODIMP nsNntpUrl::SetMessageKey(nsMsgKey aKey)
{
    m_messageKey = aKey;
    return NS_OK;
}

NS_IMETHODIMP nsNntpUrl::GetMessageKey(nsMsgKey * aKey)
{
    *aKey = m_messageKey;
    return NS_OK;
}
