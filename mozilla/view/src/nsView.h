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

#ifndef nsView_h___
#define nsView_h___

#include "nsIView.h"
#include "nsRect.h"
#include "nsCRT.h"
#include "nsIWidget.h"
#include "nsIFactory.h"
#include "nsIViewObserver.h"

//mmptemp

class nsIPresContext;
class nsIViewManager;

class nsView : public nsIView
{
public:
  nsView();

  NS_DECL_AND_IMPL_ZEROING_OPERATOR_NEW

  // nsISupports
  NS_IMETHOD  QueryInterface(const nsIID& aIID, void** aInstancePtr);

  // nsIView
  NS_IMETHOD  Init(nsIViewManager* aManager,
      						 const nsRect &aBounds,
                   const nsIView *aParent,
      						 const nsViewClip *aClip = nsnull,
      						 nsViewVisibility aVisibilityFlag = nsViewVisibility_kShow);

  NS_IMETHOD  Destroy();
  NS_IMETHOD  GetViewManager(nsIViewManager *&aViewMgr) const;
  NS_IMETHOD  Paint(nsIRenderingContext& rc, const nsRect& rect,
                    PRUint32 aPaintFlags, PRBool &aResult);
  NS_IMETHOD  Paint(nsIRenderingContext& rc, const nsIRegion& region,
                    PRUint32 aPaintFlags, PRBool &aResult);
  NS_IMETHOD  HandleEvent(nsGUIEvent *event, PRUint32 aEventFlags, nsEventStatus* aStatus, PRBool& aHandled);
  NS_IMETHOD  SetPosition(nscoord x, nscoord y);
  NS_IMETHOD  GetPosition(nscoord *x, nscoord *y) const;
  NS_IMETHOD  SetDimensions(nscoord width, nscoord height, PRBool aPaint = PR_TRUE);
  NS_IMETHOD  GetDimensions(nscoord *width, nscoord *height) const;
  NS_IMETHOD  SetBounds(const nsRect &aBounds, PRBool aPaint = PR_TRUE);
  NS_IMETHOD  SetBounds(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight, PRBool aPaint = PR_TRUE);
  NS_IMETHOD  GetBounds(nsRect &aBounds) const;
  NS_IMETHOD  SetClip(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight);
  NS_IMETHOD  GetClip(nscoord *aLeft, nscoord *aTop, nscoord *aRight, nscoord *aBottom, PRBool &aResult) const;
  NS_IMETHOD  SetVisibility(nsViewVisibility visibility);
  NS_IMETHOD  GetVisibility(nsViewVisibility &aVisibility) const;
  NS_IMETHOD  SetZIndex(PRInt32 aZIndex);
  NS_IMETHOD  GetZIndex(PRInt32 &aZIndex) const;
  NS_IMETHOD  SetAutoZIndex(PRBool aAutoZIndex);
  NS_IMETHOD  GetAutoZIndex(PRBool &aAutoZIndex) const;
  NS_IMETHOD  SetParent(nsIView *aParent);
  NS_IMETHOD  GetParent(nsIView *&aParent) const;
  NS_IMETHOD  GetNextSibling(nsIView *&aNextSibling) const;
  NS_IMETHOD  SetNextSibling(nsIView* aNextSibling);
  NS_IMETHOD  InsertChild(nsIView *child, nsIView *sibling);
  NS_IMETHOD  RemoveChild(nsIView *child);
  NS_IMETHOD  GetChildCount(PRInt32 &aCount) const;
  NS_IMETHOD  GetChild(PRInt32 index, nsIView*& aChild) const;
  NS_IMETHOD  SetTransform(nsTransform2D &aXForm);
  NS_IMETHOD  GetTransform(nsTransform2D &aXForm) const;
  NS_IMETHOD  SetOpacity(float opacity);
  NS_IMETHOD  GetOpacity(float &aOpacity) const;
  NS_IMETHOD  HasTransparency(PRBool &aTransparent) const;
  NS_IMETHOD  SetContentTransparency(PRBool aTransparent);
  NS_IMETHOD  SetClientData(void *aData);
  NS_IMETHOD  GetClientData(void *&aData) const;
  NS_IMETHOD  GetOffsetFromWidget(nscoord *aDx, nscoord *aDy, nsIWidget *&aWidget);
  NS_IMETHOD  GetDirtyRegion(nsIRegion*& aRegion) const;
  NS_IMETHOD  CreateWidget(const nsIID &aWindowIID,
                           nsWidgetInitData *aWidgetInitData = nsnull,
                           nsNativeWidget aNative = nsnull,
                           PRBool aEnableDragDrop = PR_TRUE);
  NS_IMETHOD  SetWidget(nsIWidget *aWidget);
  NS_IMETHOD  GetWidget(nsIWidget *&aWidget) const;
  NS_IMETHOD  HasWidget(PRBool *aHasWidget) const;
  NS_IMETHOD  List(FILE* out = stdout, PRInt32 aIndent = 0) const;
  NS_IMETHOD  SetViewFlags(PRUint32 aFlags);
  NS_IMETHOD  ClearViewFlags(PRUint32 aFlags);
  NS_IMETHOD  GetViewFlags(PRUint32 *aFlags) const;
  NS_IMETHOD  GetScratchPoint(nsPoint **aPoint);
  NS_IMETHOD  GetExtents(nsRect *aExtents);

  // XXX Temporary for Bug #19416
  NS_IMETHOD IgnoreSetPosition(PRBool aShouldIgnore);

  // Helper function to get the view that's associated with a widget
  static nsIView*  GetViewFor(nsIWidget* aWidget);

protected:
  virtual ~nsView();
  //
  virtual nsresult LoadWidget(const nsCID &aClassIID);

protected:
  nsIViewManager    *mViewManager;
  nsIView           *mParent;
  nsIWidget         *mWindow;

  //XXX should there be pointers to last child so backward walking is fast?
  nsIView           *mNextSibling;
  nsIView           *mFirstChild;
  void              *mClientData;
  PRInt32           mZindex;
  nsViewVisibility  mVis;
  PRInt32           mNumKids;
  nsRect            mBounds;
  nsViewClip        mClip;
  nsTransform2D     *mXForm;
  float             mOpacity;
  PRUint32          mVFlags;
  nsIRegion*        mDirtyRegion;
  nsPoint           mScratchPoint;

  // Bug #19416
  PRBool            mShouldIgnoreSetPosition;

private:
  NS_IMETHOD_(nsrefcnt) AddRef(void);
  NS_IMETHOD_(nsrefcnt) Release(void);
};

#endif
