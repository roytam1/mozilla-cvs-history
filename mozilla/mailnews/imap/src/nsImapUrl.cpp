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
 * Copyright (C) 1999 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

#include "msgCore.h"    // precompiled header...

#ifdef XP_PC
#include <windows.h>    // for InterlockedIncrement
#endif

#include "nsIURL.h"
#include "nsImapUrl.h"
#include "nsINetService.h"

#include "nsString.h"
#include "prmem.h"
#include "plstr.h"
#include "prprf.h"
#include "nsCRT.h"

// we need this because of an egcs 1.0 (and possibly gcc) compiler bug
// that doesn't allow you to call ::nsISupports::GetIID() inside of a class
// that multiply inherits from nsISupports
static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);
static NS_DEFINE_CID(kUrlListenerManagerCID, NS_URLLISTENERMANAGER_CID);

nsImapUrl::nsImapUrl()
{
    NS_INIT_REFCNT();

	m_errorMessage = nsnull;
	
	// nsINetLibUrl specific state
    m_URL_s = nsnull;
 
	// nsIURL specific state
    m_protocol = nsnull;
    m_host = nsnull;
    m_port = IMAP_PORT;
    m_spec = nsnull;
    m_search = nsnull;
	
	m_imapLog = nsnull;
	m_runningUrl = PR_FALSE;

	nsComponentManager::CreateInstance(kUrlListenerManagerCID, nsnull, nsIUrlListenerManager::GetIID(), 
									   (void **) &m_urlListeners);
}
 
nsImapUrl::~nsImapUrl()
{
	NS_IF_RELEASE(m_imapLog);
	NS_IF_RELEASE(m_urlListeners);
	PR_FREEIF(m_errorMessage);

    PR_FREEIF(m_spec);
    PR_FREEIF(m_protocol);
    PR_FREEIF(m_host);
    PR_FREEIF(m_search);
}
  
NS_IMPL_THREADSAFE_ADDREF(nsImapUrl);
NS_IMPL_THREADSAFE_RELEASE(nsImapUrl);

NS_IMETHODIMP nsImapUrl::QueryInterface(const nsIID &aIID, void** aInstancePtr)
{
    if (NULL == aInstancePtr) {
        return NS_ERROR_NULL_POINTER;
    }
 
    if (aIID.Equals(nsIImapUrl::GetIID()) || aIID.Equals(kISupportsIID)) 
	{
        *aInstancePtr = (void*) ((nsIImapUrl*)this);
        AddRef();
        return NS_OK;
    }
    if (aIID.Equals(nsIURL::GetIID())) 
	{
        *aInstancePtr = (void*) ((nsIURL*)this);
        AddRef();
        return NS_OK;
    }
    if (aIID.Equals(nsINetlibURL::GetIID())) 
	{
        *aInstancePtr = (void*) ((nsINetlibURL*)this);
        AddRef();
        return NS_OK;
    }
	if (aIID.Equals(nsIMsgMailNewsUrl::GetIID()))
	{
		*aInstancePtr = (void*) ((nsIMsgMailNewsUrl*) this);
		AddRef();
		return NS_OK;
	}

#if defined(NS_DEBUG)
    /*
     * Check for the debug-only interface indicating thread-safety
     */
    static NS_DEFINE_IID(kIsThreadsafeIID, NS_ISTHREADSAFE_IID);
    if (aIID.Equals(kIsThreadsafeIID)) {
        return NS_OK;
    }
#endif
 
    return NS_NOINTERFACE;
}

////////////////////////////////////////////////////////////////////////////////////
// Begin nsIImapUrl specific support
////////////////////////////////////////////////////////////////////////////////////

NS_IMETHODIMP nsImapUrl::GetImapLog(nsIImapLog ** aImapLog)
{
	if (aImapLog)
	{
		*aImapLog = m_imapLog;
		NS_IF_ADDREF(m_imapLog);
	}

	return NS_OK;
}

NS_IMETHODIMP nsImapUrl::SetImapLog(nsIImapLog  * aImapLog)
{
	if (aImapLog)
	{
		NS_IF_RELEASE(m_imapLog);
		m_imapLog = aImapLog;
		NS_ADDREF(m_imapLog);
	}

	return NS_OK;
}


////////////////////////////////////////////////////////////////////////////////////
// End nsIImapUrl specific support
////////////////////////////////////////////////////////////////////////////////////

// url listener registration details...
	
NS_IMETHODIMP nsImapUrl::RegisterListener (nsIUrlListener * aUrlListener)
{
	NS_LOCK_INSTANCE();
	nsresult rv = NS_OK;
	if (m_urlListeners)
		rv = m_urlListeners->RegisterListener(aUrlListener);
    NS_UNLOCK_INSTANCE();
	return rv;
}
	
NS_IMETHODIMP nsImapUrl::UnRegisterListener (nsIUrlListener * aUrlListener)
{
	NS_LOCK_INSTANCE();
	nsresult rv = NS_OK;
	if (m_urlListeners)
		rv = m_urlListeners->UnRegisterListener(aUrlListener);
	NS_UNLOCK_INSTANCE();
	return rv;
}

NS_IMETHODIMP nsImapUrl::GetUrlState(PRBool * aRunningUrl)
{
	NS_LOCK_INSTANCE();
	if (aRunningUrl)
		*aRunningUrl = m_runningUrl;

	NS_UNLOCK_INSTANCE();
	return NS_OK;
}

NS_IMETHODIMP nsImapUrl::SetUrlState(PRBool aRunningUrl, nsresult aExitCode)
{
	NS_LOCK_INSTANCE();
	m_runningUrl = aRunningUrl;
	if (m_urlListeners)
	{
		if (m_runningUrl)
			m_urlListeners->OnStartRunningUrl(this);
		else
			m_urlListeners->OnStopRunningUrl(this, aExitCode);
	}
	NS_UNLOCK_INSTANCE();
	return NS_OK;
}

NS_IMETHODIMP nsImapUrl::SetErrorMessage (char * errorMessage)
{
	NS_LOCK_INSTANCE();
	if (errorMessage)
	{
		PR_FREEIF(m_errorMessage);
		m_errorMessage = errorMessage;
	}
	NS_UNLOCK_INSTANCE();
	return NS_OK;
}

// caller must free using PR_FREE
NS_IMETHODIMP nsImapUrl::GetErrorMessage (char ** errorMessage) const
{
	NS_LOCK_INSTANCE();
	if (errorMessage)
	{
		if (m_errorMessage)
			*errorMessage = nsCRT::strdup(m_errorMessage);
		else
			*errorMessage = nsnull;
	}
    NS_UNLOCK_INSTANCE();
    return NS_OK;
}

////////////////////////////////////////////////////////////////////////////////////
// Begin nsINetlibURL support
////////////////////////////////////////////////////////////////////////////////////

NS_IMETHODIMP nsImapUrl::SetURLInfo(URL_Struct *URL_s)
{
    nsresult result = NS_OK;
    /* Hook us up with the world. */
    m_URL_s = URL_s;
    return result;
}
  
NS_IMETHODIMP nsImapUrl::GetURLInfo(URL_Struct_** aResult) const
{
  nsresult rv;

  if (nsnull == aResult) 
    rv = NS_ERROR_NULL_POINTER;
  else 
  {
    /* XXX: Should the URL be reference counted here?? */
    *aResult = m_URL_s;
    rv = NS_OK;
  }

  return rv;
}

////////////////////////////////////////////////////////////////////////////////////
// End nsINetlibURL support
////////////////////////////////////////////////////////////////////////////////////

nsresult nsImapUrl::ParseURL(const nsString& aSpec, const nsIURL* aURL)
{
    // XXX hack!
    char* cSpec = aSpec.ToNewCString();

    const char* uProtocol = nsnull;
    const char* uHost = nsnull;
    const char* uFile = nsnull;
    PRUint32 uPort;
    if (nsnull != aURL) 
	{
        nsresult rslt = aURL->GetProtocol(&uProtocol);
        if (rslt != NS_OK) return rslt;
        rslt = aURL->GetHost(&uHost);
        if (rslt != NS_OK) return rslt;
        rslt = aURL->GetFile(&uFile);
        if (rslt != NS_OK) return rslt;
        rslt = aURL->GetHostPort(&uPort);
        if (rslt != NS_OK) return rslt;
    }

    NS_LOCK_INSTANCE();

    PR_FREEIF(m_protocol);
    PR_FREEIF(m_host);
    PR_FREEIF(m_search);
    m_port = IMAP_PORT;

	// mscott -> eventually we'll replace all of this duplicate host and port parsing code with a url parser
	// class..this should come with N2 Landing...

    if (nsnull == cSpec) 
	{
        if (nsnull == aURL) 
		{
            NS_UNLOCK_INSTANCE();
            return NS_ERROR_ILLEGAL_VALUE;
        }
        
		m_protocol = (nsnull != uProtocol) ? PL_strdup(uProtocol) : nsnull;
        m_host = (nsnull != uHost) ? PL_strdup(uHost) : nsnull;
        m_port = uPort;

        NS_UNLOCK_INSTANCE();
        return NS_OK;
    }

    // The URL is considered absolute if and only if it begins with a
    // protocol spec. A protocol spec is an alphanumeric string of 1 or
    // more characters that is terminated with a colon.
    PRBool isAbsolute = PR_FALSE;
    char* cp;
    char* ap = cSpec;
    char ch;
    while (0 != (ch = *ap)) 
	{
        if (((ch >= 'a') && (ch <= 'z')) ||
            ((ch >= 'A') && (ch <= 'Z')) ||
            ((ch >= '0') && (ch <= '9'))) 
		{
            ap++;
            continue;
        }
        if ((ch == ':') && (ap - cSpec >= 2)) 
		{
            isAbsolute = PR_TRUE;
            cp = ap;
            break;
        }
        break;
    }

    PRInt32 slen = aSpec.Length();
    m_spec = (char *) PR_Malloc(slen + 1);
    aSpec.ToCString(m_spec, slen+1);

    // get protocol first
    PRInt32 plen = cp - cSpec;
    m_protocol = (char*) PR_Malloc(plen + 1);
    PL_strncpy(m_protocol, cSpec, plen);
    m_protocol[plen] = 0;
    cp++;                               // eat : in protocol
    
	// skip over one, two or three slashes
    if (*cp == '/') 
	{
		cp++;
        if (*cp == '/') 
		{
			cp++;
			if (*cp == '/') 
				cp++;
        }
	} 
	else 
	{
		delete cSpec;
		NS_UNLOCK_INSTANCE();
        return NS_ERROR_ILLEGAL_VALUE;
    }

	// Host name follows protocol for http style urls
	const char* cp0 = cp;
	cp = PL_strpbrk(cp, "/:");
	if (nsnull == cp) 
	{
		// There is only a host name
		PRInt32 hlen = PL_strlen(cp0);
        m_host = (char*) PR_Malloc(hlen + 1);
        PL_strcpy(m_host, cp0);
	}
    else 
	{
		PRInt32 hlen = cp - cp0;
        m_host = (char*) PR_Malloc(hlen + 1);
        PL_strncpy(m_host, cp0, hlen);        
        m_host[hlen] = 0;

		if (':' == *cp) 
		{
			// We have a port number
            cp0 = cp+1;
            cp = PL_strchr(cp, '/');
            m_port = strtol(cp0, (char **)nsnull, 10);
        }
	}

    delete cSpec;

    NS_UNLOCK_INSTANCE();
    return NS_OK;
}

void nsImapUrl::ReconstructSpec(void)
{
    PR_FREEIF(m_spec);

    char portBuffer[10];
    if (-1 != m_port)
        PR_snprintf(portBuffer, 10, ":%d", m_port);
    else
        portBuffer[0] = '\0';

    PRInt32 plen = PL_strlen(m_protocol) + PL_strlen(m_host) +
        PL_strlen(portBuffer) + 4;

    if (m_search)
        plen += 1 + PL_strlen(m_search);

    m_spec = (char *) PR_Malloc(plen + 1);
    PR_snprintf(m_spec, plen, "%s://%s%s", 
                m_protocol, ((nsnull != m_host) ? m_host : ""), portBuffer);

    if (m_search) 
	{
        PL_strcat(m_spec, "?");
        PL_strcat(m_spec, m_search);
    }
}

////////////////////////////////////////////////////////////////////////////////

PRBool nsImapUrl::Equals(const nsIURL* aURL) const 
{
    PRBool bIsEqual;
    nsImapUrl* other;
    NS_LOCK_INSTANCE();
	// are they both imap urls?? if yes...for now just compare the pointers until 
	// I figure out if we need to check any of the guts for equality....
    if (((nsIURL*)aURL)->QueryInterface(nsIImapUrl::GetIID(), (void**)&other) == NS_OK)
        bIsEqual = other == this; // compare the pointers...
    else
        bIsEqual = PR_FALSE;
    NS_UNLOCK_INSTANCE();
    return bIsEqual;
}

NS_IMETHODIMP nsImapUrl::GetProtocol(const char* *result) const
{
    NS_LOCK_INSTANCE();
    *result = m_protocol;
    NS_UNLOCK_INSTANCE();
    return NS_OK;
}

NS_IMETHODIMP nsImapUrl::SetProtocol(const char *aNewProtocol)
{
    NS_LOCK_INSTANCE();
    m_protocol = nsCRT::strdup(aNewProtocol);
    ReconstructSpec();
    NS_UNLOCK_INSTANCE();
    return NS_OK;
}

NS_IMETHODIMP nsImapUrl::GetHost(const char* *result) const
{
    NS_LOCK_INSTANCE();
    *result = m_host;
    NS_UNLOCK_INSTANCE();
    return NS_OK;
}

NS_IMETHODIMP nsImapUrl::SetHost(const char *aNewHost)
{
    NS_ASSERTION(m_URL_s == nsnull, "URL has already been opened");
    NS_LOCK_INSTANCE();
    m_host = nsCRT::strdup(aNewHost);
    ReconstructSpec();
    NS_UNLOCK_INSTANCE();
    return NS_OK;
}

NS_IMETHODIMP nsImapUrl::GetFile(const char* *result) const
{
    NS_LOCK_INSTANCE();
    *result = nsnull;
    NS_UNLOCK_INSTANCE();
    return NS_OK;
}

NS_IMETHODIMP nsImapUrl::SetFile(const char *aNewFile)
{
	// imap doesn't have a file portion to the url yet...
    return NS_OK;
}

NS_IMETHODIMP nsImapUrl::GetSpec(const char* *result) const
{
    NS_LOCK_INSTANCE();
    *result = m_spec;
    NS_UNLOCK_INSTANCE();
    return NS_OK;
}

NS_IMETHODIMP nsImapUrl::SetSpec(const char *aNewSpec)
{
    // XXX is this right, or should we call ParseURL?
    nsresult rv = NS_OK;
    NS_LOCK_INSTANCE();
    rv = ParseURL(aNewSpec);
    NS_UNLOCK_INSTANCE();
    return rv;
}

NS_IMETHODIMP nsImapUrl::GetRef(const char* *result) const
{
    *result = nsnull;
    return NS_OK;
}

NS_IMETHODIMP nsImapUrl::SetRef(const char *aNewRef)
{
    return NS_OK;
}

NS_IMETHODIMP nsImapUrl::GetHostPort(PRUint32 *result) const
{
    NS_LOCK_INSTANCE();
    *result = m_port;
    NS_UNLOCK_INSTANCE();
    return NS_OK;
}

NS_IMETHODIMP nsImapUrl::SetHostPort(PRUint32 aNewPort)
{
    NS_LOCK_INSTANCE();
    m_port = aNewPort;
    ReconstructSpec();
    NS_UNLOCK_INSTANCE();
    return NS_OK;
}

NS_IMETHODIMP nsImapUrl::GetSearch(const char* *result) const
{
    NS_LOCK_INSTANCE();
    *result = m_search;
    NS_UNLOCK_INSTANCE();
    return NS_OK;
}

NS_IMETHODIMP nsImapUrl::SetSearch(const char *aNewSearch)
{
    NS_ASSERTION(m_URL_s == nsnull, "URL has already been opened");
    NS_LOCK_INSTANCE();
    m_search = nsCRT::strdup(aNewSearch);
    ReconstructSpec();
    NS_UNLOCK_INSTANCE();
    return NS_OK;
}

NS_IMETHODIMP nsImapUrl::GetContainer(nsISupports* *result) const
{
    *result = nsnull;
    return NS_OK;
}
  
NS_IMETHODIMP nsImapUrl::SetContainer(nsISupports* container)
{
    return NS_OK;
}

NS_IMETHODIMP nsImapUrl::GetContentLength(PRInt32 *len)
{
    NS_LOCK_INSTANCE();
    *len = m_URL_s->content_length;
    NS_UNLOCK_INSTANCE();
    return NS_OK;
}

////////////////////////////////////////////////////////////////////////////////////
// End of nsIURL support
////////////////////////////////////////////////////////////////////////////////////
 

////////////////////////////////////////////////////////////////////////////////////
// The following set of functions should become obsolete once we take them out of
// nsIURL.....
////////////////////////////////////////////////////////////////////////////////////
NS_IMETHODIMP nsImapUrl::GetLoadAttribs(nsILoadAttribs* *result) const
{
    NS_LOCK_INSTANCE();
    *result = NULL;
    NS_UNLOCK_INSTANCE();
    return NS_OK;
}
  
NS_IMETHODIMP nsImapUrl::SetLoadAttribs(nsILoadAttribs* aLoadAttribs)
{
    NS_ASSERTION(m_URL_s == nsnull, "URL has already been opened");
    return NS_OK;
}

NS_IMETHODIMP nsImapUrl::SetPostHeader(const char* name, const char* value)
{
    NS_LOCK_INSTANCE();
    // XXX
    PR_ASSERT(0);
    NS_UNLOCK_INSTANCE();
    return NS_OK;
}

NS_IMETHODIMP nsImapUrl::SetPostData(nsIInputStream* input)
{
    return NS_OK;
}

NS_IMETHODIMP nsImapUrl::GetURLGroup(nsIURLGroup* *result) const
{
    return NS_OK;
}
  
NS_IMETHODIMP nsImapUrl::SetURLGroup(nsIURLGroup* group)
{
    NS_ASSERTION(m_URL_s == nsnull, "URL has already been opened");
    return NS_OK;
}

NS_IMETHODIMP nsImapUrl::GetServerStatus(PRInt32 *status)
{
    NS_LOCK_INSTANCE();
    *status = m_URL_s->server_status;
    NS_UNLOCK_INSTANCE();
    return NS_OK;
}

NS_IMETHODIMP nsImapUrl::ToString(PRUnichar* *aString) const
{ 
	if (aString)
		*aString = nsnull; 
	return NS_OK;
}

////////////////////////////////////////////////////////////////////////////////////
// End of functions which should be made obsolete after modifying nsIURL
////////////////////////////////////////////////////////////////////////////////////
