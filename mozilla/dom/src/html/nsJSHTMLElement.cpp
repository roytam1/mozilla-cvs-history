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
#include "nsIDOMHTMLElement.h"
#include "nsIDOMCSSStyleDeclaration.h"


static NS_DEFINE_IID(kIScriptObjectOwnerIID, NS_ISCRIPTOBJECTOWNER_IID);
static NS_DEFINE_IID(kIJSScriptObjectIID, NS_IJSSCRIPTOBJECT_IID);
static NS_DEFINE_IID(kIScriptGlobalObjectIID, NS_ISCRIPTGLOBALOBJECT_IID);
static NS_DEFINE_IID(kIHTMLElementIID, NS_IDOMHTMLELEMENT_IID);
static NS_DEFINE_IID(kICSSStyleDeclarationIID, NS_IDOMCSSSTYLEDECLARATION_IID);

NS_DEF_PTR(nsIDOMHTMLElement);
NS_DEF_PTR(nsIDOMCSSStyleDeclaration);

//
// HTMLElement property ids
//
enum HTMLElement_slots {
  HTMLELEMENT_ID = -1,
  HTMLELEMENT_TITLE = -2,
  HTMLELEMENT_LANG = -3,
  HTMLELEMENT_DIR = -4,
  HTMLELEMENT_CLASSNAME = -5,
  HTMLELEMENT_STYLE = -6
};

/***********************************************************************/
//
// HTMLElement Properties Getter
//
PR_STATIC_CALLBACK(JSBool)
GetHTMLElementProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  nsIDOMHTMLElement *a = (nsIDOMHTMLElement*)JS_GetPrivate(cx, obj);

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == a) {
    return JS_TRUE;
  }

  if (JSVAL_IS_INT(id)) {
    switch(JSVAL_TO_INT(id)) {
      case HTMLELEMENT_ID:
      {
        nsAutoString prop;
        if (NS_OK == a->GetId(prop)) {
          nsJSUtils::nsConvertStringToJSVal(prop, cx, vp);
        }
        else {
          return JS_FALSE;
        }
        break;
      }
      case HTMLELEMENT_TITLE:
      {
        nsAutoString prop;
        if (NS_OK == a->GetTitle(prop)) {
          nsJSUtils::nsConvertStringToJSVal(prop, cx, vp);
        }
        else {
          return JS_FALSE;
        }
        break;
      }
      case HTMLELEMENT_LANG:
      {
        nsAutoString prop;
        if (NS_OK == a->GetLang(prop)) {
          nsJSUtils::nsConvertStringToJSVal(prop, cx, vp);
        }
        else {
          return JS_FALSE;
        }
        break;
      }
      case HTMLELEMENT_DIR:
      {
        nsAutoString prop;
        if (NS_OK == a->GetDir(prop)) {
          nsJSUtils::nsConvertStringToJSVal(prop, cx, vp);
        }
        else {
          return JS_FALSE;
        }
        break;
      }
      case HTMLELEMENT_CLASSNAME:
      {
        nsAutoString prop;
        if (NS_OK == a->GetClassName(prop)) {
          nsJSUtils::nsConvertStringToJSVal(prop, cx, vp);
        }
        else {
          return JS_FALSE;
        }
        break;
      }
      case HTMLELEMENT_STYLE:
      {
        nsIDOMCSSStyleDeclaration* prop;
        if (NS_OK == a->GetStyle(&prop)) {
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
  }
  else {
    return nsJSUtils::nsCallJSScriptObjectGetProperty(a, cx, id, vp);
  }

  return PR_TRUE;
}

/***********************************************************************/
//
// HTMLElement Properties Setter
//
PR_STATIC_CALLBACK(JSBool)
SetHTMLElementProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  nsIDOMHTMLElement *a = (nsIDOMHTMLElement*)JS_GetPrivate(cx, obj);

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == a) {
    return JS_TRUE;
  }

  if (JSVAL_IS_INT(id)) {
    switch(JSVAL_TO_INT(id)) {
      case HTMLELEMENT_ID:
      {
        nsAutoString prop;
        nsJSUtils::nsConvertJSValToString(prop, cx, *vp);
      
        a->SetId(prop);
        
        break;
      }
      case HTMLELEMENT_TITLE:
      {
        nsAutoString prop;
        nsJSUtils::nsConvertJSValToString(prop, cx, *vp);
      
        a->SetTitle(prop);
        
        break;
      }
      case HTMLELEMENT_LANG:
      {
        nsAutoString prop;
        nsJSUtils::nsConvertJSValToString(prop, cx, *vp);
      
        a->SetLang(prop);
        
        break;
      }
      case HTMLELEMENT_DIR:
      {
        nsAutoString prop;
        nsJSUtils::nsConvertJSValToString(prop, cx, *vp);
      
        a->SetDir(prop);
        
        break;
      }
      case HTMLELEMENT_CLASSNAME:
      {
        nsAutoString prop;
        nsJSUtils::nsConvertJSValToString(prop, cx, *vp);
      
        a->SetClassName(prop);
        
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
// HTMLElement finalizer
//
PR_STATIC_CALLBACK(void)
FinalizeHTMLElement(JSContext *cx, JSObject *obj)
{
  nsJSUtils::nsGenericFinalize(cx, obj);
}


//
// HTMLElement enumerate
//
PR_STATIC_CALLBACK(JSBool)
EnumerateHTMLElement(JSContext *cx, JSObject *obj)
{
  return nsJSUtils::nsGenericEnumerate(cx, obj);
}


//
// HTMLElement resolve
//
PR_STATIC_CALLBACK(JSBool)
ResolveHTMLElement(JSContext *cx, JSObject *obj, jsval id)
{
  return nsJSUtils::nsGenericResolve(cx, obj, id);
}


/***********************************************************************/
//
// class for HTMLElement
//
JSClass HTMLElementClass = {
  "HTMLElement", 
  JSCLASS_HAS_PRIVATE | JSCLASS_PRIVATE_IS_NSISUPPORTS,
  JS_PropertyStub,
  JS_PropertyStub,
  GetHTMLElementProperty,
  SetHTMLElementProperty,
  EnumerateHTMLElement,
  ResolveHTMLElement,
  JS_ConvertStub,
  FinalizeHTMLElement
};


//
// HTMLElement class properties
//
static JSPropertySpec HTMLElementProperties[] =
{
  {"id",    HTMLELEMENT_ID,    JSPROP_ENUMERATE},
  {"title",    HTMLELEMENT_TITLE,    JSPROP_ENUMERATE},
  {"lang",    HTMLELEMENT_LANG,    JSPROP_ENUMERATE},
  {"dir",    HTMLELEMENT_DIR,    JSPROP_ENUMERATE},
  {"className",    HTMLELEMENT_CLASSNAME,    JSPROP_ENUMERATE},
  {"style",    HTMLELEMENT_STYLE,    JSPROP_ENUMERATE | JSPROP_READONLY},
  {0}
};


//
// HTMLElement class methods
//
static JSFunctionSpec HTMLElementMethods[] = 
{
  {0}
};


//
// HTMLElement constructor
//
PR_STATIC_CALLBACK(JSBool)
HTMLElement(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  return JS_FALSE;
}


//
// HTMLElement class initialization
//
extern "C" NS_DOM nsresult NS_InitHTMLElementClass(nsIScriptContext *aContext, void **aPrototype)
{
  JSContext *jscontext = (JSContext *)aContext->GetNativeContext();
  JSObject *proto = nsnull;
  JSObject *constructor = nsnull;
  JSObject *parent_proto = nsnull;
  JSObject *global = JS_GetGlobalObject(jscontext);
  jsval vp;

  if ((PR_TRUE != JS_LookupProperty(jscontext, global, "HTMLElement", &vp)) ||
      !JSVAL_IS_OBJECT(vp) ||
      ((constructor = JSVAL_TO_OBJECT(vp)) == nsnull) ||
      (PR_TRUE != JS_LookupProperty(jscontext, JSVAL_TO_OBJECT(vp), "prototype", &vp)) || 
      !JSVAL_IS_OBJECT(vp)) {

    if (NS_OK != NS_InitElementClass(aContext, (void **)&parent_proto)) {
      return NS_ERROR_FAILURE;
    }
    proto = JS_InitClass(jscontext,     // context
                         global,        // global object
                         parent_proto,  // parent proto 
                         &HTMLElementClass,      // JSClass
                         HTMLElement,            // JSNative ctor
                         0,             // ctor args
                         HTMLElementProperties,  // proto props
                         HTMLElementMethods,     // proto funcs
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
// Method for creating a new HTMLElement JavaScript object
//
extern "C" NS_DOM nsresult NS_NewScriptHTMLElement(nsIScriptContext *aContext, nsISupports *aSupports, nsISupports *aParent, void **aReturn)
{
  NS_PRECONDITION(nsnull != aContext && nsnull != aSupports && nsnull != aReturn, "null argument to NS_NewScriptHTMLElement");
  JSObject *proto;
  JSObject *parent;
  nsIScriptObjectOwner *owner;
  JSContext *jscontext = (JSContext *)aContext->GetNativeContext();
  nsresult result = NS_OK;
  nsIDOMHTMLElement *aHTMLElement;

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

  if (NS_OK != NS_InitHTMLElementClass(aContext, (void **)&proto)) {
    return NS_ERROR_FAILURE;
  }

  result = aSupports->QueryInterface(kIHTMLElementIID, (void **)&aHTMLElement);
  if (NS_OK != result) {
    return result;
  }

  // create a js object for this class
  *aReturn = JS_NewObject(jscontext, &HTMLElementClass, proto, parent);
  if (nsnull != *aReturn) {
    // connect the native object to the js object
    JS_SetPrivate(jscontext, (JSObject *)*aReturn, aHTMLElement);
  }
  else {
    NS_RELEASE(aHTMLElement);
    return NS_ERROR_FAILURE; 
  }

  return NS_OK;
}
