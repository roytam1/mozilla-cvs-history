/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: NPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is 
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or 
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the NPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef nsScrollingView_h___
#define nsScrollingView_h___

#include "nsView.h"
#include "nsIScrollableView.h"
#include "nsCOMPtr.h"
#include "nsITimer.h"

class nsISupportsArray;

//this is a class that acts as a container for other views and provides
//automatic management of scrolling of the views it contains.

class nsScrollingView : public nsView, public nsIScrollableView, public nsITimerCallback
{
public:
  nsScrollingView();

  NS_IMETHOD QueryInterface(REFNSIID aIID,
                            void** aInstancePtr);

  //overrides
  NS_IMETHOD  Init(nsIViewManager* aManager,
      						 const nsRect &aBounds,
                   const nsIView *aParent,
      						 nsViewVisibility aVisibilityFlag = nsViewVisibility_kShow);
  virtual void SetDimensions(const nsRect& aRect, PRBool aPaint = PR_TRUE);
  virtual void SetPosition(nscoord aX, nscoord aY);
   // SetVisibility is overriden so that it will set it's components visibility (ClipView, 
   // CornerView, ScrollBarView's),as well as it's own visibility.
  NS_IMETHOD  SetVisibility(nsViewVisibility visibility);
  NS_IMETHOD  SetWidget(nsIWidget *aWidget);

  NS_IMETHOD  SetZIndex(PRBool aAuto, PRInt32 aZIndex, PRBool aTopMost);

  //nsIScrollableView interface
  NS_IMETHOD  CreateScrollControls(nsNativeWidget aNative = nsnull);
  NS_IMETHOD  ComputeScrollOffsets(PRBool aAdjustWidgets = PR_TRUE);
  NS_IMETHOD  GetContainerSize(nscoord *aWidth, nscoord *aHeight) const;
  NS_IMETHOD  SetScrolledView(nsIView *aScrolledView);
  NS_IMETHOD  GetScrolledView(nsIView *&aScrolledView) const;

  NS_IMETHOD  ShowQuality(PRBool aShow);
  NS_IMETHOD  GetShowQuality(PRBool &aShow) const;
  NS_IMETHOD  SetQuality(nsContentQuality aQuality);

  NS_IMETHOD  SetScrollPreference(nsScrollPreference aPref);
  NS_IMETHOD  GetScrollPreference(nsScrollPreference &aScrollPreference) const;
  NS_IMETHOD  GetScrollPosition(nscoord &aX, nscoord &aY) const;
  NS_IMETHOD  ScrollTo(nscoord aX, nscoord aY, PRUint32 aUpdateFlags);
  NS_IMETHOD  SetControlInsets(const nsMargin &aInsets);
  NS_IMETHOD  GetControlInsets(nsMargin &aInsets) const;
  NS_IMETHOD  GetScrollbarVisibility(PRBool *aVerticalVisible,
                                     PRBool *aHorizontalVisible) const;
  NS_IMETHOD  SetScrollProperties(PRUint32 aProperties);
  NS_IMETHOD  GetScrollProperties(PRUint32 *aProperties);
  NS_IMETHOD  SetLineHeight(nscoord aHeight);
  NS_IMETHOD  GetLineHeight(nscoord *aHeight);
  NS_IMETHOD  ScrollByLines(PRInt32 aNumLinesX, PRInt32 aNumLinesY);
  NS_IMETHOD  ScrollByPages(PRInt32 aNumPages);
  NS_IMETHOD  ScrollByWhole(PRBool aTop);
  
  NS_IMETHOD  GetClipView(const nsIView** aClipView) const;
  nsView*     GetClipView() const { return mClipView; }

  NS_IMETHOD  AddScrollPositionListener(nsIScrollPositionListener* aListener);
  NS_IMETHOD  RemoveScrollPositionListener(nsIScrollPositionListener* aListener);

  // local to view module
  void HandleScrollEvent(nsGUIEvent *aEvent, PRUint32 aEventFlags);
 
   // Set the visibility of a nsScrollingView's component
   // @param aView nsScrollingView component to set visibility for or nsnull.
   // @param aViewVisibility new setting for the component view If not the same as the current setting 
   // @returns the result of calling the SetVisibility on the component.
  nsresult SetComponentVisibility(nsView* aView, nsViewVisibility aViewVisibility); 

   // Update the visibility of the nsScrollingView's components (ClipView, CornerView, ScrollBarView's)
   // @param aScrollingViewVisibility Visibility setting of the ScrollingView to consider when
   // setting the visibility of the components.
   // @returns the result of calling SetComponentVisibility for each component.
  nsresult UpdateComponentVisibility(nsViewVisibility aScrollingViewVisibility);

  nsresult NotifyScrollPositionWillChange(nscoord aX, nscoord aY);
  nsresult NotifyScrollPositionDidChange(nscoord aX, nscoord aY);

  nsView* GetScrolledView() const;

private:
  NS_IMETHOD_(nsrefcnt) AddRef(void);
  NS_IMETHOD_(nsrefcnt) Release(void);

protected:
  virtual ~nsScrollingView();

  // nsITimerCallback Interface
  NS_DECL_NSITIMERCALLBACK

  //private
  void AdjustChildWidgets(nsScrollingView *aScrolling, nsView *aView, nscoord aDx, nscoord aDy, float aScale);
  void UpdateScrollControls(PRBool aPaint);
  void Scroll(nsView *aScrolledView, PRInt32 aDx, PRInt32 aDy, float scale, PRUint32 aUpdateFlags);
  PRBool CannotBitBlt(nsView* aScrolledView);

protected:
  nscoord             mSizeX, mSizeY;
  nscoord             mOffsetX, mOffsetY;
  nsView              *mClipView;
  nsView              *mVScrollBarView;
  nsView              *mHScrollBarView;
  nsView              *mCornerView;
  nsScrollPreference  mScrollPref;
  nsMargin            mInsets;
  nsCOMPtr<nsITimer>  mScrollingTimer;
  nscoord             mScrollingDelta;
  PRUint32            mScrollProperties;
  nscoord             mLineHeight;
  nsISupportsArray   *mListeners;
};

#endif
