/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "MPL"); you may not use this file except in
 * compliance with the MPL.  You may obtain a copy of the MPL at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the MPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the MPL
 * for the specific language governing rights and limitations under the
 * MPL.
 *
 * The Initial Developer of this code under the MPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1999 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

#include "plhash.h"
#include "jsapi.h"
#include "nsIComponentLoader.h"
#include "nsIComponentLoaderManager.h"
#include "nsIJSRuntimeService.h"
#include "nsIJSContextStack.h"
#include "nsISupports.h"
#include "nsIXPConnect.h"
#include "nsIModule.h"
#include "nsSupportsArray.h"
#include "nsIFile.h"
#ifndef XPCONNECT_STANDALONE
#include "nsIPrincipal.h"
#endif
extern const char mozJSComponentLoaderContractID[];
extern const char jsComponentTypeName[];

/* 6bd13476-1dd2-11b2-bbef-f0ccb5fa64b6 (thanks, mozbot) */

#define MOZJSCOMPONENTLOADER_CID \
  {0x6bd13476, 0x1dd2, 0x11b2, \
    { 0xbb, 0xef, 0xf0, 0xcc, 0xb5, 0xfa, 0x64, 0xb6 }}

class mozJSComponentLoader : public nsIComponentLoader {

public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSICOMPONENTLOADER

    mozJSComponentLoader();
    virtual ~mozJSComponentLoader();

 protected:
    nsresult ReallyInit();
    nsresult AttemptRegistration(nsIFile *component, PRBool deferred);
    nsresult UnregisterComponent(nsIFile *component);
    nsresult RegisterComponentsInDir(PRInt32 when, nsIFile *dir);
    JSObject *GlobalForLocation(const char *aLocation, nsIFile *component);
    nsIModule *ModuleForLocation(const char *aLocation, nsIFile *component);
    PRBool HasChanged(const char *registryLocation, nsIFile *component);
    nsresult SetRegistryInfo(const char *registryLocation, nsIFile *component);
    nsresult RemoveRegistryInfo(nsIFile *component, const char *registryLocation);

    nsCOMPtr<nsIComponentManager> mCompMgr;
    nsCOMPtr<nsIComponentLoaderManager> mLoaderManager;
    nsCOMPtr<nsIJSRuntimeService> mRuntimeService;
#ifndef XPCONNECT_STANDALONE
    nsCOMPtr<nsIPrincipal> mSystemPrincipal;
#endif
    JSRuntime *mRuntime;
    PLHashTable *mModules;
    PLHashTable *mGlobals;

    PRBool mInitialized;
    nsSupportsArray mDeferredComponents;
};

class JSCLAutoContext
{
public:
    JSCLAutoContext(JSRuntime* rt);
    ~JSCLAutoContext();

    operator JSContext*() const {return mContext;}
    JSContext* GetContext() const {return mContext;}
    nsresult   GetError()   const {return mError;}


    JSCLAutoContext(); // not implemnted
private:
    JSContext* mContext;
    nsresult   mError;
    JSBool     mPopNeeded;
    intN       mContextThread; 
};


class JSCLAutoErrorReporterSetter
{
public:
    JSCLAutoErrorReporterSetter(JSContext* cx, JSErrorReporter reporter)
        {mContext = cx; mOldReporter = JS_SetErrorReporter(cx, reporter);}
    ~JSCLAutoErrorReporterSetter()
        {JS_SetErrorReporter(mContext, mOldReporter);} 
    JSCLAutoErrorReporterSetter(); // not implemented
private:
    JSContext* mContext;
    JSErrorReporter mOldReporter;
};
