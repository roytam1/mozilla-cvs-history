/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
 *   Pierre Phaneuf <pp@ludusdesign.com>
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

/* XPConnect JavaScript interactive shell. */

#include <stdio.h>
#include "nsIXPConnect.h"
#include "nsIXPCScriptable.h"
#include "nsIInterfaceInfo.h"
#include "nsIInterfaceInfoManager.h"
#include "nsIXPCScriptable.h"
#include "nsIServiceManager.h"
#include "nsIComponentManager.h"
#include "jsapi.h"
#include "jsprf.h"
//#include "xpclog.h"
#include "nscore.h"
#include "nsIAllocator.h"
#include "nsIGenericFactory.h"
#include "nsIJSRuntimeService.h"
#include "nsCOMPtr.h"
#include "nsIXPCSecurityManager.h"

// all this crap is needed to do the interactive shell stuff
#include <stdlib.h>
#include <errno.h>
#ifdef XP_PC
#include <io.h>     /* for isatty() */
#else
#if defined(XP_UNIX) || defined(XP_BEOS)
#include <unistd.h>     /* for isatty() */
#endif
#endif
#include "jsparse.h"
#include "jsscan.h"
#include "jsemit.h"
#include "jsscript.h"
#include "jsarena.h"
#include "jscntxt.h"

#include "nsSpecialSystemDirectory.h"	// For exe dir

#include "nsIJSContextStack.h"

/***************************************************************************/

static void SetupRegistry()
{
    nsComponentManager::AutoRegister(nsIComponentManager::NS_Startup, nsnull);
}

static nsIXPConnect* GetXPConnect()
{
    nsIXPConnect* result;

    if(NS_SUCCEEDED(nsServiceManager::GetService(
                        nsIXPConnect::GetCID(), NS_GET_IID(nsIXPConnect),
                        (nsISupports**) &result, NULL)))
        return result;
    return NULL;
}

/***************************************************************************/

#define EXITCODE_RUNTIME_ERROR 3
#define EXITCODE_FILE_NOT_FOUND 4

FILE *gOutFile = NULL;
FILE *gErrFile = NULL;

int gExitCode = 0;
JSBool gQuitting = JS_FALSE;

static void
my_ErrorReporter(JSContext *cx, const char *message, JSErrorReport *report)
{
    int i, j, k, n;
    char *prefix = NULL, *tmp;
    const char *ctmp;

    if (!report) {
        fprintf(gErrFile, "%s\n", message);
        return;
    }

    if (report->filename)
        prefix = JS_smprintf("%s:", report->filename);
    if (report->lineno) {
        tmp = prefix;
        prefix = JS_smprintf("%s%u: ", tmp ? tmp : "", report->lineno);
        JS_free(cx, tmp);
    }
    if (JSREPORT_IS_WARNING(report->flags)) {
        tmp = prefix;
        prefix = JS_smprintf("%swarning: ", tmp ? tmp : "");
        JS_free(cx, tmp);
    }

    /* embedded newlines -- argh! */
    while ((ctmp = strchr(message, '\n')) != 0) {
        ctmp++;
        if (prefix) fputs(prefix, gErrFile);
        fwrite(message, 1, ctmp - message, gErrFile);
        message = ctmp;
    }
    /* If there were no filename or lineno, the prefix might be empty */
    if (prefix)
        fputs(prefix, gErrFile);
    fputs(message, gErrFile);

    if (!report->linebuf) {
        fputc('\n', gErrFile);
        goto out;
    }

    fprintf(gErrFile, ":\n%s%s\n%s", prefix, report->linebuf, prefix);
    n = report->tokenptr - report->linebuf;
    for (i = j = 0; i < n; i++) {
        if (report->linebuf[i] == '\t') {
            for (k = (j + 8) & ~7; j < k; j++) {
                fputc('.', gErrFile);
            }
            continue;
        }
        fputc('.', gErrFile);
        j++;
    }
    fputs("^\n", gErrFile);
 out:
    gExitCode = EXITCODE_RUNTIME_ERROR;
    JS_free(cx, prefix);
}
/*
static void
my_ErrorReporter(JSContext *cx, const char *message, JSErrorReport *report)
{
    printf("%s\n", message);
}
*/

static JSBool
Print(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    uintN i, n;
    JSString *str;

    for (i = n = 0; i < argc; i++) {
        str = JS_ValueToString(cx, argv[i]);
        if (!str)
            return JS_FALSE;
        fprintf(gOutFile, "%s%s", i ? " " : "", JS_GetStringBytes(str));
    }
    n++;
    if (n)
        fputc('\n', gOutFile);
    return JS_TRUE;
}

static JSBool
Dump(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSString *str;
    if (!argc)
        return JS_TRUE;
    
    str = JS_ValueToString(cx, argv[0]);
    if (!str)
        return JS_FALSE;

    char *bytes = JS_GetStringBytes(str);
    bytes = nsCRT::strdup(bytes);

#ifdef XP_MAC
    for (char *c = bytes; *c; c++)
        if (*c == '\r')
            *c = '\n';
#endif
    fputs(bytes, gOutFile);
    nsAllocator::Free(bytes);
    return JS_TRUE;
}

static JSBool
Load(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    uintN i;
    JSString *str;
    const char *filename;
    JSScript *script;
    JSBool ok;
    jsval result;

    for (i = 0; i < argc; i++) {
        str = JS_ValueToString(cx, argv[i]);
        if (!str)
            return JS_FALSE;
        argv[i] = STRING_TO_JSVAL(str);
        filename = JS_GetStringBytes(str);
        script = JS_CompileFile(cx, obj, filename);
        if (!script)
            ok = JS_FALSE;
        else {
            ok = JS_ExecuteScript(cx, obj, script, &result);
            JS_DestroyScript(cx, script);
        }
        if (!ok)
            return JS_FALSE;
    }
    return JS_TRUE;
}

static JSBool
Version(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    if (argc > 0 && JSVAL_IS_INT(argv[0]))
        *rval = INT_TO_JSVAL(JS_SetVersion(cx, JSVersion(JSVAL_TO_INT(argv[0]))));
    else
        *rval = INT_TO_JSVAL(JS_GetVersion(cx));
    return JS_TRUE;
}

static JSBool
BuildDate(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    fprintf(gOutFile, "built on %s at %s\n", __DATE__, __TIME__);
    return JS_TRUE;
}

static JSBool
Quit(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
#ifdef LIVECONNECT
    JSJ_SimpleShutdown();
#endif

    gExitCode = 0;
    JS_ConvertArguments(cx, argc, argv,"/ i", &gExitCode);

    gQuitting = JS_TRUE;
//    exit(0);
    return JS_FALSE;
}

static JSBool
DumpXPC(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    int32 depth = 2;

    if (argc > 0) {
        if (!JS_ValueToInt32(cx, argv[0], &depth))
            return JS_FALSE;
    }

    nsIXPConnect* xpc = GetXPConnect();
    if(xpc)
    {
        xpc->DebugDump((int16)depth);
        NS_RELEASE(xpc);
    }
    return JS_TRUE;
}

#ifdef GC_MARK_DEBUG
extern "C" JS_FRIEND_DATA(FILE *) js_DumpGCHeap;
#endif

static JSBool
GC(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSRuntime *rt;
    uint32 preBytes;

    rt = cx->runtime;
    preBytes = rt->gcBytes;
#ifdef GC_MARK_DEBUG
    if (argc && JSVAL_IS_STRING(argv[0])) {
        char *name = JS_GetStringBytes(JSVAL_TO_STRING(argv[0]));
        FILE *file = fopen(name, "w");
        if (!file) {
            fprintf(gErrFile, "gc: can't open %s: %s\n", strerror(errno));
            return JS_FALSE;
        }
        js_DumpGCHeap = file;
    } else {
        js_DumpGCHeap = stdout;
    }
#endif
    js_ForceGC(cx);
#ifdef GC_MARK_DEBUG
    if (js_DumpGCHeap != stdout)
        fclose(js_DumpGCHeap);
    js_DumpGCHeap = NULL;
#endif
    fprintf(gOutFile, "before %lu, after %lu, break %08lx\n",
           (unsigned long)preBytes, (unsigned long)rt->gcBytes,
#ifdef XP_UNIX
           (unsigned long)sbrk(0)
#else
           0
#endif
           );
#ifdef JS_GCMETER
    js_DumpGCStats(rt, stdout);
#endif
    return JS_TRUE;
}

static JSFunctionSpec glob_functions[] = {
    {"print",           Print,          0},
    {"load",            Load,           1},
    {"quit",            Quit,           0},
    {"version",         Version,        1},
    {"build",           BuildDate,      0},
    {"dumpXPC",         DumpXPC,        1},
    {"dump",            Dump,           1},
    {"gc",              GC,             0},
    {0}
};

static JSClass global_class = {
    "global", 0,
    JS_PropertyStub,  JS_PropertyStub,  JS_PropertyStub,  JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub,   JS_ConvertStub,   JS_FinalizeStub
};

/***************************************************************************/

typedef enum JSShellErrNum {
#define MSG_DEF(name, number, count, exception, format) \
    name = number,
#include "jsshell.msg"
#undef MSG_DEF
    JSShellErr_Limit
#undef MSGDEF
} JSShellErrNum;

JSErrorFormatString jsShell_ErrorFormatString[JSErr_Limit] = {
#if JS_HAS_DFLT_MSG_STRINGS
#define MSG_DEF(name, number, count, exception, format) \
    { format, count } ,
#else
#define MSG_DEF(name, number, count, exception, format) \
    { NULL, count } ,
#endif
#include "jsshell.msg"
#undef MSG_DEF
};

static const JSErrorFormatString *
my_GetErrorMessage(void *userRef, const char *locale, const uintN errorNumber)
{
    if ((errorNumber > 0) && (errorNumber < JSShellErr_Limit))
            return &jsShell_ErrorFormatString[errorNumber];
        else
            return NULL;
}

#ifdef EDITLINE
extern "C" {
extern char     *readline(const char *prompt);
extern void     add_history(char *line);
}
#endif

static JSBool
GetLine(JSContext *cx, char *bufp, FILE *fh, const char *prompt) {
#ifdef EDITLINE
    /*
     * Use readline only if fh is stdin, because there's no way to specify
     * another handle.  Are other filehandles interactive?
     */
    if (fh == stdin) {
        char *linep;
        if ((linep = readline(prompt)) == NULL)
            return JS_FALSE;
        if (strlen(linep) > 0)
            add_history(linep);
        strcpy(bufp, linep);
        JS_free(cx, linep);
        bufp += strlen(bufp);
        *bufp++ = '\n';
        *bufp = '\0';
    } else
#endif
    {
        char line[256];
        fprintf(gOutFile, prompt);
        if (fgets(line, 256, fh) == NULL)
            return JS_FALSE;
        strcpy(bufp, line);
    }
    return JS_TRUE;
}

static void
Process(JSContext *cx, JSObject *obj, char *filename)
{
    JSBool ok, hitEOF;
    JSScript *script;
    jsval result;
    JSString *str;
    char buffer[4098];
    char *bufp;
    int lineno;
    int startline;
    FILE *fh;

    if (filename != NULL && strcmp(filename, "-") != 0) {
        fh = fopen(filename, "r");
        if (!fh) {
            JS_ReportErrorNumber(cx, my_GetErrorMessage, NULL,
                            JSSMSG_CANT_OPEN,
                            filename, strerror(errno));
            gExitCode = EXITCODE_FILE_NOT_FOUND;
            return;
        }
    } else {
        fh = stdin;
    }

    if (!isatty(fileno(fh))) {
        /*
         * It's not interactive - just execute it.
         *
         * Support the UNIX #! shell hack; gobble the first line if it starts
         * with '#'.  TODO - this isn't quite compatible with sharp variables,
         * as a legal js program (using sharp variables) might start with '#'.
         * But that would require multi-character lookahead.
         */
        int ch = fgetc(fh);
        if (ch == '#') {
            while((ch = fgetc(fh)) != EOF) {
                if(ch == '\n' || ch == '\r')
                    break;
            }
        }
        ungetc(ch, fh);
        script = JS_CompileFileHandle(cx, obj, filename, fh);
        if (script)
            (void)JS_ExecuteScript(cx, obj, script, &result);
        return;
    }

    /* It's an interactive filehandle; drop into read-eval-print loop. */
    lineno = 1;
    hitEOF = JS_FALSE;
    do {
        bufp = buffer;
        *bufp = '\0';

        /*
         * Accumulate lines until we get a 'compilable unit' - one that either
         * generates an error (before running out of source) or that compiles
         * cleanly.  This should be whenever we get a complete statement that
         * coincides with the end of a line.
         */
        startline = lineno;
        do {
            if (!GetLine(cx, bufp, fh, startline == lineno ? "js> " : "")) {
                hitEOF = JS_TRUE;
                break;
            }
            bufp += strlen(bufp);
            lineno++;
        } while (!JS_BufferIsCompilableUnit(cx, obj,
                                            buffer, strlen(buffer)));
        /* Clear any pending exception from previous failed compiles.  */
        JS_ClearPendingException(cx);
        script = JS_CompileScript(cx, obj, buffer, strlen(buffer),
//#ifdef JSDEBUGGER
                                  "typein",
//#else
//                                  NULL,
//#endif
                                  startline);
        if (script) {
            JSErrorReporter older;

            ok = JS_ExecuteScript(cx, obj, script, &result);
            if (ok && result != JSVAL_VOID) {
                /* Suppress error reports from JS_ValueToString(). */
                older = JS_SetErrorReporter(cx, NULL);
                str = JS_ValueToString(cx, result);
                JS_SetErrorReporter(cx, older);

                if (str)
                    fprintf(gOutFile, "%s\n", JS_GetStringBytes(str));
                else
                    ok = JS_FALSE;
            }
#if 0
#if JS_HAS_ERROR_EXCEPTIONS
            /*
             * Require that any time we return failure, an exception has
             * been set.
             */
            JS_ASSERT(ok || JS_IsExceptionPending(cx));

            /*
             * Also that any time an exception has been set, we've
             * returned failure.
             */
            JS_ASSERT(!JS_IsExceptionPending(cx) || !ok);
#endif /* JS_HAS_ERROR_EXCEPTIONS */
#endif
            JS_DestroyScript(cx, script);
        }
    } while (!hitEOF && !gQuitting);
    fprintf(gOutFile, "\n");
    return;
}

static int
usage(void)
{
    fprintf(gErrFile, "%s\n", JS_GetImplementationVersion());
    fprintf(gErrFile, "usage: js [-w] [-v version] [-f scriptfile] [scriptfile] [scriptarg...]\n");
    return 2;
}

static int reportWarnings;

static int
ProcessArgs(JSContext *cx, JSObject *obj, char **argv, int argc)
{
    int i;
    char *filename = NULL;
    jsint length;
    jsval *vector;
    jsval *p;
    JSObject *argsObj;
    JSBool isInteractive = JS_TRUE;

    for (i=0; i < argc; i++) {
        if (argv[i][0] == '-') {
            switch (argv[i][1]) {
            case 'v':
                if (i+1 == argc) {
                    return usage();
                }
                JS_SetVersion(cx, JSVersion(atoi(argv[i+1])));
                i++;
                break;
            case 'w':
                reportWarnings++;
                break;

            case 'f':
                if (i+1 == argc) {
                    return usage();
                }
                filename = argv[i+1];
                /* "-f -" means read from stdin */
                if (filename[0] == '-' && filename[1] == '\0')
                    filename = NULL;
                Process(cx, obj, filename);
                filename = NULL;
                /* XXX: js -f foo.js should interpret foo.js and then
                 * drop into interactive mode, but that breaks test
                 * harness. Just execute foo.js for now.
                 */
                isInteractive = JS_FALSE;
                i++;
                break;
            default:
                return usage();
            }
        } else {
            filename = argv[i++];
            isInteractive = JS_FALSE;
            break;
        }
    }

    length = argc - i;
    vector = (jsval*) JS_malloc(cx, length * sizeof(jsval));
    p = vector;

    if (vector == NULL)
        return 1;

    while (i < argc) {
        JSString *str = JS_NewStringCopyZ(cx, argv[i]);
        if (str == NULL)
            return 1;
        *p++ = STRING_TO_JSVAL(str);
        i++;
    }
    argsObj = JS_NewArrayObject(cx, length, vector);
    JS_free(cx, vector);
    if (argsObj == NULL)
        return 1;

    if (!JS_DefineProperty(cx, obj, "arguments",
                           OBJECT_TO_JSVAL(argsObj), NULL, NULL, 0))
        return 1;

    if (filename || isInteractive)
        Process(cx, obj, filename);
    return gExitCode;
}

/***************************************************************************/

class FullTrustSecMan : public nsIXPCSecurityManager
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIXPCSECURITYMANAGER
  FullTrustSecMan();
};

NS_IMPL_ISUPPORTS1(FullTrustSecMan, nsIXPCSecurityManager);

FullTrustSecMan::FullTrustSecMan()
{
    NS_INIT_REFCNT();
}

NS_IMETHODIMP
FullTrustSecMan::CanCreateWrapper(JSContext * aJSContext, const nsIID & aIID, nsISupports *aObj)
{
    return NS_OK;
}

NS_IMETHODIMP
FullTrustSecMan::CanCreateInstance(JSContext * aJSContext, const nsCID & aCID)
{
    return NS_OK;
}

NS_IMETHODIMP
FullTrustSecMan::CanGetService(JSContext * aJSContext, const nsCID & aCID)
{
    return NS_OK;
}

NS_IMETHODIMP
FullTrustSecMan::CanCallMethod(JSContext * aJSContext, const nsIID & aIID, nsISupports *aObj, nsIInterfaceInfo *aInterfaceInfo, PRUint16 aMethodIndex, const jsid aName)
{
    return NS_OK;
}

NS_IMETHODIMP
FullTrustSecMan::CanGetProperty(JSContext * aJSContext, const nsIID & aIID, nsISupports *aObj, nsIInterfaceInfo *aInterfaceInfo, PRUint16 aMethodIndex, const jsid aName)
{
    return NS_OK;
}
NS_IMETHODIMP
FullTrustSecMan::CanSetProperty(JSContext * aJSContext, const nsIID & aIID, nsISupports *aObj, nsIInterfaceInfo *aInterfaceInfo, PRUint16 aMethodIndex, const jsid aName)
{
    return NS_OK;
}

/***************************************************************************/

int
main(int argc, char **argv)
{
    JSRuntime *rt;
    JSContext *jscontext;
    JSObject *glob;
    int result;
    nsresult rv;

    gErrFile = stderr;
    gOutFile = stdout;

    rv = NS_InitXPCOM(NULL, NULL);
    NS_ASSERTION( NS_SUCCEEDED(rv), "NS_InitXPCOM failed" );

    SetupRegistry();

    NS_WITH_SERVICE(nsIJSRuntimeService, rtsvc, "nsJSRuntimeService", &rv);
    // get the JSRuntime from the runtime svc, if possible
    if (NS_FAILED(rv) ||
        NS_FAILED(rtsvc->GetRuntime(&rt))) {
        rtsvc = NULL;
        rt = JS_NewRuntime(8L * 1024L * 1024L);
    }
    if (!rt)
        return 1;

    jscontext = JS_NewContext(rt, 8192);
    if (!jscontext)
        return 1;

    JS_SetErrorReporter(jscontext, my_ErrorReporter);
/*    JS_SetVersion(jscontext, JSVERSION_1_4); */

    nsIXPConnect* xpc = GetXPConnect();
    if(!xpc)
    {
        printf("GetXPConnect() returned NULL!\n");
        return 1;
    }

    // Since the caps security system might set a default security manager
    // we will be sure that the secman on this context gives full trust.
    // That way we can avoid getting principals from the caps security manager
    // just to shut it up. Also, note that even though our secman will allow
    // anything, we set the flags to '0' so it ought never get called anyway.
    nsCOMPtr<nsIXPCSecurityManager> secman = 
        NS_STATIC_CAST(nsIXPCSecurityManager*,new FullTrustSecMan());
    xpc->SetSecurityManagerForJSContext(jscontext, secman, 0);
    
    NS_WITH_SERVICE(nsIJSContextStack, cxstack, "nsThreadJSContextStack", &rv);
    if(NS_FAILED(rv))
    {
        printf("failed to get the nsThreadJSContextStack service!\n");
        return 1;
    }
    if(NS_FAILED(cxstack->Push(jscontext)))
    {
        printf("failed to get push the current jscontext on the nsThreadJSContextStack service!\n");
        return 1;
    }

    glob = JS_NewObject(jscontext, &global_class, NULL, NULL);
    if (!glob)
        return 1;

    if (!JS_InitStandardClasses(jscontext, glob))
        return 1;
    if (!JS_DefineFunctions(jscontext, glob, glob_functions))
        return 1;
    if (NS_FAILED(xpc->InitClasses(jscontext, glob)))
        return 1;

    argc--;
    argv++;

    result = ProcessArgs(jscontext, glob, argv, argc);

    JS_ClearScope(jscontext, glob);
    js_ForceGC(jscontext);
    JSContext *oldcx;
    cxstack->Pop(&oldcx);
    NS_ASSERTION(oldcx == jscontext, "JS thread context push/pop mismatch");
    js_ForceGC(jscontext);
    JS_DestroyContext(jscontext);
    xpc->SyncJSContexts();
    xpc->DebugDump(4);
    NS_RELEASE(xpc);

    if (!rtsvc) {
        /* no runtime service, so we have to handle shutdown */
        JS_DestroyRuntime(rt);
        JS_ShutDown();
    }

    rv = NS_ShutdownXPCOM( NULL );
    NS_ASSERTION(NS_SUCCEEDED(rv), "NS_ShutdownXPCOM failed");

    return result;
}

/***************************************************************************/

#include "jsatom.h"
#ifdef DEBUG
int
DumpAtom(JSHashEntry *he, int i, void *arg)
{
    FILE *fp = (FILE *)arg;
    JSAtom *atom = (JSAtom *)he;

    fprintf(fp, "%3d %08x %5lu ",
            i, (uintN)he->keyHash, (unsigned long)atom->number);
    if (ATOM_IS_STRING(atom))
        fprintf(fp, "\"%s\"\n", ATOM_BYTES(atom));
    else if (ATOM_IS_INT(atom))
        fprintf(fp, "%ld\n", (long)ATOM_TO_INT(atom));
    else
        fprintf(fp, "%.16g\n", *ATOM_TO_DOUBLE(atom));
    return HT_ENUMERATE_NEXT;
}

int
DumpSymbol(JSHashEntry *he, int i, void *arg)
{
    FILE *fp = (FILE *)arg;
    JSSymbol *sym = (JSSymbol *)he;

    fprintf(fp, "%3d %08x", i, (uintN)he->keyHash);
    if (JSVAL_IS_INT(sym_id(sym)))
        fprintf(fp, " [%ld]\n", (long)JSVAL_TO_INT(sym_id(sym)));
    else
        fprintf(fp, " \"%s\"\n", ATOM_BYTES(sym_atom(sym)));
    return HT_ENUMERATE_NEXT;
}

/* These are callable from gdb. */
JS_BEGIN_EXTERN_C
void Dsym(JSSymbol *sym) { if (sym) DumpSymbol(&sym->entry, 0, gErrFile); }
void Datom(JSAtom *atom) { if (atom) DumpAtom(&atom->entry, 0, gErrFile); }
//void Dobj(nsISupports* p, int depth) {if(p)XPC_DUMP(p,depth);}
//void Dxpc(int depth) {Dobj(GetXPConnect(), depth);}
JS_END_EXTERN_C
#endif
