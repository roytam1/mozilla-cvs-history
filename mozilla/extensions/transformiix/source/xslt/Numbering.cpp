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
 *    -- original author.
 *
 * $Id$
 */

/**
 * Numbering methods
 * @author <a href="mailto:kvisco@ziplink.net">Keith Visco</a>
 * @version $Revision$ $Date$
**/

#include "Numbering.h"
#include "Names.h"

void Numbering::doNumbering
    (Element* xslNumber, String& dest, Node* context, ProcessorState* ps)
{

    if ( !xslNumber ) return;

    int* counts = 0;
    int nbrOfCounts = 0;

    String valueAttr = xslNumber->getAttribute(VALUE_ATTR);
    //-- check for expr
    if (valueAttr.length() > 0) {
        Expr* expr = ps->getExpr(valueAttr);
        nbrOfCounts = 1;
        counts = new int[1];
        ExprResult* result = expr->evaluate(context, ps);
        double dbl = result->numberValue();
        delete result;
        counts[0] = (int)dbl;
    }
    else if (context) {

        //-- create count expression

        String countAttr = xslNumber->getAttribute(COUNT_ATTR);

        PatternExpr* countExpr = 0;
        if (countAttr.length() > 0) countExpr = ps->getPatternExpr(countAttr);
        else {
            switch(context->getNodeType()) {
                case Node::ATTRIBUTE_NODE:
                    countAttr.append('@');
                    countAttr.append(context->getNodeName());
                    break;
                case Node::ELEMENT_NODE:
                    countAttr.append(context->getNodeName());
                    break;
                case Node::CDATA_SECTION_NODE :
                case Node::TEXT_NODE :
                    countAttr.append("text()");
                    break;
                case Node::COMMENT_NODE :
                    countAttr.append("comment()");
                    break;
                case Node::PROCESSING_INSTRUCTION_NODE :
                    countAttr.append("processing-instruction()");
                    break;
                default:
                    countAttr.append("node()[false()]"); //-- for now
                    break;
            }
            countExpr = ps->getPatternExpr(countAttr);
        }
        NodeSet* nodes = 0;
        int cnum = 0;

        String level = xslNumber->getAttribute(LEVEL_ATTR);
        String fromAttr = xslNumber->getAttribute(FROM_ATTR);
        PatternExpr* from = 0;

        if (MULTIPLE_VALUE.isEqual(level))
            nodes = getAncestorsOrSelf(countExpr, from, context, ps, MB_FALSE);
        //else if (ANY_VALUE.isEqual(level))
        //    nodes = getAnyPreviousNodes(countExpr, context, ps);
        else
            nodes = getAncestorsOrSelf(countExpr, from, context, ps, MB_TRUE);

        nbrOfCounts = nodes->size();
        counts = new int[nbrOfCounts];
        cnum = 0;
        for (int i = nodes->size()-1; i >= 0; i--) {
            counts[cnum++] =
                countPreceedingSiblings(countExpr, nodes->get(i), ps);
        }
        delete nodes;
    }
    //-- format counts
    for ( int i = 0; i < nbrOfCounts; i++) {
        Integer::toString(counts[i], dest);
    }
    delete counts;
} //-- doNumbering

int Numbering::countPreceedingSiblings
    (PatternExpr* patternExpr, Node* context, ProcessorState* ps)
{
    int count = 1;

    if (!context) return 0;

    Node* sibling = context;
    while ((sibling = sibling->getPreviousSibling())) {
        if (patternExpr->matches(sibling, sibling, ps))
            ++count;
    }
    return count;
} //-- countPreceedingSiblings

NodeSet* Numbering::getAncestorsOrSelf
    ( PatternExpr* countExpr,
      PatternExpr* from,
      Node* context,
      ProcessorState* ps,
      MBool findNearest)
{
    NodeSet* nodeSet = new NodeSet();
    Node* parent = context;
    while ((parent)  && (parent->getNodeType() == Node::ELEMENT_NODE))
    {
        if ((from) && from->matches(parent, parent->getParentNode(), ps)) break;

        if (countExpr->matches(parent, parent->getParentNode(), ps)) {
            nodeSet->add(parent);
            if (findNearest) break;
        }
        parent = parent->getParentNode();
    }
    return nodeSet;
} //-- fromAncestorsOrSelf
