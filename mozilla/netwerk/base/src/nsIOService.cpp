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
#include "nsString2.h"
#include "nsIServiceManager.h"
#include "nsIEventQueueService.h"
#include "nsIFileTransportService.h"
#include "nsIURI.h"
#include "nsIStreamListener.h"
#include "prprf.h"
#include "prmem.h"      // for PR_Malloc
#include "prsystem.h"   // for PR_GetSystemInfo
#include "nsLoadGroup.h"
#include "nsInputStreamChannel.h"
#include "nsXPIDLString.h" 

static NS_DEFINE_CID(kFileTransportService, NS_FILETRANSPORTSERVICE_CID);
static NS_DEFINE_CID(kEventQueueService, NS_EVENTQUEUESERVICE_CID);
static NS_DEFINE_CID(kSocketTransportServiceCID, NS_SOCKETTRANSPORTSERVICE_CID);
static NS_DEFINE_CID(kDNSServiceCID, NS_DNSSERVICE_CID);

////////////////////////////////////////////////////////////////////////////////

nsIOService::nsIOService()
    : mAppName(nsnull),
      mAppCodeName(nsnull),
      mAppVersion(nsnull),
      mAppLanguage(nsnull),
      mAppPlatform(nsnull),
      mOffline(PR_FALSE)
{
    NS_INIT_REFCNT();
}

nsresult
nsIOService::Init()
{
    nsresult rv;

    // initialize the version and app components
    mAppName = new nsCString("Netscape");
    if (!mAppName) return NS_ERROR_OUT_OF_MEMORY;
    mAppCodeName = new nsCString("Mozilla");
    if (!mAppCodeName) return NS_ERROR_OUT_OF_MEMORY;
    mAppVersion = new nsCString();
    if (!mAppVersion) return NS_ERROR_OUT_OF_MEMORY;
    mAppLanguage = new nsCString("en-US");
    if (!mAppLanguage) return NS_ERROR_OUT_OF_MEMORY;

    char platformBuf[SYS_INFO_BUFFER_LENGTH];
    PRStatus status = PR_GetSystemInfo(PR_SI_SYSNAME, platformBuf, sizeof(char) * SYS_INFO_BUFFER_LENGTH);
    if (PR_FAILURE == status)
        return NS_ERROR_FAILURE;

    mAppPlatform = new nsCString(platformBuf);
    if (!mAppPlatform) return NS_ERROR_OUT_OF_MEMORY;

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
    return rv;
}

nsIOService::~nsIOService()
{
    nsresult rv;
    rv = SetOffline(PR_TRUE);
    NS_ASSERTION(NS_SUCCEEDED(rv), "SetOffline failed");
    // shut down the file transport service too:
    rv = mFileTransportService->Shutdown();
    NS_ASSERTION(NS_SUCCEEDED(rv), "file transport shutdown failed");
    if (mAppName) delete mAppName;
    if (mAppCodeName) delete mAppCodeName;
    if (mAppVersion) delete mAppVersion;
    if (mAppLanguage) delete mAppLanguage;
    if (mAppPlatform) delete mAppPlatform;
}

NS_METHOD
nsIOService::Create(nsISupports *aOuter, REFNSIID aIID, void **aResult)
{
    nsresult rv;
    if (aOuter)
        return NS_ERROR_NO_AGGREGATION;

    nsIOService* _ios = new nsIOService();
    if (_ios == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(_ios);
    rv = _ios->Init();
    if (NS_FAILED(rv)) {
        delete _ios;
        return rv;
    }
    rv = _ios->QueryInterface(aIID, aResult);
    NS_RELEASE(_ios);
    return rv;
}

NS_IMPL_ISUPPORTS(nsIOService, NS_GET_IID(nsIIOService));

////////////////////////////////////////////////////////////////////////////////

#define MAX_SCHEME_LENGTH       64      // XXX big enough?

#define MAX_NET_PROGID_LENGTH   (MAX_SCHEME_LENGTH + NS_NETWORK_PROTOCOL_PROGID_PREFIX_LENGTH + 1)

NS_IMETHODIMP
nsIOService::GetProtocolHandler(const char* scheme, nsIProtocolHandler* *result)
{
    nsresult rv;

    NS_ASSERTION(NS_NETWORK_PROTOCOL_PROGID_PREFIX_LENGTH
                 == nsCRT::strlen(NS_NETWORK_PROTOCOL_PROGID_PREFIX),
                 "need to fix NS_NETWORK_PROTOCOL_PROGID_PREFIX_LENGTH");

    // XXX we may want to speed this up by introducing our own protocol 
    // scheme -> protocol handler mapping, avoiding the string manipulation
    // and service manager stuff

    char buf[MAX_NET_PROGID_LENGTH];
    nsCAutoString progID(NS_NETWORK_PROTOCOL_PROGID_PREFIX);
    progID += scheme;
    progID.ToCString(buf, MAX_NET_PROGID_LENGTH);

    rv = nsServiceManager::GetService(buf, NS_GET_IID(nsIProtocolHandler), (nsISupports **)result);
    if (NS_FAILED(rv)) 
        return NS_ERROR_UNKNOWN_PROTOCOL;

    return NS_OK;
}

NS_IMETHODIMP
nsIOService::ExtractScheme(const char* inURI, PRUint32 *startPos, PRUint32 *endPos,
                           char* *scheme)
{
    // search for something up to a colon, and call it the scheme
    NS_ASSERTION(inURI, "null pointer");
    if (!inURI) return NS_ERROR_NULL_POINTER;

    const char* uri = inURI;

    // skip leading white space
    while (nsString::IsSpace(*uri))
        uri++;

    PRUint32 start = uri - inURI;
    if (startPos) {
        *startPos = start;
    }

    PRUint32 length = 0;
    char c;
    while ((c = *uri++) != '\0') {
        if (c == ':') {
            if (endPos) {
                *endPos = start + length + 1;
            }

            if (scheme) {
                char* str = (char*)nsAllocator::Alloc(length + 1);
                if (str == nsnull)
                    return NS_ERROR_OUT_OF_MEMORY;
                nsCRT::memcpy(str, &inURI[start], length);
                str[length] = '\0';
                *scheme = str;
            }
            return NS_OK;
        }
        else if (nsString::IsAlpha(c)) {
            length++;
        }
        else 
            break;
    }
    return NS_ERROR_MALFORMED_URI;
}

nsresult
nsIOService::NewURI(const char* aSpec, nsIURI* aBaseURI,
                    nsIURI* *result, nsIProtocolHandler* *hdlrResult)
{
    nsresult rv;
    nsIURI* base;
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

    if (hdlrResult) {
        *hdlrResult = handler;
        NS_ADDREF(*hdlrResult);
    }
    return handler->NewURI(aSpec, base, result);
}

NS_IMETHODIMP
nsIOService::NewURI(const char* aSpec, nsIURI* aBaseURI,
                    nsIURI* *result)
{
    return NewURI(aSpec, aBaseURI, result, nsnull);
}

NS_IMETHODIMP
nsIOService::NewChannelFromURI(const char* verb, nsIURI *aURI,
                               nsILoadGroup *aGroup,
                               nsIInterfaceRequestor* notificationCallbacks,
                               nsLoadFlags loadAttributes,
                               nsIURI* originalURI,
                               PRUint32 bufferSegmentSize,
                               PRUint32 bufferMaxSize,
                               nsIChannel **result)
{
    nsresult rv;

    nsXPIDLCString scheme;
    rv = aURI->GetScheme(getter_Copies(scheme));
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIProtocolHandler> handler;
    rv = GetProtocolHandler((const char*)scheme, getter_AddRefs(handler));
    if (NS_FAILED(rv)) return rv;

    rv = handler->NewChannel(verb, aURI, aGroup, notificationCallbacks,
                             loadAttributes, originalURI, 
                             bufferSegmentSize, bufferMaxSize, result);
    return rv;
}

NS_IMETHODIMP
nsIOService::NewChannel(const char* verb, const char *aSpec,
                        nsIURI *aBaseURI,
                        nsILoadGroup *aGroup,
                        nsIInterfaceRequestor* notificationCallbacks,
                        nsLoadFlags loadAttributes,
                        nsIURI* originalURI,
                        PRUint32 bufferSegmentSize,
                        PRUint32 bufferMaxSize,
                        nsIChannel **result)
{
    nsresult rv;
    nsCOMPtr<nsIURI> uri;
    nsCOMPtr<nsIProtocolHandler> handler;
    rv = NewURI(aSpec, aBaseURI, getter_AddRefs(uri), getter_AddRefs(handler));
    if (NS_FAILED(rv)) return rv;

    rv = handler->NewChannel(verb, uri, aGroup, notificationCallbacks,
                             loadAttributes, originalURI, 
                             bufferSegmentSize, bufferMaxSize, result);
    return rv;
}

NS_IMETHODIMP
nsIOService::GetAppCodeName(PRUnichar* *aAppCodeName)
{
    *aAppCodeName = mAppCodeName->ToNewUnicode();
    return NS_OK;
}

NS_IMETHODIMP
nsIOService::GetAppVersion(PRUnichar* *aAppVersion)
{
    *aAppVersion = mAppVersion->ToNewUnicode();
    return NS_OK;
}

// This guy needs to be called each time one of it's comprising pieces changes.
nsresult
nsIOService::BuildAppVersion() {
    // build up the app version
    mAppVersion->Truncate();
    mAppVersion->Append("5.0 [");
    mAppVersion->Append(*mAppLanguage);
    mAppVersion->Append("] (");
    mAppVersion->Append(*mAppPlatform);
    mAppVersion->Append("; I)");
    return NS_OK;
}

NS_IMETHODIMP
nsIOService::GetAppName(PRUnichar* *aAppName)
{
    *aAppName = mAppName->ToNewUnicode();
    return NS_OK;
}

NS_IMETHODIMP
nsIOService::GetLanguage(PRUnichar* *aLanguage)
{
    *aLanguage = mAppLanguage->ToNewUnicode();
    return NS_OK;
}

NS_IMETHODIMP
nsIOService::SetLanguage(const PRUnichar* aLanguage)
{
    nsCString lang(aLanguage);
    mAppLanguage->SetString(lang);
    return BuildAppVersion();
}

NS_IMETHODIMP
nsIOService::GetPlatform(PRUnichar* *aPlatform)
{
    *aPlatform = mAppPlatform->ToNewUnicode();
    return NS_OK;
}

NS_IMETHODIMP
nsIOService::GetUserAgent(PRUnichar* *aUserAgent)
{
    char buf[200];
    PR_snprintf(buf, 200, "%.100s/%.90s", mAppCodeName->GetBuffer(), mAppVersion->GetBuffer());
    nsAutoString2 aUA(buf);
    *aUserAgent = aUA.ToNewUnicode();
    return NS_OK;
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
    mOffline = offline;
    if (offline) {
        // be sure to try and shutdown both (even if the first fails)
        nsresult rv1 = mSocketTransportService->Shutdown();
        nsresult rv2 = mDNSService->Shutdown();
        if (NS_FAILED(rv1)) return rv1;
        if (NS_FAILED(rv2)) return rv2;
    }
    return NS_OK;
}

////////////////////////////////////////////////////////////////////////////////
// URL parsing utilities

NS_IMETHODIMP
nsIOService::Escape(const char *str, PRInt16 mask, char **result)
{
    // XXX Andreas
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsIOService::Unescape(const char *str, char **result)
{
    // XXX Andreas
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsIOService::ExtractPort(const char *str, PRInt32 *result)
{
    // XXX Andreas
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsIOService::ResolveRelativePath(const char *relativePath, const char* basePath,
                                 char **result)
{
    nsCAutoString name;
    nsCAutoString path(basePath);

    PRUnichar last = path.Last();
    PRBool needsDelim = !(last == '/' || last == '\\' || last == '\0');

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
