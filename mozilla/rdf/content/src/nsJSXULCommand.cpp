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
#include "nsIDOMXULCommand.h"


static NS_DEFINE_IID(kIScriptObjectOwnerIID, NS_ISCRIPTOBJECTOWNER_IID);
static NS_DEFINE_IID(kIJSScriptObjectIID, NS_IJSSCRIPTOBJECT_IID);
static NS_DEFINE_IID(kIScriptGlobalObjectIID, NS_ISCRIPTGLOBALOBJECT_IID);
static NS_DEFINE_IID(kIXULCommandIID, NS_IDOMXULCOMMAND_IID);

NS_DEF_PTR(nsIDOMXULCommand);


/***********************************************************************/
//
// XULCommand Properties Getter
//
PR_STATIC_CALLBACK(JSBool)
GetXULCommandProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  nsIDOMXULCommand *a = (nsIDOMXULCommand*)JS_GetPrivate(cx, obj);

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == a) {
    return JS_TRUE;
  }

  if (JSVAL_IS_INT(id)) {
    switch(JSVAL_TO_INT(id)) {
      case 0:
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
// XULCommand Properties Setter
//
PR_STATIC_CALLBACK(JSBool)
SetXULCommandProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  nsIDOMXULCommand *a = (nsIDOMXULCommand*)JS_GetPrivate(cx, obj);

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == a) {
    return JS_TRUE;
  }

  if (JSVAL_IS_INT(id)) {
    switch(JSVAL_TO_INT(id)) {
      case 0:
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
// XULCommand finalizer
//
PR_STATIC_CALLBACK(void)
FinalizeXULCommand(JSContext *cx, JSObject *obj)
{
  nsJSUtils::nsGenericFinalize(cx, obj);
}


//
// XULCommand enumerate
//
PR_STATIC_CALLBACK(JSBool)
EnumerateXULCommand(JSContext *cx, JSObject *obj)
{
  return nsJSUtils::nsGenericEnumerate(cx, obj);
}


//
// XULCommand resolve
//
PR_STATIC_CALLBACK(JSBool)
ResolveXULCommand(JSContext *cx, JSObject *obj, jsval id)
{
  return nsJSUtils::nsGenericResolve(cx, obj, id);
}


//
// Native method DoCommand
//
PR_STATIC_CALLBACK(JSBool)
XULCommandDoCommand(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsIDOMXULCommand *nativeThis = (nsIDOMXULCommand*)JS_GetPrivate(cx, obj);
  JSBool rBool = JS_FALSE;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == nativeThis) {
    return JS_TRUE;
  }

  if (argc >= 0) {

    if (NS_OK != nativeThis->DoCommand()) {
      return JS_FALSE;
    }

    *rval = JSVAL_VOID;
  }
  else {
    JS_ReportError(cx, "Function doCommand requires 0 parameters");
    return JS_FALSE;
  }

  return JS_TRUE;
}


/***********************************************************************/
//
// class for XULCommand
//
JSClass XULCommandClass = {
  "XULCommand", 
  JSCLASS_HAS_PRIVATE,
  JS_PropertyStub,
  JS_PropertyStub,
  GetXULCommandProperty,
  SetXULCommandProperty,
  EnumerateXULCommand,
  ResolveXULCommand,
  JS_ConvertStub,
  FinalizeXULCommand
};


//
// XULCommand class properties
//
static JSPropertySpec XULCommandProperties[] =
{
  {0}
};


//
// XULCommand class methods
//
static JSFunctionSpec XULCommandMethods[] = 
{
  {"doCommand",          XULCommandDoCommand,     0},
  {0}
};


//
// XULCommand constructor
//
PR_STATIC_CALLBACK(JSBool)
XULCommand(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  return JS_FALSE;
}


//
// XULCommand class initialization
//
nsresult NS_InitXULCommandClass(nsIScriptContext *aContext, void **aPrototype)
{
  JSContext *jscontext = (JSContext *)aContext->GetNativeContext();
  JSObject *proto = nsnull;
  JSObject *constructor = nsnull;
  JSObject *parent_proto = nsnull;
  JSObject *global = JS_GetGlobalObject(jscontext);
  jsval vp;

  if ((PR_TRUE != JS_LookupProperty(jscontext, global, "XULCommand", &vp)) ||
      !JSVAL_IS_OBJECT(vp) ||
      ((constructor = JSVAL_TO_OBJECT(vp)) == nsnull) ||
      (PR_TRUE != JS_LookupProperty(jscontext, JSVAL_TO_OBJECT(vp), "prototype", &vp)) || 
      !JSVAL_IS_OBJECT(vp)) {

    proto = JS_InitClass(jscontext,     // context
                         global,        // global object
                         parent_proto,  // parent proto 
                         &XULCommandClass,      // JSClass
                         XULCommand,            // JSNative ctor
                         0,             // ctor args
                         XULCommandProperties,  // proto props
                         XULCommandMethods,     // proto funcs
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
// Method for creating a new XULCommand JavaScript object
//
extern "C" nsresult NS_NewScriptXULCommand(nsIScriptContext *aContext, nsISupports *aSupports, nsISupports *aParent, void **aReturn)
{
  NS_PRECONDITION(nsnull != aContext && nsnull != aSupports && nsnull != aReturn, "null argument to NS_NewScriptXULCommand");
  JSObject *proto;
  JSObject *parent;
  nsIScriptObjectOwner *owner;
  JSContext *jscontext = (JSContext *)aContext->GetNativeContext();
  nsresult result = NS_OK;
  nsIDOMXULCommand *aXULCommand;

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

  if (NS_OK != NS_InitXULCommandClass(aContext, (void **)&proto)) {
    return NS_ERROR_FAILURE;
  }

  result = aSupports->QueryInterface(kIXULCommandIID, (void **)&aXULCommand);
  if (NS_OK != result) {
    return result;
  }

  // create a js object for this class
  *aReturn = JS_NewObject(jscontext, &XULCommandClass, proto, parent);
  if (nsnull != *aReturn) {
    // connect the native object to the js object
    JS_SetPrivate(jscontext, (JSObject *)*aReturn, aXULCommand);
  }
  else {
    NS_RELEASE(aXULCommand);
    return NS_ERROR_FAILURE; 
  }

  return NS_OK;
}
