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

#include "nsTextWidget.h"

NS_IMPL_ADDREF(nsTextWidget)
NS_IMPL_RELEASE(nsTextWidget)

nsTextWidget::nsTextWidget() : nsTextHelper()
{
  NS_INIT_REFCNT();
  name = "nsTextWidget";
}

nsTextWidget::~nsTextWidget()
{
}

nsresult nsTextWidget::QueryInterface(const nsIID& aIID, void** aInstancePtr)
{
  nsresult result = nsWidget::QueryInterface(aIID, aInstancePtr);
  
  static NS_DEFINE_IID(kInsTextWidgetIID, NS_ITEXTWIDGET_IID);
  if (result == NS_NOINTERFACE && aIID.Equals(kInsTextWidgetIID)) {
    *aInstancePtr = (void*) ((nsITextWidget*)this);
    NS_ADDREF_THIS();
    result = NS_OK;
  }
  
  return result;
}

NS_METHOD nsTextWidget::Paint(nsIRenderingContext& aRenderingContext,
                              const nsRect& aDirtyRect)
{
  return NS_OK;
}
