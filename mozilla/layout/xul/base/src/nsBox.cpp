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
 * Author: Eric D Vaughan <evaughan@netscape.com>
 *
 * Contributor(s): 
 *   Pierre Phaneuf <pp@ludusdesign.com>
 */

#include "nsBoxLayoutState.h"
#include "nsBox.h"
#include "nsBoxFrame.h"
#include "nsIStyleContext.h"
#include "nsIPresContext.h"
#include "nsCOMPtr.h"
#include "nsIContent.h"
#include "nsIViewManager.h"
#include "nsIView.h"
#include "nsIPresShell.h"
#include "nsHTMLContainerFrame.h"
#include "nsISelfScrollingFrame.h"
#include "nsINameSpaceManager.h"
#include "nsHTMLAtoms.h"
#include "nsXULAtoms.h"
#include "nsIDOMNode.h"
#include "nsIDOMNamedNodeMap.h"
#include "nsIDOMAttr.h"
#include "nslog.h"
#undef fprintf

NS_IMPL_LOG(nsBoxLog)
#define PRINTF NS_LOG_PRINTF(nsBoxLog)
#define FLUSH  NS_LOG_FLUSH(nsBoxLog)


//#define DEBUG_LAYOUT

#ifdef DEBUG_COELESCED
static PRInt32 coelesced = 0;
#endif

PRInt32 gIndent = 0;
PRInt32 gLayout = 0;

#ifdef DEBUG_LAYOUT
void
nsBoxAddIndents()
{
    for(PRInt32 i=0; i < gIndent; i++)
    {
      PRINTF(" ");
    }
}
#endif

void
nsBox::AppendAttribute(const nsAutoString& aAttribute, const nsAutoString& aValue, nsAutoString& aResult)
{
   aResult.Append(aAttribute);
   aResult.AppendWithConversion("='");
   aResult.Append(aValue);
   aResult.AppendWithConversion("' ");
}

void
nsBox::ListBox(nsAutoString& aResult)
{
    nsAutoString name;
    nsIFrame* frame;
    GetFrame(&frame);
    GetBoxName(name);

    char addr[100];
    sprintf(addr, "[@%p] ", frame);

    aResult.AppendWithConversion(addr);
    aResult.Append(name);
    aResult.AppendWithConversion(" ");

    nsCOMPtr<nsIContent> content;
    frame->GetContent(getter_AddRefs(content));

    // add on all the set attributes
    if (content) {
      nsCOMPtr<nsIDOMNode> node(do_QueryInterface(content));
      nsCOMPtr<nsIDOMNamedNodeMap> namedMap;

      node->GetAttributes(getter_AddRefs(namedMap));
      PRUint32 length;
      namedMap->GetLength(&length);

      nsCOMPtr<nsIDOMNode> attribute;
      for (PRUint32 i = 0; i < length; ++i)
      {
        namedMap->Item(i, getter_AddRefs(attribute));
        nsCOMPtr<nsIDOMAttr> attr(do_QueryInterface(attribute));
        nsAutoString name;
        attr->GetName(name);
        nsAutoString value;
        attr->GetValue(value);
        AppendAttribute(name, value, aResult);
      }
    }

}

NS_IMETHODIMP
nsBox::DumpBox(FILE* aFile)
{
  nsAutoString s;
  ListBox(s);
  char ch[1000];
  s.ToCString(ch,1000);
  fprintf(aFile, "%s", ch);
  return NS_OK;
}

/**
 * Hack for deck who requires that all its children has widgets
 */
NS_IMETHODIMP 
nsBox::ChildrenMustHaveWidgets(PRBool& aMust)
{
  aMust = PR_FALSE;
  return NS_OK;
}

void
nsBox::GetBoxName(nsAutoString& aName)
{
  aName.AssignWithConversion("Box");
}

NS_IMETHODIMP
nsBox::BeginLayout(nsBoxLayoutState& aState)
{
  #ifdef DEBUG_LAYOUT 

      nsBoxAddIndents();

      nsAutoString reason;
      switch(aState.GetLayoutReason())
      {
        case nsBoxLayoutState::Dirty:
           reason.AssignWithConversion("Dirty");
        break;
        case nsBoxLayoutState::Initial:
           reason.AssignWithConversion("Initial");
        break;
        case nsBoxLayoutState::Resize:
           reason.AssignWithConversion("Resize");
        break;
      }

      char ch[100];
      reason.ToCString(ch,100);
      PRINTF("%s Layout: ", ch);
      DumpBox(stdout);
      PRINTF("\n");
      gIndent++;
  #endif

  return NS_OK;
}

NS_IMETHODIMP
nsBox::DoLayout(nsBoxLayoutState& aState)
{
  return NS_OK;
}

NS_IMETHODIMP
nsBox::EndLayout(nsBoxLayoutState& aState)
{

  #ifdef DEBUG_LAYOUT
      --gIndent;
  #endif

  return SyncLayout(aState);
}

#ifdef REFLOW_COELESCED
void Coelesced()
{
  PRINTF("Coelesed=%d\n", ++coelesced);

}

#endif

nsBox::nsBox(nsIPresShell* aShell):mMouseThrough(unset),
                                   mNextChild(nsnull),
                                   mParentBox(nsnull)
                                   
{
  //mX = 0;
  //mY = 0;
}

NS_IMETHODIMP 
nsBox::GetInset(nsMargin& margin)
{
  margin.SizeTo(0,0,0,0);
  return NS_OK;
}

NS_IMETHODIMP
nsBox::IsDirty(PRBool& aDirty)
{
  nsFrameState state;
  nsIFrame* frame;
  GetFrame(&frame);
  frame->GetFrameState(&state);

  aDirty = (state & NS_FRAME_IS_DIRTY);  
  return NS_OK;
}

NS_IMETHODIMP
nsBox::HasDirtyChildren(PRBool& aDirty)
{
  nsFrameState state;
  nsIFrame* frame;
  GetFrame(&frame);
  frame->GetFrameState(&state);

  aDirty = (state & NS_FRAME_HAS_DIRTY_CHILDREN);  
  return NS_OK;
}

NS_IMETHODIMP
nsBox::MarkDirty(nsBoxLayoutState& aState)
{
  NeedsRecalc();

  nsFrameState state;
  nsIFrame* frame;
  GetFrame(&frame);
  frame->GetFrameState(&state);

  // only reflow if we aren't already dirty.
  if (state & NS_FRAME_IS_DIRTY) {      
#ifdef DEBUG_COELESCED
      Coelesced();
#endif
      return NS_OK;
  }

  state |= NS_FRAME_IS_DIRTY;
  frame->SetFrameState(state);

  nsCOMPtr<nsIBoxLayout> layout;
  GetLayoutManager(getter_AddRefs(layout));
  if (layout)
    layout->BecameDirty(this, aState);

  if (state & NS_FRAME_HAS_DIRTY_CHILDREN) {   
#ifdef DEBUG_COELESCED
      Coelesced();
#endif
      return NS_OK;
  }

  nsIBox* parent = nsnull;
  GetParentBox(&parent);
  if (parent)
     return parent->RelayoutDirtyChild(aState, this);
  else {
    nsIFrame* parentFrame = nsnull;
    frame->GetParent(&parentFrame);
    nsCOMPtr<nsIPresShell> shell;
    aState.GetPresShell(getter_AddRefs(shell));
    return parentFrame->ReflowDirtyChild(shell, frame);
  }
}

NS_IMETHODIMP
nsBox::MarkDirtyChildren(nsBoxLayoutState& aState)
{
  return RelayoutDirtyChild(aState, nsnull);
}

NS_IMETHODIMP
nsBox::MarkStyleChange(nsBoxLayoutState& aState)
{
  NeedsRecalc();

  if (HasStyleChange())
    return NS_OK;

  // iterate through all children making them dirty
  MarkChildrenStyleChange();

  nsCOMPtr<nsIBoxLayout> layout;
  GetLayoutManager(getter_AddRefs(layout));
  if (layout)
    layout->BecameDirty(this, aState);

  nsIBox* parent = nsnull;
  GetParentBox(&parent);
  if (parent)
     return parent->RelayoutDirtyChild(aState, this);
  else {
    /*
    nsCOMPtr<nsIPresShell> shell;
    aState.GetPresShell(getter_AddRefs(shell));
    nsIFrame* frame = nsnull;
    GetFrame(&frame);
    nsFrame::CreateAndPostReflowCommand(shell, frame, 
      nsIReflowCommand::StyleChange, nsnull, nsnull, nsnull);
    return NS_OK;
    */
    nsIFrame* frame = nsnull;
    GetFrame(&frame);
    nsIFrame* parentFrame = nsnull;
    frame->GetParent(&parentFrame);
    nsCOMPtr<nsIPresShell> shell;
    aState.GetPresShell(getter_AddRefs(shell));
    return parentFrame->ReflowDirtyChild(shell, frame);

  }

  return NS_OK;
}

PRBool
nsBox::HasStyleChange()
{
  PRBool aDirty = PR_FALSE;
  IsDirty(aDirty);
  return aDirty;
}

void
nsBox::SetStyleChangeFlag(PRBool aDirty)
{
  NeedsRecalc();

  nsFrameState state;
  nsIFrame* frame;
  GetFrame(&frame);
  frame->GetFrameState(&state);
  state |= (NS_FRAME_IS_DIRTY | NS_FRAME_HAS_DIRTY_CHILDREN);
  frame->SetFrameState(state);
}

NS_IMETHODIMP
nsBox::MarkChildrenStyleChange()
{
  // only reflow if we aren't already dirty.
  if (HasStyleChange()) {   
#ifdef DEBUG_COELESCED
    PRINTF("StyleChange reflows coelesced=%d\n", ++StyleCoelesced);  
#endif
    return NS_OK;
  }

  SetStyleChangeFlag(PR_TRUE);

  nsIBox* child = nsnull;
  GetChildBox(&child);
  while(child)
  {
    child->MarkChildrenStyleChange();
    child->GetNextBox(&child);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsBox::RelayoutStyleChange(nsBoxLayoutState& aState, nsIBox* aChild)
{
    NS_ERROR("Don't call this!!");

    /*
    nsFrameState state;
    nsIFrame* frame;
    GetFrame(&frame);
    frame->GetFrameState(&state);

    // if we are not dirty mark ourselves dirty and tell our parent we are dirty too.
    if (!HasStyleChange()) {      
      // Mark yourself as dirty and needing to be recalculated
      SetStyleChangeFlag(PR_TRUE);
      NeedsRecalc();

      if (aChild != nsnull) {
          nsCOMPtr<nsIBoxLayout> layout;
          GetLayoutManager(getter_AddRefs(layout));
          if (layout)
            layout->ChildBecameDirty(this, aState, aChild);
      }

      nsIBox* parent = nsnull;
      GetParentBox(&parent);
      if (parent)
         return parent->RelayoutStyleChange(aState, this);
      else {
        nsCOMPtr<nsIPresShell> shell;
        aState.GetPresShell(getter_AddRefs(shell));
        nsIFrame* frame = nsnull;
        aChild->GetFrame(&frame);
        nsFrame::CreateAndPostReflowCommand(shell, frame, 
          nsIReflowCommand::StyleChanged, nsnull, nsnull, nsnull);
        return NS_OK;
      }
    } else {
#ifdef DEBUG_COELESCED
      Coelesced();
#endif
    }
   */

    return NS_OK;
}

NS_IMETHODIMP
nsBox::RelayoutDirtyChild(nsBoxLayoutState& aState, nsIBox* aChild)
{
    nsFrameState state;
    nsIFrame* frame;
    GetFrame(&frame);
    frame->GetFrameState(&state);

    if (aChild != nsnull) {
        nsCOMPtr<nsIBoxLayout> layout;
        GetLayoutManager(getter_AddRefs(layout));
        if (layout)
          layout->ChildBecameDirty(this, aState, aChild);
    }

    // if we are not dirty mark ourselves dirty and tell our parent we are dirty too.
    if (!(state & NS_FRAME_HAS_DIRTY_CHILDREN)) {      
      // Mark yourself as dirty and needing to be recalculated
      state |= NS_FRAME_HAS_DIRTY_CHILDREN;
      frame->SetFrameState(state);
      NeedsRecalc();

      nsIBox* parent = nsnull;
      GetParentBox(&parent);
      if (parent)
         return parent->RelayoutDirtyChild(aState, this);
      else {
        nsIFrame* parent = nsnull;
        frame->GetParent(&parent);
        nsCOMPtr<nsIPresShell> shell;
        aState.GetPresShell(getter_AddRefs(shell));
        return parent->ReflowDirtyChild(shell, frame);
      }
    } else {
#ifdef DEBUG_COELESCED
      Coelesced();
#endif
    }

    return NS_OK;
}

NS_IMETHODIMP
nsBox::GetVAlign(Valignment& aAlign)
{
  aAlign = vAlign_Top;
   return NS_OK;
}

NS_IMETHODIMP
nsBox::GetHAlign(Halignment& aAlign)
{
  aAlign = hAlign_Left;
   return NS_OK;
}

NS_IMETHODIMP
nsBox::GetClientRect(nsRect& aClientRect)
{
  GetContentRect(aClientRect);

  nsMargin borderPadding;
  GetBorderAndPadding(borderPadding);

  aClientRect.Deflate(borderPadding);

  nsMargin insets;
  GetInset(insets);

  aClientRect.Deflate(insets);
  if (aClientRect.width < 0)
     aClientRect.width = 0;

  if (aClientRect.height < 0)
     aClientRect.height = 0;

 // NS_ASSERTION(aClientRect.width >=0 && aClientRect.height >= 0, "Content Size < 0");

  return NS_OK;
}

NS_IMETHODIMP
nsBox::GetContentRect(nsRect& aContentRect)
{
  GetBounds(aContentRect);
  aContentRect.x = 0;
  aContentRect.y = 0;
  NS_BOX_ASSERTION(this, aContentRect.width >=0 && aContentRect.height >= 0, "Content Size < 0");
  return NS_OK;
}

NS_IMETHODIMP
nsBox::GetBounds(nsRect& aRect)
{
   nsIFrame* frame = nsnull;
   GetFrame(&frame);

   return frame->GetRect(aRect);
}

NS_IMETHODIMP
nsBox::SetBounds(nsBoxLayoutState& aState, const nsRect& aRect)
{
    NS_BOX_ASSERTION(this, aRect.width >=0 && aRect.height >= 0, "SetBounds Size < 0");

    nsRect rect(0,0,0,0);
    GetBounds(rect);

    nsIFrame* frame = nsnull;
    GetFrame(&frame);

    nsIPresContext* presContext = aState.GetPresContext();

    PRUint32 flags = 0;
    GetLayoutFlags(flags);

    PRUint32 stateFlags = 0;
    aState.GetLayoutFlags(stateFlags);

    flags |= stateFlags;

    if (flags & NS_FRAME_NO_MOVE_FRAME)
      frame->SizeTo(presContext, aRect.width, aRect.height);
    else
      frame->SetRect(presContext, aRect);

    
    if (!(flags & NS_FRAME_NO_MOVE_VIEW) || !(flags & NS_FRAME_NO_MOVE_CHILD_VIEWS)) 
    { 
      nsIView*  view;
      frame->GetView(presContext, &view);
      if (view) {
        if (!(flags & NS_FRAME_NO_MOVE_VIEW)) {
          nsContainerFrame::PositionFrameView(presContext, frame, view);
        }
      } else {
        if (!(flags & NS_FRAME_NO_MOVE_CHILD_VIEWS)) {
          // only if the origin changed
          if ((rect.x != aRect.x) || (rect.y != aRect.y))  {
             nsContainerFrame::PositionChildViews(presContext, frame);
          }
        }
      }
    }
  

   /*  
    // only if the origin changed
    if ((rect.x != aRect.x) || (rect.y != aRect.y))  {
      nsIView*  view;
      frame->GetView(presContext, &view);
      if (view) {
          nsContainerFrame::PositionFrameView(presContext, frame, view);
      } else {
          nsContainerFrame::PositionChildViews(presContext, frame);
      }
    }
    */
    

    return NS_OK;
}

void
nsBox::GetLayoutFlags(PRUint32& aFlags)
{
  aFlags = 0;
}


NS_IMETHODIMP
nsBox::GetBorderAndPadding(nsMargin& aBorderAndPadding)
{
  nsIFrame* frame = nsnull;
  GetFrame(&frame);

  const nsStyleSpacing* spacing;
  nsresult rv = frame->GetStyleData(eStyleStruct_Spacing,
    (const nsStyleStruct*&) spacing);
  
  if (NS_FAILED(rv))
    return rv;

  nsMargin border;
  nsMargin padding;
  GetBorder(border);
  GetPadding(padding);
  aBorderAndPadding.SizeTo(0,0,0,0);
  aBorderAndPadding += border;
  aBorderAndPadding += padding;

  return rv;
}

NS_IMETHODIMP
nsBox::GetBorder(nsMargin& aMargin)
{
  nsIFrame* frame = nsnull;
  GetFrame(&frame);

  const nsStyleSpacing* spacing;
  nsresult rv = frame->GetStyleData(eStyleStruct_Spacing,
        (const nsStyleStruct*&) spacing);

  if (NS_FAILED(rv))
    return rv;

  aMargin.SizeTo(0,0,0,0);
  spacing->GetBorder(aMargin);

  return rv;
}

NS_IMETHODIMP
nsBox::GetPadding(nsMargin& aMargin)
{
  nsIFrame* frame = nsnull;
  GetFrame(&frame);

  const nsStyleSpacing* spacing;
  nsresult rv = frame->GetStyleData(eStyleStruct_Spacing,
        (const nsStyleStruct*&) spacing);

 if (NS_FAILED(rv))
    return rv;

  aMargin.SizeTo(0,0,0,0);
  spacing->GetPadding(aMargin);

  return rv;
}

NS_IMETHODIMP
nsBox::GetMargin(nsMargin& aMargin)
{
  nsIFrame* frame = nsnull;
  GetFrame(&frame);

  const nsStyleSpacing* spacing;
        nsresult rv = frame->GetStyleData(eStyleStruct_Spacing,
        (const nsStyleStruct*&) spacing);

  if (NS_FAILED(rv))
     return rv;

  aMargin.SizeTo(0,0,0,0);
  spacing->GetMargin(aMargin);

  return rv;
}

NS_IMETHODIMP 
nsBox::GetChildBox(nsIBox** aBox)
{
    *aBox = nsnull;
    return NS_OK;
}

NS_IMETHODIMP
nsBox::GetNextBox(nsIBox** aBox)
{
  *aBox = mNextChild;
  return NS_OK;
}

NS_IMETHODIMP
nsBox::SetNextBox(nsIBox* aBox)
{
  mNextChild = aBox;
  return NS_OK;
}

NS_IMETHODIMP
nsBox::SetParentBox(nsIBox* aParent)
{
  mParentBox = aParent;
  return NS_OK;
}

NS_IMETHODIMP
nsBox::GetParentBox(nsIBox** aParent)
{
  *aParent = mParentBox;
  return NS_OK;
}

NS_IMETHODIMP
nsBox::NeedsRecalc()
{
  return NS_OK;
}

nsresult
nsBox::SetDebug(nsBoxLayoutState& aState, PRBool aDebug)
{
    return NS_OK;
}


void
nsBox::SizeNeedsRecalc(nsSize& aSize)
{
  aSize.width  = -1;
  aSize.height = -1;
}

void
nsBox::CoordNeedsRecalc(PRInt32& aFlex)
{
  aFlex = -1;
}

PRBool
nsBox::DoesNeedRecalc(const nsSize& aSize)
{
  return (aSize.width == -1 || aSize.height == -1);
}

PRBool
nsBox::DoesNeedRecalc(nscoord aCoord)
{
  return (aCoord == -1);
}

PRBool
nsBox::GetWasCollapsed(nsBoxLayoutState& aState)
{
  nsIFrame* frame = nsnull;
  GetFrame(&frame);
  nsFrameState state;
  frame->GetFrameState(&state);
 
  return (state & NS_STATE_IS_COLLAPSED);
}

void
nsBox::SetWasCollapsed(nsBoxLayoutState& aState, PRBool aCollapsed)
{
  nsIFrame* frame;
  GetFrame(&frame);
  nsFrameState state;
  frame->GetFrameState(&state);
  if (aCollapsed)
     state |= NS_STATE_IS_COLLAPSED;
  else
     state &= ~NS_STATE_IS_COLLAPSED;

  frame->SetFrameState(state);
}

NS_IMETHODIMP 
nsBox::Collapse(nsBoxLayoutState& aState)
{
 // if (GetWasCollapsed(aState))
 //   return NS_OK;

  SetWasCollapsed(aState, PR_TRUE);
  nsIFrame* frame;
  GetFrame(&frame);
  SetBounds(aState, nsRect(0,0,0,0));

  return CollapseChild(aState, frame, PR_TRUE);
}

nsresult 
nsBox::CollapseChild(nsBoxLayoutState& aState, nsIFrame* aFrame, PRBool aHide)
{
      nsIPresContext* presContext = aState.GetPresContext();

    // shrink the view
      nsIView* view = nsnull;
      aFrame->GetView(presContext, &view);

      // if we find a view stop right here. All views under it
      // will be clipped.
      if (view) {
         // already hidden? We are done.
         nsViewVisibility v;
         view->GetVisibility(v);
         //if (v == nsViewVisibility_kHide)
           //return NS_OK;

         nsCOMPtr<nsIWidget> widget;
         view->GetWidget(*getter_AddRefs(widget));
         if (aHide) {
             view->SetVisibility(nsViewVisibility_kHide);
         } else {
             view->SetVisibility(nsViewVisibility_kShow);
         }
         if (widget) {

           return NS_OK;
         }
      }

      // Trees have to collapse their scrollbars manually, since you can't
      // get to the scrollbar via the normal frame list.
      nsISelfScrollingFrame* treeFrame;
      if (NS_SUCCEEDED(aFrame->QueryInterface(NS_GET_IID(nsISelfScrollingFrame), (void**)&treeFrame)) && treeFrame) {
        // Tell the tree frame to collapse its scrollbar.
        treeFrame->CollapseScrollbar(presContext, aHide);
      }
      
      // collapse the child
      nsIFrame* child = nsnull;
      aFrame->FirstChild(presContext, nsnull,&child);
       
      while (nsnull != child) 
      {
         CollapseChild(aState, child, aHide);
         nsresult rv = child->GetNextSibling(&child);
         NS_ASSERTION(rv == NS_OK,"failed to get next child");
      }

      return NS_OK;
}

NS_IMETHODIMP 
nsBox::UnCollapse(nsBoxLayoutState& aState)
{
  if (!GetWasCollapsed(aState))
    return NS_OK;

  SetWasCollapsed(aState, PR_FALSE);

  return UnCollapseChild(aState, this);
}

nsresult 
nsBox::UnCollapseChild(nsBoxLayoutState& aState, nsIBox* aBox)
{
      nsIFrame* frame;
      aBox->GetFrame(&frame);
     
      // collapse the child
      nsIBox* child = nsnull;
      aBox->GetChildBox(&child);

      if (child == nsnull) {
          nsFrameState childState;
          frame->GetFrameState(&childState);
          childState |= NS_FRAME_IS_DIRTY;
          frame->SetFrameState(childState);
      } else {        
          child->GetFrame(&frame);
          nsFrameState childState;
          frame->GetFrameState(&childState);
          childState |= NS_FRAME_HAS_DIRTY_CHILDREN;
          frame->SetFrameState(childState);
       
          while (nsnull != child) 
          {
             UnCollapseChild(aState, child);
             nsresult rv = child->GetNextBox(&child);
             NS_ASSERTION(rv == NS_OK,"failed to get next child");
          }
      }

      return NS_OK;
}

NS_IMETHODIMP
nsBox::SetLayoutManager(nsIBoxLayout* aLayout)
{
  return NS_OK;
}

NS_IMETHODIMP
nsBox::GetLayoutManager(nsIBoxLayout** aLayout)
{
  *aLayout = nsnull;
  return NS_OK;
}


NS_IMETHODIMP
nsBox::GetPrefSize(nsBoxLayoutState& aState, nsSize& aSize)
{
  aSize.width = 0;
  aSize.height = 0;

  PRBool collapsed = PR_FALSE;
  IsCollapsed(aState, collapsed);
  if (collapsed)
    return NS_OK;

  AddBorderAndPadding(aSize);
  AddInset(aSize);
  nsIBox::AddCSSPrefSize(aState, this, aSize);
  return NS_OK;
}

NS_IMETHODIMP
nsBox::GetMinSize(nsBoxLayoutState& aState, nsSize& aSize)
{
  aSize.width = 0;
  aSize.height = 0;

  PRBool collapsed = PR_FALSE;
  IsCollapsed(aState, collapsed);
  if (collapsed)
    return NS_OK;

  AddBorderAndPadding(aSize);
  AddInset(aSize);
  nsIBox::AddCSSMinSize(aState, this, aSize);
  return NS_OK;
}

NS_IMETHODIMP
nsBox::GetMaxSize(nsBoxLayoutState& aState, nsSize& aSize)
{
  aSize.width = NS_INTRINSICSIZE;
  aSize.height = NS_INTRINSICSIZE;

  PRBool collapsed = PR_FALSE;
  IsCollapsed(aState, collapsed);
  if (collapsed)
    return NS_OK;

  AddBorderAndPadding(aSize);
  AddInset(aSize);
  nsIBox::AddCSSMaxSize(aState, this, aSize);
  return NS_OK;
}

NS_IMETHODIMP
nsBox::GetFlex(nsBoxLayoutState& aState, nscoord& aFlex)
{
  aFlex = 0;

  PRBool collapsed = PR_FALSE;
  IsCollapsed(aState, collapsed);
  if (collapsed)
    return NS_OK;

  GetDefaultFlex(aFlex);
  nsIBox::AddCSSFlex(aState, this, aFlex);

  return NS_OK;
}

NS_IMETHODIMP
nsBox::GetAscent(nsBoxLayoutState& aState, nscoord& aAscent)
{
  aAscent = 0;
  PRBool collapsed = PR_FALSE;
  IsCollapsed(aState, collapsed);
  if (collapsed)
    return NS_OK;

  nsSize size(0,0);
  nsresult rv = GetPrefSize(aState, size);
  aAscent = size.height;
  return rv;
}

NS_IMETHODIMP
nsBox::IsCollapsed(nsBoxLayoutState& aState, PRBool& aCollapsed)
{
  aCollapsed = PR_FALSE;
  nsIBox::AddCSSCollapsed(aState, this, aCollapsed);

  return NS_OK;
}

NS_IMETHODIMP
nsBox::Layout(nsBoxLayoutState& aState)
{
  BeginLayout(aState);

  DoLayout(aState);

  EndLayout(aState);

  return NS_OK;
}

NS_IMETHODIMP 
nsBox::GetOrientation(PRBool& aIsHorizontal)
{
   nsIFrame* frame = nsnull;
   GetFrame(&frame);
   nsFrameState state;
   frame->GetFrameState(&state);
   aIsHorizontal = (state & NS_STATE_IS_HORIZONTAL);
   return NS_OK;
}

nsresult
nsBox::SyncLayout(nsBoxLayoutState& aState)
{
  /*
  PRBool collapsed = PR_FALSE;
  IsCollapsed(aState, collapsed);
  if (collapsed) {
    CollapseChild(aState, this, PR_TRUE);
    return NS_OK;
  }
  */
  

  PRBool dirty = PR_FALSE;
  IsDirty(dirty);
  if (dirty || aState.GetLayoutReason() == nsBoxLayoutState::Initial)
     Redraw(aState);

  nsFrameState state;
  nsIFrame* frame;
  GetFrame(&frame);
  frame->GetFrameState(&state);
  state &= ~(NS_FRAME_HAS_DIRTY_CHILDREN | NS_FRAME_IS_DIRTY | NS_FRAME_FIRST_REFLOW | NS_FRAME_IN_REFLOW);
  frame->SetFrameState(state);

  nsIPresContext* presContext = aState.GetPresContext();
  nsRect rect(0,0,0,0);
  GetBounds(rect);

  PRUint32 flags = 0;
  GetLayoutFlags(flags);

  PRUint32 stateFlags = 0;
  aState.GetLayoutFlags(stateFlags);

  flags |= stateFlags;

  nsIView*  view;
  frame->GetView(presContext, &view);

/*
      // only if the origin changed
    if ((mX != rect.x) || (mY != rect.y)) {
        if (view) {
            nsContainerFrame::PositionFrameView(presContext, frame, view);
        } else
            nsContainerFrame::PositionChildViews(presContext, frame);

        mX = rect.x;
        mY = rect.y;
    }
*/

  if (view) {
    // Make sure the frame's view is properly sized and positioned and has
    // things like opacity correct
    nsHTMLContainerFrame::SyncFrameViewAfterReflow(
                             presContext, 
                             frame, 
                             view,
                             nsnull,
                             flags);

  } 


  return NS_OK;
}

NS_IMETHODIMP
nsBox::Redraw(nsBoxLayoutState& aState,
              const nsRect*   aDamageRect,
              PRBool          aImmediate)
{
  if (aState.GetDisablePainting())
    return NS_OK;

  nsIPresContext* presContext = aState.GetPresContext();
  const nsHTMLReflowState* s = aState.GetReflowState();
  if (s) {
    if (s->reason != eReflowReason_Incremental)
      return NS_OK;
  }

  nsIFrame* frame = nsnull;
  GetFrame(&frame);

  nsCOMPtr<nsIViewManager> viewManager;
  nsRect damageRect(0,0,0,0);
  if (aDamageRect)
    damageRect = *aDamageRect;
  else 
    GetContentRect(damageRect);

  // Checks to see if the damaged rect should be infalted 
  // to include the outline
  const nsStyleSpacing* spacing;
  frame->GetStyleData(eStyleStruct_Spacing, (const nsStyleStruct*&)spacing);
  nscoord width;
  spacing->GetOutlineWidth(width);
  if (width > 0) {
    damageRect.Inflate(width, width);
  }

  PRUint32 flags = aImmediate ? NS_VMREFRESH_IMMEDIATE : NS_VMREFRESH_NO_SYNC;
  nsIView* view;

  frame->GetView(presContext, &view);
  if (view) {
    view->GetViewManager(*getter_AddRefs(viewManager));
    viewManager->UpdateView(view, damageRect, flags);
    
  } else {
    nsRect    rect(damageRect);
    nsPoint   offset;
  
    frame->GetOffsetFromView(presContext, offset, &view);
    NS_BOX_ASSERTION(this, nsnull != view, "no view");
    rect += offset;
    view->GetViewManager(*getter_AddRefs(viewManager));
    viewManager->UpdateView(view, rect, flags);
  }

  return NS_OK;
}

PRBool 
nsIBox::AddCSSPrefSize(nsBoxLayoutState& aState, nsIBox* aBox, nsSize& aSize)
{
    PRBool widthSet = PR_FALSE;
    PRBool heightSet = PR_FALSE;
    nsIFrame* frame = nsnull;
    aBox->GetFrame(&frame);

    // add in the css min, max, pref
    const nsStylePosition* position;
    frame->GetStyleData(eStyleStruct_Position,
                  (const nsStyleStruct*&) position);

    // see if the width or height was specifically set
    if (position->mWidth.GetUnit() == eStyleUnit_Coord)  {
        aSize.width = position->mWidth.GetCoordValue();
        widthSet = PR_TRUE;
    }

    if (position->mHeight.GetUnit() == eStyleUnit_Coord) {
        aSize.height = position->mHeight.GetCoordValue();     
        heightSet = PR_TRUE;
    }
    
    nsCOMPtr<nsIContent> content;
    frame->GetContent(getter_AddRefs(content));

    if (content) {
        nsIPresContext* presContext = aState.GetPresContext();

        nsAutoString value;
        PRInt32 error;

        if (NS_CONTENT_ATTR_HAS_VALUE == content->GetAttribute(kNameSpaceID_None, nsHTMLAtoms::width, value))
        {
            float p2t;
            presContext->GetScaledPixelsToTwips(&p2t);

            value.Trim("%");

            aSize.width = NSIntPixelsToTwips(value.ToInteger(&error), p2t);
            widthSet = PR_TRUE;
        }

        if (NS_CONTENT_ATTR_HAS_VALUE == content->GetAttribute(kNameSpaceID_None, nsHTMLAtoms::height, value))
        {
            float p2t;
            presContext->GetScaledPixelsToTwips(&p2t);

            value.Trim("%");

            aSize.height = NSIntPixelsToTwips(value.ToInteger(&error), p2t);
            heightSet = PR_TRUE;
        }
    }

    return (widthSet && heightSet);
}


PRBool 
nsIBox::AddCSSMinSize(nsBoxLayoutState& aState, nsIBox* aBox, nsSize& aSize)
{

    PRBool widthSet = PR_FALSE;
    PRBool heightSet = PR_FALSE;

    nsIFrame* frame = nsnull;
    aBox->GetFrame(&frame);

    // add in the css min, max, pref
    const nsStylePosition* position;
    frame->GetStyleData(eStyleStruct_Position,
                  (const nsStyleStruct*&) position);

    // same for min size. Unfortunately min size is always set to 0. So for now
    // we will assume 0 means not set.
    if (position->mMinWidth.GetUnit() == eStyleUnit_Coord) {
        nscoord min = position->mMinWidth.GetCoordValue();
        if (min != 0) {
           aSize.width = min;
           widthSet = PR_TRUE;
        }
    }

    if (position->mMinHeight.GetUnit() == eStyleUnit_Coord) {
        nscoord min = position->mMinHeight.GetCoordValue();
        if (min != 0) {
           aSize.height = min;
           heightSet = PR_TRUE;
        }
    }

    return (widthSet && heightSet);
}

PRBool 
nsIBox::AddCSSMaxSize(nsBoxLayoutState& aState, nsIBox* aBox, nsSize& aSize)
{  

    PRBool widthSet = PR_FALSE;
    PRBool heightSet = PR_FALSE;

    nsIFrame* frame = nsnull;
    aBox->GetFrame(&frame);

    // add in the css min, max, pref
    const nsStylePosition* position;
    frame->GetStyleData(eStyleStruct_Position,
                  (const nsStyleStruct*&) position);

    // and max
    if (position->mMaxWidth.GetUnit() == eStyleUnit_Coord) {
        nscoord max = position->mMaxWidth.GetCoordValue();
        aSize.width = max;
        widthSet = PR_TRUE;
    }

    if (position->mMaxHeight.GetUnit() == eStyleUnit_Coord) {
        nscoord max = position->mMaxHeight.GetCoordValue();
        aSize.height = max;
        heightSet = PR_TRUE;
    }


    return NS_OK;
}

PRBool 
nsIBox::AddCSSFlex(nsBoxLayoutState& aState, nsIBox* aBox, nscoord& aFlex)
{
    PRBool flexSet = PR_FALSE;

    nsIFrame* frame = nsnull;
    aBox->GetFrame(&frame);

    // get the flexibility
    nsCOMPtr<nsIContent> content;
    frame->GetContent(getter_AddRefs(content));

    if (content) {
        PRInt32 error;
        nsAutoString value;

        if (NS_CONTENT_ATTR_HAS_VALUE == content->GetAttribute(kNameSpaceID_None, nsXULAtoms::flex, value))
        {
            value.Trim("%");
            aFlex = value.ToInteger(&error);
            flexSet = PR_TRUE;
        }
    }

    return flexSet;
}

PRBool 
nsIBox::AddCSSCollapsed(nsBoxLayoutState& aState, nsIBox* aBox, PRBool& aCollapsed)
{
  nsIFrame* frame = nsnull;
  aBox->GetFrame(&frame);
  const nsStyleDisplay* disp;
  frame->GetStyleData(eStyleStruct_Display, ((const nsStyleStruct *&)disp));
  aCollapsed = disp->mVisible == NS_STYLE_VISIBILITY_COLLAPSE;
  return PR_TRUE;
}

void
nsBox::AddBorderAndPadding(nsSize& aSize)
{
  AddBorderAndPadding(this, aSize);
}

void
nsBox::AddMargin(nsSize& aSize)
{
  AddMargin(this, aSize);
}

void
nsBox::AddInset(nsSize& aSize)
{
  AddInset(this, aSize);
}

void
nsBox::AddBorderAndPadding(nsIBox* aBox, nsSize& aSize)
{
  nsMargin borderPadding(0,0,0,0);
  aBox->GetBorderAndPadding(borderPadding);
  AddMargin(aSize, borderPadding);
}

void
nsBox::AddMargin(nsIBox* aChild, nsSize& aSize)
{
  nsMargin margin(0,0,0,0);
  aChild->GetMargin(margin);
  AddMargin(aSize, margin);
}

void
nsBox::AddMargin(nsSize& aSize, const nsMargin& aMargin)
{
  if (aSize.width != NS_INTRINSICSIZE)
    aSize.width += aMargin.left + aMargin.right;

  if (aSize.height != NS_INTRINSICSIZE)
     aSize.height += aMargin.top + aMargin.bottom;
}

void
nsBox::AddInset(nsIBox* aBox, nsSize& aSize)
{
  nsMargin margin(0,0,0,0);
  aBox->GetInset(margin);
  AddMargin(aSize, margin);
}

void
nsBox::BoundsCheck(nsSize& aMinSize, nsSize& aPrefSize, nsSize& aMaxSize)
{
  
   if (aMinSize.width > aMaxSize.width)
       aMinSize.width = aMaxSize.width;

   if (aMinSize.height > aMaxSize.height)
       aMinSize.height = aMaxSize.height;


/*
   if (aMinSize.width > aMaxSize.width)
       aMaxSize.width = aMinSize.width;

   if (aMinSize.height > aMaxSize.height)
       aMaxSize.height = aMinSize.height;
*/

   if (aPrefSize.width > aMaxSize.width)
       aPrefSize.width = aMaxSize.width;

   if (aPrefSize.height > aMaxSize.height)
       aPrefSize.height = aMaxSize.height;

   if (aPrefSize.width < aMinSize.width)
       aPrefSize.width = aMinSize.width;

   if (aPrefSize.height < aMinSize.height)
       aPrefSize.height = aMinSize.height;
}

NS_IMETHODIMP
nsBox::GetDebugBoxAt( const nsPoint& aPoint,
                      nsIBox**     aBox)
{
  nsPoint tmp;

  nsRect rect;
  GetBounds(rect);

  if (!rect.Contains(aPoint))
    return NS_ERROR_FAILURE;

  nsIBox* child = nsnull;
  nsIBox* hit = nsnull;
  GetChildBox(&child);

  *aBox = nsnull;
  tmp.MoveTo(aPoint.x - rect.x, aPoint.y - rect.y);
  while (nsnull != child) {
    nsresult rv = child->GetDebugBoxAt(tmp, &hit);

    if (NS_SUCCEEDED(rv) && hit) {
      *aBox = hit;
    }
    child->GetNextBox(&child);
  }

  // found a child
  if (*aBox) {
    return NS_OK;
  }

 // see if it is in our in our insets
  nsMargin m;
  GetBorderAndPadding(m);
  rect.Deflate(m);
  if (rect.Contains(aPoint)) {
    GetInset(m);
    rect.Deflate(m);
    if (!rect.Contains(aPoint)) {
      *aBox = this;
      return NS_OK;
    }
  }

  return NS_ERROR_FAILURE;
}


NS_IMETHODIMP
nsBox::GetDebug(PRBool& aDebug)
{
  aDebug = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP
nsBox::GetMouseThrough(PRBool& aMouseThrough)
{
  switch(mMouseThrough)
  {
    case always:
      aMouseThrough = PR_TRUE;
      return NS_OK;
    case never:
      aMouseThrough = PR_FALSE;      
      return NS_OK;
    case unset:
    {
      nsIBox* parent = nsnull;
      GetParentBox(&parent);
      if (parent)
        return parent->GetMouseThrough(aMouseThrough);
      else {
        aMouseThrough = PR_FALSE;      
        return NS_OK;
      }
    }
  }

  return NS_ERROR_FAILURE;
}

PRBool
nsBox::GetDefaultFlex(PRInt32& aFlex) 
{ 
  aFlex = 0; 
  return PR_TRUE; 
}


// nsISupports
NS_IMETHODIMP_(nsrefcnt) 
nsBox::AddRef(void)
{
  return NS_OK;
}

NS_IMETHODIMP_(nsrefcnt)
nsBox::Release(void)
{
    return NS_OK;
}

//
// QueryInterface
//
NS_INTERFACE_MAP_BEGIN(nsBox)
  NS_INTERFACE_MAP_ENTRY(nsIBox)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

