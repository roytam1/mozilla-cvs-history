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

#include "nsDialog.h"
//#include "nsWindow.h"
#include "nsToolkit.h"
#include "nsColor.h"
#include "nsGUIEvent.h"
#include "nsString.h"
#include "nsStringUtil.h"
#include <windows.h>

#include "nsIAppShell.h"
#include "nsIFontMetrics.h"
#include "nsIFontCache.h"
#include "nsGUIEvent.h"
#include "nsIRenderingContext.h"
#include "nsIDeviceContext.h"
#include "nsRect.h"
#include "nsTransform2D.h"
#include "nsStringUtil.h"
#include <windows.h>
#include "nsGfxCIID.h"
#include "resource.h"
#include <commctrl.h>

NS_IMPL_ADDREF(nsDialog)
NS_IMPL_RELEASE(nsDialog)

//-------------------------------------------------------------------------
//
// nsDialog constructor
//
//-------------------------------------------------------------------------
nsDialog::nsDialog() : nsWindow(), nsIDialog()
{
  NS_INIT_REFCNT();
   /* hDlgPrint = CreateDialog (hInst, (LPCTSTR) "PrintDlgBox", hwnd, PrintDlgProc) ;
     SetDlgItemText (hDlgPrint, IDD_FNAME, szTitleName) ;

     SetAbortProc (pd.hDC, AbortProc) ;

     GetWindowText (hwnd, (PTSTR) di.lpszDocName, sizeof (PTSTR)) ;
     */


}



//-------------------------------------------------------------------------
//
// Create the proper widget
//
//-------------------------------------------------------------------------
void nsDialog::Create(nsIWidget *aParent,
                      const nsRect &aRect,
                      EVENT_CALLBACK aHandleEventFunction,
                      nsIDeviceContext *aContext,
                      nsIAppShell *aAppShell,
                      nsIToolkit *aToolkit,
                      nsWidgetInitData *aInitData)
{
  nsWindow::Create(aParent,aRect,aHandleEventFunction,aContext,aAppShell,aToolkit,aInitData);
}


//-------------------------------------------------------------------------
//
// nsDialog destructor
//
//-------------------------------------------------------------------------
nsDialog::~nsDialog()
{
}

//-------------------------------------------------------------------------
//
// Set this button label
//
//-------------------------------------------------------------------------
NS_METHOD nsDialog::SetLabel(const nsString& aText)
{
  NS_ALLOC_STR_BUF(label, aText, 256);
  VERIFY(::SetWindowText(mWnd, label));
  NS_FREE_STR_BUF(label);
  return NS_OK;
}

//-------------------------------------------------------------------------
//
// Get this button label
//
//-------------------------------------------------------------------------
NS_METHOD nsDialog::GetLabel(nsString& aBuffer)
{
  int actualSize = ::GetWindowTextLength(mWnd)+1;
  NS_ALLOC_CHAR_BUF(label, 256, actualSize);
  ::GetWindowText(mWnd, label, actualSize);
  aBuffer.SetLength(0);
  aBuffer.Append(label);
  NS_FREE_CHAR_BUF(label);
  return NS_OK;
}



//-------------------------------------------------------------------------
//
// Query interface implementation
//
//-------------------------------------------------------------------------
nsresult nsDialog::QueryInterface(const nsIID& aIID, void** aInstancePtr)
{
  nsresult result = nsWindow::QueryInterface(aIID, aInstancePtr);

  static NS_DEFINE_IID(kInsDialogIID, NS_IDIALOG_IID);
  if (result == NS_NOINTERFACE && aIID.Equals(kInsDialogIID)) {
      *aInstancePtr = (void*) ((nsIDialog*)this);
      AddRef();
      result = NS_OK;
  }

  return result;
}


//-------------------------------------------------------------------------
//
// move, paint, resizes message - ignore
//
//-------------------------------------------------------------------------
PRBool nsDialog::OnMove(PRInt32, PRInt32)
{
  return PR_FALSE;
}

PRBool nsDialog::OnPaint()
{
  return nsWindow::OnPaint();
}

PRBool nsDialog::OnResize(nsRect &aWindowRect)
{
  return nsWindow::OnResize(aWindowRect);
}

//-------------------------------------------------------------------------
//
// return the window class name and initialize the class if needed
//
//-------------------------------------------------------------------------
LPCTSTR nsDialog::WindowClass()
{
  return nsWindow::WindowClass();
}

//-------------------------------------------------------------------------
//
// return window styles
//
//-------------------------------------------------------------------------
DWORD nsDialog::WindowStyle()
{ 
  return DS_MODALFRAME;//WS_CHILD | WS_CLIPSIBLINGS; 
}

//-------------------------------------------------------------------------
//
// return window extended styles
//
//-------------------------------------------------------------------------
DWORD nsDialog::WindowExStyle()
{
  return WS_EX_DLGMODALFRAME;
}


//-------------------------------------------------------------------------
//
// get position/dimensions
//
//-------------------------------------------------------------------------

void nsDialog::GetBounds(nsRect &aRect)
{
    nsWindow::GetBounds(aRect);
}

