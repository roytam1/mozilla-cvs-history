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

#include "nsISupports.h"

#include "nsWidget.h"
#include "nsIEnumerator.h"
#include "nsIAppShell.h"

#include "nsString.h"

class nsFont;

#define NSRGB_2_COLOREF(color) \
            RGB(NS_GET_R(color),NS_GET_G(color),NS_GET_B(color))


/**
 * Native GTK+ window wrapper.
 */

class nsWindow : public nsWidget
{

public:
      // nsIWidget interface

    nsWindow();
    virtual ~nsWindow();

    virtual void ConvertToDeviceCoordinates(nscoord &aX, nscoord &aY);

    NS_IMETHOD            PreCreateWidget(nsWidgetInitData *aWidgetInitData) { return NS_OK; }

    virtual void*           GetNativeData(PRUint32 aDataType);

    NS_IMETHOD              Show  (PRBool bState);

    NS_IMETHOD              Resize(PRUint32 aWidth,
                                   PRUint32 aHeight,
                                   PRBool   aRepaint);
    NS_IMETHOD              Resize(PRUint32 aX,
                                   PRUint32 aY,
                                   PRUint32 aWidth,
                                   PRUint32 aHeight,
                                   PRBool   aRepaint);

    NS_IMETHOD              GetBounds(nsRect &aRect);
    NS_IMETHOD              GetClientBounds(nsRect &aRect);
    NS_IMETHOD              GetBorderSize(PRInt32 &aWidth, PRInt32 &aHeight);

    NS_IMETHOD              Invalidate(PRBool aIsSynchronous);
    NS_IMETHOD              Invalidate(const nsRect &aRect, PRBool aIsSynchronous);
    NS_IMETHOD              Update();
    virtual nsIRenderingContext* GetRenderingContext();
    NS_IMETHOD              SetColorMap(nsColorMap *aColorMap);
    virtual nsIDeviceContext* GetDeviceContext();
    virtual nsIAppShell *   GetAppShell();
    NS_IMETHOD              Scroll(PRInt32 aDx, PRInt32 aDy, nsRect *aClipRect);

    NS_IMETHOD            SetBorderStyle(nsBorderStyle aBorderStyle);
    NS_IMETHOD            SetTitle(const nsString& aTitle);
    NS_IMETHOD            SetMenuBar(nsIMenuBar * aMenuBar);

    NS_IMETHOD            SetTooltips(PRUint32 aNumberOfTips,nsRect* aTooltipAreas[]);
    NS_IMETHOD            UpdateTooltips(nsRect* aNewTips[]);
    NS_IMETHOD            RemoveTooltips();

    NS_IMETHOD            BeginResizingChildren(void);
    NS_IMETHOD            EndResizingChildren(void);


    virtual PRBool IsChild() { return(PR_FALSE); };

     // nsBaseWidget overrides
    NS_IMETHOD SetBounds(const nsRect &aRect);

     // Utility methods
    virtual  PRBool OnPaint(nsPaintEvent &event);
    virtual  void   OnDestroy();
    PRBool   OnKey(PRUint32 aEventType, PRUint32 aKeyCode, nsKeyEvent* aEvent);
    PRBool   DispatchFocus(nsGUIEvent &aEvent);
    virtual  PRBool OnScroll(nsScrollbarEvent & aEvent, PRUint32 cPos);
    void     SetIgnoreResize(PRBool aIgnore);
    PRBool   IgnoreResize();
    PRUint32 GetYCoord(PRUint32 aNewY);
    virtual  PRBool OnResize(nsSizeEvent &aEvent);

     // Resize event management
    void   SetResizeRect(nsRect& aRect);
    void   SetResized(PRBool aResized);
    void   GetResizeRect(nsRect* aRect);
    PRBool GetResized();

    char gInstanceClassName[256];
  
protected:
  virtual void InitCallbacks(char * aName = nsnull);
  NS_IMETHOD CreateNative(GtkWidget *parentWidget);

  virtual void            UpdateVisibilityFlag();
  virtual void            UpdateDisplay();

public:
protected:
  nsIFontMetrics *mFontMetrics;
  PRBool      mIgnoreResize;
  PRBool      mVisible;
  PRBool      mDisplayed;

  // XXX Temporary, should not be caching the font
  nsFont *    mFont;

  // Resize event management
  nsRect mResizeRect;
  int    mResized;
  PRBool mLowerLeft;

  GtkWidget *mVBox;

private:
};

//
// A child window is a window with different style
//
class ChildWindow : public nsWindow {
  public:
    ChildWindow() {};
    virtual PRBool IsChild() { return(PR_TRUE); };
};

#endif // Window_h__
