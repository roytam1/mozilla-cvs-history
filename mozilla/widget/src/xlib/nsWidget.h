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

#ifndef nsWidget_h__
#define nsWidget_h__

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>

#include "nsBaseWidget.h"
#include "nsHashtable.h"
#include "prlog.h"

#include "nsIXlibWindowService.h"

#ifdef DEBUG_blizzard
#define XLIB_WIDGET_NOISY
#endif

class nsWidget : public nsBaseWidget
{
public:
  nsWidget();
  virtual ~nsWidget();

  NS_IMETHOD Create(nsIWidget *aParent,
                    const nsRect &aRect,
                    EVENT_CALLBACK aHandleEventFunction,
                    nsIDeviceContext *aContext,
                    nsIAppShell *aAppShell = nsnull,
                    nsIToolkit *aToolkit = nsnull,
                    nsWidgetInitData *aInitData = nsnull);

  NS_IMETHOD Create(nsNativeWidget aParent,
                    const nsRect &aRect,
                    EVENT_CALLBACK aHandleEventFunction,
                    nsIDeviceContext *aContext,
                    nsIAppShell *aAppShell = nsnull,
                    nsIToolkit *aToolkit = nsnull,
                    nsWidgetInitData *aInitData = nsnull);

  virtual nsresult StandardWidgetCreate(nsIWidget *aParent,
                                        const nsRect &aRect,
                                        EVENT_CALLBACK aHandleEventFunction,
                                        nsIDeviceContext *aContext,
                                        nsIAppShell *aAppShell,
                                        nsIToolkit *aToolkit,
                                        nsWidgetInitData *aInitData,
                                        nsNativeWidget aNativeParent = nsnull);
  NS_IMETHOD Destroy();
  virtual nsIWidget *GetParent(void);
  NS_IMETHOD Show(PRBool bState);
  NS_IMETHOD IsVisible(PRBool &aState);

  NS_IMETHOD Move(PRInt32 aX, PRInt32 aY);
  NS_IMETHOD Resize(PRInt32 aWidth,
                    PRInt32 aHeight,
                    PRBool   aRepaint);
  NS_IMETHOD Resize(PRInt32 aX,
                    PRInt32 aY,
                    PRInt32 aWidth,
                    PRInt32 aHeight,
                    PRBool   aRepaint);

  NS_IMETHOD Enable(PRBool bState);
  NS_IMETHOD              SetFocus(void);
  NS_IMETHOD              SetBackgroundColor(const nscolor &aColor);
  virtual nsIFontMetrics* GetFont(void);
  NS_IMETHOD              SetFont(const nsFont &aFont);
  NS_IMETHOD              SetCursor(nsCursor aCursor);
  NS_IMETHOD Invalidate(PRBool aIsSynchronous);
  NS_IMETHOD              Invalidate(const nsRect & aRect, PRBool aIsSynchronous);
  NS_IMETHOD              Update();
  virtual void*           GetNativeData(PRUint32 aDataType);
  NS_IMETHOD              SetColorMap(nsColorMap *aColorMap);
  NS_IMETHOD              Scroll(PRInt32 aDx, PRInt32 aDy, nsRect *aClipRect);
  NS_IMETHOD              SetMenuBar(nsIMenuBar * aMenuBar); 
  NS_IMETHOD              ShowMenuBar(PRBool aShow);
  NS_IMETHOD              SetTooltips(PRUint32 aNumberOfTips,nsRect* aTooltipAreas[]);   
  NS_IMETHOD              RemoveTooltips();
  NS_IMETHOD              UpdateTooltips(nsRect* aNewTips[]);
  NS_IMETHOD              WidgetToScreen(const nsRect& aOldRect, nsRect& aNewRect);
  NS_IMETHOD              ScreenToWidget(const nsRect& aOldRect, nsRect& aNewRect);
  NS_IMETHOD              BeginResizingChildren(void);
  NS_IMETHOD              EndResizingChildren(void);
  NS_IMETHOD              GetPreferredSize(PRInt32& aWidth, PRInt32& aHeight);
  NS_IMETHOD              SetPreferredSize(PRInt32 aWidth, PRInt32 aHeight);
  NS_IMETHOD              DispatchEvent(nsGUIEvent* event, nsEventStatus & aStatus);
  NS_IMETHOD              PreCreateWidget(nsWidgetInitData *aInitData);
  NS_IMETHOD              SetBounds(const nsRect &aRect);
  NS_IMETHOD              GetRequestedBounds(nsRect &aRect);

#ifdef DEBUG
  void                    DebugPrintEvent(nsGUIEvent & aEvent,Window aWindow);
#endif

  virtual PRBool          OnPaint(nsPaintEvent &event);
  virtual PRBool          OnResize(nsSizeEvent &event);
  virtual PRBool          OnDeleteWindow(void);
  virtual PRBool          DispatchMouseEvent(nsMouseEvent &aEvent);
  virtual PRBool          DispatchKeyEvent(nsKeyEvent &aKeyEvent);
  virtual PRBool          DispatchDestroyEvent(void);

  static nsWidget        * GetWidgetForWindow(Window aWindow);
  void                     SetVisibility(int aState); // using the X constants here
  static Window            GetFocusWindow(void);

  PRBool DispatchWindowEvent(nsGUIEvent & aEvent);

//   static nsresult         SetXlibWindowCallback(nsXlibWindowCallback *aCallback);
//   static nsresult         XWindowCreated(Window aWindow);
//   static nsresult         XWindowDestroyed(Window aWindow);

  // these are for the wm protocols
  static Atom   WMDeleteWindow;
  static Atom   WMTakeFocus;
  static Atom   WMSaveYourself;
  static PRBool WMProtocolsInitialized;

protected:

  // private event functions
  PRBool ConvertStatus(nsEventStatus aStatus);

  // create the native window for this class
  virtual void CreateNativeWindow(Window aParent, nsRect aRect,
                                  XSetWindowAttributes aAttr, unsigned long aMask);
  virtual void CreateNative(Window aParent, nsRect aRect);
  virtual void DestroyNative(void);
  void         CreateGC(void);
  void         Map(void);
  void         Unmap(void);

  // Let each sublclass set the event mask according to their needs
  virtual long GetEventMask();

  // these will add and delete a window
  static void  AddWindowCallback   (Window aWindow, nsWidget *aWidget);
  static void  DeleteWindowCallback(Window aWindow);

  // set up our wm hints
  void          SetUpWMHints(void);

  // here's how we add children
  // there's no geometry information here because that should be in the mBounds
  // in the widget
  void WidgetPut        (nsWidget *aWidget);
  void WidgetMove       (nsWidget *aWidget);
  void WidgetMoveResize (nsWidget *aWidget);
  void WidgetResize     (nsWidget *aWidget);
  void WidgetShow       (nsWidget *aWidget);
  // check to see whether or not a rect will intersect with the current scrolled area
  PRBool WidgetVisible  (nsRect   &aBounds);

  PRBool         mIsShown;
  int            mVisibility; // this is an int because that's the way X likes it
  PRUint32       mPreferredWidth;
  PRUint32       mPreferredHeight;

  nsIWidget   *  mParentWidget;

  // All widgets have at least these items.
  Display *      mDisplay;
  Screen *       mScreen;
  Window         mBaseWindow;
  Visual *       mVisual;
  int            mDepth;
  unsigned long  mBackgroundPixel;
  PRUint32       mBorderRGB;
  unsigned long  mBorderPixel;
  GC             mGC;             // until we get gc pooling working...
  nsString       mName;           // name of the type of widget
  PRBool         mIsToplevel;
  nsRect         mRequestedSize;

  static         Window                 mFocusWindow;

private:
  static       nsHashtable *          gsWindowList;

  static       nsXlibWindowCallback   gsWindowCreateCallback;
  static       nsXlibWindowCallback   gsWindowDestroyCallback;
  static       nsXlibEventDispatcher  gsEventDispatcher;
};

extern PRLogModuleInfo *XlibWidgetsLM;
extern PRLogModuleInfo *XlibScrollingLM;

#endif







