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

#include "nsTabWidget.h"
#include "nsToolkit.h"
#include "nsColor.h"
#include "nsGUIEvent.h"
#include "nsString.h"
#include "nsStringUtil.h"


NS_IMPL_ADDREF(nsTabWidget)
NS_IMPL_RELEASE(nsTabWidget)

//-------------------------------------------------------------------------
//
// nsTabWidget constructor
//
//-------------------------------------------------------------------------
nsTabWidget::nsTabWidget() : nsWindow()
{
  NS_INIT_REFCNT();
  // Ensure that common controls dll is loaded
#if 0
  InitCommonControls();
#endif
}

//-------------------------------------------------------------------------
//
// nsTabWidget destructor
//
//-------------------------------------------------------------------------
nsTabWidget::~nsTabWidget()
{
}

//-------------------------------------------------------------------------
//
// Query interface implementation
//
//-------------------------------------------------------------------------
nsresult nsTabWidget::QueryInterface(const nsIID& aIID, void** aInstancePtr)
{
  nsresult result = nsWindow::QueryInterface(aIID, aInstancePtr);

  static NS_DEFINE_IID(kInsTabWidgetIID, NS_ITABWIDGET_IID);
  if (result == NS_NOINTERFACE && aIID.Equals(kInsTabWidgetIID)) {
      *aInstancePtr = (void*) ((nsITabWidget*)this);
      NS_ADDREF_THIS();
      result = NS_OK;
  }

  return result;
}

//-------------------------------------------------------------------------
//
// Set the tab labels
//
//-------------------------------------------------------------------------

NS_METHOD nsTabWidget::SetTabs(PRUint32 aNumberOfTabs, const nsString aTabLabels[])
{
#if 0
  TC_ITEM tie;
  char labelTemp[256];

  for (PRUint32 i = 0; i < aNumberOfTabs; i++) {
    tie.mask = TCIF_TEXT;
    aTabLabels[i].ToCString(labelTemp, 256);
    tie.pszText = labelTemp;
    TabCtrl_InsertItem(mWnd, i, &tie);
  }
#endif
  return NS_OK;
}

//-------------------------------------------------------------------------
//
// Get the currently selected tab
//
//-------------------------------------------------------------------------


PRUint32 nsTabWidget::GetSelectedTab(PRUint32& aTabNumber)
{
#if 0
  aTabNumber = TabCtrl_GetCurSel(mWnd);
#endif
  return NS_OK;
}

//-------------------------------------------------------------------------
//
// paint message. Don't send the paint out
//
//-------------------------------------------------------------------------
PRBool nsTabWidget::OnPaint(nsRect &r)
{
  return PR_FALSE;
}

PRBool nsTabWidget::OnResize(nsRect &aWindowRect)
{
    return PR_FALSE;
}

//-------------------------------------------------------------------------
//
// get position/dimensions
//
//-------------------------------------------------------------------------

NS_METHOD nsTabWidget::GetBounds(nsRect &aRect)
{
  return nsWindow::GetBounds(aRect);
}

