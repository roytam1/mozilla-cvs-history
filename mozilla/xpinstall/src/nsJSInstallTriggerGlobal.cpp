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
#include "nsCRT.h"
#include "nsString.h"
#include "nsVector.h"
#include "nsIDOMInstallVersion.h"
#include "nsIDOMInstallTriggerGlobal.h"

extern void ConvertJSValToStr(nsString&  aString,
                             JSContext* aContext,
                             jsval      aValue);

extern void ConvertStrToJSVal(const nsString& aProp,
                             JSContext* aContext,
                             jsval* aReturn);

extern PRBool ConvertJSValToBool(PRBool* aProp,
                                JSContext* aContext,
                                jsval aValue);

extern PRBool ConvertJSValToObj(nsISupports** aSupports,
                               REFNSIID aIID,
                               const nsString& aTypeName,
                               JSContext* aContext,
                               jsval aValue);

static NS_DEFINE_IID(kIScriptObjectOwnerIID, NS_ISCRIPTOBJECTOWNER_IID);
static NS_DEFINE_IID(kIJSScriptObjectIID, NS_IJSSCRIPTOBJECT_IID);
static NS_DEFINE_IID(kIScriptGlobalObjectIID, NS_ISCRIPTGLOBALOBJECT_IID);
static NS_DEFINE_IID(kIInstallTriggerGlobalIID, NS_IDOMINSTALLTRIGGERGLOBAL_IID);

NS_DEF_PTR(nsIDOMInstallTriggerGlobal);


/***********************************************************************/
//
// InstallTriggerGlobal Properties Getter
//
PR_STATIC_CALLBACK(JSBool)
GetInstallTriggerGlobalProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  nsIDOMInstallTriggerGlobal *a = (nsIDOMInstallTriggerGlobal*)JS_GetPrivate(cx, obj);

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
// InstallTriggerGlobal Properties Setter
//
PR_STATIC_CALLBACK(JSBool)
SetInstallTriggerGlobalProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  nsIDOMInstallTriggerGlobal *a = (nsIDOMInstallTriggerGlobal*)JS_GetPrivate(cx, obj);

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
// InstallTriggerGlobal finalizer
//
PR_STATIC_CALLBACK(void)
FinalizeInstallTriggerGlobal(JSContext *cx, JSObject *obj)
{
  nsJSUtils::nsGenericFinalize(cx, obj);
}


//
// InstallTriggerGlobal enumerate
//
PR_STATIC_CALLBACK(JSBool)
EnumerateInstallTriggerGlobal(JSContext *cx, JSObject *obj)
{
  return nsJSUtils::nsGenericEnumerate(cx, obj);
}


//
// InstallTriggerGlobal resolve
//
PR_STATIC_CALLBACK(JSBool)
ResolveInstallTriggerGlobal(JSContext *cx, JSObject *obj, jsval id)
{
  return nsJSUtils::nsGenericResolve(cx, obj, id);
}

//
// Native method UpdateEnabled
//
PR_STATIC_CALLBACK(JSBool)
InstallTriggerGlobalUpdateEnabled(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsIDOMInstallTriggerGlobal *nativeThis = (nsIDOMInstallTriggerGlobal*)JS_GetPrivate(cx, obj);
  JSBool rBool = JS_FALSE;
  PRBool nativeRet;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == nativeThis) {
    return JS_TRUE;
  }

  if (argc >= 0) {

    if (NS_OK != nativeThis->UpdateEnabled(&nativeRet)) {
      return JS_FALSE;
    }

    *rval = BOOLEAN_TO_JSVAL(nativeRet);
  }
  else {
    JS_ReportError(cx, "Function UpdateEnabled requires 0 parameters");
    return JS_FALSE;
  }

  return JS_TRUE;
}

//
// Native method Install
//
PR_STATIC_CALLBACK(JSBool)
InstallTriggerGlobalInstall(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsIDOMInstallTriggerGlobal *nativeThis = (nsIDOMInstallTriggerGlobal*)JS_GetPrivate(cx, obj);

  *rval = JSVAL_FALSE;

  // If there's no private data this must be the prototype, so ignore
  if (nsnull == nativeThis) {
    return JS_TRUE;
  }

  // parse associative array of installs
  if ( argc >= 1 && JSVAL_IS_OBJECT(argv[0]) )
  {
    nsXPITriggerInfo *trigger = new nsXPITriggerInfo();
    if (!trigger)
      return JS_FALSE;

    JSIdArray *ida = JS_Enumerate( cx, JSVAL_TO_OBJECT(argv[0]) );
    if ( ida ) 
    {
      jsval v;
      PRUnichar *name, *URL;

      for (int i = 0; i < ida->length; i++ )
      {
        JS_IdToValue( cx, ida->vector[i], &v );
        name = JS_GetStringChars( JS_ValueToString( cx, v ) );

        JS_GetUCProperty( cx, JSVAL_TO_OBJECT(argv[0]), name, nsCRT::strlen(name), &v );
        URL = JS_GetStringChars( JS_ValueToString( cx, v ) );

        if ( name && URL )
        {
            nsXPITriggerItem *item = new nsXPITriggerItem( name, URL );
            if ( item )
                trigger->Add( item );
            else
                ; // XXX signal error somehow
        }
        else
            ; // XXX need to signal error
      }
      JS_DestroyIdArray( cx, ida );
    }

    // save callback function if any (ignore bad args for now)
    if ( argc >= 2 && JS_TypeOfValue(cx,argv[1]) == JSTYPE_FUNCTION )
    {
      trigger->SaveCallback( cx, argv[1] );
    }

    // pass on only if good stuff found
    if (trigger->Size() > 0)
    {
        PRBool result;
        nativeThis->Install(trigger,&result);
        *rval = BOOLEAN_TO_JSVAL(result);
        return JS_TRUE;
    }
    else
        delete trigger;
  }

  JS_ReportError(cx, "Incorrect arguments to InstallTrigger.Install()");
  return JS_FALSE;
}

//
// Native method StartSoftwareUpdate
//
PR_STATIC_CALLBACK(JSBool)
InstallTriggerGlobalStartSoftwareUpdate(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsIDOMInstallTriggerGlobal *nativeThis = (nsIDOMInstallTriggerGlobal*)JS_GetPrivate(cx, obj);
  PRInt32      nativeRet;
  nsAutoString b0;
  PRInt32      b1 = 0;

  *rval = JSVAL_FALSE;

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == nativeThis) {
    return JS_TRUE;
  }

  if ( argc >= 1 )
  {
    ConvertJSValToStr(b0, cx, argv[0]);

    if (argc >= 2 && !JS_ValueToInt32(cx, argv[1], (int32 *)&b1))
    {
        JS_ReportError(cx, "StartSoftwareUpdate() 2nd parameter must be a number");
        return JS_FALSE;
    }

    if(NS_OK != nativeThis->StartSoftwareUpdate(b0, b1, &nativeRet))
    {
      return JS_FALSE;
    }
    *rval = INT_TO_JSVAL(nativeRet);
  }
  else
  {
    JS_ReportError(cx, "Function StartSoftwareUpdate requires 2 parameters");
    return JS_FALSE;
  }

  return JS_TRUE;
}


//
// Native method ConditionalSoftwareUpdate
//
PR_STATIC_CALLBACK(JSBool)
InstallTriggerGlobalConditionalSoftwareUpdate(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsIDOMInstallTriggerGlobal *nativeThis = (nsIDOMInstallTriggerGlobal*)JS_GetPrivate(cx, obj);
  JSBool rBool = JS_FALSE;
  PRInt32 nativeRet;
  nsAutoString b0;
  nsAutoString b1;
  nsAutoString b2str;
  PRInt32      b2int;
  nsAutoString b3str;
  PRInt32      b3int;
  PRInt32      b4;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == nativeThis) {
    return JS_TRUE;
  }

  if(argc >= 5)
  {
    //  public int ConditionalSoftwareUpdate(String url,
    //                                       String registryName,
    //                                       int    diffLevel,
    //                                       String version, --OR-- VersionInfo version
    //                                       int    mode);

    ConvertJSValToStr(b0, cx, argv[0]);
    ConvertJSValToStr(b1, cx, argv[1]);

    if(!JS_ValueToInt32(cx, argv[2], (int32 *)&b2int))
    {
      JS_ReportError(cx, "3rd parameter must be a number");
      return JS_FALSE;
    }

    if(!JS_ValueToInt32(cx, argv[4], (int32 *)&b4))
    {
      JS_ReportError(cx, "5th parameter must be a number");
      return JS_FALSE;
    }
    
    if(JSVAL_IS_OBJECT(argv[3]))
    {
        JSObject* jsobj = JSVAL_TO_OBJECT(argv[3]);
        JSClass* jsclass = JS_GetClass(cx, jsobj);
        if((nsnull != jsclass) && (jsclass->flags & JSCLASS_HAS_PRIVATE)) 
        {
          nsIDOMInstallVersion* version = (nsIDOMInstallVersion*)JS_GetPrivate(cx, jsobj);

          if(NS_OK != nativeThis->ConditionalSoftwareUpdate(b0, b1, b2int, version, b4, &nativeRet))
          {
                return JS_FALSE;
          }
        }
    }
    else
    {
        ConvertJSValToStr(b3str, cx, argv[3]);
        if(NS_OK != nativeThis->ConditionalSoftwareUpdate(b0, b1, b2int, b3str, b4, &nativeRet))
        {
          return JS_FALSE;
        }
    }

    *rval = INT_TO_JSVAL(nativeRet);
  }
  else if(argc >= 4)
  {
    //  public int ConditionalSoftwareUpdate(String url,
    //                                       String registryName,
    //                                       String version,  --OR-- VersionInfo version
    //                                       int    mode);

    ConvertJSValToStr(b0, cx, argv[0]);
    ConvertJSValToStr(b1, cx, argv[1]);

    if(!JS_ValueToInt32(cx, argv[3], (int32 *)&b3int))
    {
      JS_ReportError(cx, "4th parameter must be a number");
      return JS_FALSE;
    }

    if(JSVAL_IS_OBJECT(argv[2]))
    {
        JSObject* jsobj = JSVAL_TO_OBJECT(argv[2]);
        JSClass* jsclass = JS_GetClass(cx, jsobj);
        if((nsnull != jsclass) && (jsclass->flags & JSCLASS_HAS_PRIVATE)) 
        {
          nsIDOMInstallVersion* version = (nsIDOMInstallVersion*)JS_GetPrivate(cx, jsobj);

          if(NS_OK != nativeThis->ConditionalSoftwareUpdate(b0, b1, version, b3int, &nativeRet))
          {
                return JS_FALSE;
          }
        }
    }
    else
    {
        ConvertJSValToStr(b2str, cx, argv[2]);
        if(NS_OK != nativeThis->ConditionalSoftwareUpdate(b0, b1, b2str, b3int, &nativeRet))
        {
          return JS_FALSE;
        }
    }
        
    *rval = INT_TO_JSVAL(nativeRet);
  }
  else if(argc >= 3)
  {
    //  public int ConditionalSoftwareUpdate(String url,
    //                                       String registryName,
    //                                       String version);  --OR-- VersionInfo version

    ConvertJSValToStr(b0, cx, argv[0]);
    ConvertJSValToStr(b1, cx, argv[1]);

    if(JSVAL_IS_OBJECT(argv[2]))
    {
        JSObject* jsobj = JSVAL_TO_OBJECT(argv[2]);
        JSClass* jsclass = JS_GetClass(cx, jsobj);
        if((nsnull != jsclass) && (jsclass->flags & JSCLASS_HAS_PRIVATE)) 
        {
          nsIDOMInstallVersion* version = (nsIDOMInstallVersion*)JS_GetPrivate(cx, jsobj);

          if(NS_OK != nativeThis->ConditionalSoftwareUpdate(b0, b1, version, &nativeRet))
          {
                return JS_FALSE;
          }
        }
    }
    else
    {
        ConvertJSValToStr(b2str, cx, argv[2]);
        if(NS_OK != nativeThis->ConditionalSoftwareUpdate(b0, b1, b2str, &nativeRet))
        {
          return JS_FALSE;
        }
    }
        
    *rval = INT_TO_JSVAL(nativeRet);
  }
  else
  {
    JS_ReportError(cx, "Function ConditionalSoftwareUpdate requires 5 parameters");
    return JS_FALSE;
  }

  return JS_TRUE;
}


//
// Native method CompareVersion
//
PR_STATIC_CALLBACK(JSBool)
InstallTriggerGlobalCompareVersion(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsIDOMInstallTriggerGlobal *nativeThis = (nsIDOMInstallTriggerGlobal*)JS_GetPrivate(cx, obj);
  JSBool rBool = JS_FALSE;
  PRInt32 nativeRet;
  nsAutoString b0;
  nsAutoString b1str;
  PRInt32      b1int;
  PRInt32      b2int;
  PRInt32      b3int;
  PRInt32      b4int;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if (nsnull == nativeThis) {
    return JS_TRUE;
  }

  if(argc >= 5)
  {
    //  public int CompareVersion(String registryName,
    //                            int    major,
    //                            int    minor,
    //                            int    release,
    //                            int    build);

    ConvertJSValToStr(b0, cx, argv[0]);

    if(!JS_ValueToInt32(cx, argv[1], (int32 *)&b1int))
    {
      JS_ReportError(cx, "2th parameter must be a number");
      return JS_FALSE;
    }
    if(!JS_ValueToInt32(cx, argv[2], (int32 *)&b2int))
    {
      JS_ReportError(cx, "3th parameter must be a number");
      return JS_FALSE;
    }
    if(!JS_ValueToInt32(cx, argv[3], (int32 *)&b3int))
    {
      JS_ReportError(cx, "4th parameter must be a number");
      return JS_FALSE;
    }
    if(!JS_ValueToInt32(cx, argv[4], (int32 *)&b4int))
    {
      JS_ReportError(cx, "5th parameter must be a number");
      return JS_FALSE;
    }

    if(NS_OK != nativeThis->CompareVersion(b0, b1int, b2int, b3int, b4int, &nativeRet))
    {
      return JS_FALSE;
    }

    *rval = INT_TO_JSVAL(nativeRet);
  }
  else if(argc >= 2)
  {
    //  public int CompareVersion(String registryName,
    //                            String version); --OR-- VersionInfo version

    ConvertJSValToStr(b0, cx, argv[0]);

    if(JSVAL_IS_OBJECT(argv[1]))
    {
        JSObject* jsobj = JSVAL_TO_OBJECT(argv[1]);
        JSClass* jsclass = JS_GetClass(cx, jsobj);
        if((nsnull != jsclass) && (jsclass->flags & JSCLASS_HAS_PRIVATE)) 
        {
          nsIDOMInstallVersion* version = (nsIDOMInstallVersion*)JS_GetPrivate(cx, jsobj);

          if(NS_OK != nativeThis->CompareVersion(b0, version, &nativeRet))
          {
                return JS_FALSE;
          }
        }
    }
    else
    {
        ConvertJSValToStr(b1str, cx, argv[1]);
        if(NS_OK != nativeThis->CompareVersion(b0, b1str, &nativeRet))
        {
          return JS_FALSE;
        }
    }

    *rval = INT_TO_JSVAL(nativeRet);
  }
  else
  {
    JS_ReportError(cx, "Function CompareVersion requires 5 parameters");
    return JS_FALSE;
  }

  return JS_TRUE;
}


/***********************************************************************/
//
// class for InstallTriggerGlobal
//
JSClass InstallTriggerGlobalClass = {
  "InstallTriggerGlobal", 
  JSCLASS_HAS_PRIVATE,
  JS_PropertyStub,
  JS_PropertyStub,
  GetInstallTriggerGlobalProperty,
  SetInstallTriggerGlobalProperty,
  EnumerateInstallTriggerGlobal,
  ResolveInstallTriggerGlobal,
  JS_ConvertStub,
  FinalizeInstallTriggerGlobal
};


//
// InstallTriggerGlobal class properties
//
static JSPropertySpec InstallTriggerGlobalProperties[] =
{
  {0}
};


//
// InstallTriggerGlobal class methods
//
static JSFunctionSpec InstallTriggerGlobalMethods[] = 
{
  {"UpdateEnabled",             InstallTriggerGlobalUpdateEnabled,              0},
  {"Install",                   InstallTriggerGlobalInstall,                    2},
  {"StartSoftwareUpdate",       InstallTriggerGlobalStartSoftwareUpdate,        2},
  {"ConditionalSoftwareUpdate", InstallTriggerGlobalConditionalSoftwareUpdate,  5},
  {"CompareVersion",            InstallTriggerGlobalCompareVersion,             5},
  {0}
};


//
// InstallTriggerGlobal constructor
//
PR_STATIC_CALLBACK(JSBool)
InstallTriggerGlobal(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  return JS_FALSE;
}


//
// InstallTriggerGlobal class initialization
//
nsresult NS_InitInstallTriggerGlobalClass(nsIScriptContext *aContext, void **aPrototype)
{
  JSContext *jscontext = (JSContext *)aContext->GetNativeContext();
  JSObject *proto = nsnull;
  JSObject *constructor = nsnull;
  JSObject *parent_proto = nsnull;
  JSObject *global = JS_GetGlobalObject(jscontext);
  jsval vp;

  if ((PR_TRUE != JS_LookupProperty(jscontext, global, "InstallTriggerGlobal", &vp)) ||
      !JSVAL_IS_OBJECT(vp) ||
      ((constructor = JSVAL_TO_OBJECT(vp)) == nsnull) ||
      (PR_TRUE != JS_LookupProperty(jscontext, JSVAL_TO_OBJECT(vp), "prototype", &vp)) || 
      !JSVAL_IS_OBJECT(vp)) {

    proto = JS_InitClass(jscontext,     // context
                         global,        // global object
                         parent_proto,  // parent proto 
                         &InstallTriggerGlobalClass,      // JSClass
                         InstallTriggerGlobal,            // JSNative ctor
                         0,             // ctor args
                         InstallTriggerGlobalProperties,  // proto props
                         InstallTriggerGlobalMethods,     // proto funcs
                         nsnull,        // ctor props (static)
                         nsnull);       // ctor funcs (static)
    if (nsnull == proto) {
      return NS_ERROR_FAILURE;
    }

    if ((PR_TRUE == JS_LookupProperty(jscontext, global, "InstallTriggerGlobal", &vp)) &&
        JSVAL_IS_OBJECT(vp) &&
        ((constructor = JSVAL_TO_OBJECT(vp)) != nsnull)) {
      vp = INT_TO_JSVAL(nsIDOMInstallTriggerGlobal::MAJOR_DIFF);
      JS_SetProperty(jscontext, constructor, "MAJOR_DIFF", &vp);

      vp = INT_TO_JSVAL(nsIDOMInstallTriggerGlobal::MINOR_DIFF);
      JS_SetProperty(jscontext, constructor, "MINOR_DIFF", &vp);

      vp = INT_TO_JSVAL(nsIDOMInstallTriggerGlobal::REL_DIFF);
      JS_SetProperty(jscontext, constructor, "REL_DIFF", &vp);

      vp = INT_TO_JSVAL(nsIDOMInstallTriggerGlobal::BLD_DIFF);
      JS_SetProperty(jscontext, constructor, "BLD_DIFF", &vp);

      vp = INT_TO_JSVAL(nsIDOMInstallTriggerGlobal::EQUAL);
      JS_SetProperty(jscontext, constructor, "EQUAL", &vp);

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
// Method for creating a new InstallTriggerGlobal JavaScript object
//
extern "C" NS_DOM nsresult NS_NewScriptInstallTriggerGlobal(nsIScriptContext *aContext, nsISupports *aSupports, nsISupports *aParent, void **aReturn)
{
  NS_PRECONDITION(nsnull != aContext && nsnull != aSupports && nsnull != aReturn, "null argument to NS_NewScriptInstallTriggerGlobal");
  JSObject *proto;
  JSObject *parent;
  nsIScriptObjectOwner *owner;
  JSContext *jscontext = (JSContext *)aContext->GetNativeContext();
  nsresult result = NS_OK;
  nsIDOMInstallTriggerGlobal *aInstallTriggerGlobal;

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

  if (NS_OK != NS_InitInstallTriggerGlobalClass(aContext, (void **)&proto)) {
    return NS_ERROR_FAILURE;
  }

  result = aSupports->QueryInterface(kIInstallTriggerGlobalIID, (void **)&aInstallTriggerGlobal);
  if (NS_OK != result) {
    return result;
  }

  // create a js object for this class
  *aReturn = JS_NewObject(jscontext, &InstallTriggerGlobalClass, proto, parent);
  if (nsnull != *aReturn) {
    // connect the native object to the js object
    JS_SetPrivate(jscontext, (JSObject *)*aReturn, aInstallTriggerGlobal);
  }
  else {
    NS_RELEASE(aInstallTriggerGlobal);
    return NS_ERROR_FAILURE; 
  }

  return NS_OK;
}
