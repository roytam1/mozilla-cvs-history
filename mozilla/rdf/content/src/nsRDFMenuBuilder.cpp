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

#include "nsRDFGenericBuilder.h"

////////////////////////////////////////////////////////////////////////

DEFINE_RDF_VOCAB(NC_NAMESPACE_URI, NC, child);
DEFINE_RDF_VOCAB(NC_NAMESPACE_URI, NC, Columns);
DEFINE_RDF_VOCAB(NC_NAMESPACE_URI, NC, Column);
DEFINE_RDF_VOCAB(NC_NAMESPACE_URI, NC, Folder);
DEFINE_RDF_VOCAB(NC_NAMESPACE_URI, NC, Title);

DEFINE_RDF_VOCAB(RDF_NAMESPACE_URI, RDF, child);

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

////////////////////////////////////////////////////////////////////////

class RDFMenuBuilderImpl : public RDFGenericBuilderImpl
{
public:
    RDFMenuBuilderImpl();
    virtual ~RDFMenuBuilderImpl();

    // Implementation methods
    nsresult
    AddWidgetItem(nsIContent* aMenuItemElement,
                  nsIRDFResource* aProperty,
                  nsIRDFResource* aValue, PRInt32 aNaturalOrderPos);

    nsresult 
    GetRootWidgetAtom(nsIAtom** aResult) {
        NS_ADDREF(kMenuAtom);
        *aResult = kMenuAtom;
        return NS_OK;
    }

    nsresult
    GetWidgetItemAtom(nsIAtom** aResult) {
        NS_ADDREF(kMenuItemAtom);
        *aResult = kMenuItemAtom;
        return NS_OK;
    }

    nsresult
    GetInsertionRootAtom(nsIAtom** aResult) {
        NS_ADDREF(kMenuAtom);
        *aResult = kMenuAtom;
        return NS_OK;
    }

    nsresult
    GetItemAtomThatContainsTheChildren(nsIAtom** aResult) {
        NS_ADDREF(kMenuAtom);
        *aResult = kMenuAtom;
        return NS_OK;
    }

    // pseudo-constants
    static nsrefcnt gRefCnt;
 
    static nsIAtom* kMenuAtom;
    static nsIAtom* kMenuItemAtom;
};

////////////////////////////////////////////////////////////////////////

nsrefcnt RDFMenuBuilderImpl::gRefCnt = 0;

nsIAtom* RDFMenuBuilderImpl::kMenuAtom;
nsIAtom* RDFMenuBuilderImpl::kMenuItemAtom;

////////////////////////////////////////////////////////////////////////

nsresult
NS_NewRDFMenuBuilder(nsIRDFContentModelBuilder** result)
{
    NS_PRECONDITION(result != nsnull, "null ptr");
    if (! result)
        return NS_ERROR_NULL_POINTER;

    RDFMenuBuilderImpl* builder = new RDFMenuBuilderImpl();
    if (! builder)
        return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(builder);
    *result = builder;
    return NS_OK;
}



RDFMenuBuilderImpl::RDFMenuBuilderImpl(void)
    : RDFGenericBuilderImpl()
{
    if (gRefCnt == 0) {
        kMenuAtom            = NS_NewAtom("menu");
        kMenuItemAtom        = NS_NewAtom("menuitem");
    }

    ++gRefCnt;
}

RDFMenuBuilderImpl::~RDFMenuBuilderImpl(void)
{
    --gRefCnt;
    if (gRefCnt == 0) {
        
        NS_RELEASE(kMenuAtom);
        NS_RELEASE(kMenuItemAtom);
    }
}

////////////////////////////////////////////////////////////////////////
// Implementation methods

nsresult
RDFMenuBuilderImpl::AddWidgetItem(nsIContent* aElement,
                                  nsIRDFResource* aProperty,
                                  nsIRDFResource* aValue)
{
    nsresult rv;

    nsCOMPtr<nsIContent> menuParent;
    menuParent = dont_QueryInterface(aElement);
    if (!IsWidgetItemElement(aElement) && !IsWidgetInsertionRootElement(aElement))
    {
        NS_ERROR("Can't add something here!");
        return NS_ERROR_UNEXPECTED;
    }

    // Create the <xul:menuitem> element
    nsCOMPtr<nsIContent> menuItem;
    if (NS_FAILED(rv = CreateResourceElement(kNameSpaceID_XUL,
                                             kMenuItemAtom,
                                             aValue,
                                             getter_AddRefs(menuItem))))
        return rv;

    // Add the <xul:menuitem> to the <xul:menu> element.
    menuParent->AppendChildTo(menuItem, PR_TRUE);

    // Add miscellaneous attributes by iterating _all_ of the
    // properties out of the resource.
    nsCOMPtr<nsIRDFArcsOutCursor> arcs;
    if (NS_FAILED(rv = mDB->ArcLabelsOut(aValue, getter_AddRefs(arcs)))) {
        NS_ERROR("unable to get arcs out");
        return rv;
    }

    while (NS_SUCCEEDED(rv = arcs->Advance())) {
        nsCOMPtr<nsIRDFResource> property;
        if (NS_FAILED(rv = arcs->GetPredicate(getter_AddRefs(property)))) {
            NS_ERROR("unable to get cursor value");
            return rv;
        }

        // Ignore ordinal properties
        if (rdf_IsOrdinalProperty(property))
            continue;

        PRInt32 nameSpaceID;
        nsCOMPtr<nsIAtom> tag;
        if (NS_FAILED(rv = mDocument->SplitProperty(property, &nameSpaceID, getter_AddRefs(tag)))) {
            NS_ERROR("unable to split property");
            return rv;
        }

        nsCOMPtr<nsIRDFNode> value;
        if (NS_FAILED(rv = mDB->GetTarget(aValue, property, PR_TRUE, getter_AddRefs(value)))) {
            NS_ERROR("unable to get target");
            return rv;
        }

        nsCOMPtr<nsIRDFResource> resource;
        nsCOMPtr<nsIRDFLiteral> literal;

        nsAutoString s;
        if (NS_SUCCEEDED(rv = value->QueryInterface(kIRDFResourceIID, getter_AddRefs(resource)))) {
            const char* uri;
            resource->GetValue(&uri);
            s = uri;
        }
        else if (NS_SUCCEEDED(rv = value->QueryInterface(kIRDFLiteralIID, getter_AddRefs(literal)))) {
            const PRUnichar* p;
            literal->GetValue(&p);
            s = p;
        }
        else {
            NS_ERROR("not a resource or a literal");
            return NS_ERROR_UNEXPECTED;
        }

        menuItem->SetAttribute(nameSpaceID, tag, s, PR_FALSE);

        nsString nameAtom;
        tag->ToString(nameAtom);
        if (nameAtom == "Name")
        {
            nsIAtom* lowerName = NS_NewAtom("name");
            // Hack to ensure that we add in a lowercase name attribute also.
            menuItem->SetAttribute(kNameSpaceID_None, lowerName, s, PR_FALSE);
            NS_RELEASE(lowerName);
        }
    }

    if (NS_FAILED(rv) && (rv != NS_ERROR_RDF_CURSOR_EMPTY)) {
        NS_ERROR("error advancing cursor");
        return rv;
    }

    // Finally, mark this as a "container" so that we know to
    // recursively generate kids if they're asked for.
    if (NS_FAILED(rv = menuItem->SetAttribute(kNameSpaceID_RDF, kContainerAtom, "true", PR_FALSE)))
        return rv;

    return NS_OK;
}
