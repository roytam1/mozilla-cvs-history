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
#include "nsIDOMHTMLInputElement.h"
#include "nsIDOMHTMLFormElement.h"


static NS_DEFINE_IID(kIScriptObjectOwnerIID, NS_ISCRIPTOBJECTOWNER_IID);
static NS_DEFINE_IID(kIJSScriptObjectIID, NS_IJSSCRIPTOBJECT_IID);
static NS_DEFINE_IID(kIScriptGlobalObjectIID, NS_ISCRIPTGLOBALOBJECT_IID);
static NS_DEFINE_IID(kIHTMLInputElementIID, NS_IDOMHTMLINPUTELEMENT_IID);
static NS_DEFINE_IID(kIHTMLFormElementIID, NS_IDOMHTMLFORMELEMENT_IID);

NS_DEF_PTR(nsIDOMHTMLInputElement);
NS_DEF_PTR(nsIDOMHTMLFormElement);

//
// HTMLInputElement property ids
//
enum HTMLInputElement_slots {
  HTMLINPUTELEMENT_DEFAULTVALUE = -11,
  HTMLINPUTELEMENT_DEFAULTCHECKED = -12,
  HTMLINPUTELEMENT_FORM = -13,
  HTMLINPUTELEMENT_ACCEPT = -14,
  HTMLINPUTELEMENT_ACCESSKEY = -15,
  HTMLINPUTELEMENT_ALIGN = -16,
  HTMLINPUTELEMENT_ALT = -17,
  HTMLINPUTELEMENT_CHECKED = -18,
  HTMLINPUTELEMENT_DISABLED = -19,
  HTMLINPUTELEMENT_MAXLENGTH = -110,
  HTMLINPUTELEMENT_NAME = -111,
  HTMLINPUTELEMENT_READONLY = -112,
  HTMLINPUTELEMENT_SIZE = -113,
  HTMLINPUTELEMENT_SRC = -114,
  HTMLINPUTELEMENT_TABINDEX = -115,
  HTMLINPUTELEMENT_TYPE = -116,
  HTMLINPUTELEMENT_USEMAP = -117,
  HTMLINPUTELEMENT_VALUE = -118
};

/***********************************************************************/
//
// HTMLInputElement Properties Getter
//
PR_STATIC_CALLBACK(JSBool)
GetHTMLInputElementProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  nsIDOMHTMLInputElement *a = (nsIDOMHTMLInputElement*)JS_GetPrivate(cx, obj);

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == a) {
    return JS_TRUE;
  }

  if (JSVAL_IS_INT(id)) {
    switch(JSVAL_TO_INT(id)) {
      case HTMLINPUTELEMENT_DEFAULTVALUE:
      {
        nsAutoString prop;
        if (NS_OK == a->GetDefaultValue(prop)) {
          JSString *jsstring = JS_NewUCStringCopyN(cx, prop, prop.Length());
          // set the return value
          *vp = STRING_TO_JSVAL(jsstring);
        }
        else {
          return JS_FALSE;
        }
        break;
      }
      case HTMLINPUTELEMENT_DEFAULTCHECKED:
      {
        PRBool prop;
        if (NS_OK == a->GetDefaultChecked(&prop)) {
          *vp = BOOLEAN_TO_JSVAL(prop);
        }
        else {
          return JS_FALSE;
        }
        break;
      }
      case HTMLINPUTELEMENT_FORM:
      {
        nsIDOMHTMLFormElement* prop;
        if (NS_OK == a->GetForm(&prop)) {
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
      case HTMLINPUTELEMENT_ACCEPT:
      {
        nsAutoString prop;
        if (NS_OK == a->GetAccept(prop)) {
          JSString *jsstring = JS_NewUCStringCopyN(cx, prop, prop.Length());
          // set the return value
          *vp = STRING_TO_JSVAL(jsstring);
        }
        else {
          return JS_FALSE;
        }
        break;
      }
      case HTMLINPUTELEMENT_ACCESSKEY:
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
      case HTMLINPUTELEMENT_ALIGN:
      {
        nsAutoString prop;
        if (NS_OK == a->GetAlign(prop)) {
          JSString *jsstring = JS_NewUCStringCopyN(cx, prop, prop.Length());
          // set the return value
          *vp = STRING_TO_JSVAL(jsstring);
        }
        else {
          return JS_FALSE;
        }
        break;
      }
      case HTMLINPUTELEMENT_ALT:
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
      case HTMLINPUTELEMENT_CHECKED:
      {
        PRBool prop;
        if (NS_OK == a->GetChecked(&prop)) {
          *vp = BOOLEAN_TO_JSVAL(prop);
        }
        else {
          return JS_FALSE;
        }
        break;
      }
      case HTMLINPUTELEMENT_DISABLED:
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
      case HTMLINPUTELEMENT_MAXLENGTH:
      {
        PRInt32 prop;
        if (NS_OK == a->GetMaxLength(&prop)) {
          *vp = INT_TO_JSVAL(prop);
        }
        else {
          return JS_FALSE;
        }
        break;
      }
      case HTMLINPUTELEMENT_NAME:
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
      case HTMLINPUTELEMENT_READONLY:
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
      case HTMLINPUTELEMENT_SIZE:
      {
        nsAutoString prop;
        if (NS_OK == a->GetSize(prop)) {
          JSString *jsstring = JS_NewUCStringCopyN(cx, prop, prop.Length());
          // set the return value
          *vp = STRING_TO_JSVAL(jsstring);
        }
        else {
          return JS_FALSE;
        }
        break;
      }
      case HTMLINPUTELEMENT_SRC:
      {
        nsAutoString prop;
        if (NS_OK == a->GetSrc(prop)) {
          JSString *jsstring = JS_NewUCStringCopyN(cx, prop, prop.Length());
          // set the return value
          *vp = STRING_TO_JSVAL(jsstring);
        }
        else {
          return JS_FALSE;
        }
        break;
      }
      case HTMLINPUTELEMENT_TABINDEX:
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
      case HTMLINPUTELEMENT_TYPE:
      {
        nsAutoString prop;
        if (NS_OK == a->GetType(prop)) {
          JSString *jsstring = JS_NewUCStringCopyN(cx, prop, prop.Length());
          // set the return value
          *vp = STRING_TO_JSVAL(jsstring);
        }
        else {
          return JS_FALSE;
        }
        break;
      }
      case HTMLINPUTELEMENT_USEMAP:
      {
        nsAutoString prop;
        if (NS_OK == a->GetUseMap(prop)) {
          JSString *jsstring = JS_NewUCStringCopyN(cx, prop, prop.Length());
          // set the return value
          *vp = STRING_TO_JSVAL(jsstring);
        }
        else {
          return JS_FALSE;
        }
        break;
      }
      case HTMLINPUTELEMENT_VALUE:
      {
        nsAutoString prop;
        if (NS_OK == a->GetValue(prop)) {
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
// HTMLInputElement Properties Setter
//
PR_STATIC_CALLBACK(JSBool)
SetHTMLInputElementProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  nsIDOMHTMLInputElement *a = (nsIDOMHTMLInputElement*)JS_GetPrivate(cx, obj);

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == a) {
    return JS_TRUE;
  }

  if (JSVAL_IS_INT(id)) {
    switch(JSVAL_TO_INT(id)) {
      case HTMLINPUTELEMENT_DEFAULTVALUE:
      {
        nsAutoString prop;
        JSString *jsstring;
        if ((jsstring = JS_ValueToString(cx, *vp)) != nsnull) {
          prop.SetString(JS_GetStringChars(jsstring));
        }
        else {
          prop.SetString((const char *)nsnull);
        }
      
        a->SetDefaultValue(prop);
        
        break;
      }
      case HTMLINPUTELEMENT_DEFAULTCHECKED:
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
      
        a->SetDefaultChecked(prop);
        
        break;
      }
      case HTMLINPUTELEMENT_ACCEPT:
      {
        nsAutoString prop;
        JSString *jsstring;
        if ((jsstring = JS_ValueToString(cx, *vp)) != nsnull) {
          prop.SetString(JS_GetStringChars(jsstring));
        }
        else {
          prop.SetString((const char *)nsnull);
        }
      
        a->SetAccept(prop);
        
        break;
      }
      case HTMLINPUTELEMENT_ACCESSKEY:
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
      case HTMLINPUTELEMENT_ALIGN:
      {
        nsAutoString prop;
        JSString *jsstring;
        if ((jsstring = JS_ValueToString(cx, *vp)) != nsnull) {
          prop.SetString(JS_GetStringChars(jsstring));
        }
        else {
          prop.SetString((const char *)nsnull);
        }
      
        a->SetAlign(prop);
        
        break;
      }
      case HTMLINPUTELEMENT_ALT:
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
      case HTMLINPUTELEMENT_CHECKED:
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
      
        a->SetChecked(prop);
        
        break;
      }
      case HTMLINPUTELEMENT_DISABLED:
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
      
        a->SetDisabled(prop);
        
        break;
      }
      case HTMLINPUTELEMENT_MAXLENGTH:
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
      
        a->SetMaxLength(prop);
        
        break;
      }
      case HTMLINPUTELEMENT_NAME:
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
      case HTMLINPUTELEMENT_READONLY:
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
      
        a->SetReadOnly(prop);
        
        break;
      }
      case HTMLINPUTELEMENT_SIZE:
      {
        nsAutoString prop;
        JSString *jsstring;
        if ((jsstring = JS_ValueToString(cx, *vp)) != nsnull) {
          prop.SetString(JS_GetStringChars(jsstring));
        }
        else {
          prop.SetString((const char *)nsnull);
        }
      
        a->SetSize(prop);
        
        break;
      }
      case HTMLINPUTELEMENT_SRC:
      {
        nsAutoString prop;
        JSString *jsstring;
        if ((jsstring = JS_ValueToString(cx, *vp)) != nsnull) {
          prop.SetString(JS_GetStringChars(jsstring));
        }
        else {
          prop.SetString((const char *)nsnull);
        }
      
        a->SetSrc(prop);
        
        break;
      }
      case HTMLINPUTELEMENT_TABINDEX:
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
      case HTMLINPUTELEMENT_USEMAP:
      {
        nsAutoString prop;
        JSString *jsstring;
        if ((jsstring = JS_ValueToString(cx, *vp)) != nsnull) {
          prop.SetString(JS_GetStringChars(jsstring));
        }
        else {
          prop.SetString((const char *)nsnull);
        }
      
        a->SetUseMap(prop);
        
        break;
      }
      case HTMLINPUTELEMENT_VALUE:
      {
        nsAutoString prop;
        JSString *jsstring;
        if ((jsstring = JS_ValueToString(cx, *vp)) != nsnull) {
          prop.SetString(JS_GetStringChars(jsstring));
        }
        else {
          prop.SetString((const char *)nsnull);
        }
      
        a->SetValue(prop);
        
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
// HTMLInputElement finalizer
//
PR_STATIC_CALLBACK(void)
FinalizeHTMLInputElement(JSContext *cx, JSObject *obj)
{
  nsIDOMHTMLInputElement *a = (nsIDOMHTMLInputElement*)JS_GetPrivate(cx, obj);
  
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
// HTMLInputElement enumerate
//
PR_STATIC_CALLBACK(JSBool)
EnumerateHTMLInputElement(JSContext *cx, JSObject *obj)
{
  nsIDOMHTMLInputElement *a = (nsIDOMHTMLInputElement*)JS_GetPrivate(cx, obj);
  
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
// HTMLInputElement resolve
//
PR_STATIC_CALLBACK(JSBool)
ResolveHTMLInputElement(JSContext *cx, JSObject *obj, jsval id)
{
  nsIDOMHTMLInputElement *a = (nsIDOMHTMLInputElement*)JS_GetPrivate(cx, obj);
  
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
// Native method Blur
//
PR_STATIC_CALLBACK(JSBool)
HTMLInputElementBlur(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsIDOMHTMLInputElement *nativeThis = (nsIDOMHTMLInputElement*)JS_GetPrivate(cx, obj);
  JSBool rBool = JS_FALSE;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == nativeThis) {
    return JS_TRUE;
  }

  if (argc >= 0) {

    if (NS_OK != nativeThis->Blur()) {
      return JS_FALSE;
    }

    *rval = JSVAL_VOID;
  }
  else {
    JS_ReportError(cx, "Function blur requires 0 parameters");
    return JS_FALSE;
  }

  return JS_TRUE;
}


//
// Native method Focus
//
PR_STATIC_CALLBACK(JSBool)
HTMLInputElementFocus(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsIDOMHTMLInputElement *nativeThis = (nsIDOMHTMLInputElement*)JS_GetPrivate(cx, obj);
  JSBool rBool = JS_FALSE;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == nativeThis) {
    return JS_TRUE;
  }

  if (argc >= 0) {

    if (NS_OK != nativeThis->Focus()) {
      return JS_FALSE;
    }

    *rval = JSVAL_VOID;
  }
  else {
    JS_ReportError(cx, "Function focus requires 0 parameters");
    return JS_FALSE;
  }

  return JS_TRUE;
}


//
// Native method Select
//
PR_STATIC_CALLBACK(JSBool)
HTMLInputElementSelect(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsIDOMHTMLInputElement *nativeThis = (nsIDOMHTMLInputElement*)JS_GetPrivate(cx, obj);
  JSBool rBool = JS_FALSE;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == nativeThis) {
    return JS_TRUE;
  }

  if (argc >= 0) {

    if (NS_OK != nativeThis->Select()) {
      return JS_FALSE;
    }

    *rval = JSVAL_VOID;
  }
  else {
    JS_ReportError(cx, "Function select requires 0 parameters");
    return JS_FALSE;
  }

  return JS_TRUE;
}


//
// Native method Click
//
PR_STATIC_CALLBACK(JSBool)
HTMLInputElementClick(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsIDOMHTMLInputElement *nativeThis = (nsIDOMHTMLInputElement*)JS_GetPrivate(cx, obj);
  JSBool rBool = JS_FALSE;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == nativeThis) {
    return JS_TRUE;
  }

  if (argc >= 0) {

    if (NS_OK != nativeThis->Click()) {
      return JS_FALSE;
    }

    *rval = JSVAL_VOID;
  }
  else {
    JS_ReportError(cx, "Function click requires 0 parameters");
    return JS_FALSE;
  }

  return JS_TRUE;
}


/***********************************************************************/
//
// class for HTMLInputElement
//
JSClass HTMLInputElementClass = {
  "HTMLInputElement", 
  JSCLASS_HAS_PRIVATE,
  JS_PropertyStub,
  JS_PropertyStub,
  GetHTMLInputElementProperty,
  SetHTMLInputElementProperty,
  EnumerateHTMLInputElement,
  ResolveHTMLInputElement,
  JS_ConvertStub,
  FinalizeHTMLInputElement
};


//
// HTMLInputElement class properties
//
static JSPropertySpec HTMLInputElementProperties[] =
{
  {"defaultValue",    HTMLINPUTELEMENT_DEFAULTVALUE,    JSPROP_ENUMERATE},
  {"defaultChecked",    HTMLINPUTELEMENT_DEFAULTCHECKED,    JSPROP_ENUMERATE},
  {"form",    HTMLINPUTELEMENT_FORM,    JSPROP_ENUMERATE | JSPROP_READONLY},
  {"accept",    HTMLINPUTELEMENT_ACCEPT,    JSPROP_ENUMERATE},
  {"accessKey",    HTMLINPUTELEMENT_ACCESSKEY,    JSPROP_ENUMERATE},
  {"align",    HTMLINPUTELEMENT_ALIGN,    JSPROP_ENUMERATE},
  {"alt",    HTMLINPUTELEMENT_ALT,    JSPROP_ENUMERATE},
  {"checked",    HTMLINPUTELEMENT_CHECKED,    JSPROP_ENUMERATE},
  {"disabled",    HTMLINPUTELEMENT_DISABLED,    JSPROP_ENUMERATE},
  {"maxLength",    HTMLINPUTELEMENT_MAXLENGTH,    JSPROP_ENUMERATE},
  {"name",    HTMLINPUTELEMENT_NAME,    JSPROP_ENUMERATE},
  {"readOnly",    HTMLINPUTELEMENT_READONLY,    JSPROP_ENUMERATE},
  {"size",    HTMLINPUTELEMENT_SIZE,    JSPROP_ENUMERATE},
  {"src",    HTMLINPUTELEMENT_SRC,    JSPROP_ENUMERATE},
  {"tabIndex",    HTMLINPUTELEMENT_TABINDEX,    JSPROP_ENUMERATE},
  {"type",    HTMLINPUTELEMENT_TYPE,    JSPROP_ENUMERATE | JSPROP_READONLY},
  {"useMap",    HTMLINPUTELEMENT_USEMAP,    JSPROP_ENUMERATE},
  {"value",    HTMLINPUTELEMENT_VALUE,    JSPROP_ENUMERATE},
  {0}
};


//
// HTMLInputElement class methods
//
static JSFunctionSpec HTMLInputElementMethods[] = 
{
  {"blur",          HTMLInputElementBlur,     0},
  {"focus",          HTMLInputElementFocus,     0},
  {"select",          HTMLInputElementSelect,     0},
  {"click",          HTMLInputElementClick,     0},
  {0}
};


//
// HTMLInputElement constructor
//
PR_STATIC_CALLBACK(JSBool)
HTMLInputElement(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsIDOMHTMLInputElement *a = (nsIDOMHTMLInputElement*)JS_GetPrivate(cx, obj);
  PRBool result = PR_TRUE;
  
  if (nsnull != a) {
    // get the js object
    nsIJSScriptObject *object;
    if (NS_OK == a->QueryInterface(kIJSScriptObjectIID, (void**)&object)) {
      result = object->Construct(cx, obj, argc, argv, rval);
      NS_RELEASE(object);
    }
  }
  return (result == PR_TRUE) ? JS_TRUE : JS_FALSE;
}


//
// HTMLInputElement class initialization
//
nsresult NS_InitHTMLInputElementClass(nsIScriptContext *aContext, void **aPrototype)
{
  JSContext *jscontext = (JSContext *)aContext->GetNativeContext();
  JSObject *proto = nsnull;
  JSObject *constructor = nsnull;
  JSObject *parent_proto = nsnull;
  JSObject *global = JS_GetGlobalObject(jscontext);
  jsval vp;

  if ((PR_TRUE != JS_LookupProperty(jscontext, global, "HTMLInputElement", &vp)) ||
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
                         &HTMLInputElementClass,      // JSClass
                         HTMLInputElement,            // JSNative ctor
                         0,             // ctor args
                         HTMLInputElementProperties,  // proto props
                         HTMLInputElementMethods,     // proto funcs
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
// Method for creating a new HTMLInputElement JavaScript object
//
extern "C" NS_DOM nsresult NS_NewScriptHTMLInputElement(nsIScriptContext *aContext, nsISupports *aSupports, nsISupports *aParent, void **aReturn)
{
  NS_PRECONDITION(nsnull != aContext && nsnull != aSupports && nsnull != aReturn, "null argument to NS_NewScriptHTMLInputElement");
  JSObject *proto;
  JSObject *parent;
  nsIScriptObjectOwner *owner;
  JSContext *jscontext = (JSContext *)aContext->GetNativeContext();
  nsresult result = NS_OK;
  nsIDOMHTMLInputElement *aHTMLInputElement;

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

  if (NS_OK != NS_InitHTMLInputElementClass(aContext, (void **)&proto)) {
    return NS_ERROR_FAILURE;
  }

  result = aSupports->QueryInterface(kIHTMLInputElementIID, (void **)&aHTMLInputElement);
  if (NS_OK != result) {
    return result;
  }

  // create a js object for this class
  *aReturn = JS_NewObject(jscontext, &HTMLInputElementClass, proto, parent);
  if (nsnull != *aReturn) {
    // connect the native object to the js object
    JS_SetPrivate(jscontext, (JSObject *)*aReturn, aHTMLInputElement);
  }
  else {
    NS_RELEASE(aHTMLInputElement);
    return NS_ERROR_FAILURE; 
  }

  return NS_OK;
}
