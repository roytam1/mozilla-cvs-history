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
#ifndef Window_h__
#define Window_h__

#include "nsdefs.h"
#include "nsObject.h"
#include "nsSwitchToUIThread.h"
#include "nsToolkit.h"

#include "nsIWidget.h"
#include "nsIEnumerator.h"

#include "nsIMouseListener.h"
#include "nsIEventListener.h"
#include "nsStringUtil.h"
#include "nsString.h"


#define NSRGB_2_COLOREF(color) \
            RGB(NS_GET_R(color),NS_GET_G(color),NS_GET_B(color))

/**
 * Native WIN32 window wrapper. 
 */

class nsWindow : public nsObject,
                 public nsIWidget,
                 public nsSwitchToUIThread
{

public:
                            nsWindow(nsISupports *aOuter);
    virtual                 ~nsWindow();

    // nsISupports. Forward to the nsObject base class
    BASE_SUPPORT

    virtual nsresult        QueryObject(const nsIID& aIID, void** aInstancePtr);

    virtual void            PreCreateWidget(nsWidgetInitData *aWidgetInitData) {}
    // nsIWidget interface
    virtual void            Create(nsIWidget *aParent,
                                     const nsRect &aRect,
                                     EVENT_CALLBACK aHandleEventFunction,
                                     nsIDeviceContext *aContext,
                                     nsIAppShell *aAppShell = nsnull,
                                     nsIToolkit *aToolkit = nsnull,
                                     nsWidgetInitData *aInitData = nsnull);
    virtual void            Create(nsNativeWidget aParent,
                                     const nsRect &aRect,
                                     EVENT_CALLBACK aHandleEventFunction,
                                     nsIDeviceContext *aContext,
                                     nsIAppShell *aAppShell = nsnull,
                                     nsIToolkit *aToolkit = nsnull,
                                     nsWidgetInitData *aInitData = nsnull);
    virtual void            Destroy();
    virtual nsIWidget*      GetParent(void);
    virtual nsIEnumerator*  GetChildren();
    virtual void            AddChild(nsIWidget* aChild);
    virtual void            RemoveChild(nsIWidget* aChild);
    virtual void            Show(PRBool bState);
    virtual void            Move(PRUint32 aX, PRUint32 aY);
    virtual void            Resize(PRUint32 aWidth,
                                   PRUint32 aHeight,
                                   PRBool   aRepaint);
    virtual void            Resize(PRUint32 aX,
                                   PRUint32 aY,
                                   PRUint32 aWidth,
                                   PRUint32 aHeight,
                                   PRBool   aRepaint);
    virtual void            Enable(PRBool bState);
    virtual void            SetFocus(void);
    virtual void            GetBounds(nsRect &aRect);
    virtual nscolor         GetForegroundColor(void);
    virtual void            SetForegroundColor(const nscolor &aColor);
    virtual nscolor         GetBackgroundColor(void);
    virtual void            SetBackgroundColor(const nscolor &aColor);
    virtual nsIFontMetrics* GetFont(void);
    virtual void            SetFont(const nsFont &aFont);
    virtual nsCursor        GetCursor();
    virtual void            SetCursor(nsCursor aCursor);
    virtual void            Invalidate(PRBool aIsSynchronous);
    virtual void*           GetNativeData(PRUint32 aDataType);
    virtual nsIRenderingContext* GetRenderingContext();
    virtual void            SetColorMap(nsColorMap *aColorMap);
    virtual nsIDeviceContext* GetDeviceContext();
    virtual nsIAppShell *   GetAppShell();
    virtual void            Scroll(PRInt32 aDx, PRInt32 aDy, nsRect *aClipRect);
    virtual nsIToolkit*     GetToolkit();  
    virtual void            SetBorderStyle(nsBorderStyle aBorderStyle); 
    virtual void            SetTitle(const nsString& aTitle); 
    virtual void            SetTooltips(PRUint32 aNumberOfTips,nsRect* aTooltipAreas[]);   
    virtual void            RemoveTooltips();
    virtual void            UpdateTooltips(nsRect* aNewTips[]);
    virtual void            WidgetToScreen(const nsRect& aOldRect, nsRect& aNewRect);
    virtual void            ScreenToWidget(const nsRect& aOldRect, nsRect& aNewRect);
    virtual void            AddMouseListener(nsIMouseListener * aListener);
    virtual void            AddEventListener(nsIEventListener * aListener);
    virtual void            BeginResizingChildren(void);
    virtual void            EndResizingChildren(void);

    virtual void            SetUpForPaint(HDC aHDC);

    // nsSwitchToUIThread interface
    virtual BOOL            CallMethod(MethodInfo *info);

            HWND            GetWindowHandle() { return mWnd; }
            WNDPROC         GetPrevWindowProc() { return mPrevWndProc; }

    virtual PRBool          DispatchMouseEvent(PRUint32 aEventType, nsPoint* aPoint = nsnull);
    virtual PRBool          AutoErase();
    nsPoint*                GetLastPoint() { return &mLastPoint; }

protected:

  virtual PRBool          ProcessMessage(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *aRetValue);

     // Allow Derived classes to modify the height that is passed
     // when the window is created or resized.
    virtual PRInt32         GetHeight(PRInt32 aProposedHeight);
    virtual LPCTSTR         WindowClass();
    virtual DWORD           WindowStyle();
    virtual DWORD           WindowExStyle();

    virtual void            SubclassWindow(BOOL bState);

    virtual void            OnDestroy();
    virtual PRBool          OnMove(PRInt32 aX, PRInt32 aY);
    virtual PRBool          OnPaint();
    virtual PRBool          OnResize(nsRect &aWindowRect);
    virtual PRBool          OnKey(PRUint32 aEventType, PRUint32 aKeyCode);

    virtual PRBool          DispatchFocus(PRUint32 aEventType);
    virtual PRBool          OnScroll(UINT scrollCode, int cPos);
    virtual HBRUSH          OnControlColor();

    static LRESULT CALLBACK WindowProc(HWND hWnd,
                                        UINT msg,
                                        WPARAM wParam,
                                        LPARAM lParam);
    
    static PRBool ConvertStatus(nsEventStatus aStatus);
    DWORD  GetBorderStyle(nsBorderStyle aBorderStyle);

    void InitEvent(nsGUIEvent& event, PRUint32 aEventType, nsPoint* aPoint = nsnull);
    PRBool DispatchEvent(nsGUIEvent* event);
    PRBool DispatchStandardEvent(PRUint32 aMsg);
    void AddTooltip(HWND hwndOwner, nsRect* aRect, int aId);
    void RelayMouseEvent(UINT aMsg, WPARAM wParam, LPARAM lParam);

    void GetNonClientBounds(nsRect &aRect);

protected:
    static      nsWindow* gCurrentWindow;
    nsPoint     mLastPoint;
    HWND        mWnd;
    HWND        mTooltip;
    HPALETTE    mPalette;
    WNDPROC     mPrevWndProc;
    EVENT_CALLBACK mEventCallback;
    nsIDeviceContext *mContext;
    nsIFontMetrics *mFontMetrics;
    nsIAppShell *mAppShell;
    nsToolkit   *mToolkit;

    nsIMouseListener * mMouseListener;
    nsIEventListener * mEventListener;

    nscolor     mBackground;
    HBRUSH      mBrush;
    nscolor     mForeground;
    nsCursor    mCursor;
    nsBorderStyle mBorderStyle;

    PRBool      mIsShiftDown;
    PRBool      mIsControlDown;
    PRBool      mIsAltDown;
    PRBool      mIsDestroying;

    PRInt32     mWidth;
    PRInt32     mHeight;

    // keep the list of children
    class Enumerator : public nsIEnumerator {
        nsIWidget   **mChildrens;
        int         mCurrentPosition;
        int         mArraySize;

    public:
        NS_DECL_ISUPPORTS

        Enumerator();
        ~Enumerator();

        NS_IMETHOD_(nsISupports*) Next();
        NS_IMETHOD_(void) Reset();

        void Append(nsIWidget* aWidget);
        void Remove(nsIWidget* aWidget);

    private:
        void GrowArray();

    } *mChildren;

    // Enumeration of the methods which are accessable on the "main GUI thread"
    // via the CallMethod(...) mechanism...
    // see nsSwitchToUIThread
    enum {
        CREATE       = 0x0101,
        CREATE_NATIVE,
        DESTROY, 
        SET_FOCUS,
        SET_CURSOR,
        CREATE_HACK
    };

    static BOOL sIsRegistered;

    HDWP mDeferredPositioner;
};

//
// A child window is a window with different style
//
class ChildWindow : public nsWindow {

public:
                            ChildWindow(nsISupports *aOuter) : nsWindow(aOuter) {}
    PRBool          DispatchMouseEvent(PRUint32 aEventType, nsPoint* aPoint = nsnull);

protected:
    virtual DWORD           WindowStyle();
    virtual DWORD           WindowExStyle() { return 0; }
};

#define BASE_IWIDGET_IMPL  BASE_INITIALIZE_IMPL BASE_WINDOWS_METHODS


#define BASE_INITIALIZE_IMPL \
    void  Create(nsIWidget *aParent, \
                    const nsRect &aRect, \
                    EVENT_CALLBACK aHandleEventFunction, \
                    nsIDeviceContext *aContext, \
                    nsIAppShell *aAppShell = nsnull, \
                    nsIToolkit *aToolkit = nsnull, \
                    nsWidgetInitData *aInitData = nsnull) \
    { \
        nsWindow::Create(aParent, aRect, aHandleEventFunction, aContext, aAppShell, aToolkit, aInitData); \
    } \

#define BASE_WINDOWS_METHODS \
    void Create(nsNativeWidget aParent, \
                 const nsRect &aRect, \
                 EVENT_CALLBACK aHandleEventFunction, \
                 nsIDeviceContext *aContext, \
                 nsIAppShell *aAppShell = nsnull, \
                 nsIToolkit *aToolkit = nsnull, \
                 nsWidgetInitData *aInitData = nsnull) \
    { \
        nsWindow::Create(aParent, aRect, aHandleEventFunction, aContext, aAppShell, aToolkit, aInitData); \
    } \
    void Destroy(void) \
    { \
        nsWindow::Destroy(); \
    } \
    nsIWidget* GetParent(void) \
    { \
        return nsWindow::GetParent(); \
    } \
    nsIEnumerator* GetChildren(void) \
    { \
        return nsWindow::GetChildren(); \
    } \
    void AddChild(nsIWidget* aChild) \
    { \
        nsWindow::AddChild(aChild); \
    } \
    void RemoveChild(nsIWidget* aChild) \
    { \
        nsWindow::RemoveChild(aChild); \
    } \
    void Show(PRBool bState) \
    { \
        nsWindow::Show(bState); \
    } \
    void Move(PRUint32 aX, PRUint32 aY) \
    { \
        nsWindow::Move(aX, aY); \
    } \
    void Resize(PRUint32 aWidth, \
                PRUint32 aHeight, \
                PRBool   aRepaint) \
    { \
        nsWindow::Resize(aWidth, aHeight, aRepaint); \
    } \
    void Resize(PRUint32 aX, \
                PRUint32 aY, \
                PRUint32 aWidth, \
                PRUint32 aHeight, \
                PRBool   aRepaint) \
    { \
        nsWindow::Resize(aX, aY, aWidth, aHeight, aRepaint); \
    } \
    void Enable(PRBool bState) \
    { \
        nsWindow::Enable(bState); \
    } \
    void SetFocus(void) \
    { \
        nsWindow::SetFocus(); \
    } \
    nscolor GetForegroundColor(void) \
    { \
        return nsWindow::GetForegroundColor(); \
    } \
    void SetForegroundColor(const nscolor &aColor) \
    { \
        nsWindow::SetForegroundColor(aColor); \
    } \
    nscolor GetBackgroundColor(void) \
    { \
        return nsWindow::GetBackgroundColor(); \
    } \
    void SetBackgroundColor(const nscolor &aColor) \
    { \
        nsWindow::SetBackgroundColor(aColor); \
    } \
    nsIFontMetrics* GetFont(void) \
    { \
        return nsWindow::GetFont(); \
    } \
    void SetFont(const nsFont &aFont) \
    { \
        nsWindow::SetFont(aFont); \
    } \
    nsCursor GetCursor() \
    { \
        return nsWindow::GetCursor(); \
    } \
    void SetCursor(nsCursor aCursor) \
    { \
        nsWindow::SetCursor(aCursor); \
    } \
    void Invalidate(PRBool aIsSynchronous) \
    { \
        nsWindow::Invalidate(aIsSynchronous); \
    } \
    void* GetNativeData(PRUint32 aDataType) \
    { \
        return nsWindow::GetNativeData(aDataType); \
    } \
    nsIRenderingContext* GetRenderingContext() \
    { \
        return nsWindow::GetRenderingContext(); \
    } \
    nsIDeviceContext* GetDeviceContext() \
    { \
        return nsWindow::GetDeviceContext(); \
    } \
    nsIAppShell* GetAppShell() \
    { \
        return nsWindow::GetAppShell(); \
    } \
    void Scroll(PRInt32 aDx, PRInt32 aDy, nsRect *aClipRect) \
    { \
        nsWindow::Scroll(aDx, aDy, aClipRect); \
    } \
    nsIToolkit* GetToolkit() \
    { \
        return nsWindow::GetToolkit(); \
    } \
    void SetColorMap(nsColorMap *aColorMap) \
    { \
        nsWindow::SetColorMap(aColorMap); \
    } \
    void AddMouseListener(nsIMouseListener * aListener) \
    { \
       nsWindow::AddMouseListener(aListener); \
    } \
    void AddEventListener(nsIEventListener * aListener) \
    { \
       nsWindow::AddEventListener(aListener); \
    } \
    PRBool OnKey(PRUint32 aEventType, PRUint32 aKeyCode) \
    { \
      return nsWindow::OnKey(aEventType, aKeyCode); \
    } \
    void SetBorderStyle(nsBorderStyle aBorderStyle) \
    { \
      nsWindow::SetBorderStyle(aBorderStyle); \
    } \
    void SetTitle(const nsString& aTitle) \
    { \
      nsWindow::SetTitle(aTitle); \
    } \
    void SetTooltips(PRUint32 aNumberOfTips,nsRect* aTooltipAreas[]) \
    { \
      nsWindow::SetTooltips(aNumberOfTips, aTooltipAreas); \
    } \
    void UpdateTooltips(nsRect* aNewTips[]) \
    { \
      nsWindow::UpdateTooltips(aNewTips); \
    } \
    void RemoveTooltips() \
    { \
      nsWindow::RemoveTooltips(); \
    } \
    void WidgetToScreen(const nsRect& aOldRect, nsRect& aNewRect) \
    { \
      nsWindow::WidgetToScreen(aOldRect, aNewRect); \
    } \
    void ScreenToWidget(const nsRect& aOldRect, nsRect& aNewRect) \
    { \
      nsWindow::ScreenToWidget(aOldRect, aNewRect); \
    } \
    void BeginResizingChildren(void) \
    { \
      nsWindow::BeginResizingChildren(); \
    } \
    void EndResizingChildren(void) \
    { \
      nsWindow::EndResizingChildren(); \
    } 



#endif // Window_h__
