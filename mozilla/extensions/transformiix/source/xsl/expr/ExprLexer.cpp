/*
 * (C) Copyright The MITRE Corporation 1999  All rights reserved.
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * The program provided "as is" without any warranty express or
 * implied, including the warranty of non-infringement and the implied
 * warranties of merchantibility and fitness for a particular purpose.
 * The Copyright owner will not be liable for any damages suffered by
 * you as a result of using the Program. In no event will the Copyright
 * owner be liable for any special, indirect or consequential damages or
 * lost profits even if the Copyright owner has been advised of the
 * possibility of their occurrence.
 *
 * Please see release.txt distributed with this file for more information.
 *
 */

/**
 * Lexical analyzer for XPath expressions
 * @author <a href="mailto:kvisco@mitre.org">Keith Visco</a>
**/

#include <iostream.h>
#include "ExprLexer.h"

  //---------------------------/
 //- Implementation of Token -/
//---------------------------/


/**
 * Default constructor for Token
**/
Token::Token() {
    this->type =0;
} //-- Token;

/**
 * Constructor for Token
 * @param type, the type of Token being represented
**/
Token::Token(short type) {
    this->type = type;
} //-- Token;

/**
 *  Constructor for Token
 * @param value the value of this Token
 * @param type, the type of Token being represented
**/
Token::Token(const String& value, short type) {
    this->type = type;
    //-- make copy of value String
    this->value = value;
} //-- Token

Token::Token(UNICODE_CHAR uniChar, short type) {
    this->type = type;
    this->value.append(uniChar);
} //-- Token

/**
 * Copy Constructor
**/
Token::Token(const Token& token) {
    this->type = token.type;
    this->value = token.value;
} //-- Token

/**
 * Destructor for Token
**/
Token::~Token() {
    //-- currently nothing is needed
} //-- ~Token


  //--------------------------------/
 //- Implementation of ExprLexer -/
//-------------------------------/

/*
 * Complex Tokens
*/
//-- Nodetype tokens
const String ExprLexer::COMMENT = "comment";
const String ExprLexer::NODE    = "node";
const String ExprLexer::PI      = "processing-instruction";
const String ExprLexer::TEXT    = "text";

//-- multiplicative operators
const String ExprLexer::MODULUS = "mod";
const String ExprLexer::DIVIDE  = "div";

/**
 * The set of a XSL Expression Tokens
**/
const Token ExprLexer::TOKENS[] = {

    //-- Nodetype tokens
    Token(ExprLexer::COMMENT,       Token::COMMENT),
    Token(ExprLexer::NODE,          Token::NODE),
    Token(ExprLexer::PI,            Token::PI),
    Token(ExprLexer::TEXT,          Token::TEXT),
    //-- multiplicative operators
    Token(ExprLexer::MODULUS,       Token::MODULUS_OP),
    Token(ExprLexer::DIVIDE,        Token::DIVIDE_OP)
};

const short ExprLexer::NUMBER_OF_TOKENS  = 6;

  //---------------/
 //- Contructors -/
//---------------/

/**
 * Creates a new ExprLexer using the given String
**/
ExprLexer::ExprLexer(const String& pattern) {

    firstItem    = 0;
    lastItem     = 0;
    tokenCount   = 0;
    prevToken    = 0;
    parse(pattern);
    currentItem = firstItem;
} //-- ExprLexer

/**
 * Destroys this instance of an ExprLexer
**/
ExprLexer::~ExprLexer() {
   //-- delete tokens

   //cout << "~ExprLexer() - start"<<endl;
   currentItem = firstItem;
   while ( currentItem ) {
       TokenListItem* temp = currentItem->next;
       //cout << "deleting token: " << currentItem->token->value << endl;
       delete currentItem->token;
       delete currentItem;
       currentItem = temp;
   }
   //cout << "~ExprLexer() - done"<<endl;
} //-- ~ExprLexer


int ExprLexer::countAllTokens() {
    return tokenCount;
} //-- countAllTokens

int ExprLexer::countRemainingTokens() {
    TokenListItem* temp = currentItem;
    int c = 0;
    while ( temp ) {
        ++c;
        temp = temp->next;
    }
    return c;
} //-- countRemainingTokens


MBool ExprLexer::hasMoreTokens() {
    return (MBool) ( currentItem );
} //-- hasMoreTokens

Token* ExprLexer::nextToken() {
    if ( currentItem ) {
        Token* token = currentItem->token;
        currentItem = currentItem->next;
        return token;
    }
    return 0;
} //-- nextToken

void ExprLexer::pushBack() {
    if ( !currentItem ) {
        currentItem = lastItem;
    }
    else currentItem = currentItem->previous;
} //-- pushBack

/*
Token* ExprLexer::lastToken() {
    if (lastItem) {
        return lastItem->token;
    }
    return 0;
} //-- lastToken
*/

Token* ExprLexer::peek() {
    Token* token = 0;
    TokenListItem* tlItem = currentItem;
    if (tlItem) token = tlItem->token;
    return token;
} //-- peek

Token* ExprLexer::lookAhead(int offset) {
    Token* token = 0;
    TokenListItem* tlItem = currentItem;
    //-- advance to proper offset
    for ( int i = 0; i < offset; i++ )
        if ( tlItem ) tlItem = currentItem->next;

    if (tlItem) token = tlItem->token;
    return token;
} //-- lookAhead

void ExprLexer::addToken(Token* token) {
    TokenListItem* tlItem = new TokenListItem;
    tlItem->token = token;
    tlItem->next  = 0;
    if (lastItem) {
        tlItem->previous = lastItem;
        lastItem->next = tlItem;
    }
    if (!firstItem) firstItem = tlItem;
    lastItem = tlItem;
    prevToken = token;
    ++tokenCount;
} //-- addToken

/**
 * Returns true if the given character represents an Alpha letter
**/
MBool ExprLexer::isAlphaChar(Int32 ch) {
    if ((ch >= 'a' ) && (ch <= 'z' )) return MB_TRUE;
    if ((ch >= 'A' ) && (ch <= 'Z' )) return MB_TRUE;
    return MB_FALSE;
} //-- isAlphaChar

/**
 * Returns true if the given character represents a numeric letter (digit)
**/
MBool ExprLexer::isDigit(Int32 ch) {
    if ((ch >= '0') && (ch <= '9'))   return MB_TRUE;
    return MB_FALSE;
} //-- isDigit

/**
 * Returns true if the given character is an allowable QName character
**/
MBool ExprLexer::isNCNameChar(Int32 ch) {
    if (isDigit(ch) || isAlphaChar(ch)) return MB_TRUE;
    return (MBool) ((ch == '.') ||(ch == '_') || (ch == '-'));
} //-- isNCNameChar

/**
 * Returns true if the given character is an allowable NCName character
**/
MBool ExprLexer::isQNameChar(Int32 ch) {
    return (MBool) (( ch == ':') || isNCNameChar(ch));
} //-- isQNameChar

/**
 * Returns true if the given String is a valid XML QName
**/
MBool ExprLexer::isValidQName(String& name) {

    int size = name.length();
    if ( size == 0 ) return MB_FALSE;
    else if ( !isAlphaChar(name.charAt(0))) return MB_FALSE;
    else {
        for ( int i = 1; i < size; i++) {
            if ( ! isQNameChar(name.charAt(i))) return MB_FALSE;
        }
    }
    return MB_TRUE;
} //-- isValidQName

MBool ExprLexer::isOperatorToken(Token* token) {
    if ( !token ) return MB_FALSE;
    switch ( token->type ) {
        //-- relational operators
        case Token::EQUAL_OP:
        case Token::NOT_EQUAL_OP:
        case Token::LESS_THAN_OP:
        case Token::GREATER_THAN_OP:
        case Token::LESS_OR_EQUAL_OP:
        case Token::GREATER_OR_EQUAL_OP:
        //-- additive operators
        case Token::ADDITION_OP:
        case Token::SUBTRACTION_OP:
        //-- multiplicative operators
        case Token::DIVIDE_OP:
        case Token::MODULUS_OP:
        case Token::MULTIPLY_OP:
            return MB_TRUE;
        default:
            break;
    }

    return MB_FALSE;
} //-- isOperatorToken

MBool ExprLexer::matchDelimiter(UNICODE_CHAR ch) {

    short tokenType = 0;
    MBool addChar = MB_TRUE;
    switch (ch) {
        case FORWARD_SLASH :
            tokenType = Token::PARENT_OP;
            break;
        case L_PAREN :
            tokenType = Token::L_PAREN;
            break;
        case R_PAREN :
            tokenType = Token::R_PAREN;
            break;
        case L_BRACKET :
            tokenType = Token::L_BRACKET;
            break;
        case R_BRACKET :
            tokenType = Token::R_BRACKET;
            break;
        case COMMA :
            tokenType = Token::COMMA;
            break;
        case PERIOD :
            tokenType = Token::SELF_NODE;
            break;
        case EQUAL :
            tokenType = Token::EQUAL_OP;
            break;
        case PLUS :
            tokenType = Token::ADDITION_OP;
            break;
        case HYPHEN :
            tokenType = Token::SUBTRACTION_OP;
            break;
        case VERT_BAR:
            tokenType = Token::UNION_OP;
            break;
        case ASTERIX:
            tokenType = Token::WILD_CARD;
            break;
        case AT_SIGN:
            tokenType = Token::AT_SIGN;
            break;
        case DOLLAR_SIGN:
            tokenType = Token::VAR_REFERENCE;
            addChar = MB_FALSE;
            break;
        default:
            return MB_FALSE;;
    }
    Token* token = 0;
    if ( addChar ) token = new Token(ch, tokenType);
    else token = new Token(tokenType);

    addToken(token);
    return MB_TRUE;
} //-- matchDelimiter

/**
 * Returns true if the value of the given String matches
 * an OperatorName
**/
MBool ExprLexer::matchesOperator(String& buffer) {

    int index = 0;
    while (index < NUMBER_OF_TOKENS) {
        Token tok = TOKENS[index++];
        if ( tok.value.isEqual(buffer) ) {
            if (isOperatorToken( &tok )) return MB_TRUE;
        }
    }
    return MB_FALSE;

} //-- matchesOperator

/**
 * Matches the given String to the appropriate Token
 * @param buffer the current StringBuffer representing the value of the Token
 * @param ch, the current delimiter token
**/
void ExprLexer::matchToken(String& buffer, UNICODE_CHAR ch) {

    if ( buffer.length() == 0) return;

    Token* match = new Token();
    MBool foundMatch = MB_FALSE;
    int index = 0;

    //-- check previous token
    switch(prevToken->type) {
        case Token::VAR_REFERENCE :
            if ( prevToken->value.length() == 0) {
                prevToken->value.append(buffer);
                buffer.clear();
                return;
            }
            break;
        default:
            break;
    }

    //-- look for next match
    while ( !foundMatch && (index < NUMBER_OF_TOKENS) ) {

        Token tok = TOKENS[index++];

        if ( tok.value.isEqual(buffer) ) {

            foundMatch = MB_TRUE;

            switch (tok.type) {

                //-- NodeType tokens
                case Token::COMMENT:
                case Token::NODE :
                case Token::PI :
                case Token::TEXT :
                    // make sure next delimiter is '('
                    if ( ch == L_PAREN) {
                        //-- copy buffer
                        match->value = buffer;
                        //-- copy type
                        match->type = tok.type;
                    }
                    break;
               case Token::MULTIPLY_OP :
               case Token::DIVIDE_OP:
               case Token::MODULUS_OP:
                    switch ( prevToken->type ) {
                        case Token::AT_SIGN :
                        case Token::NULL_TOKEN:
                        case Token::L_PAREN:
                        case Token::L_BRACKET:
                            foundMatch = MB_FALSE;
                            break; //-- do not match
                        default:
                            if ( isOperatorToken(prevToken) ) {
                                foundMatch = MB_FALSE;
                                break; //-- do not match
                            }
                            match->value = buffer;
                            match->type = tok.type;
                    }
                    break;
               default :
                    //-- copy buffer
                    match->value = buffer;
                    match->type = tok.type;
                    break;
            }
        } //-- if equal
    } //-- while

    if (!foundMatch) {
        //-- copy buffer
        match->value = buffer;
        //-- look for function name
        if ( ch == L_PAREN) match->type = Token::FUNCTION_NAME;
        else match->type = Token::CNAME;
    }
    addToken(match);
    buffer.clear();
} //-- matchToken

/**
 *  Parses the given String into the set of Tokens
**/
void ExprLexer::parse(const String& pattern) {


    String tokenBuffer;
    MBool inLiteral = MB_FALSE;
    MBool inNumber  = MB_FALSE;

    Int32 currentPos = 0;

    UNICODE_CHAR ch = '\0';
    UNICODE_CHAR prevCh = ch;

    //-- initialize previous token, this will automatically get
    //-- deleted when it goes out of scope
    Token nullToken('\0', Token::NULL_TOKEN);

    prevToken = &nullToken;

    while (currentPos < pattern.length()) {

        prevCh = ch;
        ch = pattern.charAt(currentPos);

        if ( inLiteral ) {
            //-- look for end of literal
            if ( ch == S_QUOTE ) {
                inLiteral = MB_FALSE;
                addToken(new Token(tokenBuffer, Token::LITERAL));
                tokenBuffer.clear();
            }
            else {
                tokenBuffer.append(ch);
            }
        }
        else if ( inNumber ) {
            if (isDigit(ch) || (ch == '.')) {
                tokenBuffer.append(ch);
            }
            else {
                inNumber = MB_FALSE;
                addToken(new Token(tokenBuffer, Token::NUMBER));
                tokenBuffer.clear();
                //-- push back last char
                --currentPos;
            }
        }
        else if (isDigit(ch)) {
            if  ((tokenBuffer.length() == 0 ) || matchesOperator(tokenBuffer) ) {
                //-- match operator and free up token buffer
                matchToken(tokenBuffer, ch);
                inNumber = MB_TRUE;
            }
            else if (( tokenBuffer.length() == 1 ) && (prevCh = '-')) {
                inNumber = MB_TRUE;
            }
            tokenBuffer.append(ch);
        }
        else {
            switch (ch) {
                //-- ignore whitespace
                case SPACE:
                case TAB:
                case CR:
                case LF:
                    break;
                case S_QUOTE :
                    matchToken(tokenBuffer, ch);
                    inLiteral = MB_TRUE;
                    break;
                case PERIOD:
                    if ( inNumber ) tokenBuffer.append(ch);
                    else if ( prevToken->type == Token::SELF_NODE ) {
                        prevToken->type = Token::PARENT_NODE;
                    }
                    else matchDelimiter(ch);
                    break;
                case COLON:
                    if ( prevCh == ch) {
                        Int32 bufSize = tokenBuffer.length();
                        tokenBuffer.setLength(bufSize-1);
                        addToken(new Token(tokenBuffer, Token::AXIS_IDENTIFIER));
                        tokenBuffer.clear();
                    }
                    else tokenBuffer.append(ch);
                    break;
                case FORWARD_SLASH :
                    matchToken(tokenBuffer, ch);
                    if ( prevToken->type == Token::PARENT_OP ) {
                        prevToken->type = Token::ANCESTOR_OP;
                        prevToken->value.append(ch);
                    }
                    else matchDelimiter(ch);
                    break;
                case BANG : //-- used as previous...see EQUAL
                    break;
                case EQUAL:
                    matchToken(tokenBuffer, ch);
                    matchDelimiter(ch);
                    if ( prevCh == BANG ) {
                        prevToken->type = Token::NOT_EQUAL_OP;
                    }
                    break;
                case HYPHEN :
                    if ( isValidQName(tokenBuffer) ) tokenBuffer.append(ch);
                    else {
                        switch ( prevToken->type ) {
                            case Token::NULL_TOKEN:
                            case Token::L_PAREN:
                            case Token::L_BRACKET:
                            case Token::COMMA:
                                inNumber = MB_TRUE;
                                tokenBuffer.append(ch);
                                break;
                            default:
                                matchToken(tokenBuffer, ch);
                                matchDelimiter(ch);
                                break;
                        }
                    }
                    break;
                case ASTERIX:
                    matchToken(tokenBuffer, ch);
                    switch ( prevToken->type ) {
                        case Token::AT_SIGN :
                        case Token::NULL_TOKEN:
                        case Token::L_PAREN:
                        case Token::L_BRACKET:
                            matchDelimiter(ch);
                            break;
                        default:
                            if ( isOperatorToken(prevToken) ) {
                                matchDelimiter(ch);
                                break; //-- do not match
                            }
                            addToken( new Token(ch, Token::MULTIPLY_OP) );
                    }
                    break;
                case L_PAREN:
                case R_PAREN:
                case L_BRACKET:
                case R_BRACKET:
                case COMMA:
                case AT_SIGN :
                case PLUS:
                case DOLLAR_SIGN :
                case VERT_BAR:
                    matchToken(tokenBuffer, ch);
                    matchDelimiter(ch);
                    break;
                default:
                    switch (prevCh) {
                        case SPACE :
                        case TAB :
                        case CR :
                        case LF :
                            matchToken(tokenBuffer, ch);
                            tokenBuffer.append(ch);
                            break;
                        default:
                            tokenBuffer.append(ch);
                            break;
                    }
                    break;
            }
        }
        ++currentPos;
    }
    //-- end lexical parsing of current token
    //-- freeBuffer if needed

    if ( inNumber ) {
        addToken(new Token(tokenBuffer, Token::NUMBER));
    }
    else matchToken(tokenBuffer, ch);
    prevToken = 0;
} //-- parse

