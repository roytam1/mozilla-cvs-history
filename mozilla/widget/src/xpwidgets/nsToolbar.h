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

#ifndef nsToolbar_h___
#define nsToolbar_h___

#include "nsIToolbar.h"
#include "nsWindow.h"
#include "nsIImageButton.h"
#include "nsIToolbarItem.h"

class ToolbarLayoutInfo;

//------------------------------------------------------------
class nsToolbar : public ChildWindow,
                  public nsIToolbar,
                  public nsIToolbarItem
                  
{
public:
    nsToolbar();
    virtual ~nsToolbar();

    NS_DECL_ISUPPORTS

    NS_IMETHOD AddItem(nsIToolbarItem* anItem, PRInt32 aLeftGap, PRBool aStretchable);
    NS_IMETHOD InsertItemAt(nsIToolbarItem* anItem, 
                            PRInt32         aLeftGap, 
                            PRBool          aStretchable, 
                            PRInt32         anIndex);
    NS_IMETHOD GetItemAt(nsIToolbarItem*& anItem, PRInt32 anIndex);


    NS_IMETHOD DoLayout();
    NS_IMETHOD SetHorizontalLayout(PRBool aDoHorizontalLayout);
    NS_IMETHOD SetHGap(PRInt32 aGap);
    NS_IMETHOD SetVGap(PRInt32 aGap);
    NS_IMETHOD SetMargin(PRInt32 aMargin);

    NS_IMETHOD SetLastItemIsRightJustified(const PRBool & aState);
    NS_IMETHOD SetNextLastItemIsStretchy(const PRBool & aState);

    NS_IMETHOD SetToolbarManager(nsIToolbarManager * aToolbarManager);
    NS_IMETHOD GetToolbarManager(nsIToolbarManager *& aToolbarManager);
    NS_IMETHOD SetBorderType(nsToolbarBorderType aBorderType);

    NS_IMETHOD_(nsEventStatus) HandleEvent(nsGUIEvent *aEvent);
    NS_IMETHOD_(nsEventStatus) OnPaint(nsIRenderingContext& aRenderingContext,
                                       const nsRect& aDirtyRect);


    // nsIToolbarItem
    NS_IMETHOD Repaint(PRBool aIsSynchronous);
    NS_IMETHOD GetBounds(nsRect &aRect);
    NS_IMETHOD SetVisible(PRBool aState);
    //NS_IMETHOD IsVisible(PRBool & aState);
    NS_IMETHOD SetLocation(PRUint32 aX, PRUint32 aY);
    NS_IMETHOD SetBounds(PRUint32 aWidth,
                        PRUint32 aHeight,
                        PRBool   aRepaint);
    NS_IMETHOD SetBounds(PRUint32 aX,
                         PRUint32 aY,
                         PRUint32 aWidth,
                         PRUint32 aHeight,
                         PRBool   aRepaint);
    //NS_IMETHOD GetPreferredSize(PRInt32& aWidth, PRInt32& aHeight);
    NS_IMETHOD SetPreferredSize(PRInt32 aWidth, PRInt32 aHeight);


    // Overriding nsWindow & nsIToolbarItem
    NS_IMETHOD IsVisible(PRBool & aIsVisible);
    NS_IMETHOD GetPreferredSize(PRInt32& aWidth, PRInt32& aHeight);

    NS_IMETHOD            Resize(PRUint32 aWidth,
                                   PRUint32 aHeight,
                                   PRBool   aRepaint);

    NS_IMETHOD            Resize(PRUint32 aX,
                                   PRUint32 aY,
                                   PRUint32 aWidth,
                                   PRUint32 aHeight,
                                   PRBool   aRepaint);

  NS_IMETHOD SetWrapping(PRBool aDoWrap);
  NS_IMETHOD GetWrapping(PRBool & aDoWrap);

  NS_IMETHOD GetPreferredConstrainedSize(PRInt32& aSuggestedWidth, PRInt32& aSuggestedHeight, 
                                         PRInt32& aWidth,          PRInt32& aHeight);
  NS_IMETHOD CreateTab(nsIWidget *& aTab);

protected:
  void GetMargins(PRInt32 &aX, PRInt32 &aY);
  void DoHorizontalLayout(const nsRect& aTBRect);
  void DoVerticalLayout(const nsRect& aTBRect);
  void AddTab(const nsString& aUpURL,
              const nsString& aPressedURL,
              const nsString& aDisabledURL,
              const nsString& aRollOverURL);


  // This will be changed to a nsVoidArray or a Deque
  ToolbarLayoutInfo ** mItems;
  PRInt32              mNumItems;

  PRBool mLastItemIsRightJustified;
  PRBool mNextLastItemIsStretchy;

  PRInt32 mMargin;
  PRInt32 mWrapMargin;
  PRInt32 mHGap;
  PRInt32 mVGap;

  PRBool  mBorderType;

  PRBool  mWrapItems;
  PRBool  mDoHorizontalLayout;

  nsIToolbarManager * mToolbarMgr;

};

#endif /* nsToolbar_h___ */
