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
#include "nscore.h"
#include "nsIScriptContext.h"
#include "nsIJSScriptObject.h"
#include "nsIScriptObjectOwner.h"
#include "nsIScriptGlobalObject.h"
#include "nsIPtr.h"
#include "nsString.h"
#include "nsIDOMCSSStyleDeclaration.h"


static NS_DEFINE_IID(kIScriptObjectOwnerIID, NS_ISCRIPTOBJECTOWNER_IID);
static NS_DEFINE_IID(kIJSScriptObjectIID, NS_IJSSCRIPTOBJECT_IID);
static NS_DEFINE_IID(kIScriptGlobalObjectIID, NS_ISCRIPTGLOBALOBJECT_IID);
static NS_DEFINE_IID(kICSSStyleDeclarationIID, NS_IDOMCSSSTYLEDECLARATION_IID);

NS_DEF_PTR(nsIDOMCSSStyleDeclaration);

//
// CSSStyleDeclaration property ids
//
enum CSSStyleDeclaration_slots {
  CSSSTYLEDECLARATION_CSSTEXT = -1,
  CSSSTYLEDECLARATION_LENGTH = -2
};

/***********************************************************************/
//
// CSSStyleDeclaration Properties Getter
//
PR_STATIC_CALLBACK(JSBool)
GetCSSStyleDeclarationProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  nsIDOMCSSStyleDeclaration *a = (nsIDOMCSSStyleDeclaration*)JS_GetPrivate(cx, obj);

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == a) {
    return JS_TRUE;
  }

  if (JSVAL_IS_INT(id)) {
    switch(JSVAL_TO_INT(id)) {
      case CSSSTYLEDECLARATION_CSSTEXT:
      {
        nsAutoString prop;
        if (NS_OK == a->GetCssText(prop)) {
          JSString *jsstring = JS_NewUCStringCopyN(cx, prop, prop.Length());
          // set the return value
          *vp = STRING_TO_JSVAL(jsstring);
        }
        else {
          return JS_FALSE;
        }
        break;
      }
      case CSSSTYLEDECLARATION_LENGTH:
      {
        PRUint32 prop;
        if (NS_OK == a->GetLength(&prop)) {
          *vp = INT_TO_JSVAL(prop);
        }
        else {
          return JS_FALSE;
        }
        break;
      }
      default:
      {
        nsAutoString prop;
        if (NS_OK == a->Item(JSVAL_TO_INT(id), prop)) {
          JSString *jsstring = JS_NewUCStringCopyN(cx, prop, prop.Length());
          // set the return value
          *vp = STRING_TO_JSVAL(jsstring);
        }
        else {
          return JS_FALSE;
        }
      }
    }
  }
  else {
    nsIJSScriptObject *object;
    if (NS_OK == a->QueryInterface(kIJSScriptObjectIID, (void**)&object)) {
      PRBool rval;
      rval =  object->GetProperty(cx, id, vp);
      NS_RELEASE(object);
      return rval;
    }
  }

  return PR_TRUE;
}

/***********************************************************************/
//
// CSSStyleDeclaration Properties Setter
//
PR_STATIC_CALLBACK(JSBool)
SetCSSStyleDeclarationProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  nsIDOMCSSStyleDeclaration *a = (nsIDOMCSSStyleDeclaration*)JS_GetPrivate(cx, obj);

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == a) {
    return JS_TRUE;
  }

  if (JSVAL_IS_INT(id)) {
    switch(JSVAL_TO_INT(id)) {
      case CSSSTYLEDECLARATION_CSSTEXT:
      {
        nsAutoString prop;
        JSString *jsstring;
        if ((jsstring = JS_ValueToString(cx, *vp)) != nsnull) {
          prop.SetString(JS_GetStringChars(jsstring));
        }
        else {
          prop.SetString((const char *)nsnull);
        }
      
        a->SetCssText(prop);
        
        break;
      }
      default:
      {
        nsIJSScriptObject *object;
        if (NS_OK == a->QueryInterface(kIJSScriptObjectIID, (void**)&object)) {
          PRBool rval;
          rval =  object->SetProperty(cx, id, vp);
          NS_RELEASE(object);
          return rval;
        }
      }
    }
  }
  else {
    nsIJSScriptObject *object;
    if (NS_OK == a->QueryInterface(kIJSScriptObjectIID, (void**)&object)) {
      PRBool rval;
      rval =  object->SetProperty(cx, id, vp);
      NS_RELEASE(object);
      return rval;
    }
  }

  return PR_TRUE;
}


//
// CSSStyleDeclaration finalizer
//
PR_STATIC_CALLBACK(void)
FinalizeCSSStyleDeclaration(JSContext *cx, JSObject *obj)
{
  nsIDOMCSSStyleDeclaration *a = (nsIDOMCSSStyleDeclaration*)JS_GetPrivate(cx, obj);
  
  if (nsnull != a) {
    // get the js object
    nsIScriptObjectOwner *owner = nsnull;
    if (NS_OK == a->QueryInterface(kIScriptObjectOwnerIID, (void**)&owner)) {
      owner->SetScriptObject(nsnull);
      NS_RELEASE(owner);
    }

    NS_RELEASE(a);
  }
}


//
// CSSStyleDeclaration enumerate
//
PR_STATIC_CALLBACK(JSBool)
EnumerateCSSStyleDeclaration(JSContext *cx, JSObject *obj)
{
  nsIDOMCSSStyleDeclaration *a = (nsIDOMCSSStyleDeclaration*)JS_GetPrivate(cx, obj);
  
  if (nsnull != a) {
    // get the js object
    nsIJSScriptObject *object;
    if (NS_OK == a->QueryInterface(kIJSScriptObjectIID, (void**)&object)) {
      object->EnumerateProperty(cx);
      NS_RELEASE(object);
    }
  }
  return JS_TRUE;
}


//
// CSSStyleDeclaration resolve
//
PR_STATIC_CALLBACK(JSBool)
ResolveCSSStyleDeclaration(JSContext *cx, JSObject *obj, jsval id)
{
  nsIDOMCSSStyleDeclaration *a = (nsIDOMCSSStyleDeclaration*)JS_GetPrivate(cx, obj);
  
  if (nsnull != a) {
    // get the js object
    nsIJSScriptObject *object;
    if (NS_OK == a->QueryInterface(kIJSScriptObjectIID, (void**)&object)) {
      object->Resolve(cx, id);
      NS_RELEASE(object);
    }
  }
  return JS_TRUE;
}


//
// Native method GetPropertyValue
//
PR_STATIC_CALLBACK(JSBool)
CSSStyleDeclarationGetPropertyValue(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsIDOMCSSStyleDeclaration *nativeThis = (nsIDOMCSSStyleDeclaration*)JS_GetPrivate(cx, obj);
  JSBool rBool = JS_FALSE;
  nsAutoString nativeRet;
  nsAutoString b0;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == nativeThis) {
    return JS_TRUE;
  }

  if (argc >= 1) {

    JSString *jsstring0 = JS_ValueToString(cx, argv[0]);
    if (nsnull != jsstring0) {
      b0.SetString(JS_GetStringChars(jsstring0));
    }
    else {
      b0.SetString("");   // Should this really be null?? 
    }

    if (NS_OK != nativeThis->GetPropertyValue(b0, nativeRet)) {
      return JS_FALSE;
    }

    JSString *jsstring = JS_NewUCStringCopyN(cx, nativeRet, nativeRet.Length());
    // set the return value
    *rval = STRING_TO_JSVAL(jsstring);
  }
  else {
    JS_ReportError(cx, "Function getPropertyValue requires 1 parameters");
    return JS_FALSE;
  }

  return JS_TRUE;
}


//
// Native method GetPropertyPriority
//
PR_STATIC_CALLBACK(JSBool)
CSSStyleDeclarationGetPropertyPriority(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsIDOMCSSStyleDeclaration *nativeThis = (nsIDOMCSSStyleDeclaration*)JS_GetPrivate(cx, obj);
  JSBool rBool = JS_FALSE;
  nsAutoString nativeRet;
  nsAutoString b0;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == nativeThis) {
    return JS_TRUE;
  }

  if (argc >= 1) {

    JSString *jsstring0 = JS_ValueToString(cx, argv[0]);
    if (nsnull != jsstring0) {
      b0.SetString(JS_GetStringChars(jsstring0));
    }
    else {
      b0.SetString("");   // Should this really be null?? 
    }

    if (NS_OK != nativeThis->GetPropertyPriority(b0, nativeRet)) {
      return JS_FALSE;
    }

    JSString *jsstring = JS_NewUCStringCopyN(cx, nativeRet, nativeRet.Length());
    // set the return value
    *rval = STRING_TO_JSVAL(jsstring);
  }
  else {
    JS_ReportError(cx, "Function getPropertyPriority requires 1 parameters");
    return JS_FALSE;
  }

  return JS_TRUE;
}


//
// Native method SetProperty
//
PR_STATIC_CALLBACK(JSBool)
CSSStyleDeclarationSetProperty(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsIDOMCSSStyleDeclaration *nativeThis = (nsIDOMCSSStyleDeclaration*)JS_GetPrivate(cx, obj);
  JSBool rBool = JS_FALSE;
  nsAutoString b0;
  nsAutoString b1;
  nsAutoString b2;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == nativeThis) {
    return JS_TRUE;
  }

  if (argc >= 3) {

    JSString *jsstring0 = JS_ValueToString(cx, argv[0]);
    if (nsnull != jsstring0) {
      b0.SetString(JS_GetStringChars(jsstring0));
    }
    else {
      b0.SetString("");   // Should this really be null?? 
    }

    JSString *jsstring1 = JS_ValueToString(cx, argv[1]);
    if (nsnull != jsstring1) {
      b1.SetString(JS_GetStringChars(jsstring1));
    }
    else {
      b1.SetString("");   // Should this really be null?? 
    }

    JSString *jsstring2 = JS_ValueToString(cx, argv[2]);
    if (nsnull != jsstring2) {
      b2.SetString(JS_GetStringChars(jsstring2));
    }
    else {
      b2.SetString("");   // Should this really be null?? 
    }

    if (NS_OK != nativeThis->SetProperty(b0, b1, b2)) {
      return JS_FALSE;
    }

    *rval = JSVAL_VOID;
  }
  else {
    JS_ReportError(cx, "Function setProperty requires 3 parameters");
    return JS_FALSE;
  }

  return JS_TRUE;
}


//
// Native method Item
//
PR_STATIC_CALLBACK(JSBool)
CSSStyleDeclarationItem(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsIDOMCSSStyleDeclaration *nativeThis = (nsIDOMCSSStyleDeclaration*)JS_GetPrivate(cx, obj);
  JSBool rBool = JS_FALSE;
  nsAutoString nativeRet;
  PRUint32 b0;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == nativeThis) {
    return JS_TRUE;
  }

  if (argc >= 1) {

    if (!JS_ValueToInt32(cx, argv[0], (int32 *)&b0)) {
      JS_ReportError(cx, "Parameter must be a number");
      return JS_FALSE;
    }

    if (NS_OK != nativeThis->Item(b0, nativeRet)) {
      return JS_FALSE;
    }

    JSString *jsstring = JS_NewUCStringCopyN(cx, nativeRet, nativeRet.Length());
    // set the return value
    *rval = STRING_TO_JSVAL(jsstring);
  }
  else {
    JS_ReportError(cx, "Function item requires 1 parameters");
    return JS_FALSE;
  }

  return JS_TRUE;
}


/***********************************************************************/
//
// class for CSSStyleDeclaration
//
JSClass CSSStyleDeclarationClass = {
  "CSSStyleDeclaration", 
  JSCLASS_HAS_PRIVATE,
  JS_PropertyStub,
  JS_PropertyStub,
  GetCSSStyleDeclarationProperty,
  SetCSSStyleDeclarationProperty,
  EnumerateCSSStyleDeclaration,
  ResolveCSSStyleDeclaration,
  JS_ConvertStub,
  FinalizeCSSStyleDeclaration
};


//
// CSSStyleDeclaration class properties
//
static JSPropertySpec CSSStyleDeclarationProperties[] =
{
  {"cssText",    CSSSTYLEDECLARATION_CSSTEXT,    JSPROP_ENUMERATE},
  {"length",    CSSSTYLEDECLARATION_LENGTH,    JSPROP_ENUMERATE | JSPROP_READONLY},
  {0}
};


//
// CSSStyleDeclaration class methods
//
static JSFunctionSpec CSSStyleDeclarationMethods[] = 
{
  {"getPropertyValue",          CSSStyleDeclarationGetPropertyValue,     1},
  {"getPropertyPriority",          CSSStyleDeclarationGetPropertyPriority,     1},
  {"setProperty",          CSSStyleDeclarationSetProperty,     3},
  {"item",          CSSStyleDeclarationItem,     1},
  {0}
};


//
// CSSStyleDeclaration constructor
//
PR_STATIC_CALLBACK(JSBool)
CSSStyleDeclaration(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  return JS_FALSE;
}


//
// CSSStyleDeclaration class initialization
//
nsresult NS_InitCSSStyleDeclarationClass(nsIScriptContext *aContext, void **aPrototype)
{
  JSContext *jscontext = (JSContext *)aContext->GetNativeContext();
  JSObject *proto = nsnull;
  JSObject *constructor = nsnull;
  JSObject *parent_proto = nsnull;
  JSObject *global = JS_GetGlobalObject(jscontext);
  jsval vp;

  if ((PR_TRUE != JS_LookupProperty(jscontext, global, "CSSStyleDeclaration", &vp)) ||
      !JSVAL_IS_OBJECT(vp) ||
      ((constructor = JSVAL_TO_OBJECT(vp)) == nsnull) ||
      (PR_TRUE != JS_LookupProperty(jscontext, JSVAL_TO_OBJECT(vp), "prototype", &vp)) || 
      !JSVAL_IS_OBJECT(vp)) {

    proto = JS_InitClass(jscontext,     // context
                         global,        // global object
                         parent_proto,  // parent proto 
                         &CSSStyleDeclarationClass,      // JSClass
                         CSSStyleDeclaration,            // JSNative ctor
                         0,             // ctor args
                         CSSStyleDeclarationProperties,  // proto props
                         CSSStyleDeclarationMethods,     // proto funcs
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
// Method for creating a new CSSStyleDeclaration JavaScript object
//
extern "C" NS_DOM nsresult NS_NewScriptCSSStyleDeclaration(nsIScriptContext *aContext, nsISupports *aSupports, nsISupports *aParent, void **aReturn)
{
  NS_PRECONDITION(nsnull != aContext && nsnull != aSupports && nsnull != aReturn, "null argument to NS_NewScriptCSSStyleDeclaration");
  JSObject *proto;
  JSObject *parent;
  nsIScriptObjectOwner *owner;
  JSContext *jscontext = (JSContext *)aContext->GetNativeContext();
  nsresult result = NS_OK;
  nsIDOMCSSStyleDeclaration *aCSSStyleDeclaration;

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

  if (NS_OK != NS_InitCSSStyleDeclarationClass(aContext, (void **)&proto)) {
    return NS_ERROR_FAILURE;
  }

  result = aSupports->QueryInterface(kICSSStyleDeclarationIID, (void **)&aCSSStyleDeclaration);
  if (NS_OK != result) {
    return result;
  }

  // create a js object for this class
  *aReturn = JS_NewObject(jscontext, &CSSStyleDeclarationClass, proto, parent);
  if (nsnull != *aReturn) {
    // connect the native object to the js object
    JS_SetPrivate(jscontext, (JSObject *)*aReturn, aCSSStyleDeclaration);
  }
  else {
    NS_RELEASE(aCSSStyleDeclaration);
    return NS_ERROR_FAILURE; 
  }

  return NS_OK;
}
