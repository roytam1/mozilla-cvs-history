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
/* Name:		<Xfe/ToolScrollP.h>										*/
/* Description:	XfeToolScroll widget private header file.				*/
/* Author:		Ramiro Estrugo <ramiro@netscape.com>					*/
/*																		*/
/*----------------------------------------------------------------------*/

#ifndef _XfeToolScrollP_h_						/* start ToolScrollP.h	*/
#define _XfeToolScrollP_h_

#include <Xfe/ToolScroll.h>
#include <Xfe/OrientedP.h>

XFE_BEGIN_CPLUSPLUS_PROTECTION
   
/*----------------------------------------------------------------------*/
/*																		*/
/* XfeToolScrollClassPart												*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct
{
	XtPointer		extension;					/* Extension			*/ 
} XfeToolScrollClassPart;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeToolScrollClassRec												*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct _XfeToolScrollClassRec
{
	CoreClassPart				core_class;
	CompositeClassPart			composite_class;
	ConstraintClassPart			constraint_class;
	XmManagerClassPart			manager_class;
	XfeManagerClassPart			xfe_manager_class;
	XfeDynamicManagerClassPart	xfe_dynamic_manager_class;
	XfeOrientedClassPart		xfe_oriented_class;
	XfeToolScrollClassPart		xfe_tool_scroll_class;
} XfeToolScrollClassRec;

externalref XfeToolScrollClassRec xfeToolScrollClassRec;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeToolScrollPart													*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct _XfeToolScrollPart
{
    /* Arrow resources */
	Widget				backward_arrow;			/* Backward arrow		*/
	Widget				forward_arrow;			/* Forward arrow		*/
	unsigned char		arrow_display_policy;	/* Arrow disp policy	*/
	unsigned char		arrow_placement;		/* Arrow placement		*/

	Widget				clip_area;				/* Clip area			*/
	Dimension			clip_shadow_thickness;	/* Clip shadow thickness*/
	unsigned char		clip_shadow_type;		/* Clip shadow type		*/

	/* Tool bar resources */
	Widget				tool_bar;				/* Tool bar				*/
	Position			tool_bar_position;		/* Tool bar position	*/

} XfeToolScrollPart;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeToolScrollRec														*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct _XfeToolScrollRec
{
	CorePart				core;
	CompositePart			composite;
	ConstraintPart			constraint;
	XmManagerPart			manager;
	XfeManagerPart			xfe_manager;
	XfeDynamicManagerPart	xfe_dynamic_manager;
	XfeOrientedPart			xfe_oriented;
	XfeToolScrollPart		xfe_tool_scroll;
} XfeToolScrollRec;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeToolScrollPart Access Macro										*/
/*																		*/
/*----------------------------------------------------------------------*/
#define _XfeToolScrollPart(w) &(((XfeToolScrollWidget) w) -> xfe_tool_scroll)

XFE_END_CPLUSPLUS_PROTECTION

#endif											/* end ToolScrollP.h	*/

