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

// Original Author:
// David W Hyatt (hyatt@netscape.com)
// Netscape Communications
//
// See documentation in associated header file
//

#include "nsTreeLayout.h"
#include "nsBoxLayoutState.h"
#include "nsIBox.h"
#include "nsIScrollableFrame.h"
#include "nsBox.h"

// ------ nsTreeLayout ------


nsresult
NS_NewTreeLayout( nsIPresShell* aPresShell, nsCOMPtr<nsIBoxLayout>& aNewLayout)
{
  aNewLayout = new nsTreeLayout(aPresShell);

  return NS_OK;
} 

nsTreeLayout::nsTreeLayout(nsIPresShell* aPresShell):nsTempleLayout(aPresShell)
{
}

nsXULTreeOuterGroupFrame* nsTreeLayout::GetOuterFrame(nsIBox* aBox)
{
  nsCOMPtr<nsIXULTreeSlice> slice(do_QueryInterface(aBox));
  if (slice) {
    PRBool outer;
    slice->IsOutermostFrame(&outer);
    if (outer)
      return (nsXULTreeOuterGroupFrame*) aBox;
  }
  return nsnull;
}

nsXULTreeGroupFrame* nsTreeLayout::GetGroupFrame(nsIBox* aBox)
{
  nsCOMPtr<nsIXULTreeSlice> slice(do_QueryInterface(aBox));
  if (slice) {
    PRBool group;
    slice->IsGroupFrame(&group);
    if (group)
      return (nsXULTreeGroupFrame*) aBox;
  }
  return nsnull;
}

nsXULTreeSliceFrame* nsTreeLayout::GetRowFrame(nsIBox* aBox)
{
  nsCOMPtr<nsIXULTreeSlice> slice(do_QueryInterface(aBox));
  if (slice) {
    PRBool row;
    slice->IsRowFrame(&row);
    if (row)
      return (nsXULTreeSliceFrame*) aBox;
  }
  return nsnull;
}

NS_IMETHODIMP
nsTreeLayout::GetPrefSize(nsIBox* aBox, nsBoxLayoutState& aBoxLayoutState, nsSize& aSize)
{
  nsresult rv = nsTempleLayout::GetPrefSize(aBox, aBoxLayoutState, aSize);
  nsXULTreeOuterGroupFrame* frame = GetOuterFrame(aBox);
  if (frame) {
    nscoord rowheight = frame->GetRowHeightTwips();
    aSize.height = frame->GetRowCount() * rowheight;
    // Pad the height.
    nscoord y = frame->GetAvailableHeight();
    if (aSize.height > y && y > 0 && rowheight > 0) {
      nscoord m = (aSize.height-y)%rowheight;
      nscoord remainder = m == 0 ? 0 : rowheight - m;
      aSize.height += remainder;
    }
  }
  return rv;
}

NS_IMETHODIMP
nsTreeLayout::GetMinSize(nsIBox* aBox, nsBoxLayoutState& aBoxLayoutState, nsSize& aSize)
{
  nsresult rv = nsTempleLayout::GetMinSize(aBox, aBoxLayoutState, aSize);
  nsXULTreeOuterGroupFrame* frame = GetOuterFrame(aBox);
  if (frame) {
    nscoord rowheight = frame->GetRowHeightTwips();
    aSize.height = frame->GetRowCount() * rowheight;
    // Pad the height.
    nscoord y = frame->GetAvailableHeight();
    if (aSize.height > y && y > 0 && rowheight > 0) {
      nscoord m = (aSize.height-y)%rowheight;
      nscoord remainder = m == 0 ? 0 : rowheight - m;
      aSize.height += remainder;
    }
  }
  return rv;
}

NS_IMETHODIMP
nsTreeLayout::GetMaxSize(nsIBox* aBox, nsBoxLayoutState& aBoxLayoutState, nsSize& aSize)
{
  nsresult rv = nsTempleLayout::GetMaxSize(aBox, aBoxLayoutState, aSize);
  nsXULTreeOuterGroupFrame* frame = GetOuterFrame(aBox);
  if (frame) {
    nscoord rowheight = frame->GetRowHeightTwips();
    aSize.height = frame->GetRowCount() * rowheight;
    // Pad the height.
    nscoord y = frame->GetAvailableHeight();
    if (aSize.height > y && y > 0 && rowheight > 0) {
      nscoord m = (aSize.height-y)%rowheight;
      nscoord remainder = m == 0 ? 0 : rowheight - m;
      aSize.height += remainder;
    }
  }
  return rv;
}


NS_IMETHODIMP
nsTreeLayout::Layout(nsIBox* aBox, nsBoxLayoutState& aState)
{
  // Get the start y position.
  nsXULTreeGroupFrame* frame = GetGroupFrame(aBox);
  if (!frame) {
    NS_ERROR("Frame encountered that isn't a tree row group!\n");
    return NS_ERROR_FAILURE;
  }

  // Get our client rect.
  nsRect clientRect;
  aBox->GetClientRect(clientRect);

  // Get the starting y position and the remaining available
  // height.
  nscoord availableHeight = frame->GetAvailableHeight();
  nscoord yOffset = frame->GetYPosition();
  nscoord currY = 0;
  
  if (availableHeight <= 0)
    return NS_OK;

  // Walk our frames, building them dynamically as needed.
  nsIBox* box = frame->GetFirstTreeBox();
  while (box) {  
    // If this box is dirty or if it has dirty children, we
    // call layout on it.
    PRBool dirty = PR_FALSE;           
    PRBool dirtyChildren = PR_FALSE;           
    box->IsDirty(dirty);
    box->HasDirtyChildren(dirtyChildren);
    
    PRBool sizeChanged = PR_FALSE;
    nsRect childRect;
    box->GetContentRect(childRect);
    nsMargin margin(0,0,0,0);
    box->GetMargin(margin);
    childRect.Inflate(margin);
    nsSize size;
    box->NeedsRecalc();
    box->GetPrefSize(aState, size);
    if (clientRect.width != childRect.width || size.height != childRect.height)
      sizeChanged = PR_TRUE;

    if (sizeChanged || dirty || dirtyChildren || aState.GetLayoutReason() == nsBoxLayoutState::Initial) {
      PRBool isRow = PR_TRUE;
      nsXULTreeGroupFrame* childGroup = GetGroupFrame(box);
      if (childGroup) {
        // Set the available height.
        childGroup->SetAvailableHeight(availableHeight);
        isRow = PR_FALSE;
      }
      childRect.width = clientRect.width;

      box->GetMargin(margin);
      childRect.Deflate(margin);
      box->SetBounds(aState, childRect);
      box->Layout(aState);
      // Get the prefsize.
      nsSize size;
      box->NeedsRecalc();
      box->GetPrefSize(aState, size);
      childRect.height = size.height;
      box->SetBounds(aState, childRect);
      if (isRow) {
        frame->GetOuterFrame()->SetRowHeight(size.height);
      }
    }

    // Place the child by just grabbing its rect and adjusting the x,y.
    box->GetContentRect(childRect);
    childRect.x = 0;
    childRect.y = currY + yOffset;
    yOffset += childRect.height;
    availableHeight -= childRect.height;
    box->GetMargin(margin);
    childRect.Deflate(margin);
    childRect.width < 0 ? 0 : childRect.width;
    childRect.height < 0 ? 0 : childRect.height;
    
    box->SetBounds(aState, childRect);

    if (!frame->ContinueReflow(availableHeight))
      break;

    box = frame->GetNextTreeBox(box);
  }

  return NS_OK;
}

