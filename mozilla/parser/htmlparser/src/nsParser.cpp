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
  


#include "nsParser.h"
#include "nsIContentSink.h" 
#include "nsString.h"
#include "nsCRT.h"
#include "nsScanner.h"
#include "prenv.h"  //this is here for debug reasons...
#include "plstr.h"
#include <fstream.h>
#include "nsIParserFilter.h"
#include "nsIDTDDebug.h"
#include "nshtmlpars.h"

#undef rickgdebug
#ifdef  rickgdebug
#include "CRtfDTD.h"
#include "nsWellFormedDTD.h"
#endif


static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);                 
static NS_DEFINE_IID(kClassIID, NS_PARSER_IID); 
static NS_DEFINE_IID(kIParserIID, NS_IPARSER_IID);
static NS_DEFINE_IID(kIStreamListenerIID, NS_ISTREAMLISTENER_IID);

static const char* kNullURL = "Error: Null URL given";
static nsString    kUnknownFilename("unknown");

static const int  gTransferBufferSize=4096;  //size of the buffer used in moving data from iistream



/**
 *  This method is defined in nsIParser. It is used to 
 *  cause the COM-like construction of an nsParser.
 *  
 *  @update  gess 3/25/98
 *  @param   nsIParser** ptr to newly instantiated parser
 *  @return  NS_xxx error result
 */
NS_HTMLPARS nsresult NS_NewParser(nsIParser** aInstancePtrResult)
{
  nsParser *it = new nsParser();

  if (it == 0) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  return it->QueryInterface(kIParserIID, (void **) aInstancePtrResult);
}

class CTokenDeallocator: public nsDequeFunctor{
public:
  virtual void* operator()(void* anObject) {
    CToken* aToken = (CToken*)anObject;
    delete aToken;
    return 0;
  }
};

CTokenDeallocator gTokenDeallocator2;

class CDTDDeallocator: public nsDequeFunctor{
public:
  virtual void* operator()(void* anObject) {
    nsIDTD* aDTD =(nsIDTD*)anObject;
    NS_RELEASE(aDTD);
    return 0;
  }
};

class CDTDFinder: public nsDequeFunctor{
public:
  CDTDFinder(nsIDTD* aDTD) {
    NS_IF_ADDREF(mTargetDTD=aDTD);
  }
  virtual ~CDTDFinder() {
    NS_IF_RELEASE(mTargetDTD);
  }
  virtual void* operator()(void* anObject) {
    return (anObject==(void*)mTargetDTD) ? anObject : 0;
  }
  nsIDTD* mTargetDTD;
};

class CSharedParserObjects {
public:

  CSharedParserObjects() : mDeallocator(), mDTDDeque(mDeallocator) {
  }

  ~CSharedParserObjects() {
  }

  void RegisterDTD(nsIDTD* aDTD){
    CDTDFinder theFinder(aDTD);
    if(!mDTDDeque.ForEach(theFinder))
      mDTDDeque.Push(aDTD);
  }
  
  nsIDTD*  FindDTD(nsIDTD* aDTD){
    return 0;
  }

  CDTDDeallocator mDeallocator;
  nsDeque         mDTDDeque;
};

CSharedParserObjects gSharedParserObjects;


/**
 *  default constructor
 *  
 *  @update  gess 3/25/98
 *  @param   
 *  @return  
 */
nsParser::nsParser() {
  NS_INIT_REFCNT();
  mDTDDebug = 0;
  mParserFilter = 0;
  mObserver = 0;
  mSink=0;
  mParserContext=0;
}


/**
 *  Default destructor
 *  
 *  @update  gess 3/25/98
 *  @param   
 *  @return  
 */
nsParser::~nsParser() {
  NS_IF_RELEASE(mObserver);
//  NS_IF_RELEASE(mDTDDebug);
  NS_RELEASE(mSink);

  //don't forget to add code here to delete 
  //what may be several contexts...
  delete mParserContext;
}


NS_IMPL_ADDREF(nsParser)
NS_IMPL_RELEASE(nsParser)
//NS_IMPL_ISUPPORTS(nsParser,NS_IHTML_PARSER_IID)


/**
 *  This method gets called as part of our COM-like interfaces.
 *  Its purpose is to create an interface to parser object
 *  of some type.
 *  
 *  @update   gess 3/25/98
 *  @param    nsIID  id of object to discover
 *  @param    aInstancePtr ptr to newly discovered interface
 *  @return   NS_xxx result code
 */
nsresult nsParser::QueryInterface(const nsIID& aIID, void** aInstancePtr)  
{                                                                        
  if (NULL == aInstancePtr) {                                            
    return NS_ERROR_NULL_POINTER;                                        
  }                                                                      

  if(aIID.Equals(kISupportsIID))    {  //do IUnknown...
    *aInstancePtr = (nsIParser*)(this);                                        
  }
  else if(aIID.Equals(kIParserIID)) {  //do IParser base class...
    *aInstancePtr = (nsIParser*)(this);                                        
  }
  else if(aIID.Equals(kIStreamListenerIID)) {  //do IStreamListener base class...
    *aInstancePtr = (nsIStreamListener*)(this);                                        
  }
  else if(aIID.Equals(kClassIID)) {  //do this class...
    *aInstancePtr = (nsParser*)(this);                                        
  }                 
  else {
    *aInstancePtr=0;
    return NS_NOINTERFACE;
  }
  ((nsISupports*) *aInstancePtr)->AddRef();
  return NS_OK;                                                        
}


/**
 * 
 * @update	gess6/18/98
 * @param 
 * @return
 */
nsIParserFilter * nsParser::SetParserFilter(nsIParserFilter * aFilter)
{
  nsIParserFilter* old=mParserFilter;
  if(old)
    NS_RELEASE(old);
  if(aFilter) {
    mParserFilter=aFilter;
    NS_ADDREF(aFilter);
  }
  return old;
}

/**
 *  This method gets called in order to set the content
 *  sink for this parser to dump nodes to.
 *  
 *  @update  gess 3/25/98
 *  @param   nsIContentSink interface for node receiver
 *  @return  
 */
nsIContentSink* nsParser::SetContentSink(nsIContentSink* aSink) {
  NS_PRECONDITION(0!=aSink,"sink cannot be null!");
  nsIContentSink* old=mSink;
  if(old)
    NS_RELEASE(old);
  if(aSink) {
    mSink=aSink;
    NS_ADDREF(aSink);
  }
  return old;
}

/**
 *  Call this static method when you want to
 *  register your dynamic DTD's with the parser.
 *  
 *  @update  gess 6/9/98
 *  @param   aDTD  is the object to be registered.
 *  @return  nothing.
 */
void nsParser::RegisterDTD(nsIDTD* aDTD){

#ifdef rickgdebug
  nsIDTD* rv=0;
  NS_NewRTF_DTD(&rv);
  gSharedParserObjects.RegisterDTD(rv);
  NS_NewWellFormed_DTD(&rv);
  gSharedParserObjects.RegisterDTD(rv);
#endif

  gSharedParserObjects.RegisterDTD(aDTD);
}

/**
 *  
 *  
 *  @update  gess 6/9/98
 *  @param   
 *  @return  
 */
CScanner* nsParser::GetScanner(void){
  if(mParserContext)
    return mParserContext->mScanner;
  return 0;
}

/**
 *  
 *  
 *  @update  gess 5/13/98
 *  @param   
 *  @return  
 */
eParseMode DetermineParseMode() {
  const char* theModeStr= PR_GetEnv("PARSE_MODE");
  const char* other="other";
  
  eParseMode  result=eParseMode_navigator;

  if(theModeStr) 
    if(0==nsCRT::strcasecmp(other,theModeStr))
      result=eParseMode_other;    
  return result;
}

/**
 *  
 *  
 *  @update  gess 5/13/98
 *  @param   
 *  @return  
 */
PRBool FindSuitableDTD( CParserContext& aParserContext) {

    //Let's start by tring the defaultDTD, if one exists...
  if(aParserContext.mDTD && (aParserContext.mDTD->CanParse(aParserContext.mSourceType,0)))
    return PR_TRUE;

  PRBool  result=PR_FALSE;

  nsDequeIterator b=gSharedParserObjects.mDTDDeque.Begin(); 
  nsDequeIterator e=gSharedParserObjects.mDTDDeque.End(); 

  while(b<e){
    nsIDTD* theDTD=(nsIDTD*)b.GetCurrent();
    if(theDTD) {
      result=theDTD->CanParse(aParserContext.mSourceType,0); 
      if(result){
        theDTD->CreateNewInstance(&aParserContext.mDTD);
        break;
      }
    }
    b++;
  } 

  return result;
}

/**
 * Call this method if you want the known DTD's to try 
 * to detect the document type based through analysis
 * of the underlying stream.
 *
 * @update	gess6/22/98
 * @param   aBuffer -- nsString containing sample data to be analyzed.
 * @param   aType -- may hold typename given from netlib; will hold result given by DTD's.
 * @return  auto-detect result: eValid, eInvalid, eUnknown
 */
eAutoDetectResult nsParser::AutoDetectContentType(nsString& aBuffer,nsString& aType) {

    //The process:
    //  You should go out and ask each DTD if they
    //  recognize the content in the scanner.
    //  Somebody should say yes, or we can't continue.

    //This method may change mSourceType and mParserContext->mDTD.
    //It absolutely changes mParserContext->mAutoDetectStatus

  nsDequeIterator b=gSharedParserObjects.mDTDDeque.Begin(); 
  nsDequeIterator e=gSharedParserObjects.mDTDDeque.End(); 

  mParserContext->mAutoDetectStatus=eUnknownDetect;
  while((b<e) && (eUnknownDetect==mParserContext->mAutoDetectStatus)){
    nsIDTD* theDTD=(nsIDTD*)b.GetCurrent();
    if(theDTD) {
      mParserContext->mAutoDetectStatus=theDTD->AutoDetectContentType(aBuffer,aType);
    }
    b++;
  } 

  return mParserContext->mAutoDetectStatus;  
}


/**
 * This gets called just prior to the model actually
 * being constructed. It's important to make this the
 * last thing that happens right before parsing, so we
 * can delay until the last moment the resolution of
 * which DTD to use (unless of course we're assigned one).
 *
 * @update	gess5/18/98
 * @param 
 * @return  error code -- 0 if ok, non-zero if error.
 */
PRInt32 nsParser::WillBuildModel(nsString& aFilename){

  mMajorIteration=-1;
  mMinorIteration=-1;

  mParserContext->mParseMode=DetermineParseMode();  
  if(PR_TRUE==FindSuitableDTD(*mParserContext)) {
    mParserContext->mDTD->SetParser(this);
    mParserContext->mDTD->SetContentSink(mSink);
    mParserContext->mDTD->WillBuildModel(aFilename,PRBool(0==mParserContext->mPrevContext));
  }

  return kNoError;
}

/**
 * 
 * @update	gess5/18/98
 * @param 
 * @return  error code -- 0 if ok, non-zero if error.
 */
PRInt32 nsParser::DidBuildModel(PRInt32 anErrorCode) {
  //One last thing...close any open containers.
  PRInt32 result=anErrorCode;
  if(mParserContext->mDTD) {
    result=mParserContext->mDTD->DidBuildModel(anErrorCode,PRBool(0==mParserContext->mPrevContext));
  }

  return result;
}


/**
 * This method adds a new parser context to the list,
 * pushing the current one to the next position.
 * @update	gess7/22/98
 * @param   ptr to new context
 * @return  nada
 */
void nsParser::PushContext(CParserContext& aContext) {
  aContext.mPrevContext=mParserContext;
  mParserContext=&aContext;
}

/**
 * This method pops the topmost context off the stack,
 * returning it to the user. The next context  (if any)
 * becomes the current context.
 * @update	gess7/22/98
 * @return  prev. context
 */
CParserContext* nsParser::PopContext() {
  CParserContext* oldContext=mParserContext;
  if(oldContext) {
    mParserContext=oldContext->mPrevContext;
  }
  return oldContext;
}


/**
 *  This is the main controlling routine in the parsing process. 
 *  Note that it may get called multiple times for the same scanner, 
 *  since this is a pushed based system, and all the tokens may 
 *  not have been consumed by the scanner during a given invocation 
 *  of this method. 
 *
 *  NOTE: We don't call willbuildmodel here, because it will happen
 *        as a result of calling OnStartBinding later on.
 *
 *  @update  gess 3/25/98
 *  @param   aFilename -- const char* containing file to be parsed.
 *  @return  error code -- 0 if ok, non-zero if error.
 */
PRInt32 nsParser::Parse(nsIURL* aURL,nsIStreamObserver* aListener, nsIDTDDebug * aDTDDebug) {
  NS_PRECONDITION(0!=aURL,kNullURL);

  PRInt32 status=kBadURL;

/* Disable DTD Debug for now...
  mDTDDebug = aDTDDebug;
  NS_IF_ADDREF(mDTDDebug);
*/
 
  if(aURL) {
    nsAutoString theName(aURL->GetSpec());
    CParserContext* cp=new CParserContext(new CScanner(theName,PR_FALSE),aURL,aListener);
    PushContext(*cp);
    status=NS_OK;
  }
  return status;
}


/**
 * Cause parser to parse input from given stream 
 * @update	gess5/11/98
 * @param   aStream is the i/o source
 * @return  error code -- 0 if ok, non-zero if error.
 */
PRInt32 nsParser::Parse(fstream& aStream){

  PRInt32 status=kNoError;
  
  //ok, time to create our tokenizer and begin the process
  CParserContext* pc=new CParserContext(new CScanner(kUnknownFilename,aStream,PR_FALSE),&aStream,0);
  PushContext(*pc);

  mParserContext->mScanner->Eof();
  if(eValidDetect==AutoDetectContentType(mParserContext->mScanner->GetBuffer(),
                                         mParserContext->mSourceType)) {
    WillBuildModel(mParserContext->mScanner->GetFilename());
    status=ResumeParse();
    DidBuildModel(status);
  } //if

  pc=PopContext();
  delete pc;

  return status;
}


/**
 * Call this method if all you want to do is parse 1 string full of HTML text.
 * In particular, this method should be called by the DOM when it has an HTML
 * string to feed to the parser in real-time.
 *
 * @update	gess5/11/98
 * @param   aSourceBuffer contains a string-full of real content
 * @param   anHTMLString tells us whether we should assume the content is HTML (usually true)
 * @return  error code -- 0 if ok, non-zero if error.
 */
PRInt32 nsParser::Parse(nsString& aSourceBuffer,PRBool anHTMLString){
  PRInt32 result=kNoError;

  CParserContext* pc=new CParserContext(new CScanner(aSourceBuffer),&aSourceBuffer,0);

  PushContext(*pc);
  if(PR_TRUE==anHTMLString)
    pc->mSourceType="text/html";
  if(eValidDetect==AutoDetectContentType(aSourceBuffer,mParserContext->mSourceType)) {
    WillBuildModel(mParserContext->mScanner->GetFilename());
    result=ResumeParse();
    DidBuildModel(result);
  }
  pc=PopContext();
  delete pc;
  return result;
}

/**
 *  This routine is called to cause the parser to continue
 *  parsing it's underling stream. This call allows the
 *  parse process to happen in chunks, such as when the
 *  content is push based, and we need to parse in pieces.
 *  
 *  @update  gess 3/25/98
 *  @param   
 *  @return  error code -- 0 if ok, non-zero if error.
 */
PRInt32 nsParser::ResumeParse() {
  PRInt32 result=kNoError;

  mParserContext->mDTD->WillResumeParse();
  if(kNoError==result) {
    result=Tokenize();
    BuildModel();
    if(kInterrupted==result)
      mParserContext->mDTD->WillInterruptParse();
  }
  return result;
}

/**
 *  This is where we loop over the tokens created in the 
 *  tokenization phase, and try to make sense out of them. 
 *
 *  @update  gess 3/25/98
 *  @param   
 *  @return  error code -- 0 if ok, non-zero if error.
 */
PRInt32 nsParser::BuildModel() {
  
  nsDequeIterator e=mParserContext->mTokenDeque.End(); 
  nsDequeIterator theMarkPos(e);

  if(!mParserContext->mCurrentPos)
    mParserContext->mCurrentPos=new nsDequeIterator(mParserContext->mTokenDeque.Begin());

    //Get the root DTD for use in model building...
  CParserContext* theRootContext=mParserContext;
  while(theRootContext->mPrevContext)
    theRootContext=theRootContext->mPrevContext;

  nsIDTD* theRootDTD=theRootContext->mDTD;
  
  PRInt32 result=kNoError;
  while((kNoError==result) && ((*mParserContext->mCurrentPos<e))){
    mMinorIteration++;
    CToken* theToken=(CToken*)mParserContext->mCurrentPos->GetCurrent();
 
    /**************************************************************************
      The point of this code to serve as a testbed for parser reentrancy.
      If you set recurse=1, we go reentrant, passing a text string of HTML onto 
      the parser for inline processing, just like javascript/DOM would do it.
      And guess what? It worked the first time!
      Uncomment the following code to enable the test: 

      int recurse=0;
      if(recurse){
          nsString theString("  this is just a great big empty string  ");
//        nsString theString("<P>doc.write");
//        nsString theString("<table border=1><tr><td BGCOLOR=blue>cell</td></tr></table>");
        Parse(theString,PR_TRUE);
      }
      */

    theMarkPos=*mParserContext->mCurrentPos;
    
    result=theRootDTD->HandleToken(theToken);
//    result=mParserContext->mDTD->HandleToken(theToken);
    ++(*mParserContext->mCurrentPos);
  }

  if(kInterrupted==result)
    *mParserContext->mCurrentPos=theMarkPos;

  return result;
}

/**
 * Retrieve the attributes for this node, and add then into
 * the node.
 *
 * @update  gess4/22/98
 * @param   aNode is the node you want to collect attributes for
 * @param   aCount is the # of attributes you're expecting
 * @return error code (should be 0)
 */
PRInt32 nsParser::CollectAttributes(nsCParserNode& aNode,PRInt32 aCount){
  nsDequeIterator end=mParserContext->mTokenDeque.End();

  int attr=0;
  for(attr=0;attr<aCount;attr++) {
    if(*mParserContext->mCurrentPos<end) {
      CToken* tkn=(CToken*)(++(*mParserContext->mCurrentPos));
      if(tkn){
        if(eToken_attribute==eHTMLTokenTypes(tkn->GetTokenType())){
          aNode.AddAttribute(tkn);
        } 
        else (*mParserContext->mCurrentPos)--;
      }
      else return kInterrupted;
    }
    else return kInterrupted;
  }
  return kNoError;
}


/**
 * Causes the next skipped-content token (if any) to
 * be consumed by this node.
 * @update	gess5/11/98
 * @param   node to consume skipped-content
 * @param   holds the number of skipped content elements encountered
 * @return  Error condition.
 */
PRInt32 nsParser::CollectSkippedContent(nsCParserNode& aNode,PRInt32& aCount) {
  eHTMLTokenTypes   subtype=eToken_attribute;
  nsDequeIterator   end=mParserContext->mTokenDeque.End();
  PRInt32           result=kNoError;

  aCount=0;
  while((*mParserContext->mCurrentPos!=end) && (eToken_attribute==subtype)) {
    CToken* tkn=(CToken*)(++(*mParserContext->mCurrentPos));
    subtype=eHTMLTokenTypes(tkn->GetTokenType());
    if(eToken_skippedcontent==subtype) {
      aNode.SetSkippedContent(tkn);
      aCount++;
    } 
    else (*mParserContext->mCurrentPos)--;
  }
  return result;
}

/*******************************************************************
  These methods are used to talk to the netlib system...
 *******************************************************************/

/**
 *  
 *  
 *  @update  gess 5/12/98
 *  @param   
 *  @return  error code -- 0 if ok, non-zero if error.
 */
nsresult nsParser::GetBindInfo(nsIURL* aURL){
  nsresult result=0;
  return result;
}

/**
 *  
 *  
 *  @update  gess 5/12/98
 *  @param   
 *  @return  error code -- 0 if ok, non-zero if error.
 */
nsresult
nsParser::OnProgress(nsIURL* aURL, PRInt32 aProgress, PRInt32 aProgressMax)
{
  nsresult result=0;
  if (nsnull != mObserver) {
    mObserver->OnProgress(aURL, aProgress, aProgressMax);
  }
  return result;
}

/**
 *  
 *  
 *  @update  gess 5/12/98
 *  @param   
 *  @return  error code -- 0 if ok, non-zero if error.
 */
nsresult
nsParser::OnStatus(nsIURL* aURL, const nsString &aMsg)
{
  nsresult result=0;
  if (nsnull != mObserver) {
    mObserver->OnStatus(aURL, aMsg);
  }
  return result;
}

/**
 *  
 *  
 *  @update  gess 5/12/98
 *  @param   
 *  @return  error code -- 0 if ok, non-zero if error.
 */
nsresult nsParser::OnStartBinding(nsIURL* aURL, const char *aSourceType){
  if (nsnull != mObserver) {
    mObserver->OnStartBinding(aURL, aSourceType);
  }
  mParserContext->mAutoDetectStatus=eUnknownDetect;
  mParserContext->mDTD=0;
  mParserContext->mSourceType=aSourceType;
  return kNoError;
}

/**
 *  
 *  
 *  @update  gess 5/12/98
 *  @param   pIStream contains the input chars
 *  @param   length is the number of bytes waiting input
 *  @return  error code (usually 0)
 */
nsresult nsParser::OnDataAvailable(nsIURL* aURL, nsIInputStream *pIStream, PRInt32 length){
/*  if (nsnull != mListener) {
      //Rick potts removed this.
      //Does it need to be here?
    mListener->OnDataAvailable(pIStream, length);
  }
*/

  if(eInvalidDetect==mParserContext->mAutoDetectStatus) {
    if(mParserContext->mScanner) {
      mParserContext->mScanner->GetBuffer().Truncate();
    }
  }

  int len=1; //init to a non-zero value

  if(!mParserContext->mTransferBuffer)
    mParserContext->mTransferBuffer = new char[CParserContext::eTransferBufferSize+1];

  while (len > 0) {
    nsresult rv = pIStream->Read(mParserContext->mTransferBuffer, 0, mParserContext->eTransferBufferSize, &len);
    if((rv == NS_OK) && (len>0)) {

      if(mParserFilter)
         mParserFilter->RawBuffer(mParserContext->mTransferBuffer, &len);

      mParserContext->mScanner->Append(mParserContext->mTransferBuffer,len);

      if(eUnknownDetect==mParserContext->mAutoDetectStatus) {
        if(eValidDetect==AutoDetectContentType(mParserContext->mScanner->GetBuffer(),mParserContext->mSourceType)) {
          WillBuildModel(mParserContext->mScanner->GetFilename());
        } //if
      }
    } //if
  }

  return ResumeParse();
}

/**
 *  
 *  
 *  @update  gess 5/12/98
 *  @param   
 *  @return  
 */
nsresult nsParser::OnStopBinding(nsIURL* aURL, PRInt32 status, const nsString& aMsg){
  nsresult result=DidBuildModel(status);
  if (nsnull != mObserver) {
    mObserver->OnStopBinding(aURL, status, aMsg);
  }
  return result;
}


/*******************************************************************
  Here comes the tokenization methods...
 *******************************************************************/


/**
 *  Part of the code sandwich, this gets called right before
 *  the tokenization process begins. The main reason for
 *  this call is to allow the delegate to do initialization.
 *  
 *  @update  gess 3/25/98
 *  @param   
 *  @return  TRUE if it's ok to proceed
 */
PRBool nsParser::WillTokenize(){
  PRBool result=PR_TRUE;
  return result;
}


/**
 *  This is the primary control routine. It iteratively
 *  consumes tokens until an error occurs or you run out
 *  of data.
 *  
 *  @update  gess 3/25/98
 *  @return  error code -- 0 if ok, non-zero if error.
 */
PRInt32 nsParser::Tokenize(){
  CToken* theToken=0;
  PRInt32 result=kNoError;

  ++mMajorIteration;

  WillTokenize();
  while(kNoError==result) {
    mParserContext->mScanner->Mark();
    result=mParserContext->mDTD->ConsumeToken(theToken);
    if(kNoError==result) {
      if(theToken) {

  #ifdef VERBOSE_DEBUG
          theToken->DebugDumpToken(cout);
  #endif
        mParserContext->mTokenDeque.Push(theToken);
      }

    }
    else {
      if(theToken)
        delete theToken;
      mParserContext->mScanner->RewindToMark();
    }
  } 
  if(kProcessComplete==result)
    result=NS_OK;
  DidTokenize();
  return result;
}

/**
 *  This is the tail-end of the code sandwich for the
 *  tokenization process. It gets called once tokenziation
 *  has completed.
 *  
 *  @update  gess 3/25/98
 *  @param   
 *  @return  TRUE if all went well
 */
PRBool nsParser::DidTokenize(){
  PRBool result=PR_TRUE;
  return result;
}

/**
 *  This debug routine is used to cause the tokenizer to
 *  iterate its token list, asking each token to dump its
 *  contents to the given output stream.
 *  
 *  @update  gess 3/25/98
 *  @param   
 *  @return  
 */
void nsParser::DebugDumpTokens(ostream& out) {
  nsDequeIterator b=mParserContext->mTokenDeque.Begin();
  nsDequeIterator e=mParserContext->mTokenDeque.End();

  CToken* theToken;
  while(b!=e) {
    theToken=(CToken*)(b++);
    theToken->DebugDumpToken(out);
  }
}


/**
 *  This debug routine is used to cause the tokenizer to
 *  iterate its token list, asking each token to dump its
 *  contents to the given output stream.
 *  
 *  @update  gess 3/25/98
 *  @param   
 *  @return  
 */
void nsParser::DebugDumpSource(ostream& out) {
  nsDequeIterator b=mParserContext->mTokenDeque.Begin();
  nsDequeIterator e=mParserContext->mTokenDeque.End();

  CToken* theToken;
  while(b!=e) {
    theToken=(CToken*)(b++);
    theToken->DebugDumpSource(out);
  }

}


