/* 
The contents of this file are subject to the Mozilla Public License
Version 1.0 (the "License"); you may not use this file except in
compliance with the License. You may obtain a copy of the License at
http://www.mozilla.org/MPL/

Software distributed under the License is distributed on an "AS IS"
basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
the License for the specific language governing rights and limitations
under the License.

The Initial Developer of the Original Code is Sun Microsystems,
Inc. Portions created by Sun are Copyright (C) 1999 Sun Microsystems,
Inc. All Rights Reserved. 
*/

#include "prlog.h"
#include "nsIDOMNode.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMNamedNodeMap.h"
#include "nsIDOMDocument.h"
#include "nsDOMError.h"
#include "javaDOMGlobals.h"
#include "org_mozilla_dom_NodeImpl.h"

static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);

JNIEXPORT jboolean JNICALL Java_org_mozilla_dom_NodeImpl_XPCOM_1equals
  (JNIEnv *env, jobject jthis, jobject nodeArg)
{
  jboolean b_retFlag = JNI_FALSE;

  nsIDOMNode* p_thisNode = 
    (nsIDOMNode*) env->GetLongField(jthis, JavaDOMGlobals::nodePtrFID);
  if (!p_thisNode) {
    PR_LOG(JavaDOMGlobals::log, PR_LOG_WARNING, 
	   ("Node.equals: NULL pointer\n"));
    return b_retFlag;
  }

  nsIDOMNode* p_argNode = 
    (nsIDOMNode*) env->GetLongField(nodeArg, JavaDOMGlobals::nodePtrFID);
  if (!p_argNode) {
    PR_LOG(JavaDOMGlobals::log, PR_LOG_WARNING, 
	   ("Node.equals: NULL arg pointer\n"));
    return b_retFlag;
  }

  nsISupports* thisSupports = nsnull;
  nsISupports* argNodeSupports = nsnull;

  nsresult rvThis = 
    p_thisNode->QueryInterface(kISupportsIID, (void**)(&thisSupports));
  if (NS_FAILED(rvThis) || !thisSupports) {
    PR_LOG(JavaDOMGlobals::log, PR_LOG_ERROR, 
	   ("Node.equals: this->QueryInterface failed (%x)\n", rvThis));
    return b_retFlag; 	
  }

  nsresult rvArgNode =
    p_argNode->QueryInterface(kISupportsIID, (void**)(&argNodeSupports));
  if (NS_FAILED(rvArgNode) || !argNodeSupports) {
    PR_LOG(JavaDOMGlobals::log, PR_LOG_ERROR, 
	   ("Node.equals: arg->QueryInterface failed (%x)\n", rvArgNode));
    thisSupports->Release();
    return b_retFlag;
  }

  if (thisSupports == argNodeSupports)
    b_retFlag = JNI_TRUE;
  
  thisSupports->Release();
  argNodeSupports->Release();

  return b_retFlag;
}

JNIEXPORT jint JNICALL Java_org_mozilla_dom_NodeImpl_XPCOM_1hashCode
  (JNIEnv *env, jobject jthis)
{
  nsIDOMNode* p_thisNode = 
    (nsIDOMNode*) env->GetLongField(jthis, JavaDOMGlobals::nodePtrFID);
  if (!p_thisNode) {
    PR_LOG(JavaDOMGlobals::log, PR_LOG_WARNING, 
	   ("Node.hashCode: NULL pointer\n"));
    return (jint) 0;
  }

  nsISupports* thisSupports = nsnull;
  nsresult rvThis = 
    p_thisNode->QueryInterface(kISupportsIID, (void**)(&thisSupports));
  if (NS_FAILED(rvThis) || !thisSupports) {
    PR_LOG(JavaDOMGlobals::log, PR_LOG_ERROR, 
	   ("Node.hashCode: QueryInterface failed (%x)\n", rvThis));
    return (jint) 0;
  }

  thisSupports->Release();
  return (jint) thisSupports;
}

/*
 * Class:     org_mozilla_dom_NodeImpl
 * Method:    finalize
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_mozilla_dom_NodeImpl_finalize
  (JNIEnv *env, jobject jthis)
{
  nsIDOMNode* node = (nsIDOMNode*) 
    env->GetLongField(jthis, JavaDOMGlobals::nodePtrFID);
  if (!node) {
    PR_LOG(JavaDOMGlobals::log, PR_LOG_WARNING, 
	   ("Node.finalize: NULL pointer\n"));
    return;
  }

  JavaDOMGlobals::AddToGarbage(node);
}

/*
 * Class:     org_mozilla_dom_NodeImpl
 * Method:    appendChild
 * Signature: (Lorg/w3c/dom/Node;)Lorg/w3c/dom/Node;
 */
JNIEXPORT jobject JNICALL Java_org_mozilla_dom_NodeImpl_appendChild
  (JNIEnv *env, jobject jthis, jobject jchild)
{
  nsIDOMNode* node = (nsIDOMNode*) 
    env->GetLongField(jthis, JavaDOMGlobals::nodePtrFID);
  if (!node) {
    JavaDOMGlobals::ThrowException(env,
      "Node.appendChild: NULL pointer");
    return NULL;
  }

  nsIDOMNode* child = (nsIDOMNode*) 
    env->GetLongField(jchild, JavaDOMGlobals::nodePtrFID);
  if (!child) {
    JavaDOMGlobals::ThrowException(env,
      "Node.appendChild: NULL child pointer");
    return NULL;
  }

  nsIDOMNode* ret = nsnull;
  nsresult rv = node->AppendChild(child, &ret);
  if (NS_FAILED(rv) || !ret) {
    JavaDOMGlobals::ExceptionType exceptionType = JavaDOMGlobals::EXCEPTION_RUNTIME;
    if (NS_ERROR_GET_MODULE(rv) == NS_ERROR_MODULE_DOM &&
        (NS_ERROR_GET_CODE(rv) == NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR ||
         NS_ERROR_GET_CODE(rv) == NS_ERROR_DOM_WRONG_DOCUMENT_ERR ||
         NS_ERROR_GET_CODE(rv) == NS_ERROR_DOM_HIERARCHY_REQUEST_ERR)) {
      exceptionType = JavaDOMGlobals::EXCEPTION_DOM;
    }
    JavaDOMGlobals::ThrowException(env,
      "Node.appendChild: failed", rv, exceptionType);
    return NULL;
  }

  return JavaDOMGlobals::CreateNodeSubtype(env, ret);
}

/*
 * Class:     org_mozilla_dom_NodeImpl
 * Method:    cloneNode
 * Signature: (Z)Lorg/w3c/dom/Node;
 */
JNIEXPORT jobject JNICALL Java_org_mozilla_dom_NodeImpl_cloneNode
  (JNIEnv *env, jobject jthis, jboolean jdeep)
{
  nsIDOMNode* node = (nsIDOMNode*) 
    env->GetLongField(jthis, JavaDOMGlobals::nodePtrFID);
  if (!node) {
    JavaDOMGlobals::ThrowException(env,
      "Node.cloneNode: NULL pointer");
    return NULL;
  }

  nsIDOMNode* ret = nsnull;
  PRBool deep = jdeep == JNI_TRUE ? PR_TRUE : PR_FALSE;
  nsresult rv = node->CloneNode(deep, &ret);
  if (NS_FAILED(rv) || !ret) {
    JavaDOMGlobals::ThrowException(env,
      "Node.cloneNode: failed", rv);
    return NULL;
  }

  return JavaDOMGlobals::CreateNodeSubtype(env, ret);
}

/*
 * Class:     org_mozilla_dom_NodeImpl
 * Method:    getAttributes
 * Signature: ()Lorg/w3c/dom/NamedNodeMap;
 */
JNIEXPORT jobject JNICALL Java_org_mozilla_dom_NodeImpl_getAttributes
  (JNIEnv *env, jobject jthis)
{
  nsIDOMNode* node = (nsIDOMNode*) 
    env->GetLongField(jthis, JavaDOMGlobals::nodePtrFID);
  if (!node) 
    return NULL;

  nsIDOMNamedNodeMap* nodeMap = nsnull;
  nsresult rv = node->GetAttributes(&nodeMap);
  if (NS_FAILED(rv)) {
    JavaDOMGlobals::ThrowException(env,
      "Node.getAttributes: failed", rv);
    return NULL;
  }
  if (!nodeMap) {
    /* according to the spec, getAttributes may return NULL when there
       are no attributes. So this is not an error */
    return NULL;
  }

  jobject jret = env->AllocObject(JavaDOMGlobals::namedNodeMapClass);
  if (!jret) {
    JavaDOMGlobals::ThrowException(env,
      "Node.getAttributes: failed to allocate object");
    return NULL;
  }

  env->SetLongField(jret, JavaDOMGlobals::nodePtrFID, (jlong) nodeMap);
  if (env->ExceptionOccurred()) {
    JavaDOMGlobals::ThrowException(env,
      "Node.getAttributes: failed to set node ptr");
    return NULL;
  }

  nodeMap->AddRef();
  return jret;
}

/*
 * Class:     org_mozilla_dom_NodeImpl
 * Method:    getChildNodes
 * Signature: ()Lorg/w3c/dom/NodeList;
 */
JNIEXPORT jobject JNICALL Java_org_mozilla_dom_NodeImpl_getChildNodes
  (JNIEnv *env, jobject jthis)
{
  nsIDOMNode* node = (nsIDOMNode*) 
    env->GetLongField(jthis, JavaDOMGlobals::nodePtrFID);
  if (!node) {
    JavaDOMGlobals::ThrowException(env,
      "Node.getChildNodes: NULL pointer");
    return NULL;
  }

  nsIDOMNodeList* nodeList = nsnull;
  nsresult rv = node->GetChildNodes(&nodeList);
  if (NS_FAILED(rv) || !nodeList) {
    JavaDOMGlobals::ThrowException(env,
      "Node.getChildNodes: failed", rv);
    return NULL;
  }

  jobject jret = env->AllocObject(JavaDOMGlobals::nodeListClass);
  if (!jret) {
    JavaDOMGlobals::ThrowException(env,
      "Node.getChildNodes: failed to allocate object");
    return NULL;
  }

  env->SetLongField(jret, JavaDOMGlobals::nodeListPtrFID, (jlong) nodeList);
  if (env->ExceptionOccurred()) {
    JavaDOMGlobals::ThrowException(env,
      "Node.getChildNodes: failed to set node ptr");
    return NULL;
  }

  nodeList->AddRef();
  return jret;
}

/*
 * Class:     org_mozilla_dom_NodeImpl
 * Method:    getFirstChild
 * Signature: ()Lorg/w3c/dom/Node;
 */
JNIEXPORT jobject JNICALL Java_org_mozilla_dom_NodeImpl_getFirstChild
  (JNIEnv *env, jobject jthis)
{
  nsIDOMNode* node = (nsIDOMNode*) 
    env->GetLongField(jthis, JavaDOMGlobals::nodePtrFID);
  if (!node)
    return NULL;

  nsIDOMNode* ret = nsnull;
  nsresult rv = node->GetFirstChild(&ret);
  if (NS_FAILED(rv)) {
    JavaDOMGlobals::ThrowException(env,
      "Node.getFirstChild: failed", rv);
    return NULL;
  }
  if (!ret) {
    /* according to the spec, getFirstChild may return NULL when there
       are no children. So this is not an error */
    return NULL;
  }

  return JavaDOMGlobals::CreateNodeSubtype(env, ret);
}

/*
 * Class:     org_mozilla_dom_NodeImpl
 * Method:    getLastChild
 * Signature: ()Lorg/w3c/dom/Node;
 */
JNIEXPORT jobject JNICALL Java_org_mozilla_dom_NodeImpl_getLastChild
  (JNIEnv *env, jobject jthis)
{
  nsIDOMNode* node = (nsIDOMNode*) 
    env->GetLongField(jthis, JavaDOMGlobals::nodePtrFID);
  if (!node)
    return NULL;

  nsIDOMNode* ret = nsnull;
  nsresult rv = node->GetLastChild(&ret);
  if (NS_FAILED(rv)) {
    JavaDOMGlobals::ThrowException(env,
      "Node.getLastChild: failed", rv);
    return NULL;
  }
  if (!ret) {
    /* according to the spec, getLastChild may return NULL when there
       are no children. So this is not an error */
    return NULL;
  }

  return JavaDOMGlobals::CreateNodeSubtype(env, ret);
}

/*
 * Class:     org_mozilla_dom_NodeImpl
 * Method:    getNextSibling
 * Signature: ()Lorg/w3c/dom/Node;
 */
JNIEXPORT jobject JNICALL Java_org_mozilla_dom_NodeImpl_getNextSibling
  (JNIEnv *env, jobject jthis)
{
  nsIDOMNode* node = (nsIDOMNode*) 
    env->GetLongField(jthis, JavaDOMGlobals::nodePtrFID);
  if (!node)
    return NULL;

  nsIDOMNode* ret = nsnull;
  nsresult rv = node->GetNextSibling(&ret);
  if (NS_FAILED(rv) || !ret) {
    JavaDOMGlobals::ThrowException(env,
      "Node.getNextSibling: failed", rv);
    return NULL;
  }

  return JavaDOMGlobals::CreateNodeSubtype(env, ret);
}

/*
 * Class:     org_mozilla_dom_NodeImpl
 * Method:    getNodeName
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_org_mozilla_dom_NodeImpl_getNodeName
  (JNIEnv *env, jobject jthis)
{
  nsIDOMNode* node = (nsIDOMNode*) 
    env->GetLongField(jthis, JavaDOMGlobals::nodePtrFID);
  if (!node) {
    JavaDOMGlobals::ThrowException(env,
      "Node.getNodeName: NULL pointer");
    return NULL;
  }

  nsString ret;
  nsresult rv = node->GetNodeName(ret);
  if (NS_FAILED(rv)) {
    JavaDOMGlobals::ThrowException(env,
      "Node.getNodeName: failed", rv);
    return NULL;
  }

  jstring jret = env->NewString(ret.GetUnicode(), ret.Length());
  if (!jret) {
    JavaDOMGlobals::ThrowException(env,
      "Node.getNodeName: NewString failed");
    return NULL;
  }

  return jret;
}

/*
 * Class:     org_mozilla_dom_NodeImpl
 * Method:    getNodeType
 * Signature: ()S
 */
JNIEXPORT jshort JNICALL Java_org_mozilla_dom_NodeImpl_getNodeType
  (JNIEnv *env, jobject jthis)
{
  nsIDOMNode* node = (nsIDOMNode*) 
    env->GetLongField(jthis, JavaDOMGlobals::nodePtrFID);
  if (!node) {
    JavaDOMGlobals::ThrowException(env,
      "Node.getNodeType: NULL pointer");
    return (jshort) NULL;
  }

  PRUint16 type;
  nsresult rv = node->GetNodeType(&type);
  if (NS_FAILED(rv)) {
    JavaDOMGlobals::ThrowException(env,
      "Node.getNodeType: failed", rv);
    return (jshort) NULL;
  }

  jfieldID typeFID = NULL;
  switch (type) {
  case nsIDOMNode::ATTRIBUTE_NODE:
      typeFID = JavaDOMGlobals::nodeTypeAttributeFID;
      break;

  case nsIDOMNode::CDATA_SECTION_NODE:
      typeFID = JavaDOMGlobals::nodeTypeCDataSectionFID;
      break;

  case nsIDOMNode::COMMENT_NODE:
      typeFID = JavaDOMGlobals::nodeTypeCommentFID;
      break;

  case nsIDOMNode::DOCUMENT_FRAGMENT_NODE:
      typeFID = JavaDOMGlobals::nodeTypeDocumentFragmentFID;
      break;

  case nsIDOMNode::DOCUMENT_NODE:
      typeFID = JavaDOMGlobals::nodeTypeDocumentFID;
      break;

  case nsIDOMNode::DOCUMENT_TYPE_NODE:
      typeFID = JavaDOMGlobals::nodeTypeDocumentTypeFID;
      break;

  case nsIDOMNode::ELEMENT_NODE:
      typeFID = JavaDOMGlobals::nodeTypeElementFID;
      break;

  case nsIDOMNode::ENTITY_NODE:
      typeFID = JavaDOMGlobals::nodeTypeEntityFID;
      break;

  case nsIDOMNode::ENTITY_REFERENCE_NODE:
      typeFID = JavaDOMGlobals::nodeTypeEntityReferenceFID;
      break;

  case nsIDOMNode::NOTATION_NODE:
      typeFID = JavaDOMGlobals::nodeTypeNotationFID;
      break;

  case nsIDOMNode::PROCESSING_INSTRUCTION_NODE:
      typeFID = JavaDOMGlobals::nodeTypeProcessingInstructionFID;
      break;

  case nsIDOMNode::TEXT_NODE:
      typeFID = JavaDOMGlobals::nodeTypeTextFID;
      break;
  }

  jshort ret = 0;
  if (typeFID) {
      jclass nodeClass = env->GetObjectClass(jthis);
      if (!nodeClass) {
        JavaDOMGlobals::ThrowException(env,
	  "Node.getNodeType: GetObjectClass failed");
	return ret;
      }

      ret = env->GetStaticShortField(nodeClass, typeFID);
      if (env->ExceptionOccurred()) {
        JavaDOMGlobals::ThrowException(env,
	  "Node.getNodeType: typeFID failed");
	return ret;
      }
  } else {
    JavaDOMGlobals::ThrowException(env,
      "Node.getNodeType: illegal type");
  }

  return ret;
}

/*
 * Class:     org_mozilla_dom_NodeImpl
 * Method:    getNodeValue
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_org_mozilla_dom_NodeImpl_getNodeValue
  (JNIEnv *env, jobject jthis)
{
  nsIDOMNode* node = (nsIDOMNode*) 
    env->GetLongField(jthis, JavaDOMGlobals::nodePtrFID);
  if (!node)
    return NULL;

  nsString ret;
  nsresult rv = node->GetNodeValue(ret);
  if (NS_FAILED(rv)) {
    JavaDOMGlobals::ExceptionType exceptionType = JavaDOMGlobals::EXCEPTION_RUNTIME;
    if (NS_ERROR_GET_MODULE(rv) == NS_ERROR_MODULE_DOM &&
        NS_ERROR_GET_CODE(rv) == NS_ERROR_DOM_DOMSTRING_SIZE_ERR) {
      exceptionType = JavaDOMGlobals::EXCEPTION_DOM;
    }
    JavaDOMGlobals::ThrowException(env,
      "Node.getNodeValue: failed", rv, exceptionType);
    return NULL;
  }

  jstring jret = env->NewString(ret.GetUnicode(), ret.Length());
  if (!jret) {
    JavaDOMGlobals::ThrowException(env,
      "Node.getNodeValue: NewString failed");
    return NULL;
  }

  return jret;
}

/*
 * Class:     org_mozilla_dom_NodeImpl
 * Method:    getOwnerDocument
 * Signature: ()Lorg/w3c/dom/Document;
 */
JNIEXPORT jobject JNICALL Java_org_mozilla_dom_NodeImpl_getOwnerDocument
  (JNIEnv *env, jobject jthis)
{
  nsIDOMNode* node = (nsIDOMNode*) 
    env->GetLongField(jthis, JavaDOMGlobals::nodePtrFID);
  if (!node)
    return NULL;

  nsIDOMDocument* ret = nsnull;
  nsresult rv = node->GetOwnerDocument(&ret);
  if (NS_FAILED(rv) || !ret) {
    JavaDOMGlobals::ThrowException(env,
      "Node.getOwnerDocument: failed", rv);
    return NULL;
  }

  jobject jret = env->AllocObject(JavaDOMGlobals::documentClass);
  if (!jret) {
    JavaDOMGlobals::ThrowException(env,
      "Node.getOwnerDocument: failed to allocate object");
    return NULL;
  }

  env->SetLongField(jret, JavaDOMGlobals::nodePtrFID, (jlong) ret);
  if (env->ExceptionOccurred()) {
    JavaDOMGlobals::ThrowException(env,
      "Node.getOwnerDocument: failed to set node ptr");
    return NULL;
  }

  ret->AddRef();
  return jret;
}

/*
 * Class:     org_mozilla_dom_NodeImpl
 * Method:    getParentNode
 * Signature: ()Lorg/w3c/dom/Node;
 */
JNIEXPORT jobject JNICALL Java_org_mozilla_dom_NodeImpl_getParentNode
  (JNIEnv *env, jobject jthis)
{
  nsIDOMNode* node = (nsIDOMNode*) 
    env->GetLongField(jthis, JavaDOMGlobals::nodePtrFID);
  if (!node)
    return NULL;

  nsIDOMNode* ret = nsnull;
  nsresult rv = node->GetParentNode(&ret);
  if (NS_FAILED(rv) || !ret) {
    JavaDOMGlobals::ThrowException(env,
      "Node.getParentNode: failed", rv);
    return NULL;
  }

  return JavaDOMGlobals::CreateNodeSubtype(env, ret);
}

/*
 * Class:     org_mozilla_dom_NodeImpl
 * Method:    getPreviousSibling
 * Signature: ()Lorg/w3c/dom/Node;
 */
JNIEXPORT jobject JNICALL Java_org_mozilla_dom_NodeImpl_getPreviousSibling
  (JNIEnv *env, jobject jthis)
{
  nsIDOMNode* node = (nsIDOMNode*) 
    env->GetLongField(jthis, JavaDOMGlobals::nodePtrFID);
  if (!node)
    return NULL;

  nsIDOMNode* ret = nsnull;
  nsresult rv = node->GetPreviousSibling(&ret);
  if (NS_FAILED(rv) || !ret) {
    JavaDOMGlobals::ThrowException(env,
      "Node.getPreviousSibling: failed", rv);
    return NULL;
  }

  return JavaDOMGlobals::CreateNodeSubtype(env, ret);
}

/*
 * Class:     org_mozilla_dom_NodeImpl
 * Method:    hasChildNodes
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_org_mozilla_dom_NodeImpl_hasChildNodes
  (JNIEnv *env, jobject jthis)
{
  nsIDOMNode* node = (nsIDOMNode*) 
    env->GetLongField(jthis, JavaDOMGlobals::nodePtrFID);
  if (!node) {
    JavaDOMGlobals::ThrowException(env,
      "Node.hasChildNodes: NULL pointer");
    return (jboolean) NULL;
  }

  PRBool ret = PR_FALSE;
  nsresult rv = node->HasChildNodes(&ret);
  if (NS_FAILED(rv)) {
    JavaDOMGlobals::ThrowException(env,
      "Node.hasChildNodes: failed", rv);
    return (jboolean) NULL;
  }

  return ret == PR_TRUE ? JNI_TRUE : JNI_FALSE;
}

/*
 * Class:     org_mozilla_dom_NodeImpl
 * Method:    insertBefore
 * Signature: (Lorg/w3c/dom/Node;Lorg/w3c/dom/Node;)Lorg/w3c/dom/Node;
 */
JNIEXPORT jobject JNICALL Java_org_mozilla_dom_NodeImpl_insertBefore
  (JNIEnv *env, jobject jthis, jobject jnewChild, jobject jrefChild)
{
  nsIDOMNode* node = (nsIDOMNode*) 
    env->GetLongField(jthis, JavaDOMGlobals::nodePtrFID);
  if (!node) {
    JavaDOMGlobals::ThrowException(env,
      "Node.insertBefore: NULL pointer");
    return NULL;
  }

  nsIDOMNode* newChild = (nsIDOMNode*) 
    env->GetLongField(jnewChild, JavaDOMGlobals::nodePtrFID);
  if (!newChild) {
    JavaDOMGlobals::ThrowException(env,
      "Node.insertBefore: NULL newChild pointer");
    return NULL;
  }

  nsIDOMNode* refChild = (nsIDOMNode*) 
    env->GetLongField(jrefChild, JavaDOMGlobals::nodePtrFID);
  if (!refChild) {
    JavaDOMGlobals::ThrowException(env,
      "Node.insertBefore: NULL refChild pointer");
    return NULL;
  }

  nsIDOMNode* ret = nsnull;
  nsresult rv = node->InsertBefore(newChild, refChild, &ret);
  if (NS_FAILED(rv) || !ret) {
    JavaDOMGlobals::ExceptionType exceptionType = JavaDOMGlobals::EXCEPTION_RUNTIME;
    if (NS_ERROR_GET_MODULE(rv) == NS_ERROR_MODULE_DOM &&
        (NS_ERROR_GET_CODE(rv) == NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR ||
         NS_ERROR_GET_CODE(rv) == NS_ERROR_DOM_WRONG_DOCUMENT_ERR ||
         NS_ERROR_GET_CODE(rv) == NS_ERROR_DOM_NOT_FOUND_ERR ||
         NS_ERROR_GET_CODE(rv) == NS_ERROR_DOM_HIERARCHY_REQUEST_ERR)) {
        exceptionType = JavaDOMGlobals::EXCEPTION_DOM;
    }
    JavaDOMGlobals::ThrowException(env,
      "Node.insertBefore: failed", rv, exceptionType);
    return NULL;
  }

  return JavaDOMGlobals::CreateNodeSubtype(env, ret);
}

/*
 * Class:     org_mozilla_dom_NodeImpl
 * Method:    removeChild
 * Signature: (Lorg/w3c/dom/Node;)Lorg/w3c/dom/Node;
 */
JNIEXPORT jobject JNICALL Java_org_mozilla_dom_NodeImpl_removeChild
  (JNIEnv *env, jobject jthis, jobject joldChild)
{
  nsIDOMNode* node = (nsIDOMNode*) 
    env->GetLongField(jthis, JavaDOMGlobals::nodePtrFID);
  if (!node) {
    JavaDOMGlobals::ThrowException(env,
      "Node.removeChild: NULL pointer");
    return NULL;
  }

  nsIDOMNode* oldChild = (nsIDOMNode*) 
    env->GetLongField(joldChild, JavaDOMGlobals::nodePtrFID);
  if (!oldChild) {
    JavaDOMGlobals::ThrowException(env,
      "Node.removeChild: NULL oldChild pointer");
    return NULL;
  }

  nsIDOMNode* ret = nsnull;
  nsresult rv = node->RemoveChild(oldChild, &ret);
  if (NS_FAILED(rv) || !ret) {
    JavaDOMGlobals::ExceptionType exceptionType = JavaDOMGlobals::EXCEPTION_RUNTIME;
    if (NS_ERROR_GET_MODULE(rv) == NS_ERROR_MODULE_DOM &&
        (NS_ERROR_GET_CODE(rv) == NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR ||
         NS_ERROR_GET_CODE(rv) == NS_ERROR_DOM_NOT_FOUND_ERR)) {
      exceptionType = JavaDOMGlobals::EXCEPTION_DOM;
    }
    JavaDOMGlobals::ThrowException(env,
      "Node.removeChild: failed", rv, exceptionType);
    return NULL;
  }

  return JavaDOMGlobals::CreateNodeSubtype(env, ret);
}

/*
 * Class:     org_mozilla_dom_NodeImpl
 * Method:    replaceChild
 * Signature: (Lorg/w3c/dom/Node;Lorg/w3c/dom/Node;)Lorg/w3c/dom/Node;
 */
JNIEXPORT jobject JNICALL Java_org_mozilla_dom_NodeImpl_replaceChild
  (JNIEnv *env, jobject jthis, jobject jnewChild, jobject joldChild)
{
  nsIDOMNode* node = (nsIDOMNode*) 
    env->GetLongField(jthis, JavaDOMGlobals::nodePtrFID);
  if (!node) {
    JavaDOMGlobals::ThrowException(env,
      "Node.replaceChild: NULL pointer");
    return NULL;
  }

  nsIDOMNode* newChild = (nsIDOMNode*) 
    env->GetLongField(jnewChild, JavaDOMGlobals::nodePtrFID);
  if (!newChild) {
    JavaDOMGlobals::ThrowException(env,
      "Node.replaceChild: NULL newChild pointer");
    return NULL;
  }

  nsIDOMNode* oldChild = (nsIDOMNode*) 
    env->GetLongField(joldChild, JavaDOMGlobals::nodePtrFID);
  if (!oldChild) {
    JavaDOMGlobals::ThrowException(env,
      "Node.replaceChild: NULL oldChild pointer");
    return NULL;
  }

  nsIDOMNode* ret = nsnull;
  nsresult rv = node->ReplaceChild(newChild, oldChild, &ret);
  if (NS_FAILED(rv) || !ret) {
    JavaDOMGlobals::ExceptionType exceptionType = JavaDOMGlobals::EXCEPTION_RUNTIME;
    if (NS_ERROR_GET_MODULE(rv) == NS_ERROR_MODULE_DOM &&
        (NS_ERROR_GET_CODE(rv) == NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR ||
         NS_ERROR_GET_CODE(rv) == NS_ERROR_DOM_WRONG_DOCUMENT_ERR ||
         NS_ERROR_GET_CODE(rv) == NS_ERROR_DOM_HIERARCHY_REQUEST_ERR ||
         NS_ERROR_GET_CODE(rv) == NS_ERROR_DOM_NOT_FOUND_ERR)) {
      exceptionType = JavaDOMGlobals::EXCEPTION_DOM;
    }
    JavaDOMGlobals::ThrowException(env,
      "Node.replaceChild: failed", rv, exceptionType);
    return NULL;
  }

  return JavaDOMGlobals::CreateNodeSubtype(env, ret);
}

/*
 * Class:     org_mozilla_dom_NodeImpl
 * Method:    setNodeValue
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_org_mozilla_dom_NodeImpl_setNodeValue
  (JNIEnv *env, jobject jthis, jstring jvalue)
{
  nsIDOMNode* node = (nsIDOMNode*) 
    env->GetLongField(jthis, JavaDOMGlobals::nodePtrFID);
  if (!node) {
    JavaDOMGlobals::ThrowException(env,
      "Node.setNodeValue: NULL pointer");
    return;
  }

  jboolean iscopy = JNI_FALSE;
  const char* value = env->GetStringUTFChars(jvalue, &iscopy);
  if (!value) {
    JavaDOMGlobals::ThrowException(env,
      "Node.setNodeValue: GetStringUTFChars failed");
    return;
  }

  nsresult rv = node->SetNodeValue(value);
  if (iscopy == JNI_TRUE)
    env->ReleaseStringUTFChars(jvalue, value);
  if (NS_FAILED(rv)) {
    JavaDOMGlobals::ExceptionType exceptionType = JavaDOMGlobals::EXCEPTION_RUNTIME;
    if (NS_ERROR_GET_MODULE(rv) == NS_ERROR_MODULE_DOM &&
        NS_ERROR_GET_CODE(rv) == NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR) {
      exceptionType = JavaDOMGlobals::EXCEPTION_DOM;
    }
    JavaDOMGlobals::ThrowException(env,
      "Node.setNodeValue: failed", rv, exceptionType);

    return;
  }
}
