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
/* Name:		<Xfe/MenuUtil.h>										*/
/* Description:	Menu/RowColum misc utilities header.					*/
/* Author:		Ramiro Estrugo <ramiro@netscape.com>					*/
/*																		*/
/*----------------------------------------------------------------------*/

#ifndef _XfeMenuUtil_h_							/* start MenuUtil.h		*/
#define _XfeMenuUtil_h_

#include <Xfe/BasicDefines.h>

XFE_BEGIN_CPLUSPLUS_PROTECTION

/*----------------------------------------------------------------------*/
/*																		*/
/* Menus																*/
/*																		*/
/*----------------------------------------------------------------------*/
extern void
XfeMenuPositionXY				(Widget			menu,
								 Position		x_root,
								 Position		y_root);
/*----------------------------------------------------------------------*/
extern int
XfeMenuItemPositionIndex		(Widget			item);
/*----------------------------------------------------------------------*/
extern Widget
XfeMenuItemAtPosition			(Widget			menu,
								 int			position);
/*----------------------------------------------------------------------*/
extern Boolean
XfeMenuIsFull					(Widget			menu);
/*----------------------------------------------------------------------*/
extern Widget
XfeMenuFindLastMoreMenu			(Widget			menu,
								 String			more_button_name);
/*----------------------------------------------------------------------*/
extern Widget
XfeMenuGetMoreButton			(Widget			menu,
								 String			more_button_name);
/*----------------------------------------------------------------------*/
extern Widget
XfeCascadeGetSubMenu			(Widget			w);
/*----------------------------------------------------------------------*/
extern unsigned char
XfeMenuType						(Widget			menu);
/*----------------------------------------------------------------------*/


/*----------------------------------------------------------------------*/
/*																		*/
/* Option menus															*/
/*																		*/
/*----------------------------------------------------------------------*/
extern Boolean
XfeMenuIsOptionMenu				(Widget			menu);
/*----------------------------------------------------------------------*/
extern void
XfeOptionMenuSetItem			(Widget			menu,
								 Cardinal		i);
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*																		*/
/* Menu item functions.													*/
/*																		*/
/*----------------------------------------------------------------------*/
extern Widget
XfeMenuItemNextItem				(Widget			item);
/*----------------------------------------------------------------------*/
extern Widget
XfeMenuItemPreviousItem			(Widget			item);

/*----------------------------------------------------------------------*/
/*																		*/
/* Display grabbed access.												*/
/*																		*/
/*----------------------------------------------------------------------*/
extern Boolean
XfeDisplayIsUserGrabbed			(Widget				w);
/*----------------------------------------------------------------------*/
extern void
XfeDisplaySetUserGrabbed		(Widget				w,
								 Boolean			grabbed);
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*																		*/
/* Creation of menu hierarchies											*/
/*																		*/
/*----------------------------------------------------------------------*/
extern Widget
XfeMenuCreateCascadeItem		(Widget			menu,
								 Widget			pulldown,
								 String			cascade_name,
								 WidgetClass	cascade_widget_class,
								 Boolean		manage_cascade,
								 ArgList		cascade_av,
								 Cardinal		cascade_ac);
/*----------------------------------------------------------------------*/
extern void
XfeMenuCreatePulldownPane		(Widget			menu,
								 Widget			visual_widget,
								 String			cascade_name,
								 String			pulldown_name,
								 WidgetClass	cascade_widget_class,
								 Boolean		manage_cascade,
								 ArgList		cascade_av,
								 Cardinal		cascade_ac,
								 Widget *		cascade_out,
								 Widget *		pulldown_out);
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*																		*/
/* Destruction of menu hierarchies										*/
/*																		*/
/*----------------------------------------------------------------------*/
extern void
XfeDestroyMenuWidgetTree		(WidgetList		children,
								 int			num_children,
								 Boolean		skip_private_components);
/*----------------------------------------------------------------------*/

XFE_END_CPLUSPLUS_PROTECTION

#endif											/* end MenuUtil.h		*/
