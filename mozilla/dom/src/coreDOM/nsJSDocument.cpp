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
#include "nsIDOMElement.h"
#include "nsIDOMDocument.h"
#include "nsIDOMProcessingInstruction.h"
#include "nsIDOMNamedNodeMap.h"
#include "nsIDOMAttribute.h"
#include "nsIDOMNode.h"
#include "nsIDOMText.h"
#include "nsIDOMDocumentType.h"
#include "nsIDOMDocumentFragment.h"
#include "nsIDOMComment.h"
#include "nsIDOMEventCapturer.h"
#include "nsIDOMNodeList.h"


static NS_DEFINE_IID(kIScriptObjectOwnerIID, NS_ISCRIPTOBJECTOWNER_IID);
static NS_DEFINE_IID(kIJSScriptObjectIID, NS_IJSSCRIPTOBJECT_IID);
static NS_DEFINE_IID(kIScriptGlobalObjectIID, NS_ISCRIPTGLOBALOBJECT_IID);
static NS_DEFINE_IID(kIElementIID, NS_IDOMELEMENT_IID);
static NS_DEFINE_IID(kIDocumentIID, NS_IDOMDOCUMENT_IID);
static NS_DEFINE_IID(kIProcessingInstructionIID, NS_IDOMPROCESSINGINSTRUCTION_IID);
static NS_DEFINE_IID(kINamedNodeMapIID, NS_IDOMNAMEDNODEMAP_IID);
static NS_DEFINE_IID(kIAttributeIID, NS_IDOMATTRIBUTE_IID);
static NS_DEFINE_IID(kINodeIID, NS_IDOMNODE_IID);
static NS_DEFINE_IID(kITextIID, NS_IDOMTEXT_IID);
static NS_DEFINE_IID(kIDocumentTypeIID, NS_IDOMDOCUMENTTYPE_IID);
static NS_DEFINE_IID(kIDocumentFragmentIID, NS_IDOMDOCUMENTFRAGMENT_IID);
static NS_DEFINE_IID(kICommentIID, NS_IDOMCOMMENT_IID);
static NS_DEFINE_IID(kIEventCapturerIID, NS_IDOMEVENTCAPTURER_IID);
static NS_DEFINE_IID(kINodeListIID, NS_IDOMNODELIST_IID);

NS_DEF_PTR(nsIDOMElement);
NS_DEF_PTR(nsIDOMDocument);
NS_DEF_PTR(nsIDOMProcessingInstruction);
NS_DEF_PTR(nsIDOMNamedNodeMap);
NS_DEF_PTR(nsIDOMAttribute);
NS_DEF_PTR(nsIDOMNode);
NS_DEF_PTR(nsIDOMText);
NS_DEF_PTR(nsIDOMDocumentType);
NS_DEF_PTR(nsIDOMDocumentFragment);
NS_DEF_PTR(nsIDOMComment);
NS_DEF_PTR(nsIDOMEventCapturer);
NS_DEF_PTR(nsIDOMNodeList);

//
// Document property ids
//
enum Document_slots {
  DOCUMENT_DOCUMENTTYPE = -11,
  DOCUMENT_PROLOG = -12,
  DOCUMENT_EPILOG = -13,
  DOCUMENT_DOCUMENTELEMENT = -14
};

/***********************************************************************/
//
// Document Properties Getter
//
PR_STATIC_CALLBACK(JSBool)
GetDocumentProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  nsIDOMDocument *a = (nsIDOMDocument*)JS_GetPrivate(cx, obj);

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == a) {
    return JS_TRUE;
  }

  if (JSVAL_IS_INT(id)) {
    switch(JSVAL_TO_INT(id)) {
      case DOCUMENT_DOCUMENTTYPE:
      {
        nsIDOMDocumentType* prop;
        if (NS_OK == a->GetDocumentType(&prop)) {
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
      case DOCUMENT_PROLOG:
      {
        nsIDOMNodeList* prop;
        if (NS_OK == a->GetProlog(&prop)) {
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
      case DOCUMENT_EPILOG:
      {
        nsIDOMNodeList* prop;
        if (NS_OK == a->GetEpilog(&prop)) {
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
      case DOCUMENT_DOCUMENTELEMENT:
      {
        nsIDOMElement* prop;
        if (NS_OK == a->GetDocumentElement(&prop)) {
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
// Document Properties Setter
//
PR_STATIC_CALLBACK(JSBool)
SetDocumentProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  nsIDOMDocument *a = (nsIDOMDocument*)JS_GetPrivate(cx, obj);

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == a) {
    return JS_TRUE;
  }

  if (JSVAL_IS_INT(id)) {
    switch(JSVAL_TO_INT(id)) {
      case 0:
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
// Document finalizer
//
PR_STATIC_CALLBACK(void)
FinalizeDocument(JSContext *cx, JSObject *obj)
{
  nsIDOMDocument *a = (nsIDOMDocument*)JS_GetPrivate(cx, obj);
  
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
// Document enumerate
//
PR_STATIC_CALLBACK(JSBool)
EnumerateDocument(JSContext *cx, JSObject *obj)
{
  nsIDOMDocument *a = (nsIDOMDocument*)JS_GetPrivate(cx, obj);
  
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
// Document resolve
//
PR_STATIC_CALLBACK(JSBool)
ResolveDocument(JSContext *cx, JSObject *obj, jsval id)
{
  nsIDOMDocument *a = (nsIDOMDocument*)JS_GetPrivate(cx, obj);
  
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
// Native method CreateElement
//
PR_STATIC_CALLBACK(JSBool)
DocumentCreateElement(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsIDOMDocument *nativeThis = (nsIDOMDocument*)JS_GetPrivate(cx, obj);
  JSBool rBool = JS_FALSE;
  nsIDOMElement* nativeRet;
  nsAutoString b0;
  nsIDOMNamedNodeMapPtr b1;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == nativeThis) {
    return JS_TRUE;
  }

  if (argc >= 2) {

    JSString *jsstring0 = JS_ValueToString(cx, argv[0]);
    if (nsnull != jsstring0) {
      b0.SetString(JS_GetStringChars(jsstring0));
    }
    else {
      b0.SetString("");   // Should this really be null?? 
    }

    if (JSVAL_IS_NULL(argv[1])){
      b1 = nsnull;
    }
    else if (JSVAL_IS_OBJECT(argv[1])) {
      nsISupports *supports1 = (nsISupports *)JS_GetPrivate(cx, JSVAL_TO_OBJECT(argv[1]));
      NS_ASSERTION(nsnull != supports1, "null pointer");

      if ((nsnull == supports1) ||
          (NS_OK != supports1->QueryInterface(kINamedNodeMapIID, (void **)(b1.Query())))) {
        JS_ReportError(cx, "Parameter must be of type NamedNodeMap");
        return JS_FALSE;
      }
    }
    else {
      JS_ReportError(cx, "Parameter must be an object");
      return JS_FALSE;
    }

    if (NS_OK != nativeThis->CreateElement(b0, b1, &nativeRet)) {
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
    JS_ReportError(cx, "Function createElement requires 2 parameters");
    return JS_FALSE;
  }

  return JS_TRUE;
}


//
// Native method CreateDocumentFragment
//
PR_STATIC_CALLBACK(JSBool)
DocumentCreateDocumentFragment(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsIDOMDocument *nativeThis = (nsIDOMDocument*)JS_GetPrivate(cx, obj);
  JSBool rBool = JS_FALSE;
  nsIDOMDocumentFragment* nativeRet;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == nativeThis) {
    return JS_TRUE;
  }

  if (argc >= 0) {

    if (NS_OK != nativeThis->CreateDocumentFragment(&nativeRet)) {
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
    JS_ReportError(cx, "Function createDocumentFragment requires 0 parameters");
    return JS_FALSE;
  }

  return JS_TRUE;
}


//
// Native method CreateTextNode
//
PR_STATIC_CALLBACK(JSBool)
DocumentCreateTextNode(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsIDOMDocument *nativeThis = (nsIDOMDocument*)JS_GetPrivate(cx, obj);
  JSBool rBool = JS_FALSE;
  nsIDOMText* nativeRet;
  nsAutoString b0;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == nativeThis) {
    return JS_TRUE;
  }

  if (argc >= 1) {

    JSString *jsstring0 = JS_ValueToString(cx, argv[0]);
    if (nsnull != jsstring0) {
      b0.SetString(JS_GetStringChars(jsstring0));
    }
    else {
      b0.SetString("");   // Should this really be null?? 
    }

    if (NS_OK != nativeThis->CreateTextNode(b0, &nativeRet)) {
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
    JS_ReportError(cx, "Function createTextNode requires 1 parameters");
    return JS_FALSE;
  }

  return JS_TRUE;
}


//
// Native method CreateComment
//
PR_STATIC_CALLBACK(JSBool)
DocumentCreateComment(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsIDOMDocument *nativeThis = (nsIDOMDocument*)JS_GetPrivate(cx, obj);
  JSBool rBool = JS_FALSE;
  nsIDOMComment* nativeRet;
  nsAutoString b0;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == nativeThis) {
    return JS_TRUE;
  }

  if (argc >= 1) {

    JSString *jsstring0 = JS_ValueToString(cx, argv[0]);
    if (nsnull != jsstring0) {
      b0.SetString(JS_GetStringChars(jsstring0));
    }
    else {
      b0.SetString("");   // Should this really be null?? 
    }

    if (NS_OK != nativeThis->CreateComment(b0, &nativeRet)) {
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
    JS_ReportError(cx, "Function createComment requires 1 parameters");
    return JS_FALSE;
  }

  return JS_TRUE;
}


//
// Native method CreateProcessingInstruction
//
PR_STATIC_CALLBACK(JSBool)
DocumentCreateProcessingInstruction(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsIDOMDocument *nativeThis = (nsIDOMDocument*)JS_GetPrivate(cx, obj);
  JSBool rBool = JS_FALSE;
  nsIDOMProcessingInstruction* nativeRet;
  nsAutoString b0;
  nsAutoString b1;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == nativeThis) {
    return JS_TRUE;
  }

  if (argc >= 2) {

    JSString *jsstring0 = JS_ValueToString(cx, argv[0]);
    if (nsnull != jsstring0) {
      b0.SetString(JS_GetStringChars(jsstring0));
    }
    else {
      b0.SetString("");   // Should this really be null?? 
    }

    JSString *jsstring1 = JS_ValueToString(cx, argv[1]);
    if (nsnull != jsstring1) {
      b1.SetString(JS_GetStringChars(jsstring1));
    }
    else {
      b1.SetString("");   // Should this really be null?? 
    }

    if (NS_OK != nativeThis->CreateProcessingInstruction(b0, b1, &nativeRet)) {
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
    JS_ReportError(cx, "Function createProcessingInstruction requires 2 parameters");
    return JS_FALSE;
  }

  return JS_TRUE;
}


//
// Native method CreateAttribute
//
PR_STATIC_CALLBACK(JSBool)
DocumentCreateAttribute(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsIDOMDocument *nativeThis = (nsIDOMDocument*)JS_GetPrivate(cx, obj);
  JSBool rBool = JS_FALSE;
  nsIDOMAttribute* nativeRet;
  nsAutoString b0;
  nsIDOMNodePtr b1;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == nativeThis) {
    return JS_TRUE;
  }

  if (argc >= 2) {

    JSString *jsstring0 = JS_ValueToString(cx, argv[0]);
    if (nsnull != jsstring0) {
      b0.SetString(JS_GetStringChars(jsstring0));
    }
    else {
      b0.SetString("");   // Should this really be null?? 
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

    if (NS_OK != nativeThis->CreateAttribute(b0, b1, &nativeRet)) {
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
    JS_ReportError(cx, "Function createAttribute requires 2 parameters");
    return JS_FALSE;
  }

  return JS_TRUE;
}


//
// Native method GetElementsByTagName
//
PR_STATIC_CALLBACK(JSBool)
DocumentGetElementsByTagName(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsIDOMDocument *nativeThis = (nsIDOMDocument*)JS_GetPrivate(cx, obj);
  JSBool rBool = JS_FALSE;
  nsIDOMNodeList* nativeRet;
  nsAutoString b0;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == nativeThis) {
    return JS_TRUE;
  }

  if (argc >= 1) {

    JSString *jsstring0 = JS_ValueToString(cx, argv[0]);
    if (nsnull != jsstring0) {
      b0.SetString(JS_GetStringChars(jsstring0));
    }
    else {
      b0.SetString("");   // Should this really be null?? 
    }

    if (NS_OK != nativeThis->GetElementsByTagName(b0, &nativeRet)) {
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
    JS_ReportError(cx, "Function getElementsByTagName requires 1 parameters");
    return JS_FALSE;
  }

  return JS_TRUE;
}


//
// Native method CaptureEvent
//
PR_STATIC_CALLBACK(JSBool)
EventCapturerCaptureEvent(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsIDOMDocument *privateThis = (nsIDOMDocument*)JS_GetPrivate(cx, obj);
  nsIDOMEventCapturer *nativeThis;
  if (NS_OK != privateThis->QueryInterface(kIEventCapturerIID, (void **)nativeThis)) {
    JS_ReportError(cx, "Object must be of type EventCapturer");
    return JS_FALSE;
  }

  JSBool rBool = JS_FALSE;
  nsAutoString b0;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == nativeThis) {
    return JS_TRUE;
  }

  if (argc >= 1) {

    JSString *jsstring0 = JS_ValueToString(cx, argv[0]);
    if (nsnull != jsstring0) {
      b0.SetString(JS_GetStringChars(jsstring0));
    }
    else {
      b0.SetString("");   // Should this really be null?? 
    }

    if (NS_OK != nativeThis->CaptureEvent(b0)) {
      return JS_FALSE;
    }

    *rval = JSVAL_VOID;
  }
  else {
    JS_ReportError(cx, "Function captureEvent requires 1 parameters");
    return JS_FALSE;
  }

  return JS_TRUE;
}


//
// Native method ReleaseEvent
//
PR_STATIC_CALLBACK(JSBool)
EventCapturerReleaseEvent(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsIDOMDocument *privateThis = (nsIDOMDocument*)JS_GetPrivate(cx, obj);
  nsIDOMEventCapturer *nativeThis;
  if (NS_OK != privateThis->QueryInterface(kIEventCapturerIID, (void **)nativeThis)) {
    JS_ReportError(cx, "Object must be of type EventCapturer");
    return JS_FALSE;
  }

  JSBool rBool = JS_FALSE;
  nsAutoString b0;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == nativeThis) {
    return JS_TRUE;
  }

  if (argc >= 1) {

    JSString *jsstring0 = JS_ValueToString(cx, argv[0]);
    if (nsnull != jsstring0) {
      b0.SetString(JS_GetStringChars(jsstring0));
    }
    else {
      b0.SetString("");   // Should this really be null?? 
    }

    if (NS_OK != nativeThis->ReleaseEvent(b0)) {
      return JS_FALSE;
    }

    *rval = JSVAL_VOID;
  }
  else {
    JS_ReportError(cx, "Function releaseEvent requires 1 parameters");
    return JS_FALSE;
  }

  return JS_TRUE;
}


/***********************************************************************/
//
// class for Document
//
JSClass DocumentClass = {
  "Document", 
  JSCLASS_HAS_PRIVATE,
  JS_PropertyStub,
  JS_PropertyStub,
  GetDocumentProperty,
  SetDocumentProperty,
  EnumerateDocument,
  ResolveDocument,
  JS_ConvertStub,
  FinalizeDocument
};


//
// Document class properties
//
static JSPropertySpec DocumentProperties[] =
{
  {"documentType",    DOCUMENT_DOCUMENTTYPE,    JSPROP_ENUMERATE | JSPROP_READONLY},
  {"prolog",    DOCUMENT_PROLOG,    JSPROP_ENUMERATE | JSPROP_READONLY},
  {"epilog",    DOCUMENT_EPILOG,    JSPROP_ENUMERATE | JSPROP_READONLY},
  {"documentElement",    DOCUMENT_DOCUMENTELEMENT,    JSPROP_ENUMERATE | JSPROP_READONLY},
  {0}
};


//
// Document class methods
//
static JSFunctionSpec DocumentMethods[] = 
{
  {"createElement",          DocumentCreateElement,     2},
  {"createDocumentFragment",          DocumentCreateDocumentFragment,     0},
  {"createTextNode",          DocumentCreateTextNode,     1},
  {"createComment",          DocumentCreateComment,     1},
  {"createProcessingInstruction",          DocumentCreateProcessingInstruction,     2},
  {"createAttribute",          DocumentCreateAttribute,     2},
  {"getElementsByTagName",          DocumentGetElementsByTagName,     1},
  {"captureEvent",          EventCapturerCaptureEvent,     1},
  {"releaseEvent",          EventCapturerReleaseEvent,     1},
  {0}
};


//
// Document constructor
//
PR_STATIC_CALLBACK(JSBool)
Document(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  return JS_TRUE;
}


//
// Document class initialization
//
nsresult NS_InitDocumentClass(nsIScriptContext *aContext, void **aPrototype)
{
  JSContext *jscontext = (JSContext *)aContext->GetNativeContext();
  JSObject *proto = nsnull;
  JSObject *constructor = nsnull;
  JSObject *parent_proto = nsnull;
  JSObject *global = JS_GetGlobalObject(jscontext);
  jsval vp;

  if ((PR_TRUE != JS_LookupProperty(jscontext, global, "Document", &vp)) ||
      !JSVAL_IS_OBJECT(vp) ||
      ((constructor = JSVAL_TO_OBJECT(vp)) == nsnull) ||
      (PR_TRUE != JS_LookupProperty(jscontext, JSVAL_TO_OBJECT(vp), "prototype", &vp)) || 
      !JSVAL_IS_OBJECT(vp)) {

    if (NS_OK != NS_InitDocumentFragmentClass(aContext, (void **)&parent_proto)) {
      return NS_ERROR_FAILURE;
    }
    proto = JS_InitClass(jscontext,     // context
                         global,        // global object
                         parent_proto,  // parent proto 
                         &DocumentClass,      // JSClass
                         Document,            // JSNative ctor
                         0,             // ctor args
                         DocumentProperties,  // proto props
                         DocumentMethods,     // proto funcs
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
// Method for creating a new Document JavaScript object
//
extern "C" NS_DOM nsresult NS_NewScriptDocument(nsIScriptContext *aContext, nsISupports *aSupports, nsISupports *aParent, void **aReturn)
{
  NS_PRECONDITION(nsnull != aContext && nsnull != aSupports && nsnull != aReturn, "null argument to NS_NewScriptDocument");
  JSObject *proto;
  JSObject *parent;
  nsIScriptObjectOwner *owner;
  JSContext *jscontext = (JSContext *)aContext->GetNativeContext();
  nsresult result = NS_OK;
  nsIDOMDocument *aDocument;

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

  if (NS_OK != NS_InitDocumentClass(aContext, (void **)&proto)) {
    return NS_ERROR_FAILURE;
  }

  result = aSupports->QueryInterface(kIDocumentIID, (void **)&aDocument);
  if (NS_OK != result) {
    return result;
  }

  // create a js object for this class
  *aReturn = JS_NewObject(jscontext, &DocumentClass, proto, parent);
  if (nsnull != *aReturn) {
    // connect the native object to the js object
    JS_SetPrivate(jscontext, (JSObject *)*aReturn, aDocument);
  }
  else {
    NS_RELEASE(aDocument);
    return NS_ERROR_FAILURE; 
  }

  return NS_OK;
}
