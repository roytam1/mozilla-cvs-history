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
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */
/* AUTO-GENERATED. DO NOT EDIT!!! */

#include "jsapi.h"
#include "nsJSUtils.h"
#include "nsDOMError.h"
#include "nscore.h"
#include "nsIScriptContext.h"
#include "nsIScriptSecurityManager.h"
#include "nsIJSScriptObject.h"
#include "nsIScriptObjectOwner.h"
#include "nsIScriptGlobalObject.h"
#include "nsCOMPtr.h"
#include "nsDOMPropEnums.h"
#include "nsIPtr.h"
#include "nsString.h"
#include "nsIDOMCSSStyleRuleCollection.h"
#include "nsIDOMCSSMediaRule.h"


static NS_DEFINE_IID(kIScriptObjectOwnerIID, NS_ISCRIPTOBJECTOWNER_IID);
static NS_DEFINE_IID(kIJSScriptObjectIID, NS_IJSSCRIPTOBJECT_IID);
static NS_DEFINE_IID(kIScriptGlobalObjectIID, NS_ISCRIPTGLOBALOBJECT_IID);
static NS_DEFINE_IID(kICSSStyleRuleCollectionIID, NS_IDOMCSSSTYLERULECOLLECTION_IID);
static NS_DEFINE_IID(kICSSMediaRuleIID, NS_IDOMCSSMEDIARULE_IID);

NS_DEF_PTR(nsIDOMCSSStyleRuleCollection);
NS_DEF_PTR(nsIDOMCSSMediaRule);

//
// CSSMediaRule property ids
//
enum CSSMediaRule_slots {
  CSSMEDIARULE_MEDIATYPES = -1,
  CSSMEDIARULE_CSSRULES = -2
};

/***********************************************************************/
//
// CSSMediaRule Properties Getter
//
PR_STATIC_CALLBACK(JSBool)
GetCSSMediaRuleProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  nsIDOMCSSMediaRule *a = (nsIDOMCSSMediaRule*)nsJSUtils::nsGetNativeThis(cx, obj);

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == a) {
    return JS_TRUE;
  }

  if (JSVAL_IS_INT(id)) {
    nsIScriptContext *scriptCX = (nsIScriptContext *)JS_GetContextPrivate(cx);
    nsCOMPtr<nsIScriptSecurityManager> secMan;
    if (NS_OK != scriptCX->GetSecurityManager(getter_AddRefs(secMan))) {
      return nsJSUtils::nsReportError(cx, NS_ERROR_DOM_SECMAN_ERR);
    }
    switch(JSVAL_TO_INT(id)) {
      case CSSMEDIARULE_MEDIATYPES:
      {
        PRBool ok = PR_FALSE;
        secMan->CheckScriptAccess(scriptCX, obj, NS_DOM_PROP_CSSMEDIARULE_MEDIATYPES, PR_FALSE, &ok);
        if (!ok) {
          return nsJSUtils::nsReportError(cx, NS_ERROR_DOM_SECURITY_ERR);
        }
        nsAutoString prop;
        nsresult result = NS_OK;
        result = a->GetMediaTypes(prop);
        if (NS_SUCCEEDED(result)) {
          nsJSUtils::nsConvertStringToJSVal(prop, cx, vp);
        }
        else {
          return nsJSUtils::nsReportError(cx, result);
        }
        break;
      }
      case CSSMEDIARULE_CSSRULES:
      {
        PRBool ok = PR_FALSE;
        secMan->CheckScriptAccess(scriptCX, obj, NS_DOM_PROP_CSSMEDIARULE_CSSRULES, PR_FALSE, &ok);
        if (!ok) {
          return nsJSUtils::nsReportError(cx, NS_ERROR_DOM_SECURITY_ERR);
        }
        nsIDOMCSSStyleRuleCollection* prop;
        nsresult result = NS_OK;
        result = a->GetCssRules(&prop);
        if (NS_SUCCEEDED(result)) {
          // get the js object
          nsJSUtils::nsConvertObjectToJSVal((nsISupports *)prop, cx, vp);
        }
        else {
          return nsJSUtils::nsReportError(cx, result);
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
// CSSMediaRule Properties Setter
//
PR_STATIC_CALLBACK(JSBool)
SetCSSMediaRuleProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  nsIDOMCSSMediaRule *a = (nsIDOMCSSMediaRule*)nsJSUtils::nsGetNativeThis(cx, obj);

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == a) {
    return JS_TRUE;
  }

  if (JSVAL_IS_INT(id)) {
    nsIScriptContext *scriptCX = (nsIScriptContext *)JS_GetContextPrivate(cx);
    nsCOMPtr<nsIScriptSecurityManager> secMan;
    if (NS_OK != scriptCX->GetSecurityManager(getter_AddRefs(secMan))) {
      return nsJSUtils::nsReportError(cx, NS_ERROR_DOM_SECMAN_ERR);
    }
    switch(JSVAL_TO_INT(id)) {
      case CSSMEDIARULE_MEDIATYPES:
      {
        PRBool ok = PR_FALSE;
        secMan->CheckScriptAccess(scriptCX, obj, NS_DOM_PROP_CSSMEDIARULE_MEDIATYPES, PR_TRUE, &ok);
        if (!ok) {
          return nsJSUtils::nsReportError(cx, NS_ERROR_DOM_SECURITY_ERR);
        }
        nsAutoString prop;
        nsJSUtils::nsConvertJSValToString(prop, cx, *vp);
      
        a->SetMediaTypes(prop);
        
        break;
      }
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
// CSSMediaRule finalizer
//
PR_STATIC_CALLBACK(void)
FinalizeCSSMediaRule(JSContext *cx, JSObject *obj)
{
  nsJSUtils::nsGenericFinalize(cx, obj);
}


//
// CSSMediaRule enumerate
//
PR_STATIC_CALLBACK(JSBool)
EnumerateCSSMediaRule(JSContext *cx, JSObject *obj)
{
  return nsJSUtils::nsGenericEnumerate(cx, obj);
}


//
// CSSMediaRule resolve
//
PR_STATIC_CALLBACK(JSBool)
ResolveCSSMediaRule(JSContext *cx, JSObject *obj, jsval id)
{
  return nsJSUtils::nsGenericResolve(cx, obj, id);
}


//
// Native method InsertRule
//
PR_STATIC_CALLBACK(JSBool)
CSSMediaRuleInsertRule(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsIDOMCSSMediaRule *nativeThis = (nsIDOMCSSMediaRule*)nsJSUtils::nsGetNativeThis(cx, obj);
  nsresult result = NS_OK;
  PRUint32 nativeRet;
  nsAutoString b0;
  PRUint32 b1;

  *rval = JSVAL_NULL;

  nsIScriptContext *scriptCX = (nsIScriptContext *)JS_GetContextPrivate(cx);
  nsCOMPtr<nsIScriptSecurityManager> secMan;
  if (NS_OK != scriptCX->GetSecurityManager(getter_AddRefs(secMan))) {
    return nsJSUtils::nsReportError(cx, NS_ERROR_DOM_SECMAN_ERR);
  }
  {
    PRBool ok;
    secMan->CheckScriptAccess(scriptCX, obj, NS_DOM_PROP_CSSMEDIARULE_INSERTRULE,PR_FALSE , &ok);
    if (!ok) {
      return nsJSUtils::nsReportError(cx, NS_ERROR_DOM_SECURITY_ERR);
    }
  }

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == nativeThis) {
    return JS_TRUE;
  }

  {
    if (argc < 2) {
      return nsJSUtils::nsReportError(cx, NS_ERROR_DOM_TOO_FEW_PARAMETERS_ERR);
    }

    nsJSUtils::nsConvertJSValToString(b0, cx, argv[0]);
    if (!JS_ValueToInt32(cx, argv[1], (int32 *)&b1)) {
      return nsJSUtils::nsReportError(cx, NS_ERROR_DOM_NOT_NUMBER_ERR);
    }

    result = nativeThis->InsertRule(b0, b1, &nativeRet);
    if (NS_FAILED(result)) {
      return nsJSUtils::nsReportError(cx, result);
    }

    *rval = INT_TO_JSVAL(nativeRet);
  }

  return JS_TRUE;
}


//
// Native method DeleteRule
//
PR_STATIC_CALLBACK(JSBool)
CSSMediaRuleDeleteRule(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsIDOMCSSMediaRule *nativeThis = (nsIDOMCSSMediaRule*)nsJSUtils::nsGetNativeThis(cx, obj);
  nsresult result = NS_OK;
  PRUint32 b0;

  *rval = JSVAL_NULL;

  nsIScriptContext *scriptCX = (nsIScriptContext *)JS_GetContextPrivate(cx);
  nsCOMPtr<nsIScriptSecurityManager> secMan;
  if (NS_OK != scriptCX->GetSecurityManager(getter_AddRefs(secMan))) {
    return nsJSUtils::nsReportError(cx, NS_ERROR_DOM_SECMAN_ERR);
  }
  {
    PRBool ok;
    secMan->CheckScriptAccess(scriptCX, obj, NS_DOM_PROP_CSSMEDIARULE_DELETERULE,PR_FALSE , &ok);
    if (!ok) {
      return nsJSUtils::nsReportError(cx, NS_ERROR_DOM_SECURITY_ERR);
    }
  }

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == nativeThis) {
    return JS_TRUE;
  }

  {
    if (argc < 1) {
      return nsJSUtils::nsReportError(cx, NS_ERROR_DOM_TOO_FEW_PARAMETERS_ERR);
    }

    if (!JS_ValueToInt32(cx, argv[0], (int32 *)&b0)) {
      return nsJSUtils::nsReportError(cx, NS_ERROR_DOM_NOT_NUMBER_ERR);
    }

    result = nativeThis->DeleteRule(b0);
    if (NS_FAILED(result)) {
      return nsJSUtils::nsReportError(cx, result);
    }

    *rval = JSVAL_VOID;
  }

  return JS_TRUE;
}


/***********************************************************************/
//
// class for CSSMediaRule
//
JSClass CSSMediaRuleClass = {
  "CSSMediaRule", 
  JSCLASS_HAS_PRIVATE | JSCLASS_PRIVATE_IS_NSISUPPORTS,
  JS_PropertyStub,
  JS_PropertyStub,
  GetCSSMediaRuleProperty,
  SetCSSMediaRuleProperty,
  EnumerateCSSMediaRule,
  ResolveCSSMediaRule,
  JS_ConvertStub,
  FinalizeCSSMediaRule
};


//
// CSSMediaRule class properties
//
static JSPropertySpec CSSMediaRuleProperties[] =
{
  {"mediaTypes",    CSSMEDIARULE_MEDIATYPES,    JSPROP_ENUMERATE},
  {"cssRules",    CSSMEDIARULE_CSSRULES,    JSPROP_ENUMERATE | JSPROP_READONLY},
  {0}
};


//
// CSSMediaRule class methods
//
static JSFunctionSpec CSSMediaRuleMethods[] = 
{
  {"insertRule",          CSSMediaRuleInsertRule,     2},
  {"deleteRule",          CSSMediaRuleDeleteRule,     1},
  {0}
};


//
// CSSMediaRule constructor
//
PR_STATIC_CALLBACK(JSBool)
CSSMediaRule(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  return JS_FALSE;
}


//
// CSSMediaRule class initialization
//
extern "C" NS_DOM nsresult NS_InitCSSMediaRuleClass(nsIScriptContext *aContext, void **aPrototype)
{
  JSContext *jscontext = (JSContext *)aContext->GetNativeContext();
  JSObject *proto = nsnull;
  JSObject *constructor = nsnull;
  JSObject *parent_proto = nsnull;
  JSObject *global = JS_GetGlobalObject(jscontext);
  jsval vp;

  if ((PR_TRUE != JS_LookupProperty(jscontext, global, "CSSMediaRule", &vp)) ||
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
                         &CSSMediaRuleClass,      // JSClass
                         CSSMediaRule,            // JSNative ctor
                         0,             // ctor args
                         CSSMediaRuleProperties,  // proto props
                         CSSMediaRuleMethods,     // proto funcs
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
// Method for creating a new CSSMediaRule JavaScript object
//
extern "C" NS_DOM nsresult NS_NewScriptCSSMediaRule(nsIScriptContext *aContext, nsISupports *aSupports, nsISupports *aParent, void **aReturn)
{
  NS_PRECONDITION(nsnull != aContext && nsnull != aSupports && nsnull != aReturn, "null argument to NS_NewScriptCSSMediaRule");
  JSObject *proto;
  JSObject *parent;
  nsIScriptObjectOwner *owner;
  JSContext *jscontext = (JSContext *)aContext->GetNativeContext();
  nsresult result = NS_OK;
  nsIDOMCSSMediaRule *aCSSMediaRule;

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

  if (NS_OK != NS_InitCSSMediaRuleClass(aContext, (void **)&proto)) {
    return NS_ERROR_FAILURE;
  }

  result = aSupports->QueryInterface(kICSSMediaRuleIID, (void **)&aCSSMediaRule);
  if (NS_OK != result) {
    return result;
  }

  // create a js object for this class
  *aReturn = JS_NewObject(jscontext, &CSSMediaRuleClass, proto, parent);
  if (nsnull != *aReturn) {
    // connect the native object to the js object
    JS_SetPrivate(jscontext, (JSObject *)*aReturn, aCSSMediaRule);
  }
  else {
    NS_RELEASE(aCSSMediaRule);
    return NS_ERROR_FAILURE; 
  }

  return NS_OK;
}
