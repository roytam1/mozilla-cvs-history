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

#include "msgCore.h"    // precompiled header...
#include "nsXPIDLString.h"
#include "nsIPref.h"
#include "nsIIOService.h"

#include "nsSmtpService.h"
#include "nsIMsgMailSession.h"
#include "nsMsgBaseCID.h"
#include "nsMsgCompCID.h"

#include "nsSmtpUrl.h"
#include "nsSmtpProtocol.h"
#include "nsIFileSpec.h"
#include "nsCOMPtr.h"
#include "nsIMsgIdentity.h"
#include "nsINetPrompt.h"
#include "nsMsgComposeStringBundle.h"

typedef struct _findServerByKeyEntry {
    const char *key;
    nsISmtpServer *server;
} findServerByKeyEntry;

typedef struct _findServerByHostnameEntry {
    const char *hostname;
    nsISmtpServer *server;
} findServerByHostnameEntry;

static NS_DEFINE_CID(kCSmtpUrlCID, NS_SMTPURL_CID);
static NS_DEFINE_CID(kCMailtoUrlCID, NS_MAILTOURL_CID);
static NS_DEFINE_CID(kSmtpServiceCID, NS_SMTPSERVICE_CID); 
static NS_DEFINE_CID(kIOServiceCID, NS_IOSERVICE_CID); 

// foward declarations...
nsresult
NS_MsgBuildSmtpUrl(nsIFileSpec * aFilePath,
                   const char* aSmtpHostName, 
                   const char* aSmtpUserName, 
                   const char* aRecipients, 
                   nsIMsgIdentity * aSenderIdentity,
                   nsIUrlListener * aUrlListener,
                   nsINetPrompt * aNetPrompt,
                   nsIURI ** aUrl);

nsresult NS_MsgLoadSmtpUrl(nsIURI * aUrl, nsISupports * aConsumer);

nsSmtpService::nsSmtpService() :
    mSmtpServersLoaded(PR_FALSE)
{
    NS_INIT_REFCNT();
    NS_NewISupportsArray(getter_AddRefs(mSmtpServers));
}

nsSmtpService::~nsSmtpService()
{
    // save the SMTP servers to disk

}

NS_IMPL_ISUPPORTS2(nsSmtpService, nsISmtpService, nsIProtocolHandler);


nsresult nsSmtpService::SendMailMessage(nsIFileSpec * aFilePath,
                                        const char * aRecipients, 
                                        nsIMsgIdentity * aSenderIdentity,
                                        nsIUrlListener * aUrlListener, 
                                        nsISmtpServer * aServer,
                                        nsINetPrompt * aNetPrompt,
                                        nsIURI ** aURL)
{
	nsIURI * urlToRun = nsnull;
	nsresult rv = NS_OK;

	NS_WITH_SERVICE(nsISmtpService, smtpService, kSmtpServiceCID, &rv); 
	if (NS_SUCCEEDED(rv) && smtpService)
	{
        nsCOMPtr<nsISmtpServer> smtpServer;

        // first try the identity's preferred server
        if (aSenderIdentity)
            rv = aSenderIdentity->GetSmtpServer(getter_AddRefs(smtpServer));

        // fallback to the default
        if (NS_FAILED(rv) || !smtpServer)
            rv = smtpService->GetDefaultServer(getter_AddRefs(smtpServer));

		if (NS_SUCCEEDED(rv) && smtpServer)
		{
      nsXPIDLCString smtpHostName;
      nsXPIDLCString smtpUserName;

			smtpServer->GetHostname(getter_Copies(smtpHostName));
			smtpServer->GetUsername(getter_Copies(smtpUserName));

      if ((const char*)smtpHostName) 
			{
        rv = NS_MsgBuildSmtpUrl(aFilePath, smtpHostName, smtpUserName,
                                aRecipients, aSenderIdentity, aUrlListener,
                                aNetPrompt, &urlToRun); // this ref counts urlToRun
        if (NS_SUCCEEDED(rv) && urlToRun)	
        {
            nsCOMPtr<nsISmtpUrl> smtpUrl = do_QueryInterface(urlToRun, &rv);
            if (NS_SUCCEEDED(rv))
                smtpUrl->SetSmtpServer(smtpServer);
            rv = NS_MsgLoadSmtpUrl(urlToRun, nsnull);
        }

        if (aURL) // does the caller want a handle on the url?
          *aURL = urlToRun; // transfer our ref count to the caller....
        else
          NS_IF_RELEASE(urlToRun);
      }
      else
        rv = NS_ERROR_COULD_NOT_LOGIN_TO_SMTP_SERVER;
		}
	} // if we had a mail session

	return rv;
}


// The following are two convience functions I'm using to help expedite building and running a mail to url...

// short cut function for creating a mailto url...
nsresult NS_MsgBuildSmtpUrl(nsIFileSpec * aFilePath,
                            const char* aSmtpHostName, 
                            const char* aSmtpUserName, 
                            const char * aRecipients, 
                            nsIMsgIdentity * aSenderIdentity,
                            nsIUrlListener * aUrlListener, 
                            nsINetPrompt * aNetPrompt,
                            nsIURI ** aUrl)
{
	// mscott: this function is a convience hack until netlib actually dispatches smtp urls.
	// in addition until we have a session to get a password, host and other stuff from, we need to use default values....
	// ..for testing purposes....
	
	nsresult rv = NS_OK;
	nsCOMPtr <nsISmtpUrl> smtpUrl;

	rv = nsComponentManager::CreateInstance(kCSmtpUrlCID, NULL, NS_GET_IID(nsISmtpUrl), getter_AddRefs(smtpUrl));

	if (NS_SUCCEEDED(rv) && smtpUrl)
	{
		// this is complicated because the smtp username can be null
		char * urlSpec= PR_smprintf("smtp://%s%s%s:%d/%s",
					((const char*)aSmtpUserName)?(const char*)aSmtpUserName:"",
					((const char*)aSmtpUserName)?"@":"",
                                    	(const char*)aSmtpHostName, 
					SMTP_PORT, aRecipients ? aRecipients : "");
		if (urlSpec)
		{
			nsCOMPtr<nsIMsgMailNewsUrl> url = do_QueryInterface(smtpUrl);
			url->SetSpec(urlSpec);
			smtpUrl->SetPostMessageFile(aFilePath);
			smtpUrl->SetSenderIdentity(aSenderIdentity);
            smtpUrl->SetNetPrompt(aNetPrompt);
			url->RegisterListener(aUrlListener);
			PR_Free(urlSpec);
		}
		rv = smtpUrl->QueryInterface(NS_GET_IID(nsIURI), (void **) aUrl);
	 }

	 return rv;
}

nsresult NS_MsgLoadSmtpUrl(nsIURI * aUrl, nsISupports * aConsumer)
{
	// mscott: this function is pretty clumsy right now...eventually all of the dispatching
	// and transport creation code will live in netlib..this whole function is just a hack
	// for our mail news demo....

	// for now, assume the url is a news url and load it....
	nsCOMPtr <nsISmtpUrl> smtpUrl;
	nsSmtpProtocol	*smtpProtocol = nsnull;
	nsresult rv = NS_OK;

	if (!aUrl)
		return rv;

    // turn the url into an smtp url...
	smtpUrl = do_QueryInterface(aUrl);
    if (smtpUrl)
    {
		// almost there...now create a nntp protocol instance to run the url in...
		smtpProtocol = new nsSmtpProtocol(aUrl);
		if (smtpProtocol == nsnull)
			rv = NS_ERROR_OUT_OF_MEMORY;
		else
			rv = smtpProtocol->LoadUrl(aUrl, aConsumer); // protocol will get destroyed when url is completed...
	}

	return rv;
}

NS_IMETHODIMP nsSmtpService::GetScheme(char * *aScheme)
{
	nsresult rv = NS_OK;
	if (aScheme)
		*aScheme = PL_strdup("mailto");
	else
		rv = NS_ERROR_NULL_POINTER;
	return rv; 
}

NS_IMETHODIMP nsSmtpService::GetDefaultPort(PRInt32 *aDefaultPort)
{
	nsresult rv = NS_OK;
	if (aDefaultPort)
		*aDefaultPort = SMTP_PORT;
	else
		rv = NS_ERROR_NULL_POINTER;
	return rv; 	
}

NS_IMETHODIMP nsSmtpService::GetUritype(PRInt16 *result)
{
    *result = url_nonhierachical + url_norelative + url_noauth;
    return NS_OK;
}

//////////////////////////////////////////////////////////////////////////
// This is just a little stub channel class for mailto urls. Mailto urls
// don't really have any data for the stream calls in nsIChannel to make much sense.
// But we need to have a channel to return for nsSmtpService::NewChannel
// that can simulate a real channel such that the uri loader can then get the
// content type for the channel.
class nsMailtoChannel : public nsIChannel
{
public:

	  NS_DECL_ISUPPORTS
    NS_DECL_NSICHANNEL
    NS_DECL_NSIREQUEST
	
    nsMailtoChannel(nsIURI * aURI);
	  virtual ~nsMailtoChannel();

protected:
  nsCOMPtr<nsIURI> m_url;
};

nsMailtoChannel::nsMailtoChannel(nsIURI * aURI)
{
  m_url = aURI;
  NS_INIT_ISUPPORTS();
}

nsMailtoChannel::~nsMailtoChannel()
{}

NS_IMPL_ISUPPORTS2(nsMailtoChannel, nsIChannel, nsIRequest);

NS_IMETHODIMP nsMailtoChannel::GetLoadGroup(nsILoadGroup * *aLoadGroup)
{
    *aLoadGroup = nsnull;
    return NS_OK;
}

NS_IMETHODIMP nsMailtoChannel::SetLoadGroup(nsILoadGroup * aLoadGroup)
{
	return NS_OK;
}

NS_IMETHODIMP nsMailtoChannel::GetNotificationCallbacks(nsIInterfaceRequestor* *aNotificationCallbacks)
{
	return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsMailtoChannel::SetNotificationCallbacks(nsIInterfaceRequestor* aNotificationCallbacks)
{
	return NS_OK;       // don't fail when trying to set this
}


NS_IMETHODIMP nsMailtoChannel::GetOriginalURI(nsIURI * *aURI)
{
    *aURI = nsnull;
    return NS_OK; 
}
 
NS_IMETHODIMP nsMailtoChannel::GetURI(nsIURI * *aURI)
{
    *aURI = m_url;
    NS_IF_ADDREF(*aURI);
    return NS_OK; 
}
 
NS_IMETHODIMP nsMailtoChannel::OpenInputStream(PRUint32 startPosition, PRInt32 readCount, nsIInputStream **_retval)
{
	return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsMailtoChannel::OpenOutputStream(PRUint32 startPosition, nsIOutputStream **_retval)
{
	return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsMailtoChannel::AsyncOpen(nsIStreamObserver *observer, nsISupports* ctxt)
{
  // we already know the content type...
  observer->OnStartRequest(this, ctxt);
  return NS_OK;
}

NS_IMETHODIMP nsMailtoChannel::AsyncRead(PRUint32 startPosition, PRInt32 readCount, nsISupports *ctxt, nsIStreamListener *listener)
{
  return listener->OnStartRequest(this, ctxt);
}

NS_IMETHODIMP nsMailtoChannel::AsyncWrite(nsIInputStream *fromStream, PRUint32 startPosition, PRInt32 writeCount, nsISupports *ctxt, nsIStreamObserver *observer)
{
	return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsMailtoChannel::GetLoadAttributes(nsLoadFlags *aLoadAttributes)
{
	return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsMailtoChannel::SetLoadAttributes(nsLoadFlags aLoadAttributes)
{
	return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsMailtoChannel::GetContentType(char * *aContentType)
{
	*aContentType = nsCRT::strdup("x-application-mailto");
	return NS_OK;
}

NS_IMETHODIMP nsMailtoChannel::SetContentType(const char *aContentType)
{
    // Do not allow the content type to change...
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP nsMailtoChannel::GetContentLength(PRInt32 * aContentLength)
{
  *aContentLength = -1;
  return NS_OK;
}

NS_IMETHODIMP nsMailtoChannel::GetOwner(nsISupports * *aPrincipal)
{
	return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsMailtoChannel::SetOwner(nsISupports * aPrincipal)
{
	return NS_ERROR_NOT_IMPLEMENTED;
}

////////////////////////////////////////////////////////////////////////////////
// From nsIRequest
////////////////////////////////////////////////////////////////////////////////

NS_IMETHODIMP nsMailtoChannel::IsPending(PRBool *result)
{
    *result = PR_TRUE;
    return NS_OK; 
}

NS_IMETHODIMP nsMailtoChannel::Cancel()
{
    return NS_OK;
}

NS_IMETHODIMP nsMailtoChannel::Suspend()
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsMailtoChannel::Resume()
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


// the smtp service is also the protocol handler for mailto urls....

NS_IMETHODIMP nsSmtpService::NewURI(const char *aSpec, nsIURI *aBaseURI, nsIURI **_retval)
{
  // get a new smtp url 

  nsresult rv = NS_OK;
	nsCOMPtr <nsIURI> mailtoUrl;

	rv = nsComponentManager::CreateInstance(kCMailtoUrlCID, NULL, NS_GET_IID(nsIURI), getter_AddRefs(mailtoUrl));

	if (NS_SUCCEEDED(rv))
	{
    mailtoUrl->SetSpec(aSpec);
		rv = mailtoUrl->QueryInterface(NS_GET_IID(nsIURI), (void **) _retval);
	}
  return rv;
}

NS_IMETHODIMP nsSmtpService::NewChannel(const char *verb, 
                                        nsIURI *aURI, 
                                        nsILoadGroup* aLoadGroup,
                                        nsIInterfaceRequestor* notificationCallbacks,
                                        nsLoadFlags loadAttributes,
                                        nsIURI* originalURI,
                                        PRUint32 bufferSegmentSize,
                                        PRUint32 bufferMaxSize,
                                        nsIChannel **_retval)
{
  nsresult rv = NS_OK;
  nsMailtoChannel * aMailtoChannel = new nsMailtoChannel(aURI);
  if (aMailtoChannel)
  {
      rv = aMailtoChannel->QueryInterface(NS_GET_IID(nsIChannel), (void **) _retval);
  }
  else
    rv = NS_ERROR_OUT_OF_MEMORY;
  return rv;
}


NS_IMETHODIMP
nsSmtpService::GetSmtpServers(nsISupportsArray ** aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);

  nsresult rv;
  
  // now read in the servers from prefs if necessary
  PRUint32 serverCount;
  rv = mSmtpServers->Count(&serverCount);
  if (NS_FAILED(rv)) return rv;

  if (serverCount<=0) loadSmtpServers();

  *aResult = mSmtpServers;
  NS_ADDREF(*aResult);

  return NS_OK;
}

nsresult
nsSmtpService::loadSmtpServers()
{
    if (mSmtpServersLoaded) return NS_OK;
    
    nsresult rv;
    NS_WITH_SERVICE(nsIPref, prefs, NS_PREF_PROGID, &rv);
    if (NS_FAILED(rv)) return rv;
    
    nsXPIDLCString serverList;
    rv = prefs->CopyCharPref("mail.smtpservers", getter_Copies(serverList));
    if (NS_FAILED(rv)) return rv;

    char *newStr;
    char *pref = nsCRT::strtok(NS_CONST_CAST(char*,(const char*)serverList),
                               ", ", &newStr);
    while (pref) {

        rv = createKeyedServer(pref);
        
        pref = nsCRT::strtok(newStr, ", ", &newStr);
    }

    saveKeyList();

    mSmtpServersLoaded = PR_TRUE;
    return NS_OK;
}

// save the list of keys
nsresult
nsSmtpService::saveKeyList()
{
    nsresult rv;
    NS_WITH_SERVICE(nsIPref, prefs, NS_PREF_PROGID, &rv);
    if (NS_FAILED(rv)) return rv;
    
    return prefs->SetCharPref("mail.smtpservers", mServerKeyList);
}

nsresult
nsSmtpService::createKeyedServer(const char *key, nsISmtpServer** aResult)
{
    if (!key) return NS_ERROR_NULL_POINTER;
    
    nsCOMPtr<nsISmtpServer> server;
    
    nsresult rv;
    rv = nsComponentManager::CreateInstance(NS_SMTPSERVER_PROGID,
                                            nsnull,
                                            NS_GET_IID(nsISmtpServer),
                                            (void **)getter_AddRefs(server));
    if (NS_FAILED(rv)) return rv;
    
    server->SetKey(NS_CONST_CAST(char *,key));
    mSmtpServers->AppendElement(server);

    NS_WITH_SERVICE(nsIPref, prefs, NS_PREF_PROGID, &rv);
    if (NS_SUCCEEDED(rv)) {
        if (mServerKeyList.IsEmpty())
            mServerKeyList = key;
        else {
            mServerKeyList += ",";
            mServerKeyList += key;
        }
    }

    if (aResult) {
        *aResult = server;
        NS_IF_ADDREF(*aResult);
    }
    return NS_OK;
}

NS_IMETHODIMP
nsSmtpService::GetDefaultServer(nsISmtpServer **aServer)
{
  NS_ENSURE_ARG_POINTER(aServer);

  nsresult rv;

  *aServer = nsnull;
  // always returns NS_OK, just leaving *aServer at nsnull
  if (!mDefaultSmtpServer) {
      NS_WITH_SERVICE(nsIPref, pref, NS_PREF_PROGID, &rv);
      if (NS_FAILED(rv)) return rv;

      // try to get it from the prefs
      nsXPIDLCString defaultServerKey;
      rv = pref->CopyCharPref("mail.smtp.defaultserver",
                             getter_Copies(defaultServerKey));
      if (NS_SUCCEEDED(rv) &&
          nsCRT::strcmp(defaultServerKey, "")==0) {

          nsCOMPtr<nsISmtpServer> server;
          rv = GetServerByKey(defaultServerKey,
                              getter_AddRefs(mDefaultSmtpServer));
      } else {
          // no pref set, so just return the first one, and set the pref
      
          PRUint32 count=0;
          nsCOMPtr<nsISupportsArray> smtpServers;
          rv = GetSmtpServers(getter_AddRefs(smtpServers));
          rv = smtpServers->Count(&count);

          // nothing in the array, we had better create a new server
          // (which will add it to the array & prefs anyway)
          if (count == 0)
              rv = CreateSmtpServer(getter_AddRefs(mDefaultSmtpServer));
          else
              rv = mSmtpServers->QueryElementAt(0, NS_GET_IID(nsISmtpServer),
                                                (void **)getter_AddRefs(mDefaultSmtpServer));

          if (NS_FAILED(rv)) return rv;
          NS_ENSURE_TRUE(mDefaultSmtpServer, NS_ERROR_UNEXPECTED);
          
          // now we have a default server, set the prefs correctly
          nsXPIDLCString serverKey;
          mDefaultSmtpServer->GetKey(getter_Copies(serverKey));
          if (NS_SUCCEEDED(rv))
              pref->SetCharPref("mail.smtp.defaultserver", serverKey);
      }
  }

  // at this point:
  // * mDefaultSmtpServer has a valid server
  // * the key has been set in the prefs
    
  *aServer = mDefaultSmtpServer;
  NS_IF_ADDREF(*aServer);

  return NS_OK;
}

NS_IMETHODIMP
nsSmtpService::SetDefaultServer(nsISmtpServer *aServer)
{
    nsresult rv;
    mDefaultSmtpServer = aServer;

    nsXPIDLCString serverKey;
    rv = aServer->GetKey(getter_Copies(serverKey));
    if (NS_FAILED(rv)) return rv;
    
    NS_WITH_SERVICE(nsIPref, pref, NS_PREF_PROGID, &rv);
    pref->SetCharPref("mail.smtp.defaultserver", serverKey);
    return NS_OK;
}

PRBool
nsSmtpService::findServerByKey (nsISupports *element, void *aData)
{
    nsresult rv;
    nsCOMPtr<nsISmtpServer> server = do_QueryInterface(element, &rv);
    if (NS_FAILED(rv)) return PR_TRUE;
    
    findServerByKeyEntry *entry = (findServerByKeyEntry*) aData;

    nsXPIDLCString key;
    rv = server->GetKey(getter_Copies(key));
    if (NS_FAILED(rv)) return PR_TRUE;

    if (nsCRT::strcmp(key, entry->key)==0) {
        entry->server = server;
        return PR_FALSE;
    }
    
    return PR_TRUE;
}

NS_IMETHODIMP
nsSmtpService::CreateSmtpServer(nsISmtpServer **aResult)
{
    if (!aResult) return NS_ERROR_NULL_POINTER;

    loadSmtpServers();
    nsresult rv;
    
    PRInt32 i=0;
    PRBool unique = PR_FALSE;

    findServerByKeyEntry entry;
    nsCAutoString key;
    
    do {
        key = "smtp";
        key.Append(++i);
        
        entry.key = key;
        entry.server = nsnull;

        mSmtpServers->EnumerateForwards(findServerByKey, (void *)&entry);
        if (!entry.server) unique=PR_TRUE;
        
    } while (!unique);

    rv = createKeyedServer(key, aResult);
    saveKeyList();
    return rv;
}


nsresult
nsSmtpService::GetServerByKey(const char* aKey, nsISmtpServer **aResult)
{
    findServerByKeyEntry entry;
    entry.key = aKey;
    entry.server = nsnull;
    mSmtpServers->EnumerateForwards(findServerByKey, (void *)&entry);

    if (entry.server) {
        (*aResult) = entry.server;
        NS_ADDREF(*aResult);
        return NS_OK;
    }

    // not found in array, I guess we load it
    return createKeyedServer(aKey, aResult);
}

NS_IMETHODIMP
nsSmtpService::DeleteSmtpServer(nsISmtpServer *aServer)
{
    if (!aServer) return NS_OK;

    nsresult rv;

    PRInt32 idx = 0;
    rv = mSmtpServers->GetIndexOf(aServer, &idx);
    if (NS_FAILED(rv) || idx==0)
        return NS_OK;

    nsXPIDLCString serverKey;
    aServer->GetKey(getter_Copies(serverKey));
    
    rv = mSmtpServers->DeleteElementAt(idx);

    nsCAutoString newServerList;
    char *newStr;
    char *rest = mServerKeyList.ToNewCString();
    
    char *token = nsCRT::strtok(rest, ",", &newStr);
    while (token) {
        // only re-add the string if it's not the key
        if (nsCRT::strcmp(token, serverKey) != 0) {
            if (newServerList.IsEmpty())
                newServerList = token;
            else {
                newServerList += ',';
                newServerList += token;
            }
        }

        token = nsCRT::strtok(newStr, ",", &newStr);
    }

    mServerKeyList = newServerList;
    saveKeyList();
    return rv;
}

PRBool
nsSmtpService::findServerByHostname(nsISupports *element, void *aData)
{
    nsresult rv;
    
    nsCOMPtr<nsISmtpServer> server = do_QueryInterface(element, &rv);
    if (NS_FAILED(rv)) return PR_TRUE;

    findServerByHostnameEntry *entry = (findServerByHostnameEntry*)aData;

    nsXPIDLCString hostname;
    rv = server->GetHostname(getter_Copies(hostname));
    if (NS_FAILED(rv)) return PR_TRUE;

    if (((const char*)hostname) &&
        nsCRT::strcmp(hostname, entry->hostname) == 0) {
        entry->server = server;
        return PR_FALSE;        // stop when found
    }
    return PR_TRUE;
}

NS_IMETHODIMP
nsSmtpService::FindServer(const char *hostname, nsISmtpServer ** aResult)
{
    if (!aResult) return NS_ERROR_NULL_POINTER;

    findServerByHostnameEntry entry;
    entry.server=nsnull;
    entry.hostname = hostname;

    mSmtpServers->EnumerateForwards(findServerByHostname, (void *)&entry);

    // entry.server may be null, but that's ok.
    // just return null if no server is found
    *aResult = entry.server;
    NS_IF_ADDREF(*aResult);
    
    return NS_OK;
}
