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
/* Name:		<XfeBm/BmAccent.c>										*/
/*																		*/
/* Description:	Functions for drawing accents on push and cascade		*/
/*              buttons.  Button accents are used to give feedback by	*/
/*              the bookmark filing mechanism in mozilla.				*/
/*																		*/
/* Author:		Ramiro Estrugo <ramiro@netscape.com>					*/
/*																		*/
/*----------------------------------------------------------------------*/

#include <Xfe/XfeP.h>

#include <Xfe/BmCascadeP.h>
#include <Xfe/BmButtonP.h>

#include <Xfe/ManagerP.h>

/*----------------------------------------------------------------------*/
/*																		*/
/* Private accent functions.											*/
/*																		*/
/*----------------------------------------------------------------------*/
#define ACCENT_DRAW		1
#define ACCENT_ERASE	2

static void		AccentBottom	(Widget,GC,GC,Dimension,Dimension,
								 Dimension,Dimension,int);
static void		AccentAll		(Widget,GC,GC,Dimension,Dimension,
								 Dimension,Dimension,int);
static void		AccentTop		(Widget,GC,GC,Dimension,Dimension,
								 Dimension,Dimension,int);

/*----------------------------------------------------------------------*/
/*																		*/
/* Public accent drawing functions.										*/
/*																		*/
/*----------------------------------------------------------------------*/
/* extern */ void
XfeMenuItemDrawAccent(Widget			item,
					  unsigned char		accent_type,
					  Dimension			offset_left,
					  Dimension			offset_right,
					  Dimension			shadow_thickness,
					  Dimension			accent_thickness)
{
	Widget		pw;
	GC			top_gc;
	GC			bottom_gc;
	XGCValues	values;
	int			top_subwindow_mode;
	int			bottom_subwindow_mode;

	/* duh */
	if (accent_type == XmACCENT_NONE)
	{
		return;
	}

	assert( _XfeIsAlive(item) );
	assert( XmIsPushButton(item) || XmIsCascadeButton(item) );

	pw = _XfeParent(item);

	assert( XmIsRowColumn(pw) );

	top_gc		= _XfemTopShadowGC(pw);
	bottom_gc	= _XfemBottomShadowGC(pw);

	/* Remember the old wubwindow mode values */
	XGetGCValues(XtDisplay(item),top_gc,GCSubwindowMode,&values);

	top_subwindow_mode = values.subwindow_mode;

	XGetGCValues(XtDisplay(item),bottom_gc,GCSubwindowMode,&values);

	bottom_subwindow_mode = values.subwindow_mode;

	/* Force the subwindow mode to IncludeInferiors */
	XSetSubwindowMode(XtDisplay(item),top_gc,IncludeInferiors);
	XSetSubwindowMode(XtDisplay(item),bottom_gc,IncludeInferiors);

	switch(accent_type)
	{
	case XmACCENT_BOTTOM:
		AccentBottom(item,top_gc,bottom_gc,offset_left,offset_right,
					 shadow_thickness,accent_thickness,ACCENT_DRAW);
		break;

	case XmACCENT_ALL:
		AccentAll(item,top_gc,bottom_gc,offset_left,offset_right,
				  shadow_thickness,accent_thickness,ACCENT_DRAW);
		break;

	case XmACCENT_TOP:
		AccentTop(item,top_gc,bottom_gc,offset_left,offset_right,
				  shadow_thickness,accent_thickness,ACCENT_DRAW);
		break;

	default:
		break;
	}

	/* Restore the old subwindow mode */
	XSetSubwindowMode(XtDisplay(item),top_gc,top_subwindow_mode);
	XSetSubwindowMode(XtDisplay(item),bottom_gc,bottom_subwindow_mode);
}
/*----------------------------------------------------------------------*/
/* extern */ void
XfeMenuItemEraseAccent(Widget			item,
					   unsigned char	accent_type,
					   Dimension		offset_left,
					   Dimension		offset_right,
					   Dimension		shadow_thickness,
					   Dimension		accent_thickness)
{
	Widget		pw;
	GC			gc;
	XGCValues	values;
	int			subwindow_mode;

	/* duh */
	if (accent_type == XmACCENT_NONE)
	{
		return;
	}

	assert( _XfeIsAlive(item) );
	assert( XmIsPushButton(item) || XmIsCascadeButton(item) );

	pw = _XfeParent(item);

	assert( XmIsRowColumn(pw) );

	gc = _XfemBackgroundGC(pw);

	/* Remember the old wubwindow mode values */
	XGetGCValues(XtDisplay(item),gc,GCSubwindowMode,&values);

	subwindow_mode = values.subwindow_mode;

	/* Force the subwindow mode to IncludeInferiors */
	XSetSubwindowMode(XtDisplay(item),gc,IncludeInferiors);

	switch(accent_type)
	{
	case XmACCENT_BOTTOM:
		AccentBottom(item,gc,None,offset_left,offset_right,
					 shadow_thickness,accent_thickness,ACCENT_ERASE);
		break;

	case XmACCENT_ALL:
		AccentAll(item,gc,None,offset_left,offset_right,
				  shadow_thickness,accent_thickness,ACCENT_ERASE);
		break;

	case XmACCENT_TOP:
		AccentTop(item,gc,None,offset_left,offset_right,
				  shadow_thickness,accent_thickness,ACCENT_ERASE);
		break;

	default:
		break;
	}

	/* Restore the old subwindow mode */
	XSetSubwindowMode(XtDisplay(item),gc,subwindow_mode);
}
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*																		*/
/* Private accent functions.											*/
/*																		*/
/*----------------------------------------------------------------------*/
static void
AccentBottom(Widget			w,
			 GC				top_gc,
			 GC				bottom_gc,
			 Dimension		offset_left,
			 Dimension		offset_right,
			 Dimension		shadow_thickness,
			 Dimension		accent_thickness,
			 int			code)
{
	Widget		pw = _XfeParent(w);
	int			position_index = XfeMenuItemPositionIndex(w);
	Cardinal	num_children = _XfemNumChildren(pw);
	Position	x;
	Position	y;
	Dimension	width;
	Dimension	height = 2 * shadow_thickness + accent_thickness;

	assert( code == ACCENT_DRAW || code == ACCENT_ERASE );

	/* Last item */
	if (position_index == (num_children - 1))
	{
		x = _XfeX(w) + offset_left;
		y = _XfeY(w) + _XfeHeight(w) - height;
		
		width = _XfeWidth(w) - offset_left - offset_right;
	}
	/* Any other item */
	else
	{
		x = _XfeX(w) + offset_left;
		y = _XfeY(w) + _XfeHeight(w) - height / 2;
		
		width = _XfeWidth(w) - offset_left - offset_right;
	}

	if (code == ACCENT_DRAW)
	{
		/* Draw accent */
		_XmDrawShadows(XtDisplay(pw),_XfeWindow(pw),top_gc,bottom_gc,
					   x,y,width,height,shadow_thickness,XmSHADOW_IN);
	}
	else
	{
		/* Erase accent */
		XfeDrawRectangle(XtDisplay(pw),_XfeWindow(pw),top_gc,
						 x,y,width,height,shadow_thickness);
	}
}
/*----------------------------------------------------------------------*/
static void
AccentTop(Widget			w,
		  GC				top_gc,
		  GC				bottom_gc,
		  Dimension			offset_left,
		  Dimension			offset_right,
		  Dimension			shadow_thickness,
		  Dimension			accent_thickness,
		  int				code)
{
	Widget		pw = _XfeParent(w);
	int			position_index = XfeMenuItemPositionIndex(w);
	Position	x;
	Position	y;
	Dimension	width;
	Dimension	height = 2 * shadow_thickness + accent_thickness;

	assert( code == ACCENT_DRAW || code == ACCENT_ERASE );

	/* First item */
	if (position_index == 0)
	{
		x = _XfeX(w) + offset_left;
		y = _XfeY(w);
		
		width = _XfeWidth(w) - offset_left - offset_right;
	}
	/* Any other item */
	else
	{
		x = _XfeX(w) + offset_left;
		y = _XfeY(w) - height / 2;
		
		width = _XfeWidth(w) - offset_left - offset_right;
	}

	if (code == ACCENT_DRAW)
	{
		/* Draw accent */
		_XmDrawShadows(XtDisplay(pw),_XfeWindow(pw),top_gc,bottom_gc,
					   x,y,width,height,shadow_thickness,XmSHADOW_IN);
	}
	else
	{
		/* Erase accent */
		XfeDrawRectangle(XtDisplay(pw),_XfeWindow(pw),top_gc,
						 x,y,width,height,shadow_thickness);
	}
}
/*----------------------------------------------------------------------*/
static void
AccentAll(Widget			w,
		  GC				top_gc,
		  GC				bottom_gc,
		  Dimension			offset_left,
		  Dimension			offset_right,
		  Dimension			shadow_thickness,
		  Dimension			accent_thickness,
		  int				code)
{
#if 1	
	if (code == ACCENT_DRAW)
	{
		/* Draw accent */
		_XmDrawShadows(XtDisplay(w),_XfeWindow(w),
					   top_gc,bottom_gc,
					   _XfeHighlightThickness(w),
					   _XfeHighlightThickness(w),
					   _XfeWidth(w) - 2 * _XfeHighlightThickness(w),
					   _XfeHeight(w) - 2 * _XfeHighlightThickness(w),
					   _XfeShadowThickness(w),
					   XmSHADOW_OUT);
	}
	else
	{
		/* Erase accent */
		XfeDrawRectangle(XtDisplay(w),_XfeWindow(w),top_gc,
						 _XfeHighlightThickness(w),
						 _XfeHighlightThickness(w),
						 _XfeWidth(w) - 2 * _XfeHighlightThickness(w),
						 _XfeHeight(w) - 2 * _XfeHighlightThickness(w),
						 _XfeShadowThickness(w));
	}
#else
	Widget		pw = _XfeParent(w);
	int			position_index = XfeMenuItemPositionIndex(w);
	Cardinal	num_children = _XfemNumChildren(pw);
	Position	x;
	Position	y;
	Dimension	width;
	Dimension	height;
	Dimension	total_thickness = shadow_thickness + accent_thickness;

	assert( code == ACCENT_DRAW || code == ACCENT_ERASE );

	/* One and only */
	if (num_children == 1)
	{
		x = _XfeX(w) + offset_left;
		y = _XfeY(w);

		width = _XfeWidth(w) - offset_left - offset_right;
		height = _XfeHeight(w);
	}
	else
	{
		Dimension overlap = (2 * shadow_thickness + accent_thickness) / 2;

		/* First item */
		if (position_index == 0)
		{
			x = _XfeX(w) + offset_left;
			y = _XfeY(w);
			
			width = _XfeWidth(w) - offset_left - offset_right;

			height = _XfeHeight(w) + overlap;
		}
		/* Last item */
		else if (position_index == (num_children - 1))
		{
			x = _XfeX(w) + offset_left;
			y = _XfeY(w) - overlap;
			
			width = _XfeWidth(w) - offset_left - offset_right;

			height = _XfeHeight(w) + overlap;
		}
		/* In between two others */
		else
		{
			x = _XfeX(w) + offset_left;
			y = _XfeY(w) - overlap;
			
			width = _XfeWidth(w) - offset_left - offset_right;

			height = _XfeHeight(w) + 2 * total_thickness;
		}
	}

	if (code == ACCENT_DRAW)
	{
		/* Draw accent */
		_XmDrawShadows(XtDisplay(pw),_XfeWindow(pw),top_gc,bottom_gc,
				   x,y,width,height,shadow_thickness,XmSHADOW_IN);
		
		x += total_thickness;
		y += total_thickness;
		
		width -= (2 * total_thickness);
		height -= (2 * total_thickness);
				   
		_XmDrawShadows(XtDisplay(pw),_XfeWindow(pw),top_gc,bottom_gc,
					   x,y,width,height,shadow_thickness,XmSHADOW_OUT);
	}
	else
	{
		/* Erase accent */
		XfeDrawRectangle(XtDisplay(pw),_XfeWindow(pw),top_gc,
						 x,y,width,height,shadow_thickness);
		
		x += total_thickness;
		y += total_thickness;
		
		width -= (2 * total_thickness);
		height -= (2 * total_thickness);
				   
		XfeDrawRectangle(XtDisplay(pw),_XfeWindow(pw),top_gc,
						 x,y,width,height,shadow_thickness);
	}
#endif
}
/*----------------------------------------------------------------------*/
