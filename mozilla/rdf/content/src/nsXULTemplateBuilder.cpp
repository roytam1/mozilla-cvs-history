/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License.  You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are Copyright (C) 1998
 * Netscape Communications Corporation.  All Rights Reserved.
 */

/*

  An nsIRDFDocument implementation that builds a tree widget XUL
  content model that is to be used with a tree control.

  TO DO

  1) Get a real namespace for XUL. This should go in a 
     header file somewhere.

  2) Can we do sorting as an insertion sort? This is especially
     important in the case where content streams in; e.g., gets added
     in the OnAssert() method as opposed to the CreateContents()
     method.

  3) I really _really_ need to figure out how to factor the logic in
     OnAssert, OnUnassert, OnMove, and OnChange. These all mostly
     kinda do the same sort of thing. It atrocious how much code is
     cut-n-pasted.

 */

#include "nsCOMPtr.h"
#include "nsCRT.h"
#include "nsIAtom.h"
#include "nsIContent.h"
#include "nsIDOMElement.h"
#include "nsIDOMElementObserver.h"
#include "nsIDOMNode.h"
#include "nsIDOMNodeObserver.h"
#include "nsIDocument.h"
#include "nsINameSpaceManager.h"
#include "nsIRDFContentModelBuilder.h"
#include "nsIRDFCompositeDataSource.h"
#include "nsIRDFDocument.h"
#include "nsIRDFNode.h"
#include "nsIRDFObserver.h"
#include "nsIRDFService.h"
#include "nsIServiceManager.h"
#include "nsINameSpaceManager.h"
#include "nsIServiceManager.h"
#include "nsISupportsArray.h"
#include "nsIURL.h"
#include "nsLayoutCID.h"
#include "nsRDFCID.h"
#include "nsRDFContentUtils.h"
#include "nsString.h"
#include "nsXPIDLString.h"
#include "rdf.h"
#include "rdfutil.h"
#include "nsITimer.h"
#include "nsITimerCallback.h"
#include "nsIDOMXULElement.h"
#include "nsVoidArray.h"
#include "nsIXULSortService.h"
#include "nsIHTMLElementFactory.h"
#include "nsIHTMLContent.h"
#include "nsRDFGenericBuilder.h"

#include "nsIDOMXULDocument.h"

// Remove this to try running without the C++ builders. This will
// inhibit delegation to the fallback builder for constructing content
// (but still relies on it for determining the correct tagset to use).
#define FALLBACK_BUILDERS 1

////////////////////////////////////////////////////////////////////////

static NS_DEFINE_IID(kIContentIID,                NS_ICONTENT_IID);
static NS_DEFINE_IID(kIDocumentIID,               NS_IDOCUMENT_IID);
static NS_DEFINE_IID(kINameSpaceManagerIID,       NS_INAMESPACEMANAGER_IID);
static NS_DEFINE_IID(kIRDFResourceIID,            NS_IRDFRESOURCE_IID);
static NS_DEFINE_IID(kIRDFLiteralIID,             NS_IRDFLITERAL_IID);
static NS_DEFINE_IID(kIRDFContentModelBuilderIID, NS_IRDFCONTENTMODELBUILDER_IID);
static NS_DEFINE_IID(kIRDFObserverIID,            NS_IRDFOBSERVER_IID);
static NS_DEFINE_IID(kIRDFServiceIID,             NS_IRDFSERVICE_IID);
static NS_DEFINE_IID(kISupportsIID,               NS_ISUPPORTS_IID);

static NS_DEFINE_CID(kNameSpaceManagerCID,        NS_NAMESPACEMANAGER_CID);
static NS_DEFINE_CID(kRDFServiceCID,              NS_RDFSERVICE_CID);
static NS_DEFINE_CID(kRDFContainerUtilsCID,       NS_RDFCONTAINERUTILS_CID);

static NS_DEFINE_IID(kXULSortServiceCID,         NS_XULSORTSERVICE_CID);
static NS_DEFINE_IID(kIXULSortServiceIID,        NS_IXULSORTSERVICE_IID);

static NS_DEFINE_CID(kHTMLElementFactoryCID,  NS_HTML_ELEMENT_FACTORY_CID);
static NS_DEFINE_CID(kIHTMLElementFactoryIID, NS_IHTML_ELEMENT_FACTORY_IID);

////////////////////////////////////////////////////////////////////////

nsrefcnt		RDFGenericBuilderImpl::gRefCnt = 0;
nsIXULSortService*	RDFGenericBuilderImpl::XULSortService = nsnull;

nsIAtom* RDFGenericBuilderImpl::kContainerAtom;
nsIAtom* RDFGenericBuilderImpl::kIsContainerAtom;
nsIAtom* RDFGenericBuilderImpl::kXULContentsGeneratedAtom;
nsIAtom* RDFGenericBuilderImpl::kItemContentsGeneratedAtom;
nsIAtom* RDFGenericBuilderImpl::kIdAtom;
nsIAtom* RDFGenericBuilderImpl::kOpenAtom;
nsIAtom* RDFGenericBuilderImpl::kResourceAtom;
nsIAtom* RDFGenericBuilderImpl::kURIAtom;
nsIAtom* RDFGenericBuilderImpl::kContainmentAtom;
nsIAtom* RDFGenericBuilderImpl::kNaturalOrderPosAtom;
nsIAtom* RDFGenericBuilderImpl::kIgnoreAtom;

nsIAtom* RDFGenericBuilderImpl::kSubcontainmentAtom;
nsIAtom* RDFGenericBuilderImpl::kRootcontainmentAtom;
nsIAtom* RDFGenericBuilderImpl::kTemplateAtom;
nsIAtom* RDFGenericBuilderImpl::kRuleAtom;
nsIAtom* RDFGenericBuilderImpl::kTreeContentsGeneratedAtom;
nsIAtom* RDFGenericBuilderImpl::kTextAtom;
nsIAtom* RDFGenericBuilderImpl::kPropertyAtom;
nsIAtom* RDFGenericBuilderImpl::kInstanceOfAtom;

PRInt32  RDFGenericBuilderImpl::kNameSpaceID_RDF;
PRInt32  RDFGenericBuilderImpl::kNameSpaceID_XUL;

nsIRDFService*  RDFGenericBuilderImpl::gRDFService;
nsIRDFContainerUtils* RDFGenericBuilderImpl::gRDFContainerUtils;
nsINameSpaceManager* RDFGenericBuilderImpl::gNameSpaceManager;

nsIRDFResource* RDFGenericBuilderImpl::kNC_Title;
nsIRDFResource* RDFGenericBuilderImpl::kNC_child;
nsIRDFResource* RDFGenericBuilderImpl::kNC_Column;
nsIRDFResource* RDFGenericBuilderImpl::kNC_Folder;
nsIRDFResource* RDFGenericBuilderImpl::kRDF_child;


////////////////////////////////////////////////////////////////////////


RDFGenericBuilderImpl::RDFGenericBuilderImpl(void)
    : mDocument(nsnull),
      mDB(nsnull),
      mRoot(nsnull),
      mTimer(nsnull)
{
    NS_INIT_REFCNT();

    if (gRefCnt == 0) {
        kContainerAtom             = NS_NewAtom("container");
		kIsContainerAtom           = NS_NewAtom("iscontainer");
		kXULContentsGeneratedAtom  = NS_NewAtom("xulcontentsgenerated");
        kItemContentsGeneratedAtom = NS_NewAtom("itemcontentsgenerated");
        kTreeContentsGeneratedAtom = NS_NewAtom("treecontentsgenerated");

        kIdAtom              = NS_NewAtom("id");
        kOpenAtom            = NS_NewAtom("open");
        kResourceAtom        = NS_NewAtom("resource");
		kURIAtom             = NS_NewAtom("uri");
        kContainmentAtom     = NS_NewAtom("containment");
        kIgnoreAtom          = NS_NewAtom("ignore");
        kNaturalOrderPosAtom = NS_NewAtom("pos");

        kSubcontainmentAtom  = NS_NewAtom("subcontainment");
        kRootcontainmentAtom = NS_NewAtom("rootcontainment");
        kTemplateAtom    = NS_NewAtom("template");
        kRuleAtom            = NS_NewAtom("rule");

        kTextAtom            = NS_NewAtom("text");
        kPropertyAtom        = NS_NewAtom("property");
        kInstanceOfAtom      = NS_NewAtom("instanceOf");

        nsresult rv;

        // Register the XUL and RDF namespaces: these'll just retrieve
        // the IDs if they've already been registered by someone else.
        if (NS_SUCCEEDED(rv = nsComponentManager::CreateInstance(kNameSpaceManagerCID,
                                                           nsnull,
                                                           kINameSpaceManagerIID,
                                                           (void**) &gNameSpaceManager))) {

// XXX This is sure to change. Copied from mozilla/layout/xul/content/src/nsXULAtoms.cpp
static const char kXULNameSpaceURI[]
    = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";

static const char kRDFNameSpaceURI[]
    = RDF_NAMESPACE_URI;

            rv = gNameSpaceManager->RegisterNameSpace(kXULNameSpaceURI, kNameSpaceID_XUL);
            NS_ASSERTION(NS_SUCCEEDED(rv), "unable to register XUL namespace");

            rv = gNameSpaceManager->RegisterNameSpace(kRDFNameSpaceURI, kNameSpaceID_RDF);
            NS_ASSERTION(NS_SUCCEEDED(rv), "unable to register RDF namespace");
        }
        else {
            NS_ERROR("couldn't create namepsace manager");
        }


        // Initialize the global shared reference to the service
        // manager and get some shared resource objects.
        rv = nsServiceManager::GetService(kRDFServiceCID,
                                          kIRDFServiceIID,
                                          (nsISupports**) &gRDFService);

        if (NS_SUCCEEDED(rv)) {
            gRDFService->GetResource(NC_NAMESPACE_URI "Title",   &kNC_Title);
            gRDFService->GetResource(NC_NAMESPACE_URI "child",   &kNC_child);
            gRDFService->GetResource(NC_NAMESPACE_URI "Column",  &kNC_Column);
            gRDFService->GetResource(NC_NAMESPACE_URI "Folder",  &kNC_Folder);
            gRDFService->GetResource(RDF_NAMESPACE_URI "child",  &kRDF_child);
        }

        rv = nsServiceManager::GetService(kRDFContainerUtilsCID,
                                          nsIRDFContainerUtils::GetIID(),
                                          (nsISupports**) &gRDFContainerUtils);

        rv = nsServiceManager::GetService(kXULSortServiceCID,
                                          kIXULSortServiceIID,
                                          (nsISupports**) &XULSortService);

    }
    ++gRefCnt;
}

RDFGenericBuilderImpl::~RDFGenericBuilderImpl(void)
{
    NS_IF_RELEASE(mRoot);
    if (mDB) {
        mDB->RemoveObserver(this);
        NS_RELEASE(mDB);
    }
    // NS_IF_RELEASE(mDocument) not refcounted

    --gRefCnt;
    if (gRefCnt == 0) {
        NS_RELEASE(kContainerAtom);
        NS_RELEASE(kIsContainerAtom);
		NS_RELEASE(kXULContentsGeneratedAtom);
        NS_RELEASE(kItemContentsGeneratedAtom);

        NS_RELEASE(kIdAtom);
        NS_RELEASE(kOpenAtom);
        NS_RELEASE(kResourceAtom);
		NS_RELEASE(kURIAtom);
        NS_RELEASE(kContainmentAtom);
        NS_RELEASE(kIgnoreAtom);
        NS_RELEASE(kNaturalOrderPosAtom);

        NS_RELEASE(kSubcontainmentAtom);
        NS_RELEASE(kRootcontainmentAtom);
        NS_RELEASE(kTemplateAtom);
        NS_RELEASE(kRuleAtom);
        NS_RELEASE(kTreeContentsGeneratedAtom);
        NS_RELEASE(kTextAtom);
        NS_RELEASE(kPropertyAtom);
        NS_RELEASE(kInstanceOfAtom);

        NS_RELEASE(kNC_Title);
        NS_RELEASE(kNC_Column);

        nsServiceManager::ReleaseService(kRDFServiceCID, gRDFService);
        nsServiceManager::ReleaseService(kRDFContainerUtilsCID, gRDFContainerUtils);
        nsServiceManager::ReleaseService(kXULSortServiceCID, XULSortService);
        NS_RELEASE(gNameSpaceManager);
    }

    if (mTimer)
    {
        mTimer->Cancel();
        NS_RELEASE(mTimer);
        mTimer = nsnull;
    }
}

////////////////////////////////////////////////////////////////////////

NS_IMPL_ADDREF(RDFGenericBuilderImpl);
NS_IMPL_RELEASE(RDFGenericBuilderImpl);

NS_IMETHODIMP
RDFGenericBuilderImpl::QueryInterface(REFNSIID iid, void** aResult)
{
    NS_PRECONDITION(aResult != nsnull, "null ptr");
    if (! aResult)
        return NS_ERROR_NULL_POINTER;

    if (iid.Equals(kIRDFContentModelBuilderIID) ||
        iid.Equals(kISupportsIID)) {
        *aResult = NS_STATIC_CAST(nsIRDFContentModelBuilder*, this);
    }
    else if (iid.Equals(kIRDFObserverIID)) {
        *aResult = NS_STATIC_CAST(nsIRDFObserver*, this);
    }
    else if (iid.Equals(nsIDOMNodeObserver::GetIID())) {
        *aResult = NS_STATIC_CAST(nsIDOMNodeObserver*, this);
    }
    else if (iid.Equals(nsIDOMElementObserver::GetIID())) {
        *aResult = NS_STATIC_CAST(nsIDOMElementObserver*, this);
    }
    else {
        *aResult = nsnull;
        return NS_NOINTERFACE;
    }
    NS_ADDREF(this);
    return NS_OK;
}

////////////////////////////////////////////////////////////////////////
// nsIRDFContentModelBuilder methods

NS_IMETHODIMP
RDFGenericBuilderImpl::SetDocument(nsIRDFDocument* aDocument)
{
    // note: null now allowed, it indicates document going away

    mDocument = aDocument; // not refcounted
    if (aDocument)
    {
    	if (nsnull == mTimer)
    	{
		NS_VERIFY(NS_SUCCEEDED(NS_NewTimer(&mTimer)), "couldn't get timer");
		if (mTimer)
		{
			mTimer->Init(this, /* PR_TRUE, */ 1L);
		}
	}
    }
    else
    {
    	if (mTimer)
    	{
    		mTimer->Cancel();
    		NS_RELEASE(mTimer);
    		mTimer = nsnull;
    	}
    }
    return NS_OK;
}


NS_IMETHODIMP
RDFGenericBuilderImpl::SetDataBase(nsIRDFCompositeDataSource* aDataBase)
{
    NS_PRECONDITION(aDataBase != nsnull, "null ptr");
    if (! aDataBase)
        return NS_ERROR_NULL_POINTER;

    NS_PRECONDITION(mDB == nsnull, "already initialized");
    if (mDB)
        return NS_ERROR_ALREADY_INITIALIZED;

    mDB = aDataBase;
    NS_ADDREF(mDB);

    mDB->AddObserver(this);
    return NS_OK;
}


NS_IMETHODIMP
RDFGenericBuilderImpl::GetDataBase(nsIRDFCompositeDataSource** aDataBase)
{
    NS_PRECONDITION(aDataBase != nsnull, "null ptr");
    if (! aDataBase)
        return NS_ERROR_NULL_POINTER;

    *aDataBase = mDB;
    NS_ADDREF(mDB);
    return NS_OK;
}


NS_IMETHODIMP
RDFGenericBuilderImpl::CreateRootContent(nsIRDFResource* aResource)
{
    // Create a structure that looks like this:
    //
    // <html:document>
    //    <html:body>
    //       <xul:rootwidget>
    //       </xul:rootwidget>
    //    </html:body>
    // </html:document>
    //
    // Eventually, we should be able to do away with the HTML tags and
    // just have it create XUL in some other context.

    nsresult rv;
    nsCOMPtr<nsIAtom>       tag;
    nsCOMPtr<nsIDocument>   doc;
    nsCOMPtr<nsIAtom>       rootAtom;
    if (NS_FAILED(rv = GetRootWidgetAtom(getter_AddRefs(rootAtom)))) {
        return rv;
    }

    if (NS_FAILED(rv = mDocument->QueryInterface(kIDocumentIID,
                                                 (void**) getter_AddRefs(doc))))
        return rv;

    // Create the DOCUMENT element
    if ((tag = dont_AddRef(NS_NewAtom("document"))) == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;

    nsCOMPtr<nsIContent> root;
    if (NS_FAILED(rv = NS_NewRDFElement(kNameSpaceID_HTML, /* XXX */
                                        tag,
                                        getter_AddRefs(root))))
        return rv;

    doc->SetRootContent(NS_STATIC_CAST(nsIContent*, root));

    // Create the BODY element
    nsCOMPtr<nsIContent> body;
    if ((tag = dont_AddRef(NS_NewAtom("body"))) == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;

    if (NS_FAILED(rv = NS_NewRDFElement(kNameSpaceID_HTML, /* XXX */
                                        tag,
                                        getter_AddRefs(body))))
        return rv;


    // Attach the BODY to the DOCUMENT
    if (NS_FAILED(rv = root->AppendChildTo(body, PR_FALSE)))
        return rv;

    // Create the xul:rootwidget element, and indicate that children should
    // be recursively generated on demand
    nsCOMPtr<nsIContent> widget;
    if (NS_FAILED(rv = CreateElement(kNameSpaceID_XUL,
                                     rootAtom,
                                     aResource,
                                     getter_AddRefs(widget))))
        return rv;

    if (NS_FAILED(rv = widget->SetAttribute(kNameSpaceID_RDF, kContainerAtom, "true", PR_FALSE)))
        return rv;

    // Attach the ROOTWIDGET to the BODY
    if (NS_FAILED(rv = body->AppendChildTo(widget, PR_FALSE)))
        return rv;

    return NS_OK;
}

NS_IMETHODIMP
RDFGenericBuilderImpl::SetRootContent(nsIContent* aElement)
{
    NS_IF_RELEASE(mRoot);
    mRoot = aElement;
    NS_IF_ADDREF(mRoot);
    return NS_OK;
}


NS_IMETHODIMP
RDFGenericBuilderImpl::CreateContents(nsIContent* aElement)
{
    nsresult rv;

    // First, make sure that the element is in the right widget -- ours.
    if (!IsElementInWidget(aElement))
        return NS_OK;

    // Next, see if the item is even "open". If not, then just
    // pretend it doesn't have _any_ contents. We check this _before_
    // checking the contents-generated attribute so that we don't
    // eagerly set contents-generated on a closed node.
    if (!IsOpen(aElement))
        return NS_OK;

    // Now see if someone has marked the element's contents as being
    // generated: this prevents a re-entrant call from triggering
    // another generation.
    nsAutoString attrValue;
    if (NS_FAILED(rv = aElement->GetAttribute(kNameSpaceID_None,
                                              kItemContentsGeneratedAtom,
                                              attrValue))) {
        NS_ERROR("unable to test contents-generated attribute");
        return rv;
    }

    if ((rv == NS_CONTENT_ATTR_HAS_VALUE) && (attrValue.EqualsIgnoreCase("true")))
        return NS_OK;

    // Now mark the element's contents as being generated so that
    // any re-entrant calls don't trigger an infinite recursion.
    if (NS_FAILED(rv = aElement->SetAttribute(kNameSpaceID_None,
                                              kItemContentsGeneratedAtom,
                                              "true",
                                              PR_FALSE))) {
        NS_ERROR("unable to set contents-generated attribute");
        return rv;
    }

    // Get the element's resource so that we can generate cell
    // values. We could QI for the nsIRDFResource here, but doing this
    // via the nsIContent interface allows us to support generic nodes
    // that might get added in by DOM calls.
    nsCOMPtr<nsIRDFResource> resource;
    if (NS_FAILED(rv = nsRDFContentUtils::GetElementRefResource(aElement, getter_AddRefs(resource)))) {
        NS_ERROR("unable to get element resource");
        return rv;
    }

    // XXX Eventually, we may want to factor this into one method that
    // handles RDF containers (RDF:Bag, et al.) and another that
    // handles multi-attributes. For performance...

    // Create a cursor that'll enumerate all of the outbound arcs
    nsCOMPtr<nsISimpleEnumerator> properties;
    rv = mDB->ArcLabelsOut(resource, getter_AddRefs(properties));
    if (NS_FAILED(rv)) return rv;

// rjc - sort
	nsISupportsArray	*tempArray;
	if (NS_FAILED(rv = NS_NewISupportsArray(&tempArray)))
		return(rv);

    while (1) {
        PRBool hasMore;
        rv = properties->HasMoreElements(&hasMore);
        if (NS_FAILED(rv)) return rv;

        if (! hasMore)
            break;

        nsCOMPtr<nsISupports> isupports;
        rv = properties->GetNext(getter_AddRefs(isupports));
        if (NS_FAILED(rv)) return rv;

        nsCOMPtr<nsIRDFResource> property = do_QueryInterface(isupports);

        // If it's not a widget item property, then it doesn't specify an
        // object that is member of the current container element;
        // rather, it specifies a "simple" property of the container
        // element. Skip it.
        if (!IsContainmentProperty(aElement, property))
            continue;

        // Create a second cursor that'll enumerate all of the values
        // for all of the arcs.
        nsCOMPtr<nsISimpleEnumerator> targets;
        rv = mDB->GetTargets(resource, property, PR_TRUE, getter_AddRefs(targets));
        if (NS_FAILED(rv)) return rv;

        while (1) {
            PRBool hasMoreElements;
            rv = targets->HasMoreElements(&hasMoreElements);
            if (NS_FAILED(rv)) return rv;

            if (! hasMoreElements)
                break;

            nsCOMPtr<nsISupports> isupportsNext;
            rv = targets->GetNext(getter_AddRefs(isupportsNext));
            if (NS_FAILED(rv)) return rv;

            nsCOMPtr<nsIRDFResource> valueResource = do_QueryInterface(isupportsNext);
            if (valueResource && IsContainmentProperty(aElement, property)) {
			/* XXX hack: always append value resource 1st!
			       due to sort callback implementation */
                	tempArray->AppendElement(valueResource);
                	tempArray->AppendElement(property);
            }
            else {
                nsCOMPtr<nsIRDFNode> value = do_QueryInterface(isupportsNext);
                rv = SetWidgetAttribute(aElement, property, value);
                if (NS_FAILED(rv)) return rv;
            }
        }
    }

    PRUint32 cnt = 0;
    rv = tempArray->Count(&cnt);
    NS_ASSERTION(NS_SUCCEEDED(rv), "Count failed");
    unsigned long numElements = cnt;
    if (numElements > 0)
    {
        nsIRDFResource ** flatArray = new nsIRDFResource *[numElements];
        if (flatArray)
        {
			// flatten array of resources, sort them, then add as item elements
			unsigned long loop;

            for (loop=0; loop<numElements; loop++)
				flatArray[loop] = (nsIRDFResource *)tempArray->ElementAt(loop);

			if (nsnull != XULSortService)
			{
				XULSortService->OpenContainer(mDB, aElement, flatArray, numElements/2, 2*sizeof(nsIRDFResource *));
			}

            for (loop=0; loop<numElements; loop+=2)
            {
                rv = CreateWidgetItem(aElement, flatArray[loop+1], flatArray[loop], loop+1);
                NS_ASSERTION(NS_SUCCEEDED(rv), "unable to create widget item");
            }

            for (int i = numElements - 1; i >=0; i--)
            {
				NS_IF_RELEASE(flatArray[i]);
            }
			delete [] flatArray;
        }
    }
    for (int i = numElements - 1; i >= 0; i--)
    {
        tempArray->RemoveElementAt(i);
    }
    NS_IF_RELEASE(tempArray);

    return NS_OK;
}


NS_IMETHODIMP
RDFGenericBuilderImpl::CreateElement(PRInt32 aNameSpaceID,
                                     nsIAtom* aTag,
                                     nsIRDFResource* aResource,
                                     nsIContent** aResult)
{
    nsresult rv;

    nsCOMPtr<nsIContent> result;
    rv = NS_NewRDFElement(aNameSpaceID, aTag, getter_AddRefs(result));
    NS_ASSERTION(NS_SUCCEEDED(rv), "unable to create new RDFElement");
    if (NS_FAILED(rv)) return rv;

    nsXPIDLCString uri;
    rv = aResource->GetValue( getter_Copies(uri) );
    NS_ASSERTION(NS_SUCCEEDED(rv), "unable to get resource URI");
    if (NS_FAILED(rv)) return rv;

    rv = result->SetAttribute(kNameSpaceID_None, kIdAtom, (const char*) uri, PR_FALSE);
    NS_ASSERTION(NS_SUCCEEDED(rv), "unable to set id attribute");
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIDocument> doc( do_QueryInterface(mDocument) );
    rv = result->SetDocument(doc, PR_FALSE);
    NS_ASSERTION(NS_SUCCEEDED(rv), "unable to set element's document");
    if (NS_FAILED(rv)) return rv;

    *aResult = result;
    NS_ADDREF(*aResult);
    return NS_OK;
}


NS_IMETHODIMP
RDFGenericBuilderImpl::SetAllAttributesOnElement(nsIContent *aParentNode, nsIContent *aNode, nsIRDFResource *res)
{
	// get all arcs out, and if not a containment property, set attribute
	nsresult			rv = NS_OK;
	PRBool				markAsContainer = PR_FALSE;
	nsCOMPtr<nsISimpleEnumerator>	arcs;
	if (NS_SUCCEEDED(rv = mDB->ArcLabelsOut(res, getter_AddRefs(arcs))))
	{
		PRBool		hasMore = PR_TRUE;
		while(hasMore)
		{
			rv = arcs->HasMoreElements(&hasMore);
			if (NS_FAILED(rv))		break;
			if (hasMore == PR_FALSE)	break;

			nsCOMPtr<nsISupports>	isupports;
			if (NS_FAILED(rv = arcs->GetNext(getter_AddRefs(isupports))))	break;

			nsCOMPtr<nsIRDFResource> property = do_QueryInterface(isupports);
			// Ignore properties that are used to indicate "tree-ness"
			if (IsContainmentProperty(aNode, property))
			{
				markAsContainer = PR_TRUE;
				continue;
			}

			// Ignore any properties set in ignore attribute
			// Note: since node isn't in the content model yet, start with its parent
			if (IsIgnoredProperty(aParentNode, property))
			{
				continue;
			}

			PRInt32			nameSpaceID;
			nsCOMPtr<nsIAtom>	tag;
			if (NS_FAILED(rv = mDocument->SplitProperty(property, &nameSpaceID, getter_AddRefs(tag))))
				break;
			nsCOMPtr<nsIRDFNode>	value;
			if (NS_FAILED(rv = mDB->GetTarget(res, property, PR_TRUE, getter_AddRefs(value))))
				break;
			if (rv == NS_RDF_NO_VALUE)
				continue;
			nsAutoString		s;
			if (NS_SUCCEEDED(rv = nsRDFContentUtils::GetTextForNode(value, s)))
			{
				aNode->SetAttribute(nameSpaceID, tag, s, PR_FALSE);
			}
		}
	}
	if (markAsContainer == PR_TRUE)
	{
		// mark this as a "container" so that we know to
		// recursively generate kids if they're asked for.
		rv = aNode->SetAttribute(kNameSpaceID_RDF, kContainerAtom, "true", PR_FALSE);
	}
	return(rv);
}


NS_IMETHODIMP
RDFGenericBuilderImpl::IsTemplateRuleMatch(nsIRDFResource *aNode, nsIContent *aRule, PRBool *matchingRuleFound)
{
	nsresult		rv = NS_OK;
	PRInt32			count;

	*matchingRuleFound = PR_FALSE;

	if (NS_FAILED(rv = aRule->GetAttributeCount(count)))
	{
		return(rv);
	}

	*matchingRuleFound = PR_TRUE;
	for (PRInt32 loop=0; loop<count; loop++)
	{
		PRInt32			attribNameSpaceID;
		nsCOMPtr<nsIAtom>	attribAtom;
		if (NS_FAILED(rv = aRule->GetAttributeNameAt(loop, attribNameSpaceID, *getter_AddRefs(attribAtom))))
		{
			*matchingRuleFound = PR_FALSE;
			break;
		}
		nsAutoString		attribValue;
		if (NS_FAILED(rv = aRule->GetAttribute(attribNameSpaceID, attribAtom, attribValue)))
		{
			*matchingRuleFound = PR_FALSE;
			break;
		}

#ifdef	DEBUG
		nsAutoString		nsName;
		attribAtom->ToString(nsName);
		char *debugName = nsName.ToNewCString();
		if (debugName)
		{
			delete [] debugName;
			debugName = nsnull;
		}
#endif

		// Note: some attributes must be skipped on XUL template rule subtree

		// never compare against rdf:container attribute
		if ((attribAtom.get() == kContainerAtom) && (attribNameSpaceID == kNameSpaceID_RDF))
			continue;
		// never compare against rdf:property attribute
		else if ((attribAtom.get() == kPropertyAtom) && (attribNameSpaceID == kNameSpaceID_RDF))
			continue;
		// never compare against rdf:instanceOf attribute
		else if ((attribAtom.get() == kInstanceOfAtom) && (attribNameSpaceID == kNameSpaceID_RDF))
			continue;
		// never compare against {}:id attribute
		else if ((attribAtom.get() == kIdAtom) && (attribNameSpaceID == kNameSpaceID_None))
			continue;
		// never compare against {}:rootcontainment attribute
		else if ((attribAtom.get() == kRootcontainmentAtom) && (attribNameSpaceID == kNameSpaceID_None))
			continue;
		// never compare against {}:subcontainment attribute
		else if ((attribAtom.get() == kSubcontainmentAtom) && (attribNameSpaceID == kNameSpaceID_None))
			continue;
		// never compare against {}:xulcontentsgenerated attribute
		else if ((attribAtom.get() == kXULContentsGeneratedAtom) && (attribNameSpaceID == kNameSpaceID_None))
			continue;
		// never compare against {}:itemcontentsgenerated attribute (bogus)
		else if ((attribAtom.get() == kItemContentsGeneratedAtom) && (attribNameSpaceID == kNameSpaceID_None))
			continue;
		// never compare against {}:treecontentsgenerated attribute (bogus)
		else if ((attribAtom.get() == kTreeContentsGeneratedAtom) && (attribNameSpaceID == kNameSpaceID_None))
			continue;

		else if ((attribNameSpaceID == kNameSpaceID_None) && (attribAtom.get() == kIsContainerAtom))
		{
			// check and see if aNode is a container
			PRBool	containerFlag = IsContainer(aRule, aNode);
			if (containerFlag && (!attribValue.EqualsIgnoreCase("true")))
			{
				*matchingRuleFound = PR_FALSE;
				break;
			}
			else if (!containerFlag && (!attribValue.EqualsIgnoreCase("false")))
			{
				*matchingRuleFound = PR_FALSE;
				break;
			}
		}
		else
		{
			nsCOMPtr<nsIRDFResource>	attribAtomResource;
			if (NS_FAILED(rv = GetResource(attribNameSpaceID, attribAtom, getter_AddRefs(attribAtomResource))))
			{
				*matchingRuleFound = PR_FALSE;
				break;
			}
			nsCOMPtr<nsIRDFNode>	aResult;
			if (NS_FAILED(rv = mDB->GetTarget(aNode, attribAtomResource, PR_TRUE, getter_AddRefs(aResult))))
			{
				*matchingRuleFound = PR_FALSE;
				break;
			}
			nsAutoString	resultStr;
			if (NS_FAILED(rv = nsRDFContentUtils::GetTextForNode(aResult, resultStr)))
			{
				*matchingRuleFound = PR_FALSE;
				break;
			}
			if (!attribValue.Equals(resultStr))
			{
				*matchingRuleFound = PR_FALSE;
				break;
			}
		}
	}
	return(rv);
}

NS_IMETHODIMP
RDFGenericBuilderImpl::FindTemplateForResource(nsIRDFResource *aNode, nsIContent **theTemplate)
{
	nsresult		rv;
	PRInt32			count;
	nsCOMPtr<nsIContent>	aTemplate;

	*theTemplate = nsnull;
	if (NS_FAILED(rv = mRoot->ChildCount(count)))	return(rv);

	for (PRInt32 loop=0; loop<count; loop++)
	{
		if (NS_FAILED(rv = mRoot->ChildAt(loop, *getter_AddRefs(aTemplate))))
			continue;
		PRInt32 nameSpaceID;
		if (NS_FAILED(rv = aTemplate->GetNameSpaceID(nameSpaceID)))
			continue;
		if (nameSpaceID != kNameSpaceID_XUL)
			continue;

		nsCOMPtr<nsIAtom>	tag;
		if (NS_FAILED(rv = aTemplate->GetTag(*getter_AddRefs(tag))))
			continue;
		if (tag.get() != kTemplateAtom)
			continue;

/*
		// check for debugging
		nsAutoString		debugValue;
		if (NS_SUCCEEDED(rv = aTemplate->GetAttribute(kNameSpaceID_None, kDebugAtom, debugValue)))
		{
			debugValue = "true";
		}
*/

		// found a template; check against any (optional) rules
		PRInt32		numRuleChildren, numRulesFound = 0;
		if (NS_FAILED(rv = aTemplate->ChildCount(numRuleChildren)))
			continue;
		for (PRInt32 ruleLoop=0; ruleLoop<numRuleChildren; ruleLoop++)
		{
			nsCOMPtr<nsIContent>	aRule;
			if (NS_FAILED(rv = aTemplate->ChildAt(ruleLoop, *getter_AddRefs(aRule))))
				continue;
			PRInt32	ruleNameSpaceID;
			if (NS_FAILED(rv = aRule->GetNameSpaceID(ruleNameSpaceID)))
				continue;
			if (ruleNameSpaceID != kNameSpaceID_XUL)
				continue;
			nsCOMPtr<nsIAtom>	ruleTag;
			if (NS_SUCCEEDED(rv = aRule->GetTag(*getter_AddRefs(ruleTag))))
			{
				if (ruleTag.get() == kRuleAtom)
				{
					++numRulesFound;
					PRBool		matchingRuleFound = PR_FALSE;
					if (NS_SUCCEEDED(rv = IsTemplateRuleMatch(aNode, aRule,
						&matchingRuleFound)))
					{
						if (matchingRuleFound == PR_TRUE)
						{
							// found a matching rule, use it as the template
							*theTemplate = aRule;
							NS_ADDREF(*theTemplate);
							return(NS_OK);
						}
					}
				}
			}
		}
		if (numRulesFound == 0)
		{
			// if no rules are specified in the template, just use it
			*theTemplate = aTemplate;
			NS_ADDREF(*theTemplate);
			break;
		}
	}
	return(rv);
}


NS_IMETHODIMP
RDFGenericBuilderImpl::PopulateWidgetItemSubtree(nsIContent *aTemplateRoot, nsIContent *aTemplate,
		nsIContent *treeChildren, nsIContent* aElement, PRBool isUnique, nsIRDFResource* aProperty,
		nsIRDFResource* aValue, PRInt32 aNaturalOrderPos)
{
	PRInt32		count;
	nsresult	rv;

	if (NS_FAILED(rv = aTemplate->ChildCount(count)))	return(rv);
	for (PRInt32 loop=0; loop<count; loop++)
	{
		nsCOMPtr<nsIContent>	aTemplateKid;
		if (NS_FAILED(rv = aTemplate->ChildAt(loop, *getter_AddRefs(aTemplateKid))))
			continue;
		PRInt32 nameSpaceID;
		if (NS_FAILED(rv = aTemplateKid->GetNameSpaceID(nameSpaceID)))
			continue;

		// check whether this item is a containment item for aValue 
		PRBool		isContainmentElement = PR_FALSE;
		if (nameSpaceID == kNameSpaceID_XUL)
		{
			nsAutoString	idValue;
			if (NS_SUCCEEDED(rv = aTemplateKid->GetAttribute(kNameSpaceID_None,
	                              kURIAtom, idValue)) && (rv == NS_CONTENT_ATTR_HAS_VALUE))
			{
				if (idValue.EqualsIgnoreCase("..."))
				{
					isContainmentElement = PR_TRUE;
					isUnique=PR_FALSE;
				}
			}
		}

		nsCOMPtr<nsIAtom>	tag;
		if (NS_SUCCEEDED(rv = aTemplateKid->GetTag(*getter_AddRefs(tag))))
		{
			nsCOMPtr<nsIContent>	treeGrandchild;
			if (isUnique == PR_TRUE)
			{
				if (NS_FAILED(rv = EnsureElementHasGenericChild(treeChildren,
					nameSpaceID, tag, getter_AddRefs(treeGrandchild))))
					continue;
			}
			else if (isContainmentElement == PR_TRUE)
			{
				if (NS_FAILED(rv = CreateElement(nameSpaceID,
					tag, aValue, getter_AddRefs(treeGrandchild))))
					continue;
				if (NS_FAILED(rv = SetAllAttributesOnElement(treeChildren,
					treeGrandchild, aValue)))
					continue;

				// save a reference (its ID) to the template node that was used
				nsAutoString	templateID("");
				aTemplateKid->GetAttribute(kNameSpaceID_None, kIdAtom, templateID);
				treeGrandchild->SetAttribute(kNameSpaceID_None, kTemplateAtom, templateID, PR_FALSE);
			}
			else if ((tag.get() == kTextAtom) && (nameSpaceID == kNameSpaceID_XUL))
			{
				// <xul:text rdf:resource="..."> is replaced by text of the
				// actual value of the rdf:resource attribute for the given node
				nsAutoString	attrValue;
				if (NS_FAILED(rv = aTemplateKid->GetAttribute(kNameSpaceID_RDF,
                                              kResourceAtom, attrValue)))
					continue;
				if (rv != NS_CONTENT_ATTR_HAS_VALUE)
					continue;
				if (attrValue.Length() < 1)
					continue;
				char	*prop = attrValue.ToNewCString();
				if (nsnull == prop)
					continue;

				nsCOMPtr<nsIRDFResource>	propRes;
				if (NS_FAILED(rv = gRDFService->GetResource(prop, getter_AddRefs(propRes))))
					continue;

				nsCOMPtr<nsIRDFNode>	aResult;
				if (NS_FAILED(rv = mDB->GetTarget(aValue,
					propRes, PR_TRUE, getter_AddRefs(aResult))))
					continue;
				rv = nsRDFContentUtils::AttachTextNode(treeChildren, aResult);
				delete [] prop;
				prop = nsnull;
			}
			else
			{
				if (nameSpaceID == kNameSpaceID_HTML)
				{
					nsIHTMLElementFactory	*mHTMLElementFactory;

					rv = nsComponentManager::CreateInstance(kHTMLElementFactoryCID,
						nsnull, kIHTMLElementFactoryIID, (void**) &mHTMLElementFactory);
					if (NS_FAILED(rv))
						continue;

					nsCOMPtr<nsIHTMLContent>	element;
					nsAutoString			tagName(tag->GetUnicode());
					rv = mHTMLElementFactory->CreateInstanceByTag(tagName, getter_AddRefs(element));
					if (NS_FAILED(rv))
						continue;

					nsCOMPtr<nsIDocument>		doc;
					if (NS_FAILED(rv = mDocument->QueryInterface(kIDocumentIID, getter_AddRefs(doc))))
						continue;

					// XXX To do
					// Make sure our ID is set. Unlike XUL elements, we want to make sure
					// that our ID is relative if possible.
					
					if (NS_FAILED(rv = element->SetDocument(doc, PR_FALSE)))
						continue;

					treeGrandchild = do_QueryInterface(element);

					NS_RELEASE(mHTMLElementFactory);
				}
				else
				{
					if (NS_FAILED(rv = NS_NewRDFElement(nameSpaceID, tag,
								getter_AddRefs(treeGrandchild))))
						continue;

					nsCOMPtr<nsIDocument>	doc = do_QueryInterface(mDocument);
					if (! doc)
						continue;

					treeGrandchild->SetDocument(doc, PR_FALSE);
				}
			}

			if (treeGrandchild && treeGrandchild.get())
			{
				// copy all attributes from template to new node
				PRInt32		numAttribs;
				if (NS_SUCCEEDED(rv = aTemplateKid->GetAttributeCount(numAttribs)) &&
						(rv == NS_CONTENT_ATTR_HAS_VALUE))
				{
					for (PRInt32 aLoop=0; aLoop<numAttribs; aLoop++)
					{
						PRInt32			attribNameSpaceID;
						nsCOMPtr<nsIAtom>	attribName;
						if (NS_SUCCEEDED(rv = aTemplateKid->GetAttributeNameAt(aLoop,
							attribNameSpaceID, *getter_AddRefs(attribName))))
						{
							// only copy attributes that aren't already set on the node
							nsAutoString	val;
							if (NS_SUCCEEDED(rv = treeGrandchild->GetAttribute(attribNameSpaceID,
								attribName, val)))
							{
								if (rv == NS_CONTENT_ATTR_HAS_VALUE)	continue;
							}
							// never copy rdf:container attribute
							if ((attribName.get() == kContainerAtom) &&
								(attribNameSpaceID == kNameSpaceID_RDF))
								continue;
							// never copy rdf:property attribute
							else if ((attribName.get() == kPropertyAtom) &&
								(attribNameSpaceID == kNameSpaceID_RDF))
								continue;
							// never copy rdf:instanceOf attribute
							else if ((attribName.get() == kInstanceOfAtom) &&
								(attribNameSpaceID == kNameSpaceID_RDF))
								continue;
							// never copy {}:rootcontainment attribute
							else if ((attribName.get() == kRootcontainmentAtom) &&
								(attribNameSpaceID == kNameSpaceID_None))
								continue;
							// never copy {}:subcontainment attribute
							else if ((attribName.get() == kSubcontainmentAtom) &&
								(attribNameSpaceID == kNameSpaceID_None))
								continue;
							// never copy {}:ID attribute
							else if ((attribName.get() == kIdAtom) &&
								(attribNameSpaceID == kNameSpaceID_None))
								continue;
							// never copy {}:uri attribute
							else if ((attribName.get() == kURIAtom) &&
								(attribNameSpaceID == kNameSpaceID_None))
								continue;
							// never copy {}:itemcontentsgenerated attribute (bogus)
							else if ((attribName.get() == kItemContentsGeneratedAtom) &&
								(attribNameSpaceID == kNameSpaceID_None))
								continue;
							// never copy {}:treecontentsgenerated attribute (bogus)
							else if ((attribName.get() == kTreeContentsGeneratedAtom) &&
								(attribNameSpaceID == kNameSpaceID_None))
								continue;

							nsAutoString	attribValue;
							if (NS_SUCCEEDED(rv = aTemplateKid->GetAttribute(attribNameSpaceID,
									attribName,attribValue)) && (rv == NS_CONTENT_ATTR_HAS_VALUE))
							{
								if (attribValue.Find("rdf:") == 0)
								{
									// found an attribute which wants to bind its value
									// to RDF so look it up in the graph
									attribValue.Cut(0,4);
									char *prop = attribValue.ToNewCString();
									if (nsnull == prop)	continue;
									attribValue = "";

									nsCOMPtr<nsIRDFResource>	propRes;
									rv = gRDFService->GetResource(prop, getter_AddRefs(propRes));
									delete [] prop;
									if (NS_FAILED(rv))	continue;

									nsCOMPtr<nsIRDFNode>		valueNode;
									if (NS_FAILED(rv = mDB->GetTarget(aValue, propRes, PR_TRUE,
										getter_AddRefs(valueNode))) || (rv == NS_RDF_NO_VALUE))
										continue;
									nsCOMPtr<nsIRDFLiteral>	literalValue = do_QueryInterface(valueNode);
									if (!literalValue)
										continue;
									PRUnichar	*uniVal;
									if (NS_FAILED(rv = literalValue->GetValue(&uniVal)))
										continue;
									attribValue = uniVal;
								}
								else if (attribValue.EqualsIgnoreCase("..."))
								{
									char	*uri = nsnull;
									aValue->GetValue(&uri);
									attribValue=uri;
								}
								treeGrandchild->SetAttribute(attribNameSpaceID,
									attribName, attribValue, PR_FALSE);
							}
						}
					}
				}
				// set natural order hint
				if ((aNaturalOrderPos > 0) && (isContainmentElement == PR_TRUE))
				{
					nsAutoString	pos, zero("0000");
					pos.Append(aNaturalOrderPos, 10);
					if (pos.Length() < 4)
					{
					    pos.Insert(zero, 0, 4-pos.Length()); 
					}
					treeGrandchild->SetAttribute(kNameSpaceID_None, kNaturalOrderPosAtom, pos, PR_FALSE);
				}

				// recurse into template subtree
				PRInt32		numTemplateKids;
				if (NS_SUCCEEDED(rv = aTemplateKid->ChildCount(numTemplateKids)))
				{
					if (numTemplateKids > 0)
					{
						rv = PopulateWidgetItemSubtree(aTemplateRoot, aTemplateKid, treeGrandchild,
							aElement, isUnique, aProperty, aValue, aNaturalOrderPos);
					}
				}

				// Note: add into tree, but only sort if its a containment element!
				if ((nsnull != XULSortService) && (isContainmentElement == PR_TRUE))
				{
					if (NS_FAILED(rv = XULSortService->InsertContainerNode(treeChildren, treeGrandchild)))
					{
						treeChildren->AppendChildTo(treeGrandchild, PR_TRUE);
					}
				}
				else
				{
					treeChildren->AppendChildTo(treeGrandchild, PR_TRUE);
				}

				// If item says its "open", then recurve now and build up its children
				nsAutoString	openState;
				if (NS_SUCCEEDED(rv = treeGrandchild->GetAttribute(kNameSpaceID_None, kOpenAtom, openState))
					 && (rv == NS_CONTENT_ATTR_HAS_VALUE) && (openState.EqualsIgnoreCase("true")))
				{
					OpenWidgetItem(treeGrandchild);
				}
			}
		}
	}
	return(rv);
}


NS_IMETHODIMP
RDFGenericBuilderImpl::CreateWidgetItem(nsIContent *aElement, nsIRDFResource *aProperty,
					nsIRDFResource *aValue, PRInt32 aNaturalOrderPos)
{
	nsCOMPtr<nsIContent>	aTemplate;
	nsresult		rv;

	if (NS_SUCCEEDED(rv = FindTemplateForResource(aValue, getter_AddRefs(aTemplate))) &&
		(aTemplate))
	{
		nsCOMPtr<nsIContent>	children = do_QueryInterface(aElement);
		PRBool			isRoot = PR_FALSE;

		// if template specifies a rootcontainment attribute, get it
		// and determine if we are building children off of the root
		nsAutoString	rootAttrValue;
		if (NS_SUCCEEDED(rv = aTemplate->GetAttribute(kNameSpaceID_None,
                              kRootcontainmentAtom, rootAttrValue)) && (rv == NS_CONTENT_ATTR_HAS_VALUE))
		{
			nsCOMPtr<nsIAtom>	rootAtom = NS_NewAtom(rootAttrValue);
			if (rootAtom)
			{
				PRInt32 nameSpaceID;
				if (NS_SUCCEEDED(rv = aElement->GetNameSpaceID(nameSpaceID)))
				{
					if (nameSpaceID == kNameSpaceID_XUL)
					{
						nsCOMPtr<nsIAtom>	tag;
						if (NS_SUCCEEDED(rv = aElement->GetTag(*getter_AddRefs(tag))))
						{
							if (tag == rootAtom)
							{
								isRoot = PR_TRUE;
							}
						}
					}
				}
			}
		}

		// if template specifies a subcontainment attribute, get it
		// and make sure aElement contains it (if not the root),
		// otherwise use aElement
		if (isRoot == PR_FALSE)
		{
			nsAutoString	attrValue;
			if (NS_SUCCEEDED(rv = aTemplate->GetAttribute(kNameSpaceID_None,
	                              kSubcontainmentAtom, attrValue)) && (rv == NS_CONTENT_ATTR_HAS_VALUE))
			{
				nsCOMPtr<nsIAtom>	childrenAtom = NS_NewAtom(attrValue);
				if (childrenAtom)
				{
					if (NS_SUCCEEDED(rv = EnsureElementHasGenericChild(aElement,
						kNameSpaceID_XUL, childrenAtom, getter_AddRefs(children))))
					{
					}
				}
			}
		}
		rv = PopulateWidgetItemSubtree(aTemplate, aTemplate, children, aElement,
				PR_TRUE, aProperty, aValue, aNaturalOrderPos);
	}
#ifdef FALLBACK_BUILDERS
	else
	{
		rv = AddWidgetItem(aElement, aProperty, aValue, aNaturalOrderPos);
	}
#endif

	return(rv);
}



nsresult
RDFGenericBuilderImpl::UpdateWidgetItemAttribute(nsIContent* aTemplateNode,
                                                 nsIContent* aTreeItemElement,
                                                 eUpdateAction aAction,
                                                 nsIRDFResource* aProperty,
                                                 nsIRDFNode* aValue)
{
	nsresult		rv = NS_OK;
	PRInt32			nameSpaceID;
	nsCOMPtr<nsIAtom>	tag;
	rv = mDocument->SplitProperty(aProperty, &nameSpaceID, getter_AddRefs(tag));
	if (NS_FAILED(rv))	return(rv);

#define ALL_PROPERTIES_AS_ATTRIBUTES
#ifdef ALL_PROPERTIES_AS_ATTRIBUTES
	// check whether this item is a containment item for aValue 
	nsAutoString	idValue;
	if (NS_SUCCEEDED(rv = aTemplateNode->GetAttribute(kNameSpaceID_None,
                      kURIAtom, idValue)) && (rv == NS_CONTENT_ATTR_HAS_VALUE))
	{
		if (idValue.EqualsIgnoreCase("..."))
		{
			// no matter what, it's also been set as an attribute.
			nsAutoString		idNodeValue("");
			if (NS_SUCCEEDED(rv = nsRDFContentUtils::GetTextForNode(aValue, idNodeValue)))
			{
                if (aAction == eSet) {
					rv = aTreeItemElement->SetAttribute(nameSpaceID, tag, idNodeValue, PR_TRUE);
                }
                else {
					rv = aTreeItemElement->UnsetAttribute(nameSpaceID, tag, PR_TRUE);
                }
			}
		}
	}
#endif

	// check all attributes on the template node; if they reference a resource,
	// update the equivalent attribute on the content node
	PRInt32		numAttribs;
	if (NS_SUCCEEDED(rv = aTemplateNode->GetAttributeCount(numAttribs)) &&
			(rv == NS_CONTENT_ATTR_HAS_VALUE))
	{
		for (PRInt32 aLoop=0; aLoop<numAttribs; aLoop++)
		{
			PRInt32			attribNameSpaceID;
			nsCOMPtr<nsIAtom>	attribName;
			if (NS_SUCCEEDED(rv = aTemplateNode->GetAttributeNameAt(aLoop,
				attribNameSpaceID, *getter_AddRefs(attribName))))
			{
				nsAutoString	attribValue;
				if (NS_SUCCEEDED(rv = aTemplateNode->GetAttribute(attribNameSpaceID,
						attribName,attribValue)) && (rv == NS_CONTENT_ATTR_HAS_VALUE))
				{
					if (attribValue.Find("rdf:") == 0)
					{
						// found an attribute which wants to bind its value
						// to RDF so look it up in the graph
						attribValue.Cut(0,4);
						char	*prop = attribValue.ToNewCString();
						if (nsnull == prop)	continue;
						nsCOMPtr<nsIRDFResource>	propRes;
						rv = gRDFService->GetResource(prop, getter_AddRefs(propRes));
						delete [] prop;
						if (NS_FAILED(rv))	continue;

						if (propRes.get() == aProperty)
						{
							nsAutoString		text("");
							if (NS_SUCCEEDED(rv = nsRDFContentUtils::GetTextForNode(aValue, text)))
							{
								if ((text.Length() > 0) && (aAction == eSet))
								{
									aTreeItemElement->SetAttribute(attribNameSpaceID,
										attribName, text, PR_TRUE);
								}
								else
								{
									aTreeItemElement->UnsetAttribute(attribNameSpaceID,
										attribName, PR_TRUE);
								}
							}
						}
					}
				}
			}
		}
	}

	// sync up template kids with content kids
	PRInt32			count;
	if (NS_FAILED(rv = aTemplateNode->ChildCount(count)))	return(rv);
	for (PRInt32 loop=0; loop<count; loop++)
	{
		nsCOMPtr<nsIContent>	aTemplateKid;
		if (NS_FAILED(rv = aTemplateNode->ChildAt(loop, *getter_AddRefs(aTemplateKid))))
			continue;
		nsCOMPtr<nsIContent>	aTreeItemKid;
		if (NS_FAILED(rv = aTreeItemElement->ChildAt(loop, *getter_AddRefs(aTreeItemKid))))
			continue;
		rv = UpdateWidgetItemAttribute(aTemplateKid, aTreeItemKid, aAction, aProperty, aValue);
	}
	return(rv);
}


NS_IMETHODIMP
RDFGenericBuilderImpl::OnAssert(nsIRDFResource* aSource,
                                nsIRDFResource* aProperty,
                                nsIRDFNode* aTarget)
{
	// Just silently fail, because this can happen "normally" as part
	// of tear-down code. (Bug 9098)
    if (! mDocument)
        return NS_OK;

    nsresult rv;

    nsCOMPtr<nsISupportsArray> elements;
    rv = NS_NewISupportsArray(getter_AddRefs(elements));
	NS_ASSERTION(NS_SUCCEEDED(rv), "unable to create new ISupportsArray");
	if (NS_FAILED(rv)) return rv;

    // Find all the elements in the content model that correspond to
    // aSource: for each, we'll try to build XUL children if
    // appropriate.
    rv = mDocument->GetElementsForResource(aSource, elements);
	NS_ASSERTION(NS_SUCCEEDED(rv), "unable to retrieve elements from resource");
	if (NS_FAILED(rv)) return rv;

    PRUint32 cnt;
    rv = elements->Count(&cnt);
    if (NS_FAILED(rv)) return rv;

    for (PRInt32 i = PRInt32(cnt) - 1; i >= 0; --i) {
		nsISupports* isupports = elements->ElementAt(i);
        nsCOMPtr<nsIContent> element( do_QueryInterface(isupports) );
		NS_IF_RELEASE(isupports);

        // XXX somehow figure out if building XUL kids on this
        // particular element makes any sense whatsoever.

        // We'll start by making sure that the element at least has
        // the same parent has the content model builder's root
        if (!IsElementInWidget(element))
            continue;
        
        nsCOMPtr<nsIRDFResource> resource = do_QueryInterface(aTarget);
        if (resource && IsContainmentProperty(element, aProperty)) {
            // Okay, the target  _is_ a resource, and the property is
            // a containment property. So this'll be a new item in the widget
            // control.

            // But if the contents of aElement _haven't_ yet been
            // generated, then just ignore the assertion. We do this
            // because we know that _eventually_ the contents will be
            // generated (via CreateContents()) when somebody asks for
            // them later.
            nsAutoString contentsGenerated;
            rv = element->GetAttribute(kNameSpaceID_None,
                                       kItemContentsGeneratedAtom,
                                       contentsGenerated);
			if (NS_FAILED(rv)) return rv;

            if ((rv != NS_CONTENT_ATTR_HAS_VALUE) || !contentsGenerated.EqualsIgnoreCase("true"))
                return NS_OK;

            // Okay, it's a "live" element, so go ahead and append the new
            // child to this node.
            rv = CreateWidgetItem(element, aProperty, resource, 0);
            NS_ASSERTION(NS_SUCCEEDED(rv), "unable to create widget item");
            if (NS_FAILED(rv)) return rv;
        }
        else {
            // Either the target of the assertion is not a resource,
            // or the object is a resource and the predicate is not a
            // containment property. So this won't be a new item in
            // the widget. See if we can use it to set a some
            // substructure on the current element.
            nsAutoString templateID;
            rv = element->GetAttribute(kNameSpaceID_None,
                                       kTemplateAtom,
                                       templateID);
            if (NS_FAILED(rv)) return rv;

            if (rv == NS_CONTENT_ATTR_HAS_VALUE) {
                nsCOMPtr<nsIDOMXULDocument>	xulDoc;
                xulDoc = do_QueryInterface(mDocument);
                if (! xulDoc)
                    return NS_ERROR_UNEXPECTED;

                nsCOMPtr<nsIDOMElement>	domElement;
                rv = xulDoc->GetElementById(templateID, getter_AddRefs(domElement));
                NS_ASSERTION(NS_SUCCEEDED(rv), "unable to find template node");
                if (NS_FAILED(rv)) return rv;

                nsCOMPtr<nsIContent> templateNode = do_QueryInterface(domElement);
                if (! templateNode)
                    return NS_ERROR_UNEXPECTED;

                // this node was created by a XUL template, so update it accordingly
                rv = UpdateWidgetItemAttribute(templateNode, element, eSet, aProperty, aTarget);
                if (NS_FAILED(rv)) return rv;
            }
#ifdef FALLBACK_BUILDERS
            else {
                // fall through if node wasn't created by a XUL template
                rv = SetWidgetAttribute(element, aProperty, aTarget);
                if (NS_FAILED(rv)) return rv;
            }
#endif
        }
    }
    return NS_OK;
}

NS_IMETHODIMP
RDFGenericBuilderImpl::OnUnassert(nsIRDFResource* aSource,
                                  nsIRDFResource* aProperty,
                                  nsIRDFNode* aTarget)
{
	// Just silently fail, because this can happen "normally" as part
	// of tear-down code. (Bug 9098)
    if (! mDocument)
        return NS_OK;

    nsresult rv;

    nsCOMPtr<nsISupportsArray> elements;
    rv = NS_NewISupportsArray(getter_AddRefs(elements));
	NS_ASSERTION(NS_SUCCEEDED(rv), "unable to create new ISupportsArray");
	if (NS_FAILED(rv)) return rv;

    // Find all the elements in the content model that correspond to
    // aSource: for each, we'll try to build XUL children if
    // appropriate.
    rv = mDocument->GetElementsForResource(aSource, elements);
	NS_ASSERTION(NS_SUCCEEDED(rv), "unable to retrieve elements from resource");
	if (NS_FAILED(rv)) return rv;

    PRUint32 cnt;
    rv = elements->Count(&cnt);
    if (NS_FAILED(rv)) return rv;

    for (PRInt32 i = cnt - 1; i >= 0; --i) {
		nsISupports* isupports = elements->ElementAt(i);
        nsCOMPtr<nsIContent> element( do_QueryInterface(isupports) );
		NS_IF_RELEASE(isupports);
        
        // XXX somehow figure out if building XUL kids on this
        // particular element makes any sense whatsoever.

        // We'll start by making sure that the element at least has
        // the same parent has the content model builder's root
        if (!IsElementInWidget(element))
            continue;
        
        nsCOMPtr<nsIRDFResource> resource = do_QueryInterface(aTarget);
        if (resource && IsContainmentProperty(element, aProperty)) {
            // Okay, the object _is_ a resource, and the predicate is
            // a containment property. So we'll need to remove this
            // item from the widget.

            // But if the contents of aElement _haven't_ yet been
            // generated, then just ignore the unassertion: nothing is
            // in the content model to remove.
            nsAutoString contentsGenerated;
            rv = element->GetAttribute(kNameSpaceID_None,
                                       kItemContentsGeneratedAtom,
                                       contentsGenerated);
            if (NS_FAILED(rv)) return rv;

            if ((rv != NS_CONTENT_ATTR_HAS_VALUE) || !contentsGenerated.EqualsIgnoreCase("true"))
                return NS_OK;

            // Okay, it's a "live" element, so go ahead and remove the
            // child from this node.
            rv = RemoveWidgetItem(element, aProperty, resource);
            NS_ASSERTION(NS_SUCCEEDED(rv), "unable to remove widget item");
            if (NS_FAILED(rv)) return rv;
        }
        else {
            // Either the target of the assertion is not a resource,
            // or the target is a resource and the property is not a
            // containment property. So this won't be an item in the
            // widget. See if we need to tear down some substructure
            // on the current element.
            nsAutoString templateID;
            rv = element->GetAttribute(kNameSpaceID_None,
                                       kTemplateAtom,
                                       templateID);
            if (NS_FAILED(rv)) return rv;

            if (rv == NS_CONTENT_ATTR_HAS_VALUE) {
				nsCOMPtr<nsIDOMXULDocument>	xulDoc;
				xulDoc = do_QueryInterface(mDocument);
                if (! xulDoc)
                    return NS_ERROR_UNEXPECTED;

				nsCOMPtr<nsIDOMElement>	domElement;
                rv = xulDoc->GetElementById(templateID, getter_AddRefs(domElement));
                NS_ASSERTION(NS_SUCCEEDED(rv), "unable to find template node");
                if (NS_FAILED(rv)) return rv;

				nsCOMPtr<nsIContent> templateNode = do_QueryInterface(domElement);
                if (! templateNode)
                    return NS_ERROR_UNEXPECTED;

                // this node was created by a XUL template, so update it accordingly
                rv = UpdateWidgetItemAttribute(templateNode, element, eClear, aProperty, aTarget);
                if (NS_FAILED(rv)) return rv;
            }
#ifdef FALLBACK_BUILDERS
            else {
                rv = UnsetWidgetAttribute(element, aProperty, aTarget);
                if (NS_FAILED(rv)) return rv;
            }
#endif
        }
    }
    return NS_OK;
}


NS_IMETHODIMP
RDFGenericBuilderImpl::OnChange(nsIRDFResource* aSource,
								nsIRDFResource* aProperty,
								nsIRDFNode* aOldTarget,
								nsIRDFNode* aNewTarget)
{
	// Just silently fail, because this can happen "normally" as part
	// of tear-down code. (Bug 9098)
    if (! mDocument)
        return NS_OK;

    nsresult rv;

    nsCOMPtr<nsISupportsArray> elements;
    rv = NS_NewISupportsArray(getter_AddRefs(elements));
	NS_ASSERTION(NS_SUCCEEDED(rv), "unable to create new ISupportsArray");
	if (NS_FAILED(rv)) return rv;

    // Find all the elements in the content model that correspond to
    // aSource: for each, we'll try to build XUL children if
    // appropriate.
    rv = mDocument->GetElementsForResource(aSource, elements);
	NS_ASSERTION(NS_SUCCEEDED(rv), "unable to retrieve elements from resource");
	if (NS_FAILED(rv)) return rv;

    PRUint32 cnt;
    rv = elements->Count(&cnt);
    if (NS_FAILED(rv)) return rv;

    for (PRInt32 i = cnt - 1; i >= 0; --i) {
		nsISupports* isupports = elements->ElementAt(i);
        nsCOMPtr<nsIContent> element( do_QueryInterface(isupports) );
		NS_IF_RELEASE(isupports);
        
        // XXX somehow figure out if building XUL kids on this
        // particular element makes any sense whatsoever.

        // We'll start by making sure that the element at least has
        // the same parent has the content model builder's root
        if (!IsElementInWidget(element))
            continue;
        
        nsCOMPtr<nsIRDFResource> oldresource = do_QueryInterface(aOldTarget);
        if (oldresource && IsContainmentProperty(element, aProperty)) {
            // Okay, the object _is_ a resource, and the predicate is
            // a containment property. So we'll need to remove the old
            // item from the widget, and then add the new one.

            // But if the contents of aElement _haven't_ yet been
            // generated, then just ignore: nothing is in the content
            // model to remove.
            nsAutoString contentsGenerated;
            rv = element->GetAttribute(kNameSpaceID_None,
                                       kItemContentsGeneratedAtom,
                                       contentsGenerated);
            if (NS_FAILED(rv)) return rv;

            if ((rv != NS_CONTENT_ATTR_HAS_VALUE) || !contentsGenerated.EqualsIgnoreCase("true"))
                return NS_OK;

            // Okay, it's a "live" element, so go ahead and remove the
            // child from this node.
            rv = RemoveWidgetItem(element, aProperty, oldresource);
            NS_ASSERTION(NS_SUCCEEDED(rv), "unable to remove widget item");
            if (NS_FAILED(rv)) return rv;

            nsCOMPtr<nsIRDFResource> newresource = do_QueryInterface(aNewTarget);

            // XXX Tough. We don't handle the case where they've
            // changed a resource to a literal. But this is
            // bizarre anyway.
            if (! newresource)
                return NS_OK;

            rv = CreateWidgetItem(element, aProperty, newresource, 0);
            if (NS_FAILED(rv)) return rv;
        }
        else {
            // Either the target of the assertion is not a resource,
            // or the target is a resource and the property is not a
            // containment property. So this won't be an item in the
            // widget. See if we need to tear down some substructure
            // on the current element.
            nsAutoString templateID;
            rv = element->GetAttribute(kNameSpaceID_None,
                                       kTemplateAtom,
                                       templateID);
            if (NS_FAILED(rv)) return rv;

            if (rv == NS_CONTENT_ATTR_HAS_VALUE) {
				nsCOMPtr<nsIDOMXULDocument>	xulDoc;
				xulDoc = do_QueryInterface(mDocument);
                if (! xulDoc)
                    return NS_ERROR_UNEXPECTED;

				nsCOMPtr<nsIDOMElement>	domElement;
                rv = xulDoc->GetElementById(templateID, getter_AddRefs(domElement));
                NS_ASSERTION(NS_SUCCEEDED(rv), "unable to find template node");
                if (NS_FAILED(rv)) return rv;

				nsCOMPtr<nsIContent> templateNode = do_QueryInterface(domElement);
                if (! templateNode)
                    return NS_ERROR_UNEXPECTED;

                // this node was created by a XUL template, so update it accordingly
                rv = UpdateWidgetItemAttribute(templateNode, element, eSet, aProperty, aNewTarget);
                if (NS_FAILED(rv)) return rv;
            }
#ifdef FALLBACK_BUILDERS
            else {
                rv = SetWidgetAttribute(element, aProperty, aNewTarget);
                if (NS_FAILED(rv)) return rv;
            }
#endif
        }
    }
    return NS_OK;
}


NS_IMETHODIMP
RDFGenericBuilderImpl::OnMove(nsIRDFResource* aOldSource,
							  nsIRDFResource* aNewSource,
							  nsIRDFResource* aProperty,
							  nsIRDFNode* aTarget)
{
	NS_NOTYETIMPLEMENTED("write me");
	return NS_ERROR_NOT_IMPLEMENTED;
}


////////////////////////////////////////////////////////////////////////
// nsIDOMNodeObserver interface
//
//   XXX Any of these methods that can't be implemented in a generic
//   way should become pure virtual on this class.
//

NS_IMETHODIMP
RDFGenericBuilderImpl::OnSetNodeValue(nsIDOMNode* aNode, const nsString& aValue)
{
    NS_NOTYETIMPLEMENTED("write me!");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
RDFGenericBuilderImpl::OnInsertBefore(nsIDOMNode* aParent, nsIDOMNode* aNewChild, nsIDOMNode* aRefChild)
{
    return NS_OK;
}



NS_IMETHODIMP
RDFGenericBuilderImpl::OnReplaceChild(nsIDOMNode* aParent, nsIDOMNode* aNewChild, nsIDOMNode* aOldChild)
{
    return NS_OK;
}



NS_IMETHODIMP
RDFGenericBuilderImpl::OnRemoveChild(nsIDOMNode* aParent, nsIDOMNode* aOldChild)
{
    return NS_OK;
}



NS_IMETHODIMP
RDFGenericBuilderImpl::OnAppendChild(nsIDOMNode* aParent, nsIDOMNode* aNewChild)
{
    return NS_OK;
}


////////////////////////////////////////////////////////////////////////
// nsIDOMElementObserver interface


NS_IMETHODIMP
RDFGenericBuilderImpl::OnSetAttribute(nsIDOMElement* aElement, const nsString& aName, const nsString& aValue)
{
    nsresult rv;

    nsCOMPtr<nsIRDFResource> resource;
    if (NS_FAILED(rv = GetDOMNodeResource(aElement, getter_AddRefs(resource)))) {
        // XXX it's not a resource element, so there's no assertions
        // we need to make on the back-end. Should we just do the
        // update?
        return NS_OK;
    }

    // Get the nsIContent interface, it's a bit more utilitarian
    nsCOMPtr<nsIContent> element( do_QueryInterface(aElement) );
    if (! element) {
        NS_ERROR("element doesn't support nsIContent");
        return NS_ERROR_UNEXPECTED;
    }

    // Make sure that the element is in the widget. XXX Even this may be
    // a bit too promiscuous: an element may also be a XUL element...
    if (!IsElementInWidget(element))
        return NS_OK;

    // Split the element into its namespace and tag components
    PRInt32  elementNameSpaceID;
    if (NS_FAILED(rv = element->GetNameSpaceID(elementNameSpaceID))) {
        NS_ERROR("unable to get element namespace ID");
        return rv;
    }

    nsCOMPtr<nsIAtom> elementNameAtom;
    if (NS_FAILED(rv = element->GetTag( *getter_AddRefs(elementNameAtom) ))) {
        NS_ERROR("unable to get element tag");
        return rv;
    }

    // Split the property name into its namespace and tag components
    PRInt32  attrNameSpaceID;
    nsCOMPtr<nsIAtom> attrNameAtom;
    if (NS_FAILED(rv = element->ParseAttributeString(aName, *getter_AddRefs(attrNameAtom), attrNameSpaceID))) {
        NS_ERROR("unable to parse attribute string");
        return rv;
    }

    // Now do the work to change the attribute. There are a couple of
    // special cases that we need to check for here...
    if ((elementNameSpaceID == kNameSpaceID_XUL) &&
        IsItemOrFolder(element) && // XXX IsItemOrFolder(): is this what we really mean?
        (attrNameSpaceID    == kNameSpaceID_None) &&
        (attrNameAtom.get() == kOpenAtom)) {

        // We are (possibly) changing the value of the "open"
        // attribute. This may require us to generate or destroy
        // content in the widget. See what the old value was...
        nsAutoString attrValue;
        if (NS_FAILED(rv = element->GetAttribute(kNameSpaceID_None, kOpenAtom, attrValue))) {
            NS_ERROR("unable to get current open state");
            return rv;
        }

        if ((rv == NS_CONTENT_ATTR_NO_VALUE) || (rv == NS_CONTENT_ATTR_NOT_THERE) ||
            ((rv == NS_CONTENT_ATTR_HAS_VALUE) && (! attrValue.EqualsIgnoreCase(aValue))) ||
            PR_TRUE // XXX just always allow this to fire.
            ) {
            // Okay, it's really changing.

            // This is a "transient" property, so we _don't_ go to the
            // RDF graph to set it.
            if (NS_FAILED(rv = element->SetAttribute(kNameSpaceID_None, kOpenAtom, aValue, PR_TRUE))) {
                NS_ERROR("unable to update attribute on content node");
                return rv;
            }

            if (aValue.EqualsIgnoreCase("true")) {
                rv = OpenWidgetItem(element);
            }
            else {
                rv = CloseWidgetItem(element);
            }

            NS_ASSERTION(NS_SUCCEEDED(rv), "unable to open/close tree item");
            return rv;
        }
    }
    else if ((elementNameSpaceID == kNameSpaceID_XUL) &&
             (IsItemOrFolder(element) || // XXX IsItemOrFolder(): is this what we really mean?
              IsWidgetInsertionRootElement(element)) &&
             (attrNameSpaceID    == kNameSpaceID_None) &&
             (attrNameAtom.get() == kIdAtom)) {

        // We are (possibly) changing the actual identity of the
        // element; e.g., re-rooting an item in the tree.

        nsCOMPtr<nsIRDFResource> newResource;
        if (NS_FAILED(rv = gRDFService->GetUnicodeResource(aValue.GetUnicode(), getter_AddRefs(newResource)))) {
            NS_ERROR("unable to get new resource");
            return rv;
        }

#if 0 // XXX we're fighting with the XUL builder, so just _always_ let this through.
        // Didn't change. So bail!
        if (resource == newResource)
            return NS_OK;
#endif

        // Allright, it really is changing. So blow away the old
        // content node and insert a new one with the new ID in
        // its place.
        nsCOMPtr<nsIContent> parent;
        if (NS_FAILED(rv = element->GetParent(*getter_AddRefs(parent)))) {
            NS_ERROR("unable to get element's parent");
            return rv;
        }

        PRInt32 elementIndex;
        if (NS_FAILED(rv = parent->IndexOf(element, elementIndex))) {
            NS_ERROR("unable to get element's index within parent");
            return rv;
        }

        if (NS_FAILED(rv = parent->RemoveChildAt(elementIndex, PR_TRUE))) {
            NS_ERROR("unable to remove element");
            return rv;
        }

        nsCOMPtr<nsIContent> newElement;
        if (NS_FAILED(rv = CreateElement(elementNameSpaceID,
                                         elementNameAtom,
                                         newResource,
                                         getter_AddRefs(newElement)))) {
            NS_ERROR("unable to create new element");
            return rv;
        }

        // Attach transient properties to the new element.
        //
        // XXX all I really care about right this minute is the
        // "open" state. We could put this stuff in a table and
        // drive it that way.
        nsAutoString attrValue;
        if (NS_FAILED(rv = element->GetAttribute(kNameSpaceID_None, kOpenAtom, attrValue))) {
            NS_ERROR("unable to get open state of old element");
            return rv;
        }

        if (rv == NS_CONTENT_ATTR_HAS_VALUE) {
            if (NS_FAILED(rv = newElement->SetAttribute(kNameSpaceID_None, kOpenAtom, attrValue, PR_FALSE))) {
                NS_ERROR("unable to set open state of new element");
                return rv;
            }
        }

        // Mark as a container so the contents get regenerated
        if (NS_FAILED(rv = newElement->SetAttribute(kNameSpaceID_RDF,
                                                    kContainerAtom,
                                                    "true",
                                                    PR_FALSE))) {
            NS_ERROR("unable to mark as a container");
            return rv;
        }

        // Now insert the new element into the parent. This should
        // trigger a reflow and cause the contents to be regenerated.
        if (NS_FAILED(rv = parent->InsertChildAt(newElement, elementIndex, PR_TRUE))) {
            NS_ERROR("unable to add new element to the parent");
            return rv;
        }

        return NS_OK;
    }

    // If we get here, it's a "vanilla" property: push its value into the graph.
    if (kNameSpaceID_Unknown == attrNameSpaceID) {
      attrNameSpaceID = kNameSpaceID_None;  // ignore unknown prefix XXX is this correct?
    }
    nsCOMPtr<nsIRDFResource> property;
    if (NS_FAILED(rv = GetResource(attrNameSpaceID, attrNameAtom, getter_AddRefs(property)))) {
        NS_ERROR("unable to construct resource");
        return rv;
    }

    // Unassert the old value, if there was one.
    nsAutoString oldValue;
    if (NS_CONTENT_ATTR_HAS_VALUE == element->GetAttribute(attrNameSpaceID, attrNameAtom, oldValue)) {
        nsCOMPtr<nsIRDFLiteral> value;
        if (NS_FAILED(rv = gRDFService->GetLiteral(oldValue.GetUnicode(), getter_AddRefs(value)))) {
            NS_ERROR("unable to construct literal");
            return rv;
        }

        rv = mDB->Unassert(resource, property, value);
        NS_ASSERTION(NS_SUCCEEDED(rv), "unable to unassert old property value");
    }

    // Assert the new value
    {
        nsCOMPtr<nsIRDFLiteral> value;
        if (NS_FAILED(rv = gRDFService->GetLiteral(aValue.GetUnicode(), getter_AddRefs(value)))) {
            NS_ERROR("unable to construct literal");
            return rv;
        }

        rv = mDB->Assert(resource, property, value, PR_TRUE);
        NS_ASSERTION(rv == NS_RDF_ASSERTION_ACCEPTED, "unable to assert new property value");
        if (NS_FAILED(rv)) return rv;
    }

    return NS_OK;
}

NS_IMETHODIMP
RDFGenericBuilderImpl::OnRemoveAttribute(nsIDOMElement* aElement, const nsString& aName)
{
    nsresult rv;

    nsCOMPtr<nsIRDFResource> resource;
    if (NS_FAILED(rv = GetDOMNodeResource(aElement, getter_AddRefs(resource)))) {
        // XXX it's not a resource element, so there's no assertions
        // we need to make on the back-end. Should we just do the
        // update?
        return NS_OK;
    }

    // Get the nsIContent interface, it's a bit more utilitarian
    nsCOMPtr<nsIContent> element( do_QueryInterface(aElement) );
    if (! element) {
        NS_ERROR("element doesn't support nsIContent");
        return NS_ERROR_UNEXPECTED;
    }

    // Make sure that the element is in the widget. XXX Even this may be
    // a bit too promiscuous: an element may also be a XUL element...
    if (!IsElementInWidget(element))
        return NS_OK;

    // Split the element into its namespace and tag components
    PRInt32  elementNameSpaceID;
    if (NS_FAILED(rv = element->GetNameSpaceID(elementNameSpaceID))) {
        NS_ERROR("unable to get element namespace ID");
        return rv;
    }

    nsCOMPtr<nsIAtom> elementNameAtom;
    if (NS_FAILED(rv = element->GetTag( *getter_AddRefs(elementNameAtom) ))) {
        NS_ERROR("unable to get element tag");
        return rv;
    }

    // Split the property name into its namespace and tag components
    PRInt32  attrNameSpaceID;
    nsCOMPtr<nsIAtom> attrNameAtom;
    if (NS_FAILED(rv = element->ParseAttributeString(aName, *getter_AddRefs(attrNameAtom), attrNameSpaceID))) {
        NS_ERROR("unable to parse attribute string");
        return rv;
    }

    if ((elementNameSpaceID    == kNameSpaceID_XUL) &&
        IsItemOrFolder(element) && // XXX Is this what we really mean?
        (attrNameSpaceID       == kNameSpaceID_None) &&
        (attrNameAtom.get()    == kOpenAtom)) {
        // We are removing the value of the "open" attribute. This may
        // require us to destroy content from the tree.

        // XXX should we check for existence of the attribute first?
        if (NS_FAILED(rv = element->UnsetAttribute(kNameSpaceID_None, kOpenAtom, PR_TRUE))) {
            NS_ERROR("unable to attribute on update content node");
            return rv;
        }

        if (NS_FAILED(rv = CloseWidgetItem(element))) {
            NS_ERROR("unable to close widget item");
            return rv;
        }
    }
    else {
        // It's a "vanilla" property: push its value into the graph.

        nsCOMPtr<nsIRDFResource> property;
        if (kNameSpaceID_Unknown == attrNameSpaceID) {
          attrNameSpaceID = kNameSpaceID_None;  // ignore unknown prefix XXX is this correct?
        }
        if (NS_FAILED(rv = GetResource(attrNameSpaceID, attrNameAtom, getter_AddRefs(property)))) {
            NS_ERROR("unable to construct resource");
            return rv;
        }

        // Unassert the old value, if there was one.
        nsAutoString oldValue;
        if (NS_CONTENT_ATTR_HAS_VALUE == element->GetAttribute(attrNameSpaceID, attrNameAtom, oldValue)) {
            nsCOMPtr<nsIRDFLiteral> value;
            if (NS_FAILED(rv = gRDFService->GetLiteral(oldValue.GetUnicode(), getter_AddRefs(value)))) {
                NS_ERROR("unable to construct literal");
                return rv;
            }

            rv = mDB->Unassert(resource, property, value);
            NS_ASSERTION(NS_SUCCEEDED(rv), "unable to unassert old property value");
        }
    }
    return NS_OK;
}

NS_IMETHODIMP
RDFGenericBuilderImpl::OnSetAttributeNode(nsIDOMElement* aElement, nsIDOMAttr* aNewAttr)
{
    NS_NOTYETIMPLEMENTED("write me!");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
RDFGenericBuilderImpl::OnRemoveAttributeNode(nsIDOMElement* aElement, nsIDOMAttr* aOldAttr)
{
    NS_NOTYETIMPLEMENTED("write me!");
    return NS_ERROR_NOT_IMPLEMENTED;
}


////////////////////////////////////////////////////////////////////////
// Implementation methods

nsresult
RDFGenericBuilderImpl::EnsureElementHasGenericChild(nsIContent* parent,
                                                 PRInt32 nameSpaceID,
                                                 nsIAtom* tag,
                                                 nsIContent** result)
{
    nsresult rv;

    rv = nsRDFContentUtils::FindChildByTag(parent, nameSpaceID, tag, result);
    if (NS_FAILED(rv))
        return rv;

    if (rv == NS_RDF_NO_VALUE) {
        // we need to construct a new child element.
        nsCOMPtr<nsIContent> element;

        if (NS_FAILED(rv = NS_NewRDFElement(nameSpaceID, tag, getter_AddRefs(element))))
            return rv;

        if (NS_FAILED(rv = parent->AppendChildTo(element, PR_TRUE))) 
          // XXX Note that the notification ensures we won't batch insertions! This could be bad! - Dave

            return rv;

        *result = element;
        NS_ADDREF(*result);
    }
    return NS_OK;
}


PRBool
RDFGenericBuilderImpl::IsItemOrFolder(nsIContent* aElement)
{
    // XXX It seems like this should be a pure virtual method that
    // subclasses implement?

    // XXX All of the callers of this method actually only seem to
    // care if the element is a "folder".

    // Returns PR_TRUE if the element is an "item" or a "folder" in
    // the widget.
    nsresult rv;

    nsCOMPtr<nsIAtom> itemAtom;
    nsCOMPtr<nsIAtom> folderAtom;
    if (NS_FAILED(rv = GetWidgetItemAtom(getter_AddRefs(itemAtom))) ||
        NS_FAILED(rv = GetWidgetFolderAtom(getter_AddRefs(folderAtom)))) {
        return rv;
    }

    // "element" must be itemAtom
    nsCOMPtr<nsIAtom> tag;
    if (NS_FAILED(rv = aElement->GetTag(*getter_AddRefs(tag))))
        return PR_FALSE;

    if (tag != itemAtom && tag != folderAtom)
        return PR_FALSE;

    return PR_TRUE;
}

PRBool
RDFGenericBuilderImpl::IsWidgetInsertionRootElement(nsIContent* element)
{
    // Returns PR_TRUE if the element is the root point to insert new items.
    nsresult rv;

    nsCOMPtr<nsIAtom> rootAtom;
    if (NS_FAILED(rv = GetInsertionRootAtom(getter_AddRefs(rootAtom)))) {
        return rv;
    }

    PRInt32 nameSpaceID;
    if (NS_FAILED(rv = element->GetNameSpaceID(nameSpaceID))) {
        NS_ERROR("unable to get namespace ID");
        return PR_FALSE;
    }

    if (nameSpaceID != kNameSpaceID_XUL)
        return PR_FALSE; // not a XUL element

    nsCOMPtr<nsIAtom> elementTag;
    if (NS_FAILED(rv = element->GetTag(*getter_AddRefs(elementTag)))) {
        NS_ERROR("unable to get tag");
        return PR_FALSE;
    }

    if (elementTag != rootAtom)
        return PR_FALSE; // not the place to insert a child

    return PR_TRUE;
}

PRBool
RDFGenericBuilderImpl::IsContainmentProperty(nsIContent* aElement, nsIRDFResource* aProperty)
{
    // XXX is this okay to _always_ treat ordinal properties as tree
    // properties? Probably not...
    nsresult rv;

    PRBool isOrdinal;
    rv = gRDFContainerUtils->IsOrdinalProperty(aProperty, &isOrdinal);
    if (NS_FAILED(rv))
        return PR_FALSE;

    if (isOrdinal)
        return PR_TRUE;

    nsXPIDLCString propertyURI;
    if (NS_FAILED(rv = aProperty->GetValue( getter_Copies(propertyURI) ))) {
        NS_ERROR("unable to get property URI");
        return PR_FALSE;
    }

    nsAutoString uri;

    // Walk up the content tree looking for the "rdf:containment"
    // attribute, so we can determine if the specified property
    // actually defines containment.
    nsCOMPtr<nsIContent> element( dont_QueryInterface(aElement) );
    while (element) {
        // Never ever ask an HTML element about non-HTML attributes.
        PRInt32 nameSpaceID;
        if (NS_FAILED(rv = element->GetNameSpaceID(nameSpaceID))) {
            NS_ERROR("unable to get element namespace");
            return PR_FALSE;
        }

        if (nameSpaceID != kNameSpaceID_HTML) {
            // Is this the "containment" property?
            if (NS_FAILED(rv = element->GetAttribute(kNameSpaceID_None, kContainmentAtom, uri))) {
                NS_ERROR("unable to get attribute value");
                return PR_FALSE;
            }

            if (rv == NS_CONTENT_ATTR_HAS_VALUE) {
                // Okay we've found the locally-scoped tree properties,
                // a whitespace-separated list of property URIs. So we
                // definitively know whether this is a tree property or
                // not.
                if (uri.Find(propertyURI) >= 0)
                    return PR_TRUE;
                else
                    return PR_FALSE;
            }
        }

        nsCOMPtr<nsIContent> parent;
        element->GetParent(*getter_AddRefs(parent));
        element = parent;
    }

    // If we get here, we didn't find any tree property: so now
    // defaults start to kick in.

#define TREE_PROPERTY_HACK
#if defined(TREE_PROPERTY_HACK)
    if ((aProperty == kNC_child) ||
        (aProperty == kNC_Folder) ||
        (aProperty == kRDF_child)) {
        return PR_TRUE;
    }
    else
#endif // defined(TREE_PROPERTY_HACK)

        return PR_FALSE;
}


PRBool
RDFGenericBuilderImpl::IsIgnoredProperty(nsIContent* aElement, nsIRDFResource* aProperty)
{
    nsresult rv;

    nsXPIDLCString propertyURI;
    rv = aProperty->GetValue( getter_Copies(propertyURI) );
    if (NS_FAILED(rv)) return rv;

    nsAutoString uri;

    // Walk up the content tree looking for the "rdf:ignore"
    // attribute, so we can determine if the specified property should
    // be ignored.
    nsCOMPtr<nsIContent> element( dont_QueryInterface(aElement) );
    while (element) {
        PRInt32 nameSpaceID;
        rv = element->GetNameSpaceID(nameSpaceID);
        if (NS_FAILED(rv)) return rv;

        // Never ever ask an HTML element about non-HTML attributes
        if (nameSpaceID != kNameSpaceID_HTML) {
            rv = element->GetAttribute(kNameSpaceID_None, kIgnoreAtom, uri);
            if (NS_FAILED(rv)) return rv;

            if (rv == NS_CONTENT_ATTR_HAS_VALUE) {
                // Okay, we've found the locally-scoped ignore
                // properties, which is a whitespace-separated list of
                // property URIs. So we definitively know whether this
                // property should be ignored or not.
                if (uri.Find(propertyURI) >= 0)
                    return PR_TRUE;
                else
                    return PR_FALSE;
            }
        }

        nsCOMPtr<nsIContent> parent;
        element->GetParent(*getter_AddRefs(parent));
        element = parent;
    }

    // Walked _all_ the way to the top and couldn't find anything to
    // ignore.
    return PR_FALSE;
}

PRBool
RDFGenericBuilderImpl::IsContainer(nsIContent* aElement, nsIRDFResource* aResource)
{
    // Look at all of the arcs extending _out_ of the resource: if any
    // of them are that "containment" property, then we know we'll
    // have children.

    nsCOMPtr<nsISimpleEnumerator> arcs;
    nsresult rv;

    rv = mDB->ArcLabelsOut(aResource, getter_AddRefs(arcs));
    NS_ASSERTION(NS_SUCCEEDED(rv), "unable to get arcs out");
    if (NS_FAILED(rv))
        return PR_FALSE;

    while (1) {
        PRBool hasMore;
        rv = arcs->HasMoreElements(&hasMore);
        NS_ASSERTION(NS_SUCCEEDED(rv), "severe error advancing cursor");
        if (NS_FAILED(rv))
            return PR_FALSE;

        if (! hasMore)
            break;

        nsCOMPtr<nsISupports> isupports;
        rv = arcs->GetNext(getter_AddRefs(isupports));
        if (NS_FAILED(rv))
            return PR_FALSE;

        nsCOMPtr<nsIRDFResource> property = do_QueryInterface(isupports);

        if (! IsContainmentProperty(aElement, property))
            continue;

        // see if it has a value
        nsCOMPtr<nsIRDFNode> value;
        rv = mDB->GetTarget(aResource, property, PR_TRUE, getter_AddRefs(value));
        if (NS_FAILED(rv))
            return PR_FALSE;

        if (value)
            return PR_TRUE;
    }

    return PR_FALSE;
}

PRBool
RDFGenericBuilderImpl::IsOpen(nsIContent* aElement)
{
    nsresult rv;

    // needs to be a the valid insertion root or an item to begin with.
    PRInt32 nameSpaceID;
    if (NS_FAILED(rv = aElement->GetNameSpaceID(nameSpaceID))) {
        NS_ERROR("unable to get namespace ID");
        return PR_FALSE;
    }

    if (nameSpaceID != kNameSpaceID_XUL)
        return PR_FALSE;

    if (aElement == mRoot)
      return PR_TRUE;

    nsAutoString value;
    if (NS_FAILED(rv = aElement->GetAttribute(kNameSpaceID_None, kOpenAtom, value))) {
        NS_ERROR("unable to get open attribute");
        return rv;
    }

    if (rv == NS_CONTENT_ATTR_HAS_VALUE) {
        if (value.EqualsIgnoreCase("true"))
            return PR_TRUE;
    }

	
    return PR_FALSE;
}


PRBool
RDFGenericBuilderImpl::IsElementInWidget(nsIContent* aElement)
{
    // Make sure that we're actually creating content for the tree
    // content model that we've been assigned to deal with.

    // Walk up the parent chain from us to the root and
    // see what we find.
    if (aElement == mRoot)
      return PR_TRUE;

    // walk up the tree until you find rootAtom
    nsCOMPtr<nsIContent> element(do_QueryInterface(aElement));
    nsCOMPtr<nsIContent> parent;
    element->GetParent(*getter_AddRefs(parent));
    element = parent;
    
    while (element) {
        
        if (element.get() == mRoot)
          return PR_TRUE;

        // up to the parent...
//        nsCOMPtr<nsIContent> parent;
        element->GetParent(*getter_AddRefs(parent));
        element = parent;
    }
    
    return PR_FALSE;
}

nsresult
RDFGenericBuilderImpl::GetDOMNodeResource(nsIDOMNode* aNode, nsIRDFResource** aResource)
{
    nsresult rv;

    // Given an nsIDOMNode that presumably has been created as a proxy
    // for an RDF resource, pull the RDF resource information out of
    // it.

    nsCOMPtr<nsIContent> element;
    if (NS_FAILED(rv = aNode->QueryInterface(kIContentIID, getter_AddRefs(element) ))) {
        NS_ERROR("DOM element doesn't support nsIContent");
        return rv;
    }

    return nsRDFContentUtils::GetElementRefResource(element, aResource);
}


nsresult
RDFGenericBuilderImpl::GetResource(PRInt32 aNameSpaceID,
                                   nsIAtom* aNameAtom,
                                   nsIRDFResource** aResource)
{
    NS_PRECONDITION(aNameAtom != nsnull, "null ptr");
    if (! aNameAtom)
        return NS_ERROR_NULL_POINTER;

    // XXX should we allow nodes with no namespace???
    NS_PRECONDITION(aNameSpaceID != kNameSpaceID_Unknown, "no namespace");
    if (aNameSpaceID == kNameSpaceID_Unknown)
        return NS_ERROR_UNEXPECTED;

    // construct a fully-qualified URI from the namespace/tag pair.
    nsAutoString uri;
    gNameSpaceManager->GetNameSpaceURI(aNameSpaceID, uri);

    // XXX check to see if we need to insert a '/' or a '#'
    nsAutoString tag(aNameAtom->GetUnicode());
    if (0 < uri.Length() && uri.Last() != '#' && uri.Last() != '/' && tag.First() != '#')
        uri.Append('#');

    uri.Append(tag);

    nsresult rv = gRDFService->GetUnicodeResource(uri.GetUnicode(), aResource);
    NS_ASSERTION(NS_SUCCEEDED(rv), "unable to get resource");
    return rv;
}


nsresult
RDFGenericBuilderImpl::OpenWidgetItem(nsIContent* aElement)
{
    return CreateContents(aElement);
}


nsresult
RDFGenericBuilderImpl::CloseWidgetItem(nsIContent* aElement)
{
    nsresult rv;

    // Find the tag that contains the children so that we can remove all of
    // the children.
    nsCOMPtr<nsIAtom> parentAtom;
    if (NS_FAILED(rv = GetItemAtomThatContainsTheChildren(getter_AddRefs(parentAtom)))) {
        return rv;
    }

    nsCOMPtr<nsIContent> parentNode;
    nsCOMPtr<nsIAtom> tag;
    if (NS_FAILED(rv = aElement->GetTag(*getter_AddRefs(tag))))
        return rv; // XXX fatal

    if (tag == parentAtom) {
        parentNode = dont_QueryInterface(aElement);
        rv = NS_OK;
    }
    else {
        rv = nsRDFContentUtils::FindChildByTag(aElement, kNameSpaceID_XUL, parentAtom, getter_AddRefs(parentNode));
    }

    if (rv == NS_RDF_NO_VALUE) {
        // No tag; must've already been closed
        return NS_OK;
    }

    if (NS_FAILED(rv)) {
        NS_ERROR("severe error retrieving parent node for removal");
        return rv;
    }

    PRInt32 count;
    if (NS_FAILED(rv = parentNode->ChildCount(count))) {
        NS_ERROR("unable to get count of the parent's children");
        return rv;
    }

    while (--count >= 0) {
        // XXX This is a bit gross. We need to recursively remove any
        // subtrees before we remove the current subtree.
        nsIContent* child;
        if (NS_SUCCEEDED(rv = parentNode->ChildAt(count, child))) {
            rv = CloseWidgetItem(child);
            NS_RELEASE(child);
        }

        rv = parentNode->RemoveChildAt(count, PR_TRUE);
        NS_ASSERTION(NS_SUCCEEDED(rv), "error removing child");
    }

    // Clear the contents-generated attribute so that the next time we
    // come back, we'll regenerate the kids we just killed.
    rv = aElement->UnsetAttribute(kNameSpaceID_None,
								  kItemContentsGeneratedAtom,
								  PR_FALSE);
	if (NS_FAILED(rv)) return rv;

	// This is a _total_ hack to make sure that any XUL we blow away
	// gets rebuilt.
	rv = aElement->UnsetAttribute(kNameSpaceID_None,
								  kXULContentsGeneratedAtom,
								  PR_FALSE);
	if (NS_FAILED(rv)) return rv;

	rv = aElement->SetAttribute(kNameSpaceID_RDF,
								kContainerAtom,
								"true",
								PR_FALSE);
	if (NS_FAILED(rv)) return rv;

    return NS_OK;
}


