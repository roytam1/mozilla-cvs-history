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
/* Name:		<Xfe/OrientedP.h>										*/
/* Description:	XfeOriented widget private header file.					*/
/* Author:		Ramiro Estrugo <ramiro@netscape.com>					*/
/*																		*/
/*----------------------------------------------------------------------*/

#ifndef _XfeOrientedP_h_						/* start OrientedP.h	*/
#define _XfeOrientedP_h_

#include <Xfe/Oriented.h>
#include <Xfe/DynamicManagerP.h>

XFE_BEGIN_CPLUSPLUS_PROTECTION

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeOriented method inheritance macros								*/
/*																		*/
/*----------------------------------------------------------------------*/
#define XfeInheritEnter						((XfeOrientedProc)		_XtInherit)
#define XfeInheritLeave						((XfeOrientedProc)		_XtInherit)
#define XfeInheritMotion					((XfeOrientedProc)		_XtInherit)
#define XfeInheritDragStart					((XfeOrientedProc)		_XtInherit)
#define XfeInheritDragEnd					((XfeOrientedProc)		_XtInherit)
#define XfeInheritDragMotion				((XfeOrientedProc)		_XtInherit)
#define XfeInheritDescendantEnter			((XfeOrientedProc)		_XtInherit)
#define XfeInheritDescendantLeave			((XfeOrientedProc)		_XtInherit)
#define XfeInheritDescendantMotion			((XfeOrientedProc)		_XtInherit)
#define XfeInheritDescendantDragStart		((XfeOrientedProc)		_XtInherit)
#define XfeInheritDescendantDragEnd			((XfeOrientedProc)		_XtInherit)
#define XfeInheritDescendantDragMotion		((XfeOrientedProc)		_XtInherit)

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeOrientedClassPart													*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct
{
	XfeOrientedProc		enter;					/* Enter				*/
	XfeOrientedProc		leave;					/* Leave				*/
	XfeOrientedProc		motion;					/* Motion				*/

	XfeOrientedProc		drag_start;				/* Drag start			*/
	XfeOrientedProc		drag_end;				/* Drag end				*/
	XfeOrientedProc		drag_motion;			/* Drag motion			*/

	XfeOrientedProc		descendant_enter;		/* Descendant Enter		*/
	XfeOrientedProc		descendant_leave;		/* Descendant Leave		*/
	XfeOrientedProc		descendant_motion;		/* Descendant Motion	*/

	XfeOrientedProc		descendant_drag_start;	/* Drag start			*/
	XfeOrientedProc		descendant_drag_end;	/* Drag end				*/
	XfeOrientedProc		descendant_drag_motion;	/* Drag motion			*/

	XtPointer			extension;				/* Extension			*/ 
} XfeOrientedClassPart;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeOrientedClassRec													*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct _XfeOrientedClassRec
{
	CoreClassPart				core_class;
	CompositeClassPart			composite_class;
	ConstraintClassPart			constraint_class;
	XmManagerClassPart			manager_class;
	XfeManagerClassPart			xfe_manager_class;
	XfeDynamicManagerClassPart	xfe_dynamic_manager_class;
	XfeOrientedClassPart		xfe_oriented_class;
} XfeOrientedClassRec;

externalref XfeOrientedClassRec xfeOrientedClassRec;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeOrientedPart														*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct _XfeOrientedPart
{
    /* Drag resources */
	Boolean				allow_drag;				/* Allow drag			*/
	Boolean				drag_in_progress;		/* Drag in progress		*/
	Boolean				cursor_on;				/* Cursor on			*/

    /* Cursor resources */
	Cursor				vertical_cursor;		/* Vertical cursor		*/
	Cursor				horizontal_cursor;		/* Horizontal cursor	*/

    /* Orientation resources */
    unsigned char		orientation;			/* Orientation			*/

	/* Spacing resources */
    Dimension			spacing;				/* Spacing				*/

    /* Private data -- Dont even look past this comment -- */
	int					drag_start_x;			/* Drag start x			*/
	int					drag_start_y;			/* Drag start x			*/
} XfeOrientedPart;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeOrientedRec														*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct _XfeOrientedRec
{
	CorePart				core;
	CompositePart			composite;
	ConstraintPart			constraint;
	XmManagerPart			manager;
	XfeManagerPart			xfe_manager;
	XfeDynamicManagerPart	xfe_dynamic_manager;
	XfeOrientedPart			xfe_oriented;
} XfeOrientedRec;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeOrientedConstraintPart											*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct _XfeOrientedConstraintPart
{
	Boolean							allow_drag;
} XfeOrientedConstraintPart;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeOrientedConstraintRec												*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct _XfeOrientedConstraintRec
{
    XmManagerConstraintPart				manager;
    XfeManagerConstraintPart			xfe_manager;
    XfeDynamicManagerConstraintPart		xfe_dynamic_manager;
    XfeOrientedConstraintPart			xfe_oriented;
} XfeOrientedConstraintRec;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeOrientedPart Access Macro											*/
/*																		*/
/*----------------------------------------------------------------------*/
#define _XfeOrientedPart(w) &(((XfeOrientedWidget) w) -> xfe_oriented)

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeToolBar - superclass = XfeOriented								*/
/*																		*/
/*----------------------------------------------------------------------*/
#define _XFE_PREPARE_MAX_CHILD_DIMENSIONS			XfePrepare1

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeOriented member access macros										*/
/*																		*/
/*----------------------------------------------------------------------*/
#define _XfeOrientedOrientation(w) \
(((XfeOrientedWidget) (w))-> xfe_oriented . orientation)
/*----------------------------------------------------------------------*/
#define _XfeOrientedSpacing(w) \
(((XfeOrientedWidget) (w))-> xfe_oriented . spacing)
/*----------------------------------------------------------------------*/
#define _XfeOrientedDragStartX(w) \
(((XfeOrientedWidget) (w))-> xfe_oriented . drag_start_x)
/*----------------------------------------------------------------------*/
#define _XfeOrientedDragStartY(w) \
(((XfeOrientedWidget) (w))-> xfe_oriented . drag_start_y)
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeOrientedPart child constraint part access macro					*/
/*																		*/
/*----------------------------------------------------------------------*/
#define _XfeOrientedConstraintPart(w) \
(&(((XfeOrientedConstraintRec *) _XfeConstraints(w)) -> xfe_oriented))

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeOriented method invocation functions								*/
/*																		*/
/*----------------------------------------------------------------------*/
extern void
_XfeOrientedEnter					(Widget			w,
									 Widget			descendant,
									 int			x,
									 int			y);
/*----------------------------------------------------------------------*/
extern void
_XfeOrientedLeave					(Widget			w,
									 Widget			descendant,
									 int			x,
									 int			y);
/*----------------------------------------------------------------------*/
extern void
_XfeOrientedMotion					(Widget			w,
									 Widget			descendant,
									 int			x,
									 int			y);
/*----------------------------------------------------------------------*/
extern void
_XfeOrientedDragStart				(Widget			w,
									 Widget			descendant,
									 int			x,
									 int			y);
/*----------------------------------------------------------------------*/
extern void
_XfeOrientedDragEnd					(Widget			w,
									 Widget			descendant,
									 int			x,
									 int			y);
/*----------------------------------------------------------------------*/
extern void
_XfeOrientedDragMotion				(Widget			w,
									 Widget			descendant,
									 int			x,
									 int			y);
/*----------------------------------------------------------------------*/
extern void
_XfeOrientedDescendantEnter			(Widget			w,
									 Widget			descendant,
									 int			x,
									 int			y);
/*----------------------------------------------------------------------*/
extern void
_XfeOrientedDescendantLeave			(Widget			w,
									 Widget			descendant,
									 int			x,
									 int			y);
/*----------------------------------------------------------------------*/
extern void
_XfeOrientedDescendantMotion		(Widget			w,
									 Widget			descendant,
									 int			x,
									 int			y);
/*----------------------------------------------------------------------*/
extern void
_XfeOrientedDescendantDragStart		(Widget			w,
									 Widget			descendant,
									 int			x,
									 int			y);
/*----------------------------------------------------------------------*/
extern void
_XfeOrientedDescendantDragEnd		(Widget			w,
									 Widget			descendant,
									 int			x,
									 int			y);
/*----------------------------------------------------------------------*/
extern void
_XfeOrientedDescendantDragMotion	(Widget			w,
									 Widget			descendant,
									 int			x,
									 int			y);
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeOriented private Methods											*/
/*																		*/
/*----------------------------------------------------------------------*/
extern void
_XfeOrientedSetCursorState			(Widget			w,
									 Boolean		state);
/*----------------------------------------------------------------------*/

XFE_END_CPLUSPLUS_PROTECTION

#endif											/* end OrientedP.h		*/

