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
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

#include "nsTableFrame.h"
#include "nsVoidArray.h"
#include "nsISelfScrollingFrame.h"
#include "nsITreeFrame.h"
#include "nsILayoutHistoryState.h"
#include "nsCOMPtr.h"

class nsTreeCellFrame;
class nsTreeRowGroupFrame;
class nsTreeTwistyListener;
class nsILayoutHistoryState;

class nsTreeFrame : public nsTableFrame,
                    public nsITreeFrame,
                    public nsISelfScrollingFrame
{
public:
  friend nsresult NS_NewTreeFrame(nsIPresShell* aPresShell, nsIFrame** aNewFrame);

  NS_DECL_ISUPPORTS_INHERITED

  void SetSelection(nsIPresContext* presContext, nsTreeCellFrame* pFrame);
  void ToggleSelection(nsIPresContext* presContext, nsTreeCellFrame* pFrame);
  void RangedSelection(nsIPresContext* aPresContext, nsTreeCellFrame* pEndFrame);
  
  void MoveUp(nsIPresContext* aPresContext, nsTreeCellFrame* pFrame);
  void MoveDown(nsIPresContext* aPresContext, nsTreeCellFrame* pFrame);
  void MoveLeft(nsIPresContext* aPresContext, nsTreeCellFrame* pFrame);
  void MoveRight(nsIPresContext* aPresContext, nsTreeCellFrame* pFrame);
  void MoveToRowCol(nsIPresContext* aPresContext, PRInt32 row, PRInt32 col);
    
  PRBool IsSlatedForReflow() { return mSlatedForReflow; };
  void SlateForReflow() { mSlatedForReflow = PR_TRUE; };

  void GetTreeBody(nsTreeRowGroupFrame** aResult);

  // Overridden methods
  NS_IMETHOD Destroy(nsIPresContext* aPresContext);
  PRBool RowGroupsShouldBeConstrained() { return PR_TRUE; }
  
  NS_IMETHOD Reflow(nsIPresContext*          aPresContext,
							         nsHTMLReflowMetrics&     aMetrics,
							         const nsHTMLReflowState& aReflowState,
							         nsReflowStatus&          aStatus);
  NS_IMETHOD HandleEvent(nsIPresContext* aPresContext, 
                             nsGUIEvent*     aEvent,
                             nsEventStatus*  aEventStatus);

  NS_IMETHOD Init(nsIPresContext*  aPresContext,
                  nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIStyleContext* aContext,
                  nsIFrame*        aPrevInFlow);

  NS_IMETHOD  DidReflow(nsIPresContext*   aPresContext,
                        nsDidReflowStatus aStatus);

  PRInt32 GetCurrentGeneration() { return mGeneration; };
  void SetCurrentGeneration(PRInt32 aGeneration) { mGeneration = aGeneration; };

  PRBool UseGeneration() { return mUseGeneration; };
  void SetUseGeneration(PRBool aUse) { mUseGeneration = aUse; };

  PRBool ContainsFlexibleColumn(PRInt32 aStartIndex, PRInt32 aEndIndex, nsTableColFrame** aResult);

  NS_IMETHOD MarkForDirtyReflow(nsIPresContext* aPresContext);

  void SuppressReflow() { mSuppressReflow = PR_TRUE; };
  void UnsuppressReflow() { mSuppressReflow = PR_FALSE; };

  PRInt32 GetInsertionIndex(nsIFrame *aFrame);

  // nsISelfScrollingFrame interface
  NS_IMETHOD ScrollByLines(nsIPresContext* aPresContext, PRInt32 lines);
  NS_IMETHOD CollapseScrollbar(nsIPresContext* aPresContext, PRBool aHide);

  // nsITreeFrame.h
  NS_IMETHOD EnsureRowIsVisible(PRInt32 aRowIndex);

protected:
  nsTreeFrame();
  virtual ~nsTreeFrame();

protected: // Data Members
  PRBool mSlatedForReflow; // If set, don't waste time scheduling excess reflows.
  nsTreeTwistyListener* mTwistyListener;
  PRInt32 mGeneration;
  PRBool mUseGeneration;
  PRBool mSuppressReflow;
}; // class nsTreeFrame
