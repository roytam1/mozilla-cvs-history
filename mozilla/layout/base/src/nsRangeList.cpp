/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
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

/*
 * nsIRangeList: the implementation of selection.
 */

#include "nsIFactory.h"
#include "nsIEnumerator.h"
#include "nsIDOMRange.h"
#include "nsIFrameSelection.h"
#include "nsIDOMSelection.h"
#include "nsIDOMSelectionListener.h"
#include "nsIFocusTracker.h"
#include "nsIComponentManager.h"
#include "nsLayoutCID.h"
#include "nsIContent.h"
#include "nsIDOMNode.h"
#include "nsCOMPtr.h"
#include "nsRange.h"
#include "nsISupportsArray.h"
#include "nsIDOMUIEvent.h"

#include "nsIDOMSelectionListener.h"
#include "nsIContentIterator.h"

//included for desired x position;
#include "nsIPresContext.h"
#include "nsIPresShell.h"
#include "nsICaret.h"

#include "nsIScriptObjectOwner.h"
#include "nsIScriptGlobalObject.h"


#define STATUS_CHECK_RETURN_MACRO() {if (!mTracker) return NS_ERROR_FAILURE;}

static NS_DEFINE_IID(kRangeCID, NS_RANGE_CID);
static NS_DEFINE_IID(kCContentIteratorCID, NS_CONTENTITERATOR_CID);
static NS_DEFINE_IID(kCSubtreeIteratorCID, NS_SUBTREEITERATOR_CID);

//PROTOTYPES
static void printRange(nsIDOMRange *aDomRange);
static nsCOMPtr<nsIAtom> GetTag(nsIDOMNode *aNode);
static nsresult ParentOffset(nsIDOMNode *aNode, nsIDOMNode **aParent, PRInt32 *aChildOffset);



#if 0
#define DEBUG_OUT_RANGE(x)  printRange(x)
#else
#define DEBUG_OUT_RANGE(x)  
#endif //MOZ_DEBUG

//#define DEBUG_SELECTION // uncomment for printf describing every collapse and extend.
//#define DEBUG_NAVIGATION

class nsRangeListIterator;

class nsRangeList : public nsIFrameSelection,
                    public nsIDOMSelection,
                    public nsIScriptObjectOwner
{
public:
  /*interfaces for addref and release and queryinterface*/
  
  NS_DECL_ISUPPORTS

/*BEGIN nsIFrameSelection interfaces*/
  NS_IMETHOD Init(nsIFocusTracker *aTracker);
  NS_IMETHOD ShutDown();
  NS_IMETHOD HandleTextEvent(nsGUIEvent *aGUIEvent);
  NS_IMETHOD HandleKeyEvent(nsGUIEvent *aGuiEvent);
  NS_IMETHOD TakeFocus(nsIContent *aNewFocus, PRUint32 aContentOffset, PRUint32 aContentEndOffset, PRBool aContinueSelection);
  NS_IMETHOD EnableFrameNotification(PRBool aEnable){mNotifyFrames = aEnable; return NS_OK;}
  NS_IMETHOD LookUpSelection(nsIContent *aContent, PRInt32 aContentOffset, PRInt32 aContentLength,
                             PRInt32 *aStart, PRInt32 *aEnd, PRBool *aDrawSelected , PRUint32 aFlag/*not used*/);
  NS_IMETHOD SetMouseDownState(PRBool aState);
  NS_IMETHOD GetMouseDownState(PRBool *aState);

/*END nsIFrameSelection interfacse*/

/*BEGIN nsIDOMSelection interface implementations*/
  NS_IMETHOD    GetAnchorNode(nsIDOMNode** aAnchorNode);
  NS_IMETHOD    GetAnchorOffset(PRInt32* aAnchorOffset);
  NS_IMETHOD    GetFocusNode(nsIDOMNode** aFocusNode);
  NS_IMETHOD    GetFocusOffset(PRInt32* aFocusOffset);
  NS_IMETHOD    GetIsCollapsed(PRBool* aIsCollapsed);
  NS_IMETHOD    GetRangeCount(PRInt32* aRangeCount);
  NS_IMETHOD    GetRangeAt(PRInt32 aIndex, nsIDOMRange** aReturn);
  NS_IMETHOD    ClearSelection();
  NS_IMETHOD    Collapse(nsIDOMNode* aParentNode, PRInt32 aOffset);
  NS_IMETHOD    Extend(nsIDOMNode* aParentNode, PRInt32 aOffset);
  NS_IMETHOD    DeleteFromDocument();
  NS_IMETHOD    AddRange(nsIDOMRange* aRange);

  NS_IMETHOD    StartBatchChanges();
  NS_IMETHOD    EndBatchChanges();

  NS_IMETHOD    AddSelectionListener(nsIDOMSelectionListener* aNewListener);
  NS_IMETHOD    RemoveSelectionListener(nsIDOMSelectionListener* aListenerToRemove);
/*END nsIDOMSelection interface implementations*/

/*BEGIN nsIScriptObjectOwner interface implementations*/
  NS_IMETHOD 		GetScriptObject(nsIScriptContext *aContext, void** aScriptObject);
  NS_IMETHOD 		SetScriptObject(void *aScriptObject);
/*END nsIScriptObjectOwner interface implementations*/

  nsRangeList();
  virtual ~nsRangeList();

private:
  nsresult AddItem(nsISupports *aRange);
  nsresult RemoveItem(nsISupports *aRange);

  nsresult Clear();
  NS_IMETHOD ScrollIntoView();

  friend class nsRangeListIterator;

#ifdef DEBUG
  void printSelection();       // for debugging
#endif /* DEBUG */

  void ResizeBuffer(PRUint32 aNewBufSize);

	// inline methods for convenience. Note, these don't addref
  nsIDOMNode*  FetchAnchorNode();  //where did the selection begin
  PRInt32      FetchAnchorOffset();

  nsIDOMNode*  FetchOriginalAnchorNode();  //where did the ORIGINAL selection begin
  PRInt32      FetchOriginalAnchorOffset();

  nsIDOMNode*  FetchFocusNode();   //where is the carret
  PRInt32      FetchFocusOffset();

  nsIDOMNode*  FetchStartParent(nsIDOMRange *aRange);   //skip all the com stuff and give me the start/end
  PRInt32      FetchStartOffset(nsIDOMRange *aRange);
  nsIDOMNode*  FetchEndParent(nsIDOMRange *aRange);     //skip all the com stuff and give me the start/end
  PRInt32      FetchEndOffset(nsIDOMRange *aRange);

  nscoord      FetchDesiredX(); //the x position requested by the Key Handling for up down
  void         InvalidateDesiredX(); //do not listen to mDesiredX you must get another.
  void         SetDesiredX(nscoord aX); //set the mDesiredX

  void         setAnchorFocusRange(PRInt32); //pass in index into rangelist
  
  PRUint32     GetBatching(){return mBatching;}
  PRBool       GetNotifyFrames(){return mNotifyFrames;}
  void         SetDirty(PRBool aDirty=PR_TRUE){if (mBatching) mChangesDuringBatching = aDirty;}

  nsresult     NotifySelectionListeners();			// add parameters to say collapsed etc?
  nsDirection  GetDirection(){return mDirection;}
  void         SetDirection(nsDirection aDir){mDirection = aDir;}

  NS_IMETHOD selectFrames(nsIDOMRange *aRange, PRBool aSelect);
  
  NS_IMETHOD FixupSelectionPoints(nsIDOMRange *aRange, nsDirection *aDir, PRBool *aFixupState);
  NS_IMETHOD SetOriginalAnchorPoint(nsIDOMNode *aNode, PRInt32 aOffset);
  NS_IMETHOD GetOriginalAnchorPoint(nsIDOMNode **aNode, PRInt32 *aOffset);

  NS_IMETHOD GetPrimaryFrameForFocusNode(nsIFrame **aResultFrame);
  nsCOMPtr<nsISupportsArray> mRangeArray;

  nsCOMPtr<nsIDOMRange> mAnchorFocusRange;
  nsCOMPtr<nsIDOMRange> mOriginalAnchorRange; //used as a point with range gravity for security
  PRBool mFixupState; //was there a fixup?
  nsDirection mDirection; //FALSE = focus, anchor;  TRUE = anchor,focus

  //batching
  PRUint32 mBatching;
  PRBool mChangesDuringBatching;
  PRBool mNotifyFrames;
  
  nsCOMPtr<nsISupportsArray> mSelectionListeners;
  
  // for nsIScriptContextOwner
  void*		mScriptObject;
  nsIFocusTracker *mTracker;
  PRBool mMouseDownState; //for drag purposes
  PRInt32 mDesiredX;
  PRBool mDesiredXSet;
  static nsIAtom *sTableAtom;
  static nsIAtom *sCellAtom;
  static nsIAtom *sTbodyAtom;
  static PRInt32 sInstanceCount;
};

class nsRangeListIterator : public nsIBidirectionalEnumerator
{
public:
/*BEGIN nsIEnumerator interfaces
see the nsIEnumerator for more details*/

  NS_DECL_ISUPPORTS

  NS_IMETHOD First();

  NS_IMETHOD Last();

  NS_IMETHOD Next();

  NS_IMETHOD Prev();

  NS_IMETHOD CurrentItem(nsISupports **aRange);

  NS_IMETHOD IsDone();

/*END nsIEnumerator interfaces*/
/*BEGIN Helper Methods*/
  NS_IMETHOD CurrentItem(nsIDOMRange **aRange);
/*END Helper Methods*/
private:
  friend class nsRangeList;
  nsRangeListIterator(nsRangeList *);
  virtual ~nsRangeListIterator();
  PRInt32     mIndex;
  nsRangeList *mRangeList;
};


nsresult NS_NewRangeList(nsIDOMSelection **);

nsresult NS_NewRangeList(nsIDOMSelection **aRangeList)
{
  nsRangeList *rlist = new nsRangeList;
  if (!rlist)
    return NS_ERROR_OUT_OF_MEMORY;
  *aRangeList = (nsIDOMSelection *)rlist;
  nsresult result = rlist->AddRef();
  if (!NS_SUCCEEDED(result))
  {
    delete rlist;
  }
  return result;
}



//Horrible statics but no choice
nsIAtom *nsRangeList::sTableAtom = 0;
nsIAtom *nsRangeList::sCellAtom = 0;
nsIAtom *nsRangeList::sTbodyAtom = 0;
PRInt32 nsRangeList::sInstanceCount = 0;
///////////BEGIN nsRangeListIterator methods

nsRangeListIterator::nsRangeListIterator(nsRangeList *aList)
:mIndex(0)
{
  if (!aList)
  {
    NS_NOTREACHED("nsRangeList");
    return;
  }
  mRangeList = aList;
  NS_INIT_REFCNT();
}



nsRangeListIterator::~nsRangeListIterator()
{
}



////////////END nsRangeListIterator methods

////////////BEGIN nsIRangeListIterator methods



NS_IMETHODIMP
nsRangeListIterator::Next()
{
  mIndex++;
  PRUint32 cnt;
  nsresult rv = mRangeList->mRangeArray->Count(&cnt);
  if (NS_FAILED(rv)) return rv;
  if (mIndex < (PRInt32)cnt)
    return NS_OK;
  return NS_ERROR_FAILURE;
}



NS_IMETHODIMP
nsRangeListIterator::Prev()
{
  mIndex--;
  if (mIndex >= 0 )
    return NS_OK;
  return NS_ERROR_FAILURE;
}



NS_IMETHODIMP
nsRangeListIterator::First()
{
  if (!mRangeList)
    return NS_ERROR_NULL_POINTER;
  mIndex = 0;
  return NS_OK;
}



NS_IMETHODIMP
nsRangeListIterator::Last()
{
  if (!mRangeList)
    return NS_ERROR_NULL_POINTER;
  PRUint32 cnt;
  nsresult rv = mRangeList->mRangeArray->Count(&cnt);
  if (NS_FAILED(rv)) return rv;
  mIndex = (PRInt32)cnt-1;
  return NS_OK;
}



NS_IMETHODIMP 
nsRangeListIterator::CurrentItem(nsISupports **aItem)
{
  if (!aItem)
    return NS_ERROR_NULL_POINTER;
  PRUint32 cnt;
  nsresult rv = mRangeList->mRangeArray->Count(&cnt);
  if (NS_FAILED(rv)) return rv;
  if (mIndex >=0 && mIndex < (PRInt32)cnt){
    *aItem = mRangeList->mRangeArray->ElementAt(mIndex);
    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}



NS_IMETHODIMP 
nsRangeListIterator::CurrentItem(nsIDOMRange **aItem)
{
  if (!aItem)
    return NS_ERROR_NULL_POINTER;
  PRUint32 cnt;
  nsresult rv = mRangeList->mRangeArray->Count(&cnt);
  if (NS_FAILED(rv)) return rv;
  if (mIndex >=0 && mIndex < (PRInt32)cnt){
    nsCOMPtr<nsISupports> indexIsupports = dont_AddRef(mRangeList->mRangeArray->ElementAt(mIndex));
    return indexIsupports->QueryInterface(nsIDOMRange::GetIID(),(void **)aItem);
  }
  return NS_ERROR_FAILURE;
}



NS_IMETHODIMP
nsRangeListIterator::IsDone()
{
  PRUint32 cnt;
  nsresult rv = mRangeList->mRangeArray->Count(&cnt);
  if (NS_FAILED(rv)) return rv;
  if (mIndex >= 0 && mIndex < (PRInt32)cnt ) { 
    return NS_COMFALSE;
  }
  return NS_OK;
}



NS_IMPL_ADDREF(nsRangeListIterator)

NS_IMPL_RELEASE(nsRangeListIterator)



NS_IMETHODIMP
nsRangeListIterator::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
  if (NULL == aInstancePtr) {
    return NS_ERROR_NULL_POINTER;
  }
  // XXX shouldn't this just do mRangeList->QueryInterface instead of
  // having a complete copy of that method here? What about AddRef and
  // Release? shouldn't they be delegated too?
  if (aIID.Equals(nsISupports::GetIID())) {
    nsIDOMSelection* tmp = mRangeList;
    *aInstancePtr = (void*)tmp;
    NS_ADDREF(mRangeList);
    return NS_OK;
  }
  if (aIID.Equals(nsIDOMSelection::GetIID())) {
    *aInstancePtr = (void *)mRangeList;
    NS_ADDREF(mRangeList);
    return NS_OK;
  }
  if (aIID.Equals(nsIFrameSelection::GetIID())) {
    *aInstancePtr = (void *)mRangeList;
    NS_ADDREF(mRangeList);
    return NS_OK;
  }
  if (aIID.Equals(nsIEnumerator::GetIID()) ||
      aIID.Equals(nsIBidirectionalEnumerator::GetIID())) {
    nsIBidirectionalEnumerator* tmp = this;
    *aInstancePtr = (void*) tmp;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  return NS_NOINTERFACE;
}

////////////END nsIRangeListIterator methods

#ifdef XP_MAC
#pragma mark -
#endif

////////////BEGIN nsRangeList methods

nsRangeList::nsRangeList()
{
  NS_INIT_REFCNT();
  NS_NewISupportsArray(getter_AddRefs(mRangeArray));
  NS_NewISupportsArray(getter_AddRefs(mSelectionListeners));
  mBatching = 0;
  mChangesDuringBatching = PR_FALSE;
  mFixupState = PR_FALSE;
  mNotifyFrames = PR_TRUE;
  mScriptObject = nsnull;
  mDirection = eDirNext;
  if (sInstanceCount <= 0)
  {
    sTableAtom = NS_NewAtom("table");
    sCellAtom = NS_NewAtom("td");
    sTbodyAtom = NS_NewAtom("tbody");
  }
  sInstanceCount ++;
}



nsRangeList::~nsRangeList()
{
  if (mRangeArray)
  {
	  PRUint32 cnt;
    nsresult rv = mRangeArray->Count(&cnt);
    NS_ASSERTION(NS_SUCCEEDED(rv), "Count failed");
    for (PRUint32 i=0;i < cnt; i++)
	  {
	    mRangeArray->RemoveElementAt(i);
	  }
  }
  
  if (mSelectionListeners)
  {
	  PRUint32 cnt;
    nsresult rv = mSelectionListeners->Count(&cnt);
    NS_ASSERTION(NS_SUCCEEDED(rv), "Count failed");
    for (PRUint32 i=0;i < cnt; i++)
	  {
	    mSelectionListeners->RemoveElementAt(i);
	  }
  }
  setAnchorFocusRange(-1);
  if (sInstanceCount <= 1)
  {
    NS_IF_RELEASE(sTableAtom);
    NS_IF_RELEASE(sCellAtom);
    NS_IF_RELEASE(sTbodyAtom);
  }
  sInstanceCount--;
}



//END nsRangeList methods



NS_IMPL_ADDREF(nsRangeList)

NS_IMPL_RELEASE(nsRangeList)


NS_IMETHODIMP
nsRangeList::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
  if (NULL == aInstancePtr) {
    return NS_ERROR_NULL_POINTER;
  }
  if (aIID.Equals(nsIDOMSelection::GetIID())) {
    nsIDOMSelection* tmp = this;
    *aInstancePtr = (void*) tmp;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  if (aIID.Equals(nsIFrameSelection::GetIID())) {
    nsIFrameSelection* tmp = this;
    *aInstancePtr = (void*) tmp;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  if (aIID.Equals(nsIScriptObjectOwner::GetIID())) {
    nsIScriptObjectOwner* tmp = this;
    *aInstancePtr = (void*) tmp;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  if (aIID.Equals(nsIEnumerator::GetIID())) {
    nsRangeListIterator *iterator =  new nsRangeListIterator(this);
    *aInstancePtr = (void *)iterator;
    return iterator->AddRef();
  }
  if (aIID.Equals(((nsISupports *)((nsIFrameSelection *)this))->GetIID())) {
    // use *first* base class for ISupports
    nsIFrameSelection* tmp1 = this;
    nsISupports* tmp2 = tmp1;
    *aInstancePtr = (void*) tmp2;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  return NS_NOINTERFACE;
}

// note: this can return a nil anchor node
NS_METHOD nsRangeList::GetAnchorNode(nsIDOMNode** aAnchorNode)
{
	if (!aAnchorNode || !mAnchorFocusRange)
		return NS_ERROR_NULL_POINTER;
  nsresult result;
  if (GetDirection() == eDirNext){
    result = mAnchorFocusRange->GetStartParent(aAnchorNode);
  }
  else{
    result = mAnchorFocusRange->GetEndParent(aAnchorNode);
  }
	return result;
}

NS_METHOD nsRangeList::GetAnchorOffset(PRInt32* aAnchorOffset)
{
	if (!aAnchorOffset || !mAnchorFocusRange)
		return NS_ERROR_NULL_POINTER;
  nsresult result;
  if (GetDirection() == eDirNext){
    result = mAnchorFocusRange->GetStartOffset(aAnchorOffset);
  }
  else{
    result = mAnchorFocusRange->GetEndOffset(aAnchorOffset);
  }
	return result;
}

// note: this can return a nil focus node
NS_METHOD nsRangeList::GetFocusNode(nsIDOMNode** aFocusNode)
{
	if (!aFocusNode || !mAnchorFocusRange)
		return NS_ERROR_NULL_POINTER;
  nsresult result;
  if (GetDirection() == eDirNext){
    result = mAnchorFocusRange->GetEndParent(aFocusNode);
  }
  else{
    result = mAnchorFocusRange->GetStartParent(aFocusNode);
  }

	return result;
}

NS_METHOD nsRangeList::GetFocusOffset(PRInt32* aFocusOffset)
{
	if (!aFocusOffset || !mAnchorFocusRange)
		return NS_ERROR_NULL_POINTER;
  nsresult result;
  if (GetDirection() == eDirNext){
    result = mAnchorFocusRange->GetEndOffset(aFocusOffset);
  }
  else{
    result = mAnchorFocusRange->GetStartOffset(aFocusOffset);
  }
	return result;
}


void nsRangeList::setAnchorFocusRange(PRInt32 index)
{
  PRUint32 cnt;
  nsresult rv = mRangeArray->Count(&cnt);
  if (NS_FAILED(rv)) return;    // XXX error?
  if (((PRUint32)index) >= cnt )
    return;
  if (index < 0) //release all
  {
    mAnchorFocusRange = nsCOMPtr<nsIDOMRange>();
  }
  else{
    nsCOMPtr<nsISupports> indexIsupports = dont_AddRef(mRangeArray->ElementAt(index));
    mAnchorFocusRange = do_QueryInterface(indexIsupports);
  }
}



nsIDOMNode*
nsRangeList::FetchAnchorNode()
{  //where did the selection begin
  nsCOMPtr<nsIDOMNode>returnval;
  GetAnchorNode(getter_AddRefs(returnval));//this queries
  return returnval;
}//at end it will release, no addreff was called



PRInt32
nsRangeList::FetchAnchorOffset()
{
  PRInt32 returnval;
  if (NS_SUCCEEDED(GetAnchorOffset(&returnval)))//this queries
    return returnval;
  return 0;
}



nsIDOMNode*
nsRangeList::FetchOriginalAnchorNode()  //where did the ORIGINAL selection begin
{
  nsCOMPtr<nsIDOMNode>returnval;
  PRInt32 unused;
  GetOriginalAnchorPoint(getter_AddRefs(returnval), &unused);//this queries
  return returnval;
}



PRInt32
nsRangeList::FetchOriginalAnchorOffset()
{
  nsCOMPtr<nsIDOMNode>unused;
  PRInt32 returnval;
  if (NS_SUCCEEDED(GetOriginalAnchorPoint(getter_AddRefs(unused), &returnval)))//this queries
    return returnval;
  return 0;
}



nsIDOMNode*
nsRangeList::FetchFocusNode()
{   //where is the carret
  nsCOMPtr<nsIDOMNode>returnval;
  GetFocusNode(getter_AddRefs(returnval));//this queries
  return returnval;
}//at end it will release, no addreff was called



PRInt32
nsRangeList::FetchFocusOffset()
{
  PRInt32 returnval;
  if (NS_SUCCEEDED(GetFocusOffset(&returnval)))//this queries
    return returnval;
  return 0;
}



nsIDOMNode*
nsRangeList::FetchStartParent(nsIDOMRange *aRange)   //skip all the com stuff and give me the start/end
{
  if (!aRange)
    return nsnull;
  nsCOMPtr<nsIDOMNode> returnval;
  aRange->GetStartParent(getter_AddRefs(returnval));
  return returnval;
}



PRInt32
nsRangeList::FetchStartOffset(nsIDOMRange *aRange)
{
  if (!aRange)
    return nsnull;
  PRInt32 returnval;
  if (NS_SUCCEEDED(aRange->GetStartOffset(&returnval)))
    return returnval;
  return 0;
}



nsIDOMNode*
nsRangeList::FetchEndParent(nsIDOMRange *aRange)     //skip all the com stuff and give me the start/end
{
  if (!aRange)
    return nsnull;
  nsCOMPtr<nsIDOMNode> returnval;
  aRange->GetEndParent(getter_AddRefs(returnval));
  return returnval;
}



PRInt32
nsRangeList::FetchEndOffset(nsIDOMRange *aRange)
{
  if (!aRange)
    return nsnull;
  PRInt32 returnval;
  if (NS_SUCCEEDED(aRange->GetEndOffset(&returnval)))
    return returnval;
  return 0;
}



nscoord
nsRangeList::FetchDesiredX() //the x position requested by the Key Handling for up down
{
  if (!mTracker)
  {
    NS_ASSERTION(0,"fetch desired X failed\n");
    return -1;
  }
  if (mDesiredXSet)
    return mDesiredX;
  else {
    nsPoint coord;
    PRBool  collapsed;
    nsCOMPtr<nsICaret> caret;
    nsCOMPtr<nsIPresContext> context;
    nsCOMPtr<nsIPresShell> shell;
    nsresult result = mTracker->GetPresContext(getter_AddRefs(context));
    if (NS_FAILED(result) || !context)
      return result;
    result = context->GetShell(getter_AddRefs(shell));
    if (NS_FAILED(result) || !shell)
      return result;
    result = shell->GetCaret(getter_AddRefs(caret));
    if (NS_FAILED(result) || !caret)
      return result;

    result = caret->GetWindowRelativeCoordinates(coord,collapsed);
    if (NS_FAILED(result))
      return result;
    return coord.x;
  }
}



void
nsRangeList::InvalidateDesiredX() //do not listen to mDesiredX you must get another.
{
  mDesiredXSet = PR_FALSE;
}



void
nsRangeList::SetDesiredX(nscoord aX) //set the mDesiredX
{
  mDesiredX = aX;
  mDesiredXSet = PR_TRUE;
}



nsresult
nsRangeList::AddItem(nsISupports *aItem)
{
  if (!mRangeArray)
    return NS_ERROR_FAILURE;
  if (!aItem)
    return NS_ERROR_NULL_POINTER;
  mRangeArray->AppendElement(aItem);
  return NS_OK;
}



nsresult
nsRangeList::RemoveItem(nsISupports *aItem)
{
  if (!mRangeArray)
    return NS_ERROR_FAILURE;
  if (!aItem )
    return NS_ERROR_NULL_POINTER;
  PRUint32 cnt;
  nsresult rv = mRangeArray->Count(&cnt);
  if (NS_FAILED(rv)) return rv;
  for (PRUint32 i = 0; i < cnt;i++)
  {
    nsCOMPtr<nsISupports> indexIsupports = dont_AddRef(mRangeArray->ElementAt(i));

    if (aItem == indexIsupports.get())
    {
      mRangeArray->RemoveElementAt(i);
      return NS_OK;
    }
  }
  return NS_COMFALSE;
}



nsresult
nsRangeList::Clear()
{
  setAnchorFocusRange(-1);
  if (!mRangeArray)
    return NS_ERROR_FAILURE;
  // Get an iterator
  while (PR_TRUE)
  {
    PRUint32 cnt;
    nsresult rv = mRangeArray->Count(&cnt);
    if (NS_FAILED(rv)) return rv;
    if (cnt == 0)
      break;
    nsCOMPtr<nsIDOMRange> range;
    nsCOMPtr<nsISupports> isupportsindex = dont_AddRef(mRangeArray->ElementAt(0));
    range = do_QueryInterface(isupportsindex);
    mRangeArray->RemoveElementAt(0);
    selectFrames(range, 0);
    // Does RemoveElementAt also delete the elements?
  }

  return NS_OK;
}



#ifdef XP_MAC
#pragma mark -
#endif

//BEGIN nsIFrameSelection methods


void printRange(nsIDOMRange *aDomRange)
{
  if (!aDomRange)
  {
    printf("NULL nsIDOMRange\n");
  }
  nsCOMPtr<nsIDOMNode> startNode;
  nsCOMPtr<nsIDOMNode> endNode;
  PRInt32 startOffset;
  PRInt32 endOffset;
  aDomRange->GetStartParent(getter_AddRefs(startNode));
  aDomRange->GetStartOffset(&startOffset);
  aDomRange->GetEndParent(getter_AddRefs(endNode));
  aDomRange->GetEndOffset(&endOffset);
  
  printf("range: 0x%lx\t start: 0x%lx %ld, \t end: 0x%lx,%ld\n",
         (unsigned long)aDomRange,
         (unsigned long)(nsIDOMNode*)startNode, (long)startOffset,
         (unsigned long)(nsIDOMNode*)endNode, (long)endOffset);
         
}


nsCOMPtr<nsIAtom> GetTag(nsIDOMNode *aNode)
{
  nsCOMPtr<nsIAtom> atom;
  
  if (!aNode) 
  {
    NS_NOTREACHED("null node passed to nsHTMLEditRules::GetTag()");
    return atom;
  }
  
  nsCOMPtr<nsIContent> content = do_QueryInterface(aNode);
  if (content)
    content->GetTag(*getter_AddRefs(atom));

  return atom;
}



nsresult
ParentOffset(nsIDOMNode *aNode, nsIDOMNode **aParent, PRInt32 *aChildOffset)
{
  if (!aNode || !aParent || !aChildOffset)
    return NS_ERROR_NULL_POINTER;
  nsresult result = NS_OK;
  nsCOMPtr<nsIContent> content;
  result = aNode->QueryInterface(nsIContent::GetIID(),getter_AddRefs(content));
  if (NS_SUCCEEDED(result) && content)
  {
    nsCOMPtr<nsIContent> parent;
    result = content->GetParent(*getter_AddRefs(parent));
    if (NS_SUCCEEDED(result))
    {
      result = parent->IndexOf(content, *aChildOffset);
      if (NS_SUCCEEDED(result))
        result = parent->QueryInterface(nsIDOMNode::GetIID(),(void **)aParent);
    }
  }
  return result;
}



NS_IMETHODIMP
nsRangeList::Init(nsIFocusTracker *aTracker)
{
  mTracker = aTracker;
  mMouseDownState = PR_FALSE;
  mDesiredXSet = PR_FALSE;
  return NS_OK;
}



NS_IMETHODIMP
nsRangeList::ShutDown()
{
  return NS_OK;
}

  
  
NS_IMETHODIMP
nsRangeList::HandleTextEvent(nsGUIEvent *aGUIEvent)
{
	if (!aGUIEvent)
		return NS_ERROR_NULL_POINTER;

#ifdef DEBUG_TAGUE
	printf("nsRangeList: HandleTextEvent\n");
#endif
  nsresult result(NS_OK);
	if (NS_TEXT_EVENT == aGUIEvent->message) {

		result = ScrollIntoView();
	}
	return result;
}



/** This raises a question, if this method is called and the aFrame does not reflect the current
 *  focus  DomNode, it is invalid?  The answer now is yes.
 */
NS_IMETHODIMP
nsRangeList::HandleKeyEvent(nsGUIEvent *aGuiEvent)
{
  if (!aGuiEvent)
    return NS_ERROR_NULL_POINTER;
  STATUS_CHECK_RETURN_MACRO();

  nsresult result = NS_OK;
  if (NS_KEY_DOWN == aGuiEvent->message) {
    nsKeyEvent *keyEvent = (nsKeyEvent *)aGuiEvent; //this is ok. It really is a keyevent
    nsCOMPtr<nsIDOMNode> weakNodeUsed;
    PRInt32 offsetused = 0;
    nsSelectionAmount amount = eSelectCharacter;
    if (keyEvent->isControl)
      amount = eSelectWord;

    PRBool isCollapsed;
    nscoord desiredX; //we must keep this around and revalidate it when its just UP/DOWN

    result = GetIsCollapsed(&isCollapsed);
    if (NS_FAILED(result))
      return result;
    if (keyEvent->keyCode == nsIDOMUIEvent::VK_UP || keyEvent->keyCode == nsIDOMUIEvent::VK_DOWN)
      desiredX= FetchDesiredX();

    if (!isCollapsed && !keyEvent->isShift) {
      switch (keyEvent->keyCode){
        case nsIDOMUIEvent::VK_LEFT  : 
        case nsIDOMUIEvent::VK_UP    : {
            if ((GetDirection() == eDirPrevious)) { //f,a
              offsetused = FetchFocusOffset();
              weakNodeUsed = FetchFocusNode();
            }
            else {
              offsetused = FetchAnchorOffset();
              weakNodeUsed = FetchAnchorNode();
            }
            result = Collapse(weakNodeUsed,offsetused);

                                       } break;
        case nsIDOMUIEvent::VK_RIGHT : 
        case nsIDOMUIEvent::VK_DOWN  : {
            if ((GetDirection() == eDirPrevious)) { //f,a
              offsetused = FetchAnchorOffset();
              weakNodeUsed = FetchAnchorNode();
            }
            else {
              offsetused = FetchFocusOffset();
              weakNodeUsed = FetchFocusNode();
            }
            result = Collapse(weakNodeUsed,offsetused);
                                       } break;
      }
      if (keyEvent->keyCode == nsIDOMUIEvent::VK_UP || keyEvent->keyCode == nsIDOMUIEvent::VK_DOWN)
        SetDesiredX(desiredX);
      return NS_OK;
    }

    offsetused = FetchFocusOffset();
    weakNodeUsed = FetchFocusNode();
    nsIFrame *frame;
    nsCOMPtr<nsIContent> content;
    result = GetPrimaryFrameForFocusNode(&frame);
    if (NS_FAILED(result))
      return result;
    switch (keyEvent->keyCode){
      case nsIDOMUIEvent::VK_LEFT  : 
        {
        //we need to look for the previous PAINTED location to move the cursor to.
          if (NS_SUCCEEDED(result) && NS_SUCCEEDED(frame->PeekOffset(mTracker, desiredX, amount, eDirPrevious, offsetused, getter_AddRefs(content), &offsetused, PR_FALSE)) && content){
            result = TakeFocus(content, offsetused, offsetused, keyEvent->isShift);
          }
          result = ScrollIntoView();
        }
        break;
      case nsIDOMUIEvent::VK_RIGHT : 
        {
        //we need to look for the previous PAINTED location to move the cursor to.
          if (NS_SUCCEEDED(result) && NS_SUCCEEDED(frame->PeekOffset(mTracker, desiredX, amount, eDirNext, offsetused, getter_AddRefs(content), &offsetused, PR_FALSE)) && content){
            result = TakeFocus(content, offsetused, offsetused, keyEvent->isShift);
          }
          result = ScrollIntoView();
        }
       break;
      case nsIDOMUIEvent::VK_UP : 
      case nsIDOMUIEvent::VK_DOWN : 
        {
  //we need to look for the previous PAINTED location to move the cursor to.
          amount = eSelectLine;
          if (nsIDOMUIEvent::VK_UP == keyEvent->keyCode)
          {
            if (NS_SUCCEEDED(result) && NS_SUCCEEDED(frame->PeekOffset(mTracker, desiredX, amount, eDirPrevious, offsetused, getter_AddRefs(content), &offsetused, PR_FALSE)) && content){
              result = TakeFocus(content, offsetused, offsetused, keyEvent->isShift);
            }
          }
          else
            if (NS_SUCCEEDED(result) && NS_SUCCEEDED(frame->PeekOffset(mTracker, desiredX, amount, eDirNext, offsetused, getter_AddRefs(content), &offsetused, PR_FALSE)) && content){
              result = TakeFocus(content, offsetused, offsetused, keyEvent->isShift);
            }
          result = ScrollIntoView();
          SetDesiredX(desiredX);
        }
        break;
#ifdef DEBUG_NAVIGATION
        printf("debug vk down\n");
#endif
        break;
    default :break;
    }
  }
  return result;
}


//utility method to get the primary frame of node or use the offset to get frame of child node
NS_IMETHODIMP
nsRangeList::GetPrimaryFrameForFocusNode(nsIFrame **aReturnFrame)
{
  if (!aReturnFrame)
    return NS_ERROR_NULL_POINTER;
  
  nsresult	result = NS_OK;
  
  nsCOMPtr<nsIDOMNode> node = dont_QueryInterface(FetchFocusNode());
  nsCOMPtr<nsIContent> content = do_QueryInterface(node, &result);
  if (NS_FAILED(result) || !content)
    return result;
  
  PRBool canContainChildren = PR_FALSE;
  result = content->CanContainChildren(canContainChildren);
  if (NS_SUCCEEDED(result) && canContainChildren)
  {
    PRInt32 offset = FetchFocusOffset();
    if (GetDirection() == eDirNext)
      offset--;
    if (offset >0)
    {
      nsCOMPtr<nsIContent> child;
      result = content->ChildAt(offset, *getter_AddRefs(child));
      if (NS_FAILED(result) || !child) //out of bounds?
        return result;
      content = child;//releases the focusnode
    }
  }
  result = mTracker->GetPrimaryFrameFor(content,aReturnFrame);
  return result;
}


#ifdef DEBUG
void nsRangeList::printSelection()
{
  PRUint32 cnt;
  (void)mRangeArray->Count(&cnt);
  printf("nsRangeList 0x%lx: %d items\n", (unsigned long)this,
         mRangeArray ? cnt : -99);

  // Get an iterator
  nsRangeListIterator iter(this);
  nsresult res = iter.First();
  if (!NS_SUCCEEDED(res))
  {
    printf(" Can't get an iterator\n");
    return;
  }

  while (iter.IsDone())
  {
    nsCOMPtr<nsIDOMRange> range;
    res = iter.CurrentItem(getter_AddRefs(range));
    if (!NS_SUCCEEDED(res))
    {
      printf(" OOPS\n");
      return;
    }
    printRange(range);
    iter.Next();
  }

  printf("Anchor is 0x%lx, %d\n",
         (unsigned long)(nsIDOMNode*)FetchAnchorNode(), FetchAnchorOffset());
  printf("Focus is 0x%lx, %d\n",
         (unsigned long)(nsIDOMNode*)FetchFocusNode(), FetchFocusOffset());
  printf(" ... end of selection\n");
}
#endif /* DEBUG */



//the idea of this helper method is to select, deselect "top to bottom" traversing through the frames
NS_IMETHODIMP
nsRangeList::selectFrames(nsIDOMRange *aRange, PRBool aFlags)
{
  if (!aRange) 
    return NS_ERROR_NULL_POINTER;
  nsCOMPtr<nsIContentIterator> iter;
  nsresult result = nsComponentManager::CreateInstance(kCSubtreeIteratorCID, nsnull,
                                              nsIContentIterator::GetIID(), 
                                              getter_AddRefs(iter));
  if ((NS_SUCCEEDED(result)) && iter)
  {
    iter->Init(aRange);
    // loop through the content iterator for each content node
    // for each text node:
    // get the frame for the content, and from it the style context
    // ask the style context about the property
    nsCOMPtr<nsIContent> content;
    PRBool drawWholeContent = PR_FALSE;
    nsIFrame *frame;
//we must call first one explicitly
    content = do_QueryInterface(FetchStartParent(aRange), &result);
    if (NS_FAILED(result))
      return result;
    PRBool canContainChildren = PR_FALSE;
    result = content->CanContainChildren(canContainChildren);
    if (NS_SUCCEEDED(result) && !canContainChildren)
    {
      result = mTracker->GetPrimaryFrameFor(content, &frame);
      if (NS_SUCCEEDED(result) && frame)
        frame->SetSelected(aRange,aFlags,eSpreadDown);//spread from here to hit all frames in flow
    }
//end start content
    result = iter->First();
    if (NS_SUCCEEDED(result))
    {
      while (NS_COMFALSE == iter->IsDone())
      {
        result = iter->CurrentNode(getter_AddRefs(content));
        if (NS_FAILED(result))
          return result;
        result = mTracker->GetPrimaryFrameFor(content, &frame);
        if (NS_SUCCEEDED(result) && frame)
           frame->SetSelected(aRange,aFlags,eSpreadDown);//spread from here to hit all frames in flow
        result = iter->Next();
        if (NS_FAILED(result))
      	  return result;
      }
    }
//we must now do the last one  if it is not the same as the first
    if (FetchEndParent(aRange) != FetchStartParent(aRange))
    {
      content = do_QueryInterface(FetchEndParent(aRange), &result);
      if (NS_FAILED(result))
        return result;
      canContainChildren = PR_FALSE;
      result = content->CanContainChildren(canContainChildren);
      if (NS_SUCCEEDED(result) && !canContainChildren)
      {
        result = mTracker->GetPrimaryFrameFor(content, &frame);
        if (NS_SUCCEEDED(result) && frame)
           frame->SetSelected(aRange,aFlags,eSpreadDown);//spread from here to hit all frames in flow
      }
    }
//end end parent
  }
  return result;
}


/**
hard to go from nodes to frames, easy the other way!
 */
NS_IMETHODIMP
nsRangeList::TakeFocus(nsIContent *aNewFocus, PRUint32 aContentOffset, 
                       PRUint32 aContentEndOffset, PRBool aContinueSelection)
{
  if (!aNewFocus)
    return NS_ERROR_NULL_POINTER;
  if (GetBatching())
    return NS_ERROR_FAILURE;
  STATUS_CHECK_RETURN_MACRO();
  //HACKHACKHACK
  nsCOMPtr<nsIContent> content;
  nsCOMPtr<nsIDOMNode> domNode;
  nsCOMPtr<nsIContent> parent;
  nsCOMPtr<nsIContent> parent2;
  if (NS_FAILED(aNewFocus->GetParent(*getter_AddRefs(parent))) || !parent)
    return NS_ERROR_FAILURE;
  //if (NS_FAILED(parent->GetParent(*getter_AddRefs(parent2))) || !parent2)
    //return NS_ERROR_FAILURE;

  //END HACKHACKHACK /checking for root frames/content

  domNode = do_QueryInterface(aNewFocus);
  //traverse through document and unselect crap here
  if (!aContinueSelection){ //single click? setting cursor down
    PRUint32 batching = mBatching;//hack to use the collapse code.
    PRBool changes = mChangesDuringBatching;
    mBatching = 1;
    Collapse(domNode, aContentOffset);
    mBatching = batching;
    mChangesDuringBatching = changes;
    if (aContentEndOffset > aContentOffset)
      Extend(domNode,aContentEndOffset);
  }
  else {
    // Now update the range list:
    if (aContinueSelection && domNode)
    {
      Extend(domNode, aContentOffset);
      if (mDirection == eDirNext && aContentEndOffset > aContentOffset) //didnt go far enough 
      {
        Extend(domNode, aContentEndOffset);//this will only redraw the diff 
      }
    }
  }
    
  return NotifySelectionListeners();
}



NS_METHOD
nsRangeList::LookUpSelection(nsIContent *aContent, PRInt32 aContentOffset, PRInt32 aContentLength,
                             PRInt32 *aStart, PRInt32 *aEnd, PRBool *aDrawSelected, PRUint32 aFlag/*not used*/)
{
  if (!aContent || !aStart || !aEnd || !aDrawSelected)
    return NS_ERROR_NULL_POINTER;
  if (aFlag)
    return NS_ERROR_NOT_IMPLEMENTED;

  STATUS_CHECK_RETURN_MACRO();
  //THIS WILL NOT WORK FOR MULTIPLE SELECTIONS YET!!!
  nsCOMPtr<nsIDOMNode> passedInNode;
  passedInNode = do_QueryInterface(aContent);
  if (!passedInNode)
    return NS_ERROR_FAILURE;

  *aDrawSelected = PR_FALSE;
  PRUint32 cnt;
  nsresult rv = mRangeArray->Count(&cnt);
  if (NS_FAILED(rv)) return rv;
  for (PRUint32 i =0; i<cnt && i < 1; i++){
    nsCOMPtr<nsIDOMRange> range;
    nsCOMPtr<nsISupports> isupportsindex = dont_AddRef(mRangeArray->ElementAt(i));
    range = do_QueryInterface(isupportsindex);
    if (range){
      nsCOMPtr<nsIDOMNode> startNode;
      nsCOMPtr<nsIDOMNode> endNode;
      PRInt32 startOffset;
      PRInt32 endOffset;
      range->GetStartParent(getter_AddRefs(startNode));
      range->GetStartOffset(&startOffset);
      range->GetEndParent(getter_AddRefs(endNode));
      range->GetEndOffset(&endOffset);
      if (passedInNode == startNode && passedInNode == endNode){
        if (startOffset < (aContentOffset + aContentLength)  &&
            endOffset > aContentOffset){
          *aDrawSelected = PR_TRUE;
          *aStart = PR_MAX(0,startOffset - aContentOffset);
          *aEnd = PR_MIN(aContentLength, endOffset - aContentOffset);
        }
      }
      else if (passedInNode == startNode){
        if (startOffset < (aContentOffset + aContentLength)){
          *aDrawSelected = PR_TRUE;
          *aStart = PR_MAX(0,startOffset - aContentOffset);
          *aEnd = aContentLength;
        }
      }
      else if (passedInNode == endNode){
        if (endOffset > aContentOffset){
          *aDrawSelected = PR_TRUE;
          *aStart = 0;
          *aEnd = PR_MIN(aContentLength, endOffset - aContentOffset);
        }
      }
      else {
        *aDrawSelected = PR_TRUE;
        *aStart = 0;
        *aEnd = aContentLength;
      }
    }
    else
      return NS_ERROR_FAILURE;
  }
  return NS_OK;
}



NS_METHOD 
nsRangeList::SetMouseDownState(PRBool aState)
{
  mMouseDownState = aState;
  return NS_OK;
}



NS_METHOD
nsRangeList::GetMouseDownState(PRBool *aState)
{
  if (!aState)
    return NS_ERROR_NULL_POINTER;
  *aState = mMouseDownState;
  return NS_OK;
}



//////////END FRAMESELECTION
NS_METHOD
nsRangeList::AddSelectionListener(nsIDOMSelectionListener* inNewListener)
{
  if (!mSelectionListeners)
    return NS_ERROR_FAILURE;
  if (!inNewListener)
    return NS_ERROR_NULL_POINTER;
  mSelectionListeners->AppendElement(inNewListener);		// addrefs
  return NS_OK;
}



NS_IMETHODIMP
nsRangeList::RemoveSelectionListener(nsIDOMSelectionListener* inListenerToRemove)
{
  if (!mSelectionListeners)
    return NS_ERROR_FAILURE;
  if (!inListenerToRemove )
    return NS_ERROR_NULL_POINTER;
    
  return mSelectionListeners->RemoveElement(inListenerToRemove);		// releases
}



NS_IMETHODIMP
nsRangeList::StartBatchChanges()
{
  nsresult result(NS_OK);
  mBatching++;
  return result;
}


 
NS_IMETHODIMP
nsRangeList::EndBatchChanges()
{
  nsresult result(NS_OK);
  mBatching--;
  NS_ASSERTION(mBatching >=0,"Bad mBatching");
  if (mBatching == 0 && mChangesDuringBatching){
    mChangesDuringBatching = PR_FALSE;
    NotifySelectionListeners();
  }
  return result;
}

  
  
NS_IMETHODIMP
nsRangeList::ScrollIntoView()
{
  nsresult result;
  nsIFrame *frame;
  result = GetPrimaryFrameForFocusNode(&frame);
  if (NS_FAILED(result))
    return result;
  result = mTracker->ScrollFrameIntoView(frame);
  return result;
}



nsresult
nsRangeList::NotifySelectionListeners()
{
  if (!mSelectionListeners)
    return NS_ERROR_FAILURE;
 
  if (GetBatching()){
    SetDirty();
    return NS_OK;
  }
  PRUint32 cnt;
  nsresult rv = mSelectionListeners->Count(&cnt);
  if (NS_FAILED(rv)) return rv;
  for (PRUint32 i = 0; i < cnt;i++)
  {
    nsCOMPtr<nsIDOMSelectionListener> thisListener;
    nsCOMPtr<nsISupports> isupports(dont_AddRef(mSelectionListeners->ElementAt(i)));
    thisListener = do_QueryInterface(isupports);
    if (thisListener)
    	thisListener->NotifySelectionChanged();
  }
	return NS_OK;
}

//END nsIFrameSelection methods

#ifdef XP_MAC
#pragma mark -
#endif

//BEGIN nsIDOMSelection interface implementations

/** ClearSelection zeroes the selection
 */
NS_IMETHODIMP
nsRangeList::ClearSelection()
{
  nsresult	result = Clear();
  if (NS_FAILED(result))
  	return result;
  	
  return NotifySelectionListeners();
  // Also need to notify the frames!
  // PresShell::ContentChanged should do that on DocumentChanged
}

/** AddRange adds the specified range to the selection
 *  @param aRange is the range to be added
 */
NS_IMETHODIMP
nsRangeList::AddRange(nsIDOMRange* aRange)
{
  nsresult      result = AddItem(aRange);
  
  if (NS_FAILED(result))
    return result;
  PRInt32 count;
  result = GetRangeCount(&count);
  if (NS_FAILED(result))
    return result;
  if (count <= 0)
  {
    NS_ASSERTION(0,"bad count after additem\n");
    return NS_ERROR_FAILURE;
  }
  setAnchorFocusRange(count -1);
  selectFrames(aRange, PR_TRUE);        
  ScrollIntoView();

  return NotifySelectionListeners();
}


/*
 * Collapse sets the whole selection to be one point.
 */
NS_IMETHODIMP
nsRangeList::Collapse(nsIDOMNode* aParentNode, PRInt32 aOffset)
{
  nsresult result;
  InvalidateDesiredX();
  // Delete all of the current ranges
  if (!mRangeArray)
    return NS_ERROR_FAILURE;
  if (NS_FAILED(SetOriginalAnchorPoint(aParentNode,aOffset)))
    return NS_ERROR_FAILURE; //???
  Clear();

  nsCOMPtr<nsIDOMRange> range;
  result = nsComponentManager::CreateInstance(kRangeCID, nsnull,
                                     nsIDOMRange::GetIID(),
                                     getter_AddRefs(range));
  if (NS_FAILED(result))
    return result;

  if (! range){
    NS_ASSERTION(PR_FALSE,"Create Instance Failed nsRangeList::Collapse");
    return NS_ERROR_UNEXPECTED;
  }
  result = range->SetEnd(aParentNode, aOffset);
  if (NS_FAILED(result))
    return result;
  result = range->SetStart(aParentNode, aOffset);
  if (NS_FAILED(result))
    return result;

#ifdef DEBUG_SELECTION
  if (aParentNode)
  {
    nsCOMPtr<nsIContent>content;
    content = do_QueryInterface(aParentNode);
    nsIAtom *tag;
    content->GetTag(tag);
    if (tag)
    {
	    nsString tagString;
	    tag->ToString(tagString);
	    char * tagCString = tagString.ToNewCString();
	    printf ("Sel. Collapse to %p %s %d\n", content, tagCString, aOffset);
	    delete [] tagCString;
    }
  }
  else {
    printf ("Sel. Collapse set to null parent.\n");
  }
#endif


  result = AddItem(range);
  setAnchorFocusRange(0);
  selectFrames(range,PR_TRUE);
  if (NS_FAILED(result))
    return result;
    
	return NotifySelectionListeners();
}

/*
 * IsCollapsed -- is the whole selection just one point, or unset?
 */
NS_IMETHODIMP
nsRangeList::GetIsCollapsed(PRBool* aIsCollapsed)
{
	if (!aIsCollapsed)
		return NS_ERROR_NULL_POINTER;
		
  PRUint32 cnt = 0;
  if (mRangeArray) {
    nsresult rv = mRangeArray->Count(&cnt);
    if (NS_FAILED(rv)) return rv;
  }
  if (!mRangeArray || (cnt == 0))
  {
    *aIsCollapsed = PR_TRUE;
    return NS_OK;
  }
  
  if (cnt != 1)
  {
    *aIsCollapsed = PR_FALSE;
    return NS_OK;
  }
  
  nsCOMPtr<nsISupports> nsisup(dont_AddRef(mRangeArray->ElementAt(0)));
  nsCOMPtr<nsIDOMRange> range;
  nsresult rv;
  range = do_QueryInterface(nsisup,&rv);
  if (NS_FAILED(rv))
  {
    return rv;
  }
                             
  return (range->GetIsCollapsed(aIsCollapsed));
}


NS_IMETHODIMP
nsRangeList::GetRangeCount(PRInt32* aRangeCount)
{
	if (!aRangeCount)
		return NS_ERROR_NULL_POINTER;

	if (mRangeArray)
	{
		PRUint32 cnt;
    nsresult rv = mRangeArray->Count(&cnt);
    if (NS_FAILED(rv)) return rv;
    *aRangeCount = cnt;
	}
	else
	{
		*aRangeCount = 0;
	}
	
	return NS_OK;
}

NS_IMETHODIMP
nsRangeList::GetRangeAt(PRInt32 aIndex, nsIDOMRange** aReturn)
{
	if (!aReturn)
		return NS_ERROR_NULL_POINTER;

	if (!mRangeArray)
		return NS_ERROR_INVALID_ARG;
		
	PRUint32 cnt;
  nsresult rv = mRangeArray->Count(&cnt);
  if (NS_FAILED(rv)) return rv;
	if (aIndex < 0 || ((PRUint32)aIndex) >= cnt)
		return NS_ERROR_INVALID_ARG;

	// the result of all this is one additional ref on the item, as
	// the caller would expect.
	//
	// ElementAt addrefs once
	// do_QueryInterface addrefs once
	// when the COMPtr goes out of scope, it releases.
	//
	nsISupports*	element = mRangeArray->ElementAt((PRUint32)aIndex);
	nsCOMPtr<nsIDOMRange>	foundRange = do_QueryInterface(element);
	*aReturn = foundRange;
	
	return NS_OK;
}


//may change parameters may not.
//return NS_ERROR_FAILED if invalid new selection between anchor and passed in parameters
NS_IMETHODIMP
nsRangeList::FixupSelectionPoints(nsIDOMRange *aRange , nsDirection *aDir, PRBool *aFixupState)
{
  if (!aRange || !aFixupState)
    return NS_ERROR_NULL_POINTER;
  *aFixupState = PR_FALSE;
  nsresult res;

  //startNode is the beginning or "anchor" of the range
  //end Node is the end or "focus of the range
  nsCOMPtr<nsIDOMNode> startNode;
  nsCOMPtr<nsIDOMNode> endNode;
  PRInt32 startOffset;
  PRInt32 endOffset;
  nsresult result;
  if (*aDir == eDirNext)
  {
    if (NS_FAILED(GetOriginalAnchorPoint(getter_AddRefs(startNode), &startOffset)))
    {
      aRange->GetStartParent(getter_AddRefs(startNode));
      aRange->GetStartOffset(&startOffset);
    }
    aRange->GetEndParent(getter_AddRefs(endNode));
    aRange->GetEndOffset(&endOffset);
  }
  else
  {
    if (NS_FAILED(GetOriginalAnchorPoint(getter_AddRefs(startNode), &startOffset)))
    {
      aRange->GetEndParent(getter_AddRefs(startNode));
      aRange->GetEndOffset(&startOffset);
    }
    aRange->GetStartParent(getter_AddRefs(endNode));
    aRange->GetStartOffset(&endOffset);
  }
  if (!startNode || !endNode)
    return NS_ERROR_FAILURE;

  // if end node is a tbody then all bets are off we cannot select "rows"
  nsCOMPtr<nsIAtom> atom;
  atom = GetTag(endNode);
  if (atom.get() == sTbodyAtom)
    return NS_ERROR_FAILURE; //cannot select INTO row node ony cells

  //get common parent
  nsCOMPtr<nsIDOMNode> parent;
  res = aRange->GetCommonParent(getter_AddRefs(parent));
  if (NS_FAILED(res) || !parent)
    return res;
 
  //look for dest. if you see a cell you are in "cell mode"
  //if you see a table you select "whole" table

  //src first 
  //if src EVER hits a table, all bets are off!
  nsCOMPtr<nsIDOMNode> tempNode;
  nsCOMPtr<nsIDOMNode> tempNode2;
  PRBool cellMode = PR_FALSE;
  PRBool dirty = PR_FALSE;
  PRBool fixupState = PR_FALSE;
  if (startNode != endNode)
  {
    if (parent != startNode)
    {
      result = startNode->GetParentNode(getter_AddRefs(tempNode));
      if (NS_FAILED(result))
        return result;
      while (tempNode != parent)
      {
        atom = GetTag(tempNode);
        if (atom.get() == sTableAtom) 
        {
          return NS_ERROR_FAILURE; //cannot do this
        }
        else if (atom.get() == sCellAtom) //you are in "cell" mode put selection to end of cell
        {
          cellMode = PR_TRUE;
          result = ParentOffset(tempNode, getter_AddRefs(startNode), &startOffset);
          if (NS_FAILED(result))
            return result;
          if (*aDir == eDirPrevious) //select after
            startOffset++;
          dirty = PR_TRUE;
        }
        result = tempNode->GetParentNode(getter_AddRefs(tempNode2));
        if (NS_FAILED(result))
          return result;
        tempNode = tempNode2;
      }
    }
  
  //now for dest node
    if (parent != endNode)
    {
      result = endNode->GetParentNode(getter_AddRefs(tempNode));
      PRBool found = !cellMode;
      if (NS_FAILED(result))
        return result;
      while (tempNode != parent)
      {
        nsCOMPtr<nsIAtom> atom = GetTag(tempNode);
        if (atom.get() == sTableAtom) //select whole table  if in cell mode, wait for cell
        {
          if (!cellMode)
          {
            result = ParentOffset(tempNode, getter_AddRefs(endNode), &endOffset);
            if (NS_FAILED(result))
              return result;
            if (*aDir == eDirNext) //select after
              endOffset++;
            dirty = PR_TRUE;
          }
          else
            found = PR_FALSE; //didnt find the right cell yet
        }
        else if (atom.get() == sCellAtom) //you are in "cell" mode put selection to end of cell
        {
          result = ParentOffset(tempNode, getter_AddRefs(endNode), &endOffset);
          if (NS_FAILED(result))
            return result;
          if (*aDir == eDirNext) //select after
            endOffset++;
          found = PR_TRUE;
          dirty = PR_TRUE;
        }
        result = tempNode->GetParentNode(getter_AddRefs(tempNode2));
        if (NS_FAILED(result))
          return result;
        tempNode = tempNode2;
      }
      if (!found)
        return NS_ERROR_FAILURE;
    }
  }
  if (FetchAnchorNode() != startNode.get() || startOffset != FetchAnchorOffset())
    dirty = PR_TRUE; //something has changed we are dirty no matter what
  if (dirty && *aDir != mDirection) //fixup took place but new direction all bets are off
  {
    *aFixupState = PR_TRUE;
    mFixupState = PR_FALSE;
  }
  else
    if (startNode.get() != FetchOriginalAnchorNode() && PR_TRUE == mFixupState) //no longer a fixup
    {
      *aFixupState = PR_TRUE;
      mFixupState = PR_FALSE;
    }
    else
    {
      mFixupState = dirty;
      *aFixupState = dirty;
    }
  if (dirty){
    if (*aDir == eDirNext)
    {
      if (NS_FAILED(aRange->SetStart(startNode,startOffset)) || NS_FAILED(aRange->SetEnd(endNode, endOffset)))
      {
        *aDir = eDirPrevious;
        aRange->SetStart(endNode, endOffset);
        aRange->SetEnd(startNode, startOffset);
      }
    }
    else
    {
      if (NS_FAILED(aRange->SetStart(endNode,endOffset)) || NS_FAILED(aRange->SetEnd(startNode, startOffset)))
      {
        *aDir = eDirNext;
        aRange->SetStart(startNode, startOffset);
        aRange->SetEnd(endNode, endOffset);
      }
    }
  }
  return NS_OK;
}



NS_IMETHODIMP
nsRangeList::SetOriginalAnchorPoint(nsIDOMNode *aNode, PRInt32 aOffset)
{
  if (!aNode){
    mOriginalAnchorRange = 0;
    return NS_OK;
  }
  nsCOMPtr<nsIDOMRange> newRange;
  nsresult result;
  result = nsComponentManager::CreateInstance(kRangeCID, nsnull,
                                     nsIDOMRange::GetIID(),
                                     getter_AddRefs(newRange));
  result = newRange->SetStart(aNode,aOffset);
  if (NS_FAILED(result))
    return result;
  result = newRange->SetEnd(aNode,aOffset);
  if (NS_FAILED(result))
    return result;

  mOriginalAnchorRange = newRange;
  return result;
}



NS_IMETHODIMP
nsRangeList::GetOriginalAnchorPoint(nsIDOMNode **aNode, PRInt32 *aOffset)
{
  if (!aNode || !aOffset || !mOriginalAnchorRange)
    return NS_ERROR_NULL_POINTER;
  nsresult result;
  result = mOriginalAnchorRange->GetStartParent(aNode);
  if (NS_FAILED(result))
    return result;
  result = mOriginalAnchorRange->GetStartOffset(aOffset);
  return result;
}



/*
Notes which might come in handy for extend:

We can tell the direction of the selection by asking for the anchors selection
if the begin is less than the end then we know the selection is to the "right".
else it is a backwards selection.
a = anchor
1 = old cursor
2 = new cursor

  if (a <= 1 && 1 <=2)    a,1,2  or (a1,2)
  if (a < 2 && 1 > 2)     a,2,1
  if (1 < a && a <2)      1,a,2
  if (a > 2 && 2 >1)      1,2,a
  if (2 < a && a <1)      2,a,1
  if (a > 1 && 1 >2)      2,1,a
then execute
a  1  2 select from 1 to 2
a  2  1 deselect from 2 to 1
1  a  2 deselect from 1 to a select from a to 2
1  2  a deselect from 1 to 2
2  1  a = continue selection from 2 to 1
*/


/*
 * Extend extends the selection away from the anchor.
 * We don't need to know the direction, because we always change the focus.
 */
NS_IMETHODIMP
nsRangeList::Extend(nsIDOMNode* aParentNode, PRInt32 aOffset)
{
  // First, find the range containing the old focus point:
  if (!mRangeArray)
    return NS_ERROR_FAILURE;
  InvalidateDesiredX();
  nsCOMPtr<nsIDOMRange> difRange;
  nsresult res;
  res = nsComponentManager::CreateInstance(kRangeCID, nsnull,
                                     nsIDOMRange::GetIID(),
                                     getter_AddRefs(difRange));


  if (NS_FAILED(res)) 
    return res;
  nsCOMPtr<nsIDOMRange> range;
  res = mAnchorFocusRange->Clone(getter_AddRefs(range));

  nsCOMPtr<nsIDOMNode> startNode;
  nsCOMPtr<nsIDOMNode> endNode;
  PRInt32 startOffset;
  PRInt32 endOffset;

  range->GetStartParent(getter_AddRefs(startNode));
  range->GetEndParent(getter_AddRefs(endNode));
  range->GetStartOffset(&startOffset);
  range->GetEndOffset(&endOffset);


  nsDirection dir = GetDirection();
  PRBool fixupState; //if there was a previous fixup the optimal drawing erasing will NOT work
  if (NS_FAILED(res))
    return res;

  res = nsComponentManager::CreateInstance(kRangeCID, nsnull,
                                     nsIDOMRange::GetIID(),
                                     getter_AddRefs(difRange));

  if (NS_FAILED(res))
    return res;
  //compare anchor to old cursor.

  if (NS_FAILED(res))
    return res;
  PRInt32 result1 = ComparePoints(FetchAnchorNode(), FetchAnchorOffset() 
                                  ,FetchFocusNode(), FetchFocusOffset());
  //compare old cursor to new cursor
  PRInt32 result2 = ComparePoints(FetchFocusNode(), FetchFocusOffset(),
                            aParentNode, aOffset );
  //compare anchor to new cursor
  PRInt32 result3 = ComparePoints(FetchAnchorNode(), FetchAnchorOffset(),
                            aParentNode , aOffset );

  if ((result1 == 0 && result3 < 0) || (result1 <= 0 && result2 <= 0)){//a1,2  a,1,2
    //select from 1 to 2
    res = range->SetEnd(aParentNode,aOffset);
    if (NS_FAILED(res))
      return res;
    dir = eDirNext;
    res = FixupSelectionPoints(range, &dir, &fixupState);
    if (NS_FAILED(res))
      return res;
    if (fixupState) //unselect previous and select new state has changed to not fixed up
    {
      selectFrames(mAnchorFocusRange, PR_FALSE);
      selectFrames(range, PR_TRUE);
    }
    else{
      res = difRange->SetEnd(FetchEndParent(range), FetchEndOffset(range));
      res |= difRange->SetStart(FetchFocusNode(), FetchFocusOffset());
      if (NS_FAILED(res))
        return res;
      selectFrames(difRange , PR_TRUE);
    }
  }
  else if (result1 == 0 && result3 > 0){//2, a1
    //select from 2 to 1a
    dir = eDirPrevious;
    res = range->SetStart(aParentNode,aOffset);
    if (NS_FAILED(res))
      return res;
    res = FixupSelectionPoints(range, &dir, &fixupState);
    if (NS_FAILED(res))
      return res;
    if (fixupState) //unselect previous and select new state has changed to not fixed up
    {
      selectFrames(mAnchorFocusRange, PR_FALSE);
      selectFrames(range, PR_TRUE);
    }
    else
      selectFrames(range, PR_TRUE);
  }
  else if (result3 <= 0 && result2 >= 0) {//a,2,1 or a2,1 or a,21 or a21
    //deselect from 2 to 1
    res = difRange->SetEnd(FetchFocusNode(), FetchFocusOffset());
    res |= difRange->SetStart(aParentNode, aOffset);
    if (NS_FAILED(res))
      return res;

    dir = eDirNext;
    res = range->SetEnd(aParentNode,aOffset);
    if (NS_FAILED(res))
      return res;
    res = FixupSelectionPoints(range, &dir, &fixupState);
    if (NS_FAILED(res))
      return res;
    if (fixupState) //unselect previous and select new state has changed to not fixed up
    {
      selectFrames(mAnchorFocusRange, PR_FALSE);
      selectFrames(range, PR_TRUE);
    }
    else {
      selectFrames(difRange, 0);//deselect now if fixup succeeded
      difRange->SetEnd(FetchEndParent(range),FetchEndOffset(range));
      selectFrames(difRange, PR_TRUE);//must reselect last node maybe more if fixup did something
    }
  }
  else if (result1 >= 0 && result3 <= 0) {//1,a,2 or 1a,2 or 1,a2 or 1a2
    if (GetDirection() == eDirPrevious){
      res = range->SetStart(endNode,endOffset);
      if (NS_FAILED(res))
        return res;
    }
    dir = eDirNext;
    res = range->SetEnd(aParentNode,aOffset);
    if (NS_FAILED(res))
      return res;
    res = FixupSelectionPoints(range, &dir, &fixupState);
    if (NS_FAILED(res))
      return res;
    if (fixupState) //unselect previous and select new state has changed to not fixed up
    {
      selectFrames(mAnchorFocusRange, PR_FALSE);
      selectFrames(range, PR_TRUE);
    }
    else {
      if (FetchFocusNode() != FetchAnchorNode() || FetchFocusOffset() != FetchAnchorOffset() ){//if collapsed diff dont do anything
        res = difRange->SetStart(FetchFocusNode(), FetchFocusOffset());
        res |= difRange->SetEnd(FetchAnchorNode(), FetchAnchorOffset());
        if (NS_FAILED(res))
          return res;
        //deselect from 1 to a
        selectFrames(difRange , PR_FALSE);
      }
      //select from a to 2
      selectFrames(range , PR_TRUE);
    }
  }
  else if (result2 <= 0 && result3 >= 0) {//1,2,a or 12,a or 1,2a or 12a
    //deselect from 1 to 2
    res = difRange->SetEnd(aParentNode, aOffset);
    res |= difRange->SetStart(FetchFocusNode(), FetchFocusOffset());
    if (NS_FAILED(res))
      return res;
    dir = eDirPrevious;
    res = range->SetStart(aParentNode,aOffset);
    if (NS_FAILED(res))
      return res;

    res = FixupSelectionPoints(range, &dir, &fixupState);
    if (NS_FAILED(res))
      return res;
    if (fixupState) //unselect previous and select new state has changed to not fixed up
    {
      selectFrames(mAnchorFocusRange, PR_FALSE);
      selectFrames(range, PR_TRUE);
    }
    else {
      selectFrames(difRange , PR_FALSE);
      difRange->SetStart(FetchStartParent(range),FetchStartOffset(range));
      selectFrames(difRange, PR_TRUE);//must reselect last node
    }
  }
  else if (result3 >= 0 && result1 <= 0) {//2,a,1 or 2a,1 or 2,a1 or 2a1
    if (GetDirection() == eDirNext){
      range->SetEnd(startNode,startOffset);
    }
    dir = eDirPrevious;
    res = range->SetStart(aParentNode,aOffset);
    if (NS_FAILED(res))
      return res;
    res = FixupSelectionPoints(range, &dir, &fixupState);
    if (NS_FAILED(res))
      return res;
    if (fixupState) //unselect previous and select new state has changed to not fixed up
    {
      selectFrames(mAnchorFocusRange, PR_FALSE);
      selectFrames(range, PR_TRUE);
    }
    else
    {
      //deselect from a to 1
      if (FetchFocusNode() != FetchAnchorNode() || FetchFocusOffset() != FetchAnchorOffset() ){//if collapsed diff dont do anything
        res = difRange->SetStart(FetchAnchorNode(), FetchAnchorOffset());
        res |= difRange->SetEnd(FetchFocusNode(), FetchFocusOffset());
        selectFrames(difRange, 0);
      }
      //select from 2 to a
      selectFrames(range , PR_TRUE);
    }
  }
  else if (result2 >= 0 && result1 >= 0) {//2,1,a or 21,a or 2,1a or 21a
    //select from 2 to 1
    res = range->SetStart(aParentNode,aOffset);
    if (NS_FAILED(res))
      return res;
    dir = eDirPrevious;
    res = FixupSelectionPoints(range, &dir, &fixupState);
    if (NS_FAILED(res))
      return res;
    if (fixupState) //unselect previous and select new state has changed to not fixed up
    {
      selectFrames(mAnchorFocusRange, PR_FALSE);
      selectFrames(range, PR_TRUE);
    }
    else {
      res = difRange->SetEnd(FetchFocusNode(), FetchFocusOffset());
      res |= difRange->SetStart(FetchStartParent(range), FetchStartOffset(range));
      if (NS_FAILED(res))
        return res;
      selectFrames(difRange, PR_TRUE);
    }
  }

  DEBUG_OUT_RANGE(range);
#if 0
  if (eDirNext == mDirection)
    printf("    direction = 1  LEFT TO RIGHT\n");
  else
    printf("    direction = 0  RIGHT TO LEFT\n");
#endif
  SetDirection(dir);
  /*hack*/
  range->GetStartParent(getter_AddRefs(startNode));
  range->GetEndParent(getter_AddRefs(endNode));
  range->GetStartOffset(&startOffset);
  range->GetEndOffset(&endOffset);
  if (NS_FAILED(mAnchorFocusRange->SetStart(startNode,startOffset)))
  {
    if (NS_FAILED(mAnchorFocusRange->SetEnd(endNode,endOffset)))
      return NS_ERROR_FAILURE;//???
    if (NS_FAILED(mAnchorFocusRange->SetStart(startNode,startOffset)))
      return NS_ERROR_FAILURE;//???
  }
  else if (NS_FAILED(mAnchorFocusRange->SetEnd(endNode,endOffset)))
          return NS_ERROR_FAILURE;//???
  /*end hack*/
  ScrollIntoView();
  return NotifySelectionListeners();
}


/** DeleteFromDocument
 *  will return NS_OK if it handles the event or NS_COMFALSE if not.
 */
NS_IMETHODIMP
nsRangeList::DeleteFromDocument()
{
  nsresult res;

  // If we're already collapsed, then set ourselves to include the
  // last item BEFORE the current range, rather than the range itself,
  // before we do the delete.
  PRBool isCollapsed;
  GetIsCollapsed(&isCollapsed);
  if (isCollapsed)
  {
    // If the offset is positive, then it's easy:
    if (FetchFocusOffset() > 0)
    {
      Extend(FetchFocusNode(), FetchFocusOffset() - 1);
    }
    else
    {
      // Otherwise it's harder, have to find the previous node
      printf("Sorry, don't know how to delete across frame boundaries yet\n");
      return NS_ERROR_NOT_IMPLEMENTED;
    }
  }

  // Get an iterator
  nsRangeListIterator iter(this);
  res = iter.First();
  if (!NS_SUCCEEDED(res))
    return res;

  nsCOMPtr<nsIDOMRange> range;
  while (iter.IsDone())
  {
    res = iter.CurrentItem(getter_AddRefs(range));
    if (!NS_SUCCEEDED(res))
      return res;
    res = range->DeleteContents();
    if (!NS_SUCCEEDED(res))
      return res;
    iter.Next();
  }

  // Collapse to the new location.
  // If we deleted one character, then we move back one element.
  // FIXME  We don't know how to do this past frame boundaries yet.
  if (isCollapsed)
    Collapse(FetchAnchorNode(), FetchAnchorOffset()-1);
  else if (FetchAnchorOffset() > 0)
    Collapse(FetchAnchorNode(), FetchAnchorOffset());
#ifdef DEBUG
  else
    printf("Don't know how to set selection back past frame boundary\n");
#endif

  return NS_OK;
}


//END nsIDOMSelection interface implementations

#ifdef XP_MAC
#pragma mark -
#endif

// BEGIN nsIScriptContextOwner interface implementations
NS_IMETHODIMP
nsRangeList::GetScriptObject(nsIScriptContext *aContext, void** aScriptObject)
{
  nsresult res = NS_OK;
  nsIScriptGlobalObject *globalObj = aContext->GetGlobalObject();

  if (nsnull == mScriptObject) {
    res = NS_NewScriptSelection(aContext, (nsISupports *)(nsIDOMSelection *)this, globalObj, (void**)&mScriptObject);
  }
  *aScriptObject = mScriptObject;

  NS_RELEASE(globalObj);
  return res;
}

NS_IMETHODIMP
nsRangeList::SetScriptObject(void *aScriptObject)
{
  mScriptObject = aScriptObject;
  return NS_OK;
}

// END nsIScriptContextOwner interface implementations

