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
/* Name:		<Xfe/Resources.c>										*/
/* Description:	Xfe widgets resources utilities.						*/
/* Author:		Ramiro Estrugo <ramiro@netscape.com>					*/
/*																		*/
/*----------------------------------------------------------------------*/

#include <Xfe/XfeP.h>

#define XmNisEnabled "isEnabled"
#define XmCIsEnabled "IsEnabled"

/*----------------------------------------------------------------------*/
/*																		*/
/* Private functions													*/
/*																		*/
/*----------------------------------------------------------------------*/
static XtPointer
SubResourceGetValue			(Widget		parent,
							 String		widget_name,
							 String		widget_class,
							 String		resource_name,
							 String		resource_class,
							 String		resource_type,
							 Cardinal	resource_size,
							 XtPointer	default_addr);
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*																		*/
/* Public functions														*/
/*																		*/
/*----------------------------------------------------------------------*/
/* extern */ Boolean
XfeSubResourceGetBooleanValue(Widget	parent,
							  String	widget_name,
							  String	widget_class,
							  String	resource_name,
							  String	resource_class,
							  Boolean	default_value)
{
	XtPointer value;
	XtPointer default_ptr = default_value ? (XtPointer)True : (XtPointer)False;

	value = SubResourceGetValue(parent,
								widget_name,
								widget_class,
								resource_name,
								resource_class,
								XmRBoolean,
								sizeof(Boolean),
								default_ptr);

	return (value != NULL) ? True : False;
}
/*----------------------------------------------------------------------*/
/* extern */ XmString
XfeSubResourceGetXmStringValue(Widget	parent,
							   String	widget_name,
							   String	widget_class,
							   String	resource_name,
							   String	resource_class,
							   XmString	default_value)
{
	return (XmString) SubResourceGetValue(parent,
										  widget_name,
										  widget_class,
										  resource_name,
										  resource_class,
										  XmRXmString,
										  sizeof(XmString),
										  (XtPointer) default_value);
}
/*----------------------------------------------------------------------*/
/* extern */ String
XfeSubResourceGetStringValue(Widget		parent,
							 String		widget_name,
							 String		widget_class,
							 String		resource_name,
							 String		resource_class,
							 String		default_value)
{
	return (String) SubResourceGetValue(parent,
										widget_name,
										widget_class,
										resource_name,
										resource_class,
										XmRString,
										sizeof(String),
										(XtPointer) default_value);
}
/*----------------------------------------------------------------------*/
/* extern */ Pixel
XfeSubResourceGetPixelValue(Widget		parent,
							String		widget_name,
							String		widget_class,
							String		resource_name,
							String		resource_class,
							Pixel		default_value)
{
	/* Pixel default_pixel = WhitePixelOfScreen(_XfeScreen(parent)); */

	return (Pixel) SubResourceGetValue(parent,
									   widget_name,
									   widget_class,
									   resource_name,
									   resource_class,
									   XmRPixel,
									   sizeof(Pixel),
									   (XtPointer) default_value);
}
/*----------------------------------------------------------------------*/
/* extern */ String
XfeSubResourceGetWidgetStringValue(Widget		w,
								   String		resource_name,
								   String		resource_class)
{
	return XfeSubResourceGetStringValue(_XfeParent(w),
										XtName(w),
										XfeClassNameForWidget(w),
										resource_name,
										resource_class,
										NULL);
}
/*----------------------------------------------------------------------*/
/* extern */ XmString
XfeSubResourceGetWidgetXmStringValue(Widget		w,
									 String		resource_name,
									 String		resource_class)
{
	return XfeSubResourceGetXmStringValue(_XfeParent(w),
										  XtName(w),
										  XfeClassNameForWidget(w),
										  resource_name,
										  resource_class,
										  NULL);
}
/*----------------------------------------------------------------------*/


/*----------------------------------------------------------------------*/
/*																		*/
/* Check whether a child is enabled.									*/
/*																		*/
/*----------------------------------------------------------------------*/
/* extern */ Boolean
XfeChildIsEnabled(Widget	parent,
				  String	child_name,
				  String	child_class,
				  Boolean	default_value)
{
	return XfeSubResourceGetBooleanValue(parent,
										 child_name,
										 child_class,
										 XmNisEnabled,
										 XmCIsEnabled,
										 default_value);
}
/*----------------------------------------------------------------------*/


/*----------------------------------------------------------------------*/
/*																		*/
/* Private functions													*/
/*																		*/
/*----------------------------------------------------------------------*/
static XtPointer
SubResourceGetValue(Widget		parent,
					String		widget_name,
					String		widget_class,
					String		resource_name,
					String		resource_class,
					String		resource_type,
					Cardinal	resource_size,
					XtPointer	default_addr)
{
	static XtResource	_resource;

	XtPointer			value = NULL;

    _resource.resource_name		= resource_name;
    _resource.resource_class	= resource_class;
    _resource.resource_type		= resource_type;
    _resource.resource_size		= resource_size;
    _resource.resource_offset	= 0;
    _resource.default_type		= XtRImmediate;
    _resource.default_addr		= default_addr;

    XtGetSubresources(parent,				/* w				*/
					  &value,				/* base				*/
					  widget_name,			/* name				*/
					  widget_class,			/* class			*/
					  &_resource,			/* resources		*/
					  1,					/* num_resources	*/
					  NULL,					/* args				*/
					  0);					/* num_args			*/

	return value;
}
/*----------------------------------------------------------------------*/
