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

#include "nsToolbar.h"
#include "nsWidgetsCID.h"
#include "nspr.h"
#include "nsIWidget.h"
#include "nsIImageButton.h"
#include "nsIToolbarManager.h"
#include "nsIToolbarItemHolder.h"
#include "nsImageButton.h"
#include "nsRepository.h"

static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);
static NS_DEFINE_IID(kCToolbarCID,  NS_TOOLBAR_CID);
static NS_DEFINE_IID(kCIToolbarIID, NS_ITOOLBAR_IID);
static NS_DEFINE_IID(kIToolbarIID, NS_ITOOLBAR_IID);

#define TAB_WIDTH  9
#define TAB_HEIGHT 42

const PRInt32 gMaxInfoItems = 32;

NS_IMPL_ADDREF(nsToolbar)
NS_IMPL_RELEASE(nsToolbar)

static NS_DEFINE_IID(kIImageButtonIID, NS_IIMAGEBUTTON_IID);
static NS_DEFINE_IID(kImageButtonCID, NS_IMAGEBUTTON_CID);
static NS_DEFINE_IID(kIToolbarItemIID, NS_ITOOLBARITEM_IID);
static NS_DEFINE_IID(kIWidgetIID, NS_IWIDGET_IID);

static NS_DEFINE_IID(kIToolbarItemHolderIID, NS_ITOOLBARITEMHOLDER_IID);
static NS_DEFINE_IID(kToolbarItemHolderCID, NS_TOOLBARITEMHOLDER_CID);

//------------------------------------------------------------
class ToolbarLayoutInfo {
public:
  nsIToolbarItem * mItem;
  PRInt32          mGap;
  PRBool           mStretchable;

  ToolbarLayoutInfo(nsIToolbarItem * aItem, PRInt32 aGap, PRBool isStretchable) 
  {
    mItem = aItem;
    mGap  = aGap;
    mStretchable = isStretchable;
    NS_ADDREF(aItem);
  }

  virtual ~ToolbarLayoutInfo() 
  {
    NS_RELEASE(mItem);
  }

};

/**************************************************************
  Now define the token deallocator class...
 **************************************************************/
/*class CToolbarItemInfoDeallocator: public nsDequeFunctor{
public:
  virtual void* operator()(void* anObject) {
    ToolbarLayoutInfo* aItem = (ToolbarLayoutInfo*)anObject;
    delete aItem;
    return 0;
  }
};
static CNavTokenDeallocator gItemInfoKiller;*/



//--------------------------------------------------------------------
//-- nsToolbar Constructor
//--------------------------------------------------------------------
nsToolbar::nsToolbar() : ChildWindow(), nsIToolbar()
{
  NS_INIT_REFCNT();

  mMargin     = 0;
  mWrapMargin = 15;
  mHGap       = 0;
  mVGap       = 0;

  mLastItemIsRightJustified = PR_FALSE;
  mNextLastItemIsStretchy   = PR_FALSE;
  mDoDrawFullBorder         = PR_FALSE;
  mWrapItems                = PR_FALSE;
  mDoHorizontalLayout       = PR_TRUE;

  mToolbarMgr = nsnull;

  //mItemDeque = new nsDeque(gItemInfoKiller);
  mItems = (ToolbarLayoutInfo **) new PRInt32[gMaxInfoItems];
  mNumItems = 0;
}

//--------------------------------------------------------------------
nsToolbar::~nsToolbar()
{
  NS_IF_RELEASE(mToolbarMgr);

  //delete mItemDeque;

  PRInt32 i;
  for (i=0;i<mNumItems;i++) {
    delete mItems[i];
  }
  delete[] mItems;
}

//--------------------------------------------------------------------
nsresult nsToolbar::QueryInterface(REFNSIID aIID, void** aInstancePtr)      
{                                                                        
  if (NULL == aInstancePtr) {                                            
    return NS_ERROR_NULL_POINTER;                                        
  }                                                                      
  static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);                 
  static NS_DEFINE_IID(kClassIID, kCToolbarCID);                         
  if (aIID.Equals(kCIToolbarIID)) {                                          
    *aInstancePtr = (void*) (nsIToolbar *)this;                                        
    AddRef();                                                            
    return NS_OK;                                                        
  }   
  if (aIID.Equals(kIToolbarItemIID)) {                                          
    *aInstancePtr = (void*) (nsIToolbarItem *)this;                                        
    AddRef();                                                            
    return NS_OK;                                                        
  }                                                                        
  if (aIID.Equals(kClassIID)) {                                          
    *aInstancePtr = (void*) (nsToolbar *)this;                                        
    AddRef();                                                            
    return NS_OK;                                                        
  }                                                                      
  if (aIID.Equals(kISupportsIID)) {                                      
    *aInstancePtr = (void*) (this);                        
    AddRef();                                                            
    return NS_OK;                                                        
  }                                                                      
  return (nsWindow::QueryInterface(aIID, aInstancePtr));
}

//-----------------------------------------------------
static nsEventStatus PR_CALLBACK
HandleTabEvent(nsGUIEvent *aEvent)
{
  nsEventStatus result = nsEventStatus_eIgnore;

  nsIImageButton * button;
	if (NS_OK == aEvent->widget->QueryInterface(kIImageButtonIID,(void**)&button)) {
    result = button->HandleEvent(aEvent);

    if (aEvent->message == NS_MOUSE_LEFT_BUTTON_UP) {
      nsIWidget * widget;
      aEvent->widget->GetClientData((void *&)widget);
      if (nsnull != widget) {
        nsIToolbar * toolbar;
	      if (NS_OK == widget->QueryInterface(kIToolbarIID,(void**)&toolbar)) {
          nsIToolbarManager * toolbarMgr;
          if (NS_OK == toolbar->GetToolbarManager(toolbarMgr)) {
            toolbarMgr->CollapseToolbar(toolbar);
            NS_RELEASE(toolbarMgr);
          }
          NS_RELEASE(toolbar);
        }
      }
    }

    NS_RELEASE(button);
  }


  /*switch(aEvent->message) {
    case NS_PAINT: {
      nsRect pRect;
      nsRect r;
      nsIWidget * parent;
      parent = aEvent->widget->GetParent();
      parent->GetBounds(pRect);
      aEvent->widget->GetBounds(r);
      NS_RELEASE(parent);
      nsIRenderingContext *drawCtx = ((nsPaintEvent*)aEvent)->renderingContext;
      drawCtx->SetColor(NS_RGB(128,128,128));
      drawCtx->SetColor(NS_RGB(255,0,0));
      drawCtx->DrawLine(0, pRect.height-1, r.width, pRect.height-1);
      drawCtx->DrawLine(0, pRect.height-5, r.width, pRect.height-5);
    }
    break;
  }*/

  return result;
}



//--------------------------------------------------------------------
NS_METHOD nsToolbar::AddItem(nsIToolbarItem* anItem, PRInt32 aLeftGap, PRBool aStretchable)
{
  mItems[mNumItems++] = new ToolbarLayoutInfo(anItem, aLeftGap, aStretchable);
  return NS_OK;    
}

//--------------------------------------------------------------------
NS_METHOD nsToolbar::InsertItemAt(nsIToolbarItem* anItem, 
                                  PRInt32         aLeftGap, 
                                  PRBool          aStretchable, 
                                  PRInt32         anIndex)
{

  if ((anIndex < 0 || anIndex > mNumItems-1) && !(anIndex == 0 && mNumItems == 0)) {
    return NS_ERROR_FAILURE;
  }

  if (mNumItems > 0) {
    // Shift them down to make room
    PRInt32 i;
    PRInt32 downToInx = anIndex + 1;
    for (i=mNumItems;i>downToInx;i--) {
      mItems[i] = mItems[i-1];
    }

    // Insert the new widget
    mItems[downToInx] = new ToolbarLayoutInfo(anItem, aLeftGap, aStretchable);
  } else {
    mItems[0] = new ToolbarLayoutInfo(anItem, aLeftGap, aStretchable);
  }
    mNumItems++;

  NS_ADDREF(anItem);
  return NS_OK;    
}
//--------------------------------------------------------------------
NS_METHOD nsToolbar::GetItemAt(nsIToolbarItem*& anItem, PRInt32 anIndex)
{
  if (anIndex < 0 || anIndex > mNumItems-1) {
    anItem = nsnull;
    return NS_ERROR_FAILURE;
  }

  anItem = mItems[anIndex]->mItem;
  NS_ADDREF(anItem);
  return NS_OK;    
}

//--------------------------------------------------------------------
NS_METHOD nsToolbar::DoLayout()
{
  nsRect tbRect;
  nsWindow::GetBounds(tbRect);

  if (mDoHorizontalLayout) {
    DoHorizontalLayout(tbRect);
  } else {
    DoVerticalLayout(tbRect);
  }
  return NS_OK;
}

//--------------------------------------------------------------------
NS_METHOD nsToolbar::SetHorizontalLayout(PRBool aDoHorizontalLayout)
{
  mDoHorizontalLayout = aDoHorizontalLayout;
  return NS_OK;
}

//--------------------------------------------------------------------
void nsToolbar::DoVerticalLayout(const nsRect& aTBRect)
{
  PRInt32 i;
  PRInt32 x = mDoDrawFullBorder ? mMargin : 0;
  PRInt32 y = mMargin;

  PRInt32 maxWidth = 0;

  // First layout all the items
  for (i=0;i<mNumItems;i++) {
    PRBool isVisible;
    mItems[i]->mItem->IsVisible(isVisible);
    if (isVisible) {
      PRInt32 width, height;

      if (NS_OK != mItems[i]->mItem->GetPreferredSize(width, height)) {
        nsRect rect;
        mItems[i]->mItem->GetBounds(rect);
        width  = rect.width;
        height = rect.height;
      }
      if (!mItems[i]->mStretchable) {
        maxWidth = maxWidth > height? maxWidth:height;
      }
       
      if (((y + height + mItems[i]->mGap) > aTBRect.height) && mWrapItems) {
        y = mWrapMargin; 
        x += maxWidth;
        maxWidth = 0;
      }

      PRInt32 xLoc;
      if (mWrapItems) {
        xLoc = x;
      } else {
        xLoc = ((aTBRect.width - width) / 2);
        xLoc = xLoc > -1 ? xLoc : mMargin;
      }
      // Gap is added before hand because it is the left hand gap
      y += mItems[i]->mGap;
      mItems[i]->mItem->SetBounds(xLoc, y, width, height, PR_FALSE);
      y += width;
    }
  }

  // Right justify the last item
  PRBool rightGotJustified = PR_FALSE;

  if (mNumItems > 1 && mLastItemIsRightJustified) {
    PRInt32 index = mNumItems-1;
    PRBool isVisible;
    mItems[index]->mItem->IsVisible(isVisible);
    if (isVisible) {
      PRInt32 width, height;
      if (NS_OK != mItems[index]->mItem->GetPreferredSize(width, height)) {
        nsRect rect;
        mItems[index]->mItem->GetBounds(rect);
        width  = rect.width;
        height = rect.height;
      }
      PRInt32 yLoc = aTBRect.height - height - mItems[index]->mGap - mMargin;
      PRInt32 xLoc;
      if (mWrapItems) {
        xLoc = x;
      } else {
        xLoc = (aTBRect.width - width) / 2;
        xLoc = xLoc > -1 ? xLoc : mMargin;
      }
      mItems[index]->mItem->SetBounds(xLoc, yLoc, width, height, PR_FALSE);
      rightGotJustified = PR_TRUE;
    }
  }

  // Make the next to the last item strechy
  if (mNumItems > 1 && mNextLastItemIsStretchy) {
    PRInt32 lastIndex     = mNumItems-1;
    PRInt32 nextLastIndex = mNumItems-2;

    if (!rightGotJustified) { // last item is not visible, so stretch to end
      nsRect nextLastRect;
      mItems[nextLastIndex]->mItem->GetBounds(nextLastRect);
      nextLastRect.height = aTBRect.height - nextLastRect.y - mMargin;
      mItems[nextLastIndex]->mItem->SetBounds(nextLastRect.x, nextLastRect.y, nextLastRect.width, nextLastRect.height, PR_TRUE);
    } else {

      PRBool isVisible;
      mItems[nextLastIndex]->mItem->IsVisible(isVisible);
      if (isVisible) { // stretch if visible
        nsRect lastRect;
        nsRect nextLastRect;

        mItems[lastIndex]->mItem->GetBounds(lastRect);
        mItems[nextLastIndex]->mItem->GetBounds(nextLastRect);

        nextLastRect.height = lastRect.y - nextLastRect.y - mItems[lastIndex]->mGap;
        mItems[nextLastIndex]->mItem->SetBounds(nextLastRect.x, nextLastRect.y, nextLastRect.width, nextLastRect.height, PR_TRUE);
      }
    }
  }

  for (i=0;i<mNumItems;i++) {
    if (mItems[i]->mStretchable) {
      nsRect rect;
      mItems[i]->mItem->GetBounds(rect);
      mItems[i]->mItem->SetBounds(rect.x, rect.y, rect.width, y+maxWidth, PR_TRUE);
    } else {
      mItems[i]->mItem->Repaint(PR_TRUE);
    }
  }

  Invalidate(PR_TRUE); // repaint toolbar
}

//--------------------------------------------------------------------
void nsToolbar::DoHorizontalLayout(const nsRect& aTBRect)
{
  PRInt32 i;
  PRInt32 x = mDoDrawFullBorder ? mMargin : 0;
  PRInt32 y = mMargin;

  PRInt32 maxHeight = 0;

  // First layout all the items
  for (i=0;i<mNumItems;i++) {
    if (i == 10) {
      int x = 0;
    }
    PRBool isVisible;
    mItems[i]->mItem->IsVisible(isVisible);
    if (isVisible) {
      PRInt32 width, height;

      if (NS_OK != mItems[i]->mItem->GetPreferredSize(width, height)) {
        nsRect rect;
        mItems[i]->mItem->GetBounds(rect);
        width  = rect.width;
        height = rect.height;
      }
       
      if (((x + width + mItems[i]->mGap) > aTBRect.width) && mWrapItems) {
        x = mMargin + mWrapMargin; 
        y += maxHeight;
        maxHeight = 0;
      }
      if (!mItems[i]->mStretchable) {
        maxHeight = maxHeight > height? maxHeight:height;
      }

      PRInt32 yLoc;
      if (mWrapItems) {
        yLoc = y;
      } else {
        yLoc = ((aTBRect.height - height) / 2);
        yLoc = yLoc > -1 ? yLoc : mMargin;
      }
      // Gap is added before hand because it is the left hand gap
      // Don't set the bounds on the last item if it is right justified
      x += mItems[i]->mGap;
      if (((i == (mNumItems-1) && !mLastItemIsRightJustified)) || (i != (mNumItems-1))) {
        mItems[i]->mItem->SetBounds(x, yLoc, width, height, PR_FALSE);
      }
      x += width;
    }
  }

  // Right justify the last item
  PRBool rightGotJustified = PR_FALSE;

  if (mNumItems > 1 && mLastItemIsRightJustified) {
    PRInt32 index = mNumItems-1;
    PRBool isVisible;
    mItems[index]->mItem->IsVisible(isVisible);
    if (isVisible) {
      PRInt32 width, height;
      if (NS_OK != mItems[index]->mItem->GetPreferredSize(width, height)) {
        nsRect rect;
        mItems[index]->mItem->GetBounds(rect);
        width  = rect.width;
        height = rect.height;
      }
      PRInt32 xLoc = aTBRect.width - width - mItems[index]->mGap - mMargin;
      PRInt32 yLoc;
      if (mWrapItems) {
        yLoc = y;
      } else {
        yLoc = (aTBRect.height - height) / 2;
        yLoc = yLoc > -1 ? yLoc : mMargin;
      }
      mItems[index]->mItem->SetBounds(xLoc, yLoc, width, height, PR_FALSE);
      rightGotJustified = PR_TRUE;
    }
  }

  // Make the next to the last item strechy
  if (mNumItems > 1 && mNextLastItemIsStretchy) {
    PRInt32 lastIndex     = mNumItems-1;
    PRInt32 nextLastIndex = mNumItems-2;

    if (!rightGotJustified) { // last item is not visible, so stretch to end
      nsRect nextLastRect;
      mItems[nextLastIndex]->mItem->GetBounds(nextLastRect);
      nextLastRect.width = aTBRect.width - nextLastRect.x - mMargin;
      mItems[nextLastIndex]->mItem->SetBounds(nextLastRect.x, nextLastRect.y, nextLastRect.width, nextLastRect.height, PR_TRUE);
    } else {

      PRBool isVisible;
      mItems[nextLastIndex]->mItem->IsVisible(isVisible);
      if (isVisible) { // stretch if visible
        nsRect lastRect;
        nsRect nextLastRect;

        mItems[lastIndex]->mItem->GetBounds(lastRect);
        mItems[nextLastIndex]->mItem->GetBounds(nextLastRect);

        nextLastRect.width = lastRect.x - nextLastRect.x - mItems[lastIndex]->mGap;
        mItems[nextLastIndex]->mItem->SetBounds(nextLastRect.x, nextLastRect.y, nextLastRect.width, nextLastRect.height, PR_TRUE);
      }
    }
  }

  for (i=0;i<mNumItems;i++) {
    if (mItems[i]->mStretchable) {
      nsRect rect;
      mItems[i]->mItem->GetBounds(rect);
      mItems[i]->mItem->SetBounds(rect.x, rect.y, rect.width, y+maxHeight, PR_TRUE);
    } else {
      mItems[i]->mItem->Repaint(PR_TRUE);
    }
  }

  Invalidate(PR_TRUE); // repaint toolbar
}

//--------------------------------------------------------------------
NS_METHOD nsToolbar::SetLastItemIsRightJustified(const PRBool & aState)
{
  mLastItemIsRightJustified = aState;
  return NS_OK;    
}

//--------------------------------------------------------------------
NS_METHOD nsToolbar::SetNextLastItemIsStretchy(const PRBool & aState)
{
  mNextLastItemIsStretchy = aState;
  return NS_OK;    
}

//--------------------------------------------------------------------
NS_METHOD nsToolbar::GetPreferredSize(PRInt32& aWidth, PRInt32& aHeight)
{
  nsRect rect;
  nsWindow::GetBounds(rect);

  if (mDoHorizontalLayout) {
    aWidth  = mMargin*2;
    aHeight = 0;
    PRInt32 i;
    for (i=0;i<mNumItems;i++) {
      PRBool isVisible;
      mItems[i]->mItem->IsVisible(isVisible);
      if (isVisible) {
        PRInt32 width;
        PRInt32 height;
        if (NS_OK == mItems[i]->mItem->GetPreferredSize(width, height)) {
          aWidth += width + mItems[i]->mGap;
          if (!mItems[i]->mStretchable) {
            aHeight = height > aHeight? height : aHeight;
          }
        } else {
          nsRect rect;
          mItems[i]->mItem->GetBounds(rect);
          aWidth += rect.width + mItems[i]->mGap;
          if (!mItems[i]->mStretchable) {
            aHeight = rect.height > aHeight? rect.height : aHeight;
          }
        }
      }
    }
    aWidth += mHGap;

    if (aHeight == 0) {
      aHeight = 32;
    }
    aHeight += (mMargin*2);
  } else {
    aHeight = mMargin*2;
    aWidth  = 0;
    PRInt32 i;
    for (i=0;i<mNumItems;i++) {
      PRBool isVisible;
      mItems[i]->mItem->IsVisible(isVisible);
      if (isVisible) {
        PRInt32 width;
        PRInt32 height;
        if (NS_OK == mItems[i]->mItem->GetPreferredSize(width, height)) {
          aHeight += height + mItems[i]->mGap;
          if (!mItems[i]->mStretchable) {
            aWidth = width > aWidth? width : aWidth;
          }
        } else {
          nsRect rect;
          mItems[i]->mItem->GetBounds(rect);
          aHeight += rect.height + mItems[i]->mGap;
          if (!mItems[i]->mStretchable) {
            aWidth = rect.width > aWidth? rect.width : aWidth;
          }
        }
      }
    }
    aHeight += mHGap;

    if (aWidth == 0) {
      aWidth = 32;
    }
  }

  return NS_OK;
}

//--------------------------------------------------------------------
NS_METHOD nsToolbar::SetHGap(PRInt32 aGap)
{
  mHGap = aGap;
  return NS_OK;    
}

//--------------------------------------------------------------------
NS_METHOD nsToolbar::SetVGap(PRInt32 aGap)
{
  mVGap = aGap;
  return NS_OK;    
}

//--------------------------------------------------------------------
NS_METHOD nsToolbar::SetMargin(PRInt32 aMargin)
{
  mMargin = aMargin;
  return NS_OK;    
}

//--------------------------------------------------------------------
NS_METHOD nsToolbar::SetDrawFullBorder(PRBool aDoDrawFullBorder)
{
  mDoDrawFullBorder = aDoDrawFullBorder;
  return NS_OK;
}

//--------------------------------------------------------------------
NS_METHOD nsToolbar::SetToolbarManager(nsIToolbarManager * aToolbarManager)
{
  mToolbarMgr = aToolbarManager;
  NS_ADDREF(mToolbarMgr);

  return NS_OK;
}

//--------------------------------------------------------------------
NS_METHOD nsToolbar::GetToolbarManager(nsIToolbarManager *& aToolbarManager)
{
  aToolbarManager = mToolbarMgr;
  NS_ADDREF(aToolbarManager);
  return NS_OK;
}

//--------------------------------------------------------------------
//
// Resize this component
//
//-------------------------------------------------------------------------
NS_METHOD nsToolbar::Resize(PRUint32 aWidth, PRUint32 aHeight, PRBool aRepaint)
{
  nsresult result = nsWindow::Resize(aWidth, aHeight, aRepaint);
  DoLayout();
  return result;
}

    
//-------------------------------------------------------------------------
//
// Resize this component
//
//-------------------------------------------------------------------------
NS_METHOD nsToolbar::Resize(PRUint32 aX,
                      PRUint32 aY,
                      PRUint32 aWidth,
                      PRUint32 aHeight,
                      PRBool   aRepaint)
{
  nsresult result = nsWindow::Resize(aX, aY, aWidth, aHeight, aRepaint);
  DoLayout();
  return result;
}

//-------------------------------------------------------------------------
NS_METHOD nsToolbar::Repaint(PRBool aIsSynchronous)

{
  Invalidate(aIsSynchronous);
  return NS_OK;
}
    
//--------------------------------------------------------------------
NS_METHOD nsToolbar::SetBounds(PRUint32 aWidth, PRUint32 aHeight, PRBool aRepaint)
{
  return Resize(aWidth, aHeight, aRepaint);
}
    
//-------------------------------------------------------------------------
NS_METHOD nsToolbar::SetBounds(PRUint32 aX,
                                          PRUint32 aY,
                                          PRUint32 aWidth,
                                          PRUint32 aHeight,
                                          PRBool   aRepaint)
{
  return Resize(aX, aY, aWidth, aHeight, aRepaint);
}

//-------------------------------------------------------------------------
NS_METHOD nsToolbar::SetVisible(PRBool aState) 
{
  nsWindow::Show(aState);
  return NS_OK;
}

//-------------------------------------------------------------------------
NS_METHOD nsToolbar::SetLocation(PRUint32 aX, PRUint32 aY) 
{
  nsWindow::Move(aX, aY);
  return NS_OK;
}


//-------------------------------------------------------------------------
NS_METHOD nsToolbar::IsVisible(PRBool & aState) 
{
  nsWindow::IsVisible(aState);
  return NS_OK;
}

//--------------------------------------------------------------------
NS_METHOD nsToolbar::SetPreferredSize(PRInt32 aWidth, PRInt32 aHeight)
{
  nsWindow::SetPreferredSize(aWidth, aHeight);
  return NS_OK;
}

//-------------------------------------------------------------------
NS_METHOD nsToolbar::GetBounds(nsRect & aRect)
{
  nsWindow::GetBounds(aRect);
  return NS_OK;
}

//-----------------------------------------------------------------------------
nsEventStatus nsToolbar::HandleEvent(nsGUIEvent *aEvent) 
{

  if (aEvent->message == NS_PAINT) {
    nsRect r;
    aEvent->widget->GetBounds(r);
    r.x = 0;
    r.y = 0;
    nsIRenderingContext *drawCtx = ((nsPaintEvent*)aEvent)->renderingContext;
    drawCtx->SetColor(aEvent->widget->GetBackgroundColor());
    drawCtx->FillRect(r);
    r.width--;

    nsRect rect(r);
    // draw top & left
    drawCtx->SetColor(NS_RGB(255,255,255));
    drawCtx->DrawLine(0,0,rect.width,0);
    if (mDoDrawFullBorder) {
      drawCtx->DrawLine(0,0,0,rect.height);
    }

    // draw bottom & right
    drawCtx->SetColor(NS_RGB(128,128,128));
    drawCtx->DrawLine(0,rect.height-1,rect.width,rect.height-1);
    if (mDoDrawFullBorder) {
      drawCtx->DrawLine(rect.width,0,rect.width,rect.height);
    }
  }



  return nsEventStatus_eIgnore;
  
}

//-------------------------------------------------------------------
NS_METHOD nsToolbar::SetWrapping(PRBool aDoWrap)
{
  mWrapItems = aDoWrap;
  return NS_OK;
}

//-------------------------------------------------------------------
NS_METHOD nsToolbar::GetWrapping(PRBool & aDoWrap)
{
  aDoWrap = mWrapItems;
  return NS_OK;
}

//-------------------------------------------------------------------
NS_METHOD nsToolbar::GetPreferredConstrainedSize(PRInt32& aSuggestedWidth, PRInt32& aSuggestedHeight, 
                                                 PRInt32& aWidth,          PRInt32& aHeight)
{
  nsRect rect;
  nsWindow::GetBounds(rect);

  PRInt32 rows        = 1;
  PRInt32 calcSize    = mMargin;
  PRInt32 maxSize     = 0;
  PRInt32 maxRowSize  = 0;
  PRInt32 currentSize = mMargin; // the current height of the "growing" toolbar

  PRInt32 i;
  // Loop throgh each item in the toolbar
  // Skip it if it is not visible
  for (i=0;i<mNumItems;i++) {
    PRBool isVisible;
    mItems[i]->mItem->IsVisible(isVisible);
    if (isVisible) {
      PRInt32 width;
      PRInt32 height;
      // Get the item's Preferred width, height
      if (NS_OK != mItems[i]->mItem->GetPreferredSize(width, height)) {
        nsRect rect;
        mItems[i]->mItem->GetBounds(rect);
        width = rect.width;
        height = rect.height;
      }

      // If it is greater than the suggested width than add 1 to the number of rows
      // and start the x over
      if (mDoHorizontalLayout) {
        if ((calcSize + width + mItems[i]->mGap) > aSuggestedWidth) {
          currentSize += maxRowSize;
          maxRowSize = 0;
          calcSize   = mMargin + mWrapMargin + width + mItems[i]->mGap;
        } else {
          calcSize += width + mItems[i]->mGap;
        }
        if (!mItems[i]->mStretchable) {
          maxRowSize = height > maxRowSize? height : maxRowSize;
        }
      } else { // vertical
        if (calcSize + height + mItems[i]->mGap > aSuggestedHeight) {
          currentSize += maxRowSize;
          maxRowSize   = 0;
          calcSize     = mMargin;
        } else {
          calcSize += height + mItems[i]->mGap;
        }
        if (!mItems[i]->mStretchable) {
          maxRowSize = width > maxRowSize? width : maxRowSize;
        }
      }
    } // isVisible
  }

  // Now set the width and height accordingly
  if (mDoHorizontalLayout) {
    aWidth  = aSuggestedWidth;
    aHeight = currentSize + mMargin + maxRowSize;
  } else {
    aHeight  = aSuggestedHeight;
    aWidth = currentSize + mMargin + maxRowSize;
  }

  return NS_OK;
}

