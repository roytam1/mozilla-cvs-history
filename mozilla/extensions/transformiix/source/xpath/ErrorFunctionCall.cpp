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

#include "FunctionLib.h"

/**
 *  Creates an Error FunctionCall with no error message
**/
ErrorFunctionCall::ErrorFunctionCall() : FunctionCall(XPathNames::ERROR_FN) {};

/**
 * Creates an Error FunctionCall with the given error message
**/
ErrorFunctionCall::ErrorFunctionCall
    (const String& errorMsg) : FunctionCall(XPathNames::ERROR_FN)
{
    //-- copy errorMsg
    this->errorMessage = errorMsg;
} //-- ErrorFunctionCall

/**
 * Evaluates this Expr based on the given context node and processor state
 * @param context the context node for evaluation of this Expr
 * @param ps the ContextState containing the stack information needed
 * for evaluation
 * @return the result of the evaluation
**/
ExprResult* ErrorFunctionCall::evaluate(txIEvalContext* aContext) {
    return new StringResult(errorMessage);
} //-- evaluate

void ErrorFunctionCall::setErrorMessage(String& errorMsg) {
    //-- copy errorMsg
    this->errorMessage = errorMsg;
} //-- setError

