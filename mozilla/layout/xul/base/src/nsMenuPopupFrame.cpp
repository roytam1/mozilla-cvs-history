/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: NPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is 
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 * Original Author: David W. Hyatt (hyatt@netscape.com)
 *   Mike Pinkerton (pinkerton@netscape.com)
 *   Dean Tessman <dean_tessman@hotmail.com>
 *   Ben Goodger <ben@netscape.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or 
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the NPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */


#include "nsMenuPopupFrame.h"
#include "nsXULAtoms.h"
#include "nsHTMLAtoms.h"
#include "nsIContent.h"
#include "prtypes.h"
#include "nsIAtom.h"
#include "nsIPresContext.h"
#include "nsStyleContext.h"
#include "nsCSSRendering.h"
#include "nsINameSpaceManager.h"
#include "nsIViewManager.h"
#include "nsWidgetsCID.h"
#include "nsMenuFrame.h"
#include "nsIPopupSetFrame.h"
#include "nsIDOMWindowInternal.h"
#include "nsIDOMScreen.h"
#include "nsIScriptGlobalObject.h"
#include "nsIPresShell.h"
#include "nsIDocument.h"
#include "nsIDeviceContext.h"
#include "nsRect.h"
#include "nsIDOMXULDocument.h"
#include "nsILookAndFeel.h"
#include "nsIComponentManager.h"
#include "nsBoxLayoutState.h"
#include "nsIScrollableView.h"
#include "nsIFrameManager.h"
#include "nsGUIEvent.h"
#include "nsIRootBox.h"
#include "nsIDocShellTreeItem.h"
#include "nsITimerInternal.h"
#include "nsUnicharUtils.h"
#ifdef XP_WIN
#include "nsISound.h"
#endif

static NS_DEFINE_IID(kLookAndFeelCID, NS_LOOKANDFEEL_CID);

const PRInt32 kMaxZ = 0x7fffffff; //XXX: Shouldn't there be a define somewhere for MaxInt for PRInt32


static nsIPopupSetFrame*
GetPopupSetFrame(nsIPresContext* aPresContext)
{
  nsCOMPtr<nsIPresShell> shell;
  aPresContext->GetShell(getter_AddRefs(shell));
  nsIFrame* rootFrame;
  shell->GetRootFrame(&rootFrame);
  if (!rootFrame)
    return nsnull;

  if (rootFrame)
    rootFrame->FirstChild(aPresContext, nsnull, &rootFrame);   
 
  nsCOMPtr<nsIRootBox> rootBox(do_QueryInterface(rootFrame));
  if (!rootBox)
    return NS_OK;

  nsIFrame* popupSetFrame;
  rootBox->GetPopupSetFrame(&popupSetFrame);
  if (!popupSetFrame)
    return nsnull;

  nsCOMPtr<nsIPopupSetFrame> popupSet(do_QueryInterface(popupSetFrame));
  return popupSet;
}


// NS_NewMenuPopupFrame
//
// Wrapper for creating a new menu popup container
//
nsresult
NS_NewMenuPopupFrame(nsIPresShell* aPresShell, nsIFrame** aNewFrame)
{
  NS_PRECONDITION(aNewFrame, "null OUT ptr");
  if (nsnull == aNewFrame) {
    return NS_ERROR_NULL_POINTER;
  }
  nsMenuPopupFrame* it = new (aPresShell) nsMenuPopupFrame (aPresShell);
  if ( !it )
    return NS_ERROR_OUT_OF_MEMORY;
  *aNewFrame = it;
  return NS_OK;
}

NS_IMETHODIMP_(nsrefcnt) 
nsMenuPopupFrame::AddRef(void)
{
  return NS_OK;
}

NS_IMETHODIMP_(nsrefcnt) 
nsMenuPopupFrame::Release(void)
{
    return NS_OK;
}


//
// QueryInterface
//
NS_INTERFACE_MAP_BEGIN(nsMenuPopupFrame)
  NS_INTERFACE_MAP_ENTRY(nsIMenuParent)
  NS_INTERFACE_MAP_ENTRY(nsITimerCallback)
NS_INTERFACE_MAP_END_INHERITING(nsBoxFrame)


//
// nsMenuPopupFrame ctor
//
nsMenuPopupFrame::nsMenuPopupFrame(nsIPresShell* aShell)
  :nsBoxFrame(aShell), mCurrentMenu(nsnull), mTimerMenu(nsnull), mCloseTimer(nsnull),
    mMenuCanOverlapOSBar(PR_FALSE), mShouldAutoPosition(PR_TRUE), mShouldRollup(PR_TRUE)
{
  SetIsContextMenu(PR_FALSE);   // we're not a context menu by default
  // Don't allow container frames to automatically position
  // the popup because they will put it in the wrong position.
  nsFrameState state;
  GetFrameState(&state);
  state &= ~NS_FRAME_SYNC_FRAME_AND_VIEW;
  SetFrameState(state);
} // ctor


NS_IMETHODIMP
nsMenuPopupFrame::Init(nsIPresContext*  aPresContext,
                       nsIContent*      aContent,
                       nsIFrame*        aParent,
                       nsStyleContext*  aContext,
                       nsIFrame*        aPrevInFlow)
{
  nsresult rv = nsBoxFrame::Init(aPresContext, aContent, aParent, aContext, aPrevInFlow);

  // lookup if we're allowed to overlap the OS bar (menubar/taskbar) from the
  // look&feel object
  nsCOMPtr<nsILookAndFeel> lookAndFeel;
  aPresContext->GetLookAndFeel(getter_AddRefs(lookAndFeel));
  if ( lookAndFeel ) {
    PRBool tempBool;
    lookAndFeel->GetMetric(nsILookAndFeel::eMetric_MenusCanOverlapOSBar, tempBool);
    mMenuCanOverlapOSBar = tempBool;
  }
  
  // XXX Hack
  mPresContext = aPresContext;

  CreateViewForFrame(aPresContext, this, aContext, PR_TRUE);

  // Now that we've made a view, remove it and insert it at the correct
  // position in the view hierarchy (as the root view).  We do this so that we
  // can draw the menus outside the confines of the window.
  nsIView* ourView;
  GetView(aPresContext, &ourView);

  nsCOMPtr<nsIViewManager> viewManager;
  ourView->GetViewManager(*getter_AddRefs(viewManager));

  // Remove the view from its old position.
  viewManager->RemoveChild(ourView);

  // Reinsert ourselves as the root view with a maximum z-index.
  nsIView* rootView;
  viewManager->GetRootView(rootView);
  viewManager->SetViewZIndex(ourView, PR_FALSE, kMaxZ);
  viewManager->InsertChild(rootView, ourView, nsnull, PR_TRUE);

  // XXX Hack. The menu's view should float above all other views,
  // so we use the nsIView::SetFloating() to tell the view manager
  // about that constraint.
  viewManager->SetViewFloating(ourView, PR_TRUE);

  // XXX Hack. Change our transparency to be non-transparent
  // until the bug related to update of transparency on show/hide
  // is fixed.
  viewManager->SetViewContentTransparency(ourView, PR_FALSE);

  // Create a widget for ourselves.
  nsWidgetInitData widgetData;
  widgetData.mWindowType = eWindowType_popup;
  widgetData.mBorderStyle = eBorderStyle_default;
  widgetData.clipSiblings = PR_TRUE;

  nsCOMPtr<nsIContent> parentContent;
  aContent->GetParent(*getter_AddRefs(parentContent));
  nsCOMPtr<nsIAtom> tag;
  if (parentContent)
    parentContent->GetTag(*getter_AddRefs(tag));
  widgetData.mDropShadow = !(tag && tag == nsXULAtoms::menulist);
  
  // XXX make sure we are hidden (shouldn't this be done automatically?)
  viewManager->SetViewVisibility(ourView, nsViewVisibility_kHide);
#if defined(XP_MAC) || defined(XP_MACOSX)
#ifdef DEBUG
  printf("XP Popups: This is a nag to indicate that an inconsistent hack is being done on the Mac for popups.\n");
#endif  
  static NS_DEFINE_IID(kCPopupCID,  NS_POPUP_CID);
  ourView->CreateWidget(kCPopupCID, &widgetData, nsnull, PR_TRUE, PR_TRUE, 
                        eContentTypeUI);
#else
  static NS_DEFINE_IID(kCChildCID,  NS_CHILD_CID);
  ourView->CreateWidget(kCChildCID, &widgetData, nsnull, PR_TRUE, PR_TRUE, 
                        eContentTypeUI);
#endif   

  MoveToAttributePosition();

  return rv;
}

NS_IMETHODIMP
nsMenuPopupFrame::MarkStyleChange(nsBoxLayoutState& aState)
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
  nsIMenuFrame* menuFrame = nsnull;
  CallQueryInterface(parent, &menuFrame);

  if (menuFrame)
     return parent->RelayoutDirtyChild(aState, this);
  else {
    nsIPopupSetFrame* popupSet = GetPopupSetFrame(mPresContext);
    nsIBox *box;
    if (popupSet && NS_SUCCEEDED(CallQueryInterface(popupSet, &box))) {
      nsBoxLayoutState state(mPresContext);
      box->MarkDirtyChildren(state); // Mark the popupset as dirty.
    }
    else {
      nsIFrame* frame = nsnull;
      GetFrame(&frame);
      nsIFrame* parentFrame = nsnull;
      frame->GetParent(&parentFrame);
      nsCOMPtr<nsIPresShell> shell;
      aState.GetPresShell(getter_AddRefs(shell));
      return parentFrame->ReflowDirtyChild(shell, frame);
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsMenuPopupFrame::MarkDirty(nsBoxLayoutState& aState)
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
  nsIMenuFrame* menuFrame = nsnull;
  CallQueryInterface(parent, &menuFrame);

  if (menuFrame)
     return parent->RelayoutDirtyChild(aState, this);
  else {
    nsIPopupSetFrame* popupSet = GetPopupSetFrame(mPresContext);
    nsIBox *box;
    if (popupSet && NS_SUCCEEDED(CallQueryInterface(popupSet, &box))) {
      nsBoxLayoutState state(mPresContext);
      box->MarkDirtyChildren(state); // Mark the popupset as dirty.
    }
    else {
      nsIFrame* parentFrame = nsnull;
      frame->GetParent(&parentFrame);
      nsCOMPtr<nsIPresShell> shell;
      aState.GetPresShell(getter_AddRefs(shell));
      return parentFrame->ReflowDirtyChild(shell, frame);
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsMenuPopupFrame::RelayoutDirtyChild(nsBoxLayoutState& aState, nsIBox* aChild)
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

    nsIBox* parentBox = nsnull;
    GetParentBox(&parentBox);
    nsIMenuFrame* menuFrame = nsnull;
    CallQueryInterface(parentBox, &menuFrame);

    if (menuFrame)
      return parentBox->RelayoutDirtyChild(aState, this);
    else {
      nsIPopupSetFrame* popupSet = GetPopupSetFrame(mPresContext);
      nsIBox *box;
      if (popupSet && NS_SUCCEEDED(CallQueryInterface(popupSet, &box))) {
        nsBoxLayoutState state(mPresContext);
        box->MarkDirtyChildren(state); // Mark the popupset as dirty.
      }
      else 
        return nsBox::RelayoutDirtyChild(aState, aChild);
    }
  }

  return NS_OK;
}

void
nsMenuPopupFrame::GetLayoutFlags(PRUint32& aFlags)
{
  aFlags = NS_FRAME_NO_SIZE_VIEW | NS_FRAME_NO_MOVE_VIEW | NS_FRAME_NO_VISIBILITY;
}

PRBool ParentIsScrollableView(nsIView* aStartView);
PRBool ParentIsScrollableView(nsIView* aStartView)
{
  nsIView* scrollportView = nsnull;
  nsIScrollableView* scrollableView = nsnull;
  aStartView->GetParent(scrollportView);
  if (scrollportView)
    scrollportView->QueryInterface(NS_GET_IID(nsIScrollableView), (void**) &scrollableView);
  return scrollableView != nsnull;
}

///////////////////////////////////////////////////////////////////////////////
// GetViewOffset
//   Retrieves the offset of the given view with the root view, in the 
//   coordinate system of the root view. 
void
nsMenuPopupFrame::GetViewOffset(nsIView* aView, nsPoint& aPoint)
{
  // Notes:
  //   1) The root view is the client area of the toplevel window that
  //      this popup is anchored to. 
  //   2) Each menupopup is a child of the root view (see 
  //      nsMenuPopupFrame::Init())
  //   3) The coordinates that we return are the total distance between 
  //      the top left of the start view and the origin of the root view.
  //      Note that for extremely tall menus there can be negative bounds
  //      as the menupopup may fall north of the client area (e.g. above
  //      the titlebar). We must take this into account for correct positioning,
  //      however, negative bounds due to views that are the canvas in a 
  //      ScrollPortView must be ignored (as this has no bearing on 
  //      view offset), hence the call to ParentIsScrollableView. 
  //      
  
  aPoint.x = 0;
  aPoint.y = 0;
 
  // Keep track of the root view so that we know to stop there
  nsCOMPtr<nsIViewManager> vm;
  aView->GetViewManager(*getter_AddRefs(vm));
  nsIView* rootView;
  vm->GetRootView(rootView);

  nsIView *parent;
  nsRect bounds;

  parent = aView;
  while (parent) {
    parent->GetBounds(bounds);
    if ((bounds.y >= 0 && bounds.x >= 0) || !ParentIsScrollableView(parent)) {
      //
      // The Extremely Tall Menu: 
      //           +----+---------------------------------------
      //           |    |             +--------------------+  -       -
      //           |    |             |    (Decoration)    |  | <-(4) | <-(3)
      // (0, 0) -> +----+-------------+                    |  +       |
      //           | _File    _Edit   +--------------------+  |       +
      //           +------------------|  (ScrollPortView)  |  |       |
      //           |                  |                    |  |       |
      //           |                  |                    |  | <-(1) | <-(2)
      //           |                  |                    |  |       |
      //           |<---------------->|                    |  |       |
      //           |        (5)       |====================|  -       -
      //           |                  | MenuFrame         >|
      //           |                  |====================|
      // 
      // Typically, we want to ignore negative view bounds as these imply
      // a canvas view inside a scrollport view. However in other cases this
      // means the view falls outside the positive quadrant of the root view,
      // and has negative y-axis bounds. We still want to add this negative
      // bounds as when positioning the menu, the following calculation can
      // be performed:
      // 
      //   (1) = (2) + (3) + (4) 
      //
      // (1) - the position on the y-axis in the coordinate system of the root
      //       view at which to position the popup
      // (2) - the offset of the invoking MenuFrame from the top of the scroll-
      //       port view (adjusted for canvas area scrolled out of view)
      // (3) - the bounds of the scrollport view with respect to the popup's
      //       view (at this point, aPoint.y)
      // (4) - the bounds of the popup's view with respect to the root view
      //       (a negative value for extremely tall popups)
      //  
      // (5) - the position on the x-axis in the coordinate system of the root
      //       view at which to position the popup.
  
      aPoint.y += bounds.y;
      aPoint.x += bounds.x;
    }
    if (parent == rootView)
      break;
    parent->GetParent(parent);
  }
}

///////////////////////////////////////////////////////////////////////////////
// GetRootViewForPopup
//   Retrieves the view for the popup widget that contains the given frame. 
//   If the given frame is not contained by a popup widget, return the
//   root view.  This is the root view of the pres context's
//   viewmanager if aStopAtViewManagerRoot is true; otherwise it's the
//   root view of the root viewmanager.
void
nsMenuPopupFrame::GetRootViewForPopup(nsIPresContext* aPresContext, 
                                      nsIFrame* aStartFrame,
                                      PRBool aStopAtViewManagerRoot,
                                      nsIView** aResult)
{
  *aResult = nsnull;

  // A frame with a view.
  nsIFrame* parentWithView = nsnull;

  nsFrameState fs;
  aStartFrame->GetFrameState(&fs);
  if (fs & NS_FRAME_HAS_VIEW) {
    // If the given frame has a view, we don't need to climb anywhere.
    parentWithView = aStartFrame;
  }
  else {
    // Otherwise, walk up the frame tree looking for the first parent which
    // has a view.
    aStartFrame->GetParentWithView(aPresContext, &parentWithView);
  }

  if (parentWithView) {
    nsIView* view = nsnull;
    nsIView* temp = nsnull;
    parentWithView->GetView(aPresContext, &view);
    NS_ASSERTION(view, "GetParentWithView returned frame with no view!");
    nsIView* rootView = nsnull;
    if (aStopAtViewManagerRoot) {
      nsCOMPtr<nsIViewManager> viewManager;
      view->GetViewManager(*getter_AddRefs(viewManager));
      NS_ASSERTION(viewManager, "View must have a viewmanager");
      viewManager->GetRootView(rootView);
    }
    
    while (view) {
      // Walk up the view hierachy looking for a view whose widget has a 
      // window type of eWindowType_popup - in other words a popup window
      // widget. If we find one, this is the view we want. 
      nsCOMPtr<nsIWidget> widget;
      view->GetWidget(*getter_AddRefs(widget));
      if (widget) {
        nsWindowType wtype;
        widget->GetWindowType(wtype);
        if (wtype == eWindowType_popup) {
          *aResult = view;
          return;
        }
      }

      if (aStopAtViewManagerRoot && view == rootView) {
        *aResult = view;
        return;
      }

      view->GetParent(temp);
      if (!temp) {
        // Otherwise, we've walked all the way up to the root view and not
        // found a view for a popup window widget. Just return the root view.
        *aResult = view;
      }
      view = temp;
    }
  }
}


void GetWidgetForView(nsIView *aView, nsIWidget *&aWidget);
void GetWidgetForView(nsIView *aView, nsIWidget *&aWidget)
{
  aWidget = nsnull;
  nsIView *view = aView;
  while (!aWidget && view)
  {
    view->GetWidget(aWidget);
    if (!aWidget)
      view->GetParent(view);
  }
}


//
// AdjustClientXYForNestedDocuments
// 
// almost certainly, the document where the mouse was clicked is not
// the document that contains the popup, especially if we're viewing a page
// with frames. Thus we need to make adjustments to the client coordinates to
// take this into account and get them back into the relative coordinates of
// this document.
//
void
nsMenuPopupFrame::AdjustClientXYForNestedDocuments ( nsIDOMXULDocument* inPopupDoc, nsIPresShell* inPopupShell, 
                                                         PRInt32 inClientX, PRInt32 inClientY, 
                                                         PRInt32* outAdjX, PRInt32* outAdjY )
{
  if ( !inPopupDoc || !outAdjX || !outAdjY )
    return;

  // Find the widget associated with the popup's document
  nsCOMPtr<nsIWidget> popupDocumentWidget;
  nsCOMPtr<nsIViewManager> viewManager;
  inPopupShell->GetViewManager(getter_AddRefs(viewManager));
  if ( viewManager ) {  
    nsIView* rootView;
    viewManager->GetRootView(rootView);
    nscoord wOffsetX, wOffsetY;
    if ( rootView )
      rootView->GetOffsetFromWidget(&wOffsetX, &wOffsetY, *getter_AddRefs(popupDocumentWidget));
  }
  NS_WARN_IF_FALSE(popupDocumentWidget, "ACK, BAD WIDGET");
  
  // Find the widget associated with the target's document.
  // For tooltips, we check the document's tooltipNode (which is set by
  // nsXULTooltipListener).  For regular popups, use popupNode (set by
  // nsXULPopupListener).

  nsCOMPtr<nsIAtom> tag;
  mContent->GetTag(*getter_AddRefs(tag));
  nsCOMPtr<nsIDOMNode> targetNode;
  if (tag == nsXULAtoms::tooltip)
    inPopupDoc->GetTooltipNode(getter_AddRefs(targetNode));
  else
    inPopupDoc->GetPopupNode(getter_AddRefs(targetNode));

  //NS_WARN_IF_FALSE(targetNode, "no popup/tooltip node on document!");
  nsCOMPtr<nsIContent> targetAsContent ( do_QueryInterface(targetNode) );
  nsCOMPtr<nsIWidget> targetDocumentWidget;
  if ( targetAsContent ) {
    nsCOMPtr<nsIDocument> targetDocument;
    targetAsContent->GetDocument(*getter_AddRefs(targetDocument));
    if (targetDocument) {
      nsCOMPtr<nsIPresShell> shell;
      targetDocument->GetShellAt(0, getter_AddRefs(shell));
      nsCOMPtr<nsIViewManager> viewManagerTarget;
      if ( shell ) {
        // We might be inside a popup widget. If so, we need to use that widget and
        // not the root view's widget.
        nsIFrame* targetFrame;
        shell->GetPrimaryFrameFor(targetAsContent, &targetFrame);
        nsIView* parentView = nsnull;
        if (targetFrame) {
          nsCOMPtr<nsIPresContext> targetContext;
          shell->GetPresContext(getter_AddRefs(targetContext));
          if (targetContext) {
            GetRootViewForPopup(targetContext, targetFrame, PR_TRUE, &parentView);
            GetWidgetForView(parentView, *getter_AddRefs(targetDocumentWidget));
          }
        }
        if (!targetDocumentWidget) {
          // We aren't inside a popup. This means we should use the root view's
          // widget.
          shell->GetViewManager(getter_AddRefs(viewManagerTarget));
          if ( viewManagerTarget ) {
            nsIView* rootViewTarget;
            viewManagerTarget->GetRootView(rootViewTarget);
            if ( rootViewTarget ) {
              nscoord unusedX, unusedY;
              rootViewTarget->GetOffsetFromWidget(&unusedX, &unusedY, *getter_AddRefs(targetDocumentWidget));
            }
          }
        }
      }
    }
  }
  //NS_WARN_IF_FALSE(targetDocumentWidget, "ACK, BAD TARGET");

  // the offset we need is the difference between the upper left corner of the two widgets. Use
  // screen coordinates to find the global offset between them.
  nsRect popupDocTopLeft;
  if ( popupDocumentWidget ) {
    nsRect topLeftClient ( 0, 0, 10, 10 );
    popupDocumentWidget->WidgetToScreen ( topLeftClient, popupDocTopLeft );
  }
  nsRect targetDocTopLeft;
  if ( targetDocumentWidget ) {
    nsRect topLeftClient ( 0, 0, 10, 10 );
    targetDocumentWidget->WidgetToScreen ( topLeftClient, targetDocTopLeft );
  }
  nsPoint pixelOffset ( targetDocTopLeft.x - popupDocTopLeft.x, targetDocTopLeft.y - popupDocTopLeft.y );

  *outAdjX = inClientX + pixelOffset.x;
  *outAdjY = inClientY + pixelOffset.y;
  
} // AdjustClientXYForNestedDocuments


//
// AdjustPositionForAnchorAlign
// 
// Uses the |popupanchor| and |popupalign| attributes on the popup to move the popup around and
// anchor it to its parent. |outFlushWithTopBottom| will be TRUE if the popup is flush with either
// the top or bottom edge of its parent, and FALSE if it is flush with the left or right edge of
// the parent.
// 
void
nsMenuPopupFrame::AdjustPositionForAnchorAlign ( PRInt32* ioXPos, PRInt32* ioYPos, const nsRect & inParentRect,
                                                    const nsString& aPopupAnchor, const nsString& aPopupAlign,
                                                    PRBool* outFlushWithTopBottom )
{
  nsAutoString popupAnchor(aPopupAnchor);
  nsAutoString popupAlign(aPopupAlign);

  if (GetStyleVisibility()->mDirection == NS_STYLE_DIRECTION_RTL) {
    if (popupAnchor == NS_LITERAL_STRING("topright"))
      popupAnchor.Assign(NS_LITERAL_STRING("topleft"));
    else if (popupAnchor == NS_LITERAL_STRING("topleft"))
      popupAnchor.Assign(NS_LITERAL_STRING("topright"));
    else if (popupAnchor == NS_LITERAL_STRING("bottomleft"))
      popupAnchor.Assign(NS_LITERAL_STRING("bottomright"));
    else if (popupAnchor == NS_LITERAL_STRING("bottomright"))
      popupAnchor.Assign(NS_LITERAL_STRING("bottomleft"));

    if (popupAlign == NS_LITERAL_STRING("topright"))
      popupAlign.Assign(NS_LITERAL_STRING("topleft"));
    else if (popupAlign == NS_LITERAL_STRING("topleft"))
      popupAlign.Assign(NS_LITERAL_STRING("topright"));
    else if (popupAlign == NS_LITERAL_STRING("bottomleft"))
      popupAlign.Assign(NS_LITERAL_STRING("bottomright"));
    else if (popupAnchor == NS_LITERAL_STRING("bottomright"))
      popupAlign.Assign(NS_LITERAL_STRING("bottomleft"));
  }

  // Adjust position for margins at the aligned corner
  nsMargin margin;
  GetStyleMargin()->GetMargin(margin);
  if (popupAlign == NS_LITERAL_STRING("topleft")) {
    *ioXPos += margin.left;
    *ioYPos += margin.top;
  } else if (popupAlign == NS_LITERAL_STRING("topright")) {
    *ioXPos += margin.right;
    *ioYPos += margin.top;
  } else if (popupAlign == NS_LITERAL_STRING("bottomleft")) {
    *ioXPos += margin.left;
    *ioYPos += margin.bottom;
  } else if (popupAlign == NS_LITERAL_STRING("bottomright")) {
    *ioXPos += margin.right;
    *ioYPos += margin.bottom;
  }
  
  if (popupAnchor == NS_LITERAL_STRING("topright") && popupAlign == NS_LITERAL_STRING("topleft")) {
    *ioXPos += inParentRect.width;
  }
  else if (popupAnchor == NS_LITERAL_STRING("topleft") && popupAlign == NS_LITERAL_STRING("topleft")) {
    *outFlushWithTopBottom = PR_TRUE;
  }
  else if (popupAnchor == NS_LITERAL_STRING("topright") && popupAlign == NS_LITERAL_STRING("bottomright")) {
    *ioXPos -= (mRect.width - inParentRect.width);
    *ioYPos -= mRect.height;
    *outFlushWithTopBottom = PR_TRUE;
  }
  else if (popupAnchor == NS_LITERAL_STRING("bottomright") && popupAlign == NS_LITERAL_STRING("bottomleft")) {
    *ioXPos += inParentRect.width;
    *ioYPos -= (mRect.height - inParentRect.height);
  }
  else if (popupAnchor == NS_LITERAL_STRING("bottomright") && popupAlign == NS_LITERAL_STRING("topright")) {
    *ioXPos -= (mRect.width - inParentRect.width);
    *ioYPos += inParentRect.height;
    *outFlushWithTopBottom = PR_TRUE;
  }
  else if (popupAnchor == NS_LITERAL_STRING("topleft") && popupAlign == NS_LITERAL_STRING("topright")) {
    *ioXPos -= mRect.width;
  }
  else if (popupAnchor == NS_LITERAL_STRING("topleft") && popupAlign == NS_LITERAL_STRING("bottomleft")) {
    *ioYPos -= mRect.height;
    *outFlushWithTopBottom = PR_TRUE;
  }
  else if (popupAnchor == NS_LITERAL_STRING("bottomleft") && popupAlign == NS_LITERAL_STRING("bottomright")) {
    *ioXPos -= mRect.width;
    *ioYPos -= (mRect.height - inParentRect.height);
  }
  else if (popupAnchor == NS_LITERAL_STRING("bottomleft") && popupAlign == NS_LITERAL_STRING("topleft")) {
    *ioYPos += inParentRect.height;
    *outFlushWithTopBottom = PR_TRUE;
  }
  else
    NS_WARNING ( "Hmmm, looks like you've hit a anchor/align case we weren't setup for." );

} // AdjustPositionForAnchorAlign


//
// IsMoreRoomOnOtherSideOfParent
//
// Determine if there is more room on the screen for the popup to live if it was positioned
// on the flip side of the parent from the side it is flush against (ie, if it's top edge was
// flush against the bottom, is there more room if its bottom edge were flush against the top)
//
PRBool
nsMenuPopupFrame::IsMoreRoomOnOtherSideOfParent ( PRBool inFlushAboveBelow, PRInt32 inScreenViewLocX, PRInt32 inScreenViewLocY,
                                                     const nsRect & inScreenParentFrameRect, PRInt32 inScreenTopTwips, PRInt32 inScreenLeftTwips,
                                                     PRInt32 inScreenBottomTwips, PRInt32 inScreenRightTwips )
{
  PRBool switchSides = PR_FALSE;
  if ( inFlushAboveBelow ) {
    PRInt32 availAbove = inScreenParentFrameRect.y - inScreenTopTwips;
    PRInt32 availBelow = inScreenBottomTwips - (inScreenParentFrameRect.y + inScreenParentFrameRect.height) ;
    if ( inScreenViewLocY > inScreenParentFrameRect.y )       // view is now below parent
      switchSides = availAbove > availBelow;
    else
      switchSides = availBelow > availAbove;
  }
  else {
    PRInt32 availLeft = inScreenParentFrameRect.x - inScreenLeftTwips;
    PRInt32 availRight = inScreenRightTwips - (inScreenParentFrameRect.x + inScreenParentFrameRect.width) ;
    if ( inScreenViewLocX > inScreenParentFrameRect.x )       // view is now to the right of parent
      switchSides = availLeft > availRight;
    else
      switchSides = availRight > availLeft;           
  }

  return switchSides;
  
} // IsMoreRoomOnOtherSideOfParent


//
// MovePopupToOtherSideOfParent
//
// Move the popup to the other side of the parent (ie, if it the popup's top edge is flush against the
// bottom of its parent, move the popup so that its bottom edge is now flush against the top of its
// parent...same idea for left/right).
//
// NOTE: In moving the popup, it may need to change size in order to stay on the screen. This will
//       have the side effect of touching |mRect|.
//
void
nsMenuPopupFrame::MovePopupToOtherSideOfParent ( PRBool inFlushAboveBelow, PRInt32* ioXPos, PRInt32* ioYPos, 
                                                 PRInt32* ioScreenViewLocX, PRInt32* ioScreenViewLocY,
                                                 const nsRect & inScreenParentFrameRect, PRInt32 inScreenTopTwips, PRInt32 inScreenLeftTwips,
                                                 PRInt32 inScreenBottomTwips, PRInt32 inScreenRightTwips )
{
  if ( inFlushAboveBelow ) {
    if ( *ioScreenViewLocY > inScreenParentFrameRect.y ) {     // view is currently below parent
      // move it above.
      PRInt32 shiftDistY = inScreenParentFrameRect.height + mRect.height;
      *ioYPos -= shiftDistY;
      *ioScreenViewLocY -= shiftDistY;
      // trim it to fit.
      if ( *ioScreenViewLocY < inScreenTopTwips ) {
        PRInt32 trimY = inScreenTopTwips - *ioScreenViewLocY;
        *ioYPos += trimY;
        *ioScreenViewLocY += trimY;
        mRect.height -= trimY;
      }
    }
    else {                                               // view is currently above parent
      // move it below
      PRInt32 shiftDistY = inScreenParentFrameRect.height + mRect.height;
      *ioYPos += shiftDistY;
      *ioScreenViewLocY += shiftDistY;
    }
  }
  else {
    if ( *ioScreenViewLocX > inScreenParentFrameRect.x ) {     // view is currently to the right of the parent
      // move it to the left.
      PRInt32 shiftDistX = inScreenParentFrameRect.width + mRect.width;
      *ioXPos -= shiftDistX;
      *ioScreenViewLocX -= shiftDistX;
      // trim it to fit.
      if ( *ioScreenViewLocX < inScreenLeftTwips ) {
        PRInt32 trimX = inScreenLeftTwips - *ioScreenViewLocX;
        *ioXPos += trimX;
        *ioScreenViewLocX += trimX;
        mRect.width -= trimX;
      }
    }
    else {                                               // view is currently to the right of the parent
      // move it to the right
      PRInt32 shiftDistX = inScreenParentFrameRect.width + mRect.width;
      *ioXPos += shiftDistX;
      *ioScreenViewLocX += shiftDistX;
    }               
  }

} // MovePopupToOtherSideOfParent



nsresult 
nsMenuPopupFrame::SyncViewWithFrame(nsIPresContext* aPresContext,
                                    const nsString& aPopupAnchor,
                                    const nsString& aPopupAlign,
                                    nsIFrame* aFrame, 
                                    PRInt32 aXPos, PRInt32 aYPos)
{
  NS_ENSURE_ARG(aPresContext);
  NS_ENSURE_ARG(aFrame);

  if (!mShouldAutoPosition) 
    return NS_OK;

  // |containingView|
  //   The view that contains the frame that is invoking this popup. This is 
  //   the canvas view inside the scrollport view. It can have negative bounds
  //   if the canvas is scrolled so that part is off screen.
  nsIView* containingView = nsnull;
  nsPoint offset;
  nsMargin margin;
  aFrame->GetOffsetFromView(aPresContext, offset, &containingView);
  if (!containingView)
    return NS_OK;

  // |view|
  //   The root view for the popup window widget associated with this frame,
  //   or, the view associated with this frame. 
  nsIView* view = nsnull;
  GetView(aPresContext, &view);

  ///////////////////////////////////////////////////////////////////////////////
  //
  //  (0,-y)    +- - - - - - - - - - - - - - - - - - - - - -+  _            _
  //            |   (part of canvas scrolled off the top)   |  |  bounds.y  |
  //  (0, 0) -> +-------------------------------------------+  +            |
  //            |                                           |  |            | offset
  //            |  (part of canvas visible through parent   |  |  dY        |
  //            |   nsIScrollableView (nsScrollPortView) )  |  |            |
  //            |                                           |  |            |
  //            |===========================================|  -            -
  //            | aFrame                                  > |
  //            |===========================================|
  //            |                                           |
  //            +-------------------------------------------+
  //            |                                           |
  //            +- - - - - - - - - - - - - - - - - - - - - -+
  //
  // Explanation: 
  //
  // If the frame we're trying to align this popup to is on a canvas inside
  // a scrolled viewport (that is, the containing view of the frame is
  // the child of a scrolling view, nsIScrollableView) the offset y 
  // dimension of that view contains matter that is not onscreen (scrolled 
  // up out of view) and must be adjusted when positioning the popup. 
  // The y dimension on the bounds of the containing view is negative if 
  // any content is offscreen, and the size of this dimension represents 
  // the amount we must adjust the offset by. For most submenus this is 0, and
  // so the offset is unchanged. For toplevel menus whose containing view is 
  // a window or other view, whose bounds should not be taken into account. 
  //
  if (ParentIsScrollableView(containingView)) {
    nsRect bounds;
    containingView->GetBounds(bounds);
    offset += nsPoint(bounds.x, bounds.y);
  }
  
  // |parentPos|
  //   The distance between the containingView and the root view. This provides
  //   a hint as to where to position the menu relative to the window. 
  nsPoint parentPos;
  GetViewOffset(containingView, parentPos);

  // |parentRect|
  //   The dimensions of the frame invoking the popup. 
  nsRect parentRect;
  aFrame->GetRect(parentRect);

  float p2t, t2p;
  aPresContext->GetScaledPixelsToTwips(&p2t);

  nsCOMPtr<nsIViewManager> viewManager;
  containingView->GetViewManager(*getter_AddRefs(viewManager));
    
  nsCOMPtr<nsIDeviceContext> dx;
  viewManager->GetDeviceContext(*getter_AddRefs(dx));
  dx->GetAppUnitsToDevUnits(t2p);

  // get the document and the global script object
  nsCOMPtr<nsIPresShell> presShell;
  aPresContext->GetShell(getter_AddRefs(presShell));
  nsCOMPtr<nsIDocument> document;
  presShell->GetDocument(getter_AddRefs(document));
  nsCOMPtr<nsIScriptGlobalObject> scriptGlobalObject;
  document->GetScriptGlobalObject(getter_AddRefs(scriptGlobalObject));
  
  nsCOMPtr<nsIContent> parentContent;
  aFrame->GetContent(getter_AddRefs(parentContent));
  nsCOMPtr<nsIAtom> tag;
  mContent->GetTag(*getter_AddRefs(tag));
  PRBool sizedToPopup = (tag != nsXULAtoms::tooltip)
                        && (nsMenuFrame::IsSizedToPopup(parentContent, PR_FALSE));
  
  // If we stick to our parent's width, set it here before we move the
  // window around, because moving is done with respect to the width...
  if (sizedToPopup) {
    mRect.width = parentRect.width;
  }

  // |xpos| and |ypos| hold the x and y positions of where the popup will be moved to,
  // in _twips_, in the coordinate system of the _parent view_.
  PRInt32 xpos = 0, ypos = 0;

  // if we are anchored to our parent, there are certain things we don't want to do
  // when repositioning the view to fit on the screen, such as end up positioned over
  // the parent. When doing this reposition, we want to move the popup to the side with
  // the most room. The combination of anchor and alignment dictate if we readjst 
  // above/below or to the left/right.
  PRBool anchoredToParent = PR_FALSE;
  PRBool readjustAboveBelow = PR_FALSE;
  
  if ( aXPos != -1 || aYPos != -1 ) {
  
    // for this case, we've been handed a specific x/y location (in client coordinates) for
    // the popup. However, we may be deeply nested in a frameset, etc and so the client coordinates
    // need some adjusting. 
    nsCOMPtr<nsIDOMXULDocument> xulDoc ( do_QueryInterface(document) );
    PRInt32 newXPos = 0, newYPos = 0;
    AdjustClientXYForNestedDocuments ( xulDoc, presShell, aXPos, aYPos, &newXPos, &newYPos );

    xpos = NSIntPixelsToTwips(newXPos, p2t);
    ypos = NSIntPixelsToTwips(newYPos, p2t);

    // Add in the top and left margins
    GetStyleMargin()->GetMargin(margin);
    
    xpos += margin.left;
    ypos += margin.top;
  } 
  else {
    anchoredToParent = PR_TRUE;

    xpos = parentPos.x + offset.x;
    ypos = parentPos.y + offset.y;
    
    // move the popup according to the anchor/alignment attributes. This will also tell us
    // which axis the popup is flush against in case we have to move it around later.
    AdjustPositionForAnchorAlign ( &xpos, &ypos, parentRect, aPopupAnchor, aPopupAlign, &readjustAboveBelow );    
  }
  
  // Compute info about the screen dimensions. Because of multiple monitor systems,
  // the left or top sides of the screen may be in negative space (main monitor is on the
  // right, etc). We need to be sure to do the right thing.
  nsCOMPtr<nsIDOMWindowInternal> window(do_QueryInterface(scriptGlobalObject));
  nsCOMPtr<nsIDOMScreen> screen;
  window->GetScreen(getter_AddRefs(screen));
  PRInt32 screenWidth = 0, screenHeight = 0;
  PRInt32 screenLeft = 0, screenTop = 0;
  PRInt32 screenRight = 0, screenBottom = 0;
  if ( mMenuCanOverlapOSBar ) {
    screen->GetLeft(&screenLeft);
    screen->GetTop(&screenTop);
    screen->GetWidth(&screenWidth);
    screen->GetHeight(&screenHeight);
  }
  else {
    screen->GetAvailLeft(&screenLeft);
    screen->GetAvailTop(&screenTop); 
    screen->GetAvailWidth(&screenWidth);
    screen->GetAvailHeight(&screenHeight);
  }
  screenRight = screenLeft + screenWidth;
  screenBottom = screenTop + screenHeight;
  
   // inset the screen by 5px so that menus don't butt up against the side
  const PRInt32 kTrimMargin = 5;
  screenLeft += kTrimMargin;
  screenTop += kTrimMargin;
  screenRight -= kTrimMargin;
  screenBottom -= kTrimMargin;
  screenWidth -= 2 * kTrimMargin;
  screenHeight -= 2 * kTrimMargin;

  PRInt32 screenTopTwips    = NSIntPixelsToTwips(screenTop, p2t);
  PRInt32 screenLeftTwips   = NSIntPixelsToTwips(screenLeft, p2t);
  PRInt32 screenWidthTwips  = NSIntPixelsToTwips(screenWidth, p2t);
  PRInt32 screenHeightTwips = NSIntPixelsToTwips(screenHeight, p2t);
  PRInt32 screenRightTwips  = NSIntPixelsToTwips(screenRight, p2t);
  PRInt32 screenBottomTwips = NSIntPixelsToTwips(screenBottom, p2t);
  
  // Recall that |xpos| and |ypos| are in the coordinate system of the parent view. In
  // order to determine the screen coordinates of where our view will end up, we
  // need to find the x/y position of the parent view in screen coords. That is done
  // by getting the widget associated with the parent view and determining the offset 
  // based on converting (0,0) in its coordinate space to screen coords. We then
  // offset that point by (|xpos|,|ypos|) to get the true screen coordinates of
  // the view. *whew*

  // |parentView|
  //   The root view for the window that contains the frame, for frames inside 
  //   menupopups this is the first view inside the popup window widget, for 
  //   frames inside a toplevel window, this is the root view of the toplevel
  //   window.
  nsIView* parentView = nsnull;
  GetRootViewForPopup(aPresContext, aFrame, PR_FALSE, &parentView);
  if (!parentView)
    return NS_OK;

  // Use containingView instead of parentView, to account for the scrollarrows
  // that a parent menu might have.

  nsCOMPtr<nsIWidget> parentViewWidget;
  GetWidgetForView ( containingView, *getter_AddRefs(parentViewWidget) );
  nsRect localParentWidgetRect(0,0,0,0), screenParentWidgetRect;
  parentViewWidget->WidgetToScreen ( localParentWidgetRect, screenParentWidgetRect );
  PRInt32 screenViewLocX = NSIntPixelsToTwips(screenParentWidgetRect.x,p2t) + (xpos - parentPos.x);
  PRInt32 screenViewLocY = NSIntPixelsToTwips(screenParentWidgetRect.y,p2t) + (ypos - parentPos.y);

  if ( anchoredToParent ) {
    
    //
    // Popup is anchored to the parent, guarantee that it does not cover the parent. We
    // shouldn't do anything funky if it will already fit on the screen as is.
    //

    ///////////////////////////////////////////////////////////////////////////////
    //
    //                +------------------------+          
    //                |           /\           |
    // parentPos -> - +------------------------+
    //              | |                        |
    //       offset | |                        |
    //              | |                        |
    //              | |                        | (screenViewLocX,screenViewLocY)
    //              - |========================|+--------------
    //                | parentRect           > ||
    //                |========================||
    //                |                        || Submenu 
    //                +------------------------+|  ( = mRect )
    //                |           \/           ||
    //                +------------------------+



    // compute screen coordinates of parent frame so we can play with it. Make sure we put it
    // into twips as everything else is as well.
    nsRect screenParentFrameRect ( NSTwipsToIntPixels(offset.x,t2p), NSTwipsToIntPixels(offset.y,t2p),
                                    parentRect.width, parentRect.height );
    parentViewWidget->WidgetToScreen ( screenParentFrameRect, screenParentFrameRect );
    screenParentFrameRect.x = NSIntPixelsToTwips(screenParentFrameRect.x, p2t);
    screenParentFrameRect.y = NSIntPixelsToTwips(screenParentFrameRect.y, p2t);

    // Don't let it spill off the screen to the top
    if (screenViewLocY < screenTopTwips) {
      PRInt32 moveDist = screenTopTwips - screenViewLocY;
      screenViewLocY = screenTopTwips;
      ypos += moveDist;
    }
    
    // if it doesn't fit on the screen, do our magic.
    if ( (screenViewLocX + mRect.width) > screenRightTwips ||
           screenViewLocX < screenLeftTwips ||
          (screenViewLocY + mRect.height) > screenBottomTwips ) {
      
      // figure out which side of the parent has the most free space so we can move/resize
      // the popup there. This should still work if the parent frame is partially screen.
      PRBool switchSides = IsMoreRoomOnOtherSideOfParent ( readjustAboveBelow, screenViewLocX, screenViewLocY,
                                                            screenParentFrameRect, screenTopTwips, screenLeftTwips,
                                                            screenBottomTwips, screenRightTwips );
      
      // move the popup to the correct side, if necessary. Note that MovePopupToOtherSideOfParent() 
      // can change width/height of |mRect|.
      if ( switchSides )
        MovePopupToOtherSideOfParent ( readjustAboveBelow, &xpos, &ypos, &screenViewLocX, &screenViewLocY, 
                                        screenParentFrameRect, screenTopTwips, screenLeftTwips,
                                        screenBottomTwips, screenRightTwips );
                                        
      // We are allowed to move the popup along the axis to which we're not anchored to the parent
      // in order to get it to not spill off the screen.
      if ( readjustAboveBelow ) {
        // move left to be on screen, but don't let it go off the screen at the left
        if ( (screenViewLocX + mRect.width) > screenRightTwips ) {
          PRInt32 moveDistX = (screenViewLocX + mRect.width) - screenRightTwips;
          if ( screenViewLocX - moveDistX < screenLeftTwips )
            moveDistX = screenViewLocX - screenLeftTwips;          
          screenViewLocX -= moveDistX;
          xpos -= moveDistX;
        } else if (screenViewLocX < screenLeftTwips) {
          // move right to be on screen, but don't let it go off the screen at the right
          PRInt32 moveDistX = screenLeftTwips - screenViewLocX;
          if ( (screenViewLocX + mRect.width + moveDistX) > screenRightTwips )
            moveDistX = screenRightTwips - screenViewLocX - mRect.width;
          screenViewLocX += moveDistX;
          xpos += moveDistX;
        }
      }
      else {
        // move it up to be on screen, but don't let it go off the screen at the top

        //  |
        //  |
        //  |+----  screenViewLocY
        //  ||
        //  ||  Submenu ( = mRect )
        // -+|
        //   |
        //   |
        // - - - - - - - - - - screenBottomTwips (bottom of the screen)
        //   |    \ 
        //   |     }  moveDistY
        //   |    /
        //   +----  screenViewLocY + mRect.height
        //

        if ( (screenViewLocY + mRect.height) > screenBottomTwips ) {
          // XXX Bug 84121 comment 48 says the next line has to use screenHeightTwips, why not screenBottomTwips?
          PRInt32 moveDistY = (screenViewLocY + mRect.height) - screenHeightTwips;
          if ( screenViewLocY - moveDistY < screenTopTwips )
            moveDistY = screenViewLocY - screenTopTwips;          
          screenViewLocY -= moveDistY;
          ypos -= moveDistY; 
        } 
      }
      
      // Resize it to fit on the screen. By this point, we've given the popup as much
      // room as we can w/out covering the parent. If it still can't be as big
      // as it wants to be, well, it just has to suck up and deal. 
      //
      // ySpillage is calculated the same way as moveDistY above. see picture there.

      PRInt32 xSpillage = (screenViewLocX + mRect.width) - screenRightTwips;
      if ( xSpillage > 0 )
        mRect.width -= xSpillage;
      PRInt32 ySpillage = (screenViewLocY + mRect.height) - screenBottomTwips;
      if ( ySpillage > 0 )
        mRect.height -= ySpillage;

      // shrink to fit onto the screen, vertically and horizontally
      if(mRect.width > screenWidthTwips) 
          mRect.width = screenWidthTwips;    
      if(mRect.height > screenHeightTwips)
          mRect.height = screenHeightTwips;   

    } // if it doesn't fit on screen
  } // if anchored to parent
  else {
  
    //
    // Popup not anchored to anything, just make sure it's on the screen by any
    // means necessary
    //

    // shrink to fit onto the screen, vertically and horizontally
    if(mRect.width > screenWidthTwips) 
        mRect.width = screenWidthTwips;    
    if(mRect.height > screenHeightTwips)
        mRect.height = screenHeightTwips;   
    
    // we now know where the view is...make sure that it's still onscreen at all!
    if ( screenViewLocX < screenLeftTwips ) {
      PRInt32 moveDistX = screenLeftTwips - screenViewLocX;
      xpos += moveDistX;
      screenViewLocX += moveDistX;
    }
    if ( screenViewLocY < screenTopTwips ) {
      PRInt32 moveDistY = screenTopTwips - screenViewLocY;
      ypos += moveDistY;
      screenViewLocY += moveDistY;
    }

    // ensure it is not even partially offscreen.
    if ( (screenViewLocX + mRect.width) > screenRightTwips ) {
      if (tag == nsXULAtoms::tooltip) {
        //the tooltip is off the screen to the right - shift it to the left as much as needed
        xpos -= (screenViewLocX + mRect.width) - screenRightTwips;
      }
      else        
        xpos -= mRect.width;      
    }
    if ( (screenViewLocY + mRect.height) > screenBottomTwips )
      ypos -= (mRect.height + margin.top + margin.bottom);
      
  }  

  viewManager->MoveViewTo(view, xpos, ypos); 

  // Now that we've positioned the view, sync up the frame's origin.
  nsPoint frameOrigin, offsetToView;
  GetOrigin(frameOrigin);
  GetOriginToViewOffset(aPresContext, offsetToView, nsnull);
  frameOrigin -= offsetToView;
  nsBoxFrame::MoveTo(aPresContext, frameOrigin.x, frameOrigin.y);

  if (sizedToPopup) {
      nsBoxLayoutState state(mPresContext);
      SetBounds(state, nsRect(mRect.x, mRect.y, parentRect.width, mRect.height));
  }
    
  nsAutoString shouldDisplay, menuActive;
  mContent->GetAttr(kNameSpaceID_None, nsXULAtoms::menuactive, menuActive);
  if (menuActive != NS_LITERAL_STRING("true")) {
    mContent->GetAttr(kNameSpaceID_None, nsXULAtoms::menutobedisplayed, shouldDisplay);
    if ( shouldDisplay == NS_LITERAL_STRING("true") )
      mContent->SetAttr(kNameSpaceID_None, nsXULAtoms::menuactive, NS_LITERAL_STRING("true"), PR_TRUE);
  }

  return NS_OK;
}

static void GetInsertionPoint(nsIPresShell* aShell, nsIFrame* aFrame, nsIFrame* aChild,
                              nsIFrame** aResult)
{
  nsCOMPtr<nsIStyleSet> styleSet;
  aShell->GetStyleSet(getter_AddRefs(styleSet));
  nsCOMPtr<nsIContent> child;
  if (aChild)
    aChild->GetContent(getter_AddRefs(child));
  styleSet->GetInsertionPoint(aShell, aFrame, child, aResult);
}

NS_IMETHODIMP
nsMenuPopupFrame::GetNextMenuItem(nsIMenuFrame* aStart, nsIMenuFrame** aResult)
{
  nsIFrame* immediateParent = nsnull;
  nsCOMPtr<nsIPresShell> shell;
  mPresContext->GetShell(getter_AddRefs(shell));
  GetInsertionPoint(shell, this, nsnull, &immediateParent);
  if (!immediateParent)
    immediateParent = this;

  nsIFrame* currFrame = nsnull;
  nsIFrame* startFrame = nsnull;
  if (aStart) {
    aStart->QueryInterface(NS_GET_IID(nsIFrame), (void**)&currFrame); 
    if (currFrame) {
      startFrame = currFrame;
      currFrame->GetNextSibling(&currFrame);
    }
  }
  else 
    immediateParent->FirstChild(mPresContext,
                                nsnull,
                                &currFrame);

  
  while (currFrame) {
    nsCOMPtr<nsIContent> current;
    currFrame->GetContent(getter_AddRefs(current));

    // See if it's a menu item.
    if (IsValidItem(current)) {
      nsCOMPtr<nsIMenuFrame> menuFrame = do_QueryInterface(currFrame);
      *aResult = menuFrame.get();
      NS_IF_ADDREF(*aResult);
      return NS_OK;
    }
    currFrame->GetNextSibling(&currFrame);
  }

  immediateParent->FirstChild(mPresContext,
                              nsnull,
                              &currFrame);

  // Still don't have anything. Try cycling from the beginning.
  while (currFrame && currFrame != startFrame) {
    nsCOMPtr<nsIContent> current;
    currFrame->GetContent(getter_AddRefs(current));
    
    // See if it's a menu item.
    if (IsValidItem(current)) {
      nsCOMPtr<nsIMenuFrame> menuFrame = do_QueryInterface(currFrame);
      *aResult = menuFrame.get();
      NS_IF_ADDREF(*aResult);
      return NS_OK;
    }

    currFrame->GetNextSibling(&currFrame);
  }

  // No luck. Just return our start value.
  *aResult = aStart;
  return NS_OK;
}

NS_IMETHODIMP
nsMenuPopupFrame::GetPreviousMenuItem(nsIMenuFrame* aStart, nsIMenuFrame** aResult)
{
  nsIFrame* immediateParent = nsnull;
  nsCOMPtr<nsIPresShell> shell;
  mPresContext->GetShell(getter_AddRefs(shell));
  GetInsertionPoint(shell, this, nsnull, &immediateParent);
  if (!immediateParent)
    immediateParent = this;

  nsIFrame* first;
  immediateParent->FirstChild(mPresContext,
                              nsnull, &first);
  nsFrameList frames(first);
  
                              
  nsIFrame* currFrame = nsnull;
  nsIFrame* startFrame = nsnull;
  if (aStart) {
    aStart->QueryInterface(NS_GET_IID(nsIFrame), (void**)&currFrame);
    if (currFrame) {
      startFrame = currFrame;
      currFrame = frames.GetPrevSiblingFor(currFrame);
    }
  }
  else currFrame = frames.LastChild();

  while (currFrame) {
    nsCOMPtr<nsIContent> current;
    currFrame->GetContent(getter_AddRefs(current));

    // See if it's a menu item.
    if (IsValidItem(current)) {
      nsCOMPtr<nsIMenuFrame> menuFrame = do_QueryInterface(currFrame);
      *aResult = menuFrame.get();
      NS_IF_ADDREF(*aResult);
      return NS_OK;
    }
    currFrame = frames.GetPrevSiblingFor(currFrame);
  }

  currFrame = frames.LastChild();

  // Still don't have anything. Try cycling from the end.
  while (currFrame && currFrame != startFrame) {
    nsCOMPtr<nsIContent> current;
    currFrame->GetContent(getter_AddRefs(current));
    
    // See if it's a menu item.
    if (IsValidItem(current)) {
      nsCOMPtr<nsIMenuFrame> menuFrame = do_QueryInterface(currFrame);
      *aResult = menuFrame.get();
      NS_IF_ADDREF(*aResult);
      return NS_OK;
    }

    currFrame = frames.GetPrevSiblingFor(currFrame);
  }

  // No luck. Just return our start value.
  *aResult = aStart;
  return NS_OK;
}

NS_IMETHODIMP nsMenuPopupFrame::GetCurrentMenuItem(nsIMenuFrame** aResult)
{
  *aResult = mCurrentMenu;
  NS_IF_ADDREF(*aResult);
  return NS_OK;
}

NS_IMETHODIMP nsMenuPopupFrame::ConsumeOutsideClicks(PRBool& aConsumeOutsideClicks)
{
  /*
   * When this popup is open, should clicks outside of it be consumed?
   * Return PR_TRUE if the popup hould rollup on an outside click, 
   * but consume that click so it can't be used for anything else.
   * Return PR_FALSE to allow clicks outside the popup to activate content 
   * even when the popup is open.
   * ---------------------------------------------------------------------
   * 
   * Should clicks outside of a popup be eaten?
   *
   *       Menus     Autocomplete     Comboboxes
   * Mac     Eat           No              Eat
   * Win     No            No              Eat     
   * Unix    Eat           No              Eat
   *
   */

  aConsumeOutsideClicks = PR_TRUE;

  nsCOMPtr<nsIContent> parentContent;
  mContent->GetParent(*getter_AddRefs(parentContent));

  if (parentContent) {
    nsCOMPtr<nsIAtom> parentTag;
    parentContent->GetTag(*getter_AddRefs(parentTag));
    if (parentTag == nsXULAtoms::menulist)
      return NS_OK;  // Consume outside clicks for combo boxes on all platforms
    if (parentTag == nsXULAtoms::menu || parentTag == nsXULAtoms::popupset) {
#ifdef XP_WIN
      // Don't consume outside clicks for menus in Windows
      aConsumeOutsideClicks = PR_FALSE;
#endif
      return NS_OK;
    }
    if (parentTag == nsXULAtoms::textbox) {
      // Don't consume outside clicks for autocomplete widget
      nsAutoString typeString;
      parentContent->GetAttr(kNameSpaceID_None, nsHTMLAtoms::type, typeString);
      if (typeString.EqualsIgnoreCase("autocomplete"))
        aConsumeOutsideClicks = PR_FALSE;
    }
  }

  return NS_OK;
}

nsIScrollableView* nsMenuPopupFrame::GetScrollableView(nsIFrame* aStart)
{
  if ( ! aStart )
    return nsnull;  

  nsIFrame* currFrame;
  nsIView* view=nsnull;
  nsIScrollableView* scrollableView=nsnull;

  // try start frame and siblings
  currFrame=aStart;
  do {
    currFrame->GetView(mPresContext, &view);
    if ( view )
      view->QueryInterface(NS_GET_IID(nsIScrollableView), (void**)&scrollableView);
    if ( scrollableView )
      return scrollableView;
    currFrame->GetNextSibling(&currFrame);
  } while ( currFrame );

  // try children
  nsIFrame* childFrame;
  currFrame=aStart;
  do {
    currFrame->FirstChild(mPresContext, nsnull, &childFrame);
    scrollableView=GetScrollableView(childFrame);
    if ( scrollableView )
      return scrollableView;
    currFrame->GetNextSibling(&currFrame);
  } while ( currFrame );

  return nsnull;
}

void nsMenuPopupFrame::EnsureMenuItemIsVisible(nsIMenuFrame* aMenuItem)
{
  nsIFrame* frame=nsnull;
  aMenuItem->QueryInterface(NS_GET_IID(nsIFrame), (void**)&frame);
  if ( frame ) {
    nsIFrame* childFrame=nsnull;
    FirstChild(mPresContext, nsnull, &childFrame);
    nsIScrollableView *scrollableView;
    scrollableView=GetScrollableView(childFrame);
    if ( scrollableView ) {
      nsIView* view=nsnull;
      scrollableView->QueryInterface(NS_GET_IID(nsIView), (void**)&view);
      if ( view ) {
        nsRect viewRect, itemRect;
        nscoord scrollX, scrollY;

        view->GetBounds(viewRect);
        frame->GetRect(itemRect);
        scrollableView->GetScrollPosition(scrollX, scrollY);
    
        // scroll down
        if ( itemRect.y + itemRect.height > scrollY + viewRect.height )
          scrollableView->ScrollTo(scrollX, itemRect.y + itemRect.height - viewRect.height, NS_SCROLL_PROPERTY_ALWAYS_BLIT);
        
        // scroll up
        else if ( itemRect.y < scrollY )
          scrollableView->ScrollTo(scrollX, itemRect.y, NS_SCROLL_PROPERTY_ALWAYS_BLIT);
      }
    }
  }
}

NS_IMETHODIMP nsMenuPopupFrame::SetCurrentMenuItem(nsIMenuFrame* aMenuItem)
{
  // When a context menu is open, the current menu is locked, and no change
  // to the menu is allowed.
  nsCOMPtr<nsIMenuParent> contextMenu;
  GetContextMenu(getter_AddRefs(contextMenu));
  if (contextMenu)
    return NS_OK;

  if (mCurrentMenu == aMenuItem)
    return NS_OK;
  
  // Unset the current child.
  if (mCurrentMenu) {
    PRBool isOpen = PR_FALSE;
    mCurrentMenu->MenuIsOpen(isOpen);
    mCurrentMenu->SelectMenu(PR_FALSE);
    if (isOpen) {
      // Don't close up immediately.
      // Kick off a close timer.
      KillCloseTimer(); // Ensure we don't have another stray waiting closure.
      PRInt32 menuDelay = 300;   // ms

      nsILookAndFeel * lookAndFeel;
      if (NS_OK == nsComponentManager::CreateInstance(kLookAndFeelCID, nsnull, 
                      NS_GET_IID(nsILookAndFeel), (void**)&lookAndFeel)) {
        lookAndFeel->GetMetric(nsILookAndFeel::eMetric_SubmenuDelay, menuDelay);
       NS_RELEASE(lookAndFeel);
      }

      // Kick off the timer.
      mCloseTimer = do_CreateInstance("@mozilla.org/timer;1");
      nsCOMPtr<nsITimerInternal> ti = do_QueryInterface(mCloseTimer);
      ti->SetIdle(PR_FALSE);
      mCloseTimer->InitWithCallback(this, menuDelay, nsITimer::TYPE_ONE_SHOT); 
      mTimerMenu = mCurrentMenu;
    }
  }

  // Set the new child.
  if (aMenuItem) {
    EnsureMenuItemIsVisible(aMenuItem);
    aMenuItem->SelectMenu(PR_TRUE);
  }

  mCurrentMenu = aMenuItem;

  return NS_OK;
}


NS_IMETHODIMP
nsMenuPopupFrame::Escape(PRBool& aHandledFlag)
{
  mIncrementalString.Truncate();

  // See if we have a context menu open.
  nsCOMPtr<nsIMenuParent> contextMenu;
  GetContextMenu(getter_AddRefs(contextMenu));
  if (contextMenu) {
    // Get the context menu parent.
    nsIFrame* childFrame;
    CallQueryInterface(contextMenu, &childFrame);
    nsIPopupSetFrame* popupSetFrame = GetPopupSetFrame(mPresContext);
    if (popupSetFrame)
      // Destroy the popup.
      popupSetFrame->DestroyPopup(childFrame, PR_FALSE);
    aHandledFlag = PR_TRUE;
    return NS_OK;
  }

  if (!mCurrentMenu)
    return NS_OK;

  // See if our menu is open.
  PRBool isOpen = PR_FALSE;
  mCurrentMenu->MenuIsOpen(isOpen);
  if (isOpen) {
    // Let the child menu handle this.
    mCurrentMenu->Escape(aHandledFlag);
    if (!aHandledFlag) {
      // We should close up.
      mCurrentMenu->OpenMenu(PR_FALSE);
      aHandledFlag = PR_TRUE;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsMenuPopupFrame::Enter()
{
  mIncrementalString.Truncate();

  // See if we have a context menu open.
  nsCOMPtr<nsIMenuParent> contextMenu;
  GetContextMenu(getter_AddRefs(contextMenu));
  if (contextMenu)
    return contextMenu->Enter();

  // Give it to the child.
  if (mCurrentMenu)
    mCurrentMenu->Enter();

  return NS_OK;
}

void 
nsMenuPopupFrame::GetContextMenu(nsIMenuParent** aContextMenu)
{
  *aContextMenu = nsnull;
  if (mIsContextMenu || !nsMenuFrame::sDismissalListener)
    return;

  nsCOMPtr<nsIMenuParent> menuParent;
  nsMenuFrame::sDismissalListener->GetCurrentMenuParent(getter_AddRefs(menuParent));
  if (!menuParent)
    return;

  PRBool isContextMenu;
  menuParent->GetIsContextMenu(isContextMenu);
  if (isContextMenu) {
    *aContextMenu = menuParent;
    NS_ADDREF(*aContextMenu);
  }
}

nsIMenuFrame*
nsMenuPopupFrame::FindMenuWithShortcut(nsIDOMKeyEvent* aKeyEvent, PRBool& doAction)
{
  PRUint32 charCode, keyCode;
  aKeyEvent->GetCharCode(&charCode);
  aKeyEvent->GetKeyCode(&keyCode);

  doAction = PR_FALSE;

  // Enumerate over our list of frames.
  nsIFrame* immediateParent = nsnull;
  nsCOMPtr<nsIPresShell> shell;
  mPresContext->GetShell(getter_AddRefs(shell));
  GetInsertionPoint(shell, this, nsnull, &immediateParent);
  if (!immediateParent)
    immediateParent = this;

  PRUint32 matchCount = 0, matchShortcutCount = 0;
  PRBool foundActive = PR_FALSE;
  PRBool isMenu = PR_FALSE;
  PRBool isShortcut;
  nsIMenuFrame* frameBefore = nsnull;
  nsIMenuFrame* frameAfter = nsnull;
  nsIMenuFrame* frameShortcut = nsnull;

  nsCOMPtr<nsIContent> parentContent;
  mContent->GetParent(*getter_AddRefs(parentContent));
  if (parentContent) {
    nsCOMPtr<nsIAtom> tag;
    parentContent->GetTag(*getter_AddRefs(tag));
    if (tag != nsXULAtoms::menulist)
      isMenu = PR_TRUE;
  }

  static DOMTimeStamp lastKeyTime = 0;
  DOMTimeStamp keyTime;
  aKeyEvent->GetTimeStamp(&keyTime);

  if (charCode == 0) {
    if (keyCode == NS_VK_BACK) {
      if (!isMenu && !mIncrementalString.IsEmpty()) {
        mIncrementalString.SetLength(mIncrementalString.Length() - 1);
        return nsnull;
      }
      else {
#ifdef XP_WIN
        nsCOMPtr<nsISound> soundInterface = do_CreateInstance("@mozilla.org/sound;1");
        if (soundInterface)
          soundInterface->Beep();
#endif  // #ifdef XP_WIN
      }
    }
    return nsnull;
  }
  else {
    PRUnichar uniChar = ToLowerCase(NS_STATIC_CAST(PRUnichar, charCode));
    if (isMenu || // Menu supports only first-letter navigation
        keyTime - lastKeyTime > INC_TYP_INTERVAL) // Interval too long, treat as new typing
      mIncrementalString = uniChar;
    else {
      mIncrementalString.Append(uniChar);
    }
  }

  // See bug 188199 & 192346, if all letters in incremental string are same, just try to match the first one
  nsAutoString incrementalString(mIncrementalString);
  PRUint32 charIndex = 1, stringLength = incrementalString.Length();
  while (charIndex < stringLength && incrementalString[charIndex] == incrementalString[charIndex - 1]) {
    charIndex++;
  }
  if (charIndex == stringLength) {
    incrementalString.Truncate(1);
    stringLength = 1;
  }

  lastKeyTime = keyTime;

  nsIFrame* currFrame;
  // NOTE: If you crashed here due to a bogus |immediateParent| it is 
  //       possible that the menu whose shortcut is being looked up has 
  //       been destroyed already.  One strategy would be to 
  //       setTimeout(<func>,0) as detailed in:
  //       <http://bugzilla.mozilla.org/show_bug.cgi?id=126675#c32>
  immediateParent->FirstChild(mPresContext, nsnull, &currFrame);

  // We start searching from first child. This process is divided into two parts
  //   -- before current and after current -- by the current item
  while (currFrame) {
    nsCOMPtr<nsIContent> current;
    currFrame->GetContent(getter_AddRefs(current));
    
    // See if it's a menu item.
    if (IsValidItem(current)) {
      nsAutoString activeKey, textKey;
      // Get the shortcut attribute.
      current->GetAttr(kNameSpaceID_None, nsXULAtoms::accesskey, textKey);
      if (textKey.IsEmpty()) { // No shortcut, try first letter
        isShortcut = PR_FALSE;
        current->GetAttr(kNameSpaceID_None, nsXULAtoms::label, textKey);
        if (textKey.IsEmpty()) // No label, try another attribute (value)
          current->GetAttr(kNameSpaceID_None, nsXULAtoms::value, textKey);
      }
      else
        isShortcut = PR_TRUE;

      if (Substring(textKey, 0, stringLength).Equals(incrementalString, nsCaseInsensitiveStringComparator())) {
        // mIncrementalString is a prefix of textKey
        nsCOMPtr<nsIMenuFrame> menuFrame = do_QueryInterface(currFrame);
        if (menuFrame) {
          // There is one match
          matchCount++;
          if (isShortcut) {
            // There is one shortcut-key match
            matchShortcutCount++;
            // Record the matched item. If there is only one matched shortcut item, do it
            frameShortcut = menuFrame.get();
          }
          if (!foundActive) {
            // It's a first candidate item located before/on the current item
            if (!frameBefore)
              frameBefore = menuFrame.get();
          }
          else {
            // It's a first candidate item located after the current item
            if (!frameAfter)
              frameAfter = menuFrame.get();
          }
        }
        else
          return nsnull;
      }

      // Get the active status
      current->GetAttr(kNameSpaceID_None, nsXULAtoms::menuactive, activeKey);
      if (activeKey == NS_LITERAL_STRING("true")) {
        foundActive = PR_TRUE;
        if (stringLength > 1) {
          // If there is more than one char typed, the current item has highest priority,
          //   otherwise the item next to current has highest priority
          nsCOMPtr<nsIMenuFrame> menuFrame = do_QueryInterface(currFrame);
          if (menuFrame && menuFrame.get() == frameBefore) {
            return frameBefore;
          }
        }
      }
    }
    currFrame->GetNextSibling(&currFrame);
  }

  doAction = (isMenu && (matchCount == 1 || matchShortcutCount == 1));

  if (matchShortcutCount == 1) // We have one matched shortcut item
    return frameShortcut;
  if (frameAfter) // If we have matched item after the current, use it
    return frameAfter;
  else if (frameBefore) // If we haven't, use the item before the current
    return frameBefore;

  // If we don't match anything, rollback the last typing
  mIncrementalString.SetLength(mIncrementalString.Length() - 1);

  // didn't find a matching menu item
#ifdef XP_WIN
  // behavior on Windows - this item is in a menu popup off of the
  // menu bar, so beep and do nothing else
  if (isMenu) {
    nsCOMPtr<nsISound> soundInterface = do_CreateInstance("@mozilla.org/sound;1");
    if (soundInterface)
      soundInterface->Beep();
  }
#endif  // #ifdef XP_WIN

  return nsnull;
}

NS_IMETHODIMP 
nsMenuPopupFrame::ShortcutNavigation(nsIDOMKeyEvent* aKeyEvent, PRBool& aHandledFlag)
{
  if (mCurrentMenu) {
    PRBool isOpen = PR_FALSE;
    mCurrentMenu->MenuIsOpen(isOpen);
    if (isOpen) {
      // No way this applies to us. Give it to our child.
      mCurrentMenu->ShortcutNavigation(aKeyEvent, aHandledFlag);
      return NS_OK;
    }
  }

  // This applies to us. Let's see if one of the shortcuts applies
  PRBool action;
  nsIMenuFrame* result = FindMenuWithShortcut(aKeyEvent, action);
  if (result) {
    // We got one!
    aHandledFlag = PR_TRUE;
    SetCurrentMenuItem(result);
    if (action)
      result->Enter();
  }

  return NS_OK;
}

NS_IMETHODIMP
nsMenuPopupFrame::KeyboardNavigation(PRUint32 aKeyCode, PRBool& aHandledFlag)
{
  // See if we have a context menu open.
  nsCOMPtr<nsIMenuParent> contextMenu;
  GetContextMenu(getter_AddRefs(contextMenu));
  if (contextMenu)
    return contextMenu->KeyboardNavigation(aKeyCode, aHandledFlag);

  nsNavigationDirection theDirection;
  NS_DIRECTION_FROM_KEY_CODE(theDirection, aKeyCode);

  mIncrementalString.Truncate();

  // This method only gets called if we're open.
  if (!mCurrentMenu && NS_DIRECTION_IS_INLINE(theDirection)) {
    // We've been opened, but we haven't had anything selected.
    // We can handle End, but our parent handles Start.
    if (theDirection == eNavigationDirection_End) {
      nsIMenuFrame* nextItem;
      GetNextMenuItem(nsnull, &nextItem);
      if (nextItem) {
        aHandledFlag = PR_TRUE;
        SetCurrentMenuItem(nextItem);
      }
    }
    return NS_OK;
  }

  PRBool isContainer = PR_FALSE;
  PRBool isOpen = PR_FALSE;
  PRBool isDisabled = PR_FALSE;
  if (mCurrentMenu) {
    mCurrentMenu->MenuIsContainer(isContainer);
    mCurrentMenu->MenuIsOpen(isOpen);
    mCurrentMenu->MenuIsDisabled(isDisabled);

    if (isOpen) {
      // Give our child a shot.
      mCurrentMenu->KeyboardNavigation(aKeyCode, aHandledFlag);
    }
    else if (theDirection == eNavigationDirection_End &&
             isContainer && !isDisabled) {
      // The menu is not yet open. Open it and select the first item.
      aHandledFlag = PR_TRUE;
      mCurrentMenu->OpenMenu(PR_TRUE);
      mCurrentMenu->SelectFirstItem();
    }
  }

  if (aHandledFlag)
    return NS_OK; // The child menu took it for us.

  // For block progression, we can move in either direction
  if (NS_DIRECTION_IS_BLOCK(theDirection) ||
      NS_DIRECTION_IS_BLOCK_TO_EDGE(theDirection)) {

    nsIMenuFrame* nextItem;
    
    if (theDirection == eNavigationDirection_Before)
      GetPreviousMenuItem(mCurrentMenu, &nextItem);
    else if (theDirection == eNavigationDirection_After)
      GetNextMenuItem(mCurrentMenu, &nextItem);
    else if (theDirection == eNavigationDirection_First)
      GetNextMenuItem(nsnull, &nextItem);
    else
      GetPreviousMenuItem(nsnull, &nextItem);

    SetCurrentMenuItem(nextItem);

    aHandledFlag = PR_TRUE;
  }
  else if (mCurrentMenu && isContainer && isOpen) {
    if (theDirection == eNavigationDirection_Start) {
      // Close it up.
      mCurrentMenu->OpenMenu(PR_FALSE);
      aHandledFlag = PR_TRUE;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsMenuPopupFrame::GetParentPopup(nsIMenuParent** aMenuParent)
{
  *aMenuParent = nsnull;
  nsIFrame* frame;
  GetParent(&frame);
  if (frame) {
    nsIFrame* grandparent;
    frame->GetParent(&grandparent);
    if (grandparent) {
      nsCOMPtr<nsIMenuParent> menuParent = do_QueryInterface(grandparent);
      if (menuParent) {
        *aMenuParent = menuParent.get();
        NS_ADDREF(*aMenuParent);
      }
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsMenuPopupFrame::HideChain()
{
  if (!mShouldRollup)
    return NS_OK;

  // Stop capturing rollups
  // (must do this during Hide, which happens before the menu item is executed,
  // since this reinstates normal event handling.)
  if (nsMenuFrame::sDismissalListener)
    nsMenuFrame::sDismissalListener->Unregister();
  
  nsIFrame* frame;
  GetParent(&frame);
  if (frame) {
    nsCOMPtr<nsIMenuFrame> menuFrame = do_QueryInterface(frame);
    if (!menuFrame) {
      nsIPopupSetFrame* popupSetFrame = GetPopupSetFrame(mPresContext);
      if (popupSetFrame)
        // Hide the popup.
        popupSetFrame->HidePopup(this);
      return NS_OK;
    }
   
    menuFrame->ActivateMenu(PR_FALSE);
    menuFrame->SelectMenu(PR_FALSE);

    // Get the parent.
    nsCOMPtr<nsIMenuParent> menuParent;
    menuFrame->GetMenuParent(getter_AddRefs(menuParent));
    if (menuParent)
      menuParent->HideChain();
  }

  return NS_OK;
}

NS_IMETHODIMP
nsMenuPopupFrame::DismissChain()
{
  if (!mShouldRollup)
    return NS_OK;

  // Stop capturing rollups
  if (nsMenuFrame::sDismissalListener)
    nsMenuFrame::sDismissalListener->Unregister();
  
  // Get our menu parent.
  nsIFrame* frame;
  GetParent(&frame);
  if (frame) {
    nsIMenuFrame *menuFrame = nsnull;
    CallQueryInterface(frame, &menuFrame);
    if (!menuFrame) {
      nsIPopupSetFrame* popupSetFrame = GetPopupSetFrame(mPresContext);
      if (popupSetFrame) {
        // make sure the menu is not highlighted
        if (mCurrentMenu)
          mCurrentMenu->SelectMenu(PR_FALSE);
        // Destroy the popup.
        popupSetFrame->DestroyPopup(this, PR_TRUE);
      }
      return NS_OK;
    }
  
    menuFrame->OpenMenu(PR_FALSE);

    // Get the parent.
    nsIMenuParent* menuParent;
    menuFrame->GetMenuParent(&menuParent);
    if (menuParent)
      menuParent->DismissChain();
  }

  return NS_OK;
}

NS_IMETHODIMP
nsMenuPopupFrame::GetWidget(nsIWidget **aWidget)
{
  // Get parent view
  nsIView * view = nsnull;
  // XXX should this be passing PR_FALSE or PR_TRUE for aStopAtViewManagerRoot?
  nsMenuPopupFrame::GetRootViewForPopup(mPresContext, this, PR_FALSE, &view);
  if (!view)
    return NS_OK;

  view->GetWidget(*aWidget);
  return NS_OK;
}

NS_IMETHODIMP
nsMenuPopupFrame::CreateDismissalListener()
{
  nsMenuDismissalListener *listener = new nsMenuDismissalListener();
  if (!listener) return NS_ERROR_OUT_OF_MEMORY;
  nsMenuFrame::sDismissalListener = listener;
  NS_ADDREF(listener);
  return NS_OK;
}

NS_IMETHODIMP
nsMenuPopupFrame::InstallKeyboardNavigator()
{
  if (mKeyboardNavigator)
    return NS_OK;

  nsCOMPtr<nsIDocument> doc;
  mContent->GetDocument(*getter_AddRefs(doc));
  nsCOMPtr<nsIDOMEventReceiver> target = do_QueryInterface(doc);
  
  mTarget = target;
  mKeyboardNavigator = new nsMenuListener(this);
  NS_IF_ADDREF(mKeyboardNavigator);

  target->AddEventListener(NS_LITERAL_STRING("keypress"), (nsIDOMKeyListener*)mKeyboardNavigator, PR_TRUE); 
  target->AddEventListener(NS_LITERAL_STRING("keydown"), (nsIDOMKeyListener*)mKeyboardNavigator, PR_TRUE);  
  target->AddEventListener(NS_LITERAL_STRING("keyup"), (nsIDOMKeyListener*)mKeyboardNavigator, PR_TRUE);   
  
  return NS_OK;
}

NS_IMETHODIMP
nsMenuPopupFrame::RemoveKeyboardNavigator()
{
  if (!mKeyboardNavigator)
    return NS_OK;

  mTarget->RemoveEventListener(NS_LITERAL_STRING("keypress"), (nsIDOMKeyListener*)mKeyboardNavigator, PR_TRUE);
  mTarget->RemoveEventListener(NS_LITERAL_STRING("keydown"), (nsIDOMKeyListener*)mKeyboardNavigator, PR_TRUE);
  mTarget->RemoveEventListener(NS_LITERAL_STRING("keyup"), (nsIDOMKeyListener*)mKeyboardNavigator, PR_TRUE);
  
  NS_IF_RELEASE(mKeyboardNavigator);

  return NS_OK;
}

// helpers /////////////////////////////////////////////////////////////

PRBool 
nsMenuPopupFrame::IsValidItem(nsIContent* aContent)
{
  nsCOMPtr<nsIAtom> tag;
  aContent->GetTag(*getter_AddRefs(tag));
  if (tag && (tag.get() == nsXULAtoms::menu ||
              tag.get() == nsXULAtoms::menuitem ||
              tag.get() == nsHTMLAtoms::option))
      return PR_TRUE;

  return PR_FALSE;
}

PRBool 
nsMenuPopupFrame::IsDisabled(nsIContent* aContent)
{
  nsString disabled;
  aContent->GetAttr(kNameSpaceID_None, nsHTMLAtoms::disabled, disabled);
  if (disabled == NS_LITERAL_STRING("true"))
    return PR_TRUE;
  return PR_FALSE;
}

NS_IMETHODIMP 
nsMenuPopupFrame::AttributeChanged(nsIPresContext* aPresContext,
                                   nsIContent* aChild,
                                   PRInt32 aNameSpaceID,
                                   nsIAtom* aAttribute,
                                   PRInt32 aModType, 
                                   PRInt32 aHint)

{
  nsresult rv = nsBoxFrame::AttributeChanged(aPresContext, aChild,
                                             aNameSpaceID, aAttribute, aModType, aHint);
  
  if (aAttribute == nsXULAtoms::left || aAttribute == nsXULAtoms::top)
    MoveToAttributePosition();
  
  return rv;
}

void 
nsMenuPopupFrame::MoveToAttributePosition()
{
  // Move the widget around when the user sets the |left| and |top| attributes. 
  // Note that this is not the best way to move the widget, as it results in lots
  // of FE notifications and is likely to be slow as molasses. Use |moveTo| on
  // nsIPopupBoxObject if possible. 
  nsAutoString left, top;
  mContent->GetAttr(kNameSpaceID_None, nsXULAtoms::left, left);
  mContent->GetAttr(kNameSpaceID_None, nsXULAtoms::top, top);
  PRInt32 err1, err2, xPos, yPos;
  xPos = left.ToInteger(&err1);
  yPos = top.ToInteger(&err2);

  if (NS_SUCCEEDED(err1) && NS_SUCCEEDED(err2))
    MoveTo(xPos, yPos);
}


NS_IMETHODIMP 
nsMenuPopupFrame::HandleEvent(nsIPresContext* aPresContext, 
                              nsGUIEvent*     aEvent,
                              nsEventStatus*  aEventStatus)
{
  return nsBoxFrame::HandleEvent(aPresContext, aEvent, aEventStatus);
}

NS_IMETHODIMP
nsMenuPopupFrame::Destroy(nsIPresContext* aPresContext)
{
  return nsBoxFrame::Destroy(aPresContext);
}

NS_IMETHODIMP
nsMenuPopupFrame::GetFrameForPoint(nsIPresContext* aPresContext,
                                   const nsPoint& aPoint,
                                   nsFramePaintLayer aWhichLayer,    
                                   nsIFrame** aFrame)
{
  return nsBoxFrame::GetFrameForPoint(aPresContext, aPoint, aWhichLayer, aFrame);
}


//
// Notify
//
// The item selection timer has fired, we might have to readjust the 
// selected item. There are two cases here that we are trying to deal with:
//   (1) diagonal movement from a parent menu to a submenu passing briefly over
//       other items, and
//   (2) moving out from a submenu to a parent or grandparent menu.
// In both cases, |mTimerMenu| is the menu item that might have an open submenu and
// |mCurrentMenu| is the item the mouse is currently over, which could be none of them.
//
// case (1):
//  As the mouse moves from the parent item of a submenu (we'll call 'A') diagonally into the
//  submenu, it probably passes through one or more sibilings (B). As the mouse passes
//  through B, it becomes the current menu item and the timer is set and mTimerMenu is 
//  set to A. Before the timer fires, the mouse leaves the menu containing A and B and
//  enters the submenus. Now when the timer fires, |mCurrentMenu| is null (!= |mTimerMenu|)
//  so we have to see if anything in A's children is selected (recall that even disabled
//  items are selected, the style just doesn't show it). If that is the case, we need to
//  set the selected item back to A.
//
// case (2);
//  Item A has an open submenu, and in it there is an item (B) which also has an open
//  submenu (so there are 3 menus displayed right now). The mouse then leaves B's child
//  submenu and selects an item that is a sibling of A, call it C. When the mouse enters C,
//  the timer is set and |mTimerMenu| is A and |mCurrentMenu| is C. As the timer fires,
//  the mouse is still within C. The correct behavior is to set the current item to C
//  and close up the chain parented at A.
//
//  This brings up the question of is the logic of case (1) enough? The answer is no,
//  and is discussed in bugzilla bug 29400. Case (1) asks if A's submenu has a selected
//  child, and if it does, set the selected item to A. Because B has a submenu open, it
//  is selected and as a result, A is set to be the selected item even though the mouse
//  rests in C -- very wrong. 
//
//  The solution is to use the same idea, but instead of only checking one level, 
//  drill all the way down to the deepest open submenu and check if it has something 
//  selected. Since the mouse is in a grandparent, it won't, and we know that we can
//  safely close up A and all its children.
//
// The code below melds the two cases together.
//
NS_IMETHODIMP
nsMenuPopupFrame::Notify(nsITimer* aTimer)
{
  // Our timer has fired. 
  if (aTimer == mCloseTimer.get()) {
    PRBool menuOpen = PR_FALSE;
    mTimerMenu->MenuIsOpen(menuOpen);
    if (menuOpen) {
      if (mCurrentMenu != mTimerMenu) {
        // Walk through all of the sub-menus of this menu item until we get to the
        // last sub-menu, then check if that sub-menu has an active menu item.  If it
        // does, then keep that menu open.  If it doesn't, close menu and its sub-menus.
        nsIFrame* child;
        mTimerMenu->GetMenuChild(&child);
        nsCOMPtr<nsIMenuFrame> currentMenuItem = nsnull;

        nsCOMPtr<nsIMenuParent> menuParent = do_QueryInterface(child);
        while (menuParent)
        {
          // get the selected menu item for this sub-menu
          menuParent->GetCurrentMenuItem(getter_AddRefs(currentMenuItem));
          menuParent = nsnull;
          if (currentMenuItem)
          {
            // this sub-menu has a selected menu item - does that item open a sub-menu?
            currentMenuItem->GetMenuChild(&child);
            if (child) {
              // the selected menu item opens a sub-menu - move down
              // to that sub-menu and then go through the loop again
              menuParent = do_QueryInterface(child);
            } 
          } // if item is selected
        } // while we're not at the last submenu

        if (currentMenuItem)
        {
          // the sub-menu has a selected menu item, we're dealing with case (1)
          SetCurrentMenuItem(mTimerMenu);
        }
        else {
          // Nothing selected. Either the mouse never made it to the submenu 
          // in case (1) or we're in a sibling of a grandparent in case (2).
          // Regardless, close up the open chain.
          mTimerMenu->OpenMenu(PR_FALSE);
        }
      } // if not the menu with an open submenu
    } // if menu open
    
    if (mCloseTimer) {
      mCloseTimer->Cancel();
    }
  }
  
  mCloseTimer = nsnull;
  mTimerMenu = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
nsMenuPopupFrame::KillCloseTimer()
{
  if (mCloseTimer && mTimerMenu) {
    PRBool menuOpen = PR_FALSE;
    mTimerMenu->MenuIsOpen(menuOpen);
    if (menuOpen) {
      mTimerMenu->OpenMenu(PR_FALSE);
    }
    mCloseTimer->Cancel();
    mCloseTimer = nsnull;
    mTimerMenu = nsnull;
  }
  return NS_OK;
}



NS_IMETHODIMP
nsMenuPopupFrame::KillPendingTimers ( )
{
  return KillCloseTimer();

} // KillPendingTimers

void
nsMenuPopupFrame::MoveTo(PRInt32 aLeft, PRInt32 aTop)
{
  // Set the 'left' and 'top' attributes
  nsAutoString left, top;
  left.AppendInt(aLeft);
  top.AppendInt(aTop);

  mContent->SetAttr(kNameSpaceID_None, nsXULAtoms::left, left, PR_FALSE);
  mContent->SetAttr(kNameSpaceID_None, nsXULAtoms::top, top, PR_FALSE);

  nsIView* view = nsnull;
  GetView(mPresContext, &view);   
  
  // Retrieve screen position of parent view
  nsIView* parentView = nsnull;
  view->GetParent(parentView);
  nsPoint screenPos;
  GetScreenPosition(parentView, screenPos);

  // Move the widget
  nsCOMPtr<nsIWidget> widget;
  view->GetWidget(*getter_AddRefs(widget));
  widget->Move(aLeft - screenPos.x, aTop - screenPos.y);
}

void
nsMenuPopupFrame::GetScreenPosition(nsIView* aView, nsPoint& aScreenPosition)
{
  nsPoint screenPos(0,0);
  nscoord x, y;

  nsIView* currView = aView;
  nsIView* nextView = nsnull;
  
  while (1) {
    currView->GetPosition(&x, &y);
    screenPos.x += x;
    screenPos.y += y;

    currView->GetParent(nextView);
    if (!nextView) 
      break;
    else 
      currView = nextView;
  }

  nsCOMPtr<nsIWidget> rootWidget;
  currView->GetWidget(*getter_AddRefs(rootWidget));
  nsRect bounds, screenBounds;
  rootWidget->GetScreenBounds(bounds);
  rootWidget->WidgetToScreen(bounds, screenBounds);

  float t2p;
  mPresContext->GetTwipsToPixels(&t2p);

  aScreenPosition.x = NSTwipsToIntPixels(screenPos.x, t2p) + screenBounds.x;
  aScreenPosition.y = NSTwipsToIntPixels(screenPos.y, t2p) + screenBounds.y;
}

void 
nsMenuPopupFrame::GetAutoPosition(PRBool* aShouldAutoPosition)
{
  *aShouldAutoPosition = mShouldAutoPosition;
}

void
nsMenuPopupFrame::SetAutoPosition(PRBool aShouldAutoPosition)
{
  mShouldAutoPosition = aShouldAutoPosition;
}

void
nsMenuPopupFrame::EnableRollup(PRBool aShouldRollup)
{
  if (!aShouldRollup) {
    if (nsMenuFrame::sDismissalListener)
      nsMenuFrame::sDismissalListener->Unregister();
  }
  else
    CreateDismissalListener();
}

