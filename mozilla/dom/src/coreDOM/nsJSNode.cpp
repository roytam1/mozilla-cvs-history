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
#include "nsIDOMNamedNodeMap.h"
#include "nsIDOMNode.h"
#include "nsIDOMNodeList.h"


static NS_DEFINE_IID(kIScriptObjectOwnerIID, NS_ISCRIPTOBJECTOWNER_IID);
static NS_DEFINE_IID(kIJSScriptObjectIID, NS_IJSSCRIPTOBJECT_IID);
static NS_DEFINE_IID(kIScriptGlobalObjectIID, NS_ISCRIPTGLOBALOBJECT_IID);
static NS_DEFINE_IID(kINamedNodeMapIID, NS_IDOMNAMEDNODEMAP_IID);
static NS_DEFINE_IID(kINodeIID, NS_IDOMNODE_IID);
static NS_DEFINE_IID(kINodeListIID, NS_IDOMNODELIST_IID);

NS_DEF_PTR(nsIDOMNamedNodeMap);
NS_DEF_PTR(nsIDOMNode);
NS_DEF_PTR(nsIDOMNodeList);

//
// Node property ids
//
enum Node_slots {
  NODE_NODENAME = -11,
  NODE_NODEVALUE = -12,
  NODE_NODETYPE = -13,
  NODE_PARENTNODE = -14,
  NODE_CHILDNODES = -15,
  NODE_HASCHILDNODES = -16,
  NODE_FIRSTCHILD = -17,
  NODE_LASTCHILD = -18,
  NODE_PREVIOUSSIBLING = -19,
  NODE_NEXTSIBLING = -110,
  NODE_ATTRIBUTES = -111
};

/***********************************************************************/
//
// Node Properties Getter
//
PR_STATIC_CALLBACK(JSBool)
GetNodeProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  nsIDOMNode *a = (nsIDOMNode*)JS_GetPrivate(cx, obj);

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == a) {
    return JS_TRUE;
  }

  if (JSVAL_IS_INT(id)) {
    switch(JSVAL_TO_INT(id)) {
      case NODE_NODENAME:
      {
        nsAutoString prop;
        if (NS_OK == a->GetNodeName(prop)) {
          JSString *jsstring = JS_NewUCStringCopyN(cx, prop, prop.Length());
          // set the return value
          *vp = STRING_TO_JSVAL(jsstring);
        }
        else {
          return JS_FALSE;
        }
        break;
      }
      case NODE_NODEVALUE:
      {
        nsAutoString prop;
        if (NS_OK == a->GetNodeValue(prop)) {
          JSString *jsstring = JS_NewUCStringCopyN(cx, prop, prop.Length());
          // set the return value
          *vp = STRING_TO_JSVAL(jsstring);
        }
        else {
          return JS_FALSE;
        }
        break;
      }
      case NODE_NODETYPE:
      {
        PRInt32 prop;
        if (NS_OK == a->GetNodeType(&prop)) {
          *vp = INT_TO_JSVAL(prop);
        }
        else {
          return JS_FALSE;
        }
        break;
      }
      case NODE_PARENTNODE:
      {
        nsIDOMNode* prop;
        if (NS_OK == a->GetParentNode(&prop)) {
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
      case NODE_CHILDNODES:
      {
        nsIDOMNodeList* prop;
        if (NS_OK == a->GetChildNodes(&prop)) {
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
      case NODE_HASCHILDNODES:
      {
        PRBool prop;
        if (NS_OK == a->GetHasChildNodes(&prop)) {
          *vp = BOOLEAN_TO_JSVAL(prop);
        }
        else {
          return JS_FALSE;
        }
        break;
      }
      case NODE_FIRSTCHILD:
      {
        nsIDOMNode* prop;
        if (NS_OK == a->GetFirstChild(&prop)) {
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
      case NODE_LASTCHILD:
      {
        nsIDOMNode* prop;
        if (NS_OK == a->GetLastChild(&prop)) {
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
      case NODE_PREVIOUSSIBLING:
      {
        nsIDOMNode* prop;
        if (NS_OK == a->GetPreviousSibling(&prop)) {
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
      case NODE_NEXTSIBLING:
      {
        nsIDOMNode* prop;
        if (NS_OK == a->GetNextSibling(&prop)) {
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
      case NODE_ATTRIBUTES:
      {
        nsIDOMNamedNodeMap* prop;
        if (NS_OK == a->GetAttributes(&prop)) {
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
// Node Properties Setter
//
PR_STATIC_CALLBACK(JSBool)
SetNodeProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  nsIDOMNode *a = (nsIDOMNode*)JS_GetPrivate(cx, obj);

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == a) {
    return JS_TRUE;
  }

  if (JSVAL_IS_INT(id)) {
    switch(JSVAL_TO_INT(id)) {
      case NODE_NODEVALUE:
      {
        nsAutoString prop;
        JSString *jsstring;
        if ((jsstring = JS_ValueToString(cx, *vp)) != nsnull) {
          prop.SetString(JS_GetStringChars(jsstring));
        }
        else {
          prop.SetString((const char *)nsnull);
        }
      
        a->SetNodeValue(prop);
        
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
// Node finalizer
//
PR_STATIC_CALLBACK(void)
FinalizeNode(JSContext *cx, JSObject *obj)
{
  nsIDOMNode *a = (nsIDOMNode*)JS_GetPrivate(cx, obj);
  
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
// Node enumerate
//
PR_STATIC_CALLBACK(JSBool)
EnumerateNode(JSContext *cx, JSObject *obj)
{
  nsIDOMNode *a = (nsIDOMNode*)JS_GetPrivate(cx, obj);
  
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
// Node resolve
//
PR_STATIC_CALLBACK(JSBool)
ResolveNode(JSContext *cx, JSObject *obj, jsval id)
{
  nsIDOMNode *a = (nsIDOMNode*)JS_GetPrivate(cx, obj);
  
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
// Native method InsertBefore
//
PR_STATIC_CALLBACK(JSBool)
NodeInsertBefore(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsIDOMNode *nativeThis = (nsIDOMNode*)JS_GetPrivate(cx, obj);
  JSBool rBool = JS_FALSE;
  nsIDOMNode* nativeRet;
  nsIDOMNodePtr b0;
  nsIDOMNodePtr b1;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == nativeThis) {
    return JS_TRUE;
  }

  if (argc >= 2) {

    if (JSVAL_IS_NULL(argv[0])){
      b0 = nsnull;
    }
    else if (JSVAL_IS_OBJECT(argv[0])) {
      nsISupports *supports0 = (nsISupports *)JS_GetPrivate(cx, JSVAL_TO_OBJECT(argv[0]));
      NS_ASSERTION(nsnull != supports0, "null pointer");

      if ((nsnull == supports0) ||
          (NS_OK != supports0->QueryInterface(kINodeIID, (void **)(b0.Query())))) {
        JS_ReportError(cx, "Parameter must be of type Node");
        return JS_FALSE;
      }
    }
    else {
      JS_ReportError(cx, "Parameter must be an object");
      return JS_FALSE;
    }

    if (JSVAL_IS_NULL(argv[1])){
      b1 = nsnull;
    }
    else if (JSVAL_IS_OBJECT(argv[1])) {
      nsISupports *supports1 = (nsISupports *)JS_GetPrivate(cx, JSVAL_TO_OBJECT(argv[1]));
      NS_ASSERTION(nsnull != supports1, "null pointer");

      if ((nsnull == supports1) ||
          (NS_OK != supports1->QueryInterface(kINodeIID, (void **)(b1.Query())))) {
        JS_ReportError(cx, "Parameter must be of type Node");
        return JS_FALSE;
      }
    }
    else {
      JS_ReportError(cx, "Parameter must be an object");
      return JS_FALSE;
    }

    if (NS_OK != nativeThis->InsertBefore(b0, b1, &nativeRet)) {
      return JS_FALSE;
    }

    if (nativeRet != nsnull) {
      nsIScriptObjectOwner *owner = nsnull;
      if (NS_OK == nativeRet->QueryInterface(kIScriptObjectOwnerIID, (void**)&owner)) {
        JSObject *object = nsnull;
        nsIScriptContext *script_cx = (nsIScriptContext *)JS_GetContextPrivate(cx);
        if (NS_OK == owner->GetScriptObject(script_cx, (void**)&object)) {
          // set the return value
          *rval = OBJECT_TO_JSVAL(object);
        }
        NS_RELEASE(owner);
      }
      NS_RELEASE(nativeRet);
    }
    else {
      *rval = JSVAL_NULL;
    }
  }
  else {
    JS_ReportError(cx, "Function insertBefore requires 2 parameters");
    return JS_FALSE;
  }

  return JS_TRUE;
}


//
// Native method ReplaceChild
//
PR_STATIC_CALLBACK(JSBool)
NodeReplaceChild(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsIDOMNode *nativeThis = (nsIDOMNode*)JS_GetPrivate(cx, obj);
  JSBool rBool = JS_FALSE;
  nsIDOMNode* nativeRet;
  nsIDOMNodePtr b0;
  nsIDOMNodePtr b1;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == nativeThis) {
    return JS_TRUE;
  }

  if (argc >= 2) {

    if (JSVAL_IS_NULL(argv[0])){
      b0 = nsnull;
    }
    else if (JSVAL_IS_OBJECT(argv[0])) {
      nsISupports *supports0 = (nsISupports *)JS_GetPrivate(cx, JSVAL_TO_OBJECT(argv[0]));
      NS_ASSERTION(nsnull != supports0, "null pointer");

      if ((nsnull == supports0) ||
          (NS_OK != supports0->QueryInterface(kINodeIID, (void **)(b0.Query())))) {
        JS_ReportError(cx, "Parameter must be of type Node");
        return JS_FALSE;
      }
    }
    else {
      JS_ReportError(cx, "Parameter must be an object");
      return JS_FALSE;
    }

    if (JSVAL_IS_NULL(argv[1])){
      b1 = nsnull;
    }
    else if (JSVAL_IS_OBJECT(argv[1])) {
      nsISupports *supports1 = (nsISupports *)JS_GetPrivate(cx, JSVAL_TO_OBJECT(argv[1]));
      NS_ASSERTION(nsnull != supports1, "null pointer");

      if ((nsnull == supports1) ||
          (NS_OK != supports1->QueryInterface(kINodeIID, (void **)(b1.Query())))) {
        JS_ReportError(cx, "Parameter must be of type Node");
        return JS_FALSE;
      }
    }
    else {
      JS_ReportError(cx, "Parameter must be an object");
      return JS_FALSE;
    }

    if (NS_OK != nativeThis->ReplaceChild(b0, b1, &nativeRet)) {
      return JS_FALSE;
    }

    if (nativeRet != nsnull) {
      nsIScriptObjectOwner *owner = nsnull;
      if (NS_OK == nativeRet->QueryInterface(kIScriptObjectOwnerIID, (void**)&owner)) {
        JSObject *object = nsnull;
        nsIScriptContext *script_cx = (nsIScriptContext *)JS_GetContextPrivate(cx);
        if (NS_OK == owner->GetScriptObject(script_cx, (void**)&object)) {
          // set the return value
          *rval = OBJECT_TO_JSVAL(object);
        }
        NS_RELEASE(owner);
      }
      NS_RELEASE(nativeRet);
    }
    else {
      *rval = JSVAL_NULL;
    }
  }
  else {
    JS_ReportError(cx, "Function replaceChild requires 2 parameters");
    return JS_FALSE;
  }

  return JS_TRUE;
}


//
// Native method RemoveChild
//
PR_STATIC_CALLBACK(JSBool)
NodeRemoveChild(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsIDOMNode *nativeThis = (nsIDOMNode*)JS_GetPrivate(cx, obj);
  JSBool rBool = JS_FALSE;
  nsIDOMNode* nativeRet;
  nsIDOMNodePtr b0;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == nativeThis) {
    return JS_TRUE;
  }

  if (argc >= 1) {

    if (JSVAL_IS_NULL(argv[0])){
      b0 = nsnull;
    }
    else if (JSVAL_IS_OBJECT(argv[0])) {
      nsISupports *supports0 = (nsISupports *)JS_GetPrivate(cx, JSVAL_TO_OBJECT(argv[0]));
      NS_ASSERTION(nsnull != supports0, "null pointer");

      if ((nsnull == supports0) ||
          (NS_OK != supports0->QueryInterface(kINodeIID, (void **)(b0.Query())))) {
        JS_ReportError(cx, "Parameter must be of type Node");
        return JS_FALSE;
      }
    }
    else {
      JS_ReportError(cx, "Parameter must be an object");
      return JS_FALSE;
    }

    if (NS_OK != nativeThis->RemoveChild(b0, &nativeRet)) {
      return JS_FALSE;
    }

    if (nativeRet != nsnull) {
      nsIScriptObjectOwner *owner = nsnull;
      if (NS_OK == nativeRet->QueryInterface(kIScriptObjectOwnerIID, (void**)&owner)) {
        JSObject *object = nsnull;
        nsIScriptContext *script_cx = (nsIScriptContext *)JS_GetContextPrivate(cx);
        if (NS_OK == owner->GetScriptObject(script_cx, (void**)&object)) {
          // set the return value
          *rval = OBJECT_TO_JSVAL(object);
        }
        NS_RELEASE(owner);
      }
      NS_RELEASE(nativeRet);
    }
    else {
      *rval = JSVAL_NULL;
    }
  }
  else {
    JS_ReportError(cx, "Function removeChild requires 1 parameters");
    return JS_FALSE;
  }

  return JS_TRUE;
}


//
// Native method AppendChild
//
PR_STATIC_CALLBACK(JSBool)
NodeAppendChild(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsIDOMNode *nativeThis = (nsIDOMNode*)JS_GetPrivate(cx, obj);
  JSBool rBool = JS_FALSE;
  nsIDOMNode* nativeRet;
  nsIDOMNodePtr b0;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == nativeThis) {
    return JS_TRUE;
  }

  if (argc >= 1) {

    if (JSVAL_IS_NULL(argv[0])){
      b0 = nsnull;
    }
    else if (JSVAL_IS_OBJECT(argv[0])) {
      nsISupports *supports0 = (nsISupports *)JS_GetPrivate(cx, JSVAL_TO_OBJECT(argv[0]));
      NS_ASSERTION(nsnull != supports0, "null pointer");

      if ((nsnull == supports0) ||
          (NS_OK != supports0->QueryInterface(kINodeIID, (void **)(b0.Query())))) {
        JS_ReportError(cx, "Parameter must be of type Node");
        return JS_FALSE;
      }
    }
    else {
      JS_ReportError(cx, "Parameter must be an object");
      return JS_FALSE;
    }

    if (NS_OK != nativeThis->AppendChild(b0, &nativeRet)) {
      return JS_FALSE;
    }

    if (nativeRet != nsnull) {
      nsIScriptObjectOwner *owner = nsnull;
      if (NS_OK == nativeRet->QueryInterface(kIScriptObjectOwnerIID, (void**)&owner)) {
        JSObject *object = nsnull;
        nsIScriptContext *script_cx = (nsIScriptContext *)JS_GetContextPrivate(cx);
        if (NS_OK == owner->GetScriptObject(script_cx, (void**)&object)) {
          // set the return value
          *rval = OBJECT_TO_JSVAL(object);
        }
        NS_RELEASE(owner);
      }
      NS_RELEASE(nativeRet);
    }
    else {
      *rval = JSVAL_NULL;
    }
  }
  else {
    JS_ReportError(cx, "Function appendChild requires 1 parameters");
    return JS_FALSE;
  }

  return JS_TRUE;
}


//
// Native method CloneNode
//
PR_STATIC_CALLBACK(JSBool)
NodeCloneNode(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsIDOMNode *nativeThis = (nsIDOMNode*)JS_GetPrivate(cx, obj);
  JSBool rBool = JS_FALSE;
  nsIDOMNode* nativeRet;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == nativeThis) {
    return JS_TRUE;
  }

  if (argc >= 0) {

    if (NS_OK != nativeThis->CloneNode(&nativeRet)) {
      return JS_FALSE;
    }

    if (nativeRet != nsnull) {
      nsIScriptObjectOwner *owner = nsnull;
      if (NS_OK == nativeRet->QueryInterface(kIScriptObjectOwnerIID, (void**)&owner)) {
        JSObject *object = nsnull;
        nsIScriptContext *script_cx = (nsIScriptContext *)JS_GetContextPrivate(cx);
        if (NS_OK == owner->GetScriptObject(script_cx, (void**)&object)) {
          // set the return value
          *rval = OBJECT_TO_JSVAL(object);
        }
        NS_RELEASE(owner);
      }
      NS_RELEASE(nativeRet);
    }
    else {
      *rval = JSVAL_NULL;
    }
  }
  else {
    JS_ReportError(cx, "Function cloneNode requires 0 parameters");
    return JS_FALSE;
  }

  return JS_TRUE;
}


//
// Native method Equals
//
PR_STATIC_CALLBACK(JSBool)
NodeEquals(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsIDOMNode *nativeThis = (nsIDOMNode*)JS_GetPrivate(cx, obj);
  JSBool rBool = JS_FALSE;
  PRBool nativeRet;
  nsIDOMNodePtr b0;
  PRBool b1;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == nativeThis) {
    return JS_TRUE;
  }

  if (argc >= 2) {

    if (JSVAL_IS_NULL(argv[0])){
      b0 = nsnull;
    }
    else if (JSVAL_IS_OBJECT(argv[0])) {
      nsISupports *supports0 = (nsISupports *)JS_GetPrivate(cx, JSVAL_TO_OBJECT(argv[0]));
      NS_ASSERTION(nsnull != supports0, "null pointer");

      if ((nsnull == supports0) ||
          (NS_OK != supports0->QueryInterface(kINodeIID, (void **)(b0.Query())))) {
        JS_ReportError(cx, "Parameter must be of type Node");
        return JS_FALSE;
      }
    }
    else {
      JS_ReportError(cx, "Parameter must be an object");
      return JS_FALSE;
    }

    if (!JS_ValueToBoolean(cx, argv[1], &b1)) {
      JS_ReportError(cx, "Parameter must be a boolean");
      return JS_FALSE;
    }

    if (NS_OK != nativeThis->Equals(b0, b1, &nativeRet)) {
      return JS_FALSE;
    }

    *rval = BOOLEAN_TO_JSVAL(nativeRet);
  }
  else {
    JS_ReportError(cx, "Function equals requires 2 parameters");
    return JS_FALSE;
  }

  return JS_TRUE;
}


/***********************************************************************/
//
// class for Node
//
JSClass NodeClass = {
  "Node", 
  JSCLASS_HAS_PRIVATE,
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
  {"hasChildNodes",    NODE_HASCHILDNODES,    JSPROP_ENUMERATE | JSPROP_READONLY},
  {"firstChild",    NODE_FIRSTCHILD,    JSPROP_ENUMERATE | JSPROP_READONLY},
  {"lastChild",    NODE_LASTCHILD,    JSPROP_ENUMERATE | JSPROP_READONLY},
  {"previousSibling",    NODE_PREVIOUSSIBLING,    JSPROP_ENUMERATE | JSPROP_READONLY},
  {"nextSibling",    NODE_NEXTSIBLING,    JSPROP_ENUMERATE | JSPROP_READONLY},
  {"attributes",    NODE_ATTRIBUTES,    JSPROP_ENUMERATE | JSPROP_READONLY},
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
  {"cloneNode",          NodeCloneNode,     0},
  {"equals",          NodeEquals,     2},
  {0}
};


//
// Node constructor
//
PR_STATIC_CALLBACK(JSBool)
Node(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsIDOMNode *a = (nsIDOMNode*)JS_GetPrivate(cx, obj);
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
// Node class initialization
//
nsresult NS_InitNodeClass(nsIScriptContext *aContext, void **aPrototype)
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
      vp = INT_TO_JSVAL(nsIDOMNode::DOCUMENT);
      JS_SetProperty(jscontext, constructor, "DOCUMENT", &vp);

      vp = INT_TO_JSVAL(nsIDOMNode::ELEMENT);
      JS_SetProperty(jscontext, constructor, "ELEMENT", &vp);

      vp = INT_TO_JSVAL(nsIDOMNode::ATTRIBUTE);
      JS_SetProperty(jscontext, constructor, "ATTRIBUTE", &vp);

      vp = INT_TO_JSVAL(nsIDOMNode::PROCESSING_INSTRUCTION);
      JS_SetProperty(jscontext, constructor, "PROCESSING_INSTRUCTION", &vp);

      vp = INT_TO_JSVAL(nsIDOMNode::COMMENT);
      JS_SetProperty(jscontext, constructor, "COMMENT", &vp);

      vp = INT_TO_JSVAL(nsIDOMNode::TEXT);
      JS_SetProperty(jscontext, constructor, "TEXT", &vp);

      vp = INT_TO_JSVAL(nsIDOMNode::CDATA_SECTION);
      JS_SetProperty(jscontext, constructor, "CDATA_SECTION", &vp);

      vp = INT_TO_JSVAL(nsIDOMNode::DOCUMENT_FRAGMENT);
      JS_SetProperty(jscontext, constructor, "DOCUMENT_FRAGMENT", &vp);

      vp = INT_TO_JSVAL(nsIDOMNode::ENTITY_DECLARATION);
      JS_SetProperty(jscontext, constructor, "ENTITY_DECLARATION", &vp);

      vp = INT_TO_JSVAL(nsIDOMNode::ENTITY_REFERENCE);
      JS_SetProperty(jscontext, constructor, "ENTITY_REFERENCE", &vp);

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
