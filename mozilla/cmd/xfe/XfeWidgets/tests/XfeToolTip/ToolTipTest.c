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
/* Name:		ToolTipTest.c											*/
/* Description:	Test for XfeToolTip widget.								*/
/* Author:		Ramiro Estrugo <ramiro@netscape.com>					*/
/*																		*/
/*----------------------------------------------------------------------*/
 
#include <Xfe/XfeTest.h>
#include <Xfe/ToolTip.h>

/*----------------------------------------------------------------------*/
static void
string_obtain_cb	(Widget			w,
					 XtPointer		client_data,
					 XmString *		string_return,
					 Boolean *		need_to_free_string);
/*----------------------------------------------------------------------*/
static void
doc_string_cb		(Widget			w,
					 XtPointer		client_data,
					 unsigned char	reason,
					 XmString		string);
/*----------------------------------------------------------------------*/


static Widget	_button_gadgets[3];
static Widget	_label_gadgets[3];

static Widget	_button_widgets[3];
static Widget	_label_widgets[3];

static Widget	_status_label = NULL;


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

	_status_label = XfeCreateLabel(form,"StatusLabel",NULL,0);

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

	XtManageChild(_status_label);


	/* Tip Strings */
	XfeTipStringAdd(_button_widgets[0]);
	XfeTipStringAdd(_button_widgets[1]);
/* 	XfeTipStringAdd(_button_widgets[2]); */

	XfeTipStringAdd(_label_widgets[0]);
	XfeTipStringAdd(_label_widgets[1]);
/* 	XfeTipStringAdd(_label_widgets[2]); */

	XfeTipStringAdd(_button_gadgets[0]);
	XfeTipStringAdd(_button_gadgets[1]);
/* 	XfeTipStringAdd(_button_gadgets[2]); */

	XfeTipStringAdd(_label_gadgets[0]);
	XfeTipStringAdd(_label_gadgets[1]);
/* 	XfeTipStringAdd(_label_gadgets[2]); */

	XfeTipStringSetObtainCallback(_button_widgets[0],
								  string_obtain_cb,
								  NULL);
	
 	XfeTipStringGlobalSetEnabledState(True);

	/* Doc Strings */
	XfeDocStringAdd(_button_widgets[0]);
	XfeDocStringAdd(_button_widgets[1]);
/* 	XfeDocStringAdd(_button_widgets[2]); */

	XfeDocStringAdd(_label_widgets[0]);
	XfeDocStringAdd(_label_widgets[1]);
/* 	XfeDocStringAdd(_label_widgets[2]); */

	XfeDocStringAdd(_button_gadgets[0]);
	XfeDocStringAdd(_button_gadgets[1]);
/* 	XfeDocStringAdd(_button_gadgets[2]); */

	XfeDocStringAdd(_label_gadgets[0]);
	XfeDocStringAdd(_label_gadgets[1]);
/* 	XfeDocStringAdd(_label_gadgets[2]); */

	XfeDocStringSetObtainCallback(_button_widgets[0],
								  string_obtain_cb,
								  NULL);

	XfeDocStringSetCallback(_button_widgets[0],
							doc_string_cb,
							NULL);

	XtPopup(frame,XtGrabNone);

    XfeAppMainLoop();

	return 0;
}
/*----------------------------------------------------------------------*/
static void
string_obtain_cb(Widget		w,
				 XtPointer	client_data,
				 XmString *	string_return,
				 Boolean *	need_to_free_string)
{
	static int count = 1;

	char buf[256];

	sprintf(buf,"%s_%d","Count",count++);

	*string_return = XmStringCreateLocalized(buf);
	*need_to_free_string = True;
}
/*----------------------------------------------------------------------*/
static void
doc_string_cb(Widget				w,
			  XtPointer				client_data,
			  unsigned char	reason,
			  XmString				string)
{
	if (!XfeIsAlive(_status_label))
	{
		return;
	}

	if (reason == XfeDOC_STRING_SET)
	{
 		XfeLabelSetString(_status_label,string);
	}
	else if (reason == XfeDOC_STRING_CLEAR)
	{
		XfeLabelSetStringPSZ(_status_label,"");
	}
}
/*----------------------------------------------------------------------*/
