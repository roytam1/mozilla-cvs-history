/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// Name:        ToolbarButton.cpp                                       //
//                                                                      //
// Description:	XFE_ToolbarButton class implementation.                 //
//              A toolbar push button.                                  //
//                                                                      //
// Author:		Ramiro Estrugo <ramiro@netscape.com>                    //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

#include "ToolbarButton.h"
#include "RDFUtils.h"
#include "BrowserFrame.h"			// for fe_reuseBrowser()

#include <Xfe/Button.h>

#include "prefapi.h"

//////////////////////////////////////////////////////////////////////////
XFE_ToolbarButton::XFE_ToolbarButton(XFE_Frame *		frame,
									 Widget				parent,
                                     HT_Resource		htResource,
									 const String		name) :
	XFE_ToolbarItem(frame,parent,htResource,name)
{
}
//////////////////////////////////////////////////////////////////////////
XFE_ToolbarButton::~XFE_ToolbarButton()
{
}
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
//
// Initialize
//
//////////////////////////////////////////////////////////////////////////
/* virtual */ void
XFE_ToolbarButton::initialize()
{
    Widget button = createBaseWidget(getParent(),getName());

	setBaseWidget(button);
}
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
//
// Sensitive interface
//
//////////////////////////////////////////////////////////////////////////
/* virtual */ void
XFE_ToolbarButton::setSensitive(Boolean state)
{
	XP_ASSERT( isAlive() );

	XfeSetPretendSensitive(m_widget,state);
}
//////////////////////////////////////////////////////////////////////////
/* virtual */ Boolean
XFE_ToolbarButton::isSensitive()
{
	XP_ASSERT( isAlive() );
	
	return XfeIsPretendSensitive(m_widget);
}
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
//
// Widget creation interface
//
//////////////////////////////////////////////////////////////////////////
/* virtual */ Widget
XFE_ToolbarButton::createBaseWidget(Widget			parent,
									const String	name)
{
	XP_ASSERT( XfeIsAlive(parent) );
	XP_ASSERT( name != NULL );

	Widget button;

	button = XtVaCreateWidget(name,
							  xfeButtonWidgetClass,
							  parent,
							  NULL);

	return button;
}
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
//
// Configure
//
//////////////////////////////////////////////////////////////////////////
/* virtual */ void
XFE_ToolbarButton::configure()
{
	XP_ASSERT( isAlive() );

	// TODO, all the toolbar item resizing magic
	// XtVaSetValues(m_widget,XmNforceDimensionToMax,False,NULL);

    // Set the item's label
    XFE_RDFUtils::setItemLabelString(getAncestorContext(),
									 m_widget,
									 getHtResource());

	// Set the item's style and layout
	HT_Resource		entry = getHtResource();
    int32			style = XFE_RDFUtils::getStyleForEntry(entry);
	unsigned char	layout = XFE_RDFUtils::getButtonLayoutForEntry(entry,
																   style);

    if (style == BROWSER_TOOLBAR_TEXT_ONLY)
	{
		XtVaSetValues(m_widget,
					  XmNpixmap,			XmUNSPECIFIED_PIXMAP,
					  XmNpixmapMask,		XmUNSPECIFIED_PIXMAP,
					  NULL);

		XtVaSetValues(m_widget, XmNbuttonLayout, layout, NULL);
	}
	else
	{
		Pixmap pixmap;
		Pixmap pixmapMask;

		XFE_RDFUtils::getPixmapsForEntry(m_widget,
										 getHtResource(),
										 &pixmap,
										 &pixmapMask,
										 NULL,
										 NULL);

        XtVaSetValues(m_widget,
                      XmNpixmap,		pixmap,
                      XmNpixmapMask,	pixmapMask,
                      XmNbuttonLayout,	layout,
                      NULL);
	}

}
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
//
// addCallbacks
//
//////////////////////////////////////////////////////////////////////////
/* virtual */ void
XFE_ToolbarButton::addCallbacks()
{
	XP_ASSERT( isAlive() );

	// Add activate callback
    XtAddCallback(m_widget,
				  XmNactivateCallback,
				  XFE_ToolbarButton::activateCB,
				  (XtPointer) this);

	// Add popup callback
	XtAddCallback(m_widget,
				  XmNbutton3DownCallback,
				  XFE_ToolbarButton::popupCB,
				  (XtPointer) this);
}
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
//
// Button callback interface
//
//////////////////////////////////////////////////////////////////////////
/* virtual */ void
XFE_ToolbarButton::activate()
{
	HT_Resource		entry = getHtResource();

	XP_ASSERT( entry != NULL );

	// Check for xfe commands
	if (XFE_RDFUtils::ht_IsFECommand(entry)) 
	{
		// Convert the entry name to a xfe command
		CommandType cmd = XFE_RDFUtils::ht_GetFECommand(entry);

		XP_ASSERT( cmd != NULL );

		// Fill in the command info
		XFE_CommandInfo info(XFE_COMMAND_EVENT_ACTION,
							 m_widget, 
							 NULL /*event*/,
							 NULL /*params*/,
							 0    /*num_params*/);

		XFE_Frame * frame = getAncestorFrame();

		XP_ASSERT( frame != NULL );
		
		// If the frame handles the command and its enabled, execute it.
		if (frame->handlesCommand(cmd,NULL,&info) &&
			frame->isCommandEnabled(cmd,NULL,&info))
		{
			xfe_ExecuteCommand(frame,cmd,NULL,&info);
			
			//_frame->notifyInterested(Command::commandDispatchedCallback, 
			//callData);
		}
	}
	// Other URLs
	else if (!HT_IsContainer(entry))
	{
		char *			address = HT_GetNodeURL(entry);
		URL_Struct *	url = NET_CreateURLStruct(address,NET_DONT_RELOAD);
		MWContext *		context = getAncestorContext();
		
		XP_ASSERT( context != NULL );
		
		fe_reuseBrowser(context,url);
	}
}
//////////////////////////////////////////////////////////////////////////
/* virtual */ void
XFE_ToolbarButton::popup()
{
	printf("popup(%s) - Write Me Please.\n",XtName(m_widget));
}
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
//
// Private callbacks
//
//////////////////////////////////////////////////////////////////////////
/* static */ void
XFE_ToolbarButton::activateCB(Widget		/* w */,
							  XtPointer		clientData,
							  XtPointer		/* callData */)
{
	XFE_ToolbarButton * button = (XFE_ToolbarButton *) clientData;

	XP_ASSERT( button != NULL );

	button->activate();
}
//////////////////////////////////////////////////////////////////////////
/* static */ void
XFE_ToolbarButton::popupCB(Widget			/* w */,
						   XtPointer		clientData,
						   XtPointer		/* callData */)
{
	XFE_ToolbarButton * button = (XFE_ToolbarButton *) clientData;

	XP_ASSERT( button != NULL );

	button->popup();
}
//////////////////////////////////////////////////////////////////////////
