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
/* Name:		<Xfe/Util.h>											*/
/* Description:	Xfe widgets misc utilities source.						*/
/* Author:		Ramiro Estrugo <ramiro@netscape.com>					*/
/*																		*/
/*----------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>

#include <Xfe/XfeP.h>

#include <Xfe/PrimitiveP.h>
#include <Xfe/ManagerP.h>

#include <Xm/LabelP.h>
#include <Xm/LabelGP.h>
#include <Xfe/LabelP.h>

#if XmVersion >= 2000
#include <Xm/Gadget.h>
#include <Xm/Primitive.h>
#include <Xm/Manager.h>
#endif

#include <Xm/ToggleB.h>
#include <Xm/ToggleBG.h>

#include <Xm/RepType.h>

#define MESSAGE0 "XfeInstancePointer() called with non XfePrimitive or XfeManager widget."

/*----------------------------------------------------------------------*/
/*																		*/
/* Simple Widget access functions										*/
/*																		*/
/*----------------------------------------------------------------------*/
/* extern */ Widget
XfeWindowedWidget(Widget w)
{
    assert( _XfeIsAlive(w) );

	if (!_XfeIsAlive(w))
	{
		return NULL;
	}

    return (XmIsGadget(w) ? XtParent(w) : w);
}
/*----------------------------------------------------------------------*/
/* extern */ Boolean
XfeIsViewable(Widget w)
{
    XWindowAttributes xwa;

	if (!_XfeIsAlive(w) || !XtIsWidget(w) || !_XfeIsRealized(w))
	{
		return False;
	}

    if (XGetWindowAttributes(XtDisplay(w),_XfeWindow(w),&xwa))
    {
		return (xwa.map_state == IsViewable);
    }
    
    return False;
}
/*----------------------------------------------------------------------*/
/* extern */ Boolean
XfeIsAlive(Widget w)
{
    return _XfeIsAlive(w);
}
/*----------------------------------------------------------------------*/
/* extern */ XtPointer
XfeUserData(Widget w)
{
    XtPointer user_data = NULL;
    
    assert( w != NULL );
    assert(XmIsGadget(w) || XmIsPrimitive(w) || XmIsManager(w));
    
    if (XmIsPrimitive(w))
    {
		user_data = _XfeUserData(w);
    }
    else if (XmIsManager(w))
    {
		user_data = _XfemUserData(w);
    }
    else if (XmIsGadget(w))
    {
		user_data = (XtPointer) XfeGetValue(w,XmNuserData);
    }
    
    return user_data;
}
/*----------------------------------------------------------------------*/
/* extern */ XtPointer
XfeInstancePointer(Widget w)
{
    assert( w != NULL );
    assert( XfeIsPrimitive(w) || XfeIsManager(w) );
    
    if (XfeIsPrimitive(w))
    {
		return _XfeInstancePointer(w);
    }
    else
    {
		return _XfemInstancePointer(w);
    }

	_XfeWarning(w,MESSAGE0);

	return NULL;
}
/*----------------------------------------------------------------------*/
/* extern */ Colormap
XfeColormap(Widget w)
{
    assert( _XfeIsAlive(w) );
    
    return _XfeColormap(w);
}
/*----------------------------------------------------------------------*/
/* extern */ Cardinal
XfeDepth(Widget w)
{
    assert( _XfeIsAlive(w) );
    
    return _XfeDepth(w);
}
/*----------------------------------------------------------------------*/
/* extern */ Pixel
XfeBackground(Widget w)
{
    assert( _XfeIsAlive(w) );

	if (!_XfeIsAlive(w))
	{
		return 0;
	}
    
    return _XfeBackgroundPixel(w);
}
/*----------------------------------------------------------------------*/
/* extern */ Pixel
XfeForeground(Widget w)
{
    assert( _XfeIsAlive(w) );

	if (!_XfeIsAlive(w))
	{
		return 0;
	}
    
	if (XmIsPrimitive(w))
	{
		return _XfeForeground(w);
	}

	return _XfemForeground(w);
}
/*----------------------------------------------------------------------*/
/* extern */ Pixel
XfeBottomShadowColor(Widget w)
{
    assert( _XfeIsAlive(w) );

	if (!_XfeIsAlive(w))
	{
		return 0;
	}
    
	if (XmIsPrimitive(w))
	{
		return _XfeBottomShadowColor(w);
	}

	return _XfemBottomShadowColor(w);
}
/*----------------------------------------------------------------------*/
/* extern */ Pixel
XfeTopShadowColor(Widget w)
{
    assert( _XfeIsAlive(w) );

	if (!_XfeIsAlive(w))
	{
		return 0;
	}
    
	if (XmIsPrimitive(w))
	{
		return _XfeTopShadowColor(w);
	}

	return _XfemTopShadowColor(w);
}
/*----------------------------------------------------------------------*/
/* extern */ String
XfeClassNameForWidget(Widget w)
{
	assert( w != NULL );

	return _XfeClassName(_XfeClass(w));
}
/*----------------------------------------------------------------------*/


/*----------------------------------------------------------------------*/
/*																		*/
/* Simple WidgetClass access functions									*/
/*																		*/
/*----------------------------------------------------------------------*/
/* extern */ String
XfeClassName(WidgetClass wc)
{
	assert( wc != NULL );

	return _XfeClassName(wc);
}
/*----------------------------------------------------------------------*/
/* extern */ WidgetClass
XfeSuperClass(WidgetClass wc)
{
	assert( wc != NULL );

	return _XfeSuperClass(wc);
}
/*----------------------------------------------------------------------*/


/*----------------------------------------------------------------------*/
/*																		*/
/* XmFontList															*/
/*																		*/
/*----------------------------------------------------------------------*/
/* extern */ XmFontList
XfeXmFontListCopy(Widget w,XmFontList font_list,unsigned char font_type)
{
    XmFontList new_font_list;
    
    /* If the font is null, create a default one */
    if (!font_list)
    {
		new_font_list = XmFontListCopy(_XmGetDefaultFontList(w,font_type));
    }
    /* Otherwise make a carbon copy */
    else
    {
		new_font_list = XmFontListCopy(font_list);
    }
    
    return new_font_list;
}
/*----------------------------------------------------------------------*/
/*																		*/
/* This function is useful when effecient access to a widget's 			*/
/* XmNfontList is needed.  It avoids the GetValues() and				*/
/* XmStringCopy() overhead.  Of course, the result should be 			*/
/* considered read only.												*/
/*																		*/
/*----------------------------------------------------------------------*/
/* extern */ XmFontList
XfeFastAccessFontList(Widget w)
{
	XmFontList	font_list = NULL;

	assert( _XfeIsAlive(w) );

	if (!_XfeIsAlive(w))
	{
		return NULL;
	}

	if (XmIsLabel(w))
	{
		font_list = ((XmLabelWidget) w) -> label . font;
	}
	else if (XmIsLabelGadget(w))
	{
		font_list = ((XmLabelGadget) w) -> label . font;
	}
	else if (XfeIsLabel(w))
	{
		font_list = ((XfeLabelWidget) w) -> xfe_label . font_list;
	}
#ifdef DEBUG_ramiro
	else
	{
		assert( 0 );
	}
#endif

	return font_list;
}
/*----------------------------------------------------------------------*/


/*----------------------------------------------------------------------*/
/*																		*/
/* Colors																*/
/*																		*/
/*----------------------------------------------------------------------*/
/* extern */ Pixel
XfeSelectPixel(Widget w,Pixel base)
{
    Pixel	select_pixel;
    Widget	core_widget = XfeWindowedWidget(w);
    
    XmGetColors(_XfeScreen(core_widget),_XfeColormap(core_widget),
				base,NULL,NULL,NULL,&select_pixel);
    
    return select_pixel;
}
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*																		*/
/* Explicit invocation of core methods.  								*/
/*																		*/
/*----------------------------------------------------------------------*/
/* extern */ void
XfeExpose(Widget w,XEvent * event,Region region)
{
	WidgetClass wc;

	assert( _XfeIsAlive(w) );
	assert( XtClass(w) != NULL );

	if (!_XfeIsAlive(w))
	{
		return;
	}

	wc = XtClass(w);
	
	assert( wc->core_class.expose != NULL );

	(*wc->core_class.expose)(w,event,region);
}
/*----------------------------------------------------------------------*/
/* extern */ void
XfeResize(Widget w)
{
	WidgetClass wc;

	assert( _XfeIsAlive(w) );
	assert( XtClass(w) != NULL );

	if (!_XfeIsAlive(w))
	{
		return;
	}

	wc = XtClass(w);
	
	assert( wc->core_class.resize != NULL );

	(*wc->core_class.resize)(w);
}
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*																		*/
/* Sleep routine.														*/
/*																		*/
/*----------------------------------------------------------------------*/
static void
SleepTimeout(XtPointer client_data,XtIntervalId * id)
{
 	Boolean * sleeping = (Boolean *) client_data;

 	*sleeping = False;
}
/*----------------------------------------------------------------------*/
/* extern */ void
XfeSleep(Widget w,XfeEventLoopProc proc,int ms)
{
 	Boolean		sleeping = True;

	assert( _XfeIsAlive(w) );
	assert( proc != NULL );

	XtAppAddTimeOut(XtWidgetToApplicationContext(w),
					ms,
					SleepTimeout,
					(XtPointer) &sleeping);	

	/* Invoke the user's event loop while we are sleeping */
	while(sleeping)
	{
		(*proc)();
	}
}
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/* extern */ void 
XfeRectSet(XRectangle *	rect,
		   Position		x,
		   Position		y,
		   Dimension	width,
		   Dimension	height)
{
	assert( rect != NULL );

	rect->x			= x;
	rect->y			= y;
	rect->width		= width;
	rect->height	= height;
}
/*----------------------------------------------------------------------*/
/* extern */ void 
XfeRectCopy(XRectangle * dst,XRectangle * src)
{
	assert( dst != NULL );
	assert( src != NULL );

	dst->x			= src->x;
	dst->y			= src->y;
	dst->width		= src->width;
	dst->height		= src->height;
}
/*----------------------------------------------------------------------*/
/* extern */ void 
XfePointSet(XPoint *	point,
			Position	x,
			Position	y)
{
	assert( point != NULL );

	point->x	= x;
	point->y	= y;
}
/*----------------------------------------------------------------------*/
/* extern */ Boolean
XfePointInRect(XRectangle *	rect,int x,int y)
{
	assert( rect != NULL );

	return ( (x >= rect->x) &&
			 (x <= (rect->x + rect->width)) &&
			 (y >= rect->y) &&
			 (y <= (rect->y + rect->height)) );
}
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*																		*/
/* Management															*/
/*																		*/
/*----------------------------------------------------------------------*/
/* extern */ void
XfeSetManagedState(Widget w,Boolean state)
{
	if (!_XfeIsAlive(w))
	{
		return;
	}

	if (state)
	{
		XtManageChild(w);
	}
	else
	{
		XtUnmanageChild(w);
	}
}
/*----------------------------------------------------------------------*/
/* extern */ void
XfeToggleManagedState(Widget w)
{
	if (!_XfeIsAlive(w))
	{
		return;
	}

	if (_XfeIsManaged(w))
	{
		XtUnmanageChild(w);
	}
	else
	{
		XtManageChild(w);
	}
}
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*																		*/
/* XEvent functions														*/
/*																		*/
/*----------------------------------------------------------------------*/
/* extern */ Boolean
XfeEventGetXY(XEvent * event,int * x_out,int * y_out)
{
	Boolean	result = False;
	int		x = 0;
	int		y = 0;

	assert( x_out || y_out );
	
	if (event)
	{
		if (event->type == EnterNotify || event->type == LeaveNotify)
		{
			x = event->xcrossing.x;
			y = event->xcrossing.y;

			result = True;
		}
		else if (event->type == MotionNotify)
		{
			x = event->xmotion.x;
			y = event->xmotion.y;

			result = True;
		}
		else if (event->type == ButtonPress || event->type == ButtonRelease)
		{
			x = event->xbutton.x;
			y = event->xbutton.y;

			result = True;
		}
#ifdef DEBUG_ramiro
		else
        {
          printf("Unknown event type '%d'\n",event->type);

          assert( 0 );
        }
#endif
	}

	if (x_out)
	{
		*x_out = x;
	}

	if (y_out)
	{
		*y_out = y;
	}

	return result;
}
/*----------------------------------------------------------------------*/
/* extern */ Boolean
XfeEventGetRootXY(XEvent * event,int * x_out,int * y_out)
{
	Boolean	result = False;
	int		x = 0;
	int		y = 0;

	assert( x_out || y_out );
	
	if (event)
	{
		if (event->type == EnterNotify || event->type == LeaveNotify)
		{
			x = event->xcrossing.x_root;
			y = event->xcrossing.y_root;

			result = True;
		}
		else if (event->type == MotionNotify)
		{
			x = event->xmotion.x_root;
			y = event->xmotion.y_root;

			result = True;
		}
		else if (event->type == ButtonPress || event->type == ButtonRelease)
		{
			x = event->xbutton.x_root;
			y = event->xbutton.y_root;

			result = True;
		}
#ifdef DEBUG_ramiro
		else
        {
          printf("Unknown event type '%d'\n",event->type);

          assert( 0 );
        }
#endif
	}

	if (x_out)
	{
		*x_out = x;
	}

	if (y_out)
	{
		*y_out = y;
	}

	return result;
}
/*----------------------------------------------------------------------*/
/* extern */ Modifiers
XfeEventGetModifiers(XEvent * event)
{
	if (event)
	{
		/*
		 * There probably a better way to do this using unions.
		 * But, I want to limit which events are checked.
		 */
		if (event->type == ButtonPress || event->type == ButtonRelease)
		{
			return event->xbutton.state;
		}
		else if (event->type == MotionNotify)
		{
			return event->xmotion.state;
		}
		else if (event->type == KeyPress || event->type == KeyRelease)
		{
			return event->xkey.state;
		}
		else if (event->type == EnterNotify || event->type == LeaveNotify)
		{
			return event->xcrossing.state;
		}
#ifdef DEBUG_ramiro
        else
        {
          printf("Unknown event type '%d'\n",event->type);

          assert( 0 );
        }
#endif
	}

	return 0;
}
/*----------------------------------------------------------------------*/
/* extern */ Boolean
XfeToggleButtonIsSet(Widget w)
{
	Boolean result = False;

	assert( XfeIsAlive(w) );
	assert( XmIsToggleButton(w) || XmIsToggleButtonGadget(w) );

	if (XmIsToggleButton(w))
	{
		result = XmToggleButtonGetState(w);
	}
	else
	{
		result = XmToggleButtonGadgetGetState(w);
	}
	
	return result;
}
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*																		*/
/* Test whether a widget is a private component of an XfeManager parent */
/*																		*/
/*----------------------------------------------------------------------*/
/* extern */ Boolean
XfeIsPrivateComponent(Widget w)
{
	return (XfeIsManager(_XfeParent(w)) && 
			_XfeConstraintManagerChildType(w) == XmMANAGER_COMPONENT_CHILD);
}
/*----------------------------------------------------------------------*/


/*----------------------------------------------------------------------*/
/*																		*/
/* Representation type utilities										 */
/*																		*/
/*----------------------------------------------------------------------*/
/* extern */ Boolean
XfeRepTypeCheck(Widget				w,
				String				rep_type,
				unsigned char *		address,
				unsigned char		fallback)
/*----------------------------------------------------------------------*/
{
	Boolean result = True;

	assert( address != NULL );

	if (!XmRepTypeValidValue(XmRepTypeGetId(rep_type),*address,w))
	{
		result = False;

		*address = fallback;
	}

	return result;
}
/*----------------------------------------------------------------------*/
/* extern */ void
XfeRepTypeRegister(String rep_type,String * names)
{
	Cardinal num_names = 0;

	assert( rep_type != NULL );

	while(names[num_names] != NULL)
	{
		num_names++;
	}

	assert( num_names > 0 );

    XmRepTypeRegister(rep_type,names,NULL,num_names);
}
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*																		*/
/* Translation / Action functions										*/
/*																		*/
/*----------------------------------------------------------------------*/
/* extern */ void
XfeOverrideTranslations(Widget w,String table)
/*----------------------------------------------------------------------*/
{
	XtTranslations parsed = NULL;

	assert( _XfeIsAlive(w) );
	assert( table != NULL );
	
	parsed = XtParseTranslationTable(table);
	
	XtOverrideTranslations(w,parsed);
}
/*----------------------------------------------------------------------*/
/* extern */ void
XfeAddActions(Widget w,XtActionList actions,Cardinal num_actions)
/*----------------------------------------------------------------------*/
{
	assert( _XfeIsAlive(w) );
	assert( actions != NULL );
	assert( num_actions > 0 );

	XtAppAddActions(XtWidgetToApplicationContext(w),actions,num_actions);
}
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*																		*/
/* SetValue / GetValue utilities                                        */
/*																		*/
/*----------------------------------------------------------------------*/
/* extern */ void
XfeSetValue(Widget w,String resource_name,XtArgVal value)
/*----------------------------------------------------------------------*/
{
	Arg av[1];
	
	assert( _XfeIsAlive(w) ) ;
	assert( resource_name != NULL );

	XtSetArg(av[0],resource_name,value);
	
	XtSetValues(w,av,1);
}
/*----------------------------------------------------------------------*/
/* extern */ XtArgVal
XfeGetValue(Widget w,String resource_name)
/*----------------------------------------------------------------------*/
{
	XtArgVal	value = 0;
	Arg			av[1];
	
	assert( _XfeIsAlive(w) ) ;
	assert( resource_name != NULL );

	XtSetArg(av[0],resource_name,&value);
	
	XtGetValues(w,av,1);

	return value;
}
/*----------------------------------------------------------------------*/
