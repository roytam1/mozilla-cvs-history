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

#ifndef nsMathMLmstyleFrame_h___
#define nsMathMLmstyleFrame_h___

#include "nsCOMPtr.h"
#include "nsMathMLContainerFrame.h"

//
// <mstyle> -- style change
//

#define NS_MATHML_MSTYLE_SCRIPTLEVEL_EXPLICIT  (1)
#define NS_MATHML_MSTYLE_DISPLAYSTYLE          (1<<1)

#define NS_MATHML_MSTYLE_HAS_SCRIPTLEVEL_EXPLICIT(_flags) \
  (NS_MATHML_MSTYLE_SCRIPTLEVEL_EXPLICIT == ((_flags) & NS_MATHML_MSTYLE_SCRIPTLEVEL_EXPLICIT))

#define NS_MATHML_MSTYLE_HAS_DISPLAYSTYLE(_flags) \
  (NS_MATHML_MSTYLE_DISPLAYSTYLE == ((_flags) & NS_MATHML_MSTYLE_DISPLAYSTYLE))


class nsMathMLmstyleFrame : public nsMathMLContainerFrame {
public:
  friend nsresult NS_NewMathMLmstyleFrame(nsIFrame** aNewFrame);

  NS_IMETHOD
  Init(nsIPresContext&  aPresContext,
       nsIContent*      aContent,
       nsIFrame*        aParent,
       nsIStyleContext* aContext,
       nsIFrame*        aPrevInFlow);

  NS_IMETHOD
  UpdatePresentationData(PRInt32 aScriptLevel,
                         PRBool  aDisplayStyle);

  NS_IMETHOD
  UpdatePresentationDataFromChildAt(PRInt32 aIndex,
                                    PRInt32 aScriptLevelIncrement,
                                    PRBool  aDisplayStyle);

  NS_IMETHOD
  SetInitialChildList(nsIPresContext& aPresContext,
                      nsIAtom*        aListName,
                      nsIFrame*       aChildList)
  {
    nsresult rv;
    rv = nsMathMLContainerFrame::SetInitialChildList(aPresContext, aListName, aChildList);
    ReResolveStyleContext(&aPresContext, mStyleContext, NS_STYLE_HINT_REFLOW, nsnull, nsnull);
    return rv;
  }

protected:
  nsMathMLmstyleFrame();
  virtual ~nsMathMLmstyleFrame();

  virtual PRIntn GetSkipSides() const { return 0; }

  PRInt32 mInnerScriptLevelIncrement;
  PRInt32 mFlags;
};

#endif /* nsMathMLmstyleFrame_h___ */
