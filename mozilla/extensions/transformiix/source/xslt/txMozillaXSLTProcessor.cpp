/* -*- Mode: IDL; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 2002
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Peter Van der Beken <peterv@netscape.com> (original author)
 *
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

#include "txMozillaXSLTProcessor.h"
#include "nsContentCID.h"
#include "nsDOMError.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsIDOMClassInfo.h"
#include "nsIScriptLoader.h"
#include "txExecutionState.h"
#include "txMozillaTextOutput.h"
#include "txMozillaXMLOutput.h"
#include "txSingleNodeContext.h"
#include "txURIUtils.h"
#include "XMLUtils.h"
#include "txUnknownHandler.h"
#include "txXSLTProcessor.h"
#include "nsIHTMLDocument.h"

/**
 * Output Handler Factories
 */
class txToDocHandlerFactory : public txAOutputHandlerFactory
{
public:
    txToDocHandlerFactory(txExecutionState* aEs,
                          nsIDOMDocument* aSourceDocument,
                          nsIDOMDocument* aResultDocument,
                          nsITransformObserver* aObserver)
        : mEs(aEs), mSourceDocument(aSourceDocument),
          mResultDocument(aResultDocument), mObserver(aObserver)
    {
    }

    virtual ~txToDocHandlerFactory()
    {
    }

    TX_DECL_TXAOUTPUTHANDLERFACTORY

private:
    txExecutionState* mEs;
    nsCOMPtr<nsIDOMDocument> mSourceDocument;
    nsCOMPtr<nsIDOMDocument> mResultDocument;
    nsCOMPtr<nsITransformObserver> mObserver;
};

class txToFragmentHandlerFactory : public txAOutputHandlerFactory
{
public:
    txToFragmentHandlerFactory(nsIDOMDocumentFragment* aFragment)
        : mFragment(aFragment)
    {
    }

    virtual ~txToFragmentHandlerFactory()
    {
    }

    TX_DECL_TXAOUTPUTHANDLERFACTORY

private:
    nsCOMPtr<nsIDOMDocumentFragment> mFragment;
};

nsresult
txToDocHandlerFactory::createHandlerWith(txOutputFormat* aFormat,
                                         txAXMLEventHandler** aHandler)
{
    *aHandler = nsnull;
    switch (aFormat->mMethod) {
        case eMethodNotSet:
        case eXMLOutput:
        {
            *aHandler = new txUnknownHandler(mEs);
            break;
        }

        case eHTMLOutput:
        {
            *aHandler = new txMozillaXMLOutput(nsString(),
                                               kNameSpaceID_None,
                                               aFormat, mSourceDocument,
                                               mResultDocument, mObserver);
            break;
        }

        case eTextOutput:
        {
            *aHandler = new txMozillaTextOutput(mSourceDocument,
                                                mResultDocument,
                                                mObserver);
            break;
        }
    }
    NS_ENSURE_TRUE(*aHandler, NS_ERROR_OUT_OF_MEMORY);
    return NS_OK;
}

nsresult
txToDocHandlerFactory::createHandlerWith(txOutputFormat* aFormat,
                                         const nsAString& aName,
                                         PRInt32 aNsID,
                                         txAXMLEventHandler** aHandler)
{
    *aHandler = nsnull;
    switch (aFormat->mMethod) {
        case eMethodNotSet:
        {
            NS_ERROR("How can method not be known when root element is?");
            return NS_ERROR_UNEXPECTED;
        }

        case eXMLOutput:
        case eHTMLOutput:
        {
            *aHandler = new txMozillaXMLOutput(aName, aNsID, aFormat,
                                               mSourceDocument,
                                               mResultDocument,
                                               mObserver);
            break;
        }

        case eTextOutput:
        {
            *aHandler = new txMozillaTextOutput(mSourceDocument,
                                                mResultDocument,
                                                mObserver);
            break;
        }
    }
    NS_ENSURE_TRUE(*aHandler, NS_ERROR_OUT_OF_MEMORY);
    return NS_OK;
}

nsresult
txToFragmentHandlerFactory::createHandlerWith(txOutputFormat* aFormat,
                                              txAXMLEventHandler** aHandler)
{
    *aHandler = nsnull;
    switch (aFormat->mMethod) {
        case eMethodNotSet:
        {
            txOutputFormat format;
            format.merge(*aFormat);
            nsCOMPtr<nsIDOMDocument> doc;
            mFragment->GetOwnerDocument(getter_AddRefs(doc));
            NS_ASSERTION(doc, "unable to get ownerdocument");
            // Need a way for testing xhtml vs. html. But this is the best
            // we can do for now.
            nsCOMPtr<nsIHTMLDocument> htmldoc = do_QueryInterface(doc);
            format.mMethod = htmldoc ? eHTMLOutput : eXMLOutput;
            *aHandler = new txMozillaXMLOutput(&format, mFragment);
            break;
        }

        case eXMLOutput:
        case eHTMLOutput:
        {
            *aHandler = new txMozillaXMLOutput(aFormat, mFragment);
            break;
        }

        case eTextOutput:
        {
            *aHandler = new txMozillaTextOutput(mFragment);
            break;
        }
    }
    NS_ENSURE_TRUE(*aHandler, NS_ERROR_OUT_OF_MEMORY);
    return NS_OK;
}

nsresult
txToFragmentHandlerFactory::createHandlerWith(txOutputFormat* aFormat,
                                              const nsAString& aName,
                                              PRInt32 aNsID,
                                              txAXMLEventHandler** aHandler)
{
    *aHandler = nsnull;
    NS_ASSERTION(aFormat->mMethod != eMethodNotSet,
                 "How can method not be known when root element is?");
    NS_ENSURE_TRUE(aFormat->mMethod != eMethodNotSet, NS_ERROR_UNEXPECTED);
    return createHandlerWith(aFormat, aHandler);
}

/**
 * txMozillaXSLTProcessor
 */

NS_IMPL_ADDREF(txMozillaXSLTProcessor)
NS_IMPL_RELEASE(txMozillaXSLTProcessor)
NS_INTERFACE_MAP_BEGIN(txMozillaXSLTProcessor)
    NS_INTERFACE_MAP_ENTRY(nsIXSLTProcessor)
    NS_INTERFACE_MAP_ENTRY(nsIXSLTProcessorObsolete)
    NS_INTERFACE_MAP_ENTRY(nsIDocumentTransformer)
    NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIXSLTProcessor)
    NS_INTERFACE_MAP_ENTRY_EXTERNAL_DOM_CLASSINFO(XSLTProcessor)
NS_INTERFACE_MAP_END

txMozillaXSLTProcessor::txMozillaXSLTProcessor() : mVariables(PR_TRUE)
{
}

txMozillaXSLTProcessor::~txMozillaXSLTProcessor()
{
}

NS_IMETHODIMP
txMozillaXSLTProcessor::TransformDocument(nsIDOMNode* aSourceDOM,
                                          nsIDOMNode* aStyleDOM,
                                          nsIDOMDocument* aOutputDoc,
                                          nsISupports* aObserver)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
txMozillaXSLTProcessor::SetTransformObserver(nsITransformObserver* aObserver)
{
    mObserver = aObserver;
    return NS_OK;
}

nsresult
txMozillaXSLTProcessor::SetSourceContentModel(nsIDOMNode* aSourceDOM)
{
    mSource = aSourceDOM;
    if (mStylesheet) {
        return DoTransform();
    }
    return NS_OK;
}

nsresult
txMozillaXSLTProcessor::DoTransform()
{
    NS_ENSURE_TRUE(mSource, NS_ERROR_UNEXPECTED);
    NS_ENSURE_TRUE(mStylesheet, NS_ERROR_UNEXPECTED);
    NS_ASSERTION(mObserver, "no observer");

    // Create wrapper for the source document.
    nsCOMPtr<nsIDOMDocument> sourceDOMDocument;
    mSource->GetOwnerDocument(getter_AddRefs(sourceDOMDocument));
    if (!sourceDOMDocument) {
        sourceDOMDocument = do_QueryInterface(mSource);
    }
    NS_ENSURE_TRUE(sourceDOMDocument, NS_ERROR_FAILURE);
    Document sourceDocument(sourceDOMDocument);
    Node* sourceNode = sourceDocument.createWrapper(mSource);
    NS_ENSURE_TRUE(sourceNode, NS_ERROR_FAILURE);

    txExecutionState es(mStylesheet);

    // XXX Need to add error observers

    txToDocHandlerFactory handlerFactory(&es, sourceDOMDocument, nsnull,
                                         mObserver);
    es.mOutputHandlerFactory = &handlerFactory;

    es.init(sourceNode, &mVariables);

    // Process root of XML source document
    txXSLTProcessor::execute(es);
    es.end();


    return NS_OK;
}

NS_IMETHODIMP
txMozillaXSLTProcessor::ImportStylesheet(nsIDOMNode *aStyle)
{
    if (!URIUtils::CanCallerAccess(aStyle)) {
        return NS_ERROR_DOM_SECURITY_ERR;
    }
    
    PRUint16 type = 0;
    aStyle->GetNodeType(&type);
    NS_ENSURE_TRUE(type == nsIDOMNode::ELEMENT_NODE ||
                   type == nsIDOMNode::DOCUMENT_NODE,
                   NS_ERROR_INVALID_ARG);

    nsresult rv = TX_CompileStylesheet(aStyle, getter_AddRefs(mStylesheet));
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
}

NS_IMETHODIMP
txMozillaXSLTProcessor::TransformToDocument(nsIDOMNode *aSource,
                                            nsIDOMDocument **aResult)
{
    NS_ENSURE_ARG(aSource);
    NS_ENSURE_ARG_POINTER(aResult);
    NS_ENSURE_TRUE(mStylesheet, NS_ERROR_NOT_INITIALIZED);

    if (!URIUtils::CanCallerAccess(aSource)) {
        return NS_ERROR_DOM_SECURITY_ERR;
    }

    // Create wrapper for the source document.
    nsCOMPtr<nsIDOMDocument> sourceDOMDocument;
    aSource->GetOwnerDocument(getter_AddRefs(sourceDOMDocument));
    if (!sourceDOMDocument) {
        sourceDOMDocument = do_QueryInterface(aSource);
    }
    NS_ENSURE_TRUE(sourceDOMDocument, NS_ERROR_FAILURE);
    Document sourceDocument(sourceDOMDocument);
    Node* sourceNode = sourceDocument.createWrapper(aSource);
    NS_ENSURE_TRUE(sourceNode, NS_ERROR_FAILURE);

    txExecutionState es(mStylesheet);

    // XXX Need to add error observers

    txToDocHandlerFactory handlerFactory(&es, sourceDOMDocument, nsnull,
                                         nsnull);
    es.mOutputHandlerFactory = &handlerFactory;

    es.init(sourceNode, &mVariables);

    // Process root of XML source document
    txXSLTProcessor::execute(es);
    es.end();

    txAOutputXMLEventHandler* handler =
        NS_STATIC_CAST(txAOutputXMLEventHandler*, es.mOutputHandler);
    handler->getOutputDocument(aResult);

    return NS_OK;
}

NS_IMETHODIMP
txMozillaXSLTProcessor::TransformToFragment(nsIDOMNode *aSource,
                                            nsIDOMDocument *aOutput,
                                            nsIDOMDocumentFragment **aResult)
{
    NS_ENSURE_ARG(aSource);
    NS_ENSURE_ARG(aOutput);
    NS_ENSURE_ARG_POINTER(aResult);
    NS_ENSURE_TRUE(mStylesheet, NS_ERROR_NOT_INITIALIZED);

    if (!URIUtils::CanCallerAccess(aSource) ||
        !URIUtils::CanCallerAccess(aOutput)) {
        return NS_ERROR_DOM_SECURITY_ERR;
    }

    // Create wrapper for the source document.
    nsCOMPtr<nsIDOMDocument> sourceDOMDocument;
    aSource->GetOwnerDocument(getter_AddRefs(sourceDOMDocument));
    if (!sourceDOMDocument) {
        sourceDOMDocument = do_QueryInterface(aSource);
    }
    NS_ENSURE_TRUE(sourceDOMDocument, NS_ERROR_FAILURE);
    Document sourceDocument(sourceDOMDocument);
    Node* sourceNode = sourceDocument.createWrapper(aSource);
    NS_ENSURE_TRUE(sourceNode, NS_ERROR_FAILURE);

    txExecutionState es(mStylesheet);

    // XXX Need to add error observers

    nsresult rv = aOutput->CreateDocumentFragment(aResult);
    NS_ENSURE_SUCCESS(rv, rv);
    txToFragmentHandlerFactory handlerFactory(*aResult);
    es.mOutputHandlerFactory = &handlerFactory;

    es.init(sourceNode, &mVariables);

    // Process root of XML source document
    txXSLTProcessor::execute(es);
    es.end();

    return NS_OK;
}

NS_IMETHODIMP
txMozillaXSLTProcessor::SetParameter(const nsAString & aNamespaceURI,
                                     const nsAString & aLocalName,
                                     nsIVariant *aValue)
{
    NS_ENSURE_ARG(aValue);
    PRUint16 dataType;
    aValue->GetDataType(&dataType);
    switch (dataType) {
        // Number
        case nsIDataType::VTYPE_INT8:
        case nsIDataType::VTYPE_INT16:
        case nsIDataType::VTYPE_INT32:
        case nsIDataType::VTYPE_INT64:
        case nsIDataType::VTYPE_UINT8:
        case nsIDataType::VTYPE_UINT16:
        case nsIDataType::VTYPE_UINT32:
        case nsIDataType::VTYPE_UINT64:
        case nsIDataType::VTYPE_FLOAT:
        case nsIDataType::VTYPE_DOUBLE:

        // Boolean
        case nsIDataType::VTYPE_BOOL:

        // String
        case nsIDataType::VTYPE_CHAR:
        case nsIDataType::VTYPE_WCHAR:
        case nsIDataType::VTYPE_DOMSTRING:
        case nsIDataType::VTYPE_CHAR_STR:
        case nsIDataType::VTYPE_WCHAR_STR:
        case nsIDataType::VTYPE_STRING_SIZE_IS:
        case nsIDataType::VTYPE_WSTRING_SIZE_IS:
        case nsIDataType::VTYPE_UTF8STRING:
        case nsIDataType::VTYPE_CSTRING:
        case nsIDataType::VTYPE_ASTRING:

        // Nodeset
        case nsIDataType::VTYPE_INTERFACE:
        case nsIDataType::VTYPE_INTERFACE_IS:
        case nsIDataType::VTYPE_ARRAY:
        {
            // This might still be an error, but we'll only
            // find out later since we lazily evaluate.
            break;
        }

        default:
        {
            return NS_ERROR_FAILURE;
        }        
    }

    PRInt32 nsId = kNameSpaceID_Unknown;
    nsresult rv = gTxNameSpaceManager->RegisterNameSpace(aNamespaceURI, nsId);
    NS_ENSURE_SUCCESS(rv, rv);
    nsCOMPtr<nsIAtom> localName = do_GetAtom(aLocalName);
    txExpandedName varName(nsId, localName);

    txVariable* var = (txVariable*)mVariables.get(varName);
    if (var) {
        var->setValue(aValue);
        return NS_OK;
    }

    var = new txVariable(aValue);
    NS_ENSURE_TRUE(var, NS_ERROR_OUT_OF_MEMORY);

    return mVariables.add(varName, var);
}

NS_IMETHODIMP
txMozillaXSLTProcessor::GetParameter(const nsAString& aNamespaceURI,
                                     const nsAString& aLocalName,
                                     nsIVariant **aResult)
{
    PRInt32 nsId = kNameSpaceID_Unknown;
    nsresult rv = gTxNameSpaceManager->RegisterNameSpace(aNamespaceURI, nsId);
    NS_ENSURE_SUCCESS(rv, rv);
    nsCOMPtr<nsIAtom> localName = do_GetAtom(aLocalName);
    txExpandedName varName(nsId, localName);

    txVariable* var = (txVariable*)mVariables.get(varName);
    if (var) {
        return var->getValue(aResult);
    }
    return NS_OK;
}

NS_IMETHODIMP
txMozillaXSLTProcessor::RemoveParameter(const nsAString& aNamespaceURI,
                                        const nsAString& aLocalName)
{
    PRInt32 nsId = kNameSpaceID_Unknown;
    nsresult rv = gTxNameSpaceManager->RegisterNameSpace(aNamespaceURI, nsId);
    NS_ENSURE_SUCCESS(rv, rv);
    nsCOMPtr<nsIAtom> localName = do_GetAtom(aLocalName);
    txExpandedName varName(nsId, localName);

    mVariables.remove(varName);
    return NS_OK;
}

NS_IMETHODIMP
txMozillaXSLTProcessor::ClearParameters()
{
    mVariables.clear();

    return NS_OK;
}

NS_IMETHODIMP
txMozillaXSLTProcessor::Reset()
{
    mStylesheet = nsnull;
    mVariables.clear();

    return NS_OK;
}

NS_IMETHODIMP
txMozillaXSLTProcessor::LoadStyleSheet(nsIURI* aUri, nsILoadGroup* aLoadGroup,
                                       nsIURI* aReferrerUri)
{
    return TX_LoadSheet(aUri, this, aLoadGroup, aReferrerUri);
}

nsresult
txMozillaXSLTProcessor::setStylesheet(txStylesheet* aStylesheet)
{
    mStylesheet = aStylesheet;
    if (mSource) {
        return DoTransform();
    }
    return NS_OK;
}

/* static*/
nsresult
txVariable::Convert(nsIVariant *aValue, ExprResult** aResult)
{
    *aResult = nsnull;

    PRUint16 dataType;
    aValue->GetDataType(&dataType);
    switch (dataType) {
        // Number
        case nsIDataType::VTYPE_INT8:
        case nsIDataType::VTYPE_INT16:
        case nsIDataType::VTYPE_INT32:
        case nsIDataType::VTYPE_INT64:
        case nsIDataType::VTYPE_UINT8:
        case nsIDataType::VTYPE_UINT16:
        case nsIDataType::VTYPE_UINT32:
        case nsIDataType::VTYPE_UINT64:
        case nsIDataType::VTYPE_FLOAT:
        case nsIDataType::VTYPE_DOUBLE:
        {
            double value;
            nsresult rv = aValue->GetAsDouble(&value);
            NS_ENSURE_SUCCESS(rv, rv);

            *aResult = new NumberResult(value);
            NS_ENSURE_TRUE(aResult, NS_ERROR_OUT_OF_MEMORY);

            return NS_OK;
        }

        // Boolean
        case nsIDataType::VTYPE_BOOL:
        {
            PRBool value;
            nsresult rv = aValue->GetAsBool(&value);
            NS_ENSURE_SUCCESS(rv, rv);

            *aResult = new BooleanResult(value);
            NS_ENSURE_TRUE(aResult, NS_ERROR_OUT_OF_MEMORY);

            return NS_OK;
        }

        // String
        case nsIDataType::VTYPE_CHAR:
        case nsIDataType::VTYPE_WCHAR:
        case nsIDataType::VTYPE_DOMSTRING:
        case nsIDataType::VTYPE_CHAR_STR:
        case nsIDataType::VTYPE_WCHAR_STR:
        case nsIDataType::VTYPE_STRING_SIZE_IS:
        case nsIDataType::VTYPE_WSTRING_SIZE_IS:
        case nsIDataType::VTYPE_UTF8STRING:
        case nsIDataType::VTYPE_CSTRING:
        case nsIDataType::VTYPE_ASTRING:
        {
            nsAutoString value;
            nsresult rv = aValue->GetAsAString(value);
            NS_ENSURE_SUCCESS(rv, rv);

            *aResult = new StringResult(value);
            NS_ENSURE_TRUE(aResult, NS_ERROR_OUT_OF_MEMORY);

            return NS_OK;
        }

        // Nodeset
        case nsIDataType::VTYPE_INTERFACE:
        case nsIDataType::VTYPE_INTERFACE_IS:
        {
            nsID *iid;
            nsCOMPtr<nsISupports> supports;
            nsresult rv = aValue->GetAsInterface(&iid, getter_AddRefs(supports));
            NS_ENSURE_SUCCESS(rv, rv);
            if (iid) {
                // XXX Figure out what the user added and if we can do
                //     anything with it.
                //     nsIDOMNode, nsIDOMNodeList, nsIDOMXPathResult
                nsMemory::Free(iid);
            }
            break;
        }

        case nsIDataType::VTYPE_ARRAY:
        {
            // XXX Figure out what the user added and if we can do
            //     anything with it. Array of Nodes. 
            break;
        }
    }
    return NS_ERROR_ILLEGAL_VALUE;
}
