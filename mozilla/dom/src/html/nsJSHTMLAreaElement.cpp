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
#include "nsIDOMHTMLAreaElement.h"


static NS_DEFINE_IID(kIScriptObjectOwnerIID, NS_ISCRIPTOBJECTOWNER_IID);
static NS_DEFINE_IID(kIJSScriptObjectIID, NS_IJSSCRIPTOBJECT_IID);
static NS_DEFINE_IID(kIScriptGlobalObjectIID, NS_ISCRIPTGLOBALOBJECT_IID);
static NS_DEFINE_IID(kIHTMLAreaElementIID, NS_IDOMHTMLAREAELEMENT_IID);

NS_DEF_PTR(nsIDOMHTMLAreaElement);

//
// HTMLAreaElement property ids
//
enum HTMLAreaElement_slots {
  HTMLAREAELEMENT_ACCESSKEY = -1,
  HTMLAREAELEMENT_ALT = -2,
  HTMLAREAELEMENT_COORDS = -3,
  HTMLAREAELEMENT_HREF = -4,
  HTMLAREAELEMENT_NOHREF = -5,
  HTMLAREAELEMENT_SHAPE = -6,
  HTMLAREAELEMENT_TABINDEX = -7,
  HTMLAREAELEMENT_TARGET = -8
};

/***********************************************************************/
//
// HTMLAreaElement Properties Getter
//
PR_STATIC_CALLBACK(JSBool)
GetHTMLAreaElementProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  nsIDOMHTMLAreaElement *a = (nsIDOMHTMLAreaElement*)JS_GetPrivate(cx, obj);

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == a) {
    return JS_TRUE;
  }

  if (JSVAL_IS_INT(id)) {
    switch(JSVAL_TO_INT(id)) {
      case HTMLAREAELEMENT_ACCESSKEY:
      {
        nsAutoString prop;
        if (NS_OK == a->GetAccessKey(prop)) {
          JSString *jsstring = JS_NewUCStringCopyN(cx, prop, prop.Length());
          // set the return value
          *vp = STRING_TO_JSVAL(jsstring);
        }
        else {
          return JS_FALSE;
        }
        break;
      }
      case HTMLAREAELEMENT_ALT:
      {
        nsAutoString prop;
        if (NS_OK == a->GetAlt(prop)) {
          JSString *jsstring = JS_NewUCStringCopyN(cx, prop, prop.Length());
          // set the return value
          *vp = STRING_TO_JSVAL(jsstring);
        }
        else {
          return JS_FALSE;
        }
        break;
      }
      case HTMLAREAELEMENT_COORDS:
      {
        nsAutoString prop;
        if (NS_OK == a->GetCoords(prop)) {
          JSString *jsstring = JS_NewUCStringCopyN(cx, prop, prop.Length());
          // set the return value
          *vp = STRING_TO_JSVAL(jsstring);
        }
        else {
          return JS_FALSE;
        }
        break;
      }
      case HTMLAREAELEMENT_HREF:
      {
        nsAutoString prop;
        if (NS_OK == a->GetHref(prop)) {
          JSString *jsstring = JS_NewUCStringCopyN(cx, prop, prop.Length());
          // set the return value
          *vp = STRING_TO_JSVAL(jsstring);
        }
        else {
          return JS_FALSE;
        }
        break;
      }
      case HTMLAREAELEMENT_NOHREF:
      {
        PRBool prop;
        if (NS_OK == a->GetNoHref(&prop)) {
          *vp = BOOLEAN_TO_JSVAL(prop);
        }
        else {
          return JS_FALSE;
        }
        break;
      }
      case HTMLAREAELEMENT_SHAPE:
      {
        nsAutoString prop;
        if (NS_OK == a->GetShape(prop)) {
          JSString *jsstring = JS_NewUCStringCopyN(cx, prop, prop.Length());
          // set the return value
          *vp = STRING_TO_JSVAL(jsstring);
        }
        else {
          return JS_FALSE;
        }
        break;
      }
      case HTMLAREAELEMENT_TABINDEX:
      {
        PRInt32 prop;
        if (NS_OK == a->GetTabIndex(&prop)) {
          *vp = INT_TO_JSVAL(prop);
        }
        else {
          return JS_FALSE;
        }
        break;
      }
      case HTMLAREAELEMENT_TARGET:
      {
        nsAutoString prop;
        if (NS_OK == a->GetTarget(prop)) {
          JSString *jsstring = JS_NewUCStringCopyN(cx, prop, prop.Length());
          // set the return value
          *vp = STRING_TO_JSVAL(jsstring);
        }
        else {
          return JS_FALSE;
        }
        break;
      }
      default:
      {
        nsIJSScriptObject *object;
        if (NS_OK == a->QueryInterface(kIJSScriptObjectIID, (void**)&object)) {
          PRBool rval;
          rval =  object->GetProperty(cx, id, vp);
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
      rval =  object->GetProperty(cx, id, vp);
      NS_RELEASE(object);
      return rval;
    }
  }

  return PR_TRUE;
}

/***********************************************************************/
//
// HTMLAreaElement Properties Setter
//
PR_STATIC_CALLBACK(JSBool)
SetHTMLAreaElementProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  nsIDOMHTMLAreaElement *a = (nsIDOMHTMLAreaElement*)JS_GetPrivate(cx, obj);

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == a) {
    return JS_TRUE;
  }

  if (JSVAL_IS_INT(id)) {
    switch(JSVAL_TO_INT(id)) {
      case HTMLAREAELEMENT_ACCESSKEY:
      {
        nsAutoString prop;
        JSString *jsstring;
        if ((jsstring = JS_ValueToString(cx, *vp)) != nsnull) {
          prop.SetString(JS_GetStringChars(jsstring));
        }
        else {
          prop.SetString((const char *)nsnull);
        }
      
        a->SetAccessKey(prop);
        
        break;
      }
      case HTMLAREAELEMENT_ALT:
      {
        nsAutoString prop;
        JSString *jsstring;
        if ((jsstring = JS_ValueToString(cx, *vp)) != nsnull) {
          prop.SetString(JS_GetStringChars(jsstring));
        }
        else {
          prop.SetString((const char *)nsnull);
        }
      
        a->SetAlt(prop);
        
        break;
      }
      case HTMLAREAELEMENT_COORDS:
      {
        nsAutoString prop;
        JSString *jsstring;
        if ((jsstring = JS_ValueToString(cx, *vp)) != nsnull) {
          prop.SetString(JS_GetStringChars(jsstring));
        }
        else {
          prop.SetString((const char *)nsnull);
        }
      
        a->SetCoords(prop);
        
        break;
      }
      case HTMLAREAELEMENT_HREF:
      {
        nsAutoString prop;
        JSString *jsstring;
        if ((jsstring = JS_ValueToString(cx, *vp)) != nsnull) {
          prop.SetString(JS_GetStringChars(jsstring));
        }
        else {
          prop.SetString((const char *)nsnull);
        }
      
        a->SetHref(prop);
        
        break;
      }
      case HTMLAREAELEMENT_NOHREF:
      {
        PRBool prop;
        JSBool temp;
        if (JSVAL_IS_BOOLEAN(*vp) && JS_ValueToBoolean(cx, *vp, &temp)) {
          prop = (PRBool)temp;
        }
        else {
          JS_ReportError(cx, "Parameter must be a boolean");
          return JS_FALSE;
        }
      
        a->SetNoHref(prop);
        
        break;
      }
      case HTMLAREAELEMENT_SHAPE:
      {
        nsAutoString prop;
        JSString *jsstring;
        if ((jsstring = JS_ValueToString(cx, *vp)) != nsnull) {
          prop.SetString(JS_GetStringChars(jsstring));
        }
        else {
          prop.SetString((const char *)nsnull);
        }
      
        a->SetShape(prop);
        
        break;
      }
      case HTMLAREAELEMENT_TABINDEX:
      {
        PRInt32 prop;
        int32 temp;
        if (JSVAL_IS_NUMBER(*vp) && JS_ValueToInt32(cx, *vp, &temp)) {
          prop = (PRInt32)temp;
        }
        else {
          JS_ReportError(cx, "Parameter must be a number");
          return JS_FALSE;
        }
      
        a->SetTabIndex(prop);
        
        break;
      }
      case HTMLAREAELEMENT_TARGET:
      {
        nsAutoString prop;
        JSString *jsstring;
        if ((jsstring = JS_ValueToString(cx, *vp)) != nsnull) {
          prop.SetString(JS_GetStringChars(jsstring));
        }
        else {
          prop.SetString((const char *)nsnull);
        }
      
        a->SetTarget(prop);
        
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
// HTMLAreaElement finalizer
//
PR_STATIC_CALLBACK(void)
FinalizeHTMLAreaElement(JSContext *cx, JSObject *obj)
{
  nsIDOMHTMLAreaElement *a = (nsIDOMHTMLAreaElement*)JS_GetPrivate(cx, obj);
  
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
// HTMLAreaElement enumerate
//
PR_STATIC_CALLBACK(JSBool)
EnumerateHTMLAreaElement(JSContext *cx, JSObject *obj)
{
  nsIDOMHTMLAreaElement *a = (nsIDOMHTMLAreaElement*)JS_GetPrivate(cx, obj);
  
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
// HTMLAreaElement resolve
//
PR_STATIC_CALLBACK(JSBool)
ResolveHTMLAreaElement(JSContext *cx, JSObject *obj, jsval id)
{
  nsIDOMHTMLAreaElement *a = (nsIDOMHTMLAreaElement*)JS_GetPrivate(cx, obj);
  
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


/***********************************************************************/
//
// class for HTMLAreaElement
//
JSClass HTMLAreaElementClass = {
  "HTMLAreaElement", 
  JSCLASS_HAS_PRIVATE,
  JS_PropertyStub,
  JS_PropertyStub,
  GetHTMLAreaElementProperty,
  SetHTMLAreaElementProperty,
  EnumerateHTMLAreaElement,
  ResolveHTMLAreaElement,
  JS_ConvertStub,
  FinalizeHTMLAreaElement
};


//
// HTMLAreaElement class properties
//
static JSPropertySpec HTMLAreaElementProperties[] =
{
  {"accessKey",    HTMLAREAELEMENT_ACCESSKEY,    JSPROP_ENUMERATE},
  {"alt",    HTMLAREAELEMENT_ALT,    JSPROP_ENUMERATE},
  {"coords",    HTMLAREAELEMENT_COORDS,    JSPROP_ENUMERATE},
  {"href",    HTMLAREAELEMENT_HREF,    JSPROP_ENUMERATE},
  {"noHref",    HTMLAREAELEMENT_NOHREF,    JSPROP_ENUMERATE},
  {"shape",    HTMLAREAELEMENT_SHAPE,    JSPROP_ENUMERATE},
  {"tabIndex",    HTMLAREAELEMENT_TABINDEX,    JSPROP_ENUMERATE},
  {"target",    HTMLAREAELEMENT_TARGET,    JSPROP_ENUMERATE},
  {0}
};


//
// HTMLAreaElement class methods
//
static JSFunctionSpec HTMLAreaElementMethods[] = 
{
  {0}
};


//
// HTMLAreaElement constructor
//
PR_STATIC_CALLBACK(JSBool)
HTMLAreaElement(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  return JS_FALSE;
}


//
// HTMLAreaElement class initialization
//
nsresult NS_InitHTMLAreaElementClass(nsIScriptContext *aContext, void **aPrototype)
{
  JSContext *jscontext = (JSContext *)aContext->GetNativeContext();
  JSObject *proto = nsnull;
  JSObject *constructor = nsnull;
  JSObject *parent_proto = nsnull;
  JSObject *global = JS_GetGlobalObject(jscontext);
  jsval vp;

  if ((PR_TRUE != JS_LookupProperty(jscontext, global, "HTMLAreaElement", &vp)) ||
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
                         &HTMLAreaElementClass,      // JSClass
                         HTMLAreaElement,            // JSNative ctor
                         0,             // ctor args
                         HTMLAreaElementProperties,  // proto props
                         HTMLAreaElementMethods,     // proto funcs
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
// Method for creating a new HTMLAreaElement JavaScript object
//
extern "C" NS_DOM nsresult NS_NewScriptHTMLAreaElement(nsIScriptContext *aContext, nsISupports *aSupports, nsISupports *aParent, void **aReturn)
{
  NS_PRECONDITION(nsnull != aContext && nsnull != aSupports && nsnull != aReturn, "null argument to NS_NewScriptHTMLAreaElement");
  JSObject *proto;
  JSObject *parent;
  nsIScriptObjectOwner *owner;
  JSContext *jscontext = (JSContext *)aContext->GetNativeContext();
  nsresult result = NS_OK;
  nsIDOMHTMLAreaElement *aHTMLAreaElement;

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

  if (NS_OK != NS_InitHTMLAreaElementClass(aContext, (void **)&proto)) {
    return NS_ERROR_FAILURE;
  }

  result = aSupports->QueryInterface(kIHTMLAreaElementIID, (void **)&aHTMLAreaElement);
  if (NS_OK != result) {
    return result;
  }

  // create a js object for this class
  *aReturn = JS_NewObject(jscontext, &HTMLAreaElementClass, proto, parent);
  if (nsnull != *aReturn) {
    // connect the native object to the js object
    JS_SetPrivate(jscontext, (JSObject *)*aReturn, aHTMLAreaElement);
  }
  else {
    NS_RELEASE(aHTMLAreaElement);
    return NS_ERROR_FAILURE; 
  }

  return NS_OK;
}
