/* -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; -*- */
/*
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

#include "nsMacMessagePump.h"
#include "nsMacMessageSink.h"
#include "nsWidgetsCID.h"
#include "nsToolkit.h"
#include "nscore.h"

#include <MacWindows.h>
#include <LPeriodical.h>
#include <ToolUtils.h>
#include <LowMem.h>

#define DRAW_ON_RESIZE

const char SUSPENDRESUMEMESSAGE = 0x01;
const char MOUSEMOVEDMESSAGE = 0xFA;

//nsWindow* nsMacMessagePump::gCurrentWindow = nsnull;   
//nsWindow* nsMacMessagePump::gGrabWindow = nsnull;			// need this for grabmouse

//static NS_DEFINE_IID(kITEXTWIDGETIID, NS_TEXTFIELD_CID);
nsMacMessagePump::nsWindowlessMenuEventHandler nsMacMessagePump::gWindowlessMenuEventHandler = nsnull;


bool IsUserWindow ( WindowPtr ) ;

// a small helper routine, inlined for efficiency
inline
bool IsUserWindow ( WindowPtr wp )
{
	return wp && (::GetWindowKind(wp) >= kApplicationWindowKind);
}


//=================================================================
/*  Constructor
 *  @update  dc 08/31/98
 *  @param   aToolkit -- The toolkit created by the application
 *  @return  NONE
 */
nsMacMessagePump::nsMacMessagePump(nsToolkit *aToolkit, nsMacMessageSink* aSink)
	: mToolkit(aToolkit), mMessageSink(aSink)
{
	mRunning = PR_FALSE;
}

//=================================================================
/*  Destructor
 *  @update  dc 08/31/98
 *  @param   NONE
 *  @return  NONE
 */
nsMacMessagePump::~nsMacMessagePump()
{
  //��� release the toolkits and sinks? not if we use COM_auto_ptr.
}

//=================================================================
/*  Runs the message pump for the macintosh.  Turns them into Raptor events
 *  @update  dc 08/31/98
 *  @param   NONE
 *  @return  A boolean which states how the pump terminated
 */
PRBool 
nsMacMessagePump::DoMessagePump()
{
	Boolean					haveEvent;
	EventRecord			theEvent;
	long						sleep				=	0;
	unsigned short	eventMask 	= (everyEvent - diskMask);

	mRunning = PR_TRUE;
	mInBackground = PR_FALSE;
	
	// calculate the region to watch
	RgnHandle mouseRgn = ::NewRgn();
	::SetRectRgn(mouseRgn, -32000, -32000, -32001, -32001);
	
	while (mRunning)
	{			
		::LMSetSysEvtMask(eventMask);	// we need keyUp events
		haveEvent = ::WaitNextEvent(eventMask, &theEvent, sleep, mouseRgn);

		if (haveEvent)
		{
			switch(theEvent.what)
			{
				case keyUp:
				case keyDown:
				case autoKey:
					DoKey(theEvent);
					break;

				case mouseDown:
					DoMouseDown(theEvent);
					break;

				case mouseUp:
					DoMouseUp(theEvent);
					break;

				case updateEvt:
					DoUpdate(theEvent);
					break;

				case activateEvt:
					DoActivate(theEvent);
					break;

				case osEvt:
					unsigned char eventType = ((theEvent.message >> 24) & 0x00ff);
					switch (eventType)
					{
						case SUSPENDRESUMEMESSAGE:
							if (theEvent.message & 0x00000001)
								mInBackground = PR_FALSE;		// resume message
							else
								mInBackground = PR_TRUE;		// suspend message
							break;

						case MOUSEMOVEDMESSAGE:
							DoMouseMove(theEvent);
							break;
					}
					break;
			}
		}
		else
		{
			DoIdle(theEvent);
			if (mRunning)
				LPeriodical::DevoteTimeToIdlers(theEvent);
		}

		if (mRunning)
			LPeriodical::DevoteTimeToRepeaters(theEvent);
	}

  return NS_OK;
}

#pragma mark -
//-------------------------------------------------------------------------
//
// DoUpdate
//
//-------------------------------------------------------------------------
void nsMacMessagePump::DoUpdate(EventRecord &anEvent)
{
	WindowPtr whichWindow = (WindowPtr)anEvent.message;
	if (IsUserWindow(whichWindow))
	{
		GrafPtr savePort;
		::GetPort(&savePort);
		::SetPort(whichWindow);
		::BeginUpdate(whichWindow);
#if 0		//���test���
				static Boolean aBool = 1;
				RGBColor green = {0,65535,0};
				RGBColor red   = {65535,0,0};
				::RGBForeColor((aBool ? &green : &red));
				aBool ^= 1;
				::PenSize(2,2);
				::ClipRect(&whichWindow->portRect);
				::FrameRgn(whichWindow->visRgn);
#endif	//���������
		// The app can do its own updates here
		DispatchOSEventToRaptor(anEvent, whichWindow);
		::EndUpdate(whichWindow);
		::SetPort(savePort);
	}
}


//-------------------------------------------------------------------------
//
// DoMouseDown
//
//-------------------------------------------------------------------------

void nsMacMessagePump::DoMouseDown(EventRecord &anEvent)
{
		WindowPtr			whichWindow;
		PRInt16				partCode;

	partCode = ::FindWindow(anEvent.where, &whichWindow);

	switch (partCode)
	{
			case inSysWindow:
				break;

			case inMenuBar:
			{
			  long menuResult = ::MenuSelect(anEvent.where);
			  if (HiWord(menuResult) != 0)
			      DoMenu(anEvent, menuResult);
				break;
			}

			case inContent:
			{
				::SetPort(whichWindow);
				if (IsWindowHilited(whichWindow))
					DispatchOSEventToRaptor(anEvent, whichWindow);
				else
					::SelectWindow(whichWindow);
				break;
			}

			case inDrag:
			{
				::SetPort(whichWindow);
				if (!(anEvent.modifiers & cmdKey))
					::SelectWindow(whichWindow);
				Rect screenRect = qd.screenBits.bounds;
				::InsetRect(&screenRect, 4, 4);
				screenRect.top += ::LMGetMBarHeight();
				::DragWindow(whichWindow, anEvent.where, &screenRect);

				::GetMouse(&anEvent.where);
				::LocalToGlobal(&anEvent.where);
				// it's not really necessary to send that event to Raptor but (who knows?)
				// some windows may want to know that they have been moved
				DispatchOSEventToRaptor(anEvent, whichWindow);
				break;
			}

			case inGrow:
			{
				::SetPort(whichWindow);
#ifdef DRAW_ON_RESIZE
				Point oldPt = anEvent.where;
				while (::WaitMouseUp())
				{
					LPeriodical::DevoteTimeToRepeaters(anEvent);
					Point newPt;
					::GetMouse(&newPt);
					::LocalToGlobal(&newPt);
					if (::DeltaPoint(oldPt, newPt))
					{
						Rect portRect = whichWindow->portRect;
						short	width = (portRect.right - portRect.left) + (newPt.h - oldPt.h);
						short	height = (portRect.bottom - portRect.top) + (newPt.v - oldPt.v);
						
						oldPt = newPt;
						::SizeWindow(whichWindow, width, height, true);
						::DrawGrowIcon(whichWindow);
						anEvent.where = newPt;	// important!
						DispatchOSEventToRaptor(anEvent, whichWindow);
					}
				}
#else
				Rect sizeRect;
				sizeRect.bottom = qd.screenBits.bounds.bottom;
				sizeRect.right = qd.screenBits.bounds.right;
				sizeRect.top = sizeRect.left = 75;
				long newSize = ::GrowWindow(whichWindow, anEvent.where, &sizeRect);
				if (newSize != 0)
					::SizeWindow(whichWindow, newSize & 0x0FFFF, (newSize >> 16) & 0x0FFFF, true);
				::DrawGrowIcon(whichWindow);
				Point newPt;
				::GetMouse(&newPt);
				::LocalToGlobal(&newPt);
				anEvent.where = newPt;	// important!
				DispatchOSEventToRaptor(anEvent, whichWindow);
#endif
				break;
			}

			case inGoAway:
			{
				::SetPort(whichWindow);
				if (::TrackGoAway(whichWindow, anEvent.where))
					DispatchOSEventToRaptor(anEvent, whichWindow);
				break;
			}

			case inZoomIn:
			case inZoomOut:
				break;
	}
}


//-------------------------------------------------------------------------
//
// DoMouseUp
//
//-------------------------------------------------------------------------
void nsMacMessagePump::DoMouseUp(EventRecord &anEvent)
{
		WindowPtr			whichWindow;
		PRInt16				partCode;

	partCode = ::FindWindow(anEvent.where, &whichWindow);
	if (whichWindow == nil)
	{
		// We need to report the event even when it happens over no window:
		// when the user clicks a widget, keeps the mouse button pressed and
		// releases it outside the window, the event needs to be reported to
		// the widget so that it can deactivate itself.
		whichWindow = ::FrontWindow();
	}
	DispatchOSEventToRaptor(anEvent, whichWindow);
}


//-------------------------------------------------------------------------
//
// DoMouseMove
//
//-------------------------------------------------------------------------
void  nsMacMessagePump::DoMouseMove(EventRecord &anEvent)
{
	// same thing as DoMouseUp
		WindowPtr			whichWindow;
		PRInt16				partCode;

	partCode = ::FindWindow(anEvent.where, &whichWindow);
	if (whichWindow == nil)
		whichWindow = ::FrontWindow();
	DispatchOSEventToRaptor(anEvent, whichWindow);
}


//-------------------------------------------------------------------------
//
// DoKey
// 
// This is called for keydown, keyup, and key repeating events. So we need
// to be careful not to do things twice.
//-------------------------------------------------------------------------
void  nsMacMessagePump::DoKey(EventRecord &anEvent)
{
	char theChar = (char)(anEvent.message & charCodeMask);
	if ((anEvent.what == keyDown) && ((anEvent.modifiers & cmdKey) != 0))
	{
		// do a menu key command
		long menuResult = ::MenuKey(theChar);
		if (HiWord(menuResult) != 0)
			DoMenu(anEvent, menuResult);
	}
	else
	{
		DispatchOSEventToRaptor(anEvent, ::FrontWindow());
	}
}


//-------------------------------------------------------------------------
//
// DoMenu
//
//-------------------------------------------------------------------------
void  nsMacMessagePump::DoMenu(EventRecord &anEvent, long menuResult)
{
	// The app can handle its menu commands here or
	// in the nsNativeBrowserWindow and nsNativeViewerApp
	if (::FrontWindow() != nil)
	{
		DispatchMenuCommandToRaptor(anEvent, menuResult);
	}
	else
	{
		if (gWindowlessMenuEventHandler != nsnull)
			gWindowlessMenuEventHandler(menuResult);
	}
	HiliteMenu(0);
}


//-------------------------------------------------------------------------
//
// DoActivate
//
//-------------------------------------------------------------------------
void  nsMacMessagePump::DoActivate(EventRecord &anEvent)
{
	WindowPtr whichWindow = (WindowPtr)anEvent.message;
	::SetPort(whichWindow);
	if (anEvent.modifiers & activeFlag)
	{
		::BringToFront(whichWindow);
		::HiliteWindow(whichWindow,TRUE);
	}
	else
	{
		::HiliteWindow(whichWindow,FALSE);
	}

	DispatchOSEventToRaptor(anEvent, whichWindow);
}


//-------------------------------------------------------------------------
//
// DoIdle
//
//-------------------------------------------------------------------------
void  nsMacMessagePump::DoIdle(EventRecord &anEvent)
{
	// send mouseMove event
		static Point	lastWhere = {0, 0};

	if (*(long*)&lastWhere == *(long*)&anEvent.where)
		return;

	lastWhere = anEvent.where;
	DoMouseMove(anEvent);

	// idle controls						//�TODO? : is this really necessary?
	WindowPtr win = ::FrontWindow();
	while (win)
	{
		::SetPort(win);
		::SetOrigin(0,0);
		::ClipRect(&win->portRect);
		::IdleControls(win);
		win = ::GetNextWindow(win);
	}
}


#pragma mark -
//-------------------------------------------------------------------------
//
// DispatchOSEventToRaptor
//
//-------------------------------------------------------------------------
void  nsMacMessagePump::DispatchOSEventToRaptor(
													EventRecord 	&anEvent,
													WindowPtr			aWindow)
{
	if (aWindow && IsUserWindow(aWindow))
		mMessageSink->DispatchOSEvent(anEvent, aWindow);
}


//-------------------------------------------------------------------------
//
// DispatchMenuCommandToRaptor
//
//-------------------------------------------------------------------------
void nsMacMessagePump::DispatchMenuCommandToRaptor(
													EventRecord 	&anEvent,
													long					menuResult)
{
	WindowPtr whichWindow = ::FrontWindow();
	if (whichWindow && IsUserWindow(whichWindow))
		mMessageSink->DispatchMenuCommand(anEvent, menuResult);

}
