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

static NS_DEFINE_IID(kILookAndFeelIID, NS_ILOOKANDFEEL_IID);

NS_IMPL_ISUPPORTS(nsLookAndFeel, kILookAndFeelIID)

nsLookAndFeel::nsLookAndFeel() : nsILookAndFeel()
{
    NS_INIT_REFCNT();

#ifdef XLIB_WIDGET_NOISY
    printf("nsLookAndFeel::nsLookAndFeel()\n");
#endif
}

nsLookAndFeel::~nsLookAndFeel()
{
#ifdef XLIB_WIDGET_NOISY
    printf("nsLookAndFeel::~nsLookAndFeel()\n");
#endif

}

NS_IMETHODIMP nsLookAndFeel::GetColor(const nsColorID aID, nscolor &aColor)
{
#ifdef XLIB_WIDGET_NOISY
    printf("nsLookAndFeel::GetColor()\n");
#endif

    return NS_OK;
}

NS_IMETHODIMP nsLookAndFeel::GetMetric(const nsMetricID aID, PRInt32 & aMetric)
{
#ifdef XLIB_WIDGET_NOISY
    printf("nsLookAndFeel::GetMetric()\n");
#endif

    // XXX These are not complete!
    nsresult res = NS_OK;
    switch (aID) { 
    case eMetric_WindowTitleHeight:
        aMetric = 20;
        break;
    case eMetric_WindowBorderWidth:
        aMetric = 1;
        break;
    case eMetric_WindowBorderHeight:
        aMetric = 1;
        break;
    case eMetric_Widget3DBorder:
        aMetric = 1;
        break;
    case eMetric_TextFieldHeight:
        aMetric = 24;
        break;
    case eMetric_ButtonHorizontalInsidePaddingNavQuirks:
        aMetric = 10;
        break;
    case eMetric_ButtonHorizontalInsidePaddingOffsetNavQuirks:
        aMetric = 8;
        break;
    case eMetric_CheckboxSize:
        aMetric = 15;
        break;
    case eMetric_RadioboxSize:
        aMetric = 15;
        break;
    case eMetric_TextHorizontalInsideMinimumPadding:
        aMetric = 3;
        break;
    case eMetric_TextVerticalInsidePadding:
        aMetric = 0;
        break;
    case eMetric_TextShouldUseVerticalInsidePadding:
        aMetric = 0;
        break;
    case eMetric_TextShouldUseHorizontalInsideMinimumPadding:
        aMetric = 1;
        break;
    case eMetric_ListShouldUseHorizontalInsideMinimumPadding:
        aMetric = 0;
        break;
    case eMetric_ListHorizontalInsideMinimumPadding:
        aMetric = 3;
        break;
    case eMetric_ListShouldUseVerticalInsidePadding:
        aMetric = 0;
        break;
    case eMetric_ListVerticalInsidePadding:
        aMetric = 0;
        break;
    default:
        aMetric = -1;
        res = NS_ERROR_FAILURE;
    }
    return res;
}

NS_IMETHODIMP nsLookAndFeel::GetMetric(const nsMetricFloatID aID, float & aMetric)
{
#ifdef XLIB_WIDGET_NOISY
    printf("nsLookAndFeel::GetMetric()\n");
#endif

    // XXX this is not complete
    nsresult res = NS_OK;
    switch (aID) {
    case eMetricFloat_TextFieldVerticalInsidePadding:
        aMetric = 0.25f;
        break;
    case eMetricFloat_TextFieldHorizontalInsidePadding:
        aMetric = 0.95f;
        break;
    case eMetricFloat_TextAreaVerticalInsidePadding:
        aMetric = 0.40f;
        break;
    case eMetricFloat_TextAreaHorizontalInsidePadding:
        aMetric = 0.40f;
        break;
    case eMetricFloat_ListVerticalInsidePadding:
        aMetric = 0.10f;
        break;
    case eMetricFloat_ListHorizontalInsidePadding:
        aMetric = 0.40f;
        break;
    case eMetricFloat_ButtonVerticalInsidePadding:
        aMetric = 0.25f;
        break;
    case eMetricFloat_ButtonHorizontalInsidePadding:
        aMetric = 0.25f;
        break;
    default:
        aMetric = -1.0;
        res = NS_ERROR_FAILURE;
    }
    return res;
}
