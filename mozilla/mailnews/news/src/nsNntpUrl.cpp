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
 *   Pierre Phaneuf <pp@ludusdesign.com>
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
#include "nsMsgNewsCID.h"
#include "nsIRDFService.h"
#include "rdf.h"
#include "nsIMsgFolder.h"
#include "nsINntpService.h"
#include "nsIMsgMessageService.h"

static NS_DEFINE_CID(kCNewsDB, NS_NEWSDB_CID);
    
nsNntpUrl::nsNntpUrl()
{
  m_offlineNews = nsnull;
  m_newsgroupPost = nsnull;
  m_newsAction = nsINntpUrl::ActionGetNewNews;
  m_addDummyEnvelope = PR_FALSE;
  m_canonicalLineEnding = PR_FALSE;
  m_filePath = nsnull;
  m_getOldMessages = PR_FALSE;
}
         
nsNntpUrl::~nsNntpUrl()
{
  NS_IF_RELEASE(m_offlineNews);
  NS_IF_RELEASE(m_newsgroupPost);
}
  
NS_IMPL_ADDREF_INHERITED(nsNntpUrl, nsMsgMailNewsUrl)
NS_IMPL_RELEASE_INHERITED(nsNntpUrl, nsMsgMailNewsUrl)

NS_INTERFACE_MAP_BEGIN(nsNntpUrl)
   NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsINntpUrl)
   NS_INTERFACE_MAP_ENTRY(nsINntpUrl)
   NS_INTERFACE_MAP_ENTRY(nsIMsgMessageUrl)
   NS_INTERFACE_MAP_ENTRY(nsIMsgI18NUrl)
NS_INTERFACE_MAP_END_INHERITING(nsMsgMailNewsUrl)

////////////////////////////////////////////////////////////////////////////////////
// Begin nsINntpUrl specific support
////////////////////////////////////////////////////////////////////////////////////

NS_IMETHODIMP nsNntpUrl::SetGetOldMessages(PRBool aGetOldMessages)
{
	m_getOldMessages = aGetOldMessages;
	return NS_OK;
}

NS_IMETHODIMP nsNntpUrl::GetGetOldMessages(PRBool * aGetOldMessages)
{
	NS_ENSURE_ARG(aGetOldMessages);
	*aGetOldMessages = m_getOldMessages;
	return NS_OK;
}

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

NS_IMETHODIMP nsNntpUrl::SetUri(const char * aURI)
{
  mURI = aURI;
  return NS_OK;
}

// from nsIMsgMessageUrl
NS_IMETHODIMP nsNntpUrl::GetUri(char ** aURI)
{
  nsresult rv = NS_OK;

  // if we have been given a uri to associate with this url, then use it
  // otherwise try to reconstruct a URI on the fly....
  if (mURI.IsEmpty()) {
    nsXPIDLCString spec;
    rv = GetSpec(getter_Copies(spec));
    NS_ENSURE_SUCCESS(rv,rv);
    mURI = (const char *)spec;
  }
  
  *aURI = mURI.ToNewCString();
  if (!*aURI) return NS_ERROR_OUT_OF_MEMORY; 
  return rv;
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
  nsresult rv;
  
  nsCOMPtr <nsINntpService> nntpService = do_GetService(NS_NNTPSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv,rv);

  nsCOMPtr <nsIMsgMessageService> msgService = do_QueryInterface(nntpService, &rv);
  NS_ENSURE_SUCCESS(rv,rv);

  if (mOriginalMessageURI.IsEmpty()) {
    // this can happen when viewing a news://host/message-id url
    return NS_ERROR_FAILURE;
  }

  rv = msgService->MessageURIToMsgHdr(mOriginalMessageURI.get(), aMsgHdr);
  NS_ENSURE_SUCCESS(rv,rv);
  
  return NS_OK;
}

NS_IMETHODIMP nsNntpUrl::IsUrlType(PRUint32 type, PRBool *isType)
{
	NS_ENSURE_ARG(isType);

	switch(type)
	{
		case nsIMsgMailNewsUrl::eDisplay:
			*isType = (m_newsAction == nsINntpUrl::ActionDisplayArticle);
			break;
		default:
			*isType = PR_FALSE;
	};				

	return NS_OK;

}

NS_IMETHODIMP
nsNntpUrl::GetOriginalSpec(char **aSpec)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsNntpUrl::SetOriginalSpec(const char *aSpec)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult nsNntpUrl::GetMsgFolder(nsIMsgFolder **msgFolder)
{
   nsresult rv;

   if (mOriginalMessageURI.IsEmpty()) {
    // this could be a autosubscribe url (news://host/group)
    // or a message id url (news://host/message-id)
    // either way, we won't have a msgFolder for you
    return NS_ERROR_UNEXPECTED;
   }

   nsCOMPtr <nsINntpService> nntpService = do_GetService(NS_NNTPSERVICE_CONTRACTID, &rv);
   NS_ENSURE_SUCCESS(rv,rv);

   nsMsgKey msgKey;
   rv = nntpService->DecomposeNewsURI(mOriginalMessageURI.get(), msgFolder, &msgKey);
   NS_ENSURE_SUCCESS(rv,rv);
   return NS_OK;
}

NS_IMETHODIMP 
nsNntpUrl::GetFolderCharset(PRUnichar **aCharacterSet)
{
  nsCOMPtr<nsIMsgFolder> folder;
  nsresult rv = GetMsgFolder(getter_AddRefs(folder));
  NS_ENSURE_SUCCESS(rv,rv);

  NS_ENSURE_TRUE(folder, NS_ERROR_FAILURE);
  rv = folder->GetCharset(aCharacterSet);
  NS_ENSURE_SUCCESS(rv,rv);
  return rv;
}

NS_IMETHODIMP nsNntpUrl::GetFolderCharsetOverride(PRBool * aCharacterSetOverride)
{
  nsCOMPtr<nsIMsgFolder> folder;
  nsresult rv = GetMsgFolder(getter_AddRefs(folder));
  NS_ENSURE_SUCCESS(rv,rv);
  NS_ENSURE_TRUE(folder, NS_ERROR_FAILURE);
  rv = folder->GetCharsetOverride(aCharacterSetOverride);
  NS_ENSURE_SUCCESS(rv,rv);
  return rv;
}

NS_IMETHODIMP nsNntpUrl::GetCharsetOverRide(PRUnichar ** aCharacterSet)
{
  if (!mCharsetOverride.IsEmpty())
    *aCharacterSet = mCharsetOverride.ToNewUnicode(); 
  else
    *aCharacterSet = nsnull;
  return NS_OK;
}

NS_IMETHODIMP nsNntpUrl::SetCharsetOverRide(const PRUnichar * aCharacterSet)
{
  mCharsetOverride = aCharacterSet;
  return NS_OK;
}

NS_IMETHODIMP nsNntpUrl::SetOriginalMessageURI(const char *uri)
{
    mOriginalMessageURI = uri;
    return NS_OK;
}

NS_IMETHODIMP nsNntpUrl::GetOriginalMessageURI(char **uri)
{
    NS_ENSURE_ARG_POINTER(uri);
    *uri = nsCRT::strdup((const char *)mOriginalMessageURI);
    if (!*uri) return NS_ERROR_OUT_OF_MEMORY;
    return NS_OK;
}

