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
#include "nsIDOMHTMLFormElement.h"
#include "nsIDOMHTMLCollection.h"


static NS_DEFINE_IID(kIScriptObjectOwnerIID, NS_ISCRIPTOBJECTOWNER_IID);
static NS_DEFINE_IID(kIJSScriptObjectIID, NS_IJSSCRIPTOBJECT_IID);
static NS_DEFINE_IID(kIScriptGlobalObjectIID, NS_ISCRIPTGLOBALOBJECT_IID);
static NS_DEFINE_IID(kIHTMLFormElementIID, NS_IDOMHTMLFORMELEMENT_IID);
static NS_DEFINE_IID(kIHTMLCollectionIID, NS_IDOMHTMLCOLLECTION_IID);

NS_DEF_PTR(nsIDOMHTMLFormElement);
NS_DEF_PTR(nsIDOMHTMLCollection);

//
// HTMLFormElement property ids
//
enum HTMLFormElement_slots {
  HTMLFORMELEMENT_ELEMENTS = -11,
  HTMLFORMELEMENT_NAME = -12,
  HTMLFORMELEMENT_ACCEPTCHARSET = -13,
  HTMLFORMELEMENT_ACTION = -14,
  HTMLFORMELEMENT_ENCTYPE = -15,
  HTMLFORMELEMENT_METHOD = -16,
  HTMLFORMELEMENT_TARGET = -17
};

/***********************************************************************/
//
// HTMLFormElement Properties Getter
//
PR_STATIC_CALLBACK(JSBool)
GetHTMLFormElementProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  nsIDOMHTMLFormElement *a = (nsIDOMHTMLFormElement*)JS_GetPrivate(cx, obj);

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == a) {
    return JS_TRUE;
  }

  if (JSVAL_IS_INT(id)) {
    switch(JSVAL_TO_INT(id)) {
      case HTMLFORMELEMENT_ELEMENTS:
      {
        nsIDOMHTMLCollection* prop;
        if (NS_OK == a->GetElements(&prop)) {
          // get the js object
          if (prop != nsnull) {
            nsIScriptObjectOwner *owner = nsnull;
            if (NS_OK == prop->QueryInterface(kIScriptObjectOwnerIID, (void**)&owner)) {
              JSObject *object = nsnull;
              nsIScriptContext *script_cx = (nsIScriptContext *)JS_GetContextPrivate(cx);
              if (NS_OK == owner->GetScriptObject(script_cx, (void**)&object)) {
                // set the return value
                *vp = OBJECT_TO_JSVAL(object);
              }
              NS_RELEASE(owner);
            }
            NS_RELEASE(prop);
          }
          else {
            *vp = JSVAL_NULL;
          }
        }
        else {
          return JS_FALSE;
        }
        break;
      }
      case HTMLFORMELEMENT_NAME:
      {
        nsAutoString prop;
        if (NS_OK == a->GetName(prop)) {
          JSString *jsstring = JS_NewUCStringCopyN(cx, prop, prop.Length());
          // set the return value
          *vp = STRING_TO_JSVAL(jsstring);
        }
        else {
          return JS_FALSE;
        }
        break;
      }
      case HTMLFORMELEMENT_ACCEPTCHARSET:
      {
        nsAutoString prop;
        if (NS_OK == a->GetAcceptCharset(prop)) {
          JSString *jsstring = JS_NewUCStringCopyN(cx, prop, prop.Length());
          // set the return value
          *vp = STRING_TO_JSVAL(jsstring);
        }
        else {
          return JS_FALSE;
        }
        break;
      }
      case HTMLFORMELEMENT_ACTION:
      {
        nsAutoString prop;
        if (NS_OK == a->GetAction(prop)) {
          JSString *jsstring = JS_NewUCStringCopyN(cx, prop, prop.Length());
          // set the return value
          *vp = STRING_TO_JSVAL(jsstring);
        }
        else {
          return JS_FALSE;
        }
        break;
      }
      case HTMLFORMELEMENT_ENCTYPE:
      {
        nsAutoString prop;
        if (NS_OK == a->GetEnctype(prop)) {
          JSString *jsstring = JS_NewUCStringCopyN(cx, prop, prop.Length());
          // set the return value
          *vp = STRING_TO_JSVAL(jsstring);
        }
        else {
          return JS_FALSE;
        }
        break;
      }
      case HTMLFORMELEMENT_METHOD:
      {
        nsAutoString prop;
        if (NS_OK == a->GetMethod(prop)) {
          JSString *jsstring = JS_NewUCStringCopyN(cx, prop, prop.Length());
          // set the return value
          *vp = STRING_TO_JSVAL(jsstring);
        }
        else {
          return JS_FALSE;
        }
        break;
      }
      case HTMLFORMELEMENT_TARGET:
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
// HTMLFormElement Properties Setter
//
PR_STATIC_CALLBACK(JSBool)
SetHTMLFormElementProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  nsIDOMHTMLFormElement *a = (nsIDOMHTMLFormElement*)JS_GetPrivate(cx, obj);

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == a) {
    return JS_TRUE;
  }

  if (JSVAL_IS_INT(id)) {
    switch(JSVAL_TO_INT(id)) {
      case HTMLFORMELEMENT_ELEMENTS:
      {
        nsIDOMHTMLCollection* prop;
        if (JSVAL_IS_NULL(*vp)) {
          prop = nsnull;
        }
        else if (JSVAL_IS_OBJECT(*vp)) {
          JSObject *jsobj = JSVAL_TO_OBJECT(*vp); 
          nsISupports *supports = (nsISupports *)JS_GetPrivate(cx, jsobj);
          if (NS_OK != supports->QueryInterface(kIHTMLCollectionIID, (void **)&prop)) {
            JS_ReportError(cx, "Parameter must be of type HTMLCollection");
            return JS_FALSE;
          }
        }
        else {
          JS_ReportError(cx, "Parameter must be an object");
          return JS_FALSE;
        }
      
        a->SetElements(prop);
        if (prop) NS_RELEASE(prop);
        break;
      }
      case HTMLFORMELEMENT_NAME:
      {
        nsAutoString prop;
        JSString *jsstring;
        if ((jsstring = JS_ValueToString(cx, *vp)) != nsnull) {
          prop.SetString(JS_GetStringChars(jsstring));
        }
        else {
          prop.SetString((const char *)nsnull);
        }
      
        a->SetName(prop);
        
        break;
      }
      case HTMLFORMELEMENT_ACCEPTCHARSET:
      {
        nsAutoString prop;
        JSString *jsstring;
        if ((jsstring = JS_ValueToString(cx, *vp)) != nsnull) {
          prop.SetString(JS_GetStringChars(jsstring));
        }
        else {
          prop.SetString((const char *)nsnull);
        }
      
        a->SetAcceptCharset(prop);
        
        break;
      }
      case HTMLFORMELEMENT_ACTION:
      {
        nsAutoString prop;
        JSString *jsstring;
        if ((jsstring = JS_ValueToString(cx, *vp)) != nsnull) {
          prop.SetString(JS_GetStringChars(jsstring));
        }
        else {
          prop.SetString((const char *)nsnull);
        }
      
        a->SetAction(prop);
        
        break;
      }
      case HTMLFORMELEMENT_ENCTYPE:
      {
        nsAutoString prop;
        JSString *jsstring;
        if ((jsstring = JS_ValueToString(cx, *vp)) != nsnull) {
          prop.SetString(JS_GetStringChars(jsstring));
        }
        else {
          prop.SetString((const char *)nsnull);
        }
      
        a->SetEnctype(prop);
        
        break;
      }
      case HTMLFORMELEMENT_METHOD:
      {
        nsAutoString prop;
        JSString *jsstring;
        if ((jsstring = JS_ValueToString(cx, *vp)) != nsnull) {
          prop.SetString(JS_GetStringChars(jsstring));
        }
        else {
          prop.SetString((const char *)nsnull);
        }
      
        a->SetMethod(prop);
        
        break;
      }
      case HTMLFORMELEMENT_TARGET:
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
// HTMLFormElement finalizer
//
PR_STATIC_CALLBACK(void)
FinalizeHTMLFormElement(JSContext *cx, JSObject *obj)
{
  nsIDOMHTMLFormElement *a = (nsIDOMHTMLFormElement*)JS_GetPrivate(cx, obj);
  
  if (nsnull != a) {
    // get the js object
    nsIScriptObjectOwner *owner = nsnull;
    if (NS_OK == a->QueryInterface(kIScriptObjectOwnerIID, (void**)&owner)) {
      owner->ResetScriptObject();
      NS_RELEASE(owner);
    }

    NS_RELEASE(a);
  }
}


//
// HTMLFormElement enumerate
//
PR_STATIC_CALLBACK(JSBool)
EnumerateHTMLFormElement(JSContext *cx, JSObject *obj)
{
  nsIDOMHTMLFormElement *a = (nsIDOMHTMLFormElement*)JS_GetPrivate(cx, obj);
  
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
// HTMLFormElement resolve
//
PR_STATIC_CALLBACK(JSBool)
ResolveHTMLFormElement(JSContext *cx, JSObject *obj, jsval id)
{
  nsIDOMHTMLFormElement *a = (nsIDOMHTMLFormElement*)JS_GetPrivate(cx, obj);
  
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
// class for HTMLFormElement
//
JSClass HTMLFormElementClass = {
  "HTMLFormElement", 
  JSCLASS_HAS_PRIVATE,
  JS_PropertyStub,
  JS_PropertyStub,
  GetHTMLFormElementProperty,
  SetHTMLFormElementProperty,
  EnumerateHTMLFormElement,
  ResolveHTMLFormElement,
  JS_ConvertStub,
  FinalizeHTMLFormElement
};


//
// HTMLFormElement class properties
//
static JSPropertySpec HTMLFormElementProperties[] =
{
  {"elements",    HTMLFORMELEMENT_ELEMENTS,    JSPROP_ENUMERATE},
  {"name",    HTMLFORMELEMENT_NAME,    JSPROP_ENUMERATE},
  {"acceptCharset",    HTMLFORMELEMENT_ACCEPTCHARSET,    JSPROP_ENUMERATE},
  {"action",    HTMLFORMELEMENT_ACTION,    JSPROP_ENUMERATE},
  {"enctype",    HTMLFORMELEMENT_ENCTYPE,    JSPROP_ENUMERATE},
  {"method",    HTMLFORMELEMENT_METHOD,    JSPROP_ENUMERATE},
  {"target",    HTMLFORMELEMENT_TARGET,    JSPROP_ENUMERATE},
  {0}
};


//
// HTMLFormElement class methods
//
static JSFunctionSpec HTMLFormElementMethods[] = 
{
  {0}
};


//
// HTMLFormElement constructor
//
PR_STATIC_CALLBACK(JSBool)
HTMLFormElement(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  return JS_TRUE;
}


//
// HTMLFormElement class initialization
//
nsresult NS_InitHTMLFormElementClass(nsIScriptContext *aContext, void **aPrototype)
{
  JSContext *jscontext = (JSContext *)aContext->GetNativeContext();
  JSObject *proto = nsnull;
  JSObject *constructor = nsnull;
  JSObject *parent_proto = nsnull;
  JSObject *global = JS_GetGlobalObject(jscontext);
  jsval vp;

  if ((PR_TRUE != JS_LookupProperty(jscontext, global, "HTMLFormElement", &vp)) ||
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
                         &HTMLFormElementClass,      // JSClass
                         HTMLFormElement,            // JSNative ctor
                         0,             // ctor args
                         HTMLFormElementProperties,  // proto props
                         HTMLFormElementMethods,     // proto funcs
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
// Method for creating a new HTMLFormElement JavaScript object
//
extern "C" NS_DOM nsresult NS_NewScriptHTMLFormElement(nsIScriptContext *aContext, nsIDOMHTMLFormElement *aSupports, nsISupports *aParent, void **aReturn)
{
  NS_PRECONDITION(nsnull != aContext && nsnull != aSupports && nsnull != aReturn, "null argument to NS_NewScriptHTMLFormElement");
  JSObject *proto;
  JSObject *parent;
  nsIScriptObjectOwner *owner;
  JSContext *jscontext = (JSContext *)aContext->GetNativeContext();

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

  if (NS_OK != NS_InitHTMLFormElementClass(aContext, (void **)&proto)) {
    return NS_ERROR_FAILURE;
  }

  // create a js object for this class
  *aReturn = JS_NewObject(jscontext, &HTMLFormElementClass, proto, parent);
  if (nsnull != *aReturn) {
    // connect the native object to the js object
    JS_SetPrivate(jscontext, (JSObject *)*aReturn, aSupports);
    NS_ADDREF(aSupports);
  }
  else {
    return NS_ERROR_FAILURE; 
  }

  return NS_OK;
}
