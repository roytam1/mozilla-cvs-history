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
#include "nsIPtr.h"
#include "nsString.h"
#include "nsIDOMCSSStyleDeclaration.h"
#include "nsIDOMCSSFontFaceRule.h"


static NS_DEFINE_IID(kIScriptObjectOwnerIID, NS_ISCRIPTOBJECTOWNER_IID);
static NS_DEFINE_IID(kIJSScriptObjectIID, NS_IJSSCRIPTOBJECT_IID);
static NS_DEFINE_IID(kIScriptGlobalObjectIID, NS_ISCRIPTGLOBALOBJECT_IID);
static NS_DEFINE_IID(kICSSStyleDeclarationIID, NS_IDOMCSSSTYLEDECLARATION_IID);
static NS_DEFINE_IID(kICSSFontFaceRuleIID, NS_IDOMCSSFONTFACERULE_IID);

NS_DEF_PTR(nsIDOMCSSStyleDeclaration);
NS_DEF_PTR(nsIDOMCSSFontFaceRule);

//
// CSSFontFaceRule property ids
//
enum CSSFontFaceRule_slots {
  CSSFONTFACERULE_STYLE = -1
};

/***********************************************************************/
//
// CSSFontFaceRule Properties Getter
//
PR_STATIC_CALLBACK(JSBool)
GetCSSFontFaceRuleProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  nsIDOMCSSFontFaceRule *a = (nsIDOMCSSFontFaceRule*)nsJSUtils::nsGetNativeThis(cx, obj);

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == a) {
    return JS_TRUE;
  }

  if (JSVAL_IS_INT(id)) {
    nsIScriptContext *scriptCX = (nsIScriptContext *)JS_GetContextPrivate(cx);
    nsIScriptSecurityManager *secMan;
    PRBool ok = PR_FALSE;
    if (NS_OK != scriptCX->GetSecurityManager(&secMan)) {
      return JS_FALSE;
    }
    switch(JSVAL_TO_INT(id)) {
      case CSSFONTFACERULE_STYLE:
      {
        secMan->CheckScriptAccess(scriptCX, obj, "cssfontfacerule.style", &ok);
        if (!ok) {
          //Need to throw error here
          return JS_FALSE;
        }
        nsIDOMCSSStyleDeclaration* prop;
        if (NS_SUCCEEDED(a->GetStyle(&prop))) {
          // get the js object
          nsJSUtils::nsConvertObjectToJSVal((nsISupports *)prop, cx, vp);
        }
        else {
          return JS_FALSE;
        }
        break;
      }
      default:
        return nsJSUtils::nsCallJSScriptObjectGetProperty(a, cx, id, vp);
    }
    NS_RELEASE(secMan);
  }
  else {
    return nsJSUtils::nsCallJSScriptObjectGetProperty(a, cx, id, vp);
  }

  return PR_TRUE;
}

/***********************************************************************/
//
// CSSFontFaceRule Properties Setter
//
PR_STATIC_CALLBACK(JSBool)
SetCSSFontFaceRuleProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  nsIDOMCSSFontFaceRule *a = (nsIDOMCSSFontFaceRule*)nsJSUtils::nsGetNativeThis(cx, obj);

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == a) {
    return JS_TRUE;
  }

  if (JSVAL_IS_INT(id)) {
    nsIScriptContext *scriptCX = (nsIScriptContext *)JS_GetContextPrivate(cx);
    nsIScriptSecurityManager *secMan;
    PRBool ok = PR_FALSE;
    if (NS_OK != scriptCX->GetSecurityManager(&secMan)) {
      return JS_FALSE;
    }
    switch(JSVAL_TO_INT(id)) {
      case CSSFONTFACERULE_STYLE:
      {
        secMan->CheckScriptAccess(scriptCX, obj, "cssfontfacerule.style", &ok);
        if (!ok) {
          //Need to throw error here
          return JS_FALSE;
        }
        nsIDOMCSSStyleDeclaration* prop;
        if (PR_FALSE == nsJSUtils::nsConvertJSValToObject((nsISupports **)&prop,
                                                kICSSStyleDeclarationIID, "CSSStyleDeclaration",
                                                cx, *vp)) {
          return JS_FALSE;
        }
      
        a->SetStyle(prop);
        NS_IF_RELEASE(prop);
        break;
      }
      default:
        return nsJSUtils::nsCallJSScriptObjectSetProperty(a, cx, id, vp);
    }
    NS_RELEASE(secMan);
  }
  else {
    return nsJSUtils::nsCallJSScriptObjectSetProperty(a, cx, id, vp);
  }

  return PR_TRUE;
}


//
// CSSFontFaceRule finalizer
//
PR_STATIC_CALLBACK(void)
FinalizeCSSFontFaceRule(JSContext *cx, JSObject *obj)
{
  nsJSUtils::nsGenericFinalize(cx, obj);
}


//
// CSSFontFaceRule enumerate
//
PR_STATIC_CALLBACK(JSBool)
EnumerateCSSFontFaceRule(JSContext *cx, JSObject *obj)
{
  return nsJSUtils::nsGenericEnumerate(cx, obj);
}


//
// CSSFontFaceRule resolve
//
PR_STATIC_CALLBACK(JSBool)
ResolveCSSFontFaceRule(JSContext *cx, JSObject *obj, jsval id)
{
  return nsJSUtils::nsGenericResolve(cx, obj, id);
}


/***********************************************************************/
//
// class for CSSFontFaceRule
//
JSClass CSSFontFaceRuleClass = {
  "CSSFontFaceRule", 
  JSCLASS_HAS_PRIVATE | JSCLASS_PRIVATE_IS_NSISUPPORTS,
  JS_PropertyStub,
  JS_PropertyStub,
  GetCSSFontFaceRuleProperty,
  SetCSSFontFaceRuleProperty,
  EnumerateCSSFontFaceRule,
  ResolveCSSFontFaceRule,
  JS_ConvertStub,
  FinalizeCSSFontFaceRule
};


//
// CSSFontFaceRule class properties
//
static JSPropertySpec CSSFontFaceRuleProperties[] =
{
  {"style",    CSSFONTFACERULE_STYLE,    JSPROP_ENUMERATE},
  {0}
};


//
// CSSFontFaceRule class methods
//
static JSFunctionSpec CSSFontFaceRuleMethods[] = 
{
  {0}
};


//
// CSSFontFaceRule constructor
//
PR_STATIC_CALLBACK(JSBool)
CSSFontFaceRule(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  return JS_FALSE;
}


//
// CSSFontFaceRule class initialization
//
extern "C" NS_DOM nsresult NS_InitCSSFontFaceRuleClass(nsIScriptContext *aContext, void **aPrototype)
{
  JSContext *jscontext = (JSContext *)aContext->GetNativeContext();
  JSObject *proto = nsnull;
  JSObject *constructor = nsnull;
  JSObject *parent_proto = nsnull;
  JSObject *global = JS_GetGlobalObject(jscontext);
  jsval vp;

  if ((PR_TRUE != JS_LookupProperty(jscontext, global, "CSSFontFaceRule", &vp)) ||
      !JSVAL_IS_OBJECT(vp) ||
      ((constructor = JSVAL_TO_OBJECT(vp)) == nsnull) ||
      (PR_TRUE != JS_LookupProperty(jscontext, JSVAL_TO_OBJECT(vp), "prototype", &vp)) || 
      !JSVAL_IS_OBJECT(vp)) {

    if (NS_OK != NS_InitCSSRuleClass(aContext, (void **)&parent_proto)) {
      return NS_ERROR_FAILURE;
    }
    proto = JS_InitClass(jscontext,     // context
                         global,        // global object
                         parent_proto,  // parent proto 
                         &CSSFontFaceRuleClass,      // JSClass
                         CSSFontFaceRule,            // JSNative ctor
                         0,             // ctor args
                         CSSFontFaceRuleProperties,  // proto props
                         CSSFontFaceRuleMethods,     // proto funcs
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
// Method for creating a new CSSFontFaceRule JavaScript object
//
extern "C" NS_DOM nsresult NS_NewScriptCSSFontFaceRule(nsIScriptContext *aContext, nsISupports *aSupports, nsISupports *aParent, void **aReturn)
{
  NS_PRECONDITION(nsnull != aContext && nsnull != aSupports && nsnull != aReturn, "null argument to NS_NewScriptCSSFontFaceRule");
  JSObject *proto;
  JSObject *parent;
  nsIScriptObjectOwner *owner;
  JSContext *jscontext = (JSContext *)aContext->GetNativeContext();
  nsresult result = NS_OK;
  nsIDOMCSSFontFaceRule *aCSSFontFaceRule;

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

  if (NS_OK != NS_InitCSSFontFaceRuleClass(aContext, (void **)&proto)) {
    return NS_ERROR_FAILURE;
  }

  result = aSupports->QueryInterface(kICSSFontFaceRuleIID, (void **)&aCSSFontFaceRule);
  if (NS_OK != result) {
    return result;
  }

  // create a js object for this class
  *aReturn = JS_NewObject(jscontext, &CSSFontFaceRuleClass, proto, parent);
  if (nsnull != *aReturn) {
    // connect the native object to the js object
    JS_SetPrivate(jscontext, (JSObject *)*aReturn, aCSSFontFaceRule);
  }
  else {
    NS_RELEASE(aCSSFontFaceRule);
    return NS_ERROR_FAILURE; 
  }

  return NS_OK;
}
