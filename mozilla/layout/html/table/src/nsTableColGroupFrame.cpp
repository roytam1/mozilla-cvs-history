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
#include "nsReflowCommand.h"
#include "nsIStyleContext.h"
#include "nsStyleConsts.h"
#include "nsIPresContext.h"

static PRBool gsDebug = PR_FALSE;


nsTableColGroupFrame::nsTableColGroupFrame(nsIContent* aContent,
                     PRInt32     aIndexInParent,
                     nsIFrame*   aParentFrame)
  : nsContainerFrame(aContent, aIndexInParent, aParentFrame)
{
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


NS_METHOD
nsTableColGroupFrame::ResizeReflow(nsIPresContext* aPresContext,
                        nsReflowMetrics& aDesiredSize,
                        const nsSize&   aMaxSize,
                        nsSize*         aMaxElementSize,
                        ReflowStatus& aStatus)
{
  NS_ASSERTION(nsnull!=aPresContext, "bad arg");
  if (gsDebug==PR_TRUE) printf("nsTableColGroupFrame::ResizeReflow\n");
  aDesiredSize.width=0;
  aDesiredSize.height=0;
  if (nsnull!=aMaxElementSize)
  {
    aMaxElementSize->width=0;
    aMaxElementSize->height=0;
  }
  aStatus = nsIFrame::frComplete;
  return NS_OK;
}

NS_METHOD
nsTableColGroupFrame::IncrementalReflow(nsIPresContext*  aPresContext,
                                        nsReflowMetrics& aDesiredSize,
                                        const nsSize&    aMaxSize,
                                        nsReflowCommand& aReflowCommand,
                                        ReflowStatus&    aStatus)
{
  NS_ASSERTION(nsnull!=aPresContext, "bad arg");
  if (gsDebug==PR_TRUE) printf("nsTableColGroupFrame::IncrementalReflow\n");
  return NS_OK;
}

nsresult nsTableColGroupFrame::NewFrame(nsIFrame** aInstancePtrResult,
                                        nsIContent* aContent,
                                        PRInt32     aIndexInParent,
                                        nsIFrame*   aParent)
{
  NS_PRECONDITION(nsnull != aInstancePtrResult, "null ptr");
  if (nsnull == aInstancePtrResult) {
    return NS_ERROR_NULL_POINTER;
  }
  nsIFrame* it = new nsTableColGroupFrame(aContent, aIndexInParent, aParent);
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  *aInstancePtrResult = it;
  return NS_OK;
}
