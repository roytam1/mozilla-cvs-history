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
/* Name:		<Xfe/BmCascadeP.h>										*/
/* Description:	XfeBmCascade widget private header file.				*/
/* Author:		Ramiro Estrugo <ramiro@netscape.com>					*/
/*																		*/
/*----------------------------------------------------------------------*/

#ifndef _XfeBmCascadeP_h_						/* start BmCascadeP.h	*/
#define _XfeBmCascadeP_h_

#include <Xfe/PrimitiveP.h>
#include <Xfe/BmCascade.h>
#include <Xm/CascadeBP.h>

XFE_BEGIN_CPLUSPLUS_PROTECTION
   
/*----------------------------------------------------------------------*/
/*																		*/
/* XfeBmCascadeClassRec													*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct
{
    XtPointer		extension;					/* Extension			*/
} XfeBmCascadeClassPart;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeBmCascadePart														*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct _XfeBmCascadeClassRec
{
    CoreClassPart				core_class;			/* Core class		*/
    XmPrimitiveClassPart		primitive_class;	/* XmPrimitive		*/
    XmLabelClassPart			label_class;		/* XmLabel			*/
    XmCascadeButtonClassPart	cascade_button_class;/* XmCascadeButton	*/
    XfeBmCascadeClassPart		bm_cascade_class;	/* XfeBmCascade		*/
} XfeBmCascadeClassRec;

externalref XfeBmCascadeClassRec xfeBmCascadeClassRec;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeBmCascadeRec														*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct _XfeBmCascadePart
{
    /* Pixmap resources */
	Pixmap				arm_pixmap;					/* Arm pixmap		*/
	Pixmap				arm_pixmap_mask;			/* Arm pixmap mask	*/
	Pixmap				label_pixmap_mask;			/* Label pixmap mask*/
	unsigned char		accent_type;				/* Accent type		*/

    /* Private Data Members */
    GC					pixmap_GC;					/* Pixmap gc		*/
	Dimension			pixmap_width;
	Dimension			pixmap_height;
} XfeBmCascadePart;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeBmCascadePart														*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct _XfeBmCascadeRec
{
    CorePart				core;				/* Core Part			*/
    XmPrimitivePart			primitive;			/* XmPrimitive Part		*/
    XmLabelPart				label;				/* XmLabel Part			*/
    XmCascadeButtonPart		cascade_button;		/* XmCascadeButton Part	*/
    XfeBmCascadePart		bm_cascade;			/* XfeBmCascade part	*/
} XfeBmCascadeRec;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeBmCascadePart Access Macro										*/
/*																		*/
/*----------------------------------------------------------------------*/
#define _XfeBmCascadePart(w) &(((XfeBmCascadeWidget) w) -> bm_cascade)

XFE_END_CPLUSPLUS_PROTECTION

#endif											/* end BmCascadeP.h		*/
