/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * 
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is RaptorCanvas.
 *
 * The Initial Developer of the Original Code is Kirk Baker and
 * Ian Wilkinson. Portions created by Kirk Baker and Ian Wilkinson are
 * Copyright (C) 1999 Kirk Baker and Ian Wilkinson. All
 * Rights Reserved.
 *
 * Contributor(s):  Ed Burns <edburns@acm.org>
 */

#include "RDFEnumeration.h"

#include "rdf_util.h"
#include "jni_util.h"

#include "nsIRDFContainer.h"
#include "nsIServiceManager.h"

//
// Local function prototypes
//

/**

 * pull the int for the field nativeEnum from the java object obj.

 */

jint getNativeEnumFromJava(JNIEnv *env, jobject obj, jint nativeRDFNode);

// 
// JNI methods
//

JNIEXPORT jboolean JNICALL 
Java_org_mozilla_webclient_wrapper_1native_RDFEnumeration_nativeHasMoreElements
(JNIEnv *env, jobject obj, jint nativeRDFNode)
{
  nsresult rv;
  jboolean result = JNI_FALSE;
  PRBool prResult = PR_FALSE;
  // assert -1 != nativeRDFNode
  jint nativeEnum;

  if (-1 == (nativeEnum = getNativeEnumFromJava(env, obj, nativeRDFNode))) {
      ::util_ThrowExceptionToJava(env, "Exception: nativeHasMoreElements: Can't get nativeEnum from nativeRDFNode.");
      return result;
  }

  nsCOMPtr<nsISimpleEnumerator> enumerator = (nsISimpleEnumerator *)nativeEnum;
  rv = enumerator->HasMoreElements(&prResult);
  if (NS_FAILED(rv)) {
      ::util_ThrowExceptionToJava(env, "Exception: nativeHasMoreElements: Can't ask nsISimpleEnumerator->HasMoreElements().");
      return result;
  }
  result = (PR_FALSE == prResult) ? JNI_FALSE : JNI_TRUE;

  return result;
}

JNIEXPORT jint JNICALL 
Java_org_mozilla_webclient_wrapper_1native_RDFEnumeration_nativeNextElement
(JNIEnv *env, jobject obj, jint nativeRDFNode)
{
  nsresult rv;
  jint result = -1;
  PRBool hasMoreElements = PR_FALSE;
  // assert -1 != nativeRDFNode
  jint nativeEnum;
  nsCOMPtr<nsISupports> supportsResult;
  nsCOMPtr<nsIRDFNode> nodeResult;

  if (-1 == (nativeEnum = getNativeEnumFromJava(env, obj, nativeRDFNode))) {
      ::util_ThrowExceptionToJava(env, "Exception: nativeNextElement: Can't get nativeEnum from nativeRDFNode.");
      return result;
  }
  
  nsCOMPtr<nsISimpleEnumerator> enumerator = (nsISimpleEnumerator *)nativeEnum;
  rv = enumerator->HasMoreElements(&hasMoreElements);
  if (NS_FAILED(rv)) {
      ::util_ThrowExceptionToJava(env, "Exception: nativeNextElement: Can't ask nsISimpleEnumerator->HasMoreElements().");
      return result;
  }
  
  if (!hasMoreElements) {
      return result;
  }
  
  rv = enumerator->GetNext(getter_AddRefs(supportsResult));
  if (NS_FAILED(rv)) {
      printf("Exception: nativeNextElement: Can't get next from enumerator.\n");
      return result;
  }
  
  // make sure it's an RDFNode
  rv = supportsResult->QueryInterface(NS_GET_IID(nsIRDFNode), 
                                      getter_AddRefs(nodeResult));
  if (NS_FAILED(rv)) {
      printf("Exception: nativeNextElement: next from enumerator is not an nsIRDFNode.\n");
      return result;
  }

  result = (jint)nodeResult.get();
  ((nsISupports *)result)->AddRef();
  return result;
}

JNIEXPORT void JNICALL 
Java_org_mozilla_webclient_wrapper_1native_RDFEnumeration_nativeFinalize
(JNIEnv *env, jobject obj)
{
    jclass objClass;
    jfieldID nativeEnumFieldID;
    jfieldID nativeContainerFieldID;
    jint nativeEnum, nativeContainer;
    
    objClass = env->GetObjectClass(obj);
    if (NULL == objClass) {
        printf("nativeFinalize: Can't get object class for RDFEnumeration.\n");
        return;
    }

    // release the nsISimpleEnumerator
    nativeEnumFieldID = env->GetFieldID(objClass, "nativeEnum", "I");
    if (NULL == nativeEnumFieldID) {
        printf("nativeFinalize: Can't get fieldID for nativeEnum.\n");
        return;
    }

    nativeEnum = env->GetIntField(obj, nativeEnumFieldID);
    nsCOMPtr<nsISimpleEnumerator> enumerator = 
        (nsISimpleEnumerator *) nativeEnum;
    ((nsISupports *)enumerator.get())->Release();
    
    // release the nsIRDFContainer
    nativeContainerFieldID = env->GetFieldID(objClass, "nativeContainer", "I");
    if (NULL == nativeContainerFieldID) {
        printf("nativeFinalize: Can't get fieldID for nativeContainer.\n");
        return;
    }

    nativeContainer = env->GetIntField(obj, nativeContainerFieldID);
    nsCOMPtr<nsIRDFContainer> container = 
        (nsIRDFContainer *) nativeContainer;
    ((nsISupports *)container.get())->Release();

    return;
}



//
// Local functions
//

jint getNativeEnumFromJava(JNIEnv *env, jobject obj, jint nativeRDFNode)
{
    nsresult rv;
    jint result = -1;
    jclass objClass;
    jfieldID nativeEnumFieldID;
    jfieldID nativeContainerFieldID;

    objClass = env->GetObjectClass(obj);
    if (NULL == objClass) {
        printf("getNativeEnumFromJava: Can't get object class for RDFEnumeration.\n");
        return -1;
    }

    nativeEnumFieldID = env->GetFieldID(objClass, "nativeEnum", "I");
    if (NULL == nativeEnumFieldID) {
        printf("getNativeEnumFromJava: Can't get fieldID for nativeEnum.\n");
        return -1;
    }

    result = env->GetIntField(obj, nativeEnumFieldID);

    // if the field has been initialized, just return the value
    if (-1 != result) {
        // NORMAL EXIT 1
        return result;
    }

    // else, we need to create the enum
    nsCOMPtr<nsIRDFNode> node = (nsIRDFNode *) nativeRDFNode;
    nsCOMPtr<nsIRDFResource> nodeResource;
    nsCOMPtr<nsIRDFContainer> container;
    nsCOMPtr<nsISimpleEnumerator> enumerator;

    rv = node->QueryInterface(NS_GET_IID(nsIRDFResource), 
                              getter_AddRefs(nodeResource));
    if (NS_FAILED(rv)) {
        printf("getNativeEnumFromJava: Argument nativeRDFNode isn't an nsIRDFResource.\n");
        return -1;
    }
    
    // get a container in order to get the enum
    rv = nsComponentManager::CreateInstance(NS_IRDFCONTAINER_PROGID,
                                            nsnull,
                                            NS_GET_IID(nsIRDFContainer),
                                            getter_AddRefs(container));
    if (NS_FAILED(rv)) {
        printf("recursiveResourceTraversal: can't get a new container\n");
        return -1;
    }
    
    rv = container->Init(gBookmarksDataSource, nodeResource);
    if (NS_FAILED(rv)) {
        printf("getNativeEnumFromJava: Can't Init container.\n");
        return -1;
    }

    rv = container->GetElements(getter_AddRefs(enumerator));
    if (NS_FAILED(rv)) {
        printf("getNativeEnumFromJava: Can't get enumeration from container.\n");
        return -1;
    }

    // IMPORTANT: Store the enum back into java
    env->SetIntField(obj, nativeEnumFieldID, (jint) enumerator.get());
    // IMPORTANT: make sure it doesn't get deleted when it goes out of scope
    ((nsISupports *)enumerator.get())->AddRef(); 

    // PENDING(edburns): I'm not sure if we need to keep the
    // nsIRDFContainer from being destructed in order to maintain the
    // validity of the nsISimpleEnumerator that came from the container.
    // Just to be safe, I'm doing so.
    nativeContainerFieldID = env->GetFieldID(objClass, "nativeContainer", "I");
    if (NULL == nativeContainerFieldID) {
        printf("getNativeEnumFromJava: Can't get fieldID for nativeContainer.\n");
        return -1;
    }
    env->SetIntField(obj, nativeContainerFieldID, (jint) container.get());
    ((nsISupports *)container.get())->AddRef();
    
    // NORMAL EXIT 2
    result = (jint)enumerator.get();    
    return result;
}
