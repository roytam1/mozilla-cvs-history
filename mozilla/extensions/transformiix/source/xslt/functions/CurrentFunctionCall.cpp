#include "ProcessorState.h"
#include "txAtoms.h"
#include "XSLTFunctions.h"

/*
  Implementation of XSLT 1.0 extension function: current
*/

/**
 * Creates a new current function call
**/
CurrentFunctionCall::CurrentFunctionCall(ProcessorState* aPs) 
    : mPs(aPs)
{
}

/*
 * Evaluates this Expr
 *
 * @return NodeSet containing the context node used for the complete
 * Expr or Pattern.
 */
ExprResult* CurrentFunctionCall::evaluate(txIEvalContext* aContext)
{
    return new NodeSet(mPs->getEvalContext()->getContextNode());
}

nsresult CurrentFunctionCall::getNameAtom(txAtom** aAtom)
{
    *aAtom = txXSLTAtoms::current;
    TX_ADDREF_ATOM(*aAtom);
    return NS_OK;
}
