/* -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
 * Contributor(s): 
 */
#include "nsCOMPtr.h"
#include "nsContentModule.h"
#include "nsIComponentManager.h"
#include "nsIDocumentLoader.h"
#include "nsIDocumentLoaderFactory.h"
#include "nsIDocument.h"
#include "nsIDocumentViewer.h"
#include "nsIURL.h"
#include "nsICSSStyleSheet.h"
#include "nsString.h"
#include "nsContentCID.h"
#include "prprf.h"
#include "nsNetUtil.h"
#include "nsICSSLoader.h"

#include "nsRDFCID.h"
#include "nsIRDFResource.h"
#include "nsIXULContentSink.h"
#include "nsIDocStreamLoaderFactory.h"

#define VIEW_SOURCE_HTML

// URL for the "user agent" style sheet
#define UA_CSS_URL "resource:/res/ua.css"

// Factory code for creating variations on html documents

#undef NOISY_REGISTRY

static NS_DEFINE_IID(kHTMLDocumentCID, NS_HTMLDOCUMENT_CID);
static NS_DEFINE_IID(kXMLDocumentCID, NS_XMLDOCUMENT_CID);
static NS_DEFINE_IID(kImageDocumentCID, NS_IMAGEDOCUMENT_CID);
static NS_DEFINE_IID(kXULDocumentCID, NS_XULDOCUMENT_CID);

extern nsresult NS_NewDocumentViewer(nsIDocumentViewer** aResult);

static char* gHTMLTypes[] = {
  "text/html",
  "text/plain",
  "text/css",
  0
};
  
static char* gXMLTypes[] = {
  "text/xml",
  "application/xml",
  "application/xhtml+xml",
  0
};


static char* gRDFTypes[] = {
  "text/rdf",
  "application/vnd.mozilla.xul+xml",
  "mozilla.application/cached-xul",
  0
};

static char* gImageTypes[] = {
  "image/gif",
  "image/jpeg",
  "image/jpg",
  "image/pjpeg",
  "image/png",
  "image/x-png",
  "image/x-art",
  "image/x-jg",
  0
};

class nsContentDLF : public nsIDocumentLoaderFactory,
                    public nsIDocStreamLoaderFactory
{
public:
  nsContentDLF();
  virtual ~nsContentDLF();

  NS_DECL_ISUPPORTS

  // for nsIDocumentLoaderFactory
  NS_IMETHOD CreateInstance(const char* aCommand,
                            nsIChannel* aChannel,
                            nsILoadGroup* aLoadGroup,
                            const char* aContentType, 
                            nsISupports* aContainer,
                            nsISupports* aExtraInfo,
                            nsIStreamListener** aDocListener,
                            nsIContentViewer** aDocViewer);

  NS_IMETHOD CreateInstanceForDocument(nsISupports* aContainer,
                                       nsIDocument* aDocument,
                                       const char *aCommand,
                                       nsIContentViewer** aDocViewerResult);

  // for nsIDocStreamLoaderFactory
  NS_METHOD CreateInstance(nsIInputStream& aInputStream,
                           const char* aContentType,
                           const char* aCommand,
                           nsISupports* aContainer,
                           nsISupports* aExtraInfo,
                           nsIContentViewer** aDocViewer);

  nsresult InitUAStyleSheet();

  nsresult CreateDocument(const char* aCommand,
                          nsIChannel* aChannel,
                          nsILoadGroup* aLoadGroup,
                          nsISupports* aContainer,
                          const nsCID& aDocumentCID,
                          nsIStreamListener** aDocListener,
                          nsIContentViewer** aDocViewer);

  nsresult CreateRDFDocument(const char* aCommand,
                             nsIChannel* aChannel,
                             nsILoadGroup* aLoadGroup,
                             const char* aContentType,
                             nsISupports* aContainer,
                             nsISupports* aExtraInfo,
                             nsIStreamListener** aDocListener,
                             nsIContentViewer** aDocViewer);

  nsresult CreateXULDocumentFromStream(nsIInputStream& aXULStream,
                                       const char* aCommand,
                                       nsISupports* aContainer,
                                       nsISupports* aExtraInfo,
                                       nsIContentViewer** aDocViewer);

  nsresult CreateRDFDocument(nsISupports*,
                             nsCOMPtr<nsIDocument>*,
                             nsCOMPtr<nsIDocumentViewer>*);

  static nsICSSStyleSheet* GetUAStyleSheet() {
    return nsContentModule::gUAStyleSheet;
  }
};

nsresult
NS_NewContentDocumentLoaderFactory(nsIDocumentLoaderFactory** aResult)
{
  NS_PRECONDITION(aResult, "null OUT ptr");
  if (!aResult) {
    return NS_ERROR_NULL_POINTER;
  }
  nsContentDLF* it = new nsContentDLF();
  if (!it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  return it->QueryInterface(NS_GET_IID(nsIDocumentLoaderFactory), (void**)aResult);
}

nsContentDLF::nsContentDLF()
{
  NS_INIT_REFCNT();
}

nsContentDLF::~nsContentDLF()
{
}

NS_IMPL_ADDREF(nsContentDLF)
NS_IMPL_RELEASE(nsContentDLF)

NS_IMETHODIMP
nsContentDLF::QueryInterface(REFNSIID aIID, void** aInstancePtrResult)
{
  if (NULL == aInstancePtrResult) {
    return NS_ERROR_NULL_POINTER;
  }

  static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);
  if (aIID.Equals(NS_GET_IID(nsIDocumentLoaderFactory))) {
    nsIDocumentLoaderFactory *tmp = this;
    *aInstancePtrResult = (void*) tmp;
    AddRef();
    return NS_OK;
  }
  if (aIID.Equals(NS_GET_IID(nsIDocStreamLoaderFactory))) {
    nsIDocStreamLoaderFactory *tmp = this;
    *aInstancePtrResult = (void*) tmp;
    AddRef();
    return NS_OK;
  }
  if (aIID.Equals(kISupportsIID)) {
    nsIDocumentLoaderFactory *tmp = this;
    nsISupports *tmp2 = tmp;
    *aInstancePtrResult = (void*) tmp2;
    AddRef();
    return NS_OK;
  }
  return NS_NOINTERFACE;
}

NS_IMETHODIMP
nsContentDLF::CreateInstance(const char *aCommand,
                            nsIChannel* aChannel,
                            nsILoadGroup* aLoadGroup,
                            const char* aContentType, 
                            nsISupports* aContainer,
                            nsISupports* aExtraInfo,
                            nsIStreamListener** aDocListener,
                            nsIContentViewer** aDocViewer)
{
  nsresult rv = NS_OK;
  if (!GetUAStyleSheet()) {
    // Load the UA style sheet
    nsCOMPtr<nsIURI> uaURL;
    rv = NS_NewURI(getter_AddRefs(uaURL), UA_CSS_URL);
    if (NS_SUCCEEDED(rv)) {
      nsCOMPtr<nsICSSLoader> cssLoader;
      rv = NS_NewCSSLoader(getter_AddRefs(cssLoader));
      if (cssLoader) {
        PRBool complete;
        rv = cssLoader->LoadAgentSheet(uaURL, nsContentModule::gUAStyleSheet, complete,
                                       nsnull);
      }
    }
    if (NS_FAILED(rv)) {
  #ifdef DEBUG
      printf("*** open of %s failed: error=%x\n", UA_CSS_URL, rv);
  #endif
      return rv;
    }
  }

  if(0==PL_strcmp(aCommand,"view-source")) {
#ifdef VIEW_SOURCE_HTML
    aContentType=gHTMLTypes[0];    
#else
    if(0==PL_strcmp(aContentType,gHTMLTypes[1])) {
      aContentType=gHTMLTypes[0];
    }
    else if(0==PL_strcmp(aContentType,gHTMLTypes[2])) {
      aContentType=gHTMLTypes[0];
    }
    else 
      aContentType=gXMLTypes[0];
#endif
  }

  // Try html
  int typeIndex=0;
  while(gHTMLTypes[typeIndex]) {
    if (0== PL_strcmp(gHTMLTypes[typeIndex++], aContentType)) {
      return CreateDocument(aCommand, 
                            aChannel, aLoadGroup,
                            aContainer, kHTMLDocumentCID,
                            aDocListener, aDocViewer);
    }
  }

  // Try XML
  typeIndex = 0;
  while(gXMLTypes[typeIndex]) {
    if (0== PL_strcmp(gXMLTypes[typeIndex++], aContentType)) {
      return CreateDocument(aCommand, 
                            aChannel, aLoadGroup,
                            aContainer, kXMLDocumentCID,
                            aDocListener, aDocViewer);
    }
  }

  // Try RDF
  typeIndex = 0;
  while (gRDFTypes[typeIndex]) {
    if (0 == PL_strcmp(gRDFTypes[typeIndex++], aContentType)) {
      return CreateRDFDocument(aCommand, 
                               aChannel, aLoadGroup,
                               aContentType, aContainer,
                               aExtraInfo, aDocListener, aDocViewer);
    }
  }

  // Try image types
  typeIndex = 0;
  while(gImageTypes[typeIndex]) {
    if (0== PL_strcmp(gImageTypes[typeIndex++], aContentType)) {
      return CreateDocument(aCommand, 
                            aChannel, aLoadGroup,
                            aContainer, kImageDocumentCID,
                            aDocListener, aDocViewer);
    }
  }

  // If we get here, then we weren't able to create anything. Sorry!
  return NS_ERROR_FAILURE;
}


NS_IMETHODIMP
nsContentDLF::CreateInstanceForDocument(nsISupports* aContainer,
                                       nsIDocument* aDocument,
                                       const char *aCommand,
                                       nsIContentViewer** aDocViewerResult)
{
  nsresult rv = NS_ERROR_FAILURE;  

  do {
    nsCOMPtr<nsIDocumentViewer> docv;
    // Create the document viewer
    rv = NS_NewDocumentViewer(getter_AddRefs(docv));
    if (NS_FAILED(rv))
      break;
    docv->SetUAStyleSheet(nsContentDLF::GetUAStyleSheet());

    // Bind the document to the Content Viewer
    rv = docv->LoadStart(aDocument);
    *aDocViewerResult = docv;
    NS_IF_ADDREF(*aDocViewerResult);
  } while (PR_FALSE);

  return rv;
}

nsresult
nsContentDLF::CreateDocument(const char* aCommand,
                            nsIChannel* aChannel,
                            nsILoadGroup* aLoadGroup,
                            nsISupports* aContainer,
                            const nsCID& aDocumentCID,
                            nsIStreamListener** aDocListener,
                            nsIContentViewer** aDocViewer)
{
  nsresult rv = NS_ERROR_FAILURE;

  nsCOMPtr<nsIURI> aURL;
  rv = aChannel->GetURI(getter_AddRefs(aURL));
  if (NS_FAILED(rv)) return rv;

#ifdef NOISY_CREATE_DOC
  if (nsnull != aURL) {
    nsAutoString tmp;
    aURL->ToString(tmp);
    fputs(tmp, stdout);
    printf(": creating document\n");
  }
#endif

  nsCOMPtr<nsIDocument> doc;
  nsCOMPtr<nsIDocumentViewer> docv;
  do {
    // Create the document
    rv = nsComponentManager::CreateInstance(aDocumentCID, nsnull,
                                            NS_GET_IID(nsIDocument),
                                            getter_AddRefs(doc));
    if (NS_FAILED(rv))
      break;

    // Create the document viewer  XXX: could reuse document viewer here!
    rv = NS_NewDocumentViewer(getter_AddRefs(docv));
    if (NS_FAILED(rv))
      break;
    docv->SetUAStyleSheet(nsContentDLF::GetUAStyleSheet());

    // Initialize the document to begin loading the data.  An
    // nsIStreamListener connected to the parser is returned in
    // aDocListener.
    rv = doc->StartDocumentLoad(aCommand, aChannel, aLoadGroup, aContainer, aDocListener, PR_TRUE);
    if (NS_FAILED(rv))
      break;

    // Bind the document to the Content Viewer
    rv = docv->LoadStart(doc);
    *aDocViewer = docv;
    NS_IF_ADDREF(*aDocViewer);
  } while (PR_FALSE);

  return rv;
}

NS_IMETHODIMP
nsContentDLF::CreateInstance(nsIInputStream& aInputStream,
                            const char* aContentType,
                            const char* aCommand,
                            nsISupports* aContainer,
                            nsISupports* aExtraInfo,
                            nsIContentViewer** aDocViewer)

{
  nsresult status = NS_ERROR_FAILURE;

  // Try RDF
  int typeIndex = 0;
  while (gRDFTypes[typeIndex]) {
    if (0 == PL_strcmp(gRDFTypes[typeIndex++], aContentType)) {
      return CreateXULDocumentFromStream(aInputStream,
                                         aCommand,
                                         aContainer,
                                         aExtraInfo,
                                         aDocViewer);
    }
  }

  return status;
}

// ...common work for |CreateRDFDocument| and |CreateXULDocumentFromStream|
nsresult
nsContentDLF::CreateRDFDocument(nsISupports* aExtraInfo,
                               nsCOMPtr<nsIDocument>* doc,
                               nsCOMPtr<nsIDocumentViewer>* docv)
{
  nsresult rv = NS_ERROR_FAILURE;
    
  // Create the XUL document
  rv = nsComponentManager::CreateInstance(kXULDocumentCID, nsnull,
                                          NS_GET_IID(nsIDocument),
                                          getter_AddRefs(*doc));
  if (NS_FAILED(rv)) return rv;

  // Create the image content viewer...
  rv = NS_NewDocumentViewer(getter_AddRefs(*docv));
  if (NS_FAILED(rv)) return rv;

  // Load the UA style sheet if we haven't already done that
  (*docv)->SetUAStyleSheet(nsContentDLF::GetUAStyleSheet());

  return NS_OK;
}

// ...note, this RDF document _may_ be XUL :-)
nsresult
nsContentDLF::CreateRDFDocument(const char* aCommand,
                               nsIChannel* aChannel,
                               nsILoadGroup* aLoadGroup,
                               const char* aContentType,
                               nsISupports* aContainer,
                               nsISupports* aExtraInfo,
                               nsIStreamListener** aDocListener,
                               nsIContentViewer** aDocViewer)
{
  nsCOMPtr<nsIDocument> doc;
  nsCOMPtr<nsIDocumentViewer> docv;
  nsresult rv = CreateRDFDocument(aExtraInfo, address_of(doc), address_of(docv));
  if (NS_FAILED(rv)) {
    return rv;
  }

  nsCOMPtr<nsIURI> aURL;
  rv = aChannel->GetURI(getter_AddRefs(aURL));
  if (NS_FAILED(rv)) return rv;

  /* 
   * Initialize the document to begin loading the data...
   *
   * An nsIStreamListener connected to the parser is returned in
   * aDocListener.
   */
  rv = doc->StartDocumentLoad(aCommand, aChannel, aLoadGroup, aContainer, aDocListener, PR_TRUE);
  if (NS_SUCCEEDED(rv)) {
    /*
     * Bind the document to the Content Viewer...
     */
    rv = docv->LoadStart(doc);
    *aDocViewer = docv;
    NS_IF_ADDREF(*aDocViewer);
  }
   
  return rv;
}

nsresult
nsContentDLF::CreateXULDocumentFromStream(nsIInputStream& aXULStream,
                                         const char* aCommand,
                                         nsISupports* aContainer,
                                         nsISupports* aExtraInfo,
                                         nsIContentViewer** aDocViewer)
{
  nsresult status = NS_OK;

#if 0 // XXX dead code; remove
  do
  {
    nsCOMPtr<nsIDocument> doc;
    nsCOMPtr<nsIDocumentViewer> docv;
    if ( NS_FAILED(status = CreateRDFDocument(aExtraInfo, address_of(doc), address_of(docv))) )
      break;

    if ( NS_FAILED(status = docv->LoadStart(doc)) )
      break;

    *aDocViewer = docv;
    NS_IF_ADDREF(*aDocViewer);

    nsCOMPtr<nsIStreamLoadableDocument> loader = do_QueryInterface(doc, &status);
    if ( NS_FAILED(status) )
      break;

    status = loader->LoadFromStream(aXULStream, aContainer, aCommand);
  }
  while (0);
#endif

  return status;
}

nsresult
nsContentModule::RegisterDocumentFactories(nsIComponentManager* aCompMgr,
                                          nsIFile* aPath)
{
  // We do not register view or view-source document factories because
  // that is for layout. Now load-as-data might be a different story...
  return NS_OK;
}

static NS_DEFINE_IID(kDocumentFactoryImplCID, NS_CONTENT_DOCUMENT_LOADER_FACTORY_CID);

void
nsContentModule::UnregisterDocumentFactories(nsIComponentManager* aCompMgr,
                                            nsIFile* aPath)
{
  aCompMgr->UnregisterComponentSpec(kDocumentFactoryImplCID, aPath);
}
