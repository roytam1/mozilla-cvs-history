/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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

#include "nsLookAndFeel.h"
#include "windows.h"
 
static NS_DEFINE_IID(kILookAndFeelIID, NS_ILOOKANDFEEL_IID);

NS_IMPL_ADDREF(nsLookAndFeel)
NS_IMPL_RELEASE(nsLookAndFeel)

nsLookAndFeel::nsLookAndFeel() : nsObject(), nsILookAndFeel()
{
  NS_INIT_REFCNT();

}

//-------------------------------------------------------------------------
//
// Query interface implementation
//
//-------------------------------------------------------------------------

nsresult nsLookAndFeel::QueryInterface(const nsIID& aIID, void** aInstancePtr)
{
  nsresult result = nsObject::QueryInterface(aIID, aInstancePtr);

  if (result == NS_NOINTERFACE && aIID.Equals(kILookAndFeelIID)) {
      *aInstancePtr = (void*) ((nsILookAndFeel*)this);
      NS_ADDREF_THIS();
      result = NS_OK;
  }

  return result;
}

nsLookAndFeel::~nsLookAndFeel()
{
}

NS_IMETHODIMP nsLookAndFeel::GetColor(const nsColorID aID, nscolor &aColor)
{
    nsresult res = NS_OK;
    int idx;
    switch (aID) {
    case eColor_WindowBackground:
        idx = COLOR_WINDOW;
        break;
    case eColor_WindowForeground:
        idx = COLOR_WINDOWTEXT;
        break;
    case eColor_WidgetBackground:
        idx = COLOR_BTNFACE;
        break;
    case eColor_WidgetForeground:
        idx = COLOR_BTNTEXT;
        break;
    case eColor_WidgetSelectBackground:
        idx = COLOR_HIGHLIGHT;
        break;
    case eColor_WidgetSelectForeground:
        idx = COLOR_HIGHLIGHTTEXT;
        break;
    case eColor_Widget3DHighlight:
        idx = COLOR_BTNHIGHLIGHT;
        break;
    case eColor_Widget3DShadow:
        idx = COLOR_BTNSHADOW;
        break;
    case eColor_TextBackground:
        idx = COLOR_WINDOW;
        break;
    case eColor_TextForeground:
        idx = COLOR_WINDOWTEXT;
        break;
    case eColor_TextSelectBackground:
        idx = COLOR_HIGHLIGHT;
        break;
    case eColor_TextSelectForeground:
        idx = COLOR_HIGHLIGHTTEXT;
        break;
    default:
        idx = COLOR_WINDOW;    
        break;
    }

    aColor = ::GetSysColor(idx);

    return res;
}
  
NS_IMETHODIMP nsLookAndFeel::GetMetric(const nsMetricID aID, PRInt32 & aMetric)
{
    nsresult res = NS_OK;
    switch (aID) {
    case eMetric_WindowTitleHeight:
        aMetric = ::GetSystemMetrics(SM_CYCAPTION);
        break;
    case eMetric_WindowBorderWidth:
        aMetric = ::GetSystemMetrics(SM_CXFRAME);
        break;
    case eMetric_WindowBorderHeight:
        aMetric = ::GetSystemMetrics(SM_CYFRAME);
        break;
    case eMetric_Widget3DBorder:
        aMetric = ::GetSystemMetrics(SM_CXEDGE);
        break;
    case eMetric_TextFieldHeight:
        aMetric = 24;
        break;
    default:
        aMetric = -1;
        res = NS_ERROR_FAILURE;
    }
    return res;
}

