/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

/*
  This file provides the implementation for the sort service manager.
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
#include "nsIRDFCursor.h"
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
#include "rdf.h"
#include "rdfutil.h"

#include "nsVoidArray.h"
#include "rdf_qsort.h"
#include "nsIAtom.h"
#include "nsIXULSortService.h"
#include "nsString.h"
#include "plhash.h"
#include "plstr.h"
#include "prlog.h"

#include "nsCOMPtr.h"
#include "nsIServiceManager.h"
#include "nsINameSpaceManager.h"
#include "nsLayoutCID.h"
#include "nsRDFCID.h"

#include "nsIContent.h"
#include "nsITextContent.h"
#include "nsTextFragment.h"

#include "nsVoidArray.h"
#include "rdf_qsort.h"

#include "nsIDOMNode.h"
#include "nsIDOMElement.h"

#include "nsIRDFContentModelBuilder.h"
#include "nsIRDFCompositeDataSource.h"
#include "nsIRDFNode.h"
#include "nsIRDFService.h"
#include "rdf.h"


////////////////////////////////////////////////////////////////////////

static NS_DEFINE_IID(kXULSortServiceCID,      NS_XULSORTSERVICE_CID);
static NS_DEFINE_IID(kIXULSortServiceIID,     NS_IXULSORTSERVICE_IID);
static NS_DEFINE_IID(kISupportsIID,           NS_ISUPPORTS_IID);

static NS_DEFINE_CID(kNameSpaceManagerCID,    NS_NAMESPACEMANAGER_CID);
static NS_DEFINE_IID(kINameSpaceManagerIID,   NS_INAMESPACEMANAGER_IID);

static NS_DEFINE_IID(kIContentIID,            NS_ICONTENT_IID);
static NS_DEFINE_IID(kITextContentIID,        NS_ITEXT_CONTENT_IID);

static NS_DEFINE_IID(kIDomNodeIID,            NS_IDOMNODE_IID);
static NS_DEFINE_IID(kIDomElementIID,         NS_IDOMELEMENT_IID);

static NS_DEFINE_CID(kRDFServiceCID,          NS_RDFSERVICE_CID);
static NS_DEFINE_IID(kIRDFServiceIID,         NS_IRDFSERVICE_IID);
static NS_DEFINE_IID(kIRDFResourceIID,        NS_IRDFRESOURCE_IID);
static NS_DEFINE_IID(kIRDFLiteralIID,         NS_IRDFLITERAL_IID);

// XXX This is sure to change. Copied from mozilla/layout/xul/content/src/nsXULAtoms.cpp
static const char kXULNameSpaceURI[]
    = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";
DEFINE_RDF_VOCAB(RDF_NAMESPACE_URI, RDF, child);



typedef	struct	_sortStruct	{
    nsIRDFCompositeDataSource	*db;
    nsIRDFResource		*sortProperty;
    PRBool			descendingSort;
    PRBool			naturalOrderSort;
} sortStruct, *sortPtr;



int	openSortCallback(const void *data1, const void *data2, void *sortData);
int	inplaceSortCallback(const void *data1, const void *data2, void *sortData);


////////////////////////////////////////////////////////////////////////
// ServiceImpl
//
//   This is the sort service.
//
class XULSortServiceImpl : public nsIXULSortService
{
protected:
    XULSortServiceImpl(void);
    virtual ~XULSortServiceImpl(void);

    static nsIXULSortService* gXULSortService; // The one-and-only sort service

private:
    static nsrefcnt	gRefCnt;
    static nsIAtom	*kTreeAtom;
    static nsIAtom	*kTreeBodyAtom;
    static nsIAtom	*kTreeCellAtom;
    static nsIAtom	*kTreeChildrenAtom;
    static nsIAtom	*kTreeColAtom;
    static nsIAtom	*kTreeItemAtom;
    static nsIAtom	*kResourceAtom;
    static nsIAtom	*kTreeContentsGeneratedAtom;
    static nsIAtom	*kNameAtom;
    static nsIAtom	*kSortAtom;
    static nsIAtom	*kSortDirectionAtom;
    static nsIAtom	*kIdAtom;
    static nsIAtom	*kNaturalOrderPosAtom;

    static PRInt32	kNameSpaceID_XUL;
    static PRInt32	kNameSpaceID_RDF;

    static nsIRDFService	*gRDFService;

nsresult	FindTreeElement(nsIContent* aElement,nsIContent** aTreeElement);
nsresult	FindTreeBodyElement(nsIContent *tree, nsIContent **treeBody);
nsresult	GetSortColumnIndex(nsIContent *tree, nsString sortResource, nsString sortDirection, PRInt32 *colIndex);
nsresult	GetSortColumnInfo(nsIContent *tree, nsString &sortResource, nsString &sortDirection);
nsresult	GetTreeCell(nsIContent *node, PRInt32 colIndex, nsIContent **cell);
nsresult	GetTreeCellValue(nsIContent *node, nsString & value);
nsresult	RemoveAllChildren(nsIContent *node);
nsresult	SortTreeChildren(nsIContent *container, PRInt32 colIndex, sortPtr sortInfo, PRInt32 indentLevel);
nsresult	PrintTreeChildren(nsIContent *container, PRInt32 colIndex, PRInt32 indentLevel);

public:

    static nsresult GetSortService(nsIXULSortService** result);

    // nsISupports
    NS_DECL_ISUPPORTS

    // nsISortService
    NS_IMETHOD DoSort(nsIDOMNode* node, const nsString& sortResource, const nsString& sortDirection);
    NS_IMETHOD OpenContainer(nsIRDFCompositeDataSource *db, nsIContent *container, nsIRDFResource **flatArray,
				PRInt32 numElements, PRInt32 elementSize);
};

nsIXULSortService* XULSortServiceImpl::gXULSortService = nsnull;
nsrefcnt XULSortServiceImpl::gRefCnt = 0;

nsIRDFService *XULSortServiceImpl::gRDFService = nsnull;

nsIAtom* XULSortServiceImpl::kTreeAtom;
nsIAtom* XULSortServiceImpl::kTreeBodyAtom;
nsIAtom* XULSortServiceImpl::kTreeCellAtom;
nsIAtom* XULSortServiceImpl::kTreeChildrenAtom;
nsIAtom* XULSortServiceImpl::kTreeColAtom;
nsIAtom* XULSortServiceImpl::kTreeItemAtom;
nsIAtom* XULSortServiceImpl::kResourceAtom;
nsIAtom* XULSortServiceImpl::kTreeContentsGeneratedAtom;
nsIAtom* XULSortServiceImpl::kNameAtom;
nsIAtom* XULSortServiceImpl::kSortAtom;
nsIAtom* XULSortServiceImpl::kSortDirectionAtom;
nsIAtom* XULSortServiceImpl::kIdAtom;
nsIAtom* XULSortServiceImpl::kNaturalOrderPosAtom;

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
		kTreeContentsGeneratedAtom	= NS_NewAtom("treecontentsgenerated");
		kNameAtom			= NS_NewAtom("Name");
		kSortAtom			= NS_NewAtom("sortActive");
		kSortDirectionAtom		= NS_NewAtom("sortDirection");
		kIdAtom				= NS_NewAtom("id");
		kNaturalOrderPosAtom		= NS_NewAtom("pos");
 
		nsresult rv;

		if (NS_FAILED(rv = nsServiceManager::GetService(kRDFServiceCID,
						  kIRDFServiceIID, (nsISupports**) &gRDFService)))
		{
			NS_ERROR("couldn't create rdf service");
		}
	        // Register the XUL and RDF namespaces: these'll just retrieve
	        // the IDs if they've already been registered by someone else.
		nsINameSpaceManager* mgr;
		if (NS_SUCCEEDED(rv = nsComponentManager::CreateInstance(kNameSpaceManagerCID,
		                           nsnull,
		                           kINameSpaceManagerIID,
		                           (void**) &mgr)))
		{

// XXX This is sure to change. Copied from mozilla/layout/xul/content/src/nsXULAtoms.cpp
static const char kXULNameSpaceURI[]
    = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";

static const char kRDFNameSpaceURI[]
    = RDF_NAMESPACE_URI;

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
	--gRefCnt;
	if (gRefCnt == 0)
	{
		NS_RELEASE(kTreeAtom);
		NS_RELEASE(kTreeBodyAtom);
		NS_RELEASE(kTreeCellAtom);
	        NS_RELEASE(kTreeChildrenAtom);
	        NS_RELEASE(kTreeColAtom);
	        NS_RELEASE(kTreeItemAtom);
	        NS_RELEASE(kResourceAtom);
	        NS_RELEASE(kTreeContentsGeneratedAtom);
	        NS_RELEASE(kNameAtom);
	        NS_RELEASE(kSortAtom);
	        NS_RELEASE(kSortDirectionAtom);
	        NS_RELEASE(kIdAtom);
	        NS_RELEASE(kNaturalOrderPosAtom);

		nsServiceManager::ReleaseService(kRDFServiceCID, gRDFService);
		gRDFService = nsnull;
		nsServiceManager::ReleaseService(kXULSortServiceCID, gXULSortService);
		gXULSortService = nsnull;
	}
}


nsresult
XULSortServiceImpl::GetSortService(nsIXULSortService** mgr)
{
    if (! gXULSortService) {
        gXULSortService = new XULSortServiceImpl();
        if (! gXULSortService)
            return NS_ERROR_OUT_OF_MEMORY;
    }

    NS_ADDREF(gXULSortService);
    *mgr = gXULSortService;
    return NS_OK;
}



NS_IMETHODIMP_(nsrefcnt)
XULSortServiceImpl::AddRef(void)
{
    return 2;
}

NS_IMETHODIMP_(nsrefcnt)
XULSortServiceImpl::Release(void)
{
    return 1;
}


NS_IMPL_QUERY_INTERFACE(XULSortServiceImpl, kIXULSortServiceIID);


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
XULSortServiceImpl::FindTreeBodyElement(nsIContent *tree, nsIContent **treeBody)
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
			if (tag.get() == kTreeBodyAtom)
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
XULSortServiceImpl::GetSortColumnIndex(nsIContent *tree, nsString sortResource, nsString sortDirection, PRInt32 *sortColIndex)
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
				nsString	colResource;

				if (NS_OK == child->GetAttribute(kNameSpaceID_RDF, kResourceAtom, colResource))
				{
					if (colResource == sortResource)
					{
						nsString	trueStr("true");
						child->SetAttribute(kNameSpaceID_None, kSortAtom, trueStr, PR_TRUE);
						child->SetAttribute(kNameSpaceID_None, kSortDirectionAtom, sortDirection, PR_TRUE);
						*sortColIndex = colIndex;
						found = PR_TRUE;
						// Note: don't break, want to set/unset attribs on ALL sort columns
						// break;
					}
					else
					{
						nsString	falseStr("false");
						child->SetAttribute(kNameSpaceID_None, kSortAtom, falseStr, PR_TRUE);
						child->SetAttribute(kNameSpaceID_None, kSortDirectionAtom, sortDirection, PR_TRUE);
					}
				}
				++colIndex;
			}
		}
	}
	return((found == PR_TRUE) ? NS_OK : NS_ERROR_FAILURE);
}

nsresult
XULSortServiceImpl::GetSortColumnInfo(nsIContent *tree, nsString &sortResource, nsString &sortDirection)
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
				nsString	value;
				if (NS_OK == child->GetAttribute(kNameSpaceID_None, kSortAtom, value))
				{
					if (value.Equals("true"))
					{
						if (NS_OK == child->GetAttribute(kNameSpaceID_RDF, kResourceAtom, sortResource))
						{
							if (NS_OK == child->GetAttribute(kNameSpaceID_None, kSortDirectionAtom, sortDirection))
							{
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
XULSortServiceImpl::GetTreeCell(nsIContent *node, PRInt32 cellIndex, nsIContent **cell)
{
	PRBool			found = PR_FALSE;
	PRInt32			childIndex = 0, numChildren = 0, nameSpaceID;
        nsCOMPtr<nsIContent>	child;
	nsresult		rv;

	if (NS_FAILED(rv = node->ChildCount(numChildren)))	return(rv);

	for (childIndex=0; childIndex<numChildren; childIndex++)
	{
		if (NS_FAILED(rv = node->ChildAt(childIndex, *getter_AddRefs(child))))	break;;
		if (NS_FAILED(rv = child->GetNameSpaceID(nameSpaceID)))	break;
		if (nameSpaceID == kNameSpaceID_XUL)
		{
			nsCOMPtr<nsIAtom> tag;
			if (NS_FAILED(rv = child->GetTag(*getter_AddRefs(tag))))	return rv;
			if (tag.get() == kTreeCellAtom)
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
XULSortServiceImpl::GetTreeCellValue(nsIContent *node, nsString & val)
{
	PRBool			found = PR_FALSE;
	PRInt32			childIndex = 0, numChildren = 0, nameSpaceID;
        nsCOMPtr<nsIContent>	child;
	nsresult		rv;

	if (NS_FAILED(rv = node->ChildCount(numChildren)))	return(rv);

	for (childIndex=0; childIndex<numChildren; childIndex++)
	{
		if (NS_FAILED(rv = node->ChildAt(childIndex, *getter_AddRefs(child))))	break;;
		if (NS_FAILED(rv = child->GetNameSpaceID(nameSpaceID)))	break;
		// XXX is this the correct way to get text?  Probably not...
		if (nameSpaceID != kNameSpaceID_XUL)
		{
			nsITextContent	*text = nsnull;
			if (NS_SUCCEEDED(rv = child->QueryInterface(kITextContentIID, (void **)&text)))
			{
				const nsTextFragment	*textFrags;
				PRInt32		numTextFrags;
				if (NS_SUCCEEDED(rv = text->GetText(textFrags, numTextFrags)))
				{
					const char *value = textFrags->Get1b();
					PRInt32 len = textFrags->GetLength();
					val.SetString(value, len);
					found = PR_TRUE;
					break;
				}
			}
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
		container->RemoveChildAt(childIndex, PR_TRUE);
	}
	return(rv);
}



int
openSortCallback(const void *data1, const void *data2, void *sortData)
{
	int		sortOrder = 0;
	nsresult	rv;

	nsIRDFNode	*node1, *node2;
	node1 = *(nsIRDFNode **)data1;
	node2 = *(nsIRDFNode **)data2;
	_sortStruct	*sortPtr = (_sortStruct *)sortData;

	nsIRDFResource	*res1;
	nsIRDFResource	*res2;
	const PRUnichar	*uniStr1 = nsnull;
	const PRUnichar	*uniStr2 = nsnull;

	if (NS_SUCCEEDED(node1->QueryInterface(kIRDFResourceIID, (void **) &res1)))
	{
		nsIRDFNode	*nodeVal1;
		if (NS_SUCCEEDED(rv = sortPtr->db->GetTarget(res1, sortPtr->sortProperty, PR_TRUE, &nodeVal1)))
		{
			nsIRDFLiteral *literal1;
			if (NS_SUCCEEDED(nodeVal1->QueryInterface(kIRDFLiteralIID, (void **) &literal1)))
			{
				literal1->GetValue(&uniStr1);
			}
		}
	}
	if (NS_SUCCEEDED(node2->QueryInterface(kIRDFResourceIID, (void **) &res2)))
	{
		nsIRDFNode	*nodeVal2;
		if (NS_SUCCEEDED(rv = sortPtr->db->GetTarget(res2, sortPtr->sortProperty, PR_TRUE, &nodeVal2)))
		{
			nsIRDFLiteral	*literal2;
			if (NS_SUCCEEDED(nodeVal2->QueryInterface(kIRDFLiteralIID, (void **) &literal2)))
			{
				literal2->GetValue(&uniStr2);
			}
		}
	}
	if ((uniStr1 != nsnull) && (uniStr2 != nsnull))
	{
		nsAutoString	str1(uniStr1), str2(uniStr2);
		sortOrder = (int)str1.Compare(str2, PR_TRUE);
		if (sortPtr->descendingSort == PR_TRUE)
		{
			sortOrder = -sortOrder;
		}
	}
	else if ((uniStr1 != nsnull) && (uniStr2 == nsnull))
	{
		sortOrder = -1;
	}
	else
	{
		sortOrder = 1;
	}
	return(sortOrder);
}


int
inplaceSortCallback(const void *data1, const void *data2, void *sortData)
{
	char		*name1, *name2;
	name1 = *(char **)data1;
	name2 = *(char **)data2;
	_sortStruct	*sortPtr = (_sortStruct *)sortData;

	PRInt32		sortOrder=0;
	nsAutoString	str1(name1), str2(name2);

	sortOrder = str1.Compare(name2, PR_TRUE);
	if (sortPtr->descendingSort == PR_TRUE)
	{
		sortOrder = -sortOrder;
	}
	return(sortOrder);
}



nsresult
XULSortServiceImpl::SortTreeChildren(nsIContent *container, PRInt32 colIndex, sortPtr sortInfo, PRInt32 indentLevel)
{
	PRInt32			childIndex = 0, numChildren = 0, nameSpaceID;
        nsCOMPtr<nsIContent>	child;
	nsresult		rv;
	nsVoidArray		*childArray;

	if (NS_FAILED(rv = container->ChildCount(numChildren)))	return(rv);

        if ((childArray = new nsVoidArray()) == nsnull)	return (NS_ERROR_OUT_OF_MEMORY);

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
				PRBool		found = PR_FALSE;
			        nsIContent	*cell;
				if ((indentLevel == 0) && (NS_SUCCEEDED(rv = GetTreeCell(child, colIndex, &cell))))
				{
					if (cell)
					{
						nsString	val;
						if (NS_SUCCEEDED(rv = GetTreeCellValue(cell, val)))
						{
							// hack:  always append cell value 1st as
							//        that's what sort callback expects

							if (sortInfo->naturalOrderSort == PR_TRUE)
							{
								// ???????????????????

								// XXX: note memory leak!
								childArray->AppendElement(val.ToNewCString());
								childArray->AppendElement(child);
							}
							else
							{
								// XXX: note memory leak!
								childArray->AppendElement(val.ToNewCString());
								childArray->AppendElement(child);
							}
							found = PR_TRUE;
						}
					}
				}
				if (found == PR_FALSE)
				{
					// hack:  always append cell value 1st as
					//        that's what sort callback expects

					PRBool	foundValue = PR_FALSE;
					if (sortInfo->naturalOrderSort == PR_TRUE)
					{
						nsString	pos;
						if (NS_OK == child->GetAttribute(kNameSpaceID_None, kNaturalOrderPosAtom, pos))
						{
							// XXX: note memory leak!
							childArray->AppendElement(pos.ToNewCString());
							childArray->AppendElement(child);
							foundValue = PR_TRUE;
						}
					}
					if (foundValue == PR_FALSE)
					{
						nsString	name;
						if (NS_OK == child->GetAttribute(kNameSpaceID_None, kNameAtom, name))
						{
							// XXX: note memory leak!
							childArray->AppendElement(name.ToNewCString());
							childArray->AppendElement(child);
							foundValue = PR_TRUE;
						}
					}
					found = PR_TRUE;
				}
			}
		}
	}
	unsigned long numElements = childArray->Count();
	if (numElements > 1)
	{
		nsIContent ** flatArray = new nsIContent*[numElements];
		if (flatArray)
		{
			// flatten array of resources, sort them, then add as tree elements
			unsigned long loop;
		        for (loop=0; loop<numElements; loop++)
				flatArray[loop] = (nsIContent *)childArray->ElementAt(loop);
			rdf_qsort((void *)flatArray, numElements/2, 2 * sizeof(void *),
				inplaceSortCallback, (void *)sortInfo);

			RemoveAllChildren(container);
			if (NS_FAILED(rv = container->UnsetAttribute(kNameSpaceID_None,
			                        kTreeContentsGeneratedAtom,
			                        PR_FALSE)))
			{
				printf("unable to clear contents-generated attribute\n");
			}
			
			// insert sorted children			
			numChildren = 0;
			for (loop=0; loop<numElements; loop+=2)
			{
				container->InsertChildAt((nsIContent *)flatArray[loop+1], numChildren++, PR_TRUE);
			}

			// recurse on grandchildren

			for (loop=0; loop<numElements; loop+=2)
			{
				container =  (nsIContent *)flatArray[loop+1];

				PRBool	canHaveKids = PR_FALSE;
				if (NS_FAILED(rv = container->CanContainChildren(canHaveKids)))	continue;
				if (canHaveKids == PR_FALSE)	continue;

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
							SortTreeChildren(child, colIndex, sortInfo, indentLevel+1);
						}
					}
				}
			}
			delete [] flatArray;
			flatArray = nsnull;
		}
	}
	return(NS_OK);
}



NS_IMETHODIMP
XULSortServiceImpl::OpenContainer(nsIRDFCompositeDataSource *db, nsIContent *container,
			nsIRDFResource **flatArray, PRInt32 numElements, PRInt32 elementSize)
{
	nsresult	rv;
	nsIContent	*treeNode;
	nsString	sortResource, sortDirection;
	_sortStruct	sortInfo;

	// get sorting info (property to sort on, direction to sort, etc)

	if (NS_FAILED(rv = FindTreeElement(container, &treeNode)))	return(rv);

	sortInfo.db = db;
	if (NS_FAILED(rv = GetSortColumnInfo(treeNode, sortResource, sortDirection)))	return(rv);
	char *uri = sortResource.ToNewCString();
	if (NS_FAILED(rv = gRDFService->GetResource(uri, &sortInfo.sortProperty)))	return(rv);
	delete [] uri;
	if (sortDirection.Equals("natural"))
	{
		sortInfo.naturalOrderSort = PR_TRUE;
		sortInfo.descendingSort = PR_FALSE;
		// no need to sort for natural order
	}
	else
	{
		sortInfo.naturalOrderSort = PR_FALSE;
		if (sortDirection.Equals("descending"))
			sortInfo.descendingSort = PR_TRUE;
		else
			sortInfo.descendingSort = PR_FALSE;
		rdf_qsort((void *)flatArray, numElements, elementSize, openSortCallback, (void *)&sortInfo);
	}

	return(NS_OK);
}



nsresult
XULSortServiceImpl::PrintTreeChildren(nsIContent *container, PRInt32 colIndex, PRInt32 indentLevel)
{
	PRInt32			childIndex = 0, numChildren = 0, nameSpaceID;
        nsCOMPtr<nsIContent>	child;
	nsresult		rv;

	if (NS_FAILED(rv = container->ChildCount(numChildren)))	return(rv);
	for (childIndex=0; childIndex<numChildren; childIndex++)
	{
		if (NS_FAILED(rv = container->ChildAt(childIndex, *getter_AddRefs(child))))	return(rv);
		if (NS_FAILED(rv = child->GetNameSpaceID(nameSpaceID)))	return(rv);
		if (nameSpaceID == kNameSpaceID_XUL)
		{
			nsCOMPtr<nsIAtom> tag;

			if (NS_FAILED(rv = child->GetTag(*getter_AddRefs(tag))))
				return rv;
			nsString	tagName;
			tag->ToString(tagName);
			for (PRInt32 indentLoop=0; indentLoop<indentLevel; indentLoop++) printf("    ");
			printf("Child #%d: tagName='%s'\n", childIndex, tagName.ToNewCString());

			PRInt32		attribIndex, numAttribs;
			child->GetAttributeCount(numAttribs);
			for (attribIndex = 0; attribIndex < numAttribs; attribIndex++)
			{
				PRInt32			attribNameSpaceID;
				nsCOMPtr<nsIAtom> 	attribAtom;

				if (NS_SUCCEEDED(rv = child->GetAttributeNameAt(attribIndex, attribNameSpaceID,
					*getter_AddRefs(attribAtom))))
				{
					nsString	attribName, attribValue;
					attribAtom->ToString(attribName);
					rv = child->GetAttribute(attribNameSpaceID, attribAtom, attribValue);
					if (rv == NS_CONTENT_ATTR_HAS_VALUE)
					{
						for (PRInt32 indentLoop=0; indentLoop<indentLevel; indentLoop++) printf("    ");
						printf("Attrib #%d: name='%s' value='%s'\n", attribIndex,
							attribName.ToNewCString(),
							attribValue.ToNewCString());
					}
				}
			}
			if ((tag.get() == kTreeItemAtom) || (tag.get() == kTreeChildrenAtom) || (tag.get() == kTreeCellAtom))
			{
				PrintTreeChildren(child, colIndex, indentLevel+1);
			}
		}
		else
		{
			for (PRInt32 loop=0; loop<indentLevel; loop++) printf("    ");
			printf("(Non-XUL node)  ");
			nsITextContent	*text = nsnull;
			if (NS_SUCCEEDED(rv = child->QueryInterface(kITextContentIID, (void **)&text)))
			{
				for (PRInt32 indentLoop=0; indentLoop<indentLevel; indentLoop++) printf("    ");
				printf("(kITextContentIID)  ");

				const nsTextFragment	*textFrags;
				PRInt32		numTextFrags;
				if (NS_SUCCEEDED(rv = text->GetText(textFrags, numTextFrags)))
				{
					const char *val = textFrags->Get1b();
					PRInt32 len = textFrags->GetLength();
					if (val)	printf("value='%.*s'", len, val);
				}
			}
			printf("\n");
		}
	}
	return(NS_OK);
}



// crap



NS_IMETHODIMP
XULSortServiceImpl::DoSort(nsIDOMNode* node, const nsString& sortResource,
                           const nsString& sortDirection)
{
	PRInt32		colIndex, treeBodyIndex;
	nsIContent	*contentNode, *treeNode, *treeBody, *treeParent;
	nsresult	rv;
	_sortStruct	sortInfo;

	if (NS_FAILED(rv = node->QueryInterface(kIContentIID, (void**)&contentNode)))	return(rv);
	if (NS_FAILED(rv = FindTreeElement(contentNode, &treeNode)))	return(rv);

	nsString	currentSortDirection;

	sortInfo.db = nsnull;
	sortInfo.sortProperty = nsnull;
	if (NS_SUCCEEDED(rv = GetSortColumnInfo(treeNode, sortResource, currentSortDirection)))
	{
		char *uri = sortResource.ToNewCString();
		if (NS_FAILED(rv = gRDFService->GetResource(uri, &sortInfo.sortProperty)))	return(rv);
		delete [] uri;
	}
	if (sortDirection.Equals("natural"))
	{
		sortInfo.naturalOrderSort = PR_TRUE;
		sortInfo.descendingSort = PR_FALSE;
	}
	else
	{
		sortInfo.naturalOrderSort = PR_FALSE;;
		if (sortDirection.Equals("ascending"))		sortInfo.descendingSort = PR_FALSE;
		else if (sortDirection.Equals("descending"))	sortInfo.descendingSort = PR_TRUE;
	}

	if (NS_FAILED(rv = GetSortColumnIndex(treeNode, sortResource, sortDirection, &colIndex)))	return(rv);
	if (NS_FAILED(rv = FindTreeBodyElement(treeNode, &treeBody)))	return(rv);
	if (NS_FAILED(rv = treeBody->GetParent(treeParent)))	return(rv);
	if (NS_FAILED(rv = treeParent->IndexOf(treeBody, treeBodyIndex)))	return(rv);
	if (NS_FAILED(rv = treeParent->RemoveChildAt(treeBodyIndex, PR_TRUE)))	return(rv);
	if (NS_SUCCEEDED(rv = SortTreeChildren(treeBody, colIndex, &sortInfo, 0)))
	{
	}

	if (NS_SUCCEEDED(rv = treeBody->UnsetAttribute(kNameSpaceID_None,
		kTreeContentsGeneratedAtom,PR_FALSE)))
	{
	}
	if (NS_SUCCEEDED(rv = treeParent->UnsetAttribute(kNameSpaceID_None,
		kTreeContentsGeneratedAtom,PR_FALSE)))
	{
	}

	if (NS_FAILED(rv = treeParent->AppendChildTo(treeBody, PR_TRUE)))	return(rv);

#if 0
	if (NS_FAILED(rv = PrintTreeChildren(treeBody, colIndex, 0)))	return(rv);
#endif
	return(NS_OK);
}



nsresult
NS_NewXULSortService(nsIXULSortService** mgr)
{
    return XULSortServiceImpl::GetSortService(mgr);
}

