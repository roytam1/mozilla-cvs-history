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
 * Marc Schefer, schefer@xo3.com
 *   -- fixed a bug regarding Or expressions
 *      - If the left hand was false, the right hand expression
 *        was not getting checked.
 *
 */


/**
 * Represents a BooleanExpr, a binary expression that
 * performs a boolean operation between it's lvalue and rvalue.
**/

#include "Expr.h"

/**
 * Creates a new BooleanExpr using the given operator
**/
BooleanExpr::BooleanExpr(Expr* leftExpr, Expr* rightExpr, short op) 
    :mOp(op), mLeftExpr(leftExpr), mRightExpr(rightExpr)
{
} // BooleanExpr

BooleanExpr::~BooleanExpr()
{
    delete mLeftExpr;
    delete mRightExpr;
} // ~BooleanExpr

/**
 * Evaluates this Expr based on the given context node and processor state
 * @param context the context node for evaluation of this Expr
 * @param ps the ContextState containing the stack information needed
 * for evaluation
 * @return the result of the evaluation
**/
ExprResult* BooleanExpr::evaluate(txIEvalContext* aContext)
{
    MBool lval = MB_FALSE;
    ExprResult* exprRes = 0;
    if (mLeftExpr) {
        exprRes = mLeftExpr->evaluate(aContext);
        if (exprRes)
            lval = exprRes->booleanValue();
        delete exprRes;
    }

    // check left expression for early decision
    if ((mOp == OR) && (lval))
        return new BooleanResult(MB_TRUE);
    if ((mOp == AND) && (!lval)) 
        return new BooleanResult(MB_FALSE);

    MBool rval = MB_FALSE;
    if (mRightExpr) {
        exprRes = mRightExpr->evaluate(aContext);
        if (exprRes)
            rval = exprRes->booleanValue();
        delete exprRes;
    }
    // just use rval, since we already checked lval
    return new BooleanResult(rval);
} // evaluate

/**
 * Returns the String representation of this Expr.
 * @param dest the String to use when creating the String
 * representation. The String representation will be appended to
 * any data in the destination String, to allow cascading calls to
 * other #toString() methods for Expressions.
 * @return the String representation of this Expr.
**/
void BooleanExpr::toString(String& aDest)
{
    if (mLeftExpr)
        mLeftExpr->toString(aDest);
    else
        aDest.append("null");

    switch (mOp) {
        case OR:
            aDest.append(" or ");
            break;
        default:
            aDest.append(" and ");
            break;
    }
    if (mRightExpr)
        mRightExpr->toString(aDest);
    else
        aDest.append("null");
} // toString

