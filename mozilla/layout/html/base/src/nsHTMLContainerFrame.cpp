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
#include "nsHTMLContainerFrame.h"
#include "nsIRenderingContext.h"
#include "nsIPresContext.h"
#include "nsIStyleContext.h"
#include "nsStyleConsts.h"
#include "nsCSSRendering.h"
#include "nsIContent.h"
#include "nsHTMLAtoms.h"
#include "nsIWidget.h"
#include "nsILinkHandler.h"
#include "nsHTMLValue.h"
#include "nsGUIEvent.h"
#include "nsIDocument.h"
#include "nsIURL.h"

static NS_DEFINE_IID(kStyleMoleculeSID, NS_STYLEMOLECULE_SID);
static NS_DEFINE_IID(kStyleBorderSID, NS_STYLEBORDER_SID);
static NS_DEFINE_IID(kStyleColorSID, NS_STYLECOLOR_SID);

nsHTMLContainerFrame::nsHTMLContainerFrame(nsIContent* aContent,
                                           PRInt32 aIndexInParent,
                                           nsIFrame* aParent)
  : nsContainerFrame(aContent, aIndexInParent, aParent)
{
}

nsHTMLContainerFrame::~nsHTMLContainerFrame()
{
}

NS_METHOD nsHTMLContainerFrame::Paint(nsIPresContext& aPresContext,
                                      nsIRenderingContext& aRenderingContext,
                                      const nsRect& aDirtyRect)
{
  // Do not paint ourselves if we are a pseudo-frame
  if (PR_FALSE == IsPseudoFrame()) {
    PRIntn skipSides = GetSkipSides();
    nsStyleColor* color =
      (nsStyleColor*)mStyleContext->GetData(kStyleColorSID);
    nsStyleBorder* border =
      (nsStyleBorder*)mStyleContext->GetData(kStyleBorderSID);
    nsCSSRendering::PaintBackground(aPresContext, aRenderingContext, this,
                                    aDirtyRect, mRect, *color);
    nsCSSRendering::PaintBorder(aPresContext, aRenderingContext, this,
                                aDirtyRect, mRect, *border, skipSides);
  }

  PaintChildren(aPresContext, aRenderingContext, aDirtyRect);

  if (nsIFrame::GetShowFrameBorders()) {
    aRenderingContext.SetColor(NS_RGB(255,0,0));
    aRenderingContext.DrawRect(0, 0, mRect.width, mRect.height);
  }
  return NS_OK;
}

void nsHTMLContainerFrame::TriggerLink(nsIPresContext& aPresContext,
                                       const nsString& aBase,
                                       const nsString& aURLSpec,
                                       const nsString& aTargetSpec)
{
  nsILinkHandler* handler;
  if (NS_OK == aPresContext.GetLinkHandler(&handler)) {
    // Resolve url to an absolute url
    nsIURL* docURL = nsnull;
    nsIDocument* doc = mContent->GetDocument();
    if (nsnull != doc) {
      docURL = doc->GetDocumentURL();
      NS_RELEASE(doc);
    }

    nsAutoString absURLSpec;
    nsresult rv = NS_MakeAbsoluteURL(docURL, aBase, aURLSpec, absURLSpec);
    if (nsnull != docURL) {
      NS_RELEASE(docURL);
    }

    // Now pass on absolute url to the click handler
    handler->OnLinkClick(this, absURLSpec, aTargetSpec);
  }
}

NS_METHOD nsHTMLContainerFrame::HandleEvent(nsIPresContext& aPresContext,
                                            nsGUIEvent* aEvent,
                                            nsEventStatus& aEventStatus)
{
  aEventStatus = nsEventStatus_eIgnore; 
  switch (aEvent->message) {
  case NS_MOUSE_LEFT_BUTTON_UP:
    if (nsEventStatus_eIgnore ==
        nsContainerFrame::HandleEvent(aPresContext, aEvent, aEventStatus)) { 
      // If our child didn't take the click then since we are an
      // anchor, we take the click.
      nsIAtom* tag = mContent->GetTag();
      if (tag == nsHTMLAtoms::a) {
        nsAutoString base, href, target;
        mContent->GetAttribute("href", href);
        mContent->GetAttribute("target", target);
        TriggerLink(aPresContext, base, href, target);
        aEventStatus = nsEventStatus_eConsumeNoDefault; 
      }
      NS_IF_RELEASE(tag);
    }
    break;

  case NS_MOUSE_RIGHT_BUTTON_DOWN:
    // XXX Bring up a contextual menu provided by the application
    break;

  default:
    nsContainerFrame::HandleEvent(aPresContext, aEvent, aEventStatus);
    break;
  }
  return NS_OK;
}

NS_METHOD nsHTMLContainerFrame::GetCursorAt(nsIPresContext& aPresContext,
                                            const nsPoint& aPoint,
                                            nsIFrame** aFrame,
                                            PRInt32& aCursor)
{
  nsStyleMolecule* mol = (nsStyleMolecule*)
    mStyleContext->GetData(kStyleMoleculeSID);
  if (mol->cursor != NS_STYLE_CURSOR_INHERIT) {
    // If this container has a particular cursor, use it, otherwise
    // let the child decide.
    *aFrame = this;
    aCursor = (PRInt32)mol->cursor;
    return NS_OK;
  }
  return nsContainerFrame::GetCursorAt(aPresContext, aPoint, aFrame, aCursor);
}

#if 0
nsIFrame::ReflowStatus
nsHTMLContainerFrame::IncrementalReflow(nsIPresContext*  aPresContext,
                                        nsReflowMetrics& aDesiredSize,
                                        const nsSize&    aMaxSize,
                                        nsISpaceManager* aSpaceManager,
                                        nsReflowCommand& aReflowCommand)
{
  // 0. Get to the correct flow block for this frame that applies to
  // the effected frame. If the reflow command is not for us or it's
  // deleted or changed then "child" is the child being
  // effected. Otherwise child is the child before the effected
  // content child (or null if the effected child is our first child)
  <T>* flow = FindFlowBlock(aReflowCommand, &child);

  // 1. Recover reflow state
  <T>State state;
  RecoverState(aPresContext, ..., state);

  // 2. Put state into presentation shell cache so child frames can find
  // it.

  if (aReflowCommand.GetTarget() == this) {
    // Apply reflow command to the flow frame; one of it's immediate
    // children has changed state.
    ReflowStatus status;
    switch (aReflowCommand.GetType()) {
    case nsReflowCommand::rcContentAppended:
      status = ReflowAppended(aPresContext, aDesiredSize, aMaxSize,
                              aSpaceManager, state, flow);
      break;
    case nsReflowCommand::rcContentInserted:
      status = ReflowInserted(aPresContext, aDesiredSize, aMaxSize,
                              aSpaceManager, state, flow);
      break;
    case nsReflowCommand::rcContentDeleted:
      status = ReflowDeleted(aPresContext, aDesiredSize, aMaxSize,
                             aSpaceManager, state, flow);
      break;
    case nsReflowCommand::rcContentChanged:
      status = ReflowChanged(aPresContext, aDesiredSize, aMaxSize,
                             aSpaceManager, state, flow);
      break;
    case nsReflowCommand::rcUserDefined:
      switch (
      break;
    default:
      // Ignore all other reflow commands
      status = frComplete;
      break;
    }
  } else {
    // Reflow command applies to one of our children. We need to
    // figure out which child because it's going to change size most
    // likely and we need to be prepared to deal with it.
    nsIFrame* kid = nsnull;
    status = aReflowCommand.Next(aDesiredSize, aMaxSize, aSpaceManager, kid);

    // Execute the ReflowChanged post-processing code that deals with
    // the child frame's size change; next-in-flows, overflow-lists,
    // etc.
  }

  // 4. Remove state from presentation shell cache

  return status;
}

nsIFrame::ReflowStatus
nsHTMLContainerFrame::ReflowAppended(nsIPresContext*  aPresContext,
                                     nsReflowMetrics& aDesiredSize,
                                     const nsSize&    aMaxSize,
                                     nsISpaceManager* aSpaceManager,
                                     nsReflowCommand& aReflowCommand)
{
  // 1. compute state up to but not including new content w/o reflowing
  // everything that's already been flowed

  // 2. if this container uses pseudo-frames then 2b, else 2a

  //   2a. start a reflow-unmapped of the new children

  //   2b. reflow-mapped the last-child if it's a pseudo as it might
  //   pickup the new children; smarter containers can avoid this if
  //   they can determine that the last-child won't pickup the new
  //   children up. Once reflowing the last-child is complete then if
  //   the status is frComplete then and only then try reflowing any
  //   remaining unmapped children

  //   2c. For inline and column code the result of a reflow mapped
  //   cannot impact any previously reflowed children. For block this
  //   is not true so block needs to reconstruct the line and then
  //   proceed as if the line were being reflowed for the first time
  //   (skipping over the existing children, of course). The line still
  //   needs to be flushed out, vertically aligned, etc, which may cause
  //   it to not fit.

  // 3. we may end up pushing kids to next-in-flow or stopping before
  // all children have been mapped because we are out of room. parent
  // needs to look at our return status and create a next-in-flow for
  // us; the currently implemented reflow-unmapped code will do the
  // right thing in that a child that is being appended and reflowed
  // will get it's continuations created by us; if we run out of room
  // we return an incomplete status to our parent and it does the same
  // to us.
}

// use a custom reflow command when we push or create an overflow list; 

nsIFrame::ReflowStatus
nsHTMLContainerFrame::ReflowInserted(nsIPresContext*  aPresContext,
                                     nsReflowMetrics& aDesiredSize,
                                     const nsSize&    aMaxSize,
                                     nsISpaceManager* aSpaceManager,
                                     nsReflowCommand& aReflowCommand)
{
  // 1. compute state up to but not including new content w/o reflowing
  // everything that's already been flowed

  // 2. Content insertion implies a new child of this container; the
  // content inserted may need special attention (see
  // ContentAppended). The same rules apply. However, if the
  // pseudo-frame doesn't pullup the child then we need to
  // ResizeReflow the addition (We cannot go through the
  // reflow-unmapped path because the child frame(s) will need to be
  // inserted into the list).

  // 2a if reflow results in a push then go through push reflow
  // command path else go through deal-with-size-change
}

nsIFrame::ReflowStatus
nsHTMLContainerFrame::ReflowDeleted(nsIPresContext*  aPresContext,
                                    nsReflowMetrics& aDesiredSize,
                                    const nsSize&    aMaxSize,
                                    nsISpaceManager* aSpaceManager,
                                    nsReflowCommand& aReflowCommand)
{
  // 1. compute state up to but not including deleted content w/o reflowing
  // everything that's already been flowed

  // 2. remove all of the child frames that belong to the deleted
  // child; this includes next-in-flows. mark all of our next-in-flows
  // that are impacted by this as dirty (or generate another reflow
  // command)

  // 3. Run reflow-mapped code starting at the deleted child point;
  // return the usual status to the parent.


  // Generate reflow commands for my next-in-flows if they are
  // impacted by deleting my child's next-in-flows
}

// Meta point about ReflowChanged; we will factor out the change so
// that only stylistic changes that actually require a reflow end up
// at this frame. The style system will differentiate between
// rendering-only changes and reflow changes.

nsIFrame::ReflowStatus
nsHTMLContainerFrame::ReflowChanged(nsIPresContext*  aPresContext,
                                    nsReflowMetrics& aDesiredSize,
                                    const nsSize&    aMaxSize,
                                    nsISpaceManager* aSpaceManager,
                                    nsReflowCommand& aReflowCommand)
{
  // 1. compute state up to but not including deleted content w/o reflowing
  // everything that's already been flowed

  // 2. delete the frame that corresponds to the changed child (and
  // it's next-in-flows, etc.)

  // 3. run the ReflowInserted code on the content
}
#endif
