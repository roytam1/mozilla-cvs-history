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
 * @update  gess 4/8/98
 * 
 *         
 */


#include "nsIDTDDebug.h"
#include "nsWellFormedDTD.h"
#include "nsCRT.h"
#include "nsParser.h"
#include "nsScanner.h"
#include "nsIParser.h"
#include "nsTokenHandler.h"
#include "nsDTDUtils.h"
#include "nsIContentSink.h"
#include "nsIHTMLContentSink.h"
#include "nsHTMLTokenizer.h"
#include "nsExpatTokenizer.h"

#include "prenv.h"  //this is here for debug reasons...
#include "prtypes.h"  //this is here for debug reasons...
#include "prio.h"
#include "plstr.h"

#ifdef XP_PC
#include <direct.h> //this is here for debug reasons...
#endif
#include "prmem.h"
#include "nsSpecialSystemDirectory.h"

#include <ctype.h>  // toupper()
#include "nsString.h"

static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);                 
static NS_DEFINE_IID(kIDTDIID,      NS_IDTD_IID);
static NS_DEFINE_IID(kClassIID,     NS_WELLFORMED_DTD_IID); 


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
nsresult CWellFormedDTD::QueryInterface(const nsIID& aIID, void** aInstancePtr)  
{                                                                        
  if (NULL == aInstancePtr) {                                            
    return NS_ERROR_NULL_POINTER;                                        
  }                                                                      

  if(aIID.Equals(kISupportsIID))    {  //do IUnknown...
    *aInstancePtr = (nsIDTD*)(this);                                        
  }
  else if(aIID.Equals(kIDTDIID)) {  //do IParser base class...
    *aInstancePtr = (nsIDTD*)(this);                                        
  }
  else if(aIID.Equals(kClassIID)) {  //do this class...
    *aInstancePtr = (CWellFormedDTD*)(this);                                        
  }                 
  else {
    *aInstancePtr=0;
    return NS_NOINTERFACE;
  }
  NS_ADDREF_THIS();
  return NS_OK;                                                        
}

/**
 *  This method is defined in nsIParser. It is used to 
 *  cause the COM-like construction of an nsParser.
 *  
 *  @update  gess 4/8/98
 *  @param   nsIParser** ptr to newly instantiated parser
 *  @return  NS_xxx error result
 */
NS_HTMLPARS nsresult NS_NewWellFormed_DTD(nsIDTD** aInstancePtrResult)
{
  CWellFormedDTD* it = new CWellFormedDTD();

  if (it == 0) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  return it->QueryInterface(kClassIID, (void **) aInstancePtrResult);
}


NS_IMPL_ADDREF(CWellFormedDTD)
NS_IMPL_RELEASE(CWellFormedDTD)


/**
 *  Default constructor
 *  
 *  @update  gess 4/9/98
 *  @param   
 *  @return  
 */
CWellFormedDTD::CWellFormedDTD() : nsIDTD(), mFilename("") {
  NS_INIT_REFCNT();
  mParser=0;
  mSink=0;
  mLineNumber=0;
  mTokenizer=0;
}

/**
 *  Default destructor
 *  
 *  @update  gess 4/9/98
 *  @param   
 *  @return  
 */
CWellFormedDTD::~CWellFormedDTD(){
  mParser=0; //just to prove we destructed...
  if (mTokenizer)
    delete mTokenizer;
  mTokenizer=0;
}

/**
 * 
 * @update	gess1/8/99
 * @param 
 * @return
 */
const nsIID& CWellFormedDTD::GetMostDerivedIID(void) const{
  return kClassIID;
}

/**
 * Call this method if you want the DTD to construct a fresh 
 * instance of itself. 
 * @update	gess7/23/98
 * @param 
 * @return
 */
nsresult CWellFormedDTD::CreateNewInstance(nsIDTD** aInstancePtrResult){
  return NS_NewWellFormed_DTD(aInstancePtrResult);
}

/**
 * This method is called to determine if the given DTD can parse
 * a document in a given source-type. 
 * NOTE: Parsing always assumes that the end result will involve
 *       storing the result in the main content model.
 * @update	gess6/24/98
 * @param   
 * @return  TRUE if this DTD can satisfy the request; FALSE otherwise.
 */
eAutoDetectResult CWellFormedDTD::CanParse(nsString& aContentType, nsString& aCommand, nsString& aBuffer, PRInt32 aVersion) {
  eAutoDetectResult result=eUnknownDetect;

  if(!aCommand.Equals(kViewSourceCommand)) {
    if(aContentType.Equals(kXMLTextContentType) ||
       aContentType.Equals(kRDFTextContentType) ||
       aContentType.Equals(kXULTextContentType)) {
      result=ePrimaryDetect;
    }
    else {
      if(-1<aBuffer.Find("<?xml ")) {
        if(0==aContentType.Length()) {
          aContentType = kXMLTextContentType; //only reset it if it's empty
        }
        result=eValidDetect;
      }
    }
  }
  return result;
}


/**
 * 
 * @update	gess5/18/98
 * @param 
 * @return
 */
NS_IMETHODIMP CWellFormedDTD::WillBuildModel(nsString& aFilename,PRBool aNotifySink,nsString& aSourceType,nsIContentSink* aSink){
  nsresult result=NS_OK;
  mFilename=aFilename;

  mSink=aSink;
  if((aNotifySink) && (mSink)) {
    mLineNumber=0;
    result = mSink->WillBuildModel();

#if 0
    /* COMMENT OUT THIS BLOCK IF: you aren't using an nsHTMLContentSink...*/
    {

        //now let's automatically open the html...
      CStartToken theHTMLToken(eHTMLTag_html);
      nsCParserNode theHTMLNode(&theHTMLToken,0);
      mSink->OpenHTML(theHTMLNode);

        //now let's automatically open the body...
      CStartToken theBodyToken(eHTMLTag_body);
      nsCParserNode theBodyNode(&theBodyToken,0);
      mSink->OpenBody(theBodyNode);
    }
    /* COMMENT OUT THIS BLOCK IF: you aren't using an nsHTMLContentSink...*/
#endif
  }

  return result;
}

/**
  * The parser uses a code sandwich to wrap the parsing process. Before
  * the process begins, WillBuildModel() is called. Afterwards the parser
  * calls DidBuildModel(). 
  * @update	gess  1/4/99
  * @param	aFilename is the name of the file being parsed.
  * @return	error code (almost always 0)
  */
NS_IMETHODIMP CWellFormedDTD::BuildModel(nsIParser* aParser,nsITokenizer* aTokenizer,nsITokenObserver* anObserver,nsIContentSink* aSink) {
  nsresult result=NS_OK;

  if(aTokenizer) {
    nsHTMLTokenizer*  oldTokenizer=mTokenizer;
    mTokenizer=(nsHTMLTokenizer*)aTokenizer;
    nsITokenRecycler* theRecycler=aTokenizer->GetTokenRecycler();

    while(NS_OK==result){
      CToken* theToken=mTokenizer->PopToken();
      if(theToken) {
        result=HandleToken(theToken,aParser);
        if(NS_SUCCEEDED(result) || (NS_ERROR_HTMLPARSER_BLOCK==result)) {
          theRecycler->RecycleToken(theToken);
        }
        else {
          // if(NS_ERROR_HTMLPARSER_BLOCK!=result){
          mTokenizer->PushTokenFront(theToken);
        }
        // theRootDTD->Verify(kEmptyString,aParser);
      }
      else break;
    }//while
    mTokenizer=oldTokenizer;
  }
  else result=NS_ERROR_HTMLPARSER_BADTOKENIZER;
  return result;
}


/**
 * 
 * @update	gess5/18/98
 * @param 
 * @return
 */
NS_IMETHODIMP CWellFormedDTD::DidBuildModel(nsresult anErrorCode,PRBool aNotifySink,nsIParser* aParser,nsIContentSink* aSink){
  nsresult result= NS_OK;

  //ADD CODE HERE TO CLOSE OPEN CONTAINERS...

  if(aParser){
    mSink=aParser->GetContentSink();
    if((aNotifySink) && (mSink)) {
        result = mSink->DidBuildModel(1);

  #if 0
      /* COMMENT OUT THIS BLOCK IF: you aren't using an nsHTMLContentSink...*/
      {
        nsIHTMLContentSink* mSink=(nsIHTMLContentSink*)mSink;

          //now let's automatically open the body...
        CEndToken theBodyToken(eHTMLTag_body);
        nsCParserNode theBodyNode(&theBodyToken,0);
        mSink->CloseBody(theBodyNode);

          //now let's automatically open the html...
        CEndToken theHTMLToken(eHTMLTag_html);
        nsCParserNode theHTMLNode(&theBodyToken,0);
        mSink->CloseHTML(theBodyNode);

      }
      /* COMMENT OUT THIS BLOCK IF: you aren't using an nsHTMLContentSink...*/
  #endif
    }
  }
  return result;
}

/**
 * 
 * @update	gess8/4/98
 * @param 
 * @return
 */
nsITokenRecycler* CWellFormedDTD::GetTokenRecycler(void){
  nsITokenizer* theTokenizer=GetTokenizer();
  if(theTokenizer)
    return theTokenizer->GetTokenRecycler();
  return 0;
}

/**
 * Use this id you want to stop the building content model
 * --------------[ Sets DTD to STOP mode ]----------------
 * It's recommended to use this method in accordance with
 * the parser's terminate() method.
 *
 * @update	harishd 07/22/99
 * @param 
 * @return
 */
nsresult  CWellFormedDTD::Terminate(void)
{
  return NS_ERROR_HTMLPARSER_STOPPARSING;
}

/**
 * Retrieve the preferred tokenizer for use by this DTD.
 * @update	gess12/28/98
 * @param   none
 * @return  ptr to tokenizer
 */
nsITokenizer* CWellFormedDTD::GetTokenizer(void) {
  if(!mTokenizer)
    mTokenizer=(nsHTMLTokenizer*)new nsExpatTokenizer(&mFilename);
  return mTokenizer;
}

/**
 * 
 * @update	gess5/18/98
 * @param 
 * @return
 */
NS_IMETHODIMP CWellFormedDTD::WillResumeParse(void){
  nsresult result = NS_OK;
  if(mSink) {
    result = mSink->WillResume();
  }
  return result;
}

/**
 * 
 * @update	gess5/18/98
 * @param 
 * @return
 */
NS_IMETHODIMP CWellFormedDTD::WillInterruptParse(void){
  nsresult result = NS_OK;
  if(mSink) {
    result = mSink->WillInterrupt();
  }
  return result;
}

/**
 * Called by the parser to initiate dtd verification of the
 * internal context stack.
 * @update	gess 7/23/98
 * @param 
 * @return
 */
PRBool CWellFormedDTD::Verify(nsString& aURLRef,nsIParser* aParser) {
  PRBool result=PR_TRUE;
  mParser=(nsParser*)aParser;
  return result;
}

/**
 * Called by the parser to enable/disable dtd verification of the
 * internal context stack.
 * @update	gess 7/23/98
 * @param 
 * @return
 */
void CWellFormedDTD::SetVerification(PRBool aEnabled){
}

/**
 *  This method is called to determine whether or not a tag
 *  of one type can contain a tag of another type.
 *  
 *  @update  gess 3/25/98
 *  @param   aParent -- int tag of parent container
 *  @param   aChild -- int tag of child container
 *  @return  PR_TRUE if parent can contain child
 */
PRBool CWellFormedDTD::CanContain(PRInt32 aParent,PRInt32 aChild) const{
  PRBool result=PR_TRUE;
  return result;
}

/**
 * Give rest of world access to our tag enums, so that CanContain(), etc,
 * become useful.
 */
NS_IMETHODIMP CWellFormedDTD::StringTagToIntTag(nsString &aTag, PRInt32* aIntTag) const
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/**
 *  This method gets called to determine whether a given 
 *  tag is itself a container
 *  
 *  @update  gess 3/25/98
 *  @param   aTag -- tag to test for containership
 *  @return  PR_TRUE if given tag can contain other tags
 */
PRBool CWellFormedDTD::IsContainer(PRInt32 aTag) const{
  PRBool result=PR_TRUE;
  return result;
}

/**
 *  Helper method that filters out the PI from a given string. 
 *  
 *  
 *  @update  harishd 06/27/99
 *  @param   aPIString -- string that contains the PI.
 *  @param   aPI -- PI that's filtered out from aPIString.
 *  @return  
 */

void GetProcessingInstruction(const nsString& aPIString, char* a_PI) 
{
  static nsAutoString theWS2("\b\t\n ");

  if(aPIString.Length() > 0) {
    nsString temp;
    PRInt32 theOffset = aPIString.FindCharInSet(theWS2,1);
    aPIString.Mid(temp,1,theOffset);
    temp.ToCString(a_PI,temp.Length());
  }
  return;
}

/**
 *  
 *  @update  vidur 11/12/98
 *  @param   aToken -- token object to be put into content model
 *  @return  0 if all is well; non-zero is an error
 */
NS_IMETHODIMP CWellFormedDTD::HandleToken(CToken* aToken,nsIParser* aParser) {
  nsresult        result=NS_OK;
  CHTMLToken*     theToken= (CHTMLToken*)(aToken);
  eHTMLTokenTypes theType= (eHTMLTokenTypes)theToken->GetTokenType();

  mParser=(nsParser*)aParser;
  mSink=aParser->GetContentSink();

  nsCParserNode theNode(theToken,mLineNumber,mTokenizer->GetTokenRecycler());
  switch(theType) {

    case eToken_newline:
      mLineNumber++; //now fall through
    case eToken_entity:
    case eToken_whitespace:
    case eToken_text:
    case eToken_cdatasection:
      result=mSink->AddLeaf(theNode); 
      break;

    case eToken_comment:
      result=mSink->AddComment(theNode); 
      break;
    
    case eToken_instruction:
     {
        char thePI[30]={0};
        nsString& thePIString = theToken->GetStringValueXXX();
        GetProcessingInstruction(thePIString,thePI);
        // XXX - HACK - The current observer dictionary is tag based. Converting it to be string based
        // might cause some overhead.  Until we figure out a better solution, in handling PIs and tags, I'm hardcoding
        // a specific PI observer-list to be notified.
        eHTMLTags theTag  = (nsCRT::strcasecmp(thePI,"?xml") == 0)? eHTMLTag_unknown:eHTMLTag_userdefined;
        nsDeque*  theDeque= (mParser)? (mParser->GetObserverDictionary()).GetObserversForTag(theTag):nsnull;
        if(theDeque) {
          CParserContext* pc=mParser->PeekContext();
          void* theDocID=(pc) ? pc-> mKey : 0; 
          nsObserverNotifier theNotifier(thePIString.GetUnicode(),(PRUint32)theDocID); 
          theDeque->FirstThat(theNotifier);
        }
        result=mSink->AddProcessingInstruction(theNode); 
        break;
      }

    case eToken_start:
      {
        PRInt16 attrCount=aToken->GetAttributeCount();

        if(0<attrCount){ //go collect the attributes...
          int attr=0;
          for(attr=0;attr<attrCount;attr++){
            CToken* theInnerToken=mTokenizer->PeekToken();
            if(theInnerToken)  {
              eHTMLTokenTypes theInnerType=eHTMLTokenTypes(theInnerToken->GetTokenType());
              if(eToken_attribute==theInnerType){
                mTokenizer->PopToken(); //pop it for real...
                theNode.AddAttribute(theInnerToken);
              } 
            }
            else return kEOF;
          }
        }
        if(NS_OK==result){
          result=mSink->OpenContainer(theNode); 
          if(((CStartToken*)aToken)->IsEmpty()){
            result=mSink->CloseContainer(theNode); 
          }
        }
      }
      break;

    case eToken_end:
      result=mSink->CloseContainer(theNode); 
      break;

    case eToken_error:
      {
        // Propagate the error onto the content sink.
        CErrorToken *errTok = (CErrorToken *)aToken;
        
        // XXX Dump error to error output stream just in case the content
        // sink is RDF or XUL and does not implement error handling.  We need to factor
        // code better among HTMLContentSink, XMLContentSink, RDFContentSink,
        // and XULContentSink.  Until that happens, instead of cutting and
        // pasting error handling code for each content sink, I output an
        // error to cerr here.
        const nsParserError* error = errTok->GetError();
        if (error) {
          char* temp;          
          cerr << "XML Error in file '" << (temp = mFilename.ToNewCString()) << "', ";
          delete [] temp;
          cerr << "Line Number: " << error->lineNumber << ", ";
          cerr << "Col Number: " << error->colNumber << ", ";
          cerr << "Description: " << (temp = error->description.ToNewCString()) << "\n";
          delete [] temp;
          cerr << "Source Line: " << (temp = error->sourceLine.ToNewCString()) << "\n";
          delete [] temp;
        }

        result = mSink->NotifyError(errTok->GetError());
      }
      break;
    case eToken_xmlDecl:
      // result = mSink->AddXMLDecl(theNode);
      break;
    case eToken_doctypeDecl:
      result = mSink->AddDocTypeDecl(theNode);
      break;
    case eToken_style:
    case eToken_skippedcontent:
    default:
      result=NS_OK;
  }//switch
  return result;
}


/**
 *  This method causes all tokens to be dispatched to the given tag handler.
 *
 *  @update  gess 3/25/98
 *  @param   aHandler -- object to receive subsequent tokens...
 *  @return	 error code (usually 0)
 */
nsresult CWellFormedDTD::CaptureTokenPump(nsITagHandler* aHandler) {
  nsresult result=NS_OK;
  return result;
}

/**
 *  This method releases the token-pump capture obtained in CaptureTokenPump()
 *
 *  @update  gess 3/25/98
 *  @param   aHandler -- object that received tokens...
 *  @return	 error code (usually 0)
 */
nsresult CWellFormedDTD::ReleaseTokenPump(nsITagHandler* aHandler){
  nsresult result=NS_OK;
  return result;
}
