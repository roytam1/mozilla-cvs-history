/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

#include "nsString.h"
#include "nsReadableUtils.h"
#include "nsSOAPParameter.h"
#include "nsSOAPJSValue.h"
#include "nsSOAPUtils.h"
#include "nsIXPConnect.h"
#include "nsIServiceManager.h"
#include "nsISupportsPrimitives.h"

nsSOAPParameter::nsSOAPParameter()
{
  NS_INIT_ISUPPORTS();
}

nsSOAPParameter::~nsSOAPParameter()
{
}

NS_IMPL_ISUPPORTS4(nsSOAPParameter, 
                   nsISOAPParameter, 
                   nsISecurityCheckedComponent,
                   nsIXPCScriptable,
                   nsIJSNativeInitializer)

/* attribute AString name; */
NS_IMETHODIMP nsSOAPParameter::GetName(nsAWritableString & aName)
{
  NS_ENSURE_ARG_POINTER(&aName);
  aName.Assign(mName);
  return NS_OK;
}
NS_IMETHODIMP nsSOAPParameter::SetName(const nsAReadableString & aName)
{
  mName.Assign(aName);
  return NS_OK;
}

/* attribute AString encodingStyleURI; */
NS_IMETHODIMP nsSOAPParameter::GetEncodingStyleURI(nsAWritableString & aEncodingStyleURI)
{
  NS_ENSURE_ARG_POINTER(&aEncodingStyleURI);
  aEncodingStyleURI.Assign(mEncodingStyleURI);
  return NS_OK;
}
NS_IMETHODIMP nsSOAPParameter::SetEncodingStyleURI(const nsAReadableString & aEncodingStyleURI)
{
  mEncodingStyleURI.Assign(aEncodingStyleURI);
  return NS_OK;
}

/* attribute AString type; */
NS_IMETHODIMP nsSOAPParameter::GetType(nsAWritableString & aType)
{
  NS_ENSURE_ARG_POINTER(&aType);
  aType.Assign(mType);
  return NS_OK;
}
NS_IMETHODIMP nsSOAPParameter::SetType(const nsAReadableString & aType)
{
  mType.Assign(aType);
  return NS_OK;
}

/* attribute AString schemaType; */
NS_IMETHODIMP nsSOAPParameter::GetSchemaType(nsAWritableString & aSchemaType)
{
  NS_ENSURE_ARG_POINTER(&aSchemaType);
  aSchemaType.Assign(mSchemaType);
  return NS_OK;
}
NS_IMETHODIMP nsSOAPParameter::SetSchemaType(const nsAReadableString & aSchemaType)
{
  mType.Assign(aSchemaType);
  return NS_OK;
}

/* [noscript] readonly attribute nsISupports value; */
NS_IMETHODIMP nsSOAPParameter::GetValue(nsISupports * *aValue)
{
  NS_ENSURE_ARG_POINTER(aValue);
  *aValue = mValue;
  NS_IF_ADDREF(*aValue);
  return NS_OK;
}
/* void setValue (in nsISupports aValue); */
NS_IMETHODIMP nsSOAPParameter::SetValue(nsISupports *aValue)
{
  mValue = aValue;
  return NS_OK;
}

/* attribute boolean header; */
NS_IMETHODIMP nsSOAPParameter::GetHeader(PRBool *aHeader)
{
  NS_ENSURE_ARG_POINTER(aHeader);
  *aHeader = mHeader;
  return NS_OK;
}
NS_IMETHODIMP nsSOAPParameter::SetHeader(PRBool aHeader)
{
  mHeader = aHeader;
  return NS_OK;
}

/* void setAsString (in AString aValue); */
NS_IMETHODIMP nsSOAPParameter::SetAsString(const nsAReadableString & aValue)
{
    mType.Assign(nsSOAPUtils::kStringType);
    nsCOMPtr<nsISupportsWString> value = do_CreateInstance(NS_SUPPORTS_WSTRING_CONTRACTID);
    value->SetData(ToNewUnicode(aValue));
    mValue = value;
    return NS_OK;
}

/* void setAsBoolean (in PRBool aValue); */
NS_IMETHODIMP nsSOAPParameter::SetAsBoolean(PRBool aValue)
{
    mType.Assign(nsSOAPUtils::kPRBoolType);
    nsCOMPtr<nsISupportsPRBool> value = do_CreateInstance(NS_SUPPORTS_PRBOOL_CONTRACTID);
    value->SetData(aValue);
    mValue = value;
    return NS_OK;
}

/* void setAsDouble (in double aValue); */
NS_IMETHODIMP nsSOAPParameter::SetAsDouble(double aValue)
{
    mType.Assign(nsSOAPUtils::kDoubleType);
    nsCOMPtr<nsISupportsDouble> value = do_CreateInstance(NS_SUPPORTS_DOUBLE_CONTRACTID);
    value->SetData(aValue);
    mValue = value;
    return NS_OK;
}

/* void setAsFloat (in float aValue); */
NS_IMETHODIMP nsSOAPParameter::SetAsFloat(float aValue)
{
    mType.Assign(nsSOAPUtils::kFloatType);
    nsCOMPtr<nsISupportsFloat> value = do_CreateInstance(NS_SUPPORTS_FLOAT_CONTRACTID);
    value->SetData(aValue);
    mValue = value;
    return NS_OK;
}

/* void setAsLong (in PRInt64 aValue); */
NS_IMETHODIMP nsSOAPParameter::SetAsLong(PRInt64 aValue)
{
    mType.Assign(nsSOAPUtils::kPRInt64Type);
    nsCOMPtr<nsISupportsPRInt64> value = do_CreateInstance(NS_SUPPORTS_PRINT64_CONTRACTID);
    value->SetData(aValue);
    mValue = value;
    return NS_OK;
}

/* void setAsInt (in PRInt32 aValue); */
NS_IMETHODIMP nsSOAPParameter::SetAsInt(PRInt32 aValue)
{
    mType.Assign(nsSOAPUtils::kPRInt32Type);
    nsCOMPtr<nsISupportsPRInt32> value = do_CreateInstance(NS_SUPPORTS_PRINT32_CONTRACTID);
    value->SetData(aValue);
    mValue = value;
    return NS_OK;
}

/* void setAsShort (in PRInt16 aValue); */
NS_IMETHODIMP nsSOAPParameter::SetAsShort(PRInt16 aValue)
{
    mType.Assign(nsSOAPUtils::kPRInt16Type);
    nsCOMPtr<nsISupportsPRInt16> value = do_CreateInstance(NS_SUPPORTS_PRINT16_CONTRACTID);
    value->SetData(aValue);
    mValue = value;
    return NS_OK;
}

/* void setAsByte (in PRUint8 aValue); */
NS_IMETHODIMP nsSOAPParameter::SetAsByte(PRUint8 aValue)
{
    mType.Assign(nsSOAPUtils::kCharType);
    nsCOMPtr<nsISupportsChar> value = do_CreateInstance(NS_SUPPORTS_CHAR_CONTRACTID);
    value->SetData(aValue);
    mValue = value;
    return NS_OK;
}

/* void setAsArray (in nsISupportsArray aValue); */
NS_IMETHODIMP nsSOAPParameter::SetAsArray(nsISupportsArray *aValue)
{
    mType.Assign(nsSOAPUtils::kArrayType);
    mValue = NS_REINTERPRET_CAST(nsISupports*, aValue);
    return NS_OK;
}

/* void setAsArray (in AString structType, in nsISOAPStruct aValue); */
NS_IMETHODIMP nsSOAPParameter::SetAsStruct(const nsAReadableString & aStructType, nsISOAPStruct *aValue)
{
    mType.Assign(nsSOAPUtils::kStructTypePrefix);
    mType.Append(aStructType);
    mValue = NS_REINTERPRET_CAST(nsISupports*, aValue);
    return NS_OK;
}

/* void setAsLiteral (in nsIDOMNode aValue); */
NS_IMETHODIMP nsSOAPParameter::SetAsLiteral(nsIDOMNode *aValue)
{
    mType.Assign(nsSOAPUtils::kLiteralType);
    mValue = NS_REINTERPRET_CAST(nsISupports*, aValue);
    return NS_OK;
}

NS_IMETHODIMP 
nsSOAPParameter::Initialize(JSContext *cx, JSObject *obj, 
                            PRUint32 argc, jsval *argv)
{
  if (argc > 0) {
    JSString* namestr = JS_ValueToString(cx, argv[0]);
    if (namestr) {
      SetName(nsString(NS_REINTERPRET_CAST(PRUnichar*, JS_GetStringChars(namestr))));
    }
    if (argc > 1) {
      nsCOMPtr<nsISupports> value;
      nsAutoString type;
      nsresult rc = nsSOAPJSValue::ConvertJSArgsToValue(cx,
                                        argc - 1,
                                        argv + 1,
                                        PR_FALSE,
                                        getter_AddRefs(value),
                                        type);
      mType = type;
      mValue = value;
      return rc;
    }
  }
  return NS_OK;
}

// The nsIXPCScriptable map declaration that will generate stubs for us...
#define XPC_MAP_CLASSNAME           nsSOAPParameter
#define XPC_MAP_QUOTED_CLASSNAME   "SOAPParameter"
#define                             XPC_MAP_WANT_SETPROPERTY
#define                             XPC_MAP_WANT_GETPROPERTY
#define XPC_MAP_FLAGS       nsIXPCScriptable::USE_JSSTUB_FOR_ADDPROPERTY   | \
                            nsIXPCScriptable::USE_JSSTUB_FOR_DELPROPERTY   | \
                            nsIXPCScriptable::USE_JSSTUB_FOR_SETPROPERTY
#include "xpc_map_end.h" /* This will #undef the above */

NS_IMETHODIMP 
nsSOAPParameter::GetProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                             JSObject *obj, jsval id, jsval *vp,
                             PRBool *_retval)
{
  if (JSVAL_IS_STRING(id)) {
    JSString* str = JSVAL_TO_STRING(id);
    const PRUnichar* name = NS_REINTERPRET_CAST(const PRUnichar *,
                                                JS_GetStringChars(str));
    if (NS_LITERAL_STRING("value").Equals(name)) {
      return nsSOAPJSValue::ConvertValueToJSVal(cx,
                                        mValue,
                                        mType,
                                        vp);
    }
  }
  return NS_OK;
}

NS_IMETHODIMP 
nsSOAPParameter::SetProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                             JSObject *obj, jsval id, jsval *vp,
                             PRBool *_retval)
{
  if (JSVAL_IS_STRING(id)) {
    JSString* str = JSVAL_TO_STRING(id);
    const PRUnichar* name = NS_REINTERPRET_CAST(const PRUnichar *,
                                                JS_GetStringChars(str));
    if (NS_LITERAL_STRING("value").Equals(name)) {
      return nsSOAPJSValue::ConvertJSValToValue(cx,
                                        *vp,
                                        getter_AddRefs(mValue),
                                        mType);
    }
  }
  return NS_OK;
}


static const char* kAllAccess = "AllAccess";

/* string canCreateWrapper (in nsIIDPtr iid); */
NS_IMETHODIMP 
nsSOAPParameter::CanCreateWrapper(const nsIID * iid, char **_retval)
{
  if (iid->Equals(NS_GET_IID(nsISOAPParameter))) {
    *_retval = nsCRT::strdup(kAllAccess);
  }

  return NS_OK;
}

/* string canCallMethod (in nsIIDPtr iid, in wstring methodName); */
NS_IMETHODIMP 
nsSOAPParameter::CanCallMethod(const nsIID * iid, const PRUnichar *methodName, char **_retval)
{
  if (iid->Equals(NS_GET_IID(nsISOAPParameter))) {
    *_retval = nsCRT::strdup(kAllAccess);
  }

  return NS_OK;
}

/* string canGetProperty (in nsIIDPtr iid, in wstring propertyName); */
NS_IMETHODIMP 
nsSOAPParameter::CanGetProperty(const nsIID * iid, const PRUnichar *propertyName, char **_retval)
{
  if (iid->Equals(NS_GET_IID(nsISOAPParameter))) {
    *_retval = nsCRT::strdup(kAllAccess);
  }

  return NS_OK;
}

/* string canSetProperty (in nsIIDPtr iid, in wstring propertyName); */
NS_IMETHODIMP 
nsSOAPParameter::CanSetProperty(const nsIID * iid, const PRUnichar *propertyName, char **_retval)
{
  if (iid->Equals(NS_GET_IID(nsISOAPParameter))) {
    *_retval = nsCRT::strdup(kAllAccess);
  }

  return NS_OK;
}
