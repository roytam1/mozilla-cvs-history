/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- 
 *
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is TransforMiiX XSLT processor.
 *
 * The Initial Developer of the Original Code is The MITRE Corporation.
 * Portions created by MITRE are Copyright (C) 1999 The MITRE Corporation.
 *
 * Portions created by Keith Visco as a Non MITRE employee,
 * (C) 1999, 2000 Keith Visco. All Rights Reserved.
 *
 * Contributor(s):
 * Keith Visco, kvisco@ziplink.net
 *    -- original author.
 *
 * Bob Miller, kbob@oblix.com
 *    -- plugged core leak.
 *
 * Pierre Phaneuf, pp@ludusdesign.com
 *    -- fixed some XPCOM usage.
 *
 * Marina Mechtcheriakova, mmarina@mindspring.com
 *    -- Added call to recurisvely attribute-set processing on
 *       xsl:attribute-set itself
 *    -- Added call to handle attribute-set processing for xsl:copy
 *
 * Nathan Pride, npride@wavo.com
 *    -- fixed a document base issue
 *
 * Olivier Gerardin
 *    -- Changed behavior of passing parameters to templates
 *
 */

#include "XSLTProcessor.h"
#include "Names.h"
#include "XMLParser.h"
#include "VariableBinding.h"
#include "XMLUtils.h"
#include "XMLDOMUtils.h"
#include "txNodeSorter.h"
#include "Numbering.h"
#include "Tokenizer.h"
#include "URIUtils.h"
#include "txAtoms.h"
#include "TxLog.h"
#include "txRtfHandler.h"
#include "txNodeSetContext.h"
#include "txSingleNodeContext.h"
#ifndef TX_EXE
#include "nsIDocShell.h"
#include "nsIObserverService.h"
#include "nsIURL.h"
#include "nsIServiceManager.h"
#include "nsIIOService.h"
#include "nsILoadGroup.h"
#include "nsIChannel.h"
#include "nsNetUtil.h"
#include "nsIDOMClassInfo.h"
#include "nsIConsoleService.h"
#else
#include "txHTMLOutput.h"
#include "txTextOutput.h"
#include "txXMLOutput.h"
#endif

  //-----------------------------------/
 //- Implementation of XSLTProcessor -/
//-----------------------------------/

/**
 * XSLTProcessor is a class for Processing XSL stylesheets
**/

/**
 * A warning message used by all templates that do not allow non character
 * data to be generated
**/
const String XSLTProcessor::NON_TEXT_TEMPLATE_WARNING =
"templates for the following element are not allowed to generate non character data: ";

/*
 * Implement static variables for atomservice and dom.
 */
#ifdef TX_EXE
TX_IMPL_ATOM_STATICS;
TX_IMPL_DOM_STATICS;
#endif

/**
 * Creates a new XSLTProcessor
**/
XSLTProcessor::XSLTProcessor() : mOutputHandler(0),
                                 mResultHandler(0)
{
#ifndef TX_EXE
    NS_INIT_ISUPPORTS();
#endif

    xslVersion.append("1.0");
    appName.append("TransforMiiX");
    appVersion.append("1.0 [beta v20010123]");

    // Create default expressions

    // "node()"
    txNodeTest* nt = new txNodeTypeTest(txNodeTypeTest::NODE_TYPE);
    mNodeExpr = new LocationStep(nt, LocationStep::CHILD_AXIS);

} //-- XSLTProcessor

/**
 * Default destructor
**/
XSLTProcessor::~XSLTProcessor()
{
    delete mOutputHandler;
    delete mNodeExpr;
}

#ifndef TX_EXE

// XXX START
// XXX Mozilla module only code. This should move to txMozillaXSLTProcessor
// XXX

NS_IMPL_ADDREF(XSLTProcessor)
NS_IMPL_RELEASE(XSLTProcessor)
NS_INTERFACE_MAP_BEGIN(XSLTProcessor)
    NS_INTERFACE_MAP_ENTRY(nsIDocumentTransformer)
    NS_INTERFACE_MAP_ENTRY(nsIScriptLoaderObserver)
    NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDocumentTransformer)
    NS_INTERFACE_MAP_ENTRY_EXTERNAL_DOM_CLASSINFO(XSLTProcessor)
NS_INTERFACE_MAP_END

// XXX
// XXX Mozilla module only code. This should move to txMozillaXSLTProcessor
// XXX END

#else

/*
 * Initialize atom tables.
 */
MBool txInit()
{
    if (!txNamespaceManager::init())
        return MB_FALSE;
    if (!txHTMLAtoms::init())
        return MB_FALSE;
    if (!txXMLAtoms::init())
        return MB_FALSE;
    if (!txXPathAtoms::init())
        return MB_FALSE;
    return txXSLTAtoms::init();
}

/*
 * To be called when done with transformiix.
 *
 * Free atom table, namespace manager.
 */
MBool txShutdown()
{
    txNamespaceManager::shutdown();
    txHTMLAtoms::shutdown();
    txXMLAtoms::shutdown();
    txXPathAtoms::shutdown();
    txXSLTAtoms::shutdown();
    return MB_TRUE;
}
#endif

/**
 * Registers the given ErrorObserver with this ProcessorState
**/
void XSLTProcessor::addErrorObserver(ErrorObserver& errorObserver) {
    errorObservers.add(&errorObserver);
} //-- addErrorObserver

String& XSLTProcessor::getAppName() {
    return appName;
} //-- getAppName

String& XSLTProcessor::getAppVersion() {
    return appVersion;
} //-- getAppVersion

#ifdef TX_EXE

// XXX START
// XXX Standalone only code. This should move to txStandaloneXSLTProcessor
// XXX

/**
 * Parses all XML Stylesheet PIs associated with the
 * given XML document. If any stylesheet PIs are found with
 * type="text/xsl" the href psuedo attribute value will be
 * added to the given href argument. If multiple text/xsl stylesheet PIs
 * are found, the one closest to the end of the document is used.
**/
void XSLTProcessor::getHrefFromStylesheetPI(Document& xmlDocument, String& href) {

    Node* node = xmlDocument.getFirstChild();
    String type;
    String tmpHref;
    while (node) {
        if ( node->getNodeType() == Node::PROCESSING_INSTRUCTION_NODE ) {
            String target = ((ProcessingInstruction*)node)->getTarget();
            if ( STYLESHEET_PI.isEqual(target) ||
                 STYLESHEET_PI_OLD.isEqual(target) ) {
                String data = ((ProcessingInstruction*)node)->getData();
                type.clear();
                tmpHref.clear();
                parseStylesheetPI(data, type, tmpHref);
                if ( XSL_MIME_TYPE.isEqual(type) ) {
                    href.clear();
                    URIUtils::resolveHref(tmpHref, node->getBaseURI(), href);
                }
            }
        }
        node = node->getNextSibling();
    }

} //-- getHrefFromStylesheetPI

/**
 * Parses the contents of data, and returns the type and href psuedo attributes
**/
void XSLTProcessor::parseStylesheetPI(String& data, String& type, String& href) {

    PRInt32 size = data.length();
    NamedMap bufferMap;
    bufferMap.put("type", &type);
    bufferMap.put("href", &href);
    int ccount = 0;
    MBool inLiteral = MB_FALSE;
    UNICODE_CHAR matchQuote = '"';
    String sink;
    String* buffer = &sink;

    for (ccount = 0; ccount < size; ccount++) {
        UNICODE_CHAR ch = data.charAt(ccount);
        switch ( ch ) {
            case ' ' :
                if ( inLiteral ) {
                    buffer->append(ch);
                }
                break;
            case '=':
                if ( inLiteral ) buffer->append(ch);
                else if (!buffer->isEmpty()) {
                    buffer = (String*)bufferMap.get(*buffer);
                    if ( !buffer ) {
                        sink.clear();
                        buffer = &sink;
                    }
                }
                break;
            case '"' :
            case '\'':
                if (inLiteral) {
                    if ( matchQuote == ch ) {
                        inLiteral = MB_FALSE;
                        sink.clear();
                        buffer = &sink;
                    }
                    else buffer->append(ch);
                }
                else {
                    inLiteral = MB_TRUE;
                    matchQuote = ch;
                }
                break;
            default:
                buffer->append(ch);
                break;
        }
    }

} //-- parseStylesheetPI

/**
 * Processes the given XML Document, the XSL stylesheet
 * will be retrieved from the XML Stylesheet Processing instruction,
 * otherwise an empty document will be returned.
 * @param xmlDocument the XML document to process
 * @param documentBase the document base of the XML document, for
 * resolving relative URIs
 * @return the result tree.
**/
Document* XSLTProcessor::process(Document& xmlDocument) {
    //-- look for Stylesheet PI
    Document xslDocument; //-- empty for now
    return process(xmlDocument, xslDocument);
} //-- process

/**
 * Reads an XML Document from the given XML input stream, and
 * processes the document using the XSL document derived from
 * the given XSL input stream.
 * @return the result tree.
**/
Document* XSLTProcessor::process
    (istream& xmlInput, String& xmlFilename,
     istream& xslInput, String& xslFilename) {
    //-- read in XML Document
    XMLParser xmlParser;
    Document* xmlDoc = xmlParser.parse(xmlInput, xmlFilename);
    if (!xmlDoc) {
        String err("error reading XML document: ");
        err.append(xmlParser.getErrorString());
        cerr << err << endl;
        return 0;
    }
    //-- Read in XSL document
    Document* xslDoc = xmlParser.parse(xslInput, xslFilename);
    if (!xslDoc) {
        String err("error reading XSL stylesheet document: ");
        err.append(xmlParser.getErrorString());
        cerr << err << endl;
        delete xmlDoc;
        return 0;
    }
    Document* result = process(*xmlDoc, *xslDoc);
    delete xmlDoc;
    delete xslDoc;
    return result;
} //-- process

/**
 * Reads an XML document from the given XML input stream. The
 * XML document is processed using the associated XSL document
 * retrieved from the XML document's Stylesheet Processing Instruction,
 * otherwise an empty document will be returned.
 * @param xmlDocument the XML document to process
 * @param documentBase the document base of the XML document, for
 * resolving relative URIs
 * @return the result tree.
**/
Document* XSLTProcessor::process(istream& xmlInput, String& xmlFilename) {
    //-- read in XML Document
    XMLParser xmlParser;
    Document* xmlDoc = xmlParser.parse(xmlInput, xmlFilename);
    if (!xmlDoc) {
        String err("error reading XML document: ");
        err.append(xmlParser.getErrorString());
        cerr << err << endl;
        return 0;
    }
    //-- Read in XSL document
    String href;
    String errMsg;
    getHrefFromStylesheetPI(*xmlDoc, href);
    istream* xslInput = URIUtils::getInputStream(href,errMsg);
    Document* xslDoc = 0;
    if ( xslInput ) {
        xslDoc = xmlParser.parse(*xslInput, href);
        delete xslInput;
    }
    if (!xslDoc) {
        String err("error reading XSL stylesheet document: ");
        err.append(xmlParser.getErrorString());
        cerr << err << endl;
        delete xmlDoc;
        return 0;
    }
    Document* result = process(*xmlDoc, *xslDoc);
    delete xmlDoc;
    delete xslDoc;
    return result;
} //-- process

// XXX
// XXX Standalone only code. This should move to txStandaloneXSLTProcessor
// XXX END

#endif

/**
 * Processes the Top level elements for an XSL stylesheet
**/
void XSLTProcessor::processStylesheet(Document* aSource,
                                      Document* aStylesheet,
                                      ListIterator* aImportFrame,
                                      ProcessorState* aPs)
{
    NS_ASSERTION(aStylesheet, "processTopLevel called without stylesheet");
    if (!aStylesheet || !aStylesheet->getDocumentElement())
        return;

    Element* elem = aStylesheet->getDocumentElement();

    txAtom* localName;
    PRInt32 namespaceID = elem->getNamespaceID();
    elem->getLocalName(&localName);

    if (((localName == txXSLTAtoms::stylesheet) ||
         (localName == txXSLTAtoms::transform)) &&
        (namespaceID == kNameSpaceID_XSLT)) {
        processTopLevel(aSource, elem, aImportFrame, aPs);
    }
    else {
        NS_ASSERTION(aImportFrame->current(), "no current importframe");
        if (!aImportFrame->current()) {
            TX_IF_RELEASE_ATOM(localName);
            return;
        }
        aPs->addLREStylesheet(aStylesheet,
            (ProcessorState::ImportFrame*)aImportFrame->current());
    }
    TX_IF_RELEASE_ATOM(localName);
}

/**
 * Processes the Top level elements for an XSL stylesheet
**/
void XSLTProcessor::processTopLevel(Document* aSource,
                                    Element* aStylesheet,
                                    ListIterator* importFrame,
                                    ProcessorState* aPs)
{
    // Index templates and process top level xsl elements
    NS_ASSERTION(aStylesheet, "processTopLevel called without stylesheet element");
    if (!aStylesheet)
        return;

    ProcessorState::ImportFrame* currentFrame =
        (ProcessorState::ImportFrame*)importFrame->current();

    NS_ASSERTION(currentFrame,
                 "processTopLevel called with no current importframe");
    if (!currentFrame)
        return;

    NS_ASSERTION(aSource, "processTopLevel called without source document");

    MBool importsDone = MB_FALSE;
    Node* node = aStylesheet->getFirstChild();
    while (node && !importsDone) {
        if (node->getNodeType() == Node::ELEMENT_NODE) {
            txAtom* localName;
            node->getLocalName(&localName);
            if (node->getNamespaceID() == kNameSpaceID_XSLT &&
                localName == txXSLTAtoms::import) {
                Element* element = (Element*)node;
                String hrefAttr, href;
                element->getAttr(txXSLTAtoms::href, kNameSpaceID_None,
                                 hrefAttr);
                URIUtils::resolveHref(hrefAttr, element->getBaseURI(),
                                      href);

                // Create a new ImportFrame with correct firstNotImported
                ProcessorState::ImportFrame *nextFrame, *newFrame;
                nextFrame =
                    (ProcessorState::ImportFrame*)importFrame->next();
                newFrame = new ProcessorState::ImportFrame(nextFrame);
                if (!newFrame) {
                    // XXX ErrorReport: out of memory
                    break;
                }

                // Insert frame and process stylesheet
                importFrame->addBefore(newFrame);
                importFrame->previous();
                processInclude(href, aSource, importFrame, aPs);

                // Restore iterator to initial position
                importFrame->previous();
            }
            else {
                importsDone = MB_TRUE;
            }
            TX_IF_RELEASE_ATOM(localName);
        }
        if (!importsDone)
            node = node->getNextSibling();
    }

    while (node) {
        if (node->getNodeType() != Node::ELEMENT_NODE ||
            node->getNamespaceID() != kNameSpaceID_XSLT) {
            node = node->getNextSibling();
            continue;
        }

        txAtom* localName;
        node->getLocalName(&localName);
        Element* element = (Element*)node;
        // xsl:attribute-set
        if (localName == txXSLTAtoms::attributeSet) {
            aPs->addAttributeSet(element, currentFrame);
        }
        // xsl:decimal-format
        else if (localName == txXSLTAtoms::decimalFormat) {
            if (!aPs->addDecimalFormat(element)) {
                // Add error to ErrorObserver
                String fName;
                element->getAttr(txXSLTAtoms::name, kNameSpaceID_None,
                                 fName);
                String err("unable to add ");
                if (fName.isEmpty()) {
                    err.append("default");
                }
                else {
                    err.append("\"");
                    err.append(fName);
                    err.append("\"");
                }
                err.append(" decimal format for xsl:decimal-format");
                aPs->receiveError(err, NS_ERROR_FAILURE);
            }
        }
        // xsl:param
        else if (localName == txXSLTAtoms::param) {
            String name;
            element->getAttr(txXSLTAtoms::name, kNameSpaceID_None,
                             name);
            if (name.isEmpty()) {
                String err("missing required name attribute for xsl:param");
                aPs->receiveError(err, NS_ERROR_FAILURE);
                break;
            }
            ExprResult* exprResult = processVariable(aSource, element, aPs);
            bindVariable(name, exprResult, MB_TRUE, aPs);
        }
        // xsl:import
        else if (localName == txXSLTAtoms::import) {
            String err("xsl:import only allowed at top of stylesheet");
            aPs->receiveError(err, NS_ERROR_FAILURE);
        }
        // xsl:include
        else if (localName == txXSLTAtoms::include) {
            String hrefAttr, href;
            element->getAttr(txXSLTAtoms::href, kNameSpaceID_None,
                             hrefAttr);
            URIUtils::resolveHref(hrefAttr, element->getBaseURI(),
                                  href);

            processInclude(href, aSource, importFrame, aPs);
        }
        // xsl:key
        else if (localName == txXSLTAtoms::key) {
            if (!aPs->addKey(element)) {
                String name;
                element->getAttr(txXSLTAtoms::name, kNameSpaceID_None,
                                 name);
                String err("error adding key '");
                err.append(name);
                err.append("'");
                aPs->receiveError(err, NS_ERROR_FAILURE);
            }
        }
        // xsl:output
        else if (localName == txXSLTAtoms::output) {
            txOutputFormat& format = currentFrame->mOutputFormat;
            String attValue;

            if (element->getAttr(txXSLTAtoms::method, kNameSpaceID_None,
                                 attValue)) {
                if (attValue.isEqual("html")) {
                    format.mMethod = eHTMLOutput;
                }
                else if (attValue.isEqual("text")) {
                    format.mMethod = eTextOutput;
                }
                else {
                    format.mMethod = eXMLOutput;
                }
            }

            if (element->getAttr(txXSLTAtoms::version, kNameSpaceID_None,
                                 attValue)) {
                format.mVersion = attValue;
            }

            if (element->getAttr(txXSLTAtoms::encoding, kNameSpaceID_None,
                                 attValue)) {
                format.mEncoding = attValue;
            }

            if (element->getAttr(txXSLTAtoms::omitXmlDeclaration,
                                 kNameSpaceID_None, attValue)) {
                format.mOmitXMLDeclaration = attValue.isEqual(YES_VALUE) ? eTrue : eFalse;
            }

            if (element->getAttr(txXSLTAtoms::standalone, kNameSpaceID_None,
                                 attValue)) {
                format.mStandalone = attValue.isEqual(YES_VALUE) ? eTrue : eFalse;
            }

            if (element->getAttr(txXSLTAtoms::doctypePublic,
                                 kNameSpaceID_None, attValue)) {
                format.mPublicId = attValue;
            }

            if (element->getAttr(txXSLTAtoms::doctypeSystem,
                                 kNameSpaceID_None, attValue)) {
                format.mSystemId = attValue;
            }

            if (element->getAttr(txXSLTAtoms::cdataSectionElements,
                                 kNameSpaceID_None, attValue)) {
                txTokenizer tokens(attValue);
                String token;
                while (tokens.hasMoreTokens()) {
                    tokens.nextToken(token);
                    if (!XMLUtils::isValidQName(token)) {
                        break;
                    }

                    String namePart;
                    XMLUtils::getPrefix(token, namePart);
                    txAtom* nameAtom = TX_GET_ATOM(namePart);
                    PRInt32 nsID = element->lookupNamespaceID(nameAtom);
                    TX_IF_RELEASE_ATOM(nameAtom);
                    if (nsID == kNameSpaceID_Unknown) {
                        // XXX ErrorReport: unknown prefix
                        break;
                    }
                    XMLUtils::getLocalPart(token, namePart);
                    nameAtom = TX_GET_ATOM(namePart);
                    if (!nameAtom) {
                        // XXX ErrorReport: out of memory
                        break;
                    }
                    txExpandedName* qname = new txExpandedName(nsID, nameAtom);
                    TX_RELEASE_ATOM(nameAtom);
                    if (!qname) {
                        // XXX ErrorReport: out of memory
                        break;
                    }
                    format.mCDATASectionElements.add(qname);
                }
            }

            if (element->getAttr(txXSLTAtoms::indent, kNameSpaceID_None,
                                 attValue)) {
                format.mIndent = attValue.isEqual(YES_VALUE) ? eTrue : eFalse;
            }

            if (element->getAttr(txXSLTAtoms::mediaType, kNameSpaceID_None,
                                 attValue)) {
                format.mMediaType = attValue;
            }
        }
        // xsl:template
        else if (localName == txXSLTAtoms::_template) {
            aPs->addTemplate(element, currentFrame);
        }
        // xsl:variable
        else if (localName == txXSLTAtoms::variable) {
            String name;
            element->getAttr(txXSLTAtoms::name, kNameSpaceID_None,
                             name);
            if (name.isEmpty()) {
                String err("missing required name attribute for xsl:variable");
                aPs->receiveError(err, NS_ERROR_FAILURE);
                break;
            }
            ExprResult* exprResult = processVariable(aSource, element, aPs);
            bindVariable(name, exprResult, MB_FALSE, aPs);
        }
        // xsl:preserve-space
        else if (localName == txXSLTAtoms::preserveSpace) {
            String elements;
            if (!element->getAttr(txXSLTAtoms::elements,
                                  kNameSpaceID_None, elements)) {
                //-- add error to ErrorObserver
                String err("missing required 'elements' attribute for ");
                err.append("xsl:preserve-space");
                aPs->receiveError(err, NS_ERROR_FAILURE);
            }
            else {
                aPs->shouldStripSpace(elements, element,
                                      MB_FALSE,
                                      currentFrame);
            }
        }
        // xsl:strip-space
        else if (localName == txXSLTAtoms::stripSpace) {
            String elements;
            if (!element->getAttr(txXSLTAtoms::elements,
                                  kNameSpaceID_None, elements)) {
                //-- add error to ErrorObserver
                String err("missing required 'elements' attribute for ");
                err.append("xsl:strip-space");
                aPs->receiveError(err, NS_ERROR_FAILURE);
            }
            else {
                aPs->shouldStripSpace(elements, element,
                                      MB_TRUE,
                                      currentFrame);
            }
        }
        TX_IF_RELEASE_ATOM(localName);
        node = node->getNextSibling();
    }
}

/*
 * Processes an include or import stylesheet
 * @param aHref    URI of stylesheet to process
 * @param aSource  source document
 * @param aImportFrame current importFrame iterator
 * @param aPs      current ProcessorState
 */
void XSLTProcessor::processInclude(String& aHref,
                                   Document* aSource,
                                   ListIterator* aImportFrame,
                                   ProcessorState* aPs)
{
    // make sure the include isn't included yet
    StackIterator* iter = aPs->getEnteredStylesheets()->iterator();
    if (!iter) {
        // XXX report out of memory
        return;
    }
    
    while (iter->hasNext()) {
        if (((String*)iter->next())->isEqual(aHref)) {
            String err("Stylesheet includes itself. URI: ");
            err.append(aHref);
            aPs->receiveError(err, NS_ERROR_FAILURE);
            delete iter;
            return;
        }
    }
    aPs->getEnteredStylesheets()->push(&aHref);
    delete iter;

    // Load XSL document
    Node* stylesheet = aPs->retrieveDocument(aHref, NULL_STRING);
    if (!stylesheet) {
        String err("Unable to load included stylesheet ");
        err.append(aHref);
        aPs->receiveError(err, NS_ERROR_FAILURE);
        aPs->getEnteredStylesheets()->pop();
        return;
    }

    switch(stylesheet->getNodeType()) {
        case Node::DOCUMENT_NODE :
            processStylesheet(aSource,
                              (Document*)stylesheet,
                              aImportFrame,
                              aPs);
            break;
        case Node::ELEMENT_NODE :
            processTopLevel(aSource, (Element*)stylesheet, aImportFrame, aPs);
            break;
        default:
            // This should never happen
            String err("Unsupported fragment identifier");
            aPs->receiveError(err, NS_ERROR_FAILURE);
            break;
    }

    aPs->getEnteredStylesheets()->pop();
}

#ifdef TX_EXE

// XXX START
// XXX Standalone only code. This should move to txStandaloneXSLTProcessor
// XXX

/*
 * Processes the given XML Document using the given XSL document
 * and returns the result tree
 */
Document* XSLTProcessor::process(Document& xmlDocument,
                                 Document& xslDocument)
{
    Document* result = new Document();
    if (!result)
        // XXX ErrorReport: out of memory
        return 0;

    /* XXX Disabled for now, need to implement a handler
           that creates a result tree.
    // Start of block to ensure the destruction of the ProcessorState
    // before the destruction of the result document.
    {
        // Create a new ProcessorState
        ProcessorState ps(&aXMLDocument, &aXSLTDocument, &result);
    
        // Add error observers
        txListIterator iter(&errorObservers);
        while (iter.hasNext())
            ps.addErrorObserver(*(ErrorObserver*)iter.next());
    
        txSingleNodeContext evalContext(&aXMLDocument, &ps);
        ps.setEvalContext(&evalContext);

        // Index templates and process top level xsl elements
        txListIterator importFrame(ps.getImportFrames());
        importFrame.addAfter(new ProcessorState::ImportFrame(0));
        if (!importFrame.next()) {
            delete result;
            // XXX ErrorReport: out of memory
            return 0;
        }
        processStylesheet(&aXMLDocument, &aXSLTDocument, &importFrame, &ps);
    
        initializeHandlers(&ps);
        // XXX Set the result document on the handler
    
        // Process root of XML source document
        startTransform(&aXMLDocument, &ps);
    }
    // End of block to ensure the destruction of the ProcessorState
    // before the destruction of the result document.
       XXX End of disabled section */

    // Return result Document
    return result;
}

/*
 * Processes the given XML Document using the given XSL document
 * and prints the results to the given ostream argument
 */
void XSLTProcessor::process(Document& aXMLDocument,
                            Node& aStylesheet,
                            ostream& aOut)
{
    // Need a result document for creating result tree fragments.
    Document result;

    // Start of block to ensure the destruction of the ProcessorState
    // before the destruction of the result document.
    {
        // Create a new ProcessorState
        Document* stylesheetDoc = 0;
        Element* stylesheetElem = 0;
        if (aStylesheet.getNodeType() == Node::DOCUMENT_NODE) {
            stylesheetDoc = (Document*)&aStylesheet;
        }
        else {
            stylesheetElem = (Element*)&aStylesheet;
            stylesheetDoc = aStylesheet.getOwnerDocument();
        }
        ProcessorState ps(&aXMLDocument, stylesheetDoc, &result);

        // Add error observers
        txListIterator iter(&errorObservers);
        while (iter.hasNext())
            ps.addErrorObserver(*(ErrorObserver*)iter.next());

        txSingleNodeContext evalContext(&aXMLDocument, &ps);
        ps.setEvalContext(&evalContext);

        // Index templates and process top level xsl elements
        txListIterator importFrame(ps.getImportFrames());
        importFrame.addAfter(new ProcessorState::ImportFrame(0));
        if (!importFrame.next())
            // XXX ErrorReport: out of memory
            return;
        
        if (stylesheetElem)
            processTopLevel(&aXMLDocument, stylesheetElem, &importFrame, &ps);
        else
            processStylesheet(&aXMLDocument, stylesheetDoc, &importFrame, &ps);

        initializeHandlers(&ps);
        if (mOutputHandler)
            mOutputHandler->setOutputStream(&aOut);

        // Process root of XML source document
        startTransform(&aXMLDocument, &ps);
    }
    // End of block to ensure the destruction of the ProcessorState
    // before the destruction of the result document.
}

/**
 * Reads an XML Document from the given XML input stream.
 * The XSL Stylesheet is obtained from the XML Documents stylesheet PI.
 * If no Stylesheet is found, an empty document will be the result;
 * otherwise the XML Document is processed using the stylesheet.
 * The result tree is printed to the given ostream argument,
 * will not close the ostream argument
**/
void XSLTProcessor::process
   (istream& xmlInput, String& xmlFilename, ostream& out)
{

    XMLParser xmlParser;
    Document* xmlDoc = xmlParser.parse(xmlInput, xmlFilename);
    if (!xmlDoc) {
        String err("error reading XML document: ");
        err.append(xmlParser.getErrorString());
        cerr << err << endl;
        return;
    }
    //-- Read in XSL document
    String href;
    String errMsg;
    getHrefFromStylesheetPI(*xmlDoc, href);
    istream* xslInput = URIUtils::getInputStream(href,errMsg);
    Document* xslDoc = 0;
    if ( xslInput ) {
        xslDoc = xmlParser.parse(*xslInput, href);
        delete xslInput;
    }
    if (!xslDoc) {
        String err("error reading XSL stylesheet document: ");
        err.append(xmlParser.getErrorString());
        cerr << err << endl;
        delete xmlDoc;
        return;
    }

    Node* stylesheet;
    String frag;
    URIUtils::getFragmentIdentifier(href, frag);
    if (!frag.isEmpty()) {
        stylesheet = xslDoc->getElementById(frag);
        if (!stylesheet) {
            String err("unable to get fragment");
            cerr << err << endl;
            delete xmlDoc;
            delete xslDoc;
            return;
        }
    }
    else {
        stylesheet = xslDoc;
    }

    process(*xmlDoc, *stylesheet, out);
    delete xmlDoc;
    delete xslDoc;
} //-- process

/**
 * Reads an XML Document from the given XML input stream, and
 * processes the document using the XSL document derived from
 * the given XSL input stream.
 * The result tree is printed to the given ostream argument,
 * will not close the ostream argument
**/
void XSLTProcessor::process
   (istream& xmlInput, String& xmlFilename,
    istream& xslInput, String& xslFilename,
    ostream& out)
{
    //-- read in XML Document
    XMLParser xmlParser;
    Document* xmlDoc = xmlParser.parse(xmlInput, xmlFilename);
    if (!xmlDoc) {
        String err("error reading XML document: ");
        err.append(xmlParser.getErrorString());
        cerr << err << endl;
        return;
    }
    //-- read in XSL Document
    Document* xslDoc = xmlParser.parse(xslInput, xslFilename);
    if (!xslDoc) {
        String err("error reading XSL stylesheet document: ");
        err.append(xmlParser.getErrorString());
        cerr << err << endl;
        delete xmlDoc;
        return;
    }
    process(*xmlDoc, *xslDoc, out);
    delete xmlDoc;
    delete xslDoc;
} //-- process

// XXX
// XXX Standalone only code. This should move to txStandaloneXSLTProcessor
// XXX END

#endif // ifdef TX_EXE

  //-------------------/
 //- Private Methods -/
//-------------------/

void XSLTProcessor::bindVariable
    (String& name, ExprResult* value, MBool allowShadowing, ProcessorState* ps)
{
    NamedMap* varSet = (NamedMap*)ps->getVariableSetStack()->peek();
    //-- check for duplicate variable names
    VariableBinding* current = (VariableBinding*) varSet->get(name);
    VariableBinding* binding = 0;
    if (current) {
        binding = current;
        if (current->isShadowingAllowed() ) {
            current->setShadowValue(value);
        }
        else {
            //-- error cannot rebind variables
            String err("cannot rebind variables: ");
            err.append(name);
            err.append(" already exists in this scope.");
            ps->receiveError(err, NS_ERROR_FAILURE);
        }
    }
    else {
        binding = new VariableBinding(name, value);
        varSet->put((const String&)name, binding);
    }
    if ( allowShadowing ) binding->allowShadowing();
    else binding->disallowShadowing();

} //-- bindVariable

void XSLTProcessor::process(Node* node,
                            const String& mode,
                            ProcessorState* ps) {
    if (!node)
        return;

    ProcessorState::ImportFrame *frame;
    Node* xslTemplate = ps->findTemplate(node, mode, &frame);
    processMatchedTemplate(xslTemplate, node, 0, NULL_STRING, frame, ps);
} //-- process

void XSLTProcessor::processAction(Node* aNode,
                                  Node* aXSLTAction,
                                  ProcessorState* aPs)
{
    NS_ASSERTION(aXSLTAction, "We need an action to process.");
    if (!aXSLTAction)
        return;

    Document* resultDoc = aPs->getResultDocument();

    unsigned short nodeType = aXSLTAction->getNodeType();

    // Handle text nodes
    if (nodeType == Node::TEXT_NODE ||
        nodeType == Node::CDATA_SECTION_NODE) {
        const String& textValue = aXSLTAction->getNodeValue();
        if (!aPs->isXSLStripSpaceAllowed(aXSLTAction) ||
            !XMLUtils::shouldStripTextnode(textValue)) {
            NS_ASSERTION(mResultHandler, "mResultHandler must not be NULL!");
            mResultHandler->characters(textValue);
        }
        return;
    }

    if (nodeType != Node::ELEMENT_NODE) {
        return;
    }

    // Handle element nodes
    Element* actionElement = (Element*)aXSLTAction;
    PRInt32 nsID = aXSLTAction->getNamespaceID();
    if (nsID != kNameSpaceID_XSLT) {
        // Literal result element
        // XXX TODO Check for excluded namespaces and aliased namespaces (element and attributes) 
        const String& nodeName = aXSLTAction->getNodeName();
        startElement(aPs, nodeName, nsID);

        processAttributeSets(actionElement, aNode, aPs);

        // Handle attributes
        NamedNodeMap* atts = actionElement->getAttributes();

        if (atts) {
            // Process all non XSLT attributes
            PRUint32 i;
            for (i = 0; i < atts->getLength(); ++i) {
                Attr* attr = (Attr*)atts->item(i);
                if (attr->getNamespaceID() == kNameSpaceID_XSLT)
                    continue;
                // Process Attribute Value Templates
                String value;
                aPs->processAttrValueTemplate(attr->getValue(), actionElement, value);
                NS_ASSERTION(mResultHandler, "mResultHandler must not be NULL!");
                mResultHandler->attribute(attr->getName(), attr->getNamespaceID(), value);
            }
        }

        // Process children
        processChildren(aNode, actionElement, aPs);
        NS_ASSERTION(mResultHandler, "mResultHandler must not be NULL!");
        mResultHandler->endElement(nodeName, nsID);
        return;
    }

    Expr* expr = 0;
    txAtom* localName;
    aXSLTAction->getLocalName(&localName);
    // xsl:apply-imports
    if (localName == txXSLTAtoms::applyImports) {
        ProcessorState::TemplateRule* curr;
        Node* xslTemplate;
        ProcessorState::ImportFrame *frame;

        curr = aPs->getCurrentTemplateRule();
        if (!curr) {
            String err("apply-imports not allowed here");
            aPs->receiveError(err, NS_ERROR_FAILURE);
            TX_RELEASE_ATOM(localName);
            return;
        }

        xslTemplate = aPs->findTemplate(aNode, *curr->mMode,
                                        curr->mFrame, &frame);
        processMatchedTemplate(xslTemplate, aNode, curr->mParams,
                               *curr->mMode, frame, aPs);

    }
    // xsl:apply-templates
    else if (localName == txXSLTAtoms::applyTemplates) {
        if (actionElement->hasAttr(txXSLTAtoms::select,
                                   kNameSpaceID_None))
            expr = aPs->getExpr(actionElement,
                                ProcessorState::SelectAttr);
        else
            expr = mNodeExpr;

        if (!expr) {
            TX_RELEASE_ATOM(localName);
            return;
        }

        ExprResult* exprResult = expr->evaluate(aPs->getEvalContext());
        if (!exprResult) {
            TX_RELEASE_ATOM(localName);
            return;
        }

        if (exprResult->getResultType() == ExprResult::NODESET) {
            NodeSet* nodeSet = (NodeSet*)exprResult;

            // Look for xsl:sort elements
            txNodeSorter sorter(aPs);
            Node* child = actionElement->getFirstChild();
            while (child) {
                if (child->getNodeType() == Node::ELEMENT_NODE &&
                    child->getNamespaceID() == kNameSpaceID_XSLT) {
                    txAtom* childLocalName;
                    child->getLocalName(&childLocalName);
                    if (childLocalName == txXSLTAtoms::sort) {
                        sorter.addSortElement((Element*)child, aNode);
                    }
                    TX_IF_RELEASE_ATOM(childLocalName);
                }
                child = child->getNextSibling();
            }
            sorter.sortNodeSet(nodeSet);

            // Process xsl:with-param elements
            NamedMap* actualParams = processParameters(actionElement, aNode, aPs);

            String mode;
            actionElement->getAttr(txXSLTAtoms::mode,
                                   kNameSpaceID_None, mode);

            txNodeSetContext evalContext(nodeSet, aPs);
            txIEvalContext* priorEC =
                aPs->setEvalContext(&evalContext);
            while (evalContext.hasNext()) {
                evalContext.next();
                ProcessorState::ImportFrame *frame;
                Node* currNode = evalContext.getContextNode();
                Node* xslTemplate;
                xslTemplate = aPs->findTemplate(currNode, mode, &frame);
                processMatchedTemplate(xslTemplate, currNode,
                                       actualParams, mode, frame, aPs);
            }

            aPs->setEvalContext(priorEC);

            delete actualParams;
        }
        else {
            String err("error processing apply-templates");
            aPs->receiveError(err, NS_ERROR_FAILURE);
        }
        //-- clean up
        delete exprResult;
    }
    // xsl:attribute
    else if (localName == txXSLTAtoms::attribute) {
        String nameAttr;
        if (!actionElement->getAttr(txXSLTAtoms::name,
                                    kNameSpaceID_None, nameAttr)) {
            String err("missing required name attribute for xsl:attribute");
            aPs->receiveError(err, NS_ERROR_FAILURE);
            TX_RELEASE_ATOM(localName);
            return;
        }

        // Process name as an AttributeValueTemplate
        String name;
        aPs->processAttrValueTemplate(nameAttr, actionElement, name);

        // Check name validity (must be valid QName and not xmlns)
        if (!XMLUtils::isValidQName(name)) {
            String err("error processing xsl:attribute, ");
            err.append(name);
            err.append(" is not a valid QName.");
            aPs->receiveError(err, NS_ERROR_FAILURE);
            TX_RELEASE_ATOM(localName);
            return;
        }

        txAtom* nameAtom = TX_GET_ATOM(name);
        if (nameAtom == txXMLAtoms::xmlns) {
            TX_RELEASE_ATOM(nameAtom);
            String err("error processing xsl:attribute, name is xmlns.");
            aPs->receiveError(err, NS_ERROR_FAILURE);
            TX_RELEASE_ATOM(localName);
            return;
        }
        TX_IF_RELEASE_ATOM(nameAtom);

        // Determine namespace URI from the namespace attribute or
        // from the prefix of the name (using the xslt action element).
        String resultNs;
        PRInt32 resultNsID = kNameSpaceID_None;
        if (actionElement->getAttr(txXSLTAtoms::_namespace, kNameSpaceID_None,
                                   resultNs)) {
            String nsURI;
            aPs->processAttrValueTemplate(resultNs, actionElement,
                                          nsURI);
            resultNsID = resultDoc->namespaceURIToID(nsURI);
        }
        else {
            String prefix;
            XMLUtils::getPrefix(name, prefix);
            txAtom* prefixAtom = TX_GET_ATOM(prefix);
            if (prefixAtom != txXMLAtoms::_empty) {
                if (prefixAtom != txXMLAtoms::xmlns)
                    resultNsID = actionElement->lookupNamespaceID(prefixAtom);
                else
                    // Cut xmlns: (6 characters)
                    name.deleteChars(0, 6);
            }
            TX_IF_RELEASE_ATOM(prefixAtom);
        }

        // XXX Should verify that this is correct behaviour. Signal error too?
        if (resultNsID == kNameSpaceID_Unknown) {
            TX_RELEASE_ATOM(localName);
            return;
        }

        // Compute value
        String value;
        processChildrenAsValue(aNode, actionElement, aPs, MB_TRUE, value);

        NS_ASSERTION(mResultHandler, "mResultHandler must not be NULL!");
        mResultHandler->attribute(name, resultNsID, value);
    }
    // xsl:call-template
    else if (localName == txXSLTAtoms::callTemplate) {
        String templateName;
        if (actionElement->getAttr(txXSLTAtoms::name,
                                   kNameSpaceID_None, templateName)) {
            Element* xslTemplate = aPs->getNamedTemplate(templateName);
            if (xslTemplate) {
#ifdef PR_LOGGING
                char *nameBuf = 0, *uriBuf = 0;
                PR_LOG(txLog::xslt, PR_LOG_DEBUG,
                       ("CallTemplate, Name %s, Stylesheet %s\n",
                        (nameBuf = templateName.toCharArray()),
                        (uriBuf = xslTemplate->getBaseURI().toCharArray())));
                delete nameBuf;
                delete uriBuf;
#endif
                NamedMap* actualParams = processParameters(actionElement, aNode, aPs);
                processTemplate(aNode, xslTemplate, aPs, actualParams);
                delete actualParams;
            }
        }
        else {
            String err("missing required name attribute for xsl:call-template");
            aPs->receiveError(err, NS_ERROR_FAILURE);
        }
    }
    // xsl:choose
    else if (localName == txXSLTAtoms::choose) {
        Node* condition = actionElement->getFirstChild();
        MBool caseFound = MB_FALSE;
        while (!caseFound && condition) {
            if (condition->getNodeType() != Node::ELEMENT_NODE ||
                condition->getNamespaceID() != kNameSpaceID_XSLT) {
                condition = condition->getNextSibling();
                continue;
            }

            Element* xslTemplate = (Element*)condition;
            txAtom* conditionLocalName;
            condition->getLocalName(&conditionLocalName);
            if (conditionLocalName == txXSLTAtoms::when) {
                expr = aPs->getExpr(xslTemplate,
                                    ProcessorState::TestAttr);
                if (!expr) {
                    TX_RELEASE_ATOM(conditionLocalName);
                    condition = condition->getNextSibling();
                    continue;
                }

                ExprResult* result = expr->evaluate
                    (aPs->getEvalContext());
                if (result && result->booleanValue()) {
                    processChildren(aNode, xslTemplate, aPs);
                    caseFound = MB_TRUE;
                }
                delete result;
            }
            else if (conditionLocalName == txXSLTAtoms::otherwise) {
                processChildren(aNode, xslTemplate, aPs);
                caseFound = MB_TRUE;
            }
            TX_IF_RELEASE_ATOM(conditionLocalName);
            condition = condition->getNextSibling();
        } // end for-each child of xsl:choose
    }
    // xsl:comment
    else if (localName == txXSLTAtoms::comment) {
        String value;
        processChildrenAsValue(aNode, actionElement, aPs, MB_TRUE, value);
        PRInt32 pos = 0;
        PRInt32 length = value.length();
        while ((pos = value.indexOf('-', pos)) != NOT_FOUND) {
            ++pos;
            if ((pos == length) || (value.charAt(pos) == '-'))
                value.insert(pos++, ' ');
        }
        NS_ASSERTION(mResultHandler, "mResultHandler must not be NULL!");
        mResultHandler->comment(value);
    }
    // xsl:copy
    else if (localName == txXSLTAtoms::copy) {
        xslCopy(aNode, actionElement, aPs);
    }
    // xsl:copy-of
    else if (localName == txXSLTAtoms::copyOf) {
        expr = aPs->getExpr(actionElement, ProcessorState::SelectAttr);
        if (!expr) {
            TX_RELEASE_ATOM(localName);
            return;
        }

        ExprResult* exprResult = expr->evaluate(aPs->getEvalContext());
        xslCopyOf(exprResult, aPs);
        delete exprResult;
    }
    // xsl:element
    else if (localName == txXSLTAtoms::element) {
        String nameAttr;
        if (!actionElement->getAttr(txXSLTAtoms::name,
                                    kNameSpaceID_None, nameAttr)) {
            String err("missing required name attribute for xsl:element");
            aPs->receiveError(err, NS_ERROR_FAILURE);
            TX_RELEASE_ATOM(localName);
            return;
        }

        // Process name as an AttributeValueTemplate
        String name;
        aPs->processAttrValueTemplate(nameAttr, actionElement, name);

        // Check name validity (must be valid QName and not xmlns)
        if (!XMLUtils::isValidQName(name)) {
            String err("error processing xsl:element, '");
            err.append(name);
            err.append("' is not a valid QName.");
            aPs->receiveError(err, NS_ERROR_FAILURE);
            // XXX We should processChildren without creating attributes or
            //     namespace nodes.
            TX_RELEASE_ATOM(localName);
            return;
        }

        // Determine namespace URI from the namespace attribute or
        // from the prefix of the name (using the xslt action element).
        String resultNs;
        PRInt32 resultNsID;
        if (actionElement->getAttr(txXSLTAtoms::_namespace, kNameSpaceID_None, resultNs)) {
            String nsURI;
            aPs->processAttrValueTemplate(resultNs, actionElement, nsURI);
            if (nsURI.isEmpty())
                resultNsID = kNameSpaceID_None;
            else
                resultNsID = resultDoc->namespaceURIToID(nsURI);
        }
        else {
            String prefix;
            XMLUtils::getPrefix(name, prefix);
            txAtom* prefixAtom = TX_GET_ATOM(prefix);
            resultNsID = actionElement->lookupNamespaceID(prefixAtom);
            TX_IF_RELEASE_ATOM(prefixAtom);
         }

        if (resultNsID == kNameSpaceID_Unknown) {
            String err("error processing xsl:element, can't resolve prefix on'");
            err.append(name);
            err.append("'.");
            aPs->receiveError(err, NS_ERROR_FAILURE);
            // XXX We should processChildren without creating attributes or
            //     namespace nodes.
            TX_RELEASE_ATOM(localName);
            return;
        }

        startElement(aPs, name, resultNsID);
        processAttributeSets(actionElement, aNode, aPs);
        processChildren(aNode, actionElement, aPs);
        NS_ASSERTION(mResultHandler, "mResultHandler must not be NULL!");
        mResultHandler->endElement(name, resultNsID);
    }
    // xsl:for-each
    else if (localName == txXSLTAtoms::forEach) {
        expr = aPs->getExpr(actionElement, ProcessorState::SelectAttr);
        if (!expr) {
            TX_RELEASE_ATOM(localName);
            return;
        }

        ExprResult* exprResult = expr->evaluate(aPs->getEvalContext());
        if (!exprResult) {
            TX_RELEASE_ATOM(localName);
            return;
        }

        if (exprResult->getResultType() == ExprResult::NODESET) {
            NodeSet* nodeSet = (NodeSet*)exprResult;
            txNodeSetContext evalContext(nodeSet, aPs);
            txIEvalContext* priorEC =
                aPs->setEvalContext(&evalContext);

            // Look for xsl:sort elements
            txNodeSorter sorter(aPs);
            Node* child = actionElement->getFirstChild();
            while (child) {
                unsigned short nodeType = child->getNodeType();
                if (nodeType == Node::ELEMENT_NODE) {
                    txAtom* childLocalName;
                    child->getLocalName(&childLocalName);
                    if (child->getNamespaceID() != kNameSpaceID_XSLT ||
                        childLocalName != txXSLTAtoms::sort) {
                        // xsl:sort must occur first
                        TX_IF_RELEASE_ATOM(childLocalName);
                        break;
                    }
                    sorter.addSortElement((Element*)child, aNode);
                    TX_RELEASE_ATOM(childLocalName);
                }
                else if ((nodeType == Node::TEXT_NODE ||
                          nodeType == Node::CDATA_SECTION_NODE) &&
                         !XMLUtils::isWhitespace(child->getNodeValue())) {
                    break;
                }

                child = child->getNextSibling();
            }
            sorter.sortNodeSet(nodeSet);

            // Set current template to null
            ProcessorState::TemplateRule *oldTemplate;
            oldTemplate = aPs->getCurrentTemplateRule();
            aPs->setCurrentTemplateRule(0);

            while (evalContext.hasNext()) {
                evalContext.next();
                Node* currNode = evalContext.getContextNode();
                processChildren(currNode, actionElement, aPs);
            }

            aPs->setCurrentTemplateRule(oldTemplate);
            aPs->setEvalContext(priorEC);
        }
        else {
            String err("error processing for-each");
            aPs->receiveError(err, NS_ERROR_FAILURE);
        }
        //-- clean up exprResult
        delete exprResult;
    }
    // xsl:if
    else if (localName == txXSLTAtoms::_if) {
        expr = aPs->getExpr(actionElement, ProcessorState::TestAttr);
        if (!expr) {
            TX_RELEASE_ATOM(localName);
            return;
        }

        ExprResult* exprResult = expr->evaluate(aPs->getEvalContext());
        if (!exprResult) {
            TX_RELEASE_ATOM(localName);
            return;
        }

        if ( exprResult->booleanValue() ) {
            processChildren(aNode, actionElement, aPs);
        }
        delete exprResult;
    }
    // xsl:message
    else if (localName == txXSLTAtoms::message) {
        String message;
        processChildrenAsValue(aNode, actionElement, aPs, MB_FALSE, message);
        // We should add a MessageObserver class
#ifdef TX_EXE
        cout << "xsl:message - "<< message << endl;
#else
        nsresult rv;
        nsCOMPtr<nsIConsoleService> consoleSvc = 
          do_GetService("@mozilla.org/consoleservice;1", &rv);
        NS_ASSERTION(NS_SUCCEEDED(rv), "xsl:message couldn't get console service");
        if (consoleSvc) {
            nsAutoString logString(NS_LITERAL_STRING("xsl:message - "));
            logString.Append(message.getConstNSString());
            rv = consoleSvc->LogStringMessage(logString.get());
            NS_ASSERTION(NS_SUCCEEDED(rv), "xsl:message couldn't log");
        }
#endif
    }
    // xsl:number
    else if (localName == txXSLTAtoms::number) {
        String result;
        Numbering::doNumbering(actionElement, result, aPs);
        NS_ASSERTION(mResultHandler, "mResultHandler must not be NULL!");
        mResultHandler->characters(result);
    }
    // xsl:param
    else if (localName == txXSLTAtoms::param) {
        // Ignore in this loop (already processed)
    }
    // xsl:processing-instruction
    else if (localName == txXSLTAtoms::processingInstruction) {
        String nameAttr;
        if (!actionElement->getAttr(txXSLTAtoms::name,
                                    kNameSpaceID_None, nameAttr)) {
            String err("missing required name attribute for"
                       " xsl:processing-instruction");
            aPs->receiveError(err, NS_ERROR_FAILURE);
            TX_RELEASE_ATOM(localName);
            return;
        }

        // Process name as an AttributeValueTemplate
        String name;
        aPs->processAttrValueTemplate(nameAttr, actionElement, name);

        // Check name validity (must be valid NCName and a PITarget)
        // XXX Need to check for NCName and PITarget
        if (!XMLUtils::isValidQName(name)) {
            String err("error processing xsl:processing-instruction, '");
            err.append(name);
            err.append("' is not a valid QName.");
            aPs->receiveError(err, NS_ERROR_FAILURE);
        }

        // Compute value
        String value;
        processChildrenAsValue(aNode, actionElement, aPs, MB_TRUE, value);
        XMLUtils::normalizePIValue(value);
        NS_ASSERTION(mResultHandler, "mResultHandler must not be NULL!");
        mResultHandler->processingInstruction(name, value);
    }
    // xsl:sort
    else if (localName == txXSLTAtoms::sort) {
        // Ignore in this loop
    }
    // xsl:text
    else if (localName == txXSLTAtoms::text) {
        String data;
        XMLDOMUtils::getNodeValue(actionElement, data);

        NS_ASSERTION(mResultHandler, "mResultHandler must not be NULL!");
#ifdef TX_EXE
        String aValue;
        if ((mResultHandler == mOutputHandler) &&
            actionElement->getAttr(txXSLTAtoms::disableOutputEscaping,
                                   kNameSpaceID_None, aValue) &&
            aValue.isEqual(YES_VALUE))
            mOutputHandler->charactersNoOutputEscaping(data);
        else
            mResultHandler->characters(data);
#else
        mResultHandler->characters(data);
#endif
    }
    // xsl:value-of
    else if (localName == txXSLTAtoms::valueOf) {
        expr = aPs->getExpr(actionElement, ProcessorState::SelectAttr);
        if (!expr) {
            TX_RELEASE_ATOM(localName);
            return;
        }

        ExprResult* exprResult = expr->evaluate(aPs->getEvalContext());
        String value;
        if (!exprResult) {
            String err("null ExprResult");
            aPs->receiveError(err, NS_ERROR_FAILURE);
            TX_RELEASE_ATOM(localName);
            return;
        }
        exprResult->stringValue(value);

        NS_ASSERTION(mResultHandler, "mResultHandler must not be NULL!");
#ifdef TX_EXE
        String aValue;
        if ((mResultHandler == mOutputHandler) &&
            actionElement->getAttr(txXSLTAtoms::disableOutputEscaping,
                                   kNameSpaceID_None, aValue) &&
            aValue.isEqual(YES_VALUE))
            mOutputHandler->charactersNoOutputEscaping(value);
        else
            mResultHandler->characters(value);
#else
        mResultHandler->characters(value);
#endif
        delete exprResult;
    }
    // xsl:variable
    else if (localName == txXSLTAtoms::variable) {
        String name;
        if (!actionElement->getAttr(txXSLTAtoms::name,
                                    kNameSpaceID_None, name)) {
            String err("missing required name attribute for xsl:variable");
            aPs->receiveError(err, NS_ERROR_FAILURE);
            TX_IF_RELEASE_ATOM(localName);
            return;
        }
        ExprResult* exprResult = processVariable(aNode, actionElement, aPs);
        bindVariable(name, exprResult, MB_FALSE, aPs);
    }
    TX_IF_RELEASE_ATOM(localName);
}

/**
 * Processes the attribute sets specified in the use-attribute-sets attribute
 * of the element specified in aElement
**/
void XSLTProcessor::processAttributeSets(Element* aElement, Node* aNode, ProcessorState* aPs)
{
    String names;
    PRInt32 namespaceID;
    if (aElement->getNamespaceID() == kNameSpaceID_XSLT)
        namespaceID = kNameSpaceID_None;
    else
        namespaceID = kNameSpaceID_XSLT;
    if (!aElement->getAttr(txXSLTAtoms::useAttributeSets, namespaceID, names) || names.isEmpty())
        return;

    // Split names
    txTokenizer tokenizer(names);
    String name;
    while (tokenizer.hasMoreTokens()) {
        tokenizer.nextToken(name);
        StackIterator *attributeSets = mAttributeSetStack.iterator();
        NS_ASSERTION(attributeSets, "Out of memory");
        if (!attributeSets)
            return;
        while (attributeSets->hasNext()) {
            String* test = (String*)attributeSets->next();
            if (test->isEqual(name))
                return;
        }
        delete attributeSets;

        NodeSet* attSet = aPs->getAttributeSet(name);
        if (attSet) {
            int i;
            //-- issue: we still need to handle the following fix cleaner, since
            //-- attribute sets are merged, a different parent could exist
            //-- for different xsl:attribute nodes. I will probably create
            //-- an AttributeSet object, which will handle this case better. - Keith V.
            if (attSet->size() > 0) {
                mAttributeSetStack.push(&name);
                Element* parent = (Element*) attSet->get(0)->getXPathParent();
                processAttributeSets(parent, aNode, aPs);
                mAttributeSetStack.pop();
            }
            for (i = 0; i < attSet->size(); i++)
                processAction(aNode, attSet->get(i), aPs);
            delete attSet;
        }
    }
} //-- processAttributeSets

/**
 * Processes the xsl:with-param child elements of the given xsl action.
 * A VariableBinding is created for each actual parameter, and
 * added to the result NamedMap. At this point, we do not care
 * whether the actual parameter matches a formal parameter of the template
 * or not.
 * @param xslAction the action node that takes parameters (xsl:call-template
 *   or xsl:apply-templates
 * @param context the current context node
 * @ps the current ProcessorState
 * @return a NamedMap of variable bindings
**/
NamedMap* XSLTProcessor::processParameters(Element* xslAction, Node* context, ProcessorState* ps)
{
    NamedMap* params = new NamedMap();

    if (!xslAction || !params)
      return params;

    params->setObjectDeletion(MB_TRUE);

    //-- handle xsl:with-param elements
    Node* tmpNode = xslAction->getFirstChild();
    while (tmpNode) {
        if (tmpNode->getNodeType() == Node::ELEMENT_NODE &&
            tmpNode->getNamespaceID() == kNameSpaceID_XSLT) {
            txAtom* localName;
            tmpNode->getLocalName(&localName);
            if (localName != txXSLTAtoms::withParam) {
                TX_IF_RELEASE_ATOM(localName);
                tmpNode = tmpNode->getNextSibling();
                continue;
            }

            Element* action = (Element*)tmpNode;
            String name;
            if (!action->getAttr(txXSLTAtoms::name,
                                 kNameSpaceID_None, name)) {
                String err("missing required name attribute for xsl:with-param");
                ps->receiveError(err, NS_ERROR_FAILURE);
            }
            else {
                ExprResult* exprResult = processVariable(context, action, ps);
                if (params->get(name)) {
                    //-- error cannot rebind parameters
                    String err("value for parameter '");
                    err.append(name);
                    err.append("' specified more than once.");
                    ps->receiveError(err, NS_ERROR_FAILURE);
                }
                else {
                    VariableBinding* binding = new VariableBinding(name, exprResult);
                    params->put((const String&)name, binding);
                }
            }
            TX_RELEASE_ATOM(localName);
        }
        tmpNode = tmpNode->getNextSibling();
    }
    return params;
} //-- processParameters

/**
 * Processes the children of the specified element using the given context node
 * and ProcessorState
 * @param node the context node
 * @param xslElement the template to be processed. Must be != NULL
 * @param ps the current ProcessorState
**/
void XSLTProcessor::processChildren(Node* node, Element* xslElement, ProcessorState* ps) {

    NS_ASSERTION(xslElement,"xslElement is NULL in call to XSLTProcessor::processChildren!");

    Stack* bindings = ps->getVariableSetStack();
    NamedMap localBindings;
    localBindings.setObjectDeletion(MB_TRUE);
    bindings->push(&localBindings);
    Node* child = xslElement->getFirstChild();
    while (child) {
        processAction(node, child, ps);
        child = child->getNextSibling();
    }
    bindings->pop();
} //-- processChildren

void
XSLTProcessor::processChildrenAsValue(Node* aNode,
                                      Element* aElement,
                                      ProcessorState* aPs,
                                      MBool aOnlyText,
                                      String& aValue)
{
    txXMLEventHandler* previousHandler = mResultHandler;
    txTextHandler valueHandler(aValue, aOnlyText);
    mResultHandler = &valueHandler;
    processChildren(aNode, aElement, aPs);
    mResultHandler = previousHandler;
}

/**
 * Processes the specified template using the given context, ProcessorState, and actual
 * parameters.
 * @param xslTemplate the template to be processed
 * @ps the current ProcessorState
 * @param params a NamedMap of variable bindings that contain the actual parameters for
 *   the template. Parameters that do not match a formal parameter of the template (i.e.
 *   there is no corresponding xsl:param in the template definition) will be discarded.
**/
void XSLTProcessor::processTemplate(Node* node, Node* xslTemplate, ProcessorState* ps, NamedMap* params) {

    NS_ASSERTION(xslTemplate, "xslTemplate is NULL in call to XSLTProcessor::processTemplate!");

    Stack* bindings = ps->getVariableSetStack();
    NamedMap localBindings;
    localBindings.setObjectDeletion(MB_TRUE);
    bindings->push(&localBindings);
    processTemplateParams(xslTemplate, node, ps, params);
    Node* tmp = xslTemplate->getFirstChild();
    while (tmp) {
        processAction(node,tmp,ps);
        tmp = tmp->getNextSibling();
    }
    
    if (params) {
        StringList* keys = params->keys();
        if (keys) {
            StringListIterator keyIter(keys);
            String* key;
            while((key = keyIter.next())) {
                VariableBinding *var, *param;
                var = (VariableBinding*)localBindings.get(*key);
                param = (VariableBinding*)params->get(*key);
                if (var && var->getValue() == param->getValue()) {
                    // Don't delete the contained ExprResult since it's
                    // not ours
                    var->setValue(0);
                }
            }
        }
        else {
            // out of memory so we can't get the keys
            // don't delete any variables since it's better we leak then
            // crash
            localBindings.setObjectDeletion(MB_FALSE);
        }
        delete keys;
    }
    
    bindings->pop();
} //-- processTemplate

void XSLTProcessor::processMatchedTemplate(Node* aXslTemplate,
                                           Node* aNode,
                                           NamedMap* aParams,
                                           const String& aMode,
                                           ProcessorState::ImportFrame* aFrame,
                                           ProcessorState* aPs)
{
    if (aXslTemplate) {
        ProcessorState::TemplateRule *oldTemplate, newTemplate;
        oldTemplate = aPs->getCurrentTemplateRule();
        newTemplate.mFrame = aFrame;
        newTemplate.mMode = &aMode;
        newTemplate.mParams = aParams;
        aPs->setCurrentTemplateRule(&newTemplate);

        processTemplate(aNode, aXslTemplate, aPs, aParams);

        aPs->setCurrentTemplateRule(oldTemplate);
    }
    else {
        processDefaultTemplate(aNode, aPs, aMode);
    }
}

/**
 * Invokes the default template for the specified node
 * @param node  context node
 * @param ps    current ProcessorState
 * @param mode  template mode
**/
void XSLTProcessor::processDefaultTemplate(Node* node,
                                           ProcessorState* ps,
                                           const String& mode)
{
    NS_ASSERTION(node, "context node is NULL in call to XSLTProcessor::processTemplate!");

    switch(node->getNodeType())
    {
        case Node::ELEMENT_NODE :
        case Node::DOCUMENT_NODE :
        {
            if (!mNodeExpr)
                break;

            ExprResult* exprResult = mNodeExpr->evaluate(ps->getEvalContext());
            if (!exprResult ||
                exprResult->getResultType() != ExprResult::NODESET) {
                String err("None-nodeset returned while processing default template");
                ps->receiveError(err, NS_ERROR_FAILURE);
                delete exprResult;
                return;
            }

            NodeSet* nodeSet = (NodeSet*)exprResult;
            txNodeSetContext evalContext(nodeSet, ps);
            txIEvalContext* priorEC = ps->setEvalContext(&evalContext);

            while (evalContext.hasNext()) {
                evalContext.next();
                Node* currNode = evalContext.getContextNode();

                ProcessorState::ImportFrame *frame;
                Node* xslTemplate = ps->findTemplate(currNode, mode, &frame);
                processMatchedTemplate(xslTemplate, currNode, 0, mode, frame,
                                       ps);
            }
            ps->setEvalContext(priorEC);
            delete exprResult;
            break;
        }
        case Node::ATTRIBUTE_NODE :
        case Node::TEXT_NODE :
        case Node::CDATA_SECTION_NODE :
        {
            NS_ASSERTION(mResultHandler, "mResultHandler must not be NULL!");
            mResultHandler->characters(node->getNodeValue());
            break;
        }
        default:
            // on all other nodetypes (including namespace nodes)
            // we do nothing
            break;
    }
} //-- processDefaultTemplate

/**
 * Builds the initial bindings for the template. Formal parameters (xsl:param) that
 *   have a corresponding binding in actualParams are bound to the actual parameter value,
 *   otherwise to their default value. Actual parameters that do not match any formal
 *   parameter are discarded.
 * @param xslTemplate the template node
 * @param context the current context node
 * @param ps the current ProcessorState
 * @param actualParams a NamedMap of variable bindings that contains the actual parameters
**/
void XSLTProcessor::processTemplateParams
    (Node* xslTemplate, Node* context, ProcessorState* ps, NamedMap* actualParams)
{

    if ( xslTemplate ) {
        Node* tmpNode = xslTemplate->getFirstChild();
        //-- handle params
        while (tmpNode) {
            unsigned short nodeType = tmpNode->getNodeType();
            if (nodeType == Node::ELEMENT_NODE) {
                txAtom* localName;
                tmpNode->getLocalName(&localName);
                if (tmpNode->getNamespaceID() != kNameSpaceID_XSLT ||
                    localName != txXSLTAtoms::param) {
                    TX_IF_RELEASE_ATOM(localName);
                    break;
                }
                TX_RELEASE_ATOM(localName);

                Element* action = (Element*)tmpNode;
                String name;
                if (!action->getAttr(txXSLTAtoms::name,
                                     kNameSpaceID_None, name)) {
                    String err("missing required name attribute for xsl:param");
                    ps->receiveError(err, NS_ERROR_FAILURE);
                }
                else {
                    VariableBinding* binding = 0;
                    if (actualParams) {
                        binding = (VariableBinding*) actualParams->get((const String&)name);
                    }
                    if (binding) {
                        // the formal parameter has a corresponding actual parameter, use it
                        ExprResult* exprResult = binding->getValue();
                        bindVariable(name, exprResult, MB_FALSE, ps);
                    }
                    else {
                        // no actual param, use default
                        ExprResult* exprResult = processVariable(context, action, ps);
                        bindVariable(name, exprResult, MB_FALSE, ps);
                    }
                }
            }
            else if (nodeType == Node::TEXT_NODE ||
                     nodeType == Node::CDATA_SECTION_NODE) {
                if (!XMLUtils::isWhitespace(tmpNode->getNodeValue()))
                    break;
            }
            tmpNode = tmpNode->getNextSibling();
        }
    }
} //-- processTemplateParams


/**
 *  processes the xslVariable parameter as an xsl:variable using the given context,
 *  and ProcessorState.
 *  If the xslTemplate contains an "expr" attribute, the attribute is evaluated
 *  as an Expression and the ExprResult is returned. Otherwise the xslVariable is
 *  is processed as a template, and it's result is converted into an ExprResult
 *  @return an ExprResult
**/
ExprResult* XSLTProcessor::processVariable
        (Node* node, Element* xslVariable, ProcessorState* ps)
{

    if ( !xslVariable ) {
        return new StringResult("unable to process variable");
    }

    //-- check for select attribute
    if (xslVariable->hasAttr(txXSLTAtoms::select, kNameSpaceID_None)) {
        Expr* expr = ps->getExpr(xslVariable, ProcessorState::SelectAttr);
        if (!expr)
            return new StringResult("unable to process variable");
        return expr->evaluate(ps->getEvalContext());
    }
    else if (xslVariable->hasChildNodes()) {
        txResultTreeFragment* rtf = new txResultTreeFragment();
        if (!rtf)
            // XXX ErrorReport: Out of memory
            return 0;
        txXMLEventHandler* previousHandler = mResultHandler;
        txRtfHandler rtfHandler(ps->getResultDocument(), rtf);
        mResultHandler = &rtfHandler;
        processChildren(node, xslVariable, ps);
        //NS_ASSERTION(previousHandler, "Setting mResultHandler to NULL!");
        mResultHandler = previousHandler;
        return rtf;
    }
    else {
        return new StringResult("");
    }
} //-- processVariable

void XSLTProcessor::startTransform(Node* aNode, ProcessorState* aPs)
{
    mHaveDocumentElement = MB_FALSE;
    mOutputHandler->startDocument();
    process(aNode, NULL_STRING, aPs);
    mOutputHandler->endDocument();
}

MBool XSLTProcessor::initializeHandlers(ProcessorState* aPs)
{
    txListIterator frameIter(aPs->getImportFrames());
    ProcessorState::ImportFrame* frame;
    txOutputFormat* outputFormat = aPs->getOutputFormat();
    while ((frame = (ProcessorState::ImportFrame*)frameIter.next()))
        outputFormat->merge(frame->mOutputFormat);

    delete mOutputHandler;
#ifdef TX_EXE
    switch (outputFormat->mMethod) {
        case eMethodNotSet:
        case eXMLOutput:
        {
            mOutputHandler = new txXMLOutput();
            break;
        }
        case eHTMLOutput:
        {
            mOutputHandler = new txHTMLOutput();
            break;
        }
        case eTextOutput:
        {
            mOutputHandler = new txTextOutput();
            break;
        }
    }
#else
    switch (outputFormat->mMethod) {
        case eMethodNotSet:
        case eXMLOutput:
        case eHTMLOutput:
        {
            mOutputHandler = new txMozillaXMLOutput();
            break;
        }
        case eTextOutput:
        {
            mOutputHandler = new txMozillaTextOutput();
            break;
        }
    }
#endif

    mResultHandler = mOutputHandler;
    if (!mOutputHandler)
        return MB_FALSE;
    mOutputHandler->setOutputFormat(outputFormat);
    return MB_TRUE;
}

/**
 * Performs the xsl:copy action as specified in the XSLT specification
 */
void XSLTProcessor::xslCopy(Node* aNode, Element* aAction, ProcessorState* aPs)
{
    if (!aNode)
        return;

    switch (aNode->getNodeType()) {
        case Node::DOCUMENT_NODE:
        {
            // Just process children
            processChildren(aNode, aAction, aPs);
            break;
        }
        case Node::ELEMENT_NODE:
        {
            Element* element = (Element*)aNode;
            String nodeName = element->getNodeName();
            PRInt32 nsID = element->getNamespaceID();

            startElement(aPs, nodeName, nsID);
            // XXX copy namespace attributes once we have them
            processAttributeSets(aAction, aNode, aPs);
            processChildren(aNode, aAction, aPs);
            NS_ASSERTION(mResultHandler, "mResultHandler must not be NULL!");
            mResultHandler->endElement(nodeName, nsID);
            break;
        }
        default:
        {
            copyNode(aNode, aPs);
        }
    }
}

/**
 * Performs the xsl:copy-of action as specified in the XSLT specification
 */
void XSLTProcessor::xslCopyOf(ExprResult* aExprResult, ProcessorState* aPs)
{
    if (!aExprResult)
        return;

    switch (aExprResult->getResultType()) {
        case ExprResult::NODESET:
        {
            NodeSet* nodes = (NodeSet*)aExprResult;
            int i;
            for (i = 0; i < nodes->size(); i++) {
                Node* node = nodes->get(i);
                copyNode(node, aPs);
            }
            break;
        }
        default:
        {
            String value;
            aExprResult->stringValue(value);
            NS_ASSERTION(mResultHandler, "mResultHandler must not be NULL!");
            mResultHandler->characters(value);
        }
    }
}

/**
 * Copy a node. For document nodes and document fragments, copy the children.
 */
void XSLTProcessor::copyNode(Node* aNode, ProcessorState* aPs)
{
    if (!aNode)
        return;

    switch (aNode->getNodeType()) {
        case Node::ATTRIBUTE_NODE:
        {
            NS_ASSERTION(mResultHandler, "mResultHandler must not be NULL!");
            mResultHandler->attribute(aNode->getNodeName(), aNode->getNamespaceID(),
                                      aNode->getNodeValue());
            break;
        }
        case Node::COMMENT_NODE:
        {
            NS_ASSERTION(mResultHandler, "mResultHandler must not be NULL!");
            mResultHandler->comment(((Comment*)aNode)->getData());
            break;
        }
        case Node::DOCUMENT_NODE:
        case Node::DOCUMENT_FRAGMENT_NODE:
        {
            // Copy children
            Node* child = aNode->getFirstChild();
            while (child) {
                copyNode(child, aPs);
                child = child->getNextSibling();
            }
            break;
        }
        case Node::ELEMENT_NODE:
        {
            Element* element = (Element*)aNode;
            const String& name = element->getNodeName();
            PRInt32 nsID = element->getNamespaceID();
            startElement(aPs, name, nsID);

            // Copy attributes
            NamedNodeMap* attList = element->getAttributes();
            if (attList) {
                PRUint32 i = 0;
                for (i = 0; i < attList->getLength(); i++) {
                    Attr* attr = (Attr*)attList->item(i);
                    NS_ASSERTION(mResultHandler, "mResultHandler must not be NULL!");
                    mResultHandler->attribute(attr->getName(), attr->getNamespaceID(),
                                              attr->getValue());
                }
            }

            // Copy children
            Node* child = element->getFirstChild();
            while (child) {
                copyNode(child, aPs);
                child = child->getNextSibling();
            }

            NS_ASSERTION(mResultHandler, "mResultHandler must not be NULL!");
            mResultHandler->endElement(name, nsID);
            break;
        }
        case Node::PROCESSING_INSTRUCTION_NODE:
        {
            ProcessingInstruction* pi = (ProcessingInstruction*)aNode;
            NS_ASSERTION(mResultHandler, "mResultHandler must not be NULL!");
            mResultHandler->processingInstruction(pi->getTarget(), pi->getData());
            break;
        }
        case Node::TEXT_NODE:
        case Node::CDATA_SECTION_NODE:
        {
            NS_ASSERTION(mResultHandler, "mResultHandler must not be NULL!");
            mResultHandler->characters(((CharacterData*)aNode)->getData());
            break;
        }
    }
}

void
XSLTProcessor::startElement(ProcessorState* aPs, const String& aName, const PRInt32 aNsID)
{
    if (!mHaveDocumentElement && (mResultHandler == mOutputHandler)) {
        txOutputFormat* format = aPs->getOutputFormat();
        // XXX Should check for whitespace-only sibling text nodes
        if ((format->mMethod == eMethodNotSet)
            && (aNsID == kNameSpaceID_None)
            && aName.isEqualIgnoreCase("html")) {
            // Switch to html output mode according to the XSLT spec.
            format->mMethod = eHTMLOutput;
#ifdef TX_EXE
            ostream* out;
            mOutputHandler->getOutputStream(&out);
            delete mOutputHandler;
            mOutputHandler = new txHTMLOutput();
            NS_ASSERTION(mOutputHandler, "Setting mResultHandler to NULL!");
            mOutputHandler->setOutputStream(out);
            mResultHandler = mOutputHandler;
#endif
            mOutputHandler->setOutputFormat(format);
        }
        mHaveDocumentElement = MB_TRUE;
    }
    NS_ASSERTION(mResultHandler, "mResultHandler must not be NULL!");
    mResultHandler->startElement(aName, aNsID);
}

#ifndef TX_EXE

// XXX START
// XXX Mozilla module only code. This should move to txMozillaXSLTProcessor
// XXX

NS_IMETHODIMP
XSLTProcessor::TransformDocument(nsIDOMNode* aSourceDOM,
                                 nsIDOMNode* aStyleDOM,
                                 nsIDOMDocument* aOutputDoc,
                                 nsIObserver* aObserver)
{
    // We need source and result documents but no stylesheet.
    NS_ENSURE_ARG(aSourceDOM);
    NS_ENSURE_ARG(aOutputDoc);

    // Create wrapper for the source document.
    nsCOMPtr<nsIDOMDocument> sourceDOMDocument;
    aSourceDOM->GetOwnerDocument(getter_AddRefs(sourceDOMDocument));
    if (!sourceDOMDocument)
        sourceDOMDocument = do_QueryInterface(aSourceDOM);
    NS_ENSURE_TRUE(sourceDOMDocument, NS_ERROR_FAILURE);
    Document sourceDocument(sourceDOMDocument);
    Node* sourceNode = sourceDocument.createWrapper(aSourceDOM);
    NS_ENSURE_TRUE(sourceNode, NS_ERROR_FAILURE);

    // Create wrapper for the style document.
    nsCOMPtr<nsIDOMDocument> styleDOMDocument;
    aStyleDOM->GetOwnerDocument(getter_AddRefs(styleDOMDocument));
    if (!styleDOMDocument)
        styleDOMDocument = do_QueryInterface(aStyleDOM);
    Document xslDocument(styleDOMDocument);

    // Create wrapper for the output document.
    nsCOMPtr<nsIDocument> document = do_QueryInterface(aOutputDoc);
    NS_ENSURE_TRUE(document, NS_ERROR_FAILURE);
    Document resultDocument(aOutputDoc);

    // Reset the output document.
    // Create a temporary channel to get nsIDocument->Reset to
    // do the right thing.
    nsCOMPtr<nsILoadGroup> loadGroup;
    nsCOMPtr<nsIChannel> channel;
    nsCOMPtr<nsIURI> docURL;

    document->GetDocumentURL(getter_AddRefs(docURL));
    NS_ASSERTION(docURL, "No document URL");
    document->GetDocumentLoadGroup(getter_AddRefs(loadGroup));
    if (!loadGroup) {
        nsCOMPtr<nsIDocument> source = do_QueryInterface(sourceDOMDocument);
        if (source) {
            source->GetDocumentLoadGroup(getter_AddRefs(loadGroup));
        }
    }

    nsresult rv = NS_NewChannel(getter_AddRefs(channel), docURL,
                                nsnull, loadGroup);
    NS_ENSURE_SUCCESS(rv, rv);

    // Start of hack for keeping the scrollbars on an existing document.
    // Based on similar hack that jst wrote for document.open().
    // See bugs 78070 and 55334.
    nsCOMPtr<nsIContent> root;
    document->GetRootContent(getter_AddRefs(root));
    if (root) {
        document->SetRootContent(nsnull);
    }

    // Call Reset(), this will now do the full reset, except removing
    // the root from the document, doing that confuses the scrollbar
    // code in mozilla since the document in the root element and all
    // the anonymous content (i.e. scrollbar elements) is set to
    // null.
    rv = document->Reset(channel, loadGroup);
    NS_ENSURE_SUCCESS(rv, rv);

    if (root) {
        // Tear down the frames for the root element.
        document->ContentRemoved(nsnull, root, 0);
    }
    // End of hack for keeping the scrollbars on an existing document.

    // Start of block to ensure the destruction of the ProcessorState
    // before the destruction of the documents.
    {
        // Create a new ProcessorState
        ProcessorState ps(&sourceDocument, &xslDocument, &resultDocument);

        // XXX Need to add error observers

        // Set current txIEvalContext
        txSingleNodeContext evalContext(&sourceDocument, &ps);
        ps.setEvalContext(&evalContext);

        // Index templates and process top level xsl elements
        ListIterator importFrame(ps.getImportFrames());
        importFrame.addAfter(new ProcessorState::ImportFrame(0));
        if (!importFrame.next())
            return NS_ERROR_OUT_OF_MEMORY;
        nsCOMPtr<nsIDOMDocument> styleDoc = do_QueryInterface(aStyleDOM);
        if (styleDoc) {
            processStylesheet(&sourceDocument, &xslDocument, &importFrame,
                              &ps);
        }
        else {
            nsCOMPtr<nsIDOMElement> styleElem = do_QueryInterface(aStyleDOM);
            NS_ENSURE_TRUE(styleElem, NS_ERROR_FAILURE);
            Element* element = xslDocument.createElement(styleElem);
            NS_ENSURE_TRUE(element, NS_ERROR_OUT_OF_MEMORY);
            processTopLevel(&sourceDocument, element, &importFrame, &ps);
        }

        initializeHandlers(&ps);

        if (mOutputHandler) {
            mOutputHandler->setOutputDocument(aOutputDoc);
        }

        // Get the script loader of the result document.
        if (aObserver) {
            document->GetScriptLoader(getter_AddRefs(mScriptLoader));
            if (mScriptLoader) {
                mScriptLoader->AddObserver(this);
            }
        }

        // Process root of XML source document
        startTransform(sourceNode, &ps);
    }
    // End of block to ensure the destruction of the ProcessorState
    // before the destruction of the documents.

    mOutputHandler->getRootContent(getter_AddRefs(root));
    if (root) {
        document->ContentInserted(nsnull, root, 0);
    }

    mObserver = aObserver;
    SignalTransformEnd();

    return NS_OK;
}

NS_IMETHODIMP
XSLTProcessor::ScriptAvailable(nsresult aResult, 
                               nsIDOMHTMLScriptElement *aElement, 
                               PRBool aIsInline,
                               PRBool aWasPending,
                               nsIURI *aURI, 
                               PRInt32 aLineNo,
                               const nsAString& aScript)
{
    if (NS_FAILED(aResult) && mOutputHandler) {
        mOutputHandler->removeScriptElement(aElement);
        SignalTransformEnd();
    }

    return NS_OK;
}

NS_IMETHODIMP 
XSLTProcessor::ScriptEvaluated(nsresult aResult, 
                               nsIDOMHTMLScriptElement *aElement,
                               PRBool aIsInline,
                               PRBool aWasPending)
{
    if (mOutputHandler) {
        mOutputHandler->removeScriptElement(aElement);
        SignalTransformEnd();
    }

    return NS_OK;
}

void
XSLTProcessor::SignalTransformEnd()
{
    if (!mObserver)
        return;

    if (!mOutputHandler || !mOutputHandler->isDone())
        return;

    if (mScriptLoader) {
        mScriptLoader->RemoveObserver(this);
        mScriptLoader = nsnull;
    }

    nsresult rv;
    nsCOMPtr<nsIObserverService> anObserverService = do_GetService("@mozilla.org/observer-service;1", &rv);
    if (NS_SUCCEEDED(rv)) {
        nsCOMPtr<nsIContent> rootContent;
        mOutputHandler->getRootContent(getter_AddRefs(rootContent));
        anObserverService->AddObserver(mObserver, "xslt-done", PR_TRUE);
        anObserverService->NotifyObservers(rootContent, "xslt-done", nsnull);
    }
    mObserver = nsnull;
}

// XXX
// XXX Mozilla module only code. This should move to txMozillaXSLTProcessor
// XXX END

#endif
