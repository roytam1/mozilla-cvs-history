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

/* Data conversion between native and JavaScript types. */

#include "xpcprivate.h"

/*
* This is a table driven scheme to determine if the types of the params of the
* given method exclude that method from being reflected via XPConnect.
*
* The table can be appended and modified as requirements change. However...
*
* The table ASSUMES that all the type idenetifiers are contiguous starting
* at ZERO. And, it also ASSUMES that the additional criteria of whether or
* not a give type is reflectable are its use as a pointer and/or 'out' type.
*
* The table has a row for each type and columns for the combinations of
* that type being used as a pointer type and/or as an 'out' param.
*/

#define XPC_MK_BIT(p,o) (1 << (((p)?1:0)+((o)?2:0)))
#define XPC_IS_REFLECTABLE(f, p, o) ((f) & XPC_MK_BIT((p),(o)))
#define XPC_MK_FLAG(np_no,p_no,np_o,p_o) \
        ((uint8)((np_no) | ((p_no) << 1) | ((np_o) << 2) | ((p_o) << 3)))

/***********************************************************/
#define XPC_FLAG_COUNT nsXPTType::T_PWSTRING_SIZE_IS+1

/* '1' means 'reflectable'. '0' means 'not reflectable'.        */
static uint8 xpc_reflectable_flags[XPC_FLAG_COUNT] = {
    /* 'p' stands for 'pointer' and 'o' stands for 'out'        */
    /*          !p&!o, p&!o, !p&o, p&o                          */
    XPC_MK_FLAG(  1  ,  1  ,   1 ,  0 ), /* T_I8                */
    XPC_MK_FLAG(  1  ,  1  ,   1 ,  0 ), /* T_I16               */
    XPC_MK_FLAG(  1  ,  1  ,   1 ,  0 ), /* T_I32               */
    XPC_MK_FLAG(  1  ,  1  ,   1 ,  0 ), /* T_I64               */
    XPC_MK_FLAG(  1  ,  1  ,   1 ,  0 ), /* T_U8                */
    XPC_MK_FLAG(  1  ,  1  ,   1 ,  0 ), /* T_U16               */
    XPC_MK_FLAG(  1  ,  1  ,   1 ,  0 ), /* T_U32               */
    XPC_MK_FLAG(  1  ,  1  ,   1 ,  0 ), /* T_U64               */
    XPC_MK_FLAG(  1  ,  1  ,   1 ,  0 ), /* T_FLOAT             */
    XPC_MK_FLAG(  1  ,  1  ,   1 ,  0 ), /* T_DOUBLE            */
    XPC_MK_FLAG(  1  ,  1  ,   1 ,  0 ), /* T_BOOL              */
    XPC_MK_FLAG(  1  ,  1  ,   1 ,  0 ), /* T_CHAR              */
    XPC_MK_FLAG(  1  ,  1  ,   1 ,  0 ), /* T_WCHAR             */
    XPC_MK_FLAG(  0  ,  0  ,   0 ,  0 ), /* T_VOID              */
    XPC_MK_FLAG(  0  ,  1  ,   0 ,  1 ), /* T_IID               */
    XPC_MK_FLAG(  0  ,  0  ,   0 ,  0 ), /* T_BSTR              */
    XPC_MK_FLAG(  0  ,  1  ,   0 ,  1 ), /* T_CHAR_STR          */
    XPC_MK_FLAG(  0  ,  1  ,   0 ,  1 ), /* T_WCHAR_STR         */
    XPC_MK_FLAG(  0  ,  1  ,   0 ,  1 ), /* T_INTERFACE         */
    XPC_MK_FLAG(  0  ,  1  ,   0 ,  1 ), /* T_INTERFACE_IS      */
    XPC_MK_FLAG(  0  ,  1  ,   0 ,  1 ), /* T_ARRAY             */
    XPC_MK_FLAG(  0  ,  0  ,   0 ,  0 ), /* T_PSTRING_SIZE_IS   */ //XXX disabled
    XPC_MK_FLAG(  0  ,  0  ,   0 ,  0 )  /* T_PWSTRING_SIZE_IS  */ //XXX disabled
    };
/***********************************************************/

// static
JSBool
XPCConvert::IsMethodReflectable(const nsXPTMethodInfo& info)
{
    if(info.IsNotXPCOM() || info.IsHidden())
        return JS_FALSE;

    for(int i = info.GetParamCount()-1; i >= 0; i--)
    {
        const nsXPTParamInfo& param = info.GetParam(i);
        const nsXPTType& type = param.GetType();

        uint8 base_type = type.TagPart();
        NS_ASSERTION(base_type < XPC_FLAG_COUNT, "BAD TYPE");

        if(!XPC_IS_REFLECTABLE(xpc_reflectable_flags[base_type],
                               type.IsPointer(), param.IsOut()))
            return JS_FALSE;
    }
    return JS_TRUE;
}

/***************************************************************************/

static JSBool
ObjectHasPrivate(JSContext* cx, JSObject* obj)
{
    JSClass* jsclass =
#ifdef JS_THREADSAFE
            JS_GetClass(cx, obj);
#else
            JS_GetClass(obj);
#endif
    NS_ASSERTION(jsclass, "obj has no class");
    return jsclass && (jsclass->flags & JSCLASS_HAS_PRIVATE);
}

static JSBool
GetISupportsFromJSObject(JSContext* cx, JSObject* obj, nsISupports** iface)
{
    JSClass* jsclass =
#ifdef JS_THREADSAFE
            JS_GetClass(cx, obj);
#else
            JS_GetClass(obj);
#endif
    NS_ASSERTION(jsclass, "obj has no class");
    if(jsclass &&
       (jsclass->flags & JSCLASS_HAS_PRIVATE) &&
       (jsclass->flags & JSCLASS_PRIVATE_IS_NSISUPPORTS))
    {
        *iface = (nsISupports*) JS_GetPrivate(cx, obj);
        return JS_TRUE;
    }
    return JS_FALSE;
}

/***************************************************************************/

/*
* Support for 64 bit conversions were 'long long' not supported.
* (from John Fairhurst <mjf35@cam.ac.uk>)
*/

#ifdef HAVE_LONG_LONG

#define JAM_DOUBLE(cx,v,d) (d=JS_NewDouble(cx,(jsdouble)v),DOUBLE_TO_JSVAL(d))
// Win32 can't handle uint64 to double conversion
#define JAM_DOUBLE_U64(cx,v,d) JAM_DOUBLE(cx,((int64)v),d)

#else

inline jsval
JAM_DOUBLE(JSContext *cx, const int64 &v, jsdouble *dbl)
{
   jsdouble d;
   LL_L2D(d, v);
   dbl = JS_NewDouble(cx, d);
   return DOUBLE_TO_JSVAL(dbl);
}

inline jsval
JAM_DOUBLE(JSContext *cx, double v, jsdouble *dbl)
{
   dbl = JS_NewDouble(cx, (jsdouble)v);
   return DOUBLE_TO_JSVAL(dbl);
}

// if !HAVE_LONG_LONG, then uint64 is a typedef of int64
#define JAM_DOUBLE_U64(cx,v,d) JAM_DOUBLE(cx,v,d)

#endif

#define FIT_32(cx,i,d) (INT_FITS_IN_JSVAL(i)?INT_TO_JSVAL(i):JAM_DOUBLE(cx,i,d))

// static
JSBool
XPCConvert::NativeData2JS(JSContext* cx, jsval* d, const void* s,
                          const nsXPTType& type, const nsID* iid,
                          nsresult* pErr)
{
    NS_PRECONDITION(s, "bad param");
    NS_PRECONDITION(d, "bad param");

    jsdouble* dbl;

    if(pErr)
        *pErr = NS_ERROR_XPC_BAD_CONVERT_NATIVE;

    switch(type.TagPart())
    {
    case nsXPTType::T_I8    : *d = INT_TO_JSVAL((int32)*((int8*)s));     break;
    case nsXPTType::T_I16   : *d = INT_TO_JSVAL((int32)*((int16*)s));    break;
    case nsXPTType::T_I32   : *d = FIT_32(cx,*((int32*)s),dbl);          break;
    case nsXPTType::T_I64   : *d = JAM_DOUBLE(cx,*((int64*)s),dbl);      break;
    case nsXPTType::T_U8    : *d = INT_TO_JSVAL((int32)*((uint8*)s));    break;
    case nsXPTType::T_U16   : *d = INT_TO_JSVAL((int32)*((uint16*)s));   break;
    case nsXPTType::T_U32   : *d = FIT_32(cx,*((uint32*)s),dbl);         break;
    case nsXPTType::T_U64   : *d = JAM_DOUBLE_U64(cx,*((uint64*)s),dbl); break;
    case nsXPTType::T_FLOAT : *d = JAM_DOUBLE(cx,*((float*)s),dbl);      break;
    case nsXPTType::T_DOUBLE: *d = JAM_DOUBLE(cx,*((double*)s),dbl);     break;
    case nsXPTType::T_BOOL  : *d = *((PRBool*)s)?JSVAL_TRUE:JSVAL_FALSE; break;
    case nsXPTType::T_CHAR  :
        {
            char* p = (char*)s;
            if(!p)
                return JS_FALSE;
            JSString* str;
            if(!(str = JS_NewStringCopyN(cx, p, 1)))
                return JS_FALSE;
            *d = STRING_TO_JSVAL(str);
            break;
        }
    case nsXPTType::T_WCHAR :
        {
            jschar* p = (jschar*)s;
            if(!p)
                return JS_FALSE;
            JSString* str;
            if(!(str = JS_NewUCStringCopyN(cx, p, 1)))
                return JS_FALSE;
            *d = STRING_TO_JSVAL(str);
            break;
        }
    default:
        if(!type.IsPointer())
        {
            XPC_LOG_ERROR(("XPCConvert::NativeData2JS : unsupported type"));
            return JS_FALSE;
        }

        // set the default result
        *d = JSVAL_NULL;

        switch(type.TagPart())
        {
        case nsXPTType::T_VOID:
            // XXX implement void* ?
            XPC_LOG_ERROR(("XPCConvert::NativeData2JS : void* params not supported"));
            return JS_FALSE;

        case nsXPTType::T_IID:
            {
                nsID* iid = *((nsID**)s);
                if(!iid)
                    break;
                JSObject* obj;
                if(!(obj = xpc_NewIDObject(cx, *iid)))
                    return JS_FALSE;
                *d = OBJECT_TO_JSVAL(obj);
                break;
            }

        case nsXPTType::T_BSTR:
            // XXX implement BSTR ?
            XPC_LOG_ERROR(("XPCConvert::NativeData2JS : BSTR params not supported"));
            return JS_FALSE;

        case nsXPTType::T_CHAR_STR:
            {
                char* p = *((char**)s);
                if(!p)
                    break;
                JSString* str;
                if(!(str = JS_NewStringCopyZ(cx, p)))
                    return JS_FALSE;
                *d = STRING_TO_JSVAL(str);
                break;
            }

        case nsXPTType::T_WCHAR_STR:
            {
                jschar* p = *((jschar**)s);
                if(!p)
                    break;
                JSString* str;
                if(!(str = JS_NewUCStringCopyZ(cx, p)))
                    return JS_FALSE;
                *d = STRING_TO_JSVAL(str);
                break;
            }

        case nsXPTType::T_INTERFACE:
        case nsXPTType::T_INTERFACE_IS:
            {
                nsISupports* iface = *((nsISupports**)s);
                if(!iface)
                    break;
                nsIScriptObjectOwner* owner;
                JSObject* aJSObj = nsnull;
                // is this a wrapped JS object?
                if(nsXPCWrappedJSClass::IsWrappedJS(iface))
                {
                    nsIXPConnectWrappedJSMethods* methods;
                    if(NS_SUCCEEDED(iface->QueryInterface(
                                nsIXPConnectWrappedJSMethods::GetIID(),
                                (void**)&methods)) &&
                       NS_SUCCEEDED(methods->GetJSObject(&aJSObj)))
                    {
                        NS_RELEASE(methods);
                        *d = OBJECT_TO_JSVAL(aJSObj);
                    }
                }
                // is this a DOM wrapped native object?
                else if(NS_SUCCEEDED(iface->QueryInterface(
                            nsIScriptObjectOwner::GetIID(), (void**)&owner)))
                {

                    nsresult rv;
                    JSBool success = JS_FALSE;
                    JSObject* globalObject;
                    nsISupports* domObject;
                    NS_ASSERTION(owner,"QI succeeded but yielded NULL!");
                    if(nsnull != (globalObject =
                                    JS_GetGlobalObject(cx)) &&
                       ObjectHasPrivate(cx, globalObject) &&
                       nsnull != (domObject = (nsISupports*)
                                    JS_GetPrivate(cx, globalObject)))
                    {
                        nsIScriptGlobalObject* scriptObject;
                        if(NS_SUCCEEDED(domObject->QueryInterface(
                                            nsIScriptGlobalObject::GetIID(),
                                            (void**)&scriptObject)))
                        {
                            NS_ASSERTION(scriptObject,"QI succeeded but yielded NULL!");
                            nsIScriptContext* scriptContext = nsnull;
                            scriptObject->GetContext(&scriptContext);
                            if(scriptContext)
                            {
                                rv = owner->GetScriptObject(scriptContext,
                                                    (void **)&aJSObj);
                                NS_RELEASE(scriptContext);
                                if (NS_SUCCEEDED(rv))
                                    success = JS_TRUE;
                            }
                            NS_RELEASE(scriptObject);
                        }
                    }
                    NS_RELEASE(owner);
                    if(!success)
                        return JS_FALSE;
                }
                else
                {
                    // we need to build a wrapper
                    nsXPCWrappedNative* wrapper = nsnull;
                    XPCContext* xpcc;
                    if(!iid ||
                       !(xpcc = nsXPConnect::GetContext(cx)) ||
                       !(wrapper =
                            nsXPCWrappedNative::GetNewOrUsedWrapper(xpcc,
                                                             iface, *iid)))
                    {
                        return JS_FALSE;
                    }
                    aJSObj = wrapper->GetJSObject();
                    NS_RELEASE(wrapper);
                }
                if(aJSObj)
                    *d = OBJECT_TO_JSVAL(aJSObj);
                break;
            }
        default:
            NS_ASSERTION(0, "bad type");
            return JS_FALSE;
        }
    }
    return JS_TRUE;
}

/***************************************************************************/
// static
JSBool
XPCConvert::JSData2Native(JSContext* cx, void* d, jsval s,
                          const nsXPTType& type,
                          JSBool useAllocator, const nsID* iid,
                          nsresult* pErr)
{
    NS_PRECONDITION(d, "bad param");

    int32    ti;
    uint32   tu;
    jsdouble td;

    if(pErr)
        *pErr = NS_ERROR_XPC_BAD_CONVERT_JS;

    switch(type.TagPart())
    {
    case nsXPTType::T_I8     :
        if(!JS_ValueToECMAInt32(cx, s, &ti))
            return JS_FALSE;
        *((int8*)d)  = (int8) ti;
        break;
    case nsXPTType::T_I16    :
        if(!JS_ValueToECMAInt32(cx, s, &ti))
            return JS_FALSE;
        *((int16*)d)  = (int16) ti;
        break;
    case nsXPTType::T_I32    :
        if(!JS_ValueToECMAInt32(cx, s, (int32*)d))
            return JS_FALSE;
        break;
    case nsXPTType::T_I64    :
        if(JSVAL_IS_INT(s))
        {
            if(!JS_ValueToECMAInt32(cx, s, &ti))
                return JS_FALSE;
            LL_I2L(*((int64*)d),ti);

        }
        else
        {
            if(!JS_ValueToNumber(cx, s, &td))
                return JS_FALSE;
            LL_D2L(*((int64*)d),td);
        }
        break;
    case nsXPTType::T_U8     :
        if(!JS_ValueToECMAUint32(cx, s, &tu))
            return JS_FALSE;
        *((uint8*)d)  = (uint8) tu;
        break;
    case nsXPTType::T_U16    :
        if(!JS_ValueToECMAUint32(cx, s, &tu))
            return JS_FALSE;
        *((uint16*)d)  = (uint16) tu;
        break;
    case nsXPTType::T_U32    :
        if(!JS_ValueToECMAUint32(cx, s, (uint32*)d))
            return JS_FALSE;
        break;
    case nsXPTType::T_U64    :
        if(JSVAL_IS_INT(s))
        {
            if(!JS_ValueToECMAUint32(cx, s, &tu))
                return JS_FALSE;
            LL_UI2L(*((int64*)d),tu);
        }
        else
        {
            if(!JS_ValueToNumber(cx, s, &td))
                return JS_FALSE;
#ifdef XP_WIN
            // XXX Win32 can't handle double to uint64 directly
            *((uint64*)d) = (uint64)((int64) td);
#else
            LL_D2L(*((uint64*)d),td);
#endif
        }
        break;
    case nsXPTType::T_FLOAT  :
        if(!JS_ValueToNumber(cx, s, &td))
            return JS_FALSE;
        *((float*)d) = (float) td;
        break;
    case nsXPTType::T_DOUBLE :
        if(!JS_ValueToNumber(cx, s, (double*)d))
            return JS_FALSE;
        break;
    case nsXPTType::T_BOOL   :
        if(!JS_ValueToBoolean(cx, s, (JSBool*)d))
            return JS_FALSE;
        break;
    case nsXPTType::T_CHAR   :
        {
            char* bytes=nsnull;
            JSString* str;

            if(!(str = JS_ValueToString(cx, s))||
               !(bytes = JS_GetStringBytes(str)))
            {
                return JS_FALSE;
            }
            *((char*)d) = bytes[0];
            break;
        }
    case nsXPTType::T_WCHAR  :
        {
            jschar* chars=nsnull;
            JSString* str;
            if(!(str = JS_ValueToString(cx, s))||
               !(chars = JS_GetStringChars(str)))
            {
                return JS_FALSE;
            }
            *((uint16*)d)  = (uint16) chars[0];
            break;
        }
    default:
        if(!type.IsPointer())
        {
            NS_ASSERTION(0,"unsupported type");
            return JS_FALSE;
        }

        switch(type.TagPart())
        {
        case nsXPTType::T_VOID:
            // XXX implement void* ?
            XPC_LOG_ERROR(("XPCConvert::JSData2Native : void* params not supported"));
            NS_ASSERTION(0,"void* params not supported");
            return JS_FALSE;
        case nsXPTType::T_IID:
        {
            NS_ASSERTION(useAllocator,"trying to convert a JSID to nsID without allocator : this would leak");

            JSObject* obj;
            const nsID* pid=nsnull;

            if(JSVAL_IS_VOID(s) || JSVAL_IS_NULL(s))
            {
                if(type.IsReference())
                {
                    if(pErr)
                        *pErr = NS_ERROR_XPC_BAD_CONVERT_JS_NULL_REF;
                    return JS_FALSE;
                }
                // else ...
                *((const nsID**)d) = nsnull;
                return JS_TRUE;
            }

            if(!JSVAL_IS_OBJECT(s) ||
               (!(obj = JSVAL_TO_OBJECT(s))) ||
               (!(pid = xpc_JSObjectToID(cx, obj))))
            {
                return JS_FALSE;
            }
            *((const nsID**)d) = pid;
            return JS_TRUE;
        }

        case nsXPTType::T_BSTR:
            // XXX implement BSTR
            XPC_LOG_ERROR(("XPCConvert::JSData2Native : BSTR params not supported"));
            return JS_FALSE;

        case nsXPTType::T_CHAR_STR:
        {
            char* bytes=nsnull;
            JSString* str;

            if(JSVAL_IS_VOID(s) || JSVAL_IS_NULL(s))
            {
                if(type.IsReference())
                {
                    if(pErr)
                        *pErr = NS_ERROR_XPC_BAD_CONVERT_JS_NULL_REF;
                    return JS_FALSE;
                }
                // else ...
                *((char**)d) = nsnull;
                return JS_TRUE;
            }

            if(!(str = JS_ValueToString(cx, s))||
               !(bytes = JS_GetStringBytes(str)))
            {
                return JS_FALSE;
            }
            if(useAllocator)
            {
                int len = (JS_GetStringLength(str) + 1) * sizeof(char);
                if(!(*((void**)d) = nsAllocator::Alloc(len)))
                {
                    return JS_FALSE;
                }
                memcpy(*((void**)d), bytes, len);
            }
            else
                *((char**)d) = bytes;

            return JS_TRUE;
        }

        case nsXPTType::T_WCHAR_STR:
        {
            jschar* chars=nsnull;
            JSString* str;

            if(JSVAL_IS_VOID(s) || JSVAL_IS_NULL(s))
            {
                if(type.IsReference())
                {
                    if(pErr)
                        *pErr = NS_ERROR_XPC_BAD_CONVERT_JS_NULL_REF;
                    return JS_FALSE;
                }
                // else ...
                *((jschar**)d) = nsnull;
                return JS_TRUE;
            }

            if(!(str = JS_ValueToString(cx, s))||
               !(chars = JS_GetStringChars(str)))
            {
                return JS_FALSE;
            }
            if(useAllocator)
            {
                int byte_len = (JS_GetStringLength(str)+1)*sizeof(jschar);
                if(!(*((void**)d) = nsAllocator::Alloc(byte_len)))
                {
                    // XXX should report error
                    return JS_FALSE;
                }
                memcpy(*((void**)d), chars, byte_len);
            }
            else
                *((jschar**)d) = chars;

            return JS_TRUE;
        }

        case nsXPTType::T_INTERFACE:
        case nsXPTType::T_INTERFACE_IS:
        {
            NS_ASSERTION(iid,"can't do interface conversions without iid");
            JSObject* obj;
            nsISupports* iface = nsnull;

            if(JSVAL_IS_VOID(s) || JSVAL_IS_NULL(s))
            {
                if(type.IsReference())
                {
                    if(pErr)
                        *pErr = NS_ERROR_XPC_BAD_CONVERT_JS_NULL_REF;
                    return JS_FALSE;
                }
                // else ...
                *((nsISupports**)d) = nsnull;
                return JS_TRUE;
            }

            // only wrap JSObjects
            if(!JSVAL_IS_OBJECT(s) || !(obj = JSVAL_TO_OBJECT(s)))
            {
                return JS_FALSE;
            }

            // is this really a native xpcom object with a wrapper?
            nsXPCWrappedNative* wrapper;
            if(nsnull != (wrapper =
               nsXPCWrappedNativeClass::GetWrappedNativeOfJSObject(cx,obj)))
            {
                iface = wrapper->GetNative();
                // is the underlying object the right interface?
                if(wrapper->GetIID().Equals(*iid))
                    NS_ADDREF(iface);
                else
                    iface->QueryInterface(*iid, (void**)&iface);
            }
            else if(GetISupportsFromJSObject(cx, obj, &iface))
            {
                if(iface)
                    iface->QueryInterface(*iid, (void**)&iface);
                else
                {
                    *((nsISupports**)d) = nsnull;
                    return JS_TRUE;
                }
            }
            else
            {
                // lets try to build a wrapper around the JSObject
                XPCContext* xpcc;
                if(nsnull != (xpcc = nsXPConnect::GetContext(cx)))
                    iface = nsXPCWrappedJS::GetNewOrUsedWrapper(xpcc, obj, *iid);
            }
            if(iface)
            {
                // one AddRef has already been done
                *((nsISupports**)d) = iface;
                return JS_TRUE;
            }
            return JS_FALSE;
        }
        default:
            NS_ASSERTION(0, "bad type");
            return JS_FALSE;
        }
    }
    return JS_TRUE;
}

/***************************************************************************/

static nsIXPCException*
ConstructException(nsresult rv, const char* message,
                   const char* ifaceName, const char* methodName,
                   nsISupports* data)
{
    nsIXPCException* e;
    const char format[] = "%s [%s::%s]";
    const char * msg = message;
    char* sz = nsnull;

    if(!msg)
        if(!nsXPCException::NameAndFormatForNSResult(rv, nsnull, &msg) || ! msg)
            msg = "<error>";

    if(ifaceName && methodName)
        sz = JS_smprintf(format, msg, ifaceName, methodName);
    else
        sz = (char*) msg; // I promise to play nice after casting away const

    e = nsXPCException::NewException(sz, rv, nsnull, data, 0);

    if(sz && sz != msg)
        JS_smprintf_free(sz);
    return e;
}

/********************************/

// static
nsIXPCException*
XPCConvert::JSValToXPCException(JSContext* cx,
                                jsval s,
                                const char* ifaceName,
                                const char* methodName)
{
    if(!JSVAL_IS_PRIMITIVE(s))
    {
        // we have a JSObject
        JSObject* obj = JSVAL_TO_OBJECT(s);

        if(!obj)
        {
            NS_ASSERTION(0, "when is an object not an object?");
            return nsnull;
        }

        // is this really a native xpcom object with a wrapper?
        nsXPCWrappedNative* wrapper;
        if(nsnull != (wrapper =
           nsXPCWrappedNativeClass::GetWrappedNativeOfJSObject(cx,obj)))
        {
            nsIXPCException* iface;
            nsISupports* supports = wrapper->GetNative();
            if(NS_SUCCEEDED(supports->QueryInterface(
                                NS_GET_IID(nsIXPCException), (void**)&iface)))
            {
                // just pass through the exception (with extra ref and all)
                return iface;
            }
            else
            {
                // it is a wrapped native, but not an exception!
                return ConstructException(NS_ERROR_XPC_JS_THREW_NATIVE_OBJECT,
                                          nsnull, ifaceName, methodName, supports);
            }
        }
        else
        {
            // It is a JSObject, but not a wrapped native.

            // XXX should be able to extract info from 'regular' JS Exceptions

            uintN ignored;
            JSBool found;

            // heuristic to see if it might be usable as an xpcexception
            if(JS_GetPropertyAttributes(cx, obj, "message", &ignored, &found) &&
               found &&
               JS_GetPropertyAttributes(cx, obj, "result", &ignored, &found) &&
               found)
            {
                // lets try to build a wrapper around the JSObject
                XPCContext* xpcc;
                if(nsnull != (xpcc = nsXPConnect::GetContext(cx)))
                {
                    return (nsIXPCException*)
                        nsXPCWrappedJS::GetNewOrUsedWrapper(xpcc, obj,
                                                NS_GET_IID(nsIXPCException));
                }
            }

            // otherwise we'll just try to convert it to a string

            JSString* str = JS_ValueToString(cx, s);
            if(!str)
                return nsnull;

            return ConstructException(NS_ERROR_XPC_JS_THREW_JS_OBJECT,
                                      JS_GetStringBytes(str),
                                      ifaceName, methodName, nsnull);
        }
    }

    if(JSVAL_IS_VOID(s) || JSVAL_IS_NULL(s))
    {
        return ConstructException(NS_ERROR_XPC_JS_THREW_NULL,
                                  nsnull, ifaceName, methodName, nsnull);
    }

    if(JSVAL_IS_NUMBER(s))
    {
        // lets see if it looks like an nsresult
        nsresult rv;
        double number;
        JSBool isResult = JS_FALSE;

        if(JSVAL_IS_INT(s))
        {
            rv = (nsresult) JSVAL_TO_INT(s);
            if(NS_FAILED(rv))
                isResult = JS_TRUE;
            else
                number = (double) JSVAL_TO_INT(s);
        }
        else
        {
            if(JSVAL_TO_DOUBLE(s))
            {
                number = *(JSVAL_TO_DOUBLE(s));
                if(number > 0.0 &&
                   number < (double)0xffffffff &&
                   0.0 == fmod(number,1))
                {
                    rv = (nsresult) number;
                    if(NS_FAILED(rv))
                        isResult = JS_TRUE;
                }
            }
        }

        if(isResult)
            return ConstructException(rv, nsnull, ifaceName, methodName, nsnull);
        else
        {
            nsISupportsDouble* data;
            nsIComponentManager* cm;
            if(NS_FAILED(NS_GetGlobalComponentManager(&cm)) || !cm ||
               NS_FAILED(cm->CreateInstanceByProgID(
                                NS_SUPPORTS_DOUBLE_PROGID,
                                nsnull,
                                NS_GET_IID(nsISupportsDouble),
                                (void**)&data)))
                return nsnull;
            data->SetData(number);
            nsIXPCException* e =
                        ConstructException(NS_ERROR_XPC_JS_THREW_NUMBER,
                                           nsnull,
                                           ifaceName, methodName, data);
            NS_RELEASE(data);
            return e;
        }
    }

    // otherwise we'll just try to convert it to a string

    JSString* str = JS_ValueToString(cx, s);
    if(str)
        return ConstructException(NS_ERROR_XPC_JS_THREW_STRING,
                                  JS_GetStringBytes(str),
                                  ifaceName, methodName, nsnull);
    return nsnull;
}

/********************************/

// static
nsIXPCException*
XPCConvert::JSErrorToXPCException(JSContext* cx,
                                  const char* message,
                                  const JSErrorReport* report)
{
    nsIXPCException* result;
    xpcJSErrorReport* data;
    if(report)
    {
        static const char defaultMsg[] = "JavaScript Error";
        data = xpcJSErrorReport::NewReport(message ? message : defaultMsg,
                                           report);
    }
    else
        data = nsnull;

    if(data)
    {
        char* formattedMsg;
        if(NS_FAILED(data->ToString(&formattedMsg)))
            formattedMsg = nsnull;

        result = ConstructException(NS_ERROR_XPC_JAVASCRIPT_ERROR_WITH_DETAILS,
                                  formattedMsg, nsnull, nsnull, data);

        if(formattedMsg)
            nsAllocator::Free(formattedMsg);
        NS_RELEASE(data);
    }
    else
    {
        result = ConstructException(NS_ERROR_XPC_JAVASCRIPT_ERROR,
                                    nsnull, nsnull, nsnull, nsnull);
    }
    return result;
}

/***************************************************************************/

const char XPC_ARG_FORMATTER_FORMAT_STR[] = "%ip";

JSBool JS_DLL_CALLBACK
XPC_JSArgumentFormatter(JSContext *cx, const char *format,
                        JSBool fromJS, jsval **vpp, va_list *app)
{
    jsval *vp;
    va_list ap;

    vp = *vpp;
    ap = *app;

    nsXPTType type = nsXPTType((uint8)(TD_INTERFACE_TYPE | XPT_TDP_POINTER));
    nsISupports* iface;

    if(fromJS)
    {
        if(!XPCConvert::JSData2Native(cx, &iface, vp[0], type, JS_FALSE,
                                      &(NS_GET_IID(nsISupports)), nsnull))
            return JS_FALSE;
        *va_arg(ap, nsISupports **) = iface;
    }
    else
    {
        const nsIID* iid;

        iid  = va_arg(ap, const nsIID*);
        iface = va_arg(ap, nsISupports *);

        if(!XPCConvert::NativeData2JS(cx, &vp[0], &iface, type, iid, nsnull))
            return JS_FALSE;
    }
    *vpp = vp + 1;
    *app = ap;
    return JS_TRUE;
}

/***************************************************************************/

// array fun...

#ifdef POPULATE
#undef POPULATE
#endif

// static
JSBool
XPCConvert::NativeArray2JS(JSContext* cx,
                           jsval* d, const void** s,
                           const nsXPTType& type, const nsID* iid,
                           JSUint32 count,
                           nsresult* pErr)
{
    NS_PRECONDITION(s, "bad param");
    NS_PRECONDITION(d, "bad param");

    // XXX add support for putting chars in a string rather than an array

    // XXX add support to indicate *which* array element was not convertable

    JSObject *array = JS_NewArrayObject(cx, count, nsnull);

    if(!array)
        return JS_FALSE;

    // root this early
    *d = OBJECT_TO_JSVAL(array);

    if(pErr)
        *pErr = NS_ERROR_XPC_BAD_CONVERT_NATIVE;

    JSUint32 i;
    jsval current;

#define POPULATE(_t)                                                         \
    PR_BEGIN_MACRO                                                           \
        for(i = 0; i < count; i++)                                           \
        {                                                                    \
            if(!NativeData2JS(cx, &current, ((_t*)*s)+i, type, iid, pErr) || \
               !JS_SetElement(cx, array, i, &current))                       \
                goto failure;                                                \
        }                                                                    \
    PR_END_MACRO

    // XXX check IsPtr - esp. to handle array of nsID (as opposed to nsID*)

    switch(type.TagPart())
    {
    case nsXPTType::T_I8            : POPULATE(int8);           break;
    case nsXPTType::T_I16           : POPULATE(int16);          break;
    case nsXPTType::T_I32           : POPULATE(int32);          break;
    case nsXPTType::T_I64           : POPULATE(int64);          break;
    case nsXPTType::T_U8            : POPULATE(uint8);          break;
    case nsXPTType::T_U16           : POPULATE(uint16);         break;
    case nsXPTType::T_U32           : POPULATE(uint32);         break;
    case nsXPTType::T_U64           : POPULATE(uint64);         break;
    case nsXPTType::T_FLOAT         : POPULATE(float);          break;
    case nsXPTType::T_DOUBLE        : POPULATE(double);         break;
    case nsXPTType::T_BOOL          : POPULATE(PRBool);         break;
    case nsXPTType::T_CHAR          : POPULATE(char);           break;
    case nsXPTType::T_WCHAR         : POPULATE(jschar);         break;
    case nsXPTType::T_VOID          : NS_ASSERTION(0,"bad type"); goto failure;
    case nsXPTType::T_IID           : POPULATE(nsID*);          break;
    case nsXPTType::T_BSTR          : NS_ASSERTION(0,"bad type"); goto failure;
    case nsXPTType::T_CHAR_STR      : POPULATE(char*);          break;
    case nsXPTType::T_WCHAR_STR     : POPULATE(jschar*);        break;
    case nsXPTType::T_INTERFACE     : POPULATE(nsISupports*);   break;
    case nsXPTType::T_INTERFACE_IS  : POPULATE(nsISupports*);   break;
    default                         : NS_ASSERTION(0,"bad type"); goto failure;
    }

    if(pErr)
        *pErr = NS_OK;
    return JS_TRUE;

failure:
    return JS_FALSE;

#undef POPULATE
}

// static
JSBool
XPCConvert::JSArray2Native(JSContext* cx, void** d, jsval s,
                           JSUint32 count, JSUint32 capacity,
                           const nsXPTType& type,
                           JSBool useAllocator, const nsID* iid,
                           uintN* pErr)
{
    NS_PRECONDITION(d, "bad param");

    enum CleanupMode {na, fr, re};
    CleanupMode cleanupMode;
    void* array = nsnull;
    JSUint32 initedCount;
    jsval current;

    // XXX add support for getting chars from strings

    // XXX add support to indicate *which* array element was not convertable

    if(JSVAL_IS_VOID(s) || JSVAL_IS_NULL(s))
    {
        *d = nsnull;
        return JS_TRUE;
    }

    if(!JSVAL_IS_OBJECT(s))
    {
        if(pErr)
            *pErr = NS_ERROR_XPC_CANT_CONVERT_PRIMITIVE_TO_ARRAY;
        return JS_FALSE;
    }

    JSObject* jsarray = JSVAL_TO_OBJECT(s);
    if(!JS_IsArrayObject(cx, jsarray))
    {
        if(pErr)
            *pErr = NS_ERROR_XPC_CANT_CONVERT_OBJECT_TO_ARRAY;
        return JS_FALSE;
    }

    jsuint len;
    if(!JS_GetArrayLength(cx, jsarray, &len) || len < count || capacity < count)
    {
        if(pErr)
            *pErr = NS_ERROR_XPC_NOT_ENOUGH_ELEMENTS_IN_ARRAY;
        return JS_FALSE;
    }

    if(pErr)
        *pErr = NS_ERROR_XPC_BAD_CONVERT_JS;

#define POPULATE(_mode, _t)                                                  \
    PR_BEGIN_MACRO                                                           \
        cleanupMode = _mode;                                                 \
        if(nsnull == (array = nsAllocator::Alloc(capacity * sizeof(_t))))    \
        {                                                                    \
            if(pErr)                                                         \
                *pErr = NS_ERROR_OUT_OF_MEMORY;                              \
            goto failure;                                                    \
        }                                                                    \
        for(initedCount = 0; initedCount < count; initedCount++)             \
        {                                                                    \
            if(!JS_GetElement(cx, jsarray, initedCount, &current) ||         \
               !JSData2Native(cx, ((_t*)array)+initedCount, current, type,   \
                              useAllocator, iid, pErr))                      \
                goto failure;                                                \
        }                                                                    \
    PR_END_MACRO


    // XXX check IsPtr - esp. to handle array of nsID (as opposed to nsID*)

    // XXX make extra space at end of char* and wchar* and null termintate

    switch(type.TagPart())
    {
    case nsXPTType::T_I8            : POPULATE(na, int8);           break;
    case nsXPTType::T_I16           : POPULATE(na, int16);          break;
    case nsXPTType::T_I32           : POPULATE(na, int32);          break;
    case nsXPTType::T_I64           : POPULATE(na, int64);          break;
    case nsXPTType::T_U8            : POPULATE(na, uint8);          break;
    case nsXPTType::T_U16           : POPULATE(na, uint16);         break;
    case nsXPTType::T_U32           : POPULATE(na, uint32);         break;
    case nsXPTType::T_U64           : POPULATE(na, uint64);         break;
    case nsXPTType::T_FLOAT         : POPULATE(na, float);          break;
    case nsXPTType::T_DOUBLE        : POPULATE(na, double);         break;
    case nsXPTType::T_BOOL          : POPULATE(na, PRBool);         break;
    case nsXPTType::T_CHAR          : POPULATE(na, char);           break;
    case nsXPTType::T_WCHAR         : POPULATE(na, jschar);         break;
    case nsXPTType::T_VOID          : NS_ASSERTION(0,"bad type"); goto failure;
    case nsXPTType::T_IID           : POPULATE(fr, nsID*);          break;
    case nsXPTType::T_BSTR          : NS_ASSERTION(0,"bad type"); goto failure;
    case nsXPTType::T_CHAR_STR      : POPULATE(fr, char*);          break;
    case nsXPTType::T_WCHAR_STR     : POPULATE(fr, jschar*);        break;
    case nsXPTType::T_INTERFACE     : POPULATE(re, nsISupports*);   break;
    case nsXPTType::T_INTERFACE_IS  : POPULATE(re, nsISupports*);   break;
    default                         : NS_ASSERTION(0,"bad type"); goto failure;
    }

    *d = array;
    if(pErr)
        *pErr = NS_OK;
    return JS_TRUE;

failure:
    // we may need to cleanup the partially filled array of converted stuff
    if(array)
    {
        if(cleanupMode == re)
        {
            nsISupports** a = (nsISupports**) array;
            for(PRUint32 i = 0; i < initedCount; i++)
            {
                nsISupports* p = a[i];
                NS_IF_RELEASE(p);
            }
        }
        else if(cleanupMode == fr && useAllocator)
        {
            void** a = (void**) array;
            for(PRUint32 i = 0; i < initedCount; i++)
            {
                void* p = a[i];
                if(p) nsAllocator::Free(p);
            }
        }
        nsAllocator::Free(array);
    }

    return JS_FALSE;

#undef POPULATE
}

