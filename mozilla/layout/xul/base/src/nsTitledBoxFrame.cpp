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

// YY need to pass isMultiple before create called

/*
//#include "nsFormControlFrame.h"
#include "nsHTMLContainerFrame.h"
#include "nsLegendFrame.h"
#include "nsIDOMNode.h"
#include "nsIDOMHTMLFieldSetElement.h"
#include "nsIDOMHTMLLegendElement.h"
//#include "nsIDOMHTMLCollection.h"
#include "nsIContent.h"
#include "nsIFrame.h"
#include "nsISupports.h"
#include "nsIAtom.h"
#include "nsIPresContext.h"
#include "nsIHTMLContent.h"
#include "nsHTMLIIDs.h"
#include "nsHTMLParts.h"
#include "nsHTMLAtoms.h"
#include "nsIStyleContext.h"
#include "nsStyleConsts.h"
#include "nsStyleUtil.h"
#include "nsFont.h"
#include "nsCOMPtr.h"
*/

#include "nsCSSRendering.h"
#include "nsIStyleContext.h"
#include "nsBoxFrame.h"

class nsTitledBoxFrame : public nsBoxFrame {
public:

  nsTitledBoxFrame(nsIPresShell* aShell);

  NS_IMETHOD SetInitialChildList(nsIPresContext* aPresContext,
                                 nsIAtom*        aListName,
                                 nsIFrame*       aChildList);
                               
  NS_METHOD Paint(nsIPresContext* aPresContext,
                  nsIRenderingContext& aRenderingContext,
                  const nsRect& aDirtyRect,
                  nsFramePaintLayer aWhichLayer);

  NS_IMETHOD  AppendFrames(nsIPresContext* aPresContext,
                           nsIPresShell&   aPresShell,
                           nsIAtom*        aListName,
                           nsIFrame*       aFrameList);

  NS_IMETHOD  InsertFrames(nsIPresContext* aPresContext,
                           nsIPresShell&   aPresShell,
                           nsIAtom*        aListName,
                           nsIFrame*       aPrevFrame,
                           nsIFrame*       aFrameList);

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsString& aResult) const {
    return MakeFrameName("GroupBoxFrame", aResult);
  }
#endif

  NS_IMETHOD Reflow(nsIPresContext* aPresContext,
                    nsHTMLReflowMetrics& aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus& aStatus);
  
  virtual PRBool GetInitialOrientation(PRBool& aHorizontal) { aHorizontal = PR_FALSE; return PR_TRUE; }

  nsIFrame* GetTitleFrame(nsIPresContext* aPresContext, nsRect& aRect);
  nsIFrame* GetContentFrame(nsIPresContext* aPresContext);

};

class nsTitledBoxInnerFrame : public nsBoxFrame {
public:

    nsTitledBoxInnerFrame(nsIPresShell* aShell):nsBoxFrame(aShell) {}


#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsString& aResult) const {
    return MakeFrameName("TitledBoxFrameInner", aResult);
  }
#endif
  
  // we are always flexible
  virtual PRBool GetDefaultFlex(PRInt32& aFlex) { aFlex = 1; return PR_TRUE; }

};

nsresult
NS_NewTitledBoxInnerFrame(nsIPresShell* aPresShell, nsIFrame** aNewFrame)
{
  NS_PRECONDITION(aNewFrame, "null OUT ptr");
  if (nsnull == aNewFrame) {
    return NS_ERROR_NULL_POINTER;
  }
  nsTitledBoxInnerFrame* it = new (aPresShell) nsTitledBoxInnerFrame(aPresShell);
  if (!it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  
  *aNewFrame = it;
  return NS_OK;
}

nsresult
NS_NewTitledBoxFrame(nsIPresShell* aPresShell, nsIFrame** aNewFrame)
{
  NS_PRECONDITION(aNewFrame, "null OUT ptr");
  if (nsnull == aNewFrame) {
    return NS_ERROR_NULL_POINTER;
  }
  nsTitledBoxFrame* it = new (aPresShell) nsTitledBoxFrame(aPresShell);
  if (!it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  
  *aNewFrame = it;
  return NS_OK;
}

nsTitledBoxFrame::nsTitledBoxFrame(nsIPresShell* aShell):nsBoxFrame(aShell)
{
}


NS_IMETHODIMP
nsTitledBoxFrame::SetInitialChildList(nsIPresContext* aPresContext,
                                     nsIAtom*        aListName,
                                     nsIFrame*       aChildList)
{ 
  // Queue up the frames for the content frame
  return nsBoxFrame::SetInitialChildList(aPresContext, nsnull, aChildList);
}

// this is identical to nsHTMLContainerFrame::Paint except for the background and border. 
NS_IMETHODIMP
nsTitledBoxFrame::Paint(nsIPresContext* aPresContext,
                       nsIRenderingContext& aRenderingContext,
                       const nsRect& aDirtyRect,
                       nsFramePaintLayer aWhichLayer)
{
  if (NS_FRAME_PAINT_LAYER_BACKGROUND == aWhichLayer) {
    // Paint our background and border
    const nsStyleDisplay* disp =
      (const nsStyleDisplay*)mStyleContext->GetStyleData(eStyleStruct_Display);

    if (disp->IsVisible() && mRect.width && mRect.height) {
      PRIntn skipSides = GetSkipSides();
      const nsStyleColor* color =
        (const nsStyleColor*)mStyleContext->GetStyleData(eStyleStruct_Color);
      const nsStyleSpacing* spacing =
        (const nsStyleSpacing*)mStyleContext->GetStyleData(eStyleStruct_Spacing);
       
        nsMargin border;
        if (!spacing->GetBorder(border)) {
          NS_NOTYETIMPLEMENTED("percentage border");
        }

        nscoord yoff = 0;

        nsRect titleRect;
        nsIFrame* titleFrame = GetTitleFrame(aPresContext, titleRect);

        if (titleFrame) {        
            // if the border is smaller than the legend. Move the border down
            // to be centered on the legend. 
            const nsStyleSpacing* titleSpacing;
            titleFrame->GetStyleData(eStyleStruct_Spacing,
                                    (const nsStyleStruct*&) titleSpacing);

            nsMargin titleMargin;
            titleSpacing->GetMargin(titleMargin);
            titleRect.Inflate(titleMargin);
         
            if (border.top < titleRect.height)
                yoff = (titleRect.height - border.top)/2 + titleRect.y;
        }

        nsRect rect(0, yoff, mRect.width, mRect.height - yoff);

        nsCSSRendering::PaintBackground(aPresContext, aRenderingContext, this,
                                        aDirtyRect, rect, *color, *spacing, 0, 0);


        if (titleFrame) {

          // we should probably use PaintBorderEdges to do this but for now just use clipping
          // to achieve the same effect.
          PRBool clipState;

          // draw left side
          nsRect clipRect(rect);
          clipRect.width = titleRect.x - rect.x;
          clipRect.height = border.top;

          aRenderingContext.PushState();
          aRenderingContext.SetClipRect(clipRect, nsClipCombine_kIntersect, clipState);
          nsCSSRendering::PaintBorder(aPresContext, aRenderingContext, this,
                                      aDirtyRect, rect, *spacing, mStyleContext, skipSides);
  
          aRenderingContext.PopState(clipState);


          // draw right side
          clipRect = rect;
          clipRect.x = titleRect.x + titleRect.width;
          clipRect.width -= (titleRect.x + titleRect.width);
          clipRect.height = border.top;

          aRenderingContext.PushState();
          aRenderingContext.SetClipRect(clipRect, nsClipCombine_kIntersect, clipState);
          nsCSSRendering::PaintBorder(aPresContext, aRenderingContext, this,
                                      aDirtyRect, rect, *spacing, mStyleContext, skipSides);
  
          aRenderingContext.PopState(clipState);

          
        
          // draw bottom

          clipRect = rect;
          clipRect.y += border.top;
          clipRect.height = mRect.height - (yoff + border.top);
        
          aRenderingContext.PushState();
          aRenderingContext.SetClipRect(clipRect, nsClipCombine_kIntersect, clipState);
          nsCSSRendering::PaintBorder(aPresContext, aRenderingContext, this,
                                      aDirtyRect, rect, *spacing, mStyleContext, skipSides);
  
          aRenderingContext.PopState(clipState);
          
        } else {

          
          nsCSSRendering::PaintBorder(aPresContext, aRenderingContext, this,
                                      aDirtyRect, nsRect(0,0,mRect.width, mRect.height), *spacing, mStyleContext, skipSides);
        }
    }
  }

  PaintChildren(aPresContext, aRenderingContext, aDirtyRect, aWhichLayer);

#ifdef DEBUG
  if ((NS_FRAME_PAINT_LAYER_DEBUG == aWhichLayer) && GetShowFrameBorders()) {
    nsIView* view;
    GetView(aPresContext, &view);
    if (nsnull != view) {
      aRenderingContext.SetColor(NS_RGB(0,0,255));
    }
    else {
      aRenderingContext.SetColor(NS_RGB(255,0,0));
    }
    aRenderingContext.DrawRect(0, 0, mRect.width, mRect.height);
  }
#endif
  return NS_OK;
}

nsIFrame*
nsTitledBoxFrame::GetTitleFrame(nsIPresContext* aPresContext, nsRect& aTitleRect)
{
    // if we only have one child fail
    nsIFrame* frame = mFrames.FirstChild();
    nsIFrame* next = nsnull;
    frame->GetNextSibling(&next);
    if (!next)
        return nsnull;

    // if we have more than one child. The first is our baby.
    // then get the first child inside.
    nsIFrame* child;
    frame->FirstChild(aPresContext, nsnull, &child);

    if (child) {
       // convert to our coordinates.
       nsRect parentRect;
       frame->GetRect(parentRect);
       child->GetRect(aTitleRect);
       aTitleRect.x += parentRect.x;
       aTitleRect.y += parentRect.y;
    }

    return child;
}

nsIFrame*
nsTitledBoxFrame::GetContentFrame(nsIPresContext* aPresContext)
{
    // the content frame is always our second frame. The title frame
    // is the first.
    nsIFrame* frame = mFrames.FirstChild();
    nsIFrame* next = nsnull;
    frame->GetNextSibling(&next);
    if (!next)
        return frame;
    else 
        return next;
}

NS_IMETHODIMP 
nsTitledBoxFrame::Reflow(nsIPresContext*          aPresContext,
                        nsHTMLReflowMetrics&     aDesiredSize,
                        const nsHTMLReflowState& aReflowState,
                        nsReflowStatus&          aStatus)
{
  if (aReflowState.mComputedBorderPadding.top != 0) 
  {
      nsHTMLReflowState newState(aReflowState);

      if (newState.mComputedHeight != NS_INTRINSICSIZE)
        newState.mComputedHeight += aReflowState.mComputedBorderPadding.top;

      // remove the border from border padding
      ((nsMargin&)newState.mComputedBorderPadding).top = 0;
      ((nsMargin&)newState.mComputedPadding).top = 0;

      // reflow us again with the correct values.
      return Reflow(aPresContext, aDesiredSize, newState, aStatus);
  }

  return nsBoxFrame::Reflow(aPresContext, aDesiredSize, aReflowState, aStatus);
}

NS_IMETHODIMP
nsTitledBoxFrame::InsertFrames(nsIPresContext* aPresContext,
                            nsIPresShell& aPresShell,
                            nsIAtom* aListName,
                            nsIFrame* aPrevFrame,
                            nsIFrame* aFrameList)
{
   nsIFrame* content = GetContentFrame(aPresContext);
   NS_ASSERTION(content, "ERROR TitledBoxFrame has no content!!!");
   return content->InsertFrames(aPresContext, aPresShell, aListName, aPrevFrame, aFrameList);
}


NS_IMETHODIMP
nsTitledBoxFrame::AppendFrames(nsIPresContext* aPresContext,
                           nsIPresShell&   aPresShell,
                           nsIAtom*        aListName,
                           nsIFrame*       aFrameList)
{
   nsIFrame* content = GetContentFrame(aPresContext);
   NS_ASSERTION(content, "ERROR TitledBoxFrame has no content!!!");
   return content->AppendFrames(aPresContext, aPresShell, aListName, aFrameList); 
}

