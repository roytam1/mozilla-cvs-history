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
/* Name:		CascadeTest.c											*/
/* Description:	Test for XfeCascade widget.								*/
/* Author:		Ramiro Estrugo <ramiro@netscape.com>					*/
/*																		*/
/*----------------------------------------------------------------------*/

#include <Xfe/XfeTest.h>

static String names[] =
{
    "One",
    "Two",
    "Three",
    "Four",
    "Five",
    "Six",
    "Seven",
    "Eight",
    "Nine",
    "Ten",
};

static Cardinal delays[] =
{
	0,
	200,
	0,
	0,
	300,
	1000,
	500,
	5000,
	400,
	0,
};

static Cardinal counts[] =
{
	0,
	1,
	2,
	4,
	8,
	16,
	32,
	64,
	128,
	1,
};

static void		activate_callback		(Widget,XtPointer,XtPointer);
static void		arm_callback			(Widget,XtPointer,XtPointer);
static void		disarm_callback			(Widget,XtPointer,XtPointer);
static void		popup_callback			(Widget,XtPointer,XtPointer);
static void		popdown_callback		(Widget,XtPointer,XtPointer);
static void		cascading_callback		(Widget,XtPointer,XtPointer);
static void		populate_callback		(Widget,XtPointer,XtPointer);

static Widget	create_button			(Widget,String,Cardinal,Cardinal);
static void		add_items				(Widget,Cardinal);
static Widget	add_item				(Widget,String);
static Widget	get_sub_menu_id			(Widget);

#define MAX_J 10

/*----------------------------------------------------------------------*/
int
main(int argc,char *argv[])
{
	Widget		form;
	Widget		frame;
    Widget		tool_bar;
    Cardinal	i;
    
	XfeAppCreateSimple("CascadeTest",&argc,argv,"MainFrame",&frame,&form);
    
    tool_bar = XtVaCreateManagedWidget("ToolBar",
									   xfeToolBarWidgetClass,
									   form,
									   NULL);
	
    for (i = 0; i < XtNumber(names); i++)
    {
		create_button(tool_bar,names[i],counts[i],delays[i]);
    }

	XtPopup(frame,XtGrabNone);
	
    XfeAppMainLoop();

	return 0;
}
/*----------------------------------------------------------------------*/
static void
activate_callback(Widget w,XtPointer client_data,XtPointer call_data)
{
    XfeButtonCallbackStruct * cbs = (XfeButtonCallbackStruct *) call_data;

	printf("Activate(%s)\n\n",XtName(w));
}
/*----------------------------------------------------------------------*/
static void
arm_callback(Widget w,XtPointer client_data,XtPointer call_data)
{
    XfeButtonCallbackStruct * cbs = (XfeButtonCallbackStruct *) call_data;

    printf("Arm(%s)\n\n",XtName(w));
}
/*----------------------------------------------------------------------*/
static void
disarm_callback(Widget w,XtPointer client_data,XtPointer call_data)
{
    printf("Disarm(%s)\n\n",XtName(w));
}
/*----------------------------------------------------------------------*/
static void
popup_callback(Widget w,XtPointer client_data,XtPointer call_data)
{
    printf("Popup(%s)\n\n",XtName(w));
}
/*----------------------------------------------------------------------*/
static void
popdown_callback(Widget w,XtPointer client_data,XtPointer call_data)
{
    printf("Popdown(%s)\n\n",XtName(w));
}
/*----------------------------------------------------------------------*/
static void
populate_callback(Widget w,XtPointer client_data,XtPointer call_data)
{
	Widget		submenu;
	Cardinal	num_children;

	XtVaGetValues(w,XmNsubMenuId,&submenu,NULL);

	assert( XfeIsAlive(submenu) );

	XfeChildrenGet(submenu,NULL,&num_children);


    printf("Populate(%s) Have %d menu items\n\n",XtName(w),num_children);

	if (strcmp(XtName(w),"One") == 0)
	{
		XfeCascadeDestroyChildren(w);
		/*XfeChildrenDestroy(submenu);*/

		XSync(XtDisplay(w),True);

		XmUpdateDisplay(w);

		add_items(w,30);
	}
}
/*----------------------------------------------------------------------*/
static void
cascading_callback(Widget w,XtPointer client_data,XtPointer call_data)
{
	Widget		submenu;
	Cardinal	num_children;

	XtVaGetValues(w,XmNsubMenuId,&submenu,NULL);

	assert( XfeIsAlive(submenu) );

	XfeChildrenGet(submenu,NULL,&num_children);

    printf("Cascading(%s) Have %d menu items\n\n",XtName(w),num_children);
}
/*----------------------------------------------------------------------*/
static Widget
create_button(Widget parent,String name,Cardinal item_count,Cardinal delay)
{
	Widget		button;
	Cardinal	i;
	char		buf[100];

	static String items[] =
	{
		"Item One",
		"Item Two",
		"Item Three",
		"Item Four",
		"Item Five",
		"Item Six",
		"Item Seven",
		"Item Eight",
		"Item Nine",
		"Item Ten",
	};
	
	button = XtVaCreateManagedWidget(name,
									 xfeCascadeWidgetClass,
									 parent,
									 XmNmappingDelay,		delay,
									 NULL);

	XtAddCallback(button,XmNactivateCallback,activate_callback,NULL);
	XtAddCallback(button,XmNarmCallback,arm_callback,NULL);
	XtAddCallback(button,XmNcascadingCallback,cascading_callback,NULL);
	XtAddCallback(button,XmNdisarmCallback,disarm_callback,NULL);
	XtAddCallback(button,XmNpopdownCallback,popdown_callback,NULL);
	XtAddCallback(button,XmNcascadingCallback,populate_callback,NULL);
	XtAddCallback(button,XmNpopupCallback,popup_callback,NULL);

	add_items(button,item_count);

	return button;
}
/*----------------------------------------------------------------------*/
static void
add_items(Widget parent,Cardinal item_count)
{

	if (item_count > 0)
	{
		Cardinal	i;
		char		buf[100];
		Widget		sub_menu_id = get_sub_menu_id(parent);
		
		assert( XfeIsAlive(sub_menu_id) );

		for (i = 0; i < item_count; i++)
		{
			sprintf(buf,"Item %-2d",i);

			add_item(sub_menu_id,buf);
		}
	}
}
/*----------------------------------------------------------------------*/
static Widget
add_item(Widget parent,String name)
{
	Widget item;

	item = XtVaCreateManagedWidget(name,
								   /*xmPushButtonGadgetClass,*/
								   xmPushButtonWidgetClass,
								   parent,
								   NULL);

	return item;
}
/*----------------------------------------------------------------------*/
static Widget
get_sub_menu_id(Widget w)
{
	Widget sub_menu_id;

	XtVaGetValues(w,XmNsubMenuId,&sub_menu_id,NULL);

	return sub_menu_id;
}
/*----------------------------------------------------------------------*/
