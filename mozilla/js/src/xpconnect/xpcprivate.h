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
#include "nsIAllocator.h"
#include "nsIXPConnect.h"
#include "nsIInterfaceInfo.h"
#include "nsIInterfaceInfoManager.h"
#include "nsIXPCScriptable.h"
#include "jsapi.h"
#include "jshash.h"
#include "xpt_cpp.h"
#include "xpcforwards.h"
#include "xpcvariant.h"

extern const char* XPC_VAL_STR; // 'val' property name for out params

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

    // non-interface implementation
public:
    static nsXPConnect* GetXPConnect();
    static nsIAllocator* GetAllocator(nsXPConnect* xpc = NULL);
    static nsIInterfaceInfoManager* GetInterfaceInfoManager(nsXPConnect* xpc = NULL);
    static XPCContext*  GetContext(JSContext* cx, nsXPConnect* xpc = NULL);

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

    ~XPCContext();
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

// this interfaces exists so we can refcount nsXPCWrappedJSClass
// {2453EBA0-A9B8-11d2-BA64-00805F8A5DD7}
#define NS_IXPCONNECT_WRAPPED_JS_CLASS_IID  \
{ 0x2453eba0, 0xa9b8, 0x11d2,               \
  { 0xba, 0x64, 0x0, 0x80, 0x5f, 0x8a, 0x5d, 0xd7 } }

class nsIXPCWrappedJSClass : public nsISupports
{
    NS_DEFINE_STATIC_IID_ACCESSOR(NS_IXPCONNECT_WRAPPED_JS_CLASS_IID)
    // no methods
};

/*************************/

class nsXPCWrappedJSClass : public nsIXPCWrappedJSClass
{
    // all the interface method declarations...
    NS_DECL_ISUPPORTS;
public:

    static nsXPCWrappedJSClass* GetNewOrUsedClass(XPCContext* xpcc,
                                                  REFNSIID aIID);
    REFNSIID GetIID(){return mIID;}
    XPCContext*  GetXPCContext() {return mXPCContext;}
    nsIInterfaceInfo* GetInterfaceInfo() const {return mInfo;}

    static JSBool InitForContext(XPCContext* xpcc);
    static JSBool IsWrappedJS(nsISupports* aPtr);

    NS_IMETHOD DelegatedQueryInterface(nsXPCWrappedJS* self, REFNSIID aIID,
                                       void** aInstancePtr);

    JSObject* GetRootJSObject(JSObject* aJSObj);

    NS_IMETHOD CallMethod(nsXPCWrappedJS* wrapper, uint16 methodIndex,
                        const nsXPTMethodInfo* info,
                        nsXPCMiniVariant* params);

    ~nsXPCWrappedJSClass();
private:
    nsXPCWrappedJSClass();   // not implemented
    nsXPCWrappedJSClass(XPCContext* xpcc, REFNSIID aIID,
                        nsIInterfaceInfo* aInfo);

    JSContext* GetJSContext() {return mXPCContext->GetJSContext();}
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

    JSObject* GetJSObject() {return mJSObj;}
    nsXPCWrappedJSClass*  GetClass() {return mClass;}
    REFNSIID GetIID() {return GetClass()->GetIID();}
    nsXPCWrappedJS* GetRootWrapper() {return mRoot;}

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

    nsXPCWrappedJSMethods(nsXPCWrappedJS* aWrapper);
    virtual ~nsXPCWrappedJSMethods();

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

    JSBool IsConstant() const  {return (flags & NMD_CAT_MASK) == NMD_CONSTANT;}
    JSBool IsMethod() const    {return (flags & NMD_CAT_MASK) == NMD_METHOD;}
    JSBool IsAttribute() const {return (JSBool)((flags & NMD_CAT_MASK) & NMD_ATTRIB_RO);}
    JSBool IsWritableAttribute() const {return (flags & NMD_CAT_MASK) == NMD_ATTRIB_RW;}
    JSBool IsReadOnlyAttribute() const {return (flags & NMD_CAT_MASK) == NMD_ATTRIB_RO;}

    void SetConstant()          {flags=(flags&~NMD_CAT_MASK)|NMD_CONSTANT;}
    void SetMethod()            {flags=(flags&~NMD_CAT_MASK)|NMD_METHOD;}
    void SetReadOnlyAttribute() {flags=(flags&~NMD_CAT_MASK)|NMD_ATTRIB_RO;}
    void SetWritableAttribute() {flags=(flags&~NMD_CAT_MASK)|NMD_ATTRIB_RW;}

    XPCNativeMemberDescriptor()
        : invokeFuncObj(NULL), id(0), flags(0) {}
private:
    uint16          flags;
};

/*************************/

// this interfaces exists just so we can refcount nsXPCWrappedNativeClass
// {C9E36280-954A-11d2-BA5A-00805F8A5DD7}
#define NS_IXPCONNECT_WRAPPED_NATIVE_CLASS_IID \
{ 0xc9e36280, 0x954a, 0x11d2,                   \
  { 0xba, 0x5a, 0x0, 0x80, 0x5f, 0x8a, 0x5d, 0xd7 } }

class nsIXPCWrappedNativeClass : public nsISupports
{
    NS_DEFINE_STATIC_IID_ACCESSOR(NS_IXPCONNECT_WRAPPED_NATIVE_CLASS_IID)
    // no methods
};

/*************************/

class nsXPCWrappedNativeClass : public nsIXPCWrappedNativeClass
{
    // all the interface method declarations...
    NS_DECL_ISUPPORTS;

public:
    static nsXPCWrappedNativeClass* GetNewOrUsedClass(XPCContext* xpcc,
                                                     REFNSIID aIID);

    REFNSIID GetIID() const {return mIID;}
    const char* GetInterfaceName();
    nsIInterfaceInfo* GetInterfaceInfo() const {return mInfo;}
    XPCContext*  GetXPCContext() const {return mXPCContext;}
    JSContext* GetJSContext() {return mXPCContext->GetJSContext();}
    nsIXPCScriptable* GetArbitraryScriptable()
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

    JSBool CallWrappedMethod(nsXPCWrappedNative* wrapper,
                             const XPCNativeMemberDescriptor* desc,
                             JSBool isAttributeSet,
                             uintN argc, jsval *argv, jsval *vp);

    JSBool GetAttributeAsJSVal(nsXPCWrappedNative* wrapper,
                               const XPCNativeMemberDescriptor* desc,
                               jsval* vp)
    {
        return CallWrappedMethod(wrapper, desc, JS_FALSE, 0, NULL, vp);
    }

    JSBool SetAttributeFromJSVal(nsXPCWrappedNative* wrapper,
                                 const XPCNativeMemberDescriptor* desc,
                                 jsval* vp)
    {
        return CallWrappedMethod(wrapper, desc, JS_TRUE, 1, vp, NULL);
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

    ~nsXPCWrappedNativeClass();
private:
    nsXPCWrappedNativeClass();   // not implemented
    nsXPCWrappedNativeClass(XPCContext* xpcc, REFNSIID aIID,
                           nsIInterfaceInfo* aInfo);

    const char* GetMemberName(const XPCNativeMemberDescriptor* desc) const;

    void ReportError(const XPCNativeMemberDescriptor* desc, const char* msg);
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

public:
    static nsXPCWrappedNative* GetNewOrUsedWrapper(XPCContext* xpcc,
                                                  nsISupports* aObj,
                                                  REFNSIID aIID);
    nsISupports* GetNative() const {return mObj;}
    JSObject* GetJSObject() const {return mJSObj;}
    nsXPCWrappedNativeClass* GetClass() const {return mClass;}
    REFNSIID GetIID() const {return GetClass()->GetIID();}

    nsIXPCScriptable* GetDynamicScriptable()
        {return mRoot->mDynamicScriptable;}

    nsIXPCScriptable* GetArbitraryScriptable()
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
    nsXPCArbitraryScriptable() {NS_INIT_REFCNT();NS_ADDREF_THIS();}
};

/***************************************************************************/
// nsID JavaScript class functions

JSBool
xpc_InitIDClass(XPCContext* xpcc);

JSObject*
xpc_NewIDObject(JSContext *cx, const nsID& aID);

const nsID*
xpc_JSObjectToID(JSContext *cx, JSObject* obj);

/***************************************************************************/
// data convertion

// class here just for static methods
class XPCConvert
{
public:
    static JSBool IsMethodReflectable(const nsXPTMethodInfo& info);

    static JSBool NativeData2JS(JSContext* cx, jsval* d, const void* s,
                                const nsXPTType& type, const nsID* iid);

    static JSBool JSData2Native(JSContext* cx, void* d, jsval s,
                                const nsXPTType& type,
                                nsIAllocator* al, const nsID* iid);
private:
    XPCConvert(); // not implemented
};

/***************************************************************************/

// platform specific method invoker
nsresult
xpc_InvokeNativeMethod(void* that, PRUint32 index,
                       uint32 paramCount, nsXPCVariant* params);

/***************************************************************************/

// the include of declarations of the maps comes last because they have
// inlines which call methods on classes above.

#include "xpcmaps.h"

#endif /* xpcprivate_h___ */
