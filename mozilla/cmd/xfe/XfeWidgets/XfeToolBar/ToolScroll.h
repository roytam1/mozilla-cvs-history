/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is the Xfe Widgets.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * In addition, as a special exception to the GNU GPL, the copyright holders
 * give permission to link the code of this program with the Motif and Open
 * Motif libraries (or with modified versions of these that use the same
 * license), and distribute linked combinations including the two. You
 * must obey the GNU General Public License in all respects for all of
 * the code used other than linking with Motif/Open Motif. If you modify
 * this file, you may extend this exception to your version of the file,
 * but you are not obligated to do so. If you do not wish to do so,
 * delete this exception statement from your version.
 *
 * ***** END LICENSE BLOCK ***** */

/*----------------------------------------------------------------------*/
/*																		*/
/* Name:		<Xfe/ToolScroll.h>										*/
/* Description:	XfeToolScroll widget public header file.				*/
/* Author:		Ramiro Estrugo <ramiro@netscape.com>					*/
/*																		*/
/*----------------------------------------------------------------------*/

#ifndef _XfeToolScroll_h_						/* start ToolScroll.h	*/
#define _XfeToolScroll_h_

#include <Xfe/Oriented.h>
#include <Xfe/Logo.h>
#include <Xfe/ToolBar.h>

XFE_BEGIN_CPLUSPLUS_PROTECTION

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeToolScroll resource names											*/
/*																		*/
/*----------------------------------------------------------------------*/
#define XmNarrowDisplayPolicy				"arrowDisplayPolicy"
#define XmNarrowPlacement					"arrowPlacement"
#define XmNbackwardArrow					"backwardArrow"
#define XmNclipArea							"clipArea"
#define XmNclipShadowThickness				"clipShadowThickness"
#define XmNclipShadowType					"clipShadowType"
#define XmNforwardArrow						"forwardArrow"
#define XmNtoolBarPosition					"toolBarPosition"

#define XmCArrowDisplayPolicy				"ArrowDisplayPolicy"
#define XmCArrowPlacement					"ArrowPlacement"
#define XmCBackwardArrow					"BackwardArrow"
#define XmCClipArea							"ClipArea"
#define XmCForwardArrow						"ForwardArrow"
#define XmCToolBarPosition					"ToolBarPosition"

#define XmRArrowDisplayPolicy				XmRScrollBarDisplayPolicy
#define XmRToolScrollArrowPlacement			"ToolScrollArrowPlacement"

/*----------------------------------------------------------------------*/
/*																		*/
/* XmRarrowPlacement													*/
/*																		*/
/*----------------------------------------------------------------------*/
enum
{
	XmTOOL_SCROLL_ARROW_PLACEMENT_BOTH,
	XmTOOL_SCROLL_ARROW_PLACEMENT_END,
	XmTOOL_SCROLL_ARROW_PLACEMENT_START
};

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeBox class names													*/
/*																		*/
/*----------------------------------------------------------------------*/
externalref WidgetClass xfeToolScrollWidgetClass;

typedef struct _XfeToolScrollClassRec *		XfeToolScrollWidgetClass;
typedef struct _XfeToolScrollRec *			XfeToolScrollWidget;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeBox subclass test macro											*/
/*																		*/
/*----------------------------------------------------------------------*/
#define XfeIsToolScroll(w)	XtIsSubclass(w,xfeToolScrollWidgetClass)

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeToolScroll Public Methods											*/
/*																		*/
/*----------------------------------------------------------------------*/
extern Widget
XfeCreateToolScroll				(Widget		pw,
								 String		name,
								 Arg *		av,
								 Cardinal	ac);
/*----------------------------------------------------------------------*/

XFE_END_CPLUSPLUS_PROTECTION

#endif											/* end ToolScroll.h		*/
