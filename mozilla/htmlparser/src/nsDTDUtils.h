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



#ifndef DTDUTILS_
#define DTDUTILS_

#include "nsHTMLTags.h"
#include "nsHTMLTokens.h"
#include "nsIParser.h"
#include "nsCRT.h"
#include "nsDeque.h"
#include "nsIDTD.h"
#include "nsITokenizer.h"
#include "nsString.h"
#include "nsIElementObserver.h"
#include "nsIParserNode.h"

class nsIParserNode;
class nsCParserNode;
class CNodeRecycler;


void DebugDumpContainmentRules(nsIDTD& theDTD,const char* aFilename,const char* aTitle);
void DebugDumpContainmentRules2(nsIDTD& theDTD,const char* aFilename,const char* aTitle);
PRUint32 AccumulateCRC(PRUint32 crc_accum, char *data_blk_ptr, int data_blk_size);



/***************************************************************
  First, define the tagstack class
 ***************************************************************/

class nsEntryStack;  //forware declare to make compilers happy.

struct nsTagEntry {
  eHTMLTags       mTag;  //for speedier access to tag id
  nsIParserNode*  mNode;
  nsEntryStack*   mParent;
  nsEntryStack*   mStyles;
};

class nsEntryStack {
public:
                  nsEntryStack();
                  ~nsEntryStack();

  void            EnsureCapacityFor(PRInt32 aNewMax, PRInt32 aShiftOffset=0);
  void            Push(const nsIParserNode* aNode,nsEntryStack* aStyleStack=0);
  void            PushFront(const nsIParserNode* aNode,nsEntryStack* aStyleStack=0);
  void            Append(nsEntryStack *theStack);
  nsIParserNode*  Pop(void);
  nsIParserNode*  Remove(PRInt32 anIndex,eHTMLTags aTag);
  nsIParserNode*  NodeAt(PRInt32 anIndex) const;
  eHTMLTags       First() const;
  eHTMLTags       TagAt(PRInt32 anIndex) const;
  nsTagEntry*     EntryAt(PRInt32 anIndex) const;
  eHTMLTags       operator[](PRInt32 anIndex) const;
  eHTMLTags       Last() const;
  void            Empty(void); 

  /**
   * Find the first instance of given tag on the stack.
   * @update	gess 12/14/99
   * @param   aTag
   * @return  index of tag, or kNotFound if not found
   */
  inline PRInt32 FirstOf(eHTMLTags aTag) const {
    PRInt32 index=-1;
    
    if(0<mCount) {
      while(++index<mCount) {
        if(aTag==mEntries[index].mTag) {
          return index;
        }
      } //while
    }
    return kNotFound;
  }


  /**
   * Find the last instance of given tag on the stack.
   * @update	gess 12/14/99
   * @param   aTag
   * @return  index of tag, or kNotFound if not found
   */
  inline PRInt32 LastOf(eHTMLTags aTag) const {
    PRInt32 index=mCount;
    while(--index>=0) {
        if(aTag==mEntries[index].mTag) {
          return index; 
        }
    }
    return kNotFound;
  }

  nsTagEntry* mEntries;
  PRInt32    mCount;
  PRInt32    mCapacity;
};


/**********************************************************
  The table state class is used to store info about each
  table that is opened on the stack. As tables open and
  close on the context, we update these objects to track 
  what has/hasn't been seen on a per table basis. 
 **********************************************************/
class CTableState {
public:
  CTableState(CTableState *aPreviousState=0) {
    mHasCaption=PR_FALSE;
    mHasCols=PR_FALSE;
    mHasTHead=PR_FALSE;
    mHasTFoot=PR_FALSE;
    mHasTBody=PR_FALSE;    
    mPrevious=aPreviousState;
  }

  PRBool  CanOpenCaption() {
    PRBool result=!(mHasCaption || mHasCols || mHasTHead || mHasTFoot || mHasTBody);
    return result;
  }

  PRBool  CanOpenCols() {
    PRBool result=!(mHasCols || mHasTHead || mHasTFoot || mHasTBody);
    return result;
  }

  PRBool  CanOpenTBody() {
    PRBool result=!(mHasTBody);
    return result;
  }

  PRBool  CanOpenTHead() {
    PRBool result=!(mHasTHead || mHasTFoot || mHasTBody);
    return result;
  }

  PRBool  CanOpenTFoot() {
    PRBool result=!(mHasTFoot || mHasTBody);
    return result;
  }

  PRBool  mHasCaption;
  PRBool  mHasCols;
  PRBool  mHasTHead;
  PRBool  mHasTFoot;
  PRBool  mHasTBody;
  CTableState *mPrevious;
};


//used for named entities and counters (XXX debug only)
class CNamedEntity {
public:
  CNamedEntity(const nsString& aName,const nsString& aValue) : mName(), mValue() {
    PRUnichar theFirst=aName.First();
    PRUnichar theLast=aName.Last();
    PRInt32   theLen=aName.Length();
    if((2<theLen) && (theFirst==theLast) && (kQuote==theFirst)) {
      aName.Mid(mName,1,theLen-2);
    } 
    else mName=aName;

    theFirst=aValue.First();
    theLast=aValue.Last();
    theLen=aValue.Length();
    if((2<theLen) && (theFirst==theLast) && (kQuote==theFirst)) {
      aValue.Mid(mValue,1,theLen-2);
    }
    else mValue=aValue;

  }

  nsAutoString mName;
  nsAutoString mValue;
  PRInt32      mOrdinal;
};


/************************************************************************
  The dtdcontext class defines an ordered list of tags (a context).
 ************************************************************************/

class nsDTDContext {
public:
                  nsDTDContext();
                  ~nsDTDContext();

  void            Push(const nsIParserNode* aNode,nsEntryStack* aStyleStack=0);
  nsIParserNode*  Pop(nsEntryStack*& aChildStack);
  nsIParserNode*  Pop();
  nsIParserNode*  PeekNode() { return mStack.NodeAt(mStack.mCount-1); }
  eHTMLTags       First(void) const;
  eHTMLTags       Last(void) const;
  nsTagEntry*     LastEntry(void) const;
  eHTMLTags       TagAt(PRInt32 anIndex) const;
  eHTMLTags       operator[](PRInt32 anIndex) const {return TagAt(anIndex);}
  PRBool          HasOpenContainer(eHTMLTags aTag) const;
  PRInt32         FirstOf(eHTMLTags aTag) const {return mStack.FirstOf(aTag);}
  PRInt32         LastOf(eHTMLTags aTag) const {return mStack.LastOf(aTag);}

  void            Empty(void); 
  PRInt32         GetCount(void) {return mStack.mCount;}
  PRInt32         GetResidualStyleCount(void) {return mResidualStyleCount;}
  nsEntryStack*   GetStylesAt(PRInt32 anIndex) const;
  void            PushStyle(const nsIParserNode* aNode);
  void            PushStyles(nsEntryStack *theStyles);
  nsIParserNode*  PopStyle(void);
  nsIParserNode*  PopStyle(eHTMLTags aTag);
  nsIParserNode*  RemoveStyle(eHTMLTags aTag);

  nsresult        GetNodeRecycler(CNodeRecycler*& aNodeRecycler);
  static  void    FreeNodeRecycler(void);

  CNamedEntity*   RegisterEntity(const nsString& aName,const nsString& aValue);
  CNamedEntity*   GetEntity(const nsString& aName)const;

  void            ResetCounters(void);
  PRInt32         IncrementCounter(eHTMLTags aTag,nsCParserNode& aNode,nsString& aResult);

  nsEntryStack    mStack; //this will hold a list of tagentries...
  PRInt32         mResidualStyleCount;
  PRInt32         mContextTopIndex;
  PRBool          mHadBody;
  PRBool          mHadFrameset;
  PRBool          mHadDocTypeDecl;

  static   CNodeRecycler* mNodeRecycler;

  CTableState     *mTableStates;
  PRInt32         mCounters[NS_HTML_TAG_MAX];
  nsString        mDefaultEntity;
  nsDeque         mEntities;

#ifdef  NS_DEBUG
  enum { eMaxTags = 100 };
  eHTMLTags       mXTags[eMaxTags];
#endif
};

/**************************************************************
  Now define the token deallocator class...
 **************************************************************/
class CTokenDeallocator: public nsDequeFunctor{
public:
  virtual void* operator()(void* anObject) {
    CToken* aToken = (CToken*)anObject;
    delete aToken;
    return 0;
  }
};


/************************************************************************
  CTokenRecycler class implementation.
  This class is used to recycle tokens. 
  By using this simple class, we cut WAY down on the number of tokens
  that get created during the run of the system.
 ************************************************************************/
class CTokenRecycler : public nsITokenRecycler {
public:
  
//      enum {eCacheMaxSize=100}; 

                  CTokenRecycler();
  virtual         ~CTokenRecycler();
  virtual void    RecycleToken(CToken* aToken);
  virtual CToken* CreateTokenOfType(eHTMLTokenTypes aType,eHTMLTags aTag, const nsString& aString);
  virtual CToken* CreateTokenOfType(eHTMLTokenTypes aType,eHTMLTags aTag);

protected:
    nsDeque*  mTokenCache[eToken_last-1];
    nsString  mEmpty;

#ifdef  NS_DEBUG
    int       mTotals[eToken_last-1];
#endif
};

/************************************************************************
  CNodeRecycler class implementation.
  This class is used to recycle nodes. 
  By using this simple class, we cut down on the number of nodes
  that get created during the run of the system.
 ************************************************************************/

class CNodeRecycler {
public:
  
                         CNodeRecycler();
  virtual                ~CNodeRecycler();
  virtual nsCParserNode* CreateNode(void);
  virtual void           RecycleNode(nsCParserNode* aNode,nsITokenRecycler* aTokenRecycler=0);

protected:
    nsDeque  mSharedNodes;

#ifdef NS_DEBUG
    PRInt32 gNodeCount;
#endif
};

/************************************************************************
  ITagHandler class offers an API for taking care of specific tokens.
 ************************************************************************/
class nsITagHandler {
public:
  
  virtual void          SetString(const nsString &aTheString)=0;
  virtual nsString*     GetString()=0;
  virtual PRBool        HandleToken(CToken* aToken,nsIDTD* aDTD)=0;
  virtual PRBool        HandleCapturedTokens(CToken* aToken,nsIDTD* aDTD)=0;
};

/************************************************************************
  Here are a few useful utility methods...
 ************************************************************************/

/**
 * This method quickly scans the given set of tags,
 * looking for the given tag.
 * @update	gess8/27/98
 * @param   aTag -- tag to be search for in set
 * @param   aTagSet -- set of tags to be searched
 * @return
 */
inline PRInt32 IndexOfTagInSet(PRInt32 aTag,const eHTMLTags* aTagSet,PRInt32 aCount)  {

  const eHTMLTags* theEnd=aTagSet+aCount;
  const eHTMLTags* theTag=aTagSet;

  while(theTag<theEnd) {
    if(aTag==*theTag) {
      return theTag-aTagSet;
    }
    theTag++;
  }

  return kNotFound;
}

/**
 * This method quickly scans the given set of tags,
 * looking for the given tag.
 * @update	gess8/27/98
 * @param   aTag -- tag to be search for in set
 * @param   aTagSet -- set of tags to be searched
 * @return
 */
inline PRBool FindTagInSet(PRInt32 aTag,const eHTMLTags *aTagSet,PRInt32 aCount)  {
  return PRBool(-1<IndexOfTagInSet(aTag,aTagSet,aCount));
}

/**
 * Called from various DTD's to determine the type of data in the buffer...
 * @update	gess11/20/98
 * @param 
 * @return
 */
inline PRBool BufferContainsHTML(nsString& aBuffer,PRBool& aHasXMLFragment){
  PRBool result=PR_FALSE;
  nsString temp;
  aBuffer.Left(temp,200);
  temp.ToLowerCase();

  aHasXMLFragment=PRBool(-1<temp.Find("<?xml"));
  if((-1<temp.Find("<html ") || (-1<temp.Find("!doctype html public")))) {
    result=PR_TRUE;
  }
  return result;
}


/******************************************************************************
  This little structure is used to compute CRC32 values for our debug validator
 ******************************************************************************/

struct CRCStruct {
  CRCStruct(eHTMLTags aTag,PRInt32 anOp) {mTag=aTag; mOperation=anOp;}
  eHTMLTags mTag; 
  PRInt32   mOperation; //usually open or close
};

/**************************************************************
  This defines the topic object used by the observer service.
  The observerService uses a list of these, 1 per topic when
  registering tags.
 **************************************************************/

class nsObserverTopic {
public:
            nsObserverTopic(const nsString& aTopic);
            ~nsObserverTopic();
  PRBool    Matches(const nsString& aTopic);
  void      RegisterObserverForTag(nsIElementObserver *anObserver,eHTMLTags aTag);
  nsDeque*  GetObserversForTag(eHTMLTags aTag);
  nsresult  Notify(eHTMLTags aTag,nsIParserNode& aNode,void* aUniqueID,nsIParser* aParser);

  nsString  mTopic;
  nsDeque   mKeys;
  nsDeque   mValues;
  nsString  mCharsetKey;
  nsString  mSourceKey;
  nsString  mDTDKey;
  nsDeque*  mObservers[NS_HTML_TAG_MAX + 1];
};

/******************************************************************************
  This class is used to store ref's to token observers during the parse phase.
  Note that for simplicity, this is a singleton that is constructed in the
  CNavDTD and shared for the duration of the application session. Later on it
  might be nice to use a more dynamic approach that would permit observers to
  come and go on a document basis.
 ******************************************************************************/
class CObserverService {
public:
  CObserverService();
  ~CObserverService();

  nsDeque*  GetObserversForTagInTopic(eHTMLTags aTag,const nsString& aTopic);
  nsresult  Notify( eHTMLTags aTag,
                    nsIParserNode& aNode,
                    void* aUniqueID, 
                    const nsString& aTopic,
                    nsIParser* aParser);
  nsObserverTopic* GetTopic(const nsString& aTopic);
  nsObserverTopic* CreateTopic(const nsString& aTopic);

protected:
  void      RegisterObservers(const nsString& aTopic);
  void      UnregisterObservers(const nsString& aTopic);
  nsDeque   mTopics;  //each topic holds a list of observers per tag.
};

/************************************************************** 
  Define the a functor used to notify observers... 
 **************************************************************/ 
class nsObserverNotifier: public nsDequeFunctor{ 
public: 
  nsObserverNotifier(const PRUnichar* aTagName,nsISupports* aUniqueKey,
                     const nsDeque* aKeys=0,const nsDeque* aValues=0){ 
    mKeys=aKeys; 
    mValues=aValues; 
    mUniqueKey=aUniqueKey; 
    mTagName=aTagName; 
  } 

  virtual void* operator()(void* anObject) { 
    nsIElementObserver* theObserver= (nsIElementObserver*)anObject; 
    if(theObserver) { 
      mResult = theObserver->Notify(mUniqueKey,mTagName,mKeys,mValues); 
    } 
    if(NS_OK==mResult) 
      return 0; 
    return anObject; 
  } 

  const nsDeque*    mKeys; 
  const nsDeque*    mValues;   
  const PRUnichar*  mTagName; 
  nsISupports*      mUniqueKey; 
  nsresult          mResult;
};


//*********************************************************************************************
//*********************************************************************************************


struct TagList {
  PRUint32    mCount;
  eHTMLTags   mTags[10];
};

/**
 * Find the last member of given taglist on the given context
 * @update	gess 12/14/99
 * @param   aContext
 * @param   aTagList
 * @return  index of tag, or kNotFound if not found
 */
inline PRInt32 LastOf(nsDTDContext& aContext,TagList& aTagList){
  int max = aContext.GetCount();
  int index;
  for(index=max-1;index>=0;index--){
    PRBool result=FindTagInSet(aContext[index],aTagList.mTags,aTagList.mCount);
    if(result) {
      return index;
    }
  }
  return kNotFound;
}
 
/**
 * Find the first member of given taglist on the given context
 * @update	gess 12/14/99
 * @param   aContext
 * @param   aStartOffset
 * @param   aTagList
 * @return  index of tag, or kNotFound if not found
 */
inline PRInt32 FirstOf(nsDTDContext& aContext,PRInt32 aStartOffset,TagList& aTagList){
  int max = aContext.GetCount();
  int index;
  for(index=aStartOffset;index<max;index++){
    PRBool result=FindTagInSet(aContext[index],aTagList.mTags,aTagList.mCount);
    if(result) {
      return index;
    }
  }
  return kNotFound;
}

#endif


