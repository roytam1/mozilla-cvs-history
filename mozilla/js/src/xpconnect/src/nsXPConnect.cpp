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
 *   John Bandhauer <jband@netscape.com>
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

/* High level class and public functions implementation. */

#include "xpcprivate.h"

NS_IMPL_THREADSAFE_ISUPPORTS1(nsXPConnect,nsIXPConnect)

nsXPConnect* nsXPConnect::gSelf = nsnull;
JSBool nsXPConnect::gOnceAliveNowDead = JS_FALSE;

/***************************************************************************/

nsXPConnect::nsXPConnect()
    :   mRuntime(nsnull),
        mInterfaceInfoManager(nsnull),
        mContextStack(nsnull),
        mDefaultSecurityManager(nsnull),
        mDefaultSecurityManagerFlags(0),
        mShutingDown(JS_FALSE)
{
    NS_INIT_REFCNT();

    // ignore result - if the runtime service is not ready to rumble
    // then we'll set this up later as needed.
    CreateRuntime();

    mInterfaceInfoManager = XPTI_GetInterfaceInfoManager();

    nsServiceManager::GetService("@mozilla.org/js/xpc/ContextStack;1",
                                 NS_GET_IID(nsIThreadJSContextStack),
                                 (nsISupports **)&mContextStack);

#ifdef XPC_TOOLS_SUPPORT
  {
    const char* filename = PR_GetEnv("MOZILLA_JS_PROFILER_OUTPUT");
    if(filename)
    {
        
        mProfilerOutputFile = do_CreateInstance(NS_LOCAL_FILE_CONTRACTID);         
        if(mProfilerOutputFile && 
           NS_SUCCEEDED(mProfilerOutputFile->InitWithPath(filename)))
        {
            mProfiler = do_GetService(XPCTOOLS_PROFILER_CONTRACTID);
            if(mProfiler)
            {
                if(NS_SUCCEEDED(mProfiler->Start()))
                {
#ifdef DEBUG
                    printf("***** profiling JavaScript. Output to: %s\n",
                           filename);
#endif
                }
            }
        }
    }
  }
#endif

}

nsXPConnect::~nsXPConnect()
{
    mShutingDown = JS_TRUE;
    { // scoped callcontext
        XPCCallContext ccx(NATIVE_CALLER);
        if(ccx.IsValid())
            XPCWrappedNativeScope::SystemIsBeingShutDown(ccx);
    }

    NS_IF_RELEASE(mInterfaceInfoManager);
    NS_IF_RELEASE(mContextStack);
    NS_IF_RELEASE(mDefaultSecurityManager);

    // Unfortunately calling CleanupAllThreads before the stuff above
    // (esp. SystemIsBeingShutDown) causes too many bad things to happen 
    // as the Release calls propagate. See the comment in this function in 
    // revision 1.35 of this file.
    //
    // I filed a bug on xpcom regarding the bad things that happen
    // if people try to create components during shutdown. 
    // http://bugzilla.mozilla.org/show_bug.cgi?id=37058
    //
    // Also, we just plain need the context stack for at least the current 
    // thread to be in place. Unfortunately, this will leak stuff on the 
    // stacks' safeJSContexts. But, this is a shutdown leak only.

    XPCPerThreadData::CleanupAllThreads();

    // shutdown the logging system
    XPC_LOG_FINISH();

    if(mRuntime)
        delete mRuntime;
    gSelf = nsnull;
    gOnceAliveNowDead = JS_TRUE;
}

// static
nsXPConnect*
nsXPConnect::GetXPConnect()
{
    if(!gSelf)
    {
        if(gOnceAliveNowDead)
            return nsnull;
        gSelf = new nsXPConnect();
        if (!gSelf ||
            !gSelf->mInterfaceInfoManager ||
            !gSelf->mContextStack)
        {
            // ctor failed to create an acceptable instance
            if(gSelf)
            {
                delete gSelf;
                gSelf = nsnull;
            }
            return nsnull;
        }
        else
        {
            // Initial extra ref to keep the singleton alive
            // balanced by explicit call to ReleaseXPConnectSingleton()
            NS_ADDREF(gSelf);
        }
    }
    NS_ADDREF(gSelf);
    return gSelf;
}

// In order to enable this jsgc heap dumping you need to compile
// _both_ js/src/jsgc.c and this file with 'GC_MARK_DEBUG' #defined.
// Normally this is done by adding -DGC_MARK_DEBUG to the appropriate
// defines lists in the makefiles.

#ifdef GC_MARK_DEBUG                                
extern "C" JS_FRIEND_DATA(FILE *) js_DumpGCHeap;
#endif

void
nsXPConnect::ReleaseXPConnectSingleton()
{
    nsXPConnect* xpc = gSelf;
    if (xpc) {

#ifdef XPC_TOOLS_SUPPORT
        if(xpc->mProfiler)
        {
            xpc->mProfiler->Stop();
            xpc->mProfiler->WriteResults(xpc->mProfilerOutputFile);
        }
#endif

#ifdef GC_MARK_DEBUG
        // force a dump of the JavaScript gc heap if JS is still alive
        if(GetRuntime() && GetRuntime()->GetJSRuntime())
        {
            AutoPushCompatibleJSContext a(GetRuntime()->GetJSRuntime());
            if(a.GetJSContext())
            {
                FILE* oldFileHandle = js_DumpGCHeap;
                js_DumpGCHeap = stdout;
                js_ForceGC(a.GetJSContext());
                js_DumpGCHeap = oldFileHandle;
            }
        }
#endif
#ifdef XPC_DUMP_AT_SHUTDOWN
        // NOTE: to see really interesting stuff turn on the prlog stuff.
        // See the comment at the top of xpclog.h to see how to do that.
        xpc->DebugDump(7);
#endif
        nsrefcnt cnt;
        NS_RELEASE2(xpc, cnt);
#ifdef XPC_DUMP_AT_SHUTDOWN
        if (0 != cnt) {
            printf("*** dangling reference to nsXPConnect: refcnt=%d\n", cnt);
        } else {
            printf("+++ XPConnect had no dangling references.\n");
        }
#endif
    }
}

// static
nsIInterfaceInfoManager*
nsXPConnect::GetInterfaceInfoManager(nsXPConnect* xpc /*= nsnull*/)
{
    nsIInterfaceInfoManager* iim;
    nsXPConnect* xpcl = xpc;

    if(!xpcl && !(xpcl = GetXPConnect()))
        return nsnull;
    if(nsnull != (iim = xpcl->mInterfaceInfoManager))
        NS_ADDREF(iim);
    if(!xpc)
        NS_RELEASE(xpcl);
    return iim;
}

// static
nsIThreadJSContextStack*
nsXPConnect::GetContextStack(nsXPConnect* xpc /*= nsnull*/)
{
    nsIThreadJSContextStack* cs;
    nsXPConnect* xpcl = xpc;

    if(!xpcl && !(xpcl = GetXPConnect()))
        return nsnull;
    if(nsnull != (cs = xpcl->mContextStack))
        NS_ADDREF(cs);
    if(!xpc)
        NS_RELEASE(xpcl);
    return cs;
}

// static
XPCJSRuntime*
nsXPConnect::GetRuntime(nsXPConnect* xpc /*= nsnull*/)
{
    XPCJSRuntime* rt;
    nsXPConnect* xpcl = xpc;

    if(!xpcl && !(xpcl = GetXPConnect()))
        return nsnull;
    rt = xpcl->EnsureRuntime() ? xpcl->mRuntime : nsnull;
    if(!xpc)
        NS_RELEASE(xpcl);
    return rt;
}

// static
XPCContext*
nsXPConnect::GetContext(JSContext* cx, nsXPConnect* xpc /*= nsnull*/)
{
    NS_PRECONDITION(cx,"bad param");

    XPCContext* xpcc;
    nsXPConnect* xpcl = xpc;

    if(!xpcl && !(xpcl = GetXPConnect()))
        return nsnull;

    if(xpcl->EnsureRuntime() && 
       xpcl->mRuntime->GetJSRuntime() == JS_GetRuntime(cx))
        xpcc = xpcl->mRuntime->GetXPCContext(cx);
    else
        xpcc = nsnull;
    if(!xpc)
        NS_RELEASE(xpcl);
    return xpcc;
}

// static
JSBool
nsXPConnect::IsISupportsDescendant(nsIInterfaceInfo* info)
{
    if(!info)
        return JS_FALSE;

    nsCOMPtr<nsIInterfaceInfo> oldest = info;
    nsCOMPtr<nsIInterfaceInfo> parent;

    while(NS_SUCCEEDED(oldest->GetParent(getter_AddRefs(parent))) && parent)
    {
        oldest = parent;
    }

    JSBool retval = JS_FALSE;
    nsID* iid;
    if(NS_SUCCEEDED(oldest->GetIID(&iid)))
    {
        retval = iid->Equals(NS_GET_IID(nsISupports));
        nsMemory::Free(iid);
    }
    return retval;
}

JSBool 
nsXPConnect::CreateRuntime()
{
    NS_ASSERTION(!mRuntime,"CreateRuntime called but mRuntime already init'd");
    nsresult rv;
    NS_WITH_SERVICE(nsIJSRuntimeService, rtsvc, "@mozilla.org/js/xpc/RuntimeService;1", &rv);
    if(NS_SUCCEEDED(rv) && rtsvc)
    {
        mRuntime = XPCJSRuntime::newXPCJSRuntime(this, rtsvc);
    }
    return nsnull != mRuntime;
}        

/***************************************************************************/
/***************************************************************************/
// nsIXPConnect interface methods...

#if 0
// XXX hacky test code...
#include "xpctest.h"

static JSBool
AttachEcho(XPCCallContext& ccx, XPCWrappedNativeScope* aScope, JSObject* aGlobal) 
{
    nsCOMPtr<nsIEcho> echo(do_CreateInstance("@mozilla.org/js/xpc/test/Echo;1"));
    if(!echo)
        return JS_FALSE;

    XPCNativeInterface* iface = 
        XPCNativeInterface::GetNewOrUsed(ccx, &NS_GET_IID(nsIEcho));

    if(!iface)
        return JS_FALSE;

    nsCOMPtr<XPCWrappedNative> 
        wrapper(
            dont_AddRef(
                XPCWrappedNative::GetNewOrUsed(ccx, echo, aScope, iface)));
    if(!wrapper)
        return JS_FALSE;
    
    JSObject* obj = wrapper->GetFlatJSObject();
    
    return obj && JS_DefineProperty(ccx.GetJSContext(),
                                    aGlobal, "echo", OBJECT_TO_JSVAL(obj),
                                    nsnull, nsnull,
                                    JSPROP_ENUMERATE);
}        
#endif

/* void initClasses (in JSContextPtr aJSContext, in JSObjectPtr aGlobalJSObj); */
NS_IMETHODIMP
nsXPConnect::InitClasses(JSContext * aJSContext, JSObject * aGlobalJSObj)
{
    NS_ENSURE_ARG_POINTER(aJSContext);
    NS_ENSURE_ARG_POINTER(aGlobalJSObj);

    XPCCallContext ccx(NATIVE_CALLER, aJSContext);
    if(!ccx.IsValid())
        return NS_ERROR_FAILURE;

    XPCContext* xpcc = ccx.GetXPCContext();

    if(!xpc_InitWrappedNativeJSOps())
        return NS_ERROR_FAILURE;

    if(!nsXPCWrappedJSClass::InitClasses(ccx, aGlobalJSObj))
        return NS_ERROR_FAILURE;

    XPCWrappedNativeScope* scope = new XPCWrappedNativeScope(ccx, aGlobalJSObj);
    if(!scope)
        return NS_ERROR_FAILURE;

    if(!nsXPCComponents::AttachNewComponentsObject(ccx, scope, aGlobalJSObj))
        return NS_ERROR_FAILURE;

#if 0
    // XXX hacky test code...
    if(!AttachEcho(ccx, scope, aGlobalJSObj)) 
        return NS_ERROR_FAILURE;
#endif

    return NS_OK;
}        

static JSClass xpcTempGlobalClass = {
    "xpcTempGlobalClass", 0,
    JS_PropertyStub,  JS_PropertyStub,  JS_PropertyStub,  JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub,   JS_ConvertStub,   JS_FinalizeStub
};


/* nsIXPConnectJSObjectHolder initClassesWithNewWrappedGlobal (in JSContextPtr aJSContext, in nsISupports aCOMObj, in nsIIDRef aIID, in PRBool aCallJS_InitStandardClasses); */
NS_IMETHODIMP 
nsXPConnect::InitClassesWithNewWrappedGlobal(JSContext * aJSContext, nsISupports *aCOMObj, const nsIID & aIID, PRBool aCallJS_InitStandardClasses, nsIXPConnectJSObjectHolder **_retval)
{
    NS_ENSURE_ARG_POINTER(aJSContext);
    NS_ENSURE_ARG_POINTER(aCOMObj);
    NS_ENSURE_ARG_POINTER(_retval);

    // XXX This is not pretty. We make a temporary global object and 
    // init it with all the Components object junk just so we have a 
    // parent with an xpc scope to use when wrapping the object that will 
    // become the 'real' global.  

    XPCCallContext ccx(NATIVE_CALLER, aJSContext);

    JSObject* tempGlobal = JS_NewObject(aJSContext, &xpcTempGlobalClass, 
                                        nsnull, nsnull);

    if(!tempGlobal ||
       !JS_InitStandardClasses(aJSContext, tempGlobal))
        return NS_ERROR_FAILURE;

    if(NS_FAILED(InitClasses(aJSContext, tempGlobal)))
        return NS_ERROR_FAILURE;

    nsCOMPtr<nsIXPConnectJSObjectHolder> holder;
    if(NS_FAILED(WrapNative(aJSContext, tempGlobal, aCOMObj, aIID,
                            getter_AddRefs(holder))) || !holder)
        return NS_ERROR_FAILURE;

    JSObject* aGlobalJSObj;
    if(NS_FAILED(holder->GetJSObject(&aGlobalJSObj)) || !aGlobalJSObj)
        return NS_ERROR_FAILURE;

    // voodoo to fixup scoping and parenting...

    JS_SetParent(aJSContext, aGlobalJSObj, nsnull);
    JS_SetGlobalObject(aJSContext, aGlobalJSObj);

    if(aCallJS_InitStandardClasses &&
       !JS_InitStandardClasses(aJSContext, aGlobalJSObj))
        return NS_ERROR_FAILURE;

    XPCWrappedNative* wrapper = 
        NS_REINTERPRET_CAST(XPCWrappedNative*, holder.get());
    XPCWrappedNativeScope* scope = wrapper->GetScope();
    
    if(!scope)
        return NS_ERROR_FAILURE;

    scope->SetGlobal(ccx, aGlobalJSObj);
    
    XPCWrappedNativeProto* proto = wrapper->GetProto();
    if(!proto)
        return NS_ERROR_FAILURE;

    JSObject* protoJSObject = proto->GetJSProtoObject();
    if(protoJSObject)
    {
        JS_SetParent(aJSContext, protoJSObject, aGlobalJSObj);
        JS_SetPrototype(aJSContext, protoJSObject, scope->GetPrototypeJSObject());
    }

    if(NS_FAILED(InitClasses(aJSContext, aGlobalJSObj)))
        return NS_ERROR_FAILURE;
    
    NS_ADDREF(*_retval = holder);

    return NS_OK;
}        

/* nsIXPConnectJSObjectHolder wrapNative (in JSContextPtr aJSContext, in JSObjectPtr aScope, in nsISupports aCOMObj, in nsIIDRef aIID); */
NS_IMETHODIMP
nsXPConnect::WrapNative(JSContext * aJSContext, JSObject * aScope, nsISupports *aCOMObj, const nsIID & aIID, nsIXPConnectJSObjectHolder **_retval)
{
    NS_ENSURE_ARG_POINTER(aJSContext);
    NS_ENSURE_ARG_POINTER(aScope);
    NS_ENSURE_ARG_POINTER(aCOMObj);
    NS_ENSURE_ARG_POINTER(_retval);

    *_retval = nsnull;

    XPCCallContext ccx(NATIVE_CALLER, aJSContext);
    if(!ccx.IsValid())
        return NS_ERROR_FAILURE;

    nsresult rv;
    if(!XPCConvert::NativeInterface2JSObject(ccx, _retval,
                                             aCOMObj, &aIID, aScope, &rv))
        return rv;
    return NS_OK;
}

/* void wrapJS (in JSContextPtr aJSContext, in JSObjectPtr aJSObj, in nsIIDRef aIID, [iid_is (aIID), retval] out nsQIResult result); */
NS_IMETHODIMP 
nsXPConnect::WrapJS(JSContext * aJSContext, JSObject * aJSObj, const nsIID & aIID, void * *result)
{
    NS_ENSURE_ARG_POINTER(aJSContext);
    NS_ENSURE_ARG_POINTER(aJSObj);
    NS_ENSURE_ARG_POINTER(result);

    *result = nsnull;

    XPCCallContext ccx(NATIVE_CALLER, aJSContext);
    if(!ccx.IsValid())
        return NS_ERROR_FAILURE;

    nsresult rv;
    if(!XPCConvert::JSObject2NativeInterface(ccx, result, aJSObj, 
                                             &aIID, nsnull, &rv))
        return rv;
    return NS_OK;
}

/* void wrapJSAggregatedToNative (in nsISupports aOuter, in JSContextPtr aJSContext, in JSObjectPtr aJSObj, in nsIIDRef aIID, [iid_is (aIID), retval] out nsQIResult result); */
NS_IMETHODIMP 
nsXPConnect::WrapJSAggregatedToNative(nsISupports *aOuter, JSContext * aJSContext, JSObject * aJSObj, const nsIID & aIID, void * *result)
{
    NS_ENSURE_ARG_POINTER(aOuter);
    NS_ENSURE_ARG_POINTER(aJSContext);
    NS_ENSURE_ARG_POINTER(aJSObj);
    NS_ENSURE_ARG_POINTER(result);

    *result = nsnull;

    XPCCallContext ccx(NATIVE_CALLER, aJSContext);
    if(!ccx.IsValid())
        return NS_ERROR_FAILURE;

    nsresult rv;
    if(!XPCConvert::JSObject2NativeInterface(ccx, result, aJSObj, 
                                             &aIID, aOuter, &rv))
        return rv;
    return NS_OK;
}

/* nsIXPConnectWrappedNative getWrappedNativeOfJSObject (in JSContextPtr aJSContext, in JSObjectPtr aJSObj); */
NS_IMETHODIMP
nsXPConnect::GetWrappedNativeOfJSObject(JSContext * aJSContext, JSObject * aJSObj, nsIXPConnectWrappedNative **_retval)
{
    NS_ENSURE_ARG_POINTER(aJSContext);
    NS_ENSURE_ARG_POINTER(aJSObj);
    NS_ENSURE_ARG_POINTER(_retval);

    XPCCallContext ccx(NATIVE_CALLER, aJSContext);
    if(!ccx.IsValid())
        return NS_ERROR_FAILURE;

    nsIXPConnectWrappedNative* wrapper = 
        XPCWrappedNative::GetWrappedNativeOfJSObject(aJSContext, aJSObj);
    if(wrapper)
    {
        NS_ADDREF(wrapper);
        *_retval = wrapper;        
        return NS_OK;
    }
    // else...
    *_retval = nsnull;
    return NS_ERROR_FAILURE;
}        

/* nsIXPConnectWrappedNative getWrappedNativeOfNativeObject (in JSContextPtr aJSContext, in JSObjectPtr aScope, in nsISupports aCOMObj, in nsIIDRef aIID); */
NS_IMETHODIMP 
nsXPConnect::GetWrappedNativeOfNativeObject(JSContext * aJSContext, JSObject * aScope, nsISupports *aCOMObj, const nsIID & aIID, nsIXPConnectWrappedNative **_retval)
{
    NS_ENSURE_ARG_POINTER(aJSContext);
    NS_ENSURE_ARG_POINTER(aScope);
    NS_ENSURE_ARG_POINTER(aCOMObj);
    NS_ENSURE_ARG_POINTER(_retval);

    *_retval = nsnull;

    XPCCallContext ccx(NATIVE_CALLER, aJSContext);
    if(!ccx.IsValid())
        return NS_ERROR_FAILURE;

    XPCWrappedNativeScope* scope =
        XPCWrappedNativeScope::FindInJSObjectScope(ccx, aScope);
    if(!scope)
        return NS_ERROR_FAILURE;

    XPCNativeInterface* iface =
        XPCNativeInterface::GetNewOrUsed(ccx, &aIID);
    if(!iface)
        return NS_ERROR_FAILURE;

    XPCWrappedNative* wrapper = 
        XPCWrappedNative::GetUsedOnly(ccx, aCOMObj, scope, iface);
    if(!wrapper)
        return NS_ERROR_FAILURE;

    *_retval = wrapper;
    return NS_OK;
}

/* void setSecurityManagerForJSContext (in JSContextPtr aJSContext, in nsIXPCSecurityManager aManager, in PRUint16 flags); */
NS_IMETHODIMP
nsXPConnect::SetSecurityManagerForJSContext(JSContext * aJSContext, nsIXPCSecurityManager *aManager, PRUint16 flags)
{
    NS_ENSURE_ARG_POINTER(aJSContext);

    XPCCallContext ccx(NATIVE_CALLER, aJSContext);
    if(!ccx.IsValid())
        return NS_ERROR_FAILURE;

    XPCContext* xpcc = ccx.GetXPCContext();

    NS_IF_ADDREF(aManager);
    nsIXPCSecurityManager* oldManager = xpcc->GetSecurityManager();
    NS_IF_RELEASE(oldManager);

    xpcc->SetSecurityManager(aManager);
    xpcc->SetSecurityManagerFlags(flags);
    return NS_OK;
}        

/* void getSecurityManagerForJSContext (in JSContextPtr aJSContext, out nsIXPCSecurityManager aManager, out PRUint16 flags); */
NS_IMETHODIMP
nsXPConnect::GetSecurityManagerForJSContext(JSContext * aJSContext, nsIXPCSecurityManager **aManager, PRUint16 *flags)
{
    NS_ENSURE_ARG_POINTER(aJSContext);
    NS_ENSURE_ARG_POINTER(aManager);
    NS_ENSURE_ARG_POINTER(flags);

    XPCCallContext ccx(NATIVE_CALLER, aJSContext);
    if(!ccx.IsValid())
        return NS_ERROR_FAILURE;

    XPCContext* xpcc = ccx.GetXPCContext();

    nsIXPCSecurityManager* manager = xpcc->GetSecurityManager();
    NS_IF_ADDREF(manager);
    *aManager = manager;
    *flags = xpcc->GetSecurityManagerFlags();
    return NS_OK;
}        

/* void setDefaultSecurityManager (in nsIXPCSecurityManager aManager, in PRUint16 flags); */
NS_IMETHODIMP
nsXPConnect::SetDefaultSecurityManager(nsIXPCSecurityManager *aManager, PRUint16 flags)
{
    NS_IF_ADDREF(aManager);
    NS_IF_RELEASE(mDefaultSecurityManager);
    mDefaultSecurityManager = aManager;
    mDefaultSecurityManagerFlags = flags;
    return NS_OK;
}        

/* void getDefaultSecurityManager (out nsIXPCSecurityManager aManager, out PRUint16 flags); */
NS_IMETHODIMP
nsXPConnect::GetDefaultSecurityManager(nsIXPCSecurityManager **aManager, PRUint16 *flags)
{
    NS_ENSURE_ARG_POINTER(aManager);
    NS_ENSURE_ARG_POINTER(flags);

    NS_IF_ADDREF(mDefaultSecurityManager);
    *aManager = mDefaultSecurityManager;
    *flags = mDefaultSecurityManagerFlags;
    return NS_OK;
}        

/* nsIJSStackFrameLocation createStackFrameLocation (in PRBool isJSFrame, in string aFilename, in string aFunctionName, in PRInt32 aLineNumber, in nsIJSStackFrameLocation aCaller); */
NS_IMETHODIMP
nsXPConnect::CreateStackFrameLocation(PRBool isJSFrame, const char *aFilename, const char *aFunctionName, PRInt32 aLineNumber, nsIJSStackFrameLocation *aCaller, nsIJSStackFrameLocation **_retval)
{
    NS_ENSURE_ARG_POINTER(_retval);

    nsIJSStackFrameLocation* location = 
            XPCJSStack::CreateStackFrameLocation(isJSFrame,
                                                 aFilename,
                                                 aFunctionName,
                                                 aLineNumber,
                                                 aCaller);
    *_retval = location;    
    return location ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}        

/* readonly attribute nsIJSStackFrameLocation CurrentJSStack; */
NS_IMETHODIMP
nsXPConnect::GetCurrentJSStack(nsIJSStackFrameLocation * *aCurrentJSStack)
{
    NS_ENSURE_ARG_POINTER(aCurrentJSStack);    

    JSContext* cx;
    // is there a current context available?
    if(mContextStack && NS_SUCCEEDED(mContextStack->Peek(&cx)) && cx)
        *aCurrentJSStack = XPCJSStack::CreateStack(cx);
    else
        *aCurrentJSStack = nsnull;
    return NS_OK;
}        

/* readonly attribute nsIXPCNativeCallContext CurrentNativeCallContext; */
NS_IMETHODIMP
nsXPConnect::GetCurrentNativeCallContext(nsIXPCNativeCallContext * *aCurrentNativeCallContext)
{
    NS_ENSURE_ARG_POINTER(aCurrentNativeCallContext);    

    XPCPerThreadData* data = XPCPerThreadData::GetData();
    if(data)
    {
        nsIXPCNativeCallContext* temp = data->GetCallContext();
        NS_IF_ADDREF(temp);
        *aCurrentNativeCallContext = temp;
        return NS_OK;
    }
    //else...
    *aCurrentNativeCallContext = nsnull;
    return NS_ERROR_FAILURE;
}        

/* attribute nsIXPCException PendingException; */
NS_IMETHODIMP
nsXPConnect::GetPendingException(nsIXPCException * *aPendingException)
{
    NS_ENSURE_ARG_POINTER(aPendingException);    

    XPCPerThreadData* data = XPCPerThreadData::GetData();
    if(!data)
    {
        *aPendingException = nsnull;
        return NS_ERROR_FAILURE;
    }

    *aPendingException = data->GetException();
    return NS_OK;
}        

NS_IMETHODIMP
nsXPConnect::SetPendingException(nsIXPCException * aPendingException)
{
    XPCPerThreadData* data = XPCPerThreadData::GetData();
    if(!data)
        return NS_ERROR_FAILURE;

    data->SetException(aPendingException);
    return NS_OK;
}        

/* void syncJSContexts (); */
NS_IMETHODIMP
nsXPConnect::SyncJSContexts(void)
{
    if(mRuntime)
        mRuntime->SyncXPCContextList();
    return NS_OK;
}        

/* void debugDump (in short depth); */
NS_IMETHODIMP
nsXPConnect::DebugDump(PRInt16 depth)
{
#ifdef DEBUG
    depth-- ;
    XPC_LOG_ALWAYS(("nsXPConnect @ %x with mRefCnt = %d", this, mRefCnt));
    XPC_LOG_INDENT();
        XPC_LOG_ALWAYS(("gSelf @ %x", gSelf));
        XPC_LOG_ALWAYS(("gOnceAliveNowDead is %d", (int)gOnceAliveNowDead));
        XPC_LOG_ALWAYS(("mDefaultSecurityManager @ %x", mDefaultSecurityManager));
        XPC_LOG_ALWAYS(("mDefaultSecurityManagerFlags of %x", mDefaultSecurityManagerFlags));
        XPC_LOG_ALWAYS(("mInterfaceInfoManager @ %x", mInterfaceInfoManager));
        XPC_LOG_ALWAYS(("mContextStack @ %x", mContextStack));
        if(mRuntime)
        {
            if(depth)
                mRuntime->DebugDump(depth);
            else
                XPC_LOG_ALWAYS(("XPCJSRuntime @ %x", mRuntime));
        }
        else
            XPC_LOG_ALWAYS(("mRuntime is null"));
        XPCWrappedNativeScope::DebugDumpAllScopes(depth);
    XPC_LOG_OUTDENT();
#endif
    return NS_OK;
}        

/* void debugDumpObject (in nsISupports aCOMObj, in short depth); */
NS_IMETHODIMP
nsXPConnect::DebugDumpObject(nsISupports *p, PRInt16 depth)
{
#ifdef DEBUG
    if(!depth)
        return NS_OK;
    if(!p)
    {
        XPC_LOG_ALWAYS(("*** Cound not dump object with NULL address"));
        return NS_OK;
    }

    nsIXPConnect* xpc;
  //  nsIXPCWrappedNativeClass* wnc;
    nsIXPCWrappedJSClass* wjsc;
//    nsIXPConnectWrappedNative* wn;
    nsIXPConnectWrappedJS* wjs;

    if(NS_SUCCEEDED(p->QueryInterface(NS_GET_IID(nsIXPConnect),
                        (void**)&xpc)))
    {
        XPC_LOG_ALWAYS(("Dumping a nsIXPConnect..."));
        xpc->DebugDump(depth);
        NS_RELEASE(xpc);
    }
/*
    else if(NS_SUCCEEDED(p->QueryInterface(NS_GET_IID(nsIXPCWrappedNativeClass),
                        (void**)&wnc)))
    {
        XPC_LOG_ALWAYS(("Dumping a nsIXPCWrappedNativeClass..."));
        wnc->DebugDump(depth);
        NS_RELEASE(wnc);
    }
*/
    else if(NS_SUCCEEDED(p->QueryInterface(NS_GET_IID(nsIXPCWrappedJSClass),
                        (void**)&wjsc)))
    {
        XPC_LOG_ALWAYS(("Dumping a nsIXPCWrappedJSClass..."));
        wjsc->DebugDump(depth);
        NS_RELEASE(wjsc);
    }
/*
    else if(NS_SUCCEEDED(p->QueryInterface(NS_GET_IID(nsIXPConnectWrappedNative),
                        (void**)&wn)))
    {
        XPC_LOG_ALWAYS(("Dumping a nsIXPConnectWrappedNative..."));
        wn->DebugDump(depth);
        NS_RELEASE(wn);
    }
*/
    else if(NS_SUCCEEDED(p->QueryInterface(NS_GET_IID(nsIXPConnectWrappedJS),
                        (void**)&wjs)))
    {
        XPC_LOG_ALWAYS(("Dumping a nsIXPConnectWrappedJS..."));
        wjs->DebugDump(depth);
        NS_RELEASE(wjs);
    }
    else
        XPC_LOG_ALWAYS(("*** Could not dump the nsISupports @ %x", p));
#endif
    return NS_OK;
}        

/* void debugDumpJSStack (in PRBool showArgs, in PRBool showLocals, in PRBool showThisProps); */
NS_IMETHODIMP 
nsXPConnect::DebugDumpJSStack(PRBool showArgs, PRBool showLocals, PRBool showThisProps)
{
#ifdef DEBUG
    JSContext* cx;
    nsresult rv;
    NS_WITH_SERVICE(nsIThreadJSContextStack, stack, "@mozilla.org/js/xpc/ContextStack;1", &rv);
    if(NS_FAILED(rv) || !stack)
        printf("failed to get nsIThreadJSContextStack service!\n");
    else if(NS_FAILED(stack->Peek(&cx)))
        printf("failed to peek into nsIThreadJSContextStack service!\n");
    else if(!cx)
        printf("there is no JSContext on the nsIThreadJSContextStack!\n");
    else
        xpc_DumpJSStack(cx, showArgs, showLocals, showThisProps);
#endif
    return NS_OK;
}

/* void debugDumpEvalInJSStackFrame (in PRUint32 aFrameNumber, in string aSourceText); */
NS_IMETHODIMP
nsXPConnect::DebugDumpEvalInJSStackFrame(PRUint32 aFrameNumber, const char *aSourceText)
{
#ifdef DEBUG
    JSContext* cx;
    nsresult rv;
    NS_WITH_SERVICE(nsIThreadJSContextStack, stack, "@mozilla.org/js/xpc/ContextStack;1", &rv);
    if(NS_FAILED(rv) || !stack)
        printf("failed to get nsIThreadJSContextStack service!\n");
    else if(NS_FAILED(stack->Peek(&cx)))
        printf("failed to peek into nsIThreadJSContextStack service!\n");
    else if(!cx)
        printf("there is no JSContext on the nsIThreadJSContextStack!\n");
    else
        xpc_DumpEvalInJSStackFrame(cx, aFrameNumber, aSourceText);
#endif
    return NS_OK;
}        

#ifdef DEBUG
/* These are here to be callable from a debugger */
JS_BEGIN_EXTERN_C
void DumpJSStack()
{
    nsresult rv;
    NS_WITH_SERVICE(nsIXPConnect, xpc, nsIXPConnect::GetCID(), &rv);
    if(NS_SUCCEEDED(rv) && xpc)
        xpc->DebugDumpJSStack(PR_TRUE, PR_TRUE, PR_FALSE);
    else    
        printf("failed to get XPConnect service!\n");
}

void DumpJSEval(PRUint32 frameno, const char* text)
{
    nsresult rv;
    NS_WITH_SERVICE(nsIXPConnect, xpc, nsIXPConnect::GetCID(), &rv);
    if(NS_SUCCEEDED(rv) && xpc)
        xpc->DebugDumpEvalInJSStackFrame(frameno, text);
    else    
        printf("failed to get XPConnect service!\n");
}

void DumpJSObject(JSObject* obj)
{
    xpc_DumpJSObject(obj);
}        
JS_END_EXTERN_C
#endif
