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

#include "nsJSEnvironment.h"
#include "nsIScriptObjectOwner.h"
#include "nsIScriptContextOwner.h"
#include "nsIScriptGlobalObject.h"
#include "nsIScriptGlobalObjectData.h"
#include "nsIDOMWindow.h"
#include "nsIDOMNode.h"
#include "nsIDOMElement.h"
#include "nsIDOMDocument.h"
#include "nsIDOMText.h"
#include "nsIDOMAttr.h"
#include "nsIDOMNamedNodeMap.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMHTMLImageElement.h"
#include "nsIDOMHTMLOptionElement.h"
#include "nsIScriptSecurityManager.h"
#include "nsIScriptNameSetRegistry.h"
#include "nsIScriptNameSpaceManager.h"
#include "nsDOMCID.h"
#include "nsIServiceManager.h"
#include "nsIXPConnect.h"
#include "nsIXPCSecurityManager.h"
#include "nsIJSContextStack.h"
#include "nsIJSRuntimeService.h"
#include "nsCOMPtr.h"

#if defined(OJI)
#include "nsIJVMManager.h"
#include "nsILiveConnectManager.h"
#endif

const size_t gStackSize = 8192;

static NS_DEFINE_IID(kIScriptContextIID, NS_ISCRIPTCONTEXT_IID);
static NS_DEFINE_IID(kIScriptObjectOwnerIID, NS_ISCRIPTOBJECTOWNER_IID);
static NS_DEFINE_IID(kIScriptGlobalObjectIID, NS_ISCRIPTGLOBALOBJECT_IID);
static NS_DEFINE_IID(kIScriptNameSetRegistryIID, NS_ISCRIPTNAMESETREGISTRY_IID);
static NS_DEFINE_IID(kCScriptNameSetRegistryCID, NS_SCRIPT_NAMESET_REGISTRY_CID);
static NS_DEFINE_CID(kXPConnectCID,              NS_XPCONNECT_CID);

void PR_CALLBACK
NS_ScriptErrorReporter(JSContext *cx, 
                       const char *message, 
                       JSErrorReport *report)
{
  nsIScriptContext* context = (nsIScriptContext*)JS_GetContextPrivate(cx);
  nsIScriptContextOwner* owner;

  if (context) {
    nsresult result = context->GetOwner(&owner);
    if (NS_SUCCEEDED(result)) {
      const char* error;
      if (message) {
        error = message;
      }
      else {
        error = "<unknown>";
      }
 
      if (report) {
        owner->ReportScriptError(error, 
                                 report->filename, 
                                 report->lineno,
                                 report->linebuf);
      }
      else {
        owner->ReportScriptError(error, nsnull, 0, nsnull);
      }
      NS_RELEASE(owner);
    }
  }

  JS_ClearPendingException(cx);
}

nsJSContext::nsJSContext(JSRuntime *aRuntime)
{
	mRefCnt = 0;
	mContext = JS_NewContext(aRuntime, gStackSize);
	JS_SetContextPrivate(mContext, (void *)this);
	mNameSpaceManager = nsnull;
	mIsInitialized = PR_FALSE;
	mNumEvaluations = 0;
  mSecurityManager = nsnull;
  mOwner = nsnull;
}

nsJSContext::~nsJSContext()
{
	// Tell xpconnect that we're about to destroy the JSContext so it can cleanup
	nsIXPConnect* xpc;
	nsresult res;
	res = nsServiceManager::GetService(kXPConnectCID, nsIXPConnect::GetIID(), 
		(nsISupports**) &xpc);
	//NS_ASSERTION(NS_SUCCEEDED(res), "unable to get xpconnect");
	if (NS_SUCCEEDED(res)) {
		xpc->AbandonJSContext(mContext);
		nsServiceManager::ReleaseService(kXPConnectCID, xpc);
	}
	
	NS_IF_RELEASE(mNameSpaceManager);
	JS_DestroyContext(mContext);
}

NS_IMPL_ISUPPORTS(nsJSContext, kIScriptContextIID);

NS_IMETHODIMP
nsJSContext::EvaluateString(const nsString& aScript,
                            const char *aURL,
                            PRUint32 aLineNo,
                            nsString& aRetValue,
                            PRBool* aIsUndefined)
{
  return EvaluateString(aScript, nsnull, nsnull, aURL, aLineNo, aRetValue, aIsUndefined);
}

NS_IMETHODIMP
nsJSContext::EvaluateString(const nsString& aScript,
                            void *jsObj,
                            nsIPrincipal *principal,
                            const char *aURL,
                            PRUint32 aLineNo,
                            nsString& aRetValue,
                            PRBool* aIsUndefined)
{
  nsresult rv;
  if (!jsObj)
    jsObj = JS_GetGlobalObject(mContext);
  NS_WITH_SERVICE(nsIJSContextStack, stack, "nsThreadJSContextStack", &rv);
  if (NS_FAILED(rv)) 
    return NS_ERROR_FAILURE;
  if (NS_FAILED(stack->Push(mContext))) 
    return NS_ERROR_FAILURE;
  JSPrincipals *jsprin;
  if (principal) {
    principal->GetJSPrincipals(&jsprin);
  } else {
    // norris TODO: Using GetGlobalObject to get principals is unrelible.
    nsCOMPtr<nsIScriptGlobalObjectData> globalData;
    nsCOMPtr<nsIPrincipal> principal;
    nsCOMPtr<nsIScriptGlobalObject> global = GetGlobalObject();
    if (!global || NS_FAILED(global->QueryInterface(NS_GET_IID(nsIScriptGlobalObjectData), 
                                                    (void**) getter_AddRefs(globalData)))) 
    {
      return NS_ERROR_FAILURE;
    }
    if (NS_FAILED(globalData->GetPrincipal(getter_AddRefs(principal)))) 
      return NS_ERROR_FAILURE;
    principal->GetJSPrincipals(&jsprin);
  }
  jsval val;
  NS_WITH_SERVICE(nsIScriptSecurityManager, securityManager, 
                  NS_SCRIPTSECURITYMANAGER_PROGID, &rv);
  if (NS_FAILED(rv)) 
    return NS_ERROR_FAILURE;
  PRBool canExecute;
  if (NS_FAILED(securityManager->CanExecuteScripts(principal, &canExecute)))
    return NS_ERROR_FAILURE;
  PRBool ret = PR_FALSE;
  if (canExecute) {
    ret = ::JS_EvaluateUCScriptForPrincipals(mContext, 
                                             (JSObject *)jsObj,
                                             jsprin,
                                             (jschar*)aScript.GetUnicode(), 
                                             aScript.Length(),
                                             aURL, 
                                             aLineNo,
                                             &val);
  }
  JSPRINCIPALS_DROP(mContext, jsprin);
  if (ret) {
    *aIsUndefined = JSVAL_IS_VOID(val);
    JSString* jsstring = JS_ValueToString(mContext, val);   
    aRetValue.SetString(JS_GetStringChars(jsstring));
  }
  else {
    *aIsUndefined = PR_TRUE;
    aRetValue.Truncate();
  }
	
  ScriptEvaluated();
  
  if (NS_FAILED(stack->Pop(nsnull))) 
    return NS_ERROR_FAILURE;

  return NS_OK;
}

NS_IMETHODIMP
nsJSContext::CallFunction(void *aObj, void *aFunction, PRUint32 argc, 
                          void *argv, PRBool *aBoolResult)
{
  nsresult rv;
  NS_WITH_SERVICE(nsIScriptSecurityManager, securityManager, 
                  NS_SCRIPTSECURITYMANAGER_PROGID, &rv);
  if (NS_FAILED(rv)) 
    return NS_ERROR_FAILURE;
  PRBool canExecute;
  if (NS_FAILED(securityManager->CanExecuteFunction((JSFunction *)aFunction, 
                                                    &canExecute)))
    return NS_ERROR_FAILURE;

  NS_WITH_SERVICE(nsIJSContextStack, stack, "nsThreadJSContextStack", &rv);
  if (NS_FAILED(rv)) 
    return NS_ERROR_FAILURE;
  if (NS_FAILED(stack->Push(mContext))) 
    return NS_ERROR_FAILURE;

  jsval val;
  PRBool ret = canExecute &&
               JS_CallFunction(mContext, (JSObject *)aObj, 
                               (JSFunction *)aFunction, argc, 
                               (jsval *)argv, &val);
  *aBoolResult = ret 
                 ? !JSVAL_IS_BOOLEAN(val) || JSVAL_TO_BOOLEAN(val) 
                 : JS_TRUE;
	
  ScriptEvaluated();
  
  if (NS_FAILED(stack->Pop(nsnull))) 
    return NS_ERROR_FAILURE;

  return NS_OK;
}


NS_IMETHODIMP_(nsIScriptGlobalObject*)
nsJSContext::GetGlobalObject()
{
	JSObject *global = JS_GetGlobalObject(mContext);
	nsIScriptGlobalObject *script_global = nsnull;
	
	if (nsnull != global) {
		nsISupports* sup = (nsISupports *)JS_GetPrivate(mContext, global);
		if (nsnull != sup) {
			sup->QueryInterface(kIScriptGlobalObjectIID, (void**) &script_global);
		}
		return script_global;
	}
	else {
		return nsnull;
	}
}

NS_IMETHODIMP_(void*)
nsJSContext::GetNativeContext()
{
	return (void *)mContext;
}


NS_IMETHODIMP
nsJSContext::InitContext(nsIScriptGlobalObject *aGlobalObject)
{
	nsIScriptObjectOwner *owner;
	JSObject *global;
	nsresult res = aGlobalObject->QueryInterface(kIScriptObjectOwnerIID, (void **)&owner);
	mIsInitialized = PR_FALSE;
	
	if (NS_OK == res) {
		res = owner->GetScriptObject(this, (void **)&global);
		
		// init standard classes
		if ((NS_OK == res) && ::JS_InitStandardClasses(mContext, global)) {
			JS_SetGlobalObject(mContext, global);
			res = InitClasses(); // this will complete the global object initialization
		}
		
		NS_RELEASE(owner);
		
		if (NS_OK == res) {
			::JS_SetErrorReporter(mContext, NS_ScriptErrorReporter); 
		}
	}
	
	return res;
}

nsresult
nsJSContext::InitializeExternalClasses()
{
	nsresult result = NS_OK;
	
	nsIScriptNameSetRegistry* registry;
	result = nsServiceManager::GetService(kCScriptNameSetRegistryCID,
		kIScriptNameSetRegistryIID,
		(nsISupports **)&registry);
	if (NS_OK == result) {
		result = registry->InitializeClasses(this);
		nsServiceManager::ReleaseService(kCScriptNameSetRegistryCID,
			registry);
	}
	
	return result;
}

nsresult
nsJSContext::InitializeLiveConnectClasses()
{
#if defined(OJI)
	nsresult rv = NS_OK;
	NS_WITH_SERVICE(nsIJVMManager, jvmManager, nsIJVMManager::GetCID(), &rv);
	if (rv == NS_OK && jvmManager != nsnull) {
		PRBool javaEnabled = PR_FALSE;
		if (NS_OK == jvmManager->IsJavaEnabled(&javaEnabled) && javaEnabled) {
			nsILiveConnectManager* liveConnectManager = nsnull;
			if (NS_OK == jvmManager->QueryInterface(nsILiveConnectManager::GetIID(),
				(void**)&liveConnectManager)) {
				rv = liveConnectManager->InitLiveConnectClasses(mContext, JS_GetGlobalObject(mContext));
				NS_RELEASE(liveConnectManager);
			}
		}
	}
#endif
	
	// return all is well until things are stable.
	return NS_OK;
}

NS_IMETHODIMP
nsJSContext::InitClasses()
{
	nsresult res = NS_ERROR_FAILURE;
	nsIScriptGlobalObject *global = GetGlobalObject();
	
	if (NS_OK == NS_InitWindowClass(this, global) &&
		NS_OK == NS_InitNodeClass(this, nsnull) &&
		NS_OK == NS_InitElementClass(this, nsnull) &&
		NS_OK == NS_InitDocumentClass(this, nsnull) &&
		NS_OK == NS_InitTextClass(this, nsnull) &&
		NS_OK == NS_InitAttrClass(this, nsnull) &&
		NS_OK == NS_InitNamedNodeMapClass(this, nsnull) &&
		NS_OK == NS_InitNodeListClass(this, nsnull) &&
		NS_OK == InitializeExternalClasses() && 
		NS_OK == InitializeLiveConnectClasses() &&
		// XXX Temporarily here. These shouldn't be hardcoded.
		NS_OK == NS_InitHTMLImageElementClass(this, nsnull) &&
		NS_OK == NS_InitHTMLOptionElementClass(this, nsnull)) {
		res = NS_OK;
	}
	
	// Hook up XPConnect
	{
		nsIXPConnect* xpc;
		res = nsServiceManager::GetService(kXPConnectCID, NS_GET_IID(nsIXPConnect), (nsISupports**) &xpc);
		//NS_ASSERTION(NS_SUCCEEDED(res), "unable to get xpconnect");
		if (NS_SUCCEEDED(res)) {
			res = xpc->AddNewComponentsObject(mContext, JS_GetGlobalObject(mContext));
			NS_ASSERTION(NS_SUCCEEDED(res), "unable to add Components object");
			nsServiceManager::ReleaseService(kXPConnectCID, xpc);
		}
		else {
			// silently fail for now
			res = NS_OK;
		}
	}
	mIsInitialized = PR_TRUE;
	NS_RELEASE(global);
	return res;
}

NS_IMETHODIMP     
nsJSContext::IsContextInitialized()
{
	return (mIsInitialized) ? NS_OK : NS_COMFALSE;
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
	JS_GC(mContext);
	return NS_OK;
}

NS_IMETHODIMP
nsJSContext::ScriptEvaluated(void)
{
	mNumEvaluations++;
	
	if (mNumEvaluations > 20) {
		mNumEvaluations = 0;
		GC();
	}
	
	return NS_OK;
}

NS_IMETHODIMP 
nsJSContext::GetNameSpaceManager(nsIScriptNameSpaceManager** aInstancePtr)
{
	nsresult result = NS_OK;
	
	if (nsnull == mNameSpaceManager) {
		result = NS_NewScriptNameSpaceManager(&mNameSpaceManager);
		if (NS_OK == result) {
			nsIScriptNameSetRegistry* registry;
			result = nsServiceManager::GetService(kCScriptNameSetRegistryCID,
				kIScriptNameSetRegistryIID,
				(nsISupports **)&registry);
			if (NS_OK == result) {
				result = registry->PopulateNameSpace(this);
				nsServiceManager::ReleaseService(kCScriptNameSetRegistryCID,
					registry);
			}
		}
	}
	
	*aInstancePtr = mNameSpaceManager;
	NS_ADDREF(mNameSpaceManager);
	
	return result;
}

NS_IMETHODIMP
nsJSContext::GetSecurityManager(nsIScriptSecurityManager **aInstancePtr)
{
    if (!mSecurityManager) {
        nsresult ret;
        NS_WITH_SERVICE(nsIScriptSecurityManager, securityManager, 
                        NS_SCRIPTSECURITYMANAGER_PROGID, &ret);
        if (NS_FAILED(ret)) 
            return NS_ERROR_FAILURE;
        mSecurityManager = securityManager;
        NS_ADDREF(mSecurityManager);
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
  NS_IF_ADDREF(mOwner);

  return NS_OK;
}

nsJSEnvironment *nsJSEnvironment::sTheEnvironment = nsnull;

// Class to manage destruction of the singleton
// JSEnvironment
struct JSEnvironmentInit {
	JSEnvironmentInit() {
	}
	
	~JSEnvironmentInit() {
		if (nsJSEnvironment::sTheEnvironment) {
			delete nsJSEnvironment::sTheEnvironment;
			nsJSEnvironment::sTheEnvironment = nsnull;
		}
	}
};

#ifndef XP_MAC
static JSEnvironmentInit initJSEnvironment;
#endif

nsJSEnvironment *
nsJSEnvironment::GetScriptingEnvironment()
{
	if (nsnull == sTheEnvironment) {
		sTheEnvironment = new nsJSEnvironment();
	}
	return sTheEnvironment;
}

nsJSEnvironment::nsJSEnvironment()
{
  nsresult result;

  NS_WITH_SERVICE(nsIJSRuntimeService, rtsvc, "nsJSRuntimeService", &result);
  // get the JSRuntime from the runtime svc, if possible
  if (NS_FAILED(result))
    return;                     // XXX swallow error! need Init()?
  result = rtsvc->GetRuntime(&mRuntime);
  if (NS_FAILED(result))
    return;                     // XXX swallow error! need Init()?
  	
#if defined(OJI)
	// Initialize LiveConnect.
	nsILiveConnectManager* manager = NULL;
	result = nsServiceManager::GetService(nsIJVMManager::GetCID(),
		nsILiveConnectManager::GetIID(),
		(nsISupports **)&manager);
	
	// Should the JVM manager perhaps define methods for starting up LiveConnect?
	if (result == NS_OK && manager != NULL) {
		PRBool started = PR_FALSE;
		result = manager->StartupLiveConnect(mRuntime, started);
		result = nsServiceManager::ReleaseService(nsIJVMManager::GetCID(), manager);
	}
#endif

}

nsJSEnvironment::~nsJSEnvironment()
{
}

nsIScriptContext* nsJSEnvironment::GetNewContext()
{
	nsIScriptContext *context;
	context = new nsJSContext(mRuntime);
	NS_ADDREF(context);
	return context;
}

#define XPC_HOOK_VALUE (nsIXPCSecurityManager::HOOK_CREATE_WRAPPER  |   \
                        nsIXPCSecurityManager::HOOK_CREATE_INSTANCE |   \
                        nsIXPCSecurityManager::HOOK_GET_SERVICE)

extern "C" NS_DOM nsresult NS_CreateScriptContext(nsIScriptGlobalObject *aGlobal, 
                                                  nsIScriptContext **aContext)
{
	nsresult rv = NS_OK;
	nsJSEnvironment *environment = nsJSEnvironment::GetScriptingEnvironment();
	*aContext = environment->GetNewContext();
	if (! *aContext) return NS_ERROR_OUT_OF_MEMORY; // XXX
	// Hook up XPConnect
	nsIXPConnect* xpc;
	rv = nsServiceManager::GetService(kXPConnectCID, nsIXPConnect::GetIID(), (nsISupports**) &xpc);
	//NS_ASSERTION(NS_SUCCEEDED(rv), "unable to get xpconnect");
	if (NS_SUCCEEDED(rv)) {
		nsIScriptObjectOwner* owner;
		rv = aGlobal->QueryInterface(nsIScriptObjectOwner::GetIID(), (void**) &owner);
		if (NS_SUCCEEDED(rv)) {
			JSObject* global;
			rv = owner->GetScriptObject(*aContext, (void**) &global);
			if (NS_SUCCEEDED(rv)) {
				JSContext *cx = (JSContext*) (*aContext)->GetNativeContext();
				rv = xpc->InitJSContext(cx, global, JS_FALSE);
				//NS_ASSERTION(NS_SUCCEEDED(rv), "xpconnect unable to init jscontext");
				nsIScriptSecurityManager *mgr=NULL;
				nsIXPCSecurityManager *xpcSecurityManager=NULL;
				if (NS_SUCCEEDED(rv)) 
					rv=(*aContext)->GetSecurityManager(&mgr);
				if (NS_SUCCEEDED(rv))
					rv = mgr->QueryInterface(nsIXPCSecurityManager::GetIID(), (void**)&xpcSecurityManager);
	        // Bind the script context and the global object
	        (*aContext)->InitContext(aGlobal);
	         aGlobal->SetContext(*aContext);
				if (NS_SUCCEEDED(rv))
					xpc->SetSecurityManagerForJSContext(cx, xpcSecurityManager, XPC_HOOK_VALUE);
			}
			NS_RELEASE(owner);
		}
		nsServiceManager::ReleaseService(kXPConnectCID, xpc);
	}
	else {
	    (*aContext)->InitContext(aGlobal);
	    aGlobal->SetContext(*aContext);
        rv = NS_OK;
	}
	// XXX silently fail for now
	//  return rv;
	return NS_OK;    
}

