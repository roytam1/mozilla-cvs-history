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

#include "prthread.h"
#include "pprthred.h"
#include "plstr.h"
#include "jni.h"
#include "jsjava.h"
#include "jsdbgapi.h"
#include "libmocha.h"
#include "libevent.h"
#include "nsJVMManager.h"
#include "nsPluginInstancePeer.h"
#include "npglue.h"
#include "nsCCapsManager.h"
#include "prinrval.h"
#include "ProxyJNI.h"
#include "prcmon.h"
#include "nsCSecurityContext.h"
#include "nsISecurityContext.h"
#include "xpgetstr.h"
#include "lcglue.h"
extern "C" int XP_PROGRESS_STARTING_JAVA;
extern "C" int XP_PROGRESS_STARTING_JAVA_DONE;
extern "C" int XP_JAVA_NO_CLASSES;
extern "C" int XP_JAVA_GENERAL_FAILURE;
extern "C" int XP_JAVA_STARTUP_FAILED;
extern "C" int XP_JAVA_DEBUGGER_FAILED;


/**
 * Template based Thread Local Storage.
 */
template <class T>
class ThreadLocalStorage {
public:
	ThreadLocalStorage(PRThreadPrivateDTOR dtor) : mIndex(0), mValid(PR_FALSE)
	{
		mValid = (PR_NewThreadPrivateIndex(&mIndex, dtor) == PR_SUCCESS);
	}
	
	void set(T value)
	{
		if (mValid) PR_SetThreadPrivate(mIndex, value);
	}
	
	T get()
	{
		return (T) (mValid ? PR_GetThreadPrivate(mIndex) : 0);
	}

private:
	PRUintn mIndex;
	PRBool mValid;
};


static void PR_CALLBACK detach_JVMContext(void* storage)
{
	JVMContext* context = (JVMContext*)storage;
	
	JNIEnv* proxyEnv = context->proxyEnv;
	if (proxyEnv != NULL) {
		DeleteProxyJNI(proxyEnv);
		context->proxyEnv = NULL;
	}
	
	delete storage;
}

JVMContext* GetJVMContext()
{
	/* Use NSPR thread private data to manage the per-thread JNIEnv* association. */
	static ThreadLocalStorage<JVMContext*> localContext(&detach_JVMContext);
	JVMContext* context = localContext.get();
	if (context == NULL) {
		context = new JVMContext;
		context->proxyEnv = NULL;
		context->securityStack = NULL;
		context->jsj_env = NULL;
		context->js_context = NULL;
		context->js_startframe = NULL;
		localContext.set(context);
	}
	return context;
}

static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);
static NS_DEFINE_IID(kIJVMManagerIID, NS_IJVMMANAGER_IID);
static NS_DEFINE_IID(kIThreadManagerIID, NS_ITHREADMANAGER_IID);
static NS_DEFINE_IID(kIJVMPluginIID, NS_IJVMPLUGIN_IID);
static NS_DEFINE_IID(kISymantecDebugManagerIID, NS_ISYMANTECDEBUGMANAGER_IID);
static NS_DEFINE_IID(kIJVMPluginInstanceIID, NS_IJVMPLUGININSTANCE_IID);
static NS_DEFINE_IID(kIJVMPluginTagInfoIID, NS_IJVMPLUGINTAGINFO_IID);
static NS_DEFINE_IID(kIPluginTagInfo2IID, NS_IPLUGINTAGINFO2_IID);
static NS_DEFINE_IID(kIPluginManagerIID, NS_IPLUGINMANAGER_IID);
static NS_DEFINE_IID(kIJVMConsoleIID, NS_IJVMCONSOLE_IID);
static NS_DEFINE_IID(kISymantecDebuggerIID, NS_ISYMANTECDEBUGGER_IID);
static NS_DEFINE_IID(kISecurityContextIID, NS_ISECURITYCONTEXT_IID);


////////////////////////////////////////////////////////////////////////////////
// LiveConnect callbacks
////////////////////////////////////////////////////////////////////////////////

PR_BEGIN_EXTERN_C

#include "jscntxt.h"


static JSContext* PR_CALLBACK
map_jsj_thread_to_js_context_impl(JSJavaThreadState *jsj_env, JNIEnv *env, char **errp)
{
    JSContext *cx    = LM_GetCrippledContext();

    *errp = NULL;
#if 0    
    nsJVMManager* pJVMMgr = JVM_GetJVMMgr();
    if (pJVMMgr != NULL) {
        nsIJVMPlugin* pJVMPI = pJVMMgr->GetJVMPlugin();
        jvmMochaPrefsEnabled = LM_GetMochaEnabled();
        if (pJVMPI != NULL) {
            nsIPluginInstance* pPIT;
            nsresult err = pJVMPI->GetPluginInstance(env, &pPIT);
            if ( (err == NS_OK) && (pPIT != NULL) ) {
                nsIJVMPluginInstance* pJVMPIT;
                if (pPIT->QueryInterface(kIJVMPluginInstanceIID,
                                         (void**)&pJVMPIT) == NS_OK) {
                    nsPluginInstancePeer* pPITP;
                    err = pJVMPIT->GetPeer((nsIPluginInstancePeer**)&pPITP); 
                    if ( (err == NS_OK) &&(pPITP != NULL) ) {
                        cx = pPITP->GetJSContext();
                        pPITP->Release();
                    }
                    pJVMPIT->Release();
                }
                pPIT->Release();
            }
            // pJVMPI->Release(); // GetJVMPlugin no longer calls AddRef
        }
        pJVMMgr->Release();
    }
    if (jvmMochaPrefsEnabled == PR_FALSE)
    {
        *errp = strdup("Java preference is disabled");
        return NULL;
    }

    if ( cx == NULL )
    {
        *errp = strdup("Java thread could not be associated with a JSContext");
        return NULL;
    }
#endif
    return cx;
}

static void PR_CALLBACK detach_jsjava_thread_state(void* env)
{
    JSJavaThreadState *jsj_env = (JSJavaThreadState*)env;
    JSJ_DetachCurrentThreadFromJava(jsj_env);
}

/*
** This callback is called to map a JSContext to a JSJavaThreadState which
** is a wrapper around JNIEnv. Hence this callback essentially maps a JSContext
** to a java thread. JSJ_AttachCurrentThreadToJava just calls AttachCurrentThread
** on the java vm.
*/
static JSJavaThreadState* PR_CALLBACK
map_js_context_to_jsj_thread_impl(JSContext *cx, char **errp)
{
	*errp = NULL;

	JVMContext* context = GetJVMContext();
	JSJavaThreadState* jsj_env = context->jsj_env;
	if (jsj_env != NULL)
		return jsj_env;
    
	if (ET_InitMoja(0) != LM_MOJA_OK) {
		*errp = strdup("LiveConnect initialization failed.");
		return NULL;
	}

	JSJavaVM* js_jvm = NULL;
	nsJVMManager* pJVMMgr = JVM_GetJVMMgr();
	if (pJVMMgr != NULL) {
		js_jvm = pJVMMgr->GetJSJavaVM();
		pJVMMgr->Release();
		if (js_jvm == NULL) {
			*errp = strdup("Failed to attach to a Java VM.");
			return NULL;
		}
	}

	jsj_env = JSJ_AttachCurrentThreadToJava(js_jvm, NULL, NULL);
	context->jsj_env = jsj_env;
	context->js_context = cx;

	return jsj_env;
}

/*
** This callback is called to map a applet,bean to its corresponding JSObject
** created on javascript side and then map that to a java wrapper JSObject class.
** This callback is called in JSObject.getWindow implementation to get
** a java wrapper JSObject class for a applet only once.
** Note that once a mapping between applet -> javascript JSObject -> Java wrapper JSObject 
** is made, all subsequent method calls via JSObject use the internal field
** to get to the javascript JSObject.
*/
static JSObject* PR_CALLBACK
map_java_object_to_js_object_impl(JNIEnv *env, void *pNSIPluginInstanceIn, char **errp)
{
    MWContext       *cx;
    JSObject        *window;
    MochaDecoder    *decoder; 
    PRBool           mayscript = PR_FALSE;
    PRBool           jvmMochaPrefsEnabled = PR_FALSE;
    nsresult         err = NS_OK;

    *errp = NULL;
    /* XXX assert JS is locked */

    if (!pNSIPluginInstanceIn) {
        env->ThrowNew(env->FindClass("java/lang/NullPointerException"),0);
        return 0;
    }

    nsJVMManager* pJVMMgr = JVM_GetJVMMgr();
    if (pJVMMgr != NULL) {
        nsIJVMPlugin* pJVMPI = pJVMMgr->GetJVMPlugin();
        jvmMochaPrefsEnabled = LM_GetMochaEnabled();
        if (pJVMPI != NULL) {
            //jobject javaObject = applet;
            nsIPluginInstance* pPIT;
            //nsresult err = pJVMPI->GetPluginInstance(javaObject, &pPIT);
            pPIT = (nsIPluginInstance*)pNSIPluginInstanceIn;
            if ( (err == NS_OK) && (pPIT != NULL) ) {
                nsIJVMPluginInstance* pJVMPIT;
                if (pPIT->QueryInterface(kIJVMPluginInstanceIID,
                                         (void**)&pJVMPIT) == NS_OK) {
                    nsPluginInstancePeer* pPITP;
                    err = pJVMPIT->GetPeer((nsIPluginInstancePeer**)&pPITP); 
                    if ( (err == NS_OK) &&(pPITP != NULL) ) {
                        nsIJVMPluginTagInfo* pJVMTagInfo;
                        if (pPITP->QueryInterface(kIJVMPluginTagInfoIID,
                                                  (void**)&pJVMTagInfo) == NS_OK) {
                            err = pJVMTagInfo->GetMayScript(&mayscript);
                            PR_ASSERT(err != NS_OK ? mayscript == PR_FALSE : PR_TRUE);
                            pJVMTagInfo->Release();
                        }
                        cx = pPITP->GetMWContext();
                        pPITP->Release();
                    }
                    pJVMPIT->Release();
                }
            }
            // pJVMPI->Release(); // GetJVMPlugin no longer calls AddRef
        }
        pJVMMgr->Release();
    }

    if (  (mayscript            == PR_FALSE)
          ||(jvmMochaPrefsEnabled == PR_FALSE)
        )
    {
        *errp = strdup("JSObject.getWindow() requires mayscript attribute on this Applet or java preference is disabled");
        goto except;
    }


    if (!cx || (cx->type != MWContextBrowser && cx->type != MWContextPane))
    {
        *errp = strdup("JSObject.getWindow() can only be called in MWContextBrowser or MWContextPane");
        return 0;
    }

    decoder = LM_GetMochaDecoder(cx);

    /* if there is a decoder now, reflect the window */
    if (decoder && (jvmMochaPrefsEnabled == PR_TRUE)) {
        window = decoder->window_object;
    }

    LM_PutMochaDecoder(decoder);

    return window;
  except:
    return 0;
}

#if 0
static JavaVM* PR_CALLBACK
get_java_vm_impl(char **errp)
{
    *errp = NULL;
    JavaVM *pJavaVM = NULL;
    
    nsJVMManager* pJVMMgr = JVM_GetJVMMgr();
    if (pJVMMgr != NULL) {
        nsIJVMPlugin* pJVMPI = pJVMMgr->GetJVMPlugin();
        if (pJVMPI != NULL) {
            nsresult err = pJVMPI->GetJavaVM(&pJavaVM);
            PR_ASSERT(err != NS_OK ? pJavaVM == NULL : PR_TRUE);
            // pJVMPI->Release(); // GetJVMPlugin no longer calls AddRef
        }
        pJVMMgr->Release();
    }
    if ( pJavaVM == NULL )
    {
        *errp = strdup("Could not find a JavaVM to attach to.");
    }
    return pJavaVM;
}
#endif

void* 
ConvertNSIPrincipalToNSPrincipalArray(JNIEnv *pJNIEnv, JSContext *pJSContext, void  **ppNSIPrincipalArrayIN, int numPrincipals, void *pNSISecurityContext)
{
    nsIPrincipal  **ppNSIPrincipalArray = (nsIPrincipal  **)ppNSIPrincipalArrayIN;
    PRInt32        length = numPrincipals;
    nsresult       err    = NS_OK;
    nsJVMManager* pJVMMgr = JVM_GetJVMMgr();
    void          *pNSPrincipalArray = NULL;
    if (pJVMMgr != NULL) {
         if (ppNSIPrincipalArray != NULL) {
             nsIPluginManager *pNSIPluginManager = NULL;
             NS_DEFINE_IID(kIPluginManagerIID, NS_IPLUGINMANAGER_IID);
             err = pJVMMgr->QueryInterface(kIPluginManagerIID,
                                  (void**)&pNSIPluginManager);

             if(   (err == NS_OK) 
                && (pNSIPluginManager != NULL )
               )
             {
               nsCCapsManager *pNSCCapsManager = NULL;
               NS_DEFINE_IID(kICapsManagerIID, NS_ICAPSMANAGER_IID);
               err = pNSIPluginManager->QueryInterface(kICapsManagerIID, (void**)&pNSCCapsManager);
               if(   (err == NS_OK) 
                  && (pNSCCapsManager != NULL)
                 )
               {
                 PRInt32 i=0;
                 nsPrincipal *pNSPrincipal = NULL;
                 pNSPrincipalArray = nsCapsNewPrincipalArray(length);
                 if (pNSPrincipalArray != NULL) 
                 {
                   while( i<length )
                   {
                      err = pNSCCapsManager->GetNSPrincipal(ppNSIPrincipalArray[i], &pNSPrincipal);
                      nsCapsSetPrincipalArrayElement(pNSPrincipalArray, i, pNSPrincipal);
                      i++;
                   }
                 }
                 pNSCCapsManager->Release();
               }
               pNSIPluginManager->Release();
             }
         }
      pJVMMgr->Release();
    }
    if (  (pNSPrincipalArray != NULL)
        &&(length != 0)
       )
    {
        return pNSPrincipalArray;
    }

    return NULL;
}




static JSPrincipals* PR_CALLBACK
get_JSPrincipals_from_java_caller_impl(JNIEnv *pJNIEnv, JSContext *pJSContext, void  **ppNSIPrincipalArrayIN, int numPrincipals, void *pNSISecurityContext)
{
    void *pNSPrincipalArray  = ConvertNSIPrincipalToNSPrincipalArray(pJNIEnv, pJSContext, ppNSIPrincipalArrayIN, numPrincipals, pNSISecurityContext);
    if (pNSPrincipalArray != NULL)
    {
        return LM_GetJSPrincipalsFromJavaCaller(pJSContext, pNSPrincipalArray, pNSISecurityContext);
    }

    return NULL;
}

static jobject PR_CALLBACK
get_java_wrapper_impl(JNIEnv *pJNIEnv, jint jsobject)
{
    nsresult       err    = NS_OK;
    jobject  pJSObjectWrapper = NULL;
    nsJVMManager* pJVMMgr = JVM_GetJVMMgr();
    if (pJVMMgr != NULL) {
      nsIJVMPlugin* pJVMPI = pJVMMgr->GetJVMPlugin();
      if (pJVMPI != NULL) {
         err = pJVMPI->GetJavaWrapper(pJNIEnv, jsobject, &pJSObjectWrapper);
         //pJVMPI->Release();
      }
      pJVMMgr->Release();
    }
    if ( err != NS_OK )
    {
       return NULL;
    }
    return pJSObjectWrapper;
}

static JSBool PR_CALLBACK
enter_js_from_java_impl(JNIEnv *jEnv, char **errp,
                        void **pNSIPrincipaArray, int numPrincipals, void *pNSISecurityContext)
{
	JVMContext* context = GetJVMContext();
	MWContext *cx = XP_FindSomeContext();   /* XXXMLM */
	JSContext *pJSCX = context->js_context;

	LM_LockJS(cx, errp);
	if (pJSCX == NULL) {
		pJSCX = LM_GetCrippledContext();
	}

	// Setup tls to maintain a stack of security contexts.
	if ((pNSIPrincipaArray != NULL) && (pNSISecurityContext != NULL)) {
		JVMSecurityStack *pSecInfoNew    = new JVMSecurityStack;
		pSecInfoNew->pNSIPrincipaArray   = pNSIPrincipaArray;
		pSecInfoNew->numPrincipals       = numPrincipals;
		pSecInfoNew->pNSISecurityContext = pNSISecurityContext;
		pSecInfoNew->prev                = pSecInfoNew;
		pSecInfoNew->next                = pSecInfoNew;
		JSStackFrame *fp                 = NULL;
		pSecInfoNew->pJavaToJSFrame      = JS_FrameIterator(pJSCX, &fp);
		pSecInfoNew->pJSToJavaFrame      = NULL;

		JVMSecurityStack *pSecInfoBottom = context->securityStack;
		if (pSecInfoBottom == NULL) {
			context->securityStack = pSecInfoNew;
		} else {
			pSecInfoBottom->prev->next = pSecInfoNew;
			pSecInfoNew->prev          = pSecInfoBottom->prev;
			pSecInfoNew->next          = pSecInfoBottom;
			pSecInfoBottom->prev       = pSecInfoNew;
		}
	}
	return PR_TRUE;
	//return LM_LockJS(cx, errp);
}

static void PR_CALLBACK
exit_js_impl(JNIEnv *jEnv)
{
    MWContext *cx = XP_FindSomeContext();   /* XXXMLM */

    LM_UnlockJS(cx);

    // Pop the security context stack
    JVMContext* context = GetJVMContext();
    JVMSecurityStack *pSecInfoBottom = context->securityStack;
    if (pSecInfoBottom != NULL)
    {
      if(pSecInfoBottom->next == pSecInfoBottom)
      {
        context->securityStack = NULL;
        pSecInfoBottom->next   = NULL;            
        pSecInfoBottom->prev   = NULL;            
        delete pSecInfoBottom;
      }
      else
      {
        JVMSecurityStack *top = pSecInfoBottom->prev;
        top->next        = NULL;            
        pSecInfoBottom->prev = top->prev;        
        top->prev->next  = pSecInfoBottom;
        top->prev        = NULL;
        delete top;
      }
    }

    return;
}

static PRBool PR_CALLBACK
create_java_vm_impl(SystemJavaVM* *jvm, JNIEnv* *initialEnv, void* initargs)
{
    const char* classpath = (const char*)initargs;      // unused (should it be?)
    *jvm = (SystemJavaVM*)JVM_GetJVMMgr();              // unused in the browser
    *initialEnv = JVM_GetJNIEnv();
    return (*jvm != NULL && *initialEnv != NULL);
}

static PRBool PR_CALLBACK
destroy_java_vm_impl(SystemJavaVM* jvm, JNIEnv* initialEnv)
{
    JVM_ReleaseJNIEnv(initialEnv);
    // need to release jvm
    return PR_TRUE;
}

static JNIEnv* PR_CALLBACK
attach_current_thread_impl(SystemJavaVM* jvm)
{
    return JVM_GetJNIEnv();
}

static PRBool PR_CALLBACK
detach_current_thread_impl(SystemJavaVM* jvm, JNIEnv* env)
{
    JVM_ReleaseJNIEnv(env);
    return PR_TRUE;
}

static SystemJavaVM* PR_CALLBACK
get_java_vm_impl(JNIEnv* env)
{
    // only one SystemJavaVM for the whole browser, so it doesn't depend on env
    return (SystemJavaVM*)JVM_GetJVMMgr();
}

PR_END_EXTERN_C


/*
 * Callbacks for client-specific jsjava glue
 */
static JSJCallbacks jsj_callbacks = {
    map_jsj_thread_to_js_context_impl,
    map_js_context_to_jsj_thread_impl,
    map_java_object_to_js_object_impl,
    get_JSPrincipals_from_java_caller_impl,
    enter_js_from_java_impl,
    exit_js_impl,
    NULL,       // error_print
    get_java_wrapper_impl,
    create_java_vm_impl,
    destroy_java_vm_impl,
    attach_current_thread_impl,
    detach_current_thread_impl,
    get_java_vm_impl
};

void
jvm_InitLCGlue(void)
{
    JSJ_Init(&jsj_callbacks);
}

////////////////////////////////////////////////////////////////////////////////
