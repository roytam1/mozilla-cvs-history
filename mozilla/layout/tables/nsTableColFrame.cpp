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
#include "nsTableColFrame.h"
#include "nsContainerFrame.h"
#include "nsIReflowCommand.h"
#include "nsIStyleContext.h"
#include "nsStyleConsts.h"
#include "nsIPresContext.h"
#include "nsHTMLIIDs.h"
#include "nsHTMLAtoms.h"

#ifdef NS_DEBUG
static PRBool gsDebug = PR_FALSE;
static PRBool gsNoisyRefs = PR_FALSE;
#else
static const PRBool gsDebug = PR_FALSE;
static const PRBool gsNoisyRefs = PR_FALSE;
#endif

nsTableColFrame::nsTableColFrame(nsIContent* aContent, nsIFrame* aParentFrame)
  : nsFrame(aContent, aParentFrame)
{
  mColIndex = 0;
  mRepeat = 1;
  mMaxColWidth = 0;
  mMinColWidth = 0;
  mMaxEffectiveColWidth = 0;
  mMinEffectiveColWidth = 0;
  mMinAdjustedColWidth = 0;
  mWidthSource = eWIDTH_SOURCE_NONE;
}


nsTableColFrame::~nsTableColFrame()
{
}

NS_METHOD nsTableColFrame::Paint(nsIPresContext& aPresContext,
                                 nsIRenderingContext& aRenderingContext,
                                 const nsRect& aDirtyRect)
{
  if (gsDebug==PR_TRUE)
    printf("nsTableColFrame::Paint\n");
  return NS_OK;
}


NS_METHOD nsTableColFrame::Reflow(nsIPresContext*      aPresContext,
                                  nsHTMLReflowMetrics& aDesiredSize,
                                  const nsReflowState& aReflowState,
                                  nsReflowStatus&      aStatus)
{
  NS_ASSERTION(nsnull!=aPresContext, "bad arg");
  aDesiredSize.width=0;
  aDesiredSize.height=0;
  if (nsnull!=aDesiredSize.maxElementSize)
  {
    aDesiredSize.maxElementSize->width=0;
    aDesiredSize.maxElementSize->height=0;
  }
  aStatus = NS_FRAME_COMPLETE;
  return NS_OK;
}


/* ----- global methods ----- */

nsresult 
NS_NewTableColFrame(nsIContent* aContent,
                    nsIFrame*   aParentFrame,
                    nsIFrame*&  aResult)
{
  nsIFrame* it = new nsTableColFrame(aContent, aParentFrame);
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  aResult = it;
  return NS_OK;
}

/* ----- debugging methods ----- */
NS_METHOD nsTableColFrame::List(FILE* out, PRInt32 aIndent, nsIListFilter *aFilter) const
{
  // if a filter is present, only output this frame if the filter says we should
  // since this could be any "tag" with the right display type, we'll
  // just pretend it's a cell
  if (nsnull==aFilter)
    return nsFrame::List(out, aIndent, aFilter);

  nsAutoString tagString("col");
  if (PR_TRUE==aFilter->OutputTag(&tagString))
  {
    // Indent
    for (PRInt32 i = aIndent; --i >= 0; ) fputs("  ", out);

    // Output the tag and rect
    nsIAtom* tag;
    mContent->GetTag(tag);
    if (tag != nsnull) {
      nsAutoString buf;
      tag->ToString(buf);
      fputs(buf, out);
      NS_RELEASE(tag);
    }
    PRInt32 contentIndex;

    GetContentIndex(contentIndex);
    fprintf(out, "(%d)", contentIndex);
    out << mRect;
    if (0 != mState) {
      fprintf(out, " [state=%08x]", mState);
    }
    fputs("\n", out);
  }

  return NS_OK;
}