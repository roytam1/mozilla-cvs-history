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
 */


#include "nsCOMPtr.h"
#include "nsHTMLParts.h"
#include "nsIHTMLContent.h"
#include "nsFrame.h"
#include "nsLineLayout.h"
#include "nsHTMLIIDs.h"
#include "nsIPresContext.h"
#include "nsHTMLAtoms.h"
#include "nsUnitConversion.h"
#include "nsIStyleContext.h"
#include "nsStyleConsts.h"
#include "nsINameSpaceManager.h"
#include "nsIRenderingContext.h"
#include "nsIFontMetrics.h"
#include "nsStyleUtil.h"

#include "nsMathMLmfracFrame.h"

//
// <mfrac> -- form a fraction from two subexpressions - implementation
//

nsresult
NS_NewMathMLmfracFrame(nsIPresShell* aPresShell, nsIFrame** aNewFrame)
{
  NS_PRECONDITION(aNewFrame, "null OUT ptr");
  if (nsnull == aNewFrame) {
    return NS_ERROR_NULL_POINTER;
  }
  nsMathMLmfracFrame* it = new (aPresShell) nsMathMLmfracFrame;
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  *aNewFrame = it;
  return NS_OK;
}

nsMathMLmfracFrame::nsMathMLmfracFrame()
{
}

nsMathMLmfracFrame::~nsMathMLmfracFrame()
{
}

NS_IMETHODIMP
nsMathMLmfracFrame::Init(nsIPresContext*  aPresContext,
                         nsIContent*      aContent,
                         nsIFrame*        aParent,
                         nsIStyleContext* aContext,
                         nsIFrame*        aPrevInFlow)
{
  nsresult rv = NS_OK;

  rv = nsMathMLContainerFrame::Init(aPresContext, aContent, aParent, aContext, aPrevInFlow);
  if (NS_FAILED(rv)) {
    return rv;
  }

  nsAutoString value;

  float p2t;
  aPresContext->GetScaledPixelsToTwips(&p2t);
  nscoord onePixel = NSIntPixelsToTwips(1, p2t);

  nscoord defaultThickness = onePixel * DEFAULT_FRACTION_LINE_THICKNESS;

  mLineThickness = defaultThickness;

  // see if the linethickness attribute is there
  if (NS_CONTENT_ATTR_HAS_VALUE == 
      GetAttribute(mContent, mPresentationData.mstyle, 
                   nsMathMLAtoms::linethickness_, value))
  {
    if (value == "thin")
      mLineThickness = onePixel * THIN_FRACTION_LINE_THICKNESS;
    else if (value == "medium")
      mLineThickness = onePixel * MEDIUM_FRACTION_LINE_THICKNESS;
    else if (value == "thick")
      mLineThickness = onePixel * THICK_FRACTION_LINE_THICKNESS;
    else { // see if it is a plain number, or a percentage, or a h/v-unit like 1ex, 2px, 1em
      nsCSSValue cssValue;
      if (ParseNumericValue(value, cssValue)) {
        nsCSSUnit unit = cssValue.GetUnit();
        if (eCSSUnit_Number == unit)
          mLineThickness = nscoord(float(defaultThickness) * cssValue.GetFloatValue());
        else if (eCSSUnit_Percent == unit)
          mLineThickness = nscoord(float(defaultThickness) * cssValue.GetPercentValue());
        else if (eCSSUnit_Null != unit)
          mLineThickness = CalcLength(aPresContext, mStyleContext, cssValue);
      }
#ifdef NS_DEBUG
      else {
        char str[50];
        value.ToCString(str, 50);
        printf("Invalid attribute linethickness=%s\n", str);
      }
#endif
    }
  }

  mLineOrigin.x = 0;
  mLineOrigin.y = 0;

  return rv;
}

NS_IMETHODIMP
nsMathMLmfracFrame::Paint(nsIPresContext*      aPresContext,
                          nsIRenderingContext& aRenderingContext,
                          const nsRect&        aDirtyRect,
                          nsFramePaintLayer    aWhichLayer)
{
  nsresult rv = NS_OK;

  /////////////
  // paint the numerator and denominator
  rv = nsMathMLContainerFrame::Paint(aPresContext, aRenderingContext,
                                     aDirtyRect, aWhichLayer);

  if (NS_FRAME_PAINT_LAYER_FOREGROUND != aWhichLayer) {
    return rv;
  }

  ////////////
  // paint the fraction line
  if (NS_SUCCEEDED(rv) && 0 < mLineThickness) {

/*
//  line looking like <hr noshade>
    const nsStyleColor* color;
    nscolor colors[2];
    color = nsStyleUtil::FindNonTransparentBackground(mStyleContext);
    NS_Get3DColors(colors, color->mBackgroundColor);
    aRenderingContext.SetColor(colors[0]);
*/
//  solid line with the current text color
    const nsStyleColor* color =
      (const nsStyleColor*)mStyleContext->GetStyleData(eStyleStruct_Color);
    aRenderingContext.SetColor(color->mColor);

//  draw the line
    aRenderingContext.FillRect(mLineOrigin.x, mLineOrigin.y, mRect.width, mLineThickness);
  }

  return rv;
}

NS_IMETHODIMP
nsMathMLmfracFrame::Reflow(nsIPresContext*          aPresContext,
                           nsHTMLReflowMetrics&     aDesiredSize,
                           const nsHTMLReflowState& aReflowState,
                           nsReflowStatus&          aStatus)
{
  nsresult rv = NS_OK;
  nsReflowStatus childStatus;
  // ask our children to compute their bounding metrics
  nsHTMLReflowMetrics childDesiredSize(aDesiredSize.maxElementSize,
                      aDesiredSize.mFlags | NS_REFLOW_CALC_BOUNDING_METRICS);
  nsSize availSize(aReflowState.mComputedWidth, aReflowState.mComputedHeight);

  //////////////////
  // Reflow Children

  nscoord count = 0;
  nsRect rect[2];
  nsIFrame* child[2];
  nsIFrame* childFrame = mFrames.FirstChild();
  while (nsnull != childFrame)
  {
    //////////////
    // WHITESPACE: don't forget that whitespace doesn't count in MathML!
    if (IsOnlyWhitespace(childFrame)) {
      ReflowEmptyChild(aPresContext, childFrame);
    }
    else if (2 > count)  {

      nsHTMLReflowState childReflowState(aPresContext, aReflowState, childFrame, availSize);
      rv = ReflowChild(childFrame, aPresContext, childDesiredSize, childReflowState, childStatus);
      NS_ASSERTION(NS_FRAME_IS_COMPLETE(childStatus), "bad status");
      if (NS_FAILED(rv)) {
        return rv;
      }

      child[count] = childFrame;
      rect[count].width = childDesiredSize.width;
      rect[count].height = childDesiredSize.height;
      count++;
    }
//  else { invalid markup... }

    rv = childFrame->GetNextSibling(&childFrame);
    NS_ASSERTION(NS_SUCCEEDED(rv),"failed to get next child");
  }

  //////////////////
  // Place Children

  // Get the <strike> line and center the fraction bar with the <strike> line.
  nscoord strikeOffset, strikeThickness;
  nsCOMPtr<nsIFontMetrics> fm;
  const nsStyleFont* aFont =
    (const nsStyleFont*)mStyleContext->GetStyleData(eStyleStruct_Font);
  aPresContext->GetMetricsFor(aFont->mFont, getter_AddRefs(fm));
  fm->GetStrikeout(strikeOffset, strikeThickness);

  // Take care of mLineThickness
  float p2t;
  nscoord halfThickspace;
  aPresContext->GetScaledPixelsToTwips(&p2t);
  nscoord onePixel = NSIntPixelsToTwips(1, p2t);

  // distance from the middle of the axis
  halfThickspace = (mLineThickness > onePixel)? mLineThickness/2 : 0;

  aDesiredSize.width = PR_MAX(rect[0].width, rect[1].width);
  aDesiredSize.ascent = rect[0].height + strikeOffset + halfThickspace;
  aDesiredSize.descent = rect[1].height - strikeOffset + halfThickspace;
  aDesiredSize.height = aDesiredSize.ascent + aDesiredSize.descent;

  rect[0].x = (aDesiredSize.width - rect[0].width) / 2; // center w.r.t largest width
  rect[1].x = (aDesiredSize.width - rect[1].width) / 2;
  rect[0].y = 0;
  rect[1].y = aDesiredSize.height - rect[1].height;

  // child[0]->SetRect(aPresContext, rect[0]);
  // child[1]->SetRect(aPresContext, rect[1]);
  nsHTMLReflowMetrics childSize(nsnull);
  for (PRInt32 i=0; i<count; i++) {
    childSize.width = rect[i].width;
    childSize.height = rect[i].height;
    FinishReflowChild(child[i], aPresContext, childSize, rect[i].x, rect[i].y, 0);
  }
  SetLineOrigin(nsPoint(0,rect[0].height)); // position the fraction bar

  if (nsnull != aDesiredSize.maxElementSize) {
    aDesiredSize.maxElementSize->width = aDesiredSize.width;
    aDesiredSize.maxElementSize->height = aDesiredSize.height;
  }
  aStatus = NS_FRAME_COMPLETE;

  // XXX Fix me!
  mBoundingMetrics.ascent  = aDesiredSize.ascent;
  mBoundingMetrics.descent = aDesiredSize.descent;
  mBoundingMetrics.width   = aDesiredSize.width;

  return NS_OK;
}

