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

#include "nsITimer.h"
#include "nsITimerCallback.h"

#include "nsIComponentManager.h"
#include "nsIFrameSelection.h"
#include "nsIFrame.h"
#include "nsIDOMNode.h"
#include "nsIDOMRange.h"
#include "nsIDOMSelection.h"
#include "nsIDOMCharacterData.h"
#include "nsIContent.h"
#include "nsIPresShell.h"
#include "nsIRenderingContext.h"
#include "nsIDeviceContext.h"
#include "nsIView.h"
#include "nsIViewManager.h"
#include "nsIPresContext.h"
#include "nsILookAndFeel.h"
#include "nsWidgetsCID.h"			// for NS_LOOKANDFEEL_CID

#include "nsCaret.h"


static NS_DEFINE_IID(kISupportsIID, 		NS_ISUPPORTS_IID);
static NS_DEFINE_IID(kICaretID, NS_ICARET_IID);

static NS_DEFINE_IID(kLookAndFeelCID,  NS_LOOKANDFEEL_CID);

//-----------------------------------------------------------------------------
nsCaret::nsCaret()
:	mPresShell(nsnull)
,	mBlinkTimer(nsnull)
,	mBlinkRate(500)
, mCaretWidth(20)
,	mVisible(PR_FALSE)
,	mReadOnly(PR_TRUE)
,	mDrawn(PR_FALSE)
, mRendContext(nsnull)
, mLastCaretFrame(nsnull)
, mLastContentOffset(0)
{
  NS_INIT_REFCNT();
}


//-----------------------------------------------------------------------------
nsCaret::~nsCaret()
{
	KillTimer();
}

//-----------------------------------------------------------------------------
NS_IMETHODIMP nsCaret::Init(nsIPresShell *inPresShell)
{
	if (!inPresShell)
		return NS_ERROR_NULL_POINTER;
	
	mPresShell = inPresShell;		// the presshell owns us, so no addref
	
  nsILookAndFeel* touchyFeely;
  if (NS_SUCCEEDED(nsComponentManager::CreateInstance(kLookAndFeelCID, nsnull, nsILookAndFeel::GetIID(), (void**)&touchyFeely)))
  {
    PRInt32	tempInt;
    
    if (NS_SUCCEEDED(touchyFeely->GetMetric(nsILookAndFeel::eMetric_CaretWidthTwips, tempInt)))
      mCaretWidth = (nscoord)tempInt;
    if (NS_SUCCEEDED(touchyFeely->GetMetric(nsILookAndFeel::eMetric_CaretBlinkTime, tempInt)))
      mBlinkRate = (PRUint32)tempInt;
    
    NS_RELEASE(touchyFeely);
  }
  
	// get the selection from the pres shell, and set ourselves up as a selection
	// listener
	
  nsCOMPtr<nsIDOMSelection> domSelection;
  if (NS_SUCCEEDED(mPresShell->GetSelection(SELECTION_NORMAL, getter_AddRefs(domSelection))))
  {
		domSelection->AddSelectionListener(this);
	}
	
	// set up the blink timer
	if (mVisible)
	{
		nsresult	err = StartBlinking();
		if (NS_FAILED(err))
			return err;
	}
	
	return NS_OK;
}



//-----------------------------------------------------------------------------
NS_IMPL_ADDREF(nsCaret);
NS_IMPL_RELEASE(nsCaret);
//-----------------------------------------------------------------------------
NS_IMETHODIMP nsCaret::QueryInterface(const nsIID& aIID,
                                     void** aInstancePtrResult)
{
  NS_PRECONDITION(nsnull != aInstancePtrResult, "null pointer");
  if (nsnull == aInstancePtrResult) {
    return NS_ERROR_NULL_POINTER;
  }
  if (aIID.Equals(kISupportsIID)) {
    *aInstancePtrResult = (void*)(nsISupports*)(nsICaret*)this;		// whoo boy
    NS_ADDREF_THIS();
    return NS_OK;
  }
  if (aIID.Equals(nsICaret::GetIID())) {
    *aInstancePtrResult = (void*)(nsICaret*)this;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  if (aIID.Equals(nsIDOMSelectionListener::GetIID())) {
    *aInstancePtrResult = (void*)(nsIDOMSelectionListener*)this;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  return NS_NOINTERFACE;
}


//-----------------------------------------------------------------------------
NS_IMETHODIMP nsCaret::SetCaretVisible(PRBool inMakeVisible)
{
	mVisible = inMakeVisible;
	nsresult	err = NS_OK;
	if (mVisible)
		err = StartBlinking();
	else
		err = StopBlinking();
		
	return err;
}


//-----------------------------------------------------------------------------
NS_IMETHODIMP nsCaret::SetCaretReadOnly(PRBool inMakeReadonly)
{
	mReadOnly = inMakeReadonly;
	return NS_OK;
}


//-----------------------------------------------------------------------------
NS_IMETHODIMP nsCaret::Refresh(nsIView *aView, nsIRenderingContext& inRendContext, const nsRect& aDirtyRect)
{
	// a refresh is different from a simple redraw, in that it does not affect
	// the timer, or toggle the drawn status. It's used to redraw the caret
	// when painting happens, beacuse this will have drawn over the caret.
	if (mVisible && mDrawn)
	{
		RefreshDrawCaret(aView, inRendContext, aDirtyRect);
	}
	
	return NS_OK;
}


//-----------------------------------------------------------------------------
NS_IMETHODIMP nsCaret::GetWindowRelativeCoordinates(nsPoint& outCoordinates, PRBool& outIsCollapsed)
{
	if (!mPresShell)
		return NS_ERROR_NOT_INITIALIZED;
		
	nsCOMPtr<nsIDOMSelection> domSelection;
	nsresult err = mPresShell->GetSelection(SELECTION_NORMAL,getter_AddRefs(domSelection));
	if (NS_FAILED(err))
		return err;
		
	if (!domSelection)
		return NS_ERROR_NOT_INITIALIZED;		// no selection
	
	// fill in defaults for failure
	outCoordinates.x = -1;
	outCoordinates.y = -1;
	outIsCollapsed = PR_FALSE;
	
	err = domSelection->GetIsCollapsed(&outIsCollapsed);
	if (NS_FAILED(err))	
		return err;
		
	// code in progress
	nsCOMPtr<nsIDOMNode>	focusNode;
	
	err = domSelection->GetFocusNode(getter_AddRefs(focusNode));
	if (NS_FAILED(err))
		return err;
	if (!focusNode)
		return NS_ERROR_FAILURE;
	
	PRInt32	focusOffset;
	err = domSelection->GetFocusOffset(&focusOffset);
	if (NS_FAILED(err))
		return err;
		
	// is this a text node?
	nsCOMPtr<nsIDOMCharacterData>	nodeAsText = do_QueryInterface(focusNode);
	// note that we only work with text nodes here, unlike when drawing the caret.
	// this is because this routine is intended for IME support, which only cares about text.
	if (!nodeAsText)
		return NS_ERROR_UNEXPECTED;
	
	nsCOMPtr<nsIContent>contentNode = do_QueryInterface(focusNode);
	if (!contentNode)
		return NS_ERROR_FAILURE;

	// find the frame that contains the content node that has focus
	nsIFrame*	theFrame = nsnull;

  //get frame selection and find out what frame to use...
  nsCOMPtr<nsIFrameSelection> frameSelection;
  err = mPresShell->GetFrameSelection(getter_AddRefs(frameSelection));
	if (NS_FAILED(err) || !frameSelection)
		return err; 
  err = frameSelection->GetFrameForNodeOffset(contentNode, focusOffset, &theFrame);
	if (NS_FAILED(err))
		return err;
	
	nsPoint		viewOffset(0, 0);
	nsIView		*drawingView;			// views are not refcounted
	GetViewForRendering(theFrame, eTopLevelWindowCoordinates, viewOffset, drawingView);
	if (!drawingView)
		return NS_ERROR_UNEXPECTED;

	// ramp up to make a rendering context for measuring text.
	// First, we get the pres context ...
	nsCOMPtr<nsIPresContext> presContext;
	err = mPresShell->GetPresContext(getter_AddRefs(presContext));
	if (NS_FAILED(err))
		return err;
	
	// ... then get a device context
	nsCOMPtr<nsIDeviceContext> 		dx;
	err = presContext->GetDeviceContext(getter_AddRefs(dx));
	if (NS_FAILED(err))
		return err;
	if (!dx)
		return NS_ERROR_UNEXPECTED;

	// ... then tell it to make a rendering context
	nsCOMPtr<nsIRenderingContext> rendContext;	
	err = dx->CreateRenderingContext(drawingView, *getter_AddRefs(rendContext));						
	if (NS_FAILED(err))
		return err;
	if (!rendContext)
		return NS_ERROR_UNEXPECTED;

	// now we can measure the offset into the frame.
	nsPoint		framePos(0, 0);
	theFrame->GetPointFromOffset(presContext, rendContext, focusOffset, &framePos);

	// now add the frame offset to the view offset, and we're done
	viewOffset += framePos;
	outCoordinates = viewOffset;
	
	return NS_OK;
}

//-----------------------------------------------------------------------------
NS_IMETHODIMP nsCaret::ClearFrameRefs(nsIFrame* aFrame)
{

	if (mLastCaretFrame == aFrame)
	{
		mLastCaretFrame = nsnull;			// frames are not refcounted.
		mLastContentOffset = 0;
	}
	
	// blow away the rendering context, because the frames may well get
	// reflowed before we draw again
	NS_IF_RELEASE(mRendContext);		// this nulls the ptr

	// should we just call StopBlinking() here?
	
	return NS_OK;	
}


#ifdef XP_MAC
#pragma mark -
#endif

//-----------------------------------------------------------------------------
NS_IMETHODIMP nsCaret::NotifySelectionChanged()
{

	if (mVisible)
	{
		StopBlinking();
		StartBlinking();
	}
	
	return NS_OK;
}

#ifdef XP_MAC
#pragma mark -
#endif

//-----------------------------------------------------------------------------
void nsCaret::KillTimer()
{
	if (mBlinkTimer)
	{
		mBlinkTimer->Cancel();
		NS_RELEASE(mBlinkTimer);
	}
}


//-----------------------------------------------------------------------------
nsresult nsCaret::PrimeTimer()
{
	KillTimer();
	
	// set up the blink timer
	if (!mReadOnly && mBlinkRate > 0)
	{
		nsresult	err = NS_NewTimer(&mBlinkTimer);
		
		if (NS_FAILED(err))
			return err;
		
		mBlinkTimer->Init(CaretBlinkCallback, this, mBlinkRate);
	}

	return NS_OK;
}


//-----------------------------------------------------------------------------
nsresult nsCaret::StartBlinking()
{
	PrimeTimer();

	ToggleDrawnStatus();
	DrawCaret();		// draw it right away
	
	return NS_OK;
}


//-----------------------------------------------------------------------------
nsresult nsCaret::StopBlinking()
{
	if (mDrawn)			// erase the caret if necessary
	{
		ToggleDrawnStatus();
		DrawCaret();
	}
	
	// blow away the rendering context, because the frames may well get
	// reflowed before we draw again
	NS_IF_RELEASE(mRendContext);		// this nulls the ptr
	
	KillTimer();
	
	return NS_OK;
}


//-----------------------------------------------------------------------------
// Get the nsIFrame and the content offset for the current caret position.
// Returns PR_TRUE if we should go ahead and draw, PR_FALSE otherwise. 
//
PRBool nsCaret::SetupDrawingFrameAndOffset()
{
	if (!mDrawn)
	{
		if (!mLastCaretFrame) return PR_FALSE;
		
		// check there that our cached offset into the frame content
		// is still valid. It can go bad when splitting nodes
		PRInt32		contentStart, contentEnd;
		mLastCaretFrame->GetOffsets(contentStart, contentEnd);
		
		if (mLastContentOffset < contentStart)
			mLastContentOffset = contentStart;
			
		if (mLastContentOffset > contentEnd)
			mLastContentOffset = contentEnd;
			
		return PR_TRUE;
	}
	
	nsCOMPtr<nsIDOMSelection> domSelection;
	nsresult err = mPresShell->GetSelection(SELECTION_NORMAL, getter_AddRefs(domSelection));
	if (!NS_SUCCEEDED(err) || !domSelection)
		return PR_FALSE;

	PRBool isCollapsed;

	if (domSelection && NS_SUCCEEDED(domSelection->GetIsCollapsed(&isCollapsed)) && isCollapsed)
	{
		// start and end parent should be the same since we are collapsed
		nsCOMPtr<nsIDOMNode>	focusNode;
		PRInt32	contentOffset;
		
		if (NS_SUCCEEDED(domSelection->GetFocusNode(getter_AddRefs(focusNode))) && focusNode &&
				NS_SUCCEEDED(domSelection->GetFocusOffset(&contentOffset)))
		{
			nsCOMPtr<nsIContent>contentNode = do_QueryInterface(focusNode);
      
			if (contentNode)
			{
				PRBool  canContainChildren;
			  
		      // see if we have an offset between child nodes, or an offset into a text
		      // node.
				if (NS_SUCCEEDED(contentNode->CanContainChildren(canContainChildren)) && canContainChildren)
				{
					// point the caret to the start of the child node
					nsCOMPtr<nsIContent> childNode;
					contentNode->ChildAt(contentOffset, *getter_AddRefs(childNode));
					if (childNode)
					{
						contentNode = childNode;
						contentOffset = 0;
					}
				}
				else
				{
				//nsCOMPtr<nsIDOMCharacterData>	nodeAsText = do_QueryInterface(focusNode);
				//NS_ASSERTION(nodeAsText, "Should have a text node here");

				// we can be in a text node, or a BR node here.
				}
			
				nsIFrame*	theFrame = nsnull;

				//get frame selection and find out what frame to use...
				nsCOMPtr<nsIFrameSelection> frameSelection;
				err = mPresShell->GetFrameSelection(getter_AddRefs(frameSelection));
				if (NS_FAILED(err) || !frameSelection)
					return PR_FALSE;
				
				err = frameSelection->GetFrameForNodeOffset(contentNode, contentOffset, &theFrame);
				if (NS_FAILED(err))
					return PR_FALSE;
				else
				{

					// mark the frame, so we get notified on deletion.
					// frames are never unmarked, which means that we'll touch every frame we visit.
					// this is not ideal.
					nsFrameState state;
					theFrame->GetFrameState(&state);
					state |= NS_FRAME_EXTERNAL_REFERENCE;
					theFrame->SetFrameState(state);
					
					mLastCaretFrame = theFrame;
					mLastContentOffset = contentOffset;
					return PR_TRUE;
				}
			}
		}
	}

	return PR_FALSE;
}


//-----------------------------------------------------------------------------
void nsCaret::GetViewForRendering(nsIFrame *caretFrame, EViewCoordinates coordType, nsPoint &viewOffset, nsIView* &outView)
{
  if (!caretFrame) return;
	outView = nsnull;
	
	nsIView* theView = nsnull;
	NS_ASSERTION(caretFrame, "Should have frame here");
	caretFrame->GetOffsetFromView(viewOffset, &theView);
	if (theView == nsnull) return;
	
	nsIView* returnView = nsnull;
	
	nscoord		x, y;
	
	do {
		theView->GetPosition(&x, &y);
		viewOffset.x += x;
		viewOffset.y += y;

		if (!returnView)
		{
			nsCOMPtr<nsIWidget>	viewWidget;
			theView->GetWidget(*getter_AddRefs(viewWidget));
			
			if (viewWidget)
			{
				returnView = theView;
			
				if (coordType == eViewCoordinates)
					break;
			}
		}		
		
		theView->GetParent(theView);
	} while (theView);
		
	outView = returnView;
}


//-----------------------------------------------------------------------------
void nsCaret::DrawCaretWithContext(nsIRenderingContext& inRendContext)
{
	
	NS_ASSERTION(mLastCaretFrame != nsnull, "Should have a frame here");
	
	nsRect		frameRect;
	mLastCaretFrame->GetRect(frameRect);
	frameRect.x = 0;			// the origin is accounted for in GetViewForRendering()
	frameRect.y = 0;
	
	if (0 == frameRect.height)
	{
	  frameRect.height = 200;
	  frameRect.y += 200;
	}
	
	nsPoint		viewOffset(0, 0);
	nsIView		*drawingView;
	GetViewForRendering(mLastCaretFrame, eViewCoordinates, viewOffset, drawingView);

	if (drawingView == nsnull)
		return;
	
	frameRect += viewOffset;

	inRendContext.PushState();
	
	nsCOMPtr<nsIPresContext> presContext;
	if (NS_SUCCEEDED(mPresShell->GetPresContext(getter_AddRefs(presContext))))
	{
		nsPoint		framePos(0, 0);
		
		mLastCaretFrame->GetPointFromOffset(presContext, &inRendContext, mLastContentOffset, &framePos);
		frameRect += framePos;
		
		//printf("Content offset %ld, frame offset %ld\n", focusOffset, framePos.x);
		
		frameRect.width = mCaretWidth;
		mCaretRect = frameRect;
		
		/*
		if (mReadOnly)
			inRendContext.SetColor(NS_RGB(85, 85, 85));		// we are drawing it; gray
	  */
	  
		inRendContext.InvertRect(mCaretRect);
	}
	
	PRBool dummy;
	inRendContext.PopState(dummy);
}

//-----------------------------------------------------------------------------
void nsCaret::DrawCaret()
{
	if (! SetupDrawingFrameAndOffset())
		return;
	
	NS_ASSERTION(mLastCaretFrame != nsnull, "Should have a frame here");
	
	nsPoint		viewOffset(0, 0);
	nsIView		*drawingView = nsnull;
	GetViewForRendering(mLastCaretFrame, eViewCoordinates, viewOffset, drawingView);
	
	if (drawingView)
	{
		// make a rendering context for the first view that has a widget
		if (!mRendContext)
		{
			nsCOMPtr<nsIPresContext> presContext;
			if (NS_SUCCEEDED(mPresShell->GetPresContext(getter_AddRefs(presContext))))
			{
				nsCOMPtr<nsIDeviceContext> 		dx;

				if (NS_SUCCEEDED(presContext->GetDeviceContext(getter_AddRefs(dx))) && dx)
				{
					dx->CreateRenderingContext(drawingView, mRendContext);						
					if (!mRendContext) return;
				}
			}

			mRendContext->PushState();		// save the state. We'll pop then push on drawing
		}
		
		DrawCaretWithContext(*mRendContext);
		NS_RELEASE(mRendContext);
	}
	
}


//-----------------------------------------------------------------------------
void nsCaret::RefreshDrawCaret(nsIView *aView, nsIRenderingContext& inRendContext, const nsRect& aDirtyRect)
{
	if (! SetupDrawingFrameAndOffset())
		return;

	NS_ASSERTION(mLastCaretFrame != nsnull, "Should have a frame here");
	
	nsPoint		viewOffset(0, 0);
	nsIView		*drawingView;
	//GetViewForRendering(viewOffset, drawingView);

	mLastCaretFrame->GetOffsetFromView(viewOffset, &drawingView);

	// are we in the view that is being painted?
	if (drawingView == nsnull || drawingView != aView)
		return;
		
	DrawCaretWithContext(inRendContext);
}

#ifdef XP_MAC
#pragma mark -
#endif

//-----------------------------------------------------------------------------
/* static */
void nsCaret::CaretBlinkCallback(nsITimer *aTimer, void *aClosure)
{
	nsCaret		*theCaret = NS_REINTERPRET_CAST(nsCaret*, aClosure);
	if (!theCaret) return;
	
	theCaret->ToggleDrawnStatus();
	theCaret->DrawCaret();
	theCaret->PrimeTimer();
}


//-----------------------------------------------------------------------------
nsresult NS_NewCaret(nsICaret** aInstancePtrResult)
{
  NS_PRECONDITION(nsnull != aInstancePtrResult, "null ptr");
  
  nsCaret* caret = new nsCaret();
  if (nsnull == caret)
      return NS_ERROR_OUT_OF_MEMORY;
      
  return caret->QueryInterface(kICaretID, (void**) aInstancePtrResult);
}

