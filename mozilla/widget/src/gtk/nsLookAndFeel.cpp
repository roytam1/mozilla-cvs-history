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
#include <gtk/gtk.h>

static NS_DEFINE_IID(kILookAndFeelIID, NS_ILOOKANDFEEL_IID);

#define GDK_COLOR_TO_NS_RGB(c) \
  ((nscolor) NS_RGB(c.red, c.green, c.blue))

NS_IMPL_ISUPPORTS(nsLookAndFeel, kILookAndFeelIID);

//-------------------------------------------------------------------------
//
// Query interface implementation
//
//-------------------------------------------------------------------------
nsLookAndFeel::nsLookAndFeel()
{
  NS_INIT_REFCNT();
}

nsLookAndFeel::~nsLookAndFeel()
{
}

NS_IMETHODIMP nsLookAndFeel::GetColor(const nsColorID aID, nscolor &aColor)
{
    GtkStyle *style = gtk_style_new();  // get the default styles
    nsresult res = NS_OK;
    switch (aID) {
    case eColor_WindowBackground:
        aColor = GDK_COLOR_TO_NS_RGB(style->bg[GTK_STATE_NORMAL]);
        break;
    case eColor_WindowForeground:
        aColor = GDK_COLOR_TO_NS_RGB(style->fg[GTK_STATE_NORMAL]);
        break;
    case eColor_WidgetBackground:
        aColor = GDK_COLOR_TO_NS_RGB(style->bg[GTK_STATE_NORMAL]);
        break;
    case eColor_WidgetForeground:
        aColor = GDK_COLOR_TO_NS_RGB(style->fg[GTK_STATE_NORMAL]);
        break;
    case eColor_WidgetSelectBackground:
        aColor = GDK_COLOR_TO_NS_RGB(style->bg[GTK_STATE_SELECTED]);
        break;
    case eColor_WidgetSelectForeground:
        aColor = GDK_COLOR_TO_NS_RGB(style->fg[GTK_STATE_SELECTED]);
        break;
    case eColor_Widget3DHighlight:
        aColor = NS_RGB(0xa0,0xa0,0xa0);
        break;
    case eColor_Widget3DShadow:
        aColor = NS_RGB(0x40,0x40,0x40);
        break;
    case eColor_TextBackground:
        aColor = GDK_COLOR_TO_NS_RGB(style->bg[GTK_STATE_NORMAL]);
        break;
    case eColor_TextForeground: 
        aColor = GDK_COLOR_TO_NS_RGB(style->fg[GTK_STATE_NORMAL]);
        break;
    case eColor_TextSelectBackground:
        aColor = GDK_COLOR_TO_NS_RGB(style->bg[GTK_STATE_SELECTED]);
        aColor;
    case eColor_TextSelectForeground:
        aColor = GDK_COLOR_TO_NS_RGB(style->text[GTK_STATE_SELECTED]);
        break;
    default:
        aColor = GDK_COLOR_TO_NS_RGB(style->fg[GTK_STATE_NORMAL]);
        res    = NS_ERROR_FAILURE;
        break;
    }
    gtk_style_unref(style);

    return res;
}

NS_IMETHODIMP nsLookAndFeel::GetMetric(const nsMetricID aID, PRInt32 & aMetric)
{
    GtkStyle *style = gtk_style_new();  // get the default styles
    nsresult res = NS_OK;
    switch (aID) {
    case eMetric_WindowTitleHeight:
        aMetric = 0;
        break;
    case eMetric_WindowBorderWidth:
//        aMetric = 4;
        aMetric = style->klass->xthickness;
        break;
    case eMetric_WindowBorderHeight:
//        aMetric = 4;
        aMetric = style->klass->ythickness;
        break;
    case eMetric_Widget3DBorder:
// XXX look into this
        aMetric = 4;
        break;
    case eMetric_TextFieldHeight:
	// FIXME we need to allow the entry to size itself based on the font
        aMetric = 20;
        break;
    case eMetric_ButtonHorizontalInsidePaddingNavQuirks:
        aMetric = 20;
        break;
    case eMetric_ButtonHorizontalInsidePaddingOffsetNavQuirks:
        aMetric = 0;
        break;
    case eMetric_TextHorizontalInsideMinimumPadding:
        aMetric = 6;
        break;
    case eMetric_TextVerticalInsidePadding:
        aMetric = 10;
        break;
    case eMetric_TextShouldUseVerticalInsidePadding:
        aMetric = 1;
        break;
    case eMetric_TextShouldUseHorizontalInsideMinimumPadding:
        aMetric = 0;
        break;
    case eMetric_ListShouldUseHorizontalInsideMinimumPadding:
        aMetric = 1;
        break;
    case eMetric_ListHorizontalInsideMinimumPadding:
        aMetric = 7;
        break;
    case eMetric_ListShouldUseVerticalInsidePadding:
        aMetric = 1;
        break;
    case eMetric_ListVerticalInsidePadding:
        aMetric = 1;
        break;
    default:
        aMetric = 0;
        res     = NS_ERROR_FAILURE;
    }

    gtk_style_unref(style);

    return res;
}

NS_IMETHODIMP nsLookAndFeel::GetMetric(const nsMetricFloatID aID, float & aMetric)
{
  nsresult res = NS_OK;
  switch (aID) {
    case eMetricFloat_TextFieldVerticalInsidePadding:
        aMetric = 10.0f;
        break;
    case eMetricFloat_TextFieldHorizontalInsidePadding:
        aMetric = 10000.0f; // large number on purpose so minimum padding is used
        break;
    case eMetricFloat_TextAreaVerticalInsidePadding:
        aMetric = 10.0f;    
        break;
    case eMetricFloat_TextAreaHorizontalInsidePadding:
        aMetric = 10000.0f; // large number on purpose so minimum padding is used
        break;
    case eMetricFloat_ListVerticalInsidePadding:
        aMetric = 1.0f;
        break;
    case eMetricFloat_ListHorizontalInsidePadding:
        aMetric = 7.0f;
        break;
    case eMetricFloat_ButtonVerticalInsidePadding:
        aMetric = 0.5f;
        break;
    case eMetricFloat_ButtonHorizontalInsidePadding:
        aMetric = 0.5f;
        break;
    default:
        aMetric = -1.0;
        res = NS_ERROR_FAILURE;
    }
  return res;
}

