/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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

  A catch-all header file for miscellaneous RDF stuff. Currently
  contains error codes and vocabulary macros.

 */

#ifndef rdf_h___
#define rdf_h___

#include "nsError.h"

/*
 * The following macros are to aid in vocabulary definition.
 * They creates const char*'s for "kURI[prefix]_[name]" and
 * "kTag[prefix]_[name]", with appropriate complete namespace
 * qualification on the URI, e.g.,
 *
 * #define RDF_NAMESPACE_URI "http://www.w3.org/TR/WD-rdf-syntax#"
 * DEFINE_RDF_ELEMENT(RDF_NAMESPACE_URI, RDF, ID);
 *
 * will define:
 *
 * kURIRDF_ID to be "http://www.w3.org/TR/WD-rdf-syntax#ID", and
 * kTagRDF_ID to be "ID"
 */

#define DEFINE_RDF_VOCAB(namespace, prefix, name) \
static const char* kURI##prefix##_##name = ##namespace #name ;\
static const char* kTag##prefix##_##name = #name

/**
 * Core RDF vocabularies that we use to infer semantic actions
 */
#define RDF_NAMESPACE_URI  "http://www.w3.org/TR/WD-rdf-syntax#"
#define WEB_NAMESPACE_URI  "http://home.netscape.com/WEB-rdf#"
#define NC_NAMESPACE_URI   "http://home.netscape.com/NC-rdf#"


/**
 * @name Standard RDF error codes
 */

/*@{*/

/* Returned from nsIRDFCursor::Advance() if the cursor has no more
   elements to enuemrate */
#define NS_ERROR_RDF_CURSOR_EMPTY       NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_RDF, 1)
#define NS_ERROR_RDF_NO_VALUE           NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_RDF, 2)




/* ProgID prefixes for RDF DLL registration. */
#define NS_RDF_PROGID                           "component:||netscape|rdf"
#define NS_RDF_DATASOURCE_PROGID                NS_RDF_PROGID "|datasource"
#define NS_RDF_DATASOURCE_PROGID_PREFIX         NS_RDF_DATASOURCE_PROGID "?name#"
#define NS_RDF_RESOURCE_FACTORY_PROGID          "component:||netscape|rdf|resource-factory"
#define NS_RDF_RESOURCE_FACTORY_PROGID_PREFIX   NS_RDF_RESOURCE_FACTORY_PROGID "?name#"


/*@}*/

#ifdef _IMPL_NS_RDF
#ifdef XP_PC
#define NS_RDF _declspec(dllexport)
#else  /* !XP_PC */
#define NS_RDF
#endif /* !XP_PC */
#else  /* !_IMPL_NS_RDF */
#ifdef XP_PC
#define NS_RDF _declspec(dllimport)
#else  /* !XP_PC */
#define NS_RDF
#endif /* !XP_PC */
#endif /* !_IMPL_NS_RDF */

#endif /* rdf_h___ */
