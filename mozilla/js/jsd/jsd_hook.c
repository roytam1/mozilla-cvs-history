/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
 * JavaScript Debugging support - Hook support
 */

#include "jsd.h"

JSTrapStatus JS_DLL_CALLBACK
jsd_InterruptHandler(JSContext *cx, JSScript *script, jsbytecode *pc, jsval *rval,
                     void *closure)
{
    JSDScript*      jsdscript;
    JSDContext*     jsdc = (JSDContext*) closure;
    JSD_ExecutionHookProc hook;
    void*                 hookData;

    if( ! jsdc || ! jsdc->inited )
        return JSTRAP_CONTINUE;

    if( JSD_IS_DANGEROUS_THREAD(jsdc) )
        return JSTRAP_CONTINUE;

    jsd_JSContextUsed(jsdc, cx);

    JSD_LOCK_SCRIPTS(jsdc);
    jsdscript = jsd_FindJSDScript(jsdc, script);
    JSD_UNLOCK_SCRIPTS(jsdc);
    if( ! jsdscript )
        return JSTRAP_CONTINUE;

#ifdef LIVEWIRE
    if( ! jsdlw_UserCodeAtPC(jsdc, jsdscript, (jsuword)pc) )
        return JSTRAP_CONTINUE;
#endif

    /* local in case jsdc->interruptHook gets cleared on another thread */
    JSD_LOCK();
    hook     = jsdc->interruptHook;
    hookData = jsdc->interruptHookData;
    JSD_UNLOCK();

    return jsd_CallExecutionHook(jsdc, cx, JSD_HOOK_INTERRUPTED,
                                 hook, hookData, rval);
}

JSTrapStatus JS_DLL_CALLBACK
jsd_DebuggerHandler(JSContext *cx, JSScript *script, jsbytecode *pc,
                    jsval *rval, void *closure)
{
    JSDScript*      jsdscript;
    JSDContext*     jsdc = (JSDContext*) closure;
    JSD_ExecutionHookProc hook;
    void*                 hookData;

    if( ! jsdc || ! jsdc->inited )
        return JSTRAP_CONTINUE;

    if( JSD_IS_DANGEROUS_THREAD(jsdc) )
        return JSTRAP_CONTINUE;

    jsd_JSContextUsed(jsdc, cx);

    JSD_LOCK_SCRIPTS(jsdc);
    jsdscript = jsd_FindJSDScript(jsdc, script);
    JSD_UNLOCK_SCRIPTS(jsdc);
    if( ! jsdscript )
        return JSTRAP_CONTINUE;

    /* local in case jsdc->debuggerHook gets cleared on another thread */
    JSD_LOCK();
    hook     = jsdc->debuggerHook;
    hookData = jsdc->debuggerHookData;
    JSD_UNLOCK();

    return jsd_CallExecutionHook(jsdc, cx, JSD_HOOK_DEBUGGER_KEYWORD,
                                 hook, hookData, rval);
}

JSTrapStatus
jsd_CallExecutionHook(JSDContext* jsdc,
                      JSContext *cx,
                      uintN type,
                      JSD_ExecutionHookProc hook,
                      void* hookData,
                      jsval* rval)
{
    uintN hookanswer = JSD_HOOK_RETURN_CONTINUE;
    JSDThreadState* jsdthreadstate;

    if(hook && NULL != (jsdthreadstate = jsd_NewThreadState(jsdc,cx)))
    {
        hookanswer = hook(jsdc, jsdthreadstate, type, hookData, rval);
        jsd_DestroyThreadState(jsdc, jsdthreadstate);
    }

    switch(hookanswer)
    {
        case JSD_HOOK_RETURN_ABORT:
        case JSD_HOOK_RETURN_HOOK_ERROR:
            return JSTRAP_ERROR;
        case JSD_HOOK_RETURN_RET_WITH_VAL:
            return JSTRAP_RETURN;
        case JSD_HOOK_RETURN_CONTINUE:
            break;
        default:
            JS_ASSERT(0);
            break;
    }
    return JSTRAP_CONTINUE;
}

JSBool
jsd_SetInterruptHook(JSDContext*           jsdc,
                     JSD_ExecutionHookProc hook,
                     void*                 callerdata)
{
    JSD_LOCK();
    jsdc->interruptHookData  = callerdata;
    jsdc->interruptHook      = hook;
    JS_SetInterrupt(jsdc->jsrt, jsd_InterruptHandler, (void*) jsdc);
    JSD_UNLOCK();

    return JS_TRUE;
}

JSBool
jsd_ClearInterruptHook(JSDContext* jsdc)
{
    JSD_LOCK();
    JS_ClearInterrupt(jsdc->jsrt, NULL, NULL );
    jsdc->interruptHook      = NULL;
    JSD_UNLOCK();

    return JS_TRUE;
}

JSBool
jsd_SetDebugBreakHook(JSDContext*           jsdc,
                      JSD_ExecutionHookProc hook,
                      void*                 callerdata)
{
    JSD_LOCK();
    jsdc->debugBreakHookData  = callerdata;
    jsdc->debugBreakHook      = hook;
    JSD_UNLOCK();

    return JS_TRUE;
}

JSBool
jsd_ClearDebugBreakHook(JSDContext* jsdc)
{
    JSD_LOCK();
    jsdc->debugBreakHook      = NULL;
    JSD_UNLOCK();

    return JS_TRUE;
}

JSBool
jsd_SetDebuggerHook(JSDContext*           jsdc,
                      JSD_ExecutionHookProc hook,
                      void*                 callerdata)
{
    JSD_LOCK();
    jsdc->debuggerHookData  = callerdata;
    jsdc->debuggerHook      = hook;
    JSD_UNLOCK();

    return JS_TRUE;
}

JSBool
jsd_ClearDebuggerHook(JSDContext* jsdc)
{
    JSD_LOCK();
    jsdc->debuggerHook      = NULL;
    JSD_UNLOCK();

    return JS_TRUE;
}
