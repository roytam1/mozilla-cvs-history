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

/**
 * @author <a href="mailto:kvisco@ziplink.net">Keith Visco</a>
 * @version $Revision$ $Date$
**/

//- Constructors -/

/**
 * Creates a new BasicNodeExpr, which matchs any Node
**/
BasicNodeExpr::BasicNodeExpr() {
    this->type = NodeExpr::NODE_EXPR;
} //-- BasicNodeExpr

/**
 * Creates a new BasicNodeExpr of the given type
**/
BasicNodeExpr::BasicNodeExpr(NodeExpr::NodeExprType nodeExprType) {
    this->type = nodeExprType;
} //-- BasicNodeExpr

/**
 * Destroys this NodeExpr
**/
BasicNodeExpr::~BasicNodeExpr() {};

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
ExprResult* BasicNodeExpr::evaluate(Node* context, ContextState* cs) {
    NodeSet* nodeSet = new NodeSet();
    if ( !context ) return nodeSet;
    NodeList* nl = context->getChildNodes();
    for (UInt32 i = 0; i < nl->getLength(); i++ ) {
        Node* node = nl->item(i);
        if (matches(node, context, cs)) nodeSet->add(node);
    }
    return nodeSet;
} //-- evaluate

  //-----------------------------/
 //- Methods from NodeExpr.cpp -/
//-----------------------------/

/**
 * Returns the default priority of this Pattern based on the given Node,
 * context Node, and ContextState.
 * If this pattern does not match the given Node under the current context Node and
 * ContextState then Negative Infinity is returned.
**/
double BasicNodeExpr::getDefaultPriority(Node* node, Node* context, ContextState* cs) {
    return -0.5;
} //-- getDefaultPriority

/**
 * Returns the type of this NodeExpr
 * @return the type of this NodeExpr
**/
short BasicNodeExpr::getType() {
    return type;
} //-- getType

/**
 * Determines whether this NodeExpr matches the given node within
 * the given context
**/
MBool BasicNodeExpr::matches(Node* node, Node* context, ContextState* cs) {
    if ( !node ) return MB_FALSE;
    switch ( type ) {
        case NodeExpr::COMMENT_EXPR:
            return (MBool) (node->getNodeType() == Node::COMMENT_NODE);
        case NodeExpr::PI_EXPR :
            return (MBool) (node->getNodeType() == Node::PROCESSING_INSTRUCTION_NODE);
        default: //-- node()
            break;
    }
    return MB_TRUE;

} //-- matches


/**
 * Returns the String representation of this NodeExpr.
 * @param dest the String to use when creating the String
 * representation. The String representation will be appended to
 * any data in the destination String, to allow cascading calls to
 * other #toString() methods for Expressions.
 * @return the String representation of this NodeExpr.
**/
void BasicNodeExpr::toString(String& dest) {
    switch ( type ) {
        case NodeExpr::COMMENT_EXPR:
            dest.append("comment()");
            break;
        case NodeExpr::PI_EXPR :
            dest.append("processing-instruction()");
            break;
        default: //-- node()
            dest.append("node()");
            break;
    }
} //-- toString

