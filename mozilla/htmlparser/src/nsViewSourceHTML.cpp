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
 * jce2@po.cwru.edu <Jason Eager>: Added pref to turn on/off 
 *                                 syntax highlighting in view source
 *                                 window.
 *                                
 * 
 */

/**
 * MODULE NOTES:
 * @update  gess 4/8/98
 * 
 *         
 */
 

#ifdef RAPTOR_PERF_METRICS
#  define START_TIMER()                    \
    if(mParser) mParser->mParseTime.Start(PR_FALSE); \
    if(mParser) mParser->mDTDTime.Start(PR_FALSE); 

#  define STOP_TIMER()                     \
    if(mParser) mParser->mParseTime.Stop(); \
    if(mParser) mParser->mDTDTime.Stop(); 

#else
#  define STOP_TIMER() 
#  define START_TIMER()
#endif

#define VIEW_SOURCE_HTML
#define VIEW_SOURCE_COLORING

#include "nsIDTDDebug.h"
#include "nsViewSourceHTML.h"
#include "nsCRT.h"
#include "nsParser.h"
#include "nsScanner.h"
#include "nsIParser.h"
#include "nsTokenHandler.h"
#include "nsDTDUtils.h"
#include "nsIContentSink.h"
#include "nsIHTMLContentSink.h"
#include "nsHTMLTokenizer.h"
#ifdef VIEW_SOURCE_HTML
#include "nsHTMLEntities.h"
#endif // VIEW_SOURCE_HTML

#ifdef VIEW_SOURCE_COLORING
// For Coloring pref only
// If we aren't going to define it, then should save on bloat.
#include "nsIPref.h"
#include "nsIServiceManager.h"
static NS_DEFINE_CID(kPrefCID, NS_PREF_CID);
#endif // VIEW_SOURCE_COLORING

#include "COtherDTD.h"
#include "nsElementTable.h"

#include "prenv.h"  //this is here for debug reasons...
#include "prtypes.h"  //this is here for debug reasons...
#include "prio.h"
#include "plstr.h"

#include "prmem.h"


#ifdef RAPTOR_PERF_METRICS
#include "stopwatch.h"
Stopwatch vsTimer;
#endif


static NS_DEFINE_IID(kClassIID,     NS_VIEWSOURCE_HTML_IID); 
static int gErrorThreshold = 10;

//#define rickgdebug
#ifdef rickgdebug
#include <fstream.h>
  fstream* gDumpFile=0;
#endif

#ifdef VIEW_SOURCE_HTML
static const char* kPreStyle = "font-family: -moz-fixed; font-weight:normal; color:black; padding-top:4px; margin-left:4px;";
#endif // VIEW_SOURCE_HTML

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
nsresult CViewSourceHTML::QueryInterface(const nsIID& aIID, void** aInstancePtr)  
{                                                                        
  if (NULL == aInstancePtr) {                                            
    return NS_ERROR_NULL_POINTER;                                        
  }                                                                      

  if(aIID.Equals(NS_GET_IID(nsISupports)))    {  //do IUnknown...
    *aInstancePtr = (nsIDTD*)(this);                                        
  }
  else if(aIID.Equals(NS_GET_IID(nsIDTD))) {  //do IParser base class...
    *aInstancePtr = (nsIDTD*)(this);                                        
  }
  else if(aIID.Equals(kClassIID)) {  //do this class...
    *aInstancePtr = (CViewSourceHTML*)(this);                                        
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
NS_HTMLPARS nsresult NS_NewViewSourceHTML(nsIDTD** aInstancePtrResult)
{
  CViewSourceHTML* it = new CViewSourceHTML();

  if (it == 0) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  return it->QueryInterface(kClassIID, (void **) aInstancePtrResult);
}


NS_IMPL_ADDREF(CViewSourceHTML)
NS_IMPL_RELEASE(CViewSourceHTML)

/********************************************
 ********************************************/

class CIndirectTextToken : public CTextToken {
public:
  CIndirectTextToken() : CTextToken() {
    mIndirectString=0;
  }
  
  void SetIndirectString(const nsAReadableString& aString) {
    mIndirectString=&aString;
  }

  virtual const nsAReadableString& GetStringValue(void){
    return (nsAReadableString&)*mIndirectString;
  }

  const nsAReadableString* mIndirectString;
};


/*******************************************************************
  Now define the CSharedVSCOntext class...
 *******************************************************************/

class CSharedVSContext {
public:

  CSharedVSContext() : 
    mEndNode(), 
    mStartNode(),
    mTokenNode(),
    mErrorNode(),
    mITextToken(),
    mITextNode(&mITextToken),
    mTextToken(),
    mErrorToken(NS_ConvertASCIItoUCS2("error")),
    mTextNode(&mTextToken){
  }
  
  ~CSharedVSContext() {
  }

  static CSharedVSContext& GetSharedContext() {
    static CSharedVSContext gSharedVSContext;
    return gSharedVSContext;
  }

  nsCParserNode       mEndNode;
  nsCParserNode       mStartNode;
  nsCParserNode       mTokenNode;
  nsCParserNode       mErrorNode;
  CIndirectTextToken  mITextToken;
  nsCParserNode       mITextNode;
  CTextToken          mTextToken;
  CTextToken          mErrorToken;
  nsCParserNode       mTextNode;
};

#ifdef VIEW_SOURCE_HTML
enum {
  VIEW_SOURCE_START_TAG = 0,
  VIEW_SOURCE_END_TAG = 1,
  VIEW_SOURCE_COMMENT = 2,
  VIEW_SOURCE_CDATA = 3,
  VIEW_SOURCE_DOCTYPE = 4,
  VIEW_SOURCE_PI = 5,
  VIEW_SOURCE_ENTITY = 6,
  VIEW_SOURCE_TEXT = 7,
  VIEW_SOURCE_ATTRIBUTE_NAME = 8,
  VIEW_SOURCE_ATTRIBUTE_VALUE = 9,
  VIEW_SOURCE_SUMMARY = 10,
  VIEW_SOURCE_POPUP = 11,
  VIEW_SOURCE_MARKUPDECLARATION = 12
};

#ifdef VIEW_SOURCE_COLORING
static char* kElementStyles[] = {
  "color: purple;  font-weight:bold;",
  "color: purple;  font-weight:bold;",
  "color: green; font-style:italic;",
  "color: #CC0066;",
  "color:steelblue; font-style:italic;",
  "color:orchid; font-style:italic;",
  "color:#FF4500; font-weight:normal;",
  "font-weight: normal;",
  "color: black; font-weight:bold;",
  "color: blue; font-weight:normal;",
  "display:block; background-color:#FFFFCC; width:90%; border:solid; border-width:1pt; font-family: Sans-serif;",
  "font-weight: normal;",
  "color:steelblue; font-style:italic;"
};
#endif

static char* kBeforeText[] = {
  "<",
  "</",
  "",
  "",
  "",
  "",
  "&",
  "",
  "",
  "=",
  "",
  "",
  ""
};

static char* kAfterText[] = {
  ">",
  ">",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  ""
};

#endif // VIEW_SOURCE_HTML

/**
 *  Default constructor
 *  
 *  @update  gess 4/9/98
 *  @param   
 *  @return  
 */
CViewSourceHTML::CViewSourceHTML() : mTags(), mErrors() {
  NS_INIT_REFCNT();

#ifdef VIEW_SOURCE_HTML
  mStartTag = VIEW_SOURCE_START_TAG;
  mEndTag = VIEW_SOURCE_END_TAG;
  mCommentTag = VIEW_SOURCE_COMMENT;
  mCDATATag = VIEW_SOURCE_CDATA;
  mMarkupDeclaration = VIEW_SOURCE_MARKUPDECLARATION;
  mDocTypeTag = VIEW_SOURCE_DOCTYPE;
  mPITag = VIEW_SOURCE_PI;
  mEntityTag = VIEW_SOURCE_ENTITY;
  mText = VIEW_SOURCE_TEXT;
  mKey = VIEW_SOURCE_ATTRIBUTE_NAME;
  mValue = VIEW_SOURCE_ATTRIBUTE_VALUE;
  mSummaryTag = VIEW_SOURCE_SUMMARY;
  mPopupTag = VIEW_SOURCE_POPUP;
#else
  mStartTag.AssignWithConversion("start");
  mEndTag.AssignWithConversion("end");
  mCommentTag.AssignWithConversion("comment");
  mCDATATag.AssignWithConversion("cdata");
  mMarkupDeclaration.AssignWithConversion("markupdeclaration");
  mDocTypeTag.AssignWithConversion("doctype");
  mPITag.AssignWithConversion("pi");
  mEntityTag.AssignWithConversion("entity");
  mText.AssignWithConversion("txt");
  mKey.AssignWithConversion("key");
  mValue.AssignWithConversion("val");
  mSummaryTag.AssignWithConversion("summary");
  mPopupTag.AssignWithConversion("popup");
#endif // VIEW_SOURCE_HTML

  mParser=0;
  mSink=0;
  mLineNumber=0;
  mTokenizer=0;
  mDocType=eHTML3Text;
  mValidator=0;
  mHasOpenRoot=PR_FALSE;
  mInCDATAContainer = PR_FALSE;

  //set this to 1 if you want to see errors in your HTML markup.
  char* theEnvString = PR_GetEnv("MOZ_VALIDATE_HTML"); 
  mShowErrors=PRBool(theEnvString);

#ifdef rickgdebug
  gDumpFile = new fstream("c:/temp/viewsource.xml",ios::trunc);
#endif

}



/**
 *  Default destructor
 *  
 *  @update  gess 4/9/98
 *  @param   
 *  @return  
 */
CViewSourceHTML::~CViewSourceHTML(){
  mParser=0; //just to prove we destructed...

  NS_IF_RELEASE(mTokenizer);

}

/**
 * 
 * @update	gess1/8/99
 * @param 
 * @return
 */
const nsIID& CViewSourceHTML::GetMostDerivedIID(void) const{
  return kClassIID;
}

/**
 * Call this method if you want the DTD to construct a fresh 
 * instance of itself. 
 * @update	gess7/23/98
 * @param 
 * @return
 */
nsresult CViewSourceHTML::CreateNewInstance(nsIDTD** aInstancePtrResult){
  return NS_NewViewSourceHTML(aInstancePtrResult);
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
eAutoDetectResult CViewSourceHTML::CanParse(CParserContext& aParserContext,nsString& aBuffer, PRInt32 aVersion) {
  eAutoDetectResult result=eUnknownDetect;

  if(aParserContext.mMimeType.EqualsWithConversion(kPlainTextContentType) ||
     aParserContext.mMimeType.EqualsWithConversion(kTextCSSContentType)) {
    result=eValidDetect;
  }
  else if(eViewSource==aParserContext.mParserCommand) {
    if(aParserContext.mMimeType.EqualsWithConversion(kXMLTextContentType) ||
       aParserContext.mMimeType.EqualsWithConversion(kRDFTextContentType) ||
       aParserContext.mMimeType.EqualsWithConversion(kHTMLTextContentType) ||
       aParserContext.mMimeType.EqualsWithConversion(kXULTextContentType)) {
      result=ePrimaryDetect;
    }
  }
  return result;
}


/**
  * The parser uses a code sandwich to wrap the parsing process. Before
  * the process begins, WillBuildModel() is called. Afterwards the parser
  * calls DidBuildModel(). 
  * @update	rickg 03.20.2000
  * @param	aParserContext
  * @param	aSink
  * @return	error code (almost always 0)
  */
nsresult CViewSourceHTML::WillBuildModel(  const CParserContext& aParserContext,nsIContentSink* aSink){

  nsresult result=NS_OK;

#ifdef RAPTOR_PERF_METRICS
  vsTimer.Reset();
  NS_START_STOPWATCH(vsTimer);
#endif 

  STOP_TIMER();
#ifdef VIEW_SOURCE_HTML
  mSink=(nsIHTMLContentSink*)aSink;
#else
  mSink=(nsIXMLContentSink*)aSink;
#endif // VIEW_SOURCE_HTML

  if((!aParserContext.mPrevContext) && (mSink)) {

    mTags.Truncate();
    mErrors.AssignWithConversion(" HTML 4.0 Strict-DTD validation (enabled); [Should use Transitional?].\n");

    mValidator=aParserContext.mValidator;
    mDocType=aParserContext.mDocType;
    mMimeType=aParserContext.mMimeType;
    mDTDMode=aParserContext.mDTDMode;
    mParserCommand=aParserContext.mParserCommand;
    mErrorCount=0;
    mTagCount=0;

#ifdef VIEW_SOURCE_HTML
    nsAutoString tag;
    
    tag.AssignWithConversion("HTML");
    CStartToken htmlToken(tag, eHTMLTag_html);
    nsCParserNode htmlNode(&htmlToken,0);
    mSink->OpenHTML(htmlNode);

    tag.AssignWithConversion("BODY");
    CStartToken bodyToken(tag, eHTMLTag_body);
    nsCParserNode bodyNode(&bodyToken,0);
    mSink->OpenBody(bodyNode);
#else
    CCommentToken ssToken(NS_LITERAL_STRING("<?xml version=\"1.0\"?>"));
    nsCParserNode ssNode(&ssToken);
    result= mSink->AddCharacterData(ssNode);
#endif // VIEW_SOURCE_HTML

  #ifdef rickgdebug
    (*gDumpFile) << theHeader << endl;
    (*gDumpFile) << "<viewsource xmlns=\"viewsource\">" << endl;
  #endif
  }


  if(eViewSource!=aParserContext.mParserCommand)
    mDocType=ePlainText;
  else mDocType=aParserContext.mDocType;

  mLineNumber=0;
  result = mSink->WillBuildModel(); 

  START_TIMER();
  return result;
}

/**
  * The parser uses a code sandwich to wrap the parsing process. Before
  * the process begins, WillBuildModel() is called. Afterwards the parser
  * calls DidBuildModel(). 
  * @update	gess5/18/98
  * @param	aFilename is the name of the file being parsed.
  * @return	error code (almost always 0)
  */
NS_IMETHODIMP CViewSourceHTML::BuildModel(nsIParser* aParser,nsITokenizer* aTokenizer,nsITokenObserver* anObserver,nsIContentSink* aSink) {
  nsresult result=NS_OK;

  if(aTokenizer && aParser) {

    nsITokenizer*  oldTokenizer=mTokenizer;
    mTokenizer=aTokenizer;

    if(!mHasOpenRoot) {
#ifdef VIEW_SOURCE_HTML
      nsAutoString tag;
      tag.AssignWithConversion("PRE");
      CStartToken theToken(tag, eHTMLTag_pre);
#else
      //now let's automatically open the root container...
      CStartToken theToken(NS_LITERAL_STRING("viewsource"));
#endif // VIEW_SOURCE_HTML
      nsCParserNode theNode(&theToken,0);
     
      CAttributeToken *theAttr=nsnull;
      nsTokenAllocator* theAllocator=mTokenizer->GetTokenAllocator();
      if(theAllocator) {
#ifdef VIEW_SOURCE_HTML
        theAttr=(CAttributeToken*)theAllocator->CreateTokenOfType(eToken_attribute,eHTMLTag_unknown,NS_ConvertASCIItoUCS2(kPreStyle));
        theAttr->SetKey(NS_LITERAL_STRING("style"));
#else
        theAttr=(CAttributeToken*)theAllocator->CreateTokenOfType(eToken_attribute,eHTMLTag_unknown,NS_LITERAL_STRING("http://www.mozilla.org/viewsource"));
        theAttr->SetKey(NS_LITERAL_STRING("xmlns"));
#endif // VIEW_SOURCE_HTML
      }
      if(theAttr)
        theNode.AddAttribute(theAttr);
      result=mSink->OpenContainer(theNode);
      if(NS_SUCCEEDED(result)) mHasOpenRoot=PR_TRUE;
    }

    while(NS_SUCCEEDED(result)){
      CToken* theToken=mTokenizer->PopToken();
      if(theToken) {
        result=HandleToken(theToken,aParser);
        if(NS_SUCCEEDED(result)) {
          IF_FREE(theToken, mTokenizer->GetTokenAllocator());
        }
        else if(NS_ERROR_HTMLPARSER_BLOCK!=result){
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
 * Call this to display an error summary regarding the page.
 * 
 * @update	rickg 6June2000
 * @return  nsresult
 */
nsresult  CViewSourceHTML::GenerateSummary() {
  nsresult result=NS_OK;

  if(mErrorCount && mTagCount) {

    mErrors.AppendWithConversion("\n\n ");
    mErrors.AppendInt(mErrorCount);
    mErrors.AppendWithConversion(" error(s) detected -- see highlighted portions.\n");

    result=WriteTag(mSummaryTag,mErrors,0,PR_FALSE);
  }

  return result;
}

/**
 * 
 * @update	gess5/18/98
 * @param 
 * @return
 */
NS_IMETHODIMP CViewSourceHTML::DidBuildModel(nsresult anErrorCode,PRBool aNotifySink,nsIParser* aParser,nsIContentSink* aSink){
  nsresult result= NS_OK;

  //ADD CODE HERE TO CLOSE OPEN CONTAINERS...

  if(aParser){

    mParser=(nsParser*)aParser;  //debug XXX
    STOP_TIMER();

#ifdef VIEW_SOURCE_HTML
    mSink=(nsIHTMLContentSink*)aParser->GetContentSink();
#else
    mSink=(nsIXMLContentSink*)aParser->GetContentSink();
#endif // VIEW_SOURCE_HTML
    if((aNotifySink) && (mSink)) {
        //now let's automatically auto-opened containers...

#ifdef rickgdebug
  if(gDumpFile){
    (*gDumpFile) << "</viewsource>" << endl;
    gDumpFile->close();
    delete gDumpFile;
  }
#endif

      if(ePlainText!=mDocType) {
#ifdef VIEW_SOURCE_HTML
        CEndToken theToken(eHTMLTag_pre);
        nsCParserNode preNode(&theToken,0);
        mSink->CloseContainer(preNode);
        
        CEndToken bodyToken(eHTMLTag_body);
        nsCParserNode bodyNode(&bodyToken,0);
        mSink->CloseBody(bodyNode);
        
        CEndToken htmlToken(eHTMLTag_html);
        nsCParserNode htmlNode(&htmlToken,0);
        mSink->CloseHTML(htmlNode);
#else
        //now let's automatically close the root container...

        GenerateSummary();

        CEndToken theToken(NS_LITERAL_STRING("viewsource"));
        nsCParserNode theNode(&theToken,0);
        mSink->CloseContainer(theNode);
#endif // VIEW_SOURCE_HTML
      }
      result = mSink->DidBuildModel(1);
    }

    START_TIMER();

  }

#ifdef RAPTOR_PERF_METRICS
  NS_STOP_STOPWATCH(vsTimer);
  printf("viewsource timer: ");
  vsTimer.Print();
  printf("\n");
#endif 

  return result;
}

/**
 * 
 * @update	gess8/4/98
 * @param 
 * @return
 */
nsTokenAllocator* CViewSourceHTML::GetTokenAllocator(void){
  nsITokenizer* theTokenizer=0;
  nsresult result=GetTokenizer(theTokenizer);

  if (NS_SUCCEEDED(result)) {
    return theTokenizer->GetTokenAllocator();
  }
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
nsresult  CViewSourceHTML::Terminate(nsIParser* aParser) {
  return NS_ERROR_HTMLPARSER_STOPPARSING;
}


/**
 * Retrieve the preferred tokenizer for use by this DTD.
 * @update  gess12/28/98
 * @param   none
 * @return  ptr to tokenizer
 */
nsresult CViewSourceHTML::GetTokenizer(nsITokenizer*& aTokenizer) {
  nsresult result=NS_OK;
  if(!mTokenizer) {

    result=NS_NewHTMLTokenizer(&mTokenizer,eDTDMode_quirks,mDocType,mParserCommand);
  }
  aTokenizer=mTokenizer;
  return result;
}


/**
 * 
 * @update	gess5/18/98
 * @param 
 * @return
 */
NS_IMETHODIMP CViewSourceHTML::WillResumeParse(void){
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
NS_IMETHODIMP CViewSourceHTML::WillInterruptParse(void){
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
PRBool CViewSourceHTML::Verify(nsString& aURLRef,nsIParser* aParser) {
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
void CViewSourceHTML::SetVerification(PRBool aEnabled){
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
PRBool CViewSourceHTML::CanContain(PRInt32 aParent,PRInt32 aChild) const{
  PRBool result=PR_TRUE;
  return result;
}

/**
 * Give rest of world access to our tag enums, so that CanContain(), etc,
 * become useful.
 */
NS_IMETHODIMP CViewSourceHTML::StringTagToIntTag(nsString &aTag, PRInt32* aIntTag) const
{
#ifdef VIEW_SOURCE_HTML
  *aIntTag = nsHTMLTags::LookupTag(aTag);
  return NS_OK;
#else
  return NS_ERROR_NOT_IMPLEMENTED;
#endif // VIEW_SOURCE_HTML
}

NS_IMETHODIMP CViewSourceHTML::IntTagToStringTag(PRInt32 aIntTag, nsString& aTag) const
{
#ifdef VIEW_SOURCE_HTML
  aTag.AssignWithConversion(nsHTMLTags::GetStringValue((nsHTMLTag)aIntTag));
  return NS_OK;
#else
  return NS_ERROR_NOT_IMPLEMENTED;
#endif // VIEW_SOURCE_HTML
}

NS_IMETHODIMP CViewSourceHTML::ConvertEntityToUnicode(const nsString& aEntity, PRInt32* aUnicode) const
{
#ifdef VIEW_SOURCE_HTML
  *aUnicode = nsHTMLEntities::EntityToUnicode(aEntity);
  return NS_OK;
#else
  return NS_ERROR_NOT_IMPLEMENTED;
#endif // VIEW_SOURCE_HTML
}


PRBool CViewSourceHTML::IsBlockElement(PRInt32 aTagID,PRInt32 aParentID) const {
  PRBool result=PR_FALSE;
  return result;
}

PRBool CViewSourceHTML::IsInlineElement(PRInt32 aTagID,PRInt32 aParentID) const {
  PRBool result=PR_FALSE;
  return result;
}

/**
 *  This method gets called to determine whether a given 
 *  tag is itself a container
 *  
 *  @update  gess 3/25/98
 *  @param   aTag -- tag to test for containership
 *  @return  PR_TRUE if given tag can contain other tags
 */
PRBool CViewSourceHTML::IsContainer(PRInt32 aTag) const{
  PRBool result=PR_TRUE;
  return result;
}

/**
 *  This method gets called when a tag needs to write it's attributes
 *  
 *  @update  gess 3/25/98
 *  @param   
 *  @return  result status
 */
nsresult CViewSourceHTML::WriteAttributes(PRInt32 attrCount) {
  nsresult result=NS_OK;
  
  if(attrCount){ //go collect the attributes...

    CSharedVSContext& theContext=CSharedVSContext::GetSharedContext();

    int attr=0;
    for(attr=0;attr<attrCount;attr++){
      CToken* theToken=mTokenizer->PeekToken();
      if(theToken)  {
        eHTMLTokenTypes theType=eHTMLTokenTypes(theToken->GetTokenType());
        if(eToken_attribute==theType){
          mTokenizer->PopToken(); //pop it for real...
          theContext.mTokenNode.AddAttribute(theToken);  //and add it to the node.

          CAttributeToken* theAttrToken=(CAttributeToken*)theToken;
          const nsAReadableString& theKey=theAttrToken->GetKey();

          result=WriteTag(mKey,theKey,0,PR_FALSE);
          const nsString& theValue=theAttrToken->GetValue();

          if((0<theValue.Length()) || (theAttrToken->mHasEqualWithoutValue)){
            result=WriteTag(mValue,theValue,0,PR_FALSE);
          }
        } 
      }
      else return kEOF;
    }
  }

  return result;
}

/**
 *  This method gets called when a tag needs to be sent out
 *  
 *  @update  gess 3/25/98
 *  @param   
 *  @return  result status
 */
#ifdef VIEW_SOURCE_HTML
nsresult CViewSourceHTML::WriteTag(PRInt32 aTagType,const nsAReadableString & aText,PRInt32 attrCount,PRBool aNewlineRequired) {
#else
nsresult CViewSourceHTML::WriteTag(nsString &theXMLTagName,const nsAReadableString & aText,PRInt32 attrCount,PRBool aNewlineRequired) {
#endif // VIEW_SOURCE_HTML
  static nsString       theString;

  nsresult result=NS_OK;

  CSharedVSContext& theContext=CSharedVSContext::GetSharedContext();
#ifdef VIEW_SOURCE_COLORING
  // This determines the value of the boolean syntax_highlight preference.
  PRBool syntaxHighlight = PR_FALSE;
  NS_WITH_SERVICE(nsIPref, thePrefsService, kPrefCID, &result);
  if (NS_SUCCEEDED(result) && thePrefsService)
      thePrefsService->GetBoolPref("view_source.syntax_highlight", &syntaxHighlight);
#endif // VIEW_SOURCE_COLORING

#ifdef VIEW_SOURCE_HTML
  if (kBeforeText[aTagType][0] != 0) {
    nsAutoString beforeText;
    beforeText.AssignWithConversion(kBeforeText[aTagType]);
    theContext.mITextToken.SetIndirectString(beforeText);
    mSink->AddLeaf(theContext.mITextNode);
  }

#ifdef VIEW_SOURCE_COLORING
  nsAutoString tag (NS_LITERAL_STRING("SPAN"));
  CStartToken theTagToken(tag, eHTMLTag_span);
#endif // VIEW_SOURCE_COLORING
#else
  CStartToken theTagToken(theXMLTagName);
#endif // VIEW_SOURCE_HTML

#ifdef VIEW_SOURCE_COLORING
  if (syntaxHighlight)
  {
  	theContext.mStartNode.Init(&theTagToken,mLineNumber);
#ifdef VIEW_SOURCE_HTML
    nsTokenAllocator* theAllocator=mTokenizer->GetTokenAllocator();
    if(theAllocator) {
      CAttributeToken* theAttr=(CAttributeToken*)theAllocator->CreateTokenOfType(eToken_attribute,eHTMLTag_unknown,NS_ConvertASCIItoUCS2(kElementStyles[aTagType]));
      theAttr->SetKey(NS_LITERAL_STRING("style"));
      theContext.mStartNode.AddAttribute(theAttr);
    }
#endif // VIEW_SOURCE_HTML

  STOP_TIMER();

  mSink->OpenContainer(theContext.mStartNode);  //emit <starttag>...
  }
#endif // VIEW_SOURCE_COLORING

#ifdef rickgdebug

      if(aNewlineRequired) {
        (*gDumpFile)<<endl;
      }

      nsCAutoString cstr(theXMLTagName);
      (*gDumpFile) << "<" << cstr << ">";
      cstr.Assign(aText);
      (*gDumpFile) << cstr;
#endif
  

  theContext.mITextToken.SetIndirectString(aText);  //now emit the tag name...

  mSink->AddLeaf(theContext.mITextNode);

  if(attrCount){
    result=WriteAttributes(attrCount);
  }

#ifdef VIEW_SOURCE_HTML
#ifdef VIEW_SOURCE_COLORING
  if (syntaxHighlight)
  {
    theContext.mStartNode.ReleaseAll(); 
    CEndToken theEndToken(eHTMLTag_span);
    theContext.mEndNode.Init(&theEndToken,mLineNumber);
    mSink->CloseContainer(theContext.mEndNode);  //emit </starttag>...
  }
#endif // VIEW_SOURCE_COLORING
  if (kAfterText[aTagType][0] != 0) {
    nsAutoString afterText;
    afterText.AssignWithConversion(kAfterText[aTagType]);
    theContext.mITextToken.SetIndirectString(afterText);
    mSink->AddLeaf(theContext.mITextNode);
  }
#else
  theContext.mEndNode.Init(&theTagToken,mLineNumber);
  mSink->CloseContainer(theContext.mEndNode);  //emit </starttag>...
#endif // VIEW_SOURCE_HTML

#ifdef rickgdebug
      cstr.Assign(theXMLTagName);
      (*gDumpFile) << "</" << cstr << ">";
#endif

  START_TIMER();

  return result;
}

/**
 *  This method gets called when a tag needs to be sent out but is known to be misplaced (error)
 *  
 *  @update  gess 6June2000 -- 
 *  @param   
 *  @return  result status
 */
#ifdef VIEW_SOURCE_HTML
nsresult CViewSourceHTML::WriteTagWithError(PRInt32 aTagType,const nsAReadableString& aStr,PRInt32 attrCount,PRBool aNewlineRequired) {
#else
nsresult CViewSourceHTML::WriteTagWithError(nsString &theXMLTagName,const nsAReadableString& aStr,PRInt32 attrCount,PRBool aNewlineRequired) {
#endif // VIEW_SOURCE_HTML

  STOP_TIMER();

  CSharedVSContext& theContext=CSharedVSContext::GetSharedContext();
  nsresult result=NS_OK;
  
  if(ePlainText!=mDocType) {

    //first write the error tag itself...

    theContext.mErrorNode.Init(&theContext.mErrorToken,mLineNumber);
    result=mSink->OpenContainer(theContext.mErrorNode);  //emit <error>...
  }

#ifdef VIEW_SOURCE_HTML
    //now write the tag from the source file...
  result=WriteTag(aTagType,aStr,attrCount,aNewlineRequired);
#else
  result=WriteTag(theXMLTagName,aStr,attrCount,aNewlineRequired);
#endif // VIEW_SOURCE_HTML

  if(ePlainText!=mDocType) {

   //now close the error tag...
    STOP_TIMER();
    theContext.mErrorNode.Init(&theContext.mErrorToken,mLineNumber);
    mSink->CloseContainer(theContext.mErrorNode);
    START_TIMER();
  }

  return result; 
}

void CViewSourceHTML::AddContainmentError(eHTMLTags aChildTag,eHTMLTags aParentTag,PRInt32 aLineNumber) {

  if (mShowErrors) {
    mErrorCount++;

    if(mErrorCount<=gErrorThreshold) {

      char theChildMsg[100];
      if(eHTMLTag_text==aChildTag) 
        strcpy(theChildMsg,"text");
      else sprintf(theChildMsg,"<%s>",nsHTMLTags::GetCStringValue(aChildTag));

      char theMsg[256];
      sprintf(theMsg,"\n -- Line (%i) error: %s is not a legal child of <%s>",
              aLineNumber,theChildMsg,nsHTMLTags::GetCStringValue(aParentTag));

      mErrors.AppendWithConversion(theMsg);
    }
    else if(gErrorThreshold+1==mErrorCount){
      mErrors.AppendWithConversion("\n -- Too many errors -- terminating output.");
    }
  }

}

/**
 *  
 *  @update  gess 3/25/98
 *  @param   aToken -- token object to be put into content model
 *  @return  0 if all is well; non-zero is an error
 */
NS_IMETHODIMP CViewSourceHTML::HandleToken(CToken* aToken,nsIParser* aParser) {
  nsresult        result=NS_OK;
  CHTMLToken*     theToken= (CHTMLToken*)(aToken);
  eHTMLTokenTypes theType= (eHTMLTokenTypes)theToken->GetTokenType();
 
  mParser=(nsParser*)aParser;
#ifdef VIEW_SOURCE_HTML
  mSink=(nsIHTMLContentSink*)aParser->GetContentSink();
#else
  mSink=(nsIXMLContentSink*)aParser->GetContentSink();
#endif // VIEW_SOURCE_HTML
 
  CSharedVSContext& theContext=CSharedVSContext::GetSharedContext();
  theContext.mTokenNode.Init(theToken,mLineNumber);

  eHTMLTags theParent=(mTags.Length()) ? (eHTMLTags)mTags.Last() : eHTMLTag_unknown;
  eHTMLTags theChild=(eHTMLTags)aToken->GetTypeID();

  switch(theType) {
    
    case eToken_start:
      {
        mTagCount++;

        if (gHTMLElements[theChild].CanContainType(kCDATA)) {
          mInCDATAContainer = PR_TRUE;
        }

        const nsAReadableString& startValue = aToken->GetStringValue();
        if(mShowErrors) {
          PRBool theChildIsValid=PR_TRUE;
          if(mValidator) {
            theChildIsValid=mValidator->CanContain(theParent,theChild);
            if(theChildIsValid) {
              if(mValidator->IsContainer(theChild))
                mTags.Append(PRUnichar(theChild));
            }
          }
          
          if(theChildIsValid)
            result=WriteTag(mStartTag,startValue,aToken->GetAttributeCount(),PR_TRUE);
          else {
            AddContainmentError(theChild,theParent,mLineNumber);
            result=WriteTagWithError(mStartTag,startValue,aToken->GetAttributeCount(),PR_TRUE);
          }
        }
        else result=WriteTag(mStartTag,startValue,aToken->GetAttributeCount(),PR_TRUE);

        if((ePlainText!=mDocType) && mParser && (NS_OK==result)) {
          CObserverService* theService=mParser->GetObserverService();
          if(theService) {
            eHTMLTags theTag=(eHTMLTags)theToken->GetTypeID(); 
            const nsISupportsParserBundle*  bundle=mParser->GetParserBundle();
            result=theService->Notify(theTag,theContext.mTokenNode,(void*)bundle, mMimeType, mParser);
          }
        }
      }
      break;

    case eToken_end:
      {
        if (gHTMLElements[theChild].CanContainType(kCDATA)) {
          mInCDATAContainer = PR_FALSE;
        }

        if(theParent==theChild) {
          mTags.Truncate(mTags.Length()-1);
        }
        const nsAReadableString& endValue = aToken->GetStringValue();
        result=WriteTag(mEndTag,endValue,0,PR_TRUE);
      }
      break;

    case eToken_cdatasection:
      {
        nsAutoString theStr;
        theStr.Assign(NS_LITERAL_STRING("<!"));
        theStr.Append(aToken->GetStringValue());
        theStr.Append(NS_LITERAL_STRING(">"));
        result=WriteTag(mCDATATag,theStr,0,PR_TRUE);
      }
      break;

    case eToken_markupDecl:
      {
        nsAutoString theStr;
        theStr.Assign(NS_LITERAL_STRING("<!"));
        theStr.Append(aToken->GetStringValue());
        theStr.Append(NS_LITERAL_STRING(">"));
        result=WriteTag(mMarkupDeclaration,theStr,0,PR_TRUE);
      }
      break;

    case eToken_comment: 
      {
        const nsAReadableString& commentValue = aToken->GetStringValue();
        result=WriteTag(mCommentTag,commentValue,0,PR_TRUE);
      }
      break;

    case eToken_doctypeDecl:
      {
        const nsAReadableString& doctypeValue = aToken->GetStringValue();
        result=WriteTag(mDocTypeTag,doctypeValue,0,PR_TRUE);
      }
      break;

    case eToken_newline:
      {
        const nsAReadableString& newlineValue = aToken->GetStringValue();
        mLineNumber++; 
        result=WriteTag(mText,newlineValue,0,PR_FALSE);
      }
      break;

    case eToken_whitespace:
      {
        const nsAReadableString& wsValue = aToken->GetStringValue();
        result=WriteTag(mText,wsValue,0,PR_FALSE);
      }
      break;

    case eToken_text:
      {
        if(mShowErrors) {
          const nsAReadableString& str = aToken->GetStringValue();
          if((0==mValidator) || 
             mValidator->CanContain(theParent,eHTMLTag_text))
            result=WriteTag(mText,str,aToken->GetAttributeCount(),PR_TRUE);
          else {
            AddContainmentError(eHTMLTag_text,theParent,mLineNumber);
            result=WriteTagWithError(mText,str,aToken->GetAttributeCount(),PR_FALSE);
          }
        }
        else if (mInCDATAContainer) {
          // Fix bug 40809
          nsAutoString theStr;
          aToken->GetSource(theStr);
          theStr.ReplaceSubstring(NS_ConvertASCIItoUCS2("\r\n"), NS_ConvertASCIItoUCS2("\n"));
          theStr.ReplaceChar(kCR,kLF);  
          result=WriteTag(mText,theStr,aToken->GetAttributeCount(),PR_TRUE);
        }
        else {
          const nsAReadableString& str = aToken->GetStringValue();         
          result=WriteTag(mText,str,aToken->GetAttributeCount(),PR_TRUE);
        }
      }

      break;

    case eToken_entity:
      {
        nsAutoString theStr;
        theStr.Assign(aToken->GetStringValue());
        if(!theStr.EqualsWithConversion("XI",IGNORE_CASE)) {
          PRUnichar theChar=theStr.CharAt(0);
          if((nsCRT::IsAsciiDigit(theChar)) || ('X'==theChar) || ('x'==theChar)){
            theStr.InsertWithConversion("#", 0);
          }
        }
        result=WriteTag(mEntityTag,theStr,0,PR_FALSE);
      }
      break;

    case eToken_instruction:
      result=WriteTag(mPITag,aToken->GetStringValue(),0,PR_TRUE);

    default:
      result=NS_OK;
  }//switch

  theContext.mTokenNode.ReleaseAll(); 

  return result;
}

