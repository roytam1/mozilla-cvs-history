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

#include "nsWindow.h"
#include "nsIFontMetrics.h"
#include "nsIFontCache.h"
#include "nsGUIEvent.h"
#include "nsIRenderingContext.h"
#include "nsIDeviceContext.h"
#include "nsRect.h"
#include "nsTransform2D.h"
#include "nsStringUtil.h"
#include <windows.h>
#include "nsGfxCIID.h"
#include "resource.h"
#include <commctrl.h>

#include "prtime.h"

BOOL nsWindow::sIsRegistered = FALSE;

nsWindow * mCurrentWindow = NULL;

static NS_DEFINE_IID(kIWidgetIID, NS_IWIDGET_IID);


//-------------------------------------------------------------------------
//
// Convert nsEventStatus value to a windows boolean
//
//-------------------------------------------------------------------------

PRBool nsWindow::ConvertStatus(nsEventStatus aStatus)
{
  switch(aStatus) {
    case nsEventStatus_eIgnore:
      return(PR_FALSE);
    break;
    case nsEventStatus_eConsumeNoDefault:
      return(PR_TRUE);
    break;
    case nsEventStatus_eConsumeDoDefault:
      return(PR_FALSE);
    break;
    default:
      NS_ASSERTION(0, "Illegal nsEventStatus enumeration value");
      return(PR_FALSE);
    break;
  }
}

//-------------------------------------------------------------------------
//
// the nsWindow procedure for all nsWindows in this toolkit
//
//-------------------------------------------------------------------------
LRESULT CALLBACK nsWindow::WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
      // Get the window which caused the event and ask it to process the message
    nsWindow *someWindow = (nsWindow*)::GetWindowLong(hWnd, GWL_USERDATA);


      // Re-direct a tab change message destined for it's parent window to the
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

    return ::CallWindowProc((FARPROC)someWindow->GetPrevWindowProc(), hWnd, msg, wParam, lParam);
}


//-------------------------------------------------------------------------
//
// nsWindow constructor
//
//-------------------------------------------------------------------------
nsWindow::nsWindow(nsISupports *aOuter) : nsObject(aOuter)
{
    mWnd           = 0;
    mPrevWndProc   = NULL;
    mChildren      = NULL;
    mEventCallback = NULL;
    mToolkit       = NULL;
    mMouseListener = NULL;
    mEventListener = NULL;
    mBackground    = ::GetSysColor(COLOR_WINDOW);
    mBrush         = ::CreateSolidBrush(NSRGB_2_COLOREF(mBackground));
    mForeground    = ::GetSysColor(COLOR_WINDOWTEXT);
    mPalette       = NULL;
    mCursor        = eCursor_standard;
    mBorderStyle   = eBorderStyle_none;
    mIsShiftDown   = PR_FALSE;
    mIsControlDown = PR_FALSE;
    mIsAltDown     = PR_FALSE;
}


//-------------------------------------------------------------------------
//
// nsWindow destructor
//
//-------------------------------------------------------------------------
nsWindow::~nsWindow()
{
    Destroy();
}


//-------------------------------------------------------------------------
//
// Query interface implementation
//
//-------------------------------------------------------------------------
nsresult nsWindow::QueryObject(const nsIID& aIID, void** aInstancePtr)
{
    nsresult result = nsObject::QueryObject(aIID, aInstancePtr);

    static NS_DEFINE_IID(kIWidgetIID, NS_IWIDGET_IID);
    if (result == NS_NOINTERFACE && aIID.Equals(kIWidgetIID)) {
        *aInstancePtr = (void*) ((nsIWidget*)this);
        AddRef();
        result = NS_OK;
    }

    return result;
}


//-------------------------------------------------------------------------
//
// Create the proper widget
//
//-------------------------------------------------------------------------
void nsWindow::Create(nsIWidget *aParent,
                      const nsRect &aRect,
                      EVENT_CALLBACK aHandleEventFunction,
                      nsIDeviceContext *aContext,
                      nsIToolkit *aToolkit)
{
    if (NULL == mToolkit) {
        if (NULL != aToolkit) {
            mToolkit = (nsToolkit*)aToolkit;
            mToolkit->AddRef();
        }
        else {
            if (NULL != aParent) {
                mToolkit = (nsToolkit*)(aParent->GetToolkit()); // the call AddRef's, we don't have to
            }
            // it's some top level window with no toolkit passed in.
            // Create a default toolkit with the current thread
            else {
                mToolkit = new nsToolkit();
                mToolkit->AddRef();
                mToolkit->Init(PR_GetCurrentThread());
            }
        }

    }

    //
    // Switch to the "main gui thread" if necessary... This method must
    // be executed on the "gui thread"...
    //
    if (!mToolkit->IsGuiThread()) {
        DWORD args[5];
        args[0] = (DWORD)aParent;
        args[1] = (DWORD)&aRect;
        args[2] = (DWORD)aHandleEventFunction;
        args[3] = (DWORD)aContext;
        args[4] = (DWORD)aToolkit;
        MethodInfo info(this, nsWindow::CREATE, 5, args);
        mToolkit->CallMethod(&info);
        return;
    }

    // save the event callback function
    mEventCallback = aHandleEventFunction;

    // keep a reference to the toolkit object
    if (aContext) {
        mContext = aContext;
        mContext->AddRef();
    }
    else {
      nsresult  res;

      static NS_DEFINE_IID(kDeviceContextCID, NS_DEVICE_CONTEXT_CID);
      static NS_DEFINE_IID(kDeviceContextIID, NS_IDEVICE_CONTEXT_IID);

      res = NSRepository::CreateInstance(kDeviceContextCID, nsnull, kDeviceContextIID, (void **)&mContext);

      if (NS_OK == res)
        mContext->Init();
    }

    mWnd = ::CreateWindowEx(WindowExStyle(),
                            WindowClass(),
                            "",
                            WindowStyle(),
                            aRect.x,
                            aRect.y,
                            aRect.width,
                            aRect.height,
                            (aParent) ? (HWND)aParent->GetNativeData(NS_NATIVE_WINDOW):
                                        (HWND)NULL,
                            NULL,
                            nsToolkit::mDllInstance,
                            NULL);
    
    VERIFY(mWnd);
    if (aParent) {
        aParent->AddChild(this);
    }

    // Force cursor to default setting
    mCursor = eCursor_select;
    SetCursor(eCursor_standard);

    // call the event callback to notify about creation
    if (mEventCallback) {
        nsGUIEvent event;
        event.widget = this;

        DWORD pos = ::GetMessagePos();
        POINT cpos;

        cpos.x = LOWORD(pos);
        cpos.y = HIWORD(pos);

        ::ScreenToClient(mWnd, &cpos);

        event.point.x = cpos.x;
        event.point.y = cpos.y;

        event.time = ::GetMessageTime();

        // 
        event.message = NS_CREATE;
        (*mEventCallback)(&event);
    }

    SubclassWindow(TRUE);
}


//-------------------------------------------------------------------------
//
// create with a native parent
//
//-------------------------------------------------------------------------
void nsWindow::Create(nsNativeWindow aParent,
                         const nsRect &aRect,
                         EVENT_CALLBACK aHandleEventFunction,
                         nsIDeviceContext *aContext,
                         nsIToolkit *aToolkit)
{

    if (NULL == mToolkit) {
        if (NULL != aToolkit) {
            mToolkit = (nsToolkit*)aToolkit;
            mToolkit->AddRef();
        }
        else {
            mToolkit = new nsToolkit();
            mToolkit->AddRef();
            mToolkit->Init(PR_GetCurrentThread());
        }

    }

    //
    // Switch to the "main gui thread" if necessary... This method must
    // be executed on the "gui thread"...
    //
    if (!mToolkit->IsGuiThread()) {
        DWORD args[5];
        args[0] = (DWORD)aParent;
        args[1] = (DWORD)&aRect;
        args[2] = (DWORD)aHandleEventFunction;
        args[3] = (DWORD)aContext;
        args[4] = (DWORD)aToolkit;
        MethodInfo info(this, nsWindow::CREATE_NATIVE, 5, args);
        mToolkit->CallMethod(&info);
        return;
    }

    // save the event callback function
    mEventCallback = aHandleEventFunction;

    // keep a reference to the toolkit object
    if (aContext) {
        mContext = aContext;
        mContext->AddRef();
    }
    else {
      nsresult  res;

      static NS_DEFINE_IID(kDeviceContextCID, NS_DEVICE_CONTEXT_CID);
      static NS_DEFINE_IID(kDeviceContextIID, NS_IDEVICE_CONTEXT_IID);

      res = NSRepository::CreateInstance(kDeviceContextCID, nsnull, kDeviceContextIID, (void **)&mContext);

      if (NS_OK == res)
        mContext->Init();
    }

    mWnd = ::CreateWindowEx(WindowExStyle(),
                            WindowClass(),
                            "",
                            WindowStyle(),
                            aRect.x,
                            aRect.y,
                            aRect.width,
                            aRect.height,
                            aParent,
                            NULL,
                            nsToolkit::mDllInstance,
                            NULL);
    
    VERIFY(mWnd);

    // call the event callback to notify about creation
    if (mEventCallback) {
        nsGUIEvent event;
        event.widget = this;

        DWORD pos = ::GetMessagePos();
        POINT cpos;

        cpos.x = LOWORD(pos);
        cpos.y = HIWORD(pos);

        ::ScreenToClient(mWnd, &cpos);

        event.point.x = cpos.x;
        event.point.y = cpos.y;

        event.time = ::GetMessageTime();

        // 
        event.message = NS_CREATE;
        (*mEventCallback)(&event);
    }

    SubclassWindow(TRUE);
}


//-------------------------------------------------------------------------
//
// Close this nsWindow
//
//-------------------------------------------------------------------------
void nsWindow::Destroy()
{
    //
    // Switch to the "main gui thread" if necessary... This method must
    // be executed on the "gui thread"...
    //
    if (!mToolkit->IsGuiThread()) {
        MethodInfo info(this, nsWindow::DESTROY);
        mToolkit->CallMethod(&info);
        return;
    }

    // destroy the nsWindow 
    if (mWnd) {
        // destroy the nsWindow
        mEventCallback = nsnull;  // prevent the widget from causing additional events
        VERIFY(::DestroyWindow(mWnd));
    }

    if (mPalette) {
        VERIFY(::DeleteObject(mPalette));
    }
}


//-------------------------------------------------------------------------
//
// Get this nsWindow parent
//
//-------------------------------------------------------------------------
nsIWidget* nsWindow::GetParent(void)
{
    nsIWidget* widget = NULL;
    if (mWnd) {
        HWND parent = ::GetParent(mWnd);
        if (parent) {
            nsIWidget* widget = (nsIWidget*)::GetWindowLong(mWnd, GWL_USERDATA);
            widget->AddRef();
        }
    }

    return widget;
}


//-------------------------------------------------------------------------
//
// Get this nsWindow's list of children
//
//-------------------------------------------------------------------------
nsIEnumerator* nsWindow::GetChildren()
{
    if (mChildren) {
        mChildren->Reset();
        //mChildren->AddRef();

        Enumerator * children = new Enumerator();
        nsISupports   * next = mChildren->Next();
        if (next) {
          nsIWidget *widget;
          if (NS_OK == next->QueryInterface(kIWidgetIID, (void**)&widget)) {
            children->Append(widget);
            //NS_RELEASE(widget);
          }
        }

        return (nsIEnumerator*)children;
    }

    return NULL;
}


//-------------------------------------------------------------------------
//
// Add a child to the list of children
//
//-------------------------------------------------------------------------
void nsWindow::AddChild(nsIWidget* aChild)
{
    if (!mChildren) {
        mChildren = new Enumerator();
    }

    mChildren->Append(aChild);
}


//-------------------------------------------------------------------------
//
// Remove a child from the list of children
//
//-------------------------------------------------------------------------
void nsWindow::RemoveChild(nsIWidget* aChild)
{
    if (mChildren) {
        mChildren->Remove(aChild);
    }
}


//-------------------------------------------------------------------------
//
// Hide or show this component
//
//-------------------------------------------------------------------------
void nsWindow::Show(PRBool bState)
{
    if (mWnd) {
        if (bState) {
            ::SetWindowPos(mWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_SHOWWINDOW);
        }
        else
            ::ShowWindow(mWnd, SW_HIDE);
    }
}

//-------------------------------------------------------------------------
//
// Move this component
//
//-------------------------------------------------------------------------
void nsWindow::Move(PRUint32 aX, PRUint32 aY)
{
    if (mWnd) {
        VERIFY(::SetWindowPos(mWnd, NULL, aX, aY, 0, 0, 
                                SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOSIZE));
    }
}

//-------------------------------------------------------------------------
//
// Resize this component
//
//-------------------------------------------------------------------------
void nsWindow::Resize(PRUint32 aWidth, PRUint32 aHeight)
{
    if (mWnd) {
        VERIFY(::SetWindowPos(mWnd, NULL, 0, 0, aWidth, aHeight, 
                                SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE));
    }
}

    
//-------------------------------------------------------------------------
//
// Resize this component
//
//-------------------------------------------------------------------------
void nsWindow::Resize(PRUint32 aX, PRUint32 aY, PRUint32 aWidth, PRUint32 aHeight)
{
    if (mWnd) {
        VERIFY(::SetWindowPos(mWnd, NULL, aX, aY, aWidth, aHeight, 
                                SWP_NOZORDER | SWP_NOACTIVATE));
    }
}

    
//-------------------------------------------------------------------------
//
// Enable/disable this component
//
//-------------------------------------------------------------------------
void nsWindow::Enable(PRBool bState)
{
    if (mWnd) {
        ::EnableWindow(mWnd, bState);
    }
}

    
//-------------------------------------------------------------------------
//
// Give the focus to this component
//
//-------------------------------------------------------------------------
void nsWindow::SetFocus(void)
{
    //
    // Switch to the "main gui thread" if necessary... This method must
    // be executed on the "gui thread"...
    //
    if (!mToolkit->IsGuiThread()) {
        MethodInfo info(this, nsWindow::SET_FOCUS);
        mToolkit->CallMethod(&info);
        return;
    }

    if (mWnd) {
        ::SetFocus(mWnd);
    }
}

    
//-------------------------------------------------------------------------
//
// Get this component dimension
//
//-------------------------------------------------------------------------
void nsWindow::GetBounds(nsRect &aRect)
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
            ::ScreenToClient(parent, (LPPOINT)&r);
        }
        aRect.x = r.left;
        aRect.y = r.top;
    }
}

    
//-------------------------------------------------------------------------
//
// Get the foreground color
//
//-------------------------------------------------------------------------
nscolor nsWindow::GetForegroundColor(void)
{
    return mForeground;
}

    
//-------------------------------------------------------------------------
//
// Set the foreground color
//
//-------------------------------------------------------------------------
void nsWindow::SetForegroundColor(const nscolor &aColor)
{
    mForeground = aColor;
}

    
//-------------------------------------------------------------------------
//
// Get the background color
//
//-------------------------------------------------------------------------
nscolor nsWindow::GetBackgroundColor(void)
{
    return mBackground;
}

    
//-------------------------------------------------------------------------
//
// Set the background color
//
//-------------------------------------------------------------------------
void nsWindow::SetBackgroundColor(const nscolor &aColor)
{
    mBackground = aColor;

    ::DeleteObject(mBrush);
    mBrush = ::CreateSolidBrush(NSRGB_2_COLOREF(mBackground));
    SetClassLong(mWnd, GCL_HBRBACKGROUND, (LONG)mBrush);
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
void nsWindow::SetFont(const nsFont &aFont)
{
    nsIFontCache* fontCache = mContext->GetFontCache();
    nsIFontMetrics* metrics = fontCache->GetMetricsFor(aFont);
    HFONT hfont = metrics->GetFontHandle();

      // Draw in the new font
    ::SendMessage(mWnd, WM_SETFONT, (WPARAM)hfont, (LPARAM)0); 
    NS_RELEASE(metrics);
    NS_RELEASE(fontCache);
}

    
//-------------------------------------------------------------------------
//
// Get this component cursor
//
//-------------------------------------------------------------------------
nsCursor nsWindow::GetCursor()
{
    return mCursor;
}

    
//-------------------------------------------------------------------------
//
// Set this component cursor
//
//-------------------------------------------------------------------------

#define DLLQUOTE(x) #x
#define DLLNAME(x) DLLQUOTE(x)

void nsWindow::SetCursor(nsCursor aCursor)
{
  // Only change cursor if it's changing
  if (aCursor != mCursor) {
    HCURSOR newCursor = NULL;

    switch(aCursor) {
    case eCursor_select:
      newCursor = ::LoadCursor(NULL, IDC_IBEAM);
      break;
      
    case eCursor_wait:
      newCursor = ::LoadCursor(NULL, IDC_WAIT);
      break;

    case eCursor_hyperlink: {
      HMODULE hm = ::GetModuleHandle(DLLNAME(NS_DLLNAME));
      newCursor = ::LoadCursor(hm, MAKEINTRESOURCE(IDC_SELECTANCHOR));
      break;
    }

    case eCursor_standard:
      newCursor = ::LoadCursor(NULL, IDC_ARROW);
      break;

    default:
      NS_ASSERTION(0, "Invalid cursor type");
      break;
    }

    if (NULL != newCursor) {
      mCursor = aCursor;
      HCURSOR oldCursor = ::SetCursor(newCursor);
    }
  }
}
    
//-------------------------------------------------------------------------
//
// Invalidate this component visible area
//
//-------------------------------------------------------------------------
void nsWindow::Invalidate(PRBool aIsSynchronous)
{
    if (mWnd) {
        VERIFY(::InvalidateRect(mWnd, NULL, TRUE));
        if (aIsSynchronous) {
          VERIFY(::UpdateWindow(mWnd));
        }
    }
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
            return (void*)mWnd;
        case NS_NATIVE_GRAPHIC:
            return (void*)::GetDC(mWnd);
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
nsRect  bounds;

    nsIRenderingContext *renderingCtx = NULL;
    if (mWnd) {
        HDC aDC = ::GetDC(mWnd);
        nsresult  res;

        static NS_DEFINE_IID(kRenderingContextCID, NS_RENDERING_CONTEXT_CID);
        static NS_DEFINE_IID(kRenderingContextIID, NS_IRENDERING_CONTEXT_IID);

        res = NSRepository::CreateInstance(kRenderingContextCID, nsnull, kRenderingContextIID, (void **)&renderingCtx);

        if (NS_OK == res)
          renderingCtx->Init(mContext, (nsDrawingSurface)aDC);

        NS_ASSERTION(NULL != renderingCtx, "Null rendering context");
    }

    return renderingCtx;
}

//-------------------------------------------------------------------------
//
// Return the toolkit this widget was created on
//
//-------------------------------------------------------------------------
nsIToolkit* nsWindow::GetToolkit()
{
    if (NULL != mToolkit) {
        mToolkit->AddRef();
    }

    return mToolkit;
}


//-------------------------------------------------------------------------
//
// Set the colormap of the window
//
//-------------------------------------------------------------------------
void nsWindow::SetColorMap(nsColorMap *aColorMap)
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
}

//-------------------------------------------------------------------------
//
// Return the used device context
//
//-------------------------------------------------------------------------
nsIDeviceContext* nsWindow::GetDeviceContext() 
{ 
    if (mContext) {
        mContext->AddRef();
    }

    return mContext; 
}


//-------------------------------------------------------------------------
//
// Scroll the bits of a window
//
//-------------------------------------------------------------------------
void nsWindow::Scroll(PRInt32 aDx, PRInt32 aDy, nsRect *aClipRect)
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
                   NULL, NULL, SW_INVALIDATE);
  ::UpdateWindow(mWnd);
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
            NS_ASSERTION(info->nArgs == 5, "Wrong number of arguments to CallMethod");
            Create((nsIWidget*)(info->args[0]), 
                        (nsRect&)*(nsRect*)(info->args[1]), 
                        (EVENT_CALLBACK)(info->args[2]), 
                        (nsIDeviceContext*)(info->args[3]),
                        (nsIToolkit*)(info->args[4]));
            break;

        case nsWindow::CREATE_NATIVE:
            NS_ASSERTION(info->nArgs == 5, "Wrong number of arguments to CallMethod");
            Create((nsNativeWindow)(info->args[0]), 
                        (nsRect&)*(nsRect*)(info->args[1]), 
                        (EVENT_CALLBACK)(info->args[2]), 
                        (nsIDeviceContext*)(info->args[3]),
                        (nsIToolkit*)(info->args[4]));
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
//
// OnKey
//
//-------------------------------------------------------------------------
PRBool nsWindow::OnKey(PRUint32 aEventType, PRUint32 aKeyCode)
{
    PRBool result = PR_TRUE;
	
    // call the event callback 
    if (mEventCallback) {
      nsKeyEvent event;
      event.widget = this;

      DWORD pos = ::GetMessagePos();
      POINT cpos;

      cpos.x = LOWORD(pos);
      cpos.y = HIWORD(pos);

      ::ScreenToClient(mWnd, &cpos);

      event.point.x = cpos.x;
      event.point.y = cpos.y;

      event.time      = ::GetMessageTime();
      event.message   = aEventType;
      event.keyCode   = aKeyCode;
      event.isShift   = mIsShiftDown;
      event.isControl = mIsControlDown;
      event.isAlt     = mIsAltDown;
      result = ConvertStatus((*mEventCallback)(&event));
    }

    return result;
}


//-------------------------------------------------------------------------
//
// Process all nsWindows messages
//
//-------------------------------------------------------------------------
PRBool nsWindow::ProcessMessage(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *aRetValue)
{
    PRBool result = PR_FALSE; // call the default nsWindow proc

            
    *aRetValue = 0;

    switch (msg) {

        
        case WM_NOTIFY:
            // TAB change
        {
          LPNMHDR pnmh = (LPNMHDR) lParam;

          if (pnmh->code == TCN_SELCHANGE) {
            DispatchEventToCallback(NS_TABCHANGE);
            result = PR_TRUE;
          }
        }
        break;

        case WM_DESTROY:
            // clean up.
            OnDestroy();
            result = PR_TRUE;
            if (nsnull != mEventListener) {
              printf("Destroy for window called\n");
              DispatchEvent(NS_DESTROY);
            }
            break;

        case WM_PAINT:
            result = OnPaint();
            if (nsnull != mEventListener) {
              DispatchEvent(NS_PAINT);
            }
            break;

        case WM_KEYUP: 
            if (wParam == NS_VK_SHIFT) {
              mIsShiftDown = PR_FALSE;
            }
            if (wParam == NS_VK_CONTROL) {
              mIsControlDown = PR_FALSE;
            }
            if (wParam == NS_VK_ALT) {
              mIsAltDown = PR_FALSE;
            }
            result = OnKey(NS_KEY_UP, wParam);
            break;

        case WM_KEYDOWN:
            if (wParam == NS_VK_SHIFT) {
              mIsShiftDown = PR_TRUE;
            }
            if (wParam == NS_VK_CONTROL) {
              mIsControlDown = PR_TRUE;
            }
            if (wParam == NS_VK_ALT) {
              mIsAltDown = PR_TRUE;
            }
            result = OnKey(NS_KEY_DOWN, wParam);
            break;

        // say we've dealt with erase background
        case WM_ERASEBKGND:
            *aRetValue = 1;
            result = PR_TRUE;
            break;

        case WM_MOUSEMOVE:
            result = DispatchMouseEvent(NS_MOUSE_MOVE);
            break;

        case WM_LBUTTONDOWN:
            result = DispatchMouseEvent(NS_MOUSE_LEFT_BUTTON_DOWN);
            break;

        case WM_LBUTTONUP:
            result = DispatchMouseEvent(NS_MOUSE_LEFT_BUTTON_UP);
            break;

        case WM_LBUTTONDBLCLK:
            result = DispatchMouseEvent(NS_MOUSE_LEFT_BUTTON_DOWN);
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
            result = DispatchMouseEvent(NS_MOUSE_RIGHT_BUTTON_DOWN);
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

        case WM_CTLCOLORBTN:
        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORLISTBOX:
        case WM_CTLCOLORSCROLLBAR:
        case WM_CTLCOLORSTATIC:
	          if (lParam) {
              nsWindow* control = (nsWindow*)::GetWindowLong((HWND)lParam, GWL_USERDATA);
		          if (control) {
                HDC hDC = (HDC)wParam;
                ::SetBkColor (hDC, mBackground);
                ::SetTextColor(hDC, mForeground);
		            *aRetValue = (LPARAM)control->OnControlColor();
		          }
	          }
    
            result = PR_TRUE;
            break;

        case WM_SETFOCUS:
            result = DispatchFocus(NS_GOTFOCUS);
            if (nsnull != mEventListener) {
              DispatchEvent(NS_GOTFOCUS);
            }
            break;

        case WM_KILLFOCUS:
            result = DispatchFocus(NS_LOSTFOCUS);
            if (nsnull != mEventListener) {
              DispatchEvent(NS_LOSTFOCUS);
            }
            break;

        case WM_WINDOWPOSCHANGED: 
        {
            WINDOWPOS *wp = (LPWINDOWPOS)lParam;
            nsRect rect(wp->x, wp->y, wp->cx, wp->cy);
            result = OnResize(rect);
            if (nsnull != mEventListener) {
              DispatchEvent(NS_SIZE);
            }
            break;
        }
        case WM_QUERYNEWPALETTE:
            if (mPalette) {
                HDC hDC = ::GetDC(mWnd);
                HPALETTE hOldPal = ::SelectPalette(hDC, mPalette, 0);
                
                int i = ::RealizePalette(hDC);

                ::SelectPalette(hDC, hOldPal, 0);
                ::ReleaseDC(mWnd, hDC);

                if (i) {
                    ::InvalidateRect(mWnd, (LPRECT)(NULL), 1);
                    result = PR_TRUE;
                }
                else {
                    result = PR_FALSE;
                }
            }
            else {
                result = PR_FALSE;
            }
            break;
       case WM_PALETTECHANGED:
           if (mPalette && (HWND)wParam != mWnd) {
                HDC hDC = ::GetDC(mWnd);
                HPALETTE hOldPal = ::SelectPalette(hDC, mPalette, 0);
                
                int i = ::RealizePalette(hDC);

                if (i) {
                    ::InvalidateRect(mWnd, (LPRECT)(NULL), 1);
                }

                ::SelectPalette(hDC, hOldPal, 0);
                ::ReleaseDC(mWnd, hDC);
           }
           result = PR_TRUE;
           break;

    }

    return result;
}



//-------------------------------------------------------------------------
//
// return the window class name and initialize the class if needed
//
//-------------------------------------------------------------------------
LPCTSTR nsWindow::WindowClass()
{
    if (!nsWindow::sIsRegistered) {
        WNDCLASS wc;

        wc.style            = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
        wc.lpfnWndProc      = ::DefWindowProc;
        wc.cbClsExtra       = 0;
        wc.cbWndExtra       = 0;
        wc.hInstance        = nsToolkit::mDllInstance;
        wc.hIcon            = ::LoadIcon(NULL, IDI_APPLICATION);
        wc.hCursor          = NULL;
        wc.hbrBackground    = NULL; 
        wc.lpszMenuName     = NULL;
        wc.lpszClassName    = "NetscapeWindowClass";
    
        nsWindow::sIsRegistered = ::RegisterClass(&wc);
    }

    return "NetscapeWindowClass";
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
    return WS_EX_CLIENTEDGE;
}


// -----------------------------------------------------------------------
//
// Subclass (or remove the subclass from) this component's nsWindow
//
// -----------------------------------------------------------------------
void nsWindow::SubclassWindow(BOOL bState)
{
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
        (void) ::SetWindowLong(mWnd, GWL_WNDPROC, (LONG)mPrevWndProc);
        mPrevWndProc = NULL;
    }
}


//-------------------------------------------------------------------------
//
// WM_DESTROY has been called
//
//-------------------------------------------------------------------------
void nsWindow::OnDestroy()
{
    SubclassWindow(FALSE);

    if (mBrush)
      ::DeleteObject(mBrush);

    // disconnect from the parent
    /* 
    nsIWidget *parent = GetParent();
    if (parent) {
        parent->RemoveChild(this);
    }*/

    mWnd = 0;

    // release children (I don't think we need this but...)
    if (mChildren) {
        NS_RELEASE(mChildren);
        mChildren = NULL;
    }

    // get the pixels to twips info before the device context is released
    if (mContext) {
        NS_RELEASE(mContext);
        mContext = 0;
    }

    if (NULL != mToolkit) {
        NS_RELEASE(mToolkit);
        mToolkit = NULL;
    }

    // call the event callback 
    if (mEventCallback) {
        nsGUIEvent event;
        event.widget = this;
        
        DWORD pos = ::GetMessagePos();
        POINT cpos;

        cpos.x = LOWORD(pos);
        cpos.y = HIWORD(pos);

        ::ScreenToClient(mWnd, &cpos);

        event.point.x = cpos.x;
        event.point.y = cpos.y;

        event.time = ::GetMessageTime();
        event.message = NS_DESTROY;

        (*mEventCallback)(&event);
    }
    if (nsnull != mEventListener) {
      DispatchEvent(NS_DESTROY);
    }
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
    HDC hDC = ::BeginPaint(mWnd, &ps);

    if (ps.rcPaint.left || ps.rcPaint.right || ps.rcPaint.top || ps.rcPaint.bottom) {

        // call the event callback 
        if (mEventCallback) {
            nsPaintEvent event;
            event.widget = this;

            DWORD pos = ::GetMessagePos();
            POINT cpos;

            cpos.x = LOWORD(pos);
            cpos.y = HIWORD(pos);

            ::ScreenToClient(mWnd, &cpos);

            event.point.x = cpos.x;
            event.point.y = cpos.y;

            event.time = ::GetMessageTime();
            event.message = NS_PAINT;

            nsRect rect(ps.rcPaint.left, 
                        ps.rcPaint.top, 
                        ps.rcPaint.right - ps.rcPaint.left, 
                        ps.rcPaint.bottom - ps.rcPaint.top);
            event.rect = &rect;

            static NS_DEFINE_IID(kRenderingContextCID, NS_RENDERING_CONTEXT_CID);
            static NS_DEFINE_IID(kRenderingContextIID, NS_IRENDERING_CONTEXT_IID);

            if (NS_OK == NSRepository::CreateInstance(kRenderingContextCID, nsnull, kRenderingContextIID, (void **)&event.renderingContext))
            {
              event.renderingContext->Init(mContext, (nsDrawingSurface)hDC);
              result = ConvertStatus((*mEventCallback)(&event));

              NS_RELEASE(event.renderingContext);
            }
            else
              result = PR_FALSE;
        }
    }

    ::EndPaint(mWnd, &ps);

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
        event.widget = this;
        event.message = NS_SIZE;
        
        // get the message position in client coordinates and in twips
        DWORD pos = ::GetMessagePos();
        POINT cpos;

        cpos.x = LOWORD(pos);
        cpos.y = HIWORD(pos);

        ::ScreenToClient(mWnd, &cpos);

        event.point.x = cpos.x;
        event.point.y = cpos.y;

        event.time = ::GetMessageTime();

        event.windowSize = &aWindowRect;
        return ConvertStatus((*mEventCallback)(&event));
    }

    return PR_FALSE;
}

//-------------------------------------------------------------------------
//
// Invokes ProcessEvent method on Event Listener object
//
//-------------------------------------------------------------------------
PRBool nsWindow::DispatchEvent(PRUint32 aEventType)
{
    if (nsnull == mEventListener) {
      return PR_FALSE;
    }

    nsGUIEvent event;
    event.widget = this;
    
    // get the message position in client coordinates and in twips
    DWORD pos = ::GetMessagePos();
    POINT cpos;

    cpos.x = LOWORD(pos);
    cpos.y = HIWORD(pos);

    ::ScreenToClient(mWnd, &cpos);

    event.point.x = cpos.x;
    event.point.y = cpos.y;

    event.time = ::GetMessageTime();
    event.message = aEventType;

    return ConvertStatus(mEventListener->ProcessEvent(event));
}


//-------------------------------------------------------------------------
//
// Invokes ProcessEvent method on Event Listener object
//
//-------------------------------------------------------------------------
PRBool nsWindow::DispatchEventToCallback(PRUint32 aEventType)
{
   // call the event callback 
    if (mEventCallback) {
        nsGUIEvent event;
        event.widget = this;
        
        DWORD pos = ::GetMessagePos();
        POINT cpos;

        cpos.x = LOWORD(pos);
        cpos.y = HIWORD(pos);

        ::ScreenToClient(mWnd, &cpos);

        event.point.x = cpos.x;
        event.point.y = cpos.y;

        event.time = ::GetMessageTime();
        event.message = aEventType;

        return(ConvertStatus((*mEventCallback)(&event)));
    }
    else
        return(PR_FALSE);
}


//-------------------------------------------------------------------------
//
// Deal with all sort of mouse event
//
//-------------------------------------------------------------------------
PRBool nsWindow::DispatchMouseEvent(PRUint32 aEventType)
{
  PRBool result = PR_FALSE;

  if (nsnull == mEventCallback && nsnull == mMouseListener) {
    return result;
  }

 // nsMouseEvent event;
  nsGUIEvent event;
  event.widget = this;
  
  // get the message position in client coordinates and in twips
  DWORD pos = ::GetMessagePos();
  POINT cpos;

  cpos.x = LOWORD(pos);
  cpos.y = HIWORD(pos);

  ::ScreenToClient(mWnd, &cpos);

  event.point.x = cpos.x;
  event.point.y = cpos.y;
  event.time = ::GetMessageTime();
  event.message = aEventType;

  // call the event callback 
  if (nsnull != mEventCallback) {
    result = ConvertStatus((*mEventCallback)(&event));

    //printf("**result=%d%\n",result);
    if (aEventType == NS_MOUSE_MOVE) {

      MouseTrailer * mouseTrailer = MouseTrailer::GetMouseTrailer(0);
      MouseTrailer::SetMouseTrailerWindow(this);
      mouseTrailer->CreateTimer();

      nsRect rect;
      GetBounds(rect);
      rect.x = 0;
      rect.y = 0;
      //printf("Rect[%d, %d, %d, %d]  Point[%d,%d]\n", rect.x, rect.y, rect.width, rect.height, event.position.x, event.position.y);
      //printf("mCurrentWindow 0x%X\n", mCurrentWindow);

      if (rect.Contains(event.point.x, event.point.y)) {
        if (mCurrentWindow == NULL || mCurrentWindow != this) {
          if (nsnull != mCurrentWindow) {
            mCurrentWindow->DispatchMouseEvent(NS_MOUSE_EXIT);
          }
          mCurrentWindow = this;
          mCurrentWindow->DispatchMouseEvent(NS_MOUSE_ENTER);
        }
      } 
    } else if (aEventType == NS_MOUSE_EXIT) {
      if (mCurrentWindow == this) {
        mCurrentWindow = nsnull;
      }
    }

    return result;
  }

  if (nsnull != mMouseListener) {
    switch (aEventType) {
      case NS_MOUSE_MOVE: {
        result = ConvertStatus(mMouseListener->MouseMoved(event));
        nsRect rect;
        GetBounds(rect);
        if (rect.Contains(event.point.x, event.point.y)) {
          if (mCurrentWindow == NULL || mCurrentWindow != this) {
            printf("Mouse enter");
            mCurrentWindow = this;
          }
        } else {
          printf("Mouse exit");
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
        event.widget = this;
        
        DWORD pos = ::GetMessagePos();
        POINT cpos;

        cpos.x = LOWORD(pos);
        cpos.y = HIWORD(pos);

        ::ScreenToClient(mWnd, &cpos);

        event.point.x = cpos.x;
        event.point.y = cpos.y;

        event.time = ::GetMessageTime();
        event.message = aEventType;

        return ConvertStatus((*mEventCallback)(&event));
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
// Constructor
//
//-------------------------------------------------------------------------
#define INITIAL_SIZE        2

nsWindow::Enumerator::Enumerator()
{
    mRefCnt = 1;
    mArraySize = INITIAL_SIZE;
    mChildrens = (nsIWidget**)new DWORD[mArraySize];
    memset(mChildrens, 0, sizeof(DWORD) * mArraySize);
    mCurrentPosition = 0;
}


//-------------------------------------------------------------------------
//
// Destructor
//
//-------------------------------------------------------------------------
nsWindow::Enumerator::~Enumerator()
{   
    if (mChildrens) {
        //for (int i = 0; mChildrens[i] && i < mArraySize; i++) {
        //    NS_RELEASE(mChildrens[i]);
        //}

        delete[] mChildrens;
    }
}

//
// nsISupports implementation macro
//
NS_IMPL_ISUPPORTS(nsWindow::Enumerator, NS_IENUMERATOR_IID);

//-------------------------------------------------------------------------
//
// Get enumeration next element. Return null at the end
//
//-------------------------------------------------------------------------
nsISupports* nsWindow::Enumerator::Next()
{
    if (mCurrentPosition < mArraySize && mChildrens[mCurrentPosition]) {
        mChildrens[mCurrentPosition]->AddRef();
        return mChildrens[mCurrentPosition++];
    }

    return NULL;
}


//-------------------------------------------------------------------------
//
// Reset enumerator internal pointer to the beginning
//
//-------------------------------------------------------------------------
void nsWindow::Enumerator::Reset()
{
    mCurrentPosition = 0;
}


//-------------------------------------------------------------------------
//
// Append an element 
//
//-------------------------------------------------------------------------
void nsWindow::Enumerator::Append(nsIWidget* aWidget)
{
    NS_PRECONDITION(aWidget, "Null widget");
    if (aWidget) {
        int pos;
        for (pos = 0; pos < mArraySize && mChildrens[pos]; pos++);
        if (pos == mArraySize) {
            GrowArray();
        }
        mChildrens[pos] = aWidget;
        aWidget->AddRef();
    }
}


//-------------------------------------------------------------------------
//
// Remove an element 
//
//-------------------------------------------------------------------------
void nsWindow::Enumerator::Remove(nsIWidget* aWidget)
{
    int pos;
    for(pos = 0; mChildrens[pos] && (mChildrens[pos] != aWidget); pos++);
    if (mChildrens[pos] == aWidget) {
        NS_RELEASE(aWidget);
        memcpy(mChildrens + pos, mChildrens + pos + 1, mArraySize - pos - 1);
    }

}


//-------------------------------------------------------------------------
//
// Grow the size of the children array
//
//-------------------------------------------------------------------------
void nsWindow::Enumerator::GrowArray()
{
    mArraySize <<= 1;
    nsIWidget **newArray = (nsIWidget**)new DWORD[mArraySize];
    memset(newArray, 0, sizeof(DWORD) * mArraySize);
    memcpy(newArray, mChildrens, (mArraySize>>1) * sizeof(DWORD));
    mChildrens = newArray;
}


//-------------------------------------------------------------------------
//
// return the style for a child nsWindow
//
//-------------------------------------------------------------------------
DWORD ChildWindow::WindowStyle()
{
    return WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | GetBorderStyle(mBorderStyle);
}


DWORD nsWindow::GetBorderStyle(nsBorderStyle aBorderStyle)
{
  switch(aBorderStyle)
  {
    case eBorderStyle_none:
      return(0);
    break;

    case eBorderStyle_dialog:
     return(WS_DLGFRAME | DS_3DLOOK);
    break;

    default:
      NS_ASSERTION(0, "unknown border style");
      return(WS_OVERLAPPEDWINDOW);
  }
}


void nsWindow::SetBorderStyle(nsBorderStyle aBorderStyle) 
{
  mBorderStyle = aBorderStyle; 
} 

void nsWindow::SetTitle(nsString aTitle) 
{
  NS_ALLOC_STR_BUF(buf, aTitle, 256);
  ::SendMessage(mWnd, WM_SETTEXT, (WPARAM)0, (LPARAM)(LPCTSTR)buf);
  NS_FREE_STR_BUF(buf);
} 


/**
 * Processes a mouse pressed event
 *
 **/
void nsWindow::AddMouseListener(nsIMouseListener * aListener)
{
  NS_PRECONDITION(mMouseListener == nsnull, "Null mouse listener");
  mMouseListener = aListener;
}

/**
 * Processes a mouse pressed event
 *
 **/
void nsWindow::AddEventListener(nsIEventListener * aListener)
{
  NS_PRECONDITION(mEventListener == nsnull, "Null mouse listener");
  mEventListener = aListener;
}



