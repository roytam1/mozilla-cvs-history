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
 * Contributor(s): 
 */

#ifndef nsIView_h___
#define nsIView_h___

#include <stdio.h>
#include "nsISupports.h"
#include "gfxtypes.h"
#include "nsGUIEvent.h"

class nsIWindow;
class nsIWindow;
class nsIViewManager;
class nsIRegion;
class nsIRenderingContext;
class nsTransform2D;
class nsIFrame;
class nsIViewObserver;
class nsVoidArray;
struct nsRect;

//this is used by the view clipping APIs since the description of
//a clip rect is different than a rect

struct nsViewClip {
  gfx_coord mLeft;
  gfx_coord mRight;
  gfx_coord mTop;
  gfx_coord mBottom;
};

// Enumerated type to indicate the visibility of a layer.
// hide - the layer is not shown.
// show - the layer is shown irrespective of the visibility of 
//        the layer's parent.
// inherit - the layer inherits its visibility from its parent.
enum nsViewVisibility {
  nsViewVisibility_kHide = 0,
  nsViewVisibility_kShow = 1,
  nsViewVisibility_kInherit = 2
};

// IID for the nsIView interface
#define NS_IVIEW_IID    \
{ 0xf0a21c40, 0xa7e1, 0x11d1, \
{ 0xa8, 0x24, 0x00, 0x40, 0x95, 0x9a, 0x28, 0xc9 } }

//----------------------------------------------------------------------

/**
 * View interface
 *
 * Views are NOT reference counted. Use the Destroy() member function to
 * destroy a frame.
 *
 * The lifetime of the view hierarchy is bounded by the lifetime of the
 * view manager that owns the views.
 */
class nsIView : public nsISupports
{
public:
  NS_DEFINE_STATIC_IID_ACCESSOR(NS_IVIEW_IID)

  /**
   * Initialize the view
   * @param aManager view manager that "owns" the view. The view does NOT
   *        hold a reference to the view manager
   * @param aBounds initial bounds for view
   * @param aParent intended parent for view. this is not actually set in the
   *        nsIView through this method. it is only used by the initialization
   *        code to walk up the view tree, if necessary, to find resources.
   * @param aVisibilityFlag initial visibility state of view
   * @result The result of the initialization, NS_OK if no errors
   */
  NS_IMETHOD  Init(nsIViewManager* aManager,
						       const nsRect &aBounds,
                   const nsIView *aParent,
        					 nsViewVisibility aVisibilityFlag = nsViewVisibility_kShow) = 0;

  /**
   * Destroy the view.
   *
   * The view destroys its child views, and destroys and releases its
   * widget (if it has one).
   *
   * Also informs the view manager that the view is destroyed by calling
   * SetRootView(NULL) if the view is the root view and calling RemoveChild()
   * otherwise.
   */
  NS_IMETHOD  Destroy() = 0;

  /**
   * Get the view manager the "owns" the view
   * @result view manager
   */
  NS_IMETHOD  GetViewManager(nsIViewManager *&aViewMgr) const = 0;

  /**
   * Called to indicate that the specified rect of the view
   * needs to be drawn via the rendering context. The rect
   * is specified in view coordinates.
   * @param rc rendering context to paint into
   * @param rect damage area
   * @param aPaintFlags see nsIView.h for flag definitions
   * @return PR_TRUE if the entire clip region has been eliminated, else PR_FALSE
   */
  NS_IMETHOD  Paint(nsIRenderingContext& rc, const nsRect& rect,
                    PRUint32 aPaintFlags, PRBool &aResult) = 0;

  /**
   * Called to indicate that the specified region of the view
   * needs to be drawn via the rendering context. The region
   * is specified in view coordinates.
   * @param rc rendering context to paint into
   * @param region damage area
   * @param aPaintFlags see nsIView.h for flag definitions
   * @return PR_TRUE if the entire clip region has been eliminated, else PR_FALSE
   */
  NS_IMETHOD  Paint(nsIRenderingContext& rc, const nsIRegion& region,
                    PRUint32 aPaintFlags, PRBool &aResult) = 0;
  
  /**
   * Called to indicate that the specified event should be handled
   * by the view. This method should return nsEventStatus_eConsumeDoDefault
   * or nsEventStatus_eConsumeNoDefault if the event has been handled.
   * @param event event to process
   * @param aEventFlags see nsIView.h for flag definitions
   * @result processing status
   */
  NS_IMETHOD  HandleEvent(nsGUIEvent *event, 
                          PRUint32 aEventFlags, 
                          nsEventStatus* aStatus,
                          PRBool aForceHandle,
                          PRBool& aHandled) = 0;

  /**
   * Called to indicate that the position of the view has been changed.
   * The specified coordinates are in the parent view's coordinate space.
   * @param x new x position
   * @param y new y position
   */
  NS_IMETHOD  SetPosition(gfx_coord x, gfx_coord y) = 0;

  /**
   * Called to get the position of a view.
   * The specified coordinates are in the parent view's coordinate space.
   * @param x out parameter for x position
   * @param y out parameter for y position
   */
  NS_IMETHOD  GetPosition(gfx_coord *x, gfx_coord *y) const = 0;
  
  /**
   * Called to indicate that the dimensions of the view (actually the
   * width and height of the clip) have been changed. 
   * @param width new width
   * @param height new height
   */
  NS_IMETHOD  SetDimensions(gfx_dimension width, gfx_dimension height, PRBool aPaint = PR_TRUE) = 0;
  NS_IMETHOD  GetDimensions(gfx_dimension *width, gfx_dimension *height) const = 0;

  /**
   * Called to indicate that the dimensions and position of the view have
   * been changed.
   * @param aBounds new bounds
   */
  NS_IMETHOD  SetBounds(const nsRect &aBounds, PRBool aPaint = PR_TRUE) = 0;

  /**
   * Called to indicate that the dimensions and position of the view have
   * been changed.
   * @param aX new x position
   * @param aY new y position
   * @param aWidth new width
   * @param aHeight new height
   */
  NS_IMETHOD  SetBounds(gfx_coord aX, gfx_coord aY,
                        gfx_coord aWidth, gfx_coord aHeight,
                        PRBool aPaint = PR_TRUE) = 0;

  /**
   * Called to get the dimensions and position of the view.
   * @param aBounds out parameter for bounds
   */
  NS_IMETHOD  GetBounds(nsRect &aBounds) const = 0;

  /**
   * Called to set the clip of the children of this view.
   * The clip is relative to the origin of the view.
   * All of the children of this view will be clipped using
   * the specified rectangle
   * @param aLeft new left position
   * @param aTop new top position
   * @param aRight new right position
   * @param aBottom new bottom position
   */
  NS_IMETHOD  SetChildClip(gfx_coord aLeft, gfx_coord aTop, gfx_coord aRight, gfx_coord aBottom) = 0;

  /**
   * Called to get the dimensions and position of the clip for the view.
   * @param aLeft left position
   * @param aTop top position
   * @param aRight right position
   * @param aBottom bottom position
   */
  NS_IMETHOD  GetChildClip(gfx_coord *aLeft, gfx_coord *aTop, gfx_coord *aRight, gfx_coord *aBottom) const = 0;

  /**
   * Called to indicate that the visibility of a view has been
   * changed.
   * @param visibility new visibility state
   */
  NS_IMETHOD  SetVisibility(nsViewVisibility aVisibility) = 0;

  /**
   * Called to query the visibility state of a view.
   * @result current visibility state
   */
  NS_IMETHOD  GetVisibility(nsViewVisibility &aVisibility) const = 0;

  /**
   * Called to set the Z-order parent of the view. This is the
   * parent from which we derive our Z-order grouping. It might not
   * be the same as the geometric parent.
   * @param aParent new parent
   */
  NS_IMETHOD  SetZParent(nsIView *aZParent) = 0;
  NS_IMETHOD  GetZParent(nsIView *&aZParent) const = 0;

  /**
   * Called to indicate that the z-index of a view has been changed.
   * The z-index is relative to all siblings of the view.
   * @param zindex new z depth
   */
  NS_IMETHOD  SetZIndex(PRInt32 aZIndex) = 0;

  /**
   * Called to query the z-index of a view.
   * The z-index is relative to all siblings of the view.
   * @result current z depth
   */
  NS_IMETHOD  GetZIndex(PRInt32 &aZIndex) const = 0;

  /**
   * Indicate that the z-index of a view is "auto". An "auto" z-index
   * means that the view does not define a new stacking context,
   * which means that the z-indicies of the view's children are
   * relative to the view's siblings.
   * @param aAutoZIndex if true then z-index will be auto
   */
  NS_IMETHOD  SetAutoZIndex(PRBool aAutoZIndex) = 0;

  /**
   * Returns true if an auto z-index is set for this view.
   * @result current state of auto z-indexing
   */
  NS_IMETHOD  GetAutoZIndex(PRBool &aAutoZIndex) const = 0;

  /**
   * Set/Get whether the view "floats" above all other views,
   * which tells the compositor not to consider higher views in
   * the view hierarchy that would geometrically intersect with
   * this view. This is a hack, but it fixes some problems with
   * views that need to be drawn in front of all other views.
   * @result PR_TRUE if the view floats, PR_FALSE otherwise.
   */
  NS_IMETHOD SetFloating(PRBool aFloatingView) = 0;
  NS_IMETHOD GetFloating(PRBool &aFloatingView) const = 0;

  /**
   * Called to set the parent of the view. This is the geometric parent
   * (from which we derive our coordinate system).
   * @param aParent new parent
   */
  NS_IMETHOD  SetParent(nsIView *aParent) = 0;

  /**
   * Called to query the parent of the view.
   * @result view's parent
   */
  NS_IMETHOD  GetParent(nsIView *&aParent) const = 0;

  /**
   * Called to query the next sibling of the view.
   * @result view's next sibling
   */
  NS_IMETHOD  GetNextSibling(nsIView *&aNextSibling) const = 0;

  /**
   * Called to set the next sibling of the view.
   * @param aNextSibling new next sibling
   */
  NS_IMETHOD  SetNextSibling(nsIView* aNextSibling) = 0;

  /**
   * Used to insert a child after the specified sibling. In general,
   * child insertion will happen through the view manager and it
   * will determine the ordering of children in the child list.
   * @param child to insert in this view's child list
   * @param sibling view to set as previous sibling of child
   *                if nsnull, then child is inserted at head of list
   */
  NS_IMETHOD  InsertChild(nsIView *aChild, nsIView *aSibling) = 0;

  /**
   * Remove a child from the child list. The removal will be driven
   * through the view manager.
   * @param child to remove
   */
  NS_IMETHOD  RemoveChild(nsIView *aChild) = 0;
  
  /**
   * Get the number of children for this view.
   * @result child count
   */
  NS_IMETHOD  GetChildCount(PRInt32 &aCount) const = 0;
  
  /**
   * Get a child at a specific index. Could be replaced by some sort of
   * enumeration API.
   * @param index of desired child view
   * @result the view at index or nsnull if there is no such child
   */
  NS_IMETHOD  GetChild(PRInt32 index, nsIView*& aChild) const = 0;

  /**
   * Note: This didn't exist in 4.0. This transform might include scaling
   * but probably not rotation for the first pass.
   * @param transform new transformation of view
   */
  NS_IMETHOD  SetTransform(nsTransform2D &aXForm) = 0;

  /**
   * Note: This didn't exist in 4.0. This transform might include scaling
   * but probably not rotation for the first pass.
   * @result view's transformation
   */
  NS_IMETHOD  GetTransform(nsTransform2D &aXForm) const = 0;

  /**
   * Note: This didn't exist in 4.0. Called to set the opacity of a view. 
   * A value of 0.0 means completely transparent. A value of 1.0 means
   * completely opaque.
   * @param opacity new opacity value
   */
  NS_IMETHOD  SetOpacity(float aOpacity) = 0;

  /**
   * Note: This didn't exist in 4.0. Called to set the opacity of a view. 
   * A value of 0.0 means completely transparent. A value of 1.0 means
   * completely opaque.
   * @result view's opacity value
   */
  NS_IMETHOD  GetOpacity(float &aOpacity) const = 0;

  /**
   * Used to ask a view if it has any areas within its bounding box
   * that are transparent. This is not the same as opacity - opacity can
   * be set externally, transparency is a quality of the view itself.
   * @result Returns PR_TRUE if there are transparent areas, PR_FALSE otherwise.
   */
  NS_IMETHOD  HasTransparency(PRBool &aTransparent) const = 0;

  /**
   * Used set the transparency status of the content in a view. see
   * HasTransparency().
   * @param aTransparent PR_TRUE if there are transparent areas, PR_FALSE otherwise.
   */
  NS_IMETHOD  SetContentTransparency(PRBool aTransparent) = 0;

  /**
   * Set the view's link to client owned data.
   * @param aData - data to associate with view. nsnull to disassociate
   */
  NS_IMETHOD  SetClientData(void *aData) = 0;

  /**
   * Query the view for it's link to client owned data.
   * @result data associated with view or nsnull if there is none.
   */
  NS_IMETHOD  GetClientData(void *&aData) const = 0;

  /**
   * Get the nearest widget in this view or a parent of this view and
   * the offset from the view that contains the widget to this view
   * @param aDx out parameter for x offset
   * @param aDy out parameter for y offset
   * @return widget (if there is one) closest to view
   */
  NS_IMETHOD  GetOffsetFromWidget(gfx_coord *aDx, gfx_coord *aDy, nsIWindow *aWindow) = 0;

  /**
   * Gets the dirty region associated with this view. Used by the view
   * manager.
   */
  NS_IMETHOD GetDirtyRegion(nsIRegion *&aRegion) const = 0;

  /**
   * Create a widget to associate with this view. This is a helper
   * function for SetWidget.
   * @param aWindowIID IID for Widget type that this view
   *        should have associated with it. if nsull, then no
   *        width will be created for this view
   * @param aWidgetInitData data used to initialize this view's widget before
   *        its create is called.
   * @param aNative native window that will be used as parent of
   *        aWindowIID. if nsnull, then parent will be derived from
   *        parent view and it's ancestors
   * @return error status
   */
  NS_IMETHOD CreateWidget(const char *) = 0;

  /**
   * Set the widget associated with this view.
   * @param aWidget widget to associate with view. It is an error
   *        to associate a widget with more than one view. To disassociate
   *        a widget from a view, use nsnull. If there are no more references
   *        to the widget that may have been associated with the view, it will
   *        be destroyed.
   * @return error status
   */
  NS_IMETHOD SetWidget(nsIWindow *aWidget) = 0;

  /**
   * In 4.0, the "cutout" nature of a view is queryable.
   * If we believe that all cutout view have a native widget, this
   * could be a replacement.
   * @param aWidget out parameter for widget that this view contains,
   *        or nsnull if there is none.
   */
  NS_IMETHOD GetWidget(nsIWindow **aWindow) const = 0;


  /**
   * Returns PR_TRUE if the view has a widget associated with it.
   * @param aHasWidget out parameter that indicates whether a view has a widget.
   */
  NS_IMETHOD HasWidget(PRBool *aHasWidget) const = 0;

  /**
   * Output debug info to FILE
   * @param out output file handle
   * @param aIndent indentation depth
   */
  NS_IMETHOD List(FILE* out = stdout, PRInt32 aIndent = 0) const = 0;

  /**
   * Set flags on view to allow customization of view behavior during
   * event handling
   * @param aFlags flags to be added to view
   */
  NS_IMETHOD SetViewFlags(PRUint32 aFlags) = 0;

  /**
   * Remove flags from view to allow customization of view behavior during
   * event handling
   * @param aFlags flags to be removed from view
   */
  NS_IMETHOD ClearViewFlags(PRUint32 aFlags) = 0;

  /**
   * Get flags on view to allow customization of view behavior during
   * event handling
   * @param aFlags out parameter for view flags
   */
  NS_IMETHOD GetViewFlags(PRUint32 *aFlags) const = 0;

  /**
   * Get pointer to temporary data storage used by
   * the compositor. make no assumptions about the
   * data returned by this method. oh yeah, and it's a hack.
   * @param aPoint out paramemter for nsPoint structure
   *        stored in view.
   * @return error status
   */
  NS_IMETHOD GetScratchPoint(nsPoint **aPoint) = 0;
  
  /**
   * Used by the compositor for temporary marking of a view during
   * compositing. This will eventually replace GetScratchPoint above.
   */
  NS_IMETHOD SetCompositorFlags(PRUint32 aFlags) = 0;
  NS_IMETHOD GetCompositorFlags(PRUint32 *aFlags) = 0;

  /**
   * Get the extents of the view tree from 'this' down.
   * 'this' is the coordinate system origin.
   * @param aExtents out paramemter for extents
   * @return error status
   */
  NS_IMETHOD GetExtents(nsRect *aExtents) = 0;

  // XXX Temporary for Bug #19416
  NS_IMETHOD IgnoreSetPosition(PRBool aShouldIgnore) = 0;

  /**
   * Sync your widget size and position with the view
   */
  NS_IMETHOD SynchWidgetSizePosition() = 0;

  /**
   * Return a rectangle containing the view's bounds adjusted for it's ancestors clipping
   * @param aClippedRect views bounds adjusted for ancestors clipping. If aEmpty is TRUE it
   * aClippedRect is set to an empty rect.
   * @param aIsClipped returns with PR_TRUE if view's rectangle is clipped by an ancestor
   * @param aEmpty returns with PR_TRUE if view's rectangle is 'clipped out'
   */
  NS_IMETHOD GetClippedRect(nsRect& aClippedRect, PRBool& aIsClipped, PRBool& aEmpty) const = 0;


private:
  NS_IMETHOD_(nsrefcnt) AddRef(void) = 0;
  NS_IMETHOD_(nsrefcnt) Release(void) = 0;
};

//this is passed down to child views during painting and event handling
//so that a child can determine if it is hidden or shown when it's
//visibility state is set to inherit
#define NS_VIEW_FLAG_PARENT_HIDDEN  0x0001

//when painting, if we have determined that we need to do a combination
//of front to back and back to front painting, this flag will be set
//while in the back to front pass
#define NS_VIEW_FLAG_BACK_TO_FRONT  0x0002

//during event propagation, see if parent views can handle the event
#define NS_VIEW_FLAG_CHECK_PARENT   0x0004

//during event propagation, see if child views can handle the event
#define NS_VIEW_FLAG_CHECK_CHILDREN 0x0008

//during event propagation, see if sibling views can handle the event
#define NS_VIEW_FLAG_CHECK_SIBLINGS 0x0010

//passed down through the class hierarchy
//to indicate that the clip is set by an
//outer class
#define NS_VIEW_FLAG_CLIP_SET       0x0020

//when painting, if we have determined that we need to do a combination
//of front to back and back to front painting, this flag will be set
//while in the front to back pass
#define NS_VIEW_FLAG_FRONT_TO_BACK  0x0040

//temporary hack so that michael can work on the new
//compositor and make checkins without busting the rest
//of the world.
#define NS_VIEW_FLAG_JUST_PAINT     0x0080

//the following are public flags accessed through the *ViewFlags methods.

//Flag to determine whether the view will check if events can be handled
//by its children or just handle the events itself
#define NS_VIEW_PUBLIC_FLAG_DONT_CHECK_CHILDREN  0x0001
//the view is dying.
#define NS_VIEW_PUBLIC_FLAG_DYING                0x0002
//the view is transparent
#define NS_VIEW_PUBLIC_FLAG_TRANSPARENT          0x0004
//indicates that a view should not zoom values to/from widgets
#define NS_VIEW_PUBLIC_FLAG_DONT_ZOOM            0x0008
//indicates that the view should not be bitblt'd when moved
//or scrolled and instead must be repainted
#define NS_VIEW_PUBLIC_FLAG_DONT_BITBLT          0x0010
// indicates that the view is using auto z-indexing
#define NS_VIEW_PUBLIC_FLAG_AUTO_ZINDEX          0x0020
// indicatest hat the view is a floating view.
#define NS_VIEW_PUBLIC_FLAG_FLOATING             0x0040

// set if our widget resized. 
#define NS_VIEW_PUBLIC_FLAG_WIDGET_RESIZED       0x0080
// set if our widget moved. 
#define NS_VIEW_PUBLIC_FLAG_WIDGET_MOVED         0x0100

// indicates that the view should clip its child views using ClipRect specified 
// by SetClip
#define NS_VIEW_PUBLIC_FLAG_CLIPCHILDREN         0x0200

#endif
