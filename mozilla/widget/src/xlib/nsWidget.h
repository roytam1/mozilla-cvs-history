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

  NS_IMETHOD Move(PRUint32 aX, PRUint32 aY);
  NS_IMETHOD Resize(PRUint32 aWidth,
                    PRUint32 aHeight,
                    PRBool   aRepaint);
  NS_IMETHOD Resize(PRUint32 aX,
                    PRUint32 aY,
                    PRUint32 aWidth,
                    PRUint32 aHeight,
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
  NS_IMETHOD              SetTitle(const nsString& aTitle); 
  NS_IMETHOD              SetMenuBar(nsIMenuBar * aMenuBar); 
  NS_IMETHOD              ShowMenuBar(PRBool aShow);
  NS_IMETHOD              IsMenuBarVisible(PRBool *aVisible);
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

  virtual PRBool          OnPaint(nsPaintEvent &event);
  virtual PRBool          OnResize(nsSizeEvent &event);
  virtual PRBool          DispatchMouseEvent(nsMouseEvent &aEvent);

  static nsWidget        * GetWidgetForWindow(Window aWindow);

protected:

  // private event functions
  PRBool DispatchWindowEvent(nsGUIEvent* event);
  PRBool ConvertStatus(nsEventStatus aStatus);

  // create the native window for this class
  virtual void CreateNativeWindow(Window aParent, nsRect aRect,
                                  XSetWindowAttributes aAttr, unsigned long aMask);
  virtual void CreateNative(Window aParent, nsRect aRect);
  virtual void DestroyNative(void);
  void         CreateGC(void);

  // these will add and delete a window
  static void  AddWindowCallback   (Window aWindow, nsWidget *aWidget);
  static void  DeleteWindowCallback(Window aWindow);
  static       nsHashtable *window_list;

  PRUint32       mPreferredWidth;
  PRUint32       mPreferredHeight;

  nsIWidget   *  mParentWidget;

  // All widgets have at least these items.
  Display *      mDisplay;
  Screen *       mScreen;
  Window         mBaseWindow;
  Visual *       mVisual;
  unsigned long  mBackgroundPixel;
  PRUint32       mBorderRGB;
  unsigned long  mBorderPixel;
  GC             mGC;             // until we get gc pooling working...
  nsString       mName;           // name of the type of widget

};

extern Display         *gDisplay;
extern Screen          *gScreen;
extern int              gScreenNum;
extern int              gDepth;
extern Visual          *gVisual;
extern XVisualInfo     *gVisualInfo;

// this is from the xlibrgb code.

extern "C"
unsigned long
xlib_rgb_xpixel_from_rgb (unsigned int rgb);

extern "C"
Colormap
xlib_rgb_get_cmap (void);

#endif

