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
#include "nslayout.h"
#include "nsContentModule.h"
#include "nsIFactory.h"
#include "nsISupports.h"
#include "nsContentCID.h"
#include "nsIDocument.h"
#include "nsIHTMLContent.h"
#include "nsITextContent.h"
#include "nsIPresShell.h"
#include "nsIPresContext.h"
#include "nsISelection.h"
#include "nsIFrameUtil.h"

#include "nsHTMLAtoms.h"
#include "nsHTMLParts.h"
#include "nsDOMCID.h"
#include "nsIServiceManager.h"
#include "nsICSSParser.h"
#include "nsIHTMLStyleSheet.h"
#include "nsIHTMLCSSStyleSheet.h"
#include "nsICSSStyleSheet.h"
#include "nsICSSLoader.h"
#include "nsIDOMRange.h"
#include "nsIContentIterator.h"
#include "nsINameSpaceManager.h"
#include "nsIEventListenerManager.h"
#include "nsILayoutDebugger.h"
#include "nsIElementFactory.h"
#include "nsIDocumentEncoder.h"
#include "nsCOMPtr.h"
#include "nsIFrameSelection.h"
#include "nsIDOMDOMImplementation.h"
#include "nsIPrivateDOMImplementation.h"
#include "nsIDocumentViewer.h"
#include "nsIHTMLAttributes.h"

#include "nsIXBLService.h"
#include "nsIBindingManager.h"

#include "nsContentPolicyUtils.h"

#include "nsXMLContentSerializer.h"
#include "nsHTMLContentSerializer.h"
#include "nsPlainTextSerializer.h"

#include "nsINodeInfo.h"
#include "nsIComputedDOMStyle.h"

#include "nsEventStateManager.h"
#include "nsIGenericFactory.h"
#include "nsContentHTTPStartup.h"

class nsIDocumentLoaderFactory;

static NS_DEFINE_CID(kComponentManagerCID, NS_COMPONENTMANAGER_CID);

static NS_DEFINE_IID(kHTMLDocumentCID, NS_HTMLDOCUMENT_CID);
static NS_DEFINE_IID(kXMLDocumentCID, NS_XMLDOCUMENT_CID);
static NS_DEFINE_CID(kXMLElementFactoryCID, NS_XML_ELEMENT_FACTORY_CID);
static NS_DEFINE_IID(kImageDocumentCID, NS_IMAGEDOCUMENT_CID);
static NS_DEFINE_IID(kCSSParserCID,     NS_CSSPARSER_CID);
static NS_DEFINE_CID(kHTMLCSSStyleSheetCID, NS_HTML_CSS_STYLESHEET_CID);
static NS_DEFINE_CID(kHTMLStyleSheetCID, NS_HTMLSTYLESHEET_CID);
static NS_DEFINE_CID(kStyleSetCID, NS_STYLESET_CID);
static NS_DEFINE_CID(kCSSStyleSheetCID, NS_CSS_STYLESHEET_CID);
static NS_DEFINE_CID(kCSSLoaderCID, NS_CSS_LOADER_CID);
static NS_DEFINE_IID(kHTMLImageElementCID, NS_HTMLIMAGEELEMENT_CID);
static NS_DEFINE_IID(kHTMLHrElementCID, NS_HTMLHRELEMENT_CID);
static NS_DEFINE_IID(kHTMLInputElementCID, NS_HTMLINPUTELEMENT_CID);
static NS_DEFINE_IID(kHTMLOptionElementCID, NS_HTMLOPTIONELEMENT_CID);

static NS_DEFINE_IID(kAnonymousElementCID, NS_ANONYMOUSCONTENT_CID);

static NS_DEFINE_IID(kHTMLAttributesCID, NS_HTMLATTRIBUTES_CID);

static NS_DEFINE_CID(kSelectionCID, NS_SELECTION_CID);
static NS_DEFINE_IID(kFrameSelectionCID, NS_FRAMESELECTION_CID);
static NS_DEFINE_IID(kDOMSelectionCID, NS_DOMSELECTION_CID);

static NS_DEFINE_IID(kRangeCID,     NS_RANGE_CID);
static NS_DEFINE_IID(kAttributeContentCID, NS_ATTRIBUTECONTENT_CID);
static NS_DEFINE_IID(kContentIteratorCID, NS_CONTENTITERATOR_CID);
static NS_DEFINE_IID(kGeneratedContentIteratorCID, NS_GENERATEDCONTENTITERATOR_CID);
static NS_DEFINE_IID(kGeneratedSubtreeIteratorCID, NS_GENERATEDSUBTREEITERATOR_CID);
static NS_DEFINE_IID(kSubtreeIteratorCID, NS_SUBTREEITERATOR_CID);

static NS_DEFINE_CID(kTextNodeCID,   NS_TEXTNODE_CID);
static NS_DEFINE_CID(kNameSpaceManagerCID,  NS_NAMESPACEMANAGER_CID);
static NS_DEFINE_CID(kFrameUtilCID,  NS_FRAME_UTIL_CID);
static NS_DEFINE_CID(kEventListenerManagerCID, NS_EVENTLISTENERMANAGER_CID);
static NS_DEFINE_CID(kEventStateManagerCID, NS_EVENTSTATEMANAGER_CID);

static NS_DEFINE_CID(kDocumentViewerCID, NS_DOCUMENT_VIEWER_CID);
static NS_DEFINE_CID(kContentDocumentLoaderFactoryCID, NS_CONTENT_DOCUMENT_LOADER_FACTORY_CID);
static NS_DEFINE_CID(kLayoutDebuggerCID, NS_LAYOUT_DEBUGGER_CID);
static NS_DEFINE_CID(kHTMLElementFactoryCID, NS_HTML_ELEMENT_FACTORY_CID);
static NS_DEFINE_CID(kTextEncoderCID, NS_TEXT_ENCODER_CID);
static NS_DEFINE_CID(kHTMLCopyTextEncoderCID, NS_HTMLCOPY_TEXT_ENCODER_CID);

static NS_DEFINE_CID(kXMLContentSerializerCID, NS_XMLCONTENTSERIALIZER_CID);
static NS_DEFINE_CID(kHTMLContentSerializerCID, NS_HTMLCONTENTSERIALIZER_CID);
static NS_DEFINE_CID(kPlainTextSerializerCID, NS_PLAINTEXTSERIALIZER_CID);

static NS_DEFINE_CID(kXBLServiceCID, NS_XBLSERVICE_CID);
static NS_DEFINE_CID(kBindingManagerCID, NS_BINDINGMANAGER_CID);

static NS_DEFINE_CID(kDOMImplementationCID, NS_DOM_IMPLEMENTATION_CID);
static NS_DEFINE_CID(kNodeInfoManagerCID, NS_NODEINFOMANAGER_CID);
static NS_DEFINE_CID(kContentPolicyCID, NS_CONTENTPOLICY_CID);
static NS_DEFINE_CID(kComputedDOMStyleCID, NS_COMPUTEDDOMSTYLE_CID);

static NS_DEFINE_CID(kControllerCommandManagerCID, NS_CONTROLLERCOMMANDMANAGER_CID);
static NS_DEFINE_CID(kContentHTTPStartupCID, NS_CONTENTHTTPSTARTUP_CID);

extern nsresult NS_NewSelection(nsIFrameSelection** aResult);
extern nsresult NS_NewDomSelection(nsISelection** aResult);
extern nsresult NS_NewDocumentViewer(nsIDocumentViewer** aResult);
extern nsresult NS_NewRange(nsIDOMRange** aResult);
extern nsresult NS_NewContentIterator(nsIContentIterator** aResult);
extern nsresult NS_NewGenRegularIterator(nsIContentIterator** aResult);
extern nsresult NS_NewContentSubtreeIterator(nsIContentIterator** aResult);
extern nsresult NS_NewGenSubtreeIterator(nsIContentIterator** aInstancePtrResult);

extern nsresult NS_NewContentDocumentLoaderFactory(nsIDocumentLoaderFactory** aResult);
extern nsresult NS_NewHTMLElementFactory(nsIElementFactory** aResult);
extern nsresult NS_NewXMLElementFactory(nsIElementFactory** aResult);

extern nsresult NS_NewHTMLCopyTextEncoder(nsIDocumentEncoder** aResult);
extern nsresult NS_NewTextEncoder(nsIDocumentEncoder** aResult);

extern nsresult NS_NewXBLService(nsIXBLService** aResult);

extern nsresult NS_NewBindingManager(nsIBindingManager** aResult);

extern nsresult NS_NewNodeInfoManager(nsINodeInfoManager** aResult);

extern nsresult NS_NewContentPolicy(nsIContentPolicy** aResult);

NS_GENERIC_FACTORY_CONSTRUCTOR(nsContentHTTPStartup)

#ifdef MOZ_XUL
#include "nsIXULSortService.h"
#include "nsIRDFContentModelBuilder.h"
#include "nsIXULContentSink.h"
#include "nsIXULDocument.h"
#include "nsIXULPopupListener.h"
#include "nsIXULPrototypeCache.h"
#include "nsIController.h"
#include "nsIControllers.h"
#include "nsIControllerCommand.h"

static NS_DEFINE_CID(kXULSortServiceCID,     NS_XULSORTSERVICE_CID);
static NS_DEFINE_CID(kXULTemplateBuilderCID, NS_XULTEMPLATEBUILDER_CID);
static NS_DEFINE_CID(kXULOutlinerBuilderCID, NS_XULOUTLINERBUILDER_CID);
static NS_DEFINE_CID(kXULContentSinkCID,     NS_XULCONTENTSINK_CID);
static NS_DEFINE_CID(kXULDocumentCID,        NS_XULDOCUMENT_CID);
static NS_DEFINE_CID(kXULPopupListenerCID,   NS_XULPOPUPLISTENER_CID);
static NS_DEFINE_CID(kXULElementFactoryCID,  NS_XULELEMENTFACTORY_CID);
static NS_DEFINE_CID(kXULPrototypeCacheCID,  NS_XULPROTOTYPECACHE_CID);
static NS_DEFINE_CID(kXULControllersCID,     NS_XULCONTROLLERS_CID);

extern nsresult NS_NewXULElementFactory(nsIElementFactory** aResult);
extern NS_IMETHODIMP NS_NewXULControllers(nsISupports* aOuter, REFNSIID aIID, void** aResult);
#endif


//----------------------------------------------------------------------

nsContentFactory::nsContentFactory(const nsCID &aClass)
{
  NS_INIT_ISUPPORTS();
  mClassID = aClass;
#if 0
  char* cs = aClass.ToString();
  printf("+++ Creating Content factory for %s\n", cs);
  nsCRT::free(cs);
#endif
}

nsContentFactory::~nsContentFactory()
{
#if 0
  char* cs = mClassID.ToString();
  printf("+++ Destroying Content factory for %s\n", cs);
  nsCRT::free(cs);
#endif
}

NS_IMPL_ISUPPORTS(nsContentFactory, NS_GET_IID(nsIFactory))

#ifdef DEBUG
#define LOG_NEW_FAILURE(_msg,_ec)                                           \
  printf("nsContentFactory::CreateInstance failed for %s: error=%d(0x%x)\n", \
         _msg, _ec, _ec)
#else
#define LOG_NEW_FAILURE(_msg,_ec)
#endif

nsresult
nsContentFactory::CreateInstance(nsISupports *aOuter,
                                const nsIID &aIID,
                                void **aResult)
{
  nsresult res;

  if (aResult == NULL) {
    return NS_ERROR_NULL_POINTER;
  }

  *aResult = NULL;

  if (aOuter) {
    return NS_ERROR_NO_AGGREGATION;
  }

  nsISupports *inst = nsnull;

  // XXX ClassID check happens here
  if (mClassID.Equals(kHTMLDocumentCID)) {
    res = NS_NewHTMLDocument((nsIDocument **)&inst);
    if (NS_FAILED(res)) {
      LOG_NEW_FAILURE("NS_NewHTMLDocument", res);
      return res;
    }
  }
  else if (mClassID.Equals(kXMLDocumentCID)) {
    res = NS_NewXMLDocument((nsIDocument **)&inst);
    if (NS_FAILED(res)) {
      LOG_NEW_FAILURE("NS_NewXMLDocument", res);
      return res;
    }
  }
  else if (mClassID.Equals(kImageDocumentCID)) {
    res = NS_NewImageDocument((nsIDocument **)&inst);
    if (NS_FAILED(res)) {
      LOG_NEW_FAILURE("NS_NewImageDocument", res);
      return res;
    }
  }
  else if (mClassID.Equals(kHTMLAttributesCID)) {
    res = NS_NewHTMLAttributes((nsIHTMLAttributes**)&inst);
    if (NS_FAILED(res)) {
      LOG_NEW_FAILURE("NS_NewHTMLAttributes", res);
      return res;
    }
  }
#if 1
// XXX replace these with nsIElementFactory calls
  else if (mClassID.Equals(kHTMLImageElementCID)) {
    // Note! NS_NewHTMLImageElement is special cased to handle a null nodeinfo
    res = NS_NewHTMLImageElement((nsIHTMLContent**)&inst, nsnull);
    if (NS_FAILED(res)) {
      LOG_NEW_FAILURE("NS_NewHTMLImageElement", res);
      return res;
    }
  }
  else if (mClassID.Equals(kHTMLOptionElementCID)) {
    // Note! NS_NewHTMLOptionElement is special cased to handle a null nodeinfo
    res = NS_NewHTMLOptionElement((nsIHTMLContent**)&inst, nsnull);
    if (NS_FAILED(res)) {
      LOG_NEW_FAILURE("NS_NewHTMLOptionElement", res);
      return res;
    }
  }
  else if (mClassID.Equals(kHTMLInputElementCID)) {
    // Note! NS_NewHTMLOptionElement is special cased to handle a null nodeinfo
    res = NS_NewHTMLInputElement((nsIHTMLContent**)&inst, nsnull);
    if (NS_FAILED(res)) {
      LOG_NEW_FAILURE("NS_NewHTMLInputElement", res);
      return res;
    }
  }
#endif
  else if (mClassID.Equals(kFrameSelectionCID)) {
    res = NS_NewSelection((nsIFrameSelection**)&inst);
    if (NS_FAILED(res)) {
      LOG_NEW_FAILURE("NS_NewSelection", res);
      return res;
    }
  }
  else if (mClassID.Equals(kDOMSelectionCID)) {
    res = NS_NewDomSelection((nsISelection**)&inst);
    if (NS_FAILED(res)) {
      LOG_NEW_FAILURE("NS_NewDomSelection", res);
      return res;
    }
  }
  else if (mClassID.Equals(kRangeCID)) {
    res = NS_NewRange((nsIDOMRange **)&inst);
    if (NS_FAILED(res)) {
      LOG_NEW_FAILURE("NS_NewRange", res);
      return res;
    }
  }
  else if (mClassID.Equals(kAttributeContentCID)) {
    res = NS_NewAttributeContent((nsIContent **)&inst);
    if (NS_FAILED(res)) {
      LOG_NEW_FAILURE("NS_NewAttributeContent", res);
      return res;
    }
  }
  else if (mClassID.Equals(kContentIteratorCID)) {
    res = NS_NewContentIterator((nsIContentIterator **)&inst);
    if (NS_FAILED(res)) {
      LOG_NEW_FAILURE("NS_NewContentIterator", res);
      return res;
    }
  }
  else if (mClassID.Equals(kGeneratedContentIteratorCID)) {
    res = NS_NewGenRegularIterator((nsIContentIterator **)&inst);
    if (NS_FAILED(res)) {
      LOG_NEW_FAILURE("NS_NewGenRegularIterator", res);
      return res;
    }
  }
  else if (mClassID.Equals(kSubtreeIteratorCID)) {
    res = NS_NewContentSubtreeIterator((nsIContentIterator **)&inst);
    if (NS_FAILED(res)) {
      LOG_NEW_FAILURE("NS_NewContentSubtreeIterator", res);
      return res;
    }
  }
  else if (mClassID.Equals(kGeneratedSubtreeIteratorCID)) {
    res = NS_NewGenSubtreeIterator((nsIContentIterator **)&inst);
    if (NS_FAILED(res)) {
      LOG_NEW_FAILURE("NS_NewGenSubtreeIterator", res);
      return res;
    }
  }
  else if (mClassID.Equals(kCSSParserCID)) {
    res = NS_NewCSSParser((nsICSSParser**)&inst);
    if (NS_FAILED(res)) {
      LOG_NEW_FAILURE("NS_NewCSSParser", res);
      return res;
    }
  }
  else if (mClassID.Equals(kHTMLCSSStyleSheetCID)) {
    res = NS_NewHTMLCSSStyleSheet((nsIHTMLCSSStyleSheet**)&inst);
    if (NS_FAILED(res)) {
      LOG_NEW_FAILURE("NS_NewHTMLCSSStyleSheet", res);
      return res;
    }
  }
  else if (mClassID.Equals(kCSSStyleSheetCID)) {
    res = NS_NewCSSStyleSheet((nsICSSStyleSheet**)&inst);
    if (NS_FAILED(res)) {
      LOG_NEW_FAILURE("NS_NewCSSStyleSheet", res);
      return res;
    }
  }
  else if (mClassID.Equals(kStyleSetCID)) {
    res = NS_NewStyleSet((nsIStyleSet**)&inst);
    if (NS_FAILED(res)) {
      LOG_NEW_FAILURE("NS_NewStyleSet", res);
      return res;
    }
  }
  else if (mClassID.Equals(kHTMLStyleSheetCID)) {
    res = NS_NewHTMLStyleSheet((nsIHTMLStyleSheet**)&inst);
    if (NS_FAILED(res)) {
      LOG_NEW_FAILURE("NS_NewHTMLStyleSheet", res);
      return res;
    }
  }
  else if (mClassID.Equals(kCSSLoaderCID)) {
    res = NS_NewCSSLoader((nsICSSLoader**)&inst);
    if (NS_FAILED(res)) {
      LOG_NEW_FAILURE("NS_NewCSSLoader", res);
      return res;
    }
  }
  else if (mClassID.Equals(kTextNodeCID)) {
    res = NS_NewTextNode((nsIContent**) &inst);
    if (NS_FAILED(res)) {
      LOG_NEW_FAILURE("NS_NewTextNode", res);
      return res;
    }
  }
  else if (mClassID.Equals(kNameSpaceManagerCID)) {
    res = NS_NewNameSpaceManager((nsINameSpaceManager**)&inst);
    if (NS_FAILED(res)) {
      LOG_NEW_FAILURE("NS_NewNameSpaceManager", res);
      return res;
    }
  }
  else if (mClassID.Equals(kEventListenerManagerCID)) {
    res = NS_NewEventListenerManager((nsIEventListenerManager**) &inst);
    if (NS_FAILED(res)) {
      LOG_NEW_FAILURE("NS_NewEventListenerManager", res);
      return res;
    }
  }
  else if (mClassID.Equals(kEventStateManagerCID)) {
    res = NS_NewEventStateManager((nsIEventStateManager**) &inst);
    if (NS_FAILED(res)) {
      LOG_NEW_FAILURE("NS_NewEventStateManager", res);
      return res;
    }
  }
  else if (mClassID.Equals(kContentDocumentLoaderFactoryCID)) {
    res = NS_NewContentDocumentLoaderFactory((nsIDocumentLoaderFactory**)&inst);
    if (NS_FAILED(res)) {
      LOG_NEW_FAILURE("NS_NewContentDocumentLoaderFactory", res);
      return res;
    }
  }
  else if (mClassID.Equals(kHTMLElementFactoryCID)) {
    res = NS_NewHTMLElementFactory((nsIElementFactory**) &inst);
    if (NS_FAILED(res)) {
      LOG_NEW_FAILURE("NS_NewHTMLElementFactory", res);
      return res;
    }
  }
  else if (mClassID.Equals(kXMLElementFactoryCID)) {
    res = NS_NewXMLElementFactory((nsIElementFactory**) &inst);
    if (NS_FAILED(res)) {
      LOG_NEW_FAILURE("NS_NewXMLElementFactory", res);
      return res;
    }
  }
  else if (mClassID.Equals(kTextEncoderCID)) {
    res = NS_NewTextEncoder((nsIDocumentEncoder**) &inst);
    if (NS_FAILED(res)) {
      LOG_NEW_FAILURE("NS_NewTextEncoder", res);
      return res;
    }
  }
  else if (mClassID.Equals(kHTMLCopyTextEncoderCID)) {
    res = NS_NewHTMLCopyTextEncoder((nsIDocumentEncoder**) &inst);
    if (NS_FAILED(res)) {
      LOG_NEW_FAILURE("NS_NewHTMLCopyTextEncoder", res);
      return res;
    }
  }
  else if (mClassID.Equals(kXMLContentSerializerCID)) {
    res = NS_NewXMLContentSerializer((nsIContentSerializer**) &inst);
    if (NS_FAILED(res)) {
      LOG_NEW_FAILURE("NS_NewXMLContentSerializer", res);
      return res;
    }
  }
  else if (mClassID.Equals(kHTMLContentSerializerCID)) {
    res = NS_NewHTMLContentSerializer((nsIContentSerializer**) &inst);
    if (NS_FAILED(res)) {
      LOG_NEW_FAILURE("NS_NewHTMLContentSerializer", res);
      return res;
    }
  }
  else if (mClassID.Equals(kPlainTextSerializerCID)) {
    res = NS_NewPlainTextSerializer((nsIContentSerializer**) &inst);
    if (NS_FAILED(res)) {
      LOG_NEW_FAILURE("NS_NewPlainTextSerializer", res);
      return res;
    }
  }
  else if (mClassID.Equals(kXBLServiceCID)) {
    res = NS_NewXBLService((nsIXBLService**) &inst);
    if (NS_FAILED(res)) {
      LOG_NEW_FAILURE("NS_NewXBLService", res);
      return res;
    }
  }
  else if (mClassID.Equals(kBindingManagerCID)) {
    res = NS_NewBindingManager((nsIBindingManager**) &inst);
    if (NS_FAILED(res)) {
      LOG_NEW_FAILURE("NS_NewBindingManager", res);
      return res;
    }
  }
  else if (mClassID.Equals(kNodeInfoManagerCID)) {
    res = NS_NewNodeInfoManager((nsINodeInfoManager**) &inst);
    if (NS_FAILED(res)) {
      LOG_NEW_FAILURE("NS_NewNodeInfoManager", res);
      return res;
    }
  }
  else if (mClassID.Equals(kDOMImplementationCID)) {
    res = NS_NewDOMImplementation((nsIDOMDOMImplementation**) &inst);
    if (NS_FAILED(res)) {
      LOG_NEW_FAILURE("NS_NewDOMImplementation", res);
      return res;
    }
  }
  else if (mClassID.Equals(kDocumentViewerCID)) {
    res = NS_NewDocumentViewer((nsIDocumentViewer**) &inst);
    if (NS_FAILED(res)) {
      LOG_NEW_FAILURE("NS_NewDocumentViewer", res);
      return res;
    }
  }
  else if (mClassID.Equals(kContentPolicyCID)) {
    res = NS_NewContentPolicy((nsIContentPolicy**) &inst);
    if (NS_FAILED(res)) {
      LOG_NEW_FAILURE("NS_NewContentPolicy", res);
      return res;
    }
  }
  else if (mClassID.Equals(kComputedDOMStyleCID)) {
    res = NS_NewComputedDOMStyle((nsIComputedDOMStyle**) &inst);
    if (NS_FAILED(res)) {
      LOG_NEW_FAILURE("NS_NewComputedDOMStyle", res);
      return res;
    }
  }
  else if (mClassID.Equals(kContentHTTPStartupCID)) {
    res = nsContentHTTPStartupConstructor(aOuter, NS_GET_IID(nsISupports),
                                          (void **)&inst);
    if (NS_FAILED(res)) {
      LOG_NEW_FAILURE("nsContentHTTPStartupConstructor", res);
      return res;
    }
  }
#if defined(MOZ_XUL)
  else if (mClassID.Equals(kXULSortServiceCID)) {
    res = NS_NewXULSortService((nsIXULSortService**) &inst);
    if (NS_FAILED(res)) {
      LOG_NEW_FAILURE("NS_NewXULSortService", res);
      return res;
    }
  }
  else if (mClassID.Equals(kXULTemplateBuilderCID)) {
    res = NS_NewXULContentBuilder(nsnull, aIID, (void**) &inst);
    if (NS_FAILED(res)) {
      LOG_NEW_FAILURE("NS_NewXULContentBuilder", res);
      return res;
    }
  }
  else if (mClassID.Equals(kXULOutlinerBuilderCID)) {
    res = NS_NewXULOutlinerBuilder(nsnull, aIID, (void**) &inst);
    if (NS_FAILED(res)) {
      LOG_NEW_FAILURE("NS_NewXULOutlinerBuilder", res);
      return res;
    }
  }
  else if (mClassID.Equals(kXULContentSinkCID)) {
    res = NS_NewXULContentSink((nsIXULContentSink**) &inst);
    if (NS_FAILED(res)) {
      LOG_NEW_FAILURE("NS_NewXULContentSink", res);
      return res;
    }
  }
  else if (mClassID.Equals(kXULDocumentCID)) {
    res = NS_NewXULDocument((nsIXULDocument**) &inst);
    if (NS_FAILED(res)) {
      LOG_NEW_FAILURE("NS_NewXULDocument", res);
      return res;
    }
  }
  else if (mClassID.Equals(kXULPopupListenerCID)) {
    res = NS_NewXULPopupListener((nsIXULPopupListener**) &inst);
    if (NS_FAILED(res)) {
      LOG_NEW_FAILURE("NS_NewXULPopupListener", res);
      return res;
    }
  }
  else if (mClassID.Equals(kXULElementFactoryCID)) {
    res = NS_NewXULElementFactory((nsIElementFactory**) &inst);
    if (NS_FAILED(res)) {
      LOG_NEW_FAILURE("NS_NewXULElementFactory", res);
      return res;
    }
  }
  else if (mClassID.Equals(kXULPrototypeCacheCID)) {
    res = NS_NewXULPrototypeCache(nsnull, NS_GET_IID(nsIXULPrototypeCache), (void**) &inst);
    if (NS_FAILED(res)) {
      LOG_NEW_FAILURE("NS_NewXULPrototypeCache", res);
      return res;
    }
  }
  else if (mClassID.Equals(kXULControllersCID)) {
    res = NS_NewXULControllers(nsnull, NS_GET_IID(nsIControllers), (void**) &inst);
    if (NS_FAILED(res)) {
      LOG_NEW_FAILURE("NS_NewXULControllers", res);
      return res;
    }
  }
  else if (mClassID.Equals(kControllerCommandManagerCID)) {
    res = NS_NewControllerCommandManager((nsIControllerCommandManager**) &inst);
    if (NS_FAILED(res)) {
      LOG_NEW_FAILURE("NS_NewControllerCommandManager", res);
      return res;
    }
  }
#endif
  else {
    return NS_NOINTERFACE;
  }

  if (NS_FAILED(res)) return res;

  res = inst->QueryInterface(aIID, aResult);

  NS_RELEASE(inst);

  if (NS_FAILED(res)) {
    // We didn't get the right interface, so clean up
    LOG_NEW_FAILURE("final QueryInterface", res);
  }

  return res;
}

nsresult nsContentFactory::LockFactory(PRBool aLock)
{
  // Not implemented in simplest case.
  return NS_OK;
}
