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
/* Name:		ToolBarTest.c											*/
/* Description:	Test for XfeToolBar widget.								*/
/* Author:		Ramiro Estrugo <ramiro@netscape.com>					*/
/*																		*/
/*----------------------------------------------------------------------*/

#include <Xfe/XfeTest.h>

static void		scale_cb			(Widget,XtPointer,XtPointer);
static void		hide_cb				(Widget,XtPointer,XtPointer);
static void		location_cb			(Widget,XtPointer,XtPointer);

static Widget	_tool_bar = NULL;
static Widget	_popup_menu = NULL;

static Widget	_popup_target = NULL;

static void		destroy_button		(Widget,XtPointer,XtPointer);
static void		add_buttons			(Widget,XtPointer,XtPointer);
static void		remove_buttons		(Widget,XtPointer,XtPointer);

static void		popup_eh				(Widget,XtPointer,XEvent *,Boolean *);

static void		update_popup_handler	(Widget,Widget);

/*----------------------------------------------------------------------*/
static XfeMenuItemRec popup_items[] =
{
	{ "DestroyButton",	XfeMENU_PUSH,	destroy_button		},
	{ "Sep",			XfeMENU_SEP							},
	{ "AddButtons",		XfeMENU_PUSH,	add_buttons,	NULL,(XtPointer) 10 },
	{ "RemoveButtons",	XfeMENU_PUSH,	remove_buttons,	NULL,(XtPointer) 10 },
	{ NULL }
};
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
int
main(int argc,char *argv[])
{
	Widget		form;
	Widget		frame;
    Widget		scale;
    Widget		hide;
    Widget		location_tool_bar;
    
	XfeAppCreateSimple("ToolBarTest",&argc,argv,"MainFrame",&frame,&form);
    
	_tool_bar = XfeCreateLoadedToolBar(form,
									  "ToolBar",
									  "Tool",
									  50,
/* 									  20, */
									   0,
									  XfeArmCallback,
									  XfeDisarmCallback,
									  XfeActivateCallback,
									  NULL);

	_popup_menu = XfePopupMenuCreate(XfeAncestorFindApplicationShell(form),
									 "PopupMenu",
									 popup_items,
									 NULL,
									 NULL,
									 0);

	update_popup_handler(_tool_bar,_popup_menu);

    scale = XtVaCreateManagedWidget("Scale",
                                    xmScaleWidgetClass,
                                    form,
									NULL);

	XtAddCallback(scale,XmNvalueChangedCallback,scale_cb,NULL);
	XtAddCallback(scale,XmNdragCallback,scale_cb,NULL);

    hide = XtVaCreateManagedWidget("Hide",
                                    xmPushButtonWidgetClass,
                                    form,
									NULL);

	XtAddCallback(hide,XmNactivateCallback,hide_cb,NULL);

    location_tool_bar = XfeCreateLoadedToolBar(form,
											   "LocationToolBar",
											   "Item",
											   4,
											   0,
											   NULL,
											   NULL,
											   location_cb,
											   NULL);

	XtPopup(frame,XtGrabNone);
	
    XfeAppMainLoop();

	return 0;
}
/*----------------------------------------------------------------------*/
static void
scale_cb(Widget w,XtPointer client_data,XtPointer call_data)
{
	int			value;

	assert( XfeIsAlive(_tool_bar) );

	XmScaleGetValue(w,&value);

	value = value / 10;

	printf("%s(%s,%d)\n","__FUNCTION__",XtName(w),value);

	XtVaSetValues(_tool_bar,XmNindicatorPosition,value,NULL);
}
/*----------------------------------------------------------------------*/
static void
hide_cb(Widget w,XtPointer client_data,XtPointer call_data)
{
	assert( XfeIsAlive(_tool_bar) );

	printf("%s(%s)\n","__FUNCTION__",XtName(w));

	XtVaSetValues(_tool_bar,XmNindicatorPosition,XmINDICATOR_DONT_SHOW,NULL);
}
/*----------------------------------------------------------------------*/
static void
location_cb(Widget w,XtPointer client_data,XtPointer call_data)
{
	unsigned char location = XmINDICATOR_LOCATION_NONE;

	assert( XfeIsAlive(_tool_bar) );

	if (strcmp(XtName(w),"Item1") == 0)
	{
		location = XmINDICATOR_LOCATION_NONE;
	}
	else if (strcmp(XtName(w),"Item2") == 0)
	{
		location = XmINDICATOR_LOCATION_BEGINNING;
	}
	else if (strcmp(XtName(w),"Item3") == 0)
	{
		location = XmINDICATOR_LOCATION_END;
	}
	else if (strcmp(XtName(w),"Item4") == 0)
	{
		location = XmINDICATOR_LOCATION_MIDDLE;
	}

	printf("%s(%s) location = %s\n",
		   "__FUNCTION__",
		   XtName(w),
		   XfeDebugRepTypeValueToName(XmRToolBarIndicatorLocation,location));

	XtVaSetValues(_tool_bar,XmNindicatorLocation,location,NULL);
}
/*----------------------------------------------------------------------*/
static void
destroy_button(Widget w,XtPointer client_data,XtPointer call_data)
{
	if (!XfeIsAlive(_popup_target))
	{
		return;
	}

	printf("XtDestroyWidget(%s)\n",XtName(_popup_target));

	XtDestroyWidget(_popup_target);

	_popup_target = NULL;
}
/*----------------------------------------------------------------------*/
static void
add_buttons(Widget w,XtPointer client_data,XtPointer call_data)
{
	Cardinal count = (Cardinal) client_data;

	printf("%s(%s,%d)\n",
		   __FUNCTION__,
		   XtName(w),
		   count);

#if 0	
	String name = XtName(w);

	Widget tw = XfeDescendantFindByName(_dash_board,name,XfeFIND_ANY,False);


	if (XfeIsAlive(tw))
	{
		XfeWidgetToggleManaged(tw);
	}
#endif
}
/*----------------------------------------------------------------------*/
static void
remove_buttons(Widget w,XtPointer client_data,XtPointer call_data)
{
	Cardinal count = (Cardinal) client_data;

	printf("%s(%s,%d)\n",
		   __FUNCTION__,
		   XtName(w),
		   count);

#if 0	
	String name = XtName(w);

	Widget tw = XfeDescendantFindByName(_dash_board,name,XfeFIND_ANY,False);


	if (XfeIsAlive(tw))
	{
		XfeWidgetToggleManaged(tw);
	}
#endif
}
/*----------------------------------------------------------------------*/
static void
popup_eh(Widget		w,
		 XtPointer	client_data,
		 XEvent *	event,
		 Boolean *	cont)
{
	Widget popup_menu = (Widget) client_data;

	if ((event->type == ButtonPress) && (event->xbutton.button == Button3))
	{
		XmMenuPosition(popup_menu,(XButtonPressedEvent *) event);

		_popup_target = w;

		XtManageChild(popup_menu);

		printf("%s(%s)\n",
			   __FUNCTION__,
			   XtName(w));
	}
	
	*cont = True;
}
/*----------------------------------------------------------------------*/
static void
update_popup_handler(Widget tool_bar,Widget popup_menu)
{
	XfeChildrenRemoveEventHandler(tool_bar,
								  ButtonPressMask,
								  True,
								  popup_eh,
								  (XtPointer) popup_menu);
	
	XtRemoveEventHandler(tool_bar,
						 ButtonPressMask,
						 True,
						 popup_eh,
						 (XtPointer) popup_menu);
	
	XfeChildrenAddEventHandler(tool_bar,
							   ButtonPressMask,
							   True,
							   popup_eh,
							   (XtPointer) popup_menu);

	XtAddEventHandler(tool_bar,
					  ButtonPressMask,
					  True,
					  popup_eh,
					  (XtPointer) popup_menu);
}
/*----------------------------------------------------------------------*/
