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
 */



#ifndef _NSELEMENTABLE
#define _NSELEMENTABLE

#include "nsHTMLTokens.h"
#include "nsDTDUtils.h"

class CTagList {
public:
  CTagList( int aCount,
            eHTMLTags*  aTagList=0,
            eHTMLTags aTag1=eHTMLTag_unknown,
            eHTMLTags aTag2=eHTMLTag_unknown,
            eHTMLTags aTag3=eHTMLTag_unknown,
            eHTMLTags aTag4=eHTMLTag_unknown,
            eHTMLTags aTag5=eHTMLTag_unknown) 
  {
    mCount=aCount;
    mTags[0]=aTag1; mTags[1]=aTag2; mTags[2]=aTag3; mTags[3]=aTag4; mTags[4]=aTag5;
    mTagList=aTagList;
  }

  PRInt32   GetTopmostIndexOf(nsEntryStack& aTagStack);
  PRInt32   GetBottommostIndexOf(nsEntryStack& aTagStack,PRInt32 aStartOffset);
  PRBool    Contains(eHTMLTags aTag);
  eHTMLTags GetTagAt(PRUint32 anIndex) const;

  eHTMLTags   mTags[5];
  eHTMLTags*  mTagList;
  PRUint32    mCount;
};

//*********************************************************************************************
// The following ints define the standard groups of HTML elements...
//*********************************************************************************************


extern void InitializeElementTable(void);

/**
 * 
 * @update	gess 01/04/99
 * @param 
 * @return
 */
struct nsHTMLElement {

  static  void    DebugDumpMembership(const char* aFilename);
  static  void    DebugDumpContainment(const char* aFilename,const char* aTitle);
  static  void    DebugDumpContainType(const char* aFilename);

  static  PRBool  IsBlockEntity(eHTMLTags aTag);
  static  PRBool  IsInlineEntity(eHTMLTags aTag);
  static  PRBool  IsFlowEntity(eHTMLTags aTag);
  static  PRBool  IsBlockCloser(eHTMLTags aTag);
  static  int     GetSynonymousGroups(int aGroup);

  CTagList*       GetSynonymousTags(void) const {return mSynonymousTags;}
  CTagList*       GetRootTags(void) const {return mRootNodes;}
  CTagList*       GetEndRootTags(void) const {return mEndRootNodes;}
  CTagList*       GetAutoCloseStartTags(void) const {return mAutocloseStart;}
  CTagList*       GetAutoCloseEndTags(void) const {return mAutocloseEnd;}
  CTagList*       GetNonAutoCloseEndTags(void) const {return mDontAutocloseEnd;}
  eHTMLTags       GetCloseTargetForEndTag(nsEntryStack& aTagStack,PRInt32 anIndex) const;

  CTagList*       GetSpecialChildren(void) const {return mSpecialKids;}
  CTagList*       GetSpecialParents(void) const {return mSpecialParents;}

  PRBool          IsMemberOf(PRInt32 aType) const;
  PRBool          CanContainType(PRInt32 aType) const;
  
  eHTMLTags       GetTag(void) const {return mTagID;}
  PRBool          CanContain(eHTMLTags aChild) const;
  PRBool          CanExclude(eHTMLTags aChild) const;
  PRBool          CanOmitStartTag(eHTMLTags aChild) const;
  PRBool          CanOmitEndTag(void) const;
  PRBool          CanContainSelf(void) const;
  PRBool          CanAutoCloseTag(eHTMLTags aTag) const;
  PRBool          HasSpecialProperty(PRInt32 aProperty) const;
  PRBool          SectionContains(eHTMLTags aTag,PRBool allowDepthSearch);
 
  static  PRBool  CanContain(eHTMLTags aParent,eHTMLTags aChild);
  static  PRBool  IsContainer(eHTMLTags aTag) ;
  static  PRBool  IsStyleTag(eHTMLTags aTag) ;
  static  PRBool  IsHeadingTag(eHTMLTags aTag) ;
  static  PRBool  IsTextTag(eHTMLTags aTag);
  static  PRBool  IsWhitespaceTag(eHTMLTags aTag);

  static  PRBool  IsBlockParent(eHTMLTags aTag);
  static  PRBool  IsInlineParent(eHTMLTags aTag); 
  static  PRBool  IsFlowParent(eHTMLTags aTag);
  static  PRBool  IsSectionTag(eHTMLTags aTag);
  static  PRBool  IsChildOfHead(eHTMLTags aTag) ;

  eHTMLTags       mTagID;
  eHTMLTags       mRequiredAncestor;
  eHTMLTags       mExcludingAncestor; //If set, the presence of the excl-ancestor prevents this from opening.
  CTagList*       mRootNodes;         //These are the tags above which you many not autoclose a START tag
  CTagList*       mEndRootNodes;      //These are the tags above which you many not autoclose an END tag
  CTagList*       mAutocloseStart;    //these are the start tags that you can automatically close with this START tag
  CTagList*       mAutocloseEnd;      //these are the start tags that you can automatically close with this END tag
  CTagList*       mSynonymousTags;    //These are morally equivalent; an end tag for one can close a start tag for another (like <Hn>)
  CTagList*       mDontAutocloseEnd;  //these are the end tags that you cannot automatically close with this END tag
  int             mParentBits;        //defines groups that can contain this element
  int             mInclusionBits;     //defines parental and containment rules
  int             mExclusionBits;     //defines things you CANNOT contain
  int             mSpecialProperties; //used for various special purposes...
  int             mPropagateRange;    //tells us how far a parent is willing to prop. badly formed children
  CTagList*       mSpecialParents;    //These are the special tags that contain this tag (directly)
  CTagList*       mSpecialKids;       //These are the extra things you can contain
  eHTMLTags       mSkipTarget;        //If set, then we skip all content until this tag is seen
}; 

extern nsHTMLElement* gHTMLElements;

//special property bits...
static const int kDiscardTag       = 0x0001; //tells us to toss this tag
static const int kOmitEndTag       = 0x0002; //safely ignore end tag
static const int kLegalOpen        = 0x0004; //Lets BODY, TITLE, SCRIPT to reopen
static const int kOmitWS           = 0x0008; //If set, the tag can omit all ws and newlines
static const int kNoPropagate      = 0x0010; //If set, this tag won't propagate as a child
static const int kBadContentWatch  = 0x0020; 
static const int kNoStyleLeaksIn   = 0x0040; 
static const int kNoStyleLeaksOut  = 0x0080; 
static const int kMustCloseSelf    = 0x0100; 
static const int kSaveMisplaced    = 0x0200; //If set, then children this tag can't contain are pushed onto the misplaced stack

//*********************************************************************************************
// The following ints define the standard groups of HTML elements...
//*********************************************************************************************

static const int kNone= 0x0;

static const int kHTMLContent   = 0x0001; //  HEAD, (FRAMESET | BODY)
static const int kHeadContent   = 0x0002; //  TITLE, ISINDEX, BASE
static const int kHeadMisc      = 0x0004; //  SCRIPT, STYLE, META,  LINK, OBJECT

static const int kSpecial       = 0x0008; //  A,    IMG,  APPLET, OBJECT, FONT, BASEFONT, BR, SCRIPT, 
                                          //  MAP,  Q,    SUB,    SUP,    SPAN, BDO,      IFRAME

static const int kFormControl   = 0x0010; //  INPUT SELECT  TEXTAREA  LABEL BUTTON
static const int kPreformatted  = 0x0020; //  PRE
static const int kPreExclusion  = 0x0040; //  IMG,  OBJECT, APPLET, BIG,  SMALL,  SUB,  SUP,  FONT, BASEFONT
static const int kFontStyle     = 0x0080; //  TT, I, B, U, S, STRIKE, BIG, SMALL, BLINK
static const int kPhrase        = 0x0100; //  EM, STRONG, DFN, CODE, SAMP, KBD, VAR, CITE, ABBR, ACRONYM
static const int kHeading       = 0x0200; //  H1..H6
static const int kBlockMisc     = 0x0400; //  OBJECT, SCRIPT
static const int kBlock         = 0x0800; //  ADDRESS, BLOCKQUOTE, CENTER, DIV, DL, FIELDSET, FORM, 
                                          //  ISINDEX, HR, NOSCRIPT, NOFRAMES, P, TABLE
static const int kList          = 0x1000; //  UL, OL, DIR, MENU
static const int kPCDATA        = 0x2000; //  just plain text...
static const int kSelf          = 0x4000; //  whatever THIS tag is...
static const int kExtensions    = 0x8000; //  BGSOUND, WBR, NOBR
static const int kTable         = 0x10000;//  TR,TD,THEAD,TBODY,TFOOT,CAPTION,TH

static const int kInlineEntity  = (kPCDATA|kFontStyle|kPhrase|kSpecial|kFormControl|kExtensions);  //  #PCDATA, %fontstyle, %phrase, %special, %formctrl
static const int kBlockEntity   = (kHeading|kList|kPreformatted|kBlock); //  %heading, %list, %preformatted, %blockmisc
static const int kFlowEntity    = (kBlockEntity|kInlineEntity); //  %block, %inline
static const int kAllTags       = 0xffffff;


#endif
