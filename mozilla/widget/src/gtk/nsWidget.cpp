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


#include "nsWidget.h"

#include "nsGtkEventHandler.h"
#include "nsIAppShell.h"
#include "nsIComponentManager.h"
#include "nsIDeviceContext.h"
#include "nsIFontMetrics.h"
#include "nsILookAndFeel.h"
#include "nsToolkit.h"
#include "nsWidgetsCID.h"
#include "nsGfxCIID.h"
#include <gdk/gdkx.h>
#include "nsIRollupListener.h"
#include "nsIMenuRollup.h"
#include "nsIServiceManager.h"
#include "nsIDragSessionGTK.h"
#include "nsIDragService.h"

#include "nsGtkUtils.h" // for nsGtkUtils::gdk_keyboard_get_modifiers()

#include "nsIPref.h"
#include "nsGtkIMEHelper.h"

static NS_DEFINE_CID(kLookAndFeelCID, NS_LOOKANDFEEL_CID);
static NS_DEFINE_CID(kRegionCID, NS_REGION_CID);
static NS_DEFINE_CID(kPrefServiceCID, NS_PREF_CID);
static NS_DEFINE_IID(kCDragServiceCID,  NS_DRAGSERVICE_CID);

// keeping track of a list of simultaneously modal widgets
class ModalWidgetList {
public:
  ModalWidgetList(nsWidget *aWidget);
  ~ModalWidgetList();

  static PRBool Find(nsWidget *aWidget);
  static void   Append(nsWidget *aWidget);
  static void   Remove(nsWidget *aWidget);
  static void   RemoveLast(void);
  static void   Suppress(PRBool aSuppress);

private:
  nsWidget        *mWidget;
  ModalWidgetList *mNext,
                  *mPrev,
                  *mLast; // valid only for head of list
};

static nsWidget *GetShellWidget(GdkWindow *gdkWindow);

nsILookAndFeel *nsWidget::sLookAndFeel = nsnull;
PRUint32 nsWidget::sWidgetCount = 0;

// this is the nsWindow with the focus
nsWidget *nsWidget::focusWindow = NULL;

#ifdef USE_XIM
GdkFont *nsWidget::gPreeditFontset = nsnull;
GdkFont *nsWidget::gStatusFontset = nsnull;
GdkIMStyle nsWidget::gInputStyle = (GdkIMStyle)nsnull;
#endif // USE_XIM 
// this is the last time that an event happened.  we keep this
// around so that we can synth drag events properly
guint32 nsWidget::sLastEventTime = 0;

PRBool nsWidget::OnInput(nsInputEvent &aEvent)
{

  PRBool    ret = PR_FALSE;
  PRBool    releaseWidget = PR_FALSE;
  nsWidget *widget = NULL;

  // rewrite the key event to the window with 'de focus
  if (focusWindow) {
    widget = focusWindow;
    NS_ADDREF(widget);
    aEvent.widget = focusWindow;
    releaseWidget = PR_TRUE;
  }
  if (mEventCallback) {
    ret = DispatchWindowEvent(&aEvent);
  }

  if (releaseWidget)
    NS_RELEASE(widget);

  return ret;
}

void nsWidget::SetLastEventTime(guint32 aTime)
{
  sLastEventTime = aTime;
}

void nsWidget::GetLastEventTime(guint32 *aTime)
{
  if (aTime)
    *aTime = sLastEventTime;
}

#ifdef USE_XIM
nsresult nsWidget::KillICSpotTimer ()
{
   if(mICSpotTimer)
   {
     mICSpotTimer->Cancel();
     NS_RELEASE(mICSpotTimer);
     mICSpotTimer = nsnull;
   }
   return NS_OK;
}
nsresult nsWidget::PrimeICSpotTimer ()
{
   KillICSpotTimer();
   nsresult err = NS_NewTimer(&mICSpotTimer);
   if(NS_FAILED(err))
       return err;
   mICSpotTimer->Init(ICSpotCallback, this, 1000);
   return NS_OK;
}
void nsWidget::ICSpotCallback(nsITimer * aTimer, void * aClosure)
{
   nsWidget *widget= NS_REINTERPRET_CAST(nsWidget*, aClosure);
   if( ! widget) return;
   nsresult res = widget->UpdateICSpot();
   if(NS_SUCCEEDED(res))
   {
      widget->PrimeICSpotTimer();
   }
}
nsresult nsWidget::UpdateICSpot()
{
   // set spot location
   nsCompositionEvent compEvent;
   compEvent.widget = (nsWidget*)this;
   compEvent.point.x = 0;
   compEvent.point.y = 0;
   compEvent.time = 0;
   compEvent.message = NS_COMPOSITION_QUERY;
   compEvent.eventStructType = NS_COMPOSITION_QUERY;
   compEvent.compositionMessage = NS_COMPOSITION_QUERY;
   static gint oldx =0;
   static gint oldy =0;
   compEvent.theReply.mCursorPosition.x=-1;
   compEvent.theReply.mCursorPosition.y=-1;
   this->OnComposition(compEvent);
   // set SpotLocation
   if((compEvent.theReply.mCursorPosition.x < 0) &&
      (compEvent.theReply.mCursorPosition.y < 0))
     return NS_ERROR_FAILURE;
   if((compEvent.theReply.mCursorPosition.x != oldx)||
      (compEvent.theReply.mCursorPosition.y != oldy))
   {
       nsPoint spot;
       spot.x = compEvent.theReply.mCursorPosition.x;
       spot.y = compEvent.theReply.mCursorPosition.y + 
                compEvent.theReply.mCursorPosition.height;
       SetXICBaseFontSize( compEvent.theReply.mCursorPosition.height - 1);
       SetXICSpotLocation(spot);
       oldx = spot.x;
       oldy = spot.y;
   } 
   return NS_OK;
}
#endif // USE_XIM 

nsCOMPtr<nsIRollupListener> nsWidget::gRollupListener;
nsWeakPtr          nsWidget::gRollupWidget;
PRBool             nsWidget::gRollupConsumeRollupEvent = PR_FALSE;

PRBool             nsWidget::mGDKHandlerInstalled = PR_FALSE;
PRBool             nsWidget::mTimeCBSet = PR_FALSE;

#ifdef NS_DEBUG
// debugging window
static GtkWidget *debugTopLevel = NULL;
static GtkWidget *debugBox = NULL;
static GtkWidget *debugEntryBox = NULL;
static GtkWidget *debugButton = NULL;
static GtkWidget *debugCheckBox = NULL;
nsWidget  *nsWidget::debugWidget = NULL;
PRBool     nsWidget::sDebugFeedback = PR_FALSE;
static PRBool     debugCheckedDebugWindow = PR_FALSE;
static PRBool     debugCallbackRegistered = PR_FALSE;

static void      debugHandleActivate(GtkEditable *editable,
                                     gpointer user_data);
static void      debugHandleClicked (GtkButton   *button,
                                     gpointer user_data);
static void      debugHandleToggle  (GtkToggleButton *button,
                                     gpointer user_data);
static void      debugSetupWindow   (void);
static void      debugDestroyWindow (void);
static int       debugWindowPrefChanged (const char *newpref, void *data);
static void      debugRegisterCallback  (void);
static gint      debugHandleWindowClose(GtkWidget *window, void *data);
const char      *debugPrefName = "nglayout.widget.debugWindow";
#endif /* NS_DEBUG */

//
// Keep track of the last widget being "dragged"
//
nsWidget *nsWidget::sButtonMotionTarget = NULL;
gint nsWidget::sButtonMotionRootX = -1;
gint nsWidget::sButtonMotionRootY = -1;
gint nsWidget::sButtonMotionWidgetX = -1;
gint nsWidget::sButtonMotionWidgetY = -1;

//#undef DEBUG_pavlov

nsWidget::nsWidget()
{
  // XXX Shouldn't this be done in nsBaseWidget?
  //  NS_INIT_REFCNT();

  if (!sLookAndFeel) {
    if (NS_OK != nsComponentManager::CreateInstance(kLookAndFeelCID,
                                                    nsnull,
                                                    NS_GET_IID(nsILookAndFeel),
                                                    (void**)&sLookAndFeel))
      sLookAndFeel = nsnull;
  }

  if (sLookAndFeel)
    sLookAndFeel->GetColor(nsILookAndFeel::eColor_WindowBackground,
                           mBackground);

  mGrabTime = 0;
  mWidget = nsnull;
  mMozBox = 0;
  mParent = nsnull;
  mPreferredWidth  = 0;
  mPreferredHeight = 0;
  mShown = PR_FALSE;
  mBounds.x = 0;
  mBounds.y = 0;
  mBounds.width = 0;
  mBounds.height = 0;
  mIsDragDest = PR_FALSE;
  mIsToplevel = PR_FALSE;

  mUpdateArea = do_CreateInstance(kRegionCID);
  if (mUpdateArea) {
    mUpdateArea->Init();
    mUpdateArea->SetTo(0, 0, 0, 0);
  }
  
  sWidgetCount++;

#ifdef USE_XIM
  mIMEEnable = PR_TRUE;
  mIMEIsDeactivating = PR_FALSE;
  mICPerShell = PR_TRUE;       // default
  mXIC = 0;

  mIMEShellWidget = 0;
  mIMEChildWidget = 0;
  mIMECallComposeStart = PR_FALSE;
  mIMECallComposeEnd = PR_TRUE;

  mICSpotTimer = nsnull;
#endif // USE_XIM 
  mIMECompositionUniString = nsnull;
  mIMECompositionUniStringSize = 0;

  mListenForResizes = PR_FALSE;
  mHasFocus = PR_FALSE;
  if (mGDKHandlerInstalled == PR_FALSE) {
    mGDKHandlerInstalled = PR_TRUE;
    // It is most convenient for us to intercept our events after
    // they have been converted to GDK, but before GTK+ gets them
    gdk_event_handler_set (handle_gdk_event, NULL, NULL);
  }
  if (mTimeCBSet == PR_FALSE) {
    mTimeCBSet = PR_TRUE;
    nsCOMPtr<nsIDragService> dragService;
    nsresult rv = nsServiceManager::GetService(kCDragServiceCID,
                                               nsIDragService::GetIID(),
                                               (nsISupports **)&dragService);
    if (NS_FAILED(rv)) {
      g_print("*** warning: failed to get the drag service. this is a _bad_ thing.\n");
      mTimeCBSet = PR_FALSE;
    }
    nsCOMPtr<nsIDragSessionGTK> dragServiceGTK;
    dragServiceGTK = do_QueryInterface(dragService);
    if (!dragServiceGTK) {
      mTimeCBSet = PR_FALSE;
      return;
    }
    dragServiceGTK->SetTimeCallback(nsWidget::GetLastEventTime);
  }
#ifdef NS_DEBUG
  // see if we need to set up the debugging window
  if (!debugCheckedDebugWindow) {
    debugSetupWindow();
  }
  // this will set up the callback for when the debug
  // pref changes
  if (!debugCallbackRegistered) {
    debugRegisterCallback();
  }
#endif /* NS_DEBUG */
}

nsWidget::~nsWidget()
{
#ifdef USE_XIM
  mXIC = 0;
  KillICSpotTimer();
#endif // USE_XIM 

#ifdef NOISY_DESTROY
  IndentByDepth(stdout);
  printf("nsWidget::~nsWidget:%p\n", this);
#endif

  // it's safe to always call Destroy() because it will only allow itself
  // to be called once
  Destroy();

  if (!--sWidgetCount) {
    NS_IF_RELEASE(sLookAndFeel);
  }

  if (mIMECompositionUniString) {
    delete[] mIMECompositionUniString;
    mIMECompositionUniString = nsnull;
  }
  NS_ASSERTION(!ModalWidgetList::Find(this), "destroying widget without first clearing modality.");
#ifdef NS_DEBUG
  if (mIsToplevel) {
    g_print("nsWidget::~nsWidget() of toplevel: %d widgets still exist.\n", sWidgetCount);
  }
#endif
}


//-------------------------------------------------------------------------
//
// nsISupport stuff
//
//-------------------------------------------------------------------------
NS_IMPL_ISUPPORTS_INHERITED2(nsWidget, nsBaseWidget, nsIKBStateControl, nsISupportsWeakReference)

NS_IMETHODIMP nsWidget::WidgetToScreen(const nsRect& aOldRect, nsRect& aNewRect)
{
  gint x;
  gint y;

  if (mWidget)
  {
    if (mWidget->window)
    {
      gdk_window_get_origin(mWidget->window, &x, &y);
      aNewRect.x = x + aOldRect.x;
      aNewRect.y = y + aOldRect.y;
    }
    else
      return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

NS_IMETHODIMP nsWidget::ScreenToWidget(const nsRect& aOldRect, nsRect& aNewRect)
{
#ifdef DEBUG_pavlov
    g_print("nsWidget::ScreenToWidget\n");
#endif
    NS_NOTYETIMPLEMENTED("nsWidget::ScreenToWidget");
    return NS_OK;
}

#ifdef DEBUG
void
nsWidget::IndentByDepth(FILE* out)
{
  PRInt32 depth = 0;
  nsWidget* parent = (nsWidget*)mParent.get();
  while (parent) {
    parent = (nsWidget*) parent->mParent.get();
    depth++;
  }
  while (--depth >= 0) fprintf(out, "  ");
}
#endif

//-------------------------------------------------------------------------
//
// Close this nsWidget
//
//-------------------------------------------------------------------------

NS_IMETHODIMP nsWidget::Destroy(void)
{
  //  printf("%p nsWidget::Destroy()\n", this);
  // make sure we don't call this more than once.
  if (mIsDestroying)
    return NS_OK;

  // we don't want people sending us events if we are the button motion target
  if (sButtonMotionTarget == this)
    sButtonMotionTarget = nsnull;

  // ok, set our state
  mIsDestroying = PR_TRUE;

  // call in and clean up any of our base widget resources
  // are released
  nsBaseWidget::Destroy();

  // destroy our native windows
  DestroyNative();

  // make sure to call the OnDestroy if it hasn't been called yet
  if (mOnDestroyCalled == PR_FALSE)
    OnDestroy();

  // make sure no callbacks happen
  mEventCallback = nsnull;

  return NS_OK;
}

// this is the function that will destroy the native windows for this widget.

/* virtual */
void nsWidget::DestroyNative(void)
{
  if (mMozBox) {
    // destroying the mMozBox will also destroy the mWidget in question.
    ::gtk_widget_destroy(mMozBox);
    mWidget = NULL;
    mMozBox = NULL;
  }
}

// make sure that we clean up here

void nsWidget::OnDestroy()
{
  mOnDestroyCalled = PR_TRUE;
  // release references to children, device context, toolkit + app shell
  nsBaseWidget::OnDestroy();

  nsCOMPtr<nsIWidget> kungFuDeathGrip = this;
  DispatchStandardEvent(NS_DESTROY);
}

gint
nsWidget::DestroySignal(GtkWidget* aGtkWidget, nsWidget* aWidget)
{
  aWidget->OnDestroySignal(aGtkWidget);
  return PR_TRUE;
}

void
nsWidget::SuppressModality(PRBool aSuppress)
{
  ModalWidgetList::Suppress(aSuppress); 
}


void
nsWidget::OnDestroySignal(GtkWidget* aGtkWidget)
{
  OnDestroy();
}

//-------------------------------------------------------------------------
//
// Get this nsWidget parent
//
//-------------------------------------------------------------------------

nsIWidget* nsWidget::GetParent(void)
{
  nsIWidget *ret;
  ret = mParent;
  NS_IF_ADDREF(ret);
  return ret;
}

//-------------------------------------------------------------------------
//
// Hide or show this component
//
//-------------------------------------------------------------------------

NS_IMETHODIMP nsWidget::Show(PRBool bState)
{
  if (!mWidget)
    return NS_OK; // Will be null durring printing

  if (bState) {
    gtk_widget_show(mWidget);
    gtk_widget_show(mMozBox);
  }
  else {
    gtk_widget_hide(mMozBox);
    gtk_widget_hide(mWidget);
  }

  mShown = bState;

  return NS_OK;
}


NS_IMETHODIMP nsWidget::CaptureRollupEvents(nsIRollupListener * aListener, PRBool aDoCapture, PRBool aConsumeRollupEvent)
{
  return NS_OK;
}

NS_IMETHODIMP nsWidget::SetModal(PRBool aModal)
{
  GtkWindow *topWindow;

  topWindow = GetTopLevelWindow();

  if (!topWindow) {
    if (!aModal) {
      ModalWidgetList::RemoveLast();
    }
    return NS_ERROR_FAILURE;
  }

  if (aModal) {
    ModalWidgetList::Append(this);
    gtk_window_set_modal(topWindow, TRUE);
  } else {
    ModalWidgetList::Remove(this);
    gtk_window_set_modal(topWindow, FALSE);
  }

	return NS_OK;
}

NS_IMETHODIMP nsWidget::IsVisible(PRBool &aState)
{
  if (mWidget)
    aState = GTK_WIDGET_VISIBLE(mWidget);
  else
    aState = PR_FALSE;

  return NS_OK;
}

//-------------------------------------------------------------------------
//
// Move this component
//
//-------------------------------------------------------------------------

NS_IMETHODIMP nsWidget::Move(PRInt32 aX, PRInt32 aY)
{
  if (mMozBox) 
  {
    gtk_mozbox_set_position(GTK_MOZBOX(mMozBox), aX, aY);
  }

  return NS_OK;
}

NS_IMETHODIMP nsWidget::Resize(PRInt32 aWidth, PRInt32 aHeight, PRBool aRepaint)
{
#if 0
  printf("nsWidget::Resize %s (%p) to %d %d\n",
         (const char *) debug_GetName(mWidget),
         this,
         aWidth, aHeight);
#endif
  mBounds.width  = aWidth;
  mBounds.height = aHeight;

  if (mWidget)
    gtk_widget_set_usize(mWidget, aWidth, aHeight);

  return NS_OK;
}

NS_IMETHODIMP nsWidget::Resize(PRInt32 aX, PRInt32 aY, PRInt32 aWidth,
                           PRInt32 aHeight, PRBool aRepaint)
{
  Move(aX, aY);
  Resize(aWidth,aHeight,aRepaint);
  return NS_OK;
}

//-------------------------------------------------------------------------
//
// Send a resize message to the listener
//
//-------------------------------------------------------------------------
PRBool nsWidget::OnResize(nsSizeEvent event)
{

  mBounds.width = event.mWinWidth;
  mBounds.height = event.mWinHeight;

  return DispatchWindowEvent(&event);
}


PRBool nsWidget::OnResize(nsRect &aRect)
{
  nsSizeEvent event;

  InitEvent(event, NS_SIZE);
  event.eventStructType = NS_SIZE_EVENT;

  nsRect *foo = new nsRect(0, 0, aRect.width, aRect.height);
  event.windowSize = foo;

  event.point.x = 0;
  event.point.y = 0;
  event.mWinWidth = aRect.width;
  event.mWinHeight = aRect.height;
  
  mBounds.width = aRect.width;
  mBounds.height = aRect.height;

  NS_ADDREF_THIS();
  PRBool result = OnResize(event);
  NS_RELEASE_THIS();

  return result;
}


//------
// Move
//------
PRBool nsWidget::OnMove(PRInt32 aX, PRInt32 aY)
{
    nsGUIEvent event;
#if 0
    printf("nsWidget::OnMove %s (%p)\n",
           (const char *) debug_GetName(mWidget),
           this);
#endif
    InitEvent(event, NS_MOVE);
    event.point.x = aX;
    event.point.y = aY;
    event.eventStructType = NS_GUI_EVENT;
    PRBool result = DispatchWindowEvent(&event);
    return result;
}

//-------------------------------------------------------------------------
//
// Enable/disable this component
//
//-------------------------------------------------------------------------
NS_IMETHODIMP nsWidget::Enable(PRBool bState)
{
  if (mWidget)
  {
    if (GTK_WIDGET_SENSITIVE(mWidget) == bState)
      return NS_OK;
    gtk_widget_set_sensitive(mWidget, bState);
  }

  return NS_OK;
}

//-------------------------------------------------------------------------
//
// Give the focus to this component
//
//-------------------------------------------------------------------------
NS_IMETHODIMP nsWidget::SetFocus(void)
{
  // call this so that any cleanup will happen that needs to...
  LooseFocus();

  if (mWidget)
  {
    if (!GTK_WIDGET_HAS_FOCUS(mWidget))
      gtk_widget_grab_focus(mWidget);
  }

  return NS_OK;
}

/* virtual */ void
nsWidget::LooseFocus(void)
{
  // doesn't do anything.  needed for nsWindow housekeeping, really.
  if (mHasFocus == PR_FALSE) {
    return;
  }
  
  focusWindow = NULL;
  mHasFocus = PR_FALSE;

  // we don't need to send out a focus out event from here because
  // when the gtk widget goes out of focus, it will get a focus_out
  // event

}

//-------------------------------------------------------------------------
//
// Get this component font
//
//-------------------------------------------------------------------------
nsIFontMetrics *nsWidget::GetFont(void)
{
  NS_NOTYETIMPLEMENTED("nsWidget::GetFont");
  return nsnull;
}

//-------------------------------------------------------------------------
//
// Set this component font
//
//-------------------------------------------------------------------------
NS_IMETHODIMP nsWidget::SetFont(const nsFont &aFont)
{
  nsCOMPtr<nsIFontMetrics> fontMetrics;
  mContext->GetMetricsFor(aFont, *getter_AddRefs(fontMetrics));

  if (!fontMetrics)
    return NS_ERROR_FAILURE;

  nsFontHandle fontHandle;
  fontMetrics->GetFontHandle(fontHandle);
  if (fontHandle) {
    // FIXME avoid fontset problems....
    if (((GdkFont*)fontHandle)->type == GDK_FONT_FONTSET) {
      g_print("nsWidget:SetFont - got a FontSet.. ignoring\n");
      return NS_ERROR_FAILURE;
    }

    if (mWidget) 
      SetFontNative((GdkFont *)fontHandle);
  }

  return NS_OK;
}

//-------------------------------------------------------------------------
//
// Set the background color
//
//-------------------------------------------------------------------------

NS_IMETHODIMP nsWidget::SetBackgroundColor(const nscolor &aColor)
{
  nsBaseWidget::SetBackgroundColor(aColor);

  if (nsnull != mWidget) {
    GdkColor color_nor, color_bri, color_dark;

    NSCOLOR_TO_GDKCOLOR(aColor, color_nor);
    NSCOLOR_TO_GDKCOLOR(NS_BrightenColor(aColor), color_bri);
    NSCOLOR_TO_GDKCOLOR(NS_DarkenColor(aColor), color_dark);

    //    gdk_color.red = 256 * NS_GET_R(aColor);
    // gdk_color.green = 256 * NS_GET_G(aColor);
    // gdk_color.blue = 256 * NS_GET_B(aColor);
    // gdk_color.pixel ?

    // calls virtual native set color
    SetBackgroundColorNative(&color_nor, &color_bri, &color_dark);

#if 0
    GtkStyle *style = gtk_style_copy(mWidget->style);
  
    style->bg[GTK_STATE_NORMAL]=gdk_color;
    // other states too? (GTK_STATE_ACTIVE, GTK_STATE_PRELIGHT,
    //               GTK_STATE_SELECTED, GTK_STATE_INSENSITIVE)
    gtk_widget_set_style(mWidget, style);
    gtk_style_unref(style);
#endif
  }

  return NS_OK;
}

//-------------------------------------------------------------------------
//
// Set this component cursor
//
//-------------------------------------------------------------------------
NS_IMETHODIMP nsWidget::SetCursor(nsCursor aCursor)
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

      case eCursor_sizeWE:
      case eCursor_sizeNS:
        newCursor = gdk_cursor_new(GDK_TCROSS);
        break;
      
      case eCursor_sizeNW:
      case eCursor_sizeSE:
      case eCursor_sizeNE:
      case eCursor_sizeSW:
        // XXX: these resize cursors need to be implemented
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
      ::gdk_window_set_cursor(mWidget->window, newCursor);
      ::gdk_cursor_destroy(newCursor);
    }
  }
  return NS_OK;
}

#define CAPS_LOCK_IS_ON \
(nsWidget::sDebugFeedback && (nsGtkUtils::gdk_keyboard_get_modifiers() & GDK_LOCK_MASK))

NS_IMETHODIMP nsWidget::Invalidate(PRBool aIsSynchronous)
{
  if (!mWidget)
    return NS_OK; // mWidget will be null during printing. 

  if (!GTK_IS_WIDGET(mWidget))
    return NS_ERROR_FAILURE;

  if (!GTK_WIDGET_REALIZED(mWidget) || !GTK_WIDGET_VISIBLE(mWidget))
    return NS_ERROR_FAILURE;

#ifdef NS_DEBUG
  if (CAPS_LOCK_IS_ON)
  {
    debug_DumpInvalidate(stdout,
                         this,
                         nsnull,
                         aIsSynchronous,
                         debug_GetName(mWidget),
                         debug_GetRenderXID(mWidget));
  }
#endif // NS_DEBUG

  mUpdateArea->SetTo(0, 0, mBounds.width, mBounds.height);

  if (aIsSynchronous) {
    ::gtk_widget_draw(mWidget, (GdkRectangle *) NULL);
  } else {
    ::gtk_widget_queue_draw(mWidget);
  }

  return NS_OK;
}

NS_IMETHODIMP nsWidget::Invalidate(const nsRect & aRect, PRBool aIsSynchronous)
{
  if (!mWidget)
    return NS_OK;  // mWidget is null during printing

  if (!GTK_IS_WIDGET(mWidget))
    return NS_ERROR_FAILURE;

  if (!GTK_WIDGET_REALIZED(mWidget) || !GTK_WIDGET_VISIBLE(mWidget))
    return NS_ERROR_FAILURE;

  mUpdateArea->Union(aRect.x, aRect.y, aRect.width, aRect.height);

#ifdef NS_DEBUG
  if (CAPS_LOCK_IS_ON)
  {
    debug_DumpInvalidate(stdout,
                         this,
                         &aRect,
                         aIsSynchronous,
                         debug_GetName(mWidget),
                         debug_GetRenderXID(mWidget));
  }
#endif // NS_DEBUG

  if (aIsSynchronous)
  {
    GdkRectangle nRect;
    NSRECT_TO_GDKRECT(aRect, nRect);

    gtk_widget_draw(mWidget, &nRect);
  }
  else
  {
    gtk_widget_queue_draw_area(mWidget,
                               aRect.x, aRect.y,
                               aRect.width, aRect.height);
  }

  return NS_OK;
}


NS_IMETHODIMP nsWidget::InvalidateRegion(const nsIRegion *aRegion, PRBool aIsSynchronous)
{
  nsRegionRectSet *regionRectSet = nsnull;

  if (!GTK_IS_WIDGET(mWidget))
    return NS_ERROR_FAILURE;

  if (!GTK_WIDGET_REALIZED(mWidget) || !GTK_WIDGET_VISIBLE(mWidget))
    return NS_ERROR_FAILURE;

  mUpdateArea->Union(*aRegion);

  if (NS_FAILED(mUpdateArea->GetRects(&regionRectSet)))
  {
    return NS_ERROR_FAILURE;
  }

  PRUint32 len;
  PRUint32 i;

  len = regionRectSet->mRectsLen;

  for (i=0;i<len;++i)
  {
    nsRegionRect *r = &(regionRectSet->mRects[i]);


#ifdef NS_DEBUG
    if (CAPS_LOCK_IS_ON)
    {
      nsRect rect(r->x, r->y, r->width, r->height);
      debug_DumpInvalidate(stdout,
                           this,
                           &rect,
                           aIsSynchronous,
                           debug_GetName(mWidget),
                           debug_GetRenderXID(mWidget));
    }
#endif // NS_DEBUG


    if (aIsSynchronous)
    {
      GdkRectangle nRect;
      nRect.x = r->x;
      nRect.y = r->y;
      nRect.width = r->width;
      nRect.height = r->height;
      gtk_widget_draw(mWidget, &nRect);
    } else {
      gtk_widget_queue_draw_area(mWidget,
                                 r->x, r->y,
                                 r->width, r->height);
    }
  }

  // drop the const.. whats the right thing to do here?
  ((nsIRegion*)aRegion)->FreeRects(regionRectSet);

  return NS_OK;
}


NS_IMETHODIMP nsWidget::Update(void)
{
  if (!mWidget)
    return NS_OK;

  if (!GTK_IS_WIDGET(mWidget))
    return NS_ERROR_FAILURE;

  if (!GTK_WIDGET_REALIZED(mWidget) || !GTK_WIDGET_VISIBLE(mWidget))
    return NS_ERROR_FAILURE;

  //  printf("nsWidget::Update()\n");

  // this will Union() again, but so what?
  return InvalidateRegion(mUpdateArea, PR_TRUE);
}

//-------------------------------------------------------------------------
//
// Return some native data according to aDataType
//
//-------------------------------------------------------------------------
void *nsWidget::GetNativeData(PRUint32 aDataType)
{
  switch(aDataType) {
  case NS_NATIVE_WINDOW:
    if (mWidget) {
      return (void *)mWidget->window;
    }
    break;

  case NS_NATIVE_DISPLAY:
    return (void *)GDK_DISPLAY();

  case NS_NATIVE_WIDGET:
  case NS_NATIVE_PLUGIN_PORT:
    if (mWidget) {
      return (void *)mWidget;
    }
    break;

  case NS_NATIVE_GRAPHIC:
    /* GetSharedGC ups the ref count on the GdkGC so make sure you release
     * it afterwards. */
    NS_ASSERTION(nsnull != mToolkit, "NULL toolkit, unable to get a GC");
    return (void *)NS_STATIC_CAST(nsToolkit*,mToolkit)->GetSharedGC();

  default:
    g_print("nsWidget::GetNativeData(%i) - weird value\n", aDataType);
    break;
  }
  return nsnull;
}

//-------------------------------------------------------------------------
//
// Set the colormap of the window
//
//-------------------------------------------------------------------------
NS_IMETHODIMP nsWidget::SetColorMap(nsColorMap *aColorMap)
{
  return NS_OK;
}

NS_IMETHODIMP nsWidget::BeginResizingChildren(void)
{
  return NS_OK;
}

NS_IMETHODIMP nsWidget::EndResizingChildren(void)
{
  return NS_OK;
}

NS_IMETHODIMP nsWidget::GetPreferredSize(PRInt32& aWidth, PRInt32& aHeight)
{
  aWidth  = mPreferredWidth;
  aHeight = mPreferredHeight;
  return (mPreferredWidth != 0 && mPreferredHeight != 0)?NS_OK:NS_ERROR_FAILURE;
}

NS_IMETHODIMP nsWidget::SetPreferredSize(PRInt32 aWidth, PRInt32 aHeight)
{
  mPreferredWidth  = aWidth;
  mPreferredHeight = aHeight;
  return NS_OK;
}

NS_IMETHODIMP nsWidget::SetTitle(const nsString &aTitle)
{
  gtk_widget_set_name(mWidget, "foo");
  return NS_OK;
}

nsresult nsWidget::CreateWidget(nsIWidget *aParent,
                                const nsRect &aRect,
                                EVENT_CALLBACK aHandleEventFunction,
                                nsIDeviceContext *aContext,
                                nsIAppShell *aAppShell,
                                nsIToolkit *aToolkit,
                                nsWidgetInitData *aInitData,
                                nsNativeWidget aNativeParent)
{
  GtkObject *parentWidget = nsnull;

#ifdef NOISY_DESTROY
  if (aParent)
    g_print("nsWidget::CreateWidget (%p) nsIWidget parent\n",
            this);
  else if (aNativeParent)
    g_print("nsWidget::CreateWidget (%p) native parent\n",
            this);
  else if(aAppShell)
    g_print("nsWidget::CreateWidget (%p) nsAppShell parent\n",
            this);
#endif

  gtk_widget_push_colormap(gdk_rgb_get_cmap());
  gtk_widget_push_visual(gdk_rgb_get_visual());

  nsIWidget *baseParent = aInitData &&
    (aInitData->mWindowType == eWindowType_dialog ||
     aInitData->mWindowType == eWindowType_toplevel) ?
    nsnull : aParent;

  BaseCreate(baseParent, aRect, aHandleEventFunction, aContext,
             aAppShell, aToolkit, aInitData);

  mParent = aParent;

  if (aNativeParent) {
    parentWidget = GTK_OBJECT(aNativeParent);
    // we've got a native parent so listen for resizes
    mListenForResizes = PR_TRUE;
  } else if (aParent) {
    // this ups the refcount of the gtk widget, we must unref later.
    parentWidget = GTK_OBJECT(aParent->GetNativeData(NS_NATIVE_WIDGET));
  }

  mBounds = aRect;
  CreateNative (parentWidget);

  Resize(aRect.width, aRect.height, PR_FALSE);

  gtk_widget_pop_colormap();
  gtk_widget_pop_visual();

  if (mWidget) {

    InstallButtonPressSignal(mWidget);
    InstallButtonReleaseSignal(mWidget);
    
    // InstallMotionNotifySignal(mWidget);
    
    InstallEnterNotifySignal(mWidget);
    InstallLeaveNotifySignal(mWidget);
    
    // Initialize this window instance as a drag target.
    gtk_drag_dest_set (mWidget,
                       (GtkDestDefaults)0,
                       NULL,
                       0,
                       (GdkDragAction)0);
    
    // Drag & Drop events.
    InstallDragBeginSignal(mWidget);
    InstallDragLeaveSignal(mWidget);
    InstallDragMotionSignal(mWidget);
    InstallDragDropSignal(mWidget);
    InstallDragDataReceived(mWidget);
    
    // Focus
    InstallFocusInSignal(mWidget);
    InstallFocusOutSignal(mWidget);
    
  }

  DispatchStandardEvent(NS_CREATE);
  InitCallbacks();

  if (mWidget) {
    // Add in destroy callback
    gtk_signal_connect(GTK_OBJECT(mWidget),
                       "destroy",
                       GTK_SIGNAL_FUNC(DestroySignal),
                       this);
  }

  return NS_OK;
}

//-------------------------------------------------------------------------
//
// create with nsIWidget parent
//
//-------------------------------------------------------------------------

NS_IMETHODIMP nsWidget::Create(nsIWidget *aParent,
                           const nsRect &aRect,
                           EVENT_CALLBACK aHandleEventFunction,
                           nsIDeviceContext *aContext,
                           nsIAppShell *aAppShell,
                           nsIToolkit *aToolkit,
                           nsWidgetInitData *aInitData)
{
  return CreateWidget(aParent, aRect, aHandleEventFunction,
                      aContext, aAppShell, aToolkit, aInitData,
                      nsnull);
}

//-------------------------------------------------------------------------
//
// create with a native parent
//
//-------------------------------------------------------------------------
NS_IMETHODIMP nsWidget::Create(nsNativeWidget aParent,
                           const nsRect &aRect,
                           EVENT_CALLBACK aHandleEventFunction,
                           nsIDeviceContext *aContext,
                           nsIAppShell *aAppShell,
                           nsIToolkit *aToolkit,
                           nsWidgetInitData *aInitData)
{
  return CreateWidget(nsnull, aRect, aHandleEventFunction,
                      aContext, aAppShell, aToolkit, aInitData,
                      aParent);
}

//-------------------------------------------------------------------------
//
// Initialize all the Callbacks
//
//-------------------------------------------------------------------------
void nsWidget::InitCallbacks(char *aName)
{
}

void nsWidget::ConvertToDeviceCoordinates(nscoord &aX, nscoord &aY)
{

}

void nsWidget::InitEvent(nsGUIEvent& event, PRUint32 aEventType, nsPoint* aPoint)
{
  event.widget = this;

  GdkEventConfigure *ge;
  ge = (GdkEventConfigure*)gtk_get_current_event();

  if (aPoint == nsnull) {     // use the point from the event
    // get the message position in client coordinates and in twips

    if (ge != nsnull) {
      //       ::ScreenToClient(mWnd, &cpos);
      event.point.x = PRInt32(ge->x);
      event.point.y = PRInt32(ge->y);
    } else { 
      event.point.x = 0;
      event.point.y = 0;
    }  
  }    
  else {                      // use the point override if provided
    event.point.x = aPoint->x;
    event.point.y = aPoint->y;
  }

  event.time = gdk_event_get_time((GdkEvent*)ge);
  event.message = aEventType;

  //    mLastPoint.x = event.point.x;
  //    mLastPoint.y = event.point.y;
}

PRBool nsWidget::ConvertStatus(nsEventStatus aStatus)
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
  return PR_FALSE;
}

PRBool nsWidget::DispatchWindowEvent(nsGUIEvent* event)
{
  nsEventStatus status;
  DispatchEvent(event, status);
  return ConvertStatus(status);
}

//-------------------------------------------------------------------------
//
// Dispatch standard event
//
//-------------------------------------------------------------------------

PRBool nsWidget::DispatchStandardEvent(PRUint32 aMsg)
{
  nsGUIEvent event;
  event.eventStructType = NS_GUI_EVENT;
  InitEvent(event, aMsg);
  PRBool result = DispatchWindowEvent(&event);
  return result;
}

PRBool nsWidget::DispatchFocus(nsGUIEvent &aEvent)
{
  if (mEventCallback)
    return DispatchWindowEvent(&aEvent);

  return PR_FALSE;
}

//////////////////////////////////////////////////////////////////
//
// OnSomething handlers
//
//////////////////////////////////////////////////////////////////

#ifdef NS_DEBUG
PRInt32
nsWidget::debug_GetRenderXID(GtkObject * aGtkWidget)
{
  GdkWindow * renderWindow = GetRenderWindow(aGtkWidget);
  
  Window      xid = renderWindow ? GDK_WINDOW_XWINDOW(renderWindow) : 0x0;
  
  return (PRInt32) xid;
}

PRInt32
nsWidget::debug_GetRenderXID(GtkWidget * aGtkWidget)
{
  return debug_GetRenderXID(GTK_OBJECT(aGtkWidget));
}

nsCAutoString
nsWidget::debug_GetName(GtkObject * aGtkWidget)
{
  if (nsnull != aGtkWidget && GTK_IS_WIDGET(aGtkWidget))
    return debug_GetName(GTK_WIDGET(aGtkWidget));
  
  return nsCAutoString("null");
}

nsCAutoString
nsWidget::debug_GetName(GtkWidget * aGtkWidget)
{

  if (nsnull != aGtkWidget)
    return nsCAutoString(gtk_widget_get_name(aGtkWidget));
  
  return nsCAutoString("null");
}

#endif // NS_DEBUG


//////////////////////////////////////////////////////////////////

//-------------------------------------------------------------------------
//
// Invokes callback and  ProcessEvent method on Event Listener object
//
//-------------------------------------------------------------------------

NS_IMETHODIMP nsWidget::DispatchEvent(nsGUIEvent *aEvent,
                                      nsEventStatus &aStatus)
{
  NS_ADDREF(aEvent->widget);

#ifdef NS_DEBUG
  GtkObject *gw;
  void *nativeWidget = aEvent->widget->GetNativeData(NS_NATIVE_WIDGET);
  if (nativeWidget) {
    gw = GTK_OBJECT(nativeWidget);
    
    if (CAPS_LOCK_IS_ON)
      {
        debug_DumpEvent(stdout,
                        aEvent->widget,
                        aEvent,
                        debug_GetName(gw),
                        (PRInt32) debug_GetRenderXID(gw));
      }
  }
#endif // NS_DEBUG

  if (nsnull != mMenuListener) {
    if (NS_MENU_EVENT == aEvent->eventStructType)
      aStatus = mMenuListener->MenuSelected(NS_STATIC_CAST(nsMenuEvent&, *aEvent));
  }

  aStatus = nsEventStatus_eIgnore;
  if (nsnull != mEventCallback) {
    aStatus = (*mEventCallback)(aEvent);
  }

  // Dispatch to event listener if event was not consumed
  if ((aStatus != nsEventStatus_eIgnore) && (nsnull != mEventListener)) {
    aStatus = mEventListener->ProcessEvent(*aEvent);
  }
  NS_RELEASE(aEvent->widget);

  return NS_OK;
}


//-------------------------------------------------------------------------
//
// Deal with all sort of mouse event
//
//-------------------------------------------------------------------------
PRBool nsWidget::DispatchMouseEvent(nsMouseEvent& aEvent)
{
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
//         result = ConvertStatus(mMouseListener->MouseMoved(aEvent));
//         nsRect rect;
//         GetBounds(rect);
//         if (rect.Contains(event.point.x, event.point.y)) {
//           if (mCurrentWindow == NULL || mCurrentWindow != this) {
//             printf("Mouse enter");
//             mCurrentWindow = this;
//           }
//         } else {
//           printf("Mouse exit");
//         }

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

    case NS_DRAGDROP_DROP:
      printf("nsWidget::DispatchMouseEvent, NS_DRAGDROP_DROP\n");
      break;

    default:
      break;

    } // switch
  }
  return result;
}

//////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////
//
// GTK signal installers
//
//////////////////////////////////////////////////////////////////
void
nsWidget::AddToEventMask(GtkWidget * aWidget,
                         gint        aEventMask)
{
  NS_ASSERTION( nsnull != aWidget, "widget is null");
  NS_ASSERTION( 0 != aEventMask, "mask is 0");

  gtk_widget_add_events(aWidget,aEventMask);
}
//////////////////////////////////////////////////////////////////
void 
nsWidget::InstallMotionNotifySignal(GtkWidget * aWidget)
{
  NS_ASSERTION( nsnull != aWidget, "widget is null");

  InstallSignal((GtkWidget *)aWidget,
				(gchar *)"motion_notify_event",
				GTK_SIGNAL_FUNC(nsWidget::MotionNotifySignal));
}

void 
nsWidget::InstallDragLeaveSignal(GtkWidget * aWidget)
{
  NS_ASSERTION( nsnull != aWidget, "widget is null");

  InstallSignal(aWidget,
                (gchar *)"drag_leave",
                GTK_SIGNAL_FUNC(nsWidget::DragLeaveSignal));
}

void 
nsWidget::InstallDragMotionSignal(GtkWidget * aWidget)
{
  NS_ASSERTION( nsnull != aWidget, "widget is null");

  InstallSignal(aWidget,
                (gchar *)"drag_motion",
                GTK_SIGNAL_FUNC(nsWidget::DragMotionSignal));
}

void 
nsWidget::InstallDragBeginSignal(GtkWidget * aWidget)
{
  NS_ASSERTION( nsnull != aWidget, "widget is null");

  InstallSignal(aWidget,
                (gchar *)"drag_begin",
                GTK_SIGNAL_FUNC(nsWidget::DragBeginSignal));
}

void 
nsWidget::InstallDragDropSignal(GtkWidget * aWidget)
{
  NS_ASSERTION( nsnull != aWidget, "widget is null");

  InstallSignal(aWidget,
                (gchar *)"drag_drop",
                GTK_SIGNAL_FUNC(nsWidget::DragDropSignal));
}

void
nsWidget::InstallDragDataReceived(GtkWidget *aWidget)
{
  NS_ASSERTION( nsnull != aWidget, "widget is null");
  InstallSignal(aWidget,
                (gchar *)"drag_data_received",
                GTK_SIGNAL_FUNC(nsWidget::DragDataReceivedSignal));
}


//////////////////////////////////////////////////////////////////
void 
nsWidget::InstallEnterNotifySignal(GtkWidget * aWidget)
{
  NS_ASSERTION( nsnull != aWidget, "widget is null");

  InstallSignal(aWidget,
				(gchar *)"enter_notify_event",
				GTK_SIGNAL_FUNC(nsWidget::EnterNotifySignal));
}
//////////////////////////////////////////////////////////////////
void 
nsWidget::InstallLeaveNotifySignal(GtkWidget * aWidget)
{
  NS_ASSERTION( nsnull != aWidget, "widget is null");

  InstallSignal(aWidget,
				(gchar *)"leave_notify_event",
				GTK_SIGNAL_FUNC(nsWidget::LeaveNotifySignal));
}
//////////////////////////////////////////////////////////////////
void 
nsWidget::InstallButtonPressSignal(GtkWidget * aWidget)
{
  NS_ASSERTION( nsnull != aWidget, "widget is null");

  InstallSignal(aWidget,
				(gchar *)"button_press_event",
				GTK_SIGNAL_FUNC(nsWidget::ButtonPressSignal));
}
//////////////////////////////////////////////////////////////////
void 
nsWidget::InstallButtonReleaseSignal(GtkWidget * aWidget)
{
  NS_ASSERTION( nsnull != aWidget, "widget is null");

  InstallSignal(aWidget,
				(gchar *)"button_release_event",
				GTK_SIGNAL_FUNC(nsWidget::ButtonReleaseSignal));
}
//////////////////////////////////////////////////////////////////
void 
nsWidget::InstallFocusInSignal(GtkWidget * aWidget)
{
  NS_ASSERTION( nsnull != aWidget, "widget is null");

  InstallSignal(aWidget,
				(gchar *)"focus_in_event",
				GTK_SIGNAL_FUNC(nsWidget::FocusInSignal));
}
//////////////////////////////////////////////////////////////////
void 
nsWidget::InstallFocusOutSignal(GtkWidget * aWidget)
{
  NS_ASSERTION( nsnull != aWidget, "widget is null");

  InstallSignal(aWidget,
				(gchar *)"focus_out_event",
				GTK_SIGNAL_FUNC(nsWidget::FocusOutSignal));
}
//////////////////////////////////////////////////////////////////
void 
nsWidget::InstallRealizeSignal(GtkWidget * aWidget)
{
  NS_ASSERTION( nsnull != aWidget, "widget is null");
  
  InstallSignal(aWidget,
				(gchar *)"realize",
				GTK_SIGNAL_FUNC(nsWidget::RealizeSignal));
}
//////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////
/* virtual */ void 
nsWidget::OnMotionNotifySignal(GdkEventMotion * aGdkMotionEvent)
{
  
  if (mIsDestroying)
    return;

  nsMouseEvent event;

  event.message = NS_MOUSE_MOVE;
  event.eventStructType = NS_MOUSE_EVENT;

  // If there is a button motion target, use that instead of the
  // current widget

  // XXX pav
  // i'm confused as to wtf this sButtonMoetionTarget thing is for.
  // so i'm not going to use it.

  // XXX ramiro
  // 
  // Because of dynamic widget creation and destruction, this could
  // potentially be a dangerious thing to do.  
  //
  // If the sButtonMotionTarget is destroyed between the time when
  // it got set and now, we should end up sending an event to 
  // a junk nsWidget.
  //
  // The way to solve this would be to add a destroy signal to
  // the GtkWidget corresponding to the sButtonMotionTarget and
  // marking if nsnull in there.
  //
  gint x, y;

  if (aGdkMotionEvent)
  {
    x = (gint) aGdkMotionEvent->x;
    y = (gint) aGdkMotionEvent->y;

    event.point.x = nscoord(x);
    event.point.y = nscoord(y);
    event.widget = this;
  }

  if (sButtonMotionTarget)
  {
    gint diffX;
    gint diffY;

    if (aGdkMotionEvent) 
    {
      // Compute the difference between the original root coordinates
      diffX = (gint) aGdkMotionEvent->x_root - sButtonMotionRootX;
      diffY = (gint) aGdkMotionEvent->y_root - sButtonMotionRootY;
      
      event.widget = sButtonMotionTarget;
      
      // The event coords will be the initial *widget* coords plus the 
      // root difference computed above.
      event.point.x = nscoord(sButtonMotionWidgetX + diffX);
      event.point.y = nscoord(sButtonMotionWidgetY + diffY);
    }
  }
  else
  {
    event.widget = this;

    if (aGdkMotionEvent)
    {
      event.point.x = nscoord(x);
      event.point.y = nscoord(y);
    }
  }

  if (aGdkMotionEvent)
  {
    event.time = aGdkMotionEvent->time;
    event.isShift = aGdkMotionEvent->state & ShiftMask;
    event.isControl = aGdkMotionEvent->state & ControlMask;
    event.isAlt = aGdkMotionEvent->state & Mod1Mask;
  }

  AddRef();

  if (sButtonMotionTarget)
    sButtonMotionTarget->DispatchMouseEvent(event);
  else
    DispatchMouseEvent(event);

  Release();
}

/* virtual */ void 
nsWidget::OnDragMotionSignal(GdkDragContext *aGdkDragContext,
                             gint            x,
                             gint            y,
                             guint           aTime)
{
  if (!mIsDragDest)
  {
    // this will happen on the first motion event, so we will generate an ENTER event
    OnDragEnterSignal(aGdkDragContext, x, y, aTime);
  }


  GtkWidget *source_widget;
  source_widget = gtk_drag_get_source_widget (aGdkDragContext);
  g_print("motion, source %s\n", source_widget ?
	    gtk_type_name (GTK_OBJECT (source_widget)->klass->type) :
	    "unknown");

  gdk_drag_status (aGdkDragContext, aGdkDragContext->suggested_action, aTime);

  nsMouseEvent event;

  event.message = NS_DRAGDROP_OVER;
  event.eventStructType = NS_DRAGDROP_EVENT;

  event.widget = this;

  event.point.x = x;
  event.point.y = y;

  AddRef();

  DispatchMouseEvent(event);

  Release();
}

/* not a real signal.. called from OnDragMotionSignal */
/* virtual */ void 
nsWidget::OnDragEnterSignal(GdkDragContext *aGdkDragContext,
                             gint            x,
                             gint            y,
                             guint           aTime)
{

  // make sure that we tell the drag manager what the hell is going on.
  UpdateDragContext(NULL, aGdkDragContext, aTime);

  // we are a drag dest.. cool huh?
  mIsDragDest = PR_TRUE;

  nsMouseEvent event;

  event.message = NS_DRAGDROP_ENTER;
  event.eventStructType = NS_DRAGDROP_EVENT;

  event.widget = this;

  event.point.x = x;
  event.point.y = y;

  AddRef();

  DispatchMouseEvent(event);

  Release();
}

/* virtual */ void 
nsWidget::OnDragLeaveSignal(GdkDragContext   *context,
                            guint             aTime)
{
  // update our drag context
  UpdateDragContext(NULL, NULL, aTime);

  mIsDragDest = PR_FALSE;

  nsMouseEvent event;

  event.message = NS_DRAGDROP_EXIT;
  event.eventStructType = NS_DRAGDROP_EVENT;

  event.widget = this;

  //  GdkEvent *current_event;
  //  current_event = gtk_get_current_event();

  //  g_print("current event's x_root = %i , y_root = %i\n", current_event->dnd.x_root, current_event->dnd.y_root);

  // FIXME
  event.point.x = 0;
  event.point.y = 0;

  AddRef();

  DispatchMouseEvent(event);

  Release();
}

/* virtual */ void 
nsWidget::OnDragBeginSignal(GdkDragContext * aGdkDragContext)
{
  nsMouseEvent event;

  event.message = NS_MOUSE_MOVE;
  event.eventStructType = NS_MOUSE_EVENT;
  
  AddRef();

  DispatchMouseEvent(event);

  Release();
}


/* virtual */ void 
nsWidget::OnDragDropSignal(GtkWidget      *aWidget,
                           GdkDragContext *aDragContext,
                           gint            x,
                           gint            y,
                           guint           aTime)
{
  UpdateDragContext(aWidget, aDragContext, aTime);

  nsMouseEvent    event;

  event.message = NS_DRAGDROP_DROP;
  event.eventStructType = NS_DRAGDROP_EVENT;
  event.widget = this;
  
  event.point.x = x;
  event.point.y = y;

  AddRef();

  DispatchWindowEvent(&event);

  Release();

  // after a drop takes place we need to make sure that the drag
  // service doesn't think that it still has a context.  if the other
  // way ( besides the drop ) to end a drag event is during the leave
  // event and and that case is handled in that handler.
  UpdateDragContext(NULL, NULL, 0);

}

/* virtual */
void nsWidget::OnDragDataReceivedSignal(GtkWidget         *aWidget,
                                        GdkDragContext    *context,
                                        gint               x,
                                        gint               y,
                                        GtkSelectionData  *selection_data,
                                        guint              info,
                                        guint32            aTime)
{
  // the service is the only one that cares that we get this.
  g_print("nsWidget::OnDragDataReceivedSignal\n");
  nsCOMPtr<nsIDragService> dragService;
  nsresult rv = nsServiceManager::GetService(kCDragServiceCID,
                                             nsIDragService::GetIID(),
                                             (nsISupports **)&dragService);
  if (NS_FAILED(rv)) {
    g_print("*** warning: failed to get the drag service. this is a _bad_ thing.\n");
    return;
  }
  nsCOMPtr<nsIDragSessionGTK> dragServiceGTK;
  dragServiceGTK = do_QueryInterface(dragService);
  if (!dragServiceGTK) {
    return;
  }
  dragServiceGTK->SetDataReceived(aWidget, context, x, y, selection_data, info, aTime);
}



//////////////////////////////////////////////////////////////////
/* virtual */ void
nsWidget::OnEnterNotifySignal(GdkEventCrossing * aGdkCrossingEvent)
{
  // If there is a button motion target, then we can ignore this
  // event since what the gecko event system expects is for
  // only motion events to be sent to that widget, even if the
  // pointer is crossing on other widgets.
  //
  // XXX ramiro - Same as above.
  //
  if (sButtonMotionTarget)
  {
    return;
  }

  nsMouseEvent event;

  event.message = NS_MOUSE_ENTER;
  event.widget  = this;
  event.eventStructType = NS_MOUSE_EVENT;

  if (aGdkCrossingEvent != NULL) 
  {
    event.point.x = nscoord(aGdkCrossingEvent->x);
    event.point.y = nscoord(aGdkCrossingEvent->y);
    event.time = aGdkCrossingEvent->time;
  }

  AddRef();

  DispatchMouseEvent(event);

  Release();
}
//////////////////////////////////////////////////////////////////////
/* virtual */ void
nsWidget::OnLeaveNotifySignal(GdkEventCrossing * aGdkCrossingEvent)
{
  // If there is a button motion target, then we can ignore this
  // event since what the gecko event system expects is for
  // only motion events to be sent to that widget, even if the
  // pointer is crossing on other widgets.
  //
  // XXX ramiro - Same as above.
  //
  if (nsnull != sButtonMotionTarget)
  {
    return;
  }

  nsMouseEvent event;

  event.message = NS_MOUSE_EXIT;
  event.widget  = this;
  event.eventStructType = NS_MOUSE_EVENT;

  if (aGdkCrossingEvent != NULL) 
  {
    event.point.x = nscoord(aGdkCrossingEvent->x);
    event.point.y = nscoord(aGdkCrossingEvent->y);
    event.time = aGdkCrossingEvent->time;
  }

  AddRef();

  DispatchMouseEvent(event);

  Release();
}


//
// IsMouseInWindow
//
// Check if the mouse x/y is within the given window
//
PRBool
nsWidget :: IsMouseInWindow ( GdkWindow* inWindow, PRInt32 inMouseX, PRInt32 inMouseY )
{
  gint x, y;
  gint w, h;

  // XXX this causes a round trip to the ever lovely X server.  fix me.
  gdk_window_get_origin(inWindow, &x, &y);

  // this doesn't... it just pokes GdkWindowPrivate :-)
  gdk_window_get_size(inWindow, &w, &h);

  if ( inMouseX > x && inMouseX < x + w &&
       inMouseY > y && inMouseY < y + h )
    return PR_TRUE;

  return PR_FALSE;

} // IsMouseInWindow


//
// HandlePopup
//
// Deal with rollup of popups (xpmenus, etc)
// 
PRBool
nsWidget :: HandlePopup ( PRInt32 inMouseX, PRInt32 inMouseY )
{
  PRBool retVal = PR_FALSE;
  nsCOMPtr<nsIWidget> rollupWidget = do_QueryReferent(gRollupWidget);
  if (rollupWidget && gRollupListener)
  {
    GdkWindow *currentPopup = (GdkWindow *)rollupWidget->GetNativeData(NS_NATIVE_WINDOW);
    if ( !IsMouseInWindow(currentPopup, inMouseX, inMouseY) ) {
      PRBool rollup = PR_TRUE;
      // if we're dealing with menus, we probably have submenus and we don't
      // want to rollup if the clickis in a parent menu of the current submenu
      nsCOMPtr<nsIMenuRollup> menuRollup ( do_QueryInterface(gRollupListener) );
      if ( menuRollup ) {
        nsCOMPtr<nsISupportsArray> widgetChain;
        menuRollup->GetSubmenuWidgetChain ( getter_AddRefs(widgetChain) );
        if ( widgetChain ) {
          PRUint32 count = 0;
          widgetChain->Count ( &count );
          for ( PRUint32 i = 0; i < count; ++i ) {
            nsCOMPtr<nsISupports> genericWidget;
            widgetChain->GetElementAt ( i, getter_AddRefs(genericWidget) );
            nsCOMPtr<nsIWidget> widget ( do_QueryInterface(genericWidget) );
            if ( widget ) {
              GdkWindow* currWindow = (GdkWindow*) widget->GetNativeData(NS_NATIVE_WINDOW);
              if ( IsMouseInWindow(currWindow, inMouseX, inMouseY) ) {
                rollup = PR_FALSE;
                break;
              }
            }
          } // foreach parent menu widget
        }
      } // if rollup listener knows about menus

      // if we've determined that we should still rollup, do it.
      if ( rollup ) {
        gRollupListener->Rollup();
        retVal = PR_TRUE;
      }
    }
  } else {
    gRollupWidget = nsnull;
    gRollupListener = nsnull;
  }

  return retVal;
} // HandlePopup


//////////////////////////////////////////////////////////////////////
/* virtual */ void
nsWidget::OnButtonPressSignal(GdkEventButton * aGdkButtonEvent)
{
  nsMouseEvent event;
  nsMouseScrollEvent scrollEvent;
  PRUint32 eventType = 0;

  if ( HandlePopup(aGdkButtonEvent->x_root, aGdkButtonEvent->y_root) )
    return;
   
  // Switch on single, double, triple click.
  switch (aGdkButtonEvent->type) 
  {
    // Single click.
  case GDK_BUTTON_PRESS:
    // Double click.
  case GDK_2BUTTON_PRESS:
    // Triple click.
  case GDK_3BUTTON_PRESS:

    switch (aGdkButtonEvent->button)  // Which button?
    {
    case 1:
      eventType = NS_MOUSE_LEFT_BUTTON_DOWN;
      break;

    case 2:
      eventType = NS_MOUSE_MIDDLE_BUTTON_DOWN;
      break;

    case 3:
      eventType = NS_MOUSE_RIGHT_BUTTON_DOWN;
      break;

    case 4:
    case 5:
      if (aGdkButtonEvent->button == 4)
        scrollEvent.deltaLines = -3;
      else
        scrollEvent.deltaLines = 3;

      scrollEvent.message = NS_MOUSE_SCROLL;
      scrollEvent.widget = this;
      scrollEvent.eventStructType = NS_MOUSE_SCROLL_EVENT;

      scrollEvent.point.x = nscoord(aGdkButtonEvent->x);
      scrollEvent.point.y = nscoord(aGdkButtonEvent->y);
      
      scrollEvent.isShift = (aGdkButtonEvent->state & GDK_SHIFT_MASK) ? PR_TRUE : PR_FALSE;
      scrollEvent.isControl = (aGdkButtonEvent->state & GDK_CONTROL_MASK) ? PR_TRUE : PR_FALSE;
      scrollEvent.isAlt = (aGdkButtonEvent->state & GDK_MOD1_MASK) ? PR_TRUE : PR_FALSE;
      scrollEvent.isMeta = PR_FALSE;  // GTK+ doesn't support the meta key
      scrollEvent.time = aGdkButtonEvent->time;
      AddRef();
      if (mEventCallback)
        DispatchWindowEvent(&scrollEvent);
      Release();
      return;

      // Single-click default.
    default:
      eventType = NS_MOUSE_LEFT_BUTTON_DOWN;
      break;
    }
    break;

  default:
    break;
  }

  InitMouseEvent(aGdkButtonEvent, event, eventType);

  // Set the button motion target and remeber the widget and root coords
  sButtonMotionTarget = this;

  sButtonMotionRootX = (gint) aGdkButtonEvent->x_root;
  sButtonMotionRootY = (gint) aGdkButtonEvent->y_root;

  sButtonMotionWidgetX = (gint) aGdkButtonEvent->x;
  sButtonMotionWidgetY = (gint) aGdkButtonEvent->y;
  
  AddRef();

  DispatchMouseEvent(event);

  Release();

}
//////////////////////////////////////////////////////////////////////
/* virtual */ void
nsWidget::OnButtonReleaseSignal(GdkEventButton * aGdkButtonEvent)
{
  nsMouseEvent event;
  PRUint32 eventType = 0;

  switch (aGdkButtonEvent->button)
  {
  case 1:
    eventType = NS_MOUSE_LEFT_BUTTON_UP;
    break;
	  
  case 2:
    eventType = NS_MOUSE_MIDDLE_BUTTON_UP;
    break;
	  
  case 3:
    eventType = NS_MOUSE_RIGHT_BUTTON_UP;
    break;

  case 4:
  case 5:
    // We don't really need to do anything here, but we don't want
    // LEFT_BUTTON_UP to happen
    return;

  default:
    eventType = NS_MOUSE_LEFT_BUTTON_UP;
    break;
	}


  InitMouseEvent(aGdkButtonEvent, event, eventType);

  if (sButtonMotionTarget) {
    gint diffX = 0;
    gint diffY = 0;

    diffX = (gint) aGdkButtonEvent->x_root - sButtonMotionRootX;
    diffY = (gint) aGdkButtonEvent->y_root - sButtonMotionRootY;
    
    event.widget = sButtonMotionTarget;

    // see comments in nsWidget::OnMotionNotifySignal
    event.point.x = nscoord(sButtonMotionWidgetX + diffX);
    event.point.y = nscoord(sButtonMotionWidgetY + diffY);
  }

  // event.widget can get set to null when calling DispatchMouseEvent,
  // so to release it we must make a copy
  nsWidget* theWidget = NS_STATIC_CAST(nsWidget*,event.widget);

  NS_ADDREF(theWidget);
  theWidget->DispatchMouseEvent(event);
  NS_IF_RELEASE(theWidget);


  if (sButtonMotionTarget)
  {
    sButtonMotionTarget = nsnull;

    sButtonMotionRootX = -1;
    sButtonMotionRootY = -1;
  }
}
//////////////////////////////////////////////////////////////////////
/* virtual */ void
nsWidget::OnFocusInSignal(GdkEventFocus * aGdkFocusEvent)
{
  if (mIsDestroying)
    return;

  GTK_WIDGET_SET_FLAGS(mWidget, GTK_HAS_FOCUS);

  nsGUIEvent event;
  
  event.message = NS_GOTFOCUS;
  event.widget  = this;

  event.eventStructType = NS_GUI_EVENT;

//  event.time = aGdkFocusEvent->time;;
//  event.time = PR_Now();
  event.time = 0;
  event.point.x = 0;
  event.point.y = 0;

  AddRef();
  
  DispatchFocus(event);
  
  Release();
}
//////////////////////////////////////////////////////////////////////
/* virtual */ void
nsWidget::OnFocusOutSignal(GdkEventFocus * aGdkFocusEvent)
{
  if (mIsDestroying)
    return;

  GTK_WIDGET_UNSET_FLAGS(mWidget, GTK_HAS_FOCUS);

  nsGUIEvent event;
  
  event.message = NS_LOSTFOCUS;
  event.widget  = this;

  event.eventStructType = NS_GUI_EVENT;

//  event.time = aGdkFocusEvent->time;;
//  event.time = PR_Now();
  event.time = 0;
  event.point.x = 0;
  event.point.y = 0;

  AddRef();
  
  DispatchFocus(event);
  
  Release();
}
//////////////////////////////////////////////////////////////////////
/* virtual */ void
nsWidget::OnRealize(GtkWidget *aWidget)
{
  printf("nsWidget::OnRealize(%p)\n",this);
}
//////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////
//
// GTK event support methods
//
//////////////////////////////////////////////////////////////////
void 
nsWidget::InstallSignal(GtkWidget *   aWidget,
                        gchar *       aSignalName,
                        GtkSignalFunc aSignalFunction)
{
  NS_ASSERTION( nsnull != aWidget, "widget is null");
  NS_ASSERTION( aSignalName, "signal name is null");
  NS_ASSERTION( aSignalFunction, "signal function is null");

  gtk_signal_connect(GTK_OBJECT(aWidget),
                     aSignalName,
                     GTK_SIGNAL_FUNC(aSignalFunction),
                     (gpointer) this);
}
//////////////////////////////////////////////////////////////////
void 
nsWidget::InitMouseEvent(GdkEventButton * aGdkButtonEvent,
						 nsMouseEvent &anEvent,
						 PRUint32   aEventType)
{
  anEvent.message = aEventType;
  anEvent.widget  = this;

  anEvent.eventStructType = NS_MOUSE_EVENT;

  if (aGdkButtonEvent != NULL) {
    anEvent.point.x = nscoord(aGdkButtonEvent->x);
    anEvent.point.y = nscoord(aGdkButtonEvent->y);

    anEvent.isShift = (aGdkButtonEvent->state & GDK_SHIFT_MASK) ? PR_TRUE : PR_FALSE;
    anEvent.isControl = (aGdkButtonEvent->state & GDK_CONTROL_MASK) ? PR_TRUE : PR_FALSE;
    anEvent.isAlt = (aGdkButtonEvent->state & GDK_MOD1_MASK) ? PR_TRUE : PR_FALSE;
    anEvent.time = aGdkButtonEvent->time;

    switch(aGdkButtonEvent->type)
      {
      case GDK_BUTTON_PRESS:
        anEvent.clickCount = 1;
        break;
      case GDK_2BUTTON_PRESS:
        anEvent.clickCount = 2;
        break;
      case GDK_3BUTTON_PRESS:
        anEvent.clickCount = 3;
        break;
      default:
        anEvent.clickCount = 1;
    }

  }
}
//////////////////////////////////////////////////////////////////
PRBool
nsWidget::DropEvent(GtkWidget * aWidget, 
                    GdkWindow * aEventWindow) 
{
  NS_ASSERTION( nsnull != aWidget, "widget is null");
  NS_ASSERTION( nsnull != aEventWindow, "event window is null");

#if 0
  static int count = 0;

  if (GTK_IS_LAYOUT(aWidget))
  {
    GtkLayout * layout = GTK_LAYOUT(aWidget);

    printf("%4d DropEvent(this=%p,widget=%p,event_win=%p,wid_win=%p,bin_win=%p)\n",
           count++,
           this,
           aWidget,
           aEventWindow,
           aWidget->window,
           layout->bin_window);
  }
  else
  {
    printf("%4d DropEvent(this=%p,widget=%p,event_win=%p,wid_win=%p)\n",
           count++,
           this,
           aWidget,
           aEventWindow,
           aWidget->window);
  }
#endif


  // For gtklayout widgets, we dont want to handle events
  // that occur in the sub windows.  Check the window member
  // of the GdkEvent, if it is not the gtklayout's bin_window,
  // drop the event.
  if (GTK_IS_LAYOUT(aWidget))
  {
    GtkLayout * layout = GTK_LAYOUT(aWidget);

    if (aEventWindow != layout->bin_window)
    {
#ifdef DEBUG_pavlov
      printf("dropping event!!!!!!!\n");
#endif
      return PR_TRUE;
    }
  }
  return PR_FALSE;
}
//////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////
//
// GTK widget signals
//
//////////////////////////////////////////////////////////////////
/* static */ gint
nsWidget::MotionNotifySignal(GtkWidget *      aWidget,
							 GdkEventMotion * aGdkMotionEvent,
							 gpointer         aData)
{
  NS_ASSERTION( nsnull != aWidget, "widget is null");
  NS_ASSERTION( nsnull != aGdkMotionEvent, "event is null");

  nsWidget * widget = (nsWidget *) aData;

  NS_ASSERTION( nsnull != widget, "instance pointer is null");

  if (widget->DropEvent(aWidget, aGdkMotionEvent->window))
  {
    return PR_TRUE;
  }

  widget->OnMotionNotifySignal(aGdkMotionEvent);

  return PR_TRUE;
}

/* static */ gint
nsWidget::DragMotionSignal(GtkWidget *      aWidget,
                           GdkDragContext   *aDragContext,
                           gint             x,
                           gint             y,
                           guint            aTime,
                           void             *aData)
{
  nsWidget * widget = (nsWidget *) aData;
  NS_ASSERTION( nsnull != widget, "instance pointer is null");

  widget->OnDragMotionSignal(aDragContext, x, y, aTime);

  return PR_TRUE;
}

/* static */ void
nsWidget::DragLeaveSignal(GtkWidget *      aWidget,
                          GdkDragContext   *aDragContext,
                          guint            aTime,
                          void             *aData)
{
  nsWidget * widget = (nsWidget *) aData;
  NS_ASSERTION( nsnull != widget, "instance pointer is null");

  widget->OnDragLeaveSignal(aDragContext, aTime);
}

/* static */ gint
nsWidget::DragBeginSignal(GtkWidget *      aWidget,
                          GdkDragContext   *aDragContext,
                          gint             x,
                          gint             y,
                          guint            aTime,
                          void             *aData)
{
  printf("nsWidget::DragBeginSignal\n");
  fflush(stdout);

  return PR_TRUE;
}

/* static */ gint
nsWidget::DragDropSignal(GtkWidget *      aWidget,
                         GdkDragContext   *aDragContext,
                         gint             x,
                         gint             y,
                         guint            aTime,
                         void             *aData)
{
  NS_ASSERTION( nsnull != aWidget, "widget is null");
  NS_ASSERTION( nsnull != aDragContext, "dragcontext is null");

  nsWidget * widget = (nsWidget *) aData;
  NS_ASSERTION( nsnull != widget, "instance pointer is null");

#if 0
  if (widget->DropEvent(aWidget, aDragContext->source_window)) {
    return PR_TRUE;
  }
#endif

  widget->OnDragDropSignal(aWidget, aDragContext, x, y, aTime);

  return PR_TRUE;
}


/* static */
void
nsWidget::DragDataReceivedSignal(GtkWidget         *aWidget,
                                 GdkDragContext    *aDragContext,
                                 gint               x,
                                 gint               y,
                                 GtkSelectionData  *aSelectionData,
                                 guint              aInfo,
                                 guint32            aTime,
                                 gpointer           aData)
{
  NS_ASSERTION(nsnull != aWidget, "widget is null");
  NS_ASSERTION(nsnull != aDragContext, "dragcontext is null");
  nsWidget *widget = (nsWidget *)aData;
  NS_ASSERTION( nsnull != widget, "instance pointer is null");

  widget->OnDragDataReceivedSignal(aWidget, aDragContext, x, y, aSelectionData, aInfo, aTime);
}


//////////////////////////////////////////////////////////////////
/* static */ gint 
nsWidget::EnterNotifySignal(GtkWidget *        aWidget, 
							GdkEventCrossing * aGdkCrossingEvent, 
							gpointer           aData)
{
  NS_ASSERTION( nsnull != aWidget, "widget is null");
  NS_ASSERTION( nsnull != aGdkCrossingEvent, "event is null");

  nsWidget * widget = (nsWidget *) aData;

  NS_ASSERTION( nsnull != widget, "instance pointer is null");

  if (widget->DropEvent(aWidget, aGdkCrossingEvent->window))
  {
	return PR_TRUE;
  }

  widget->OnEnterNotifySignal(aGdkCrossingEvent);

  return PR_TRUE;
}
//////////////////////////////////////////////////////////////////////
/* static */ gint 
nsWidget::LeaveNotifySignal(GtkWidget *        aWidget, 
							GdkEventCrossing * aGdkCrossingEvent, 
							gpointer           aData)
{
  NS_ASSERTION( nsnull != aWidget, "widget is null");
  NS_ASSERTION( nsnull != aGdkCrossingEvent, "event is null");

  nsWidget * widget = (nsWidget *) aData;

  NS_ASSERTION( nsnull != widget, "instance pointer is null");

  if (widget->DropEvent(aWidget, aGdkCrossingEvent->window))
  {
    return PR_TRUE;
  }

  widget->OnLeaveNotifySignal(aGdkCrossingEvent);

  return PR_TRUE;
}
//////////////////////////////////////////////////////////////////////
/* static */ gint 
nsWidget::ButtonPressSignal(GtkWidget *      aWidget, 
							GdkEventButton * aGdkButtonEvent, 
							gpointer         aData)
{
  //  printf("nsWidget::ButtonPressSignal(%p)\n",aData);

  NS_ASSERTION( nsnull != aWidget, "widget is null");
  NS_ASSERTION( nsnull != aGdkButtonEvent, "event is null");

  nsWidget * widget = (nsWidget *) aData;

  NS_ASSERTION( nsnull != widget, "instance pointer is null");

  if (widget->DropEvent(aWidget, aGdkButtonEvent->window))
  {
    return PR_TRUE;
  }

  widget->OnButtonPressSignal(aGdkButtonEvent);

  return PR_TRUE;
}
//////////////////////////////////////////////////////////////////////
/* static */ gint 
nsWidget::ButtonReleaseSignal(GtkWidget *      aWidget, 
							GdkEventButton * aGdkButtonEvent, 
							gpointer         aData)
{
  //  printf("nsWidget::ButtonReleaseSignal(%p)\n",aData);

  NS_ASSERTION( nsnull != aWidget, "widget is null");
  NS_ASSERTION( nsnull != aGdkButtonEvent, "event is null");

  nsWidget * widget = (nsWidget *) aData;

  NS_ASSERTION( nsnull != widget, "instance pointer is null");

  if (widget->DropEvent(aWidget, aGdkButtonEvent->window))
  {
    return PR_TRUE;
  }

  widget->OnButtonReleaseSignal(aGdkButtonEvent);

  return PR_TRUE;
}
//////////////////////////////////////////////////////////////////////
/* static */ gint 
nsWidget::RealizeSignal(GtkWidget *      aWidget,
                        gpointer         aData)
{
  NS_ASSERTION( nsnull != aWidget, "widget is null");
  
  nsWidget * widget = (nsWidget *) aData;

  NS_ASSERTION( nsnull != widget, "instance pointer is null");
  
  widget->OnRealize(aWidget);

  return PR_TRUE;
}
//////////////////////////////////////////////////////////////////////
/* static */ gint
nsWidget::FocusInSignal(GtkWidget *      aWidget, 
                        GdkEventFocus *  aGdkFocusEvent, 
                        gpointer         aData)
{
  //  printf("nsWidget::ButtonReleaseSignal(%p)\n",aData);

  NS_ASSERTION( nsnull != aWidget, "widget is null");
  NS_ASSERTION( nsnull != aGdkFocusEvent, "event is null");

  nsWidget * widget = (nsWidget *) aData;

  NS_ASSERTION( nsnull != widget, "instance pointer is null");

//   if (widget->DropEvent(aWidget, aGdkFocusEvent->window))
//   {
// 	return PR_TRUE;
//   }

  widget->OnFocusInSignal(aGdkFocusEvent);

  if (GTK_IS_WINDOW(aWidget))
    gtk_signal_emit_stop_by_name(GTK_OBJECT(aWidget), "focus_in_event");
  
  return PR_TRUE;
}
//////////////////////////////////////////////////////////////////////
/* static */ gint
nsWidget::FocusOutSignal(GtkWidget *      aWidget, 
                        GdkEventFocus *  aGdkFocusEvent, 
                        gpointer         aData)
{
  //  printf("nsWidget::ButtonReleaseSignal(%p)\n",aData);

  NS_ASSERTION( nsnull != aWidget, "widget is null");
  NS_ASSERTION( nsnull != aGdkFocusEvent, "event is null");

  nsWidget * widget = (nsWidget *) aData;

  NS_ASSERTION( nsnull != widget, "instance pointer is null");

//   if (widget->DropEvent(aWidget, aGdkFocusEvent->window))
//   {
// 	return PR_TRUE;
//   }

  widget->OnFocusOutSignal(aGdkFocusEvent);

  if (GTK_IS_WINDOW(aWidget))
    gtk_signal_emit_stop_by_name(GTK_OBJECT(aWidget), "focus_out_event");
  
  return PR_TRUE;
}
//////////////////////////////////////////////////////////////////////

/* virtual */ GdkWindow *
nsWidget::GetWindowForSetBackground()
{
  GdkWindow * gdk_window = nsnull;

  if (mWidget)
  {
    gdk_window = mWidget->window;
  }

  return gdk_window;
}

/* virtual */ GdkWindow *
nsWidget::GetRenderWindow(GtkObject * aGtkWidget)
{
  GdkWindow * renderWindow = nsnull;

  if (GDK_IS_SUPERWIN(aGtkWidget)) {
    renderWindow = GDK_SUPERWIN(aGtkWidget)->bin_window;
  }

  return renderWindow;
}


//////////////////////////////////////////////////////////////////////
// default setfont for most widgets
/*virtual*/
void nsWidget::SetFontNative(GdkFont *aFont)
{
  GtkStyle *style = gtk_style_copy(mWidget->style);
  // gtk_style_copy ups the ref count of the font
  gdk_font_unref (style->font);
  
  style->font = aFont;
  gdk_font_ref(style->font);
  
  gtk_widget_set_style(mWidget, style);
  
  gtk_style_unref(style);
}

//////////////////////////////////////////////////////////////////////
// default SetBackgroundColor for most widgets
/*virtual*/
void nsWidget::SetBackgroundColorNative(GdkColor *aColorNor,
                                        GdkColor *aColorBri,
                                        GdkColor *aColorDark)
{
  // use same style copy as SetFont
  GtkStyle *style = gtk_style_copy(mWidget->style);
  
  style->bg[GTK_STATE_NORMAL]=*aColorNor;
  
  // Mouse over button
  style->bg[GTK_STATE_PRELIGHT]=*aColorBri;

  // Button is down
  style->bg[GTK_STATE_ACTIVE]=*aColorDark;

  // other states too? (GTK_STATE_ACTIVE, GTK_STATE_PRELIGHT,
  //               GTK_STATE_SELECTED, GTK_STATE_INSENSITIVE)
  gtk_widget_set_style(mWidget, style);
  gtk_style_unref(style);
}



















//////////////////////////////////////////////////////////////////////

#ifdef USE_XIM
void
nsWidget::GetXIC()
{
  if (gInputStyle == nsnull) {
    gInputStyle = nsIMEGtkIC::GetInputStyle();
  }
  if (nsIMEGtkIC::GetInputPolicy() == NSIME_IC_PER_WIDGET) {
    mICPerShell = PR_FALSE;
  }
  if (gPreeditFontset == nsnull) {
    gPreeditFontset = gdk_fontset_load("-*-*-*-*-*-*-16-*-*-*-*-*-*-*");
    mXICFontSize = 16;          // default
  }
  if (gStatusFontset == nsnull) {
    gStatusFontset = gdk_fontset_load("-*-*-*-*-*-*-16-*-*-*-*-*-*-*");
  }
  if (!mIMEShellWidget) {
    mIMEShellWidget = GetShellWidget((GdkWindow*)
                                     GetNativeData(NS_NATIVE_WINDOW));
    NS_ASSERTION(mIMEShellWidget, "GetShellWidget() fails");
  }
  if (!gInputStyle || !gPreeditFontset || !gStatusFontset || !mIMEShellWidget) {
    return;
  }

  if (mICPerShell) {
    if (mIMEShellWidget->mXIC){
      mXIC = mIMEShellWidget->mXIC;
    } else {
      mXIC = nsIMEGtkIC::GetXIC(mIMEShellWidget, gPreeditFontset,
							gStatusFontset);
      if (mXIC) {
        // fix me! need to know how to get spot location
        mXIC->SetPreeditSpotLocation(0, 14);
        mIMEShellWidget->mXIC = mXIC;
      }
    }
  } else {
    mXIC = nsIMEGtkIC::GetXIC(this, gPreeditFontset, gStatusFontset);
    if (mXIC) {
      // fix me! need to know how to get spot location
      mXIC->SetPreeditSpotLocation(0, mXICFontSize);
    }
  }
  if (mXIC) {
    nsWidget* parent = (nsWidget*)mParent.get();
    if (parent) {
      parent->mIMEChildWidget = this;
    }
  }
  return;
}
#endif // USE_XIM 

NS_IMETHODIMP nsWidget::ResetInputState()
{
#ifdef USE_XIM
  if (mIMEEnable == PR_FALSE) return NS_OK;
  if (mXIC) {
    if (mIMEShellWidget->mIMEIsDeactivating == PR_TRUE) {
      return NS_OK;
    }
    PRInt32 uniCharSize = 
      mXIC->ResetIC(&(mIMECompositionUniString),
                    &(mIMECompositionUniStringSize));
    if (uniCharSize) {
      mIMECompositionUniString[uniCharSize] = 0;

      nsWidget *saved_focusWindow = 0;
      if (focusWindow) {
        saved_focusWindow = focusWindow;
        focusWindow = 0;
      }

      IMEComposeStart(nsnull);

      //-------------------------------------------------------
      // send Text Event
      //-------------------------------------------------------
      IMEComposeText(nsnull,
                     mIMECompositionUniString,
                     uniCharSize,
                     nsnull);
      IMEComposeEnd(nsnull);

      if (saved_focusWindow) {
        focusWindow = saved_focusWindow;
      }

      if (gInputStyle & GDK_IM_PREEDIT_POSITION) {
        UpdateICSpot();
      }
    }
  } else {
    GetXIC();
  }
#endif // USE_XIM 
  return NS_OK;
}
NS_IMETHODIMP nsWidget::PasswordFieldInit()
{
#ifdef USE_XIM
  mIMEEnable = PR_FALSE;
#endif // USE_XIM 
  return NS_OK;
}

#ifdef USE_XIM
void
nsWidget::GetXYFromPosition(unsigned long *aX,
                            unsigned long *aY)
{
  if (mIMEEnable == PR_FALSE) return;
  if (nsnull == mXIC) return;
  if (mXIC) {
    GdkFont *gfontset = mXIC->GetPreeditFont();
    if (gfontset) {
      // this is currently not working well
      // We change from += ascent to -= descent because we change the nsCaret
      // code to return the nsPoint from the top of the cursor to the bottom
      // of the cursor
      *aY -= gfontset->descent;
    }
  }
  return;
}

void
nsWidget::SetXICBaseFontSize(int height)
{
  if (mIMEEnable == PR_FALSE) return;
  if (nsnull == mXIC) return;
  if (mXIC) {
    if (height == mXICFontSize) return;
    if (gPreeditFontset) {
      gdk_font_unref(gPreeditFontset);
    }
    char xlfdbase[128];
    sprintf(xlfdbase, "-*-*-*-*-*-*-%d-*-*-*-*-*-*-*", height);
    gPreeditFontset = gdk_fontset_load(xlfdbase);
    if (gPreeditFontset) {
      mXIC->SetPreeditFont(gPreeditFontset);
    }
    mXICFontSize = height;
  }
}
void
nsWidget::SetXICSpotLocation(nsPoint aPoint)
{
  if (mIMEEnable == PR_FALSE) return;
  if (nsnull == mXIC) return;
  if (mXIC) {
    unsigned long x, y;
    x = aPoint.x, y = aPoint.y;
    GetXYFromPosition(&x, &y);
    mXIC->SetPreeditSpotLocation(x, y);
  }
  return;
}
#endif // USE_XIM 

/********************** class ModalWidgetList ***********************/
/* This silly little thing is a linked list of widgets that have been
   declared modal, in the order of their declaration. We do this only
   so clients can stack modal dialogs on top of each other, as they
   are wont to do. Yes, glib keeps its own list, but we need our own copy
   so we can temporarily disable them all when a popup control window
   makes an appearance within a modal dialog.
*/
static ModalWidgetList *gModalWidgets = nsnull;

ModalWidgetList::ModalWidgetList(nsWidget *aWidget) {
  mWidget = aWidget;
  mNext = 0;
  mPrev = 0;
  mLast = 0;
}

ModalWidgetList::~ModalWidgetList() {
}

PRBool ModalWidgetList::Find(nsWidget *aWidget) {
  ModalWidgetList *next;

  for (next = gModalWidgets; next && next->mWidget != aWidget; next = next->mNext)
    ;
  return next ? PR_TRUE : PR_FALSE;
}

void ModalWidgetList::Append(nsWidget *aWidget) {

  ModalWidgetList *newElement = new ModalWidgetList(aWidget);

  NS_ASSERTION(newElement, "out of memory in modal widget list creation");
  if (!newElement)
    return;

  if (gModalWidgets) {
    newElement->mPrev = gModalWidgets->mLast;
    gModalWidgets->mLast->mNext = newElement;
    gModalWidgets->mLast = newElement;
  } else {
    newElement->mLast = newElement;
    gModalWidgets = newElement;
  }
}

void ModalWidgetList::Remove(nsWidget *aWidget) {
  NS_ASSERTION(gModalWidgets && gModalWidgets->mLast->mWidget == aWidget,
    "removing modal widgets out of order");
  if (gModalWidgets && gModalWidgets->mLast->mWidget == aWidget)
    ModalWidgetList::RemoveLast();
}

void ModalWidgetList::RemoveLast() {
  NS_ASSERTION(gModalWidgets, "removing modal widgets from empty list");
  if (!gModalWidgets)
    return;

  ModalWidgetList *deadElement = gModalWidgets->mLast;
  if (deadElement->mPrev) {
    deadElement->mPrev->mNext = 0;
    gModalWidgets->mLast = deadElement->mPrev;
  } else
    gModalWidgets = 0;
  delete deadElement;
}

void ModalWidgetList::Suppress(PRBool aSuppress) {

  if (!gModalWidgets)
    return;

  GtkWindow       *window;
  ModalWidgetList *widget;

  if (aSuppress)
    for (widget = gModalWidgets->mLast; widget; widget = widget->mPrev) {
      window = widget->mWidget->GetTopLevelWindow();
       NS_ASSERTION(window, "non-window in modality suppression list");
      gtk_window_set_modal(window, FALSE);
    }
  else
    for (widget = gModalWidgets; widget; widget = widget->mNext) {
      window = widget->mWidget->GetTopLevelWindow();
      NS_ASSERTION(window, "non-window in modality suppression list");
      gtk_window_set_modal(window, TRUE);
    }
}

/* virtual */
GtkWindow *nsWidget::GetTopLevelWindow(void)
{
  if (mWidget) 
    return GTK_WINDOW(gtk_widget_get_toplevel(mWidget));
  else
    return NULL;
}

/* virtual */
void nsWidget::UpdateDragContext(GtkWidget *aWidget, GdkDragContext *aGdkDragContext, guint aTime)
{
  // make sure that we tell the drag manager what the hell is going on.
  nsCOMPtr<nsIDragService> dragService;
  nsresult rv = nsServiceManager::GetService(kCDragServiceCID,
                                             nsIDragService::GetIID(),
                                             (nsISupports **)&dragService);
  if (NS_FAILED(rv)) {
    g_print("*** warning: failed to get the drag service. this is a _bad_ thing.\n");
    return;
  }
  nsCOMPtr<nsIDragSessionGTK> dragServiceGTK;
  dragServiceGTK = do_QueryInterface(dragService);
  if (!dragServiceGTK) {
    return;
  }

  dragServiceGTK->SetLastContext(aWidget, aGdkDragContext, aTime);

}

/* virtual */
void nsWidget::StartDragMotion(GtkWidget *aWidget, GdkDragContext *aGdkDragContext, guint aTime)
{
  // make sure that we tell the drag manager what the hell is going on.
  nsCOMPtr<nsIDragService> dragService;
  nsresult rv = nsServiceManager::GetService(kCDragServiceCID,
                                             nsIDragService::GetIID(),
                                             (nsISupports **)&dragService);
  if (NS_FAILED(rv)) {
    g_print("*** warning: failed to get the drag service. this is a _bad_ thing.\n");
    return;
  }
  nsCOMPtr<nsIDragSessionGTK> dragServiceGTK;
  dragServiceGTK = do_QueryInterface(dragService);
  if (!dragServiceGTK) {
    return;
  }
  dragServiceGTK->StartDragMotion(aWidget, aGdkDragContext, aTime);
}

/* virtual */
void nsWidget::EndDragMotion(GtkWidget *aWidget, GdkDragContext *aGdkDragContext, guint aTime)
{
  // make sure that we tell the drag manager what the hell is going on.
  nsCOMPtr<nsIDragService> dragService;
  nsresult rv = nsServiceManager::GetService(kCDragServiceCID,
                                             nsIDragService::GetIID(),
                                             (nsISupports **)&dragService);
  if (NS_FAILED(rv)) {
    g_print("*** warning: failed to get the drag service. this is a _bad_ thing.\n");
    return;
  }
  nsCOMPtr<nsIDragSessionGTK> dragServiceGTK;
  dragServiceGTK = do_QueryInterface(dragService);
  if (!dragServiceGTK) {
    return;
  }
  dragServiceGTK->EndDragMotion(aWidget, aGdkDragContext, aTime);
}

#ifdef NS_DEBUG

static void setDebugWindow(void)
{
  gchar *text = NULL;
  int val = 0;

  text = gtk_editable_get_chars(GTK_EDITABLE(debugEntryBox), 0, -1);
  if (!text) {
    return;
  }

  if (strlen(text) == 0) {
    g_print("setting value to null\n");
    nsWidget::debugWidget = NULL;
    g_free(text);
    return;
  }
  
  if (strlen(text) < 3) {
    g_print("string not long enough\n");
    return;
  }

  if (memcmp(text, "0x", 2) != 0) {
    g_print("string must begin in 0x\n");
    return;
  }

  sscanf(&text[2], "%x", &val);

  printf("setting value to 0x%x\n", val);
  nsWidget::debugWidget = (nsWidget *)val;

  g_free(text);
}

static void debugHandleActivate(GtkEditable *editable,
                                gpointer user_data)
{
  setDebugWindow();
}

static void      debugHandleClicked (GtkButton   *button,
                                     gpointer user_data)
{
  setDebugWindow();
}

static void      debugHandleToggle  (GtkToggleButton *button,
                                     gpointer user_data)
{
  if (gtk_toggle_button_get_active(button) == TRUE) {
    nsWidget::sDebugFeedback = PR_TRUE;
  }
  else {
    nsWidget::sDebugFeedback = PR_FALSE;
  }
}

static void      debugDestroyWindow (void)
{
  // this will destroy all of the widgets inside the window, too.
  gtk_widget_destroy(debugTopLevel);
  debugTopLevel = NULL;
  debugBox = NULL;
  debugEntryBox = NULL;
  debugButton = NULL;
  debugCheckBox = NULL;
  nsWidget::debugWidget = NULL;  
}

static void      debugSetupWindow   (void)
{
  PRBool   enable_window = PR_FALSE;
  nsresult rv;
  
  debugCheckedDebugWindow = PR_TRUE;
  NS_WITH_SERVICE(nsIPref, prefs, kPrefServiceCID, &rv);
  if (!NS_FAILED(rv) && (prefs)) {
    rv = prefs->GetBoolPref(debugPrefName, &enable_window);
    if (!NS_FAILED(rv) && enable_window) {
      debugTopLevel = gtk_window_new(GTK_WINDOW_TOPLEVEL);
      gtk_signal_connect(GTK_OBJECT(debugTopLevel),
                         "delete_event",
                         GTK_SIGNAL_FUNC(debugHandleWindowClose),
                         NULL);
      
      debugBox = gtk_hbox_new(PR_FALSE, 0);
      gtk_container_add(GTK_CONTAINER(debugTopLevel), debugBox);
      
      debugEntryBox = gtk_entry_new();
      gtk_box_pack_start_defaults(GTK_BOX(debugBox), debugEntryBox);
      gtk_signal_connect(GTK_OBJECT(debugEntryBox), "activate",
                         GTK_SIGNAL_FUNC(debugHandleActivate), NULL);
      
      debugButton = gtk_button_new_with_label("Set Window");
      gtk_box_pack_start_defaults(GTK_BOX(debugBox), debugButton);
      gtk_signal_connect(GTK_OBJECT(debugButton), "clicked",
                         GTK_SIGNAL_FUNC(debugHandleClicked), NULL);

      debugCheckBox = gtk_check_button_new_with_label("Debug Feedback");
      gtk_box_pack_start_defaults(GTK_BOX(debugBox), debugCheckBox);
      gtk_signal_connect(GTK_OBJECT(debugCheckBox), "toggled",
                         GTK_SIGNAL_FUNC(debugHandleToggle), NULL);
      
      gtk_widget_show_all(debugTopLevel);
    }
  }
}

static int debugWindowPrefChanged (const char *newpref, void *data)
{
  PRBool enable_window;
  nsresult rv;
  NS_WITH_SERVICE(nsIPref, prefs, kPrefServiceCID, &rv);
  if (!NS_FAILED(rv) && (prefs)) {
    rv = prefs->GetBoolPref(debugPrefName, &enable_window);
    if (!NS_FAILED(rv) && enable_window) {
      if (!debugTopLevel) {
        // this will trigger the creation of the window
        debugCheckedDebugWindow = PR_FALSE;
        debugSetupWindow();
      }
    }
    else if (!NS_FAILED(rv) && (!enable_window)) {
      if (debugTopLevel) {
        debugDestroyWindow();
      }
    }
  }
  return 0;
}

static void      debugRegisterCallback  (void)
{
  nsresult rv;
  
  // make sure we don't call in here again
  debugCallbackRegistered = PR_TRUE;
  NS_WITH_SERVICE(nsIPref, prefs, kPrefServiceCID, &rv);
  if (!NS_FAILED(rv)) {
    rv = prefs->RegisterCallback(debugPrefName, debugWindowPrefChanged, NULL);
  }
}

static gint debugHandleWindowClose(GtkWidget *window, void *data)
{
  debugDestroyWindow();
  return TRUE;
}

#endif /* NS_DEBUG */

#ifdef USE_XIM
void
nsWidget::ime_preedit_start() {
  IMEComposeStart(nsnull);
}

void
nsWidget::ime_preedit_draw() {
  IMEComposeStart(nsnull);
  nsIMEPreedit *preedit = mXIC->GetPreedit();
  IMEComposeText(nsnull,
                 preedit->GetPreeditString(),
                 preedit->GetPreeditLength(),
                 preedit->GetPreeditFeedback());
  if (mXIC->IsPreeditComposing() == PR_FALSE) {
    IMEComposeEnd(nsnull);
  }
}

void
nsWidget::ime_preedit_done() {
  IMEComposeEnd(nsnull);
}

void
nsWidget::IMEUnsetFocusWidget()
{
  KillICSpotTimer();
}

void
nsWidget::IMESetFocusWidget()
{
  if (!mIMEShellWidget) {
    mIMEShellWidget = GetShellWidget((GdkWindow*)
                                     GetNativeData(NS_NATIVE_WINDOW));
    NS_ASSERTION(mIMEShellWidget, "GetShellWidget() fails");
    if (!mIMEShellWidget) return;
  }
  if (mIMEShellWidget == this) {
    return;
  }
  if (mXIC) {
    if (mXIC->IsPreeditComposing() == PR_FALSE) {
      IMEComposeEnd(nsnull);
    }
    mXIC->SetFocusWidget(this);
    if (gInputStyle & GDK_IM_PREEDIT_POSITION) {
      UpdateICSpot();
      PrimeICSpotTimer();
    }
  } else {
    if (mIMEChildWidget) {
      IMEActivateWidget();
      mIMEChildWidget->IMESetFocusWidget();
      return;
    }
    // kinput2 does not work well if we call UnsetFocusWidget()
    // when mIMEIsDeactivating is true
    if (mIMEShellWidget->mIMEIsDeactivating == PR_FALSE) {
      nsIMEGtkIC::UnsetFocusWidget();
    }
  }
}

void
nsWidget::IMEActivateWidget()
{
  if (!mIMEShellWidget) {
    mIMEShellWidget = GetShellWidget((GdkWindow*)
                                     GetNativeData(NS_NATIVE_WINDOW));
    NS_ASSERTION(mIMEShellWidget, "GetShellWidget() fails");
    if (!mIMEShellWidget) return;
  }
  mIMEShellWidget->mIMEIsDeactivating = PR_FALSE;
}

void
nsWidget::IMEDeactivateWidget()
{
  if (!mIMEShellWidget) {
    mIMEShellWidget = GetShellWidget((GdkWindow*)
                                     GetNativeData(NS_NATIVE_WINDOW));
    NS_ASSERTION(mIMEShellWidget, "GetShellWidget() fails");
    if (!mIMEShellWidget) return;
  }
  mIMEShellWidget->mIMEIsDeactivating = PR_TRUE;
}
#endif // USE_XIM 

void
nsWidget::IMEComposeStart(guint aTime)
{
#ifdef USE_XIM
  if (mIMECallComposeStart == PR_TRUE) {
    return;
  }
#endif // USE_XIM 
  nsCompositionEvent compEvent;
  compEvent.widget = (nsWidget *) this;
  compEvent.point.x = compEvent.point.y = 0;
  compEvent.time = aTime;
  compEvent.message = compEvent.eventStructType
    = compEvent.compositionMessage = NS_COMPOSITION_START;

  OnComposition(compEvent);

#ifdef USE_XIM
  mIMECallComposeStart = PR_TRUE;
  mIMECallComposeEnd = PR_FALSE;
#endif // USE_XIM 
}

void
nsWidget::IMECommitEvent(GdkEventKey *aEvent) {
#ifdef USE_XIM
  if (gInputStyle & GDK_IM_PREEDIT_CALLBACKS) {
    IMECheckPreedit_PreProc();
  }
#endif // USE_XIM 

  PRInt32 srcLen = aEvent->length;

  if (srcLen && aEvent->string && aEvent->string[0] &&
      nsGtkIMEHelper::GetSingleton()) {
    PRInt32 uniCharSize;
    uniCharSize = nsGtkIMEHelper::GetSingleton()->MultiByteToUnicode(
                                aEvent->string,
                                srcLen,
                                &(mIMECompositionUniString),
                                &(mIMECompositionUniStringSize));
    if (uniCharSize) {
      mIMECompositionUniString[uniCharSize] = 0;
      IMEComposeStart(aEvent->time);
      IMEComposeText(aEvent,
                 mIMECompositionUniString,
                 uniCharSize,
                 nsnull);
      IMEComposeEnd(aEvent->time);
    }
  }

#ifdef USE_XIM
  if (gInputStyle & GDK_IM_PREEDIT_CALLBACKS) {
    IMECheckPreedit_PostProc();
  } else if (gInputStyle & GDK_IM_PREEDIT_POSITION) {
    // update spot location
    if (!mIMEShellWidget) {
      mIMEShellWidget = GetShellWidget((GdkWindow*)
                                     GetNativeData(NS_NATIVE_WINDOW));
      NS_ASSERTION(mIMEShellWidget, "GetShellWidget() fails");
      if (!mIMEShellWidget) return;
    }
    nsIMEGtkIC *XIC_tmp = mXIC ? mXIC : mIMEShellWidget->mXIC;
    if (XIC_tmp) {
      nsWidget *widget = XIC_tmp->GetFocusWidget();
      if (widget) {
        widget->UpdateICSpot();
        widget->PrimeICSpotTimer();
      }
    }
  }
#endif // USE_XIM 
}

void
nsWidget::IMEComposeText(GdkEventKey *aEvent,
                         const PRUnichar *aText, const PRInt32 aLen,
                         const char *aFeedback) {
  nsTextEvent textEvent;
  if (aEvent) {
    textEvent.isShift = (aEvent->state & GDK_SHIFT_MASK) ? PR_TRUE : PR_FALSE;
    textEvent.isControl = (aEvent->state & GDK_CONTROL_MASK) ? PR_TRUE : PR_FALSE;
    textEvent.isAlt = (aEvent->state & GDK_MOD1_MASK) ? PR_TRUE : PR_FALSE;
    // XXX
    textEvent.isMeta = PR_FALSE; //(aEvent->state & GDK_MOD2_MASK) ? PR_TRUE : PR_FALSE;
    textEvent.time = aEvent->time;
  } else {
    textEvent.time = 0;
    textEvent.isShift = textEvent.isControl =
      textEvent.isAlt = textEvent.isMeta = PR_FALSE;
  }
  textEvent.message = textEvent.eventStructType = NS_TEXT_EVENT;
  textEvent.widget = (nsWidget *)this;
  textEvent.point.x = textEvent.point.y = 0;

  if (aLen == 0) {
    textEvent.theText = nsnull;
    textEvent.rangeCount = 0;
    textEvent.rangeArray = nsnull;
  } else {
    textEvent.theText = (PRUnichar*)aText;
    textEvent.rangeCount = 0;
    textEvent.rangeArray = nsnull;
#ifdef USE_XIM
    if (aFeedback) {
      nsIMEPreedit::IMSetTextRange(aLen,
                                   aFeedback,
                                   &(textEvent.rangeCount),
                                   &(textEvent.rangeArray));
    }
#endif // USE_XIM 
  }
  OnText(textEvent);
#ifdef USE_XIM
  if (textEvent.rangeArray) {
    delete[] textEvent.rangeArray;
  }
#endif // USE_XIM 
}

void
nsWidget::IMEComposeEnd(guint aTime)
{
#ifdef USE_XIM
  if (mIMECallComposeEnd == PR_TRUE) {
    return;
  }
#endif // USE_XIM 

  nsCompositionEvent compEvent;
  compEvent.widget = (nsWidget *) this;
  compEvent.point.x = compEvent.point.y = 0;
  compEvent.time = aTime;
  compEvent.message = compEvent.eventStructType
    = compEvent.compositionMessage = NS_COMPOSITION_END;
  OnComposition(compEvent);

#ifdef USE_XIM
  mIMECallComposeStart = PR_FALSE;
  mIMECallComposeEnd = PR_TRUE;
#endif // USE_XIM 
}

#ifdef USE_XIM
// FIXME: need to find more efficient way for finding the shell widget
static nsWidget *
GetShellWidget(GdkWindow *gdkWindow)
{
  GdkWindow *parent = gdk_window_get_parent(gdkWindow);
  nsWidget *root_win = nsnull;
  gpointer data;

  while (parent) {
    gdk_window_get_user_data(parent, &data);
    if (GTK_IS_OBJECT(data)) {
      root_win = (nsWidget *) gtk_object_get_data(GTK_OBJECT(data), "nsWindow");
      if(root_win && root_win->mIsToplevel == PR_TRUE){
	return root_win;
      }
    }
    parent = gdk_window_get_parent(parent);
  }
  return nsnull;
}

void
nsWidget::IMEDestroyIC()
{
  if (!mXIC) return;
  if (mICPerShell == PR_FALSE || mIsToplevel == PR_TRUE) {
    delete mXIC;
  }
  mXIC = 0;
}

/*
	workaround for linux/kinput2

	The order of events is not good when linux/kinput2 is used.

	 1. Commit text
	 2. Preedit Draw Callback

        is expected, however, the order is,

	 1. Preedit Draw Callback
	 2. Commit text

	so workaround is,

	 1) when composed text exits, clear the text
	 2) Commit text (in nsGtkEventHandler)
	 3) when composed text exits, redraw the existing preedit
*/

void nsWidget::IMECheckPreedit_PreProc()
{
  // There is a case that IMECommitEvent is called from non-inputfield
  // (bug 17419)
  // mXIC is null so we need to use ShellWidget->mXIC

  if (!mIMEShellWidget) {
    mIMEShellWidget = GetShellWidget((GdkWindow*)
                                     GetNativeData(NS_NATIVE_WINDOW));
    NS_ASSERTION(mIMEShellWidget, "GetShellWidget() fails");
    if (!mIMEShellWidget) return;
  }

  nsIMEGtkIC *XIC_tmp = mXIC ? mXIC : mIMEShellWidget->mXIC;
  if (XIC_tmp && XIC_tmp->IsPreeditComposing() == PR_TRUE) {
    IMEComposeText(nsnull, nsnull, nsnull, nsnull); // clear composed string
    IMEComposeEnd(nsnull);		            // force to end
  }
}

void nsWidget::IMECheckPreedit_PostProc()
{
  // There is a case that IMECommitEvent is called from non-inputfield
  // (bug 17419)
  // mXIC is null so we need to use ShellWidget->mXIC

  if (!mIMEShellWidget) {
    mIMEShellWidget = GetShellWidget((GdkWindow*)
                                     GetNativeData(NS_NATIVE_WINDOW));
    NS_ASSERTION(mIMEShellWidget, "GetShellWidget() fails");
    if (!mIMEShellWidget) return;
  }
  nsIMEGtkIC *XIC_tmp = mXIC ? mXIC : mIMEShellWidget->mXIC;
  if(XIC_tmp && XIC_tmp->IsPreeditComposing() == PR_TRUE) {
    IMEComposeStart(nsnull);  // do not reset compose string
    nsIMEPreedit *preedit = XIC_tmp->GetPreedit();
    IMEComposeText(nsnull,
                   preedit->GetPreeditString(),
                   preedit->GetPreeditLength(),
                   preedit->GetPreeditFeedback());
  }
}
#endif // USE_XIM 
