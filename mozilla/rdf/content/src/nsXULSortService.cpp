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
 * Original Author(s):
 *   Robert John Churchill    <rjc@netscape.com>
 *   Chris Waterson           <waterson@netscape.com>
 *
 * Contributor(s): 
 *   Scott Putterman          <putterman@netscape.com>
 *   Pierre Phaneuf           <pp@ludusdesign.com>
 *
 *
 * This Original Code has been modified by IBM Corporation.
 * Modifications made by IBM described herein are
 * Copyright (c) International Business Machines
 * Corporation, 2000
 *
 * Modifications to Mozilla code or documentation
 * identified per MPL Section 3.3
 *
 * Date         Modified by     Description of modification
 * 03/27/2000   IBM Corp.       Added PR_CALLBACK for Optlink
 *                               use in OS2
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
#include "nsRDFSort.h"
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
#include "nsILocaleService.h"



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

typedef struct {
	nsIContent * content;
	nsCOMPtr<nsIRDFResource> resource; 
	nsCOMPtr<nsIRDFNode> collationNode1;
	nsCOMPtr<nsIRDFNode> collationNode2;
	nsCOMPtr<nsIRDFNode> sortNode1;
	nsCOMPtr<nsIRDFNode> sortNode2;
	nsCOMPtr<nsIRDFNode> node1;
	nsCOMPtr<nsIRDFNode> node2;
	PRBool checkedCollation1;
	PRBool checkedCollation2;
	PRBool checkedSort1;
	PRBool checkedSort2;
	PRBool checkedNode1;
	PRBool checkedNode2;
} contentSortInfo;

int		PR_CALLBACK inplaceSortCallback(const void *data1, const void *data2, void *privateData);
int		PR_CALLBACK testSortCallback(const void * data1, const void *data2, void *privateData);


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
	static nsIAtom		*kTemplateAtom;
	static nsIAtom		*kStaticHintAtom;
	static nsIAtom		*kStaticsSortLastHintAtom;
	static nsIAtom		*kBoxAtom;
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
	static nsIAtom		*kRefAtom;
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
static nsresult GetTarget(contentSortInfo *contentSortInfo, sortPtr sortInfo,  PRBool first, PRBool onlyCollationHint, PRBool truthValue,nsIRDFNode **target, PRBool &isCollationKey);
static nsresult	GetResourceValue(nsIRDFResource *res1, sortPtr sortInfo, PRBool first, PRBool useCache, PRBool onlyCollationHint, nsIRDFNode **, PRBool &isCollationKey);
static nsresult GetResourceValue(contentSortInfo *contentSortInfo, sortPtr sortInfo, PRBool first, PRBool useCache,	PRBool onlyCollationHint, nsIRDFNode **target, PRBool &isCollationKey);
static nsresult	GetNodeValue(nsIContent *node1, sortPtr sortInfo, PRBool first, PRBool onlyCollationHint, nsIRDFNode **, PRBool &isCollationKey);
static nsresult GetNodeValue(contentSortInfo *info1, sortPtr sortInfo, PRBool first, PRBool onlyCollationHint, nsIRDFNode **theNode, PRBool &isCollationKey);
static nsresult	GetTreeCell(sortPtr sortInfo, nsIContent *node, PRInt32 cellIndex, nsIContent **cell);
static nsresult	GetNodeTextValue(sortPtr sortInfo, nsIContent *node, nsString & val);

public:
    static nsresult	InplaceSort(nsIContent *node1, nsIContent *node2, sortPtr sortInfo, PRInt32 & sortOrder);
	static nsresult InplaceSort(contentSortInfo *info1, contentSortInfo *info2, sortPtr sortInfo, PRInt32 & sortOrder);
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

nsIAtom* XULSortServiceImpl::kTemplateAtom;
nsIAtom* XULSortServiceImpl::kStaticHintAtom;
nsIAtom* XULSortServiceImpl::kStaticsSortLastHintAtom;
nsIAtom* XULSortServiceImpl::kBoxAtom;
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
nsIAtom* XULSortServiceImpl::kRefAtom;
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
	        kTemplateAtom 	           	= NS_NewAtom("template");
	        kStaticHintAtom 	        = NS_NewAtom("staticHint");
	        kStaticsSortLastHintAtom 	= NS_NewAtom("sortStaticsLast");
	        kBoxAtom            		= NS_NewAtom("box");
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
		kRefAtom			= NS_NewAtom("ref");
		kIdAtom				= NS_NewAtom("id");
		kRDF_type			= NS_NewAtom("type");
 
 		trueStr.AssignWithConversion("true");
 		naturalStr.AssignWithConversion("natural");
		ascendingStr.AssignWithConversion("ascending");
		descendingStr.AssignWithConversion("descending");
 
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

		// get a locale service 
		nsCOMPtr<nsILocaleService> localeService = do_GetService(NS_LOCALESERVICE_PROGID, &rv);
		if (NS_SUCCEEDED(rv))
		{
			nsCOMPtr<nsILocale>	locale;
			if (NS_SUCCEEDED(rv = localeService->GetApplicationLocale(getter_AddRefs(locale))) && (locale))
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

			rv = mgr->RegisterNameSpace(NS_ConvertASCIItoUCS2(kXULNameSpaceURI), kNameSpaceID_XUL);
			NS_ASSERTION(NS_SUCCEEDED(rv), "unable to register XUL namespace");

			rv = mgr->RegisterNameSpace(NS_ConvertASCIItoUCS2(kRDFNameSpaceURI), kNameSpaceID_RDF);
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
		NS_IF_RELEASE(kTemplateAtom);
		NS_IF_RELEASE(kStaticHintAtom);
		NS_IF_RELEASE(kStaticsSortLastHintAtom);
		NS_IF_RELEASE(kBoxAtom);
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
	        NS_IF_RELEASE(kRefAtom);
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
			if (NS_SUCCEEDED(rv = element->GetTag(*getter_AddRefs(tag))))
			{
				if (tag.get() == kTreeAtom)
				{
					isTreeNode = PR_TRUE;
				}
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
			nsAutoString uni1Str(uni1);
			nsAutoString uni2Str(uni2);
			collationService->CompareSortKey(uni1Str, uni2Str, &sortOrder);
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
			sortOrder = 0;
			if (dateLiteral1 && dateLiteral2)
			{
				PRInt64			dateVal1, dateVal2;
				dateLiteral1->GetValue(&dateVal1);
				dateLiteral2->GetValue(&dateVal2);
				bothValid = PR_TRUE;
				if (LL_CMP(dateVal1, <, dateVal2))	sortOrder = -1;
				else if (LL_CMP(dateVal1, >, dateVal2))	sortOrder = 1;
			}
			else if (dateLiteral1)	sortOrder = -1;
			else if (dateLiteral2)	sortOrder = 1;
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
				PRBool onlyCollationHint, nsIRDFNode **target, PRBool &isCollationKey)
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
		if ((!(*target)) && (onlyCollationHint == PR_FALSE))
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
		if ((!(*target)) && (onlyCollationHint == PR_FALSE))
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
XULSortServiceImpl::GetTarget(contentSortInfo *contentSortInfo, sortPtr sortInfo,  PRBool first, PRBool onlyCollationHint, PRBool truthValue,
				   nsIRDFNode **target, PRBool &isCollationKey)
{

	nsresult rv;
	nsIRDFResource *resource = contentSortInfo->resource;

	if(first)
	{
		if(contentSortInfo->collationNode1)
		{
			*target = contentSortInfo->collationNode1;
			NS_IF_ADDREF(*target);
		}
		else if ( !contentSortInfo->checkedCollation1 && NS_SUCCEEDED(rv = (sortInfo->db)->GetTarget(resource, sortInfo->sortPropertyColl,
				truthValue, target)))
		{
			if(rv != NS_RDF_NO_VALUE)
				contentSortInfo->collationNode1 = *target;
			contentSortInfo->checkedCollation1 = PR_TRUE;
		}
	
		if(*target)
		{
			isCollationKey = PR_TRUE;
			return NS_OK;
		}

		if(onlyCollationHint == PR_FALSE)
		{
			if(contentSortInfo->sortNode1)
			{
				*target = contentSortInfo->sortNode1;
				NS_IF_ADDREF(*target);
			}
			else if (!contentSortInfo->checkedSort1 && NS_SUCCEEDED(rv = (sortInfo->db)->GetTarget(resource, sortInfo->sortPropertySort,
				truthValue, target)))
			{
				if(rv != NS_RDF_NO_VALUE)
					contentSortInfo->sortNode1 = *target;
				contentSortInfo->checkedSort1 = PR_TRUE;
			}

			if(*target)
				return NS_OK;

			if(contentSortInfo->node1)
			{
				*target = contentSortInfo->node1;
				NS_IF_ADDREF(*target);
			}
			else if (!contentSortInfo->checkedNode1 && NS_SUCCEEDED(rv = (sortInfo->db)->GetTarget(resource, sortInfo->sortProperty,
				truthValue, target)))
			{
				if(rv != NS_RDF_NO_VALUE)
					contentSortInfo->node1 = *target;
				contentSortInfo->checkedNode1 = PR_TRUE;
			}

			if(*target)
				return NS_OK;
		}
	}
	else
	{
		if(contentSortInfo->collationNode2)
		{
			*target = contentSortInfo->collationNode2;
			NS_IF_ADDREF(*target);
		}
		else if (!contentSortInfo->checkedCollation2 && NS_SUCCEEDED(rv = (sortInfo->db)->GetTarget(resource, sortInfo->sortPropertyColl2,
				truthValue, target)))
		{
			if(rv != NS_RDF_NO_VALUE)
				contentSortInfo->collationNode2 = *target;
			contentSortInfo->checkedCollation2 = PR_TRUE;
		}
	
		if(*target)
		{
			isCollationKey = PR_TRUE;
			return NS_OK;
		}

		if(onlyCollationHint == PR_FALSE)
		{
			if(contentSortInfo->sortNode2)
			{
				*target = contentSortInfo->sortNode2;
				NS_IF_ADDREF(*target);
			}
			else if (!contentSortInfo->checkedSort2 && NS_SUCCEEDED(rv = (sortInfo->db)->GetTarget(resource, sortInfo->sortPropertySort2,
				truthValue, target)))
			{
				if(rv != NS_RDF_NO_VALUE)
					contentSortInfo->sortNode2 = *target;
				contentSortInfo->checkedSort2 = PR_TRUE;
			}

			if(*target)
				return NS_OK;

			if(contentSortInfo->node2)
			{
				*target = contentSortInfo->node2;
				NS_IF_ADDREF(*target);
			}
			else if (!contentSortInfo->checkedNode2 && NS_SUCCEEDED(rv = (sortInfo->db)->GetTarget(resource, sortInfo->sortProperty2,
				truthValue, target)))
			{
				if(rv != NS_RDF_NO_VALUE)
					contentSortInfo->node2 = *target;
				contentSortInfo->checkedNode2 = PR_TRUE;
			}

			if(*target)
				return NS_OK;
		}
	}

			
	return NS_RDF_NO_VALUE;
}



nsresult
XULSortServiceImpl::GetResourceValue(contentSortInfo *contentSortInfo, sortPtr sortInfo, PRBool first, PRBool useCache,
				PRBool onlyCollationHint, nsIRDFNode **target, PRBool &isCollationKey)
{
	nsresult		rv = NS_OK;

	*target = nsnull;
	isCollationKey = PR_FALSE;

	nsIRDFResource *res1 = contentSortInfo->resource;

	if ((res1) && (sortInfo->naturalOrderSort == PR_FALSE))
	{

		rv = GetTarget(contentSortInfo, sortInfo, first, onlyCollationHint, PR_TRUE, target, isCollationKey);
	}
	return(rv);
}



nsresult
XULSortServiceImpl::GetNodeValue(nsIContent *node1, sortPtr sortInfo, PRBool first,
				PRBool onlyCollationHint, nsIRDFNode **theNode, PRBool &isCollationKey)
{
	nsresult			rv;
	nsCOMPtr<nsIRDFResource>	res1;

	*theNode = nsnull;
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
			rv = GetResourceValue(res1, sortInfo, first, PR_TRUE, onlyCollationHint, theNode, isCollationKey);
			if ((rv == NS_RDF_NO_VALUE) || (!*theNode))
			{
				rv = GetResourceValue(res1, sortInfo, first, PR_FALSE, onlyCollationHint, theNode, isCollationKey);
			}
		}
		else
		{
			rv = NS_RDF_NO_VALUE;
		}

		if ((onlyCollationHint == PR_FALSE) && (NS_FAILED(rv) || (rv == NS_RDF_NO_VALUE)))
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
							cellPosVal1.AssignWithConversion(uri);
							cellPosVal1.Cut(0, sizeof(kRDFNameSpace_Seq_Prefix)-1);

							// hack: assume that its a number, so pad out a bit
					                nsAutoString	zero; zero.AssignWithConversion("000000");
					                if (cellPosVal1.Length() < zero.Length())
					                {
								cellPosVal1.Insert(zero.GetUnicode(), 0, PRInt32(zero.Length() - cellPosVal1.Length()));
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
XULSortServiceImpl::GetNodeValue(contentSortInfo *info1, sortPtr sortInfo, PRBool first,
				PRBool onlyCollationHint, nsIRDFNode **theNode, PRBool &isCollationKey)
{
	nsresult			rv;
	nsCOMPtr<nsIRDFResource>	res1;

	nsIContent *node1 = info1->content;

	*theNode = nsnull;
	isCollationKey = PR_FALSE;

	nsCOMPtr<nsIDOMXULElement>	dom1 = do_QueryInterface(node1);
	if(dom1)
	{
		res1 = info1->resource;
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
				info1->resource = res1;
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
			rv = GetResourceValue(info1, sortInfo, first, PR_TRUE, onlyCollationHint, theNode, isCollationKey);
			if ((rv == NS_RDF_NO_VALUE) || (!*theNode))
			{
				rv = GetResourceValue(info1, sortInfo, first, PR_FALSE, onlyCollationHint, theNode, isCollationKey);
			}
		}
		else
		{
			rv = NS_RDF_NO_VALUE;
		}

		if ((onlyCollationHint == PR_FALSE) && (NS_FAILED(rv) || (rv == NS_RDF_NO_VALUE)))
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
							cellPosVal1.AssignWithConversion(uri);
							cellPosVal1.Cut(0, sizeof(kRDFNameSpace_Seq_Prefix)-1);

							// hack: assume that its a number, so pad out a bit
					                nsAutoString	zero; zero.AssignWithConversion("000000");
					                if (cellPosVal1.Length() < zero.Length())
					                {
								cellPosVal1.Insert(zero.GetUnicode(), 0, zero.Length() - cellPosVal1.Length());
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
		GetNodeValue(node1, sortInfo, PR_TRUE, PR_FALSE, getter_AddRefs(cellNode1), isCollationKey1);
		if (sortInfo->cacheFirstHint == PR_TRUE)
		{
			sortInfo->cacheFirstNode = cellNode1;
			sortInfo->cacheIsFirstNodeCollationKey = isCollationKey1;
		}
	}
	GetNodeValue(node2, sortInfo, PR_TRUE, isCollationKey1, getter_AddRefs(cellNode2), isCollationKey2);

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
			
			GetNodeValue(node1, sortInfo, PR_FALSE, PR_FALSE, getter_AddRefs(cellNode1), isCollationKey1);
			GetNodeValue(node2, sortInfo, PR_FALSE, isCollationKey1, getter_AddRefs(cellNode2), isCollationKey2);

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



nsresult
XULSortServiceImpl::InplaceSort(contentSortInfo *info1, contentSortInfo *info2, sortPtr sortInfo, PRInt32 & sortOrder)
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
		GetNodeValue(info1, sortInfo, PR_TRUE, PR_FALSE, getter_AddRefs(cellNode1), isCollationKey1);
		if (sortInfo->cacheFirstHint == PR_TRUE)
		{
			sortInfo->cacheFirstNode = cellNode1;
			sortInfo->cacheIsFirstNodeCollationKey = isCollationKey1;
		}
	}
	GetNodeValue(info2, sortInfo, PR_TRUE, isCollationKey1, getter_AddRefs(cellNode2), isCollationKey2);

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
			
			GetNodeValue(info1, sortInfo, PR_FALSE, PR_FALSE, getter_AddRefs(cellNode1), isCollationKey1);
			GetNodeValue(info2, sortInfo, PR_FALSE, isCollationKey1, getter_AddRefs(cellNode2), isCollationKey2);

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



int PR_CALLBACK
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



int PR_CALLBACK
testSortCallback(const void *data1, const void *data2, void *privateData)
{
	/// Note: inplaceSortCallback is a small C callback stub for NS_QuickSort

	_sortStruct		*sortInfo = (_sortStruct *)privateData;
	contentSortInfo	*info1 = *(contentSortInfo **)data1;
	contentSortInfo	*info2 = *(contentSortInfo **)data2;
	PRInt32			sortOrder = 0;
	nsresult		rv;

	if (nsnull != sortInfo)
	{
		rv = XULSortServiceImpl::InplaceSort(info1, info2, sortInfo, sortOrder);
	}
	return(sortOrder);
}



static contentSortInfo *
CreateContentSortInfo(nsIContent *content, nsIRDFResource * resource)
{
	contentSortInfo * info = new contentSortInfo;
	if(!info)
		return nsnull;

	info->content = content;
	NS_IF_ADDREF(info->content);

	info->resource = resource;

	info->checkedCollation1 = PR_FALSE;
	info->checkedCollation2 = PR_FALSE;
	info->checkedSort1 = PR_FALSE;
	info->checkedSort2 = PR_FALSE;
	info->checkedNode1 = PR_FALSE;
	info->checkedNode2 = PR_FALSE;

	return(info);
}



nsresult
XULSortServiceImpl::SortTreeChildren(nsIContent *container, sortPtr sortInfo)
{
	PRInt32			childIndex = 0, loop, numChildren = 0, numElements = 0, currentElement, nameSpaceID;
        nsCOMPtr<nsIContent>	child;
	nsresult		rv;

	if (NS_FAILED(rv = container->ChildCount(numChildren)))	return(rv);
	if (numChildren < 1)	return(NS_OK);

	nsCOMPtr<nsIDocument> doc;
	container->GetDocument(*getter_AddRefs(doc));
	if (!doc)	return(NS_ERROR_UNEXPECTED);

	// Note: This is a straight allocation (not a COMPtr) so we
	// can't return out of this routine until/unless we free it!
	contentSortInfo ** contentSortInfoArray = new contentSortInfo*[numChildren + 1];
	if(!contentSortInfoArray)	return(NS_ERROR_OUT_OF_MEMORY);

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

				nsIRDFResource	*resource;
				gXULUtils->GetElementResource(child, &resource);
				contentSortInfo *contentInfo = CreateContentSortInfo(child, resource);
				if(contentInfo)
					contentSortInfoArray[currentElement] = contentInfo;

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
				if (NS_SUCCEEDED(rv = contentSortInfoArray[loop]->content->GetAttribute(kNameSpaceID_None, kRDF_type, type))
					&& (rv == NS_CONTENT_ATTR_HAS_VALUE))
				{
					if (type.EqualsWithConversion(kURINC_BookmarkSeparator))
					{
						if (loop > startIndex+1)
						{
							NS_QuickSort((void *)&contentSortInfoArray[startIndex], loop-startIndex,
								sizeof(contentSortInfo *), testSortCallback, (void *)sortInfo);
							startIndex = loop+1;
						}
					}
				}
			}
			if (loop > startIndex+1)
			{
				NS_QuickSort((void *)&contentSortInfoArray[startIndex], loop-startIndex, sizeof(contentSortInfo *),
					testSortCallback, (void *)sortInfo);
				startIndex = loop+1;
			}
		}
		else
		{
			NS_QuickSort((void *)(&contentSortInfoArray[currentElement]), numElements, sizeof(contentSortInfo *),
				testSortCallback, (void *)sortInfo);
		}

		nsCOMPtr<nsIContent>	parentNode;
		nsAutoString		value;
		PRInt32			childPos = 0;
		// recurse on grandchildren
		for (loop=currentElement; loop < currentElement + numElements; loop++)
		{
			nsIContent* kid = NS_STATIC_CAST(nsIContent*, contentSortInfoArray[loop]->content);

			// Because InsertChildAt() only does a
			// "shallow" SetDocument(), we need to do a
			// "deep" one now...
			kid->SetDocument(doc, PR_TRUE, PR_TRUE);

			container->InsertChildAt(kid, childPos++, PR_FALSE);

			parentNode = (nsIContent *)contentSortInfoArray[loop]->content;

			contentSortInfo * contentSortInfo = contentSortInfoArray[loop];
			NS_RELEASE(contentSortInfo->content);
			delete contentSortInfo;

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
	delete [] contentSortInfoArray;
	contentSortInfoArray = nsnull;

	return(NS_OK);
}



NS_IMETHODIMP
XULSortServiceImpl::InsertContainerNode(nsIRDFCompositeDataSource *db, nsRDFSortState *sortState, nsIContent *root,
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
	sortInfo.cacheFirstNode = nsnull;
	sortInfo.cacheIsFirstNodeCollationKey = PR_FALSE;

	if (sortState->mCache)
	{
		sortInfo.mInner = sortState->mCache;		// Note: this can/might be null
	}
	else
	{
		sortInfo.mInner = nsnull;
	}

	if (container != sortState->lastContainer.get())
	{
		sortState->lastContainer = container;
		sortState->lastWasFirst = PR_FALSE;
		sortState->lastWasLast = PR_FALSE;
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
			sortDirection.Assign(naturalStr);
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
			temp.AppendWithConversion("?collation=true");
			rv = gRDFService->GetUnicodeResource(temp.GetUnicode(), getter_AddRefs(sortInfo.sortPropertyColl));
			if (NS_FAILED(rv))	return(rv);
			sortState->sortPropertyColl = sortInfo.sortPropertyColl;

			temp = sortResource;
			temp.AppendWithConversion("?sort=true");
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
				temp.AppendWithConversion("?collation=true");
				rv = gRDFService->GetUnicodeResource(temp.GetUnicode(), getter_AddRefs(sortInfo.sortPropertyColl2));
				if (NS_FAILED(rv))	return(rv);
				sortState->sortPropertyColl2 = sortInfo.sortPropertyColl2;

				temp = sortResource2;
				temp.AppendWithConversion("?sort=true");
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

		nsCOMPtr<nsIDocument>		doc;
		if (NS_SUCCEEDED(rv) && parent)
		{
			rv = parent->GetDocument(*getter_AddRefs(doc));
			if (! doc)
			{
				parent = nsnull;
			}
		}

		if (parent)
		{
			nsAutoString		id;
			nsCOMPtr<nsIAtom>	tag;

			if (NS_FAILED(rv = trueParent->GetTag(*getter_AddRefs(tag))))
				return(rv);

			// XXX hmmm... currently, if we just check for ref on "boxes" before
			// checking for id, this gives the sidebar huge fits.  So, for the
			// moment, until the solution is determined, let's restrict checking
			// for ref before id if we have a "box"
			if (tag.get() != kBoxAtom)
			{
				rv = trueParent->GetAttribute(kNameSpaceID_None, kRefAtom, id);
			}
			if (id.Length() == 0)
			{
				rv = trueParent->GetAttribute(kNameSpaceID_None, kIdAtom, id);
			}
			if (id.Length() > 0)
			{
				nsCOMPtr<nsIRDFResource>	containerRes;
				rv = gXULUtils->MakeElementResource(doc, id, getter_AddRefs(containerRes));

				if (NS_SUCCEEDED(rv))
				{
					rv = sortInfo.db->HasAssertion(containerRes, kRDF_instanceOf,
								kRDF_Seq, PR_TRUE, &isContainerRDFSeq);
				}
			}
		}
	}

	PRBool			childAdded = PR_FALSE;

	if ((sortInfo.naturalOrderSort == PR_FALSE) ||
		((sortInfo.naturalOrderSort == PR_TRUE) &&
		(isContainerRDFSeq == PR_TRUE)))
	{
	        nsCOMPtr<nsIContent>	child;
		PRInt32			numChildren = 0;
		if (NS_FAILED(rv = container->ChildCount(numChildren)))	return(rv);

		// rjc says: determine where static XUL ends and generated XUL/RDF begins
		PRInt32			staticCount = 0;

		nsAutoString		staticValue;
		if (NS_SUCCEEDED(rv = container->GetAttribute(kNameSpaceID_None, kStaticHintAtom, staticValue))
			&& (rv == NS_CONTENT_ATTR_HAS_VALUE))
		{
			// found "static" XUL element count hint
			PRInt32		strErr=0;
			staticCount = staticValue.ToInteger(&strErr);
			if (strErr)	staticCount = 0;
		}
		else
		{
			// compute the "static" XUL element count
			nsAutoString	valueStr;
			for (PRInt32 childLoop=0; childLoop<numChildren; childLoop++)
			{
				container->ChildAt(childLoop, *getter_AddRefs(child));
				if (!child)	break;

				if (NS_SUCCEEDED(rv = child->GetAttribute(kNameSpaceID_None, kTemplateAtom, valueStr))
					&& (rv == NS_CONTENT_ATTR_HAS_VALUE))
				{
					break;
				}
				else
				{
					++staticCount;
				}
			}
			if (NS_SUCCEEDED(rv = container->GetAttribute(kNameSpaceID_None, kStaticsSortLastHintAtom, valueStr))
				&& (rv == NS_CONTENT_ATTR_HAS_VALUE) && (valueStr.EqualsIgnoreCase(trueStr)))
			{
				// indicate that static XUL comes after RDF-generated content by making negative
				staticCount = -staticCount;
			}

			// save the "static" XUL element count hint
			valueStr.Truncate();
			valueStr.AppendInt(staticCount);
			container->SetAttribute(kNameSpaceID_None, kStaticHintAtom, valueStr, PR_FALSE);
		}

		if (staticCount <= 0)
		{
			staticCount = 0;
		}
		else if (staticCount > numChildren)
		{
			staticCount = numChildren;
		}
		numChildren -= staticCount;

		// figure out where to insert the node when a sort order is being imposed
		if (numChildren > 0)
		{
		        nsIContent		*temp;
		        PRInt32			direction;

			// rjc says: The following is an implementation of a fairly optimal
			// binary search insertion sort... with interpolation at either end-point.

			if (sortState->lastWasFirst == PR_TRUE)
			{
				container->ChildAt(staticCount, *getter_AddRefs(child));
				temp = child.get();
				direction = inplaceSortCallback(&node, &temp, &sortInfo);
				if (direction < 0)
				{
					container->InsertChildAt(node, staticCount, aNotify);
					childAdded = PR_TRUE;
				}
				else
				{
					sortState->lastWasFirst = PR_FALSE;
				}
			}
			else if (sortState->lastWasLast == PR_TRUE)
			{
				container->ChildAt(staticCount+numChildren-1, *getter_AddRefs(child));
				temp = child.get();
				direction = inplaceSortCallback(&node, &temp, &sortInfo);
				if (direction > 0)
				{
					container->AppendChildTo(node, aNotify);
					childAdded = PR_TRUE;
				}
				else
				{
					sortState->lastWasLast = PR_FALSE;
				}
			}

		        PRInt32			left = staticCount+1, right = staticCount+numChildren, x;
		        while ((childAdded == PR_FALSE) && (right >= left))
		        {
		        	x = (left + right) / 2;
				container->ChildAt(x-1, *getter_AddRefs(child));
				temp = child.get();

				// rjc says: since cacheFirstHint is PR_TRUE, the first node passed
				// into inplaceSortCallback() must be the node that doesn't change

				direction = inplaceSortCallback(&node, &temp, &sortInfo);
				if (((x == left) && (direction < 0)) || (((x == right)) && (direction > 0)) || (left == right))
				{
					PRInt32		thePos = ((direction > 0) ? x : x-1);
					container->InsertChildAt(node, thePos, aNotify);
					childAdded = PR_TRUE;
					
					sortState->lastWasFirst = (thePos == 0) ? PR_TRUE: PR_FALSE;
					sortState->lastWasLast = (thePos >= staticCount+numChildren) ? PR_TRUE: PR_FALSE;
					
					break;
				}
		        	if (direction < 0)	right = x-1;
		        	else			left = x+1;
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
	return DoSort(node, NS_ConvertASCIItoUCS2(sortResource), NS_ConvertASCIItoUCS2(sortDirection));
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
	sortInfo.cacheFirstNode = nsnull;
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
	temp.Assign(sortResource);
	temp.AppendWithConversion("?collation=true");
	rv = gRDFService->GetUnicodeResource(temp.GetUnicode(), getter_AddRefs(sortInfo.sortPropertyColl));
	if (NS_FAILED(rv))	return(rv);

	temp.Assign(sortResource);
	temp.AppendWithConversion("?sort=true");
	rv = gRDFService->GetUnicodeResource(temp.GetUnicode(), getter_AddRefs(sortInfo.sortPropertySort));
	if (NS_FAILED(rv))	return(rv);

	if (sortResource2.Length() > 0)
	{
		rv = gRDFService->GetUnicodeResource(sortResource2.GetUnicode(), getter_AddRefs(sortInfo.sortProperty2));
		if (NS_FAILED(rv))	return(rv);

		temp = sortResource2;
		temp.AppendWithConversion("?collation=true");
		rv = gRDFService->GetUnicodeResource(temp.GetUnicode(), getter_AddRefs(sortInfo.sortPropertyColl2));
		if (NS_FAILED(rv))	return(rv);

		temp = sortResource2;
		temp.AppendWithConversion("?sort=true");
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

	// We need to do a deep SetDocument(), because AppendChildTo()
	// only does a shallow one.
	nsCOMPtr<nsIDocument> doc;
	treeParent->GetDocument(*getter_AddRefs(doc));
	treeBody->SetDocument(doc, PR_TRUE, PR_TRUE);

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
