/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
/*
 * This file is part of the Java-vendor-neutral implementation of LiveConnect
 *
 * It contains the implementation providing nsIFactory XP-COM interface.
 *
 */


#include <stdlib.h>
#include <string.h>
#include "prtypes.h"
#include "prprf.h"
#include "prlog.h"
PR_BEGIN_EXTERN_C

#ifdef XP_MAC
#include "prosdep.h"
#endif

#include "jsj_private.h"
#include "jsjava.h"

#include "jscntxt.h"        /* For js_ReportErrorAgain().
                               TODO - get rid of private header */

#include "netscape_javascript_JSObject.h"   /* javah-generated headers */


/* A captured JavaScript error, created when JS_ReportError() is called while
   running JavaScript code that is itself called from Java. */
struct CapturedJSError {
    char *              message;
    JSErrorReport       report;         /* Line # of error, etc. */
    jthrowable          java_exception; /* Java exception, error, or null */
    CapturedJSError *   next;                   /* Next oldest captured JS error */
};
PR_END_EXTERN_C

#include "nsCLiveconnect.h"

static NS_DEFINE_IID(kILiveconnectIID, NS_ILIVECONNECT_IID);
static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);


////////////////////////////////////////////////////////////////////////////
// from nsISupports and AggregatedQueryInterface:

// Thes macro expands to the aggregated query interface scheme.

NS_IMPL_AGGREGATED(nsCLiveconnect);

NS_METHOD
nsCLiveconnect::AggregatedQueryInterface(const nsIID& aIID, void** aInstancePtr)
{
    if (aIID.Equals(kISupportsIID)) {
      *aInstancePtr = GetInner();
      AddRef();
      return NS_OK;
    }
    if (aIID.Equals(kILiveconnectIID)) {
        *aInstancePtr = this;
        AddRef();
        return NS_OK;
    }
    return NS_NOINTERFACE;
}



////////////////////////////////////////////////////////////////////////////
// from nsILiveconnect:

/**
 * get member of a Native JSObject for a given name.
 *
 * @param jEnv       - JNIEnv on which the call is being made.
 * @param obj        - A Native JS Object.
 * @param name       - Name of a member.
 * @param pjobj      - return parameter as a java object representing 
 *                     the member. If it is a basic data type it is converted to
 *                     a corresponding java type. If it is a NJSObject, then it is
 *                     wrapped up as java wrapper netscape.javascript.JSObject.
 */
NS_METHOD	
nsCLiveconnect::GetMember(JNIEnv *jEnv, jsobject obj, const char *name, jobject *pjobj)
{
    JSContext         *cx             = NULL;
    JSJavaThreadState *jsj_env        = NULL;
    JSObject          *js_obj         = (JSObject *)obj;
    nsresult           err            = NS_OK;
    jobject            member         = NULL;
    jsval              js_val         = NULL;
    int                dummy_cost     = 0;
    JSBool             dummy_bool     = PR_FALSE;
    JavaToJSSavedState saved_state    = {NULL,NULL};

    if(jEnv == NULL)
    {
       return NS_ERROR_FAILURE;
    }
    jsj_env = jsj_enter_js(jEnv, NULL, &cx, NULL, &saved_state);
    if (!jsj_env)
        return NS_ERROR_FAILURE;

    if (!name) {
        JS_ReportError(cx, "illegal null member name");
        member = NULL;
        goto done;
    }

    
    if (!JS_GetProperty(cx, js_obj, name, &js_val))
        goto done;

    jsj_ConvertJSValueToJavaObject(cx, jEnv, js_val, jsj_get_jlObject_descriptor(cx, jEnv),
                                   &dummy_cost, &member, &dummy_bool);

done:
    if (!jsj_exit_js(cx, jsj_env, &saved_state))
        return NS_ERROR_FAILURE;
    
    *pjobj = member;

    return NS_OK;
}


/**
 * get member of a Native JSObject for a given index.
 *
 * @param jEnv       - JNIEnv on which the call is being made.
 * @param obj        - A Native JS Object.
 * @param index      - Index of a member.
 * @param pjobj      - return parameter as a java object representing 
 *                     the member. 
 */
NS_METHOD	
nsCLiveconnect::GetSlot(JNIEnv *jEnv, jsobject obj, int slot, jobject *pjobj)
{
    JSContext         *cx             = NULL;
    JSJavaThreadState *jsj_env        = NULL;
    JSObject          *js_obj         = (JSObject *)obj;
    nsresult           err            = NS_OK;
    jobject            member         = NULL;
    jsval              js_val         = NULL;
    int                dummy_cost     = 0;
    JSBool             dummy_bool     = PR_FALSE;
    JavaToJSSavedState saved_state    = {NULL,NULL};

    if(jEnv == NULL)
    {
       return NS_ERROR_FAILURE;
    }
    jsj_env = jsj_enter_js(jEnv, NULL, &cx, NULL, &saved_state);
    if (!jsj_env)
       return NS_ERROR_FAILURE;
    
    // =-= sudu: check to see if slot can be passed in as is.
    //           Should it be converted to a jsint?
    if (!JS_GetElement(cx, js_obj, slot, &js_val))
        goto done;
    if (!jsj_ConvertJSValueToJavaObject(cx, jEnv, js_val, jsj_get_jlObject_descriptor(cx, jEnv),
                                        &dummy_cost, &member, &dummy_bool))
        goto done;

done:
    if (!jsj_exit_js(cx, jsj_env, &saved_state))
       return NS_ERROR_FAILURE;
    
    *pjobj = member;
    return NS_OK;
}

/**
 * set member of a Native JSObject for a given name.
 *
 * @param jEnv       - JNIEnv on which the call is being made.
 * @param obj        - A Native JS Object.
 * @param name       - Name of a member.
 * @param jobj       - Value to set. If this is a basic data type, it is converted
 *                     using standard JNI calls but if it is a wrapper to a JSObject
 *                     then a internal mapping is consulted to convert to a NJSObject.
 */
NS_METHOD	
nsCLiveconnect::SetMember(JNIEnv *jEnv, jsobject obj, const char *name, jobject java_obj)
{
    JSContext         *cx             = NULL;
    JSJavaThreadState *jsj_env        = NULL;
    JSObject          *js_obj         = (JSObject *)obj;
    nsresult           err            = NS_OK;
    jobject            member         = NULL;
    jsval              js_val         = NULL;
    JavaToJSSavedState saved_state    = {NULL,NULL};

    if(jEnv == NULL)
    {
       return NS_ERROR_FAILURE;
    }
    
    jsj_env = jsj_enter_js(jEnv, NULL, &cx, NULL, &saved_state);
    if (!jsj_env)
        return NS_ERROR_FAILURE;
    
    if (!name) {
        JS_ReportError(cx, "illegal null member name");
        goto done;
    }

    if (!jsj_ConvertJavaObjectToJSValue(cx, jEnv, java_obj, &js_val))
        goto done;

    JS_SetProperty(cx, js_obj, name, &js_val);

done:
    jsj_exit_js(cx, jsj_env, &saved_state);
   return NS_OK;
}


/**
 * set member of a Native JSObject for a given index.
 *
 * @param jEnv       - JNIEnv on which the call is being made.
 * @param obj        - A Native JS Object.
 * @param index      - Index of a member.
 * @param jobj       - Value to set. If this is a basic data type, it is converted
 *                     using standard JNI calls but if it is a wrapper to a JSObject
 *                     then a internal mapping is consulted to convert to a NJSObject.
 */
NS_METHOD	
nsCLiveconnect::SetSlot(JNIEnv *jEnv, jsobject obj, int slot, jobject java_obj)
{
    JSContext         *cx             = NULL;
    JSJavaThreadState *jsj_env        = NULL;
    JSObject          *js_obj         = (JSObject *)obj;
    nsresult           err            = NS_OK;
    jsval              js_val         = NULL;
    JavaToJSSavedState saved_state    = {NULL,NULL};

    if(jEnv == NULL)
    {
       return NS_ERROR_FAILURE;
    }
    jsj_env = jsj_enter_js(jEnv, NULL, &cx, NULL, &saved_state);
    if (!jsj_env)
        return NS_ERROR_FAILURE;
    
    if (!jsj_ConvertJavaObjectToJSValue(cx, jEnv, java_obj, &js_val))
        goto done;
    JS_SetElement(cx, js_obj, slot, &js_val);

done:
    jsj_exit_js(cx, jsj_env, &saved_state);
    return NS_OK;
}


/**
 * remove member of a Native JSObject for a given name.
 *
 * @param jEnv       - JNIEnv on which the call is being made.
 * @param obj        - A Native JS Object.
 * @param name       - Name of a member.
 */
NS_METHOD	
nsCLiveconnect::RemoveMember(JNIEnv *jEnv, jsobject obj, const char *name)
{
    JSContext         *cx             = NULL;
    JSJavaThreadState *jsj_env        = NULL;
    JSObject          *js_obj         = (JSObject *)obj;
    nsresult           err            = NS_OK;
    jsval              js_val         = NULL;
    JavaToJSSavedState saved_state    = {NULL,NULL};

    if(jEnv == NULL)
    {
       return NS_ERROR_FAILURE;
    }
    jsj_env = jsj_enter_js(jEnv, NULL, &cx, NULL, &saved_state);
    if (!jsj_env)
        return NS_ERROR_FAILURE;
    
    if (!name) {
        JS_ReportError(cx, "illegal null member name");
        goto done;
    }
    JS_DeleteProperty2(cx, js_obj, name, &js_val);

done:
    jsj_exit_js(cx, jsj_env, &saved_state);
    return NS_OK;
}


/**
 * call a method of Native JSObject. 
 *
 * @param jEnv       - JNIEnv on which the call is being made.
 * @param obj        - A Native JS Object.
 * @param name       - Name of a method.
 * @param jobjArr    - Array of jobjects representing parameters of method being caled.
 * @param pjobj      - return value.
 */
NS_METHOD	
nsCLiveconnect::Call(JNIEnv *jEnv, jsobject obj, const char *name, jobjectArray java_args, jobject *pjobj)
{
    int                i              = 0;
    int                argc           = 0;
    int                arg_num        = 0;
    jsval             *argv           = 0;
    JSContext         *cx             = NULL;
    JSJavaThreadState *jsj_env        = NULL;
    JSObject          *js_obj         = (JSObject *)obj;
    nsresult           err            = NS_OK;
    jobject            member         = NULL;
    jsval              js_val         = NULL;
    jsval              function_val   = NULL;
    int                dummy_cost     = 0;
    JSBool             dummy_bool     = PR_FALSE;
    JavaToJSSavedState saved_state    = {NULL,NULL};
    jobject            result         = NULL;

    if(jEnv == NULL)
    {
       return NS_ERROR_FAILURE;
    }
    jsj_env = jsj_enter_js(jEnv, NULL, &cx, NULL, &saved_state);
    if (!jsj_env)
        return NS_ERROR_FAILURE;
    
    result = NULL;
    if (!name) {
        JS_ReportError(cx, "illegal null JavaScript function name");
        goto done;
    }

    
    /* FIXME: What about security stuff ? Don't principals need to be set here ? */

    /* Allocate space for JS arguments */
    if (java_args) {
        argc = jEnv->GetArrayLength(java_args);
        argv = (jsval*)JS_malloc(cx, argc * sizeof(jsval));
    } else {
        argc = 0;
        argv = 0;
    }

    /* Convert arguments from Java to JS values */
    for (arg_num = 0; arg_num < argc; arg_num++) {
        jobject arg = jEnv->GetObjectArrayElement(java_args, arg_num);

        if (!jsj_ConvertJavaObjectToJSValue(cx, jEnv, arg, &argv[arg_num]))
            goto cleanup_argv;
        JS_AddRoot(cx, &argv[arg_num]);
    }

    if (!JS_GetProperty(cx, js_obj, name, &function_val))
        goto cleanup_argv;

    if (!JS_CallFunctionValue(cx, js_obj, function_val, argc, argv, &js_val))
        goto cleanup_argv;

    jsj_ConvertJSValueToJavaObject(cx, jEnv, js_val, jsj_get_jlObject_descriptor(cx, jEnv),
                                   &dummy_cost, &result, &dummy_bool);

cleanup_argv:
    if (argv) {
        for (i = 0; i < arg_num; i++)
            JS_RemoveRoot(cx, &argv[i]);
        JS_free(cx, argv);
    }

done:
    if (!jsj_exit_js(cx, jsj_env, &saved_state))
        return NS_ERROR_FAILURE;
    
    *pjobj = result;

    return NS_OK;
}


/**
 * Evaluate a script with a Native JS Object representing scope.
 *
 * @param jEnv       - JNIEnv on which the call is being made.
 * @param obj                - A Native JS Object.
 * @param pNSIPrincipaArray  - Array of principals to be used to compare privileges.
 * @param numPrincipals      - Number of principals being passed.
 * @param script             - Script to be executed.
 * @param pjobj              - return value.
 */
NS_METHOD	
nsCLiveconnect::Eval(JNIEnv *jEnv, jsobject obj, const char *script, jobject *pjobj)
{
    JSContext         *cx             = NULL;
    JSJavaThreadState *jsj_env        = NULL;
    JSObject          *js_obj         = (JSObject *)obj;
    nsresult           err            = NS_OK;
    jobject            member         = NULL;
    jsval              js_val         = NULL;
    jsval              function_val   = NULL;
    int                dummy_cost     = 0;
    JSBool             dummy_bool     = PR_FALSE;
    JavaToJSSavedState saved_state    = {NULL,NULL};
    jobject            result         = NULL;
	   const char		      *codebase       = NULL;
    JSPrincipals      *principals     = NULL;
    JSBool             eval_succeeded = PR_FALSE;

    if(jEnv == NULL)
    {
       return NS_ERROR_FAILURE;
    }
    jsj_env = jsj_enter_js(jEnv, NULL, &cx, NULL, &saved_state);
    if (!jsj_env)
        return NULL;
    
    result = NULL;
    if (!script) {
        JS_ReportError(cx, "illegal null string eval argument");
        goto done;
    }

    
    /* Set up security stuff */
    principals = NULL;
    if (JSJ_callbacks->get_JSPrincipals_from_java_caller)
        principals = JSJ_callbacks->get_JSPrincipals_from_java_caller(jEnv, cx);
    codebase = principals ? principals->codebase : NULL;

    /* Have the JS engine evaluate the unicode string */
    eval_succeeded = JS_EvaluateScriptForPrincipals(cx, js_obj, principals,
                                                      script, strlen(script),
                                                      codebase, 0, &js_val);
    if (!eval_succeeded)
        goto done;

    /* Convert result to a subclass of java.lang.Object */
    jsj_ConvertJSValueToJavaObject(cx, jEnv, js_val, jsj_get_jlObject_descriptor(cx, jEnv),
                                   &dummy_cost, &result, &dummy_bool);

done:
    if (!jsj_exit_js(cx, jsj_env, &saved_state))
        return NULL;
    
    *pjobj = result;
    return NS_OK;
}


/**
 * Get the window object for a plugin instance.
 *
 * @param jEnv               - JNIEnv on which the call is being made.
 * @param pJavaObject        - Either a jobject or a pointer to a plugin instance 
 *                             representing the java object.
 * @param pjobj              - return value. This is a native js object 
 *                             representing the window object of a frame 
 *                             in which a applet/bean resides.
 */
NS_METHOD	
nsCLiveconnect::GetWindow(JNIEnv *jEnv, void *pJavaObject, jsobject *pobj)
{
    char              *err_msg        = NULL;
    JSContext         *cx             = NULL;
    JSObject          *js_obj         = NULL;
    jsval              js_val         = NULL;
    int                dummy_cost     = 0;
    JSBool             dummy_bool     = PR_FALSE;
    JavaToJSSavedState saved_state    = {NULL,NULL};
    jobject            java_obj         = NULL;
    JSJavaThreadState *jsj_env        = NULL;

    if(jEnv == NULL)
    {
       return NS_ERROR_FAILURE;
    }
    jsj_env = jsj_enter_js(jEnv, NULL, &cx, NULL, &saved_state);
    if (!jsj_env)
        return NULL;
    
    err_msg = NULL;
    java_obj = NULL;
    js_obj = JSJ_callbacks->map_java_object_to_js_object(jEnv, pJavaObject, &err_msg);
    if (!js_obj) {
        if (err_msg) {
            JS_ReportError(cx, err_msg);
            free(err_msg);
        }
        goto done;
    }
#if 0
    js_val = OBJECT_TO_JSVAL(js_obj);
    jsj_ConvertJSValueToJavaObject(cx, jEnv, js_val, jsj_get_jlObject_descriptor(cx, jEnv),
                                   &dummy_cost, &java_obj, &dummy_bool);
#endif
done:
    if (!jsj_exit_js(cx, jsj_env, &saved_state))
        return NULL;
    
    //*pjobj = java_obj;
    *pobj = (jint)js_obj;
    return NS_OK;
}

/**
 * Get the window object for a plugin instance.
 *
 * @param jEnv       - JNIEnv on which the call is being made.
 * @param obj        - A Native JS Object.
 */
NS_METHOD	
nsCLiveconnect::FinalizeJSObject(JNIEnv *jEnv, jsobject obj)
{
    JSContext         *cx             = NULL;
    JavaToJSSavedState saved_state    = {NULL,NULL};
    JSJavaThreadState *jsj_env        = NULL;
    JSObject          *js_obj         = (JSObject *)obj;
    
    if(jEnv == NULL)
    {
       return NS_ERROR_FAILURE;
    }
    jsj_env = jsj_enter_js(jEnv, NULL, &cx, NULL, &saved_state);
    if (!jsj_env)   /* Note: memory leak if we exit here */
        return NS_ERROR_FAILURE;
#ifdef PRESERVE_JSOBJECT_IDENTITY
    jsj_remove_js_obj_reflection_from_hashtable(cx, js_obj);
#endif  /* PRESERVE_JSOBJECT_IDENTITY */
    jsj_exit_js(cx, jsj_env, &saved_state);
    return NS_OK;
}



////////////////////////////////////////////////////////////////////////////
// from nsCLiveconnect:

nsCLiveconnect::nsCLiveconnect(nsISupports *aOuter)
{
    NS_INIT_AGGREGATED(aOuter);
    jsj_init_js_obj_reflections_table();
}

nsCLiveconnect::~nsCLiveconnect()
{
}

