/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* 
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


/**
 * MODULE NOTES:
 * @update  gess 4/1/98
 * 
 */

#include "nsExpatTokenizer.h"
#include "nsScanner.h"
#include "nsDTDUtils.h"
#include "nsParserError.h"
#include "nsIParser.h"
#include "prlog.h"

#include "prmem.h"
#include "nsIUnicharInputStream.h"
#include "nsNetUtil.h"
#include "nsIServiceManager.h"
#include "nsCOMPtr.h"
#include "nsSpecialSystemDirectory.h"
#include "nsIURL.h"

typedef struct _XMLParserState {
  XML_Parser parser;
  nsScanner* scanner;
  const PRUnichar* bufferStart;
  const PRUnichar* bufferEnd;
  nsReadingIterator<PRUnichar> currentIterator;
  nsDeque* tokenDeque;
  nsTokenAllocator* tokenAllocator;
  nsString doctypeText;
  PRBool indoctype;
  nsString cdataText;
  PRBool incdata;
} XMLParserState;

 /************************************************************************
  And now for the main class -- nsExpatTokenizer...
 ************************************************************************/

static NS_DEFINE_IID(kHTMLTokenizerIID,   NS_HTMLTOKENIZER_IID);
static NS_DEFINE_IID(kClassIID,           NS_EXPATTOKENIZER_IID);

static const char* kDTDDirectory = "dtd/";
static const char kHTMLNameSpaceURI[] = "http://www.w3.org/1999/xhtml";

const nsIID&
nsExpatTokenizer::GetIID()
{
  return kClassIID;
}

  
const nsIID&
nsExpatTokenizer::GetCID()
{
  static NS_DEFINE_IID(kCID, NS_EXPATTOKENIZER_CID);
  return kCID;
}


/**
 *  This method gets called as part of our COM-like interfaces.
 *  Its purpose is to create an interface to parser object
 *  of some type.
 *  
 *  @update   gess 4/8/98
 *  @param    nsIID  id of object to discover
 *  @param    aInstancePtr ptr to newly discovered interface
 *  @return   NS_xxx result code
 */
nsresult nsExpatTokenizer::QueryInterface(const nsIID& aIID, void** aInstancePtr)  
{                                                                        
  if (NULL == aInstancePtr) {                                            
    return NS_ERROR_NULL_POINTER;                                        
  }                                                                      

  if(aIID.Equals(NS_GET_IID(nsISupports)))    {  //do IUnknown...
    *aInstancePtr = (nsExpatTokenizer*)(this);                                        
  }
  else if(aIID.Equals(NS_GET_IID(nsITokenizer))) {  //do ITokenizer base class...    
    *aInstancePtr = (nsITokenizer*)(this);
  }
  else if(aIID.Equals(kHTMLTokenizerIID)) {  //do nsHTMLTokenizer base class...
    *aInstancePtr = (nsHTMLTokenizer*)(this);                                        
  }
  else if(aIID.Equals(kClassIID)) {  //do this class...
    *aInstancePtr = (nsExpatTokenizer*)(this);                                        
  }                 
  else {
    *aInstancePtr=0;
    return NS_NOINTERFACE;
  }
  NS_ADDREF_THIS();
  return NS_OK;                                                        
}

NS_IMPL_ADDREF(nsExpatTokenizer)
NS_IMPL_RELEASE(nsExpatTokenizer)

/**
 * Sets up the callbacks and user data for the expat parser
 * @update  nra 2/24/99
 * @param   none
 * @return  none
 */
void nsExpatTokenizer::SetupExpatParser(void) {
  if (mExpatParser) {
    // Set up the callbacks
    XML_SetElementHandler(mExpatParser, Tokenizer_HandleStartElement, Tokenizer_HandleEndElement);    
    XML_SetCharacterDataHandler(mExpatParser, Tokenizer_HandleCharacterData);
    XML_SetProcessingInstructionHandler(mExpatParser, Tokenizer_HandleProcessingInstruction);
    XML_SetDefaultHandlerExpand(mExpatParser, Tokenizer_HandleDefault);
    XML_SetUnparsedEntityDeclHandler(mExpatParser, Tokenizer_HandleUnparsedEntityDecl);
    XML_SetNotationDeclHandler(mExpatParser, Tokenizer_HandleNotationDecl);
    XML_SetExternalEntityRefHandler(mExpatParser, Tokenizer_HandleExternalEntityRef);
    XML_SetCommentHandler(mExpatParser, Tokenizer_HandleComment);
    XML_SetUnknownEncodingHandler(mExpatParser, Tokenizer_HandleUnknownEncoding, NULL);
    XML_SetCdataSectionHandler(mExpatParser, Tokenizer_HandleStartCdataSection,
                               Tokenizer_HandleEndCdataSection);

    XML_SetDoctypeDeclHandler(mExpatParser, Tokenizer_HandleStartDoctypeDecl, Tokenizer_HandleEndDoctypeDecl);

    // Set up the user data.
    XML_SetUserData(mExpatParser, (void*) mState);
  }
}


/**
 *  Default constructor
 *   
 *  @update  gess 4/9/98
 *  @param   
 *  @return  
 */
nsExpatTokenizer::nsExpatTokenizer(nsString* aURL) : nsHTMLTokenizer() {  
  NS_INIT_REFCNT();
  mBytesParsed = 0;
  mState = new XMLParserState;
  mState->tokenAllocator = nsnull;
  mState->parser = nsnull;
  mState->tokenDeque = nsnull;
  mState->indoctype = PR_FALSE;
  mState->incdata = PR_FALSE;

  nsAutoString buffer; buffer.AssignWithConversion("UTF-16");
  const PRUnichar* encoding = buffer.get();
  if (encoding) {
    mExpatParser = XML_ParserCreate((const XML_Char*) encoding);
    if (mExpatParser) {
#ifdef XML_DTD
      XML_SetParamEntityParsing(mExpatParser, XML_PARAM_ENTITY_PARSING_ALWAYS);
#endif
      if (aURL)
        XML_SetBase(mExpatParser, (const XML_Char*) aURL->get());
      
      SetupExpatParser();
    }
  }
}

/**
 *  Destructor
 *   
 *  @update  gess 4/9/98
 *  @param   
 *  @return  
 */
nsExpatTokenizer::~nsExpatTokenizer(){
  if (mExpatParser) {    
    XML_ParserFree(mExpatParser);
    mExpatParser = nsnull;
  }
  
  if (mState)
    delete mState;
}


/*******************************************************************
  Here begins the real working methods for the tokenizer.
 *******************************************************************/

nsresult nsExpatTokenizer::WillTokenize(PRBool aIsFinalChunk,nsTokenAllocator* aTokenAllocator)
{
  mState->tokenAllocator=aTokenAllocator;
  return nsHTMLTokenizer::WillTokenize(aIsFinalChunk,aTokenAllocator);
}

/* 
 * Parameters:
 * 
 * aSourceBuffer (in): String buffer.
 * aLength (in): Length of input buffer.
 * aOffset (in): Offset in buffer
 * aLine (out): Line on which the character, aSourceBuffer[aOffset], is located.
 */
void nsExpatTokenizer::GetLine(const char* aSourceBuffer, PRUint32 aLength, 
                                PRUint32 aOffset, nsString& aLine)
{
  /* Figure out the line inside aSourceBuffer that contains character specified by aOffset.
     Copy it into aLine. */
  NS_ASSERTION(aOffset >= 0 && aOffset < aLength, "?");
  /* Assert that the byteIndex and the length of the buffer is even */
  NS_ASSERTION(aOffset % 2 == 0 && aLength % 2 == 0, "?");  
  PRUnichar* start = (PRUnichar* ) &aSourceBuffer[aOffset];  /* Will try to find the start of the line */
  PRUnichar* end = (PRUnichar* ) &aSourceBuffer[aOffset];    /* Will try to find the end of the line */
  PRUint32 startIndex = aOffset / sizeof(PRUnichar);          /* Track the position of the 'start' pointer into the buffer */
  PRUint32 endIndex = aOffset / sizeof(PRUnichar);          /* Track the position of the 'end' pointer into the buffer */
  PRUint32 numCharsInBuffer = aLength / sizeof(PRUnichar);
  PRBool reachedStart;
  PRBool reachedEnd;
  

  /* Use start to find the first new line before the error position and
     end to find the first new line after the error position */
  reachedStart = (startIndex <= 0 || '\n' == *start || '\r' == *start);
  reachedEnd = (endIndex >= numCharsInBuffer || '\n' == *end || '\r' == *end);
  while (!reachedStart || !reachedEnd) {
    if (!reachedStart) {
      start--;
      startIndex--;
      reachedStart = (startIndex <= 0 || '\n' == *start || '\r' == *start);
    }
    if (!reachedEnd) {
      end++;
      endIndex++;
      reachedEnd = (endIndex >= numCharsInBuffer || '\n' == *end || '\r' == *end);
    }
  }

  aLine.Truncate(0);
  if (startIndex == endIndex) {
    /* Special case if the error is on a line where the only character is a newline */
      // STRING USE WARNING: I have no idea what this is supposed to do; to me it looks like a no-op
      //  ... so I'm not going to delete it but I will fix it to conform to the new standard.
    // aLine.Append("");
    aLine.AppendWithConversion("");
  }
  else {
    NS_ASSERTION(endIndex - startIndex >= sizeof(PRUnichar), "?");
    /* At this point, there are two cases.  Either the error is on the first line or
       on subsequent lines.  If the error is on the first line, startIndex will decrement
       all the way to zero.  If not, startIndex will decrement to the position of the
       newline character on the previous line.  So, in the first case, the start position
       of the error line = startIndex (== 0).  In the second case, the start position of the
       error line = startIndex + 1.  In both cases, the end position of the error line will be 
       (endIndex - 1).  */
    PRUint32 startPosn = (startIndex <= 0) ? startIndex : startIndex + 1;
        
    /* At this point, the substring starting at startPosn and ending at (endIndex - 1),
       is the line on which the error occurred. Copy that substring into the error structure. */
    const PRUnichar* unicodeBuffer = (const PRUnichar*) aSourceBuffer;
    aLine.Append(&unicodeBuffer[startPosn], endIndex - startPosn);
  }
}


static nsresult 
CreateErrorText(const nsParserError* aError, nsString& aErrorString)
{
  aErrorString.AssignWithConversion("XML Parsing Error: ");

  if (aError) {
    aErrorString.Append(aError->description);
    aErrorString.AppendWithConversion("\nLocation: ");
    aErrorString.Append(aError->sourceURL);
    aErrorString.AppendWithConversion("\nLine Number ");
    aErrorString.AppendInt(aError->lineNumber, 10);
    aErrorString.AppendWithConversion(", Column ");
    aErrorString.AppendInt(aError->colNumber, 10);
    aErrorString.AppendWithConversion(":");
  }

  return NS_OK;
}

static nsresult 
CreateSourceText(const nsParserError* aError, nsString& aSourceString)
{  
  PRInt32 errorPosition = aError->colNumber;

  aSourceString.Append(aError->sourceLine);
  aSourceString.AppendWithConversion("\n");
  for (PRInt32 i = 0; i < errorPosition - 1; i++)
    aSourceString.AppendWithConversion("-");
  aSourceString.AppendWithConversion("^");  

  return NS_OK;
}

/* Create and add the tokens in the following order to display the error:
   ParserError start token
     Text token containing error message
     SourceText start token
       Text token containing source text
     SourceText end token
   ParserError end token
*/
nsresult
nsExpatTokenizer::AddErrorMessageTokens(nsParserError* aError)
{   
  nsresult rv = NS_OK;
  CToken* newToken = mState->tokenAllocator->CreateTokenOfType(eToken_start, eHTMLTag_parsererror);
  AddToken(newToken, NS_OK, mState->tokenDeque, mState->tokenAllocator);

  CAttributeToken* attrToken = (CAttributeToken*) 
      mState->tokenAllocator->CreateTokenOfType(eToken_attribute, eHTMLTag_unknown, NS_ConvertASCIItoUCS2(kHTMLNameSpaceURI));  
  attrToken->SetKey(NS_LITERAL_STRING("xmlns"));
  newToken->SetAttributeCount(1);
  newToken = (CToken*) attrToken;
  AddToken(newToken, NS_OK, mState->tokenDeque, mState->tokenAllocator);
  
  nsAutoString textStr;
  CreateErrorText(aError, textStr);
  newToken = mState->tokenAllocator->CreateTokenOfType(eToken_text, eHTMLTag_unknown, textStr);
  AddToken(newToken, NS_OK, mState->tokenDeque, mState->tokenAllocator);

  newToken = mState->tokenAllocator->CreateTokenOfType(eToken_start, eHTMLTag_sourcetext);
  AddToken(newToken, NS_OK, mState->tokenDeque, mState->tokenAllocator);  
  
  textStr.Truncate();
  CreateSourceText(aError, textStr);
  newToken = mState->tokenAllocator->CreateTokenOfType(eToken_text, eHTMLTag_unknown,textStr);      
  AddToken(newToken, NS_OK, mState->tokenDeque, mState->tokenAllocator);  

  newToken = mState->tokenAllocator->CreateTokenOfType(eToken_end, eHTMLTag_sourcetext);  
  AddToken(newToken, NS_OK, mState->tokenDeque, mState->tokenAllocator);

  newToken = mState->tokenAllocator->CreateTokenOfType(eToken_end, eHTMLTag_parsererror);  
  AddToken(newToken, NS_OK, mState->tokenDeque, mState->tokenAllocator);
  
  return rv;
}

/* 
 * Called immediately after an error has occurred in expat.  Creates
 * tokens to display the error and an error token to the token stream.
 *
 * The error tokens will end up creating the following content model
 * in the content sink:
 *
 *    <ParserError>
 *       XML Error: "contents of aError->description"
 *       Line Number: "contents of aError->lineNumber"
 *       <SourceText>
 *          "Contents of aError->sourceLine"
 *          "^ pointing at the error location"
 *       </SourceText>
 *    </ParserError>
 *
 */
nsresult
nsExpatTokenizer::PushXMLErrorTokens(const char *aBuffer, PRUint32 aLength, PRBool aIsFinal)
{
  CErrorToken* errorToken= (CErrorToken *) mState->tokenAllocator->CreateTokenOfType(eToken_error, eHTMLTag_unknown);
  nsParserError *error = new nsParserError;
  nsresult rv = NS_OK;
  
  if (error && errorToken) {  
    /* Fill in the values of the error token */
    error->code = XML_GetErrorCode(mExpatParser);
    error->lineNumber = XML_GetCurrentLineNumber(mExpatParser);
    // Adjust the column number so that it is one based rather than zero based.
    error->colNumber = XML_GetCurrentColumnNumber(mExpatParser) + 1;
    error->description.AssignWithConversion(XML_ErrorString(error->code));
    error->sourceURL.Assign((PRUnichar*)XML_GetBase(mExpatParser));
    if (!aIsFinal) {
      PRInt32 byteIndexRelativeToFile = 0;
      byteIndexRelativeToFile = XML_GetCurrentByteIndex(mExpatParser);
      GetLine(aBuffer, aLength, (byteIndexRelativeToFile - mBytesParsed), error->sourceLine);
    }
    else {
      error->sourceLine.Append(mLastLine);
    }

    errorToken->SetError(error);

 
    /* Add the error token */
    CToken* newToken = (CToken*) errorToken;
    AddToken(newToken, NS_OK, mState->tokenDeque, mState->tokenAllocator);

    /* Add the error message tokens */
    AddErrorMessageTokens(error);
  }

  return rv;
}

nsresult nsExpatTokenizer::ParseXMLBuffer(const char* aBuffer, PRUint32 aLength, PRBool aIsFinal)
{
  nsresult result=NS_OK;
  NS_ASSERTION((aBuffer && aLength) || (aBuffer == nsnull && aLength == 0), "?");
  if (mExpatParser) {

    nsCOMPtr<nsExpatTokenizer> me=this;

    if (!XML_Parse(mExpatParser, aBuffer, aLength, aIsFinal)) {
      PushXMLErrorTokens(aBuffer, aLength, aIsFinal);
      result=NS_ERROR_HTMLPARSER_STOPPARSING;
    }
    else if (aBuffer && aLength) {
      // Cache the last line in the buffer
      GetLine(aBuffer, aLength, aLength - sizeof(PRUnichar), mLastLine);
    }
	  mBytesParsed += aLength;    
  }
  else {
    result = NS_ERROR_FAILURE;
  }
  return result;
}


/**
 *  This method repeatedly called by the tokenizer. 
 *  Each time, we determine the kind of token were about to 
 *  read, and then we call the appropriate method to handle
 *  that token type.
 *  
 *  @update gess 3/25/98
 *  @param  aChar: last char read
 *  @param  aScanner: see nsScanner.h
 *  @param  anErrorCode: arg that will hold error condition
 *  @return new token or null 
 */
nsresult nsExpatTokenizer::ConsumeToken(nsScanner& aScanner,PRBool& aFlushTokens) {
  
  // return nsHTMLTokenizer::ConsumeToken(aScanner);

  // Ask the scanner to send us all the data it has
  // scanned and pass that data to expat.
  nsresult result = NS_OK;
  nsReadingIterator<PRUnichar> start, end;
  aScanner.CurrentPosition(start);
  aScanner.EndReading(end);
  mState->tokenDeque = &mTokenDeque;
  mState->parser = mExpatParser;
  mState->scanner = &aScanner;

  while (start != end) {
    PRUint32 fragLength = PRUint32(start.size_forward());
    PRUint32 bufLength = fragLength * sizeof(PRUnichar);
    const PRUnichar* expatBuffer = start.get();

    mState->bufferStart = expatBuffer;
    mState->bufferEnd = expatBuffer + fragLength;
    mState->currentIterator = start;
    result = ParseXMLBuffer((const char *)expatBuffer, bufLength);
    if (NS_FAILED(result)) return result;

    start.advance(fragLength);
  }

  aScanner.SetPosition(end, PR_TRUE);
  
  if(NS_OK==result)
    result=aScanner.Eof();

  mState->scanner = nsnull;
  mState->bufferStart = mState->bufferEnd = nsnull;

  return result;
}

nsresult nsExpatTokenizer::DidTokenize(PRBool aIsFinalChunk)
{
  return ParseXMLBuffer(nsnull, 0, aIsFinalChunk);
}

/**
 * 
 * @update	gess12/29/98
 * @param 
 * @return
 */
void nsExpatTokenizer::FrontloadMisplacedContent(nsDeque& aDeque){
}

/***************************************/
/* Expat Callback Functions start here */
/***************************************/

void Tokenizer_HandleStartElement(void *userData, const XML_Char *name, const XML_Char **atts) {
  XMLParserState* state = (XMLParserState*) userData;

  typedef const PRUnichar* const_PRUnichar_ptr;
  CToken* theToken = state->tokenAllocator->CreateTokenOfType(eToken_start,eHTMLTag_unknown, nsDependentString(const_PRUnichar_ptr(name)));
  if(theToken) {    
    // If an ID attribute exists for this element, set it on the start token
    PRInt32 index = XML_GetIdAttributeIndex(state->parser);
    if (index >= 0) {      
      nsCOMPtr<nsIAtom> attributeAtom = dont_AddRef(NS_NewAtom((const PRUnichar *) atts[index]));
      CStartToken* startToken = NS_STATIC_CAST(CStartToken*, theToken);
      startToken->SetIDAttributeAtom(attributeAtom);
    }

    nsExpatTokenizer::AddToken(theToken, NS_OK, state->tokenDeque, state->tokenAllocator);

    // For each attribute on this element, create and add attribute tokens to the token queue
    int theAttrCount=0;    
    while(*atts){
      theAttrCount++;
      CAttributeToken* theAttrToken = (CAttributeToken*) 
      state->tokenAllocator->CreateTokenOfType(eToken_attribute, eHTMLTag_unknown, nsDependentString(const_PRUnichar_ptr(atts[1])));
      if(theAttrToken){
        PRUnichar* ptr = (PRUnichar*)atts[0];
        if ((ptr >= state->bufferStart) && (ptr < state->bufferEnd)) {
          PRUint32 len = nsCRT::strlen(ptr);
          nsReadingIterator<PRUnichar> start, end;
          start = state->currentIterator;
          start.advance(ptr - state->bufferStart);
          end = start;
          end.advance(len);
          theAttrToken->BindKey(state->scanner, start, end);
        }
        else {
          theAttrToken->SetKey(nsDependentString(ptr));
        }
      }
      CToken* theTok=(CToken*)theAttrToken;
      nsExpatTokenizer::AddToken(theTok, NS_OK, state->tokenDeque, state->tokenAllocator);
      atts += 2;
    }
    theToken->SetAttributeCount(theAttrCount);
  }
  else{
    //THROW A HUGE ERROR IF WE CANT CREATE A TOKEN!
  }
}

void Tokenizer_HandleEndElement(void *userData, const XML_Char *name) {
  XMLParserState* state = (XMLParserState*) userData;
  typedef const PRUnichar* const_PRUnichar_ptr;
  CToken* theToken = state->tokenAllocator->CreateTokenOfType(eToken_end,eHTMLTag_unknown, nsDependentString(const_PRUnichar_ptr(name)));
  if(theToken) {
    nsExpatTokenizer::AddToken(theToken, NS_OK, state->tokenDeque, state->tokenAllocator);
  }
  else{
    //THROW A HUGE ERROR IF WE CANT CREATE A TOKEN!
  }
}

void Tokenizer_HandleCharacterData(void *userData, const XML_Char *s, int len) { 
  XMLParserState* state = (XMLParserState*) userData;

  if (state->incdata) {    
    // While we're in a CDATASection, keep appending all strings
    // from expat into it.
    state->cdataText.Append((PRUnichar *) s,len);
  } else {
    CToken* newToken = 0;

    switch(((PRUnichar*)s)[0]){
      case kNewLine:
      case nsCRT::CR:
        newToken = state->tokenAllocator->CreateTokenOfType(eToken_newline,eHTMLTag_unknown); 
        break;
      case kSpace:
      case kTab:
        newToken = state->tokenAllocator->CreateTokenOfType(eToken_whitespace,eHTMLTag_unknown, nsDependentString((PRUnichar*)s, len)); 
        break;
      default:
        {
          CTextToken* textToken = (CTextToken*)state->tokenAllocator->CreateTokenOfType(eToken_text, eHTMLTag_unknown);
          PRUnichar* ptr = (PRUnichar*)s;
          if ((ptr >= state->bufferStart) && (ptr < state->bufferEnd)) {
            nsReadingIterator<PRUnichar> start, end;
            start = state->currentIterator;
            start.advance(ptr - state->bufferStart);
            end = start;
            end.advance(len);
            textToken->Bind(state->scanner, start, end);
          }
          else {
            textToken->Bind(nsDependentString(ptr, len));
          }
          newToken = textToken;
        }
    }
    
    if(newToken) {
      nsExpatTokenizer::AddToken(newToken, NS_OK, state->tokenDeque, state->tokenAllocator);
    }
    else {
      //THROW A HUGE ERROR IF WE CANT CREATE A TOKEN!
    }
  }  
}

void Tokenizer_HandleComment(void *userData, const XML_Char *name) {
  XMLParserState* state = (XMLParserState*) userData;
  if (state->indoctype) {
    // We do not want comments popping out of the doctype...
    state->doctypeText.Append(NS_LITERAL_STRING("<!--"));
    state->doctypeText.Append((PRUnichar*)name);
    state->doctypeText.Append(NS_LITERAL_STRING("-->"));
  } else {
    typedef const PRUnichar* const_PRUnichar_ptr;
    CToken* theToken = state->tokenAllocator->CreateTokenOfType(eToken_comment, eHTMLTag_unknown, nsDependentString(const_PRUnichar_ptr(name)));
    if(theToken) {
      nsExpatTokenizer::AddToken(theToken, NS_OK, state->tokenDeque, state->tokenAllocator);
    }
    else{
      //THROW A HUGE ERROR IF WE CANT CREATE A TOKEN!
    }
  }
}

void Tokenizer_HandleStartCdataSection(void *userData) {
  XMLParserState* state = (XMLParserState*) userData;

  state->incdata = PR_TRUE;
}

void Tokenizer_HandleEndCdataSection(void *userData) {
  XMLParserState* state = (XMLParserState*) userData;
  CToken* cdataToken = state->tokenAllocator->CreateTokenOfType(eToken_cdatasection,
                                                                eHTMLTag_unknown,
                                                                state->cdataText);

  // We've reached the end of the current CDATA section. Push the current
  // CDATA token onto the token queue 
  nsExpatTokenizer::AddToken(cdataToken, NS_OK, state->tokenDeque, state->tokenAllocator);

  state->incdata = PR_FALSE;
  state->cdataText.Truncate();
}

void Tokenizer_HandleProcessingInstruction(void *userData, 
                                           const XML_Char *target, 
                                           const XML_Char *data) 
{
  XMLParserState* state = (XMLParserState*) userData;
  nsAutoString theString;
  theString. AppendWithConversion("<?");
  theString.Append((PRUnichar *) target);
  if(data) {
    theString.AppendWithConversion(" ");
    theString.Append((PRUnichar *) data);
  }
  theString.AppendWithConversion("?>");
  
  CToken* theToken = state->tokenAllocator->CreateTokenOfType(eToken_instruction,eHTMLTag_unknown, theString);
  if(theToken) {
    nsExpatTokenizer::AddToken(theToken, NS_OK, state->tokenDeque, state->tokenAllocator);
  }
  else{
    //THROW A HUGE ERROR IF WE CANT CREATE A TOKEN!
  }
}

void Tokenizer_HandleDefault(void *userData, const XML_Char *s, int len) {
  XMLParserState* state = (XMLParserState*) userData;
  if (state->indoctype) {
    state->doctypeText.Append((PRUnichar*)s, len);
  }
  else {
    nsAutoString str((PRUnichar *)s, len);
    PRInt32 offset = -1;
    CToken* newLine = 0;
    
    while ((offset = str.FindChar('\n', PR_FALSE, offset + 1)) != -1) {
      newLine = state->tokenAllocator->CreateTokenOfType(eToken_newline, eHTMLTag_unknown);
      nsExpatTokenizer::AddToken(newLine, NS_OK, state->tokenDeque, state->tokenAllocator);
    }
  }
}

void Tokenizer_HandleUnparsedEntityDecl(void *userData, 
                                        const XML_Char *entityName, 
                                        const XML_Char *base, 
                                        const XML_Char *systemId, 
                                        const XML_Char *publicId,
                                        const XML_Char *notationName) {
  NS_NOTYETIMPLEMENTED("Error: Tokenizer_HandleUnparsedEntityDecl() not yet implemented.");
}


// aDTD is an in/out parameter.  Returns true if the aDTD is a chrome url or if the
// filename contained within the url exists in the special DTD directory ("dtd"
// relative to the current process directory).  For the latter case, aDTD is set
// to the file: url that points to the DTD file found in the local DTD directory.
static PRBool
IsLoadableDTD(nsCOMPtr<nsIURI>* aDTD)
{
  PRBool isLoadable = PR_FALSE;
  nsresult res = NS_OK;

  if (!aDTD || !*aDTD) {
    NS_ASSERTION(0, "Null parameter.");
    return PR_FALSE;
  }

  // Return true if the url is a chrome url
  res = (*aDTD)->SchemeIs("chrome", &isLoadable);

  // If the url is not a chrome url, check to see if a DTD file of the same name
  // exists in the special DTD directory
  if (!isLoadable) {   
    nsCOMPtr<nsIURL> dtdURL;
    dtdURL = do_QueryInterface(*aDTD, &res);
    if (NS_SUCCEEDED(res)) {
      char* fileName = nsnull;    
      res = dtdURL->GetFileName(&fileName);
      if (NS_SUCCEEDED(res) && nsnull != fileName) {
        nsSpecialSystemDirectory dtdPath(nsSpecialSystemDirectory::OS_CurrentProcessDirectory);
        nsString path; path.AssignWithConversion(kDTDDirectory);        
        path.AppendWithConversion(fileName);
        dtdPath += path;
        if (dtdPath.Exists()) {
          // The DTD was found in the local DTD directory.
          // Set aDTD to a file: url pointing to the local DTD
          nsFileURL dtdFile(dtdPath);
          nsCOMPtr<nsIURI> dtdURI;
          res = NS_NewURI(getter_AddRefs(dtdURI), dtdFile.GetURLString());
          if (NS_SUCCEEDED(res) && nsnull != dtdURI) {
            *aDTD = dtdURI;
            isLoadable = PR_TRUE;
          }
        }
        nsCRT::free(fileName);
      }
    }
  }  

  return isLoadable;
}

nsresult
nsExpatTokenizer::OpenInputStream(const nsString& aURLStr, 
                                  const nsString& aBaseURL, 
                                  nsIInputStream** in, 
                                  nsString* aAbsURL) 
{
  nsresult rv;
  nsCOMPtr<nsIURI> baseURI;  
  rv = NS_NewURI(getter_AddRefs(baseURI), aBaseURL);
  if (NS_SUCCEEDED(rv) && nsnull != baseURI) {
    nsCOMPtr<nsIURI> uri;
    rv = NS_NewURI(getter_AddRefs(uri), aURLStr, baseURI);
    if (NS_SUCCEEDED(rv) && uri) {
      if (IsLoadableDTD(address_of(uri))) {
        rv = NS_OpenURI(in, uri);
        char* absURL = nsnull;
        uri->GetSpec(&absURL);
        aAbsURL->AppendWithConversion(absURL);
        nsCRT::free(absURL);
      } 
      else {
        rv = NS_ERROR_NOT_IMPLEMENTED;
      }
    }    
  }
  return rv;
}

nsresult nsExpatTokenizer::LoadStream(nsIInputStream* in, 
                                      PRUnichar*& uniBuf, 
                                      PRUint32& retLen)
{
  // read it
  PRUint32               aCount = 1024,
                         bufsize = aCount*sizeof(PRUnichar);  
  nsIUnicharInputStream *uniIn = nsnull;
  nsAutoString utf8; utf8.AssignWithConversion("UTF-8");

  nsresult res = NS_NewConverterStream(&uniIn,
                                       nsnull,
                                       in,
                                       aCount,
                                       &utf8);
  if (NS_FAILED(res)) return res;

  PRUint32 aReadCount = 0;
  PRUnichar             *aBuf = (PRUnichar *) PR_Malloc(bufsize);

  while (NS_OK == (res=uniIn->Read(aBuf, retLen, aCount, &aReadCount))
         && aReadCount != 0) {
    retLen += aReadCount;
#if 1
    bufsize += aCount * sizeof(PRUnichar);
    aBuf = (PRUnichar *) PR_Realloc(aBuf, bufsize);
#else
    if (((aReadCount+32) >= aCount) &&
        ((retLen+aCount) * sizeof(PRUnichar) >= bufsize)) {

      bufsize += aCount * sizeof(PRUnichar);
      uniBuf = (PRUnichar *) PR_Realloc(uniBuf, bufsize*sizeof(PRUnichar));
    }
#endif
  }/* while */
  uniBuf = (PRUnichar *) PR_Malloc(retLen*sizeof(PRUnichar));
  nsCRT::memcpy(uniBuf, aBuf, sizeof(PRUnichar) * retLen);
  PR_FREEIF(aBuf);      
  NS_RELEASE(uniIn);

  return res;
}

void Tokenizer_HandleNotationDecl(void *userData,
                                  const XML_Char *notationName,
                                  const XML_Char *base,
                                  const XML_Char *systemId,
                                  const XML_Char *publicId){
  NS_NOTYETIMPLEMENTED("Error: Tokenizer_HandleNotationDecl() not yet implemented.");
}

int Tokenizer_HandleExternalEntityRef(XML_Parser parser,
                                      const XML_Char *openEntityNames,
                                      const XML_Char *base,
                                      const XML_Char *systemId,
                                      const XML_Char *publicId)
{
  int result = PR_TRUE;

#ifdef XML_DTD
  // Load the external entity into a buffer
  nsCOMPtr<nsIInputStream> in = nsnull;
  nsAutoString urlSpec( (const PRUnichar*) systemId );
  nsAutoString baseURL( (const PRUnichar*) base );
  nsAutoString absURL;
  nsresult rv = NS_OK;
  PRUnichar *uniBuf = nsnull;
  PRUint32 retLen = 0;
  
#ifdef MOZ_SVG
  // yuck. I don't know of any other way to do this, though, since we don't
  // read external dtd's, and we need the #FIXED xmlns attribute, so we
  // can't do this later based on what namespace we're in - bbaetz

  // The alternative is remapping the urlSpec, and installing an svg.dtd file
  // in the dtd directory. This is simpler, for now.
  
  // XXX - need to do this for the other #FIXED attribuues as well
  NS_NAMED_LITERAL_STRING(svgDtd,
                          "<!ATTLIST svg xmlns CDATA #FIXED \"http://www.w3.org/2000/svg\" >");
  
#define svgPublicIdPrefix "-//W3C//DTD SVG "
  
  if (publicId && !nsCRT::strncmp((const PRUnichar*)publicId,
                                  svgPublicIdPrefix,
                                  sizeof(svgPublicIdPrefix)-1)) {
    uniBuf = ToNewUnicode(svgDtd);
    retLen = svgDtd.Length();
  } else {
#endif      
    rv = nsExpatTokenizer::OpenInputStream(urlSpec, baseURL, getter_AddRefs(in), &absURL);
    
    if (NS_SUCCEEDED(rv) && nsnull != in) {
      rv = nsExpatTokenizer::LoadStream(in, uniBuf, retLen);
    }
#ifdef MOZ_SVG
  }
#endif

  // Pass the buffer to expat for parsing
  if (NS_SUCCEEDED(rv) && nsnull != uniBuf) {    
    // Create a parser for parsing the external entity
    nsAutoString encoding; encoding.AssignWithConversion("UTF-16");  
    XML_Parser entParser = nsnull;
    
    entParser = XML_ExternalEntityParserCreate(parser, 0, 
                                               (const XML_Char*) encoding.get());
    
    if (nsnull != entParser) {
      XML_SetBase(entParser, (const XML_Char*) absURL.get());
      result = XML_Parse(entParser, (char *)uniBuf,  retLen * sizeof(PRUnichar), 1);
      XML_ParserFree(entParser);
    }
    
    PR_FREEIF(uniBuf);
  }
#else /* ! XML_DTD */

  NS_NOTYETIMPLEMENTED("Error: Tokenizer_HandleExternalEntityRef() not yet implemented.");

#endif /* XML_DTD */

  return result;
}

int Tokenizer_HandleUnknownEncoding(void *encodingHandlerData,
                                    const XML_Char *name,
                                    XML_Encoding *info) {
  NS_NOTYETIMPLEMENTED("Error: Tokenizer_HandleUnknownEncoding() not yet implemented.");
  int result=0;
  return result;
}

void Tokenizer_HandleStartDoctypeDecl(void *userData, 
                                      const XML_Char *doctypeName)
{  
  XMLParserState* state = (XMLParserState*) userData;
  state->indoctype = PR_TRUE;
  state->doctypeText.Assign(NS_LITERAL_STRING("<!DOCTYPE "));
}

void Tokenizer_HandleEndDoctypeDecl(void *userData)
{
  XMLParserState* state = (XMLParserState*) userData;

  state->doctypeText.AppendWithConversion(">");
  CToken* token = state->tokenAllocator->CreateTokenOfType(eToken_doctypeDecl, eHTMLTag_unknown, state->doctypeText);
  if (token) {
    nsExpatTokenizer::AddToken(token, NS_OK, state->tokenDeque, state->tokenAllocator);
  }
  state->indoctype = PR_FALSE;
  state->doctypeText.Truncate();
  // Do nothing
}
