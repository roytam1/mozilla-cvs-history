/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
