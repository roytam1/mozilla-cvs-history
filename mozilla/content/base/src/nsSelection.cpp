/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
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
 */

/*
 * Implementation of selection: nsIDOMSelection and nsIFrameSelection
 */

#include "nsCOMPtr.h"
#include "nsWeakReference.h"
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
#include "nsIDOMElement.h"
#include "nsIDOMNode.h"
#include "nsRange.h"
#include "nsISupportsArray.h"
#include "nsIDOMKeyEvent.h"
#include "nsITableLayout.h"
#include "nsITableCellLayout.h"

#include "nsIDOMSelectionListener.h"
#include "nsIContentIterator.h"
#include "nsIDocumentEncoder.h"
#include "nsIIndependentSelection.h"
#include "nsIPref.h"

#ifdef IBMBIDI
#include "nsFrameTraversal.h"
#include "nsILineIterator.h"
#include "nsLayoutAtoms.h"
#endif // IBMBIDI

#include "nsIDOMText.h"

//included for desired x position;
#include "nsIPresContext.h"
#include "nsIPresShell.h"
#include "nsICaret.h"

#include "nsIScriptObjectOwner.h"
#include "nsIScriptGlobalObject.h"

// included for view scrolling
#include "nsIViewManager.h"
#include "nsIScrollableView.h"
#include "nsIDeviceContext.h"
#include "nsITimer.h"
#include "nsITimerCallback.h"
#include "nsIServiceManager.h"
#include "nsIAutoCopy.h"

//nodtifications
#include "nsIDOMDocument.h"
#include "nsIDocument.h"

#include "nsISelectionController.h"//for the enums

#define STATUS_CHECK_RETURN_MACRO() {if (!mTracker) return NS_ERROR_FAILURE;}
//#define DEBUG_TABLE 1

//static NS_DEFINE_IID(kCContentIteratorCID, NS_CONTENTITERATOR_CID);
static NS_DEFINE_IID(kCGenContentIteratorCID, NS_GENERATEDCONTENTITERATOR_CID);
static NS_DEFINE_IID(kCGenSubtreeIteratorCID, NS_GENERATEDSUBTREEITERATOR_CID);
static NS_DEFINE_IID(kCSubtreeIteratorCID, NS_SUBTREEITERATOR_CID);

//PROTOTYPES
#if OLD_SELECTION
static nsCOMPtr<nsIAtom> GetTag(nsIDOMNode *aNode);
static nsresult ParentOffset(nsIDOMNode *aNode, nsIDOMNode **aParent, PRInt32 *aChildOffset);
#endif

#ifdef PRINT_RANGE
static void printRange(nsIDOMRange *aDomRange);
#define DEBUG_OUT_RANGE(x)  printRange(x)
#else
#define DEBUG_OUT_RANGE(x)  
#endif //MOZ_DEBUG


#define OLD_SELECTION 0

//#define DEBUG_SELECTION // uncomment for printf describing every collapse and extend.
//#define DEBUG_NAVIGATION


#if DEBUG_cmanske
//#define DEBUG_TABLE_SELECTION 1
#endif


class nsSelectionIterator;
class nsSelection;
class nsAutoScrollTimer;

class nsDOMSelection : public nsIDOMSelection , public nsIScriptObjectOwner, public nsSupportsWeakReference, public nsIIndependentSelection
{
public:
  nsDOMSelection();
  nsDOMSelection(nsSelection *aList);
  virtual ~nsDOMSelection();
  
  NS_DECL_ISUPPORTS
  /*BEGIN nsIIndependentSelection interface implementations */
  NS_IMETHOD    SetPresShell(nsIPresShell *aPresShell);

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
  NS_IMETHOD    CollapseToStart();
  NS_IMETHOD    CollapseToEnd();
  NS_IMETHOD    Extend(nsIDOMNode* aParentNode, PRInt32 aOffset);
  NS_IMETHOD    ContainsNode(nsIDOMNode* aNode, PRBool aRecursive, PRBool* aAYes);
  NS_IMETHOD    DeleteFromDocument();
  NS_IMETHOD    AddRange(nsIDOMRange* aRange);
  NS_IMETHOD    RemoveRange(nsIDOMRange* aRange);

  NS_IMETHOD    StartBatchChanges();
  NS_IMETHOD    EndBatchChanges();

  NS_IMETHOD    AddSelectionListener(nsIDOMSelectionListener* aNewListener);
  NS_IMETHOD    RemoveSelectionListener(nsIDOMSelectionListener* aListenerToRemove);
  NS_IMETHOD    GetEnumerator(nsIEnumerator **aIterator);

  NS_IMETHOD    ToString(const nsString& aFormatType, PRUint32 aFlags, PRInt32 aWrapCount, nsString& aReturn);

  NS_IMETHOD    SetHint(PRBool aHintRight);
  NS_IMETHOD    GetHint(PRBool *aHintRight);

/*END nsIDOMSelection interface implementations*/

/*BEGIN nsIScriptObjectOwner interface implementations*/
  NS_IMETHOD 		GetScriptObject(nsIScriptContext *aContext, void** aScriptObject);
  NS_IMETHOD 		SetScriptObject(void *aScriptObject);
/*END nsIScriptObjectOwner interface implementations*/

  // utility methods for scrolling the selection into view
  nsresult      GetPresContext(nsIPresContext **aPresContext);
  nsresult      GetPresShell(nsIPresShell **aPresShell);
  nsresult      GetRootScrollableView(nsIScrollableView **aScrollableView);
  nsresult      GetFrameToRootViewOffset(nsIFrame *aFrame, nscoord *aXOffset, nscoord *aYOffset);
  nsresult      GetPointFromOffset(nsIFrame *aFrame, PRInt32 aContentOffset, nsPoint *aPoint);
  nsresult      GetSelectionRegionRect(SelectionRegion aRegion, nsRect *aRect);
  nsresult      ScrollRectIntoView(nsRect& aRect, PRIntn  aVPercent, PRIntn  aHPercent);

  NS_IMETHOD    ScrollIntoView(SelectionRegion aRegion=nsISelectionController::SELECTION_FOCUS_REGION);
  nsresult      AddItem(nsIDOMRange *aRange);
  nsresult      RemoveItem(nsIDOMRange *aRange);

  nsresult      Clear(nsIPresContext* aPresContext);
	// methods for convenience. Note, these don't addref
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

  nsDirection  GetDirection(){return mDirection;}
  void         SetDirection(nsDirection aDir){mDirection = aDir;}
  NS_IMETHOD   CopyRangeToAnchorFocus(nsIDOMRange *aRange);
//  NS_IMETHOD   GetPrimaryFrameForRangeEndpoint(nsIDOMNode *aNode, PRInt32 aOffset, PRBool aIsEndNode, nsIFrame **aResultFrame);
  NS_IMETHOD   GetPrimaryFrameForAnchorNode(nsIFrame **aResultFrame);
  NS_IMETHOD   GetPrimaryFrameForFocusNode(nsIFrame **aResultFrame);
  NS_IMETHOD   SetOriginalAnchorPoint(nsIDOMNode *aNode, PRInt32 aOffset);
  NS_IMETHOD   GetOriginalAnchorPoint(nsIDOMNode **aNode, PRInt32 *aOffset);
  NS_IMETHOD   LookUpSelection(nsIContent *aContent, PRInt32 aContentOffset, PRInt32 aContentLength,
                             SelectionDetails **aReturnDetails, SelectionType aType, PRBool aSlowCheck);
  NS_IMETHOD   Repaint(nsIPresContext* aPresContext);

  nsresult     StartAutoScrollTimer(nsIPresContext *aPresContext, nsIFrame *aFrame, nsPoint& aPoint, PRUint32 aDelay);
  nsresult     StopAutoScrollTimer();
  nsresult     DoAutoScroll(nsIPresContext *aPresContext, nsIFrame *aFrame, nsPoint& aPoint);

  SelectionType GetType(){return mType;}
  void          SetType(SelectionType aType){mType = aType;}

  nsresult     NotifySelectionListeners();

private:
  friend class nsSelectionIterator;

  

  void         setAnchorFocusRange(PRInt32 aIndex); //pass in index into FrameSelection
  NS_IMETHOD   selectFrames(nsIPresContext* aPresContext, nsIContentIterator *aInnerIter, nsIContent *aContent, nsIDOMRange *aRange, nsIPresShell *aPresShell, PRBool aFlags);
  NS_IMETHOD   selectFrames(nsIPresContext* aPresContext, nsIDOMRange *aRange, PRBool aSelect);
  
#if OLD_SELECTION
  NS_IMETHOD   FixupSelectionPoints(nsIDOMRange *aRange, nsDirection *aDir, PRBool *aFixupState);
#endif //OLD_SELECTION

  nsCOMPtr<nsISupportsArray> mRangeArray;

  nsCOMPtr<nsIDOMRange> mAnchorFocusRange;
  nsCOMPtr<nsIDOMRange> mOriginalAnchorRange; //used as a point with range gravity for security
  nsDirection mDirection; //FALSE = focus, anchor;  TRUE = anchor,focus
  PRBool mFixupState; //was there a fixup?

  nsSelection *mFrameSelection;
  nsWeakPtr mPresShellWeak; //weak reference to presshell.
  // for nsIScriptObjectOwner
  void*		mScriptObject;
  SelectionType mType;//type of this nsDOMSelection;
  nsAutoScrollTimer *mAutoScrollTimer; // timer for autoscrolling.
  nsCOMPtr<nsISupportsArray> mSelectionListeners;
};

// Stack-class to turn on/off selection batching for table selection
class nsSelectionBatcher
{
private:
  nsCOMPtr<nsIDOMSelection> mSelection;
public:
  nsSelectionBatcher(nsIDOMSelection *aSelection) : mSelection(aSelection)
  {
    if (mSelection) mSelection->StartBatchChanges();
  }
  virtual ~nsSelectionBatcher() 
  { 
    if (mSelection) mSelection->EndBatchChanges();
  }
};

class nsSelection : public nsIFrameSelection
                    
{
public:
  /*interfaces for addref and release and queryinterface*/
  
  NS_DECL_ISUPPORTS

/*BEGIN nsIFrameSelection interfaces*/
  NS_IMETHOD Init(nsIFocusTracker *aTracker, nsIContent *aLimiter);
  NS_IMETHOD SetScrollableView(nsIScrollableView *aScrollView);

  NS_IMETHOD ShutDown();
  NS_IMETHOD HandleTextEvent(nsGUIEvent *aGUIEvent);
  NS_IMETHOD HandleKeyEvent(nsIPresContext* aPresContext, nsGUIEvent *aGuiEvent);
  NS_IMETHOD HandleClick(nsIContent *aNewFocus, PRUint32 aContentOffset, PRUint32 aContentEndOffset, 
                       PRBool aContinueSelection, PRBool aMultipleSelection,PRBool aHint);
  NS_IMETHOD HandleDrag(nsIPresContext *aPresContext, nsIFrame *aFrame, nsPoint& aPoint);
  NS_IMETHOD HandleTableSelection(nsIContent *aParentContent, PRInt32 aContentOffset, PRUint32 aTarget, nsMouseEvent *aMouseEvent);
  NS_IMETHOD StartAutoScrollTimer(nsIPresContext *aPresContext, nsIFrame *aFrame, nsPoint& aPoint, PRUint32 aDelay);
  NS_IMETHOD StopAutoScrollTimer();
  NS_IMETHOD EnableFrameNotification(PRBool aEnable){mNotifyFrames = aEnable; return NS_OK;}
  NS_IMETHOD LookUpSelection(nsIContent *aContent, PRInt32 aContentOffset, PRInt32 aContentLength,
                             SelectionDetails **aReturnDetails, PRBool aSlowCheck);
  NS_IMETHOD SetMouseDownState(PRBool aState);
  NS_IMETHOD GetMouseDownState(PRBool *aState);

  NS_IMETHOD GetTableCellSelection(PRBool *aState){if (aState){*aState = mSelectingTableCellMode != 0; return NS_OK;}return NS_ERROR_NULL_POINTER;}
  NS_IMETHOD ClearTableCellSelection(){mSelectingTableCellMode = 0; return NS_OK;}
  NS_IMETHOD GetTableCellSelectionStyleColor(const nsStyleColor **aStyleColor);

  NS_IMETHOD GetSelection(SelectionType aType, nsIDOMSelection **aDomSelection);
  NS_IMETHOD ScrollSelectionIntoView(SelectionType aType, SelectionRegion aRegion);
  NS_IMETHOD RepaintSelection(nsIPresContext* aPresContext, SelectionType aType);
  NS_IMETHOD GetFrameForNodeOffset(nsIContent *aNode, PRInt32 aOffset, HINT aHint, nsIFrame **aReturnFrame, PRInt32 *aReturnOffset);

  NS_IMETHOD AdjustOffsetsFromStyle(nsIFrame *aFrame, PRBool *changeSelection,
        nsIContent** outContent, PRInt32* outStartOffset, PRInt32* outEndOffset);
  
  NS_IMETHOD SetHint(HINT aHintRight);
  NS_IMETHOD GetHint(HINT *aHintRight);
  NS_IMETHOD CharacterMove(PRBool aForward, PRBool aExtend);
  NS_IMETHOD WordMove(PRBool aForward, PRBool aExtend);
  NS_IMETHOD LineMove(PRBool aForward, PRBool aExtend);
  NS_IMETHOD IntraLineMove(PRBool aForward, PRBool aExtend); 
  NS_IMETHOD SelectAll();
  NS_IMETHOD SetDisplaySelection(PRInt16 aState);
  NS_IMETHOD GetDisplaySelection(PRInt16 *aState);
  NS_IMETHOD SetDelayCaretOverExistingSelection(PRBool aDelay);
  NS_IMETHOD GetDelayCaretOverExistingSelection(PRBool *aDelay);
  NS_IMETHOD SetDelayedCaretData(nsMouseEvent *aMouseEvent);
  NS_IMETHOD GetDelayedCaretData(nsMouseEvent **aMouseEvent);
  NS_IMETHOD GetLimiter(nsIContent **aLimiterContent);
#ifdef IBMBIDI
  NS_IMETHOD GetPrevNextBidiLevels(nsIPresContext *aPresContext,
                                   nsIContent *aNode,
                                   PRUint32 aContentOffset,
                                   nsIFrame **aPrevFrame,
                                   nsIFrame **aNextFrame,
                                   PRUint8 *aPrevLevel,
                                   PRUint8 *aNextLevel);
#endif
  /*END nsIFrameSelection interfacse*/



  nsSelection();
  virtual ~nsSelection();

  NS_IMETHOD    StartBatchChanges();
  NS_IMETHOD    EndBatchChanges();
  NS_IMETHOD    DeleteFromDocument();

  nsIFocusTracker *GetTracker(){return mTracker;}
private:
  NS_IMETHOD TakeFocus(nsIContent *aNewFocus, PRUint32 aContentOffset, PRUint32 aContentEndOffset, 
                       PRBool aContinueSelection, PRBool aMultipleSelection);

#ifdef IBMBIDI
  void BidiLevelFromMove(nsIPresContext* aContext,
                         nsISelectionController* aSelCon,
                         nsIContent *aNode,
                         PRUint32 aContentOffset,
                         PRUint32 aKeycode);
  void BidiLevelFromClick(nsIContent *aNewFocus, PRUint32 aContentOffset);
#endif

//post and pop reasons for notifications. we may stack these later
  void  PostReason(short aReason){mReason = aReason;}
  short PopReason(){short retval = mReason; mReason=0;return retval;}

  friend class nsDOMSelection; 
#ifdef DEBUG
  void printSelection();       // for debugging
#endif /* DEBUG */

  void ResizeBuffer(PRUint32 aNewBufSize);
/*HELPER METHODS*/
  nsresult     MoveCaret(PRUint32 aKeycode, PRBool aContinue, nsSelectionAmount aAmount);

  nscoord      FetchDesiredX(); //the x position requested by the Key Handling for up down
  void         InvalidateDesiredX(); //do not listen to mDesiredX you must get another.
  void         SetDesiredX(nscoord aX); //set the mDesiredX

  nsresult     GetRootForContentSubtree(nsIContent *aContent, nsIContent **aParent);
  nsresult     GetGlobalViewOffsetsFromFrame(nsIPresContext *aPresContext, nsIFrame *aFrame, nscoord *offsetX, nscoord *offsetY);
  nsresult     ConstrainFrameAndPointToAnchorSubtree(nsIPresContext *aPresContext, nsIFrame *aFrame, nsPoint& aPoint, nsIFrame **aRetFrame, nsPoint& aRetPoint);

  PRUint32     GetBatching(){return mBatching;}
  PRBool       GetNotifyFrames(){return mNotifyFrames;}
  void         SetDirty(PRBool aDirty=PR_TRUE){if (mBatching) mChangesDuringBatching = aDirty;}

  nsresult     NotifySelectionListeners(SelectionType aType);			// add parameters to say collapsed etc?

  // utility method to lookup frame style
  nsresult      FrameOrParentHasSpecialSelectionStyle(nsIFrame* aFrame, PRUint8 aSelectionStyle, nsIFrame* *foundFrame);

  nsDOMSelection *mDomSelections[nsISelectionController::NUM_SELECTIONTYPES];
  nsIScrollableView *GetScrollView(){return mScrollView;}

  // Table selection support.
  // Interfaces that let us get info based on cellmap locations
  nsITableLayout* GetTableLayout(nsIContent *aTableContent);
  nsITableCellLayout* GetCellLayout(nsIContent *aCellContent);
  nsresult SelectBlockOfCells(nsIContent *aEndNode);
  nsresult SelectRowOrColumn(nsIContent *aCellContent, PRUint32 aTarget);
  nsresult GetCellIndexes(nsIContent *aCell, PRInt32 &aRowIndex, PRInt32 &aColIndex);

  nsresult GetFirstSelectedCellAndRange(nsIDOMNode **aCell, nsIDOMRange **aRange);
  nsresult GetNextSelectedCellAndRange(nsIDOMNode **aCell, nsIDOMRange **aRange);
  nsresult GetFirstCellNodeInRange(nsIDOMRange *aRange, nsIDOMNode **aCellNode);
  // aTableNode may be null if table isn't needed to be returned
  PRBool   IsInSameTable(nsIContent *aContent1, nsIContent *aContent2, nsIContent **aTableNode);
  nsresult GetParentTable(nsIContent *aCellNode, nsIContent **aTableNode);
  nsresult SelectCellElement(nsIDOMElement* aCellElement);
  nsresult CreateAndAddRange(nsIDOMNode *aParentNode, PRInt32 aOffset);
  nsresult ClearNormalSelection();

  nsCOMPtr<nsIContent> mStartSelectedCell;
  nsCOMPtr<nsIContent> mEndSelectedCell;
  nsCOMPtr<nsIContent> mAppendStartSelectedCell;
  nsCOMPtr<nsIContent> mUnselectCellOnMouseUp;
  PRBool   mSelectingTableCells;
  PRUint32 mSelectingTableCellMode;
  PRInt32  mSelectedCellIndex;

  //batching
  PRInt32 mBatching;
  PRBool mChangesDuringBatching;
  PRBool mNotifyFrames;
    
  nsIContent *mLimiter;     //limit selection navigation to a child of this node.
  nsIFocusTracker *mTracker;
  PRBool mMouseDownState;   //for drag purposes
  PRInt16 mDisplaySelection; //for visual display purposes.
  PRInt32 mDesiredX;
  PRBool mDesiredXSet;
  nsIScrollableView *mScrollView;

  PRBool mDelayCaretOverExistingSelection;
  PRBool mDelayedMouseEventValid;
  nsMouseEvent mDelayedMouseEvent;
  short mReason; //reason for notifications of selection changing
public:
  static nsIAtom *sTableAtom;
  static nsIAtom *sCellAtom;
  static nsIAtom *sTbodyAtom;
  static PRInt32 sInstanceCount;
  static nsStyleColor sTableStyleColor;
};

class nsSelectionIterator : public nsIBidirectionalEnumerator
{
public:
/*BEGIN nsIEnumerator interfaces
see the nsIEnumerator for more details*/

  NS_DECL_ISUPPORTS

  NS_DECL_NSIENUMERATOR

  NS_DECL_NSIBIDIRECTIONALENUMERATOR

/*END nsIEnumerator interfaces*/
/*BEGIN Helper Methods*/
  NS_IMETHOD CurrentItem(nsIDOMRange **aRange);
/*END Helper Methods*/
private:
  friend class nsDOMSelection;

  //lame lame lame if delete from document goes away then get rid of this unless its debug
  friend class nsSelection; 

  nsSelectionIterator(nsDOMSelection *);
  virtual ~nsSelectionIterator();
  PRInt32     mIndex;
  nsDOMSelection *mDomSelection;
  SelectionType mType;
};

class nsAutoScrollTimer : public nsITimerCallback
{
public:

  NS_DECL_ISUPPORTS

  nsAutoScrollTimer()
      : mSelection(0), mFrame(0), mPresContext(0), mPoint(0,0), mScrollPoint(0,0), mDelay(30)
  {
    NS_INIT_ISUPPORTS();
  }

  virtual ~nsAutoScrollTimer()
  {
    if (mTimer)
    {
      mTimer->Cancel();
    }
  }

  nsresult Start(nsIPresContext *aPresContext, nsIFrame *aFrame, nsPoint &aPoint)
  {
    mFrame       = aFrame;
    mPresContext = aPresContext;
    mPoint       = aPoint;

    if (!mTimer)
    {
      nsresult result;
      mTimer = do_CreateInstance("component://netscape/timer", &result);

      if (NS_FAILED(result))
        return result;
    }

    return mTimer->Init(this, mDelay);
  }

  nsresult Stop()
  {
    nsresult result = NS_OK;

    if (mTimer)
    {
      mTimer->Cancel();
      mTimer = 0;
    }

    return result;
  }

  nsresult Init(nsSelection *aFrameSelection, nsDOMSelection *aSelection)
  {
    mFrameSelection = aFrameSelection;
    mSelection = aSelection;
    return NS_OK;
  }

  nsresult SetDelay(PRUint32 aDelay)
  {
    mDelay = aDelay;
    return NS_OK;
  }

  NS_IMETHOD_(void) Notify(nsITimer *timer)
  {
    if (mSelection && mPresContext && mFrame)
    {
      //the frame passed in here will be a root frame for the view. there is no need to call the constrain
      //method here. the root frame has NO content now unfortunately...
      PRInt32 startPos = 0;
      PRInt32 contentOffsetEnd = 0;
      PRBool  beginOfContent;
      nsCOMPtr<nsIContent> newContent;
      nsresult result = mFrame->GetContentAndOffsetsFromPoint(mPresContext, mPoint,
                                                   getter_AddRefs(newContent), 
                                                   startPos, contentOffsetEnd,beginOfContent);

      if (NS_SUCCEEDED(result))
      {
        result = mFrameSelection->HandleClick(newContent, startPos, contentOffsetEnd , PR_TRUE, PR_FALSE, beginOfContent);
      }
      //mFrameSelection->HandleDrag(mPresContext, mFrame, mPoint);
      mSelection->DoAutoScroll(mPresContext, mFrame, mPoint);
    }
  }
private:
  nsSelection    *mFrameSelection;
  nsDOMSelection *mSelection;
  nsCOMPtr<nsITimer> mTimer;
  nsIFrame       *mFrame;
  nsIPresContext *mPresContext;
  nsPoint         mPoint;
  nsPoint         mScrollPoint;
  PRUint32        mDelay;
};

NS_IMPL_ADDREF(nsAutoScrollTimer)
NS_IMPL_RELEASE(nsAutoScrollTimer)
NS_IMPL_QUERY_INTERFACE1(nsAutoScrollTimer, nsITimerCallback)

nsresult NS_NewAutoScrollTimer(nsAutoScrollTimer **aResult)
{
  if (!aResult)
    return NS_ERROR_NULL_POINTER;

  *aResult = (nsAutoScrollTimer*) new nsAutoScrollTimer;

  if (!aResult)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(*aResult);

  return NS_OK;
}

nsresult NS_NewSelection(nsIFrameSelection **aFrameSelection);

nsresult NS_NewSelection(nsIFrameSelection **aFrameSelection)
{
  nsSelection *rlist = new nsSelection;
  if (!rlist)
    return NS_ERROR_OUT_OF_MEMORY;
  *aFrameSelection = (nsIFrameSelection *)rlist;
  rlist->AddRef();
  return NS_OK;
}

nsresult NS_NewDomSelection(nsIDOMSelection **aDomSelection);

nsresult NS_NewDomSelection(nsIDOMSelection **aDomSelection)
{
  nsDOMSelection *rlist = new nsDOMSelection;
  if (!rlist)
    return NS_ERROR_OUT_OF_MEMORY;
  *aDomSelection = (nsIDOMSelection *)rlist;
  rlist->AddRef();
  return NS_OK;
}

//Horrible statics but no choice
nsIAtom *nsSelection::sTableAtom = 0;
nsIAtom *nsSelection::sCellAtom = 0;
nsIAtom *nsSelection::sTbodyAtom = 0;
PRInt32 nsSelection::sInstanceCount = 0;
nsStyleColor nsSelection::sTableStyleColor;

static PRInt8
GetIndexFromSelectionType(SelectionType aType)
{
    switch (aType)
    {
    case nsISelectionController::SELECTION_NORMAL: return 0; break;
    case nsISelectionController::SELECTION_SPELLCHECK: return 1; break;
    case nsISelectionController::SELECTION_IME_RAWINPUT: return 2; break;
    case nsISelectionController::SELECTION_IME_SELECTEDRAWTEXT: return 3; break;
    case nsISelectionController::SELECTION_IME_CONVERTEDTEXT: return 4; break;
    case nsISelectionController::SELECTION_IME_SELECTEDCONVERTEDTEXT: return 5; break;
    default:return -1;break;
    }
}

static SelectionType 
GetSelectionTypeFromIndex(PRInt8 aIndex)
{
  switch (aIndex)
  {
    case 0: return nsISelectionController::SELECTION_NORMAL;break;
    case 1: return nsISelectionController::SELECTION_SPELLCHECK;break;
    case 2: return nsISelectionController::SELECTION_IME_RAWINPUT;break;
    case 3: return nsISelectionController::SELECTION_IME_SELECTEDRAWTEXT;break;
    case 4: return nsISelectionController::SELECTION_IME_CONVERTEDTEXT;break;
    case 5: return nsISelectionController::SELECTION_IME_SELECTEDCONVERTEDTEXT;break;
    default:
      return nsISelectionController::SELECTION_NORMAL;break;
  }
}


///////////BEGIN nsSelectionIterator methods

nsSelectionIterator::nsSelectionIterator(nsDOMSelection *aList)
:mIndex(0)
{
  NS_INIT_REFCNT();
  if (!aList)
  {
    NS_NOTREACHED("nsSelection");
    return;
  }
  mDomSelection = aList;
}



nsSelectionIterator::~nsSelectionIterator()
{
}



////////////END nsSelectionIterator methods

////////////BEGIN nsIFrameSelectionIterator methods



NS_IMETHODIMP
nsSelectionIterator::Next()
{
  mIndex++;
  PRUint32 cnt;
  nsresult rv = mDomSelection->mRangeArray->Count(&cnt);
  if (NS_FAILED(rv)) return rv;
  if (mIndex < (PRInt32)cnt)
    return NS_OK;
  return NS_ERROR_FAILURE;
}



NS_IMETHODIMP
nsSelectionIterator::Prev()
{
  mIndex--;
  if (mIndex >= 0 )
    return NS_OK;
  return NS_ERROR_FAILURE;
}



NS_IMETHODIMP
nsSelectionIterator::First()
{
  if (!mDomSelection)
    return NS_ERROR_NULL_POINTER;
  mIndex = 0;
  return NS_OK;
}



NS_IMETHODIMP
nsSelectionIterator::Last()
{
  if (!mDomSelection)
    return NS_ERROR_NULL_POINTER;
  PRUint32 cnt;
  nsresult rv = mDomSelection->mRangeArray->Count(&cnt);
  if (NS_FAILED(rv)) return rv;
  mIndex = (PRInt32)cnt-1;
  return NS_OK;
}



NS_IMETHODIMP 
nsSelectionIterator::CurrentItem(nsISupports **aItem)
{
  if (!aItem)
    return NS_ERROR_NULL_POINTER;
  PRUint32 cnt;
  nsresult rv = mDomSelection->mRangeArray->Count(&cnt);
  if (NS_FAILED(rv)) return rv;
  if (mIndex >=0 && mIndex < (PRInt32)cnt){
    *aItem = mDomSelection->mRangeArray->ElementAt(mIndex);
    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}

NS_IMPL_ADDREF(nsSelectionIterator)

NS_IMPL_RELEASE(nsSelectionIterator)

NS_IMETHODIMP 
nsSelectionIterator::CurrentItem(nsIDOMRange **aItem)
{
  if (!aItem)
    return NS_ERROR_NULL_POINTER;
  PRUint32 cnt;
  nsresult rv = mDomSelection->mRangeArray->Count(&cnt);
  if (NS_FAILED(rv)) return rv;
  if (mIndex >=0 && mIndex < (PRInt32)cnt){
    nsCOMPtr<nsISupports> indexIsupports = dont_AddRef(mDomSelection->mRangeArray->ElementAt(mIndex));
    return indexIsupports->QueryInterface(NS_GET_IID(nsIDOMRange),(void **)aItem);
  }
  return NS_ERROR_FAILURE;
}



NS_IMETHODIMP
nsSelectionIterator::IsDone()
{
  PRUint32 cnt;
  nsresult rv = mDomSelection->mRangeArray->Count(&cnt);
  if (NS_FAILED(rv)) return rv;
  if (mIndex >= 0 && mIndex < (PRInt32)cnt ) { 
    return NS_ENUMERATOR_FALSE;
  }
  return NS_OK;
}



NS_IMETHODIMP
nsSelectionIterator::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
  if (nsnull == aInstancePtr) {
    return NS_ERROR_NULL_POINTER;
  }
  if (aIID.Equals(NS_GET_IID(nsIEnumerator))) {
    *aInstancePtr = NS_STATIC_CAST(nsIEnumerator*, this);
    NS_ADDREF_THIS();
    return NS_OK;
  }
  if (aIID.Equals(NS_GET_IID(nsIBidirectionalEnumerator))) {
    *aInstancePtr = NS_STATIC_CAST(nsIBidirectionalEnumerator*, this);
    NS_ADDREF_THIS();
    return NS_OK;
  }
  return mDomSelection->QueryInterface(aIID, aInstancePtr);
}






////////////END nsIFrameSelectionIterator methods

#ifdef XP_MAC
#pragma mark -
#endif

////////////BEGIN nsSelection methods

nsSelection::nsSelection()
{
  NS_INIT_REFCNT();
  PRInt32 i;
  for (i = 0;i<nsISelectionController::NUM_SELECTIONTYPES;i++){
    mDomSelections[i] = nsnull;
  }
  for (i = 0;i<nsISelectionController::NUM_SELECTIONTYPES;i++){
    mDomSelections[i] = new nsDOMSelection(this);
    if (!mDomSelections[i])
      return;
    mDomSelections[i]->AddRef();
    mDomSelections[i]->SetType(GetSelectionTypeFromIndex(i));
  }
  mBatching = 0;
  mChangesDuringBatching = PR_FALSE;
  mNotifyFrames = PR_TRUE;
  mLimiter = nsnull; //no default limiter.
    
  if (sInstanceCount <= 0)
  {
    sTableAtom = NS_NewAtom("table");
    sCellAtom = NS_NewAtom("td");
    sTbodyAtom = NS_NewAtom("tbody");
    sTableStyleColor.mColor =  NS_RGB(128,0,0);
    sTableStyleColor.mBackgroundColor =  NS_RGB(128,0,0);
    sTableStyleColor.mOpacity=  (float)1;
    sTableStyleColor.mBackgroundAttachment=0;
    sTableStyleColor.mBackgroundFlags=NS_STYLE_BG_IMAGE_NONE;
    sTableStyleColor.mBackgroundRepeat=1;
    sTableStyleColor.mBackgroundXPosition=0;
    sTableStyleColor.mBackgroundYPosition=0;
    sTableStyleColor.mCursor=1;
  }
  mHint = HINTLEFT;
  sInstanceCount ++;
  mSelectingTableCells = PR_FALSE;
  mSelectingTableCellMode = 0;

  mSelectedCellIndex = 0;

  // Check to see if the autocopy pref is enabled
  //   and add the autocopy listener if it is
  nsresult rv;
	NS_WITH_SERVICE(nsIPref, prefs, "component://netscape/preferences", &rv);
	if (NS_SUCCEEDED(rv) && prefs)
  {
    static char pref[] = "clipboard.autocopy";
	  PRBool autoCopy = PR_FALSE;
    if (NS_SUCCEEDED(prefs->GetBoolPref(pref, &autoCopy)) && autoCopy)
    {
      NS_WITH_SERVICE(nsIAutoCopyService, autoCopyService, "component://netscape/autocopy", &rv);

      if (NS_SUCCEEDED(rv) && autoCopyService)
      {
        PRInt8 index = GetIndexFromSelectionType(nsISelectionController::SELECTION_NORMAL);
        if (mDomSelections[index])
          autoCopyService->Listen(mDomSelections[index]);
      }
    }
  }

  mDisplaySelection = nsISelectionController::SELECTION_OFF;

  mDelayCaretOverExistingSelection = PR_TRUE;
  mDelayedMouseEventValid = PR_FALSE;
  mReason = nsIDOMSelectionListener::NO_REASON;
}



nsSelection::~nsSelection() 
{
  if (sInstanceCount <= 1)
  {
    NS_IF_RELEASE(sTableAtom);
    NS_IF_RELEASE(sCellAtom);
    NS_IF_RELEASE(sTbodyAtom);
  }
  PRInt32 i;
  for (i = 0;i<nsISelectionController::NUM_SELECTIONTYPES;i++){
    if (mDomSelections[i])
        NS_IF_RELEASE(mDomSelections[i]);
  }
  sInstanceCount--;
}


NS_IMPL_ADDREF(nsSelection)

NS_IMPL_RELEASE(nsSelection)


NS_IMETHODIMP
nsSelection::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
  if (nsnull == aInstancePtr) {
    return NS_ERROR_NULL_POINTER;
  }
  if (aIID.Equals(NS_GET_IID(nsIFrameSelection))) {
    nsIFrameSelection* tmp = this;
    *aInstancePtr = (void*) tmp;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  if (aIID.Equals(NS_GET_IID(nsISupports))) {
    // use *first* base class for ISupports
    nsIFrameSelection* tmp1 = this;
    nsISupports* tmp2 = tmp1;
    *aInstancePtr = (void*) tmp2;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  NS_ASSERTION(PR_FALSE,"bad query interface in FrameSelection");
  return NS_NOINTERFACE;
}






nscoord
nsSelection::FetchDesiredX() //the x position requested by the Key Handling for up down
{
  if (!mTracker)
  {
    NS_ASSERTION(0,"fetch desired X failed\n");
    return -1;
  }
  if (mDesiredXSet)
    return mDesiredX;
  else {
    nsRect coord;
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

    PRInt8 index = GetIndexFromSelectionType(nsISelectionController::SELECTION_NORMAL);
    result = caret->GetWindowRelativeCoordinates(coord,collapsed,mDomSelections[index]);
    if (NS_FAILED(result))
      return result;
    return coord.x;
  }
}



void
nsSelection::InvalidateDesiredX() //do not listen to mDesiredX you must get another.
{
  mDesiredXSet = PR_FALSE;
}



void
nsSelection::SetDesiredX(nscoord aX) //set the mDesiredX
{
  mDesiredX = aX;
  mDesiredXSet = PR_TRUE;
}

nsresult
nsSelection::GetRootForContentSubtree(nsIContent *aContent, nsIContent **aParent)
{
  // This method returns the root of the sub-tree containing aContent.
  // We do this by searching up through the parent hierarchy, and stopping
  // when there are no more parents, or we hit a situation where the
  // parent/child relationship becomes invalid.
  //
  // An example of an invalid parent/child relationship is anonymous content.
  // Anonymous content has a pointer to it's parent, but it is not listed
  // as a child of it's parent. In this case, the anonymous content would
  // be considered the root of the subtree.

  nsresult result = NS_OK;

  if (!aContent || !aParent)
    return NS_ERROR_NULL_POINTER;

  *aParent = 0;

  nsCOMPtr<nsIContent> parent = do_QueryInterface(aContent);
  nsCOMPtr<nsIContent> child = parent;

  while (child)
  {
    result = child->GetParent(*getter_AddRefs(parent));

    if (NS_FAILED(result))
      return result;

    if (!parent)
      break;

    PRInt32 childIndex = 0;
    PRInt32 childCount = 0;

    result = parent->ChildCount(childCount);

    if (NS_FAILED(result))
      return result;

    if (childCount < 1)
      break;

    result = parent->IndexOf(child, childIndex);

    if (NS_FAILED(result))
      return result;

    if (childIndex < 0 || childIndex >= childCount)
      break;

    child = parent;
  }

  *aParent = child;

  NS_IF_ADDREF(*aParent);

  return result;
}

nsresult
nsSelection::GetGlobalViewOffsetsFromFrame(nsIPresContext *aPresContext, nsIFrame *aFrame, nscoord *offsetX, nscoord *offsetY)
{
  //
  // The idea here is to figure out what the offset of aFrame's view
  // is within the global space. Where I define the global space to
  // be the coordinate system that exists above all views.
  //
  // The offsets are calculated by walking up the view parent hierarchy,
  // adding up all the view positions, until there are no more views.
  //
  // A point in a view's coordinate space can be converted to the global
  // coordinate space by simply adding the offsets returned by this method
  // to the point itself.
  //

  if (!aPresContext || !aFrame || !offsetX || !offsetY)
    return NS_ERROR_NULL_POINTER;

  *offsetX = *offsetY = 0;

  nsresult result;
  nsIFrame *frame = aFrame;
  nsIView  *view;

  while (frame)
  {
    result = frame->GetParentWithView(aPresContext, &frame);

    if (NS_FAILED(result))
      return result;

    if (frame) {
      view   = 0;

      result = frame->GetView(aPresContext, &view);

      if (NS_FAILED(result))
        return result;

      if (view)
      {
        nscoord vX = 0, vY = 0;

        result = view->GetPosition(&vX, &vY);

        if (NS_FAILED(result))
          return result;

        *offsetX += vX;
        *offsetY += vY;
      }
    }

  }

  return NS_OK;
}

nsresult
nsSelection::ConstrainFrameAndPointToAnchorSubtree(nsIPresContext *aPresContext, nsIFrame *aFrame, nsPoint& aPoint, nsIFrame **aRetFrame, nsPoint& aRetPoint)
{
  //
  // The whole point of this method is to return a frame and point that
  // that lie within the same valid subtree as the anchor node's frame,
  // for use with the method GetContentAndOffsetsFromPoint().
  //
  // A valid subtree is defined to be one where all the content nodes in
  // the tree have a valid parent-child relationship.
  //
  // If the anchor frame and aFrame are in the same subtree, aFrame will
  // be returned in aRetFrame. If they are in different subtrees, we
  // return the frame for the root of the subtree.
  //

  if (!aFrame || !aRetFrame)
    return NS_ERROR_NULL_POINTER;

  *aRetFrame = aFrame;
  aRetPoint  = aPoint;

  //
  // Get the frame and content for the selection's anchor point!
  //

  nsresult result;
  nsCOMPtr<nsIDOMNode> anchorNode;
  PRInt32 anchorOffset = 0;
  PRInt32 anchorFrameOffset = 0;

  PRInt8 index = GetIndexFromSelectionType(nsISelectionController::SELECTION_NORMAL);
  if (! mDomSelections[index])
    return NS_ERROR_NULL_POINTER;

  result = mDomSelections[index]->GetAnchorNode(getter_AddRefs(anchorNode));

  if (NS_FAILED(result))
    return result;

  if (!anchorNode)
    return NS_OK;

  result = mDomSelections[index]->GetAnchorOffset(&anchorOffset);

  if (NS_FAILED(result))
    return result;

  nsIFrame *anchorFrame = 0;
  nsCOMPtr<nsIContent> anchorContent = do_QueryInterface(anchorNode);

  if (!anchorContent)
    return NS_ERROR_FAILURE;
  
  result = GetFrameForNodeOffset(anchorContent, anchorOffset, mHint, &anchorFrame, &anchorFrameOffset);

  //
  // Now find the root of the subtree containing the anchor's content.
  //

  nsCOMPtr<nsIContent> anchorRoot;
  result = GetRootForContentSubtree(anchorContent, getter_AddRefs(anchorRoot));

  if (NS_FAILED(result))
    return result;

  //
  // Now find the root of the subtree containing aFrame's content.
  //

  nsCOMPtr<nsIContent> content;

  result = aFrame->GetContent(getter_AddRefs(content));

  if (NS_FAILED(result))
    return result;

  if (content)
  {
    nsCOMPtr<nsIContent> contentRoot;

    result = GetRootForContentSubtree(content, getter_AddRefs(contentRoot));

    if (anchorRoot == contentRoot)
    {
      //
      // The anchor and AFrame's root are the same. There
      // is no need to constrain, simply return aFrame.
      //
      *aRetFrame = aFrame;
      return NS_OK;
    }
  }

  //
  // aFrame's root does not match the anchor's root, or there is no
  // content associated with aFrame. Just return the primary frame
  // for the anchor's root. We'll let GetContentAndOffsetsFromPoint()
  // find the closest frame aPoint.
  //

  result = mTracker->GetPrimaryFrameFor(anchorRoot, aRetFrame);

  if (NS_FAILED(result))
    return result;

  if (! *aRetFrame)
    return NS_ERROR_FAILURE;

  //
  // Now make sure that aRetPoint is converted to the same coordinate
  // system used by aRetFrame.
  //

  nsPoint frameOffset;
  nsPoint retFrameOffset;

  result = GetGlobalViewOffsetsFromFrame(aPresContext, aFrame, &frameOffset.x, &frameOffset.y);

  if (NS_FAILED(result))
    return result;

  result = GetGlobalViewOffsetsFromFrame(aPresContext, *aRetFrame, &retFrameOffset.x, &retFrameOffset.y);

  if (NS_FAILED(result))
    return result;

  aRetPoint = aPoint + frameOffset - retFrameOffset;

  return NS_OK;
}

#ifdef XP_MAC
#pragma mark -
#endif

#ifdef PRINT_RANGE
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
#endif /* PRINT_RANGE */

#if OLD_SELECTION
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
  result = aNode->QueryInterface(NS_GET_IID(nsIContent),getter_AddRefs(content));
  if (NS_SUCCEEDED(result) && content)
  {
    nsCOMPtr<nsIContent> parent;
    result = content->GetParent(*getter_AddRefs(parent));
    if (NS_SUCCEEDED(result) && parent)
    {
      result = parent->IndexOf(content, *aChildOffset);
      if (NS_SUCCEEDED(result))
        result = parent->QueryInterface(NS_GET_IID(nsIDOMNode),(void **)aParent);
    }
  }
  return result;
}
#endif



NS_IMETHODIMP
nsSelection::Init(nsIFocusTracker *aTracker, nsIContent *aLimiter)
{
  mTracker = aTracker;
  mMouseDownState = PR_FALSE;
  mDesiredXSet = PR_FALSE;
  mLimiter = aLimiter;
  mScrollView = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
nsSelection::SetScrollableView(nsIScrollableView *aScrollView)
{
  mScrollView = aScrollView;
  return NS_OK;
}


NS_IMETHODIMP
nsSelection::ShutDown()
{
  return NS_OK;
}

  
  
NS_IMETHODIMP
nsSelection::HandleTextEvent(nsGUIEvent *aGUIEvent)
{
	if (!aGUIEvent)
		return NS_ERROR_NULL_POINTER;

#ifdef DEBUG_TAGUE
	printf("nsSelection: HandleTextEvent\n");
#endif
  nsresult result(NS_OK);
	if (NS_TEXT_EVENT == aGUIEvent->message) {
    PRInt8 index = GetIndexFromSelectionType(nsISelectionController::SELECTION_NORMAL);
    result = mDomSelections[index]->ScrollIntoView();
	}
	return result;
}


nsresult
nsSelection::MoveCaret(PRUint32 aKeycode, PRBool aContinue, nsSelectionAmount aAmount)
{
  nsCOMPtr<nsIPresContext> context;
  nsresult result = mTracker->GetPresContext(getter_AddRefs(context));
  if (NS_FAILED(result) || !context)
    return result?result:NS_ERROR_FAILURE;

  nsCOMPtr<nsIDOMNode> weakNodeUsed;
  PRInt32 offsetused = 0;

  PRBool isCollapsed;
  nscoord desiredX = 0; //we must keep this around and revalidate it when its just UP/DOWN

  PRInt8 index = GetIndexFromSelectionType(nsISelectionController::SELECTION_NORMAL);
  result = mDomSelections[index]->GetIsCollapsed(&isCollapsed);
  if (NS_FAILED(result))
    return result;
  if (aKeycode == nsIDOMKeyEvent::DOM_VK_UP || aKeycode == nsIDOMKeyEvent::DOM_VK_DOWN)
  {
    desiredX= FetchDesiredX();
    SetDesiredX(desiredX);
  }

  if (!isCollapsed && !aContinue) {
    switch (aKeycode){
      case nsIDOMKeyEvent::DOM_VK_LEFT  : 
      case nsIDOMKeyEvent::DOM_VK_UP    : {
          if ((mDomSelections[index]->GetDirection() == eDirPrevious)) { //f,a
            offsetused = mDomSelections[index]->FetchFocusOffset();
            weakNodeUsed = mDomSelections[index]->FetchFocusNode();
          }
          else {
            offsetused = mDomSelections[index]->FetchAnchorOffset();
            weakNodeUsed = mDomSelections[index]->FetchAnchorNode();
          }
          result = mDomSelections[index]->Collapse(weakNodeUsed,offsetused);
          mDomSelections[index]->ScrollIntoView();
          return NS_OK;
         } break;
      case nsIDOMKeyEvent::DOM_VK_RIGHT : 
      case nsIDOMKeyEvent::DOM_VK_DOWN  : {
          if ((mDomSelections[index]->GetDirection() == eDirPrevious)) { //f,a
            offsetused = mDomSelections[index]->FetchAnchorOffset();
            weakNodeUsed = mDomSelections[index]->FetchAnchorNode();
          }
          else {
            offsetused = mDomSelections[index]->FetchFocusOffset();
            weakNodeUsed = mDomSelections[index]->FetchFocusNode();
          }
          result = mDomSelections[index]->Collapse(weakNodeUsed,offsetused);
          mDomSelections[index]->ScrollIntoView();
          return NS_OK;
         } break;
      
    }
//      if (keyEvent->keyCode == nsIDOMKeyEvent::DOM_VK_UP || keyEvent->keyCode == nsIDOMKeyEvent::DOM_VK_DOWN)
//        SetDesiredX(desiredX);
  }

#ifdef IBMBIDI
  nsCOMPtr<nsICaret> caret;
  nsCOMPtr<nsIPresShell> shell;
  result = context->GetShell(getter_AddRefs(shell));
  if (NS_FAILED(result) || !shell)
    return 0;
  result = shell->GetCaret(getter_AddRefs(caret));
  if (NS_FAILED(result) || !caret)
    return 0;
#endif

  offsetused = mDomSelections[index]->FetchFocusOffset();
  weakNodeUsed = mDomSelections[index]->FetchFocusNode();

    
  nsIFrame *frame;
  result = mDomSelections[index]->GetPrimaryFrameForFocusNode(&frame);

  if (NS_FAILED(result) || !frame)
    return result?result:NS_ERROR_FAILURE;
  nsCOMPtr<nsIContent> content;
  result = frame->GetContent(getter_AddRefs(content));
  nsCOMPtr<nsIDOMNode> node = do_QueryInterface(content);
  nsCOMPtr<nsIDOMNode> parentNode;
  //we also need to check to see if the result frame's content's parent is equal to the weaknode used of course.
  //except for special case of text frames where they are their own parent.
  nsCOMPtr<nsIDOMText> textNode = do_QueryInterface(node);
  if (NS_SUCCEEDED(result) && textNode && node != weakNodeUsed)//then the offset is meaningless.
  {
    offsetused = 0;//0 because when grabbing a child content we grab the IDX'th object or: body has 2 children, 
                     //index 0 of parent is the first child so if we say the first child is the frame then say offset is 0 we are correct
  }
  else
  {
    result = node->GetParentNode(getter_AddRefs(parentNode));
    if ((NS_FAILED(result) || parentNode != weakNodeUsed) && node != weakNodeUsed) //we are not pointing to same node! offset is meaningless
      offsetused = -1;
  }  
  nsPeekOffsetStruct pos;
  pos.SetData(mTracker, desiredX, aAmount, eDirPrevious, offsetused, PR_FALSE,PR_TRUE, PR_TRUE);
  switch (aKeycode){
    case nsIDOMKeyEvent::DOM_VK_RIGHT : 
        InvalidateDesiredX();
        pos.mDirection = eDirNext;
        mHint = HINTLEFT;//stick to this line
      break;
    case nsIDOMKeyEvent::DOM_VK_LEFT  : //no break
        InvalidateDesiredX();
        mHint = HINTRIGHT;//stick to opposite of movement
      break;
    case nsIDOMKeyEvent::DOM_VK_DOWN : 
        pos.mAmount = eSelectLine;
        pos.mDirection = eDirNext;//no break here
      break;
    case nsIDOMKeyEvent::DOM_VK_UP : 
        pos.mAmount = eSelectLine;
      break;
    case nsIDOMKeyEvent::DOM_VK_HOME :
        InvalidateDesiredX();
        pos.mAmount = eSelectBeginLine;
        InvalidateDesiredX();
        mHint = HINTRIGHT;//stick to opposite of movement
#ifdef IBMBIDI
				caret->VKHomePress(1);  // Mamdouh To identify home key press for layer change;
#endif
      break;
    case nsIDOMKeyEvent::DOM_VK_END :
        InvalidateDesiredX();
        pos.mAmount = eSelectEndLine;
        InvalidateDesiredX();
        mHint = HINTLEFT;//stick to this line
     break;
  default :return NS_ERROR_FAILURE;
  }
#ifdef IBMBIDI
  // Mamdouh : Flage for VK key
  caret->AccessVirtualKey(1);

  // Simon
  nsCOMPtr<nsISelectionController> selCon;
  selCon = do_QueryInterface(shell);
#endif 
  pos.mPreferLeft = mHint;
  if (NS_SUCCEEDED(result) && NS_SUCCEEDED(result = frame->PeekOffset(context, &pos)) && pos.mResultContent)
  {
    mHint = (HINT)pos.mPreferLeft;
#ifdef IBMBIDI
    nsIFrame *theFrame;
    PRInt32 currentOffset, frameStart, frameEnd;
    PRUint8 level;

    // XXX - I expected to be able to use pos.mResultFrame, but when we move from frame to frame
    //       and |PeekOffset| is called recursively, pos.mResultFrame on exit is sometimes set to the original
    //       frame, not the frame that we ended up in, so I need this call to |GetFrameForNodeOffset|.
    //       I don't know if that could or should be changed or if it would break something else.
    GetFrameForNodeOffset(pos.mResultContent, pos.mContentOffset, mHint, &theFrame, &currentOffset);
    theFrame->GetOffsets(frameStart, frameEnd);

    // the hint might have been reversed by an RTL frame, so make sure of it
    if (nsIDOMKeyEvent::DOM_VK_HOME == aKeycode)
      pos.mPreferLeft = PR_TRUE;
    else if (nsIDOMKeyEvent::DOM_VK_END == aKeycode)
      pos.mPreferLeft = PR_FALSE;
    mHint = (HINT)pos.mPreferLeft;
    switch (aKeycode) {
      case nsIDOMKeyEvent::DOM_VK_HOME:
      case nsIDOMKeyEvent::DOM_VK_END:

        // force the offset to the logical beginning (for HOME) or end (for END) of the frame
        // (if it is an RTL frame it will be at the visual beginning or end, which we don't want in this case)
          if (nsIDOMKeyEvent::DOM_VK_HOME == aKeycode)
        pos.mContentOffset = frameStart;
          else
            pos.mContentOffset = frameEnd;

        // set the cursor Bidi level to the paragraph embedding level
          theFrame->GetBidiProperty(context, nsLayoutAtoms::baseLevel, (void**) &level);
          selCon->SetCursorBidiLevel(level);
          break;

      default:
        // If the current position is not a frame boundary, it's enough just to take the Bidi level of the current frame
        if (pos.mContentOffset != frameStart && pos.mContentOffset != frameEnd) {
          theFrame->GetBidiProperty(context, nsLayoutAtoms::embeddingLevel, (void**) &level);
          selCon->SetCursorBidiLevel(level);
        }
        else
          BidiLevelFromMove(context, selCon, pos.mResultContent, pos.mContentOffset, aKeycode);
    }
#endif
    result = TakeFocus(pos.mResultContent, pos.mContentOffset, pos.mContentOffset, aContinue, PR_FALSE);
  }
  else if (NS_FAILED(result))
  {
    if (nsIDOMKeyEvent::DOM_VK_UP == aKeycode)
    {
      pos.mPreferLeft = HINTRIGHT;
      pos.mAmount = eSelectBeginLine;
      InvalidateDesiredX();
    }
    else if(nsIDOMKeyEvent::DOM_VK_DOWN == aKeycode)
    {
      pos.mPreferLeft = HINTLEFT;
      pos.mAmount = eSelectEndLine;
      InvalidateDesiredX();
    }
    if (nsIDOMKeyEvent::DOM_VK_UP == aKeycode || nsIDOMKeyEvent::DOM_VK_DOWN == aKeycode )
    {
      if (NS_SUCCEEDED(result = frame->PeekOffset(context, &pos)) && pos.mResultContent)
      {
#ifdef IBMBIDI
        BidiLevelFromMove(context, selCon, pos.mResultContent, pos.mContentOffset, aKeycode);
#endif // IBMBIDI
        mHint = (HINT)pos.mPreferLeft;
        PostReason(nsIDOMSelectionListener::MOUSEUP_REASON);//force an update as though we used the mouse.
        result = TakeFocus(pos.mResultContent, pos.mContentOffset, pos.mContentOffset, aContinue, PR_FALSE);
      }
    }
  }
  if (NS_SUCCEEDED(result))
    result = mDomSelections[index]->ScrollIntoView();

  return result;
}



/** This raises a question, if this method is called and the aFrame does not reflect the current
 *  focus  DomNode, it is invalid?  The answer now is yes.
 */
NS_IMETHODIMP
nsSelection::HandleKeyEvent(nsIPresContext* aPresContext, nsGUIEvent *aGuiEvent)
{
  if (!aGuiEvent)
    return NS_ERROR_NULL_POINTER;
  STATUS_CHECK_RETURN_MACRO();

  nsresult result = NS_ERROR_FAILURE;
  if (NS_KEY_PRESS == aGuiEvent->message) {
    nsKeyEvent *keyEvent = (nsKeyEvent *)aGuiEvent; //this is ok. It really is a keyevent
    switch (keyEvent->keyCode)
    {
        case nsIDOMKeyEvent::DOM_VK_LEFT  : 
        case nsIDOMKeyEvent::DOM_VK_UP    :
        case nsIDOMKeyEvent::DOM_VK_DOWN  : 
        case nsIDOMKeyEvent::DOM_VK_RIGHT    :
        case nsIDOMKeyEvent::DOM_VK_HOME  : 
        case nsIDOMKeyEvent::DOM_VK_END    :
          break;
        default:
           return NS_ERROR_FAILURE;
    }

//XXX Need xp way get platfrom specific behavior into key navigation.
//XXX This really shouldn't have to use an ifdef
#ifdef _WIN32
    if (keyEvent->isAlt) {
      return NS_ERROR_FAILURE;
    }
#endif
    nsSelectionAmount amount = eSelectCharacter;
    if (keyEvent->isControl)
      amount = eSelectWord;
    return MoveCaret(keyEvent->keyCode, keyEvent->isShift, amount);
  }
  return result;
}

//END nsSelection methods


//BEGIN nsIFrameSelection methods

NS_IMETHODIMP
nsDOMSelection::ToString(const nsString& aFormatType, PRUint32 aFlags, PRInt32 aWrapCount, nsString& aReturn)
{
  nsresult rv = NS_OK;

  nsCOMPtr<nsIDocumentEncoder> encoder;
  nsCAutoString formatType = NS_DOC_ENCODER_PROGID_BASE;
  formatType.AppendWithConversion(aFormatType);
  rv = nsComponentManager::CreateInstance(formatType,
                                          nsnull,
                                          NS_GET_IID(nsIDocumentEncoder),
                                          getter_AddRefs(encoder));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIPresShell> shell;
  rv = GetPresShell(getter_AddRefs(shell));
  if (NS_FAILED(rv) || !shell) {
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIDocument> doc;
  rv = shell->GetDocument(getter_AddRefs(doc));
  NS_ENSURE_SUCCESS(rv, rv);

  // Flags should always include OutputSelectionOnly if we're coming from here:
  aFlags |= nsIDocumentEncoder::OutputSelectionOnly;

  rv = encoder->Init(doc, aFormatType, aFlags);
  NS_ENSURE_SUCCESS(rv, rv);

  encoder->SetSelection(this);
  if (aWrapCount != 0)
    encoder->SetWrapColumn(aWrapCount);

  rv = encoder->EncodeToString(aReturn);

  return rv;
}

NS_IMETHODIMP
nsDOMSelection::SetHint(PRBool aHintRight)
{
  nsIFrameSelection::HINT hint;
  if (aHintRight)
    hint = nsIFrameSelection::HINTRIGHT;
  else
    hint = nsIFrameSelection::HINTLEFT;
  return mFrameSelection->SetHint(hint);
}

NS_IMETHODIMP
nsDOMSelection::GetHint(PRBool *aHintRight)
{
  nsIFrameSelection::HINT hint;
  nsresult rv = mFrameSelection->GetHint(&hint);
  if (hint == nsIFrameSelection::HINTRIGHT)
    *aHintRight = PR_TRUE;
  else
    *aHintRight = PR_FALSE;
  return rv;
}

#ifdef IBMBIDI
NS_IMETHODIMP
   nsSelection::GetPrevNextBidiLevels(nsIPresContext *aPresContext,
                                      nsIContent *aNode,
                                      PRUint32 aContentOffset,
                                      nsIFrame **aPrevFrame,
                                      nsIFrame **aNextFrame,
                                      PRUint8 *aPrevLevel,
                                      PRUint8 *aNextLevel)
{
  if (!aPrevFrame || !aNextFrame)
    return NS_ERROR_NULL_POINTER;
  // Get the level of the frames on each side
  nsIFrame    *currentFrame;
  PRInt32     currentOffset;
  PRInt32     frameStart, frameEnd;
  nsDirection direction;

  GetFrameForNodeOffset(aNode, aContentOffset, mHint, &currentFrame, &currentOffset);
  currentFrame->GetOffsets(frameStart, frameEnd);

  if (frameStart == currentOffset)
    direction = eDirPrevious;
  else if (frameEnd == currentOffset)
    direction = eDirNext;
  else {
    // we are neither at the beginning nor at the end of the frame, so we have no worries
    *aPrevFrame = *aNextFrame = currentFrame;
    currentFrame->GetBidiProperty(aPresContext, nsLayoutAtoms::embeddingLevel, (void**) aNextLevel);
    *aPrevLevel = *aNextLevel;
    return NS_OK;
  }

  /*
  we have to find the next or previous *logical* frame.

  Unfortunately |GetFrameFromDirection| has already been munged to return the next/previous *visual* frame, so we can't use that.
  The following code is taken from there without the Bidi changes.

  XXX is there a simpler way to do this? 
  */

  nsIFrame *blockFrame = currentFrame;
  nsIFrame *thisBlock;
  PRInt32   thisLine;
  nsCOMPtr<nsILineIteratorNavigator> it; 
  nsresult result = NS_ERROR_FAILURE;
  while (NS_FAILED(result) && blockFrame)
  {
    thisBlock = blockFrame;
    result = blockFrame->GetParent(&blockFrame);
    if (NS_SUCCEEDED(result) && blockFrame){
      result = blockFrame->QueryInterface(NS_GET_IID(nsILineIteratorNavigator),getter_AddRefs(it));
    }
    else
      blockFrame = nsnull;
  }
  if (!blockFrame || !it)
    return NS_ERROR_FAILURE;
  result = it->FindLineContaining(thisBlock, &thisLine);
  if (NS_FAILED(result))
    return result;

  nsIFrame *firstFrame;
  nsIFrame *lastFrame;
  nsRect    nonUsedRect;
  PRInt32   lineFrameCount;
  PRUint32  lineFlags;

  result = it->GetLine(thisLine, &firstFrame, &lineFrameCount,nonUsedRect,
                       &lineFlags);
  if (NS_FAILED(result))
    return result;

  lastFrame = firstFrame;

  for (;lineFrameCount > 1;lineFrameCount --) {
    result = lastFrame->GetNextSibling(&lastFrame);

    if (NS_FAILED(result)){
      NS_ASSERTION(0,"should not be reached nsFrame\n");
      return NS_ERROR_FAILURE;
    }
  }

  // GetFirstLeaf
  nsIFrame *lookahead = nsnull;
  while (1) {
    result = firstFrame->FirstChild(aPresContext, nsnull, &lookahead);
    if (NS_FAILED(result) || !lookahead)
      break; //nothing to do
    firstFrame = lookahead;
  }

  // GetLastLeaf
  lookahead = nsnull;
  while (1) {
    result = lastFrame->FirstChild(aPresContext, nsnull, &lookahead);
    if (NS_FAILED(result) || !lookahead)
      break; //nothing to do
    lastFrame = lookahead;
    while (NS_SUCCEEDED(lastFrame->GetNextSibling(&lookahead)) && lookahead)
      lastFrame = lookahead;
  }
  //END LINE DATA CODE

  if (direction == eDirNext && lastFrame == currentFrame) { // End of line: set aPrevFrame to the current frame
                                                            //              set aPrevLevel to the embedding level of the current frame
                                                            //              set aNextFrame to null
                                                            //              set aNextLevel to the paragraph embedding level
    *aPrevFrame = currentFrame;
    currentFrame->GetBidiProperty(aPresContext, nsLayoutAtoms::embeddingLevel, (void**) aPrevLevel);
    currentFrame->GetBidiProperty(aPresContext, nsLayoutAtoms::baseLevel, (void**) aNextLevel);
    *aNextFrame = nsnull;
    return NS_OK;
  }

  if (direction == eDirPrevious && firstFrame == currentFrame) { // Beginning of line: set aPrevFrame to null
                                                                 //                    set aPrevLevel to the paragraph embedding level
                                                                 //                    set aNextFrame to the current frame
                                                                 //                    set aNextLevel to the embedding level of the current frame
    *aNextFrame = currentFrame;
    currentFrame->GetBidiProperty(aPresContext, nsLayoutAtoms::embeddingLevel, (void**) aNextLevel);
    currentFrame->GetBidiProperty(aPresContext, nsLayoutAtoms::baseLevel, (void**) aPrevLevel);
    *aPrevFrame = nsnull;
    return NS_OK;
  }

  // Find the adjacent frame

  nsCOMPtr<nsIBidirectionalEnumerator> frameTraversal;
  result = NS_NewFrameTraversal(getter_AddRefs(frameTraversal),LEAF, aPresContext, currentFrame);
  if (NS_FAILED(result))
    return result;
  nsISupports *isupports = nsnull;
  if (direction == eDirNext)
    result = frameTraversal->Next();
  else 
    result = frameTraversal->Prev();

  if (NS_FAILED(result))
    return result;
  result = frameTraversal->CurrentItem(&isupports);
  if (NS_FAILED(result))
    return result;
  if (!isupports)
    return NS_ERROR_NULL_POINTER;
  //we must CAST here to an nsIFrame. nsIFrame doesnt really follow the rules
  //for speed reasons
  nsIFrame *newFrame = (nsIFrame *)isupports;

  if (direction == eDirNext) {
    *aPrevFrame = currentFrame;
    currentFrame->GetBidiProperty(aPresContext, nsLayoutAtoms::embeddingLevel, (void**) aPrevLevel);
    *aNextFrame = newFrame;
    newFrame->GetBidiProperty(aPresContext, nsLayoutAtoms::embeddingLevel, (void**) aNextLevel);
  }
  else {
    *aNextFrame = currentFrame;
    currentFrame->GetBidiProperty(aPresContext, nsLayoutAtoms::embeddingLevel, (void**) aNextLevel);
    *aPrevFrame = newFrame;
    newFrame->GetBidiProperty(aPresContext, nsLayoutAtoms::embeddingLevel, (void**) aPrevLevel);
  }

  return NS_OK;

}

/** After moving the caret, its Bidi level is set according to the following rules:
 *
 *  After moving over a character with left/right arrow, set to the Bidi level of the last moved over character.
 *  After Home and End, set to the paragraph embedding level.
 *  After up/down arrow, PageUp/Down, set to the lower level of the 2 surrounding characters.
 *  After mouse click, set to the level of the current frame.
 *
 *  The following two methods use GetPrevNextBidiLevels to determine the new Bidi level.
 */

void nsSelection::BidiLevelFromMove(nsIPresContext* aContext,
                                    nsISelectionController* aSelCon,
                                    nsIContent *aNode,
                                    PRUint32 aContentOffset,
                                    PRUint32 aKeycode)
{
  PRUint8 firstLevel;
  PRUint8 secondLevel;
  PRUint8 currentLevel;
  nsIFrame* firstFrame=nsnull;
  nsIFrame* secondFrame=nsnull;

  aSelCon->GetCursorBidiLevel(&currentLevel);

  GetPrevNextBidiLevels(aContext, aNode, aContentOffset, &firstFrame, &secondFrame, &firstLevel, &secondLevel);

  switch (aKeycode) {

    // Right and Left: the new cursor Bidi level is the level of the character moved over
    case nsIDOMKeyEvent::DOM_VK_RIGHT:
        if (currentLevel & 1)
      aSelCon->SetCursorBidiLevel(secondLevel);
        else
          aSelCon->SetCursorBidiLevel(firstLevel);
        break;

    case nsIDOMKeyEvent::DOM_VK_LEFT:
        if (currentLevel & 1)
      aSelCon->SetCursorBidiLevel(firstLevel);
        else
          aSelCon->SetCursorBidiLevel(secondLevel);
        break;

    // Up and Down: the new cursor Bidi level is the smaller of the two surrounding characters
    case nsIDOMKeyEvent::DOM_VK_UP:
    case nsIDOMKeyEvent::DOM_VK_DOWN:
        aSelCon->SetCursorBidiLevel(PR_MIN(firstLevel, secondLevel));
    break;

    default:
      aSelCon->UndefineCursorBidiLevel();
  }
}

void nsSelection::BidiLevelFromClick(nsIContent *aNode, PRUint32 aContentOffset)
{
  nsCOMPtr<nsIPresContext> context;
  nsresult result = mTracker->GetPresContext(getter_AddRefs(context));
  if (NS_FAILED(result) || !context)
    return;

  nsCOMPtr<nsIPresShell> shell;
  result = context->GetShell(getter_AddRefs(shell));
  if (NS_FAILED(result) || !shell)
    return;

  nsCOMPtr<nsISelectionController> selCon;
  selCon = do_QueryInterface(shell);
  if (!selCon)
    return;

  nsIFrame* clickInFrame=nsnull;
  PRUint8 frameLevel;
  PRInt32 OffsetNotUsed;

  GetFrameForNodeOffset(aNode, aContentOffset, mHint, &clickInFrame, &OffsetNotUsed);
  clickInFrame->GetBidiProperty(context, nsLayoutAtoms::embeddingLevel, (void**) &frameLevel);
  selCon->SetCursorBidiLevel(frameLevel);
}
#endif IBMBIDI

NS_IMETHODIMP
nsSelection::HandleClick(nsIContent *aNewFocus, PRUint32 aContentOffset, 
                       PRUint32 aContentEndOffset, PRBool aContinueSelection, 
                       PRBool aMultipleSelection, PRBool aHint) 
{
  if (!aNewFocus)
    return NS_ERROR_INVALID_ARG;
  InvalidateDesiredX();
  
  mHint = HINT(aHint);
  // Don't take focus when dragging off of a table
  if (!mSelectingTableCells)
  {
#ifdef IBMBIDI
    BidiLevelFromClick(aNewFocus, aContentOffset);
#endif
    PostReason(nsIDOMSelectionListener::MOUSEDOWN_REASON + nsIDOMSelectionListener::DRAG_REASON);
    return TakeFocus(aNewFocus, aContentOffset, aContentEndOffset, aContinueSelection, aMultipleSelection);
  }
  
  return NS_OK;
}

NS_IMETHODIMP
nsSelection::HandleDrag(nsIPresContext *aPresContext, nsIFrame *aFrame, nsPoint& aPoint)
{
  if (!aPresContext || !aFrame)
    return NS_ERROR_NULL_POINTER;

  nsresult result;
  nsIFrame *newFrame = 0;
  nsPoint   newPoint;

  result = ConstrainFrameAndPointToAnchorSubtree(aPresContext, aFrame, aPoint, &newFrame, newPoint);

  if (NS_FAILED(result))
    return result;

  if (!newFrame)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIPresShell> presShell;

  result = aPresContext->GetShell(getter_AddRefs(presShell));

  if (NS_FAILED(result))
    return result;

  PRInt32 startPos = 0;
  PRInt32 contentOffsetEnd = 0;
  PRBool  beginOfContent;
  nsCOMPtr<nsIContent> newContent;

  result = newFrame->GetContentAndOffsetsFromPoint(aPresContext, newPoint,
                                                   getter_AddRefs(newContent), 
                                                   startPos, contentOffsetEnd, beginOfContent);

  // do we have CSS that changes selection behaviour?
  {
    PRBool    changeSelection;
    nsCOMPtr<nsIContent>  selectContent;
    PRInt32   newStart, newEnd;
    if (NS_SUCCEEDED(AdjustOffsetsFromStyle(newFrame, &changeSelection, getter_AddRefs(selectContent), &newStart, &newEnd))
      && changeSelection)
    {
      newContent = selectContent;
      startPos = newStart;
      contentOffsetEnd = newEnd;
    }
  }

  if (NS_SUCCEEDED(result))
  {
    result = HandleClick(newContent, startPos, contentOffsetEnd , PR_TRUE, PR_FALSE, beginOfContent);
  }

  return result;
}

NS_IMETHODIMP
nsSelection::StartAutoScrollTimer(nsIPresContext *aPresContext, nsIFrame *aFrame, nsPoint& aPoint, PRUint32 aDelay)
{
  PRInt8 index = GetIndexFromSelectionType(nsISelectionController::SELECTION_NORMAL);
  return mDomSelections[index]->StartAutoScrollTimer(aPresContext, aFrame, aPoint, aDelay);
}

NS_IMETHODIMP
nsSelection::StopAutoScrollTimer()
{
  PRInt8 index = GetIndexFromSelectionType(nsISelectionController::SELECTION_NORMAL);
  return mDomSelections[index]->StopAutoScrollTimer();
}

/**
hard to go from nodes to frames, easy the other way!
 */
NS_IMETHODIMP
nsSelection::TakeFocus(nsIContent *aNewFocus, PRUint32 aContentOffset, 
                       PRUint32 aContentEndOffset, PRBool aContinueSelection, PRBool aMultipleSelection)
{
  if (!aNewFocus)
    return NS_ERROR_NULL_POINTER;

  STATUS_CHECK_RETURN_MACRO();

  if (mLimiter )
  {
    nsCOMPtr<nsIContent> parent;
    nsresult rv = aNewFocus->GetParent(*getter_AddRefs(parent));
    if (NS_FAILED(rv))
      return rv;
    if (mLimiter != parent.get() && mLimiter != aNewFocus) //if newfocus == the limiter. thats ok.
      return NS_ERROR_FAILURE; //not in the right content. mLimiter said so
  }

  //HACKHACKHACK
  nsCOMPtr<nsIContent> content;
  nsCOMPtr<nsIDOMNode> domNode;
  nsCOMPtr<nsIContent> parent;
  nsCOMPtr<nsIContent> parent2;
  if (NS_FAILED(aNewFocus->GetParent(*getter_AddRefs(parent))) || !parent)
    return NS_ERROR_FAILURE;
  //END HACKHACKHACK /checking for root frames/content

  PRInt8 index = GetIndexFromSelectionType(nsISelectionController::SELECTION_NORMAL);
  domNode = do_QueryInterface(aNewFocus);
  //traverse through document and unselect crap here
  if (!aContinueSelection){ //single click? setting cursor down
    PRUint32 batching = mBatching;//hack to use the collapse code.
    PRBool changes = mChangesDuringBatching;
    mBatching = 1;

    if (aMultipleSelection){
      nsCOMPtr<nsIDOMRange> newRange;
      NS_NewRange(getter_AddRefs(newRange));

      newRange->SetStart(domNode,aContentOffset);
      newRange->SetEnd(domNode,aContentOffset);
      mDomSelections[index]->AddRange(newRange);
      mBatching = batching;
      mChangesDuringBatching = changes;
      mDomSelections[index]->SetOriginalAnchorPoint(domNode,aContentOffset);
    }
    else
    {
      mDomSelections[index]->Collapse(domNode, aContentOffset);
      mBatching = batching;
      mChangesDuringBatching = changes;
    }
    if (aContentEndOffset > aContentOffset)
      mDomSelections[index]->Extend(domNode,aContentEndOffset);
  }
  else {
    // Now update the range list:
    if (aContinueSelection && domNode)
    {
      if (mDomSelections[index]->GetDirection() == eDirNext && aContentEndOffset > aContentOffset) //didnt go far enough 
      {
        mDomSelections[index]->Extend(domNode, aContentEndOffset);//this will only redraw the diff 
      }
      else
        mDomSelections[index]->Extend(domNode, aContentOffset);
    }
  }

  // Don't notify selection listeners if batching is on:
  if (GetBatching())
    return NS_OK;
  return NotifySelectionListeners(nsISelectionController::SELECTION_NORMAL);
}



NS_METHOD
nsSelection::LookUpSelection(nsIContent *aContent, PRInt32 aContentOffset, PRInt32 aContentLength,
                             SelectionDetails **aReturnDetails, PRBool aSlowCheck)
{
  if (!aContent || !aReturnDetails)
    return NS_ERROR_NULL_POINTER;

  STATUS_CHECK_RETURN_MACRO();

  // Never use "slow check" when in table selection mode
  if (aSlowCheck)
    aSlowCheck = (mSelectingTableCellMode == 0);

  *aReturnDetails = nsnull;
  PRInt8 j;
  for (j = (PRInt8) 0; j < (PRInt8)nsISelectionController::NUM_SELECTIONTYPES; j++){
    if (mDomSelections[j])
     mDomSelections[j]->LookUpSelection(aContent, aContentOffset, aContentLength, aReturnDetails, (SelectionType)(1<<j), aSlowCheck);
  }
  return NS_OK;
}



NS_METHOD 
nsSelection::SetMouseDownState(PRBool aState)
{
  if (mMouseDownState == aState)
    return NS_OK;
  mMouseDownState = aState;
  if (!mMouseDownState)
  {
    // Mouse up kills dragging-table cell selection
#ifdef DEBUG_TABLE_SELECTION
printf("SetMouseDownState to FALSE - stopping cell selection\n");
#endif
    mSelectingTableCells = PR_FALSE;
    mStartSelectedCell = nsnull;
    mEndSelectedCell = nsnull;
    PostReason(aState?nsIDOMSelectionListener::MOUSEDOWN_REASON:nsIDOMSelectionListener::MOUSEUP_REASON);//not a drag reason
    NotifySelectionListeners(nsISelectionController::SELECTION_NORMAL);//notify that reason is mouse up please.
  }
  return NS_OK;
}



NS_METHOD
nsSelection::GetMouseDownState(PRBool *aState)
{
  if (!aState)
    return NS_ERROR_NULL_POINTER;
  *aState = mMouseDownState;
  return NS_OK;
}



NS_IMETHODIMP
nsSelection::GetTableCellSelectionStyleColor(const nsStyleColor **aStyleColor)
{
  if (!aStyleColor)
    return NS_ERROR_NULL_POINTER;
  *aStyleColor = &sTableStyleColor;
  return NS_OK;
}



NS_IMETHODIMP
nsSelection::GetSelection(SelectionType aType, nsIDOMSelection **aDomSelection)
{
  if (!aDomSelection)
    return NS_ERROR_NULL_POINTER;
  PRInt8 index = GetIndexFromSelectionType(aType);
  if (index < 0)
    return NS_ERROR_INVALID_ARG;
  *aDomSelection = mDomSelections[index];
  (*aDomSelection)->AddRef();
  return NS_OK;
}

NS_IMETHODIMP
nsSelection::ScrollSelectionIntoView(SelectionType aType, SelectionRegion aRegion)
{
  PRInt8 index = GetIndexFromSelectionType(aType);
  if (index < 0)
    return NS_ERROR_INVALID_ARG;

  if (!mDomSelections[index])
    return NS_ERROR_NULL_POINTER;

  return mDomSelections[index]->ScrollIntoView(aRegion);
}

NS_IMETHODIMP
nsSelection::RepaintSelection(nsIPresContext* aPresContext, SelectionType aType)
{
  PRInt8 index = GetIndexFromSelectionType(aType);
  if (index < 0)
    return NS_ERROR_INVALID_ARG;
  if (!mDomSelections[index])
    return NS_ERROR_NULL_POINTER;
  return mDomSelections[index]->Repaint(aPresContext);
}
 
NS_IMETHODIMP
nsSelection::GetFrameForNodeOffset(nsIContent *aNode, PRInt32 aOffset, HINT aHint, nsIFrame **aReturnFrame, PRInt32 *aReturnOffset)
{
  if (!aNode || !aReturnFrame || !aReturnOffset)
    return NS_ERROR_NULL_POINTER;

  if (aOffset < 0)
    return NS_ERROR_FAILURE;

  *aReturnOffset = aOffset;

  nsresult result;
  PRBool canContainChildren = PR_FALSE;

  result = aNode->CanContainChildren(canContainChildren);

  if (NS_FAILED(result))
    return result;

  nsCOMPtr<nsIContent> theNode = aNode;

  if (canContainChildren)
  {
    PRInt32 childIndex  = 0;
    PRInt32 numChildren = 0;

    if (aHint == HINTLEFT)
    {
      if (aOffset > 0)
        childIndex = aOffset - 1;
      else
        childIndex = aOffset;
    }
    else // HINTRIGHT
    {
      result = theNode->ChildCount(numChildren);

      if (NS_FAILED(result))
        return result;

      if (aOffset >= numChildren)
      {
        if (numChildren > 0)
          childIndex = numChildren - 1;
        else
          childIndex = 0;
      }
      else
        childIndex = aOffset;
    }
    
    nsCOMPtr<nsIContent> childNode;

    result = theNode->ChildAt(childIndex, *getter_AddRefs(childNode));

    if (NS_FAILED(result))
      return result;

    if (!childNode)
      return NS_ERROR_FAILURE;

    theNode = childNode;

#ifdef DONT_DO_THIS_YET
    // XXX: We can't use this code yet because the hinting
    //      can cause us to attatch to the wrong line frame.

    // Now that we have the child node, check if it too
    // can contain children. If so, call this method again!

    result = theNode->CanContainChildren(canContainChildren);

    if (NS_FAILED(result))
      return result;

    if (canContainChildren)
    {
      PRInt32 newOffset = 0;

      if (aOffset > childIndex)
      {
        result = theNode->ChildCount(numChildren);

        if (NS_FAILED(result))
          return result;

        newOffset = numChildren;
      }

      return GetFrameForNodeOffset(theNode, newOffset, aHint, aReturnFrame,aReturnOffset);
    }
    else
#endif // DONT_DO_THIS_YET
    {
      // Check to see if theNode is a text node. If it is, translate
      // aOffset into an offset into the text node.

      nsCOMPtr<nsIDOMText> textNode = do_QueryInterface(theNode);

      if (textNode)
      {
        if (aOffset > childIndex)
        {
          PRUint32 textLength = 0;

          result = textNode->GetLength(&textLength);

          if (NS_FAILED(result))
            return NS_ERROR_FAILURE;

          *aReturnOffset = (PRInt32)textLength;
        }
        else
          *aReturnOffset = 0;
      }
    }
  }
  
  result = mTracker->GetPrimaryFrameFor(theNode, aReturnFrame);
  if (NS_FAILED(result))
    return result;
	
  if (!*aReturnFrame)
    return NS_ERROR_UNEXPECTED;
		
  // find the child frame containing the offset we want
  result = (*aReturnFrame)->GetChildFrameContainingOffset(*aReturnOffset, aHint, &aOffset, aReturnFrame);
  return result;
}

NS_IMETHODIMP 
nsSelection::CharacterMove(PRBool aForward, PRBool aExtend)
{
  if (aForward)
    return MoveCaret(nsIDOMKeyEvent::DOM_VK_RIGHT,aExtend,eSelectCharacter);
  else
    return MoveCaret(nsIDOMKeyEvent::DOM_VK_LEFT,aExtend,eSelectCharacter);
}

NS_IMETHODIMP
nsSelection::WordMove(PRBool aForward, PRBool aExtend)
{
  if (aForward)
    return MoveCaret(nsIDOMKeyEvent::DOM_VK_RIGHT,aExtend,eSelectWord);
  else
    return MoveCaret(nsIDOMKeyEvent::DOM_VK_LEFT,aExtend,eSelectWord);
}

NS_IMETHODIMP
nsSelection::LineMove(PRBool aForward, PRBool aExtend)
{
  if (aForward)
    return MoveCaret(nsIDOMKeyEvent::DOM_VK_DOWN,aExtend,eSelectLine);
  else
    return MoveCaret(nsIDOMKeyEvent::DOM_VK_UP,aExtend,eSelectLine);
}

NS_IMETHODIMP
nsSelection::IntraLineMove(PRBool aForward, PRBool aExtend)
{
  if (aForward)
    return MoveCaret(nsIDOMKeyEvent::DOM_VK_END,aExtend,eSelectLine);
  else
    return MoveCaret(nsIDOMKeyEvent::DOM_VK_HOME,aExtend,eSelectLine);
}

NS_IMETHODIMP nsSelection::SelectAll()
{
  nsCOMPtr<nsIContent> rootContent;
  if (mLimiter)
  {
    rootContent = mLimiter;//addrefit
  }
  else
  {
    nsresult rv;
    nsCOMPtr<nsIPresShell> shell(do_QueryInterface(mTracker,&rv));
    if (NS_FAILED(rv) || !shell) {
      return NS_ERROR_FAILURE;
    }

    nsCOMPtr<nsIDocument> doc;
    rv = shell->GetDocument(getter_AddRefs(doc));
    if (NS_FAILED(rv) || !doc)
      return rv?rv:NS_ERROR_FAILURE;
    rootContent = getter_AddRefs(doc->GetRootContent());
    if (!rootContent)
      return NS_ERROR_FAILURE;
  }
  PRInt32 numChildren;
  rootContent->ChildCount(numChildren);
  PostReason(nsIDOMSelectionListener::NO_REASON);
  return TakeFocus(mLimiter, 0, numChildren, PR_FALSE, PR_FALSE);
}

//////////END FRAMESELECTION

NS_IMETHODIMP
nsSelection::StartBatchChanges()
{
  nsresult result(NS_OK);
  mBatching++;
  return result;
}

 
NS_IMETHODIMP
nsSelection::EndBatchChanges()
{
  nsresult result(NS_OK);
  mBatching--;
  NS_ASSERTION(mBatching >=0,"Bad mBatching");
  if (mBatching == 0 && mChangesDuringBatching){
    mChangesDuringBatching = PR_FALSE;
    NotifySelectionListeners(nsISelectionController::SELECTION_NORMAL);
  }
  return result;
}


nsresult
nsSelection::NotifySelectionListeners(SelectionType aType)
{
  PRInt8 index = GetIndexFromSelectionType(aType);
  if (index >=0)
  {
    return mDomSelections[index]->NotifySelectionListeners();
  }
  return NS_ERROR_FAILURE;
}

nsresult
nsSelection::FrameOrParentHasSpecialSelectionStyle(nsIFrame* aFrame, PRUint8 aSelectionStyle, nsIFrame* *foundFrame)
{
  nsIFrame* thisFrame = aFrame;
  
  while (thisFrame)
  {
	  const nsStyleUserInterface* userinterface;
	  thisFrame->GetStyleData(eStyleStruct_UserInterface, (const nsStyleStruct*&)userinterface);
  
    if (userinterface->mUserSelect == aSelectionStyle)
    {
      *foundFrame = thisFrame;
      return NS_OK;
    }
  
    thisFrame->GetParent(&thisFrame);
  }
  
  *foundFrame = nsnull;
  return NS_OK;
}


// Start of Table Selection methods

static PRBool IsCell(nsIContent *aContent)
{
  nsIAtom *tag;
  aContent->GetTag(tag);
  return (tag != 0 && tag == nsSelection::sCellAtom);
}

nsITableCellLayout* 
nsSelection::GetCellLayout(nsIContent *aCellContent)
{
  // Get frame for cell
  nsIFrame  *cellFrame = nsnull;
  nsresult result = GetTracker()->GetPrimaryFrameFor(aCellContent, &cellFrame);
  if (!cellFrame) return nsnull;
  nsITableCellLayout *cellLayoutObject = nsnull;
  result = cellFrame->QueryInterface(NS_GET_IID(nsITableCellLayout), (void**)(&cellLayoutObject)); 
  if (NS_FAILED(result)) return nsnull;
  return cellLayoutObject;
}

nsITableLayout* 
nsSelection::GetTableLayout(nsIContent *aTableContent)
{
  // Get frame for table
  nsIFrame  *tableFrame = nsnull;
  nsresult result = GetTracker()->GetPrimaryFrameFor(aTableContent, &tableFrame);
  if (!tableFrame) return nsnull;
  nsITableLayout *tableLayoutObject = nsnull;
  result = tableFrame->QueryInterface(NS_GET_IID(nsITableLayout), (void**)(&tableLayoutObject)); 
  if (NS_FAILED(result)) return nsnull;
  return tableLayoutObject;
}

nsresult
nsSelection::ClearNormalSelection()
{
  PRInt8 index = GetIndexFromSelectionType(nsISelectionController::SELECTION_NORMAL);
  return mDomSelections[index]->ClearSelection();
}

nsresult
nsSelection::HandleTableSelection(nsIContent *aParentContent, PRInt32 aContentOffset, PRUint32 aTarget, nsMouseEvent *aMouseEvent)
{
  if (!aParentContent) return NS_ERROR_NULL_POINTER;
  if (mSelectingTableCells && (aTarget & TABLESELECTION_TABLE))
  {
    // We were selecting cells and user drags mouse in table border or inbetween cells,
    //  just do nothing
      return NS_OK;
  }

  nsCOMPtr<nsIDOMNode> parentNode = do_QueryInterface(aParentContent);
  if (!parentNode)  return NS_ERROR_FAILURE;

  nsCOMPtr<nsIContent> childContent;
  nsresult result = aParentContent->ChildAt(aContentOffset, *getter_AddRefs(childContent));
  if (NS_FAILED(result)) return result;
  if (!childContent)  return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDOMNode> childNode = do_QueryInterface(childContent);
  if (!childNode)  return NS_ERROR_FAILURE;

  // When doing table selection, always set the direction to next
  //  so we can be sure that anchorNode's offset always points to the selected cell
  PRInt8 index = GetIndexFromSelectionType(nsISelectionController::SELECTION_NORMAL);
  mDomSelections[index]->SetDirection(eDirNext);

  // Stack-class to wrap all table selection changes in 
  //  BeginBatchChanges() / EndBatchChanges()
  nsSelectionBatcher selectionBatcher(mDomSelections[index]);

  PRInt32 startRowIndex, startColIndex, curRowIndex, curColIndex;
  if (mSelectingTableCells)
  {
    // We are drag-selecting

    if (aTarget != TABLESELECTION_TABLE)
    {
      // If dragging in the same cell as last event, do nothing
      if (mEndSelectedCell == childContent)
        return NS_OK;

      // aTarget can be any "cell mode",
      //  so we can easily drag-select rows and columns 
      // Once we are in row or column mode,
      //  we can drift into any cell to stay in that mode
      //  even if aTarget = TABLESELECTION_CELL

      if (mSelectingTableCellMode == TABLESELECTION_ROW ||
          mSelectingTableCellMode == TABLESELECTION_COLUMN)
      {
        if (mEndSelectedCell)
        {
          // Also check if cell is in same row/col
          result = GetCellIndexes(mEndSelectedCell, startRowIndex, startColIndex);
          if (NS_FAILED(result)) return result;
          result = GetCellIndexes(childContent, curRowIndex, curColIndex);
          if (NS_FAILED(result)) return result;
        
          if ((mSelectingTableCellMode == TABLESELECTION_ROW && startRowIndex == curRowIndex) ||
              (mSelectingTableCellMode == TABLESELECTION_COLUMN && startColIndex == curColIndex)) 
            return NS_OK;
        }
#ifdef DEBUG_TABLE_SELECTION
printf("HandleTableSelection: Dragged into a new column or row\n");
#endif
        // Continue dragging row or column selection
        return SelectRowOrColumn(childContent, mSelectingTableCellMode);
      }
      else if (mSelectingTableCellMode == TABLESELECTION_CELL)
      {
#ifdef DEBUG_TABLE_SELECTION
printf("HandleTableSelection: Dragged into a new cell\n");
#endif
        // Clear this to be sure SelectBlockOfCells works correctly
        mAppendStartSelectedCell = nsnull;
        
        // Trick for quick selection of rows and columns
        // Hold down shift, then start selecting in one direction
        // If next cell dragged into is in same row, select entire row,
        //   if next cell is in same column, select entire column
        if (mStartSelectedCell && aMouseEvent->isShift)
        {
          result = GetCellIndexes(mStartSelectedCell, startRowIndex, startColIndex);
          if (NS_FAILED(result)) return result;
          result = GetCellIndexes(childContent, curRowIndex, curColIndex);
          if (NS_FAILED(result)) return result;
          
          if (startRowIndex == curRowIndex || 
              startColIndex == curColIndex)
          {
            // Force new selection block
            mStartSelectedCell = nsnull;
            mDomSelections[index]->ClearSelection();

            mSelectingTableCellMode = 
              (startRowIndex == curRowIndex) ? TABLESELECTION_ROW : TABLESELECTION_COLUMN;

            return SelectRowOrColumn(childContent, mSelectingTableCellMode);
          }
        }
        
        // Reselect block of cells to new end location
        return SelectBlockOfCells(childContent);
      }
    }
    // Do nothing if dragging in table, but outside a cell
    return NS_OK;
  }
  else 
  {
    // Not dragging  -- mouse event is down or up

    if (mMouseDownState)
    {
#ifdef DEBUG_TABLE_SELECTION
printf("HandleTableSelection: Mouse down event\n");
#endif
      // Clear cell we stored in mouse-down
      mUnselectCellOnMouseUp = nsnull;
      
      if (aTarget == TABLESELECTION_CELL)
      {
        PRBool isSelected = PR_FALSE;

        // Check if we have other selected cells
        nsCOMPtr<nsIDOMNode> previousCellNode;
        GetFirstSelectedCellAndRange(getter_AddRefs(previousCellNode), nsnull);
        if (previousCellNode)
        {
          // We have at least 1 other selected cell

          // Check if new cell is already selected
          nsIFrame  *cellFrame = nsnull;
          result = GetTracker()->GetPrimaryFrameFor(childContent, &cellFrame);
          if (NS_FAILED(result)) return result;
          if (!cellFrame) return NS_ERROR_NULL_POINTER;
          result = cellFrame->GetSelected(&isSelected);
          if (NS_FAILED(result)) return result;
        }
        else
        {
          // No cells selected -- remove non-cell selection
          mDomSelections[index]->ClearSelection();
        }
        mSelectingTableCells = PR_TRUE;    // Signal to start drag-cell-selection
        mSelectingTableCellMode = aTarget;
        // Set start for new drag-selection block (not appended)
        mStartSelectedCell = childContent;
        // The initial block end is same as the start
        mEndSelectedCell = childContent;
        
        if (isSelected)
        {
          // Remember this cell to (possibly) unselect it on mouseup
          mUnselectCellOnMouseUp = childContent;
#ifdef DEBUG_TABLE_SELECTION
printf("HandleTableSelection: Saving mUnselectCellOnMouseUp\n");
#endif
        }
        else
        {
          // Select an unselected cell
          // but first remove existing selection if not in same table
          nsCOMPtr<nsIContent> previousCellContent = do_QueryInterface(previousCellNode);
          if (!IsInSameTable(previousCellContent, childContent, nsnull))
          {
            mDomSelections[index]->ClearSelection();
            // Reset selection mode that is cleared in ClearSelection
            mSelectingTableCellMode = aTarget;
          }

          nsCOMPtr<nsIDOMElement> cellElement = do_QueryInterface(childContent);
          return SelectCellElement(cellElement);
        }

        return NS_OK;
      }
      else if (aTarget == TABLESELECTION_TABLE)
      {
        //TODO: We currently select entire table when clicked between cells,
        //  should we restrict to only around border?
        //  *** How do we get location data for cell and click?
        mSelectingTableCells = PR_FALSE;
        mStartSelectedCell = nsnull;
        mEndSelectedCell = nsnull;

        // Remove existing selection and select the table
        mDomSelections[index]->ClearSelection();
        return CreateAndAddRange(parentNode, aContentOffset);
      }
      else if (aTarget == TABLESELECTION_ROW || aTarget == TABLESELECTION_COLUMN)
      {
        // Start drag-selecting mode so multiple rows/cols can be selected
        // Note: Currently, nsFrame::GetDataForTableSelection
        //       will never call us for row or column selection on mouse down
        mSelectingTableCells = PR_TRUE;
      
        // Force new selection block
        mStartSelectedCell = nsnull;
        mDomSelections[index]->ClearSelection();
        // Always do this AFTER ClearSelection
        mSelectingTableCellMode = aTarget;
        return SelectRowOrColumn(childContent, aTarget);
      }
    }
    else
    {
#ifdef DEBUG_TABLE_SELECTION
printf("HandleTableSelection: Mouse UP event\n");
#endif
      // First check if we are extending a block selection
      PRInt32 rangeCount;
      result = mDomSelections[index]->GetRangeCount(&rangeCount);
      if (NS_FAILED(result)) return result;

      if (rangeCount > 0 && aMouseEvent->isShift && 
          mAppendStartSelectedCell && mAppendStartSelectedCell != childContent)
      {
        // If Shift is down as well, append a block selection
        return SelectBlockOfCells(childContent);
      }
    
      // Unselect a cell only if it wasn't
      //  just selected on mousedown
      if( childContent == mUnselectCellOnMouseUp)
      {
        // Scan ranges to find the cell to unselect (the selection range to remove)
        nsCOMPtr<nsIDOMNode> previousCellParent;
        nsCOMPtr<nsIDOMRange> range;
        PRInt32 offset;
#ifdef DEBUG_TABLE_SELECTION
printf("HandleTableSelection: Unselecting mUnselectCellOnMouseUp; rangeCount=%d\n", rangeCount);
#endif
        for( PRInt32 i = 0; i < rangeCount; i++)
        {
          result = mDomSelections[index]->GetRangeAt(i, getter_AddRefs(range));
          if (NS_FAILED(result)) return result;
          if (!range) return NS_ERROR_NULL_POINTER;

          nsCOMPtr<nsIDOMNode> parent;
          result = range->GetStartParent(getter_AddRefs(parent));
          if (NS_FAILED(result)) return result;
          if (!parent) return NS_ERROR_NULL_POINTER;

          range->GetStartOffset(&offset);
          // Be sure previous selection is a table cell
          nsCOMPtr<nsIContent> parentContent = do_QueryInterface(parent);
          nsCOMPtr<nsIContent> childContent;
          result = parentContent->ChildAt(offset, *getter_AddRefs(childContent));
          if (NS_FAILED(result)) return result;
          if (childContent && IsCell(childContent))
            previousCellParent = parent;

          // We're done if we didn't find parent of a previously-selected cell
          if (!previousCellParent) break;
        
          if (previousCellParent == parentNode && offset == aContentOffset)
          {
            // Cell is already selected
            if (rangeCount == 1)
            {
#ifdef DEBUG_TABLE_SELECTION
printf("HandleTableSelection: Unselecting single selected cell\n");
#endif
              // This was the only cell selected.
              // Collapse to "normal" selection inside the cell
              mSelectingTableCells = PR_FALSE;
              mStartSelectedCell = nsnull;
              mEndSelectedCell = nsnull;
              mSelectingTableCellMode = 0;
              //TODO: We need a "Collapse to just before deepest child" routine
              // Even better, should we collapse to just after the LAST deepest child
              //  (i.e., at the end of the cell's contents)?
              return mDomSelections[index]->Collapse(childNode, 0);
            }
#ifdef DEBUG_TABLE_SELECTION
printf("HandleTableSelection: Removing cell from multi-cell selection\n");
#endif
            //TODO: Should we try to reassign to a different existing cell?
            //mStartSelectedCell = nsnull;
            //mEndSelectedCell = nsnull;
            // Other cells are selected:
            // Deselect cell by removing its range from selection
            return mDomSelections[index]->RemoveRange(range);
          }
        }
        mUnselectCellOnMouseUp = nsnull;
        // Should we just return here?
        // (we failed to unselect the cell)
      }
      // We have mouse up in a cell that was just selected on mouse down,
      //  (and no drag or shift-extend action intervened)
      //  Use it as the start of a block that 
      //  we may append by using Shift+click in another cell
      mAppendStartSelectedCell = childContent;
#ifdef DEBUG_TABLE_SELECTION
printf("HandleTableSelection: Setting mAppendStartSelectedCell for append block\n");
#endif
    }
  }
  return result;
}

nsresult
nsSelection::SelectBlockOfCells(nsIContent *aEndCell)
{
  if (!aEndCell) return NS_ERROR_NULL_POINTER;
  mEndSelectedCell = aEndCell;

  nsCOMPtr<nsIContent> startCell;
  nsresult result = NS_OK;

  if (mAppendStartSelectedCell)
  {
    // We are appending a new block
#ifdef DEBUG_TABLE_SELECTION
printf("SelectBlockOfCells -- using mAppendStartSelectedCell\n");
#endif
    startCell = mAppendStartSelectedCell;
  }
  else if (mStartSelectedCell)
  {
    startCell = mStartSelectedCell;
#ifdef DEBUG_TABLE_SELECTION
printf("SelectBlockOfCells -- using mStartSelectedCell\n");
#endif
  }

  if (!startCell)
  {
#ifdef DEBUG_TABLE_SELECTION
printf("SelectBlockOfCells -- NO START CELL!\n");
#endif
    return NS_OK;
  }
  // If new end cell is in a different table, do nothing
  nsCOMPtr<nsIContent> table;
  if (!IsInSameTable(startCell, aEndCell, getter_AddRefs(table)))
    return NS_OK;

  // Get starting and ending cells' location in the cellmap
  PRInt32 startRowIndex, startColIndex, endRowIndex, endColIndex;
  result = GetCellIndexes(startCell, startRowIndex, startColIndex);
  if(NS_FAILED(result)) return result;
  result = GetCellIndexes(aEndCell, endRowIndex, endColIndex);
  if(NS_FAILED(result)) return result;

  // Get TableLayout interface to access cell data based on cellmap location
  // frames are not ref counted, so don't use an nsCOMPtr
  nsITableLayout *tableLayoutObject = GetTableLayout(table);
  if (!tableLayoutObject) return NS_ERROR_FAILURE;

  PRInt32 curRowIndex, curColIndex;

  if (!mAppendStartSelectedCell)
  {
    // Not appending - remove selected cells outside of new block limits

    PRInt8 index = GetIndexFromSelectionType(nsISelectionController::SELECTION_NORMAL);

    nsCOMPtr<nsIDOMNode> cellNode;
    nsCOMPtr<nsIDOMRange> range;
    result = GetFirstSelectedCellAndRange(getter_AddRefs(cellNode), getter_AddRefs(range));
    if (NS_FAILED(result)) return result;

    PRInt32 minRowIndex = PR_MIN(startRowIndex, endRowIndex);
    PRInt32 maxRowIndex = PR_MAX(startRowIndex, endRowIndex);
    PRInt32 minColIndex = PR_MIN(startColIndex, endColIndex);
    PRInt32 maxColIndex = PR_MAX(startColIndex, endColIndex);

    while (cellNode)
    {
      nsCOMPtr<nsIContent> childContent = do_QueryInterface(cellNode);
      result = GetCellIndexes(childContent, curRowIndex, curColIndex);
      if (NS_FAILED(result)) return result;

#ifdef DEBUG_TABLE_SELECTION
if (!range)
printf("SelectBlockOfCells -- range is null\n");
#endif
      if (range &&
          (curRowIndex < minRowIndex || curRowIndex > maxRowIndex || 
           curColIndex < minColIndex || curColIndex > maxColIndex))
      {
        mDomSelections[index]->RemoveRange(range);
        // Since we've removed the range, decrement pointer to next range
        mSelectedCellIndex--;
      }    
      result = GetNextSelectedCellAndRange(getter_AddRefs(cellNode), getter_AddRefs(range));
      if (NS_FAILED(result)) return result;
    }
  }

  nsCOMPtr<nsIDOMElement> cellElement;
  PRInt32 rowSpan, colSpan, actualRowSpan, actualColSpan;
  PRBool  isSelected;

  // Note that we select block in the direction of user's mouse dragging,
  //  which means start cell may be after the end cell in either row or column
  PRInt32 row = startRowIndex;
  while(PR_TRUE)
  {
    PRInt32 col = startColIndex;
    while(PR_TRUE)
    {
      result = tableLayoutObject->GetCellDataAt(row, col, *getter_AddRefs(cellElement),
                                                curRowIndex, curColIndex, rowSpan, colSpan, 
                                                actualRowSpan, actualColSpan, isSelected);
      if (NS_FAILED(result)) return result;

      NS_ASSERTION(actualColSpan, "!actualColSpan is 0!");

      // Skip cells that are spanned from previous locations or are already selected
      if (!isSelected && cellElement && row == curRowIndex && col == curColIndex)
      {
        result = SelectCellElement(cellElement);
        if (NS_FAILED(result)) return result;
      }
      // Done when we reach end column
      if (col == endColIndex) break;

      if (startColIndex < endColIndex)
        col ++;
      else
        col--;
    };
    if (row == endRowIndex) break;

    if (startRowIndex < endRowIndex)
      row++;
    else
      row--;
  };
  return result;
}

nsresult
nsSelection::SelectRowOrColumn(nsIContent *aCellContent, PRUint32 aTarget)
{
  if (!aCellContent) return NS_ERROR_NULL_POINTER;

  nsCOMPtr<nsIContent> table;
  nsresult result = GetParentTable(aCellContent, getter_AddRefs(table));
  if (NS_FAILED(result)) return PR_FALSE;
  if (!table) return NS_ERROR_NULL_POINTER;

  // Get table and cell layout interfaces to access 
  //   cell data based on cellmap location
  // Frames are not ref counted, so don't use an nsCOMPtr
  nsITableLayout *tableLayout = GetTableLayout(table);
  if (!tableLayout) return NS_ERROR_FAILURE;
  nsITableCellLayout *cellLayout = GetCellLayout(aCellContent);
  if (!cellLayout) return NS_ERROR_FAILURE;

  // Get location of target cell:      
  PRInt32 rowIndex, colIndex, curRowIndex, curColIndex;
  result = cellLayout->GetCellIndexes(rowIndex, colIndex);
  if (NS_FAILED(result)) return result;

  // Be sure we start at proper beginning
  // (This allows us to select row or col given ANY cell!)
  if (aTarget == TABLESELECTION_ROW)
    colIndex = 0;
  if (aTarget == TABLESELECTION_COLUMN)
    rowIndex = 0;

  nsCOMPtr<nsIDOMElement> cellElement;
  nsCOMPtr<nsIDOMElement> firstCell;
  nsCOMPtr<nsIDOMElement> lastCell;
  PRInt32 rowSpan, colSpan, actualRowSpan, actualColSpan;
  PRBool isSelected;

  do {
    // Loop through all cells in column or row to find first and last
    result = tableLayout->GetCellDataAt(rowIndex, colIndex, *getter_AddRefs(cellElement),
                                        curRowIndex, curColIndex, rowSpan, colSpan, 
                                        actualRowSpan, actualColSpan, isSelected);
    if (NS_FAILED(result)) return result;
    if (cellElement)
    {
      NS_ASSERTION(actualRowSpan > 0 && actualColSpan> 0, "SelectRowOrColumn: Bad rowspan or colspan\n");
      if (!firstCell)
        firstCell = cellElement;

      lastCell = cellElement;

      // Move to next cell in cellmap, skipping spanned locations
      if (aTarget == TABLESELECTION_ROW)
        colIndex += actualColSpan;
      else
        rowIndex += actualRowSpan;
    }
  }
  while (cellElement);

  // Use SelectBlockOfCells:
  // This will replace existing selection,
  //  but allow unselecting by dragging out of selected region
  if (firstCell && lastCell)
  {
    if (!mStartSelectedCell)
    {
      // We are starting a new block, so select the first cell
      result = SelectCellElement(firstCell);
      if (NS_FAILED(result)) return result;
      mStartSelectedCell = do_QueryInterface(firstCell);
    }
    nsCOMPtr<nsIContent> lastCellContent = do_QueryInterface(lastCell);
    return SelectBlockOfCells(lastCellContent);
  }

#if 0
// This is a more efficient strategy that appends row to current selection,
//  but doesn't allow dragging OFF of an existing selection to unselect!
  do {
    // Loop through all cells in column or row
    result = tableLayout->GetCellDataAt(rowIndex, colIndex, *getter_AddRefs(cellElement),
                                        curRowIndex, curColIndex, rowSpan, colSpan, 
                                        actualRowSpan, actualColSpan, isSelected);
    if (NS_FAILED(result)) return result;
    // We're done when cell is not found
    if (!cellElement) break;


    // Check spans else we infinitely loop
    NS_ASSERTION(actualColSpan, "actualColSpan is 0!");
    NS_ASSERTION(actualRowSpan, "actualRowSpan is 0!");
    
    // Skip cells that are already selected or span from outside our region
    if (!isSelected && rowIndex == curRowIndex && colIndex == curColIndex)
    {
      result = SelectCellElement(cellElement);
      if (NS_FAILED(result)) return result;
    }
    // Move to next row or column in cellmap, skipping spanned locations
    if (aTarget == TABLESELECTION_ROW)
      colIndex += actualColSpan;
    else
      rowIndex += actualRowSpan;
  }
  while (cellElement);
#endif

  return NS_OK;
}

nsresult 
nsSelection::GetFirstCellNodeInRange(nsIDOMRange *aRange, nsIDOMNode **aCellNode)
{
  if (!aRange || !aCellNode) return NS_ERROR_NULL_POINTER;

  *aCellNode = nsnull;

  nsCOMPtr<nsIDOMNode> startParent;
  nsresult result = aRange->GetStartParent(getter_AddRefs(startParent));
  if (NS_FAILED(result)) return result;
  if (!startParent) return NS_ERROR_FAILURE;

  PRInt32 offset;
  result = aRange->GetStartOffset(&offset);
  if (NS_FAILED(result)) return result;

  nsCOMPtr<nsIContent> parentContent = do_QueryInterface(startParent);
  nsCOMPtr<nsIContent> childContent;
  result = parentContent->ChildAt(offset, *getter_AddRefs(childContent));
  if (NS_FAILED(result)) return result;
  if (!childContent) return NS_ERROR_NULL_POINTER;
  // Don't return node if not a cell
  if (!IsCell(childContent)) return NS_OK;

  nsCOMPtr<nsIDOMNode> childNode = do_QueryInterface(childContent);
  if (childNode)
  {
    *aCellNode = childNode;
    NS_ADDREF(*aCellNode);
  }
  return NS_OK;
}

nsresult 
nsSelection::GetFirstSelectedCellAndRange(nsIDOMNode **aCell, nsIDOMRange **aRange)
{
  if (!aCell) return NS_ERROR_NULL_POINTER;
  *aCell = nsnull;

  // aRange is optional
  if (aRange)
    *aRange = nsnull;

  nsCOMPtr<nsIDOMRange> firstRange;
  PRInt8 index = GetIndexFromSelectionType(nsISelectionController::SELECTION_NORMAL);
  nsresult result = mDomSelections[index]->GetRangeAt(0, getter_AddRefs(firstRange));
  if (NS_FAILED(result)) return result;
  if (!firstRange) return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDOMNode> cellNode;
  result = GetFirstCellNodeInRange(firstRange, getter_AddRefs(cellNode));
  if (NS_FAILED(result)) return result;
  if (!cellNode) return NS_OK;

  *aCell = cellNode;
  NS_ADDREF(*aCell);
  if (aRange)
  {
    *aRange = firstRange;
    NS_ADDREF(*aRange);
  }

  // Setup for next cell
  mSelectedCellIndex = 1;

  return NS_OK;
}

nsresult
nsSelection::GetNextSelectedCellAndRange(nsIDOMNode **aCell, nsIDOMRange **aRange)
{
  if (!aCell) return NS_ERROR_NULL_POINTER;
  *aCell = nsnull;

  // aRange is optional
  if (aRange)
    *aRange = nsnull;

  PRInt32 rangeCount;
  PRInt8 index = GetIndexFromSelectionType(nsISelectionController::SELECTION_NORMAL);
  nsresult result = mDomSelections[index]->GetRangeCount(&rangeCount);
  if (NS_FAILED(result)) return result;

  // Don't even try if index exceeds range count
  if (mSelectedCellIndex >= rangeCount) 
  {
    // Should we reset index? 
    // Maybe better to force recalling GetFirstSelectedCell()
    //mSelectedCellIndex = 0;
    return NS_OK;
  }

  // Get first node in next range of selection - test if it's a cell
  nsCOMPtr<nsIDOMRange> range;
  result = mDomSelections[index]->GetRangeAt(mSelectedCellIndex, getter_AddRefs(range));
  if (NS_FAILED(result)) return result;
  if (!range) return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDOMNode> cellNode;
  result = GetFirstCellNodeInRange(range, getter_AddRefs(cellNode));
  if (NS_FAILED(result)) return result;
  // No cell in selection range
  if (!cellNode) return NS_OK;

  *aCell = cellNode;
  NS_ADDREF(*aCell);
  if (aRange)
  {
    *aRange = range;
    NS_ADDREF(*aRange);
  }

  // Setup for next cell
  mSelectedCellIndex++;

  return NS_OK;
}

nsresult
nsSelection::GetCellIndexes(nsIContent *aCell, PRInt32 &aRowIndex, PRInt32 &aColIndex)
{
  if (!aCell) return NS_ERROR_NULL_POINTER;

  aColIndex=0; // initialize out params
  aRowIndex=0;

  nsITableCellLayout *cellLayoutObject = GetCellLayout(aCell);
  if (!cellLayoutObject)  return NS_ERROR_FAILURE;
  return cellLayoutObject->GetCellIndexes(aRowIndex, aColIndex);
}

PRBool 
nsSelection::IsInSameTable(nsIContent *aContent1, nsIContent *aContent2, nsIContent **aTable)
{
  if (!aContent1 || !aContent2) return PR_FALSE;
  
  // aTable is optional:
  if(aTable) *aTable = nsnull;
  
  nsCOMPtr<nsIContent> tableNode1;
  nsCOMPtr<nsIContent> tableNode2;

  nsresult result = GetParentTable(aContent1, getter_AddRefs(tableNode1));
  if (NS_FAILED(result)) return PR_FALSE;
  result = GetParentTable(aContent2, getter_AddRefs(tableNode2));
  if (NS_FAILED(result)) return PR_FALSE;

  // Must be in the same table
  if (tableNode1 && (tableNode1 == tableNode2))
  {
    if (aTable)
    {
      *aTable = tableNode1;
      NS_ADDREF(*aTable);
    }
    return PR_TRUE;;
  }
  return PR_FALSE;
}

nsresult
nsSelection::GetParentTable(nsIContent *aCell, nsIContent **aTable)
{
  if (!aCell || !aTable)
    return NS_ERROR_NULL_POINTER;

  nsCOMPtr<nsIContent> parent;
  nsresult result = aCell->GetParent(*getter_AddRefs(parent));

  while (NS_SUCCEEDED(result) && parent)
  {  
    nsIAtom *tag;
    parent->GetTag(tag);
    if (tag && tag == nsSelection::sTableAtom)
    {
      *aTable = parent;
      NS_ADDREF(*aTable);
      return NS_OK;
    }
    nsCOMPtr<nsIContent> temp;
    result = parent->GetParent(*getter_AddRefs(temp));
    parent = temp;
  }
  return result;
}

nsresult
nsSelection::SelectCellElement(nsIDOMElement *aCellElement)
{
  nsCOMPtr<nsIDOMNode> cellNode = do_QueryInterface(aCellElement);
  nsCOMPtr<nsIDOMNode> parent;
  nsresult result = cellNode->GetParentNode(getter_AddRefs(parent));
  if (NS_FAILED(result)) return result;
  if (!parent) return NS_ERROR_FAILURE;

  nsCOMPtr<nsIContent> parentContent = do_QueryInterface(parent);
  nsCOMPtr<nsIContent> cellContent = do_QueryInterface(aCellElement);

  // Get child offset
  PRInt32 offset;
  result = parentContent->IndexOf(cellContent, offset);
  if (NS_FAILED(result)) return result;
  return CreateAndAddRange(parent, offset);
}

nsresult
nsSelection::CreateAndAddRange(nsIDOMNode *aParentNode, PRInt32 aOffset)
{
  if (!aParentNode) return NS_ERROR_NULL_POINTER;
  nsCOMPtr<nsIDOMRange> range;
  NS_NewRange(getter_AddRefs(range));
  if (!range) return NS_ERROR_OUT_OF_MEMORY;

  // Set range around child at given offset
  nsresult result = range->SetStart(aParentNode, aOffset);
  if (NS_FAILED(result)) return result;
  result = range->SetEnd(aParentNode, aOffset+1);
  if (NS_FAILED(result)) return result;
  
  PRInt8 index = GetIndexFromSelectionType(nsISelectionController::SELECTION_NORMAL);
  return mDomSelections[index]->AddRange(range);
}

// End of Table Selection

NS_IMETHODIMP
nsSelection::AdjustOffsetsFromStyle(nsIFrame *aFrame, PRBool *changeSelection,
      nsIContent** outContent, PRInt32* outStartOffset, PRInt32* outEndOffset)
{
  
  *changeSelection = PR_FALSE;
  *outContent = nsnull;
  
  nsresult  rv;  
  nsIFrame*   selectAllFrame;
  rv = FrameOrParentHasSpecialSelectionStyle(aFrame, NS_STYLE_USER_SELECT_ALL, &selectAllFrame);
  if (NS_FAILED(rv)) return rv;
  
  if (!selectAllFrame)
    return NS_OK;
  
  nsCOMPtr<nsIContent> selectAllContent;
  selectAllFrame->GetContent(getter_AddRefs(selectAllContent));
  if (selectAllContent)
  {
    nsCOMPtr<nsIContent>  parentContent;
    rv = selectAllContent->GetParent(*getter_AddRefs(parentContent));
    if (parentContent)
    {
      PRInt32 startOffset;
      rv = parentContent->IndexOf(selectAllContent, startOffset);
      if (NS_FAILED(rv)) return rv;
      if (startOffset < 0)
      {
        // hrmm, this is probably anonymous content. Let's go up another level
        // do we need to do this if we get the right frameSelection to start with?
        nsCOMPtr<nsIContent> superParent;
        parentContent->GetParent(*getter_AddRefs(superParent));
        if (superParent)
        {
          PRInt32 superStartOffset;
          rv = superParent->IndexOf(parentContent, superStartOffset);
          if (NS_FAILED(rv)) return rv;
          if (superStartOffset < 0)
            return NS_ERROR_FAILURE;    // give up
        
          parentContent = superParent;
          startOffset = superStartOffset;
        }
      }
      
      NS_IF_ADDREF(*outContent = parentContent);

      *outStartOffset = startOffset;
      *outEndOffset = startOffset + 1;

      *changeSelection = PR_TRUE;
    }    
  }

  return NS_OK;
}


NS_IMETHODIMP
nsSelection::SetHint(HINT aHintRight)
{
  mHint = aHintRight;
  return NS_OK;
}

NS_IMETHODIMP
nsSelection::GetHint(HINT *aHintRight)
{
  *aHintRight = mHint;
  return NS_OK; 
}


//END nsIFrameSelection methods


#ifdef XP_MAC
#pragma mark -
#endif

//BEGIN nsIDOMSelection interface implementations



/** DeleteFromDocument
 *  will return NS_OK if it handles the event or NS_COMFALSE if not.
 */
NS_IMETHODIMP
nsSelection::DeleteFromDocument()
{
  nsresult res;

  // If we're already collapsed, then set ourselves to include the
  // last item BEFORE the current range, rather than the range itself,
  // before we do the delete.
  PRBool isCollapsed;
  PRInt8 index = GetIndexFromSelectionType(nsISelectionController::SELECTION_NORMAL);
  mDomSelections[index]->GetIsCollapsed( &isCollapsed);
  if (isCollapsed)
  {
    // If the offset is positive, then it's easy:
    if (mDomSelections[index]->FetchFocusOffset() > 0)
    {
      mDomSelections[index]->Extend(mDomSelections[index]->FetchFocusNode(), mDomSelections[index]->FetchFocusOffset() - 1);
    }
    else
    {
      // Otherwise it's harder, have to find the previous node
      printf("Sorry, don't know how to delete across frame boundaries yet\n");
      return NS_ERROR_NOT_IMPLEMENTED;
    }
  }

  // Get an iterator
  nsSelectionIterator iter(mDomSelections[index]);
  res = iter.First();
  if (!NS_SUCCEEDED(res))
    return res;

  nsCOMPtr<nsIDOMRange> range;
  while (iter.IsDone())
  {
    res = iter.CurrentItem(NS_STATIC_CAST(nsIDOMRange**, getter_AddRefs(range)));
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
    mDomSelections[index]->Collapse(mDomSelections[index]->FetchAnchorNode(), mDomSelections[index]->FetchAnchorOffset()-1);
  else if (mDomSelections[index]->FetchAnchorOffset() > 0)
    mDomSelections[index]->Collapse(mDomSelections[index]->FetchAnchorNode(), mDomSelections[index]->FetchAnchorOffset());
#ifdef DEBUG
  else
    printf("Don't know how to set selection back past frame boundary\n");
#endif

  return NS_OK;
}


NS_IMETHODIMP
nsSelection::SetDisplaySelection(PRInt16 aToggle)
{
  mDisplaySelection = aToggle;
  return NS_OK;
}

NS_IMETHODIMP
nsSelection::GetDisplaySelection(PRInt16 *aToggle)
{
  if (!aToggle)
    return NS_ERROR_INVALID_ARG;
  *aToggle = mDisplaySelection;
  return NS_OK;
}

NS_IMETHODIMP
nsSelection::SetDelayCaretOverExistingSelection(PRBool aDelay)
{
  mDelayCaretOverExistingSelection = aDelay;
  
  if (! aDelay)
    mDelayedMouseEventValid = PR_FALSE;

  return NS_OK;
}

NS_IMETHODIMP
nsSelection::GetDelayCaretOverExistingSelection(PRBool *aDelay)
{
  if (!aDelay)
    return NS_ERROR_NULL_POINTER;

  *aDelay =   mDelayCaretOverExistingSelection;

  return NS_OK;
}

NS_IMETHODIMP
nsSelection::SetDelayedCaretData(nsMouseEvent *aMouseEvent)
{
  if (aMouseEvent)
  {
    mDelayedMouseEventValid = PR_TRUE;
    mDelayedMouseEvent      = *aMouseEvent;

    // XXX: Hmmm, should we AddRef mDelayedMouseEvent->widget?
    //      Doing so might introduce a leak if things in the app
    //      are not released in the correct order though, so for now
    //      don't do anything.
  }
  else
    mDelayedMouseEventValid = PR_FALSE;

  return NS_OK;
}

NS_IMETHODIMP
nsSelection::GetDelayedCaretData(nsMouseEvent **aMouseEvent)
{
  if (!aMouseEvent)
    return NS_ERROR_NULL_POINTER;

  if (mDelayedMouseEventValid)
    *aMouseEvent = &mDelayedMouseEvent;
  else
    *aMouseEvent = 0;

  return NS_OK;
}

// Frame needs to get the limiting content node for parent node searches
NS_IMETHODIMP
nsSelection::GetLimiter(nsIContent **aLimiterContent)
{
  if (!aLimiterContent) return NS_ERROR_NULL_POINTER;
  *aLimiterContent = mLimiter;
  NS_IF_ADDREF(*aLimiterContent);

  return NS_OK;
}


//END nsIDOMSelection interface implementations

#ifdef XP_MAC
#pragma mark -
#endif






// nsDOMSelection implementation

// note: this can return a nil anchor node

nsDOMSelection::nsDOMSelection(nsSelection *aList)
{
  mFrameSelection = aList;
  mFixupState = PR_FALSE;
  mDirection = eDirNext;
  NS_NewISupportsArray(getter_AddRefs(mRangeArray));
  mScriptObject = nsnull;
  mAutoScrollTimer = nsnull;
  NS_NewISupportsArray(getter_AddRefs(mSelectionListeners));
  NS_INIT_REFCNT();
}


nsDOMSelection::nsDOMSelection()
{
  mFrameSelection = nsnull;
  mFixupState = PR_FALSE;
  mDirection = eDirNext;
  NS_NewISupportsArray(getter_AddRefs(mRangeArray));
  mScriptObject = nsnull;
  mAutoScrollTimer = nsnull;
  NS_NewISupportsArray(getter_AddRefs(mSelectionListeners));
  NS_INIT_REFCNT();
}



nsDOMSelection::~nsDOMSelection()
{
  PRUint32 cnt = 0;
  if (mSelectionListeners)
  {
    nsresult rv = mSelectionListeners->Count(&cnt);
    NS_ASSERTION(NS_SUCCEEDED(rv), "Count failed");
    for (PRUint32 i=0;i < cnt; i++)
	  {
	    mSelectionListeners->RemoveElementAt(i);
	  }
  }
  nsresult rv = mRangeArray->Count(&cnt);
  NS_ASSERTION(NS_SUCCEEDED(rv), "Count failed");
  PRUint32 j;
  for (j=0; j<cnt; j++)
	{
	  mRangeArray->RemoveElementAt(0);
	}
  setAnchorFocusRange(-1);

  if (mAutoScrollTimer) {
    mAutoScrollTimer->Stop();
    NS_RELEASE(mAutoScrollTimer);
  }
}



NS_IMPL_ADDREF(nsDOMSelection)

NS_IMPL_RELEASE(nsDOMSelection)

NS_IMPL_QUERY_INTERFACE4(nsDOMSelection, nsIDOMSelection, nsIScriptObjectOwner, nsISupportsWeakReference, nsIIndependentSelection)
 

NS_IMETHODIMP
nsDOMSelection::SetPresShell(nsIPresShell *aPresShell)
{
  mPresShellWeak = getter_AddRefs(NS_GetWeakReference(aPresShell));
  return NS_OK;
}



NS_IMETHODIMP
nsDOMSelection::GetAnchorNode(nsIDOMNode** aAnchorNode)
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

NS_IMETHODIMP
nsDOMSelection::GetAnchorOffset(PRInt32* aAnchorOffset)
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
NS_IMETHODIMP
nsDOMSelection::GetFocusNode(nsIDOMNode** aFocusNode)
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

NS_IMETHODIMP nsDOMSelection::GetFocusOffset(PRInt32* aFocusOffset)
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


void nsDOMSelection::setAnchorFocusRange(PRInt32 indx)
{
  PRUint32 cnt;
  nsresult rv = mRangeArray->Count(&cnt);
  if (NS_FAILED(rv)) return;    // XXX error?
  if (((PRUint32)indx) >= cnt )
    return;
  if (indx < 0) //release all
  {
    mAnchorFocusRange = nsCOMPtr<nsIDOMRange>();
  }
  else{
    nsCOMPtr<nsISupports> indexIsupports = dont_AddRef(mRangeArray->ElementAt(indx));
    mAnchorFocusRange = do_QueryInterface(indexIsupports);
  }
}



nsIDOMNode*
nsDOMSelection::FetchAnchorNode()
{  //where did the selection begin
  nsCOMPtr<nsIDOMNode>returnval;
  GetAnchorNode(getter_AddRefs(returnval));//this queries
  return returnval;
}//at end it will release, no addreff was called



PRInt32
nsDOMSelection::FetchAnchorOffset()
{
  PRInt32 returnval;
  if (NS_SUCCEEDED(GetAnchorOffset(&returnval)))//this queries
    return returnval;
  return 0;
}



nsIDOMNode*
nsDOMSelection::FetchOriginalAnchorNode()  //where did the ORIGINAL selection begin
{
  nsCOMPtr<nsIDOMNode>returnval;
  PRInt32 unused;
  GetOriginalAnchorPoint(getter_AddRefs(returnval),  &unused);//this queries
  return returnval;
}



PRInt32
nsDOMSelection::FetchOriginalAnchorOffset()
{
  nsCOMPtr<nsIDOMNode>unused;
  PRInt32 returnval;
  if (NS_SUCCEEDED(GetOriginalAnchorPoint(getter_AddRefs(unused), &returnval)))//this queries
    return returnval;
  return NS_OK;
}



nsIDOMNode*
nsDOMSelection::FetchFocusNode()
{   //where is the carret
  nsCOMPtr<nsIDOMNode>returnval;
  GetFocusNode(getter_AddRefs(returnval));//this queries
  return returnval;
}//at end it will release, no addreff was called



PRInt32
nsDOMSelection::FetchFocusOffset()
{
  PRInt32 returnval;
  if (NS_SUCCEEDED(GetFocusOffset(&returnval)))//this queries
    return returnval;
  return NS_OK;
}



nsIDOMNode*
nsDOMSelection::FetchStartParent(nsIDOMRange *aRange)   //skip all the com stuff and give me the start/end
{
  if (!aRange)
    return nsnull;
  nsCOMPtr<nsIDOMNode> returnval;
  aRange->GetStartParent(getter_AddRefs(returnval));
  return returnval;
}



PRInt32
nsDOMSelection::FetchStartOffset(nsIDOMRange *aRange)
{
  if (!aRange)
    return nsnull;
  PRInt32 returnval;
  if (NS_SUCCEEDED(aRange->GetStartOffset(&returnval)))
    return returnval;
  return 0;
}



nsIDOMNode*
nsDOMSelection::FetchEndParent(nsIDOMRange *aRange)     //skip all the com stuff and give me the start/end
{
  if (!aRange)
    return nsnull;
  nsCOMPtr<nsIDOMNode> returnval;
  aRange->GetEndParent(getter_AddRefs(returnval));
  return returnval;
}



PRInt32
nsDOMSelection::FetchEndOffset(nsIDOMRange *aRange)
{
  if (!aRange)
    return nsnull;
  PRInt32 returnval;
  if (NS_SUCCEEDED(aRange->GetEndOffset(&returnval)))
    return returnval;
  return 0;
}

nsresult
nsDOMSelection::AddItem(nsIDOMRange *aItem)
{
  if (!mRangeArray)
    return NS_ERROR_FAILURE;
  if (!aItem)
    return NS_ERROR_NULL_POINTER;
  nsresult result;
  nsCOMPtr<nsISupports> isupp = do_QueryInterface(aItem, &result);
  if (NS_SUCCEEDED(result)) {
    result = mRangeArray->AppendElement(isupp);
  }
  return result;
}



nsresult
nsDOMSelection::RemoveItem(nsIDOMRange *aItem)
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
    nsCOMPtr<nsISupports> isupp;
    aItem->QueryInterface(NS_GET_IID(nsISupports),getter_AddRefs(isupp));
    if (isupp.get() == indexIsupports.get())
    {
      mRangeArray->RemoveElementAt(i);
      return NS_OK;
    }
  }
  return NS_COMFALSE;
}



nsresult
nsDOMSelection::Clear(nsIPresContext* aPresContext)
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
    nsCOMPtr<nsISupports> isupportsindex = dont_AddRef(mRangeArray->ElementAt(0));
    nsCOMPtr<nsIDOMRange> range = do_QueryInterface(isupportsindex);
    mRangeArray->RemoveElementAt(0);
    selectFrames(aPresContext, range, 0);
    // Does RemoveElementAt also delete the elements?
  }
  // Reset direction so for more dependable table selection range handling
  SetDirection(eDirNext);
  return NS_OK;
}

//utility method to get the primary frame of node or use the offset to get frame of child node

#if 0
NS_IMETHODIMP
nsDOMSelection::GetPrimaryFrameForRangeEndpoint(nsIDOMNode *aNode, PRInt32 aOffset, PRBool aIsEndNode, nsIFrame **aReturnFrame)
{
  if (!aNode || !aReturnFrame)
    return NS_ERROR_NULL_POINTER;
  
  if (aOffset < 0)
    return NS_ERROR_FAILURE;

  *aReturnFrame = 0;
  
  nsresult	result = NS_OK;
  
  nsCOMPtr<nsIDOMNode> node = dont_QueryInterface(aNode);

  if (!node)
    return NS_ERROR_NULL_POINTER;
  
  nsCOMPtr<nsIContent> content = do_QueryInterface(node, &result);

  if (NS_FAILED(result))
    return result;

  if (!content)
    return NS_ERROR_NULL_POINTER;
  
  PRBool canContainChildren = PR_FALSE;

  result = content->CanContainChildren(canContainChildren);

  if (NS_SUCCEEDED(result) && canContainChildren)
  {
    if (aIsEndNode)
      aOffset--;

    if (aOffset >= 0)
    {
      nsCOMPtr<nsIContent> child;
      result = content->ChildAt(aOffset, *getter_AddRefs(child));
      if (NS_FAILED(result))
        return result;
      if (!child) //out of bounds?
        return NS_ERROR_FAILURE;
      content = child;//releases the focusnode
    }
  }
  result = mFrameSelection->GetTracker()->GetPrimaryFrameFor(content,aReturnFrame);
  return result;
}
#endif


NS_IMETHODIMP
nsDOMSelection::GetPrimaryFrameForAnchorNode(nsIFrame **aReturnFrame)
{
  if (!aReturnFrame)
    return NS_ERROR_NULL_POINTER;
  
  PRInt32 frameOffset = 0;
  *aReturnFrame = 0;
  nsCOMPtr<nsIContent> content = do_QueryInterface(FetchAnchorNode());
  if (content && mFrameSelection)
  {
    nsIFrameSelection::HINT hint;
    mFrameSelection->GetHint(&hint);
    return mFrameSelection->GetFrameForNodeOffset(content, FetchAnchorOffset(),hint,aReturnFrame, &frameOffset);
  }
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsDOMSelection::GetPrimaryFrameForFocusNode(nsIFrame **aReturnFrame)
{
  if (!aReturnFrame)
    return NS_ERROR_NULL_POINTER;
  
  PRInt32 frameOffset = 0;
  *aReturnFrame = 0;

  nsCOMPtr<nsIContent> content = do_QueryInterface(FetchFocusNode());
  if (content && mFrameSelection)
  {
    nsIFrameSelection::HINT hint;
    mFrameSelection->GetHint(&hint);
    return mFrameSelection->GetFrameForNodeOffset(content, FetchFocusOffset(),hint,aReturnFrame, &frameOffset);
  }
  return NS_ERROR_FAILURE;
}



//select all content children of aContent
NS_IMETHODIMP
nsDOMSelection::selectFrames(nsIPresContext* aPresContext,
                             nsIContentIterator *aInnerIter,
                             nsIContent *aContent,
                             nsIDOMRange *aRange,
                             nsIPresShell *aPresShell,
                             PRBool aFlags)
{
  if (!mFrameSelection)
    return NS_OK;//nothing to do
  nsresult result;
  nsCOMPtr<nsIGeneratedContentIterator> genericiter = do_QueryInterface(aInnerIter);
  if (genericiter && aPresShell)
  {
    result = genericiter->Init(aPresShell,aContent);
  }
  else
    result = aInnerIter->Init(aContent);
  nsIFrame *frame;
  if (NS_SUCCEEDED(result))
  {
    // First select frame of content passed in
    result = mFrameSelection->GetTracker()->GetPrimaryFrameFor(aContent, &frame);
    if (NS_SUCCEEDED(result) && frame)
      //NOTE: aRange and eSpreadDown are now IGNORED. Selected state is set only for given frame
      frame->SetSelected(aPresContext, aRange, aFlags, eSpreadDown);
    
    // Now iterated through the child frames and set them
    nsCOMPtr<nsIContent> innercontent;
    while (NS_ENUMERATOR_FALSE == aInnerIter->IsDone())
    {
      result = aInnerIter->CurrentNode(getter_AddRefs(innercontent));
      if (NS_FAILED(result) || !innercontent)
        continue;
      result = mFrameSelection->GetTracker()->GetPrimaryFrameFor(innercontent, &frame);
      if (NS_SUCCEEDED(result) && frame)
        //NOTE: aRange and eSpreadDown are now IGNORED. Selected state is set only for given frame
        frame->SetSelected(aPresContext, aRange,aFlags,eSpreadDown);//spread from here to hit all frames in flow
      result = aInnerIter->Next();
      if (NS_FAILED(result))
        return result;
    }
#if 0
    result = mFrameSelection->GetTracker()->GetPrimaryFrameFor(content, &frame);
    if (NS_SUCCEEDED(result) && frame)
      frame->SetSelected(aRange,aFlags,eSpreadDown);//spread from here to hit all frames in flow
#endif
    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}



//the idea of this helper method is to select, deselect "top to bottom" traversing through the frames
NS_IMETHODIMP
nsDOMSelection::selectFrames(nsIPresContext* aPresContext, nsIDOMRange *aRange, PRBool aFlags)
{
  if (!mFrameSelection)
    return NS_OK;//nothing to do
  if (!aRange || !aPresContext) 
    return NS_ERROR_NULL_POINTER;
  nsCOMPtr<nsIContentIterator> iter;
  nsCOMPtr<nsIContentIterator> inneriter;
  nsresult result = nsComponentManager::CreateInstance(kCGenSubtreeIteratorCID, nsnull,
                                              NS_GET_IID(nsIContentIterator), 
                                              getter_AddRefs(iter));
  if (NS_FAILED(result))
    return result;
  result = nsComponentManager::CreateInstance(kCGenContentIteratorCID, nsnull,
                                              NS_GET_IID(nsIContentIterator), 
                                              getter_AddRefs(inneriter));

  if ((NS_SUCCEEDED(result)) && iter && inneriter)
  {
    nsCOMPtr<nsIPresShell> presShell;
    result = aPresContext->GetShell(getter_AddRefs(presShell));
    if (NS_FAILED(result) && presShell)
      presShell = 0;
    nsCOMPtr<nsIGeneratedContentIterator> genericiter = do_QueryInterface(iter);
    if (genericiter && presShell)
      result = genericiter->Init(presShell,aRange);
    else
      result = iter->Init(aRange);

    // loop through the content iterator for each content node
    // for each text node:
    // get the frame for the content, and from it the style context
    // ask the style context about the property
    nsCOMPtr<nsIContent> content;
    nsIFrame *frame;
//we must call first one explicitly
    content = do_QueryInterface(FetchStartParent(aRange), &result);
    if (NS_FAILED(result) || !content)
      return result;
    PRBool canContainChildren = PR_FALSE;
    result = content->CanContainChildren(canContainChildren);
    if (NS_SUCCEEDED(result) && !canContainChildren)
    {
      result = mFrameSelection->GetTracker()->GetPrimaryFrameFor(content, &frame);
      if (NS_SUCCEEDED(result) && frame)
        frame->SetSelected(aPresContext, aRange,aFlags,eSpreadDown);//spread from here to hit all frames in flow
    }
//end start content
    result = iter->First();
    while (NS_SUCCEEDED(result) && NS_ENUMERATOR_FALSE == iter->IsDone())
    {
      result = iter->CurrentNode(getter_AddRefs(content));
      if (NS_FAILED(result) || !content)
        return result;
      selectFrames(aPresContext, inneriter, content, aRange, presShell,aFlags);
      result = iter->Next();
    }
//we must now do the last one  if it is not the same as the first
    if (FetchEndParent(aRange) != FetchStartParent(aRange))
    {
      content = do_QueryInterface(FetchEndParent(aRange), &result);
      if (NS_FAILED(result) || !content)
        return result;
      canContainChildren = PR_FALSE;
      result = content->CanContainChildren(canContainChildren);
      if (NS_SUCCEEDED(result) && !canContainChildren)
      {
        result = mFrameSelection->GetTracker()->GetPrimaryFrameFor(content, &frame);
        if (NS_SUCCEEDED(result) && frame)
           frame->SetSelected(aPresContext, aRange,aFlags,eSpreadDown);//spread from here to hit all frames in flow
      }
    }
//end end parent
  }
  return result;
}


NS_IMETHODIMP
nsDOMSelection::LookUpSelection(nsIContent *aContent, PRInt32 aContentOffset, PRInt32 aContentLength,
                           SelectionDetails **aReturnDetails, SelectionType aType, PRBool aSlowCheck)
{
  PRInt32 cnt;
  nsresult rv = GetRangeCount(&cnt);
  if (NS_FAILED(rv)) 
    return rv;
  PRInt32 i;

  nsCOMPtr<nsIDOMNode> passedInNode;
  passedInNode = do_QueryInterface(aContent);
  if (!passedInNode)
    return NS_ERROR_FAILURE;

  SelectionDetails *details = nsnull;
  SelectionDetails *newDetails = details;

  for (i =0; i<cnt; i++){
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
          if (!details){
            details = new SelectionDetails;

            newDetails = details;
          }
          else{
            newDetails->mNext = new SelectionDetails;
            newDetails = newDetails->mNext;
          }
          if (!newDetails)
            return NS_ERROR_OUT_OF_MEMORY;
          newDetails->mNext = nsnull;
          newDetails->mStart = PR_MAX(0,startOffset - aContentOffset);
          newDetails->mEnd = PR_MIN(aContentLength, endOffset - aContentOffset);
          newDetails->mType = aType;
        }
      }
      else if (passedInNode == startNode){ //are we at the beginning?
        if (startOffset < (aContentOffset + aContentLength)){
          if (!details){
            details = new SelectionDetails;
            newDetails = details;
          }
          else{
            newDetails->mNext = new SelectionDetails;
            newDetails = newDetails->mNext;
          }
          if (!newDetails)
            return NS_ERROR_OUT_OF_MEMORY;
          newDetails->mNext = nsnull;
          newDetails->mStart = PR_MAX(0,startOffset - aContentOffset);
          newDetails->mEnd = aContentLength;
          newDetails->mType = aType;
        }
      }
      else if (passedInNode == endNode){//are we at the end?
        if (endOffset > aContentOffset){
          if (!details){
            details = new SelectionDetails;
            newDetails = details;
          }
          else{
            newDetails->mNext = new SelectionDetails;
            newDetails = newDetails->mNext;
          }
          if (!newDetails)
            return NS_ERROR_OUT_OF_MEMORY;
          newDetails->mNext = nsnull;
          newDetails->mStart = 0;
          newDetails->mEnd = PR_MIN(aContentLength, endOffset - aContentOffset);
          newDetails->mType = aType;
        }
      }
      else { //then we MUST be completely selected! unless someone needs us to check to make sure with slowcheck

        if (cnt > 1 || aSlowCheck){ //if more than 1 selection or we need to do slow check see if farther than start or less than end.
          //we only have to look at start offset because anything else would have been in the range
          PRInt32 resultnum = ComparePoints(startNode, startOffset 
                                  ,passedInNode, aContentOffset);
          if (resultnum > 0)
            continue; 
          resultnum = ComparePoints(endNode, endOffset,
                              passedInNode, aContentOffset );
          if (resultnum <0)
            continue;
        }

        if (!details){
          details = new SelectionDetails;
          newDetails = details;
        }
        else{
          newDetails->mNext = new SelectionDetails;
          newDetails = newDetails->mNext;
        }
        if (!newDetails)
          return NS_ERROR_OUT_OF_MEMORY;
        newDetails->mNext = nsnull;
        newDetails->mStart = 0;
        newDetails->mEnd = aContentLength;
        newDetails->mType = aType;
     }
    }
    else
      continue;
  }
  if (*aReturnDetails && newDetails)
    newDetails->mNext = *aReturnDetails;
  if (details)
    *aReturnDetails = details;
  return NS_OK;
}

NS_IMETHODIMP
nsDOMSelection::Repaint(nsIPresContext* aPresContext)
{
  PRUint32 arrCount = 0;

  if (!mRangeArray)
    return NS_ERROR_NULL_POINTER;

  nsCOMPtr<nsISupports> isupp;
  nsCOMPtr<nsIDOMRange> range;

  nsresult result = mRangeArray->Count(&arrCount);

  if (NS_FAILED(result))
    return result;

  if (arrCount < 1)
    return NS_OK;

  PRUint32 i;

  for (i = 0; i < arrCount; i++)
  {
    result = mRangeArray->GetElementAt(i, getter_AddRefs(isupp));

    if (NS_FAILED(result))
      return result;

    if (!isupp)
      return NS_ERROR_NULL_POINTER;

    range = do_QueryInterface(isupp);

    if (!range)
      return NS_ERROR_NULL_POINTER;

    result = selectFrames(aPresContext, range, PR_TRUE);

    if (NS_FAILED(result))
      return result;
  }

  return NS_OK;
}

nsresult
nsDOMSelection::StartAutoScrollTimer(nsIPresContext *aPresContext, nsIFrame *aFrame, nsPoint& aPoint, PRUint32 aDelay)
{
  nsresult result;
  if (!mFrameSelection)
    return NS_OK;//nothing to do

  if (!mAutoScrollTimer)
  {
    result = NS_NewAutoScrollTimer(&mAutoScrollTimer);

    if (NS_FAILED(result))
      return result;

    if (!mAutoScrollTimer)
      return NS_ERROR_OUT_OF_MEMORY;

    result = mAutoScrollTimer->Init(mFrameSelection, this);

    if (NS_FAILED(result))
      return result;
  }

  result = mAutoScrollTimer->SetDelay(aDelay);

  if (NS_FAILED(result))
    return result;

  return DoAutoScroll(aPresContext, aFrame, aPoint);
}

nsresult
nsDOMSelection::StopAutoScrollTimer()
{
  if (mAutoScrollTimer)
    return mAutoScrollTimer->Stop();

  return NS_OK; 
}

nsresult
nsDOMSelection::DoAutoScroll(nsIPresContext *aPresContext, nsIFrame *aFrame, nsPoint& aPoint)
{
  nsresult result;

  if (!aPresContext || !aFrame)
    return NS_ERROR_NULL_POINTER;

  if (mAutoScrollTimer)
    result = mAutoScrollTimer->Stop();

  //
  // Get a hold of the root scrollable view for presShell.
  //

  nsIScrollableView *scrollableView = 0;

  result = GetRootScrollableView(&scrollableView);
  if (NS_SUCCEEDED(result) && scrollableView)
  {
    //
    // Get a hold of the scrollable view's clip view.
    //

    const nsIView *cView = 0;
    nsIView *scrolledView;
    result = scrollableView->GetClipView(&cView);
    scrollableView->GetScrolledView(scrolledView);
    if (NS_SUCCEEDED(result) && cView)
    {
      //
      // Find out if this frame's view is in the parent hierarchy of the clip view.
      // If it is, then we know the drag is happening outside of the clip view,
      // so we may need to auto scroll.
      //

      // Get the frame's parent view.

      nsPoint viewOffset(0,0);

      nsIView *frameView = 0;

      nsIFrame *parentFrame = aFrame;

      while (NS_SUCCEEDED(result) && parentFrame && !frameView)
      {
        result = parentFrame->GetView(aPresContext, &frameView);
        if (NS_SUCCEEDED(result) && !frameView)
          result = parentFrame->GetParent(&parentFrame);
      }

      if (NS_SUCCEEDED(result) && frameView)
      {
        //
        // Now make sure that the frame's view is in the
        // scrollable view's parent hierarchy.
        //

        nsIView *view = (nsIView*)frameView;
        nscoord x, y;

        while (view && view != cView)
        {
          result = view->GetParent(view);

          if (NS_FAILED(result))
            view = 0;
          else if (view && view != cView) 
          {
            result = view->GetPosition(&x, &y);

            if (NS_FAILED(result))
              view = 0;
            else
            {
              //
              // Keep track of the view offsets so we can
              // translate aPoint into the scrollable view's
              // coordinate system.
              //

              viewOffset.x += x;
              viewOffset.y += y;
            }
          }
        }
        if (cView)
        {
          //
          // See if aPoint is outside the clip view's boundaries.
          // If it is, scroll the view till it is inside the visible area!
          //

          nsRect bounds;

          result = cView->GetBounds(bounds);
          scrollableView->GetScrollPosition(bounds.x,bounds.y);
          if (NS_SUCCEEDED(result))
          {
            //
            // Calculate the amount we would have to scroll in
            // the vertical and horizontal directions to get the point
            // within the clip area.
            //

            nscoord dx = 0, dy = 0;
            nsPoint ePoint = aPoint;

            ePoint.x -= viewOffset.x;
            ePoint.y -= viewOffset.y;
            
            nscoord x1 = bounds.x;
            nscoord x2 = bounds.x + bounds.width;
            nscoord y1 = bounds.y;
            nscoord y2 = bounds.y + bounds.height;

            if (ePoint.x < x1)
              dx = ePoint.x - x1;
            else if (ePoint.x > x2)
              dx = ePoint.x - x2;
                
            if (ePoint.y < y1)
              dy = ePoint.y - y1;
            else if (ePoint.y > y2)
              dy = ePoint.y - y2;

            //
            // Now clip the scroll amounts so that we don't scroll
            // beyond the ends of the document.
            //

            nscoord scrollX = 0, scrollY = 0;
            nscoord docWidth = 0, docHeight = 0;

            result = scrollableView->GetScrollPosition(scrollX, scrollY);

            if (NS_SUCCEEDED(result))
              result = scrollableView->GetContainerSize(&docWidth, &docHeight);

            if (NS_SUCCEEDED(result))
            {
              if (dx < 0 && scrollX == 0)
                dx = 0;
              else if (dx > 0)
              {
                x1 = scrollX + dx + bounds.width;

                if (x1 > docWidth)
                  dx -= x1 - docWidth;
              }


              if (dy < 0 && scrollY == 0)
                dy = 0;
              else if (dy > 0)
              {
                y1 = scrollY + dy + bounds.height;

                if (y1 > docHeight)
                  dy -= y1 - docHeight;
              }

              //
              // Now scroll the view if neccessary.
              //

              if (dx != 0 || dy != 0)
              {
// make sure latest bits are available before we scroll them.
//get the presshell
                nsCOMPtr<nsIPresShell> presShell;
                result = aPresContext->GetShell(getter_AddRefs(presShell));
                NS_ENSURE_TRUE(presShell,result);

//get viewmanager
                nsCOMPtr<nsIViewManager> viewManager;
                result = presShell->GetViewManager(getter_AddRefs(viewManager));
                NS_ENSURE_TRUE(viewManager,result);

                viewManager->Composite();
                result = scrollableView->ScrollTo(scrollX + dx, scrollY + dy, NS_VMREFRESH_NO_SYNC);
                if (mAutoScrollTimer)
                {
                  nsPoint point(aPoint.x + dx,aPoint.y + dy);
                  result = mAutoScrollTimer->Start(aPresContext, parentFrame, point);
#ifdef DEBUG_SELECTION
printf("point out of view: x=%u, y=%u\n", (point.x), (point.y));
#endif
                }
              }
            }
          }
        }
      }
    }
  }

  return result;
}

NS_IMETHODIMP
nsDOMSelection::GetEnumerator(nsIEnumerator **aIterator)
{
  nsresult status = NS_ERROR_OUT_OF_MEMORY;
  nsSelectionIterator *iterator =  new nsSelectionIterator(this);
  if ( iterator && !NS_SUCCEEDED(status = CallQueryInterface(iterator, aIterator)) )
  	delete iterator;
  return status;
}



/** ClearSelection zeroes the selection
 */
NS_IMETHODIMP
nsDOMSelection::ClearSelection()
{
  if (!mFrameSelection)
    return NS_OK;//nothing to do
  nsCOMPtr<nsIPresContext>  presContext;
  GetPresContext(getter_AddRefs(presContext));


  nsresult	result = Clear(presContext);
  if (NS_FAILED(result))
  	return result;
  
  // Turn off signal for table selection  	
  mFrameSelection->ClearTableCellSelection();

  return mFrameSelection->NotifySelectionListeners(GetType());
  // Also need to notify the frames!
  // PresShell::ContentChanged should do that on DocumentChanged
}

/** AddRange adds the specified range to the selection
 *  @param aRange is the range to be added
 */
NS_IMETHODIMP
nsDOMSelection::AddRange(nsIDOMRange* aRange)
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
  nsCOMPtr<nsIPresContext>  presContext;
  GetPresContext(getter_AddRefs(presContext));
  selectFrames(presContext, aRange, PR_TRUE);        
  //ScrollIntoView(); this should not happen automatically
  if (!mFrameSelection)
    return NS_OK;//nothing to do
  return mFrameSelection->NotifySelectionListeners(GetType());
}

NS_IMETHODIMP
nsDOMSelection::RemoveRange(nsIDOMRange* aRange)
{
  if (!aRange)
    return NS_ERROR_INVALID_ARG;
  RemoveItem(aRange);
  
  nsCOMPtr<nsIPresContext>  presContext;
  GetPresContext(getter_AddRefs(presContext));
  selectFrames(presContext, aRange, PR_FALSE);        
  if (aRange == mAnchorFocusRange.get())
  {
		PRUint32 cnt;
    nsresult rv = mRangeArray->Count(&cnt);
    if (NS_SUCCEEDED(rv) && cnt > 0 )
    {
      setAnchorFocusRange(cnt -1);//reset anchor to LAST range.
      ScrollIntoView();
    }
  }
  if (!mFrameSelection)
    return NS_OK;//nothing to do
  return mFrameSelection->NotifySelectionListeners(GetType());
}


/*
 * Collapse sets the whole selection to be one point.
 */
NS_IMETHODIMP
nsDOMSelection::Collapse(nsIDOMNode* aParentNode, PRInt32 aOffset)
{
  if (!aParentNode)
    return NS_ERROR_INVALID_ARG;

  nsresult result;
  // Delete all of the current ranges
  if (NS_FAILED(SetOriginalAnchorPoint(aParentNode,aOffset)))
    return NS_ERROR_FAILURE; //???
  nsCOMPtr<nsIPresContext>  presContext;
  GetPresContext(getter_AddRefs(presContext));
  Clear(presContext);

  // Turn off signal for table selection  	
  if (mFrameSelection)
    mFrameSelection->ClearTableCellSelection();

  nsCOMPtr<nsIDOMRange> range;
  NS_NewRange(getter_AddRefs(range));
  if (! range){
    NS_ASSERTION(PR_FALSE,"Couldn't make a range - nsSelection::Collapse");
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
    if (!content)
      return NS_ERROR_FAILURE;
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
  selectFrames(presContext, range,PR_TRUE);
  if (NS_FAILED(result))
    return result;
  if (!mFrameSelection)
   return NS_OK;//nothing to do
	return mFrameSelection->NotifySelectionListeners(GetType());
}

/*
 * Sets the whole selection to be one point
 * at the start of the current selection
 */
NS_IMETHODIMP
nsDOMSelection::CollapseToStart()
{
  PRInt32 cnt;
  nsresult rv = GetRangeCount(&cnt);
  if (NS_FAILED(rv) || cnt<=0 || !mRangeArray)
		return NS_ERROR_FAILURE;

  // Get the first range (from GetRangeAt)
	nsISupports*	element = mRangeArray->ElementAt(0);
	nsCOMPtr<nsIDOMRange>	firstRange = do_QueryInterface(element);
  if (!firstRange)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDOMNode> parent;
  rv = firstRange->GetStartParent(getter_AddRefs(parent));
  if (NS_SUCCEEDED(rv))
  {
    if (parent)
    {
      PRInt32 startOffset;
      firstRange->GetStartOffset(&startOffset);
      rv = Collapse(parent, startOffset);
    } else {
      // not very likely!
      rv = NS_ERROR_FAILURE;
    }
  }
  return rv;
}

/*
 * Sets the whole selection to be one point
 * at the end of the current selection
 */
NS_IMETHODIMP
nsDOMSelection::CollapseToEnd()
{
  PRInt32 cnt;
  nsresult rv = GetRangeCount(&cnt);
  if (NS_FAILED(rv) || cnt<=0 || !mRangeArray)
		return NS_ERROR_FAILURE;

  // Get the last range (from GetRangeAt)
	nsISupports*	element = mRangeArray->ElementAt(cnt-1);
	nsCOMPtr<nsIDOMRange>	lastRange = do_QueryInterface(element);
  if (!lastRange)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDOMNode> parent;
  rv = lastRange->GetEndParent(getter_AddRefs(parent));
  if (NS_SUCCEEDED(rv))
  {
    if (parent)
    {
      PRInt32 endOffset;
      lastRange->GetEndOffset(&endOffset);
      rv = Collapse(parent, endOffset);
    } else {
      // not very likely!
      rv = NS_ERROR_FAILURE;
    }
  }
  return rv;
}

/*
 * IsCollapsed -- is the whole selection just one point, or unset?
 */
NS_IMETHODIMP
nsDOMSelection::GetIsCollapsed(PRBool* aIsCollapsed)
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
nsDOMSelection::GetRangeCount(PRInt32* aRangeCount)
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
nsDOMSelection::GetRangeAt(PRInt32 aIndex, nsIDOMRange** aReturn)
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

#if OLD_SELECTION

//may change parameters may not.
//return NS_ERROR_FAILURE if invalid new selection between anchor and passed in parameters
NS_IMETHODIMP
nsDOMSelection::FixupSelectionPoints(nsIDOMRange *aRange , nsDirection *aDir, PRBool *aFixupState)
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
  if (atom.get() == nsSelection::sTbodyAtom)
    return NS_ERROR_FAILURE; //cannot select INTO row node ony cells

  //get common parent
  nsCOMPtr<nsIDOMNode> parent;
  nsCOMPtr<nsIDOMRange> subRange;
  NS_NewRange(getter_AddRefs(subRange));
  if (!subRange) return NS_ERROR_OUT_OF_MEMORY

  result = subRange->SetStart(startNode,startOffset);
  if (NS_FAILED(result))
    return result;
  result = subRange->SetEnd(endNode,endOffset);
  if (NS_FAILED(result))
  {
    result = subRange->SetEnd(startNode,startOffset);
    if (NS_FAILED(result))
      return result;
    result = subRange->SetStart(endNode,endOffset);
    if (NS_FAILED(result))
      return result;
  }

  res = subRange->GetCommonParent(getter_AddRefs(parent));
  if (NS_FAILED(res) || !parent)
    return res;
 
  //look for dest. if you see a cell you are in "cell mode"
  //if you see a table you select "whole" table

  //src first 
  nsCOMPtr<nsIDOMNode> tempNode;
  nsCOMPtr<nsIDOMNode> tempNode2;
  PRBool cellMode = PR_FALSE;
  PRBool dirtystart = PR_FALSE;
  PRBool dirtyend = PR_FALSE;
  if (startNode != endNode)
  {
    if (parent != startNode)
    {
      result = startNode->GetParentNode(getter_AddRefs(tempNode));
      if (NS_FAILED(result) || !tempNode)
        return NS_ERROR_FAILURE;
      while (tempNode != parent)
      {
        atom = GetTag(tempNode);
        if (atom.get() == nsSelection::sTableAtom) //select whole table  if in cell mode, wait for cell
        {
          result = ParentOffset(tempNode, getter_AddRefs(startNode), &startOffset);
          if (NS_FAILED(result))
            return NS_ERROR_FAILURE;
          if (*aDir == eDirPrevious) //select after
            startOffset++;
          dirtystart = PR_TRUE;
          cellMode = PR_FALSE;
        }
        else if (atom.get() == nsSelection::sCellAtom) //you are in "cell" mode put selection to end of cell
        {
          cellMode = PR_TRUE;
          result = ParentOffset(tempNode, getter_AddRefs(startNode), &startOffset);
          if (NS_FAILED(result))
            return result;
          if (*aDir == eDirPrevious) //select after
            startOffset++;
          dirtystart = PR_TRUE;
        }
        result = tempNode->GetParentNode(getter_AddRefs(tempNode2));
        if (NS_FAILED(result) || !tempNode2)
          return NS_ERROR_FAILURE;
        tempNode = tempNode2;
      }
    }
  
  //now for dest node
    if (parent != endNode)
    {
      result = endNode->GetParentNode(getter_AddRefs(tempNode));
      PRBool found = !cellMode;
      if (NS_FAILED(result) || !tempNode)
        return NS_ERROR_FAILURE;
      while (tempNode != parent)
      {
        atom = GetTag(tempNode);
        if (atom.get() == nsSelection::sTableAtom) //select whole table  if in cell mode, wait for cell
        {
          if (!cellMode)
          {
            result = ParentOffset(tempNode, getter_AddRefs(endNode), &endOffset);
            if (NS_FAILED(result))
              return result;
            if (*aDir == eDirNext) //select after
              endOffset++;
            dirtyend = PR_TRUE;
          }
          else
            found = PR_FALSE; //didnt find the right cell yet
        }
        else if (atom.get() == nsSelection::sCellAtom) //you are in "cell" mode put selection to end of cell
        {
          result = ParentOffset(tempNode, getter_AddRefs(endNode), &endOffset);
          if (NS_FAILED(result))
            return result;
          if (*aDir == eDirNext) //select after
            endOffset++;
          found = PR_TRUE;
          dirtyend = PR_TRUE;
        }
        result = tempNode->GetParentNode(getter_AddRefs(tempNode2));
        if (NS_FAILED(result) || !tempNode2)
          return NS_ERROR_FAILURE;
        tempNode = tempNode2;
      }
      if (!found)
        return NS_ERROR_FAILURE;
    }
  }
  if (*aDir == eDirNext)
  {
    if (FetchAnchorNode() == startNode.get() && FetchFocusNode() == endNode.get() &&
      FetchAnchorOffset() == startOffset && FetchFocusOffset() == endOffset)
    {
      *aFixupState = PR_FALSE;
      return NS_ERROR_FAILURE;//nothing to do
    }
  }
  else
  {
    if (FetchAnchorNode() == endNode.get() && FetchFocusNode() == startNode.get() &&
      FetchAnchorOffset() == endOffset && FetchFocusOffset() == startOffset)
    {
      *aFixupState = PR_FALSE;
      return NS_ERROR_FAILURE;//nothing to do
    }
  }
  if (mFixupState && !dirtyend && !dirtystart)//no mor fixup! all bets off
  {
    dirtystart = PR_TRUE;//force a reset of anchor positions
    dirtystart = PR_TRUE;
    *aFixupState = PR_TRUE;//redraw all selection here
    mFixupState = PR_FALSE;//no more fixup for next time
  }
  else
  if ((dirtystart || dirtyend) && *aDir != mDirection) //fixup took place but new direction all bets are off
  {
    *aFixupState = PR_TRUE;
    //mFixupState = PR_FALSE;
  }
  else
  if (dirtystart && (FetchAnchorNode() != startNode.get() || FetchAnchorOffset() != startOffset))
  {
    *aFixupState = PR_TRUE;
    mFixupState  = PR_TRUE;
  }
  else
  if (dirtyend && (FetchFocusNode() != endNode.get() || FetchFocusOffset() != endOffset))
  {
    *aFixupState = PR_TRUE;
    mFixupState  = PR_TRUE;
  }
  else
  {
    mFixupState = dirtystart || dirtyend;
    *aFixupState = PR_FALSE;
  }
  if (dirtystart || dirtyend){
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
#endif //OLD_SELECTION




NS_IMETHODIMP
nsDOMSelection::SetOriginalAnchorPoint(nsIDOMNode *aNode, PRInt32 aOffset)
{
  if (!aNode){
    mOriginalAnchorRange = 0;
    return NS_OK;
  }
  nsCOMPtr<nsIDOMRange> newRange;
  nsresult result;
  NS_NewRange(getter_AddRefs(newRange));
  if (!newRange) return NS_ERROR_OUT_OF_MEMORY;

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
nsDOMSelection::GetOriginalAnchorPoint(nsIDOMNode **aNode, PRInt32 *aOffset)
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
utility function
*/
NS_IMETHODIMP
nsDOMSelection::CopyRangeToAnchorFocus(nsIDOMRange *aRange)
{
  nsCOMPtr<nsIDOMNode> startNode;
  nsCOMPtr<nsIDOMNode> endNode;
  PRInt32 startOffset;
  PRInt32 endOffset;
  aRange->GetStartParent(getter_AddRefs(startNode));
  aRange->GetEndParent(getter_AddRefs(endNode));
  aRange->GetStartOffset(&startOffset);
  aRange->GetEndOffset(&endOffset);
  if (NS_FAILED(mAnchorFocusRange->SetStart(startNode,startOffset)))
  {
    if (NS_FAILED(mAnchorFocusRange->SetEnd(endNode,endOffset)))
      return NS_ERROR_FAILURE;//???
    if (NS_FAILED(mAnchorFocusRange->SetStart(startNode,startOffset)))
      return NS_ERROR_FAILURE;//???
  }
  else if (NS_FAILED(mAnchorFocusRange->SetEnd(endNode,endOffset)))
          return NS_ERROR_FAILURE;//???
  return NS_OK;
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
nsDOMSelection::Extend(nsIDOMNode* aParentNode, PRInt32 aOffset)
{
  if (!aParentNode)
    return NS_ERROR_INVALID_ARG;

  // First, find the range containing the old focus point:
  if (!mRangeArray || !mAnchorFocusRange)
    return NS_ERROR_NOT_INITIALIZED;
  //mFrameSelection->InvalidateDesiredX();
  nsCOMPtr<nsIDOMRange> difRange;
  nsresult res;
  NS_NewRange(getter_AddRefs(difRange));
  nsCOMPtr<nsIDOMRange> range;

  if (FetchFocusNode() ==  aParentNode && FetchFocusOffset() == aOffset)
    return NS_ERROR_FAILURE;//same node nothing to do!

  res = mAnchorFocusRange->Clone(getter_AddRefs(range));
  //range = mAnchorFocusRange;

  nsCOMPtr<nsIDOMNode> startNode;
  nsCOMPtr<nsIDOMNode> endNode;
  PRInt32 startOffset;
  PRInt32 endOffset;

  range->GetStartParent(getter_AddRefs(startNode));
  range->GetEndParent(getter_AddRefs(endNode));
  range->GetStartOffset(&startOffset);
  range->GetEndOffset(&endOffset);


  nsDirection dir = GetDirection();
  PRBool fixupState = PR_FALSE; //if there was a previous fixup the optimal drawing erasing will NOT work
  if (NS_FAILED(res))
    return res;

  NS_NewRange(getter_AddRefs(difRange));
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

  if (result2 == 0) //not selecting anywhere
    return NS_OK;

  nsCOMPtr<nsIPresContext>  presContext;
  GetPresContext(getter_AddRefs(presContext));
  if ((result1 == 0 && result3 < 0) || (result1 <= 0 && result2 < 0)){//a1,2  a,1,2
    //select from 1 to 2 unless they are collapsed
    res = range->SetEnd(aParentNode,aOffset);
    if (NS_FAILED(res))
      return res;
    dir = eDirNext;
    res = difRange->SetEnd(FetchEndParent(range), FetchEndOffset(range));
    res |= difRange->SetStart(FetchFocusNode(), FetchFocusOffset());
    if (NS_FAILED(res))
      return res;
#if OLD_SELECTION
    res = FixupSelectionPoints(range, &dir, &fixupState);
#endif
    if (NS_FAILED(res))
      return res;
    if (fixupState) 
    {
#if OLD_SELECTION
      selectFrames(mAnchorFocusRange, PR_FALSE);
      selectFrames(range, PR_TRUE);
#endif
    }
    else{
      selectFrames(presContext, difRange , PR_TRUE);
    }
    res = CopyRangeToAnchorFocus(range);
    if (NS_FAILED(res))
      return res;
  }
  else if (result1 == 0 && result3 > 0){//2, a1
    //select from 2 to 1a
    dir = eDirPrevious;
    res = range->SetStart(aParentNode,aOffset);
    if (NS_FAILED(res))
      return res;
#if OLD_SELECTION
    res = FixupSelectionPoints(range, &dir, &fixupState);
    if (NS_FAILED(res))
      return res;
    if (fixupState) //unselect previous and select new state has changed to not fixed up
    {
      selectFrames(mAnchorFocusRange, PR_FALSE);
      selectFrames(range, PR_TRUE);
    }
    else
#endif
      selectFrames(presContext, range, PR_TRUE);
    res = CopyRangeToAnchorFocus(range);
    if (NS_FAILED(res))
      return res;
  }
  else if (result3 <= 0 && result2 >= 0) {//a,2,1 or a2,1 or a,21 or a21
    //deselect from 2 to 1
    res = difRange->SetEnd(FetchFocusNode(), FetchFocusOffset());
    res |= difRange->SetStart(aParentNode, aOffset);
    if (NS_FAILED(res))
      return res;

    res = range->SetEnd(aParentNode,aOffset);
    if (NS_FAILED(res))
      return res;
#if OLD_SELECTION    
    dir = eDirNext;
    res = FixupSelectionPoints(range, &dir, &fixupState);
#endif
    if (NS_FAILED(res))
      return res;
    if (fixupState) //unselect previous and select new state has changed to not fixed up
    {
#if OLD_SELECTION    
      selectFrames(mAnchorFocusRange, PR_FALSE);
      selectFrames(range, PR_TRUE);
#endif
    }
    else 
    {
      res = CopyRangeToAnchorFocus(range);
      if (NS_FAILED(res))
        return res;
      selectFrames(presContext, difRange, 0);//deselect now if fixup succeeded
      difRange->SetEnd(FetchEndParent(range),FetchEndOffset(range));
      selectFrames(presContext, difRange, PR_TRUE);//must reselect last node maybe more if fixup did something
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
#if OLD_SELECTION
    res = FixupSelectionPoints(range, &dir, &fixupState);
    if (NS_FAILED(res))
      return res;

    if (fixupState) //unselect previous and select new state has changed to not fixed up
    {
      selectFrames(mAnchorFocusRange, PR_FALSE);
      selectFrames(range, PR_TRUE);
    }
    else 
#endif
    {
      if (FetchFocusNode() != FetchAnchorNode() || FetchFocusOffset() != FetchAnchorOffset() ){//if collapsed diff dont do anything
        res = difRange->SetStart(FetchFocusNode(), FetchFocusOffset());
        res |= difRange->SetEnd(FetchAnchorNode(), FetchAnchorOffset());
        if (NS_FAILED(res))
          return res;
        res = CopyRangeToAnchorFocus(range);
        if (NS_FAILED(res))
          return res;
        //deselect from 1 to a
        selectFrames(presContext, difRange , PR_FALSE);
      }
      else
      {
        res = CopyRangeToAnchorFocus(range);
        if (NS_FAILED(res))
          return res;
      }
      //select from a to 2
      selectFrames(presContext, range , PR_TRUE);
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

#if OLD_SELECTION
    res = FixupSelectionPoints(range, &dir, &fixupState);
#endif
    if (NS_FAILED(res))
      return res;
    if (fixupState) //unselect previous and select new state has changed to not fixed up
    {
#if OLD_SELECTION
      selectFrames(mAnchorFocusRange, PR_FALSE);
      selectFrames(range, PR_TRUE);
#endif
    }
    else 
    {
      res = CopyRangeToAnchorFocus(range);
      if (NS_FAILED(res))
        return res;
      selectFrames(presContext, difRange , PR_FALSE);
      difRange->SetStart(FetchStartParent(range),FetchStartOffset(range));
      selectFrames(presContext, difRange, PR_TRUE);//must reselect last node
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
#if OLD_SELECTION
    res = FixupSelectionPoints(range, &dir, &fixupState);
    if (NS_FAILED(res))
      return res;
    if (fixupState) //unselect previous and select new state has changed to not fixed up
    {
      selectFrames(mAnchorFocusRange, PR_FALSE);
      selectFrames(range, PR_TRUE);
    }
    else
#endif
    {
      //deselect from a to 1
      if (FetchFocusNode() != FetchAnchorNode() || FetchFocusOffset() != FetchAnchorOffset() ){//if collapsed diff dont do anything
        res = difRange->SetStart(FetchAnchorNode(), FetchAnchorOffset());
        res |= difRange->SetEnd(FetchFocusNode(), FetchFocusOffset());
        res = CopyRangeToAnchorFocus(range);
        if (NS_FAILED(res))
          return res;
        selectFrames(presContext, difRange, 0);
      }
      else
      {
        res = CopyRangeToAnchorFocus(range);
        if (NS_FAILED(res))
          return res;
      }
      //select from 2 to a
      selectFrames(presContext, range , PR_TRUE);
    }
  }
  else if (result2 >= 0 && result1 >= 0) {//2,1,a or 21,a or 2,1a or 21a
    //select from 2 to 1
    res = range->SetStart(aParentNode,aOffset);
    if (NS_FAILED(res))
      return res;
    dir = eDirPrevious;
    res = difRange->SetEnd(FetchFocusNode(), FetchFocusOffset());
    res |= difRange->SetStart(FetchStartParent(range), FetchStartOffset(range));
    if (NS_FAILED(res))
      return res;

#if OLD_SELECTION
    res = FixupSelectionPoints(range, &dir, &fixupState);
#endif
    if (NS_FAILED(res))
      return res;
    if (fixupState) //unselect previous and select new state has changed to not fixed up
    {
#if OLD_SELECTION
      selectFrames(mAnchorFocusRange, PR_FALSE);
      selectFrames(range, PR_TRUE);
#endif
    }
    else {
      selectFrames(presContext, difRange, PR_TRUE);
    }
    res = CopyRangeToAnchorFocus(range);
    if (NS_FAILED(res))
      return res;

  }

  DEBUG_OUT_RANGE(range);
#if 0
  if (eDirNext == mDirection)
    printf("    direction = 1  LEFT TO RIGHT\n");
  else
    printf("    direction = 0  RIGHT TO LEFT\n");
#endif
  SetDirection(dir);
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
	    printf ("Sel. Extend to %p %s %d\n", content, tagCString, aOffset);
	    delete [] tagCString;
    }
  }
  else {
    printf ("Sel. Extend set to null parent.\n");
  }
#endif
  if (!mFrameSelection)
    return NS_OK;//nothing to do
  return mFrameSelection->NotifySelectionListeners(GetType());
}

NS_IMETHODIMP
nsDOMSelection::ContainsNode(nsIDOMNode* aNode, PRBool aRecursive, PRBool* aYes)
{
  if (!aYes)
    return NS_ERROR_NULL_POINTER;
  *aYes = PR_FALSE;

  // Iterate over the ranges in the selection checking for the content:
  if (!mRangeArray)
    return NS_OK;

  PRUint32 cnt;
  nsresult rv = mRangeArray->Count(&cnt);
  if (NS_FAILED(rv))
    return rv;
  for (PRUint32 i=0; i < cnt; ++i)
  {
    nsCOMPtr<nsISupports> element = dont_AddRef(mRangeArray->ElementAt(i));
    nsCOMPtr<nsIDOMRange>	range = do_QueryInterface(element);
    if (!range)
      return NS_ERROR_UNEXPECTED;

    nsCOMPtr<nsIContent> content (do_QueryInterface(aNode));
    if (content)
    {
      if (IsNodeIntersectsRange(content, range))
      {
        // If recursive, then we're done -- IsNodeIntersectsRange does the right thing
        if (aRecursive)
        {
          *aYes = PR_TRUE;
          return NS_OK;
        }

        // else not recursive -- node itself must be contained,
        // so we need to do more checking
        PRBool nodeStartsBeforeRange, nodeEndsAfterRange;
        if (NS_SUCCEEDED(CompareNodeToRange(content, range,
                                            &nodeStartsBeforeRange,
                                            &nodeEndsAfterRange)))
        {
#ifdef DEBUG_ContainsNode
          nsAutoString name, value;
          aNode->GetNodeName(name);
          aNode->GetNodeValue(value);
          printf("%s [%s]: %d, %d\n", name.ToNewCString(), value.ToNewCString(),
                 nodeStartsBeforeRange, nodeEndsAfterRange);
#endif
          PRUint16 nodeType;
          aNode->GetNodeType(&nodeType);
          if ((!nodeStartsBeforeRange && !nodeEndsAfterRange)
              || (nodeType == nsIDOMNode::TEXT_NODE))
          {
            *aYes = PR_TRUE;
            return NS_OK;
          }
        }
      }
    }
  }
  return NS_OK;
}

nsresult
nsDOMSelection::GetPresContext(nsIPresContext **aPresContext)
{
  if (!mFrameSelection)
    return NS_ERROR_FAILURE;//nothing to do
  nsIFocusTracker *tracker = mFrameSelection->GetTracker();

  if (!tracker)
    return NS_ERROR_NULL_POINTER;

  return tracker->GetPresContext(aPresContext);
}

nsresult
nsDOMSelection::GetPresShell(nsIPresShell **aPresShell)
{
  if (mPresShellWeak)
  {
    nsCOMPtr<nsIPresShell> presShell = do_QueryReferent(mPresShellWeak);
    if (presShell)
      NS_ADDREF(*aPresShell = presShell);
    return NS_OK;
  }
  nsresult rv = NS_OK;
  if (!mFrameSelection)
    return NS_ERROR_FAILURE;//nothing to do

  nsIFocusTracker *tracker = mFrameSelection->GetTracker();

  if (!tracker)
    return NS_ERROR_NULL_POINTER;

  nsCOMPtr<nsIPresContext> presContext;

  rv = tracker->GetPresContext(getter_AddRefs(presContext));

  if (NS_FAILED(rv))
    return rv;

  if (!presContext)
    return NS_ERROR_NULL_POINTER;
  
  nsCOMPtr<nsIPresShell> shell;
  rv = presContext->GetShell(getter_AddRefs(shell));
	mPresShellWeak = getter_AddRefs(NS_GetWeakReference(shell));		// the presshell owns us, so no addref
  if (mPresShellWeak)
    NS_ADDREF(*aPresShell = shell);
  return rv;
}

nsresult
nsDOMSelection::GetRootScrollableView(nsIScrollableView **aScrollableView)
{
  //
  // NOTE: This method returns a NON-AddRef'd pointer
  //       to the scrollable view!
  //
  NS_ENSURE_ARG_POINTER(aScrollableView);

  if (!mFrameSelection)
    return NS_ERROR_FAILURE;//nothing to do
  nsresult rv = NS_OK;
  nsIScrollableView *scrollView = mFrameSelection->GetScrollView();
  if (!scrollView)
  {

    nsCOMPtr<nsIPresShell> presShell;

    rv = GetPresShell(getter_AddRefs(presShell));

    if (NS_FAILED(rv))
      return rv;

    if (!presShell)
      return NS_ERROR_NULL_POINTER;

    nsCOMPtr<nsIViewManager> viewManager;

    rv = presShell->GetViewManager(getter_AddRefs(viewManager));

    if (NS_FAILED(rv))
      return rv;

    if (!viewManager)
      return NS_ERROR_NULL_POINTER;

    //
    // nsIViewManager::GetRootScrollableView() does not
    // AddRef the pointer it returns.
    //
    return viewManager->GetRootScrollableView(aScrollableView);
  }
  else //SCROLLVIEW_FROM_FRAME
  {
    *aScrollableView = scrollView;
  }
  return NS_OK;
}

nsresult
nsDOMSelection::GetFrameToRootViewOffset(nsIFrame *aFrame, nscoord *aX, nscoord *aY)
{
  nsresult rv = NS_OK;
  if (!mFrameSelection)
    return NS_ERROR_FAILURE;//nothing to do

  if (!aFrame || !aX || !aY) {
    return NS_ERROR_NULL_POINTER;
  }

  *aX = 0;
  *aY = 0;

  nsIScrollableView* scrollingView = 0;

  rv = GetRootScrollableView(&scrollingView);

  if (NS_FAILED(rv))
    return rv;

  if (!scrollingView)
    return NS_ERROR_NULL_POINTER;

  nsIView*  scrolledView;
  nsPoint   offset;
  nsIView*  closestView;
          
  // Determine the offset from aFrame to the scrolled view. We do that by
  // getting the offset from its closest view and then walking up
  scrollingView->GetScrolledView(scrolledView);
  nsIFocusTracker *tracker = mFrameSelection->GetTracker();

  if (!tracker)
    return NS_ERROR_NULL_POINTER;

  nsCOMPtr<nsIPresContext> presContext;
  tracker->GetPresContext(getter_AddRefs(presContext));
  aFrame->GetOffsetFromView(presContext, offset, &closestView);

  // XXX Deal with the case where there is a scrolled element, e.g., a
  // DIV in the middle...
  while ((closestView != nsnull) && (closestView != scrolledView)) {
    nscoord dx, dy;

    // Update the offset
    closestView->GetPosition(&dx, &dy);
    offset.MoveBy(dx, dy);

    // Get its parent view
    closestView->GetParent(closestView);
  }

  *aX = offset.x;
  *aY = offset.y;

  return rv;
}

nsresult
nsDOMSelection::GetPointFromOffset(nsIFrame *aFrame, PRInt32 aContentOffset, nsPoint *aPoint)
{
  nsresult rv = NS_OK;
  if (!mFrameSelection)
    return NS_ERROR_FAILURE;//nothing to do
  if (!aFrame || !aPoint)
    return NS_ERROR_NULL_POINTER;

  aPoint->x = 0;
  aPoint->y = 0;

  //
  // Retrieve the device context. We need one to create
  // a rendering context.
  //

  nsIFocusTracker *tracker = mFrameSelection->GetTracker();

  if (!tracker)
    return NS_ERROR_NULL_POINTER;

  nsCOMPtr<nsIPresContext> presContext;

  rv = tracker->GetPresContext(getter_AddRefs(presContext));

  if (NS_FAILED(rv))
    return rv;

  if (!presContext)
    return NS_ERROR_NULL_POINTER;
  
  nsCOMPtr<nsIDeviceContext> deviceContext;

	rv = presContext->GetDeviceContext(getter_AddRefs(deviceContext));

	if (NS_FAILED(rv))
		return rv;

  if (!deviceContext)
		return NS_ERROR_NULL_POINTER;

  //
  // Now get the closest view with a widget so we can create
  // a rendering context.
  //

  nsCOMPtr<nsIWidget> widget;
  nsIView *closestView = 0;
  nsPoint offset(0, 0);

  rv = aFrame->GetOffsetFromView(presContext, offset, &closestView);

  while (!widget && closestView)
  {
    rv = closestView->GetWidget(*getter_AddRefs(widget));

    if (NS_FAILED(rv))
      return rv;

    if (!widget)
    {
      rv = closestView->GetParent(closestView);

      if (NS_FAILED(rv))
        return rv;
    }
  }

  if (!closestView)
    return NS_ERROR_FAILURE;

  //
  // Create a rendering context. This context is used by text frames
  // to calculate text widths so it can figure out where the point is
  // in the frame.
  //

	nsCOMPtr<nsIRenderingContext> rendContext;

	rv = deviceContext->CreateRenderingContext(closestView, *getter_AddRefs(rendContext));		
  
	if (NS_FAILED(rv))
		return rv;

  if (!rendContext)
		return NS_ERROR_NULL_POINTER;

  //
  // Now get the point and return!
  //

	rv = aFrame->GetPointFromOffset(presContext, rendContext, aContentOffset, aPoint);

  return rv;
}

nsresult
nsDOMSelection::GetSelectionRegionRect(SelectionRegion aRegion, nsRect *aRect)
{
  nsresult result = NS_OK;
  if (!mFrameSelection)
    return NS_ERROR_FAILURE;//nothing to do

  if (!aRect)
    return NS_ERROR_NULL_POINTER;

  // Init aRect:

  aRect->x = 0;
  aRect->y = 0;
  aRect->width  = 0;
  aRect->height = 0;

  nsIDOMNode *node       = 0;
  PRInt32     nodeOffset = 0;
  PRBool      isEndNode  = PR_TRUE;
  nsIFrame   *frame      = 0;

  switch (aRegion)
  {
  case nsISelectionController::SELECTION_ANCHOR_REGION:
    node       = FetchAnchorNode();
    nodeOffset = FetchAnchorOffset();
    isEndNode  = GetDirection() == eDirPrevious;
    break;
  case nsISelectionController::SELECTION_FOCUS_REGION:
    node       = FetchFocusNode();
    nodeOffset = FetchFocusOffset();
    isEndNode  = GetDirection() == eDirNext;
    break;
  default:
    return NS_ERROR_FAILURE;
  }

  if (!node)
    return NS_ERROR_NULL_POINTER;

  nsCOMPtr<nsIContent> content = do_QueryInterface(node);
  PRInt32 frameOffset = 0;

  if (content)
  {
    nsIFrameSelection::HINT hint;
    mFrameSelection->GetHint(&hint);
    result = mFrameSelection->GetFrameForNodeOffset(content, nodeOffset, hint, &frame, &frameOffset);
  }
  else
    result = NS_ERROR_FAILURE;
  if(NS_FAILED(result))
    return result;

  if (!frame)
    return NS_ERROR_NULL_POINTER;

  PRUint16 nodeType = nsIDOMNode::ELEMENT_NODE;

  result = node->GetNodeType(&nodeType);

  if (NS_FAILED(result))
    return NS_ERROR_NULL_POINTER;

  if (nodeType == nsIDOMNode::TEXT_NODE)
  {
    nsIFrame *childFrame = 0;
    frameOffset  = 0;

    result = frame->GetChildFrameContainingOffset(nodeOffset, mFrameSelection->mHint, &frameOffset, &childFrame);

    if (NS_FAILED(result))
      return result;

    if (!childFrame)
      return NS_ERROR_NULL_POINTER;

    frame = childFrame;

    //
    // Get the x coordinate of the offset into the text frame.
    // The x coordinate will be relative to the frame's origin,
    // so we'll have to translate it into the root view's coordinate
    // system.
    //
    nsPoint pt;

    result = GetPointFromOffset(frame, nodeOffset, &pt);

    if (NS_FAILED(result))
      return result;
    
    //
    // Get the frame's rect.
    //
    result = frame->GetRect(*aRect);

    if (NS_FAILED(result))
      return result;

    //
    // Translate the frame's rect into root view coordinates.
    //
    result = GetFrameToRootViewOffset(frame, &aRect->x, &aRect->y);

    if (NS_FAILED(result))
      return result;

    //
    // Now add the offset's x coordinate.
    //
    aRect->x += pt.x;

    //
    // Adjust the width of the rect to account for any neccessary
    // padding!
    //

    nsIScrollableView *scrollingView = 0;

    result = GetRootScrollableView(&scrollingView);

    if (NS_FAILED(result))
      return result;

    const nsIView* clipView = 0;
    nsRect clipRect;

    result = scrollingView->GetScrollPosition(clipRect.x, clipRect.y);

    if (NS_FAILED(result))
      return result;

    result = scrollingView->GetClipView(&clipView);

    if (NS_FAILED(result))
      return result;

    result = clipView->GetDimensions(&clipRect.width, &clipRect.height);

    if (NS_FAILED(result))
      return result;

    // If the point we are interested is outside the clip
    // region, we will scroll it into view with a padding
    // equal to a quarter of the clip's width.

    PRInt32 pad = clipRect.width >> 2;

    if (pad <= 0)
      pad = 3; // Arbitrary

    if (aRect->x >= clipRect.XMost())
      aRect->width = pad;
    else if (aRect->x <= clipRect.x)
    {
      aRect->x -= pad;
      aRect->width = pad;
    }
    else
      aRect->width = 60; // Arbitrary
  }
  else
  {
    //
    // Must be a non-text frame, just scroll the frame
    // into view.
    //
    result = frame->GetRect(*aRect);

    if (NS_FAILED(result))
      return result;

    result = GetFrameToRootViewOffset(frame, &aRect->x, &aRect->y);
  }

  return result;
}

nsresult
nsDOMSelection::ScrollRectIntoView(nsRect& aRect,
                              PRIntn  aVPercent, 
                              PRIntn  aHPercent)
{
  nsresult rv = NS_OK;
  if (!mFrameSelection)
    return NS_OK;//nothing to do

  nsIScrollableView *scrollingView = 0;

  rv = GetRootScrollableView(&scrollingView);

  if (NS_FAILED(rv))
    return rv;

  if (! scrollingView)
    return NS_ERROR_NULL_POINTER;

  // Determine the visible rect in the scrolled view's coordinate space.
  // The size of the visible area is the clip view size
  const nsIView*  clipView;
  nsRect          visibleRect;

  scrollingView->GetScrollPosition(visibleRect.x, visibleRect.y);
  scrollingView->GetClipView(&clipView);
  clipView->GetDimensions(&visibleRect.width, &visibleRect.height);

  // The actual scroll offsets
  nscoord scrollOffsetX = visibleRect.x;
  nscoord scrollOffsetY = visibleRect.y;

  // See how aRect should be positioned vertically
  if (NS_PRESSHELL_SCROLL_ANYWHERE == aVPercent) {
    // The caller doesn't care where aRect is positioned vertically,
    // so long as it's fully visible
    if (aRect.y < visibleRect.y) {
      // Scroll up so aRect's top edge is visible
      scrollOffsetY = aRect.y;
    } else if (aRect.YMost() > visibleRect.YMost()) {
      // Scroll down so aRect's bottom edge is visible. Make sure
      // aRect's top edge is still visible
      scrollOffsetY += aRect.YMost() - visibleRect.YMost();
      if (scrollOffsetY > aRect.y) {
        scrollOffsetY = aRect.y;
      }
    }
  } else {
    // Align the aRect edge according to the specified percentage
    nscoord frameAlignY = aRect.y + (aRect.height * aVPercent) / 100;
    scrollOffsetY = frameAlignY - (visibleRect.height * aVPercent) / 100;
  }

  // See how the aRect should be positioned horizontally
  if (NS_PRESSHELL_SCROLL_ANYWHERE == aHPercent) {
    // The caller doesn't care where the aRect is positioned horizontally,
    // so long as it's fully visible
    if (aRect.x < visibleRect.x) {
      // Scroll left so the aRect's left edge is visible
      scrollOffsetX = aRect.x;
    } else if (aRect.XMost() > visibleRect.XMost()) {
      // Scroll right so the aRect's right edge is visible. Make sure the
      // aRect's left edge is still visible
      scrollOffsetX += aRect.XMost() - visibleRect.XMost();
      if (scrollOffsetX > aRect.x) {
        scrollOffsetX = aRect.x;
      }
    }
      
  } else {
    // Align the aRect edge according to the specified percentage
    nscoord frameAlignX = aRect.x + (aRect.width * aHPercent) / 100;
    scrollOffsetX = frameAlignX - (visibleRect.width * aHPercent) / 100;
  }
      
  scrollingView->ScrollTo(scrollOffsetX, scrollOffsetY, NS_VMREFRESH_IMMEDIATE);

  return rv;
}

NS_IMETHODIMP
nsDOMSelection::ScrollIntoView(SelectionRegion aRegion)
{
  nsresult result;
  if (!mFrameSelection)
    return NS_OK;//nothing to do

  if (mFrameSelection->GetBatching())
    return NS_OK;

  //
  // Shut the caret off before scrolling to avoid
  // leaving caret turds on the screen!
  //
  nsCOMPtr<nsIPresShell> presShell;
  result = GetPresShell(getter_AddRefs(presShell));
  if (NS_FAILED(result))
    return result;
  nsCOMPtr<nsICaret> caret;
  presShell->GetCaret(getter_AddRefs(caret));
  if (caret)
  {
    StCaretHider  caretHider(caret);			// stack-based class hides and shows the caret

    //
    // Scroll the selection region into view.
    //
    nsRect rect;
    result = GetSelectionRegionRect(aRegion, &rect);

    if (NS_FAILED(result))
      return result;

    result = ScrollRectIntoView(rect, NS_PRESSHELL_SCROLL_ANYWHERE, NS_PRESSHELL_SCROLL_ANYWHERE);
  }
  return result;
}



NS_IMETHODIMP
nsDOMSelection::AddSelectionListener(nsIDOMSelectionListener* aNewListener)
{
  if (!mSelectionListeners)
    return NS_ERROR_FAILURE;
  if (!aNewListener)
    return NS_ERROR_NULL_POINTER;
  nsresult result;
  nsCOMPtr<nsISupports> isupports = do_QueryInterface(aNewListener , &result);
  if (NS_SUCCEEDED(result))
    result = mSelectionListeners->AppendElement(isupports) ? NS_OK : NS_ERROR_FAILURE;		// addrefs
  return result;
}



NS_IMETHODIMP
nsDOMSelection::RemoveSelectionListener(nsIDOMSelectionListener* aListenerToRemove)
{
  if (!mSelectionListeners)
    return NS_ERROR_FAILURE;
  if (!aListenerToRemove )
    return NS_ERROR_NULL_POINTER;
  nsCOMPtr<nsISupports> isupports = do_QueryInterface(aListenerToRemove);
  return mSelectionListeners->RemoveElement(isupports) ? NS_OK : NS_ERROR_FAILURE;		// releases
}


nsresult
nsDOMSelection::NotifySelectionListeners()
{
  if (!mSelectionListeners)
    return NS_ERROR_FAILURE;
  if (!mFrameSelection)
    return NS_OK;//nothing to do
 
  if (mFrameSelection->GetBatching()){
    mFrameSelection->SetDirty();
    return NS_OK;
  }
  PRUint32 cnt;
  nsresult rv = mSelectionListeners->Count(&cnt);
  if (NS_FAILED(rv)) return rv;
  
  nsCOMPtr<nsIDOMDocument> domdoc;
  nsCOMPtr<nsIDocument> doc;
  nsCOMPtr<nsIPresShell> shell;
  rv = GetPresShell(getter_AddRefs(shell));
  if (NS_SUCCEEDED(rv) && shell)
  {
    rv = shell->GetDocument(getter_AddRefs(doc));
    if (NS_FAILED(rv))
      doc = 0;
    domdoc = do_QueryInterface(doc);
  }
  short reason = mFrameSelection->PopReason();
  for (PRUint32 i = 0; i < cnt;i++)
  {
    nsCOMPtr<nsISupports> isupports(dont_AddRef(mSelectionListeners->ElementAt(i)));
    nsCOMPtr<nsIDOMSelectionListener> thisListener = do_QueryInterface(isupports);
    if (thisListener)
    	thisListener->NotifySelectionChanged(domdoc,this, reason);
  }
	return NS_OK;
}

NS_IMETHODIMP
nsDOMSelection::StartBatchChanges()
{
  if (!mFrameSelection)
    return NS_OK;//nothing to do
  return mFrameSelection->StartBatchChanges();
}



NS_IMETHODIMP
nsDOMSelection::EndBatchChanges()
{
  if (!mFrameSelection)
    return NS_OK;//nothing to do
  return mFrameSelection->EndBatchChanges();
}



NS_IMETHODIMP
nsDOMSelection::DeleteFromDocument()
{
  if (!mFrameSelection)
    return NS_OK;//nothing to do
  return mFrameSelection->DeleteFromDocument();
}

// BEGIN nsIScriptObjectOwner interface implementations
NS_IMETHODIMP
nsDOMSelection::GetScriptObject(nsIScriptContext *aContext, void** aScriptObject)
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
nsDOMSelection::SetScriptObject(void *aScriptObject)
{
  mScriptObject = aScriptObject;
  return NS_OK;
}

// END nsIScriptObjectOwner interface implementations
