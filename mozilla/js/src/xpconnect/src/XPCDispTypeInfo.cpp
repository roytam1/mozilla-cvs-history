/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is the IDispatch implementation for XPConnect
 *
 * The Initial Developer of the Original Code is
 * David Bradley.
 * Portions created by the Initial Developer are Copyright (C) 2002
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

/**
 * \file XPCDispTypeInfo.cpp implementation XPCDispTypeInfo
 * This file contains the implementations for XPCDispTypeInfo class and
 * associated helper classes and functions
 */

#include "xpcprivate.h"

inline
void FillOutElemDesc(VARTYPE vt, PRUint16 paramFlags, ELEMDESC & elemDesc)
{
    elemDesc.tdesc.vt = vt;
    elemDesc.paramdesc.wParamFlags = paramFlags;
    elemDesc.paramdesc.pparamdescex = 0;
    elemDesc.tdesc.lptdesc = 0;
    elemDesc.tdesc.lpadesc = 0;
    elemDesc.tdesc.hreftype = 0;
}

void XPCDispJSPropertyInfo::GetReturnType(XPCCallContext& ccx, ELEMDESC & elemDesc)
{
    VARTYPE vt;
    if(IsSetterMode())     // we're working on a property
    {
        vt = VT_EMPTY;
    }
    else if(IsProperty())
    {
        vt = XPCDispConvert::JSTypeToCOMType(ccx, mProperty);
    }
    else // Function
    {
        vt = VT_VARIANT;
    }
    FillOutElemDesc(vt, PARAMFLAG_FRETVAL, elemDesc);
}

ELEMDESC* XPCDispJSPropertyInfo::GetParamInfo()
{
    PRUint32 paramCount = GetParamCount();
    ELEMDESC* elemDesc;
    if(paramCount != 0)
    {
        elemDesc = new ELEMDESC[paramCount];
        for(PRUint32 index = 0; index < paramCount; ++index)
        {
            FillOutElemDesc(VT_VARIANT, PARAMFLAG_FIN, elemDesc[index]);
        }
    }
    else
        elemDesc = 0;
    return elemDesc;
}

XPCDispJSPropertyInfo::XPCDispJSPropertyInfo(JSContext* cx, PRUint32 memid, 
                                     JSObject* obj, jsval val) : 
    mPropertyType(INVALID), mMemID(memid)
{
    JSString* str = JS_ValueToString(cx, val);
    if(str)
    {
        const char* chars = JS_GetStringBytes(str);
        if(chars)
        {
            mName = chars;
            JSBool found;
            uintN attr;
            // Get the property, and if it's found and enumerable 
            if(JS_GetPropertyAttributes(cx, obj, chars, &attr, &found) && 
                found && (attr & JSPROP_ENUMERATE) != 0)
            {
                if(chars && JS_GetProperty(cx, obj, chars, &mProperty) && 
                    !JSVAL_IS_VOID(mProperty))
                {
                    if(JSVAL_IS_OBJECT(mProperty) && 
                        JS_ObjectIsFunction(cx, JSVAL_TO_OBJECT(mProperty)))
                    {
                        mPropertyType = FUNCTION;
                        JSObject * funcObj = JSVAL_TO_OBJECT(mProperty);
                        JSIdArray * funcObjArray = JS_Enumerate(cx, funcObj);
                        if(funcObjArray)
                        {
                            mParamCount = funcObjArray->length;
                        }
                    }
                    else
                    {
                        mParamCount = 0;
                        if((attr & JSPROP_READONLY) != 0)
                        {
                            mPropertyType = READONLY_PROPERTY;
                        }
                        else
                        {
                            mPropertyType = PROPERTY;
                        }
                    }
                }
            }
        }
    }
}

void XPCDispTypeInfo::FuncDescArray::BuildFuncDesc(XPCCallContext& ccx, JSObject* obj, 
                                     XPCDispJSPropertyInfo & propInfo)
{
    FUNCDESC* funcDesc = new FUNCDESC;
    mArray.AppendElement(funcDesc);
    // zero is reserved
    funcDesc->memid = propInfo.GetMemID();
    funcDesc->lprgscode = 0;
    funcDesc->funckind = FUNC_DISPATCH;
    funcDesc->invkind = propInfo.GetInvokeKind();
    funcDesc->callconv = CC_STDCALL;
    funcDesc->cParams = propInfo.GetParamCount();
    funcDesc->lprgelemdescParam = propInfo.GetParamInfo();
    funcDesc->cParamsOpt = 0;
    // This could be a problem, supposed to be used for functions of
    // type FUNC_VIRTUAL which we aren't, so zero should be ok
    funcDesc->oVft = 0;
    funcDesc->cScodes = 0;
    funcDesc->wFuncFlags = 0;
    propInfo.GetReturnType(ccx, funcDesc->elemdescFunc);
}

XPCDispTypeInfo::FuncDescArray::FuncDescArray(XPCCallContext& ccx, JSObject* obj, const XPCDispIDArray& array, XPCDispNameArray & names)
{
    PRUint32 size = array.Length();
    names.SetSize(size);
    PRUint32 memid = 0;
    JSContext* cx = ccx;
    for(PRUint32 index = 0; index < size; ++index)
    {
        XPCDispJSPropertyInfo propInfo(cx, ++memid, obj, array.Item(cx, index));
        names.SetName(index + 1, propInfo.GetName());
        BuildFuncDesc(ccx, obj, propInfo);
        if(propInfo.IsProperty() && !propInfo.IsReadOnly())
        {
            propInfo.SetSetterMode();
            BuildFuncDesc(ccx, obj, propInfo);
        }
    }
}

XPCDispTypeInfo::FuncDescArray::~FuncDescArray()
{
    PRUint32 size = mArray.Count();
    for(PRUint32 index = 0; index < size; ++index)
    {
        delete NS_REINTERPRET_CAST(FUNCDESC*,mArray.ElementAt(index));
    }
}

XPCDispTypeInfo::XPCDispTypeInfo(XPCCallContext& ccx, JSObject* obj, 
                               XPCDispIDArray* array) :
    mRefCnt(0), mJSObject(obj), mIDArray(ccx, array), 
    mFuncDescArray(ccx, obj, *array, mNameArray)
{
}

XPCDispTypeInfo::~XPCDispTypeInfo()
{
}

XPCDispTypeInfo * XPCDispTypeInfo::New(XPCCallContext& ccx, JSObject* obj)
{
    nsresult retval = NS_ERROR_FAILURE;
    // Saved state must be restored, all exits through 'out'...
    JSExceptionState* saved_exception = xpc_DoPreScriptEvaluated(ccx);
    JSErrorReporter older = JS_SetErrorReporter(ccx, nsnull);
    XPCDispTypeInfo * pTypeInfo = 0;
    JSIdArray * jsArray = JS_Enumerate(ccx, obj);
    if(jsArray)
    {
        XPCDispIDArray* array = new XPCDispIDArray(ccx, jsArray);
        if(array)
        {
            pTypeInfo = new XPCDispTypeInfo(ccx, obj, array);
        }
    }
    JS_SetErrorReporter(ccx, older);
    xpc_DoPostScriptEvaluated(ccx, saved_exception);

    return pTypeInfo;
}

NS_COM_MAP_BEGIN(XPCDispTypeInfo)
NS_COM_MAP_ENTRY(ITypeInfo)
NS_COM_MAP_ENTRY(IUnknown)
NS_COM_MAP_END

NS_IMPL_THREADSAFE_ADDREF(XPCDispTypeInfo)
NS_IMPL_THREADSAFE_RELEASE(XPCDispTypeInfo)


STDMETHODIMP XPCDispTypeInfo::GetTypeAttr( 
    /* [out] */ TYPEATTR __RPC_FAR *__RPC_FAR *ppTypeAttr)
{
    return E_NOTIMPL;
}
    
STDMETHODIMP XPCDispTypeInfo::GetTypeComp(
    /* [out] */ ITypeComp __RPC_FAR *__RPC_FAR *ppTComp)
{
    return E_NOTIMPL;
}

STDMETHODIMP XPCDispTypeInfo::GetFuncDesc(
        /* [in] */ UINT index,
        /* [out] */ FUNCDESC __RPC_FAR *__RPC_FAR *ppFuncDesc)
{
    return E_NOTIMPL;
}
    
STDMETHODIMP XPCDispTypeInfo::GetVarDesc(
        /* [in] */ UINT index,
        /* [out] */ VARDESC __RPC_FAR *__RPC_FAR *ppVarDesc)
{
    return E_NOTIMPL;
}
    
STDMETHODIMP XPCDispTypeInfo::GetNames(
        /* [in] */ MEMBERID memid,
        /* [length_is][size_is][out] */ BSTR __RPC_FAR *rgBstrNames,
        /* [in] */ UINT cMaxNames,
        /* [out] */ UINT __RPC_FAR *pcNames)
{
    return E_NOTIMPL;
}
    
STDMETHODIMP XPCDispTypeInfo::GetRefTypeOfImplType(
        /* [in] */ UINT index,
        /* [out] */ HREFTYPE __RPC_FAR *pRefType)
{
    return E_NOTIMPL;
}
    
STDMETHODIMP XPCDispTypeInfo::GetImplTypeFlags(
        /* [in] */ UINT index,
        /* [out] */ INT __RPC_FAR *pImplTypeFlags)
{
    return E_NOTIMPL;
}

STDMETHODIMP XPCDispTypeInfo::GetIDsOfNames(
        /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
        /* [in] */ UINT cNames,
        /* [size_is][out] */ MEMBERID __RPC_FAR *pMemId)
{
    _bstr_t name;
    for(UINT index = 0; index < cNames; ++index)
    {
        name = rgszNames[index];
        nsDependentCString buffer(NS_STATIC_CAST(const char *,name));
        pMemId[index] = mNameArray.Find(buffer);

    }
    return S_OK;
}

// TODO: Factor out with nsXPCWrappedJS::Invoke
STDMETHODIMP XPCDispTypeInfo::Invoke(
        /* [in] */ PVOID pvInstance,
        /* [in] */ MEMBERID memid,
        /* [in] */ WORD wFlags,
        /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
        /* [out] */ VARIANT __RPC_FAR *pVarResult,
        /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
        /* [out] */ UINT __RPC_FAR *puArgErr)
{
    IUnknown* ptr = NS_REINTERPRET_CAST(IUnknown*,pvInstance);
    IDispatch* pDisp;
    HRESULT result = ptr->QueryInterface(IID_IDispatch, 
                                         NS_REINTERPRET_CAST(void**,&pDisp));
    if(SUCCEEDED(result))
    {
        result = pDisp->Invoke(memid, IID_NULL, LOCALE_SYSTEM_DEFAULT, wFlags, 
                               pDispParams, pVarResult, pExcepInfo, puArgErr);
    }
    return result;
}
    
STDMETHODIMP XPCDispTypeInfo::GetDocumentation(
        /* [in] */ MEMBERID memid,
        /* [out] */ BSTR __RPC_FAR *pBstrName,
        /* [out] */ BSTR __RPC_FAR *pBstrDocString,
        /* [out] */ DWORD __RPC_FAR *pdwHelpContext,
        /* [out] */ BSTR __RPC_FAR *pBstrHelpFile)
{
    PRUint32 index = memid;
    if(index < mIDArray->Length())
    {
        XPCCallContext ccx(NATIVE_CALLER);
        JSString* str = JS_ValueToString(ccx, mIDArray->Item(ccx, index));
        if(str)
        {
            jschar* chars = JS_GetStringChars(str);
            CComBSTR name(chars);
            *pBstrName = name.Detach();
            pBstrDocString = 0;
            pdwHelpContext = 0;
            pBstrHelpFile = 0;
        }
    }
    return E_NOTIMPL;
}
    
STDMETHODIMP XPCDispTypeInfo::GetDllEntry(
        /* [in] */ MEMBERID memid,
        /* [in] */ INVOKEKIND invKind,
        /* [out] */ BSTR __RPC_FAR *pBstrDllName,
        /* [out] */ BSTR __RPC_FAR *pBstrName,
        /* [out] */ WORD __RPC_FAR *pwOrdinal)
{
    // We are not supporting this till a need arrises
    return E_NOTIMPL;
}
    
STDMETHODIMP XPCDispTypeInfo::GetRefTypeInfo(
        /* [in] */ HREFTYPE hRefType,
        /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo)
{
    return E_NOTIMPL;
}
    
STDMETHODIMP XPCDispTypeInfo::AddressOfMember(
        /* [in] */ MEMBERID memid,
        /* [in] */ INVOKEKIND invKind,
        /* [out] */ PVOID __RPC_FAR *ppv)
{
    // We are not supporting this till a need arrises
    return E_NOTIMPL;
}
    
STDMETHODIMP XPCDispTypeInfo::CreateInstance(
        /* [in] */ IUnknown __RPC_FAR *pUnkOuter,
        /* [in] */ REFIID riid,
        /* [iid_is][out] */ PVOID __RPC_FAR *ppvObj)
{
    // We are not supporting this till a need arrises
    return E_NOTIMPL;
}
    
STDMETHODIMP XPCDispTypeInfo::GetMops(
        /* [in] */ MEMBERID memid,
        /* [out] */ BSTR __RPC_FAR *pBstrMops)
{
    return E_NOTIMPL;
}
    
STDMETHODIMP XPCDispTypeInfo::GetContainingTypeLib(
        /* [out] */ ITypeLib __RPC_FAR *__RPC_FAR *ppTLib,
        /* [out] */ UINT __RPC_FAR *pIndex)
{
    // We are not supporting this till a need arrises
    return E_NOTIMPL;
}
    
void STDMETHODCALLTYPE XPCDispTypeInfo::ReleaseTypeAttr( 
    /* [in] */ TYPEATTR __RPC_FAR *pTypeAttr)
{
    // Nothing for us to do
}

void STDMETHODCALLTYPE XPCDispTypeInfo::ReleaseFuncDesc( 
    /* [in] */ FUNCDESC __RPC_FAR *pFuncDesc)
{
    // Nothing for us to do
}

void STDMETHODCALLTYPE XPCDispTypeInfo::ReleaseVarDesc( 
    /* [in] */ VARDESC __RPC_FAR *pVarDesc)
{
    // Nothing for us to do
}

XPCDispIDArray::XPCDispIDArray(XPCCallContext& ccx, JSIdArray* array) : 
    mMarked(JS_FALSE), mIDArray(array->length)
{
    for(jsint index = 0; index < array->length; ++index)
    {
        mIDArray.ReplaceElementAt(NS_REINTERPRET_CAST(void*,
                                                      array->vector[index]), 
                                  index);
    }   
}

void XPCDispIDArray::Mark()
{
    if(!IsMarked())
    {
        mMarked = JS_TRUE;
        XPCCallContext ccx(NATIVE_CALLER);
        if(ccx.IsValid())
        {
            PRInt32 count = Length();
            jsval val;
            JSContext* cx = ccx;
            for(PRInt32 index = 0; index < count; ++index)
            {
                if(JS_IdToValue(cx,
                                NS_REINTERPRET_CAST(jsid,
                                                    mIDArray.ElementAt(index)),
                                &val) &&
                    JSVAL_IS_GCTHING(val))
                {
                    JS_MarkGCThing(cx, NS_REINTERPRET_CAST(void*,val),
                                   nsnull, nsnull);
                }
            }
        }
    }
}

