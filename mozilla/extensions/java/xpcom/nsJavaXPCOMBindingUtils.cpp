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


#include "nsJavaXPCOMBindingUtils.h"
#include "nsJavaXPTCStub.h"
#include "jni.h"
#include "nsIInterfaceInfoManager.h"
#include "pldhash.h"
#include "nsILocalFile.h"
#include "nsEventQueueUtils.h"
#include "nsProxyRelease.h"


#ifdef DEBUG
extern PRBool gEmbeddingInitialized;
#endif

/* Java JNI globals */
jclass classClass = nsnull;
jmethodID getNameMID = nsnull;

jclass objectClass = nsnull;
jmethodID hashCodeMID = nsnull;

jclass booleanClass = nsnull;
jclass booleanArrayClass = nsnull;
jmethodID booleanInitMID = nsnull;
jmethodID booleanValueMID = nsnull;

jclass charClass = nsnull;
jclass charArrayClass = nsnull;
jmethodID charInitMID = nsnull;
jmethodID charValueMID = nsnull;

jclass byteClass = nsnull;
jclass byteArrayClass = nsnull;
jmethodID byteInitMID = nsnull;
jmethodID byteValueMID = nsnull;

jclass shortClass = nsnull;
jclass shortArrayClass = nsnull;
jmethodID shortInitMID = nsnull;
jmethodID shortValueMID = nsnull;

jclass intClass = nsnull;
jclass intArrayClass = nsnull;
jmethodID intInitMID = nsnull;
jmethodID intValueMID = nsnull;

jclass longClass = nsnull;
jclass longArrayClass = nsnull;
jmethodID longInitMID = nsnull;
jmethodID longValueMID = nsnull;

jclass floatClass = nsnull;
jclass floatArrayClass = nsnull;
jmethodID floatInitMID = nsnull;
jmethodID floatValueMID = nsnull;

jclass doubleClass = nsnull;
jclass doubleArrayClass = nsnull;
jmethodID doubleInitMID = nsnull;
jmethodID doubleValueMID = nsnull;

jclass stringClass = nsnull;
jclass stringArrayClass = nsnull;

jclass nsISupportsClass = nsnull;

jclass xpcomExceptionClass = nsnull;

/**************************************
 *  Java<->XPCOM binding stores
 **************************************/
class JavaXPCOMBindingEntry : public PLDHashEntryHdr
{
public:
  // mKey will either be a Java hash of the Java object, or the address of
  // the XPCOM object, depending on which hash table this entry is used in.
  const void*   mKey;
  jobject mJavaObject;
  void*   mXPCOMInstance;
};

static PLDHashTable *gJAVAtoXPCOMBindings = nsnull;
static PLDHashTable *gXPCOMtoJAVABindings = nsnull;

PR_STATIC_CALLBACK(PRBool)
InitJavaXPCOMBindingEntry(PLDHashTable *table, PLDHashEntryHdr *entry,
                          const void *key)
{
  JavaXPCOMBindingEntry *e =
    NS_CONST_CAST(JavaXPCOMBindingEntry *,
                  NS_STATIC_CAST(const JavaXPCOMBindingEntry *, entry));

  e->mKey = key;

  return PR_TRUE;
}

void
AddJavaXPCOMBinding(JNIEnv* env, jobject aJavaObject, void* aXPCOMObject)
{
  // We use a Java hash of the Java object as a key since the JVM can return
  // different "addresses" for the same Java object, but the hash code (the
  // result of calling |hashCode()| on the Java object) will always be the same.
  jint hash = env->CallIntMethod(aJavaObject, hashCodeMID);
  jweak java_ref = env->NewWeakGlobalRef(aJavaObject);

  JavaXPCOMBindingEntry *entry =
    NS_STATIC_CAST(JavaXPCOMBindingEntry*,
                   PL_DHashTableOperate(gJAVAtoXPCOMBindings,
                                        NS_INT32_TO_PTR(hash),
                                        PL_DHASH_ADD));
  entry->mJavaObject = java_ref;
  entry->mXPCOMInstance = aXPCOMObject;

  // We want the hash key to be the actual XPCOM object
  void* xpcomObjKey = nsnull;
  if (IsXPTCStub(aXPCOMObject))
    xpcomObjKey = GetXPTCStubAddr(aXPCOMObject);
  else
    xpcomObjKey = ((JavaXPCOMInstance*) aXPCOMObject)->GetInstance();

  entry =
    NS_STATIC_CAST(JavaXPCOMBindingEntry*,
                   PL_DHashTableOperate(gXPCOMtoJAVABindings, xpcomObjKey,
                                        PL_DHASH_ADD));
  entry->mJavaObject = java_ref;
  entry->mXPCOMInstance = aXPCOMObject;

  LOG("+ Adding Java<->XPCOM binding (Java=0x%08x | XPCOM=0x%08x) weakref=0x%08x\n",
         hash, (int) xpcomObjKey, (PRUint32) java_ref);
}

void
RemoveJavaXPCOMBinding(JNIEnv* env, jobject aJavaObject, void* aXPCOMObject)
{
  // We either get a Java or an XPCOM object.  So find the other object.
  jint hash = 0;
  JavaXPCOMBindingEntry* entry = nsnull;

  if (aJavaObject) {
    hash = env->CallIntMethod(aJavaObject, hashCodeMID);
    JavaXPCOMBindingEntry* e =
      NS_STATIC_CAST(JavaXPCOMBindingEntry*,
                     PL_DHashTableOperate(gJAVAtoXPCOMBindings,
                                          NS_INT32_TO_PTR(hash),
                                          PL_DHASH_LOOKUP));
    if (PL_DHASH_ENTRY_IS_BUSY(e))
      entry = e;
  } else {
    JavaXPCOMBindingEntry* e =
      NS_STATIC_CAST(JavaXPCOMBindingEntry*,
                     PL_DHashTableOperate(gXPCOMtoJAVABindings, aXPCOMObject,
                                          PL_DHASH_LOOKUP));
    if (PL_DHASH_ENTRY_IS_BUSY(e))
      entry = e;
  }
  NS_ASSERTION(entry != nsnull, "Failed to find matching entry");

  if (entry) {
    jobject jweakref = entry->mJavaObject;
    void* xpcom_obj = entry->mXPCOMInstance;

    // Get keys.  For Java, we use the hash key of the Java object.  For XPCOM,
    // we get the 'actual' object (either the nsJavaXPTCStub or the instance
    // that is wrapped by JavaXPCOMInstance).
    void* xpcomObjKey = nsnull;
    if (hash == 0)
      hash = env->CallIntMethod(jweakref, hashCodeMID);
    if (aXPCOMObject) {
      xpcomObjKey = aXPCOMObject;
    } else {
      if (IsXPTCStub(xpcom_obj))
        xpcomObjKey = GetXPTCStubAddr(xpcom_obj);
      else
        xpcomObjKey = ((JavaXPCOMInstance*) xpcom_obj)->GetInstance();
    }

    NS_ASSERTION(env->IsSameObject(jweakref, aJavaObject),
                 "Weakref does not point to expected Java object");
    NS_ASSERTION(!env->IsSameObject(jweakref, NULL),
                 "Weakref refers to garbage collected Java object. No longer valid");

    // Remove both instances from stores
    PL_DHashTableOperate(gJAVAtoXPCOMBindings, NS_INT32_TO_PTR(hash),
                         PL_DHASH_REMOVE);
    PL_DHashTableOperate(gXPCOMtoJAVABindings, xpcomObjKey, PL_DHASH_REMOVE);
    LOG("- Removing Java<->XPCOM binding (Java=0x%08x | XPCOM=0x%08x) weakref=0x%08x\n",
        hash, (int) xpcomObjKey, (PRUint32) jweakref);

    env->DeleteWeakGlobalRef(NS_STATIC_CAST(jweak, jweakref));
  }
}

void*
GetMatchingXPCOMObject(JNIEnv* env, jobject aJavaObject)
{
  jint hash = env->CallIntMethod(aJavaObject, hashCodeMID);

  JavaXPCOMBindingEntry *entry =
    NS_STATIC_CAST(JavaXPCOMBindingEntry*,
                   PL_DHashTableOperate(gJAVAtoXPCOMBindings,
                                        NS_INT32_TO_PTR(hash),
                                        PL_DHASH_LOOKUP));

  if (PL_DHASH_ENTRY_IS_BUSY(entry)) {
#ifdef DEBUG
    void* xpcomObjKey = nsnull;
    if (IsXPTCStub(entry->mXPCOMInstance))
      xpcomObjKey = GetXPTCStubAddr(entry->mXPCOMInstance);
    else
      xpcomObjKey = ((JavaXPCOMInstance*) entry->mXPCOMInstance)->GetInstance();
    LOG("< Get Java<->XPCOM binding (Java=0x%08x | XPCOM=0x%08x)\n",
           hash, (int) xpcomObjKey);
#endif
    return entry->mXPCOMInstance;
  }

  return nsnull;
}

jobject
GetMatchingJavaObject(JNIEnv* env, void* aXPCOMObject)
{
  jobject java_obj = nsnull;
  nsISupports* xpcom_obj = NS_STATIC_CAST(nsISupports*, aXPCOMObject);

  nsJavaXPTCStub* stub = nsnull;
  xpcom_obj->QueryInterface(NS_GET_IID(nsJavaXPTCStub), (void**) &stub);
  if (stub) {
    java_obj = stub->GetJavaObject();
    NS_ASSERTION(java_obj != nsnull, "nsJavaXPTCStub w/o matching Java object");
    NS_RELEASE(stub);
  }

  if (java_obj == nsnull) {
    JavaXPCOMBindingEntry *entry =
      NS_STATIC_CAST(JavaXPCOMBindingEntry*,
                     PL_DHashTableOperate(gXPCOMtoJAVABindings, aXPCOMObject,
                                          PL_DHASH_LOOKUP));

    if (PL_DHASH_ENTRY_IS_BUSY(entry)) {
      java_obj = entry->mJavaObject;
    }
  }

  if (java_obj) {
    LOG("< Get Java<->XPCOM binding (Java=0x%08x | XPCOM=0x%08x)\n",
        env->CallIntMethod(java_obj, hashCodeMID), (int) aXPCOMObject);
  }

  return java_obj;
}


/******************************
 *  InitializeJavaGlobals
 ******************************/
PRBool
InitializeJavaGlobals(JNIEnv *env)
{
  static PRBool initialized = PR_FALSE;
  if (!initialized)
  {
    jclass clazz;
    if (!(clazz = env->FindClass("java/lang/Class")) ||
        !(classClass = (jclass) env->NewGlobalRef(clazz)))
    {
        return PR_FALSE;
    }
  
    if (!(clazz = env->FindClass("java/lang/Object")) ||
        !(objectClass = (jclass) env->NewGlobalRef(clazz)))
    {
        return PR_FALSE;
    }
  
    if (!(clazz = env->FindClass("java/lang/Boolean")) ||
        !(booleanClass = (jclass) env->NewGlobalRef(clazz)) ||
        !(clazz = env->FindClass("[Z")) ||
        !(booleanArrayClass = (jclass) env->NewGlobalRef(clazz)))
    {
        return PR_FALSE;
    }
  
    if (!(clazz = env->FindClass("java/lang/Character")) ||
        !(charClass = (jclass) env->NewGlobalRef(clazz)) ||
        !(clazz = env->FindClass("[C")) ||
        !(charArrayClass = (jclass) env->NewGlobalRef(clazz)))
    {
        return PR_FALSE;
    }
    if (!(clazz = env->FindClass("java/lang/Byte")) ||
        !(byteClass = (jclass) env->NewGlobalRef(clazz)) ||
        !(clazz = env->FindClass("[B")) ||
        !(byteArrayClass = (jclass) env->NewGlobalRef(clazz)))
    {
        return PR_FALSE;
    }
    if (!(clazz = env->FindClass("java/lang/Short")) ||
        !(shortClass = (jclass) env->NewGlobalRef(clazz)) ||
        !(clazz = env->FindClass("[[S")) ||
        !(shortArrayClass = (jclass) env->NewGlobalRef(clazz)))
    {
        return PR_FALSE;
    }
    if (!(clazz = env->FindClass("java/lang/Integer")) ||
        !(intClass = (jclass) env->NewGlobalRef(clazz)) ||
        !(clazz = env->FindClass("[I")) ||
        !(intArrayClass = (jclass) env->NewGlobalRef(clazz)))
    {
        return PR_FALSE;
    }
    if (!(clazz = env->FindClass("java/lang/Long")) ||
        !(longClass = (jclass) env->NewGlobalRef(clazz)) ||
        !(clazz = env->FindClass("[J")))
    {
        return PR_FALSE;
    }
    if (!(clazz = env->FindClass("java/lang/Float")) ||
        !(floatClass = (jclass) env->NewGlobalRef(clazz)) ||
        !(clazz = env->FindClass("[F")) ||
        !(floatArrayClass = (jclass) env->NewGlobalRef(clazz)))
    {
        return PR_FALSE;
    }
    if (!(clazz = env->FindClass("java/lang/Double")) ||
        !(doubleClass = (jclass) env->NewGlobalRef(clazz)) ||
        !(clazz = env->FindClass("[D")) ||
        !(doubleArrayClass = (jclass) env->NewGlobalRef(clazz)))
    {
        return PR_FALSE;
    }
  
    if (!(clazz = env->FindClass("java/lang/String")) ||
        !(stringClass = (jclass) env->NewGlobalRef(clazz)) ||
        !(clazz = env->FindClass("[Ljava/lang/String;")) ||
        !(stringArrayClass = (jclass) env->NewGlobalRef(clazz)))
    {
        return PR_FALSE;
    }
  
    if (!(clazz = env->FindClass("org/mozilla/xpcom/nsISupports")) ||
        !(nsISupportsClass = (jclass) env->NewGlobalRef(clazz)))
    {
        return PR_FALSE;
    }

    if (!(clazz = env->FindClass("org/mozilla/xpcom/XPCOMException")) ||
        !(xpcomExceptionClass = (jclass) env->NewGlobalRef(clazz)))
    {
        return PR_FALSE;
    }

    if (!(hashCodeMID = env->GetMethodID(objectClass, "hashCode","()I"))) {
        return PR_FALSE;
    }
  
    if (!(booleanInitMID = env->GetMethodID(booleanClass,"<init>","(Z)V"))) {
        return PR_FALSE;
    }
    if (!(booleanValueMID = env->GetMethodID(booleanClass,"booleanValue","()Z"))) {
        return PR_FALSE;
    }
  
    if (!(charInitMID = env->GetMethodID(charClass,"<init>","(C)V"))) {
        return PR_FALSE;
    }
    if (!(charValueMID = env->GetMethodID(charClass,"charValue","()C"))) {
        return PR_FALSE;
    }
  
    if (!(byteInitMID = env->GetMethodID(byteClass,"<init>","(B)V"))) {
        return PR_FALSE;
    }
    if (!(byteValueMID = env->GetMethodID(byteClass,"byteValue","()B"))) {
        return PR_FALSE;
    }
    if (!(shortInitMID = env->GetMethodID(shortClass,"<init>","(S)V"))) {
        return PR_FALSE;
    }
    if (!(shortValueMID = env->GetMethodID(shortClass,"shortValue","()S"))) {
        return PR_FALSE;
    }
  
    if (!(intInitMID = env->GetMethodID(intClass,"<init>","(I)V"))) {
        return PR_FALSE;
    }
    if (!(intValueMID = env->GetMethodID(intClass,"intValue","()I"))) {
        return PR_FALSE;
    }
  
    if (!(longInitMID = env->GetMethodID(longClass,"<init>","(J)V"))) {
        return PR_FALSE;
    }
    if (!(longValueMID = env->GetMethodID(longClass,"longValue","()J"))) {
        return PR_FALSE;
    }
  
    if (!(floatInitMID = env->GetMethodID(floatClass,"<init>","(F)V"))) {
        return PR_FALSE;
    }
    if (!(floatValueMID = env->GetMethodID(floatClass,"floatValue","()F"))) {
        return PR_FALSE;
    }
  
    if (!(doubleInitMID = env->GetMethodID(doubleClass,"<init>","(D)V"))) {
        return PR_FALSE;
    }
    if (!(doubleValueMID = env->GetMethodID(doubleClass,"doubleValue","()D"))) {
        return PR_FALSE;
    }
  
    if (!(getNameMID = env->GetMethodID(classClass, "getName","()Ljava/lang/String;"))) {
        return PR_FALSE;
    }
  
    static PLDHashTableOps java_to_xpcom_hash_ops =
    {
      PL_DHashAllocTable,
      PL_DHashFreeTable,
      PL_DHashGetKeyStub,
      PL_DHashVoidPtrKeyStub,
      PL_DHashMatchEntryStub,
      PL_DHashMoveEntryStub,
      PL_DHashClearEntryStub,
      PL_DHashFinalizeStub,
      InitJavaXPCOMBindingEntry
    };
  
    gJAVAtoXPCOMBindings = PL_NewDHashTable(&java_to_xpcom_hash_ops, nsnull,
                                            sizeof(JavaXPCOMBindingEntry), 16);
    if (!gJAVAtoXPCOMBindings) {
      return PR_FALSE;
    }
  
    static PLDHashTableOps xpcom_to_java_hash_ops =
    {
      PL_DHashAllocTable,
      PL_DHashFreeTable,
      PL_DHashGetKeyStub,
      PL_DHashVoidPtrKeyStub,
      PL_DHashMatchEntryStub,
      PL_DHashMoveEntryStub,
      PL_DHashClearEntryStub,
      PL_DHashFinalizeStub,
      InitJavaXPCOMBindingEntry
    };
  
    gXPCOMtoJAVABindings = PL_NewDHashTable(&xpcom_to_java_hash_ops, nsnull,
                                            sizeof(JavaXPCOMBindingEntry), 16);
    if (!gXPCOMtoJAVABindings) {
      return PR_FALSE;
    }

    initialized = PR_TRUE;
  }

  return PR_TRUE;
}

/*************************
 *    FreeJavaGlobals
 *************************/
void
FreeJavaGlobals(JNIEnv* env)
{
  // XXX Need to write
}


/**********************************************************
 *    JavaXPCOMInstance
 *********************************************************/
JavaXPCOMInstance::JavaXPCOMInstance(nsISupports* aInstance,
                                     nsIInterfaceInfo* aIInfo)
		: mInstance(aInstance),
		  mIInfo(aIInfo)
{
  NS_ADDREF(mInstance);
}

JavaXPCOMInstance::~JavaXPCOMInstance()
{
  nsCOMPtr<nsIEventQueue> eventQ;
  nsresult rv = NS_GetMainEventQ(getter_AddRefs(eventQ));
  if (NS_SUCCEEDED(rv))
    rv = NS_ProxyRelease(eventQ, mInstance);
  NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to get MainEventQ");
}

nsIInterfaceInfo*
JavaXPCOMInstance::InterfaceInfo()
{
  // We lazily create the interfaceInfo for nsILocalFile.
  if (!mIInfo) {
    NS_ASSERTION(gEmbeddingInitialized, "Trying to create interface info, but XPCOM not inited");

    // Get interface info for class
    nsCOMPtr<nsIInterfaceInfoManager> iim = XPTI_GetInterfaceInfoManager();
    NS_ASSERTION(iim != nsnull, "Failed to get InterfaceInfoManager");
    if (iim) {
      iim->GetInfoForIID(&NS_GET_IID(nsILocalFile), getter_AddRefs(mIInfo));
    }
  }

  NS_ASSERTION(mIInfo, "No interfaceInfo for JavaXPCOMInstance");
  return mIInfo;
}

JavaXPCOMInstance*
CreateJavaXPCOMInstance(nsISupports* aXPCOMObject, const nsIID* aIID)
{
  JavaXPCOMInstance* inst = nsnull;

  // We can't call XPTI_GetInterfaceInfoManager() before NS_InitEmbedding(),
  // so for NS_NewLocalFile (which can be called before NS_InitEmbedding), we
  // pass in a null aIID, and create the interface info lazily later.
  if (!aIID) {
    inst = new JavaXPCOMInstance(aXPCOMObject, nsnull);
  } else {
    // Get interface info for class
    nsCOMPtr<nsIInterfaceInfoManager> iim = XPTI_GetInterfaceInfoManager();
    NS_ASSERTION(iim != nsnull, "Failed to get InterfaceInfoManager");
    if (iim) {
      nsCOMPtr<nsIInterfaceInfo> info;
      iim->GetInfoForIID(aIID, getter_AddRefs(info));

      // Wrap XPCOM object
      inst = new JavaXPCOMInstance(aXPCOMObject, info);
    }
  }

  return inst;
}


void
ThrowXPCOMException(JNIEnv* env, int aFailureCode)
{
  // Only throw this exception if one hasn't already been thrown, so we don't
  // mask a previous exception/error.
  jthrowable throwObj = env->ExceptionOccurred();
  if (throwObj == nsnull) {
    char exp_msg[40];
    sprintf(exp_msg, "\nInternal XPCOM Error: %x", aFailureCode);
    jint rc = env->ThrowNew(xpcomExceptionClass, exp_msg);
    NS_ASSERTION(rc == 0, "Failed to throw XPCOMException");
  }
}

nsresult
GetIIDForMethodParam(nsIInterfaceInfo *iinfo,
                                         const nsXPTMethodInfo *methodInfo,
                                         const nsXPTParamInfo &paramInfo,
                                         PRUint16 methodIndex,
                                         nsXPTCMiniVariant *dispatchParams,
                                         PRBool isFullVariantArray,
                                         nsID &result)
{
  nsresult rv;

  switch (paramInfo.GetType().TagPart())
  {
    case nsXPTType::T_INTERFACE:
      rv = iinfo->GetIIDForParamNoAlloc(methodIndex, &paramInfo, &result);
      break;

    case nsXPTType::T_INTERFACE_IS:
    {
      PRUint8 argnum;
      rv = iinfo->GetInterfaceIsArgNumberForParam(methodIndex, &paramInfo, &argnum);
      if (NS_FAILED(rv))
        return rv;

      const nsXPTParamInfo& arg_param = methodInfo->GetParam(argnum);
      const nsXPTType& arg_type = arg_param.GetType();

      // The xpidl compiler ensures this. We reaffirm it for safety.
      if (!arg_type.IsPointer() || arg_type.TagPart() != nsXPTType::T_IID)
        return NS_ERROR_UNEXPECTED;

      nsID *p = nsnull;
      if (isFullVariantArray) {
        p = (nsID *) ((nsXPTCVariant*) dispatchParams)[argnum].val.p;
      } else {
        p = (nsID *) dispatchParams[argnum].val.p;
      }
      if (!p)
        return NS_ERROR_UNEXPECTED;

      result = *p;
      break;
    }

    default:
      rv = NS_ERROR_UNEXPECTED;
  }
  return rv;
}

