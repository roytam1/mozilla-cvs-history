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
/* Name:		LogoTest.c												*/
/* Description:	Test for XfeLogo widget.								*/
/* Author:		Ramiro Estrugo <ramiro@netscape.com>					*/
/*																		*/
/*----------------------------------------------------------------------*/

#include <Xfe/XfeTest.h>

static void	animation_callback		(Widget,XtPointer,XtPointer);
static void	init_animations			(void);
static void	set_animation			(Cardinal);

static void	logo_activate_callback	(Widget,XtPointer,XtPointer);
static void	start_callback			(Widget,XtPointer,XtPointer);
static void	stop_callback			(Widget,XtPointer,XtPointer);
static void	reset_callback			(Widget,XtPointer,XtPointer);
static void	tick_callback			(Widget,XtPointer,XtPointer);

#define NUM_ANIMATIONS		4
#define ANIMATION_INTERVAL	100

static XfePixmapTable	_animation_pixmaps[NUM_ANIMATIONS];
static Cardinal			_animation_frame_counts[NUM_ANIMATIONS];
static Cardinal			_current_animation = 0;
static String			_animation_names[NUM_ANIMATIONS] =
{
	"Transparent Large",
	"Transparent Small",
	"Main Large",
	"Main Small"
};

static Widget _logo			= NULL;
static Widget _progress_bar	= NULL;

/*----------------------------------------------------------------------*/
int
main(int argc,char *argv[])
{
	Widget	form;
	Widget	frame;
    Widget	tool_bar;
    Widget	start;
    Widget	stop;
    Widget	reset;
    Widget	tick;

	XfeAppCreateSimple("LogoTest",&argc,argv,"MainFrame",&frame,&form);

    _logo = XtVaCreateManagedWidget("Logo",
									xfeLogoWidgetClass,
									form,
									NULL);
	
    _progress_bar = XtVaCreateManagedWidget("ProgressBar",
											xfeProgressBarWidgetClass,
											form,
											NULL);

    tool_bar = XtVaCreateManagedWidget("ToolBar",
									   xfeToolBarWidgetClass,
									   form,
									   NULL);

    start = XtVaCreateManagedWidget("Start",
									xfeButtonWidgetClass,
									tool_bar,
									NULL);

    stop = XtVaCreateManagedWidget("Stop",
									xfeButtonWidgetClass,
									tool_bar,
									NULL);

    tick = XtVaCreateManagedWidget("Tick",
									xfeButtonWidgetClass,
									tool_bar,
									NULL);

    reset = XtVaCreateManagedWidget("Reset",
									xfeButtonWidgetClass,
									tool_bar,
									NULL);

	
	init_animations();

	XtVaSetValues(_logo,
				  XmNbuttonLayout,		XmBUTTON_LABEL_ON_BOTTOM,
				  NULL);

    XtAddCallback(_logo,XmNactivateCallback,logo_activate_callback,NULL);
    XtAddCallback(_logo,XmNanimationCallback,animation_callback,NULL);

    XtAddCallback(start,XmNactivateCallback,start_callback,NULL);
    XtAddCallback(stop,XmNactivateCallback,stop_callback,NULL);
    XtAddCallback(reset,XmNactivateCallback,reset_callback,NULL);
    XtAddCallback(tick,XmNactivateCallback,tick_callback,NULL);

	set_animation(0);

	XtPopup(frame,XtGrabNone);

    XfeAppMainLoop();

	return 0;
}
/*----------------------------------------------------------------------*/
static void
set_animation(Cardinal i)
{
	assert( i < NUM_ANIMATIONS );

	assert( _animation_pixmaps != NULL );
	assert( _animation_pixmaps[i] != NULL );
	assert( _animation_frame_counts != NULL );

	XtVaSetValues(_logo,
   				  XmNanimationPixmaps,		_animation_pixmaps[i],
				  XmNnumAnimationPixmaps,	_animation_frame_counts[i],
				  NULL);
}
/*----------------------------------------------------------------------*/
static void
animation_callback(Widget w,XtPointer client_data,XtPointer call_data)
{
	Cardinal	i;
	char		buf[1024];

	XtVaGetValues(_logo,XmNcurrentPixmapIndex,&i,NULL);

	sprintf(buf,"%s\n%d",_animation_names[_current_animation],i);

	XfeSetXmStringPSZ(_logo,XmNlabelString,XmFONTLIST_DEFAULT_TAG,buf);
}
/*----------------------------------------------------------------------*/
static void
logo_activate_callback(Widget w,XtPointer client_data,XtPointer call_data)
{
	_current_animation++;

	if (_current_animation == NUM_ANIMATIONS)
	{
		_current_animation = 0;
	}

	set_animation(_current_animation);
}
/*----------------------------------------------------------------------*/
static void
start_callback(Widget w,XtPointer client_data,XtPointer call_data)
{
	XfeLogoAnimationStart(_logo);

	XfeProgressBarCylonStart(_progress_bar);
}
/*----------------------------------------------------------------------*/
static void
stop_callback(Widget w,XtPointer client_data,XtPointer call_data)
{
	XfeLogoAnimationStop(_logo);

	XfeProgressBarCylonStop(_progress_bar);
}
/*----------------------------------------------------------------------*/
static void
reset_callback(Widget w,XtPointer client_data,XtPointer call_data)
{
	XfeLogoAnimationReset(_logo);

	XfeProgressBarCylonReset(_progress_bar);
}
/*----------------------------------------------------------------------*/
static void
tick_callback(Widget w,XtPointer client_data,XtPointer call_data)
{
	XfeProgressBarCylonTick(_progress_bar);
}
/*----------------------------------------------------------------------*/
static void
init_animations()
{
	XfePixmapTable	pixmaps = NULL;
	Cardinal		num_pixmaps = 0;

	/* Transparent 40x40 */
	pixmaps = NULL;
	num_pixmaps = 0;

	XfeGetTransparent40x40Animation(_logo,&pixmaps,&num_pixmaps);

	_animation_pixmaps[0] = pixmaps;
	_animation_frame_counts[0] = num_pixmaps;

	/* Transparent 20x20 */
	pixmaps = NULL;
	num_pixmaps = 0;

	XfeGetTransparent20x20Animation(_logo,&pixmaps,&num_pixmaps);

	_animation_pixmaps[1] = pixmaps;
	_animation_frame_counts[1] = num_pixmaps;

	/* Main 40x40 */
	pixmaps = NULL;
	num_pixmaps = 0;

	XfeGetMain40x40Animation(_logo,&pixmaps,&num_pixmaps);

	_animation_pixmaps[2] = pixmaps;
	_animation_frame_counts[2] = num_pixmaps;

	/* Main 20x20 */
	pixmaps = NULL;
	num_pixmaps = 0;

	XfeGetMain20x20Animation(_logo,&pixmaps,&num_pixmaps);

	_animation_pixmaps[3] = pixmaps;
	_animation_frame_counts[3] = num_pixmaps;
}
/*----------------------------------------------------------------------*/
