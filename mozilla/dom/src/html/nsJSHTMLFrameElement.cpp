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
#include "nsIDOMHTMLFrameElement.h"


static NS_DEFINE_IID(kIScriptObjectOwnerIID, NS_ISCRIPTOBJECTOWNER_IID);
static NS_DEFINE_IID(kIJSScriptObjectIID, NS_IJSSCRIPTOBJECT_IID);
static NS_DEFINE_IID(kIScriptGlobalObjectIID, NS_ISCRIPTGLOBALOBJECT_IID);
static NS_DEFINE_IID(kIHTMLFrameElementIID, NS_IDOMHTMLFRAMEELEMENT_IID);

//
// HTMLFrameElement property ids
//
enum HTMLFrameElement_slots {
  HTMLFRAMEELEMENT_FRAMEBORDER = -1,
  HTMLFRAMEELEMENT_LONGDESC = -2,
  HTMLFRAMEELEMENT_MARGINHEIGHT = -3,
  HTMLFRAMEELEMENT_MARGINWIDTH = -4,
  HTMLFRAMEELEMENT_NAME = -5,
  HTMLFRAMEELEMENT_NORESIZE = -6,
  HTMLFRAMEELEMENT_SCROLLING = -7,
  HTMLFRAMEELEMENT_SRC = -8
};

/***********************************************************************/
//
// HTMLFrameElement Properties Getter
//
PR_STATIC_CALLBACK(JSBool)
GetHTMLFrameElementProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  nsIDOMHTMLFrameElement *a = (nsIDOMHTMLFrameElement*)nsJSUtils::nsGetNativeThis(cx, obj);

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
      case HTMLFRAMEELEMENT_FRAMEBORDER:
      {
        PRBool ok = PR_FALSE;
        secMan->CheckScriptAccess(cx, obj, NS_DOM_PROP_HTMLFRAMEELEMENT_FRAMEBORDER, PR_FALSE, &ok);
        if (!ok) {
          return nsJSUtils::nsReportError(cx, obj, NS_ERROR_DOM_SECURITY_ERR);
        }
        nsAutoString prop;
        nsresult result = NS_OK;
        result = a->GetFrameBorder(prop);
        if (NS_SUCCEEDED(result)) {
          nsJSUtils::nsConvertStringToJSVal(prop, cx, vp);
        }
        else {
          return nsJSUtils::nsReportError(cx, obj, result);
        }
        break;
      }
      case HTMLFRAMEELEMENT_LONGDESC:
      {
        PRBool ok = PR_FALSE;
        secMan->CheckScriptAccess(cx, obj, NS_DOM_PROP_HTMLFRAMEELEMENT_LONGDESC, PR_FALSE, &ok);
        if (!ok) {
          return nsJSUtils::nsReportError(cx, obj, NS_ERROR_DOM_SECURITY_ERR);
        }
        nsAutoString prop;
        nsresult result = NS_OK;
        result = a->GetLongDesc(prop);
        if (NS_SUCCEEDED(result)) {
          nsJSUtils::nsConvertStringToJSVal(prop, cx, vp);
        }
        else {
          return nsJSUtils::nsReportError(cx, obj, result);
        }
        break;
      }
      case HTMLFRAMEELEMENT_MARGINHEIGHT:
      {
        PRBool ok = PR_FALSE;
        secMan->CheckScriptAccess(cx, obj, NS_DOM_PROP_HTMLFRAMEELEMENT_MARGINHEIGHT, PR_FALSE, &ok);
        if (!ok) {
          return nsJSUtils::nsReportError(cx, obj, NS_ERROR_DOM_SECURITY_ERR);
        }
        nsAutoString prop;
        nsresult result = NS_OK;
        result = a->GetMarginHeight(prop);
        if (NS_SUCCEEDED(result)) {
          nsJSUtils::nsConvertStringToJSVal(prop, cx, vp);
        }
        else {
          return nsJSUtils::nsReportError(cx, obj, result);
        }
        break;
      }
      case HTMLFRAMEELEMENT_MARGINWIDTH:
      {
        PRBool ok = PR_FALSE;
        secMan->CheckScriptAccess(cx, obj, NS_DOM_PROP_HTMLFRAMEELEMENT_MARGINWIDTH, PR_FALSE, &ok);
        if (!ok) {
          return nsJSUtils::nsReportError(cx, obj, NS_ERROR_DOM_SECURITY_ERR);
        }
        nsAutoString prop;
        nsresult result = NS_OK;
        result = a->GetMarginWidth(prop);
        if (NS_SUCCEEDED(result)) {
          nsJSUtils::nsConvertStringToJSVal(prop, cx, vp);
        }
        else {
          return nsJSUtils::nsReportError(cx, obj, result);
        }
        break;
      }
      case HTMLFRAMEELEMENT_NAME:
      {
        PRBool ok = PR_FALSE;
        secMan->CheckScriptAccess(cx, obj, NS_DOM_PROP_HTMLFRAMEELEMENT_NAME, PR_FALSE, &ok);
        if (!ok) {
          return nsJSUtils::nsReportError(cx, obj, NS_ERROR_DOM_SECURITY_ERR);
        }
        nsAutoString prop;
        nsresult result = NS_OK;
        result = a->GetName(prop);
        if (NS_SUCCEEDED(result)) {
          nsJSUtils::nsConvertStringToJSVal(prop, cx, vp);
        }
        else {
          return nsJSUtils::nsReportError(cx, obj, result);
        }
        break;
      }
      case HTMLFRAMEELEMENT_NORESIZE:
      {
        PRBool ok = PR_FALSE;
        secMan->CheckScriptAccess(cx, obj, NS_DOM_PROP_HTMLFRAMEELEMENT_NORESIZE, PR_FALSE, &ok);
        if (!ok) {
          return nsJSUtils::nsReportError(cx, obj, NS_ERROR_DOM_SECURITY_ERR);
        }
        PRBool prop;
        nsresult result = NS_OK;
        result = a->GetNoResize(&prop);
        if (NS_SUCCEEDED(result)) {
          *vp = BOOLEAN_TO_JSVAL(prop);
        }
        else {
          return nsJSUtils::nsReportError(cx, obj, result);
        }
        break;
      }
      case HTMLFRAMEELEMENT_SCROLLING:
      {
        PRBool ok = PR_FALSE;
        secMan->CheckScriptAccess(cx, obj, NS_DOM_PROP_HTMLFRAMEELEMENT_SCROLLING, PR_FALSE, &ok);
        if (!ok) {
          return nsJSUtils::nsReportError(cx, obj, NS_ERROR_DOM_SECURITY_ERR);
        }
        nsAutoString prop;
        nsresult result = NS_OK;
        result = a->GetScrolling(prop);
        if (NS_SUCCEEDED(result)) {
          nsJSUtils::nsConvertStringToJSVal(prop, cx, vp);
        }
        else {
          return nsJSUtils::nsReportError(cx, obj, result);
        }
        break;
      }
      case HTMLFRAMEELEMENT_SRC:
      {
        PRBool ok = PR_FALSE;
        secMan->CheckScriptAccess(cx, obj, NS_DOM_PROP_HTMLFRAMEELEMENT_SRC, PR_FALSE, &ok);
        if (!ok) {
          return nsJSUtils::nsReportError(cx, obj, NS_ERROR_DOM_SECURITY_ERR);
        }
        nsAutoString prop;
        nsresult result = NS_OK;
        result = a->GetSrc(prop);
        if (NS_SUCCEEDED(result)) {
          nsJSUtils::nsConvertStringToJSVal(prop, cx, vp);
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
// HTMLFrameElement Properties Setter
//
PR_STATIC_CALLBACK(JSBool)
SetHTMLFrameElementProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  nsIDOMHTMLFrameElement *a = (nsIDOMHTMLFrameElement*)nsJSUtils::nsGetNativeThis(cx, obj);

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
      case HTMLFRAMEELEMENT_FRAMEBORDER:
      {
        PRBool ok = PR_FALSE;
        secMan->CheckScriptAccess(cx, obj, NS_DOM_PROP_HTMLFRAMEELEMENT_FRAMEBORDER, PR_TRUE, &ok);
        if (!ok) {
          return nsJSUtils::nsReportError(cx, obj, NS_ERROR_DOM_SECURITY_ERR);
        }
        nsAutoString prop;
        nsJSUtils::nsConvertJSValToString(prop, cx, *vp);
      
        a->SetFrameBorder(prop);
        
        break;
      }
      case HTMLFRAMEELEMENT_LONGDESC:
      {
        PRBool ok = PR_FALSE;
        secMan->CheckScriptAccess(cx, obj, NS_DOM_PROP_HTMLFRAMEELEMENT_LONGDESC, PR_TRUE, &ok);
        if (!ok) {
          return nsJSUtils::nsReportError(cx, obj, NS_ERROR_DOM_SECURITY_ERR);
        }
        nsAutoString prop;
        nsJSUtils::nsConvertJSValToString(prop, cx, *vp);
      
        a->SetLongDesc(prop);
        
        break;
      }
      case HTMLFRAMEELEMENT_MARGINHEIGHT:
      {
        PRBool ok = PR_FALSE;
        secMan->CheckScriptAccess(cx, obj, NS_DOM_PROP_HTMLFRAMEELEMENT_MARGINHEIGHT, PR_TRUE, &ok);
        if (!ok) {
          return nsJSUtils::nsReportError(cx, obj, NS_ERROR_DOM_SECURITY_ERR);
        }
        nsAutoString prop;
        nsJSUtils::nsConvertJSValToString(prop, cx, *vp);
      
        a->SetMarginHeight(prop);
        
        break;
      }
      case HTMLFRAMEELEMENT_MARGINWIDTH:
      {
        PRBool ok = PR_FALSE;
        secMan->CheckScriptAccess(cx, obj, NS_DOM_PROP_HTMLFRAMEELEMENT_MARGINWIDTH, PR_TRUE, &ok);
        if (!ok) {
          return nsJSUtils::nsReportError(cx, obj, NS_ERROR_DOM_SECURITY_ERR);
        }
        nsAutoString prop;
        nsJSUtils::nsConvertJSValToString(prop, cx, *vp);
      
        a->SetMarginWidth(prop);
        
        break;
      }
      case HTMLFRAMEELEMENT_NAME:
      {
        PRBool ok = PR_FALSE;
        secMan->CheckScriptAccess(cx, obj, NS_DOM_PROP_HTMLFRAMEELEMENT_NAME, PR_TRUE, &ok);
        if (!ok) {
          return nsJSUtils::nsReportError(cx, obj, NS_ERROR_DOM_SECURITY_ERR);
        }
        nsAutoString prop;
        nsJSUtils::nsConvertJSValToString(prop, cx, *vp);
      
        a->SetName(prop);
        
        break;
      }
      case HTMLFRAMEELEMENT_NORESIZE:
      {
        PRBool ok = PR_FALSE;
        secMan->CheckScriptAccess(cx, obj, NS_DOM_PROP_HTMLFRAMEELEMENT_NORESIZE, PR_TRUE, &ok);
        if (!ok) {
          return nsJSUtils::nsReportError(cx, obj, NS_ERROR_DOM_SECURITY_ERR);
        }
        PRBool prop;
        if (PR_FALSE == nsJSUtils::nsConvertJSValToBool(&prop, cx, *vp)) {
          return nsJSUtils::nsReportError(cx, obj,  NS_ERROR_DOM_NOT_BOOLEAN_ERR);
        }
      
        a->SetNoResize(prop);
        
        break;
      }
      case HTMLFRAMEELEMENT_SCROLLING:
      {
        PRBool ok = PR_FALSE;
        secMan->CheckScriptAccess(cx, obj, NS_DOM_PROP_HTMLFRAMEELEMENT_SCROLLING, PR_TRUE, &ok);
        if (!ok) {
          return nsJSUtils::nsReportError(cx, obj, NS_ERROR_DOM_SECURITY_ERR);
        }
        nsAutoString prop;
        nsJSUtils::nsConvertJSValToString(prop, cx, *vp);
      
        a->SetScrolling(prop);
        
        break;
      }
      case HTMLFRAMEELEMENT_SRC:
      {
        PRBool ok = PR_FALSE;
        secMan->CheckScriptAccess(cx, obj, NS_DOM_PROP_HTMLFRAMEELEMENT_SRC, PR_TRUE, &ok);
        if (!ok) {
          return nsJSUtils::nsReportError(cx, obj, NS_ERROR_DOM_SECURITY_ERR);
        }
        nsAutoString prop;
        nsJSUtils::nsConvertJSValToString(prop, cx, *vp);
      
        a->SetSrc(prop);
        
        break;
      }
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
// HTMLFrameElement finalizer
//
PR_STATIC_CALLBACK(void)
FinalizeHTMLFrameElement(JSContext *cx, JSObject *obj)
{
  nsJSUtils::nsGenericFinalize(cx, obj);
}


//
// HTMLFrameElement enumerate
//
PR_STATIC_CALLBACK(JSBool)
EnumerateHTMLFrameElement(JSContext *cx, JSObject *obj)
{
  return nsJSUtils::nsGenericEnumerate(cx, obj);
}


//
// HTMLFrameElement resolve
//
PR_STATIC_CALLBACK(JSBool)
ResolveHTMLFrameElement(JSContext *cx, JSObject *obj, jsval id)
{
  return nsJSUtils::nsGenericResolve(cx, obj, id);
}


/***********************************************************************/
//
// class for HTMLFrameElement
//
JSClass HTMLFrameElementClass = {
  "HTMLFrameElement", 
  JSCLASS_HAS_PRIVATE | JSCLASS_PRIVATE_IS_NSISUPPORTS,
  JS_PropertyStub,
  JS_PropertyStub,
  GetHTMLFrameElementProperty,
  SetHTMLFrameElementProperty,
  EnumerateHTMLFrameElement,
  ResolveHTMLFrameElement,
  JS_ConvertStub,
  FinalizeHTMLFrameElement
};


//
// HTMLFrameElement class properties
//
static JSPropertySpec HTMLFrameElementProperties[] =
{
  {"frameBorder",    HTMLFRAMEELEMENT_FRAMEBORDER,    JSPROP_ENUMERATE},
  {"longDesc",    HTMLFRAMEELEMENT_LONGDESC,    JSPROP_ENUMERATE},
  {"marginHeight",    HTMLFRAMEELEMENT_MARGINHEIGHT,    JSPROP_ENUMERATE},
  {"marginWidth",    HTMLFRAMEELEMENT_MARGINWIDTH,    JSPROP_ENUMERATE},
  {"name",    HTMLFRAMEELEMENT_NAME,    JSPROP_ENUMERATE},
  {"noResize",    HTMLFRAMEELEMENT_NORESIZE,    JSPROP_ENUMERATE},
  {"scrolling",    HTMLFRAMEELEMENT_SCROLLING,    JSPROP_ENUMERATE},
  {"src",    HTMLFRAMEELEMENT_SRC,    JSPROP_ENUMERATE},
  {0}
};


//
// HTMLFrameElement class methods
//
static JSFunctionSpec HTMLFrameElementMethods[] = 
{
  {0}
};


//
// HTMLFrameElement constructor
//
PR_STATIC_CALLBACK(JSBool)
HTMLFrameElement(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  return JS_FALSE;
}


//
// HTMLFrameElement class initialization
//
extern "C" NS_DOM nsresult NS_InitHTMLFrameElementClass(nsIScriptContext *aContext, void **aPrototype)
{
  JSContext *jscontext = (JSContext *)aContext->GetNativeContext();
  JSObject *proto = nsnull;
  JSObject *constructor = nsnull;
  JSObject *parent_proto = nsnull;
  JSObject *global = JS_GetGlobalObject(jscontext);
  jsval vp;

  if ((PR_TRUE != JS_LookupProperty(jscontext, global, "HTMLFrameElement", &vp)) ||
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
                         &HTMLFrameElementClass,      // JSClass
                         HTMLFrameElement,            // JSNative ctor
                         0,             // ctor args
                         HTMLFrameElementProperties,  // proto props
                         HTMLFrameElementMethods,     // proto funcs
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
// Method for creating a new HTMLFrameElement JavaScript object
//
extern "C" NS_DOM nsresult NS_NewScriptHTMLFrameElement(nsIScriptContext *aContext, nsISupports *aSupports, nsISupports *aParent, void **aReturn)
{
  NS_PRECONDITION(nsnull != aContext && nsnull != aSupports && nsnull != aReturn, "null argument to NS_NewScriptHTMLFrameElement");
  JSObject *proto;
  JSObject *parent;
  nsIScriptObjectOwner *owner;
  JSContext *jscontext = (JSContext *)aContext->GetNativeContext();
  nsresult result = NS_OK;
  nsIDOMHTMLFrameElement *aHTMLFrameElement;

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

  if (NS_OK != NS_InitHTMLFrameElementClass(aContext, (void **)&proto)) {
    return NS_ERROR_FAILURE;
  }

  result = aSupports->QueryInterface(kIHTMLFrameElementIID, (void **)&aHTMLFrameElement);
  if (NS_OK != result) {
    return result;
  }

  // create a js object for this class
  *aReturn = JS_NewObject(jscontext, &HTMLFrameElementClass, proto, parent);
  if (nsnull != *aReturn) {
    // connect the native object to the js object
    JS_SetPrivate(jscontext, (JSObject *)*aReturn, aHTMLFrameElement);
  }
  else {
    NS_RELEASE(aHTMLFrameElement);
    return NS_ERROR_FAILURE; 
  }

  return NS_OK;
}
