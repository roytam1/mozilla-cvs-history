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

#include "nsMathMLmsupFrame.h"

//
// <msup> -- attach a superscript to a base - implementation
//

nsresult
NS_NewMathMLmsupFrame(nsIFrame** aNewFrame)
{
  NS_PRECONDITION(aNewFrame, "null OUT ptr");
  if (nsnull == aNewFrame) {
    return NS_ERROR_NULL_POINTER;
  }
  nsMathMLmsupFrame* it = new nsMathMLmsupFrame;
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  *aNewFrame = it;
  return NS_OK;
}

nsMathMLmsupFrame::nsMathMLmsupFrame()
{
}

nsMathMLmsupFrame::~nsMathMLmsupFrame()
{
}

NS_IMETHODIMP
nsMathMLmsupFrame::Reflow(nsIPresContext*          aPresContext,
                          nsHTMLReflowMetrics&     aDesiredSize,
                          const nsHTMLReflowState& aReflowState,
                          nsReflowStatus&          aStatus)
{
  nsresult rv = NS_OK;
  nsReflowStatus childStatus;
  nsHTMLReflowMetrics childDesiredSize(aDesiredSize.maxElementSize);
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
    else if (2 > count) {

      nsHTMLReflowState childReflowState(aPresContext, aReflowState, 
                                         childFrame, availSize);  	
      rv = ReflowChild(childFrame, aPresContext, childDesiredSize, 
                       childReflowState, childStatus);
      NS_ASSERTION(NS_FRAME_IS_COMPLETE(childStatus), "bad status");
      if (NS_FAILED(rv)) {
        return rv;
      }                  

      child[count] = childFrame;      
      rect[count].width = childDesiredSize.width;
      rect[count].height = childDesiredSize.height;
      if (0 == count) {
        aDesiredSize.descent = childDesiredSize.descent;
      }
      count++;
    }
//  else { invalid markup... }

    rv = childFrame->GetNextSibling(&childFrame);
    NS_ASSERTION(NS_SUCCEEDED(rv),"failed to get next child");
  }

  //////////////////
  // Place Children 
  
  // Get the superscript offset
  nscoord superscriptOffset, leading;
  nsCOMPtr<nsIFontMetrics> fm;
  const nsStyleFont* aFont =
    (const nsStyleFont*)mStyleContext->GetStyleData(eStyleStruct_Font);
  aPresContext->GetMetricsFor(aFont->mFont, getter_AddRefs(fm));
  fm->GetSuperscriptOffset(superscriptOffset);
  fm->GetLeading(leading);

  superscriptOffset += leading;

  aDesiredSize.height = rect[0].height + rect[1].height - superscriptOffset;
  aDesiredSize.width = rect[0].width + rect[1].width;
  rect[0].x = 0;
  rect[1].x = rect[0].width;
  rect[1].y = 0;
  rect[0].y = aDesiredSize.height - rect[0].height;  
  aDesiredSize.ascent = aDesiredSize.height - aDesiredSize.descent;
  
  child[0]->SetRect(aPresContext, rect[0]);
  child[1]->SetRect(aPresContext, rect[1]);
 
  if (nsnull != aDesiredSize.maxElementSize) {
    aDesiredSize.maxElementSize->width = aDesiredSize.width;
    aDesiredSize.maxElementSize->height = aDesiredSize.height;
  }
  aStatus = NS_FRAME_COMPLETE;
  return NS_OK;
}
