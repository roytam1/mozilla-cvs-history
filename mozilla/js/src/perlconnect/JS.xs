/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express oqr
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is Mozilla Communicator client code, released
 * March 31, 1998.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 *
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU Public License (the "GPL"), in which case the
 * provisions of the GPL are applicable instead of those above.
 * If you wish to allow use of your version of this file only
 * under the terms of the GPL and not to allow others to use your
 * version of this file under the NPL, indicate your decision by
 * deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL.  If you do not delete
 * the provisions above, a recipient may use your version of this
 * file under either the NPL or the GPL.
 */

/*
 * PerlConnect. Provides means for OO Perl <==> JS communications
 */

/* This is an program written in XSUB. You need to compile it using xsubpp   */
/* usually found in your perl\bin directory. On my machine I do it like this:*/
/*      perl c:\perl\lib\ExtUtils\xsubpp  -typemap  \                        */
/*           c:\perl\lib\extutils\typemap -typemap typemap JS.xs > JS.c      */
/* See perlxs man page for details.                                          */
/* Don't edit the resulting C file directly. See README.html for more info   */
/* on PerlConnect in general                                                 */

#ifdef __cplusplus
    extern "C"; {
#endif
#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"
#ifdef __cplusplus
    }
#endif

#include "../jsapi.h"
#include "jsperlpvt.h"
#include <malloc.h>

/* __REMOVE__ */
/* #include <stdio.h> */

static
JSClass global_class = {
    "Global", 0,
    JS_PropertyStub,  JS_PropertyStub,  JS_PropertyStub,  JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub,   JS_ConvertStub,   JS_FinalizeStub
};

/* __PH__BEGIN  */
/* perl callback structure */
/* prefix PCB means Perl CallBack */

struct PerlCallbackItem{
    char*  name;
    SV*    perl_proc;
    int    param_num;
    struct PerlCallbackItem *next;
};

typedef struct PerlCallbackItem PerlCallbackItem;


struct PerlObjectItem {
    char * name;
    SV* pObject;
    JSObject *jsObject;
    JSClass *jsClass;
    struct PerlCallbackItem* vector;
    struct PerlObjectItem *next;
};

typedef struct PerlObjectItem PerlObjectItem;

/* error reporter */
//struct JSContextItem;
//struct JSContextItem;
struct  JSContextItem {
    JSContext *cx;
    SV *errorReporter;
    PerlObjectItem *objects;
    int dieFromErrors;
    struct JSContextItem* next;
};

typedef struct JSContextItem JSContextItem;

static JSContextItem *context_list = NULL;

static JSContextItem*
PCB_NewContextItem() {
    JSContextItem *ret;
    ret = (JSContextItem*)calloc(1, sizeof(JSContextItem));
}

static JSContextItem*
PCB_FindContextItem (JSContext *cx) {
    JSContextItem *cxitem =  context_list;
    while ( cxitem ) {
        if (cxitem->cx == cx ) return cxitem;
        cxitem = cxitem->next;
    }
    return NULL;
}

static  SV*
PCB_FindErrorReporter (JSContext *cx) {
    JSContextItem *cxitem;
    if (cxitem = PCB_FindContextItem(cx)) {
        return cxitem->errorReporter;
    } else {
        return NULL;
    }
}

static void
PCB_ErrorReporter(JSContext *cx, const char *message, JSErrorReport *report)
{
    SV *report_proc;
    if ( report_proc = PCB_FindErrorReporter(cx) ) {
        dSP;
        PUSHMARK(SP);
        XPUSHs(sv_2mortal(newSVpv((char*)message, 0)));
        if ( report ) {
            XPUSHs(sv_2mortal(newSVpv((char*)report->filename, 0)));
            XPUSHs(sv_2mortal(newSViv(report->lineno)));
            if (report->linebuf) {
                XPUSHs(sv_2mortal(newSVpv((char*)report->linebuf, 0)));
                XPUSHs(sv_2mortal(newSVpv((char*)report->tokenptr, 0)));
            }
        }
        PUTBACK;
        perl_call_sv(report_proc, G_VOID | G_DISCARD);

    } else {
        warn(message);
    }
}

/* perl object stuff */


/* functions for callback list handling */
static PerlCallbackItem*
PCB_AddCallback(PerlObjectItem* object, char *name, 
                SV* perl_proc, int param_num) {
    PerlCallbackItem *cbk;

    cbk = (PerlCallbackItem*)calloc(1, sizeof(PerlCallbackItem));
    cbk->name = (char*) malloc(strlen(name) + 1);
    strcpy(cbk->name, name);
    SvREFCNT_inc(perl_proc);
    cbk->perl_proc = perl_proc;
    cbk->param_num = param_num;

    cbk->next = object->vector;
    object->vector = cbk;

    return cbk;
}

/* functions for perl object list handling */

static PerlObjectItem*
PCB_AddObject(char *name, SV *pobj, JSContext *cx, JSObject *jso, JSClass *class) {
    JSContextItem *cxitem;
    PerlObjectItem *object;

    /* we should always find the item */
    cxitem = PCB_FindContextItem(cx);
    object = (PerlObjectItem*) calloc(1, sizeof(PerlObjectItem));
    object->name = (char*) malloc(strlen(name) + 1);
    strcpy(object->name, name);
    SvREFCNT_inc(pobj);
    object->pObject = pobj;
    object->jsObject = jso;
    object->jsClass = class;
    object->next = cxitem->objects;
    cxitem->objects = object;

    return object;
}

static PerlObjectItem*
PCB_FindObject(JSContext *cx, JSObject *jso) {
    JSContextItem *cxitem;
    PerlObjectItem *objitem;

    cxitem = PCB_FindContextItem(cx);
    objitem = cxitem->objects;

    while ( objitem ) {
        if ( objitem->jsObject == jso ) return objitem;
        objitem = objitem->next;
    }
    return NULL;
}

static PerlCallbackItem*
PCB_FindCallback(PerlObjectItem *obj, const char *name) {
    PerlCallbackItem *cbkitem;

    cbkitem = obj->vector;
    while ( cbkitem ) {
        if ( strcmp(name, cbkitem->name) == 0 ) return cbkitem;
        cbkitem = cbkitem->next;
    }
    return NULL;
}

/* deletion functions */

static void 
PCB_FreeCallbackItem(PerlCallbackItem *callback) {
    free(callback->name);
    /* we have to decrease ref. count to proc */
    SvREFCNT_dec(callback->perl_proc);
    free(callback);
}

static void 
PCB_FreeObjectItem(PerlObjectItem *object) {
    PerlCallbackItem *cbkitem, *next;
    JSClass *class;

    free(object->name);
    free(object->jsClass);

    SvREFCNT_dec(object->pObject);
    cbkitem = object->vector;
    while ( cbkitem ) {
        next = cbkitem->next;
        PCB_FreeCallbackItem(cbkitem);
        cbkitem = next;
    }
    free(object);
}

static void 
PCB_FreeContextItem(JSContext * cx) {
    JSContextItem *cxitem, *aux;
    PerlObjectItem *objitem, *next;

    cxitem = PCB_FindContextItem(cx);
    objitem = cxitem->objects;

    while ( objitem ) {
        next = objitem->next;
        PCB_FreeObjectItem(objitem);
        objitem = next;
    }

    SvREFCNT_dec(cxitem->errorReporter);
    
    if ( context_list == cxitem ) {
        context_list = cxitem->next;
    } else {
        aux = context_list;
        while ( aux->next != cxitem ) aux = aux->next;
        aux->next = cxitem->next;
    }
    free(cxitem);
}

/* later the object list should be bind to JS Context
   in this case is needed to update destructor PerlFreeObjectList
*/

/* property getter and setter - cooperate with AUTOLOAD */

static JSBool
PCB_GetProperty(JSContext *cx, JSObject *obj, jsval name, jsval *rval) {
    PerlObjectItem *po;
    int i, cnt, len;
    I32 ax;
    SV *proc_sv;
    HV *stash;
    char prop_name[256];
    char full_name[256];
    char *foo;
    GV *gv;
    dSP;

    /* property name */
    strcpy(prop_name, JS_GetStringBytes(JSVAL_TO_STRING(name)));

    if (! (po = PCB_FindObject(cx, obj)))
        croak("Couldn't find stub for object");
    if ( (PCB_FindCallback(po, prop_name)))
        return(JS_TRUE);

    stash = SvSTASH(SvRV(po->pObject));
    /* strcpy(full_name, HvNAME(stash));
    strcat(full_name, "::");
    strcat(full_name, prop_name);

    proc_sv = sv_newmortal();
    sv_setpv(proc_sv, full_name); */
    /* start of perl call stuff */

    gv = gv_fetchmeth(stash, prop_name, strlen(prop_name), -1);

    ENTER;
    SAVETMPS;
    PUSHMARK(SP);
    XPUSHs(po->pObject); /* self for perl AUTOLOAD */
    PUTBACK;

    /* cnt = perl_call_sv(proc_sv, 0); */
    cnt = perl_call_sv((SV*)GvCV(gv), 0);    

    SPAGAIN;
    /* adjust stack for use of ST macro (see perlcall) */
    SP -= cnt;
    ax = (SP - PL_stack_base) + 1;

    /* read value(s) */
    if (cnt == 1) {
        SVToJSVAL(cx, obj, ST(0), rval);
    } else {
        warn("sorry, but array properties are not supported yet...");
    }
    PUTBACK;

    FREETMPS;
    LEAVE;

    return(JS_TRUE);
}

static JSBool
PCB_SetProperty(JSContext *cx, JSObject *obj, jsval name, jsval *rval) {
    PerlObjectItem *po;
    int i, cnt, len;
    I32 ax;
    SV *proc_sv, *value_sv;
    HV *stash;
    char prop_name[256];
    char full_name[256];
    char *foo;
    dSP;

    /* property name */
    strcpy(prop_name, JS_GetStringBytes(JSVAL_TO_STRING(name)));

    if (! (po = PCB_FindObject(cx, obj)))
        croak("Couldn't find stub for object");
    if ( (PCB_FindCallback(po, prop_name)))
        return(JS_TRUE);

    stash = SvSTASH(SvRV(po->pObject));
    strcpy(full_name, HvNAME(stash));
    strcat(full_name, "::");
    strcat(full_name, prop_name);

    proc_sv = sv_newmortal();
    sv_setpv(proc_sv, full_name);
    value_sv = sv_newmortal();
    JSVALToSV(cx, obj, *rval, &value_sv);
    /* start of perl call stuff */
    ENTER;
    SAVETMPS;
    PUSHMARK(SP);
    XPUSHs(po->pObject); /* self for perl AUTOLOAD */
    XPUSHs(value_sv);
    PUTBACK;

    cnt = perl_call_sv(proc_sv, 0);
    
    SPAGAIN;
    /* adjust stack for use of ST macro (see perlcall) */
    SP -= cnt;
    ax = (SP - PL_stack_base) + 1;

    /* read value(s) */
    if (cnt == 1) {
        SVToJSVAL(cx, obj, ST(0), rval);
    } else {
        warn("sorry, but array properties are not supported yet...");
    }
    PUTBACK;

    FREETMPS;
    LEAVE;

    return(JS_TRUE);
}

/* helper functions */ 
/* JSClass pointer is disposed by 
   JS engine during context cleanup _PH_ 
*/
static JSClass* 
PCB_NewStdJSClass(char *name) {	
    JSClass *class;

    class = (JSClass*)calloc(1, sizeof(JSClass));
    class->name = name;
    class->flags = JSCLASS_HAS_PRIVATE;
    class->addProperty = JS_PropertyStub;
    class->delProperty = JS_PropertyStub;  
    class->getProperty = PCB_GetProperty;  
    class->setProperty = PCB_SetProperty;
    class->enumerate = JS_EnumerateStub;
    class->resolve = JS_ResolveStub;
    class->convert = JS_ConvertStub;
    class->finalize = JS_FinalizeStub;
    return(class);
}

static JSBool
PCB_UniversalStub (JSContext *cx, JSObject *obj, uintN argc, 
                   jsval *argv, jsval *rval) {
    JSFunction *fun;
    PerlObjectItem *po;
    PerlCallbackItem *cbk;
    int i, cnt;
    I32 ax;
    SV* sv;
    dSP;

    fun = JS_ValueToFunction(cx, argv[-2]);
    if (! (po = PCB_FindObject(cx, obj)))
        croak("Couldn't find stub for object");
    if (! (cbk = PCB_FindCallback(po, JS_GetFunctionName(fun))))
        croak("Couldn't find perl callback");

    /* start of perl call stuff */
    ENTER;
    SAVETMPS;
    PUSHMARK(SP);
    XPUSHs(po->pObject); /* self for perl object method */
    for (i = 0; i < argc; i++) {
        sv = sv_newmortal();
        JSVALToSV(cx, obj, argv[i], &sv);
        XPUSHs(sv);
    }
    PUTBACK;

    cnt = perl_call_sv(SvRV(cbk->perl_proc), 0);
    
    SPAGAIN;
    /* adjust stack for use of ST macro (see perlcall) */
    SP -= cnt;
    ax = (SP - PL_stack_base) + 1;

    /* read value(s) */
    if (cnt == 1) {
        SVToJSVAL(cx, obj, ST(0), rval);
    } else {
        warn("sorry, but array results are not supported yet...");
    }
    PUTBACK;

    FREETMPS;
    LEAVE;

    return(JS_TRUE);
};

/* __PH__END */


/* Helper functions needed for most JS API routines */
/*
static JSRuntime *
getRuntime()
{
    return (JSRuntime *)SvIV((SV*)SvRV(perl_get_sv("JS::Runtime::rt", FALSE)));
}

static JSContext *
getContext()
{
    return (JSContext *)SvIV((SV*)SvRV(perl_get_sv("JS::Context::this", FALSE)));
}
*/ /* commented as obsolete by __PH__ */

/*
    The following packages are defined below:
    JS -- main container for all JS functionality
        JS::Runtime -- wrapper around JSRuntime *
        JS::Context -- wrapper around JSContext *
        JS::Object  -- wrapper around JSObject *
 */

MODULE = JS  PACKAGE = JS   PREFIX = JS_
PROTOTYPES:  DISABLE
 # package JS

 #   Most of the functions below have names coinsiding with those of the
 #   corresponding JS API functions. Thus, they are not commented.
JSRuntime *
JS_NewRuntime(maxbytes)
    int maxbytes
    OUTPUT:
    RETVAL

void
JS_DestroyRuntime(rt)
    JSRuntime *rt
    CODE:
    /*
        Make sure that the reference count to the runtime is zero.
        O.w. this sequence of commands will cause double-deallocation:
            $rt = new JS::Runtime(10_000);
            $rt1 = $rt;
            [exit here]
        So both $rt->DESTROY and $rt1->DESTROY will cause runtime destruction.

        _PH_ Thats not true, I guess. At least for Perl 5.
     */
     /* warn("===> before runtime check\n"); */
    if(SvREFCNT(ST(0)) == 1){
        /* warn("===> really runtime destroing"); */
        /* __PH__ */
        /*__PH__END */
        JS_DestroyRuntime(rt);
    }


 # package JS::Runtime
MODULE = JS    PACKAGE = JS::Runtime   PREFIX = JS_

JSContext *
JS_NewContext(rt, stacksize)
    JSRuntime *rt
    int        stacksize
    PREINIT:
    JSContextItem *cxitem;
    CODE:
    {
        JSObject *obj;
        /* jsval v; comment out unused var __PH__*/
        /* Here we are creating the globals object ourselves. */
        JSContext *cx;
        cx = JS_NewContext(rt, stacksize);
        cxitem = PCB_NewContextItem();
        cxitem->cx = cx;
        cxitem->next = context_list;
        context_list = cxitem;
        /* __PH__ set the error reporter */
        JS_SetErrorReporter(cx, PCB_ErrorReporter); 
        obj = JS_NewObject(cx, &global_class, NULL, NULL);
        JS_InitStandardClasses(cx, obj);
        RETVAL = cx;
    }
    OUTPUT:
    RETVAL

void
JS_DestroyContext(cx)
    JSContext *cx
    CODE:
    /* See the comment about ref. count above */
    /* warn("===> before context check\n"); */
    if(SvREFCNT(ST(0)) == 1){
        /* warn("===> really destroing context"); */
        JS_DestroyContext(cx);
        PCB_FreeContextItem(cx);
    }


 # package JS::Context
MODULE = JS    PACKAGE = JS::Context   PREFIX = JS_

jsval
JS_eval(cx, bytes)
    JSContext *cx
    char *bytes
    PREINIT:
    JSContextItem *cxitem;
    CODE:
    {
        jsval rval;
        /* Call on the global object */
        if(!JS_EvaluateScript(cx, JS_GetGlobalObject(cx), 
                              bytes, strlen(bytes), "Perl", 0, &rval)){
            cxitem = PCB_FindContextItem(cx);
            if (!cxitem || cxitem->dieFromErrors)
                croak("Perl eval failed");
            XSRETURN_UNDEF;
        }
        RETVAL = rval;
    }
    OUTPUT:
    RETVAL

# __PH__
void
JS_setErrorReporter(cx, reporter)
    JSContext *cx
    SV* reporter
    PREINIT:
    JSContextItem *cxitem;
    CODE:
    cxitem = PCB_FindContextItem(cx);
    SvREFCNT_inc(reporter);
    if ( cxitem ) cxitem->errorReporter = reporter;

void
JS_setDieFromErrors(cx, value)
     JSContext *cx
     int value
     PREINIT:
     JSContextItem *cxitem;
     CODE:
     cxitem = PCB_FindContextItem(cx);
     if ( cxitem ) cxitem->dieFromErrors = value;

void
JS_createObject(cx, object, name,  methods)
    JSContext *cx 
    SV *object
    char *name
    SV *methods
    PREINIT:
    JSObject *jso;
    HV *m_hash;
    I32 len;	
    HE *he;
    int i;
    PerlObjectItem *po;
    JSClass *object_class;
    PerlCallbackItem *pcbitem;
    CODE:
    if (SvTYPE(SvRV(methods)) != SVt_PVHV) {
        croak("Second parameter has to be HASHREF");
    }
    /* create js object in given context */
    object_class = PCB_NewStdJSClass(name);
    jso = JS_NewObject(cx, object_class, NULL, 0);
    if (!jso) croak("Unable create JS object");
    /* create callback info */
    po = PCB_AddObject(name, object, cx, jso, object_class);
    m_hash = (HV*)SvRV(methods);
    hv_iterinit(m_hash);
    while ((he = hv_iternext(m_hash))) {
	PCB_AddCallback(po, hv_iterkey(he, &len), hv_iterval(m_hash, he), 0);
    }
    /* set js object methods */
    /* HERE _PH_ */
    pcbitem = po->vector;
    while ( pcbitem ) {
        if (! JS_DefineFunction(cx, jso, pcbitem->name, 
                                PCB_UniversalStub, 0, 0)) 
            croak("Unable create JS function");
        pcbitem = pcbitem->next;
    }
/*    for (i = 0; i < po->count; i++) {
        if (! JS_DefineFunction(cx, jso, 
                           po->vector[i]->name,
                           PCB_UniversalStub,
                           0,0))
           croak("Unable create JS function\n");
           } */
    po->jsObject = JS_InitClass(cx, JS_GetGlobalObject(cx), jso, 
                object_class, 0, 0,
                NULL, NULL, NULL, NULL);


# __PH__END


 # package JS::Object
MODULE = JS    PACKAGE = JS::Object   PREFIX = JS_

 #
 #   The methods below get used when hash is tied.
 #
SV *
JS_TIEHASH(class, obj)
    char *class
    SV *obj
    PREINIT:
    JSContext* cx;
    CODE:
        RETVAL = obj;
    OUTPUT:
    RETVAL

SV *
JS_TIEARRAY(class, obj)
    char *class
    SV *obj
    PREINIT:
    JSContext* cx;
    CODE:
        RETVAL = obj;
    OUTPUT:
    RETVAL

jsval
JS_FETCH(obj, key)
    JSObject *obj
    char *key
    PREINIT:
    JSContext* cx;
    jsval rval;
    MAGIC *magic;
    CODE:
    {
        /* printf("in FETCH\n"); */
        magic = mg_find(SvRV(ST(0)), '~');
        if (magic) {
            cx = (JSContext *)SvIV(magic->mg_obj);
        } else {
            warn("Tied object has no magic\n");
        }
        JS_GetProperty(cx, obj, key, &rval);
        RETVAL = rval;
    }
    OUTPUT:
    RETVAL

int
JS_FETCHSIZE(obj)
    JSObject *obj
    PREINIT:
    JSContext* cx;
    MAGIC *magic;
    CODE:
    {
        /* printf("in FETCH\n"); */
        magic = mg_find(SvRV(ST(0)), '~');
        if (magic) {
            cx = (JSContext *)SvIV(magic->mg_obj);
        } else {
            warn("Tied object has no magic\n");
        }
        JS_GetArrayLength(cx, obj, &RETVAL);
    }
    OUTPUT:
    RETVAL

void
JS_STORE(obj, key, value)
    JSObject *obj
    char *key
    jsval value
    PREINIT:
    JSContext* cx;
    MAGIC *magic;
    CODE:
    {
        /* printf("In STORE\n"); */
        magic = mg_find(SvRV(ST(0)), '~');
        if (magic) {
            cx = (JSContext *)SvIV(magic->mg_obj);
        } else {
            warn("Tied object has no magic\n");
        }
        JS_SetProperty(cx, obj, key, &value);
    }

void
JS_DELETE(obj, key)
    JSObject *obj
    char *key
    PREINIT:
    JSContext* cx;
    MAGIC *magic;
    CODE:
    {
        /* printf("In DELETE\n"); */
        magic = mg_find(SvRV(ST(0)), '~');
        if (magic) {
            cx = (JSContext *)SvIV(magic->mg_obj);
        } else {
            warn("Tied object has no magic\n");
        }      
        JS_DeleteProperty(cx, obj, key);
    }

void
JS_CLEAR(obj)
    JSObject *obj
    PREINIT:
    JSContext* cx;
    MAGIC *magic;
    CODE:
    {
        /* printf("In CLEAR\n"); */
        magic = mg_find(SvRV(ST(0)), '~');
        if (magic) {
            cx = (JSContext *)SvIV(magic->mg_obj);
        } else {
            warn("Tied object has no magic\n");
        }
        JS_ClearScope(cx, obj);
    }

int
JS_EXISTS(obj, key)
    JSObject *obj
    char *key
    PREINIT:
    JSContext* cx;
    MAGIC *magic;
    CODE:
    {
        jsval v;
        /* printf("In EXISTS\n"); */
        magic = mg_find(SvRV(ST(0)), '~');
        if (magic) {
            cx = (JSContext *)SvIV(magic->mg_obj);
        } else {
            warn("Tied object has no magic\n");
        }
        JS_LookupProperty(cx, obj, key, &v);
        RETVAL = !JSVAL_IS_VOID(v);
    }
    OUTPUT:
    RETVAL


