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
 * either of the GNU General Public License Version 2 or later (the "GPL"),
 * or the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 * 
 * ***** END LICENSE BLOCK ***** */

/*----------------------------------------------------------------------*/
/*																		*/
/* Name:		PixmapTest.c											*/
/* Description:	Test for pixmap funcs.									*/
/* Author:		Ramiro Estrugo <ramiro@netscape.com>					*/
/*																		*/
/*----------------------------------------------------------------------*/

#include <Xfe/XfeTest.h>

#define ICON_SMALL			"../icons/taskbar/task_small_browser.xpm"
#define ICON_SMALL_ARMED	"../icons/taskbar/task_small_browser_armed.xpm"
#define ICON_SMALL_RAISED	"../icons/taskbar/task_small_browser_raised.xpm"

#define ICON				"../icons/taskbar/task_browser.xpm"
#define ICON_ARMED			"../icons/taskbar/task_browser_armed.xpm"
#define ICON_RAISED			"../icons/taskbar/task_browser_raised.xpm"

static Boolean get_icon(Widget,String,Pixmap *,Pixmap *);

/*----------------------------------------------------------------------*/
int
main(int argc,char *argv[])
{
	Widget small;
	Widget large;
	Widget form;
	Widget frame;

	Pixmap icon;
	Pixmap icon_armed;
	Pixmap icon_raised;

	Pixmap icon_mask;
	Pixmap icon_armed_mask;
	Pixmap icon_raised_mask;

	Pixmap icon_small;
	Pixmap icon_small_armed;
	Pixmap icon_small_raised;

	Pixmap icon_small_mask;
	Pixmap icon_small_armed_mask;
	Pixmap icon_small_raised_mask;
	
	XfeAppCreateSimple("PixmapTest",&argc,argv,"MainFrame",&frame,&form);

	large = XfeCreateButton(form,"Large",NULL,0);

	small = XfeCreateButton(form,"Small",NULL,0);

	get_icon(large,ICON,&icon,&icon_mask);
	get_icon(large,ICON_ARMED,&icon_armed,&icon_armed_mask);
	get_icon(large,ICON_RAISED,&icon_raised,&icon_raised_mask);

	get_icon(small,ICON_SMALL,&icon_small,&icon_small_mask);
	get_icon(small,ICON_SMALL_ARMED,&icon_small_armed,&icon_small_armed_mask);
	get_icon(small,ICON_SMALL_RAISED,&icon_small_raised,&icon_small_raised_mask);


#if 0
	XtVaSetValues(large,
				  XmNpixmap,			icon,
				  XmNpixmapMask,		icon_mask,
				  XmNarmedPixmap,		icon_armed,
				  XmNarmedPixmapMask,	icon_armed_mask,
				  XmNraisedPixmap,		icon_raised,
				  XmNraisedPixmapMask,	icon_raised_mask,
				  NULL);

	XtVaSetValues(small,
				  XmNpixmap,			icon_small,
				  XmNpixmapMask,		icon_small_mask,
				  XmNarmedPixmap,		icon_small_armed,
				  XmNarmedPixmapMask,	icon_small_armed_mask,
				  XmNraisedPixmap,		icon_small_raised,
				  XmNraisedPixmapMask,	icon_small_raised_mask,
				  NULL);
#else
	XtVaSetValues(large,
				  XmNpixmap,			XfeGetPixmap(large,"task_editor"),
				  XmNpixmapMask,		XfeGetMask(large,"task_editor"),
				  XmNarmedPixmap,		XfeGetPixmap(large,"task_editor_armed"),
				  XmNarmedPixmapMask,	XfeGetMask(large,"task_editor_armed"),
				  XmNraisedPixmap,		XfeGetPixmap(large,"task_editor_raised"),
				  XmNraisedPixmapMask,	XfeGetMask(large,"task_editor_raised"),
				  NULL);

	XtVaSetValues(small,
				  XmNpixmap,			XfeGetPixmap(large,"task_small_editor"),
				  XmNpixmapMask,		XfeGetMask(large,"task_small_editor"),
				  XmNarmedPixmap,		XfeGetPixmap(large,"task_small_editor_armed"),
				  XmNarmedPixmapMask,	XfeGetMask(large,"task_small_editor_armed"),
				  XmNraisedPixmap,		XfeGetPixmap(large,"task_small_editor_raised"),
				  XmNraisedPixmapMask,	XfeGetMask(large,"task_small_editor_raised"),
				  NULL);

#endif

	XtManageChild(large);
	XtManageChild(small);

	XtPopup(frame,XtGrabNone);

    XfeAppMainLoop();

	return 0;
}
/*----------------------------------------------------------------------*/
static Boolean
get_icon(Widget w,String filename,Pixmap * pixmap,Pixmap * mask)
{
	return XfeAllocatePixmapFromFile(filename,
									 XtDisplay(w),
									 DefaultRootWindow(XtDisplay(w)),
									 XfeColormap(w),
									 40000,
									 XfeDepth(w),
									 XfeBackground(w),
									 pixmap,
									 mask);
}
/*----------------------------------------------------------------------*/
