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

/* All the XPConnect private declarations - only include locally. */

#ifndef xpcprivate_h___
#define xpcprivate_h___

#include <string.h>
#include <stdlib.h>
#include "nscore.h"
#include "nsISupports.h"
#include "nsIServiceManager.h"
#include "nsIComponentManager.h"
#include "nsIAllocator.h"
#include "nsIXPConnect.h"
#include "nsIInterfaceInfo.h"
#include "nsIInterfaceInfoManager.h"
#include "nsIXPCScriptable.h"
#include "jsapi.h"
#include "jshash.h"
#include "jsprf.h"
#include "xpt_cpp.h"
#include "xpcforwards.h"
#include "xpcvariant.h"
#include "xpclog.h"
#include "xpccomponents.h"
#include "xpcjsid.h"

extern const char* XPC_VAL_STR; // 'value' property name for out params

/***************************************************************************/

class nsXPConnect : public nsIXPConnect
{
    // all the interface method declarations...
    NS_DECL_ISUPPORTS;

    NS_IMETHOD InitJSContext(JSContext* aJSContext,
                             JSObject* aGlobalJSObj);

    NS_IMETHOD InitJSContextWithNewWrappedGlobal(JSContext* aJSContext,
                          nsISupports* aCOMObj,
                          REFNSIID aIID,
                          nsIXPConnectWrappedNative** aWrapper);

    NS_IMETHOD WrapNative(JSContext* aJSContext,
                          nsISupports* aCOMObj,
                          REFNSIID aIID,
                          nsIXPConnectWrappedNative** aWrapper);

    NS_IMETHOD WrapJS(JSContext* aJSContext,
                      JSObject* aJSObj,
                      REFNSIID aIID,
                      nsIXPConnectWrappedJS** aWrapper);

    NS_IMETHOD GetWrappedNativeOfJSObject(JSContext* aJSContext,
                                    JSObject* aJSObj,
                                    nsIXPConnectWrappedNative** aWrapper);

    NS_IMETHOD DebugDump(int depth);

    // non-interface implementation
public:
    static nsXPConnect* GetXPConnect();
    static nsIAllocator* GetAllocator(nsXPConnect* xpc = NULL);
    static nsIInterfaceInfoManager* GetInterfaceInfoManager(nsXPConnect* xpc = NULL);
    static XPCContext*  GetContext(JSContext* cx, nsXPConnect* xpc = NULL);
    static XPCJSThrower* GetJSThrower(nsXPConnect* xpc = NULL);
    static JSBool IsISupportsDescendent(nsIInterfaceInfo* info);

    JSContext2XPCContextMap* GetContextMap() {return mContextMap;}
    nsIXPCScriptable* GetArbitraryScriptable() {return mArbitraryScriptable;}

    virtual ~nsXPConnect();
private:
    nsXPConnect();
    XPCContext*  NewContext(JSContext* cx, JSObject* global,
                            JSBool doInit = JS_TRUE);

private:
    static nsXPConnect* mSelf;
    JSContext2XPCContextMap* mContextMap;
    nsIAllocator* mAllocator;
    nsIXPCScriptable* mArbitraryScriptable;
    nsIInterfaceInfoManager* mInterfaceInfoManager;
    XPCJSThrower* mThrower;
};

/***************************************************************************/
// XPCContext is mostly a dumb class to hold JSContext specific data and
// maps that let us find wrappers created for the given JSContext.

// no virtuals
class XPCContext
{
public:
    static XPCContext* newXPCContext(JSContext* aJSContext,
                                   JSObject* aGlobalObj,
                                   int WrappedJSMapSize,
                                   int WrappedNativeMapSize,
                                   int WrappedJSClassMapSize,
                                   int WrappedNativeClassMapSize);

    JSContext*      GetJSContext()      {return mJSContext;}
    JSObject*       GetGlobalObject()   {return mGlobalObj;}
    nsXPConnect*    GetXPConnect()      {return mXPConnect;}

    JSObject2WrappedJSMap*     GetWrappedJSMap()          {return mWrappedJSMap;}
    Native2WrappedNativeMap*   GetWrappedNativeMap()      {return mWrappedNativeMap;}
    IID2WrappedJSClassMap*     GetWrappedJSClassMap()     {return mWrappedJSClassMap;}
    IID2WrappedNativeClassMap* GetWrappedNativeClassMap() {return mWrappedNativeClassMap;}

    JSBool Init(JSObject* aGlobalObj = NULL);
    void DebugDump(int depth);

    virtual ~XPCContext();
private:
    XPCContext();    // no implementation
    XPCContext(JSContext* aJSContext,
              JSObject* aGlobalObj,
              int WrappedJSMapSize,
              int WrappedNativeMapSize,
              int WrappedJSClassMapSize,
              int WrappedNativeClassMapSize);
private:
    nsXPConnect* mXPConnect;
    JSContext*  mJSContext;
    JSObject*   mGlobalObj;
    JSObject2WrappedJSMap* mWrappedJSMap;
    Native2WrappedNativeMap* mWrappedNativeMap;
    IID2WrappedJSClassMap* mWrappedJSClassMap;
    IID2WrappedNativeClassMap* mWrappedNativeClassMap;
};

/***************************************************************************/
// code for throwing exceptions into JS

struct XPCJSErrorFormatString
{
    const char *format;
/*    uintN argCount; */
};

struct XPCJSError
{
    enum {
#define MSG_DEF(name, number, count, exception, format) \
        name = number,
#include "xpc.msg"
#undef MSG_DEF
        LIMIT
    };
};

class XPCJSThrower
{
public:
    void ThrowBadResultException(JSContext* cx, 
                                 nsXPCWrappedNativeClass* clazz,
                                 const XPCNativeMemberDescriptor* desc,
                                 nsresult result);

    void ThrowBadParamException(uintN errNum,
                                JSContext* cx, 
                                nsXPCWrappedNativeClass* clazz,
                                const XPCNativeMemberDescriptor* desc,
                                uintN paramNum);

    void ThrowException(uintN errNum,
                        JSContext* cx, 
                        nsXPCWrappedNativeClass* clazz,
                        const XPCNativeMemberDescriptor* desc);

    XPCJSThrower(JSBool Verbose = JS_FALSE);
    ~XPCJSThrower();

private:

    void Verbosify(nsXPCWrappedNativeClass* clazz,
                   const XPCNativeMemberDescriptor* desc,
                   char** psz, PRBool own);

private:
    static XPCJSErrorFormatString default_formats[XPCJSError::LIMIT+1];
    XPCJSErrorFormatString* mFormats;
    JSBool mVerbose;
};

/***************************************************************************/

// this interfaces exists so we can refcount nsXPCWrappedJSClass
// {2453EBA0-A9B8-11d2-BA64-00805F8A5DD7}
#define NS_IXPCONNECT_WRAPPED_JS_CLASS_IID  \
{ 0x2453eba0, 0xa9b8, 0x11d2,               \
  { 0xba, 0x64, 0x0, 0x80, 0x5f, 0x8a, 0x5d, 0xd7 } }

class nsIXPCWrappedJSClass : public nsISupports
{
public:
    NS_DEFINE_STATIC_IID_ACCESSOR(NS_IXPCONNECT_WRAPPED_JS_CLASS_IID)
    NS_IMETHOD DebugDump(int depth) = 0;
};

/*************************/

class nsXPCWrappedJSClass : public nsIXPCWrappedJSClass
{
    // all the interface method declarations...
    NS_DECL_ISUPPORTS;
    NS_IMETHOD DebugDump(int depth);
public:

    static nsXPCWrappedJSClass* GetNewOrUsedClass(XPCContext* xpcc,
                                                  REFNSIID aIID);
    REFNSIID GetIID() const {return mIID;}
    XPCContext*  GetXPCContext() const {return mXPCContext;}
    nsIInterfaceInfo* GetInterfaceInfo() const {return mInfo;}

    static JSBool InitForContext(XPCContext* xpcc);
    static JSBool IsWrappedJS(nsISupports* aPtr);

    NS_IMETHOD DelegatedQueryInterface(nsXPCWrappedJS* self, REFNSIID aIID,
                                       void** aInstancePtr);

    JSObject* GetRootJSObject(JSObject* aJSObj);

    NS_IMETHOD CallMethod(nsXPCWrappedJS* wrapper, uint16 methodIndex,
                        const nsXPTMethodInfo* info,
                        nsXPCMiniVariant* params);

    virtual ~nsXPCWrappedJSClass();
private:
    nsXPCWrappedJSClass();   // not implemented
    nsXPCWrappedJSClass(XPCContext* xpcc, REFNSIID aIID,
                        nsIInterfaceInfo* aInfo);

    JSContext* GetJSContext() const {return mXPCContext->GetJSContext();}
    JSObject*  CreateIIDJSObject(REFNSIID aIID);
    JSObject*  NewOutObject();

    JSObject*  CallQueryInterfaceOnJSObject(JSObject* jsobj, REFNSIID aIID);

    JSBool IsReflectable(uint16 i) const
        {return mDescriptors[i/32] & (1 << (i%32));}
    void SetReflectable(uint16 i, JSBool b)
        {if(b) mDescriptors[i/32] |= (1 << (i%32));
         else  mDescriptors[i/32] &= ~(1 << (i%32));}

private:
    XPCContext* mXPCContext;
    nsIInterfaceInfo* mInfo;
    nsIID mIID;
    uint32* mDescriptors;
};

/*************************/

class nsXPCWrappedJS : public nsIXPConnectWrappedJS
{
public:
    NS_DECL_ISUPPORTS;
    // include our generated vtbl stub declarations
#include "xpcstubsdecl.inc"

    static nsXPCWrappedJS* GetNewOrUsedWrapper(XPCContext* xpcc,
                                               JSObject* aJSObj,
                                               REFNSIID aIID);

    JSObject* GetJSObject() const {return mJSObj;}
    nsXPCWrappedJSClass*  GetClass() const {return mClass;}
    REFNSIID GetIID() const {return GetClass()->GetIID();}
    nsXPCWrappedJS* GetRootWrapper() const {return mRoot;}
    void DebugDump(int depth);

    virtual ~nsXPCWrappedJS();
private:
    nsXPCWrappedJS();   // not implemented
    nsXPCWrappedJS(JSObject* aJSObj,
                   nsXPCWrappedJSClass* aClass,
                   nsXPCWrappedJS* root);

    nsXPCWrappedJS* Find(REFNSIID aIID);

private:
    JSObject* mJSObj;
    nsXPCWrappedJSClass* mClass;
    nsXPCWrappedJSMethods* mMethods;
    nsXPCWrappedJS* mRoot;
    nsXPCWrappedJS* mNext;
};

class nsXPCWrappedJSMethods : public nsIXPConnectWrappedJSMethods
{
public:
    NS_DECL_ISUPPORTS;
    NS_IMETHOD GetJSObject(JSObject** aJSObj);
    NS_IMETHOD GetInterfaceInfo(nsIInterfaceInfo** info);
    NS_IMETHOD GetIID(nsIID** iid); // returns IAllocatator alloc'd copy
    NS_IMETHOD DebugDump(int depth);

    nsXPCWrappedJSMethods(nsXPCWrappedJS* aWrapper);
    virtual ~nsXPCWrappedJSMethods();
    // used in nsXPCWrappedJS::DebugDump
    int GetRefCnt() const {return mRefCnt;}

private:
    nsXPCWrappedJSMethods();  // not implemented

private:
    nsXPCWrappedJS* mWrapper;
};

/***************************************************************************/

// nsXPCWrappedNativeClass maintains an array of these things
class XPCNativeMemberDescriptor
{
private:
    enum {
        // these are all bitwise flags!
        NMD_CONSTANT    = 0x0, // categories...
        NMD_METHOD      = 0x1,
        NMD_ATTRIB_RO   = 0x2,
        NMD_ATTRIB_RW   = 0x3,
        NMD_CAT_MASK    = 0x3, // & mask for the categories above
        // any new bits start at 0x04
    };

public:
    JSObject*       invokeFuncObj;
    jsid            id;  /* hashed name for quick JS property lookup */
    uintN           index; /* in InterfaceInfo for const, method, and get */
    uintN           index2; /* in InterfaceInfo for set */
private:
    uint16          flags;
public:

    JSBool IsConstant() const  {return (flags & NMD_CAT_MASK) == NMD_CONSTANT;}
    JSBool IsMethod() const    {return (flags & NMD_CAT_MASK) == NMD_METHOD;}
    JSBool IsAttribute() const {return (JSBool)((flags & NMD_CAT_MASK) & NMD_ATTRIB_RO);}
    JSBool IsWritableAttribute() const {return (flags & NMD_CAT_MASK) == NMD_ATTRIB_RW;}
    JSBool IsReadOnlyAttribute() const {return (flags & NMD_CAT_MASK) == NMD_ATTRIB_RO;}

    void SetConstant()          {flags=(flags&~NMD_CAT_MASK)|NMD_CONSTANT;}
    void SetMethod()            {flags=(flags&~NMD_CAT_MASK)|NMD_METHOD;}
    void SetReadOnlyAttribute() {flags=(flags&~NMD_CAT_MASK)|NMD_ATTRIB_RO;}
    void SetWritableAttribute() {flags=(flags&~NMD_CAT_MASK)|NMD_ATTRIB_RW;}

    XPCNativeMemberDescriptor();
};

/*************************/

// this interfaces exists just so we can refcount nsXPCWrappedNativeClass
// {C9E36280-954A-11d2-BA5A-00805F8A5DD7}
#define NS_IXPCONNECT_WRAPPED_NATIVE_CLASS_IID \
{ 0xc9e36280, 0x954a, 0x11d2,                   \
  { 0xba, 0x5a, 0x0, 0x80, 0x5f, 0x8a, 0x5d, 0xd7 } }

class nsIXPCWrappedNativeClass : public nsISupports
{
public:
    NS_DEFINE_STATIC_IID_ACCESSOR(NS_IXPCONNECT_WRAPPED_NATIVE_CLASS_IID)
    NS_IMETHOD DebugDump(int depth) = 0;
};

/*************************/

class nsXPCWrappedNativeClass : public nsIXPCWrappedNativeClass
{
    // all the interface method declarations...
    NS_DECL_ISUPPORTS;
    NS_IMETHOD DebugDump(int depth);
public:
    static nsXPCWrappedNativeClass* GetNewOrUsedClass(XPCContext* xpcc,
                                                     REFNSIID aIID);

    REFNSIID GetIID() const {return mIID;}
    const char* GetInterfaceName();
    const char* GetMemberName(const XPCNativeMemberDescriptor* desc) const;
    nsIInterfaceInfo* GetInterfaceInfo() const {return mInfo;}
    XPCContext*  GetXPCContext() const {return mXPCContext;}
    JSContext* GetJSContext() const {return mXPCContext->GetJSContext();}
    nsIXPCScriptable* GetArbitraryScriptable() const
        {return GetXPCContext()->GetXPConnect()->GetArbitraryScriptable();}

    static JSBool InitForContext(XPCContext* xpcc);
    static JSBool OneTimeInit();

    JSObject* NewInstanceJSObject(nsXPCWrappedNative* self);
    static nsXPCWrappedNative* GetWrappedNativeOfJSObject(JSContext* cx,
                                                          JSObject* jsobj);

    int GetMemberCount() const {return mMemberCount;}
    const XPCNativeMemberDescriptor* GetMemberDescriptor(uint16 i) const
    {
        NS_PRECONDITION(i < mMemberCount,"bad index");
        return &mDescriptors[i];
    }

    const XPCNativeMemberDescriptor* LookupMemberByID(jsid id) const;

    JSBool GetConstantAsJSVal(JSContext* cx,
                              nsXPCWrappedNative* wrapper,
                              const XPCNativeMemberDescriptor* desc,
                              jsval* vp);

    JSBool CallWrappedMethod(JSContext* cx,
                             nsXPCWrappedNative* wrapper,
                             const XPCNativeMemberDescriptor* desc,
                             JSBool isAttributeSet,
                             uintN argc, jsval *argv, jsval *vp);

    JSBool GetAttributeAsJSVal(JSContext* cx,
                               nsXPCWrappedNative* wrapper,
                               const XPCNativeMemberDescriptor* desc,
                               jsval* vp)
    {
        return CallWrappedMethod(cx, wrapper, desc, JS_FALSE, 0, NULL, vp);
    }

    JSBool SetAttributeFromJSVal(JSContext* cx,
                                 nsXPCWrappedNative* wrapper,
                                 const XPCNativeMemberDescriptor* desc,
                                 jsval* vp)
    {
        return CallWrappedMethod(cx, wrapper, desc, JS_TRUE, 1, vp, NULL);
    }

    JSObject* GetInvokeFunObj(const XPCNativeMemberDescriptor* desc);

    JSBool DynamicEnumerate(nsXPCWrappedNative* wrapper,
                            nsIXPCScriptable* ds,
                            JSContext *cx, JSObject *obj,
                            JSIterateOp enum_op,
                            jsval *statep, jsid *idp);

    JSBool StaticEnumerate(nsXPCWrappedNative* wrapper,
                           JSIterateOp enum_op,
                           jsval *statep, jsid *idp);

    virtual ~nsXPCWrappedNativeClass();
private:
    nsXPCWrappedNativeClass();   // not implemented
    nsXPCWrappedNativeClass(XPCContext* xpcc, REFNSIID aIID,
                           nsIInterfaceInfo* aInfo);

    void ThrowBadResultException(JSContext* cx, 
                                 const XPCNativeMemberDescriptor* desc,
                                 nsresult result)
        {nsXPConnect::GetJSThrower()->
                ThrowBadResultException(cx, this, desc, result);}

    void ThrowBadParamException(uintN errNum,
                                JSContext* cx, 
                                const XPCNativeMemberDescriptor* desc,
                                uintN paramNum)
        {nsXPConnect::GetJSThrower()->
                ThrowBadParamException(errNum, cx, this, desc, paramNum);}

    void ThrowException(uintN errNum,
                        JSContext* cx, 
                        const XPCNativeMemberDescriptor* desc)
        {nsXPConnect::GetJSThrower()->
                ThrowException(errNum, cx, this, desc);}

    JSBool BuildMemberDescriptors();
    void  DestroyMemberDescriptors();

private:
    XPCContext* mXPCContext;
    nsIID mIID;
    char* mName;
    nsIInterfaceInfo* mInfo;
    int mMemberCount;
    XPCNativeMemberDescriptor* mDescriptors;
};

/*************************/

class nsXPCWrappedNative : public nsIXPConnectWrappedNative
{
    // all the interface method declarations...
    NS_DECL_ISUPPORTS;

    NS_IMETHOD GetDynamicScriptable(nsIXPCScriptable** p);
    NS_IMETHOD GetArbitraryScriptable(nsIXPCScriptable** p);
    NS_IMETHOD GetJSObject(JSObject** aJSObj);
    NS_IMETHOD GetNative(nsISupports** aObj);
    NS_IMETHOD GetInterfaceInfo(nsIInterfaceInfo** info);
    NS_IMETHOD GetIID(nsIID** iid); // returns IAllocatator alloc'd copy
    NS_IMETHOD DebugDump(int depth);

public:
    static nsXPCWrappedNative* GetNewOrUsedWrapper(XPCContext* xpcc,
                                                  nsISupports* aObj,
                                                  REFNSIID aIID);
    nsISupports* GetNative() const {return mObj;}
    JSObject* GetJSObject() const {return mJSObj;}
    nsXPCWrappedNativeClass* GetClass() const {return mClass;}
    REFNSIID GetIID() const {return GetClass()->GetIID();}

    nsIXPCScriptable* GetDynamicScriptable() const
        {return mRoot->mDynamicScriptable;}

    nsIXPCScriptable* GetArbitraryScriptable() const
        {return GetClass()->GetArbitraryScriptable();}

    void JSObjectFinalized();

    virtual ~nsXPCWrappedNative();
private:
    nsXPCWrappedNative();    // not implemented
    nsXPCWrappedNative(nsISupports* aObj,
                      nsXPCWrappedNativeClass* aClass,
                      nsXPCWrappedNative* root);

    nsXPCWrappedNative* Find(REFNSIID aIID);

private:
    nsISupports* mObj;
    JSObject* mJSObj;
    nsXPCWrappedNativeClass* mClass;
    nsIXPCScriptable* mDynamicScriptable;   // only set in root!
    nsXPCWrappedNative* mRoot;
    nsXPCWrappedNative* mNext;
};

/***************************************************************************/

class nsXPCArbitraryScriptable : public nsIXPCScriptable
{
public:
    // all the interface method declarations...
    NS_DECL_ISUPPORTS;
    XPC_DECLARE_IXPCSCRIPTABLE;

public:
    nsXPCArbitraryScriptable();
};

/***************************************************************************/
// data convertion

// class here just for static methods
class XPCConvert
{
public:
    static JSBool IsMethodReflectable(const nsXPTMethodInfo& info);

    static JSBool NativeData2JS(JSContext* cx, jsval* d, const void* s,
                                const nsXPTType& type, const nsID* iid,
                                uintN* pErr);

    static JSBool JSData2Native(JSContext* cx, void* d, jsval s,
                                const nsXPTType& type,
                                nsIAllocator* al, const nsID* iid,
                                uintN* pErr);
private:
    XPCConvert(); // not implemented
};

/***************************************************************************/
// allocator access methods

// class here just for static methods
class XPCMem
{
public:
    static void* Alloc(PRUint32 size);
    static void* Realloc(void* ptr, PRUint32 size);
    static void  Free(void* ptr);
    static void  HeapMinimize();
    static void* Clone(const void* ptr,  PRUint32 size);
private:
    XPCMem();   // not implemented
    static nsIAllocator* Allocator();
};

/***************************************************************************/
// nsJSIID

class nsJSIID : public nsIJSIID
{
public:
    NS_DECL_ISUPPORTS;

    /* readonly attribute string name; */
    NS_IMETHOD GetName(char * *aName);

    /* readonly attribute string number; */
    NS_IMETHOD GetNumber(char * *aNumber);

    /* readonly attribute nsID id; */
    NS_IMETHOD GetId(nsID* *aId);

    /* readonly attribute boolean valid; */
    NS_IMETHOD GetValid(PRBool *aValid);

    /* boolean equals (in nsIJSID other); */
    NS_IMETHOD equals(nsIJSID *other, PRBool *_retval);

    /* boolean init (in string idString); */
    NS_IMETHOD init(const char *idString, PRBool *_retval);

    nsJSIID();
    virtual ~nsJSIID();

    static nsJSIID* NewID(const char* str);

private:
    void reset();
    void setName(const char* name);

private:
    nsID    mID;
    char*   mNumber;
    char*   mName;
};

/***************************************************************************/
// nsJSCID

class nsJSCID : public nsIJSCID
{
public:
    NS_DECL_ISUPPORTS;

    /* readonly attribute string name; */
    NS_IMETHOD GetName(char * *aName);

    /* readonly attribute string number; */
    NS_IMETHOD GetNumber(char * *aNumber);

    /* readonly attribute nsID id; */
    NS_IMETHOD GetId(nsID* *aId);

    /* readonly attribute boolean valid; */
    NS_IMETHOD GetValid(PRBool *aValid);

    /* boolean equals (in nsIJSID other); */
    NS_IMETHOD equals(nsIJSID *other, PRBool *_retval);

    /* boolean init (in string idString); */
    NS_IMETHOD init(const char *idString, PRBool *_retval);

    /* nsISupports newInstance (); */
    NS_IMETHOD newInstance(nsISupports **_retval);

    nsJSCID();
    virtual ~nsJSCID();

    static nsJSCID* NewID(const char* str);

private:
    void reset();
    void setName(const char* name);

private:
    nsID    mID;
    char*   mNumber;
    char*   mName;
};

JSObject*
xpc_NewIIDObject(JSContext *cx, const nsID& aID);

nsID*
xpc_JSObjectToID(JSContext *cx, JSObject* obj);

/***************************************************************************/
// 'Components' objects

class nsXPCInterfaces;
class nsXPCClasses;

class nsXPCComponents : public nsIXPCComponents
{
public:
    NS_DECL_ISUPPORTS;

    /* readonly attribute nsIXPCInterfaces interfaces; */
    NS_IMETHOD GetInterfaces(nsIXPCInterfaces * *aInterfaces);

    /* readonly attribute nsIXPCClasses classes; */
    NS_IMETHOD GetClasses(nsIXPCClasses * *aClasses);

    nsXPCComponents();
    virtual ~nsXPCComponents();
private:
    nsXPCInterfaces* mInterfaces;
    nsXPCClasses*    mClasses;
};

/***************************************************************************/
// platform specific method invoker
nsresult
xpc_InvokeNativeMethod(void* that, PRUint32 index,
                       uint32 paramCount, nsXPCVariant* params);

/***************************************************************************/
// module level stuff

void
xpc_RegisterSelf();

/***************************************************************************/

// the include of declarations of the maps comes last because they have
// inlines which call methods on classes above.

#include "xpcmaps.h"

#endif /* xpcprivate_h___ */
