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
#include "nsTableColGroupFrame.h"
#include "nsTableColFrame.h"
#include "nsTableFrame.h"
#include "nsITableContent.h"
#include "nsIReflowCommand.h"
#include "nsIStyleContext.h"
#include "nsStyleConsts.h"
#include "nsIPresContext.h"
#include "nsIPtr.h"
#include "nsIContentDelegate.h"
#include "nsHTMLAtoms.h"

NS_DEF_PTR(nsIContent);
NS_DEF_PTR(nsIStyleContext);

static NS_DEFINE_IID(kITableContentIID, NS_ITABLECONTENT_IID);

static PRBool gsDebug = PR_FALSE;


nsTableColGroupFrame::nsTableColGroupFrame(nsIContent* aContent,
                     nsIFrame*   aParentFrame)
  : nsContainerFrame(aContent, aParentFrame)
{
  mColCount=0;
}

nsTableColGroupFrame::~nsTableColGroupFrame()
{
}

NS_METHOD nsTableColGroupFrame::Paint(nsIPresContext& aPresContext,
                                      nsIRenderingContext& aRenderingContext,
                                      const nsRect&        aDirtyRect)
{
  if (gsDebug==PR_TRUE) printf("nsTableColGroupFrame::Paint\n");
  PaintChildren(aPresContext, aRenderingContext, aDirtyRect);
  return NS_OK;
}

// TODO:  incremental reflow
// today, we just throw away the column frames and start over every time
// this is dumb, we should be able to maintain column frames and adjust incrementally
NS_METHOD nsTableColGroupFrame::Reflow(nsIPresContext&      aPresContext,
                                       nsReflowMetrics&     aDesiredSize,
                                       const nsReflowState& aReflowState,
                                       nsReflowStatus&      aStatus)
{
  NS_ASSERTION(nsnull!=mContent, "bad state -- null content for frame");

  // for every content child that (is a column thingy and does not already have a frame)
  // create a frame and adjust it's style
  nsIFrame* kidFrame = nsnull;
  nsIFrame* prevKidFrame;
 
  LastChild(prevKidFrame);  // XXX remember this...
  PRInt32 kidIndex = 0;     // index of the content child we are currently working on
  PRInt32 colIndex = 0;     // number of content children that are columns, normally same as kidIndex
  for (;;)
  {
    // get the next content child, breaking if there is none
    nsIContentPtr kid;
    mContent->ChildAt(kidIndex, kid.AssignRef());   // kid: REFCNT++
    if (kid.IsNull()) {
      break;
    }

    // verify that we're dealing with table content.  If so, we know it's a column
    nsITableContent *tableContentInterface = nsnull;
    nsresult rv = kid->QueryInterface(kITableContentIID, 
                                      (void **)&tableContentInterface);  // tableContentInterface: REFCNT++
    if (NS_FAILED(rv))
    {
      kidIndex++;
      continue;
    }
    NS_RELEASE(tableContentInterface);                                   // tableContentInterface: REFCNT--

    if (mChildCount<=colIndex)
    {
      // Resolve style
      nsIStyleContextPtr kidSC =
        aPresContext.ResolveStyleContextFor(kid, this, PR_TRUE);
      const nsStyleSpacing* kidSpacing = (const nsStyleSpacing*)
        kidSC->GetStyleData(eStyleStruct_Spacing);

      // Create a child frame
      nsIContentDelegate* kidDel = nsnull;
      kidDel = kid->GetDelegate(&aPresContext);
      rv = kidDel->CreateFrame(&aPresContext, kid, this, kidSC, kidFrame);
      NS_RELEASE(kidDel);

      // give the child frame a chance to reflow, even though we know it'll have 0 size
      nsReflowMetrics kidSize(nsnull);
      nsReflowState   kidReflowState(kidFrame, aReflowState, nsSize(0,0), eReflowReason_Initial);
      kidFrame->WillReflow(aPresContext);
      nsReflowStatus status = ReflowChild(kidFrame,&aPresContext, kidSize,
                                          kidReflowState);
      // note that DidReflow is called as the result of some ancestor firing off a DidReflow above me
      kidFrame->SetRect(nsRect(0,0,0,0));

      // set nsColFrame-specific information
      ((nsTableColFrame *)kidFrame)->SetColumnIndex(colIndex+mStartColIndex);
      nsIFrame* tableFrame=nsnull;
      GetGeometricParent(tableFrame);
      ((nsTableFrame *)tableFrame)->AddColumnFrame((nsTableColFrame *)kidFrame);

      // Link child frame into the list of children
      if (nsnull != prevKidFrame) {
        prevKidFrame->SetNextSibling(kidFrame);
      } else {
        mFirstChild = kidFrame;  // our first child
        SetFirstContentOffset(kidIndex);
      }
      prevKidFrame = kidFrame;
      mChildCount++;
      SetStyleContextForFirstPass(&aPresContext, colIndex);
    }
    colIndex++; // if this wasn't a column, we would not have gotten this far
    kidIndex++;
  }
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

// Subclass hook for style post processing
NS_METHOD nsTableColGroupFrame::SetStyleContextForFirstPass(nsIPresContext* aPresContext,
                                                            PRInt32         aColIndex)
{
  // get the table frame
  nsIFrame* tableFrame=nsnull;
  GetGeometricParent(tableFrame);
  tableFrame->GetGeometricParent(tableFrame); // get the outer frame
  
  // get the style for the table frame
  nsIStyleContextPtr tableSC;
  tableFrame->GetStyleContext(aPresContext, tableSC.AssignRef());
  nsStyleTable *tableStyle = (nsStyleTable*)tableSC->GetStyleData(eStyleStruct_Table);

  // if COLS is set, then map it into the COL frames
  if (NS_STYLE_TABLE_COLS_NONE != tableStyle->mCols)
  {
    // set numCols to the number of columns effected by the COLS attribute
    PRInt32 numCols=0;
    if (NS_STYLE_TABLE_COLS_ALL == tableStyle->mCols)
      ChildCount(numCols);
    else 
      numCols = tableStyle->mCols;

    // for every column effected, set its width style
    PRInt32 colIndex=0;
    nsIFrame *colFrame=nsnull;
    nsIStyleContextPtr colStyleContext;
    ChildAt(aColIndex, colFrame);
    if (nsnull!=colFrame)
    {
      nsStylePosition * colPosition=nsnull;
      colFrame->GetStyleContext(aPresContext, colStyleContext.AssignRef());
      colPosition = (nsStylePosition*)colStyleContext->GetMutableStyleData(eStyleStruct_Position);
      nsStyleCoord width (1, eStyleUnit_Proportional);
      colPosition->mWidth = width;
      colStyleContext->RecalcAutomaticData(aPresContext);

      // if there are more columns, there width is set to "minimum"
      PRInt32 numChildFrames;
      ChildCount(numChildFrames);
      for (; aColIndex<numChildFrames-1; colIndex++)
      {
        ChildAt(colIndex, colFrame);
        if (nsnull==colFrame)
          break;
        nsStylePosition * colPosition=nsnull;
        colFrame->GetStyleContext(aPresContext, colStyleContext.AssignRef());
        colPosition = (nsStylePosition*)colStyleContext->GetMutableStyleData(eStyleStruct_Position);
        colPosition->mWidth.SetCoordValue(0);
        colStyleContext->RecalcAutomaticData(aPresContext);
      }
    }

    mStyleContext->RecalcAutomaticData(aPresContext);
  }
  return NS_OK;
}


/** returns the number of columns represented by this group.
  * if there are col children, count them (taking into account the span of each)
  * else, check my own span attribute.
  */
int nsTableColGroupFrame::GetColumnCount ()
{
  mColCount=0;
  int count;
  ChildCount (count);
  if (0 < count)
  {
    nsIFrame * child = nsnull;
    ChildAt(0, child);
    NS_ASSERTION(nsnull!=child, "bad child");
    while (nsnull!=child)
    {
      nsTableColFrame *col = (nsTableColFrame *)child;
      col->SetColumnIndex (mStartColIndex + mColCount);
      mColCount += col->GetRepeat ();
      child->GetNextSibling(child);
    }
  }
  else
  {
    const nsStyleTable *tableStyle;
    GetStyleData(eStyleStruct_Table, (nsStyleStruct *&)tableStyle);
    mColCount = tableStyle->mSpan;
  }
  return mColCount;
}

/* ----- static methods ----- */

nsresult nsTableColGroupFrame::NewFrame(nsIFrame** aInstancePtrResult,
                                        nsIContent* aContent,
                                        nsIFrame*   aParent)
{
  NS_PRECONDITION(nsnull != aInstancePtrResult, "null ptr");
  if (nsnull == aInstancePtrResult) {
    return NS_ERROR_NULL_POINTER;
  }
  nsIFrame* it = new nsTableColGroupFrame(aContent, aParent);
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  *aInstancePtrResult = it;
  return NS_OK;
}


