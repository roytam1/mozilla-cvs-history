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

/* An xpcom implementation of the JavaScript nsIID and nsCID objects. */

#include "xpcprivate.h"

/***************************************************************************/
// stuff used by all

static void ThrowException(uintN errNum, JSContext* cx)
{
    XPCThrower::Throw(errNum, cx);
}

static nsresult ThrowAndFail(nsresult errNum, JSContext* cx, JSBool* retval)
{
    XPCThrower::Throw(errNum, cx);
    *retval = JS_FALSE;
    return NS_OK;
}

static nsresult ThrowBadResultAndFail(nsresult errNum, nsresult result,
                                      XPCCallContext& ccx, JSBool* retval)
{
    XPCThrower::ThrowBadResult(errNum, result, ccx);
    *retval = JS_FALSE;
    return NS_OK;
}

/***************************************************************************/
// nsJSID

NS_IMPL_THREADSAFE_ISUPPORTS1(nsJSID, nsIJSID)

char nsJSID::gNoString[] = "";

nsJSID::nsJSID()
    : mID(GetInvalidIID()), mNumber(gNoString), mName(gNoString)
{
    NS_INIT_ISUPPORTS();
};

nsJSID::~nsJSID()
{
    if(mNumber && mNumber != gNoString)
        PR_Free(mNumber);
    if(mName && mName != gNoString)
        PR_Free(mName);
}

void nsJSID::Reset()
{
    mID = GetInvalidIID();

    if(mNumber && mNumber != gNoString)
        PR_Free(mNumber);
    if(mName && mName != gNoString)
        PR_Free(mName);

    mNumber = mName = nsnull;
}

PRBool
nsJSID::SetName(const char* name)
{
    NS_ASSERTION(!mName || mName == gNoString ,"name already set");
    NS_ASSERTION(name,"null name");
    int len = strlen(name)+1;
    mName = (char*)PR_Malloc(len);
    if(!mName)
        return PR_FALSE;
    memcpy(mName, name, len);
    return PR_TRUE;
}

NS_IMETHODIMP
nsJSID::GetName(char * *aName)
{
    if(!aName)
        return NS_ERROR_NULL_POINTER;

    if(!NameIsSet())
        SetNameToNoString();
    NS_ASSERTION(mName, "name not set");
    *aName = (char*) nsMemory::Clone(mName, strlen(mName)+1);
    return *aName ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

NS_IMETHODIMP
nsJSID::GetNumber(char * *aNumber)
{
    if(!aNumber)
        return NS_ERROR_NULL_POINTER;

    if(!mNumber)
    {
        if(!(mNumber = mID.ToString()))
            mNumber = gNoString;
    }

    *aNumber = (char*) nsMemory::Clone(mNumber, strlen(mNumber)+1);
    return *aNumber ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

NS_IMETHODIMP
nsJSID::GetId(nsID* *aId)
{
    if(!aId)
        return NS_ERROR_NULL_POINTER;

    *aId = (nsID*) nsMemory::Clone(&mID, sizeof(nsID));
    return *aId ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

NS_IMETHODIMP
nsJSID::GetValid(PRBool *aValid)
{
    if(!aValid)
        return NS_ERROR_NULL_POINTER;

    *aValid = IsValid();
    return NS_OK;
}

NS_IMETHODIMP
nsJSID::Equals(nsIJSID *other, PRBool *_retval)
{
    if(!_retval)
        return NS_ERROR_NULL_POINTER;

    *_retval = PR_FALSE;

    if(!other || mID.Equals(GetInvalidIID()))
        return NS_OK;

    nsID* otherID;
    if(NS_SUCCEEDED(other->GetId(&otherID)))
    {
        *_retval = mID.Equals(*otherID);
        nsMemory::Free(otherID);
    }
    return NS_OK;
}

NS_IMETHODIMP
nsJSID::Initialize(const char *idString)
{
    if(!idString)
        return NS_ERROR_NULL_POINTER;

    PRBool success = PR_FALSE;

    if(strlen(idString) && mID.Equals(GetInvalidIID()))
    {
        Reset();

        if(idString[0] == '{')
        {
            nsID id;
            if(id.Parse((char*)idString))
            {
                mID = id;
                success = PR_TRUE;
            }
        }
    }
    return success ? NS_OK : NS_ERROR_FAILURE;
}

PRBool
nsJSID::InitWithName(const nsID& id, const char *nameString)
{
    NS_ASSERTION(nameString, "no name");
    Reset();
    mID = id;
    return SetName(nameString);
}

// try to use the name, if no name, then use the number
NS_IMETHODIMP
nsJSID::ToString(char **_retval)
{
    if(mName != gNoString)
    {
        char* str;
        if(NS_SUCCEEDED(GetName(&str)))
        {
            if(mName != gNoString)
            {
                *_retval = str;
                return NS_OK;
            }
            else
                nsMemory::Free(str);
        }
    }
    return GetNumber(_retval);
}

const nsID&
nsJSID::GetInvalidIID() const
{
    // {BB1F47B0-D137-11d2-9841-006008962422}
    static nsID invalid = {0xbb1f47b0, 0xd137, 0x11d2,
                            {0x98, 0x41, 0x0, 0x60, 0x8, 0x96, 0x24, 0x22}};
    return invalid;
}

//static
nsJSID*
nsJSID::NewID(const char* str)
{
    if(!str)
    {
        NS_ASSERTION(0,"no string");
        return nsnull;
    }

    nsJSID* idObj = new nsJSID();
    if(idObj)
    {
        NS_ADDREF(idObj);
        if(NS_FAILED(idObj->Initialize(str)))
            NS_RELEASE(idObj);
    }
    return idObj;
}

/***************************************************************************/
/***************************************************************************/

NS_IMPL_THREADSAFE_ISUPPORTS3(nsJSIID, nsIJSID, nsIJSIID, nsIXPCScriptable)

// The nsIXPCScriptable map declaration that will generate stubs for us...
#define XPC_MAP_CLASSNAME           nsJSIID
#define XPC_MAP_QUOTED_CLASSNAME   "nsJSIID"
#define                             XPC_MAP_WANT_NEWRESOLVE
#define                             XPC_MAP_WANT_ENUMERATE
#define                             XPC_MAP_WANT_HASINSTANCE
#define XPC_MAP_FLAGS               nsIXPCScriptable::DONT_ENUM_STATIC_PROPS |\
                                    nsIXPCScriptable::ALLOW_PROP_MODS_DURING_RESOLVE
#include "xpc_map_end.h" /* This will #undef the above */

nsJSIID::nsJSIID()
{
    NS_INIT_ISUPPORTS();
}

nsJSIID::~nsJSIID() {}

NS_IMETHODIMP nsJSIID::GetName(char * *aName)
    {ResolveName(); return mDetails.GetName(aName);}

NS_IMETHODIMP nsJSIID::GetNumber(char * *aNumber)
    {return mDetails.GetNumber(aNumber);}

NS_IMETHODIMP nsJSIID::GetId(nsID* *aId)
    {return mDetails.GetId(aId);}

NS_IMETHODIMP nsJSIID::GetValid(PRBool *aValid)
    {return mDetails.GetValid(aValid);}

NS_IMETHODIMP nsJSIID::Equals(nsIJSID *other, PRBool *_retval)
    {return mDetails.Equals(other, _retval);}

NS_IMETHODIMP nsJSIID::Initialize(const char *idString)
    {return mDetails.Initialize(idString);}

NS_IMETHODIMP nsJSIID::ToString(char **_retval)
    {ResolveName(); return mDetails.ToString(_retval);}

void
nsJSIID::ResolveName()
{
    if(!mDetails.NameIsSet())
    {
        nsIInterfaceInfoManager* iim;
        if(nsnull != (iim = nsXPConnect::GetInterfaceInfoManager()))
        {
            char* name;
            if(NS_SUCCEEDED(iim->GetNameForIID(mDetails.GetID(), &name)) && name)
            {
                mDetails.SetName(name);
                nsMemory::Free(name);
            }
            NS_RELEASE(iim);
        }
        if(!mDetails.NameIsSet())
            mDetails.SetNameToNoString();
    }
}

//static
nsJSIID*
nsJSIID::NewID(const char* str)
{
    if(!str)
    {
        NS_ASSERTION(0,"no string");
        return nsnull;
    }

    nsJSIID* idObj = new nsJSIID();
    if(idObj)
    {
        PRBool success = PR_FALSE;
        NS_ADDREF(idObj);

        if(str[0] == '{')
        {
            if(NS_SUCCEEDED(idObj->Initialize(str)))
                success = PR_TRUE;
        }
        else
        {
            nsIInterfaceInfoManager* iim;
            if(nsnull != (iim = nsXPConnect::GetInterfaceInfoManager()))
            {
                nsCOMPtr<nsIInterfaceInfo> iinfo;
                PRBool canScript;
                nsID* pid;
                if(NS_SUCCEEDED(iim->GetInfoForName(str,
                                                    getter_AddRefs(iinfo))) &&
                   NS_SUCCEEDED(iinfo->IsScriptable(&canScript)) && canScript &&
                   NS_SUCCEEDED(iinfo->GetIID(&pid)) && pid)
                {
                    success = idObj->mDetails.InitWithName(*pid, str);
                    nsMemory::Free(pid);
                }
                NS_RELEASE(iim);
            }
        }
        if(!success)
            NS_RELEASE(idObj);
    }
    return idObj;
}


/* PRBool resolve (in nsIXPConnectWrappedNative wrapper, in JSContextPtr cx, in JSObjectPtr obj, in JSVal id); */
NS_IMETHODIMP
nsJSIID::NewResolve(nsIXPConnectWrappedNative *wrapper,
                    JSContext * cx, JSObject * obj,
                    jsval id, PRUint32 flags, 
                    JSObject * *objp, PRBool *_retval)
{
    XPCCallContext ccx(JS_CALLER, cx);

    XPCNativeInterface* iface =
        XPCNativeInterface::GetNewOrUsed(ccx, mDetails.GetID());

    if(!iface)
        return NS_OK;

    XPCNativeMember* member = iface->FindMember(id);
    if(member && member->IsConstant())
    {
        jsval val;
        if(!member->GetValue(ccx, iface, &val))
            return NS_ERROR_OUT_OF_MEMORY;

        jsid idid;
        if(!JS_ValueToId(cx, id, &idid))
            return NS_ERROR_OUT_OF_MEMORY;

        *objp = obj;
        *_retval = OBJ_DEFINE_PROPERTY(cx, obj, idid, val,
                                       nsnull, nsnull,
                                       JSPROP_ENUMERATE |
                                       JSPROP_READONLY |
                                       JSPROP_PERMANENT,
                                       nsnull);
    }

    return NS_OK;
}

/* PRBool enumerate (in nsIXPConnectWrappedNative wrapper, in JSContextPtr cx, in JSObjectPtr obj); */
NS_IMETHODIMP
nsJSIID::Enumerate(nsIXPConnectWrappedNative *wrapper,
                   JSContext * cx, JSObject * obj, PRBool *_retval)
{
    // In this case, let's just eagerly resolve...

    XPCCallContext ccx(JS_CALLER, cx);

    XPCNativeInterface* iface =
        XPCNativeInterface::GetNewOrUsed(ccx, mDetails.GetID());

    if(!iface)
        return NS_OK;

    PRUint16 count = iface->GetMemberCount();
    for(PRUint16 i = 0; i < count; i++)
    {
        XPCNativeMember* member = iface->GetMemberAt(i);
        if(member && member->IsConstant())
        {
            JSObject* obj2;
            JSProperty* prop;
            jsid id;

            if(!JS_ValueToId(cx, member->GetName(), &id) ||
               !OBJ_LOOKUP_PROPERTY(cx, obj, id, &obj2, &prop))
                return NS_ERROR_UNEXPECTED;
        }
    }
    return NS_OK;
}

/* PRBool hasInstance (in nsIXPConnectWrappedNative wrapper, in JSContextPtr cx, in JSObjectPtr obj, in JSVal val, out PRBool bp); */
NS_IMETHODIMP
nsJSIID::HasInstance(nsIXPConnectWrappedNative *wrapper,
                     JSContext * cx, JSObject * obj,
                     jsval val, PRBool *bp, PRBool *_retval)
{
    *bp = JS_FALSE;
    nsresult rv = NS_OK;

    if(!JSVAL_IS_PRIMITIVE(val))
    {
        // we have a JSObject
        JSObject* obj = JSVAL_TO_OBJECT(val);

        NS_ASSERTION(obj, "when is an object not an object?");

        // is this really a native xpcom object with a wrapper?
        XPCWrappedNative* other_wrapper =
           XPCWrappedNative::GetWrappedNativeOfJSObject(cx, obj);

        if(!other_wrapper)
            return NS_OK;

        // We'll trust the interface set of the wrapper if this is known
        // to be an interface that the objects *expects* to be able to
        // handle.
        if(other_wrapper->HasInterfaceNoQI(*mDetails.GetID()))
        {
            *bp = JS_TRUE;
            return NS_OK;
        }

        // Otherwise, we'll end up Querying the native object to be sure.
        XPCCallContext ccx(JS_CALLER, cx);

        XPCNativeInterface* iface =
            XPCNativeInterface::GetNewOrUsed(ccx, mDetails.GetID());

        if(iface && other_wrapper->FindTearOff(ccx, iface))
            *bp = JS_TRUE;
    }
    return rv;
}

/***************************************************************************/

NS_IMPL_THREADSAFE_ISUPPORTS3(nsJSCID, nsIJSID, nsIJSCID, nsIXPCScriptable)

// The nsIXPCScriptable map declaration that will generate stubs for us...
#define XPC_MAP_CLASSNAME           nsJSCID
#define XPC_MAP_QUOTED_CLASSNAME   "nsJSCID"
#define                             XPC_MAP_WANT_CONSTRUCT
#define                             XPC_MAP_WANT_HASINSTANCE
#define XPC_MAP_FLAGS               0
#include "xpc_map_end.h" /* This will #undef the above */

nsJSCID::nsJSCID()  {NS_INIT_ISUPPORTS();}
nsJSCID::~nsJSCID() {}

NS_IMETHODIMP nsJSCID::GetName(char * *aName)
    {ResolveName(); return mDetails.GetName(aName);}

NS_IMETHODIMP nsJSCID::GetNumber(char * *aNumber)
    {return mDetails.GetNumber(aNumber);}

NS_IMETHODIMP nsJSCID::GetId(nsID* *aId)
    {return mDetails.GetId(aId);}

NS_IMETHODIMP nsJSCID::GetValid(PRBool *aValid)
    {return mDetails.GetValid(aValid);}

NS_IMETHODIMP nsJSCID::Equals(nsIJSID *other, PRBool *_retval)
    {return mDetails.Equals(other, _retval);}

NS_IMETHODIMP nsJSCID::Initialize(const char *idString)
    {return mDetails.Initialize(idString);}

NS_IMETHODIMP nsJSCID::ToString(char **_retval)
    {ResolveName(); return mDetails.ToString(_retval);}

void
nsJSCID::ResolveName()
{
    if(!mDetails.NameIsSet())
        mDetails.SetNameToNoString();
}

//static
nsJSCID*
nsJSCID::NewID(const char* str)
{
    if(!str)
    {
        NS_ASSERTION(0,"no string");
        return nsnull;
    }

    nsJSCID* idObj = new nsJSCID();
    if(idObj)
    {
        PRBool success = PR_FALSE;
        NS_ADDREF(idObj);

        if(str[0] == '{')
        {
            if(NS_SUCCEEDED(idObj->Initialize(str)))
                success = PR_TRUE;
        }
        else
        {
            nsCID cid;
            if(NS_SUCCEEDED(nsComponentManager::ContractIDToClassID(str, &cid)))
            {
                success = idObj->mDetails.InitWithName(cid, str);
            }
        }
        if(!success)
            NS_RELEASE(idObj);
    }
    return idObj;
}


/* nsISupports createInstance (); */
NS_IMETHODIMP
nsJSCID::CreateInstance(nsISupports **_retval)
{
    if(!mDetails.IsValid())
        return NS_ERROR_XPC_BAD_CID;

    nsCOMPtr<nsXPConnect> xpc(dont_AddRef(nsXPConnect::GetXPConnect()));
    if(!xpc)
        return NS_ERROR_UNEXPECTED;

    nsCOMPtr<nsIXPCNativeCallContext> ccxp;
    xpc->GetCurrentNativeCallContext(getter_AddRefs(ccxp));
    if(!ccxp)
        return NS_ERROR_UNEXPECTED;

    PRUint32 argc;
    jsval * argv;
    jsval * vp;
    JSContext* cx;
    JSObject* obj;

    ccxp->GetJSContext(&cx);
    ccxp->GetArgc(&argc);
    ccxp->GetArgvPtr(&argv);
    ccxp->GetRetValPtr(&vp);

    nsCOMPtr<nsIXPConnectWrappedNative> wrapper;
    ccxp->GetCalleeWrapper(getter_AddRefs(wrapper));
    wrapper->GetJSObject(&obj);

    // Do the security check if necessary

    XPCContext* xpcc = nsXPConnect::GetContext(cx);

    nsIXPCSecurityManager* sm;
    sm = xpcc->GetAppropriateSecurityManager(
                        nsIXPCSecurityManager::HOOK_CREATE_INSTANCE);
    if(sm && NS_FAILED(sm->CanCreateInstance(cx, *mDetails.GetID())))
    {
        // the security manager vetoed. It should have set an exception.
        ccxp->SetExceptionWasThrown(JS_TRUE);
        return NS_OK;
    }

    nsID iid;

    // If an IID was passed in then use it
    if(argc)
    {
        JSObject* iidobj;
        jsval val = *argv;
        nsID* piid = nsnull;
        if(JSVAL_IS_PRIMITIVE(val) ||
           !(iidobj = JSVAL_TO_OBJECT(val)) ||
           !(piid = xpc_JSObjectToID(cx, iidobj)))
        {
            return NS_ERROR_XPC_BAD_IID;
        }
        iid = *piid;
        nsMemory::Free(piid);
    }
    else
        iid = NS_GET_IID(nsISupports);

    nsCOMPtr<nsISupports> inst;
    nsresult rv;

    rv = nsComponentManager::CreateInstance(*mDetails.GetID(), nsnull, iid,
                                            (void**) getter_AddRefs(inst));
    NS_ASSERTION(NS_FAILED(rv) || inst, "component manager returned success, but instance is null!");

    if(NS_FAILED(rv) || !inst)
        return NS_ERROR_XPC_CI_RETURNED_FAILURE;

    JSObject* instJSObj;
    nsCOMPtr<nsIXPConnectJSObjectHolder> holder;
    rv = xpc->WrapNative(cx, obj, inst, iid, getter_AddRefs(holder));
    if(NS_FAILED(rv) || !holder || NS_FAILED(holder->GetJSObject(&instJSObj)))
        return NS_ERROR_XPC_CANT_CREATE_WN;

    *vp = OBJECT_TO_JSVAL(instJSObj);
    ccxp->SetReturnValueWasSet(JS_TRUE);
    return NS_OK;
}

/* nsISupports getService (); */
NS_IMETHODIMP
nsJSCID::GetService(nsISupports **_retval)
{
    if(!mDetails.IsValid())
        return NS_ERROR_XPC_BAD_CID;

    nsCOMPtr<nsXPConnect> xpc(dont_AddRef(nsXPConnect::GetXPConnect()));
    if(!xpc)
        return NS_ERROR_UNEXPECTED;

    nsCOMPtr<nsIXPCNativeCallContext> ccxp;
    xpc->GetCurrentNativeCallContext(getter_AddRefs(ccxp));
    if(!ccxp)
        return NS_ERROR_UNEXPECTED;

    PRUint32 argc;
    jsval * argv;
    jsval * vp;
    JSContext* cx;
    JSObject* obj;

    ccxp->GetJSContext(&cx);
    ccxp->GetArgc(&argc);
    ccxp->GetArgvPtr(&argv);
    ccxp->GetRetValPtr(&vp);

    nsCOMPtr<nsIXPConnectWrappedNative> wrapper;
    ccxp->GetCalleeWrapper(getter_AddRefs(wrapper));
    wrapper->GetJSObject(&obj);

    // Do the security check if necessary

    XPCContext* xpcc = nsXPConnect::GetContext(cx);

    nsIXPCSecurityManager* sm;
    sm = xpcc->GetAppropriateSecurityManager(
                        nsIXPCSecurityManager::HOOK_GET_SERVICE);
    if(sm && NS_FAILED(sm->CanCreateInstance(cx, *mDetails.GetID())))
    {
        // the security manager vetoed. It should have set an exception.
        ccxp->SetExceptionWasThrown(JS_TRUE);
        return NS_OK;
    }

    nsID iid;

    // If an IID was passed in then use it
    if(argc)
    {
        JSObject* iidobj;
        jsval val = *argv;
        nsID* piid = nsnull;
        if(JSVAL_IS_PRIMITIVE(val) ||
           !(iidobj = JSVAL_TO_OBJECT(val)) ||
           !(piid = xpc_JSObjectToID(cx, iidobj)))
        {
            return NS_ERROR_XPC_BAD_IID;
        }
        iid = *piid;
        nsMemory::Free(piid);
    }
    else
        iid = NS_GET_IID(nsISupports);

    nsCOMPtr<nsISupports> srvc;
    nsresult rv;

    rv = nsServiceManager::GetService(*mDetails.GetID(), iid, 
                                      getter_AddRefs(srvc), nsnull);
    NS_ASSERTION(NS_FAILED(rv) || srvc, "service manager returned success, but service is null!");
    if(NS_FAILED(rv) || !srvc)
        return NS_ERROR_XPC_GS_RETURNED_FAILURE;

    JSObject* instJSObj;
    nsCOMPtr<nsIXPConnectJSObjectHolder> holder;
    rv = xpc->WrapNative(cx, obj, srvc, iid, getter_AddRefs(holder));
    if(NS_FAILED(rv) || !holder || NS_FAILED(holder->GetJSObject(&instJSObj)))
        return NS_ERROR_XPC_CANT_CREATE_WN;

    *vp = OBJECT_TO_JSVAL(instJSObj);
    ccxp->SetReturnValueWasSet(JS_TRUE);
    return NS_OK;
}

/* PRBool construct (in nsIXPConnectWrappedNative wrapper, in JSContextPtr cx, in JSObjectPtr obj, in PRUint32 argc, in JSValPtr argv, in JSValPtr vp); */
NS_IMETHODIMP
nsJSCID::Construct(nsIXPConnectWrappedNative *wrapper,
                   JSContext * cx, JSObject * obj,
                   PRUint32 argc, jsval * argv, jsval * vp,
                   PRBool *_retval)
{
    XPCJSRuntime* rt = nsXPConnect::GetRuntime();
    if(!rt)
        return NS_ERROR_FAILURE;

    // 'push' a call context and call on it
    XPCCallContext ccx(JS_CALLER, cx, obj,
                       rt->GetStringJSVal(XPCJSRuntime::IDX_CREATE_INSTANCE),
                       argc, argv, vp);

    *_retval = XPCWrappedNative::CallMethod(ccx);
    return NS_OK;
}

/* PRBool hasInstance (in nsIXPConnectWrappedNative wrapper, in JSContextPtr cx, in JSObjectPtr obj, in JSVal val, out PRBool bp); */
NS_IMETHODIMP
nsJSCID::HasInstance(nsIXPConnectWrappedNative *wrapper,
                     JSContext * cx, JSObject * obj,
                     jsval val, PRBool *bp, PRBool *_retval)
{
    *bp = JS_FALSE;
    nsresult rv = NS_OK;

    if(!JSVAL_IS_PRIMITIVE(val))
    {
        // we have a JSObject
        JSObject* obj = JSVAL_TO_OBJECT(val);

        NS_ASSERTION(obj, "when is an object not an object?");

        // is this really a native xpcom object with a wrapper?
        XPCWrappedNative* other_wrapper =
           XPCWrappedNative::GetWrappedNativeOfJSObject(cx, obj);

        if(!other_wrapper)
            return NS_OK;

        // We consider CID equality to be the thing that matters here.
        // This is perhaps debatable.
        nsIClassInfo* ci = other_wrapper->GetProto()->GetClassInfo();
        if(ci)
        {
            nsID* cid;
            if(NS_SUCCEEDED(ci->GetClassID(&cid)) && cid)
            {
                *bp = cid->Equals(*mDetails.GetID());
                nsMemory::Free((char*)cid);
            }
        }
    }
    return rv;
}

/***************************************************************************/
// additional utilities...

JSObject *
xpc_NewIDObject(JSContext *cx, JSObject* jsobj, const nsID& aID)
{
    JSObject *obj = nsnull;

    char* idString = aID.ToString();
    if(idString)
    {
        nsCOMPtr<nsIJSID> iid =
            dont_AddRef(NS_STATIC_CAST(nsIJSID*, nsJSID::NewID(idString)));
        nsCRT::free(idString);
        if(iid)
        {
            nsCOMPtr<nsXPConnect> xpc =
                dont_AddRef(nsXPConnect::GetXPConnect());
            if(xpc)
            {
                nsCOMPtr<nsIXPConnectJSObjectHolder> holder;
                nsresult rv = xpc->WrapNative(cx, jsobj,
                                              NS_STATIC_CAST(nsISupports*,iid),
                                              NS_GET_IID(nsIJSID),
                                              getter_AddRefs(holder));
                if(NS_SUCCEEDED(rv) && holder)
                {
                    holder->GetJSObject(&obj);
                }
            }
        }
    }
    return obj;
}

nsID*
xpc_JSObjectToID(JSContext *cx, JSObject* obj)
{
    nsID* id = nsnull;
    if(!cx || !obj)
        return nsnull;

    // NOTE: this call does NOT addref
    XPCWrappedNative* wrapper =
        XPCWrappedNative::GetWrappedNativeOfJSObject(cx, obj);
    if(wrapper)
    {
        if(wrapper->HasInterfaceNoQI(NS_GET_IID(nsIJSID))  ||
           wrapper->HasInterfaceNoQI(NS_GET_IID(nsIJSIID)) ||
           wrapper->HasInterfaceNoQI(NS_GET_IID(nsIJSCID)))
        {
            ((nsIJSID*)wrapper->GetIdentityObject())->GetId(&id);
        }
    }
    return id;
}

