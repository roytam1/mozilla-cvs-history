/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// Name:        WindowListMenu.cpp                                      //
//                                                                      //
// Description:	A menu class for listing all the xfe windows.           //
//              This class can be used with both XmCascadeButton and    //
//              XfeCascade widgets.                                     //
//                                                                      //
// Author:		Ramiro Estrugo <ramiro@netscape.com>                    //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

#include "WindowListMenu.h"
#include "MozillaApp.h"
#include "felocale.h"
#include "intl_csi.h"

#include <Xm/PushBG.h>
#include <Xm/ToggleBG.h>
#include <Xfe/Xfe.h>

#define MAX_ITEM_WIDTH 40

//////////////////////////////////////////////////////////////////////////
XFE_WindowListMenu::XFE_WindowListMenu(Widget w, XFE_Frame * frame)
{
	m_cascade = w;
	m_parentFrame = frame;

	XP_ASSERT( m_parentFrame != NULL );
	
	XtAddCallback(m_cascade,
				  XmNdestroyCallback,
				  &XFE_WindowListMenu::destroy_cb,
				  (XtPointer) this);
	
	XtAddCallback(m_cascade,
				  XmNcascadingCallback,
				  &XFE_WindowListMenu::cascading_cb,
				  (XtPointer) this);
	
	XtVaGetValues(m_cascade,XmNsubMenuId,&m_submenu,NULL);
	
	XtVaGetValues(m_submenu,XmNnumChildren,&m_firstslot,NULL);
	
	XP_ASSERT(m_submenu);
}
//////////////////////////////////////////////////////////////////////////
XFE_WindowListMenu::~XFE_WindowListMenu()
{
}
//////////////////////////////////////////////////////////////////////////
void
XFE_WindowListMenu::generate(Widget cascade, XtPointer, XFE_Frame *frame)
{
	XFE_WindowListMenu *obj;
	
	obj = new XFE_WindowListMenu(cascade, frame);
}
//////////////////////////////////////////////////////////////////////////
void
XFE_WindowListMenu::cascading()
{
	XP_List *	frame_list = getShownFrames();

	XFE_Frame *	frame;
	int			i;
	int			frame_count = XP_ListCount(frame_list);

	Cardinal	num_children;
	WidgetList	children;

	int			total_slots_needed;
	int			slots_to_add;
	int			count;

	XfeChildrenGet(m_submenu,&children,&num_children);

	XP_ASSERT( num_children > 1 );

	// Total number of slots needed
	total_slots_needed = m_firstslot + 1 + frame_count;

	// Number of slots to add
	slots_to_add = total_slots_needed - num_children;

	// Add more slots if needed
	if (slots_to_add > 0)
	{
		for (i = 0; i < slots_to_add; i++)
		{
			Widget item = XtVaCreateWidget(xfeCmdWindowListRaiseItem,
										   //xmToggleButtonGadgetClass,
										   xmPushButtonGadgetClass,
										   m_submenu,
										   NULL);

			XtAddCallback(item,
						  XmNactivateCallback,
						  //XmNvalueChangedCallback,
						  &XFE_WindowListMenu::item_activate_cb,
						  (XtPointer) this);
		}

		// Update num_slots, since we added stuff
		XfeChildrenGet(m_submenu,&children,&num_children);
	}

	count = 1;

	// Configure the items
	for (i = (int) m_firstslot + 1; i < (int) num_children; i++)
	{
		// Get the next frame
		frame = (XFE_Frame*) XP_ListNextObject(frame_list);

		// If the frame is valid, add its title to the slot buttons
		if (frame)
		{
			MWContext *			context = m_parentFrame->getContext();
			INTL_CharSetInfo	c = LO_GetDocumentCharacterSetInfo(context);
			XmFontList			font_list;
			char				name[1024];

			XP_SPRINTF(name,"%d. %s",count++,frame->getTitle());

			INTL_MidTruncateString(INTL_GetCSIWinCSID(c), 
								   name, 
								   name,
								   MAX_ITEM_WIDTH);

			XmString label = fe_ConvertToXmString((unsigned char *) name,
												  INTL_GetCSIWinCSID(c), 
												  NULL, 
												  XmFONT_IS_FONT,
												  &font_list);

			if (label)
			{
				XtVaSetValues(children[i],XmNlabelString,label,NULL);

				XmStringFree(label);
			}

			XtManageChild(children[i]);
		}
		// If the frame is not valid, the unmanage the slot button
		else
		{
			XtUnmanageChild(children[i]);
		}
	}

	// Update the display so that the gadget buttons get drawn
	XmUpdateDisplay(m_submenu);

	if (frame_list)
	{
		XP_ListDestroy(frame_list);
	}
}
//////////////////////////////////////////////////////////////////////////
void
XFE_WindowListMenu::item_activate(Widget item)
{
	XP_ASSERT( XfeIsAlive(m_submenu) );

	int i = XfeChildGetIndex(item) - m_firstslot;

	XP_ASSERT( i > 0 );

	if (i <= 0)
	{
		return;
	}

	XP_List * frame_list = getShownFrames();
	
	XFE_Frame * frame = (XFE_Frame *) XP_ListGetObjectNum(frame_list,i);
	
	XP_ASSERT( frame != NULL );
	
	if (frame)
	{
		frame->show();
	}

	XmUpdateDisplay(frame->getBaseWidget());

	if (frame_list)
	{
		XP_ListDestroy(frame_list);
	}
}
//////////////////////////////////////////////////////////////////////////
void
XFE_WindowListMenu::destroy_cb(Widget, XtPointer clientData, XtPointer)
{
  XFE_WindowListMenu *obj = (XFE_WindowListMenu*)clientData;

  delete obj;
}
//////////////////////////////////////////////////////////////////////////
void
XFE_WindowListMenu::cascading_cb(Widget, XtPointer clientData, XtPointer)
{
  XFE_WindowListMenu *obj = (XFE_WindowListMenu*)clientData;

  obj->cascading();
}
//////////////////////////////////////////////////////////////////////////
void
XFE_WindowListMenu::item_activate_cb(Widget item, XtPointer clientData, XtPointer)
{
	XFE_WindowListMenu *obj = (XFE_WindowListMenu*)clientData;

	obj->item_activate(item);
}
//////////////////////////////////////////////////////////////////////////
XP_List *
XFE_WindowListMenu::getShownFrames()
{
	XP_List *	frame_list = XFE_MozillaApp::theApp()->getAllFrameList();
	Cardinal	frame_count = XP_ListCount(frame_list);

	XP_List *	shown_frame_list = NULL;
	Cardinal	i;

	// Find the shown frames and add them to a list
	for (i = 0; i < frame_count; i++)
	{
		// Get the next frame
		XFE_Frame * frame = (XFE_Frame*) XP_ListNextObject(frame_list);

		// Add it to list if valid and shown
		if (frame && XfeIsAlive(frame->getBaseWidget()) && frame->isShown())
		{
			// Create a new list as soon as we find the first shown item
			if (!shown_frame_list)
			{
				shown_frame_list = XP_ListNew();
			}

			XP_ListAddObject(shown_frame_list,frame);
		}
	}

	return shown_frame_list;
}
//////////////////////////////////////////////////////////////////////////
