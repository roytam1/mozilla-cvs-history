/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code mozilla.org code.
 *
 * The Initial Developer of the Original Code Christopher Blizzard
 * <blizzard@mozilla.org>.  Portions created by the Initial Developer
 * are Copyright (C) 2001 the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "nsWindow.h"
#include "nsToolkit.h"
#include "nsIRenderingContext.h"
#include "nsIRegion.h"
#include "nsIRollupListener.h"
#include "nsIMenuRollup.h"

#include "nsGtkKeyUtils.h"

#include <gtk/gtkwindow.h>
#include <gdk/gdkx.h>
#include <gdk/gdkkeysyms.h>

/* utility functions */
static PRBool     check_for_rollup(GdkWindow *aWindow,
				   gdouble aMouseX, gdouble aMouseY,
				   PRBool aIsWheel);
static PRBool     is_mouse_in_window(GdkWindow* aWindow,
				     gdouble aMouseX, gdouble aMouseY);
static nsWindow  *get_window_for_gtk_widget(GtkWidget *widget);
static nsWindow  *get_window_for_gdk_window(GdkWindow *window);
static nsWindow  *get_owning_window_for_gdk_window(GdkWindow *window);
static GtkWidget *get_gtk_widget_for_gdk_window(GdkWindow *window);

/* callbacks from widgets */
static gboolean expose_event_cb           (GtkWidget *widget,
					   GdkEventExpose *event);
static gboolean configure_event_cb        (GtkWidget *widget,
					   GdkEventConfigure *event);
static void     size_allocate_cb          (GtkWidget *widget,
					   GtkAllocation *allocation);
static gboolean delete_event_cb           (GtkWidget *widget,
					   GdkEventAny *event);
static gboolean enter_notify_event_cb     (GtkWidget *widget,
					   GdkEventCrossing *event);
static gboolean leave_notify_event_cb     (GtkWidget *widget,
					   GdkEventCrossing *event);
static gboolean motion_notify_event_cb    (GtkWidget *widget,
					   GdkEventMotion *event);
static gboolean button_press_event_cb     (GtkWidget *widget,
					   GdkEventButton *event);
static gboolean button_release_event_cb   (GtkWidget *widget,
					   GdkEventButton *event);
static gboolean focus_in_event_cb         (GtkWidget *widget,
					   GdkEventFocus *event);
static gboolean focus_out_event_cb        (GtkWidget *widget,
					   GdkEventFocus *event);
static gboolean key_press_event_cb        (GtkWidget *widget,
					   GdkEventKey *event);
static gboolean key_release_event_cb      (GtkWidget *widget,
					   GdkEventKey *event);
static gboolean scroll_event_cb           (GtkWidget *widget,
					   GdkEventScroll *event);
static gboolean visibility_notify_event_cb(GtkWidget *widget,
					   GdkEventVisibility *event);

static PRBool                 gJustGotActivate = PR_FALSE;
nsCOMPtr  <nsIRollupListener> gRollupListener;
nsWeakPtr                     gRollupWindow;

nsWindow::nsWindow()
{
  mFocusChild          = nsnull;
  mContainer           = nsnull;
  mDrawingarea         = nsnull;
  mShell               = nsnull;
  mWindowGroup         = nsnull;
  mContainerGotFocus   = PR_FALSE;
  mContainerLostFocus  = PR_FALSE;
  mContainerBlockFocus = PR_FALSE;
  mHasFocus            = PR_FALSE;
  mInKeyRepeat         = PR_FALSE;
  mIsVisible           = PR_FALSE;
  mRetryPointerGrab    = PR_FALSE;
  mRetryKeyboardGrab   = PR_FALSE;
  mTransientParent     = nsnull;
  mWindowType          = eWindowType_child;
}

nsWindow::~nsWindow()
{
  LOG(("nsWindow::~nsWindow() [%p]\n", (void *)this));
  Destroy();
}

NS_IMPL_ISUPPORTS_INHERITED1(nsWindow, nsCommonWidget,
			     nsISupportsWeakReference)

NS_IMETHODIMP
nsWindow::Create(nsIWidget        *aParent,
		 const nsRect     &aRect,
		 EVENT_CALLBACK   aHandleEventFunction,
		 nsIDeviceContext *aContext,
		 nsIAppShell      *aAppShell,
		 nsIToolkit       *aToolkit,
		 nsWidgetInitData *aInitData)
{
  return NativeCreate(aParent, nsnull, aRect, aHandleEventFunction,
		      aContext, aAppShell, aToolkit, aInitData);
}

NS_IMETHODIMP
nsWindow::Create(nsNativeWidget aParent,
		 const nsRect     &aRect,
		 EVENT_CALLBACK   aHandleEventFunction,
		 nsIDeviceContext *aContext,
		 nsIAppShell      *aAppShell,
		 nsIToolkit       *aToolkit,
		 nsWidgetInitData *aInitData)
{
  return NativeCreate(nsnull, aParent, aRect, aHandleEventFunction,
		      aContext, aAppShell, aToolkit, aInitData);
}

NS_IMETHODIMP
nsWindow::Destroy(void)
{
  if (mIsDestroyed)
    return NS_OK;

  LOG(("nsWindow::Destroy [%p]\n", (void *)this));
  mIsDestroyed = PR_TRUE;

  // ungrab if required
  nsCOMPtr<nsIWidget> rollupWidget = do_QueryReferent(gRollupWindow);
  if (NS_STATIC_CAST(nsIWidget *, this) == rollupWidget.get()) {
    if (gRollupListener)
      gRollupListener->Rollup();
    gRollupWindow = nsnull;
    gRollupListener = nsnull;
  }

  NativeShow(PR_FALSE);

  // walk the list of children and call destroy on them.
  nsCOMPtr<nsIEnumerator> children = dont_AddRef(GetChildren());
  if (children) {
    nsCOMPtr<nsISupports> isupp;
    nsCOMPtr<nsIWidget> child;
    while (NS_SUCCEEDED(children->CurrentItem(getter_AddRefs(isupp))
			&& isupp)) {
      child = do_QueryInterface(isupp);
      if (child) {
	child->Destroy();
      }

      if (NS_FAILED(children->Next()))
        break;
    }
  }

  // make sure that we remove ourself as the focus window
  if (mHasFocus) {
    LOG(("automatically losing focus...\n"));
    mHasFocus = PR_FALSE;
    // get the owning gtk widget and the nsWindow for that widget and
    // remove ourselves as the focus widget tracked in that window
    nsWindow *owningWindow =
      get_owning_window_for_gdk_window(mDrawingarea->inner_window);
    owningWindow->mFocusChild = nsnull;
  }

  // Remove our reference to the window group.  If there was a window
  // group destroying the widget will have automatically unreferenced
  // the group, destroying it if necessary.  And, if we're a child
  // window this isn't going to harm anything.
  mWindowGroup = nsnull;

  if (mShell) {
    gtk_widget_destroy(mShell);
    mShell = nsnull;
  }

  mContainer = nsnull;

  if (mDrawingarea) {
    g_object_unref(mDrawingarea);
    mDrawingarea = nsnull;
  }

  OnDestroy();

  return NS_OK;
}

NS_IMETHODIMP
nsWindow::SetModal(PRBool aModal)
{
  LOG(("nsWindow::SetModal [%p] %d\n", (void *)this, aModal));

  // find the toplevel window and add it to the grab list
  GtkWidget *grabWidget = nsnull;

  GetToplevelWidget(&grabWidget);
  
  if (!grabWidget)
    return NS_ERROR_FAILURE;

  if (aModal)
    gtk_grab_add(grabWidget);
  else
    gtk_grab_remove(grabWidget);

  return NS_OK;
}

NS_IMETHODIMP
nsWindow::IsVisible(PRBool & aState)
{
  aState = mIsVisible;
  return NS_OK;
}

NS_IMETHODIMP
nsWindow::ConstrainPosition(PRBool aAllowSlop, PRInt32 *aX, PRInt32 *aY)
{
  return NS_ERROR_NOT_IMPLEMENTED; 
}

NS_IMETHODIMP
nsWindow::Move(PRInt32 aX, PRInt32 aY)
{
  if (aX == mBounds.x && aY == mBounds.y)
    return NS_OK;

  LOG(("nsWindow::Move [%p] %d %d\n", (void *)this,
       aX, aY));

  mBounds.x = aX;
  mBounds.y = aY;

  if (mIsTopLevel) {
    if (mParent && mWindowType == eWindowType_popup) {
      nsRect oldrect, newrect;
      oldrect.x = aX;
      oldrect.y = aY;
      mParent->WidgetToScreen(oldrect, newrect);
      gtk_window_move(GTK_WINDOW(mShell), newrect.x, newrect.y);
    }
    else {
      gtk_window_move(GTK_WINDOW(mShell), aX, aY);
    }
  }
  else {
    moz_drawingarea_move(mDrawingarea, aX, aY);
  }
  
  return NS_OK;
}

NS_IMETHODIMP
nsWindow::PlaceBehind(nsIWidget *aWidget,
		      PRBool     aActivate)
{
  return NS_ERROR_NOT_IMPLEMENTED; 
}

NS_IMETHODIMP
nsWindow::Enable(PRBool aState)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsWindow::SetFocus(PRBool aRaise)
{
  // Make sure that our owning widget has focus.  If it doesn't try to
  // grab it.  Note that we don't set our focus flag in this case.
  
  LOG(("SetFocus [%p]\n", (void *)this));

  GtkWidget *owningWidget =
    get_gtk_widget_for_gdk_window(mDrawingarea->inner_window);
  if (!owningWidget)
    return NS_ERROR_FAILURE;

  nsWindow  *owningWindow = get_window_for_gtk_widget(owningWidget);
  if (!owningWindow)
    return NS_ERROR_FAILURE;

  if (!GTK_WIDGET_HAS_FOCUS(owningWidget)) {

    LOG(("grabbing focus for the toplevel\n"));
    owningWindow->mContainerBlockFocus = PR_TRUE;
    gtk_widget_grab_focus(owningWidget);
    owningWindow->mContainerBlockFocus = PR_FALSE;

    DispatchGotFocusEvent();

    // unset the activate flag
    if (gJustGotActivate) {
      gJustGotActivate = PR_FALSE;
      DispatchActivateEvent();
    }

    return NS_OK;
  }

  // Raise the window if someone passed in PR_TRUE and the prefs are
  // set properly.
  // XXX do this

  // If this is the widget that already has focus, return.
  if (mHasFocus) {
    LOG(("already have focus...\n"));
    return NS_OK;
  }
  
  // If there is already a focued child window, dispatch a LOSTFOCUS
  // event from that widget and unset its got focus flag.
  if (owningWindow->mFocusChild) {
    LOG(("removing focus child %p\n", (void *)owningWindow->mFocusChild));
    owningWindow->mFocusChild->LoseFocus();
  }

  // Set this window to be the focused child window, update our has
  // focus flag and dispatch a GOTFOCUS event.
  owningWindow->mFocusChild = this;
  mHasFocus = PR_TRUE;

  DispatchGotFocusEvent();

  // make sure to unset the activate flag and send an activate event
  if (gJustGotActivate) {
    gJustGotActivate = PR_FALSE;
    DispatchActivateEvent();
  }
  
  return NS_OK;
}

NS_IMETHODIMP
nsWindow::GetScreenBounds(nsRect &aRect)
{
  nsRect origin(0, 0, mBounds.width, mBounds.height);
  WidgetToScreen(origin, aRect);
  LOG(("GetScreenBounds %d %d | %d %d | %d %d\n",
       aRect.x, aRect.y,
       mBounds.width, mBounds.height,
       aRect.width, aRect.height));
  return NS_OK;
}

NS_IMETHODIMP
nsWindow::SetForegroundColor(const nscolor &aColor)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsWindow::SetBackgroundColor(const nscolor &aColor)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

nsIFontMetrics*
nsWindow::GetFont(void)
{
  return nsnull;
}

NS_IMETHODIMP
nsWindow::SetFont(const nsFont &aFont)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsWindow::SetCursor(nsCursor aCursor)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsWindow::Validate()
{
  // Get the update for this window and, well, just drop it on the
  // floor.
  GdkRegion *region = gdk_window_get_update_area(mDrawingarea->inner_window);

  if (region)
    gdk_region_destroy(region);

  return NS_OK;
}

NS_IMETHODIMP
nsWindow::Invalidate(PRBool aIsSynchronous)
{
  GdkRectangle rect;

  rect.x = mBounds.x;
  rect.y = mBounds.y;
  rect.width = mBounds.width;
  rect.height = mBounds.height;

  gdk_window_invalidate_rect(mDrawingarea->inner_window,
			     &rect, TRUE);
  if (aIsSynchronous)
    gdk_window_process_updates(mDrawingarea->inner_window, TRUE);
  
  return NS_OK;
}

NS_IMETHODIMP
nsWindow::Invalidate(const nsRect &aRect,
		     PRBool        aIsSynchronous)
{
  GdkRectangle rect;

  rect.x = aRect.x;
  rect.y = aRect.y;
  rect.width = aRect.width;
  rect.height = aRect.height;

  gdk_window_invalidate_rect(mDrawingarea->inner_window,
			     &rect, TRUE);
  if (aIsSynchronous)
    gdk_window_process_updates(mDrawingarea->inner_window, TRUE);

  return NS_OK;
}

NS_IMETHODIMP
nsWindow::InvalidateRegion(const nsIRegion* aRegion,
			   PRBool           aIsSynchronous)
{
  GdkRegion *region = nsnull;
  aRegion->GetNativeRegion((void *)region);

  if (region)
    gdk_window_invalidate_region(mDrawingarea->inner_window,
				 region, TRUE);
  
  return NS_OK;
}

NS_IMETHODIMP
nsWindow::Update()
{
  gdk_window_process_updates(mDrawingarea->inner_window, TRUE);
  return NS_OK;
}

NS_IMETHODIMP
nsWindow::SetColorMap(nsColorMap *aColorMap)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsWindow::Scroll(PRInt32  aDx,
		 PRInt32  aDy,
		 nsRect  *aClipRect)
{
  moz_drawingarea_scroll(mDrawingarea, aDx, aDy);

  // Update bounds on our child windows
  nsCOMPtr<nsIEnumerator> children = dont_AddRef(GetChildren());
  if (children) {
    nsCOMPtr<nsISupports> isupp;
    nsCOMPtr<nsIWidget> child;
    while (NS_SUCCEEDED(children->CurrentItem(getter_AddRefs(isupp))
			&& isupp)) {
      child = do_QueryInterface(isupp);
      if (child) {
        nsRect bounds;
        child->GetBounds(bounds);
        bounds.x += aDx;
        bounds.y += aDy;
        NS_STATIC_CAST(nsBaseWidget*, (nsIWidget*)child)->SetBounds(bounds);
      }

      if (NS_FAILED(children->Next()))
        break;
    }
  }

  // Process all updates so that everything is drawn.
  gdk_window_process_all_updates();
  return NS_OK;
}

NS_IMETHODIMP
nsWindow::ScrollWidgets(PRInt32 aDx,
			PRInt32 aDy)
{
  moz_drawingarea_scroll(mDrawingarea, aDx, aDy);
  return NS_OK;
}

NS_IMETHODIMP
nsWindow::ScrollRect(nsRect  &aSrcRect,
		     PRInt32  aDx,
		     PRInt32  aDy)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

void*
nsWindow::GetNativeData(PRUint32 aDataType)
{
  switch (aDataType) {
  case NS_NATIVE_WINDOW:
  case NS_NATIVE_WIDGET:
    return mDrawingarea->inner_window;
    break;

  case NS_NATIVE_PLUGIN_PORT:
    NS_WARNING("nsWindow::GetNativeData plugin port not supported yet");
    return nsnull;
    break;

  case NS_NATIVE_DISPLAY:
    return GDK_DISPLAY();
    break;

  case NS_NATIVE_GRAPHIC:
    NS_ASSERTION(nsnull != mToolkit, "NULL toolkit, unable to get a GC");
    return (void *)NS_STATIC_CAST(nsToolkit *, mToolkit)->GetSharedGC();
    break;

  default:
    NS_WARNING("nsWindow::GetNativeData called with bad value");
    return nsnull;
  }
}

NS_IMETHODIMP
nsWindow::SetBorderStyle(nsBorderStyle aBorderStyle)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsWindow::SetTitle(const nsString& aTitle)
{
  if (!mShell)
    return NS_OK;

  // convert the string into utf8 and set the title.
  NS_ConvertUCS2toUTF8 utf8title(aTitle);
  gtk_window_set_title(GTK_WINDOW(mShell), (const char *)utf8title.get());

  return NS_OK;
}

NS_IMETHODIMP
nsWindow::SetIcon(const nsAReadableString& anIconSpec)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsWindow::SetMenuBar(nsIMenuBar * aMenuBar)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsWindow::ShowMenuBar(PRBool aShow)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsWindow::WidgetToScreen(const nsRect& aOldRect, nsRect& aNewRect)
{
  gint x, y = 0;

  if (mContainer) {
    gdk_window_get_root_origin(GTK_WIDGET(mContainer)->window,
			       &x, &y);
    LOG(("WidgetToScreen (container) %d %d\n", x, y));
  }
  else {
    gdk_window_get_origin(mDrawingarea->inner_window, &x, &y);
    LOG(("WidgetToScreen (drawing) %d %d\n", x, y));
  }

  aNewRect.x = x + aOldRect.x;
  aNewRect.y = y + aOldRect.y;
  aNewRect.width = aOldRect.width;
  aNewRect.height = aOldRect.height;

  return NS_OK;
}

NS_IMETHODIMP
nsWindow::ScreenToWidget(const nsRect& aOldRect, nsRect& aNewRect)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsWindow::BeginResizingChildren(void)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsWindow::EndResizingChildren(void)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsWindow::EnableDragDrop(PRBool aEnable)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}
   
void
nsWindow::ConvertToDeviceCoordinates(nscoord &aX,
				     nscoord &aY)
{
}

NS_IMETHODIMP
nsWindow::PreCreateWidget(nsWidgetInitData *aWidgetInitData)
{
  if (nsnull != aWidgetInitData) {
    mWindowType = aWidgetInitData->mWindowType;
    mBorderStyle = aWidgetInitData->mBorderStyle;
    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}
  
NS_IMETHODIMP
nsWindow::CaptureMouse(PRBool aCapture)
{
  LOG(("CaptureMouse %p\n", (void *)this));
  
  GtkWidget *widget = 
    get_gtk_widget_for_gdk_window(mDrawingarea->inner_window);

  if (aCapture) {
    gtk_grab_add(widget);
    GrabPointer();
  }
  else {
    ReleaseGrabs();
    gtk_grab_remove(widget);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsWindow::CaptureRollupEvents(nsIRollupListener *aListener,
			      PRBool             aDoCapture,
			      PRBool             aConsumeRollupEvent)
{
  GtkWidget *widget = 
    get_gtk_widget_for_gdk_window(mDrawingarea->inner_window);

  LOG(("CaptureRollupEvents %p\n", (void *)this));

  if (aDoCapture) {
    gRollupListener = aListener;
    gRollupWindow =
      getter_AddRefs(NS_GetWeakReference(NS_STATIC_CAST(nsIWidget*,this)));
    gtk_grab_add(widget);
    GrabPointer();
    GrabKeyboard();
  }
  else {
    ReleaseGrabs();
    gtk_grab_remove(widget);
    gRollupListener = nsnull;
    gRollupWindow = nsnull;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsWindow::GetAttention()
{
  LOG(("nsWindow::GetAttention [%p]\n", (void *)this));
  return NS_OK;
}

void
nsWindow::LoseFocus(void)
{
  // we don't have focus
  mHasFocus = PR_FALSE;

  // make sure that we reset our repeat counter so the next keypress
  // for this widget will get the down event
  mInKeyRepeat = PR_FALSE;

  // Dispatch a lostfocus event
  DispatchLostFocusEvent();
}

gboolean
nsWindow::OnExposeEvent(GtkWidget *aWidget, GdkEventExpose *aEvent)
{
  if (mIsDestroyed) {
    LOG(("Expose event on destroyed window [%p] window %p\n",
	 (void *)this, (void *)aEvent->window));
    return NS_OK;
  }

  // handle exposes for the inner window only
  if (aEvent->window != mDrawingarea->inner_window)
    return FALSE;

  LOG(("sending expose event [%p] %p 0x%lx\n\t%d %d %d %d\n",
       (void *)this,
       (void *)aEvent->window,
       GDK_WINDOW_XWINDOW(aEvent->window),
       aEvent->area.x, aEvent->area.y,
       aEvent->area.width, aEvent->area.height));

  // ok, send out the paint event
  // XXX figure out the region/rect stuff!
  nsRect rect(aEvent->area.x, aEvent->area.y,
	      aEvent->area.width, aEvent->area.height);
  nsPaintEvent event;

  InitPaintEvent(event);

  event.point.x = aEvent->area.x;
  event.point.y = aEvent->area.y;
  event.rect = &rect;
  // XXX fix this!
  event.region = nsnull;
  // XXX fix this!
  event.renderingContext = GetRenderingContext();

  nsEventStatus status;
  DispatchEvent(&event, status);

  NS_RELEASE(event.renderingContext);

  // check the return value!
  return TRUE;
}

gboolean
nsWindow::OnConfigureEvent(GtkWidget *aWidget, GdkEventConfigure *aEvent)
{
  LOG(("configure event [%p] %d %d %d %d\n", (void *)this,
       aEvent->x, aEvent->y, aEvent->width, aEvent->height));

  // can we shortcut?
  if (mBounds.x == aEvent->x &&
      mBounds.y == aEvent->y)
    return FALSE;

  nsGUIEvent event;
  InitGUIEvent(event, NS_MOVE);

  event.point.x = aEvent->x;
  event.point.y = aEvent->y;

  // XXX mozilla will invalidate the entire window after this move
  // complete.  wtf?
  nsEventStatus status;
  DispatchEvent(&event, status);

  return FALSE;
}

void
nsWindow::OnSizeAllocate(GtkWidget *aWidget, GtkAllocation *aAllocation)
{
  LOG(("size_allocate [%p] %d %d %d %d\n",
       (void *)this, aAllocation->x, aAllocation->y,
       aAllocation->width, aAllocation->height));
  
  nsRect rect(aAllocation->x, aAllocation->y,
	      aAllocation->width, aAllocation->height);

  mBounds.width = rect.width;
  mBounds.height = rect.height;

  moz_drawingarea_resize (mDrawingarea, rect.width, rect.height);

  nsEventStatus status;
  DispatchResizeEvent (rect, status);
}

void
nsWindow::OnDeleteEvent(GtkWidget *aWidget, GdkEventAny *aEvent)
{
  nsGUIEvent event;

  InitGUIEvent(event, NS_XUL_CLOSE);

  event.point.x = 0;
  event.point.y = 0;

  nsEventStatus status;
  DispatchEvent(&event, status);
}

void
nsWindow::OnEnterNotifyEvent(GtkWidget *aWidget, GdkEventCrossing *aEvent)
{
  nsMouseEvent event;
  InitMouseEvent(event, NS_MOUSE_ENTER);

  event.point.x = nscoord(aEvent->x);
  event.point.y = nscoord(aEvent->y);

  nsEventStatus status;
  DispatchEvent(&event, status);
}

void
nsWindow::OnLeaveNotifyEvent(GtkWidget *aWidget, GdkEventCrossing *aEvent)
{
  nsMouseEvent event;
  InitMouseEvent(event, NS_MOUSE_EXIT);

  event.point.x = nscoord(aEvent->x);
  event.point.y = nscoord(aEvent->y);

  nsEventStatus status;
  DispatchEvent(&event, status);
}

void
nsWindow::OnMotionNotifyEvent(GtkWidget *aWidget, GdkEventMotion *aEvent)
{

  // see if we can compress this event
  XEvent xevent;
  PRPackedBool synthEvent = PR_FALSE;
  while (XCheckWindowEvent(GDK_WINDOW_XDISPLAY(aEvent->window),
			   GDK_WINDOW_XWINDOW(aEvent->window),
			   ButtonMotionMask, &xevent)) {
    synthEvent = PR_TRUE;
  }

  nsMouseEvent event;
  InitMouseEvent(event, NS_MOUSE_MOVE);
 
  if (synthEvent) {
    event.point.x = nscoord(xevent.xmotion.x);
    event.point.y = nscoord(xevent.xmotion.y);

    event.isShift   = (xevent.xmotion.state & GDK_SHIFT_MASK)
      ? PR_TRUE : PR_FALSE;
    event.isControl = (xevent.xmotion.state & GDK_CONTROL_MASK)
      ? PR_TRUE : PR_FALSE;
    event.isAlt     = (xevent.xmotion.state & GDK_MOD1_MASK)
      ? PR_TRUE : PR_FALSE;
  }
  else {
    event.point.x = nscoord(aEvent->x);
    event.point.y = nscoord(aEvent->y);

    event.isShift   = (aEvent->state & GDK_SHIFT_MASK)
      ? PR_TRUE : PR_FALSE;
    event.isControl = (aEvent->state & GDK_CONTROL_MASK)
      ? PR_TRUE : PR_FALSE;
    event.isAlt     = (aEvent->state & GDK_MOD1_MASK)
      ? PR_TRUE : PR_FALSE;
  }

  nsEventStatus status;
  DispatchEvent(&event, status);
}

void
nsWindow::OnButtonPressEvent(GtkWidget *aWidget, GdkEventButton *aEvent)
{
  nsMouseEvent  event;
  PRUint32      eventType;
  nsEventStatus status;

  // check to see if we should rollup
  if (check_for_rollup(aEvent->window, aEvent->x_root, aEvent->y_root,
		     PR_FALSE))
    return;

  switch (aEvent->button) {
  case 2:
    eventType = NS_MOUSE_MIDDLE_BUTTON_DOWN;
    break;
  case 3:
    eventType = NS_MOUSE_RIGHT_BUTTON_DOWN;
    break;
  default:
    eventType = NS_MOUSE_LEFT_BUTTON_DOWN;
    break;
  }

  InitButtonEvent(event, eventType, aEvent);
  
  DispatchEvent(&event, status);

  // right menu click on linux should also pop up a context menu
  if (eventType == NS_MOUSE_RIGHT_BUTTON_DOWN) {
    InitButtonEvent(event, NS_CONTEXTMENU, aEvent);
    DispatchEvent(&event, status);
  }
}

void
nsWindow::OnButtonReleaseEvent(GtkWidget *aWidget, GdkEventButton *aEvent)
{
  nsMouseEvent  event;
  PRUint32      eventType;

  switch (aEvent->button) {
  case 2:
    eventType = NS_MOUSE_MIDDLE_BUTTON_UP;
    break;
  case 3:
    eventType = NS_MOUSE_RIGHT_BUTTON_UP;
    break;
    // don't send events for these types
  case 4:
  case 5:
    return;
    break;
    // default including button 1 is left button up
  default:
    eventType = NS_MOUSE_LEFT_BUTTON_UP;
    break;
  }

  InitButtonEvent(event, eventType, aEvent);

  nsEventStatus status;
  DispatchEvent(&event, status);
}

void
nsWindow::OnContainerFocusInEvent(GtkWidget *aWidget, GdkEventFocus *aEvent)
{
  // Return if someone has blocked events for this widget.  This will
  // happen if someone has called gtk_widget_grab_focus() from
  // nsWindow::SetFocus() and will prevent recursion.
  if (mContainerBlockFocus)
    return;

  if (mIsTopLevel)
    gJustGotActivate = PR_TRUE;

  // dispatch a got focus event
  DispatchGotFocusEvent();
}

void
nsWindow::OnContainerFocusOutEvent(GtkWidget *aWidget, GdkEventFocus *aEvent)
{
  // send a lost focus event for the child window
  if (mFocusChild) {
    mFocusChild->LoseFocus();
    mFocusChild->DispatchDeactivateEvent();
    mFocusChild = nsnull;
  }
}

gboolean
nsWindow::OnKeyPressEvent(GtkWidget *aWidget, GdkEventKey *aEvent)
{
  // work around for annoying things.
  if (aEvent->keyval == GDK_Tab)
    if (aEvent->state & GDK_CONTROL_MASK)
      if (aEvent->state & GDK_MOD1_MASK)
        return FALSE;

  // Don't pass shift, control and alt as key press events
  if (aEvent->keyval == GDK_Shift_L
      || aEvent->keyval == GDK_Shift_R
      || aEvent->keyval == GDK_Control_L
      || aEvent->keyval == GDK_Control_R
      || aEvent->keyval == GDK_Alt_L
      || aEvent->keyval == GDK_Alt_R)
    return TRUE;


  // If the key repeat flag isn't set then set it so we don't send
  // another key down event on the next key press -- DOM events are
  // key down, key press and key up.  X only has key press and key
  // release.  gtk2 already filters the extra key release events for
  // us.
  nsEventStatus status;
  nsKeyEvent event;

  if (!mInKeyRepeat) {
    mInKeyRepeat = PR_TRUE;
    // send the key down event
    InitKeyEvent(event, aEvent, NS_KEY_DOWN);
    DispatchEvent(&event, status);
  }
  
  InitKeyEvent(event, aEvent, NS_KEY_PRESS);
  event.charCode = nsConvertCharCodeToUnicode(aEvent);
  if (event.charCode) {
    event.keyCode = 0;
    // if the control, meta, or alt key is down, then we should leave
    // the isShift flag alone (probably not a printable character)
    // if none of the other modifier keys are pressed then we need to
    // clear isShift so the character can be inserted in the editor
    if (!event.isControl && !event.isAlt && !event.isMeta)
      event.isShift = PR_FALSE;
  }

  // send the key press event
  DispatchEvent(&event, status);
  return TRUE;
}

gboolean
nsWindow::OnKeyReleaseEvent(GtkWidget *aWidget, GdkEventKey *aEvent)
{
  // unset the repeat flag
  mInKeyRepeat = PR_FALSE;

  // send the key event as a key up event
  // Don't pass shift, control and alt as key press events
  if (aEvent->keyval == GDK_Shift_L
      || aEvent->keyval == GDK_Shift_R
      || aEvent->keyval == GDK_Control_L
      || aEvent->keyval == GDK_Control_R
      || aEvent->keyval == GDK_Alt_L
      || aEvent->keyval == GDK_Alt_R)
    return TRUE;

  nsKeyEvent event;
  InitKeyEvent(event, aEvent, NS_KEY_UP);

  nsEventStatus status;
  DispatchEvent(&event, status);

  return TRUE;
}

void
nsWindow::OnScrollEvent(GtkWidget *aWidget, GdkEventScroll *aEvent)
{
  nsMouseScrollEvent event;
  InitMouseScrollEvent(event, aEvent, NS_MOUSE_SCROLL);

  // check to see if we should rollup
  if (check_for_rollup(aEvent->window, aEvent->x_root, aEvent->y_root,
		     PR_TRUE)) {
    printf("ignoring event\n");
    return;
  }

  nsEventStatus status;
  DispatchEvent(&event, status);
}

void
nsWindow::OnVisibilityNotifyEvent(GtkWidget *aWidget,
				  GdkEventVisibility *aEvent)
{
  switch (aEvent->state) {
  case GDK_VISIBILITY_UNOBSCURED:
  case GDK_VISIBILITY_PARTIAL:
    mIsVisible = PR_TRUE;
    // if we have to retry the grab, retry it.
    EnsureGrabs();
    break;
  default: // includes GDK_VISIBILITY_FULLY_OBSCURED
    mIsVisible = PR_FALSE;
    break;
  }
}

nsresult
nsWindow::NativeCreate(nsIWidget        *aParent,
		       nsNativeWidget    aNativeParent,
		       const nsRect     &aRect,
		       EVENT_CALLBACK    aHandleEventFunction,
		       nsIDeviceContext *aContext,
		       nsIAppShell      *aAppShell,
		       nsIToolkit       *aToolkit,
		       nsWidgetInitData *aInitData)
{
  // only set the base parent if we're going to be a dialog or a
  // toplevel
  nsIWidget *baseParent = aInitData &&
    (aInitData->mWindowType == eWindowType_dialog ||
     aInitData->mWindowType == eWindowType_toplevel) ?
    nsnull : aParent;

  // initialize all the common bits of this class
  BaseCreate(baseParent, aRect, aHandleEventFunction, aContext,
	     aAppShell, aToolkit, aInitData);
  
  // and do our common creation
  CommonCreate(aParent, aNativeParent);

  // save our bounds
  mBounds = aRect;

  // figure out our parent window
  MozDrawingarea *parentArea = nsnull;
  MozContainer   *parentContainer = nsnull;
  GtkWindow      *topLevelParent = nsnull;
  if (aParent || aNativeParent) {
    GdkWindow *parentWindow;
    // get the drawing area and the container from the parent
    if (aParent)
      parentWindow = GDK_WINDOW(aParent->GetNativeData(NS_NATIVE_WINDOW));
    else
      parentWindow = GDK_WINDOW(aNativeParent);

    // find the mozarea on that window
    gpointer user_data = nsnull;
    user_data = g_object_get_data(G_OBJECT(parentWindow), "mozdrawingarea");
    parentArea = MOZ_DRAWINGAREA(user_data);

    NS_ASSERTION(parentArea, "no drawingarea for parent widget!\n");
    if (!parentArea)
      return NS_ERROR_FAILURE;

    // get the user data for the widget - it should be a container
    user_data = nsnull;
    gdk_window_get_user_data(parentArea->inner_window, &user_data);
    NS_ASSERTION(user_data, "no user data for parentArea\n");
    if (!user_data)
      return NS_ERROR_FAILURE;

    // XXX support generic containers here for embedding!
    parentContainer = MOZ_CONTAINER(user_data);
    NS_ASSERTION(parentContainer, "owning widget is not a mozcontainer!\n");
    if (!parentContainer)
      return NS_ERROR_FAILURE;

    // get the toplevel window just in case someone needs to use it
    // for setting transients or whatever.
    topLevelParent =
      GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(parentContainer)));
  }

  // ok, create our windows
  switch (mWindowType) {
  case eWindowType_dialog:
  case eWindowType_popup:
  case eWindowType_toplevel:
    {
      mIsTopLevel = PR_TRUE;
      if (mWindowType == eWindowType_dialog) {
	mShell = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_type_hint(GTK_WINDOW(mShell),
				 GDK_WINDOW_TYPE_HINT_DIALOG);
	gtk_window_set_transient_for(GTK_WINDOW(mShell), topLevelParent);
	mTransientParent = topLevelParent;
	// add ourselves to the parent window's window group
	if (parentArea) {
	  nsWindow *parentnsWindow =
	    get_window_for_gdk_window(parentArea->inner_window);
	  NS_ASSERTION(parentnsWindow, "no nsWindow for parentArea!");
	  if (parentnsWindow && parentnsWindow->mWindowGroup) {
	    gtk_window_group_add_window(parentnsWindow->mWindowGroup,
					GTK_WINDOW(mShell));
	    // store this in case any children are created
	    mWindowGroup = parentnsWindow->mWindowGroup;
	    LOG(("adding window %p to group %p\n",
		 (void *)mShell, (void *)mWindowGroup));
	  }
	}
      }
      else if (mWindowType == eWindowType_popup) {
	mShell = gtk_window_new(GTK_WINDOW_POPUP);
	gtk_window_set_transient_for(GTK_WINDOW(mShell), topLevelParent);
	mTransientParent = topLevelParent;
      }
      else { // must be eWindowType_toplevel
	mShell = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	// each toplevel window gets its own window group
	mWindowGroup = gtk_window_group_new();
	// and add ourselves to the window group
	LOG(("adding window %p to new group %p\n",
	     (void *)mShell, (void *)mWindowGroup));
	gtk_window_group_add_window(mWindowGroup, GTK_WINDOW(mShell));
      }

      // create our container
      mContainer = MOZ_CONTAINER(moz_container_new());
      gtk_container_add(GTK_CONTAINER(mShell), GTK_WIDGET(mContainer));
      gtk_widget_realize(GTK_WIDGET(mContainer));

      // make sure this is the focus widget in the container
      gtk_window_set_focus(GTK_WINDOW(mShell), GTK_WIDGET(mContainer));

      // and the drawing area
      mDrawingarea = moz_drawingarea_new(nsnull, mContainer);
    }
    break;
  case eWindowType_child:
    {
      mDrawingarea = moz_drawingarea_new(parentArea, parentContainer);
    }
    break;
  default:
    break;
  }

  // label the drawing area with this object so we can find our way
  // home
  g_object_set_data(G_OBJECT(mDrawingarea->clip_window), "nsWindow",
		    this);
  g_object_set_data(G_OBJECT(mDrawingarea->inner_window), "nsWindow",
		    this);

  g_object_set_data(G_OBJECT(mDrawingarea->clip_window), "mozdrawingarea",
		    mDrawingarea);
  g_object_set_data(G_OBJECT(mDrawingarea->inner_window), "mozdrawingarea",
		    mDrawingarea);

  if (mContainer)
    g_object_set_data(G_OBJECT(mContainer), "nsWindow", this);

  if (mShell)
    g_object_set_data(G_OBJECT(mShell), "nsWindow", this);

  // attach listeners for events
  if (mShell) {
    g_signal_connect(G_OBJECT(mShell), "configure_event",
		     G_CALLBACK(configure_event_cb), NULL);
    g_signal_connect(G_OBJECT(mShell), "delete_event",
		     G_CALLBACK(delete_event_cb), NULL);
  }
  if (mContainer) {
    g_signal_connect_after(G_OBJECT(mContainer), "size_allocate",
			   G_CALLBACK(size_allocate_cb), NULL);
    g_signal_connect(G_OBJECT(mContainer), "expose_event",
		     G_CALLBACK(expose_event_cb), NULL);
    g_signal_connect(G_OBJECT(mContainer), "enter_notify_event",
		     G_CALLBACK(enter_notify_event_cb), NULL);
    g_signal_connect(G_OBJECT(mContainer), "leave_notify_event",
		     G_CALLBACK(leave_notify_event_cb), NULL);
    g_signal_connect(G_OBJECT(mContainer), "motion_notify_event",
		     G_CALLBACK(motion_notify_event_cb), NULL);
    g_signal_connect(G_OBJECT(mContainer), "button_press_event",
		     G_CALLBACK(button_press_event_cb), NULL);
    g_signal_connect(G_OBJECT(mContainer), "button_release_event",
		     G_CALLBACK(button_release_event_cb), NULL);
    g_signal_connect(G_OBJECT(mContainer), "focus_in_event",
		     G_CALLBACK(focus_in_event_cb), NULL);
    g_signal_connect(G_OBJECT(mContainer), "focus_out_event",
		     G_CALLBACK(focus_out_event_cb), NULL);
    g_signal_connect(G_OBJECT(mContainer), "key_press_event",
		     G_CALLBACK(key_press_event_cb), NULL);
    g_signal_connect(G_OBJECT(mContainer), "key_release_event",
		     G_CALLBACK(key_release_event_cb), NULL);
    g_signal_connect(G_OBJECT(mContainer), "scroll_event",
		     G_CALLBACK(scroll_event_cb), NULL);
    g_signal_connect(G_OBJECT(mContainer), "visibility_notify_event",
		     G_CALLBACK(visibility_notify_event_cb), NULL);
  }

  LOG(("nsWindow [%p]\n", (void *)this));
  if (mShell) {
    LOG(("\tmShell %p %p %lx\n", (void *)mShell, (void *)mShell->window,
	 GDK_WINDOW_XWINDOW(mShell->window)));
  }
  if (mContainer) {
    LOG(("\tmContainer %p %p %lx\n", (void *)mContainer,
	 (void *)GTK_WIDGET(mContainer)->window,
	 GDK_WINDOW_XWINDOW(GTK_WIDGET(mContainer)->window)));
  }
  if (mDrawingarea) {
    LOG(("\tmDrawingarea %p %p %p %lx %lx\n", (void *)mDrawingarea,
	 (void *)mDrawingarea->clip_window,
	 (void *)mDrawingarea->inner_window,
	 GDK_WINDOW_XWINDOW(mDrawingarea->clip_window),
	 GDK_WINDOW_XWINDOW(mDrawingarea->inner_window)));
  }

  // resize so that everything is set to the right dimensions
  Resize(mBounds.width, mBounds.height, PR_FALSE);

  return NS_OK;
}

void
nsWindow::NativeResize(PRInt32 aWidth, PRInt32 aHeight, PRBool  aRepaint)
{
  LOG(("nsWindow::NativeResize [%p] %d %d\n", (void *)this,
       aWidth, aHeight));

  // clear our resize flag
  mNeedsResize = PR_FALSE;

  if (mIsTopLevel)
    gtk_window_resize(GTK_WINDOW(mShell), aWidth, aHeight);
  
  moz_drawingarea_resize (mDrawingarea, aWidth, aHeight);
}

void
nsWindow::NativeResize(PRInt32 aX, PRInt32 aY,
		       PRInt32 aWidth, PRInt32 aHeight,
		       PRBool  aRepaint)
{
  mNeedsResize = PR_FALSE;
  
  LOG(("nsWindow::NativeResize [%p] %d %d %d %d\n", (void *)this,
       aX, aY, aWidth, aHeight));
  
  if (mIsTopLevel) {
    if (mParent && mWindowType == eWindowType_popup) {
      nsRect oldrect, newrect;
      oldrect.x = aX;
      oldrect.y = aY;
      mParent->WidgetToScreen(oldrect, newrect);
      moz_drawingarea_resize(mDrawingarea, aWidth, aHeight);
      gtk_window_move(GTK_WINDOW(mShell), newrect.x, newrect.y);
      gtk_window_resize(GTK_WINDOW(mShell), aWidth, aHeight);
    }
    else {
      gtk_window_move(GTK_WINDOW(mShell), aX, aY);
      gtk_window_resize(GTK_WINDOW(mShell), aWidth, aHeight);
      moz_drawingarea_resize(mDrawingarea, aWidth, aHeight);
    }
  }
  else {
    moz_drawingarea_move_resize(mDrawingarea, aX, aY, aWidth, aHeight);
  }
}

void
nsWindow::NativeShow (PRBool  aAction)
{
  if (aAction) {
    // unset our flag now that our window has been shown
    mNeedsShow = PR_FALSE;
    
    if (mIsTopLevel) {
      moz_drawingarea_set_visibility(mDrawingarea, aAction);
      gtk_widget_show(GTK_WIDGET(mContainer));
      gtk_widget_show(mShell);
      
    }
    else {
      moz_drawingarea_set_visibility(mDrawingarea, aAction);
    }
  }
  else {
    if (mIsTopLevel) {
      gtk_widget_hide(GTK_WIDGET(mShell));
      gtk_widget_hide(GTK_WIDGET(mContainer));
    }
    moz_drawingarea_set_visibility(mDrawingarea, aAction);
  }
}

void
nsWindow::EnsureGrabs(void)
{
  if (mRetryPointerGrab)
    GrabPointer();
  if (mRetryKeyboardGrab)
    GrabKeyboard();
}

void
nsWindow::GrabPointer(void)
{
  LOG(("GrabPointer %d\n", mRetryPointerGrab));

  mRetryPointerGrab = PR_FALSE;

  // If the window isn't visible, just set the flag to retry the
  // grab.  When this window becomes visible, the grab will be
  // retried.
  PRBool visibility = PR_TRUE;
  IsVisible(visibility);
  if (!visibility) {
    LOG(("GrabPointer: window not visible\n"));
    mRetryPointerGrab = PR_TRUE;
    return;
  }

  gint retval;
  retval = gdk_pointer_grab(mDrawingarea->inner_window, FALSE,
			    (GdkEventMask)(GDK_BUTTON_PRESS_MASK |
					   GDK_BUTTON_RELEASE_MASK |
					   GDK_ENTER_NOTIFY_MASK |
					   GDK_LEAVE_NOTIFY_MASK |
					   GDK_POINTER_MOTION_MASK),
			    (GdkWindow *)NULL, NULL, GDK_CURRENT_TIME);
  
  if (retval != GDK_GRAB_SUCCESS) {
    LOG(("GrabPointer: pointer grab failed\n"));
    mRetryPointerGrab = PR_TRUE;
  }
}

void
nsWindow::GrabKeyboard(void)
{
  LOG(("GrabKeyboard %d\n", mRetryKeyboardGrab));

  mRetryKeyboardGrab = PR_FALSE;

  // If the window isn't visible, just set the flag to retry the
  // grab.  When this window becomes visible, the grab will be
  // retried.
  PRBool visibility = PR_TRUE;
  IsVisible(visibility);
  if (!visibility) {
    LOG(("GrabKeyboard: window not visible\n"));
    mRetryKeyboardGrab = PR_TRUE;
    return;
  }
  
  // we need to grab the keyboard on the transient parent so that we
  // don't end up with any focus events that end up on the parent
  // window that will cause the popup to go away
  GdkWindow *grabWindow;
  
  if (mTransientParent)
    grabWindow = GTK_WIDGET(mTransientParent)->window;
  else
    grabWindow = mDrawingarea->inner_window;

  gint retval;
  retval = gdk_keyboard_grab(grabWindow, TRUE, GDK_CURRENT_TIME);
  
  if (retval != GDK_GRAB_SUCCESS) {
    LOG(("GrabKeyboard: keyboard grab failed %d\n", retval));
    gdk_pointer_ungrab(GDK_CURRENT_TIME);
    mRetryKeyboardGrab = PR_TRUE;
  }
}

void
nsWindow::ReleaseGrabs(void)
{
  LOG(("ReleaseGrabs\n"));

  mRetryPointerGrab = PR_FALSE;
  mRetryKeyboardGrab = PR_FALSE;

  gdk_pointer_ungrab(GDK_CURRENT_TIME);
  gdk_keyboard_ungrab(GDK_CURRENT_TIME);
}

void
nsWindow::GetToplevelWidget(GtkWidget **aWidget)
{
  *aWidget = nsnull;

  if (mShell) {
    *aWidget = mShell;
    return;
  }

  GtkWidget *widget =
    get_gtk_widget_for_gdk_window(mDrawingarea->inner_window);
  if (!widget)
    return;
  
  *aWidget = gtk_widget_get_toplevel(widget);
}

PRBool
check_for_rollup(GdkWindow *aWindow, gdouble aMouseX, gdouble aMouseY,
		 PRBool aIsWheel)
{
  PRBool retVal = PR_FALSE;
  nsCOMPtr<nsIWidget> rollupWidget = do_QueryReferent(gRollupWindow);

  if (rollupWidget && gRollupListener) {
    GdkWindow *currentPopup =
      (GdkWindow *)rollupWidget->GetNativeData(NS_NATIVE_WINDOW);
    if (!is_mouse_in_window(currentPopup, aMouseX, aMouseY)) {
      PRBool rollup = PR_TRUE;
      if (aIsWheel) {
	gRollupListener->ShouldRollupOnMouseWheelEvent(&rollup);
	retVal = PR_TRUE;
      }
      // if we're dealing with menus, we probably have submenus and
      // we don't want to rollup if the clickis in a parent menu of
      // the current submenu
      nsCOMPtr<nsIMenuRollup> menuRollup;
      menuRollup = (do_QueryInterface(gRollupListener));
      if (menuRollup) {
	nsCOMPtr<nsISupportsArray> widgetChain;
	menuRollup->GetSubmenuWidgetChain(getter_AddRefs(widgetChain));
	if (widgetChain) {
	  PRUint32 count = 0;
	  widgetChain->Count(&count);
	  for (PRUint32 i=0; i<count; ++i) {
	    nsCOMPtr<nsISupports> genericWidget;
	    widgetChain->GetElementAt(i, getter_AddRefs(genericWidget));
	    nsCOMPtr<nsIWidget> widget(do_QueryInterface(genericWidget));
	    if (widget) {
              GdkWindow* currWindow =
		(GdkWindow*) widget->GetNativeData(NS_NATIVE_WINDOW);
              if (is_mouse_in_window(currWindow, aMouseX, aMouseY)) {
                rollup = PR_FALSE;
                break;
              }
            }
          } // foreach parent menu widget
        }
      } // if rollup listener knows about menus
      
      // if we've determined that we should still rollup, do it.
      if (rollup) {
        gRollupListener->Rollup();
        retVal = PR_TRUE;
      }
    }
  } else {
    gRollupWindow = nsnull;
    gRollupListener = nsnull;
  }
  
  return retVal;
}

/* static */
PRBool
is_mouse_in_window (GdkWindow* aWindow, gdouble aMouseX, gdouble aMouseY)
{
  gint x = 0;
  gint y = y;
  gint w, h;

  gint offsetX = 0;
  gint offsetY = 0;
  
  GtkWidget *widget;
  GdkWindow *window;

  window = aWindow;

  while (window) {
    gint tmpX = 0;
    gint tmpY = 0;

    gdk_window_get_position(window, &tmpX, &tmpY);
    widget = get_gtk_widget_for_gdk_window(window);

    // if this is a window, compute x and y given its origin and our
    // offset
    if (GTK_IS_WINDOW(widget)) {
      x = tmpX + offsetX;
      y = tmpY + offsetY;
      break;
    }

    offsetX += tmpX;
    offsetY += tmpY;
    window = gdk_window_get_parent(window);
  }

  gdk_window_get_size(aWindow, &w, &h);
  
  if ( aMouseX > x && aMouseX < x + w &&
       aMouseY > y && aMouseY < y + h )
    return PR_TRUE;
  
  return PR_FALSE;
}

/* static */
nsWindow *
get_window_for_gtk_widget(GtkWidget *widget)
{
  gpointer user_data;
  user_data = g_object_get_data(G_OBJECT(widget), "nsWindow");

  if (!user_data)
    return nsnull;

  return NS_STATIC_CAST(nsWindow *, user_data);
}

/* static */
nsWindow *
get_window_for_gdk_window(GdkWindow *window)
{
  gpointer user_data;
  user_data = g_object_get_data(G_OBJECT(window), "nsWindow");

  if (!user_data)
    return nsnull;
  
  return NS_STATIC_CAST(nsWindow *, user_data);
}

/* static */
nsWindow *
get_owning_window_for_gdk_window(GdkWindow *window)
{
  GtkWidget *owningWidget = get_gtk_widget_for_gdk_window(window);
  if (!owningWidget)
    return nsnull;

  gpointer user_data;
  user_data = g_object_get_data(G_OBJECT(owningWidget), "nsWindow");
  
  if (!user_data)
    return nsnull;

  return (nsWindow *)user_data;
}

/* static */
GtkWidget *
get_gtk_widget_for_gdk_window(GdkWindow *window)
{
  gpointer user_data = NULL;
  gdk_window_get_user_data(window, &user_data);
  if (!user_data)
    return NULL;

  return GTK_WIDGET(user_data);
}

// gtk callbacks

/* static */
gboolean
expose_event_cb(GtkWidget *widget, GdkEventExpose *event)
{
  nsWindow *window = get_window_for_gdk_window(event->window);
  if (!window)
    return FALSE;

  // XXX We are so getting lucky here.  We are doing all of
  // mozilla's painting and then allowing default processing to occur.
  // This means that Mozilla paints in all of it's stuff and then
  // NO_WINDOW widgets (like scrollbars, for example) are painted by
  // Gtk on top of what we painted.

  // This return window->OnExposeEvent(widget, event); */

  window->OnExposeEvent(widget, event);
  return FALSE;
}

/* static */
gboolean
configure_event_cb(GtkWidget *widget,
		   GdkEventConfigure *event)
{
  nsWindow *window = get_window_for_gtk_widget(widget);
  if (!window)
    return FALSE;
  
  return window->OnConfigureEvent(widget, event);
}

/* static */
void
size_allocate_cb (GtkWidget *widget, GtkAllocation *allocation)
{
  nsWindow *window = get_window_for_gtk_widget(widget);
  if (!window)
    return;

  window->OnSizeAllocate(widget, allocation);
}

/* static */
gboolean
delete_event_cb(GtkWidget *widget, GdkEventAny *event)
{
  nsWindow *window = get_window_for_gtk_widget(widget);
  if (!window)
    return FALSE;

  window->OnDeleteEvent(widget, event);

  return TRUE;
}

/* static */
gboolean
enter_notify_event_cb (GtkWidget *widget,
				GdkEventCrossing *event)
{
  nsWindow *window = get_window_for_gdk_window(event->window);
  if (!window)
    return TRUE;
  
  window->OnEnterNotifyEvent(widget, event);

  return TRUE;
}

/* static */
gboolean
leave_notify_event_cb (GtkWidget *widget,
		       GdkEventCrossing *event)
{
  nsWindow *window = get_window_for_gdk_window(event->window);
  if (!window)
    return TRUE;
  
  window->OnLeaveNotifyEvent(widget, event);

  return TRUE;
}

/* static */
gboolean
motion_notify_event_cb (GtkWidget *widget, GdkEventMotion *event)
{
  nsWindow *window = get_window_for_gdk_window(event->window);
  if (!window)
    return TRUE;

  window->OnMotionNotifyEvent(widget, event);

  return TRUE;
}

/* static */
gboolean
button_press_event_cb   (GtkWidget *widget, GdkEventButton *event)
{
  LOG(("button_press_event_cb\n"));
  nsWindow *window = get_window_for_gdk_window(event->window);
  if (!window)
    return TRUE;

  window->OnButtonPressEvent(widget, event);

  return TRUE;
}

/* static */
gboolean
button_release_event_cb (GtkWidget *widget, GdkEventButton *event)
{
  nsWindow *window = get_window_for_gdk_window(event->window);
  if (!window)
    return TRUE;

  window->OnButtonReleaseEvent(widget, event);

  return TRUE;
}

/* static */
gboolean
focus_in_event_cb (GtkWidget *widget, GdkEventFocus *event)
{
  nsWindow *window = get_window_for_gtk_widget(widget);
  if (!window)
    return FALSE;

  window->OnContainerFocusInEvent(widget, event);

  return FALSE;
}

/* static */
gboolean
focus_out_event_cb (GtkWidget *widget, GdkEventFocus *event)
{
  nsWindow *window = get_window_for_gtk_widget(widget);
  if (!window)
    return FALSE;

  window->OnContainerFocusOutEvent(widget, event);

  return FALSE;
}

/* static */
gboolean
key_press_event_cb (GtkWidget *widget, GdkEventKey *event)
{
  LOG(("key_press_event_cb\n"));
  // find the window with focus and dispatch this event to that widget
  nsWindow *window = get_window_for_gtk_widget(widget);
  if (!window)
    return FALSE;

  nsWindow *focusWindow = window->mFocusChild;
  if (!focusWindow)
    focusWindow = window;

  return focusWindow->OnKeyPressEvent(widget, event);
}

gboolean
key_release_event_cb (GtkWidget *widget, GdkEventKey *event)
{
  LOG(("key_release_event_cb\n"));
  // find the window with focus and dispatch this event to that widget
  nsWindow *window = get_window_for_gtk_widget(widget);
  if (!window)
    return FALSE;

  nsWindow *focusWindow = window->mFocusChild;
  if (!focusWindow)
    focusWindow = window;

  return focusWindow->OnKeyReleaseEvent(widget, event);
}

/* static */
gboolean
scroll_event_cb (GtkWidget *widget, GdkEventScroll *event)
{
  nsWindow *window = get_window_for_gdk_window(event->window);
  if (!window)
    return FALSE;

  window->OnScrollEvent(widget, event);

  return TRUE;
}

/* static */
gboolean
visibility_notify_event_cb (GtkWidget *widget, GdkEventVisibility *event)
{
  nsWindow *window = get_window_for_gdk_window(event->window);
  if (!window)
    return FALSE;

  window->OnVisibilityNotifyEvent(widget, event);

  return TRUE;
}

// nsChildWindow class

nsChildWindow::nsChildWindow()
{
}

nsChildWindow::~nsChildWindow()
{
}
