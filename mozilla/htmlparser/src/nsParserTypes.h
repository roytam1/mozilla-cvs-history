/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

/**
 * MODULE NOTES:
 * @update  gess 4/1/98
 * 
 * This file defines some useful types to the parser.
 *
 */

#ifndef PARSER_TYPES__
#define PARSER_TYPES__

/* ===========================================================*
  Some useful constants...
 * ===========================================================*/

#include "prtypes.h"
#include "nsError.h"


const PRInt32   kEOF              = 10000;
const PRInt32   kUnknownError     = 10001;
const PRInt32   kCantPropagate    = 10002;
const PRInt32   kContextMismatch  = 10003;
const PRInt32   kBadFilename      = 10004;
const PRInt32   kBadURL           = 10005;
const PRInt32   kInterrupted      = 10006;
const PRInt32   kProcessComplete  = 10007;
const PRInt32   kNotFound         = -1;
const PRInt32   kNoError          = NS_OK;

const PRUint32  kNewLine          = '\n';
const PRUint32  kCR               = '\r';
const PRUint32  kLF               = '\n';
const PRUint32  kTab              = '\t';
const PRUint32  kSpace            = ' ';
const PRUint32  kQuote            = '"';
const PRUint32  kApostrophe       = '\'';
const PRUint32  kLessThan         = '<';
const PRUint32  kGreaterThan      = '>';
const PRUint32  kAmpersand        = '&';
const PRUint32  kForwardSlash     = '/';
const PRUint32  kBackSlash        = '\\';
const PRUint32  kEqual            = '=';
const PRUint32  kMinus            = '-';
const PRUint32  kPlus             = '+';
const PRUint32  kExclamation      = '!';
const PRUint32  kSemicolon        = ';';
const PRUint32  kHashsign         = '#';
const PRUint32  kAsterisk         = '*';
const PRUint32  kUnderbar         = '_';
const PRUint32  kComma            = ',';
const PRUint32  kLeftParen        = '(';
const PRUint32  kRightParen       = ')';
const PRUint32  kLeftBrace        = '{';
const PRUint32  kRightBrace       = '}';
const PRUint32  kQuestionMark     = '?';


#endif


