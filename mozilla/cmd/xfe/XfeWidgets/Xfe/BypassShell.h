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
/* Name:		<Xfe/BypassShell.h>										*/
/* Description:	XfeBypassShell widget public header file.				*/
/* Author:		Ramiro Estrugo <ramiro@netscape.com>					*/
/*																		*/
/*----------------------------------------------------------------------*/

#ifndef _XfeBypassShell_h_						/* start BypassShell.h	*/
#define _XfeBypassShell_h_

#include <Xfe/Xfe.h>
#include <Xfe/Manager.h>

XFE_BEGIN_CPLUSPLUS_PROTECTION

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeBypassShell resource names										*/
/*																		*/
/*----------------------------------------------------------------------*/
#define XmNbeforeRealizeCallback			"beforeRealizeCallback"
#define XmNfirstMapCallback					"firstMapCallback"

#define XmNignoreExposures				"ignoreExposures"

#define XmCIgnoreExposures				"IgnoreExposures"

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeBypassShell class names											*/
/*																		*/
/*----------------------------------------------------------------------*/
externalref WidgetClass xfeBypassShellWidgetClass;

typedef struct _XfeBypassShellClassRec *		XfeBypassShellWidgetClass;
typedef struct _XfeBypassShellRec *				XfeBypassShellWidget;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeBypassShell subclass test macro									*/
/*																		*/
/*----------------------------------------------------------------------*/
#define XfeIsBypassShell(w)	XtIsSubclass(w,xfeBypassShellWidgetClass)

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeBypassShell public methods										*/
/*																		*/
/*----------------------------------------------------------------------*/
extern Widget
XfeCreateBypassShell			(Widget		pw,
								 String		name,
								 Arg *		av,
								 Cardinal	ac);
/*----------------------------------------------------------------------*/
extern void
XfeBypassShellUpdateSize		(Widget		w);
/*----------------------------------------------------------------------*/

XFE_END_CPLUSPLUS_PROTECTION

#endif											/* end BypassShell.h	*/
