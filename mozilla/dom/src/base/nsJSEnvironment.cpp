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

#include "nsJSEnvironment.h"
#include "nsIScriptObjectOwner.h"
#include "nsIScriptContextOwner.h"
#include "nsIScriptGlobalObject.h"
#include "nsIScriptGlobalObjectOwner.h"
#include "nsIDOMWindow.h"
#include "nsIDOMNode.h"
#include "nsIDOMElement.h"
#include "nsIDOMDocument.h"
#include "nsIDOMText.h"
#include "nsIDOMAttr.h"
#include "nsIDOMNamedNodeMap.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMKeyEvent.h"
#include "nsIDOMHTMLImageElement.h"
#include "nsIDOMHTMLOptionElement.h"
#include "nsIScriptSecurityManager.h"
#include "nsIScriptNameSetRegistry.h"
#include "nsIScriptNameSpaceManager.h"
#include "nsDOMCID.h"
#include "nsIServiceManager.h"
#include "nsIXPConnect.h"
#include "nsIJSContextStack.h"
#include "nsIJSRuntimeService.h"
#include "nsIPref.h"
#include "nsCOMPtr.h"
#include "nsJSUtils.h"
#include "nsIDocShell.h"
#include "nsIPresContext.h"
#include "nsIInterfaceRequestor.h"
#include "nsIPrompt.h"
#include "nsIObserverService.h"

// Force PR_LOGGING so we can get JS strict warnings even in release builds
#define FORCE_PR_LOG 1
#include "prlog.h"
#include "prthread.h"

#include "nsIJVMManager.h"
#include "nsILiveConnectManager.h"

const size_t gStackSize = 8192;

static NS_DEFINE_IID(kCScriptNameSetRegistryCID, NS_SCRIPT_NAMESET_REGISTRY_CID);
static NS_DEFINE_IID(kPrefServiceCID, NS_PREF_CID);

#ifdef PR_LOGGING
static PRLogModuleInfo* gJSDiagnostics = nsnull;
#endif

#include "nsIScriptError.h"

void PR_CALLBACK
NS_ScriptErrorReporter(JSContext *cx,
                       const char *message,
                       JSErrorReport *report)
{
  nsCOMPtr<nsIScriptContext> context;
  nsEventStatus status = nsEventStatus_eIgnore;

  // XXX this means we are not going to get error reports on non DOM contexts
  nsJSUtils::nsGetDynamicScriptContext(cx, getter_AddRefs(context));
  if (context) {
    nsCOMPtr<nsIScriptGlobalObject> globalObject( dont_AddRef( context->GetGlobalObject() ) );

    if (globalObject) {
      nsCOMPtr<nsIScriptGlobalObjectOwner> owner;
      if(NS_FAILED(globalObject->GetGlobalObjectOwner(getter_AddRefs(owner))) ||
         !owner) {
        NS_WARN_IF_FALSE(PR_FALSE, "Failed to get a global Object Owner");
        return;
      }

      //send error event first, then proceed
      nsCOMPtr<nsIDocShell> docShell;
      globalObject->GetDocShell(getter_AddRefs(docShell));
      if (docShell) {
        nsCOMPtr<nsIPresContext> presContext;
        docShell->GetPresContext(getter_AddRefs(presContext));
        if(presContext) {
          nsEvent errorevent;
          errorevent.eventStructType = NS_EVENT;
          errorevent.message = NS_SCRIPT_ERROR;

          globalObject->HandleDOMEvent(presContext, &errorevent, nsnull, NS_EVENT_FLAG_INIT, &status);
        }
      }

      if (status != nsEventStatus_eConsumeNoDefault) {

        // Make an nsIScriptError and populate it with information from
        // this error.
        nsCOMPtr<nsIScriptError>
          errorObject(do_CreateInstance("mozilla.scripterror.1"));

        // XXX possible here to distinguish between XUL and content js?
        // or could just expose setCategory and twiddle it later.
        const char *category = "XUL/Content JavaScript";

        if (errorObject != nsnull) {
          nsresult rv = NS_ERROR_FAILURE;
          if (report) {
            nsAutoString fileUni;
            fileUni.AssignWithConversion(report->filename);
            const PRUnichar *newFileUni = fileUni.ToNewUnicode();
            PRUint32 column = report->uctokenptr - report->uclinebuf;
            rv = errorObject->Init(NS_REINTERPRET_CAST(const PRUnichar*, report->ucmessage), newFileUni,
                                   NS_REINTERPRET_CAST(const PRUnichar*, report->uclinebuf), report->lineno,
                                   column, report->flags, category);
            nsMemory::Free((void *)newFileUni);
          } else if (message) {
            nsAutoString messageUni;
            messageUni.AssignWithConversion(message);
            const PRUnichar *newMessageUni = messageUni.ToNewUnicode();
            rv = errorObject->Init(newMessageUni, nsnull, nsnull,
                                   0, 0, 0, category);
            nsMemory::Free((void *)newMessageUni);
          }
      
          if (NS_SUCCEEDED(rv))
            owner->ReportScriptError(errorObject);
        }
      }
    }
  }

  // Print it to stderr as well, for the benefit of those invoking
  // mozilla with -console.
  nsAutoString error;
  error.AssignWithConversion("JavaScript ");
  error.AppendWithConversion(JSREPORT_IS_WARNING(report->flags) ? "warning: " : "error: ");
  error.AppendWithConversion("\n");
  error.AppendWithConversion(report->filename);
  error.AppendWithConversion(" line ");
  error.AppendInt(report->lineno, 10);
  error.AppendWithConversion(": ");
  error.Append(NS_REINTERPRET_CAST(const PRUnichar*, report->ucmessage));
  error.AppendWithConversion("\n");
  if (status != nsEventStatus_eIgnore)
    error.AppendWithConversion("Error was suppressed by event handler\n");
  
  char *errorStr = error.ToNewCString();
  if (errorStr) {
    fprintf(stderr, "%s\n", errorStr);
    nsMemory::Free(errorStr);
  }

#ifdef PR_LOGGING
  if (report) {
    if (!gJSDiagnostics)
      gJSDiagnostics = PR_NewLogModule("JSDiagnostics");

    if (gJSDiagnostics) {
      PR_LOG(gJSDiagnostics,
             JSREPORT_IS_WARNING(report->flags) ? PR_LOG_WARNING : PR_LOG_ERROR,
             ("file %s, line %u: %s\n%s%s",
              report->filename, report->lineno, message,
              report->linebuf ? report->linebuf : "",
              (report->linebuf &&
               report->linebuf[strlen(report->linebuf)-1] != '\n')
              ? "\n"
              : ""));
    }
  }
#endif

  // XXX do we really want to be doing this?
  ::JS_ClearPendingException(cx);
}

#define MAYBE_GC_BRANCH_COUNT_MASK 0x00000fff // 4095
#define MAYBE_STOP_BRANCH_COUNT_MASK 0x003fffff

JSBool PR_CALLBACK
nsJSContext::DOMBranchCallback(JSContext *cx, JSScript *script)
{
  // Get the native context
  nsJSContext *ctx = (nsJSContext *)::JS_GetContextPrivate(cx);
  NS_ENSURE_TRUE(ctx, JS_TRUE);

  // Filter out most of the calls to this callback
  if (++ctx->mBranchCallbackCount & MAYBE_GC_BRANCH_COUNT_MASK)
    return JS_TRUE;

  // Run the GC if we get this far.
  JS_MaybeGC(cx);

  // Filter out most of the calls to this callback that make it this far
  if (ctx->mBranchCallbackCount & MAYBE_STOP_BRANCH_COUNT_MASK)
    return JS_TRUE;

  // If we get here we're most likely executing an infinite loop in JS,
  // we'll tell the user about this and we'll give the user the option
  // of stopping the execution of the script.
  nsCOMPtr<nsIScriptGlobalObject> global(dont_AddRef(ctx->GetGlobalObject()));
  NS_ENSURE_TRUE(global, JS_TRUE);

  nsCOMPtr<nsIDocShell> docShell;
  global->GetDocShell(getter_AddRefs(docShell));
  NS_ENSURE_TRUE(docShell, JS_TRUE);

  nsCOMPtr<nsIInterfaceRequestor> ireq(do_QueryInterface(docShell));
  NS_ENSURE_TRUE(ireq, JS_TRUE);

  // Get the nsIPrompt interface from the docshell
  nsCOMPtr<nsIPrompt> prompt;
  ireq->GetInterface(NS_GET_IID(nsIPrompt), getter_AddRefs(prompt));
  NS_ENSURE_TRUE(prompt, JS_TRUE);

  nsAutoString title, msg;
  title.AssignWithConversion("Script warning");
  msg.AssignWithConversion("A script on this page is causing mozilla to "
                           "run slowly. If it continues to run, your "
                           "computer may become unresponsive.\n\nDo you "
                           "want to abort the script?");

  JSBool ret = JS_TRUE;

  // Open the dialog.
  if (NS_FAILED(prompt->Confirm(title.GetUnicode(), msg.GetUnicode(), &ret)))
    return JS_TRUE;

  return !ret;
}

nsJSContext::nsJSContext(JSRuntime *aRuntime)
{
  NS_INIT_REFCNT();
  mContext = ::JS_NewContext(aRuntime, gStackSize);
  if (mContext) {
    ::JS_SetContextPrivate(mContext, (void *)this);

    // Check for the JS strict option, which enables extra error checks
    nsresult rv;
    PRBool strict;
    NS_WITH_SERVICE(nsIPref, prefs, kPrefServiceCID, &rv);
    if (NS_SUCCEEDED(rv)) {
      uint32 options = 0;
#ifdef JSOPTION_STRICT
      if (NS_SUCCEEDED(prefs->GetBoolPref("javascript.options.strict",
                                          &strict)) &&
          strict) {
        options |= JSOPTION_STRICT;
      }
#endif
#ifdef JSOPTION_WERROR
      PRBool werror;
      if (NS_SUCCEEDED(prefs->GetBoolPref("javascript.options.werror",
                                          &werror)) &&
          werror) {
        options |= JSOPTION_WERROR;
      }
#endif
      if (options)
        ::JS_SetOptions(mContext, options);
    }

    ::JS_SetBranchCallback(mContext, DOMBranchCallback);
  }
  mIsInitialized = PR_FALSE;
  mNumEvaluations = 0;
  mSecurityManager = nsnull;
  mOwner = nsnull;
  mTerminationFunc = nsnull;
  mScriptsEnabled = PR_TRUE;
  mBranchCallbackCount = 0;
}

const char kScriptSecurityManagerProgID[] = NS_SCRIPTSECURITYMANAGER_PROGID;

nsJSContext::~nsJSContext()
{
  if (mSecurityManager) {
    nsServiceManager::ReleaseService(kScriptSecurityManagerProgID,
                                     mSecurityManager);
    mSecurityManager = nsnull;
  }

  // Cope with JS_NewContext failure in ctor (XXXbe move NewContext to Init?)
  if (!mContext)
    return;

  /* Remove global object reference to window object, so it can be collected. */
  ::JS_SetGlobalObject(mContext, nsnull);
  ::JS_DestroyContext(mContext);

  // Let xpconnect resync its JSContext tracker (this is optional)
  nsresult rv;
  NS_WITH_SERVICE(nsIXPConnect, xpc, nsIXPConnect::GetCID(), &rv);
  if (NS_SUCCEEDED(rv))
    xpc->SyncJSContexts();
}

NS_IMPL_ISUPPORTS(nsJSContext, NS_GET_IID(nsIScriptContext));

NS_IMETHODIMP
nsJSContext::EvaluateStringWithValue(const nsAReadableString& aScript,
                                     void *aScopeObject,
                                     nsIPrincipal *aPrincipal,
                                     const char *aURL,
                                     PRUint32 aLineNo,
                                     const char* aVersion,
                                     void* aRetValue,
                                     PRBool* aIsUndefined)
{
  // Beware that the result is not rooted! Be very careful not to run
  // the GC before rooting the result somehow!
  if (!mScriptsEnabled) {
    *aIsUndefined = PR_TRUE;
    return NS_OK;
  }

  nsresult rv;
  if (!aScopeObject)
    aScopeObject = ::JS_GetGlobalObject(mContext);

  // Safety first: get an object representing the script's principals, i.e.,
  // the entities who signed this script, or the fully-qualified-domain-name
  // or "codebase" from which it was loaded.
  JSPrincipals *jsprin;
  nsCOMPtr<nsIPrincipal> principal = aPrincipal;
  if (aPrincipal) {
    aPrincipal->GetJSPrincipals(&jsprin);
  }
  else {
    nsCOMPtr<nsIScriptGlobalObject> global = dont_AddRef(GetGlobalObject());
    if (!global)
      return NS_ERROR_FAILURE;
    nsCOMPtr<nsIScriptObjectPrincipal> objPrincipal = do_QueryInterface(global, &rv);
    if (NS_FAILED(rv))
      return NS_ERROR_FAILURE;
    rv = objPrincipal->GetPrincipal(getter_AddRefs(principal));
    if (NS_FAILED(rv))
      return NS_ERROR_FAILURE;
    principal->GetJSPrincipals(&jsprin);
  }
  // From here on, we must JSPRINCIPALS_DROP(jsprin) before returning...

  PRBool ok = PR_FALSE;
  nsCOMPtr<nsIScriptSecurityManager> securityManager;
  rv = GetSecurityManager(getter_AddRefs(securityManager));
  if (NS_SUCCEEDED(rv))
    rv = securityManager->CanExecuteScripts(principal, &ok);
  if (NS_FAILED(rv)) {
    JSPRINCIPALS_DROP(mContext, jsprin);
    return NS_ERROR_FAILURE;
  }

  // Push our JSContext on the current thread's context stack so JS called
  // from native code via XPConnect uses the right context.  Do this whether
  // or not the SecurityManager said "ok", in order to simplify control flow
  // below where we pop before returning.
  NS_WITH_SERVICE(nsIJSContextStack, stack, "nsThreadJSContextStack", &rv);
  if (NS_FAILED(rv) || NS_FAILED(stack->Push(mContext))) {
    JSPRINCIPALS_DROP(mContext, jsprin);
    return NS_ERROR_FAILURE;
  }

  // The result of evaluation, used only if there were no errors.  This need
  // not be a GC root currently, provided we run the GC only from the branch
  // callback or from ScriptEvaluated.  TODO: use JS_Begin/EndRequest to keep
  // the GC from racing with JS execution on any thread.
  jsval val;

  if (ok) {
    JSVersion newVersion;

    // SecurityManager said "ok", but don't execute if aVersion is specified
    // and unknown.  Do execute with the default version (and avoid thrashing
    // the context's version) if aVersion is not specified.
    ok = (!aVersion ||
          (newVersion = ::JS_StringToVersion(aVersion)) != JSVERSION_UNKNOWN);
    if (ok) {
      JSVersion oldVersion;

      if (aVersion)
        oldVersion = ::JS_SetVersion(mContext, newVersion);
      mRef = nsnull;
      mTerminationFunc = nsnull;
      ok = ::JS_EvaluateUCScriptForPrincipals(mContext,
                                              (JSObject *)aScopeObject,
                                              jsprin,
                                              (jschar*)(const PRUnichar*)nsPromiseFlatString(aScript),
                                              aScript.Length(),
                                              aURL,
                                              aLineNo,
                                              &val);
      if (aVersion)
        ::JS_SetVersion(mContext, oldVersion);
    }
  }

  // Whew!  Finally done with these manually ref-counted things.
  JSPRINCIPALS_DROP(mContext, jsprin);

  // If all went well, convert val to a string (XXXbe unless undefined?).
  if (ok) {
    if (aIsUndefined) *aIsUndefined = JSVAL_IS_VOID(val);
    *NS_STATIC_CAST(jsval*, aRetValue) = val;
  }
  else {
    if (aIsUndefined) *aIsUndefined = PR_TRUE;
  }

  // Pop here, after JS_ValueToString and any other possible evaluation.
  if (NS_FAILED(stack->Pop(nsnull)))
    rv = NS_ERROR_FAILURE;

  return rv;

}

NS_IMETHODIMP
nsJSContext::EvaluateString(const nsAReadableString& aScript,
                            void *aScopeObject,
                            nsIPrincipal *aPrincipal,
                            const char *aURL,
                            PRUint32 aLineNo,
                            const char* aVersion,
                            nsAWritableString& aRetValue,
                            PRBool* aIsUndefined)
{
  if (!mScriptsEnabled) {
    *aIsUndefined = PR_TRUE;
    aRetValue.Truncate();
    return NS_OK;
  }

  nsresult rv;
  if (!aScopeObject)
    aScopeObject = ::JS_GetGlobalObject(mContext);

  // Safety first: get an object representing the script's principals, i.e.,
  // the entities who signed this script, or the fully-qualified-domain-name
  // or "codebase" from which it was loaded.
  JSPrincipals *jsprin;
  nsCOMPtr<nsIPrincipal> principal = aPrincipal;
  if (aPrincipal) {
    aPrincipal->GetJSPrincipals(&jsprin);
  }
  else {
    nsCOMPtr<nsIScriptGlobalObject> global = dont_AddRef(GetGlobalObject());
    if (!global)
      return NS_ERROR_FAILURE;
    nsCOMPtr<nsIScriptObjectPrincipal> objPrincipal = do_QueryInterface(global, &rv);
    if (NS_FAILED(rv))
      return NS_ERROR_FAILURE;
    rv = objPrincipal->GetPrincipal(getter_AddRefs(principal));
    if (NS_FAILED(rv))
      return NS_ERROR_FAILURE;
    principal->GetJSPrincipals(&jsprin);
  }
  // From here on, we must JSPRINCIPALS_DROP(jsprin) before returning...

  PRBool ok = PR_FALSE;
  nsCOMPtr<nsIScriptSecurityManager> securityManager;
  rv = GetSecurityManager(getter_AddRefs(securityManager));
  if (NS_SUCCEEDED(rv))
    rv = securityManager->CanExecuteScripts(principal, &ok);
  if (NS_FAILED(rv)) {
    JSPRINCIPALS_DROP(mContext, jsprin);
    return NS_ERROR_FAILURE;
  }

  // Push our JSContext on the current thread's context stack so JS called
  // from native code via XPConnect uses the right context.  Do this whether
  // or not the SecurityManager said "ok", in order to simplify control flow
  // below where we pop before returning.
  NS_WITH_SERVICE(nsIJSContextStack, stack, "nsThreadJSContextStack", &rv);
  if (NS_FAILED(rv) || NS_FAILED(stack->Push(mContext))) {
    JSPRINCIPALS_DROP(mContext, jsprin);
    return NS_ERROR_FAILURE;
  }

  // The result of evaluation, used only if there were no errors.  This need
  // not be a GC root currently, provided we run the GC only from the branch
  // callback or from ScriptEvaluated.  TODO: use JS_Begin/EndRequest to keep
  // the GC from racing with JS execution on any thread.
  jsval val;

  if (ok) {
    JSVersion newVersion;

    // SecurityManager said "ok", but don't execute if aVersion is specified
    // and unknown.  Do execute with the default version (and avoid thrashing
    // the context's version) if aVersion is not specified.
    ok = (!aVersion ||
          (newVersion = ::JS_StringToVersion(aVersion)) != JSVERSION_UNKNOWN);
    if (ok) {
      JSVersion oldVersion;

      if (aVersion)
        oldVersion = ::JS_SetVersion(mContext, newVersion);
      mRef = nsnull;
      mTerminationFunc = nsnull;
      ok = ::JS_EvaluateUCScriptForPrincipals(mContext,
                                              (JSObject *)aScopeObject,
                                              jsprin,
                                              (jschar*)(const PRUnichar*)nsPromiseFlatString(aScript),
                                              aScript.Length(),
                                              aURL,
                                              aLineNo,
                                              &val);
      if (aVersion)
        ::JS_SetVersion(mContext, oldVersion);
    }
  }

  // Whew!  Finally done with these manually ref-counted things.
  JSPRINCIPALS_DROP(mContext, jsprin);

  // If all went well, convert val to a string (XXXbe unless undefined?).
  if (ok) {
    if (aIsUndefined) *aIsUndefined = JSVAL_IS_VOID(val);
    JSString* jsstring = ::JS_ValueToString(mContext, val);
    if (jsstring)
      aRetValue.Assign(NS_REINTERPRET_CAST(const PRUnichar*, ::JS_GetStringChars(jsstring)));
    else
      rv = NS_ERROR_OUT_OF_MEMORY;
  }
  else {
    if (aIsUndefined) *aIsUndefined = PR_TRUE;
    aRetValue.Truncate();
  }

  ScriptEvaluated();

  // Pop here, after JS_ValueToString and any other possible evaluation.
  if (NS_FAILED(stack->Pop(nsnull)))
    rv = NS_ERROR_FAILURE;

  return rv;
}

NS_IMETHODIMP
nsJSContext::CompileScript(const PRUnichar* aText,
                           PRInt32 aTextLength,
                           void *aScopeObject,
                           nsIPrincipal *aPrincipal,
                           const char *aURL,
                           PRUint32 aLineNo,
                           const char* aVersion,
                           void** aScriptObject)
{
  nsresult rv;
  NS_ENSURE_ARG_POINTER(aPrincipal);

  if (!aScopeObject)
    aScopeObject = ::JS_GetGlobalObject(mContext);

  JSPrincipals *jsprin;
  aPrincipal->GetJSPrincipals(&jsprin);
  // From here on, we must JSPRINCIPALS_DROP(jsprin) before returning...

  PRBool ok = PR_FALSE;
  nsCOMPtr<nsIScriptSecurityManager> securityManager;
  rv = GetSecurityManager(getter_AddRefs(securityManager));
  if (NS_SUCCEEDED(rv))
    rv = securityManager->CanExecuteScripts(aPrincipal, &ok);
  if (NS_FAILED(rv)) {
    JSPRINCIPALS_DROP(mContext, jsprin);
    return NS_ERROR_FAILURE;
  }

  *aScriptObject = nsnull;
  if (ok) {
    JSVersion newVersion;

    // SecurityManager said "ok", but don't compile if aVersion is specified
    // and unknown.  Do compile with the default version (and avoid thrashing
    // the context's version) if aVersion is not specified.
    if (!aVersion ||
        (newVersion = ::JS_StringToVersion(aVersion)) != JSVERSION_UNKNOWN) {
      JSVersion oldVersion;
      if (aVersion)
        oldVersion = ::JS_SetVersion(mContext, newVersion);

      JSScript* script =
        ::JS_CompileUCScriptForPrincipals(mContext,
                                          (JSObject*) aScopeObject,
                                          jsprin,
                                          (jschar*) aText,
                                          aTextLength,
                                          aURL,
                                          aLineNo);
      if (script) {
        *aScriptObject = (void*) ::JS_NewScriptObject(mContext, script);
        if (! *aScriptObject) {
          ::JS_DestroyScript(mContext, script);
          script = nsnull;
        }
      }
      if (!script)
        rv = NS_ERROR_OUT_OF_MEMORY;

      if (aVersion)
        ::JS_SetVersion(mContext, oldVersion);
    }
  }

  // Whew!  Finally done with these manually ref-counted things.
  JSPRINCIPALS_DROP(mContext, jsprin);
  return rv;
}

NS_IMETHODIMP
nsJSContext::ExecuteScript(void* aScriptObject,
                           void *aScopeObject,
                           nsAWritableString* aRetValue,
                           PRBool* aIsUndefined)
{
  if (!mScriptsEnabled) {
    *aIsUndefined = PR_TRUE;
    aRetValue->Truncate();
    return NS_OK;
  }

  nsresult rv;

  if (!aScopeObject)
    aScopeObject = ::JS_GetGlobalObject(mContext);

  // Push our JSContext on our thread's context stack, in case native code
  // called from JS calls back into JS via XPConnect.
  NS_WITH_SERVICE(nsIJSContextStack, stack, "nsThreadJSContextStack", &rv);
  if (NS_FAILED(rv) || NS_FAILED(stack->Push(mContext))) {
    return NS_ERROR_FAILURE;
  }

  // The result of evaluation, used only if there were no errors.  This need
  // not be a GC root currently, provided we run the GC only from the branch
  // callback or from ScriptEvaluated.  TODO: use JS_Begin/EndRequest to keep
  // the GC from racing with JS execution on any thread.
  jsval val;
  JSBool ok;

  mRef = nsnull;
  mTerminationFunc = nsnull;
  ok = ::JS_ExecuteScript(mContext,
                          (JSObject*) aScopeObject,
                          (JSScript*) ::JS_GetPrivate(mContext,
                                                    (JSObject*)aScriptObject),
                          &val);

  if (ok) {
    // If all went well, convert val to a string (XXXbe unless undefined?).
    if (aIsUndefined)
      *aIsUndefined = JSVAL_IS_VOID(val);
    if (aRetValue) {
      JSString* jsstring = ::JS_ValueToString(mContext, val);
      if (jsstring)
        aRetValue->Assign(NS_REINTERPRET_CAST(const PRUnichar*, ::JS_GetStringChars(jsstring)));
      else
        rv = NS_ERROR_OUT_OF_MEMORY;
    }
  } else {
    if (aIsUndefined)
      *aIsUndefined = PR_TRUE;
    if (aRetValue)
      aRetValue->Truncate();
  }

  ScriptEvaluated();

  // Pop here, after JS_ValueToString and any other possible evaluation.
  if (NS_FAILED(stack->Pop(nsnull)))
    rv = NS_ERROR_FAILURE;

  return rv;
}

const char *gEventArgv[] = {"event"};

void
AtomToEventHandlerName(nsIAtom *aName, char *charName, PRUint32 charNameSize)
{
  // optimized to avoid ns*Str*.h explicit/implicit copying and malloc'ing
  // even nsCAutoString may call an Append that copy-constructs an nsStr from
  // a const PRUnichar*
  const PRUnichar *name;
  aName->GetUnicode(&name);
  char c;
  PRUint32 i = 0;

  do {
    NS_ASSERTION(name[i] < 128, "non-ASCII event handler name");
    c = char(name[i]);

    // The HTML content sink must have folded to lowercase already.
    NS_ASSERTION(c == '\0' || ('a' <= c && c <= 'z'), "non-alphabetic event handler name");

    NS_ASSERTION(i < charNameSize, "overlong event handler name");
    charName[i++] = c;
  } while (c != '\0');
}

NS_IMETHODIMP
nsJSContext::CompileEventHandler(void *aTarget, nsIAtom *aName,
                                 const nsAReadableString& aBody,
                                 PRBool aShared, void** aHandler)
{
  JSPrincipals *jsprin = nsnull;

  nsCOMPtr<nsIScriptGlobalObject> global = getter_AddRefs(GetGlobalObject());
  if (global) {
    // XXXbe why the two-step QI? speed up via a new GetGlobalObjectData func?
    nsCOMPtr<nsIScriptObjectPrincipal> globalData = do_QueryInterface(global);
    if (globalData) {
      nsCOMPtr<nsIPrincipal> prin;
      if (NS_FAILED(globalData->GetPrincipal(getter_AddRefs(prin))))
        return NS_ERROR_FAILURE;
      prin->GetJSPrincipals(&jsprin);
    }
  }

  char charName[64];
  AtomToEventHandlerName(aName, charName, sizeof charName);

  JSObject *target = (JSObject*)aTarget;
  JSFunction* fun =
      ::JS_CompileUCFunctionForPrincipals(mContext, target, jsprin,
                                          charName, 1, gEventArgv,
                                          (jschar*)(const PRUnichar*)nsPromiseFlatString(aBody),
                                          aBody.Length(),
                                          //XXXbe filename, lineno:
                                          nsnull, 0);

  if (jsprin)
    JSPRINCIPALS_DROP(mContext, jsprin);
  if (!fun)
    return NS_ERROR_FAILURE;

  JSObject *handler = ::JS_GetFunctionObject(fun);
  if (aHandler)
    *aHandler = (void*) handler;

  if (aShared) {
    /* Break scope link to avoid entraining shared compilation scope. */
    ::JS_SetParent(mContext, handler, nsnull);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsJSContext::CompileFunction(void* aTarget,
                             const nsCString& aName,
                             PRUint32 aArgCount,
                             const char** aArgArray,
                             const nsAReadableString& aBody,
                             const char* aURL,
                             PRUint32 aLineNo,
                             PRBool aShared,
                             void** aFunctionObject)
{
  JSPrincipals *jsprin = nsnull;

  nsCOMPtr<nsIScriptGlobalObject> global = getter_AddRefs(GetGlobalObject());
  if (global) {
    // XXXbe why the two-step QI? speed up via a new GetGlobalObjectData func?
    nsCOMPtr<nsIScriptObjectPrincipal> globalData = do_QueryInterface(global);
    if (globalData) {
      nsCOMPtr<nsIPrincipal> prin;
      if (NS_FAILED(globalData->GetPrincipal(getter_AddRefs(prin))))
        return NS_ERROR_FAILURE;
      prin->GetJSPrincipals(&jsprin);
    }
  }

  JSObject *target = (JSObject*)aTarget;
  JSFunction* fun =
      ::JS_CompileUCFunctionForPrincipals(mContext, target, jsprin,
                                          aName, aArgCount, aArgArray,
                                          (jschar*)(const PRUnichar*)nsPromiseFlatString(aBody),
                                          aBody.Length(),
                                          aURL, aLineNo);

  if (jsprin)
    JSPRINCIPALS_DROP(mContext, jsprin);
  if (!fun)
    return NS_ERROR_FAILURE;

  JSObject *handler = ::JS_GetFunctionObject(fun);
  if (aFunctionObject)
    *aFunctionObject = (void*) handler;

  // Prevent entraining just like CompileEventHandler does?
  if (aShared) {
    /* Break scope link to avoid entraining shared compilation scope. */
    ::JS_SetParent(mContext, handler, nsnull);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsJSContext::CallEventHandler(void *aTarget, void *aHandler, PRUint32 argc,
                              void *argv, PRBool *aBoolResult, PRBool aReverseReturnResult)
{
  // This one's a lot easier than EvaluateString because we don't have to
  // hassle with principals: they're already compiled into the JS function.
  nsresult rv;
  nsCOMPtr<nsIScriptSecurityManager> securityManager;
  rv = GetSecurityManager(getter_AddRefs(securityManager));
  if (NS_FAILED(rv))
    return NS_ERROR_FAILURE;

  NS_WITH_SERVICE(nsIJSContextStack, stack, "nsThreadJSContextStack", &rv);
  if (NS_FAILED(rv) || NS_FAILED(stack->Push(mContext)))
    return NS_ERROR_FAILURE;

  // this context can be deleted unexpectedly if the JS closes
  // the owning window. we ran into this problem specifically
  // when going through the "close window" key event handler
  // (that is, hitting ^W on Windows). the addref just below
  // prevents our untimely destruction.
  nsCOMPtr<nsIScriptContext> kungFuDeathGrip(this);
  mRef = nsnull;
  mTerminationFunc = nsnull;

  // check if the event handler can be run on the object in question
  rv = securityManager->CheckFunctionAccess(mContext, aHandler, aTarget);

  if (NS_SUCCEEDED(rv)) {
    jsval val;
    jsval funval = OBJECT_TO_JSVAL(aHandler);
    PRBool ok = ::JS_CallFunctionValue(mContext, (JSObject *)aTarget, funval,
                                argc, (jsval *)argv, &val);
    *aBoolResult = ok
                   ? !JSVAL_IS_BOOLEAN(val) || (aReverseReturnResult ? !JSVAL_TO_BOOLEAN(val) : JSVAL_TO_BOOLEAN(val))
                   : JS_TRUE;

    ScriptEvaluated();
  }

  if (NS_FAILED(stack->Pop(nsnull)))
    return NS_ERROR_FAILURE;

  return NS_OK;
}

NS_IMETHODIMP
nsJSContext::BindCompiledEventHandler(void *aTarget, nsIAtom *aName, void *aHandler)
{
  char charName[64];
  AtomToEventHandlerName(aName, charName, sizeof charName);

  JSObject *funobj = (JSObject*) aHandler;
  JSObject *target = (JSObject*) aTarget;

  // Make sure the handler function is parented by its event target object
  if (funobj && ::JS_GetParent(mContext, funobj) != target) {
    funobj = ::JS_CloneFunctionObject(mContext, funobj, target);
    if (!funobj)
      return NS_ERROR_OUT_OF_MEMORY;
  }

  if (!::JS_DefineProperty(mContext, target, charName,
                           OBJECT_TO_JSVAL(funobj), nsnull, nsnull,
                           JSPROP_ENUMERATE | JSPROP_PERMANENT)) {
    return NS_ERROR_FAILURE;
  }
  return NS_OK;
}

NS_IMETHODIMP
nsJSContext::SetDefaultLanguageVersion(const char* aVersion)
{
  (void) ::JS_SetVersion(mContext, ::JS_StringToVersion(aVersion));
  return NS_OK;
}

NS_IMETHODIMP_(nsIScriptGlobalObject*)
nsJSContext::GetGlobalObject()
{
  JSObject *global = ::JS_GetGlobalObject(mContext);
  nsIScriptGlobalObject *script_global = nsnull;

  if (nsnull != global) {
    nsISupports* sup = (nsISupports *)::JS_GetPrivate(mContext, global);
    if (nsnull != sup) {
      sup->QueryInterface(NS_GET_IID(nsIScriptGlobalObject), (void**) &script_global);
    }
    return script_global;
  }
  return nsnull;
}

NS_IMETHODIMP_(void*)
nsJSContext::GetNativeContext()
{
  return (void *)mContext;
}


NS_IMETHODIMP
nsJSContext::InitContext(nsIScriptGlobalObject *aGlobalObject)
{
  if (!mContext)
    return NS_ERROR_OUT_OF_MEMORY;

  nsresult rv;
  nsCOMPtr<nsIScriptObjectOwner> owner = do_QueryInterface(aGlobalObject, &rv);
  JSObject *global;
  mIsInitialized = PR_FALSE;

  if (NS_SUCCEEDED(rv)) {
    rv = owner->GetScriptObject(this, (void **)&global);

    // init standard classes
    if (NS_SUCCEEDED(rv) && ::JS_InitStandardClasses(mContext, global)) {
      ::JS_SetGlobalObject(mContext, global);
      rv = InitClasses(); // this will complete the global object initialization
    }
    // XXX there ought to be an else here!

    if (NS_SUCCEEDED(rv)) {
      ::JS_SetErrorReporter(mContext, NS_ScriptErrorReporter);
    }
  }

  return rv;
}

nsresult
nsJSContext::InitializeExternalClasses()
{
  nsresult rv;
  NS_WITH_SERVICE(nsIScriptNameSetRegistry, registry, kCScriptNameSetRegistryCID, &rv);
  if (NS_SUCCEEDED(rv)) {
    rv = registry->InitializeClasses(this);
  }
  return rv;
}

nsresult
nsJSContext::InitializeLiveConnectClasses()
{
  nsresult rv = NS_OK;
  NS_WITH_SERVICE(nsIJVMManager, jvmManager, nsIJVMManager::GetCID(), &rv);
  if (NS_SUCCEEDED(rv) && jvmManager != nsnull) {
    PRBool javaEnabled = PR_FALSE;
    if (NS_SUCCEEDED(jvmManager->IsJavaEnabled(&javaEnabled)) && javaEnabled) {
      nsCOMPtr<nsILiveConnectManager> liveConnectManager = do_QueryInterface(jvmManager);
      if (liveConnectManager) {
        rv = liveConnectManager->InitLiveConnectClasses(mContext, ::JS_GetGlobalObject(mContext));
      }
    }
  }

  // return all is well until things are stable.
  return NS_OK;
}

#ifdef NS_TRACE_MALLOC

#include <errno.h>              // XXX assume Linux if NS_TRACE_MALLOC
#include <fcntl.h>
#include <unistd.h>
#include "nsTraceMalloc.h"

static JSBool
TraceMallocDisable(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    NS_TraceMallocDisable();
    return JS_TRUE;
}

static JSBool
TraceMallocEnable(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    NS_TraceMallocEnable();
    return JS_TRUE;
}

static JSBool
TraceMallocOpenLogFile(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    int fd;
    JSString *str;
    char *filename;

    if (argc == 0) {
        fd = -1;
    } else {
        str = JS_ValueToString(cx, argv[0]);
        if (!str)
            return JS_FALSE;
        filename = JS_GetStringBytes(str);
        fd = open(filename, O_CREAT | O_WRONLY | O_TRUNC, 0644);
        if (fd < 0) {
            JS_ReportError(cx, "can't open %s: %s", filename, strerror(errno));
            return JS_FALSE;
        }
    }
    *rval = INT_TO_JSVAL(fd);
    return JS_TRUE;
}

static JSBool
TraceMallocChangeLogFD(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    int fd, oldfd;

    if (argc == 0) {
        oldfd = -1;
    } else {
        if (!JS_ValueToECMAInt32(cx, argv[0], &fd))
            return JS_FALSE;
        oldfd = NS_TraceMallocChangeLogFD(fd);
        if (oldfd == -2) {
            JS_ReportOutOfMemory(cx);
            return JS_FALSE;
        }
    }
    *rval = INT_TO_JSVAL(oldfd);
    return JS_TRUE;
}

static JSBool
TraceMallocCloseLogFD(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    int32 fd;

    if (argc == 0)
        return JS_TRUE;
    if (!JS_ValueToECMAInt32(cx, argv[0], &fd))
        return JS_FALSE;
    NS_TraceMallocCloseLogFD((int) fd);
    return JS_TRUE;
}

static JSBool
TraceMallocLogTimestamp(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSString *str;
    const char *caption;

    str = JS_ValueToString(cx, argv[0]);
    if (!str)
        return JS_FALSE;
    caption = JS_GetStringBytes(str);
    NS_TraceMallocLogTimestamp(caption);
    return JS_TRUE;
}

static JSFunctionSpec TraceMallocFunctions[] = {
    {"TraceMallocDisable",        TraceMallocDisable,       0, 0, 0},
    {"TraceMallocEnable",         TraceMallocEnable,        0, 0, 0},
    {"TraceMallocOpenLogFile",    TraceMallocOpenLogFile,   1, 0, 0},
    {"TraceMallocChangeLogFD",    TraceMallocChangeLogFD,   1, 0, 0},
    {"TraceMallocCloseLogFD",     TraceMallocCloseLogFD,    1, 0, 0},
    {"TraceMallocLogTimestamp",   TraceMallocLogTimestamp,  1, 0, 0},
    {NULL,                        NULL,                     0, 0, 0}
};

#endif /* NS_TRACE_MALLOC */

NS_IMETHODIMP
nsJSContext::InitClasses()
{
  nsresult rv = NS_ERROR_FAILURE;
  nsCOMPtr<nsIScriptGlobalObject> global = dont_AddRef(GetGlobalObject());

  if (NS_OK == NS_InitWindowClass(this, global) &&
      NS_OK == NS_InitNodeClass(this, nsnull) &&
      NS_OK == NS_InitElementClass(this, nsnull) &&
      NS_OK == NS_InitDocumentClass(this, nsnull) &&
      NS_OK == NS_InitTextClass(this, nsnull) &&
      NS_OK == NS_InitAttrClass(this, nsnull) &&
      NS_OK == NS_InitNamedNodeMapClass(this, nsnull) &&
      NS_OK == NS_InitNodeListClass(this, nsnull) &&
      NS_OK == NS_InitKeyEventClass(this, nsnull) &&
      NS_OK == InitializeLiveConnectClasses() &&
      NS_OK == InitializeExternalClasses() &&
      // XXX Temporarily here. These shouldn't be hardcoded.
      NS_OK == NS_InitHTMLImageElementClass(this, nsnull) &&
      NS_OK == NS_InitHTMLOptionElementClass(this, nsnull)) {
    rv = NS_OK;
  }

  // Initialize XPConnect classes
  if (NS_SUCCEEDED(rv)) {
    NS_WITH_SERVICE(nsIXPConnect, xpc, nsIXPConnect::GetCID(), &rv);
    if (NS_SUCCEEDED(rv)) {
      rv = xpc->InitClasses(mContext, ::JS_GetGlobalObject(mContext));
    }
    NS_ASSERTION(NS_SUCCEEDED(rv), "unable to init xpconnect classes");
  }

#ifdef NS_TRACE_MALLOC
  // Attempt to initialize TraceMalloc functions
  ::JS_DefineFunctions(mContext, ::JS_GetGlobalObject(mContext),
                       TraceMallocFunctions);
#endif

  mIsInitialized = PR_TRUE;
  return rv;
}

NS_IMETHODIMP
nsJSContext::IsContextInitialized()
{
  return (mIsInitialized) ? NS_OK : NS_ERROR_NOT_INITIALIZED;
}

NS_IMETHODIMP
nsJSContext::AddNamedReference(void *aSlot, void *aScriptObject, const char *aName)
{
  return (::JS_AddNamedRoot(mContext, aSlot, aName)) ? NS_OK : NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsJSContext::RemoveReference(void *aSlot, void *aScriptObject)
{
  return (::JS_RemoveRoot(mContext, aSlot)) ? NS_OK : NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsJSContext::GC()
{
  ::JS_GC(mContext);
  return NS_OK;
}

NS_IMETHODIMP
nsJSContext::ScriptEvaluated(void)
{
  if (mTerminationFunc) {
    (*mTerminationFunc)(mRef);
    mRef = nsnull;
    mTerminationFunc = nsnull;
  }

  mNumEvaluations++;

  if (mNumEvaluations > 20) {
    mNumEvaluations = 0;
    ::JS_MaybeGC(mContext);
  }

  mBranchCallbackCount = 0;

  return NS_OK;
}

NS_IMETHODIMP
nsJSContext::GetNameSpaceManager(nsIScriptNameSpaceManager** aInstancePtr)
{
  nsresult rv = NS_OK;

  if (!mNameSpaceManager.get()) {
    rv = NS_NewScriptNameSpaceManager(getter_AddRefs(mNameSpaceManager));
    if (NS_SUCCEEDED(rv)) {
      // XXXbe used progid rather than CID?
      NS_WITH_SERVICE(nsIScriptNameSetRegistry, registry, kCScriptNameSetRegistryCID, &rv);
      if (NS_SUCCEEDED(rv)) {
        rv = registry->PopulateNameSpace(this);
      }
    }
  }

  *aInstancePtr = mNameSpaceManager;
  NS_ADDREF(*aInstancePtr);
  return rv;
}

NS_IMETHODIMP
nsJSContext::GetSecurityManager(nsIScriptSecurityManager **aInstancePtr)
{
  if (!mSecurityManager) {
    nsresult rv = nsServiceManager::GetService(kScriptSecurityManagerProgID,
                                               NS_GET_IID(nsIScriptSecurityManager),
                                               (nsISupports**)&mSecurityManager);
    if (NS_FAILED(rv))
      return NS_ERROR_FAILURE;
  }
  *aInstancePtr = mSecurityManager;
  NS_ADDREF(*aInstancePtr);
  return NS_OK;
}

NS_IMETHODIMP
nsJSContext::SetOwner(nsIScriptContextOwner* owner)
{
  // The owner should not be addrefed!! We'll be told
  // when the owner goes away.
  mOwner = owner;

  return NS_OK;
}

NS_IMETHODIMP
nsJSContext::GetOwner(nsIScriptContextOwner** owner)
{
  *owner = mOwner;
  NS_IF_ADDREF(*owner);

  return NS_OK;
}

NS_IMETHODIMP
nsJSContext::SetTerminationFunction(nsScriptTerminationFunc aFunc,
                                    nsISupports* aRef)
{
  mTerminationFunc = aFunc;
  mRef = aRef;

  return NS_OK;
}

NS_IMETHODIMP
nsJSContext::GetScriptsEnabled(PRBool *aEnabled)
{
  *aEnabled = mScriptsEnabled;
  return NS_OK;
}

NS_IMETHODIMP
nsJSContext::SetScriptsEnabled(PRBool aEnabled)
{
  mScriptsEnabled = aEnabled;
  return NS_OK;
}

nsJSEnvironment *nsJSEnvironment::sTheEnvironment = nsnull;

nsJSEnvironment *
nsJSEnvironment::GetScriptingEnvironment()
{
  if (nsnull == sTheEnvironment) {
    sTheEnvironment = new nsJSEnvironment();
    NS_IF_ADDREF(sTheEnvironment); // released in |Observe|
  }
  return sTheEnvironment;
}

const char kJSRuntimeServiceProgID[] = "nsJSRuntimeService";
static int globalCount;
static PRThread *gDOMThread;

static JSBool PR_CALLBACK
DOMGCCallback(JSContext *cx, JSGCStatus status)
{
  if (status == JSGC_BEGIN && PR_GetCurrentThread() != gDOMThread)
    return JS_FALSE;
  return JS_TRUE;
}

nsJSEnvironment::nsJSEnvironment()
{
  NS_INIT_ISUPPORTS();

  // So that we get deleted on XPCOM shutdown, set up an
  // observer.
  nsresult rv;
  NS_WITH_SERVICE(nsIObserverService, observerService, NS_OBSERVERSERVICE_PROGID, &rv);
  NS_ASSERTION(NS_SUCCEEDED(rv), "going to leak a nsJSEnvironment");
  if (NS_SUCCEEDED(rv))
  {
    nsAutoString topic;
    topic.AssignWithConversion(NS_XPCOM_SHUTDOWN_OBSERVER_ID);
    observerService->AddObserver(this,topic.GetUnicode());
  }

  mRuntimeService = nsnull;
  rv = nsServiceManager::GetService(kJSRuntimeServiceProgID,
                                    NS_GET_IID(nsIJSRuntimeService),
                                    (nsISupports**)&mRuntimeService);
  // get the JSRuntime from the runtime svc, if possible
  if (NS_FAILED(rv))
    return;                     // XXX swallow error! need Init()?
  rv = mRuntimeService->GetRuntime(&mRuntime);
  if (NS_FAILED(rv))
    return;                     // XXX swallow error! need Init()?

  gDOMThread = PR_GetCurrentThread();
  ::JS_SetGCCallbackRT(mRuntime, DOMGCCallback);

  // Initialize LiveConnect.  XXXbe use progid rather than GetCID
  NS_WITH_SERVICE(nsILiveConnectManager, manager,
                  nsIJVMManager::GetCID(), &rv);

  // Should the JVM manager perhaps define methods for starting up LiveConnect?
  if (NS_SUCCEEDED(rv) && manager != nsnull) {
    PRBool started = PR_FALSE;
    rv = manager->StartupLiveConnect(mRuntime, started);
  }

  globalCount++;
}

nsJSEnvironment::~nsJSEnvironment()
{
  if (--globalCount == 0)
    nsJSUtils::nsClearCachedSecurityManager();
  if (mRuntimeService)
    nsServiceManager::ReleaseService(kJSRuntimeServiceProgID, mRuntimeService);
}

NS_IMPL_ISUPPORTS1(nsJSEnvironment,nsIObserver);

NS_IMETHODIMP nsJSEnvironment::Observe(nsISupports *aSubject, 
                                       const PRUnichar *aTopic,
                                       const PRUnichar *someData)
{
#ifdef DEBUG
  nsAutoString topic;
  topic.AssignWithConversion(NS_XPCOM_SHUTDOWN_OBSERVER_ID);
  NS_ASSERTION(topic.EqualsWithConversion(aTopic), "not shutdown");
#endif
  NS_RELEASE_THIS(); // release ref from |GetScriptingEnvironment|
  return NS_OK;
}

nsIScriptContext* nsJSEnvironment::GetNewContext()
{
  nsIScriptContext *context;
  context = new nsJSContext(mRuntime);
  NS_ADDREF(context);
  return context;
}

extern "C" NS_DOM nsresult NS_CreateScriptContext(nsIScriptGlobalObject *aGlobal,
                                                  nsIScriptContext **aContext)
{
  nsJSEnvironment *environment = nsJSEnvironment::GetScriptingEnvironment();
  if (!environment)
    return NS_ERROR_OUT_OF_MEMORY;

  nsIScriptContext *scriptContext = environment->GetNewContext();
  if (!scriptContext)
    return NS_ERROR_OUT_OF_MEMORY;
  *aContext = scriptContext;

  // Bind the script context and the global object
  scriptContext->InitContext(aGlobal);
  aGlobal->SetContext(scriptContext);

  return NS_OK;
}

