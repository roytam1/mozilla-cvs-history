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

#include "nsJavaWrapper.h"
#include "nsJavaXPCOMBindingUtils.h"
#include "nsJavaXPTCStub.h"
#include "nsEmbedAPI.h"
#include "nsString.h"
#include "nsISimpleEnumerator.h"
#include "nsIInterfaceInfoManager.h"
#include "nsIInputStream.h"
#include "nsEnumeratorUtils.h"

#define XPCOM_NATIVE(func) Java_org_mozilla_xpcom_GeckoEmbed_##func

PRBool gEmbeddingInitialized = PR_FALSE;


extern "C" JNIEXPORT void JNICALL
XPCOM_NATIVE(NS_1InitEmbedding) (JNIEnv* env, jclass, jobject aMozBinDirectory,
                                 jobject aAppFileLocProvider)
{
  if (!InitializeJavaGlobals(env)) {
    FreeJavaGlobals(env);
    ThrowXPCOMException(env, 0);
    return;
  }

  nsresult rv;
  nsCOMPtr<nsILocalFile> directory;
  if (aMozBinDirectory)
  {
    // Find corresponding XPCOM object
    void* xpcomObj = GetMatchingXPCOMObject(env, aMozBinDirectory);
    NS_ASSERTION(xpcomObj != nsnull, "Failed to get matching XPCOM object");
    if (xpcomObj == nsnull) {
      ThrowXPCOMException(env, 0);
      return;
    }

    NS_ASSERTION(!IsXPTCStub(xpcomObj), "Expected JavaXPCOMInstance, but got nsJavaXPTCStub");
    directory = do_QueryInterface(((JavaXPCOMInstance*) xpcomObj)->GetInstance());
  }

  /* XXX How do we handle AppFileLocProvider, if we can't create any of the
   * Java<->XPCOM mappings before NS_InitEmbedding has been called?
   */
  nsIDirectoryServiceProvider* provider = nsnull;
/*  if (aAppFileLocProvider)
  {
    JavaXPCOMInstance* inst = (JavaXPCOMInstance*) aMozBinDirectory;
    provider = (nsIDirectoryServiceProvider*) inst->GetInstance();
  } */

  rv = NS_InitEmbedding(directory, provider);
  if (NS_FAILED(rv))
    ThrowXPCOMException(env, rv);

  gEmbeddingInitialized = PR_TRUE;
}

extern "C" JNIEXPORT void JNICALL
XPCOM_NATIVE(NS_1TermEmbedding) (JNIEnv *env, jclass)
{
  FreeJavaGlobals(env);

	nsresult rv = NS_TermEmbedding();
  if (NS_FAILED(rv))
    ThrowXPCOMException(env, rv);
}

/* XXX This can be called before XPCOM is init'd.  So we need to find a way
 * to create an appropriate Java class for this, such that if it is passed
 * through the JNI code (or if we make NS_InitEmbedding take it as a param),
 * then we can deal with it accordingly, since it won't yet have an
 * InterfaceInfo attached to it.  Perhaps we can set its InterfaceInfo to
 * NULL and just create it lazily.
 */
extern "C" JNIEXPORT jobject JNICALL
XPCOM_NATIVE(NS_1NewLocalFile) (JNIEnv *env, jclass, jstring aPath,
                                jboolean aFollowLinks)
{
  if (!InitializeJavaGlobals(env)) {
    FreeJavaGlobals(env);
    ThrowXPCOMException(env, 0);
    return nsnull;
  }

  jobject java_stub = nsnull;

  // Create a Mozilla string from the jstring
  jboolean isCopy;
  const PRUnichar* buf = nsnull;
  if (aPath) {
    buf = env->GetStringChars(aPath, &isCopy);
  }

  nsAutoString path_str(buf);
  if (isCopy) {
    env->ReleaseStringChars(aPath, buf);
  }

  // Make call to given function
  nsCOMPtr<nsILocalFile> file;
  nsresult rv = NS_NewLocalFile(path_str, aFollowLinks, getter_AddRefs(file));

  if (NS_SUCCEEDED(rv)) {
    // wrap xpcom instance
    JavaXPCOMInstance* inst;
    inst = CreateJavaXPCOMInstance(file, nsnull);

    if (inst) {
      // create java stub
      java_stub = CreateJavaWrapper(env, "nsILocalFile");

      if (java_stub) {
        // Associate XPCOM object w/ Java stub
        AddJavaXPCOMBinding(env, java_stub, inst);
      }
    }
  }

  if (java_stub == nsnull)
    ThrowXPCOMException(env, 0);

  return java_stub;
}

extern "C" JNIEXPORT jobject JNICALL
XPCOM_NATIVE(NS_1NewSingletonEnumerator) (JNIEnv *env, jclass, jobject aSingleton)
{
  void* inst = GetMatchingXPCOMObject(env, aSingleton);
  if (inst == nsnull) {
    // If there is not corresponding XPCOM object, then that means that the
    // parameter is non-generated class (that is, it is not one of our
    // Java stubs that represent an exising XPCOM object).  So we need to
    // create an XPCOM stub, that can route any method calls to the class.

    // Get interface info for class
    nsCOMPtr<nsIInterfaceInfoManager> iim = XPTI_GetInterfaceInfoManager();
    nsCOMPtr<nsIInterfaceInfo> iinfo;
    iim->GetInfoForIID(&NS_GET_IID(nsISupports), getter_AddRefs(iinfo));

    // Create XPCOM stub
    nsJavaXPTCStub* xpcomStub = new nsJavaXPTCStub(env, aSingleton, iinfo);
    NS_ADDREF(xpcomStub);
    inst = SetAsXPTCStub(xpcomStub);
    AddJavaXPCOMBinding(env, aSingleton, inst);
  }

  nsISupports* singleton;
  if (IsXPTCStub(inst))
    GetXPTCStubAddr(inst)->QueryInterface(NS_GET_IID(nsISupports),
                                          (void**) &singleton);
  else {
    JavaXPCOMInstance* xpcomInst = (JavaXPCOMInstance*) inst;
    singleton = xpcomInst->GetInstance();
  }

  // Call XPCOM method
  jobject java_stub = nsnull;
	nsISimpleEnumerator *enumerator = nsnull;
	nsresult rv = NS_NewSingletonEnumerator(&enumerator, singleton);

  if (NS_SUCCEEDED(rv)) {
    // wrap xpcom instance
    JavaXPCOMInstance* inst;
    inst = CreateJavaXPCOMInstance(enumerator, &NS_GET_IID(nsISimpleEnumerator));

    if (inst) {
      // create java stub
      java_stub = CreateJavaWrapper(env, "nsISimpleEnumerator");

      if (java_stub) {
        // Associate XPCOM object w/ Java stub
        AddJavaXPCOMBinding(env, java_stub, inst);
      }
    }
  }

  if (java_stub == nsnull)
    ThrowXPCOMException(env, 0);

  return java_stub;
}

extern "C" JNIEXPORT jobject JNICALL
XPCOM_NATIVE(NS_1GetComponentManager) (JNIEnv *env, jclass)
{
  jobject java_stub = nsnull;

  // Call XPCOM method
  nsCOMPtr<nsIComponentManager> cm;
  nsresult rv = NS_GetComponentManager(getter_AddRefs(cm));

  if (NS_SUCCEEDED(rv)) {
    // wrap xpcom instance
    JavaXPCOMInstance* inst;
    inst = CreateJavaXPCOMInstance(cm, &NS_GET_IID(nsIComponentManager));

    if (inst) {
      // create java stub
      java_stub = CreateJavaWrapper(env, "nsIComponentManager");

      if (java_stub) {
        // Associate XPCOM object w/ Java stub
        AddJavaXPCOMBinding(env, java_stub, inst);
      }
    }
  }

  if (java_stub == nsnull)
    ThrowXPCOMException(env, 0);

  return java_stub;
}

extern "C" JNIEXPORT jobject JNICALL
XPCOM_NATIVE(NS_1GetServiceManager) (JNIEnv *env, jclass)
{
  jobject java_stub = nsnull;

  // Call XPCOM method
  nsCOMPtr<nsIServiceManager> sm;
  nsresult rv = NS_GetServiceManager(getter_AddRefs(sm));

  if (NS_SUCCEEDED(rv)) {
    // wrap xpcom instance
    JavaXPCOMInstance* inst;
    inst = CreateJavaXPCOMInstance(sm, &NS_GET_IID(nsIServiceManager));

    if (inst) {
      // create java stub
      java_stub = CreateJavaWrapper(env, "nsIServiceManager");

      if (java_stub) {
        // Associate XPCOM object w/ Java stub
        AddJavaXPCOMBinding(env, java_stub, inst);
      }
    }
  }

  if (java_stub == nsnull)
    ThrowXPCOMException(env, 0);

  return java_stub;
}

// JNI wrapper for calling an nsWriteSegmentFun function
extern "C" JNIEXPORT jint JNICALL
XPCOM_NATIVE(nsWriteSegmentFun) (JNIEnv *env, jclass that, jint aWriterFunc,
                                 jobject aInStream, jint aClosure,
                                 jbyteArray aFromSegment, jint aToOffset,
                                 jint aCount)
{
  nsresult rc;

  void* inst = GetMatchingXPCOMObject(env, aInStream);
  if (inst == nsnull) {
    // If there is not corresponding XPCOM object, then that means that the
    // parameter is non-generated class (that is, it is not one of our
    // Java stubs that represent an exising XPCOM object).  So we need to
    // create an XPCOM stub, that can route any method calls to the class.

    // Get interface info for class
    nsCOMPtr<nsIInterfaceInfoManager> iim = XPTI_GetInterfaceInfoManager();
    nsCOMPtr<nsIInterfaceInfo> iinfo;
    iim->GetInfoForIID(&NS_GET_IID(nsIInputStream), getter_AddRefs(iinfo));

    // Create XPCOM stub
    nsJavaXPTCStub* xpcomStub = new nsJavaXPTCStub(env, aInStream, iinfo);
    NS_ADDREF(xpcomStub);
    inst = SetAsXPTCStub(xpcomStub);
    AddJavaXPCOMBinding(env, aInStream, inst);
  }

  nsIInputStream* instream;
  if (IsXPTCStub(inst))
    instream = (nsIInputStream*) GetXPTCStubAddr(inst);
  else {
    JavaXPCOMInstance* xpcomInst = (JavaXPCOMInstance*) inst;
    instream = (nsIInputStream*) xpcomInst->GetInstance();
  }

  jbyte* fromSegment = nsnull;
  jboolean isCopy = JNI_FALSE;
  if (aFromSegment) {
    fromSegment = env->GetByteArrayElements(aFromSegment, &isCopy);
  }

  PRUint32 write_count;
	nsWriteSegmentFun writer = (nsWriteSegmentFun) aWriterFunc;
	rc = writer(instream, (void*) aClosure, (const char*) fromSegment, aToOffset,
              aCount, &write_count);
  if (NS_FAILED(rc)) {
    ThrowXPCOMException(env, rc);
    return 0;
  }

  if (isCopy) {
    env->ReleaseByteArrayElements(aFromSegment, fromSegment, 0);
  }
	return write_count;
}

extern "C" JNIEXPORT void JNICALL
XPCOM_NATIVE(CallXPCOMMethodVoid) (JNIEnv *env, jclass that, jobject aJavaObject,
                                   jint aMethodIndex, jobjectArray aParams)
{
  jvalue rc;
  CallXPCOMMethod(env, that, aJavaObject, aMethodIndex, aParams, rc);
}

extern "C" JNIEXPORT jboolean JNICALL
XPCOM_NATIVE(CallXPCOMMethodBool) (JNIEnv *env, jclass that, jobject aJavaObject,
                                   jint aMethodIndex, jobjectArray aParams)
{
  jvalue rc;
  CallXPCOMMethod(env, that, aJavaObject, aMethodIndex, aParams, rc);
  return rc.z;
}

extern "C" JNIEXPORT jbooleanArray JNICALL
XPCOM_NATIVE(CallXPCOMMethodBoolA) (JNIEnv *env, jclass that, jobject aJavaObject,
                                    jint aMethodIndex, jobjectArray aParams)
{
  jvalue rc;
  CallXPCOMMethod(env, that, aJavaObject, aMethodIndex, aParams, rc);
  return (jbooleanArray) rc.l;
}

extern "C" JNIEXPORT jbyte JNICALL
XPCOM_NATIVE(CallXPCOMMethodByte) (JNIEnv *env, jclass that, jobject aJavaObject,
                                   jint aMethodIndex, jobjectArray aParams)
{
  jvalue rc;
  CallXPCOMMethod(env, that, aJavaObject, aMethodIndex, aParams, rc);
  return rc.b;
}

extern "C" JNIEXPORT jbyteArray JNICALL
XPCOM_NATIVE(CallXPCOMMethodByteA) (JNIEnv *env, jclass that, jobject aJavaObject,
                                    jint aMethodIndex, jobjectArray aParams)
{
  jvalue rc;
  CallXPCOMMethod(env, that, aJavaObject, aMethodIndex, aParams, rc);
  return (jbyteArray) rc.l;
}

extern "C" JNIEXPORT jchar JNICALL
XPCOM_NATIVE(CallXPCOMMethodChar) (JNIEnv *env, jclass that, jobject aJavaObject,
                                   jint aMethodIndex, jobjectArray aParams)
{
  jvalue rc;
  CallXPCOMMethod(env, that, aJavaObject, aMethodIndex, aParams, rc);
  return rc.c;
}

extern "C" JNIEXPORT jcharArray JNICALL
XPCOM_NATIVE(CallXPCOMMethodCharA) (JNIEnv *env, jclass that, jobject aJavaObject,
                                    jint aMethodIndex, jobjectArray aParams)
{
  jvalue rc;
  CallXPCOMMethod(env, that, aJavaObject, aMethodIndex, aParams, rc);
  return (jcharArray) rc.l;
}

extern "C" JNIEXPORT jshort JNICALL
XPCOM_NATIVE(CallXPCOMMethodShort) (JNIEnv *env, jclass that, jobject aJavaObject,
                                    jint aMethodIndex, jobjectArray aParams)
{
  jvalue rc;
  CallXPCOMMethod(env, that, aJavaObject, aMethodIndex, aParams, rc);
  return rc.s;
}

extern "C" JNIEXPORT jshortArray JNICALL
XPCOM_NATIVE(CallXPCOMMethodShortA) (JNIEnv *env, jclass that, jobject aJavaObject,
                                     jint aMethodIndex, jobjectArray aParams)
{
  jvalue rc;
  CallXPCOMMethod(env, that, aJavaObject, aMethodIndex, aParams, rc);
  return (jshortArray) rc.l;
}

extern "C" JNIEXPORT jint JNICALL
XPCOM_NATIVE(CallXPCOMMethodInt) (JNIEnv *env, jclass that, jobject aJavaObject,
                                  jint aMethodIndex, jobjectArray aParams)
{
  jvalue rc;
  CallXPCOMMethod(env, that, aJavaObject, aMethodIndex, aParams, rc);
  return rc.i;
}

extern "C" JNIEXPORT jintArray JNICALL
XPCOM_NATIVE(CallXPCOMMethodIntA) (JNIEnv *env, jclass that, jobject aJavaObject,
                                   jint aMethodIndex, jobjectArray aParams)
{
  jvalue rc;
  CallXPCOMMethod(env, that, aJavaObject, aMethodIndex, aParams, rc);
  return (jintArray) rc.l;
}

extern "C" JNIEXPORT jlong JNICALL
XPCOM_NATIVE(CallXPCOMMethodLong) (JNIEnv *env, jclass that, jobject aJavaObject,
                                   jint aMethodIndex, jobjectArray aParams)
{
  jvalue rc;
  CallXPCOMMethod(env, that, aJavaObject, aMethodIndex, aParams, rc);
  return rc.j;
}

extern "C" JNIEXPORT jlongArray JNICALL
XPCOM_NATIVE(CallXPCOMMethodLongA) (JNIEnv *env, jclass that, jobject aJavaObject,
                                    jint aMethodIndex, jobjectArray aParams)
{
  jvalue rc;
  CallXPCOMMethod(env, that, aJavaObject, aMethodIndex, aParams, rc);
  return (jlongArray) rc.l;
}

extern "C" JNIEXPORT jfloat JNICALL
XPCOM_NATIVE(CallXPCOMMethodFloat) (JNIEnv *env, jclass that, jobject aJavaObject,
                                    jint aMethodIndex, jobjectArray aParams)
{
  jvalue rc;
  CallXPCOMMethod(env, that, aJavaObject, aMethodIndex, aParams, rc);
  return rc.f;
}

extern "C" JNIEXPORT jfloatArray JNICALL
XPCOM_NATIVE(CallXPCOMMethodFloatA) (JNIEnv *env, jclass that, jobject aJavaObject,
                                     jint aMethodIndex, jobjectArray aParams)
{
  jvalue rc;
  CallXPCOMMethod(env, that, aJavaObject, aMethodIndex, aParams, rc);
  return (jfloatArray) rc.l;
}

extern "C" JNIEXPORT jdouble JNICALL
XPCOM_NATIVE(CallXPCOMMethodDouble) (JNIEnv *env, jclass that, jobject aJavaObject,
                                     jint aMethodIndex, jobjectArray aParams)
{
  jvalue rc;
  CallXPCOMMethod(env, that, aJavaObject, aMethodIndex, aParams, rc);
  return rc.d;
}

extern "C" JNIEXPORT jdoubleArray JNICALL
XPCOM_NATIVE(CallXPCOMMethodDoubleA) (JNIEnv *env, jclass that, jobject aJavaObject,
                                      jint aMethodIndex, jobjectArray aParams)
{
  jvalue rc;
  CallXPCOMMethod(env, that, aJavaObject, aMethodIndex, aParams, rc);
  return (jdoubleArray) rc.l;
}

extern "C" JNIEXPORT jobject JNICALL
XPCOM_NATIVE(CallXPCOMMethodObj) (JNIEnv *env, jclass that, jobject aJavaObject,
                                  jint aMethodIndex, jobjectArray aParams)
{
  jvalue rc;
  CallXPCOMMethod(env, that, aJavaObject, aMethodIndex, aParams, rc);
  return rc.l;
}

extern "C" JNIEXPORT jobjectArray JNICALL
XPCOM_NATIVE(CallXPCOMMethodObjA) (JNIEnv *env, jclass that, jobject aJavaObject,
                                   jint aMethodIndex, jobjectArray aParams)
{
  jvalue rc;
  CallXPCOMMethod(env, that, aJavaObject, aMethodIndex, aParams, rc);
  return (jobjectArray) rc.l;
}

extern "C" JNIEXPORT void JNICALL
XPCOM_NATIVE(FinalizeStub) (JNIEnv *env, jclass that, jobject aJavaObject)
{
#ifdef DEBUG
  jboolean isCopy;
  jclass clazz = env->GetObjectClass(aJavaObject);
  jstring name = (jstring) env->CallObjectMethod(clazz, getNameMID);
  const char* javaObjectName = env->GetStringUTFChars(name, &isCopy);
  fprintf(stderr, "*** Finalize(java_obj=%s)\n", javaObjectName);
  if (isCopy)
    env->ReleaseStringUTFChars(name, javaObjectName);
#endif

  void* obj = GetMatchingXPCOMObject(env, aJavaObject);
  RemoveJavaXPCOMBinding(env, aJavaObject, nsnull);

  nsISupports* xpcom_obj = nsnull;
  if (IsXPTCStub(obj)) {
    GetXPTCStubAddr(obj)->QueryInterface(NS_GET_IID(nsISupports),
                                         (void**) &xpcom_obj);
  } else {
    JavaXPCOMInstance* inst = (JavaXPCOMInstance*) obj;
    xpcom_obj = inst->GetInstance();
    // XXX Getting some odd thread issues when calling delete.  Addreffing for
    //  now to work around the errors.
//    NS_ADDREF(inst);
    delete inst;
  }
  NS_RELEASE(xpcom_obj);
}

