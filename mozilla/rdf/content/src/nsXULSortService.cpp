/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8; c-file-style: "stroustrup" -*-
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

/*
  This file provides the implementation for the sort service manager.
 */

#include "nsCOMPtr.h"
#include "nsCRT.h"
#include "nsIAtom.h"
#include "nsIContent.h"
#include "nsIDOMElement.h"
#include "nsIDOMNode.h"
#include "nsIDocument.h"
#include "nsINameSpaceManager.h"
#include "nsIRDFContentModelBuilder.h"
#include "nsIRDFCompositeDataSource.h"
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
#include "nsIXULContentUtils.h"
#include "nsString.h"
#include "nsXPIDLString.h"
#include "rdf.h"
#include "rdfutil.h"

#include "nsVoidArray.h"
#include "nsQuickSort.h"
#include "nsIAtom.h"
#include "nsIXULSortService.h"
#include "nsString.h"
#include "plhash.h"
#include "plstr.h"
#include "prlong.h"
#include "prlog.h"

#include "nsICollation.h"
#include "nsCollationCID.h"

#include "nsCOMPtr.h"
#include "nsIServiceManager.h"
#include "nsINameSpaceManager.h"
#include "nsLayoutCID.h"
#include "nsRDFCID.h"

#include "nsIContent.h"
#include "nsIDOMText.h"

#include "nsVoidArray.h"

#include "nsIDOMNode.h"
#include "nsIDOMElement.h"

#include "nsIRDFContentModelBuilder.h"
#include "nsIRDFCompositeDataSource.h"
#include "nsIRDFNode.h"
#include "nsIRDFService.h"
#include "rdf.h"

#include "nsIDOMXULElement.h"

#include "nsILocale.h"
#include "nsLocaleCID.h"
#include "nsILocaleFactory.h"

#define	XUL_BINARY_INSERTION_SORT	1


////////////////////////////////////////////////////////////////////////

static NS_DEFINE_IID(kXULSortServiceCID,      NS_XULSORTSERVICE_CID);
static NS_DEFINE_IID(kIXULSortServiceIID,     NS_IXULSORTSERVICE_IID);
static NS_DEFINE_IID(kISupportsIID,           NS_ISUPPORTS_IID);

static NS_DEFINE_CID(kNameSpaceManagerCID,    NS_NAMESPACEMANAGER_CID);
static NS_DEFINE_IID(kINameSpaceManagerIID,   NS_INAMESPACEMANAGER_IID);

static NS_DEFINE_IID(kIContentIID,            NS_ICONTENT_IID);
static NS_DEFINE_IID(kIDOMTextIID,            NS_IDOMTEXT_IID);

static NS_DEFINE_IID(kIDomNodeIID,            NS_IDOMNODE_IID);
static NS_DEFINE_IID(kIDomElementIID,         NS_IDOMELEMENT_IID);

static NS_DEFINE_CID(kRDFServiceCID,          NS_RDFSERVICE_CID);
static NS_DEFINE_IID(kIRDFServiceIID,         NS_IRDFSERVICE_IID);
static NS_DEFINE_IID(kIRDFResourceIID,        NS_IRDFRESOURCE_IID);
static NS_DEFINE_IID(kIRDFLiteralIID,         NS_IRDFLITERAL_IID);

static NS_DEFINE_CID(kXULContentUtilsCID,     NS_XULCONTENTUTILS_CID);

static NS_DEFINE_IID(kIDomXulElementIID,      NS_IDOMXULELEMENT_IID);

static NS_DEFINE_CID(kCollationFactoryCID,    NS_COLLATIONFACTORY_CID);
static NS_DEFINE_IID(kICollationFactoryIID,   NS_ICOLLATIONFACTORY_IID);

static NS_DEFINE_CID(kRDFInMemoryDataSourceCID, NS_RDFINMEMORYDATASOURCE_CID);

static NS_DEFINE_CID(kLocaleFactoryCID, NS_LOCALEFACTORY_CID);
static NS_DEFINE_IID(kILocaleFactoryIID, NS_ILOCALEFACTORY_IID);
static NS_DEFINE_CID(kLocaleCID, NS_LOCALE_CID);
static NS_DEFINE_IID(kILocaleIID, NS_ILOCALE_IID);

// XXX This is sure to change. Copied from mozilla/layout/xul/content/src/nsXULAtoms.cpp
static const char kXULNameSpaceURI[]
    = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";
DEFINE_RDF_VOCAB(NC_NAMESPACE_URI, NC, Name);
DEFINE_RDF_VOCAB(NC_NAMESPACE_URI, NC, BookmarkSeparator);


typedef	struct	_sortStruct	{
	nsCOMPtr<nsIRDFResource>		sortProperty;
	nsCOMPtr<nsIRDFResource>		sortProperty2;
	nsCOMPtr<nsIRDFCompositeDataSource>	db;
	nsCOMPtr<nsIRDFService>			rdfService;
	nsCOMPtr<nsIRDFDataSource>		mInner;
	nsCOMPtr<nsISupportsArray>		resCache;
	nsCOMPtr<nsIAtom>			kNaturalOrderPosAtom;
	nsCOMPtr<nsIAtom>			kTreeCellAtom;
	PRInt32					colIndex;
	PRInt32					kNameSpaceID_XUL;
	PRBool					descendingSort;
	PRBool					naturalOrderSort;
} sortStruct, *sortPtr;



int		openSortCallback(const void *data1, const void *data2, void *privateData);
int		inplaceSortCallback(const void *data1, const void *data2, void *privateData);



////////////////////////////////////////////////////////////////////////
// ServiceImpl
//
//   This is the sort service.
//
class XULSortServiceImpl : public nsIXULSortService
{
protected:
				XULSortServiceImpl(void);
	virtual			~XULSortServiceImpl(void);

	static nsICollation	*collationService;

	friend nsresult		NS_NewXULSortService(nsIXULSortService** mgr);

private:
	static nsrefcnt		gRefCnt;
	static nsIAtom		*kTreeAtom;
	static nsIAtom		*kTreeBodyAtom;
	static nsIAtom		*kTreeCellAtom;
	static nsIAtom		*kTreeChildrenAtom;
	static nsIAtom		*kTreeColAtom;
	static nsIAtom		*kTreeItemAtom;
	static nsIAtom		*kResourceAtom;
	static nsIAtom		*kResource2Atom;
	static nsIAtom		*kNameAtom;
	static nsIAtom		*kSortAtom;
	static nsIAtom		*kSortDirectionAtom;
	static nsIAtom		*kIdAtom;
	static nsIAtom		*kNaturalOrderPosAtom;
	static nsIAtom		*kRDF_type;
	static nsIAtom		*kURIAtom;

    static nsIRDFResource	*kNC_Name;
    static nsIRDFResource	*kRDF_instanceOf;
    static nsIRDFResource	*kRDF_Seq;

    static PRInt32	kNameSpaceID_XUL;
    static PRInt32	kNameSpaceID_RDF;

    static nsIRDFService	*gRDFService;

    static nsIXULContentUtils   *gXULUtils;

nsresult	FindTreeElement(nsIContent* aElement,nsIContent** aTreeElement);
nsresult	FindTreeChildrenElement(nsIContent *tree, nsIContent **treeBody);
nsresult	GetSortColumnIndex(nsIContent *tree, const nsString&sortResource, const nsString& sortDirection, PRInt32 *colIndex);
nsresult	GetSortColumnInfo(nsIContent *tree, nsString &sortResource, nsString &sortDirection, nsString &sortResource2);
nsresult	GetTreeCell(nsIContent *node, PRInt32 colIndex, nsIContent **cell);
nsresult	GetTreeCellValue(nsIContent *node, nsString & value);
nsresult	RemoveAllChildren(nsIContent *node);
nsresult	SortTreeChildren(nsIContent *container, PRInt32 colIndex, sortPtr sortInfo);
nsresult	DoSort(nsIDOMNode* node, const nsString& sortResource, const nsString& sortDirection);

static nsresult	GetCachedTarget(sortPtr sortInfo, nsIRDFResource* aSource, nsIRDFResource *aProperty, PRBool aTruthValue, nsIRDFNode **aResult);
static nsresult	GetCachedResource(sortPtr sortInfo, nsIRDFResource *sortProperty, const char *suffix, nsIRDFResource **res);
static nsresult	GetResourceValue(nsIRDFResource *res1, nsIRDFResource *sortProperty, sortPtr sortInfo, nsIRDFNode **, PRBool &isCollationKey);
static nsresult	GetNodeValue(nsIContent *node1, nsIRDFResource *sortProperty, sortPtr sortInfo, nsIRDFNode **, PRBool &isCollationKey);
static nsresult	GetTreeCell(sortPtr sortInfo, nsIContent *node, PRInt32 cellIndex, nsIContent **cell);
static nsresult	GetTreeCellValue(sortPtr sortInfo, nsIContent *node, nsString & val);

public:
    static nsresult	InplaceSort(nsIContent *node1, nsIContent *node2, sortPtr sortInfo, PRInt32 & sortOrder);
    static nsresult	OpenSort(nsIRDFNode *node1, nsIRDFNode *node2, sortPtr sortInfo, PRInt32 & sortOrder);
    static nsresult	CompareNodes(nsIRDFNode *cellNode1, PRBool isCollationKey1,
				 nsIRDFNode *cellNode2, PRBool isCollationKey2,
				 PRInt32 & sortOrder);

    // nsISupports
    NS_DECL_ISUPPORTS

    // nsISortService
    NS_DECL_NSIXULSORTSERVICE
};



nsICollation		*XULSortServiceImpl::collationService = nsnull;
nsIRDFService		*XULSortServiceImpl::gRDFService = nsnull;
nsrefcnt XULSortServiceImpl::gRefCnt = 0;

nsIXULContentUtils      *XULSortServiceImpl::gXULUtils = nsnull;

nsIAtom* XULSortServiceImpl::kTreeAtom;
nsIAtom* XULSortServiceImpl::kTreeBodyAtom;
nsIAtom* XULSortServiceImpl::kTreeCellAtom;
nsIAtom* XULSortServiceImpl::kTreeChildrenAtom;
nsIAtom* XULSortServiceImpl::kTreeColAtom;
nsIAtom* XULSortServiceImpl::kTreeItemAtom;
nsIAtom* XULSortServiceImpl::kResourceAtom;
nsIAtom* XULSortServiceImpl::kResource2Atom;
nsIAtom* XULSortServiceImpl::kNameAtom;
nsIAtom* XULSortServiceImpl::kSortAtom;
nsIAtom* XULSortServiceImpl::kSortDirectionAtom;
nsIAtom* XULSortServiceImpl::kIdAtom;
nsIAtom* XULSortServiceImpl::kNaturalOrderPosAtom;
nsIAtom* XULSortServiceImpl::kRDF_type;
nsIAtom* XULSortServiceImpl::kURIAtom;

nsIRDFResource		*XULSortServiceImpl::kNC_Name;
nsIRDFResource		*XULSortServiceImpl::kRDF_instanceOf;
nsIRDFResource		*XULSortServiceImpl::kRDF_Seq;

PRInt32  XULSortServiceImpl::kNameSpaceID_XUL;
PRInt32  XULSortServiceImpl::kNameSpaceID_RDF;

////////////////////////////////////////////////////////////////////////


XULSortServiceImpl::XULSortServiceImpl(void)
{
	NS_INIT_REFCNT();
	if (gRefCnt == 0)
	{
	        kTreeAtom            		= NS_NewAtom("tree");
	        kTreeBodyAtom        		= NS_NewAtom("treebody");
	        kTreeCellAtom        		= NS_NewAtom("treecell");
		kTreeChildrenAtom    		= NS_NewAtom("treechildren");
		kTreeColAtom         		= NS_NewAtom("treecol");
		kTreeItemAtom        		= NS_NewAtom("treeitem");
		kResourceAtom        		= NS_NewAtom("resource");
		kResource2Atom        		= NS_NewAtom("resource2");
		kNameAtom			= NS_NewAtom("Name");
		kSortAtom			= NS_NewAtom("sortActive");
		kSortDirectionAtom		= NS_NewAtom("sortDirection");
		kIdAtom				= NS_NewAtom("id");
		kNaturalOrderPosAtom		= NS_NewAtom("pos");
		kRDF_type			= NS_NewAtom("type");
		kURIAtom			= NS_NewAtom("uri");
 
		nsresult rv;

		if (NS_FAILED(rv = nsServiceManager::GetService(kRDFServiceCID,
						  kIRDFServiceIID, (nsISupports**) &gRDFService)))
		{
			NS_ERROR("couldn't create rdf service");
		}


		rv = nsServiceManager::GetService(kXULContentUtilsCID,
						  nsCOMTypeInfo<nsIXULContentUtils>::GetIID(),
						  (nsISupports**) &gXULUtils);
		NS_ASSERTION(NS_SUCCEEDED(rv), "unable to get XUL content utils");

		// get a locale factory 
		nsCOMPtr<nsIFactory>		aFactory;
		nsCOMPtr<nsILocaleFactory>	localeFactory;
		if (NS_SUCCEEDED(rv = nsComponentManager::FindFactory(kLocaleFactoryCID, getter_AddRefs(aFactory)))
			&& ((localeFactory = do_QueryInterface(aFactory)) != nsnull))
		{
			nsCOMPtr<nsILocale>	locale;
			if (NS_SUCCEEDED(rv = localeFactory->GetApplicationLocale(getter_AddRefs(locale))) && (locale))
			{
				nsCOMPtr<nsICollationFactory>	colFactory;
				if (NS_SUCCEEDED(rv = nsComponentManager::CreateInstance(kCollationFactoryCID, NULL,
						kICollationFactoryIID, getter_AddRefs(colFactory))))
				{
					if (NS_FAILED(rv = colFactory->CreateCollation(locale, &collationService)))
					{
						NS_ERROR("couldn't create collation instance");
					}
				}
				else
				{
					NS_ERROR("couldn't create instance of collation factory");
				}
			}
			else
			{
				NS_ERROR("unable to get application locale");
			}
		}
		else
		{
			NS_ERROR("couldn't get locale factory");
		}

		gRDFService->GetResource(kURINC_Name, 				&kNC_Name);
		gRDFService->GetResource(RDF_NAMESPACE_URI "instanceOf",	&kRDF_instanceOf);
		gRDFService->GetResource(RDF_NAMESPACE_URI "Seq",		&kRDF_Seq);

	        // Register the XUL and RDF namespaces: these'll just retrieve
	        // the IDs if they've already been registered by someone else.
		nsINameSpaceManager* mgr;
		if (NS_SUCCEEDED(rv = nsComponentManager::CreateInstance(kNameSpaceManagerCID,
		                           nsnull,
		                           kINameSpaceManagerIID,
		                           (void**) &mgr)))
		{
			static const char kRDFNameSpaceURI[] = RDF_NAMESPACE_URI;

			rv = mgr->RegisterNameSpace(kXULNameSpaceURI, kNameSpaceID_XUL);
			NS_ASSERTION(NS_SUCCEEDED(rv), "unable to register XUL namespace");

			rv = mgr->RegisterNameSpace(kRDFNameSpaceURI, kNameSpaceID_RDF);
			NS_ASSERTION(NS_SUCCEEDED(rv), "unable to register RDF namespace");

			NS_RELEASE(mgr);
		}
		else
		{
			NS_ERROR("couldn't create namepsace manager");
		}
	}
	++gRefCnt;
}



XULSortServiceImpl::~XULSortServiceImpl(void)
{
#ifdef DEBUG_REFS
    --gInstanceCount;
    fprintf(stdout, "%d - RDF: XULSortServiceImpl\n", gInstanceCount);
#endif

	--gRefCnt;
	if (gRefCnt == 0)
	{
		NS_IF_RELEASE(kTreeAtom);
		NS_IF_RELEASE(kTreeBodyAtom);
		NS_IF_RELEASE(kTreeCellAtom);
	        NS_IF_RELEASE(kTreeChildrenAtom);
	        NS_IF_RELEASE(kTreeColAtom);
	        NS_IF_RELEASE(kTreeItemAtom);
	        NS_IF_RELEASE(kResourceAtom);
	        NS_IF_RELEASE(kResource2Atom);
	        NS_IF_RELEASE(kNameAtom);
	        NS_IF_RELEASE(kSortAtom);
	        NS_IF_RELEASE(kSortDirectionAtom);
	        NS_IF_RELEASE(kIdAtom);
	        NS_IF_RELEASE(kNaturalOrderPosAtom);
		NS_IF_RELEASE(kRDF_type);
		NS_IF_RELEASE(kURIAtom);
	        NS_IF_RELEASE(kNC_Name);
	        NS_IF_RELEASE(kRDF_instanceOf);
	        NS_IF_RELEASE(kRDF_Seq);

		NS_IF_RELEASE(collationService);
		collationService = nsnull;

		if (gRDFService) {
		    nsServiceManager::ReleaseService(kRDFServiceCID, gRDFService);
		    gRDFService = nsnull;
		}

		if (gXULUtils) {
		    nsServiceManager::ReleaseService(kXULContentUtilsCID, gXULUtils);
		    gXULUtils = nsnull;
		}
	}
}



NS_IMPL_ISUPPORTS(XULSortServiceImpl, nsIXULSortService::GetIID());



////////////////////////////////////////////////////////////////////////



nsresult
XULSortServiceImpl::FindTreeElement(nsIContent *aElement, nsIContent **aTreeElement)
{
	nsresult rv;
	nsCOMPtr<nsIContent> element(do_QueryInterface(aElement));

	while (element)
	{
		PRInt32 nameSpaceID;
		if (NS_FAILED(rv = element->GetNameSpaceID(nameSpaceID)))	return rv;
		if (nameSpaceID == kNameSpaceID_XUL)
		{
			nsCOMPtr<nsIAtom> tag;
			if (NS_FAILED(rv = element->GetTag(*getter_AddRefs(tag))))	return rv;
			if (tag.get() == kTreeAtom)
			{
				*aTreeElement = element;
				NS_ADDREF(*aTreeElement);
				return NS_OK;
			}
		}
		nsCOMPtr<nsIContent> parent;
		element->GetParent(*getter_AddRefs(parent));
		element = parent;
	}
	return(NS_ERROR_FAILURE);
}



nsresult
XULSortServiceImpl::FindTreeChildrenElement(nsIContent *tree, nsIContent **treeBody)
{
        nsCOMPtr<nsIContent>	child;
	PRInt32			childIndex = 0, numChildren = 0, nameSpaceID;
	nsresult		rv;

	if (NS_FAILED(rv = tree->ChildCount(numChildren)))	return(rv);
	for (childIndex=0; childIndex<numChildren; childIndex++)
	{
		if (NS_FAILED(rv = tree->ChildAt(childIndex, *getter_AddRefs(child))))	return(rv);
		if (NS_FAILED(rv = child->GetNameSpaceID(nameSpaceID)))	return rv;
		if (nameSpaceID == kNameSpaceID_XUL)
		{
			nsCOMPtr<nsIAtom> tag;
			if (NS_FAILED(rv = child->GetTag(*getter_AddRefs(tag))))	return rv;
			if (tag.get() == kTreeChildrenAtom)
			{
				*treeBody = child;
				NS_ADDREF(*treeBody);
				return NS_OK;
			}
		}
	}
	return(NS_ERROR_FAILURE);
}



nsresult
XULSortServiceImpl::GetSortColumnIndex(nsIContent *tree, const nsString& sortResource, const nsString& sortDirection, PRInt32 *sortColIndex)
{
	PRBool			found = PR_FALSE;
	PRInt32			childIndex, colIndex = 0, numChildren, nameSpaceID;
        nsCOMPtr<nsIContent>	child;
	nsresult		rv;

	*sortColIndex = 0;
	if (NS_FAILED(rv = tree->ChildCount(numChildren)))	return(rv);
	for (childIndex=0; childIndex<numChildren; childIndex++)
	{
		if (NS_FAILED(rv = tree->ChildAt(childIndex, *getter_AddRefs(child))))	return(rv);
		if (NS_FAILED(rv = child->GetNameSpaceID(nameSpaceID)))	return(rv);
		if (nameSpaceID == kNameSpaceID_XUL)
		{
			nsCOMPtr<nsIAtom> tag;

			if (NS_FAILED(rv = child->GetTag(*getter_AddRefs(tag))))
				return rv;
			if (tag.get() == kTreeColAtom)
			{
				nsAutoString	colResource;

				if (NS_SUCCEEDED(rv= child->GetAttribute(kNameSpaceID_RDF, kResourceAtom, colResource))
					&& (rv == NS_CONTENT_ATTR_HAS_VALUE))
				{
					PRBool		setFlag = PR_FALSE;
					if (colResource == sortResource)
					{
						*sortColIndex = colIndex;
						found = PR_TRUE;
						if (!sortDirection.EqualsIgnoreCase("natural"))
						{
							setFlag = PR_TRUE;
						}
					}
					if (setFlag == PR_TRUE)
					{
						nsAutoString	trueStr("true");
						child->SetAttribute(kNameSpaceID_None, kSortAtom, trueStr, PR_TRUE);
						child->SetAttribute(kNameSpaceID_None, kSortDirectionAtom, sortDirection, PR_TRUE);

						// Note: don't break out of loop; want to set/unset attribs on ALL sort columns
						// break;
					}
					else
					{
						child->UnsetAttribute(kNameSpaceID_None, kSortAtom, PR_TRUE);
						child->UnsetAttribute(kNameSpaceID_None, kSortDirectionAtom, PR_TRUE);
					}
				}
				++colIndex;
			}
		}
	}
	return((found == PR_TRUE) ? NS_OK : NS_ERROR_FAILURE);
}



nsresult
XULSortServiceImpl::GetSortColumnInfo(nsIContent *tree, nsString &sortResource, nsString &sortDirection, nsString &sortResource2)
{
        nsCOMPtr<nsIContent>	child;
	PRBool			found = PR_FALSE;
	PRInt32			childIndex, numChildren, nameSpaceID;
	nsresult		rv;

	if (NS_FAILED(rv = tree->ChildCount(numChildren)))	return(rv);
	for (childIndex=0; childIndex<numChildren; childIndex++)
	{
		if (NS_FAILED(rv = tree->ChildAt(childIndex, *getter_AddRefs(child))))	return(rv);
		if (NS_FAILED(rv = child->GetNameSpaceID(nameSpaceID)))	return(rv);
		if (nameSpaceID == kNameSpaceID_XUL)
		{
			nsCOMPtr<nsIAtom> tag;

			if (NS_FAILED(rv = child->GetTag(*getter_AddRefs(tag))))
				return rv;
			if (tag.get() == kTreeColAtom)
			{
				nsAutoString	value;
				if (NS_SUCCEEDED(rv = child->GetAttribute(kNameSpaceID_None, kSortAtom, value))
					&& (rv == NS_CONTENT_ATTR_HAS_VALUE))
				{
					if (value.EqualsIgnoreCase("true"))
					{
						if (NS_SUCCEEDED(rv = child->GetAttribute(kNameSpaceID_RDF, kResourceAtom,
							sortResource)) && (rv == NS_CONTENT_ATTR_HAS_VALUE))
						{
							if (NS_SUCCEEDED(rv = child->GetAttribute(kNameSpaceID_None, kSortDirectionAtom,
								sortDirection)) && (rv == NS_CONTENT_ATTR_HAS_VALUE))
							{
								if (NS_FAILED(rv = child->GetAttribute(kNameSpaceID_None, kResource2Atom,
									sortResource2)) || (rv != NS_CONTENT_ATTR_HAS_VALUE))
								{
									sortResource2.Truncate();
								}
								found = PR_TRUE;
							}
						}
						break;
					}
				}
			}
		}
	}
	return((found == PR_TRUE) ? NS_OK : NS_ERROR_FAILURE);
}



nsresult
XULSortServiceImpl::GetTreeCell(sortPtr sortInfo, nsIContent *node, PRInt32 cellIndex, nsIContent **cell)
{
	PRBool			found = PR_FALSE;
	PRInt32			childIndex = 0, numChildren = 0, nameSpaceID;
        nsCOMPtr<nsIContent>	child;
	nsresult		rv;

	if (NS_FAILED(rv = node->ChildCount(numChildren)))	return(rv);

	for (childIndex=0; childIndex<numChildren; childIndex++)
	{
		if (NS_FAILED(rv = node->ChildAt(childIndex, *getter_AddRefs(child))))	break;
		if (NS_FAILED(rv = child->GetNameSpaceID(nameSpaceID)))	break;
		if (nameSpaceID == sortInfo->kNameSpaceID_XUL)
		{
			nsCOMPtr<nsIAtom> tag;
			if (NS_FAILED(rv = child->GetTag(*getter_AddRefs(tag))))	return rv;
			if (tag.get() == sortInfo->kTreeCellAtom.get())
			{
				if (cellIndex == 0)
				{
					found = PR_TRUE;
					*cell = child;
					NS_ADDREF(*cell);
					break;
				}
				--cellIndex;
			}
		}
	}
	return((found == PR_TRUE) ? NS_OK : NS_ERROR_FAILURE);
}



nsresult
XULSortServiceImpl::GetTreeCellValue(sortPtr sortInfo, nsIContent *node, nsString & val)
{
	PRBool			found = PR_FALSE;
	PRInt32			childIndex = 0, numChildren = 0, nameSpaceID;
        nsCOMPtr<nsIContent>	child;
	nsresult		rv;

	if (NS_FAILED(rv = node->ChildCount(numChildren)))	return(rv);

	for (childIndex=0; childIndex<numChildren; childIndex++)
	{
		if (NS_FAILED(rv = node->ChildAt(childIndex, *getter_AddRefs(child))))
			break;
		if (NS_FAILED(rv = child->GetNameSpaceID(nameSpaceID)))
			break;
		if (nameSpaceID != sortInfo->kNameSpaceID_XUL)
		{
			// Get text using the DOM
			nsCOMPtr<nsIDOMText> domText;
			rv = child->QueryInterface(kIDOMTextIID, getter_AddRefs(domText));
			if (NS_FAILED(rv))
				break;
			val.Truncate();
			domText->GetData(val);
			found = PR_TRUE;
			break;
		}
	}
	return((found == PR_TRUE) ? NS_OK : NS_ERROR_FAILURE);
}



nsresult
XULSortServiceImpl::RemoveAllChildren(nsIContent *container)
{
        nsCOMPtr<nsIContent>	child;
	PRInt32			childIndex, numChildren;
	nsresult		rv;

	if (NS_FAILED(rv = container->ChildCount(numChildren)))	return(rv);
	if (numChildren == 0)	return(NS_OK);

	for (childIndex=numChildren-1; childIndex >= 0; childIndex--)
	{
		if (NS_FAILED(rv = container->ChildAt(childIndex, *getter_AddRefs(child))))	break;
		container->RemoveChildAt(childIndex, PR_FALSE);
	}
	return(rv);
}



nsresult
XULSortServiceImpl::CompareNodes(nsIRDFNode *cellNode1, PRBool isCollationKey1,
				 nsIRDFNode *cellNode2, PRBool isCollationKey2,
				 PRInt32 & sortOrder)
{
	sortOrder = 0;

	nsAutoString			cellVal1, cellVal2;
	nsCOMPtr<nsIRDFLiteral>		literal1 = do_QueryInterface(cellNode1);
	nsCOMPtr<nsIRDFLiteral>		literal2 = do_QueryInterface(cellNode2);
	if (literal1)
	{
		const PRUnichar		*uni1 = nsnull;
		literal1->GetValueConst(&uni1);
		if (uni1)	cellVal1 = uni1;
	}
	if (literal2)
	{
		const PRUnichar		*uni2 = nsnull;
		literal2->GetValueConst(&uni2);
		if (uni2)	cellVal2 = uni2;
	}

	if (isCollationKey1 == PR_TRUE && isCollationKey2 == PR_TRUE)
	{
		// sort collation keys
		if (collationService)
		{
			collationService->CompareSortKey(cellVal1, cellVal2, &sortOrder);
		}
		else
		{
			// without a collation service, unable to collate
			sortOrder = 0;
		}
	}
	else if ((isCollationKey1 == PR_TRUE) && (isCollationKey2 == PR_FALSE))
	{
		sortOrder = -1;
	}
	else if ((isCollationKey1 == PR_FALSE) && (isCollationKey2 == PR_TRUE))
	{
		sortOrder = 1;
	}
	else if (literal1 && literal2)
	{
		// neither is a collation key, but both are strings, so fallback to a string comparison
		sortOrder = (PRInt32)cellVal1.Compare(cellVal2, PR_TRUE);
	}
	else
	{
		// not a collation key, and both aren't strings, so try other data types (ints)
		nsCOMPtr<nsIRDFInt>		intLiteral1 = do_QueryInterface(cellNode1);
		nsCOMPtr<nsIRDFInt>		intLiteral2 = do_QueryInterface(cellNode2);
		if (intLiteral1 && intLiteral2)
		{
			PRInt32			intVal1, intVal2;
			intLiteral1->GetValue(&intVal1);
			intLiteral2->GetValue(&intVal2);
			
			sortOrder = 0;
			if (intVal1 < intVal2)		sortOrder = -1;
			else if (intVal1 > intVal2)	sortOrder = 1;
		}
		else
		{
			// not a collation key, and both aren't strings, so try other data types (dates)
			nsCOMPtr<nsIRDFDate>		dateLiteral1 = do_QueryInterface(cellNode1);
			nsCOMPtr<nsIRDFDate>		dateLiteral2 = do_QueryInterface(cellNode2);
			if (dateLiteral1 && dateLiteral2)
			{
				PRInt64			dateVal1, dateVal2;
				dateLiteral1->GetValue(&dateVal1);
				dateLiteral2->GetValue(&dateVal2);
				
				sortOrder = 0;
				if (LL_CMP(dateVal1, <, dateVal2))	sortOrder = -1;
				else if (LL_CMP(dateVal1, >, dateVal2))	sortOrder = 1;
			}
		}
	}
	return(NS_OK);
}



nsresult
XULSortServiceImpl::OpenSort(nsIRDFNode *node1, nsIRDFNode *node2, sortPtr sortInfo, PRInt32 & sortOrder)
{
	nsCOMPtr<nsIRDFNode>	cellNode1, cellNode2;
	nsAutoString		cellVal1(""), cellVal2("");
	nsresult		rv = NS_OK;
	PRBool			isCollationKey1 = PR_FALSE, isCollationKey2 = PR_FALSE;

	sortOrder = 0;

	nsCOMPtr<nsIRDFResource>	res1 = do_QueryInterface(node1);
	if (res1)
	{
		rv = GetResourceValue(res1, sortInfo->sortProperty, sortInfo, getter_AddRefs(cellNode1), isCollationKey1);
	}
	nsCOMPtr<nsIRDFResource>	res2 = do_QueryInterface(node2);
	if (res2)
	{
		rv = GetResourceValue(res2, sortInfo->sortProperty, sortInfo, getter_AddRefs(cellNode2), isCollationKey2);
	}

	rv = CompareNodes(cellNode1, isCollationKey1, cellNode2, isCollationKey2, sortOrder);

	if (sortOrder == 0)
	{
		// nodes appear to be equivalent, check for secondary sort criteria
		if (sortInfo->sortProperty2 != nsnull)
		{
			nsCOMPtr<nsIRDFResource>	temp = sortInfo->sortProperty;
			sortInfo->sortProperty = sortInfo->sortProperty2;
			sortInfo->sortProperty2 = nsnull;

			rv = OpenSort(node1, node2, sortInfo, sortOrder);

			sortInfo->sortProperty2 = sortInfo->sortProperty;
			sortInfo->sortProperty = temp;
		}
	}

	if (sortInfo->descendingSort == PR_TRUE)
	{
		// descending sort is being imposed, so reverse the sort order
		sortOrder = -sortOrder;
	}

	return(rv);
}



int
openSortCallback(const void *data1, const void *data2, void *privateData)
{
	/// Note: openSortCallback is a small C callback stub for NS_QuickSort

	_sortStruct		*sortInfo = (_sortStruct *)privateData;
	nsIRDFNode		*node1 = *(nsIRDFNode **)data1;
	nsIRDFNode		*node2 = *(nsIRDFNode **)data2;
	PRInt32			sortOrder = 0;
	nsresult		rv;

	if (nsnull != sortInfo)
	{
		rv = XULSortServiceImpl::OpenSort(node1, node2, sortInfo, sortOrder);
	}
	return(sortOrder);
}



nsresult
XULSortServiceImpl::GetCachedTarget(sortPtr sortInfo, nsIRDFResource* aSource,
		nsIRDFResource *aProperty, PRBool aTruthValue, nsIRDFNode **aResult)
{
	nsresult	rv = NS_OK, rvTemp;

	if (!(sortInfo->mInner))
	{
		// if we don't have a mInner, create one
		rvTemp = nsComponentManager::CreateInstance(kRDFInMemoryDataSourceCID,
			nsnull, nsIRDFDataSource::GetIID(), (void **)&(sortInfo->mInner));
	}
	if (sortInfo->mInner)
	{
		// else, if we do have a mInner, look for the resource in it
		rv = sortInfo->mInner->GetTarget(aSource, aProperty, aTruthValue, aResult);
	}
	if (NS_SUCCEEDED(rv) && (rv == NS_RDF_NO_VALUE) && (sortInfo->db))
	{
		// if we don't have a cached value, look it up in the document's DB
		if (NS_SUCCEEDED(rv = (sortInfo->db)->GetTarget(aSource, aProperty,
			aTruthValue, aResult)) && (rv != NS_RDF_NO_VALUE))
		{
			// and if we have a value, cache it away in our mInner also
			rvTemp = sortInfo->mInner->Assert(aSource, aProperty, *aResult, PR_TRUE);
		}
	}
	return(rv);
}



nsresult
XULSortServiceImpl::GetCachedResource(sortPtr sortInfo, nsIRDFResource *sortProperty, const char *suffix, nsIRDFResource **res)
{
	nsresult		rv;

	*res = nsnull;

	const char		*sortPropertyURI = nsnull;
	rv = sortProperty->GetValueConst(&sortPropertyURI);
	if (NS_SUCCEEDED(rv) && (sortPropertyURI))
	{
		nsAutoString		resName(sortPropertyURI);
		if (suffix)		resName += suffix;

		if (!(sortInfo->resCache))
		{
			// if we don't have a cache, create one
			rv = NS_NewISupportsArray(getter_AddRefs(sortInfo->resCache));
		}
		else
		{
			// else, if we do have a cache, look for the resource in it
			PRUint32		numRes;
			if (NS_SUCCEEDED(rv = sortInfo->resCache->Count(&numRes)))
			{
				PRUint32	loop;
				for (loop=0; loop<numRes; loop++)
				{
					nsCOMPtr<nsISupports>	iSupports;
					if (NS_SUCCEEDED(rv = sortInfo->resCache->GetElementAt(loop,
						getter_AddRefs(iSupports))))
					{
						nsCOMPtr<nsIRDFResource>	aRes = do_QueryInterface(iSupports);
						if (aRes)
						{
							const char	*resURI = nsnull;
							if (NS_SUCCEEDED(rv = aRes->GetValueConst(&resURI)) && (resURI))
							{
								if (resName.Equals(resURI))
								{
									// found res in cache, so just return it
									*res = aRes;
									NS_ADDREF(*res);
									break;
								}
							}
						}
					}
				}
			}
		}

		if (!(*res))
		{
			nsCOMPtr<nsIRDFResource>	sortRes;
			if (NS_SUCCEEDED(rv = sortInfo->rdfService->GetResource(nsCAutoString(resName), 
				getter_AddRefs(sortRes))) && (sortRes))
			{
				*res = sortRes;
				NS_ADDREF(*res);
				
				// and add it into the cache array
				if (sortInfo->resCache)
				{
					sortInfo->resCache->AppendElement(sortRes);
				}
			}
		}
	}
	return(rv);
}



nsresult
XULSortServiceImpl::GetResourceValue(nsIRDFResource *res1, nsIRDFResource *sortProperty, sortPtr sortInfo,
				nsIRDFNode **target, PRBool &isCollationKey)
{
	nsresult		rv = NS_OK;

	*target = nsnull;
	isCollationKey = PR_FALSE;

	if ((res1) && (sortInfo->naturalOrderSort == PR_FALSE) && (sortInfo->sortProperty))
	{
		nsCOMPtr<nsIRDFResource>	modSortRes;

		// for any given property, first ask the graph for its value with "?collation=true" appended
		// to indicate that if there is a collation key available for this value, we want it
		if (NS_SUCCEEDED(rv = GetCachedResource(sortInfo, sortInfo->sortProperty, "?collation=true",
			getter_AddRefs(modSortRes))) && (modSortRes))
		{
			if (NS_SUCCEEDED(rv = GetCachedTarget(sortInfo, res1, modSortRes,
				PR_TRUE, target)) && (rv != NS_RDF_NO_VALUE))
			{
				isCollationKey = PR_TRUE;
			}
		}
		if (!(*target))
		{
			// if no collation key, ask the graph for its value with "?sort=true" appended
			// to indicate that if there is any distinction between its display value and sorting
			// value, we want the sorting value (so that, for example, a mail datasource could strip
			// off a "Re:" on a mail message subject)
			if (NS_SUCCEEDED(rv = GetCachedResource(sortInfo, sortInfo->sortProperty, "?sort=true",
				getter_AddRefs(modSortRes))) && (modSortRes))
			{
				if (NS_SUCCEEDED(rv = GetCachedTarget(sortInfo, res1, modSortRes,
					PR_TRUE, target)) && (rv != NS_RDF_NO_VALUE))
				{
				}
			}
		}
		if (!(*target))
		{
			// if no collation key and no special sorting value, just get the property value
			if (NS_SUCCEEDED(rv = GetCachedTarget(sortInfo, res1, sortProperty,
				PR_TRUE, target) && (rv != NS_RDF_NO_VALUE)))
			{
			}
		}
	}
	return(rv);
}



nsresult
XULSortServiceImpl::GetNodeValue(nsIContent *node1, nsIRDFResource *sortProperty, sortPtr sortInfo,
				nsIRDFNode **theNode, PRBool &isCollationKey)
{
	nsresult		rv;

	isCollationKey = PR_FALSE;

	nsCOMPtr<nsIDOMXULElement>	dom1 = do_QueryInterface(node1);
	if (!dom1)	return(NS_ERROR_FAILURE);

//	nsCOMPtr<nsIRDFResource>	res1 = do_QueryInterface(dom1);
	nsCOMPtr<nsIRDFResource>	res1;
	if (NS_FAILED(rv = dom1->GetResource(getter_AddRefs(res1))))
	{
		res1 = null_nsCOMPtr();
	}
	// Note: don't check for res1 QI failure here.  It only succeeds for RDF nodes,
	// but for XUL nodes it will failure; in the failure case, the code below gets
	// the cell's text value straight from the DOM
	
	if ((sortInfo->naturalOrderSort == PR_FALSE) && (sortInfo->sortProperty))
	{
		rv = GetResourceValue(res1, sortProperty, sortInfo, theNode, isCollationKey);

//		if (cellVal1.Length() == 0)
		if (NS_FAILED(rv) || (rv == NS_RDF_NO_VALUE))
		{
		        nsCOMPtr<nsIContent>	cell1;
			if (NS_SUCCEEDED(rv = GetTreeCell(sortInfo, node1, sortInfo->colIndex,
				getter_AddRefs(cell1))) && (cell1))
			{
				nsAutoString		cellVal1;
				if (NS_SUCCEEDED(rv = GetTreeCellValue(sortInfo, cell1, cellVal1)) &&
					(rv != NS_RDF_NO_VALUE))
				{
					nsCOMPtr<nsIRDFLiteral>	nodeLiteral;
					gRDFService->GetLiteral(cellVal1.GetUnicode(), getter_AddRefs(nodeLiteral));
					*theNode = nodeLiteral;
					NS_IF_ADDREF(*theNode);
					isCollationKey = PR_FALSE;
				}
			}
		}
	}
	else if (sortInfo->naturalOrderSort == PR_TRUE)
	{
		nsAutoString		cellPosVal1;

		// check to see if this is a RDF_Seq
		// Note: this code doesn't handle the aggregated Seq case especially well
		if ((res1) && (sortInfo->db))
		{
			nsCOMPtr<nsISimpleEnumerator>	arcs;
			if (NS_SUCCEEDED(rv = sortInfo->db->ArcLabelsIn(res1, getter_AddRefs(arcs))))
			{
				PRBool		hasMore = PR_TRUE;
				while(hasMore)
				{
					if (NS_FAILED(rv = arcs->HasMoreElements(&hasMore)))	break;
					if (hasMore == PR_FALSE)	break;

					nsCOMPtr<nsISupports>	isupports;
					if (NS_FAILED(rv = arcs->GetNext(getter_AddRefs(isupports))))	break;
					nsCOMPtr<nsIRDFResource> property = do_QueryInterface(isupports);
					if (!property)			continue;

					// hack: it it looks like a RDF_Seq and smells like a RDF_Seq...
					static const char kRDFNameSpace_Seq_Prefix[] = "http://www.w3.org/1999/02/22-rdf-syntax-ns#_";
					const char	*uri = nsnull;
					if (NS_FAILED(rv = property->GetValueConst(&uri)))	continue;
					if (!uri)	continue;
					if (nsCRT::strncasecmp(uri, kRDFNameSpace_Seq_Prefix, sizeof(kRDFNameSpace_Seq_Prefix)-1))
						continue;

					cellPosVal1 = uri;
					cellPosVal1.Cut(0, sizeof(kRDFNameSpace_Seq_Prefix)-1);

					// hack: assume that its a number, so pad out a bit
			                nsAutoString	zero("000000");
			                if (cellPosVal1.Length() < zero.Length())
			                {
						cellPosVal1.Insert(zero, 0, zero.Length() - cellPosVal1.Length());
			                }

					hasMore = PR_FALSE;
					break;
				}
			}
		}
		if (cellPosVal1.Length() == 0)
		{
			rv = node1->GetAttribute(kNameSpaceID_None, sortInfo->kNaturalOrderPosAtom, cellPosVal1);
		}
		if (NS_SUCCEEDED(rv) && (rv != NS_RDF_NO_VALUE))
		{
			nsCOMPtr<nsIRDFLiteral>	nodePosLiteral;
			gRDFService->GetLiteral(cellPosVal1.GetUnicode(), getter_AddRefs(nodePosLiteral));
			*theNode = nodePosLiteral;
			NS_IF_ADDREF(*theNode);
			isCollationKey = PR_FALSE;
		}
	}
	return(rv);
}



nsresult
XULSortServiceImpl::InplaceSort(nsIContent *node1, nsIContent *node2, sortPtr sortInfo, PRInt32 & sortOrder)
{
	nsAutoString		cellVal1(""), cellVal2("");
	PRBool			isCollationKey1 = PR_FALSE, isCollationKey2 = PR_FALSE;
	nsresult		rv;

	sortOrder = 0;

	nsCOMPtr<nsIRDFNode>	cellNode1, cellNode2;
	GetNodeValue(node1, sortInfo->sortProperty, sortInfo, getter_AddRefs(cellNode1), isCollationKey1);
	GetNodeValue(node2, sortInfo->sortProperty, sortInfo, getter_AddRefs(cellNode2), isCollationKey2);
	if ((!cellNode1) && (!cellNode2) && (sortInfo->sortProperty2 == nsnull))
	{
		rv = GetNodeValue(node1, kNC_Name, sortInfo, getter_AddRefs(cellNode1), isCollationKey1);
		rv = GetNodeValue(node2, kNC_Name, sortInfo, getter_AddRefs(cellNode2), isCollationKey2);
	}

	rv = CompareNodes(cellNode1, isCollationKey1, cellNode2, isCollationKey2, sortOrder);

	if (sortOrder == 0)
	{
		// nodes appear to be equivalent, check for secondary sort criteria
		if (sortInfo->sortProperty2 != nsnull)
		{
			nsCOMPtr<nsIRDFResource>	temp = sortInfo->sortProperty;
			sortInfo->sortProperty = sortInfo->sortProperty2;
			sortInfo->sortProperty2 = nsnull;

			rv = InplaceSort(node1, node2, sortInfo, sortOrder);

			sortInfo->sortProperty2 = sortInfo->sortProperty;
			sortInfo->sortProperty = temp;
		}
	}

	if (sortInfo->descendingSort == PR_TRUE)
	{
		// descending sort is being imposed, so reverse the sort order
		sortOrder = -sortOrder;
	}

	return(NS_OK);
}



int
inplaceSortCallback(const void *data1, const void *data2, void *privateData)
{
	/// Note: inplaceSortCallback is a small C callback stub for NS_QuickSort

	_sortStruct		*sortInfo = (_sortStruct *)privateData;
	nsIContent		*node1 = *(nsIContent **)data1;
	nsIContent		*node2 = *(nsIContent **)data2;
	PRInt32			sortOrder = 0;
	nsresult		rv;

	if (nsnull != sortInfo)
	{
		rv = XULSortServiceImpl::InplaceSort(node1, node2, sortInfo, sortOrder);
	}
	return(sortOrder);
}



nsresult
XULSortServiceImpl::SortTreeChildren(nsIContent *container, PRInt32 colIndex, sortPtr sortInfo)
{
	PRInt32			childIndex = 0, numChildren = 0, nameSpaceID;
        nsCOMPtr<nsIContent>	child;
	nsresult		rv;

	if (NS_FAILED(rv = container->ChildCount(numChildren)))	return(rv);

	nsCOMPtr<nsISupportsArray> childArray;
	rv = NS_NewISupportsArray(getter_AddRefs(childArray));
	if (NS_FAILED(rv))	return(rv);

	for (childIndex=0; childIndex<numChildren; childIndex++)
	{
		if (NS_FAILED(rv = container->ChildAt(childIndex, *getter_AddRefs(child))))	break;
		if (NS_FAILED(rv = child->GetNameSpaceID(nameSpaceID)))	break;
		if (nameSpaceID == kNameSpaceID_XUL)
		{
			nsCOMPtr<nsIAtom> tag;
			if (NS_FAILED(rv = child->GetTag(*getter_AddRefs(tag))))	return rv;
			if (tag.get() == kTreeItemAtom)
			{
				childArray->AppendElement(child);
				
				// if no pos is specified, set one
				nsAutoString	pos;
				if (NS_FAILED(rv = child->GetAttribute(kNameSpaceID_None, kNaturalOrderPosAtom, pos))
					|| (rv != NS_CONTENT_ATTR_HAS_VALUE))
				{
					nsAutoString	zero("0000");
					pos = "";
					pos.Append(childIndex+1, 10);
					if (pos.Length() < 4)
					{
						pos.Insert(zero, 0, 4-pos.Length()); 
					}
					child->SetAttribute(kNameSpaceID_None, kNaturalOrderPosAtom, pos, PR_FALSE);
				}
			}
		}
	}
	PRUint32 cnt = 0;
	rv = childArray->Count(&cnt);
	NS_ASSERTION(NS_SUCCEEDED(rv), "Count failed");
	PRUint32 numElements = cnt;
	if (numElements > 0)
	{
		nsIContent ** flatArray = new nsIContent*[numElements];
		if (flatArray)
		{
			// flatten array of resources, sort them, then add as tree elements
			PRUint32	loop;
		        for (loop=0; loop<numElements; loop++)
		        {
				flatArray[loop] = (nsIContent *)childArray->ElementAt(loop);
			}

			/* smart sorting (sort within separators) on name column */
			if (sortInfo->sortProperty == kNC_Name)
			{
				PRUint32	startIndex=0;
				for (loop=0; loop<numElements; loop++)
				{
					nsAutoString	type;
					if (NS_SUCCEEDED(rv = flatArray[loop]->GetAttribute(kNameSpaceID_None, kRDF_type, type))
						&& (rv == NS_CONTENT_ATTR_HAS_VALUE))
					{
						if (type.EqualsIgnoreCase(kURINC_BookmarkSeparator))
						{
							if (loop > startIndex+1)
							{
								NS_QuickSort((void *)&flatArray[startIndex], loop-startIndex, sizeof(nsIContent *),
									inplaceSortCallback, (void *)sortInfo);
								startIndex = loop+1;
							}
						}
					}
				}
				if (loop > startIndex+1)
				{
					NS_QuickSort((void *)&flatArray[startIndex], loop-startIndex, sizeof(nsIContent *),
						inplaceSortCallback, (void *)sortInfo);
					startIndex = loop+1;
				}
			}
			else
			{
				NS_QuickSort((void *)flatArray, numElements, sizeof(nsIContent *),
					inplaceSortCallback, (void *)sortInfo);
			}

			// Bug 6665. This is a hack to "addref" the resources
			// before we remove them from the content model. This
			// keeps them from getting released, which causes
			// performance problems for some datasources.
			for (loop = 0; loop < numElements; loop++)
			{
				nsIRDFResource	*resource;
				gXULUtils->GetElementResource(flatArray[loop], &resource);
				// Note that we don't release; see part deux below...
			}

			RemoveAllChildren(container);
			
			// insert sorted children			
			numChildren = 0;
			for (loop=0; loop<numElements; loop++)
			{
				container->InsertChildAt((nsIContent *)flatArray[loop], numChildren++, PR_FALSE);
			}

			// Bug 6665, part deux. The Big Hack.
			for (loop = 0; loop < numElements; loop++)
			{
				nsIRDFResource	*resource;
				gXULUtils->GetElementResource(flatArray[loop], &resource);
				nsrefcnt	refcnt;
				NS_RELEASE2(resource, refcnt);
				NS_RELEASE(resource);
			}

			// recurse on grandchildren
			for (loop=0; loop<numElements; loop++)
			{
				container =  (nsIContent *)flatArray[loop];
				if (NS_FAILED(rv = container->ChildCount(numChildren)))	continue;
				for (childIndex=0; childIndex<numChildren; childIndex++)
				{
					if (NS_FAILED(rv = container->ChildAt(childIndex, *getter_AddRefs(child))))
						continue;
					if (NS_FAILED(rv = child->GetNameSpaceID(nameSpaceID)))	continue;
					if (nameSpaceID == kNameSpaceID_XUL)
					{
						nsCOMPtr<nsIAtom> tag;
						if (NS_FAILED(rv = child->GetTag(*getter_AddRefs(tag))))
							continue;
						if (tag.get() == kTreeChildrenAtom)
						{
							SortTreeChildren(child, colIndex, sortInfo);
						}
					}
				}
			}

			delete [] flatArray;
			flatArray = nsnull;
		}
	}
	rv = childArray->Count(&cnt);
	if (NS_FAILED(rv)) return rv;
	for (int i = cnt - 1; i >= 0; i--)
	{
		childArray->RemoveElementAt(i);
	}
	return(NS_OK);
}



NS_IMETHODIMP
XULSortServiceImpl::OpenContainer(nsIRDFCompositeDataSource *db, nsIContent *container,
			nsIRDFResource **flatArray, PRInt32 numElements, PRInt32 elementSize)
{
	nsresult	rv;
	nsAutoString	sortResource, sortDirection, sortResource2;
	_sortStruct	sortInfo;

	// get sorting info (property to sort on, direction to sort, etc)

	nsCOMPtr<nsIContent>	treeNode;
	if (NS_FAILED(rv = FindTreeElement(container, getter_AddRefs(treeNode))))
		return(rv);

	sortInfo.rdfService = gRDFService;
	sortInfo.db = db;
	sortInfo.resCache = nsnull;
	sortInfo.mInner = nsnull;
	sortInfo.kNaturalOrderPosAtom = kNaturalOrderPosAtom;
	sortInfo.kTreeCellAtom = kTreeCellAtom;
	sortInfo.kNameSpaceID_XUL = kNameSpaceID_XUL;

	if (NS_FAILED(rv = GetSortColumnInfo(treeNode, sortResource, sortDirection, sortResource2)))
		return(rv);

	rv = gRDFService->GetResource(nsCAutoString(sortResource), getter_AddRefs(sortInfo.sortProperty));
	if (NS_FAILED(rv))	return(rv);
	
	sortInfo.sortProperty2 = nsnull;
	if (sortResource2.Length() > 0)
	{
		rv = gRDFService->GetResource(nsCAutoString(sortResource2), getter_AddRefs(sortInfo.sortProperty2));
		if (NS_FAILED(rv))	return(rv);
	}

	if (sortDirection.EqualsIgnoreCase("natural"))
	{
		sortInfo.naturalOrderSort = PR_TRUE;
		sortInfo.descendingSort = PR_FALSE;
		// no need to sort for natural order
	}
	else
	{
		sortInfo.naturalOrderSort = PR_FALSE;
		if (sortDirection.EqualsIgnoreCase("descending"))
			sortInfo.descendingSort = PR_TRUE;
		else
			sortInfo.descendingSort = PR_FALSE;
		NS_QuickSort((void *)flatArray, numElements, elementSize, openSortCallback, (void *)&sortInfo);
	}
	return(NS_OK);
}



NS_IMETHODIMP
XULSortServiceImpl::InsertContainerNode(nsIContent *container, nsIContent *node, PRBool aNotify)
{
	nsresult	rv;
	nsAutoString	sortResource, sortDirection, sortResource2;
	_sortStruct	sortInfo;

	// get sorting info (property to sort on, direction to sort, etc)

	nsCOMPtr<nsIContent>	treeNode;
	if (NS_FAILED(rv = FindTreeElement(container, getter_AddRefs(treeNode))))	return(rv);

	// get composite db for tree
	nsCOMPtr<nsIDOMXULElement> domXulTree;
	sortInfo.rdfService = gRDFService;
	sortInfo.db = nsnull;
	sortInfo.resCache = nsnull;
	sortInfo.mInner = nsnull;

	rv = treeNode->QueryInterface(kIDomXulElementIID, getter_AddRefs(domXulTree));
	if (NS_SUCCEEDED(rv))
	{
		nsCOMPtr<nsIRDFCompositeDataSource>	cds;
		if (NS_SUCCEEDED(rv = domXulTree->GetDatabase(getter_AddRefs(cds))))
		{
			sortInfo.db = cds;
		}
	}

	sortInfo.kNaturalOrderPosAtom = kNaturalOrderPosAtom;
	sortInfo.kTreeCellAtom = kTreeCellAtom;
	sortInfo.kNameSpaceID_XUL = kNameSpaceID_XUL;

	sortInfo.sortProperty = nsnull;
	sortInfo.sortProperty2 = nsnull;
	if (NS_SUCCEEDED(rv = GetSortColumnInfo(treeNode, sortResource, sortDirection, sortResource2)))
	{
		rv = gRDFService->GetResource(nsCAutoString(sortResource), getter_AddRefs(sortInfo.sortProperty));
		if (NS_FAILED(rv))	return(rv);
		if (sortResource2.Length() > 0)
		{
			rv = gRDFService->GetResource(nsCAutoString(sortResource2), getter_AddRefs(sortInfo.sortProperty2));
			if (NS_FAILED(rv))	return(rv);
		}
	}

	// set up sort order info
	sortInfo.naturalOrderSort = PR_FALSE;
	sortInfo.descendingSort = PR_FALSE;
	if (sortDirection.EqualsIgnoreCase("descending"))
	{
		sortInfo.descendingSort = PR_TRUE;
	}
	else if (!sortDirection.EqualsIgnoreCase("ascending"))
	{
		sortInfo.naturalOrderSort = PR_TRUE;
	}

	PRBool			isContainerRDFSeq = PR_FALSE;

	if ((sortInfo.db) && (sortInfo.naturalOrderSort == PR_TRUE))
	{
		// walk up the content model to find the REAL
		// parent container to determine if its a RDF_Seq
		nsCOMPtr<nsIContent>		parent = do_QueryInterface(container, &rv);
		nsCOMPtr<nsIContent>		aContent;

		nsCOMPtr<nsIDocument> doc;
		if (NS_SUCCEEDED(rv) && parent) {
			rv = parent->GetDocument(*getter_AddRefs(doc));

			if (! doc)
				parent = nsnull;
		}

		while(NS_SUCCEEDED(rv) && parent)
		{
			nsAutoString id;
			if (NS_SUCCEEDED(rv = parent->GetAttribute(kNameSpaceID_None, kIdAtom, id))
				&& (rv == NS_CONTENT_ATTR_HAS_VALUE))
			{
				nsCOMPtr<nsIRDFResource>	containerRes;
				rv = gXULUtils->MakeElementResource(doc, id, getter_AddRefs(containerRes));

				if (NS_SUCCEEDED(rv)) {
					rv = sortInfo.db->HasAssertion(containerRes,
								       kRDF_instanceOf,
								       kRDF_Seq,
								       PR_TRUE,
								       &isContainerRDFSeq);
				}
				break;
			}
			aContent = do_QueryInterface(parent, &rv);
			if (NS_SUCCEEDED(rv))
				rv = aContent->GetParent(*getter_AddRefs(parent));
		}
	}

	PRBool			childAdded = PR_FALSE;

	if ((sortInfo.naturalOrderSort == PR_FALSE) ||
		((sortInfo.naturalOrderSort == PR_TRUE) &&
		(isContainerRDFSeq == PR_TRUE)))
	{
#ifdef	XUL_BINARY_INSERTION_SORT
		// figure out where to insert the node when a sort order is being imposed
		// using a smart binary comparison
		PRInt32			numChildren = 0;
		if (NS_FAILED(rv = container->ChildCount(numChildren)))	return(rv);
		if (numChildren > 0)
		{
		        nsCOMPtr<nsIContent>	child;
			PRInt32			last = -1, current, direction = 0, delta = numChildren;
			while(PR_TRUE)
			{
				delta = delta / 2;

				if (last == -1)
				{
					current = delta;
				}
				else if (direction > 0)
				{
					if (delta == 0)	delta = 1;
					current = last + delta;
				}
				else
				{
					if (delta == 0)	delta = 1;
					current = last - delta;
				}

				if (current != last)
				{
					container->ChildAt(current, *getter_AddRefs(child));
					nsIContent	*theChild = child.get();
					direction = inplaceSortCallback(&node, &theChild, &sortInfo);
				}
				if ( (direction == 0) ||
					((current == last + 1) && (direction < 0)) ||
					((current == last - 1) && (direction > 0)) ||
					((current == 0) && (direction < 0)) ||
					((current >= numChildren - 1) && (direction > 0)) )
				{
					if (current >= numChildren)
					{
						container->AppendChildTo(node, aNotify);
					}
					else
					{
						container->InsertChildAt(node,
							((direction > 0) ? current + 1: (current >= 0) ? current : 0),
							aNotify);
					}
					childAdded = PR_TRUE;
					break;
				}
				last = current;
			}
		}
#else
		// figure out where to insert the node when a sort order is being imposed
		// using a simple linear brute-force comparison
		PRInt32			childIndex = 0, numChildren = 0, nameSpaceID;
	        nsCOMPtr<nsIContent>	child;

		if (NS_FAILED(rv = container->ChildCount(numChildren)))	return(rv);
		for (childIndex=0; childIndex<numChildren; childIndex++)
		{
			if (NS_FAILED(rv = container->ChildAt(childIndex, *getter_AddRefs(child))))	return(rv);
			if (NS_FAILED(rv = child->GetNameSpaceID(nameSpaceID)))	return(rv);
			if (nameSpaceID == kNameSpaceID_XUL)
			{
				nsIContent	*theChild = child.get();
				PRInt32 sortVal = inplaceSortCallback(&node, &theChild, &sortInfo);
				if (sortVal <= 0)
				{
					container->InsertChildAt(node, childIndex, aNotify);
					childAdded = PR_TRUE;
					break;
				}
			}
		}
#endif
	}

	if (childAdded == PR_FALSE)
	{
		container->AppendChildTo(node, aNotify);
	}
	return(NS_OK);
}



NS_IMETHODIMP
XULSortServiceImpl::Sort(nsIDOMNode* node, const char *sortResource, const char *sortDirection)
{
	nsAutoString	sortRes(sortResource), sortDir(sortDirection);
	nsresult	rv = DoSort(node, sortResource, sortDirection);
	return(rv);
}



nsresult
XULSortServiceImpl::DoSort(nsIDOMNode* node, const nsString& sortResource,
                           const nsString& sortDirection)
{
	PRInt32		colIndex, treeBodyIndex;
	nsresult	rv;
	_sortStruct	sortInfo;

	// get tree node
	nsCOMPtr<nsIContent>	contentNode = do_QueryInterface(node);
	if (!contentNode)	return(NS_ERROR_FAILURE);
	nsCOMPtr<nsIContent>	treeNode;
	if (NS_FAILED(rv = FindTreeElement(contentNode, getter_AddRefs(treeNode))))	return(rv);

	// get composite db for tree
	sortInfo.rdfService = gRDFService;
	sortInfo.db = nsnull;
	sortInfo.resCache = nsnull;
	sortInfo.mInner = nsnull;

	nsCOMPtr<nsIDOMXULElement>	domXulTree = do_QueryInterface(treeNode);
	if (!domXulTree)	return(NS_ERROR_FAILURE);
	nsCOMPtr<nsIRDFCompositeDataSource>	cds;
	if (NS_SUCCEEDED(rv = domXulTree->GetDatabase(getter_AddRefs(cds))))
	{
		sortInfo.db = cds;
	}

	sortInfo.kNaturalOrderPosAtom = kNaturalOrderPosAtom;
	sortInfo.kTreeCellAtom = kTreeCellAtom;
	sortInfo.kNameSpaceID_XUL = kNameSpaceID_XUL;

	if (NS_FAILED(rv = gRDFService->GetResource(nsCAutoString(sortResource), getter_AddRefs(sortInfo.sortProperty))))
		return(rv);
	
	// determine new sort resource and direction to use
	if (sortDirection.EqualsIgnoreCase("natural"))
	{
		sortInfo.naturalOrderSort = PR_TRUE;
		sortInfo.descendingSort = PR_FALSE;
	}
	else
	{
		sortInfo.naturalOrderSort = PR_FALSE;
		if (sortDirection.EqualsIgnoreCase("ascending"))	sortInfo.descendingSort = PR_FALSE;
		else if (sortDirection.EqualsIgnoreCase("descending"))	sortInfo.descendingSort = PR_TRUE;
	}

	// get index of sort column, find tree body, and sort. The sort
	// _won't_ send any notifications, so we won't trigger any reflows...
	if (NS_FAILED(rv = GetSortColumnIndex(treeNode, sortResource, sortDirection, &colIndex)))	return(rv);
	sortInfo.colIndex = colIndex;
	nsCOMPtr<nsIContent>	treeBody;
	if (NS_FAILED(rv = FindTreeChildrenElement(treeNode, getter_AddRefs(treeBody))))	return(rv);
	if (NS_SUCCEEDED(rv = SortTreeChildren(treeBody, colIndex, &sortInfo)))
	{
	}

	// Now remove the treebody and re-insert it to force the frames to be rebuilt.
    	nsCOMPtr<nsIContent>	treeParent;
	if (NS_FAILED(rv = treeBody->GetParent(*getter_AddRefs(treeParent))))	return(rv);
	if (NS_FAILED(rv = treeParent->IndexOf(treeBody, treeBodyIndex)))	return(rv);
	if (NS_FAILED(rv = treeParent->RemoveChildAt(treeBodyIndex, PR_TRUE)))	return(rv);

	if (NS_FAILED(rv = treeParent->AppendChildTo(treeBody, PR_TRUE)))	return(rv);
	return(NS_OK);
}



nsresult
NS_NewXULSortService(nsIXULSortService** mgr)
{
	XULSortServiceImpl	*sortService = new XULSortServiceImpl();
	if (!sortService)
	        return(NS_ERROR_OUT_OF_MEMORY);
	
	*mgr = sortService;
	NS_ADDREF(*mgr);
	return(NS_OK);
}
