
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

#ifndef NS_NAVHTMLDTD__
#define NS_NAVHTMLDTD__

#include "nsIDTD.h"
#include "nsISupports.h"
#include "nsHTMLTokens.h"
#include "nshtmlpars.h"
#include "nsVoidArray.h"
#include "nsDeque.h"

#define NS_RTF_DTD_IID      \
  {0xa39c6bfc, 0x15f0,  0x11d2,  \
  {0x80, 0x41, 0x00,    0x10, 0x4b, 0x98, 0x3f, 0xd4}}

class nsIParserNode;
class nsParser;


enum eRTFTokenTypes {
  eRTFToken_unknown=0,
  eRTFToken_group,        // '{'
  eRTFToken_controlword,  // '\'
  eRTFToken_content,      // contains all document content
    //everything else is just text or attributes of controls...
  eRTFToken_last //make sure this stays the last token...
};


enum eRTFTags {
  eRTFCtrl_unknown=0,
  eRTFCtrl_quote,
  eRTFCtrl_star,   
  eRTFCtrl_linefeed,
  eRTFCtrl_return,
  eRTFCtrl_begincontrol,
  eRTFCtrl_bold, 
  eRTFCtrl_bin,
  eRTFCtrl_blue,
  eRTFCtrl_cols,
  eRTFCtrl_comment,
  eRTFCtrl_italic,
  eRTFCtrl_font,
  eRTFCtrl_fonttable,
  eRTFCtrl_green,
  eRTFCtrl_bottommargin,
  eRTFCtrl_leftmargin,
  eRTFCtrl_rightmargin,
  eRTFCtrl_topmargin,
  eRTFCtrl_par,
  eRTFCtrl_pard,
  eRTFCtrl_plain,
  eRTFCtrl_justified,
  eRTFCtrl_fulljustified,
  eRTFCtrl_leftjustified,
  eRTFCtrl_rightjustified,
  eRTFCtrl_rdoublequote,
  eRTFCtrl_red,
  eRTFCtrl_rtf,
  eRTFCtrl_tab,
  eRTFCtrl_title,
  eRTFCtrl_underline,
  eRTFCtrl_startgroup,
  eRTFCtrl_endgroup,
  eRTFCtrl_last //make sure this stays the last token...
};



/**
 * 
 * @update	gess7/8/98
 * @param 
 * @return
 */
class CRTFControlWord : public CToken {
public:
                  CRTFControlWord(char* aKey);
  virtual PRInt32 GetTokenType();
  virtual PRInt32 Consume(CScanner& aScanner);
protected:
  nsString  mArgument;
};


/**
 * 
 * @update	gess7/8/98
 * @param 
 * @return
 */
class CRTFGroup: public CToken {
public:
                  CRTFGroup(char* aKey,PRBool aStartGroup);
  virtual PRInt32 GetTokenType();
  virtual void    SetGroupStart(PRBool aFlag);
  virtual PRBool  IsGroupStart();
  virtual PRInt32 Consume(CScanner& aScanner);
protected:
          PRBool  mStart;
};


/**
 * 
 * @update	gess7/8/98
 * @param 
 * @return
 */
class CRTFContent: public CToken {
public:
                  CRTFContent(PRUnichar* aValue);
  virtual PRInt32 GetTokenType();
  virtual PRInt32 Consume(CScanner& aScanner);
};


class CRtfDTD : public nsIDTD {
            
  public:

    NS_DECL_ISUPPORTS


    /**
     *  
     *  
     *  @update  gess 4/9/98
     *  @param   
     *  @return  
     */
    CRtfDTD();

    /**
     *  
     *  
     *  @update  gess 4/9/98
     *  @param   
     *  @return  
     */
    virtual ~CRtfDTD();

    /**
     * This method is called to determine if the given DTD can parse
     * a document in a given source-type. 
     * NOTE: Parsing always assumes that the end result will involve
     *       storing the result in the main content model.
     * @update	gess6/24/98
     * @param   
     * @return  TRUE if this DTD can satisfy the request; FALSE otherwise.
     */
    virtual PRBool CanParse(nsString& aContentType, PRInt32 aVersion);

    /**
     * 
     * @update	gess7/7/98
     * @param 
     * @return
     */
    virtual eAutoDetectResult AutoDetectContentType(nsString& aBuffer,nsString& aType);

    /**
     * 
     * @update	gess5/18/98
     * @param 
     * @return
     */
    virtual PRInt32 WillBuildModel(const char* aFilename=0);

    /**
     * 
     * @update	gess5/18/98
     * @param 
     * @return
     */
    virtual PRInt32 DidBuildModel(PRInt32 anErrorCode);

    /**
     *  
     *  @update  gess 3/25/98
     *  @param   aToken -- token object to be put into content model
     *  @return  0 if all is well; non-zero is an error
     */
    virtual PRInt32 HandleGroup(CToken* aToken);

    /**
     *  
     *  @update  gess 3/25/98
     *  @param   aToken -- token object to be put into content model
     *  @return  0 if all is well; non-zero is an error
     */
    virtual PRInt32 HandleControlWord(CToken* aToken);

    /**
     *  
     *  @update  gess 3/25/98
     *  @param   aToken -- token object to be put into content model
     *  @return  0 if all is well; non-zero is an error
     */
    virtual PRInt32 HandleContent(CToken* aToken);

    /**
     *  
     *  @update  gess 3/25/98
     *  @param   aToken -- token object to be put into content model
     *  @return  0 if all is well; non-zero is an error
     */
    virtual PRInt32 HandleToken(CToken* aToken);

    /**
     * 
     *  
     *  @update  gess 3/25/98
     *  @param   
     *  @return 
     */
    virtual void SetParser(nsIParser* aParser);

    /**
     *  Cause the tokenizer to consume the next token, and 
     *  return an error result.
     *  
     *  @update  gess 3/25/98
     *  @param   anError -- ref to error code
     *  @return  new token or null
     */
    virtual PRInt32 ConsumeControlWord(CToken*& aToken);

    /**
     *  Cause the tokenizer to consume the next token, and 
     *  return an error result.
     *  
     *  @update  gess 3/25/98
     *  @param   anError -- ref to error code
     *  @return  new token or null
     */
    virtual PRInt32 ConsumeGroupTag(CToken*& aToken,PRBool aStartTag);

    /**
     *  Cause the tokenizer to consume the next token, and 
     *  return an error result.
     *  
     *  @update  gess 3/25/98
     *  @param   anError -- ref to error code
     *  @return  new token or null
     */
    virtual PRInt32 ConsumeContent(PRUnichar aChar,CToken*& aToken);

    /**
     *  Cause the tokenizer to consume the next token, and 
     *  return an error result.
     *  
     *  @update  gess 3/25/98
     *  @param   anError -- ref to error code
     *  @return  new token or null
     */
    virtual PRInt32 ConsumeToken(CToken*& aToken);


    /**
     * 
     * @update	gess5/18/98
     * @param 
     * @return
     */
    virtual void WillResumeParse(void);

    /**
     * 
     * @update	gess5/18/98
     * @param 
     * @return
     */
    virtual void WillInterruptParse(void);

   /**
     * Select given content sink into parser for parser output
     * @update	gess5/11/98
     * @param   aSink is the new sink to be used by parser
     * @return  old sink, or NULL
     */
    virtual nsIContentSink* SetContentSink(nsIContentSink* aSink);

    /**
     * 
     * @update	jevering6/23/98
     * @param 
     * @return
     */
	  virtual void SetDTDDebug(nsIDTDDebug * aDTDDebug);

    /**
     *  This method is called to determine whether or not a tag
     *  of one type can contain a tag of another type.
     *  
     *  @update  gess 3/25/98
     *  @param   aParent -- int tag of parent container
     *  @param   aChild -- int tag of child container
     *  @return  PR_TRUE if parent can contain child
     */
    virtual PRBool CanContain(PRInt32 aParent,PRInt32 aChild);
	
protected:
    
    nsParser*           mParser;
    char*               mFilename;
};

extern NS_HTMLPARS nsresult NS_NewRTF_DTD(nsIDTD** aInstancePtrResult);

#endif 



