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
#include "nsIDOMHTMLFormElement.h"
#include "nsIDOMHTMLIsIndexElement.h"


static NS_DEFINE_IID(kIScriptObjectOwnerIID, NS_ISCRIPTOBJECTOWNER_IID);
static NS_DEFINE_IID(kIJSScriptObjectIID, NS_IJSSCRIPTOBJECT_IID);
static NS_DEFINE_IID(kIScriptGlobalObjectIID, NS_ISCRIPTGLOBALOBJECT_IID);
static NS_DEFINE_IID(kIHTMLFormElementIID, NS_IDOMHTMLFORMELEMENT_IID);
static NS_DEFINE_IID(kIHTMLIsIndexElementIID, NS_IDOMHTMLISINDEXELEMENT_IID);

NS_DEF_PTR(nsIDOMHTMLFormElement);
NS_DEF_PTR(nsIDOMHTMLIsIndexElement);

//
// HTMLIsIndexElement property ids
//
enum HTMLIsIndexElement_slots {
  HTMLISINDEXELEMENT_FORM = -1,
  HTMLISINDEXELEMENT_PROMPT = -2
};

/***********************************************************************/
//
// HTMLIsIndexElement Properties Getter
//
PR_STATIC_CALLBACK(JSBool)
GetHTMLIsIndexElementProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  nsIDOMHTMLIsIndexElement *a = (nsIDOMHTMLIsIndexElement*)JS_GetPrivate(cx, obj);

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == a) {
    return JS_TRUE;
  }

  if (JSVAL_IS_INT(id)) {
    switch(JSVAL_TO_INT(id)) {
      case HTMLISINDEXELEMENT_FORM:
      {
        nsIDOMHTMLFormElement* prop;
        if (NS_OK == a->GetForm(&prop)) {
          // get the js object
          nsConvertObjectToJSVal((nsISupports *)prop, cx, vp);
        }
        else {
          return JS_FALSE;
        }
        break;
      }
      case HTMLISINDEXELEMENT_PROMPT:
      {
        nsAutoString prop;
        if (NS_OK == a->GetPrompt(prop)) {
          nsConvertStringToJSVal(prop, cx, vp);
        }
        else {
          return JS_FALSE;
        }
        break;
      }
      default:
        return nsCallJSScriptObjectGetProperty(a, cx, id, vp);
    }
  }
  else {
    return nsCallJSScriptObjectGetProperty(a, cx, id, vp);
  }

  return PR_TRUE;
}

/***********************************************************************/
//
// HTMLIsIndexElement Properties Setter
//
PR_STATIC_CALLBACK(JSBool)
SetHTMLIsIndexElementProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  nsIDOMHTMLIsIndexElement *a = (nsIDOMHTMLIsIndexElement*)JS_GetPrivate(cx, obj);

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == a) {
    return JS_TRUE;
  }

  if (JSVAL_IS_INT(id)) {
    switch(JSVAL_TO_INT(id)) {
      case HTMLISINDEXELEMENT_PROMPT:
      {
        nsAutoString prop;
        nsConvertJSValToString(prop, cx, *vp);
      
        a->SetPrompt(prop);
        
        break;
      }
      default:
        return nsCallJSScriptObjectSetProperty(a, cx, id, vp);
    }
  }
  else {
    return nsCallJSScriptObjectSetProperty(a, cx, id, vp);
  }

  return PR_TRUE;
}


//
// HTMLIsIndexElement finalizer
//
PR_STATIC_CALLBACK(void)
FinalizeHTMLIsIndexElement(JSContext *cx, JSObject *obj)
{
  nsGenericFinalize(cx, obj);
}


//
// HTMLIsIndexElement enumerate
//
PR_STATIC_CALLBACK(JSBool)
EnumerateHTMLIsIndexElement(JSContext *cx, JSObject *obj)
{
  return nsGenericEnumerate(cx, obj);
}


//
// HTMLIsIndexElement resolve
//
PR_STATIC_CALLBACK(JSBool)
ResolveHTMLIsIndexElement(JSContext *cx, JSObject *obj, jsval id)
{
  return nsGenericResolve(cx, obj, id);
}


/***********************************************************************/
//
// class for HTMLIsIndexElement
//
JSClass HTMLIsIndexElementClass = {
  "HTMLIsIndexElement", 
  JSCLASS_HAS_PRIVATE,
  JS_PropertyStub,
  JS_PropertyStub,
  GetHTMLIsIndexElementProperty,
  SetHTMLIsIndexElementProperty,
  EnumerateHTMLIsIndexElement,
  ResolveHTMLIsIndexElement,
  JS_ConvertStub,
  FinalizeHTMLIsIndexElement
};


//
// HTMLIsIndexElement class properties
//
static JSPropertySpec HTMLIsIndexElementProperties[] =
{
  {"form",    HTMLISINDEXELEMENT_FORM,    JSPROP_ENUMERATE | JSPROP_READONLY},
  {"prompt",    HTMLISINDEXELEMENT_PROMPT,    JSPROP_ENUMERATE},
  {0}
};


//
// HTMLIsIndexElement class methods
//
static JSFunctionSpec HTMLIsIndexElementMethods[] = 
{
  {0}
};


//
// HTMLIsIndexElement constructor
//
PR_STATIC_CALLBACK(JSBool)
HTMLIsIndexElement(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  return JS_FALSE;
}


//
// HTMLIsIndexElement class initialization
//
nsresult NS_InitHTMLIsIndexElementClass(nsIScriptContext *aContext, void **aPrototype)
{
  JSContext *jscontext = (JSContext *)aContext->GetNativeContext();
  JSObject *proto = nsnull;
  JSObject *constructor = nsnull;
  JSObject *parent_proto = nsnull;
  JSObject *global = JS_GetGlobalObject(jscontext);
  jsval vp;

  if ((PR_TRUE != JS_LookupProperty(jscontext, global, "HTMLIsIndexElement", &vp)) ||
      !JSVAL_IS_OBJECT(vp) ||
      ((constructor = JSVAL_TO_OBJECT(vp)) == nsnull) ||
      (PR_TRUE != JS_LookupProperty(jscontext, JSVAL_TO_OBJECT(vp), "prototype", &vp)) || 
      !JSVAL_IS_OBJECT(vp)) {

    if (NS_OK != NS_InitHTMLElementClass(aContext, (void **)&parent_proto)) {
      return NS_ERROR_FAILURE;
    }
    proto = JS_InitClass(jscontext,     // context
                         global,        // global object
                         parent_proto,  // parent proto 
                         &HTMLIsIndexElementClass,      // JSClass
                         HTMLIsIndexElement,            // JSNative ctor
                         0,             // ctor args
                         HTMLIsIndexElementProperties,  // proto props
                         HTMLIsIndexElementMethods,     // proto funcs
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
// Method for creating a new HTMLIsIndexElement JavaScript object
//
extern "C" NS_DOM nsresult NS_NewScriptHTMLIsIndexElement(nsIScriptContext *aContext, nsISupports *aSupports, nsISupports *aParent, void **aReturn)
{
  NS_PRECONDITION(nsnull != aContext && nsnull != aSupports && nsnull != aReturn, "null argument to NS_NewScriptHTMLIsIndexElement");
  JSObject *proto;
  JSObject *parent;
  nsIScriptObjectOwner *owner;
  JSContext *jscontext = (JSContext *)aContext->GetNativeContext();
  nsresult result = NS_OK;
  nsIDOMHTMLIsIndexElement *aHTMLIsIndexElement;

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

  if (NS_OK != NS_InitHTMLIsIndexElementClass(aContext, (void **)&proto)) {
    return NS_ERROR_FAILURE;
  }

  result = aSupports->QueryInterface(kIHTMLIsIndexElementIID, (void **)&aHTMLIsIndexElement);
  if (NS_OK != result) {
    return result;
  }

  // create a js object for this class
  *aReturn = JS_NewObject(jscontext, &HTMLIsIndexElementClass, proto, parent);
  if (nsnull != *aReturn) {
    // connect the native object to the js object
    JS_SetPrivate(jscontext, (JSObject *)*aReturn, aHTMLIsIndexElement);
  }
  else {
    NS_RELEASE(aHTMLIsIndexElement);
    return NS_ERROR_FAILURE; 
  }

  return NS_OK;
}
