/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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

/*

  An implementation for the XUL document. This implementation serves
  as the basis for generating an NGLayout content model.

  To Do
  -----

  1. Implement DOM range constructors.

  2. Implement XIF conversion (this is really low priority).

  Notes
  -----

  1. We do some monkey business in the document observer methods to
     keep the element map in sync for HTML elements. Why don't we just
     do it for _all_ elements? Well, in the case of XUL elements,
     which may be lazily created during frame construction, the
     document observer methods will never be called because we'll be
     adding the XUL nodes into the content model "quietly".

  2. The "element map" maps an RDF resource to the elements whose 'id'
     or 'ref' attributes refer to that resource. We re-use the element
     map to support the HTML-like 'getElementById()' method.

*/

// Note the ALPHABETICAL ORDERING
#include "nsXULDocument.h"

#include "nsDOMCID.h"
#include "nsIBrowserWindow.h"
#include "nsIChromeRegistry.h"
#include "nsIComponentManager.h"
#include "nsIContentViewer.h"
#include "nsICSSStyleSheet.h"
#include "nsIDOMEvent.h"
#include "nsIDOMEventListener.h"
#include "nsIDOMEventReceiver.h"
#include "nsIDOMScriptObjectFactory.h"
#include "nsIDOMStyleSheetCollection.h"
#include "nsIDOMText.h"
#include "nsIDOMXULElement.h"
#include "nsIDTD.h"
#include "nsIDocumentObserver.h"
#include "nsIFormControl.h"
#include "nsIHTMLContent.h"
#include "nsIHTMLElementFactory.h"
#include "nsIInputStream.h"
#include "nsILoadGroup.h"
#include "nsINameSpace.h"
#include "nsIParser.h"
#include "nsIPresContext.h"
#include "nsIPresShell.h"
#include "nsIPrincipal.h"
#include "nsIPrivateDOMEvent.h"
#include "nsIRDFCompositeDataSource.h"
#include "nsIRDFContainerUtils.h"
#include "nsIRDFContentModelBuilder.h"
#include "nsIRDFNode.h"
#include "nsIRDFRemoteDataSource.h"
#include "nsIRDFService.h"
#include "nsIScriptContextOwner.h"
#include "nsIScriptGlobalObject.h"
#include "nsIServiceManager.h"
#include "nsIStreamListener.h"
#include "nsIStyleContext.h"
#include "nsIStyleSet.h"
#include "nsIStyleSheet.h"
#include "nsITextContent.h"
#include "nsITimer.h"
#include "nsIURL.h"
#include "nsIWebShell.h"
#include "nsIBaseWindow.h"
#include "nsIXMLContent.h"
#include "nsIXMLElementFactory.h"
#include "nsIXULCommandDispatcher.h"
#include "nsIXULContent.h"
#include "nsIXULContentSink.h"
#include "nsIXULContentUtils.h"
#include "nsIXULKeyListener.h"
#include "nsIXULPrototypeCache.h"
#include "nsLWBrkCIID.h"
#include "nsLayoutCID.h"
#include "nsNetUtil.h"
#include "nsParserCIID.h"
#include "nsRDFCID.h"
#include "nsRDFDOMNodeList.h"
#include "nsXPIDLString.h"
#include "nsXULDocument.h"
#include "nsXULElement.h"
#include "plstr.h"
#include "prlog.h"
#include "rdf.h"
#include "rdfutil.h"
#include "nsIFrame.h"

//----------------------------------------------------------------------
//
// CIDs
//

static NS_DEFINE_CID(kCSSLoaderCID,              NS_CSS_LOADER_CID);
static NS_DEFINE_CID(kCSSParserCID,              NS_CSSPARSER_CID);
static NS_DEFINE_CID(kChromeRegistryCID,         NS_CHROMEREGISTRY_CID);
static NS_DEFINE_CID(kDOMScriptObjectFactoryCID, NS_DOM_SCRIPT_OBJECT_FACTORY_CID);
static NS_DEFINE_CID(kEventListenerManagerCID,   NS_EVENTLISTENERMANAGER_CID);
static NS_DEFINE_CID(kHTMLCSSStyleSheetCID,      NS_HTML_CSS_STYLESHEET_CID);
static NS_DEFINE_CID(kHTMLElementFactoryCID,     NS_HTML_ELEMENT_FACTORY_CID);
static NS_DEFINE_CID(kHTMLStyleSheetCID,         NS_HTMLSTYLESHEET_CID);
static NS_DEFINE_CID(kLWBrkCID,                  NS_LWBRK_CID);
static NS_DEFINE_CID(kLoadGroupCID,              NS_LOADGROUP_CID);
static NS_DEFINE_CID(kLocalStoreCID,             NS_LOCALSTORE_CID);
static NS_DEFINE_CID(kNameSpaceManagerCID,       NS_NAMESPACEMANAGER_CID);
static NS_DEFINE_CID(kParserCID,                 NS_PARSER_IID); // XXX
static NS_DEFINE_CID(kPresShellCID,              NS_PRESSHELL_CID);
static NS_DEFINE_CID(kRDFCompositeDataSourceCID, NS_RDFCOMPOSITEDATASOURCE_CID);
static NS_DEFINE_CID(kRDFContainerUtilsCID,      NS_RDFCONTAINERUTILS_CID);
static NS_DEFINE_CID(kRDFInMemoryDataSourceCID,  NS_RDFINMEMORYDATASOURCE_CID);
static NS_DEFINE_CID(kRDFServiceCID,             NS_RDFSERVICE_CID);
static NS_DEFINE_CID(kRDFXMLDataSourceCID,       NS_RDFXMLDATASOURCE_CID);
static NS_DEFINE_CID(kTextNodeCID,               NS_TEXTNODE_CID);
static NS_DEFINE_CID(kWellFormedDTDCID,          NS_WELLFORMEDDTD_CID);
static NS_DEFINE_CID(kXMLElementFactoryCID,      NS_XML_ELEMENT_FACTORY_CID);
static NS_DEFINE_CID(kXULCommandDispatcherCID,   NS_XULCOMMANDDISPATCHER_CID);
static NS_DEFINE_CID(kXULContentSinkCID,         NS_XULCONTENTSINK_CID);
static NS_DEFINE_CID(kXULContentUtilsCID,        NS_XULCONTENTUTILS_CID);
static NS_DEFINE_CID(kXULKeyListenerCID,         NS_XULKEYLISTENER_CID);
static NS_DEFINE_CID(kXULPrototypeCacheCID,      NS_XULPROTOTYPECACHE_CID);
static NS_DEFINE_CID(kXULTemplateBuilderCID,     NS_XULTEMPLATEBUILDER_CID);

static NS_DEFINE_IID(kIParserIID, NS_IPARSER_IID);

//----------------------------------------------------------------------
//
// Miscellaneous Constants
//

#define XUL_NAMESPACE_URI "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul"

const nsForwardReference::Phase nsForwardReference::kPasses[] = {
    nsForwardReference::eConstruction,
    nsForwardReference::eHookup,
    nsForwardReference::eDone
};


//----------------------------------------------------------------------
//
// Statics
//

PRInt32 nsXULDocument::gRefCnt = 0;

nsIAtom* nsXULDocument::kAttributeAtom;
nsIAtom* nsXULDocument::kCommandUpdaterAtom;
nsIAtom* nsXULDocument::kDataSourcesAtom;
nsIAtom* nsXULDocument::kElementAtom;
nsIAtom* nsXULDocument::kIdAtom;
nsIAtom* nsXULDocument::kKeysetAtom;
nsIAtom* nsXULDocument::kObservesAtom;
nsIAtom* nsXULDocument::kOpenAtom;
nsIAtom* nsXULDocument::kOverlayAtom;
nsIAtom* nsXULDocument::kPersistAtom;
nsIAtom* nsXULDocument::kPositionAtom;
nsIAtom* nsXULDocument::kRefAtom;
nsIAtom* nsXULDocument::kRuleAtom;
nsIAtom* nsXULDocument::kTemplateAtom;

nsIRDFService* nsXULDocument::gRDFService;
nsIRDFResource* nsXULDocument::kNC_persist;
nsIRDFResource* nsXULDocument::kNC_attribute;
nsIRDFResource* nsXULDocument::kNC_value;

nsIHTMLElementFactory* nsXULDocument::gHTMLElementFactory;
nsIXMLElementFactory*  nsXULDocument::gXMLElementFactory;

nsINameSpaceManager* nsXULDocument::gNameSpaceManager;
PRInt32 nsXULDocument::kNameSpaceID_XUL;

nsIXULContentUtils* nsXULDocument::gXULUtils;
nsIXULPrototypeCache* nsXULDocument::gXULCache;

PRLogModuleInfo* nsXULDocument::gXULLog;


//----------------------------------------------------------------------
//
// PlaceholderChannel
//
//   This is a dummy channel implementation that we add to the load
//   group. It ensures that EndDocumentLoad() in the webshell doesn't
//   fire before we've finished building the complete document content
//   model.
//

class PlaceholderChannel : public nsIChannel
{
protected:
    PlaceholderChannel();
    virtual ~PlaceholderChannel();

    static PRInt32 gRefCnt;
    static nsIURI* gURI;

    nsCOMPtr<nsILoadGroup> mLoadGroup;

public:
    static nsresult
    Create(nsIChannel** aResult);
	
    NS_DECL_ISUPPORTS

	// nsIRequest
    NS_IMETHOD IsPending(PRBool *_retval) { *_retval = PR_TRUE; return NS_OK; }
    NS_IMETHOD Cancel(void)  { return NS_OK; }
    NS_IMETHOD Suspend(void) { return NS_OK; }
    NS_IMETHOD Resume(void)  { return NS_OK; }

	// nsIChannel    
    NS_IMETHOD GetOriginalURI(nsIURI * *aOriginalURI) { *aOriginalURI = gURI; NS_ADDREF(*aOriginalURI); return NS_OK; }
    NS_IMETHOD GetURI(nsIURI * *aURI) { *aURI = gURI; NS_ADDREF(*aURI); return NS_OK; }
    NS_IMETHOD OpenInputStream(PRUint32 startPosition, PRInt32 readCount, nsIInputStream **_retval) { *_retval = nsnull; return NS_OK; }
	NS_IMETHOD OpenOutputStream(PRUint32 startPosition, nsIOutputStream **_retval) { *_retval = nsnull; return NS_OK; }
	NS_IMETHOD AsyncOpen(nsIStreamObserver *observer, nsISupports *ctxt) { return NS_OK; }
	NS_IMETHOD AsyncRead(PRUint32 startPosition, PRInt32 readCount, nsISupports *ctxt, nsIStreamListener *listener) { return NS_OK; }
	NS_IMETHOD AsyncWrite(nsIInputStream *fromStream, PRUint32 startPosition, PRInt32 writeCount, nsISupports *ctxt, nsIStreamObserver *observer) { return NS_OK; }
	NS_IMETHOD GetLoadAttributes(nsLoadFlags *aLoadAttributes) { *aLoadAttributes = nsIChannel::LOAD_NORMAL; return NS_OK; }
  	NS_IMETHOD SetLoadAttributes(nsLoadFlags aLoadAttributes) { return NS_OK; }
	NS_IMETHOD GetContentType(char * *aContentType) { *aContentType = nsnull; return NS_OK; }
	NS_IMETHOD GetContentLength(PRInt32 *aContentLength) { *aContentLength = 0; return NS_OK; }
	NS_IMETHOD GetOwner(nsISupports * *aOwner) { *aOwner = nsnull; return NS_OK; }
	NS_IMETHOD SetOwner(nsISupports * aOwner) { return NS_OK; }
	NS_IMETHOD GetLoadGroup(nsILoadGroup * *aLoadGroup) { *aLoadGroup = mLoadGroup; NS_IF_ADDREF(*aLoadGroup); return NS_OK; }
	NS_IMETHOD SetLoadGroup(nsILoadGroup * aLoadGroup) { mLoadGroup = aLoadGroup; return NS_OK; }
	NS_IMETHOD GetNotificationCallbacks(nsIInterfaceRequestor * *aNotificationCallbacks) { *aNotificationCallbacks = nsnull; return NS_OK; }
	NS_IMETHOD SetNotificationCallbacks(nsIInterfaceRequestor * aNotificationCallbacks) { return NS_OK; }
};

PRInt32 PlaceholderChannel::gRefCnt;
nsIURI* PlaceholderChannel::gURI;

NS_IMPL_ADDREF(PlaceholderChannel);
NS_IMPL_RELEASE(PlaceholderChannel);
NS_IMPL_QUERY_INTERFACE2(PlaceholderChannel, nsIRequest, nsIChannel);

nsresult
PlaceholderChannel::Create(nsIChannel** aResult)
{
    PlaceholderChannel* channel = new PlaceholderChannel();
    if (! channel)
        return NS_ERROR_OUT_OF_MEMORY;

    *aResult = channel;
    NS_ADDREF(*aResult);
    return NS_OK;
}


PlaceholderChannel::PlaceholderChannel()
{
    NS_INIT_REFCNT();

    if (gRefCnt++ == 0) {
        nsresult rv;
        rv = NS_NewURI(&gURI, "about:xul-master-placeholder", nsnull);
        NS_ASSERTION(NS_SUCCEEDED(rv), "unable to create about:xul-master-placeholder");
    }
}


PlaceholderChannel::~PlaceholderChannel()
{
    if (--gRefCnt == 0) {
        NS_IF_RELEASE(gURI);
    }
}


//----------------------------------------------------------------------
//
// ctors & dtors
//

nsXULDocument::nsXULDocument(void)
    : mParentDocument(nsnull),
      mScriptContextOwner(nsnull),
      mScriptObject(nsnull),
      mNextSrcLoadWaiter(nsnull),
      mCharSetID("UTF-8"),
      mDisplaySelection(PR_FALSE),
      mIsPopup(PR_FALSE),
      mResolutionPhase(nsForwardReference::eStart),
      mState(eState_Master),
      mCurrentScriptProto(nsnull)
{
    NS_INIT_REFCNT();
}

nsXULDocument::~nsXULDocument()
{
    NS_ASSERTION(mNextSrcLoadWaiter == nsnull,
        "unreferenced document still waiting for script source to load?");

    // In case we failed somewhere early on and the forward observer
    // decls never got resolved.
    DestroyForwardReferences();

    // mParentDocument is never refcounted
    // Delete references to sub-documents
    {
        PRInt32 i = mSubDocuments.Count();
        while (--i >= 0) {
            nsIDocument* subdoc = (nsIDocument*) mSubDocuments.ElementAt(i);
            NS_RELEASE(subdoc);
        }
    }

    // Delete references to style sheets but only if we aren't a popup document.
    if (!mIsPopup) {
        PRInt32 i = mStyleSheets.Count();
        while (--i >= 0) {
            nsIStyleSheet* sheet = (nsIStyleSheet*) mStyleSheets.ElementAt(i);
            sheet->SetOwningDocument(nsnull);
            NS_RELEASE(sheet);
        }
    }

    // set all builder references to document to nsnull -- out of band notification
    // to break ownership cycle
    if (mBuilders)
    {
        PRUint32 cnt = 0;
        nsresult rv = mBuilders->Count(&cnt);
        NS_ASSERTION(NS_SUCCEEDED(rv), "Count failed");

#ifdef DEBUG
        printf("# of builders: %lu\n", (unsigned long)cnt);
#endif

        for (PRUint32 i = 0; i < cnt; ++i) {
          nsIRDFContentModelBuilder* builder
            = (nsIRDFContentModelBuilder*) mBuilders->ElementAt(i);

          NS_ASSERTION(builder != nsnull, "null ptr");
          if (! builder) continue;

          rv = builder->SetDocument(nsnull);
          NS_ASSERTION(NS_SUCCEEDED(rv), "error unlinking builder from document");
          // XXX ignore error code?

          rv = builder->SetDataBase(nsnull);
          NS_ASSERTION(NS_SUCCEEDED(rv), "error unlinking builder from database");

          NS_RELEASE(builder);
        }
    }

    if (mLocalStore) {
        nsCOMPtr<nsIRDFRemoteDataSource> remote = do_QueryInterface(mLocalStore);
        if (remote)
            remote->Flush();
    }

    if (mCSSLoader) {
      mCSSLoader->DropDocumentReference();
    }

#if 0
    PRInt32 i;
    for (i = 0; i < mObservers.Count(); i++) {
        nsIDocumentObserver*  observer = (nsIDocumentObserver*)mObservers.ElementAt(i);
        observer->DocumentWillBeDestroyed(this);
        if (observer != (nsIDocumentObserver*)mObservers.ElementAt(i)) {
          i--;
        }
    }
#endif

    if (--gRefCnt == 0) {
        NS_IF_RELEASE(kAttributeAtom);
        NS_IF_RELEASE(kCommandUpdaterAtom);
        NS_IF_RELEASE(kDataSourcesAtom);
        NS_IF_RELEASE(kElementAtom);
        NS_IF_RELEASE(kIdAtom);
        NS_IF_RELEASE(kKeysetAtom);
        NS_IF_RELEASE(kObservesAtom);
        NS_IF_RELEASE(kOpenAtom);
        NS_IF_RELEASE(kOverlayAtom);
        NS_IF_RELEASE(kPersistAtom);
        NS_IF_RELEASE(kPositionAtom);
        NS_IF_RELEASE(kRefAtom);
        NS_IF_RELEASE(kRuleAtom);
        NS_IF_RELEASE(kTemplateAtom);

        if (gRDFService) {
            nsServiceManager::ReleaseService(kRDFServiceCID, gRDFService);
            gRDFService = nsnull;
        }

        NS_IF_RELEASE(kNC_persist);
        NS_IF_RELEASE(kNC_attribute);
        NS_IF_RELEASE(kNC_value);

        NS_IF_RELEASE(gHTMLElementFactory);
        NS_IF_RELEASE(gXMLElementFactory);

        if (gXULUtils) {
            nsServiceManager::ReleaseService(kXULContentUtilsCID, gXULUtils);
            gXULUtils = nsnull;
        }

        if (gXULCache) {
            nsServiceManager::ReleaseService(kXULPrototypeCacheCID, gXULCache);
            gXULCache = nsnull;
        }
    }
}


nsresult
NS_NewXULDocument(nsIXULDocument** result)
{
    NS_PRECONDITION(result != nsnull, "null ptr");
    if (! result)
        return NS_ERROR_NULL_POINTER;

    nsXULDocument* doc = new nsXULDocument();
    if (! doc)
        return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(doc);

    nsresult rv;
    if (NS_FAILED(rv = doc->Init())) {
        NS_RELEASE(doc);
        return rv;
    }

    *result = doc;
    return NS_OK;
}


//----------------------------------------------------------------------
//
// nsISupports interface
//

NS_IMETHODIMP
nsXULDocument::QueryInterface(REFNSIID iid, void** result)
{
    if (! result)
        return NS_ERROR_NULL_POINTER;

    *result = nsnull;
    if (iid.Equals(NS_GET_IID(nsIDocument)) ||
        iid.Equals(NS_GET_IID(nsISupports))) {
        *result = NS_STATIC_CAST(nsIDocument*, this);
    }
    else if (iid.Equals(NS_GET_IID(nsIXULDocument)) ||
             iid.Equals(NS_GET_IID(nsIXMLDocument))) {
        *result = NS_STATIC_CAST(nsIXULDocument*, this);
    }
    else if (iid.Equals(nsIDOMXULDocument::GetIID()) ||
             iid.Equals(nsIDOMDocument::GetIID()) ||
             iid.Equals(nsIDOMNode::GetIID())) {
        *result = NS_STATIC_CAST(nsIDOMXULDocument*, this);
    }
    else if (iid.Equals(nsIDOMNSDocument::GetIID())) {
        *result = NS_STATIC_CAST(nsIDOMNSDocument*, this);
    }
    else if (iid.Equals(NS_GET_IID(nsIJSScriptObject))) {
        *result = NS_STATIC_CAST(nsIJSScriptObject*, this);
    }
    else if (iid.Equals(NS_GET_IID(nsIScriptObjectOwner))) {
        *result = NS_STATIC_CAST(nsIScriptObjectOwner*, this);
    }
    else if (iid.Equals(NS_GET_IID(nsIHTMLContentContainer))) {
        *result = NS_STATIC_CAST(nsIHTMLContentContainer*, this);
    }
    else if (iid.Equals(NS_GET_IID(nsIDOMEventReceiver))) {
        *result = NS_STATIC_CAST(nsIDOMEventReceiver*, this);
    }
    else if (iid.Equals(NS_GET_IID(nsIDOMEventTarget))) {
        *result = NS_STATIC_CAST(nsIDOMEventTarget*, this);
    }
    else if (iid.Equals(NS_GET_IID(nsIDOMEventCapturer))) {
        *result = NS_STATIC_CAST(nsIDOMEventCapturer*, this);
    }
    else if (iid.Equals(NS_GET_IID(nsIStreamLoadableDocument))) {
        *result = NS_STATIC_CAST(nsIStreamLoadableDocument*, this);
    }
    else if (iid.Equals(NS_GET_IID(nsISupportsWeakReference))) {
        *result = NS_STATIC_CAST(nsISupportsWeakReference*, this);
    }
    else if (iid.Equals(NS_GET_IID(nsIUnicharStreamLoaderObserver))) {
        *result = NS_STATIC_CAST(nsIUnicharStreamLoaderObserver*, this);
    }
    else {
        *result = nsnull;
        return NS_NOINTERFACE;
    }
    NS_ADDREF(this);
    return NS_OK;
}

NS_IMPL_ADDREF(nsXULDocument);
NS_IMPL_RELEASE(nsXULDocument);

//----------------------------------------------------------------------
//
// nsIDocument interface
//

nsIArena*
nsXULDocument::GetArena()
{
    nsIArena* result = mArena;
    NS_IF_ADDREF(result);
    return result;
}

NS_IMETHODIMP
nsXULDocument::GetContentType(nsString& aContentType) const
{
    aContentType.SetString("text/xul");
    return NS_OK;
}

NS_IMETHODIMP
nsXULDocument::PrepareStyleSheets(nsIURI* anURL)
{
    nsresult rv;

    // Delete references to style sheets - this should be done in superclass...
    PRInt32 i = mStyleSheets.Count();
    while (--i >= 0) {
        nsIStyleSheet* sheet = (nsIStyleSheet*) mStyleSheets.ElementAt(i);
        sheet->SetOwningDocument(nsnull);
        NS_RELEASE(sheet);
    }
    mStyleSheets.Clear();

    // Create an HTML style sheet for the HTML content.
    nsCOMPtr<nsIHTMLStyleSheet> sheet;
    if (NS_SUCCEEDED(rv = nsComponentManager::CreateInstance(kHTMLStyleSheetCID,
                                                       nsnull,
                                                       NS_GET_IID(nsIHTMLStyleSheet),
                                                       getter_AddRefs(sheet)))) {
        if (NS_SUCCEEDED(rv = sheet->Init(anURL, this))) {
            mAttrStyleSheet = sheet;
            AddStyleSheet(mAttrStyleSheet);
        }
    }

    if (NS_FAILED(rv)) {
        NS_ERROR("unable to add HTML style sheet");
        return rv;
    }

    // Create an inline style sheet for inline content that contains a style
    // attribute.
    nsIHTMLCSSStyleSheet* inlineSheet;
    if (NS_SUCCEEDED(rv = nsComponentManager::CreateInstance(kHTMLCSSStyleSheetCID,
                                                       nsnull,
                                                       NS_GET_IID(nsIHTMLCSSStyleSheet),
                                                       (void**)&inlineSheet))) {
        if (NS_SUCCEEDED(rv = inlineSheet->Init(anURL, this))) {
            mInlineStyleSheet = dont_QueryInterface(inlineSheet);
            AddStyleSheet(mInlineStyleSheet);
        }
        NS_RELEASE(inlineSheet);
    }

    if (NS_FAILED(rv)) {
        NS_ERROR("unable to add inline style sheet");
        return rv;
    }

    return NS_OK;
}

void
nsXULDocument::SetDocumentURLAndGroup(nsIURI* anURL)
{
    mDocumentURL = dont_QueryInterface(anURL);
    // XXX help
}

NS_IMETHODIMP
nsXULDocument::StartDocumentLoad(const char* aCommand,
                                 nsIChannel* aChannel,
                                 nsILoadGroup* aLoadGroup,
                                 nsISupports* aContainer,
                                 nsIStreamListener **aDocListener)
{
    nsresult rv;

#if defined(DEBUG_waterson) || defined(DEBUG_hyatt)
    mLoadStart = PR_Now();

    {
        nsInt64 now(PR_Now());
        now /= nsInt64(1000);
        printf("##### XUL document created at %d\n", PRInt32(now));
    }
#endif

    nsCOMPtr<nsIURI> url;
    rv = aChannel->GetURI(getter_AddRefs(url));
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIParser> parser;
    rv = PrepareToLoad(aContainer, aCommand, aChannel, aLoadGroup, getter_AddRefs(parser));
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIStreamListener> listener = do_QueryInterface(parser, &rv);
    NS_ASSERTION(NS_SUCCEEDED(rv), "parser doesn't support nsIStreamListener");
    if (NS_FAILED(rv)) return rv;

    *aDocListener = listener;
    NS_IF_ADDREF(*aDocListener);

    parser->Parse(url);
    return NS_OK;
}

const nsString*
nsXULDocument::GetDocumentTitle() const
{
    return &mDocumentTitle;
}

nsIURI*
nsXULDocument::GetDocumentURL() const
{
    nsIURI* result = mDocumentURL;
    NS_IF_ADDREF(result);
    return result;
}

nsIPrincipal*
nsXULDocument::GetDocumentPrincipal()
{
    nsIPrincipal* principal = nsnull;
    mMasterPrototype->GetDocumentPrincipal(&principal);
    return principal;
}


NS_IMETHODIMP
nsXULDocument::GetDocumentLoadGroup(nsILoadGroup **aGroup) const
{
    nsCOMPtr<nsILoadGroup> group = do_QueryReferent(mDocumentLoadGroup);

    *aGroup = group;
    NS_IF_ADDREF(*aGroup);
    return NS_OK;
}

NS_IMETHODIMP
nsXULDocument::GetBaseURL(nsIURI*& aURL) const
{
    aURL = mDocumentURL;
    NS_IF_ADDREF(aURL);
    return NS_OK;
}

NS_IMETHODIMP
nsXULDocument::GetDocumentCharacterSet(nsString& oCharSetID)
{
    oCharSetID = mCharSetID;
    return NS_OK;
}

NS_IMETHODIMP
nsXULDocument::SetDocumentCharacterSet(const nsString& aCharSetID)
{
    mCharSetID = aCharSetID;
    return NS_OK;
}


NS_IMETHODIMP
nsXULDocument::GetLineBreaker(nsILineBreaker** aResult)
{
  if(! mLineBreaker) {
     // no line breaker, find a default one
     nsILineBreakerFactory *lf;
     nsresult result;
     result = nsServiceManager::GetService(kLWBrkCID,
                                          NS_GET_IID(nsILineBreakerFactory),
                                          (nsISupports **)&lf);
     if (NS_SUCCEEDED(result)) {
      nsILineBreaker *lb = nsnull ;
      nsAutoString lbarg("");
      result = lf->GetBreaker(lbarg, &lb);
      if(NS_SUCCEEDED(result)) {
         mLineBreaker = lb;
      }
      result = nsServiceManager::ReleaseService(kLWBrkCID, lf);
     }
  }
  *aResult = mLineBreaker;
  NS_IF_ADDREF(*aResult);
  return NS_OK; // XXX we should do error handling here
}

NS_IMETHODIMP
nsXULDocument::SetLineBreaker(nsILineBreaker* aLineBreaker)
{
  mLineBreaker = dont_QueryInterface(aLineBreaker);
  return NS_OK;
}
NS_IMETHODIMP
nsXULDocument::GetWordBreaker(nsIWordBreaker** aResult)
{
  if (! mWordBreaker) {
     // no line breaker, find a default one
     nsIWordBreakerFactory *lf;
     nsresult result;
     result = nsServiceManager::GetService(kLWBrkCID,
                                          NS_GET_IID(nsIWordBreakerFactory),
                                          (nsISupports **)&lf);
     if (NS_SUCCEEDED(result)) {
      nsIWordBreaker *lb = nsnull ;
      nsAutoString lbarg("");
      result = lf->GetBreaker(lbarg, &lb);
      if(NS_SUCCEEDED(result)) {
         mWordBreaker = lb;
      }
      result = nsServiceManager::ReleaseService(kLWBrkCID, lf);
     }
  }
  *aResult = mWordBreaker;
  NS_IF_ADDREF(*aResult);
  return NS_OK; // XXX we should do error handling here
}

NS_IMETHODIMP
nsXULDocument::SetWordBreaker(nsIWordBreaker* aWordBreaker)
{
  mWordBreaker = dont_QueryInterface(aWordBreaker);
  return NS_OK;
}


NS_IMETHODIMP
nsXULDocument::GetHeaderData(nsIAtom* aHeaderField, nsString& aData) const
{
  return NS_OK;
}

NS_IMETHODIMP
nsXULDocument:: SetHeaderData(nsIAtom* aHeaderField, const nsString& aData)
{
  return NS_OK;
}


NS_IMETHODIMP
nsXULDocument::CreateShell(nsIPresContext* aContext,
                             nsIViewManager* aViewManager,
                             nsIStyleSet* aStyleSet,
                             nsIPresShell** aInstancePtrResult)
{
    NS_PRECONDITION(aInstancePtrResult, "null ptr");
    if (! aInstancePtrResult)
        return NS_ERROR_NULL_POINTER;

    nsresult rv;

    nsIPresShell* shell;
    if (NS_FAILED(rv = nsComponentManager::CreateInstance(kPresShellCID,
                                                    nsnull,
                                                    NS_GET_IID(nsIPresShell),
                                                    (void**) &shell)))
        return rv;

    if (NS_FAILED(rv = shell->Init(this, aContext, aViewManager, aStyleSet))) {
        NS_RELEASE(shell);
        return rv;
    }

    mPresShells.AppendElement(shell);
    *aInstancePtrResult = shell; // addref implicit in CreateInstance()

    return NS_OK;
}

PRBool
nsXULDocument::DeleteShell(nsIPresShell* aShell)
{
    return mPresShells.RemoveElement(aShell);
}

PRInt32
nsXULDocument::GetNumberOfShells()
{
    return mPresShells.Count();
}

nsIPresShell*
nsXULDocument::GetShellAt(PRInt32 aIndex)
{
    nsIPresShell* shell = NS_STATIC_CAST(nsIPresShell*, mPresShells[aIndex]);
    NS_IF_ADDREF(shell);
    return shell;
}

nsIDocument*
nsXULDocument::GetParentDocument()
{
    NS_IF_ADDREF(mParentDocument);
    return mParentDocument;
}

void
nsXULDocument::SetParentDocument(nsIDocument* aParent)
{
    // Note that we do *not* AddRef our parent because that would
    // create a circular reference.
    mParentDocument = aParent;
}

void
nsXULDocument::AddSubDocument(nsIDocument* aSubDoc)
{
    NS_ADDREF(aSubDoc);
    mSubDocuments.AppendElement(aSubDoc);
}

PRInt32
nsXULDocument::GetNumberOfSubDocuments()
{
    return mSubDocuments.Count();
}

nsIDocument*
nsXULDocument::GetSubDocumentAt(PRInt32 aIndex)
{
    nsIDocument* doc = (nsIDocument*) mSubDocuments.ElementAt(aIndex);
    if (nsnull != doc) {
        NS_ADDREF(doc);
    }
    return doc;
}

nsIContent*
nsXULDocument::GetRootContent()
{
    nsIContent* result = mRootContent;
    NS_IF_ADDREF(result);
    return result;
}

void
nsXULDocument::SetRootContent(nsIContent* aRoot)
{
    if (mRootContent) {
        mRootContent->SetDocument(nsnull, PR_TRUE);
    }
    mRootContent = aRoot;
    if (mRootContent) {
        mRootContent->SetDocument(this, PR_TRUE);
    }
}

NS_IMETHODIMP
nsXULDocument::AppendToProlog(nsIContent* aContent)
{
    PR_ASSERT(0);
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsXULDocument::AppendToEpilog(nsIContent* aContent)
{
    PR_ASSERT(0);
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsXULDocument::ChildAt(PRInt32 aIndex, nsIContent*& aResult) const
{
    PR_ASSERT(0);
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsXULDocument::IndexOf(nsIContent* aPossibleChild, PRInt32& aIndex) const
{
    PR_ASSERT(0);
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsXULDocument::GetChildCount(PRInt32& aCount)
{
    PR_ASSERT(0);
    return NS_ERROR_NOT_IMPLEMENTED;
}

PRInt32
nsXULDocument::GetNumberOfStyleSheets()
{
    return mStyleSheets.Count();
}

nsIStyleSheet*
nsXULDocument::GetStyleSheetAt(PRInt32 aIndex)
{
    nsIStyleSheet* sheet = NS_STATIC_CAST(nsIStyleSheet*, mStyleSheets[aIndex]);
    NS_IF_ADDREF(sheet);
    return sheet;
}

PRInt32
nsXULDocument::GetIndexOfStyleSheet(nsIStyleSheet* aSheet)
{
  return mStyleSheets.IndexOf(aSheet);
}

void
nsXULDocument::AddStyleSheet(nsIStyleSheet* aSheet)
{
    NS_PRECONDITION(aSheet, "null arg");
    if (!aSheet)
        return;

    if (aSheet == mAttrStyleSheet.get()) {  // always first
      mStyleSheets.InsertElementAt(aSheet, 0);
    }
    else if (aSheet == (nsIHTMLCSSStyleSheet*)mInlineStyleSheet) {  // always last
      mStyleSheets.AppendElement(aSheet);
    }
    else {
      if ((nsIHTMLCSSStyleSheet*)mInlineStyleSheet == mStyleSheets.ElementAt(mStyleSheets.Count() - 1)) {
        // keep attr sheet last
        mStyleSheets.InsertElementAt(aSheet, mStyleSheets.Count() - 1);
      }
      else {
        mStyleSheets.AppendElement(aSheet);
      }

      // Put the style sheet into the XUL cache if the XUL cache is
      // actually enabled and the document is chrome.
      if (gXULUtils->UseXULCache() && IsChromeURI(mDocumentURL)) {
          nsCOMPtr<nsICSSStyleSheet> css = do_QueryInterface(aSheet);
          if (css) {
              gXULCache->PutStyleSheet(css);
          }
      }
    }
    NS_ADDREF(aSheet);

    aSheet->SetOwningDocument(this);

    PRBool enabled;
    aSheet->GetEnabled(enabled);

    if (enabled) {
        PRInt32 count, i;

        count = mPresShells.Count();
        for (i = 0; i < count; i++) {
            nsIPresShell* shell = NS_STATIC_CAST(nsIPresShell*, mPresShells[i]);
            nsCOMPtr<nsIStyleSet> set;
            shell->GetStyleSet(getter_AddRefs(set));
            if (set) {
                set->AddDocStyleSheet(aSheet, this);
            }
        }

        // XXX should observers be notified for disabled sheets??? I think not, but I could be wrong
        for (i = 0; i < mObservers.Count(); i++) {
            nsIDocumentObserver*  observer = (nsIDocumentObserver*)mObservers.ElementAt(i);
            observer->StyleSheetAdded(this, aSheet);
            if (observer != (nsIDocumentObserver*)mObservers.ElementAt(i)) {
              i--;
            }
        }
    }
}

void 
nsXULDocument::RemoveStyleSheet(nsIStyleSheet* aSheet)
{
  NS_PRECONDITION(nsnull != aSheet, "null arg");
  mStyleSheets.RemoveElement(aSheet);
  
  PRBool enabled = PR_TRUE;
  aSheet->GetEnabled(enabled);

  if (enabled) {
    PRInt32 count = mPresShells.Count();
    PRInt32 index;
    for (index = 0; index < count; index++) {
      nsIPresShell* shell = (nsIPresShell*)mPresShells.ElementAt(index);
      nsCOMPtr<nsIStyleSet> set;
      if (NS_SUCCEEDED(shell->GetStyleSet(getter_AddRefs(set)))) {
        if (set) {
          set->RemoveDocStyleSheet(aSheet);
        }
      }
    }

    // XXX should observers be notified for disabled sheets??? I think not, but I could be wrong
    for (index = 0; index < mObservers.Count(); index++) {
      nsIDocumentObserver*  observer = (nsIDocumentObserver*)mObservers.ElementAt(index);
      observer->StyleSheetRemoved(this, aSheet);
      if (observer != (nsIDocumentObserver*)mObservers.ElementAt(index)) {
        index--;
      }
    }
  }

  aSheet->SetOwningDocument(nsnull);
  NS_RELEASE(aSheet);
}

NS_IMETHODIMP
nsXULDocument::InsertStyleSheetAt(nsIStyleSheet* aSheet, PRInt32 aIndex, PRBool aNotify)
{
  NS_PRECONDITION(nsnull != aSheet, "null ptr");

  // Put the style sheet into the XUL cache if the XUL cache is
  // actually enabled and the document is chrome.
  if (gXULUtils->UseXULCache() && IsChromeURI(mDocumentURL)) {
      nsCOMPtr<nsICSSStyleSheet> css = do_QueryInterface(aSheet);
      if (css) {
          gXULCache->PutStyleSheet(css);
      }
  }

  mStyleSheets.InsertElementAt(aSheet, aIndex + 1); // offset by one for attribute sheet

  NS_ADDREF(aSheet);
  aSheet->SetOwningDocument(this);

  PRBool enabled = PR_TRUE;
  aSheet->GetEnabled(enabled);

  PRInt32 count;
  PRInt32 i;
  if (enabled) {
    count = mPresShells.Count();
    for (i = 0; i < count; i++) {
      nsIPresShell* shell = (nsIPresShell*)mPresShells.ElementAt(i);
      nsCOMPtr<nsIStyleSet> set;
      shell->GetStyleSet(getter_AddRefs(set));
      if (set) {
        set->AddDocStyleSheet(aSheet, this);
      }
    }
  }
  if (aNotify) {  // notify here even if disabled, there may have been others that weren't notified
    for (i = 0; i < mObservers.Count(); i++) {
      nsIDocumentObserver*  observer = (nsIDocumentObserver*)mObservers.ElementAt(i);
      observer->StyleSheetAdded(this, aSheet);
      if (observer != (nsIDocumentObserver*)mObservers.ElementAt(i)) {
        i--;
      }
    }
  }

  return NS_OK;
}

void
nsXULDocument::SetStyleSheetDisabledState(nsIStyleSheet* aSheet,
                                          PRBool aDisabled)
{
    NS_PRECONDITION(nsnull != aSheet, "null arg");
    PRInt32 count;
    PRInt32 i;

    // If we're actually in the document style sheet list
    if (-1 != mStyleSheets.IndexOf((void *)aSheet)) {
        count = mPresShells.Count();
        for (i = 0; i < count; i++) {
            nsIPresShell* shell = (nsIPresShell*)mPresShells.ElementAt(i);
            nsCOMPtr<nsIStyleSet> set;
            shell->GetStyleSet(getter_AddRefs(set));
            if (set) {
                if (aDisabled) {
                    set->RemoveDocStyleSheet(aSheet);
                }
                else {
                    set->AddDocStyleSheet(aSheet, this);  // put it first
                }
            }
        }
    }

    for (i = 0; i < mObservers.Count(); i++) {
        nsIDocumentObserver*  observer = (nsIDocumentObserver*)mObservers.ElementAt(i);
        observer->StyleSheetDisabledStateChanged(this, aSheet, aDisabled);
        if (observer != (nsIDocumentObserver*)mObservers.ElementAt(i)) {
          i--;
        }
    }
}

NS_IMETHODIMP
nsXULDocument::GetCSSLoader(nsICSSLoader*& aLoader)
{
  nsresult result = NS_OK;
  if (! mCSSLoader) {
    result = nsComponentManager::CreateInstance(kCSSLoaderCID,
                                                nsnull,
                                                NS_GET_IID(nsICSSLoader),
                                                getter_AddRefs(mCSSLoader));
    if (NS_SUCCEEDED(result)) {
      result = mCSSLoader->Init(this);
      mCSSLoader->SetCaseSensitive(PR_TRUE);
      mCSSLoader->SetQuirkMode(PR_FALSE); // no quirks in XUL
    }
  }
  aLoader = mCSSLoader;
  NS_IF_ADDREF(aLoader);
  return result;
}

nsIScriptContextOwner *
nsXULDocument::GetScriptContextOwner()
{
    NS_IF_ADDREF(mScriptContextOwner);
    return mScriptContextOwner;
}

void
nsXULDocument::SetScriptContextOwner(nsIScriptContextOwner *aScriptContextOwner)
{
    // XXX HACK ALERT! If the script context owner is null, the document
    // will soon be going away. So tell our content that to lose its
    // reference to the document. This has to be done before we
    // actually set the script context owner to null so that the
    // content elements can remove references to their script objects.
    if (!aScriptContextOwner && mRootContent)
        mRootContent->SetDocument(nsnull, PR_TRUE);

    mScriptContextOwner = aScriptContextOwner;
}

NS_IMETHODIMP
nsXULDocument::GetNameSpaceManager(nsINameSpaceManager*& aManager)
{
  aManager = mNameSpaceManager;
  NS_IF_ADDREF(aManager);
  return NS_OK;
}


// Note: We don't hold a reference to the document observer; we assume
// that it has a live reference to the document.
void
nsXULDocument::AddObserver(nsIDocumentObserver* aObserver)
{
    // XXX Make sure the observer isn't already in the list
    if (mObservers.IndexOf(aObserver) == -1) {
        mObservers.AppendElement(aObserver);
    }
}

PRBool
nsXULDocument::RemoveObserver(nsIDocumentObserver* aObserver)
{
    return mObservers.RemoveElement(aObserver);
}

NS_IMETHODIMP
nsXULDocument::BeginLoad()
{
    // XXX Never called. Does this matter?
    for (PRInt32 i = 0; i < mObservers.Count(); i++) {
        nsIDocumentObserver* observer = (nsIDocumentObserver*) mObservers[i];
        observer->BeginLoad(this);
        if (observer != (nsIDocumentObserver*)mObservers.ElementAt(i)) {
          i--;
        }
    }
    return NS_OK;
}

NS_IMETHODIMP
nsXULDocument::EndLoad()
{
    nsresult rv;

    // Whack the prototype document into the cache so that the next
    // time somebody asks for it, they don't need to load it by hand.
    nsCOMPtr<nsIURI> uri;
    rv = mCurrentPrototype->GetURI(getter_AddRefs(uri));
    if (NS_FAILED(rv)) return rv;

    if (gXULUtils->UseXULCache() && IsChromeURI(mDocumentURL)) {
        // If it's a 'chrome:' prototype document, then put it into
        // the prototype cache; other XUL documents will be reloaded
        // each time.
        rv = gXULCache->PutPrototype(mCurrentPrototype);
        if (NS_FAILED(rv)) return rv;
    }

    // Now walk the prototype to build content.
    rv = PrepareToWalk();
    if (NS_FAILED(rv)) return rv;

    rv = ResumeWalk();
    return rv;
}


NS_IMETHODIMP
nsXULDocument::ContentChanged(nsIContent* aContent,
                              nsISupports* aSubContent)
{
    for (PRInt32 i = 0; i < mObservers.Count(); i++) {
        nsIDocumentObserver*  observer = (nsIDocumentObserver*)mObservers[i];
        observer->ContentChanged(this, aContent, aSubContent);
        if (observer != (nsIDocumentObserver*)mObservers.ElementAt(i)) {
          i--;
        }
    }
    return NS_OK;
}

NS_IMETHODIMP
nsXULDocument::ContentStatesChanged(nsIContent* aContent1, nsIContent* aContent2)
{
    for (PRInt32 i = 0; i < mObservers.Count(); i++) {
        nsIDocumentObserver*  observer = (nsIDocumentObserver*)mObservers[i];
        observer->ContentStatesChanged(this, aContent1, aContent2);
        if (observer != (nsIDocumentObserver*)mObservers.ElementAt(i)) {
          i--;
        }
    }
    return NS_OK;
}

NS_IMETHODIMP
nsXULDocument::AttributeChanged(nsIContent* aElement,
                                  PRInt32 aNameSpaceID,
                                  nsIAtom* aAttribute,
                                  PRInt32 aHint)
{
    nsresult rv;

    PRInt32 nameSpaceID;
    rv = aElement->GetNameSpaceID(nameSpaceID);
    if (NS_FAILED(rv)) return rv;

    // First see if we need to update our element map.
    if ((aAttribute == kIdAtom) || (aAttribute == kRefAtom)) {

        rv = mElementMap.Enumerate(RemoveElementsFromMapByContent, aElement);
        if (NS_FAILED(rv)) return rv;

        // That'll have removed _both_ the 'ref' and 'id' entries from
        // the map. So add 'em back now.
        rv = AddElementToMap(aElement);
        if (NS_FAILED(rv)) return rv;
    }

    // Now notify external observers
    for (PRInt32 i = 0; i < mObservers.Count(); i++) {
        nsIDocumentObserver*  observer = (nsIDocumentObserver*)mObservers[i];
        observer->AttributeChanged(this, aElement, aNameSpaceID, aAttribute, aHint);
        if (observer != (nsIDocumentObserver*)mObservers.ElementAt(i)) {
            i--;
        }
    }

    // Handle "special" cases. We do this handling _after_ we've
    // notified the observer to ensure that any frames that are
    // caching information (e.g., the tree widget and the 'open'
    // attribute) will notice things properly.
    if ((nameSpaceID == kNameSpaceID_XUL) && (aAttribute == kOpenAtom)) {
        nsAutoString open;
        rv = aElement->GetAttribute(kNameSpaceID_None, kOpenAtom, open);
        if (NS_FAILED(rv)) return rv;

        if ((rv == NS_CONTENT_ATTR_HAS_VALUE) && (open.Equals("true"))) {
            OpenWidgetItem(aElement);
        }
        else {
            CloseWidgetItem(aElement);
        }
    }
    else if (aAttribute == kRefAtom) {
        RebuildWidgetItem(aElement);
    }


    // Finally, see if there is anything we need to persist in the
    // localstore.
    //
    // XXX Namespace handling broken :-(
    nsAutoString persist;
    rv = aElement->GetAttribute(kNameSpaceID_None, kPersistAtom, persist);
    if (NS_FAILED(rv)) return rv;

    if (rv == NS_CONTENT_ATTR_HAS_VALUE) {
        nsAutoString attr;
        rv = aAttribute->ToString(attr);
        if (NS_FAILED(rv)) return rv;

        if (persist.Find(attr) >= 0) {
            rv = Persist(aElement, kNameSpaceID_None, aAttribute);
            if (NS_FAILED(rv)) return rv;
        }
    }

    return NS_OK;
}

NS_IMETHODIMP
nsXULDocument::ContentAppended(nsIContent* aContainer,
                                 PRInt32 aNewIndexInContainer)
{
    // First update our element map
    {
        nsresult rv;

        PRInt32 count;
        rv = aContainer->ChildCount(count);
        if (NS_FAILED(rv)) return rv;

        for (PRInt32 i = aNewIndexInContainer; i < count; ++i) {
            nsCOMPtr<nsIContent> child;
            rv = aContainer->ChildAt(i, *getter_AddRefs(child));
            if (NS_FAILED(rv)) return rv;

            rv = AddSubtreeToDocument(child);
            if (NS_FAILED(rv)) return rv;
        }
    }

    // Now notify external observers
    for (PRInt32 i = 0; i < mObservers.Count(); i++) {
        nsIDocumentObserver*  observer = (nsIDocumentObserver*)mObservers[i];
        observer->ContentAppended(this, aContainer, aNewIndexInContainer);
        if (observer != (nsIDocumentObserver*)mObservers.ElementAt(i)) {
          i--;
        }
    }
    return NS_OK;
}

NS_IMETHODIMP
nsXULDocument::ContentInserted(nsIContent* aContainer,
                                 nsIContent* aChild,
                                 PRInt32 aIndexInContainer)
{
    {
        nsresult rv;
        rv = AddSubtreeToDocument(aChild);
        if (NS_FAILED(rv)) return rv;
    }

    // Now notify external observers
    for (PRInt32 i = 0; i < mObservers.Count(); i++) {
        nsIDocumentObserver*  observer = (nsIDocumentObserver*)mObservers[i];
        observer->ContentInserted(this, aContainer, aChild, aIndexInContainer);
        if (observer != (nsIDocumentObserver*)mObservers.ElementAt(i)) {
          i--;
        }
    }
    return NS_OK;
}

NS_IMETHODIMP
nsXULDocument::ContentReplaced(nsIContent* aContainer,
                                 nsIContent* aOldChild,
                                 nsIContent* aNewChild,
                                 PRInt32 aIndexInContainer)
{
    {
        nsresult rv;
        rv = RemoveSubtreeFromDocument(aOldChild);
        if (NS_FAILED(rv)) return rv;

        rv = AddSubtreeToDocument(aNewChild);
        if (NS_FAILED(rv)) return rv;
    }

    // Now notify external observers
    for (PRInt32 i = 0; i < mObservers.Count(); i++) {
        nsIDocumentObserver*  observer = (nsIDocumentObserver*)mObservers[i];
        observer->ContentReplaced(this, aContainer, aOldChild, aNewChild,
                                  aIndexInContainer);
        if (observer != (nsIDocumentObserver*)mObservers.ElementAt(i)) {
          i--;
        }
    }
    return NS_OK;
}

NS_IMETHODIMP
nsXULDocument::ContentRemoved(nsIContent* aContainer,
                                nsIContent* aChild,
                                PRInt32 aIndexInContainer)
{
    {
        nsresult rv;
        rv = RemoveSubtreeFromDocument(aChild);
        if (NS_FAILED(rv)) return rv;
    }

    // Now notify external observers
    for (PRInt32 i = 0; i < mObservers.Count(); i++) {
        nsIDocumentObserver*  observer = (nsIDocumentObserver*)mObservers[i];
        observer->ContentRemoved(this, aContainer,
                                 aChild, aIndexInContainer);
        if (observer != (nsIDocumentObserver*)mObservers.ElementAt(i)) {
          i--;
        }
    }
    return NS_OK;
}

NS_IMETHODIMP
nsXULDocument::StyleRuleChanged(nsIStyleSheet* aStyleSheet,
                                  nsIStyleRule* aStyleRule,
                                  PRInt32 aHint)
{
    for (PRInt32 i = 0; i < mObservers.Count(); i++) {
        nsIDocumentObserver*  observer = (nsIDocumentObserver*)mObservers[i];
        observer->StyleRuleChanged(this, aStyleSheet, aStyleRule, aHint);
        if (observer != (nsIDocumentObserver*)mObservers.ElementAt(i)) {
          i--;
        }
    }
    return NS_OK;
}

NS_IMETHODIMP
nsXULDocument::StyleRuleAdded(nsIStyleSheet* aStyleSheet,
                                nsIStyleRule* aStyleRule)
{
    for (PRInt32 i = 0; i < mObservers.Count(); i++) {
        nsIDocumentObserver*  observer = (nsIDocumentObserver*)mObservers[i];
        observer->StyleRuleAdded(this, aStyleSheet, aStyleRule);
        if (observer != (nsIDocumentObserver*)mObservers.ElementAt(i)) {
          i--;
        }
    }
    return NS_OK;
}

NS_IMETHODIMP
nsXULDocument::StyleRuleRemoved(nsIStyleSheet* aStyleSheet,
                                  nsIStyleRule* aStyleRule)
{
    for (PRInt32 i = 0; i < mObservers.Count(); i++) {
        nsIDocumentObserver*  observer = (nsIDocumentObserver*)mObservers[i];
        observer->StyleRuleRemoved(this, aStyleSheet, aStyleRule);
        if (observer != (nsIDocumentObserver*)mObservers.ElementAt(i)) {
          i--;
        }
    }
    return NS_OK;
}

NS_IMETHODIMP
nsXULDocument::GetSelection(nsIDOMSelection** aSelection)
{
    if (!mSelection) {
        PR_ASSERT(0);
        *aSelection = nsnull;
        return NS_ERROR_NOT_INITIALIZED;
    }
    *aSelection = mSelection;
    NS_ADDREF(*aSelection);
    return NS_OK;
}

NS_IMETHODIMP
nsXULDocument::SelectAll()
{

    nsIContent * start = nsnull;
    nsIContent * end   = nsnull;
    nsIContent * body  = nsnull;

    nsAutoString bodyStr("BODY");
    PRInt32 i, n;
    mRootContent->ChildCount(n);
    for (i=0;i<n;i++) {
        nsIContent * child;
        mRootContent->ChildAt(i, child);
        PRBool isSynthetic;
        child->IsSynthetic(isSynthetic);
        if (!isSynthetic) {
            nsIAtom * atom;
            child->GetTag(atom);
            if (bodyStr.EqualsIgnoreCase(atom)) {
                body = child;
                break;
            }

        }
        NS_RELEASE(child);
    }

    if (body == nsnull) {
        return NS_ERROR_FAILURE;
    }

    start = body;
    // Find Very first Piece of Content
    for (;;) {
        start->ChildCount(n);
        if (n <= 0) {
            break;
        }
        nsIContent * child = start;
        child->ChildAt(0, start);
        NS_RELEASE(child);
    }

    end = body;
    // Last piece of Content
    for (;;) {
        end->ChildCount(n);
        if (n <= 0) {
            break;
        }
        nsIContent * child = end;
        child->ChildAt(n-1, end);
        NS_RELEASE(child);
    }

    //NS_RELEASE(start);
    //NS_RELEASE(end);

#if 0 // XXX nsSelectionRange is in another DLL
    nsSelectionRange * range    = mSelection->GetRange();
    nsSelectionPoint * startPnt = range->GetStartPoint();
    nsSelectionPoint * endPnt   = range->GetEndPoint();
    startPnt->SetPoint(start, -1, PR_TRUE);
    endPnt->SetPoint(end, -1, PR_FALSE);
#endif
    SetDisplaySelection(PR_TRUE);

    return NS_OK;
}

NS_IMETHODIMP
nsXULDocument::FindNext(const nsString &aSearchStr, PRBool aMatchCase, PRBool aSearchDown, PRBool &aIsFound)
{
    aIsFound = PR_FALSE;
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsXULDocument::CreateXIF(nsString & aBuffer, nsIDOMSelection* aSelection)
{
    PR_ASSERT(0);
    return NS_OK;
}

NS_IMETHODIMP
nsXULDocument::ToXIF(nsXIFConverter& aConverter, nsIDOMNode* aNode)
{
    PR_ASSERT(0);
    return NS_OK;
}

void
nsXULDocument::BeginConvertToXIF(nsXIFConverter& aConverter, nsIDOMNode* aNode)
{
    PR_ASSERT(0);
}

void
nsXULDocument::ConvertChildrenToXIF(nsXIFConverter& aConverter, nsIDOMNode* aNode)
{
    PR_ASSERT(0);
}

void
nsXULDocument::FinishConvertToXIF(nsXIFConverter& aConverter, nsIDOMNode* aNode)
{
    PR_ASSERT(0);
}

PRBool
nsXULDocument::IsInRange(const nsIContent *aStartContent, const nsIContent* aEndContent, const nsIContent* aContent) const
{
    PRBool  result;

    if (aStartContent == aEndContent) {
            return PRBool(aContent == aStartContent);
    }
    else if (aStartContent == aContent || aEndContent == aContent) {
        result = PR_TRUE;
    }
    else {
        result = IsBefore(aStartContent,aContent);
        if (result)
            result = IsBefore(aContent, aEndContent);
    }
    return result;
}

PRBool
nsXULDocument::IsBefore(const nsIContent *aNewContent, const nsIContent* aCurrentContent) const
{
    PRBool result = PR_FALSE;

    if (nsnull != aNewContent && nsnull != aCurrentContent && aNewContent != aCurrentContent) {
        nsIContent* test = FindContent(mRootContent, aNewContent, aCurrentContent);
        if (test == aNewContent)
            result = PR_TRUE;

        NS_RELEASE(test);
    }
    return result;
}

PRBool
nsXULDocument::IsInSelection(nsIDOMSelection* aSelection, const nsIContent *aContent) const
{
    PRBool  result = PR_FALSE;

    if (mSelection != nsnull) {
#if 0 // XXX can't include this because nsSelectionPoint is in another DLL.
        nsSelectionRange* range = mSelection->GetRange();
        if (range != nsnull) {
            nsSelectionPoint* startPoint = range->GetStartPoint();
            nsSelectionPoint* endPoint = range->GetEndPoint();

            nsIContent* startContent = startPoint->GetContent();
            nsIContent* endContent = endPoint->GetContent();
            result = IsInRange(startContent, endContent, aContent);
            NS_IF_RELEASE(startContent);
            NS_IF_RELEASE(endContent);
        }
#endif
    }
    return result;
}

nsIContent*
nsXULDocument::GetPrevContent(const nsIContent *aContent) const
{
    nsIContent* result = nsnull;

    // Look at previous sibling

    if (nsnull != aContent) {
        nsIContent* parent;
        aContent->GetParent(parent);

        if (parent && parent != mRootContent.get()) {
            PRInt32 i;
            parent->IndexOf((nsIContent*)aContent, i);
            if (i > 0)
                parent->ChildAt(i - 1, result);
            else
                result = GetPrevContent(parent);
        }
        NS_IF_RELEASE(parent);
    }
    return result;
}

nsIContent*
nsXULDocument::GetNextContent(const nsIContent *aContent) const
{
    nsIContent* result = nsnull;

    if (nsnull != aContent) {
        // Look at next sibling
        nsIContent* parent;
        aContent->GetParent(parent);

        if (parent != nsnull && parent != mRootContent.get()) {
            PRInt32 i;
            parent->IndexOf((nsIContent*)aContent, i);

            PRInt32 count;
            parent->ChildCount(count);
            if (i + 1 < count) {
                parent->ChildAt(i + 1, result);
                // Get first child down the tree
                for (;;) {
                    PRInt32 n;
                    result->ChildCount(n);
                    if (n <= 0)
                        break;

                    nsIContent * old = result;
                    old->ChildAt(0, result);
                    NS_RELEASE(old);
                    result->ChildCount(n);
                }
            } else {
                result = GetNextContent(parent);
            }
        }
        NS_IF_RELEASE(parent);
    }
    return result;
}

void
nsXULDocument::SetDisplaySelection(PRBool aToggle)
{
    mDisplaySelection = aToggle;
}

PRBool
nsXULDocument::GetDisplaySelection() const
{
    return mDisplaySelection;
}

NS_IMETHODIMP
nsXULDocument::HandleDOMEvent(nsIPresContext* aPresContext,
                            nsEvent* aEvent,
                            nsIDOMEvent** aDOMEvent,
                            PRUint32 aFlags,
                            nsEventStatus* aEventStatus)
{
  nsresult ret = NS_OK;
  nsIDOMEvent* domEvent = nsnull;

  if (NS_EVENT_FLAG_INIT == aFlags) {
    aDOMEvent = &domEvent;
    aEvent->flags = NS_EVENT_FLAG_NONE;
  }

  //Capturing stage
  if (NS_EVENT_FLAG_BUBBLE != aFlags && nsnull != mScriptContextOwner) {
    nsIScriptGlobalObject* global;
    if (NS_OK == mScriptContextOwner->GetScriptGlobalObject(&global)) {
      global->HandleDOMEvent(aPresContext, aEvent, aDOMEvent, NS_EVENT_FLAG_CAPTURE, aEventStatus);
      NS_RELEASE(global);
    }
  }

  //Local handling stage
  if (mListenerManager && !(aEvent->flags & NS_EVENT_FLAG_STOP_DISPATCH)) {
    aEvent->flags = aFlags;
    mListenerManager->HandleEvent(aPresContext, aEvent, aDOMEvent, aFlags, aEventStatus);
  }

  //Bubbling stage
  if (NS_EVENT_FLAG_CAPTURE != aFlags && nsnull != mScriptContextOwner) {
    nsIScriptGlobalObject* global;
    if (NS_OK == mScriptContextOwner->GetScriptGlobalObject(&global)) {
      global->HandleDOMEvent(aPresContext, aEvent, aDOMEvent, NS_EVENT_FLAG_BUBBLE, aEventStatus);
      NS_RELEASE(global);
    }
  }

  if (NS_EVENT_FLAG_INIT == aFlags) {
    // We're leaving the DOM event loop so if we created a DOM event, release here.
    if (nsnull != *aDOMEvent) {
      nsrefcnt rc;
      NS_RELEASE2(*aDOMEvent, rc);
      if (0 != rc) {
      //Okay, so someone in the DOM loop (a listener, JS object) still has a ref to the DOM Event but
      //the internal data hasn't been malloc'd.  Force a copy of the data here so the DOM Event is still valid.
        nsIPrivateDOMEvent *privateEvent;
        if (NS_OK == (*aDOMEvent)->QueryInterface(NS_GET_IID(nsIPrivateDOMEvent), (void**)&privateEvent)) {
          privateEvent->DuplicatePrivateData();
          NS_RELEASE(privateEvent);
        }
      }
    }
    aDOMEvent = nsnull;
  }

  return ret;
}


//----------------------------------------------------------------------
//
// nsIXMLDocument interface
//

NS_IMETHODIMP
nsXULDocument::GetContentById(const nsString& aName, nsIContent** aContent)
{
    PR_ASSERT(0);
    return NS_ERROR_NOT_IMPLEMENTED;
}

#ifdef XSL
NS_IMETHODIMP
nsXULDocument::SetTransformMediator(nsITransformMediator* aMediator)
{
    PR_ASSERT(0);
    return NS_ERROR_NOT_IMPLEMENTED;
}
#endif

//----------------------------------------------------------------------
//
// nsIXULDocument interface
//

NS_IMETHODIMP
nsXULDocument::AddElementForID(const nsString& aID, nsIContent* aElement)
{
    NS_PRECONDITION(aElement != nsnull, "null ptr");
    if (! aElement)
        return NS_ERROR_NULL_POINTER;

    mElementMap.Add(aID, aElement);
    return NS_OK;
}


NS_IMETHODIMP
nsXULDocument::RemoveElementForID(const nsString& aID, nsIContent* aElement)
{
    NS_PRECONDITION(aElement != nsnull, "null ptr");
    if (! aElement)
        return NS_ERROR_NULL_POINTER;

    mElementMap.Remove(aID, aElement);
    return NS_OK;
}


NS_IMETHODIMP
nsXULDocument::GetElementsForID(const nsString& aID, nsISupportsArray* aElements)
{
    NS_PRECONDITION(aElements != nsnull, "null ptr");
    if (! aElements)
        return NS_ERROR_NULL_POINTER;

    mElementMap.Find(aID, aElements);
    return NS_OK;
}


NS_IMETHODIMP
nsXULDocument::CreateContents(nsIContent* aElement)
{
    NS_PRECONDITION(aElement != nsnull, "null ptr");
    if (! aElement)
        return NS_ERROR_NULL_POINTER;

    if (! mBuilders)
        return NS_ERROR_NOT_INITIALIZED;

    PRUint32 cnt = 0;
    nsresult rv = mBuilders->Count(&cnt);
    NS_ASSERTION(NS_SUCCEEDED(rv), "Count failed");
    for (PRUint32 i = 0; i < cnt; ++i) {
        // XXX we should QueryInterface() here
        nsIRDFContentModelBuilder* builder
            = (nsIRDFContentModelBuilder*) mBuilders->ElementAt(i);

        NS_ASSERTION(builder != nsnull, "null ptr");
        if (! builder)
            continue;

        rv = builder->CreateContents(aElement);
        NS_ASSERTION(NS_SUCCEEDED(rv), "error creating content");
        // XXX ignore error code?

        NS_RELEASE(builder);
    }

    return NS_OK;
}


NS_IMETHODIMP
nsXULDocument::AddContentModelBuilder(nsIRDFContentModelBuilder* aBuilder)
{
    NS_PRECONDITION(aBuilder != nsnull, "null ptr");
    if (! aBuilder)
        return NS_ERROR_NULL_POINTER;

    nsresult rv;
    if (! mBuilders) {
        rv = NS_NewISupportsArray(getter_AddRefs(mBuilders));
        if (NS_FAILED(rv)) return rv;
    }

    rv = aBuilder->SetDocument(this);
    if (NS_FAILED(rv)) return rv;

    return mBuilders->AppendElement(aBuilder) ? NS_OK : NS_ERROR_FAILURE;
}


NS_IMETHODIMP
nsXULDocument::GetForm(nsIDOMHTMLFormElement** aForm)
{
  *aForm = mHiddenForm;
  NS_IF_ADDREF(*aForm);
  return NS_OK;
}

NS_IMETHODIMP
nsXULDocument::SetForm(nsIDOMHTMLFormElement* aForm)
{
    mHiddenForm = dont_QueryInterface(aForm);

    // Set the document.
    nsCOMPtr<nsIContent> formContent = do_QueryInterface(aForm);
    formContent->SetDocument(this, PR_TRUE);

    // Forms are containers, and as such take up a bit of space.
    // Set a style attribute to keep the hidden form from showing up.
    mHiddenForm->SetAttribute("style", "margin:0em");
    return NS_OK;
}


NS_IMETHODIMP
nsXULDocument::AddForwardReference(nsForwardReference* aRef)
{
    if (mResolutionPhase < aRef->GetPhase()) {
        mForwardReferences.AppendElement(aRef);
    }
    else {
        NS_ERROR("forward references have already been resolved");
        delete aRef;
    }

    return NS_OK;
}


NS_IMETHODIMP
nsXULDocument::ResolveForwardReferences()
{
    if (mResolutionPhase == nsForwardReference::eDone)
        return NS_OK;

    // Resolve each outstanding 'forward' reference. We iterate
    // through the list of forward references until no more forward
    // references can be resolved. This annealing process is
    // guaranteed to converge because we've "closed the gate" to new
    // forward references.

    const nsForwardReference::Phase* pass = nsForwardReference::kPasses;
    while ((mResolutionPhase = *pass) != nsForwardReference::eDone) {
        PRInt32 previous = 0;
        while (mForwardReferences.Count() && mForwardReferences.Count() != previous) {
            previous = mForwardReferences.Count();

            for (PRInt32 i = 0; i < mForwardReferences.Count(); ++i) {
                nsForwardReference* fwdref = NS_REINTERPRET_CAST(nsForwardReference*, mForwardReferences[i]);

                if (fwdref->GetPhase() == *pass) {
                    nsForwardReference::Result result = fwdref->Resolve();

                    switch (result) {
                    case nsForwardReference::eResolve_Succeeded:
                    case nsForwardReference::eResolve_Error:
                        mForwardReferences.RemoveElementAt(i);
                        delete fwdref;

                        // fixup because we removed from list
                        --i;
                        break;

                    case nsForwardReference::eResolve_Later:
                        // do nothing. we'll try again later
                        ;
                    }
                }
            }
        }

        ++pass;
    }

    DestroyForwardReferences();
    return NS_OK;
}


NS_IMETHODIMP
nsXULDocument::CreateFromPrototype(const char* aCommand,
                                   nsIXULPrototypeDocument* aPrototype)
{
    nsresult rv;

    mMasterPrototype = mCurrentPrototype = aPrototype;

    rv = mCurrentPrototype->GetURI(getter_AddRefs(mDocumentURL));
    if (NS_FAILED(rv)) return rv;

    rv = PrepareStyleSheets(mDocumentURL);
    if (NS_FAILED(rv)) return rv;

    mCommand = aCommand;

    rv = AddPrototypeSheets();
    if (NS_FAILED(rv)) return rv;

    {
        nsCOMPtr<nsILoadGroup> loadgroup;
        rv = nsComponentManager::CreateInstance(kLoadGroupCID,
                                                nsnull,
                                                NS_GET_IID(nsILoadGroup),
                                                getter_AddRefs(loadgroup));
        if (NS_FAILED(rv)) return rv;

        CachedChromeLoader* loader = new CachedChromeLoader(this);
        if (! loader)
            return NS_ERROR_OUT_OF_MEMORY;

        NS_ADDREF(loader);
        nsCOMPtr<nsIStreamObserver> anchor = do_QueryInterface(loader, &rv);
        NS_RELEASE(loader);

        if (NS_FAILED(rv)) return rv;

        rv = loadgroup->Init(loader);
        if (NS_FAILED(rv)) return rv;

        // Even though we are only holding a weak reference to the
        // load group, any channels that get added to the load group
        // will acquire strong refs, keeping the load group alive
        // until the last channel is complete.
        mDocumentLoadGroup = getter_AddRefs(NS_GetWeakReference(loadgroup));

        // Now create the delegates from the prototype
        rv = PrepareToWalk();
        if (NS_FAILED(rv)) return rv;

        // Closing this scope will result in the placeholder channel
        // being the only reference to the load group.
    }

    rv = ResumeWalk();
    return rv;
}

//----------------------------------------------------------------------
//
// nsIStreamLoadableDocument interface
//

NS_IMETHODIMP
nsXULDocument::LoadFromStream(nsIInputStream& xulStream,
                              nsISupports* aContainer,
                              const char* aCommand)
{
    nsresult rv;

    // XXXbe this is dead code, eliminate
    nsCOMPtr<nsIParser> parser;
    rv = PrepareToLoad(aContainer, aCommand, nsnull, nsnull, getter_AddRefs(parser));
    if (NS_FAILED(rv)) return rv;

    parser->Parse(xulStream);
    return NS_OK;
}


//----------------------------------------------------------------------
//
// nsIDOMDocument interface
//

NS_IMETHODIMP
nsXULDocument::GetDoctype(nsIDOMDocumentType** aDoctype)
{
    NS_NOTYETIMPLEMENTED("write me!");
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsXULDocument::GetImplementation(nsIDOMDOMImplementation** aImplementation)
{
    NS_NOTYETIMPLEMENTED("write me!");
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsXULDocument::GetDocumentElement(nsIDOMElement** aDocumentElement)
{
    NS_PRECONDITION(aDocumentElement != nsnull, "null ptr");
    if (! aDocumentElement)
        return NS_ERROR_NULL_POINTER;

    if (mRootContent) {
        return mRootContent->QueryInterface(nsIDOMElement::GetIID(), (void**)aDocumentElement);
    }
    else {
        *aDocumentElement = nsnull;
        return NS_OK;
    }
}



NS_IMETHODIMP
nsXULDocument::CreateElement(const nsString& aTagName, nsIDOMElement** aReturn)
{
    NS_PRECONDITION(aReturn != nsnull, "null ptr");
    if (! aReturn)
        return NS_ERROR_NULL_POINTER;

    nsresult rv;

#ifdef PR_LOGGING
    if (PR_LOG_TEST(gXULLog, PR_LOG_DEBUG)) {
      char* tagCStr = aTagName.ToNewCString();

      PR_LOG(gXULLog, PR_LOG_DEBUG,
             ("xul[CreateElement] %s", tagCStr));

      nsCRT::free(tagCStr);
    }
#endif

    nsCOMPtr<nsIAtom> name;
    PRInt32 nameSpaceID;

    *aReturn = nsnull;

    // parse the user-provided string into a tag name and a namespace ID
    rv = ParseTagString(aTagName, *getter_AddRefs(name), nameSpaceID);
    if (NS_FAILED(rv)) {
#ifdef PR_LOGGING
        char* tagNameStr = aTagName.ToNewCString();
        PR_LOG(gXULLog, PR_LOG_ERROR,
               ("xul[CreateElement] unable to parse tag '%s'; no such namespace.", tagNameStr));
        nsCRT::free(tagNameStr);
#endif
        return rv;
    }

    nsCOMPtr<nsIContent> result;
    rv = CreateElement(nameSpaceID, name, getter_AddRefs(result));
    if (NS_FAILED(rv)) return rv;

    // get the DOM interface
    rv = result->QueryInterface(nsIDOMElement::GetIID(), (void**) aReturn);
    NS_ASSERTION(NS_SUCCEEDED(rv), "not a DOM element");
    if (NS_FAILED(rv)) return rv;

    return NS_OK;
}


NS_IMETHODIMP
nsXULDocument::CreateDocumentFragment(nsIDOMDocumentFragment** aReturn)
{
    NS_NOTYETIMPLEMENTED("write me!");
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsXULDocument::CreateTextNode(const nsString& aData, nsIDOMText** aReturn)
{
    NS_PRECONDITION(aReturn != nsnull, "null ptr");
    if (! aReturn)
        return NS_ERROR_NULL_POINTER;

    nsresult rv;

    nsCOMPtr<nsITextContent> text;
    rv = nsComponentManager::CreateInstance(kTextNodeCID, nsnull, nsITextContent::GetIID(), getter_AddRefs(text));
    if (NS_FAILED(rv)) return rv;

    rv = text->SetText(aData.GetUnicode(), aData.Length(), PR_FALSE);
    if (NS_FAILED(rv)) return rv;

    rv = text->QueryInterface(nsIDOMText::GetIID(), (void**) aReturn);
    NS_ASSERTION(NS_SUCCEEDED(rv), "not a DOMText");
    if (NS_FAILED(rv)) return rv;

    return NS_OK;
}


NS_IMETHODIMP
nsXULDocument::CreateComment(const nsString& aData, nsIDOMComment** aReturn)
{
    NS_NOTYETIMPLEMENTED("write me!");
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsXULDocument::CreateCDATASection(const nsString& aData, nsIDOMCDATASection** aReturn)
{
    NS_NOTYETIMPLEMENTED("write me!");
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsXULDocument::CreateProcessingInstruction(const nsString& aTarget, const nsString& aData, nsIDOMProcessingInstruction** aReturn)
{
    NS_NOTYETIMPLEMENTED("write me!");
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsXULDocument::CreateAttribute(const nsString& aName, nsIDOMAttr** aReturn)
{
    NS_NOTYETIMPLEMENTED("write me!");
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsXULDocument::CreateEntityReference(const nsString& aName, nsIDOMEntityReference** aReturn)
{
    NS_NOTYETIMPLEMENTED("write me!");
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsXULDocument::GetElementsByTagName(const nsString& aTagName, nsIDOMNodeList** aReturn)
{
    nsresult rv;
    nsRDFDOMNodeList* elements;
    if (NS_FAILED(rv = nsRDFDOMNodeList::Create(&elements))) {
        NS_ERROR("unable to create node list");
        return rv;
    }

    nsIContent* root = GetRootContent();
    NS_ASSERTION(root != nsnull, "no doc root");

    if (root != nsnull) {
        nsIDOMNode* domRoot;
        if (NS_SUCCEEDED(rv = root->QueryInterface(nsIDOMNode::GetIID(), (void**) &domRoot))) {
            rv = GetElementsByTagName(domRoot, aTagName, elements);
            NS_RELEASE(domRoot);
        }
        NS_RELEASE(root);
    }

    *aReturn = elements;
    return NS_OK;
}

NS_IMETHODIMP
nsXULDocument::GetElementsByAttribute(const nsString& aAttribute, const nsString& aValue,
                                        nsIDOMNodeList** aReturn)
{
    nsresult rv;
    nsRDFDOMNodeList* elements;
    if (NS_FAILED(rv = nsRDFDOMNodeList::Create(&elements))) {
        NS_ERROR("unable to create node list");
        return rv;
    }

    nsIContent* root = GetRootContent();
    NS_ASSERTION(root != nsnull, "no doc root");

    if (root != nsnull) {
        nsIDOMNode* domRoot;
        if (NS_SUCCEEDED(rv = root->QueryInterface(nsIDOMNode::GetIID(), (void**) &domRoot))) {
            rv = GetElementsByAttribute(domRoot, aAttribute, aValue, elements);
            NS_RELEASE(domRoot);
        }
        NS_RELEASE(root);
    }

    *aReturn = elements;
    return NS_OK;
}


NS_IMETHODIMP
nsXULDocument::Persist(const nsString& aID, const nsString& aAttr)
{
    nsresult rv;

    nsCOMPtr<nsIDOMElement> domelement;
    rv = GetElementById(aID, getter_AddRefs(domelement));
    if (NS_FAILED(rv)) return rv;

    if (! domelement)
        return NS_OK;

    nsCOMPtr<nsIContent> element = do_QueryInterface(domelement);
    NS_ASSERTION(element != nsnull, "null ptr");
    if (! element)
        return NS_ERROR_UNEXPECTED;

    PRInt32 nameSpaceID;
    nsCOMPtr<nsIAtom> tag;
    rv = element->ParseAttributeString(aAttr, *getter_AddRefs(tag), nameSpaceID);
    if (NS_FAILED(rv)) return rv;

    rv = Persist(element, nameSpaceID, tag);
    if (NS_FAILED(rv)) return rv;

    return NS_OK;
}


nsresult
nsXULDocument::Persist(nsIContent* aElement, PRInt32 aNameSpaceID, nsIAtom* aAttribute)
{
    // First make sure we _have_ a local store to stuff the persited
    // information into. (We might not have one if profile information
    // hasn't been loaded yet...)
    if (! mLocalStore)
        return NS_OK;

    nsresult rv;

    nsCOMPtr<nsIRDFResource> element;
    rv = gXULUtils->GetElementResource(aElement, getter_AddRefs(element));
    if (NS_FAILED(rv)) return rv;

    // No ID, so nothing to persist.
    if (! element)
        return NS_OK;

    // Ick. Construct a property from the attribute. Punt on
    // namespaces for now.
    const PRUnichar* attrstr;
    rv = aAttribute->GetUnicode(&attrstr);
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIRDFResource> attr;
    rv = gRDFService->GetResource(nsCAutoString(attrstr), getter_AddRefs(attr));
    if (NS_FAILED(rv)) return rv;

    // Turn the value into a literal
    nsAutoString valuestr;
    rv = aElement->GetAttribute(kNameSpaceID_None, aAttribute, valuestr);
    if (NS_FAILED(rv)) return rv;

    PRBool novalue = (rv != NS_CONTENT_ATTR_HAS_VALUE);

    // See if there was an old value...
    nsCOMPtr<nsIRDFNode> oldvalue;
    rv = mLocalStore->GetTarget(element, attr, PR_TRUE, getter_AddRefs(oldvalue));
    if (NS_FAILED(rv)) return rv;

    if (oldvalue && novalue) {
        // ...there was an oldvalue, and they've removed it. XXXThis
        // handling isn't quite right...
        rv = mLocalStore->Unassert(element, attr, oldvalue);
    }
    else {
        // Now either 'change' or 'assert' based on whether there was
        // an old value.
        nsCOMPtr<nsIRDFLiteral> newvalue;
        rv = gRDFService->GetLiteral(valuestr.GetUnicode(), getter_AddRefs(newvalue));
        if (NS_FAILED(rv)) return rv;

        if (oldvalue) {
            rv = mLocalStore->Change(element, attr, oldvalue, newvalue);
        }
        else {
            rv = mLocalStore->Assert(element, attr, newvalue, PR_TRUE);
        }
    }

    if (NS_FAILED(rv)) return rv;

    // Add it to the persisted set for this document (if it's not
    // there already).
    {
        nsXPIDLCString docurl;
        rv = mDocumentURL->GetSpec(getter_Copies(docurl));
        if (NS_FAILED(rv)) return rv;

        nsCOMPtr<nsIRDFResource> doc;
        rv = gRDFService->GetResource(docurl, getter_AddRefs(doc));
        if (NS_FAILED(rv)) return rv;

        PRBool hasAssertion;
        rv = mLocalStore->HasAssertion(doc, kNC_persist, element, PR_TRUE, &hasAssertion);
        if (NS_FAILED(rv)) return rv;

        if (! hasAssertion) {
            rv = mLocalStore->Assert(doc, kNC_persist, element, PR_TRUE);
            if (NS_FAILED(rv)) return rv;
        }
    }

    return NS_OK;
}



nsresult
nsXULDocument::DestroyForwardReferences()
{
    for (PRInt32 i = mForwardReferences.Count() - 1; i >= 0; --i) {
        nsForwardReference* fwdref = NS_REINTERPRET_CAST(nsForwardReference*, mForwardReferences[i]);
        delete fwdref;
    }

    mForwardReferences.Clear();
    return NS_OK;
}


//----------------------------------------------------------------------
//
// nsIDOMNSDocument interface
//

NS_IMETHODIMP
nsXULDocument::GetStyleSheets(nsIDOMStyleSheetCollection** aStyleSheets)
{
    NS_NOTYETIMPLEMENTED("write me!");
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsXULDocument::CreateElementWithNameSpace(const nsString& aTagName,
                                            const nsString& aNameSpace,
                                            nsIDOMElement** aResult)
{
    // Create a DOM element given a namespace URI and a tag name.
    NS_PRECONDITION(aResult != nsnull, "null ptr");
    if (! aResult)
        return NS_ERROR_NULL_POINTER;

    nsresult rv;

#ifdef PR_LOGGING
    if (PR_LOG_TEST(gXULLog, PR_LOG_DEBUG)) {
      char* namespaceCStr = aNameSpace.ToNewCString();
      char* tagCStr = aTagName.ToNewCString();

      PR_LOG(gXULLog, PR_LOG_DEBUG,
             ("xul[CreateElementWithNameSpace] [%s]:%s", namespaceCStr, tagCStr));

      nsCRT::free(tagCStr);
      nsCRT::free(namespaceCStr);
    }
#endif

    nsCOMPtr<nsIAtom> name = dont_AddRef(NS_NewAtom(aTagName.GetUnicode()));
    if (! name)
        return NS_ERROR_OUT_OF_MEMORY;

    PRInt32 nameSpaceID;
    rv = mNameSpaceManager->GetNameSpaceID(aNameSpace, nameSpaceID);
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIContent> result;
    rv = CreateElement(nameSpaceID, name, getter_AddRefs(result));
    if (NS_FAILED(rv)) return rv;

    // get the DOM interface
    rv = result->QueryInterface(nsIDOMElement::GetIID(), (void**) aResult);
    NS_ASSERTION(NS_SUCCEEDED(rv), "not a DOM element");
    if (NS_FAILED(rv)) return rv;

    return NS_OK;
}


NS_IMETHODIMP
nsXULDocument::CreateRange(nsIDOMRange** aRange)
{
    NS_NOTYETIMPLEMENTED("write me!");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsXULDocument::GetWidth(PRInt32* aWidth)
{
    NS_NOTYETIMPLEMENTED("write me!");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsXULDocument::GetHeight(PRInt32* aHeight)
{
    NS_NOTYETIMPLEMENTED("write me!");
    return NS_ERROR_NOT_IMPLEMENTED;
}

//----------------------------------------------------------------------
//
// nsIDOMXULDocument interface
//

NS_IMETHODIMP
nsXULDocument::GetPopupNode(nsIDOMNode** aNode)
{
    *aNode = mPopupNode;
    NS_IF_ADDREF(*aNode);
    return NS_OK;
}

NS_IMETHODIMP
nsXULDocument::SetPopupNode(nsIDOMNode* aNode)
{
    mPopupNode = dont_QueryInterface(aNode);
    return NS_OK;
}

NS_IMETHODIMP
nsXULDocument::GetTooltipNode(nsIDOMNode** aNode)
{
    *aNode = mTooltipNode;
    NS_IF_ADDREF(*aNode);
    return NS_OK;
}

NS_IMETHODIMP
nsXULDocument::SetTooltipNode(nsIDOMNode* aNode)
{
    mTooltipNode = dont_QueryInterface(aNode);
    return NS_OK;
}


NS_IMETHODIMP
nsXULDocument::GetCommandDispatcher(nsIDOMXULCommandDispatcher** aTracker)
{
  *aTracker = mCommandDispatcher;
  NS_IF_ADDREF(*aTracker);
  return NS_OK;
}

NS_IMETHODIMP
nsXULDocument::GetElementById(const nsString& aId, nsIDOMElement** aReturn)
{
    nsresult rv;

    nsCOMPtr<nsIContent> element;
    rv = mElementMap.FindFirst(aId, getter_AddRefs(element));
    if (NS_FAILED(rv)) return rv;

    if (element) {
        rv = element->QueryInterface(NS_GET_IID(nsIDOMElement), (void**) aReturn);
    }
    else {
        *aReturn = nsnull;
        rv = NS_OK;
    }

    return rv;
}

nsresult
nsXULDocument::AddSubtreeToDocument(nsIContent* aElement)
{
    // Do a bunch of work that's necessary when an element gets added
    // to the XUL Document.
    nsresult rv;

    // 1. Add the element to the resource-to-element map
    rv = AddElementToMap(aElement);
    if (NS_FAILED(rv)) return rv;

    // 2. If the element is a 'command updater' (i.e., has a
    // "commandupdater='true'" attribute), then add the element to the
    // document's command dispatcher
    nsAutoString value;
    rv = aElement->GetAttribute(kNameSpaceID_None, kCommandUpdaterAtom, value);
    if ((rv == NS_CONTENT_ATTR_HAS_VALUE) && value.Equals("true")) {
        rv = gXULUtils->SetCommandUpdater(this, aElement);
        if (NS_FAILED(rv)) return rv;
    }

    // 3. Check for a broadcaster hookup attribute, in which case
    // we'll hook the node up as a listener on a broadcaster.
    PRBool listener, resolved;
    rv = CheckBroadcasterHookup(this, aElement, &listener, &resolved);
    if (NS_FAILED(rv)) return rv;

    // If it's not there yet, we may be able to defer hookup until
    // later.
    if (listener && !resolved && (mResolutionPhase != nsForwardReference::eDone)) {
        BroadcasterHookup* hookup = new BroadcasterHookup(this, aElement);
        if (! hookup)
            return NS_ERROR_OUT_OF_MEMORY;

        rv = AddForwardReference(hookup);
        if (NS_FAILED(rv)) return rv;
    }

    // Finally, recurse to children.
    PRInt32 count;
    nsCOMPtr<nsIXULContent> xulcontent = do_QueryInterface(aElement);
    rv = xulcontent ? xulcontent->PeekChildCount(count) : aElement->ChildCount(count);
    if (NS_FAILED(rv)) return rv;

    while (--count >= 0) {
        nsCOMPtr<nsIContent> child;
        rv = aElement->ChildAt(count, *getter_AddRefs(child));
        if (NS_FAILED(rv)) return rv;

        rv = AddSubtreeToDocument(child);
        if (NS_FAILED(rv)) return rv;
    }

    return NS_OK;
}

nsresult
nsXULDocument::RemoveSubtreeFromDocument(nsIContent* aElement)
{
    // Do a bunch of cleanup to remove an element from the XUL
    // document.
    nsresult rv;

    PRInt32 count;
    nsCOMPtr<nsIXULContent> xulcontent = do_QueryInterface(aElement);
    rv = xulcontent ? xulcontent->PeekChildCount(count) : aElement->ChildCount(count);
    if (NS_FAILED(rv)) return rv;

    while (--count >= 0) {
        nsCOMPtr<nsIContent> child;
        rv = aElement->ChildAt(count, *getter_AddRefs(child));
        if (NS_FAILED(rv)) return rv;

        rv = RemoveSubtreeFromDocument(child);
        if (NS_FAILED(rv)) return rv;
    }

    // 1. Remove the element from the resource-to-element map
    rv = RemoveElementFromMap(aElement);
    if (NS_FAILED(rv)) return rv;

    // 2. If the element is a 'command updater', then remove the
    // element from the document's command dispatcher.
    nsAutoString value;
    rv = aElement->GetAttribute(kNameSpaceID_None, kCommandUpdaterAtom, value);
    if ((rv == NS_CONTENT_ATTR_HAS_VALUE) && value.Equals("true")) {
        nsCOMPtr<nsIDOMElement> domelement = do_QueryInterface(aElement);
        NS_ASSERTION(domelement != nsnull, "not a DOM element");
        if (! domelement)
            return NS_ERROR_UNEXPECTED;

        rv = mCommandDispatcher->RemoveCommandUpdater(domelement);
        if (NS_FAILED(rv)) return rv;
    }

    return NS_OK;
}

// Attributes that are used with getElementById() and the
// resource-to-element map.
nsIAtom** nsXULDocument::kIdentityAttrs[] = { &kIdAtom, &kRefAtom, nsnull };

nsresult
nsXULDocument::AddElementToMap(nsIContent* aElement)
{
    // Look at the element's 'id' and 'ref' attributes, and if set,
    // add pointers in the resource-to-element map to the element.
    nsresult rv;

    for (PRInt32 i = 0; kIdentityAttrs[i] != nsnull; ++i) {
        nsAutoString value;
        rv = aElement->GetAttribute(kNameSpaceID_None, *kIdentityAttrs[i], value);
        NS_ASSERTION(NS_SUCCEEDED(rv), "unable to get attribute");
        if (NS_FAILED(rv)) return rv;

        if (rv == NS_CONTENT_ATTR_HAS_VALUE) {
            rv = mElementMap.Add(value, aElement);
            if (NS_FAILED(rv)) return rv;
        }
    }

    return NS_OK;
}


nsresult
nsXULDocument::RemoveElementFromMap(nsIContent* aElement)
{
    // Remove the element from the resource-to-element map.
    nsresult rv;

    for (PRInt32 i = 0; kIdentityAttrs[i] != nsnull; ++i) {
        nsAutoString value;
        rv = aElement->GetAttribute(kNameSpaceID_None, *kIdentityAttrs[i], value);
        NS_ASSERTION(NS_SUCCEEDED(rv), "unable to get attribute");
        if (NS_FAILED(rv)) return rv;

        if (rv == NS_CONTENT_ATTR_HAS_VALUE) {
            rv = mElementMap.Remove(value, aElement);
            if (NS_FAILED(rv)) return rv;
        }
    }

    return NS_OK;
}


PRIntn
nsXULDocument::RemoveElementsFromMapByContent(const nsString& aID,
                                                nsIContent* aElement,
                                                void* aClosure)
{
    nsIContent* content = NS_REINTERPRET_CAST(nsIContent*, aClosure);
    return (aElement == content) ? HT_ENUMERATE_REMOVE : HT_ENUMERATE_NEXT;
}



//----------------------------------------------------------------------
//
// nsIDOMNode interface
//

NS_IMETHODIMP
nsXULDocument::GetNodeName(nsString& aNodeName)
{
    aNodeName.SetString("#document");
    return NS_OK;
}


NS_IMETHODIMP
nsXULDocument::GetNodeValue(nsString& aNodeValue)
{
    aNodeValue.Truncate();
    return NS_OK;
}


NS_IMETHODIMP
nsXULDocument::SetNodeValue(const nsString& aNodeValue)
{
    return NS_OK;
}


NS_IMETHODIMP
nsXULDocument::GetNodeType(PRUint16* aNodeType)
{
    *aNodeType = nsIDOMNode::DOCUMENT_NODE;
    return NS_OK;
}


NS_IMETHODIMP
nsXULDocument::GetParentNode(nsIDOMNode** aParentNode)
{
    *aParentNode = nsnull;
    return NS_OK;
}


NS_IMETHODIMP
nsXULDocument::GetChildNodes(nsIDOMNodeList** aChildNodes)
{
    NS_PRECONDITION(aChildNodes != nsnull, "null ptr");
    if (! aChildNodes)
        return NS_ERROR_NULL_POINTER;

    if (mRootContent) {
        nsresult rv;

        *aChildNodes = nsnull;

        nsRDFDOMNodeList* children;
        rv = nsRDFDOMNodeList::Create(&children);

        if (NS_SUCCEEDED(rv)) {
            nsIDOMNode* domNode = nsnull;
            rv = mRootContent->QueryInterface(nsIDOMNode::GetIID(), (void**)&domNode);
            NS_ASSERTION(NS_SUCCEEDED(rv), "root content is not a DOM node");

            if (NS_SUCCEEDED(rv)) {
                rv = children->AppendNode(domNode);
                NS_RELEASE(domNode);

                *aChildNodes = children;
                return NS_OK;
            }
        }

        // If we get here, something bad happened.
        NS_RELEASE(children);
        return rv;
    }
    else {
        *aChildNodes = nsnull;
        return NS_OK;
    }
}


NS_IMETHODIMP
nsXULDocument::HasChildNodes(PRBool* aHasChildNodes)
{
    NS_PRECONDITION(aHasChildNodes != nsnull, "null ptr");
    if (! aHasChildNodes)
        return NS_ERROR_NULL_POINTER;

    if (mRootContent) {
        *aHasChildNodes = PR_TRUE;
    }
    else {
        *aHasChildNodes = PR_FALSE;
    }
    return NS_OK;
}


NS_IMETHODIMP
nsXULDocument::GetFirstChild(nsIDOMNode** aFirstChild)
{
    NS_PRECONDITION(aFirstChild != nsnull, "null ptr");
    if (! aFirstChild)
        return NS_ERROR_NULL_POINTER;

    if (mRootContent) {
        return mRootContent->QueryInterface(nsIDOMNode::GetIID(), (void**) aFirstChild);
    }
    else {
        *aFirstChild = nsnull;
        return NS_OK;
    }
}


NS_IMETHODIMP
nsXULDocument::GetLastChild(nsIDOMNode** aLastChild)
{
    NS_PRECONDITION(aLastChild != nsnull, "null ptr");
    if (! aLastChild)
        return NS_ERROR_NULL_POINTER;

    if (mRootContent) {
        return mRootContent->QueryInterface(nsIDOMNode::GetIID(), (void**) aLastChild);
    }
    else {
        *aLastChild = nsnull;
        return NS_OK;
    }
}


NS_IMETHODIMP
nsXULDocument::GetPreviousSibling(nsIDOMNode** aPreviousSibling)
{
    NS_PRECONDITION(aPreviousSibling != nsnull, "null ptr");
    if (! aPreviousSibling)
        return NS_ERROR_NULL_POINTER;

    *aPreviousSibling = nsnull;
    return NS_OK;
}


NS_IMETHODIMP
nsXULDocument::GetNextSibling(nsIDOMNode** aNextSibling)
{
    NS_PRECONDITION(aNextSibling != nsnull, "null ptr");
    if (! aNextSibling)
        return NS_ERROR_NULL_POINTER;

    *aNextSibling = nsnull;
    return NS_OK;
}


NS_IMETHODIMP
nsXULDocument::GetAttributes(nsIDOMNamedNodeMap** aAttributes)
{
    NS_PRECONDITION(aAttributes != nsnull, "null ptr");
    if (! aAttributes)
        return NS_ERROR_NULL_POINTER;

    *aAttributes = nsnull;
    return NS_OK;
}


NS_IMETHODIMP
nsXULDocument::GetOwnerDocument(nsIDOMDocument** aOwnerDocument)
{
    NS_PRECONDITION(aOwnerDocument != nsnull, "null ptr");
    if (! aOwnerDocument)
        return NS_ERROR_NULL_POINTER;

    *aOwnerDocument = nsnull;
    return NS_OK;
}


NS_IMETHODIMP
nsXULDocument::InsertBefore(nsIDOMNode* aNewChild, nsIDOMNode* aRefChild, nsIDOMNode** aReturn)
{
    NS_NOTYETIMPLEMENTED("write me!");
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsXULDocument::ReplaceChild(nsIDOMNode* aNewChild, nsIDOMNode* aOldChild, nsIDOMNode** aReturn)
{
    NS_NOTYETIMPLEMENTED("write me!");
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsXULDocument::RemoveChild(nsIDOMNode* aOldChild, nsIDOMNode** aReturn)
{
    NS_NOTYETIMPLEMENTED("write me!");
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsXULDocument::AppendChild(nsIDOMNode* aNewChild, nsIDOMNode** aReturn)
{
    NS_NOTYETIMPLEMENTED("write me!");
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsXULDocument::CloneNode(PRBool aDeep, nsIDOMNode** aReturn)
{
    // We don't allow cloning of a document
    *aReturn = nsnull;
    return NS_OK;
}


//----------------------------------------------------------------------
//
// nsIJSScriptObject interface
//

PRBool
nsXULDocument::AddProperty(JSContext *aContext, jsval aID, jsval *aVp)
{
    NS_NOTYETIMPLEMENTED("write me");
    return PR_TRUE;
}


PRBool
nsXULDocument::DeleteProperty(JSContext *aContext, jsval aID, jsval *aVp)
{
    NS_NOTYETIMPLEMENTED("write me");
    return PR_TRUE;
}


PRBool
nsXULDocument::GetProperty(JSContext *aContext, jsval aID, jsval *aVp)
{
    if (JSVAL_IS_STRING(aID)) {
        JSString *jsString = JS_ValueToString(aContext, aID);
        if (!jsString)
            return PR_FALSE;

        if (PL_strcmp("location", JS_GetStringBytes(jsString)) == 0) {
            if (nsnull != mScriptContextOwner) {
                nsCOMPtr<nsIScriptGlobalObject> global;
                mScriptContextOwner->GetScriptGlobalObject(getter_AddRefs(global));
                if (nsnull != global) {
                    nsCOMPtr<nsIJSScriptObject> window = do_QueryInterface(global);
                    if (nsnull != window) {
                        return window->GetProperty(aContext, aID, aVp);
                    }
                }
            }
        }
    }

    return PR_TRUE;
}


PRBool
nsXULDocument::SetProperty(JSContext *aContext, jsval aID, jsval *aVp)
{
    nsresult rv;

    if (JSVAL_IS_STRING(aID)) {
        char* s = JS_GetStringBytes(JS_ValueToString(aContext, aID));
        if (PL_strcmp("title", s) == 0) {
            JSString* jsString = JS_ValueToString(aContext, *aVp);
            if (!jsString)
                return PR_FALSE;
            nsAutoString title(JS_GetStringChars(jsString));
            for (PRInt32 i = mPresShells.Count() - 1; i >= 0; --i) {
                nsIPresShell* shell = NS_STATIC_CAST(nsIPresShell*, mPresShells[i]);
                nsCOMPtr<nsIPresContext> context;
                rv = shell->GetPresContext(getter_AddRefs(context));
                if (NS_FAILED(rv)) return PR_FALSE;

                nsCOMPtr<nsISupports> container;
                rv = context->GetContainer(getter_AddRefs(container));
                if (NS_FAILED(rv)) return PR_FALSE;

                if (! container) continue;

                nsCOMPtr<nsIBaseWindow> webShellWin = do_QueryInterface(container);
                if(!webShellWin) continue;

                NS_ENSURE_SUCCESS(webShellWin->SetTitle(title.GetUnicode()),
                  PR_FALSE);
            }
        }
        else if (PL_strcmp("location", s) == 0) {
            NS_NOTYETIMPLEMENTED("write me");
            return PR_FALSE;
        }
    }
    return PR_TRUE;
}


PRBool
nsXULDocument::EnumerateProperty(JSContext *aContext)
{
    NS_NOTYETIMPLEMENTED("write me");
    return PR_TRUE;
}


PRBool
nsXULDocument::Resolve(JSContext *aContext, jsval aID)
{
    return PR_TRUE;
}


PRBool
nsXULDocument::Convert(JSContext *aContext, jsval aID)
{
    NS_NOTYETIMPLEMENTED("write me");
    return PR_TRUE;
}


void
nsXULDocument::Finalize(JSContext *aContext)
{
    NS_NOTYETIMPLEMENTED("write me");
}



//----------------------------------------------------------------------
//
// nsIScriptObjectOwner interface
//

NS_IMETHODIMP
nsXULDocument::GetScriptObject(nsIScriptContext *aContext, void** aScriptObject)
{
    if (! mScriptObject) {
        // ...we need to instantiate our script object for the first
        // time.

        // Make sure that we've got our script context owner; this
        // assertion will fire if we've tried to get the script object
        // before our scope has been set up.
        NS_ASSERTION(mScriptContextOwner != nsnull, "no script context owner");
        if (! mScriptContextOwner)
            return NS_ERROR_NOT_INITIALIZED;

        nsresult rv;

        // Use the global object from our script context owner (the
        // window) as the parent of our own script object. (Using the
        // global object from aContext would make our script object
        // dynamically scoped in the first context that ever tried to
        // use us!)
        nsCOMPtr<nsIScriptGlobalObject> global;
        rv = mScriptContextOwner->GetScriptGlobalObject(getter_AddRefs(global));
        if (NS_FAILED(rv)) return rv;

        NS_ASSERTION(global != nsnull, "script context owner has no global object");
        if (! global)
            return NS_ERROR_UNEXPECTED;

        rv = NS_NewScriptXULDocument(aContext,
                                     NS_STATIC_CAST(nsISupports*, NS_STATIC_CAST(nsIDOMXULDocument*, this)),
                                     global, &mScriptObject);
        if (NS_FAILED(rv)) return rv;
    }

    *aScriptObject = mScriptObject;
    return NS_OK;
}


NS_IMETHODIMP
nsXULDocument::SetScriptObject(void *aScriptObject)
{
    mScriptObject = aScriptObject;
    return NS_OK;
}


//----------------------------------------------------------------------
//
// nsIHTMLContentContainer interface
//

NS_IMETHODIMP
nsXULDocument::GetAttributeStyleSheet(nsIHTMLStyleSheet** aResult)
{
    NS_PRECONDITION(nsnull != aResult, "null ptr");
    if (nsnull == aResult) {
        return NS_ERROR_NULL_POINTER;
    }
    *aResult = mAttrStyleSheet;
    if (! mAttrStyleSheet) {
        return NS_ERROR_NOT_AVAILABLE;  // probably not the right error...
    }
    else {
        NS_ADDREF(*aResult);
    }
    return NS_OK;
}

NS_IMETHODIMP
nsXULDocument::GetInlineStyleSheet(nsIHTMLCSSStyleSheet** aResult)
{
    NS_PRECONDITION(nsnull != aResult, "null ptr");
    if (nsnull == aResult) {
        return NS_ERROR_NULL_POINTER;
    }
    *aResult = mInlineStyleSheet;
    if (!mInlineStyleSheet) {
        return NS_ERROR_NOT_AVAILABLE;  // probably not the right error...
    }
    else {
        NS_ADDREF(*aResult);
    }
    return NS_OK;
}

//----------------------------------------------------------------------
//
// Implementation methods
//

nsIContent*
nsXULDocument::FindContent(const nsIContent* aStartNode,
                             const nsIContent* aTest1,
                             const nsIContent* aTest2) const
{
    PRInt32 count;
    aStartNode->ChildCount(count);

    PRInt32 i;
    for(i = 0; i < count; i++) {
        nsIContent* child;
        aStartNode->ChildAt(i, child);
        nsIContent* content = FindContent(child,aTest1,aTest2);
        if (content != nsnull) {
            NS_IF_RELEASE(child);
            return content;
        }
        if (child == aTest1 || child == aTest2) {
            NS_IF_RELEASE(content);
            return child;
        }
        NS_IF_RELEASE(child);
        NS_IF_RELEASE(content);
    }
    return nsnull;
}


nsresult
nsXULDocument::Init()
{
    nsresult rv;

    rv = NS_NewHeapArena(getter_AddRefs(mArena), nsnull);
    if (NS_FAILED(rv)) return rv;

    // Create a namespace manager so we can manage tags
    rv = nsComponentManager::CreateInstance(kNameSpaceManagerCID,
                                            nsnull,
                                            NS_GET_IID(nsINameSpaceManager),
                                            getter_AddRefs(mNameSpaceManager));
    if (NS_FAILED(rv)) return rv;

    // Create our focus tracker and hook it up.
    nsCOMPtr<nsIXULCommandDispatcher> commandDis;
    rv = nsComponentManager::CreateInstance(kXULCommandDispatcherCID,
                                            nsnull,
                                            NS_GET_IID(nsIXULCommandDispatcher),
                                            getter_AddRefs(commandDis));
    NS_ASSERTION(NS_SUCCEEDED(rv), "unable to create a focus tracker");
    if (NS_FAILED(rv)) return rv;

    mCommandDispatcher = do_QueryInterface(commandDis);

    nsCOMPtr<nsIDOMEventListener> CommandDispatcher =
        do_QueryInterface(mCommandDispatcher);

    if (CommandDispatcher) {
      // Take the focus tracker and add it as an event listener for focus and blur events.
        AddEventListener("focus", CommandDispatcher, PR_TRUE);
        AddEventListener("blur", CommandDispatcher, PR_TRUE);
    }

    // Get the local store. Yeah, I know. I wish GetService() used a
    // 'void**', too.
    nsIRDFDataSource* localstore;
    rv = nsServiceManager::GetService(kLocalStoreCID,
                                      NS_GET_IID(nsIRDFDataSource),
                                      (nsISupports**) &localstore);

    // this _could_ fail; e.g., if we've tried to grab the local store
    // before profiles have initialized. If so, no big deal; nothing
    // will persist.

    if (NS_SUCCEEDED(rv)) {
        mLocalStore = localstore;
        NS_IF_RELEASE(localstore);
    }

    // Create a new nsISupportsArray for dealing with overlay references
    rv = NS_NewISupportsArray(getter_AddRefs(mUnloadedOverlays));
    if (NS_FAILED(rv)) return rv;

    // Create a new nsISupportsArray that will hold owning references
    // to each of the prototype documents used to construct this
    // document. That will ensure that prototype elements will remain
    // alive.
    rv = NS_NewISupportsArray(getter_AddRefs(mPrototypes));
    if (NS_FAILED(rv)) return rv;

#if 0
    // construct a selection object
    if (NS_FAILED(rv = nsComponentManager::CreateInstance(kRangeListCID,
                                                    nsnull,
                                                    kIDOMSelectionIID,
                                                    (void**) &mSelection))) {
        NS_ERROR("unable to create DOM selection");
    }
#endif

    if (gRefCnt++ == 0) {
        kAttributeAtom      = NS_NewAtom("attribute");
        kCommandUpdaterAtom = NS_NewAtom("commandupdater");
        kDataSourcesAtom    = NS_NewAtom("datasources");
        kElementAtom        = NS_NewAtom("element");
        kIdAtom             = NS_NewAtom("id");
        kKeysetAtom         = NS_NewAtom("keyset");
        kObservesAtom       = NS_NewAtom("observes");
        kOpenAtom           = NS_NewAtom("open");
        kOverlayAtom        = NS_NewAtom("overlay");
        kPersistAtom        = NS_NewAtom("persist");
        kPositionAtom       = NS_NewAtom("position");
        kRefAtom            = NS_NewAtom("ref");
        kRuleAtom           = NS_NewAtom("rule");
        kTemplateAtom       = NS_NewAtom("template");

        // Keep the RDF service cached in a member variable to make using
        // it a bit less painful
        rv = nsServiceManager::GetService(kRDFServiceCID,
                                          NS_GET_IID(nsIRDFService),
                                          (nsISupports**) &gRDFService);

        NS_ASSERTION(NS_SUCCEEDED(rv), "unable to get RDF Service");
        if (NS_FAILED(rv)) return rv;

        gRDFService->GetResource(NC_NAMESPACE_URI "persist",   &kNC_persist);
        gRDFService->GetResource(NC_NAMESPACE_URI "attribute", &kNC_attribute);
        gRDFService->GetResource(NC_NAMESPACE_URI "value",     &kNC_value);

        rv = nsComponentManager::CreateInstance(kHTMLElementFactoryCID,
                                                nsnull,
                                                NS_GET_IID(nsIHTMLElementFactory),
                                                (void**) &gHTMLElementFactory);

        NS_ASSERTION(NS_SUCCEEDED(rv), "unable to get HTML element factory");
        if (NS_FAILED(rv)) return rv;

        rv = nsComponentManager::CreateInstance(kXMLElementFactoryCID,
                                                nsnull,
                                                NS_GET_IID(nsIXMLElementFactory),
                                                (void**) &gXMLElementFactory);
        NS_ASSERTION(NS_SUCCEEDED(rv), "unable to get XML element factory");
        if (NS_FAILED(rv)) return rv;

        rv = nsServiceManager::GetService(kNameSpaceManagerCID,
                                          NS_GET_IID(nsINameSpaceManager),
                                          (nsISupports**) &gNameSpaceManager);
        NS_ASSERTION(NS_SUCCEEDED(rv), "unable to get namespace manager");
        if (NS_FAILED(rv)) return rv;

#define XUL_NAMESPACE_URI "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul"
static const char kXULNameSpaceURI[] = XUL_NAMESPACE_URI;
        gNameSpaceManager->RegisterNameSpace(kXULNameSpaceURI, kNameSpaceID_XUL);


        rv = nsServiceManager::GetService(kXULContentUtilsCID,
                                          NS_GET_IID(nsIXULContentUtils),
                                          (nsISupports**) &gXULUtils);
        if (NS_FAILED(rv)) return rv;

        rv = nsServiceManager::GetService(kXULPrototypeCacheCID,
                                          NS_GET_IID(nsIXULPrototypeCache),
                                          (nsISupports**) &gXULCache);
        if (NS_FAILED(rv)) return rv;
    }

#ifdef PR_LOGGING
    if (! gXULLog)
        gXULLog = PR_NewLogModule("nsXULDocument");
#endif

    return NS_OK;
}



nsresult
nsXULDocument::StartLayout(void)
{
    if (! mRootContent) {
#ifdef PR_LOGGING
        nsXPIDLCString urlspec;
        mDocumentURL->GetSpec(getter_Copies(urlspec));

        PR_LOG(gXULLog, PR_LOG_ALWAYS,
               ("xul: unable to layout '%s'; no root content", (const char*) urlspec));
#endif
        return NS_OK;
    }

    PRInt32 count = GetNumberOfShells();
    for (PRInt32 i = 0; i < count; i++) {
      nsIPresShell* shell = GetShellAt(i);
      if (nsnull == shell)
          continue;

      // Resize-reflow this time
      nsCOMPtr<nsIPresContext> cx;
      shell->GetPresContext(getter_AddRefs(cx));

      PRBool intrinsic = PR_FALSE;
      nsCOMPtr<nsIWebShell> webShell;
      nsCOMPtr<nsIBrowserWindow> browser;

      if (cx) {
          nsCOMPtr<nsISupports> container;
          cx->GetContainer(getter_AddRefs(container));
          if (container) {
              webShell = do_QueryInterface(container);
              if (webShell) {
                  webShell->SetScrolling(NS_STYLE_OVERFLOW_HIDDEN);
                  nsCOMPtr<nsIWebShellContainer> webShellContainer;
                  webShell->GetContainer(*getter_AddRefs(webShellContainer));
                  if (webShellContainer) {
                      browser = do_QueryInterface(webShellContainer);
                      if (browser)
                          browser->IsIntrinsicallySized(intrinsic);
                  }
              }
          }
      }

      nsRect r;
      cx->GetVisibleArea(r);
      if (intrinsic) {
        // Flow at an unconstrained width and height
        r.width = NS_UNCONSTRAINEDSIZE;
        r.height = NS_UNCONSTRAINEDSIZE;
      }

      if (browser) {
        // We're top-level.
        // See if we have attributes on our root tag that set the width and height.
        // read "height" attribute// Convert r.width and r.height to twips.
        float p2t;
        cx->GetPixelsToTwips(&p2t);

        nsCOMPtr<nsIDOMElement> windowElement = do_QueryInterface(mRootContent);
        nsString sizeString;
        PRInt32 specSize;
        PRInt32 errorCode;
        if (NS_SUCCEEDED(windowElement->GetAttribute("height", sizeString))) {
          specSize = sizeString.ToInteger(&errorCode);
          if (NS_SUCCEEDED(errorCode) && specSize > 0)
            r.height = NSIntPixelsToTwips(specSize, p2t);
        }

        // read "width" attribute
        if (NS_SUCCEEDED(windowElement->GetAttribute("width", sizeString))) {
          specSize = sizeString.ToInteger(&errorCode);
          if (NS_SUCCEEDED(errorCode) || specSize > 0)
            r.width = NSIntPixelsToTwips(specSize, p2t);
        }
      }

      cx->SetVisibleArea(r);

      // XXX Copy of the code below. See XXX below for details...
      // Now trigger a refresh
      nsCOMPtr<nsIViewManager> vm;
      shell->GetViewManager(getter_AddRefs(vm));
      if (vm) {
        nsCOMPtr<nsIContentViewer> contentViewer;
        nsresult rv = webShell->GetContentViewer(getter_AddRefs(contentViewer));
        if (NS_SUCCEEDED(rv) && (contentViewer != nsnull)) {
          PRBool enabled;
          contentViewer->GetEnableRendering(&enabled);
          if (enabled) {
            vm->EnableRefresh();
          }
        }
      }

      shell->InitialReflow(r.width, r.height);

      if (browser) {
        // We're top level.
        // Retrieve the answer.
        cx->GetVisibleArea(r);

        // Perform the resize
        PRInt32 chromeX,chromeY,chromeWidth,chromeHeight;
        nsCOMPtr<nsIBaseWindow> webShellWin(do_QueryInterface(webShell));
        NS_ABORT_IF_FALSE(webShellWin, "QI Failed!!!!");
        webShellWin->GetPositionAndSize(&chromeX, &chromeY, &chromeWidth,
         &chromeHeight);

        float t2p;
        cx->GetTwipsToPixels(&t2p);
        PRInt32 width = PRInt32((float)r.width*t2p);
        PRInt32 height = PRInt32((float)r.height*t2p);

        PRInt32 widthDelta = width - chromeWidth;
        PRInt32 heightDelta = height - chromeHeight;

        nsRect windowBounds;
        browser->GetWindowBounds(windowBounds);
        browser->SizeWindowTo(windowBounds.width + widthDelta,
                              windowBounds.height + heightDelta);
      }

      // XXX Moving this call up before the call to InitialReflow(), because
      // the view manager's UpdateView() function is dropping dirty rects if
      // refresh is disabled rather than accumulating them until refresh is
      // enabled and then triggering a repaint...
#if 0
      // Now trigger a refresh
      nsCOMPtr<nsIViewManager> vm;
      shell->GetViewManager(getter_AddRefs(vm));
      if (vm) {
        nsCOMPtr<nsIContentViewer> contentViewer;
        nsresult rv = webShell->GetContentViewer(getter_AddRefs(contentViewer));
        if (NS_SUCCEEDED(rv) && (contentViewer != nsnull)) {
          PRBool enabled;
          contentViewer->GetEnableRendering(&enabled);
          if (enabled) {
            vm->EnableRefresh();
          }
        }
      }
#endif

      // Start observing the document _after_ we do the initial
      // reflow. Otherwise, we'll get into an trouble trying to
      // create kids before the root frame is established.
      shell->BeginObservingDocument();

      NS_RELEASE(shell);
    }
    return NS_OK;
}


nsresult
nsXULDocument::GetElementsByTagName(nsIDOMNode* aNode,
                                      const nsString& aTagName,
                                      nsRDFDOMNodeList* aElements)
{
    nsresult rv;

    nsCOMPtr<nsIDOMElement> element;
    element = do_QueryInterface(aNode);
    if (!element)
      return NS_OK;

    if (aTagName.Equals("*")) {
        if (NS_FAILED(rv = aElements->AppendNode(aNode))) {
            NS_ERROR("unable to append element to node list");
            return rv;
        }
    }
    else {
        nsAutoString name;
        if (NS_FAILED(rv = aNode->GetNodeName(name))) {
            NS_ERROR("unable to get node name");
            return rv;
        }

        if (aTagName.Equals(name)) {
            if (NS_FAILED(rv = aElements->AppendNode(aNode))) {
                NS_ERROR("unable to append element to node list");
                return rv;
            }
        }
    }

    nsCOMPtr<nsIDOMNodeList> children;
    if (NS_FAILED(rv = aNode->GetChildNodes( getter_AddRefs(children) ))) {
        NS_ERROR("unable to get node's children");
        return rv;
    }

    // no kids: terminate the recursion
    if (! children)
        return NS_OK;

    PRUint32 length;
    if (NS_FAILED(children->GetLength(&length))) {
        NS_ERROR("unable to get node list's length");
        return rv;
    }

    for (PRUint32 i = 0; i < length; ++i) {
        nsCOMPtr<nsIDOMNode> child;
        if (NS_FAILED(rv = children->Item(i, getter_AddRefs(child) ))) {
            NS_ERROR("unable to get child from list");
            return rv;
        }

        if (NS_FAILED(rv = GetElementsByTagName(child, aTagName, aElements))) {
            NS_ERROR("unable to recursively get elements by tag name");
            return rv;
        }
    }

    return NS_OK;
}

nsresult
nsXULDocument::GetElementsByAttribute(nsIDOMNode* aNode,
                                        const nsString& aAttribute,
                                        const nsString& aValue,
                                        nsRDFDOMNodeList* aElements)
{
    nsresult rv;

    nsCOMPtr<nsIDOMElement> element;
    element = do_QueryInterface(aNode);
    if (!element)
        return NS_OK;

    nsAutoString attrValue;
    if (NS_FAILED(rv = element->GetAttribute(aAttribute, attrValue))) {
        NS_ERROR("unable to get attribute value");
        return rv;
    }

    if ((attrValue == aValue) || (attrValue.Length() > 0 && aValue == "*")) {
        if (NS_FAILED(rv = aElements->AppendNode(aNode))) {
            NS_ERROR("unable to append element to node list");
            return rv;
        }
    }

    nsCOMPtr<nsIDOMNodeList> children;
    if (NS_FAILED(rv = aNode->GetChildNodes( getter_AddRefs(children) ))) {
        NS_ERROR("unable to get node's children");
        return rv;
    }

    // no kids: terminate the recursion
    if (! children)
        return NS_OK;

    PRUint32 length;
    if (NS_FAILED(children->GetLength(&length))) {
        NS_ERROR("unable to get node list's length");
        return rv;
    }

    for (PRUint32 i = 0; i < length; ++i) {
        nsCOMPtr<nsIDOMNode> child;
        if (NS_FAILED(rv = children->Item(i, getter_AddRefs(child) ))) {
            NS_ERROR("unable to get child from list");
            return rv;
        }

        if (NS_FAILED(rv = GetElementsByAttribute(child, aAttribute, aValue, aElements))) {
            NS_ERROR("unable to recursively get elements by attribute");
            return rv;
        }
    }

    return NS_OK;
}



nsresult
nsXULDocument::ParseTagString(const nsString& aTagName, nsIAtom*& aName, PRInt32& aNameSpaceID)
{
    // Parse the tag into a name and a namespace ID. This is slightly
    // different than nsIContent::ParseAttributeString() because we
    // take the default namespace into account (rather than just
    // assigning "no namespace") in the case that there is no
    // namespace prefix present.

static char kNameSpaceSeparator = ':';

    // XXX this is a gross hack, but it'll carry us for now. We parse
    // the tag name using the root content, which presumably has all
    // the namespace info we need.
    NS_PRECONDITION(mRootContent != nsnull, "not initialized");
    if (! mRootContent)
        return NS_ERROR_NOT_INITIALIZED;

    nsCOMPtr<nsIXMLContent> xml( do_QueryInterface(mRootContent) );
    if (! xml) return NS_ERROR_UNEXPECTED;

    nsresult rv;
    nsCOMPtr<nsINameSpace> ns;
    rv = xml->GetContainingNameSpace(*getter_AddRefs(ns));
    if (NS_FAILED(rv)) return rv;

    NS_ASSERTION(ns != nsnull, "expected xml namespace info to be available");
    if (! ns)
        return NS_ERROR_UNEXPECTED;

    nsAutoString prefix;
    nsAutoString name(aTagName);
    PRInt32 nsoffset = name.FindChar(kNameSpaceSeparator);
    if (-1 != nsoffset) {
        name.Left(prefix, nsoffset);
        name.Cut(0, nsoffset+1);
    }

    // Figure out the namespace ID
    nsCOMPtr<nsIAtom> nameSpaceAtom;
    if (0 < prefix.Length())
        nameSpaceAtom = getter_AddRefs(NS_NewAtom(prefix));

    rv = ns->FindNameSpaceID(nameSpaceAtom, aNameSpaceID);
    if (NS_FAILED(rv)) return rv;

    aName = NS_NewAtom(name);
    return NS_OK;
}


// nsIDOMEventCapturer and nsIDOMEventReceiver Interface Implementations

NS_IMETHODIMP
nsXULDocument::AddEventListenerByIID(nsIDOMEventListener *aListener, const nsIID& aIID)
{
    nsIEventListenerManager *manager;

    if (NS_OK == GetListenerManager(&manager)) {
        manager->AddEventListenerByIID(aListener, aIID, NS_EVENT_FLAG_BUBBLE);
        NS_RELEASE(manager);
        return NS_OK;
    }
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsXULDocument::RemoveEventListenerByIID(nsIDOMEventListener *aListener, const nsIID& aIID)
{
    if (mListenerManager) {
        mListenerManager->RemoveEventListenerByIID(aListener, aIID, NS_EVENT_FLAG_BUBBLE);
        return NS_OK;
    }
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsXULDocument::AddEventListener(const nsString& aType, nsIDOMEventListener* aListener,
                                 PRBool aUseCapture)
{
  nsIEventListenerManager *manager;

  if (NS_OK == GetListenerManager(&manager)) {
    PRInt32 flags = aUseCapture ? NS_EVENT_FLAG_CAPTURE : NS_EVENT_FLAG_BUBBLE;

    manager->AddEventListenerByType(aListener, aType, flags);
    NS_RELEASE(manager);
    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsXULDocument::RemoveEventListener(const nsString& aType, nsIDOMEventListener* aListener,
                                    PRBool aUseCapture)
{
  if (mListenerManager) {
    PRInt32 flags = aUseCapture ? NS_EVENT_FLAG_CAPTURE : NS_EVENT_FLAG_BUBBLE;

    mListenerManager->RemoveEventListenerByType(aListener, aType, flags);
    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsXULDocument::GetListenerManager(nsIEventListenerManager** aResult)
{
    if (! mListenerManager) {
        nsresult rv;
        rv = nsComponentManager::CreateInstance(kEventListenerManagerCID,
                                                nsnull,
                                                NS_GET_IID(nsIEventListenerManager),
                                                getter_AddRefs(mListenerManager));

        if (NS_FAILED(rv)) return rv;
    }
    *aResult = mListenerManager;
    NS_ADDREF(*aResult);
    return NS_OK;
}

NS_IMETHODIMP
nsXULDocument::GetNewListenerManager(nsIEventListenerManager **aResult)
{
    return nsComponentManager::CreateInstance(kEventListenerManagerCID,
                                        nsnull,
                                        NS_GET_IID(nsIEventListenerManager),
                                        (void**) aResult);
}

nsresult
nsXULDocument::CaptureEvent(const nsString& aType)
{
  nsIEventListenerManager *mManager;

  if (NS_OK == GetListenerManager(&mManager)) {
    //mManager->CaptureEvent(aListener);
    NS_RELEASE(mManager);
    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}

nsresult
nsXULDocument::ReleaseEvent(const nsString& aType)
{
  if (mListenerManager) {
    //mListenerManager->ReleaseEvent(aListener);
    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}

nsresult
nsXULDocument::OpenWidgetItem(nsIContent* aElement)
{
    NS_PRECONDITION(aElement != nsnull, "null ptr");
    if (! aElement)
        return NS_ERROR_NULL_POINTER;

    if (! mBuilders)
        return NS_ERROR_NOT_INITIALIZED;

    nsresult rv;

#ifdef PR_LOGGING
    if (PR_LOG_TEST(gXULLog, PR_LOG_DEBUG)) {
        nsCOMPtr<nsIAtom> tag;
        rv = aElement->GetTag(*getter_AddRefs(tag));
        if (NS_FAILED(rv)) return rv;

        nsAutoString tagStr;
        tag->ToString(tagStr);

        PR_LOG(gXULLog, PR_LOG_DEBUG,
               ("xuldoc open-widget-item %s",
                (const char*) nsCAutoString(tagStr)));
    }
#endif

    PRUint32 cnt = 0;
    rv = mBuilders->Count(&cnt);
    NS_ASSERTION(NS_SUCCEEDED(rv), "Count failed");
    for (PRUint32 i = 0; i < cnt; ++i) {
        // XXX we should QueryInterface() here
        nsIRDFContentModelBuilder* builder
            = (nsIRDFContentModelBuilder*) mBuilders->ElementAt(i);

        NS_ASSERTION(builder != nsnull, "null ptr");
        if (! builder)
            continue;

        rv = builder->OpenContainer(aElement);
        NS_ASSERTION(NS_SUCCEEDED(rv), "error opening container");
        // XXX ignore error code?

        NS_RELEASE(builder);
    }

    return NS_OK;
}

nsresult
nsXULDocument::CloseWidgetItem(nsIContent* aElement)
{
    NS_PRECONDITION(aElement != nsnull, "null ptr");
    if (! aElement)
        return NS_ERROR_NULL_POINTER;

    if (! mBuilders)
        return NS_ERROR_NOT_INITIALIZED;

    nsresult rv;

#ifdef PR_LOGGING
    if (PR_LOG_TEST(gXULLog, PR_LOG_DEBUG)) {
        nsCOMPtr<nsIAtom> tag;
        rv = aElement->GetTag(*getter_AddRefs(tag));
        if (NS_FAILED(rv)) return rv;

        nsAutoString tagStr;
        tag->ToString(tagStr);

        PR_LOG(gXULLog, PR_LOG_DEBUG,
               ("xuldoc close-widget-item %s",
                (const char*) nsCAutoString(tagStr)));
    }
#endif

    PRUint32 cnt = 0;
    rv = mBuilders->Count(&cnt);
    NS_ASSERTION(NS_SUCCEEDED(rv), "Count failed");
    for (PRUint32 i = 0; i < cnt; ++i) {
        // XXX we should QueryInterface() here
        nsIRDFContentModelBuilder* builder
            = (nsIRDFContentModelBuilder*) mBuilders->ElementAt(i);

        NS_ASSERTION(builder != nsnull, "null ptr");
        if (! builder)
            continue;

        rv = builder->CloseContainer(aElement);
        NS_ASSERTION(NS_SUCCEEDED(rv), "error closing container");
        // XXX ignore error code?

        NS_RELEASE(builder);
    }

    return NS_OK;
}


nsresult
nsXULDocument::RebuildWidgetItem(nsIContent* aElement)
{
    NS_PRECONDITION(aElement != nsnull, "null ptr");
    if (! aElement)
        return NS_ERROR_NULL_POINTER;

    if (! mBuilders)
        return NS_ERROR_NOT_INITIALIZED;

    nsresult rv;

#ifdef PR_LOGGING
    if (PR_LOG_TEST(gXULLog, PR_LOG_DEBUG)) {
        nsCOMPtr<nsIAtom> tag;
        rv = aElement->GetTag(*getter_AddRefs(tag));
        if (NS_FAILED(rv)) return rv;

        nsAutoString tagStr;
        tag->ToString(tagStr);

        PR_LOG(gXULLog, PR_LOG_DEBUG,
               ("xuldoc close-widget-item %s",
                (const char*) nsCAutoString(tagStr)));
    }
#endif

    PRUint32 cnt = 0;
    rv = mBuilders->Count(&cnt);
    NS_ASSERTION(NS_SUCCEEDED(rv), "Count failed");
    for (PRUint32 i = 0; i < cnt; ++i) {
        // XXX we should QueryInterface() here
        nsIRDFContentModelBuilder* builder
            = (nsIRDFContentModelBuilder*) mBuilders->ElementAt(i);

        NS_ASSERTION(builder != nsnull, "null ptr");
        if (! builder)
            continue;

        rv = builder->RebuildContainer(aElement);
        NS_ASSERTION(NS_SUCCEEDED(rv), "error rebuilding container");
        // XXX ignore error code?

        NS_RELEASE(builder);
    }

    return NS_OK;
}


nsresult
nsXULDocument::CreateElement(PRInt32 aNameSpaceID,
                               nsIAtom* aTag,
                               nsIContent** aResult)
{
    nsresult rv;
    nsCOMPtr<nsIContent> result;

    if (aNameSpaceID == kNameSpaceID_HTML) {
        nsCOMPtr<nsIHTMLContent> element;
        const PRUnichar *tagName;
        aTag->GetUnicode(&tagName);

        rv = gHTMLElementFactory->CreateInstanceByTag(tagName, getter_AddRefs(element));
        if (NS_FAILED(rv)) return rv;

        result = do_QueryInterface(element);
        if (! result)
            return NS_ERROR_UNEXPECTED;
    }
    else {
        rv = nsXULElement::Create(aNameSpaceID, aTag, getter_AddRefs(result));
        if (NS_FAILED(rv)) return rv;
    }

    rv = result->SetDocument(this, PR_FALSE);
    NS_ASSERTION(NS_SUCCEEDED(rv), "unable to set element's document");
    if (NS_FAILED(rv)) return rv;

    *aResult = result;
    NS_ADDREF(*aResult);
    return NS_OK;
}


nsresult
nsXULDocument::PrepareToLoad(nsISupports* aContainer,
                             const char* aCommand,
                             nsIChannel* aChannel,
                             nsILoadGroup* aLoadGroup,
                             nsIParser** aResult)
{
    nsresult rv;

    // Make sure we're not called from dead code (LoadFromStream)
    NS_ENSURE_ARG_POINTER(aChannel);

    // Get the document's URL
    rv = aChannel->GetOriginalURI(getter_AddRefs(mDocumentURL));
    if (NS_FAILED(rv)) return rv;

    mDocumentTitle.Truncate();

    // Get the document's principal
    nsCOMPtr<nsISupports> owner;
    rv = aChannel->GetOwner(getter_AddRefs(owner));
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIPrincipal> principal = do_QueryInterface(owner);

    // Set the document's load group
    mDocumentLoadGroup = getter_AddRefs(NS_GetWeakReference(aLoadGroup));

    // Prepare the document's style sheets
    rv = PrepareStyleSheets(mDocumentURL);
    if (NS_FAILED(rv)) return rv;

    mCommand = aCommand;

    return PrepareToLoadPrototype(mDocumentURL, aCommand, principal, aResult);
}


nsresult
nsXULDocument::PrepareToLoadPrototype(nsIURI* aURI, const char* aCommand,
                                      nsIPrincipal* aDocumentPrincipal,
                                      nsIParser** aResult)
{
    nsresult rv;

    // Create a new prototype document
    rv = NS_NewXULPrototypeDocument(nsnull, NS_GET_IID(nsIXULPrototypeDocument), getter_AddRefs(mCurrentPrototype));
    if (NS_FAILED(rv)) return rv;

    // Bootstrap the master document prototype
    if (! mMasterPrototype) {
        mMasterPrototype = mCurrentPrototype;
        mMasterPrototype->SetDocumentPrincipal(aDocumentPrincipal);
    }

    rv = mCurrentPrototype->SetURI(aURI);
    if (NS_FAILED(rv)) return rv;

    // Create a XUL content sink, a parser, and kick off a load for
    // the overlay.
    nsCOMPtr<nsIXULContentSink> sink;
    rv = nsComponentManager::CreateInstance(kXULContentSinkCID,
                                            nsnull,
                                            NS_GET_IID(nsIXULContentSink),
                                            getter_AddRefs(sink));
    NS_ASSERTION(NS_SUCCEEDED(rv), "unable to create XUL content sink");
    if (NS_FAILED(rv)) return rv;

    rv = sink->Init(this, mCurrentPrototype);
    NS_ASSERTION(NS_SUCCEEDED(rv), "Unable to initialize datasource sink");
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIParser> parser;
    rv = nsComponentManager::CreateInstance(kParserCID,
                                            nsnull,
                                            kIParserIID,
                                            getter_AddRefs(parser));
    NS_ASSERTION(NS_SUCCEEDED(rv), "unable to create parser");
    if (NS_FAILED(rv)) return rv;

    parser->SetCommand(aCommand);

    nsAutoString utf8("UTF-8");
    parser->SetDocumentCharset(utf8, kCharsetFromDocTypeDefault);
    parser->SetContentSink(sink); // grabs a reference to the parser

    *aResult = parser;
    NS_ADDREF(*aResult);
    return NS_OK;
}


nsresult
nsXULDocument::ApplyPersistentAttributes()
{
    // Add all of the 'persisted' attributes into the content
    // model.
    if (! mLocalStore)
        return NS_OK;

    nsresult rv;
    nsCOMPtr<nsISupportsArray> array;

    nsXPIDLCString docurl;
    rv = mDocumentURL->GetSpec(getter_Copies(docurl));
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIRDFResource> doc;
    rv = gRDFService->GetResource(docurl, getter_AddRefs(doc));
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsISimpleEnumerator> elements;
    rv = mLocalStore->GetTargets(doc, kNC_persist, PR_TRUE, getter_AddRefs(elements));
    if (NS_FAILED(rv)) return rv;

    while (1) {
        PRBool hasmore;
        rv = elements->HasMoreElements(&hasmore);
        if (NS_FAILED(rv)) return rv;

        if (! hasmore)
            break;

        if (! array) {
            rv = NS_NewISupportsArray(getter_AddRefs(array));
            if (NS_FAILED(rv)) return rv;
        }

        nsCOMPtr<nsISupports> isupports;
        rv = elements->GetNext(getter_AddRefs(isupports));
        if (NS_FAILED(rv)) return rv;

        nsCOMPtr<nsIRDFResource> resource = do_QueryInterface(isupports);
        if (! resource) {
            NS_WARNING("expected element to be a resource");
            continue;
        }

        const char* uri;
        rv = resource->GetValueConst(&uri);
        if (NS_FAILED(rv)) return rv;

        nsAutoString id;
        rv = gXULUtils->MakeElementID(this, nsAutoString(uri), id);
        NS_ASSERTION(NS_SUCCEEDED(rv), "unable to compute element ID");
        if (NS_FAILED(rv)) return rv;

        rv = GetElementsForID(id, array);
        if (NS_FAILED(rv)) return rv;

        PRUint32 cnt;
        rv = array->Count(&cnt);
        if (NS_FAILED(rv)) return rv;

        if (! cnt)
            continue;

        rv = ApplyPersistentAttributesToElements(resource, array);
        if (NS_FAILED(rv)) return rv;
    }

    return NS_OK;
}


nsresult
nsXULDocument::ApplyPersistentAttributesToElements(nsIRDFResource* aResource, nsISupportsArray* aElements)
{
    nsresult rv;

    nsCOMPtr<nsISimpleEnumerator> attrs;
    rv = mLocalStore->ArcLabelsOut(aResource, getter_AddRefs(attrs));
    if (NS_FAILED(rv)) return rv;

    while (1) {
        PRBool hasmore;
        rv = attrs->HasMoreElements(&hasmore);
        if (NS_FAILED(rv)) return rv;

        if (! hasmore)
            break;

        nsCOMPtr<nsISupports> isupports;
        rv = attrs->GetNext(getter_AddRefs(isupports));
        if (NS_FAILED(rv)) return rv;

        nsCOMPtr<nsIRDFResource> property = do_QueryInterface(isupports);
        if (! property) {
            NS_WARNING("expected a resource");
            continue;
        }

        const char* attrname;
        rv = property->GetValueConst(&attrname);
        if (NS_FAILED(rv)) return rv;

        nsCOMPtr<nsIAtom> attr = dont_AddRef(NS_NewAtom(attrname));
        if (! attr)
            return NS_ERROR_OUT_OF_MEMORY;

        // XXX could hang namespace off here, as well...

        nsCOMPtr<nsIRDFNode> node;
        rv = mLocalStore->GetTarget(aResource, property, PR_TRUE, getter_AddRefs(node));
        if (NS_FAILED(rv)) return rv;

        nsCOMPtr<nsIRDFLiteral> literal = do_QueryInterface(node);
        if (! literal) {
            NS_WARNING("expected a literal");
            continue;
        }

        const PRUnichar* value;
        rv = literal->GetValueConst(&value);
        if (NS_FAILED(rv)) return rv;

        PRInt32 len = nsCRT::strlen(value);
        CBufDescriptor wrapper(value, PR_TRUE, len + 1, len);

        PRUint32 cnt;
        rv = aElements->Count(&cnt);
        if (NS_FAILED(rv)) return rv;

        for (PRInt32 i = PRInt32(cnt) - 1; i >= 0; --i) {
            nsISupports* isupports2 = aElements->ElementAt(i);
            if (! isupports2)
                continue;

            nsCOMPtr<nsIContent> element = do_QueryInterface(isupports2);
            NS_RELEASE(isupports2);

            rv = element->SetAttribute(/* XXX */ kNameSpaceID_None,
                                       attr,
                                       nsAutoString(wrapper),
                                       PR_FALSE);
        }
    }

    return NS_OK;
}

//----------------------------------------------------------------------
//
// nsXULDocument::ContextStack
//

nsXULDocument::ContextStack::ContextStack()
    : mTop(nsnull), mDepth(0)
{
}

nsXULDocument::ContextStack::~ContextStack()
{
    while (mTop) {
        Entry* doomed = mTop;
        mTop = mTop->mNext;
        NS_IF_RELEASE(doomed->mElement);
        delete doomed;
    }
}

nsresult
nsXULDocument::ContextStack::Push(nsXULPrototypeElement* aPrototype, nsIContent* aElement)
{
    Entry* entry = new Entry;
    if (! entry)
        return NS_ERROR_OUT_OF_MEMORY;

    entry->mPrototype = aPrototype;
    entry->mElement   = aElement;
    NS_IF_ADDREF(entry->mElement);
    entry->mIndex     = 0;

    entry->mNext = mTop;
    mTop = entry;

    ++mDepth;
    return NS_OK;
}

nsresult
nsXULDocument::ContextStack::Pop()
{
    if (mDepth == 0)
        return NS_ERROR_UNEXPECTED;

    Entry* doomed = mTop;
    mTop = mTop->mNext;
    --mDepth;

    NS_IF_RELEASE(doomed->mElement);
    delete doomed;
    return NS_OK;
}

nsresult
nsXULDocument::ContextStack::Peek(nsXULPrototypeElement** aPrototype,
                                           nsIContent** aElement,
                                           PRInt32* aIndex)
{
    if (mDepth == 0)
        return NS_ERROR_UNEXPECTED;

    *aPrototype = mTop->mPrototype;
    *aElement   = mTop->mElement;
    NS_IF_ADDREF(*aElement);
    *aIndex     = mTop->mIndex;

    return NS_OK;
}


nsresult
nsXULDocument::ContextStack::SetTopIndex(PRInt32 aIndex)
{
    if (mDepth == 0)
        return NS_ERROR_UNEXPECTED;

    mTop->mIndex = aIndex;
    return NS_OK;
}


PRBool
nsXULDocument::ContextStack::IsInsideXULTemplate()
{
    if (mDepth) {
        nsCOMPtr<nsIContent> element = dont_QueryInterface(mTop->mElement);
        while (element) {
            PRInt32 nameSpaceID;
            element->GetNameSpaceID(nameSpaceID);
            if (nameSpaceID == kNameSpaceID_XUL) {
                nsCOMPtr<nsIAtom> tag;
                element->GetTag(*getter_AddRefs(tag));
                if (tag.get() == kTemplateAtom) {
                    return PR_TRUE;
                }
            }

            nsCOMPtr<nsIContent> parent;
            element->GetParent(*getter_AddRefs(parent));
            element = parent;
        }
    }
    return PR_FALSE;
}


//----------------------------------------------------------------------
//
// Content model walking routines
//

nsresult
nsXULDocument::PrepareToWalk()
{
    // Prepare to walk the mCurrentPrototype
    nsresult rv;

    // Keep an owning reference to the prototype document so that its
    // elements aren't yanked from beneath us.
    mPrototypes->AppendElement(mCurrentPrototype);

    // Push the overlay references onto our overlay processing
    // stack. GetOverlayReferences() will return an ordered array of
    // overlay references...
    nsCOMPtr<nsISupportsArray> overlays;
    rv = mCurrentPrototype->GetOverlayReferences(getter_AddRefs(overlays));
    if (NS_FAILED(rv)) return rv;

    // ...and we preserve this ordering by appending to our
    // mUnloadedOverlays array in reverse order
    PRUint32 count;
    overlays->Count(&count);
    for (PRInt32 i = count - 1; i >= 0; --i) {
        nsISupports* isupports = overlays->ElementAt(i);
        mUnloadedOverlays->AppendElement(isupports);
        NS_IF_RELEASE(isupports);
    }


    // Now check the chrome registry for any additional overlays.
    rv = AddChromeOverlays();

    // Get the prototype's root element and initialize the context
    // stack for the prototype walk.
    nsXULPrototypeElement* proto;
    rv = mCurrentPrototype->GetRootElement(&proto);
    if (NS_FAILED(rv)) return rv;


    if (! proto) {
#ifdef PR_LOGGING
        nsCOMPtr<nsIURI> url;
        rv = mCurrentPrototype->GetURI(getter_AddRefs(url));
        if (NS_FAILED(rv)) return rv;

        nsXPIDLCString urlspec;
        rv = url->GetSpec(getter_Copies(urlspec));
        if (NS_FAILED(rv)) return rv;

        PR_LOG(gXULLog, PR_LOG_ALWAYS,
               ("xul: error parsing '%s'", (const char*) urlspec));
#endif

        return NS_OK;
    }

    // Do one-time initialization if we're preparing to walk the
    // master document's prototype.
    nsCOMPtr<nsIContent> root;

    if (mState == eState_Master) {
        rv = CreateElement(proto, getter_AddRefs(root));
        if (NS_FAILED(rv)) return rv;

        SetRootContent(root);

        // Create the document's "hidden form" element which will wrap all
        // HTML form elements that turn up.
        nsCOMPtr<nsIHTMLContent> form;
        rv = gHTMLElementFactory->CreateInstanceByTag(nsAutoString("form"),
                                                      getter_AddRefs(form));
        NS_ASSERTION(NS_SUCCEEDED(rv), "unable to create form element");
        if (NS_FAILED(rv)) return rv;

        nsCOMPtr<nsIDOMHTMLFormElement> htmlFormElement = do_QueryInterface(form);
        NS_ASSERTION(htmlFormElement != nsnull, "not an nSIDOMHTMLFormElement");
        if (! htmlFormElement)
            return NS_ERROR_UNEXPECTED;

        rv = SetForm(htmlFormElement);
        NS_ASSERTION(NS_SUCCEEDED(rv), "unable to set form element");
        if (NS_FAILED(rv)) return NS_ERROR_UNEXPECTED;

        nsCOMPtr<nsIContent> content = do_QueryInterface(form);
        NS_ASSERTION(content != nsnull, "not an nsIContent");
        if (! content)
            return NS_ERROR_UNEXPECTED;

        // XXX Would like to make this anonymous, but still need the
        // form's frame to get built. For now make it explicit.
        rv = root->InsertChildAt(content, 0, PR_FALSE);
        NS_ASSERTION(NS_SUCCEEDED(rv), "unable to add anonymous form element");
        if (NS_FAILED(rv)) return rv;

        // Add the root element to the XUL document's ID-to-element map.
        rv = AddElementToMap(root);
        if (NS_FAILED(rv)) return rv;

        // Add a dummy channel to the load group as a placeholder for the document
        // load
        rv = PlaceholderChannel::Create(getter_AddRefs(mPlaceholderChannel));
        if (NS_FAILED(rv)) return rv;

        nsCOMPtr<nsILoadGroup> group = do_QueryReferent(mDocumentLoadGroup);
        NS_ASSERTION(group != nsnull, "no load group");

        if (group) {
            rv = mPlaceholderChannel->SetLoadGroup(group);
            if (NS_FAILED(rv)) return rv;

            rv = group->AddChannel(mPlaceholderChannel, nsnull);
            if (NS_FAILED(rv)) return rv;
        }
    }

    rv = mContextStack.Push(proto, root);
    if (NS_FAILED(rv)) return rv;

    return NS_OK;
}


nsresult
nsXULDocument::AddChromeOverlays()
{
    nsresult rv;
    NS_WITH_SERVICE(nsIChromeRegistry, reg, kChromeRegistryCID, &rv);

    if (NS_FAILED(rv))
        return NS_ERROR_FAILURE;

    nsCOMPtr<nsISimpleEnumerator> oe;

    {
        nsCOMPtr<nsIURI> uri;
        rv = mCurrentPrototype->GetURI(getter_AddRefs(uri));
        if (NS_FAILED(rv)) return rv;

        reg->GetOverlays(uri, getter_AddRefs(oe));
    }

    if (!oe)
        return NS_OK;

    PRBool moreElements;
    oe->HasMoreElements(&moreElements);

    while (moreElements) {
        nsCOMPtr<nsISupports> next;
        oe->GetNext(getter_AddRefs(next));
        if (!next)
            return NS_OK;

        nsCOMPtr<nsIURI> uri = do_QueryInterface(next);
        if (!uri)
            return NS_OK;

        mUnloadedOverlays->AppendElement(uri);

        oe->HasMoreElements(&moreElements);
    }

    return NS_OK;
}


nsresult
nsXULDocument::ResumeWalk()
{
    // Walk the prototype and build the delegate content model. The
    // walk is performed in a top-down, left-to-right fashion. That
    // is, a parent is built before any of its children; a node is
    // only built after all of its siblings to the left are fully
    // constructed.
    //
    // It is interruptable so that transcluded documents (e.g.,
    // <html:script src="..." />) can be properly re-loaded if the
    // cached copy of the document becomes stale.
    nsresult rv;

    while (1) {
        // Begin (or resume) walking the current prototype.

        while (mContextStack.Depth() > 0) {
            // Look at the top of the stack to determine what we're
            // currently working on.
            nsXULPrototypeElement* proto;
            nsCOMPtr<nsIContent> element;
            PRInt32 indx;
            rv = mContextStack.Peek(&proto, getter_AddRefs(element), &indx);
            if (NS_FAILED(rv)) return rv;

            if (indx >= proto->mNumChildren) {
                if (element && ((mState == eState_Master) || (mContextStack.Depth() > 1))) {
                    // We've processed all of the prototype's children.
                    // Check the element for a 'datasources' attribute, in
                    // which case we'll need to create a template builder
                    // and construct the first 'ply' of elements beneath
                    // it.
                    //
                    // N.B. that we do this -after- all other XUL children
                    // have been created: this ensures that there'll be a
                    // <template> tag available when we try to build that
                    // first ply of generated elements.
                    //
                    // N.B. that we do -not- do this if we are dealing
                    // with an 'overlay element'; that is, an element
                    // in the first ply of an overlay
                    // document. OverlayForwardReference::Merge() will
                    // handle that case.
                    rv = CheckTemplateBuilder(element);
                    if (NS_FAILED(rv)) return rv;
                }

                // Now pop the context stack back up to the parent
                // element and continue the prototype walk.
                mContextStack.Pop();
                continue;
            }

            // Grab the next child, and advance the current context stack
            // to the next sibling to our right.
            nsXULPrototypeNode* childproto = proto->mChildren[indx];
            mContextStack.SetTopIndex(++indx);

            switch (childproto->mType) {
            case nsXULPrototypeNode::eType_Element: {
                // An 'element', which may contain more content.
                nsXULPrototypeElement* protoele =
                    NS_REINTERPRET_CAST(nsXULPrototypeElement*, childproto);

                nsCOMPtr<nsIContent> child;

                if ((mState == eState_Master) || (mContextStack.Depth() > 1)) {
                    // We're in the master document -or -we're in an
                    // overlay, and far enough down into the overlay's
                    // content that we can simply build the delegates
                    // and attach them to the parent node.
                    rv = CreateElement(protoele, getter_AddRefs(child));
                    if (NS_FAILED(rv)) return rv;

                    // ...and append it to the content model.
                    rv = element->AppendChildTo(child, PR_FALSE);
                    if (NS_FAILED(rv)) return rv;

                    // ...but only do document-level hookup if we're
                    // in the master document. For an overlay, this
                    // will happend when the overlay is successfully
                    // resolved.
                    if (mState == eState_Master) {
                        rv = AddSubtreeToDocument(child);
                        if (NS_FAILED(rv)) return rv;
                    }
                }
                else {
                    // We're in the "first ply" of an overlay: the
                    // "hookup" nodes. Create an 'overlay' element so
                    // that we can continue to build content, and
                    // enter a forward reference so we can hook it up
                    // later.
                    rv = CreateOverlayElement(protoele, getter_AddRefs(child));
                    if (NS_FAILED(rv)) return rv;
                }

                // If it has children, push the element onto the context
                // stack and begin to process them.
                if (protoele->mNumChildren > 0) {
                    rv = mContextStack.Push(protoele, child);
                    if (NS_FAILED(rv)) return rv;
                }
            }
            break;

            case nsXULPrototypeNode::eType_Script: {
                // A script reference. Execute the script immediately;
                // this may have side effects in the content model.
                nsXULPrototypeScript* scriptproto =
                    NS_REINTERPRET_CAST(nsXULPrototypeScript*, childproto);

                if (scriptproto->mSrcURI) {
                    // A transcluded script reference; this may
                    // "block" our prototype walk if the script isn't
                    // cached, or the cached copy of the script is
                    // stale and must be reloaded.
                    PRBool blocked;
                    rv = LoadScript(scriptproto, &blocked);
                    if (NS_FAILED(rv)) return rv;

                    if (blocked)
                        return NS_OK;
                }
                else if (scriptproto->mScriptObject) {
                    // An inline script
                    rv = ExecuteScript(scriptproto->mScriptObject);
                    if (NS_FAILED(rv)) return rv;
                }
            }
            break;

            case nsXULPrototypeNode::eType_Text: {
                // A simple text node.
                nsCOMPtr<nsITextContent> text;
                rv = nsComponentManager::CreateInstance(kTextNodeCID,
                                                        nsnull,
                                                        NS_GET_IID(nsITextContent),
                                                        getter_AddRefs(text));
                if (NS_FAILED(rv)) return rv;

                nsXULPrototypeText* textproto =
                    NS_REINTERPRET_CAST(nsXULPrototypeText*, childproto);

                rv = text->SetText(textproto->mValue.GetUnicode(),
                                   textproto->mValue.Length(),
                                   PR_FALSE);

                if (NS_FAILED(rv)) return rv;

                nsCOMPtr<nsIContent> child = do_QueryInterface(text);
                if (! child)
                    return NS_ERROR_UNEXPECTED;

                rv = element->AppendChildTo(child, PR_FALSE);
                if (NS_FAILED(rv)) return rv;
            }
            break;

            }
        }

        // Once we get here, the context stack will have been
        // depleted. That means that the entire prototype has been
        // walked and content has been constructed.

        // If we're not already, mark us as now processing overlays.
        mState = eState_Overlay;

        PRUint32 count;
        mUnloadedOverlays->Count(&count);

        // If there are no overlay URIs, then we're done.
        if (! count)
            break;

        nsCOMPtr<nsIURI> uri =
            dont_AddRef(NS_REINTERPRET_CAST(nsIURI*, mUnloadedOverlays->ElementAt(count - 1)));

        mUnloadedOverlays->RemoveElementAt(count - 1);

#ifdef PR_LOGGING
        if (PR_LOG_TEST(gXULLog, PR_LOG_DEBUG)) {
            nsXPIDLCString urlspec;
            uri->GetSpec(getter_Copies(urlspec));

            PR_LOG(gXULLog, PR_LOG_DEBUG,
                   ("xul: loading overlay %s", (const char*) urlspec));
        }
#endif

        // Look in the prototype cache for the prototype document with
        // the specified URI.
        rv = gXULCache->GetPrototype(uri, getter_AddRefs(mCurrentPrototype));
        if (NS_FAILED(rv)) return rv;

        if (gXULUtils->UseXULCache() && mCurrentPrototype) {
            // Found the overlay's prototype in the cache.
            rv = AddPrototypeSheets();
            if (NS_FAILED(rv)) return rv;

            // Now prepare to walk the prototype to create its content
            rv = PrepareToWalk();
            if (NS_FAILED(rv)) return rv;

            PR_LOG(gXULLog, PR_LOG_DEBUG, ("xul: overlay was cached"));
        }
        else {
            // Not there. Initiate an asynchronous load.
            PR_LOG(gXULLog, PR_LOG_DEBUG, ("xul: overlay was not cached"));

            nsCOMPtr<nsIParser> parser;
            rv = PrepareToLoadPrototype(uri, "view", nsnull, getter_AddRefs(parser));
            if (NS_FAILED(rv)) return rv;

            nsCOMPtr<nsIStreamListener> listener = do_QueryInterface(parser);
            if (! listener)
                return NS_ERROR_UNEXPECTED;

            parser->Parse(uri);

            nsCOMPtr<nsILoadGroup> group = do_QueryReferent(mDocumentLoadGroup);
            rv = NS_OpenURI(listener, nsnull, uri, group);
            if (NS_FAILED(rv)) return rv;

            // Return to the main event loop and eagerly await the
            // overlay load's completion. When the content sink
            // completes, it will trigger an EndLoad(), which'll wind
            // us back up here, in ResumeWalk().
            return NS_OK;
        }
    }

    // If we get here, there is nothing left for us to walk. The content
    // model is built and ready for layout.
    rv = ResolveForwardReferences();
    if (NS_FAILED(rv)) return rv;

    rv = ApplyPersistentAttributes();
    if (NS_FAILED(rv)) return rv;

#if defined(DEBUG_waterson) || defined(DEBUG_hyatt)
    {
        nsTime finish = PR_Now();
        nsInt64 diff64 = finish - mLoadStart;
        PRInt32 diff = PRInt32(diff64 / nsInt64(1000));
        printf("***** XUL document loaded in %dmsec\n", diff);
    }

    {
        nsInt64 now(PR_Now());
        now /= nsInt64(1000);
        printf("##### XUL document loaded at %d\n", PRInt32(now));
    }
#endif

    StartLayout();

    for (PRInt32 i = 0; i < mObservers.Count(); i++) {
        nsIDocumentObserver* observer = (nsIDocumentObserver*) mObservers[i];
        observer->EndLoad(this);
        if (observer != (nsIDocumentObserver*)mObservers.ElementAt(i)) {
          i--;
        }
    }

#if defined(DEBUG_waterson) || defined(DEBUG_hyatt)
    {
        nsTime finish = PR_Now();
        nsInt64 diff64 = finish - mLoadStart;
        PRInt32 diff = PRInt32(diff64 / nsInt64(1000));
        printf("***** XUL document flowed in %dmsec\n", diff);
    }

    {
        nsInt64 now(PR_Now());
        now /= nsInt64(1000);
        printf("##### XUL document flowed at %d\n", PRInt32(now));
    }
#endif

    // Remove the placeholder channel; if we're the last channel in the
    // load group, this will fire the OnEndDocumentLoad() method in the
    // webshell, and run the onload handlers, etc.
    nsCOMPtr<nsILoadGroup> group = do_QueryReferent(mDocumentLoadGroup);
    if (group) {
        rv = group->RemoveChannel(mPlaceholderChannel, nsnull, NS_OK, nsnull);
        if (NS_FAILED(rv)) return rv;

        mPlaceholderChannel = nsnull;
    }

    return rv;
}


nsresult
nsXULDocument::LoadScript(nsXULPrototypeScript* aScriptProto, PRBool* aBlock)
{
    // Load a transcluded script
    nsresult rv;

    if (aScriptProto->mScriptObject) {
        rv = ExecuteScript(aScriptProto->mScriptObject);

        // Ignore return value from execution, and don't block
        *aBlock = PR_FALSE;
    }
    else {
        // Set the current script prototype so that OnUnicharStreamComplete
        // can get report the right file if there are errors in the script.
        NS_ASSERTION(!mCurrentScriptProto,
                     "still loading a script when starting another load?");
        mCurrentScriptProto = aScriptProto;

        if (aScriptProto->mSrcLoading) {
            // Another XULDocument load has started, which is still in progress.
            // Remember to ResumeWalk this document when the load completes.
            mNextSrcLoadWaiter = aScriptProto->mSrcLoadWaiters;
            aScriptProto->mSrcLoadWaiters = this;
            NS_ADDREF_THIS();
        }
        else {
            nsCOMPtr<nsILoadGroup> group = do_QueryReferent(mDocumentLoadGroup);

            // N.B., the loader will be released in OnUnicharStreamComplete
            nsIUnicharStreamLoader* loader;
            rv = NS_NewUnicharStreamLoader(&loader, aScriptProto->mSrcURI, this, group);
            if (NS_FAILED(rv)) return rv;

            aScriptProto->mSrcLoading = PR_TRUE;
        }

        // Block until OnUnicharStreamComplete resumes us.
        *aBlock = PR_TRUE;
    }
    return NS_OK;
}


NS_IMETHODIMP
nsXULDocument::OnUnicharStreamComplete(nsIUnicharStreamLoader* aLoader,
                                       nsresult aStatus,
                                       PRUint32 stringLen,
                                       const PRUnichar* string)
{
    // This is the completion routine that will be called when a
    // transcluded script completes. Compile and execute the script
    // if the load was successful, then continue building content
    // from the prototype.
    nsresult rv;

    NS_ASSERTION(mCurrentScriptProto && mCurrentScriptProto->mSrcLoading,
                 "script source not loading on unichar stream complete?");

    // Clear mCurrentScriptProto now, but save it first for use below in
    // the compile/execute code, and in the while loop that resumes walks
    // of other documents that raced to load this script
    nsXULPrototypeScript* scriptProto = mCurrentScriptProto;
    mCurrentScriptProto = nsnull;

    // Clear the prototype's loading flag before executing the script or
    // resuming document walks, in case any of those control flows starts a
    // new script load.
    scriptProto->mSrcLoading = PR_FALSE;

    if (NS_SUCCEEDED(aStatus)) {
        rv = scriptProto->Compile(string, stringLen,
                                  scriptProto->mSrcURI, 1,
                                  this);
        aStatus = rv;
        if (NS_SUCCEEDED(rv)) {
            rv = ExecuteScript(scriptProto->mScriptObject);
        }
        // ignore any evaluation errors
    }

    // balance the addref we added in LoadScript()
    NS_RELEASE(aLoader);

    rv = ResumeWalk();

    // Load a pointer to the prototype-script's list of nsXULDocuments who
    // raced to load the same script
    nsXULDocument** docp = &scriptProto->mSrcLoadWaiters;

    // Resume walking other documents that waited for this one's load, first
    // executing the script we just compiled, in each doc's script context
    nsXULDocument* doc;
    while ((doc = *docp) != nsnull) {
        NS_ASSERTION(doc->mCurrentScriptProto == scriptProto,
                     "waiting for wrong script to load?");
        doc->mCurrentScriptProto = nsnull;

        // Unlink doc from scriptProto's list before executing and resuming
        *docp = doc->mNextSrcLoadWaiter;
        doc->mNextSrcLoadWaiter = nsnull;

        // Execute only if we loaded and compiled successfully, then resume
        if (NS_SUCCEEDED(aStatus)) {
            doc->ExecuteScript(scriptProto->mScriptObject);
        }
        doc->ResumeWalk();
        NS_RELEASE(doc);
    }

    return rv;
}


nsresult
nsXULDocument::ExecuteScript(JSObject* aScriptObject)
{
    // Execute the precompiled script with the given version
    nsresult rv;

    nsCOMPtr<nsIScriptContextOwner> owner = dont_AddRef(GetScriptContextOwner());
    NS_ASSERTION(owner != nsnull, "document has no script context owner");
    if (! owner)
        return NS_ERROR_UNEXPECTED;

    nsCOMPtr<nsIScriptContext> context;
    rv = owner->GetScriptContext(getter_AddRefs(context));
    if (NS_FAILED(rv)) return rv;

    rv = context->ExecuteScript(aScriptObject, nsnull, nsnull, nsnull);
    return rv;
}


nsresult
nsXULDocument::CreateElement(nsXULPrototypeElement* aPrototype, nsIContent** aResult)
{
    // Create a content model element from a prototype element.
    NS_PRECONDITION(aPrototype != nsnull, "null ptr");
    if (! aPrototype)
        return NS_ERROR_NULL_POINTER;

    nsresult rv;

#ifdef PR_LOGGING
    if (PR_LOG_TEST(gXULLog, PR_LOG_ALWAYS)) {
        nsAutoString tagstr;
        aPrototype->mTag->ToString(tagstr);

        PR_LOG(gXULLog, PR_LOG_ALWAYS,
               ("xul: creating <%s> from prototype",
                (const char*) nsCAutoString(tagstr)));
    }
#endif

    nsCOMPtr<nsIContent> result;

    if (aPrototype->mNameSpaceID == kNameSpaceID_HTML) {
        // If it's an HTML element, it's gonna be heavyweight no matter
        // what. So we need to copy everything out of the prototype
        // into the element.
        nsAutoString tagStr;
        rv = aPrototype->mTag->ToString(tagStr);
        if (NS_FAILED(rv)) return rv;

        nsCOMPtr<nsIHTMLContent> element;
        gHTMLElementFactory->CreateInstanceByTag(tagStr.GetUnicode(), getter_AddRefs(element));
        if (NS_FAILED(rv)) return rv;

        result = do_QueryInterface(element);
        if (! result)
            return NS_ERROR_UNEXPECTED;

        rv = result->SetDocument(this, PR_FALSE);
        if (NS_FAILED(rv)) return rv;

        rv = AddAttributes(aPrototype, result);
        if (NS_FAILED(rv)) return rv;

        nsCOMPtr<nsIFormControl> htmlformctrl = do_QueryInterface(element);
        if (htmlformctrl) {
            nsCOMPtr<nsIDOMHTMLFormElement> docform;
            rv = GetForm(getter_AddRefs(docform));
            if (NS_FAILED(rv)) return rv;

            NS_ASSERTION(docform != nsnull, "no document form");
            if (! docform)
                return NS_ERROR_UNEXPECTED;

            htmlformctrl->SetForm(docform);
        }
    }
    else {
        // If it's a XUL element, it'll be lightweight until somebody
        // monkeys with it.
        rv = nsXULElement::Create(aPrototype, this, PR_TRUE, getter_AddRefs(result));
        if (NS_FAILED(rv)) return rv;

        // We also need to pay special attention to the keyset tag to set up a listener
        if ((aPrototype->mNameSpaceID == kNameSpaceID_XUL) &&
            (aPrototype->mTag.get() == kKeysetAtom)) {
            // Create our nsXULKeyListener and hook it up.
            nsCOMPtr<nsIXULKeyListener> keyListener;
            rv = nsComponentManager::CreateInstance(kXULKeyListenerCID,
                                                    nsnull,
                                                    NS_GET_IID(nsIXULKeyListener),
                                                    getter_AddRefs(keyListener));
            NS_ASSERTION(NS_SUCCEEDED(rv), "unable to create a key listener");
            if (NS_FAILED(rv)) return rv;

            nsCOMPtr<nsIDOMEventListener> domEventListener = do_QueryInterface(keyListener);
            if (domEventListener) {
                // Init the listener with the keyset node
                nsCOMPtr<nsIDOMElement> domElement = do_QueryInterface(result);
                keyListener->Init(domElement, this);

                AddEventListener("keypress", domEventListener, PR_TRUE);
                AddEventListener("keydown",  domEventListener, PR_TRUE);
                AddEventListener("keyup",    domEventListener, PR_TRUE);
            }
        }
    }

    *aResult = result;
    NS_ADDREF(*aResult);
    return NS_OK;
}

nsresult
nsXULDocument::CreateOverlayElement(nsXULPrototypeElement* aPrototype, nsIContent** aResult)
{
    nsresult rv;

    // This doesn't really do anything except create a placeholder
    // element. I'd use an XML element, but it gets its knickers in a
    // knot with DOM ranges when you try to remove its children.
    nsCOMPtr<nsIContent> element;
    rv = nsXULElement::Create(aPrototype, this, PR_FALSE, getter_AddRefs(element));
    if (NS_FAILED(rv)) return rv;

    OverlayForwardReference* fwdref = new OverlayForwardReference(this, element);
    if (! fwdref)
        return NS_ERROR_OUT_OF_MEMORY;

    // transferring ownership to ya...
    rv = AddForwardReference(fwdref);
    if (NS_FAILED(rv)) return rv;

    *aResult = element;
    NS_ADDREF(*aResult);
    return NS_OK;
}


nsresult
nsXULDocument::AddAttributes(nsXULPrototypeElement* aPrototype, nsIContent* aElement)
{
    nsresult rv;

    for (PRInt32 i = 0; i < aPrototype->mNumAttributes; ++i) {
        nsXULPrototypeAttribute* protoattr = &(aPrototype->mAttributes[i]);

        rv = aElement->SetAttribute(protoattr->mNameSpaceID,
                                    protoattr->mName,
                                    protoattr->mValue,
                                    PR_FALSE);
        if (NS_FAILED(rv)) return rv;
    }

    return NS_OK;
}


nsresult
nsXULDocument::CheckTemplateBuilder(nsIContent* aElement)
{
    // Check aElement for a 'datasources' attribute, and if it has
    // one, create and initialize a template builder.
    nsresult rv;

    nsAutoString datasources;
    rv = aElement->GetAttribute(kNameSpaceID_None, kDataSourcesAtom, datasources);
    if (NS_FAILED(rv)) return rv;

    if (rv != NS_CONTENT_ATTR_HAS_VALUE)
        return NS_OK;

    // Get the document and its URL
    nsCOMPtr<nsIDocument> doc;
    rv = aElement->GetDocument(*getter_AddRefs(doc));
    if (NS_FAILED(rv)) return rv;

    NS_ASSERTION(doc != nsnull, "no document");
    if (! doc)
        return NS_ERROR_UNEXPECTED;

    nsCOMPtr<nsIURI> docurl = dont_AddRef(doc->GetDocumentURL());

    // construct a new builder
    nsCOMPtr<nsIRDFContentModelBuilder> builder;
    rv = nsComponentManager::CreateInstance(kXULTemplateBuilderCID,
                                            nsnull,
                                            NS_GET_IID(nsIRDFContentModelBuilder),
                                            getter_AddRefs(builder));
    NS_ASSERTION(NS_SUCCEEDED(rv), "unable to create tree content model builder");
    if (NS_FAILED(rv)) return rv;

    rv = builder->SetRootContent(aElement);
    NS_ASSERTION(NS_SUCCEEDED(rv), "unable to set builder's root content element");
    if (NS_FAILED(rv)) return rv;

    // create a database for the builder
    nsCOMPtr<nsIRDFCompositeDataSource> db;
    rv = nsComponentManager::CreateInstance(kRDFCompositeDataSourceCID,
                                            nsnull,
                                            NS_GET_IID(nsIRDFCompositeDataSource),
                                            getter_AddRefs(db));

    NS_ASSERTION(NS_SUCCEEDED(rv), "unable to construct new composite data source");
    if (NS_FAILED(rv)) return rv;

    // Add the local store as the first data source in the db. Note
    // that we _might_ not be able to get a local store if we haven't
    // got a profile to read from yet.
    nsCOMPtr<nsIRDFDataSource> localstore;
    rv = gRDFService->GetDataSource("rdf:local-store", getter_AddRefs(localstore));
    if (NS_SUCCEEDED(rv)) {
        rv = db->AddDataSource(localstore);
        NS_ASSERTION(NS_SUCCEEDED(rv), "unable to add local store to db");
        if (NS_FAILED(rv)) return rv;
    }

    // Parse datasources: they are assumed to be a whitespace
    // separated list of URIs; e.g.,
    //
    //     rdf:bookmarks rdf:history http://foo.bar.com/blah.cgi?baz=9
    //
    PRInt32 first = 0;

    while(1) {
        while (first < datasources.Length() && nsString::IsSpace(datasources.CharAt(first)))
            ++first;

        if (first >= datasources.Length())
            break;

        PRInt32 last = first;
        while (last < datasources.Length() && !nsString::IsSpace(datasources.CharAt(last)))
            ++last;

        nsAutoString uri;
        datasources.Mid(uri, first, last - first);
        first = last + 1;

        // A special 'dummy' datasource
        if (uri.Equals("rdf:null"))
            continue;

        rv = rdf_MakeAbsoluteURI(docurl, uri);
        if (NS_FAILED(rv)) return rv;

        nsCOMPtr<nsIRDFDataSource> ds;
        rv = gRDFService->GetDataSource(nsCAutoString(uri), getter_AddRefs(ds));

        if (NS_FAILED(rv)) {
            // This is only a warning because the data source may not
            // be accessable for any number of reasons, including
            // security, a bad URL, etc.
#ifdef DEBUG
            nsCAutoString msg;
            msg += "unable to load datasource '";
            msg += nsCAutoString(uri);
            msg += '\'';
            NS_WARNING((const char*) msg);
#endif
            continue;
        }

        rv = db->AddDataSource(ds);
        NS_ASSERTION(NS_SUCCEEDED(rv), "unable to add datasource to composite data source");
        if (NS_FAILED(rv)) return rv;
    }

    // add it to the set of builders in use by the document
    nsCOMPtr<nsIXULDocument> xuldoc = do_QueryInterface(doc);
    if (! xuldoc)
        return NS_ERROR_UNEXPECTED;

    rv = xuldoc->AddContentModelBuilder(builder);
    NS_ASSERTION(NS_SUCCEEDED(rv), "unable to add builder to the document");
    if (NS_FAILED(rv)) return rv;

    rv = builder->SetDataBase(db);
    NS_ASSERTION(NS_SUCCEEDED(rv), "unable to set builder's database");
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIXULContent> xulcontent = do_QueryInterface(aElement);
    if (xulcontent) {
        // Mark the XUL element as being lazy, so the template builder
        // will run when layout first asks for these nodes.
        //
        //rv = xulcontent->ClearLazyState(eTemplateContentsBuilt | eContainerContentsBuilt);
        //if (NS_FAILED(rv)) return rv;

        xulcontent->SetLazyState(nsIXULContent::eChildrenMustBeRebuilt);
        if (NS_FAILED(rv)) return rv;
    }
    else {
        // Force construction of immediate template sub-content _now_.
        rv = builder->CreateContents(aElement);
        NS_ASSERTION(NS_SUCCEEDED(rv), "unable to create template contents");
        if (NS_FAILED(rv)) return rv;
    }

    return NS_OK;
}


nsresult
nsXULDocument::AddPrototypeSheets()
{
    // Add mCurrentPrototype's style sheets to the document.
    nsresult rv;

    nsCOMPtr<nsISupportsArray> sheets;
    rv = mCurrentPrototype->GetStyleSheetReferences(getter_AddRefs(sheets));
    if (NS_FAILED(rv)) return rv;

    PRUint32 count;
    sheets->Count(&count);
    for (PRUint32 i = 0; i < count; ++i) {
        nsISupports* isupports = sheets->ElementAt(i);
        nsCOMPtr<nsIURI> uri = do_QueryInterface(isupports);
        NS_IF_RELEASE(isupports);

        NS_ASSERTION(uri != nsnull, "not a URI!!!");
        if (! uri)
            return NS_ERROR_UNEXPECTED;

        nsCOMPtr<nsICSSStyleSheet> sheet;
        rv = gXULCache->GetStyleSheet(uri, getter_AddRefs(sheet));
        if (NS_FAILED(rv)) return rv;

        NS_ASSERTION(sheet != nsnull, "uh oh, sheet wasn't in the cache. go reload it");
        if (! sheet)
            return NS_ERROR_UNEXPECTED;

        nsCOMPtr<nsICSSStyleSheet> newsheet;
        rv = sheet->Clone(*getter_AddRefs(newsheet));
        if (NS_FAILED(rv)) return rv;

        AddStyleSheet(newsheet);
    }

    return NS_OK;
}


//----------------------------------------------------------------------
//
// nsXULDocument::OverlayForwardReference
//

nsForwardReference::Result
nsXULDocument::OverlayForwardReference::Resolve()
{
    // Resolve a forward reference from an overlay element; attempt to
    // hook it up into the main document.
    nsresult rv;

    nsAutoString id;
    rv = mOverlay->GetAttribute(kNameSpaceID_None, kIdAtom, id);
    if (NS_FAILED(rv)) return eResolve_Error;

    nsCOMPtr<nsIDOMElement> domtarget;
    rv = mDocument->GetElementById(id, getter_AddRefs(domtarget));
    if (NS_FAILED(rv)) return eResolve_Error;

    // If we can't find the element in the document, defer the hookup
    // until later.
    if (! domtarget)
        return eResolve_Later;

    nsCOMPtr<nsIContent> target = do_QueryInterface(domtarget);
    NS_ASSERTION(target != nsnull, "not an nsIContent");
    if (! target)
        return eResolve_Error;

    rv = Merge(target, mOverlay);
    if (NS_FAILED(rv)) return eResolve_Error;

    PR_LOG(gXULLog, PR_LOG_ALWAYS,
           ("xul: overlay resolved '%s'",
            (const char*) nsCAutoString(id)));

    mResolved = PR_TRUE;
    return eResolve_Succeeded;
}



nsresult
nsXULDocument::OverlayForwardReference::Merge(nsIContent* aTargetNode,
                                              nsIContent* aOverlayNode)
{
    nsresult rv;

    {
        // Whack the attributes from aOverlayNode onto aTargetNode
        PRInt32 count;
        rv = aOverlayNode->GetAttributeCount(count);
        if (NS_FAILED(rv)) return rv;

        for (PRInt32 i = 0; i < count; ++i) {
            PRInt32 nameSpaceID;
            nsCOMPtr<nsIAtom> tag;
            rv = aOverlayNode->GetAttributeNameAt(i, nameSpaceID, *getter_AddRefs(tag));
            if (NS_FAILED(rv)) return rv;

            if (nameSpaceID == kNameSpaceID_None && tag.get() == kIdAtom)
                continue;

            nsAutoString value;
            rv = aOverlayNode->GetAttribute(nameSpaceID, tag, value);
            if (NS_FAILED(rv)) return rv;

            rv = aTargetNode->SetAttribute(nameSpaceID, tag, value, PR_FALSE);
            if (NS_FAILED(rv)) return rv;
        }
    }

    {
        // Now move any kids
        PRInt32 count;
        rv = aOverlayNode->ChildCount(count);
        if (NS_FAILED(rv)) return rv;

        for (PRInt32 i = 0; i < count; ++i) {
            // Remove the child from the temporary "overlay
            // placeholder" node, and insert into the content model.
            nsCOMPtr<nsIContent> child;
            rv = aOverlayNode->ChildAt(0, *getter_AddRefs(child));
            if (NS_FAILED(rv)) return rv;

            rv = aOverlayNode->RemoveChildAt(0, PR_FALSE);
            if (NS_FAILED(rv)) return rv;

            rv = InsertElement(aTargetNode, child);
            if (NS_FAILED(rv)) return rv;
        }

        // Add child and any descendants to the element map
        rv = mDocument->AddSubtreeToDocument(aTargetNode);
        if (NS_FAILED(rv)) return rv;

        // Now check for a 'datasources' attribute, and build a
        // template builder if necessary.
        rv = CheckTemplateBuilder(aTargetNode);
        if (NS_FAILED(rv)) return rv;
    }

    return NS_OK;
}



nsXULDocument::OverlayForwardReference::~OverlayForwardReference()
{
#ifdef PR_LOGGING
    if (PR_LOG_TEST(gXULLog, PR_LOG_ALWAYS) && !mResolved) {
        nsAutoString id;
        mOverlay->GetAttribute(kNameSpaceID_None, kIdAtom, id);

        PR_LOG(gXULLog, PR_LOG_ALWAYS,
               ("xul: overlay failed to resolve '%s'",
                (const char*) nsCAutoString(id)));
    }
#endif
}


//----------------------------------------------------------------------
//
// nsXULDocument::BroadcasterHookup
//

nsForwardReference::Result
nsXULDocument::BroadcasterHookup::Resolve()
{
    nsresult rv;

    PRBool listener;
    rv = CheckBroadcasterHookup(mDocument, mObservesElement, &listener, &mResolved);
    if (NS_FAILED(rv)) return eResolve_Error;

    return mResolved ? eResolve_Succeeded : eResolve_Later;
}


nsXULDocument::BroadcasterHookup::~BroadcasterHookup()
{
#ifdef PR_LOGGING
    if (PR_LOG_TEST(gXULLog, PR_LOG_ALWAYS) && !mResolved) {
        // Tell the world we failed
        nsresult rv;

        nsCOMPtr<nsIAtom> tag;
        rv = mObservesElement->GetTag(*getter_AddRefs(tag));
        if (NS_FAILED(rv)) return;

        nsAutoString broadcasterID;
        nsAutoString attribute;

        if (tag.get() == kObservesAtom) {
            rv = mObservesElement->GetAttribute(kNameSpaceID_None, kElementAtom, broadcasterID);
            if (NS_FAILED(rv)) return;

            rv = mObservesElement->GetAttribute(kNameSpaceID_None, kAttributeAtom, attribute);
            if (NS_FAILED(rv)) return;
        }
        else {
            rv = mObservesElement->GetAttribute(kNameSpaceID_None, kObservesAtom, broadcasterID);
            if (NS_FAILED(rv)) return;

            attribute = "*";
        }

        nsAutoString tagStr;
        rv = tag->ToString(tagStr);
        if (NS_FAILED(rv)) return;

        PR_LOG(gXULLog, PR_LOG_ALWAYS,
               ("xul: broadcaster hookup failed <%s attribute='%s'> to %s",
                (const char*) nsCAutoString(tagStr),
                (const char*) nsCAutoString(attribute),
                (const char*) nsCAutoString(broadcasterID)));
    }
#endif
}


//----------------------------------------------------------------------


nsresult
nsXULDocument::CheckBroadcasterHookup(nsXULDocument* aDocument,
                                      nsIContent* aElement,
                                      PRBool* aNeedsHookup,
                                      PRBool* aDidResolve)
{
    // Resolve a broadcaster hookup. Look at the element that we're
    // trying to resolve: it could be an '<observes>' element, or just
    // a vanilla element with an 'observes' attribute on it.
    nsresult rv;

    *aDidResolve = PR_FALSE;

    PRInt32 nameSpaceID;
    rv = aElement->GetNameSpaceID(nameSpaceID);
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIAtom> tag;
    rv = aElement->GetTag(*getter_AddRefs(tag));
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIDOMElement> listener;
    nsAutoString broadcasterID;
    nsAutoString attribute;

    if ((nameSpaceID == kNameSpaceID_XUL) && (tag.get() == kObservesAtom)) {
        // It's an <observes> element, which means that the actual
        // listener is the _parent_ node. This element should have an
        // 'element' attribute that specifies the ID of the
        // broadcaster element, and an 'attribute' element, which
        // specifies the name of the attribute to observe.
        nsCOMPtr<nsIContent> parent;
        rv = aElement->GetParent(*getter_AddRefs(parent));
        if (NS_FAILED(rv)) return rv;

        nsCOMPtr<nsIAtom> parentTag;
        rv = parent->GetTag(*getter_AddRefs(parentTag));
        if (NS_FAILED(rv)) return rv;

        // If we're still parented by an 'overlay' tag, then we haven't
        // made it into the real document yet. Defer hookup.
        if (parentTag.get() == kOverlayAtom) {
            *aNeedsHookup = PR_TRUE;
            return NS_OK;
        }

        listener = do_QueryInterface(parent);

        rv = aElement->GetAttribute(kNameSpaceID_None, kElementAtom, broadcasterID);
        if (NS_FAILED(rv)) return rv;

        rv = aElement->GetAttribute(kNameSpaceID_None, kAttributeAtom, attribute);
        if (NS_FAILED(rv)) return rv;
    }
    else {
        // It's a generic element, which means that we'll use the
        // value of the 'observes' attribute to determine the ID of
        // the broadcaster element, and we'll watch _all_ of its
        // values.
        rv = aElement->GetAttribute(kNameSpaceID_None, kObservesAtom, broadcasterID);
        if (NS_FAILED(rv)) return rv;

        // Bail if there's no broadcasterID
        if ((rv != NS_CONTENT_ATTR_HAS_VALUE) || (broadcasterID.Length() == 0)) {
            *aNeedsHookup = PR_FALSE;
            return NS_OK;
        }

        listener = do_QueryInterface(aElement);

        attribute = "*";
    }

    // Make sure we got a valid listener.
    NS_ASSERTION(listener != nsnull, "no listener");
    if (! listener)
        return NS_ERROR_UNEXPECTED;

    // Try to find the broadcaster element in the document.
    nsCOMPtr<nsIDOMElement> target;
    rv = aDocument->GetElementById(broadcasterID, getter_AddRefs(target));
    if (NS_FAILED(rv)) return rv;

    // If we can't find the broadcaster, then we'll need to defer the
    // hookup. We may need to resolve some of the other overlays
    // first.
    if (! target) {
        *aNeedsHookup = PR_TRUE;
        return NS_OK;
    }

    nsCOMPtr<nsIDOMXULElement> broadcaster = do_QueryInterface(target);
    if (! broadcaster) {
        *aNeedsHookup = PR_FALSE;
        return NS_OK; // not a XUL element, so we can't subscribe
    }

    rv = broadcaster->AddBroadcastListener(attribute, listener);
    if (NS_FAILED(rv)) return rv;

#ifdef PR_LOGGING
    // Tell the world we succeeded
    if (PR_LOG_TEST(gXULLog, PR_LOG_ALWAYS)) {
        nsCOMPtr<nsIContent> content =
            do_QueryInterface(listener);

        NS_ASSERTION(content != nsnull, "not an nsIContent");
        if (! content)
            return rv;

        nsCOMPtr<nsIAtom> tag2;
        rv = content->GetTag(*getter_AddRefs(tag2));
        if (NS_FAILED(rv)) return rv;

        nsAutoString tagStr;
        rv = tag2->ToString(tagStr);
        if (NS_FAILED(rv)) return rv;

        PR_LOG(gXULLog, PR_LOG_ALWAYS,
               ("xul: broadcaster hookup <%s attribute='%s'> to %s",
                (const char*) nsCAutoString(tagStr),
                (const char*) nsCAutoString(attribute),
                (const char*) nsCAutoString(broadcasterID)));
    }
#endif

    *aNeedsHookup = PR_FALSE;
    *aDidResolve = PR_TRUE;
    return NS_OK;
}

nsresult
nsXULDocument::InsertElement(nsIContent* aParent, nsIContent* aChild)
{
    // Insert aChild appropriately into aParent, accountinf for a
    // 'pos' attribute set on aChild.
    nsresult rv;

    nsAutoString posStr;
    rv = aChild->GetAttribute(kNameSpaceID_None, kPositionAtom, posStr);
    if (NS_FAILED(rv)) return rv;

    PRBool wasInserted = PR_FALSE;

    if (rv == NS_CONTENT_ATTR_HAS_VALUE) {
        // Positions are one-indexed.
        PRInt32 pos = posStr.ToInteger(NS_REINTERPRET_CAST(PRInt32*, &rv));
        if (NS_SUCCEEDED(rv)) {
            rv = aParent->InsertChildAt(aChild, pos - 1, PR_FALSE);
            if (NS_FAILED(rv)) return rv;

            wasInserted = PR_TRUE;
        }
    }

    if (! wasInserted) {
        rv = aParent->AppendChildTo(aChild, PR_FALSE);
        if (NS_FAILED(rv)) return rv;
    }

    return NS_OK;
}



PRBool
nsXULDocument::IsChromeURI(nsIURI* aURI)
{
    nsresult rv;
    nsXPIDLCString protocol;
    rv = aURI->GetScheme(getter_Copies(protocol));
    if (NS_SUCCEEDED(rv)) {
        if (PL_strcmp(protocol, "chrome") == 0) {
            return PR_TRUE;
        }
    }

    return PR_FALSE;
}


//----------------------------------------------------------------------
//
// CachedChromeLoader
//

nsXULDocument::CachedChromeLoader::CachedChromeLoader(nsXULDocument* aDocument)
    : mDocument(aDocument),
      mLoading(PR_TRUE)
{
    NS_INIT_REFCNT();
    NS_ADDREF(aDocument);
}

nsXULDocument::CachedChromeLoader::~CachedChromeLoader()
{
    NS_RELEASE(mDocument);
}

NS_IMPL_ADDREF(nsXULDocument::CachedChromeLoader);
NS_IMPL_RELEASE(nsXULDocument::CachedChromeLoader);

NS_IMETHODIMP
nsXULDocument::CachedChromeLoader::QueryInterface(REFNSIID aIID, void** aResult)
{
    NS_PRECONDITION(aResult != nsnull, "null ptr");
    if (! aResult)
        return NS_ERROR_NULL_POINTER;

    if (aIID.Equals(NS_GET_IID(nsIStreamObserver)) ||
        aIID.Equals(NS_GET_IID(nsISupports))) {
        *aResult = this;
        NS_ADDREF(this);
        return NS_OK;
    }
    else {
        *aResult = nsnull;
        return NS_NOINTERFACE;
    }
}

NS_IMETHODIMP
nsXULDocument::CachedChromeLoader::OnStartRequest(nsIChannel* aChannel, nsISupports* aContext)
{
    return NS_OK;
}


NS_IMETHODIMP
nsXULDocument::CachedChromeLoader::OnStopRequest(nsIChannel* aChannel,
                                                 nsISupports* aContext,
                                                 nsresult aStatus,
                                                 const PRUnichar* aErrorMsg)
{
    // Not loading. Bail!
    if (! mLoading)
        return NS_OK;

    nsCOMPtr<nsILoadGroup> group = do_QueryReferent(mDocument->mDocumentLoadGroup);

    // No more load group. Bail!
    if (! group)
        return NS_OK;

    nsresult rv;
    PRUint32 count;
    rv = group->GetActiveCount(&count);
    if (NS_FAILED(rv)) return rv;

    // Still loading. Bail!
    if (count != 0)
        return NS_OK;

    mLoading = PR_FALSE;

    for (PRInt32 i = mDocument->GetNumberOfShells() - 1; i >= 0; --i) {
        // Walk from the pres shell down to the script context
        // owner. We'll consider any failure along the way
        // non-terminal, and will continue on to the other pres
        // shells.
        nsIPresShell* shell = mDocument->GetShellAt(i);
        NS_ASSERTION(shell != nsnull, "null shell");
        if (! shell)
            continue;

        // the pres shell has a pres context...
        nsCOMPtr<nsIPresContext> presContext;
        shell->GetPresContext(getter_AddRefs(presContext));
        NS_ASSERTION(presContext != nsnull, "shell has no pres context");
        if (! presContext)
            continue;

        // the pres context should have a content container (really the webshell)...
        nsCOMPtr<nsISupports> container;
        presContext->GetContainer(getter_AddRefs(container));
        NS_ASSERTION(container != nsnull, "pres context has no container");
        if (! container)
            continue;

        // the container should be a script context owner...
        nsCOMPtr<nsIScriptContextOwner> owner = do_QueryInterface(container);
        NS_ASSERTION(owner != nsnull, "container is not a script context owner");
        if (! owner)
            continue;

        // and the script context owner has a script global object...
        nsCOMPtr<nsIScriptGlobalObject> global;
        rv = owner->GetScriptGlobalObject(getter_AddRefs(global));
        if (NS_FAILED(rv))
            continue;

        // dispatch the 'load' event to the script global object
        nsEventStatus status = nsEventStatus_eIgnore;
        nsMouseEvent event;
        event.eventStructType = NS_EVENT;
        event.message = NS_PAGE_LOAD;

        rv = global->HandleDOMEvent(presContext, &event, nsnull, NS_EVENT_FLAG_INIT, &status);
        if (NS_FAILED(rv))
            continue;
    }

    return NS_OK;
}

