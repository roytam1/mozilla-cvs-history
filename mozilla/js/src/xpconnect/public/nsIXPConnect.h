/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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

/* All the XPConnect public interfaces. */

#ifndef nsIXPConnect_h___
#define nsIXPConnect_h___

#include "nsISupports.h"
#include "jspubtd.h"
#include "nsIInterfaceInfo.h"
#include "nsIInterfaceInfoManager.h"
#include "xptinfo.h"

#include "xpccomponents.h"
#include "xpcjsid.h"
#include "xpcexception.h"

/*
 * The linkage of XPC API functions differs depending on whether the file is
 * used within the XPC library or not.  Any source file within the XPC
 * library should define EXPORT_XPC_API whereas any client of the library
 * should not.
 */
#ifdef EXPORT_XPC_API
#define XPC_PUBLIC_API(t)    JS_EXPORT_API(t)
#define XPC_PUBLIC_DATA(t)   JS_EXPORT_DATA(t)
#else
#define XPC_PUBLIC_API(t)    JS_IMPORT_API(t)
#define XPC_PUBLIC_DATA(t)   JS_IMPORT_DATA(t)
#endif

#define XPC_FRIEND_API(t)    XPC_PUBLIC_API(t)
#define XPC_FRIEND_DATA(t)   XPC_PUBLIC_DATA(t)

// XXX break these up into separate files?
// XXX declare them in XPIDL :)

/***************************************************************************/
// forward declarations...
class nsIXPCScriptable;
class nsIInterfaceInfo;
class nsIXPConnectWrappedNative;
class nsIXPConnectFinalizeListener;
class nsIXPCSecurityManager;

// {1B2DDB00-EEE8-11d2-BAA4-00805F8A5DD7}
#define NS_IXPCONNECT_FINALIZE_LISTENER_IID   \
{ 0x1b2ddb00, 0xeee8, 0x11d2, \
  { 0xba, 0xa4, 0x0, 0x80, 0x5f, 0x8a, 0x5d, 0xd7 } }
class nsIXPConnectFinalizeListener : public nsISupports
{
public:
    NS_DEFINE_STATIC_IID_ACCESSOR(NS_IXPCONNECT_FINALIZE_LISTENER_IID)

    NS_IMETHOD AboutToRelease(nsISupports* aObj) = 0;
};

// {215DBE02-94A7-11d2-BA58-00805F8A5DD7}
#define NS_IXPCONNECT_WRAPPED_NATIVE_IID   \
{ 0x215dbe02, 0x94a7, 0x11d2,               \
  { 0xba, 0x58, 0x0, 0x80, 0x5f, 0x8a, 0x5d, 0xd7 } }

class nsIXPConnectWrappedNative : public nsISupports
{
public:
    NS_DEFINE_STATIC_IID_ACCESSOR(NS_IXPCONNECT_WRAPPED_NATIVE_IID)

    NS_IMETHOD GetDynamicScriptable(nsIXPCScriptable** p) = 0;
    NS_IMETHOD GetArbitraryScriptable(nsIXPCScriptable** p) = 0;
    NS_IMETHOD GetJSObject(JSObject** aJSObj) = 0;
    NS_IMETHOD GetNative(nsISupports** aObj) = 0;
    NS_IMETHOD GetInterfaceInfo(nsIInterfaceInfo** info) = 0;
    NS_IMETHOD GetIID(nsIID** iid) = 0; // returns IAllocatator alloc'd copy
    NS_IMETHOD DebugDump(int depth) = 0;
    NS_IMETHOD SetFinalizeListener(nsIXPConnectFinalizeListener* aListener) = 0;

    // NOT YET IMPLEMTED!!!
    NS_IMETHOD GetJSObjectPrototype(JSObject** aJSObj) = 0;

    // XXX other methods?
};

/***************************************************************************/
// {215DBE03-94A7-11d2-BA58-00805F8A5DD7}
#define NS_IXPCONNECT_WRAPPED_JS_IID   \
{ 0x215dbe03, 0x94a7, 0x11d2,           \
  { 0xba, 0x58, 0x0, 0x80, 0x5f, 0x8a, 0x5d, 0xd7 } }

// wrappers around JSObject are passed around as plain nsISupports pointers.
// To manipulate such a wrapper (as opposed to manipulating the wrapped
// JSObject via the wrapper) do a QueryInterface for the
// nsIXPConnectWrappedJSMethods interface 
// i.e. 'nsIXPConnectWrappedJSMethods::GetIID()'
// and use the methods on that interface. (see below)

/******************************************/

// {BED52030-BCA6-11d2-BA79-00805F8A5DD7}
#define NS_IXPCONNECT_WRAPPED_JS_METHODS_IID   \
{ 0xbed52030, 0xbca6, 0x11d2, \
  { 0xba, 0x79, 0x0, 0x80, 0x5f, 0x8a, 0x5d, 0xd7 } }

class nsIXPConnectWrappedJSMethods : public nsISupports
{
public:
    NS_DEFINE_STATIC_IID_ACCESSOR(NS_IXPCONNECT_WRAPPED_JS_METHODS_IID)

    NS_IMETHOD GetJSObject(JSObject** aJSObj) = 0;
    NS_IMETHOD GetInterfaceInfo(nsIInterfaceInfo** info) = 0;
    NS_IMETHOD GetIID(nsIID** iid) = 0; // returns IAllocatator alloc'd copy
    NS_IMETHOD DebugDump(int depth) = 0;
    // XXX other methods?
};

/***************************************************************************/
// {EFAE37B0-946D-11d2-BA58-00805F8A5DD7}
#define NS_IXPCONNECT_IID    \
{ 0xefae37b0, 0x946d, 0x11d2, \
  {0xba, 0x58, 0x0, 0x80, 0x5f, 0x8a, 0x5d, 0xd7} }

// For use with the service manager
// {CB6593E0-F9B2-11d2-BDD6-000064657374}
#define NS_XPCONNECT_CID \
{ 0xcb6593e0, 0xf9b2, 0x11d2, { 0xbd, 0xd6, 0x0, 0x0, 0x64, 0x65, 0x73, 0x74 } }

class nsIXPConnect : public nsISupports
{
public:
    NS_DEFINE_STATIC_IID_ACCESSOR(NS_IXPCONNECT_IID)
    NS_DEFINE_STATIC_CID_ACCESSOR(NS_XPCONNECT_CID)

    NS_IMETHOD InitJSContext(JSContext* aJSContext,
                             JSObject* aGlobalJSObj,
                             JSBool AddComponentsObject) = 0;

    NS_IMETHOD InitJSContextWithNewWrappedGlobal(JSContext* aJSContext,
                          nsISupports* aCOMObj,
                          REFNSIID aIID,
                          JSBool AddComponentsObject,
                          nsIXPConnectWrappedNative** aWrapper) = 0;

    // Creates a new Components object, builds a wrapper for it, and adds
    // it as the 'Components' property of the JSObject passed in.
    NS_IMETHOD AddNewComponentsObject(JSContext* aJSContext,
                                      JSObject* aGlobalJSObj) = 0;

    // Creates a new Components object (with no wrapper)
    NS_IMETHOD CreateComponentsObject(nsIXPCComponents** aComponentsObj) = 0;

    NS_IMETHOD WrapNative(JSContext* aJSContext,
                          nsISupports* aCOMObj,
                          REFNSIID aIID,
                          nsIXPConnectWrappedNative** aWrapper) = 0;

    NS_IMETHOD WrapJS(JSContext* aJSContext,
                      JSObject* aJSObj,
                      REFNSIID aIID,
                      nsISupports** aWrapper) = 0;

    NS_IMETHOD GetWrappedNativeOfJSObject(JSContext* aJSContext,
                                    JSObject* aJSObj,
                                    nsIXPConnectWrappedNative** aWrapper) = 0;

    NS_IMETHOD DebugDump(int depth) = 0;
    NS_IMETHOD DebugDumpObject(nsISupports* p, int depth) = 0;

    NS_IMETHOD AbandonJSContext(JSContext* aJSContext) = 0;

    NS_IMETHOD SetSecurityManagerForJSContext(JSContext* aJSContext,
                                    nsIXPCSecurityManager* aManager,
                                    PRUint16 flags) = 0;

    NS_IMETHOD GetSecurityManagerForJSContext(JSContext* aJSContext,
                                    nsIXPCSecurityManager** aManager,
                                    PRUint16* flags) = 0;

    NS_IMETHOD GetCurrentJSStack(nsIJSStackFrameLocation** aStack) = 0;

    NS_IMETHOD CreateStackFrameLocation(JSBool isJSFrame,
                                        const char* aFilename,
                                        const char* aFunctionName,
                                        PRInt32 aLineNumber,
                                        nsIJSStackFrameLocation* aCaller,
                                        nsIJSStackFrameLocation** aStack) = 0;

    NS_IMETHOD GetPendingException(nsIXPCException** aException) = 0;
    /* pass nsnull to clear pending exception */
    NS_IMETHOD SetPendingException(nsIXPCException* aException) = 0;

    // XXX other methods?
};

#endif /* nsIXPConnect_h___ */
