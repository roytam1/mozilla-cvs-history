
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

#include <ctype.h> 
#include <time.h>
#include <stdio.h>  
#include "nsScanner.h"
#include "nsToken.h" 
#include "nsHTMLTokens.h"
#include "nsIParser.h"
#include "prtypes.h"
#include "nsDebug.h"
#include "nsHTMLTags.h"
#include "nsHTMLEntities.h"
#include "nsCRT.h"



static const char*  gUserdefined = "userdefined";

const PRInt32 kMAXNAMELEN=10;


/**************************************************************
  And now for the token classes...
 **************************************************************/

/*
 *  default constructor
 *  
 *  @update  gess 3/25/98
 *  @param   
 *  @return  
 */
CHTMLToken::CHTMLToken(const nsString& aName,eHTMLTags aTag) : CToken(aName) {
  mTypeID=aTag;
}

/*
 *  constructor from tag id
 *  
 *  @update  gess 3/25/98
 *  @param   
 *  @return  
 */
CHTMLToken::CHTMLToken(eHTMLTags aTag) : CToken(aTag) {

}

/**
 * Setter method that changes the string value of this token
 * @update	gess5/11/98
 * @param   name is a char* value containing new string value
 */
void CHTMLToken::SetStringValue(const char* name){
  if(name) {
    mTextValue=name;
    mTypeID = nsHTMLTags::LookupTag(mTextValue);
  }
}

/*
 *  constructor from tag id
 *  
 *  @update  gess 3/25/98
 *  @param   
 *  @return  
 */
CStartToken::CStartToken(eHTMLTags aTag) : CHTMLToken(aTag) {
  mAttributed=PR_FALSE;
  mEmpty=PR_FALSE;
}

/*
 *  constructor from tag id
 *  
 *  @update  gess 3/25/98
 *  @param   
 *  @return  
 */
CStartToken::CStartToken(nsString& aString,eHTMLTags aTag) : CHTMLToken(aString,aTag) {
  mAttributed=PR_FALSE;
  mEmpty=PR_FALSE;
}


/**
 * 
 * @update	gess8/4/98
 * @param 
 * @return
 */
void CStartToken::Reinitialize(PRInt32 aTag, const nsString& aString){
  CToken::Reinitialize(aTag,aString);
  mAttributed=PR_FALSE;
  mEmpty=PR_FALSE;
}

/*
 *  This method returns the typeid (the tag type) for this token.
 *  
 *  @update  gess 3/25/98
 *  @param   
 *  @return  
 */
PRInt32 CStartToken::GetTypeID(){
  if(eHTMLTag_unknown==mTypeID) {
    mTypeID = nsHTMLTags::LookupTag(mTextValue);
  }
  return mTypeID;
}

/*
 *  
 *  
 *  @update  gess 3/25/98
 *  @param   
 *  @return  
 */
const char*  CStartToken::GetClassName(void) {
  return "start";
}

/*
 *  
 *  
 *  @update  gess 3/25/98
 *  @param   
 *  @return  
 */
PRInt32 CStartToken::GetTokenType(void) {
  return eToken_start;
}

/*
 *  
 *  
 *  @update  gess 3/25/98
 *  @param   
 *  @return  
 */
void CStartToken::SetAttributed(PRBool aValue) {
  mAttributed=aValue;
}

/*
 *  
 *  
 *  @update  gess 3/25/98
 *  @param   
 *  @return  
 */
PRBool CStartToken::IsAttributed(void) {
  return mAttributed;
}

/*
 *  
 *  
 *  @update  gess 3/25/98
 *  @param   
 *  @return  
 */
void CStartToken::SetEmpty(PRBool aValue) {
  mEmpty=aValue;
}

/*
 *  
 *  
 *  @update  gess 3/25/98
 *  @param   
 *  @return  
 */
PRBool CStartToken::IsEmpty(void) {
  return mEmpty;
}


/*
 *  Consume the identifier portion of the start tag
 *  
 *  @update  gess 3/25/98
 *  @param   aChar -- last char consumed from stream
 *  @param   aScanner -- controller of underlying input source
 *  @return  error result
 */
nsresult CStartToken::Consume(PRUnichar aChar, nsScanner& aScanner) {

  //if you're here, we've already Consumed the < char, and are
   //ready to Consume the rest of the open tag identifier.
   //Stop consuming as soon as you see a space or a '>'.
   //NOTE: We don't Consume the tag attributes here, nor do we eat the ">"

  mTextValue=aChar;
  nsresult result=aScanner.ReadIdentifier(mTextValue);
  mTypeID = nsHTMLTags::LookupTag(mTextValue);

   //Good. Now, let's skip whitespace after the identifier,
   //and see if the next char is ">". If so, we have a complete
   //tag without attributes.
  if(NS_OK==result) {  
    result=aScanner.SkipWhitespace();
    if(NS_OK==result) {
      result=aScanner.GetChar(aChar);
      if(NS_OK==result) {
        if(kGreaterThan!=aChar) { //look for '>' 
         //push that char back, since we apparently have attributes...
          result=aScanner.PutBack(aChar);
          mAttributed=PR_TRUE;
        } //if
      } //if
    }//if
  }
  return result;
}


/*
 *  Dump contents of this token to givne output stream
 *  
 *  @update  gess 3/25/98
 *  @param   out -- ostream to output content
 *  @return  
 */
void CStartToken::DebugDumpSource(nsOutputStream& out) {
  char buffer[1000];
  mTextValue.ToCString(buffer,sizeof(buffer));
  out << "<" << buffer;
  if(!mAttributed)
    out << ">";
}

/*
 *  
 *  
 *  @update  gess 3/25/98
 *  @param   anOutputString will recieve the result
 *  @return  nada
 */
void CStartToken::GetSource(nsString& anOutputString){
  anOutputString="<";
  anOutputString+=mTextValue;
  if(!mAttributed)
    anOutputString+=">";
}

/*
 *  constructor from tag id
 *  
 *  @update  gess 3/25/98
 *  @param   
 *  @return  
 */
CEndToken::CEndToken(eHTMLTags aTag) : CHTMLToken(aTag) {
  SetStringValue(GetTagName(aTag));
}


/*
 *  default constructor for end token
 *  
 *  @update  gess 3/25/98
 *  @param   aName -- char* containing token name
 *  @return  
 */
CEndToken::CEndToken(const nsString& aName) : CHTMLToken(aName) {
}

/*
 *  Consume the identifier portion of the end tag
 *  
 *  @update  gess 3/25/98
 *  @param   aChar -- last char consumed from stream
 *  @param   aScanner -- controller of underlying input source
 *  @return  error result
 */
nsresult CEndToken::Consume(PRUnichar aChar, nsScanner& aScanner) {
  //if you're here, we've already Consumed the <! chars, and are
   //ready to Consume the rest of the open tag identifier.
   //Stop consuming as soon as you see a space or a '>'.
   //NOTE: We don't Consume the tag attributes here, nor do we eat the ">"

  mTextValue="";
  nsresult result=aScanner.ReadUntil(mTextValue,kGreaterThan,PR_FALSE);

  if(NS_OK==result){

    PRInt32 theIndex=mTextValue.FindCharInSet(" \r\n\t\b",0);
    nsAutoString  buffer(mTextValue);
    buffer.Truncate(theIndex);
    mTypeID= nsHTMLTags::LookupTag(buffer);
    result=aScanner.GetChar(aChar); //eat the closing '>;
  }
  return result;
}


/*
 *  Asks the token to determine the <i>HTMLTag type</i> of
 *  the token. This turns around and looks up the tag name
 *  in the tag dictionary.
 *  
 *  @update  gess 3/25/98
 *  @param   
 *  @return  eHTMLTag id of this endtag
 */
PRInt32 CEndToken::GetTypeID(){
  if(eHTMLTag_unknown==mTypeID) {
    mTypeID = nsHTMLTags::LookupTag(mTextValue);
    switch(mTypeID) {
      case eHTMLTag_dir:
      case eHTMLTag_menu:
        mTypeID=eHTMLTag_ul;
        break;
      default:
        break;
    }
  }
  return mTypeID;
}

/*
 *  
 *  
 *  @update  gess 3/25/98
 *  @param   
 *  @return  
 */
const char*  CEndToken::GetClassName(void) {
  return "/end";
}

/*
 *  
 *  
 *  @update  gess 3/25/98
 *  @param   
 *  @return  
 */
PRInt32 CEndToken::GetTokenType(void) {
  return eToken_end;
}

/*
 *  Dump contents of this token to givne output stream
 *  
 *  @update  gess 3/25/98
 *  @param   out -- ostream to output content
 *  @return  
 */
void CEndToken::DebugDumpSource(nsOutputStream& out) {
  char buffer[1000];
  mTextValue.ToCString(buffer,sizeof(buffer));
  out << "</" << buffer << ">";
}

/*
 *  
 *  
 *  @update  gess 3/25/98
 *  @param   anOutputString will recieve the result
 *  @return  nada
 */
void CEndToken::GetSource(nsString& anOutputString){
  anOutputString="</";
  anOutputString+=mTextValue;
  anOutputString+=">";
}

/*
 *  default constructor
 *  
 *  @update  gess 3/25/98
 *  @param   aName -- string to init token name with
 *  @return  
 */
CTextToken::CTextToken() : CHTMLToken(eHTMLTag_text) {
}


/*
 *  string based constructor
 *  
 *  @update  gess 3/25/98
 *  @param   aName -- string to init token name with
 *  @return  
 */
CTextToken::CTextToken(const nsString& aName) : CHTMLToken(aName) {
  mTypeID=eHTMLTag_text;
}

/*
 *  
 *  
 *  @update  gess 3/25/98
 *  @param   
 *  @return  
 */
const char*  CTextToken::GetClassName(void) {
  return "text";
}

/*
 *  
 *  
 *  @update  gess 3/25/98
 *  @param   
 *  @return  
 */
PRInt32 CTextToken::GetTokenType(void) {
  return eToken_text;
}

/*
 *  Consume as much clear text from scanner as possible.
 *
 *  @update  gess 3/25/98
 *  @param   aChar -- last char consumed from stream
 *  @param   aScanner -- controller of underlying input source
 *  @return  error result
 */
nsresult CTextToken::Consume(PRUnichar aChar, nsScanner& aScanner) {
  static    const char* theTerminals="\n\r&<";
  nsresult  result=NS_OK;
  PRBool    done=PR_FALSE;

  while((NS_OK==result) && (!done)) {
    result=aScanner.ReadUntil(mTextValue,theTerminals,PR_TRUE,PR_FALSE);
    if(NS_OK==result) {
      result=aScanner.Peek(aChar);
      if(((kCR==aChar) || (kNewLine==aChar)) && (NS_OK==result)) {
        result=aScanner.GetChar(aChar); //strip off the \r
        result=aScanner.Peek(aChar);    //then see what's next.
        if(NS_OK==result) {
          switch(aChar) {
            case kCR:
              result=aScanner.GetChar(aChar); //strip off the \r
              mTextValue.Append("\n\n");
              break;
            case kNewLine:
               //which means we saw \r\n, which becomes \n
              result=aScanner.GetChar(aChar); //strip off the \n
                  //now fall through on purpose...
            default:
              mTextValue.Append("\n");
              break;
          }//switch
        }//if
      }
      else done=PR_TRUE;
    }
  }
  return result;
}

/*
 *  Consume as much clear text from scanner as possible.
 *
 *  @update  gess 3/25/98
 *  @param   aChar -- last char consumed from stream
 *  @param   aScanner -- controller of underlying input source
 *  @return  error result
 */
nsresult CTextToken::ConsumeUntil(PRUnichar aChar,PRBool aIgnoreComments,nsScanner& aScanner,nsString& aTerminalString){
  PRBool        done=PR_FALSE; 
  nsresult      result=NS_OK; 
  PRUnichar     theChar;
  nsAutoString  theRight;
  PRInt32       rpos=0;


  //We're going to try a new algorithm here. Rather than scan for the matching 
 //end tag like we used to do, we're now going to scan for whitespace and comments. 
 //If we find either, just eat them. If we find text or a tag, then go to the 
 //target endtag, or the start of another comment. 


  PRInt32 termStrLen=aTerminalString.Length();
  while((!done) && (NS_OK==result)) { 
    result=aScanner.GetChar(aChar); 
    if((NS_OK==result) && (kLessThan==aChar)) { 
      //we're reading a tag or a comment... 
      //FYI: <STYLE> and <SCRIPT> should be treated as CDATA. So, 
      //don't try to acknowledge "HTML COMMENTS"...just ignore 'em.
      result=aScanner.GetChar(theChar); 
      if((NS_OK==result) && (kExclamation==theChar) && (PR_FALSE==aIgnoreComments)) { 
        //read a comment... 
        static CCommentToken theComment; 
        result=theComment.Consume(aChar,aScanner); 
        if(NS_OK==result) { 
          //result=aScanner.SkipWhitespace();
          mTextValue.Append(theComment.GetStringValueXXX()); 
        } 
      } else { 
        //read a tag... 
        mTextValue+=aChar; 
        mTextValue+=theChar; 
        result=aScanner.ReadUntil(mTextValue,kGreaterThan,PR_TRUE); 
      } 
    } 
    else if(('\b'==theChar) || ('\t'==theChar) || (' '==theChar)) {
      static CWhitespaceToken theWS; 
      result=theWS.Consume(aChar,aScanner); 
      if(NS_OK==result) { 
        mTextValue.Append(theWS.GetStringValueXXX()); 
      } 
    } 
    else { 
      mTextValue+=aChar; 
      result=aScanner.ReadUntil(mTextValue,kLessThan,PR_FALSE); 
    } 
    mTextValue.Right(theRight,termStrLen+10); //first, get a wad of chars from the temp string
    rpos=theRight.RFindChar('<');   //now scan for the '<'
    if(-1<rpos)
      rpos=theRight.RFind(aTerminalString,PR_TRUE);
    done=PRBool(-1<rpos); 
  }  //while
  if(NS_SUCCEEDED(result)) {
    int len=mTextValue.Length();
    mTextValue.Truncate(len-(theRight.Length()-rpos)); 
   
    // Make aTerminalString contain the name of the end tag ** as seen in **
    // the document and not the made up one.
    theRight.Cut(0,rpos+2);
    theRight.Truncate(theRight.Length()-1);
    aTerminalString = theRight;
  }
  return result; 
}

/*
 *  default constructor
 *  
 *  @update  vidur 11/12/98
 *  @param   aName -- string to init token name with
 *  @return  
 */
CCDATASectionToken::CCDATASectionToken() : CHTMLToken(eHTMLTag_unknown) {
}


/*
 *  string based constructor
 *  
 *  @update  vidur 11/12/98
 *  @param   aName -- string to init token name with
 *  @return  
 */
CCDATASectionToken::CCDATASectionToken(const nsString& aName) : CHTMLToken(aName) {
  mTypeID=eHTMLTag_unknown;
}

/*
 *  
 *  
 *  @update  vidur 11/12/98
 *  @param   
 *  @return  
 */
const char*  CCDATASectionToken::GetClassName(void) {
  return "cdatasection";
}

/*
 *  
 *  @update  vidur 11/12/98
 *  @param   
 *  @return  
 */
PRInt32 CCDATASectionToken::GetTokenType(void) {
  return eToken_cdatasection;
}

/*
 *  Consume as much marked test from scanner as possible.
 *
 *  @update  vidur 11/12/98
 *  @param   aChar -- last char consumed from stream
 *  @param   aScanner -- controller of underlying input source
 *  @return  error result
 */
nsresult CCDATASectionToken::Consume(PRUnichar aChar, nsScanner& aScanner) {
  static    const char* theTerminals="\r]";
  nsresult  result=NS_OK;
  PRBool    done=PR_FALSE;

  while((NS_OK==result) && (!done)) {
    result=aScanner.ReadUntil(mTextValue,theTerminals,PR_TRUE,PR_FALSE);
    if(NS_OK==result) {
      result=aScanner.Peek(aChar);
      if((kCR==aChar) && (NS_OK==result)) {
        result=aScanner.GetChar(aChar); //strip off the \r
        result=aScanner.Peek(aChar);    //then see what's next.
        if(NS_OK==result) {
          switch(aChar) {
            case kCR:
              result=aScanner.GetChar(aChar); //strip off the \r
              mTextValue.Append("\n\n");
              break;
            case kNewLine:
               //which means we saw \r\n, which becomes \n
              result=aScanner.GetChar(aChar); //strip off the \n
                  //now fall through on purpose...
            default:
              mTextValue.Append("\n");
              break;
          } //switch
        } //if
      }
      else if (kRightSquareBracket==aChar) {
        result=aScanner.GetChar(aChar); //strip off the ]
        result=aScanner.Peek(aChar);    //then see what's next.
        if((NS_OK==result) && (kRightSquareBracket==aChar)) {
          result=aScanner.GetChar(aChar);    //strip off the second ]
          result=aScanner.Peek(aChar);    //then see what's next.
          if(NS_OK==result) {
            if (kGreaterThan==aChar) {
              result=aScanner.GetChar(aChar); //strip off the >
              done=PR_TRUE;
            }
            else {
              // This isn't the end of the CDATA section so go on
              mTextValue.Append("]");
            }
          }//if
        }
        else {
          // This isn't the end of the CDATA section so go on
          mTextValue.Append("]");
        }
      }
      else done=PR_TRUE;
    }
  }
  return result;
}


/*
 *  Default constructor
 *  
 *  @update  gess 3/25/98
 *  @param   aName -- string to init token name with
 *  @return  
 */
CCommentToken::CCommentToken() : CHTMLToken(eHTMLTag_comment) {
}


/*
 *  Copy constructor
 *  
 *  @update  gess 3/25/98
 *  @param   
 *  @return  
 */
CCommentToken::CCommentToken(const nsString& aName) : CHTMLToken(aName) {
  mTypeID=eHTMLTag_comment;
}

/*
 *  This method consumes a comment using the (CORRECT) comment parsing
 *  algorithm supplied by W3C. 
 *  
 *  @update  gess 01/04/99
 *  @param   
 *  @param   
 *  @return  
 */
static
nsresult ConsumeStrictComment(PRUnichar aChar, nsScanner& aScanner,nsString& aString) {
  static const char* gMinus="-";
  nsresult  result=NS_OK;
 
  /*********************************************************
    NOTE: This algorithm does a fine job of handling comments
          when they're formatted per spec, but if they're not
          we don't handle them well. For example, we gack
          on the following:

          <!-- xx -- xx --> 
   *********************************************************/

  aString="<!";
  while(NS_OK==result) {
    result=aScanner.GetChar(aChar);
    if(NS_OK==result) {
      aString+=aChar;
      if(kMinus==aChar) {
        result=aScanner.GetChar(aChar);
        if(NS_OK==result) {
          if(kMinus==aChar) {
               //in this case, we're reading a long-form comment <-- xxx -->
            aString+=aChar;
            result=aScanner.ReadWhile(aString,gMinus,PR_TRUE,PR_FALSE);  //get all available '---'
            if(NS_OK==result) {
              PRInt32 findpos=-1;
              nsAutoString temp("");
              //Read to the first ending sequence '--'
              while((kNotFound==findpos) && (NS_OK==result)) {
                result=aScanner.ReadUntil(temp,kMinus,PR_TRUE);
                findpos=temp.RFind("--");
              } 
              aString+=temp;
              if(NS_OK==result) {
                result=aScanner.ReadWhile(aString,gMinus,PR_TRUE,PR_FALSE);  //get all available '---'
                if(NS_OK==result) {
                  temp="->";
                  result=aScanner.ReadUntil(aString,temp,PR_FALSE,PR_FALSE);
                }
              } 
            }
          } //
          else break; //go find '>'
        }
      }//if
      else if(kGreaterThan==aChar) {
        return result;
      }
      else break; //go find '>'
    }//if
  }//while
  if(NS_OK==result) {
     //if you're here, we're consuming a "short-form" comment
    result=aScanner.ReadUntil(aString,kGreaterThan,PR_TRUE);
  }
  return result;
}

/*
 *  This method consumes a comment using common (actually non-standard)
 *  algorithm that seems to work against the content on the web.
 *  
 *  @update  gess 01/04/99
 *  @param   
 *  @param   
 *  @return  
 */
static
nsresult ConsumeComment(PRUnichar aChar, nsScanner& aScanner,nsString& aString) {
  

  nsresult  result=NS_OK;
 
  /*********************************************************
    NOTE: This algorithm does a fine job of handling comments
          commonly used, but it doesn't really consume them
          per spec (But then, neither does IE or Nav).
   *********************************************************/

  aString="<!";
  nsAutoString  theRightChars;
  PRInt32       theBestAltPos=kNotFound;
  PRUint32      theStartOffset=0;

  result=aScanner.GetChar(aChar);
  if(NS_OK==result) {
    aString+=aChar;
    if(kMinus==aChar) {
      result=aScanner.GetChar(aChar);
      if(NS_OK==result) {
        if(kMinus==aChar) {
             //in this case, we're reading a long-form comment <-- xxx -->

          nsAutoString gDfltEndComment("-->");

          aString+=aChar;
          PRInt32 findpos=kNotFound;
          while((kNotFound==findpos) && (NS_OK==result)) {
            result=aScanner.ReadUntil(aString,kGreaterThan,PR_TRUE);          
            if(NS_OK==result){

              if(kNotFound==theBestAltPos) {
                const PRUnichar* theBuf=aString.GetUnicode();
                findpos=aString.Length()-3;
                theBuf=(PRUnichar*)&theBuf[findpos];
                if(!gDfltEndComment.Equals(theBuf,PR_FALSE,3)) {
                  //we didn't find the dflt end comment delimiter, so look for alternatives...
                  findpos=kNotFound;
                  theRightChars.Truncate(0);
                  aString.Right(theRightChars,15);
                  theRightChars.StripChars(" ");

                  int rclen=theRightChars.Length();
                  aChar=theRightChars[rclen-2];
                  if(('!'==aChar) || ('-'==aChar)) {
                    theBestAltPos=aString.Length();
                    theStartOffset=aScanner.GetOffset();
                  }
                }
              }

            }
          } //while
          if((kNotFound==findpos) && (!aScanner.IsIncremental())) {
            //if you're here, then we're in a special state. 
            //The problem at hand is that we've hit the end of the document without finding the normal endcomment delimiter "-->".
            //In this case, the first thing we try is to see if we found one of the alternate endcomment delimiters "->" or "!>".
            //If so, rewind just pass than, and use everything up to that point as your comment.
            //If not, the document has no end comment and should be treated as one big comment.
            if(kNotFound<theBestAltPos) {
              aString.Truncate(theBestAltPos);
              aScanner.Mark(theStartOffset);
              result=NS_OK;
            }
          }
          return result;

        } //if
      }//if
    }//if
  }//if
  if(NS_OK==result) {
     //Read up to the closing '>'
    result=aScanner.ReadUntil(aString,kGreaterThan,PR_TRUE);
  }
  return result;
}

/*
 *  Consume the identifier portion of the comment. 
 *  Note that we've already eaten the "<!" portion.
 *  
 *  @update  gess 1/27/99
 *  @param   aChar -- last char consumed from stream
 *  @param   aScanner -- controller of underlying input source
 *  @return  error result
 */
nsresult CCommentToken::Consume(PRUnichar aChar, nsScanner& aScanner) {
  PRBool theStrictForm=PR_FALSE;
  nsresult result=(theStrictForm) ? ConsumeStrictComment(aChar,aScanner,mTextValue) : ConsumeComment(aChar,aScanner,mTextValue);

#if 0
  if(NS_OK==result) {
      //ok then, all is well so strip off the delimiters...
    nsAutoString theLeft("");
    mTextValue.Left(theLeft,2);
    if(theLeft=="<!")
      mTextValue.Cut(0,2);
    if('>'==mTextValue.Last())
      mTextValue.Truncate(mTextValue.Length()-1);
  }
#endif
  return result;
}


/*
 *  
 *  
 *  @update  gess 3/25/98
 *  @param   
 *  @return  
 */
const char* CCommentToken::GetClassName(void){
  return "/**/";
}

/*
 *  
 *  
 *  @update  gess 3/25/98
 *  @param   
 *  @return  
 */
PRInt32 CCommentToken::GetTokenType(void) {
  return eToken_comment;
}

/*
 *  default constructor
 *  
 *  @update  gess 3/25/98
 *  @param   aName -- string to init token name with
 *  @return  
 */
CNewlineToken::CNewlineToken() : CHTMLToken(eHTMLTag_newline) {
}


/*
 *  default constructor
 *  
 *  @update  gess 3/25/98
 *  @param   aName -- string value to init token name with
 *  @return  
 */
CNewlineToken::CNewlineToken(const nsString& aName) : CHTMLToken(aName) {
  mTypeID=eHTMLTag_newline;
}

/*
 *  
 *  
 *  @update  gess 3/25/98
 *  @param   
 *  @return  
 */
const char*  CNewlineToken::GetClassName(void) {
  return "crlf";
}

/*
 *  
 *  
 *  @update  gess 3/25/98
 *  @param   
 *  @return  
 */
PRInt32 CNewlineToken::GetTokenType(void) {
  return eToken_newline;
}

/**
 *  This method retrieves the value of this internal string. 
 *  
 *  @update gess 3/25/98
 *  @return nsString reference to internal string value
 */
nsString& CNewlineToken::GetStringValueXXX(void) {
  static nsString* theStr=0;
  if(!theStr)
    theStr=new nsString("\n");
  return *theStr;
}

/*
 *  Consume as many cr/lf pairs as you can find.
 *  
 *  @update  gess 3/25/98
 *  @param   aChar -- last char consumed from stream
 *  @param   aScanner -- controller of underlying input source
 *  @return  error result
 */
nsresult CNewlineToken::Consume(PRUnichar aChar, nsScanner& aScanner) {
  mTextValue=aChar;

    //we already read the \r or \n, let's see what's next!
  PRUnichar theChar;
  nsresult result=aScanner.Peek(theChar);

  if(NS_OK==result) {
    switch(aChar) {
      case kNewLine:
        if(kCR==theChar) {
          result=aScanner.GetChar(theChar);
          mTextValue+=theChar;
        }
        break;
      case kCR:
        if(kNewLine==theChar) {
          result=aScanner.GetChar(theChar);
          mTextValue+=theChar;
        }
        break;
      default:
        break;
    }
  }  
  return result;
}

/*
 *  default constructor
 *  
 *  @update  gess 3/25/98
 *  @param   aName -- string to init token name with
 *  @return  
 */
CAttributeToken::CAttributeToken() : CHTMLToken(eHTMLTag_unknown) {
}

/*
 *  string based constructor
 *  
 *  @update  gess 3/25/98
 *  @param   aName -- string value to init token name with
 *  @return  
 */
CAttributeToken::CAttributeToken(const nsString& aName) : CHTMLToken(aName),  
  mTextKey() {
  mLastAttribute=PR_FALSE;
}

/*
 *  construct initializing data to 
 *  key value pair
 *  
 *  @update  gess 3/25/98
 *  @param   aName -- string value to init token name with
 *  @return  
 */
CAttributeToken::CAttributeToken(const nsString& aKey, const nsString& aName) : CHTMLToken(aName) {
  mTextKey = aKey;
  mLastAttribute=PR_FALSE;
}

/**
 * 
 * @update	gess8/4/98
 * @param 
 * @return
 */
void CAttributeToken::Reinitialize(PRInt32 aTag, const nsString& aString){
  CHTMLToken::Reinitialize(aTag,aString);
  mTextKey.Truncate();
  mLastAttribute=PR_FALSE;
}

/*
 *  
 *  
 *  @update  gess 3/25/98
 *  @param   
 *  @return  
 */
const char*  CAttributeToken::GetClassName(void) {
  return "attr";
}

/*
 *  
 *  
 *  @update  gess 3/25/98
 *  @param   
 *  @return  
 */
PRInt32 CAttributeToken::GetTokenType(void) {
  return eToken_attribute;
}

/*
 *  Removes non-alpha-non-digit characters from the end of a KEY
 *  
 *  @update harishd 07/15/99
 *  @param  
 *  @return  
 */
void CAttributeToken::SanitizeKey() {
  PRInt32   length=mTextKey.Length();
  if(length > 0) {
    PRUnichar theChar=mTextKey.Last();
    while(!nsString::IsAlpha(theChar) && !nsString::IsDigit(theChar)) {
      mTextKey.Truncate(length-1);
      length = mTextKey.Length();
      if(length <= 0) break;
      theChar = mTextKey.Last();
    }
  }
  return;
}

/*
 *  Dump contents of this token to given output stream
 *  
 *  @update  gess 3/25/98
 *  @param   out -- ostream to output content
 *  @return  
 */
void CAttributeToken::DebugDumpToken(nsOutputStream& out) {
  char buffer[200];
  mTextKey.ToCString(buffer,sizeof(buffer));
  out << "[" << GetClassName() << "] " << buffer << "=";
  mTextValue.ToCString(buffer,sizeof(buffer));
  out << buffer << ": " << mTypeID << nsEndl;
}
 
/*
 *  
 *  
 *  @update  gess 3/25/98
 *  @param   anOutputString will recieve the result
 *  @return  nada
 */
void CAttributeToken::GetSource(nsString& anOutputString){
  anOutputString=mTextKey;
  anOutputString+="=";
  anOutputString+=mTextValue;
  anOutputString+=";";
}

/*
 *  This general purpose method is used when you want to
 *  consume a known quoted string. 
 *  
 *  @update  gess 3/25/98
 *  @param   aChar -- last char consumed from stream
 *  @param   aScanner -- controller of underlying input source
 *  @return  error result
 */
nsresult ConsumeQuotedString(PRUnichar aChar,nsString& aString,nsScanner& aScanner){
  nsresult result=NS_OK;
  switch(aChar) {
    case kQuote:
      result=aScanner.ReadUntil(aString,kQuote,PR_TRUE);
      if(NS_OK==result)
        result=aScanner.SkipOver(kQuote);  //this code is here in case someone mistakenly adds multiple quotes...
      break;
    case kApostrophe:
      result=aScanner.ReadUntil(aString,kApostrophe,PR_TRUE);
      if(NS_OK==result)
        result=aScanner.SkipOver(kApostrophe); //this code is here in case someone mistakenly adds multiple apostrophes...
      break;
    default:
      break;
  }
  PRUnichar ch=aString.Last();
  if(ch!=aChar)
    aString+=aChar;
  //aString.ReplaceChar(PRUnichar('\n'),PRUnichar(' '));
  aString.StripChars("\r\n"); //per the HTML spec, ignore linefeeds...
  return result;
}

/*
 *  This general purpose method is used when you want to
 *  consume attributed text value.
 *  
 *  @update  gess 3/25/98
 *  @param   aChar -- last char consumed from stream
 *  @param   aScanner -- controller of underlying input source
 *  @return  error result
 */
static
nsresult ConsumeAttributeValueText(PRUnichar,nsString& aString,nsScanner& aScanner){
  static const char* theTerminals="\b\t\n\r >";
  nsresult result=aScanner.ReadUntil(aString,theTerminals,PR_TRUE,PR_FALSE);
  
  //Let's force quotes if either the first or last char is quoted.
  PRUnichar theLast=aString.Last();
  PRUnichar theFirst=aString.First();
  if(kQuote==theLast) {
    if(kQuote!=theFirst) {
      aString.Insert(kQuote,0);;
    }
  }
  else if(kQuote==theFirst) {
    if(kQuote!=theLast) {
      aString+=kQuote;
    }
  }

  return result;
}


/*
 *  Consume the key and value portions of the attribute.
 *  
 *  @update  gess 3/25/98
 *  @param   aChar -- last char consumed from stream
 *  @param   aScanner -- controller of underlying input source
 *  @return  error result
 */
nsresult CAttributeToken::Consume(PRUnichar aChar, nsScanner& aScanner) {

  nsresult result=aScanner.SkipWhitespace();             //skip leading whitespace 
  if(NS_OK==result) {
    result=aScanner.Peek(aChar);
    if(NS_OK==result) {
      if(kQuote==aChar) {               //if you're here, handle quoted key...
        result=aScanner.GetChar(aChar);        //skip the quote sign...
        if(NS_OK==result) {
          result=aScanner.Peek(aChar);  //peek ahead to make sure the next char is a legal attr-key
          if(NS_OK==result) {
            if(nsString::IsAlpha(aChar) || nsString::IsDigit(aChar)){
              mTextKey=aChar;
              result=ConsumeQuotedString(aChar,mTextKey,aScanner);
            }
            else {
              return NS_ERROR_HTMLPARSER_BADATTRIBUTE;
            }
          } //if
        }//if
      }
      else if(kHashsign==aChar) {
        result=aScanner.GetChar(aChar);        //skip the hash sign...
        if(NS_OK==result) {
          mTextKey=aChar;
          result=aScanner.ReadNumber(mTextKey);
        }
      }
      else {
          //If you're here, handle an unquoted key.
          //Don't forget to reduce entities inline!
        static const char* theTerminals="\b\t\n\r \"<=>";
        result=aScanner.ReadUntil(mTextKey,theTerminals,PR_TRUE,PR_FALSE);
      }

        //now it's time to Consume the (optional) value...
      if(NS_OK==result) {
        result=aScanner.SkipWhitespace();
        if(NS_OK==result) { 
          result=aScanner.Peek(aChar);       //Skip ahead until you find an equal sign or a '>'...
          if(NS_OK==result) {  
            if(kEqual==aChar){
              result=aScanner.GetChar(aChar);  //skip the equal sign...
              if(NS_OK==result) {
                result=aScanner.SkipWhitespace();     //now skip any intervening whitespace
                if(NS_OK==result) {
                  result=aScanner.GetChar(aChar);  //and grab the next char.    
                  if(NS_OK==result) {
                    if((kQuote==aChar) || (kApostrophe==aChar)) {
                      mTextValue=aChar;
                      result=ConsumeQuotedString(aChar,mTextValue,aScanner);
                    }
                    else if(kGreaterThan==aChar){      
                      result=aScanner.PutBack(aChar);
                    }
                    else if(kAmpersand==aChar) {
                      mTextValue=aChar;
                      result=aScanner.GetChar(aChar);
                      if(NS_OK==result) {
                        mTextValue += aChar;
                        result=CEntityToken::ConsumeEntity(aChar,mTextValue,aScanner);
                      }
                    }
                    else {
                      mTextValue=aChar;       //it's an alphanum attribute...
                      result=ConsumeAttributeValueText(aChar,mTextValue,aScanner);
                    } 
                  }//if
                  if(NS_OK==result)
                    result=aScanner.SkipWhitespace();     
                }//if
              }//if
            }//if
            else {
              //This is where we have to handle fairly busted content.
              //If you're here, it means we saw an attribute name, but couldn't find 
              //the following equal sign.  <tag NAME=....
          
              //Doing this right in all cases is <i>REALLY</i> ugly. 
              //My best guess is to grab the next non-ws char. We know it's not '=',
              //so let's see what it is. If it's a '"', then assume we're reading
              //from the middle of the value. Try stripping the quote and continuing...

              if(kQuote==aChar){
                result=aScanner.SkipOver(aChar); //strip quote.
              }
            }
          }//if
        } //if
      }//if
      if(NS_OK==result) {
        result=aScanner.Peek(aChar);
        mLastAttribute= PRBool((kGreaterThan==aChar) || (kEOF==result));
      }
    } //if
  }//if
  return result;
}

/*
 *  Dump contents of this token to givne output stream
 *  
 *  @update  gess 3/25/98
 *  @param   out -- ostream to output content
 *  @return  
 */
void CAttributeToken::DebugDumpSource(nsOutputStream& out) {
  static char buffer[1000];
  mTextKey.ToCString(buffer,sizeof(buffer));
  out << " " << buffer;
  if(mTextValue.Length()){
    mTextValue.ToCString(buffer,sizeof(buffer));
    out << "=" << buffer;
  }
  if(mLastAttribute)
    out<<">";
}

/*
 *  default constructor
 *  
 *  @update  gess 3/25/98
 *  @param   aName -- string to init token name with
 *  @return  
 */
CWhitespaceToken::CWhitespaceToken() : CHTMLToken(eHTMLTag_whitespace) {
}


/*
 *  default constructor
 *  
 *  @update  gess 3/25/98
 *  @param   aName -- string value to init token name with
 *  @return  
 */
CWhitespaceToken::CWhitespaceToken(const nsString& aName) : CHTMLToken(aName) {
  mTypeID=eHTMLTag_whitespace;
}

/*
 *  
 *  
 *  @update  gess 3/25/98
 *  @param   
 *  @return  
 */
const char*  CWhitespaceToken::GetClassName(void) {
  return "ws";
}

/*
 *  
 *  
 *  @update  gess 3/25/98
 *  @param   
 *  @return  
 */
PRInt32 CWhitespaceToken::GetTokenType(void) {
  return eToken_whitespace;
}

/*
 *  This general purpose method is used when you want to
 *  consume an aribrary sequence of whitespace. 
 *  
 *  @update  gess 3/25/98
 *  @param   aChar -- last char consumed from stream
 *  @param   aScanner -- controller of underlying input source
 *  @return  error result
 */
nsresult CWhitespaceToken::Consume(PRUnichar aChar, nsScanner& aScanner) {
  mTextValue=aChar;
  nsresult result=aScanner.ReadWhitespace(mTextValue);
  if(NS_OK==result) {
    mTextValue.StripChars("\r");
  }
  return result;
}

/*
 *  default constructor
 *  
 *  @update  gess 3/25/98
 *  @param   aName -- string to init token name with
 *  @return  
 */
CEntityToken::CEntityToken() : CHTMLToken(eHTMLTag_entity) {
}

/*
 *  default constructor
 *  
 *  @update  gess 3/25/98
 *  @param   aName -- string value to init token name with
 *  @return  
 */
CEntityToken::CEntityToken(const nsString& aName) : CHTMLToken(aName) {
  mTypeID=eHTMLTag_entity;
#ifdef VERBOSE_DEBUG
  if(!VerifyEntityTable())  {
    cout<<"Entity table is invalid!" << endl;
  }
#endif
}


/*
 *  Consume the rest of the entity. We've already eaten the "&".
 *  
 *  @update  gess 3/25/98
 *  @param   aChar -- last char consumed from stream
 *  @param   aScanner -- controller of underlying input source
 *  @return  error result
 */
nsresult CEntityToken::Consume(PRUnichar aChar, nsScanner& aScanner) {
  if(aChar)
    mTextValue=aChar;
  nsresult result=ConsumeEntity(aChar,mTextValue,aScanner);
  return result;
}

/*
 *  
 *  
 *  @update  gess 3/25/98
 *  @param   
 *  @return  
 */
const char*  CEntityToken::GetClassName(void) {
  return "&entity";
}


/*
 *  
 *  
 *  @update  gess 3/25/98
 *  @param   
 *  @return  
 */
PRInt32 CEntityToken::GetTokenType(void) {
  return eToken_entity;
}

/*
 *  This general purpose method is used when you want to
 *  consume an entity &xxxx;. Keep in mind that entities
 *  are <i>not</i> reduced inline.
 *  
 *  @update  gess 3/25/98
 *  @param   aChar -- last char consumed from stream
 *  @param   aScanner -- controller of underlying input source
 *  @return  error result
 */
PRInt32 CEntityToken::ConsumeEntity(PRUnichar aChar,nsString& aString,nsScanner& aScanner){
  PRUnichar theChar=0;
  PRInt32 result=aScanner.Peek(theChar);
  if(NS_OK==result) {
    if(kLeftBrace==aChar) {
      //you're consuming a script entity...
      PRInt32 rightBraceCount = 0;
      PRInt32 leftBraceCount  = 1;
      while(leftBraceCount!=rightBraceCount) {
        result=aScanner.GetChar(aChar);
        if(NS_OK!=result) return result;
        aString += aChar;
        if(aChar==kRightBrace)
          rightBraceCount++;
        else if(aChar==kLeftBrace)
          leftBraceCount++;
      }
      result=aScanner.ReadUntil(aString,kSemicolon,PR_FALSE);
      if(NS_OK==result) {
        result=aScanner.GetChar(aChar); // This character should be a semicolon
        if(NS_OK==result) aString += aChar;
      }
    } //if
    else {
      if(kHashsign==aChar) {
        if('X'==(toupper((char)theChar))) {
          result=aScanner.GetChar(theChar);
          aString+=theChar;
        }
        if(NS_OK==result){
          result=aScanner.ReadNumber(aString);
        }
      }
      else result=aScanner.ReadIdentifier(aString);
      if(NS_OK==result) {
        result=aScanner.Peek(theChar);
        if(NS_OK==result) {
          if (kSemicolon == theChar) {
            // consume semicolon that stopped the scan
            aString+=theChar;
            result=aScanner.GetChar(theChar);
          }
        }
      }//if
    } //else
  } //if
  return result;
}

#define PA_REMAP_128_TO_160_ILLEGAL_NCR 1

#ifdef PA_REMAP_128_TO_160_ILLEGAL_NCR
/**
 * Map some illegal but commonly used numeric entities into their
 * appropriate unicode value.
 */
#define NOT_USED 0xfffd

static PRUint16 PA_HackTable[] = {
	NOT_USED,
	NOT_USED,
	0x201a,  /* SINGLE LOW-9 QUOTATION MARK */
	0x0192,  /* LATIN SMALL LETTER F WITH HOOK */
	0x201e,  /* DOUBLE LOW-9 QUOTATION MARK */
	0x2026,  /* HORIZONTAL ELLIPSIS */
	0x2020,  /* DAGGER */
	0x2021,  /* DOUBLE DAGGER */
	0x02c6,  /* MODIFIER LETTER CIRCUMFLEX ACCENT */
	0x2030,  /* PER MILLE SIGN */
	0x0160,  /* LATIN CAPITAL LETTER S WITH CARON */
	0x2039,  /* SINGLE LEFT-POINTING ANGLE QUOTATION MARK */
	0x0152,  /* LATIN CAPITAL LIGATURE OE */
	NOT_USED,
	0x017D,  /* CAPITAL Z HACEK */
	NOT_USED,
	NOT_USED,
	0x2018,  /* LEFT SINGLE QUOTATION MARK */
	0x2019,  /* RIGHT SINGLE QUOTATION MARK */
	0x201c,  /* LEFT DOUBLE QUOTATION MARK */
	0x201d,  /* RIGHT DOUBLE QUOTATION MARK */
	0x2022,  /* BULLET */
	0x2013,  /* EN DASH */
	0x2014,  /* EM DASH */
	0x02dc,  /* SMALL TILDE */
	0x2122,  /* TRADE MARK SIGN */
	0x0161,  /* LATIN SMALL LETTER S WITH CARON */
	0x203a,  /* SINGLE RIGHT-POINTING ANGLE QUOTATION MARK */
	0x0153,  /* LATIN SMALL LIGATURE OE */
	NOT_USED,
	NOT_USED,
	0x0178   /* LATIN CAPITAL LETTER Y WITH DIAERESIS */
};
#endif /* PA_REMAP_128_TO_160_ILLEGAL_NCR */


/*
 *  This method converts this entity into its underlying
 *  unicode equivalent.
 *  
 *  @update  gess 3/25/98
 *  @param   aString will hold the resulting string value
 *  @return  numeric (unichar) value
 */
PRInt32 CEntityToken::TranslateToUnicodeStr(nsString& aString) {
  PRInt32 value=0;
  PRInt32 theRadix[2]={16,10};

  if(mTextValue.Length()>1) {
    PRUnichar theChar0=mTextValue.CharAt(0);

    if(kHashsign==theChar0) {
      PRInt32 err=0;
      
      PRUnichar theChar1=mTextValue.CharAt(1);
      PRBool    isDigit1=nsString::IsDigit(theChar1);
      value=mTextValue.ToInteger(&err,theRadix[isDigit1]);
      if(0==err) {
  #ifdef PA_REMAP_128_TO_160_ILLEGAL_NCR
        /* for some illegal, but popular usage */
        if ((value >= 0x0080) && (value <= 0x009f)) {
          value = PA_HackTable[value - 0x0080];
        }
  #endif
        aString.Append(PRUnichar(value));
      }//if
    }
    else{
      value = nsHTMLEntities::EntityToUnicode(mTextValue);
      if(-1<value) {
        //we found a named entity...
        aString=PRUnichar(value);
      }
    }//else
  }//if

  return value;
}

/*
 *  Dump contents of this token to givne output stream
 *  
 *  @update  gess 3/25/98
 *  @param   out -- ostream to output content
 *  @return  
 */
void CEntityToken::DebugDumpSource(nsOutputStream& out) {
  char* cp=mTextValue.ToNewCString();
  out << "&" << *cp;
  delete[] cp;
}

/*
 *  
 *  
 *  @update  gess 3/25/98
 *  @param   anOutputString will recieve the result
 *  @return  nada
 */
void CEntityToken::GetSource(nsString& anOutputString){
  anOutputString="&";
  anOutputString+=mTextValue;
  //anOutputString+=";";
}

/*
 *  default constructor
 *  
 *  @update  gess 3/25/98
 *  @param   aName -- string to init token name with
 *  @return  
 */
CScriptToken::CScriptToken() : CHTMLToken(eHTMLTag_script) {
}

/*
 *  
 *  
 *  @update  gess 3/25/98
 *  @param   
 *  @return  
 */
const char*  CScriptToken::GetClassName(void) {
  return "script";
}

/*
 *  
 *  
 *  @update  gess 3/25/98
 *  @param   
 *  @return  
 */
PRInt32 CScriptToken::GetTokenType(void) {
  return eToken_script;
}

/*
 *  default constructor
 *  
 *  @update  gess 3/25/98
 *  @param   aName -- string to init token name with
 *  @return  
 */
CStyleToken::CStyleToken() : CHTMLToken(eHTMLTag_style) {
}

/*
 *  
 *  
 *  @update  gess 3/25/98
 *  @param   
 *  @return  
 */
const char*  CStyleToken::GetClassName(void) {
  return "style";
}

/*
 *  
 *  
 *  @update  gess 3/25/98
 *  @param   
 *  @return  
 */
PRInt32 CStyleToken::GetTokenType(void) {
  return eToken_style;
}


/*
 *  string based constructor
 *  
 *  @update  gess 3/25/98
 *  @param   aName -- string value to init token name with
 *  @return  
 */
CSkippedContentToken::CSkippedContentToken(const nsString& aName) : CAttributeToken(aName) {
  mTextKey = "$skipped-content";/* XXX need a better answer! */
}

/*
 *  
 *  
 *  @update  gess 3/25/98
 *  @param   
 *  @return  
 */
const char*  CSkippedContentToken::GetClassName(void) {
  return "skipped";
}

/*
 *  Retrieve the token type as an int.
 *  @update  gess 3/25/98 
 *  @return  
 */
PRInt32 CSkippedContentToken::GetTokenType(void) {
  return eToken_skippedcontent;
}

/* 
 *  Consume content until you find an end sequence that matches 
 *  this objects current mTextValue. Note that this is complicated 
 *  by the fact that you can be parsing content that itself 
 *  contains quoted content of the same type (like <SCRIPT>). 
 *  That means we have to look for quote-pairs, and ignore the 
 *  content inside them. 
 * 
 *  @update  gess 7/25/98 
 *  @param   aScanner -- controller of underlying input source 
 *  @return  error result 
 */ 
nsresult CSkippedContentToken::Consume(PRUnichar aChar,nsScanner& aScanner) { 
  PRBool      done=PR_FALSE; 
  nsresult    result=NS_OK; 
  nsString    temp; 
  PRUnichar   theChar;

  //We're going to try a new algorithm here. Rather than scan for the matching 
 //end tag like we used to do, we're now going to scan for whitespace and comments. 
 //If we find either, just eat them. If we find text or a tag, then go to the 
 //target endtag, or the start of another comment. 

  while((!done) && (NS_OK==result)) { 
    result=aScanner.GetChar(aChar); 
    if((NS_OK==result) && (kLessThan==aChar)) { 
      //we're reading a tag or a comment... 
      result=aScanner.GetChar(theChar); 
      if((NS_OK==result) && (kExclamation==theChar)) { 
        //read a comment... 
        static CCommentToken theComment; 
        result=theComment.Consume(aChar,aScanner); 
        if(NS_OK==result) { 
          //result=aScanner.SkipWhitespace();
          temp.Append(theComment.GetStringValueXXX()); 
        } 
      } else { 
        //read a tag... 
        temp+=aChar; 
        temp+=theChar; 
        result=aScanner.ReadUntil(temp,kGreaterThan,PR_TRUE); 
      } 
    } 
    else if(('\b'==theChar) || ('\t'==theChar) || (' '==theChar)) {
      static CWhitespaceToken theWS; 
      result=theWS.Consume(aChar,aScanner); 
      if(NS_OK==result) { 
        temp.Append(theWS.GetStringValueXXX()); 
      } 
    } 
    else { 
      temp+=aChar; 
      result=aScanner.ReadUntil(temp,kLessThan,PR_FALSE); 
    } 
    nsAutoString theRight;
    temp.Right(theRight,mTextValue.Length());
    done=PRBool(0==theRight.Compare(mTextValue,PR_TRUE)); 
  } 
  int len=temp.Length(); 
  temp.Truncate(len-mTextValue.Length()); 
  mTextKey=temp; 
  return result; 
} 

/*
 *  Dump contents of this token to givne output stream
 *  
 *  @update  gess 3/25/98
 *  @param   out -- ostream to output content
 *  @return  
 */
void CSkippedContentToken::DebugDumpSource(nsOutputStream& out) {
  static char buffer[1000];
  mTextKey.ToCString(buffer,sizeof(buffer));
  out << " " << buffer;
  if(mLastAttribute)
    out<<">";
}

/*
 *  
 *  
 *  @update  gess 3/25/98
 *  @param   anOutputString will recieve the result
 *  @return  nada
 */
void CSkippedContentToken::GetSource(nsString& anOutputString){
  anOutputString="$skipped-content";
}

/**
 * 
 * @update	gess4/25/98
 * @param 
 * @return
 */
const char* GetTagName(PRInt32 aTag) {
  const nsCString& result = nsHTMLTags::GetStringValue((nsHTMLTag) aTag);
  if (0 == result.Length()) {
    if(aTag>=eHTMLTag_userdefined)
      return gUserdefined;
    else return 0;
  }
  return result;
}


/**
 *  
 *  
 *  @update  gess 9/23/98
 *  @param   
 *  @return  
 */
CInstructionToken::CInstructionToken() : CHTMLToken(eHTMLTag_unknown) {
}

/**
 *  
 *  
 *  @update  gess 9/23/98
 *  @param   
 *  @return  
 */
CInstructionToken::CInstructionToken(const nsString& aString) : CHTMLToken(aString) {
}

/**
 *  
 *  
 *  @update  gess 9/23/98
 *  @param   
 *  @return  
 */
nsresult CInstructionToken::Consume(PRUnichar aChar,nsScanner& aScanner){
  mTextValue="<?";
  nsresult result=aScanner.ReadUntil(mTextValue,kGreaterThan,PR_TRUE);
  return result;
}

/**
 *  
 *  
 *  @update  gess 9/23/98
 *  @param   
 *  @return  
 */
const char* CInstructionToken::GetClassName(void){
  return "instruction";
}

/**
 *  
 *  
 *  @update  gess 9/23/98
 *  @param   
 *  @return  
 */
PRInt32 CInstructionToken::GetTokenType(void){
  return eToken_instruction;
}


CErrorToken::CErrorToken(nsParserError *aError) : CHTMLToken(eHTMLTag_unknown)
{
  mError = aError;
}

CErrorToken::~CErrorToken() 
{
  delete mError;
}

PRInt32 CErrorToken::GetTokenType(void){
  return eToken_error;
}

const char* CErrorToken::GetClassName(void){
  return "error";
}

void CErrorToken::SetError(nsParserError *aError) {
  mError = aError;
}

const nsParserError * CErrorToken::GetError(void) 
{ 
  return mError; 
}

// Doctype decl token

CDoctypeDeclToken::CDoctypeDeclToken() : CHTMLToken(eHTMLTag_unknown) {
}

nsresult CDoctypeDeclToken::Consume(PRUnichar aChar, nsScanner& aScanner) {
 return ConsumeComment(aChar,aScanner,mTextValue);
}

const char*  CDoctypeDeclToken::GetClassName(void) {
  return "doctype";
}

PRInt32 CDoctypeDeclToken::GetTokenType(void) {
  return eToken_doctypeDecl;
}
