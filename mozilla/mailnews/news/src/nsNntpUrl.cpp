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
#include "nsNntpUrl.h"

#include "nsINetService.h"  /* XXX: NS_FALSE */

#include "nsString.h"
#include "prmem.h"
#include "plstr.h"
#include "prprf.h"
#include "nsCRT.h"
#include "nsNewsUtils.h"

#include "nntpCore.h"

#include "nsCOMPtr.h"
#include "nsIMsgDatabase.h"
#include "nsMsgDBCID.h"

// we need this because of an egcs 1.0 (and possibly gcc) compiler bug
// that doesn't allow you to call ::nsISupports::GetIID() inside of a class
// that multiply inherits from nsISupports
static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);
static NS_DEFINE_CID(kUrlListenerManagerCID, NS_URLLISTENERMANAGER_CID);
static NS_DEFINE_CID(kCNewsDB, NS_NEWSDB_CID);
    
nsNntpUrl::nsNntpUrl(nsISupports* aContainer, nsIURLGroup* aGroup)
{
    NS_INIT_REFCNT();

	// nsINntpUrl specific code...
	m_newsHost = nsnull;
	m_articleList = nsnull;
	m_newsgroup = nsnull;
	m_offlineNews = nsnull;
	m_newsgroupList = nsnull;
	m_errorMessage = nsnull;
    m_newsgroupPost = nsnull;
    m_newsgroupName = nsnull;
    m_messageKey = nsMsgKey_None;
    
	// nsINetLibUrl specific state
    m_URL_s = nsnull;

	// nsIURL specific state
    m_protocol = nsnull;
    m_host = nsnull;
    m_file = nsnull;
    m_ref = nsnull;
    m_port = NEWS_PORT;
    m_spec = nsnull;
    m_search = nsnull;

	m_filePath = nsnull;
 
    m_container = aContainer;
    NS_IF_ADDREF(m_container);

	m_runningUrl = PR_FALSE;
	m_urlListeners = nsnull;
	nsComponentManager::CreateInstance(kUrlListenerManagerCID, nsnull, nsIUrlListenerManager::GetIID(), 
									   (void **) &m_urlListeners);
}
 
nsNntpUrl::~nsNntpUrl()
{
	NS_IF_RELEASE(m_urlListeners);
    NS_IF_RELEASE(m_container);
	NS_IF_RELEASE(m_newsHost);
	NS_IF_RELEASE(m_articleList);
	NS_IF_RELEASE(m_newsgroup);
	NS_IF_RELEASE(m_offlineNews);
	NS_IF_RELEASE(m_newsgroupList);
    PR_FREEIF(m_newsgroupPost);
	PR_FREEIF(m_errorMessage);
    PR_FREEIF(m_newsgroupName);
    
    delete m_filePath;
    PR_FREEIF(m_spec);
    PR_FREEIF(m_protocol);
    PR_FREEIF(m_host);
    PR_FREEIF(m_file);
    PR_FREEIF(m_ref);
    PR_FREEIF(m_search);
    if (nsnull != m_URL_s) 
	{
//        NET_DropURLStruct(m_URL_s);
    }
}
  
NS_IMPL_THREADSAFE_ADDREF(nsNntpUrl);
NS_IMPL_THREADSAFE_RELEASE(nsNntpUrl);

nsresult nsNntpUrl::QueryInterface(const nsIID &aIID, void** aInstancePtr)
{
    if (NULL == aInstancePtr) {
        return NS_ERROR_NULL_POINTER;
    }
 
    if (aIID.Equals(nsINntpUrl::GetIID()) ||
        aIID.Equals(kISupportsIID)) {
        *aInstancePtr = (void*) ((nsINntpUrl*)this);
        NS_ADDREF_THIS();
        return NS_OK;
    }
    if (aIID.Equals(nsIURL::GetIID())) {
        *aInstancePtr = (void*) ((nsIURL*)this);
        NS_ADDREF_THIS();
        return NS_OK;
    }
    if (aIID.Equals(nsINetlibURL::GetIID())) {
        *aInstancePtr = (void*) ((nsINetlibURL*)this);
        NS_ADDREF_THIS();
        return NS_OK;
    }

	if (aIID.Equals(nsIMsgMailNewsUrl::GetIID()))
	{
		*aInstancePtr = (void *) ((nsIMsgMailNewsUrl*) this);
		NS_ADDREF_THIS();
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
// Begin nsINntpUrl specific support
////////////////////////////////////////////////////////////////////////////////////
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

nsresult nsNntpUrl::GetNntpHost (nsINNTPHost ** newsHost) const
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

nsresult nsNntpUrl::GetNntpArticleList (nsINNTPArticleList ** articleList) const
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

nsresult nsNntpUrl::GetNewsgroup (nsINNTPNewsgroup ** newsgroup) const
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

nsresult nsNntpUrl::GetOfflineNewsState (nsIMsgOfflineNewsState ** offlineNews) const
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

nsresult nsNntpUrl::GetNewsgroupList (nsINNTPNewsgroupList ** newsgroupList) const
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

// url listener registration details...
	
NS_IMETHODIMP nsNntpUrl::RegisterListener (nsIUrlListener * aUrlListener)
{
	NS_LOCK_INSTANCE();
	nsresult rv = NS_OK;
	if (m_urlListeners)
		rv = m_urlListeners->RegisterListener(aUrlListener);
    NS_UNLOCK_INSTANCE();
	return rv;
}
	
NS_IMETHODIMP nsNntpUrl::UnRegisterListener (nsIUrlListener * aUrlListener)
{
	NS_LOCK_INSTANCE();
	nsresult rv = NS_OK;
	if (m_urlListeners)
		rv = m_urlListeners->UnRegisterListener(aUrlListener);
	NS_UNLOCK_INSTANCE();
	return rv;
}

NS_IMETHODIMP nsNntpUrl::GetUrlState(PRBool * aRunningUrl)
{
	NS_LOCK_INSTANCE();
	if (aRunningUrl)
		*aRunningUrl = m_runningUrl;

	NS_UNLOCK_INSTANCE();
	return NS_OK;
}

NS_IMETHODIMP nsNntpUrl::SetUrlState(PRBool aRunningUrl, nsresult aExitCode)
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

nsresult nsNntpUrl::SetErrorMessage (char * errorMessage)
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
nsresult nsNntpUrl::GetErrorMessage (char ** errorMessage) const
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

nsresult 
nsNntpUrl::GetFilePath(const nsFileSpec ** aFilePath)
{
	if (aFilePath)
		*aFilePath = m_filePath;
	return NS_OK;
}

#if 0
nsresult nsNntpUrl::SetFilePath(const nsFileSpec& aFilePath)
{
	NS_LOCK_INSTANCE();
	if (m_filePath)
		delete m_filePath;
	m_filePath = new nsFileSpec(aFilePath);

    NS_UNLOCK_INSTANCE();
    return NS_OK;	
}
#endif

// from nsIMsgUriUrl
NS_IMETHODIMP nsNntpUrl::GetURI(char ** aURI)
{	
	if (aURI)
	{
		const nsFileSpec * filePath = nsnull;
		GetFilePath(&filePath);
		if (filePath)
		{
			char * uri = nsnull;
			nsFileSpec folder = *filePath;
			nsBuildNewsMessageURI(m_spec, 0 /* don't have keys yet */, &uri);
			*aURI = uri;
		}
		else
			*aURI = nsnull;

	}

	return NS_OK;
}

////////////////////////////////////////////////////////////////////////////////////
// End nsINntpUrl specific support
////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////
// Begin nsINetlibURL support
////////////////////////////////////////////////////////////////////////////////////

NS_METHOD nsNntpUrl::SetURLInfo(URL_Struct *URL_s)
{
    nsresult result = NS_OK;
  
    /* Hook us up with the world. */
    m_URL_s = URL_s;
//    NET_HoldURLStruct(URL_s);
    return result;
}
  
NS_METHOD nsNntpUrl::GetURLInfo(URL_Struct_** aResult) const
{
  nsresult rv;

  if (nsnull == aResult) {
    rv = NS_ERROR_NULL_POINTER;
  } else {
    /* XXX: Should the URL be reference counted here?? */
    *aResult = m_URL_s;
    rv = NS_OK;
  }

  return rv;
}

////////////////////////////////////////////////////////////////////////////////////
// End nsINetlibURL support
////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////

// XXX recode to use nsString api's
// XXX don't bother with port numbers
// XXX don't bother with ref's
// XXX null pointer checks are incomplete

// warning: don't assume when parsing the url that the protocol part is "news"...

nsresult nsNntpUrl::ParseURL(const nsString& aSpec, const nsIURL* aURL)
{
    // XXX hack!
    char* cSpec = aSpec.ToNewCString();

    const char* uProtocol = nsnull;
    const char* uHost = nsnull;
    const char* uFile = nsnull;
    PRUint32 uPort;
    if (nsnull != aURL) {
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
    PR_FREEIF(m_file);
    PR_FREEIF(m_ref);
    PR_FREEIF(m_search);
    m_port = NEWS_PORT;

    if (nsnull == cSpec) {
        if (nsnull == aURL) {
            NS_UNLOCK_INSTANCE();
            return NS_ERROR_ILLEGAL_VALUE;
        }
        m_protocol = (nsnull != uProtocol) ? PL_strdup(uProtocol) : nsnull;
        m_host = (nsnull != uHost) ? PL_strdup(uHost) : nsnull;
        m_port = uPort;
        m_file = (nsnull != uFile) ? PL_strdup(uFile) : nsnull;

        NS_UNLOCK_INSTANCE();
        return NS_OK;
    }

    // Strip out reference and search info
    char* ref = strpbrk(cSpec, "#?");
    if (nsnull != ref) {
        char* search = nsnull;
        if ('#' == *ref) {
            search = PL_strchr(ref + 1, '?');
            if (nsnull != search) {
                *search++ = '\0';
            }

            PRIntn hashLen = PL_strlen(ref + 1);
            if (0 != hashLen) {
                m_ref = (char*) PR_Malloc(hashLen + 1);
                PL_strcpy(m_ref, ref + 1);
            }      
        }
        else {
            search = ref + 1;
        }

        if (nsnull != search) {
            // The rest is the search
            PRIntn searchLen = PL_strlen(search);
            if (0 != searchLen) {
                m_search = (char*) PR_Malloc(searchLen + 1);
                PL_strcpy(m_search, search);
            }      
        }

        // XXX Terminate string at start of reference or search
        *ref = '\0';
    }

    // The URL is considered absolute if and only if it begins with a
    // protocol spec. A protocol spec is an alphanumeric string of 1 or
    // more characters that is terminated with a colon.
    PRBool isAbsolute = PR_FALSE;
    char* cp=NULL;
    char* ap = cSpec;
    char ch;
    while (0 != (ch = *ap)) {
        if (((ch >= 'a') && (ch <= 'z')) ||
            ((ch >= 'A') && (ch <= 'Z')) ||
            ((ch >= '0') && (ch <= '9'))) {
            ap++;
            continue;
        }
        if ((ch == ':') && (ap - cSpec >= 2)) {
            isAbsolute = PR_TRUE;
            cp = ap;
            break;
        }
        break;
    }

    if (!isAbsolute) {
        // relative spec
        if (nsnull == aURL) {
            delete [] cSpec;

            NS_UNLOCK_INSTANCE();
            return NS_ERROR_ILLEGAL_VALUE;
        }

        // keep protocol and host
        m_protocol = (nsnull != uProtocol) ? PL_strdup(uProtocol) : nsnull;
        m_host = (nsnull != uHost) ? PL_strdup(uHost) : nsnull;
        m_port = uPort;

        // figure out file name
        PRInt32 len = PL_strlen(cSpec) + 1;
        if ((len > 1) && (cSpec[0] == '/')) {
            // Relative spec is absolute to the server
            m_file = PL_strdup(cSpec);
        } else {
            if (cSpec[0] != '\0') {
                // Strip out old tail component and put in the new one
                char* dp = PL_strrchr(uFile, '/');
                if (!dp) {
                    delete [] cSpec;
                    NS_UNLOCK_INSTANCE();
                    return NS_ERROR_ILLEGAL_VALUE;
                }
                PRInt32 dirlen = (dp + 1) - uFile;
                m_file = (char*) PR_Malloc(dirlen + len);
                PL_strncpy(m_file, uFile, dirlen);
                PL_strcpy(m_file + dirlen, cSpec);
            }
            else {
                m_file = PL_strdup(uFile);
            }
        }

        /* Stolen from netlib's mkparse.c.
         *
         * modifies a url of the form   /foo/../foo1  ->  /foo1
         *                       and    /foo/./foo1   ->  /foo/foo1
         */
        char *fwdPtr = m_file;
        char *urlPtr = m_file;
    
        for(; *fwdPtr != '\0'; fwdPtr++)
        {
    
            if(*fwdPtr == '/' && *(fwdPtr+1) == '.' && *(fwdPtr+2) == '/')
            {
                /* remove ./ 
                 */	
                fwdPtr += 1;
            }
            else if(*fwdPtr == '/' && *(fwdPtr+1) == '.' && *(fwdPtr+2) == '.' && 
                    (*(fwdPtr+3) == '/' || *(fwdPtr+3) == '\0'))
            {
                /* remove foo/.. 
                 */	
                /* reverse the urlPtr to the previous slash
                 */
                if(urlPtr != m_file) 
                    urlPtr--; /* we must be going back at least one */
                for(;*urlPtr != '/' && urlPtr != m_file; urlPtr--)
                    ;  /* null body */
        
                /* forward the fwd_prt past the ../
                 */
                fwdPtr += 2;
            }
            else
            {
                /* copy the url incrementaly 
                 */
                *urlPtr++ = *fwdPtr;
            }
        }
    
        *urlPtr = '\0';  /* terminate the url */

        // Now that we've resolved the relative URL, we need to reconstruct
        // a URL spec from the components.
        ReconstructSpec();
    } else {
        // absolute spec

        PR_FREEIF(m_spec);
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
        if (*cp == '/') {
            cp++;
            if (*cp == '/') {
                cp++;
                if (*cp == '/') {
                    cp++;
                }
            }
        } else {
            delete [] cSpec;

            NS_UNLOCK_INSTANCE();
            return NS_ERROR_ILLEGAL_VALUE;
        }


#if defined(XP_UNIX) || defined (XP_MAC)
        // Always leave the top level slash for absolute file paths under Mac and UNIX.
        // The code above sometimes results in stripping all of slashes
        // off. This only happens when a previously stripped url is asked to be
        // parsed again. Under Win32 this is not a problem since file urls begin
        // with a drive letter not a slash. This problem show's itself when 
        // nested documents such as iframes within iframes are parsed.

        if (PL_strcmp(m_protocol, "file") == 0) {
            if (*cp != '/') {
                cp--;
            }
        }
#endif /* XP_UNIX */

        const char* cp0 = cp;
        if ((PL_strcmp(m_protocol, "resource") == 0) ||
            (PL_strcmp(m_protocol, "file") == 0)) {
            // resource/file url's do not have host names.
            // The remainder of the string is the file name
            PRInt32 flen = PL_strlen(cp);
            m_file = (char*) PR_Malloc(flen + 1);
            PL_strcpy(m_file, cp);
      
#ifdef NS_WIN32
            if (PL_strcmp(m_protocol, "file") == 0) {
                // If the filename starts with a "x|" where is an single
                // character then we assume it's a drive name and change the
                // vertical bar back to a ":"
                if ((flen >= 2) && (m_file[1] == '|')) {
                    m_file[1] = ':';
                }
            }
#endif /* NS_WIN32 */
        } else {
            // Host name follows protocol for http style urls
            cp = PL_strpbrk(cp, "/:");
      
            if (nsnull == cp) {
                // There is only a host name
                PRInt32 hlen = PL_strlen(cp0);
                m_host = (char*) PR_Malloc(hlen + 1);
                PL_strcpy(m_host, cp0);
            }
            else {
                PRInt32 hlen = cp - cp0;
                m_host = (char*) PR_Malloc(hlen + 1);
                PL_strncpy(m_host, cp0, hlen);        
                m_host[hlen] = 0;

                if (':' == *cp) {
                    // We have a port number
                    cp0 = cp+1;
                    cp = PL_strchr(cp, '/');
                    m_port = strtol(cp0, (char **)nsnull, 10);
                }
            }

            if (nsnull == cp) {
                // There is no file name
                // Set filename to "/"
                m_file = (char*) PR_Malloc(2);
                m_file[0] = '/';
                m_file[1] = 0;
            }
            else {
                // The rest is the file name
                PRInt32 flen = PL_strlen(cp);
                m_file = (char*) PR_Malloc(flen + 1);
                PL_strcpy(m_file, cp);
            }
        }
    }

#ifdef DEBUG_NEWS
    printf("protocol='%s' host='%s' file='%s'\n", m_protocol, m_host, m_file);
#endif
    delete [] cSpec;

    NS_UNLOCK_INSTANCE();
    return NS_OK;
}

void nsNntpUrl::ReconstructSpec(void)
{
    PR_FREEIF(m_spec);

    char portBuffer[10];
    PR_snprintf(portBuffer, 10, ":%u", m_port);

    PRInt32 plen = PL_strlen(m_protocol) + PL_strlen(m_host) +
        PL_strlen(portBuffer) + PL_strlen(m_file) + 4;
    if (m_ref) {
        plen += 1 + PL_strlen(m_ref);
    }
    if (m_search) {
        plen += 1 + PL_strlen(m_search);
    }

    m_spec = (char *) PR_Malloc(plen + 1);
    PR_snprintf(m_spec, plen, "%s://%s%s%s", 
                m_protocol, ((nsnull != m_host) ? m_host : ""), portBuffer,
                m_file);

    if (m_ref) {
        PL_strcat(m_spec, "#");
        PL_strcat(m_spec, m_ref);
    }
    if (m_search) {
        PL_strcat(m_spec, "?");
        PL_strcat(m_spec, m_search);
    }
}

////////////////////////////////////////////////////////////////////////////////

PRBool nsNntpUrl::Equals(const nsIURL* aURL) const 
{
    PRBool bIsEqual;
    nsNntpUrl* other;
    NS_LOCK_INSTANCE();
	// are they both Nntp urls?? if yes...for now just compare the pointers until 
	// I figure out if we need to check any of the guts for equality....
    if (((nsIURL*)aURL)->QueryInterface(nsINntpUrl::GetIID(), (void**)&other) == NS_OK) {
        bIsEqual = other == this; // compare the pointers...
    }
    else
        bIsEqual = PR_FALSE;
    NS_UNLOCK_INSTANCE();
    return bIsEqual;
}

nsresult nsNntpUrl::GetProtocol(const char* *result) const
{
    NS_LOCK_INSTANCE();
    *result = m_protocol;
    NS_UNLOCK_INSTANCE();
    return NS_OK;
}

nsresult nsNntpUrl::SetProtocol(const char *aNewProtocol)
{
    NS_LOCK_INSTANCE();
    m_protocol = nsCRT::strdup(aNewProtocol);
    ReconstructSpec();
    NS_UNLOCK_INSTANCE();
    return NS_OK;
}

nsresult nsNntpUrl::GetHost(const char* *result) const
{
    NS_LOCK_INSTANCE();
    *result = m_host;
    NS_UNLOCK_INSTANCE();
    return NS_OK;
}

nsresult nsNntpUrl::SetHost(const char *aNewHost)
{
    NS_ASSERTION(m_URL_s == nsnull, "URL has already been opened");
    NS_LOCK_INSTANCE();
    m_host = nsCRT::strdup(aNewHost);
    ReconstructSpec();
    NS_UNLOCK_INSTANCE();
    return NS_OK;
}

nsresult nsNntpUrl::GetFile(const char* *result) const
{
    NS_LOCK_INSTANCE();
    *result = m_file;
    NS_UNLOCK_INSTANCE();
    return NS_OK;
}

nsresult nsNntpUrl::SetFile(const char *aNewFile)
{
    NS_ASSERTION(m_URL_s == nsnull, "URL has already been opened");
    NS_LOCK_INSTANCE();
    m_file = nsCRT::strdup(aNewFile);
    ReconstructSpec();
    NS_UNLOCK_INSTANCE();
    return NS_OK;
}

nsresult nsNntpUrl::GetSpec(const char* *result) const
{
    NS_LOCK_INSTANCE();
    *result = m_spec;
    NS_UNLOCK_INSTANCE();
    return NS_OK;
}

nsresult nsNntpUrl::SetSpec(const char *aNewSpec)
{
#ifdef DEBUG_NEWS
    if (aNewSpec) {
	printf("nsNntpUrl::SetSpec(%s)\n", aNewSpec);
    }
#endif
    // XXX is this right, or should we call ParseURL?
    nsresult rv = NS_OK;
//    NS_ASSERTION(m_URL_s == nsnull, "URL has already been opened");
    NS_LOCK_INSTANCE();
    rv = ParseURL(aNewSpec);
#if 0
    PR_FREEIF(m_spec);
    m_spec = nsCRT::strdup(aNewSpec);
#endif
    NS_UNLOCK_INSTANCE();
    return rv;
}

nsresult nsNntpUrl::GetRef(const char* *result) const
{
    NS_LOCK_INSTANCE();
    *result = m_ref;
    NS_UNLOCK_INSTANCE();
    return NS_OK;
}

nsresult nsNntpUrl::SetRef(const char *aNewRef)
{
    NS_ASSERTION(m_URL_s == nsnull, "URL has already been opened");
    NS_LOCK_INSTANCE();
    m_ref = nsCRT::strdup(aNewRef);
    ReconstructSpec();
    NS_UNLOCK_INSTANCE();
    return NS_OK;
}

nsresult nsNntpUrl::GetHostPort(PRUint32 *result) const
{
    NS_LOCK_INSTANCE();
    *result = m_port;
    NS_UNLOCK_INSTANCE();
    return NS_OK;
}

nsresult nsNntpUrl::SetHostPort(PRUint32 aNewPort)
{
    NS_ASSERTION(m_URL_s == nsnull, "URL has already been opened");
    NS_LOCK_INSTANCE();
    m_port = aNewPort;
    ReconstructSpec();
    NS_UNLOCK_INSTANCE();
    return NS_OK;
}

nsresult nsNntpUrl::GetSearch(const char* *result) const
{
    NS_LOCK_INSTANCE();
    *result = m_search;
    NS_UNLOCK_INSTANCE();
    return NS_OK;
}

nsresult nsNntpUrl::SetSearch(const char *aNewSearch)
{
    NS_ASSERTION(m_URL_s == nsnull, "URL has already been opened");
    NS_LOCK_INSTANCE();
    m_search = nsCRT::strdup(aNewSearch);
    ReconstructSpec();
    NS_UNLOCK_INSTANCE();
    return NS_OK;
}

nsresult nsNntpUrl::GetContainer(nsISupports* *result) const
{
    NS_LOCK_INSTANCE();
    *result = m_container;
    NS_IF_ADDREF(m_container);
    NS_UNLOCK_INSTANCE();
    return NS_OK;
}
  
nsresult nsNntpUrl::SetContainer(nsISupports* container)
{
    NS_ASSERTION(m_URL_s == nsnull, "URL has already been opened");
    NS_LOCK_INSTANCE();
    NS_IF_RELEASE(m_container);
    m_container = container;
    NS_IF_ADDREF(m_container);
    NS_UNLOCK_INSTANCE();
    return NS_OK;
}

nsresult nsNntpUrl::GetContentLength(PRInt32 *len)
{
    NS_LOCK_INSTANCE();
    *len = m_URL_s->content_length;
    NS_UNLOCK_INSTANCE();
    return NS_OK;
}

nsresult nsNntpUrl::GetLoadAttribs(nsILoadAttribs* *result) const
{
    NS_LOCK_INSTANCE();
    *result = NULL;
    NS_UNLOCK_INSTANCE();
    return NS_OK;
}
  
nsresult nsNntpUrl::SetLoadAttribs(nsILoadAttribs* aLoadAttribs)
{
    NS_ASSERTION(m_URL_s == nsnull, "URL has already been opened");
    return NS_OK;
}

nsresult nsNntpUrl::SetPostHeader(const char* name, const char* value)
{
    NS_LOCK_INSTANCE();
    // XXX
    PR_ASSERT(0);
    NS_UNLOCK_INSTANCE();
    return NS_OK;
}

nsresult nsNntpUrl::SetPostData(nsIInputStream* input)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult nsNntpUrl::GetURLGroup(nsIURLGroup* *result) const
{
    return NS_OK;
}
  
nsresult nsNntpUrl::SetURLGroup(nsIURLGroup* group)
{
    NS_ASSERTION(m_URL_s == nsnull, "URL has already been opened");
    return NS_OK;
}

nsresult nsNntpUrl::GetServerStatus(PRInt32 *status)
{
    NS_LOCK_INSTANCE();
    *status = m_URL_s->server_status;
    NS_UNLOCK_INSTANCE();
    return NS_OK;
}

nsresult nsNntpUrl::ToString(PRUnichar* *aString) const
{ 
	if (aString)
		*aString = nsnull; 
	return NS_OK;
}

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
    nsNativeFileSpec pathResult;
    
    if (!aMsgHdr) return NS_ERROR_NULL_POINTER;

    if (!m_newsgroupName) return NS_ERROR_FAILURE;

    if (!m_host) return NS_ERROR_FAILURE;

    nsString2 newsgroupURI(kNewsMessageRootURI, eOneByte);
    newsgroupURI.Append("/");
    newsgroupURI.Append(m_host);
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
    
    rv = newsDBFactory->Open(pathResult, PR_TRUE, getter_AddRefs(newsDB), PR_FALSE);
    
    if (NS_FAILED(rv) || (!newsDB)) {
        return rv;
    }
    
    rv = newsDB->GetMsgHdrForKey(m_messageKey, aMsgHdr);
    if (NS_FAILED(rv) || (!aMsgHdr)) {
        return rv;
    }
  
	return NS_OK;
}

NS_IMETHODIMP nsNntpUrl::SetNewsgroupName(char * aNewsgroupName)
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