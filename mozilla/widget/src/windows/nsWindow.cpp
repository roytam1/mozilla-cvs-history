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

//#define KE_DEBUG

#include "nsWindow.h"
#include "nsIAppShell.h"
#include "nsIFontMetrics.h"
#include "nsFont.h"
#include "nsGUIEvent.h"
#include "nsIRenderingContext.h"
#include "nsIDeviceContext.h"
#include "nsRect.h"
#include "nsTransform2D.h"
#include "nsStringUtil.h"
#include <windows.h>
//#include <winuser.h>
#include <zmouse.h>
//#include "sysmets.h"
#include "nsGfxCIID.h"
#include "resource.h"
#include <commctrl.h>
#include "prtime.h"
#include "nsIRenderingContextWin.h"

#include "nsIMenu.h"
#include "nsMenu.h"
#include "nsIMenuItem.h"
#include "nsIMenuListener.h"
#include "nsMenuItem.h"
#include <imm.h>

#include "nsNativeDragTarget.h"
#include "nsIRollupListener.h"

// we define the following because there are some MS sample code say
// we should do it. We are not sure we really need it.
#define IME_FROM_ON_CHAR

//~~~ windowless plugin support
#include "nsplugindefs.h"

// For clipboard support
#include "nsIServiceManager.h"
#include "nsIClipboard.h"
#include "nsWidgetsCID.h"

static NS_DEFINE_CID(kCClipboardCID,       NS_CLIPBOARD_CID);
static NS_DEFINE_IID(kRenderingContextCID, NS_RENDERING_CONTEXT_CID);


BOOL nsWindow::sIsRegistered = FALSE;


////////////////////////////////////////////////////
static nsIRollupListener * gRollupListener           = nsnull;
static nsIWidget         * gRollupWidget             = nsnull;
static PRBool              gRollupConsumeRollupEvent = PR_FALSE;

nsWindow* nsWindow::gCurrentWindow = nsnull;

#if 0
// #ifdef KE_DEBUG
static PRBool is_vk_down(int vk)
{
   SHORT st = GetKeyState(vk);
   printf("is_vk_down vk=%x st=%x\n",vk, st);
   return (st & 0x80) ? PR_TRUE : PR_FALSE;
}
#define IS_VK_DOWN is_vk_down
#else
#define IS_VK_DOWN(a) (PRBool)(((GetKeyState(a) & 0x80)) ? (PR_TRUE) : (PR_FALSE))
#endif


// Global variable 
//     g_hinst - handle of the application instance 
extern HINSTANCE g_hinst; 

//
// input method offsets
//
#define IME_X_OFFSET	35
#define IME_Y_OFFSET	35


#ifdef IME_FROM_ON_CHAR
static PRBool NS_IsDBCSLeadByte(UINT aCP, BYTE aByte)
{
   switch(aCP) {
       case 932: 
         return (((0x81<=aByte)&&(aByte<=0x9F))||((0xE0<=aByte)&&(aByte<=0xFC)));
       case 936:
       case 949:
       case 950: 
         return ((0x81<=aByte)&&(aByte<=0xFE));
       default: 
         return PR_FALSE;
   };
}
#endif // IME_FROM_ON_CHAR

static PRBool LangIDToCP(WORD aLangID, UINT& oCP)
{
	int localeid=MAKELCID(aLangID,SORT_DEFAULT);
	int numchar=GetLocaleInfo(localeid,LOCALE_IDEFAULTANSICODEPAGE,NULL,0);
        char cp_on_stack[32];

	char* cp_name;
        if(numchar > 32)
           cp_name  = new char[numchar];
        else 
           cp_name = cp_on_stack;
	if (cp_name) {
           GetLocaleInfo(localeid,LOCALE_IDEFAULTANSICODEPAGE,cp_name,numchar);
           oCP = atoi(cp_name);
           if(cp_name != cp_on_stack)
               delete [] cp_name;
           return PR_TRUE;
	} else {
           oCP = CP_ACP;
           return PR_FALSE;
        }
}

//-------------------------------------------------------------------------
//
// nsWindow constructor
//
//-------------------------------------------------------------------------
nsWindow::nsWindow() : nsBaseWidget()
{
    NS_INIT_REFCNT();
    mWnd                = 0;
    mPrevWndProc        = NULL;
    mBackground         = ::GetSysColor(COLOR_BTNFACE);
    mBrush              = ::CreateSolidBrush(NSRGB_2_COLOREF(mBackground));
    mForeground         = ::GetSysColor(COLOR_WINDOWTEXT);
    mPalette            = NULL;
    mIsShiftDown        = PR_FALSE;
    mIsControlDown      = PR_FALSE;
    mIsAltDown          = PR_FALSE;
    mIsDestroying       = PR_FALSE;
    mOnDestroyCalled    = PR_FALSE;
    mDeferredPositioner = NULL;
    mLastPoint.x        = 0;
    mLastPoint.y        = 0;
    mPreferredWidth     = 0;
    mPreferredHeight    = 0;
    mFont               = nsnull;
    mIsVisible          = PR_FALSE;
    mHas3DBorder        = PR_FALSE;
    mMenuBar            = nsnull;
    mMenuCmdId          = 0;
    mWindowType         = eWindowType_child;
    mBorderStyle        = eBorderStyle_default;
    mBorderlessParent   = 0;
   
    mHitMenu            = nsnull;
    mHitSubMenus        = new nsVoidArray();
#ifdef LOSER
    mVScrollbar         = nsnull;
#endif
    mIsInMouseCapture   = PR_FALSE;

	mIMEProperty		= 0;
	mIMEIsComposing		= PR_FALSE;
	mIMECompositionString = NULL;
	mIMECompositionStringSize = 0;
	mIMECompositionStringLength = 0;
	mIMECompositionUniString = NULL;
	mIMECompositionUniStringSize = 0;
	mIMEAttributeString = NULL;
	mIMEAttributeStringSize = 0;
	mIMEAttributeStringLength = 0;
	mIMECompClauseString = NULL;
	mIMECompClauseStringSize = 0;
	mIMECompClauseStringLength = 0;
        WORD kblayout = (WORD)GetKeyboardLayout(0);
        LangIDToCP((WORD)(0x0FFFFL & kblayout), mCurrentKeyboardCP);

#ifdef IME_FROM_ON_CHAR
	mHaveDBCSLeadByte = false;
	mDBCSLeadByte = '\0';
#endif // IME_FROM_ON_CHAR

  mNativeDragTarget = nsnull;
  mIsTopWidgetWindow = PR_FALSE;
}


//-------------------------------------------------------------------------
//
// nsWindow destructor
//
//-------------------------------------------------------------------------
nsWindow::~nsWindow()
{
  mIsDestroying = PR_TRUE;
  if (gCurrentWindow == this) {
    gCurrentWindow = nsnull;
  }

  MouseTrailer * mouseTrailer = MouseTrailer::GetMouseTrailer(0);
  if (mouseTrailer->GetMouseTrailerWindow() == this) {
    mouseTrailer->DestroyTimer();
  } 

  // If the widget was released without calling Destroy() then the native
  // window still exists, and we need to destroy it
  if (NULL != mWnd) {
    Destroy();
  }

  NS_IF_RELEASE(mHitMenu); // this should always have already been freed by the deselect

  delete mHitSubMenus;
  
  //XXX Temporary: Should not be caching the font
  delete mFont;

  //
  // delete any of the IME structures that we allocated
  //
  if (mIMECompositionString!=NULL) delete [] mIMECompositionString;
  if (mIMEAttributeString!=NULL) delete [] mIMEAttributeString;
  if (mIMECompositionUniString!=NULL) delete [] mIMECompositionUniString;

  NS_IF_RELEASE(mNativeDragTarget);
}


NS_METHOD nsWindow::CaptureMouse(PRBool aCapture)
{
  if (PR_TRUE == aCapture) { 
    MouseTrailer::SetCaptureWindow(this);
    ::SetCapture(mWnd);
  } else {
    MouseTrailer::SetCaptureWindow(NULL);
    ::ReleaseCapture();
  }
  mIsInMouseCapture = aCapture;
  return NS_OK;
}


//-------------------------------------------------------------------------
//
// Default for height modification is to do nothing
//
//-------------------------------------------------------------------------

PRInt32 nsWindow::GetHeight(PRInt32 aProposedHeight)
{
  return(aProposedHeight);
}

//-------------------------------------------------------------------------
//
// Deferred Window positioning
//
//-------------------------------------------------------------------------

NS_METHOD nsWindow::BeginResizingChildren(void)
{
  if (NULL == mDeferredPositioner)
    mDeferredPositioner = ::BeginDeferWindowPos(1);
  return NS_OK;
}

NS_METHOD nsWindow::EndResizingChildren(void)
{
  if (NULL != mDeferredPositioner) {
    ::EndDeferWindowPos(mDeferredPositioner);
    mDeferredPositioner = NULL;
  }
  return NS_OK;
}

NS_METHOD nsWindow::WidgetToScreen(const nsRect& aOldRect, nsRect& aNewRect)
{
  POINT point;
  point.x = aOldRect.x;
  point.y = aOldRect.y;
  ::ClientToScreen((HWND)GetNativeData(NS_NATIVE_WINDOW), &point);
  aNewRect.x = point.x;
  aNewRect.y = point.y;
  aNewRect.width = aOldRect.width;
  aNewRect.height = aOldRect.height;
  return NS_OK;
}

NS_METHOD nsWindow::ScreenToWidget(const nsRect& aOldRect, nsRect& aNewRect)
{
  POINT point;
  point.x = aOldRect.x;
  point.y = aOldRect.y;
  ::ScreenToClient((HWND)GetNativeData(NS_NATIVE_WINDOW), &point);
  aNewRect.x = point.x;
  aNewRect.y = point.y;
  aNewRect.width = aOldRect.width;
  aNewRect.height = aOldRect.height;
  return NS_OK;
} 

//-------------------------------------------------------------------------
//
// Convert nsEventStatus value to a windows boolean
//
//-------------------------------------------------------------------------

PRBool nsWindow::ConvertStatus(nsEventStatus aStatus)
{
  switch(aStatus) {
  case nsEventStatus_eIgnore:
    return PR_FALSE;
  case nsEventStatus_eConsumeNoDefault:
    return PR_TRUE;
  case nsEventStatus_eConsumeDoDefault:
    return PR_FALSE;
  default:
    NS_ASSERTION(0, "Illegal nsEventStatus enumeration value");
    break;
  }
  return PR_FALSE;
}

//-------------------------------------------------------------------------
//
// Initialize an event to dispatch
//
//-------------------------------------------------------------------------
void nsWindow::InitEvent(nsGUIEvent& event, PRUint32 aEventType, nsPoint* aPoint)
{
    event.widget = this;
    NS_ADDREF(event.widget);

    if (nsnull == aPoint) {     // use the point from the event
      // get the message position in client coordinates and in twips
      DWORD pos = ::GetMessagePos();
      POINT cpos;

      cpos.x = LOWORD(pos);
      cpos.y = HIWORD(pos);

      if (mWnd != NULL) {
        ::ScreenToClient(mWnd, &cpos);
        event.point.x = cpos.x;
        event.point.y = cpos.y;
      } else {
        event.point.x = 0;
        event.point.y = 0;
      }
    }
    else {                      // use the point override if provided
      event.point.x = aPoint->x;
      event.point.y = aPoint->y;
    }

    event.time = ::GetMessageTime();
    event.message = aEventType;

    mLastPoint.x = event.point.x;
    mLastPoint.y = event.point.y;
}

//-------------------------------------------------------------------------
//
// Invokes callback and  ProcessEvent method on Event Listener object
//
//-------------------------------------------------------------------------

NS_IMETHODIMP nsWindow::DispatchEvent(nsGUIEvent* event, nsEventStatus & aStatus)
{
#ifdef NS_DEBUG
  debug_DumpEvent(stdout,
                  event->widget,
                  event,
                  "something",
                  (PRInt32) mWnd);
#endif // NS_DEBUG

  aStatus = nsEventStatus_eIgnore;
 
  //if (nsnull != mMenuListener)
  //	aStatus = mMenuListener->MenuSelected(*event);
  if (nsnull != mEventCallback) {
    aStatus = (*mEventCallback)(event);
  }

    // Dispatch to event listener if event was not consumed
  if ((aStatus != nsEventStatus_eIgnore) && (nsnull != mEventListener)) {
    aStatus = mEventListener->ProcessEvent(*event);
  }

  // the window can be destroyed during processing of seemingly innocuous events like, say,
  // mousedowns due to the magic of scripting. mousedowns will return nsEventStatus_eIgnore,
  // which causes problems with the deleted window. therefore:
  if (mOnDestroyCalled)
    aStatus = nsEventStatus_eConsumeNoDefault;
  return NS_OK;
}

//-------------------------------------------------------------------------
PRBool nsWindow::DispatchWindowEvent(nsGUIEvent* event)
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

PRBool nsWindow::DispatchStandardEvent(PRUint32 aMsg)
{
  nsGUIEvent event;
  event.eventStructType = NS_GUI_EVENT;
  InitEvent(event, aMsg);

  PRBool result = DispatchWindowEvent(&event);
  NS_RELEASE(event.widget);
  return result;
}

//-------------------------------------------------------------------------
NS_IMETHODIMP nsWindow::CaptureRollupEvents(nsIRollupListener * aListener, 
                                            PRBool aDoCapture, 
                                            PRBool aConsumeRollupEvent)
{
  if (aDoCapture) {
    gRollupConsumeRollupEvent = aConsumeRollupEvent;
    NS_IF_RELEASE(gRollupListener);
    NS_IF_RELEASE(gRollupWidget);
    gRollupListener = aListener;
    NS_ADDREF(aListener);
    gRollupWidget = this;
    NS_ADDREF(this);
  } else {
    NS_IF_RELEASE(gRollupListener);
    //gRollupListener = nsnull;
    NS_IF_RELEASE(gRollupWidget);
  }

  return NS_OK;
}

PRBool 
nsWindow::EventIsInsideWindow(nsWindow* aWindow) 
{
  RECT r;
  ::GetWindowRect(aWindow->mWnd, &r);
  DWORD pos = ::GetMessagePos();
  POINT mp;
  mp.x = LOWORD(pos);
  mp.y = HIWORD(pos);
  // now make sure that it wasn't one of our children
  if (mp.x < r.left || mp.x > r.right ||
      mp.y < r.top || mp.y > r.bottom) {
    return PR_FALSE;
  } 

  return PR_TRUE;
}


PRBool
nsWindow::IsScrollbar(HWND aWnd) {

   // Make sure this is one of our windows by comparing the window procedures
  LONG proc = ::GetWindowLong(aWnd, GWL_WNDPROC);
  if (proc == (LONG)&nsWindow::WindowProc) {
    // It is a one of our windows.
    nsWindow *someWindow = (nsWindow*)::GetWindowLong(aWnd, GWL_USERDATA);
      //This is inefficient, but this method is only called when
      //a popup window has been displayed, and your clicking within it.
      //The default window class begins with Netscape so comparing with the initial
      //S in SCROLLBAR will cause strcmp to immediately return.
    if (strcmp(someWindow->WindowClass(),"SCROLLBAR") == 0) {
      return PR_TRUE;
    }
  } 

  return PR_FALSE;
}

//-------------------------------------------------------------------------
//
// the nsWindow procedure for all nsWindows in this toolkit
//
//-------------------------------------------------------------------------
LRESULT CALLBACK nsWindow::WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
 
     // check to see if we have a rollup listener registered
    if (nsnull != gRollupListener && nsnull != gRollupWidget) {
  
       // All mouse wheel events cause the popup to rollup.
       // XXX: Need something more reliable then WM_MOUSEWHEEL.
       // This event is not always generated. The mouse wheel
       // is plugged into the scrollbar scrolling in odd ways
       // which make it difficult to find a message which will
       // reliably be generated when the mouse wheel changes position
      if (msg == WM_MOUSEWHEEL) {
        gRollupListener->Rollup();
        return TRUE;
      }
      
      if (msg == WM_ACTIVATE || msg == WM_NCLBUTTONDOWN || msg == WM_LBUTTONDOWN ||
        msg == WM_NCMBUTTONDOWN || msg == WM_NCRBUTTONDOWN) {
        // Rollup if the event is outside the popup
        if (PR_FALSE == nsWindow::EventIsInsideWindow((nsWindow*)gRollupWidget)) {
          gRollupListener->Rollup();
        // return TRUE tells Windows that the event is consumed, 
        // false allows the event to be dispacthed
        //
        // So if we are NOT suppose to be consuming events and it is a 
        // non-client Mouse Button down, let it go through
        if (!gRollupConsumeRollupEvent && msg == WM_NCLBUTTONDOWN) {
          return FALSE;
        } else {
          return TRUE;
        }
        } 
      }

      if ((msg == WM_MOUSEACTIVATE) && (nsWindow::EventIsInsideWindow((nsWindow*)gRollupWidget))) {
        // Prevent the click inside the popup from causing a change in window
        // activation. Since the popup and shown non-activated, we need to eat 
        // any requests to activate the window while it is displayed. Windows 
        // will automatically activate the popup on the mousedown otherwise.
        // We have to special case native scrollbars since they need to get the event otherwise
        // they will not scroll.
        if (nsWindow::IsScrollbar(hWnd)) {
          return MA_NOACTIVATE;
        } else {
          return MA_NOACTIVATEANDEAT;   
        }
      }
    }

    // Get the window which caused the event and ask it to process the message
    nsWindow *someWindow = (nsWindow*)::GetWindowLong(hWnd, GWL_USERDATA);

    // hold on to the window for the life of this method, in case it gets
    // deleted during processing. yes, it's a double hack, since someWindow
    // is not really an interface.
    nsCOMPtr<nsISupports> kungFuDeathGrip;
    if (!someWindow->mIsDestroying) // not if we're in the destructor!
      kungFuDeathGrip = do_QueryInterface(someWindow);

    // Re-direct a tab change message destined for its parent window to the
    // the actual window which generated the event.
    if (msg == WM_NOTIFY) {
      LPNMHDR pnmh = (LPNMHDR) lParam;
      if (pnmh->code == TCN_SELCHANGE) {
        someWindow = (nsWindow*)::GetWindowLong(pnmh->hwndFrom, GWL_USERDATA);
      }
    }

    if (nsnull != someWindow) {
        LRESULT retValue;
        if (PR_TRUE == someWindow->ProcessMessage(msg, wParam, lParam, &retValue)) {
            return retValue;
        }
    }

#if defined(STRICT)
    return ::CallWindowProc((WNDPROC)someWindow->GetPrevWindowProc(), hWnd, 
                            msg, wParam, lParam);
#else
    return ::CallWindowProc((FARPROC)someWindow->GetPrevWindowProc(), hWnd, 
                            msg, wParam, lParam);
#endif
}


//WINOLEAPI oleStatus;
//-------------------------------------------------------------------------
//
// Utility method for implementing both Create(nsIWidget ...) and
// Create(nsNativeWidget...)
//-------------------------------------------------------------------------
// This is needed for drag & drop & Clipboard support
BOOL gOLEInited = FALSE;

nsresult nsWindow::StandardWindowCreate(nsIWidget *aParent,
                      const nsRect &aRect,
                      EVENT_CALLBACK aHandleEventFunction,
                      nsIDeviceContext *aContext,
                      nsIAppShell *aAppShell,
                      nsIToolkit *aToolkit,
                      nsWidgetInitData *aInitData,
                      nsNativeWidget aNativeParent)
{
    nsIWidget *baseParent = aInitData &&
                 (aInitData->mWindowType == eWindowType_dialog ||
                  aInitData->mWindowType == eWindowType_toplevel) ?
                  nsnull : aParent;

    mIsTopWidgetWindow = (nsnull == baseParent);

    BaseCreate(baseParent, aRect, aHandleEventFunction, aContext, 
       aAppShell, aToolkit, aInitData);

      // See if the caller wants to explictly set clip children and clip siblings
    DWORD style = WindowStyle();
    if (nsnull != aInitData) {
      if (aInitData->clipChildren) {
        style |= WS_CLIPCHILDREN;
      } else {
        style &= ~WS_CLIPCHILDREN;
      }
      if (aInitData->clipSiblings) {
        style |= WS_CLIPSIBLINGS;
      }
    }

    // Switch to the "main gui thread" if necessary... This method must
    // be executed on the "gui thread"...
    //
  
    nsToolkit* toolkit = (nsToolkit *)mToolkit;
    if (toolkit) {
    if (!toolkit->IsGuiThread()) {
        DWORD args[7];
        args[0] = (DWORD)aParent;
        args[1] = (DWORD)&aRect;
        args[2] = (DWORD)aHandleEventFunction;
        args[3] = (DWORD)aContext;
        args[4] = (DWORD)aAppShell;
        args[5] = (DWORD)aToolkit;
        args[6] = (DWORD)aInitData;

        if (nsnull != aParent) {
           // nsIWidget parent dispatch
          MethodInfo info(this, nsWindow::CREATE, 7, args);
          toolkit->CallMethod(&info);
           return NS_OK;
        }
        else {
            // Native parent dispatch
          MethodInfo info(this, nsWindow::CREATE_NATIVE, 5, args);
          toolkit->CallMethod(&info);
          return NS_OK;
        }
    }
    }
 
    HWND parent;
    if (nsnull != aParent) { // has a nsIWidget parent
      parent = ((aParent) ? (HWND)aParent->GetNativeData(NS_NATIVE_WINDOW) : nsnull);
    } else { // has a nsNative parent
       parent = (HWND)aNativeParent;
    }

    DWORD extendedStyle = WindowExStyle();
    if (nsnull != aInitData) {
      SetWindowType(aInitData->mWindowType);
      SetBorderStyle(aInitData->mBorderStyle);

      if (mWindowType == eWindowType_dialog) {
        extendedStyle &= ~WS_EX_CLIENTEDGE;
      } else if (mWindowType == eWindowType_popup) {
        extendedStyle = WS_EX_TOPMOST;
        style = WS_POPUP;
        mBorderlessParent = parent;
         // Don't set the parent of a popup window. 
        parent = NULL;
         // WS_EX_TOOLWINDOW prevents a button from being placed on 
         // the taskbar for the popup window.
        extendedStyle |= WS_EX_TOOLWINDOW;
      }

      if (aInitData->mBorderStyle == eBorderStyle_default) {
        if (mWindowType == eWindowType_dialog)
          style &= ~(WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);
      } else if (aInitData->mBorderStyle != eBorderStyle_all) {
        if (aInitData->mBorderStyle == eBorderStyle_none ||
            !(aInitData->mBorderStyle & eBorderStyle_border))
          style &= ~WS_BORDER;

        if (aInitData->mBorderStyle == eBorderStyle_none ||
            !(aInitData->mBorderStyle & eBorderStyle_title)) {
          style &= ~WS_DLGFRAME;
          style |= WS_POPUP;
        }
        if (aInitData->mBorderStyle == eBorderStyle_none ||
            !(aInitData->mBorderStyle & (eBorderStyle_close | eBorderStyle_menu)))
          style &= ~WS_SYSMENU;

        if (aInitData->mBorderStyle == eBorderStyle_none ||
            !(aInitData->mBorderStyle & eBorderStyle_resizeh))
          style &= ~WS_THICKFRAME;

        if (aInitData->mBorderStyle == eBorderStyle_none ||
            !(aInitData->mBorderStyle & eBorderStyle_minimize))
          style &= ~WS_MINIMIZEBOX;

        if (aInitData->mBorderStyle == eBorderStyle_none ||
            !(aInitData->mBorderStyle & eBorderStyle_maximize))
          style &= ~WS_MAXIMIZEBOX;
      }
    }

    mHas3DBorder = (extendedStyle & WS_EX_CLIENTEDGE) > 0;

    mWnd = ::CreateWindowEx(extendedStyle,
                            WindowClass(),
                            "",
                            style,
                            aRect.x,
                            aRect.y,
                            aRect.width,
                            GetHeight(aRect.height),
                            parent,
                            NULL,
                            nsToolkit::mDllInstance,
                            NULL);
   
    VERIFY(mWnd);

    // Initial Drag & Drop Work
   if (!gOLEInited) {
     DWORD dwVer = ::OleBuildVersion();

     if (FAILED(::OleInitialize(NULL))){
       printf("***** OLE has not been initialized!\n");
     } else {
       //if defined(DEBUG) printf("***** OLE has been initialized!\n");
     }
     gOLEInited = TRUE;
   }

   /*mNativeDragTarget = new nsNativeDragTarget(this);
   if (NULL != mNativeDragTarget) {
     mNativeDragTarget->AddRef();
     if (S_OK == ::CoLockObjectExternal((LPUNKNOWN)mNativeDragTarget,TRUE,FALSE)) {
       if (S_OK == ::RegisterDragDrop(mWnd, (LPDROPTARGET)mNativeDragTarget)) {
       }
     }
   }*/


    // call the event callback to notify about creation

    DispatchStandardEvent(NS_CREATE);
    SubclassWindow(TRUE);
    return(NS_OK);
}

//-------------------------------------------------------------------------
//
// Create the proper widget
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
  // Switch to the "main gui thread" if necessary... This method must
  // be executed on the "gui thread"...
  nsToolkit* toolkit = (nsToolkit *)mToolkit;
  if (toolkit != nsnull && !toolkit->IsGuiThread()) {
    MethodInfo info(this, nsWindow::DESTROY);
    toolkit->CallMethod(&info);
    return NS_ERROR_FAILURE;
  }

  // disconnect from the parent
  if (!mIsDestroying) {
    nsBaseWidget::Destroy();
  }

  EnableDragDrop(PR_FALSE);

  // destroy the HWND
  if (mWnd) {
    // prevent the widget from causing additional events
    mEventCallback = nsnull;
    VERIFY(::DestroyWindow(mWnd));
    mWnd = NULL;
    //our windows can be subclassed by
    //others and these namless, faceless others
    //may not let us know about WM_DESTROY. so,
    //if OnDestroy() didn't get called, just call
    //it now. MMP
    if (PR_FALSE == mOnDestroyCalled)
      OnDestroy();
  }

  return NS_OK;
}


//-------------------------------------------------------------------------
//
// Get this nsWindow parent
//
//-------------------------------------------------------------------------
nsIWidget* nsWindow::GetParent(void)
{
    if (mIsTopWidgetWindow) {
       // Must use a flag instead of mWindowType to tell if the window is the 
       // owned by the topmost widget, because a child window can be embedded inside
       // a HWND which is not associated with a nsIWidget.
      return nsnull;
    }


    nsWindow* widget = nsnull;
    if (mWnd) {
        HWND parent = ::GetParent(mWnd);
        if (parent) {
            widget = (nsWindow *)::GetWindowLong(parent, GWL_USERDATA);
            if (widget) {
              // If the widget is in the process of being destroyed then
              // do NOT return it
              if (widget->mIsDestroying) {
                widget = nsnull;
              } else {
                NS_ADDREF(widget);
              }
            }
        }
    }

    return (nsIWidget*)widget;
}


//-------------------------------------------------------------------------
//
// Hide or show this component
//
//-------------------------------------------------------------------------
NS_METHOD nsWindow::Show(PRBool bState)
{
    if (mWnd) {
        if (bState) {
          DWORD flags = SWP_NOSIZE | SWP_NOMOVE | SWP_SHOWWINDOW;
          if ((mIsVisible) || (mWindowType == eWindowType_popup)) {
             flags |= SWP_NOZORDER;
          }

          if (mWindowType == eWindowType_popup) {
            flags |= SWP_NOACTIVATE;
            ::SetWindowPos(mWnd, HWND_TOPMOST, 0, 0, 0, 0, flags);
          } else {
            ::SetWindowPos(mWnd, HWND_TOP, 0, 0, 0, 0, flags);
          }
        }
        else
            ::ShowWindow(mWnd, SW_HIDE);
    }
    mIsVisible = bState;
    return NS_OK;
}

//-------------------------------------------------------------------------
//
// Return PR_TRUE if the whether the component is visible, PR_FALSE otherwise
//
//-------------------------------------------------------------------------
NS_METHOD nsWindow::IsVisible(PRBool & bState)
{
  bState = mIsVisible;
  return NS_OK;
}

//-------------------------------------------------------------------------
//
// Move this component
//
//-------------------------------------------------------------------------
NS_METHOD nsWindow::Move(PRInt32 aX, PRInt32 aY)
{
   // When moving a borderless top-level window the window
   // must be placed relative to its parent. WIN32 wants to
   // place it relative to the screen, so we used the cached parent
   // to calculate the parent's location then add the x,y passed to
   // the move to get the screen coordinate for the borderless top-level
   // window.
  if (mWindowType == eWindowType_popup) {
    HWND parent = mBorderlessParent;
    if (parent) { 
      RECT pr; 
      VERIFY(::GetWindowRect(parent, &pr)); 
      aX += pr.left; 
      aY += pr.top;   
    } 
  } 

  mBounds.x = aX;
  mBounds.y = aY;

    if (mWnd) {
        nsIWidget *par = GetParent();
        HDWP      deferrer = NULL;

        if (nsnull != par) {
          deferrer = ((nsWindow *)par)->mDeferredPositioner;
        }

        if (NULL != deferrer) {
            VERIFY(((nsWindow *)par)->mDeferredPositioner = ::DeferWindowPos(deferrer,
                                  mWnd, NULL, aX, aY, 0, 0,
                                  SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOSIZE));
        }
        else {
            VERIFY(::SetWindowPos(mWnd, NULL, aX, aY, 0, 0, 
                                  SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOSIZE));
        }

        NS_IF_RELEASE(par);
    }
    return NS_OK;
}

//-------------------------------------------------------------------------
//
// Resize this component
//
//-------------------------------------------------------------------------
NS_METHOD nsWindow::Resize(PRInt32 aWidth, PRInt32 aHeight, PRBool aRepaint)
{
  NS_ASSERTION((aWidth >=0 ) , "Negative width passed to nsWindow::Resize");
  NS_ASSERTION((aHeight >=0 ), "Negative height passed to nsWindow::Resize");
  // Set cached value for lightweight and printing
  mBounds.width  = aWidth;
  mBounds.height = aHeight;

    if (mWnd) {
        nsIWidget *par = GetParent();
        HDWP      deferrer = NULL;

        if (nsnull != par) {
          deferrer = ((nsWindow *)par)->mDeferredPositioner;
        }

        UINT  flags = SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE;
        if (!aRepaint) {
          flags |= SWP_NOREDRAW;
        }

        if (NULL != deferrer) {
            VERIFY(((nsWindow *)par)->mDeferredPositioner = ::DeferWindowPos(deferrer,
                                    mWnd, NULL, 0, 0, aWidth, GetHeight(aHeight), flags));
        }
        else {
            VERIFY(::SetWindowPos(mWnd, NULL, 0, 0, aWidth, GetHeight(aHeight), 
                                  flags));
        }

        NS_IF_RELEASE(par);
    }
    return NS_OK;
}

    
//-------------------------------------------------------------------------
//
// Resize this component
//
//-------------------------------------------------------------------------
NS_METHOD nsWindow::Resize(PRInt32 aX,
                      PRInt32 aY,
                      PRInt32 aWidth,
                      PRInt32 aHeight,
                      PRBool   aRepaint)
{
  NS_ASSERTION((aWidth >=0 ),  "Negative width passed to nsWindow::Resize");
  NS_ASSERTION((aHeight >=0 ), "Negative height passed to nsWindow::Resize");

  // Set cached value for lightweight and printing
  mBounds.x      = aX;
  mBounds.y      = aY;
  mBounds.width  = aWidth;
  mBounds.height = aHeight;

    if (mWnd) {
        nsIWidget *par = GetParent();
        HDWP      deferrer = NULL;

        if (nsnull != par) {
          deferrer = ((nsWindow *)par)->mDeferredPositioner;
        }

        UINT  flags = SWP_NOZORDER | SWP_NOACTIVATE;
        if (!aRepaint) {
          flags |= SWP_NOREDRAW;
        }

        if (NULL != deferrer) {
            VERIFY(((nsWindow *)par)->mDeferredPositioner = ::DeferWindowPos(deferrer,
                                    mWnd, NULL, aX, aY, aWidth, GetHeight(aHeight), 
                                    flags));
        }
        else {
            VERIFY(::SetWindowPos(mWnd, NULL, aX, aY, aWidth, GetHeight(aHeight), 
                                  flags));
        }

        NS_IF_RELEASE(par);
    }
    return NS_OK;
}

    
//-------------------------------------------------------------------------
//
// Enable/disable this component
//
//-------------------------------------------------------------------------
NS_METHOD nsWindow::Enable(PRBool bState)
{
    if (mWnd) {
        ::EnableWindow(mWnd, bState);
    }
    return NS_OK;
}

    
//-------------------------------------------------------------------------
//
// Give the focus to this component
//
//-------------------------------------------------------------------------
NS_METHOD nsWindow::SetFocus(void)
{
    //
    // Switch to the "main gui thread" if necessary... This method must
    // be executed on the "gui thread"...
    //
    nsToolkit* toolkit = (nsToolkit *)mToolkit;
    if (!toolkit->IsGuiThread()) {
        MethodInfo info(this, nsWindow::SET_FOCUS);
        toolkit->CallMethod(&info);
        return NS_ERROR_FAILURE;
    }

    if (mWnd) {
        ::SetFocus(mWnd);
    }
    return NS_OK;
}

    
//-------------------------------------------------------------------------
//
// Get this component dimension
//
//-------------------------------------------------------------------------
NS_METHOD nsWindow::GetBounds(nsRect &aRect)
{
  if (mWnd) {
    RECT r;
    VERIFY(::GetWindowRect(mWnd, &r));

    // assign size
    aRect.width  = r.right - r.left;
    aRect.height = r.bottom - r.top;

    // convert coordinates if parent exists
    HWND parent = ::GetParent(mWnd);
    if (parent) {
      RECT pr;
      VERIFY(::GetWindowRect(parent, &pr));
      r.left -= pr.left;
      r.top  -= pr.top;
    }
    aRect.x = r.left;
    aRect.y = r.top;
  } else {
    aRect = mBounds;
  }

  return NS_OK;
}

//-------------------------------------------------------------------------
//
// Get this component dimension
//
//-------------------------------------------------------------------------
NS_METHOD nsWindow::GetClientBounds(nsRect &aRect)
{

  if (mWnd) {
    RECT r;
    VERIFY(::GetClientRect(mWnd, &r));

    // assign size
    aRect.x = 0;
    aRect.y = 0;
    aRect.width  = r.right - r.left;
    aRect.height = r.bottom - r.top;

  } else {
    aRect.SetRect(0,0,0,0);
  }
  return NS_OK;
}

//get the bounds, but don't take into account the client size

void nsWindow::GetNonClientBounds(nsRect &aRect)
{
  if (mWnd) {
      RECT r;
      VERIFY(::GetWindowRect(mWnd, &r));

      // assign size
      aRect.width = r.right - r.left;
      aRect.height = r.bottom - r.top;

      // convert coordinates if parent exists
      HWND parent = ::GetParent(mWnd);
      if (parent) {
        RECT pr;
        VERIFY(::GetWindowRect(parent, &pr));
        r.left -= pr.left;
        r.top -= pr.top;
      }
      aRect.x = r.left;
      aRect.y = r.top;
  } else {
      aRect.SetRect(0,0,0,0);
  }
}

           
//-------------------------------------------------------------------------
//
// Set the background color
//
//-------------------------------------------------------------------------
NS_METHOD nsWindow::SetBackgroundColor(const nscolor &aColor)
{
    nsBaseWidget::SetBackgroundColor(aColor);
  
    if (mBrush)
      ::DeleteObject(mBrush);

    mBrush = ::CreateSolidBrush(NSRGB_2_COLOREF(mBackground));
    if (mWnd != NULL) {
      SetClassLong(mWnd, GCL_HBRBACKGROUND, (LONG)mBrush);
    }
    return NS_OK;
}

    
//-------------------------------------------------------------------------
//
// Get this component font
//
//-------------------------------------------------------------------------
nsIFontMetrics* nsWindow::GetFont(void)
{
    NS_NOTYETIMPLEMENTED("GetFont not yet implemented"); // to be implemented
    return NULL;
}

    
//-------------------------------------------------------------------------
//
// Set this component font
//
//-------------------------------------------------------------------------
NS_METHOD nsWindow::SetFont(const nsFont &aFont)
{
  // Cache Font for owner draw
  if (mFont == nsnull) {
    mFont = new nsFont(aFont);
  } else {
    *mFont  = aFont;
  }
  
  // Bail out if there is no context
  if (nsnull == mContext) {
    return NS_ERROR_FAILURE;
  }

  nsIFontMetrics* metrics;
  mContext->GetMetricsFor(aFont, metrics);
  nsFontHandle  fontHandle;
  metrics->GetFontHandle(fontHandle);
  HFONT hfont = (HFONT)fontHandle;

    // Draw in the new font
  ::SendMessage(mWnd, WM_SETFONT, (WPARAM)hfont, (LPARAM)0); 
  NS_RELEASE(metrics);

  return NS_OK;
}

        
//-------------------------------------------------------------------------
//
// Set this component cursor
//
//-------------------------------------------------------------------------

NS_METHOD nsWindow::SetCursor(nsCursor aCursor)
{
 
  // Only change cursor if it's changing

  //XXX mCursor isn't always right.  Scrollbars and others change it, too.
  //XXX If we want this optimization we need a better way to do it.
  //if (aCursor != mCursor) {
    HCURSOR newCursor = NULL;

    switch(aCursor) {
    case eCursor_select:
      newCursor = ::LoadCursor(NULL, IDC_IBEAM);
      break;
      
    case eCursor_wait:
      newCursor = ::LoadCursor(NULL, IDC_WAIT);
      break;

    case eCursor_hyperlink: {
      newCursor = ::LoadCursor(nsToolkit::mDllInstance, MAKEINTRESOURCE(IDC_SELECTANCHOR));
      break;
    }

    case eCursor_standard:
      newCursor = ::LoadCursor(NULL, IDC_ARROW);
      break;

    case eCursor_sizeWE:
      newCursor = ::LoadCursor(NULL, IDC_SIZEWE);
      break;

    case eCursor_sizeNS:
      newCursor = ::LoadCursor(NULL, IDC_SIZENS);
      break;

    case eCursor_arrow_north:
      newCursor = ::LoadCursor(nsToolkit::mDllInstance, MAKEINTRESOURCE(IDC_ARROWNORTH));
      break;

    case eCursor_arrow_north_plus:
      newCursor = ::LoadCursor(nsToolkit::mDllInstance, MAKEINTRESOURCE(IDC_ARROWNORTHPLUS));
      break;

    case eCursor_arrow_south:
      newCursor = ::LoadCursor(nsToolkit::mDllInstance, MAKEINTRESOURCE(IDC_ARROWSOUTH));
      break;

    case eCursor_arrow_south_plus:
      newCursor = ::LoadCursor(nsToolkit::mDllInstance, MAKEINTRESOURCE(IDC_ARROWSOUTHPLUS));
      break;

    case eCursor_arrow_east:
      newCursor = ::LoadCursor(nsToolkit::mDllInstance, MAKEINTRESOURCE(IDC_ARROWEAST));
      break;

    case eCursor_arrow_east_plus:
      newCursor = ::LoadCursor(nsToolkit::mDllInstance, MAKEINTRESOURCE(IDC_ARROWEASTPLUS));
      break;

    case eCursor_arrow_west:
      newCursor = ::LoadCursor(nsToolkit::mDllInstance, MAKEINTRESOURCE(IDC_ARROWWEST));
      break;

    case eCursor_arrow_west_plus:
      newCursor = ::LoadCursor(nsToolkit::mDllInstance, MAKEINTRESOURCE(IDC_ARROWWESTPLUS));
      break;

    default:
      NS_ASSERTION(0, "Invalid cursor type");
      break;
    }

    if (NULL != newCursor) {
      mCursor = aCursor;
      HCURSOR oldCursor = ::SetCursor(newCursor);
    }
  //}
  return NS_OK;
}
    
//-------------------------------------------------------------------------
//
// Invalidate this component visible area
//
//-------------------------------------------------------------------------
NS_METHOD nsWindow::Invalidate(PRBool aIsSynchronous)
{
    if (mWnd) 
    {
#ifdef NS_DEBUG
      debug_DumpInvalidate(stdout,
                           this,
                           nsnull,
                           aIsSynchronous,
                           "noname",
                           (PRInt32) mWnd);
#endif // NS_DEBUG
      
      VERIFY(::InvalidateRect(mWnd, NULL, TRUE));
      if (aIsSynchronous) {
          VERIFY(::UpdateWindow(mWnd));
      }
    }

    return NS_OK;
}

//-------------------------------------------------------------------------
//
// Invalidate this component visible area
//
//-------------------------------------------------------------------------
NS_METHOD nsWindow::Invalidate(const nsRect & aRect, PRBool aIsSynchronous)
{
  RECT rect;

  if (mWnd) 
  {
    rect.left   = aRect.x;
    rect.top    = aRect.y;
    rect.right  = aRect.x + aRect.width;
    rect.bottom = aRect.y  + aRect.height;

#ifdef NS_DEBUG
    debug_DumpInvalidate(stdout,
                         this,
                         &aRect,
                         aIsSynchronous,
                         "noname",
                         (PRInt32) mWnd);
#endif // NS_DEBUG

    VERIFY(::InvalidateRect(mWnd, &rect, TRUE));
    if (aIsSynchronous) {
      VERIFY(::UpdateWindow(mWnd));
    }
  }
  return NS_OK;
}

//-------------------------------------------------------------------------
//
// Force a synchronous repaint of the window
//
//-------------------------------------------------------------------------
NS_IMETHODIMP nsWindow::Update()
{
  // updates can come through for windows no longer holding an mWnd during
  // deletes triggered by JavaScript in buttons with mouse feedback
  if (mWnd)
    VERIFY(::UpdateWindow(mWnd));
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
        case NS_NATIVE_WIDGET:
        case NS_NATIVE_WINDOW:
        case NS_NATIVE_PLUGIN_PORT:
            return (void*)mWnd;
        case NS_NATIVE_GRAPHIC:
            // XXX:  This is sleezy!!  Remember to Release the DC after using it!
            return (void*)::GetDC(mWnd);
        case NS_NATIVE_COLORMAP:
        default:
            break;
    }

    return NULL;
}

//~~~
void nsWindow::FreeNativeData(void * data, PRUint32 aDataType)
{
  switch(aDataType) 
  {
    case NS_NATIVE_GRAPHIC:
    ::ReleaseDC(mWnd, (HDC)data);
    case NS_NATIVE_WIDGET:
    case NS_NATIVE_WINDOW:
    case NS_NATIVE_PLUGIN_PORT:
    case NS_NATIVE_COLORMAP:
      break;
    default:
      break;
  }
}

//-------------------------------------------------------------------------
//
// Set the colormap of the window
//
//-------------------------------------------------------------------------
NS_METHOD nsWindow::SetColorMap(nsColorMap *aColorMap)
{
    if (mPalette != NULL) {
        ::DeleteObject(mPalette);
    }

    PRUint8 *map = aColorMap->Index;
    LPLOGPALETTE pLogPal = (LPLOGPALETTE) new char[2 * sizeof(WORD) +
                                              aColorMap->NumColors * sizeof(PALETTEENTRY)];
	pLogPal->palVersion = 0x300;
	pLogPal->palNumEntries = aColorMap->NumColors;
	for(int i = 0; i < aColorMap->NumColors; i++) 
    {
		pLogPal->palPalEntry[i].peRed = *map++;
		pLogPal->palPalEntry[i].peGreen = *map++;
		pLogPal->palPalEntry[i].peBlue = *map++;
		pLogPal->palPalEntry[i].peFlags = 0;
	}
	mPalette = ::CreatePalette(pLogPal);
	delete pLogPal;

    NS_ASSERTION(mPalette != NULL, "Null palette");
    if (mPalette != NULL) {
        HDC hDC = ::GetDC(mWnd);
        HPALETTE hOldPalette = ::SelectPalette(hDC, mPalette, TRUE);
        ::RealizePalette(hDC);
        ::SelectPalette(hDC, hOldPalette, TRUE);
        ::ReleaseDC(mWnd, hDC);
    }
    return NS_OK;
}


//-------------------------------------------------------------------------
//
// Scroll the bits of a window
//
//-------------------------------------------------------------------------
NS_METHOD nsWindow::Scroll(PRInt32 aDx, PRInt32 aDy, nsRect *aClipRect)
{
  RECT  trect;

  if (nsnull != aClipRect)
  {
    trect.left = aClipRect->x;
    trect.top = aClipRect->y;
    trect.right = aClipRect->XMost();
    trect.bottom = aClipRect->YMost();
  }

  ::ScrollWindowEx(mWnd, aDx, aDy, (nsnull != aClipRect) ? &trect : NULL, NULL,
                   NULL, NULL, SW_INVALIDATE | SW_SCROLLCHILDREN);
  ::UpdateWindow(mWnd);
  return NS_OK;
}


//-------------------------------------------------------------------------
//
// Every function that needs a thread switch goes through this function
// by calling SendMessage (..WM_CALLMETHOD..) in nsToolkit::CallMethod.
//
//-------------------------------------------------------------------------
BOOL nsWindow::CallMethod(MethodInfo *info)
{
    BOOL bRet = TRUE;

    switch (info->methodId) {
        case nsWindow::CREATE:
            NS_ASSERTION(info->nArgs == 7, "Wrong number of arguments to CallMethod");
            Create((nsIWidget*)(info->args[0]), 
                        (nsRect&)*(nsRect*)(info->args[1]), 
                        (EVENT_CALLBACK)(info->args[2]), 
                        (nsIDeviceContext*)(info->args[3]),
                        (nsIAppShell *)(info->args[4]),
                        (nsIToolkit*)(info->args[5]),
                        (nsWidgetInitData*)(info->args[6]));
            break;

        case nsWindow::CREATE_NATIVE:
            NS_ASSERTION(info->nArgs == 7, "Wrong number of arguments to CallMethod");
            Create((nsNativeWidget)(info->args[0]), 
                        (nsRect&)*(nsRect*)(info->args[1]), 
                        (EVENT_CALLBACK)(info->args[2]), 
                        (nsIDeviceContext*)(info->args[3]),
                        (nsIAppShell *)(info->args[4]),
                        (nsIToolkit*)(info->args[5]),
                        (nsWidgetInitData*)(info->args[6]));
            return TRUE;

        case nsWindow::DESTROY:
            NS_ASSERTION(info->nArgs == 0, "Wrong number of arguments to CallMethod");
            Destroy();
            break;

        case nsWindow::SET_FOCUS:
            NS_ASSERTION(info->nArgs == 0, "Wrong number of arguments to CallMethod");
            SetFocus();
            break;

        default:
            bRet = FALSE;
            break;
    }

    return bRet;
}

//-------------------------------------------------------------------------
void nsWindow::SetUpForPaint(HDC aHDC) 
{
  ::SetBkColor (aHDC, NSRGB_2_COLOREF(mBackground));
  ::SetTextColor(aHDC, NSRGB_2_COLOREF(mForeground));
  ::SetBkMode (aHDC, TRANSPARENT);
}

//-------------------------------------------------------------------------
nsIMenuItem * nsWindow::FindMenuItem(nsIMenu * aMenu, PRUint32 aId)
{
  PRUint32 i, count;
  aMenu->GetItemCount(count);
  for (i=0;i<count;i++) {
    nsISupports * item;
    nsIMenuItem * menuItem;
    nsIMenu     * menu;

    aMenu->GetItemAt(i, item);
    if (NS_OK == item->QueryInterface(nsCOMTypeInfo<nsIMenuItem>::GetIID(), (void **)&menuItem)) {
      if (((nsMenuItem *)menuItem)->GetCmdId() == (PRInt32)aId) {
        NS_RELEASE(item);
        return menuItem;
      }
    } else if (NS_OK == item->QueryInterface(nsCOMTypeInfo<nsIMenu>::GetIID(), (void **)&menu)) {
      nsIMenuItem * fndItem = FindMenuItem(menu, aId);
      NS_RELEASE(menu);
      if (nsnull != fndItem) {
        NS_RELEASE(item);
        return fndItem;
      }
    }
    NS_RELEASE(item);
  }
  return nsnull;
}

//-------------------------------------------------------------------------
static nsIMenuItem * FindMenuChild(nsIMenu * aMenu, PRInt32 aId)
{
  PRUint32 i, count;
  aMenu->GetItemCount(count);
  for (i=0;i<count;i++) {
    nsISupports * item;
    aMenu->GetItemAt(i, item);
    nsIMenuItem * menuItem;
    if (NS_OK == item->QueryInterface(nsCOMTypeInfo<nsIMenuItem>::GetIID(), (void **)&menuItem)) {
      if (((nsMenuItem *)menuItem)->GetCmdId() == (PRInt32)aId) {
        NS_RELEASE(item);
        return menuItem;
      }
    }
    NS_RELEASE(item);
  }
  return nsnull;
}


//-------------------------------------------------------------------------
nsIMenu * nsWindow::FindMenu(nsIMenu * aMenu, HMENU aNativeMenu, PRInt32 &aDepth)
{
  if (aNativeMenu == ((nsMenu *)aMenu)->GetNativeMenu()) {
    NS_ADDREF(aMenu);
    return aMenu;
  }

  //aDepth++;
  PRUint32 i, count;
  aMenu->GetItemCount(count);
  for (i=0;i<count;i++) {
    nsISupports * item;
    aMenu->GetItemAt(i, item);
    nsIMenu * menu;
    if (NS_OK == item->QueryInterface(nsCOMTypeInfo<nsIMenu>::GetIID(), (void **)&menu)) {
      HMENU nativeMenu = ((nsMenu *)menu)->GetNativeMenu();
      if (nativeMenu == aNativeMenu) {
		aDepth++;
        return menu;
      } else {
        nsIMenu * fndMenu = FindMenu(menu, aNativeMenu, aDepth);
        if (fndMenu) {
          NS_RELEASE(item);
          NS_RELEASE(menu);
		  aDepth++;
          return fndMenu;
        }
      }
      NS_RELEASE(menu);
    }
    NS_RELEASE(item);
  }
  return nsnull;
}

//-------------------------------------------------------------------------
static void AdjustMenus(nsIMenu * aCurrentMenu, nsIMenu * aNewMenu, nsMenuEvent & aEvent) 
{
  if (nsnull != aCurrentMenu) {
    nsIMenuListener * listener;
    if (NS_OK == aCurrentMenu->QueryInterface(nsCOMTypeInfo<nsIMenuListener>::GetIID(), (void **)&listener)) {
      //listener->MenuDeselected(aEvent);
      NS_RELEASE(listener);
    }
  }
  if (nsnull != aNewMenu)  {
    nsIMenuListener * listener;
    if (NS_OK == aNewMenu->QueryInterface(nsCOMTypeInfo<nsIMenuListener>::GetIID(), (void **)&listener)) {
		NS_ASSERTION(false, "get debugger");
      //listener->MenuSelected(aEvent);
      NS_RELEASE(listener);
    }
  }
}



//-------------------------------------------------------------------------

nsresult nsWindow::MenuHasBeenSelected(
  HMENU aNativeMenu, 
  UINT  aItemNum, 
  UINT  aFlags, 
  UINT  aCommand)
{

  // Build nsMenuEvent
  nsMenuEvent event;
  event.mCommand = aCommand;
  event.eventStructType = NS_MENU_EVENT;
  InitEvent(event, NS_MENU_SELECTED);

  // The MF_POPUP flag tells us if we are a menu item or a menu
  // the aItemNum is either the command ID of the menu item or 
  // the position of the menu as a child of its parent

  PRBool isMenuItem = !(aFlags & MF_POPUP);
  if(isMenuItem) {
    //printf("WM_MENUSELECT for menu item\n"); 
    //NS_RELEASE(event.widget);
    //return NS_OK;
  }
  else
  {
	  //printf("WM_MENUSELECT for menu\n"); 
  }

  // uItem is the position of the item that was clicked
  // aNativeMenu is a handle to the menu that was clicked

  // if aNativeMenu is NULL then the menu is being deselected
  if (!aNativeMenu) {
	  //printf("... for deselect\n");
    //printf("///////////// Menu is NULL!\n");
    // check to make sure something had been selected
    //AdjustMenus(mHitMenu, nsnull, event);
	nsIMenu * aNewMenu = nsnull;
	nsMenuEvent aEvent = event;
    //static void AdjustMenus(nsIMenu * aCurrentMenu, nsIMenu * aNewMenu, nsMenuEvent & aEvent) 
	{
      if (nsnull != mHitMenu) {
        nsIMenuListener * listener;
        if (NS_OK == mHitMenu->QueryInterface(nsCOMTypeInfo<nsIMenuListener>::GetIID(), (void **)&listener)) {
          listener->MenuDeselected(aEvent);
          NS_RELEASE(listener);
		}
	  }
      if (nsnull != aNewMenu)  {
        nsIMenuListener * listener;
        if (NS_OK == aNewMenu->QueryInterface(nsCOMTypeInfo<nsIMenuListener>::GetIID(), (void **)&listener)) {
          listener->MenuSelected(aEvent);
          NS_RELEASE(listener);
		}
	  }
	}

    NS_IF_RELEASE(mHitMenu);
    // Clear All SubMenu items
    while (mHitSubMenus->Count() > 0) {
      PRUint32 inx = mHitSubMenus->Count()-1;
      nsIMenu * menu = (nsIMenu *)mHitSubMenus->ElementAt(inx);
      //AdjustMenus(menu, nsnull, event);
	  nsIMenu * aCurrentMenu = menu;
	  nsIMenu * aNewMenu = nsnull;
      //static void AdjustMenus(nsIMenu * aCurrentMenu, nsIMenu * aNewMenu, nsMenuEvent & aEvent) 
	  {
        if (nsnull != aCurrentMenu) {
          nsIMenuListener * listener;
          if (NS_OK == aCurrentMenu->QueryInterface(nsCOMTypeInfo<nsIMenuListener>::GetIID(), (void **)&listener)) {
            listener->MenuDeselected(event);
            NS_RELEASE(listener);
		  }
		}
        if (nsnull != aNewMenu)  {
          nsIMenuListener * listener;
          if (NS_OK == aNewMenu->QueryInterface(nsCOMTypeInfo<nsIMenuListener>::GetIID(), (void **)&listener)) {
            listener->MenuSelected(event);
            NS_RELEASE(listener);
		  }
		}
	  }

      NS_RELEASE(menu);
      mHitSubMenus->RemoveElementAt(inx);
    }
    NS_RELEASE(event.widget);
    return NS_OK;
  } else { // The menu is being selected
    //printf("... for selection\n");
	  void * voidData;
    mMenuBar->GetNativeData(voidData);
    HMENU nativeMenuBar = (HMENU)voidData;

    // first check to see if it is a member of the menubar
    nsIMenu * hitMenu = nsnull;
    if (aNativeMenu == nativeMenuBar) {
      mMenuBar->GetMenuAt(aItemNum, hitMenu);
      if (mHitMenu != hitMenu) {
	    //mHitMenu, hitMenu, event
		nsMenuEvent aEvent = event;
        //AdjustMenus(nsIMenu * aCurrentMenu, nsIMenu * aNewMenu, nsMenuEvent & aEvent) 
		{
          if (nsnull != mHitMenu) {
            nsIMenuListener * listener;
            if (NS_OK == mHitMenu->QueryInterface(nsCOMTypeInfo<nsIMenuListener>::GetIID(), (void **)&listener)) {
              listener->MenuDeselected(aEvent);
              NS_RELEASE(listener);
			}
		  }
          if (nsnull != hitMenu)  {
            nsIMenuListener * listener;
            if (NS_OK == hitMenu->QueryInterface(nsCOMTypeInfo<nsIMenuListener>::GetIID(), (void **)&listener)) {
              listener->MenuSelected(aEvent);
              NS_RELEASE(listener);
			}
		  }
		}
        NS_IF_RELEASE(mHitMenu);
        mHitMenu = hitMenu;
      } else {
        NS_IF_RELEASE(hitMenu);
      }
    } else {
      // At this point we know we are inside a menu

      // Find the menu we are in (the parent menu)
      nsIMenu * parentMenu = nsnull;
      PRInt32 fndDepth = 0;
      PRUint32 i, count;
      mMenuBar->GetMenuCount(count);
      for (i=0;i<count;i++) {
        nsIMenu * menu;
        mMenuBar->GetMenuAt(i, menu);
        PRInt32 depth = 0;
        parentMenu = FindMenu(menu, aNativeMenu, depth);
        if (parentMenu) {
          fndDepth = depth;
          break;
        }
        NS_RELEASE(menu);
      }

      if (nsnull != parentMenu) {

        // Sometimes an event comes through for a menu that is being popup down
        // So it its depth is great then the current hit list count it already gone.
        if (fndDepth > mHitSubMenus->Count()) {
          NS_RELEASE(parentMenu);
          NS_RELEASE(event.widget);
          return NS_OK;
        }

        nsIMenu * newMenu  = nsnull;

        // Skip if it is a menu item, otherwise, we get the menu by position
        if (!isMenuItem) {
          //printf("Getting submenu by position %d from parentMenu\n", aItemNum);
          nsISupports * item;
          parentMenu->GetItemAt((PRUint32)aItemNum, item);
          if (NS_OK != item->QueryInterface(nsCOMTypeInfo<nsIMenu>::GetIID(), (void **)&newMenu)) {
            //printf("Item was not a menu! What are we doing here? Return early....\n");
            NS_RELEASE(event.widget);
            return NS_ERROR_FAILURE;
          }
        }

        // Figure out if this new menu is in the list of popup'ed menus
        PRBool newFound = PR_FALSE;
        PRInt32 newLevel = 0;
        for (newLevel=0;newLevel<mHitSubMenus->Count();newLevel++) {
          if (newMenu == (nsIMenu *)mHitSubMenus->ElementAt(newLevel)) {
            newFound = PR_TRUE;
            break;
          }
        }

        // Figure out if the parent menu is in the list of popup'ed menus
        PRBool found = PR_FALSE;
        PRInt32 level = 0;
        for (level=0;level<mHitSubMenus->Count();level++) {
          if (parentMenu == (nsIMenu *)mHitSubMenus->ElementAt(level)) {
            found = PR_TRUE;
            break;
          }
        }

        // So now figure out were we are compared to the hit list depth
        // we figure out how many items are open below
        //
        // If the parent was found then we use it
        // if the parent was NOT found this means we are at the very first level (menu from the menubar)
        // Windows will send an event for a parent AND child that is already in the hit list
        // and we think we should be popping it down. So we check to see if the 
        // new menu is already in the tree so it doesn't get removed and then added.
        PRInt32 numToRemove = 0;
        if (found) {
          numToRemove = mHitSubMenus->Count() - level - 1;
        } else {
          // This means we got a menu event for a menubar menu
          if (newFound) { // newFound checks to see if the new menu to be added is already in the hit list
            numToRemove = mHitSubMenus->Count() - newLevel - 1;
          } else {
            numToRemove = mHitSubMenus->Count();
          }
        }

        // If we are to remove 1 item && the new menu to be added is the 
        // same as the one we would be removing, then don't remove it.
        if (numToRemove == 1 && newMenu == (nsIMenu *)mHitSubMenus->ElementAt(mHitSubMenus->Count()-1)) {
          numToRemove = 0;
        }

        // Now loop thru and removing the menu from thre list
        PRInt32 ii;
        for (ii=0;ii<numToRemove;ii++) {
          nsIMenu * m = (nsIMenu *)mHitSubMenus->ElementAt(mHitSubMenus->Count()-1 );
          //AdjustMenus(m, nsnull, event);
		  nsIMenu * aCurrentMenu = m;
		  nsIMenu * aNewMenu = nsnull;
		  nsMenuEvent aEvent = event;
          //static void AdjustMenus(nsIMenu * aCurrentMenu, nsIMenu * aNewMenu, nsMenuEvent & aEvent) 
		  {
            if (nsnull != aCurrentMenu) {
              nsIMenuListener * listener;
              if (NS_OK == aCurrentMenu->QueryInterface(nsCOMTypeInfo<nsIMenuListener>::GetIID(), (void **)&listener)) {
                listener->MenuDeselected(aEvent);
                NS_RELEASE(listener);
			  }
			}
            if (nsnull != aNewMenu)  {
              nsIMenuListener * listener;
              if (NS_OK == aNewMenu->QueryInterface(nsCOMTypeInfo<nsIMenuListener>::GetIID(), (void **)&listener)) {
		        NS_ASSERTION(false, "get debugger");
                listener->MenuSelected(aEvent);
				NS_RELEASE(listener);
			  }
			}
		  }
          nsString name;
          m->GetLabel(name);
          NS_RELEASE(m);
          mHitSubMenus->RemoveElementAt(mHitSubMenus->Count()-1);
        }
 
        // At this point we bail if we are a menu item
        if (isMenuItem) {
          NS_RELEASE(event.widget);
          return NS_OK;
        }

        // Here we know we have a menu, check one last time to see 
        // if the new one is the last one in the list
        // Add it if it isn't or skip adding it
        nsString name;
        newMenu->GetLabel(name);
        if (newMenu != (nsIMenu *)mHitSubMenus->ElementAt(mHitSubMenus->Count()-1)) {
          mHitSubMenus->AppendElement(newMenu);
          NS_ADDREF(newMenu);
          //AdjustMenus(nsnull, newMenu, event);
		  nsIMenu * aCurrentMenu = nsnull;
		  nsIMenu * aNewMenu = newMenu;
		  nsMenuEvent aEvent = event;
          //static void AdjustMenus(nsIMenu * aCurrentMenu, nsIMenu * aNewMenu, nsMenuEvent & aEvent) 
		  {
            if (nsnull != aCurrentMenu) {
              nsIMenuListener * listener;
              if (NS_OK == aCurrentMenu->QueryInterface(nsCOMTypeInfo<nsIMenuListener>::GetIID(), (void **)&listener)) {
                listener->MenuDeselected(aEvent);
                NS_RELEASE(listener);
			  }
			}
            if (nsnull != aNewMenu)  {
              nsIMenuListener * listener;
              if (NS_OK == aNewMenu->QueryInterface(nsCOMTypeInfo<nsIMenuListener>::GetIID(), (void **)&listener)) {
                listener->MenuSelected(aEvent);
                NS_RELEASE(listener);
			  }
			}
		  }
        }
        NS_RELEASE(parentMenu);
      } else {
        //printf("no menu was found. This is bad.\n");
        // XXX need to assert here!
      }
    }
  }  
  NS_RELEASE(event.widget);
  return NS_OK;
}
//---------------------------------------------------------
NS_METHOD nsWindow::EnableDragDrop(PRBool aEnable)
{
  nsresult rv = NS_ERROR_FAILURE;

  if (aEnable) {
    if (nsnull == mNativeDragTarget) {
       mNativeDragTarget = new nsNativeDragTarget(this);
       if (NULL != mNativeDragTarget) {
         mNativeDragTarget->AddRef();
         if (S_OK == ::CoLockObjectExternal((LPUNKNOWN)mNativeDragTarget,TRUE,FALSE)) {
           if (S_OK == ::RegisterDragDrop(mWnd, (LPDROPTARGET)mNativeDragTarget)) {
             rv = NS_OK;
           }
         }
       }
    }
  } else {
    if (nsnull != mWnd && NULL != mNativeDragTarget) {
      ::RevokeDragDrop(mWnd);
      if (S_OK == ::CoLockObjectExternal((LPUNKNOWN)mNativeDragTarget, FALSE, TRUE)) {
        rv = NS_OK;
      }
      NS_RELEASE(mNativeDragTarget);
    }
  }

  return rv;
}


//-------------------------------------------------------------------------
//
// OnKey
//
//-------------------------------------------------------------------------
PRBool nsWindow::DispatchKeyEvent(PRUint32 aEventType, WORD aCharCode, UINT aVirtualCharCode)
{
  nsKeyEvent event;
  nsPoint point;

  point.x = 0;
  point.y = 0;

  InitEvent(event, aEventType, &point); // this add ref's event.widget

  event.charCode = aCharCode;
  event.keyCode  = aVirtualCharCode;

#ifdef KE_DEBUG
  static cnt=0;
  printf("%d DispatchKE Type: %s charCode %d  keyCode %d ", cnt++,  
        (NS_KEY_PRESS == aEventType)?"PRESS":(aEventType == NS_KEY_UP?"Up":"Down"), 
         event.charCode, event.keyCode);
  printf("Shift: %s Control %s Alt: %s \n",  (mIsShiftDown?"D":"U"), (mIsControlDown?"D":"U"), (mIsAltDown?"D":"U"));
  printf("[%c][%c][%c] <==   [%c][%c][%c][ space bar ][%c][%c][%c]\n", 
             IS_VK_DOWN(NS_VK_SHIFT) ? 'S' : ' ',
             IS_VK_DOWN(NS_VK_CONTROL) ? 'C' : ' ',
             IS_VK_DOWN(NS_VK_ALT) ? 'A' : ' ',
             IS_VK_DOWN(VK_LSHIFT) ? 'S' : ' ',
             IS_VK_DOWN(VK_LCONTROL) ? 'C' : ' ',
             IS_VK_DOWN(VK_LMENU) ? 'A' : ' ',
             IS_VK_DOWN(VK_RMENU) ? 'A' : ' ',
             IS_VK_DOWN(VK_RCONTROL) ? 'C' : ' ',
             IS_VK_DOWN(VK_RSHIFT) ? 'S' : ' '

  );
#endif

  event.isShift   = mIsShiftDown;
  event.isControl = mIsControlDown;
  event.isMeta   =  PR_FALSE;
  event.isAlt     = mIsAltDown;
  event.eventStructType = NS_KEY_EVENT;

  PRBool result = DispatchWindowEvent(&event);
  NS_RELEASE(event.widget);

  return result;
}

//-----------------------------------------------------
static BOOL IsKeypadKey(UINT vCode, long isExtended)
{
    switch (vCode) {
        case VK_HOME:  
        case VK_END:   
        case VK_PRIOR: 
        case VK_NEXT:  
        case VK_UP:    
        case VK_DOWN:  
        case VK_LEFT:  
        case VK_RIGHT: 
        case VK_CLEAR:
        case VK_INSERT:
            // extended are not keypad keys
            if (isExtended) 
                break;
        case VK_NUMPAD0:
        case VK_NUMPAD1:
        case VK_NUMPAD2:
        case VK_NUMPAD3:
        case VK_NUMPAD4:
        case VK_NUMPAD5:
        case VK_NUMPAD6:
        case VK_NUMPAD7:
        case VK_NUMPAD8:
        case VK_NUMPAD9:
            return TRUE;
    }

    return FALSE;
}

//-------------------------------------------------------------------------
//
// return 
//   EXTENDED_KEY     for extended keys supported by java
//   SPECIAL_KEY      for extended keys 
//   DONT_PROCESS_KEY for extended keys of no interest (never exposed to java)
//   STANDARD_KEY     for standard keys
//
//-------------------------------------------------------------------------
#define STANDARD_KEY     1
#define EXTENDED_KEY     2
#define SPECIAL_KEY      3
#define DONT_PROCESS_KEY 4

//-------------------------------------------------------------------------
ULONG nsWindow::IsSpecialChar(UINT aVirtualKeyCode, WORD *aAsciiKey)
{
  ULONG keyType = EXTENDED_KEY;

  *aAsciiKey   = 0;

  // Process non-standard Control Keys
  if (mIsControlDown && !mIsShiftDown && !mIsAltDown &&
      ((aVirtualKeyCode >= 0x30 && aVirtualKeyCode <= 0x39) ||  // 0-9
       (aVirtualKeyCode >= 0xBA && aVirtualKeyCode <= 0xC0) ||  // ;=,-./` (semi-colon,equals,comma,dash,period,slash,back tick)
       (aVirtualKeyCode == 0xDE))) {                            // ' (tick)
    *aAsciiKey = aVirtualKeyCode;
    return SPECIAL_KEY;
  }

  //printf("*********************** 0x%x\n", aVirtualKeyCode);
  switch (aVirtualKeyCode) {
    case VK_TAB:
    case VK_HOME:
    case VK_END:
    case VK_PRIOR:
    case VK_NEXT:
    case VK_UP:
    case VK_DOWN:
    case VK_LEFT:
    case VK_RIGHT:
    case VK_F1: 
    case VK_F2:  
    case VK_F3:  
    case VK_F4:  
    case VK_F5:   
    case VK_F6:   
    case VK_F7:   
    case VK_F8:   
    case VK_F9:    
    case VK_F10:   
    case VK_F11:   
    case VK_F12: 
	case VK_RETURN:
	case VK_BACK:
		*aAsciiKey = aVirtualKeyCode;
      break;

    case VK_DELETE:
      *aAsciiKey = '\177'; 
      keyType = SPECIAL_KEY;   
      break;

    case VK_MENU:
      // Let this through for XP menus.
      *aAsciiKey = aVirtualKeyCode;
      break;

    default:        
      keyType = STANDARD_KEY;
      break;
  }

  return keyType;
}

//-------------------------------------------------------------------------
//
// change the virtual key coming from windows into an ascii code
//
//-------------------------------------------------------------------------
BOOL TranslateToAscii(BYTE *aKeyState, 
                      UINT  aVirtualKeyCode, 
                      UINT  aScanCode, 
                      WORD *aAsciiKey)
{
  WORD asciiBuf;
  BOOL bIsExtended;

  bIsExtended = TRUE;
  *aAsciiKey   = 0;
  switch (aVirtualKeyCode) {
    case VK_TAB:
    case VK_HOME:
    case VK_END:
    case VK_PRIOR:
    case VK_NEXT:
    case VK_UP:
    case VK_DOWN:
    case VK_LEFT:
    case VK_RIGHT:
    case VK_F1: 
    case VK_F2:  
    case VK_F3:  
    case VK_F4:  
    case VK_F5:   
    case VK_F6:   
    case VK_F7:   
    case VK_F8:   
    case VK_F9:    
    case VK_F10:   
    case VK_F11:   
    case VK_F12:  
      *aAsciiKey = aVirtualKeyCode;
      break;


    case VK_DELETE:
      *aAsciiKey = '\177'; 
      bIsExtended = FALSE;   
      break;

    case VK_RETURN:
      *aAsciiKey = '\n';   
      bIsExtended = FALSE;   
      break;

    default:        
      bIsExtended = FALSE;

      if (::ToAscii(aVirtualKeyCode, aScanCode, aKeyState, &asciiBuf, FALSE) == 1) {
        *aAsciiKey = (char)asciiBuf;
      }

      break;
  }
  return bIsExtended;
}


//-------------------------------------------------------------------------
//
//
//-------------------------------------------------------------------------
#define WM_CHAR_LATER(vk) ( ((vk)<= VK_SPACE) || \
                                 (('0'<=(vk))&&((vk)<='9')) || \
                                 (('A'<=(vk))&&((vk)<='Z')) || \
                                 ((VK_NUMPAD0 <=(vk))&&((vk)<=VK_DIVIDE)) || \
								 ((0xBA <=(vk))&&((vk)<=NS_VK_BACK_QUOTE)) || \
								 ((NS_VK_OPEN_BRACKET <=(vk))&&((vk)<=NS_VK_QUOTE)) \
                            )
#define NO_WM_CHAR_LATER(vk) (! WM_CHAR_LATER(vk))

BOOL nsWindow::OnKeyDown( UINT aVirtualKeyCode, UINT aScanCode)
{
  WORD asciiKey;

  asciiKey = 0;

  //printf("In OnKeyDown ascii %d  virt: %d  scan: %d\n", asciiKey, aVirtualKeyCode, aScanCode);

  BOOL result = DispatchKeyEvent(NS_KEY_DOWN, asciiKey, aVirtualKeyCode);

  // XXX: this is a special case hack, should probably use IsSpecialChar and
  //      do the right thing for all SPECIAL_KEY codes
  // "SPECIAL_KEY" keys don't generate a WM_CHAR, so don't generate an NS_KEY_PRESS
  // this is a special case for the delete key
  if (aVirtualKeyCode==VK_DELETE) 
  {
    DispatchKeyEvent(NS_KEY_PRESS, 0, aVirtualKeyCode);
  } 
  else if (NO_WM_CHAR_LATER(aVirtualKeyCode)) 
  {
    DispatchKeyEvent(NS_KEY_PRESS, 0, aVirtualKeyCode);
  } 
  else if (mIsControlDown && 
           (( NS_VK_0 <= aVirtualKeyCode)&&( aVirtualKeyCode <= NS_VK_9)))
  {
    // put the 0 - 9 in charcode instead of keycode.
    DispatchKeyEvent(NS_KEY_PRESS, aVirtualKeyCode, 0);
  }

  return result;
}

//-------------------------------------------------------------------------
//
//
//-------------------------------------------------------------------------
BOOL nsWindow::OnKeyUp( UINT aVirtualKeyCode, UINT aScanCode)
{
  WORD asciiKey;

  asciiKey = 0;

  switch (IsSpecialChar(aVirtualKeyCode, &asciiKey)) {
    case EXTENDED_KEY:
      break;

    case STANDARD_KEY: {
      BYTE keyState[256];
      ::GetKeyboardState(keyState);
      ::ToAscii(aVirtualKeyCode, aScanCode, keyState, &asciiKey, FALSE);
      } break;

    case SPECIAL_KEY:
      break;

  } // switch

  if (asciiKey) {
    //printf("Dispatching Key Up [%d]\n", asciiKey);
    DispatchKeyEvent(NS_KEY_UP, asciiKey, aVirtualKeyCode);
  }

  // always let the def proc process a WM_KEYUP
  return FALSE;
}


//-------------------------------------------------------------------------
//
//
//-------------------------------------------------------------------------
BOOL nsWindow::OnChar( UINT mbcsCharCode, UINT virtualKeyCode, bool isMultiByte )
{
  wchar_t	uniChar;
  char		charToConvert[2];
  size_t	length;

#ifdef IME_FROM_ON_CHAR
  if (isMultiByte) {
	  charToConvert[0]=HIBYTE(mbcsCharCode);
	  charToConvert[1] = LOBYTE(mbcsCharCode);
	  length=2;
  } 
  else 
#endif
  {
	  charToConvert[0] = LOBYTE(mbcsCharCode);
	  length=1;
  }

  
  if(mIsControlDown && (virtualKeyCode <= 0x1A)) // Ctrl+A Ctrl+Z, see Programming Windows 3.1 page 110 for details  
  { 
    // need to account for shift here.  bug 16486 
    if ( mIsShiftDown ) 
      uniChar = virtualKeyCode - 1 + 'A' ; 
    else 
      uniChar = virtualKeyCode - 1 + 'a' ; 
    virtualKeyCode = 0;
  } 
  else 
  {
    if(virtualKeyCode < 0x20) 
    {
      uniChar = 0;
    } 
    else 
    {
      ::MultiByteToWideChar(mCurrentKeyboardCP,MB_PRECOMPOSED,charToConvert,length,
	    &uniChar,sizeof(uniChar));
      virtualKeyCode = 0;
      mIsShiftDown = PR_FALSE;
    }
  }
  return DispatchKeyEvent(NS_KEY_PRESS, uniChar, virtualKeyCode);

  //return FALSE;
}



//-------------------------------------------------------------------------
//
// Process all nsWindows messages
//
//-------------------------------------------------------------------------
PRBool nsWindow::ProcessMessage(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *aRetValue)
{
    static UINT vkKeyCached = 0;                // caches VK code fon WM_KEYDOWN
    static BOOL firstTime = TRUE;                // for mouse wheel logic
    static int  iDeltaPerLine, iAccumDelta ;     // for mouse wheel logic
#ifdef LOSER
    ULONG       ulScrollLines ;                  // for mouse wheel logic
#endif

    PRBool        result = PR_FALSE; // call the default nsWindow proc
    nsPaletteInfo palInfo;
    *aRetValue = 0;

    switch (msg) {

        case WM_COMMAND: {
          WORD wNotifyCode = HIWORD(wParam); // notification code 
          if ((CBN_SELENDOK == wNotifyCode) || (CBN_SELENDCANCEL == wNotifyCode)) { // Combo box change
            nsGUIEvent event;
            event.eventStructType = NS_GUI_EVENT;
            nsPoint point(0,0);
            InitEvent(event, NS_CONTROL_CHANGE, &point); // this add ref's event.widget
            result = DispatchWindowEvent(&event);
            NS_RELEASE(event.widget);
          } else if (wNotifyCode == 0) { // Menu selection
            nsMenuEvent event;
            event.mCommand = LOWORD(wParam);
            event.eventStructType = NS_MENU_EVENT;
            InitEvent(event, NS_MENU_SELECTED);
            result = DispatchWindowEvent(&event);

            if (mMenuBar) {
              PRUint32 i, count;
              mMenuBar->GetMenuCount(count);
              for (i=0;i<count;i++) {
                nsIMenu * menu;
                mMenuBar->GetMenuAt(i, menu);
                nsIMenuItem * menuItem = FindMenuItem(menu, event.mCommand);
                if (menuItem) {
                  nsIMenuListener * listener;
                  if (NS_OK == menuItem->QueryInterface(nsCOMTypeInfo<nsIMenuListener>::GetIID(), (void **)&listener)) {
                    listener->MenuItemSelected(event);
                    NS_RELEASE(listener);

					          menu->QueryInterface(nsCOMTypeInfo<nsIMenuListener>::GetIID(), (void **)&listener);
					          if(listener){
					            //listener->MenuDestruct(event);
					            NS_RELEASE(listener);
					          }
                  }
                  NS_RELEASE(menuItem);
                }
                NS_RELEASE(menu);
              }
            }
            NS_RELEASE(event.widget);
          }
        }
        break;

        case WM_DISPLAYCHANGE:
          DispatchStandardEvent(NS_DISPLAYCHANGED);
        break;

        
        case WM_NOTIFY:
            // TAB change
          {
            LPNMHDR pnmh = (LPNMHDR) lParam;

            switch (pnmh->code) {
              case TCN_SELCHANGE: {
                DispatchStandardEvent(NS_TABCHANGE);
                result = PR_TRUE;
              }
              break;

            }
          }
          break;

        case WM_MOVE: // Window moved 
          {
            PRInt32 x = (PRInt32)LOWORD(lParam); // horizontal position in screen coordinates 
            PRInt32 y = (PRInt32)HIWORD(lParam); // vertical position in screen coordinates 
            result = OnMove(x, y); 
          }
          break;

        case WM_DESTROY:
            // clean up.
            OnDestroy();
            result = PR_TRUE;
            break;

        case WM_PAINT:
            result = OnPaint();
            break;
	case WM_SYSCHAR:
	case WM_CHAR: 
        {
#ifdef KE_DEBUG
            printf("%s\tchar=%c\twp=%4x\tlp=%8x\n", (msg == WM_SYSCHAR) ? "WM_SYSCHAR" : "WM_CHAR" , wParam, wParam, lParam);
#endif
            mIsShiftDown   = IS_VK_DOWN(NS_VK_SHIFT);
            if(WM_SYSCHAR==msg)
            {
                mIsControlDown = IS_VK_DOWN(NS_VK_CONTROL);
                mIsAltDown     = IS_VK_DOWN(NS_VK_ALT);
            } else { // WM_KEYUP
                // If the Context Code bit is down and we got a WM_KEY
                // it is a key press for character, not accelerator
                // see p246 of Programming Windows 95 [Charles Petzold] for details
                mIsControlDown = (0 == (KF_ALTDOWN & HIWORD(lParam)))&& IS_VK_DOWN(NS_VK_CONTROL);
                mIsAltDown     = (0 == (KF_ALTDOWN & HIWORD(lParam)))&& IS_VK_DOWN(NS_VK_ALT);
            }

            unsigned char    ch = (unsigned char)wParam;
            UINT            char_result;
  
            //
            // check first for backspace or return, handle them specially 
            //
            if (ch==0x0d || ch==0x08) {

#ifdef IME_FROM_ON_CHAR
                mHaveDBCSLeadByte = PR_FALSE;
#endif // IME_FROM_ON_CHAR

                result = OnChar(ch,ch==0x0d ? VK_RETURN : VK_BACK,true);
                break;
            }
  
#ifdef IME_FROM_ON_CHAR
            //
            // check first to see if we have the first byte of a two-byte DBCS sequence
            //  if so, store it away and do nothing until we get the second sequence
            //
            if (NS_IsDBCSLeadByte(mCurrentKeyboardCP, ch) && !mHaveDBCSLeadByte) {
                mHaveDBCSLeadByte = TRUE;
                mDBCSLeadByte = ch;
                result = PR_TRUE;
                break;
            }
  
            //
            // at this point, we may have the second byte of a DBCS sequence or a single byte
            // character, depending on the previous message.  Check and handle accordingly
            //
            if (mHaveDBCSLeadByte) {
                char_result = (mDBCSLeadByte << 8) | ch;
                mHaveDBCSLeadByte = FALSE;
                mDBCSLeadByte = 0;
                result = OnChar(char_result,ch,true);
            } 
            else 
#endif // IME_FROM_ON_CHAR
            {
                char_result = ch;
                result = OnChar(char_result,ch,false);
            }
  
            break;
        }
        // Let ths fall through if it isn't a key pad
        case WM_SYSKEYUP:
            // if it's a keypad key don't process a WM_CHAR will come or...oh well...
            if (IsKeypadKey(wParam, lParam & 0x01000000)) {
                result = PR_TRUE;
                break;
            }
        case WM_KEYUP: 
#ifdef KE_DEBUG
            printf("%s\t\twp=%x\tlp=%x\n",  
                   (WM_KEYUP==msg)?"WM_KEYUP":"WM_SYSKEYUP" , wParam, lParam);
#endif
            mIsShiftDown   = IS_VK_DOWN(NS_VK_SHIFT);
            if(WM_SYSKEYUP==msg)
            {
                mIsControlDown = IS_VK_DOWN(NS_VK_CONTROL);
                mIsAltDown     = IS_VK_DOWN(NS_VK_ALT);
            } else { // WM_KEYUP
                // If the Context Code bit is down and we got a WM_KEY
                // it is a key press for character, not accelerator
                // see p246 of Programming Windows 95 [Charles Petzold] for details
                mIsControlDown = (0 == (KF_ALTDOWN & HIWORD(lParam)))&& IS_VK_DOWN(NS_VK_CONTROL);
                mIsAltDown     = (0 == (KF_ALTDOWN & HIWORD(lParam)))&& IS_VK_DOWN(NS_VK_ALT);
            }

            if (!mIMEIsComposing)
              result = OnKeyUp(wParam, (HIWORD(lParam) & 0xFF));
			      else
				      result = PR_FALSE;

            // Let's consume the ALT key up so that we don't go into
            // a menu bar if we don't have one.
            // XXX This will cause a tiny breakage in viewer... namely
            // that hitting ALT by itself in viewer won't move you into
            // the menu.  ALT+shortcut key will still work, though, so
            // I figure this is ok.
            if (!mMenuBar && (wParam == NS_VK_ALT)) {
              result = PR_TRUE;
              *aRetValue = 0;
            }

            break;

        // Let ths fall through if it isn't a key pad
        case WM_SYSKEYDOWN:
            // if it's a keypad key don't process a WM_CHAR will come or...oh well...
            if (IsKeypadKey(wParam, lParam & 0x01000000)) {
				      result = PR_TRUE;
              break;
            }
        case WM_KEYDOWN: {
#ifdef KE_DEBUG
            printf("%s\t\twp=%4x\tlp=%8x\n",  
                   (WM_KEYDOWN==msg)?"WM_KEYDOWN":"WM_SYSKEYDOWN" , wParam, lParam);
#endif
            mIsShiftDown   = IS_VK_DOWN(NS_VK_SHIFT);
            if(WM_SYSKEYDOWN==msg)
            {
                mIsControlDown = IS_VK_DOWN(NS_VK_CONTROL);
                mIsAltDown     = IS_VK_DOWN(NS_VK_ALT);
            } else { // WM_KEYUP
                // If the Context Code bit is down and we got a WM_KEY
                // If the Context Code bit is down and we got a WM_KEY
                // it is a key press for character, not accelerator
                // see p246 of Programming Windows 95 [Charles Petzold] for details
                mIsControlDown = (0 == (KF_ALTDOWN & HIWORD(lParam)))&& IS_VK_DOWN(NS_VK_CONTROL);
                mIsAltDown     = (0 == (KF_ALTDOWN & HIWORD(lParam)))&& IS_VK_DOWN(NS_VK_ALT);
            }
            if (!mIMEIsComposing)
               result = OnKeyDown(wParam, (HIWORD(lParam) & 0xFF));
	    else
	       result = PR_FALSE;
            }
            break;

        // say we've dealt with erase background if widget does
        // not need auto-erasing
        case WM_ERASEBKGND: 
            if (! AutoErase()) {
              *aRetValue = 1;
              result = PR_TRUE;
            } 
            break;

        case WM_MOUSEMOVE:
            //RelayMouseEvent(msg,wParam, lParam); 
            result = DispatchMouseEvent(NS_MOUSE_MOVE);
            break;

        case WM_LBUTTONDOWN:
            //SetFocus(); // this is bad
            //RelayMouseEvent(msg,wParam, lParam); 
            result = DispatchMouseEvent(NS_MOUSE_LEFT_BUTTON_DOWN);
            break;

        case WM_LBUTTONUP:
            //RelayMouseEvent(msg,wParam, lParam); 
            result = DispatchMouseEvent(NS_MOUSE_LEFT_BUTTON_UP);
            break;

        case WM_LBUTTONDBLCLK:
            result = DispatchMouseEvent(NS_MOUSE_LEFT_DOUBLECLICK);
            break;

        case WM_MBUTTONDOWN:
            result = DispatchMouseEvent(NS_MOUSE_MIDDLE_BUTTON_DOWN);
            break;

        case WM_MBUTTONUP:
            result = DispatchMouseEvent(NS_MOUSE_MIDDLE_BUTTON_UP);
            break;

        case WM_MBUTTONDBLCLK:
            result = DispatchMouseEvent(NS_MOUSE_MIDDLE_BUTTON_DOWN);           
            break;

        case WM_RBUTTONDOWN:
            result = DispatchMouseEvent(NS_MOUSE_RIGHT_BUTTON_DOWN);            
            break;

        case WM_RBUTTONUP:
            result = DispatchMouseEvent(NS_MOUSE_RIGHT_BUTTON_UP);
            break;

        case WM_RBUTTONDBLCLK:
            result = DispatchMouseEvent(NS_MOUSE_RIGHT_DOUBLECLICK);                      
            break;

        case WM_HSCROLL:
        case WM_VSCROLL: 
	          // check for the incoming nsWindow handle to be null in which case
	          // we assume the message is coming from a horizontal scrollbar inside
	          // a listbox and we don't bother processing it (well, we don't have to)
	          if (lParam) {
                nsWindow* scrollbar = (nsWindow*)::GetWindowLong((HWND)lParam, GWL_USERDATA);

		            if (scrollbar) {
		                result = scrollbar->OnScroll(LOWORD(wParam), (short)HIWORD(wParam));
		            }
	          }
            break;

        case WM_CTLCOLORLISTBOX:
        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORBTN:
        //case WM_CTLCOLORSCROLLBAR: //XXX causes a the scrollbar to be drawn incorrectly
        case WM_CTLCOLORSTATIC:
	          if (lParam) {
              nsWindow* control = (nsWindow*)::GetWindowLong((HWND)lParam, GWL_USERDATA);
		          if (control) {
                control->SetUpForPaint((HDC)wParam);
		            *aRetValue = (LPARAM)control->OnControlColor();
              }
	          }
    
            result = PR_TRUE;
            break;

        case WM_SETFOCUS:
            result = DispatchFocus(NS_GOTFOCUS);
            break;

        case WM_KILLFOCUS:
            result = DispatchFocus(NS_LOSTFOCUS);
            break;

        case WM_WINDOWPOSCHANGED: 
        {
            WINDOWPOS *wp = (LPWINDOWPOS)lParam;

            // We only care about a resize, so filter out things like z-order
            // changes. Note: there's a WM_MOVE handler above which is why we're
            // not handling them here...
            if (0 == (wp->flags & SWP_NOSIZE)) {
              // XXX Why are we using the client size area? If the size notification
              // is for the client area then the origin should be (0,0) and not
              // the window origin in screen coordinates...
              RECT r;
              ::GetWindowRect(mWnd, &r);
              PRInt32 newWidth, newHeight;
              newWidth = PRInt32(r.right - r.left);
              newHeight = PRInt32(r.bottom - r.top);
              nsRect rect(wp->x, wp->y, newWidth, newHeight);
              //if (newWidth != mBounds.width)
              {
                RECT drect;

                //getting wider

                drect.left = wp->x + mBounds.width;
                drect.top = wp->y;
                drect.right = drect.left + (newWidth - mBounds.width);
                drect.bottom = drect.top + newHeight;

//                ::InvalidateRect(mWnd, NULL, FALSE);
//                ::InvalidateRect(mWnd, &drect, FALSE);
                ::RedrawWindow(mWnd, &drect, NULL,
                               RDW_INVALIDATE | RDW_NOERASE | RDW_NOINTERNALPAINT | RDW_ERASENOW | RDW_ALLCHILDREN);
              }
              //if (newHeight != mBounds.height)
              {
                RECT drect;

                //getting taller

                drect.left = wp->x;
                drect.top = wp->y + mBounds.height;
                drect.right = drect.left + newWidth;
                drect.bottom = drect.top + (newHeight - mBounds.height);

//                ::InvalidateRect(mWnd, NULL, FALSE);
//                ::InvalidateRect(mWnd, &drect, FALSE);
                ::RedrawWindow(mWnd, &drect, NULL,
                               RDW_INVALIDATE | RDW_NOERASE | RDW_NOINTERNALPAINT | RDW_ERASENOW | RDW_ALLCHILDREN);
              }
              mBounds.width  = newWidth;
              mBounds.height = newHeight;
              ///nsRect rect(wp->x, wp->y, wp->cx, wp->cy);

              // recalculate the width and height
              // this time based on the client area
              if (::GetClientRect(mWnd, &r)) {
                rect.width  = PRInt32(r.right - r.left);
                rect.height = PRInt32(r.bottom - r.top);
              }
              result = OnResize(rect);
            }
            break;
        }

        case WM_MENUSELECT: 
          if (mMenuBar) {
            MenuHasBeenSelected((HMENU)lParam, (UINT)LOWORD(wParam), (UINT)HIWORD(wParam), (UINT) LOWORD(wParam));
          }
          break;

        case WM_SETTINGCHANGE:
          firstTime = TRUE;
          // Fall through
        case WM_MOUSEWHEEL:
#ifdef LOSER
        {
         if (firstTime) {
           firstTime = FALSE;
            //printf("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ WM_SETTINGCHANGE\n");
            SystemParametersInfo (104, 0, &ulScrollLines, 0) ;
            //SystemParametersInfo (SPI_GETWHEELSCROLLLINES, 0, &ulScrollLines, 0) ;
          
            // ulScrollLines usually equals 3 or 0 (for no scrolling)
            // WHEEL_DELTA equals 120, so iDeltaPerLine will be 40

            if (ulScrollLines)
              iDeltaPerLine = WHEEL_DELTA / ulScrollLines ;
            else
              iDeltaPerLine = 0 ;
            //printf("ulScrollLines %d  iDeltaPerLine %d\n", ulScrollLines, iDeltaPerLine);

            if (msg == WM_SETTINGCHANGE) {
              return 0;
            }
         }
         HWND scrollbar = NULL;

         if (nsnull != mVScrollbar) {
           scrollbar = (HWND)mVScrollbar->GetNativeData(NS_NATIVE_WINDOW);
         }
         if (scrollbar) {
            if (iDeltaPerLine == 0)
              break ;

            iAccumDelta += (short) HIWORD (wParam) ;     // 120 or -120

            while (iAccumDelta >= iDeltaPerLine) {    
              //printf("iAccumDelta %d\n", iAccumDelta);
              SendMessage (mWnd, WM_VSCROLL, SB_LINEUP, (LONG)scrollbar) ;
              iAccumDelta -= iDeltaPerLine ;
            }

            while (iAccumDelta <= -iDeltaPerLine) {
              //printf("iAccumDelta %d\n", iAccumDelta);
              SendMessage (mWnd, WM_VSCROLL, SB_LINEDOWN, (LONG)scrollbar) ;
              iAccumDelta += iDeltaPerLine ;
            }
         }
            return 0 ;
      } break;
#endif

        case WM_PALETTECHANGED:
            if ((HWND)wParam == mWnd) {
                // We caused the WM_PALETTECHANGED message so avoid realizing
                // another foreground palette
                result = PR_TRUE;
                break;
            }
            // fall thru...

        case WM_QUERYNEWPALETTE:
            mContext->GetPaletteInfo(palInfo);
            if (palInfo.isPaletteDevice && palInfo.palette) {
                HDC hDC = ::GetDC(mWnd);
                // XXX Setting this to TRUE to stop it hanging
                // it should be FALSE
                //HPALETTE hOldPal = ::SelectPalette(hDC, (HPALETTE)palInfo.palette, FALSE);
                HPALETTE hOldPal = ::SelectPalette(hDC, (HPALETTE)palInfo.palette, TRUE);
                
                // Realize the drawing palette
                int i = ::RealizePalette(hDC);

                // Did any of our colors change?
                if (i > 0) {
                  // Yes, so repaint
                  ::InvalidateRect(mWnd, (LPRECT)NULL, TRUE);
                }

                ::SelectPalette(hDC, hOldPal, TRUE);
                ::RealizePalette(hDC);
                ::ReleaseDC(mWnd, hDC);
                *aRetValue = TRUE;
            }
            result = PR_TRUE;
            break;

		case WM_INPUTLANGCHANGEREQUEST:
			*aRetValue = TRUE;
			result = PR_FALSE;
			break;

		case WM_INPUTLANGCHANGE: {
#if defined(DEBUG_tague) || defined(DEBUG_ftang)
			printf("input language changed\n");
#endif

			result = PR_FALSE;  // always pass to child window
                        *aRetValue = LangIDToCP((WORD)(lParam&0x0FFFF),mCurrentKeyboardCP);
#ifdef IME_FROM_ON_CHAR
                        mHaveDBCSLeadByte=PR_FALSE; // reset this when we change keyboard layout
#endif
   
			break;
		}
		case WM_IME_STARTCOMPOSITION: {
#if defined(DEBUG_tague) || defined(DEBUG_ftang)
			printf("IME: Recieved WM_IME_STARTCOMPOSITION\n");
#endif
			HIMC hIMEContext;

			mIMEIsComposing = PR_TRUE;
			if ((mIMEProperty & IME_PROP_SPECIAL_UI) || (mIMEProperty & IME_PROP_AT_CARET)) {
				return PR_FALSE;
			}

			hIMEContext = ::ImmGetContext(mWnd);
			if (hIMEContext==NULL) {
				return PR_TRUE;
			}
			HandleStartComposition(hIMEContext);
			result = PR_TRUE;
			::ImmReleaseContext(mWnd,hIMEContext);
			}
			break;

		case WM_IME_COMPOSITION: {
#if defined(DEBUG_tague) || defined(DEBUG_ftang)
			printf("IME: Recieved WM_IME_COMPOSITION\n");
#endif
                        PRBool bSendEvent = PR_FALSE;
			HIMC hIMEContext;

			result = PR_FALSE;					// will change this if an IME message we handle

			hIMEContext = ::ImmGetContext(mWnd);
			if (hIMEContext==NULL) {
				return PR_TRUE;
			}

			//
			// This provides us with the attribute string necessary for doing hiliting
			//
			if (lParam & GCS_COMPATTR) {
#if defined(DEBUG_tague) || defined(DEBUG_ftang)
				fprintf(stderr,"nsWindow::WM_IME_COMPOSITION: handling GCS_COMPATTR\n");
#endif
				long attrStrLen = ::ImmGetCompositionString(hIMEContext,GCS_COMPATTR,NULL,0);
				if (attrStrLen+1>mIMEAttributeStringSize) {
					if (mIMEAttributeString!=NULL) delete [] mIMEAttributeString;
					mIMEAttributeString = new char[attrStrLen+32];
					mIMEAttributeStringSize = attrStrLen+32;
				}

				::ImmGetCompositionString(hIMEContext,GCS_COMPATTR,mIMEAttributeString,mIMEAttributeStringSize);
				mIMEAttributeStringLength = attrStrLen;
				mIMEAttributeString[attrStrLen]='\0';
			}

			if (lParam & GCS_COMPCLAUSE) {
#if defined(DEBUG_tague) || defined(DEBUG_ftang)
				fprintf(stderr,"nsWindow::WM_IME_COMPOSITION: handling GCS_COMPCLAUSE\n");
#endif
				long compClauseLen = ::ImmGetCompositionString(hIMEContext,GCS_COMPCLAUSE,NULL,0);
				if (compClauseLen+1>mIMECompClauseStringSize) {
					if (mIMECompClauseString!=NULL) delete [] mIMECompClauseString;
					mIMECompClauseString = new char [compClauseLen+32];
					mIMECompClauseStringSize = compClauseLen+32;
				}

				::ImmGetCompositionString(hIMEContext,GCS_COMPCLAUSE,mIMECompClauseString,mIMECompClauseStringSize);
				mIMECompClauseStringLength = compClauseLen;
				mIMECompClauseString[compClauseLen]='\0';
			} else {
				mIMECompClauseStringLength = 0;
			}

			if (lParam & GCS_CURSORPOS) {
#if defined(DEBUG_tague) || defined(DEBUG_ftang)
				fprintf(stderr,"nsWindow::WM_IME_COMPOSITION: handling GCS_CURSORPOS\n");
#endif
				mIMECursorPosition = ::ImmGetCompositionString(hIMEContext,GCS_CURSORPOS,NULL,0);
			}

			//
			// This catches a fixed result
			//
			if (lParam & GCS_RESULTSTR) {
#if defined(DEBUG_tague) || defined(DEBUG_ftang)
				fprintf(stderr,"nsWindow::WM_IME_COMPOSITION: handling GCS_RESULTSTR\n");
#endif
				long compStrLen = ::ImmGetCompositionString(hIMEContext,GCS_RESULTSTR,NULL,0);
				if (compStrLen+1>mIMECompositionStringSize) {
					delete [] mIMECompositionString;
					mIMECompositionString = new char[compStrLen+32];
					mIMECompositionStringSize = compStrLen+32;
				}
				
				::ImmGetCompositionString(hIMEContext,GCS_RESULTSTR,mIMECompositionString,mIMECompositionStringSize);
				mIMECompositionStringLength = compStrLen;
				mIMECompositionString[compStrLen]='\0';
				result = PR_TRUE;
				HandleTextEvent(hIMEContext, PR_FALSE);
				HandleEndComposition();
				HandleStartComposition(hIMEContext);
                                bSendEvent = PR_TRUE;
			}
			//
			// This provides us with a composition string
			//
			if (lParam & GCS_COMPSTR) {
#if defined(DEBUG_tague) || defined(DEBUG_ftang)
				fprintf(stderr,"nsWindow::WM_IME_COMPOSITION: handling GCS_COMPSTR\n");
#endif
				long compStrLen = ::ImmGetCompositionString(hIMEContext,GCS_COMPSTR,NULL,0);
				if (compStrLen+1>mIMECompositionStringSize) {
					if (mIMECompositionString!=NULL) delete [] mIMECompositionString;
					mIMECompositionString = new char[compStrLen+32];
					mIMECompositionStringSize = compStrLen+32;
				}
				
				::ImmGetCompositionString(hIMEContext,GCS_COMPSTR,mIMECompositionString,mIMECompositionStringSize);
				mIMECompositionStringLength = compStrLen;
				mIMECompositionString[compStrLen]='\0';
				HandleTextEvent(hIMEContext);
				result = PR_TRUE;
                                bSendEvent = PR_TRUE;
			}
                        if(! bSendEvent)
                        {
#if defined(DEBUG_tague) || defined(DEBUG_ftang)
				fprintf(stderr,"nsWindow::WM_IME_COMPOSITION: haandle 0 length TextEvent. \n");
#endif
				mIMECompositionStringLength = 0;
				HandleTextEvent(hIMEContext,PR_FALSE);
                                bSendEvent = PR_TRUE;
                        }
			
			::ImmReleaseContext(mWnd,hIMEContext);

			}
			break;

		case WM_IME_ENDCOMPOSITION:

			mIMEIsComposing = PR_FALSE;
#if defined(DEBUG_tague) || defined(DEBUG_ftang)
			printf("IME: Received WM_IME_ENDCOMPOSITION\n");
#endif
			HandleEndComposition();
			result = PR_TRUE;
			break;

        case WM_DROPFILES: {
          /*HDROP hDropInfo = (HDROP) wParam;
	        UINT nFiles = ::DragQueryFile(hDropInfo, (UINT)-1, NULL, 0);

	        for (UINT iFile = 0; iFile < nFiles; iFile++) {
		        TCHAR szFileName[_MAX_PATH];
		        ::DragQueryFile(hDropInfo, iFile, szFileName, _MAX_PATH);
            printf("szFileName [%s]\n", szFileName);
            nsAutoString fileStr(szFileName);
            nsEventStatus status;
            nsDragDropEvent event;
            InitEvent(event, NS_DRAGDROP_EVENT);
            event.mType      = nsDragDropEventStatus_eDrop;
            event.mIsFileURL = PR_FALSE;
            event.mURL       = (PRUnichar *)fileStr.GetUnicode();
            DispatchEvent(&event, status);
	        }*/
        } break;

      case WM_DESTROYCLIPBOARD: {
        nsIClipboard* clipboard;
        nsresult rv = nsServiceManager::GetService(kCClipboardCID,
                                                   nsCOMTypeInfo<nsIClipboard>::GetIID(),
                                                   (nsISupports **)&clipboard);
        clipboard->EmptyClipboard();
        nsServiceManager::ReleaseService(kCClipboardCID, clipboard);
      } break;

    }

    //*aRetValue = result;
    if (mWnd) {
      return result;
    }
    else {
      //Events which caused mWnd destruction and aren't consumed
      //will crash during the Windows default processing.
      return PR_TRUE;
    }
}



//-------------------------------------------------------------------------
//
// return the window class name and initialize the class if needed
//
//-------------------------------------------------------------------------
LPCTSTR nsWindow::WindowClass()
{
    const LPCTSTR className = "NetscapeWindowClass";

    if (!nsWindow::sIsRegistered) {
        WNDCLASS wc;

//        wc.style            = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
        wc.style            = CS_DBLCLKS;
        wc.lpfnWndProc      = ::DefWindowProc;
        wc.cbClsExtra       = 0;
        wc.cbWndExtra       = 0;
        wc.hInstance        = nsToolkit::mDllInstance;
        wc.hIcon            = ::LoadIcon(NULL, IDI_APPLICATION);
        wc.hCursor          = NULL;
        wc.hbrBackground    = mBrush;
        wc.lpszMenuName     = NULL;
        wc.lpszClassName    = className;
    
        nsWindow::sIsRegistered = ::RegisterClass(&wc);
    }

    return className;
}


//-------------------------------------------------------------------------
//
// return nsWindow styles
//
//-------------------------------------------------------------------------
DWORD nsWindow::WindowStyle()
{
    return WS_OVERLAPPEDWINDOW;
}


//-------------------------------------------------------------------------
//
// return nsWindow extended styles
//
//-------------------------------------------------------------------------
DWORD nsWindow::WindowExStyle()
{
    return WS_EX_WINDOWEDGE;
}


// -----------------------------------------------------------------------
//
// Subclass (or remove the subclass from) this component's nsWindow
//
// -----------------------------------------------------------------------
void nsWindow::SubclassWindow(BOOL bState)
{
  if (NULL != mWnd) {
    NS_PRECONDITION(::IsWindow(mWnd), "Invalid window handle");
    
    if (bState) {
        // change the nsWindow proc
        mPrevWndProc = (WNDPROC)::SetWindowLong(mWnd, GWL_WNDPROC, 
                                                 (LONG)nsWindow::WindowProc);
        NS_ASSERTION(mPrevWndProc, "Null standard window procedure");
        // connect the this pointer to the nsWindow handle
        ::SetWindowLong(mWnd, GWL_USERDATA, (LONG)this);
    } 
    else {
        ::SetWindowLong(mWnd, GWL_WNDPROC, (LONG)mPrevWndProc);
        ::SetWindowLong(mWnd, GWL_USERDATA, (LONG)NULL);
        mPrevWndProc = NULL;
    }
  }
}


//-------------------------------------------------------------------------
//
// WM_DESTROY has been called
//
//-------------------------------------------------------------------------
void nsWindow::OnDestroy()
{
    mOnDestroyCalled = PR_TRUE;

    SubclassWindow(FALSE);
    mWnd = NULL;

    // free GDI objects
    if (mBrush) {
      VERIFY(::DeleteObject(mBrush));
      mBrush = NULL;
    }
    if (mPalette) {
      VERIFY(::DeleteObject(mPalette));
      mPalette = NULL;
    }

    // if we were in the middle of deferred window positioning then
    // free the memory for the multiple-window position structure
    if (mDeferredPositioner) {
      VERIFY(::EndDeferWindowPos(mDeferredPositioner));
      mDeferredPositioner = NULL;
    }

    // release references to children, device context, toolkit, and app shell
    nsBaseWidget::OnDestroy();
 
    // dispatch the event
    if (!mIsDestroying) {
      // dispatching of the event may cause the reference count to drop to 0
      // and result in this object being destroyed. To avoid that, add a reference
      // and then release it after dispatching the event
      AddRef();
      DispatchStandardEvent(NS_DESTROY);
      Release();
    }
}

//-------------------------------------------------------------------------
//
// Move
//
//-------------------------------------------------------------------------
PRBool nsWindow::OnMove(PRInt32 aX, PRInt32 aY)
{            
  nsGUIEvent event;
  InitEvent(event, NS_MOVE);
  event.point.x = aX;
  event.point.y = aY;
  event.eventStructType = NS_GUI_EVENT;

  PRBool result = DispatchWindowEvent(&event);
  NS_RELEASE(event.widget);
  return result;
}

//-------------------------------------------------------------------------
//
// Paint
//
//-------------------------------------------------------------------------
PRBool nsWindow::OnPaint()
{
    nsRect    bounds;
    PRBool result = PR_TRUE;
    PAINTSTRUCT ps;

#ifdef NS_DEBUG
    HRGN debugPaintFlashRegion = NULL;
    HDC debugPaintFlashDC = NULL;

    if (debug_WantPaintFlashing())
    {
      debugPaintFlashRegion = ::CreateRectRgn(0, 0, 0, 0);
      ::GetUpdateRgn(mWnd, debugPaintFlashRegion, TRUE);
      debugPaintFlashDC = ::GetDC(mWnd);
    }
#endif // NS_DEBUG

    HDC hDC = ::BeginPaint(mWnd, &ps);

    // XXX What is this check doing? If it's trying to check for an empty
    // paint rect then use the IsRectEmpty() function...
    if (ps.rcPaint.left || ps.rcPaint.right || ps.rcPaint.top || ps.rcPaint.bottom) {
        // call the event callback 
        if (mEventCallback) 
        {

            nsPaintEvent event;

            InitEvent(event, NS_PAINT);

            nsRect rect(ps.rcPaint.left, 
                        ps.rcPaint.top, 
                        ps.rcPaint.right - ps.rcPaint.left, 
                        ps.rcPaint.bottom - ps.rcPaint.top);
            event.rect = &rect;
            event.eventStructType = NS_PAINT_EVENT;

#ifdef NS_DEBUG
          debug_DumpPaintEvent(stdout,
                               this,
                               &event,
                               "noname",
                               (PRInt32) mWnd);
#endif // NS_DEBUG

            if (NS_OK == nsComponentManager::CreateInstance(kRenderingContextCID, 
                                                            nsnull, 
                                                            nsCOMTypeInfo<nsIRenderingContext>::GetIID(), 
                                                            (void **)&event.renderingContext))
            {
              nsIRenderingContextWin *winrc;

              if (NS_OK == event.renderingContext->QueryInterface(nsCOMTypeInfo<nsIRenderingContextWin>::GetIID(), (void **)&winrc))
              {
                nsDrawingSurface surf;

                //i know all of this seems a little backwards. i'll fix it, i swear. MMP

                if (NS_OK == winrc->CreateDrawingSurface(hDC, surf))
                {
                  event.renderingContext->Init(mContext, surf);
                  result = DispatchWindowEvent(&event);
                  event.renderingContext->DestroyDrawingSurface(surf);
                }

                NS_RELEASE(winrc);
              }

              NS_RELEASE(event.renderingContext);
            }
            else
              result = PR_FALSE;

            NS_RELEASE(event.widget);
        }
    }

    ::EndPaint(mWnd, &ps);

#ifdef NS_DEBUG
    if (debug_WantPaintFlashing())
    {
      ::InvertRgn(debugPaintFlashDC, debugPaintFlashRegion);
      int x;
      for (x = 0; x < 1000000; x++);
      ::InvertRgn(debugPaintFlashDC, debugPaintFlashRegion);
      for (x = 0; x < 1000000; x++);
      ::ReleaseDC(mWnd, debugPaintFlashDC);
      ::DeleteObject(debugPaintFlashRegion);
    }
#endif // NS_DEBUG

    return result;
}


//-------------------------------------------------------------------------
//
// Send a resize message to the listener
//
//-------------------------------------------------------------------------
PRBool nsWindow::OnResize(nsRect &aWindowRect)
{
  // call the event callback 
  if (mEventCallback) {
    nsSizeEvent event;
    InitEvent(event, NS_SIZE);
    event.windowSize = &aWindowRect;
    event.eventStructType = NS_SIZE_EVENT;
    RECT r;
    if (::GetWindowRect(mWnd, &r)) {
      event.mWinWidth  = PRInt32(r.right - r.left);
      event.mWinHeight = PRInt32(r.bottom - r.top);
    } else {
      event.mWinWidth  = 0;
      event.mWinHeight = 0;
    }
    PRBool result = DispatchWindowEvent(&event);
    NS_RELEASE(event.widget);
    return result;
  }

  return PR_FALSE;
}



//-------------------------------------------------------------------------
//
// Deal with all sort of mouse event
//
//-------------------------------------------------------------------------
PRBool nsWindow::DispatchMouseEvent(PRUint32 aEventType, nsPoint* aPoint)
{
  PRBool result = PR_FALSE;

  if (nsnull == mEventCallback && nsnull == mMouseListener) {
    return result;
  }

  nsMouseEvent event;
  InitEvent(event, aEventType, aPoint);

  event.isShift   = IS_VK_DOWN(NS_VK_SHIFT);
  event.isControl = IS_VK_DOWN(NS_VK_CONTROL);
  event.isMeta    = PR_FALSE;
  event.isAlt     = IS_VK_DOWN(NS_VK_ALT);
  event.eventStructType = NS_MOUSE_EVENT;

  //Dblclicks are used to set the click count, then changed to mousedowns
  if (aEventType == NS_MOUSE_LEFT_DOUBLECLICK ||
      aEventType == NS_MOUSE_RIGHT_DOUBLECLICK) {
    event.message = (aEventType == NS_MOUSE_LEFT_DOUBLECLICK) ? 
                     NS_MOUSE_LEFT_BUTTON_DOWN : NS_MOUSE_RIGHT_BUTTON_DOWN;
    event.clickCount = 2;
  }
  else {
    event.clickCount = 1;
  }

  nsPluginEvent pluginEvent;

  switch (aEventType)//~~~
  {
    case NS_MOUSE_LEFT_BUTTON_DOWN:
      pluginEvent.event = WM_LBUTTONDOWN;
      break;
    case NS_MOUSE_LEFT_BUTTON_UP:
      pluginEvent.event = WM_LBUTTONUP;
      break;
    case NS_MOUSE_LEFT_DOUBLECLICK:
      pluginEvent.event = WM_LBUTTONDBLCLK;
      break;
    case NS_MOUSE_RIGHT_BUTTON_DOWN:
      pluginEvent.event = WM_RBUTTONDOWN;
      break;
    case NS_MOUSE_RIGHT_BUTTON_UP:
      pluginEvent.event = WM_RBUTTONUP;
      break;
    case NS_MOUSE_RIGHT_DOUBLECLICK:
      pluginEvent.event = WM_RBUTTONDBLCLK;
      break;
    case NS_MOUSE_MIDDLE_BUTTON_DOWN:
      pluginEvent.event = WM_MBUTTONDOWN;
      break;
    case NS_MOUSE_MIDDLE_BUTTON_UP:
      pluginEvent.event = WM_MBUTTONUP;
      break;
    case NS_MOUSE_MIDDLE_DOUBLECLICK:
      pluginEvent.event = WM_MBUTTONDBLCLK;
      break;
    case NS_MOUSE_MOVE:
      pluginEvent.event = WM_MOUSEMOVE;
      break;
    default:
      break;
  }

  pluginEvent.wParam = 0;
  pluginEvent.wParam |= (event.isShift) ? MK_SHIFT : 0;
  pluginEvent.wParam |= (event.isControl) ? MK_CONTROL : 0;
  pluginEvent.lParam = MAKELONG(event.point.x, event.point.y);

  event.nativeMsg = (void *)&pluginEvent;

  // call the event callback 
  if (nsnull != mEventCallback) {

    result = DispatchWindowEvent(&event);

    if (aEventType == NS_MOUSE_MOVE) {

      // if we are not in mouse cpature mode (mouse down and hold)
      // then use "this" window
      // if we are in mouse capture, then all events are being directed
      // back to the nsWindow doing the capture. So therefore, the detection
      // of whether we are in a new nsWindow is wrong. Meaning this MOUSE_MOVE
      // event hold the captured windows pointer not the one the mouse is over.
      //
      // So we use "WindowFromPoint" to find what window we are over and 
      // set that window into the mouse trailer timer.
      if (!mIsInMouseCapture) {
        MouseTrailer * mouseTrailer = MouseTrailer::GetMouseTrailer(0);
        MouseTrailer::SetMouseTrailerWindow(this);
        mouseTrailer->CreateTimer();
      } else {
        POINT mp;
        DWORD pos = ::GetMessagePos();
        mp.x      = LOWORD(pos);
        mp.y      = HIWORD(pos);

        // OK, now find out if we are still inside
        // the captured native window
        POINT cpos;
        cpos.x = LOWORD(pos);
        cpos.y = HIWORD(pos);

        nsWindow * someWindow = NULL;
        HWND hWnd = ::WindowFromPoint(mp);
        if (hWnd != NULL) {
          ::ScreenToClient(hWnd, &cpos);
          RECT r;
          VERIFY(::GetWindowRect(hWnd, &r));
          if (cpos.x >= r.left && cpos.x <= r.right &&
              cpos.y >= r.top && cpos.y <= r.bottom) {
            // yes we are so we should be able to get a valid window
            // although, strangley enough when we are on the frame part of the
            // window we get right here when in capture mode
            // but this window won't match the capture mode window so
            // we are ok
            someWindow = (nsWindow*)::GetWindowLong(hWnd, GWL_USERDATA);
          } 
        }
        // only set the window into the mouse trailer if we have a good window
        if (nsnull != someWindow)  {
          MouseTrailer * mouseTrailer = MouseTrailer::GetMouseTrailer(0);
          MouseTrailer::SetMouseTrailerWindow(someWindow);
          mouseTrailer->CreateTimer();
        }
      }

      nsRect rect;
      GetBounds(rect);
      rect.x = 0;
      rect.y = 0;

      if (rect.Contains(event.point.x, event.point.y)) {
        if (gCurrentWindow == NULL || gCurrentWindow != this) {
          if ((nsnull != gCurrentWindow) && (!gCurrentWindow->mIsDestroying)) {
            MouseTrailer::IgnoreNextCycle();
            gCurrentWindow->DispatchMouseEvent(NS_MOUSE_EXIT, gCurrentWindow->GetLastPoint());
          }
          gCurrentWindow = this;
          if (!mIsDestroying) {
            gCurrentWindow->DispatchMouseEvent(NS_MOUSE_ENTER);
          }
        }
      } 
    } else if (aEventType == NS_MOUSE_EXIT) {
      if (gCurrentWindow == this) {
        gCurrentWindow = nsnull;
      }
    }
    NS_RELEASE(event.widget);
    return result;
  }

  if (nsnull != mMouseListener) {
    switch (aEventType) {
      case NS_MOUSE_MOVE: {
        result = ConvertStatus(mMouseListener->MouseMoved(event));
        nsRect rect;
        GetBounds(rect);
        if (rect.Contains(event.point.x, event.point.y)) {
          if (gCurrentWindow == NULL || gCurrentWindow != this) {
            gCurrentWindow = this;
          }
        } else {
          //printf("Mouse exit");
        }

      } break;

      case NS_MOUSE_LEFT_BUTTON_DOWN:
      case NS_MOUSE_MIDDLE_BUTTON_DOWN:
      case NS_MOUSE_RIGHT_BUTTON_DOWN:
        result = ConvertStatus(mMouseListener->MousePressed(event));
        break;

      case NS_MOUSE_LEFT_BUTTON_UP:
      case NS_MOUSE_MIDDLE_BUTTON_UP:
      case NS_MOUSE_RIGHT_BUTTON_UP:
        result = ConvertStatus(mMouseListener->MouseReleased(event));
        result = ConvertStatus(mMouseListener->MouseClicked(event));
        break;
    } // switch
  } 
  NS_RELEASE(event.widget);
  return result;
}

//-------------------------------------------------------------------------
//
// Deal with focus messages
//
//-------------------------------------------------------------------------
PRBool nsWindow::DispatchFocus(PRUint32 aEventType)
{
  // call the event callback 
  if (mEventCallback) {
    nsGUIEvent event;
    event.eventStructType = NS_GUI_EVENT;
    InitEvent(event, aEventType);

    //focus and blur event should go to their base widget loc, not current mouse pos
    event.point.x = 0;
    event.point.y = 0;

    nsPluginEvent pluginEvent;

    switch (aEventType)//~~~
    {
      case NS_GOTFOCUS:
        pluginEvent.event = WM_SETFOCUS;
        break;
      case NS_LOSTFOCUS:
        pluginEvent.event = WM_KILLFOCUS;
        break;
      default:
        break;
    }

    event.nativeMsg = (void *)&pluginEvent;

    PRBool result = DispatchWindowEvent(&event);
    NS_RELEASE(event.widget);
    return result;
  }
  return PR_FALSE;
}


//-------------------------------------------------------------------------
//
// Deal with scrollbar messages (actually implemented only in nsScrollbar)
//
//-------------------------------------------------------------------------
PRBool nsWindow::OnScroll(UINT scrollCode, int cPos)
{
    return PR_FALSE;
}


//-------------------------------------------------------------------------
//
// Return the brush used to paint the background of this control 
//
//-------------------------------------------------------------------------
HBRUSH nsWindow::OnControlColor()
{
    return mBrush;
}

//-------------------------------------------------------------------------
//
// return the style for a child nsWindow
//
//-------------------------------------------------------------------------
DWORD ChildWindow::WindowStyle()
{
  //    return WS_CHILD | WS_CLIPCHILDREN | GetBorderStyle(mBorderStyle);
  return WS_CHILD | WS_CLIPCHILDREN | GetWindowType(mWindowType);
}

//-------------------------------------------------------------------------
//
// Deal with all sort of mouse event
//
//-------------------------------------------------------------------------
PRBool ChildWindow::DispatchMouseEvent(PRUint32 aEventType, nsPoint* aPoint)
{
  PRBool result = PR_FALSE;

  if (nsnull == mEventCallback && nsnull == mMouseListener) {
    return result;
  }

  switch (aEventType) {
    case NS_MOUSE_LEFT_BUTTON_DOWN:
    case NS_MOUSE_MIDDLE_BUTTON_DOWN:
    case NS_MOUSE_RIGHT_BUTTON_DOWN:
        CaptureMouse(PR_TRUE);
      break;

    case NS_MOUSE_LEFT_BUTTON_UP:
    case NS_MOUSE_MIDDLE_BUTTON_UP:
    case NS_MOUSE_RIGHT_BUTTON_UP:
        CaptureMouse(PR_FALSE);
      break;

    default:
      break;

  } // switch

  return nsWindow::DispatchMouseEvent(aEventType, aPoint);
}

DWORD nsWindow::GetWindowType(nsWindowType aWindowType)
{
  switch(aWindowType)
  {
    case eWindowType_child:
      return(0);
    break;

    case eWindowType_dialog:
     return(WS_DLGFRAME | DS_3DLOOK);
    break;

    case eWindowType_popup:
      return(0);
    break;

    case eWindowType_toplevel:
      return(0);
    break;

    default:
      NS_ASSERTION(0, "unknown border style");
      return(WS_OVERLAPPEDWINDOW);
  }
}

DWORD nsWindow::GetBorderStyle(nsBorderStyle aBorderStyle)
{
  return 0;
  /*
  switch(aBorderStyle)
  {
    case eBorderStyle_none:
      return(0);
    break;

    case eBorderStyle_dialog:
     return(WS_DLGFRAME | DS_3DLOOK);
    break;

    case eBorderStyle_BorderlessTopLevel:
      return(0);
    break;

    case eBorderStyle_window:
      return(0);
    break;

    default:
      NS_ASSERTION(0, "unknown border style");
      return(WS_OVERLAPPEDWINDOW);
  }
  */
}

NS_METHOD nsWindow::SetTitle(const nsString& aTitle) 
{
  char* title = GetACPString(aTitle);
  if (title) {
    ::SendMessage(mWnd, WM_SETTEXT, (WPARAM)0, (LPARAM)(LPCTSTR)title);
    delete [] title;
  }
  return NS_OK;
} 


PRBool nsWindow::AutoErase()
{
  return(PR_FALSE);
}

NS_METHOD nsWindow::SetMenuBar(nsIMenuBar * aMenuBar) 
{
  mMenuBar = aMenuBar;
  NS_ADDREF(mMenuBar);
  return ShowMenuBar(PR_TRUE);
}

NS_METHOD nsWindow::ShowMenuBar(PRBool aShow)
{
  nsresult rv = NS_ERROR_FAILURE;

  if (aShow) {
    if (mMenuBar) {
      HMENU nativeMenuHandle;
      void  *voidData;
      mMenuBar->GetNativeData(voidData);
      nativeMenuHandle = (HMENU)voidData;

      if (nativeMenuHandle) {
        ::SetMenu(mWnd, nativeMenuHandle);
        rv = NS_OK;
      }
    }
  } else {
    ::SetMenu(mWnd, 0);
    rv = NS_OK;
  }
  return rv;
}

NS_METHOD nsWindow::GetPreferredSize(PRInt32& aWidth, PRInt32& aHeight)
{
  aWidth  = mPreferredWidth;
  aHeight = mPreferredHeight;
  return NS_ERROR_FAILURE;
}

NS_METHOD nsWindow::SetPreferredSize(PRInt32 aWidth, PRInt32 aHeight)
{
  mPreferredWidth  = aWidth;
  mPreferredHeight = aHeight;
  return NS_OK;
}

void
nsWindow::HandleTextEvent(HIMC hIMEContext,PRBool aCheckAttr)
{
  nsTextEvent		event;
  nsPoint			point;
  size_t			unicharSize;
  CANDIDATEFORM		candForm;
  point.x = 0;
  point.y = 0;

  InitEvent(event, NS_TEXT_EVENT, &point);
 
  //
  // convert the composition string text into unicode before it is sent to xp-land
  //
  unicharSize = ::MultiByteToWideChar(mCurrentKeyboardCP,MB_PRECOMPOSED,mIMECompositionString,mIMECompositionStringLength,
	  mIMECompositionUniString,0);

  if (mIMECompositionUniStringSize < (PRInt32)unicharSize) {
	if (mIMECompositionUniString!=NULL) delete [] mIMECompositionUniString;
		mIMECompositionUniString = new PRUnichar[unicharSize+32];
		mIMECompositionUniStringSize = unicharSize+32;
  }
  ::MultiByteToWideChar(mCurrentKeyboardCP,MB_PRECOMPOSED,mIMECompositionString,mIMECompositionStringLength,
	  mIMECompositionUniString,unicharSize);
  mIMECompositionUniString[unicharSize] = (PRUnichar)0;

  //
  // we need to convert the attribute array, which is alligned with the mutibyte text into an array of offsets
  // mapped to the unicode text
  //
  
  if(aCheckAttr) {
     MapDBCSAtrributeArrayToUnicodeOffsets(&(event.rangeCount),&(event.rangeArray));
  } else {
     event.rangeCount = 0;
     event.rangeArray = nsnull;
  }

  event.theText = mIMECompositionUniString;
  event.isShift	= mIsShiftDown;
  event.isControl = mIsControlDown;
  event.isMeta	= PR_FALSE;
  event.isAlt = mIsAltDown;
  event.eventStructType = NS_TEXT_EVENT;

  (void)DispatchWindowEvent(&event);
  NS_RELEASE(event.widget);

  if(event.rangeArray)
     delete [] event.rangeArray;

  //
  // Post process event
  //
  candForm.dwIndex = 0;
  candForm.dwStyle = CFS_CANDIDATEPOS;
  candForm.ptCurrentPos.x = event.theReply.mCursorPosition.x + IME_X_OFFSET;
  candForm.ptCurrentPos.y = event.theReply.mCursorPosition.y + IME_Y_OFFSET;

  printf("Candidate window position: x=%d, y=%d\n",candForm.ptCurrentPos.x,candForm.ptCurrentPos.y);

  ::ImmSetCandidateWindow(hIMEContext,&candForm);

}

void
nsWindow::HandleStartComposition(HIMC hIMEContext)
{
	nsCompositionEvent	event;
	nsPoint				point;
	CANDIDATEFORM		candForm;

	point.x	= 0;
	point.y = 0;

	InitEvent(event,NS_COMPOSITION_START,&point);
	event.eventStructType = NS_COMPOSITION_START;
	event.compositionMessage = NS_COMPOSITION_START;
	(void)DispatchWindowEvent(&event);

	//
	// Post process event
	//
	candForm.dwIndex = 0;
	candForm.dwStyle = CFS_CANDIDATEPOS;
	candForm.ptCurrentPos.x = event.theReply.mCursorPosition.x + IME_X_OFFSET;
	candForm.ptCurrentPos.y = event.theReply.mCursorPosition.y + IME_Y_OFFSET;
#if defined(DEBUG_tague) || defined(DEBUG_ftang)
	printf("Candidate window position: x=%d, y=%d\n",candForm.ptCurrentPos.x,candForm.ptCurrentPos.y);
#endif
	::ImmSetCandidateWindow(hIMEContext,&candForm);
	NS_RELEASE(event.widget);
}

void
nsWindow::HandleEndComposition(void)
{
	nsCompositionEvent	event;
	nsPoint				point;

	point.x	= 0;
	point.y = 0;

	InitEvent(event,NS_COMPOSITION_END,&point);
	event.eventStructType = NS_COMPOSITION_END;
	event.compositionMessage = NS_COMPOSITION_END;
	(void)DispatchWindowEvent(&event);
	NS_RELEASE(event.widget);
}

static PRUint32 PlatformToNSAttr(PRUint32 aAttr)
{
 switch(aAttr)
 {
    case ATTR_INPUT:
         return NS_TEXTRANGE_RAWINPUT;
    case ATTR_CONVERTED:
         return NS_TEXTRANGE_CONVERTEDTEXT;
    case ATTR_TARGET_NOTCONVERTED:
         return NS_TEXTRANGE_SELECTEDRAWTEXT;
    case ATTR_TARGET_CONVERTED:
         return NS_TEXTRANGE_SELECTEDCONVERTEDTEXT;
    default:
         NS_ASSERTION(PR_FALSE, "unknown attribute");
         return NS_TEXTRANGE_CARETPOSITION;
 }
}
//
// This function converters the composition string (CGS_COMPSTR) into Unicode while mapping the 
//  attribute (GCS_ATTR) string t
void 
nsWindow::MapDBCSAtrributeArrayToUnicodeOffsets(PRUint32* textRangeListLengthResult,nsTextRangeArray* textRangeListResult)
{
	PRUint32	rangePointer;
	PRInt32		ictr;
	PRUint32	uctr;
	size_t		lastUnicodeOffset, substringLength, lastMBCSOffset;
	
	//
	// figure out the ranges from the compclause string
	//
	if (mIMECompClauseStringLength==0) {
		*textRangeListLengthResult = 2;
		*textRangeListResult = new nsTextRange[2];
		(*textRangeListResult)[0].mStartOffset=0;
		substringLength = ::MultiByteToWideChar(mCurrentKeyboardCP,MB_PRECOMPOSED,mIMECompositionString,
								mIMECompositionStringLength,NULL,0);
		(*textRangeListResult)[0].mEndOffset = substringLength;
		(*textRangeListResult)[0].mRangeType = NS_TEXTRANGE_RAWINPUT;
		substringLength = ::MultiByteToWideChar(mCurrentKeyboardCP,MB_PRECOMPOSED,mIMECompositionString,
								mIMECursorPosition,NULL,0);
		(*textRangeListResult)[1].mStartOffset=substringLength;
		(*textRangeListResult)[1].mEndOffset = substringLength;
		(*textRangeListResult)[1].mRangeType = NS_TEXTRANGE_CARETPOSITION;

		
	} else {
		
		*textRangeListLengthResult = 1;
		for(ictr=0;ictr<mIMECompClauseStringLength;ictr++) {
			if (mIMECompClauseString[ictr]!=0x00) 
				(*textRangeListLengthResult)++;
		}

		//
		//  allocate the offset array
		//
		*textRangeListResult = new nsTextRange[*textRangeListLengthResult];

		//
		// figure out the cursor position
		//
		
		substringLength = ::MultiByteToWideChar(mCurrentKeyboardCP,MB_PRECOMPOSED,mIMECompositionString,mIMECursorPosition,NULL,0);
		(*textRangeListResult)[0].mStartOffset=substringLength;
		(*textRangeListResult)[0].mEndOffset = substringLength;
		(*textRangeListResult)[0].mRangeType = NS_TEXTRANGE_CARETPOSITION;


	
		//
		// iterate over the attributes and convert them into unicode 
		lastUnicodeOffset = 0;
		lastMBCSOffset = 0;
		rangePointer = 1;
		for(ictr=0;ictr<mIMECompClauseStringLength;ictr++) {
			if (mIMECompClauseString[ictr]!=0) {
				(*textRangeListResult)[rangePointer].mStartOffset = lastUnicodeOffset;
				substringLength = ::MultiByteToWideChar(mCurrentKeyboardCP,MB_PRECOMPOSED,mIMECompositionString+lastMBCSOffset,
										mIMECompClauseString[ictr]-lastMBCSOffset,NULL,0);
				(*textRangeListResult)[rangePointer].mEndOffset = lastUnicodeOffset + substringLength;
				(*textRangeListResult)[rangePointer].mRangeType = 
                   PlatformToNSAttr(mIMEAttributeString[mIMECompClauseString[ictr]-1]);
				lastUnicodeOffset+= substringLength;
				lastMBCSOffset = mIMECompClauseString[ictr];
				rangePointer++;
			}
		}
	}


}
