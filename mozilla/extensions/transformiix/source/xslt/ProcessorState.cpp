/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- 
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
 * (C) 1999-2000 Keith Visco. All Rights Reserved.
 *
 * Contributor(s):
 * Keith Visco, kvisco@ziplink.net
 *    -- original author.
 *
 * Olivier Gerardin, ogerardin@vo.lu
 *   -- added code in ::resolveFunctionCall to support the
 *      document() function.
 *
 */

/**
 * Implementation of ProcessorState
 * Much of this code was ported from XSL:P
**/

#include "ProcessorState.h"
#include "XSLTFunctions.h"
#include "FunctionLib.h"
#include "txURIUtils.h"
#include "XMLUtils.h"
#include "XMLDOMUtils.h"
#include "Tokenizer.h"
#include "ExprResult.h"
#include "Names.h"
#include "XMLParser.h"
#include "TxLog.h"
#include "txAtoms.h"
#include "txSingleNodeContext.h"
#include "txVariableMap.h"
#include "XSLTProcessor.h"

/**
 * Creates a new ProcessorState for the given XSL document
**/
ProcessorState::ProcessorState(Document* aSourceDocument,
                               Document* aXslDocument)
    : mXslKeys(MB_TRUE),
      mDecimalFormats(MB_TRUE),
      mEvalContext(0),
      mLocalVariables(0),
      mGlobalVariableValues(MB_TRUE),
      mSourceDocument(aSourceDocument),
      xslDocument(aXslDocument),
      mRTFDocument(0),
      mOutputHandler(0),
      mResultHandler(0),
      mOutputHandlerFactory(0)
{
    NS_ASSERTION(aSourceDocument, "missing source document");
    NS_ASSERTION(aXslDocument, "missing xslt document");

    /* turn object deletion on for some of the Maps (NamedMap) */
    mExprHashes[SelectAttr].setOwnership(Map::eOwnsItems);
    mExprHashes[TestAttr].setOwnership(Map::eOwnsItems);
    mExprHashes[ValueAttr].setOwnership(Map::eOwnsItems);
    mPatternHashes[CountAttr].setOwnership(Map::eOwnsItems);
    mPatternHashes[FromAttr].setOwnership(Map::eOwnsItems);

    // determine xslt properties
    if (mSourceDocument) {
        loadedDocuments.put(mSourceDocument->getBaseURI(), mSourceDocument);
    }
    if (xslDocument) {
        loadedDocuments.put(xslDocument->getBaseURI(), xslDocument);
    }

    // Make sure all loaded documents get deleted
    loadedDocuments.setObjectDeletion(MB_TRUE);
}

/**
 * Destroys this ProcessorState
**/
ProcessorState::~ProcessorState()
{
  // Delete all ImportFrames
  txListIterator iter(&mImportFrames);
  while (iter.hasNext())
      delete (ImportFrame*)iter.next();

  // Make sure that xslDocument and mSourceDocument aren't deleted along with
  // the rest of the documents in the loadedDocuments hash
  if (xslDocument)
      loadedDocuments.remove(xslDocument->getBaseURI());
  if (mSourceDocument)
      loadedDocuments.remove(mSourceDocument->getBaseURI());

  // in module the outputhandler is refcounted
#ifdef TX_EXE
  delete mOutputHandler;
#endif
} //-- ~ProcessorState


/*
 * Adds the given attribute set to the list of available named attribute
 * sets
 * @param aAttributeSet the Element to add as a named attribute set
 * @param aImportFrame  ImportFrame to add the attributeset to
 */
void ProcessorState::addAttributeSet(Element* aAttributeSet,
                                     ImportFrame* aImportFrame)
{
    if (!aAttributeSet)
        return;

    String nameStr;
    txExpandedName name;
    aAttributeSet->getAttr(txXSLTAtoms::name, kNameSpaceID_None, nameStr);
    nsresult rv = name.init(nameStr, aAttributeSet, MB_FALSE);
    if (NS_FAILED(rv)) {
        String err("missing or malformed name for xsl:attribute-set");
        receiveError(err);
        return;
    }
    // Get attribute set, if already exists, then merge
    NodeSet* attSet = (NodeSet*)aImportFrame->mNamedAttributeSets.get(name);
    if (!attSet) {
        attSet = new NodeSet();
        aImportFrame->mNamedAttributeSets.add(name, attSet);
    }

    // Add xsl:attribute elements to attSet
    Node* node = aAttributeSet->getFirstChild();
    while (node) {
        if (node->getNodeType() == Node::ELEMENT_NODE) {
            PRInt32 nsID = node->getNamespaceID();
            if (nsID != kNameSpaceID_XSLT)
                continue;
            txAtom* nodeName;
            if (!node->getLocalName(&nodeName) || !nodeName)
                continue;
            if (nodeName == txXSLTAtoms::attribute)
                attSet->append(node);
            TX_RELEASE_ATOM(nodeName);
        }
        node = node->getNextSibling();
    }

}
/**
 * Registers the given ErrorObserver with this ProcessorState
**/
void ProcessorState::addErrorObserver(ErrorObserver& errorObserver) {
    errorObservers.add(&errorObserver);
} //-- addErrorObserver

/**
 * Adds the given template to the list of templates to process
 * @param xslTemplate  The Element to add as a template
 * @param importFrame  ImportFrame to add the template to
**/
void ProcessorState::addTemplate(Element* aXslTemplate,
                                 ImportFrame* aImportFrame)
{
    NS_ASSERTION(aXslTemplate, "missing template");

    nsresult rv = NS_OK;
    String nameStr;
    if (aXslTemplate->getAttr(txXSLTAtoms::name,
                              kNameSpaceID_None, nameStr)) {
        txExpandedName name;
        rv = name.init(nameStr, aXslTemplate, MB_FALSE);
        if (NS_FAILED(rv)) {
            String err("missing or malformed template name: '");
            err.append(nameStr);
            err.append('\'');
            receiveError(err, NS_ERROR_FAILURE);
            return;
        }

        rv = aImportFrame->mNamedTemplates.add(name, aXslTemplate);
        if (NS_FAILED(rv)) {
            String err("Unable to add template named '");
            err.append(nameStr);
            err.append("'. Does that name already exist?");
            receiveError(err, NS_ERROR_FAILURE);
            return;
        }
    }

    String match;
    if (!aXslTemplate->getAttr(txXSLTAtoms::match, kNameSpaceID_None, match)) {
        // This is no error, see section 6 Named Templates
        return;
    }

    // get the txList for the right mode
    String modeStr;
    txExpandedName mode;
    if (aXslTemplate->getAttr(txXSLTAtoms::mode, kNameSpaceID_None, modeStr)) {
        rv = mode.init(modeStr, aXslTemplate, MB_FALSE);
        if (NS_FAILED(rv)) {
            String err("malformed template-mode name: '");
            err.append(modeStr);
            err.append('\'');
            receiveError(err, NS_ERROR_FAILURE);
            return;
        }
    }
    txList* templates =
        (txList*)aImportFrame->mMatchableTemplates.get(mode);

    if (!templates) {
        templates = new txList;
        if (!templates) {
            NS_ASSERTION(0, "out of memory");
            return;
        }
        rv = aImportFrame->mMatchableTemplates.add(mode, templates);
        if (NS_FAILED(rv)) {
            delete templates;
            return;
        }
    }

    // Check for explicit default priority
    MBool hasPriority;
    double priority;
    String prio;
    if ((hasPriority =
         aXslTemplate->getAttr(txXSLTAtoms::priority, kNameSpaceID_None,
                               prio))) {
        priority = Double::toDouble(prio);
    }

    // Get the pattern
    txPSParseContext context(this, aXslTemplate);
    txPattern* pattern = txPatternParser::createPattern(match, &context, this);
#ifdef TX_PATTERN_DEBUG
    String foo;
    pattern->toString(foo);
#endif

    if (!pattern) {
        return;
    }

    // Add the simple patterns to the list of matchable templates, according
    // to default priority
    txList simpleMatches;
    pattern->getSimplePatterns(simpleMatches);
    txListIterator simples(&simpleMatches);
    while (simples.hasNext()) {
        txPattern* simple = (txPattern*)simples.next();
        if (simple != pattern && pattern) {
            // txUnionPattern, it doesn't own the txLocPathPatterns no more,
            // so delete it. (only once, of course)
            delete pattern;
            pattern = 0;
        }
        if (!hasPriority) {
            priority = simple->getDefaultPriority();
        }
        MatchableTemplate* nt = new MatchableTemplate(aXslTemplate,
                                                      simple,
                                                      priority);
        if (!nt) {
            NS_ASSERTION(0, "out of mem");
            return;
        }
        txListIterator templ(templates);
        MBool isLast = MB_TRUE;
        while (templ.hasNext() && isLast) {
            MatchableTemplate* mt = (MatchableTemplate*)templ.next();
            if (priority < mt->mPriority) {
                continue;
            }
            templ.addBefore(nt);
            isLast = MB_FALSE;
        }
        if (isLast)
            templates->add(nt);
    }
}

/*
 * Adds the given LRE Stylesheet to the list of templates to process
 * @param aStylesheet  The Stylesheet to add as a template
 * @param importFrame  ImportFrame to add the template to
 */
void ProcessorState::addLREStylesheet(Document* aStylesheet,
                                      ImportFrame* aImportFrame)
{
    NS_ASSERTION(aStylesheet, "missing stylesheet");
    
    // get the txList for null mode
    txExpandedName nullMode;
    txList* templates =
        (txList*)aImportFrame->mMatchableTemplates.get(nullMode);

    if (!templates) {
        templates = new txList;
        if (!templates) {
            // XXX ErrorReport: out of memory
            return;
        }
        aImportFrame->mMatchableTemplates.add(nullMode, templates);
    }

    // Add the template to the list of templates
    txPattern* root = new txRootPattern(MB_TRUE);
    MatchableTemplate* nt = 0;
    if (root) 
        nt = new MatchableTemplate(aStylesheet, root, 0.5);
    if (!nt) {
        delete root;
        // XXX ErrorReport: out of memory
        return;
    }
    txListIterator templ(templates);
    MBool isLast = MB_TRUE;
    while (templ.hasNext() && isLast) {
        MatchableTemplate* mt = (MatchableTemplate*)templ.next();
        if (0.5 < mt->mPriority) {
            continue;
        }
        templ.addBefore(nt);
        isLast = MB_FALSE;
    }
    if (isLast)
        templates->add(nt);
}

/*
 * Retrieve the document designated by the URI uri, using baseUri as base URI.
 * Parses it as an XML document, and returns it. If a fragment identifier is
 * supplied, the element with seleced id is returned.
 * The returned document is owned by the ProcessorState
 *
 * @param uri the URI of the document to retrieve
 * @param baseUri the base URI used to resolve the URI if uri is relative
 * @return loaded document or element pointed to by fragment identifier. If
 *         loading or parsing fails NULL will be returned.
 */
Node* ProcessorState::retrieveDocument(const String& uri, const String& baseUri)
{
    String absUrl, frag, docUrl;
    URIUtils::resolveHref(uri, baseUri, absUrl);
    URIUtils::getFragmentIdentifier(absUrl, frag);
    URIUtils::getDocumentURI(absUrl, docUrl);

    // try to get already loaded document
    Document* xmlDoc = (Document*)loadedDocuments.get(docUrl);

    if (!xmlDoc) {
        // open URI
        String errMsg;
        XMLParser xmlParser;

        xmlDoc = xmlParser.getDocumentFromURI(docUrl, xslDocument, errMsg);

        if (!xmlDoc) {
            String err("Couldn't load document '");
            err.append(docUrl);
            err.append("': ");
            err.append(errMsg);
            receiveError(err, NS_ERROR_XSLT_INVALID_URL);
            return NULL;
        }
        // add to list of documents
        loadedDocuments.put(docUrl, xmlDoc);
    }

    // return element with supplied id if supplied
    if (!frag.isEmpty())
        return xmlDoc->getElementById(frag);

    return xmlDoc;
}

/*
 * Return stack of urls of currently entered stylesheets
 */
Stack* ProcessorState::getEnteredStylesheets()
{
    return &enteredStylesheets;
}

/*
 * Return list of import containers
 */
List* ProcessorState::getImportFrames()
{
    return &mImportFrames;
}

/*
 * Find template in specified mode matching the supplied node. Only search
 * templates imported by a specific ImportFrame
 * @param aNode        node to find matching template for
 * @param aMode        mode of the template
 * @param aImportedBy  seach only templates imported by this ImportFrame,
 *                     or null to search all templates
 * @param aImportFrame out-param, is set to the ImportFrame containing
 *                     the found template
 * @return             root-node of found template, null if none is found
 */
Node* ProcessorState::findTemplate(Node* aNode,
                                   const txExpandedName& aMode,
                                   ImportFrame* aImportedBy,
                                   ImportFrame** aImportFrame)
{
    NS_ASSERTION(aImportFrame, "missing ImportFrame pointer");
    NS_ASSERTION(aNode, "missing node");

    if (!aNode)
        return 0;

    Node* matchTemplate = 0;
    ImportFrame* endFrame = 0;
    txListIterator frameIter(&mImportFrames);

    if (aImportedBy) {
        ImportFrame* curr = (ImportFrame*)frameIter.next();
        while (curr != aImportedBy)
               curr = (ImportFrame*)frameIter.next();

        endFrame = aImportedBy->mFirstNotImported;
    }

    ImportFrame* frame;
    while (!matchTemplate &&
           (frame = (ImportFrame*)frameIter.next()) &&
           frame != endFrame) {

        // get templatelist for this mode
        txList* templates;
        templates = (txList*)frame->mMatchableTemplates.get(aMode);

        if (templates) {
            txListIterator templateIter(templates);

            // Find template with highest priority
            MatchableTemplate* templ;
            while (!matchTemplate &&
                   (templ = (MatchableTemplate*)templateIter.next())) {
#ifdef TX_PATTERN_DEBUG
                String foo;
                templ->mMatch->toString(foo);
#endif
                if (templ->mMatch->matches(aNode, this)) {
                    matchTemplate = templ->mTemplate;
                    *aImportFrame = frame;
                }
            }
        }
    }

#ifdef PR_LOGGING
    String mode;
    if (aMode.mLocalName) {
        TX_GET_ATOM_STRING(aMode.mLocalName, mode);
    }
    if (matchTemplate) {
        String matchAttr;
        // matchTemplate can be a document (see addLREStylesheet)
        unsigned short nodeType = matchTemplate->getNodeType();
        if (nodeType == Node::ELEMENT_NODE) {
            ((Element*)matchTemplate)->getAttr(txXSLTAtoms::match,
                                               kNameSpaceID_None,
                                               matchAttr);
        }
        PR_LOG(txLog::xslt, PR_LOG_DEBUG,
               ("MatchTemplate, Pattern %s, Mode %s, Stylesheet %s, " \
                "Node %s\n",
                NS_LossyConvertUCS2toASCII(matchAttr).get(),
                NS_LossyConvertUCS2toASCII(mode).get(),
                NS_LossyConvertUCS2toASCII(matchTemplate->getBaseURI()).get(),
                NS_LossyConvertUCS2toASCII(aNode->getNodeName()).get()));
    }
    else {
        PR_LOG(txLog::xslt, PR_LOG_DEBUG,
               ("No match, Node %s, Mode %s\n", 
                NS_LossyConvertUCS2toASCII(aNode->getNodeName()).get(),
                NS_LossyConvertUCS2toASCII(mode).get()));
    }
#endif
    return matchTemplate;
}

/*
 * Gets current template rule
 */
ProcessorState::TemplateRule* ProcessorState::getCurrentTemplateRule()
{
    return mCurrentTemplateRule;
}

/*
 * Sets current template rule
 */
void ProcessorState::setCurrentTemplateRule(TemplateRule* aTemplateRule)
{
    mCurrentTemplateRule = aTemplateRule;
}

/**
 * Returns the AttributeSet associated with the given name
 * or null if no AttributeSet is found
 */
NodeSet* ProcessorState::getAttributeSet(const txExpandedName& aName)
{
    NodeSet* attset = new NodeSet;
    if (!attset)
        return attset;

    ImportFrame* frame;
    txListIterator frameIter(&mImportFrames);
    frameIter.resetToEnd();

    while ((frame = (ImportFrame*)frameIter.previous())) {
        NodeSet* nodes = (NodeSet*)frame->mNamedAttributeSets.get(aName);
        if (nodes)
            attset->append(nodes);
    }
    return attset;
}

Expr* ProcessorState::getExpr(Element* aElem, ExprAttr aAttr)
{
    NS_ASSERTION(aElem, "missing element while getting expression");

    Expr* expr = (Expr*)mExprHashes[aAttr].get(aElem);
    if (expr) {
        return expr;
    }
    String attr;
    MBool hasAttr = MB_FALSE;
    switch (aAttr) {
        case SelectAttr:
            hasAttr = aElem->getAttr(txXSLTAtoms::select, kNameSpaceID_None,
                                     attr);
            break;
        case TestAttr:
            hasAttr = aElem->getAttr(txXSLTAtoms::test, kNameSpaceID_None,
                                     attr);
            break;
        case ValueAttr:
            hasAttr = aElem->getAttr(txXSLTAtoms::value, kNameSpaceID_None,
                                     attr);
            break;
    }

    if (!hasAttr)
        return 0;

    txPSParseContext pContext(this, aElem);
    expr = ExprParser::createExpr(attr, &pContext);

    if (!expr) {
        String err("Error in parsing XPath expression: ");
        err.append(attr);
        receiveError(err, NS_ERROR_XPATH_PARSE_FAILED);
    }
    else {
        mExprHashes[aAttr].put(aElem, expr);
    }
    return expr;
}

txPattern* ProcessorState::getPattern(Element* aElem, PatternAttr aAttr)
{
    NS_ASSERTION(aElem, "missing element while getting pattern");

    txPattern* pattern = (txPattern*)mPatternHashes[aAttr].get(aElem);
    if (pattern) {
        return pattern;
    }
    String attr;
    MBool hasAttr = MB_FALSE;
    switch (aAttr) {
        case CountAttr:
            hasAttr = aElem->getAttr(txXSLTAtoms::count, kNameSpaceID_None,
                                     attr);
            break;
        case FromAttr:
            hasAttr = aElem->getAttr(txXSLTAtoms::from, kNameSpaceID_None,
                                     attr);
            break;
    }

    if (!hasAttr)
        return 0;

    
    txPSParseContext pContext(this, aElem);
    pattern = txPatternParser::createPattern(attr, &pContext, this);

    if (!pattern) {
        String err("Error in parsing pattern: ");
        err.append(attr);
        receiveError(err, NS_ERROR_XPATH_PARSE_FAILED);
    }
    else {
        mPatternHashes[aAttr].put(aElem, pattern);
    }
    return pattern;
}

/*
 * Returns the template associated with the given name, or
 * null if not template is found
 */
Element* ProcessorState::getNamedTemplate(const txExpandedName& aName)
{
    ImportFrame* frame;
    txListIterator frameIter(&mImportFrames);

    while ((frame = (ImportFrame*)frameIter.next())) {
        Element* templ = (Element*)frame->mNamedTemplates.get(aName);
        if (templ)
            return templ;
    }
    return 0;
}

txOutputFormat* ProcessorState::getOutputFormat()
{
    return &mOutputFormat;
}

Document* ProcessorState::getRTFDocument()
{
    return mRTFDocument;
}

void ProcessorState::setRTFDocument(Document* aDoc)
{
    mRTFDocument = aDoc;
}

Document* ProcessorState::getStylesheetDocument()
{
    NS_ASSERTION(xslDocument, "missing stylesheet document");
    return xslDocument;
}

/*
 * Add a global variable
 */
nsresult ProcessorState::addGlobalVariable(const txExpandedName& aVarName,
                                           Element* aVarElem,
                                           ImportFrame* aImportFrame,
                                           ExprResult* aDefaultValue)
{
    // If we don't know the value, it's a plain global var, add it
    // to the import frame for late evaluation.
    if (!aDefaultValue) {
        return aImportFrame->mVariables.add(aVarName, aVarElem);
    }
    // Otherwise, add a GlobalVariableValue not owning the value.
    GlobalVariableValue* var =
        (GlobalVariableValue*)mGlobalVariableValues.get(aVarName);
    if (var) {
        // we set this parameter twice, we should set it to the same
        // value;
        NS_ENSURE_TRUE(var->mValue == aDefaultValue, NS_ERROR_UNEXPECTED);
        return NS_OK;
    }
    var = new GlobalVariableValue(aDefaultValue);
    NS_ENSURE_TRUE(var, NS_ERROR_OUT_OF_MEMORY);

    return mGlobalVariableValues.add(aVarName, var);
}

/*
 * Returns map on top of the stack of local variable-bindings
 */
txVariableMap* ProcessorState::getLocalVariables()
{
    return mLocalVariables;
}

/*
 * Sets top map of the local variable-bindings stack
 */
void ProcessorState::setLocalVariables(txVariableMap* aMap)
{
    mLocalVariables = aMap;
}

void ProcessorState::processAttrValueTemplate(const String& aAttValue,
                                              Element* aContext,
                                              String& aResult)
{
    aResult.clear();
    txPSParseContext pContext(this, aContext);
    AttributeValueTemplate* avt =
        ExprParser::createAttributeValueTemplate(aAttValue, &pContext);

    if (!avt) {
        // fallback, just copy the attribute
        aResult.append(aAttValue);
        return;
    }

    ExprResult* exprResult = avt->evaluate(this->getEvalContext());
    delete avt;
    if (!exprResult) {
        // XXX ErrorReport: out of memory
        return;
    }

    exprResult->stringValue(aResult);
    delete exprResult;
}

/**
 * Adds the set of names to the Whitespace handling list.
 * xsl:strip-space calls this with MB_TRUE, xsl:preserve-space 
 * with MB_FALSE
 */
void ProcessorState::shouldStripSpace(String& aNames, Element* aElement,
                                      MBool aShouldStrip,
                                      ImportFrame* aImportFrame)
{
    //-- split names on whitespace
    txTokenizer tokenizer(aNames);
    String name;
    while (tokenizer.hasMoreTokens()) {
        tokenizer.nextToken(name);
        String prefix, lname;
        PRInt32 aNSID = kNameSpaceID_None;
        txAtom* prefixAtom = 0;
        XMLUtils::getPrefix(name, prefix);
        if (!prefix.isEmpty()) {
            prefixAtom = TX_GET_ATOM(prefix);
            aNSID = aElement->lookupNamespaceID(prefixAtom);
        }
        XMLUtils::getLocalPart(name, lname);
        txAtom* lNameAtom = TX_GET_ATOM(lname);
        txNameTestItem* nti = new txNameTestItem(prefixAtom, lNameAtom,
                                                 aNSID, aShouldStrip);
        TX_IF_RELEASE_ATOM(prefixAtom);
        TX_IF_RELEASE_ATOM(lNameAtom);
        if (!nti) {
            // XXX error report, parsing error or out of mem
            break;
        }
        double priority = nti->getDefaultPriority();
        txListIterator iter(&aImportFrame->mWhiteNameTests);
        while (iter.hasNext()) {
            txNameTestItem* iNameTest = (txNameTestItem*)iter.next();
            if (iNameTest->getDefaultPriority() <= priority) {
                break;
            }
        }
        iter.addBefore(nti);
    }

} //-- stripSpace

/**
 * Adds the supplied xsl:key to the set of keys
**/
MBool ProcessorState::addKey(Element* aKeyElem)
{
    nsresult rv = NS_OK;
    String keyQName;
    aKeyElem->getAttr(txXSLTAtoms::name, kNameSpaceID_None, keyQName);
    txExpandedName keyName;
    rv = keyName.init(keyQName, aKeyElem, MB_FALSE);
    if (NS_FAILED(rv))
        return MB_FALSE;

    txXSLKey* xslKey = (txXSLKey*)mXslKeys.get(keyName);
    if (!xslKey) {
        xslKey = new txXSLKey(this);
        if (!xslKey)
            return MB_FALSE;
        rv = mXslKeys.add(keyName, xslKey);
        if (NS_FAILED(rv))
            return MB_FALSE;
    }
    txPattern* match = 0;
    txPSParseContext pContext(this, aKeyElem);
    String attrVal;
    if (aKeyElem->getAttr(txXSLTAtoms::match, kNameSpaceID_None, attrVal)) {
        match = txPatternParser::createPattern(attrVal, &pContext, this);
    }
    Expr* use = 0;
    attrVal.clear();
    if (aKeyElem->getAttr(txXSLTAtoms::use, kNameSpaceID_None, attrVal)) {
        use = ExprParser::createExpr(attrVal, &pContext);
    }
    if (!match || !use || !xslKey->addKey(match, use)) {
        delete match;
        delete use;
        return MB_FALSE;
    }
    return MB_TRUE;
}

/**
 * Adds the supplied xsl:key to the set of keys
 * returns NULL if no such key exists
**/
txXSLKey* ProcessorState::getKey(txExpandedName& keyName)
{
    return (txXSLKey*)mXslKeys.get(keyName);
}

/*
 * Adds a decimal format. Returns false if the format already exists
 * but dosn't contain the exact same parametervalues
 */
MBool ProcessorState::addDecimalFormat(Element* element)
{
    // build new DecimalFormat structure
    nsresult rv = NS_OK;
    MBool success = MB_TRUE;
    txDecimalFormat* format = new txDecimalFormat;
    if (!format)
        return MB_FALSE;

    String formatNameStr, attValue;
    txExpandedName formatName;
    if (element->getAttr(txXSLTAtoms::name, kNameSpaceID_None,
                         formatNameStr)) {
        rv = formatName.init(formatNameStr, element, MB_FALSE);
        if (NS_FAILED(rv))
            return MB_FALSE;
    }

    if (element->getAttr(txXSLTAtoms::decimalSeparator,
                         kNameSpaceID_None, attValue)) {
        if (attValue.length() == 1)
            format->mDecimalSeparator = attValue.charAt(0);
        else
            success = MB_FALSE;
    }

    if (element->getAttr(txXSLTAtoms::groupingSeparator,
                         kNameSpaceID_None, attValue)) {
        if (attValue.length() == 1)
            format->mGroupingSeparator = attValue.charAt(0);
        else
            success = MB_FALSE;
    }

    if (element->getAttr(txXSLTAtoms::infinity,
                         kNameSpaceID_None, attValue))
        format->mInfinity=attValue;

    if (element->getAttr(txXSLTAtoms::minusSign,
                         kNameSpaceID_None, attValue)) {
        if (attValue.length() == 1)
            format->mMinusSign = attValue.charAt(0);
        else
            success = MB_FALSE;
    }

    if (element->getAttr(txXSLTAtoms::NaN, kNameSpaceID_None,
                         attValue))
        format->mNaN=attValue;
        
    if (element->getAttr(txXSLTAtoms::percent, kNameSpaceID_None,
                         attValue)) {
        if (attValue.length() == 1)
            format->mPercent = attValue.charAt(0);
        else
            success = MB_FALSE;
    }

    if (element->getAttr(txXSLTAtoms::perMille,
                         kNameSpaceID_None, attValue)) {
        if (attValue.length() == 1)
            format->mPerMille = attValue.charAt(0);
        else if (!attValue.isEmpty())
            success = MB_FALSE;
    }

    if (element->getAttr(txXSLTAtoms::zeroDigit,
                         kNameSpaceID_None, attValue)) {
        if (attValue.length() == 1)
            format->mZeroDigit = attValue.charAt(0);
        else if (!attValue.isEmpty())
            success = MB_FALSE;
    }

    if (element->getAttr(txXSLTAtoms::digit, kNameSpaceID_None,
                         attValue)) {
        if (attValue.length() == 1)
            format->mDigit = attValue.charAt(0);
        else
            success = MB_FALSE;
    }

    if (element->getAttr(txXSLTAtoms::patternSeparator,
                         kNameSpaceID_None, attValue)) {
        if (attValue.length() == 1)
            format->mPatternSeparator = attValue.charAt(0);
        else
            success = MB_FALSE;
    }

    if (!success) {
        delete format;
        return MB_FALSE;
    }

    // Does an existing format with that name exist?
    txDecimalFormat* existing =
        (txDecimalFormat*)mDecimalFormats.get(formatName);
    if (existing) {
        success = existing->isEqual(format);
        delete format;
    }
    else {
        rv = mDecimalFormats.add(formatName, format);
        if (NS_FAILED(rv)) {
            delete format;
            success = MB_FALSE;
        }
    }
    
    return success;
}

/*
 * Returns a decimal format or NULL if no such format exists.
 */
txDecimalFormat* ProcessorState::getDecimalFormat(const txExpandedName& aName)
{
    txDecimalFormat* format = (txDecimalFormat*)mDecimalFormats.get(aName);
    if (!format && !aName.mLocalName &&
        aName.mNamespaceID == kNameSpaceID_None)
        return &mDefaultDecimalFormat;
    return format;
}

/**
 * Returns the value of a given variable binding within the current scope
 * @param the name to which the desired variable value has been bound
 * @return the ExprResult which has been bound to the variable with the given
 * name
**/
nsresult ProcessorState::getVariable(PRInt32 aNamespace, txAtom* aLName,
                                     ExprResult*& aResult)
{
    nsresult rv;
    aResult = 0;
    ExprResult* exprResult;
    txExpandedName varName(aNamespace, aLName);
    
    // Check local variables
    if (mLocalVariables) {
        exprResult = mLocalVariables->getVariable(varName);
        if (exprResult) {
            aResult = exprResult;
            return NS_OK;
        }
    }

    // Check if global variable is already evaluated
    GlobalVariableValue* globVar;
    globVar = (GlobalVariableValue*)mGlobalVariableValues.get(varName);
    if (globVar) {
        if (globVar->mFlags == GlobalVariableValue::evaluating) {
            String err("Cyclic variable-value detected");
            receiveError(err, NS_ERROR_FAILURE);
            return NS_ERROR_FAILURE;
        }
        aResult = globVar->mValue;
        return NS_OK;
    }

    // We need to evaluate the variable

    // Search ImportFrames for the variable
    ImportFrame* frame;
    txListIterator frameIter(&mImportFrames);
    Element* varElem = 0;
    while (!varElem && (frame = (ImportFrame*)frameIter.next()))
        varElem = (Element*)frame->mVariables.get(varName);

    if (!varElem)
        return NS_ERROR_FAILURE;

    // Evaluate the variable
    globVar = new GlobalVariableValue();
    if (!globVar)
        return NS_ERROR_OUT_OF_MEMORY;
    rv = mGlobalVariableValues.add(varName, globVar);
    if (NS_FAILED(rv)) {
        delete globVar;
        return rv;
    }

    // Set up the state we have at the beginning of the transformation
    txVariableMap *oldVars = mLocalVariables;
    mLocalVariables = 0;
    txSingleNodeContext evalContext(mSourceDocument, this);
    txIEvalContext* priorEC = setEvalContext(&evalContext);
    // Compute the variable value
    globVar->mFlags = GlobalVariableValue::evaluating;
    globVar->mValue = txXSLTProcessor::processVariable(varElem, this);
    setEvalContext(priorEC);
    mLocalVariables = oldVars;

    // evaluation is over, the gvv now owns the ExprResult
    globVar->mFlags = GlobalVariableValue::owned;
    aResult = globVar->mValue;
    return NS_OK;
}

/**
 * Determines if the given XML node allows Whitespace stripping
**/
MBool ProcessorState::isStripSpaceAllowed(Node* aNode)
{
    if (!aNode)
        return MB_FALSE;

    switch (aNode->getNodeType()) {
        case Node::ELEMENT_NODE:
        {
            // check Whitespace stipping handling list against given Node
            ImportFrame* frame;
            txListIterator frameIter(&mImportFrames);

            while ((frame = (ImportFrame*)frameIter.next())) {
                txListIterator iter(&frame->mWhiteNameTests);
                while (iter.hasNext()) {
                    txNameTestItem* iNameTest = (txNameTestItem*)iter.next();
                    if (iNameTest->matches(aNode, this)) {
                        if (iNameTest->stripsSpace() && 
                            !XMLUtils::getXMLSpacePreserve(aNode)) {
                            return MB_TRUE;
                        }
                        return MB_FALSE;
                    }
                }
            }
            break;
        }
        case Node::TEXT_NODE:
        case Node::CDATA_SECTION_NODE:
        {
            if (!XMLUtils::isWhitespace(aNode->getNodeValue()))
                return MB_FALSE;
            return isStripSpaceAllowed(aNode->getParentNode());
        }
        case Node::DOCUMENT_NODE:
        {
            return MB_TRUE;
        }
    }
    return MB_FALSE;
}

/**
 *  Notifies this Error observer of a new error using the given error level
**/
void ProcessorState::receiveError(const String& errorMessage, nsresult aRes)
{
    txListIterator iter(&errorObservers);
    while (iter.hasNext()) {
        ErrorObserver* observer = (ErrorObserver*)iter.next();
        observer->receiveError(errorMessage, aRes);
    }
}

/**
 * Returns a call to the function that has the given name.
 * This method is used for XPath Extension Functions.
 * @return the FunctionCall for the function with the given name.
**/
#define CHECK_FN(_name) aName == txXSLTAtoms::_name

nsresult ProcessorState::resolveFunctionCall(txAtom* aName, PRInt32 aID,
                                             Element* aElem,
                                             FunctionCall*& aFunction)
{
   aFunction = 0;

   if (aID != kNameSpaceID_None) {
       return NS_ERROR_XPATH_PARSE_FAILED;
   }
   if (CHECK_FN(document)) {
       aFunction = new DocumentFunctionCall(this, aElem);
       return NS_OK;
   }
   if (CHECK_FN(key)) {
       aFunction = new txKeyFunctionCall(this, aElem);
       return NS_OK;
   }
   if (CHECK_FN(formatNumber)) {
       aFunction = new txFormatNumberFunctionCall(this, aElem);
       return NS_OK;
   }
   if (CHECK_FN(current)) {
       aFunction = new CurrentFunctionCall(this);
       return NS_OK;
   }
   if (CHECK_FN(unparsedEntityUri)) {
       return NS_ERROR_NOT_IMPLEMENTED;
   }
   if (CHECK_FN(generateId)) {
       aFunction = new GenerateIdFunctionCall();
       return NS_OK;
   }
   if (CHECK_FN(systemProperty)) {
       aFunction = new SystemPropertyFunctionCall(aElem);
       return NS_OK;
   }
   if (CHECK_FN(elementAvailable)) {
       aFunction = new ElementAvailableFunctionCall(aElem);
       return NS_OK;
   }
   if (CHECK_FN(functionAvailable)) {
       aFunction = new FunctionAvailableFunctionCall(aElem);
       return NS_OK;
   }

   return NS_ERROR_XPATH_PARSE_FAILED;
} //-- resolveFunctionCall

  //-------------------/
 //- Private Methods -/
//-------------------/

ProcessorState::ImportFrame::ImportFrame(ImportFrame* aFirstNotImported)
    : mNamedTemplates(MB_FALSE),
      mMatchableTemplates(MB_TRUE),
      mNamedAttributeSets(MB_TRUE),
      mFirstNotImported(aFirstNotImported),
      mVariables(MB_FALSE)
{
}

ProcessorState::ImportFrame::~ImportFrame()
{
    // Delete all txNameTestItems
    txListIterator whiteIter(&mWhiteNameTests);
    while (whiteIter.hasNext())
        delete (txNameTestItem*)whiteIter.next();

    // Delete templates in mMatchableTemplates
    txExpandedNameMap::iterator iter(mMatchableTemplates);
    while (iter.next()) {
        txListIterator templIter((txList*)iter.value());
        MatchableTemplate* templ;
        while ((templ = (MatchableTemplate*)templIter.next())) {
            delete templ->mMatch;
            delete templ;
        }
    }
}

/*
 * txPSParseContext
 * txIParseContext used by ProcessorState internally
 */

nsresult txPSParseContext::resolveNamespacePrefix(txAtom* aPrefix,
                                                  PRInt32& aID)
{
#ifdef DEBUG
    if (!aPrefix || aPrefix == txXMLAtoms::_empty) {
        // default namespace is not forwarded to xpath
        NS_ASSERTION(0, "caller should handle default namespace ''");
        aID = kNameSpaceID_None;
        return NS_OK;
    }
#endif
    aID = mStyle->lookupNamespaceID(aPrefix);
    return (aID != kNameSpaceID_Unknown) ? NS_OK : NS_ERROR_FAILURE;
}

nsresult txPSParseContext::resolveFunctionCall(txAtom* aName, PRInt32 aID,
                                               FunctionCall*& aFunction)
{
    return mPS->resolveFunctionCall(aName, aID, mStyle, aFunction);
}

void txPSParseContext::receiveError(const String& aMsg, nsresult aRes)
{
    mPS->receiveError(aMsg, aRes);
}

/*
 * GlobalVariableValue, Used avoid circular dependencies of variables
 */

ProcessorState::GlobalVariableValue::~GlobalVariableValue()
{
    NS_ASSERTION(mFlags != evaluating, "deleted while evaluating");
    if (mFlags == owned) {
        delete mValue;
    }
}
