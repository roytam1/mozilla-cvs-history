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
static void		button_layout_cb	(Widget,XtPointer,XtPointer);

static void		popup_eh				(Widget,XtPointer,XEvent *,Boolean *);

static void		update_popup_handler	(Widget,Widget);

static XfeMenuItemRec button_layout_items[] =
{
	{ "LayoutLabelOnly",		XfeMENU_RADIO,		button_layout_cb		},
	{ "LayoutLabelOnBottom",	XfeMENU_RADIO,		button_layout_cb		},
	{ "LayoutLabelOnLeft",		XfeMENU_RADIO,		button_layout_cb		},
	{ "LayoutLabelOnRight",		XfeMENU_RADIO,		button_layout_cb		},
	{ "LayoutLabelOnTop",		XfeMENU_RADIO,		button_layout_cb		},
	{ "LayoutPixmapOnly",		XfeMENU_RADIO,		button_layout_cb		},
	{ NULL }
};
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
static XfeMenuItemRec popup_items[] =
{
	{ "DestroyButton",	XfeMENU_PUSH,	destroy_button		},
	{ "Sep",			XfeMENU_SEP							},
	{ "AddButtons",		XfeMENU_PUSH,	add_buttons,	NULL,(XtPointer) 10 },
	{ "RemoveButtons",	XfeMENU_PUSH,	remove_buttons,	NULL,(XtPointer) 10 },
	{ "Sep",			XfeMENU_SEP							},
	{ "ButtonLayout",	XfeMENU_PANE,	NULL,	button_layout_items	},
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
    Cardinal	num_buttons = 20;
    Cardinal	num_separators = 0;

	XfeAppCreateSimple("ToolBarTest",&argc,argv,"MainFrame",&frame,&form);

    if (argc > 1)
    {
		num_buttons = atoi(argv[1]);
    }

    if (argc > 2)
    {
		num_separators = atoi(argv[2]);
    }

	_tool_bar = XfeCreateLoadedToolBar(form,
									   "ToolBar",
									   "Tool",
									   num_buttons,
									   num_separators,
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

#ifdef DEBUG
	printf("%s(%s) location = %s\n",
		   "__FUNCTION__",
		   XtName(w),
		   XfeDebugRepTypeValueToName(XmRToolBarIndicatorLocation,location));
#endif

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
		   "__FUNCTION__",
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
		   "__FUNCTION__",
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
button_layout_cb(Widget w,XtPointer client_data,XtPointer call_data)
{
	String		name = XtName(w);
	String		parent_name = XtName(XtParent(w));
	Arg			av[20];
	Cardinal	ac = 0;

	printf("button_layout_cb(%s,%s)\n",name,parent_name);

 	if (!XfeIsAlive(_tool_bar))
 	{
		return;
 	}

	if (strcmp(name,"LayoutLabelOnly") == 0)
	{
		XtSetArg(av[ac],XmNbuttonLayout,XmBUTTON_LABEL_ONLY); ac++;
	}
	else if (strcmp(name,"LayoutLabelOnBottom") == 0)
	{
		XtSetArg(av[ac],XmNbuttonLayout,XmBUTTON_LABEL_ON_BOTTOM); ac++;
	}
	else if (strcmp(name,"LayoutLabelOnLeft") == 0)
	{
		XtSetArg(av[ac],XmNbuttonLayout,XmBUTTON_LABEL_ON_LEFT); ac++;
	}
	else if (strcmp(name,"LayoutLabelOnRight") == 0)
	{
		XtSetArg(av[ac],XmNbuttonLayout,XmBUTTON_LABEL_ON_RIGHT); ac++;
	}
	else if (strcmp(name,"LayoutLabelOnTop") == 0)
	{
		XtSetArg(av[ac],XmNbuttonLayout,XmBUTTON_LABEL_ON_TOP); ac++;
	}
	else if (strcmp(name,"LayoutPixmapOnly") == 0)
	{
		XtSetArg(av[ac],XmNbuttonLayout,XmBUTTON_PIXMAP_ONLY); ac++;
	}

	if (ac)
	{
		XtSetValues(_tool_bar,av,ac);
	}
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
			   "__FUNCTION__",
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
