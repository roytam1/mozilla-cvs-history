/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

/*----------------------------------------------------------------------*/
/*																		*/
/* Name:		<Xfe/Chrome.h>											*/
/* Description:	XfeChrome widget public header file.					*/
/* Author:		Ramiro Estrugo <ramiro@netscape.com>					*/
/*																		*/
/*----------------------------------------------------------------------*/

#ifndef _XfeChrome_h_							/* start Chrome.h		*/
#define _XfeChrome_h_

#include <Xfe/ToolBox.h>
#include <Xfe/DashBoard.h>
#include <Xm/RowColumn.h>

XFE_BEGIN_CPLUSPLUS_PROTECTION

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeChrome resource names												*/
/*																		*/
/*----------------------------------------------------------------------*/
#define XmNtoolBox						"toolBox"
#define XmNcenterView					"centerView"
#define XmNtopView						"topView"
#define XmNbottomView					"bottomView"
#define XmNleftView						"leftView"
#define XmNrightView					"rightView"
#define XmNdashBoard					"dashBoard"
#define XmNchromeChildType				"chromeChildType"

#define XmCChromeChildType				"ChromeChildType"

#define XmRChromeChildType				"ChromeChildType"

/*----------------------------------------------------------------------*/
/*																		*/
/* XmRChromeChildType													*/
/*																		*/
/*----------------------------------------------------------------------*/
enum
{
	XmCHROME_BOTTOM_VIEW,
	XmCHROME_CENTER_VIEW,
	XmCHROME_DASH_BOARD,
	XmCHROME_IGNORE,
	XmCHROME_LEFT_VIEW,
	XmCHROME_MENU_BAR,
	XmCHROME_RIGHT_VIEW,
	XmCHROME_TOOL_BOX,
	XmCHROME_TOP_VIEW
};

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeBox class names													*/
/*																		*/
/*----------------------------------------------------------------------*/
externalref WidgetClass xfeChromeWidgetClass;

typedef struct _XfeChromeClassRec *			XfeChromeWidgetClass;
typedef struct _XfeChromeRec *				XfeChromeWidget;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeBox subclass test macro											*/
/*																		*/
/*----------------------------------------------------------------------*/
#define XfeIsChrome(w)	XtIsSubclass(w,xfeChromeWidgetClass)

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeChrome public methods												*/
/*																		*/
/*----------------------------------------------------------------------*/
extern Widget
XfeCreateChrome					(Widget				pw,
								 String				name,
								 Arg *				av,
								 Cardinal			ac);
/*----------------------------------------------------------------------*/
extern Widget
XfeChromeGetComponent			(Widget				w,
								 unsigned char		component);
/*----------------------------------------------------------------------*/

XFE_END_CPLUSPLUS_PROTECTION

#endif											/* end Chrome.h			*/
