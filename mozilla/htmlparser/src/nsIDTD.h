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
#ifndef nsIDTD_h___
#define nsIDTD_h___

/**
 * MODULE NOTES:
 * @update  gess 7/20/98
 * 
 * This interface defines standard interface for DTD's. Note that this isn't HTML specific.
 * DTD's have several primary functions within the parser system:
 *		1) To coordinate the consumption of an input stream via the parser
 *		2) To serve as proxy to represent the containment rules of the underlying document
 *		3) To offer autodetection services to the parser (mainly for doc conversion)
 *         
 */

#include "nshtmlpars.h"
#include "nsISupports.h"
#include "prtypes.h"

#define NS_IDTD_IID \
 { 0xa6cf9053, 0x15b3, 0x11d2,{0x93, 0x2e, 0x00, 0x80, 0x5f, 0x8a, 0xdd, 0x32}}

class nsIParser;
class CToken;
class nsIContentSink;
class nsIDTDDebug;
class nsIURL;
class nsString;


enum eAutoDetectResult {eUnknownDetect, eValidDetect, eInvalidDetect};


class nsITokenRecycler {
public:
    virtual void RecycleToken(CToken* aToken)=0;
};


class nsIDTD : public nsISupports {
  public:

  
    /**
     * Default constructor
     * @update	gess6/24/98
     */
    virtual ~nsIDTD() {};


    /**
     * Call this method if you want the DTD to construct a clone of itself.
     * @update	gess7/23/98
     * @param 
     * @return
     */
    virtual nsresult CreateNewInstance(nsIDTD** aInstancePtrResult)=0;

    /**
     *  This method informs the DTD about the parser being used to drive the parse process
     *  
     *  @update  gess 3/25/98
     *  @param   aParse -- ptr to parser object
     *  @return  nada
     */
    virtual void SetParser(nsIParser* aParser)=0;

   /**
     * Select given content sink into DTD for output
     * @update	gess5/11/98
     * @param   aSink is the new sink to be used by parser
     * @return  old sink, or NULL
     */
    virtual nsIContentSink* SetContentSink(nsIContentSink* aSink)=0;

    /**
     * This method is called to determine if the given DTD can parse
     * a document in a given source-type. 
     * NOTE: Parsing always assumes that the end result will involve
     *       storing the result in the main content model.
     * @update	gess6/24/98
     * @param   aContentType -- string representing type of doc to be converted (ie text/html)
     * @return  TRUE if this DTD can satisfy the request; FALSE otherwise.
     */
    virtual PRBool CanParse(nsString& aContentType, nsString& aCommand, PRInt32 aVersion)=0;

    /**
     * This method, typically called by the parser, is used to try to autodetect the 
	 * type of data contained in the given buffer. The implementor should look at the
	 * buffers contents to try to determine its encoding type. 
     * @update	gess7/7/98
     * @param	aBuffer-contains data to be scanned to autodetect type
	 * @param	aType-will hold the result if type is autodetected
     * @return	eValid (if detected), eInvalid (if not) or eUnknown (if nothing can be done)
     */
    virtual eAutoDetectResult AutoDetectContentType(nsString& aBuffer,nsString& aType)=0;

    /**
     * Called by the parser just before the parsing process begins
     * @update	gess5/18/98
     * @param	aFilename--string that contains name of file being parsed (if applicable)
     * @return  
     */
    NS_IMETHOD WillBuildModel(nsString& aFilename,PRBool aNotifySink)=0;

    /**
     * Called by the parser after the parsing process has concluded
     * @update	gess5/18/98
     * @param	anErrorCode - contains error code resulting from parse process
     * @return
     */
    NS_IMETHOD DidBuildModel(PRInt32 anErrorCode,PRBool aNotifySink)=0;
    
    /**
     *	Called during model building phase of parse process. Each token created during
	 *  the parse phase is stored in a deque (in the parser) and are passed to this method
	 *  so that the DTD can process the token. Ultimately, the DTD will transform given
	 *  token into calls onto a contentsink.
     *  @update  gess 3/25/98
     *  @param   aToken -- token object to be put into content model
	 *  @return	 error code (usually 0)
     */
    NS_IMETHOD HandleToken(CToken* aToken)=0;

    /**
     *  Cause the tokenizer to consume and create the next token, and 
     *  return an error result.
     *
     *  @update  gess 3/25/98
	 *  @param   aToken -- will contain newly created and consumed token
	 *  @return	 error code (usually 0)
     */
    NS_IMETHOD ConsumeToken(CToken*& aToken)=0;

    /**
     * If the parse process gets interrupted midway, this method is called by the
	 * parser prior to resuming the process.
     * @update	gess5/18/98
     * @return	ignored
     */
    NS_IMETHOD WillResumeParse(void)=0;

    /**
     * If the parse process gets interrupted, this method is called by the parser
	 * to notify the DTD that interruption will occur.
     * @update	gess5/18/98
     * @return	ignored
     */
    NS_IMETHOD WillInterruptParse(void)=0;

    /**
     *  This method is called to determine whether or not a tag
     *  of one type can contain a tag of another type.
     *  
     *  @update  gess 3/25/98
     *  @param   aParent -- int tag of parent container
     *  @param   aChild -- int tag of child container
     *  @return  PR_TRUE if parent can contain child
     */
    virtual PRBool CanContain(PRInt32 aParent,PRInt32 aChild) const =0;

    /**
     *  This method gets called to determine whether a given 
     *  tag is itself a container
     *  
     *  @update  gess 3/25/98
     *  @param   aTag -- tag to test for containership
     *  @return  PR_TRUE if given tag can contain other tags
     */
    virtual PRBool IsContainer(PRInt32 aTag) const=0;

    /**
     * Called by the parser to initiate dtd verification of the
     * internal context stack.
     * @update	gess 7/23/98
     * @param 
     * @return
     */
    virtual PRBool Verify(nsString& aURLRef)=0;

    /**
     * Retrieve a ptr to the global token recycler...
     * @update	gess8/4/98
     * @return  ptr to recycler (or null)
     */
    virtual nsITokenRecycler* GetTokenRecycler(void)=0;


};



#endif /* nsIDTD_h___ */

