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
 * JS execution context.
 */
#include "jsstddef.h"
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "jstypes.h"
#include "jsarena.h" /* Added by JSIFY */
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
    JSBool ok, first;

    cx = (JSContext *) malloc(sizeof *cx);
    if (!cx)
	return NULL;
    memset(cx, 0, sizeof *cx);

    cx->runtime = rt;
#ifdef JS_THREADSAFE
    js_InitContextForLocking(cx);
#endif

    JS_LOCK_RUNTIME(rt);
    for (;;) {
	first = (rt->contextList.next == (JSCList *)&rt->contextList);
	if (rt->state == JSRTS_UP) {
	    JS_ASSERT(!first);
	    break;
	}
	if (rt->state == JSRTS_DOWN) {
	    JS_ASSERT(first);
	    rt->state = JSRTS_LAUNCHING;
	    break;
	}
	JS_WAIT_CONDVAR(rt->stateChange, JS_NO_TIMEOUT);
    }
    JS_APPEND_LINK(&cx->links, &rt->contextList);
    JS_UNLOCK_RUNTIME(rt);

    if (first) {
	/* First context on this runtime: initialize atoms and keywords. */
	ok = js_InitAtomState(cx, &rt->atomState);
	if (ok)
	    ok = js_InitScanner(cx);

	JS_LOCK_RUNTIME(rt);
	rt->state = JSRTS_UP;
	JS_NOTIFY_ALL_CONDVAR(rt->stateChange);
	JS_UNLOCK_RUNTIME(rt);

	if (!ok) {
	    free(cx);
	    return NULL;
	}
    }

    cx->version = JSVERSION_DEFAULT;
    cx->jsop_eq = JSOP_EQ;
    cx->jsop_ne = JSOP_NE;
    JS_InitArenaPool(&cx->stackPool, "stack", stacksize, sizeof(jsval));
    JS_InitArenaPool(&cx->codePool, "code", 1024, sizeof(jsbytecode));
    JS_InitArenaPool(&cx->notePool, "note", 256, sizeof(jssrcnote));
    JS_InitArenaPool(&cx->tempPool, "temp", 1024, sizeof(jsdouble));

#if JS_HAS_REGEXPS
    if (!js_InitRegExpStatics(cx, &cx->regExpStatics)) {
	js_DestroyContext(cx, JS_FORCE_GC);
	return NULL;
    }
#endif
#if JS_HAS_EXCEPTIONS
    cx->throwing = JS_FALSE;
#endif

    return cx;
}

void
js_DestroyContext(JSContext *cx, JSGCMode gcmode)
{
    JSRuntime *rt;
    JSBool last;
    JSArgumentFormatMap *map;

    rt = cx->runtime;

    /* Remove cx from context list first. */
    JS_LOCK_RUNTIME(rt);
    JS_ASSERT(rt->state == JSRTS_UP);
    JS_REMOVE_LINK(&cx->links);
    last = (rt->contextList.next == (JSCList *)&rt->contextList);
    if (last)
    	rt->state = JSRTS_LANDING;
    JS_UNLOCK_RUNTIME(rt);

    if (last) {
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

    /* Used to avoid deadlock with a GC waiting for this request to end. */
#ifdef JS_THREADSAFE
    cx->destroying = JS_TRUE;
#endif

    if (gcmode == JS_FORCE_GC)
        js_ForceGC(cx);
    else if (gcmode == JS_MAYBE_GC)
        JS_MaybeGC(cx);

    if (last) {
        if (gcmode == JS_NO_GC)
            js_ForceGC(cx);

	/* Free atom state now that we've run the GC. */
	js_FreeAtomState(cx, &rt->atomState);
    }

    /* Free the stuff hanging off of cx. */
    JS_FinishArenaPool(&cx->stackPool);
    JS_FinishArenaPool(&cx->codePool);
    JS_FinishArenaPool(&cx->notePool);
    JS_FinishArenaPool(&cx->tempPool);
    if (cx->lastMessage)
	free(cx->lastMessage);

#ifdef JS_THREADSAFE
    /* Destroying a context implicitly calls JS_EndRequest(). */
    if (cx->requestDepth)
        JS_EndRequest(cx);
#endif

    /* remove any argument formatters */
    map = cx->argumentFormatMap;
    while (map) {
        JSArgumentFormatMap *temp = map;
        map = map->next;
        JS_free(cx, temp);
    }

    free(cx);

    if (last) {
	JS_LOCK_RUNTIME(rt);
    	rt->state = JSRTS_DOWN;
	JS_NOTIFY_ALL_CONDVAR(rt->stateChange);
	JS_UNLOCK_RUNTIME(rt);
    }
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

static void
ReportError(JSContext *cx, const char *message, JSErrorReport *reportp)
{
    /*
     * Check the error report, and set a JavaScript-catchable exception
     * if the error is defined to have an associated exception.  If an
     * exception is thrown, then the JSREPORT_EXCEPTION flag will be set
     * on the error report, and exception-aware hosts should ignore it.
     */
    if (reportp && reportp->errorNumber == JSMSG_UNCAUGHT_EXCEPTION)
        reportp->flags |= JSREPORT_EXCEPTION;

#if JS_HAS_ERROR_EXCEPTIONS
    /*
     * Call the error reporter only if an exception wasn't raised.
     *
     * If an exception was raised, then we call the debugErrorHook
     * (if present) to give it a chance to see the error before it
     * propagates out of scope.  This is needed for compatability
     * with the old scheme.
     */
    if (!js_ErrorToException(cx, message, reportp)) {
        js_ReportErrorAgain(cx, message, reportp);
    } else if (cx->runtime->debugErrorHook && cx->errorReporter) {
        JSDebugErrorHook hook = cx->runtime->debugErrorHook;
        /* test local in case debugErrorHook changed on another thread */
        if (hook)
            hook(cx, message, reportp, cx->runtime->debugErrorHookData);
    }
#else
    js_ReportErrorAgain(cx, message, reportp);
#endif
}

JSBool
js_ReportErrorVA(JSContext *cx, uintN flags, const char *format, va_list ap)
{
    JSStackFrame *fp;
    JSErrorReport report, *reportp;
    char *last;
    JSBool warning;

    fp = cx->fp;

    /* Walk stack until we find a frame that is associated with
       some script rather than a native frame. */
    while (fp && (!fp->script || !fp->pc))
        fp = fp->down;

    if (fp) {
	report.filename = fp->script->filename;
	report.lineno = js_PCToLineNumber(fp->script, fp->pc);
	/* XXX should fetch line somehow */
	report.linebuf = NULL;
	report.tokenptr = NULL;
        report.uclinebuf = NULL;
        report.uctokenptr = NULL;
	report.flags = flags;
        report.errorNumber = 0;
        report.ucmessage = NULL;
        report.messageArgs = NULL;
	reportp = &report;
    } else {
	/* XXXshaver still fill out report here for flags? */
	reportp = NULL;
    }
    last = JS_vsmprintf(format, ap);
    if (!last)
	return JS_FALSE;

    ReportError(cx, last, reportp);
    free(last);

    warning = JSREPORT_IS_WARNING(reportp->flags);
    if (warning && JS_HAS_WERROR_OPTION(cx)) {
        reportp->flags &= ~JSREPORT_WARNING;
        warning = JS_FALSE;
    }
    return warning;
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
 * Returns true if the expansion succeeds (can fail if out of memory).
 */
JSBool
js_ExpandErrorArguments(JSContext *cx, JSErrorCallback callback,
			void *userRef, const uintN errorNumber,
			char **messagep, JSErrorReport *reportp,
                        JSBool *warningp, JSBool charArgs, va_list ap)
{
    const JSErrorFormatString *fmtData;
    int i;
    int argCount;

    *warningp = JSREPORT_IS_WARNING(reportp->flags);
    if (*warningp && JS_HAS_WERROR_OPTION(cx)) {
        reportp->flags &= ~JSREPORT_WARNING;
        *warningp = JS_FALSE;
    }

    *messagep = NULL;
    if (callback) {
	fmtData = (*callback)(userRef, "Mountain View", errorNumber);
	if (fmtData != NULL) {
            size_t totalArgsLength = 0;
            size_t argLengths[10]; /* only {0} thru {9} supported */
	    argCount = fmtData->argCount;
            JS_ASSERT(argCount <= 10);
	    if (argCount > 0) {
		/*
                 * Gather the arguments into an array, and accumulate
                 * their sizes. We allocate 1 more than necessary and
                 * null it out to act as the caboose when we free the
                 * pointers later.
		 */
		reportp->messageArgs = (const jschar **)
                    JS_malloc(cx, sizeof(jschar *) * (argCount + 1));
		if (!reportp->messageArgs)
		    return JS_FALSE;
                reportp->messageArgs[argCount] = NULL;
                for (i = 0; i < argCount; i++) {
                    if (charArgs) {
                        char *charArg = va_arg(ap, char *);
		        reportp->messageArgs[i]
                            = js_InflateString(cx, charArg, strlen(charArg));
                    }
                    else
		        reportp->messageArgs[i] = va_arg(ap, jschar *);
                    argLengths[i] = js_strlen(reportp->messageArgs[i]);
                    totalArgsLength += argLengths[i];
                }
                /* NULL-terminate for easy copying. */
                reportp->messageArgs[i] = NULL;
	    }
	    /*
	     * Parse the error format, substituting the argument X
	     * for {X} in the format.
	     */
	    if (argCount > 0) {
		if (fmtData->format) {
		    const char *fmt;
                    const jschar *arg;
		    jschar *out;
		    int expandedArgs = 0;
		    size_t expandedLength
			= strlen(fmtData->format)
			  - (3 * argCount) /* exclude the {n} */
                          + totalArgsLength;
		    /*
		     * Note - the above calculation assumes that each argument
		     * is used once and only once in the expansion !!!
		     */
		    reportp->ucmessage = out = (jschar *)
                        JS_malloc(cx, (expandedLength + 1) * sizeof(jschar));
		    if (!out) {
			if (reportp->messageArgs) {
			    JS_free(cx, (void *)reportp->messageArgs);
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
				arg = reportp->messageArgs[d];
				js_strncpy(out, arg, argLengths[d]);
				out += argLengths[d];
				fmt += 3;
				expandedArgs++;
				continue;
			    }
			}
                        /*
                         * is this kosher?
                         */
			*out++ = (unsigned char)(*fmt++);
		    }
		    JS_ASSERT(expandedArgs == argCount);
		    *out = 0;
                    *messagep =
			js_DeflateString(cx, reportp->ucmessage,
					 (size_t)(out - reportp->ucmessage));
		}
            } else {
            	/*
            	 * Zero arguments: the format string (if it exists) is the
            	 * entire message.
            	 */
                if (fmtData->format) {
		    *messagep = JS_strdup(cx, fmtData->format);
                    reportp->ucmessage
                        = js_InflateString(cx, *messagep, strlen(*messagep));
                }
	    }
	}
    }
    if (*messagep == NULL) {
	/* where's the right place for this ??? */
	const char *defaultErrorMessage
	    = "No error message available for error number %d";
	size_t nbytes = strlen(defaultErrorMessage) + 16;
	*messagep = (char *)JS_malloc(cx, nbytes);
	JS_snprintf(*messagep, nbytes, defaultErrorMessage, errorNumber);
    }
    return JS_TRUE;
}

JSBool
js_ReportErrorNumberVA(JSContext *cx, uintN flags, JSErrorCallback callback,
                       void *userRef, const uintN errorNumber,
                       JSBool charArgs, va_list ap)
{
    JSStackFrame *fp;
    JSErrorReport report;
    char *message;
    JSBool warning;

    report.messageArgs = NULL;
    report.ucmessage = NULL;
    message = NULL;

    fp = cx->fp;
    if (fp && fp->script && fp->pc) {
	report.filename = fp->script->filename;
	report.lineno = js_PCToLineNumber(fp->script, fp->pc);
    } else {
        /*  We can't find out where the error was from the current
            frame so see if the next frame has a script/pc combo we
            could use */
        if (fp && fp->down && fp->down->script && fp->down->pc) {
	    report.filename = fp->down->script->filename;
	    report.lineno = js_PCToLineNumber(fp->down->script, fp->down->pc);
        }
        else {
	    report.filename = NULL;
	    report.lineno = 0;
        }
    }

    /* XXX should fetch line somehow */
    report.linebuf = NULL;
    report.tokenptr = NULL;
    report.flags = flags;
    report.errorNumber = errorNumber;

    /*
     * XXX js_ExpandErrorArguments only sometimes fills these in, so we
     * initialize them to clear garbage.
     */
    report.uclinebuf = NULL;
    report.uctokenptr = NULL;
    report.ucmessage = NULL;
    report.messageArgs = NULL;

    if (!js_ExpandErrorArguments(cx, callback, userRef, errorNumber,
				 &message, &report, &warning, charArgs, ap)) {
	return JS_FALSE;
    }

    ReportError(cx, message, &report);

    if (message)
	JS_free(cx, message);
    if (report.messageArgs) {
        int i = 0;
        while (report.messageArgs[i])
            JS_free(cx, (void *)report.messageArgs[i++]);
        JS_free(cx, (void *)report.messageArgs);
    }
    if (report.ucmessage)
        JS_free(cx, (void *)report.ucmessage);

    return warning;
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

    /*
     * If debugErrorHook is present then we give it a chance to veto
     * sending the error on to the regular ErrorReporter.
     */
    if (cx->runtime->debugErrorHook && onError) {
        JSDebugErrorHook hook = cx->runtime->debugErrorHook;
        /* test local in case debugErrorHook changed on another thread */
        if (hook && !hook(cx, message, reportp,
                          cx->runtime->debugErrorHookData)) {
            onError = NULL;
        }
    }
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
    return NULL;
}
