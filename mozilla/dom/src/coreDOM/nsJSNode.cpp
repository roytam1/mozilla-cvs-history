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
#include "nsIScriptContext.h"
#include "nsIScriptSecurityManager.h"
#include "nsIJSScriptObject.h"
#include "nsIScriptObjectOwner.h"
#include "nsIScriptGlobalObject.h"
#include "nsCOMPtr.h"
#include "nsDOMPropEnums.h"
#include "nsIPtr.h"
#include "nsString.h"
#include "nsIDOMDocument.h"
#include "nsIDOMNamedNodeMap.h"
#include "nsIDOMNode.h"
#include "nsIDOMEventListener.h"
#include "nsIDOMEventTarget.h"
#include "nsIDOMNodeList.h"


static NS_DEFINE_IID(kIScriptObjectOwnerIID, NS_ISCRIPTOBJECTOWNER_IID);
static NS_DEFINE_IID(kIJSScriptObjectIID, NS_IJSSCRIPTOBJECT_IID);
static NS_DEFINE_IID(kIScriptGlobalObjectIID, NS_ISCRIPTGLOBALOBJECT_IID);
static NS_DEFINE_IID(kIDocumentIID, NS_IDOMDOCUMENT_IID);
static NS_DEFINE_IID(kINamedNodeMapIID, NS_IDOMNAMEDNODEMAP_IID);
static NS_DEFINE_IID(kINodeIID, NS_IDOMNODE_IID);
static NS_DEFINE_IID(kIEventListenerIID, NS_IDOMEVENTLISTENER_IID);
static NS_DEFINE_IID(kIEventTargetIID, NS_IDOMEVENTTARGET_IID);
static NS_DEFINE_IID(kINodeListIID, NS_IDOMNODELIST_IID);

NS_DEF_PTR(nsIDOMDocument);
NS_DEF_PTR(nsIDOMNamedNodeMap);
NS_DEF_PTR(nsIDOMNode);
NS_DEF_PTR(nsIDOMEventListener);
NS_DEF_PTR(nsIDOMEventTarget);
NS_DEF_PTR(nsIDOMNodeList);

//
// Node property ids
//
enum Node_slots {
  NODE_NODENAME = -1,
  NODE_NODEVALUE = -2,
  NODE_NODETYPE = -3,
  NODE_PARENTNODE = -4,
  NODE_CHILDNODES = -5,
  NODE_FIRSTCHILD = -6,
  NODE_LASTCHILD = -7,
  NODE_PREVIOUSSIBLING = -8,
  NODE_NEXTSIBLING = -9,
  NODE_ATTRIBUTES = -10,
  NODE_OWNERDOCUMENT = -11
};

/***********************************************************************/
//
// Node Properties Getter
//
PR_STATIC_CALLBACK(JSBool)
GetNodeProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  nsIDOMNode *a = (nsIDOMNode*)nsJSUtils::nsGetNativeThis(cx, obj);

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == a) {
    return JS_TRUE;
  }

  if (JSVAL_IS_INT(id)) {
    nsIScriptContext *scriptCX = (nsIScriptContext *)JS_GetContextPrivate(cx);
    nsCOMPtr<nsIScriptSecurityManager> secMan;
    if (NS_OK != scriptCX->GetSecurityManager(getter_AddRefs(secMan))) {
      return nsJSUtils::nsReportError(cx, NS_ERROR_DOM_SECMAN_ERR);
    }
    switch(JSVAL_TO_INT(id)) {
      case NODE_NODENAME:
      {
        PRBool ok = PR_FALSE;
        secMan->CheckScriptAccess(scriptCX, obj, NS_DOM_PROP_NODE_NODENAME, PR_FALSE, &ok);
        if (!ok) {
          return nsJSUtils::nsReportError(cx, NS_ERROR_DOM_SECURITY_ERR);
        }
        nsAutoString prop;
        nsresult result = NS_OK;
        result = a->GetNodeName(prop);
        if (NS_SUCCEEDED(result)) {
          nsJSUtils::nsConvertStringToJSVal(prop, cx, vp);
        }
        else {
          return nsJSUtils::nsReportError(cx, result);
        }
        break;
      }
      case NODE_NODEVALUE:
      {
        PRBool ok = PR_FALSE;
        secMan->CheckScriptAccess(scriptCX, obj, NS_DOM_PROP_NODE_NODEVALUE, PR_FALSE, &ok);
        if (!ok) {
          return nsJSUtils::nsReportError(cx, NS_ERROR_DOM_SECURITY_ERR);
        }
        nsAutoString prop;
        nsresult result = NS_OK;
        result = a->GetNodeValue(prop);
        if (NS_SUCCEEDED(result)) {
          nsJSUtils::nsConvertStringToJSVal(prop, cx, vp);
        }
        else {
          return nsJSUtils::nsReportError(cx, result);
        }
        break;
      }
      case NODE_NODETYPE:
      {
        PRBool ok = PR_FALSE;
        secMan->CheckScriptAccess(scriptCX, obj, NS_DOM_PROP_NODE_NODETYPE, PR_FALSE, &ok);
        if (!ok) {
          return nsJSUtils::nsReportError(cx, NS_ERROR_DOM_SECURITY_ERR);
        }
        PRUint16 prop;
        nsresult result = NS_OK;
        result = a->GetNodeType(&prop);
        if (NS_SUCCEEDED(result)) {
          *vp = INT_TO_JSVAL(prop);
        }
        else {
          return nsJSUtils::nsReportError(cx, result);
        }
        break;
      }
      case NODE_PARENTNODE:
      {
        PRBool ok = PR_FALSE;
        secMan->CheckScriptAccess(scriptCX, obj, NS_DOM_PROP_NODE_PARENTNODE, PR_FALSE, &ok);
        if (!ok) {
          return nsJSUtils::nsReportError(cx, NS_ERROR_DOM_SECURITY_ERR);
        }
        nsIDOMNode* prop;
        nsresult result = NS_OK;
        result = a->GetParentNode(&prop);
        if (NS_SUCCEEDED(result)) {
          // get the js object
          nsJSUtils::nsConvertObjectToJSVal((nsISupports *)prop, cx, vp);
        }
        else {
          return nsJSUtils::nsReportError(cx, result);
        }
        break;
      }
      case NODE_CHILDNODES:
      {
        PRBool ok = PR_FALSE;
        secMan->CheckScriptAccess(scriptCX, obj, NS_DOM_PROP_NODE_CHILDNODES, PR_FALSE, &ok);
        if (!ok) {
          return nsJSUtils::nsReportError(cx, NS_ERROR_DOM_SECURITY_ERR);
        }
        nsIDOMNodeList* prop;
        nsresult result = NS_OK;
        result = a->GetChildNodes(&prop);
        if (NS_SUCCEEDED(result)) {
          // get the js object
          nsJSUtils::nsConvertObjectToJSVal((nsISupports *)prop, cx, vp);
        }
        else {
          return nsJSUtils::nsReportError(cx, result);
        }
        break;
      }
      case NODE_FIRSTCHILD:
      {
        PRBool ok = PR_FALSE;
        secMan->CheckScriptAccess(scriptCX, obj, NS_DOM_PROP_NODE_FIRSTCHILD, PR_FALSE, &ok);
        if (!ok) {
          return nsJSUtils::nsReportError(cx, NS_ERROR_DOM_SECURITY_ERR);
        }
        nsIDOMNode* prop;
        nsresult result = NS_OK;
        result = a->GetFirstChild(&prop);
        if (NS_SUCCEEDED(result)) {
          // get the js object
          nsJSUtils::nsConvertObjectToJSVal((nsISupports *)prop, cx, vp);
        }
        else {
          return nsJSUtils::nsReportError(cx, result);
        }
        break;
      }
      case NODE_LASTCHILD:
      {
        PRBool ok = PR_FALSE;
        secMan->CheckScriptAccess(scriptCX, obj, NS_DOM_PROP_NODE_LASTCHILD, PR_FALSE, &ok);
        if (!ok) {
          return nsJSUtils::nsReportError(cx, NS_ERROR_DOM_SECURITY_ERR);
        }
        nsIDOMNode* prop;
        nsresult result = NS_OK;
        result = a->GetLastChild(&prop);
        if (NS_SUCCEEDED(result)) {
          // get the js object
          nsJSUtils::nsConvertObjectToJSVal((nsISupports *)prop, cx, vp);
        }
        else {
          return nsJSUtils::nsReportError(cx, result);
        }
        break;
      }
      case NODE_PREVIOUSSIBLING:
      {
        PRBool ok = PR_FALSE;
        secMan->CheckScriptAccess(scriptCX, obj, NS_DOM_PROP_NODE_PREVIOUSSIBLING, PR_FALSE, &ok);
        if (!ok) {
          return nsJSUtils::nsReportError(cx, NS_ERROR_DOM_SECURITY_ERR);
        }
        nsIDOMNode* prop;
        nsresult result = NS_OK;
        result = a->GetPreviousSibling(&prop);
        if (NS_SUCCEEDED(result)) {
          // get the js object
          nsJSUtils::nsConvertObjectToJSVal((nsISupports *)prop, cx, vp);
        }
        else {
          return nsJSUtils::nsReportError(cx, result);
        }
        break;
      }
      case NODE_NEXTSIBLING:
      {
        PRBool ok = PR_FALSE;
        secMan->CheckScriptAccess(scriptCX, obj, NS_DOM_PROP_NODE_NEXTSIBLING, PR_FALSE, &ok);
        if (!ok) {
          return nsJSUtils::nsReportError(cx, NS_ERROR_DOM_SECURITY_ERR);
        }
        nsIDOMNode* prop;
        nsresult result = NS_OK;
        result = a->GetNextSibling(&prop);
        if (NS_SUCCEEDED(result)) {
          // get the js object
          nsJSUtils::nsConvertObjectToJSVal((nsISupports *)prop, cx, vp);
        }
        else {
          return nsJSUtils::nsReportError(cx, result);
        }
        break;
      }
      case NODE_ATTRIBUTES:
      {
        PRBool ok = PR_FALSE;
        secMan->CheckScriptAccess(scriptCX, obj, NS_DOM_PROP_NODE_ATTRIBUTES, PR_FALSE, &ok);
        if (!ok) {
          return nsJSUtils::nsReportError(cx, NS_ERROR_DOM_SECURITY_ERR);
        }
        nsIDOMNamedNodeMap* prop;
        nsresult result = NS_OK;
        result = a->GetAttributes(&prop);
        if (NS_SUCCEEDED(result)) {
          // get the js object
          nsJSUtils::nsConvertObjectToJSVal((nsISupports *)prop, cx, vp);
        }
        else {
          return nsJSUtils::nsReportError(cx, result);
        }
        break;
      }
      case NODE_OWNERDOCUMENT:
      {
        PRBool ok = PR_FALSE;
        secMan->CheckScriptAccess(scriptCX, obj, NS_DOM_PROP_NODE_OWNERDOCUMENT, PR_FALSE, &ok);
        if (!ok) {
          return nsJSUtils::nsReportError(cx, NS_ERROR_DOM_SECURITY_ERR);
        }
        nsIDOMDocument* prop;
        nsresult result = NS_OK;
        result = a->GetOwnerDocument(&prop);
        if (NS_SUCCEEDED(result)) {
          // get the js object
          nsJSUtils::nsConvertObjectToJSVal((nsISupports *)prop, cx, vp);
        }
        else {
          return nsJSUtils::nsReportError(cx, result);
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
// Node Properties Setter
//
PR_STATIC_CALLBACK(JSBool)
SetNodeProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  nsIDOMNode *a = (nsIDOMNode*)nsJSUtils::nsGetNativeThis(cx, obj);

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == a) {
    return JS_TRUE;
  }

  if (JSVAL_IS_INT(id)) {
    nsIScriptContext *scriptCX = (nsIScriptContext *)JS_GetContextPrivate(cx);
    nsCOMPtr<nsIScriptSecurityManager> secMan;
    if (NS_OK != scriptCX->GetSecurityManager(getter_AddRefs(secMan))) {
      return nsJSUtils::nsReportError(cx, NS_ERROR_DOM_SECMAN_ERR);
    }
    switch(JSVAL_TO_INT(id)) {
      case NODE_NODEVALUE:
      {
        PRBool ok = PR_FALSE;
        secMan->CheckScriptAccess(scriptCX, obj, NS_DOM_PROP_NODE_NODEVALUE, PR_TRUE, &ok);
        if (!ok) {
          return nsJSUtils::nsReportError(cx, NS_ERROR_DOM_SECURITY_ERR);
        }
        nsAutoString prop;
        nsJSUtils::nsConvertJSValToString(prop, cx, *vp);
      
        a->SetNodeValue(prop);
        
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
// Node finalizer
//
PR_STATIC_CALLBACK(void)
FinalizeNode(JSContext *cx, JSObject *obj)
{
  nsJSUtils::nsGenericFinalize(cx, obj);
}


//
// Node enumerate
//
PR_STATIC_CALLBACK(JSBool)
EnumerateNode(JSContext *cx, JSObject *obj)
{
  return nsJSUtils::nsGenericEnumerate(cx, obj);
}


//
// Node resolve
//
PR_STATIC_CALLBACK(JSBool)
ResolveNode(JSContext *cx, JSObject *obj, jsval id)
{
  return nsJSUtils::nsGenericResolve(cx, obj, id);
}


//
// Native method InsertBefore
//
PR_STATIC_CALLBACK(JSBool)
NodeInsertBefore(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsIDOMNode *nativeThis = (nsIDOMNode*)nsJSUtils::nsGetNativeThis(cx, obj);
  nsresult result = NS_OK;
  nsIDOMNode* nativeRet;
  nsIDOMNodePtr b0;
  nsIDOMNodePtr b1;

  *rval = JSVAL_NULL;

  nsIScriptContext *scriptCX = (nsIScriptContext *)JS_GetContextPrivate(cx);
  nsCOMPtr<nsIScriptSecurityManager> secMan;
  if (NS_OK != scriptCX->GetSecurityManager(getter_AddRefs(secMan))) {
    return nsJSUtils::nsReportError(cx, NS_ERROR_DOM_SECMAN_ERR);
  }
  {
    PRBool ok;
    secMan->CheckScriptAccess(scriptCX, obj, NS_DOM_PROP_NODE_INSERTBEFORE, PR_FALSE, &ok);
    if (!ok) {
      return nsJSUtils::nsReportError(cx, NS_ERROR_DOM_SECURITY_ERR);
    }
  }

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == nativeThis) {
    return JS_TRUE;
  }

  {
    if (argc < 2) {
      return nsJSUtils::nsReportError(cx, NS_ERROR_DOM_TOO_FEW_PARAMETERS_ERR);
    }

    if (JS_FALSE == nsJSUtils::nsConvertJSValToObject((nsISupports **)&b0,
                                           kINodeIID,
                                           "Node",
                                           cx,
                                           argv[0])) {
      return nsJSUtils::nsReportError(cx, NS_ERROR_DOM_NOT_OBJECT_ERR);
    }
    if (JS_FALSE == nsJSUtils::nsConvertJSValToObject((nsISupports **)&b1,
                                           kINodeIID,
                                           "Node",
                                           cx,
                                           argv[1])) {
      return nsJSUtils::nsReportError(cx, NS_ERROR_DOM_NOT_OBJECT_ERR);
    }

    result = nativeThis->InsertBefore(b0, b1, &nativeRet);
    if (NS_FAILED(result)) {
      return nsJSUtils::nsReportError(cx, result);
    }

    nsJSUtils::nsConvertObjectToJSVal(nativeRet, cx, rval);
  }

  return JS_TRUE;
}


//
// Native method ReplaceChild
//
PR_STATIC_CALLBACK(JSBool)
NodeReplaceChild(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsIDOMNode *nativeThis = (nsIDOMNode*)nsJSUtils::nsGetNativeThis(cx, obj);
  nsresult result = NS_OK;
  nsIDOMNode* nativeRet;
  nsIDOMNodePtr b0;
  nsIDOMNodePtr b1;

  *rval = JSVAL_NULL;

  nsIScriptContext *scriptCX = (nsIScriptContext *)JS_GetContextPrivate(cx);
  nsCOMPtr<nsIScriptSecurityManager> secMan;
  if (NS_OK != scriptCX->GetSecurityManager(getter_AddRefs(secMan))) {
    return nsJSUtils::nsReportError(cx, NS_ERROR_DOM_SECMAN_ERR);
  }
  {
    PRBool ok;
    secMan->CheckScriptAccess(scriptCX, obj, NS_DOM_PROP_NODE_REPLACECHILD, PR_FALSE, &ok);
    if (!ok) {
      return nsJSUtils::nsReportError(cx, NS_ERROR_DOM_SECURITY_ERR);
    }
  }

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == nativeThis) {
    return JS_TRUE;
  }

  {
    if (argc < 2) {
      return nsJSUtils::nsReportError(cx, NS_ERROR_DOM_TOO_FEW_PARAMETERS_ERR);
    }

    if (JS_FALSE == nsJSUtils::nsConvertJSValToObject((nsISupports **)&b0,
                                           kINodeIID,
                                           "Node",
                                           cx,
                                           argv[0])) {
      return nsJSUtils::nsReportError(cx, NS_ERROR_DOM_NOT_OBJECT_ERR);
    }
    if (JS_FALSE == nsJSUtils::nsConvertJSValToObject((nsISupports **)&b1,
                                           kINodeIID,
                                           "Node",
                                           cx,
                                           argv[1])) {
      return nsJSUtils::nsReportError(cx, NS_ERROR_DOM_NOT_OBJECT_ERR);
    }

    result = nativeThis->ReplaceChild(b0, b1, &nativeRet);
    if (NS_FAILED(result)) {
      return nsJSUtils::nsReportError(cx, result);
    }

    nsJSUtils::nsConvertObjectToJSVal(nativeRet, cx, rval);
  }

  return JS_TRUE;
}


//
// Native method RemoveChild
//
PR_STATIC_CALLBACK(JSBool)
NodeRemoveChild(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsIDOMNode *nativeThis = (nsIDOMNode*)nsJSUtils::nsGetNativeThis(cx, obj);
  nsresult result = NS_OK;
  nsIDOMNode* nativeRet;
  nsIDOMNodePtr b0;

  *rval = JSVAL_NULL;

  nsIScriptContext *scriptCX = (nsIScriptContext *)JS_GetContextPrivate(cx);
  nsCOMPtr<nsIScriptSecurityManager> secMan;
  if (NS_OK != scriptCX->GetSecurityManager(getter_AddRefs(secMan))) {
    return nsJSUtils::nsReportError(cx, NS_ERROR_DOM_SECMAN_ERR);
  }
  {
    PRBool ok;
    secMan->CheckScriptAccess(scriptCX, obj, NS_DOM_PROP_NODE_REMOVECHILD, PR_FALSE, &ok);
    if (!ok) {
      return nsJSUtils::nsReportError(cx, NS_ERROR_DOM_SECURITY_ERR);
    }
  }

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == nativeThis) {
    return JS_TRUE;
  }

  {
    if (argc < 1) {
      return nsJSUtils::nsReportError(cx, NS_ERROR_DOM_TOO_FEW_PARAMETERS_ERR);
    }

    if (JS_FALSE == nsJSUtils::nsConvertJSValToObject((nsISupports **)&b0,
                                           kINodeIID,
                                           "Node",
                                           cx,
                                           argv[0])) {
      return nsJSUtils::nsReportError(cx, NS_ERROR_DOM_NOT_OBJECT_ERR);
    }

    result = nativeThis->RemoveChild(b0, &nativeRet);
    if (NS_FAILED(result)) {
      return nsJSUtils::nsReportError(cx, result);
    }

    nsJSUtils::nsConvertObjectToJSVal(nativeRet, cx, rval);
  }

  return JS_TRUE;
}


//
// Native method AppendChild
//
PR_STATIC_CALLBACK(JSBool)
NodeAppendChild(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsIDOMNode *nativeThis = (nsIDOMNode*)nsJSUtils::nsGetNativeThis(cx, obj);
  nsresult result = NS_OK;
  nsIDOMNode* nativeRet;
  nsIDOMNodePtr b0;

  *rval = JSVAL_NULL;

  nsIScriptContext *scriptCX = (nsIScriptContext *)JS_GetContextPrivate(cx);
  nsCOMPtr<nsIScriptSecurityManager> secMan;
  if (NS_OK != scriptCX->GetSecurityManager(getter_AddRefs(secMan))) {
    return nsJSUtils::nsReportError(cx, NS_ERROR_DOM_SECMAN_ERR);
  }
  {
    PRBool ok;
    secMan->CheckScriptAccess(scriptCX, obj, NS_DOM_PROP_NODE_APPENDCHILD, PR_FALSE, &ok);
    if (!ok) {
      return nsJSUtils::nsReportError(cx, NS_ERROR_DOM_SECURITY_ERR);
    }
  }

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == nativeThis) {
    return JS_TRUE;
  }

  {
    if (argc < 1) {
      return nsJSUtils::nsReportError(cx, NS_ERROR_DOM_TOO_FEW_PARAMETERS_ERR);
    }

    if (JS_FALSE == nsJSUtils::nsConvertJSValToObject((nsISupports **)&b0,
                                           kINodeIID,
                                           "Node",
                                           cx,
                                           argv[0])) {
      return nsJSUtils::nsReportError(cx, NS_ERROR_DOM_NOT_OBJECT_ERR);
    }

    result = nativeThis->AppendChild(b0, &nativeRet);
    if (NS_FAILED(result)) {
      return nsJSUtils::nsReportError(cx, result);
    }

    nsJSUtils::nsConvertObjectToJSVal(nativeRet, cx, rval);
  }

  return JS_TRUE;
}


//
// Native method HasChildNodes
//
PR_STATIC_CALLBACK(JSBool)
NodeHasChildNodes(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsIDOMNode *nativeThis = (nsIDOMNode*)nsJSUtils::nsGetNativeThis(cx, obj);
  nsresult result = NS_OK;
  PRBool nativeRet;

  *rval = JSVAL_NULL;

  nsIScriptContext *scriptCX = (nsIScriptContext *)JS_GetContextPrivate(cx);
  nsCOMPtr<nsIScriptSecurityManager> secMan;
  if (NS_OK != scriptCX->GetSecurityManager(getter_AddRefs(secMan))) {
    return nsJSUtils::nsReportError(cx, NS_ERROR_DOM_SECMAN_ERR);
  }
  {
    PRBool ok;
    secMan->CheckScriptAccess(scriptCX, obj, NS_DOM_PROP_NODE_HASCHILDNODES, PR_FALSE, &ok);
    if (!ok) {
      return nsJSUtils::nsReportError(cx, NS_ERROR_DOM_SECURITY_ERR);
    }
  }

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == nativeThis) {
    return JS_TRUE;
  }

  {

    result = nativeThis->HasChildNodes(&nativeRet);
    if (NS_FAILED(result)) {
      return nsJSUtils::nsReportError(cx, result);
    }

    *rval = BOOLEAN_TO_JSVAL(nativeRet);
  }

  return JS_TRUE;
}


//
// Native method CloneNode
//
PR_STATIC_CALLBACK(JSBool)
NodeCloneNode(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsIDOMNode *nativeThis = (nsIDOMNode*)nsJSUtils::nsGetNativeThis(cx, obj);
  nsresult result = NS_OK;
  nsIDOMNode* nativeRet;
  PRBool b0;

  *rval = JSVAL_NULL;

  nsIScriptContext *scriptCX = (nsIScriptContext *)JS_GetContextPrivate(cx);
  nsCOMPtr<nsIScriptSecurityManager> secMan;
  if (NS_OK != scriptCX->GetSecurityManager(getter_AddRefs(secMan))) {
    return nsJSUtils::nsReportError(cx, NS_ERROR_DOM_SECMAN_ERR);
  }
  {
    PRBool ok;
    secMan->CheckScriptAccess(scriptCX, obj, NS_DOM_PROP_NODE_CLONENODE, PR_FALSE, &ok);
    if (!ok) {
      return nsJSUtils::nsReportError(cx, NS_ERROR_DOM_SECURITY_ERR);
    }
  }

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == nativeThis) {
    return JS_TRUE;
  }

  {
    if (argc < 1) {
      return nsJSUtils::nsReportError(cx, NS_ERROR_DOM_TOO_FEW_PARAMETERS_ERR);
    }

    if (!nsJSUtils::nsConvertJSValToBool(&b0, cx, argv[0])) {
      return nsJSUtils::nsReportError(cx, NS_ERROR_DOM_NOT_BOOLEAN_ERR);
    }

    result = nativeThis->CloneNode(b0, &nativeRet);
    if (NS_FAILED(result)) {
      return nsJSUtils::nsReportError(cx, result);
    }

    nsJSUtils::nsConvertObjectToJSVal(nativeRet, cx, rval);
  }

  return JS_TRUE;
}


//
// Native method AddEventListener
//
PR_STATIC_CALLBACK(JSBool)
EventTargetAddEventListener(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsIDOMNodePtr nativeThis = nsnull;
  nsresult result = NS_OK;
  if (NS_OK != privateThis->QueryInterface(kINodeIID, (void **)&nativeThis)) {
  nsresult result = NS_OK;
  if (NS_OK != privateThis->QueryInterface(kIEventTargetIID, (void **)&nativeThis)) {
    return nsJSUtils::nsReportError(cx, NS_ERROR_DOM_WRONG_TYPE_ERR);
  }

  nsAutoString b0;
  nsIDOMEventListenerPtr b1;
  PRBool b2;

  *rval = JSVAL_NULL;

  nsIScriptContext *scriptCX = (nsIScriptContext *)JS_GetContextPrivate(cx);
  nsCOMPtr<nsIScriptSecurityManager> secMan;
  if (NS_OK != scriptCX->GetSecurityManager(getter_AddRefs(secMan))) {
    return nsJSUtils::nsReportError(cx, NS_ERROR_DOM_SECMAN_ERR);
  }
  {
    PRBool ok;
    secMan->CheckScriptAccess(scriptCX, obj, NS_DOM_PROP_EVENTTARGET_ADDEVENTLISTENER, PR_FALSE, &ok);
    if (!ok) {
      return nsJSUtils::nsReportError(cx, NS_ERROR_DOM_SECURITY_ERR);
    }
  }

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == nativeThis) {
    return JS_TRUE;
  }

  {
    if (argc < 3) {
      return nsJSUtils::nsReportError(cx, NS_ERROR_DOM_TOO_FEW_PARAMETERS_ERR);
    }

    nsJSUtils::nsConvertJSValToString(b0, cx, argv[0]);
    if (!nsJSUtils::nsConvertJSValToFunc((nsIDOMEventListener**)(nsISupports**) &b1,
                                         cx,
                                         obj,
                                         argv[1])) {
      return nsJSUtils::nsReportError(cx, NS_ERROR_DOM_NOT_FUNCTION_ERR);
    }
    if (!nsJSUtils::nsConvertJSValToBool(&b2, cx, argv[2])) {
      return nsJSUtils::nsReportError(cx, NS_ERROR_DOM_NOT_BOOLEAN_ERR);
    }

    result = nativeThis->AddEventListener(b0, b1, b2);
    if (NS_FAILED(result)) {
      return nsJSUtils::nsReportError(cx, result);
    }

    *rval = JSVAL_VOID;
  }

  return JS_TRUE;
}


//
// Native method RemoveEventListener
//
PR_STATIC_CALLBACK(JSBool)
EventTargetRemoveEventListener(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsIDOMNodePtr nativeThis = nsnull;
  nsresult result = NS_OK;
  if (NS_OK != privateThis->QueryInterface(kINodeIID, (void **)&nativeThis)) {
  nsresult result = NS_OK;
  if (NS_OK != privateThis->QueryInterface(kIEventTargetIID, (void **)&nativeThis)) {
    return nsJSUtils::nsReportError(cx, NS_ERROR_DOM_WRONG_TYPE_ERR);
  }

  nsAutoString b0;
  nsIDOMEventListenerPtr b1;
  PRBool b2;

  *rval = JSVAL_NULL;

  nsIScriptContext *scriptCX = (nsIScriptContext *)JS_GetContextPrivate(cx);
  nsCOMPtr<nsIScriptSecurityManager> secMan;
  if (NS_OK != scriptCX->GetSecurityManager(getter_AddRefs(secMan))) {
    return nsJSUtils::nsReportError(cx, NS_ERROR_DOM_SECMAN_ERR);
  }
  {
    PRBool ok;
    secMan->CheckScriptAccess(scriptCX, obj, NS_DOM_PROP_EVENTTARGET_REMOVEEVENTLISTENER, PR_FALSE, &ok);
    if (!ok) {
      return nsJSUtils::nsReportError(cx, NS_ERROR_DOM_SECURITY_ERR);
    }
  }

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == nativeThis) {
    return JS_TRUE;
  }

  {
    if (argc < 3) {
      return nsJSUtils::nsReportError(cx, NS_ERROR_DOM_TOO_FEW_PARAMETERS_ERR);
    }

    nsJSUtils::nsConvertJSValToString(b0, cx, argv[0]);
    if (!nsJSUtils::nsConvertJSValToFunc((nsIDOMEventListener**)(nsISupports**) &b1,
                                         cx,
                                         obj,
                                         argv[1])) {
      return nsJSUtils::nsReportError(cx, NS_ERROR_DOM_NOT_FUNCTION_ERR);
    }
    if (!nsJSUtils::nsConvertJSValToBool(&b2, cx, argv[2])) {
      return nsJSUtils::nsReportError(cx, NS_ERROR_DOM_NOT_BOOLEAN_ERR);
    }

    result = nativeThis->RemoveEventListener(b0, b1, b2);
    if (NS_FAILED(result)) {
      return nsJSUtils::nsReportError(cx, result);
    }

    *rval = JSVAL_VOID;
  }

  return JS_TRUE;
}


/***********************************************************************/
//
// class for Node
//
JSClass NodeClass = {
  "Node", 
  JSCLASS_HAS_PRIVATE | JSCLASS_PRIVATE_IS_NSISUPPORTS,
  JS_PropertyStub,
  JS_PropertyStub,
  GetNodeProperty,
  SetNodeProperty,
  EnumerateNode,
  ResolveNode,
  JS_ConvertStub,
  FinalizeNode
};


//
// Node class properties
//
static JSPropertySpec NodeProperties[] =
{
  {"nodeName",    NODE_NODENAME,    JSPROP_ENUMERATE | JSPROP_READONLY},
  {"nodeValue",    NODE_NODEVALUE,    JSPROP_ENUMERATE},
  {"nodeType",    NODE_NODETYPE,    JSPROP_ENUMERATE | JSPROP_READONLY},
  {"parentNode",    NODE_PARENTNODE,    JSPROP_ENUMERATE | JSPROP_READONLY},
  {"childNodes",    NODE_CHILDNODES,    JSPROP_ENUMERATE | JSPROP_READONLY},
  {"firstChild",    NODE_FIRSTCHILD,    JSPROP_ENUMERATE | JSPROP_READONLY},
  {"lastChild",    NODE_LASTCHILD,    JSPROP_ENUMERATE | JSPROP_READONLY},
  {"previousSibling",    NODE_PREVIOUSSIBLING,    JSPROP_ENUMERATE | JSPROP_READONLY},
  {"nextSibling",    NODE_NEXTSIBLING,    JSPROP_ENUMERATE | JSPROP_READONLY},
  {"attributes",    NODE_ATTRIBUTES,    JSPROP_ENUMERATE | JSPROP_READONLY},
  {"ownerDocument",    NODE_OWNERDOCUMENT,    JSPROP_ENUMERATE | JSPROP_READONLY},
  {0}
};


//
// Node class methods
//
static JSFunctionSpec NodeMethods[] = 
{
  {"insertBefore",          NodeInsertBefore,     2},
  {"replaceChild",          NodeReplaceChild,     2},
  {"removeChild",          NodeRemoveChild,     1},
  {"appendChild",          NodeAppendChild,     1},
  {"hasChildNodes",          NodeHasChildNodes,     0},
  {"cloneNode",          NodeCloneNode,     1},
  {"addEventListener",          EventTargetAddEventListener,     3},
  {"removeEventListener",          EventTargetRemoveEventListener,     3},
  {0}
};


//
// Node constructor
//
PR_STATIC_CALLBACK(JSBool)
Node(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  return JS_FALSE;
}


//
// Node class initialization
//
extern "C" NS_DOM nsresult NS_InitNodeClass(nsIScriptContext *aContext, void **aPrototype)
{
  JSContext *jscontext = (JSContext *)aContext->GetNativeContext();
  JSObject *proto = nsnull;
  JSObject *constructor = nsnull;
  JSObject *parent_proto = nsnull;
  JSObject *global = JS_GetGlobalObject(jscontext);
  jsval vp;

  if ((PR_TRUE != JS_LookupProperty(jscontext, global, "Node", &vp)) ||
      !JSVAL_IS_OBJECT(vp) ||
      ((constructor = JSVAL_TO_OBJECT(vp)) == nsnull) ||
      (PR_TRUE != JS_LookupProperty(jscontext, JSVAL_TO_OBJECT(vp), "prototype", &vp)) || 
      !JSVAL_IS_OBJECT(vp)) {

    proto = JS_InitClass(jscontext,     // context
                         global,        // global object
                         parent_proto,  // parent proto 
                         &NodeClass,      // JSClass
                         Node,            // JSNative ctor
                         0,             // ctor args
                         NodeProperties,  // proto props
                         NodeMethods,     // proto funcs
                         nsnull,        // ctor props (static)
                         nsnull);       // ctor funcs (static)
    if (nsnull == proto) {
      return NS_ERROR_FAILURE;
    }

    if ((PR_TRUE == JS_LookupProperty(jscontext, global, "Node", &vp)) &&
        JSVAL_IS_OBJECT(vp) &&
        ((constructor = JSVAL_TO_OBJECT(vp)) != nsnull)) {
      vp = INT_TO_JSVAL(nsIDOMNode::ELEMENT_NODE);
      JS_SetProperty(jscontext, constructor, "ELEMENT_NODE", &vp);

      vp = INT_TO_JSVAL(nsIDOMNode::ATTRIBUTE_NODE);
      JS_SetProperty(jscontext, constructor, "ATTRIBUTE_NODE", &vp);

      vp = INT_TO_JSVAL(nsIDOMNode::TEXT_NODE);
      JS_SetProperty(jscontext, constructor, "TEXT_NODE", &vp);

      vp = INT_TO_JSVAL(nsIDOMNode::CDATA_SECTION_NODE);
      JS_SetProperty(jscontext, constructor, "CDATA_SECTION_NODE", &vp);

      vp = INT_TO_JSVAL(nsIDOMNode::ENTITY_REFERENCE_NODE);
      JS_SetProperty(jscontext, constructor, "ENTITY_REFERENCE_NODE", &vp);

      vp = INT_TO_JSVAL(nsIDOMNode::ENTITY_NODE);
      JS_SetProperty(jscontext, constructor, "ENTITY_NODE", &vp);

      vp = INT_TO_JSVAL(nsIDOMNode::PROCESSING_INSTRUCTION_NODE);
      JS_SetProperty(jscontext, constructor, "PROCESSING_INSTRUCTION_NODE", &vp);

      vp = INT_TO_JSVAL(nsIDOMNode::COMMENT_NODE);
      JS_SetProperty(jscontext, constructor, "COMMENT_NODE", &vp);

      vp = INT_TO_JSVAL(nsIDOMNode::DOCUMENT_NODE);
      JS_SetProperty(jscontext, constructor, "DOCUMENT_NODE", &vp);

      vp = INT_TO_JSVAL(nsIDOMNode::DOCUMENT_TYPE_NODE);
      JS_SetProperty(jscontext, constructor, "DOCUMENT_TYPE_NODE", &vp);

      vp = INT_TO_JSVAL(nsIDOMNode::DOCUMENT_FRAGMENT_NODE);
      JS_SetProperty(jscontext, constructor, "DOCUMENT_FRAGMENT_NODE", &vp);

      vp = INT_TO_JSVAL(nsIDOMNode::NOTATION_NODE);
      JS_SetProperty(jscontext, constructor, "NOTATION_NODE", &vp);

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
// Method for creating a new Node JavaScript object
//
extern "C" NS_DOM nsresult NS_NewScriptNode(nsIScriptContext *aContext, nsISupports *aSupports, nsISupports *aParent, void **aReturn)
{
  NS_PRECONDITION(nsnull != aContext && nsnull != aSupports && nsnull != aReturn, "null argument to NS_NewScriptNode");
  JSObject *proto;
  JSObject *parent;
  nsIScriptObjectOwner *owner;
  JSContext *jscontext = (JSContext *)aContext->GetNativeContext();
  nsresult result = NS_OK;
  nsIDOMNode *aNode;

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

  if (NS_OK != NS_InitNodeClass(aContext, (void **)&proto)) {
    return NS_ERROR_FAILURE;
  }

  result = aSupports->QueryInterface(kINodeIID, (void **)&aNode);
  if (NS_OK != result) {
    return result;
  }

  // create a js object for this class
  *aReturn = JS_NewObject(jscontext, &NodeClass, proto, parent);
  if (nsnull != *aReturn) {
    // connect the native object to the js object
    JS_SetPrivate(jscontext, (JSObject *)*aReturn, aNode);
  }
  else {
    NS_RELEASE(aNode);
    return NS_ERROR_FAILURE; 
  }

  return NS_OK;
}
