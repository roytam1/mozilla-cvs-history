/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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

#include "nsToolkit.h"
#include "nsWindow.h"
#include "nsGUIEvent.h"
#include <Quickdraw.h>
#include <Fonts.h>
#include <TextEdit.h>
#include <Dialogs.h>
#include <Traps.h>
#include <Events.h>
#include <Menus.h>


PRBool nsToolkit::mInit = PR_FALSE;
nsWindow* nsToolkit::mFocusedWidget = nsnull;
  

//=================================================================
/*  Constructor
 *  @update  dc 08/31/98
 *  @param   NONE
 *  @return  NONE
 */
nsToolkit::nsToolkit() 
{
	NS_INIT_REFCNT();
	
	// once only, macintosh specific initialization
	if( mInit == PR_FALSE)
		{
		mInit = PR_TRUE;
		InitGraf(&qd.thePort);
		InitFonts();
		InitWindows();
		InitMenus();
		TEInit();
		InitDialogs(0);
		InitCursor();	
		}
}


//=================================================================
/*  Destructor.
 *  @update  dc 08/31/98
 *  @param   NONE
 *  @return  NONE
 */
nsToolkit::~nsToolkit()
{
}

//=================================================================
/*  Set the focus to a widget, send out the appropriate focus/defocus events
 *  @update  dc 08/31/98
 *  @param   aMouseInside -- A boolean indicating if the mouse is inside the control
 *  @return  NONE
 */
void nsToolkit::SetFocus(nsWindow *aFocusWidget)
{ 
	nsGUIEvent		guiEvent;
	
	guiEvent.eventStructType = NS_GUI_EVENT;

	// tell the old widget, it is not focused
	if (mFocusedWidget)
	{
		guiEvent.message = NS_LOSTFOCUS;
		guiEvent.widget = mFocusedWidget;
		mFocusedWidget->DispatchWindowEvent(guiEvent);
	}
	
	// let the new one know
	if (aFocusWidget)
	{
		mFocusedWidget =  aFocusWidget;
		guiEvent.message = NS_GOTFOCUS;
		guiEvent.widget = mFocusedWidget;		
		mFocusedWidget->DispatchWindowEvent(guiEvent);
	}
}

//=================================================================
/*  nsISupports implementation macro's
 *  @update  dc 08/31/98
 *  @param   NONE
 *  @return  NONE
 */
NS_DEFINE_IID(kIToolkitIID, NS_ITOOLKIT_IID);
NS_IMPL_ISUPPORTS(nsToolkit,kIToolkitIID);

//=================================================================
/*  Initialize the Toolbox
 *  @update  dc 08/31/98
 *  @param   aThread -- A pointer to a PRThread, not really sure of its use for the Mac yet
 *  @return  NONE
 */
NS_IMETHODIMP nsToolkit::Init(PRThread *aThread)
{
	return NS_OK;
}
