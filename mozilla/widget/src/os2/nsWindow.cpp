/*
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
 * License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is the Mozilla OS/2 libraries.
 *
 * The Initial Developer of the Original Code is John Fairhurst,
 * <john_fairhurst@iname.com>.  Portions created by John Fairhurst are
 * Copyright (C) 1999 John Fairhurst. All Rights Reserved.
 *
 * Contributor(s): 
 *   Pierre Phaneuf <pp@ludusdesign.com>
 */

#include "nsWindow.h"
#include "nsIAppShell.h"
#include "nsIFontMetrics.h"
#include "nsGUIEvent.h"
#include "nsIRenderingContext.h"
#include "nsIDeviceContext.h"
#include "nsTransform2D.h"
#include "nsGfxCIID.h"
#include "prtime.h"
#include "nsFont.h"
#include "nsTooltipManager.h"
#include "nsISupportsArray.h"
#include "nsITimer.h"
#include "nsIMenuBar.h"
#include "nsIMenuItem.h"
#include "nsHashtable.h"
#include "nsMenu.h"
#include "nsDragService.h"
#include "nsContextMenu.h"

#include "tabapi.h"   // !! TAB-FIX

#include <stdlib.h>
#include <ctype.h>

// HWNDs are mapped to nsWindow objects using a custom presentation parameter,
// which is registered in nsModule -- thanks to Cindy Ross for explaining how
// to do this.
//
// The subclass proc (fnwpNSWindow) calls ProcessMessage() in the object.
// Decisions are taken here about what to do - the purpose of the OnFoo()
// methods is to generate an NS event to the various people who are
// listening, or not.
//
// OS/2 things: remember supplied coords are in the XP space.  There are
// NS2PM methods for conversion of points & rectangles; position is a bit
// different in that it's the *parent* window whose height must be used.
//
// Deferred window positioning is emulated using WinSetMultWindowPos in
// the hopes that there was a good reason for adding it to nsIWidget.
//
// SetColorSpace() is not implemented on purpose.  So there.
//
// John Fairhurst 17-09-98 first version
//        Revised 01-12-98 to inherit from nsBaseWidget.
//        Revised 24-01-99 to use hashtable
//        Revised 15-03-99 for new menu classes
//        Revised 05-06-99 to use pres-params
//        Revised 19-06-99 drag'n'drop, etc.

// XXX don't deliver click-events to obscured parents
BOOL g_bHandlingMouseClick = FALSE;

// --------------------------------------------------------------------------
// HWND -> (nsWindow *) conversion ------------------------------------------

nsWindow *NS_HWNDToWindow( HWND hwnd)
{
   nsWindow *pWnd = nsnull;

   WinQueryPresParam( hwnd, gModuleData.ppMozilla, 0, NULL,
                      sizeof pWnd, &pWnd, QPF_NOINHERIT);
   return pWnd;
}

// --------------------------------------------------------------------------
// NSWindow create / destroy ------------------------------------------------

nsWindow::nsWindow() : nsBaseWidget()
{
   NS_INIT_REFCNT();

   mWnd             = 0;
   mFnWP            = 0;
   mParent          = 0;
   mNextID          = 1;
   mNextCmdID       = 1;
   mSWPs            = 0;
   mlHave           = 0;
   mlUsed           = 0;
   mPointer         = 0;
   mPS              = 0;
   mPSRefs          = 0;
   mDragInside      = FALSE;
   mDeadKey         = 0;
   mHaveDeadKey     = FALSE;
   // This is so that frame windows can be destroyed from their destructors.
   mHackDestroyWnd  = 0;

   mPreferredWidth  = 0;
   mPreferredHeight = 0;
   mWindowState     = nsWindowState_ePrecreate;
   mFont            = nsnull;
   mOS2Toolkit      = nsnull;
   mMenuBar         = nsnull;
   mActiveMenu      = nsnull;
}

// Do a little work in both create methods & call on to a common DoCreate()
nsresult nsWindow::Create( nsIWidget *aParent, const nsRect &aRect,
                       EVENT_CALLBACK aHandleEventFunction,
                       nsIDeviceContext *aContext, nsIAppShell *aAppShell,
                       nsIToolkit *aToolkit, nsWidgetInitData *aInitData)
{
   HWND hwndP = aParent ? (HWND)aParent->GetNativeData( NS_NATIVE_WINDOW)
                        : HWND_DESKTOP;
   DoCreate( hwndP, (nsWindow*) aParent, aRect, aHandleEventFunction,
             aContext, aAppShell, aToolkit, aInitData);

   return NS_OK;
}

nsresult nsWindow::Create( nsNativeWidget aParent, const nsRect &aRect,
                           EVENT_CALLBACK aHandleEventFunction,
                           nsIDeviceContext *aContext, nsIAppShell *aAppShell,
                           nsIToolkit *aToolkit, nsWidgetInitData *aInitData)
{
   // We need to find the nsWindow that goes with the native window, or controls
   // all get the ID of 0, and a zillion toolkits get created.
   //
   nsWindow *pParent = nsnull;
   HWND      hwndP = (HWND) aParent;

   if( hwndP && hwndP != HWND_DESKTOP)
      pParent = NS_HWNDToWindow( hwndP);

   // XXX WC_MOZILLA will probably need a change here
   //
   if( !hwndP)
      hwndP = HWND_DESKTOP;

   DoCreate( hwndP, pParent, aRect, aHandleEventFunction, aContext,
             aAppShell, aToolkit, aInitData);

   return NS_OK;
}

void nsWindow::DoCreate( HWND hwndP, nsWindow *aParent, const nsRect &aRect,
                         EVENT_CALLBACK aHandleEventFunction,
                         nsIDeviceContext *aContext,
                         nsIAppShell *aAppShell, nsIToolkit *aToolkit,
                         nsWidgetInitData *aInitData)
{
   mWindowState = nsWindowState_eInCreate;

   // Must ensure toolkit before attempting to thread-switch!
   if( !mToolkit)
   {
      if( aToolkit)
      {
         mToolkit = aToolkit;
         NS_ADDREF(mToolkit);
      }
      else if( aParent)
         mToolkit = aParent->GetToolkit();
      else
      {
         // it's some top level window with no toolkit passed in.
         // Create a default toolkit with the current thread
         mToolkit = new nsToolkit;
         NS_ADDREF(mToolkit);
         mToolkit->Init( PR_GetCurrentThread());
      }
      mOS2Toolkit = (nsToolkit*) mToolkit;
   }

   // Switch to the PM thread if necessary...
   if( !mOS2Toolkit->IsPMThread())
   {
      ULONG args[7] = { hwndP, (ULONG) aParent, (ULONG) &aRect,
                        (ULONG) aHandleEventFunction,
                        (ULONG) aContext, (ULONG) aAppShell,
                        (ULONG) aInitData };
      MethodInfo info( this, nsWindow::W_CREATE, 7, args);
      mOS2Toolkit->CallMethod( &info);
   }
   else
      // This is potentially virtual; overridden in nsFrameWindow
      RealDoCreate( hwndP, aParent, aRect, aHandleEventFunction,
                    aContext, aAppShell, aInitData);

   mWindowState = nsWindowState_eLive;
}

void nsWindow::RealDoCreate( HWND              hwndP,
                             nsWindow         *aParent,
                             const nsRect     &aRect,
                             EVENT_CALLBACK    aHandleEventFunction,
                             nsIDeviceContext *aContext,
                             nsIAppShell      *aAppShell,
                             nsWidgetInitData *aInitData,
                             HWND              hwndOwner)
{
   // Set up parent data - don't addref to avoid circularity
   mParent = aParent;

   // Set up window style: first give subclass a chance to prepare
   if( aInitData)
      PreCreateWidget( aInitData);

   ULONG style = WindowStyle();
   if( aInitData)
   {
      if( aInitData->clipChildren)
         style |= WS_CLIPCHILDREN;
#if 0
      //
      // Windows has a slightly different idea of what the implications are
      // of a window having or not having the CLIPSIBLINGS style.
      // All 'canvas' components we create must have clipsiblings, or
      // strange things happen & performance actually degrades.
      //
      else
        style &= ~WS_CLIPCHILDREN;
#endif

      if( aInitData->clipSiblings)
         style |= WS_CLIPSIBLINGS;
      else
         style &= ~WS_CLIPSIBLINGS;
   }

   // Create a window: create hidden & then size to avoid swp_noadjust problems
   // owner == parent except for 'borderless top-level' -- see nsCanvas.cpp
   mWnd = WinCreateWindow( hwndP,
                           WindowClass(),
                           0,          // text
                           style,
                           0, 0, 0, 0, // pos/size
                           hwndOwner ? hwndOwner : hwndP,
                           HWND_TOP,
                           mParent ? mParent->GetNextID() : 0,
                           0, 0);      // ctldata, presparams

   NS_ASSERTION( mWnd, "Couldn't create window");

   // Make sure we have a device context from somewhere
   if( aContext)
   {
      mContext = aContext;
      NS_ADDREF(mContext);
   }
   else
   {
      nsresult rc = NS_OK;
      static NS_DEFINE_IID(kDeviceContextCID, NS_DEVICE_CONTEXT_CID);

      rc = nsComponentManager::CreateInstance( kDeviceContextCID, nsnull,
                                               NS_GET_IID(nsIDeviceContext),
                                               (void **)&mContext);
      if( NS_SUCCEEDED(rc))
         mContext->Init( (nsNativeWidget) mWnd);
#ifdef DEBUG
      else
         printf( "Couldn't find DC instance for nsWindow\n");
#endif
   }

   WinSetPresParam( mWnd, PP_FONTNAMESIZE,
                    strlen( gModuleData.pszFontNameSize) + 1,
                    gModuleData.pszFontNameSize);

   Resize( aRect.x, aRect.y, aRect.width, aRect.height, PR_FALSE);

   // Record bounds.  This is XP, the rect of the entire main window in
   // parent space.  Returned by GetBounds().
   // NB: We haven't subclassed yet, so callbacks to change mBounds won't
   //     have happened!
   mBounds = aRect;
   mBounds.height = GetHeight( aRect.height);

   // Record passed in things
   mAppShell = aAppShell;
   NS_IF_ADDREF( mAppShell);
   mEventCallback = aHandleEventFunction;

   if( mParent)
      mParent->AddChild( this);

   // call the event callback to notify about creation
   DispatchStandardEvent( NS_CREATE);
   SubclassWindow( PR_TRUE);
   PostCreateWidget();
}

// How destruction works: A call of Destroy() destroys the PM window.  This
// triggers an OnDestroy(), which frees resources.  If not Destroy'd at
// delete time, Destroy() gets called anyway.

// NOTE: Calling virtual functions from destructors is bad; they always bind
//       in the current object (ie. as if they weren't virtual).  It may even
//       be illegal to call them from here.
//
nsWindow::~nsWindow()
{
   // If the widget was released without calling Destroy() then the native
   // window still exists, and we need to destroy it.
   if( mWindowState != nsWindowState_eDead)
   {
      mWindowState = nsWindowState_eDoingDelete;
      if( mWnd);
         Destroy();
   }
}

// Close this nsWindow
nsresult nsWindow::Destroy()
{
   // Switch to the PM thread if necessary...
   if( mToolkit && !mOS2Toolkit->IsPMThread())
   {
      MethodInfo info( this, nsWindow::W_DESTROY);
      mOS2Toolkit->CallMethod( &info);
   }
   else
   {
      // avoid calling into other objects if we're being deleted, 'cos
      // they must have no references to us.
      if( mWindowState == nsWindowState_eLive && mParent)
         nsBaseWidget::Destroy();

      if( mWnd)
         WinDestroyWindow( mHackDestroyWnd ? mHackDestroyWnd : mWnd);
   }
   return NS_OK;
}

// WM_DESTROY has been received
void nsWindow::OnDestroy()
{
   SubclassWindow( PR_FALSE);
   mWnd = 0;

   // if we were in the middle of deferred window positioning then free up
   if( mSWPs) free( mSWPs);
   mSWPs = 0;
   mlHave = mlUsed = 0;

   // release any ps (erm, probably an error if this is necessary)
   if( mPS)
      WinReleasePS( mPS);
   mPS = 0;

   // release references to context, toolkit, appshell, children
   nsBaseWidget::OnDestroy();

   // kill font
   delete mFont;

   // release menubar
   NS_IF_RELEASE(mMenuBar);

   // dispatching of the event may cause the reference count to drop to 0
   // and result in this object being deleted. To avoid that, add a
   // reference and then release it after dispatching the event.
   //
   // It's important *not* to do this if we're being called from the
   // destructor -- this would result in our destructor being called *again*
   // from the Release() below.  This is very bad...
   if( nsWindowState_eDoingDelete != mWindowState)
   {
      AddRef();
      DispatchStandardEvent( NS_DESTROY);
      Release();
   }

   // dead widget
   mWindowState = nsWindowState_eDead;
}

// --------------------------------------------------------------------------
// PM messaging layer - wndproc, subclasser, default handler ----------------

// the nsWindow procedure for all nsWindows in this toolkit
MRESULT EXPENTRY fnwpNSWindow( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
   // Get the nsWindow for this hwnd
   nsWindow *wnd = NS_HWNDToWindow( hwnd);

   // Messages which get re-routed if their source was an nsWindow
   // (it's very bad to reroute messages whose source isn't an nsWindow,
   // listboxes with scrollbars for example would break)
   switch( msg)
   {
      case WM_CONTROL:
      case WM_HSCROLL:
      case WM_VSCROLL: // !! potential problems here if canvas children
      {
         // assume parent == owner, true for our creations
         HWND hwndChild = WinWindowFromID( hwnd, SHORT1FROMMP( mp1));
         if( hwndChild)
         {
            nsWindow *w = NS_HWNDToWindow( hwndChild);
            if( w)
               wnd = w;
         }
         break;
      }
   }

   MRESULT mRC = 0;

   if( wnd)
   {
      BOOL bPreHandling = g_bHandlingMouseClick;
      if( PR_FALSE == wnd->ProcessMessage( msg, mp1, mp2, mRC) &&
          wnd->GetPrevWP())
      {
         mRC = (wnd->GetPrevWP())( hwnd, msg, mp1, mp2);

      }
      if( !bPreHandling)
         g_bHandlingMouseClick = FALSE;
   }
   else
      /* erm */ mRC = WinDefWindowProc( hwnd, msg, mp1, mp2);

   return mRC;
}

// Subclass (or remove the subclass)
void nsWindow::SubclassWindow( PRBool bState)
{
   NS_PRECONDITION(WinIsWindow( 0/*hab*/, mWnd), "Invalid window handle");
    
   if( PR_TRUE == bState)
   {
      mFnWP = WinSubclassWindow( mWnd, fnwpNSWindow);
      // connect the this pointer to the window handle
      nsWindow *pWin = this; // learn something new about C++ every day...
      WinSetPresParam( mWnd, gModuleData.ppMozilla, sizeof pWin, &pWin);
   } 
   else
   {
      WinSubclassWindow( mWnd, mFnWP);
      WinRemovePresParam( mWnd, gModuleData.ppMozilla);
      mFnWP = nsnull;
   }
}

// 'Window procedure'
PRBool nsWindow::ProcessMessage( ULONG msg, MPARAM mp1, MPARAM mp2, MRESULT &rc)
{
   PRBool result = PR_FALSE; // call the default window procedure
            
   switch( msg)
   {
      case WM_COMMAND: // fire off menu selections
      {
         USHORT usSrc = SHORT1FROMMP( mp2);
         if( usSrc == CMDSRC_MENU || usSrc == CMDSRC_ACCELERATOR)
            result = OnMenuClick( SHORT1FROMMP(mp1));
         break;
      }

      case WM_INITMENU:
         result = OnActivateMenu( HWNDFROMMP(mp2), TRUE);
         break;

      case WM_MENUEND:
         result = OnActivateMenu( HWNDFROMMP(mp2), FALSE);
         break;

      case WMU_SHOW_TOOLTIP:
      {
         nsTooltipEvent event;
         InitEvent( event, NS_SHOW_TOOLTIP);
         event.tipIndex = LONGFROMMP(mp1);
         event.eventStructType = NS_TOOLTIP_EVENT;
         result = DispatchEventInternal(&event);
         break;
      }

      case WMU_HIDE_TOOLTIP:
         result = DispatchStandardEvent( NS_HIDE_TOOLTIP);
         break;

      case WM_CONTROL: // remember this is resent to the orginator...
         result = OnControl( mp1, mp2);
         break;

      case WM_HSCROLL:
      case WM_VSCROLL:
         result = OnScroll( mp1, mp2);
         break;

      case WM_DESTROY: // clean up object
         OnDestroy();
         break;

      case WM_CLOSE:
      {
         // for now... eventually there'll be an nsIFrameWindow which will
         // generate 'close' events which will be veto-able.  Hopefully.
         // (that'll be `re-write the hierarchy' time for us...)
         Destroy();
         result = PR_TRUE;
         break;
      }

      case WM_PAINT:
         result = OnPaint();
         break;

      case WM_CHAR:
         result = OnKey( mp1, mp2);
         break;

      // Mouseclicks: we don't dispatch CLICK events because they just cause
      // trouble: gecko seems to expect EITHER buttondown/up OR click events
      // and so that's what we give it.
      //
      // Plus we make WM_CHORD do a button3down in order to get warp-4 paste
      // behaviour (see nsEditorEventListeners.cpp)

      case WM_BUTTON1DOWN:
         result = DispatchMouseEvent( NS_MOUSE_LEFT_BUTTON_DOWN, 1, mp1, mp2);
         break;
      case WM_BUTTON1UP:
         result = DispatchMouseEvent( NS_MOUSE_LEFT_BUTTON_UP, 1, mp1, mp2);
         break;
      case WM_BUTTON1DBLCLK:
         result = DispatchMouseEvent( NS_MOUSE_LEFT_DOUBLECLICK, 2, mp1, mp2);
         break;

      case WM_BUTTON2DOWN:
         result = DispatchMouseEvent( NS_MOUSE_RIGHT_BUTTON_DOWN, 1, mp1, mp2);
         break;
      case WM_BUTTON2UP:
         result = DispatchMouseEvent( NS_MOUSE_RIGHT_BUTTON_UP, 1, mp1, mp2);
         break;
      case WM_BUTTON2DBLCLK:
         result = DispatchMouseEvent( NS_MOUSE_RIGHT_DOUBLECLICK, 2, mp1, mp2);
         break;

      case WM_CHORD:
         result = DispatchMouseEvent( 0, 1, mp1, mp2);
         break;
      case WM_BUTTON3DOWN:
         result = DispatchMouseEvent( NS_MOUSE_MIDDLE_BUTTON_DOWN, 1, mp1, mp2);
         break;
      case WM_BUTTON3UP:
         result = DispatchMouseEvent( NS_MOUSE_MIDDLE_BUTTON_UP, 1, mp1, mp2);
         break;
      case WM_BUTTON3DBLCLK:
         result = DispatchMouseEvent( NS_MOUSE_MIDDLE_DOUBLECLICK, 2, mp1, mp2);
         break;

      case WM_MOUSEMOVE:
         result = DispatchMouseEvent( NS_MOUSE_MOVE, 0, mp1, mp2);
         break;
      case WMU_MOUSEENTER:
         result = DispatchMouseEvent( NS_MOUSE_ENTER, 0, mp1, mp2);
         break;
      case WMU_MOUSELEAVE:
         result = DispatchMouseEvent( NS_MOUSE_EXIT, 0, mp1, mp2);
         break;

      case WM_SETFOCUS:
         result = DispatchStandardEvent( SHORT1FROMMP( mp2) ? NS_GOTFOCUS
                                                            : NS_LOSTFOCUS);
         break;

      case WM_WINDOWPOSCHANGED: 
         result = OnReposition( (PSWP) mp1);
         break;


      case WM_REALIZEPALETTE:           // hopefully only nsCanvas & nsFrame
         result = OnRealizePalette();   // will need this
         break;

      case WM_PRESPARAMCHANGED:
         // This is really for font-change notifies.  Do that first.
         rc = GetPrevWP()( mWnd, msg, mp1, mp2);
         OnPresParamChanged( mp1, mp2);
         result = PR_TRUE;
         break;

      case DM_DRAGOVER:
         result = OnDragOver( mp1, mp2, rc);
         break;

      case DM_DRAGLEAVE:
         result = OnDragLeave( mp1, mp2);
         break;

      case DM_DROP:
         result = OnDrop( mp1, mp2);
         break;

      // Need to handle this method in order to keep track of whether there
      // is a drag inside the window; we need to do *this* so that we can
      // generate DRAGENTER messages [which os/2 doesn't provide].
      case DM_DROPHELP:
         mDragInside = FALSE;
         break;
   }

   return result;
}

// --------------------------------------------------------------------------
// Local methods to dispatch NS events --------------------------------------

// Convert nsEventStatus value to a ProcessEvents() return code
PRBool nsWindow::ConvertStatus( nsEventStatus aStatus)
{
   PRBool rc = PR_FALSE;

   switch( aStatus)
   {
      case nsEventStatus_eIgnore:
      case nsEventStatus_eConsumeDoDefault:
         break;
      case nsEventStatus_eConsumeNoDefault:
         rc = PR_TRUE;
         break;
      default:
         NS_ASSERTION(0, "Illegal nsEventStatus enumeration value");
         break;
   }

   return rc;
}

// Initialize an event pre-dispatch
void nsWindow::InitEvent( nsGUIEvent &event, PRUint32 aEventType, nsPoint *pt)
{
   event.widget = this;
   event.nativeMsg = nsnull;

   if( !pt)
   {
      // get the message position in client coordinates
      POINTL ptl;
      WinQueryMsgPos( 0/*hab*/, &ptl);
      WinMapWindowPoints( HWND_DESKTOP, mWnd, &ptl, 1);
      PM2NS( ptl);

      event.point.x = ptl.x;
      event.point.y = ptl.y;
   }
   else
   {
      event.point.x = pt->x;
      event.point.y = pt->y;
   }

   event.time = WinQueryMsgTime( 0/*hab*/);
   event.message = aEventType;
}

// Invokes callback and ProcessEvent method on Event Listener object
PRBool nsWindow::DispatchEventInternal( nsGUIEvent *event)
{
   PRBool result = PR_FALSE;

   nsEventStatus status;
   DispatchEvent( event, status);
   result = ConvertStatus( status);

   return result;
}

// NOTE that this is now part of the nsIWidget interface...
NS_IMETHODIMP nsWindow::DispatchEvent( nsGUIEvent *event,
                                       nsEventStatus &aStatus)
{
   aStatus = nsEventStatus_eIgnore;

   // Filters: if state is eInCreate, only send out NS_CREATE
   //          if state is eDoingDelete, don't send out anything because,
   //                                    well, the object's being deleted...
   if( (mWindowState == nsWindowState_eInCreate && event->message == NS_CREATE)
       || (mWindowState == nsWindowState_eLive))
   {
      if( mEventCallback)
         aStatus = (*mEventCallback)( event);
   
      // Dispatch to event listener if event was not consumed
      if( (aStatus != nsEventStatus_eIgnore) && mEventListener)
         aStatus = mEventListener->ProcessEvent(*event);
   }

   return NS_OK;
}


// Dispatch a standard event
PRBool nsWindow::DispatchStandardEvent( PRUint32 aMsg, PRUint8 aEST)
{
   nsGUIEvent event;
   event.eventStructType = aEST;
   InitEvent( event, aMsg);
   return DispatchEventInternal( &event);
}

// Deal with all sort of mouse event
PRBool nsWindow::DispatchMouseEvent( PRUint32 msg, int clickcount,
                                     MPARAM mp1, MPARAM mp2)
{
   PRBool result = PR_FALSE;

   if( !mEventCallback && !mMouseListener)
      return result;

   // Stop multiple messages for the same PM action
   if( g_bHandlingMouseClick)
      return result;

   nsMouseEvent event;
   event.eventStructType = NS_MOUSE_EVENT;
   event.clickCount = clickcount;

   // Mouse leave & enter messages don't seem to have position built in.
   if( msg && msg != NS_MOUSE_ENTER && msg != NS_MOUSE_EXIT)
   {
      POINTL ptl = { SHORT1FROMMP( mp1), SHORT2FROMMP( mp1) };
      PM2NS( ptl);
      nsPoint pt( ptl.x, ptl.y);
      InitEvent( event, msg, &pt);

      USHORT usFlags = SHORT2FROMMP( mp2);
      event.isShift = (usFlags & KC_SHIFT) ? PR_TRUE : PR_FALSE;
      event.isControl = (usFlags & KC_CTRL) ? PR_TRUE : PR_FALSE;
      event.isAlt = (usFlags & KC_ALT) ? PR_TRUE : PR_FALSE;
   }
   else
   {
      if( !msg) msg = NS_MOUSE_MIDDLE_BUTTON_DOWN; // WM_CHORD hack

      InitEvent( event, msg, nsnull);
      event.isShift = WinIsKeyDown( VK_SHIFT);
      event.isControl = WinIsKeyDown( VK_CTRL);
      event.isAlt = WinIsKeyDown( VK_ALT) || WinIsKeyDown( VK_ALTGRAF);
   }

   // call the listeners
   if( mMouseListener)
      switch( msg)
      {
         case NS_MOUSE_MOVE:
            result = ConvertStatus( mMouseListener->MouseMoved( event));
            break;

         case NS_MOUSE_LEFT_BUTTON_DOWN:
         case NS_MOUSE_MIDDLE_BUTTON_DOWN:
         case NS_MOUSE_RIGHT_BUTTON_DOWN:
           result = ConvertStatus( mMouseListener->MousePressed( event));
           break;
   
         case NS_MOUSE_LEFT_BUTTON_UP:
         case NS_MOUSE_MIDDLE_BUTTON_UP:
         case NS_MOUSE_RIGHT_BUTTON_UP:
           result = ConvertStatus( mMouseListener->MouseReleased( event));
           break;

         case NS_MOUSE_LEFT_CLICK:
         case NS_MOUSE_MIDDLE_CLICK:
         case NS_MOUSE_RIGHT_CLICK:
            result = ConvertStatus( mMouseListener->MouseClicked( event));
            break;
      } 
   else
      result = DispatchEventInternal( &event);


   g_bHandlingMouseClick = TRUE;

   return result;
}

// --------------------------------------------------------------------------
// Overridable OnMessage() methods ------------------------------------------

// Key handler.  Specs for the various text messages are really confused;
// see other platforms for best results of how things are supposed to work.
//
// Perhaps more importantly, the main man listening to these events (besides
// random bits of javascript) is ender -- see 
// mozilla/editor/base/nsEditorEventListeners.cpp.
//
PRBool nsWindow::OnKey( MPARAM mp1, MPARAM mp2)
{
   nsKeyEvent event;
   USHORT     fsFlags = SHORT1FROMMP(mp1);
   USHORT     usVKey = SHORT2FROMMP(mp2);
   UCHAR      uchScan = CHAR4FROMMP(mp1);
   int        unirc = ULS_SUCCESS;

   // It appears we're not supposed to transmit shift, control & alt events
   // to gecko.  Shrug.
   //
   // XXX this may be wrong, but is what gtk is doing...
   if( fsFlags & KC_VIRTUALKEY &&
      (usVKey == VK_SHIFT || usVKey == VK_CTRL ||
       usVKey == VK_ALT || usVKey == VK_ALTGRAF)) return PR_FALSE;

   // Now check if it's a dead-key
   if( fsFlags & KC_DEADKEY)
   {
      UniChar tmp;
      unirc = gModuleData.TranslateKey( uchScan, &tmp, &mDeadKey);
      if( unirc == ULS_SUCCESS)
         mHaveDeadKey = TRUE;
      else
         printf( "Couldn't translate dead key\n");

      // XXX CUA says we're supposed to give some kind of feedback `display the
      //     dead key glyph'.  I'm not sure if we can use the COMPOSE messages
      //     to do this -- it should really be done by someone who can test it
      //     & has some idea what `ought' to happen...

      return PR_TRUE;
   }

   // Now dispatch a keyup/keydown event.  This one is *not* meant to
   // have the unicode charcode in.

   InitEvent( event, (fsFlags & KC_KEYUP) ? NS_KEY_UP : NS_KEY_DOWN);
   event.keyCode   = WMChar2KeyCode( mp1, mp2);
   event.isShift   = (fsFlags & KC_SHIFT) ? PR_TRUE : PR_FALSE;
   event.isControl = (fsFlags & KC_CTRL) ? PR_TRUE : PR_FALSE;
   event.isAlt     = (fsFlags & KC_ALT) ? PR_TRUE : PR_FALSE;
   event.eventStructType = NS_KEY_EVENT;
   event.charCode = 0;

   PRBool rc = DispatchEventInternal( &event);

   // Break off now if this was a key-up.
   if( fsFlags & KC_KEYUP) return rc;

   // Break off if we've got an "invalid composition" -- that is, the user
   // typed a deadkey last time, but has now typed something that doesn't
   // make sense in that context.
   if( fsFlags & KC_INVALIDCOMP)
   {
      mHaveDeadKey = FALSE;
      // XXX actually, not sure whether we're supposed to abort the keypress
      //     or process it as though the dead key has been pressed.
      return rc;
   }

   // Now we need to dispatch a keypress event which has the unicode char.
   //
   // Only send keypress events for characters which create textual input.
   //
   if( !(fsFlags & (KC_CHAR | KC_COMPOSITE)))
      return rc;

   event.message = NS_KEY_PRESS;

   VDKEY vdkeyTmp;
   unirc = gModuleData.TranslateKey( uchScan, (UniChar*) &event.charCode,
                                     &vdkeyTmp);

   if( mHaveDeadKey && (fsFlags & KC_COMPOSITE) && unirc == ULS_SUCCESS)
   {
      unirc = UniTranslateDeadkey( gModuleData.hKeyboard,
                                   &mDeadKey,
                                   (UniChar) event.charCode,
                                   (UniChar*) &event.charCode,
                                   &mDeadKey);
      mHaveDeadKey = FALSE;
   }

   if( unirc != ULS_SUCCESS)
   {
      printf( "UniTranslate[Dead]Key returned %d\n", unirc);
      event.charCode = CHAR2FROMMP(mp2);
   }

   return DispatchEventInternal( &event);
}

PRBool nsWindow::OnReposition( PSWP pSwp)
{
   PRBool result = PR_FALSE;
 
   if( pSwp->fl & SWP_MOVE && !(pSwp->fl & SWP_MINIMIZE))
   {
      // need screen coords.
      POINTL ptl = { pSwp->x, pSwp->y + pSwp->cy - 1 };
      PM2NS_PARENT( ptl);
      mBounds.x = ptl.x;
      mBounds.y = ptl.y;
 
      WinMapWindowPoints( GetParentHWND(), HWND_DESKTOP, &ptl, 1);
      result = OnMove( ptl.x, ptl.y);
   }
   if( pSwp->fl & SWP_SIZE && !(pSwp->fl & SWP_MINIMIZE))
      result = OnResize( pSwp->cx, pSwp->cy);

   return result;
}

// Params here are in XP-space for the desktop
PRBool nsWindow::OnMove( PRInt32 aX, PRInt32 aY)
{            
   nsGUIEvent event;
   InitEvent( event, NS_MOVE);
   event.point.x = aX;
   event.point.y = aY;
   event.eventStructType = NS_GUI_EVENT;
   return DispatchEventInternal( &event);
}

PRBool nsWindow::OnPaint()
{
   return PR_FALSE;
}

PRBool nsWindow::OnResize( PRInt32 aX, PRInt32 aY)
{
   mBounds.width = aX;
   mBounds.height = aY;
   return DispatchResizeEvent( aX, aY);
}

PRBool nsWindow::DispatchResizeEvent( PRInt32 aX, PRInt32 aY)
{
   // call the event callback 
   nsSizeEvent event;
   nsRect      rect( 0, 0, aX, aY);

   InitEvent( event, NS_SIZE);
   event.eventStructType = NS_SIZE_EVENT;
   event.windowSize = &rect;             // this is the *client* rectangle
   event.mWinWidth = mBounds.width;
   event.mWinHeight = mBounds.height;

   return DispatchEventInternal( &event);
}                                           

PRBool nsWindow::OnScroll( MPARAM mp1, MPARAM mp2)
{
   return PR_FALSE;
}

PRBool nsWindow::OnRealizePalette()
{
   return PR_FALSE;
}

PRBool nsWindow::OnPresParamChanged( MPARAM mp1, MPARAM mp2)
{
   return PR_FALSE;
}

// This is necessary because notification codes are defined from 1 for each
// control: thus we cannot tell the difference between an EN_SELECT and
// a TABN_SELECT, etc.
// So delegate to those classes who actually want to handle the thing.
PRBool nsWindow::OnControl( MPARAM mp1, MPARAM mp2)
{
   return PR_TRUE; // default to speed things up a bit...
}

// --------------------------------------------------------------------------
// Menus --------------------------------------------------------------------

// wm_commands generated from menus
// XXX should this only be in nsFrameWindow?
//     Probably worth trying for abstraction reasons.
//
PRBool nsWindow::OnMenuClick( USHORT aCmd)
{
   PRBool result = PR_TRUE;

   // find if this is a menuitem being clicked or a submenu
   // (actually I don't think submenu items generate wm_commands...)
   MENUITEM mI = { 0 };

#ifdef SUPPORT_NON_XPFE
   if( mMenuBar || mActiveMenu)
#endif
   {
      void *hwndMenu = 0;
      // context menu takes precedence over menubar
      if( mActiveMenu)
      {
         mActiveMenu->GetNativeData( &hwndMenu);
         mActiveMenu = 0; // now forget it
      }
      else
         mMenuBar->GetNativeData( hwndMenu);

      mOS2Toolkit->SendMsg( (HWND) hwndMenu, MM_QUERYITEM,
                            MPFROM2SHORT(aCmd,TRUE), MPFROMP(&mI));
   }

   if( !(mI.afStyle & MIS_SUBMENU))
   {
      // Find the nsIMenuItem for this selection
      nsISupports     *aThing    = (nsISupports*) mI.hItem;
      nsIMenuItem     *aItem     = nsnull;
      nsIMenuListener *aListener = nsnull;

#ifndef SUPPORT_NON_XPFE
      NS_ASSERTION( aThing, "Disconnected menuitem");
#else
      // viewer.exe uses pure native menus, and so aThing will be null there.
      if( aThing)
#endif
      {
         aThing->QueryInterface( NS_GET_IID(nsIMenuItem),
                                 (void**) &aItem);
      }

      // Fill out a menu event
      nsMenuEvent event;
      event.eventStructType = NS_MENU_EVENT;
      InitEvent(event, NS_MENU_SELECTED);
      event.mMenuItem = aItem;

#ifdef SUPPORT_NON_XPFE
      if( !aItem)
         event.mCommand = aCmd;
      else
#endif
         aItem->GetCommand( event.mCommand);

#ifdef SUPPORT_NON_XPFE
      // Notify those interested
      result = DispatchEventInternal( &event); // XXX just for viewer.exe

      if( aThing)
#endif
      {
         aThing->QueryInterface( NS_GET_IID(nsIMenuListener),
                                 (void**) &aListener);
         result = ConvertStatus( aListener->MenuItemSelected( event));
      }

      // Clean up
      NS_IF_RELEASE(aItem);
      NS_IF_RELEASE(aListener);
   }

   return result;
}

PRBool nsWindow::OnActivateMenu( HWND hwndMenu, BOOL aActivate)
{
   nsMenuBase *pBase = (nsMenuBase*) WinQueryWindowPtr( hwndMenu, QWL_USER);
   PRBool      result = PR_FALSE;

   if( nsnull != pBase)
   {
      nsMenu *aMenu = (nsMenu*) pBase; // XXX yeargh

      // Fill out a menu event
      nsMenuEvent event;
      event.eventStructType = NS_MENU_EVENT;
      // even if this is a deselect, same event type.
      InitEvent( event, NS_MENU_SELECTED);
      event.mMenuItem = nsnull; // XXX I guess
      event.mCommand = 0;       // XXX I guess

#ifdef SUPPORT_NON_XPFE
      // not sure if this is necessary, but can't do any harm.
      DispatchEventInternal( &event);
#endif

      // dispatch to listener
      nsEventStatus es;
      if( aActivate)
         es = aMenu->MenuSelected( event);
      else
         es = aMenu->MenuDeselected( event);

      result = ConvertStatus( es);
   }

   return result;
}

// (there needs to be a distinct nsTopLevelWindow class)
nsresult nsWindow::SetMenuBar( nsIMenuBar *aMenuBar)
{
   NS_IF_RELEASE(mMenuBar);
   mMenuBar = aMenuBar;
   NS_ADDREF(mMenuBar);

   return NS_OK;
}

nsresult nsWindow::ShowMenuBar( PRBool bShow)
{
   if( mMenuBar)
   {
      HWND hwndMenu = 0;
      mMenuBar->GetNativeData( (void*&)hwndMenu);
      if( WinIsWindowVisible(hwndMenu) != (BOOL)bShow)
      {
         WinSetWindowUShort( mWnd, QWS_ID, bShow ? FID_MENU : 0);
         mOS2Toolkit->SendMsg( GetMainWindow(), WM_UPDATEFRAME,
                               MPFROMLONG(FCF_MENU));
      }
   }

   return NS_OK;
}

void nsWindow::SetContextMenu( nsContextMenu *aMenu)
{
   mActiveMenu = aMenu;
}

// --------------------------------------------------------------------------
// Hierarchy - children & parent --------------------------------------------

// We keep a pointer to our parent because we need parent a lot for doing
// positioning things.  We don't hold a reference to it, though.  This should
// be okay 'cos there's no scope for reparenting in this library and we
// can't live longer than it.

// First, nsIWidget methods
nsIWidget* nsWindow::GetParent()
{
   nsWindow *widget = nsnull;
   if( nsnull != mParent)
   {
      NS_ADDREF(mParent);
      widget = mParent;
   }

   return widget;
}

// Now, OS/2 methods
HWND nsWindow::GetParentHWND() const
{
   HWND hwnd = 0;
   if( nsnull != mParent)
      hwnd = mParent->mWnd;
   else
      hwnd = WinQueryWindow( GetMainWindow(), QW_PARENT);
   return hwnd;
}

// ptl is in parent's space
void nsWindow::NS2PM_PARENT( POINTL &ptl)
{
   if( mParent)
      mParent->NS2PM( ptl);
   else
   {
      HWND hwndp = WinQueryWindow( GetMainWindow(), QW_PARENT);
      SWP  swp = { 0 };
      WinQueryWindowPos( hwndp, &swp);
      ptl.y = swp.cy - ptl.y - 1;
   }
}

// ptl is in this window's space
void nsWindow::NS2PM( POINTL &ptl)
{
   ptl.y = GetClientHeight() - ptl.y - 1;
}

// --------------------------------------------------------------------------
// Physical properties - size, position, visibility

// Hide or show this window
nsresult nsWindow::Show( PRBool bState)
{
   // doesn't seem to require a message queue.
   if( mWnd)
      WinShowWindow( GetMainWindow(), !!bState);

   return NS_OK;
}

// Move this component (WinSetWindowPos() appears not to require a msgq)
nsresult nsWindow::Move( PRUint32 aX, PRUint32 aY)
{
   Resize( aX, aY, mBounds.width, mBounds.height, PR_FALSE);
   return NS_OK;
}

// Resize this component: need to keep top-left corner in the same place
nsresult nsWindow::Resize( PRUint32 aWidth, PRUint32 aHeight, PRBool aRepaint)
{
   Resize( mBounds.x, mBounds.y, aWidth, aHeight, aRepaint);
   return NS_OK;
}

// Resize this component
nsresult nsWindow::Resize( PRUint32 aX, PRUint32 aY, PRUint32 w, PRUint32 h,
                           PRBool aRepaint)
{
   if( mWnd)
   {
      // work out real coords of top left
      POINTL ptl= { aX, aY };
      NS2PM_PARENT( ptl);
      // work out real coords of bottom left
      ptl.y -= GetHeight( h) - 1;

      if( !SetWindowPos( 0, ptl.x, ptl.y, w, GetHeight(h), SWP_MOVE | SWP_SIZE))
         if( aRepaint)
            Update();
   }
   else
   {
      mBounds.x = aX;
      mBounds.y = aY;
      mBounds.width = w;
      mBounds.height = h;
   }

   return NS_OK;
}

BOOL nsWindow::SetWindowPos( HWND ib, long x, long y, long cx, long cy, ULONG flags)
{
   BOOL bDeferred = FALSE;

   if( mParent && mParent->mSWPs) // XXX bit implicit...
   {
      mParent->DeferPosition( GetMainWindow(), ib, x, y, cx, cy, flags);
      bDeferred = TRUE;
   }
   else // WinSetWindowPos appears not to need msgq (hmm)
      WinSetWindowPos( GetMainWindow(), ib, x, y, cx, cy, GetSWPFlags(flags));

   // When the window is actually sized, mBounds will be updated in the fnwp.

   return bDeferred;
}

// Enable/disable this window
nsresult nsWindow::Enable( PRBool bState)
{
   if( mWnd)
      WinEnableWindow( GetMainWindow(), !!bState);
   return NS_OK;
}

// Give the focus to this component
nsresult nsWindow::SetFocus()
{
   // Switch to the PM thread if necessary...
   if( !mOS2Toolkit->IsPMThread())
   {
      MethodInfo info(this, nsWindow::W_SET_FOCUS);
      mOS2Toolkit->CallMethod(&info);
   }
   else if( mWnd)
      WinSetFocus( HWND_DESKTOP, mWnd);
   return NS_OK;
}

nsresult nsWindow::IsVisible( PRBool &aState)
{
   // I guess this means visible & not showing...
   BOOL b = WinIsWindowVisible( mWnd);
   aState = b ? PR_TRUE : PR_FALSE;
   return NS_OK;
}

nsresult nsWindow::ModalEventFilter( PRBool aRealEvent,
                                     void */*aEvent*/,
                                     PRBool *aForWindow )
{
   if( PR_FALSE == aRealEvent)
      *aForWindow = PR_FALSE;
   else
   {
      // Set aForWindow if either:
      //   * the message is for a descendent of the given window
      //   * the message is for another window, but is a message which
      //     should be allowed for a disabled window.

      PRBool isMouseEvent = PR_FALSE;
      PRBool isInWindow = PR_FALSE;

      // Examine the target window & find the frame
      // XXX should GetNativeData() use GetMainWindow() ?
      HWND hwnd = (HWND)GetNativeData(NS_NATIVE_WINDOW);
      hwnd = WinQueryWindow(hwnd, QW_PARENT);

      if( hwnd == mQmsg.hwnd || WinIsChild( mQmsg.hwnd, hwnd))
         isInWindow = PR_TRUE;

      // XXX really ought to do something about focus here

      if( !isInWindow)
      {
         // Block mouse messages for non-modal windows
         if( mQmsg.msg >= WM_MOUSEFIRST && mQmsg.msg <= WM_MOUSELAST)
            isMouseEvent = PR_TRUE;
         else if( mQmsg.msg >= WM_MOUSETRANSLATEFIRST &&
                  mQmsg.msg <= WM_MOUSETRANSLATELAST)
            isMouseEvent = PR_TRUE;
         else if( mQmsg.msg == WMU_MOUSEENTER || mQmsg.msg == WMU_MOUSELEAVE)
            isMouseEvent = PR_TRUE;
      }

      // set dispatch indicator
      *aForWindow = isInWindow || !isMouseEvent;
   }

   return NS_OK;
}

// nsFrameWindow overrides this...
nsresult nsWindow::GetClientBounds( nsRect &aRect)
{
   aRect.x = 0;
   aRect.y = 0;
   aRect.width = mBounds.width;
   aRect.height = mBounds.height;
   return NS_OK;
}

// We don' wan' no steekin' borda!
nsresult nsWindow::GetBorderSize( PRInt32 &aWidth, PRInt32 &aHeight)
{
   aWidth = 0;
   aHeight = 0;
   return NS_OK;
}

// Preferred size; default here.
nsresult nsWindow::GetPreferredSize( PRInt32 &aWidth, PRInt32 &aHeight)
{
   aWidth = mPreferredWidth;
   aHeight = mPreferredHeight;
   return NS_OK;
}

nsresult nsWindow::SetPreferredSize( PRInt32 aWidth, PRInt32 aHeight)
{
   mPreferredWidth = aWidth;
   mPreferredHeight = aHeight;
   return NS_OK;
}

// Deferred window positioning
nsresult nsWindow::BeginResizingChildren()
{
   if( !mSWPs)
   {
      mlHave = 10;
      mlUsed = 0;
      mSWPs = (PSWP) malloc( 10 * sizeof( SWP));
   }
   return NS_OK;
}

void nsWindow::DeferPosition( HWND hwnd, HWND hwndInsertBehind,
                              long x, long y, long cx, long cy, ULONG flags)
{
   if( mSWPs)
   {
      if( mlHave == mlUsed) // need more swps
      {
         mlHave += 10;
         mSWPs = (PSWP) realloc( mSWPs, mlHave * sizeof( SWP));
      }
      mSWPs[ mlUsed].hwnd = hwnd;
      mSWPs[ mlUsed].hwndInsertBehind = hwndInsertBehind;
      mSWPs[ mlUsed].x = x;
      mSWPs[ mlUsed].y = y;
      mSWPs[ mlUsed].cx = cx;
      mSWPs[ mlUsed].cy = cy;
      mSWPs[ mlUsed].fl = flags;
      mSWPs[ mlUsed].ulReserved1 = 0;
      mSWPs[ mlUsed].ulReserved2 = 0;
      mlUsed++;
   }
}

nsresult nsWindow::EndResizingChildren()
{
   if( nsnull != mSWPs)
   {
      WinSetMultWindowPos( 0/*hab*/, mSWPs, mlUsed);
      free( mSWPs);
      mSWPs = nsnull;
      mlUsed = mlHave = 0;
   }
   return NS_OK;
}

// Screen <--> window coordinate conversion
nsresult nsWindow::WidgetToScreen( const nsRect &aOldRect, nsRect &aNewRect)
{
   POINTL pt = { aOldRect.x, aOldRect.y };
   NS2PM( pt);

   WinMapWindowPoints( mWnd, HWND_DESKTOP, &pt, 1);

   aNewRect.x = pt.x;
   aNewRect.y = WinQuerySysValue( HWND_DESKTOP, SV_CYSCREEN) - pt.y - 1;
   aNewRect.width = aOldRect.width;
   aNewRect.height = aOldRect.height;
   return NS_OK;
}

nsresult nsWindow::ScreenToWidget( const nsRect &aOldRect, nsRect &aNewRect)
{
   POINTL pt = { aOldRect.x,
                 WinQuerySysValue( HWND_DESKTOP, SV_CYSCREEN) - aOldRect.y - 1 };
   WinMapWindowPoints( HWND_DESKTOP, mWnd, &pt, 1);

   PM2NS( pt);

   aNewRect.x = pt.x;
   aNewRect.y = pt.y;
   aNewRect.width = aOldRect.width;
   aNewRect.height = aOldRect.height;
   return NS_OK;
}

// --------------------------------------------------------------------------
// Colours, fonts, painting -------------------------------------------------

nsresult nsWindow::Paint( nsIRenderingContext &aRenderingContext,
                          const nsRect &aDirtyRect)
{
   printf( "Yikes - nsWindow::Paint called.  Run for the hills...\n");
#if 0  // XXX This is thankfully done in XP-land now.
   // Get a proxy window
   HWND hwndProxy = gModuleData.GetWindowForPrinting( WindowClass(),
                                                      WindowStyle());

   // Size it correctly
   WinSetWindowPos( hwndProxy, HWND_TOP, 0, 0, mBounds.width, mBounds.height,
                    SWP_SIZE | SWP_ZORDER);

   // Set pres-params: colors..

   // ..and font

   // Subclass specific bits
   SetupForPrint( hwndProxy);

   // now get a HPS from the rc & do the WM_DRAW thing
   // (via drawingsurface, QI/cast)
#endif

   return NS_OK;
}

nscolor nsWindow::QueryPresParam( ULONG ppID)
{
   nscolor col = 0;
   RGB2    rgb;
   ULONG   found = 0;

   WinQueryPresParam( mWnd, ppID, 0, &found,
                      sizeof( RGB2), &rgb, QPF_PURERGBCOLOR);

   if( found == ppID)
      col = NS_RGB( rgb.bRed, rgb.bGreen, rgb.bBlue);
   return col;
}

void nsWindow::SetPresParam( ULONG ppID, const nscolor &c)
{
   RGB2 rgb = { NS_GET_B( c), NS_GET_G( c), NS_GET_R( c), 0 };
   WinSetPresParam( mWnd, ppID, sizeof( RGB2), &rgb);
}

nscolor nsWindow::GetForegroundColor()
{
   return (mWnd ? QueryPresParam( PP_FOREGROUNDCOLOR) : mForeground);
}

nsresult nsWindow::SetForegroundColor( const nscolor &aColor)
{
   if( mWnd)
      SetPresParam( PP_FOREGROUNDCOLOR, aColor);
   mForeground = aColor;

   return NS_OK;
}

nscolor nsWindow::GetBackgroundColor()
{
   return (mWnd ? QueryPresParam( PP_BACKGROUNDCOLOR) : mBackground);
}

nsresult nsWindow::SetBackgroundColor(const nscolor &aColor)
{
   if( mWnd)
      SetPresParam( PP_BACKGROUNDCOLOR, aColor);
   mBackground = aColor;
   return NS_OK;
}

// Well this should be interesting...
nsIFontMetrics *nsWindow::GetFont()
{
   nsIFontMetrics *metrics = nsnull;

   if( mToolkit)
   {
      char buf[2][128];
      int  ptSize;
   
      WinQueryPresParam( mWnd, PP_FONTNAMESIZE, 0, 0, 128, buf[0], 0);
   
      if( 2 == sscanf( buf[0], "%d.%s", &ptSize, buf[1])) // mmm, scanf()...
      {
         float twip2dev, twip2app;
         mContext->GetTwipsToDevUnits( twip2dev);
         mContext->GetDevUnitsToAppUnits( twip2app);
         twip2app *= twip2dev;
   
         nscoord appSize = (nscoord) (twip2app * ptSize * 20);
   
         nsFont font( buf[1], NS_FONT_STYLE_NORMAL, NS_FONT_VARIANT_NORMAL,
                      NS_FONT_WEIGHT_NORMAL, 0 /*decoration*/, appSize);
   
         mContext->GetMetricsFor( font, metrics);
      }
   }

   return metrics;
}

nsresult nsWindow::SetFont( const nsFont &aFont)
{
   if( mToolkit) // called from print-routine (XXX check)
   {
      const char *fontname = gModuleData.ConvertFromUcs( aFont.name);
   
      // jump through hoops to convert the size in the font (in app units)
      // into points. 
      float dev2twip, app2twip;
      mContext->GetDevUnitsToTwips( dev2twip);
      mContext->GetAppUnitsToDevUnits( app2twip);
      app2twip *= dev2twip;
   
      int points = NSTwipsToFloorIntPoints( nscoord( aFont.size * app2twip));
   
      char *buffer = new char [ strlen( fontname) + 6];
      sprintf( buffer, "%d.%s", points, fontname);

      BOOL rc = WinSetPresParam( mWnd, PP_FONTNAMESIZE,
                                 strlen( buffer) + 1, buffer);
      if( !rc)
         printf( "WinSetPresParam PP_FONTNAMESIZE %s failed\n", buffer);
   
      delete [] buffer;
   }

   if( !mFont)
      mFont = new nsFont( aFont);
   else
      *mFont = aFont;

   return NS_OK;
}

// SetColorMap - not implemented.
// Any palette lives in the device context & should be altered there.
// And shouldn't be altered at all, once libimg has decided what should
// be in it.  So my opinion is this method shouldn't be called.
nsresult nsWindow::SetColorMap( nsColorMap *aColorMap)
{
   return NS_ERROR_NOT_IMPLEMENTED;
}

// Cursor - more bizarre types not implemented yet
    
nsresult nsWindow::SetCursor( nsCursor aCursor)
{
   ULONG sptr = 0;
   switch( aCursor)
   {
      // builtins
      case eCursor_select:   sptr = SPTR_TEXT;   break;
      case eCursor_wait:     sptr = SPTR_WAIT;   break;
      case eCursor_sizeWE:   sptr = SPTR_SIZEWE; break;
      case eCursor_sizeNS:   sptr = SPTR_SIZENS; break;
      case eCursor_standard: sptr = SPTR_ARROW;  break;
      // custom
      case eCursor_hyperlink:
      case eCursor_arrow_west_plus:
      case eCursor_arrow_west:
      case eCursor_arrow_east_plus:
      case eCursor_arrow_east:
      case eCursor_arrow_south_plus:
      case eCursor_arrow_south:
      case eCursor_arrow_north:
      case eCursor_arrow_north_plus:
         break;

      default:
         NS_ASSERTION( 0, "Unknown cursor type");
         break;
   }

   if( sptr)
      mPointer = WinQuerySysPointer( HWND_DESKTOP, sptr, FALSE);
   else
      mPointer = gModuleData.GetPointer( aCursor);

   WinSetPointer( HWND_DESKTOP, mPointer);
   mCursor = aCursor;

   return NS_OK;
}

// Invalidate and force a redraw
nsresult nsWindow::Invalidate( PRBool aIsSynchronous)
{
   if( mWnd)
   {
      WinInvalidateRect( mWnd, 0, FALSE);
#if 0
      if( PR_TRUE == aIsSynchronous)
         Update();
#endif
   }
   return NS_OK;
}

nsresult nsWindow::Invalidate( const nsRect &aRect, PRBool aIsSynchronous)
{
   if( mWnd)
   {
      // XXX could do with NS2PM for rectangles here...
      RECTL rcl;
      rcl.xLeft = aRect.x;
      rcl.xRight = aRect.x + aRect.width;
      rcl.yTop = GetClientHeight() - aRect.y;
      rcl.yBottom = rcl.yTop - aRect.height + 1;

      WinInvalidateRect( mWnd, &rcl, FALSE);
#if 0
      if( PR_TRUE == aIsSynchronous)
         Update();
#endif
   }
   return NS_OK;
}

// force invalid areas to be updated
nsresult nsWindow::Update()
{
   // Switch to the PM thread if necessary...
   if( !mOS2Toolkit->IsPMThread())
   {
      MethodInfo info(this, nsWindow::W_UPDATE_WINDOW);
      mOS2Toolkit->CallMethod(&info);
   }
   else if( mWnd)
      WinUpdateWindow( mWnd);
   return NS_OK;
}

nsresult nsWindow::SetTitle( const nsString &aTitle) 
{
   // Switch to the PM thread if necessary...
   if( mOS2Toolkit && !mOS2Toolkit->IsPMThread())
   {
      ULONG ulong = (ULONG) &aTitle;
      MethodInfo info( this, nsWindow::W_SET_TITLE, 1, &ulong);
      mOS2Toolkit->CallMethod( &info);
   }
   else if( mWnd)
   {
      WinSetWindowText( GetMainWindow(),
                        gModuleData.ConvertFromUcs( aTitle));
   }
   return NS_OK;
} 

nsresult nsWindow::GetWindowText( nsString &aStr, PRUint32 *rc)
{
   // Switch to the PM thread if necessary...
   if( !mOS2Toolkit->IsPMThread())
   {
      ULONG args[] = { (ULONG) &aStr, (ULONG) rc };
      MethodInfo info( this, nsWindow::W_GET_TITLE, 2, args);
      mOS2Toolkit->CallMethod( &info);
   }
   else if( mWnd)
   {
      // XXX there must be some way to query the text straight into the string!
      int length = WinQueryWindowTextLength( mWnd);
      char *tmp = new char [ length + 1 ];
      WinQueryWindowText( mWnd, length + 1, tmp);
      aStr = tmp;
      delete [] tmp;
   }
   return NS_OK;
}

// Scroll the bits of a window - this may need to bye thread-switched
nsresult nsWindow::Scroll( PRInt32 aDx, PRInt32 aDy, nsRect *aClipRect)
{
   RECTL rcl;

   if( aClipRect)
   {
      rcl.xLeft = aClipRect->x;
      rcl.yBottom = aClipRect->y + aClipRect->height - 1;
      NS2PM( (POINTL&) rcl); // hmm
      rcl.xRight = rcl.xLeft + aClipRect->width;
      rcl.yTop = rcl.yBottom + aClipRect->height;
      // this rect is inex
   }

   WinScrollWindow( mWnd, aDx, -aDy, aClipRect ? &rcl : 0, 0, 0,
                    0, SW_SCROLLCHILDREN | SW_INVALIDATERGN);
   Update();
   return NS_OK;
}

// --------------------------------------------------------------------------
// Misc helpers -------------------------------------------------------------

// May need to thread-switch these...
void nsWindow::AddToStyle( ULONG style)
{
   if( mWnd)
      WinSetWindowBits( mWnd, QWL_STYLE, style, style);
}

void nsWindow::RemoveFromStyle( ULONG style)
{
   if( mWnd)
   {
      ULONG oldStyle = WinQueryWindowULong( mWnd, QWL_STYLE);
      oldStyle &= ~style;
      WinSetWindowULong( mWnd, QWL_STYLE, oldStyle);
   }
}

void nsWindow::GetStyle( ULONG &out)
{
   if( mWnd)
      out = WinQueryWindowULong( mWnd, QWL_STYLE);
}

// --------------------------------------------------------------------------
// Tooltips -----------------------------------------------------------------

nsresult nsWindow::SetTooltips( PRUint32 cTips, nsRect *areas[])
{
   nsTooltipManager::GetManager()->SetTooltips( this, cTips, areas);
   return NS_OK;
}

nsresult nsWindow::UpdateTooltips( nsRect *aNewTips[])
{
   nsTooltipManager::GetManager()->UpdateTooltips( this, aNewTips);
   return NS_OK;
}

nsresult nsWindow::RemoveTooltips()
{
   nsTooltipManager::GetManager()->RemoveTooltips( this);
   return NS_OK;
}

// --------------------------------------------------------------------------
// Drag'n'drop --------------------------------------------------------------

#define DispatchDragDropEvent(msg) DispatchStandardEvent(msg,NS_DRAGDROP_EVENT)

PRBool nsWindow::OnDragOver( MPARAM mp1, MPARAM mp2, MRESULT &mr)
{
   // Drawing drop feedback should be fun, have to get DrgGetPS() involved
   // somehow.

   // Tell drag service about the drag
   gModuleData.dragService->InitDragOver( (PDRAGINFO) mp1);

   // Invoke gecko for enter if appropriate
   if( !mDragInside)
   {
      DispatchDragDropEvent( NS_DRAGDROP_ENTER);
      mDragInside = TRUE;
   }

   // Invoke for 'over' to set candrop flag
   DispatchDragDropEvent( NS_DRAGDROP_OVER);

   // Get action back from drag service
   mr = gModuleData.dragService->TermDragOver();

   return PR_TRUE;
}

PRBool nsWindow::OnDragLeave( MPARAM mp1, MPARAM mp2)
{
   gModuleData.dragService->InitDragExit( (PDRAGINFO) mp1);
   DispatchDragDropEvent( NS_DRAGDROP_EXIT);
   gModuleData.dragService->TermDragExit();

   mDragInside = FALSE;

   return PR_TRUE;
}

PRBool nsWindow::OnDrop( MPARAM mp1, MPARAM mp2)
{
   gModuleData.dragService->InitDrop( (PDRAGINFO) mp1);
   DispatchDragDropEvent( NS_DRAGDROP_DROP);
   gModuleData.dragService->TermDrop();

   mDragInside = FALSE;

   return PR_TRUE;
}

// --------------------------------------------------------------------------
// Raptor object access

// 'Native data'
void *nsWindow::GetNativeData( PRUint32 aDataType)
{
   void *rc = 0;

   switch( aDataType)
   {
      case NS_NATIVE_WIDGET:
      case NS_NATIVE_WINDOW:
      case NS_NATIVE_PLUGIN_PORT:
         rc = (void*) mWnd;
         break;

      case NS_NATIVE_GRAPHIC:
         if( !mPS)
         {
            if( mDragInside) mPS = DrgGetPS( mWnd);
            else mPS = WinGetPS( mWnd);
         }
         mPSRefs++;
         rc = (void*) mPS;
         break;

      case NS_NATIVE_COLORMAP:
      case NS_NATIVE_DISPLAY:
      case NS_NATIVE_REGION:
      case NS_NATIVE_OFFSETX:
      case NS_NATIVE_OFFSETY: // could do this, I suppose; but why?
                              // OTOH, this might make plugins work!
         break;

      default: 
#ifdef DEBUG
         printf( "*** Someone's added a new NS_NATIVE value...\n");
#endif
         break;
   }

   return rc;
}

void nsWindow::FreeNativeData( void *aDatum, PRUint32 aDataType)
{
   switch( aDataType)
   {
      case NS_NATIVE_GRAPHIC:
         mPSRefs--;
         if( !mPSRefs)
         {
            BOOL rc;
            if( mDragInside) rc = DrgReleasePS( mPS);
            else rc = WinReleasePS( mPS);
            if( !rc)
               printf( "Error from {Win/Drg}ReleasePS()\n");
            mPS = 0;
         }
         break;

      case NS_NATIVE_COLORMAP:
      case NS_NATIVE_DISPLAY:
      case NS_NATIVE_REGION:
      case NS_NATIVE_OFFSETX:
      case NS_NATIVE_OFFSETY:
      case NS_NATIVE_WIDGET:
      case NS_NATIVE_WINDOW:
      case NS_NATIVE_PLUGIN_PORT:
         break;

      default: 
#ifdef DEBUG
         printf( "*** Someone's added a new NS_NATIVE value...\n");
#endif
         break;
   }
}

// Thread switch callback
nsresult nsWindow::CallMethod(MethodInfo *info)
{
   nsresult rc = NS_ERROR_FAILURE;

   switch( info->methodId)
   {
      case nsWindow::W_CREATE:
         NS_ASSERTION(info->nArgs == 7, "Bad args to Create");
         DoCreate( (HWND)               info->args[0],
                   (nsWindow*)          info->args[1],
                   (const nsRect&)*(nsRect*) (info->args[2]),
                   (EVENT_CALLBACK)    (info->args[3]), 
                   (nsIDeviceContext*) (info->args[4]),
                   (nsIAppShell*)      (info->args[5]),
                   nsnull, /* toolkit */
                   (nsWidgetInitData*) (info->args[6]));
         rc = NS_OK;
         break;

      case nsWindow::W_DESTROY:
         NS_ASSERTION(info->nArgs == 0, "Bad args to Destroy");
         Destroy();
         rc = NS_OK;
         break;

      case nsWindow::W_SET_FOCUS:
         NS_ASSERTION(info->nArgs == 0, "Bad args to SetFocus");
         SetFocus();
         rc = NS_OK;
         break;

      case nsWindow::W_UPDATE_WINDOW:
         NS_ASSERTION(info->nArgs == 0, "Bad args to UpdateWindow");
         Update();
         rc = NS_OK;
         break;

      case nsWindow::W_SET_TITLE:
         NS_ASSERTION(info->nArgs == 1, "Bad args to SetTitle");
         SetTitle( (const nsString &) info->args[0]);
         rc = NS_OK;
         break;

      case nsWindow::W_GET_TITLE:
         NS_ASSERTION(info->nArgs == 2, "Bad args to GetTitle");
         rc = GetWindowText( *((nsString*) info->args[0]),
                             (PRUint32*)info->args[1]);
         break;

      default:
         break;
   }

   return rc;
}

// function to translate from a WM_CHAR to an NS VK_ constant ---------------
PRUint32 WMChar2KeyCode( MPARAM mp1, MPARAM mp2)
{
   PRUint32 rc = 0;
   USHORT   flags = SHORT1FROMMP( mp1);

   // First check for characters.
   // This is complicated by keystrokes such as Ctrl+K not having the KC_CHAR
   // bit set, but thankfully they do have the character actually there.
   //
   // So go on the assumption that `if not vkey or deadkey then char'
   //
   if( !(flags & (KC_VIRTUALKEY | KC_DEADKEY)))
   {
      rc = SHORT1FROMMP(mp2);
      // Need nls-correct way of doing this...
      if( isalnum( rc))
         rc = toupper( rc); // no lower case
      else switch( rc)
      {
         case ';':  rc = NS_VK_SEMICOLON;     break;
         case '=':  rc = NS_VK_EQUALS;        break;
         case '*':  rc = NS_VK_MULTIPLY;      break;
         case '+':  rc = NS_VK_ADD;           break;
         case '-':  rc = NS_VK_SUBTRACT;      break;
         case '.':  rc = NS_VK_PERIOD;        break; // NS_VK_DECIMAL ?
         case '|':  rc = NS_VK_SEPARATOR;     break;
         case ',':  rc = NS_VK_COMMA;         break;
         case '/':  rc = NS_VK_SLASH;         break; // NS_VK_DIVIDE ?
         case '`':  rc = NS_VK_BACK_QUOTE;    break;
         case '(':  rc = NS_VK_OPEN_BRACKET;  break;
         case '\\': rc = NS_VK_BACK_SLASH;    break;
         case ')':  rc = NS_VK_CLOSE_BRACKET; break;
         case '\'': rc = NS_VK_QUOTE;         break;
      }
   }
   else if( flags & KC_VIRTUALKEY)
   {
      USHORT vk = SHORT2FROMMP( mp2);

      if( vk >= VK_F1 && vk <= VK_F24)
         rc = NS_VK_F1 + (vk - VK_F1);
      else switch( vk)
      {
         case VK_NUMLOCK:   rc = NS_VK_NUM_LOCK; break;
         case VK_SCRLLOCK:  rc = NS_VK_SCROLL_LOCK; break;
         case VK_ESC:       rc = NS_VK_ESCAPE; break; // NS_VK_CANCEL
         case VK_BACKSPACE: rc = NS_VK_BACK; break;
         case VK_TAB:       rc = NS_VK_TAB; break;
         case VK_BACKTAB:   rc = NS_VK_TAB; break; // layout tests for isShift
         case VK_CLEAR:     rc = NS_VK_CLEAR; break;
         case VK_NEWLINE:   rc = NS_VK_RETURN; break;
         case VK_ENTER:     rc = NS_VK_ENTER; break;
         case VK_SHIFT:     rc = NS_VK_SHIFT; break;
         case VK_CTRL:      rc = NS_VK_CONTROL; break;
         case VK_ALT:       rc = NS_VK_ALT; break;
         case VK_PAUSE:     rc = NS_VK_PAUSE; break;
         case VK_CAPSLOCK:  rc = NS_VK_CAPS_LOCK; break;
         case VK_SPACE:     rc = NS_VK_SPACE; break;
         case VK_PAGEUP:    rc = NS_VK_PAGE_UP; break;
         case VK_PAGEDOWN:  rc = NS_VK_PAGE_DOWN; break;
         case VK_END:       rc = NS_VK_END; break;
         case VK_HOME:      rc = NS_VK_HOME; break;
         case VK_LEFT:      rc = NS_VK_LEFT; break;
         case VK_UP:        rc = NS_VK_UP; break;
         case VK_RIGHT:     rc = NS_VK_RIGHT; break;
         case VK_DOWN:      rc = NS_VK_DOWN; break;
         case VK_PRINTSCRN: rc = NS_VK_PRINTSCREEN; break;
         case VK_INSERT:    rc = NS_VK_INSERT; break;
         case VK_DELETE:    rc = NS_VK_DELETE; break;
      }
   }

   return rc;
}
