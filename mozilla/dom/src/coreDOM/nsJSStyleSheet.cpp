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
#include "nsIJSScriptObject.h"
#include "nsIScriptObjectOwner.h"
#include "nsIScriptGlobalObject.h"
#include "nsIPtr.h"
#include "nsString.h"
#include "nsIDOMStyleSheet.h"


static NS_DEFINE_IID(kIScriptObjectOwnerIID, NS_ISCRIPTOBJECTOWNER_IID);
static NS_DEFINE_IID(kIJSScriptObjectIID, NS_IJSSCRIPTOBJECT_IID);
static NS_DEFINE_IID(kIScriptGlobalObjectIID, NS_ISCRIPTGLOBALOBJECT_IID);
static NS_DEFINE_IID(kIStyleSheetIID, NS_IDOMSTYLESHEET_IID);

NS_DEF_PTR(nsIDOMStyleSheet);

//
// StyleSheet property ids
//
enum StyleSheet_slots {
  STYLESHEET_TYPE = -1,
  STYLESHEET_DISABLED = -2,
  STYLESHEET_READONLY = -3
};

/***********************************************************************/
//
// StyleSheet Properties Getter
//
PR_STATIC_CALLBACK(JSBool)
GetStyleSheetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  nsIDOMStyleSheet *a = (nsIDOMStyleSheet*)JS_GetPrivate(cx, obj);

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == a) {
    return JS_TRUE;
  }

  if (JSVAL_IS_INT(id)) {
    switch(JSVAL_TO_INT(id)) {
      case STYLESHEET_TYPE:
      {
        nsAutoString prop;
        if (NS_OK == a->GetType(prop)) {
          nsJSUtils::nsConvertStringToJSVal(prop, cx, vp);
        }
        else {
          return JS_FALSE;
        }
        break;
      }
      case STYLESHEET_DISABLED:
      {
        PRBool prop;
        if (NS_OK == a->GetDisabled(&prop)) {
          *vp = BOOLEAN_TO_JSVAL(prop);
        }
        else {
          return JS_FALSE;
        }
        break;
      }
      case STYLESHEET_READONLY:
      {
        PRBool prop;
        if (NS_OK == a->GetReadOnly(&prop)) {
          *vp = BOOLEAN_TO_JSVAL(prop);
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
// StyleSheet Properties Setter
//
PR_STATIC_CALLBACK(JSBool)
SetStyleSheetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  nsIDOMStyleSheet *a = (nsIDOMStyleSheet*)JS_GetPrivate(cx, obj);

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == a) {
    return JS_TRUE;
  }

  if (JSVAL_IS_INT(id)) {
    switch(JSVAL_TO_INT(id)) {
      case STYLESHEET_DISABLED:
      {
        PRBool prop;
        if (PR_FALSE == nsJSUtils::nsConvertJSValToBool(&prop, cx, *vp)) {
          return JS_FALSE;
        }
      
        a->SetDisabled(prop);
        
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
// StyleSheet finalizer
//
PR_STATIC_CALLBACK(void)
FinalizeStyleSheet(JSContext *cx, JSObject *obj)
{
  nsJSUtils::nsGenericFinalize(cx, obj);
}


//
// StyleSheet enumerate
//
PR_STATIC_CALLBACK(JSBool)
EnumerateStyleSheet(JSContext *cx, JSObject *obj)
{
  return nsJSUtils::nsGenericEnumerate(cx, obj);
}


//
// StyleSheet resolve
//
PR_STATIC_CALLBACK(JSBool)
ResolveStyleSheet(JSContext *cx, JSObject *obj, jsval id)
{
  return nsJSUtils::nsGenericResolve(cx, obj, id);
}


/***********************************************************************/
//
// class for StyleSheet
//
JSClass StyleSheetClass = {
  "StyleSheet", 
  JSCLASS_HAS_PRIVATE,
  JS_PropertyStub,
  JS_PropertyStub,
  GetStyleSheetProperty,
  SetStyleSheetProperty,
  EnumerateStyleSheet,
  ResolveStyleSheet,
  JS_ConvertStub,
  FinalizeStyleSheet
};


//
// StyleSheet class properties
//
static JSPropertySpec StyleSheetProperties[] =
{
  {"type",    STYLESHEET_TYPE,    JSPROP_ENUMERATE | JSPROP_READONLY},
  {"disabled",    STYLESHEET_DISABLED,    JSPROP_ENUMERATE},
  {"readOnly",    STYLESHEET_READONLY,    JSPROP_ENUMERATE | JSPROP_READONLY},
  {0}
};


//
// StyleSheet class methods
//
static JSFunctionSpec StyleSheetMethods[] = 
{
  {0}
};


//
// StyleSheet constructor
//
PR_STATIC_CALLBACK(JSBool)
StyleSheet(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  return JS_FALSE;
}


//
// StyleSheet class initialization
//
nsresult NS_InitStyleSheetClass(nsIScriptContext *aContext, void **aPrototype)
{
  JSContext *jscontext = (JSContext *)aContext->GetNativeContext();
  JSObject *proto = nsnull;
  JSObject *constructor = nsnull;
  JSObject *parent_proto = nsnull;
  JSObject *global = JS_GetGlobalObject(jscontext);
  jsval vp;

  if ((PR_TRUE != JS_LookupProperty(jscontext, global, "StyleSheet", &vp)) ||
      !JSVAL_IS_OBJECT(vp) ||
      ((constructor = JSVAL_TO_OBJECT(vp)) == nsnull) ||
      (PR_TRUE != JS_LookupProperty(jscontext, JSVAL_TO_OBJECT(vp), "prototype", &vp)) || 
      !JSVAL_IS_OBJECT(vp)) {

    proto = JS_InitClass(jscontext,     // context
                         global,        // global object
                         parent_proto,  // parent proto 
                         &StyleSheetClass,      // JSClass
                         StyleSheet,            // JSNative ctor
                         0,             // ctor args
                         StyleSheetProperties,  // proto props
                         StyleSheetMethods,     // proto funcs
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
// Method for creating a new StyleSheet JavaScript object
//
extern "C" NS_DOM nsresult NS_NewScriptStyleSheet(nsIScriptContext *aContext, nsISupports *aSupports, nsISupports *aParent, void **aReturn)
{
  NS_PRECONDITION(nsnull != aContext && nsnull != aSupports && nsnull != aReturn, "null argument to NS_NewScriptStyleSheet");
  JSObject *proto;
  JSObject *parent;
  nsIScriptObjectOwner *owner;
  JSContext *jscontext = (JSContext *)aContext->GetNativeContext();
  nsresult result = NS_OK;
  nsIDOMStyleSheet *aStyleSheet;

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

  if (NS_OK != NS_InitStyleSheetClass(aContext, (void **)&proto)) {
    return NS_ERROR_FAILURE;
  }

  result = aSupports->QueryInterface(kIStyleSheetIID, (void **)&aStyleSheet);
  if (NS_OK != result) {
    return result;
  }

  // create a js object for this class
  *aReturn = JS_NewObject(jscontext, &StyleSheetClass, proto, parent);
  if (nsnull != *aReturn) {
    // connect the native object to the js object
    JS_SetPrivate(jscontext, (JSObject *)*aReturn, aStyleSheet);
  }
  else {
    NS_RELEASE(aStyleSheet);
    return NS_ERROR_FAILURE; 
  }

  return NS_OK;
}
