/*
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is Mozilla MathML Project.
 * 
 * The Initial Developer of the Original Code is The University Of 
 * Queensland.  Portions created by The University Of Queensland are
 * Copyright (C) 1999 The University Of Queensland.  All Rights Reserved.
 * 
 * Contributor(s): 
 *   Roger B. Sidje <rbs@maths.uq.edu.au>
 *   David J. Fiddes <D.J.Fiddes@hw.ac.uk>
 *   Shyjan Mahamud <mahamud@cs.cmu.edu> (added TeX rendering rules)
 */

#ifndef nsMathMLContainerFrame_h___
#define nsMathMLContainerFrame_h___

#include "nsCOMPtr.h"
#include "nsHTMLContainerFrame.h"
#include "nsBlockFrame.h"
#include "nsInlineFrame.h"
#include "nsMathMLAtoms.h"
#include "nsMathMLOperators.h"
#include "nsMathMLChar.h"
#include "nsMathMLFrame.h"
#include "nsMathMLParts.h"

/*
 * Base class for MathML container frames. It acts like an inferred 
 * mrow. By default, this frame uses its Reflow() method to lay its 
 * children horizontally and ensure that their baselines are aligned.
 * The Reflow() method relies upon Place() to position children.
 * By overloading Place() in derived classes, it is therefore possible
 * to position children in various customized ways.
 */

// Parameters to handle the change of font-size induced by changing the
// scriptlevel. These are hard-coded values that match with the rules in
// mathml.css. Note that mScriptLevel can exceed these bounds, but the
// scaling effect on the font-size will be bounded. The following bounds can
// be expanded provided the new corresponding rules are added in mathml.css.
#define NS_MATHML_CSS_POSITIVE_SCRIPTLEVEL_LIMIT  +5
#define NS_MATHML_CSS_NEGATIVE_SCRIPTLEVEL_LIMIT  -5
#define NS_MATHML_SCRIPTSIZEMULTIPLIER             0.71f
#define NS_MATHML_SCRIPTMINSIZE                    8

// Options for the preferred size at which to stretch our stretchy children 
#define STRETCH_CONSIDER_ACTUAL_SIZE    0x00000001 // just use our current size
#define STRETCH_CONSIDER_EMBELLISHMENTS 0x00000002 // size calculations include embellishments

class nsMathMLContainerFrame : public nsHTMLContainerFrame,
                               public nsMathMLFrame {
public:

  NS_DECL_ISUPPORTS_INHERITED

  // --------------------------------------------------------------------------
  // Overloaded nsMathMLFrame methods -- see documentation in nsIMathMLFrame.h

  NS_IMETHOD
  Stretch(nsIPresContext*      aPresContext,
          nsIRenderingContext& aRenderingContext,
          nsStretchDirection   aStretchDirection,
          nsBoundingMetrics&   aContainerSize,
          nsHTMLReflowMetrics& aDesiredStretchSize);

  NS_IMETHOD
  Place(nsIPresContext*      aPresContext,
        nsIRenderingContext& aRenderingContext,
        PRBool               aPlaceOrigin,
        nsHTMLReflowMetrics& aDesiredSize);

  NS_IMETHOD
  EmbellishOperator();

  NS_IMETHOD
  UpdatePresentationDataFromChildAt(nsIPresContext* aPresContext,
                                    PRInt32         aFirstIndex,
                                    PRInt32         aLastIndex,
                                    PRInt32         aScriptLevelIncrement,
                                    PRUint32        aFlagsValues,
                                    PRUint32        aFlagsToUpdate)
  {
    if (!aFlagsToUpdate && !aScriptLevelIncrement)
      return NS_OK;
    PRInt32 index = 0;
    nsIFrame* childFrame = mFrames.FirstChild();
    while (childFrame) {
      if ((index >= aFirstIndex) &&
          ((aLastIndex <= 0) || ((aLastIndex > 0) && (index <= aLastIndex)))) {
        PropagatePresentationDataFor(aPresContext, childFrame,
          aScriptLevelIncrement, aFlagsValues, aFlagsToUpdate);
      }
      index++;
      childFrame->GetNextSibling(&childFrame);
    }
    return NS_OK;
  }

  NS_IMETHOD
  ReResolveScriptStyle(nsIPresContext* aPresContext,
                       PRInt32         aParentScriptLevel)
  {
    PropagateScriptStyleFor(aPresContext, this, aParentScriptLevel);
    return NS_OK;
  }

  // --------------------------------------------------------------------------
  // Overloaded nsHTMLContainerFrame methods -- see documentation in nsIFrame.h
 
  NS_IMETHOD
  GetFrameType(nsIAtom** aType) const;

  NS_IMETHOD
  Init(nsIPresContext*  aPresContext,
       nsIContent*      aContent,
       nsIFrame*        aParent,
       nsIStyleContext* aContext,
       nsIFrame*        aPrevInFlow);

  NS_IMETHOD
  SetInitialChildList(nsIPresContext* aPresContext,
                      nsIAtom*        aListName,
                      nsIFrame*       aChildList);

  NS_IMETHODIMP
  ReflowDirtyChild(nsIPresShell* aPresShell, 
                   nsIFrame*     aChild);

  NS_IMETHOD
  Reflow(nsIPresContext*          aPresContext,
         nsHTMLReflowMetrics&     aDesiredSize,
         const nsHTMLReflowState& aReflowState,
         nsReflowStatus&          aStatus);

  NS_IMETHOD
  DidReflow(nsIPresContext*           aPresContext,
            const nsHTMLReflowState*  aReflowState,
            nsDidReflowStatus         aStatus)

  {
    mEmbellishData.flags &= ~NS_MATHML_STRETCH_DONE;
    return nsHTMLContainerFrame::DidReflow(aPresContext, aReflowState, aStatus);
  }

  NS_IMETHOD
  AttributeChanged(nsIPresContext* aPresContext,
                   nsIContent*     aChild,
                   PRInt32         aNameSpaceID,
                   nsIAtom*        aAttribute,
                   PRInt32         aModType, 
                   PRInt32         aHint);

  NS_IMETHOD 
  Paint(nsIPresContext*      aPresContext,
        nsIRenderingContext& aRenderingContext,
        const nsRect&        aDirtyRect,
        nsFramePaintLayer    aWhichLayer,
        PRUint32             aFlags = 0);

  // --------------------------------------------------------------------------
  // Additional methods 

  // helper to get the preferred size that a container frame should use to fire
  // the stretch on its stretchy child frames.
  virtual void
  GetPreferredStretchSize(nsIPresContext*      aPresContext,
                          nsIRenderingContext& aRenderingContext,
                          PRUint32             aOptions,
                          nsStretchDirection   aStretchDirection,
                          nsBoundingMetrics&   aPreferredStretchSize);

  // error handlers to report than an error (typically invalid markup)
  // was encountered during reflow. By default the user will see the
  // Unicode REPLACEMENT CHAR U+FFFD at the spot where the error was
  // encountered.
  virtual nsresult
  ReflowError(nsIPresContext*      aPresContext,
              nsIRenderingContext& aRenderingContext,
              nsHTMLReflowMetrics& aDesiredSize);
  virtual nsresult
  PaintError(nsIPresContext*      aPresContext,
             nsIRenderingContext& aRenderingContext,
             const nsRect&        aDirtyRect,
             nsFramePaintLayer    aWhichLayer);

  // helper function to reflow token elements
  static nsresult
  ReflowTokenFor(nsIFrame*                aFrame,
                 nsIPresContext*          aPresContext,
                 nsHTMLReflowMetrics&     aDesiredSize,
                 const nsHTMLReflowState& aReflowState,
                 nsReflowStatus&          aStatus);

  // helper function to place token elements
  static nsresult
  PlaceTokenFor(nsIFrame*            aFrame,
                nsIPresContext*      aPresContext,
                nsIRenderingContext& aRenderingContext,
                PRBool               aPlaceOrigin,  
                nsHTMLReflowMetrics& aDesiredSize);

  // helper method to reflow a child frame. We are inline frames, and we don't
  // know our positions until reflow is finished. That's why we ask the
  // base method not to worry about our position.
  nsresult 
  ReflowChild(nsIFrame*                aKidFrame,
              nsIPresContext*          aPresContext,
              nsHTMLReflowMetrics&     aDesiredSize,
              const nsHTMLReflowState& aReflowState,
              nsReflowStatus&          aStatus)
  {
    return nsHTMLContainerFrame::ReflowChild(aKidFrame, aPresContext, aDesiredSize, aReflowState,
                                             0, 0, NS_FRAME_NO_MOVE_FRAME, aStatus);
  }

  // helper to add the inter-spacing when <math> is the immediate parent.
  // Since we don't (yet) handle the root <math> element ourselves, we need to
  // take special care of the inter-frame spacing on elements for which <math>
  // is the direct xml parent. This function will be repeatedly called from
  // left to right on the childframes of <math>, and by so doing it will
  // emulate the spacing that would have been done by a <mrow> container.
  // e.g., it fixes <math> <mi>f</mi> <mo>q</mo> <mi>f</mi> <mo>I</mo> </math>
  virtual nsresult
  FixInterFrameSpacing(nsIPresContext*      aPresContext,
                       nsHTMLReflowMetrics& aDesiredSize);

  // helper method to complete the post-reflow hook and ensure that embellished
  // operators don't terminate their Reflow without receiving a Stretch command.
  virtual nsresult
  FinalizeReflow(nsIPresContext*      aPresContext,
                 nsIRenderingContext& aRenderingContext,
                 nsHTMLReflowMetrics& aDesiredSize);

  // helper method to facilitate getting the reflow and bounding metrics
  // IMPORTANT: This function is only meant to be called in Place() methods 
  // where it is assumed that the frame's rect is still acting as place holder
  // for the frame's ascent and descent information
  static void
  GetReflowAndBoundingMetricsFor(nsIFrame*            aFrame,
                                 nsHTMLReflowMetrics& aReflowMetrics,
                                 nsBoundingMetrics&   aBoundingMetrics);

  // helper to let the scriptstyle re-resolution pass through
  // a subtree that may contain non-MathML container frames
  static void
  PropagateScriptStyleFor(nsIPresContext* aPresContext,
                          nsIFrame*       aFrame,
                          PRInt32         aFrameScriptLevel);

  // helper to let the update of presentation data pass through
  // a subtree that may contain non-MathML container frames
  static void
  PropagatePresentationDataFor(nsIPresContext* aPresContext,
                               nsIFrame*       aFrame,
                               PRInt32         aScriptLevelIncrement,
                               PRUint32        aFlagsValues,
                               PRUint32        aFlagsToUpdate);

protected:
  virtual PRIntn GetSkipSides() const { return 0; }
};


// --------------------------------------------------------------------------
// Currently, to benefit from line-breaking inside the <math> element, <math> is
// simply mapping to nsBlockFrame or nsInlineFrame.
// A separate implemention needs to provide:
// 1) line-breaking
// 2) proper inter-frame spacing
// 3) firing of Stretch() (in which case FinalizeReflow() would have to be cleaned)
// Issues: If/when mathml becomes a pluggable component, the separation will be needed.
class nsMathMLmathBlockFrame : public nsBlockFrame {
public:
  friend nsresult NS_NewMathMLmathBlockFrame(nsIPresShell* aPresShell, nsIFrame** aNewFrame);

  // beware, mFrames is not set by nsBlockFrame, FirstChild() is your friend
  // when you need to access the child list of the block
  NS_IMETHOD
  SetInitialChildList(nsIPresContext* aPresContext,
                      nsIAtom*        aListName,
                      nsIFrame*       aChildList)
  {
    nsresult rv;

    rv = nsBlockFrame::SetInitialChildList(aPresContext, aListName, aChildList);

    // re-resolve our subtree to set any mathml-expected scriptsizes
    nsMathMLContainerFrame::PropagateScriptStyleFor(aPresContext, this, 0);

    return rv;
  }

protected:
  nsMathMLmathBlockFrame() {}
  virtual ~nsMathMLmathBlockFrame() {}
};

class nsMathMLmathInlineFrame : public nsInlineFrame {
public:
  friend nsresult NS_NewMathMLmathInlineFrame(nsIPresShell* aPresShell, nsIFrame** aNewFrame);

  NS_IMETHOD
  SetInitialChildList(nsIPresContext* aPresContext,
                      nsIAtom*        aListName,
                      nsIFrame*       aChildList)
  {
    nsresult rv;
    rv = nsInlineFrame::SetInitialChildList(aPresContext, aListName, aChildList);

    // re-resolve our subtree to set any mathml-expected scriptsizes
    nsMathMLContainerFrame::PropagateScriptStyleFor(aPresContext, this, 0);

    return rv;
  }

protected:
  nsMathMLmathInlineFrame() {}
  virtual ~nsMathMLmathInlineFrame() {}
};
#endif /* nsMathMLContainerFrame_h___ */
