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

#include "nsIRDFContainer.h"
#include "nsRDFParserUtils.h"
#include "nsRDFResource.h"
#include "nsString.h"
#include "rdfutil.h"

void XXXNeverCalled()
{
    nsAutoString s;
    nsCAutoString cs;
    int v;

    // nsRDFParserUtils
    nsRDFParserUtils::EntityToUnicode("");
    nsRDFParserUtils::StripAndConvert(s);
    nsRDFParserUtils::GetQuotedAttributeValue(s, s, s);
    nsRDFParserUtils::IsJavaScriptLanguage(s, &v);

    // rdfutils
    rdf_MakeRelativeRef(s, s);
    rdf_MakeRelativeName(s, s);
    rdf_MakeAbsoluteURI(s, s);
    NS_NewContainerEnumerator(nsnull, nsnull, nsnull);
    NS_NewEmptyEnumerator(nsnull);

    // nsRDFResource
    nsRDFResource r();
}




