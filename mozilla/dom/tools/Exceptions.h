/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

#ifndef _Exception_h__
#define _Exception_h__

class Exception
{
private:
  char *mMessage;

public:
          Exception();
          Exception(const char *aMessage);
          Exception(Exception &aException);
          ~Exception();

  void    SetMessage(char *aMessage);
  char*   GetMessage();
};

class NotImplementedException : public Exception
{
public:
          NotImplementedException() : Exception("Function not yet implemented.") {}
          ~NotImplementedException() {}
};

class AbortParser : public Exception
{
public:
          AbortParser(char *aFileName, long aLineNumber);
          ~AbortParser() {}
};

class FileNotFoundException : public Exception
{
public:
          FileNotFoundException(char *aFileName);
          ~FileNotFoundException() {}
};

class CantOpenFileException : public Exception
{
public:
          CantOpenFileException(char *aFileName);
          ~CantOpenFileException() {}
};

class ParserException : public Exception
{
public:
          ParserException(char *aMessage) : Exception(aMessage) {}
          ~ParserException() {}
};

class InterfaceParsingException : public ParserException
{
public:
          InterfaceParsingException(char *aMessage) : ParserException(aMessage) {}
          ~InterfaceParsingException() {}
};

class EnumParsingException : public ParserException
{
public:
          EnumParsingException(char *aMessage) : ParserException(aMessage) {}
          ~EnumParsingException() {}
};

class AttributeParsingException : public ParserException
{
public:
          AttributeParsingException(char *aMessage) : ParserException(aMessage) {}
          ~AttributeParsingException() {}
};

class FunctionParsingException : public ParserException
{
public:
          FunctionParsingException(char *aMessage) : ParserException(aMessage) {}
          ~FunctionParsingException() {}
};

class ParameterParsingException : public ParserException
{
public:
          ParameterParsingException(char *aMessage) : ParserException(aMessage) {}
          ~ParameterParsingException() {}
};

class ConstParsingException : public ParserException
{
public:
          ConstParsingException(char *aMessage) : ParserException(aMessage) {}
          ~ConstParsingException() {}
};

class EndOfFileException : public Exception
{
public:
          EndOfFileException(char *aMessage) : Exception(aMessage) {}
          ~EndOfFileException() {}
};

class ScannerUnknownCharacter : public Exception
{
public:
          ScannerUnknownCharacter(char *aMessage) : Exception(aMessage) {}
          ~ScannerUnknownCharacter() {}
};

#ifdef XP_MAC
#include <ostream.h>			// required for namespace resolution
#else
class ostream;
#endif
ostream& operator<<(ostream &s, Exception &e);

#endif

