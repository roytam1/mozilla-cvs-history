/* 
 The contents of this file are subject to the Mozilla Public
 License Version 1.1 (the "License"); you may not use this file
 except in compliance with the License. You may obtain a copy of
 the License at http://www.mozilla.org/MPL/

 Software distributed under the License is distributed on an "AS
 IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 implied. See the License for the specific language governing
 rights and limitations under the License.

 The Original Code is mozilla.org code.

 The Initial Developer of the Original Code is Sun Microsystems,
 Inc. Portions created by Sun are
 Copyright (C) 1999 Sun Microsystems, Inc. All
 Rights Reserved.

 Contributor(s): 
*/

#include "prenv.h"
#include "nsISupportsUtils.h"
#include "nsIURL.h"
#include "nsIDocument.h"
#include "nsIDocumentLoader.h"
#include "nsIDocumentLoaderObserver.h"
#include "nsIDocumentViewer.h"
#include "nsIDOMDocument.h"
#include "nsIWebShell.h"
#include "nsJavaDOMImpl.h"

#ifdef JAVA_DOM_OJI_ENABLE
#include "ProxyJNI.h"
#include "nsIServiceManager.h"
#endif

#if defined(DEBUG)
#include <stdio.h>
#include "nsIDOMElement.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMNamedNodeMap.h"
static void dump_document(nsIDOMDocument* dom, const char* urlSpec);
static void dump_node(FILE* of, nsIDOMNode* node, int indent, 
		      PRBool isMapNode);
static void dump_map(FILE* of, nsIDOMNamedNodeMap* map, int indent);
static char* strip_whitespace(const PRUnichar* input, int length);
static const char* describe_type(int type);
#endif

#ifdef JAVA_DOM_OJI_ENABLE
static NS_DEFINE_CID(kJVMManagerCID,NS_JVMMANAGER_CID);
#endif
static NS_DEFINE_IID(kIWebShellIID, NS_IWEB_SHELL_IID);
static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);
static NS_DEFINE_IID(kIJavaDOMIID, NS_IJAVADOM_IID);
static NS_DEFINE_IID(kIDocumentViewerIID, NS_IDOCUMENT_VIEWER_IID);
static NS_DEFINE_IID(kIDOMDocumentIID, NS_IDOMDOCUMENT_IID);
static NS_DEFINE_IID(kIDocumentLoaderObserverIID, NS_IDOCUMENT_LOADER_OBSERVER_IID);

NS_IMPL_ADDREF(nsJavaDOMImpl);
NS_IMPL_RELEASE(nsJavaDOMImpl);

NS_IMETHODIMP nsJavaDOMImpl::QueryInterface(REFNSIID aIID, void** aInstance)
{
  if (NULL == aInstance)
    return NS_ERROR_NULL_POINTER;

  *aInstance = NULL;

  if (aIID.Equals(kIJavaDOMIID)) {
    *aInstance = (void*) ((nsIJavaDOM*)this);
    NS_ADDREF_THIS();
    return NS_OK;
  }
  if (aIID.Equals(kIDocumentLoaderObserverIID)) {
    *aInstance = (void*) ((nsIDocumentLoaderObserver*)this);
    NS_ADDREF_THIS();
    return NS_OK;
  }
  if (aIID.Equals(kISupportsIID)) {
    *aInstance = (void*) ((nsISupports*)this);
    NS_ADDREF_THIS();
    return NS_OK;
  }

  return NS_NOINTERFACE;
}

#ifdef JAVA_DOM_OJI_ENABLE
nsJVMManager* nsJavaDOMImpl::jvmManager = NULL;
JavaDOMSecurityContext* nsJavaDOMImpl::securityContext = NULL;
#else
JavaVM* nsJavaDOMImpl::jvm = NULL;
#endif

jclass nsJavaDOMImpl::domAccessorClass = NULL;
jclass nsJavaDOMImpl::documentClass = NULL;
jclass nsJavaDOMImpl::listenerClass = NULL;
jclass nsJavaDOMImpl::gcClass = NULL;

jobject nsJavaDOMImpl::docListener = NULL;

jfieldID nsJavaDOMImpl::documentPtrFID = NULL;

jmethodID nsJavaDOMImpl::documentInitID = NULL;
jmethodID nsJavaDOMImpl::getInstanceMID = NULL;
jmethodID nsJavaDOMImpl::startURLLoadMID = NULL;
jmethodID nsJavaDOMImpl::endURLLoadMID = NULL;
jmethodID nsJavaDOMImpl::progressURLLoadMID = NULL;
jmethodID nsJavaDOMImpl::statusURLLoadMID = NULL;
jmethodID nsJavaDOMImpl::startDocumentLoadMID = NULL;
jmethodID nsJavaDOMImpl::endDocumentLoadMID = NULL;

jmethodID nsJavaDOMImpl::gcMID = NULL;

nsJavaDOMImpl::nsJavaDOMImpl() 
{
  NS_INIT_ISUPPORTS();

  JNIEnv* env = GetJNIEnv();

  gcClass = env->FindClass("org/mozilla/dom/DOMGarbageCollector");
  if (!gcClass) return;
  gcClass = (jclass) env->NewGlobalRef(gcClass);
  if (!gcClass) return;

  gcMID = 
    env->GetStaticMethodID(gcClass,
			   "doGC",
			   "()V");
  if (!gcMID) return;

  domAccessorClass = env->FindClass("org/mozilla/dom/DOMAccessorImpl");
  if (!domAccessorClass) return;
  domAccessorClass = (jclass) env->NewGlobalRef(domAccessorClass);
  if (!domAccessorClass) return;

  getInstanceMID = 
    env->GetStaticMethodID(domAccessorClass, 
			   "getInstance",
			   "()Lorg/mozilla/dom/DOMAccessor;");
  if (!getInstanceMID) return;

  docListener = 
    env->CallStaticObjectMethod(domAccessorClass, getInstanceMID);
  if (!docListener) return;
  docListener = (jclass) env->NewGlobalRef(docListener);
  if (!docListener) return;

  listenerClass = 
    env->GetObjectClass(docListener);
  if (!listenerClass) return;
  listenerClass = (jclass) env->NewGlobalRef(listenerClass);
  if (!listenerClass) return;

  documentClass = 
    env->FindClass("org/mozilla/dom/DocumentImpl");
  if (!documentClass) return;
  documentClass = (jclass) env->NewGlobalRef(documentClass);
  if (!documentClass) return;

  documentInitID =
    env->GetMethodID(documentClass, "<init>", "(J)V");
  if (!documentInitID) return;

  documentPtrFID = 
    env->GetFieldID(documentClass,
		    "p_nsIDOMNode", 
		    "J");
  if (!documentPtrFID) return;

  startURLLoadMID = 
    env->GetMethodID(listenerClass,
		     "startURLLoad",
		     "(Ljava/lang/String;Ljava/lang/String;Lorg/w3c/dom/Document;)V");
  if (!startURLLoadMID) return;

  endURLLoadMID = 
    env->GetMethodID(listenerClass,
		     "endURLLoad",
		     "(Ljava/lang/String;ILorg/w3c/dom/Document;)V");
  if (!endURLLoadMID) return;

  progressURLLoadMID = 
    env->GetMethodID(listenerClass,
		     "progressURLLoad",
		     "(Ljava/lang/String;IILorg/w3c/dom/Document;)V");
  if (!progressURLLoadMID) return;

  statusURLLoadMID = 
    env->GetMethodID(listenerClass,
		     "statusURLLoad",
		     "(Ljava/lang/String;Ljava/lang/String;Lorg/w3c/dom/Document;)V");
  if (!statusURLLoadMID) return;

  startDocumentLoadMID = 
    env->GetMethodID(listenerClass,
		     "startDocumentLoad",
		     "(Ljava/lang/String;)V");
  if (!startDocumentLoadMID) return;

  endDocumentLoadMID = 
    env->GetMethodID(listenerClass,
		     "endDocumentLoad",
		     "(Ljava/lang/String;ILorg/w3c/dom/Document;)V");
  if (!endDocumentLoadMID) return;

  Cleanup(env);
}

nsJavaDOMImpl::~nsJavaDOMImpl()
{
}

PRBool nsJavaDOMImpl::Cleanup(JNIEnv* env) 
{
  if (env->ExceptionOccurred()) {
    env->ExceptionDescribe();
    return PR_TRUE;
  }

  return PR_FALSE;
}

nsIDOMDocument* nsJavaDOMImpl::GetDocument(nsIDocumentLoader* loader)
{
  nsIWebShell* webshell = nsnull;
  nsISupports* container = nsnull;
  nsIContentViewer* contentv = nsnull;
  nsIDocumentViewer* docv = nsnull;
  nsIDocument* document = nsnull;
  nsIDOMDocument* domDoc = nsnull;

  nsresult rv = loader->GetContainer(&container);
  if (NS_SUCCEEDED(rv) && container)
    rv = container->QueryInterface(kIWebShellIID, (void**) &webshell);
      if (NS_SUCCEEDED(rv) && webshell) 
        rv = webshell->GetContentViewer(&contentv);

  if (NS_SUCCEEDED(rv) && contentv) {
    rv = contentv->QueryInterface(kIDocumentViewerIID,
				       (void**) &docv);
    if (NS_SUCCEEDED(rv) && docv) {
      docv->GetDocument(document);
      rv = document->QueryInterface(kIDOMDocumentIID, 
				    (void**) &domDoc);
      if (NS_SUCCEEDED(rv) && docv) {
	return domDoc;
      }
    }
  }

  fprintf(stderr, 
	  "nsJavaDOMImpl::GetDocument: failed: "
	  "container=%x, webshell=%x, contentViewer=%x, "
	  "documentViewer=%x, document=%x, "
	  "domDocument=%x, error=%x\n", 
	  (unsigned) (void*) container, 
	  (unsigned) (void*) webshell, 
	  (unsigned) (void*) contentv, 
	  (unsigned) (void*) docv, 
	  (unsigned) (void*) document, 
	  (unsigned) (void*) domDoc, 
	  rv);
  return NULL;
}

// convert nsIDOMDocument to org.mozilla.dom.DocumentImpl
jobject nsJavaDOMImpl::CaffienateDOMDocument(nsIDOMDocument* domDoc)
{
  if (!domDoc) return NULL;
  JNIEnv* env = GetJNIEnv();

//    jobject jdoc = env->AllocObject(documentClass);
  jobject jdoc = env->NewObject(documentClass, 
                                documentInitID, 
                                (jlong) (void*) domDoc);
  if (!jdoc) return NULL;
//    env->SetLongField(jdoc, documentPtrFID, (jlong) (void*) domDoc);
  if (Cleanup(env) == PR_TRUE) return NULL;

  return jdoc;
}

/* nsIDocumentLoaderObserver methods */

NS_IMETHODIMP nsJavaDOMImpl::OnStartDocumentLoad(nsIDocumentLoader* loader, 
						 nsIURI* aURL, 
						 const char* aCommand)
{
  JNIEnv* env = GetJNIEnv();
  char* urlSpec = (char*) "";
  if (aURL)
    aURL->GetSpec(&urlSpec);
  jstring jURL = env->NewStringUTF(urlSpec);
  if (!jURL) return NS_ERROR_FAILURE;

  env->CallVoidMethod(docListener,
		      startDocumentLoadMID,
		      jURL);
  if (Cleanup(env) == PR_TRUE) return NS_ERROR_FAILURE;

  env->CallStaticVoidMethod(gcClass, gcMID);
  Cleanup(env);
  return NS_OK;
}

NS_IMETHODIMP nsJavaDOMImpl::OnEndDocumentLoad(nsIDocumentLoader* loader,
					       nsIChannel* channel, 
					       nsresult aStatus)
{
  JNIEnv* env = GetJNIEnv();
  char* urlSpec = (char*) "";
  nsIURI* url = nsnull;
  if (channel && NS_SUCCEEDED(channel->GetURI(&url)))
    url->GetSpec(&urlSpec);
  jstring jURL = env->NewStringUTF(urlSpec);
  if (!jURL) return NS_ERROR_FAILURE;

  nsIDOMDocument* domDoc = GetDocument(loader);
  jobject jdoc = CaffienateDOMDocument(domDoc);

  env->CallVoidMethod(docListener,
		      endDocumentLoadMID,
		      jURL,
		      (jint) aStatus,
		      jdoc);
  if (Cleanup(env) == PR_TRUE) return NS_ERROR_FAILURE;

  env->CallStaticVoidMethod(gcClass, gcMID);
  Cleanup(env);
  return NS_OK;
}

NS_IMETHODIMP nsJavaDOMImpl::OnStartURLLoad(nsIDocumentLoader* loader,
					    nsIChannel* channel)
{
  JNIEnv* env = GetJNIEnv();
  char* urlSpec = (char*) "";
  nsIURI* url = nsnull;
  if (channel && NS_SUCCEEDED(channel->GetURI(&url)))
    url->GetSpec(&urlSpec);
  jstring jURL = env->NewStringUTF(urlSpec);
  if (!jURL) return NS_ERROR_FAILURE;

  char* contentType = (char*) "";
  if (channel)
      channel->GetContentType(&contentType);
  jstring jContentType = env->NewStringUTF(contentType);
  if (!jContentType) return NS_ERROR_FAILURE;

  nsIDOMDocument* domDoc = GetDocument(loader);
  jobject jdoc = CaffienateDOMDocument(domDoc);

  env->CallVoidMethod(docListener,
		      startURLLoadMID,
		      jURL,
		      jContentType,
		      jdoc);
  if (Cleanup(env) == PR_TRUE) return NS_ERROR_FAILURE;

  env->CallStaticVoidMethod(gcClass, gcMID);
  Cleanup(env);
  return NS_OK;
}

NS_IMETHODIMP nsJavaDOMImpl::OnProgressURLLoad(nsIDocumentLoader* loader,
					       nsIChannel* channel, 
					       PRUint32 aProgress, 
					       PRUint32 aProgressMax)
{
  JNIEnv* env = GetJNIEnv();
  char* urlSpec = (char*) "";
  nsIURI* url = nsnull;
  if (channel && NS_SUCCEEDED(channel->GetURI(&url)))
    url->GetSpec(&urlSpec);
  jstring jURL = env->NewStringUTF(urlSpec);
  if (!jURL) return NS_ERROR_FAILURE;

  nsIDOMDocument* domDoc = GetDocument(loader);
  jobject jdoc = CaffienateDOMDocument(domDoc);

  env->CallVoidMethod(docListener,
		      progressURLLoadMID,
		      jURL,
		      (jint) aProgress,
		      (jint) aProgressMax,
		      jdoc);
  if (Cleanup(env) == PR_TRUE) return NS_ERROR_FAILURE;

  env->CallStaticVoidMethod(gcClass, gcMID);
  Cleanup(env);
  return NS_OK;
}

NS_IMETHODIMP nsJavaDOMImpl::OnStatusURLLoad(nsIDocumentLoader* loader, 
					     nsIChannel* channel, 
					     nsString& aMsg)
{
  JNIEnv* env = GetJNIEnv();
  char* urlSpec = (char*) "";
  nsIURI* url = nsnull;
  if (channel && NS_SUCCEEDED(channel->GetURI(&url)))
    url->GetSpec(&urlSpec);
  jstring jURL = env->NewStringUTF(urlSpec);
  if (!jURL) return NS_ERROR_FAILURE;

  const PRUnichar* cMsg = aMsg.GetUnicode();
  jstring jMessage = env->NewString(cMsg, aMsg.Length());
  if (!jMessage) return NS_ERROR_FAILURE;

  nsIDOMDocument* domDoc = GetDocument(loader);
  jobject jdoc = CaffienateDOMDocument(domDoc);

  env->CallVoidMethod(docListener,
		      statusURLLoadMID,
		      jURL,
		      jMessage,
		      jdoc);
  if (Cleanup(env) == PR_TRUE) return NS_ERROR_FAILURE;

  env->CallStaticVoidMethod(gcClass, gcMID);
  Cleanup(env);
  return NS_OK;
}

NS_IMETHODIMP nsJavaDOMImpl::OnEndURLLoad(nsIDocumentLoader* loader, 
					  nsIChannel* channel, 
					  nsresult aStatus)
{
  JNIEnv* env = GetJNIEnv();
  char* urlSpec = (char*) "";
  nsIURI* url = nsnull;
  if (channel && NS_SUCCEEDED(channel->GetURI(&url)))
    url->GetSpec(&urlSpec);
  jstring jURL = env->NewStringUTF(urlSpec);
  if (!jURL) return NS_ERROR_FAILURE;

  nsIDOMDocument* domDoc = GetDocument(loader);
  jobject jdoc = CaffienateDOMDocument(domDoc);

#if defined(DEBUG)
  dump_document(domDoc, urlSpec);
#endif

  env->CallVoidMethod(docListener,
		      endURLLoadMID,
		      jURL,
		      (jint) aStatus,
		      jdoc);
  if (Cleanup(env) == PR_TRUE) return NS_ERROR_FAILURE;

  env->CallStaticVoidMethod(gcClass, gcMID);
  Cleanup(env);
  return NS_OK;
}

NS_IMETHODIMP nsJavaDOMImpl::HandleUnknownContentType(nsIDocumentLoader* loader,
						      nsIChannel* channel, 
						      const char *aContentType,
						      const char *aCommand)
{
  JNIEnv* env = GetJNIEnv();
  env->CallStaticVoidMethod(gcClass, gcMID);
  Cleanup(env);
  return NS_OK;
}


JNIEnv* nsJavaDOMImpl::GetJNIEnv() {
   JNIEnv* env;
#ifdef JAVA_DOM_OJI_ENABLE
   nsresult result;
   if (!jvmManager) {
       NS_WITH_SERVICE(nsIJVMManager, _jvmManager, kJVMManagerCID, &result);
       if (NS_SUCCEEDED(result)) {
           jvmManager = (nsJVMManager*)((nsIJVMManager*)_jvmManager);
       }
   }
   if (!jvmManager) {
       return NULL;
   }
   jvmManager->CreateProxyJNI(NULL,&env);
//     if (!securityContext) {
//         securityContext = new JavaDOMSecurityContext();
//     }
   //   SetSecurityContext(env,securityContext);
   SetSecurityContext(env, new JavaDOMSecurityContext());
#else  /* JAVA_DOM_OJI_ENABLE */
   if (!jvm) {
	StartJVM();
   }
#ifdef XP_PC
   jvm->AttachCurrentThread((void**)&env,NULL);
#else
   jvm->AttachCurrentThread(&env,NULL);
#endif
#endif /* JAVA_DOM_OJI_ENABLE */
   return env;
}

#ifndef JAVA_DOM_OJI_ENABLE
void nsJavaDOMImpl::StartJVM(void) {
  JNIEnv *env = NULL;	
  JDK1_1InitArgs vm_args;
  JNI_GetDefaultJavaVMInitArgs(&vm_args);
  vm_args.version = 0x00010001;
  vm_args.verifyMode = JNI_TRUE;
#ifdef DEBUG
  vm_args.verbose = JNI_TRUE;
  vm_args.enableVerboseGC = JNI_TRUE;
#endif // DEBUG
  char* cp = PR_GetEnv("CLASSPATH");
  char* p = new char[strlen(cp) + strlen(vm_args.classpath) + 2];
  strcpy(p, vm_args.classpath);
  if (cp) {
#ifdef XP_PC
    strcat(p, ";");
#else
    strcat(p, ":");
#endif
    strcat(p, cp);
    vm_args.classpath = p;
  }

#ifdef DISABLE_JIT
   /* workaround to get java dom to work on Linux */
   char **props = new char*[2];
   props[0]="java.compiler=";  
   props[1]=0;
   vm_args.properties = props;
#endif

#ifdef DEBUG
  printf("classpath is \"%s\"\n", vm_args.classpath);
#endif // DEBUG
#ifdef XP_PC
  jint rv = JNI_CreateJavaVM(&jvm, (void**)&env, &vm_args);
#else
  jint rv = JNI_CreateJavaVM(&jvm, &env, &vm_args);
#endif
  if (rv < 0) {
    printf("\n JAVA DOM: could not start jvm\n");
  } else {
    printf("\n JAVA DOM: successfully started jvm\n");
  }
  delete[] p;
}
#endif /* JAVA_DOM_OJI_ENABLE */


#if defined(DEBUG)
static void dump_document(nsIDOMDocument* domDoc, const char* urlSpec)
{
  if (!domDoc) return;

  if (strstr(urlSpec, ".xul") ||
      strstr(urlSpec, ".js") ||
      strstr(urlSpec, ".css") ||
      strstr(urlSpec, "file:")) return;

  FILE* of = fopen("dom-cpp.txt", "a");
  if (!of) {
    perror("nsJavaDOMImpl::dump_document: failed to open output file\n");
    return;
  }

  nsIDOMElement* element = nsnull;
  nsresult rv = domDoc->GetDocumentElement(&element);
  if (NS_FAILED(rv) || !element)
    return;

  (void) element->Normalize();

  fprintf(of, "\n+++ %s +++\n", urlSpec);
  dump_node(of, (nsIDOMNode*) element, 0, PR_FALSE);
  fprintf(of, "\n");
  fflush(of);
  fclose(of);
}

static void dump_node(FILE* of, nsIDOMNode* node, int indent, 
		      PRBool isMapNode)
{
  if (!node) return;

  fprintf(of, "\n");
  for (int i=0; i < indent; i++)
    fprintf(of, " ");

  nsString name;
  nsString value;
  PRUint16 type;

  node->GetNodeName(name);
  node->GetNodeValue(value);
  node->GetNodeType(&type);

  const PRUnichar* cname = name.GetUnicode();
  const PRUnichar* cvalue = value.GetUnicode();
  char* cnorm = strip_whitespace(cvalue, value.Length());
  fprintf(of, "name=\"%s\" type=%s value=\"%s\"", 
	  cname, describe_type(type), cnorm);
  delete[] cnorm;

  if (isMapNode) return;

  nsIDOMNamedNodeMap* map = nsnull;
  node->GetAttributes(&map);
  dump_map(of, map, indent);

  nsIDOMNodeList* children = nsnull;
  node->GetChildNodes(&children);
  if (!children) return;

  PRUint32 length=0;
  children->GetLength(&length);
  for (PRUint32 j=0; j < length; j++) {
    nsIDOMNode* child = nsnull;
    children->Item(j, &child);
    dump_node(of, child, indent+2, PR_FALSE);
  }
}

static void dump_map(FILE* of, nsIDOMNamedNodeMap* map, int indent)
{
  if (!map) return;

  fprintf(of, "\n");
  PRUint32 length=0;
  map->GetLength(&length);
  if (length > 0) {
    for (int i=0; i < indent; i++)
      fprintf(of, " ");
    fprintf(of, "------- start attributes -------");
  }

  for (PRUint32 j=0; j < length; j++) {
    nsIDOMNode* node = nsnull;
    map->Item(j, &node);
    dump_node(of, node, indent, PR_TRUE);
  }

  if (length > 0) {
    fprintf(of, "\n");
    for (int k=0; k < indent; k++)
      fprintf(of, " ");
    fprintf(of, "------- end attributes -------");
  }
}

static char* strip_whitespace(const PRUnichar* input, int length)
{
  if (!input || length < 1) {
    char* out = new char[1];
    out[0] = 0;
    return out;
  }

  char* out = new char[length+1];
  char* op = out;
  const PRUnichar* ip = input;
  PRUnichar c = ' ';
  PRUnichar pc = ' ';
  int i = 0;

  for (c = *ip++; i++<length; c = *ip++) {
    if ((pc == ' ' || pc == '\n' || pc == '\t') &&
	(c == ' ' || c == '\n' || c == '\t'))
      continue;
    *op++ = (char)c;
    pc = c;
  }
  *op++ = 0;

  return out;
}

static const char* describe_type(int type)
{
  switch (type) {
  case nsIDOMNode::ELEMENT_NODE: return "ELEMENT";
  case nsIDOMNode::ATTRIBUTE_NODE: return "ATTRIBUTE";
  case nsIDOMNode::TEXT_NODE: return "TEXT";
  case nsIDOMNode::CDATA_SECTION_NODE: return "CDATA_SECTION";
  case nsIDOMNode::ENTITY_REFERENCE_NODE: return "ENTITY_REFERENCE";
  case nsIDOMNode::ENTITY_NODE: return "ENTITY";
  case nsIDOMNode::PROCESSING_INSTRUCTION_NODE: return "PROCESSING_INSTRUCTION";
  case nsIDOMNode::COMMENT_NODE: return "COMMENT";
  case nsIDOMNode::DOCUMENT_NODE: return "DOCUMENT";
  case nsIDOMNode::DOCUMENT_TYPE_NODE: return "DOCUMENT_TYPE";
  case nsIDOMNode::DOCUMENT_FRAGMENT_NODE: return "DOCUMENT_FRAGMENT";
  case nsIDOMNode::NOTATION_NODE: return "NOTATION";
  }
  return "ERROR";
}
#endif

nsresult NS_NewJavaDOM(nsIJavaDOM** aJavaDOM)
{
  if (! aJavaDOM)
    return NS_ERROR_NULL_POINTER;

  *aJavaDOM = new nsJavaDOMImpl();
  if (! *aJavaDOM)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(*aJavaDOM);

  return NS_OK;
}
