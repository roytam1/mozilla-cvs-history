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

  An nsIRDFDocument implementation that builds an HTML-like model,
  complete with text nodes. The model can be displayed in a vanilla
  HTML content viewer by applying CSS2 styles to the text.

 */

#include "nsIDocument.h"
#include "nsIRDFContent.h"
#include "nsIRDFContentModelBuilder.h"
#include "nsIRDFCursor.h"
#include "nsIRDFDataBase.h"
#include "nsIRDFDocument.h"
#include "nsIRDFNode.h"
#include "nsIRDFService.h"
#include "nsIServiceManager.h"
#include "nsINameSpaceManager.h"
#include "nsISupportsArray.h"
#include "nsRDFContentUtils.h"
#include "rdfutil.h"

////////////////////////////////////////////////////////////////////////

static NS_DEFINE_IID(kIContentIID,                NS_ICONTENT_IID);
static NS_DEFINE_IID(kIDocumentIID,               NS_IDOCUMENT_IID);
static NS_DEFINE_IID(kIRDFResourceIID,            NS_IRDFRESOURCE_IID);
static NS_DEFINE_IID(kIRDFLiteralIID,             NS_IRDFLITERAL_IID);
static NS_DEFINE_IID(kIRDFContentModelBuilderIID, NS_IRDFCONTENTMODELBUILDER_IID);

////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////

class RDFHTMLBuilderImpl : public nsIRDFContentModelBuilder
{
private:
    nsIRDFDocument* mDocument;
    nsIRDFDataBase* mDB;

public:
    RDFHTMLBuilderImpl();
    virtual ~RDFHTMLBuilderImpl();

    // nsISupports interface
    NS_DECL_ISUPPORTS

    // nsIRDFContentModelBuilder interface
    NS_IMETHOD SetDocument(nsIRDFDocument* aDocument);
    NS_IMETHOD CreateRoot(nsIRDFResource* aResource);
    NS_IMETHOD OnAssert(nsIRDFContent* aElement, nsIRDFResource* aProperty, nsIRDFNode* aValue);
    NS_IMETHOD OnUnassert(nsIRDFContent* aElement, nsIRDFResource* aProperty, nsIRDFNode* aValue);

    // Implementation methods
    nsresult AddTreeChild(nsIRDFContent* aParent,
                          nsIRDFResource* property,
                          nsIRDFResource* value);

    nsresult AddLeafChild(nsIRDFContent* parent,
                          nsIRDFResource* property,
                          nsIRDFLiteral* value);
};

////////////////////////////////////////////////////////////////////////

static nsIAtom* kIdAtom;

RDFHTMLBuilderImpl::RDFHTMLBuilderImpl(void)
    : mDocument(nsnull),
      mDB(nsnull)
{
    if (nsnull == kIdAtom) {
        kIdAtom = NS_NewAtom("ID");
    }
    else {
        NS_ADDREF(kIdAtom);
    }
}

RDFHTMLBuilderImpl::~RDFHTMLBuilderImpl(void)
{
    nsrefcnt refcnt;
    NS_RELEASE2(kIdAtom, refcnt);

    NS_IF_RELEASE(mDB);
    // mDocument is _not_ refcounted
}

////////////////////////////////////////////////////////////////////////

NS_IMPL_ISUPPORTS(RDFHTMLBuilderImpl, kIRDFContentModelBuilderIID);

////////////////////////////////////////////////////////////////////////

nsresult
RDFHTMLBuilderImpl::AddTreeChild(nsIRDFContent* parent,
                                 nsIRDFResource* property,
                                 nsIRDFResource* value)
{
    // If it's a tree property, then create a child element whose
    // value is the value of the property. We'll also attach an "ID="
    // attribute to the new child; e.g.,
    //
    // <parent>
    //   <property id="value">
    //      <!-- recursively generated -->
    //   </property>
    //   ...
    // </parent>

    nsresult rv;
    PRInt32 nameSpaceID;
    nsIAtom* tag = nsnull;
    nsIRDFContent* child = nsnull;
    const char* p;

    if (NS_FAILED(rv = mDocument->SplitProperty(property, &nameSpaceID, &tag)))
        goto done;

    if (NS_FAILED(rv = NS_NewRDFResourceElement(&child, value, nameSpaceID, tag, PR_TRUE)))
        goto done;

    if (NS_FAILED(rv = value->GetValue(&p)))
        goto done;

    if (NS_FAILED(rv = child->SetAttribute(kNameSpaceID_HTML, kIdAtom, p, PR_FALSE)))
        goto done;

    rv = parent->AppendChildTo(child, PR_TRUE);

done:
    NS_IF_RELEASE(child);
    NS_IF_RELEASE(tag);
    return rv;
}


nsresult
RDFHTMLBuilderImpl::AddLeafChild(nsIRDFContent* parent,
                                 nsIRDFResource* property,
                                 nsIRDFLiteral* value)
{
    // Otherwise, it's not a tree property. So we'll just create a
    // new element for the property, and a simple text node for
    // its value; e.g.,
    //
    // <parent>
    //   <property>value</property>
    //   ...
    // </parent>

    nsresult rv;
    PRInt32 nameSpaceID;
    nsIAtom* tag = nsnull;
    nsIRDFContent* child = nsnull;

    if (NS_FAILED(rv = mDocument->SplitProperty(property, &nameSpaceID, &tag)))
        goto done;

    if (NS_FAILED(rv = NS_NewRDFResourceElement(&child, property, nameSpaceID, tag, PR_FALSE)))
        goto done;

    if (NS_FAILED(rv = parent->AppendChildTo(child, PR_TRUE)))
        goto done;

    rv = rdf_AttachTextNode(child, value);

done:
    NS_IF_RELEASE(tag);
    NS_IF_RELEASE(child);
    return rv;
}


NS_IMETHODIMP
RDFHTMLBuilderImpl::SetDocument(nsIRDFDocument* aDocument)
{
    NS_PRECONDITION(aDocument != nsnull, "null ptr");
    if (! aDocument)
        return NS_ERROR_NULL_POINTER;

    mDocument = aDocument; // not refcounted

    nsresult rv;
    if (NS_FAILED(rv = mDocument->GetDataBase(mDB)))
        return rv;

    return NS_OK;
}


NS_IMETHODIMP
RDFHTMLBuilderImpl::CreateRoot(nsIRDFResource* aResource)
{
    NS_PRECONDITION(mDocument != nsnull, "not initialized");
    if (! mDocument)
        return NS_ERROR_NOT_INITIALIZED;

    nsresult rv;
    nsIAtom* tag        = nsnull;
    nsIDocument* doc    = nsnull;
    nsIContent* root    = nsnull;
    nsIRDFContent* body = nsnull;

    if (NS_FAILED(rv = mDocument->QueryInterface(kIDocumentIID, (void**) &doc)))
        goto done;

    rv = NS_ERROR_OUT_OF_MEMORY;
    if ((tag = NS_NewAtom("DOCUMENT")) == nsnull)
        goto done;

    if (NS_FAILED(rv = NS_NewRDFGenericElement(&root, kNameSpaceID_None, tag)))
        goto done;

    doc->SetRootContent(NS_STATIC_CAST(nsIContent*, root));

    NS_RELEASE(tag);

    rv = NS_ERROR_OUT_OF_MEMORY;
    if ((tag = NS_NewAtom("BODY")) == nsnull)
        goto done;

    // PR_TRUE indicates that children should be recursively generated on demand
    if (NS_FAILED(rv = NS_NewRDFResourceElement(&body, aResource, kNameSpaceID_None, tag, PR_TRUE)))
        goto done;

    if (NS_FAILED(rv = root->AppendChildTo(body, PR_FALSE)))
        goto done;

done:
    NS_IF_RELEASE(body); 
    NS_IF_RELEASE(root);
    NS_IF_RELEASE(tag);
    NS_IF_RELEASE(doc);
    return NS_OK;
}



NS_IMETHODIMP
RDFHTMLBuilderImpl::OnAssert(nsIRDFContent* parent,
                             nsIRDFResource* property,
                             nsIRDFNode* value)
{
    NS_PRECONDITION(mDocument != nsnull, "not initialized");
    if (! mDocument)
        return NS_ERROR_NOT_INITIALIZED;

    nsresult rv;

    nsIRDFResource* valueResource;
    if (NS_SUCCEEDED(rv = value->QueryInterface(kIRDFResourceIID, (void**) &valueResource))) {
        // If it's a tree property or an RDF container, then add it as
        // a tree child and return.
        PRBool isTreeProperty;
        if ((NS_SUCCEEDED(rv = mDocument->IsTreeProperty(property, &isTreeProperty)) && isTreeProperty) ||
            (rdf_IsContainer(mDB, valueResource))) {
            rv = AddTreeChild(parent, property, valueResource);
            NS_RELEASE(valueResource);
            return rv;
        }

        // Otherwise, fall through and add try to add it as a property
        NS_RELEASE(valueResource);
    }

    nsIRDFLiteral* valueLiteral;
    if (NS_SUCCEEDED(rv = value->QueryInterface(kIRDFLiteralIID, (void**) &valueLiteral))) {
        rv = AddLeafChild(parent, property, valueLiteral);
        NS_RELEASE(valueLiteral);
    }
    return rv;
}


NS_IMETHODIMP
RDFHTMLBuilderImpl::OnUnassert(nsIRDFContent* parent,
                               nsIRDFResource* property,
                               nsIRDFNode* value)
{
    PR_ASSERT(0);
    return NS_ERROR_NOT_IMPLEMENTED;
}

////////////////////////////////////////////////////////////////////////

nsresult
NS_NewRDFHTMLBuilder(nsIRDFContentModelBuilder** result)
{
    NS_PRECONDITION(result != nsnull, "null ptr");
    if (! result)
        return NS_ERROR_NULL_POINTER;

    RDFHTMLBuilderImpl* builder = new RDFHTMLBuilderImpl();
    if (! builder)
        return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(builder);
    *result = builder;
    return NS_OK;
}
