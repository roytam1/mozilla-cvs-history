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

#include "nsIServiceManager.h"
#include "nsIComponentManager.h"
#include "nsIXPConnect.h"
#include "nsSOAPUtils.h"
#include "nsSOAPMessage.h"
#include "nsISOAPParameter.h"
#include "nsSOAPTypeRegistry.h"
#include "nsISOAPMarshaller.h"
#include "nsISOAPUnmarshaller.h"
#include "nsSOAPJSValue.h"

/////////////////////////////////////////////
//
//
/////////////////////////////////////////////
  
nsSOAPMessage::nsSOAPMessage(): mTypes(new nsSOAPTypeRegistry())
{
  NS_INIT_ISUPPORTS();
  mStatus = 0;
}

nsSOAPMessage::~nsSOAPMessage()
{
}

NS_IMPL_ISUPPORTS3(nsSOAPMessage, 
                   nsISOAPMessage, 
                   nsISecurityCheckedComponent,
                   nsIXPCScriptable)

/* attribute nsIDOMDocument message; */
NS_IMETHODIMP nsSOAPMessage::GetMessage(nsIDOMDocument * *aMessage)
{
  NS_ENSURE_ARG_POINTER(aMessage);
  *aMessage = mMessage;
  NS_IF_ADDREF(*aMessage);
  return NS_OK;
}
NS_IMETHODIMP nsSOAPMessage::SetMessage(nsIDOMDocument * aMessage)
{
  mMessage = aMessage;
  return NS_OK;
}

/* readonly attribute nsIDOMElement envelope; */
NS_IMETHODIMP nsSOAPMessage::GetEnvelope(nsIDOMElement * *aEnvelope)
{
  NS_ENSURE_ARG_POINTER(aEnvelope);

  if (mMessage) {
    nsCOMPtr<nsIDOMElement> root;
    mMessage->GetDocumentElement(getter_AddRefs(root));
    if (root) {
      nsAutoString name, namespaceURI;
      root->GetLocalName(name);
      root->GetNamespaceURI(namespaceURI);
      if (name.Equals(nsSOAPUtils::kEnvelopeTagName)
        && namespaceURI.Equals(nsSOAPUtils::kSOAPEnvURI))
      {
        *aEnvelope = root;
        NS_ADDREF(*aEnvelope);
        return NS_OK;
      }
    } 

  }
  *aEnvelope = nsnull;
  return NS_OK;
}

/* readonly attribute nsIDOMElement header; */
NS_IMETHODIMP nsSOAPMessage::GetHeader(nsIDOMElement * *aHeader)
{
  NS_ENSURE_ARG_POINTER(aHeader);
  nsCOMPtr<nsIDOMElement> env;
  GetEnvelope(getter_AddRefs(env));
  if (env) {
    nsSOAPUtils::GetSpecificChildElement(env, 
      nsSOAPUtils::kSOAPEnvURI, nsSOAPUtils::kHeaderTagName, 
      aHeader);
  }
  else {
    *aHeader = nsnull;
  }
  return NS_OK;
}

/* readonly attribute nsIDOMElement body; */
NS_IMETHODIMP nsSOAPMessage::GetBody(nsIDOMElement * *aBody)
{
  NS_ENSURE_ARG_POINTER(aBody);
  nsCOMPtr<nsIDOMElement> env;
  GetEnvelope(getter_AddRefs(env));
  if (env) {
    nsSOAPUtils::GetSpecificChildElement(env, 
      nsSOAPUtils::kSOAPEnvURI, nsSOAPUtils::kBodyTagName, 
      aBody);
  }
  else {
    *aBody = nsnull;
  }
  return NS_OK;
}

/* attribute DOMString actionURI; */
NS_IMETHODIMP nsSOAPMessage::GetActionURI(nsAWritableString & aActionURI)
{
  NS_ENSURE_ARG_POINTER(&aActionURI);
  aActionURI.Assign(mActionURI);
  return NS_OK;
}
NS_IMETHODIMP nsSOAPMessage::SetActionURI(const nsAReadableString & aActionURI)
{
  mActionURI.Assign(aActionURI);
  return NS_OK;
}

/* attribute DOMString encodingStyleURI; */
NS_IMETHODIMP nsSOAPMessage::GetEncodingStyleURI(nsAWritableString & aEncodingStyleURI)
{
  NS_ENSURE_ARG_POINTER(&aEncodingStyleURI);
  aEncodingStyleURI.Assign(mEncodingStyleURI);
  return NS_OK;
}
NS_IMETHODIMP nsSOAPMessage::SetEncodingStyleURI(const nsAReadableString & aEncodingStyleURI)
{
  mEncodingStyleURI.Assign(aEncodingStyleURI);
  return NS_OK;
}

/* attribute DOMString methodName; */
NS_IMETHODIMP nsSOAPMessage::GetMethodName(nsAWritableString & aMethodName)
{
  NS_ENSURE_ARG_POINTER(&aMethodName);
  aMethodName.Assign(mMethodName);
  return NS_OK;
}
NS_IMETHODIMP nsSOAPMessage::SetMethodName(const nsAReadableString & aMethodName)
{
  mMethodName.Assign(aMethodName);
  return NS_OK;
}

/* attribute DOMString targetObjectURI; */
NS_IMETHODIMP nsSOAPMessage::GetTargetObjectURI(nsAWritableString & aTargetObjectURI)
{
  NS_ENSURE_ARG_POINTER(&aTargetObjectURI);
  aTargetObjectURI.Assign(mTargetObjectURI);
  return NS_OK;
}
NS_IMETHODIMP nsSOAPMessage::SetTargetObjectURI(const nsAReadableString & aTargetObjectURI)
{
  mTargetObjectURI.Assign(aTargetObjectURI);
  return NS_OK;
}

/* void marshallParameters (in nsISupportsArray SOAPParameters); */
NS_IMETHODIMP nsSOAPMessage::MarshallParameters(nsISupportsArray *SOAPParameters)
{
  nsCOMPtr<nsISupports> ignore;
  return mTypes->Marshall(this, SOAPParameters, mEncodingStyleURI, nsSOAPUtils::kSOAPCallType, getter_AddRefs(ignore));
}

/* nsISupportsArray unmarshallParameters (); */
NS_IMETHODIMP nsSOAPMessage::UnmarshallParameters(nsISupportsArray **_retval)
{
  nsCOMPtr<nsISupports> result;
  nsresult rc = mTypes->Unmarshall(this, mMessage, mEncodingStyleURI, nsSOAPUtils::kSOAPCallType, getter_AddRefs(result));
  if (result)
    return result->QueryInterface(NS_GET_IID(nsISupportsArray), (void**)_retval);
  return rc;
}

/* readonly attribute unsigned long status; */
NS_IMETHODIMP nsSOAPMessage::GetStatus(PRUint32 *aStatus)
{
  NS_ENSURE_ARG_POINTER(aStatus);
  *aStatus = mStatus;
  return NS_OK;
}

/* attribute nsISupportsArray protocol; */
NS_IMETHODIMP nsSOAPMessage::GetProtocol(nsISupportsArray * *aProtocol)
{
  NS_ENSURE_ARG_POINTER(aProtocol);
  *aProtocol = mProtocol;
  NS_IF_ADDREF(*aProtocol);
  return NS_OK;
}
NS_IMETHODIMP nsSOAPMessage::SetProtocol(nsISupportsArray * aProtocol)
{
  mProtocol = aProtocol;
  return NS_OK;
}

/* attribute nsISOAPTypeRegistry types; */
NS_IMETHODIMP nsSOAPMessage::GetTypes(nsISOAPTypeRegistry * *aTypes)
{
  NS_ENSURE_ARG_POINTER(aTypes);
  *aTypes = mTypes;
  NS_IF_ADDREF(*aTypes);
  return NS_OK;
}
NS_IMETHODIMP nsSOAPMessage::SetTypes(nsISOAPTypeRegistry * aTypes)
{
  mTypes = aTypes;
  return NS_OK;
}

// The nsIXPCScriptable map declaration that will generate stubs for us...
#define XPC_MAP_CLASSNAME           nsSOAPMessage
#define XPC_MAP_QUOTED_CLASSNAME   "SOAPMessage"
#define                             XPC_MAP_WANT_NEWRESOLVE
#define XPC_MAP_FLAGS       nsIXPCScriptable::USE_JSSTUB_FOR_ADDPROPERTY   | \
                            nsIXPCScriptable::USE_JSSTUB_FOR_DELPROPERTY   | \
                            nsIXPCScriptable::USE_JSSTUB_FOR_SETPROPERTY
#include "xpc_map_end.h" /* This will #undef the above */

NS_NAMED_LITERAL_STRING(marshallparameters, "marshallparameters");
NS_NAMED_LITERAL_STRING(unmarshallparameters, "unmarshallparameters");

static JSBool PR_CALLBACK
MarshallJSParameters(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
                jsval *rval)
{
  nsCOMPtr<nsISupports> value;
  nsAutoString type;
  nsresult rc = nsSOAPJSValue::ConvertJSArgsToValue(cx, argc, argv, PR_TRUE, getter_AddRefs(value), type);
  if (NS_FAILED(rc))
    return JS_FALSE;

//  Unwrap "this" pointer to call original function

  NS_WITH_SERVICE(nsIXPConnect, xpc, nsIXPConnect::GetCID(), &rc); 
  if (NS_FAILED(rc))
    return JS_FALSE;
  nsCOMPtr<nsIXPConnectWrappedNative> wrapper;
  xpc->GetWrappedNativeOfJSObject(cx, obj, getter_AddRefs(wrapper));
  if (wrapper) {
    nsCOMPtr<nsISupports> native;
    wrapper->GetNative(getter_AddRefs(native));
    if (native) {
      nsCOMPtr<nsISOAPMessage> message;
      nsCOMPtr<nsISupportsArray> array;
      message = do_QueryInterface(native);
      if (message) {
        array = do_QueryInterface(value);
        nsresult rc = message->MarshallParameters(array);
        if (!NS_FAILED(rc))
          return JS_TRUE;
      }
    }
  }
  return JS_FALSE;
}

static JSBool PR_CALLBACK
UnmarshallJSParameters(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
                jsval *rval)
{
  nsCOMPtr<nsISupportsArray> value;

//  Unwrap "this" pointer to call original function

  nsresult rc;
  NS_WITH_SERVICE(nsIXPConnect, xpc, nsIXPConnect::GetCID(), &rc); 
  if (NS_FAILED(rc))
    return JS_FALSE;
  nsCOMPtr<nsIXPConnectWrappedNative> wrapper;
  xpc->GetWrappedNativeOfJSObject(cx, obj, getter_AddRefs(wrapper));
  rc = NS_ERROR_FAILURE;
  if (wrapper) {
    nsCOMPtr<nsISupports> native;
    wrapper->GetNative(getter_AddRefs(native));
    if (native) {
      nsCOMPtr<nsISOAPMessage> message;
      message = do_QueryInterface(native);
      if (message) {
        rc = message->UnmarshallParameters(getter_AddRefs(value));
      }
    }
  }
  if (NS_FAILED(rc))
    return JS_FALSE;
  rc = nsSOAPJSValue::ConvertValueToJSVal(cx, value, nsSOAPUtils::kArrayType, rval);
  if (NS_FAILED(rc))
    return JS_FALSE;
  return JS_TRUE;
}

NS_IMETHODIMP
nsSOAPMessage::NewResolve(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
  JSObject *obj, jsval id, PRUint32 flags,
  JSObject **objp, PRBool *_retval)
{
  if (!JSVAL_IS_STRING(id)) {
    return NS_OK;
  }

  JSString *str = JSVAL_TO_STRING(id);
  nsLiteralString name(JS_GetStringChars(str));
  if (name.Equals(marshallparameters)) {
    JSFunction *f = ::JS_DefineFunction(cx, obj, ::JS_GetStringBytes(str),
                                        MarshallJSParameters, 0, JSPROP_READONLY);
    if (!f) {
      return NS_ERROR_OUT_OF_MEMORY;
    }

  } else if (name.Equals(unmarshallparameters)) {
    JSFunction *f = ::JS_DefineFunction(cx, obj, ::JS_GetStringBytes(str),
                                        UnmarshallJSParameters, 0, JSPROP_READONLY);
    if (!f) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }
  return NS_OK;
}

static const char*kAllAccess = "AllAccess";

/* string canCreateWrapper (in nsIIDPtr iid); */
NS_IMETHODIMP 
nsSOAPMessage::CanCreateWrapper(const nsIID * iid, char**_retval)
{
  if (iid->Equals(NS_GET_IID(nsISOAPMessage))) {
    *_retval = nsCRT::strdup(kAllAccess);
  }

  return NS_OK;
}

/* string canCallMethod (in nsIIDPtr iid, in wstring methodName); */
NS_IMETHODIMP 
nsSOAPMessage::CanCallMethod(const nsIID * iid, const PRUnichar*methodName, char**_retval)
{
  if (iid->Equals(NS_GET_IID(nsISOAPMessage))) {
    *_retval = nsCRT::strdup(kAllAccess);
  }

  return NS_OK;
}

/* string canGetProperty (in nsIIDPtr iid, in wstring propertyName); */
NS_IMETHODIMP 
nsSOAPMessage::CanGetProperty(const nsIID * iid, const PRUnichar*propertyName, char**_retval)
{
  if (iid->Equals(NS_GET_IID(nsISOAPMessage))) {
    *_retval = nsCRT::strdup(kAllAccess);
  }

  return NS_OK;
}

/* string canSetProperty (in nsIIDPtr iid, in wstring propertyName); */
NS_IMETHODIMP 
nsSOAPMessage::CanSetProperty(const nsIID * iid, const PRUnichar*propertyName, char**_retval)
{
  if (iid->Equals(NS_GET_IID(nsISOAPMessage))) {
    *_retval = nsCRT::strdup(kAllAccess);
  }

  return NS_OK;
}
