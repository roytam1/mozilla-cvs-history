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
#include "nsIServiceManager.h"
#include "nsIScriptContext.h"
#include "nsIScriptSecurityManager.h"
#include "nsIJSScriptObject.h"
#include "nsIScriptObjectOwner.h"
#include "nsIScriptGlobalObject.h"
#include "nsCOMPtr.h"
#include "nsDOMPropEnums.h"
#include "nsString.h"
#include "nsIDOMXULEditorElement.h"
#include "nsIEditorShell.h"


static NS_DEFINE_IID(kIScriptObjectOwnerIID, NS_ISCRIPTOBJECTOWNER_IID);
static NS_DEFINE_IID(kIJSScriptObjectIID, NS_IJSSCRIPTOBJECT_IID);
static NS_DEFINE_IID(kIScriptGlobalObjectIID, NS_ISCRIPTGLOBALOBJECT_IID);
static NS_DEFINE_IID(kIXULEditorElementIID, NS_IDOMXULEDITORELEMENT_IID);
static NS_DEFINE_IID(kIEditorShellIID, NS_IEDITORSHELL_IID);

//
// XULEditorElement property ids
//
enum XULEditorElement_slots {
  XULEDITORELEMENT_EDITORSHELL = -1
};

/***********************************************************************/
//
// XULEditorElement Properties Getter
//
PR_STATIC_CALLBACK(JSBool)
GetXULEditorElementProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  nsIDOMXULEditorElement *a = (nsIDOMXULEditorElement*)nsJSUtils::nsGetNativeThis(cx, obj);

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == a) {
    return JS_TRUE;
  }

  if (JSVAL_IS_INT(id)) {
    nsresult rv;
    NS_WITH_SERVICE(nsIScriptSecurityManager, secMan,
                    NS_SCRIPTSECURITYMANAGER_PROGID, &rv);
    if (NS_FAILED(rv)) {
      return nsJSUtils::nsReportError(cx, obj, NS_ERROR_DOM_SECMAN_ERR);
    }
    switch(JSVAL_TO_INT(id)) {
      case XULEDITORELEMENT_EDITORSHELL:
      {
        rv = secMan->CheckScriptAccess(cx, obj, NS_DOM_PROP_XULEDITORELEMENT_EDITORSHELL, PR_FALSE);
        if (NS_FAILED(rv)) {
          return nsJSUtils::nsReportError(cx, obj, rv);
        }
        nsIEditorShell* prop;
        nsresult result = NS_OK;
        result = a->GetEditorShell(&prop);
        if (NS_SUCCEEDED(result)) {
          // get the js object; n.b., this will do a release on 'prop'
          nsJSUtils::nsConvertXPCObjectToJSVal(prop, NS_GET_IID(nsIEditorShell), cx, obj, vp);
        }
        else {
          return nsJSUtils::nsReportError(cx, obj, result);
        }
        break;
      }
      default:
        return nsJSUtils::nsCallJSScriptObjectGetProperty(a, cx, obj, id, vp);
    }
  }
  else {
    return nsJSUtils::nsCallJSScriptObjectGetProperty(a, cx, obj, id, vp);
  }

  return PR_TRUE;
}

/***********************************************************************/
//
// XULEditorElement Properties Setter
//
PR_STATIC_CALLBACK(JSBool)
SetXULEditorElementProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  nsIDOMXULEditorElement *a = (nsIDOMXULEditorElement*)nsJSUtils::nsGetNativeThis(cx, obj);

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == a) {
    return JS_TRUE;
  }

  if (JSVAL_IS_INT(id)) {
    nsresult rv;
    NS_WITH_SERVICE(nsIScriptSecurityManager, secMan,
                    NS_SCRIPTSECURITYMANAGER_PROGID, &rv);
    if (NS_FAILED(rv)) {
      return nsJSUtils::nsReportError(cx, obj, NS_ERROR_DOM_SECMAN_ERR);
    }
    switch(JSVAL_TO_INT(id)) {
      case 0:
      default:
        return nsJSUtils::nsCallJSScriptObjectSetProperty(a, cx, obj, id, vp);
    }
  }
  else {
    return nsJSUtils::nsCallJSScriptObjectSetProperty(a, cx, obj, id, vp);
  }

  return PR_TRUE;
}


//
// XULEditorElement finalizer
//
PR_STATIC_CALLBACK(void)
FinalizeXULEditorElement(JSContext *cx, JSObject *obj)
{
  nsJSUtils::nsGenericFinalize(cx, obj);
}


//
// XULEditorElement enumerate
//
PR_STATIC_CALLBACK(JSBool)
EnumerateXULEditorElement(JSContext *cx, JSObject *obj)
{
  return nsJSUtils::nsGenericEnumerate(cx, obj);
}


//
// XULEditorElement resolve
//
PR_STATIC_CALLBACK(JSBool)
ResolveXULEditorElement(JSContext *cx, JSObject *obj, jsval id)
{
  return nsJSUtils::nsGenericResolve(cx, obj, id);
}


/***********************************************************************/
//
// class for XULEditorElement
//
JSClass XULEditorElementClass = {
  "XULEditorElement", 
  JSCLASS_HAS_PRIVATE | JSCLASS_PRIVATE_IS_NSISUPPORTS,
  JS_PropertyStub,
  JS_PropertyStub,
  GetXULEditorElementProperty,
  SetXULEditorElementProperty,
  EnumerateXULEditorElement,
  ResolveXULEditorElement,
  JS_ConvertStub,
  FinalizeXULEditorElement,
  nsnull,
  nsJSUtils::nsCheckAccess
};


//
// XULEditorElement class properties
//
static JSPropertySpec XULEditorElementProperties[] =
{
  {"editorShell",    XULEDITORELEMENT_EDITORSHELL,    JSPROP_ENUMERATE | JSPROP_READONLY},
  {0}
};


//
// XULEditorElement class methods
//
static JSFunctionSpec XULEditorElementMethods[] = 
{
  {0}
};


//
// XULEditorElement constructor
//
PR_STATIC_CALLBACK(JSBool)
XULEditorElement(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  return JS_FALSE;
}


//
// XULEditorElement class initialization
//
extern "C" NS_DOM nsresult NS_InitXULEditorElementClass(nsIScriptContext *aContext, void **aPrototype)
{
  JSContext *jscontext = (JSContext *)aContext->GetNativeContext();
  JSObject *proto = nsnull;
  JSObject *constructor = nsnull;
  JSObject *parent_proto = nsnull;
  JSObject *global = JS_GetGlobalObject(jscontext);
  jsval vp;

  if ((PR_TRUE != JS_LookupProperty(jscontext, global, "XULEditorElement", &vp)) ||
      !JSVAL_IS_OBJECT(vp) ||
      ((constructor = JSVAL_TO_OBJECT(vp)) == nsnull) ||
      (PR_TRUE != JS_LookupProperty(jscontext, JSVAL_TO_OBJECT(vp), "prototype", &vp)) || 
      !JSVAL_IS_OBJECT(vp)) {

    if (NS_OK != NS_InitXULElementClass(aContext, (void **)&parent_proto)) {
      return NS_ERROR_FAILURE;
    }
    proto = JS_InitClass(jscontext,     // context
                         global,        // global object
                         parent_proto,  // parent proto 
                         &XULEditorElementClass,      // JSClass
                         XULEditorElement,            // JSNative ctor
                         0,             // ctor args
                         XULEditorElementProperties,  // proto props
                         XULEditorElementMethods,     // proto funcs
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
// Method for creating a new XULEditorElement JavaScript object
//
extern "C" NS_DOM nsresult NS_NewScriptXULEditorElement(nsIScriptContext *aContext, nsISupports *aSupports, nsISupports *aParent, void **aReturn)
{
  NS_PRECONDITION(nsnull != aContext && nsnull != aSupports && nsnull != aReturn, "null argument to NS_NewScriptXULEditorElement");
  JSObject *proto;
  JSObject *parent;
  nsIScriptObjectOwner *owner;
  JSContext *jscontext = (JSContext *)aContext->GetNativeContext();
  nsresult result = NS_OK;
  nsIDOMXULEditorElement *aXULEditorElement;

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

  if (NS_OK != NS_InitXULEditorElementClass(aContext, (void **)&proto)) {
    return NS_ERROR_FAILURE;
  }

  result = aSupports->QueryInterface(kIXULEditorElementIID, (void **)&aXULEditorElement);
  if (NS_OK != result) {
    return result;
  }

  // create a js object for this class
  *aReturn = JS_NewObject(jscontext, &XULEditorElementClass, proto, parent);
  if (nsnull != *aReturn) {
    // connect the native object to the js object
    JS_SetPrivate(jscontext, (JSObject *)*aReturn, aXULEditorElement);
  }
  else {
    NS_RELEASE(aXULEditorElement);
    return NS_ERROR_FAILURE; 
  }

  return NS_OK;
}
