/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
 * Copyright (C) 1998-1999 Netscape Communications Corporation.  All Rights
 * Reserved.
 */
#ifndef _NS_SCRIPT_SECURITY_MANAGER_H_
#define _NS_SCRIPT_SECURITY_MANAGER_H_

#include "nsIScriptSecurityManager.h"
#include "nsIPrincipal.h"
#include "nsIURI.h"
#include "jsapi.h"
#include "jsdbgapi.h"
#include "nsIScriptContext.h"

class nsScriptSecurityManager : public nsIScriptSecurityManager {
public:
  nsScriptSecurityManager();
  virtual ~nsScriptSecurityManager();

  NS_DECL_ISUPPORTS
  
  static nsScriptSecurityManager *
  GetScriptSecurityManager();

  NS_IMETHOD CheckScriptAccess(nsIScriptContext* aContext, void* aObj, const char* aProp, PRBool* aResult);
  NS_IMETHOD GetSubjectOriginURL(JSContext *aCx, char** aOrigin);
  NS_IMETHOD GetObjectOriginURL(JSContext *aCx, JSObject *object, char** aOrigin);
  NS_IMETHOD NewJSPrincipals(nsIURI *aURL, nsString* aName, nsIPrincipal * * aPrin);
  NS_IMETHOD CheckPermissions(JSContext *aCx, JSObject *aObj, short target, PRBool* aReturn);  
  NS_IMETHOD GetContainerPrincipals(JSContext *aCx, JSObject *aContainer, nsIPrincipal * * result);
  NS_IMETHOD GetPrincipalsFromStackFrame(JSContext *aCx, JSPrincipals** aPrincipals);
	NS_IMETHOD CanAccessTarget(JSContext *aCx, PRInt16 target, PRBool* aReturn);
private:
  char * GetCanonicalizedOrigin(JSContext *cx, const char* aUrlString);
  NS_IMETHOD GetOriginFromSourceURL(nsIURI * origin, char * * result);
  PRBool SameOrigins(JSContext *aCx, const char* aOrigin1, const char* aOrigin2);
  PRInt32 CheckForPrivilege(JSContext *cx, char *prop_name, int priv_code);
  char* FindOriginURL(JSContext *aCx, JSObject *aGlobal);
  char* AddSecPolicyPrefix(JSContext *cx, char *pref_str);
  char* GetSitePolicy(const char *org);
};
#endif /*_NS_SCRIPT_SECURITY_MANAGER_H_*/
