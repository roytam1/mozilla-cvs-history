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

  A package of routines shared by the RDF content code.

 */


#include "nsIContent.h"
#include "nsIRDFNode.h"
#include "nsITextContent.h"
#include "nsLayoutCID.h"
#include "nsRDFContentUtils.h"
#include "nsString.h"

static NS_DEFINE_IID(kIContentIID,     NS_ICONTENT_IID);
static NS_DEFINE_IID(kIRDFResourceIID, NS_IRDFRESOURCE_IID);
static NS_DEFINE_IID(kIRDFLiteralIID,  NS_IRDFLITERAL_IID);
static NS_DEFINE_IID(kITextContentIID, NS_ITEXT_CONTENT_IID); // XXX grr...
static NS_DEFINE_CID(kTextNodeCID,     NS_TEXTNODE_CID);




nsresult
nsRDFContentUtils::AttachTextNode(nsIContent* parent, nsIRDFNode* value)
{
    nsresult rv;
    nsAutoString s;
    nsIContent* node         = nsnull;
    nsITextContent* text     = nsnull;
    nsIRDFResource* resource = nsnull;
    nsIRDFLiteral* literal   = nsnull;
    
    if (NS_SUCCEEDED(rv = value->QueryInterface(kIRDFResourceIID, (void**) &resource))) {
        const char* p;
        if (NS_FAILED(rv = resource->GetValue(&p)))
            goto error;

        s = p;
    }
    else if (NS_SUCCEEDED(rv = value->QueryInterface(kIRDFLiteralIID, (void**) &literal))) {
        const PRUnichar* p;
        if (NS_FAILED(rv = literal->GetValue(&p)))
            goto error;

        s = p;
    }
    else {
        PR_ASSERT(0);
        goto error;
    }

    if (NS_FAILED(rv = nsRepository::CreateInstance(kTextNodeCID,
                                                    nsnull,
                                                    kIContentIID,
                                                    (void**) &node)))
        goto error;

    if (NS_FAILED(rv = node->QueryInterface(kITextContentIID, (void**) &text)))
        goto error;

    if (NS_FAILED(rv = text->SetText(s.GetUnicode(), s.Length(), PR_FALSE)))
        goto error;

    // hook it up to the child
    if (NS_FAILED(rv = parent->AppendChildTo(NS_STATIC_CAST(nsIContent*, node), PR_TRUE)))
        goto error;

error:
    NS_IF_RELEASE(node);
    NS_IF_RELEASE(text);
    return rv;
}

