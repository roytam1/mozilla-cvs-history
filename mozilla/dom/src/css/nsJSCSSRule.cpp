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
#include "nsIDOMCSSRule.h"
#include "nsIDOMCSSStyleSheet.h"


static NS_DEFINE_IID(kIScriptObjectOwnerIID, NS_ISCRIPTOBJECTOWNER_IID);
static NS_DEFINE_IID(kIJSScriptObjectIID, NS_IJSSCRIPTOBJECT_IID);
static NS_DEFINE_IID(kIScriptGlobalObjectIID, NS_ISCRIPTGLOBALOBJECT_IID);
static NS_DEFINE_IID(kICSSRuleIID, NS_IDOMCSSRULE_IID);
static NS_DEFINE_IID(kICSSStyleSheetIID, NS_IDOMCSSSTYLESHEET_IID);

NS_DEF_PTR(nsIDOMCSSRule);
NS_DEF_PTR(nsIDOMCSSStyleSheet);

//
// CSSRule property ids
//
enum CSSRule_slots {
  CSSRULE_TYPE = -1,
  CSSRULE_CSSTEXT = -2,
  CSSRULE_SHEET = -3
};

/***********************************************************************/
//
// CSSRule Properties Getter
//
PR_STATIC_CALLBACK(JSBool)
GetCSSRuleProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  nsIDOMCSSRule *a = (nsIDOMCSSRule*)JS_GetPrivate(cx, obj);

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
      case CSSRULE_TYPE:
      {
        secMan->CheckScriptAccess(scriptCX, obj, "cssrule.type", &ok);
        if (!ok) {
          //Need to throw error here
          return JS_FALSE;
        }
        PRUint16 prop;
        if (NS_OK == a->GetType(&prop)) {
          *vp = INT_TO_JSVAL(prop);
        }
        else {
          return JS_FALSE;
        }
        break;
      }
      case CSSRULE_CSSTEXT:
      {
        secMan->CheckScriptAccess(scriptCX, obj, "cssrule.csstext", &ok);
        if (!ok) {
          //Need to throw error here
          return JS_FALSE;
        }
        nsAutoString prop;
        if (NS_OK == a->GetCssText(prop)) {
          nsJSUtils::nsConvertStringToJSVal(prop, cx, vp);
        }
        else {
          return JS_FALSE;
        }
        break;
      }
      case CSSRULE_SHEET:
      {
        secMan->CheckScriptAccess(scriptCX, obj, "cssrule.sheet", &ok);
        if (!ok) {
          //Need to throw error here
          return JS_FALSE;
        }
        nsIDOMCSSStyleSheet* prop;
        if (NS_OK == a->GetSheet(&prop)) {
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
// CSSRule Properties Setter
//
PR_STATIC_CALLBACK(JSBool)
SetCSSRuleProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  nsIDOMCSSRule *a = (nsIDOMCSSRule*)JS_GetPrivate(cx, obj);

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
      case CSSRULE_CSSTEXT:
      {
        secMan->CheckScriptAccess(scriptCX, obj, "cssrule.csstext", &ok);
        if (!ok) {
          //Need to throw error here
          return JS_FALSE;
        }
        nsAutoString prop;
        nsJSUtils::nsConvertJSValToString(prop, cx, *vp);
      
        a->SetCssText(prop);
        
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
// CSSRule finalizer
//
PR_STATIC_CALLBACK(void)
FinalizeCSSRule(JSContext *cx, JSObject *obj)
{
  nsJSUtils::nsGenericFinalize(cx, obj);
}


//
// CSSRule enumerate
//
PR_STATIC_CALLBACK(JSBool)
EnumerateCSSRule(JSContext *cx, JSObject *obj)
{
  return nsJSUtils::nsGenericEnumerate(cx, obj);
}


//
// CSSRule resolve
//
PR_STATIC_CALLBACK(JSBool)
ResolveCSSRule(JSContext *cx, JSObject *obj, jsval id)
{
  return nsJSUtils::nsGenericResolve(cx, obj, id);
}


/***********************************************************************/
//
// class for CSSRule
//
JSClass CSSRuleClass = {
  "CSSRule", 
  JSCLASS_HAS_PRIVATE | JSCLASS_PRIVATE_IS_NSISUPPORTS,
  JS_PropertyStub,
  JS_PropertyStub,
  GetCSSRuleProperty,
  SetCSSRuleProperty,
  EnumerateCSSRule,
  ResolveCSSRule,
  JS_ConvertStub,
  FinalizeCSSRule
};


//
// CSSRule class properties
//
static JSPropertySpec CSSRuleProperties[] =
{
  {"type",    CSSRULE_TYPE,    JSPROP_ENUMERATE | JSPROP_READONLY},
  {"cssText",    CSSRULE_CSSTEXT,    JSPROP_ENUMERATE},
  {"sheet",    CSSRULE_SHEET,    JSPROP_ENUMERATE | JSPROP_READONLY},
  {0}
};


//
// CSSRule class methods
//
static JSFunctionSpec CSSRuleMethods[] = 
{
  {0}
};


//
// CSSRule constructor
//
PR_STATIC_CALLBACK(JSBool)
CSSRule(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  return JS_FALSE;
}


//
// CSSRule class initialization
//
extern "C" NS_DOM nsresult NS_InitCSSRuleClass(nsIScriptContext *aContext, void **aPrototype)
{
  JSContext *jscontext = (JSContext *)aContext->GetNativeContext();
  JSObject *proto = nsnull;
  JSObject *constructor = nsnull;
  JSObject *parent_proto = nsnull;
  JSObject *global = JS_GetGlobalObject(jscontext);
  jsval vp;

  if ((PR_TRUE != JS_LookupProperty(jscontext, global, "CSSRule", &vp)) ||
      !JSVAL_IS_OBJECT(vp) ||
      ((constructor = JSVAL_TO_OBJECT(vp)) == nsnull) ||
      (PR_TRUE != JS_LookupProperty(jscontext, JSVAL_TO_OBJECT(vp), "prototype", &vp)) || 
      !JSVAL_IS_OBJECT(vp)) {

    proto = JS_InitClass(jscontext,     // context
                         global,        // global object
                         parent_proto,  // parent proto 
                         &CSSRuleClass,      // JSClass
                         CSSRule,            // JSNative ctor
                         0,             // ctor args
                         CSSRuleProperties,  // proto props
                         CSSRuleMethods,     // proto funcs
                         nsnull,        // ctor props (static)
                         nsnull);       // ctor funcs (static)
    if (nsnull == proto) {
      return NS_ERROR_FAILURE;
    }

    if ((PR_TRUE == JS_LookupProperty(jscontext, global, "CSSRule", &vp)) &&
        JSVAL_IS_OBJECT(vp) &&
        ((constructor = JSVAL_TO_OBJECT(vp)) != nsnull)) {
      vp = INT_TO_JSVAL(nsIDOMCSSRule::UNKNOWN_RULE);
      JS_SetProperty(jscontext, constructor, "UNKNOWN_RULE", &vp);

      vp = INT_TO_JSVAL(nsIDOMCSSRule::STYLE_RULE);
      JS_SetProperty(jscontext, constructor, "STYLE_RULE", &vp);

      vp = INT_TO_JSVAL(nsIDOMCSSRule::IMPORT_RULE);
      JS_SetProperty(jscontext, constructor, "IMPORT_RULE", &vp);

      vp = INT_TO_JSVAL(nsIDOMCSSRule::MEDIA_RULE);
      JS_SetProperty(jscontext, constructor, "MEDIA_RULE", &vp);

      vp = INT_TO_JSVAL(nsIDOMCSSRule::FONT_FACE_RULE);
      JS_SetProperty(jscontext, constructor, "FONT_FACE_RULE", &vp);

      vp = INT_TO_JSVAL(nsIDOMCSSRule::PAGE_RULE);
      JS_SetProperty(jscontext, constructor, "PAGE_RULE", &vp);

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
// Method for creating a new CSSRule JavaScript object
//
extern "C" NS_DOM nsresult NS_NewScriptCSSRule(nsIScriptContext *aContext, nsISupports *aSupports, nsISupports *aParent, void **aReturn)
{
  NS_PRECONDITION(nsnull != aContext && nsnull != aSupports && nsnull != aReturn, "null argument to NS_NewScriptCSSRule");
  JSObject *proto;
  JSObject *parent;
  nsIScriptObjectOwner *owner;
  JSContext *jscontext = (JSContext *)aContext->GetNativeContext();
  nsresult result = NS_OK;
  nsIDOMCSSRule *aCSSRule;

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

  if (NS_OK != NS_InitCSSRuleClass(aContext, (void **)&proto)) {
    return NS_ERROR_FAILURE;
  }

  result = aSupports->QueryInterface(kICSSRuleIID, (void **)&aCSSRule);
  if (NS_OK != result) {
    return result;
  }

  // create a js object for this class
  *aReturn = JS_NewObject(jscontext, &CSSRuleClass, proto, parent);
  if (nsnull != *aReturn) {
    // connect the native object to the js object
    JS_SetPrivate(jscontext, (JSObject *)*aReturn, aCSSRule);
  }
  else {
    NS_RELEASE(aCSSRule);
    return NS_ERROR_FAILURE; 
  }

  return NS_OK;
}
