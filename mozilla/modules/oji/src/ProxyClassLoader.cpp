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
 * Copyright (C) 2001 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s):
 *     Patrick Beard <beard@netscape.com> (original author)
 */

#include "ProxyClassLoader.h"

#include "jsapi.h"
#include "jsjava.h"
#include "prprf.h"

#include "nsIServiceManager.h"
#include "nsIJSContextStack.h"
#include "nsIPrincipal.h"
#include "nsICodebasePrincipal.h"
#include "nsIScriptContext.h"
#include "nsIScriptGlobalObject.h"
#include "nsIScriptObjectPrincipal.h"
#include "nsNetUtil.h"

/**
 * Obtain the URL of the document of the currently running script. This will
 * be used as the default location to download classes from.
 */
static nsresult getScriptCodebase(JSContext* cx, nsIURI* *result)
{
    nsIScriptContext* scriptContext = NS_REINTERPRET_CAST(nsIScriptContext*, JS_GetContextPrivate(cx));
    if (scriptContext) {
        nsCOMPtr<nsIScriptGlobalObject> scriptGlobal;
        scriptContext->GetGlobalObject(getter_AddRefs(scriptGlobal));
        nsCOMPtr<nsIScriptObjectPrincipal> scriptObjectPrincipal = do_QueryInterface(scriptGlobal);
        if (scriptObjectPrincipal) {
            nsCOMPtr<nsIPrincipal> principal;
            scriptObjectPrincipal->GetPrincipal(getter_AddRefs(principal));
            if (principal) {
                nsCOMPtr<nsICodebasePrincipal> codebasePrincipal = do_QueryInterface(principal);
                if (codebasePrincipal)
                    return codebasePrincipal->GetURI(result);
            }
        }
    }
    return NS_ERROR_FAILURE;
}

/**
 * Obtain the netscape.oji.ProxyClassLoader instance associated with the
 * document of the currently running script. For now, store a LiveConnect
 * wrapper for this instance in window.navigator.javaclasses. There
 * hopefully aren't any security concerns with exposing this to scripts,
 * as the constructor is private, and the class loader itself can
 * only load classes from the document's URL and below.
 */
static nsresult getScriptClassLoader(JNIEnv* env, jobject* classloader)
{
    // get the current JSContext from the context stack service.
    nsresult rv;
    nsCOMPtr<nsIJSContextStack> contexts = do_GetService("@mozilla.org/js/xpc/ContextStack;1", &rv);
    if (NS_FAILED(rv)) return rv;
    JSContext* cx;
    rv = contexts->Peek(&cx);
    if (NS_FAILED(rv)) return rv;
    
    // lookup "window.navigator.javaclasses", if it exists, this is the class loader bound
    // to this page.
    JSObject* window = JS_GetGlobalObject(cx);
    if (!window) return NS_ERROR_FAILURE;

    jsval navigator;
    if (!JS_GetProperty(cx, window, "navigator", &navigator)) return NS_ERROR_FAILURE;
    
    jsval javaclasses;
    if (JS_GetProperty(cx, JSVAL_TO_OBJECT(navigator), "javaclasses", &javaclasses)) {
        // unwrap this, the way LiveConnect does it.
        if (JSJ_ConvertJSValueToJavaObject(cx, javaclasses, classloader))
            return NS_OK;
    }

    nsCOMPtr<nsIURI> codebase;
    rv = getScriptCodebase(cx, getter_AddRefs(codebase));
    if (NS_FAILED(rv)) return rv;
    
    // create a netscape.oji.ProxyClassLoader instance.
    nsXPIDLCString spec;
    rv = codebase->GetSpec(getter_Copies(spec));
    if (NS_FAILED(rv)) return rv;
    
    jstring jspec = env->NewStringUTF(spec);
    if (!spec) {
        env->ExceptionClear();
        return NS_ERROR_FAILURE;
    }
    jclass netscape_oji_ProxyClassLoader = env->FindClass("netscape/oji/ProxyClassLoader");
    if (!netscape_oji_ProxyClassLoader) {
        env->ExceptionClear();
        return NS_ERROR_FAILURE;
    }
    jmethodID constructorID = env->GetMethodID(netscape_oji_ProxyClassLoader, "<init>", "(Ljava/lang/String;)V");
    if (!constructorID) {
        env->ExceptionClear();
        return NS_ERROR_FAILURE;
    }
    *classloader = env->NewObject(netscape_oji_ProxyClassLoader, constructorID, jspec);
    if (!*classloader) {
        env->ExceptionClear();
        return NS_ERROR_FAILURE;
    }

    env->DeleteLocalRef(jspec);
    env->DeleteLocalRef(netscape_oji_ProxyClassLoader);
    
    // now, cache the class loader in "window.navigator.javaclasses"
    if (JSJ_ConvertJavaObjectToJSValue(cx, *classloader, &javaclasses)) {
        JS_SetProperty(cx, JSVAL_TO_OBJECT(navigator), "javaclasses", &javaclasses);
    }
    
    return NS_OK;
}

jclass ProxyFindClass(JNIEnv* env, const char* name)
{
    do {
        // TODO:  prevent recursive call to ProxyFindClass, if netscape.oji.ProxyClassLoader
        // isn't found by getScriptClassLoader().
        jobject classloader;
        nsresult rv = getScriptClassLoader(env, &classloader);
        if (NS_FAILED(rv)) break;

        jclass netscape_oji_ProxyClassLoader = env->GetObjectClass(classloader);
        jmethodID loadClassID = env->GetMethodID(netscape_oji_ProxyClassLoader, "loadClass",
                                                 "(Ljava/lang/String;Z)Ljava/lang/Class;");
        env->DeleteLocalRef(netscape_oji_ProxyClassLoader);
        if (!loadClassID) {
            env->ExceptionClear();
            break;
        }
        jstring jname = env->NewStringUTF(name);
        jvalue jargs[2]; jargs[0].l = jname, jargs[1].z = JNI_TRUE;
        jclass c = (jclass) env->CallObjectMethodA(classloader, loadClassID, jargs);
        env->DeleteLocalRef(jname);
        return c;
    } while (0);
    return 0;
}
