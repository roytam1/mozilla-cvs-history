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
#include <Pt.h>
#include "nsFont.h"
#include "nsPhWidgetLog.h"
 
NS_DEFINE_IID(kILookAndFeelIID, NS_ILOOKANDFEEL_IID);
NS_IMPL_ISUPPORTS(nsLookAndFeel,kILookAndFeelIID);

nsLookAndFeel::nsLookAndFeel() : nsILookAndFeel()
{
  NS_INIT_REFCNT();
}

nsLookAndFeel::~nsLookAndFeel()
{
  PR_LOG(PhWidLog, PR_LOG_DEBUG, ("nsLookAndFeel::~nsLookAndFeel - %p destroyed\n", this));

}

NS_IMETHODIMP nsLookAndFeel::GetColor(const nsColorID aID, nscolor &aColor)
{
  nsresult res = NS_OK;

  PR_LOG(PhWidLog, PR_LOG_DEBUG, ("nsLookAndFeel::GetColor mRefCnt=<%d>\n", mRefCnt));

  switch( aID )
  {
    case eColor_WindowBackground:
	    PR_LOG(PhWidLog, PR_LOG_DEBUG, ("nsLookAndFeel::GetColor - WindowBackground\n"));
        aColor = NS_RGB(255,255,255);	/* White */
        break;
    case eColor_WindowForeground:
	    PR_LOG(PhWidLog, PR_LOG_DEBUG, ("nsLookAndFeel::GetColor - WindowForeground\n"));
        aColor = NS_RGB(0,0,0);	        /* Black */
        break;
    case eColor_WidgetBackground:
	    PR_LOG(PhWidLog, PR_LOG_DEBUG, ("nsLookAndFeel::GetColor - WidgetBackground\n"));
        aColor = NS_RGB(128,128,128);	/* Gray */
        break;
    case eColor_WidgetForeground:
	    PR_LOG(PhWidLog, PR_LOG_DEBUG, ("nsLookAndFeel::GetColor - WidgetForeground\n"));
        aColor = NS_RGB(0,0,0);	        /* Black */
        break;
    case eColor_WidgetSelectBackground:
	    PR_LOG(PhWidLog, PR_LOG_DEBUG, ("nsLookAndFeel::GetColor - WidgetSelectBackground\n"));
        aColor = NS_RGB(200,200,200);	/* Dark Gray */
        break;
    case eColor_WidgetSelectForeground:
	    PR_LOG(PhWidLog, PR_LOG_DEBUG, ("nsLookAndFeel::GetColor - WidgetSelectForeground\n"));
        aColor = NS_RGB(0,0,0);	        /* Black */
        break;
    case eColor_Widget3DHighlight:
	    PR_LOG(PhWidLog, PR_LOG_DEBUG, ("nsLookAndFeel::GetColor - Widget3DHighlight\n"));
        aColor = NS_RGB(255,255,255);	/* White */
        break;
    case eColor_Widget3DShadow:
	    PR_LOG(PhWidLog, PR_LOG_DEBUG, ("nsLookAndFeel::GetColor - Widget3DHighlight\n"));
        aColor = NS_RGB(200,200,200);	/* Dark Gray */
        break;
    case eColor_TextBackground:
	    PR_LOG(PhWidLog, PR_LOG_DEBUG, ("nsLookAndFeel::GetColor - TextBackground\n"));
        aColor = NS_RGB(255,255,255);	/* White */
        break;
    case eColor_TextForeground:
	    PR_LOG(PhWidLog, PR_LOG_DEBUG, ("nsLookAndFeel::GetColor - TextForeground\n"));
        aColor = NS_RGB(0,0,0);	        /* Black */
        break;
    case eColor_TextSelectBackground:
	    PR_LOG(PhWidLog, PR_LOG_DEBUG, ("nsLookAndFeel::GetColor - TextSelectBackground\n"));
        aColor = NS_RGB(0,0,255);	        /* Blue */
        break;
    case eColor_TextSelectForeground:
	    PR_LOG(PhWidLog, PR_LOG_DEBUG, ("nsLookAndFeel::GetColor - TextSelectForeground\n"));
        aColor = NS_RGB(255,255,255);	/* White */
        break;
    default:
	    PR_LOG(PhWidLog, PR_LOG_DEBUG, ("nsLookAndFeel::GetColor - Unknown Request!\n"));
        res = NS_ERROR_FAILURE;
        break;
    }

  return res;
}
  
NS_IMETHODIMP nsLookAndFeel::GetMetric(const nsMetricID aID, PRInt32 & aMetric)
{
  nsresult res = NS_OK;

  PR_LOG(PhWidLog, PR_LOG_DEBUG, ("nsLookAndFeel::GetMetric\n"));

  switch( aID )
  {
  case eMetric_WindowTitleHeight:
    aMetric = 20; // REVISIT - HACK!
    break;
  case eMetric_WindowBorderWidth:
    aMetric = 8; // REVISIT - HACK!
    break;
  case eMetric_WindowBorderHeight:
    aMetric = 8; // REVISIT - HACK!
    break;
  case eMetric_Widget3DBorder:
    aMetric = 2;
    break;
  case eMetric_TextFieldHeight:
    aMetric = 23;
    break;
  case eMetric_ButtonHorizontalInsidePaddingNavQuirks:
    aMetric = 0; // REVISIT - HACK!
    break;
  case eMetric_ButtonHorizontalInsidePaddingOffsetNavQuirks:
    aMetric = 0; // REVISIT - HACK!
    break;
  case eMetric_CheckboxSize:
    aMetric = 10; // REVISIT - HACK!
    break;
  case eMetric_RadioboxSize:
    aMetric = 10; // REVISIT - HACK!
    break;
  case eMetric_TextHorizontalInsideMinimumPadding:
    aMetric = 1; // REVISIT - HACK!
    break;
  case eMetric_TextVerticalInsidePadding:
    aMetric = 0; // REVISIT - HACK!
    break;
  case eMetric_TextShouldUseVerticalInsidePadding:
    aMetric = 0; // REVISIT - HACK!
    break;
  case eMetric_TextShouldUseHorizontalInsideMinimumPadding:
    aMetric = 0; // REVISIT - HACK!
    break;
  case eMetric_ListShouldUseHorizontalInsideMinimumPadding:
    aMetric = 0; // REVISIT - HACK!
    break;
  case eMetric_ListHorizontalInsideMinimumPadding:
    aMetric = 0; // REVISIT - HACK!
    break;
  case eMetric_ListShouldUseVerticalInsidePadding:
    aMetric = 0; // REVISIT - HACK!
    break;
  case eMetric_ListVerticalInsidePadding:
    aMetric = 0; // REVISIT - HACK!
    break;
  default:
    aMetric = -1;
    res = NS_ERROR_FAILURE;
    PR_LOG(PhWidLog, PR_LOG_DEBUG, ("nsLookAndFeel::GetMetric - Unknown ID!\n"));
    break;
  }

  return res;
}

NS_IMETHODIMP nsLookAndFeel::GetMetric(const nsMetricFloatID aID, float & aMetric)
{
  nsresult res = NS_OK;

  PR_LOG(PhWidLog, PR_LOG_DEBUG, ("nsLookAndFeel::GetMetric with float aID=<%d>\n", aID));

  switch( aID )
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
        PR_LOG(PhWidLog, PR_LOG_DEBUG, ("nsLookAndFeel::GetMetric (float) - Unknown ID!\n"));
        res = NS_ERROR_FAILURE;
    }

  return res;
}


