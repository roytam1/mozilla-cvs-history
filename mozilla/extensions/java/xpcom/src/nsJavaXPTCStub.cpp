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
 * The Original Code is Java XPCOM Bindings.
 *
 * The Initial Developer of the Original Code is
 * IBM Corporation.
 * Portions created by the Initial Developer are Copyright (C) 2004
 * IBM Corporation. All Rights Reserved.
 *
 * Contributor(s):
 *   Javier Pedemonte (jhpedemonte@gmail.com)
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

#include "nsJavaXPTCStub.h"
#include "nsJavaWrapper.h"
#include "nsJavaXPCOMBindingUtils.h"
#include "prmem.h"
#include "nsIInterfaceInfoManager.h"
#include "nsString.h"
#include "nsString.h"
#include "nsCRT.h"


nsJavaXPTCStub::nsJavaXPTCStub(JNIEnv* aJavaEnv, jobject aJavaObject,
                               nsIInterfaceInfo *aIInfo)
  : mJavaEnv(aJavaEnv)
  , mIInfo(aIInfo)
  , mMaster(nsnull)
{
  mJavaObject = aJavaEnv->NewGlobalRef(aJavaObject);

#ifdef DEBUG_JAVAXPCOM
  jstring name;
  const char* javaObjectName = nsnull;
  jclass clazz = mJavaEnv->GetObjectClass(mJavaObject);
  if (clazz) {
    name = (jstring) mJavaEnv->CallObjectMethod(clazz, getNameMID);
    if (name)
      javaObjectName = mJavaEnv->GetStringUTFChars(name, nsnull);
  }

  nsID* iid = nsnull;
  char* iid_str = nsnull;
  if (mIInfo) {
    nsresult rv = mIInfo->GetInterfaceIID(&iid);
    if (NS_SUCCEEDED(rv)) {
      iid_str = iid->ToString();
      nsMemory::Free(iid);
    }
  }

  LOG(("+++ nsJavaXPTCStub(this=0x%08x java_obj=0x%08x %s iid=%s)\n", (int) this,
       mJavaEnv->CallIntMethod(mJavaObject, hashCodeMID),
       javaObjectName ? javaObjectName : "<-->", iid_str ? iid_str : "NULL"));
  if (name)
    mJavaEnv->ReleaseStringUTFChars(name, javaObjectName);
  if (iid_str)
    nsMemory::Free(iid_str);
#endif
}

nsJavaXPTCStub::~nsJavaXPTCStub()
{
#ifdef DEBUG_JAVAXPCOM
  jstring name;
  const char* javaObjectName = nsnull;
  jclass clazz = mJavaEnv->GetObjectClass(mJavaObject);
  if (clazz) {
    name = (jstring) mJavaEnv->CallObjectMethod(clazz, getNameMID);
    if (name)
      javaObjectName = mJavaEnv->GetStringUTFChars(name, nsnull);
  }

  LOG(("--- ~nsJavaXPTCStub(this=0x%08x java_obj=0x%08x %s)\n", (int) this,
       mJavaEnv->CallIntMethod(mJavaObject, hashCodeMID),
       javaObjectName ? javaObjectName : "<-->"));
  if (name)
    mJavaEnv->ReleaseStringUTFChars(name, javaObjectName);
#endif

  if (!mMaster) {
    // delete each child stub
    for (PRInt32 i = 0; i < mChildren.Count(); i++) {
      delete (nsJavaXPTCStub*) mChildren[i];
    }

    gBindings->RemoveBinding(mJavaEnv, mJavaObject, this);
  }

  mJavaEnv->DeleteGlobalRef(mJavaObject);
}

NS_IMETHODIMP_(nsrefcnt)
nsJavaXPTCStub::AddRef()
{
  // if this is a child
  if (mMaster)
    return mMaster->AddRef();

  // if this is the master interface
  NS_PRECONDITION(PRInt32(mRefCnt) >= 0, "illegal refcnt");
  NS_ASSERT_OWNINGTHREAD(nsJavaXPTCStub);
  ++mRefCnt;
  NS_LOG_ADDREF(this, mRefCnt, "nsJavaXPTCStub", sizeof(*this));
  return mRefCnt;
}

NS_IMETHODIMP_(nsrefcnt)
nsJavaXPTCStub::Release()
{
  // if this is a child
  if (mMaster)
    return mMaster->Release();

  NS_PRECONDITION(0 != mRefCnt, "dup release");
  NS_ASSERT_OWNINGTHREAD(nsJavaXPTCStub);
  --mRefCnt;
  NS_LOG_RELEASE(this, mRefCnt, "nsJavaXPTCStub");
  if (mRefCnt == 0) {
    mRefCnt = 1; /* stabilize */
    delete this;
    return 0;
  }
  return mRefCnt;
}

NS_IMETHODIMP
nsJavaXPTCStub::QueryInterface(const nsID &aIID, void **aInstancePtr)
{
  LOG(("JavaStub::QueryInterface()\n"));
  *aInstancePtr = nsnull;
  nsJavaXPTCStub *master = mMaster ? mMaster : this;

  // This helps us differentiate between the help classes.
  if (aIID.Equals(NS_GET_IID(nsJavaXPTCStub)))
  {
    *aInstancePtr = master;
    NS_ADDREF(this);
    return NS_OK;
  }

  // always return the master stub for nsISupports
  if (aIID.Equals(NS_GET_IID(nsISupports)))
  {
    *aInstancePtr = NS_STATIC_CAST(nsISupports*,
                                   NS_STATIC_CAST(nsXPTCStubBase*, master));
    NS_ADDREF(master);
    return NS_OK;
  }

  // All Java objects support weak references
  if (aIID.Equals(NS_GET_IID(nsISupportsWeakReference)))
  {
    *aInstancePtr = NS_STATIC_CAST(nsISupportsWeakReference*, master);
    NS_ADDREF(master);
    return NS_OK;
  }

  // does any existing stub support the requested IID?
  nsJavaXPTCStub *stub = master->FindStubSupportingIID(aIID);
  if (stub)
  {
    *aInstancePtr = stub;
    NS_ADDREF(stub);
    return NS_OK;
  }

  // Query Java object
  LOG(("\tCalling Java object queryInterface\n"));
  jmethodID qiMID = 0;
  jclass clazz = mJavaEnv->GetObjectClass(mJavaObject);
  if (clazz) {
    char* sig = "(Ljava/lang/String;)Lorg/mozilla/xpcom/nsISupports;";
    qiMID = mJavaEnv->GetMethodID(clazz, "queryInterface", sig);
    NS_ASSERTION(qiMID, "Failed to get queryInterface method ID");
  }

  if (qiMID == nsnull) {
    mJavaEnv->ExceptionClear();
    return NS_NOINTERFACE;
  }

  // construct IID string
  jstring iid_jstr = nsnull;
  char* iid_str = aIID.ToString();
  if (iid_str) {
    iid_jstr = mJavaEnv->NewStringUTF(iid_str);
  }
  if (!iid_str || !iid_jstr) {
    mJavaEnv->ExceptionClear();
    return NS_ERROR_OUT_OF_MEMORY;
  }
  PR_Free(iid_str);

  // call queryInterface method
  jobject obj = mJavaEnv->CallObjectMethod(mJavaObject, qiMID, iid_jstr);
  if (mJavaEnv->ExceptionCheck()) {
    mJavaEnv->ExceptionClear();
    return NS_ERROR_FAILURE;
  }
  if (!obj)
    return NS_NOINTERFACE;

  // Get interface info for new java object
  nsCOMPtr<nsIInterfaceInfoManager> iim = XPTI_GetInterfaceInfoManager();
  nsCOMPtr<nsIInterfaceInfo> iinfo;
  nsresult rv = iim->GetInfoForIID(&aIID, getter_AddRefs(iinfo));
  if (NS_FAILED(rv))
    return rv;

  stub = new nsJavaXPTCStub(mJavaEnv, obj, iinfo);
  if (!stub)
    return NS_ERROR_OUT_OF_MEMORY;

  // add stub to the master's list of children, so we can preserve
  // symmetry in future QI calls.  the master will delete each child
  // when it is destroyed.  the refcount of each child is bound to
  // the refcount of the master.  this is done to deal with code
  // like this:
  //
  //   nsCOMPtr<nsIBar> bar = ...;
  //   nsIFoo *foo;
  //   {
  //     nsCOMPtr<nsIFoo> temp = do_QueryInterface(bar);
  //     foo = temp;
  //   }
  //   foo->DoStuff();
  //
  // while this code is not valid XPCOM (since it is using |foo|
  // after having called Release on it), such code is unfortunately
  // very common in the mozilla codebase.  the assumption this code
  // is making is that so long as |bar| is alive, it should be valid
  // to access |foo| even if the code doesn't own a strong reference
  // to |foo|!  clearly wrong, but we need to support it anyways.

  stub->mMaster = master;
  master->mChildren.AppendElement(stub);

  *aInstancePtr = stub;
  NS_ADDREF(stub);
  return NS_OK;
}

PRBool
nsJavaXPTCStub::SupportsIID(const nsID &iid)
{
  PRBool match;
  nsCOMPtr<nsIInterfaceInfo> iter = mIInfo;
  do
  {
    if (NS_SUCCEEDED(iter->IsIID(&iid, &match)) && match)
      return PR_TRUE;

    nsCOMPtr<nsIInterfaceInfo> parent;
    iter->GetParent(getter_AddRefs(parent));
    iter = parent;
  }
  while (iter != nsnull);

  return PR_FALSE;
}

nsJavaXPTCStub *
nsJavaXPTCStub::FindStubSupportingIID(const nsID &iid)
{
  NS_ASSERTION(mMaster == nsnull, "this is not a master stub");

  if (SupportsIID(iid))
    return this;

  for (PRInt32 i = 0; i < mChildren.Count(); i++)
  {
    nsJavaXPTCStub *child = (nsJavaXPTCStub *) mChildren[i];
    if (child->SupportsIID(iid))
      return child;
  }
  return nsnull;
}

NS_IMETHODIMP
nsJavaXPTCStub::GetInterfaceInfo(nsIInterfaceInfo **aInfo)
{
  NS_ADDREF(*aInfo = mIInfo);
  return NS_OK;
}

NS_IMETHODIMP
nsJavaXPTCStub::CallMethod(PRUint16 aMethodIndex,
                           const nsXPTMethodInfo *aMethodInfo,
                           nsXPTCMiniVariant *aParams)
{
#ifdef DEBUG_JAVAXPCOM
  const char* ifaceName;
  mIInfo->GetNameShared(&ifaceName);
  LOG(("nsJavaXPTCStub::CallMethod [%s::%s]\n", ifaceName, aMethodInfo->GetName()));
#endif

  nsresult rv = NS_OK;

  nsCAutoString methodSig("(");

  // Create jvalue array to hold Java params
  PRUint8 paramCount = aMethodInfo->GetParamCount();
  jvalue* java_params = nsnull;
  const nsXPTParamInfo* retvalInfo = nsnull;
  if (paramCount) {
    java_params = new jvalue[paramCount];
    if (!java_params)
      return NS_ERROR_OUT_OF_MEMORY;

    for (PRUint8 i = 0; i < paramCount && NS_SUCCEEDED(rv); i++)
    {
      const nsXPTParamInfo &paramInfo = aMethodInfo->GetParam(i);
      NS_ASSERTION(!paramInfo.IsDipper(), "Dipper!");
      if (!paramInfo.IsRetval()) {
        rv = SetupJavaParams(paramInfo, aMethodInfo, aMethodIndex, aParams,
                             aParams[i], java_params[i], methodSig);
      } else {
        retvalInfo = &paramInfo;
      }
    }
    NS_ASSERTION(NS_SUCCEEDED(rv), "SetupJavaParams failed");
  }

  // Finish method signature
  if (NS_SUCCEEDED(rv)) {
    methodSig.Append(')');
    if (retvalInfo) {
      nsCAutoString retvalSig;
      rv = GetRetvalSig(retvalInfo, retvalSig);
      methodSig.Append(retvalSig);
    } else {
      methodSig.Append('V');
    }
    NS_ASSERTION(NS_SUCCEEDED(rv), "GetRetvalSig failed");
  }

  // Get Java method to call
  jmethodID mid = nsnull;
  if (NS_SUCCEEDED(rv)) {
    nsCAutoString methodName;
    if (aMethodInfo->IsGetter() || aMethodInfo->IsSetter()) {
      if (aMethodInfo->IsGetter())
        methodName.AppendLiteral("get");
      else
        methodName.AppendLiteral("set");
      methodName.AppendASCII(aMethodInfo->GetName());
      methodName.SetCharAt(toupper(methodName[3]), 3);
    } else {
      methodName.AppendASCII(aMethodInfo->GetName());
      methodName.SetCharAt(tolower(methodName[0]), 0);
    }

    jclass clazz = mJavaEnv->GetObjectClass(mJavaObject);
    if (clazz)
      mid = mJavaEnv->GetMethodID(clazz, methodName.get(), methodSig.get());
    NS_ASSERTION(mid, "Failed to get requested method for Java object");
    if (!mid)
      rv = NS_ERROR_FAILURE;
  }

  // Call method
  jvalue retval;
  if (NS_SUCCEEDED(rv)) {
    if (!retvalInfo) {
      mJavaEnv->CallVoidMethodA(mJavaObject, mid, java_params);
    } else {
      switch (retvalInfo->GetType().TagPart())
      {
        case nsXPTType::T_I8:
        case nsXPTType::T_U8:
          retval.b = mJavaEnv->CallByteMethodA(mJavaObject, mid,
                                               java_params);
          break;

        case nsXPTType::T_I16:
        case nsXPTType::T_U16:
          retval.s = mJavaEnv->CallShortMethodA(mJavaObject, mid,
                                                java_params);
          break;

        case nsXPTType::T_I32:
        case nsXPTType::T_U32:
          retval.i = mJavaEnv->CallIntMethodA(mJavaObject, mid,
                                              java_params);
          break;

        case nsXPTType::T_FLOAT:
          retval.f = mJavaEnv->CallFloatMethodA(mJavaObject, mid,
                                                java_params);
          break;

        case nsXPTType::T_DOUBLE:
          retval.d = mJavaEnv->CallDoubleMethodA(mJavaObject, mid,
                                                 java_params);
          break;

        case nsXPTType::T_BOOL:
          retval.z = mJavaEnv->CallBooleanMethodA(mJavaObject, mid,
                                                  java_params);
          break;

        case nsXPTType::T_CHAR:
        case nsXPTType::T_WCHAR:
          retval.c = mJavaEnv->CallCharMethodA(mJavaObject, mid,
                                               java_params);
          break;

        case nsXPTType::T_CHAR_STR:
        case nsXPTType::T_WCHAR_STR:
        case nsXPTType::T_IID:
        case nsXPTType::T_ASTRING:
        case nsXPTType::T_DOMSTRING:
        case nsXPTType::T_UTF8STRING:
        case nsXPTType::T_CSTRING:
        case nsXPTType::T_INTERFACE:
        case nsXPTType::T_INTERFACE_IS:
          retval.l = mJavaEnv->CallObjectMethodA(mJavaObject, mid,
                                                 java_params);
          break;

        case nsXPTType::T_VOID:
          retval.i = mJavaEnv->CallIntMethodA(mJavaObject, mid,
                                              java_params);
          break;

        default:
          NS_WARNING("Unhandled retval type");
          break;
      }
    }

    // Check for exception from called Java function
    jthrowable exp = mJavaEnv->ExceptionOccurred();
    if (exp) {
#ifdef DEBUG
      mJavaEnv->ExceptionDescribe();
#endif

      // If the exception is an instance of XPCOMException, then get the
      // nsresult from the exception instance.  Else, default to
      // NS_ERROR_FAILURE.
      if (mJavaEnv->IsInstanceOf(exp, xpcomExceptionClass)) {
        jfieldID fid;
        fid = mJavaEnv->GetFieldID(xpcomExceptionClass, "errorcode", "J");
        if (fid) {
          rv = mJavaEnv->GetLongField(exp, fid);
        } else {
          rv = NS_ERROR_FAILURE;
        }
        NS_ASSERTION(fid, "Couldn't get 'errorcode' field of XPCOMException");
      } else {
        rv = NS_ERROR_FAILURE;
      }
    }
  }

  // Handle any 'inout', 'out' and 'retval' params
  if (NS_SUCCEEDED(rv)) {
    for (PRUint8 i = 0; i < paramCount; i++)
    {
      const nsXPTParamInfo &paramInfo = aMethodInfo->GetParam(i);
      if (paramInfo.IsIn() && !paramInfo.IsOut()) // 'in'
        continue;

      // If param is null, then caller is not expecting an output value.
      if (aParams[i].val.p == nsnull)
        continue;

      if (!paramInfo.IsRetval()) {
        rv = FinalizeJavaParams(paramInfo, aMethodInfo, aMethodIndex, aParams,
                                aParams[i], java_params[i]);
      } else {
        rv = FinalizeJavaParams(paramInfo, aMethodInfo, aMethodIndex, aParams,
                                aParams[i], retval);
      }
    }
    NS_ASSERTION(NS_SUCCEEDED(rv), "FinalizeJavaParams/SetXPCOMRetval failed");
  }

  if (java_params)
    delete [] java_params;

  mJavaEnv->ExceptionClear();
  return rv;
}

/**
 * Handle 'in', 'inout', and 'out' params
 */
nsresult
nsJavaXPTCStub::SetupJavaParams(const nsXPTParamInfo &aParamInfo,
                const nsXPTMethodInfo* aMethodInfo,
                PRUint16 aMethodIndex,
                nsXPTCMiniVariant* aDispatchParams,
                nsXPTCMiniVariant &aVariant, jvalue &aJValue,
                nsACString &aMethodSig)
{
  nsresult rv = NS_OK;
  const nsXPTType &type = aParamInfo.GetType();

  PRUint8 tag = type.TagPart();
  switch (tag)
  {
    case nsXPTType::T_I8:
    case nsXPTType::T_U8:
    {
      if (!aParamInfo.IsOut()) {  // 'in'
        aJValue.b = aVariant.val.u8;
        aMethodSig.Append('B');
      } else {  // 'inout' & 'out'
        if (aVariant.val.p) {
          jbyteArray array = mJavaEnv->NewByteArray(1);
          if (!array) {
            rv = NS_ERROR_OUT_OF_MEMORY;
            break;
          }

          mJavaEnv->SetByteArrayRegion(array, 0, 1, (jbyte*) aVariant.val.p);
          aJValue.l = array;
        } else {
          aJValue.l = nsnull;
        }
        aMethodSig.AppendLiteral("[B");
      }
    }
    break;

    case nsXPTType::T_I16:
    case nsXPTType::T_U16:
    {
      if (!aParamInfo.IsOut()) {  // 'in'
        aJValue.s = aVariant.val.u16;
        aMethodSig.Append('S');
      } else {  // 'inout' & 'out'
        if (aVariant.val.p) {
          jshortArray array = mJavaEnv->NewShortArray(1);
          if (!array) {
            rv = NS_ERROR_OUT_OF_MEMORY;
            break;
          }

          mJavaEnv->SetShortArrayRegion(array, 0, 1, (jshort*) aVariant.val.p);
          aJValue.l = array;
        } else {
          aJValue.l = nsnull;
        }
        aMethodSig.AppendLiteral("[S");
      }
    }
    break;

    case nsXPTType::T_I32:
    case nsXPTType::T_U32:
    {
      if (!aParamInfo.IsOut()) {  // 'in'
        aJValue.i = aVariant.val.u32;
        aMethodSig.Append('I');
      } else {  // 'inout' & 'out'
        if (aVariant.val.p) {
          jintArray array = mJavaEnv->NewIntArray(1);
          if (!array) {
            rv = NS_ERROR_OUT_OF_MEMORY;
            break;
          }

          mJavaEnv->SetIntArrayRegion(array, 0, 1, (jint*) aVariant.val.p);
          aJValue.l = array;
        } else {
          aJValue.l = nsnull;
        }
        aMethodSig.AppendLiteral("[I");
      }
    }
    break;

    case nsXPTType::T_I64:
    case nsXPTType::T_U64:
    {
      if (!aParamInfo.IsOut()) {  // 'in'
        aJValue.j = aVariant.val.u64;
        aMethodSig.Append('J');
      } else {  // 'inout' & 'out'
        if (aVariant.val.p) {
          jlongArray array = mJavaEnv->NewLongArray(1);
          if (!array) {
            rv = NS_ERROR_OUT_OF_MEMORY;
            break;
          }

          mJavaEnv->SetLongArrayRegion(array, 0, 1, (jlong*) aVariant.val.p);
          aJValue.l = array;
        } else {
          aJValue.l = nsnull;
        }
        aMethodSig.AppendLiteral("[J");
      }
    }
    break;

    case nsXPTType::T_FLOAT:
    {
      if (!aParamInfo.IsOut()) {  // 'in'
        aJValue.f = aVariant.val.f;
        aMethodSig.Append('F');
      } else {  // 'inout' & 'out'
        if (aVariant.val.p) {
          jfloatArray array = mJavaEnv->NewFloatArray(1);
          if (!array) {
            rv = NS_ERROR_OUT_OF_MEMORY;
            break;
          }

          mJavaEnv->SetFloatArrayRegion(array, 0, 1, (jfloat*) aVariant.val.p);
          aJValue.l = array;
        } else {
          aJValue.l = nsnull;
        }
        aMethodSig.AppendLiteral("[F");
      }
    }
    break;

    case nsXPTType::T_DOUBLE:
    {
      if (!aParamInfo.IsOut()) {  // 'in'
        aJValue.d = aVariant.val.d;
        aMethodSig.Append('D');
      } else {  // 'inout' & 'out'
        if (aVariant.val.p) {
          jdoubleArray array = mJavaEnv->NewDoubleArray(1);
          if (!array) {
            rv = NS_ERROR_OUT_OF_MEMORY;
            break;
          }

          mJavaEnv->SetDoubleArrayRegion(array, 0, 1,
                                         (jdouble*) aVariant.val.p);
          aJValue.l = array;
        } else {
          aJValue.l = nsnull;
        }
        aMethodSig.AppendLiteral("[D");
      }
    }
    break;

    case nsXPTType::T_BOOL:
    {
      if (!aParamInfo.IsOut()) {  // 'in'
        aJValue.z = aVariant.val.b;
        aMethodSig.Append('Z');
      } else {  // 'inout' & 'out'
        if (aVariant.val.p) {
          jbooleanArray array = mJavaEnv->NewBooleanArray(1);
          if (!array) {
            rv = NS_ERROR_OUT_OF_MEMORY;
            break;
          }

          mJavaEnv->SetBooleanArrayRegion(array, 0, 1,
                                          (jboolean*) aVariant.val.p);
          aJValue.l = array;
        } else {
          aJValue.l = nsnull;
        }
        aMethodSig.AppendLiteral("[Z");
      }
    }
    break;

    case nsXPTType::T_CHAR:
    case nsXPTType::T_WCHAR:
    {
      if (!aParamInfo.IsOut()) {  // 'in'
        if (tag == nsXPTType::T_CHAR)
          aJValue.c = aVariant.val.c;
        else
          aJValue.c = aVariant.val.wc;
        aMethodSig.Append('C');
      } else {  // 'inout' & 'out'
        if (aVariant.val.p) {
          jcharArray array = mJavaEnv->NewCharArray(1);
          if (!array) {
            rv = NS_ERROR_OUT_OF_MEMORY;
            break;
          }

          mJavaEnv->SetCharArrayRegion(array, 0, 1, (jchar*) aVariant.val.p);
          aJValue.l = array;
        } else {
          aJValue.l = nsnull;
        }
        aMethodSig.AppendLiteral("[C");
      }
    }
    break;

    case nsXPTType::T_CHAR_STR:
    case nsXPTType::T_WCHAR_STR:
    {
      void* ptr = nsnull;
      if (!aParamInfo.IsOut()) {  // 'in'
        ptr = aVariant.val.p;
      } else if (aVariant.val.p) {  // 'inout' & 'out'
        void** variant = NS_STATIC_CAST(void**, aVariant.val.p);
        ptr = *variant;
      }

      jobject str;
      if (ptr) {
        if (tag == nsXPTType::T_CHAR_STR) {
          str = mJavaEnv->NewStringUTF((const char*) ptr);
        } else {
          const PRUnichar* buf = (const PRUnichar*) ptr;
          str = mJavaEnv->NewString(buf, nsCRT::strlen(buf));
        }
        if (!str) {
          rv = NS_ERROR_OUT_OF_MEMORY;
          break;
        }
      } else {
        str = nsnull;
      }

      if (!aParamInfo.IsOut()) {  // 'in'
        aJValue.l = str;
        aMethodSig.AppendLiteral("Ljava/lang/String;");
      } else {  // 'inout' & 'out'
        if (aVariant.val.p) {
          aJValue.l = mJavaEnv->NewObjectArray(1, stringClass, str);
          if (aJValue.l == nsnull) {
            rv = NS_ERROR_OUT_OF_MEMORY;
            break;
          }
        } else {
          aJValue.l = nsnull;
        }
        aMethodSig.AppendLiteral("[Ljava/lang/String;");
      }
    }
    break;

    case nsXPTType::T_IID:
    {
      nsID* iid = nsnull;
      if (!aParamInfo.IsOut()) {  // 'in'
        iid = NS_STATIC_CAST(nsID*, aVariant.val.p);
      } else if (aVariant.val.p) {  // 'inout' & 'out'
        nsID** variant = NS_STATIC_CAST(nsID**, aVariant.val.p);
        iid = *variant;
      }

      jobject str;
      if (iid) {
        char* iid_str = iid->ToString();
        if (iid_str) {
          str = mJavaEnv->NewStringUTF(iid_str);
        }
        if (!iid_str || !str) {
          rv = NS_ERROR_OUT_OF_MEMORY;
          break;
        }
        PR_Free(iid_str);
      } else {
        str = nsnull;
      }

      if (!aParamInfo.IsOut()) {  // 'in'
        aJValue.l = str;
        aMethodSig.AppendLiteral("Ljava/lang/String;");
      } else {  // 'inout' & 'out'
        if (aVariant.val.p) {
          aJValue.l = mJavaEnv->NewObjectArray(1, stringClass, str);
          if (aJValue.l == nsnull) {
            rv = NS_ERROR_OUT_OF_MEMORY;
            break;
          }
        } else {
          aJValue.l = nsnull;
        }
        aMethodSig.AppendLiteral("[Ljava/lang/String;");
      }
    }
    break;

    case nsXPTType::T_INTERFACE:
    case nsXPTType::T_INTERFACE_IS:
    {
      void* xpcom_obj = nsnull;
      if (!aParamInfo.IsOut()) {  // 'in'
        xpcom_obj = aVariant.val.p;
      } else if (aVariant.val.p) {  // 'inout' & 'out'
        void** variant = NS_STATIC_CAST(void**, aVariant.val.p);
        xpcom_obj = *variant;
      }

      jobject java_stub = nsnull;
      if (xpcom_obj) {
        nsID iid;
        rv = GetIIDForMethodParam(mIInfo, aMethodInfo, aParamInfo, aMethodIndex,
                                  aDispatchParams, PR_FALSE, iid);
        if (NS_FAILED(rv))
          break;

        // Get matching Java object for given xpcom object
        rv = gBindings->GetJavaObject(mJavaEnv, xpcom_obj, iid, PR_FALSE,
                                      &java_stub);
        if (NS_FAILED(rv))
          break;
      }

      if (!aParamInfo.IsOut()) {  // 'in'
        aJValue.l = java_stub;
        aMethodSig.AppendLiteral("Lorg/mozilla/xpcom/nsISupports;");
      } else {  // 'inout' & 'out'
        if (aVariant.val.p) {
          aJValue.l = mJavaEnv->NewObjectArray(1, nsISupportsClass, java_stub);
          if (aJValue.l == nsnull) {
            rv = NS_ERROR_OUT_OF_MEMORY;
            break;
          }
        } else {
          aJValue.l = nsnull;
        }
        aMethodSig.AppendLiteral("[Lorg/mozilla/xpcom/nsISupports;");
      }
    }
    break;

    case nsXPTType::T_ASTRING:
    case nsXPTType::T_DOMSTRING:
    {
      nsString* str = nsnull;
      if (!aParamInfo.IsOut()) {  // 'in'
        str = NS_STATIC_CAST(nsString*, aVariant.val.p);
      } else if (aVariant.val.p) {  // 'inout' & 'out'
        nsString** variant = NS_STATIC_CAST(nsString**, aVariant.val.p);
        str = *variant;
      }

      jstring jstr;
      if (str) {
        jstr = mJavaEnv->NewString(str->get(), str->Length());
        if (!jstr) {
          rv = NS_ERROR_OUT_OF_MEMORY;
          break;
        }
      } else {
        jstr = nsnull;
      }

      if (!aParamInfo.IsOut()) {  // 'in'
        aJValue.l = jstr;
        aMethodSig.AppendLiteral("Ljava/lang/String;");
      } else {  // 'inout' & 'out'
        if (aVariant.val.p) {
          aJValue.l = mJavaEnv->NewObjectArray(1, stringClass, jstr);
          if (aJValue.l == nsnull) {
            rv = NS_ERROR_OUT_OF_MEMORY;
            break;
          }
        } else {
          aJValue.l = nsnull;
        }
        aMethodSig.AppendLiteral("[Ljava/lang/String;");
      }
    }
    break;

    case nsXPTType::T_UTF8STRING:
    case nsXPTType::T_CSTRING:
    {
      nsCString* str = nsnull;
      if (!aParamInfo.IsOut()) {  // 'in'
        str = NS_STATIC_CAST(nsCString*, aVariant.val.p);
      } else if (aVariant.val.p) {  // 'inout' & 'out'
        nsCString** variant = NS_STATIC_CAST(nsCString**, aVariant.val.p);
        str = *variant;
      }

      jstring jstr;
      if (str) {
        jstr = mJavaEnv->NewStringUTF(str->get());
        if (!jstr) {
          rv = NS_ERROR_OUT_OF_MEMORY;
          break;
        }
      } else {
        jstr = nsnull;
      }

      if (!aParamInfo.IsOut()) {  // 'in'
        aJValue.l = jstr;
        aMethodSig.AppendLiteral("Ljava/lang/String;");
      } else {  // 'inout' & 'out'
        if (aVariant.val.p) {
          aJValue.l = mJavaEnv->NewObjectArray(1, stringClass, jstr);
          if (aJValue.l == nsnull) {
            rv = NS_ERROR_OUT_OF_MEMORY;
            break;
          }
        } else {
          aJValue.l = nsnull;
        }
        aMethodSig.AppendLiteral("[Ljava/lang/String;");
      }
    }
    break;

    // Pass the 'void*' address as an integer
    case nsXPTType::T_VOID:
    {
      if (!aParamInfo.IsOut()) {  // 'in'
        aJValue.i = (jint) aVariant.val.p;
        aMethodSig.Append('I');
      } else {  // 'inout' & 'out'
        if (aVariant.val.p) {
          jintArray array = mJavaEnv->NewIntArray(1);
          if (!array) {
            rv = NS_ERROR_OUT_OF_MEMORY;
            break;
          }

          mJavaEnv->SetIntArrayRegion(array, 0, 1, (jint*) aVariant.val.p);
          aJValue.l = array;
        } else {
          aJValue.l = nsnull;
        }
        aMethodSig.AppendLiteral("[I");
      }
    }
    break;

    case nsXPTType::T_ARRAY:
      NS_WARNING("array types are not yet supported");
      return NS_ERROR_NOT_IMPLEMENTED;
      break;

    case nsXPTType::T_PSTRING_SIZE_IS:
    case nsXPTType::T_PWSTRING_SIZE_IS:
    default:
      NS_WARNING("unexpected parameter type");
      return NS_ERROR_UNEXPECTED;
  }

  return rv;
}

nsresult
nsJavaXPTCStub::GetRetvalSig(const nsXPTParamInfo* aParamInfo,
                           nsACString &aRetvalSig)
{
  const nsXPTType &type = aParamInfo->GetType();
  PRUint8 tag = type.TagPart();
  switch (tag)
  {
    case nsXPTType::T_I8:
    case nsXPTType::T_U8:
      aRetvalSig.Append('B');
      break;

    case nsXPTType::T_I16:
    case nsXPTType::T_U16:
      aRetvalSig.Append('S');
      break;

    case nsXPTType::T_I32:
    case nsXPTType::T_U32:
      aRetvalSig.Append('I');
      break;

    case nsXPTType::T_I64:
    case nsXPTType::T_U64:
      aRetvalSig.Append('J');
      break;

    case nsXPTType::T_FLOAT:
      aRetvalSig.Append('F');
      break;

    case nsXPTType::T_DOUBLE:
      aRetvalSig.Append('D');
      break;

    case nsXPTType::T_BOOL:
      aRetvalSig.Append('Z');
      break;

    case nsXPTType::T_CHAR:
    case nsXPTType::T_WCHAR:
      aRetvalSig.Append('C');
      break;

    case nsXPTType::T_CHAR_STR:
    case nsXPTType::T_WCHAR_STR:
    case nsXPTType::T_IID:
    case nsXPTType::T_ASTRING:
    case nsXPTType::T_DOMSTRING:
    case nsXPTType::T_UTF8STRING:
    case nsXPTType::T_CSTRING:
      aRetvalSig.AppendLiteral("Ljava/lang/String;");
      break;

    case nsXPTType::T_INTERFACE:
    case nsXPTType::T_INTERFACE_IS:
      aRetvalSig.AppendLiteral("Lorg/mozilla/xpcom/nsISupports;");
      break;

    case nsXPTType::T_VOID:
      aRetvalSig.Append('I');
      break;

    case nsXPTType::T_ARRAY:
      NS_WARNING("array types are not yet supported");
      return NS_ERROR_NOT_IMPLEMENTED;
      break;

    case nsXPTType::T_PSTRING_SIZE_IS:
    case nsXPTType::T_PWSTRING_SIZE_IS:
    default:
      NS_WARNING("unexpected parameter type");
      return NS_ERROR_UNEXPECTED;
  }

  return NS_OK;
}

/**
 * Handle 'inout', 'out', and 'retval' params
 */
nsresult
nsJavaXPTCStub::FinalizeJavaParams(const nsXPTParamInfo &aParamInfo,
                                 const nsXPTMethodInfo* aMethodInfo,
                                 PRUint16 aMethodIndex,
                                 nsXPTCMiniVariant* aDispatchParams,
                                 nsXPTCMiniVariant &aVariant, jvalue &aJValue)
{
  nsresult rv = NS_OK;
  const nsXPTType &type = aParamInfo.GetType();

  PRUint8 tag = type.TagPart();
  switch (tag)
  {
    case nsXPTType::T_I8:
    case nsXPTType::T_U8:
    {
      if (aParamInfo.IsRetval()) {  // 'retval'
        *((PRUint8 *) aVariant.val.p) = aJValue.b;
      } else {  // 'inout' & 'out'
        if (aJValue.l) {
          mJavaEnv->GetByteArrayRegion((jbyteArray) aJValue.l, 0, 1,
                                       (jbyte*) aVariant.val.p);
        }
      }
    }
    break;

    case nsXPTType::T_I16:
    case nsXPTType::T_U16:
    {
      if (aParamInfo.IsRetval()) {  // 'retval'
        *((PRUint16 *) aVariant.val.p) = aJValue.s;
      } else {  // 'inout' & 'out'
        if (aJValue.l) {
          mJavaEnv->GetShortArrayRegion((jshortArray) aJValue.l, 0, 1,
                                        (jshort*) aVariant.val.p);
        }
      }
    }
    break;

    case nsXPTType::T_I32:
    case nsXPTType::T_U32:
    {
      if (aParamInfo.IsRetval()) {  // 'retval'
        *((PRUint32 *) aVariant.val.p) = aJValue.i;
      } else {  // 'inout' & 'out'
        if (aJValue.l) {
          mJavaEnv->GetIntArrayRegion((jintArray) aJValue.l, 0, 1,
                                      (jint*) aVariant.val.p);
        }
      }
    }
    break;

    case nsXPTType::T_I64:
    case nsXPTType::T_U64:
    {
      if (aParamInfo.IsRetval()) {  // 'retval'
        *((PRUint64 *) aVariant.val.p) = aJValue.j;
      } else {  // 'inout' & 'out'
        if (aJValue.l) {
          mJavaEnv->GetLongArrayRegion((jlongArray) aJValue.l, 0, 1,
                                       (jlong*) aVariant.val.p);
        }
      }
    }
    break;

    case nsXPTType::T_FLOAT:
    {
      if (aParamInfo.IsRetval()) {  // 'retval'
        *((float *) aVariant.val.p) = aJValue.f;
      } else {  // 'inout' & 'out'
        if (aJValue.l) {
          mJavaEnv->GetFloatArrayRegion((jfloatArray) aJValue.l, 0, 1,
                                        (jfloat*) aVariant.val.p);
        }
      }
    }
    break;

    case nsXPTType::T_DOUBLE:
    {
      if (aParamInfo.IsRetval()) {  // 'retval'
        *((double *) aVariant.val.p) = aJValue.d;
      } else {  // 'inout' & 'out'
        if (aJValue.l) {
          mJavaEnv->GetDoubleArrayRegion((jdoubleArray) aJValue.l, 0, 1,
                                         (jdouble*) aVariant.val.p);
        }
      }
    }
    break;

    case nsXPTType::T_BOOL:
    {
      if (aParamInfo.IsRetval()) {  // 'retval'
        *((PRBool *) aVariant.val.p) = aJValue.z;
      } else {  // 'inout' & 'out'
        if (aJValue.l) {
          mJavaEnv->GetBooleanArrayRegion((jbooleanArray) aJValue.l, 0, 1,
                                          (jboolean*) aVariant.val.p);
        }
      }
    }
    break;

    case nsXPTType::T_CHAR:
    case nsXPTType::T_WCHAR:
    {
      if (aParamInfo.IsRetval()) {  // 'retval'
        if (type.TagPart() == nsXPTType::T_CHAR)
          *((char *) aVariant.val.p) = aJValue.c;
        else
          *((PRUnichar *) aVariant.val.p) = aJValue.c;
      } else {  // 'inout' & 'out'
        if (aJValue.l) {
          jchar* array = mJavaEnv->GetCharArrayElements((jcharArray) aJValue.l,
                                                        nsnull);
          if (!array) {
            rv = NS_ERROR_OUT_OF_MEMORY;
            break;
          }

          if (type.TagPart() == nsXPTType::T_CHAR)
            *((char *) aVariant.val.p) = array[0];
          else
            *((PRUnichar *) aVariant.val.p) = array[0];

          mJavaEnv->ReleaseCharArrayElements((jcharArray) aJValue.l, array,
                                             JNI_ABORT);
        }
      }
    }
    break;

    case nsXPTType::T_CHAR_STR:
    {
      jstring str = nsnull;
      if (aParamInfo.IsRetval()) {  // 'retval'
        str = (jstring) aJValue.l;
      } else {  // 'inout' & 'out'
        str = (jstring)
                 mJavaEnv->GetObjectArrayElement((jobjectArray) aJValue.l, 0);
      }

      char** variant = NS_STATIC_CAST(char**, aVariant.val.p);
      if (str) {
        // Get string buffer
        const char* char_ptr = mJavaEnv->GetStringUTFChars(str, nsnull);
        if (!char_ptr) {
          rv = NS_ERROR_OUT_OF_MEMORY;
          break;
        }

        // If new string is different from one passed in, free old string
        // and replace with new string.
        if (aParamInfo.IsRetval() ||
            *variant == nsnull || strcmp(*variant, char_ptr) != 0)
        {
          if (!aParamInfo.IsRetval() && *variant)
            PR_Free(*variant);

          *variant = strdup(char_ptr);
          if (*variant == nsnull) {
            rv = NS_ERROR_OUT_OF_MEMORY;
            // don't 'break'; fall through to release chars
          }
        }

        // Release string buffer
        mJavaEnv->ReleaseStringUTFChars(str, char_ptr);
      } else {
        // If we were passed in a string, delete it now, and set to null.
        // (Only for 'inout' & 'out' params)
        if (*variant && !aParamInfo.IsRetval()) {
          PR_Free(*variant);
        }
        *variant = nsnull;
      }
    }
    break;

    case nsXPTType::T_WCHAR_STR:
    {
      jstring str = nsnull;
      if (aParamInfo.IsRetval()) {  // 'retval'
        str = (jstring) aJValue.l;
      } else {  // 'inout' & 'out'
        str = (jstring)
                 mJavaEnv->GetObjectArrayElement((jobjectArray) aJValue.l, 0);
      }

      PRUnichar** variant = NS_STATIC_CAST(PRUnichar**, aVariant.val.p);
      if (str) {
        // Get string buffer
        const jchar* wchar_ptr = mJavaEnv->GetStringChars(str, nsnull);
        if (!wchar_ptr) {
          rv = NS_ERROR_OUT_OF_MEMORY;
          break;
        }

        // If new string is different from one passed in, free old string
        // and replace with new string.  We 
        if (aParamInfo.IsRetval() ||
            *variant == nsnull || nsCRT::strcmp(*variant, wchar_ptr) != 0)
        {
          if (!aParamInfo.IsRetval() && *variant)
            PR_Free(*variant);

          PRUint32 length = nsCRT::strlen(wchar_ptr);
          *variant = (PRUnichar*) PR_Malloc((length + 1) * sizeof(PRUnichar));
          if (*variant) {
            memcpy(*variant, wchar_ptr, length * sizeof(PRUnichar));
            (*variant)[length] = 0;
          } else {
            rv = NS_ERROR_OUT_OF_MEMORY;
            // don't 'break'; fall through to release chars
          }
        }

        // Release string buffer
        mJavaEnv->ReleaseStringChars(str, wchar_ptr);
      } else {
        // If we were passed in a string, delete it now, and set to null.
        // (Only for 'inout' & 'out' params)
        if (*variant && !aParamInfo.IsRetval()) {
          PR_Free(*variant);
        }
        *variant = nsnull;
      }
    }
    break;

    case nsXPTType::T_IID:
    {
      jstring str = nsnull;
      if (aParamInfo.IsRetval()) {  // 'retval'
        str = (jstring) aJValue.l;
      } else {  // 'inout' & 'out'
        str = (jstring)
                 mJavaEnv->GetObjectArrayElement((jobjectArray) aJValue.l, 0);
      }

      nsID** variant = NS_STATIC_CAST(nsID**, aVariant.val.p);
      if (str) {
        // Get string buffer
        const char* char_ptr = mJavaEnv->GetStringUTFChars(str, nsnull);
        if (!char_ptr) {
          rv = NS_ERROR_OUT_OF_MEMORY;
          break;
        }

        if (!aParamInfo.IsRetval() && *variant) {
          // If we were given an nsID, set it to the new string
          nsID* oldIID = *variant;
          oldIID->Parse(char_ptr);
        } else {
          // If the argument that was passed in was null, then we need to
          // create a new nsID.
          nsID* newIID = new nsID;
          if (newIID) {
            newIID->Parse(char_ptr);
            *variant = newIID;
          } else {
            rv = NS_ERROR_OUT_OF_MEMORY;
            // don't 'break'; fall through to release chars
          }
        }

        // Release string buffer
        mJavaEnv->ReleaseStringUTFChars(str, char_ptr);
      } else {
        // If we were passed in an nsID, delete it now, and set to null.
        // (Free only 'inout' & 'out' params)
        if (*variant && !aParamInfo.IsRetval()) {
          delete *variant;
        }
        *variant = nsnull;
      }
    }
    break;

    case nsXPTType::T_INTERFACE:
    case nsXPTType::T_INTERFACE_IS:
    {
      jobject java_obj = nsnull;
      if (aParamInfo.IsRetval()) {  // 'retval'
        java_obj = aJValue.l;
      } else if (aJValue.l) {  // 'inout' & 'out'
        java_obj = mJavaEnv->GetObjectArrayElement((jobjectArray) aJValue.l, 0);
      }

      nsISupports** variant = NS_STATIC_CAST(nsISupports**, aVariant.val.p);
      if (java_obj) {
        // Check if we already have a corresponding XPCOM object
        jboolean isProxy;
        isProxy = mJavaEnv->CallStaticBooleanMethod(xpcomJavaProxyClass,
                                                    isXPCOMJavaProxyMID,
                                                    java_obj);
        if (mJavaEnv->ExceptionCheck()) {
          rv = NS_ERROR_FAILURE;
          break;
        }

        void* inst;
        if (isProxy) {
          rv = GetXPCOMInstFromProxy(mJavaEnv, java_obj, &inst);
          if (NS_FAILED(rv))
            break;
        } else {
          inst = gBindings->GetXPCOMObject(mJavaEnv, java_obj);
        }

        // Get IID for this param
        nsID iid;
        rv = GetIIDForMethodParam(mIInfo, aMethodInfo, aParamInfo,
                                  aMethodIndex, aDispatchParams,
                                  PR_FALSE, iid);
        if (NS_FAILED(rv))
          break;

        PRBool isWeakRef = iid.Equals(NS_GET_IID(nsIWeakReference));

        if (inst == nsnull && !isWeakRef) {
          // Get interface info for class
          nsCOMPtr<nsIInterfaceInfoManager> iim = XPTI_GetInterfaceInfoManager();
          nsCOMPtr<nsIInterfaceInfo> iinfo;
          rv = iim->GetInfoForIID(&iid, getter_AddRefs(iinfo));
          if (NS_FAILED(rv))
            break;

          // Create XPCOM stub
          nsJavaXPTCStub* xpcomStub;
          xpcomStub = new nsJavaXPTCStub(mJavaEnv, java_obj, iinfo);
          if (!xpcomStub) {
            rv = NS_ERROR_OUT_OF_MEMORY;
            break;
          }
          inst = SetAsXPTCStub(xpcomStub);
          rv = gBindings->AddBinding(mJavaEnv, java_obj, inst);
          if (NS_FAILED(rv))
            break;
        }

        // Get appropriate XPCOM object
        void* xpcom_obj;
        if (isWeakRef) {
          // If the function expects an weak reference, then we need to
          // create it here.
          nsJavaXPTCStubWeakRef* weakref;
          weakref = new nsJavaXPTCStubWeakRef(mJavaEnv, java_obj);
          if (!weakref) {
            rv = NS_ERROR_OUT_OF_MEMORY;
            break;
          }
          NS_ADDREF(weakref);
          xpcom_obj = weakref;
        } else if (IsXPTCStub(inst)) {
          nsJavaXPTCStub* xpcomStub = GetXPTCStubAddr(inst);
          NS_ADDREF(xpcomStub);
          xpcom_obj = xpcomStub;
        } else {
          JavaXPCOMInstance* xpcomInst = (JavaXPCOMInstance*) inst;
          xpcom_obj = xpcomInst->GetInstance();
        }

        if (*variant && !aParamInfo.IsRetval()) {
          NS_RELEASE(*variant);
        }
        *variant = (nsISupports*) xpcom_obj;
      } else {
        // If were passed in an object, release it now, and set to null.
        if (*variant && !aParamInfo.IsRetval()) {
          NS_RELEASE(*variant);
        }
        *variant = nsnull;
      }
    }
    break;

    case nsXPTType::T_ASTRING:
    case nsXPTType::T_DOMSTRING:
    {
      jstring str = nsnull;
      if (aParamInfo.IsRetval()) {  // 'retval'
        str = (jstring) aJValue.l;
      } else {  // 'inout' & 'out'
        str = (jstring)
                 mJavaEnv->GetObjectArrayElement((jobjectArray) aJValue.l, 0);
      }

      nsString** variant = NS_STATIC_CAST(nsString**, aVariant.val.p);
      if (str) {
        // Get string buffer
        const jchar* wchar_ptr = mJavaEnv->GetStringChars(str, nsnull);
        if (!wchar_ptr) {
          rv = NS_ERROR_OUT_OF_MEMORY;
          break;
        }

        if (!aParamInfo.IsRetval() && *variant) {
          // If we were given an nsString, set it to the new string
          nsString* string = *variant;
          string->Assign(wchar_ptr);
        } else {
          // If the argument that was passed in was null, then we need to
          // create a new nsID.
          nsString* embedStr = new nsString(wchar_ptr);
          if (embedStr) {
            *variant = embedStr;
          } else {
            rv = NS_ERROR_OUT_OF_MEMORY;
            // don't 'break'; fall through to release chars
          }
        }

        // release String buffer
        mJavaEnv->ReleaseStringChars(str, wchar_ptr);
      } else {
        // If we were passed in a string, delete it now, and set to null.
        // (Free only 'inout' & 'out' params)
        if (*variant && !aParamInfo.IsRetval()) {
          delete *variant;
        }
        *variant = nsnull;
      }
    }
    break;

    case nsXPTType::T_UTF8STRING:
    case nsXPTType::T_CSTRING:
    {
      jstring str = nsnull;
      if (aParamInfo.IsRetval()) {  // 'retval'
        str = (jstring) aJValue.l;
      } else {  // 'inout' & 'out'
        str = (jstring)
                 mJavaEnv->GetObjectArrayElement((jobjectArray) aJValue.l, 0);
      }

      nsCString** variant = NS_STATIC_CAST(nsCString**, aVariant.val.p);
      if (str) {
        // Get string buffer
        const char* char_ptr = mJavaEnv->GetStringUTFChars(str, nsnull);
        if (!char_ptr) {
          rv = NS_ERROR_OUT_OF_MEMORY;
          break;
        }

        if (!aParamInfo.IsRetval() && *variant) {
          // If we were given an nsString, set it to the new string
          nsCString* string = *variant;
          string->Assign(char_ptr);
        } else {
          // If the argument that was passed in was null, then we need to
          // create a new nsID.
          nsCString* embedStr = new nsCString(char_ptr);
          if (embedStr) {
            *variant = embedStr;
          } else {
            rv = NS_ERROR_OUT_OF_MEMORY;
            // don't 'break'; fall through to release chars
          }
        }

        // release String buffer
        mJavaEnv->ReleaseStringUTFChars(str, char_ptr);
      } else {
        // If we were passed in a string, delete it now, and set to null.
        // (Free only 'inout' & 'out' params)
        if (*variant && !aParamInfo.IsRetval()) {
          delete *variant;
        }
        *variant = nsnull;
      }
    }
    break;

    case nsXPTType::T_VOID:
    {
      if (aParamInfo.IsRetval()) {  // 'retval'
        *((PRUint32 *) aVariant.val.p) = aJValue.i;
      } else {  // 'inout' & 'out'
        if (aJValue.l) {
          mJavaEnv->GetIntArrayRegion((jintArray) aJValue.l, 0, 1,
                                      (jint*) aVariant.val.p);
        }
      }
    }
    break;

    default:
      NS_WARNING("unexpected parameter type");
      return NS_ERROR_UNEXPECTED;
  }

  return rv;
}

NS_IMETHODIMP
nsJavaXPTCStub::GetWeakReference(nsIWeakReference** aInstancePtr)
{
  if (mMaster)
    return mMaster->GetWeakReference(aInstancePtr);

  LOG(("==> nsJavaXPTCStub::GetWeakReference()\n"));

  if (!aInstancePtr)
    return NS_ERROR_NULL_POINTER;

  nsJavaXPTCStubWeakRef* weakref;
  weakref = new nsJavaXPTCStubWeakRef(mJavaEnv, mJavaObject);
  if (!weakref)
    return NS_ERROR_OUT_OF_MEMORY;

  *aInstancePtr = weakref;
  NS_ADDREF(*aInstancePtr);

  return NS_OK;
}

jobject
nsJavaXPTCStub::GetJavaObject()
{
  return mJavaObject;
}
