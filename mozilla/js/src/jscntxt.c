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
 * JS execution context.
 */
#include "jsstddef.h"
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "jstypes.h"
#ifndef NSPR20
#include "jsarena.h"
#else
/* Removed by JSIFY: #include "plarena.h"
 */
#include "jsarena.h" /* Added by JSIFY */
#endif
/* Removed by JSIFY: #include "prlog.h" */
#include "jsutil.h" /* Added by JSIFY */
#include "jsclist.h"
#include "jsprf.h"
#include "jsatom.h"
#include "jscntxt.h"
#include "jsconfig.h"
#include "jsdbgapi.h"
#include "jsexn.h"
#include "jsgc.h"
#include "jslock.h"
#include "jsobj.h"
#include "jsopcode.h"
#include "jsscan.h"
#include "jsscript.h"

JSContext *
js_NewContext(JSRuntime *rt, size_t stacksize)
{
    JSContext *cx;

    cx = malloc(sizeof *cx);
    if (!cx)
	return NULL;
    memset(cx, 0, sizeof *cx);

    cx->runtime = rt;
#ifdef JS_THREADSAFE
    js_InitContextForLocking(cx);
#endif
    if (rt->contextList.next == (JSCList *)&rt->contextList) {
	/* First context on this runtime: initialize atoms and keywords. */
	if (!js_InitAtomState(cx, &rt->atomState) ||
	    !js_InitScanner(cx)) {
	    free(cx);
	    return NULL;
	}
    }
    /* Atomicly append cx to rt's context list. */
    JS_LOCK_RUNTIME_VOID(rt, JS_APPEND_LINK(&cx->links, &rt->contextList));

    cx->version = JSVERSION_DEFAULT;
    cx->jsop_eq = JSOP_EQ;
    cx->jsop_ne = JSOP_NE;
    JS_InitArenaPool(&cx->stackPool, "stack", stacksize, sizeof(jsval));
    JS_InitArenaPool(&cx->codePool, "code", 1024, sizeof(jsbytecode));
    JS_InitArenaPool(&cx->tempPool, "temp", 1024, sizeof(jsdouble));

#if JS_HAS_REGEXPS
    if (!js_InitRegExpStatics(cx, &cx->regExpStatics)) {
	js_DestroyContext(cx);
	return NULL;
    }
#endif
    return cx;
}

void
js_DestroyContext(JSContext *cx)
{
    JSRuntime *rt;
    JSBool rtempty;

    rt = cx->runtime;

    /* Remove cx from context list first. */
    JS_LOCK_RUNTIME(rt);
    JS_REMOVE_LINK(&cx->links);
    rtempty = (rt->contextList.next == (JSCList *)&rt->contextList);
    JS_UNLOCK_RUNTIME(rt);

    if (rtempty) {
	/* Unpin all pinned atoms before final GC. */
	js_UnpinPinnedAtoms(&rt->atomState);

	/* Unlock GC things held by runtime pointers. */
	js_UnlockGCThing(cx, rt->jsNaN);
	js_UnlockGCThing(cx, rt->jsNegativeInfinity);
	js_UnlockGCThing(cx, rt->jsPositiveInfinity);
	js_UnlockGCThing(cx, rt->emptyString);

	/*
	 * Clear these so they get recreated if the standard classes are
	 * initialized again.
	 */
	rt->jsNaN = NULL;
	rt->jsNegativeInfinity = NULL;
	rt->jsPositiveInfinity = NULL;
	rt->emptyString = NULL;

	/* Clear debugging state to remove GC roots. */
	JS_ClearAllTraps(cx);
	JS_ClearAllWatchPoints(cx);
    }

    /* Remove more GC roots in regExpStatics, then collect garbage. */
#if JS_HAS_REGEXPS
    js_FreeRegExpStatics(cx, &cx->regExpStatics);
#endif
    js_ForceGC(cx);

    if (rtempty) {
	/* Free atom state now that we've run the GC. */
	js_FreeAtomState(cx, &rt->atomState);
    }

    /* Free the stuff hanging off of cx. */
    JS_FinishArenaPool(&cx->stackPool);
    JS_FinishArenaPool(&cx->codePool);
    JS_FinishArenaPool(&cx->tempPool);
    if (cx->lastMessage)
	free(cx->lastMessage);
    free(cx);
}

JSContext *
js_ContextIterator(JSRuntime *rt, JSContext **iterp)
{
    JSContext *cx = *iterp;

    JS_LOCK_RUNTIME(rt);
    if (!cx)
	cx = (JSContext *)rt->contextList.next;
    if ((void *)cx == &rt->contextList)
	cx = NULL;
    else
	*iterp = (JSContext *)cx->links.next;
    JS_UNLOCK_RUNTIME(rt);
    return cx;
}

void
js_ReportErrorVA(JSContext *cx, uintN flags, const char *format, va_list ap)
{
    JSStackFrame *fp;
    JSErrorReport report, *reportp;
    char *last;

    fp = cx->fp;
    if (fp && fp->script && fp->pc) {
	report.filename = fp->script->filename;
	report.lineno = js_PCToLineNumber(fp->script, fp->pc);
	/* XXX should fetch line somehow */
	report.linebuf = NULL;
	report.tokenptr = NULL;
	report.flags = flags;
	reportp = &report;
    } else {
	/* XXXshaver still fill out report here for flags? */
	reportp = NULL;
    }
    last = PR_vsmprintf(format, ap);
    if (!last)
	return;

    js_ReportErrorAgain(cx, last, reportp);
    free(last);
}

/*
 * The arguments from ap need to be packaged up into an array and stored
 * into the report struct.
 *
 * The format string addressed by the error number may contain operands
 * identified by the format {N}, where N is a decimal digit. Each of these
 * is to be replaced by the Nth argument from the va_list. The complete
 * message is placed into reportp->ucmessage converted to a JSString.
 *
 * returns true/false if the expansion succeeds (can fail for memory errors)
 */
JSBool
js_ExpandErrorArguments(JSContext *cx, JSErrorCallback callback,
			void *userRef, const uintN errorNumber,
			char **messagep, JSErrorReport *reportp, va_list ap)
{
    const JSErrorFormatString *fmtData;
    int i;
    int argCount;

    *messagep = NULL;
    if (callback) {
	fmtData = (*callback)(userRef, "Mountain View", errorNumber);
	if (fmtData != NULL) {
	    argCount = fmtData->argCount;
	    if (argCount > 0) {
		/*
		 * Gather the arguments into a char * array, the
		 * messageArgs field is supposed to be an array of
		 * JSString's and we'll convert them later.
		 */
		reportp->messageArgs = malloc(sizeof(char *) * argCount);
		if (!reportp->messageArgs)
		    return JS_FALSE;
		for (i = 0; i < argCount; i++)
		    reportp->messageArgs[i] = (JSString *) va_arg(ap, char *);
	    }
	    /*
	     * Parse the error format, substituting the argument X
	     * for {X} in the format.
	     */
	    if (argCount > 0) {
		if (fmtData->format) {
		    const char *fmt, *arg;
		    char *out;
		    int expandedArgs = 0;
		    int expandedLength
			= strlen(fmtData->format)
			  - (3 * argCount); /* exclude the {n} */

		    for (i = 0; i < argCount; i++) {
			expandedLength
			    += strlen((char *)reportp->messageArgs[i]);
		    }
		    *messagep = out = malloc(expandedLength + 1);
		    if (!out) {
			if (reportp->messageArgs) {
			    free(reportp->messageArgs);
			    reportp->messageArgs = NULL;
			}
			return JS_FALSE;
		    }
		    fmt = fmtData->format;
		    while (*fmt) {
			if (*fmt == '{') {	/* balance} */
			    if (isdigit(fmt[1])) {
				int d = JS7_UNDEC(fmt[1]);
				JS_ASSERT(expandedArgs < argCount);
				arg = (char *)reportp->messageArgs[d];
				strcpy(out, arg);
				out += strlen(arg);
				fmt += 3;
				expandedArgs++;
				continue;
			    }
			}
			*out++ = *fmt++;
		    }
		    JS_ASSERT(expandedArgs == argCount);
		    *out = '\0';
		}
		/*
		 * Now convert all the arguments to JSStrings.
		 */
		for (i = 0; i < argCount; i++) {
		    reportp->messageArgs[i] =
			JS_NewStringCopyZ(cx, (char *)reportp->messageArgs[i]);
		}
	    } else {
		*messagep = JS_strdup(cx, fmtData->format);
	    }
	    /*
	     * And finally convert the message.
	     */
	    reportp->ucmessage = JS_NewStringCopyZ(cx, *messagep);
	}
    }
    if (*messagep == NULL) {
	/* where's the right place for this ??? */
	const char *defaultErrorMessage
	    = "No error message available for error number %d";
	size_t nbytes = strlen(defaultErrorMessage) + 16;
	*messagep = (char *)malloc(nbytes);
	PR_snprintf(*messagep, nbytes, defaultErrorMessage, errorNumber);
    }
    return JS_TRUE;
}

void
js_ReportErrorNumberVA(JSContext *cx, uintN flags, JSErrorCallback callback,
			void *userRef, const uintN errorNumber, va_list ap)
{
    JSStackFrame *fp;
    JSErrorReport report;
    char *message;

    report.messageArgs = NULL;
    report.ucmessage = NULL;
    message = NULL;

    fp = cx->fp;
    if (fp && fp->script && fp->pc) {
	report.filename = fp->script->filename;
	report.lineno = js_PCToLineNumber(fp->script, fp->pc);
    } else {
	report.filename = NULL;
	report.lineno = 0;
    }

    /* XXX should fetch line somehow */
    report.linebuf = NULL;
    report.tokenptr = NULL;
    report.flags = flags;
    report.errorNumber = errorNumber;

    if (!js_ExpandErrorArguments(cx, callback, userRef, errorNumber,
				 &message, &report, ap))
	return;

#if JS_HAS_ERROR_EXCEPTIONS
    /*
     * Check the error report, and set a JavaScript-catchable exception
     * if the error is defined to have an associated exception.  If an
     * exception is thrown, then the JSREPORT_EXCEPTION flag will be set
     * on the error report, and exception-aware hosts should ignore it.
     */
    js_ErrorToException(cx, &report, message);
#endif

    js_ReportErrorAgain(cx, message, &report);

    if (message)
	free(message);
    if (report.messageArgs)
	free(report.messageArgs);
}

JS_FRIEND_API(void)
js_ReportErrorAgain(JSContext *cx, const char *message, JSErrorReport *reportp)
{
    JSErrorReporter onError;

    if (!message)
	return;
    if (cx->lastMessage)
	free(cx->lastMessage);
    cx->lastMessage = JS_strdup(cx, message);
    if (!cx->lastMessage)
	return;
    onError = cx->errorReporter;
    if (onError)
	(*onError)(cx, cx->lastMessage, reportp);
}

void
js_ReportIsNotDefined(JSContext *cx, const char *name)
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_NOT_DEFINED, name);
}

#if defined DEBUG && defined XP_UNIX
/* For gdb usage. */
void js_traceon(JSContext *cx)  { cx->tracefp = stderr; }
void js_traceoff(JSContext *cx) { cx->tracefp = NULL; }
#endif


JSErrorFormatString js_ErrorFormatString[JSErr_Limit] = {
#if JS_HAS_DFLT_MSG_STRINGS
#define MSG_DEF(name, number, count, exception, format) \
    { format, count } ,
#else
#define MSG_DEF(name, number, count, exception, format) \
    { NULL, count } ,
#endif
#include "js.msg"
#undef MSG_DEF
};

const JSErrorFormatString *
js_GetErrorMessage(void *userRef, const char *locale, const uintN errorNumber)
{
    if ((errorNumber > 0) && (errorNumber < JSErr_Limit))
	    return &js_ErrorFormatString[errorNumber];
	else
	    return NULL;
}

