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

  This interface encapsulates information about an RDF/XML file,
  including the root resource, CSS style sheets, and named data
  sources.

  This file also includes an observer interface for nsIRDFXMLDocument
  objects.

 */

#ifndef nsIRDFXMLDocument_h__
#define nsIRDFXMLDocument_h__

#include "nsISupports.h"
class nsIOutputStream;
class nsIURI;

// {EB1A5D30-AB33-11d2-8EC6-00805F29F370}
#define NS_IRDFXMLDOCUMENTOBSERVER_IID \
{ 0xeb1a5d30, 0xab33, 0x11d2, { 0x8e, 0xc6, 0x0, 0x80, 0x5f, 0x29, 0xf3, 0x70 } }

class nsIRDFXMLDocumentObserver : public nsISupports
{
public:
    /**
     * Called when the RDF/XML document begins to load.
     */
    NS_IMETHOD OnBeginLoad(void) = 0;

    /**
     * Called when the RDF/XML document load is interrupted for some reason.
     */
    NS_IMETHOD OnInterrupt(void) = 0;

    /**
     * Called when an interrupted RDF/XML document load is resumed.
     */
    NS_IMETHOD OnResume(void) = 0;

    /**
     * Called whtn the RDF/XML document load is complete.
     */
    NS_IMETHOD OnEndLoad(void) = 0;

    /**
     * Called when the root resource of the RDF/XML document is found
     */
    NS_IMETHOD OnRootResourceFound(nsIRDFResource* aResource) = 0;

    /**
     * Called when a CSS style sheet is included (via XML processing
     * instruction) to the document.
     */
    NS_IMETHOD OnCSSStyleSheetAdded(nsIURI* aCSSStyleSheetURL) = 0;

    /**
     * Called when a named data source is included (via XML processing
     * instruction) to the document.
     */
    NS_IMETHOD OnNamedDataSourceAdded(const char* aNamedDataSourceURI) = 0;
};


// {EB1A5D31-AB33-11d2-8EC6-00805F29F370}
#define NS_IRDFXMLDOCUMENT_IID \
{ 0xeb1a5d31, 0xab33, 0x11d2, { 0x8e, 0xc6, 0x0, 0x80, 0x5f, 0x29, 0xf3, 0x70 } }

class nsIRDFXMLDocument : public nsISupports
{
public:
    /**
     * Notify the document that the load is beginning.
     */
    NS_IMETHOD BeginLoad(void) = 0;

    /**
     * Notify the document that the load is being interrupted.
     */
    NS_IMETHOD Interrupt(void) = 0;

    /**
     * Notify the document that an interrupted load is being resumed.
     */
    NS_IMETHOD Resume(void) = 0;

    /**
     * Notify the document that the load is ending.
     */
    NS_IMETHOD EndLoad(void) = 0;

    /**
     * Set the root resource for the document.
     */
    NS_IMETHOD SetRootResource(nsIRDFResource* aResource) = 0;

    /**
     * Retrieve the root resource for the document.
     */
    NS_IMETHOD GetRootResource(nsIRDFResource** aResource) = 0;

    /**
     * Add a CSS style sheet to the document.
     * @param aStyleSheetURL An nsIURI object that is the URL of the style
     * sheet to add to the document.
     */
    NS_IMETHOD AddCSSStyleSheetURL(nsIURI* aStyleSheetURL) = 0;

    /**
     * Get the set of style sheets that have been included in the
     * document.
     * @param aStyleSheetURLs (out) A pointer to an array of pointers to nsIURI objects.
     * @param aCount (out) The number of nsIURI objects returned.
     */
    NS_IMETHOD GetCSSStyleSheetURLs(nsIURI*** aStyleSheetURLs, PRInt32* aCount) = 0;

    /**
     * Add a named data source to the document.
     * @param aNamedDataSoruceURI A URI identifying the data source.
     */
    NS_IMETHOD AddNamedDataSourceURI(const char* aNamedDataSourceURI) = 0;

    /**
     * Get the set of named data sources that have been included in
     * the document
     * @param aNamedDataSourceURIs (out) A pointer to an array of C-style character
     * strings.
     * @param aCount (out) The number of named data sources in the array.
     */
    NS_IMETHOD GetNamedDataSourceURIs(const char* const** aNamedDataSourceURIs, PRInt32* aCount) = 0;

    /**
     * Add an observer to the document. The observer will be notified of
     * RDF/XML events via the nsIRDFXMLDocumentObserver interface. Note that
     * the observer is <em>not</em> reference counted.
     */
    NS_IMETHOD AddDocumentObserver(nsIRDFXMLDocumentObserver* aObserver) = 0;

    /**
     * Remove an observer from the document.
     */
    NS_IMETHOD RemoveDocumentObserver(nsIRDFXMLDocumentObserver* aObserver) = 0;
};


#endif // nsIRDFXMLDocument_h__

