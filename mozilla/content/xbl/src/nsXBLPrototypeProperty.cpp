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
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Original Author: Scott MacGregor (mscott@netscape.com)
 *
 * Contributor(s):
 */

#include "nsCOMPtr.h"
#include "nsXBLPrototypeProperty.h"
#include "nsIContent.h"
#include "nsIAtom.h"
#include "nsIScriptContext.h"
#include "nsIScriptGlobalObject.h"
#include "nsIScriptGlobalObjectOwner.h"
#include "nsIDocument.h"
#include "nsIURI.h"
#include "nsIPref.h"
#include "nsIServiceManager.h"
#include "nsXPIDLString.h"
#include "nsXULAtoms.h"
#include "nsIXPConnect.h"
#include "nsIDOMScriptObjectFactory.h"
#include "nsINameSpaceManager.h"
#include "nsXBLService.h"
#include "nsIXBLDocumentInfo.h"
#include "nsIDOMText.h"

PRUint32 nsXBLPrototypeProperty::gRefCnt = 0;

nsIAtom* nsXBLPrototypeProperty::kMethodAtom = nsnull;
nsIAtom* nsXBLPrototypeProperty::kParameterAtom = nsnull;
nsIAtom* nsXBLPrototypeProperty::kBodyAtom = nsnull;
nsIAtom* nsXBLPrototypeProperty::kPropertyAtom = nsnull;
nsIAtom* nsXBLPrototypeProperty::kOnSetAtom = nsnull;
nsIAtom* nsXBLPrototypeProperty::kOnGetAtom = nsnull;
nsIAtom* nsXBLPrototypeProperty::kGetterAtom = nsnull;
nsIAtom* nsXBLPrototypeProperty::kSetterAtom = nsnull;
nsIAtom* nsXBLPrototypeProperty::kNameAtom = nsnull;
nsIAtom* nsXBLPrototypeProperty::kReadOnlyAtom = nsnull;

#include "nsIJSRuntimeService.h"
static nsIJSRuntimeService* gJSRuntimeService;
static JSRuntime* gScriptRuntime;
static PRInt32 gScriptRuntimeRefcnt;

static nsresult
AddJSGCRoot(JSContext* cx, void* aScriptObjectRef, const char* aName)
{
    PRBool ok;
    ok = JS_AddNamedRoot(cx, aScriptObjectRef, aName);
    if (! ok) return NS_ERROR_OUT_OF_MEMORY;

    if (gScriptRuntimeRefcnt++ == 0) {
        nsServiceManager::GetService("@mozilla.org/js/xpc/RuntimeService;1", // contractid
                                     NS_GET_IID(nsIJSRuntimeService),
                                     (nsISupports**) &gJSRuntimeService);

        if (! gJSRuntimeService)
            return NS_ERROR_FAILURE;

        gJSRuntimeService->GetRuntime(&gScriptRuntime);
    }

    return NS_OK;
}

static nsresult
RemoveJSGCRoot(void* aScriptObjectRef)
{
    if (! gScriptRuntime)
        return NS_ERROR_FAILURE;

    JS_RemoveRootRT(gScriptRuntime, aScriptObjectRef);

    if (--gScriptRuntimeRefcnt == 0) {
        NS_RELEASE(gJSRuntimeService);
        gScriptRuntime = nsnull;
    }

    return NS_OK;
}

nsXBLPrototypeProperty::nsXBLPrototypeProperty(nsIXBLPrototypeBinding * aPrototypeBinding)
: mPrototypeBinding(aPrototypeBinding), mJSMethod(nsnull), mJSGetterObject(nsnull), mJSSetterObject(nsnull), mPropertyIsCompiled(PR_FALSE)
{
  NS_INIT_REFCNT();
  mClassObject = nsnull;
  gRefCnt++;
  if (gRefCnt == 1) 
  {
    kMethodAtom = NS_NewAtom("method");
    kParameterAtom = NS_NewAtom("parameter");
    kBodyAtom = NS_NewAtom("body");
    kPropertyAtom = NS_NewAtom("property");
    kOnSetAtom = NS_NewAtom("onset");
    kOnGetAtom = NS_NewAtom("onget");
    kGetterAtom = NS_NewAtom("getter");
    kSetterAtom = NS_NewAtom("setter");    
    kNameAtom = NS_NewAtom("name");
    kReadOnlyAtom = NS_NewAtom("readonly");

  }
}

nsXBLPrototypeProperty::~nsXBLPrototypeProperty()
{
  if (mJSMethod)
    RemoveJSGCRoot(&mJSMethod);
  if (mJSGetterObject)
    RemoveJSGCRoot(&mJSGetterObject);
  if (mJSSetterObject)
    RemoveJSGCRoot(&mJSSetterObject);

  gRefCnt--;
  if (gRefCnt == 0) 
  {
    NS_RELEASE(kMethodAtom);
    NS_RELEASE(kParameterAtom);
    NS_RELEASE(kBodyAtom);
    NS_RELEASE(kPropertyAtom); 
    NS_RELEASE(kOnSetAtom);
    NS_RELEASE(kOnGetAtom);
    NS_RELEASE(kGetterAtom);
    NS_RELEASE(kSetterAtom);
    NS_RELEASE(kNameAtom);
    NS_RELEASE(kReadOnlyAtom);
  }
}

NS_IMPL_ISUPPORTS1(nsXBLPrototypeProperty, nsIXBLPrototypeProperty)

NS_IMETHODIMP
nsXBLPrototypeProperty::GetNextProperty(nsIXBLPrototypeProperty** aResult)
{
  *aResult = mNextProperty;
  NS_IF_ADDREF(*aResult);
  return NS_OK;
}
  
NS_IMETHODIMP
nsXBLPrototypeProperty::SetNextProperty(nsIXBLPrototypeProperty* aProperty)
{
  mNextProperty = aProperty;
  return NS_OK;
}

// Assumes the class object has already been initialized!!
NS_IMETHODIMP 
nsXBLPrototypeProperty::InitScriptObject(nsIScriptContext * aContext, nsIContent * aBoundElement, void ** aScriptObject)
{
  if (!mClassObject)
  {
    DelayedPropertyConstruction();
  }

  NS_ENSURE_TRUE(mClassObject, NS_ERROR_FAILURE);
  nsresult rv = NS_OK;
  
  JSContext* jscontext = (JSContext*)aContext->GetNativeContext();
  JSObject* global = ::JS_GetGlobalObject(jscontext);
  nsCOMPtr<nsIXPConnectJSObjectHolder> wrapper;

  nsCOMPtr<nsIXPConnect> xpc(do_GetService(nsIXPConnect::GetCID(), &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = xpc->WrapNative(jscontext, global, aBoundElement,
                       NS_GET_IID(nsISupports), getter_AddRefs(wrapper));

  NS_ENSURE_SUCCESS(rv, rv);

  JSObject * object = nsnull;

  rv = wrapper->GetJSObject(&object);
  NS_ENSURE_SUCCESS(rv, rv);

  *aScriptObject = object;

  JSObject* objProto = JS_GetPrototype(jscontext, object);

  JSObject* glob_proto = JS_GetPrototype(jscontext, mClassObject);

  JS_SetPrototype(jscontext, objProto, glob_proto);

  // Root mBoundElement so that it doesn't loose it's binding
  nsCOMPtr<nsIDocument> doc;
  aBoundElement->GetDocument(*getter_AddRefs(doc));

  if (doc) 
  {
    nsCOMPtr<nsIXPConnectWrappedNative> native_wrapper = do_QueryInterface(wrapper);
    if (native_wrapper)
      doc->AddReference(aBoundElement, native_wrapper);
  }

  return rv;
}

NS_IMETHODIMP
nsXBLPrototypeProperty::InstallProperty(nsIScriptContext * aContext, nsIContent * aBoundElement, void * aScriptObject)
{
  if (!mPropertyIsCompiled)
  {
    DelayedPropertyConstruction();
  }

  JSContext* cx = (JSContext*) aContext->GetNativeContext();
  JSObject * scriptObject = (JSObject *) aScriptObject;

  //nsCString cName;
  //cName.AssignWithConversion(mName.get());
  //printf("installing method name: %s\n", cName.get());

  // now we want to re-evaluate our property using aContext and the script object for this window...

  if (mJSMethod)
  {
    JSObject * method = ::JS_CloneFunctionObject(cx, (JSObject *) mJSMethod, scriptObject);
    ::JS_DefineUCProperty(cx, scriptObject, NS_REINTERPRET_CAST(const jschar*, mName.get()),  mName.Length(), OBJECT_TO_JSVAL(method),
                          NULL, NULL, JSPROP_ENUMERATE);


  }
  else if (mJSGetterObject || mJSSetterObject)
  {
    // Having either a getter or setter results in the
    // destruction of any initial value that might be set.
    // This means we only have to worry about defining the getter
    // or setter.
    
    JSObject * getter = nsnull;
    if (mJSGetterObject)
      getter = ::JS_CloneFunctionObject(cx, (JSObject *) mJSGetterObject, scriptObject);
    
    JSObject * setter = nsnull;
    if (mJSSetterObject)
      setter = ::JS_CloneFunctionObject(cx, (JSObject *) mJSSetterObject, scriptObject);

    ::JS_DefineUCProperty(cx, scriptObject, NS_REINTERPRET_CAST(const jschar*, mName.get()), 
                          mName.Length(), JSVAL_VOID,  (JSPropertyOp) getter, 
                          (JSPropertyOp) setter, mJSAttributes); 
  }
  else if (!mLiteralPropertyString.IsEmpty())
  {
    // compile the literal string 
    jsval result = nsnull;
    PRBool undefined;
    aContext->EvaluateStringWithValue(mLiteralPropertyString, 
                                           (JSObject *) aScriptObject,
                                           nsnull, nsnull, 0, nsnull,
                                           (void*) &result, &undefined);
              
    if (!undefined) 
    {
      // Define that value as a property
     ::JS_DefineUCProperty(cx, (JSObject *) aScriptObject, NS_REINTERPRET_CAST(const jschar*, mName.get()), 
                           mName.Length(), result,nsnull, nsnull, mJSAttributes); 
    }
  }

  return NS_OK;
}

nsresult nsXBLPrototypeProperty::DelayedPropertyConstruction()
{
  // FIRST: get the name attribute on the property!!! need to pass in the property tag 
  // into here in order to do that.

  // See if we're a property or a method.
  nsCOMPtr<nsIAtom> tagName;
  mPropertyElement->GetTag(*getter_AddRefs(tagName));

  // We want to pre-compile the properties against a "special context". Then when we actual bind
  // the proto type to a real xbl instance, we'll resolve the pre-compiled JS against the file context.
  // right now this special context is attached to the xbl document info....

  nsCOMPtr<nsIXBLDocumentInfo> docInfo;
  mPrototypeBinding->GetXBLDocumentInfo(nsnull, getter_AddRefs(docInfo));
  NS_ENSURE_TRUE(docInfo, NS_ERROR_FAILURE);

  nsCOMPtr<nsIScriptGlobalObjectOwner> globalOwner (do_QueryInterface(docInfo));
  nsCOMPtr<nsIScriptGlobalObject> globalObject;
  globalOwner->GetScriptGlobalObject(getter_AddRefs(globalObject));

  nsCOMPtr<nsIScriptContext> context;
  globalObject->GetContext(getter_AddRefs(context));
 
  void * classObject;
  JSObject * scopeObject = globalObject->GetGlobalJSObject();
  mPrototypeBinding->GetClassObject(mClassStr, context, (void *) scopeObject, &classObject);
  mClassObject = (JSObject *) classObject;

  if (tagName.get() == kMethodAtom /* && mClassObject */) 
  {
    ParseMethod(context, mPropertyElement, mClassStr);
  }
  else if (tagName.get() == kPropertyAtom) 
  {
    ParseProperty(context, mPropertyElement, mClassStr);
  }

  mPropertyIsCompiled = PR_TRUE;
  mInterfaceElement = nsnull;
  mPropertyElement = nsnull;

  return NS_OK;
}

NS_IMETHODIMP
nsXBLPrototypeProperty::ConstructProperty(nsIContent * aInterfaceElement, nsIContent* aPropertyElement)
{
  mInterfaceElement = aInterfaceElement;
  mPropertyElement = aPropertyElement;

  // Init our class and insert it into the prototype chain.
  nsAutoString className;
  mInterfaceElement->GetAttribute(kNameSpaceID_None, kNameAtom, className);

  if (!className.IsEmpty()) {
    mClassStr.AssignWithConversion(className);
  }
  else {
    mPrototypeBinding->GetBindingURI(mClassStr);
  }

  return NS_OK;
}

const char* gPropertyArgs[] = { "val" };

nsresult nsXBLPrototypeProperty::ParseProperty(nsIScriptContext * aContext, nsIContent* aPropertyElement, const char * aClassStr)
{
  // Obtain our name attribute.
  nsAutoString name;
  nsresult rv = NS_OK;
  aPropertyElement->GetAttribute(kNameSpaceID_None, kNameAtom, mName);

#ifdef DEBUG_mscott
  nsCString cName;
  cName.AssignWithConversion(mName.get());

#endif

  if (!mName.IsEmpty()) 
  {
    // We have a property.
    nsAutoString getter, setter, readOnly;
    aPropertyElement->GetAttribute(kNameSpaceID_None, kOnGetAtom, getter);
    aPropertyElement->GetAttribute(kNameSpaceID_None, kOnSetAtom, setter);
    aPropertyElement->GetAttribute(kNameSpaceID_None, kReadOnlyAtom, readOnly);

    mJSAttributes = JSPROP_ENUMERATE;

    if (readOnly == NS_LITERAL_STRING("true"))
      mJSAttributes |= JSPROP_READONLY;

    // try for first <getter> tag
    if (getter.IsEmpty()) 
    {
      PRInt32 childCount;
      aPropertyElement->ChildCount(childCount);

      nsCOMPtr<nsIContent> getterElement;
      for (PRInt32 j=0; j<childCount; j++) 
      {
        aPropertyElement->ChildAt(j, *getter_AddRefs(getterElement));
        
        if (!getterElement) continue;
        
        nsCOMPtr<nsIAtom> getterTag;
        getterElement->GetTag(*getter_AddRefs(getterTag));
        
        if (getterTag.get() == kGetterAtom) 
        {
          GetTextData(getterElement, getter);
          break;          // stop at first tag
        }
      } // for each childCount
    } // if getter is empty
          
    
    if (!getter.IsEmpty() && mClassObject) 
    {
      nsCAutoString functionUri;
      functionUri.Assign(aClassStr);
      functionUri += ".";
      functionUri.AppendWithConversion(mName.get());
      functionUri += " (getter)";
      rv = aContext->CompileFunction(mClassObject,
                                    nsCAutoString("onget"),
                                    0,
                                    nsnull,
                                    getter, 
                                    functionUri.get(),
                                    0,
                                    PR_FALSE,
                                    &mJSGetterObject);
       if (NS_FAILED(rv)) return NS_ERROR_FAILURE;
       mJSAttributes |= JSPROP_GETTER | JSPROP_SHARED;
      if (mJSGetterObject) 
      {
        // Root the compiled prototype script object.
        JSContext* cx = NS_REINTERPRET_CAST(JSContext*,
                                            aContext->GetNativeContext());
        if (!cx) return NS_ERROR_UNEXPECTED;

        rv = AddJSGCRoot(cx, &mJSGetterObject, "nsXBLPrototypeProperty::mJSGetterObject");
        if (NS_FAILED(rv)) return rv;
      }
    } // if getter is not empty
  
    // try for first <setter> tag
    if (setter.IsEmpty()) 
    {
      PRInt32 childCount;
      aPropertyElement->ChildCount(childCount);

      nsCOMPtr<nsIContent> setterElement;
      for (PRInt32 j=0; j<childCount; j++) 
      {
        aPropertyElement->ChildAt(j, *getter_AddRefs(setterElement));
        
        if (!setterElement) continue;
        
        nsCOMPtr<nsIAtom> setterTag;
        setterElement->GetTag(*getter_AddRefs(setterTag));
        if (setterTag.get() == kSetterAtom) 
        {
          GetTextData(setterElement, setter);
          break;          // stop at first tag
        }
      }
    } // if setter is empty
          
    if (!setter.IsEmpty() && mClassObject) 
    {
      nsCAutoString functionUri (aClassStr);
      functionUri += ".";
      functionUri.AppendWithConversion(mName.get());
      functionUri += " (setter)";
      rv = aContext->CompileFunction(mClassObject,
                                    nsCAutoString("onset"),
                                    1,
                                    gPropertyArgs,
                                    setter, 
                                    functionUri.get(),
                                    0,
                                    PR_FALSE,
                                    &mJSSetterObject);
      if (NS_FAILED(rv)) return NS_ERROR_FAILURE;
      mJSAttributes |= JSPROP_SETTER | JSPROP_SHARED;
      if (mJSSetterObject) 
      {
        // Root the compiled prototype script object.
        JSContext* cx = NS_REINTERPRET_CAST(JSContext*,
                                            aContext->GetNativeContext());
        if (!cx) return NS_ERROR_UNEXPECTED;

        rv = AddJSGCRoot(cx, &mJSSetterObject, "nsXBLPrototypeProperty::mJSSetterObject");
        if (NS_FAILED(rv)) return rv;
      }
    } // if setter wasn't empty....

    // if we came through all of this without a getter or setter, look for a raw
    // literal property string...
    if (mJSSetterObject || mJSGetterObject) 
      return NS_OK;
    else
    {
      return ParseLiteral(aContext, aPropertyElement);
    } // if we have a literal string
  } // if name isn't empty

  return rv;
}

nsresult nsXBLPrototypeProperty::ParseLiteral(nsIScriptContext * aContext, nsIContent* aPropertyElement)
{
  // Look for a normal value and just define that.
  nsCOMPtr<nsIContent> textChild;
  PRInt32 textCount;
  aPropertyElement->ChildCount(textCount);
  for (PRInt32 j = 0; j < textCount; j++) 
  {
    // Get the child.
    aPropertyElement->ChildAt(j, *getter_AddRefs(textChild));
    nsCOMPtr<nsIDOMText> text(do_QueryInterface(textChild));
    if (text)
    {
      nsAutoString data;
      text->GetData(data);
      mLiteralPropertyString += data;
    }
  } // for each element

  return NS_OK;
}

nsresult nsXBLPrototypeProperty::ParseMethod(nsIScriptContext * aContext, nsIContent* aPropertyElement, const char * aClassStr)
{
  // TO DO: fix up class name and class object

  // Obtain our name attribute.
  nsAutoString body;
  nsresult rv = NS_OK;
  aPropertyElement->GetAttribute(kNameSpaceID_None, kNameAtom, mName);

  //nsCString cName;
  //cName.AssignWithConversion(mName.get());
  //printf("method name: %s\n", cName.get());

  // Now walk all of our args.
  // XXX I'm lame. 32 max args allowed.
  char* args[32];
  PRUint32 argCount = 0;
  PRInt32 kidCount;
  aPropertyElement->ChildCount(kidCount);
  for (PRInt32 j = 0; j < kidCount; j++)
  {
    nsCOMPtr<nsIContent> arg;
    aPropertyElement->ChildAt(j, *getter_AddRefs(arg));
    nsCOMPtr<nsIAtom> kidTagName;
    arg->GetTag(*getter_AddRefs(kidTagName));
    
    if (kidTagName.get() == kParameterAtom) 
    {
      // Get the argname and add it to the array.
      nsAutoString argName;
      arg->GetAttribute(kNameSpaceID_None, kNameAtom, argName);
      char* argStr = argName.ToNewCString();
      args[argCount] = argStr;
      argCount++;
    }
    else if (kidTagName.get() == kBodyAtom) 
    {
      PRInt32 textCount;
      arg->ChildCount(textCount);
    
      for (PRInt32 k = 0; k < textCount; k++) 
      {
        // Get the child.
        nsCOMPtr<nsIContent> textChild;
        arg->ChildAt(k, *getter_AddRefs(textChild));
        nsCOMPtr<nsIDOMText> text(do_QueryInterface(textChild));
        if (text) 
        {
          nsAutoString data;
          text->GetData(data);
          body += data;
        }
      } // for each body line
    } // if we have a body atom
  } // for each node in the method

  // Now that we have a body and args, compile the function
  // and then define it as a property.....
  if (!body.IsEmpty()) 
  {
    nsCAutoString cname; cname.AssignWithConversion(mName.get());
    nsCAutoString functionUri (aClassStr);
    functionUri += ".";
    functionUri += cname;
    functionUri += "()";
    
    rv = aContext->CompileFunction(mClassObject,
                                  cname,
                                  argCount,
                                  (const char**)args,
                                  body, 
                                  functionUri.get(),
                                  0,
                                  PR_FALSE,
                                  &mJSMethod);

    if (mJSMethod) 
    {
        // Root the compiled prototype script object.
        JSContext* cx = NS_REINTERPRET_CAST(JSContext*,
                                            aContext->GetNativeContext());
        if (!cx) return NS_ERROR_UNEXPECTED;

        rv = AddJSGCRoot(cx, &mJSMethod, "nsXBLPrototypeProperty::mJSMethod");
        if (NS_FAILED(rv)) return rv;
    }
  }
  
  for (PRUint32 l = 0; l < argCount; l++) 
     nsMemory::Free(args[l]);

  return rv;
}

nsresult
nsXBLPrototypeProperty::GetTextData(nsIContent *aParent, nsString& aResult)
{
  aResult.Truncate(0);

  nsCOMPtr<nsIContent> textChild;
  PRInt32 textCount;
  aParent->ChildCount(textCount);
  nsAutoString answer;
  for (PRInt32 j = 0; j < textCount; j++) {
    // Get the child.
    aParent->ChildAt(j, *getter_AddRefs(textChild));
    nsCOMPtr<nsIDOMText> text(do_QueryInterface(textChild));
    if (text) 
    {
      nsAutoString data;
      text->GetData(data);
      aResult += data;
    }
  }
  return NS_OK;
}

///////////////////////////////////////////////////////////////////////////////////

nsresult
NS_NewXBLPrototypeProperty(nsIXBLPrototypeBinding * aPrototypeBinding, nsIXBLPrototypeProperty ** aResult)
{
  *aResult = new nsXBLPrototypeProperty(aPrototypeBinding);
  if (!*aResult)
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(*aResult);
  return NS_OK;
}
