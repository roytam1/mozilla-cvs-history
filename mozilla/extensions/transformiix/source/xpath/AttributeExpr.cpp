/*
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
 * (C) 1999 Keith Visco. All Rights Reserved.
 * 
 * Contributor(s): 
 * Keith Visco, kvisco@ziplink.net
 *   -- original author.
 *    
 * $Id$
 */

#include "Expr.h"
#include "XMLDOMUtils.h"

/*
  This class represents a Attribute Expression as defined by the XPath
  1.0 Recommendation
*/

const String AttributeExpr::WILD_CARD = "*";

//- Constructors -/

AttributeExpr::AttributeExpr() {
    this->isNameWild      = MB_FALSE;
    this->isNamespaceWild = MB_FALSE;
} //-- AttributeExpr

AttributeExpr::AttributeExpr(String& name) {
    this->isNameWild      = MB_FALSE;
    this->isNamespaceWild = MB_FALSE;
    setName(name);
} //-- AttributeExpr

/**
 * Destructor
**/
AttributeExpr::~AttributeExpr() {
} //-- ~AttributeExpr

  //------------------/
 //- Public Methods -/
//------------------/

/**
 * Evaluates this Expr based on the given context node and processor state
 * @param context the context node for evaluation of this Expr
 * @param ps the ContextState containing the stack information needed
 * for evaluation
 * @return the result of the evaluation
**/
ExprResult* AttributeExpr::evaluate(Node* context, ContextState* cs) {

    NodeSet* nodeSet = new NodeSet();
    if ( !context ) return nodeSet;
    NamedNodeMap* atts = context->getAttributes();
    if ( atts ) {
        UInt32 i = 0;
        if ( isNameWild && isNamespaceWild ) {
            for ( ; i < atts->getLength(); i++ )
                nodeSet->add(atts->item(i));
        }
        else {
            for ( ; i < atts->getLength(); i++ ) {
                Node* attr = atts->item(i);
                if (matches(attr, context, cs)) {
                    nodeSet->add(attr);
                    if (!isNameWild) break;
                }
            }
        }
    }
    return nodeSet;
} //-- evaluate

/**
 * Returns the default priority of this Pattern based on the given Node,
 * context Node, and ContextState.
 * If this pattern does not match the given Node under the current context Node and
 * ContextState then Negative Infinity is returned.
**/
double AttributeExpr::getDefaultPriority(Node* node, Node* context, ContextState* cs) {
    return 0.0;
} //-- getDefaultPriority

/**
 * Returns the name of this ElementExpr
 * @return the name of this ElementExpr
**/
const String& AttributeExpr::getName() {
    return (const String&) this->name;
} //-- getName

void AttributeExpr::setName(const String& name) {

    if (name.isEqual(WILD_CARD)) {
        this->isNameWild      = MB_TRUE;
        this->isNamespaceWild = MB_TRUE;
        return;
    }

    int idx = name.indexOf(':');
    if ( idx >= 0 )
       name.subString(0,idx, this->prefix);
    else
       idx = -1;

    name.subString(idx+1, this->name);

    //-- set flags
    this->isNamespaceWild = MB_FALSE;
    this->isNameWild      = this->name.isEqual(WILD_CARD);

} //-- setName

void AttributeExpr::setWild(MBool isWild) {
    this->isNameWild      = isWild;
    this->isNamespaceWild = isWild;
} //-- setWild
  //-----------------------------/
 //- Methods from NodeExpr.cpp -/
//-----------------------------/

/**
 * Returns the type of this NodeExpr
 * @return the type of this NodeExpr
**/
short AttributeExpr::getType() {
    return NodeExpr::ATTRIBUTE_EXPR;
} //-- getType

/**
 * Determines whether this NodeExpr matches the given node within
 * the given context
**/
MBool AttributeExpr::matches(Node* node, Node* context, ContextState* cs) {

    if ( (!node) || (node->getNodeType() != Node::ATTRIBUTE_NODE) )
        return  MB_FALSE;

    if ( isNameWild && isNamespaceWild ) return MB_TRUE;

    const String nodeName = ((Attr*)node)->getName();
    int idx = nodeName.indexOf(':');
    if (idx >= 0) {
        String prefixForNode;
        nodeName.subString(0,idx,prefixForNode);
        String localName;
        nodeName.subString(idx+1, localName);
        if (isNamespaceWild) return localName.isEqual(this->name);
        String nsForNode;
        Node* parent = cs->getParentNode(node);
        if (parent) XMLDOMUtils::getNameSpace(prefixForNode, (Element*)parent, nsForNode);
        String nsForTest;
        cs->getNameSpaceURIFromPrefix(this->prefix,nsForTest);
        if (!nsForTest.isEqual(nsForNode)) return MB_FALSE;
        return localName.isEqual(this->name);
    }
    else {
        if (isNamespaceWild) return nodeName.isEqual(this->name);
        String nsForTest;
        cs->getNameSpaceURIFromPrefix(this->prefix, nsForTest);
        if (nsForTest.length() > 0) return MB_FALSE;
        return nodeName.isEqual(this->name);
    }
    return MB_FALSE;
} //-- matches


/**
 * Returns the String representation of this NodeExpr.
 * @param dest the String to use when creating the String
 * representation. The String representation will be appended to
 * any data in the destination String, to allow cascading calls to
 * other #toString() methods for Expressions.
 * @return the String representation of this NodeExpr.
**/
void AttributeExpr::toString(String& dest) {
    dest.append('@');
    if (isNameWild && isNamespaceWild) dest.append('*');
    else {
       dest.append(this->prefix);
       dest.append(':');
       dest.append(this->name);
    }
} //-- toString

