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
/* Name:		<Xfe/LabelP.h>											*/
/* Description:	XfeLabel widget private header file.					*/
/* Author:		Ramiro Estrugo <ramiro@netscape.com>					*/
/*																		*/
/*----------------------------------------------------------------------*/

#ifndef _XfeLabelP_h_							/* start LabelP.h		*/
#define _XfeLabelP_h_

#include <Xfe/Label.h>
#include <Xfe/PrimitiveP.h>

XFE_BEGIN_CPLUSPLUS_PROTECTION

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeLabel method inheritance macros									*/
/*																		*/
/*----------------------------------------------------------------------*/
#define XfeInheritLayoutString				((XtWidgetProc)			_XtInherit)
#define XfeInheritDrawString				((XfeExposeProc)		_XtInherit)
#define XfeInheritDrawSelection				((XfeExposeProc)		_XtInherit)
#define XfeInheritGetLabelGC				((XfeGetGCFunc)			_XtInherit)
#define XfeInheritGetSelectionGC			((XfeGetGCFunc)			_XtInherit)

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeLabelClassPart													*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct
{
    XtWidgetProc		layout_string;				/* layout_label		*/
    XfeExposeProc		draw_string;				/* draw_string		*/
    XfeExposeProc		draw_selection;				/* draw_selection	*/
	XfeGetGCFunc		get_label_gc;				/* get_label_gc		*/
	XfeGetGCFunc		get_selection_gc;			/* get_selection_gc	*/
    XtPointer			extension;					/* Extension		*/
} XfeLabelClassPart;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeLabelClassRec														*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct _XfeLabelClassRec
{
    CoreClassPart			core_class;
    XmPrimitiveClassPart	primitive_class;
    XfePrimitiveClassPart	xfe_primitive_class;
    XfeLabelClassPart		xfe_label_class;
} XfeLabelClassRec;

externalref XfeLabelClassRec xfeLabelClassRec;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeLabelPart															*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct _XfeLabelPart
{
    /* Callback resources */
    XtCallbackList		selection_changed_callback;	/* Selection changed cb */

    /* Label resources */
    unsigned char			label_alignment;		/* Label alignment	*/
    unsigned char			label_direction;		/* Label direction	*/
    XmFontList				font_list;				/* Label fontlist	*/
    XmString				label_string;			/* Label string		*/

	/* Truncation resources */
    Boolean					truncate_label;			/* Truncate label	*/
	XfeTruncateXmStringProc	truncate_proc;			/* Trunctae proc	*/    

	/* Selection resources */
    Boolean					selected;				/* Selected ?		*/
    Pixel					selection_color;		/* Selection color	*/
    Modifiers				selection_modifiers;	/* Selection mod	*/

	/* Edit resources */
    Modifiers				edit_modifiers;			/* Edit mod			*/

    /* Private Data Members */
    GC						label_GC;				/* Label gc			*/
    GC						insensitive_top_GC;		/* Insens top gc	*/
    GC						insensitive_bottom_GC;	/* Insens bottom gc	*/
    XRectangle				label_rect;				/* Label rectangle	*/
	Dimension				misc_offset;			/* Misc offset		*/

    GC						selection_GC;			/* Selection  gc	*/

} XfeLabelPart;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeLabelRec															*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct _XfeLabelRec
{
    CorePart			core;					/* Core Part			*/
    XmPrimitivePart		primitive;				/* XmPrimitive Part		*/
    XfePrimitivePart	xfe_primitive;			/* XfePrimitive Part	*/
    XfeLabelPart		xfe_label;				/* XfeLabel Part		*/
} XfeLabelRec;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeLabelPart Access Macro											*/
/*																		*/
/*----------------------------------------------------------------------*/
#define _XfeLabelPart(w) &(((XfeLabelWidget) w) -> xfe_label)

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeLabel - superclass = XfePrimitive									*/
/*																		*/
/*----------------------------------------------------------------------*/
#define _XFE_PREPARE_LABEL_STRING					XfePrepare1

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeLabel Method invocation functions									*/
/*																		*/
/*----------------------------------------------------------------------*/
extern void
_XfeLabelLayoutString			(Widget			w);
/*----------------------------------------------------------------------*/
extern void
_XfeLabelDrawString				(Widget			w,
								 XEvent *		event,
								 Region			region,
								 XRectangle *	clip_rect);
/*----------------------------------------------------------------------*/
extern void
_XfeLabelDrawSelection			(Widget			w,
								 XEvent *		event,
								 Region			region,
								 XRectangle *	clip_rect);
/*----------------------------------------------------------------------*/
extern GC
_XfeLabelGetLabelGC				(Widget			w);
/*----------------------------------------------------------------------*/
extern GC
_XfeLabelGetSelectionGC			(Widget			w);
/*----------------------------------------------------------------------*/
	
/*----------------------------------------------------------------------*/
/*																		*/
/* XfeLabel action procedures											*/
/*																		*/
/*----------------------------------------------------------------------*/
extern void
_XfeLabelBtn1Down				(Widget,XEvent *,char **,Cardinal *);
/*----------------------------------------------------------------------*/
extern void
_XfeLabelSelect					(Widget,XEvent *,char **,Cardinal *);
/*----------------------------------------------------------------------*/
extern void
_XfeLabelEdit					(Widget,XEvent *,char **,Cardinal *);
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeLabel private functions											*/
/*																		*/
/*----------------------------------------------------------------------*/
extern  Boolean
_XfeLabelAcceptSelectionEvent	(Widget			w,
								 XEvent *		event,
								 Boolean		inside_label);
/*----------------------------------------------------------------------*/
extern  Boolean
_XfeLabelAcceptEditEvent		(Widget			w,
								 XEvent *		event,
								 Boolean		inside_label);
/*----------------------------------------------------------------------*/
extern  void
_XfeLabelSetSelected			(Widget			w,
								 XEvent *		event,
								 Boolean		selected,
								 Boolean		invoke_callbacks);
/*----------------------------------------------------------------------*/

XFE_END_CPLUSPLUS_PROTECTION

#endif											/* end LabelP.h			*/
