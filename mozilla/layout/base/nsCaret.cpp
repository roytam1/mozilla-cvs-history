/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: NPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is 
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Pierre Phaneuf <pp@ludusdesign.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or 
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the NPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */


#include "nsCOMPtr.h"

#include "nsITimer.h"

#include "nsIComponentManager.h"
#include "nsIServiceManager.h"
#include "nsIFrameSelection.h"
#include "nsIFrame.h"
#include "nsIDOMNode.h"
#include "nsIDOMRange.h"
#include "nsIFontMetrics.h"
#include "nsISelection.h"
#include "nsISelectionPrivate.h"
#include "nsIDOMCharacterData.h"
#include "nsIContent.h"
#include "nsIPresShell.h"
#include "nsIRenderingContext.h"
#include "nsIDeviceContext.h"
#include "nsIView.h"
#include "nsIScrollableView.h"
#include "nsIViewManager.h"
#include "nsIPresContext.h"
#include "nsILookAndFeel.h"
#include "nsWidgetsCID.h"     // for NS_LOOKANDFEEL_CID
#include "nsBlockFrame.h"
#include "nsISelectionController.h"

#include "nsCaret.h"

// Because of drawing issues, we currently always make a new RC. See bug 28068
// Before removing this, stuff will need to be fixed and tested on all platforms.
// For example, turning this off on Mac right now causes drawing problems on pages
// with form elements.
#define DONT_REUSE_RENDERING_CONTEXT

#ifdef IBMBIDI
//-------------------------------IBM BIDI--------------------------------------
// Mamdouh : Modifiaction of the caret to work with Bidi in the LTR and RTL
#include "nsIPref.h"
#include "nsLayoutAtoms.h"
//------------------------------END OF IBM BIDI--------------------------------
#endif //IBMBIDI

static NS_DEFINE_CID(kLookAndFeelCID,  NS_LOOKANDFEEL_CID);

//-----------------------------------------------------------------------------

nsCaret::nsCaret()
: mPresShell(nsnull)
, mBlinkRate(500)
, mCaretTwipsWidth(-1)
, mCaretPixelsWidth(1)
, mVisible(PR_FALSE)
, mDrawn(PR_FALSE)
, mReadOnly(PR_FALSE)
, mShowDuringSelection(PR_FALSE)
, mLastCaretFrame(nsnull)
, mLastCaretView(nsnull)
, mLastContentOffset(0)
{
  NS_INIT_ISUPPORTS();
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
  
  mPresShell = getter_AddRefs(NS_GetWeakReference(inPresShell));    // the presshell owns us, so no addref

  nsILookAndFeel* touchyFeely;
  if (NS_SUCCEEDED(nsComponentManager::CreateInstance(kLookAndFeelCID, nsnull, NS_GET_IID(nsILookAndFeel), (void**)&touchyFeely)))
  {
    PRInt32 tempInt;
    
    if (NS_SUCCEEDED(touchyFeely->GetMetric(nsILookAndFeel::eMetric_SingleLineCaretWidth, tempInt)))
      mCaretTwipsWidth = (nscoord)tempInt;
    if (NS_SUCCEEDED(touchyFeely->GetMetric(nsILookAndFeel::eMetric_CaretBlinkTime, tempInt)))
      mBlinkRate = (PRUint32)tempInt;
    if (NS_SUCCEEDED(touchyFeely->GetMetric(nsILookAndFeel::eMetric_ShowCaretDuringSelection, tempInt)))
      mShowDuringSelection = tempInt ? PR_TRUE : PR_FALSE;
    
    NS_RELEASE(touchyFeely);
  }
  
  // get the selection from the pres shell, and set ourselves up as a selection
  // listener
  
	
  nsCOMPtr<nsISelection> domSelection;
  nsCOMPtr<nsISelectionPrivate> privateSelection;
  nsCOMPtr<nsISelectionController> selCon = do_QueryReferent(mPresShell);
  if (selCon)
  {
    if (NS_SUCCEEDED(selCon->GetSelection(nsISelectionController::SELECTION_NORMAL, getter_AddRefs(domSelection))))
    {
      privateSelection = do_QueryInterface(domSelection);
      privateSelection->AddSelectionListener(this);
      mDomSelectionWeak = getter_AddRefs( NS_GetWeakReference(domSelection) );
    }
  }
  else
    return NS_ERROR_FAILURE;
  
  // set up the blink timer
  if (mVisible)
  {
    nsresult  err = StartBlinking();
    if (NS_FAILED(err))
      return err;
  }
#ifdef IBMBIDI
  PRBool isRTL;
  mBidiKeyboard = do_GetService("@mozilla.org/widget/bidikeyboard;1");
  mBidiKeyboard->IsLangRTL(&isRTL);
  mKeyboardRTL = isRTL;
#endif
  
  return NS_OK;
}



//-----------------------------------------------------------------------------
NS_IMPL_ADDREF(nsCaret);
NS_IMPL_RELEASE(nsCaret);
//-----------------------------------------------------------------------------
NS_IMETHODIMP nsCaret::QueryInterface(const nsIID& aIID,
                                     void** aInstancePtrResult)
{

  NS_PRECONDITION(aInstancePtrResult, "null pointer");
  if (!aInstancePtrResult)
  return NS_ERROR_NULL_POINTER;

  nsISupports* foundInterface;  

  if (aIID.Equals(NS_GET_IID(nsISupports)))
    foundInterface = (nsISupports*)(nsICaret*)this;   // whoo boy
  else if (aIID.Equals(NS_GET_IID(nsICaret)))
    foundInterface = (nsICaret*)this;
  else if (aIID.Equals(NS_GET_IID(nsISelectionListener)))
    foundInterface = (nsISelectionListener*)this;
  else
    foundInterface = nsnull;

  nsresult status;
  if (! foundInterface)
    status = NS_NOINTERFACE;
  else
  {
    NS_ADDREF(foundInterface);
    status = NS_OK;
  }

  *aInstancePtrResult = foundInterface;
  return status;
}

//-----------------------------------------------------------------------------
NS_IMETHODIMP nsCaret::GetCaretDOMSelection(nsISelection **aDOMSel)
{
  nsCOMPtr<nsISelection> sel(do_QueryReferent(mDomSelectionWeak));
  
  NS_IF_ADDREF(*aDOMSel = sel);

  return NS_OK;
}


//-----------------------------------------------------------------------------
NS_IMETHODIMP nsCaret::SetCaretDOMSelection(nsISelection *aDOMSel)
{
  NS_ENSURE_ARG_POINTER(aDOMSel);
  mDomSelectionWeak = getter_AddRefs( NS_GetWeakReference(aDOMSel) );   // weak reference to pres shell
  return NS_OK;
}


//-----------------------------------------------------------------------------
NS_IMETHODIMP nsCaret::SetCaretVisible(PRBool inMakeVisible)
{
  mVisible = inMakeVisible;
  nsresult  err = NS_OK;
  if (mVisible)
    err = StartBlinking();
  else
    err = StopBlinking();
    
  return err;
}

NS_IMETHODIMP nsCaret::GetCaretVisible(PRBool *outMakeVisible)
{
  NS_ENSURE_ARG_POINTER(outMakeVisible);
  *outMakeVisible = mVisible;
  return NS_OK;
}


//-----------------------------------------------------------------------------
NS_IMETHODIMP nsCaret::SetCaretReadOnly(PRBool inMakeReadonly)
{
  mReadOnly = inMakeReadonly;
  return NS_OK;
}


//-----------------------------------------------------------------------------
NS_IMETHODIMP nsCaret::GetCaretCoordinates(EViewCoordinates aRelativeToType, nsISelection *aDOMSel, nsRect *outCoordinates, PRBool *outIsCollapsed, nsIView **outView)
{
  if (!mPresShell)
    return NS_ERROR_NOT_INITIALIZED;
  if (!outCoordinates || !outIsCollapsed)
    return NS_ERROR_NULL_POINTER;
		
	nsCOMPtr<nsISelection> domSelection = aDOMSel;
	nsCOMPtr<nsISelectionPrivate> privateSelection(do_QueryInterface(domSelection));
  nsresult err;
  if (!domSelection)
    return NS_ERROR_NOT_INITIALIZED;    // no selection

  if (outView)
    *outView = nsnull;

  // fill in defaults for failure
  outCoordinates->x = -1;
  outCoordinates->y = -1;
  outCoordinates->width = -1;
  outCoordinates->height = -1;
  *outIsCollapsed = PR_FALSE;
  
  err = domSelection->GetIsCollapsed(outIsCollapsed);
  if (NS_FAILED(err)) 
    return err;
    
  nsCOMPtr<nsIDOMNode>  focusNode;
  
  err = domSelection->GetFocusNode(getter_AddRefs(focusNode));
  if (NS_FAILED(err))
    return err;
  if (!focusNode)
    return NS_ERROR_FAILURE;
  
  PRInt32 focusOffset;
  err = domSelection->GetFocusOffset(&focusOffset);
  if (NS_FAILED(err))
    return err;
    
/*
  // is this a text node?
  nsCOMPtr<nsIDOMCharacterData> nodeAsText = do_QueryInterface(focusNode);
  // note that we only work with text nodes here, unlike when drawing the caret.
  // this is because this routine is intended for IME support, which only cares about text.
  if (!nodeAsText)
    return NS_ERROR_UNEXPECTED;
*/  
  nsCOMPtr<nsIContent>contentNode = do_QueryInterface(focusNode);
  if (!contentNode)
    return NS_ERROR_FAILURE;

  //get frame selection and find out what frame to use...
  nsCOMPtr<nsIFrameSelection> frameSelection;
  nsCOMPtr<nsIPresShell> presShell = do_QueryReferent(mPresShell);
  if (presShell)
    err = presShell->GetFrameSelection(getter_AddRefs(frameSelection));
  else
    return NS_ERROR_FAILURE;
  if (NS_FAILED(err) || !frameSelection)
    return err?err : NS_ERROR_FAILURE;  

  // find the frame that contains the content node that has focus
  nsIFrame*       theFrame = nsnull;
  PRInt32         theFrameOffset = 0;
  PRBool hintRight;
  privateSelection->GetInterlinePosition(&hintRight);//translate hint.
  nsIFrameSelection::HINT hint;
  if (hintRight)
    hint = nsIFrameSelection::HINTRIGHT;
  else
    hint = nsIFrameSelection::HINTLEFT;
  err = frameSelection->GetFrameForNodeOffset(contentNode, focusOffset, hint, &theFrame, &theFrameOffset);
  if (NS_FAILED(err) || !theFrame)
    return err;
  
  nsPoint   viewOffset(0, 0);
  nsRect    clipRect;
  nsIView   *drawingView;     // views are not refcounted


  GetViewForRendering(theFrame, aRelativeToType, viewOffset, clipRect, &drawingView, outView);
  if (!drawingView)
    return NS_ERROR_UNEXPECTED;
  // ramp up to make a rendering context for measuring text.
  // First, we get the pres context ...
  nsCOMPtr<nsIPresContext> presContext;
  err = presShell->GetPresContext(getter_AddRefs(presContext));
  if (NS_FAILED(err))
    return err;
  
  // ... then get a device context
  nsCOMPtr<nsIDeviceContext>    dx;
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
  nsPoint   framePos(0, 0);
  theFrame->GetPointFromOffset(presContext, rendContext, theFrameOffset, &framePos);

  nsRect          frameRect;
  theFrame->GetRect(frameRect);
  
  // we don't need drawingView anymore so reuse that; reset viewOffset values for our purposes
  if (aRelativeToType == eClosestViewCoordinates)
  {
    theFrame->GetOffsetFromView(presContext, viewOffset, &drawingView);
    if (outView)
      *outView = drawingView;
  }
  // now add the frame offset to the view offset, and we're done
  viewOffset += framePos;
  outCoordinates->x = viewOffset.x;
  outCoordinates->y = viewOffset.y;
  outCoordinates->height = frameRect.height;
  outCoordinates->width  = mCaretTwipsWidth;
  
  return NS_OK;
}

//-----------------------------------------------------------------------------
NS_IMETHODIMP nsCaret::ClearFrameRefs(nsIFrame* aFrame)
{

  if (mLastCaretFrame == aFrame)
  {
    mLastCaretFrame = nsnull;     // frames are not refcounted.
    mLastCaretView = nsnull;
    mLastContentOffset = 0;
  }
  
  mDrawn = PR_FALSE;    // assume that the view has been cleared, and ensure
                        // that we don't try to use the frame.
  
  // should we just call StopBlinking() here?
  
  return NS_OK; 
}

NS_IMETHODIMP nsCaret::EraseCaret()
{
  if (mDrawn)
    DrawCaret();
  return NS_OK;
}


#ifdef XP_MAC
#pragma mark -
#endif

//-----------------------------------------------------------------------------
NS_IMETHODIMP nsCaret::NotifySelectionChanged(nsIDOMDocument *, nsISelection *aDomSel, short aReason)
{
  if (aReason & nsISelectionListener::MOUSEUP_REASON)//this wont do
    return NS_OK;

  nsCOMPtr<nsISelection> domSel(do_QueryReferent(mDomSelectionWeak));

  // The same caret is shared amongst the document and any text widgets it
  // may contain. This means that the caret could get notifications from
  // multiple selections.
  //
  // If this notification is for a selection that is not the one the
  // the caret is currently interested in (mDomSelectionWeak), then there
  // is nothing to do!

  if (domSel != aDomSel)
    return NS_OK;

  if (mVisible)
  {
    // Stop the caret from blinking in its previous location.
    StopBlinking();

    // Start the caret blinking in the new location.
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
  }
}


//-----------------------------------------------------------------------------
nsresult nsCaret::PrimeTimer()
{
  KillTimer();
  
  // set up the blink timer
  if (!mReadOnly && mBlinkRate > 0)
  {
    nsresult  err;
    mBlinkTimer = do_CreateInstance("@mozilla.org/timer;1", &err);
    
    if (NS_FAILED(err))
      return err;
    
    mBlinkTimer->InitWithFuncCallback(CaretBlinkCallback, this, mBlinkRate,
                                      nsITimer::TYPE_REPEATING_PRECISE);
  }

  return NS_OK;
}


//-----------------------------------------------------------------------------
nsresult nsCaret::StartBlinking()
{
  PrimeTimer();

  //NS_ASSERTION(!mDrawn, "Caret should not be drawn here");
  DrawCaret();    // draw it right away
  
  return NS_OK;
}


//-----------------------------------------------------------------------------
nsresult nsCaret::StopBlinking()
{
  if (mDrawn)     // erase the caret if necessary
    DrawCaret();
    
  KillTimer();
  
  return NS_OK;
}


//-----------------------------------------------------------------------------
// Get the nsIFrame and the content offset for the current caret position.
// Returns PR_TRUE if we should go ahead and draw, PR_FALSE otherwise. 
//
PRBool nsCaret::SetupDrawingFrameAndOffset()
{
  if (!mDomSelectionWeak)
    return PR_FALSE;

  
  nsCOMPtr<nsISelection> domSelection = do_QueryReferent(mDomSelectionWeak);
  nsCOMPtr<nsISelectionPrivate> privateSelection(do_QueryInterface(domSelection));
  if (!domSelection) return PR_FALSE;
  
  PRBool isCollapsed = PR_FALSE;
  domSelection->GetIsCollapsed(&isCollapsed);
  if (!mShowDuringSelection && !isCollapsed) return PR_FALSE;

  // start and end parent should be the same since we are collapsed
  nsCOMPtr<nsIDOMNode>  focusNode;
  domSelection->GetFocusNode(getter_AddRefs(focusNode));
  if (!focusNode) return PR_FALSE;
  
  PRInt32 contentOffset;
  if (NS_FAILED(domSelection->GetFocusOffset(&contentOffset)))
    return PR_FALSE;
  
  nsCOMPtr<nsIContent> contentNode = do_QueryInterface(focusNode);
  if (!contentNode) return PR_FALSE;
      
  //get frame selection and find out what frame to use...
  nsCOMPtr<nsIPresShell> presShell = do_QueryReferent(mPresShell);
  if (!presShell)
    return PR_FALSE;
    
  nsCOMPtr<nsIFrameSelection> frameSelection;
  presShell->GetFrameSelection(getter_AddRefs(frameSelection));
  if (!frameSelection)
    return PR_FALSE;

  PRBool hintRight;
  privateSelection->GetInterlinePosition(&hintRight);//translate hint.
  nsIFrameSelection::HINT hint;
  hint = (hintRight) ? nsIFrameSelection::HINTRIGHT : nsIFrameSelection::HINTLEFT;

  nsIFrame* theFrame = nsnull;
  PRInt32   theFrameOffset = 0;

  nsresult rv = frameSelection->GetFrameForNodeOffset(contentNode, contentOffset, hint, &theFrame, &theFrameOffset);
  if (NS_FAILED(rv) || !theFrame)
    return PR_FALSE;

#ifdef IBMBIDI
  PRUint8 bidiLevel=0;
  // Mamdouh : modification of the caret to work at rtl and ltr with Bidi
  const nsStyleVisibility* vis;
  theFrame->GetStyleData(eStyleStruct_Visibility, (const nsStyleStruct*&) vis);
  //
  // Direction Style from this->GetStyleData()
  // now in (visibility->mDirection)
  // ------------------
  // NS_STYLE_DIRECTION_LTR : LTR or Default
  // NS_STYLE_DIRECTION_RTL
  // NS_STYLE_DIRECTION_INHERIT
  nsCOMPtr<nsIPresContext> presContext;
  rv = presShell->GetPresContext(getter_AddRefs(presContext));

  PRBool bidiEnabled = PR_FALSE;
  if (presContext)
    presContext->GetBidiEnabled(&bidiEnabled);

  if (bidiEnabled)
  {
    presShell->GetCaretBidiLevel(&bidiLevel);
    if (bidiLevel & BIDI_LEVEL_UNDEFINED)
    {
      PRUint8 newBidiLevel;
      bidiLevel &= ~BIDI_LEVEL_UNDEFINED;
      // There has been a reflow, so we reset the cursor Bidi level to the level of the current frame
      if (!presContext) // Use the style default or default to 0
        newBidiLevel = (vis) ? vis->mDirection : 0;
      else
      {
        theFrame->GetBidiProperty(presContext, nsLayoutAtoms::embeddingLevel,
                                  (void**)&newBidiLevel, sizeof(newBidiLevel) );
        presShell->SetCaretBidiLevel(newBidiLevel);
        bidiLevel = newBidiLevel;
      }
    }

    PRInt32 start;
    PRInt32 end;
    nsIFrame* frameBefore;
    nsIFrame* frameAfter;
    PRUint8 levelBefore;     // Bidi level of the character before the caret
    PRUint8 levelAfter;      // Bidi level of the character after the caret

    theFrame->GetOffsets(start, end);
    if (start == 0 || end == 0 || start == theFrameOffset || end == theFrameOffset)
    {
      /* Boundary condition, we need to know the Bidi levels of the characters before and after the cursor */
      if (NS_SUCCEEDED(frameSelection->GetPrevNextBidiLevels(presContext, contentNode, contentOffset,
                                                             &frameBefore, &frameAfter,
                                                             &levelBefore, &levelAfter)))
      {
        if ((levelBefore != levelAfter) || (bidiLevel != levelBefore))
        {
          bidiLevel = PR_MAX(bidiLevel, PR_MIN(levelBefore, levelAfter));                                 // rule c3
          bidiLevel = PR_MIN(bidiLevel, PR_MAX(levelBefore, levelAfter));                                 // rule c4
          if (bidiLevel == levelBefore                                                                    // rule c1
              || bidiLevel > levelBefore && bidiLevel < levelAfter && !((bidiLevel ^ levelBefore) & 1)    // rule c5
              || bidiLevel < levelBefore && bidiLevel > levelAfter && !((bidiLevel ^ levelBefore) & 1))   // rule c9
          {
            if (theFrame != frameBefore)
            {
              if (frameBefore) // if there is a frameBefore, move into it, setting HINTLEFT to make sure we stay there
              {
                theFrame = frameBefore;
                theFrame->GetOffsets(start, end);
                theFrameOffset = end;
//              frameSelection->SetHint(nsIFrameSelection::HINTLEFT);
              }
              else 
              {
                // if there is no frameBefore, we must be at the beginning of the line
                // so we stay with the current frame.
                // Exception: when the first frame on the line has a different Bidi level from the paragraph level, there is no
                // real frame for the caret to be in. We have to find the first frame whose level is the same as the
                // paragraph level, and put the caret at the end of the frame before that.
                PRUint8 baseLevel;
                frameAfter->GetBidiProperty(presContext, nsLayoutAtoms::baseLevel,
                                            (void**)&baseLevel, sizeof(baseLevel) );
                if (baseLevel != levelAfter)
                {
                  if (NS_SUCCEEDED(frameSelection->GetFrameFromLevel(presContext, frameAfter, eDirNext, baseLevel, &theFrame)))
                  {
                    theFrame->GetOffsets(start, end);
                    theFrame->GetBidiProperty(presContext, nsLayoutAtoms::embeddingLevel,
                                              (void**)&levelAfter, sizeof(levelAfter) );
                    if (baseLevel & 1) // RTL paragraph: caret to the right of the rightmost character
                      theFrameOffset = (levelAfter & 1) ? start : end;
                    else               // LTR paragraph: caret to the left of the leftmost character
                      theFrameOffset = (levelAfter & 1) ? end : start;
                  }
                }
              }
            }
          }
          else if (bidiLevel == levelAfter                                                                   // rule c2
                   || bidiLevel > levelBefore && bidiLevel < levelAfter && !((bidiLevel ^ levelAfter) & 1)   // rule c6  
                   || bidiLevel < levelBefore && bidiLevel > levelAfter && !((bidiLevel ^ levelAfter) & 1))  // rule c10
          {
            if (theFrame != frameAfter)
            {
              if (frameAfter)
              {
                // if there is a frameAfter, move into it, setting HINTRIGHT to make sure we stay there
                theFrame = frameAfter;
                theFrame->GetOffsets(start, end);
                theFrameOffset = start;
//              frameSelection->SetHint(nsIFrameSelection::HINTRIGHT);
              }
              else 
              {
                // if there is no frameAfter, we must be at the end of the line
                // so we stay with the current frame.
                //
                // Exception: when the last frame on the line has a different Bidi level from the paragraph level, there is no
                // real frame for the caret to be in. We have to find the last frame whose level is the same as the
                // paragraph level, and put the caret at the end of the frame after that.
                PRUint8 baseLevel;
                frameBefore->GetBidiProperty(presContext, nsLayoutAtoms::baseLevel,
                                             (void**)&baseLevel, sizeof(baseLevel) );
                if (baseLevel != levelBefore)
                {
                  if (NS_SUCCEEDED(frameSelection->GetFrameFromLevel(presContext, frameBefore, eDirPrevious, baseLevel, &theFrame)))
                  {
                    theFrame->GetOffsets(start, end);
                    theFrame->GetBidiProperty(presContext, nsLayoutAtoms::embeddingLevel,
                                              (void**)&levelBefore, sizeof(levelBefore) );
                    if (baseLevel & 1) // RTL paragraph: caret to the left of the leftmost character
                      theFrameOffset = (levelBefore & 1) ? end : start;
                    else               // RTL paragraph: caret to the right of the rightmost character
                      theFrameOffset = (levelBefore & 1) ? start : end;
                  }
                }
              }
            }
          }
          else if (bidiLevel > levelBefore && bidiLevel < levelAfter  // rule c7/8
                   && !((levelBefore ^ levelAfter) & 1)               //  before and after have the same parity
                   && ((bidiLevel ^ levelAfter) & 1))                 // cursor has different parity
          {
            if (NS_SUCCEEDED(frameSelection->GetFrameFromLevel(presContext, frameAfter, eDirNext, bidiLevel, &theFrame)))
            {
              theFrame->GetOffsets(start, end);
              theFrame->GetBidiProperty(presContext, nsLayoutAtoms::embeddingLevel,
                                        (void**)&levelAfter, sizeof(levelAfter) );
              if (bidiLevel & 1) // c8: caret to the right of the rightmost character
                theFrameOffset = (levelAfter & 1) ? start : end;
              else               // c7: caret to the left of the leftmost character
                theFrameOffset = (levelAfter & 1) ? end : start;
            }
          }
          else if (bidiLevel < levelBefore && bidiLevel > levelAfter  // rule c11/12
                   && !((levelBefore ^ levelAfter) & 1)               //  before and after have the same parity
                   && ((bidiLevel ^ levelAfter) & 1))                 // cursor has different parity
          {
            if (NS_SUCCEEDED(frameSelection->GetFrameFromLevel(presContext, frameBefore, eDirPrevious, bidiLevel, &theFrame)))
            {
              theFrame->GetOffsets(start, end);
              theFrame->GetBidiProperty(presContext, nsLayoutAtoms::embeddingLevel,
                                        (void**)&levelBefore, sizeof(levelBefore) );
              if (bidiLevel & 1) // c12: caret to the left of the leftmost character
                theFrameOffset = (levelBefore & 1) ? end : start;
              else               // c11: caret to the right of the rightmost character
                theFrameOffset = (levelBefore & 1) ? start : end;
            }
          }   
        }
      }
    }
  }
#endif // IBMBIDI

  // now we have a frame, check whether it's appropriate to show the caret here
  const nsStyleUserInterface* userinterface;
  theFrame->GetStyleData(eStyleStruct_UserInterface, (const nsStyleStruct*&)userinterface);
  if (userinterface)
  {
    if (
#ifdef SUPPORT_USER_MODIFY
          // editable content still defaults to NS_STYLE_USER_MODIFY_READ_ONLY at present. See bug 15284
        (userinterface->mUserModify == NS_STYLE_USER_MODIFY_READ_ONLY) ||
#endif          
        (userinterface->mUserInput == NS_STYLE_USER_INPUT_NONE) ||
        (userinterface->mUserInput == NS_STYLE_USER_INPUT_DISABLED))
    {
      return PR_FALSE;
    }  
  }

  // mark the frame, so we get notified on deletion.
  // frames are never unmarked, which means that we'll touch every frame we visit.
  // this is not ideal.
  nsFrameState frameState;
  theFrame->GetFrameState(&frameState);
  frameState |= NS_FRAME_EXTERNAL_REFERENCE;
  theFrame->SetFrameState(frameState);

  mLastCaretFrame = theFrame;
  mLastContentOffset = theFrameOffset;
  return PR_TRUE;
}


//-----------------------------------------------------------------------------
void nsCaret::GetViewForRendering(nsIFrame *caretFrame, EViewCoordinates coordType, nsPoint &viewOffset, nsRect& outClipRect, nsIView **outRenderingView, nsIView **outRelativeView)
{

  if (!caretFrame || !outRenderingView)
    return;

    //#59405, on windows and unix, the coordinate for IME need to be view (nearest native window) related.
  if (coordType == eIMECoordinates)
#if defined(XP_MAC) || defined(XP_MACOSX)
   coordType = eTopLevelWindowCoordinates; 
#else
   coordType = eRenderingViewCoordinates; 
#endif

  *outRenderingView = nsnull;
  if (outRelativeView)
    *outRelativeView = nsnull;
  
  NS_ASSERTION(caretFrame, "Should have frame here");
  nsCOMPtr<nsIPresShell> presShell = do_QueryReferent(mPresShell);
  if (!presShell)
    return;
  
  nsCOMPtr<nsIPresContext>  presContext;
  presShell->GetPresContext(getter_AddRefs(presContext));

  viewOffset.x = 0;
  viewOffset.y = 0;
  
  nsPoint   withinViewOffset(0, 0);
  // get the offset of this frame from its parent view (walks up frame hierarchy)
  nsIView* theView = nsnull;
  caretFrame->GetOffsetFromView(presContext, withinViewOffset, &theView);
  if (theView == nsnull) return;

  if (outRelativeView && coordType == eClosestViewCoordinates)
    *outRelativeView = theView;

  nsIView*    returnView = nsnull;    // views are not refcounted
  
  nscoord   x, y;
  
  // coorinates relative to the view we are going to use for drawing
  if (coordType == eRenderingViewCoordinates)
  {
    nsIScrollableView*  scrollableView = nsnull;
  
    nsPoint             drawViewOffset(0, 0);         // offset to the view we are using to draw
    
    // walk up to the first view with a widget
    do {
      theView->GetPosition(&x, &y);

      //is this a scrollable view?
      if (!scrollableView)
        theView->QueryInterface(NS_GET_IID(nsIScrollableView), (void **)&scrollableView);

      PRBool hasWidget;
      theView->HasWidget(&hasWidget);
      if (hasWidget)
      {
        returnView = theView;
        break;
      }
      drawViewOffset.x += x;
      drawViewOffset.y += y;
      
      theView->GetParent(theView);
    } while (theView);
    
    viewOffset = withinViewOffset;
    viewOffset += drawViewOffset;
    
    if (scrollableView)
    {
      const nsIView*      clipView = nsnull;
      scrollableView->GetClipView(&clipView);
      if (!clipView) return;      // should always have one
      
      nsRect  bounds;
      clipView->GetBounds(bounds);
      scrollableView->GetScrollPosition(bounds.x, bounds.y);
      
      bounds += drawViewOffset;   // offset to coords of returned view
      outClipRect = bounds;
    }
    else
    {
      returnView->GetBounds(outClipRect);
    }

    if (outRelativeView)
      *outRelativeView = returnView;
  }
  else
  {
    // window-relative coordinates (walk right to the top of the view hierarchy)
    // we don't do anything with clipping here
    viewOffset = withinViewOffset;

    do {
      theView->GetPosition(&x, &y);

      if (!returnView)
      {
        PRBool hasWidget;
        theView->HasWidget(&hasWidget);
        
        if (hasWidget)
          returnView = theView;
      }
      // is this right?
      viewOffset.x += x;
      viewOffset.y += y;
      
      if (outRelativeView && coordType == eTopLevelWindowCoordinates)
        *outRelativeView = theView;

      theView->GetParent(theView);
    } while (theView);
  
  }
  
  
  *outRenderingView = returnView;
}


/*-----------------------------------------------------------------------------

  MustDrawCaret
  
  FInd out if we need to do any caret drawing. This returns true if
  either a) or b)
  a) caret has been drawn, and we need to erase it.
  b) caret is not drawn, and selection is collapsed
  
----------------------------------------------------------------------------- */
PRBool nsCaret::MustDrawCaret()
{
  nsCOMPtr<nsIPresShell> presShell = do_QueryReferent(mPresShell);
  if (presShell) {
    PRBool isPaintingSuppressed;
    presShell->IsPaintingSuppressed(&isPaintingSuppressed);
    if (isPaintingSuppressed)
      return PR_FALSE;
  }

	if (mDrawn)
		return PR_TRUE;
		
	nsCOMPtr<nsISelection> domSelection = do_QueryReferent(mDomSelectionWeak);
  if (!domSelection)
    return PR_FALSE;
  PRBool isCollapsed;

  if (NS_FAILED(domSelection->GetIsCollapsed(&isCollapsed)))
    return PR_FALSE;

  if (mShowDuringSelection)
    return PR_TRUE;      // show the caret even in selections

  return isCollapsed;
}


/*-----------------------------------------------------------------------------

  DrawCaret
    
----------------------------------------------------------------------------- */

void nsCaret::DrawCaret()
{
  // do we need to draw the caret at all?
  if (!MustDrawCaret())
    return;
  
  // if we are drawing, not erasing, then set up the frame etc.
  if (!mDrawn)
  {
    if (!SetupDrawingFrameAndOffset())
      return;
  }
  
  NS_ASSERTION(mLastCaretFrame != nsnull, "Should have a frame here");
  
  nsRect    frameRect;
  mLastCaretFrame->GetRect(frameRect);
  
  frameRect.x = 0;      // the origin is accounted for in GetViewForRendering()
  frameRect.y = 0;
  
  nsPoint   viewOffset(0, 0);
  nsRect    clipRect;
  nsIView   *drawingView;
  GetViewForRendering(mLastCaretFrame, eRenderingViewCoordinates, viewOffset, clipRect, &drawingView, nsnull);
  
  if (drawingView == nsnull)
    return;
  
  frameRect += viewOffset;

  nsCOMPtr<nsIPresShell> presShell = do_QueryReferent(mPresShell);
  if (!presShell) return;
  
  nsCOMPtr<nsIPresContext> presContext;
  if (NS_FAILED(presShell->GetPresContext(getter_AddRefs(presContext))))
    return;

  // if the view changed, or we don't have a rendering context, make one
  // because of drawing issues, always make a new RC at the momemt. See bug 28068
  if (
#ifdef DONT_REUSE_RENDERING_CONTEXT
      PR_TRUE ||
#endif
      (mLastCaretView != drawingView) || !mRendContext)
  {
    mRendContext = nsnull;    // free existing one if we have one
    
    nsCOMPtr<nsIDeviceContext>    dx;
    if (NS_FAILED(presContext->GetDeviceContext(getter_AddRefs(dx))) || !dx)
      return;
      
    if (NS_FAILED(dx->CreateRenderingContext(drawingView, *getter_AddRefs(mRendContext))) || !mRendContext)
      return;      
  }

  // push a known good state
  mRendContext->PushState();

  // if we got a zero-height frame, it's probably a BR frame at the end of a non-empty line
  // (see BRFrame::Reflow). In that case, figure out a height. We have to do this
  // after we've got an RC.
  if (frameRect.height == 0)
  {
      const nsStyleFont* fontStyle;
      const nsStyleVisibility* vis;
      mLastCaretFrame->GetStyleData(eStyleStruct_Font, (const nsStyleStruct*&)fontStyle);
      mLastCaretFrame->GetStyleData(eStyleStruct_Visibility, (const nsStyleStruct*&)vis);
      nsCOMPtr<nsIAtom> langGroup;
      if (vis && vis->mLanguage)
        vis->mLanguage->GetLanguageGroup(getter_AddRefs(langGroup));
      mRendContext->SetFont(fontStyle->mFont, langGroup);

      nsCOMPtr<nsIFontMetrics> fm;
      mRendContext->GetFontMetrics(*getter_AddRefs(fm));
      if (fm)
      {
        nscoord ascent, descent;
        fm->GetMaxAscent(ascent);
        fm->GetMaxDescent(descent);
        frameRect.height = ascent + descent;
        frameRect.y -= ascent; // BR frames sit on the baseline of the text, so we need to subtract
                               // the ascent to account for the frame height.
      }
  }
  
  // views are not refcounted
  mLastCaretView = drawingView;

  if (!mDrawn)
  {
    nsPoint   framePos(0, 0);
    nsRect    caretRect = frameRect;
    
    mLastCaretFrame->GetPointFromOffset(presContext, mRendContext, mLastContentOffset, &framePos);
    caretRect += framePos;
    
    //printf("Content offset %ld, frame offset %ld\n", focusOffset, framePos.x);
    if(mCaretTwipsWidth < 0)
    {// need to re-compute the pixel width
      float tDevUnitsToTwips = 15;
      nsCOMPtr<nsIDeviceContext> dx;
      presContext->GetDeviceContext(getter_AddRefs(dx));
      if (dx)
        dx->GetDevUnitsToTwips(tDevUnitsToTwips);
      mCaretTwipsWidth  = (nscoord)(tDevUnitsToTwips * (float)mCaretPixelsWidth);
    }
    caretRect.width = mCaretTwipsWidth;

    // Avoid view redraw problems by making sure the
    // caret doesn't hang outside the right edge of
    // the frame. This ensures that the caret gets
    // erased properly if the frame's right edge gets
    // invalidated.

    nscoord cX = caretRect.x + caretRect.width;
    nscoord fX = frameRect.x + frameRect.width;

    if (caretRect.x <= fX && cX > fX)
    {
      caretRect.x -= cX - fX;

      if (caretRect.x < frameRect.x)
        caretRect.x = frameRect.x;
    }

    mCaretRect.IntersectRect(clipRect, caretRect);
#ifdef IBMBIDI
    // Simon -- make a hook to draw to the left or right of the caret to show keyboard language direction
    PRBool bidiEnabled;
    nsRect hookRect;
    PRBool bidiLevel=PR_FALSE;
    if (mBidiKeyboard)
      mBidiKeyboard->IsLangRTL(&bidiLevel);
    if (bidiLevel)
    {
      bidiEnabled = PR_TRUE;
      presContext->SetBidiEnabled(bidiEnabled);
    }
    else
      presContext->GetBidiEnabled(&bidiEnabled);
    if (bidiEnabled)
    {
      if (bidiLevel != mKeyboardRTL)
      {
        /* if the caret bidi level and the keyboard language direction are not in
         * synch, the keyboard language must have been changed by the
         * user, and if the caret is in a boundary condition (between left-to-right and
         * right-to-left characters) it may have to change position to
         * reflect the location in which the next character typed will
         * appear. We will call |SelectionLanguageChange| and exit
         * without drawing the caret in the old position.
         */ 
        mKeyboardRTL = bidiLevel;
        nsCOMPtr<nsISelection> domSelection = do_QueryReferent(mDomSelectionWeak);
        if (domSelection)
        {
          if (NS_SUCCEEDED(domSelection->SelectionLanguageChange(mKeyboardRTL)))
          {
            PRBool emptyClip;
            mRendContext->PopState(emptyClip);
            return;
          }
        }
      }
      // If keyboard language is RTL, draw the hook on the left; if LTR, to the right
      hookRect.SetRect(caretRect.x + caretRect.width * ((bidiLevel) ? -1 : 1), 
                       caretRect.y + caretRect.width,
                       caretRect.width,
                       caretRect.width);
      mHookRect.IntersectRect(clipRect, hookRect);
    }
#endif //IBMBIDI
  }
  
  if (mReadOnly)
    mRendContext->SetColor(NS_RGB(85, 85, 85));   // we are drawing it; gray
  else
    mRendContext->SetColor(NS_RGB(255,255,255));
  mRendContext->InvertRect(mCaretRect);

  // Ensure the buffer is flushed (Cocoa needs this), since we're drawing
  // outside the normal painting process.
  mRendContext->FlushRect(mCaretRect);

#ifdef IBMBIDI
  if (!mHookRect.IsEmpty()) // if Bidi support is disabled, the rectangle remains empty and won't be drawn
    mRendContext->InvertRect(mHookRect);
#endif

  PRBool emptyClip;   // I know what you're thinking. "Did he fire six shots or only five?"
  mRendContext->PopState(emptyClip);
  
  ToggleDrawnStatus();

#ifdef DONT_REUSE_RENDERING_CONTEXT
  mRendContext = nsnull;
#endif
}

#ifdef XP_MAC
#pragma mark -
#endif

//-----------------------------------------------------------------------------
/* static */
void nsCaret::CaretBlinkCallback(nsITimer *aTimer, void *aClosure)
{
  nsCaret   *theCaret = NS_REINTERPRET_CAST(nsCaret*, aClosure);
  if (!theCaret) return;
  
  theCaret->DrawCaret();

#ifndef REPEATING_TIMERS
  theCaret->PrimeTimer();
#endif
}


//-----------------------------------------------------------------------------
nsresult NS_NewCaret(nsICaret** aInstancePtrResult)
{
  NS_PRECONDITION(aInstancePtrResult, "null ptr");
  
  nsCaret* caret = new nsCaret();
  if (nsnull == caret)
      return NS_ERROR_OUT_OF_MEMORY;
      
  return caret->QueryInterface(NS_GET_IID(nsICaret), (void**) aInstancePtrResult);
}

NS_IMETHODIMP nsCaret::SetCaretWidth(nscoord aPixels)
{
  if(!aPixels)
    return NS_ERROR_FAILURE;
  else
  { //no need to optimize this, but if it gets too slow, we can check for case aPixels==mCaretPixelsWidth
    mCaretPixelsWidth = aPixels;
    mCaretTwipsWidth = -1;
  }
  return NS_OK;
}

NS_IMETHODIMP nsCaret::SetVisibilityDuringSelection(PRBool aVisibility) 
{
  mShowDuringSelection = aVisibility;
  return NS_OK;
}
