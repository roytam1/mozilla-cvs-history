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
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

#include <stdio.h>
#include "nsIDragService.h"
#include "nsWidgetsCID.h"
#include "nsNativeDragTarget.h"
#include "nsDragService.h"
#include "nsIServiceManager.h"
#include "nsCOMPtr.h"

#include "nsIWidget.h"
#include "nsWindow.h"

#if (_MSC_VER == 1100)
#define INITGUID
#include "objbase.h"
DEFINE_OLEGUID(IID_IDropTarget, 0x00000122L, 0, 0);
DEFINE_OLEGUID(IID_IUnknown, 0x00000000L, 0, 0);
#endif

#define DRAG_DEBUG 0

/* Define Class IDs */
static NS_DEFINE_IID(kCDragServiceCID,  NS_DRAGSERVICE_CID);

/* Define Interface IDs */
static NS_DEFINE_IID(kIDragServiceIID, NS_IDRAGSERVICE_IID);

// This is cached for Leave notification
static POINTL gDragLastPoint;




/*
 * class nsNativeDragTarget
 */
//-----------------------------------------------------
// construction
//-----------------------------------------------------
nsNativeDragTarget::nsNativeDragTarget(nsIWidget * aWnd)
: m_cRef(0), mWindow(aWnd), mCanMove(PR_TRUE)
{
  mHWnd    = (HWND)mWindow->GetNativeData(NS_NATIVE_WINDOW);

  /*
   * Create/Get the DragService that we have implemented
   */
  nsresult rv = nsServiceManager::GetService(kCDragServiceCID,
                                             kIDragServiceIID,
                                             (nsISupports**)&mDragService);
}


//-----------------------------------------------------
// destruction
//-----------------------------------------------------
nsNativeDragTarget::~nsNativeDragTarget()
{
  nsServiceManager::ReleaseService(kCDragServiceCID, mDragService);
}

//-----------------------------------------------------
// IUnknown methods - see iunknown.h for documentation
//-----------------------------------------------------
STDMETHODIMP nsNativeDragTarget::QueryInterface(REFIID riid, void** ppv)
{
    *ppv=NULL;

	 if (IID_IUnknown == riid || IID_IDropTarget == riid)
        *ppv=this;

	 if (NULL!=*ppv) {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
	 }

    return ResultFromScode(E_NOINTERFACE);
}


//-----------------------------------------------------
STDMETHODIMP_(ULONG) nsNativeDragTarget::AddRef(void)
{
    return ++m_cRef;
}

//-----------------------------------------------------
STDMETHODIMP_(ULONG) nsNativeDragTarget::Release(void)
{
	 if (0 != --m_cRef)
        return m_cRef;

    delete this;
    return 0;
}


//-----------------------------------------------------
void nsNativeDragTarget::GetGeckoDragAction(LPDATAOBJECT pData, DWORD grfKeyState, LPDWORD pdwEffect, PRUint32 * aGeckoAction) 
{
  // Check if we can link from this data object as well.
  PRBool canLink = PR_FALSE;
  if ( pData )
    canLink = (S_OK == ::OleQueryLinkFromData(pData) ? PR_TRUE : PR_FALSE);

  // Default is move if we can, in fact drop here,
  // and if the drop source supports a move operation.
  if (mCanMove) {
    *pdwEffect    = DROPEFFECT_MOVE;
    *aGeckoAction = nsIDragService::DRAGDROP_ACTION_MOVE;
  } else {
    *aGeckoAction = nsIDragService::DRAGDROP_ACTION_COPY;
    *pdwEffect    = DROPEFFECT_COPY;
  }

  // Given the key modifiers figure out what state we are in for both
  // the native system and Gecko
  if (grfKeyState & MK_CONTROL) {
    if (canLink && (grfKeyState & MK_SHIFT)) {
      *aGeckoAction = nsIDragService::DRAGDROP_ACTION_LINK;
      *pdwEffect    = DROPEFFECT_LINK;
    } else {
      *aGeckoAction = nsIDragService::DRAGDROP_ACTION_COPY;
      *pdwEffect    = DROPEFFECT_COPY;
    }
  }
}


PRBool IsKeyDown ( char key ) ;

inline
PRBool IsKeyDown ( char key )
{
  return GetKeyState(key) & 0x80 ? PR_TRUE : PR_FALSE;
}


//-----------------------------------------------------
void nsNativeDragTarget::DispatchDragDropEvent(PRUint32 aEventType, 
                                               POINTL   aPT)
{
  nsEventStatus status;
  nsMouseEvent event;
  event.eventStructType = NS_DRAGDROP_EVENT;

  nsWindow * win = NS_STATIC_CAST(nsWindow *, mWindow);
  win->InitEvent(event, aEventType);
  POINT cpos;

  cpos.x = aPT.x;
  cpos.y = aPT.y;

  if (mHWnd != NULL) {
    ::ScreenToClient(mHWnd, &cpos);
    event.point.x = cpos.x;
    event.point.y = cpos.y;
  } else {
    event.point.x = 0;
    event.point.y = 0;
  }

  event.isShift   = IsKeyDown(NS_VK_SHIFT);
  event.isControl = IsKeyDown(NS_VK_CONTROL);
  event.isMeta    = PR_FALSE;
  event.isAlt     = IsKeyDown(NS_VK_ALT);

  mWindow->DispatchEvent(&event, status);
}

//-----------------------------------------------------
void nsNativeDragTarget::ProcessDrag(LPDATAOBJECT pData,
                                     PRUint32     aEventType, 
                                     DWORD        grfKeyState,
												             POINTL       pt, 
                                     DWORD*       pdwEffect)
{
  // Before dispatching the event make sure we have the correct drop action set
  PRUint32 geckoAction;
  GetGeckoDragAction(pData, grfKeyState, pdwEffect, &geckoAction);

  // Set the current action into the Gecko specific type
  nsCOMPtr<nsIDragSession> currSession;
  mDragService->GetCurrentSession ( getter_AddRefs(currSession) );
  currSession->SetDragAction(geckoAction);

  // Dispatch the event into Gecko
  DispatchDragDropEvent(aEventType, pt);

  // Now get the cached Drag effect from the drag service
  // the data memeber should have been set by who ever handled the 
  // nsGUIEvent or nsIDOMEvent
  PRBool canDrop;
  currSession->GetCanDrop(&canDrop);
  if (!canDrop)
    *pdwEffect = DROPEFFECT_NONE;

  // Clear the cached value
  currSession->SetCanDrop(PR_FALSE);
}


//-----------------------------------------------------
// IDropTarget methods
//-----------------------------------------------------


STDMETHODIMP nsNativeDragTarget::DragEnter(LPDATAOBJECT pIDataSource, 
                                           DWORD        grfKeyState,
												                   POINTL       pt, 
                                           DWORD*       pdwEffect)
{
  if (DRAG_DEBUG) printf("DragEnter\n");

	if (mDragService) {

    // tell the drag service about this drag (it may have come from an
    // outside app).
    mDragService->StartDragSession();

    // Remember if this operation allows a move.
    mCanMove = (*pdwEffect) & DROPEFFECT_MOVE;

    // Set the native data object into drag service
    //
    // This cast is ok because in the constructor we created a 
    // the actual implementation we wanted, so we know this is
    // a nsDragService. It should be a private interface, though.
    nsDragService * winDragService = NS_STATIC_CAST(nsDragService *, mDragService);
    winDragService->SetIDataObject(pIDataSource);

    // Now process the native drag state and then dispatch the event
    ProcessDrag(pIDataSource, NS_DRAGDROP_ENTER, grfKeyState, pt, pdwEffect);

		return S_OK;
	}
  else
		return ResultFromScode(E_FAIL);
}


//-----------------------------------------------------
STDMETHODIMP nsNativeDragTarget::DragOver(DWORD   grfKeyState, 
                                          POINTL  pt, 
                                          LPDWORD pdwEffect)
{
  if (DRAG_DEBUG) printf("DragOver\n");
	if (mDragService) {
    // Now process the native drag state and then dispatch the event
    ProcessDrag(nsnull, NS_DRAGDROP_OVER, grfKeyState, pt, pdwEffect);
		return S_OK;
	} 
  else
		return ResultFromScode(E_FAIL);
}


//-----------------------------------------------------
STDMETHODIMP nsNativeDragTarget::DragLeave() {

  if (DRAG_DEBUG) printf("DragLeave\n");
	if (mDragService) {

    // dispatch the event into Gecko
    DispatchDragDropEvent(NS_DRAGDROP_EXIT, gDragLastPoint);

    // tell the drag service that we're done with it
    mDragService->EndDragSession();
		return S_OK;
	} 
  else
		return ResultFromScode(E_FAIL);

}


//-----------------------------------------------------
STDMETHODIMP nsNativeDragTarget::Drop(LPDATAOBJECT pData, 
                                      DWORD        grfKeyState,
										                  POINTL       aPT, 
                                      LPDWORD      pdwEffect)
{
	if (mDragService) {
    // Set the native data object into the drag service
    //
    // This cast is ok because in the constructor we created a 
    // the actual implementation we wanted, so we know this is
    // a nsDragService (but it should still be a private interface)
    nsDragService * winDragService = NS_STATIC_CAST(nsDragService *, mDragService);
    winDragService->SetIDataObject(pData);

    // Now process the native drag state and then dispatch the event
    ProcessDrag(pData, NS_DRAGDROP_DROP, grfKeyState, aPT, pdwEffect);

    // tell the drag service we're done with the session
    mDragService->EndDragSession();
    return S_OK;
	} 
  else
		return ResultFromScode(E_FAIL);

}


