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
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */
/* AUTO-GENERATED. DO NOT EDIT!!! */

#include "jsapi.h"
#include "nsJSUtils.h"
#include "nscore.h"
#include "nsIScriptContext.h"
#include "nsIScriptSecurityManager.h"
#include "nsIJSScriptObject.h"
#include "nsIScriptObjectOwner.h"
#include "nsIScriptGlobalObject.h"
#include "nsCOMPtr.h"
#include "nsIPtr.h"
#include "nsString.h"
#include "nsIDOMNavigator.h"
#include "nsIDOMPluginArray.h"
#include "nsIDOMMimeTypeArray.h"


static NS_DEFINE_IID(kIScriptObjectOwnerIID, NS_ISCRIPTOBJECTOWNER_IID);
static NS_DEFINE_IID(kIJSScriptObjectIID, NS_IJSSCRIPTOBJECT_IID);
static NS_DEFINE_IID(kIScriptGlobalObjectIID, NS_ISCRIPTGLOBALOBJECT_IID);
static NS_DEFINE_IID(kINavigatorIID, NS_IDOMNAVIGATOR_IID);
static NS_DEFINE_IID(kIPluginArrayIID, NS_IDOMPLUGINARRAY_IID);
static NS_DEFINE_IID(kIMimeTypeArrayIID, NS_IDOMMIMETYPEARRAY_IID);

NS_DEF_PTR(nsIDOMNavigator);
NS_DEF_PTR(nsIDOMPluginArray);
NS_DEF_PTR(nsIDOMMimeTypeArray);

//
// Navigator property ids
//
enum Navigator_slots {
  NAVIGATOR_APPCODENAME = -1,
  NAVIGATOR_APPNAME = -2,
  NAVIGATOR_APPVERSION = -3,
  NAVIGATOR_LANGUAGE = -4,
  NAVIGATOR_MIMETYPES = -5,
  NAVIGATOR_PLATFORM = -6,
  NAVIGATOR_PLUGINS = -7,
  NAVIGATOR_SECURITYPOLICY = -8,
  NAVIGATOR_USERAGENT = -9
};

/***********************************************************************/
//
// Navigator Properties Getter
//
PR_STATIC_CALLBACK(JSBool)
GetNavigatorProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  nsIDOMNavigator *a = (nsIDOMNavigator*)nsJSUtils::nsGetNativeThis(cx, obj);

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == a) {
    return JS_TRUE;
  }

  if (JSVAL_IS_INT(id)) {
    nsIScriptContext *scriptCX = (nsIScriptContext *)JS_GetContextPrivate(cx);
    nsCOMPtr<nsIScriptSecurityManager> secMan;
    if (NS_OK != scriptCX->GetSecurityManager(getter_AddRefs(secMan))) {
      return JS_FALSE;
    }
    switch(JSVAL_TO_INT(id)) {
      case NAVIGATOR_APPCODENAME:
      {
        PRBool ok = PR_FALSE;
        secMan->CheckScriptAccess(scriptCX, obj, "navigator.appcodename", &ok);
        if (!ok) {
          //Need to throw error here
          return JS_FALSE;
        }
        nsAutoString prop;
        if (NS_SUCCEEDED(a->GetAppCodeName(prop))) {
          nsJSUtils::nsConvertStringToJSVal(prop, cx, vp);
        }
        else {
          return JS_FALSE;
        }
        break;
      }
      case NAVIGATOR_APPNAME:
      {
        PRBool ok = PR_FALSE;
        secMan->CheckScriptAccess(scriptCX, obj, "navigator.appname", &ok);
        if (!ok) {
          //Need to throw error here
          return JS_FALSE;
        }
        nsAutoString prop;
        if (NS_SUCCEEDED(a->GetAppName(prop))) {
          nsJSUtils::nsConvertStringToJSVal(prop, cx, vp);
        }
        else {
          return JS_FALSE;
        }
        break;
      }
      case NAVIGATOR_APPVERSION:
      {
        PRBool ok = PR_FALSE;
        secMan->CheckScriptAccess(scriptCX, obj, "navigator.appversion", &ok);
        if (!ok) {
          //Need to throw error here
          return JS_FALSE;
        }
        nsAutoString prop;
        if (NS_SUCCEEDED(a->GetAppVersion(prop))) {
          nsJSUtils::nsConvertStringToJSVal(prop, cx, vp);
        }
        else {
          return JS_FALSE;
        }
        break;
      }
      case NAVIGATOR_LANGUAGE:
      {
        PRBool ok = PR_FALSE;
        secMan->CheckScriptAccess(scriptCX, obj, "navigator.language", &ok);
        if (!ok) {
          //Need to throw error here
          return JS_FALSE;
        }
        nsAutoString prop;
        if (NS_SUCCEEDED(a->GetLanguage(prop))) {
          nsJSUtils::nsConvertStringToJSVal(prop, cx, vp);
        }
        else {
          return JS_FALSE;
        }
        break;
      }
      case NAVIGATOR_MIMETYPES:
      {
        PRBool ok = PR_FALSE;
        secMan->CheckScriptAccess(scriptCX, obj, "navigator.mimetypes", &ok);
        if (!ok) {
          //Need to throw error here
          return JS_FALSE;
        }
        nsIDOMMimeTypeArray* prop;
        if (NS_SUCCEEDED(a->GetMimeTypes(&prop))) {
          // get the js object
          nsJSUtils::nsConvertObjectToJSVal((nsISupports *)prop, cx, vp);
        }
        else {
          return JS_FALSE;
        }
        break;
      }
      case NAVIGATOR_PLATFORM:
      {
        PRBool ok = PR_FALSE;
        secMan->CheckScriptAccess(scriptCX, obj, "navigator.platform", &ok);
        if (!ok) {
          //Need to throw error here
          return JS_FALSE;
        }
        nsAutoString prop;
        if (NS_SUCCEEDED(a->GetPlatform(prop))) {
          nsJSUtils::nsConvertStringToJSVal(prop, cx, vp);
        }
        else {
          return JS_FALSE;
        }
        break;
      }
      case NAVIGATOR_PLUGINS:
      {
        PRBool ok = PR_FALSE;
        secMan->CheckScriptAccess(scriptCX, obj, "navigator.plugins", &ok);
        if (!ok) {
          //Need to throw error here
          return JS_FALSE;
        }
        nsIDOMPluginArray* prop;
        if (NS_SUCCEEDED(a->GetPlugins(&prop))) {
          // get the js object
          nsJSUtils::nsConvertObjectToJSVal((nsISupports *)prop, cx, vp);
        }
        else {
          return JS_FALSE;
        }
        break;
      }
      case NAVIGATOR_SECURITYPOLICY:
      {
        PRBool ok = PR_FALSE;
        secMan->CheckScriptAccess(scriptCX, obj, "navigator.securitypolicy", &ok);
        if (!ok) {
          //Need to throw error here
          return JS_FALSE;
        }
        nsAutoString prop;
        if (NS_SUCCEEDED(a->GetSecurityPolicy(prop))) {
          nsJSUtils::nsConvertStringToJSVal(prop, cx, vp);
        }
        else {
          return JS_FALSE;
        }
        break;
      }
      case NAVIGATOR_USERAGENT:
      {
        PRBool ok = PR_FALSE;
        secMan->CheckScriptAccess(scriptCX, obj, "navigator.useragent", &ok);
        if (!ok) {
          //Need to throw error here
          return JS_FALSE;
        }
        nsAutoString prop;
        if (NS_SUCCEEDED(a->GetUserAgent(prop))) {
          nsJSUtils::nsConvertStringToJSVal(prop, cx, vp);
        }
        else {
          return JS_FALSE;
        }
        break;
      }
      default:
        return nsJSUtils::nsCallJSScriptObjectGetProperty(a, cx, id, vp);
    }
  }
  else {
    return nsJSUtils::nsCallJSScriptObjectGetProperty(a, cx, id, vp);
  }

  return PR_TRUE;
}

/***********************************************************************/
//
// Navigator Properties Setter
//
PR_STATIC_CALLBACK(JSBool)
SetNavigatorProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  nsIDOMNavigator *a = (nsIDOMNavigator*)nsJSUtils::nsGetNativeThis(cx, obj);

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == a) {
    return JS_TRUE;
  }

  if (JSVAL_IS_INT(id)) {
    nsIScriptContext *scriptCX = (nsIScriptContext *)JS_GetContextPrivate(cx);
    nsCOMPtr<nsIScriptSecurityManager> secMan;
    if (NS_OK != scriptCX->GetSecurityManager(getter_AddRefs(secMan))) {
      return JS_FALSE;
    }
    switch(JSVAL_TO_INT(id)) {
      case 0:
      default:
        return nsJSUtils::nsCallJSScriptObjectSetProperty(a, cx, id, vp);
    }
  }
  else {
    return nsJSUtils::nsCallJSScriptObjectSetProperty(a, cx, id, vp);
  }

  return PR_TRUE;
}


//
// Navigator finalizer
//
PR_STATIC_CALLBACK(void)
FinalizeNavigator(JSContext *cx, JSObject *obj)
{
  nsJSUtils::nsGenericFinalize(cx, obj);
}


//
// Navigator enumerate
//
PR_STATIC_CALLBACK(JSBool)
EnumerateNavigator(JSContext *cx, JSObject *obj)
{
  return nsJSUtils::nsGenericEnumerate(cx, obj);
}


//
// Navigator resolve
//
PR_STATIC_CALLBACK(JSBool)
ResolveNavigator(JSContext *cx, JSObject *obj, jsval id)
{
  return nsJSUtils::nsGenericResolve(cx, obj, id);
}


//
// Native method JavaEnabled
//
PR_STATIC_CALLBACK(JSBool)
NavigatorJavaEnabled(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsIDOMNavigator *nativeThis = (nsIDOMNavigator*)nsJSUtils::nsGetNativeThis(cx, obj);
  PRBool nativeRet;

  *rval = JSVAL_NULL;

  nsIScriptContext *scriptCX = (nsIScriptContext *)JS_GetContextPrivate(cx);
  nsCOMPtr<nsIScriptSecurityManager> secMan;
  if (NS_OK != scriptCX->GetSecurityManager(getter_AddRefs(secMan))) {
    return JS_FALSE;
  }
  {
    PRBool ok;
    secMan->CheckScriptAccess(scriptCX, obj, "navigator.javaenabled", &ok);
    if (!ok) {
      //Need to throw error here
      return JS_FALSE;
    }
  }

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == nativeThis) {
    return JS_TRUE;
  }

  {

    if (NS_OK != nativeThis->JavaEnabled(&nativeRet)) {
      return JS_FALSE;
    }

    *rval = BOOLEAN_TO_JSVAL(nativeRet);
  }

  return JS_TRUE;
}


//
// Native method TaintEnabled
//
PR_STATIC_CALLBACK(JSBool)
NavigatorTaintEnabled(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsIDOMNavigator *nativeThis = (nsIDOMNavigator*)nsJSUtils::nsGetNativeThis(cx, obj);
  PRBool nativeRet;

  *rval = JSVAL_NULL;

  nsIScriptContext *scriptCX = (nsIScriptContext *)JS_GetContextPrivate(cx);
  nsCOMPtr<nsIScriptSecurityManager> secMan;
  if (NS_OK != scriptCX->GetSecurityManager(getter_AddRefs(secMan))) {
    return JS_FALSE;
  }
  {
    PRBool ok;
    secMan->CheckScriptAccess(scriptCX, obj, "navigator.taintenabled", &ok);
    if (!ok) {
      //Need to throw error here
      return JS_FALSE;
    }
  }

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == nativeThis) {
    return JS_TRUE;
  }

  {

    if (NS_OK != nativeThis->TaintEnabled(&nativeRet)) {
      return JS_FALSE;
    }

    *rval = BOOLEAN_TO_JSVAL(nativeRet);
  }

  return JS_TRUE;
}


//
// Native method Preference
//
PR_STATIC_CALLBACK(JSBool)
NavigatorPreference(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsIDOMNavigator *nativeThis = (nsIDOMNavigator*)nsJSUtils::nsGetNativeThis(cx, obj);
  jsval nativeRet;

  *rval = JSVAL_NULL;

  nsIScriptContext *scriptCX = (nsIScriptContext *)JS_GetContextPrivate(cx);
  nsCOMPtr<nsIScriptSecurityManager> secMan;
  if (NS_OK != scriptCX->GetSecurityManager(getter_AddRefs(secMan))) {
    return JS_FALSE;
  }
  {
    PRBool ok;
    secMan->CheckScriptAccess(scriptCX, obj, "navigator.preference", &ok);
    if (!ok) {
      //Need to throw error here
      return JS_FALSE;
    }
  }

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == nativeThis) {
    return JS_TRUE;
  }

  {

    if (NS_OK != nativeThis->Preference(cx, argv+0, argc-0, &nativeRet)) {
      return JS_FALSE;
    }

    *rval = nativeRet;
  }

  return JS_TRUE;
}


/***********************************************************************/
//
// class for Navigator
//
JSClass NavigatorClass = {
  "Navigator", 
  JSCLASS_HAS_PRIVATE | JSCLASS_PRIVATE_IS_NSISUPPORTS,
  JS_PropertyStub,
  JS_PropertyStub,
  GetNavigatorProperty,
  SetNavigatorProperty,
  EnumerateNavigator,
  ResolveNavigator,
  JS_ConvertStub,
  FinalizeNavigator
};


//
// Navigator class properties
//
static JSPropertySpec NavigatorProperties[] =
{
  {"appCodeName",    NAVIGATOR_APPCODENAME,    JSPROP_ENUMERATE | JSPROP_READONLY},
  {"appName",    NAVIGATOR_APPNAME,    JSPROP_ENUMERATE | JSPROP_READONLY},
  {"appVersion",    NAVIGATOR_APPVERSION,    JSPROP_ENUMERATE | JSPROP_READONLY},
  {"language",    NAVIGATOR_LANGUAGE,    JSPROP_ENUMERATE | JSPROP_READONLY},
  {"mimeTypes",    NAVIGATOR_MIMETYPES,    JSPROP_ENUMERATE | JSPROP_READONLY},
  {"platform",    NAVIGATOR_PLATFORM,    JSPROP_ENUMERATE | JSPROP_READONLY},
  {"plugins",    NAVIGATOR_PLUGINS,    JSPROP_ENUMERATE | JSPROP_READONLY},
  {"securityPolicy",    NAVIGATOR_SECURITYPOLICY,    JSPROP_ENUMERATE | JSPROP_READONLY},
  {"userAgent",    NAVIGATOR_USERAGENT,    JSPROP_ENUMERATE | JSPROP_READONLY},
  {0}
};


//
// Navigator class methods
//
static JSFunctionSpec NavigatorMethods[] = 
{
  {"javaEnabled",          NavigatorJavaEnabled,     0},
  {"taintEnabled",          NavigatorTaintEnabled,     0},
  {"preference",          NavigatorPreference,     0},
  {0}
};


//
// Navigator constructor
//
PR_STATIC_CALLBACK(JSBool)
Navigator(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  return JS_FALSE;
}


//
// Navigator class initialization
//
extern "C" NS_DOM nsresult NS_InitNavigatorClass(nsIScriptContext *aContext, void **aPrototype)
{
  JSContext *jscontext = (JSContext *)aContext->GetNativeContext();
  JSObject *proto = nsnull;
  JSObject *constructor = nsnull;
  JSObject *parent_proto = nsnull;
  JSObject *global = JS_GetGlobalObject(jscontext);
  jsval vp;

  if ((PR_TRUE != JS_LookupProperty(jscontext, global, "Navigator", &vp)) ||
      !JSVAL_IS_OBJECT(vp) ||
      ((constructor = JSVAL_TO_OBJECT(vp)) == nsnull) ||
      (PR_TRUE != JS_LookupProperty(jscontext, JSVAL_TO_OBJECT(vp), "prototype", &vp)) || 
      !JSVAL_IS_OBJECT(vp)) {

    proto = JS_InitClass(jscontext,     // context
                         global,        // global object
                         parent_proto,  // parent proto 
                         &NavigatorClass,      // JSClass
                         Navigator,            // JSNative ctor
                         0,             // ctor args
                         NavigatorProperties,  // proto props
                         NavigatorMethods,     // proto funcs
                         nsnull,        // ctor props (static)
                         nsnull);       // ctor funcs (static)
    if (nsnull == proto) {
      return NS_ERROR_FAILURE;
    }

  }
  else if ((nsnull != constructor) && JSVAL_IS_OBJECT(vp)) {
    proto = JSVAL_TO_OBJECT(vp);
  }
  else {
    return NS_ERROR_FAILURE;
  }

  if (aPrototype) {
    *aPrototype = proto;
  }
  return NS_OK;
}


//
// Method for creating a new Navigator JavaScript object
//
extern "C" NS_DOM nsresult NS_NewScriptNavigator(nsIScriptContext *aContext, nsISupports *aSupports, nsISupports *aParent, void **aReturn)
{
  NS_PRECONDITION(nsnull != aContext && nsnull != aSupports && nsnull != aReturn, "null argument to NS_NewScriptNavigator");
  JSObject *proto;
  JSObject *parent;
  nsIScriptObjectOwner *owner;
  JSContext *jscontext = (JSContext *)aContext->GetNativeContext();
  nsresult result = NS_OK;
  nsIDOMNavigator *aNavigator;

  if (nsnull == aParent) {
    parent = nsnull;
  }
  else if (NS_OK == aParent->QueryInterface(kIScriptObjectOwnerIID, (void**)&owner)) {
    if (NS_OK != owner->GetScriptObject(aContext, (void **)&parent)) {
      NS_RELEASE(owner);
      return NS_ERROR_FAILURE;
    }
    NS_RELEASE(owner);
  }
  else {
    return NS_ERROR_FAILURE;
  }

  if (NS_OK != NS_InitNavigatorClass(aContext, (void **)&proto)) {
    return NS_ERROR_FAILURE;
  }

  result = aSupports->QueryInterface(kINavigatorIID, (void **)&aNavigator);
  if (NS_OK != result) {
    return result;
  }

  // create a js object for this class
  *aReturn = JS_NewObject(jscontext, &NavigatorClass, proto, parent);
  if (nsnull != *aReturn) {
    // connect the native object to the js object
    JS_SetPrivate(jscontext, (JSObject *)*aReturn, aNavigator);
  }
  else {
    NS_RELEASE(aNavigator);
    return NS_ERROR_FAILURE; 
  }

  return NS_OK;
}
