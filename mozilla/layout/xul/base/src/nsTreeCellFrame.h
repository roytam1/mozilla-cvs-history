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
 * Original Author: David W. Hyatt (hyatt@netscape.com)
 *
 * Contributor(s): 
 */

#include "nsTableCellFrame.h"

class nsTableColFrame;
class nsTreeFrame;

class nsTreeCellFrame : public nsTableCellFrame
{
public:
  friend nsresult NS_NewTreeCellFrame(nsIPresShell* aPresShell, nsIFrame** aNewFrame);

  NS_IMETHOD AttributeChanged(nsIPresContext* aPresContext,
                              nsIContent* aChild,
                              PRInt32 aNameSpaceID,
                              nsIAtom* aAttribute,
                              PRInt32 aHint);
  NS_IMETHOD GetFrameForPoint(nsIPresContext* aPresContext,
                              const nsPoint& aPoint, // Overridden to capture events
                              nsFramePaintLayer aWhichLayer,
                              nsIFrame**     aFrame);

  NS_IMETHOD HandleEvent(nsIPresContext* aPresContext,
                        nsGUIEvent* aEvent,
                        nsEventStatus* aEventStatus);

  NS_IMETHOD Init(nsIPresContext*  aPresContext,
                  nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIStyleContext* aContext,
                  nsIFrame*        aPrevInFlow); // Overridden to set whether we're a column header 

  NS_IMETHOD Reflow(nsIPresContext* aPresContext,
                    nsHTMLReflowMetrics& aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus& aStatus);

  NS_IMETHOD Destroy(nsIPresContext* aPresContext);

  NS_IMETHOD GetCursor(nsIPresContext* aPresContext,
                                     nsPoint&        aPoint,
                                     PRInt32&        aCursor);

  void Hover(nsIPresContext* presContext, PRBool isHover, PRBool notifyForReflow = PR_TRUE);

  nsTableFrame* GetTreeFrame();

  void SetAllowEvents(PRBool allowEvents) { mAllowEvents = allowEvents; };
  void ToggleOpenClose();
  void Open();
  void Close();

  static PRBool ShouldBuildCell(nsIFrame* aParentFrame, nsIContent* aCellContent);

protected:
  nsTreeCellFrame();
  virtual ~nsTreeCellFrame();

  nsresult HandleMouseDownEvent(nsIPresContext* aPresContext, 
								                nsGUIEvent*     aEvent,
							                  nsEventStatus*  aEventStatus);
  
  nsresult HandleMouseEnterEvent(nsIPresContext* aPresContext, 
								                nsGUIEvent*     aEvent,
							                  nsEventStatus*  aEventStatus);
  
  nsresult HandleMouseExitEvent(nsIPresContext* aPresContext, 
								                nsGUIEvent*     aEvent,
							                  nsEventStatus*  aEventStatus);

  nsresult HandleDoubleClickEvent(nsIPresContext* aPresContext, 
								                  nsGUIEvent*     aEvent,
							                    nsEventStatus*  aEventStatus);

  PRBool CanResize(nsPoint& aPoint, nsTableColFrame** aResult);

  NS_IMETHOD DidSetStyleContext(nsIPresContext* aPresContext) { return NS_OK; };

protected:
  // Data members
  PRBool mIsHeader; // Whether or not we're a column header
  nsTreeFrame* mTreeFrame; // Our parent tree frame.
  PRBool mAllowEvents; // Whether we let events go through.
}; // class nsTableCellFrame
