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

#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include "gtklayout.h"

#include "nsWindow.h"
#include "nsIFontMetrics.h"
#include "nsFont.h"
#include "nsGUIEvent.h"
#include "nsIRenderingContext.h"
#include "nsIDeviceContext.h"
#include "nsRect.h"
#include "nsTransform2D.h"
#include "nsGfxCIID.h"

#include "nsGtkEventHandler.h"
#include "nsAppShell.h"

#include "X11/Xlib.h"

#include "X11/Intrinsic.h"
#include "X11/cursorfont.h"

#include "stdio.h"

#define DBG 0

GtkWidget *gFirstTopLevelWindow = 0; //XXX: REMOVE Kludge should not be needed.

static NS_DEFINE_IID(kIWidgetIID, NS_IWIDGET_IID);

NS_IMPL_ADDREF(nsWindow)
NS_IMPL_RELEASE(nsWindow)

extern XtAppContext gAppContext;

//-------------------------------------------------------------------------
//
// nsWindow constructor
//
//-------------------------------------------------------------------------
nsWindow::nsWindow():
  mEventListener(nsnull),
  mMouseListener(nsnull),
  mToolkit(nsnull),
  mFontMetrics(nsnull),
  mContext(nsnull),
  mWidget(nsnull),
  mGC(nsnull),
  mEventCallback(nsnull),
  mIgnoreResize(PR_FALSE)
{
  NS_INIT_REFCNT();
  strcpy(gInstanceClassName, "nsWindow");
  mBounds.x = 0;
  mBounds.y = 0;
  mBounds.width = 0;
  mBounds.height = 0;
  mResized = PR_FALSE;
  mShown = PR_FALSE;
  mVisible = PR_FALSE;
  mDisplayed = PR_FALSE;
  mLowerLeft = PR_FALSE;
  mCursor = eCursor_standard;
  mClientData = nsnull;
  mPreferredWidth  = 0;
  mPreferredHeight = 0;
  mFont = nsnull;
}

//-------------------------------------------------------------------------
//
// nsWindow destructor
//
//-------------------------------------------------------------------------
nsWindow::~nsWindow()
{
  OnDestroy();
  gtk_widget_destroy(mWidget);
#if 0
  if (nsnull != mGC) {
    ::XFreeGC((Display *)GetNativeData(NS_NATIVE_DISPLAY),mGC);
    mGC = nsnull;
  }
#endif
}

//-------------------------------------------------------------------------
void nsWindow::ConvertToDeviceCoordinates(nscoord &aX, nscoord &aY)
{

}

//-------------------------------------------------------------------------
NS_METHOD nsWindow::WidgetToScreen(const nsRect& aOldRect, nsRect& aNewRect)
{
  return NS_OK;
}

//-------------------------------------------------------------------------
NS_METHOD nsWindow::ScreenToWidget(const nsRect& aOldRect, nsRect& aNewRect)
{
  return NS_OK;
}

//-------------------------------------------------------------------------
//
// Setup initial tooltip rectangles
//
//-------------------------------------------------------------------------
NS_METHOD nsWindow::SetTooltips(PRUint32 aNumberOfTips,nsRect* aTooltipAreas[])
{
  return NS_OK;
}

//-------------------------------------------------------------------------
//
// Update all tooltip rectangles
//
//-------------------------------------------------------------------------

NS_METHOD nsWindow::UpdateTooltips(nsRect* aNewTips[])
{
  return NS_OK;
}

//-------------------------------------------------------------------------
//
// Remove all tooltip rectangles
//
//-------------------------------------------------------------------------

NS_METHOD nsWindow::RemoveTooltips()
{
  return NS_OK;
}

//-------------------------------------------------------------------------
//
//
//
//-------------------------------------------------------------------------
void nsWindow::InitToolkit(nsIToolkit *aToolkit,
                           nsIWidget  *aWidgetParent)
{
  if (nsnull == mToolkit) {
    if (nsnull != aToolkit) {
      mToolkit = (nsToolkit*)aToolkit;
      mToolkit->AddRef();
    }
    else {
      if (nsnull != aWidgetParent) {
        mToolkit = (nsToolkit*)(aWidgetParent->GetToolkit()); // the call AddRef's, we don't have to
      }
      // it's some top level window with no toolkit passed in.
      // Create a default toolkit with the current thread
      else {
        mToolkit = new nsToolkit();
        mToolkit->AddRef();
        mToolkit->Init(PR_GetCurrentThread());

        // Create a shared GC for all widgets
        ((nsToolkit *)mToolkit)->SetSharedGC((GdkGC*)GetNativeData(NS_NATIVE_GRAPHIC));
      }
    }
  }
}

void nsWindow::InitDeviceContext(nsIDeviceContext *aContext,
                                 GtkWidget *aParentWidget)
{
  // keep a reference to the toolkit object
  if (aContext) {
    mContext = aContext;
    mContext->AddRef();
  }
  else {
    nsresult  res;

    static NS_DEFINE_IID(kDeviceContextCID, NS_DEVICE_CONTEXT_CID);
    static NS_DEFINE_IID(kDeviceContextIID, NS_IDEVICE_CONTEXT_IID);

    //res = !NS_OK;
    res = nsRepository::CreateInstance(kDeviceContextCID,
                                       nsnull,
                                       kDeviceContextIID,
                                       (void **)&mContext);
    if (NS_OK == res) {
      mContext->Init(aParentWidget);
    }
  }
}

void nsWindow::CreateGC()
{
  if (nsnull == mGC) {
    if (!mWidget) {
      mWidget = ::gtk_window_new(GTK_WINDOW_POPUP);
      gtk_widget_realize(mWidget);
      mGC = ::gdk_gc_new(GTK_WIDGET(mWidget)->window);
    }
    else if (!GTK_WIDGET(mWidget)->window) {
      gtk_widget_realize(mWidget);
      mGC = ::gdk_gc_new(GTK_WIDGET(mWidget)->window);
    }
    else
      mGC = ::gdk_gc_new(GTK_WIDGET(mWidget)->window);
  }
}

nsresult nsWindow::StandardWindowCreate(nsIWidget *aParent,
		      const nsRect &aRect,
		      EVENT_CALLBACK aHandleEventFunction,
		      nsIDeviceContext *aContext,
		      nsIAppShell *aAppShell,
		      nsIToolkit *aToolkit,
		      nsWidgetInitData *aInitData,
		      nsNativeWidget aNativeParent)
{
  GtkWidget *mainWindow = 0, *parentWidget = 0;
  mBounds = aRect;
  mAppShell = aAppShell;

  InitToolkit(aToolkit, aParent);

  // save the event callback function
  mEventCallback = aHandleEventFunction;

  if (aParent) {
     parentWidget = GTK_WIDGET(aParent->GetNativeData(NS_NATIVE_WIDGET));
  } else if (aAppShell) {
     parentWidget = GTK_WIDGET(aAppShell->GetNativeData(NS_NATIVE_SHELL));
  }

   // XXX: This is a kludge, need to be able to create multiple top
   // level windows instead.
  if (gFirstTopLevelWindow == 0) {
    mainWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gFirstTopLevelWindow = mainWindow;
  } else {
    mainWindow = gtk_window_new(GTK_WINDOW_POPUP);
  }

  gtk_widget_set_usize(mainWindow, aRect.width, aRect.height);

//  mWidget = gtk_layout_new(FALSE,FALSE);
  mWidget = gtk_label_new("foobar!");
  gtk_container_add(GTK_CONTAINER(mainWindow), mWidget);

  if (aParent) {
    aParent->AddChild(this);
  }

  // Force cursor to default setting
  mCursor = eCursor_select;
  SetCursor(eCursor_standard);

  InitCallbacks();
  CreateGC();

  return NS_OK;
}

//-------------------------------------------------------------------------
//
// Initialize all the Callbacks
//
//-------------------------------------------------------------------------
void nsWindow::InitCallbacks(char * aName)
{
#if 0
  XtAddEventHandler(mWidget,
		    ButtonPressMask,
		    PR_FALSE,
		    nsXtWidget_ButtonPressMask_EventHandler,
		    this);

  XtAddEventHandler(mWidget,
		    ButtonReleaseMask,
		    PR_FALSE,
		    nsXtWidget_ButtonReleaseMask_EventHandler,
		    this);

  XtAddEventHandler(mWidget,
		    ButtonMotionMask,
		    PR_FALSE,
		    nsXtWidget_ButtonMotionMask_EventHandler,
		    this);

  XtAddEventHandler(mWidget,
		    PointerMotionMask,
		    PR_FALSE,
		    nsXtWidget_MotionMask_EventHandler,
		    this);

  XtAddEventHandler(mWidget,
		    EnterWindowMask,
		    PR_FALSE,
		    nsXtWidget_EnterMask_EventHandler,
		    this);

  XtAddEventHandler(mWidget,
		    LeaveWindowMask,
		    PR_FALSE,
		    nsXtWidget_LeaveMask_EventHandler,
		    this);

  XtAddEventHandler(mWidget,
		    ExposureMask,
		    PR_TRUE,
		    nsXtWidget_ExposureMask_EventHandler,
		    this);

  XtAddEventHandler(mWidget,
                    KeyPressMask,
                    PR_FALSE,
                    nsXtWidget_KeyPressMask_EventHandler,
                    this);

  XtAddEventHandler(mWidget,
                    KeyReleaseMask,
                    PR_FALSE,
                    nsXtWidget_KeyReleaseMask_EventHandler,
                    this);
#endif
}

//-------------------------------------------------------------------------
//
// Query interface implementation
//
//-------------------------------------------------------------------------
nsresult nsWindow::QueryInterface(const nsIID& aIID, void** aInstancePtr)
{
    if (NULL == aInstancePtr) {
        return NS_ERROR_NULL_POINTER;
    }
    nsresult result = NS_NOINTERFACE;
    static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);
    if (aIID.Equals(kISupportsIID)) {
        *aInstancePtr = (void*) ((nsISupports*)this);
        AddRef();
        return NS_OK;
    }

    static NS_DEFINE_IID(kIWidgetIID, NS_IWIDGET_IID);
    if (aIID.Equals(kIWidgetIID)) {
        *aInstancePtr = (void*) ((nsIWidget*)this);
        AddRef();
        result = NS_OK;
    }

    return result;
}

//-------------------------------------------------------------------------
//
// create with nsIWidget parent
//
//-------------------------------------------------------------------------

NS_METHOD nsWindow::Create(nsIWidget *aParent,
                      const nsRect &aRect,
                      EVENT_CALLBACK aHandleEventFunction,
                      nsIDeviceContext *aContext,
                      nsIAppShell *aAppShell,
                      nsIToolkit *aToolkit,
                      nsWidgetInitData *aInitData)
{
    return(StandardWindowCreate(aParent, aRect, aHandleEventFunction,
                           aContext, aAppShell, aToolkit, aInitData,
			   nsnull));
}

//-------------------------------------------------------------------------
//
// create with a native parent
//
//-------------------------------------------------------------------------
NS_METHOD nsWindow::Create(nsNativeWidget aParent,
                      const nsRect &aRect,
                      EVENT_CALLBACK aHandleEventFunction,
                      nsIDeviceContext *aContext,
                      nsIAppShell *aAppShell,
                      nsIToolkit *aToolkit,
                      nsWidgetInitData *aInitData)
{
    return(StandardWindowCreate(nsnull, aRect, aHandleEventFunction,
                           aContext, aAppShell, aToolkit, aInitData,
			   aParent));
}


//-------------------------------------------------------------------------
//
// Close this nsWindow
//
//-------------------------------------------------------------------------
NS_METHOD nsWindow::Destroy()
{
  return NS_OK;
}

//-------------------------------------------------------------------------
//
// Accessor functions to get/set the client data
//
//-------------------------------------------------------------------------

NS_IMETHODIMP nsWindow::GetClientData(void*& aClientData)
{
  aClientData = mClientData;
  return NS_OK;
}

NS_IMETHODIMP nsWindow::SetClientData(void* aClientData)
{
  mClientData = aClientData;
  return NS_OK;
}


//-------------------------------------------------------------------------
//
// Get this nsWindow parent
//
//-------------------------------------------------------------------------
nsIWidget* nsWindow::GetParent(void)
{
  // XXX: Implement this
  return nsnull;
}


//-------------------------------------------------------------------------
//
// Get this nsWindow's list of children
//
//-------------------------------------------------------------------------
nsIEnumerator* nsWindow::GetChildren()
{
  //XXX: Implement this
  return nsnull;
}


//-------------------------------------------------------------------------
//
// Add a child to the list of children
//
//-------------------------------------------------------------------------
void nsWindow::AddChild(nsIWidget* aChild)
{
  // XXX:Implement this
}


//-------------------------------------------------------------------------
//
// Remove a child from the list of children
//
//-------------------------------------------------------------------------
void nsWindow::RemoveChild(nsIWidget* aChild)
{
  // XXX:Implement this
}


//-------------------------------------------------------------------------
//
// Hide or show this component
//
//-------------------------------------------------------------------------
NS_METHOD nsWindow::Show(PRBool bState)
{
  g_print("nsWindow::Show called with %d", bState);
  gtk_widget_show_all(gtk_widget_get_toplevel(mWidget));
  mShown = bState;
  if (bState) {
    gtk_widget_show(mWidget);
    // gtk_widget_show_all(mWidget); Maybe?
  } else {
  }

/*
  mShown = bState;
  if (bState) {
    XtManageChild(mWidget);
  }
  else
    XtUnmanageChild(mWidget);
*/
  return NS_OK;
}

//-------------------------------------------------------------------------
NS_METHOD nsWindow::IsVisible(PRBool & aState)
{
  aState = mShown;
  return NS_OK;
}

//-------------------------------------------------------------------------
//
// Move this component
//
//-------------------------------------------------------------------------
NS_METHOD nsWindow::Move(PRUint32 aX, PRUint32 aY)
{
#if 0
  mBounds.x = aX;
  mBounds.y = aY;
  // TODO
  // gtk_layout_move(GTK_LAYOUT(layout), mWidget, aX, aY);
  XtVaSetValues(mWidget, XmNx, aX, XmNy, GetYCoord(aY), nsnull);
#endif
  return NS_OK;
}

//-------------------------------------------------------------------------
//
// Resize this component
//
//-------------------------------------------------------------------------
NS_METHOD nsWindow::Resize(PRUint32 aWidth, PRUint32 aHeight, PRBool aRepaint)
{
#if 0
  if (DBG) printf("$$$$$$$$$ %s::Resize %d %d   Repaint: %s\n",
                  gInstanceClassName, aWidth, aHeight, (aRepaint?"true":"false"));
  mBounds.width  = aWidth;
  mBounds.height = aHeight;
  // TODO
  // gtk_layout_set_size(GTK_LAYOUT(layout), aWidth, aHeight);
  XtVaSetValues(mWidget, XmNx, mBounds.x, XmNy, mBounds.y, XmNwidth, aWidth, XmNheight, aHeight, nsnull);
#endif
  return NS_OK;
}


//-------------------------------------------------------------------------
//
// Resize this component
//
//-------------------------------------------------------------------------
NS_METHOD nsWindow::Resize(PRUint32 aX, PRUint32 aY, PRUint32 aWidth, PRUint32 aHeight, PRBool aRepaint)
{
#if 0
  mBounds.x      = aX;
  mBounds.y      = aY;
  mBounds.width  = aWidth;
  mBounds.height = aHeight;
  XtVaSetValues(mWidget, XmNx, aX, XmNy, GetYCoord(aY),
                        XmNwidth, aWidth, XmNheight, aHeight, nsnull);
#endif
  return NS_OK;
}


//-------------------------------------------------------------------------
//
// Enable/disable this component
//
//-------------------------------------------------------------------------
NS_METHOD nsWindow::Enable(PRBool bState)
{
  gtk_widget_set_sensitive(mWidget, bState);
  return NS_OK;
}


//-------------------------------------------------------------------------
//
// Give the focus to this component
//
//-------------------------------------------------------------------------
NS_METHOD nsWindow::SetFocus(void)
{
#if 0
   // Go get the parent of all widget's to determine which widget
   // tree to use to set the focus.
  Widget w = mWidget;
  while (NULL != XtParent(w)) {
    w = XtParent(w);
  }

  XtSetKeyboardFocus(w, mWidget);
#endif
// TODO
  gtk_widget_grab_focus(mWidget);
  return NS_OK;
}


//-------------------------------------------------------------------------
//
// Set this component dimension
//
//-------------------------------------------------------------------------
void nsWindow::SetBounds(const nsRect &aRect)
{
  mBounds.x      = aRect.x;
  mBounds.y      = aRect.y;
  mBounds.width  = aRect.width;
  mBounds.height = aRect.height;
  //Resize(mBounds.x, mBounds.y, mBounds.width, mBounds.height, PR_TRUE);
}

//-------------------------------------------------------------------------
//
// Get this component dimension
//
//-------------------------------------------------------------------------
NS_METHOD nsWindow::GetBounds(nsRect &aRect)
{
  aRect = mBounds;
  return NS_OK;
}

//-------------------------------------------------------------------------
//
// Get the foreground color
//
//-------------------------------------------------------------------------
nscolor nsWindow::GetForegroundColor(void)
{
  return (mForeground);
  return NS_OK;
}


//-------------------------------------------------------------------------
//
// Set the foreground color
//
//-------------------------------------------------------------------------
NS_METHOD nsWindow::SetForegroundColor(const nscolor &aColor)
{
#if 0
  PRUint32 pixel;
  mForeground = aColor;
  mContext->ConvertPixel(aColor, pixel);
  XtVaSetValues(mWidget, XtNforeground, pixel, nsnull);
#endif
  return NS_OK;
}


//-------------------------------------------------------------------------
//
// Get the background color
//
//-------------------------------------------------------------------------
nscolor nsWindow::GetBackgroundColor(void)
{
  return (mBackground);
  return NS_OK;
}


//-------------------------------------------------------------------------
//
// Set the background color
//
//-------------------------------------------------------------------------
NS_METHOD nsWindow::SetBackgroundColor(const nscolor &aColor)
{
#if 0
  mBackground = aColor ;
  PRUint32 pixel;
  mContext->ConvertPixel(aColor, pixel);
  XtVaSetValues(mWidget, XtNbackground, pixel, nsnull);
#endif
  return NS_OK;
}


//-------------------------------------------------------------------------
//
// Get this component font
//
//-------------------------------------------------------------------------
nsIFontMetrics* nsWindow::GetFont(void)
{
  //XXX: Implement this
  NS_NOTYETIMPLEMENTED("GetFont not yet implemented"); // to be implemented
  return nsnull;
}


//-------------------------------------------------------------------------
//
// Set this component font
//
//-------------------------------------------------------------------------
NS_METHOD nsWindow::SetFont(const nsFont &aFont)
{
#if 0
    if (mContext == nsnull) {
      return NS_ERROR_FAILURE;
    }
    nsIFontMetrics* metrics;
    mContext->GetMetricsFor(aFont, metrics);
    if (metrics != nsnull) {

      XmFontList      fontList = NULL;
      XmFontListEntry entry    = NULL;
      nsFontHandle    fontHandle;
      metrics->GetFontHandle(fontHandle);
      XFontStruct * fontStruct = XQueryFont(XtDisplay(mWidget), (XID)fontHandle);

      if (fontStruct != NULL) {
        entry = XmFontListEntryCreate(XmFONTLIST_DEFAULT_TAG,
                                      XmFONT_IS_FONT, fontStruct);
        fontList = XmFontListAppendEntry(NULL, entry);

        XtVaSetValues(mWidget, XmNfontList, fontList, NULL);

        XmFontListEntryFree(&entry);
        XmFontListFree(fontList);
      }

      NS_RELEASE(metrics);
    } else {
      printf("****** Error: Metrics is NULL!\n");
      return NS_ERROR_FAILURE;
    }

     // XXX Temporary, should not be caching the font
    if (mFont == nsnull) {
      mFont = new nsFont(aFont);
    } else {
      *mFont  = aFont;
    }
#endif
  return NS_OK;
}


//-------------------------------------------------------------------------
//
// Get this component cursor
//
//-------------------------------------------------------------------------
nsCursor nsWindow::GetCursor()
{
  return eCursor_standard;
}


//-------------------------------------------------------------------------
//
// Set this component cursor
//
//-------------------------------------------------------------------------

NS_METHOD nsWindow::SetCursor(nsCursor aCursor)
{
  if (!mWidget || !mWidget->window)
    return NS_ERROR_FAILURE;

  // Only change cursor if it's changing
  if (aCursor != mCursor) {
    GdkCursor *newCursor = 0;

    switch(aCursor) {
      case eCursor_select:
        newCursor = gdk_cursor_new(GDK_XTERM);
      break;

      case eCursor_wait:
        newCursor = gdk_cursor_new(GDK_WATCH);
      break;

      case eCursor_hyperlink:
        newCursor = gdk_cursor_new(GDK_HAND2);
      break;

      case eCursor_standard:
        newCursor = gdk_cursor_new(GDK_LEFT_PTR);
      break;

      case eCursor_arrow_south:
      case eCursor_arrow_south_plus:
        newCursor = gdk_cursor_new(GDK_BOTTOM_SIDE);
      break;

      case eCursor_arrow_north:
      case eCursor_arrow_north_plus:
        newCursor = gdk_cursor_new(GDK_TOP_SIDE);
      break;

      case eCursor_arrow_east:
      case eCursor_arrow_east_plus:
        newCursor = gdk_cursor_new(GDK_RIGHT_SIDE);
      break;

      case eCursor_arrow_west:
      case eCursor_arrow_west_plus:
        newCursor = gdk_cursor_new(GDK_LEFT_SIDE);
      break;

      default:
        NS_ASSERTION(PR_FALSE, "Invalid cursor type");
      break;
    }

    if (nsnull != newCursor) {
      mCursor = aCursor;
      gdk_window_set_cursor(mWidget->window, newCursor);
    }
 }
 return NS_OK;
}

//-------------------------------------------------------------------------
//
// Invalidate this component visible area
//
//-------------------------------------------------------------------------
NS_METHOD nsWindow::Invalidate(PRBool aIsSynchronous)
{
#if 0
  if (mWidget == nsnull) {
    return NS_ERROR_FAILURE;
  }

  if (!XtIsRealized(mWidget)) {
    return NS_ERROR_FAILURE;
  }

  Window  win      = XtWindow(mWidget);
  Display *display = XtDisplay(mWidget);


  XEvent evt;
  evt.xgraphicsexpose.type       = GraphicsExpose;
  evt.xgraphicsexpose.send_event = False;
  evt.xgraphicsexpose.display    = display;
  evt.xgraphicsexpose.drawable   = win;
  evt.xgraphicsexpose.x          = 0;
  evt.xgraphicsexpose.y          = 0;
  evt.xgraphicsexpose.width      = mBounds.width;
  evt.xgraphicsexpose.height     = mBounds.height;
  evt.xgraphicsexpose.count      = 0;
  XSendEvent(display, win, False, ExposureMask, &evt);
  XFlush(display);
#endif
  return NS_OK;
}


//-------------------------------------------------------------------------
//
// Invalidate this component visible area
//
//-------------------------------------------------------------------------
NS_METHOD nsWindow::Invalidate(const nsRect & aRect, PRBool aIsSynchronous)
{
#if 0
  if (mWidget == nsnull) {
    return NS_ERROR_FAILURE;
  }

  if (!XtIsRealized(mWidget)) {
    return NS_ERROR_FAILURE;
  }

  Window  win      = XtWindow(mWidget);
  Display *display = XtDisplay(mWidget);


  XEvent evt;
  evt.xgraphicsexpose.type       = GraphicsExpose;
  evt.xgraphicsexpose.send_event = False;
  evt.xgraphicsexpose.display    = display;
  evt.xgraphicsexpose.drawable   = win;
  evt.xgraphicsexpose.x          = aRect.x;
  evt.xgraphicsexpose.y          = aRect.y;
  evt.xgraphicsexpose.width      = aRect.width;
  evt.xgraphicsexpose.height     = aRect.height;
  evt.xgraphicsexpose.count      = 0;
  XSendEvent(display, win, False, ExposureMask, &evt);
  XFlush(display);
#endif
  return NS_OK;
}

//-------------------------------------------------------------------------
//
// Force a synchronous repaint of the window
//
//-------------------------------------------------------------------------
NS_IMETHODIMP nsWindow::Update()
{
  return NS_OK;
}

//-------------------------------------------------------------------------
//
// Return some native data according to aDataType
//
//-------------------------------------------------------------------------
void* nsWindow::GetNativeData(PRUint32 aDataType)
{
  switch(aDataType) {

    case NS_NATIVE_WINDOW:
 //     return (void*)XtWindow(mWidget);
      return (void*)mWidget->window;
    case NS_NATIVE_DISPLAY:
 //     return (void*)XtDisplay(mWidget);
      return (void*)GDK_DISPLAY();
    case NS_NATIVE_WIDGET:
      return (void*)(mWidget);
    case NS_NATIVE_GRAPHIC:
      {
        void *res = NULL;
        // We Cache a Read-Only Shared GC in the Toolkit.  If we don't
        // have one ourselves (because it needs to be writeable) grab the
        // the shared GC
        if (nsnull == mGC) {
           NS_ASSERTION(mToolkit, "Unable to return GC, toolkit is null");
          res = (void *)((nsToolkit *)mToolkit)->GetSharedGC();
        }
        else {
          res = (void *)mGC;
        }
        NS_ASSERTION(res, "Unable to return GC");
        return res;
      }
    case NS_NATIVE_COLORMAP:
    default:
      break;
  }
  return NULL;
}


//-------------------------------------------------------------------------
//
// Create a rendering context from this nsWindow
//
//-------------------------------------------------------------------------
nsIRenderingContext* nsWindow::GetRenderingContext()
{
  nsIRenderingContext * ctx = nsnull;

  if (GetNativeData(NS_NATIVE_WIDGET)) {

    nsresult  res;

    static NS_DEFINE_IID(kRenderingContextCID, NS_RENDERING_CONTEXT_CID);
    static NS_DEFINE_IID(kRenderingContextIID, NS_IRENDERING_CONTEXT_IID);

    res = nsRepository::CreateInstance(kRenderingContextCID, nsnull, kRenderingContextIID, (void **)&ctx);

    if (NS_OK == res)
      ctx->Init(mContext, this);

    NS_ASSERTION(NULL != ctx, "Null rendering context");
  }

  return ctx;
}

//-------------------------------------------------------------------------
//
// Return the toolkit this widget was created on
//
//-------------------------------------------------------------------------
nsIToolkit* nsWindow::GetToolkit()
{
  return nsnull;
}


//-------------------------------------------------------------------------
//
// Set the colormap of the window
//
//-------------------------------------------------------------------------
NS_METHOD nsWindow::SetColorMap(nsColorMap *aColorMap)
{
  return NS_OK;
}

//-------------------------------------------------------------------------
//
// Return the used device context
//
//-------------------------------------------------------------------------
nsIDeviceContext* nsWindow::GetDeviceContext()
{
  NS_IF_ADDREF(mContext);
  return mContext;
}

//-------------------------------------------------------------------------
//
// Return the used app shell
//
//-------------------------------------------------------------------------
nsIAppShell* nsWindow::GetAppShell()
{
  NS_IF_ADDREF(mAppShell);
  return mAppShell;
}

//-------------------------------------------------------------------------
//
// Scroll the bits of a window
//
//-------------------------------------------------------------------------
NS_METHOD nsWindow::Scroll(PRInt32 aDx, PRInt32 aDy, nsRect *aClipRect)
{
#if 0
  if (mWidget == nsnull) {
    return NS_ERROR_FAILURE;
  }

   // Scroll all of the child widgets
  Cardinal numChildren;
  XtVaGetValues(mWidget, XtNnumChildren, &numChildren, nsnull);
  if (numChildren > 0) {
    WidgetList children;
    XtVaGetValues(mWidget, XtNchildren, &children, nsnull);
    int i ;
    for(i = 0; i < numChildren; i++) {
      Position x;
      Position y;
      XtVaGetValues(children[i], XtNx, &x, XtNy, &y, nsnull);

      XtVaSetValues(children[i], XmNx, x + aDx, XmNy, y + aDy, nsnull);
    }
  }

  Window  win      = XtWindow(mWidget);
  Display *display = XtDisplay(mWidget);

  if (nsnull != aClipRect) {
    XCopyArea(display, win, win, XDefaultGC(display, 0),
            aClipRect->x, aClipRect->y,
            aClipRect->XMost(),  aClipRect->YMost(), aDx, aDy);
  }

   // Force a repaint
  XEvent evt;
  evt.xgraphicsexpose.type       = GraphicsExpose;
  evt.xgraphicsexpose.send_event = False;
  evt.xgraphicsexpose.display    = display;
  evt.xgraphicsexpose.drawable   = win;
  if (aDy < 0) {
    evt.xgraphicsexpose.x          = 0;
    evt.xgraphicsexpose.y          = mBounds.height+aDy;
    evt.xgraphicsexpose.width      = mBounds.width;
    evt.xgraphicsexpose.height     = -aDy;
  } else {
    evt.xgraphicsexpose.x          = 0;
    evt.xgraphicsexpose.y          = 0;
    evt.xgraphicsexpose.width      = mBounds.width;
    evt.xgraphicsexpose.height     = aDy;
  }
  evt.xgraphicsexpose.count      = 0;
  XSendEvent(display, win, False, ExposureMask, &evt);
  XFlush(display);
#endif
  return NS_OK;
}

NS_METHOD nsWindow::SetBorderStyle(nsBorderStyle aBorderStyle)
{
  return NS_OK;
}

NS_METHOD nsWindow::SetTitle(const nsString& aTitle)
{
  return NS_OK;
}


/**
 * Processes a mouse pressed event
 *
 **/
NS_METHOD nsWindow::AddMouseListener(nsIMouseListener * aListener)
{
  return NS_OK;
}

/**
 * Processes a mouse pressed event
 *
 **/
NS_METHOD nsWindow::AddEventListener(nsIEventListener * aListener)
{
  return NS_OK;
}

PRBool nsWindow::ConvertStatus(nsEventStatus aStatus)
{
  switch(aStatus) {
    case nsEventStatus_eIgnore:
      return(PR_FALSE);
    case nsEventStatus_eConsumeNoDefault:
      return(PR_TRUE);
    case nsEventStatus_eConsumeDoDefault:
      return(PR_FALSE);
    default:
      NS_ASSERTION(0, "Illegal nsEventStatus enumeration value");
      break;
  }
  return(PR_FALSE);
}

//-------------------------------------------------------------------------
//
// Invokes callback and  ProcessEvent method on Event Listener object
//
//-------------------------------------------------------------------------

NS_IMETHODIMP nsWindow::DispatchEvent(nsGUIEvent* event, nsEventStatus & aStatus)
{
#if 0
  NS_ADDREF(event->widget);

  aStatus = nsEventStatus_eIgnore;
  if (nsnull != mEventCallback) {
    aStatus = (*mEventCallback)(event);
  }

  // Dispatch to event listener if event was not consumed
  if ((aStatus != nsEventStatus_eIgnore) && (nsnull != mEventListener)) {
    aStatus = mEventListener->ProcessEvent(*event);
  }
  NS_RELEASE(event->widget);
#endif
  return NS_OK;

}

PRBool nsWindow::DispatchWindowEvent(nsGUIEvent* event)
{
#if 0
  nsEventStatus status;
  DispatchEvent(event, status);
  return ConvertStatus(status);
#endif
}


//-------------------------------------------------------------------------
//
// Deal with all sort of mouse event
//
//-------------------------------------------------------------------------
PRBool nsWindow::DispatchMouseEvent(nsMouseEvent& aEvent)
{
#if 0
  PRBool result = PR_FALSE;
  if (nsnull == mEventCallback && nsnull == mMouseListener) {
    return result;
  }


  // call the event callback
  if (nsnull != mEventCallback) {
    result = DispatchWindowEvent(&aEvent);

    return result;
  }

  if (nsnull != mMouseListener) {
    switch (aEvent.message) {
      case NS_MOUSE_MOVE: {
        /*result = ConvertStatus(mMouseListener->MouseMoved(event));
        nsRect rect;
        GetBounds(rect);
        if (rect.Contains(event.point.x, event.point.y)) {
          if (mCurrentWindow == NULL || mCurrentWindow != this) {
            //printf("Mouse enter");
            mCurrentWindow = this;
          }
        } else {
          //printf("Mouse exit");
        }*/

      } break;

      case NS_MOUSE_LEFT_BUTTON_DOWN:
      case NS_MOUSE_MIDDLE_BUTTON_DOWN:
      case NS_MOUSE_RIGHT_BUTTON_DOWN:
        result = ConvertStatus(mMouseListener->MousePressed(aEvent));
        break;

      case NS_MOUSE_LEFT_BUTTON_UP:
      case NS_MOUSE_MIDDLE_BUTTON_UP:
      case NS_MOUSE_RIGHT_BUTTON_UP:
        result = ConvertStatus(mMouseListener->MouseReleased(aEvent));
        result = ConvertStatus(mMouseListener->MouseClicked(aEvent));
        break;
    } // switch
  }
  return result;
#endif
}


/**
 * Processes an Expose Event
 *
 **/
PRBool nsWindow::OnPaint(nsPaintEvent &event)
{
#if 0
  nsresult result ;

  // call the event callback
  if (mEventCallback) {

    nsRect rr ;

    /*
     * Maybe  ... some day ... somone will pull the invalid rect
     * out of the paint message rather than drawing the whole thing...
     */
    GetBounds(rr);

    rr.x = 0;
    rr.y = 0;

    event.rect = &rr;

    event.renderingContext = nsnull;
    static NS_DEFINE_IID(kRenderingContextCID, NS_RENDERING_CONTEXT_CID);
    static NS_DEFINE_IID(kRenderingContextIID, NS_IRENDERING_CONTEXT_IID);
    if (NS_OK == nsRepository::CreateInstance(kRenderingContextCID,
					      nsnull,
					      kRenderingContextIID,
					      (void **)&event.renderingContext))
      {
        event.renderingContext->Init(mContext, this);
        result = DispatchWindowEvent(&event);
        NS_RELEASE(event.renderingContext);
      }
    else
      {
        result = PR_FALSE;
      }
  }
  return result;
#endif
}


NS_METHOD nsWindow::BeginResizingChildren(void)
{
  return NS_OK;
}

NS_METHOD nsWindow::EndResizingChildren(void)
{
  return NS_OK;
}


void nsWindow::OnDestroy()
{
    // release references to children, device context, toolkit, and app shell
  //XXX: Why is there a problem releasing these
  // NS_IF_RELEASE(mChildren);
  //  NS_IF_RELEASE(mContext);
  //   NS_IF_RELEASE(mToolkit);
  //   NS_IF_RELEASE(mAppShell);
}

PRBool nsWindow::OnResize(nsSizeEvent &aEvent)
{
#if 0
  nsRect* size = aEvent.windowSize;

  if (mEventCallback && !mIgnoreResize) {
    return DispatchWindowEvent(&aEvent);
  }
#endif
  return FALSE;
}

PRBool nsWindow::OnKey(PRUint32 aEventType, PRUint32 aKeyCode, nsKeyEvent* aEvent)
{
#if 0
  if (mEventCallback) {
    return DispatchWindowEvent(aEvent);
  }
  else
#endif
   return FALSE;
}


PRBool nsWindow::DispatchFocus(nsGUIEvent &aEvent)
{
#if 0
  if (mEventCallback) {
    return DispatchWindowEvent(&aEvent);
  }
#endif
 return FALSE;
}

PRBool nsWindow::OnScroll(nsScrollbarEvent & aEvent, PRUint32 cPos)
{
 return FALSE;
}

void nsWindow::SetIgnoreResize(PRBool aIgnore)
{
  mIgnoreResize = aIgnore;
}

PRBool nsWindow::IgnoreResize()
{
  return mIgnoreResize;
}

void nsWindow::SetResizeRect(nsRect& aRect)
{
  mResizeRect = aRect;
}

void nsWindow::GetResizeRect(nsRect* aRect)
{
  aRect->x = mResizeRect.x;
  aRect->y = mResizeRect.y;
  aRect->width = mResizeRect.width;
  aRect->height = mResizeRect.height;
}

void nsWindow::SetResized(PRBool aResized)
{
  mResized = aResized;
}

PRBool nsWindow::GetResized()
{
  return(mResized);
}

void nsWindow::UpdateVisibilityFlag()
{
#if 0
  Widget parent = XtParent(mWidget);

  if (parent) {
    PRUint32 pWidth = 0;
    PRUint32 pHeight = 0;
    XtVaGetValues(parent, XmNwidth, &pWidth, XmNheight, &pHeight, nsnull);
    if ((mBounds.y + mBounds.height) > pHeight) {
      mVisible = PR_FALSE;
      return;
    }

    if (mBounds.y < 0)
     mVisible = PR_FALSE;
  }

  mVisible = PR_TRUE;
#endif
}

void nsWindow::UpdateDisplay()
{
#if 0
    // If not displayed and needs to be displayed
  if ((PR_FALSE==mDisplayed) &&
     (PR_TRUE==mShown) &&
     (PR_TRUE==mVisible)) {
    XtManageChild(mWidget);
    mDisplayed = PR_TRUE;
  }

    // Displayed and needs to be removed
  if (PR_TRUE==mDisplayed) {
    if ((PR_FALSE==mShown) || (PR_FALSE==mVisible)) {
      XtUnmanageChild(mWidget);
      mDisplayed = PR_FALSE;
    }
  }
#endif
}

PRUint32 nsWindow::GetYCoord(PRUint32 aNewY)
{
  if (PR_TRUE==mLowerLeft) {
    return(aNewY - 12 /*KLUDGE fix this later mBounds.height */);
  }
  return(aNewY);
}


//
//-----------------------------------------------------
// Resize handler code for child and main windows.
//-----------------------------------------------------
//

void nsWindow_ResetResize_Callback(XtPointer call_data)
{
#if 0
    nsWindow* widgetWindow = (nsWindow*)call_data;
    widgetWindow->SetResized(PR_FALSE);
#endif
}

void nsWindow_Refresh_Callback(XtPointer call_data)
{
#if 0
    nsWindow* widgetWindow = (nsWindow*)call_data;
    nsRect bounds;
    widgetWindow->GetResizeRect(&bounds);

    nsSizeEvent event;
    event.message = NS_SIZE;
    event.widget  = widgetWindow;
    event.time    = 0; //TBD
    event.windowSize = &bounds;

    widgetWindow->SetBounds(bounds);
    widgetWindow->OnResize(event);
    nsPaintEvent pevent;
    pevent.message = NS_PAINT;
    pevent.widget = widgetWindow;
    pevent.time = 0;
    pevent.rect = (nsRect *)&bounds;
    widgetWindow->OnPaint(pevent);

    XtAppAddTimeOut(gAppContext, 50, (XtTimerCallbackProc)nsWindow_ResetResize_Callback, widgetWindow);
#endif
}

//
// Resize a child window widget. All nsManageWidget's use
// this to resize. The nsManageWidget passes all resize
// request's directly to this function.

extern "C" void nsWindow_ResizeWidget(Widget w)
{
#if 0
  int width = 0;
  int height = 0;
  nsWindow *win = 0;

   // Get the new size for the window
  XtVaGetValues(w, XmNuserData, &win, XmNwidth, &width, XmNheight, &height, nsnull);

   // Setup the resize rectangle for the window.
  nsRect bounds;
  bounds.width = width;
  bounds.height = height;
  bounds.x = 0;
  bounds.y = 0;
  win->SetResizeRect(bounds);

  if (! win->GetResized()) {
    if (win->IsChild()) {
       // Call refresh directly. Don't filter resize events.
      nsWindow_Refresh_Callback(win);
    }
    else {
       // XXX: KLUDGE: Do actual resize later. This lets most
       // of the resize events come through before actually
       // resizing. This is only needed for main (shell)
       // windows. This should be replaced with code that actually
       // Compresses the event queue.
      XtAppAddTimeOut(gAppContext, 250, (XtTimerCallbackProc)nsWindow_Refresh_Callback, win);
    }
  }

  win->SetResized(PR_TRUE);
#endif
}

NS_METHOD nsWindow::SetMenuBar(nsIMenuBar * aMenuBar)
{
  return NS_ERROR_FAILURE;
}

NS_METHOD nsWindow::GetPreferredSize(PRInt32& aWidth, PRInt32& aHeight)
{
  aWidth  = mPreferredWidth;
  aHeight = mPreferredHeight;
  return (mPreferredWidth != 0 && mPreferredHeight != 0)?NS_OK:NS_ERROR_FAILURE;
}

NS_METHOD nsWindow::SetPreferredSize(PRInt32 aWidth, PRInt32 aHeight)
{
  mPreferredWidth  = aWidth;
  mPreferredHeight = aHeight;
  return NS_OK;
}

/**
 * 
 *
 **/
NS_METHOD nsWindow::GetClientBounds(nsRect &aRect)
{
  return GetBounds(aRect);
}


/**
 * Calculates the border width and height
 *
 **/
NS_METHOD nsWindow::GetBorderSize(PRInt32 &aWidth, PRInt32 &aHeight)
{
  nsRect rectWin;
  nsRect rectClient;
  GetBounds(rectWin);
  GetClientBounds(rectClient);

  aWidth  = rectWin.width - rectClient.width;
  aHeight = rectWin.height - rectClient.height;

  return NS_OK;
}

