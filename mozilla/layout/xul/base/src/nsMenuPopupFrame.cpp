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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Original Author: David W. Hyatt (hyatt@netscape.com)
 *
 * Contributor(s): 
 *   Mike Pinkerton (pinkerton@netscape.com)
 *   Dean Tessman <dean_tessman@hotmail.com>
 */


#include "nsMenuPopupFrame.h"
#include "nsXULAtoms.h"
#include "nsHTMLAtoms.h"
#include "nsIContent.h"
#include "prtypes.h"
#include "nsIAtom.h"
#include "nsIPresContext.h"
#include "nsIStyleContext.h"
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

static NS_DEFINE_IID(kLookAndFeelCID, NS_LOOKANDFEEL_CID);

const PRInt32 kMaxZ = 0x7fffffff; //XXX: Shouldn't there be a define somewhere for MaxInt for PRInt32


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
    mMenuCanOverlapOSBar(PR_FALSE)
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
                       nsIStyleContext* aContext,
                       nsIFrame*        aPrevInFlow)
{
  nsresult rv = nsBoxFrame::Init(aPresContext, aContent, aParent, aContext, aPrevInFlow);

  // lookup if we're allowed to overlap the OS bar (menubar/taskbar) from the
  // look&feel object
  nsCOMPtr<nsILookAndFeel> lookAndFeel;
  nsComponentManager::CreateInstance(kLookAndFeelCID, nsnull, NS_GET_IID(nsILookAndFeel), 
                                      getter_AddRefs(lookAndFeel));
  if ( lookAndFeel )
    lookAndFeel->GetMetric(nsILookAndFeel::eMetric_MenusCanOverlapOSBar, mMenuCanOverlapOSBar);

  // XXX Hack
  mPresContext = aPresContext;

  CreateViewForFrame(aPresContext, this, aContext, PR_TRUE);

  // Now that we've made a view, remove it and insert it at the correct
  // position in the view hierarchy (as the root view).  We do this so that we
  // can draw the menus outside the confines of the window.
  nsIView* ourView;
  GetView(aPresContext, &ourView);

  nsIFrame* parent;
  GetParentWithView(aPresContext, &parent);
  nsIView* parentView;
  parent->GetView(aPresContext, &parentView);

  nsCOMPtr<nsIViewManager> viewManager;
  parentView->GetViewManager(*getter_AddRefs(viewManager));

  // Remove the view from its old position.
  viewManager->RemoveChild(parentView, ourView);

  // Reinsert ourselves as the root view with a maximum z-index.
  nsIView* rootView;
  viewManager->GetRootView(rootView);
  viewManager->InsertChild(rootView, ourView, kMaxZ);

  // XXX Hack. The menu's view should float above all other views,
  // so we use the nsIView::SetFloating() to tell the view manager
  // about that constraint.
  ourView->SetFloating(PR_TRUE);

  // XXX Hack. Change our transparency to be non-transparent
  // until the bug related to update of transparency on show/hide
  // is fixed.
  viewManager->SetViewContentTransparency(ourView, PR_FALSE);

  // Create a widget for ourselves.
  nsWidgetInitData widgetData;
  ourView->SetZIndex(kMaxZ);
  widgetData.mWindowType = eWindowType_popup;
  widgetData.mBorderStyle = eBorderStyle_default;

  // XXX make sure we are hidden (shouldn't this be done automatically?)
  ourView->SetVisibility(nsViewVisibility_kHide);
#ifdef XP_MAC
  printf("XP Popups: This is a nag to indicate that an inconsistent hack is being done on the Mac for popups.\n");
  static NS_DEFINE_IID(kCPopupCID,  NS_POPUP_CID);
  ourView->CreateWidget(kCPopupCID, &widgetData, nsnull);
#else
  static NS_DEFINE_IID(kCChildCID,  NS_CHILD_CID);
  ourView->CreateWidget(kCChildCID, &widgetData, nsnull);
#endif   

  return rv;
}

void
nsMenuPopupFrame::GetLayoutFlags(PRUint32& aFlags)
{
  aFlags = NS_FRAME_NO_SIZE_VIEW | NS_FRAME_NO_MOVE_VIEW | NS_FRAME_NO_VISIBILITY;
}

void
nsMenuPopupFrame::GetViewOffset(nsIViewManager* aManager, nsIView* aView, 
  nsPoint& aPoint)
{
  aPoint.x = 0;
  aPoint.y = 0;
 
  nsIView *parent;
  nsRect bounds;

  parent = aView;
  while (nsnull != parent) {
    parent->GetBounds(bounds);
    if (bounds.y >= 0) {
      aPoint.x += bounds.x;
      aPoint.y += bounds.y;
    }

    parent->GetParent(parent);
  }
}

void
nsMenuPopupFrame::GetNearestEnclosingView(nsIPresContext* aPresContext, nsIFrame* aStartFrame, nsIView** aResult)
{
  *aResult = nsnull;
  aStartFrame->GetView(aPresContext, aResult);
  if (!*aResult) {
    nsIFrame* parent;
    aStartFrame->GetParentWithView(aPresContext, &parent);
    if (parent)
      parent->GetView(aPresContext, aResult);
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
nsMenuPopupFrame :: AdjustClientXYForNestedDocuments ( nsIDOMXULDocument* inPopupDoc, nsIPresShell* inPopupShell, 
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
  
  // Find the widget associated with the target's document. Recall that we cached the
  // target popup node in the document of the popup in the nsXULPopupListener.
  nsCOMPtr<nsIDOMNode> targetNode;
  inPopupDoc->GetPopupNode(getter_AddRefs(targetNode));
  NS_WARN_IF_FALSE(targetNode, "ACK, BAD TARGET NODE");
  nsCOMPtr<nsIContent> targetAsContent ( do_QueryInterface(targetNode) );
  nsCOMPtr<nsIWidget> targetDocumentWidget;
  if ( targetAsContent ) {
    nsCOMPtr<nsIDocument> targetDocument;
    targetAsContent->GetDocument(*getter_AddRefs(targetDocument));
    if (targetDocument) {
      nsCOMPtr<nsIPresShell> shell ( dont_AddRef(targetDocument->GetShellAt(0)) );
      nsCOMPtr<nsIViewManager> viewManagerTarget;
      if ( shell ) {
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
  NS_WARN_IF_FALSE(targetDocumentWidget, "ACK, BAD TARGET");

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
nsMenuPopupFrame :: AdjustPositionForAnchorAlign ( PRInt32* ioXPos, PRInt32* ioYPos, const nsRect & inParentRect,
                                                    const nsString& aPopupAnchor, const nsString& aPopupAlign,
                                                    PRBool* outFlushWithTopBottom )
{
  if (aPopupAnchor == NS_LITERAL_STRING("topright") && aPopupAlign == NS_LITERAL_STRING("topleft")) {
    *ioXPos += inParentRect.width;
  }
  else if (aPopupAnchor == NS_LITERAL_STRING("topright") && aPopupAlign == NS_LITERAL_STRING("bottomright")) {
    *ioXPos -= (mRect.width - inParentRect.width);
    *ioYPos -= mRect.height;
    *outFlushWithTopBottom = PR_TRUE;
  }
  else if (aPopupAnchor == NS_LITERAL_STRING("bottomright") && aPopupAlign == NS_LITERAL_STRING("bottomleft")) {
    *ioXPos += inParentRect.width;
    *ioYPos -= (mRect.height - inParentRect.height);
  }
  else if (aPopupAnchor == NS_LITERAL_STRING("bottomright") && aPopupAlign == NS_LITERAL_STRING("topright")) {
    *ioXPos -= (mRect.width - inParentRect.width);
    *ioYPos += inParentRect.height;
    *outFlushWithTopBottom = PR_TRUE;
  }
  else if (aPopupAnchor == NS_LITERAL_STRING("topleft") && aPopupAlign == NS_LITERAL_STRING("topright")) {
    *ioXPos -= mRect.width;
  }
  else if (aPopupAnchor == NS_LITERAL_STRING("topleft") && aPopupAlign == NS_LITERAL_STRING("bottomleft")) {
    *ioYPos -= mRect.height;
    *outFlushWithTopBottom = PR_TRUE;
  }
  else if (aPopupAnchor == NS_LITERAL_STRING("bottomleft") && aPopupAlign == NS_LITERAL_STRING("bottomright")) {
    *ioXPos -= mRect.width;
    *ioYPos -= (mRect.height - inParentRect.height);
  }
  else if (aPopupAnchor == NS_LITERAL_STRING("bottomleft") && aPopupAlign == NS_LITERAL_STRING("topleft")) {
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
nsMenuPopupFrame :: IsMoreRoomOnOtherSideOfParent ( PRBool inFlushAboveBelow, PRInt32 inScreenViewLocX, PRInt32 inScreenViewLocY,
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
nsMenuPopupFrame :: MovePopupToOtherSideOfParent ( PRBool inFlushAboveBelow, PRInt32* ioXPos, PRInt32* ioYPos, 
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

  nsPoint parentPos;
  nsCOMPtr<nsIViewManager> viewManager;

  //
  // Collect info about our parent view and the frame we're sync'ing to
  //

  nsIView* parentView = nsnull;
  nsIFrame* parentFrame = nsnull;
  aFrame->GetParent(&parentFrame);

  GetNearestEnclosingView(aPresContext, parentFrame, &parentView);
  if (!parentView)
    return NS_OK;

  parentView->GetViewManager(*getter_AddRefs(viewManager));
  GetViewOffset(viewManager, parentView, parentPos);
  nsIView* view = nsnull;
  GetView(aPresContext, &view);

  nsIView* containingView = nsnull;
  nsPoint offset;
  aFrame->GetOffsetFromView(aPresContext, offset, &containingView);

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
  if (containingView) {
    nsIView* scrollportView = nsnull;
    containingView->GetParent(scrollportView);
    if (scrollportView) {
      nsIScrollableView* scrollableView = nsnull;
      scrollportView->QueryInterface(NS_GET_IID(nsIScrollableView), (void**) &scrollableView);
      if (scrollableView) {
        nsRect bounds;
        containingView->GetBounds(bounds);
        offset += nsPoint(bounds.x, bounds.y);
      }
    }
  }
  
  nsRect parentRect;
  aFrame->GetRect(parentRect);

  float p2t, t2p;
  aPresContext->GetScaledPixelsToTwips(&p2t);

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
  nsCOMPtr<nsIWidget> parentViewWidget;
  GetWidgetForView ( parentView, *getter_AddRefs(parentViewWidget) );
  nsRect localParentWidgetRect(0,0,0,0), screenParentWidgetRect;
  parentViewWidget->WidgetToScreen ( localParentWidgetRect, screenParentWidgetRect );
  PRInt32 screenViewLocX = NSIntPixelsToTwips(screenParentWidgetRect.x,p2t) + (xpos - parentPos.x);
  PRInt32 screenViewLocY = NSIntPixelsToTwips(screenParentWidgetRect.y,p2t) + (ypos - parentPos.y);

  if ( anchoredToParent ) {
    
    //
    // Popup is anchored to the parent, guarantee that it does not cover the parent. We
    // shouldn't do anything funky if it will already fit on the screen as is.
    //

    // compute screen coordinates of parent frame so we can play with it. Make sure we put it
    // into twips as everything else is as well.
    nsRect screenParentFrameRect ( NSTwipsToIntPixels(offset.x,t2p), NSTwipsToIntPixels(offset.y,t2p),
                                    parentRect.width, parentRect.height );
    parentViewWidget->WidgetToScreen ( screenParentFrameRect, screenParentFrameRect );
    screenParentFrameRect.x = NSIntPixelsToTwips(screenParentFrameRect.x, p2t);
    screenParentFrameRect.y = NSIntPixelsToTwips(screenParentFrameRect.y, p2t);
    
    // if it doesn't fit on the screen, do our magic.
    if ( (screenViewLocX + mRect.width) > screenRightTwips ||
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
        }
      }
      else {
        // move it up to be on screen, but don't let it go off the screen at the top
        if ( (screenViewLocY + mRect.height) > screenBottomTwips ) {
          PRInt32 moveDistY = (screenViewLocY + mRect.height) - screenBottomTwips;
          if ( screenViewLocY - moveDistY < screenTopTwips )
            moveDistY = screenViewLocY - screenTopTwips;          
          screenViewLocY -= moveDistY;
          ypos -= moveDistY; 
        } 
      }
      
      // Resize it to fit on the screen. By this point, we've given the popup as much
      // room as we can w/out covering the parent. If it still can't be as big
      // as it wants to be, well, it just has to suck up and deal. 
      PRInt32 xSpillage = (screenViewLocX + mRect.width) - screenRightTwips;
      if ( xSpillage > 0 )
        mRect.width -= xSpillage;
      PRInt32 ySpillage = (screenViewLocY + mRect.height) - screenBottomTwips;
      if ( ySpillage > 0 )
        mRect.height -= ySpillage;

    } // if it doesn't fit on screen
  } // if anchored to parent
  else {
  
    //
    // Popup not anchored to anything, just make sure it's on the screen by any
    // means necessary
    //

    // add back in the parentPos offset. Not sure why, but we need this for mail/news
    // context menus and we can't do this in the case where there popup is anchored.
    screenViewLocX += parentPos.x;
    screenViewLocY += parentPos.y;    
    
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
      // as a result of moving the popup, it might end up under the mouse. This
      // would be bad as the subsequent mouse_up would trigger whatever
      // unsuspecting item happens to be at that position. To get around this, make
      // move it so the right edge is where the mouse is, as we're guaranteed
      // that the mouse is on the screen!
      xpos -= mRect.width;      
    }
    if ( (screenViewLocY + mRect.height) > screenBottomTwips )
      ypos -= (screenViewLocY + mRect.height) - screenBottomTwips;
      
  }  

  // finally move and resize it
  viewManager->MoveViewTo(view, xpos, ypos); 
  //viewManager->ResizeView(view, mRect.width, mRect.height);
  
  nsAutoString shouldDisplay, menuActive;
  mContent->GetAttribute(kNameSpaceID_None, nsXULAtoms::menuactive, menuActive);
  if (menuActive != NS_LITERAL_STRING("true")) {
    mContent->GetAttribute(kNameSpaceID_None, nsXULAtoms::menutobedisplayed, shouldDisplay);
    if ( shouldDisplay == NS_LITERAL_STRING("true") )
      mContent->SetAttribute(kNameSpaceID_None, nsXULAtoms::menuactive, NS_LITERAL_STRING("true"), PR_TRUE);
  }

  return NS_OK;
}

static void GetInsertionPoint(nsIPresShell* aShell, nsIFrame* aFrame, nsIFrame* aChild,
                              nsIFrame** aResult)
{
  nsCOMPtr<nsIFrameManager> frameManager;
  aShell->GetFrameManager(getter_AddRefs(frameManager));
  nsCOMPtr<nsIContent> child;
  aChild->GetContent(getter_AddRefs(child));
  frameManager->GetInsertionPoint(aShell, aFrame, child, aResult);
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
      mCloseTimer->Init(this, menuDelay, NS_PRIORITY_HIGHEST); 
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
  // Give it to the child.
  if (mCurrentMenu)
    mCurrentMenu->Enter();

  return NS_OK;
}

nsIMenuFrame*
nsMenuPopupFrame::FindMenuWithShortcut(PRUint32 aLetter)
{

  // Enumerate over our list of frames.
  nsIFrame* immediateParent = nsnull;
  nsCOMPtr<nsIPresShell> shell;
  mPresContext->GetShell(getter_AddRefs(shell));
  GetInsertionPoint(shell, this, nsnull, &immediateParent);
  if (!immediateParent)
    immediateParent = this;

  nsIFrame* currFrame;
  immediateParent->FirstChild(mPresContext, nsnull, &currFrame);

  while (currFrame) {
    nsCOMPtr<nsIContent> current;
    currFrame->GetContent(getter_AddRefs(current));
    
    // See if it's a menu item.
    if (IsValidItem(current)) {
      // Get the shortcut attribute.
      nsAutoString shortcutKey;
      current->GetAttribute(kNameSpaceID_None, nsXULAtoms::accesskey, shortcutKey);
      if (shortcutKey.Length() > 0) {
        // We've got something.
        char tempChar[2];
        tempChar[0] = aLetter;
        tempChar[1] = 0;
        nsAutoString tempChar2; tempChar2.AssignWithConversion(tempChar);
  
        if (shortcutKey.EqualsIgnoreCase(tempChar2)) {
          // We match!
          nsCOMPtr<nsIMenuFrame> menuFrame = do_QueryInterface(currFrame);
          if (menuFrame)
            return menuFrame.get();
          return nsnull;
        }
      }
    }
    currFrame->GetNextSibling(&currFrame);
  }
  return nsnull;
}

NS_IMETHODIMP 
nsMenuPopupFrame::ShortcutNavigation(PRUint32 aLetter, PRBool& aHandledFlag)
{
  if (mCurrentMenu) {
    PRBool isOpen = PR_FALSE;
    mCurrentMenu->MenuIsOpen(isOpen);
    if (isOpen) {
      // No way this applies to us. Give it to our child.
      mCurrentMenu->ShortcutNavigation(aLetter, aHandledFlag);
      return NS_OK;
    }
  }

  // This applies to us. Let's see if one of the shortcuts applies
  nsIMenuFrame* result = FindMenuWithShortcut(aLetter);
  if (result) {
    // We got one!
    aHandledFlag = PR_TRUE;
    SetCurrentMenuItem(result);
    result->Enter();
  }

  return NS_OK;
}

NS_IMETHODIMP
nsMenuPopupFrame::KeyboardNavigation(PRUint32 aDirection, PRBool& aHandledFlag)
{
  // This method only gets called if we're open.
  if (!mCurrentMenu && (aDirection == NS_VK_RIGHT || aDirection == NS_VK_LEFT)) {
    // We've been opened, but we haven't had anything selected.
    // We can handle RIGHT, but our parent handles LEFT.
    if (aDirection == NS_VK_RIGHT) {
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
      mCurrentMenu->KeyboardNavigation(aDirection, aHandledFlag);
    }
    else if (aDirection == NS_VK_RIGHT && isContainer && !isDisabled) {
      // The menu is not yet open. Open it and select the first item.
      aHandledFlag = PR_TRUE;
      mCurrentMenu->OpenMenu(PR_TRUE);
      mCurrentMenu->SelectFirstItem();
    }
  }

  if (aHandledFlag)
    return NS_OK; // The child menu took it for us.

  // For the vertical direction, we can move up or down.
  if (aDirection == NS_VK_UP || aDirection == NS_VK_DOWN ||
      aDirection == NS_VK_HOME || aDirection == NS_VK_END) {

    nsIMenuFrame* nextItem;
    
    if (aDirection == NS_VK_UP)
      GetPreviousMenuItem(mCurrentMenu, &nextItem);
    else if (aDirection == NS_VK_DOWN)
      GetNextMenuItem(mCurrentMenu, &nextItem);
    else if (aDirection == NS_VK_HOME)
      GetNextMenuItem(nsnull, &nextItem);
    else
      GetPreviousMenuItem(nsnull, &nextItem);

    SetCurrentMenuItem(nextItem);

    aHandledFlag = PR_TRUE;
  }
  else if (mCurrentMenu && isContainer && isOpen) {
    if (aDirection == NS_VK_LEFT) {
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
  // Stop capturing rollups
  // (must do this during Hide, which happens before the menu item is executed,
  // since this reinstates normal event handling.)
  if (nsMenuFrame::mDismissalListener)
    nsMenuFrame::mDismissalListener->Unregister();
  
  nsIFrame* frame;
  GetParent(&frame);
  if (frame) {
    nsCOMPtr<nsIPopupSetFrame> popupSetFrame = do_QueryInterface(frame);
    if (popupSetFrame) {
      // Destroy the popup.
      popupSetFrame->HidePopup();
      return NS_OK;
    }
    
    nsCOMPtr<nsIMenuFrame> menuFrame = do_QueryInterface(frame);
    if (!menuFrame)
      return NS_OK;

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
  // Stop capturing rollups
  if (nsMenuFrame::mDismissalListener)
    nsMenuFrame::mDismissalListener->Unregister();
  
  // Get our menu parent.
  nsIFrame* frame;
  GetParent(&frame);
  if (frame) {
    nsCOMPtr<nsIPopupSetFrame> popupSetFrame = do_QueryInterface(frame);
    if (popupSetFrame) {
      // Destroy the popup.
      popupSetFrame->DestroyPopup();
      return NS_OK;
    }

    nsCOMPtr<nsIMenuFrame> menuFrame = do_QueryInterface(frame);
    if (!menuFrame)
      return NS_OK;
    
    menuFrame->OpenMenu(PR_FALSE);

    // Get the parent.
    nsCOMPtr<nsIMenuParent> menuParent;
    menuFrame->GetMenuParent(getter_AddRefs(menuParent));
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
  nsMenuPopupFrame::GetNearestEnclosingView(mPresContext, this, &view);
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
  nsMenuFrame::mDismissalListener = listener;
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
              tag.get() == nsXULAtoms::menuitem))
      return PR_TRUE;

  return PR_FALSE;
}

PRBool 
nsMenuPopupFrame::IsDisabled(nsIContent* aContent)
{
  nsString disabled;
  aContent->GetAttribute(kNameSpaceID_None, nsHTMLAtoms::disabled, disabled);
  if (disabled == NS_LITERAL_STRING("true"))
    return PR_TRUE;
  return PR_FALSE;
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
  //nsCOMPtr<nsIDOMEventReceiver> target = do_QueryInterface(mContent);
  //target->RemoveEventListener("mousemove", mMenuPopupEntryListener, PR_TRUE);
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
NS_IMETHODIMP_(void)
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
    
    mCloseTimer->Cancel();
  }
  
  mCloseTimer = nsnull;
  mTimerMenu = nsnull;
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
nsMenuPopupFrame :: KillPendingTimers ( )
{
  return KillCloseTimer();

} // KillPendingTimers
