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
#include "rdf_progids.h"
#include "ns_util.h"

#include "nsIRDFContainer.h"
#include "nsIServiceManager.h"

#include "prlog.h" // for PR_ASSERT

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
      if (prLogModuleInfo) {
          PR_LOG(prLogModuleInfo, 3, 
                 ("Exception: nativeNextElement: Can't get next from enumerator.\n"));
      }
      return result;
  }
  
  // make sure it's an RDFNode
  rv = supportsResult->QueryInterface(NS_GET_IID(nsIRDFNode), 
                                      getter_AddRefs(nodeResult));
  if (NS_FAILED(rv)) {
      if (prLogModuleInfo) {
          PR_LOG(prLogModuleInfo, 3, 
                 ("Exception: nativeNextElement: next from enumerator is not an nsIRDFNode.\n"));
      }
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
    jint nativeEnum, nativeContainer;

    // release the nsISimpleEnumerator
    if (-1 == (nativeEnum = 
               ::util_GetIntValueFromInstance(env, obj, "nativeEnum"))) {
        if (prLogModuleInfo) {
            PR_LOG(prLogModuleInfo, 3, 
                   ("nativeFinalize: Can't get fieldID for nativeEnum.\n"));
        }
        return;
    }
    nsCOMPtr<nsISimpleEnumerator> enumerator = 
        (nsISimpleEnumerator *) nativeEnum;
    ((nsISupports *)enumerator.get())->Release();
    
    // release the nsIRDFContainer
    if (-1 == (nativeContainer = 
               ::util_GetIntValueFromInstance(env, obj, "nativeContainer"))) {
        if (prLogModuleInfo) {
            PR_LOG(prLogModuleInfo, 3, 
                   ("nativeFinalize: Can't get fieldID for nativeContainerFieldID.\n"));
        }
        return;
    }
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

    result = ::util_GetIntValueFromInstance(env, obj, "nativeEnum");

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
        if (prLogModuleInfo) {
            PR_LOG(prLogModuleInfo, 3, 
                   ("getNativeEnumFromJava: Argument nativeRDFNode isn't an nsIRDFResource.\n"));
        }
        return -1;
    }

    PR_ASSERT(gComponentManager);
    
    // get a container in order to get the enum
    rv = gComponentManager->CreateInstanceByProgID(NS_RDFCONTAINER_PROGID,
                                                   nsnull,
                                                   NS_GET_IID(nsIRDFContainer),
                                                   getter_AddRefs(container));
    if (NS_FAILED(rv)) {
        if (prLogModuleInfo) {
            PR_LOG(prLogModuleInfo, 3, 
                   ("recursiveResourceTraversal: can't get a new container\n"));
        }
        return -1;
    }
    
    rv = container->Init(gBookmarksDataSource, nodeResource);
    if (NS_FAILED(rv)) {
        if (prLogModuleInfo) {
            PR_LOG(prLogModuleInfo, 3, 
                   ("getNativeEnumFromJava: Can't Init container.\n"));
        }
        return -1;
    }

    rv = container->GetElements(getter_AddRefs(enumerator));
    if (NS_FAILED(rv)) {
        if (prLogModuleInfo) {
            PR_LOG(prLogModuleInfo, 3, 
                   ("getNativeEnumFromJava: Can't get enumeration from container.\n"));
        }
        return -1;
    }

    // IMPORTANT: Store the enum back into java
    ::util_SetIntValueForInstance(env,obj,"nativeEnum",(jint)enumerator.get());
    // IMPORTANT: make sure it doesn't get deleted when it goes out of scope
    ((nsISupports *)enumerator.get())->AddRef(); 

    // PENDING(edburns): I'm not sure if we need to keep the
    // nsIRDFContainer from being destructed in order to maintain the
    // validity of the nsISimpleEnumerator that came from the container.
    // Just to be safe, I'm doing so.
    ::util_SetIntValueForInstance(env, obj, "nativeContainer", 
                                  (jint) container.get());
    ((nsISupports *)container.get())->AddRef();
    
    // NORMAL EXIT 2
    result = (jint)enumerator.get();    
    return result;
}
