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
#include "nsIDOMMsgAppCore.h"
#include "nsIDOMWindow.h"
#include "nsIScriptNameSpaceManager.h"
#include "nsRepository.h"
#include "nsDOMCID.h"
#include "nsIDOMXULTreeElement.h"


static NS_DEFINE_IID(kIScriptObjectOwnerIID, NS_ISCRIPTOBJECTOWNER_IID);
static NS_DEFINE_IID(kIJSScriptObjectIID, NS_IJSSCRIPTOBJECT_IID);
static NS_DEFINE_IID(kIScriptGlobalObjectIID, NS_ISCRIPTGLOBALOBJECT_IID);
static NS_DEFINE_IID(kIMsgAppCoreIID, NS_IDOMMSGAPPCORE_IID);
static NS_DEFINE_IID(kIWindowIID, NS_IDOMWINDOW_IID);

NS_DEF_PTR(nsIDOMMsgAppCore);
NS_DEF_PTR(nsIDOMWindow);


/***********************************************************************/
//
// MsgAppCore Properties Getter
//
PR_STATIC_CALLBACK(JSBool)
GetMsgAppCoreProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  nsIDOMMsgAppCore *a = (nsIDOMMsgAppCore*)JS_GetPrivate(cx, obj);

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
// MsgAppCore Properties Setter
//
PR_STATIC_CALLBACK(JSBool)
SetMsgAppCoreProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  nsIDOMMsgAppCore *a = (nsIDOMMsgAppCore*)JS_GetPrivate(cx, obj);

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
// MsgAppCore finalizer
//
PR_STATIC_CALLBACK(void)
FinalizeMsgAppCore(JSContext *cx, JSObject *obj)
{
  nsJSUtils::nsGenericFinalize(cx, obj);
}


//
// MsgAppCore enumerate
//
PR_STATIC_CALLBACK(JSBool)
EnumerateMsgAppCore(JSContext *cx, JSObject *obj)
{
  return nsJSUtils::nsGenericEnumerate(cx, obj);
}


//
// MsgAppCore resolve
//
PR_STATIC_CALLBACK(JSBool)
ResolveMsgAppCore(JSContext *cx, JSObject *obj, jsval id)
{
  return nsJSUtils::nsGenericResolve(cx, obj, id);
}


//
// Native method GetNewMail
//
PR_STATIC_CALLBACK(JSBool)
MsgAppCoreGetNewMail(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsIDOMMsgAppCore *nativeThis = (nsIDOMMsgAppCore*)JS_GetPrivate(cx, obj);
  JSBool rBool = JS_FALSE;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == nativeThis) {
    return JS_TRUE;
  }

  if (argc >= 0) {

    if (NS_OK != nativeThis->GetNewMail()) {
      return JS_FALSE;
    }

    *rval = JSVAL_VOID;
  }
  else {
    JS_ReportError(cx, "Function GetNewMail requires 0 parameters");
    return JS_FALSE;
  }

  return JS_TRUE;
}


//
// Native method Open3PaneWindow
//
PR_STATIC_CALLBACK(JSBool)
MsgAppCoreOpen3PaneWindow(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsIDOMMsgAppCore *nativeThis = (nsIDOMMsgAppCore*)JS_GetPrivate(cx, obj);
  JSBool rBool = JS_FALSE;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == nativeThis) {
    return JS_TRUE;
  }

  if (argc >= 0) {

    if (NS_OK != nativeThis->Open3PaneWindow()) {
      return JS_FALSE;
    }

    *rval = JSVAL_VOID;
  }
  else {
    JS_ReportError(cx, "Function Open3PaneWindow requires 0 parameters");
    return JS_FALSE;
  }

  return JS_TRUE;
}


//
// Native method SetWindow
//
PR_STATIC_CALLBACK(JSBool)
MsgAppCoreSetWindow(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsIDOMMsgAppCore *nativeThis = (nsIDOMMsgAppCore*)JS_GetPrivate(cx, obj);
  JSBool rBool = JS_FALSE;
  nsIDOMWindowPtr b0;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == nativeThis) {
    return JS_TRUE;
  }

  if (argc >= 1) {

    if (JS_FALSE == nsJSUtils::nsConvertJSValToObject((nsISupports **)&b0,
                                           kIWindowIID,
                                           "Window",
                                           cx,
                                           argv[0])) {
      return JS_FALSE;
    }

    if (NS_OK != nativeThis->SetWindow(b0)) {
      return JS_FALSE;
    }

    *rval = JSVAL_VOID;
  }
  else {
    JS_ReportError(cx, "Function SetWindow requires 1 parameters");
    return JS_FALSE;
  }

  return JS_TRUE;
}


//
// Native method OpenURL
//
PR_STATIC_CALLBACK(JSBool)
MsgAppCoreOpenURL(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsIDOMMsgAppCore *nativeThis = (nsIDOMMsgAppCore*)JS_GetPrivate(cx, obj);
  JSBool rBool = JS_FALSE;
  nsAutoString b0;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == nativeThis) {
    return JS_TRUE;
  }

  if (argc >= 1) {

    nsJSUtils::nsConvertJSValToString(b0, cx, argv[0]);

	char * url = b0.ToNewCString();

    if (NS_OK != nativeThis->OpenURL(url)) {
      return JS_FALSE;
    }

	delete [] url;

    *rval = JSVAL_VOID;
  }
  else {
    JS_ReportError(cx, "Function OpenURL requires 1 parameters");
    return JS_FALSE;
  }

  return JS_TRUE;
}

//
// Native method DeleteMessage
//
PR_STATIC_CALLBACK(JSBool)
MsgAppCoreDeleteMessage(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsIDOMMsgAppCore *nativeThis = (nsIDOMMsgAppCore*)JS_GetPrivate(cx, obj);
  JSBool rBool = JS_FALSE;
	nsIDOMNodeList *nodeList;
	nsIDOMXULTreeElement *tree;
	const nsString typeName;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == nativeThis) {
    return JS_TRUE;
  }

  if (argc >= 2) {
		rBool = nsJSUtils::nsConvertJSValToObject((nsISupports**)&tree,
																			nsIDOMXULTreeElement::GetIID(),
                                  typeName,
                                  cx,
                                  argv[0]);

		rBool = rBool && nsJSUtils::nsConvertJSValToObject((nsISupports**)&nodeList,
																			nsIDOMNodeList::GetIID(),
                                  typeName,
                                  cx,
                                  argv[1]);

		
    if (!rBool || NS_OK != nativeThis->DeleteMessage(tree, nodeList)) {
      return JS_FALSE;
    }

		NS_RELEASE(nodeList);
		NS_RELEASE(tree);
    *rval = JSVAL_VOID;
  }
  else {
    JS_ReportError(cx, "Function DeleteMessage requires 1 parameters");
    return JS_FALSE;
  }

  return JS_TRUE;
}
/***********************************************************************/
//
// class for MsgAppCore
//
JSClass MsgAppCoreClass = {
  "MsgAppCore", 
  JSCLASS_HAS_PRIVATE,
  JS_PropertyStub,
  JS_PropertyStub,
  GetMsgAppCoreProperty,
  SetMsgAppCoreProperty,
  EnumerateMsgAppCore,
  ResolveMsgAppCore,
  JS_ConvertStub,
  FinalizeMsgAppCore
};


//
// MsgAppCore class properties
//
static JSPropertySpec MsgAppCoreProperties[] =
{
  {0}
};


//
// MsgAppCore class methods
//
static JSFunctionSpec MsgAppCoreMethods[] = 
{
  {"GetNewMail",          MsgAppCoreGetNewMail,     0},
  {"Open3PaneWindow",          MsgAppCoreOpen3PaneWindow,     0},
  {"SetWindow",          MsgAppCoreSetWindow,     1},
  {"OpenURL",          MsgAppCoreOpenURL,     1},
  {"DeleteMessage",          MsgAppCoreDeleteMessage,     2},
  {0}
};


//
// MsgAppCore constructor
//
PR_STATIC_CALLBACK(JSBool)
MsgAppCore(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsresult result;
  nsIID classID;
  nsIScriptContext* context = (nsIScriptContext*)JS_GetContextPrivate(cx);
  nsIScriptNameSpaceManager* manager;
  nsIDOMMsgAppCore *nativeThis;
  nsIScriptObjectOwner *owner = nsnull;

  static NS_DEFINE_IID(kIDOMMsgAppCoreIID, NS_IDOMMSGAPPCORE_IID);

  result = context->GetNameSpaceManager(&manager);
  if (NS_OK != result) {
    return JS_FALSE;
  }

  result = manager->LookupName("MsgAppCore", PR_TRUE, classID);
  NS_RELEASE(manager);
  if (NS_OK != result) {
    return JS_FALSE;
  }

  result = nsRepository::CreateInstance(classID,
                                        nsnull,
                                        kIDOMMsgAppCoreIID,
                                        (void **)&nativeThis);
  if (NS_OK != result) {
    return JS_FALSE;
  }

  // XXX We should be calling Init() on the instance

  result = nativeThis->QueryInterface(kIScriptObjectOwnerIID, (void **)&owner);
  if (NS_OK != result) {
    NS_RELEASE(nativeThis);
    return JS_FALSE;
  }

  owner->SetScriptObject((void *)obj);
  JS_SetPrivate(cx, obj, nativeThis);

  NS_RELEASE(owner);
  return JS_TRUE;
}

//
// MsgAppCore class initialization
//
nsresult NS_InitMsgAppCoreClass(nsIScriptContext *aContext, void **aPrototype)
{
  JSContext *jscontext = (JSContext *)aContext->GetNativeContext();
  JSObject *proto = nsnull;
  JSObject *constructor = nsnull;
  JSObject *parent_proto = nsnull;
  JSObject *global = JS_GetGlobalObject(jscontext);
  jsval vp;

  if ((PR_TRUE != JS_LookupProperty(jscontext, global, "MsgAppCore", &vp)) ||
      !JSVAL_IS_OBJECT(vp) ||
      ((constructor = JSVAL_TO_OBJECT(vp)) == nsnull) ||
      (PR_TRUE != JS_LookupProperty(jscontext, JSVAL_TO_OBJECT(vp), "prototype", &vp)) || 
      !JSVAL_IS_OBJECT(vp)) {

    if (NS_OK != NS_InitBaseAppCoreClass(aContext, (void **)&parent_proto)) {
      return NS_ERROR_FAILURE;
    }
    proto = JS_InitClass(jscontext,     // context
                         global,        // global object
                         parent_proto,  // parent proto 
                         &MsgAppCoreClass,      // JSClass
                         MsgAppCore,            // JSNative ctor
                         0,             // ctor args
                         MsgAppCoreProperties,  // proto props
                         MsgAppCoreMethods,     // proto funcs
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
// Method for creating a new MsgAppCore JavaScript object
//
extern "C" nsresult NS_NewScriptMsgAppCore(nsIScriptContext *aContext, nsISupports *aSupports, nsISupports *aParent, void **aReturn)
{
  NS_PRECONDITION(nsnull != aContext && nsnull != aSupports && nsnull != aReturn, "null argument to NS_NewScriptMsgAppCore");
  JSObject *proto;
  JSObject *parent;
  nsIScriptObjectOwner *owner;
  JSContext *jscontext = (JSContext *)aContext->GetNativeContext();
  nsresult result = NS_OK;
  nsIDOMMsgAppCore *aMsgAppCore;

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

  if (NS_OK != NS_InitMsgAppCoreClass(aContext, (void **)&proto)) {
    return NS_ERROR_FAILURE;
  }

  result = aSupports->QueryInterface(kIMsgAppCoreIID, (void **)&aMsgAppCore);
  if (NS_OK != result) {
    return result;
  }

  // create a js object for this class
  *aReturn = JS_NewObject(jscontext, &MsgAppCoreClass, proto, parent);
  if (nsnull != *aReturn) {
    // connect the native object to the js object
    JS_SetPrivate(jscontext, (JSObject *)*aReturn, aMsgAppCore);
  }
  else {
    NS_RELEASE(aMsgAppCore);
    return NS_ERROR_FAILURE; 
  }

  return NS_OK;
}
