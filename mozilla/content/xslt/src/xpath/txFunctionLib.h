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
 * Olivier Gerardin, ogerardin@vo.lu
 *   -- added number functions
 *    
 * $Id$
 */

#include "TxString.h"
#include "primitives.h"
#include "NodeSet.h"
#include "List.h"
#include "dom.h"
#include "ExprResult.h"
#include "baseutils.h"
#include "Expr.h"
#include "Names.h"
#include "XMLUtils.h"
#include <math.h>


#ifndef TRANSFRMX_FUNCTIONLIB_H
#define TRANSFRMX_FUNCTIONLIB_H


class XPathNames {

public:
//-- Function Names
static const String BOOLEAN_FN;
static const String CONCAT_FN;
static const String CONTAINS_FN;
static const String COUNT_FN ;
static const String FALSE_FN;
static const String LAST_FN;
static const String LOCAL_NAME_FN;
static const String NAME_FN;
static const String NAMESPACE_URI_FN;
static const String NOT_FN;
static const String POSITION_FN;
static const String STARTS_WITH_FN;
static const String STRING_FN;
static const String STRING_LENGTH_FN;
static const String SUBSTRING_FN;
static const String SUBSTRING_AFTER_FN;
static const String SUBSTRING_BEFORE_FN;
static const String TRANSLATE_FN;
static const String TRUE_FN;
// OG+
static const String NUMBER_FN;
static const String ROUND_FN;
static const String CEILING_FN;
static const String FLOOR_FN;
// OG-

//-- internal XSL processor functions
static const String ERROR_FN;


}; //-- XPathNames



/**
 * The following are definitions for the XPath functions
 *
 * <PRE>
 * Modifications:
 * 20000418: Keith Visco
 *   -- added ExtensionFunctionCall
 *
 * 19990805: Keith Visco
 *   - added NodeSetFunctionCall
 *   - moved position() function into NodeSetFunctionCall
 *   - removed PositionFunctionCall
 * 19990806: Larry Fitzpatrick
 *   - changed constant short declarations for BooleanFunctionCall
 *     with enumerations
 * 19990806: Keith Visco
 *   - added StringFunctionCall
 *   - stated using Larry's enum suggestion instead of using static const shorts,
 *     as you can see, I am a Java developer! ;-)
 * </PRE>
 */

/**
 * Represents the Set of boolean functions
**/
class BooleanFunctionCall : public FunctionCall {

public:

    enum booleanFunctions { TX_BOOLEAN = 1, TX_FALSE, TX_NOT, TX_TRUE };

    /**
     * Creates a default BooleanFunctionCall, which always evaluates to False
    **/
    BooleanFunctionCall();

    /**
     * Creates a BooleanFunctionCall of the given type
    **/
    BooleanFunctionCall(short type);

    /**
     * Evaluates this Expr based on the given context node and processor state
     * @param context the context node for evaluation of this Expr
     * @param ps the ContextState containing the stack information needed
     * for evaluation
     * @return the result of the evaluation
    **/
    virtual ExprResult* evaluate(Node* context, ContextState* cs);

private:
    short type;
}; //-- BooleanFunctionCall

/**
 * Internal Function created when there is an Error during parsing
 * an Expression
**/
class ErrorFunctionCall : public FunctionCall {
public:

    ErrorFunctionCall();
    ErrorFunctionCall(const String& errorMsg);

    /**
     * Evaluates this Expr based on the given context node and processor state
     * @param context the context node for evaluation of this Expr
     * @param ps the ContextState containing the stack information needed
     * for evaluation
     * @return the result of the evaluation
    **/
    virtual ExprResult* evaluate(Node* context, ContextState* cs);

    void setErrorMessage(String& errorMsg);

private:

    String errorMessage;

}; //-- ErrorFunctionCall


/**
 * Used for extension functions
**/
class ExtensionFunctionCall : public FunctionCall {

public:

    static const String UNDEFINED_FUNCTION;

    /**
     * Creates a new ExtensionFunctionCall with the given function name
     * @param name the name of the extension function
    **/
    ExtensionFunctionCall(const String& name);

    /**
     * Destructor for extension function call
    **/
    virtual ~ExtensionFunctionCall();

    /**
     * Evaluates this Expr based on the given context node and processor state
     * @param context the context node for evaluation of this Expr
     * @param ps the ContextState containing the stack information needed
     * for evaluation
     * @return the result of the evaluation
    **/
    virtual ExprResult* evaluate(Node* context, ContextState* cs);

private:

    String fname;
    FunctionCall* fnCall;

};

/**
 * This class is used by ExtensionFunctionCall, to prevent deletion
 * of the parameter expressions, by the resolved function call. The implementation
 * for this class is in ExtensionFunctionCall.cpp
**/
class ExprWrapper : public Expr {

public:

    /**
     * Creates a new ExprWrapper for the given Expr
    **/
    ExprWrapper(Expr* expr);

    /**
     * Destructor for ExprWrapper
    **/
    virtual ~ExprWrapper();

    /**
     * Evaluates this Expr based on the given context node and processor state
     * @param context the context node for evaluation of this Expr
     * @param ps the ContextState containing the stack information needed
     * for evaluation
     * @return the result of the evaluation
    **/
    virtual ExprResult* evaluate(Node* context, ContextState* cs);

    /**
     * Returns the String representation of this Expr.
     * @param dest the String to use when creating the String
     * representation. The String representation will be appended to
     * any data in the destination String, to allow cascading calls to
     * other #toString() methods for Expressions.
     * @return the String representation of this Expr.
    **/
    virtual void toString(String& str);

private:

    Expr* expr;

}; //-- ExprWrapper



/**
 *  Represents the XPath NodeSet function calls
**/
class NodeSetFunctionCall : public FunctionCall {

public:

    enum nodeSetFunctions {
        COUNT = 1,      //-- count()
        LAST,           //-- last()
        LOCAL_NAME,     //-- local-name()
        NAMESPACE_URI,  //-- namespace-uri()
        NAME,           //-- name()
        POSITION        //-- position()
    };

    /**
     * Creates a default NodeSetFunction call. Position function is the default.
    **/
    NodeSetFunctionCall();

    /**
     * Creates a NodeSetFunctionCall of the given type
    **/
    NodeSetFunctionCall(short type);

    /**
     * Evaluates this Expr based on the given context node and processor state
     * @param context the context node for evaluation of this Expr
     * @param ps the ContextState containing the stack information needed
     * for evaluation
     * @return the result of the evaluation
    **/
    virtual ExprResult* evaluate(Node* context, ContextState* cs);

private:
    short type;
}; //-- NodeSetFunctionCall


/**
 * Represents the XPath String Function Calls
**/
class StringFunctionCall : public FunctionCall {

public:

    enum stringFunctions {
        CONCAT = 1,            //-- concat()
        CONTAINS,              //-- contains()
        NORMALIZE,             //-- normalize()
        STARTS_WITH,           //-- starts-with()
        STRING,                //-- string()
        STRING_LENGTH,         //-- string-length()
        SUBSTRING,             //-- substring()
        SUBSTRING_AFTER,       //-- substring-after()
        SUBSTRING_BEFORE,      //-- substring-before()
        TRANSLATE              //-- translate()
    };

    /**
     * Creates a default String function. String() function is the default.
    **/
    StringFunctionCall();

    /**
     * Creates a String function of the given type
    **/
    StringFunctionCall(short type);

    /**
     * Evaluates this Expr based on the given context node and processor state
     * @param context the context node for evaluation of this Expr
     * @param ps the ContextState containing the stack information needed
     * for evaluation
     * @return the result of the evaluation
    **/
    virtual ExprResult* evaluate(Node* context, ContextState* cs);

private:
    short type;
}; //-- StringFunctionCall


// OG+
/**
 * Represents the XPath Number Function Calls
**/
class NumberFunctionCall : public FunctionCall {

public:

    enum numberFunctions {
        NUMBER = 1,            //-- number()
	    ROUND,                 //-- round()
	    FLOOR,                 //-- floor()
	    CEILING                //-- ceiling()
    };

    /**
     * Creates a default Number function. number() function is the default.
    **/
    NumberFunctionCall();

    /**
     * Creates a Number function of the given type
    **/
    NumberFunctionCall(short type);

    /**
     * Evaluates this Expr based on the given context node and processor state
     * @param context the context node for evaluation of this Expr
     * @param ps the ContextState containing the stack information needed
     * for evaluation
     * @return the result of the evaluation
    **/
    virtual ExprResult* evaluate(Node* context, ContextState* cs);

private:
    short type;
}; //-- NumberFunctionCall
// OG-

#endif
