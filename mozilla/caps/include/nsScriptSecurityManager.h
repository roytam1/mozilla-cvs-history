/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
 * IBM Corporation
 */

#ifndef _NS_SCRIPT_SECURITY_MANAGER_H_
#define _NS_SCRIPT_SECURITY_MANAGER_H_

#include "nsIScriptSecurityManager.h"
#include "nsIPrincipal.h"
#include "jsapi.h"
#include "jsdbgapi.h"
#include "nsIXPCSecurityManager.h"
#include "nsHashtable.h"
#include "nsCOMPtr.h"
#include "nsIPrefService.h"
#include "nsISecurityPref.h"
#include "nsIJSContextStack.h"
#include "nsIObserver.h"
#include "nsWeakPtr.h"

class nsIDocShell;
class nsString;
class nsIClassInfo;
class nsSystemPrincipal;

/////////////////////
// nsIPrincipalKey //
/////////////////////

class nsIPrincipalKey : public nsHashKey {
public:
    nsIPrincipalKey(nsIPrincipal* key) {
        mKey = key;
        NS_IF_ADDREF(mKey);
    }
    
    ~nsIPrincipalKey(void) {
        NS_IF_RELEASE(mKey);
    }
    
    PRUint32 HashCode(void) const {
        PRUint32 hash;
        mKey->HashValue(&hash);
        return hash;
    }
    
    PRBool Equals(const nsHashKey* aKey) const {
        PRBool eq;
        mKey->Equals(((nsIPrincipalKey*) aKey)->mKey, &eq);
        return eq;
    }
    
    nsHashKey *Clone(void) const {
        return new nsIPrincipalKey(mKey);
    }

protected:
    nsIPrincipal* mKey;
};

#define NS_SCRIPTSECURITYMANAGER_CID \
{ 0x7ee2a4c0, 0x4b93, 0x17d3, \
{ 0xba, 0x18, 0x00, 0x60, 0xb0, 0xf1, 0x99, 0xa2 }}

class nsScriptSecurityManager : public nsIScriptSecurityManager, public nsIObserver
{
public:
    nsScriptSecurityManager();
    virtual ~nsScriptSecurityManager();
    
    NS_DEFINE_STATIC_CID_ACCESSOR(NS_SCRIPTSECURITYMANAGER_CID)
        
    NS_DECL_ISUPPORTS
    NS_DECL_NSISCRIPTSECURITYMANAGER
    NS_DECL_NSIXPCSECURITYMANAGER
    NS_DECL_NSIOBSERVER

    static nsScriptSecurityManager*
    GetScriptSecurityManager();

    static nsSystemPrincipal*
    SystemPrincipalSingletonConstructor();

    JSContext* GetCurrentContextQuick();

private:

    static PRBool IsDOMClass(nsIClassInfo* aClassInfo);

    nsresult
    GetBaseURIScheme(nsIURI* aURI, char** aScheme);

    static nsresult 
    ReportErrorToConsole(nsIURI* aTarget);

    nsresult
    GetRootDocShell(JSContext* cx, nsIDocShell **result);

    nsresult
    CheckPropertyAccessImpl(PRUint32 aAction, nsIXPCNativeCallContext* aCallContext,
                            JSContext* aJSContext, JSObject* aJSObject,
                            nsISupports* aObj, nsIURI* aTargetURI,
                            nsIClassInfo* aClassInfo,
                            jsval aName, const char* aClassName, 
                            const char* aProperty, void** aPolicy);

    nsresult
    CheckSameOrigin(JSContext* aCx, nsIPrincipal* aSubject, 
                    nsIPrincipal* aObject, PRUint32 aAction);
    
    PRInt32 
    GetSecurityLevel(nsIPrincipal *principal,
                     PRBool aIsDOM,
                     const char* aClassName, const char* aProperty,
                     PRUint32 aAction, nsCString &capability, void** aPolicy);

    nsresult
    GetPrefName(nsIPrincipal* principal,
                const char* aClassName, const char* aPropertyName,
                void* aClassPolicy, nsCString &result);

    nsresult
    CreateCodebasePrincipal(nsIURI* aURI, nsIPrincipal** result);

    nsresult
    GetSubjectPrincipal(JSContext* aCx, nsIPrincipal** result);

    nsresult
    GetFramePrincipal(JSContext* cx, JSStackFrame* fp, nsIPrincipal** result);
                                                     
    nsresult
    GetScriptPrincipal(JSContext* cx, JSScript* script, nsIPrincipal** result);

    nsresult
    GetFunctionObjectPrincipal(JSContext* cx, JSObject* obj, 
                               nsIPrincipal** result);

    nsresult
    GetPrincipalAndFrame(JSContext *cx,
                         nsIPrincipal** result,
                         JSStackFrame** frameResult);

    nsresult
    SavePrincipal(nsIPrincipal* aToSave);

    nsresult
    CheckXPCPermissions(JSContext* cx, nsISupports* aObj,
                        const char* aObjectSecurityLevel, const char* aErrorMsg);

    nsresult
    InitPrefs();

    static nsresult 
    PrincipalPrefNames(const char* pref, char** grantedPref, char** deniedPref);

    nsresult
    InitPolicies(PRUint32 prefCount, const char** prefNames,
                 nsISecurityPref* securityPref);

    nsresult
    InitPrincipals(PRUint32 prefCount, const char** prefNames,
                   nsISecurityPref* securityPref);

    inline void
    JSEnabledPrefChanged(nsISecurityPref* aSecurityPref);

    static const char* sJSEnabledPrefName;
    static const char* sJSMailEnabledPrefName;
    static const char* sPrincipalPrefix;

    nsObjectHashtable* mOriginToPolicyMap;
    nsHashtable* mClassPolicies;
    nsWeakPtr mPrefBranchWeakRef;
    nsIPrincipal* mSystemPrincipal;
    nsCOMPtr<nsIPrincipal> mSystemCertificate;
    nsSupportsHashtable* mPrincipals;
    PRBool mIsJavaScriptEnabled;
    PRBool mIsMailJavaScriptEnabled;
    PRBool mIsWritingPrefs;
    nsCOMPtr<nsIJSContextStack> mThreadJSContextStack;
    PRBool mNameSetRegistered;
};

#endif /*_NS_SCRIPT_SECURITY_MANAGER_H_*/

