/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License.  You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are Copyright (C) 1998
 * Netscape Communications Corporation.  All Rights Reserved.
 */

#include "nsToolbarDragListener.h"
#include "nsToolbarFrame.h"
#include "nsIDOMDragListener.h"
#include "nsIDOMEventReceiver.h"
#include "nsIDOMEventListener.h"

// Drag & Drop, Clipboard
#include "nsIServiceManager.h"
#include "nsWidgetsCID.h"
#include "nsIClipboard.h"
#include "nsIDragService.h"
#include "nsIDragSession.h"
#include "nsITransferable.h"
#include "nsIFormatConverter.h"
#include "nsCOMPtr.h"
#include "nsIDOMUIEvent.h"
#include "nsIPresContext.h"
#include "nsIContent.h"
#include "nsIDOMNode.h"
#include "nsIDOMElement.h"
#include "nsXULAtoms.h"

#include "nsIViewManager.h"
#include "nsIView.h"

// Drag & Drop, Clipboard Support
static NS_DEFINE_CID(kCDragServiceCID,         NS_DRAGSERVICE_CID);
static NS_DEFINE_CID(kCTransferableCID,        NS_TRANSFERABLE_CID);
static NS_DEFINE_IID(kCDataFlavorCID,          NS_DATAFLAVOR_CID);
static NS_DEFINE_IID(kCXIFFormatConverterCID,  NS_XIFFORMATCONVERTER_CID);

static NS_DEFINE_IID(kISupportsIID,  NS_ISUPPORTS_IID);
static NS_DEFINE_IID(kIDOMEventReceiverIID,  NS_IDOMEVENTRECEIVER_IID);

#include "nsISupportsArray.h"

/*
 * nsToolbarDragListener implementation
 */

NS_IMPL_ADDREF(nsToolbarDragListener)

NS_IMPL_RELEASE(nsToolbarDragListener)


////////////////////////////////////////////////////////////////////////
nsToolbarDragListener::nsToolbarDragListener() 
{
  NS_INIT_REFCNT();
  mPresContext = nsnull;
  mMouseDown = PR_FALSE;
  mMouseDrag = PR_FALSE;
}

////////////////////////////////////////////////////////////////////////
nsToolbarDragListener::~nsToolbarDragListener() 
{
}

////////////////////////////////////////////////////////////////////////
nsresult
nsToolbarDragListener::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
  if (nsnull == aInstancePtr) {
    return NS_ERROR_NULL_POINTER;
  }

  if (aIID.Equals(kIDOMEventReceiverIID)) {
    *aInstancePtr = (void*)(nsIDOMEventListener*)(nsIDOMMouseMotionListener*)this;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  if (aIID.Equals(nsIDOMDragListener::GetIID())) {
    *aInstancePtr = (void*)(nsIDOMDragListener*)this;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  if (aIID.Equals(nsIDOMMouseMotionListener::GetIID())) {
    *aInstancePtr = (void*)(nsIDOMMouseMotionListener*)this;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  if (aIID.Equals(nsIDOMMouseListener::GetIID())) {
    *aInstancePtr = (void*)(nsIDOMMouseListener*)this;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  if (aIID.Equals(kISupportsIID)) {                                      
    *aInstancePtr = (void*)(nsISupports*)(nsIDOMMouseMotionListener*)this;                        
    NS_ADDREF_THIS();                                                    
    return NS_OK;                                                        
  }
  return NS_NOINTERFACE;
}

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
static void ForceDrawFrame(nsIFrame * aFrame)
{
  if (aFrame == nsnull) {
    return;
  }
  nsRect    rect;
  nsIView * view;
  nsPoint   pnt;
  aFrame->GetOffsetFromView(pnt, &view);
  aFrame->GetRect(rect);
  rect.x = pnt.x;
  rect.y = pnt.y;
  if (view) {
    nsCOMPtr<nsIViewManager> viewMgr;
    view->GetViewManager(*getter_AddRefs(viewMgr));
    if (viewMgr)
      viewMgr->UpdateView(view, rect, NS_VMREFRESH_AUTO_DOUBLE_BUFFER);
  }

}

////////////////////////////////////////////////////////////////////////
nsresult
nsToolbarDragListener::HandleEvent(nsIDOMEvent* aEvent)
{
  return NS_OK;
}



////////////////////////////////////////////////////////////////////////
nsresult
nsToolbarDragListener::DragEnter(nsIDOMEvent* aDragEvent)
{
  mCurrentDropLoc = -1;

  nsIDragService* dragService;
  nsresult rv = nsServiceManager::GetService(kCDragServiceCID,
                                             nsIDragService::GetIID(),
                                             (nsISupports **)&dragService);
  if (NS_OK == rv) {
    nsCOMPtr<nsIDragSession> dragSession(do_QueryInterface(dragService));

    nsAutoString toolbarItemFlavor(TOOLBARITEM_MIME);
    if (dragSession && (NS_OK == dragSession->IsDataFlavorSupported(&toolbarItemFlavor))) {
      dragSession->SetCanDrop(PR_TRUE);
    } else {
      rv = NS_ERROR_BASE; // don't consume event
    }
    
    nsServiceManager::ReleaseService(kCDragServiceCID, dragService);
  }
  return rv;
}


////////////////////////////////////////////////////////////////////////
PRBool
nsToolbarDragListener::IsOnToolbarItem(nsIDOMEvent* aDragEvent, nscoord& aXLoc, PRBool& aIsLegalChild)
{
  aIsLegalChild = PR_FALSE;

  nsCOMPtr<nsIDOMUIEvent> uiEvent(do_QueryInterface(aDragEvent));
  PRInt32 x,y = 0;
  uiEvent->GetClientX(&x);
  uiEvent->GetClientY(&y);

  if (nsnull == mPresContext) {
    return NS_OK;
  }

  float p2t;
  mPresContext->GetScaledPixelsToTwips(&p2t);
  nscoord onePixel = NSIntPixelsToTwips(1, p2t);
  nscoord xp = NSIntPixelsToTwips(x, p2t);
  nscoord yp = NSIntPixelsToTwips(y, p2t);
  nsPoint pnt(xp,yp);

  // get the toolbar's rect
  nsRect tbRect;
  mToolbar->GetRect(tbRect);

  nscoord   count = 0;
  nsIFrame* childFrame;
  nsRect    rect;             // child frame's rect
  nsRect    prevRect(-1, -1, 0, 0);
  PRBool    found = PR_FALSE;

  mToolbar->FirstChild(nsnull, &childFrame); 
  while (nsnull != childFrame) {    

    // The mouse coords are in the toolbar's domain
    // Get child's rect and adjust to the toolbar's domain
    childFrame->GetRect(rect);
    rect.MoveBy(tbRect.x, tbRect.y);

    // remember the previous child x location
    if (pnt.x < rect.x && prevRect.x == -1) {
      prevRect = rect;
    }

    // now check to see if the mouse inside an items bounds
    if (rect.Contains(pnt)) {
      nsCOMPtr<nsIContent> content;
      nsresult result = childFrame->GetContent(getter_AddRefs(content));
      if (NS_OK == result) {
        nsCOMPtr<nsIAtom> tag;
        content->GetTag(*getter_AddRefs(tag));

        // for now I am checking for both titlebutton and toolbar items
        // but the check for titlebutton should be removed in the future
        if (tag.get() == nsXULAtoms::titledbutton || tag.get() == nsXULAtoms::toolbaritem) {

          // now check for natural order
          PRBool naturalOrder = PR_FALSE;
          nsCOMPtr<nsIDOMElement> domElement;
          domElement = do_QueryInterface(content);
          if (nsnull != domElement ) {
            nsCOMPtr<nsIAtom> tag;
            nsAutoString value;
            domElement->GetAttribute(nsAutoString("naturalorder"), value);
            naturalOrder = value.Equals("true");

          } else {
            printf("Not a DOM element\n");
          }
          
          found = PR_TRUE;

          PRInt32 xc = -1;
          if (naturalOrder) {
            //printf("%d   %d %d\n", pnt.x, rect.x, rect.width);
            if (pnt.x <= (rect.x + (rect.width / 4))) {
              xc = rect.x-tbRect.x;
              //printf("1st\n");
            } else if (pnt.x >= (rect.x + PRInt32(float(rect.width) *.75))) {
              xc = rect.x-tbRect.x+rect.width-onePixel;
              //printf("last\n");
            } else {
              //printf("no-op\n");
            }
          } else {
            //printf("no order\n");
            xc = rect.x-tbRect.x;
          }
          //printf("xc = %d\n", xc);
          aXLoc = xc;
          aIsLegalChild = PR_TRUE;
        }
        return PR_TRUE;
      }
    }

    nsresult rv = childFrame->GetNextSibling(&childFrame);
    NS_ASSERTION(rv == NS_OK,"failed to get next child");
    count++;
  }

  if (!found) {
    aXLoc = prevRect.x -tbRect.x;
  }

  return PR_FALSE;

}

////////////////////////////////////////////////////////////////////////
nsresult
nsToolbarDragListener::DragOver(nsIDOMEvent* aDragEvent)
{
  // now tell the drag session whether we can drop here
  nsIDragService* dragService;
  nsresult rv = nsServiceManager::GetService(kCDragServiceCID,
                                           nsIDragService::GetIID(),
                                           (nsISupports **)&dragService);
  if (NS_OK == rv) {
    nsCOMPtr<nsIDragSession> dragSession(do_QueryInterface(dragService));
    nsAutoString toolbarItemFlavor(TOOLBARITEM_MIME);
    if (dragSession && NS_OK == dragSession->IsDataFlavorSupported(&toolbarItemFlavor)) {
      dragSession->SetCanDrop(PR_TRUE);

      nscoord xLoc;
      PRBool isLegalChild;
      PRBool onChild = IsOnToolbarItem(aDragEvent, xLoc, isLegalChild);

      if (xLoc != mCurrentDropLoc) {
#ifdef TOOLBAR_DD
        mToolbar->SetDropfeedbackLocation(xLoc);
#endif
        // force the toolbar frame to redraw
        ForceDrawFrame(mToolbar);

        // cache the current drop location
        mCurrentDropLoc = xLoc;

        rv = NS_ERROR_BASE; // means I am consuming the event
      }
    }    
    nsServiceManager::ReleaseService(kCDragServiceCID, dragService);
  } else {
    rv = NS_OK; // don't consume event
  }

  // NS_OK means event is NOT consumed
  return rv; 
}


////////////////////////////////////////////////////////////////////////
nsresult
nsToolbarDragListener::DragExit(nsIDOMEvent* aDragEvent)
{
  // now tell the drag session whether we can drop here
  nsIDragService* dragService;
  nsresult rv = nsServiceManager::GetService(kCDragServiceCID,
                                           nsIDragService::GetIID(),
                                           (nsISupports **)&dragService);
  if (NS_OK == rv) {
    nsCOMPtr<nsIDragSession> dragSession(do_QueryInterface(dragService));
    nsAutoString toolbarItemFlavor(TOOLBARITEM_MIME);
    if (dragSession && NS_OK == dragSession->IsDataFlavorSupported(&toolbarItemFlavor)) {
#ifdef TOOLBAR_DD
      mToolbar->SetDropfeedbackLocation(-1); // clears drawing of marker
#endif
      ForceDrawFrame(mToolbar);
      rv = NS_ERROR_BASE; // consume event
    }
    
    nsServiceManager::ReleaseService(kCDragServiceCID, dragService);
  } else {
    rv = NS_OK; // don't consume event
  }

  return rv;
}



////////////////////////////////////////////////////////////////////////
nsresult
nsToolbarDragListener::DragDrop(nsIDOMEvent* aMouseEvent)
{
#ifdef TOOLBAR_DD
  mToolbar->SetDropfeedbackLocation(-1); // clears drawing of marker
#endif
  ForceDrawFrame(mToolbar);
  // String for doing paste
  nsString stuffToPaste;

  // Create drag service for getting state of drag
  nsIDragService* dragService;
  nsresult rv = nsServiceManager::GetService(kCDragServiceCID,
                                             nsIDragService::GetIID(),
                                             (nsISupports **)&dragService);
  if (NS_OK == rv) {
    nsCOMPtr<nsIDragSession> dragSession(do_QueryInterface(dragService));
  
    if (dragSession) {

      // Create transferable for getting the drag data
      nsCOMPtr<nsITransferable> trans;
      rv = nsComponentManager::CreateInstance(kCTransferableCID, nsnull, 
                                              nsITransferable::GetIID(), 
                                              (void**) getter_AddRefs(trans));
      if ( NS_SUCCEEDED(rv) && trans ) {
        // Add the text Flavor to the transferable, 
        // because that is the only type of data we are
        // looking for at the moment.
        nsAutoString toolbarItemMime (TOOLBARITEM_MIME);
        trans->AddDataFlavor(&toolbarItemMime);
        //trans->AddDataFlavor(mImageDataFlavor);

        // Fill the transferable with data for each drag item in succession
        PRUint32 numItems = 0; 
        if (NS_SUCCEEDED(dragSession->GetNumDropItems(&numItems))) { 

          //printf("Num Drop Items %d\n", numItems); 

          PRUint32 i; 
          for (i=0;i<numItems;++i) {
            if (NS_SUCCEEDED(dragSession->GetData(trans, i))) { 
 
              // Get the string data out of the transferable
              // Note: the transferable owns the pointer to the data
              char *str = 0;
              PRUint32 len;
              trans->GetAnyTransferData(&toolbarItemMime, (void **)&str, &len);

              // If the string was not empty then paste it in
              if (str) {
                char buf[256];
                strncpy(buf, str, len);
                buf[len] = 0;
                printf("Dropped: %s\n", buf);
                stuffToPaste.SetString(str, len);
                //mEditor->InsertText(stuffToPaste);
                dragSession->SetCanDrop(PR_TRUE);
              }

              // XXX This is where image support might go
              //void * data;
              //trans->GetTransferData(mImageDataFlavor, (void **)&data, &len);
            }
          } // foreach drag item
        }
      } // if valid transferable
    } // if valid drag session
    nsServiceManager::ReleaseService(kCDragServiceCID, dragService);
  } // if valid drag service

  return NS_ERROR_BASE; // consumes the event
}

////////////////////////////////////////////////////////////////////////
nsresult
nsToolbarDragListener::MouseMove(nsIDOMEvent* aMouseEvent)
{
  nsresult rv = NS_OK;
  //printf("nsToolbarDragListener::MouseMove mMouseDown %d  mMouseDrag %d\n", mMouseDown, mMouseDrag);
  if (mMouseDown && !mMouseDrag) {
    // Ok now check to see if we are dragging a toolbar item 
    // or the toolbar itself
    nscoord xLoc;
    PRBool isLegalChild;
    PRBool onChild = IsOnToolbarItem(aMouseEvent, xLoc, isLegalChild);

    if (onChild) {
      if (isLegalChild) {
        mMouseDrag = PR_TRUE;

        // Start Drag
        nsIDragService* dragService; 
        rv = nsServiceManager::GetService(kCDragServiceCID, 
                                          nsIDragService::GetIID(), 
                                          (nsISupports **)&dragService); 
        if (NS_OK == rv) { 
          // XXX NOTE!
          // Here you need to create a special transferable
          // for handling RDF nodes (instead of this text transferable)
          nsCOMPtr<nsITransferable> trans; 
          rv = nsComponentManager::CreateInstance(kCTransferableCID, nsnull, 
                                                    nsITransferable::GetIID(), getter_AddRefs(trans)); 
          if (NS_OK == rv && trans) {
            nsString ddFlavor;
            nsString dragText;
            if (onChild) {
              ddFlavor = TOOLBARITEM_MIME;
              dragText = "toolbar item";
            } else {
              ddFlavor = TOOLBAR_MIME;
              dragText = "toolbar";
            }
            trans->AddDataFlavor(&ddFlavor);
            PRUint32 len = dragText.Length(); 
            trans->SetTransferData(&ddFlavor, dragText.ToNewCString(), len);   // transferable consumes the data

            nsCOMPtr<nsISupportsArray> items;
            NS_NewISupportsArray(getter_AddRefs(items));
            if ( items ) {
              items->AppendElement(trans);
              dragService->InvokeDragSession(items, nsnull, nsIDragService::DRAGDROP_ACTION_COPY | nsIDragService::DRAGDROP_ACTION_MOVE);
              mMouseDown = PR_FALSE;
              mMouseDrag = PR_FALSE;
            }
          } 
          nsServiceManager::ReleaseService(kCDragServiceCID, dragService); 
        } 
      }
      rv = NS_ERROR_BASE; // consumes the event
    }
  }
  return rv; 
}

////////////////////////////////////////////////////////////////////////
nsresult
nsToolbarDragListener::DragMove(nsIDOMEvent* aMouseEvent)
{
  return NS_OK; // means I am NOT consuming event
}


////////////////////////////////////////////////////////////////////////
nsresult
nsToolbarDragListener::MouseDown(nsIDOMEvent* aMouseEvent)
{
  mMouseDown = PR_TRUE;
  return NS_OK;
}

////////////////////////////////////////////////////////////////////////
nsresult
nsToolbarDragListener::MouseUp(nsIDOMEvent* aMouseEvent)
{
  printf("nsToolbarDragListener::MouseUp\n");
  nsresult res = (mMouseDrag?NS_ERROR_BASE:NS_OK);
  mMouseDown = PR_FALSE;
  mMouseDrag = PR_FALSE;
  return res;
}

////////////////////////////////////////////////////////////////////////
nsresult
nsToolbarDragListener::MouseClick(nsIDOMEvent* aMouseEvent)
{
  return NS_OK; // means I am NOT consuming event
}

////////////////////////////////////////////////////////////////////////
nsresult
nsToolbarDragListener::MouseDblClick(nsIDOMEvent* aMouseEvent)
{
  return NS_OK; // means I am NOT consuming event
}

////////////////////////////////////////////////////////////////////////
nsresult
nsToolbarDragListener::MouseOver(nsIDOMEvent* aMouseEvent)
{
  return NS_OK; // means I am NOT consuming event
}

////////////////////////////////////////////////////////////////////////
nsresult
nsToolbarDragListener::MouseOut(nsIDOMEvent* aMouseEvent)
{
  return NS_OK; // means I am NOT consuming event
}
