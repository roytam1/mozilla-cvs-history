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
#include "nsCOMPtr.h"
#include "nsFrame.h"
#include "nsFrameList.h"
#include "nsLineLayout.h"
#include "nsIContent.h"
#include "nsIAtom.h"
#include "nsString.h"
#include "nsIStyleContext.h"
#include "nsIView.h"
#include "nsIViewManager.h"
#include "nsIPresContext.h"
#include "nsCRT.h"
#include "nsGUIEvent.h"
#include "nsDOMEvent.h"
#include "nsStyleConsts.h"
#include "nsIPresShell.h"
#include "prlog.h"
#include "prprf.h"
#include <stdarg.h>
#include "nsIPtr.h"
#include "nsISizeOfHandler.h"

#include "nsIDOMText.h"
#include "nsDocument.h"
#include "nsIDeviceContext.h"
#include "nsHTMLIIDs.h"
#include "nsIEventStateManager.h"
#include "nsIDOMSelection.h"
#include "nsIFrameSelection.h"
#include "nsHTMLParts.h"
#include "nsLayoutAtoms.h"

#include "nsFrameTraversal.h"
#include "nsCOMPtr.h"
#include "nsStyleChangeList.h"
#include "nsIDOMRange.h"


#define NORMAL_DRAG_HANDLING 1  // remove this to simulate a start-drag event.
#if !NORMAL_DRAG_HANDLING
#include "nsWidgetsCID.h"
#include "nsIDragService.h"
#include "nsIDragSession.h"
#include "nsITransferable.h"
#include "nsISupportsArray.h"
#include "nsIServiceManager.h"

// Define Class IDs -- i hate having to do this
static NS_DEFINE_IID(kCDragServiceCID,  NS_DRAGSERVICE_CID);
static NS_DEFINE_IID(kCTransferableCID,  NS_TRANSFERABLE_CID);
#endif


// Some Misc #defines
#define SELECTION_DEBUG        0
#define FORCE_SELECTION_UPDATE 1
#define CALC_DEBUG             0


#include "nsICaret.h"
#include "nsILineIterator.h"
// [HACK] Foward Declarations
void ForceDrawFrame(nsFrame * aFrame);
//non Hack prototypes
static void GetLastLeaf(nsIFrame **aFrame);
#if 0
static void GetFirstLeaf(nsIFrame **aFrame);
static void RefreshContentFrames(nsIPresContext& aPresContext, nsIContent * aStartContent, nsIContent * aEndContent);
#endif



//----------------------------------------------------------------------

static PRBool gShowFrameBorders = PR_FALSE;

NS_LAYOUT void nsIFrame::ShowFrameBorders(PRBool aEnable)
{
  gShowFrameBorders = aEnable;
}

NS_LAYOUT PRBool nsIFrame::GetShowFrameBorders()
{
  return gShowFrameBorders;
}

/**
 * Note: the log module is created during library initialization which
 * means that you cannot perform logging before then.
 */
static PRLogModuleInfo* gLogModule;

#ifdef NS_DEBUG
static PRLogModuleInfo* gFrameVerifyTreeLogModuleInfo;
#endif

static PRBool gFrameVerifyTreeEnable = PRBool(0x55);

NS_LAYOUT PRBool
nsIFrame::GetVerifyTreeEnable()
{
#ifdef NS_DEBUG
  if (gFrameVerifyTreeEnable == PRBool(0x55)) {
    if (nsnull == gFrameVerifyTreeLogModuleInfo) {
      gFrameVerifyTreeLogModuleInfo = PR_NewLogModule("frameverifytree");
      gFrameVerifyTreeEnable = 0 != gFrameVerifyTreeLogModuleInfo->level;
      printf("Note: frameverifytree is %sabled\n",
             gFrameVerifyTreeEnable ? "en" : "dis");
    }
  }
#endif
  return gFrameVerifyTreeEnable;
}

NS_LAYOUT void
nsIFrame::SetVerifyTreeEnable(PRBool aEnabled)
{
  gFrameVerifyTreeEnable = aEnabled;
}

#ifdef NS_DEBUG
static PRLogModuleInfo* gStyleVerifyTreeLogModuleInfo;
#endif

static PRBool gStyleVerifyTreeEnable = PRBool(0x55);

NS_LAYOUT PRBool
nsIFrame::GetVerifyStyleTreeEnable()
{
#ifdef NS_DEBUG
  if (gStyleVerifyTreeEnable == PRBool(0x55)) {
    if (nsnull == gStyleVerifyTreeLogModuleInfo) {
      gStyleVerifyTreeLogModuleInfo = PR_NewLogModule("styleverifytree");
      gStyleVerifyTreeEnable = 0 != gStyleVerifyTreeLogModuleInfo->level;
      printf("Note: styleverifytree is %sabled\n",
             gStyleVerifyTreeEnable ? "en" : "dis");
    }
  }
#endif
  return gStyleVerifyTreeEnable;
}

NS_LAYOUT void
nsIFrame::SetVerifyStyleTreeEnable(PRBool aEnabled)
{
  gStyleVerifyTreeEnable = aEnabled;
}

NS_LAYOUT PRLogModuleInfo*
nsIFrame::GetLogModuleInfo()
{
  if (nsnull == gLogModule) {
    gLogModule = PR_NewLogModule("frame");
  }
  return gLogModule;
}

//----------------------------------------------------------------------

static NS_DEFINE_IID(kIFrameIID, NS_IFRAME_IID);
static NS_DEFINE_IID(kIFrameSelection, NS_IFRAMESELECTION_IID);
nsresult
NS_NewEmptyFrame(nsIFrame** aNewFrame)
{
  NS_PRECONDITION(aNewFrame, "null OUT ptr");
  if (nsnull == aNewFrame) {
    return NS_ERROR_NULL_POINTER;
  }
  nsFrame* it = new nsFrame;
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  *aNewFrame = it;
  return NS_OK;
}

NS_IMPL_ZEROING_OPERATOR_NEW(nsFrame)

MOZ_DECL_CTOR_COUNTER(nsFrame);

nsFrame::nsFrame()
{
  MOZ_COUNT_CTOR(nsFrame);

  mState = NS_FRAME_FIRST_REFLOW | NS_FRAME_SYNC_FRAME_AND_VIEW |
    NS_FRAME_IS_DIRTY;
}

nsFrame::~nsFrame()
{
  MOZ_COUNT_DTOR(nsFrame);

  NS_IF_RELEASE(mContent);
  NS_IF_RELEASE(mStyleContext);
  if (nsnull != mView) {
    // Break association between view and frame
    mView->Destroy();
    mView = nsnull;
  }
}

/////////////////////////////////////////////////////////////////////////////
// nsISupports

nsresult nsFrame::QueryInterface(const nsIID& aIID, void** aInstancePtr)
{
  if (NULL == aInstancePtr) {
    return NS_ERROR_NULL_POINTER;
  }
  static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);
  static NS_DEFINE_IID(kClassIID, kIFrameIID);
  if (aIID.Equals(kIHTMLReflowIID)) {
    *aInstancePtr = (void*)(nsIHTMLReflow*)this;
    return NS_OK;
  } else if (aIID.Equals(kClassIID) || aIID.Equals(kISupportsIID)) {
    *aInstancePtr = (void*)this;
    return NS_OK;
  }
  return NS_NOINTERFACE;
}

nsrefcnt nsFrame::AddRef(void)
{
  NS_WARNING("not supported for frames");
  return 1;
}

nsrefcnt nsFrame::Release(void)
{
  NS_WARNING("not supported for frames");
  return 1;
}

/////////////////////////////////////////////////////////////////////////////
// nsIFrame

NS_IMETHODIMP
nsFrame::Init(nsIPresContext&  aPresContext,
              nsIContent*      aContent,
              nsIFrame*        aParent,
              nsIStyleContext* aContext,
              nsIFrame*        aPrevInFlow)
{
  mContent = aContent;
  NS_IF_ADDREF(mContent);
  mParent = aParent;
  return SetStyleContext(&aPresContext, aContext);
}

NS_IMETHODIMP nsFrame::SetInitialChildList(nsIPresContext& aPresContext,
                                           nsIAtom*        aListName,
                                           nsIFrame*       aChildList)
{
  // XXX This shouldn't be getting called at all, but currently is for backwards
  // compatility reasons...
#if 0
  NS_ERROR("not a container");
  return NS_ERROR_UNEXPECTED;
#else
  NS_ASSERTION(nsnull == aChildList, "not a container");
  return NS_OK;
#endif
}

NS_IMETHODIMP
nsFrame::AppendFrames(nsIPresContext& aPresContext,
                      nsIPresShell&   aPresShell,
                      nsIAtom*        aListName,
                      nsIFrame*       aFrameList)
{
  NS_PRECONDITION(PR_FALSE, "not a container");
  return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP
nsFrame::InsertFrames(nsIPresContext& aPresContext,
                      nsIPresShell&   aPresShell,
                      nsIAtom*        aListName,
                      nsIFrame*       aPrevFrame,
                      nsIFrame*       aFrameList)
{
  NS_PRECONDITION(PR_FALSE, "not a container");
  return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP
nsFrame::RemoveFrame(nsIPresContext& aPresContext,
                     nsIPresShell&   aPresShell,
                     nsIAtom*        aListName,
                     nsIFrame*       aOldFrame)
{
  NS_PRECONDITION(PR_FALSE, "not a container");
  return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP
nsFrame::ReplaceFrame(nsIPresContext& aPresContext,
                      nsIPresShell&   aPresShell,
                      nsIAtom*        aListName,
                      nsIFrame*       aOldFrame,
                      nsIFrame*       aNewFrame)
{
  NS_PRECONDITION(PR_FALSE, "not a container");
  return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP
nsFrame::Destroy(nsIPresContext& aPresContext)
{
  nsCOMPtr<nsIPresShell> shell;
  aPresContext.GetShell(getter_AddRefs(shell));
  
  // XXX Rather than always doing this it would be better if it was part of
  // a frame observer mechanism and the pres shell could register as an
  // observer of the frame while the reflow command is pending...
  if (shell) {
    shell->NotifyDestroyingFrame(this);
  }

  if ((mState & NS_FRAME_EXTERNAL_REFERENCE) ||
      (mState & NS_FRAME_SELECTED_CONTENT)) {
    if (shell) {
      shell->ClearFrameRefs(this);
    }
  }

  //XXX Why is this done in nsFrame instead of some frame class
  // that actually loads images?
  aPresContext.StopAllLoadImagesFor(this);

  //Set to prevent event dispatch during destruct
  if (nsnull != mView) {
    mView->SetClientData(nsnull);
  }

  delete this;
  return NS_OK;
}

NS_IMETHODIMP
nsFrame::GetContent(nsIContent** aContent) const
{
  NS_PRECONDITION(nsnull != aContent, "null OUT parameter pointer");
  NS_IF_ADDREF(mContent);
  *aContent = mContent;
  return NS_OK;
}

NS_IMETHODIMP
nsFrame::GetOffsets(PRInt32 &aStart, PRInt32 &aEnd) const
{
  aStart = 0;
  aEnd = 0;
  return NS_OK;
}


NS_IMETHODIMP
nsFrame::GetStyleContext(nsIStyleContext** aStyleContext) const
{
  NS_PRECONDITION(nsnull != aStyleContext, "null OUT parameter pointer");
  NS_ASSERTION(nsnull != mStyleContext, "frame should always have style context");
  NS_IF_ADDREF(mStyleContext);
  *aStyleContext = mStyleContext;
  return NS_OK;
}

NS_IMETHODIMP nsFrame::SetStyleContext(nsIPresContext* aPresContext,nsIStyleContext* aContext)
{
//  NS_PRECONDITION(0 == (mState & NS_FRAME_IN_REFLOW), "Shouldn't set style context during reflow");
  NS_PRECONDITION(nsnull != aContext, "null ptr");
  if (aContext != mStyleContext) {
    NS_IF_RELEASE(mStyleContext);
    if (nsnull != aContext) {
      mStyleContext = aContext;
      NS_ADDREF(aContext);
      DidSetStyleContext(aPresContext);
    }
  }

  return NS_OK;
}

// Subclass hook for style post processing
NS_IMETHODIMP nsFrame::DidSetStyleContext(nsIPresContext* aPresContext)
{
  return NS_OK;
}

NS_IMETHODIMP nsFrame::GetStyleData(nsStyleStructID aSID, const nsStyleStruct*& aStyleStruct) const
{
  NS_ASSERTION(mStyleContext!=nsnull,"null style context");
  if (mStyleContext) {
    aStyleStruct = mStyleContext->GetStyleData(aSID);
  } else {
    aStyleStruct = nsnull;
  }
  return NS_OK;
}

NS_IMETHODIMP
nsFrame::GetAdditionalStyleContext(PRInt32 aIndex, 
                                   nsIStyleContext** aStyleContext) const
{
  NS_PRECONDITION(aIndex >= 0, "invalid index number");
  NS_ASSERTION(aStyleContext, "null ptr");
  if (! aStyleContext) {
    return NS_ERROR_NULL_POINTER;
  }
  *aStyleContext = nsnull;
  return NS_ERROR_INVALID_ARG;
}

NS_IMETHODIMP
nsFrame::SetAdditionalStyleContext(PRInt32 aIndex, 
                                   nsIStyleContext* aStyleContext)
{
  NS_PRECONDITION(aIndex >= 0, "invalid index number");
  return ((aIndex < 0) ? NS_ERROR_INVALID_ARG : NS_OK);
}

// Geometric parent member functions

NS_IMETHODIMP nsFrame::GetParent(nsIFrame** aParent) const
{
  NS_PRECONDITION(nsnull != aParent, "null OUT parameter pointer");
  *aParent = mParent;
  return NS_OK;
}

NS_IMETHODIMP nsFrame::SetParent(const nsIFrame* aParent)
{
  mParent = (nsIFrame*)aParent;
  return NS_OK;
}

// Bounding rect member functions

NS_IMETHODIMP nsFrame::GetRect(nsRect& aRect) const
{
  aRect = mRect;
  return NS_OK;
}

NS_IMETHODIMP nsFrame::GetOrigin(nsPoint& aPoint) const
{
  aPoint.x = mRect.x;
  aPoint.y = mRect.y;
  return NS_OK;
}

NS_IMETHODIMP nsFrame::GetSize(nsSize& aSize) const
{
  aSize.width = mRect.width;
  aSize.height = mRect.height;
  return NS_OK;
}

NS_IMETHODIMP nsFrame::SetRect(const nsRect& aRect)
{
  MoveTo(aRect.x, aRect.y);
  SizeTo(aRect.width, aRect.height);
  return NS_OK;
}

NS_IMETHODIMP nsFrame::MoveTo(nscoord aX, nscoord aY)
{
  mRect.x = aX;
  mRect.y = aY;

  if (nsnull != mView) {
    // If we should keep the view position and size in sync with the frame
    // then position the view. Don't do this if we're in the middle of reflow.
    // Instead wait until the DidReflow() notification
    if (NS_FRAME_SYNC_FRAME_AND_VIEW == (mState & (NS_FRAME_IN_REFLOW |
                                                   NS_FRAME_SYNC_FRAME_AND_VIEW))) {
      // Position view relative to its parent, not relative to our
      // parent frame (our parent frame may not have a view).
      nsIView* parentWithView;
      nsPoint origin;
      GetOffsetFromView(origin, &parentWithView);
      nsIViewManager  *vm;
      mView->GetViewManager(vm);
      vm->MoveViewTo(mView, origin.x, origin.y);
      NS_RELEASE(vm);
    }
  }

  return NS_OK;
}

NS_IMETHODIMP nsFrame::SizeTo(nscoord aWidth, nscoord aHeight)
{
  mRect.width = aWidth;
  mRect.height = aHeight;

  // Let the view know
  if (nsnull != mView) {
    // If we should keep the view position and size in sync with the frame
    // then resize the view. Don't do this if we're in the middle of reflow.
    // Instead wait until the DidReflow() notification
    if (NS_FRAME_SYNC_FRAME_AND_VIEW == (mState & (NS_FRAME_IN_REFLOW |
                                                   NS_FRAME_SYNC_FRAME_AND_VIEW))) {
      // Resize the view to be the same size as the frame
      nsIViewManager  *vm;
      mView->GetViewManager(vm);
      vm->ResizeView(mView, aWidth, aHeight);
      NS_RELEASE(vm);
    }
  }

  return NS_OK;
}

// Child frame enumeration

NS_IMETHODIMP
nsFrame::GetAdditionalChildListName(PRInt32 aIndex, nsIAtom** aListName) const
{
  NS_PRECONDITION(nsnull != aListName, "null OUT parameter pointer");
  NS_PRECONDITION(aIndex >= 0, "invalid index number");
  *aListName = nsnull;
  return aIndex < 0 ? NS_ERROR_INVALID_ARG : NS_OK;
}

NS_IMETHODIMP nsFrame::FirstChild(nsIAtom* aListName, nsIFrame** aFirstChild) const
{
  *aFirstChild = nsnull;
  return nsnull == aListName ? NS_OK : NS_ERROR_INVALID_ARG;
}

PRBool
nsFrame::DisplaySelection(nsIPresContext& aPresContext, PRBool isOkToTurnOn)
{
  PRBool result = PR_FALSE;

  nsCOMPtr<nsIPresShell> shell;
  nsresult rv = aPresContext.GetShell(getter_AddRefs(shell));
  if (NS_SUCCEEDED(rv) && shell) {
    nsCOMPtr<nsIDocument> doc;
    rv = shell->GetDocument(getter_AddRefs(doc));
    if (NS_SUCCEEDED(rv) && doc) {
      result = doc->GetDisplaySelection();
      if (isOkToTurnOn && !result) {
        doc->SetDisplaySelection(PR_TRUE);
        result = PR_TRUE;
      }
    }
  }

  return result;
}

void
nsFrame::SetClipRect(nsIRenderingContext& aRenderingContext)
{
  PRBool clipState;
  const nsStyleDisplay* display;
  GetStyleData(eStyleStruct_Display, (const nsStyleStruct*&) display);

  // Start with the auto version of the clip rect. Then overlay on top
  // of it specific offsets.
  nscoord top = 0;
  nscoord right = mRect.width;
  nscoord bottom = mRect.height;
  nscoord left = 0;
  if (0 == (NS_STYLE_CLIP_TOP_AUTO & display->mClipFlags)) {
    top += display->mClip.top;
  }
  if (0 == (NS_STYLE_CLIP_RIGHT_AUTO & display->mClipFlags)) {
    right -= display->mClip.right;
  }
  if (0 == (NS_STYLE_CLIP_BOTTOM_AUTO & display->mClipFlags)) {
    bottom -= display->mClip.bottom;
  }
  if (0 == (NS_STYLE_CLIP_LEFT_AUTO & display->mClipFlags)) {
    left += display->mClip.left;
  }

  // Set updated clip-rect into the rendering context
  nsRect clipRect(left, top, right - left, bottom - top);
  aRenderingContext.SetClipRect(clipRect, nsClipCombine_kIntersect, clipState);
}

NS_IMETHODIMP
nsFrame::Paint(nsIPresContext&      aPresContext,
               nsIRenderingContext& aRenderingContext,
               const nsRect&        aDirtyRect,
               nsFramePaintLayer    aWhichLayer)
{
  //if (NS_FRAME_PAINT_LAYER_FOREGROUND == aWhichLayer) {
/** GetDocument
*/
    nsCOMPtr<nsIDocument> doc;
    nsresult result; 
    nsCOMPtr<nsIPresShell> shell;
    result = aPresContext.GetShell(getter_AddRefs(shell));
    if (NS_FAILED(result))
      return result;

    PRBool displaySelection;
    //result = shell->GetDisplayNonTextSelection(&displaySelection);
    //if (NS_FAILED(result))
      //return result;
    displaySelection = PR_TRUE;
    if (!displaySelection)
      return NS_OK;
    if (mContent) {
      result = mContent->GetDocument(*getter_AddRefs(doc));
    }
    if (!doc || NS_FAILED(result)) {
      if (NS_SUCCEEDED(result) && shell) {
        result = shell->GetDocument(getter_AddRefs(doc));
      }
    }

    displaySelection = doc->GetDisplaySelection();
    nsFrameState  frameState;
    PRBool        isSelected;
    GetFrameState(&frameState);
    isSelected = (frameState & NS_FRAME_SELECTED_CONTENT) == NS_FRAME_SELECTED_CONTENT;
//    PRInt32 selectionStartOffset = 0;//frame coordinates
//    PRInt32 selectionEndOffset = 0;//frame coordinates

    if (!displaySelection || !isSelected)
      return NS_OK;

    nsCOMPtr<nsIDOMSelection> selection;
    nsCOMPtr<nsIFrameSelection> frameSelection;

    nsCOMPtr<nsIContent> newContent;
    result = mContent->GetParent(*getter_AddRefs(newContent));

    SelectionDetails *details;
    PRInt32 offset;
    if (NS_SUCCEEDED(result) && newContent){
      result = newContent->IndexOf(mContent, offset);
      if (NS_FAILED(result)) 
      {
        return result;
      }
    }

    if (NS_SUCCEEDED(result) && shell){
      result = shell->GetFrameSelection(getter_AddRefs(frameSelection));
      if (NS_SUCCEEDED(result) && frameSelection){
        result = frameSelection->LookUpSelection(newContent, offset, 
                              1, &details);// last param notused
      }
    }
  //}
  if (details)
  {
    nsRect rect;
    GetRect(rect);
    rect.width-=2;
    rect.height-=2;
    rect.x++;
    rect.y++;
    aRenderingContext.SetColor(NS_RGB(0,0,255));
    nsRect drawrect(1, 1, rect.width, rect.height);
    aRenderingContext.DrawRect(drawrect );
    SelectionDetails *deletingDetails = details;
    while((deletingDetails = details->mNext) != nsnull) { 
      delete details;
      details = deletingDetails;
    }
    delete details;
    //aRenderingContext.DrawLine(rect.x, rect.y, rect.XMost(), rect.YMost());
    //aRenderingContext.DrawLine(rect.x, rect.YMost(), rect.XMost(), rect.y);
  }
  return NS_OK;
}

/**
  *
 */
NS_IMETHODIMP
nsFrame::HandleEvent(nsIPresContext& aPresContext, 
                     nsGUIEvent*     aEvent,
                     nsEventStatus&  aEventStatus)
{
  if (nsEventStatus_eConsumeNoDefault == aEventStatus) {
    return NS_OK;
  }

/*i have no idea why this is here keeping incase..
  if (DisplaySelection(aPresContext) == PR_FALSE) {
    if (aEvent->message != NS_MOUSE_LEFT_BUTTON_DOWN) {
      return NS_OK;
    }
  }
*/
  nsCOMPtr<nsIPresShell> shell;
  nsresult rv = aPresContext.GetShell(getter_AddRefs(shell));
  switch (aEvent->message)
  {
  case NS_MOUSE_MOVE:
    {
    if (NS_SUCCEEDED(rv)){
      nsCOMPtr<nsIFrameSelection> frameselection;
      if (NS_SUCCEEDED(shell->GetFrameSelection(getter_AddRefs(frameselection))) && frameselection){
          PRBool mouseDown = PR_FALSE;
          if (NS_SUCCEEDED(frameselection->GetMouseDownState(&mouseDown)) && mouseDown) {

#if NORMAL_DRAG_HANDLING
            HandleDrag(aPresContext, aEvent, aEventStatus);
#else
            nsIDragService* dragService; 
            nsresult rv = nsServiceManager::GetService(kCDragServiceCID, 
                                                       nsIDragService::GetIID(), 
                                                       (nsISupports **)&dragService); 
            if (NS_OK == rv) { 
              nsCOMPtr<nsITransferable> trans; 
              rv = nsComponentManager::CreateInstance(kCTransferableCID, nsnull, 
                                                        nsITransferable::GetIID(), getter_AddRefs(trans)); 
              nsCOMPtr<nsITransferable> trans2; 
              rv = nsComponentManager::CreateInstance(kCTransferableCID, nsnull, 
                                                        nsITransferable::GetIID(), getter_AddRefs(trans2)); 
              if ( trans && trans2 ) {
                nsString textPlainFlavor ( "text/plain" );
                trans->AddDataFlavor(&textPlainFlavor);
                nsString dragText = "Drag Text";
                PRUint32 len = 9; 
                trans->SetTransferData(&textPlainFlavor, dragText.ToNewCString(), len);   // transferable consumes the data

                trans2->AddDataFlavor(&textPlainFlavor);
                nsString dragText2 = "More Drag Text";
                len = 14; 
                trans2->SetTransferData(&textPlainFlavor, dragText2.ToNewCString(), len);   // transferable consumes the data

                nsCOMPtr<nsISupportsArray> items;
                NS_NewISupportsArray(getter_AddRefs(items));
                if ( items ) {
                  items->AppendElement(trans);
                  items->AppendElement(trans2);
                  dragService->InvokeDragSession(items, nsnull, nsIDragService::DRAGDROP_ACTION_COPY | nsIDragService::DRAGDROP_ACTION_MOVE);
                }
              } 
              nsServiceManager::ReleaseService(kCDragServiceCID, dragService); 
            } 
          //--------------------------------------------------- 
#endif             
            
          }
        }
      }
    }break;
  case NS_MOUSE_LEFT_BUTTON_DOWN:
    {
      nsCOMPtr<nsIFrameSelection> frameselection;
      if (NS_SUCCEEDED(shell->GetFrameSelection(getter_AddRefs(frameselection))) && frameselection)
        frameselection->SetMouseDownState(PR_TRUE);//not important if it fails here
      HandlePress(aPresContext, aEvent, aEventStatus);
    }break;
  case NS_MOUSE_LEFT_BUTTON_UP:
    HandleRelease(aPresContext, aEvent, aEventStatus);
    break;
  default:
    break;
  }//end switch
  return NS_OK;
}

/**
  * Handles the Mouse Press Event for the frame
 */
NS_IMETHODIMP
nsFrame::HandlePress(nsIPresContext& aPresContext, 
                     nsGUIEvent*     aEvent,
                     nsEventStatus&  aEventStatus)
{
  if (!DisplaySelection(aPresContext)) {
    return NS_OK;
  }
  nsMouseEvent *me = (nsMouseEvent *)aEvent;
  if (me->clickCount >1 )
    return HandleMultiplePress(aPresContext,aEvent,aEventStatus);

  nsCOMPtr<nsIPresShell> shell;
  nsresult rv = aPresContext.GetShell(getter_AddRefs(shell));
  if (NS_SUCCEEDED(rv) && shell) {
    nsInputEvent *inputEvent = (nsInputEvent *)aEvent;
    PRInt32 startPos = 0;
//    PRUint32 contentOffset = 0;
    PRInt32 contentOffsetEnd = 0;
    nsCOMPtr<nsIContent> newContent;
    PRBool beginContent;
    if (NS_SUCCEEDED(GetContentAndOffsetsFromPoint(aPresContext, aEvent->point,
                                 getter_AddRefs(newContent),
                                 startPos, contentOffsetEnd, beginContent))){
      nsCOMPtr<nsIFrameSelection> frameselection;
      if (NS_SUCCEEDED(shell->GetFrameSelection(getter_AddRefs(frameselection))) && frameselection){
        frameselection->SetMouseDownState(PR_TRUE);//not important if it fails here
        frameselection->HandleClick(newContent, startPos , contentOffsetEnd , inputEvent->isShift, inputEvent->isControl,beginContent);
      }
      //no release 
    }
  }
  return NS_OK;

}
 
/**
  * Handles the Multiple Mouse Press Event for the frame
 */
NS_IMETHODIMP
nsFrame::HandleMultiplePress(nsIPresContext& aPresContext, 
                             nsGUIEvent*     aEvent,
                             nsEventStatus&  aEventStatus)
{
   if (!DisplaySelection(aPresContext)) {
    return NS_OK;
  }
  nsMouseEvent *me = (nsMouseEvent *)aEvent;
  if (me->clickCount <3 )
    return NS_OK;
  nsCOMPtr<nsIPresShell> shell;
  nsresult rv = aPresContext.GetShell(getter_AddRefs(shell));
   

  if (NS_SUCCEEDED(rv) && shell) {
    nsCOMPtr<nsIRenderingContext> acx;      
    nsCOMPtr<nsIFocusTracker> tracker;
    tracker = do_QueryInterface(shell, &rv);
    if (NS_FAILED(rv) || !tracker)
      return rv;
    rv = shell->CreateRenderingContext(this, getter_AddRefs(acx));
    if (NS_SUCCEEDED(rv)){
      PRInt32 startPos = 0;
      PRInt32 contentOffsetEnd = 0;
      nsCOMPtr<nsIContent> newContent;
      PRBool beginContent = PR_FALSE;
      if (NS_SUCCEEDED(GetContentAndOffsetsFromPoint(aPresContext, aEvent->point,
                                   getter_AddRefs(newContent),
                                   startPos, contentOffsetEnd,beginContent))) {
        // find which word needs to be selected! use peek offset one
        // way then the other
        nsCOMPtr<nsIContent> startContent;
        nsCOMPtr<nsIDOMNode> startNode;
        nsCOMPtr<nsIContent> endContent;
        nsCOMPtr<nsIDOMNode> endNode;
        //peeks{}
        nsPeekOffsetStruct startpos;
        startpos.SetData(tracker, 
                        0, 
                        eSelectBeginLine,
                        eDirPrevious,
                        startPos,
                        PR_FALSE,
                        PR_TRUE);
        rv = PeekOffset(&startpos);
        if (NS_FAILED(rv))
          return rv;
        nsPeekOffsetStruct endpos;
        endpos.SetData(tracker, 
                        0, 
                        eSelectEndLine,
                        eDirNext,
                        startPos,
                        PR_FALSE,
                        PR_FALSE);
        rv = PeekOffset(&endpos);
        if (NS_FAILED(rv))
          return rv;

        endNode = do_QueryInterface(endpos.mResultContent,&rv);
        if (NS_FAILED(rv))
          return rv;
        startNode = do_QueryInterface(startpos.mResultContent,&rv);
        if (NS_FAILED(rv))
          return rv;

        nsCOMPtr<nsIDOMSelection> selection;
        if (NS_SUCCEEDED(shell->GetSelection(SELECTION_NORMAL, getter_AddRefs(selection)))){
          rv = selection->Collapse(startNode,startpos.mContentOffset);
          if (NS_FAILED(rv))
            return rv;
          rv = selection->Extend(endNode,endpos.mContentOffset);
          if (NS_FAILED(rv))
            return rv;
        }
        //no release 
      }
    }
  }
  return NS_OK;
}

NS_IMETHODIMP nsFrame::HandleDrag(nsIPresContext& aPresContext, 
                                  nsGUIEvent*     aEvent,
                                  nsEventStatus&  aEventStatus)
{
  if (!DisplaySelection(aPresContext)) {
    return NS_OK;
  }
  nsresult result;

  nsCOMPtr<nsIPresShell> presShell;

  result = aPresContext.GetShell(getter_AddRefs(presShell));

  if (NS_FAILED(result))
    return result;

  nsCOMPtr<nsIFrameSelection> frameselection;

  result = presShell->GetFrameSelection(getter_AddRefs(frameselection));

  if (NS_SUCCEEDED(result) && frameselection)
  {
    frameselection->StopAutoScrollTimer();
    frameselection->HandleDrag(&aPresContext, this, aEvent->point);
    frameselection->StartAutoScrollTimer(&aPresContext, this, aEvent->point, 30);
  }

  return NS_OK;
}

NS_IMETHODIMP nsFrame::HandleRelease(nsIPresContext& aPresContext, 
                                     nsGUIEvent*     aEvent,
                                     nsEventStatus&  aEventStatus)
{
  if (!DisplaySelection(aPresContext))
    return NS_OK;

  nsresult result;

  nsCOMPtr<nsIPresShell> presShell;

  result = aPresContext.GetShell(getter_AddRefs(presShell));

  if (NS_SUCCEEDED(result))
  {
    nsCOMPtr<nsIFrameSelection> frameselection;

    result = presShell->GetFrameSelection(getter_AddRefs(frameselection));

    if (NS_SUCCEEDED(result) && frameselection)
      frameselection->StopAutoScrollTimer();
  }

  return NS_OK;
}


nsresult nsFrame::GetContentAndOffsetsFromPoint(nsIPresContext& aCX,
                                                const nsPoint&  aPoint,
                                                nsIContent **   aNewContent,
                                                PRInt32&        aContentOffset,
                                                PRInt32&        aContentOffsetEnd,
                                                PRBool&         aBeginFrameContent)
{
  nsresult result = NS_ERROR_FAILURE;

  if (!aNewContent)
    return NS_ERROR_NULL_POINTER;

  // Traverse through children and look for the best one to give this
  // to if it fails the getposition call, make it yourself also only
  // look at primary list
  nsIView  *view         = nsnull;
  nsIFrame *kid          = nsnull;
  nsIFrame *closestFrame = nsnull;

  result = GetClosestViewForFrame(this, &view);

  if (NS_FAILED(result))
    return result;

  result = FirstChild(nsnull, &kid);

  if (NS_SUCCEEDED(result) && nsnull != kid) {

#define HUGE_DISTANCE 999999 //some HUGE number that will always fail first comparison

    PRInt32 closestXDistance = HUGE_DISTANCE;
    PRInt32 closestYDistance = HUGE_DISTANCE;

    while (nsnull != kid) {
      nsRect rect;
      nsPoint offsetPoint(0,0);
      nsIView * kidView = nsnull;

      kid->GetRect(rect);
      kid->GetOffsetFromView(offsetPoint, &kidView);

      rect.x = offsetPoint.x;
      rect.y = offsetPoint.y;

      nscoord ya = rect.y;
      nscoord yb = rect.y + rect.height;

      PRInt32 yDistance = PR_MIN(abs(ya - aPoint.y),abs(yb - aPoint.y));

      if (yDistance <= closestYDistance && rect.width > 0 && rect.height > 0)
      {
        if (yDistance < closestYDistance)
          closestXDistance = HUGE_DISTANCE;

        nscoord xa = rect.x;
        nscoord xb = rect.x + rect.width;

        if (xa <= aPoint.x && xb >= aPoint.x && ya <= aPoint.y && yb >= aPoint.y)
        {
          closestFrame = kid;
          break;
        }

        PRInt32 xDistance = PR_MIN(abs(xa - aPoint.x),abs(xb - aPoint.x));

        if (xDistance < closestXDistance)
        {
          closestXDistance = xDistance;
          closestYDistance = yDistance;
          closestFrame     = kid;
        }
        // else if (xDistance > closestXDistance)
        //   break;//done
      }
      
      kid->GetNextSibling(&kid);
    }
    if (closestFrame) {

      // If we cross a view boundary, we need to adjust
      // the coordinates because GetPosition() expects
      // them to be relative to the closest view.

      nsPoint newPoint     = aPoint;
      nsIView *closestView = nsnull;

      result = GetClosestViewForFrame(closestFrame, &closestView);

      if (NS_FAILED(result))
        return result;

      if (closestView && view != closestView)
      {
        nscoord vX = 0, vY = 0;
        result = closestView->GetPosition(&vX, &vY);
        if (NS_SUCCEEDED(result))
        {
          newPoint.x -= vX;
          newPoint.y -= vY;
        }
      }

      // printf("      0x%.8x   0x%.8x  %4d  %4d\n",
      //        closestFrame, closestView, closestXDistance, closestYDistance);

      return closestFrame->GetContentAndOffsetsFromPoint(aCX, newPoint, aNewContent,
                                                         aContentOffset, aContentOffsetEnd,aBeginFrameContent);
    }
  }

  if (!mContent)
    return NS_ERROR_NULL_POINTER;

  result = mContent->GetParent(*aNewContent);
  if (*aNewContent){
    result = (*aNewContent)->IndexOf(mContent, aContentOffset);
    if (NS_FAILED(result)) 
    {
      return result;
    }
    aContentOffsetEnd = aContentOffset +1;
  }
  aBeginFrameContent = PR_FALSE;
  return result;
}



NS_IMETHODIMP
nsFrame::GetCursor(nsIPresContext& aPresContext,
                   nsPoint& aPoint,
                   PRInt32& aCursor)
{
  const nsStyleColor* styleColor;
  GetStyleData(eStyleStruct_Color, (const nsStyleStruct*&)styleColor);
  aCursor = styleColor->mCursor;

  if ((NS_STYLE_CURSOR_AUTO == aCursor) && (nsnull != mParent)) {
    mParent->GetCursor(aPresContext, aPoint, aCursor);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsFrame::GetFrameForPoint(const nsPoint& aPoint, nsIFrame** aFrame)
{
  *aFrame = this;
  return NS_OK;
}

// Resize and incremental reflow
NS_IMETHODIMP
nsFrame::GetFrameState(nsFrameState* aResult)
{
  NS_PRECONDITION(nsnull != aResult, "null OUT parameter pointer");
  *aResult = mState;
  return NS_OK;
}

NS_IMETHODIMP
nsFrame::SetFrameState(nsFrameState aNewState)
{
  mState = aNewState;
  return NS_OK;
}

// nsIHTMLReflow member functions

NS_IMETHODIMP
nsFrame::WillReflow(nsIPresContext& aPresContext)
{
  NS_FRAME_TRACE_MSG(NS_FRAME_TRACE_CALLS,
                     ("WillReflow: oldState=%x", mState));
  mState |= NS_FRAME_IN_REFLOW;
  return NS_OK;
}

NS_IMETHODIMP
nsFrame::DidReflow(nsIPresContext& aPresContext,
                   nsDidReflowStatus aStatus)
{
  NS_FRAME_TRACE_MSG(NS_FRAME_TRACE_CALLS,
                     ("nsFrame::DidReflow: aStatus=%d", aStatus));
  if (NS_FRAME_REFLOW_FINISHED == aStatus) {
    mState &= ~(NS_FRAME_IN_REFLOW | NS_FRAME_FIRST_REFLOW | NS_FRAME_IS_DIRTY);

    // Make sure the view is sized and positioned correctly and it's
    // visibility, opacity, content transparency, and clip are correct
    if (mView) {
      nsIViewManager  *vm;
      mView->GetViewManager(vm);
      
      if (NS_FRAME_SYNC_FRAME_AND_VIEW & mState) {
        // Position and size view relative to its parent, not relative to our
        // parent frame (our parent frame may not have a view).
        nsIView* parentWithView;
        nsPoint origin;
        GetOffsetFromView(origin, &parentWithView);
        vm->ResizeView(mView, mRect.width, mRect.height);
        vm->MoveViewTo(mView, origin.x, origin.y);
      }

      const nsStyleColor* color =
        (const nsStyleColor*)mStyleContext->GetStyleData(eStyleStruct_Color);
      const nsStyleDisplay* display =
        (const nsStyleDisplay*)mStyleContext->GetStyleData(eStyleStruct_Display);

      // Set the view's opacity
      vm->SetViewOpacity(mView, color->mOpacity);
      
      // See if the view should be hidden or visible
      PRBool  viewIsVisible = PR_TRUE;
      PRBool  viewHasTransparentContent = (color->mBackgroundFlags &
                NS_STYLE_BG_COLOR_TRANSPARENT) == NS_STYLE_BG_COLOR_TRANSPARENT;

      if (NS_STYLE_VISIBILITY_COLLAPSE == display->mVisible) {
        viewIsVisible = PR_FALSE;
      }
      else if (NS_STYLE_VISIBILITY_HIDDEN == display->mVisible) {
        // If it has a widget, hide the view because the widget can't deal with it
        nsIWidget* widget = nsnull;
        mView->GetWidget(widget);
        if (widget) {
          viewIsVisible = PR_FALSE;
          NS_RELEASE(widget);
        }
        else {
          // If it's a scroll frame, then hide the view. This means that
          // child elements can't override their parent's visibility, but
          // it's not practical to leave it visible in all cases because
          // the scrollbars will be showing
          nsIAtom*  frameType;
          GetFrameType(&frameType);

          if (frameType == nsLayoutAtoms::scrollFrame) {
            viewIsVisible = PR_FALSE;

          } else {
            // If we're a container element, then leave the view visible, but
            // mark it as having transparent content. The reason we need to
            // do this is that child elements can override their parent's
            // hidden visibility and be visible anyway
            nsIFrame* firstChild;
  
            FirstChild(nsnull, &firstChild);
            if (firstChild) {
              // Not a left frame, so the view needs to be visible, but marked
              // as having transparent content
              viewHasTransparentContent = PR_TRUE;
            } else {
              // Leaf frame so go ahead and hide the view
              viewIsVisible = PR_FALSE;
            }
          }
          NS_IF_RELEASE(frameType);
        }
      }

      // If we have visible content that overflows the content area, then we
      // need the view marked as having transparent content
      if (NS_STYLE_OVERFLOW_VISIBLE == display->mOverflow) {
        if (mState & NS_FRAME_OUTSIDE_CHILDREN) {
          viewHasTransparentContent = PR_TRUE;
        }
      }

      // Make sure visibility is correct
      vm->SetViewVisibility(mView, viewIsVisible ? nsViewVisibility_kShow :
                            nsViewVisibility_kHide);
      
      // Make sure content transparency is correct
      if (viewIsVisible) {
        vm->SetViewContentTransparency(mView, viewHasTransparentContent);
      }
      
      // Clip applies to block-level and replaced elements with overflow
      // set to other than 'visible'
      if (display->IsBlockLevel()) {
        if (display->mOverflow == NS_STYLE_OVERFLOW_HIDDEN) {
          nscoord left, top, right, bottom;
          
          // Start with the 'auto' values and then factor in user
          // specified values
          left = top = 0;
          right = mRect.width;
          bottom = mRect.height;

          if (0 == (NS_STYLE_CLIP_TOP_AUTO & display->mClipFlags)) {
            top += display->mClip.top;
          }
          if (0 == (NS_STYLE_CLIP_RIGHT_AUTO & display->mClipFlags)) {
            right -= display->mClip.right;
          }
          if (0 == (NS_STYLE_CLIP_BOTTOM_AUTO & display->mClipFlags)) {
            bottom -= display->mClip.bottom;
          }
          if (0 == (NS_STYLE_CLIP_LEFT_AUTO & display->mClipFlags)) {
            left += display->mClip.left;
          }
          mView->SetClip(left, top, right, bottom);

        } else {
          // Make sure no clip is set
          mView->SetClip(0, 0, 0, 0);
        }
      }
      NS_RELEASE(vm);
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsFrame::Reflow(nsIPresContext&          aPresContext,
                nsHTMLReflowMetrics&     aDesiredSize,
                const nsHTMLReflowState& aReflowState,
                nsReflowStatus&          aStatus)
{
  aDesiredSize.width = 0;
  aDesiredSize.height = 0;
  aDesiredSize.ascent = 0;
  aDesiredSize.descent = 0;
  if (nsnull != aDesiredSize.maxElementSize) {
    aDesiredSize.maxElementSize->width = 0;
    aDesiredSize.maxElementSize->height = 0;
  }
  aStatus = NS_FRAME_COMPLETE;
  return NS_OK;
}

NS_IMETHODIMP
nsFrame::FindTextRuns(nsLineLayout& aLineLayout)
{
  aLineLayout.EndTextRun();
  return NS_OK;
}

NS_IMETHODIMP
nsFrame::AdjustFrameSize(nscoord aExtraSpace, nscoord& aUsedSpace)
{
  aUsedSpace = 0;
  return NS_OK;
}

NS_IMETHODIMP
nsFrame::TrimTrailingWhiteSpace(nsIPresContext* aPresContext,
                                nsIRenderingContext& aRC,
                                nscoord& aDeltaWidth)
{
  aDeltaWidth = 0;
  return NS_OK;
}

NS_IMETHODIMP
nsFrame::ContentChanged(nsIPresContext* aPresContext,
                        nsIContent*     aChild,
                        nsISupports*    aSubContent)
{
  nsCOMPtr<nsIPresShell> shell;
  nsresult rv = aPresContext->GetShell(getter_AddRefs(shell));
  if (NS_SUCCEEDED(rv) && shell) {
    nsIReflowCommand* reflowCmd;
    rv = NS_NewHTMLReflowCommand(&reflowCmd, this,
                                 nsIReflowCommand::ContentChanged);
    if (NS_SUCCEEDED(rv)) {
      shell->AppendReflowCommand(reflowCmd);
      NS_RELEASE(reflowCmd);
    }
  }
  return rv;
}

NS_IMETHODIMP
nsFrame::AttributeChanged(nsIPresContext* aPresContext,
                          nsIContent*     aChild,
                          nsIAtom*        aAttribute,
                          PRInt32         aHint)
{
  return NS_OK;
}

NS_IMETHODIMP
nsFrame::ContentStateChanged(nsIPresContext* aPresContext,
                             nsIContent*     aChild,
                             PRInt32         aHint)
{
  return NS_OK;
}

// Flow member functions

NS_IMETHODIMP nsFrame::IsSplittable(nsSplittableType& aIsSplittable) const
{
  aIsSplittable = NS_FRAME_NOT_SPLITTABLE;
  return NS_OK;
}

NS_IMETHODIMP nsFrame::GetPrevInFlow(nsIFrame** aPrevInFlow) const
{
  *aPrevInFlow = nsnull;
  return NS_OK;
}

NS_IMETHODIMP nsFrame::SetPrevInFlow(nsIFrame*)
{
  NS_ERROR("not splittable");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsFrame::GetNextInFlow(nsIFrame** aNextInFlow) const
{
  *aNextInFlow = nsnull;
  return NS_OK;
}

NS_IMETHODIMP nsFrame::SetNextInFlow(nsIFrame*)
{
  NS_ERROR("not splittable");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsFrame::AppendToFlow(nsIFrame* aAfterFrame)
{
  NS_ERROR("not splittable");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsFrame::PrependToFlow(nsIFrame* aBeforeFrame)
{
  NS_ERROR("not splittable");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsFrame::RemoveFromFlow()
{
  NS_ERROR("not splittable");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsFrame::BreakFromPrevFlow()
{
  NS_ERROR("not splittable");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsFrame::BreakFromNextFlow()
{
  NS_ERROR("not splittable");
  return NS_ERROR_NOT_IMPLEMENTED;
}

// Associated view object
NS_IMETHODIMP nsFrame::GetView(nsIView** aView) const
{
  NS_PRECONDITION(nsnull != aView, "null OUT parameter pointer");
  *aView = mView;
  return NS_OK;
}

NS_IMETHODIMP nsFrame::SetView(nsIView* aView)
{
  nsresult  rv;

  if (nsnull != aView) {
    mView = aView;
    aView->SetClientData(this);
    rv = NS_OK;
  }
  else
    rv = NS_OK;

  return rv;
}

// Find the first geometric parent that has a view
NS_IMETHODIMP nsFrame::GetParentWithView(nsIFrame** aParent) const
{
  NS_PRECONDITION(nsnull != aParent, "null OUT parameter pointer");

  nsIFrame* parent;
  for (parent = mParent; nsnull != parent; parent->GetParent(&parent)) {
    nsIView* parView;
     
    parent->GetView(&parView);
    if (nsnull != parView) {
      break;
    }
  }

  *aParent = parent;
  return NS_OK;
}

// Returns the offset from this frame to the closest geometric parent that
// has a view. Also returns the containing view or null in case of error
NS_IMETHODIMP nsFrame::GetOffsetFromView(nsPoint& aOffset, nsIView** aView) const
{
  NS_PRECONDITION(nsnull != aView, "null OUT parameter pointer");
  nsIFrame* frame = (nsIFrame*)this;

  *aView = nsnull;
  aOffset.MoveTo(0, 0);
  do {
    nsPoint origin;

    frame->GetOrigin(origin);
    aOffset += origin;
    frame->GetParent(&frame);
    if (nsnull != frame) {
      frame->GetView(aView);
    }
  } while ((nsnull != frame) && (nsnull == *aView));
  return NS_OK;
}

NS_IMETHODIMP nsFrame::GetWindow(nsIWidget** aWindow) const
{
  NS_PRECONDITION(nsnull != aWindow, "null OUT parameter pointer");
  
  nsIFrame*  frame;
  nsIWidget* window = nsnull;
  for (frame = (nsIFrame*)this; nsnull != frame; frame->GetParentWithView(&frame)) {
    nsIView* view;
     
    frame->GetView(&view);
    if (nsnull != view) {
      view->GetWidget(window);
      if (nsnull != window) {
        break;
      }
    }
  }
  NS_POSTCONDITION(nsnull != window, "no window in frame tree");
  *aWindow = window;
  return NS_OK;
}

NS_IMETHODIMP
nsFrame::GetFrameType(nsIAtom** aType) const
{
  NS_PRECONDITION(nsnull != aType, "null OUT parameter pointer");
  *aType = nsnull;
  return NS_OK;
}

void
nsFrame::Invalidate(const nsRect& aDamageRect,
                    PRBool aImmediate) const
{
  nsIViewManager* viewManager = nsnull;
  nsRect damageRect(aDamageRect);

  // Checks to see if the damaged rect should be infalted 
  // to include the outline
  const nsStyleSpacing* spacing;
  GetStyleData(eStyleStruct_Spacing, (const nsStyleStruct*&)spacing);
  nscoord width;
  spacing->GetOutlineWidth(width);
  if (width > 0) {
    damageRect.Inflate(width, width);
  }

  PRUint32 flags = aImmediate ? NS_VMREFRESH_IMMEDIATE : NS_VMREFRESH_NO_SYNC;
  if (nsnull != mView) {
    mView->GetViewManager(viewManager);
    viewManager->UpdateView(mView, damageRect, flags);
    
  } else {
    nsRect    rect(damageRect);
    nsPoint   offset;
    nsIView*  view;
  
    GetOffsetFromView(offset, &view);
    NS_ASSERTION(nsnull != view, "no view");
    rect += offset;
    view->GetViewManager(viewManager);
    viewManager->UpdateView(view, rect, flags);
  }

  NS_IF_RELEASE(viewManager);
}

#define MAX_REFLOW_DEPTH 500

PRBool
nsFrame::IsFrameTreeTooDeep(const nsHTMLReflowState& aReflowState,
                            nsHTMLReflowMetrics& aMetrics)
{
  if (aReflowState.mReflowDepth > MAX_REFLOW_DEPTH) {
    mState |= NS_FRAME_IS_UNFLOWABLE;
    mState &= ~NS_FRAME_OUTSIDE_CHILDREN;
    aMetrics.width = 0;
    aMetrics.height = 0;
    aMetrics.ascent = 0;
    aMetrics.descent = 0;
    aMetrics.mCarriedOutBottomMargin = 0;
    aMetrics.mCombinedArea.x = 0;
    aMetrics.mCombinedArea.y = 0;
    aMetrics.mCombinedArea.width = 0;
    aMetrics.mCombinedArea.height = 0;
    if (aMetrics.maxElementSize) {
      aMetrics.maxElementSize->width = 0;
      aMetrics.maxElementSize->height = 0;
    }
    return PR_TRUE;
  }
  mState &= ~NS_FRAME_IS_UNFLOWABLE;
  return PR_FALSE;
}

// Style sizing methods
NS_IMETHODIMP nsFrame::IsPercentageBase(PRBool& aBase) const
{
  const nsStylePosition* position;
  GetStyleData(eStyleStruct_Position, (const nsStyleStruct*&)position);
  if (position->mPosition != NS_STYLE_POSITION_NORMAL) {
    aBase = PR_TRUE;
  }
  else {
    const nsStyleDisplay* display;
    GetStyleData(eStyleStruct_Display, (const nsStyleStruct*&)display);
    if ((display->mDisplay == NS_STYLE_DISPLAY_BLOCK) || 
        (display->mDisplay == NS_STYLE_DISPLAY_LIST_ITEM) ||
        (display->mDisplay == NS_STYLE_DISPLAY_TABLE_CELL)) {
      aBase = PR_TRUE;
    }
    else {
      aBase = PR_FALSE;
    }
  }
  return NS_OK;
}

// Sibling pointer used to link together frames

NS_IMETHODIMP nsFrame::GetNextSibling(nsIFrame** aNextSibling) const
{
  NS_PRECONDITION(nsnull != aNextSibling, "null OUT parameter pointer");
  *aNextSibling = mNextSibling;
  return NS_OK;
}

NS_IMETHODIMP nsFrame::SetNextSibling(nsIFrame* aNextSibling)
{
  NS_ASSERTION(aNextSibling != this, "attempt to create circular frame list");
  mNextSibling = aNextSibling;
  return NS_OK;
}

NS_IMETHODIMP nsFrame::Scrolled(nsIView *aView)
{
  return NS_OK;
}

PRInt32 nsFrame::ContentIndexInContainer(const nsIFrame* aFrame)
{
  nsIContent* content;
  PRInt32     result = -1;

  aFrame->GetContent(&content);
  if (nsnull != content) {
    nsIContent* parentContent;

    content->GetParent(parentContent);
    if (nsnull != parentContent) {
      parentContent->IndexOf(content, result);
      NS_RELEASE(parentContent);
    }
    NS_RELEASE(content);
  }

  return result;
}

// Debugging
NS_IMETHODIMP
nsFrame::List(FILE* out, PRInt32 aIndent) const
{
  IndentBy(out, aIndent);
  ListTag(out);
  if (nsnull != mView) {
    fprintf(out, " [view=%p]", mView);
  }
  fprintf(out, " {%d,%d,%d,%d}", mRect.x, mRect.y, mRect.width, mRect.height);
  if (0 != mState) {
    fprintf(out, " [state=%08x]", mState);
  }
  fputs("\n", out);
  return NS_OK;
}

NS_IMETHODIMP
nsFrame::GetFrameName(nsString& aResult) const
{
  return MakeFrameName("Frame", aResult);
}

nsresult
nsFrame::MakeFrameName(const char* aType, nsString& aResult) const
{
  aResult = aType;
  if (nsnull != mContent) {
    nsIAtom* tag;
    mContent->GetTag(tag);
    if ((tag != nsnull) && (tag != nsLayoutAtoms::textTagName)) {
      aResult.Append("(");
      nsAutoString buf;
      tag->ToString(buf);
      aResult.Append(buf);
      NS_RELEASE(tag);
      aResult.Append(")");
    }
  }
  char buf[40];
  PR_snprintf(buf, sizeof(buf), "(%d)", ContentIndexInContainer(this));
  aResult.Append(buf);
  return NS_OK;
}

void
nsFrame::XMLQuote(nsString& aString)
{
  PRInt32 i, len = aString.Length();
  for (i = 0; i < len; i++) {
    PRUnichar ch = aString.CharAt(i);
    if (ch == '<') {
      nsAutoString tmp("&lt;");
      aString.Cut(i, 1);
      aString.Insert(tmp, i);
      len += 3;
      i += 3;
    }
    else if (ch == '>') {
      nsAutoString tmp("&gt;");
      aString.Cut(i, 1);
      aString.Insert(tmp, i);
      len += 3;
      i += 3;
    }
    else if (ch == '\"') {
      nsAutoString tmp("&quot;");
      aString.Cut(i, 1);
      aString.Insert(tmp, i);
      len += 5;
      i += 5;
    }
  }
}

PRBool
nsFrame::ParentDisablesSelection() const
{
  return PR_FALSE;//depricating method perhaps.
/*
  PRBool selected;
  if (NS_FAILED(GetSelected(&selected)))
    return PR_FALSE;
  if (selected)
    return PR_FALSE; //if this frame is selected and no one has overridden the selection from "higher up"
                     //then no one below us will be disabled by this frame.
  nsIFrame* target;
  GetParent(&target);
  if (target)
    return ((nsFrame *)target)->ParentDisablesSelection();
  return PR_FALSE; //default this does not happen
  */
}

NS_IMETHODIMP
nsFrame::DumpRegressionData(FILE* out, PRInt32 aIndent)
{
  IndentBy(out, aIndent);
  fprintf(out, "<frame va=\"%ld\" type=\"", PRUptrdiff(this));
  nsAutoString name;
  GetFrameName(name);
  XMLQuote(name);
  fputs(name, out);
  fprintf(out, "\" state=\"%d\" parent=\"%ld\">\n",
          mState, PRUptrdiff(mParent));

  aIndent++;
  DumpBaseRegressionData(out, aIndent);
  aIndent--;

  IndentBy(out, aIndent);
  fprintf(out, "</frame>\n");

  return NS_OK;
}

void
nsFrame::DumpBaseRegressionData(FILE* out, PRInt32 aIndent)
{
  if (nsnull != mNextSibling) {
    IndentBy(out, aIndent);
    fprintf(out, "<next-sibling va=\"%ld\"/>\n", PRUptrdiff(mNextSibling));
  }

  if (nsnull != mView) {
    IndentBy(out, aIndent);
    fprintf(out, "<view va=\"%ld\">\n", PRUptrdiff(mView));
    aIndent++;
    // XXX add in code to dump out view state too...
    aIndent--;
    IndentBy(out, aIndent);
    fprintf(out, "</view>\n");
  }

  IndentBy(out, aIndent);
  fprintf(out, "<bbox x=\"%d\" y=\"%d\" w=\"%d\" h=\"%d\"/>\n",
          mRect.x, mRect.y, mRect.width, mRect.height);

  // Now dump all of the children on all of the child lists
  nsIFrame* kid;
  nsIAtom* list = nsnull;
  PRInt32 listIndex = 0;
  do {
    nsresult rv = FirstChild(list, &kid);
    if (NS_SUCCEEDED(rv) && (nsnull != kid)) {
      IndentBy(out, aIndent);
      if (nsnull != list) {
        nsAutoString listName;
        list->ToString(listName);
        fprintf(out, "<child-list name=\"");
        XMLQuote(listName);
        fputs(listName, out);
        fprintf(out, "\">\n");
      }
      else {
        fprintf(out, "<child-list>\n");
      }
      aIndent++;
      while (nsnull != kid) {
        kid->DumpRegressionData(out, aIndent);
        kid->GetNextSibling(&kid);
      }
      aIndent--;
      IndentBy(out, aIndent);
      fprintf(out, "</child-list>\n");
    }
    NS_IF_RELEASE(list);
    GetAdditionalChildListName(listIndex++, &list);
  } while (nsnull != list);
}

NS_IMETHODIMP
nsFrame::SizeOf(nsISizeOfHandler* aHandler, PRUint32* aResult) const
{
  if (!aResult) {
    return NS_ERROR_NULL_POINTER;
  }
#ifdef DEBUG
  *aResult = sizeof(*this);
#else
  *aResult = 0;
#endif
  return NS_OK;
}

NS_IMETHODIMP
nsFrame::VerifyTree() const
{
  NS_ASSERTION(0 == (mState & NS_FRAME_IN_REFLOW), "frame is in reflow");
  return NS_OK;
}

/*this method may.. invalidate if the state was changed or if aForceRedraw is PR_TRUE
  it will not update immediately.*/
NS_IMETHODIMP
nsFrame::SetSelected(nsIDOMRange *aRange,PRBool aSelected, nsSpread aSpread)
{
  if (aSelected && ParentDisablesSelection())
    return NS_OK;
/*  nsresult rv;

  if (eSpreadDown == aSpread){
    nsIFrame* kid;
    rv = FirstChild(nsnull, &kid);
    if (NS_SUCCEEDED(rv)) {
      while (nsnull != kid) {
        kid->SetSelected(nsnull,aSelected,aSpread);
        kid->GetNextSibling(&kid);
      }
    }
  }
*/
  nsFrameState  frameState;
  GetFrameState(&frameState);
  PRBool isSelected = ((frameState & NS_FRAME_SELECTED_CONTENT) == NS_FRAME_SELECTED_CONTENT);
  if (aSelected == isSelected) //allready set thanks
  {
    return NS_OK;
  }
  if ( aSelected ){
    frameState |=  NS_FRAME_SELECTED_CONTENT;
  }
  else
    frameState &= ~NS_FRAME_SELECTED_CONTENT;
  SetFrameState(frameState);
  nsRect frameRect;
  GetRect(frameRect);
  nsRect rect(0, 0, frameRect.width, frameRect.height);
  Invalidate(rect, PR_FALSE);
#if 0
  if (aRange) {
    //lets see if the range contains us, if so we must redraw!
    nsCOMPtr<nsIDOMNode> endNode;
    nsCOMPtr<nsIDOMNode> startNode;
    aRange->GetEndParent(getter_AddRefs(endNode));
    aRange->GetStartParent(getter_AddRefs(startNode));
    nsCOMPtr<nsIContent> content;
    rv = GetContent(getter_AddRefs(content));
    nsCOMPtr<nsIDOMNode> thisNode;
    thisNode = do_QueryInterface(content);

//we must tell the siblings about the set selected call
//since the getprimaryframe call is done with this content node.
    if (thisNode != startNode && thisNode != endNode)
    { //whole node selected
      nsIFrame *frame;
      rv = GetNextSibling(&frame);
      while (NS_SUCCEEDED(rv) && frame)
      {
        frame->SetSelected(aRange,aSelected,eSpreadDown);
        rv = frame->GetNextSibling(&frame);
        if (NS_FAILED(rv))
          break;
      }
    }
  }
#endif
  return NS_OK;
}

NS_IMETHODIMP
nsFrame::GetSelected(PRBool *aSelected) const
{
  if (!aSelected )
    return NS_ERROR_NULL_POINTER;
  *aSelected = (PRBool)(mState & NS_FRAME_SELECTED_CONTENT);
  return NS_OK;
}

NS_IMETHODIMP
nsFrame::GetPointFromOffset(nsIPresContext* inPresContext, nsIRenderingContext* inRendContext, PRInt32 inOffset, nsPoint* outPoint)
{
  NS_PRECONDITION(outPoint != nsnull, "Null parameter");
	nsPoint		bottomLeft(0, 0);
  *outPoint = bottomLeft;
  return NS_OK;
}

NS_IMETHODIMP
nsFrame::GetChildFrameContainingOffset(PRInt32 inContentOffset, PRBool inHint, PRInt32* outFrameContentOffset, nsIFrame **outChildFrame)
{
  NS_PRECONDITION(outChildFrame && outFrameContentOffset, "Null parameter");
  *outFrameContentOffset = (PRInt32)inHint;
  *outChildFrame = this;
  return NS_OK;
}

nsresult
nsFrame::GetNextPrevLineFromeBlockFrame(nsPeekOffsetStruct *aPos,
                                        nsIFrame *aBlockFrame, 
                                        PRInt32 aLineStart, 
                                        PRInt8 aOutSideLimit
                                        )
{
  //magic numbers aLineStart will be -1 for end of block 0 will be start of block
  if (!aBlockFrame || !aPos)
    return NS_ERROR_NULL_POINTER;

  aPos->mResultFrame = nsnull;
  aPos->mResultContent = nsnull;
  aPos->mPreferLeft = (aPos->mDirection == eDirNext);

   nsresult result;
  nsCOMPtr<nsILineIterator> it; 
  result = aBlockFrame->QueryInterface(nsILineIterator::GetIID(),getter_AddRefs(it));
  if (NS_FAILED(result) || !it)
    return result;
  PRInt32 searchingLine = aLineStart;
  PRInt32 countLines;
  result = it->GetNumLines(&countLines);
  if (aOutSideLimit > 0) //start at end
    searchingLine = countLines;
  else if (aOutSideLimit <0)//start at begining
    searchingLine = -1;//"next" will be 0
  else 
  if ((aPos->mDirection == eDirPrevious && searchingLine == 0) || 
      (aPos->mDirection == eDirNext && searchingLine >= (countLines -1) )){
      //we need to jump to new block frame.
    return NS_ERROR_FAILURE;
  }
  PRInt32 lineFrameCount;
  nsIFrame *resultFrame = nsnull;
  nsIFrame *farStoppingFrame = nsnull; //we keep searching until we find a "this" frame then we go to next line
  nsIFrame *nearStoppingFrame = nsnull; //if we are backing up from edge, stop here
  nsIFrame *firstFrame;
  nsIFrame *lastFrame;
  nsRect  nonUsedRect;
  PRBool isBeforeFirstFrame, isAfterLastFrame;
  PRBool found = PR_FALSE;
  while (!found)
  {
    if (aPos->mDirection == eDirPrevious)
      searchingLine --;
    else
      searchingLine ++;
    if ((aPos->mDirection == eDirPrevious && searchingLine < 0) || 
       (aPos->mDirection == eDirNext && searchingLine >= countLines ))
    {
        //we need to jump to new block frame.
      return NS_ERROR_FAILURE;
    }
    PRUint32 lineFlags;
    result = it->GetLine(searchingLine, &firstFrame, &lineFrameCount,
                         nonUsedRect, &lineFlags);
    if (!lineFrameCount)
      continue;
    if (NS_SUCCEEDED(result)){
      lastFrame = firstFrame;
      for (;lineFrameCount > 1;lineFrameCount --){
        result = lastFrame->GetNextSibling(&lastFrame);
        if (NS_FAILED(result)){
          NS_ASSERTION(0,"should not be reached nsFrame\n");
          continue;
        }
      }
      GetLastLeaf(&lastFrame);

      if (aPos->mDirection == eDirNext){
        nearStoppingFrame = firstFrame;
        farStoppingFrame = lastFrame;
      }
      else{
        nearStoppingFrame = lastFrame;
        farStoppingFrame = firstFrame;
      }
      nsPoint offset;
      nsIView * view; //used for call of get offset from view
      aBlockFrame->GetOffsetFromView(offset,&view);
      nscoord newDesiredX  = aPos->mDesiredX - offset.x;//get desired x into blockframe coordinates!
      result = it->FindFrameAt(searchingLine, newDesiredX, &resultFrame, &isBeforeFirstFrame, &isAfterLastFrame);
    }

    if (NS_SUCCEEDED(result) && resultFrame)
    {
      nsCOMPtr<nsILineIterator> newIt; 
      //check to see if this is ANOTHER blockframe inside the other one if so then call into its lines
      result = resultFrame->QueryInterface(nsILineIterator::GetIID(),getter_AddRefs(newIt));
      if (NS_SUCCEEDED(result) && newIt)
      {
        aPos->mResultFrame = resultFrame;
        return NS_OK;
      }
      //resultFrame is not a block frame

      nsCOMPtr<nsIBidirectionalEnumerator> frameTraversal;
      result = NS_NewFrameTraversal(getter_AddRefs(frameTraversal), LEAF,
                                    resultFrame);
      if (NS_FAILED(result))
        return result;
      nsISupports *isupports = nsnull;
      nsIFrame *storeOldResultFrame = resultFrame;
      while ( !found ){
        nsCOMPtr<nsIPresContext> context;
        result = aPos->mTracker->GetPresContext(getter_AddRefs(context));
        nsPoint point;
        point.x = aPos->mDesiredX;
        point.y = 0;
        result = resultFrame->GetContentAndOffsetsFromPoint(*(context.get()),point,
                                          getter_AddRefs(aPos->mResultContent),
                                          aPos->mContentOffset,
                                          aPos->mContentOffsetEnd,
                                          aPos->mPreferLeft);
        if (NS_SUCCEEDED(result))
        {
          found = PR_TRUE;
        }
        else {
          if (aPos->mDirection == eDirPrevious && (resultFrame == farStoppingFrame))
            break;
          if (aPos->mDirection == eDirNext && (resultFrame == nearStoppingFrame))
            break;
          //always try previous on THAT line if that fails go the other way
          result = frameTraversal->Prev();
          if (NS_FAILED(result))
            break;
          result = frameTraversal->CurrentItem(&isupports);
          if (NS_FAILED(result) || !isupports)
            return result;
          //we must CAST here to an nsIFrame. nsIFrame doesnt really follow the rules
          resultFrame = (nsIFrame *)isupports;
        }
      }

      if (!found){
        resultFrame = storeOldResultFrame;
        result = NS_NewFrameTraversal(getter_AddRefs(frameTraversal), LEAF,
                                      resultFrame);
      }
      while ( !found ){
        nsCOMPtr<nsIPresContext> context;
        result = aPos->mTracker->GetPresContext(getter_AddRefs(context));

        nsPoint point;
        point.x = aPos->mDesiredX;
        point.y = 0;

        result = resultFrame->GetContentAndOffsetsFromPoint(*(context.get()),point,
                                          getter_AddRefs(aPos->mResultContent), aPos->mContentOffset,
                                          aPos->mContentOffsetEnd, aPos->mPreferLeft);
        if (NS_SUCCEEDED(result))
        {
          found = PR_TRUE;
          if (resultFrame == farStoppingFrame)
            aPos->mPreferLeft = PR_FALSE;
          else
            aPos->mPreferLeft = PR_TRUE;
        }
        else {
          if (aPos->mDirection == eDirPrevious && (resultFrame == nearStoppingFrame))
            break;
          if (aPos->mDirection == eDirNext && (resultFrame == farStoppingFrame))
            break;
          //previous didnt work now we try "next"
          result = frameTraversal->Next();
          if (NS_FAILED(result))
            break;
          result = frameTraversal->CurrentItem(&isupports);
          if (NS_FAILED(result) || !isupports)
            return result;
          //we must CAST here to an nsIFrame. nsIFrame doesnt really follow the rules
          resultFrame = (nsIFrame *)isupports;
        }
      }
      aPos->mResultFrame = resultFrame;
    }
    else {
        //we need to jump to new block frame.
      aPos->mAmount = eSelectLine;
      aPos->mStartOffset = 0;
      aPos->mEatingWS = PR_FALSE;
      aPos->mPreferLeft = !(aPos->mDirection == eDirNext);
      if (aPos->mDirection == eDirPrevious)
        aPos->mStartOffset = -1;//start from end
     return aBlockFrame->PeekOffset(aPos);
    }
  }
  return NS_OK;
}


NS_IMETHODIMP
nsFrame::PeekOffset(nsPeekOffsetStruct *aPos)
{
  if (!aPos || !aPos->mTracker )
    return NS_ERROR_NULL_POINTER;
  nsresult result = NS_ERROR_FAILURE; 
  PRInt32 endoffset;
  switch (aPos->mAmount){
    case eSelectNoAmount:
    {
      nsCOMPtr<nsIPresContext> context;
      result = aPos->mTracker->GetPresContext(getter_AddRefs(context));
      if (NS_FAILED(result) || !context)
        return result;
      nsPoint point;
      point.x = aPos->mDesiredX;
      point.y = 0;
      result = GetContentAndOffsetsFromPoint(*(context.get()),point,
                             getter_AddRefs(aPos->mResultContent),
                             aPos->mContentOffset,
                             endoffset,
                             aPos->mPreferLeft);
    }break;
    case eSelectLine :
    {
      nsCOMPtr<nsILineIterator> it; 
      nsIFrame *blockFrame = this;
      nsIFrame *thisBlock = this;
      PRInt32   thisLine;

      while (NS_FAILED(result)){
        thisBlock = blockFrame;
        result = blockFrame->GetParent(&blockFrame);
        if (NS_FAILED(result) || !blockFrame) //if at line 0 then nothing to do
          return result;
        result = blockFrame->QueryInterface(nsILineIterator::GetIID(),getter_AddRefs(it));
        while (NS_FAILED(result) && blockFrame)
        {
          thisBlock = blockFrame;
          result = blockFrame->GetParent(&blockFrame);
          if (NS_SUCCEEDED(result) && blockFrame){
            result = blockFrame->QueryInterface(nsILineIterator::GetIID(),getter_AddRefs(it));
          }
        }
        //this block is now one child down from blockframe
        if (NS_FAILED(result) || !it || !blockFrame || !thisBlock)
          return result;
        result = it->FindLineContaining(thisBlock, &thisLine);
        if (NS_FAILED(result) || thisLine <0)
          return result;
        int edgeCase = 0;//no edge case. this should look at thisLine
        PRBool doneLooping = PR_FALSE;//tells us when no more block frames hit.
        //this part will find a frame or a block frame. if its a block frame
        //it will "drill down" to find a viable frame or it will return an error.
        do {

          result = GetNextPrevLineFromeBlockFrame(aPos, 
                                        blockFrame, 
                                        thisLine, 
                                        edgeCase //start from thisLine
                                        );
          if (aPos->mResultFrame == this)//we came back to same spot! keep going
          {
            aPos->mResultFrame = nsnull;
            if (aPos->mDirection == eDirPrevious)
              thisLine--;
            else
              thisLine++;
          }
          else
            doneLooping = PR_TRUE; //do not continue with while loop
          if (NS_SUCCEEDED(result) && aPos->mResultFrame){
            result = aPos->mResultFrame->QueryInterface(nsILineIterator::GetIID(),getter_AddRefs(it));
            if (NS_SUCCEEDED(result) && it)//we have struck another block element!
            {
              doneLooping = PR_FALSE;
              if (aPos->mDirection == eDirPrevious)
                edgeCase = 1;//far edge, search from end backwards
              else
                edgeCase = -1;//near edge search from beginning onwards
              thisLine=0;//this line means nothing now.
              //everything else means something so keep looking "inside" the block
              blockFrame = aPos->mResultFrame;

            }
            else
              result = NS_OK;//THIS is to mean that everything is ok to the containing while loop
          }
        }while(!doneLooping);

      }
      break;
    }
    case eSelectBeginLine:
    case eSelectEndLine:
    {
      nsCOMPtr<nsILineIterator> it; 
      nsIFrame *blockFrame = this;
      nsIFrame *thisBlock = this;
      PRInt32   thisLine;
      result = blockFrame->GetParent(&blockFrame);
      if (NS_FAILED(result) || !blockFrame) //if at line 0 then nothing to do
        return result;
      result = blockFrame->QueryInterface(nsILineIterator::GetIID(),getter_AddRefs(it));
      while (NS_FAILED(result) && blockFrame)
      {
        thisBlock = blockFrame;
        result = blockFrame->GetParent(&blockFrame);
        if (NS_SUCCEEDED(result) && blockFrame){
          result = blockFrame->QueryInterface(nsILineIterator::GetIID(),getter_AddRefs(it));
        }
      }
      //this block is now one child down from blockframe
      if (NS_FAILED(result) || !it || !blockFrame || !thisBlock)
        return result;
      result = it->FindLineContaining(thisBlock, &thisLine);
      if (NS_FAILED(result) || thisLine < 0 )
        return result;
      nsCOMPtr<nsIPresContext> context;
      result = aPos->mTracker->GetPresContext(getter_AddRefs(context));
      if (NS_FAILED(result) || !context)
        return result;
      PRInt32 lineFrameCount;
      nsIFrame *firstFrame;
      nsRect  usedRect; 
      PRUint32 lineFlags;
      result = it->GetLine(thisLine, &firstFrame, &lineFrameCount,usedRect,
                           &lineFlags);
      if (eSelectBeginLine == aPos->mAmount)
      {
        if (firstFrame)
        {
          nsPoint point;
          point.x = aPos->mDesiredX;
          point.y = 0;
          result = firstFrame->GetContentAndOffsetsFromPoint(*(context.get()),point,
                                           getter_AddRefs(aPos->mResultContent),
                                           aPos->mContentOffset,
                                           endoffset,
                                           aPos->mPreferLeft);
        }
      }
      else
      {
        if (firstFrame)
        {
          PRBool found = PR_FALSE;
          while(!found)
          {
            nsIFrame *nextFrame = firstFrame;;
            for (PRInt32 i=1;i<lineFrameCount;i++)//allready have 1st frame
              nextFrame->GetNextSibling(&nextFrame);

            nsPoint offsetPoint; //used for offset of result frame
            nsIView * view; //used for call of get offset from view
            nextFrame->GetOffsetFromView(offsetPoint, &view);
            usedRect.x = offsetPoint.x;
            usedRect.y = offsetPoint.y;

            nsPoint point;
            point.x = 2* offsetPoint.x; //2* just to be sure we are off the edge
            point.y = offsetPoint.y;

            result = nextFrame->GetContentAndOffsetsFromPoint(*(context.get()),
                                            point,
                                            getter_AddRefs(aPos->mResultContent),
                                            aPos->mContentOffset,
                                            endoffset,
                                            aPos->mPreferLeft);
            if (NS_SUCCEEDED(result))
              found = PR_TRUE;
            else
            {
              lineFrameCount--;
              if (lineFrameCount == 0)
                break;//just fail out
            }
          }
        }
      }
    }break;
    default: 
    {
      //this will use the nsFrameTraversal as the default peek method.
      //this should change to use geometry and also look to ALL the child lists
      //we need to set up line information to make sure we dont jump across line boundaries

      nsIFrame *blockFrame = this;
      nsIFrame *thisBlock;
      PRInt32   thisLine;
      nsCOMPtr<nsILineIterator> it; 
      while (NS_FAILED(result) && blockFrame)
      {
        thisBlock = blockFrame;
        result = blockFrame->GetParent(&blockFrame);
        if (NS_SUCCEEDED(result) && blockFrame){
          result = blockFrame->QueryInterface(nsILineIterator::GetIID(),getter_AddRefs(it));
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
      nsRect  nonUsedRect;
      PRInt32 lineFrameCount;
      PRUint32 lineFlags;

      result = it->GetLine(thisLine, &firstFrame, &lineFrameCount,nonUsedRect,
                           &lineFlags);
      if (NS_FAILED(result))
        return result;

      lastFrame = firstFrame;
      for (;lineFrameCount > 1;lineFrameCount --){
        result = lastFrame->GetNextSibling(&lastFrame);
        if (NS_FAILED(result)){
          NS_ASSERTION(0,"should not be reached nsFrame\n");
          return NS_ERROR_FAILURE;
        }
      }


      //END LINE DATA CODE
      if ((aPos->mDirection == eDirNext && lastFrame == this)
        ||(aPos->mDirection == eDirPrevious && firstFrame == this))
      {
        if (aPos->mAmount != eSelectWord)
        {
          aPos->mPreferLeft = (PRBool)!(aPos->mPreferLeft);//drift to other side
          aPos->mAmount = eSelectNoAmount;
        }
        else{
          if (aPos->mEatingWS)//done finding what we wanted
            return NS_OK;
          if (aPos->mDirection == eDirNext)
          {
            aPos->mPreferLeft = (PRBool)!(aPos->mPreferLeft);//drift to other side
            aPos->mAmount = eSelectNoAmount;
          }
        }

      }
      if (aPos->mAmount == eSelectDir)
        aPos->mAmount = eSelectNoAmount;//just get to next frame.
      nsCOMPtr<nsIBidirectionalEnumerator> frameTraversal;
      result = NS_NewFrameTraversal(getter_AddRefs(frameTraversal),LEAF,this);
      if (NS_FAILED(result))
        return result;
      nsISupports *isupports = nsnull;
      if (aPos->mDirection == eDirNext)
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
      if (aPos->mDirection == eDirNext)
        aPos->mStartOffset = 0;
      else
        aPos->mStartOffset = -1;
      return newFrame->PeekOffset(aPos);
    }
  }                          
  return result;
}

nsresult nsFrame::GetClosestViewForFrame(nsIFrame *aFrame, nsIView **aView)
{
  if (!aView)
    return NS_ERROR_NULL_POINTER;

  nsresult result = NS_OK;

  *aView = 0;

  nsIFrame *parent = aFrame;

  while (parent && !*aView)
  {
    result = parent->GetView(aView);

    if (NS_FAILED(result))
      return result;

    if (!*aView)
    {
      result = parent->GetParent(&parent);

      if (NS_FAILED(result))
        return result;
    }
  }

  return result;
}

//-----------------------------------------------------------------------------------


 /********************************************************
* Refreshes each content's frame
*********************************************************/
static void RefreshAllContentFrames(nsIFrame * aFrame, nsIContent * aContent)
{
  nsIContent* frameContent;
  aFrame->GetContent(&frameContent);
  if (frameContent == aContent) {
    ForceDrawFrame((nsFrame *)aFrame);
  }
  NS_IF_RELEASE(frameContent);

  aFrame->FirstChild(nsnull, &aFrame);
  while (aFrame) {
    RefreshAllContentFrames(aFrame, aContent);
    aFrame->GetNextSibling(&aFrame);
  }
}

/********************************************************
* Refreshes each content's frame
*********************************************************/

/**
  *
 */
void ForceDrawFrame(nsFrame * aFrame)//, PRBool)
{
  if (aFrame == nsnull) {
    return;
  }
  nsRect    rect;
  nsIView * view;
  nsPoint   pnt;
  aFrame->GetOffsetFromView(pnt, &view);
  aFrame->GetRect(rect);
  rect.x = pnt.x;
  rect.y = pnt.y;
  if (view != nsnull) {
    nsIViewManager * viewMgr;
    view->GetViewManager(viewMgr);
    if (viewMgr != nsnull) {
      viewMgr->UpdateView(view, rect, 0);
      NS_RELEASE(viewMgr);
    }
    //viewMgr->UpdateView(view, rect, NS_VMREFRESH_DOUBLE_BUFFER | NS_VMREFRESH_IMMEDIATE);
  }

}

static void
GetLastLeaf(nsIFrame **aFrame)
{
  if (!aFrame || !*aFrame)
    return;
  nsIFrame *child = *aFrame;
  nsresult result;
  nsIFrame *lookahead = nsnull;
  while (1){
    result = child->FirstChild(nsnull, &lookahead);
    if (NS_FAILED(result) || !lookahead)
      return;//nothing to do
    child = lookahead;
    while (NS_SUCCEEDED(child->GetNextSibling(&lookahead)) && lookahead)
      child = lookahead;
    *aFrame = child;
  }
  *aFrame = child;
}

#ifdef NS_DEBUG
static void
GetTagName(nsFrame* aFrame, nsIContent* aContent, PRIntn aResultSize,
           char* aResult)
{
  char namebuf[40];
  namebuf[0] = 0;
  if (nsnull != aContent) {
    nsIAtom* tag;
    aContent->GetTag(tag);
    if (nsnull != tag) {
      nsAutoString tmp;
      tag->ToString(tmp);
      tmp.ToCString(namebuf, sizeof(namebuf));
      NS_RELEASE(tag);
    }
  }
  PR_snprintf(aResult, aResultSize, "%s@%p", namebuf, aFrame);
}

void
nsFrame::Trace(const char* aMethod, PRBool aEnter)
{
  if (NS_FRAME_LOG_TEST(gLogModule, NS_FRAME_TRACE_CALLS)) {
    char tagbuf[40];
    GetTagName(this, mContent, sizeof(tagbuf), tagbuf);
    PR_LogPrint("%s: %s %s", tagbuf, aEnter ? "enter" : "exit", aMethod);
  }
}

void
nsFrame::Trace(const char* aMethod, PRBool aEnter, nsReflowStatus aStatus)
{
  if (NS_FRAME_LOG_TEST(gLogModule, NS_FRAME_TRACE_CALLS)) {
    char tagbuf[40];
    GetTagName(this, mContent, sizeof(tagbuf), tagbuf);
    PR_LogPrint("%s: %s %s, status=%scomplete%s",
                tagbuf, aEnter ? "enter" : "exit", aMethod,
                NS_FRAME_IS_NOT_COMPLETE(aStatus) ? "not" : "",
                (NS_FRAME_REFLOW_NEXTINFLOW & aStatus) ? "+reflow" : "");
  }
}

void
nsFrame::TraceMsg(const char* aFormatString, ...)
{
  if (NS_FRAME_LOG_TEST(gLogModule, NS_FRAME_TRACE_CALLS)) {
    // Format arguments into a buffer
    char argbuf[200];
    va_list ap;
    va_start(ap, aFormatString);
    PR_vsnprintf(argbuf, sizeof(argbuf), aFormatString, ap);
    va_end(ap);

    char tagbuf[40];
    GetTagName(this, mContent, sizeof(tagbuf), tagbuf);
    PR_LogPrint("%s: %s", tagbuf, argbuf);
  }
}

void
nsFrame::VerifyDirtyBitSet(nsIFrame* aFrameList)
{
  for (nsIFrame*f = aFrameList; f; f->GetNextSibling(&f)) {
    nsFrameState  frameState;
    f->GetFrameState(&frameState);
    NS_ASSERTION(frameState & NS_FRAME_IS_DIRTY, "dirty bit not set");
  }
}
#endif
