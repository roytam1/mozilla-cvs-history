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
/* Name:		ToolTipTest.c											*/
/* Description:	Test for XfeToolTip widget.								*/
/* Author:		Ramiro Estrugo <ramiro@netscape.com>					*/
/*																		*/
/*----------------------------------------------------------------------*/
 
#include <Xfe/XfeTest.h>
#include <Xfe/ToolTip.h>

static Widget	_button_gadgets[3];
static Widget	_label_gadgets[3];

static Widget	_button_widgets[3];
static Widget	_label_widgets[3];

/*----------------------------------------------------------------------*/
int
main(int argc,char *argv[])
{
	Widget	form;
	Widget	frame;
	
	XfeAppCreateSimple("ToolTipTest",&argc,argv,"MainFrame",&frame,&form);

	_button_widgets[0] = XmCreatePushButton(form,"ButtonWidget1",NULL,0);
	_button_widgets[1] = XmCreatePushButton(form,"ButtonWidget2",NULL,0);
	_button_widgets[2] = XmCreatePushButton(form,"ButtonWidget3",NULL,0);

	_label_widgets[0] = XmCreateLabel(form,"LabelWidget1",NULL,0);
	_label_widgets[1] = XmCreateLabel(form,"LabelWidget2",NULL,0);
	_label_widgets[2] = XmCreateLabel(form,"LabelWidget3",NULL,0);

	_button_gadgets[0] = XmCreatePushButtonGadget(form,"ButtonGadget1",NULL,0);
	_button_gadgets[1] = XmCreatePushButtonGadget(form,"ButtonGadget2",NULL,0);
	_button_gadgets[2] = XmCreatePushButtonGadget(form,"ButtonGadget3",NULL,0);

	_label_gadgets[0] = XmCreateLabelGadget(form,"LabelGadget1",NULL,0);
	_label_gadgets[1] = XmCreateLabelGadget(form,"LabelGadget2",NULL,0);
	_label_gadgets[2] = XmCreateLabelGadget(form,"LabelGadget3",NULL,0);

	XtVaSetValues(_button_gadgets[0],XmNtopWidget,_label_widgets[2],NULL);
	XtVaSetValues(_button_gadgets[1],XmNtopWidget,_button_gadgets[0],NULL);
	XtVaSetValues(_button_gadgets[2],XmNtopWidget,_button_gadgets[1],NULL);

	XtVaSetValues(_label_gadgets[0],XmNtopWidget,_button_gadgets[2],NULL);
	XtVaSetValues(_label_gadgets[1],XmNtopWidget,_label_gadgets[0],NULL);
	XtVaSetValues(_label_gadgets[2],XmNtopWidget,_label_gadgets[1],NULL);

/*  	XtRealizeWidget(form); */

	XtManageChild(_button_widgets[0]);
	XtManageChild(_button_widgets[1]);
	XtManageChild(_button_widgets[2]);

	XtManageChild(_label_widgets[0]);
	XtManageChild(_label_widgets[1]);
	XtManageChild(_label_widgets[2]);

	XtManageChild(_button_gadgets[0]);
	XtManageChild(_button_gadgets[1]);
	XtManageChild(_button_gadgets[2]);

	XtManageChild(_label_gadgets[0]);
	XtManageChild(_label_gadgets[1]);
	XtManageChild(_label_gadgets[2]);

	XtPopup(frame,XtGrabNone);

	XfeToolTipAdd(_button_widgets[0]);
	XfeToolTipAdd(_button_widgets[1]);
/* 	XfeToolTipAdd(_button_widgets[2]); */

	XfeToolTipAdd(_label_widgets[0]);
	XfeToolTipAdd(_label_widgets[1]);
/* 	XfeToolTipAdd(_label_widgets[2]); */

	XfeToolTipAdd(_button_gadgets[0]);
	XfeToolTipAdd(_button_gadgets[1]);
/* 	XfeToolTipAdd(_button_gadgets[2]); */

	XfeToolTipAdd(_label_gadgets[0]);
	XfeToolTipAdd(_label_gadgets[1]);
/* 	XfeToolTipAdd(_label_gadgets[2]); */


 	XfeToolTipGlobalSetEnabledState(True);

    XfeAppMainLoop();

	return 0;
}
/*----------------------------------------------------------------------*/
