/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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

#include "nsIOService.h"
#include "nsIProtocolHandler.h"
#include "nscore.h"
#include "nsIServiceManager.h"
#include "nsIEventQueueService.h"
#include "nsIFileTransportService.h"
#include "nsIURI.h"
#include "nsIStreamListener.h"
#include "prprf.h"
#include "nsLoadGroup.h"
#include "nsInputStreamChannel.h"
#include "nsXPIDLString.h" 
#include "nsIErrorService.h" 
#include "netCore.h"
#include "nsIObserverService.h"
#include "nsIHttpProtocolHandler.h"

static NS_DEFINE_CID(kFileTransportService, NS_FILETRANSPORTSERVICE_CID);
static NS_DEFINE_CID(kEventQueueService, NS_EVENTQUEUESERVICE_CID);
static NS_DEFINE_CID(kSocketTransportServiceCID, NS_SOCKETTRANSPORTSERVICE_CID);
static NS_DEFINE_CID(kDNSServiceCID, NS_DNSSERVICE_CID);
static NS_DEFINE_CID(kErrorServiceCID, NS_ERRORSERVICE_CID);
static NS_DEFINE_CID(kProtocolProxyServiceCID, NS_PROTOCOLPROXYSERVICE_CID);

////////////////////////////////////////////////////////////////////////////////

nsIOService::nsIOService()
    : mOffline(PR_FALSE)
{
    NS_INIT_REFCNT();
}

nsresult
nsIOService::Init()
{
    nsresult rv = NS_OK;
    
    // Hold onto the eventQueue service.  We do not want any eventqueues to go away
    // when we shutdown until we process all remaining transports

    if (NS_SUCCEEDED(rv))
        mEventQueueService = do_GetService(NS_EVENTQUEUESERVICE_CONTRACTID, &rv);
  
    
    // We need to get references to these services so that we can shut them
    // down later. If we wait until the nsIOService is being shut down,
    // GetService will fail at that point.
    rv = nsServiceManager::GetService(kSocketTransportServiceCID,
                                      NS_GET_IID(nsISocketTransportService),
                                      getter_AddRefs(mSocketTransportService));

    if (NS_FAILED(rv)) return rv;

    rv = nsServiceManager::GetService(kFileTransportService,
                                      NS_GET_IID(nsIFileTransportService),
                                      getter_AddRefs(mFileTransportService));
    if (NS_FAILED(rv)) return rv;

    rv = nsServiceManager::GetService(kDNSServiceCID,
                                      NS_GET_IID(nsIDNSService),
                                      getter_AddRefs(mDNSService));
    if (NS_FAILED(rv)) return rv;

    rv = nsServiceManager::GetService(kProtocolProxyServiceCID,
                                      NS_GET_IID(nsIProtocolProxyService),
                                      getter_AddRefs(mProxyService));
    if (NS_FAILED(rv)) return rv;

    // XXX hack until xpidl supports error info directly (http://bugzilla.mozilla.org/show_bug.cgi?id=13423)
    nsCOMPtr<nsIErrorService> errorService = do_GetService(kErrorServiceCID, &rv);
    if (NS_SUCCEEDED(rv)) {
        rv = errorService->RegisterErrorStringBundle(NS_ERROR_MODULE_NETWORK, NECKO_MSGS_URL);
        if (NS_FAILED(rv)) return rv;
        rv = errorService->RegisterErrorStringBundleKey(NS_NET_STATUS_READ_FROM, "ReadFrom");
        if (NS_FAILED(rv)) return rv;
        rv = errorService->RegisterErrorStringBundleKey(NS_NET_STATUS_WROTE_TO, "WroteTo");
        if (NS_FAILED(rv)) return rv;
        rv = errorService->RegisterErrorStringBundleKey(NS_NET_STATUS_RESOLVING_HOST, "ResolvingHost");
        if (NS_FAILED(rv)) return rv;
        rv = errorService->RegisterErrorStringBundleKey(NS_NET_STATUS_CONNECTED_TO, "ConnectedTo");
        if (NS_FAILED(rv)) return rv;
        rv = errorService->RegisterErrorStringBundleKey(NS_NET_STATUS_SENDING_TO, "SendingTo");
        if (NS_FAILED(rv)) return rv;
        rv = errorService->RegisterErrorStringBundleKey(NS_NET_STATUS_RECEIVING_FROM, "ReceivingFrom");
        if (NS_FAILED(rv)) return rv;
        rv = errorService->RegisterErrorStringBundleKey(NS_NET_STATUS_CONNECTING_TO, "ConnectingTo");
        if (NS_FAILED(rv)) return rv;
    }
    return rv;
}


nsIOService::~nsIOService()
{
    (void)SetOffline(PR_TRUE);
    if (mFileTransportService)
        (void)mFileTransportService->Shutdown();
}   

NS_METHOD
nsIOService::Create(nsISupports *aOuter, REFNSIID aIID, void **aResult)
{
    static  nsISupports *_rValue = nsnull;

    nsresult rv;
    NS_ENSURE_NO_AGGREGATION(aOuter);

    if (_rValue)
    {
        NS_ADDREF (_rValue);
        *aResult = _rValue;
        return NS_OK;
    }

    nsIOService* _ios = new nsIOService();
    if (_ios == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(_ios);
    rv = _ios->Init();
    if (NS_FAILED(rv))
    {
        delete _ios;
        return rv;
    }

    rv = _ios->QueryInterface(aIID, aResult);

    if (NS_FAILED(rv))
    {
        delete _ios;
        return rv;
    }
    
    _rValue = NS_STATIC_CAST (nsISupports*, *aResult);
    NS_RELEASE (_rValue);
    _rValue = nsnull;

    return rv;
}

NS_IMPL_THREADSAFE_ISUPPORTS1(nsIOService, nsIIOService);

////////////////////////////////////////////////////////////////////////////////

#define MAX_SCHEME_LENGTH       64      // XXX big enough?

#define MAX_NET_CONTRACTID_LENGTH   (MAX_SCHEME_LENGTH + NS_NETWORK_PROTOCOL_CONTRACTID_PREFIX_LENGTH + 1)

NS_IMETHODIMP
nsIOService::CacheProtocolHandler(const char *scheme, nsIProtocolHandler *handler)
{
    for (unsigned int i=0; i<NS_N(gScheme); i++)
    {
        if (!nsCRT::strcasecmp(scheme, gScheme[i]))
        {
            nsresult rv;
            NS_ASSERTION(!mWeakHandler[i], "Protocol handler already cached");
            // Make sure the handler supports weak references.
            nsCOMPtr<nsISupportsWeakReference> factoryPtr = do_QueryInterface(handler, &rv);
            if (!factoryPtr)
            {
                // Dont cache handlers that dont support weak reference as
                // there is real danger of a circular reference.
#ifdef DEBUG_dp
                printf("DEBUG: %s protcol handler doesn't support weak ref. Not cached.\n", scheme);
#endif /* DEBUG_dp */
                return NS_ERROR_FAILURE;
            }
            mWeakHandler[i] = getter_AddRefs(NS_GetWeakReference(handler));
            return NS_OK;
        }
    }
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsIOService::GetCachedProtocolHandler(const char *scheme, nsIProtocolHandler **result)
{
    for (unsigned int i=0; i<NS_N(gScheme); i++)
    {
        if (!nsCRT::strcasecmp(scheme, gScheme[i]))
            if (mWeakHandler[i])
            {
                nsCOMPtr<nsIProtocolHandler> temp = do_QueryReferent(mWeakHandler[i]);
                if (temp)
                {
                    *result = temp.get();
                    NS_ADDREF(*result);
                    return NS_OK;
                }
            }
    }
    return NS_ERROR_FAILURE;
}
 

NS_IMETHODIMP
nsIOService::GetProtocolHandler(const char* scheme, nsIProtocolHandler* *result)
{
    nsresult rv;

    NS_ASSERTION(NS_NETWORK_PROTOCOL_CONTRACTID_PREFIX_LENGTH
                 == nsCRT::strlen(NS_NETWORK_PROTOCOL_CONTRACTID_PREFIX),
                 "need to fix NS_NETWORK_PROTOCOL_CONTRACTID_PREFIX_LENGTH");

    NS_ENSURE_ARG_POINTER(scheme);
    // XXX we may want to speed this up by introducing our own protocol 
    // scheme -> protocol handler mapping, avoiding the string manipulation
    // and service manager stuff

    rv = GetCachedProtocolHandler(scheme, result);
    if (NS_SUCCEEDED(rv)) return NS_OK;

    char buf[MAX_NET_CONTRACTID_LENGTH];
    nsCAutoString contractID(NS_NETWORK_PROTOCOL_CONTRACTID_PREFIX);
    contractID += scheme;
    contractID.ToLowerCase();
    contractID.ToCString(buf, MAX_NET_CONTRACTID_LENGTH);

    rv = nsServiceManager::GetService(buf, NS_GET_IID(nsIProtocolHandler), (nsISupports **)result);
    if (NS_FAILED(rv)) 
    {
      // okay we don't have a protocol handler to handle this url type, so use the default protocol handler.
      // this will cause urls to get dispatched out to the OS ('cause we can't do anything with them) when 
      // we try to read from a channel created by the default protocol handler.

      rv = nsServiceManager::GetService(NS_NETWORK_PROTOCOL_CONTRACTID_PREFIX"default", NS_GET_IID(nsIProtocolHandler), (nsISupports **)result);
      if (NS_FAILED(rv)) return NS_ERROR_UNKNOWN_PROTOCOL;
    }

    CacheProtocolHandler(scheme, *result);

    return NS_OK;
}

NS_IMETHODIMP
nsIOService::ExtractScheme(const char* inURI, PRUint32 *startPos, 
                           PRUint32 *endPos, char* *scheme)
{
    return ExtractURLScheme(inURI, startPos, endPos, scheme);
}

nsresult
nsIOService::NewURI(const char* aSpec, nsIURI* aBaseURI, nsIURI* *result)
{
    nsresult rv;
    nsIURI* base;
    NS_ENSURE_ARG_POINTER(aSpec);
    char* scheme;
    rv = ExtractScheme(aSpec, nsnull, nsnull, &scheme);
    if (NS_SUCCEEDED(rv)) {
        // then aSpec is absolute
        // ignore aBaseURI in this case
        base = nsnull;
    }
    else {
        // then aSpec is relative
        if (aBaseURI == nsnull)
            return NS_ERROR_MALFORMED_URI;
        rv = aBaseURI->GetScheme(&scheme);
        if (NS_FAILED(rv)) return rv;
        base = aBaseURI;
    }
    
    nsCOMPtr<nsIProtocolHandler> handler;
    rv = GetProtocolHandler(scheme, getter_AddRefs(handler));
    nsCRT::free(scheme);
    if (NS_FAILED(rv)) return rv;

    return handler->NewURI(aSpec, base, result);
}

NS_IMETHODIMP
nsIOService::NewChannelFromURI(nsIURI *aURI, nsIChannel **result)
{
    nsresult rv;
    NS_ENSURE_ARG_POINTER(aURI);

    nsXPIDLCString scheme;
    rv = aURI->GetScheme(getter_Copies(scheme));
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIProtocolHandler> handler;
    rv = GetProtocolHandler((const char*)scheme, getter_AddRefs(handler));
    if (NS_FAILED(rv)) return rv;

    PRBool useProxy = PR_FALSE;
    if (mProxyService) {
        mProxyService->GetProxyEnabled(&useProxy);
        if (useProxy) {
            nsXPIDLCString host, type;
            PRInt32 port;
            rv = mProxyService->ExamineForProxy(aURI, getter_Copies(host), &port, getter_Copies(type));
            if (NS_SUCCEEDED(rv) && type.get())
            {
                // we are going to proxy this channel and the only protocol that can do proxying is http
                rv = GetProtocolHandler("http", getter_AddRefs(handler));
                if (NS_FAILED(rv)) return rv;
                
                nsCOMPtr<nsIHttpProtocolHandler> httpHandler = do_QueryInterface(handler, &rv);
                if (NS_FAILED(rv)) 
                    return rv;

                if (!httpHandler)
                    return NS_ERROR_NULL_POINTER;

                return httpHandler->NewProxyChannel(aURI, host, port, type, result);
            }
        }
    }

    rv = handler->NewChannel(aURI, result);
    return rv;
}

NS_IMETHODIMP
nsIOService::NewChannel(const char *aSpec, nsIURI *aBaseURI, nsIChannel **result)
{
    nsresult rv;
    nsCOMPtr<nsIURI> uri;
    rv = NewURI(aSpec, aBaseURI, getter_AddRefs(uri));
    if (NS_FAILED(rv)) return rv;

    return NewChannelFromURI(uri, result);
}

NS_IMETHODIMP
nsIOService::GetOffline(PRBool *offline)
{
    *offline = mOffline;
    return NS_OK;
}

NS_IMETHODIMP
nsIOService::SetOffline(PRBool offline)
{
    nsCOMPtr<nsIObserverService>
        observerService(do_GetService(NS_OBSERVERSERVICE_CONTRACTID));
    
    nsresult rv1 = NS_OK;
    nsresult rv2 = NS_OK;
    if (offline) {
        // don't care if notification fails
        if (observerService)
            (void)observerService->Notify(this,
                                          NS_LITERAL_STRING("network:offline-status-changed").get(),
                                          NS_LITERAL_STRING("offline").get());
    	mOffline = PR_TRUE;		// indicate we're trying to shutdown
        // be sure to try and shutdown both (even if the first fails)
        if (mDNSService)
            rv1 = mDNSService->Shutdown();  // shutdown dns service first, because it has callbacks for socket transport
        if (mSocketTransportService)
            rv2 = mSocketTransportService->Shutdown();
        if (NS_FAILED(rv1)) return rv1;
        if (NS_FAILED(rv2)) return rv2;

    }
    else if (!offline && mOffline) {
        // go online
        if (mDNSService)
            rv1 = mDNSService->Init();
        if (NS_FAILED(rv2)) return rv1;

        if (mSocketTransportService)
            rv2 = mSocketTransportService->Init();		//XXX should we shutdown the dns service?
        if (NS_FAILED(rv2)) return rv1;        
        mOffline = PR_FALSE;    // indicate success only AFTER we've
                                // brought up the services
        // don't care if notification fails
        if (observerService)
            (void)observerService->Notify(this,
                                          NS_LITERAL_STRING("network:offline-status-changed").get(),
                                          NS_LITERAL_STRING("online").get());
    }
    return NS_OK;
}


////////////////////////////////////////////////////////////////////////////////
// URL parsing utilities

/* encode characters into % escaped hexcodes */

/* use the following masks to specify which 
   part of an URL you want to escape: 

   url_Scheme        =     1
   url_Username      =     2
   url_Password      =     4
   url_Host          =     8
   url_Directory     =    16
   url_FileBaseName  =    32
   url_FileExtension =    64
   url_Param         =   128
   url_Query         =   256
   url_Ref           =   512
*/

/* by default this function will not escape parts of a string
   that already look escaped, which means it already includes 
   a valid hexcode. This is done to avoid multiple escapes of
   a string. Use the following mask to force escaping of a 
   string:
 
   url_Forced        =  1024
*/
NS_IMETHODIMP
nsIOService::Escape(const char *str, PRInt16 mask, char** result)
{
    nsCAutoString esc_str;
    nsresult rv = nsURLEscape((char*)str,mask,esc_str);
    CRTFREEIF(*result);
    if (NS_FAILED(rv))
        return rv;
    *result = esc_str.ToNewCString();
    if (!*result)
        return NS_ERROR_OUT_OF_MEMORY;
    return NS_OK;
}

NS_IMETHODIMP
nsIOService::Unescape(const char *str, char **result)
{
    return nsURLUnescape((char*)str,result);
}

NS_IMETHODIMP
nsIOService::ExtractPort(const char *str, PRInt32 *result)
{
    PRInt32 returnValue = -1;
    *result = (0 < PR_sscanf(str, "%d", &returnValue)) ? returnValue : -1;
    return NS_OK;
}

NS_IMETHODIMP
nsIOService::ResolveRelativePath(const char *relativePath, const char* basePath,
                                 char **result)
{
    nsCAutoString name;
    nsCAutoString path(basePath);
	PRBool needsDelim = PR_FALSE;

	if ( !path.IsEmpty() ) {
		PRUnichar last = path.Last();
		needsDelim = !(last == '/' || last == '\\' );
	}

    PRBool end = PR_FALSE;
    char c;
    while (!end) {
        c = *relativePath++;
        switch (c) {
          case '\0':
          case '#':
          case ';':
          case '?':
            end = PR_TRUE;
            // fall through...
          case '/':
          case '\\':
            // delimiter found
            if (name.Equals("..")) {
                // pop path
                PRInt32 pos = path.RFind("/");
                if (pos > 0) {
                    path.Truncate(pos + 1);
                    path += name;
                }
                else {
                    return NS_ERROR_MALFORMED_URI;
                }
            }
            else if (name.Equals(".") || name.Equals("")) {
                // do nothing
            }
            else {
                // append name to path
                if (needsDelim)
                    path += "/";
                path += name;
                needsDelim = PR_TRUE;
            }
            name = "";
            break;

          default:
            // append char to name
            name += c;
        }
    }
    // append anything left on relativePath (e.g. #..., ;..., ?...)
    if (c != '\0')
        path += --relativePath;

    *result = path.ToNewCString();
    return NS_OK;
}

////////////////////////////////////////////////////////////////////////////////
