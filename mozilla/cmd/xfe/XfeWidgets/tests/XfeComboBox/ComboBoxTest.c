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
/* Name:		ComboBoxTest.c											*/
/* Description:	Test for XfeComboBox widget.							*/
/* Author:		Ramiro Estrugo <ramiro@netscape.com>					*/
/*																		*/
/*----------------------------------------------------------------------*/

#include <Xfe/XfeTest.h>
#include <Xfe/ComboBox.h>
#include <Xfe/FancyBox.h>

#include <Xfe/ListUtilP.h>

static void		activate_callback	(Widget,XtPointer,XtPointer);
static void		arm_callback		(Widget,XtPointer,XtPointer);
static void		disarm_callback		(Widget,XtPointer,XtPointer);

static void		combo_add_items		(Widget,String *,Cardinal);
static void		list_add_items		(Widget,String *,Cardinal);

static String items1[] =
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
	"Item Eleven",
	"Item Twelve",
};

static String items2[] =
{
	"Red",
	"Green",
	"Blue",
	"Brown",
	"Orange",
	"Cyan",
	"Magenta",
	"Black",
	"White",
	"Yellow"
};

static String items3[] =
{
	"North",
	"South",
	"East",
	"West",
	"South East",
	"South West",
	"North East",
	"North West"
};

static String items4[] =
{
	"January",
	"February",
	"March",
	"April",
	"May",
	"June",
	"July",
	"August",
	"September",
	"October",
	"November",
	"December"
};

#define num_items1	XtNumber(items1)
#define num_items2	XtNumber(items2)
#define num_items3	XtNumber(items3)
#define num_items4	XtNumber(items4)

/*----------------------------------------------------------------------*/
int
main(int argc,char *argv[])
{
	Widget		form;
	Widget		frame;
    Widget		cb[4];
    Widget		icon;
    Widget		list;
    
	XfeAppCreateSimple("ComboBoxTest",&argc,argv,"MainFrame",&frame,&form);
    
    cb[0] = XtVaCreateManagedWidget("ComboBox1",
								  xfeComboBoxWidgetClass,
								  form,
								  XmNcomboBoxType,XmCOMBO_BOX_EDITABLE,
								  NULL);
	
    cb[1] = XtVaCreateManagedWidget("ComboBox2",
								  xfeComboBoxWidgetClass,
								  form,
								  XmNcomboBoxType,XmCOMBO_BOX_READ_ONLY,
								  NULL);
	
    cb[2] = XtVaCreateManagedWidget("ComboBox3",
								  xfeFancyBoxWidgetClass,
								  form,
								  XmNcomboBoxType,XmCOMBO_BOX_EDITABLE,
								  NULL);
	
    cb[3] = XtVaCreateManagedWidget("ComboBox4",
									xfeFancyBoxWidgetClass,
									form,
									XmNcomboBoxType,XmCOMBO_BOX_READ_ONLY,
									NULL);
	
	icon = XtVaCreateManagedWidget("IC",
								   xfeButtonWidgetClass,
								   cb[2],
								   XmNbackground,		XfeBackground(cb[2]),
								   XmNforeground,		XfeForeground(cb[2]),
								   NULL);
	
	assert( XfeIsAlive(icon) );
	
	XtVaSetValues(
		icon,
		XmNpixmap,					XfeGetPixmap(icon,"proxy"),
		XmNpixmapMask,				XfeGetMask(icon,"proxy"),
		XmNraisedPixmap,			XfeGetPixmap(icon,"proxy_raised"),
		XmNraisedPixmapMask,		XfeGetMask(icon,"proxy_raised"),
		XmNbuttonLayout,			XmBUTTON_PIXMAP_ONLY,
		XmNfillOnEnter,				False,
		XmNfillOnArm,				False,
		XmNraiseOnEnter,			True,
		XmNshadowThickness,			0,
		XmNaccentBorderThickness,	0,
		NULL);


 	list = XmCreateScrolledList(form,"List",NULL,0);

/* 	_XfeXmListAccess(list,BaseX) += 10; */

/*     lw->list.BaseX = (Position )lw->list.margin_width + */

	XtManageChild(list);

	combo_add_items(cb[0],items1,num_items1);
	combo_add_items(cb[1],items2,num_items2);
	combo_add_items(cb[2],items3,num_items3);
	combo_add_items(cb[3],items4,num_items4);

	list_add_items(list,items1,num_items1);

	XtPopup(frame,XtGrabNone);
	
    XfeAppMainLoop();

	return 0;
}
/*----------------------------------------------------------------------*/
static void
combo_add_items(Widget w,String * items,Cardinal n)
{
	XmString *	table = NULL;
	Cardinal	i;

	assert( XfeIsAlive(w) );
	assert( XfeIsComboBox(w) );
	assert( items != NULL );
	assert( n > 0 );

	table = XfeGetXmStringTable(items,n);
	
	for (i = 0; i < n; i++)
	{
		XfeComboBoxAddItem(w,table[i],0);
	}

	XfeFreeXmStringTable(table,n);
}
/*----------------------------------------------------------------------*/
static void
list_add_items(Widget w,String * items,Cardinal n)
{
	XmString *	table = NULL;
	Cardinal	i;

	assert( XfeIsAlive(w) );
	assert( XmIsList(w) );
	assert( items != NULL );
	assert( n > 0 );

	table = XfeGetXmStringTable(items,n);
	
	for (i = 0; i < n; i++)
	{
		XmListAddItem(w,table[i],0);
	}

	XfeFreeXmStringTable(table,n);
}
/*----------------------------------------------------------------------*/
static void
activate_callback(Widget w,XtPointer client_data,XtPointer call_data)
{
    printf("Activate(%s)\n",XtName(w));
}
/*----------------------------------------------------------------------*/
static void
arm_callback(Widget w,XtPointer client_data,XtPointer call_data)
{
    printf("Arm(%s)\n",XtName(w));
}
/*----------------------------------------------------------------------*/
static void
disarm_callback(Widget w,XtPointer client_data,XtPointer call_data)
{
    printf("Disarm(%s)\n",XtName(w));
}
/*----------------------------------------------------------------------*/
