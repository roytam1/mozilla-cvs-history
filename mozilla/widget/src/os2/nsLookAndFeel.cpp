/*
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
 * License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is the Mozilla OS/2 libraries.
 *
 * The Initial Developer of the Original Code is John Fairhurst,
 * <john_fairhurst@iname.com>.  Portions created by John Fairhurst are
 * Copyright (C) 1999 John Fairhurst. All Rights Reserved.
 *
 * Contributor(s): 
 *
 */

#include "nsLookAndFeel.h"
#include "nsColor.h"
#include "nsWidgetDefs.h"

#include <stdio.h>
 
// XPCom scaffolding
NS_IMPL_ISUPPORTS(nsLookAndFeel, nsILookAndFeel::GetIID())

nsLookAndFeel::nsLookAndFeel()
{
   NS_INIT_REFCNT();
}

// Colours
NS_IMETHODIMP nsLookAndFeel::GetColor(const nsColorID aID, nscolor &aColor)
{
   int idx = 0;
   switch (aID)
   {
      case eColor_WindowBackground:       idx = SYSCLR_BACKGROUND;       break;
      case eColor_WindowForeground:       idx = SYSCLR_WINDOWTEXT;       break;
      case eColor_WidgetBackground:       idx = SYSCLR_BUTTONMIDDLE;     break;
      case eColor_WidgetForeground:       idx = SYSCLR_WINDOWTEXT;       break;
      case eColor_WidgetSelectBackground: idx = SYSCLR_HILITEBACKGROUND; break;
      case eColor_WidgetSelectForeground: idx = SYSCLR_HILITEFOREGROUND; break;
      case eColor_Widget3DHighlight:      idx = SYSCLR_BUTTONLIGHT;      break;
      case eColor_Widget3DShadow:         idx = SYSCLR_BUTTONDARK;       break;
      case eColor_TextBackground:         idx = SYSCLR_ENTRYFIELD;       break;
      case eColor_TextForeground:         idx = SYSCLR_WINDOWTEXT;       break;
      case eColor_TextSelectBackground:   idx = SYSCLR_HILITEBACKGROUND; break;
      case eColor_TextSelectForeground:   idx = SYSCLR_HILITEFOREGROUND; break;

      default: 
         NS_ASSERTION(0, "Bad system colour");
         break;
   }

   long lColor = WinQuerySysColor( HWND_DESKTOP, idx, 0);

   RGB2 *pRGB2 = (RGB2*) &lColor;

   aColor = NS_RGB( pRGB2->bRed, pRGB2->bGreen, pRGB2->bBlue);

   return NS_OK;
}

// metrics
NS_IMETHODIMP nsLookAndFeel::GetMetric(const nsMetricID aID, PRInt32 & aMetric)
{
   long svalue = 0;
   aMetric = 0;

   switch( aID)
   {
      case eMetric_WindowTitleHeight: svalue = SV_CYTITLEBAR; break;
      case eMetric_WindowBorderWidth: svalue = SV_CXSIZEBORDER; break;
      case eMetric_WindowBorderHeight: svalue = SV_CYSIZEBORDER; break;
      case eMetric_Widget3DBorder: svalue = SV_CXBORDER; break;
      case eMetric_TextFieldHeight: aMetric = gModuleData.lHtEntryfield; break; 

      //
      // These are copied from the Win32 version; changes welcome
      //
      case eMetric_TextVerticalInsidePadding:
         aMetric = 0;
         break;
      case eMetric_TextShouldUseVerticalInsidePadding:
         aMetric = 0;
         break;
      case eMetric_TextHorizontalInsideMinimumPadding:
         aMetric = 3;
         break;
      case eMetric_TextShouldUseHorizontalInsideMinimumPadding:
        aMetric = 1;
        break;

      case eMetric_ButtonHorizontalInsidePaddingNavQuirks:
         aMetric = 10;
         break;
      case eMetric_ButtonHorizontalInsidePaddingOffsetNavQuirks:
         aMetric = 8;
         break;

      case eMetric_CheckboxSize:
         aMetric = 12;
         break;
      case eMetric_RadioboxSize:
         aMetric = 12;
         break;

      case eMetric_ListHorizontalInsideMinimumPadding:
         aMetric = 3;
         break;
      case eMetric_ListShouldUseHorizontalInsideMinimumPadding:
         aMetric = 0;
         break;
      case eMetric_ListVerticalInsidePadding:
         aMetric = 0;
         break;
      case eMetric_ListShouldUseVerticalInsidePadding:
         aMetric = 0;
         break;

      default:
         NS_ASSERTION( 0, "Bad metric");
         break;
   }

   if( svalue != 0)
      aMetric = WinQuerySysValue( HWND_DESKTOP, svalue);

   return NS_OK;
}

//
// These are copied from the Win32 version; changes welcome
//
// float metrics
NS_IMETHODIMP nsLookAndFeel::GetMetric( const nsMetricFloatID aID,
                                        float &aMetric)
{
   nsresult res = NS_OK;
   switch( aID)
   {
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
         break;
   }
   return res;
}
