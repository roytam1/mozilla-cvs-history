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
 * Copyright (C) 1998-2000 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 * Norris Boyd
 * Mitch Stoltz
 * Steve Morse
 */
#include "nsScriptSecurityManager.h"
#include "nsIServiceManager.h"
#include "nsIScriptObjectOwner.h"
#include "nsIURL.h"
#include "nsIJARURI.h"
#include "nspr.h"
#include "plstr.h"
#include "nsCOMPtr.h"
#include "nsJSPrincipals.h"
#include "nsSystemPrincipal.h"
#include "nsCodebasePrincipal.h"
#include "nsCertificatePrincipal.h"
#include "nsAggregatePrincipal.h"
#include "nsCRT.h"
#include "nsXPIDLString.h"
#include "nsIJSContextStack.h"
#include "nsDOMError.h"
#include "xpcexception.h"
#include "nsDOMCID.h"
#include "jsdbgapi.h"
#include "nsIXPConnect.h"
#include "nsIXPCSecurityManager.h"
#include "nsTextFormatter.h"
#include "nsIIOService.h"
#include "nsIStringBundle.h"
#include "nsNetUtil.h"
#include "nsDirectoryService.h"
#include "nsDirectoryServiceDefs.h"
#include "nsIFile.h"
#include "nsIZipReader.h"
#include "nsIJAR.h"
#include "nsIPluginInstance.h"
#include "nsIXPConnect.h"
#include "nsIScriptGlobalObject.h"
#include "nsIDOMWindowInternal.h"
#include "nscore.h"
#include "nsIDocShell.h"
#include "nsIDocShellTreeItem.h"
#include "nsIPrompt.h"
#include "nsIWindowWatcher.h"
#include "nsIConsoleService.h"

static NS_DEFINE_IID(kIIOServiceIID, NS_IIOSERVICE_IID);
static NS_DEFINE_CID(kIOServiceCID, NS_IOSERVICE_CID);
static NS_DEFINE_IID(kIStringBundleServiceIID, NS_ISTRINGBUNDLESERVICE_IID);
static NS_DEFINE_IID(kStringBundleServiceCID, NS_STRINGBUNDLESERVICE_CID);
static NS_DEFINE_CID(kPrefServiceCID, NS_PREF_CID);
static NS_DEFINE_CID(kCScriptNameSetRegistryCID, 
                     NS_SCRIPT_NAMESET_REGISTRY_CID);
static NS_DEFINE_CID(kZipReaderCID, NS_ZIPREADER_CID);

enum {
    SCRIPT_SECURITY_UNDEFINED_ACCESS,
    SCRIPT_SECURITY_CAPABILITY_ONLY,
    SCRIPT_SECURITY_SAME_ORIGIN_ACCESS,
    SCRIPT_SECURITY_ALL_ACCESS,
    SCRIPT_SECURITY_NO_ACCESS
};

// Convinience method to get the current js context stack.
// Uses cached JSContextStack service instead of calling through
// to the service manager.
JSContext *
nsScriptSecurityManager::GetCurrentContextQuick() {
    // Get JSContext from stack.
    nsresult rv;
    if (!mThreadJSContextStack) {
        mThreadJSContextStack = do_GetService("@mozilla.org/js/xpc/ContextStack;1", &rv);
    }
    if (!mThreadJSContextStack)
        return nsnull;
    JSContext *cx;
    if (NS_FAILED(mThreadJSContextStack->Peek(&cx)))
        return nsnull;
    return cx;
}

/*
static nsDOMProp 
findDomProp(const char *propName, int n);

inline PRBool
GetBit(unsigned char *bitVector, PRInt32 index) 
{
    unsigned char c = bitVector[index >> 3];
    c &= (1 << (index & 7));
    return c != 0;
}

inline void
SetBit(unsigned char *bitVector, PRInt32 index) 
{
    bitVector[index >> 3] |= (1 << (index & 7));
}
*/

/////////////////////////////
// nsScriptSecurityManager //
/////////////////////////////

////////////////////////////////////
// Methods implementing ISupports //
////////////////////////////////////

NS_IMPL_THREADSAFE_ISUPPORTS2(nsScriptSecurityManager,
                   nsIScriptSecurityManager,
                   nsIXPCSecurityManager)




///////////////////////////////////////////////////
// Methods implementing nsIScriptSecurityManager //
///////////////////////////////////////////////////

///////////////// Security Checks //////////////////
NS_IMETHODIMP
nsScriptSecurityManager::CheckScriptAccessToURL(JSContext *cx, 
                                                const char* aObjUrlStr, PRInt32 domPropInt, 
                                                PRBool isWrite)
{
    return CheckScriptAccessInternal(cx, nsnull, aObjUrlStr, domPropInt, isWrite);
}

NS_IMETHODIMP
nsScriptSecurityManager::CheckScriptAccess(JSContext *cx, 
                                           void *aObj, PRInt32 domPropInt, 
                                           PRBool isWrite)
{
    return CheckScriptAccessInternal(cx, aObj, nsnull, domPropInt, isWrite);
}


nsresult
nsScriptSecurityManager::CheckScriptAccessInternal(JSContext *cx, 
                                                   void* aObj, const char* aObjUrlStr, 
                                                   PRInt32 domPropInt, PRBool isWrite)
{
    NS_WARNING("Shouldn't be calling CheckScriptAccess anymore - please change this call");
    return NS_OK;
}

nsresult
nsScriptSecurityManager::CheckSameOrigin(JSContext *aCx, nsIPrincipal* aSubject, 
                                         nsIPrincipal* aObject, PRUint32 aAction)
{
    /*
    ** Get origin of subject and object and compare.
    */
    if (aSubject == aObject)
    {
        return NS_OK;
    }
    PRBool isSameOrigin = PR_FALSE;
    if (NS_FAILED(aSubject->Equals(aObject, &isSameOrigin)))
        return NS_ERROR_FAILURE;
    
    if (isSameOrigin)
	{
        return NS_OK;
	}

    // Allow access to about:blank
    nsCOMPtr<nsICodebasePrincipal> objectCodebase = do_QueryInterface(aObject);
    if (objectCodebase)
    {
        nsXPIDLCString origin;
        if (NS_FAILED(objectCodebase->GetOrigin(getter_Copies(origin))))
            return NS_ERROR_FAILURE;
        if (nsCRT::strcasecmp(origin, "about:blank") == 0)
            return NS_OK;
    }

    /*
    ** If we failed the origin tests it still might be the case that we
    ** are a signed script and have permissions to do this operation.
    ** Check for that here
    */
    PRBool capabilityEnabled = PR_FALSE;
    const char* cap = aAction == nsIXPCSecurityManager::ACCESS_SET_PROPERTY ?
                      "UniversalBrowserWrite" : "UniversalBrowserRead";
    if (NS_FAILED(IsCapabilityEnabled(cap, &capabilityEnabled)))
        return NS_ERROR_FAILURE;
    if (capabilityEnabled)
        return NS_OK;

    /*
    ** Access tests failed, so now report error.
    */
    return NS_ERROR_DOM_PROP_ACCESS_DENIED;
}

PRInt32 
nsScriptSecurityManager::GetSecurityLevel(nsIPrincipal *principal,
                                          nsCString &property,
                                          PRUint32 aAction,
                                          nsCString &capability)
{
    //-- Get the source and target schemes
    //-- jar URIs can be nested. This loop finds the innermost base URI.
    nsCAutoString prefName;
    if (NS_FAILED(GetPrefName(principal, property, prefName)))
        return SCRIPT_SECURITY_NO_ACCESS;
    PRInt32 secLevel;
    char *secLevelString;
       nsresult rv;
    rv = mSecurityPrefs->SecurityGetCharPref(prefName, &secLevelString);
    if (NS_FAILED(rv)) {
        prefName += (aAction == nsIXPCSecurityManager::ACCESS_SET_PROPERTY ? ".set" : ".get");
        rv = mSecurityPrefs->SecurityGetCharPref(prefName, &secLevelString);
    }
    if (NS_SUCCEEDED(rv) && secLevelString) {
        if (PL_strcmp(secLevelString, "sameOrigin") == 0)
            secLevel = SCRIPT_SECURITY_SAME_ORIGIN_ACCESS;
        else if (PL_strcmp(secLevelString, "allAccess") == 0)
            secLevel = SCRIPT_SECURITY_ALL_ACCESS;
        else if (PL_strcmp(secLevelString, "noAccess") == 0)
            secLevel = SCRIPT_SECURITY_NO_ACCESS;
        else {
            // string should be the name of a capability
            capability = secLevelString;
            secLevelString = nsnull;
            secLevel = SCRIPT_SECURITY_CAPABILITY_ONLY;
        }
        if (secLevelString)
            PR_Free(secLevelString);
        return secLevel;
    }
    return SCRIPT_SECURITY_UNDEFINED_ACCESS;
}

/*
nsresult
nsScriptSecurityManager::CheckXPCPermissions(JSContext *aJSContext,
                                             nsISupports* aObj)
{
    NS_ASSERTION(mPrefs,"nsScriptSecurityManager::mPrefs not initialized");
    PRBool ok = PR_FALSE;
    if (NS_FAILED(IsCapabilityEnabled("UniversalXPConnect", &ok)))
        ok = PR_FALSE;
    if (!ok) {
        //-- If user allows scripting of plugins by untrusted scripts, 
        //   and the target object is a plugin, allow the access anyway.
        if(aObj)
        {
            nsresult rv;
            nsCOMPtr<nsIPluginInstance> plugin = do_QueryInterface(aObj, &rv);
            if (NS_SUCCEEDED(rv))
            {
                PRBool allow = PR_FALSE;
                //XXX May want to store the value of the pref in a local,
                //    this will help performance when dealing with plugins.
                rv = mPrefs->GetBoolPref("security.xpconnect.plugin.unrestricted", &allow);
                if (NS_SUCCEEDED(rv) && allow) 
                    return NS_OK;
            }
        }
        return NS_ERROR_DOM_XPCONNECT_ACCESS_DENIED;
    }
    return NS_OK;
}
*/

struct nsDomainEntry {
    nsDomainEntry(const char *anOrigin, const char *aPolicy, 
                  int aPolicyLength) 
        : mNext(nsnull), mOrigin(anOrigin), mPolicy(aPolicy, aPolicyLength)
    { }
    PRBool Matches(const char *anOrigin) {
        int len = nsCRT::strlen(anOrigin);
        int thisLen = mOrigin.Length();
        if (len < thisLen)
            return PR_FALSE;
        if (mOrigin.RFindChar(':', PR_FALSE, thisLen-1, 1) != -1)
        //-- Policy applies to all URLs of this scheme, compare scheme only
            return mOrigin.EqualsWithConversion(anOrigin, PR_TRUE, thisLen);

        //-- Policy applies to a particular host; compare scheme://host.domain
        if (!mOrigin.Equals(anOrigin + (len - thisLen)))
            return PR_FALSE;
        if (len == thisLen)
            return PR_TRUE;
        char charBefore = anOrigin[len-thisLen-1];
        return (charBefore == '.' || charBefore == ':' || charBefore == '/');
    }
    nsDomainEntry *mNext;
    nsCString mOrigin;
    nsCString mPolicy;
};

nsresult
nsScriptSecurityManager::GetPrefName(nsIPrincipal *principal, 
                                     nsCString &property, nsCString &result)
{
    static const char *defaultStr = "default";
    result = "capability.policy.";
    {
        PRBool equals = PR_TRUE;
        if (principal && NS_FAILED(principal->Equals(mSystemPrincipal, &equals)))
            return NS_ERROR_FAILURE;
        if (equals) {
            result += defaultStr;
        } else {
            nsCOMPtr<nsICodebasePrincipal> codebase = do_QueryInterface(principal);
            if (!codebase)
                return NS_ERROR_FAILURE;
            nsresult rv;
            nsXPIDLCString origin;
            if (NS_FAILED(rv = codebase->GetOrigin(getter_Copies(origin))))
                return rv;
            nsCString *policy = nsnull;
            if (mOriginToPolicyMap) {
                const char *s = origin;
                const char *nextToLastDot = nsnull;
                const char *lastDot = nsnull;
                const char *colon = nsnull;
                const char *p = s;
                while (*p) {
                    if (*p == '.') {
                        nextToLastDot = lastDot;
                        lastDot = p;
                    }
                    if (!colon && *p == ':')
                        colon = p;
                    p++;
                }
                nsCStringKey key(nextToLastDot ? nextToLastDot+1 : s);
                nsDomainEntry *de = (nsDomainEntry *) mOriginToPolicyMap->Get(&key);
                if (!de)
                {
                    nsCAutoString scheme(s, colon-s+1);
                    nsCStringKey schemeKey(scheme);
                    de = (nsDomainEntry *) mOriginToPolicyMap->Get(&schemeKey);
                }
                while (de) {
                    if (de->Matches(s)) {
                        policy = &de->mPolicy;
                        break;
                    }
                    de = de->mNext;
                }
            }
            if (policy)
                result += property;
            else
                result += defaultStr;
        }
    }
    result += '.';
    result += property;
    return NS_OK;
}

NS_IMETHODIMP
nsScriptSecurityManager::CheckLoadURIFromScript(JSContext *cx, nsIURI *aURI)
{
    // Get a context if necessary
    if (!cx)
    {
        cx = GetCurrentContextQuick();
        if (!cx)
            return NS_OK; // No JS context, so allow the load
    }

    // Get principal of currently executing script.
    nsCOMPtr<nsIPrincipal> principal;
    if (NS_FAILED(GetSubjectPrincipal(cx, getter_AddRefs(principal)))) {
        return NS_ERROR_FAILURE;
    }

    // Native code can load all URIs.
    if (!principal) 
        return NS_OK;

    // The system principal can load all URIs.
    PRBool equals = PR_FALSE;
    if (NS_FAILED(principal->Equals(mSystemPrincipal, &equals)))
        return NS_ERROR_FAILURE;
    if (equals)
        return NS_OK;

    // Otherwise, principal should have a codebase that we can use to
    // do the remaining tests.
    nsCOMPtr<nsICodebasePrincipal> codebase = do_QueryInterface(principal);
    if (!codebase) 
        return NS_ERROR_FAILURE;
    nsCOMPtr<nsIURI> uri;
    if (NS_FAILED(codebase->GetURI(getter_AddRefs(uri)))) 
        return NS_ERROR_FAILURE;
    if (NS_SUCCEEDED(CheckLoadURI(uri, aURI, nsIScriptSecurityManager::STANDARD )))
        return NS_OK;

    // See if we're attempting to load a file: URI. If so, let a 
    // UniversalFileRead capability trump the above check.
    PRBool isFile = PR_FALSE;
    PRBool isRes = PR_FALSE;
    if (NS_FAILED(aURI->SchemeIs("file", &isFile)) || 
        NS_FAILED(aURI->SchemeIs("resource", &isRes)))
        return NS_ERROR_FAILURE;
    if (isFile || isRes)
    {
        PRBool enabled;
        if (NS_FAILED(IsCapabilityEnabled("UniversalFileRead", &enabled)))
            return NS_ERROR_FAILURE;
        if (enabled)
            return NS_OK;
    }

    // Report error.
    nsXPIDLCString spec;
    if (NS_FAILED(aURI->GetSpec(getter_Copies(spec))))
        return NS_ERROR_FAILURE;
	JS_ReportError(cx, "illegal URL method '%s'", (const char *)spec);
    return NS_ERROR_DOM_BAD_URI;
}

NS_IMETHODIMP
nsScriptSecurityManager::CheckLoadURI(nsIURI *aSourceURI, nsIURI *aTargetURI,
                                      PRUint32 aFlags)
{
    nsCOMPtr<nsIJARURI> jarURI;
    nsCOMPtr<nsIURI> sourceUri(aSourceURI);
    while((jarURI = do_QueryInterface(sourceUri)))
        jarURI->GetJARFile(getter_AddRefs(sourceUri));
    if (!sourceUri) return NS_ERROR_FAILURE;

    nsXPIDLCString sourceScheme;
    if (NS_FAILED(sourceUri->GetScheme(getter_Copies(sourceScheme))))
        return NS_ERROR_FAILURE;

    // Some loads are not allowed from mail/news messages
    if ((aFlags & nsIScriptSecurityManager::DISALLOW_FROM_MAIL) && 
        (nsCRT::strcasecmp(sourceScheme, "mailbox")  == 0 ||
         nsCRT::strcasecmp(sourceScheme, "imap")     == 0 ||
         nsCRT::strcasecmp(sourceScheme, "news")     == 0))
    {
        return NS_ERROR_DOM_BAD_URI;
    }

    nsCOMPtr<nsIURI> targetUri(aTargetURI);
    while((jarURI = do_QueryInterface(targetUri)))
        jarURI->GetJARFile(getter_AddRefs(targetUri));
    if (!targetUri) return NS_ERROR_FAILURE;

    nsXPIDLCString targetScheme;
    if (NS_FAILED(targetUri->GetScheme(getter_Copies(targetScheme))))
        return NS_ERROR_FAILURE;
    
    if (nsCRT::strcasecmp(targetScheme, sourceScheme) == 0)
    {
        // every scheme can access another URI from the same scheme
        return NS_OK;
    }

    enum Action { AllowProtocol, DenyProtocol, PrefControlled, ChromeProtocol, AboutProtocol };
    static const struct { 
        const char *name;
        Action action;
    } protocolList[] = {
        //-- Keep the most commonly used protocols at the top of the list
        //   to increase performance
        { "http",            AllowProtocol  },
        { "file",            PrefControlled },
        { "https",           AllowProtocol  },
        { "chrome",          ChromeProtocol },
        { "mailbox",         DenyProtocol   },
        { "pop",             AllowProtocol  },
        { "imap",            DenyProtocol   },
        { "pop3",            DenyProtocol   },
        { "news",            AllowProtocol  },
        { "javascript",      AllowProtocol  },
        { "ftp",             AllowProtocol  },
        { "about",           AboutProtocol  },
        { "mailto",          AllowProtocol  },
        { "aim",             AllowProtocol  },
        { "data",            AllowProtocol  },
        { "keyword",         DenyProtocol   },
        { "resource",        DenyProtocol   },
        { "gopher",          AllowProtocol  },
        { "datetime",        DenyProtocol   },
        { "finger",          AllowProtocol  },
        { "res",             DenyProtocol   }
    };

    nsXPIDLCString targetSpec;
    for (unsigned i=0; i < sizeof(protocolList)/sizeof(protocolList[0]); i++) {
        if (nsCRT::strcasecmp(targetScheme, protocolList[i].name) == 0) {
            PRBool doCheck = PR_FALSE;
            switch (protocolList[i].action) {
            case AllowProtocol:
                // everyone can access these schemes.
                return NS_OK;
            case PrefControlled:
                // Allow access if pref is false
                mPrefs->GetBoolPref("security.checkloaduri", &doCheck);
                return doCheck ? ReportErrorToConsole(aTargetURI) : NS_OK;
            case ChromeProtocol:
                return (aFlags & nsIScriptSecurityManager::ALLOW_CHROME) ?
                    NS_OK : ReportErrorToConsole(aTargetURI);
            case AboutProtocol:
                // Allow loading about:blank, otherwise deny
                if(NS_FAILED(targetUri->GetSpec(getter_Copies(targetSpec))))
                    return NS_ERROR_FAILURE;
                return (PL_strcmp(targetSpec, "about:blank") == 0) || 
                       (PL_strcmp(targetSpec, "about:") == 0) ||
                       (PL_strcmp(targetSpec, "about:mozilla") == 0) ?
                    NS_OK : ReportErrorToConsole(aTargetURI);
            case DenyProtocol:
                // Deny access
                return ReportErrorToConsole(aTargetURI);
            }
        }
    }

    // If we reach here, we have an unknown protocol. Warn, but allow.
    // This is risky from a security standpoint, but allows flexibility
    // in installing new protocol handlers after initial ship.
    NS_WARN_IF_FALSE(PR_FALSE, "unknown protocol in nsScriptSecurityManager::CheckLoadURI");

    return NS_OK;
}

nsresult 
nsScriptSecurityManager::ReportErrorToConsole(nsIURI* aTarget)
{
    nsXPIDLCString spec;
    nsresult rv = aTarget->GetSpec(getter_Copies(spec));
    if (NS_FAILED(rv)) return rv;

    nsAutoString msg;
    msg.AssignWithConversion("The link to ");
    msg.AppendWithConversion(spec);
    msg.AppendWithConversion(" was blocked by the security manager.\nRemote content may not link to local content.");
    // Report error in JS console
    nsCOMPtr<nsIConsoleService> console(do_GetService("@mozilla.org/consoleservice;1"));
    if (console)
    {
      PRUnichar* messageUni = msg.ToNewUnicode();
      if (!messageUni)
          return NS_ERROR_FAILURE;
      console->LogStringMessage(messageUni);
      nsMemory::Free(messageUni);
    }
#ifndef DEBUG
    else // If JS console reporting failed, print to stderr.
#endif
    {
      char* messageCstr = msg.ToNewCString();
      if (!messageCstr)
          return NS_ERROR_FAILURE;
      fprintf(stderr, "%s\n", messageCstr);
      PR_Free(messageCstr);
    }
    //-- Always returns an error
    return NS_ERROR_DOM_BAD_URI;
}


NS_IMETHODIMP
nsScriptSecurityManager::CheckLoadURIStr(const char* aSourceURIStr, const char* aTargetURIStr,
                                         PRUint32 aFlags)
{
    nsCOMPtr<nsIURI> source;
    nsresult rv = NS_NewURI(getter_AddRefs(source), aSourceURIStr, nsnull);
    NS_ENSURE_SUCCESS(rv, rv);
    nsCOMPtr<nsIURI> target;
    rv = NS_NewURI(getter_AddRefs(target), aTargetURIStr, nsnull);
    NS_ENSURE_SUCCESS(rv, rv);
    return CheckLoadURI(source, target, aFlags);
}

NS_IMETHODIMP
nsScriptSecurityManager::CheckFunctionAccess(JSContext *aCx, void *aFunObj, 
                                             void *aTargetObj)
{
    //-- This check is called for event handlers
    nsCOMPtr<nsIPrincipal> subject;
    nsresult rv = GetFunctionObjectPrincipal(aCx, (JSObject *)aFunObj, 
                                             getter_AddRefs(subject));
    if (NS_FAILED(rv))
        return rv;

    PRBool isSystem;
    if (NS_SUCCEEDED(subject->Equals(mSystemPrincipal, &isSystem)) && isSystem) 
        // This is the system principal: just allow access
        return NS_OK;

    // Check if the principal the function was compiled under is
    // allowed to execute scripts.
    if (!subject) {
      return NS_ERROR_DOM_SECURITY_ERR;
    }

    PRBool result;
    rv = CanExecuteScripts(aCx, subject, &result);
    if (NS_FAILED(rv)) {
      return rv;
    }
    
    if (!result) {
      return NS_ERROR_DOM_SECURITY_ERR;
    }

    /*
    ** Get origin of subject and object and compare.
    */
    JSObject* obj = (JSObject*)aTargetObj;
    nsCOMPtr<nsIPrincipal> object;
    if (NS_FAILED(GetObjectPrincipal(aCx, obj, getter_AddRefs(object))))
        return NS_ERROR_FAILURE;
    if (subject == object) {
        return NS_OK;
    }

    PRBool isSameOrigin = PR_FALSE;
    if (NS_FAILED(subject->Equals(object, &isSameOrigin)))
        return NS_ERROR_FAILURE;

    if (isSameOrigin)
        return NS_OK;

    // Allow access to about:blank
    nsCOMPtr<nsICodebasePrincipal> objectCodebase = do_QueryInterface(object);
    if (objectCodebase) {
        nsXPIDLCString origin;
        if (NS_FAILED(objectCodebase->GetOrigin(getter_Copies(origin))))
            return NS_ERROR_FAILURE;
        if (nsCRT::strcasecmp(origin, "about:blank") == 0) {
            return NS_OK;
        }
    }

    /*
    ** Access tests failed.  Fail silently without a JS exception.
    */
    return NS_ERROR_DOM_SECURITY_ERR;
}

nsresult
nsScriptSecurityManager::GetRootDocShell(JSContext *cx, nsIDocShell **result)
{
    nsresult rv;
    *result = nsnull;
    nsCOMPtr<nsIDocShell> docshell;
    nsCOMPtr<nsIScriptContext> scriptContext = (nsIScriptContext*)JS_GetContextPrivate(cx);
    if (!scriptContext) return NS_ERROR_FAILURE;
    nsCOMPtr<nsIScriptGlobalObject> globalObject(dont_AddRef(scriptContext->GetGlobalObject()));
    if (!globalObject)  return NS_ERROR_FAILURE;
    rv = globalObject->GetDocShell(getter_AddRefs(docshell));
    if (NS_FAILED(rv)) return rv;
    nsCOMPtr<nsIDocShellTreeItem> docshellTreeItem = do_QueryInterface(docshell, &rv);
    if (NS_FAILED(rv)) return rv;
    nsCOMPtr<nsIDocShellTreeItem> rootItem;
    rv = docshellTreeItem->GetRootTreeItem(getter_AddRefs(rootItem));
    if (NS_FAILED(rv)) return rv;

    return rootItem->QueryInterface(NS_GET_IID(nsIDocShell), (void**)result);
}

NS_IMETHODIMP
nsScriptSecurityManager::CanExecuteScripts(JSContext* cx,
                                           nsIPrincipal *principal,
                                           PRBool *result)
{
    if (principal == mSystemPrincipal) {
        // Even if JavaScript is disabled, we must still execute system scripts
        *result = PR_TRUE;
        return NS_OK;
    }

    //-- See if the current window allows JS execution
    nsCOMPtr<nsIDocShell> docshell;
    nsresult rv;
    rv = GetRootDocShell(cx, getter_AddRefs(docshell));
    if (NS_SUCCEEDED(rv))
    {
        rv = docshell->GetAllowJavascript(result);
        if (NS_FAILED(rv)) return rv;
        if (!*result)
        return NS_OK;
    }

    /*
         There used to be a per-domain policy here, but that functionality
         will hopefully be taken over by nsIContentPolicy. Meantime, it's gone.
    */

    if ((mIsJavaScriptEnabled != mIsMailJavaScriptEnabled) && docshell)
    {
        // Is this script running from mail?
               PRUint32 appType;
        rv = docshell->GetAppType(&appType);
        if (NS_FAILED(rv)) return rv;
        if (appType == nsIDocShell::APP_TYPE_MAIL)
        {
            *result = mIsMailJavaScriptEnabled;
            return NS_OK;
        }
    }
    *result = mIsJavaScriptEnabled;
    return NS_OK;
}

///////////////// Principals /////////////////////// 
NS_IMETHODIMP
nsScriptSecurityManager::GetSubjectPrincipal(nsIPrincipal **result)
{
    JSContext *cx = GetCurrentContextQuick();
    if (!cx) {
        *result = nsnull;
        return NS_OK;
    }
    return GetSubjectPrincipal(cx, result);
}

NS_IMETHODIMP
nsScriptSecurityManager::GetSystemPrincipal(nsIPrincipal **result)
{
    if (!mSystemPrincipal) {
        mSystemPrincipal = new nsSystemPrincipal();
        if (!mSystemPrincipal)
            return NS_ERROR_OUT_OF_MEMORY;
        NS_ADDREF(mSystemPrincipal);
    }
    *result = mSystemPrincipal;
    NS_ADDREF(*result);
    return NS_OK;
}

NS_IMETHODIMP
nsScriptSecurityManager::GetCertificatePrincipal(const char* aCertID,
                                                 nsIPrincipal **result)
{
    nsresult rv;
    //-- Create a certificate principal
    nsCertificatePrincipal *certificate = new nsCertificatePrincipal();
    if (!certificate)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(certificate);
    if (NS_FAILED(certificate->Init(aCertID)))
    {
        NS_RELEASE(certificate);
        return NS_ERROR_FAILURE;
    }
    nsCOMPtr<nsIPrincipal> principal = do_QueryInterface((nsBasePrincipal*)certificate, &rv);
    NS_RELEASE(certificate);
    if (NS_FAILED(rv)) return rv;

    if (mPrincipals) {
        // Check to see if we already have this principal.
        nsIPrincipalKey key(principal);
        nsCOMPtr<nsIPrincipal> fromTable = (nsIPrincipal *) mPrincipals->Get(&key);
        if (fromTable) 
            principal = fromTable;
    }

    //-- Bundle this certificate principal into an aggregate principal
    nsAggregatePrincipal* agg = new nsAggregatePrincipal();
    if (!agg) return NS_ERROR_OUT_OF_MEMORY;
    rv = agg->SetCertificate(principal);
    if (NS_FAILED(rv)) return rv;
    principal = do_QueryInterface((nsBasePrincipal*)agg, &rv);
    if (NS_FAILED(rv)) return rv;

    *result = principal;
    NS_ADDREF(*result);
    return NS_OK;
}

nsresult
nsScriptSecurityManager::CreateCodebasePrincipal(nsIURI* aURI, nsIPrincipal **result)
{
	nsresult rv = NS_OK;
    nsCodebasePrincipal *codebase = new nsCodebasePrincipal();
    if (!codebase)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(codebase);
    if (NS_FAILED(codebase->Init(aURI))) {
        NS_RELEASE(codebase);
        return NS_ERROR_FAILURE;
    }
    rv = CallQueryInterface((nsBasePrincipal*)codebase, result);
    NS_RELEASE(codebase);
    return rv;
}

NS_IMETHODIMP
nsScriptSecurityManager::GetCodebasePrincipal(nsIURI *aURI,
                                              nsIPrincipal **result)
{
	nsresult rv;
	nsCOMPtr<nsIPrincipal> principal;
	rv = CreateCodebasePrincipal(aURI, getter_AddRefs(principal));
	if (NS_FAILED(rv)) return rv;

    if (mPrincipals) {
        //-- Check to see if we already have this principal.
        nsIPrincipalKey key(principal);
        nsCOMPtr<nsIPrincipal> fromTable = (nsIPrincipal *) mPrincipals->Get(&key);
        if (fromTable) 
            principal = fromTable;
		else //-- Check to see if we have a more general principal
		{
			nsCOMPtr<nsICodebasePrincipal> codebasePrin = do_QueryInterface(principal);
			nsXPIDLCString originUrl;
			rv = codebasePrin->GetOrigin(getter_Copies(originUrl));
			if (NS_FAILED(rv)) return rv;
			nsCOMPtr<nsIURI> newURI;
			rv = NS_NewURI(getter_AddRefs(newURI), originUrl, nsnull);
			if (NS_FAILED(rv)) return rv;
			nsCOMPtr<nsIPrincipal> principal2;
			rv = CreateCodebasePrincipal(newURI, getter_AddRefs(principal2));
			if (NS_FAILED(rv)) return rv;
			 nsIPrincipalKey key2(principal2);
				fromTable = (nsIPrincipal *) mPrincipals->Get(&key2);
			if (fromTable) 
				principal = fromTable;
		}		
    }

    //-- Bundle this codebase principal into an aggregate principal
    nsAggregatePrincipal* agg = new nsAggregatePrincipal();
    if (!agg) return NS_ERROR_OUT_OF_MEMORY;
    rv = agg->SetCodebase(principal);
    if (NS_FAILED(rv)) return rv;
    principal = do_QueryInterface((nsBasePrincipal*)agg, &rv);
    if (NS_FAILED(rv)) return rv;

    *result = principal;
    NS_ADDREF(*result);
    return NS_OK;
}

nsresult
nsScriptSecurityManager::GetScriptPrincipal(JSContext *cx, 
                                            JSScript *script,
                                            nsIPrincipal **result) 
{
    if (!script) {
        *result = nsnull;
        return NS_OK;
    }
    JSPrincipals *jsp = JS_GetScriptPrincipals(cx, script);
    if (!jsp) {
        // Script didn't have principals -- shouldn't happen.
        return NS_ERROR_FAILURE;
    }
    nsJSPrincipals *nsJSPrin = NS_STATIC_CAST(nsJSPrincipals *, jsp);
    *result = nsJSPrin->nsIPrincipalPtr;
    if (!result)
        return NS_ERROR_FAILURE;
    NS_ADDREF(*result);
    return NS_OK;

}

nsresult
nsScriptSecurityManager::GetFunctionObjectPrincipal(JSContext *cx, 
                                                    JSObject *obj,
                                                    nsIPrincipal **result) 
{
    JSFunction *fun = (JSFunction *) JS_GetPrivate(cx, obj);
    if (JS_GetFunctionObject(fun) != obj) {
        // Function has been cloned; get principals from scope
        return GetObjectPrincipal(cx, obj, result);
    }
    JSScript *script = JS_GetFunctionScript(cx, fun);
    return GetScriptPrincipal(cx, script, result);
}

nsresult
nsScriptSecurityManager::GetFramePrincipal(JSContext *cx, 
                                           JSStackFrame *fp,
                                           nsIPrincipal **result) 
{
    JSObject *obj = JS_GetFrameFunctionObject(cx, fp);
    if (!obj) {
        // Must be in a top-level script. Get principal from the script.
        JSScript *script = JS_GetFrameScript(cx, fp);
        return GetScriptPrincipal(cx, script, result);
    } 
    return GetFunctionObjectPrincipal(cx, obj, result);
}

nsresult
nsScriptSecurityManager::GetPrincipalAndFrame(JSContext *cx,
                                              PRBool skipInnerFrame,
                                              nsIPrincipal **result, 
                                              JSStackFrame **frameResult) 
{
    // Get principals from innermost frame of JavaScript or Java.
    JSStackFrame *fp = nsnull; // tell JS_FrameIterator to start at innermost
    if (skipInnerFrame) // Skip the innermost frame
    {
        fp = JS_FrameIterator(cx, &fp);
        NS_ASSERTION(fp, "GetPrincipalAndFrame unexpectedly reached bottom of stack");
    }
    for (fp = JS_FrameIterator(cx, &fp); fp; fp = JS_FrameIterator(cx, &fp))
    {
        if (NS_FAILED(GetFramePrincipal(cx, fp, result)))
            return NS_ERROR_FAILURE;
        if (*result)
        {
            *frameResult = fp;
            return NS_OK;
        }
    }

    //-- If there's no principal on the stack, look at the global object
    //   and return the innermost frame for annotations.
    if (cx) 
    {
        nsCOMPtr<nsIScriptContext> scriptContext = 
            NS_REINTERPRET_CAST(nsIScriptContext*,JS_GetContextPrivate(cx));
        if (scriptContext)
        {
            nsCOMPtr<nsIScriptGlobalObject> global = scriptContext->GetGlobalObject();
            NS_ENSURE_TRUE(global, NS_ERROR_FAILURE);
            nsCOMPtr<nsIScriptObjectPrincipal> globalData = do_QueryInterface(global);
            NS_ENSURE_TRUE(globalData, NS_ERROR_FAILURE);
            globalData->GetPrincipal(result);
            if (*result)
            {
                JSStackFrame *inner = nsnull;
                *frameResult = JS_FrameIterator(cx, &inner);
                return NS_OK;
            }
        }
    }   

    *result = nsnull;
    return NS_OK;
}

nsresult
nsScriptSecurityManager::GetSubjectPrincipal(JSContext *cx, 
                                             nsIPrincipal **result)
{
    JSStackFrame *fp;
    return GetPrincipalAndFrame(cx, PR_FALSE, result, &fp);
}

nsresult
nsScriptSecurityManager::GetCallingPrincipal(JSContext *cx, 
                                             nsIPrincipal **result)
{
    JSStackFrame *fp;
    return GetPrincipalAndFrame(cx, PR_TRUE, result, &fp);
}

nsresult
nsScriptSecurityManager::GetObjectPrincipal(JSContext *aCx, JSObject *aObj,
                                            nsIPrincipal **result)
{
    JSObject *parent = aObj;
    do {
        JSClass *jsClass = JS_GetClass(aCx, parent);
        const uint32 privateNsISupports = JSCLASS_HAS_PRIVATE | 
                                          JSCLASS_PRIVATE_IS_NSISUPPORTS;
        if (jsClass && (jsClass->flags & (privateNsISupports)) == 
                            privateNsISupports)
        {
            nsCOMPtr<nsISupports> supports = (nsISupports *) JS_GetPrivate(aCx, parent);
            nsCOMPtr<nsIScriptObjectPrincipal> objPrin = 
                do_QueryInterface(supports);
            if (!objPrin) {
                /*
                 * If it's a wrapped native, check the underlying native
                 * instead.
                 */
                nsCOMPtr<nsIXPConnectWrappedNative> xpcNative = 
                    do_QueryInterface(supports);
                if (xpcNative)
                    xpcNative->GetNative(getter_AddRefs(supports));
                objPrin = do_QueryInterface(supports);
            }

            if (objPrin && NS_SUCCEEDED(objPrin->GetPrincipal(result)))
                return NS_OK;
        }
        parent = JS_GetParent(aCx, parent);
    } while (parent);

    // Couldn't find a principal for this object.
    return NS_ERROR_FAILURE;
}

nsresult
nsScriptSecurityManager::SavePrincipal(nsIPrincipal* aToSave)
{
    NS_ASSERTION(mSecurityPrefs, "nsScriptSecurityManager::mSecurityPrefs not initialized");
    nsresult rv;
    nsCOMPtr<nsIPrincipal> persistent = aToSave;
    nsCOMPtr<nsIAggregatePrincipal> aggregate = do_QueryInterface(aToSave, &rv);
    if (NS_SUCCEEDED(rv))
        if (NS_FAILED(aggregate->GetPrimaryChild(getter_AddRefs(persistent))))
            return NS_ERROR_FAILURE;

    //-- Save to mPrincipals
    if (!mPrincipals) 
    {
        mPrincipals = new nsSupportsHashtable(31);
        if (!mPrincipals)
            return NS_ERROR_OUT_OF_MEMORY;
    }
    nsIPrincipalKey key(persistent);
    mPrincipals->Put(&key, persistent);

    //-- Save to prefs
    nsXPIDLCString idPrefName;
    nsXPIDLCString id;
    nsXPIDLCString grantedList;
    nsXPIDLCString deniedList;
    rv = persistent->GetPreferences(getter_Copies(idPrefName),
                                    getter_Copies(id),
                                    getter_Copies(grantedList),
                                    getter_Copies(deniedList));
    if (NS_FAILED(rv)) return NS_ERROR_FAILURE;

    nsXPIDLCString grantedPrefName;
    nsXPIDLCString deniedPrefName;
    rv = PrincipalPrefNames( idPrefName, 
                             getter_Copies(grantedPrefName), 
                             getter_Copies(deniedPrefName)  );
    if (NS_FAILED(rv)) return NS_ERROR_FAILURE;

    mIsWritingPrefs = PR_TRUE;
    if (grantedList)
        mSecurityPrefs->SecuritySetCharPref(grantedPrefName, grantedList);
    else
        mSecurityPrefs->SecurityClearUserPref(grantedPrefName);

    if (deniedList)
        mSecurityPrefs->SecuritySetCharPref(deniedPrefName, deniedList);
    else
        mSecurityPrefs->SecurityClearUserPref(deniedPrefName);

    if (grantedList || deniedList)
        mSecurityPrefs->SecuritySetCharPref(idPrefName, id);
    else
        mSecurityPrefs->SecurityClearUserPref(idPrefName);

    mIsWritingPrefs = PR_FALSE;
    return mPrefs->SavePrefFile();
}

///////////////// Capabilities API /////////////////////
NS_IMETHODIMP
nsScriptSecurityManager::IsCapabilityEnabled(const char *capability,
                                             PRBool *result)
{
    nsresult rv;
    JSStackFrame *fp = nsnull;
    JSContext *cx = GetCurrentContextQuick();
    fp = cx ? JS_FrameIterator(cx, &fp) : nsnull;
    if (!fp) {
        // No script code on stack. Allow execution.
        *result = PR_TRUE;
        return NS_OK;
    }

    do {
        nsCOMPtr<nsIPrincipal> principal;
        if (NS_FAILED(GetFramePrincipal(cx, fp, getter_AddRefs(principal)))) {
            return NS_ERROR_FAILURE;
        }
        if (!principal)
            continue;

        // First check if the principal is even able to enable the 
        // given capability. If not, don't look any further.
        PRInt16 canEnable;
        rv = principal->CanEnableCapability(capability, &canEnable);
        if (NS_FAILED(rv))
            return rv;
        if (canEnable != nsIPrincipal::ENABLE_GRANTED &&
            canEnable != nsIPrincipal::ENABLE_WITH_USER_PERMISSION) 
        {
            *result = PR_FALSE;
            return NS_OK;
        }

        // Now see if the capability is enabled.
        void *annotation = JS_GetFrameAnnotation(cx, fp);
        rv = principal->IsCapabilityEnabled(capability, annotation, 
                                            result);
        if (NS_FAILED(rv))
            return rv;
        if (*result)
            return NS_OK;
    } while ((fp = JS_FrameIterator(cx, &fp)) != nsnull);
    *result = PR_FALSE;
    return NS_OK;
}

#define PROPERTIES_URL "chrome://communicator/locale/security/security.properties"

nsresult
Localize(char *genericString, nsString &result) 
{
    nsresult ret;
    
    /* create a URL for the string resource file */
    nsIIOService *pNetService = nsnull;
    ret = nsServiceManager::GetService(kIOServiceCID, kIIOServiceIID,
                                       (nsISupports**) &pNetService);
    if (NS_FAILED(ret)) {
        NS_WARNING("cannot get net service\n");
        return ret;
    }
    nsIURI *uri = nsnull;
    ret = pNetService->NewURI(PROPERTIES_URL, nsnull, &uri);
    if (NS_FAILED(ret)) {
        NS_WARNING("cannot create URI\n");
        nsServiceManager::ReleaseService(kIOServiceCID, pNetService);
        return ret;
    }
    
    nsIURI *url = nsnull;
    ret = uri->QueryInterface(NS_GET_IID(nsIURI), (void**)&url);
    nsServiceManager::ReleaseService(kIOServiceCID, pNetService);
    
    if (NS_FAILED(ret)) {
        NS_WARNING("cannot create URL\n");
        return ret;
    }
    
    /* create a bundle for the localization */
    nsIStringBundleService *pStringService = nsnull;
    ret = nsServiceManager::GetService(kStringBundleServiceCID,
        kIStringBundleServiceIID, (nsISupports**) &pStringService);
    if (NS_FAILED(ret)) {
        NS_WARNING("cannot get string service\n");
        return ret;
    }
    char *spec = nsnull;
    ret = url->GetSpec(&spec);
    if (NS_FAILED(ret)) {
        NS_WARNING("cannot get url spec\n");
        nsServiceManager::ReleaseService(kStringBundleServiceCID, pStringService);
        nsCRT::free(spec);
        return ret;
    }
    nsILocale *locale = nsnull;
    nsIStringBundle *bundle = nsnull;
    ret = pStringService->CreateBundle(spec, locale, &bundle);
    nsCRT::free(spec);
    nsServiceManager::ReleaseService(kStringBundleServiceCID, pStringService);
    if (NS_FAILED(ret)) {
        NS_WARNING("cannot create instance\n");
        return ret;
    }
    
    /* localize the given string */
    nsAutoString strtmp;
    strtmp.AssignWithConversion(genericString);

    PRUnichar *ptrv = nsnull;
    ret = bundle->GetStringFromName(strtmp.GetUnicode(), &ptrv);
    NS_RELEASE(bundle);
    if (NS_FAILED(ret)) {
        NS_WARNING("cannot get string from name\n");
    }
    result = ptrv;
    nsCRT::free(ptrv);
    return ret;
}

static PRBool
CheckConfirmDialog(JSContext* cx, const PRUnichar *szMessage, const PRUnichar *szCheckMessage,
                   PRBool *checkValue) 
{
    nsresult res;
    //-- Get a prompter for the current window.
    nsCOMPtr<nsIPrompt> prompter;
    nsCOMPtr<nsIScriptContext> scriptContext = (nsIScriptContext*)JS_GetContextPrivate(cx);
    if (scriptContext)
    {
        nsCOMPtr<nsIScriptGlobalObject> globalObject(dont_AddRef(scriptContext->GetGlobalObject()));
        NS_ASSERTION(globalObject, "script context has no global object");
        nsCOMPtr<nsIDOMWindowInternal> domWin = do_QueryInterface(globalObject);
        if (domWin)
            domWin->GetPrompter(getter_AddRefs(prompter));
    }

    if (!prompter)
    {
        //-- Couldn't get prompter from the current window, so get the prompt service.
        nsCOMPtr<nsIWindowWatcher> wwatch(do_GetService("@mozilla.org/embedcomp/window-watcher;1"));
        if (wwatch)
          wwatch->GetNewPrompter(0, getter_AddRefs(prompter));
    }
    if (!prompter)
    {
        *checkValue = 0;
        return PR_FALSE;
    }
    
    PRInt32 buttonPressed = 1; /* in case user exits dialog by clicking X */
    nsAutoString yes, no, titleline;
    if (NS_FAILED(res = Localize("Yes", yes)))
        return PR_FALSE;
    if (NS_FAILED(res = Localize("No", no)))
        return PR_FALSE;
    if (NS_FAILED(res = Localize("Titleline", titleline)))
        return PR_FALSE;
    
    res = prompter->UniversalDialog(
        nsnull, /* title message */
        titleline.GetUnicode(), /* title text in top line of window */
        szMessage, /* this is the main message */
        szCheckMessage, /* This is the checkbox message */
        yes.GetUnicode(), /* first button text */
        no.GetUnicode(), /* second button text */
        nsnull, /* third button text */
        nsnull, /* fourth button text */
        nsnull, /* first edit field label */
        nsnull, /* second edit field label */
        nsnull, /* first edit field initial and final value */
        nsnull, /* second edit field initial and final value */
        nsnull,  /* icon: question mark by default */
        checkValue, /* initial and final value of checkbox */
        2, /* number of buttons */
        0, /* number of edit fields */
        0, /* is first edit field a password field */
        &buttonPressed);
    
    if (NS_FAILED(res)) {
        *checkValue = 0;
    }
    if (*checkValue != 0 && *checkValue != 1) {
        *checkValue = 0; /* this should never happen but it is happening!!! */
    }
    return (buttonPressed == 0);
}

NS_IMETHODIMP
nsScriptSecurityManager::RequestCapability(nsIPrincipal* aPrincipal, 
                                           const char *capability, PRInt16* canEnable)
{
    if (NS_FAILED(aPrincipal->CanEnableCapability(capability, canEnable)))
        return NS_ERROR_FAILURE;
    if (*canEnable == nsIPrincipal::ENABLE_WITH_USER_PERMISSION) {
        // Prompt user for permission to enable capability.
        static PRBool remember = PR_TRUE;
        nsAutoString query, check;
        if (NS_FAILED(Localize("EnableCapabilityQuery", query)))
            return NS_ERROR_FAILURE;
        if (NS_FAILED(Localize("CheckMessage", check)))
            return NS_ERROR_FAILURE;
        char *source;
        if (NS_FAILED(aPrincipal->ToUserVisibleString(&source)))
            return NS_ERROR_FAILURE;
        PRUnichar *message = nsTextFormatter::smprintf(query.GetUnicode(), source);
        Recycle(source);
        JSContext *cx = GetCurrentContextQuick();
        if (CheckConfirmDialog(cx, message, check.GetUnicode(), &remember))
            *canEnable = nsIPrincipal::ENABLE_GRANTED;
        else
            *canEnable = nsIPrincipal::ENABLE_DENIED;
        PR_FREEIF(message);
        if (remember) {
            //-- Save principal to prefs and to mPrincipals
            if (NS_FAILED(aPrincipal->SetCanEnableCapability(capability, *canEnable)))
                return NS_ERROR_FAILURE;
            if (NS_FAILED(SavePrincipal(aPrincipal)))
                return NS_ERROR_FAILURE;
        }
    }
    return NS_OK;
}

NS_IMETHODIMP
nsScriptSecurityManager::EnableCapability(const char *capability)
{
    JSContext *cx = GetCurrentContextQuick();
    JSStackFrame *fp;

    //Error checks for capability string length (200)
    if(PL_strlen(capability)>200) {
		static const char msg[] = "Capability name too long";
		JS_SetPendingException(cx, STRING_TO_JSVAL(JS_NewStringCopyZ(cx, msg)));
        return NS_ERROR_FAILURE;
    }
    
    nsCOMPtr<nsIPrincipal> principal;
    if (NS_FAILED(GetPrincipalAndFrame(cx, PR_FALSE, getter_AddRefs(principal), 
                                       &fp)))
    {
        return NS_ERROR_FAILURE;
    }
    void *annotation = JS_GetFrameAnnotation(cx, fp);
    PRBool enabled;
    if (NS_FAILED(principal->IsCapabilityEnabled(capability, annotation, 
                                                 &enabled)))
    {
        return NS_ERROR_FAILURE;
    }
    if (enabled)
        return NS_OK;

    PRInt16 canEnable;
    if (NS_FAILED(RequestCapability(principal, capability, &canEnable)))
        return NS_ERROR_FAILURE;
   
    if (canEnable != nsIPrincipal::ENABLE_GRANTED) {
		static const char msg[] = "enablePrivilege not granted";
		JS_SetPendingException(cx, STRING_TO_JSVAL(JS_NewStringCopyZ(cx, msg)));
        return NS_ERROR_FAILURE; // XXX better error code?
    }
    if (NS_FAILED(principal->EnableCapability(capability, &annotation))) 
        return NS_ERROR_FAILURE;
    JS_SetFrameAnnotation(cx, fp, annotation);
    return NS_OK;
}

NS_IMETHODIMP
nsScriptSecurityManager::RevertCapability(const char *capability)
{
    JSContext *cx = GetCurrentContextQuick();
    JSStackFrame *fp;
    nsCOMPtr<nsIPrincipal> principal;
    if (NS_FAILED(GetPrincipalAndFrame(cx, PR_FALSE, getter_AddRefs(principal), 
                                       &fp)))
    {
        return NS_ERROR_FAILURE;
    }
    void *annotation = JS_GetFrameAnnotation(cx, fp);
    principal->RevertCapability(capability, &annotation);
    JS_SetFrameAnnotation(cx, fp, annotation);
    return NS_OK;
}

NS_IMETHODIMP
nsScriptSecurityManager::DisableCapability(const char *capability)
{
    JSContext *cx = GetCurrentContextQuick();
    JSStackFrame *fp;
    nsCOMPtr<nsIPrincipal> principal;
    if (NS_FAILED(GetPrincipalAndFrame(cx, PR_FALSE, getter_AddRefs(principal), 
                                       &fp)))
    {
        return NS_ERROR_FAILURE;
    }
    void *annotation = JS_GetFrameAnnotation(cx, fp);
    principal->DisableCapability(capability, &annotation);
    JS_SetFrameAnnotation(cx, fp, annotation);
    return NS_OK;
}

//////////////// Master Certificate Functions ///////////////////////////////////////
NS_IMETHODIMP
nsScriptSecurityManager::SetCanEnableCapability(const char* certificateID, 
                                                const char* capability,
                                                PRInt16 canEnable)
{
    nsresult rv;
    nsCOMPtr<nsIPrincipal> subjectPrincipal;
    rv = GetSubjectPrincipal(getter_AddRefs(subjectPrincipal));
    if (NS_FAILED(rv)) return NS_ERROR_FAILURE;

    //-- Get the system certificate
    if (!mSystemCertificate)
    {
        nsCOMPtr<nsIFile> systemCertFile;
        NS_WITH_SERVICE(nsIProperties, directoryService, NS_DIRECTORY_SERVICE_CONTRACTID, &rv);
        if (!directoryService) return NS_ERROR_FAILURE;
        rv = directoryService->Get(NS_XPCOM_CURRENT_PROCESS_DIR, NS_GET_IID(nsIFile), 
                              getter_AddRefs(systemCertFile));
        if (NS_FAILED(rv)) return NS_ERROR_FAILURE;
#ifdef XP_MAC
        // On Mac, this file will be located in the Essential Files folder
        systemCertFile->Append("Essential Files");
        if (NS_FAILED(rv)) return NS_ERROR_FAILURE;
#endif
        systemCertFile->Append("systemSignature.jar");
        if (NS_FAILED(rv)) return NS_ERROR_FAILURE;
        nsCOMPtr<nsIZipReader> systemCertZip;
        rv = nsComponentManager::CreateInstance(kZipReaderCID, nsnull, 
                                                NS_GET_IID(nsIZipReader),
                                                getter_AddRefs(systemCertZip));
        if (NS_FAILED(rv)) return NS_ERROR_FAILURE;
        systemCertZip->Init(systemCertFile);
        rv = systemCertZip->Open();
        if (NS_SUCCEEDED(rv))
        {
            nsCOMPtr<nsIJAR> systemCertJar = do_QueryInterface(systemCertZip, &rv);
            if (NS_FAILED(rv)) return NS_ERROR_FAILURE;
            rv = systemCertJar->GetCertificatePrincipal(nsnull, 
                                                        getter_AddRefs(mSystemCertificate));
            if (NS_FAILED(rv)) return NS_ERROR_FAILURE;
        }
    }

    //-- Make sure the caller's principal is the system certificate
    PRBool isEqual = PR_FALSE;
    if (mSystemCertificate)
    {
        rv = mSystemCertificate->Equals(subjectPrincipal, &isEqual);
        if (NS_FAILED(rv)) return NS_ERROR_FAILURE;
    }
    if (!isEqual)
    {
        JSContext* cx = GetCurrentContextQuick();
        if (!cx) return NS_ERROR_FAILURE;
		static const char msg1[] = "Only code signed by the system certificate may call SetCanEnableCapability or Invalidate";
        static const char msg2[] = "Attempt to call SetCanEnableCapability or Invalidate when no system certificate has been established";
            JS_SetPendingException(cx, STRING_TO_JSVAL(JS_NewStringCopyZ(cx, 
                                   mSystemCertificate ? msg1 : msg2)));
        return NS_ERROR_FAILURE;
    }

    //-- Get the target principal
    nsCOMPtr<nsIPrincipal> objectPrincipal;
    rv =  GetCertificatePrincipal(certificateID, getter_AddRefs(objectPrincipal));
    if (NS_FAILED(rv)) return NS_ERROR_FAILURE;
    rv = objectPrincipal->SetCanEnableCapability(capability, canEnable);
    if (NS_FAILED(rv)) return NS_ERROR_FAILURE;
    return SavePrincipal(objectPrincipal);
}

////////////////////////////////////////////////
// Methods implementing nsIXPCSecurityManager //
////////////////////////////////////////////////

NS_IMETHODIMP
nsScriptSecurityManager::CanCreateWrapper(JSContext *aJSContext, 
                                          const nsIID &aIID, 
                                          nsISupports *aObj,
                                          void **aPolicy)
{
#if 0
    char* iidStr = aIID.ToString();
    printf("### CanCreateWrapper(%s) - always OK?\n", iidStr);
    PR_FREEIF(iidStr);
#endif
// Special case for nsIXPCException
    return NS_OK;
}

NS_IMETHODIMP
nsScriptSecurityManager::CanCreateInstance(JSContext *aJSContext, 
                                           const nsCID &aCID)
{
#if 0
    char* cidStr = aCID.ToString();
    printf("### CanCreateInstance(%s) - always OK?\n", cidStr);
    PR_FREEIF(cidStr);
#endif
    return NS_OK;
}

NS_IMETHODIMP
nsScriptSecurityManager::CanGetService(JSContext *aJSContext, 
                                       const nsCID &aCID)
{
#if 0
    char* cidStr = aCID.ToString();
    printf("### CanGetService(%s) - Privileged access only\n", cidStr);
    PR_FREEIF(cidStr);
#endif
    return NS_OK;
}

// Result of this function should not be freed.
static const PRUnichar *
JSValIDToString(JSContext *aJSContext, const jsval idval) {
    JSString *str = JS_ValueToString(aJSContext, idval);
    if(!str)
        return nsnull;
    return NS_REINTERPRET_CAST(PRUnichar*, JS_GetStringChars(str));
}

/* void CanAccess (in PRUint32 aAction, in nsIXPCNativeCallContext aCallContext, in JSContextPtr aJSContext, in JSObjectPtr aJSObject, in nsISupports aObj, in nsIClassInfo aClassInfo, in JSVal aName, inout voidPtr aPolicy); */
NS_IMETHODIMP 
nsScriptSecurityManager::CanAccess(PRUint32 aAction,
                                   nsIXPCNativeCallContext *aCallContext,
                                   JSContext * aJSContext,
                                   JSObject * aJSObject,
                                   nsISupports *aObj,
                                   nsIClassInfo *aClassInfo,
                                   jsval aName,
                                   void * *aPolicy)
{
    nsCOMPtr<nsIPrincipal> subjectPrincipal;
    if (NS_FAILED(GetCallingPrincipal(aJSContext, getter_AddRefs(subjectPrincipal))))
        return NS_ERROR_FAILURE;

    PRBool equals;
    if (!subjectPrincipal || 
        NS_SUCCEEDED(subjectPrincipal->Equals(mSystemPrincipal, &equals)) && equals) 
        // We have native code or the system principal: just allow access
        return NS_OK;

    nsCAutoString propertyStr;
    char* classDescription = nsnull;
    if (aClassInfo)
        aClassInfo->GetClassDescription(&classDescription);
    if(classDescription)
        propertyStr = classDescription;
    else
        propertyStr = "UnknownClass";
    propertyStr += '.';
    propertyStr.AppendWithConversion((PRUnichar*)JSValIDToString(aJSContext, aName));
#ifdef DEBUG_mstoltz
    char* property;
    property = propertyStr.ToNewCString();
    printf("### CanAccess(%s, %i) ", property, aAction);
    PR_FREEIF(property);
#endif
    nsresult rv;
#ifdef DEBUG_mstoltz
    nsCOMPtr<nsIPrincipal> subjectPrincipal2;
    if (NS_FAILED(GetCallingPrincipal(aJSContext, getter_AddRefs(subjectPrincipal2))))
        return NS_ERROR_FAILURE;
#endif

    //-- Look up the policy for this property/method
    nsCAutoString capability;
    PRInt32 secLevel = GetSecurityLevel(subjectPrincipal, propertyStr, aAction, capability);

    //-- If this is a DOM class and there's no specific policy for this property,
    //   then the policy is same-origin.
    if (aClassInfo)
    {
        PRUint32 classFlags;
        NS_ENSURE_SUCCESS(aClassInfo->GetFlags(&classFlags), NS_ERROR_FAILURE);
        if(secLevel == SCRIPT_SECURITY_UNDEFINED_ACCESS  && 
          (classFlags | nsIClassInfo::DOM_OBJECT))
            secLevel = SCRIPT_SECURITY_SAME_ORIGIN_ACCESS;
    }

    switch (secLevel)
    {
    case SCRIPT_SECURITY_ALL_ACCESS:
#ifdef DEBUG_mstoltz
        printf("Level: AllAccess ");
#endif
        rv = NS_OK;
        break;
    case SCRIPT_SECURITY_SAME_ORIGIN_ACCESS:
        {
#ifdef DEBUG_mstoltz
		    printf("Level: SameOrigin ");
#endif
            nsCOMPtr<nsIPrincipal> objectPrincipal;
            if (NS_FAILED(GetObjectPrincipal(aJSContext, NS_REINTERPRET_CAST(JSObject*, aJSObject),
                                             getter_AddRefs(objectPrincipal))))
                return NS_ERROR_FAILURE;
            rv = CheckSameOrigin(aJSContext, subjectPrincipal, objectPrincipal,
                                 aAction == nsIXPCSecurityManager::ACCESS_SET_PROPERTY);
            break;
        }
    case SCRIPT_SECURITY_CAPABILITY_ONLY:
        {
#ifdef DEBUG_mstoltz
		    printf("Level: Capability ");
#endif
            PRBool capabilityEnabled = PR_FALSE;
            nsresult rv = IsCapabilityEnabled(capability, &capabilityEnabled);
            if (NS_FAILED(rv) || !capabilityEnabled)
                rv = NS_ERROR_DOM_SECURITY_ERR;
            else
                rv = NS_OK;
            break;
        }
    default:
        // Default is no access
#ifdef DEBUG_mstoltz
        printf("Level: NoAccess ");
#endif
        rv = NS_ERROR_DOM_SECURITY_ERR;
    }



    if NS_SUCCEEDED(rv)
    {
#ifdef DEBUG_mstoltz
    printf(" GRANTED.\n");
#endif
        return rv;
    }

    //-- Check for carte blanche
    PRBool ok = PR_FALSE;
    if (NS_SUCCEEDED(IsCapabilityEnabled("UniversalXPConnect", &ok)) && ok)
    {
#ifdef DEBUG_mstoltz
        printf("UniversalXPConnect GRANTED.\n");
#endif
        return NS_OK;
    }

    static const char exceptionMsg[] = "Access to property denied";
    JS_SetPendingException(aJSContext, 
                           STRING_TO_JSVAL(JS_NewStringCopyZ(aJSContext, exceptionMsg)));
#ifdef DEBUG_mstoltz
    printf("DENIED.\n");
#endif
    return rv;
}

/////////////////////////////////////////////
// Constructor, Destructor, Initialization //
/////////////////////////////////////////////
nsScriptSecurityManager::nsScriptSecurityManager(void)
    : mOriginToPolicyMap(nsnull),
      mSystemPrincipal(nsnull), mPrincipals(nsnull), 
      mIsJavaScriptEnabled(PR_FALSE),
      mIsMailJavaScriptEnabled(PR_FALSE),
      mIsWritingPrefs(PR_FALSE),
      mNameSetRegistered(PR_FALSE)

{
    NS_INIT_REFCNT();
    memset(hasDomainPolicyVector, 0, sizeof(hasDomainPolicyVector));
    InitPrefs();
    mThreadJSContextStack = do_GetService("@mozilla.org/js/xpc/ContextStack;1");
}

nsScriptSecurityManager::~nsScriptSecurityManager(void)
{
    delete mOriginToPolicyMap;
    NS_IF_RELEASE(mSystemPrincipal);
    delete mPrincipals;
} 

nsScriptSecurityManager *
nsScriptSecurityManager::GetScriptSecurityManager()
{
    static nsScriptSecurityManager *ssecMan = NULL;
    if (!ssecMan) {
        ssecMan = new nsScriptSecurityManager();
        if (!ssecMan)
            return NULL;
        nsresult rv;

        NS_WITH_SERVICE(nsIXPConnect, xpc, nsIXPConnect::GetCID(), &rv);
        if (NS_SUCCEEDED(rv) && xpc) {
            rv = xpc->SetDefaultSecurityManager(
                            NS_STATIC_CAST(nsIXPCSecurityManager*, ssecMan), 
                            nsIXPCSecurityManager::HOOK_ALL);
            if (NS_FAILED(rv)) {
                NS_WARNING("failed to install xpconnect security manager!");    
            } 
#ifdef DEBUG_jband
            else {
                printf("!!!!! xpc security manager registered\n");
            }
#endif
        }
        else {
            NS_WARNING("can't get xpconnect to install security manager!");    
        }
    }
    return ssecMan;
}

PR_STATIC_CALLBACK(PRBool)
DeleteEntry(nsHashKey *aKey, void *aData, void* closure)
{
    nsDomainEntry *entry = (nsDomainEntry *) aData;
    do {
        nsDomainEntry *next = entry->mNext;
        delete entry;
        entry = next;
    } while (entry);
    return PR_TRUE;
}

void 
nsScriptSecurityManager::EnumeratePolicyCallback(const char *prefName, 
                                                 void *data)
{
    if (!prefName || !*prefName)
        return;

    nsScriptSecurityManager *mgr = (nsScriptSecurityManager *) data;
    unsigned count = 0;
    const char *dots[5];
    const char *p;
    for (p=prefName; *p; p++) {
        if (*p == '.') {
            dots[count++] = p;
            if (count == sizeof(dots)/sizeof(dots[0]))
                break;
        }
    }
    if (count < sizeof(dots)/sizeof(dots[0]))
        dots[count] = p;
    if (count < 3)
        return;
    const char *policyName = dots[1] + 1;
    int policyLength = dots[2] - policyName;
    PRBool isDefault = PL_strncmp("default", policyName, policyLength) == 0;
    if (!isDefault && count == 3) {
        // capability.policy.<policyname>.sites
        const char *sitesName = dots[2] + 1;
        int sitesLength = dots[3] - sitesName;
        if (PL_strncmp("sites", sitesName, sitesLength) == 0) {
            if (!mgr->mOriginToPolicyMap) {
                mgr->mOriginToPolicyMap = 
                    new nsObjectHashtable(nsnull, nsnull, DeleteEntry, nsnull);
                if (!mgr->mOriginToPolicyMap)
                    return;
            }
            char *s;
            if (NS_FAILED(mgr->mSecurityPrefs->SecurityGetCharPref(prefName, &s)))
                return;
            char *q=s;
            char *r=s;
            char *lastDot = nsnull;
            char *nextToLastDot = nsnull;
            PRBool working = PR_TRUE;
            while (working) {
                if (*r == ' ' || *r == '\0') {
                    working = (*r != '\0');
                    *r = '\0';
                    nsCStringKey key(nextToLastDot ? nextToLastDot+1 : q);
                    nsDomainEntry *value = new nsDomainEntry(q, policyName, 
                                                             policyLength);
                    if (!value)
                        break;
                    nsDomainEntry *de = (nsDomainEntry *) 
                        mgr->mOriginToPolicyMap->Get(&key);
                    if (!de) {
                        mgr->mOriginToPolicyMap->Put(&key, value);
                    } else {
                        if (de->Matches(q)) {
                            value->mNext = de;
                            mgr->mOriginToPolicyMap->Put(&key, value);
                        } else {
                            while (de->mNext) {
                                if (de->mNext->Matches(q)) {
                                    value->mNext = de->mNext;
                                    de->mNext = value;
                                    break;
                                }
                                de = de->mNext;
                            }
                            if (!de->mNext) {
                                de->mNext = value;
                            }
                        }
                    }
                    q = r + 1;
                    lastDot = nextToLastDot = nsnull;
                } else if (*r == '.') {
                    nextToLastDot = lastDot;
                    lastDot = r;
                }
                r++;
            }
            PR_Free(s);
            return;
        }
    }
}

nsresult
nsScriptSecurityManager::PrincipalPrefNames(const char* pref, 
                                            char** grantedPref, char** deniedPref)
{
    char* lastDot = PL_strrchr(pref, '.');
    if (!lastDot) return NS_ERROR_FAILURE;
    PRInt32 prefLen = lastDot - pref + 1;

    *grantedPref = nsnull;
    *deniedPref = nsnull;

    static const char granted[] = "granted";
    *grantedPref = (char*)PR_MALLOC(prefLen + sizeof(granted));
    if (!grantedPref) return NS_ERROR_OUT_OF_MEMORY;
    PL_strncpy(*grantedPref, pref, prefLen);
    PL_strcpy(*grantedPref + prefLen, granted);

    static const char denied[] = "denied";
    *deniedPref = (char*)PR_MALLOC(prefLen + sizeof(denied));
    if (!deniedPref)
    {
        PR_FREEIF(*grantedPref);
        return NS_ERROR_OUT_OF_MEMORY;
    }
    PL_strncpy(*deniedPref, pref, prefLen);
    PL_strcpy(*deniedPref + prefLen, denied);

    return NS_OK;
}

struct EnumeratePrincipalsInfo {
    // this struct doesn't own these objects; consider them parameters on
    // the stack
    nsSupportsHashtable *ht;
    nsISecurityPref *prefs;
};

void
nsScriptSecurityManager::EnumeratePrincipalsCallback(const char *prefName, 
                                                     void *voidParam)
{
    /* This is the principal preference syntax:
     * capability.principal.[codebase|certificate].<name>.[id|granted|denied]
     * For example:
     * user_pref("capability.principal.certificate.p1.id","12:34:AB:CD");
     * user_pref("capability.principal.certificate.p1.granted","Capability1 Capability2"); 
     * user_pref("capability.principal.certificate.p1.denied","Capability3");
     */

    EnumeratePrincipalsInfo *info = (EnumeratePrincipalsInfo *) voidParam;
    static const char idName[] = ".id";
    PRInt32 prefNameLen = PL_strlen(prefName) - (sizeof(idName)-1);
    if (PL_strcasecmp(prefName + prefNameLen, idName) != 0)
        return;

    char* id;
    if (NS_FAILED(info->prefs->SecurityGetCharPref(prefName, &id))) 
        return;

    nsXPIDLCString grantedPrefName;
    nsXPIDLCString deniedPrefName;
    if (NS_FAILED(PrincipalPrefNames( prefName, 
                                      getter_Copies(grantedPrefName), 
                                      getter_Copies(deniedPrefName)  )))
        return;

    char* grantedList = nsnull;
    info->prefs->SecurityGetCharPref(grantedPrefName, &grantedList);
    char* deniedList = nsnull;
    info->prefs->SecurityGetCharPref(deniedPrefName, &deniedList);

    //-- Delete prefs if their value is the empty string
    if ((!id || id[0] == '\0') || 
        ((!grantedList || grantedList[0] == '\0') && (!deniedList || deniedList[0] == '\0')))
    {
        info->prefs->SecurityClearUserPref(prefName);
        info->prefs->SecurityClearUserPref(grantedPrefName);
        info->prefs->SecurityClearUserPref(deniedPrefName);
        return;
    }

    //-- Create a principal based on the prefs
    static const char certificateName[] = "capability.principal.certificate";
    static const char codebaseName[] = "capability.principal.codebase";
    nsCOMPtr<nsIPrincipal> principal;
    if (PL_strncmp(prefName, certificateName, 
                   sizeof(certificateName)-1) == 0) 
    {
        nsCertificatePrincipal *certificate = new nsCertificatePrincipal();
        if (certificate) {
            NS_ADDREF(certificate);
            if (NS_SUCCEEDED(certificate->InitFromPersistent(prefName, id, 
                                                             grantedList, deniedList))) 
                principal = do_QueryInterface((nsBasePrincipal*)certificate);
            NS_RELEASE(certificate);
        }
    } else if(PL_strncmp(prefName, codebaseName, 
                   sizeof(codebaseName)-1) == 0) 
    {
        nsCodebasePrincipal *codebase = new nsCodebasePrincipal();
        if (codebase) {
            NS_ADDREF(codebase);
            if (NS_SUCCEEDED(codebase->InitFromPersistent(prefName, id, 
                                                          grantedList, deniedList))) 
                principal = do_QueryInterface((nsBasePrincipal*)codebase);
            NS_RELEASE(codebase);
        }
    }
    PR_FREEIF(grantedList);
    PR_FREEIF(deniedList);
   
    if (principal) {
        nsIPrincipalKey key(principal);
        info->ht->Put(&key, principal);
    }
}

static const char jsEnabledPrefName[] = "javascript.enabled";
static const char jsMailEnabledPrefName[] = "javascript.allow.mailnews";

int PR_CALLBACK
nsScriptSecurityManager::JSEnabledPrefChanged(const char *pref, void *data)
{
    nsScriptSecurityManager *secMgr = (nsScriptSecurityManager *) data;

    if (NS_FAILED(secMgr->mPrefs->GetBoolPref(jsEnabledPrefName, 
                                                      &secMgr->mIsJavaScriptEnabled)))
    {
        // Default to enabled.
        secMgr->mIsJavaScriptEnabled = PR_TRUE;
    }

    if (NS_FAILED(secMgr->mPrefs->GetBoolPref(jsMailEnabledPrefName, 
                                                      &secMgr->mIsMailJavaScriptEnabled))) 
    {
        // Default to enabled.
        secMgr->mIsMailJavaScriptEnabled = PR_TRUE;
    }

    return 0;
}

int PR_CALLBACK
nsScriptSecurityManager::PrincipalPrefChanged(const char *pref, void *data)
{
    nsScriptSecurityManager *secMgr = (nsScriptSecurityManager *) data;
    if (secMgr->mIsWritingPrefs)
        return 0;

    char* lastDot = PL_strrchr(pref, '.');
    if (!lastDot) return NS_ERROR_FAILURE;
    PRInt32 prefLen = lastDot - pref + 1;

    static const char id[] = "id";
    char* idPref = (char*)PR_MALLOC(prefLen + sizeof(id));
    if (!idPref) return NS_ERROR_OUT_OF_MEMORY;
    PL_strncpy(idPref, pref, prefLen);
    PL_strcpy(idPref + prefLen, id);

    EnumeratePrincipalsInfo info;
    info.ht = secMgr->mPrincipals;
    info.prefs = secMgr->mSecurityPrefs;
    EnumeratePrincipalsCallback(idPref, &info);
    PR_FREEIF(idPref);
    return 0;
}

nsresult
nsScriptSecurityManager::InitPrefs()
{
    nsresult rv;
    NS_WITH_SERVICE(nsIPref, prefs, kPrefServiceCID, &rv);
    if (NS_FAILED(rv))
        return NS_ERROR_FAILURE;
    mPrefs = prefs;

    mSecurityPrefs = do_QueryInterface(prefs, &rv);
    if (NS_FAILED(rv))
        return NS_ERROR_FAILURE;

    // Set the initial value of the "javascript.enabled" pref
    JSEnabledPrefChanged(jsEnabledPrefName, this);

    // set callbacks in case the value of the pref changes
    prefs->RegisterCallback(jsEnabledPrefName, JSEnabledPrefChanged, this);
    prefs->RegisterCallback(jsMailEnabledPrefName, JSEnabledPrefChanged, this);

    mPrefs->EnumerateChildren("capability.policy",
                              nsScriptSecurityManager::EnumeratePolicyCallback,
                              (void *) this);
    
    if (!mPrincipals) {
        mPrincipals = new nsSupportsHashtable(31);
        if (!mPrincipals)
            return NS_ERROR_OUT_OF_MEMORY;
    }
    EnumeratePrincipalsInfo info;
    info.ht = mPrincipals;
    info.prefs = mSecurityPrefs;
    
    mPrefs->EnumerateChildren("capability.principal", 
                              nsScriptSecurityManager::EnumeratePrincipalsCallback,
                              (void *) &info);
    
    mPrefs->RegisterCallback("capability.principal", PrincipalPrefChanged, this);

    return NS_OK;
}
