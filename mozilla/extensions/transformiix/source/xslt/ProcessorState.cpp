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
#include "URIUtils.h"
#include "XMLUtils.h"
#include "XMLDOMUtils.h"
#include "Tokenizer.h"
#include "VariableBinding.h"
#include "ExprResult.h"
#include "Names.h"
#ifndef TX_EXE
//  #include "nslog.h"
//  #define PRINTF NS_LOG_PRINTF(XPATH)
//  #define FLUSH  NS_LOG_FLUSH(XPATH)
#else
  #include "TxLog.h"
#endif

  //-------------/
 //- Constants -/
//-------------/
const String ProcessorState::wrapperNSPrefix  = "transformiix";
const String ProcessorState::wrapperName      = "transformiix:result";
const String ProcessorState::wrapperNS        = "http://www.mitre.org/TransforMiix";

/**
 * Creates a new ProcessorState
**/
ProcessorState::ProcessorState() {
    this->mSourceDocument = NULL;
    this->xslDocument = NULL;
    this->resultDocument = NULL;
    currentAction = 0;
    initialize();
} //-- ProcessorState

/**
 * Creates a new ProcessorState for the given XSL document
 * and resultDocument
**/
ProcessorState::ProcessorState(Document& sourceDocument, Document& xslDocument, Document& resultDocument) {
    this->mSourceDocument = &sourceDocument;
    this->xslDocument = &xslDocument;
    this->resultDocument = &resultDocument;
    currentAction = 0;
    initialize();
} //-- ProcessorState

/**
 * Destroys this ProcessorState
**/
ProcessorState::~ProcessorState() {

  delete resultNodeStack;

  while ( ! variableSets.empty() ) {
      delete (NamedMap*) variableSets.pop();
  }

  //-- delete includes
  StringList* keys = includes.keys();
  StringListIterator* iter = keys->iterator();
  while (iter->hasNext()) {
      String* key = iter->next();
      TxObjectWrapper* objWrapper
          = (TxObjectWrapper*)includes.remove(*key);
      delete (Document*)objWrapper->object;
      delete objWrapper;
  }
  delete iter;
  delete keys;

  //-- clean up XSLT actions stack
  while (currentAction) {
      XSLTAction* item = currentAction;
      item->node = 0;
      currentAction = item->prev;
      item->prev = 0;
      delete item;
  }
} //-- ~ProcessorState


/**
 *  Adds the given attribute set to the list of available named attribute sets
 * @param attributeSet the Element to add as a named attribute set
**/
void ProcessorState::addAttributeSet(Element* attributeSet) {
    if ( !attributeSet ) return;
    String name = attributeSet->getAttribute(NAME_ATTR);
    if ( name.length() == 0 ) {
#if 0
        // XXX DEBUG OUTPUT
        cout << "missing required name attribute for xsl:" << ATTRIBUTE_SET <<endl;
#endif
        return;
    }
    //-- get attribute set, if already exists, then merge
    NodeSet* attSet = (NodeSet*)namedAttributeSets.get(name);
    if ( !attSet) {
        attSet = new NodeSet();
        namedAttributeSets.put(name, attSet);
    }

    //-- add xsl:attribute elements to attSet
    Node* node = attributeSet->getFirstChild();
    while (node) {
        if ( node->getNodeType() == Node::ELEMENT_NODE) {
            String nodeName = node->getNodeName();
            String ns;
            XMLUtils::getNameSpace(nodeName, ns);
            if ( !xsltNameSpace.isEqual(ns)) continue;
            String localPart;
            XMLUtils::getLocalPart(nodeName, localPart);
            if ( ATTRIBUTE.isEqual(localPart) ) attSet->add(node);
        }
        node = node->getNextSibling();
    }

} //-- addAttributeSet

/**
 * Registers the given ErrorObserver with this ProcessorState
**/
void ProcessorState::addErrorObserver(ErrorObserver& errorObserver) {
    errorObservers.add(&errorObserver);
} //-- addErrorObserver

/**
 * Adds the given XSL document to the list of includes
 * The href is used as a key for the include, to prevent
 * including the same document more than once
**/
void ProcessorState::addInclude(const String& href, Document* xslDocument) {
  TxObjectWrapper* objWrapper = new TxObjectWrapper();
  objWrapper->object = xslDocument;
  includes.put(href, objWrapper);
} //-- addInclude


/**
 *  Adds the given template to the list of templates to process
 * @param xslTemplate, the Element to add as a template
**/
void ProcessorState::addTemplate(Element* xslTemplate) {
    if ( !xslTemplate ) return;
    const String match = xslTemplate->getAttribute(MATCH_ATTR);
    String name = xslTemplate->getAttribute(NAME_ATTR);
    if ( name.length() > 0 ) {
        //-- check for duplicates
        TxObjectWrapper* mObj = (TxObjectWrapper*)namedTemplates.get(name);
        if ( mObj ) {
            String warn("error duplicate template name: ");
            warn.append(name);
            warn.append("\n -- using template closest to end of document");
            recieveError(warn,ErrorObserver::WARNING);
            delete mObj;
        }
        TxObjectWrapper* oldObj = mObj;
        mObj= new TxObjectWrapper();
        mObj->object = xslTemplate;
        namedTemplates.put(name,mObj);
        if ( oldObj ) delete oldObj;
    }
    if (match.length() > 0) {
        getPatternExpr(match);
        templates.add(xslTemplate);
    }
} //-- addTempalte

/**
 * Adds the given node to the result tree
 * @param node the Node to add to the result tree
**/
MBool ProcessorState::addToResultTree(Node* node) {

    Node* current = resultNodeStack->peek();
#ifndef TX_EXE
    String nameSpaceURI, name, localName;
#endif

    switch (node->getNodeType()) {

        case Node::ATTRIBUTE_NODE:
        {
            if (current->getNodeType() != Node::ELEMENT_NODE) return MB_FALSE;
            Element* element = (Element*)current;
            Attr* attr = (Attr*)node;
#ifndef TX_EXE
            name = attr->getName();
            getResultNameSpaceURI(name, nameSpaceURI);
            // XXX HACK (pvdb) Workaround for BUG 51656 Html rendered as xhtml
            if (getOutputFormat()->isHTMLOutput()) {
                name.toLowerCase();
            }
            element->setAttributeNS(nameSpaceURI, name, attr->getValue());
#else
            element->setAttribute(attr->getName(),attr->getValue());
#endif
            delete node;
            break;
        }
        case Node::ELEMENT_NODE:
            //-- if current node is the document, make sure
            //-- we don't already have a document element.
            //-- if we do, create a wrapper element
            if ( current == resultDocument ) {
                Element* docElement = resultDocument->getDocumentElement();
                if ( docElement ) {
                    String nodeName(wrapperName);
                    Element* wrapper = resultDocument->createElement(nodeName);
                    resultNodeStack->push(wrapper);
                    current->appendChild(wrapper);
                    current = wrapper;
                }
#ifndef TX_EXE
                else {
                    // Checking if we should set the output method to HTML
                    name = node->getNodeName();
                    XMLUtils::getLocalPart(name, localName);
                    if (localName.isEqualIgnoreCase(HTML)) {
                        setOutputMethod(HTML);
                        // XXX HACK (pvdb) Workaround for BUG 51656 
                        // Html rendered as xhtml
                        name.toLowerCase();
                    }
                }
#endif
            }
            current->appendChild(node);
            break;
        case Node::TEXT_NODE :
            //-- if current node is the document, create wrapper element
            if ( current == resultDocument && 
                 !XMLUtils::isWhitespace(node->getNodeValue())) {
                String nodeName(wrapperName);
                Element* wrapper = resultDocument->createElement(nodeName);
                resultNodeStack->push(wrapper);
                while (current->hasChildNodes()){
                    wrapper->appendChild(current->getFirstChild());
                    current->removeChild(current->getFirstChild());
                }
                current->appendChild(wrapper);
                current = wrapper;
            }
            current->appendChild(node);
            break;
        case Node::PROCESSING_INSTRUCTION_NODE:
        case Node::COMMENT_NODE :
            current->appendChild(node);
            break;
        case Node::DOCUMENT_FRAGMENT_NODE:
        {
            current->appendChild(node);
            delete node; //-- DOM Implementation does not clean up DocumentFragments
            break;

        }
        //-- only add if not adding to document Node
        default:
            if (current != resultDocument) current->appendChild(node);
            else return MB_FALSE;
            break;
    }
    return MB_TRUE;

} //-- addToResultTree

/**
 * Copies the node using the rules defined in the XSL specification
**/
Node* ProcessorState::copyNode(Node* node) {
    return 0;
} //-- copyNode

/**
 * Finds a template for the given Node. Only templates with
 * a mode attribute equal to the given mode will be searched.
**/
Element* ProcessorState::findTemplate(Node* node, Node* context) {
    return findTemplate(node, context, 0);
} //-- findTemplate

/**
 * Finds a template for the given Node. Only templates with
 * a mode attribute equal to the given mode will be searched.
**/
Element* ProcessorState::findTemplate(Node* node, Node* context, String* mode) {

    if (!node) return 0;
    Element* matchTemplate = 0;
    double currentPriority = Double::NEGATIVE_INFINITY;

    for (int i = 0; i < templates.size(); i++) {

        //cout << "looking at template: " << i << endl;
        Element* xslTemplate = (Element*) templates.get(i);

        //-- check mode attribute
        Attr* modeAttr = xslTemplate->getAttributeNode(MODE_ATTR);
        if (( mode ) && (!modeAttr)) continue;
        else if (( !mode ) && (modeAttr)) continue;
        else if ( mode ) {
            if ( ! mode->isEqual( modeAttr->getValue() )  ) continue;
        }
        //-- get templates match expr
        String match = xslTemplate->getAttribute(MATCH_ATTR);
        //cout << "match attr: " << match << endl;

        //-- get Expr from expression hash table
        PatternExpr* pExpr = getPatternExpr(match);
        if ( !pExpr ) continue;

        if (pExpr->matches(node, context, this)) {
            String priorityAttr = xslTemplate->getAttribute(PRIORITY_ATTR);
            double tmpPriority = 0;
            if ( priorityAttr.length() > 0 ) {
                Double dbl(priorityAttr);
                tmpPriority = dbl.doubleValue();
            }
            else tmpPriority = pExpr->getDefaultPriority(node,context,this);

            if (tmpPriority >= currentPriority) {
                matchTemplate = xslTemplate;
                currentPriority = tmpPriority;
            }
        }
    }

    return matchTemplate;
} //-- findTemplate

/**
 * Generates a unique ID for the given node and places the result in
 * dest
**/
void ProcessorState::generateId(Node* node, String& dest) {
    domHelper.generateId(node, dest);
} //-- generateId

/**
 * Returns the AttributeSet associated with the given name
 * or null if no AttributeSet is found
**/
NodeSet* ProcessorState::getAttributeSet(const String& name) {
    return (NodeSet*)namedAttributeSets.get(name);
} //-- getAttributeSet

/**
 * Returns the source node currently being processed
**/
Node* ProcessorState::getCurrentNode() {
    return currentNodeStack.peek();
} //-- setCurrentNode

/**
 * Gets the default Namespace URI stack.
**/ 
Stack* ProcessorState::getDefaultNSURIStack() {
    return &defaultNameSpaceURIStack;
} //-- getDefaultNSURIStack

/**
 * @return the included xsl document that was associated with the
 * given href, or null if no document is found
**/
Document* ProcessorState::getInclude(const String& href) {
  TxObjectWrapper* objWrapper = (TxObjectWrapper*)includes.get(href);
  Document* doc = 0;
  if (objWrapper) {
    doc = (Document*) objWrapper->object;
  }
  return doc;
} //-- getInclude(String)

Expr* ProcessorState::getExpr(const String& pattern) {
//    NS_IMPL_LOG(XPATH)
//    PRINTF("Resolving XPath Expr %s",pattern.toCharArray());
//    FLUSH();
    Expr* expr = (Expr*)exprHash.get(pattern);
    if ( !expr ) {
        expr = exprParser.createExpr(pattern);
        if ( !expr ) {
            String err = "invalid expression: ";
            err.append(pattern);
            expr = new ErrorFunctionCall(err);
        }
        exprHash.put(pattern, expr);
    }
    return expr;
} //-- getExpr

/**
 * Returns the template associated with the given name, or
 * null if not template is found
**/
Element* ProcessorState::getNamedTemplate(String& name) {
    TxObjectWrapper* mObj = (TxObjectWrapper*)namedTemplates.get(name);
    if ( mObj ) {
        return (Element*)mObj->object;
    }
    return 0;
} //-- getNamedTemplate



/**
 * Returns the namespace URI for the given name, this method should only be
 * called for determining a namespace declared within the context (ie. the stylesheet)
**/
void ProcessorState::getNameSpaceURI(const String& name, String& nameSpaceURI) {
    String prefix;
    XMLUtils::getNameSpace(name, prefix);
    getNameSpaceURIFromPrefix(prefix, nameSpaceURI);

} //-- getNameSpaceURI

/**
 * Returns the namespace URI for the given namespace prefix, this method should
 * only be called for determining a namespace declared within the context
 * (ie. the stylesheet)
**/
void ProcessorState::getNameSpaceURIFromPrefix(const String& prefix, String& nameSpaceURI) {

    XSLTAction* action = currentAction;

    while (action) {
        Node* node = action->node;
        if (( node ) && (node->getNodeType() == Node::ELEMENT_NODE)) {
            if (XMLDOMUtils::getNameSpace(prefix, (Element*) node, nameSpaceURI))
                break;
        }
        action = action->prev;
    }

} //-- getNameSpaceURI

/**
 * Returns the NodeStack which keeps track of where we are in the
 * result tree
 * @return the NodeStack which keeps track of where we are in the
 * result tree
**/
NodeStack* ProcessorState::getNodeStack() {
    return resultNodeStack;
} //-- getNodeStack

/**
 * Returns the OutputFormat which contains information on how
 * to serialize the output. I will be removing this soon, when
 * change to an event based printer, so that I can serialize
 * as I go
**/
OutputFormat* ProcessorState::getOutputFormat() {
    return &format;
} //-- getOutputFormat

PatternExpr* ProcessorState::getPatternExpr(const String& pattern) {
    PatternExpr* pExpr = (PatternExpr*)patternExprHash.get(pattern);
    if ( !pExpr ) {
        pExpr = exprParser.createPatternExpr(pattern);
        patternExprHash.put(pattern, pExpr);
    }
    return pExpr;
} //-- getPatternExpr

Document* ProcessorState::getResultDocument() {
    return resultDocument;
} //-- getResultDocument

/**
 * Returns the namespace URI for the given name, this method should only be
 * called for returning a namespace declared within in the result document.
**/
void ProcessorState::getResultNameSpaceURI(const String& name, String& nameSpaceURI) {
    String prefix;
    XMLUtils::getNameSpace(name, prefix);
    if (prefix.length() == 0) {
        nameSpaceURI.clear();
        nameSpaceURI.append(*(String*)defaultNameSpaceURIStack.peek());
    }
    else {
        String* result = (String*)nameSpaceMap.get(prefix);
        if (result) {
            nameSpaceURI.clear();
            nameSpaceURI.append(*result);
        }
    }

} //-- getResultNameSpaceURI

NodeSet* ProcessorState::getTemplates() {
   return &templates;
} //-- getTemplates


Stack* ProcessorState::getVariableSetStack() {
    return &variableSets;
} //-- getVariableSetStack

String& ProcessorState::getXSLNamespace() {
    return xsltNameSpace;
} //-- getXSLNamespace

/**
 * Determines if the given XSL node allows Whitespace stripping
**/
MBool ProcessorState::isXSLStripSpaceAllowed(Node* node) {

    if ( !node ) return MB_FALSE;
    return (MBool)(PRESERVE != getXMLSpaceMode(node));

} //--isXSLStripSpaceAllowed

/**
 * Returns the current XSLT action from the top of the stack.
 * @returns the XSLT action from the top of the stack
**/
Node* ProcessorState::peekAction() {
    NS_ASSERTION(currentAction, "currentAction is NULL, this is very bad");
    if (currentAction)
        return currentAction->node;
    return NULL;
}

/**
 * Removes the current XSLT action from the top of the stack.
 * @returns the XSLT action after removing from the top of the stack
**/
Node* ProcessorState::popAction() {
    Node* xsltAction = 0;
    if (currentAction) {
        xsltAction = currentAction->node;
        XSLTAction* item = currentAction;
        currentAction = currentAction->prev;
        item->node = 0;
        delete item;
    }
    return xsltAction;
} //-- popAction

/**
 * Removes and returns the current source node being processed, from the stack
 * @return the current source node
**/
Node* ProcessorState::popCurrentNode() {
   return currentNodeStack.pop();
} //-- popCurrentNode

/**
 * Adds the given XSLT action to the top of the action stack
**/
void ProcessorState::pushAction(Node* xsltAction) {
   if (currentAction) {
       XSLTAction* newAction = new XSLTAction;
       newAction->prev = currentAction;
       currentAction = newAction;
   }
   else {
       currentAction = new XSLTAction;
       currentAction->prev = 0;
   }
   currentAction->node = xsltAction;
} //-- pushAction

/**
 * Sets the source node currently being processed
 * @param node the source node to set as the "current" node
**/
void ProcessorState::pushCurrentNode(Node* node) {
    currentNodeStack.push(node);
} //-- setCurrentNode

/**
 * Sets a new default Namespace URI.
**/ 
void ProcessorState::setDefaultNameSpaceURIForResult(const String& nsURI) {
    String* nsTempURIPointer = 0;
    String* nsURIPointer = 0;
    StringListIterator theIterator(&nameSpaceURIList);

    while (theIterator.hasNext()) {
        nsTempURIPointer = theIterator.next();
        if (nsTempURIPointer->isEqual(nsURI)) {
            nsURIPointer = nsTempURIPointer;
            break;
        }
    }
    if ( ! nsURIPointer ) {
        nsURIPointer = new String(nsURI);
        nameSpaceURIList.add(nsURIPointer);
    }
    defaultNameSpaceURIStack.push(nsURIPointer);
} //-- setDefaultNameSpaceURI

/**
 * Sets the output method. Valid output method options are,
 * "xml", "html", or "text".
**/ 
void ProcessorState::setOutputMethod(const String& method) {
    format.setMethod(method);
    if ( method.indexOf(HTML) == 0 ) {
        setDefaultNameSpaceURIForResult(HTML_NS);
    }
}

/**
 * Adds the set of names to the Whitespace handling list.
 * xsl:strip-space calls this with MB_TRUE, xsl:preserve-space 
 * with MB_FALSE
**/
void ProcessorState::shouldStripSpace(String& names, MBool shouldStrip) {
    //-- split names on whitespace
    Tokenizer tokenizer(names);
    String name;
    while (tokenizer.hasMoreTokens()) {
        tokenizer.nextToken(name);
        txNameTestItem* nti = new txNameTestItem(name,shouldStrip);
        if (!nti) {
            // XXX error report, parsing error or out of mem
            break;
        }
        // XXX ToDo: get import precedence right, bug 83651
        double priority = nti->getDefaultPriority();
        txListIterator iter(&mWhiteNameTests);
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
 * Adds a document to set of loaded documents
**/
void ProcessorState::addLoadedDocument(Document* doc, String& url) {
    String docUrl;
    URIUtils::getDocumentURI(url, docUrl);
    loadedDocuments.put(docUrl, doc);
}

/**
 * Returns a loaded document given it's url. NULL if no such doc exists
**/
Document* ProcessorState::getLoadedDocument(String& url) {
    String docUrl;
    URIUtils::getDocumentURI(url, docUrl);
    if ((mMainStylesheetURL.length() > 0) && docUrl.isEqual(mMainStylesheetURL)) {
        return xslDocument;
    }
    else if ((mMainSourceURL.length() > 0) && docUrl.isEqual(mMainSourceURL)) {
        return mSourceDocument;
    }
    return (Document*)loadedDocuments.get(docUrl);
}

/**
 * Adds the supplied xsl:key to the set of keys
**/
MBool ProcessorState::addKey(Element* keyElem) {
    String keyName = keyElem->getAttribute(NAME_ATTR);
    if(!XMLUtils::isValidQName(keyName))
        return MB_FALSE;
    txXSLKey* xslKey = (txXSLKey*)xslKeys.get(keyName);
    if (!xslKey) {
        xslKey = new txXSLKey(this);
        if (!xslKey)
            return MB_FALSE;
        xslKeys.put(keyName, xslKey);
    }

    return xslKey->addKey(keyElem->getAttribute(MATCH_ATTR), keyElem->getAttribute(USE_ATTR));
}

/**
 * Adds the supplied xsl:key to the set of keys
 * returns NULL if no such key exists
**/
txXSLKey* ProcessorState::getKey(String& keyName) {
    return (txXSLKey*)xslKeys.get(keyName);
}

  //--------------------------------------------------/
 //- Virtual Methods from derived from ContextState -/
//--------------------------------------------------/


/**
 * Returns the Stack of context NodeSets
 * @return the Stack of context NodeSets
**/
Stack* ProcessorState::getNodeSetStack() {
    return &nodeSetStack;
} //-- getNodeSetStack

/**
 * Returns the parent of the given Node. This method is needed
 * beacuse with the DOM some nodes such as Attr do not have parents
 * @param node the Node to find the parent of
 * @return the parent of the given Node, or null if not found
**/
Node* ProcessorState::getParentNode(Node* node) {

    return domHelper.getParentNode(node);

} //-- getParentNode

/**
 * Returns the value of a given variable binding within the current scope
 * @param the name to which the desired variable value has been bound
 * @return the ExprResult which has been bound to the variable with the given
 * name
**/
ExprResult* ProcessorState::getVariable(String& name) {

    StackIterator* iter = variableSets.iterator();
    ExprResult* exprResult = 0;
    while ( iter->hasNext() ) {
        NamedMap* map = (NamedMap*) iter->next();
        if ( map->get(name)) {
            exprResult = ((VariableBinding*)map->get(name))->getValue();
            break;
        }
    }
    delete iter;
    return exprResult;
} //-- getVariable

/**
 * Determines if the given XML node allows Whitespace stripping
**/
MBool ProcessorState::isStripSpaceAllowed(Node* node) {

    if ( !node ) return MB_FALSE;

    switch ( node->getNodeType() ) {

        case Node::ELEMENT_NODE :
        {
            // check Whitespace stipping handling list against given Node
            // XXX ToDo: get import precedence right, bug 83651
            String name = node->getNodeName();
            txListIterator iter(&mWhiteNameTests);
            while (iter.hasNext()) {
                txNameTestItem* iNameTest = (txNameTestItem*)iter.next();
                if (iNameTest->matches(node,this))
                    return iNameTest->stripsSpace();
            }
            String method;
            if (format.getMethod(method).isEqual("html")) {
                String ucName = name;
                ucName.toUpperCase();
                if (ucName.isEqual("SCRIPT")) return MB_FALSE;
            }
            break;
        }
        case Node::TEXT_NODE:
            if (!XMLUtils::shouldStripTextnode(node->getNodeValue()))
                return MB_FALSE;
            return isStripSpaceAllowed(node->getParentNode());
        case Node::DOCUMENT_NODE:
            return MB_TRUE;
        default:
            break;
    }
    XMLSpaceMode mode = getXMLSpaceMode(node);
    if (mode == DEFAULT) return (MBool)(defaultSpace == STRIP);
    return (MBool)(STRIP == mode);

} //--isStripSpaceAllowed


/**
 *  Notifies this Error observer of a new error, with default
 *  level of NORMAL
**/
void ProcessorState::recieveError(String& errorMessage) {
    recieveError(errorMessage, ErrorObserver::NORMAL);
} //-- recieveError

/**
 *  Notifies this Error observer of a new error using the given error level
**/
void ProcessorState::recieveError(String& errorMessage, ErrorLevel level) {
    ListIterator* iter = errorObservers.iterator();
    while ( iter->hasNext()) {
        ErrorObserver* observer = (ErrorObserver*)iter->next();
        observer->recieveError(errorMessage, level);
    }
    delete iter;
} //-- recieveError

/**
 * Returns a call to the function that has the given name.
 * This method is used for XPath Extension Functions.
 * @return the FunctionCall for the function with the given name.
**/
FunctionCall* ProcessorState::resolveFunctionCall(const String& name) {
   String err;

   if (DOCUMENT_FN.isEqual(name)) {
       return new DocumentFunctionCall(this);
   }
   else if (KEY_FN.isEqual(name)) {
       return new txKeyFunctionCall(this);
   }
   else if (FORMAT_NUMBER_FN.isEqual(name)) {
       err = "function not yet implemented: ";
       err.append(name);
   }
   else if (CURRENT_FN.isEqual(name)) {
       return new CurrentFunctionCall(this);
   }
   else if (UNPARSED_ENTITY_URI_FN.isEqual(name)) {
       err = "function not yet implemented: ";
       err.append(name);
   }
   else if (GENERATE_ID_FN.isEqual(name)) {
       return new GenerateIdFunctionCall(&domHelper);
   }
   else if (SYSTEM_PROPERTY_FN.isEqual(name)) {
       return new SystemPropertyFunctionCall();
   }
   else if (ELEMENT_AVAILABLE_FN.isEqual(name)) {
       return new ElementAvailableFunctionCall();
   }
   else if (FUNCTION_AVAILABLE_FN.isEqual(name)) {
       return new FunctionAvailableFunctionCall();
   }
   else {
       err = "invalid function call: ";
       err.append(name);
   }

   return new ErrorFunctionCall(err);

} //-- resolveFunctionCall


/**
 * Sorts the given NodeSet by DocumentOrder.
 * @param nodes the NodeSet to sort
 *
 * Note: I will be moving this functionality elsewhere soon
**/
void ProcessorState::sortByDocumentOrder(NodeSet* nodes) {
    if (!nodes || (nodes->size() < 2))
        return;

    NodeSet sorted(nodes->size());
    sorted.setDuplicateChecking(MB_FALSE);
    sorted.add(nodes->get(0));

    int i, k;
    for (i = 1; i < nodes->size(); i++) {
        Node* node = nodes->get(i);
        for (k = i - 1; k >= 0; k--) {
            Node* tmpNode = sorted.get(k);
            if (domHelper.appearsFirst(tmpNode, node) == tmpNode) {
                if (k == i - 1)
                    sorted.add(node);
                else
                    sorted.add(k, node);
                break;
            }
            else if (k == 0) {
                sorted.add(0, node);
                break;
            }
        }
    }

    //-- save current state of duplicates checking
    MBool checkDuplicates = nodes->getDuplicateChecking();
    nodes->setDuplicateChecking(MB_FALSE);
    nodes->clear();
    for (i = 0; i < sorted.size(); i++) {
        nodes->add(sorted.get(i));
    }
    nodes->setDuplicateChecking(checkDuplicates);
    sorted.clear();

} //-- sortByDocumentOrder

  //-------------------/
 //- Private Methods -/
//-------------------/

/**
 * Returns the closest xml:space value for the given Text node
**/
ProcessorState::XMLSpaceMode ProcessorState::getXMLSpaceMode(Node* node) {

    if (!node) return DEFAULT; //-- we should never see this

    Node* parent = node;
    while ( parent ) {
        switch ( parent->getNodeType() ) {
            case Node::ELEMENT_NODE:
            {
                String value = ((Element*)parent)->getAttribute(XML_SPACE);
                if ( value.isEqual(PRESERVE_VALUE)) {
                    return PRESERVE;
                }
                break;
            }
            case Node::TEXT_NODE:
                //-- we will only see this the first time through the loop
                //-- if the argument node is a text node
                break;
            default:
                return DEFAULT;
        }
        parent = parent->getParentNode();
    }
    return DEFAULT;

} //-- getXMLSpaceMode

/**
 * Initializes this ProcessorState
**/
void ProcessorState::initialize() {
    //-- initialize default-space
    defaultSpace = PRESERVE;

    //-- add global variable set
    NamedMap* globalVars = new NamedMap();
    globalVars->setObjectDeletion(MB_TRUE);
    variableSets.push(globalVars);

    /* turn object deletion on for some of the Maps (NamedMap) */
    exprHash.setObjectDeletion(MB_TRUE);
    patternExprHash.setObjectDeletion(MB_TRUE);
    nameSpaceMap.setObjectDeletion(MB_TRUE);
    namedAttributeSets.setObjectDeletion(MB_TRUE);

    //-- named templates uses deletion, to remove the ObjectWrappers
    namedTemplates.setObjectDeletion(MB_TRUE);
    //-- do not set ObjectDeletion for templates, since the Document
    //-- handles the cleanup

    //-- create NodeStack
    resultNodeStack = new NodeStack();
    resultNodeStack->push(this->resultDocument);

    setDefaultNameSpaceURIForResult("");

    //-- determine xsl properties
    Element* element = NULL;
    if (mSourceDocument) {
        mMainSourceURL = mSourceDocument->getBaseURI();
    }
    if (xslDocument) {
        element = xslDocument->getDocumentElement();
        mMainStylesheetURL = xslDocument->getBaseURI();
    }
    if ( element ) {

        pushAction(element);

	    //-- process namespace nodes
	    NamedNodeMap* atts = element->getAttributes();
	    if ( atts ) {
	        for (PRUint32 i = 0; i < atts->getLength(); i++) {
	            Attr* attr = (Attr*)atts->item(i);
	            String attName = attr->getName();
	            String attValue = attr->getValue();
	            if ( attName.indexOf(XMLUtils::XMLNS) == 0) {
	                String ns;
	                XMLUtils::getLocalPart(attName, ns);
	                // default namespace
	                if ( attName.isEqual(XMLUtils::XMLNS) ) {
	                    //-- Is this correct?
	                    setDefaultNameSpaceURIForResult(attValue);
	                }
	                // namespace declaration
	                else {
	                    String ns;
	                    XMLUtils::getLocalPart(attName, ns);
	                    nameSpaceMap.put(ns, new String(attValue));
	                }
	                // check for XSL namespace
	                if ( attValue.indexOf(XSLT_NS) == 0) {
	                    xsltNameSpace = ns;
	                }
	            }
	            else if ( attName.isEqual(DEFAULT_SPACE_ATTR) ) {
	                if ( attValue.isEqual(STRIP_VALUE) ) {
	                    defaultSpace = STRIP;
	                }
	            }
	            else if ( attName.isEqual(RESULT_NS_ATTR) ) {
	                if (attValue.length() > 0) {
	                    if ( attValue.indexOf(HTML_NS) == 0 ) {
	                        setOutputMethod("html");
	                    }
	                    else setOutputMethod(attValue);
	                }
	            }
	            else if ( attName.isEqual(INDENT_RESULT_ATTR) ) {
	                if ( attValue.length() > 0 ) {
	                    format.setIndent(attValue.isEqual(YES_VALUE));
	                }
	            }

	        } //-- end for each att
	    } //-- end if atts are not null
	}
    
    //-- make sure all keys are deleted
    xslKeys.setObjectDeletion(MB_TRUE);
    
    //-- Make sure all loaded documents get deleted
    loadedDocuments.setObjectDeletion(MB_TRUE);
}
