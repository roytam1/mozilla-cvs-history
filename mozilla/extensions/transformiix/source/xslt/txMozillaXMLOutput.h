/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is the TransforMiiX XSLT processor.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 2001
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Peter Van der Beken <peterv@netscape.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef TRANSFRMX_MOZILLA_XML_OUTPUT_H
#define TRANSFRMX_MOZILLA_XML_OUTPUT_H

#include "txXMLEventHandler.h"
#include "nsIContent.h"
#include "nsIDOMDocument.h"
#include "nsIDOMHTMLTextAreaElement.h"
#include "nsIScriptLoader.h"
#include "nsIScriptLoaderObserver.h"
#include "nsIStyleSheetLinkingElement.h"
#include "nsWeakPtr.h"
#include "txOutputFormat.h"
#include "nsCOMArray.h"
#include "nsICSSLoaderObserver.h"

class txMozillaXMLOutput : public txIOutputXMLEventHandler,
                           public nsIScriptLoaderObserver,
                           public nsICSSLoaderObserver
{
public:
    txMozillaXMLOutput(const nsAString& aRootName,
                       PRInt32 aRootNsID,
                       txOutputFormat* aFormat,
                       nsIDOMDocument* aSourceDocument,
                       nsIDOMDocument* aResultDocument,
                       nsITransformObserver* aObserver);
    txMozillaXMLOutput(txOutputFormat* aFormat,
                       nsIDOMDocumentFragment* aFragment);
    virtual ~txMozillaXMLOutput();

    NS_DECL_ISUPPORTS
    NS_DECL_NSISCRIPTLOADEROBSERVER
    
    // nsICSSLoaderObserver
    NS_IMETHOD StyleSheetLoaded(nsICSSStyleSheet* aSheet, PRBool aNotify);

    /**
     * Signals to receive the start of an attribute.
     *
     * @param aName the name of the attribute
     * @param aNsID the namespace ID of the attribute
     * @param aValue the value of the attribute
     */
    void attribute(const nsAString& aName,
                   const PRInt32 aNsID,
                   const nsAString& aValue);

    /**
     * Signals to receive characters.
     *
     * @param aData the characters to receive
     * @param aDOE disable output escaping for these characters
     */
    void characters(const nsAString& aData, PRBool aDOE);

    /**
     * Signals to receive data that should be treated as a comment.
     *
     * @param data the comment data to receive
     */
    void comment(const nsAString& aData);

    /**
     * Signals the end of a document. It is an error to call
     * this method more than once.
     */
    void endDocument();

    /**
     * Signals to receive the end of an element.
     *
     * @param aName the name of the element
     * @param aNsID the namespace ID of the element
     */
    void endElement(const nsAString& aName,
                    const PRInt32 aNsID);

    /**
     * Signals to receive a processing instruction.
     *
     * @param aTarget the target of the processing instruction
     * @param aData the data of the processing instruction
     */
    void processingInstruction(const nsAString& aTarget,
                               const nsAString& aData);

    /**
     * Signals the start of a document.
     */
    void startDocument();

    /**
     * Signals to receive the start of an element.
     *
     * @param aName the name of the element
     * @param aNsID the namespace ID of the element
     */
    void startElement(const nsAString& aName,
                      const PRInt32 aNsID);

    /**
     * Gets the Mozilla output document
     *
     * @param aDocument the Mozilla output document
     */
    void getOutputDocument(nsIDOMDocument** aDocument);

private:
    void closePrevious(PRInt8 aAction);
    void startHTMLElement(nsIDOMElement* aElement);
    void endHTMLElement(nsIDOMElement* aElement, PRBool aXHTML);
    void processHTTPEquiv(nsIAtom* aHeader, const nsAString& aValue);
    void wrapChildren(nsIDOMNode* aCurrentNode, nsIDOMElement* aWrapper);
    nsresult createResultDocument(const nsAString& aName, PRInt32 aNsID,
                                  nsIDOMDocument* aSourceDocument,
                                  nsIDOMDocument* aResultDocument);
    void SignalTransformEnd();

    nsCOMPtr<nsIDOMDocument> mDocument;
    nsCOMPtr<nsIDOMNode> mCurrentNode;
    nsCOMPtr<nsIDOMNode> mParentNode;
    nsCOMPtr<nsIContent> mRootContent;
    nsCOMPtr<nsITransformObserver> mObserver;

    nsCOMPtr<nsIDOMNode> mNonAddedParent;
    nsCOMPtr<nsIDOMNode> mNonAddedNode;

    PRUint32 mBadChildLevel;
    nsCString mRefreshString;

    nsCOMArray<nsIDOMHTMLScriptElement> mScriptElements;
    nsCOMArray<nsIStyleSheet> mStylesheets;

    nsAutoString mText;

    txOutputFormat mOutputFormat;

    PRPackedBool mDontAddCurrent;

    PRPackedBool mHaveTitleElement;
    PRPackedBool mHaveBaseElement;

    PRPackedBool mInTransform;
    PRPackedBool mCreatingNewDocument;
 
    enum txAction { eCloseElement = 1, eFlushText = 2 };
};

#endif
