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
/* Name:		<Xfe/ManagerChildren.c>									*/
/* Description:	XfeManager Children functions.							*/
/* Author:		Ramiro Estrugo <ramiro@netscape.com>					*/
/*																		*/
/*----------------------------------------------------------------------*/

#include <Xfe/ManagerP.h>
#include <Xfe/DynamicManagerP.h>

#define MESSAGE1 "Widget is not an XfeManager"
#define MESSAGE2 "Widget is not an XfeDynamicManager"

/*----------------------------------------------------------------------*/
/*																		*/
/* Private children and info functions									*/
/*																		*/
/*----------------------------------------------------------------------*/
static	Boolean		ChildMatchWithMask		(Widget,int);

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeManager children info functions									*/
/*																		*/
/*----------------------------------------------------------------------*/
/* extern */ void
_XfeManagerGetChildrenInfo(Widget			w,
						   XfeLinked		children,
						   int				mask,
                           XfeGeometryProc	proc,
						   Dimension *		max_width_out,
						   Dimension *		max_height_out,
						   Dimension *		min_width_out,
						   Dimension *		min_height_out,
						   Dimension *		total_width_out,
						   Dimension *		total_height_out,
						   Cardinal *		num_managed_out)
{
	Dimension			max_width = 0;
	Dimension			max_height = 0;
	Dimension			min_width = 0;
	Dimension			min_height = 0;
	Dimension			total_width = 0;
	Dimension			total_height = 0;
	Cardinal			num_managed = 0;
	XfeLinkNode			node = NULL;

#define FLAG_ALIVE(w,m) \
(((m) & XfeCHILDREN_INFO_ALIVE) ? _XfeIsAlive(w) : True)

#define FLAG_REALIZED(w,m) \
(((m) & XfeCHILDREN_INFO_REALIZED) ? _XfeIsRealized(w) : True)

#define FLAG_MANAGED(w,m) \
(((m) & XfeCHILDREN_INFO_MANAGED) ? _XfeIsManaged(w) : True)

	assert( _XfeIsAlive(w) );

	/* Make sure the widget is a manager */
	if (!XfeIsManager(w))
	{
		_XfeWarning(w,MESSAGE1);

		return;
	}
	
#if 0
	/* Make sure the child state mask is not NONE */
	if (mask == XfeCHILDREN_INFO_NONE)
	{
		return;
	}
#endif

	/* Make sure the children list is not NULL */
	if (children == NULL)
	{
		return;
	}

	/* Iterate through the children */
	for (node = XfeLinkedHead(children);
		 node;
		 node = XfeLinkNodeNext(node))
	{
		Widget	child = (Widget) XfeLinkNodeItem(node);

		assert( child != NULL );

		if (ChildMatchWithMask(child,mask))
		{
			Dimension child_width;
			Dimension child_height;
			
			if (proc != NULL)
			{
				(*proc)(child,&child_width,&child_height);
			}
			else
			{
				child_width = _XfeWidth(child);
				child_height = _XfeHeight(child);
			}
			
			/* Keep track of maximum width */
			if (child_width > max_width)
			{
				max_width = child_width;
			}
			/* Keep track of minimum width */
			else if (child_width < min_width)
			{
				min_width = child_width;
			}
			
			/* Keep track of maximum height */
			if (child_height > max_height)
			{
				max_height = child_height;
			}
			/* Keep track of minimum height */
			else if (child_height < min_height)
			{
				min_height = child_height;
			}
			
			total_width += child_width;
			total_height += child_height;
		}				
		
		/* Keep track of the number of managed children */
		if (_XfeIsManaged(child))
		{
			num_managed++;
		}
	}

	/* Assign only required arguments */
	if (max_width_out) 
	{
		*max_width_out = max_width;
	}

	if (max_height_out) 
	{
		*max_height_out = max_height;
	}

	if (min_width_out) 
	{
		*min_width_out = min_width;
	}

	if (min_height_out) 
	{
		*min_height_out = min_height;
	}

	if (total_width_out) 
	{
		*total_width_out = total_width;
	}

	if (total_height_out) 
	{
		*total_height_out = total_height;
	}

	if (num_managed_out) 
	{
		*num_managed_out = num_managed;
	}
}
/*----------------------------------------------------------------------*/
/* extern */ void
_XfeManagerUpdateComponentChildrenInfo(Widget w)
{
	if (_XfemComponentChildren(w) != NULL)
	{
		return;
	}

	_XfeManagerGetChildrenInfo(w,
							   _XfemComponentChildren(w),
							   XfeCHILDREN_INFO_ALIVE|XfeCHILDREN_INFO_MANAGED,
							   NULL,
							   &_XfemMaxComponentWidth(w),
							   &_XfemMaxComponentHeight(w),
							   NULL,
							   NULL,
							   NULL,
							   NULL,
							   NULL);
}
/*----------------------------------------------------------------------*/
/* extern */ void
_XfeManagerUpdateStaticChildrenInfo(Widget w)
{
	if (_XfemStaticChildren(w) != NULL)
	{
		return;
	}

	_XfeManagerGetChildrenInfo(w,
							   _XfemStaticChildren(w),
							   XfeCHILDREN_INFO_ALIVE|XfeCHILDREN_INFO_MANAGED,
							   NULL,
							   &_XfemMaxStaticWidth(w),
							   &_XfemMaxStaticHeight(w),
							   NULL,
							   NULL,
							   NULL,
							   NULL,
							   NULL);
}
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeManager private children apply functions							*/
/*																		*/
/*----------------------------------------------------------------------*/
/* extern */ void
_XfeManagerApplyProcToChildren(Widget				w,
							   XfeLinked			children,
							   int					mask,
							   XfeManagerApplyProc	proc,
							   XtPointer			data)
{
	XfeLinkNode			node = NULL;

	assert( _XfeIsAlive(w) );
	assert( proc != NULL );

	/* Make sure the widget is a manager */
	if (!XfeIsManager(w))
	{
		_XfeWarning(w,MESSAGE1);

		return;
	}

	/* Iterate through the children */
	for (node = XfeLinkedHead(children);
		 node;
		 node = XfeLinkNodeNext(node))
	{
		Widget	child = (Widget) XfeLinkNodeItem(node);

		assert( child != NULL );

		if (ChildMatchWithMask(child,mask))
		{
			(*proc)(w,child,data);
		}
	}
}
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeManager public children apply functions							*/
/*																		*/
/*----------------------------------------------------------------------*/
/* extern */ void
XfeManagerApply(Widget					w,
				int						mask,
				XfeManagerApplyProc		proc,
				XtPointer				data,
				Boolean					private_components,
				Boolean					freeze_layout)
{
	Cardinal i;

	assert( _XfeIsAlive(w) );

	/* Make sure its a Manager */
	if (!XfeIsManager(w))
	{
		_XfeWarning(w,MESSAGE1);
		
		return;
	}

	/* Freeze layout if needed */
	if (freeze_layout)
	{
		_XfemLayoutFrozen(w) = True;
	}
	
	/* Iterate through all the items */
	for (i = 0; i < _XfemNumChildren(w); i++)
	{
		Widget child = _XfeChildrenIndex(w,i);
	   
		if (_XfeIsAlive(child) && ChildMatchWithMask(child,mask))
		{
			Boolean skip = 
				(!private_components && XfeIsPrivateComponent(child));

			if (!skip)
			{
				(*proc)(w,child,data);
			}
		}
	}

	/* Un-freeze layout if needed */
	if (freeze_layout)
	{
		_XfemLayoutFrozen(w) = False;

		/* ??? I think this should be optional ??? */
		XfeManagerLayout(w);
	}
}
/*----------------------------------------------------------------------*/
/* extern */ void
XfeManagerApplyLinked(Widget				w,
					  unsigned char			child_type,
					  int					mask,
					  XfeManagerApplyProc	proc,
					  XtPointer				data,
					  Boolean				freeze_layout)
{
	XfeLinked	children = NULL;

	assert( _XfeIsAlive(w) );

	/* Make sure its a Manager */
	if (!XfeIsManager(w))
	{
		_XfeWarning(w,MESSAGE1);
		
		return;
	}

	assert( child_type == XmMANAGER_COMPONENT_CHILD ||
			child_type == XmMANAGER_DYNAMIC_CHILD ||
			child_type == XmMANAGER_STATIC_CHILD );

	if (child_type == XmMANAGER_DYNAMIC_CHILD)
	{
		/* Make sure widget is a XfeDynamicManager */
		if (!XfeIsDynamicManager(w))
		{
			_XfeWarning(w,MESSAGE2);
			
			return;
		}
	}

	if (child_type == XmMANAGER_COMPONENT_CHILD)
	{
		children = _XfemComponentChildren(w);
	}
	else if (child_type == XmMANAGER_DYNAMIC_CHILD)
	{
		children = _XfemDynamicChildren(w);
	}
	else if (child_type == XmMANAGER_STATIC_CHILD)
	{
		children = _XfemStaticChildren(w);
	}

	if (children != NULL)
	{
		/* Freeze layout if needed */
		if (freeze_layout)
		{
			_XfemLayoutFrozen(w) = True;
		}
		
		_XfeManagerApplyProcToChildren(w,children,mask,proc,data);

		/* Un-freeze layout if needed */
		if (freeze_layout)
		{
			_XfemLayoutFrozen(w) = False;
			
			/* ??? I think this should be optional ??? */
			XfeManagerLayout(w);
		}
	}
}
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*																		*/
/* Private children and info functions									*/
/*																		*/
/*----------------------------------------------------------------------*/
static Boolean
ChildMatchWithMask(Widget child,int mask)
{
	Boolean result = False;
	
#define FLAG_ALIVE(w,m) \
(((m) & XfeCHILDREN_INFO_ALIVE) ? _XfeIsAlive(w) : True)

#define FLAG_REALIZED(w,m) \
(((m) & XfeCHILDREN_INFO_REALIZED) ? _XfeIsRealized(w) : True)

#define FLAG_MANAGED(w,m) \
(((m) & XfeCHILDREN_INFO_MANAGED) ? _XfeIsManaged(w) : True)

	if (child != NULL)
	{
		Boolean	flag_alive = (child && FLAG_ALIVE(child,mask));
		Boolean	flag_realized = (flag_alive && FLAG_REALIZED(child,mask));
		Boolean	flag_managed = (flag_realized && FLAG_MANAGED(child,mask));
		
		result = (flag_alive && flag_realized && flag_managed);
	}

	return result;
}
/*----------------------------------------------------------------------*/
