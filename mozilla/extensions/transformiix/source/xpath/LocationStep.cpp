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
 * (C) 1999 Keith Visco. All Rights Reserved.
 * 
 * Contributor(s): 
 * Keith Visco, kvisco@ziplink.net
 *   -- original author.
 * 
 */

/*
  Implementation of an XPath LocationStep
*/

#include "Expr.h"
#include "txForwardContext.h"

/**
 * Creates a new LocationStep using the given NodeExpr and Axis Identifier
 * @param nodeExpr the NodeExpr to use when matching Nodes
 * @param axisIdentifier the Axis Identifier in which to search for nodes
**/
LocationStep::LocationStep(txNodeTest* aNodeTest,
                           LocationStepType aAxisIdentifier)
{
    mNodeTest = aNodeTest;
    mAxisIdentifier = aAxisIdentifier;
} //-- LocationStep

/**
 * Destroys this LocationStep
 * All predicates will be deleted
 * The NodeExpr will be deleted
**/
LocationStep::~LocationStep() {
    delete mNodeTest;
} //-- ~LocationStep

/**
 * Evaluates this Expr based on the given context node and processor state
 * @param context the context node for evaluation of this Expr
 * @param ps the ProcessorState containing the stack information needed
 * for evaluation
 * @return the result of the evaluation
 * @see Expr
**/
ExprResult* LocationStep::evaluate(txIEvalContext* aContext)
{
    return evalStep(aContext->getContextNode(), aContext);
}
   
/*
 * Evaluates this LocationStep as step.
 * This doesn't need a context node set. That is needed for the 
 * predicates, and is generated on evalution.
 */
NodeSet* LocationStep::evalStep(Node* aNode, txIMatchContext* aContext)
{


    NodeSet* nodes = new NodeSet();
    if (!aContext || !mNodeTest || !nodes)
        return nodes;

    Node* node = aNode;
    switch (mAxisIdentifier) {
        case ANCESTOR_AXIS :
            node = node->getXPathParent();
            //-- do not break here
        case ANCESTOR_OR_SELF_AXIS :
            while (node) {
                if (mNodeTest->matches(node, aContext)) {
                    nodes->add(node);
                }
                node = node->getXPathParent();
            }
            break;
        case ATTRIBUTE_AXIS :
        {
            NamedNodeMap* atts = node->getAttributes();
            if ( atts ) {
                for ( PRUint32 i = 0; i < atts->getLength(); i++ ) {
                    Node* attr = atts->item(i);
                    if (mNodeTest->matches(attr, aContext))
                        nodes->add(attr);
                }
            }
            break;
        }
        case DESCENDANT_OR_SELF_AXIS :
            if (mNodeTest->matches(node, aContext))
                nodes->add(node);
            //-- do not break here
        case DESCENDANT_AXIS :
            fromDescendants(node, aContext, nodes);
            break;
        case FOLLOWING_AXIS :
        {
            if (node->getNodeType() == Node::ATTRIBUTE_NODE) {
                node = node->getXPathParent();
                fromDescendants(node, aContext, nodes);
            }
            while (node && !node->getNextSibling()) {
                node = node->getXPathParent();
            }
            while (node) {
                node = node->getNextSibling();

                if (mNodeTest->matches(node, aContext))
                    nodes->add(node);

                if (node->hasChildNodes())
                    fromDescendants(node, aContext, nodes);

                while (node && !node->getNextSibling()) {
                    node = node->getParentNode();
                }
            }
            break;
        }
        case FOLLOWING_SIBLING_AXIS :
            node = node->getNextSibling();
            while (node) {
                if (mNodeTest->matches(node, aContext))
                    nodes->add(node);
                node = node->getNextSibling();
            }
            break;
        case NAMESPACE_AXIS : //-- not yet implemented
#if 0
            // XXX DEBUG OUTPUT
            cout << "namespace axis not yet implemented"<<endl;
#endif
            break;
        case PARENT_AXIS :
        {
            Node* parent = node->getXPathParent();
            if (mNodeTest->matches(parent, aContext))
                    nodes->add(parent);
            break;
        }
        case PRECEDING_AXIS :
            while (node && !node->getPreviousSibling()) {
                node = node->getXPathParent();
            }
            while (node) {
                node = node->getPreviousSibling();

                if (node->hasChildNodes())
                    fromDescendantsRev(node, aContext, nodes);

                if (mNodeTest->matches(node, aContext))
                    nodes->add(node);

                while (node && !node->getPreviousSibling()) {
                    node = node->getParentNode();
                }
            }
            break;
        case PRECEDING_SIBLING_AXIS:
            node = node->getPreviousSibling();
            while (node) {
                if (mNodeTest->matches(node, aContext))
                    nodes->add(node);
                node = node->getPreviousSibling();
            }
            break;
        case SELF_AXIS :
            if (mNodeTest->matches(node, aContext))
                    nodes->add(node);
            break;
        default: //-- Children Axis
        {
            Node* tmpNode = node->getFirstChild();
            while (tmpNode) {
                if (mNodeTest->matches(tmpNode, aContext))
                    nodes->add(tmpNode);
                tmpNode = tmpNode->getNextSibling();
            }
            break;
        }
    } //-- switch

    //-- apply predicates
    evaluatePredicates(nodes, aContext);

    return nodes;
} //-- evaluate

/**
 * Returns the default priority of this Pattern based on the given Node,
 * context Node, and ContextState.
**/
double LocationStep::getDefaultPriority()
{
    if (isEmpty())
        return mNodeTest->getDefaultPriority();
    return 0.5;
} //-- getDefaultPriority


void LocationStep::fromDescendants(Node* node, txIMatchContext* cs,
                                   NodeSet* nodes)
{
    if (!node)
        return;

    Node* child = node->getFirstChild();
    while (child) {
        if (mNodeTest->matches(child, cs))
            nodes->add(child);
        //-- check childs descendants
        if (child->hasChildNodes())
            fromDescendants(child, cs, nodes);

        child = child->getNextSibling();
    }

} //-- fromDescendants

void LocationStep::fromDescendantsRev(Node* node, txIMatchContext* cs,
                                      NodeSet* nodes)
{
    if (!node)
        return;

    Node* child = node->getLastChild();
    while (child) {
        //-- check childs descendants
        if (child->hasChildNodes())
            fromDescendantsRev(child, cs, nodes);

        if (mNodeTest->matches(child, cs))
            nodes->add(child);

        child = child->getPreviousSibling();
    }

} //-- fromDescendantsRev

/**
 * Determines whether this Expr matches the given node within
 * the given context
**/
MBool LocationStep::matches(Node* node, txIMatchContext* aContext)
{
    if (!mNodeTest || !node)
        return MB_FALSE;

    NS_ASSERTION(CHILD_AXIS == mAxisIdentifier ||
                 ATTRIBUTE_AXIS == mAxisIdentifier,
                 "LocationStep is invalid pattern");

    if (!mNodeTest->matches(node, aContext))
        return MB_FALSE;

    MBool result = MB_TRUE;

    NS_ASSERTION(!isEmpty(), "A LocationStep without predicates, bah");

    // Create the context node set for evaluating the predicates
    NodeSet nodes;
    Node* parent = node->getXPathParent();
    switch (mAxisIdentifier) {
        case CHILD_AXIS:
            {
                Node* tmpNode = parent->getFirstChild();
                while (tmpNode) {
                    if (mNodeTest->matches(tmpNode, aContext))
                        nodes.add(tmpNode);
                    tmpNode = tmpNode->getNextSibling();
                }
                break;
            }
        case ATTRIBUTE_AXIS:
            {
                NamedNodeMap* atts = parent->getAttributes();
                if (atts) {
                    PRUint32 i;
                    for (i = 0; i < atts->getLength(); i++) {
                        Node* attr = atts->item(i);
                        if (mNodeTest->matches(attr, aContext))
                            nodes.add(attr);
                    }
                }
            }
            break;
        default:
            return MB_FALSE;
    }
    txForwardContext evalContext(aContext, node, &nodes);
    return matchPredicates(&evalContext);
} // matches

/**
 * Creates a String representation of this Expr
 * @param str the destination String to append to
 * @see Expr
**/
void LocationStep::toString(String& aDest) {
    switch (mAxisIdentifier) {
        case ANCESTOR_AXIS :
            aDest.append("ancestor::");
            break;
        case ANCESTOR_OR_SELF_AXIS :
            aDest.append("ancestor-or-self::");
            break;
        case ATTRIBUTE_AXIS:
            aDest.append("@");
            break;
        case DESCENDANT_AXIS:
            aDest.append("descendant::");
            break;
        case DESCENDANT_OR_SELF_AXIS:
            aDest.append("descendant-or-self::");
            break;
        case FOLLOWING_AXIS :
            aDest.append("following::");
            break;
        case FOLLOWING_SIBLING_AXIS:
            aDest.append("following-sibling::");
            break;
        case NAMESPACE_AXIS:
            aDest.append("namespace::");
            break;
        case PARENT_AXIS :
            aDest.append("parent::");
            break;
        case PRECEDING_AXIS :
            aDest.append("preceding::");
            break;
        case PRECEDING_SIBLING_AXIS :
            aDest.append("preceding-sibling::");
            break;
        case SELF_AXIS :
            aDest.append("self::");
            break;
        default:
            break;
    }
    if (mNodeTest)
        mNodeTest->toString(aDest);

    PredicateList::toString(aDest);
} // toString

