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
#include "nsIDOMHTMLElement.h"
#include "nsIDOMHTMLDocument.h"
#include "nsIDOMNSHTMLDocument.h"
#include "nsIDOMHTMLCollection.h"
#include "nsIDOMNodeList.h"


static NS_DEFINE_IID(kIScriptObjectOwnerIID, NS_ISCRIPTOBJECTOWNER_IID);
static NS_DEFINE_IID(kIJSScriptObjectIID, NS_IJSSCRIPTOBJECT_IID);
static NS_DEFINE_IID(kIScriptGlobalObjectIID, NS_ISCRIPTGLOBALOBJECT_IID);
static NS_DEFINE_IID(kIElementIID, NS_IDOMELEMENT_IID);
static NS_DEFINE_IID(kIHTMLElementIID, NS_IDOMHTMLELEMENT_IID);
static NS_DEFINE_IID(kIHTMLDocumentIID, NS_IDOMHTMLDOCUMENT_IID);
static NS_DEFINE_IID(kINSHTMLDocumentIID, NS_IDOMNSHTMLDOCUMENT_IID);
static NS_DEFINE_IID(kIHTMLCollectionIID, NS_IDOMHTMLCOLLECTION_IID);
static NS_DEFINE_IID(kINodeListIID, NS_IDOMNODELIST_IID);

NS_DEF_PTR(nsIDOMElement);
NS_DEF_PTR(nsIDOMHTMLElement);
NS_DEF_PTR(nsIDOMHTMLDocument);
NS_DEF_PTR(nsIDOMNSHTMLDocument);
NS_DEF_PTR(nsIDOMHTMLCollection);
NS_DEF_PTR(nsIDOMNodeList);

//
// HTMLDocument property ids
//
enum HTMLDocument_slots {
  HTMLDOCUMENT_TITLE = -11,
  HTMLDOCUMENT_REFERRER = -12,
  HTMLDOCUMENT_FILESIZE = -13,
  HTMLDOCUMENT_FILECREATEDDATE = -14,
  HTMLDOCUMENT_FILEMODIFIEDDATE = -15,
  HTMLDOCUMENT_FILEUPDATEDDATE = -16,
  HTMLDOCUMENT_DOMAIN = -17,
  HTMLDOCUMENT_URL = -18,
  HTMLDOCUMENT_BODY = -19,
  HTMLDOCUMENT_IMAGES = -110,
  HTMLDOCUMENT_APPLETS = -111,
  HTMLDOCUMENT_LINKS = -112,
  HTMLDOCUMENT_FORMS = -113,
  HTMLDOCUMENT_ANCHORS = -114,
  HTMLDOCUMENT_COOKIE = -115,
  NSHTMLDOCUMENT_ALINKCOLOR = -21,
  NSHTMLDOCUMENT_LINKCOLOR = -22,
  NSHTMLDOCUMENT_VLINKCOLOR = -23,
  NSHTMLDOCUMENT_BGCOLOR = -24,
  NSHTMLDOCUMENT_FGCOLOR = -25,
  NSHTMLDOCUMENT_LASTMODIFIED = -26,
  NSHTMLDOCUMENT_EMBEDS = -27,
  NSHTMLDOCUMENT_LAYERS = -28,
  NSHTMLDOCUMENT_PLUGINS = -29
};

/***********************************************************************/
//
// HTMLDocument Properties Getter
//
PR_STATIC_CALLBACK(JSBool)
GetHTMLDocumentProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  nsIDOMHTMLDocument *a = (nsIDOMHTMLDocument*)JS_GetPrivate(cx, obj);

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == a) {
    return JS_TRUE;
  }

  if (JSVAL_IS_INT(id)) {
    switch(JSVAL_TO_INT(id)) {
      case HTMLDOCUMENT_TITLE:
      {
        nsAutoString prop;
        if (NS_OK == a->GetTitle(prop)) {
          JSString *jsstring = JS_NewUCStringCopyN(cx, prop, prop.Length());
          // set the return value
          *vp = STRING_TO_JSVAL(jsstring);
        }
        else {
          return JS_FALSE;
        }
        break;
      }
      case HTMLDOCUMENT_REFERRER:
      {
        nsAutoString prop;
        if (NS_OK == a->GetReferrer(prop)) {
          JSString *jsstring = JS_NewUCStringCopyN(cx, prop, prop.Length());
          // set the return value
          *vp = STRING_TO_JSVAL(jsstring);
        }
        else {
          return JS_FALSE;
        }
        break;
      }
      case HTMLDOCUMENT_FILESIZE:
      {
        nsAutoString prop;
        if (NS_OK == a->GetFileSize(prop)) {
          JSString *jsstring = JS_NewUCStringCopyN(cx, prop, prop.Length());
          // set the return value
          *vp = STRING_TO_JSVAL(jsstring);
        }
        else {
          return JS_FALSE;
        }
        break;
      }
      case HTMLDOCUMENT_FILECREATEDDATE:
      {
        nsAutoString prop;
        if (NS_OK == a->GetFileCreatedDate(prop)) {
          JSString *jsstring = JS_NewUCStringCopyN(cx, prop, prop.Length());
          // set the return value
          *vp = STRING_TO_JSVAL(jsstring);
        }
        else {
          return JS_FALSE;
        }
        break;
      }
      case HTMLDOCUMENT_FILEMODIFIEDDATE:
      {
        nsAutoString prop;
        if (NS_OK == a->GetFileModifiedDate(prop)) {
          JSString *jsstring = JS_NewUCStringCopyN(cx, prop, prop.Length());
          // set the return value
          *vp = STRING_TO_JSVAL(jsstring);
        }
        else {
          return JS_FALSE;
        }
        break;
      }
      case HTMLDOCUMENT_FILEUPDATEDDATE:
      {
        nsAutoString prop;
        if (NS_OK == a->GetFileUpdatedDate(prop)) {
          JSString *jsstring = JS_NewUCStringCopyN(cx, prop, prop.Length());
          // set the return value
          *vp = STRING_TO_JSVAL(jsstring);
        }
        else {
          return JS_FALSE;
        }
        break;
      }
      case HTMLDOCUMENT_DOMAIN:
      {
        nsAutoString prop;
        if (NS_OK == a->GetDomain(prop)) {
          JSString *jsstring = JS_NewUCStringCopyN(cx, prop, prop.Length());
          // set the return value
          *vp = STRING_TO_JSVAL(jsstring);
        }
        else {
          return JS_FALSE;
        }
        break;
      }
      case HTMLDOCUMENT_URL:
      {
        nsAutoString prop;
        if (NS_OK == a->GetURL(prop)) {
          JSString *jsstring = JS_NewUCStringCopyN(cx, prop, prop.Length());
          // set the return value
          *vp = STRING_TO_JSVAL(jsstring);
        }
        else {
          return JS_FALSE;
        }
        break;
      }
      case HTMLDOCUMENT_BODY:
      {
        nsIDOMHTMLElement* prop;
        if (NS_OK == a->GetBody(&prop)) {
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
      case HTMLDOCUMENT_IMAGES:
      {
        nsIDOMHTMLCollection* prop;
        if (NS_OK == a->GetImages(&prop)) {
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
      case HTMLDOCUMENT_APPLETS:
      {
        nsIDOMHTMLCollection* prop;
        if (NS_OK == a->GetApplets(&prop)) {
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
      case HTMLDOCUMENT_LINKS:
      {
        nsIDOMHTMLCollection* prop;
        if (NS_OK == a->GetLinks(&prop)) {
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
      case HTMLDOCUMENT_FORMS:
      {
        nsIDOMHTMLCollection* prop;
        if (NS_OK == a->GetForms(&prop)) {
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
      case HTMLDOCUMENT_ANCHORS:
      {
        nsIDOMHTMLCollection* prop;
        if (NS_OK == a->GetAnchors(&prop)) {
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
      case HTMLDOCUMENT_COOKIE:
      {
        nsAutoString prop;
        if (NS_OK == a->GetCookie(prop)) {
          JSString *jsstring = JS_NewUCStringCopyN(cx, prop, prop.Length());
          // set the return value
          *vp = STRING_TO_JSVAL(jsstring);
        }
        else {
          return JS_FALSE;
        }
        break;
      }
      case NSHTMLDOCUMENT_ALINKCOLOR:
      {
        nsAutoString prop;
        nsIDOMNSHTMLDocument* b;
        if (NS_OK == a->QueryInterface(kINSHTMLDocumentIID, (void **)&b)) {
          if(NS_OK == b->GetAlinkColor(prop)) {
          JSString *jsstring = JS_NewUCStringCopyN(cx, prop, prop.Length());
          // set the return value
          *vp = STRING_TO_JSVAL(jsstring);
            NS_RELEASE(b);
          }
          else {
            NS_RELEASE(b);
            return JS_FALSE;
          }
        }
        else {
          JS_ReportError(cx, "Object must be of type NSHTMLDocument");
          return JS_FALSE;
        }
        break;
      }
      case NSHTMLDOCUMENT_LINKCOLOR:
      {
        nsAutoString prop;
        nsIDOMNSHTMLDocument* b;
        if (NS_OK == a->QueryInterface(kINSHTMLDocumentIID, (void **)&b)) {
          if(NS_OK == b->GetLinkColor(prop)) {
          JSString *jsstring = JS_NewUCStringCopyN(cx, prop, prop.Length());
          // set the return value
          *vp = STRING_TO_JSVAL(jsstring);
            NS_RELEASE(b);
          }
          else {
            NS_RELEASE(b);
            return JS_FALSE;
          }
        }
        else {
          JS_ReportError(cx, "Object must be of type NSHTMLDocument");
          return JS_FALSE;
        }
        break;
      }
      case NSHTMLDOCUMENT_VLINKCOLOR:
      {
        nsAutoString prop;
        nsIDOMNSHTMLDocument* b;
        if (NS_OK == a->QueryInterface(kINSHTMLDocumentIID, (void **)&b)) {
          if(NS_OK == b->GetVlinkColor(prop)) {
          JSString *jsstring = JS_NewUCStringCopyN(cx, prop, prop.Length());
          // set the return value
          *vp = STRING_TO_JSVAL(jsstring);
            NS_RELEASE(b);
          }
          else {
            NS_RELEASE(b);
            return JS_FALSE;
          }
        }
        else {
          JS_ReportError(cx, "Object must be of type NSHTMLDocument");
          return JS_FALSE;
        }
        break;
      }
      case NSHTMLDOCUMENT_BGCOLOR:
      {
        nsAutoString prop;
        nsIDOMNSHTMLDocument* b;
        if (NS_OK == a->QueryInterface(kINSHTMLDocumentIID, (void **)&b)) {
          if(NS_OK == b->GetBgColor(prop)) {
          JSString *jsstring = JS_NewUCStringCopyN(cx, prop, prop.Length());
          // set the return value
          *vp = STRING_TO_JSVAL(jsstring);
            NS_RELEASE(b);
          }
          else {
            NS_RELEASE(b);
            return JS_FALSE;
          }
        }
        else {
          JS_ReportError(cx, "Object must be of type NSHTMLDocument");
          return JS_FALSE;
        }
        break;
      }
      case NSHTMLDOCUMENT_FGCOLOR:
      {
        nsAutoString prop;
        nsIDOMNSHTMLDocument* b;
        if (NS_OK == a->QueryInterface(kINSHTMLDocumentIID, (void **)&b)) {
          if(NS_OK == b->GetFgColor(prop)) {
          JSString *jsstring = JS_NewUCStringCopyN(cx, prop, prop.Length());
          // set the return value
          *vp = STRING_TO_JSVAL(jsstring);
            NS_RELEASE(b);
          }
          else {
            NS_RELEASE(b);
            return JS_FALSE;
          }
        }
        else {
          JS_ReportError(cx, "Object must be of type NSHTMLDocument");
          return JS_FALSE;
        }
        break;
      }
      case NSHTMLDOCUMENT_LASTMODIFIED:
      {
        nsAutoString prop;
        nsIDOMNSHTMLDocument* b;
        if (NS_OK == a->QueryInterface(kINSHTMLDocumentIID, (void **)&b)) {
          if(NS_OK == b->GetLastModified(prop)) {
          JSString *jsstring = JS_NewUCStringCopyN(cx, prop, prop.Length());
          // set the return value
          *vp = STRING_TO_JSVAL(jsstring);
            NS_RELEASE(b);
          }
          else {
            NS_RELEASE(b);
            return JS_FALSE;
          }
        }
        else {
          JS_ReportError(cx, "Object must be of type NSHTMLDocument");
          return JS_FALSE;
        }
        break;
      }
      case NSHTMLDOCUMENT_EMBEDS:
      {
        nsIDOMHTMLCollection* prop;
        nsIDOMNSHTMLDocument* b;
        if (NS_OK == a->QueryInterface(kINSHTMLDocumentIID, (void **)&b)) {
          if(NS_OK == b->GetEmbeds(&prop)) {
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
            NS_RELEASE(b);
          }
          else {
            NS_RELEASE(b);
            return JS_FALSE;
          }
        }
        else {
          JS_ReportError(cx, "Object must be of type NSHTMLDocument");
          return JS_FALSE;
        }
        break;
      }
      case NSHTMLDOCUMENT_LAYERS:
      {
        nsIDOMHTMLCollection* prop;
        nsIDOMNSHTMLDocument* b;
        if (NS_OK == a->QueryInterface(kINSHTMLDocumentIID, (void **)&b)) {
          if(NS_OK == b->GetLayers(&prop)) {
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
            NS_RELEASE(b);
          }
          else {
            NS_RELEASE(b);
            return JS_FALSE;
          }
        }
        else {
          JS_ReportError(cx, "Object must be of type NSHTMLDocument");
          return JS_FALSE;
        }
        break;
      }
      case NSHTMLDOCUMENT_PLUGINS:
      {
        nsIDOMHTMLCollection* prop;
        nsIDOMNSHTMLDocument* b;
        if (NS_OK == a->QueryInterface(kINSHTMLDocumentIID, (void **)&b)) {
          if(NS_OK == b->GetPlugins(&prop)) {
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
            NS_RELEASE(b);
          }
          else {
            NS_RELEASE(b);
            return JS_FALSE;
          }
        }
        else {
          JS_ReportError(cx, "Object must be of type NSHTMLDocument");
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
  else if (JSVAL_IS_STRING(id)) {
    nsIDOMElement* prop;
    nsIDOMNSHTMLDocument* b;
    nsAutoString name;

    JSString *jsstring = JS_ValueToString(cx, id);
    if (nsnull != jsstring) {
      name.SetString(JS_GetStringChars(jsstring));
    }
    else {
      name.SetString("");
    }

    if (NS_OK == a->QueryInterface(kINSHTMLDocumentIID, (void **)&b)) {
      if (NS_OK == b->NamedItem(name, &prop)) {
        NS_RELEASE(b);
        if (NULL != prop) {
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
          nsIJSScriptObject *object;
          if (NS_OK == a->QueryInterface(kIJSScriptObjectIID, (void**)&object)) {
            PRBool rval;
            rval =  object->GetProperty(cx, id, vp);
            NS_RELEASE(object);
            return rval;
          }
        }
      }
      else {
        NS_RELEASE(b);
        return JS_FALSE;
      }
    }
    else {
      JS_ReportError(cx, "Object must be of type NSHTMLDocument");
      return JS_FALSE;
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
// HTMLDocument Properties Setter
//
PR_STATIC_CALLBACK(JSBool)
SetHTMLDocumentProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  nsIDOMHTMLDocument *a = (nsIDOMHTMLDocument*)JS_GetPrivate(cx, obj);

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == a) {
    return JS_TRUE;
  }

  if (JSVAL_IS_INT(id)) {
    switch(JSVAL_TO_INT(id)) {
      case HTMLDOCUMENT_TITLE:
      {
        nsAutoString prop;
        JSString *jsstring;
        if ((jsstring = JS_ValueToString(cx, *vp)) != nsnull) {
          prop.SetString(JS_GetStringChars(jsstring));
        }
        else {
          prop.SetString((const char *)nsnull);
        }
      
        a->SetTitle(prop);
        
        break;
      }
      case HTMLDOCUMENT_BODY:
      {
        nsIDOMHTMLElement* prop;
        if (JSVAL_IS_NULL(*vp)) {
          prop = nsnull;
        }
        else if (JSVAL_IS_OBJECT(*vp)) {
          JSObject *jsobj = JSVAL_TO_OBJECT(*vp); 
          nsISupports *supports = (nsISupports *)JS_GetPrivate(cx, jsobj);
          if (NS_OK != supports->QueryInterface(kIHTMLElementIID, (void **)&prop)) {
            JS_ReportError(cx, "Parameter must be of type HTMLElement");
            return JS_FALSE;
          }
        }
        else {
          JS_ReportError(cx, "Parameter must be an object");
          return JS_FALSE;
        }
      
        a->SetBody(prop);
        if (prop) NS_RELEASE(prop);
        break;
      }
      case HTMLDOCUMENT_COOKIE:
      {
        nsAutoString prop;
        JSString *jsstring;
        if ((jsstring = JS_ValueToString(cx, *vp)) != nsnull) {
          prop.SetString(JS_GetStringChars(jsstring));
        }
        else {
          prop.SetString((const char *)nsnull);
        }
      
        a->SetCookie(prop);
        
        break;
      }
      case NSHTMLDOCUMENT_ALINKCOLOR:
      {
        nsAutoString prop;
        JSString *jsstring;
        if ((jsstring = JS_ValueToString(cx, *vp)) != nsnull) {
          prop.SetString(JS_GetStringChars(jsstring));
        }
        else {
          prop.SetString((const char *)nsnull);
        }
      
        nsIDOMNSHTMLDocument *b;
        if (NS_OK == a->QueryInterface(kINSHTMLDocumentIID, (void **)&b)) {
          b->SetAlinkColor(prop);
          NS_RELEASE(b);
        }
        else {
           
           JS_ReportError(cx, "Object must be of type NSHTMLDocument");
           return JS_FALSE;
        }
        
        break;
      }
      case NSHTMLDOCUMENT_LINKCOLOR:
      {
        nsAutoString prop;
        JSString *jsstring;
        if ((jsstring = JS_ValueToString(cx, *vp)) != nsnull) {
          prop.SetString(JS_GetStringChars(jsstring));
        }
        else {
          prop.SetString((const char *)nsnull);
        }
      
        nsIDOMNSHTMLDocument *b;
        if (NS_OK == a->QueryInterface(kINSHTMLDocumentIID, (void **)&b)) {
          b->SetLinkColor(prop);
          NS_RELEASE(b);
        }
        else {
           
           JS_ReportError(cx, "Object must be of type NSHTMLDocument");
           return JS_FALSE;
        }
        
        break;
      }
      case NSHTMLDOCUMENT_VLINKCOLOR:
      {
        nsAutoString prop;
        JSString *jsstring;
        if ((jsstring = JS_ValueToString(cx, *vp)) != nsnull) {
          prop.SetString(JS_GetStringChars(jsstring));
        }
        else {
          prop.SetString((const char *)nsnull);
        }
      
        nsIDOMNSHTMLDocument *b;
        if (NS_OK == a->QueryInterface(kINSHTMLDocumentIID, (void **)&b)) {
          b->SetVlinkColor(prop);
          NS_RELEASE(b);
        }
        else {
           
           JS_ReportError(cx, "Object must be of type NSHTMLDocument");
           return JS_FALSE;
        }
        
        break;
      }
      case NSHTMLDOCUMENT_BGCOLOR:
      {
        nsAutoString prop;
        JSString *jsstring;
        if ((jsstring = JS_ValueToString(cx, *vp)) != nsnull) {
          prop.SetString(JS_GetStringChars(jsstring));
        }
        else {
          prop.SetString((const char *)nsnull);
        }
      
        nsIDOMNSHTMLDocument *b;
        if (NS_OK == a->QueryInterface(kINSHTMLDocumentIID, (void **)&b)) {
          b->SetBgColor(prop);
          NS_RELEASE(b);
        }
        else {
           
           JS_ReportError(cx, "Object must be of type NSHTMLDocument");
           return JS_FALSE;
        }
        
        break;
      }
      case NSHTMLDOCUMENT_FGCOLOR:
      {
        nsAutoString prop;
        JSString *jsstring;
        if ((jsstring = JS_ValueToString(cx, *vp)) != nsnull) {
          prop.SetString(JS_GetStringChars(jsstring));
        }
        else {
          prop.SetString((const char *)nsnull);
        }
      
        nsIDOMNSHTMLDocument *b;
        if (NS_OK == a->QueryInterface(kINSHTMLDocumentIID, (void **)&b)) {
          b->SetFgColor(prop);
          NS_RELEASE(b);
        }
        else {
           
           JS_ReportError(cx, "Object must be of type NSHTMLDocument");
           return JS_FALSE;
        }
        
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
// HTMLDocument finalizer
//
PR_STATIC_CALLBACK(void)
FinalizeHTMLDocument(JSContext *cx, JSObject *obj)
{
  nsIDOMHTMLDocument *a = (nsIDOMHTMLDocument*)JS_GetPrivate(cx, obj);
  
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
// HTMLDocument enumerate
//
PR_STATIC_CALLBACK(JSBool)
EnumerateHTMLDocument(JSContext *cx, JSObject *obj)
{
  nsIDOMHTMLDocument *a = (nsIDOMHTMLDocument*)JS_GetPrivate(cx, obj);
  
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
// HTMLDocument resolve
//
PR_STATIC_CALLBACK(JSBool)
ResolveHTMLDocument(JSContext *cx, JSObject *obj, jsval id)
{
  nsIDOMHTMLDocument *a = (nsIDOMHTMLDocument*)JS_GetPrivate(cx, obj);
  
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
// Native method Open
//
PR_STATIC_CALLBACK(JSBool)
HTMLDocumentOpen(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsIDOMHTMLDocument *nativeThis = (nsIDOMHTMLDocument*)JS_GetPrivate(cx, obj);
  JSBool rBool = JS_FALSE;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == nativeThis) {
    return JS_TRUE;
  }

  if (argc >= 0) {

    if (NS_OK != nativeThis->Open(cx, argv+0, argc-0)) {
      return JS_FALSE;
    }

    *rval = JSVAL_VOID;
  }
  else {
    JS_ReportError(cx, "Function open requires 0 parameters");
    return JS_FALSE;
  }

  return JS_TRUE;
}


//
// Native method Close
//
PR_STATIC_CALLBACK(JSBool)
HTMLDocumentClose(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsIDOMHTMLDocument *nativeThis = (nsIDOMHTMLDocument*)JS_GetPrivate(cx, obj);
  JSBool rBool = JS_FALSE;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == nativeThis) {
    return JS_TRUE;
  }

  if (argc >= 0) {

    if (NS_OK != nativeThis->Close()) {
      return JS_FALSE;
    }

    *rval = JSVAL_VOID;
  }
  else {
    JS_ReportError(cx, "Function close requires 0 parameters");
    return JS_FALSE;
  }

  return JS_TRUE;
}


//
// Native method Write
//
PR_STATIC_CALLBACK(JSBool)
HTMLDocumentWrite(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsIDOMHTMLDocument *nativeThis = (nsIDOMHTMLDocument*)JS_GetPrivate(cx, obj);
  JSBool rBool = JS_FALSE;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == nativeThis) {
    return JS_TRUE;
  }

  if (argc >= 0) {

    if (NS_OK != nativeThis->Write(cx, argv+0, argc-0)) {
      return JS_FALSE;
    }

    *rval = JSVAL_VOID;
  }
  else {
    JS_ReportError(cx, "Function write requires 0 parameters");
    return JS_FALSE;
  }

  return JS_TRUE;
}


//
// Native method Writeln
//
PR_STATIC_CALLBACK(JSBool)
HTMLDocumentWriteln(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsIDOMHTMLDocument *nativeThis = (nsIDOMHTMLDocument*)JS_GetPrivate(cx, obj);
  JSBool rBool = JS_FALSE;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == nativeThis) {
    return JS_TRUE;
  }

  if (argc >= 0) {

    if (NS_OK != nativeThis->Writeln(cx, argv+0, argc-0)) {
      return JS_FALSE;
    }

    *rval = JSVAL_VOID;
  }
  else {
    JS_ReportError(cx, "Function writeln requires 0 parameters");
    return JS_FALSE;
  }

  return JS_TRUE;
}


//
// Native method GetElementById
//
PR_STATIC_CALLBACK(JSBool)
HTMLDocumentGetElementById(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsIDOMHTMLDocument *nativeThis = (nsIDOMHTMLDocument*)JS_GetPrivate(cx, obj);
  JSBool rBool = JS_FALSE;
  nsIDOMElement* nativeRet;
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

    if (NS_OK != nativeThis->GetElementById(b0, &nativeRet)) {
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
    JS_ReportError(cx, "Function getElementById requires 1 parameters");
    return JS_FALSE;
  }

  return JS_TRUE;
}


//
// Native method GetElementsByName
//
PR_STATIC_CALLBACK(JSBool)
HTMLDocumentGetElementsByName(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsIDOMHTMLDocument *nativeThis = (nsIDOMHTMLDocument*)JS_GetPrivate(cx, obj);
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

    if (NS_OK != nativeThis->GetElementsByName(b0, &nativeRet)) {
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
    JS_ReportError(cx, "Function getElementsByName requires 1 parameters");
    return JS_FALSE;
  }

  return JS_TRUE;
}


//
// Native method GetSelection
//
PR_STATIC_CALLBACK(JSBool)
NSHTMLDocumentGetSelection(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsIDOMHTMLDocument *privateThis = (nsIDOMHTMLDocument*)JS_GetPrivate(cx, obj);
  nsIDOMNSHTMLDocument *nativeThis;
  if (NS_OK != privateThis->QueryInterface(kINSHTMLDocumentIID, (void **)nativeThis)) {
    JS_ReportError(cx, "Object must be of type NSHTMLDocument");
    return JS_FALSE;
  }

  JSBool rBool = JS_FALSE;
  nsAutoString nativeRet;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == nativeThis) {
    return JS_TRUE;
  }

  if (argc >= 0) {

    if (NS_OK != nativeThis->GetSelection(nativeRet)) {
      return JS_FALSE;
    }

    JSString *jsstring = JS_NewUCStringCopyN(cx, nativeRet, nativeRet.Length());
    // set the return value
    *rval = STRING_TO_JSVAL(jsstring);
  }
  else {
    JS_ReportError(cx, "Function getSelection requires 0 parameters");
    return JS_FALSE;
  }

  return JS_TRUE;
}


//
// Native method NamedItem
//
PR_STATIC_CALLBACK(JSBool)
NSHTMLDocumentNamedItem(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsIDOMHTMLDocument *privateThis = (nsIDOMHTMLDocument*)JS_GetPrivate(cx, obj);
  nsIDOMNSHTMLDocument *nativeThis;
  if (NS_OK != privateThis->QueryInterface(kINSHTMLDocumentIID, (void **)nativeThis)) {
    JS_ReportError(cx, "Object must be of type NSHTMLDocument");
    return JS_FALSE;
  }

  JSBool rBool = JS_FALSE;
  nsIDOMElement* nativeRet;
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

    if (NS_OK != nativeThis->NamedItem(b0, &nativeRet)) {
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
    JS_ReportError(cx, "Function namedItem requires 1 parameters");
    return JS_FALSE;
  }

  return JS_TRUE;
}


/***********************************************************************/
//
// class for HTMLDocument
//
JSClass HTMLDocumentClass = {
  "HTMLDocument", 
  JSCLASS_HAS_PRIVATE,
  JS_PropertyStub,
  JS_PropertyStub,
  GetHTMLDocumentProperty,
  SetHTMLDocumentProperty,
  EnumerateHTMLDocument,
  ResolveHTMLDocument,
  JS_ConvertStub,
  FinalizeHTMLDocument
};


//
// HTMLDocument class properties
//
static JSPropertySpec HTMLDocumentProperties[] =
{
  {"title",    HTMLDOCUMENT_TITLE,    JSPROP_ENUMERATE},
  {"referrer",    HTMLDOCUMENT_REFERRER,    JSPROP_ENUMERATE | JSPROP_READONLY},
  {"fileSize",    HTMLDOCUMENT_FILESIZE,    JSPROP_ENUMERATE | JSPROP_READONLY},
  {"fileCreatedDate",    HTMLDOCUMENT_FILECREATEDDATE,    JSPROP_ENUMERATE | JSPROP_READONLY},
  {"fileModifiedDate",    HTMLDOCUMENT_FILEMODIFIEDDATE,    JSPROP_ENUMERATE | JSPROP_READONLY},
  {"fileUpdatedDate",    HTMLDOCUMENT_FILEUPDATEDDATE,    JSPROP_ENUMERATE | JSPROP_READONLY},
  {"domain",    HTMLDOCUMENT_DOMAIN,    JSPROP_ENUMERATE | JSPROP_READONLY},
  {"URL",    HTMLDOCUMENT_URL,    JSPROP_ENUMERATE | JSPROP_READONLY},
  {"body",    HTMLDOCUMENT_BODY,    JSPROP_ENUMERATE},
  {"images",    HTMLDOCUMENT_IMAGES,    JSPROP_ENUMERATE | JSPROP_READONLY},
  {"applets",    HTMLDOCUMENT_APPLETS,    JSPROP_ENUMERATE | JSPROP_READONLY},
  {"links",    HTMLDOCUMENT_LINKS,    JSPROP_ENUMERATE | JSPROP_READONLY},
  {"forms",    HTMLDOCUMENT_FORMS,    JSPROP_ENUMERATE | JSPROP_READONLY},
  {"anchors",    HTMLDOCUMENT_ANCHORS,    JSPROP_ENUMERATE | JSPROP_READONLY},
  {"cookie",    HTMLDOCUMENT_COOKIE,    JSPROP_ENUMERATE},
  {"alinkColor",    NSHTMLDOCUMENT_ALINKCOLOR,    JSPROP_ENUMERATE},
  {"linkColor",    NSHTMLDOCUMENT_LINKCOLOR,    JSPROP_ENUMERATE},
  {"vlinkColor",    NSHTMLDOCUMENT_VLINKCOLOR,    JSPROP_ENUMERATE},
  {"bgColor",    NSHTMLDOCUMENT_BGCOLOR,    JSPROP_ENUMERATE},
  {"fgColor",    NSHTMLDOCUMENT_FGCOLOR,    JSPROP_ENUMERATE},
  {"lastModified",    NSHTMLDOCUMENT_LASTMODIFIED,    JSPROP_ENUMERATE | JSPROP_READONLY},
  {"embeds",    NSHTMLDOCUMENT_EMBEDS,    JSPROP_ENUMERATE | JSPROP_READONLY},
  {"layers",    NSHTMLDOCUMENT_LAYERS,    JSPROP_ENUMERATE | JSPROP_READONLY},
  {"plugins",    NSHTMLDOCUMENT_PLUGINS,    JSPROP_ENUMERATE | JSPROP_READONLY},
  {0}
};


//
// HTMLDocument class methods
//
static JSFunctionSpec HTMLDocumentMethods[] = 
{
  {"open",          HTMLDocumentOpen,     0},
  {"close",          HTMLDocumentClose,     0},
  {"write",          HTMLDocumentWrite,     0},
  {"writeln",          HTMLDocumentWriteln,     0},
  {"getElementById",          HTMLDocumentGetElementById,     1},
  {"getElementsByName",          HTMLDocumentGetElementsByName,     1},
  {"getSelection",          NSHTMLDocumentGetSelection,     0},
  {"namedItem",          NSHTMLDocumentNamedItem,     1},
  {0}
};


//
// HTMLDocument constructor
//
PR_STATIC_CALLBACK(JSBool)
HTMLDocument(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  return JS_TRUE;
}


//
// HTMLDocument class initialization
//
nsresult NS_InitHTMLDocumentClass(nsIScriptContext *aContext, void **aPrototype)
{
  JSContext *jscontext = (JSContext *)aContext->GetNativeContext();
  JSObject *proto = nsnull;
  JSObject *constructor = nsnull;
  JSObject *parent_proto = nsnull;
  JSObject *global = JS_GetGlobalObject(jscontext);
  jsval vp;

  if ((PR_TRUE != JS_LookupProperty(jscontext, global, "HTMLDocument", &vp)) ||
      !JSVAL_IS_OBJECT(vp) ||
      ((constructor = JSVAL_TO_OBJECT(vp)) == nsnull) ||
      (PR_TRUE != JS_LookupProperty(jscontext, JSVAL_TO_OBJECT(vp), "prototype", &vp)) || 
      !JSVAL_IS_OBJECT(vp)) {

    if (NS_OK != NS_InitDocumentClass(aContext, (void **)&parent_proto)) {
      return NS_ERROR_FAILURE;
    }
    proto = JS_InitClass(jscontext,     // context
                         global,        // global object
                         parent_proto,  // parent proto 
                         &HTMLDocumentClass,      // JSClass
                         HTMLDocument,            // JSNative ctor
                         0,             // ctor args
                         HTMLDocumentProperties,  // proto props
                         HTMLDocumentMethods,     // proto funcs
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
// Method for creating a new HTMLDocument JavaScript object
//
extern "C" NS_DOM nsresult NS_NewScriptHTMLDocument(nsIScriptContext *aContext, nsISupports *aSupports, nsISupports *aParent, void **aReturn)
{
  NS_PRECONDITION(nsnull != aContext && nsnull != aSupports && nsnull != aReturn, "null argument to NS_NewScriptHTMLDocument");
  JSObject *proto;
  JSObject *parent;
  nsIScriptObjectOwner *owner;
  JSContext *jscontext = (JSContext *)aContext->GetNativeContext();
  nsresult result = NS_OK;
  nsIDOMHTMLDocument *aHTMLDocument;

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

  if (NS_OK != NS_InitHTMLDocumentClass(aContext, (void **)&proto)) {
    return NS_ERROR_FAILURE;
  }

  result = aSupports->QueryInterface(kIHTMLDocumentIID, (void **)&aHTMLDocument);
  if (NS_OK != result) {
    return result;
  }

  // create a js object for this class
  *aReturn = JS_NewObject(jscontext, &HTMLDocumentClass, proto, parent);
  if (nsnull != *aReturn) {
    // connect the native object to the js object
    JS_SetPrivate(jscontext, (JSObject *)*aReturn, aHTMLDocument);
  }
  else {
    NS_RELEASE(aHTMLDocument);
    return NS_ERROR_FAILURE; 
  }

  return NS_OK;
}
