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
 *   Robert John Churchill <rjc@netscape.com>
 *   Pierre Phaneuf <pp@ludusdesign.com>
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
	PRBool					firstFlag;
	nsCOMPtr<nsIRDFResource>		sortProperty, sortProperty2;
	nsCOMPtr<nsIRDFResource>		sortPropertyColl, sortPropertyColl2;
	nsCOMPtr<nsIRDFResource>		sortPropertySort, sortPropertySort2;

	PRBool					cacheFirstHint;
	nsCOMPtr<nsIRDFNode>			cacheFirstNode;
	PRBool					cacheIsFirstNodeCollationKey;

	nsCOMPtr<nsIRDFCompositeDataSource>	db;
	nsCOMPtr<nsIRDFService>			rdfService;
	nsCOMPtr<nsIRDFDataSource>		mInner;
	nsCOMPtr<nsISupportsArray>		resCache;
	nsCOMPtr<nsIAtom>			kTreeCellAtom;
	PRInt32					colIndex;
	nsCOMPtr<nsIContent>			parentContainer;
	PRInt32					kNameSpaceID_XUL;
	PRBool					descendingSort;
	PRBool					naturalOrderSort;
	PRBool					inbetweenSeparatorSort;

} sortStruct, *sortPtr;



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
	static nsIAtom		*kTreeCellAtom;
	static nsIAtom		*kTreeChildrenAtom;
	static nsIAtom		*kTreeColAtom;
	static nsIAtom		*kTreeColGroupAtom;
	static nsIAtom		*kTreeItemAtom;
	static nsIAtom		*kContainerAtom;
	static nsIAtom		*kResourceAtom;
	static nsIAtom		*kResource2Atom;
	static nsIAtom		*kSortActiveAtom;
	static nsIAtom		*kSortResourceAtom;
	static nsIAtom		*kSortResource2Atom;
	static nsIAtom		*kSortDirectionAtom;
	static nsIAtom		*kSortSeparatorsAtom;
	static nsIAtom		*kIdAtom;
	static nsIAtom		*kRDF_type;

	static nsString		trueStr;
	static nsString		naturalStr;
	static nsString		ascendingStr;
	static nsString		descendingStr;

	static nsIRDFResource	*kNC_Name;
	static nsIRDFResource	*kRDF_instanceOf;
	static nsIRDFResource	*kRDF_Seq;

	static PRInt32		kNameSpaceID_XUL;
	static PRInt32		kNameSpaceID_RDF;

	static nsIRDFService	*gRDFService;
	static nsIXULContentUtils   *gXULUtils;

PRBool		IsTreeElement(nsIContent *element);
nsresult	FindTreeElement(nsIContent *root, nsIContent* aElement,nsIContent** aTreeElement);
nsresult	FindTreeChildrenElement(nsIContent *tree, nsIContent **treeBody);
nsresult	GetSortColumnIndex(nsIContent *tree, const nsString &sortResource, const nsString& sortDirection, nsString &sortResource2, PRBool &inbetweenSeparatorSort, PRInt32 &colIndex, PRBool &found);
nsresult	SetSortHints(nsIContent *tree, const nsString &sortResource, const nsString &sortDirection, const nsString &sortResource2, PRBool inbetweenSeparatorSort, PRBool found);
nsresult	NodeHasSortInfo(nsIContent *node, nsString &sortResource, nsString &sortDirection, nsString &sortResource2, PRBool &inbetweenSeparatorSort, PRBool &found);
nsresult	GetSortColumnInfo(nsIContent *tree, nsString &sortResource, nsString &sortDirection, nsString &sortResource2, PRBool &inbetweenSeparatorSort);
nsresult	GetTreeCell(nsIContent *node, PRInt32 colIndex, nsIContent **cell);
nsresult	SortTreeChildren(nsIContent *container, sortPtr sortInfo);
nsresult	DoSort(nsIDOMNode* node, const nsString& sortResource, const nsString& sortDirection);

static nsresult	GetCachedTarget(sortPtr sortInfo, PRBool useCache, nsIRDFResource* aSource, nsIRDFResource *aProperty, PRBool aTruthValue, nsIRDFNode **aResult);
static nsresult	GetResourceValue(nsIRDFResource *res1, sortPtr sortInfo, PRBool first, PRBool useCache, nsIRDFNode **, PRBool &isCollationKey);
static nsresult	GetNodeValue(nsIContent *node1, sortPtr sortInfo, PRBool first, nsIRDFNode **, PRBool &isCollationKey);
static nsresult	GetTreeCell(sortPtr sortInfo, nsIContent *node, PRInt32 cellIndex, nsIContent **cell);
static nsresult	GetNodeTextValue(sortPtr sortInfo, nsIContent *node, nsString & val);

public:
    static nsresult	InplaceSort(nsIContent *node1, nsIContent *node2, sortPtr sortInfo, PRInt32 & sortOrder);
    static nsresult	CompareNodes(nsIRDFNode *cellNode1, PRBool isCollationKey1,
				 nsIRDFNode *cellNode2, PRBool isCollationKey2,
				 PRBool &bothValid, PRInt32 & sortOrder);

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
nsIAtom* XULSortServiceImpl::kTreeCellAtom;
nsIAtom* XULSortServiceImpl::kTreeChildrenAtom;
nsIAtom* XULSortServiceImpl::kTreeColAtom;
nsIAtom* XULSortServiceImpl::kTreeColGroupAtom;
nsIAtom* XULSortServiceImpl::kTreeItemAtom;
nsIAtom* XULSortServiceImpl::kContainerAtom;
nsIAtom* XULSortServiceImpl::kResourceAtom;
nsIAtom* XULSortServiceImpl::kResource2Atom;
nsIAtom* XULSortServiceImpl::kSortActiveAtom;
nsIAtom* XULSortServiceImpl::kSortResourceAtom;
nsIAtom* XULSortServiceImpl::kSortResource2Atom;
nsIAtom* XULSortServiceImpl::kSortDirectionAtom;
nsIAtom* XULSortServiceImpl::kSortSeparatorsAtom;
nsIAtom* XULSortServiceImpl::kIdAtom;
nsIAtom* XULSortServiceImpl::kRDF_type;

nsString XULSortServiceImpl::trueStr;
nsString XULSortServiceImpl::naturalStr;
nsString XULSortServiceImpl::ascendingStr;
nsString XULSortServiceImpl::descendingStr;

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
	        kTreeCellAtom        		= NS_NewAtom("treecell");
		kTreeChildrenAtom    		= NS_NewAtom("treechildren");
		kTreeColAtom         		= NS_NewAtom("treecol");
		kTreeColGroupAtom         	= NS_NewAtom("treecolgroup");
		kTreeItemAtom        		= NS_NewAtom("treeitem");
		kContainerAtom			= NS_NewAtom("container");
		kResourceAtom        		= NS_NewAtom("resource");
		kResource2Atom        		= NS_NewAtom("resource2");
		kSortActiveAtom			= NS_NewAtom("sortActive");
		kSortResourceAtom		= NS_NewAtom("sortResource");
		kSortResource2Atom		= NS_NewAtom("sortResource2");
		kSortDirectionAtom		= NS_NewAtom("sortDirection");
		kSortSeparatorsAtom		= NS_NewAtom("sortSeparators");
		kIdAtom				= NS_NewAtom("id");
		kRDF_type			= NS_NewAtom("type");
 
 		trueStr				= "true";
 		naturalStr			= "natural";
		ascendingStr			= "ascending";
		descendingStr			= "descending";
 
		nsresult rv;

		if (NS_FAILED(rv = nsServiceManager::GetService(kRDFServiceCID,
						  kIRDFServiceIID, (nsISupports**) &gRDFService)))
		{
			NS_ERROR("couldn't create rdf service");
		}

		rv = nsServiceManager::GetService(kXULContentUtilsCID,
						NS_GET_IID(nsIXULContentUtils),
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
		NS_IF_RELEASE(kTreeCellAtom);
	        NS_IF_RELEASE(kTreeChildrenAtom);
	        NS_IF_RELEASE(kTreeColAtom);
	        NS_IF_RELEASE(kTreeColGroupAtom);
	        NS_IF_RELEASE(kTreeItemAtom);
	        NS_IF_RELEASE(kContainerAtom);
	        NS_IF_RELEASE(kResourceAtom);
	        NS_IF_RELEASE(kResource2Atom);
	        NS_IF_RELEASE(kSortActiveAtom);
	        NS_IF_RELEASE(kSortResourceAtom);
	        NS_IF_RELEASE(kSortResource2Atom);
	        NS_IF_RELEASE(kSortDirectionAtom);
	        NS_IF_RELEASE(kSortSeparatorsAtom);
	        NS_IF_RELEASE(kIdAtom);
		NS_IF_RELEASE(kRDF_type);
	        NS_IF_RELEASE(kNC_Name);
	        NS_IF_RELEASE(kRDF_instanceOf);
	        NS_IF_RELEASE(kRDF_Seq);

		NS_IF_RELEASE(collationService);
		collationService = nsnull;

		if (gRDFService)
		{
			nsServiceManager::ReleaseService(kRDFServiceCID, gRDFService);
			gRDFService = nsnull;
		}

		if (gXULUtils)
		{
			nsServiceManager::ReleaseService(kXULContentUtilsCID, gXULUtils);
			gXULUtils = nsnull;
		}
	}
}



NS_IMPL_ISUPPORTS(XULSortServiceImpl, NS_GET_IID(nsIXULSortService));



////////////////////////////////////////////////////////////////////////



PRBool
XULSortServiceImpl::IsTreeElement(nsIContent *element)
{
	PRBool		isTreeNode = PR_FALSE;
	PRInt32		nameSpaceID;
	nsresult	rv;

	if (NS_SUCCEEDED(rv = element->GetNameSpaceID(nameSpaceID)))
	{
		if (nameSpaceID == kNameSpaceID_XUL)
		{
			nsCOMPtr<nsIAtom>	tag;
			if (NS_FAILED(rv = element->GetTag(*getter_AddRefs(tag))))
				return(rv);
			if (tag.get() == kTreeAtom)
			{
				isTreeNode = PR_TRUE;
			}
		}
	}
	return(isTreeNode);
}



nsresult
XULSortServiceImpl::FindTreeElement(nsIContent *root, nsIContent *aElement, nsIContent **aTreeElement)
{
	nsresult		rv;

	*aTreeElement = nsnull;

	if (root)
	{
		// we have a root hint, so look under it for the tree tag

		PRInt32		numKids, loop;
		if (NS_SUCCEEDED(rv = root->ChildCount(numKids)))
		{
			for (loop=0; loop<numKids; loop++)
			{
				nsCOMPtr<nsIContent>	child;
				if (NS_FAILED(rv = root->ChildAt(loop, *getter_AddRefs(child))))
					return(rv);
				if (IsTreeElement(child) == PR_TRUE)
				{
					*aTreeElement = child;
					NS_ADDREF(*aTreeElement);
					return(NS_OK);
				}
			}
		}
	}
	else
	{
		// we don't have a root hint, so look from the current
		// node upwards until we find it (or hit the top)

		nsCOMPtr<nsIContent>	element(do_QueryInterface(aElement));
		while (element)
		{
			if (IsTreeElement(element) == PR_TRUE)
			{
				*aTreeElement = element;
				NS_ADDREF(*aTreeElement);
				return(NS_OK);
			}
			nsCOMPtr<nsIContent> parent;
			element->GetParent(*getter_AddRefs(parent));
			element = parent;
		}
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
XULSortServiceImpl::GetSortColumnIndex(nsIContent *tree, const nsString &sortResource, const nsString &sortDirection,
					nsString &sortResource2, PRBool &inbetweenSeparatorSort, PRInt32 &sortColIndex, PRBool &found)
{
	PRInt32			childIndex, colIndex = 0, numChildren, nameSpaceID;
        nsCOMPtr<nsIContent>	child;
	nsresult		rv;

	found = PR_FALSE;
	inbetweenSeparatorSort = PR_FALSE;

	sortColIndex = 0;
	if (NS_FAILED(rv = tree->ChildCount(numChildren)))	return(rv);
	for (childIndex=0; childIndex<numChildren; childIndex++)
	{
		if (NS_FAILED(rv = tree->ChildAt(childIndex, *getter_AddRefs(child))))	return(rv);
		if (NS_FAILED(rv = child->GetNameSpaceID(nameSpaceID)))	return(rv);
		if (nameSpaceID == kNameSpaceID_XUL)
		{
			nsCOMPtr<nsIAtom> tag;

			if (NS_FAILED(rv = child->GetTag(*getter_AddRefs(tag))))
				return(rv);
			if (tag.get() == kTreeColGroupAtom)
			{
				rv = GetSortColumnIndex(child, sortResource, sortDirection, sortResource2,
					inbetweenSeparatorSort, sortColIndex, found);
			}
			else if (tag.get() == kTreeColAtom)
			{
				nsAutoString	value;

				if (NS_SUCCEEDED(rv = child->GetAttribute(kNameSpaceID_RDF, kResourceAtom, value))
					&& (rv == NS_CONTENT_ATTR_HAS_VALUE))
				{
					PRBool		setFlag = PR_FALSE;
					if (value == sortResource)
					{
						sortColIndex = colIndex;
												
						if (!sortDirection.Equals(naturalStr))
						{
							found = PR_TRUE;
							setFlag = PR_TRUE;

							// secondary sort info is optional
							if (NS_FAILED(rv = child->GetAttribute(kNameSpaceID_RDF, kResource2Atom,
								sortResource2)) || (rv != NS_CONTENT_ATTR_HAS_VALUE))
							{
								sortResource2.Truncate();
							}
						}
					}
					if (NS_SUCCEEDED(rv = child->GetAttribute(kNameSpaceID_None, kSortSeparatorsAtom, value))
						&& (rv == NS_CONTENT_ATTR_HAS_VALUE) && (value.EqualsIgnoreCase(trueStr)))
					{
						inbetweenSeparatorSort = PR_TRUE;
					}

					if (setFlag == PR_TRUE)
					{
						child->SetAttribute(kNameSpaceID_None, kSortActiveAtom, trueStr, PR_TRUE);
						child->SetAttribute(kNameSpaceID_None, kSortDirectionAtom, sortDirection, PR_TRUE);

						// Note: don't break out of loop; want to set/unset attribs on ALL sort columns
						// break;
					}
					else
					{
						child->UnsetAttribute(kNameSpaceID_None, kSortActiveAtom, PR_TRUE);
						child->UnsetAttribute(kNameSpaceID_None, kSortDirectionAtom, PR_TRUE);
					}
				}
				++colIndex;
			}
		}
	}
	SetSortHints(tree, sortResource, sortDirection, sortResource2, inbetweenSeparatorSort, found);
	return(NS_OK);
}



nsresult
XULSortServiceImpl::SetSortHints(nsIContent *tree, const nsString &sortResource, const nsString &sortDirection,
				const nsString &sortResource2, PRBool inbetweenSeparatorSort, PRBool found)
{
	if (found == PR_TRUE)
	{
		// set hints on tree root node
		tree->SetAttribute(kNameSpaceID_None, kSortActiveAtom, trueStr, PR_FALSE);
		tree->SetAttribute(kNameSpaceID_None, kSortDirectionAtom, sortDirection, PR_FALSE);
		tree->SetAttribute(kNameSpaceID_RDF, kResourceAtom, sortResource, PR_FALSE);

		if (sortResource2.Length() > 0)
			tree->SetAttribute(kNameSpaceID_RDF, kResource2Atom, sortResource2, PR_FALSE);
		else	tree->UnsetAttribute(kNameSpaceID_RDF, kResource2Atom, PR_FALSE);
	}
	else
	{
		tree->UnsetAttribute(kNameSpaceID_None, kSortActiveAtom, PR_FALSE);
		tree->UnsetAttribute(kNameSpaceID_None, kSortDirectionAtom, PR_FALSE);
		tree->UnsetAttribute(kNameSpaceID_RDF, kResourceAtom, PR_FALSE);
		tree->UnsetAttribute(kNameSpaceID_RDF, kResource2Atom, PR_FALSE);
	}

	// optional hint
	if (inbetweenSeparatorSort == PR_TRUE)
		tree->SetAttribute(kNameSpaceID_None, kSortSeparatorsAtom, trueStr, PR_FALSE);
	else	tree->UnsetAttribute(kNameSpaceID_None, kSortSeparatorsAtom, PR_FALSE);

	return(NS_OK);
}



nsresult
XULSortServiceImpl::NodeHasSortInfo(nsIContent *child, nsString &sortResource, nsString &sortDirection,
				nsString &sortResource2, PRBool &inbetweenSeparatorSort, PRBool &found)
{
	nsresult	rv;

	inbetweenSeparatorSort = PR_FALSE;
	found = PR_FALSE;

	nsAutoString	value;
	if (NS_SUCCEEDED(rv = child->GetAttribute(kNameSpaceID_None, kSortActiveAtom, value))
		&& (rv == NS_CONTENT_ATTR_HAS_VALUE))
	{
		if (value.EqualsIgnoreCase(trueStr))
		{
			if (NS_SUCCEEDED(rv = child->GetAttribute(kNameSpaceID_RDF, kResourceAtom,
				sortResource)) && (rv == NS_CONTENT_ATTR_HAS_VALUE))
			{
				if (NS_SUCCEEDED(rv = child->GetAttribute(kNameSpaceID_None, kSortDirectionAtom,
					sortDirection)) && (rv == NS_CONTENT_ATTR_HAS_VALUE))
				{
					found = PR_TRUE;

					// sort separator flag is optional
					if (NS_SUCCEEDED(rv = child->GetAttribute(kNameSpaceID_None, kSortSeparatorsAtom,
						value)) && (rv == NS_CONTENT_ATTR_HAS_VALUE))
					{
						if (value.EqualsIgnoreCase(trueStr))
						{
							inbetweenSeparatorSort = PR_TRUE;
						}
					}

					// secondary sort info is optional
					if (NS_FAILED(rv = child->GetAttribute(kNameSpaceID_RDF, kResource2Atom,
						sortResource2)) || (rv != NS_CONTENT_ATTR_HAS_VALUE))
					{
						sortResource2.Truncate();
					}
				}
			}
		}
	}
	return(NS_OK);
}



nsresult
XULSortServiceImpl::GetSortColumnInfo(nsIContent *tree, nsString &sortResource,
			nsString &sortDirection, nsString &sortResource2, PRBool &inbetweenSeparatorSort)
{
        nsCOMPtr<nsIContent>	child;
	PRBool			found = PR_FALSE;
	PRInt32			childIndex, nameSpaceID, numChildren;
	nsresult		rv;

	if (IsTreeElement(tree) && (NS_SUCCEEDED(rv = NodeHasSortInfo(tree, sortResource, sortDirection,
		sortResource2, inbetweenSeparatorSort, found)) && (found == PR_TRUE)))
	{
	}
	else
	{
		if (NS_FAILED(rv = tree->ChildCount(numChildren)))	return(rv);
		for (childIndex=0; childIndex<numChildren; childIndex++)
		{
			if (NS_FAILED(rv = tree->ChildAt(childIndex, *getter_AddRefs(child))))	return(rv);
			if (NS_FAILED(rv = child->GetNameSpaceID(nameSpaceID)))		return(rv);
			if (nameSpaceID != kNameSpaceID_XUL)				continue;

			nsCOMPtr<nsIAtom> tag;
			if (NS_FAILED(rv = child->GetTag(*getter_AddRefs(tag))))	return(rv);
			
			if (tag.get() == kTreeColGroupAtom)
			{
				if (NS_SUCCEEDED(rv = GetSortColumnInfo(child, sortResource, sortDirection,
					sortResource2, inbetweenSeparatorSort)))
				{
					break;
				}
			}
			else if (tag.get() == kTreeColAtom)
			{
				if (NS_SUCCEEDED(rv = NodeHasSortInfo(child, sortResource, sortDirection,
					sortResource2, inbetweenSeparatorSort, found)) && (found == PR_TRUE))
				{
					break;
				}
			}
			else
			{
				continue;
			}
		}
		SetSortHints(tree, sortResource, sortDirection, sortResource2, inbetweenSeparatorSort, found);
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
XULSortServiceImpl::GetNodeTextValue(sortPtr sortInfo, nsIContent *node, nsString & val)
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
XULSortServiceImpl::CompareNodes(nsIRDFNode *cellNode1, PRBool isCollationKey1,
				 nsIRDFNode *cellNode2, PRBool isCollationKey2,
				 PRBool &bothValid, PRInt32 & sortOrder)
{
	bothValid = PR_FALSE;
	sortOrder = 0;

	const PRUnichar			*uni1 = nsnull, *uni2 = nsnull;
	nsCOMPtr<nsIRDFLiteral>		literal1 = do_QueryInterface(cellNode1);
	nsCOMPtr<nsIRDFLiteral>		literal2 = do_QueryInterface(cellNode2);
	if (literal1)	literal1->GetValueConst(&uni1);
	if (literal2)	literal2->GetValueConst(&uni2);

	if (isCollationKey1 == PR_TRUE && isCollationKey2 == PR_TRUE)
	{
		bothValid = PR_TRUE;

		// sort collation keys
		if (collationService)
		{
			collationService->CompareSortKey(uni1, uni2, &sortOrder);
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
	else if (literal1 || literal2)
	{
		// not a collation key, but one or both are strings
		if (literal1 && literal2)
		{
			if ((*uni1) && (*uni2))
			{
				bothValid = PR_TRUE;
				sortOrder = nsCRT::strcasecmp(uni1, uni2);
			}
			else if (*uni1)	sortOrder = -1;
			else		sortOrder = 1;
		}
		else if (literal1)	sortOrder = -1;
		else			sortOrder = 1;
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
			bothValid = PR_TRUE;
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
				bothValid = PR_TRUE;
				sortOrder = 0;
				if (LL_CMP(dateVal1, <, dateVal2))	sortOrder = -1;
				else if (LL_CMP(dateVal1, >, dateVal2))	sortOrder = 1;
			}
		}
	}
	return(NS_OK);
}



nsresult
XULSortServiceImpl::GetCachedTarget(sortPtr sortInfo, PRBool useCache, nsIRDFResource* aSource,
		nsIRDFResource *aProperty, PRBool aTruthValue, nsIRDFNode **aResult)
{
	nsresult	rv;

	*aResult = nsnull;

	if (!(sortInfo->mInner))
	{
		// if we don't have a mInner, create one
		rv = nsComponentManager::CreateInstance(kRDFInMemoryDataSourceCID,
			nsnull, NS_GET_IID(nsIRDFDataSource), (void **)&(sortInfo->mInner));
		if (NS_FAILED(rv))	return(rv);
	}

	rv = NS_RDF_NO_VALUE;
	if (sortInfo->mInner)
	{
		if (useCache == PR_TRUE)
		{
			// if we do have a mInner, look for the resource in it
			rv = sortInfo->mInner->GetTarget(aSource, aProperty, aTruthValue, aResult);
		}
		else if (sortInfo->db)
		{
			// if we don't have a cached value, look it up in the document's DB
			if (NS_SUCCEEDED(rv = (sortInfo->db)->GetTarget(aSource, aProperty,
				aTruthValue, aResult)) && (rv != NS_RDF_NO_VALUE))
			{
				// and if we have a value, cache it away in our mInner also (ignore errors)
				sortInfo->mInner->Assert(aSource, aProperty, *aResult, PR_TRUE);
			}
		}
	}
	return(rv);
}



nsresult
XULSortServiceImpl::GetResourceValue(nsIRDFResource *res1, sortPtr sortInfo, PRBool first, PRBool useCache,
				nsIRDFNode **target, PRBool &isCollationKey)
{
	nsresult		rv = NS_OK;

	*target = nsnull;
	isCollationKey = PR_FALSE;

	if ((res1) && (sortInfo->naturalOrderSort == PR_FALSE))
	{
		nsCOMPtr<nsIRDFResource>	modSortRes;

		// for any given property, first ask the graph for its value with "?collation=true" appended
		// to indicate that if there is a collation key available for this value, we want it

		modSortRes = (first) ? sortInfo->sortPropertyColl : sortInfo->sortPropertyColl2;
		if (modSortRes)
		{
			if (NS_SUCCEEDED(rv = GetCachedTarget(sortInfo, useCache, res1, modSortRes,
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
			modSortRes = (first) ? sortInfo->sortPropertySort : sortInfo->sortPropertySort2;
			if (modSortRes)
			{
				if (NS_SUCCEEDED(rv = GetCachedTarget(sortInfo, useCache, res1, modSortRes,
					PR_TRUE, target)) && (rv != NS_RDF_NO_VALUE))
				{
				}
			}
		}
		if (!(*target))
		{
			// if no collation key and no special sorting value, just get the property value
			modSortRes = (first) ? sortInfo->sortProperty : sortInfo->sortProperty2;
			if (modSortRes)
			{
				if (NS_SUCCEEDED(rv = GetCachedTarget(sortInfo, useCache, res1, modSortRes,
					PR_TRUE, target) && (rv != NS_RDF_NO_VALUE)))
				{
				}
			}
		}
	}
	return(rv);
}



nsresult
XULSortServiceImpl::GetNodeValue(nsIContent *node1, sortPtr sortInfo, PRBool first,
				nsIRDFNode **theNode, PRBool &isCollationKey)
{
	nsresult			rv;
	nsCOMPtr<nsIRDFResource>	res1;

	isCollationKey = PR_FALSE;

	nsCOMPtr<nsIDOMXULElement>	dom1 = do_QueryInterface(node1);
	if (dom1)
	{
		if (NS_FAILED(rv = dom1->GetResource(getter_AddRefs(res1))))
		{
			res1 = nsnull;
		}
		// Note: don't check for res1 QI failure here.  It only succeeds for RDF nodes,
		// but for XUL nodes it will failure; in the failure case, the code below gets
		// the cell's text value straight from the DOM
	}
	else
	{
		nsCOMPtr<nsIDOMElement>	htmlDom = do_QueryInterface(node1);
		if (htmlDom)
		{
			nsAutoString	htmlID;
			if (NS_SUCCEEDED(rv = node1->GetAttribute(kNameSpaceID_None, kIdAtom, htmlID))
				&& (rv == NS_CONTENT_ATTR_HAS_VALUE))
			{
				if (NS_FAILED(rv = gRDFService->GetUnicodeResource(htmlID.GetUnicode(),
					getter_AddRefs(res1))))
				{
					res1 = nsnull;
				}
			}
		}
		else
		{
			return(NS_ERROR_FAILURE);
		}
	}
	
	if ((sortInfo->naturalOrderSort == PR_FALSE) && (sortInfo->sortProperty))
	{
		if (res1)
		{
			rv = GetResourceValue(res1, sortInfo, first, PR_TRUE, theNode, isCollationKey);
			if ((rv == NS_RDF_NO_VALUE) || (!*theNode))
			{
				rv = GetResourceValue(res1, sortInfo, first, PR_FALSE, theNode, isCollationKey);
			}
		}
		else
		{
			rv = NS_RDF_NO_VALUE;
		}

		if (NS_FAILED(rv) || (rv == NS_RDF_NO_VALUE))
		{
		        nsCOMPtr<nsIContent>	cell1;
		        if (sortInfo->colIndex >= 0)
		        {
				rv = GetTreeCell(sortInfo, node1, sortInfo->colIndex, getter_AddRefs(cell1));
			}
			else
			{
				cell1 = node1;
			}
			if (cell1)
			{
				nsAutoString		cellVal1;
				if (NS_SUCCEEDED(rv = GetNodeTextValue(sortInfo, cell1, cellVal1)) &&
					(rv != NS_RDF_NO_VALUE))
				{
					nsCOMPtr<nsIRDFLiteral>	nodeLiteral;
					rv = gRDFService->GetLiteral(cellVal1.GetUnicode(), getter_AddRefs(nodeLiteral));
					if (NS_SUCCEEDED(rv))
					{
						*theNode = nodeLiteral;
						NS_IF_ADDREF(*theNode);
						isCollationKey = PR_FALSE;
					}
				}
			}
		}
	}
	else if ((sortInfo->naturalOrderSort == PR_TRUE) && (sortInfo->parentContainer))
	{
		nsAutoString		cellPosVal1;

		// check to see if this is a RDF_Seq
		// Note: this code doesn't handle the aggregated Seq case especially well
		if ((res1) && (sortInfo->db))
		{
			nsCOMPtr<nsIRDFResource>	parentResource;
			nsCOMPtr<nsIDOMXULElement>	parentDOMNode = do_QueryInterface(sortInfo->parentContainer);
			if (parentDOMNode)
			{
				if (NS_FAILED(rv = parentDOMNode->GetResource(getter_AddRefs(parentResource))))
				{
					parentResource = nsnull;
				}
			}

			nsCOMPtr<nsISimpleEnumerator>	arcs;
			if ((parentResource) && (NS_SUCCEEDED(rv = sortInfo->db->ArcLabelsIn(res1, getter_AddRefs(arcs)))))
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

					nsCOMPtr<nsISimpleEnumerator>	srcs;
					if (NS_FAILED(rv = sortInfo->db->GetSources(property, res1, PR_TRUE, getter_AddRefs(srcs))))
						continue;
					PRBool	hasMoreSrcs = PR_TRUE;
					while(hasMoreSrcs)
					{
						if (NS_FAILED(rv = srcs->HasMoreElements(&hasMoreSrcs)))	break;
						if (hasMoreSrcs == PR_FALSE)	break;

						nsCOMPtr<nsISupports>	isupports2;
						if (NS_FAILED(rv = srcs->GetNext(getter_AddRefs(isupports2))))	break;
						nsCOMPtr<nsIRDFResource> src = do_QueryInterface(isupports2);
						if (!src)			continue;
						
						if (src == parentResource)
						{
							cellPosVal1 = uri;
							cellPosVal1.Cut(0, sizeof(kRDFNameSpace_Seq_Prefix)-1);

							// hack: assume that its a number, so pad out a bit
					                nsAutoString	zero("000000");
					                if (cellPosVal1.Length() < zero.Length())
					                {
								cellPosVal1.Insert(zero, 0, zero.Length() - cellPosVal1.Length());
					                }
							hasMore = PR_FALSE;
							hasMoreSrcs = PR_FALSE;
							break;
						}
					}
				}
			}
		}
		if (cellPosVal1.Length() > 0)
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
	PRBool			isCollationKey1 = PR_FALSE, isCollationKey2 = PR_FALSE;
	nsresult		rv;

	sortOrder = 0;

	nsCOMPtr<nsIRDFNode>	cellNode1, cellNode2;

	// rjc: in some cases, the first node is static while the second node changes
	// per comparison; in these circumstances, we can cache the first node
	if ((sortInfo->cacheFirstHint == PR_TRUE) && (sortInfo->cacheFirstNode))
	{
		cellNode1 = sortInfo->cacheFirstNode;
		isCollationKey1 = sortInfo->cacheIsFirstNodeCollationKey;
	}
	else
	{
		GetNodeValue(node1, sortInfo, PR_TRUE, getter_AddRefs(cellNode1), isCollationKey1);
		if (sortInfo->cacheFirstHint == PR_TRUE)
		{
			sortInfo->cacheFirstNode = cellNode1;
			sortInfo->cacheIsFirstNodeCollationKey = isCollationKey1;
		}
	}
	GetNodeValue(node2, sortInfo, PR_TRUE, getter_AddRefs(cellNode2), isCollationKey2);

	PRBool	bothValid = PR_FALSE;
	rv = CompareNodes(cellNode1, isCollationKey1, cellNode2, isCollationKey2, bothValid, sortOrder);

	if (sortOrder == 0)
	{
		// nodes appear to be equivalent, check for secondary sort criteria
		if (sortInfo->sortProperty2 != nsnull)
		{
			cellNode1 = nsnull;
			cellNode2 = nsnull;
			isCollationKey1 = PR_FALSE;
			isCollationKey2 = PR_FALSE;
			
			GetNodeValue(node1, sortInfo, PR_FALSE, getter_AddRefs(cellNode1), isCollationKey1);
			GetNodeValue(node2, sortInfo, PR_FALSE, getter_AddRefs(cellNode2), isCollationKey2);

			bothValid = PR_FALSE;
			rv = CompareNodes(cellNode1, isCollationKey1, cellNode2, isCollationKey2, bothValid, sortOrder);
		}
	}

	if ((bothValid == PR_TRUE) && (sortInfo->descendingSort == PR_TRUE))
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
XULSortServiceImpl::SortTreeChildren(nsIContent *container, sortPtr sortInfo)
{
	PRInt32			childIndex = 0, loop, numChildren = 0, numElements = 0, currentElement, nameSpaceID;
        nsCOMPtr<nsIContent>	child;
	nsresult		rv;

	if (NS_FAILED(rv = container->ChildCount(numChildren)))	return(rv);
	if (numChildren < 1)	return(NS_OK);

	// Note: This is a straight allocation (not a COMPtr) so we
	// can't return out of this routine until/unless we free it!
	nsIContent ** flatArray = new nsIContent*[numChildren + 1];
	if (!flatArray)	return(NS_ERROR_OUT_OF_MEMORY);

	// Note: walk backwards (and add nodes into the array backwards) because
	// we also remove the nodes in this loop [via RemoveChildAt()] and if we
	// were to do this in a forward-looking manner it would be harder
	// (since we also skip over non XUL:treeitem nodes)

	nsCOMPtr<nsIAtom> tag;
	currentElement = numChildren;
	for (childIndex=numChildren-1; childIndex >= 0; childIndex--)
	{
		if (NS_FAILED(rv = container->ChildAt(childIndex, *getter_AddRefs(child))))	continue;
		if (NS_FAILED(rv = child->GetNameSpaceID(nameSpaceID)))	continue;
		if (nameSpaceID == kNameSpaceID_XUL)
		{
			if (NS_FAILED(rv = child->GetTag(*getter_AddRefs(tag))))	continue;
			if (tag.get() == kTreeItemAtom)
			{
				--currentElement;
				flatArray[currentElement] = child;
				NS_ADDREF(flatArray[currentElement]);		// Note: addref it here

				// Bug 6665. This is a hack to "addref" the resources
				// before we remove them from the content model. This
				// keeps them from getting released, which causes
				// performance problems for some datasources.
				nsIRDFResource	*resource;
				gXULUtils->GetElementResource(flatArray[currentElement], &resource);
				// Note: we don't release; see part deux below...

				++numElements;

				// immediately remove the child node, and ignore any errors
				container->RemoveChildAt(childIndex, PR_FALSE);
			}
		}
	}
	if (numElements > 0)
	{
		/* smart sorting (sort within separators) on name column */
		if (sortInfo->inbetweenSeparatorSort == PR_TRUE)
		{
			PRInt32	startIndex=currentElement;
			nsAutoString	type;
			for (loop=currentElement; loop< currentElement + numElements; loop++)
			{
				if (NS_SUCCEEDED(rv = flatArray[loop]->GetAttribute(kNameSpaceID_None, kRDF_type, type))
					&& (rv == NS_CONTENT_ATTR_HAS_VALUE))
				{
					if (type.Equals(kURINC_BookmarkSeparator))
					{
						if (loop > startIndex+1)
						{
							NS_QuickSort((void *)&flatArray[startIndex], loop-startIndex,
								sizeof(nsIContent *), inplaceSortCallback, (void *)sortInfo);
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
			NS_QuickSort((void *)(&flatArray[currentElement]), numElements, sizeof(nsIContent *),
				inplaceSortCallback, (void *)sortInfo);
		}

		nsCOMPtr<nsIContent>	parentNode;
		nsAutoString		value;
		PRInt32			childPos = 0;
		// recurse on grandchildren
		for (loop=currentElement; loop < currentElement + numElements; loop++)
		{
			container->InsertChildAt((nsIContent *)flatArray[loop], childPos++, PR_FALSE);

			// Bug 6665, part deux. The Big Hack.
			nsIRDFResource	*resource;
			gXULUtils->GetElementResource(flatArray[loop], &resource);
			nsrefcnt	refcnt;
			NS_RELEASE2(resource, refcnt);
			NS_RELEASE(resource);

			parentNode = (nsIContent *)flatArray[loop];
			NS_RELEASE(flatArray[loop]);			// Note: release it here

			// if its a container, find its treechildren node, and sort those
			if (NS_FAILED(rv = parentNode->GetAttribute(kNameSpaceID_None, kContainerAtom, value)) ||
				(rv != NS_CONTENT_ATTR_HAS_VALUE) || (!value.EqualsIgnoreCase(trueStr)))
				continue;
			if (NS_FAILED(rv = parentNode->ChildCount(numChildren)))	continue;

			for (childIndex=0; childIndex<numChildren; childIndex++)
			{
				if (NS_FAILED(rv = parentNode->ChildAt(childIndex, *getter_AddRefs(child))))
					continue;
				if (NS_FAILED(rv = child->GetNameSpaceID(nameSpaceID)))	continue;
				if (nameSpaceID != kNameSpaceID_XUL)	continue;

				if (NS_FAILED(rv = child->GetTag(*getter_AddRefs(tag))))	continue;
				if (tag.get() != kTreeChildrenAtom)	continue;

				sortInfo->parentContainer = parentNode;
				SortTreeChildren(child, sortInfo);
			}
		}
	}
	delete [] flatArray;
	flatArray = nsnull;

	return(NS_OK);
}


// rjc: yes, I'm lame. For the moment, "class sortState" is defined both here and in
// nsRDFGenericBuilder.cpp so any changes made here must also (exactly) be made there also.

typedef	class	sortState
{
public:
	nsAutoString				sortResource, sortResource2;

	nsCOMPtr<nsIRDFDataSource>		mCache;
	nsCOMPtr<nsIRDFResource>		sortProperty, sortProperty2;
	nsCOMPtr<nsIRDFResource>		sortPropertyColl, sortPropertyColl2;
	nsCOMPtr<nsIRDFResource>		sortPropertySort, sortPropertySort2;
} sortStateClass;



NS_IMETHODIMP
XULSortServiceImpl::InsertContainerNode(nsIRDFCompositeDataSource *db, sortStateClass *sortState, nsIContent *root,
					nsIContent *trueParent, nsIContent *container, nsIContent *node, PRBool aNotify)
{
	nsresult	rv;
	nsAutoString	sortResource, sortDirection, sortResource2;
	_sortStruct	sortInfo;

	// get composite db for tree
	sortInfo.rdfService = gRDFService;
	sortInfo.db = db;
	sortInfo.resCache = nsnull;
	sortInfo.colIndex = -1;
	sortInfo.parentContainer = trueParent;
	sortInfo.kTreeCellAtom = kTreeCellAtom;
	sortInfo.kNameSpaceID_XUL = kNameSpaceID_XUL;
	sortInfo.sortProperty = nsnull;
	sortInfo.sortProperty2 = nsnull;
	sortInfo.inbetweenSeparatorSort = PR_FALSE;
	sortInfo.cacheFirstHint = PR_TRUE;
	sortInfo.cacheIsFirstNodeCollationKey = PR_FALSE;

	if (sortState->mCache)
	{
		sortInfo.mInner = sortState->mCache;		// Note: this can/might be null
	}
	else
	{
		sortInfo.mInner = nsnull;
	}

	PRBool			sortInfoAvailable = PR_FALSE;

	if (IsTreeElement(root) == PR_TRUE)
	{
		// tree, so look for treecol/treecolgroup node(s) which provide sorting info

		if (NS_SUCCEEDED(rv = GetSortColumnInfo(root, sortResource, sortDirection,
			sortResource2, sortInfo.inbetweenSeparatorSort)))
		{
			sortInfoAvailable = PR_TRUE;
		}
		else
		{
			sortDirection = naturalStr;
		}
	}
	else
	{
		// not a tree, so look for sorting info on root node

		if (NS_SUCCEEDED(rv = root->GetAttribute(kNameSpaceID_None, kSortResourceAtom,
			sortResource)) && (rv == NS_CONTENT_ATTR_HAS_VALUE))
		{
			if (NS_SUCCEEDED(rv = root->GetAttribute(kNameSpaceID_None, kSortDirectionAtom,
				sortDirection)) && (rv == NS_CONTENT_ATTR_HAS_VALUE))
			{
				sortInfoAvailable = PR_TRUE;

				if (NS_FAILED(rv = root->GetAttribute(kNameSpaceID_None, kSortResource2Atom,
					sortResource2)) || (rv != NS_CONTENT_ATTR_HAS_VALUE))
				{
					sortResource2.Truncate();
				}
			}
		}
	}

	if (sortInfoAvailable)
	{
		if (sortState->sortResource.Equals(sortResource) && sortState->sortResource2.Equals(sortResource2))
		{
			sortInfo.sortProperty = sortState->sortProperty;
			sortInfo.sortProperty2 = sortState->sortProperty2;
			sortInfo.sortPropertyColl = sortState->sortPropertyColl;
			sortInfo.sortPropertyColl2 = sortState->sortPropertyColl2;
			sortInfo.sortPropertySort = sortState->sortPropertySort;
			sortInfo.sortPropertySort2 = sortState->sortPropertySort2;
		}
		else
		{
			nsAutoString	temp;

			// either first time, or must have changing sorting info, so flush state cache
			sortState->sortProperty = nsnull;	sortState->sortProperty2 = nsnull;
			sortState->sortPropertyColl = nsnull;	sortState->sortPropertyColl2 = nsnull;
			sortState->sortPropertySort = nsnull;	sortState->sortPropertySort2 = nsnull;

			rv = gRDFService->GetUnicodeResource(sortResource.GetUnicode(), getter_AddRefs(sortInfo.sortProperty));
			if (NS_FAILED(rv))	return(rv);
			sortState->sortResource = sortResource;
			sortState->sortProperty = sortInfo.sortProperty;
			
			temp = sortResource;
			temp += "?collation=true";
			rv = gRDFService->GetUnicodeResource(temp.GetUnicode(), getter_AddRefs(sortInfo.sortPropertyColl));
			if (NS_FAILED(rv))	return(rv);
			sortState->sortPropertyColl = sortInfo.sortPropertyColl;

			temp = sortResource;
			temp += "?sort=true";
			rv = gRDFService->GetUnicodeResource(temp.GetUnicode(), getter_AddRefs(sortInfo.sortPropertySort));
			if (NS_FAILED(rv))	return(rv);
			sortState->sortPropertySort = sortInfo.sortPropertySort;

			if (sortResource2.Length() > 0)
			{
				rv = gRDFService->GetUnicodeResource(sortResource2.GetUnicode(), getter_AddRefs(sortInfo.sortProperty2));
				if (NS_FAILED(rv))	return(rv);
				sortState->sortResource2 = sortResource2;
				sortState->sortProperty2 = sortInfo.sortProperty2;

				temp = sortResource2;
				temp += "?collation=true";
				rv = gRDFService->GetUnicodeResource(temp.GetUnicode(), getter_AddRefs(sortInfo.sortPropertyColl2));
				if (NS_FAILED(rv))	return(rv);
				sortState->sortPropertyColl2 = sortInfo.sortPropertyColl2;

				temp = sortResource2;
				temp += "?sort=true";
				rv = gRDFService->GetUnicodeResource(temp.GetUnicode(), getter_AddRefs(sortInfo.sortPropertySort2));
				if (NS_FAILED(rv))	return(rv);
				sortState->sortPropertySort2 = sortInfo.sortPropertySort2;
			}
		}
	}
	else
	{
		// either first time, or must have changing sorting info, so flush state cache
		sortState->sortResource.Truncate();
		sortState->sortResource2.Truncate();

		sortState->sortProperty = nsnull;	sortState->sortProperty2 = nsnull;
		sortState->sortPropertyColl = nsnull;	sortState->sortPropertyColl2 = nsnull;
		sortState->sortPropertySort = nsnull;	sortState->sortPropertySort2 = nsnull;
	}

	// set up sort order info
	sortInfo.naturalOrderSort = PR_FALSE;
	sortInfo.descendingSort = PR_FALSE;
	if (sortDirection.Equals(descendingStr))
	{
		sortInfo.descendingSort = PR_TRUE;
	}
	else if (!sortDirection.Equals(ascendingStr))
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
			nsAutoString	id;
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
					// Note: since cacheFirstHint is PR_TRUE, the first node passed
					// into inplaceSortCallback() must be the node that doesn't change
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
						PRInt32		thePos = ((direction > 0) ? current + 1: (current >= 0) ? current : 0);
						container->InsertChildAt(node, thePos, aNotify);
					}
					childAdded = PR_TRUE;
					break;
				}
				last = current;
			}
		}
	}

	if (childAdded == PR_FALSE)
	{
		container->AppendChildTo(node, aNotify);
	}

	if ((!sortState->mCache) && (sortInfo.mInner))
	{
		sortState->mCache = sortInfo.mInner;
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
	PRInt32		treeBodyIndex;
	nsresult	rv;
	_sortStruct	sortInfo;

	// get tree node
	nsCOMPtr<nsIContent>	contentNode = do_QueryInterface(node);
	if (!contentNode)	return(NS_ERROR_FAILURE);
	nsCOMPtr<nsIContent>	treeNode;
	if (NS_FAILED(rv = FindTreeElement(nsnull, contentNode, getter_AddRefs(treeNode))))
		return(rv);
	nsCOMPtr<nsIDOMXULElement>	domXulTree = do_QueryInterface(treeNode);
	if (!domXulTree)	return(NS_ERROR_FAILURE);

	// get composite db for tree
	sortInfo.rdfService = gRDFService;
	sortInfo.db = nsnull;
	sortInfo.resCache = nsnull;
	sortInfo.mInner = nsnull;
	sortInfo.parentContainer = treeNode;
	sortInfo.inbetweenSeparatorSort = PR_FALSE;
	sortInfo.cacheFirstHint = PR_FALSE;
	sortInfo.cacheIsFirstNodeCollationKey = PR_FALSE;

	// remove any sort hints on tree root node
	treeNode->UnsetAttribute(kNameSpaceID_None, kSortActiveAtom, PR_FALSE);
	treeNode->UnsetAttribute(kNameSpaceID_None, kSortDirectionAtom, PR_FALSE);
	treeNode->UnsetAttribute(kNameSpaceID_None, kSortSeparatorsAtom, PR_FALSE);
	treeNode->UnsetAttribute(kNameSpaceID_RDF, kResourceAtom, PR_FALSE);
	treeNode->UnsetAttribute(kNameSpaceID_RDF, kResource2Atom, PR_FALSE);

	nsCOMPtr<nsIRDFCompositeDataSource>	cds;
	if (NS_SUCCEEDED(rv = domXulTree->GetDatabase(getter_AddRefs(cds))))
	{
		sortInfo.db = cds;
	}

	sortInfo.kTreeCellAtom = kTreeCellAtom;
	sortInfo.kNameSpaceID_XUL = kNameSpaceID_XUL;

	// determine new sort resource and direction to use
	if (sortDirection.Equals(naturalStr))
	{
		sortInfo.naturalOrderSort = PR_TRUE;
		sortInfo.descendingSort = PR_FALSE;
	}
	else
	{
		sortInfo.naturalOrderSort = PR_FALSE;
		if (sortDirection.Equals(ascendingStr))		sortInfo.descendingSort = PR_FALSE;
		else if (sortDirection.Equals(descendingStr))	sortInfo.descendingSort = PR_TRUE;
	}

	// get index of sort column, find tree body, and sort. The sort
	// _won't_ send any notifications, so we won't trigger any reflows...
	nsAutoString	sortResource2;
	PRBool		found;
	if (NS_FAILED(rv = GetSortColumnIndex(treeNode, sortResource, sortDirection, sortResource2,
		sortInfo.inbetweenSeparatorSort, sortInfo.colIndex, found)))	return(rv);

	rv = gRDFService->GetUnicodeResource(sortResource.GetUnicode(), getter_AddRefs(sortInfo.sortProperty));
	if (NS_FAILED(rv))	return(rv);

	nsAutoString	temp;
	temp = sortResource;
	temp += "?collation=true";
	rv = gRDFService->GetUnicodeResource(temp.GetUnicode(), getter_AddRefs(sortInfo.sortPropertyColl));
	if (NS_FAILED(rv))	return(rv);

	temp = sortResource;
	temp += "?sort=true";
	rv = gRDFService->GetUnicodeResource(temp.GetUnicode(), getter_AddRefs(sortInfo.sortPropertySort));
	if (NS_FAILED(rv))	return(rv);

	if (sortResource2.Length() > 0)
	{
		rv = gRDFService->GetUnicodeResource(sortResource2.GetUnicode(), getter_AddRefs(sortInfo.sortProperty2));
		if (NS_FAILED(rv))	return(rv);

		temp = sortResource2;
		temp += "?collation=true";
		rv = gRDFService->GetUnicodeResource(temp.GetUnicode(), getter_AddRefs(sortInfo.sortPropertyColl2));
		if (NS_FAILED(rv))	return(rv);

		temp = sortResource2;
		temp += "?sort=true";
		rv = gRDFService->GetUnicodeResource(temp.GetUnicode(), getter_AddRefs(sortInfo.sortPropertySort2));
		if (NS_FAILED(rv))	return(rv);
	}

	SetSortHints(treeNode, sortResource, sortDirection, sortResource2, sortInfo.inbetweenSeparatorSort, found);

	nsCOMPtr<nsIContent>	treeBody;
	if (NS_FAILED(rv = FindTreeChildrenElement(treeNode, getter_AddRefs(treeBody))))	return(rv);
	if (NS_SUCCEEDED(rv = SortTreeChildren(treeBody, &sortInfo)))
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
