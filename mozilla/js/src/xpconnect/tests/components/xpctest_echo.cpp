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
 *   John Bandhauer <jband@netscape.com>
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

/* implement nsIEcho for testing. */

#include "xpctest_private.h"

class xpctestEcho : public nsIEcho, public nsITimerCallback
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIECHO

    // not very xpcom compilant method from nsITimerCallback
    NS_IMETHOD_(void) Notify(nsITimer *timer);

    xpctestEcho();
    virtual ~xpctestEcho();
private:
    nsIEcho* mReceiver;
};

/***************************************************************************/

NS_IMPL_ISUPPORTS2(xpctestEcho, nsIEcho, nsITimerCallback);

xpctestEcho::xpctestEcho()
    : mReceiver(NULL)
{
    NS_INIT_REFCNT();
    NS_ADDREF_THIS();
}

xpctestEcho::~xpctestEcho()
{
    NS_IF_RELEASE(mReceiver);
}

NS_IMETHODIMP xpctestEcho::SetReceiver(nsIEcho* aReceiver)
{
    NS_IF_ADDREF(aReceiver);
    NS_IF_RELEASE(mReceiver);
    mReceiver = aReceiver;
    return NS_OK;
}

NS_IMETHODIMP xpctestEcho::SendOneString(const char* str)
{
    if(mReceiver)
        return mReceiver->SendOneString(str);
    return NS_OK;
}

NS_IMETHODIMP xpctestEcho::In2OutOneInt(int input, int* output)
{
    *output = input;
    return NS_OK;
}

NS_IMETHODIMP xpctestEcho::In2OutAddTwoInts(int input1,
                                       int input2,
                                       int* output1,
                                       int* output2,
                                       int* result)
{
    *output1 = input1;
    *output2 = input2;
    *result = input1+input2;
    return NS_OK;
}

NS_IMETHODIMP xpctestEcho::In2OutOneString(const char* input, char** output)
{
    char* p;
    int len;
    if(input && output &&
       (NULL != (p = (char*)nsAllocator::Alloc(len=strlen(input)+1))))
    {
        memcpy(p, input, len);
        *output = p;
        return NS_OK;
    }
    if(output)
        *output = NULL;
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP xpctestEcho::SimpleCallNoEcho()
{
    return NS_OK;
}

NS_IMETHODIMP
xpctestEcho::SendManyTypes(PRUint8              p1,
                      PRInt16             p2,
                      PRInt32             p3,
                      PRInt64             p4,
                      PRUint8              p5,
                      PRUint16            p6,
                      PRUint32            p7,
                      PRUint64            p8,
                      float             p9,
                      double            p10,
                      PRBool            p11,
                      char              p12,
                      PRUnichar            p13,
                      const nsID*       p14,
                      const char*       p15,
                      const PRUnichar*  p16)
{
    if(mReceiver)
        return mReceiver->SendManyTypes(p1, p2, p3, p4, p5, p6, p7, p8, p9,
                                        p10, p11, p12, p13, p14, p15, p16);
    return NS_OK;
}

NS_IMETHODIMP
xpctestEcho::SendInOutManyTypes(PRUint8*    p1,
                           PRInt16*   p2,
                           PRInt32*   p3,
                           PRInt64*   p4,
                           PRUint8*    p5,
                           PRUint16*  p6,
                           PRUint32*  p7,
                           PRUint64*  p8,
                           float*   p9,
                           double*  p10,
                           PRBool*  p11,
                           char*    p12,
                           PRUnichar*  p13,
                           nsID**   p14,
                           char**   p15,
                           PRUnichar** p16)
{
    if(mReceiver)
        return mReceiver->SendInOutManyTypes(p1, p2, p3, p4, p5, p6, p7, p8, p9,
                                             p10, p11, p12, p13, p14, p15, p16);
    return NS_OK;
}

NS_IMETHODIMP
xpctestEcho::MethodWithNative(int p1, void* p2)
{
    return NS_OK;
}

NS_IMETHODIMP
xpctestEcho::ReturnCode(int code)
{
    return (nsresult) code;
}

NS_IMETHODIMP
xpctestEcho::FailInJSTest(int fail)
{
    if(mReceiver)
        return mReceiver->FailInJSTest(fail);
    return NS_OK;
}

NS_IMETHODIMP
xpctestEcho::SharedString(const char **str)
{
    *str = "a static string";
/*
    // to do non-shared we clone the string:
    char buf[] = "a static string";
    int len;
    *str = (char*)nsAllocator::Alloc(len=strlen(buf)+1);
    memcpy(*str, buf, len);
*/
    return NS_OK;
}

NS_IMETHODIMP
xpctestEcho::ReturnCode_NS_OK()
{return NS_OK;}

NS_IMETHODIMP
xpctestEcho::ReturnCode_NS_ERROR_NULL_POINTER()
{return NS_ERROR_NULL_POINTER;}

NS_IMETHODIMP
xpctestEcho::ReturnCode_NS_ERROR_UNEXPECTED()
{return NS_ERROR_UNEXPECTED;}

NS_IMETHODIMP
xpctestEcho::ReturnCode_NS_ERROR_OUT_OF_MEMORY()
{return NS_ERROR_OUT_OF_MEMORY;}

NS_IMETHODIMP
xpctestEcho::ReturnInterface(nsISupports *obj, nsISupports **_retval)
{
    if(!_retval)
        return NS_ERROR_NULL_POINTER;
    if(obj)
        NS_ADDREF(obj);
    *_retval = obj;
    return NS_OK;
}

/* nsIJSStackFrameLocation GetStack (); */
NS_IMETHODIMP
xpctestEcho::GetStack(nsIJSStackFrameLocation **_retval)
{
    nsIJSStackFrameLocation* stack = nsnull;
    if(!_retval)
        return NS_ERROR_NULL_POINTER;

    nsresult rv;
    NS_WITH_SERVICE(nsIXPConnect, xpc, nsIXPConnect::GetCID(), &rv);
    if(NS_SUCCEEDED(rv))
    {
        nsIJSStackFrameLocation* jsstack;
        if(NS_SUCCEEDED(xpc->GetCurrentJSStack(&jsstack)) && jsstack)
        {
            xpc->CreateStackFrameLocation(JS_FALSE,
                                          __FILE__,
                                          "xpctestEcho::GetStack",
                                          __LINE__,
                                          jsstack,
                                          &stack);
            NS_RELEASE(jsstack);
        }
    }

    if(stack)
    {
        *_retval = stack;
        return NS_OK;
    }
    return NS_ERROR_FAILURE;
}

/* void SetReceiverReturnOldReceiver (inout nsIEcho aReceiver); */
NS_IMETHODIMP
xpctestEcho::SetReceiverReturnOldReceiver(nsIEcho **aReceiver)
{
    if(!aReceiver)
        return NS_ERROR_NULL_POINTER;

    nsIEcho* oldReceiver = mReceiver;
    mReceiver = *aReceiver;
    if(mReceiver)
        NS_ADDREF(mReceiver);

    /* don't release the reference, that is the caller's problem */
    *aReceiver = oldReceiver;
    return NS_OK;
}

/* void MethodWithForwardDeclaredParam (in nsITestXPCSomeUselessThing sut); */
NS_IMETHODIMP
xpctestEcho::MethodWithForwardDeclaredParam(nsITestXPCSomeUselessThing *sut)
{
    return NS_OK;
}

/* void PseudoQueryInterface (in nsIIDRef uuid, [iid_is (uuid), retval] out nsQIResult result); */
NS_IMETHODIMP
xpctestEcho::PseudoQueryInterface(const nsIID & uuid, void * *result)
{
    if(!result)
        return NS_ERROR_NULL_POINTER;
    if(mReceiver)
        return mReceiver->PseudoQueryInterface(uuid, result);
    return NS_OK;
}        

/* void DebugDumpJSStack (); */
NS_IMETHODIMP
xpctestEcho::DebugDumpJSStack()
{
    nsresult rv;
    NS_WITH_SERVICE(nsIXPConnect, xpc, nsIXPConnect::GetCID(), &rv);
    if(NS_SUCCEEDED(rv))
    {
        rv = xpc->DebugDumpJSStack(JS_TRUE, JS_TRUE, JS_TRUE);
    }
    return rv;
}        

/***************************************************/

// some tests of nsIXPCNativeCallContext

#define GET_CALL_CONTEXT \
  nsresult rv; \
  nsCOMPtr<nsIXPCNativeCallContext> cc; \
  NS_WITH_SERVICE(nsIXPConnect, xpc, nsIXPConnect::GetCID(), &rv); \
  if(NS_SUCCEEDED(rv)) \
    rv = xpc->GetCurrentNativeCallContext(getter_AddRefs(cc)) /* no ';' */        

/* void printArgTypes (); */
NS_IMETHODIMP
xpctestEcho::PrintArgTypes(void)
{
    GET_CALL_CONTEXT;
    if(NS_FAILED(rv) || !cc)
        return NS_ERROR_FAILURE;

    nsCOMPtr<nsISupports> callee;
    if(NS_FAILED(cc->GetCallee(getter_AddRefs(callee))) || callee != this)
        return NS_ERROR_FAILURE;

    PRUint32 argc;
    if(NS_SUCCEEDED(cc->GetArgc(&argc)))
        printf("argc = %d  ", (int)argc);
    else
        return NS_ERROR_FAILURE;

    jsval* argv;
    if(NS_FAILED(cc->GetArgvPtr(&argv)))
        return NS_ERROR_FAILURE;

    printf("argv types = [");

    for(PRUint32 i = 0; i < argc; i++)
    {
        const char* type = "<unknown>";
        if(JSVAL_IS_OBJECT(argv[i]))
        {
            if(JSVAL_IS_NULL(argv[i]))
                type = "null";
            else
                type = "object";
        }
        else if (JSVAL_IS_BOOLEAN(argv[i]))
            type = "boolean";
        else if (JSVAL_IS_STRING(argv[i]))
            type = "string";
        else if (JSVAL_IS_DOUBLE(argv[i]))
            type = "double";
        else if (JSVAL_IS_INT(argv[i]))
            type = "int";
        else if (JSVAL_IS_VOID(argv[i]))
            type = "void";

        printf(type);

        if(i < argc-1)
            printf(", ");
    }
    printf("]\n");
    
    return NS_OK;
}

/* void throwArg (); */
NS_IMETHODIMP
xpctestEcho::ThrowArg(void)
{
    GET_CALL_CONTEXT;
    if(NS_FAILED(rv) || !cc)
        return NS_ERROR_FAILURE;

    nsCOMPtr<nsISupports> callee;
    if(NS_FAILED(cc->GetCallee(getter_AddRefs(callee))) || callee != this)
        return NS_ERROR_FAILURE;

    PRUint32 argc;
    if(NS_FAILED(cc->GetArgc(&argc)) || !argc)
        return NS_OK;

    jsval* argv;
    JSContext* cx;
    if(NS_FAILED(cc->GetArgvPtr(&argv)) ||
       NS_FAILED(cc->GetJSContext(&cx)))
        return NS_ERROR_FAILURE;

    JS_SetPendingException(cx, argv[0]);
    cc->SetExceptionWasThrown(JS_TRUE);
        
    return NS_OK;
}

/* void callReceiverSometimeLater (); */
NS_IMETHODIMP
xpctestEcho::CallReceiverSometimeLater(void)
{
    // Mac does not even compile this code and Unix build systems
    // have build order problems with linking to the static timer lib
    // as it is built today. This is only test code and we can stand to 
    // have it only work on Win32 for now.
#ifdef WIN32
    nsITimer *timer;
    if(NS_FAILED(NS_NewTimer(&timer)))
        return NS_ERROR_FAILURE;
    timer->Init(NS_STATIC_CAST(nsITimerCallback*,this), 2000);
    return NS_OK;
#else
    return NS_ERROR_NOT_IMPLEMENTED;
#endif
}

NS_IMETHODIMP_(void)
xpctestEcho::Notify(nsITimer *timer)
{
    if(mReceiver)
        mReceiver->CallReceiverSometimeLater();
    NS_RELEASE(timer);
}        

/* readonly attribute short throwInGetter; */
NS_IMETHODIMP
xpctestEcho::GetThrowInGetter(PRInt16 *aThrowInGetter)
{
    return NS_ERROR_FAILURE;
}        

/***************************************************/

/***************************************************************************/

// static
NS_IMETHODIMP
xpctest::ConstructEcho(nsISupports *aOuter, REFNSIID aIID, void **aResult)
{
    nsresult rv;
    NS_ASSERTION(aOuter == nsnull, "no aggregation");
    xpctestEcho* obj = new xpctestEcho();

    if(obj)
    {
        rv = obj->QueryInterface(aIID, aResult);
        NS_ASSERTION(NS_SUCCEEDED(rv), "unable to find correct interface");
        NS_RELEASE(obj);
    }
    else
    {
        *aResult = nsnull;
        rv = NS_ERROR_OUT_OF_MEMORY;
    }

    return rv;
}

