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
#ifndef Window_h__
#define Window_h__

#include "nsBaseWidget.h"
#include "nsdefs.h"
#include "nsSwitchToUIThread.h"
#include "nsToolkit.h"

#include "nsIWidget.h"
#include "nsIKBStateControl.h"

#include "nsIMenuBar.h"

#include "nsIMouseListener.h"
#include "nsIEventListener.h"
#include "nsStringUtil.h"
#include "nsString.h"

#include "nsVoidArray.h"

class nsNativeDragTarget;
class nsIRollupListener;

#define NSRGB_2_COLOREF(color) \
            RGB(NS_GET_R(color),NS_GET_G(color),NS_GET_B(color))


/**
 * Native WIN32 window wrapper. 
 */

class nsWindow : public nsSwitchToUIThread,
                 public nsBaseWidget,
                 public nsIKBStateControl
{

public:
    nsWindow();
    virtual ~nsWindow();

    // nsISupports
    NS_IMETHOD_(nsrefcnt) AddRef(void);
    NS_IMETHOD_(nsrefcnt) Release(void);
    NS_IMETHOD QueryInterface(REFNSIID aIID, void** aInstancePtr);

    // nsIWidget interface
    NS_IMETHOD            Create(nsIWidget *aParent,
                                     const nsRect &aRect,
                                     EVENT_CALLBACK aHandleEventFunction,
                                     nsIDeviceContext *aContext,
                                     nsIAppShell *aAppShell = nsnull,
                                     nsIToolkit *aToolkit = nsnull,
                                     nsWidgetInitData *aInitData = nsnull);
    NS_IMETHOD              Create(nsNativeWidget aParent,
                                     const nsRect &aRect,
                                     EVENT_CALLBACK aHandleEventFunction,
                                     nsIDeviceContext *aContext,
                                     nsIAppShell *aAppShell = nsnull,
                                     nsIToolkit *aToolkit = nsnull,
                                     nsWidgetInitData *aInitData = nsnull);

     // Utility method for implementing both Create(nsIWidget ...) and
     // Create(nsNativeWidget...)

    virtual nsresult        StandardWindowCreate(nsIWidget *aParent,
                            const nsRect &aRect,
                            EVENT_CALLBACK aHandleEventFunction,
                            nsIDeviceContext *aContext,
                            nsIAppShell *aAppShell,
                            nsIToolkit *aToolkit,
                            nsWidgetInitData *aInitData,
                            nsNativeWidget aNativeParent = nsnull);

    NS_IMETHOD              Destroy();
    virtual nsIWidget*      GetParent(void);
    NS_IMETHOD              Show(PRBool bState);
    NS_IMETHOD              IsVisible(PRBool & aState);

    NS_IMETHOD              ModalEventFilter(PRBool aRealEvent, void *aEvent,
                                             PRBool *aForWindow);

    NS_IMETHOD              CaptureMouse(PRBool aCapture);
    NS_IMETHOD              Move(PRInt32 aX, PRInt32 aY);
    NS_IMETHOD              Resize(PRInt32 aWidth,
                                   PRInt32 aHeight,
                                   PRBool   aRepaint);
    NS_IMETHOD              Resize(PRInt32 aX,
                                   PRInt32 aY,
                                   PRInt32 aWidth,
                                   PRInt32 aHeight,
                                   PRBool   aRepaint);
    NS_IMETHOD              Enable(PRBool bState);
    NS_IMETHOD              SetFocus(void);
    NS_IMETHOD              GetBounds(nsRect &aRect);
    NS_IMETHOD              GetClientBounds(nsRect &aRect);
    NS_IMETHOD              SetBackgroundColor(const nscolor &aColor);
    virtual nsIFontMetrics* GetFont(void);
    NS_IMETHOD              SetFont(const nsFont &aFont);
    NS_IMETHOD              SetCursor(nsCursor aCursor);
    NS_IMETHOD              Invalidate(PRBool aIsSynchronous);
    NS_IMETHOD              Invalidate(const nsRect & aRect, PRBool aIsSynchronous);
    NS_IMETHOD              Update();
    virtual void*           GetNativeData(PRUint32 aDataType);
    virtual void            FreeNativeData(void * data, PRUint32 aDataType);//~~~
    NS_IMETHOD              SetColorMap(nsColorMap *aColorMap);
    NS_IMETHOD              Scroll(PRInt32 aDx, PRInt32 aDy, nsRect *aClipRect);
    NS_IMETHOD              SetTitle(const nsString& aTitle); 
    NS_IMETHOD              SetMenuBar(nsIMenuBar * aMenuBar); 
    NS_IMETHOD              ShowMenuBar(PRBool aShow);
    NS_IMETHOD              WidgetToScreen(const nsRect& aOldRect, nsRect& aNewRect);
    NS_IMETHOD              ScreenToWidget(const nsRect& aOldRect, nsRect& aNewRect);
    NS_IMETHOD              BeginResizingChildren(void);
    NS_IMETHOD              EndResizingChildren(void);
    NS_IMETHOD              GetPreferredSize(PRInt32& aWidth, PRInt32& aHeight);
    NS_IMETHOD              SetPreferredSize(PRInt32 aWidth, PRInt32 aHeight);
    NS_IMETHOD              DispatchEvent(nsGUIEvent* event, nsEventStatus & aStatus);
    NS_IMETHOD              EnableDragDrop(PRBool aEnable);

    virtual void            SetUpForPaint(HDC aHDC);
   	virtual void            ConvertToDeviceCoordinates(nscoord	&aX,nscoord	&aY) {}

    NS_IMETHOD              CaptureRollupEvents(nsIRollupListener * aListener, PRBool aDoCapture, PRBool aConsumeRollupEvent);

    // nsIKBStateControl interface 

    NS_IMETHOD ResetInputState();
    NS_IMETHOD PasswordFieldInit();

    HWND                    mBorderlessParent;

    // nsSwitchToUIThread interface
    virtual BOOL            CallMethod(MethodInfo *info);

            HWND            GetWindowHandle() { return mWnd; }
            WNDPROC         GetPrevWindowProc() { return mPrevWndProc; }

    virtual PRBool          DispatchMouseEvent(PRUint32 aEventType, nsPoint* aPoint = nsnull);
    virtual PRBool          AutoErase();
    nsPoint*                GetLastPoint() { return &mLastPoint; }

    PRInt32                 GetNewCmdMenuId() { mMenuCmdId++; return mMenuCmdId;}

    void InitEvent(nsGUIEvent& event, PRUint32 aEventType, nsPoint* aPoint = nsnull);

protected:

    static  PRBool          IsScrollbar(HWND aWnd);
    static  PRBool          EventIsInsideWindow(nsWindow* aWindow); 
    virtual PRBool          ProcessMessage(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *aRetValue);
    virtual PRBool          DispatchWindowEvent(nsGUIEvent* event);
    virtual PRBool          DispatchWindowEvent(nsGUIEvent*event, nsEventStatus &aStatus);
    nsresult                MenuHasBeenSelected(HMENU aNativeMenu, UINT aItemNum, UINT aFlags, UINT aCommand);
    nsresult                DynamicMenuHasBeenSelected(HMENU aNativeMenu, UINT aItemNum, UINT aFlags, UINT aCommand);
    nsIMenuItem *           FindMenuItem(nsIMenu * aMenu, PRUint32 aId);
    nsIMenu *               FindMenu(nsIMenu * aMenu, HMENU aNativeMenu, PRInt32 &aDepth);

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

    BOOL                    OnChar(UINT mbcsCharCode, UINT virtualKeyCode, bool isMultibyte);

    BOOL                    OnKeyDown( UINT aVirtualKeyCode, UINT aScanCode);
    BOOL                    OnKeyUp( UINT aVirtualKeyCode, UINT aScanCode);


    BOOL                    OnInputLangChange(WORD aLangID, LRESULT *oResult);			
    BOOL                    OnIMEChar(BYTE aByte1, BYTE aByte2, LPARAM aKeyState);
    BOOL                    OnIMEComposition(LPARAM  aGCS);			
    BOOL                    OnIMECompositionFull();			
    BOOL                    OnIMEEndComposition();			
    BOOL                    OnIMENotify(WPARAM  aIMN, LPARAM aData, LRESULT *oResult);			
    BOOL                    OnIMERequest(WPARAM  aIMR, LPARAM aData, LRESULT *oResult);			
    BOOL                    OnIMESelect(BOOL  aSelected, WORD aLangID);			
    BOOL                    OnIMESetContext(BOOL aActive, LPARAM& aISC);			
    BOOL                    OnIMEStartComposition();			

    ULONG                   IsSpecialChar(UINT aVirtualKeyCode, WORD *aAsciiKey);
    virtual PRBool          DispatchKeyEvent(PRUint32 aEventType, WORD aCharCode, UINT aVirtualCharCode);

    virtual PRBool          DispatchFocus(PRUint32 aEventType);
    virtual PRBool          OnScroll(UINT scrollCode, int cPos);
    virtual HBRUSH          OnControlColor();

    static LRESULT CALLBACK WindowProc(HWND hWnd,
                                        UINT msg,
                                        WPARAM wParam,
                                        LPARAM lParam);
    
    static PRBool ConvertStatus(nsEventStatus aStatus);
    DWORD  GetWindowType(nsWindowType aWindowType);
    DWORD  GetBorderStyle(nsBorderStyle aBorderStyle);

    PRBool DispatchStandardEvent(PRUint32 aMsg);
    void RelayMouseEvent(UINT aMsg, WPARAM wParam, LPARAM lParam);

    void GetNonClientBounds(nsRect &aRect);
	void HandleTextEvent(HIMC hIMEContext, PRBool aCheckAttr=PR_TRUE);
	void HandleStartComposition(HIMC hIMEContext);
	void HandleEndComposition(void);
	void MapDBCSAtrributeArrayToUnicodeOffsets(PRUint32* textRangeListLengthResult, nsTextRangeArray* textRangeListResult);

    NS_IMETHOD PasswordFieldEnter(PRUint32& oSavedState);
    NS_IMETHOD PasswordFieldExit(PRUint32 aRestoredState);

private:

#ifdef DEBUG
  void DebugPrintEvent(nsGUIEvent &   aEvent,
                       HWND           aWnd);
#endif

protected:
    static      nsWindow* gCurrentWindow;
    PRBool      mIsTopWidgetWindow;
    nsPoint     mLastPoint;
    HWND        mWnd;
    HPALETTE    mPalette;
    WNDPROC     mPrevWndProc;

    PRBool      mHas3DBorder;
    HBRUSH      mBrush;
    PRBool      mIsShiftDown;
    PRBool      mIsControlDown;
    PRBool      mIsAltDown;
    PRBool      mIsDestroying;
    PRBool      mOnDestroyCalled;
    PRBool      mIsVisible;
    // XXX Temporary, should not be caching the font
    nsFont *    mFont;

    PRInt32     mPreferredWidth;
    PRInt32     mPreferredHeight;

    nsIMenuBar  * mMenuBar;
    PRInt32       mMenuCmdId;
    nsIMenu     * mHitMenu;
    nsVoidArray * mHitSubMenus;

	// For Input Method Support
	DWORD		mIMEProperty;
	PRBool		mIMEIsComposing;
	nsCString*	mIMECompString;
	nsString*	mIMECompUnicode;
	char*		mIMEAttributeString;
	PRInt32		mIMEAttributeStringLength;
	PRInt32		mIMEAttributeStringSize;
	char*		mIMECompClauseString;
	PRInt32		mIMECompClauseStringLength;
	PRInt32		mIMECompClauseStringSize;
	long		mIMECursorPosition;
	UINT		mCurrentKeyboardCP;

  PRBool  mIsInMouseCapture;

	BOOL		mHaveDBCSLeadByte;
	unsigned char mDBCSLeadByte;

    // Drag & Drop
    nsNativeDragTarget * mNativeDragTarget;

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
                            ChildWindow(){}
    PRBool          DispatchMouseEvent(PRUint32 aEventType, nsPoint* aPoint = nsnull);

protected:
    virtual DWORD           WindowStyle();
    virtual DWORD           WindowExStyle() { return 0; }
};


#endif // Window_h__
