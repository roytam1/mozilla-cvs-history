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

#include "jsapi.h"
#include "nscore.h"
#include "nsIScriptContext.h"

#include "nsString.h"
#include "nsInstall.h"
#include "nsWinReg.h"
#include "nsJSWinReg.h"

static void PR_CALLBACK WinRegCleanup(JSContext *cx, JSObject *obj);

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


static void PR_CALLBACK WinRegCleanup(JSContext *cx, JSObject *obj)
{
    nsWinReg *nativeThis = (nsWinReg*)JS_GetPrivate(cx, obj);
    delete nativeThis;
}

/***********************************************************************************/
// Native mothods for WinReg functions

//
// Native method SetRootKey
//
PR_STATIC_CALLBACK(JSBool)
WinRegSetRootKey(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsWinReg *nativeThis  = (nsWinReg*)JS_GetPrivate(cx, obj);
  JSBool   rBool        = JS_FALSE;
  PRInt32  b0;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if(nsnull == nativeThis)
  {
    return JS_TRUE;
  }

  if(argc >= 1)
  {
    //  public int setRootKey(PRInt32 key);

    if(!JS_ValueToInt32(cx, argv[0], (int32 *)&b0))
    {
      JS_ReportError(cx, "Parameter must be a number");
      return JS_FALSE;
    }

    if(NS_OK != nativeThis->setRootKey(b0))
    {
      return JS_FALSE;
    }

    *rval = JSVAL_VOID;
  }
  else
  {
    JS_ReportError(cx, "Function SetRootKey requires 1 parameters");
    return JS_FALSE;
  }

  return JS_TRUE;
}


//
// Native method CreateKey
//
PR_STATIC_CALLBACK(JSBool)
WinRegCreateKey(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsWinReg *nativeThis = (nsWinReg*)JS_GetPrivate(cx, obj);
  PRInt32 nativeRet;
  nsAutoString b0;
  nsAutoString b1;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if(nsnull == nativeThis)
  {
    return JS_TRUE;
  }

  if(argc >= 2)                             
  {
    //  public int createKey ( String subKey,
    //                         String className);

    ConvertJSValToStr(b0, cx, argv[0]);
    ConvertJSValToStr(b1, cx, argv[1]);

    if(NS_OK != nativeThis->createKey(b0, b1, &nativeRet))
    {
      return JS_FALSE;
    }

    *rval = INT_TO_JSVAL(nativeRet);
  }
  else
  {
    JS_ReportError(cx, "WinReg.CreateKey() parameters error");
    return JS_FALSE;
  }

  return JS_TRUE;
}

//
// Native method DeleteKey
//
PR_STATIC_CALLBACK(JSBool)
WinRegDeleteKey(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsWinReg *nativeThis = (nsWinReg*)JS_GetPrivate(cx, obj);
  PRInt32 nativeRet;
  nsAutoString b0;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if(nsnull == nativeThis)
  {
    return JS_TRUE;
  }

  if(argc >= 1)                             
  {
    //  public int deleteKey ( String subKey);

    ConvertJSValToStr(b0, cx, argv[0]);

    if(NS_OK != nativeThis->deleteKey(b0, &nativeRet))
    {
      return JS_FALSE;
    }

    *rval = INT_TO_JSVAL(nativeRet);
  }
  else
  {
    JS_ReportError(cx, "WinReg.DeleteKey() parameters error");
    return JS_FALSE;
  }

  return JS_TRUE;
}


//
// Native method DeleteValue
//
PR_STATIC_CALLBACK(JSBool)
WinRegDeleteValue(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsWinReg *nativeThis = (nsWinReg*)JS_GetPrivate(cx, obj);
  PRInt32 nativeRet;
  nsString b0;
  nsString b1;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if(nsnull == nativeThis)
  {
    return JS_TRUE;
  }

  if(argc >= 2)                             
  {
    //  public int deleteValue ( String subKey,
    //                           String valueName);

    ConvertJSValToStr(b0, cx, argv[0]);
    ConvertJSValToStr(b1, cx, argv[1]);

    if(NS_OK != nativeThis->deleteValue(b0, b1, &nativeRet))
    {
      return JS_FALSE;
    }

    *rval = INT_TO_JSVAL(nativeRet);
  }
  else
  {
    JS_ReportError(cx, "WinReg.DeleteValue() parameters error");
    return JS_FALSE;
  }

  return JS_TRUE;
}

//
// Native method SetValueString
//
PR_STATIC_CALLBACK(JSBool)
WinRegSetValueString(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsWinReg *nativeThis = (nsWinReg*)JS_GetPrivate(cx, obj);
  PRInt32 nativeRet;
  nsAutoString b0;
  nsAutoString b1;
  nsAutoString b2;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if(nsnull == nativeThis)
  {
    return JS_TRUE;
  }

  if(argc >= 3)
  {
    //  public int setValueString ( String subKey,
    //                              String valueName,
    //                              String value);

    ConvertJSValToStr(b0, cx, argv[0]);
    ConvertJSValToStr(b1, cx, argv[1]);
    ConvertJSValToStr(b2, cx, argv[2]);

    if(NS_OK != nativeThis->setValueString(b0, b1, b2, &nativeRet))
    {
      return JS_FALSE;
    }

    *rval = INT_TO_JSVAL(nativeRet);
  }
  else
  {
    JS_ReportError(cx, "WinReg.SetValueString() parameters error");
    return JS_FALSE;
  }

  return JS_TRUE;
}

//
// Native method GetValueString
//
PR_STATIC_CALLBACK(JSBool)
WinRegGetValueString(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsWinReg *nativeThis = (nsWinReg*)JS_GetPrivate(cx, obj);
  nsString* nativeRet;
  nsAutoString b0;
  nsAutoString b1;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if(nsnull == nativeThis)
  {
    return JS_TRUE;
  }

  if(argc >= 2)                             
  {
    //  public int getValueString ( String subKey,
    //                              String valueName);

    ConvertJSValToStr(b0, cx, argv[0]);
    ConvertJSValToStr(b1, cx, argv[1]);

    if(NS_OK != nativeThis->getValueString(b0, b1, &nativeRet))
    {
      return JS_FALSE;
    }

    *rval = INT_TO_JSVAL(nativeRet);
  }
  else
  {
    JS_ReportError(cx, "WinReg.GetValueString() parameters error");
    return JS_FALSE;
  }

  return JS_TRUE;
}

//
// Native method SetValue
//
PR_STATIC_CALLBACK(JSBool)
WinRegSetValue(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsWinReg *nativeThis = (nsWinReg*)JS_GetPrivate(cx, obj);
  PRInt32 nativeRet;
  nsAutoString b0;
  nsAutoString b1;
//  nsWinRegItem *b2;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if(nsnull == nativeThis)
  {
    return JS_TRUE;
  }

  if(argc >= 3)
  {
    //  public int setValue ( String        subKey,
    //                        String        valueName,
    //                        nsWinRegItem  *value);

    ConvertJSValToStr(b0, cx, argv[0]);
    ConvertJSValToStr(b1, cx, argv[1]);

    // fix: this parameter is an object, not a string.
    // A way needs to be figured out to convert the JSVAL to this object type
//    ConvertJSValToStr(b2, cx, argv[2]);

//    if(NS_OK != nativeThis->setValue(b0, b1, b2, &nativeRet))
//    {
//      return JS_FALSE;
//    }

    *rval = INT_TO_JSVAL(nativeRet);
  }
  else
  {
    JS_ReportError(cx, "WinReg.SetValue() parameters error");
    return JS_FALSE;
  }

  return JS_TRUE;
}

//
// Native method GetValue
//
PR_STATIC_CALLBACK(JSBool)
WinRegGetValue(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsWinReg      *nativeThis = (nsWinReg*)JS_GetPrivate(cx, obj);
  nsWinRegValue *nativeRet;
  nsAutoString  b0;
  nsAutoString  b1;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if(nsnull == nativeThis)
  {
    return JS_TRUE;
  }

  if(argc >= 2)                             
  {
    //  public int getValue ( String subKey,
    //                        String valueName);

    ConvertJSValToStr(b0, cx, argv[0]);
    ConvertJSValToStr(b1, cx, argv[1]);

    if(NS_OK != nativeThis->getValue(b0, b1, &nativeRet))
    {
      return JS_FALSE;
    }

    *rval = INT_TO_JSVAL(nativeRet);
  }
  else
  {
    JS_ReportError(cx, "WinReg.GetValue() parameters error");
    return JS_FALSE;
  }

  return JS_TRUE;
}

//
// Native method InstallObject
//
PR_STATIC_CALLBACK(JSBool)
WinRegInstallObject(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsWinReg     *nativeThis = (nsWinReg*)JS_GetPrivate(cx, obj);
  nsInstall    *nativeRet;
  nsAutoString b0;
  nsAutoString b1;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if(nsnull == nativeThis)
  {
    return JS_TRUE;
  }

  //  public int installObject ();

  nativeRet = nativeThis->installObject();

  *rval = INT_TO_JSVAL(nativeRet);
  return JS_TRUE;
}

//
// WinReg constructor
//
PR_STATIC_CALLBACK(JSBool)
WinReg(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  return JS_FALSE;
}


/***********************************************************************/
//
// class for WinReg
//
JSClass WinRegClass = {
  "WinReg",
  JSCLASS_HAS_PRIVATE,
  JS_PropertyStub,
  JS_PropertyStub,
  JS_PropertyStub,
  JS_PropertyStub,
  JS_EnumerateStub,
  JS_ResolveStub,
  JS_ConvertStub,
  WinRegCleanup
};

static JSConstDoubleSpec winreg_constants[] = 
{
    { nsWinReg::HKEY_CLASSES_ROOT,           "HKEY_CLASSES_ROOT"            },
    { nsWinReg::HKEY_CURRENT_USER,           "HKEY_CURRENT_USER"            },
    { nsWinReg::HKEY_LOCAL_MACHINE,          "HKEY_LOCAL_MACHINE"           },
    { nsWinReg::HKEY_USERS,                  "HKEY_USERS"                   },
    {0}
};

//
// WinReg class methods
//
static JSFunctionSpec WinRegMethods[] = 
{
  {"setRootKey",                WinRegSetRootKey,               1},
  {"createKey",                 WinRegCreateKey,                2},
  {"deleteKey",                 WinRegDeleteKey,                1},
  {"deleteValue",               WinRegDeleteValue,              2},
  {"setValueString",            WinRegSetValueString,           3},
  {"getValueString",            WinRegGetValueString,           2},
  {"setValue",                  WinRegSetValue,                 3},
  {"getValue",                  WinRegGetValue,                 2},
  {"installObject",             WinRegInstallObject,            0},
  {0}
};

PRInt32
InitWinRegPrototype(JSContext *jscontext, JSObject *global, JSObject **winRegPrototype)
{
  *winRegPrototype = JS_InitClass( jscontext,         // context
                                  global,            // global object
                                  nsnull,            // parent proto 
                                  &WinRegClass,      // JSClass
                                  nsnull,            // JSNative ctor
                                  0,                 // ctor args
                                  nsnull,            // proto props
                                  nsnull,            // proto funcs
                                  nsnull,            // ctor props (static)
                                  WinRegMethods);    // ctor funcs (static)

  if(nsnull == *winRegPrototype) 
  {
    return NS_ERROR_FAILURE;
  }

  if(PR_FALSE == JS_DefineConstDoubles(jscontext, *winRegPrototype, winreg_constants))
    return NS_ERROR_FAILURE;

  return NS_OK;
}
