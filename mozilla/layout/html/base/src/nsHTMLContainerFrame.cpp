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
 * Contributor(s): 
 */
#include "nsHTMLContainerFrame.h"
#include "nsIRenderingContext.h"
#include "nsIPresContext.h"
#include "nsIPresShell.h"
#include "nsIStyleContext.h"
#include "nsStyleConsts.h"
#include "nsCSSRendering.h"
#include "nsIContent.h"
#include "nsLayoutAtoms.h"
#include "nsIWidget.h"
#include "nsILinkHandler.h"
#include "nsHTMLValue.h"
#include "nsGUIEvent.h"
#include "nsIDocument.h"
#include "nsIURL.h"
#include "nsIPtr.h"
#include "nsPlaceholderFrame.h"
#include "nsIHTMLContent.h"
#include "nsHTMLParts.h"
#include "nsIView.h"
#include "nsIViewManager.h"
#include "nsViewsCID.h"
#include "nsIReflowCommand.h"
#include "nsHTMLIIDs.h"
#include "nsDOMEvent.h"
#include "nsIScrollableView.h"
#include "nsWidgetsCID.h"
#include "nsIStyleSet.h"
#include "nsCOMPtr.h"

static NS_DEFINE_IID(kScrollViewIID, NS_ISCROLLABLEVIEW_IID);
static NS_DEFINE_IID(kCChildCID, NS_CHILD_CID);

NS_IMETHODIMP
nsHTMLContainerFrame::Paint(nsIPresContext* aPresContext,
                            nsIRenderingContext& aRenderingContext,
                            const nsRect& aDirtyRect,
                            nsFramePaintLayer aWhichLayer)
{
  if (NS_FRAME_IS_UNFLOWABLE & mState) {
    return NS_OK;
  }
  if (NS_FRAME_PAINT_LAYER_BACKGROUND == aWhichLayer) {
    const nsStyleDisplay* disp = (const nsStyleDisplay*)
      mStyleContext->GetStyleData(eStyleStruct_Display);
    if (disp->IsVisible() && mRect.width && mRect.height) {
      // Paint our background and border
      PRIntn skipSides = GetSkipSides();
      const nsStyleColor* color = (const nsStyleColor*)
        mStyleContext->GetStyleData(eStyleStruct_Color);
      const nsStyleSpacing* spacing = (const nsStyleSpacing*)
        mStyleContext->GetStyleData(eStyleStruct_Spacing);

      nsRect  rect(0, 0, mRect.width, mRect.height);
      nsCSSRendering::PaintBackground(aPresContext, aRenderingContext, this,
                                      aDirtyRect, rect, *color, *spacing, 0, 0);
      nsCSSRendering::PaintBorder(aPresContext, aRenderingContext, this,
                                  aDirtyRect, rect, *spacing, mStyleContext, skipSides);
      nsCSSRendering::PaintOutline(aPresContext, aRenderingContext, this,
                                  aDirtyRect, rect, *spacing, mStyleContext, 0);
      
      // The sole purpose of this is to trigger display
      //  of the selection window for Named Anchors,
      //  which don't have any children and normally don't
      //  have any size, but in Editor we use CSS to display
      //  an image to represent this "hidden" element.
      if (!mFrames.FirstChild())
      {
        nsFrame::Paint(aPresContext,
                       aRenderingContext, aDirtyRect, aWhichLayer);
      }
    }
  }

  // Now paint the kids. Note that child elements have the opportunity to
  // override the visibility property and display even if their parent is
  // hidden

  PaintChildren(aPresContext, aRenderingContext, aDirtyRect, aWhichLayer);

  // see if we have to draw a selection frame around this container
  return nsFrame::Paint(aPresContext, aRenderingContext, aDirtyRect, aWhichLayer);
}

/**
 * Create a next-in-flow for aFrame. Will return the newly created
 * frame in aNextInFlowResult <b>if and only if</b> a new frame is
 * created; otherwise nsnull is returned in aNextInFlowResult.
 */
nsresult
nsHTMLContainerFrame::CreateNextInFlow(nsIPresContext* aPresContext,
                                       nsIFrame* aOuterFrame,
                                       nsIFrame* aFrame,
                                       nsIFrame*& aNextInFlowResult)
{
  aNextInFlowResult = nsnull;

  nsIFrame* nextInFlow;
  aFrame->GetNextInFlow(&nextInFlow);
  if (nsnull == nextInFlow) {
    // Create a continuation frame for the child frame and insert it
    // into our lines child list.
    nsIFrame* nextFrame;
    aFrame->GetNextSibling(&nextFrame);

    nsIPresShell* presShell;
    nsIStyleSet*  styleSet;
    aPresContext->GetShell(&presShell);
    presShell->GetStyleSet(&styleSet);
    NS_RELEASE(presShell);
    styleSet->CreateContinuingFrame(aPresContext, aFrame, aOuterFrame, &nextInFlow);
    NS_RELEASE(styleSet);

    if (nsnull == nextInFlow) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    aFrame->SetNextSibling(nextInFlow);
    nextInFlow->SetNextSibling(nextFrame);

    NS_FRAME_LOG(NS_FRAME_TRACE_NEW_FRAMES,
       ("nsHTMLContainerFrame::MaybeCreateNextInFlow: frame=%p nextInFlow=%p",
        aFrame, nextInFlow));

    aNextInFlowResult = nextInFlow;
  }
  return NS_OK;
}

static nsresult
ReparentFrameViewTo(nsIPresContext* aPresContext,
                    nsIFrame*       aFrame,
                    nsIViewManager* aViewManager,
                    nsIView*        aNewParentView,
                    nsIView*        aOldParentView)
{
  nsIView*  view;

  // Does aFrame have a view?
  aFrame->GetView(aPresContext, &view);
  if (view) {
    // Verify that the current parent view is what we think it is
    nsIView*  parentView;

    // you have to check all the time to see if this 
    // view has been reparented.. if you try to insert for
    // a view that has been reparented.. bad things can happen.
    // I took out the debug check and check all the time now..
    // this case happens when you print.
    view->GetParent(parentView);
    if(parentView != aOldParentView)
      return NS_OK;

    //NS_ASSERTION(parentView == aOldParentView, "unexpected parent view");

    // Change the parent view.
    PRInt32 zIndex;
    view->GetZIndex(zIndex);
    aViewManager->RemoveChild(aOldParentView, view);
    
    // XXX We need to insert this view in the correct place within its z-order...
    // XXX What should we do about the Z-placeholder-child if this frame is position:fixed?
    aViewManager->InsertChild(aNewParentView, view, zIndex);

  } else {
    // Iterate the child frames, and check each child frame to see if it has
    // a view
    nsIFrame* childFrame;
    aFrame->FirstChild(aPresContext, nsnull, &childFrame);
    while (childFrame) {
      ReparentFrameViewTo(aPresContext, childFrame, aViewManager, aNewParentView, aOldParentView);
      childFrame->GetNextSibling(&childFrame);
    }

    // Also check the overflow-list
    aFrame->FirstChild(aPresContext, nsLayoutAtoms::overflowList, &childFrame);
    while (childFrame) {
      ReparentFrameViewTo(aPresContext, childFrame, aViewManager, aNewParentView, aOldParentView);
      childFrame->GetNextSibling(&childFrame);
    }
  }

  return NS_OK;
}

// Helper function that returns the nearest view to this frame. Checks
// this frame, its parent frame, its parent frame, ...
static nsIView*
GetClosestViewFor(nsIPresContext* aPresContext, nsIFrame* aFrame)
{
  NS_PRECONDITION(aFrame, "null frame pointer");
  nsIView*  view;

  do {
    aFrame->GetView(aPresContext, &view);
    if (view) {
      break;
    }
    aFrame->GetParent(&aFrame);
  } while (aFrame);

  NS_POSTCONDITION(view, "no containing view");
  return view;
}

nsresult
nsHTMLContainerFrame::ReparentFrameView(nsIPresContext* aPresContext,
                                        nsIFrame*       aChildFrame,
                                        nsIFrame*       aOldParentFrame,
                                        nsIFrame*       aNewParentFrame)
{
  NS_PRECONDITION(aChildFrame, "null child frame pointer");
  NS_PRECONDITION(aOldParentFrame, "null old parent frame pointer");
  NS_PRECONDITION(aNewParentFrame, "null new parent frame pointer");
  NS_PRECONDITION(aOldParentFrame != aNewParentFrame, "same old and new parent frame");

  nsIView*  childView;
  nsIView*  oldParentView;
  nsIView*  newParentView;
  
  // This code is called often and we need it to be as fast as possible, so
  // see if we can trivially detect that no work needs to be done
  aChildFrame->GetView(aPresContext, &childView);
  if (!childView) {
    // Child frame doesn't have a view. See if it has any child frames
    nsIFrame* firstChild;
    aChildFrame->FirstChild(aPresContext, nsnull, &firstChild);
    if (!firstChild) {
      return NS_OK;
    }
  }

  // See if either the old parent frame or the new parent frame have a view
  aOldParentFrame->GetView(aPresContext, &oldParentView);
  aNewParentFrame->GetView(aPresContext, &newParentView);

  if (!oldParentView && !newParentView) {
    // Walk up both the old parent frame and the new parent frame nodes
    // stopping when we either find a common parent or views for one
    // or both of the frames.
    //
    // This works well in the common case where we push/pull and the old parent
    // frame and the new parent frame are part of the same flow. They will
    // typically be the same distance (height wise) from the
    do {
      aOldParentFrame->GetParent(&aOldParentFrame);
      aNewParentFrame->GetParent(&aNewParentFrame);
      
      // We should never walk all the way to the root frame without finding
      // a view
      NS_ASSERTION(aOldParentFrame && aNewParentFrame, "didn't find view");

      // See if we reached a common parent
      if (aOldParentFrame == aNewParentFrame) {
        break;
      }

      // Get the views
      aOldParentFrame->GetView(aPresContext, &oldParentView);
      aNewParentFrame->GetView(aPresContext, &newParentView);
    } while (!(oldParentView || newParentView));
  }


  // See if we found a common parent frame
  if (aOldParentFrame == aNewParentFrame) {
    // We found a common parent and there are no views between the old parent
    // and the common parent or the new parent frame and the common parent.
    // Because neither the old parent frame nor the new parent frame have views,
    // then any child views don't need reparenting
    return NS_OK;
  }

  // We found views for one or both of the parent frames before we found a
  // common parent
  NS_ASSERTION(oldParentView || newParentView, "internal error");
  if (!oldParentView) {
    oldParentView = GetClosestViewFor(aPresContext, aOldParentFrame);
  }
  if (!newParentView) {
    newParentView = GetClosestViewFor(aPresContext, aNewParentFrame);
  }
  
  // See if the old parent frame and the new parent frame are in the
  // same view sub-hierarchy. If they are then we don't have to do
  // anything
  if (oldParentView != newParentView) {
    nsCOMPtr<nsIViewManager>  viewManager;
    oldParentView->GetViewManager(*getter_AddRefs(viewManager));

    // They're not so we need to reparent any child views
    return ReparentFrameViewTo(aPresContext, aChildFrame, viewManager, newParentView,
                               oldParentView);
  }

  return NS_OK;
}

nsresult
nsHTMLContainerFrame::ReparentFrameViewList(nsIPresContext* aPresContext,
                                            nsIFrame*       aChildFrameList,
                                            nsIFrame*       aOldParentFrame,
                                            nsIFrame*       aNewParentFrame)
{
  NS_PRECONDITION(aChildFrameList, "null child frame list");
  NS_PRECONDITION(aOldParentFrame, "null old parent frame pointer");
  NS_PRECONDITION(aNewParentFrame, "null new parent frame pointer");
  NS_PRECONDITION(aOldParentFrame != aNewParentFrame, "same old and new parent frame");

  nsIView*  oldParentView;
  nsIView*  newParentView;
  
  // See if either the old parent frame or the new parent frame have a view
  aOldParentFrame->GetView(aPresContext, &oldParentView);
  aNewParentFrame->GetView(aPresContext, &newParentView);

  if (!oldParentView && !newParentView) {
    // Walk up both the old parent frame and the new parent frame nodes
    // stopping when we either find a common parent or views for one
    // or both of the frames.
    //
    // This works well in the common case where we push/pull and the old parent
    // frame and the new parent frame are part of the same flow. They will
    // typically be the same distance (height wise) from the
    do {
      aOldParentFrame->GetParent(&aOldParentFrame);
      aNewParentFrame->GetParent(&aNewParentFrame);
      
      // We should never walk all the way to the root frame without finding
      // a view
      NS_ASSERTION(aOldParentFrame && aNewParentFrame, "didn't find view");

      // See if we reached a common parent
      if (aOldParentFrame == aNewParentFrame) {
        break;
      }

      // Get the views
      aOldParentFrame->GetView(aPresContext, &oldParentView);
      aNewParentFrame->GetView(aPresContext, &newParentView);
    } while (!(oldParentView || newParentView));
  }


  // See if we found a common parent frame
  if (aOldParentFrame == aNewParentFrame) {
    // We found a common parent and there are no views between the old parent
    // and the common parent or the new parent frame and the common parent.
    // Because neither the old parent frame nor the new parent frame have views,
    // then any child views don't need reparenting
    return NS_OK;
  }

  // We found views for one or both of the parent frames before we found a
  // common parent
  NS_ASSERTION(oldParentView || newParentView, "internal error");
  if (!oldParentView) {
    oldParentView = GetClosestViewFor(aPresContext, aOldParentFrame);
  }
  if (!newParentView) {
    newParentView = GetClosestViewFor(aPresContext, aNewParentFrame);
  }
  
  // See if the old parent frame and the new parent frame are in the
  // same view sub-hierarchy. If they are then we don't have to do
  // anything
  if (oldParentView != newParentView) {
    nsCOMPtr<nsIViewManager>  viewManager;
    oldParentView->GetViewManager(*getter_AddRefs(viewManager));

    // They're not so we need to reparent any child views
    for (nsIFrame* f = aChildFrameList; f; f->GetNextSibling(&f)) {
      ReparentFrameViewTo(aPresContext, f, viewManager, newParentView,
                          oldParentView);
    }
  }

  return NS_OK;
}

static PRBool
IsContainerContent(nsIFrame* aFrame)
{
  nsIContent* content;
  PRBool      result = PR_FALSE;

  aFrame->GetContent(&content);
  if (content) {
    content->CanContainChildren(result);
    NS_RELEASE(content);
  }

  return result;
}

nsresult
nsHTMLContainerFrame::CreateViewForFrame(nsIPresContext* aPresContext,
                                         nsIFrame* aFrame,
                                         nsIStyleContext* aStyleContext,
                                         nsIFrame* aContentParentFrame,
                                         PRBool aForce)
{
  nsIView* view;
  aFrame->GetView(aPresContext, &view);
  // If we don't yet have a view, see if we need a view
  if (nsnull == view) {
    PRBool  fixedBackgroundAttachment = PR_FALSE;

    // Get nsStyleColor and nsStyleDisplay
    const nsStyleColor* color = (const nsStyleColor*)
      aStyleContext->GetStyleData(eStyleStruct_Color);
    const nsStyleDisplay* display = (const nsStyleDisplay*)
      aStyleContext->GetStyleData(eStyleStruct_Display);
    const nsStylePosition* position = (const nsStylePosition*)
      aStyleContext->GetStyleData(eStyleStruct_Position);
    
    if (color->mOpacity != 1.0f) {
      NS_FRAME_LOG(NS_FRAME_TRACE_CALLS,
        ("nsHTMLContainerFrame::CreateViewForFrame: frame=%p opacity=%g",
         aFrame, color->mOpacity));
      aForce = PR_TRUE;
    }

    // See if the frame has a fixed background attachment
    if (NS_STYLE_BG_ATTACHMENT_FIXED == color->mBackgroundAttachment) {
      aForce = PR_TRUE;
      fixedBackgroundAttachment = PR_TRUE;
    }
    
    // See if the frame is being relatively positioned or absolutely
    // positioned
    if (!aForce) {
      if (NS_STYLE_POSITION_RELATIVE == position->mPosition) {
        NS_FRAME_LOG(NS_FRAME_TRACE_CALLS,
          ("nsHTMLContainerFrame::CreateViewForFrame: frame=%p relatively positioned",
          aFrame));
        aForce = PR_TRUE;
      } else if (position->IsAbsolutelyPositioned()) {
        NS_FRAME_LOG(NS_FRAME_TRACE_CALLS,
          ("nsHTMLContainerFrame::CreateViewForFrame: frame=%p absolutely positioned",
          aFrame));
        aForce = PR_TRUE;
      }
    }

    // See if the frame is a scrolled frame
    if (!aForce) {
      nsIAtom*  pseudoTag;
      aStyleContext->GetPseudoType(pseudoTag);
      if (pseudoTag == nsLayoutAtoms::scrolledContentPseudo) {
        NS_FRAME_LOG(NS_FRAME_TRACE_CALLS,
          ("nsHTMLContainerFrame::CreateViewForFrame: scrolled frame=%p", aFrame));
        aForce = PR_TRUE;
      }
      NS_IF_RELEASE(pseudoTag);
    }

    // See if the frame is block-level and has 'overflow' set to 'hidden'. If
    // so and it can have child frames, then we need to give it a view so clipping
    // of any child views works correctly. Note that if it's floated it is also
    // block-level, but we can't trust that the style context 'display' value is
    // set correctly
    if (!aForce) {
      if ((display->IsBlockLevel() || display->IsFloating()) &&
          (display->mOverflow == NS_STYLE_OVERFLOW_HIDDEN)) {

        // The reason for the check of whether it can contain children is just
        // to avoid giving it a view unnecessarily
        if (::IsContainerContent(aFrame)) {
          // XXX Check for the frame being a block frame and only force a view
          // in that case, because adding a view for box frames seems to cause
          // problems for XUL...
          nsIAtom*  frameType;

          aFrame->GetFrameType(&frameType);
          if ((frameType == nsLayoutAtoms::blockFrame) ||
              (frameType == nsLayoutAtoms::areaFrame)) {
            aForce = PR_TRUE;
          }
          NS_IF_RELEASE(frameType);
        }
      }
    }

    if (aForce) {
      // Create a view
      nsIFrame* parent = nsnull;
      nsIView* parentView = nsnull;

      aFrame->GetParentWithView(aPresContext, &parent);
      NS_ASSERTION(parent, "GetParentWithView failed");
      parent->GetView(aPresContext, &parentView);
      NS_ASSERTION(parentView, "no parent with view");

      // Create a view
      static NS_DEFINE_IID(kViewCID, NS_VIEW_CID);
      static NS_DEFINE_IID(kIViewIID, NS_IVIEW_IID);

      nsresult result = nsComponentManager::CreateInstance(kViewCID, 
                                                     nsnull, 
                                                     kIViewIID, 
                                                     (void **)&view);
      if (NS_OK == result) {
        nsIViewManager* viewManager;
        parentView->GetViewManager(viewManager);
        NS_ASSERTION(nsnull != viewManager, "null view manager");

        // Initialize the view
        nsRect bounds;
        aFrame->GetRect(bounds);
        view->Init(viewManager, bounds, parentView);

        // If the frame has a fixed background attachment, then indicate that the
        // view's contents should be repainted and not bitblt'd
        if (fixedBackgroundAttachment) {
          PRUint32  viewFlags;
          view->GetViewFlags(&viewFlags);
          view->SetViewFlags(viewFlags | NS_VIEW_PUBLIC_FLAG_DONT_BITBLT);
        }
        
        // Insert the view into the view hierarchy. If the parent view is a
        // scrolling view we need to do this differently
        nsIScrollableView*  scrollingView;
        if (NS_SUCCEEDED(parentView->QueryInterface(kScrollViewIID, (void**)&scrollingView))) {
          scrollingView->SetScrolledView(view);
        } else {
          PRInt32 zIndex = 0;
          PRBool  autoZIndex = PR_FALSE;

          // Get the z-index to use
          if (position->mZIndex.GetUnit() == eStyleUnit_Integer) {
            zIndex = position->mZIndex.GetIntValue();
          } else if (position->mZIndex.GetUnit() == eStyleUnit_Auto) {
            autoZIndex = PR_TRUE;
          }

          viewManager->InsertChild(parentView, view, zIndex);

          if (autoZIndex) {
            viewManager->SetViewAutoZIndex(view, PR_TRUE);
          }

          if (nsnull != aContentParentFrame) {
            // If, for some reason, GetView below fails to initialize zParentView,
            // then ensure that we completely bypass InsertZPlaceholder below.
            // The effect will be as if we never knew about aContentParentFrame
            // in the first place, so at least this code won't be doing any damage.
            nsIView* zParentView = parentView;
            
            aContentParentFrame->GetView(aPresContext, &zParentView);
            
            if (nsnull == zParentView) {
              nsIFrame* zParentFrame = nsnull;
              
              aContentParentFrame->GetParentWithView(aPresContext, &zParentFrame);
              NS_ASSERTION(zParentFrame, "GetParentWithView failed");
              zParentFrame->GetView(aPresContext, &zParentView);
              NS_ASSERTION(zParentView, "no parent with view");
            }
            
            if (zParentView != parentView) {
              viewManager->InsertZPlaceholder(zParentView, view, zIndex);
            }
          }
        }

        // See if the view should be hidden
        PRBool  viewIsVisible = PR_TRUE;
        PRBool  viewHasTransparentContent = (color->mBackgroundFlags &
                  NS_STYLE_BG_COLOR_TRANSPARENT) == NS_STYLE_BG_COLOR_TRANSPARENT;

        if (NS_STYLE_VISIBILITY_COLLAPSE == display->mVisible) {
          viewIsVisible = PR_FALSE;
        }
        else if (NS_STYLE_VISIBILITY_HIDDEN == display->mVisible) {
          // If it has a widget, hide the view because the widget can't deal with it
          nsIWidget* widget = nsnull;
          view->GetWidget(widget);
          if (widget) {
            viewIsVisible = PR_FALSE;
            NS_RELEASE(widget);
          }
          else {
            // If it's a container element, then leave the view visible, but
            // mark it as having transparent content. The reason we need to
            // do this is that child elements can override their parent's
            // hidden visibility and be visible anyway.
            //
            // Because this function is called before processing the content
            // object's child elements, we can't tell if it's a leaf by looking
            // at whether the frame has any child frames
            if (::IsContainerContent(aFrame)) {
              // The view needs to be visible, but marked as having transparent
              // content
              viewHasTransparentContent = PR_TRUE;
            } else {
              // Go ahead and hide the view
              viewIsVisible = PR_FALSE;
            }
          }
        }

        if (viewIsVisible) {
          if (viewHasTransparentContent) {
            viewManager->SetViewContentTransparency(view, PR_TRUE);
          }

        } else {
          view->SetVisibility(nsViewVisibility_kHide);
        }

        // XXX If it's fixed positioned, then create a widget so it floats
        // above the scrolling area
        if (NS_STYLE_POSITION_FIXED == position->mPosition) {
          view->CreateWidget(kCChildCID);
        }

        viewManager->SetViewOpacity(view, color->mOpacity);
        NS_RELEASE(viewManager);
      }

      // Remember our view
      aFrame->SetView(aPresContext, view);

      NS_FRAME_LOG(NS_FRAME_TRACE_CALLS,
        ("nsHTMLContainerFrame::CreateViewForFrame: frame=%p view=%p",
         aFrame));
      return result;
    }
  }
  return NS_OK;
}
