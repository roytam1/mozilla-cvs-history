/* -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; -*- */
/*
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
 * Communications Corporation.	Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

// 
// nsMacMessagePump
//
// This file contains the default implementation for the mac event loop. Events that
// pertain to the layout engine are routed there via a MessageSink that is passed in
// at creation time. Events not destined for layout are handled here (such as window
// moved).
//
// Clients may either use this implementation or write their own. Embedding applications
// will almost certainly write their own because they will want control of the event
// loop to do other processing. There is nothing in the architecture which forces the
// embedding app to use anything called a "message pump" so the event loop can actually
// live anywhere the app wants.
//

#include "nsMacMessagePump.h"
#include "nsMacMessageSink.h"
#include "nsWidgetsCID.h"
#include "nsToolkit.h"
#include "nscore.h"

#include "nsRepeater.h"

#include "nsIEventQueueService.h"
#include "nsIServiceManager.h"

#include "prthread.h"
#include "nsMacTSMMessagePump.h"
#include "nsIRollupListener.h"
#include "nsIWidget.h"
#include "nsGfxUtils.h"
#include "nsMacWindow.h"

#include <MacWindows.h>
#include <ToolUtils.h>
#include <DiskInit.h>
#include <LowMem.h>
#include <Devices.h>
#include <Quickdraw.h>
#include "nsCarbonHelpers.h"
#include "nsWatchTask.h"


#include "nsISocketTransportService.h"
#include "nsIFileTransportService.h"


#ifndef topLeft
#define topLeft(r)	(((Point *) &(r))[0])
#endif

#ifndef botRight
#define botRight(r) (((Point *) &(r))[1])
#endif

#if DEBUG
#include <SIOUX.h>
#include "macstdlibextras.h"
#endif

#define DRAW_ON_RESIZE	0		// if 1, enable live-resize except when the command key is down

const short kMinWindowWidth = 125;
const short kMinWindowHeight = 150;

NS_WIDGET nsMacMessagePump::nsWindowlessMenuEventHandler nsMacMessagePump::gWindowlessMenuEventHandler = nsnull;


extern nsIRollupListener * gRollupListener;
extern nsIWidget				 * gRollupWidget;



//======================================================================================
//									PROFILE
//======================================================================================
#ifdef DEBUG

// Important Notes:
// ----------------
//
//	- To turn the profiler on, define "#pragma profile on" in IDE_Options.h
//		then set $PROFILE to 1 in BuildNGLayoutDebug.pl and recompile everything.
//
//	- You may need to turn the profiler off ("#pragma profile off")
//		in NSPR.Debug.Prefix because of incompatiblity with NSPR threads.
//		It usually isn't a problem but it may be one when profiling things like
//		imap or network i/o.
//
//	- The profiler utilities (ProfilerUtils.c) and the profiler
//		shared library (ProfilerLib) sit in NSRuntime.mcp.
//

		// Define this if you want to start profiling when the Caps Lock
		// key is pressed. Press Caps Lock, start the command you want to
		// profile, release Caps Lock when the command is done. It works
		// for all the major commands: display a page, open a window, etc...
		//
		// If you want to profile the project, you must make sure that the
		// global prefix file (IDE_Options.h) contains "#pragma profile on".

//#define PROFILE


		// Define this if you want to let the profiler run while you're
		// spending time in other apps. Usually you don't.

//#define PROFILE_WAITNEXTEVENT


#ifdef PROFILE
#include "ProfilerUtils.h"
#endif //PROFILE
#endif //DEBUG


//======================================================================================

static Boolean KeyDown(const UInt8 theKey)
{
	KeyMap map;
	GetKeys(map);
	return ((*((UInt8 *)map + (theKey >> 3)) >> (theKey & 7)) & 1) != 0;
}

//=================================================================

static long ConvertOSMenuResultToPPMenuResult(long menuResult)
{
	// Convert MacOS menu item to PowerPlant menu item because
	// in our sample app, we use Constructor for resource editing
	long menuID = HiWord(menuResult);
	long menuItem = LoWord(menuResult);
	SInt16**	theMcmdH = (SInt16**) ::GetResource('Mcmd', menuID);
	if (theMcmdH != nil)
	{
		if (::GetHandleSize((Handle)theMcmdH) > 0)
		{
			SInt16 numCommands = (*theMcmdH)[0];
			if (numCommands >= menuItem)
			{
				SInt32* theCommandNums = (SInt32*)(&(*theMcmdH)[1]);
				menuItem = theCommandNums[menuItem-1];
			}
		}
		::ReleaseResource((Handle) theMcmdH);
	}
	menuResult = (menuID << 16) + menuItem;
	return (menuResult);
}

#pragma mark -

static NS_DEFINE_CID(kEventQueueServiceCID, NS_EVENTQUEUESERVICE_CID);

//=================================================================
/*	Constructor
 *	@update	 dc 08/31/98
 *	@param	 aToolkit -- The toolkit created by the application
 *	@return	 NONE
 */
nsMacMessagePump::nsMacMessagePump(nsToolkit *aToolkit, nsMacMessageSink* aSink)
	: mToolkit(aToolkit), mMessageSink(aSink), mTSMMessagePump(NULL)
{
	mRunning = PR_FALSE;
	mMouseRgn = ::NewRgn();

	//
	// create the TSM Message Pump
	//
	mTSMMessagePump = nsMacTSMMessagePump::GetSingleton();
	NS_ASSERTION(mTSMMessagePump!=NULL,"nsMacMessagePump::nsMacMessagePump: Unable to create TSM Message Pump.");

  // startup the watch cursor idle time vbl task
  nsWatchTask::GetTask().Start();
}

//=================================================================
/*	Destructor
 *	@update	 dc 08/31/98
 *	@param	 NONE
 *	@return	 NONE
 */
nsMacMessagePump::~nsMacMessagePump()
{
	if (mMouseRgn)
		::DisposeRgn(mMouseRgn);

	//�TODO? release the toolkits and sinks? not if we use COM_auto_ptr.
	
	//
	// release the TSM Message Pump
	//
}

//=================================================================
/*	Runs the message pump for the macintosh
 *	@update	 dc 08/31/98
 *	@param	 NONE
 */
void nsMacMessagePump::DoMessagePump()
{
	PRBool				haveEvent;
	EventRecord			theEvent;

	nsToolkit::AppInForeground();
	
	while (mRunning)
	{			
#ifdef PROFILE 
		if (KeyDown(0x39))	// press [caps lock] to start the profile
			ProfileStart();
		else
			ProfileStop(); 
#endif // PROFILE 

#ifdef PROFILE 
#ifndef PROFILE_WAITNEXTEVENT 
		ProfileSuspend(); 
#endif // PROFILE_WAITNEXTEVENT 
#endif // PROFILE 

		haveEvent = GetEvent(theEvent);

#ifdef PROFILE 
#ifndef PROFILE_WAITNEXTEVENT 
		ProfileResume(); 
#endif // PROFILE_WAITNEXTEVENT 
#endif // PROFILE 

		DispatchEvent(haveEvent, &theEvent);
	}
}

static NS_DEFINE_CID(kSocketTransportServiceCID, NS_SOCKETTRANSPORTSERVICE_CID);
static NS_DEFINE_CID(kFileTransportServiceCID,   NS_FILETRANSPORTSERVICE_CID);

// #define BUSINESS_INDICATOR

//=================================================================
/* Return TRUE if the program is busy, like doing network stuff.
 *
*/
PRBool nsMacMessagePump::BrowserIsBusy()
{
#ifdef BUSINESS_INDICATOR
	static PRBool wasBusy;
#endif
	
	PRBool isBusy = PR_FALSE;
	
	nsCOMPtr<nsISocketTransportService> socketTransport = do_GetService(kSocketTransportServiceCID);
	if (socketTransport)
	{
		PRUint32 inUseTransports;
		socketTransport->GetInUseTransportCount(&inUseTransports);
		isBusy = inUseTransports > 0;
	}

	nsCOMPtr<nsIFileTransportService> fileTransport = do_GetService(kFileTransportServiceCID);
	if (fileTransport)
	{
		PRUint32 inUseTransports;
		fileTransport->GetInUseTransportCount(&inUseTransports);
		isBusy |= (inUseTransports > 0);
	}
	
#ifdef BUSINESS_INDICATOR
	if (isBusy != wasBusy)
	{
		if (isBusy)
			printf("�� Message pump became busy\n");
		else
			printf("�� Message pump became idle\n");
			
	  wasBusy = isBusy;
	}
#endif

	return isBusy;
}

//=================================================================
/*	Fetch a single event
 *	@update	 dc 08/31/98
 *	@param	 NONE
 *	@return	 A boolean which states whether we have a real event
 */
#define kEventAvailMask			(mDownMask | mUpMask | keyDownMask | keyUpMask | autoKeyMask | updateMask | activMask | osMask)

PRBool nsMacMessagePump::GetEvent(EventRecord &theEvent)
{
	const UInt32	kWNECallIntervalTicks = 4;
	static UInt32	sNextWNECall = 0;
	
	// Make sure we call WNE if we have user events, or the mouse is down
	EventRecord tempEvent;
	PRBool havePendingEvent = ::EventAvail(kEventAvailMask, &tempEvent) || !(tempEvent.modifiers & btnState);
	
	// don't call more than once every 4 ticks
	if (!havePendingEvent && (::TickCount() < sNextWNECall))
		return PR_FALSE;
	
	SInt32  sleepTime = (havePendingEvent || BrowserIsBusy()) ? 0 : 2;
	
	::SetEventMask(everyEvent); // we need keyUp events
	PRBool haveEvent = ::WaitNextEvent(everyEvent, &theEvent, sleepTime, mMouseRgn);
  
	sNextWNECall = ::TickCount() + kWNECallIntervalTicks;

#if !TARGET_CARBON
	if (haveEvent && ::TSMEvent(&theEvent) )
		haveEvent = PR_FALSE;
#endif

  nsWatchTask::GetTask().EventLoopReached();
  
	return haveEvent;
}

//=================================================================
/*	Dispatch a single event
 *	@param	 theEvent - the event to dispatch
 */
void nsMacMessagePump::DispatchEvent(PRBool aRealEvent, EventRecord *anEvent)
{

	if (aRealEvent == PR_TRUE)
	{

#if DEBUG
		if ((anEvent->what != kHighLevelEvent) && SIOUXHandleOneEvent(anEvent))
			return;
#endif

		switch(anEvent->what)
		{
			case keyUp:
			case keyDown:
			case autoKey:
				DoKey(*anEvent);
				break;

			case mouseDown:
				DoMouseDown(*anEvent);
				break;

			case mouseUp:
				DoMouseUp(*anEvent);
				break;

			case updateEvt:
				DoUpdate(*anEvent);
				break;

			case activateEvt:
				DoActivate(*anEvent);
				break;

			case diskEvt:
				DoDisk(*anEvent);
				break;

			case osEvt:
				{
				unsigned char eventType = ((anEvent->message >> 24) & 0x00ff);
				switch (eventType)
				{
					case suspendResumeMessage:
						if ( anEvent->message & resumeFlag )
							nsToolkit::AppInForeground();		// resume message
						else
							nsToolkit::AppInBackground();		// suspend message

						DoMouseMove(*anEvent);
						break;

					case mouseMovedMessage:
						DoMouseMove(*anEvent);
						break;
				}
				}
				break;
			
			case kHighLevelEvent:
				::AEProcessAppleEvent(anEvent);
			break;

		}
	}
	else
	{
		DoIdle(*anEvent);
		if (mRunning)
			Repeater::DoIdlers(*anEvent);

		// yield to other threads
		::PR_Sleep(PR_INTERVAL_NO_WAIT);
	}

	if (mRunning)
		Repeater::DoRepeaters(*anEvent);
}

#pragma mark -
//-------------------------------------------------------------------------
//
// DoUpdate
//
//-------------------------------------------------------------------------
//#include "ProfilerUtils.h"
void nsMacMessagePump::DoUpdate(EventRecord &anEvent)
{
	WindowPtr whichWindow = reinterpret_cast<WindowPtr>(anEvent.message);
	
	StPortSetter portSetter(whichWindow);
	
	::BeginUpdate(whichWindow);
	// The app can do its own updates here
	DispatchOSEventToRaptor(anEvent, whichWindow);
	::EndUpdate(whichWindow);

}


//-------------------------------------------------------------------------
//
// DoMouseDown
//
//-------------------------------------------------------------------------
void nsMacMessagePump::DoMouseDown(EventRecord &anEvent)
{
  WindowPtr			whichWindow;
  WindowPartCode				partCode;

  partCode = ::FindWindow(anEvent.where, &whichWindow);
  
	switch (partCode)
	{
      case inCollapseBox:   // we never seem to get this.
			case inSysWindow:
				if ( gRollupListener && gRollupWidget )
					gRollupListener->Rollup();
				break;

			case inMenuBar:
			{
				// If a xul popup is displayed, roll it up and don't allow the click
				// through to the menu code. This is how MacOS context menus work, so
				// I think this is a valid solution.
				if ( gRollupListener && gRollupWidget )
				{
					gRollupListener->Rollup();
				}
				else
				{
			    nsWatchTask::GetTask().Suspend();			  
					long menuResult = ::MenuSelect(anEvent.where);
					nsWatchTask::GetTask().Resume();
					if (HiWord(menuResult) != 0)
					{
						menuResult = ConvertOSMenuResultToPPMenuResult(menuResult);
						DoMenu(anEvent, menuResult);
					}
				}
				
				break;
			}

			case inContent:
			{
				::SetPortWindowPort(whichWindow);
				if ( IsWindowHilited(whichWindow) || (gRollupListener && gRollupWidget) )
					DispatchOSEventToRaptor(anEvent, whichWindow);
				else {
					nsMacWindow *mw = mMessageSink->GetNSWindowFromMacWindow(whichWindow);
					if (mw)
						mw->ComeToFront();
				}
				break;
			}

			case inDrag:
			{
				::SetPortWindowPort(whichWindow);

				// grrr... DragWindow calls SelectWindow, no way to stop it. For now,
				// we'll just let it come to the front and then push it back if necessary.
				Rect screenRect;
			  nsWatchTask::GetTask().Suspend();			  
				::GetRegionBounds(::GetGrayRgn(), &screenRect);
				::DragWindow(whichWindow, anEvent.where, &screenRect);
			  nsWatchTask::GetTask().Resume();			  

        // only activate if the command key is not down
        if (!(anEvent.modifiers & cmdKey))
        {
          nsMacWindow *mw = mMessageSink->GetNSWindowFromMacWindow(whichWindow);
          if (mw)
            mw->ComeToFront();
        }
        
				// Dispatch the event because some windows may want to know that they have been moved.
#if 0
				// Hack: we can't use GetMouse here because by the time DragWindow returns, the mouse
				// can be located somewhere else than in the drag bar.
				::GetMouse(&anEvent.where);
				::LocalToGlobal(&anEvent.where);
#else
				RgnHandle strucRgn = NewRgn();
				::GetWindowRegion ( whichWindow, kWindowStructureRgn, strucRgn );
				Rect strucRect;
				::GetRegionBounds(strucRgn, &strucRect);
				::SetPt(&anEvent.where, strucRect.left, strucRect.top);
				::DisposeRgn ( strucRgn );
#endif
				DispatchOSEventToRaptor(anEvent, whichWindow);
				break;
			}

			case inGrow:
			{
				::SetPortWindowPort(whichWindow);

				// use the cmd-key to do the opposite of the DRAW_ON_RESIZE setting.
				Boolean cmdKeyDown = (anEvent.modifiers & cmdKey) != 0;
				Boolean drawOnResize = DRAW_ON_RESIZE ? !cmdKeyDown : cmdKeyDown;
				if (drawOnResize)
				{
					Point oldPt = anEvent.where;
					while (::WaitMouseUp())
					{
						Repeater::DoRepeaters(anEvent);

						Point origin = {0,0};
						::LocalToGlobal(&origin);
						Point newPt;
						::GetMouse(&newPt);
						::LocalToGlobal(&newPt);
						if (::DeltaPoint(oldPt, newPt))
						{
							Rect portRect;
							::GetWindowPortBounds(whichWindow, &portRect);

							short width = newPt.h - origin.h;
							short height = newPt.v - origin.v;
							if (width < kMinWindowWidth)
								width = kMinWindowWidth;
							if (height < kMinWindowHeight)
								height = kMinWindowHeight;

							oldPt = newPt;
							::SizeWindow(whichWindow, width, height, true);

                            // simulate a click in the grow icon
							anEvent.where.h = width - 8;    // on Aqua, clicking at (width, height) misses the grow icon. inset a bit.
							anEvent.where.v = height - 8;
							::LocalToGlobal(&anEvent.where);
							DispatchOSEventToRaptor(anEvent, whichWindow);

							Boolean					haveEvent;
							EventRecord			updateEvent;
							haveEvent = ::WaitNextEvent(updateMask, &updateEvent, 0, nil);
#if !TARGET_CARBON
							if (haveEvent && TSMEvent(&updateEvent))
							{
								haveEvent = PR_FALSE;
							}
#endif
							if (haveEvent)
								DoUpdate(updateEvent);
						}
					}
				}
				else
				{
					Rect sizeRect;
					::GetRegionBounds(::GetGrayRgn(), &sizeRect);
					
					sizeRect.top = kMinWindowHeight;
					sizeRect.left = kMinWindowWidth;
			    nsWatchTask::GetTask().Suspend();			  
					long newSize = ::GrowWindow(whichWindow, anEvent.where, &sizeRect);
			    nsWatchTask::GetTask().Resume();			  
					if (newSize != 0)
						::SizeWindow(whichWindow, newSize & 0x0FFFF, (newSize >> 16) & 0x0FFFF, true);
					Rect portRect;
					Point newPt = botRight(*::GetWindowPortBounds(whichWindow, &portRect));
					::LocalToGlobal(&newPt);
					newPt.h -= 8, newPt.v -= 8;
					anEvent.where = newPt;	// important!
					DispatchOSEventToRaptor(anEvent, whichWindow);
				}
				break;
			}

			case inGoAway:
			{
			  nsWatchTask::GetTask().Suspend();			  
				::SetPortWindowPort(whichWindow);
				if (::TrackGoAway(whichWindow, anEvent.where)) {
				  nsWatchTask::GetTask().Resume();			  
					DispatchOSEventToRaptor(anEvent, whichWindow);
			  }
			  nsWatchTask::GetTask().Resume();			  
				break;
			}

			case inZoomIn:
			case inZoomOut:
			  nsWatchTask::GetTask().Suspend();			  
				if (::TrackBox(whichWindow, anEvent.where, partCode)) {
					GrafPtr		savePort;
					GDHandle	gdNthDevice;
					GDHandle	gdZoomDevice;
					Rect		theSect;
					Rect		tempRect;
					Rect		zoomRect;
					short		wTitleHeight;
					long		sectArea, greatestArea = 0;
					Boolean		sectFlag;
					
					nsWatchTask::GetTask().Resume();			  

					GetPort(&savePort);
					::SetPortWindowPort(whichWindow);
					Rect windRect;
					::GetWindowPortBounds(whichWindow, &windRect);
					::EraseRect(&windRect);
				
					if (partCode == inZoomOut) {
						LocalToGlobal((Point *)&windRect.top);
						LocalToGlobal((Point *)&windRect.bottom);

						RgnHandle structRgn = ::NewRgn();
						::GetWindowRegion ( whichWindow, kWindowStructureRgn, structRgn );
						Rect structRgnBounds;
						::GetRegionBounds ( structRgn, &structRgnBounds );
						wTitleHeight = windRect.top - 1 - structRgnBounds.top;
						::DisposeRgn ( structRgn );

						windRect.top -= wTitleHeight;
						gdNthDevice = GetDeviceList();
						while (gdNthDevice)
						{
							if (TestDeviceAttribute(gdNthDevice, screenDevice))
								if (TestDeviceAttribute(gdNthDevice, screenActive))
								{
									sectFlag = SectRect(&windRect, &(**gdNthDevice).gdRect, &theSect);
									sectArea = (theSect.right - theSect.left) * (theSect.bottom - theSect.top);
									if (sectArea > greatestArea)
									{
										greatestArea = sectArea;
										gdZoomDevice = gdNthDevice;
									}
								}
							gdNthDevice = GetNextDevice(gdNthDevice);
						}
						if (gdZoomDevice == GetMainDevice())
							wTitleHeight += GetMBarHeight();
						tempRect = (**gdZoomDevice).gdRect;
						SetRect(&zoomRect,
							tempRect.left + 3,
							tempRect.top + wTitleHeight + 3,
							tempRect.right - 64,
							tempRect.bottom - 3);
						::SetWindowStandardState ( whichWindow, &zoomRect );
					}
					nsWatchTask::GetTask().Resume();

					SetPort(savePort);
					
					// !!!	Do not call ZoomWindow before calling DispatchOSEventToRaptor
					//		otherwise nsMacEventHandler::HandleMouseDownEvent won't get
					//		the right partcode for the click location
					
					DispatchOSEventToRaptor(anEvent, whichWindow);
				}
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
void	nsMacMessagePump::DoMouseMove(EventRecord &anEvent)
{
	// same thing as DoMouseUp
	WindowPtr			whichWindow;
	PRInt16				partCode;

	if (mMouseRgn)
	{
		Point globalMouse = anEvent.where;		
		::SetRectRgn(mMouseRgn, globalMouse.h, globalMouse.v, globalMouse.h + 1, globalMouse.v + 1);
	}

	partCode = ::FindWindow(anEvent.where, &whichWindow);
	if (whichWindow == nil)
		whichWindow = ::FrontWindow();

	/* Disable mouse moved events for windowshaded windows -- this prevents tooltips
	   from popping up in empty space.
	*/
#if TARGET_CARBON
	if (whichWindow == nil || !::IsWindowCollapsed(whichWindow))
		DispatchOSEventToRaptor(anEvent, whichWindow);
#else
	if (whichWindow == nil || !::EmptyRgn(((WindowRecord *) whichWindow)->contRgn))
		DispatchOSEventToRaptor(anEvent, whichWindow);
#endif
}


//-------------------------------------------------------------------------
//
// DoKey
// 
// This is called for keydown, keyup, and key repeating events. So we need
// to be careful not to do things twice.
//-------------------------------------------------------------------------
void	nsMacMessagePump::DoKey(EventRecord &anEvent)
{
	char theChar = (char)(anEvent.message & charCodeMask);
	//if ((anEvent.what == keyDown) && ((anEvent.modifiers & cmdKey) != 0))
	//{
		// do a menu key command
	//	long menuResult = ::MenuKey(theChar);
	//	if (HiWord(menuResult) != 0)
	//	{
	//		menuResult = ConvertOSMenuResultToPPMenuResult(menuResult);
	//		DoMenu(anEvent, menuResult);
	//	}
	//}
	//else
	{
		PRBool handled = DispatchOSEventToRaptor(anEvent, ::FrontWindow());
		/* we want to call this if cmdKey is pressed and no other modifier keys are pressed */
		if((!handled) && (anEvent.what == keyDown) && (anEvent.modifiers == cmdKey) )
		{
			// do a menu key command
			long menuResult = ::MenuKey(theChar);
			if (HiWord(menuResult) != 0)
			{
					menuResult = ConvertOSMenuResultToPPMenuResult(menuResult);
				DoMenu(anEvent, menuResult);
			}
		}
	}
}


//-------------------------------------------------------------------------
//
// DoDisk
//
//-------------------------------------------------------------------------
void nsMacMessagePump::DoDisk(const EventRecord& anEvent)
{
#if !TARGET_CARBON
	if (HiWord(anEvent.message) != noErr)
	{
		// Error mounting disk. Ask if user wishes to format it.	
		Point pt = {120, 120};	// System 7 will auto-center dialog
		::DILoad();
		::DIBadMount(pt, (SInt32) anEvent.message);
		::DIUnload();
	}
#endif
}


//-------------------------------------------------------------------------
//
// DoMenu
//
//-------------------------------------------------------------------------
void	nsMacMessagePump::DoMenu(EventRecord &anEvent, long menuResult)
{
	// The app can handle its menu commands here or
	// in the nsNativeBrowserWindow and nsNativeViewerApp
	
#if !TARGET_CARBON
extern const PRInt16 kAppleMenuID;	// Danger Will Robinson!!! - this currently requires
									// APPLE_MENU_HACK to be defined in nsMenu.h
									// One of these days it'll become a non-hack
									// and things will be less convoluted

	// See if it was the Apple Menu
	if (HiWord(menuResult) == kAppleMenuID)
	{
		short theItem = LoWord(menuResult);
		if (theItem > 2)
		{
			Str255	daName;
			GrafPtr savePort;
			
			::GetMenuItemText(::GetMenuHandle(kAppleMenuID), theItem, daName);
			::GetPort(&savePort);
			::OpenDeskAcc(daName);
			::SetPort(savePort);
			HiliteMenu(0);
			return;
		}
	}
#endif

	// Note that we still give Raptor a shot at the event as it will eventually
	// handle the About... selection
	
	if (mMessageSink->IsRaptorWindow(::FrontWindow()))
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
void	nsMacMessagePump::DoActivate(EventRecord &anEvent)
{
	WindowPtr whichWindow = (WindowPtr)anEvent.message;
	::SetPortWindowPort(whichWindow);
	if (anEvent.modifiers & activeFlag)
		::HiliteWindow(whichWindow,TRUE);
	else
		::HiliteWindow(whichWindow,FALSE);

	DispatchOSEventToRaptor(anEvent, whichWindow);
}


//-------------------------------------------------------------------------
//
// DoIdle
//
//-------------------------------------------------------------------------
void	nsMacMessagePump::DoIdle(EventRecord &anEvent)
{
	// send mouseMove event
	static Point	lastWhere = {0, 0};

	if (*(long*)&lastWhere == *(long*)&anEvent.where)
		return;

	EventRecord	localEvent = anEvent;
	localEvent.what = nullEvent;
	lastWhere = localEvent.where;
  DoMouseMove(localEvent);
}


#pragma mark -
//-------------------------------------------------------------------------
//
// DispatchOSEventToRaptor
//
//-------------------------------------------------------------------------
PRBool	nsMacMessagePump::DispatchOSEventToRaptor(
													EventRecord		&anEvent,
													WindowPtr			aWindow)
{
	if (mMessageSink->IsRaptorWindow(aWindow))
		return mMessageSink->DispatchOSEvent(anEvent, aWindow);
	return PR_FALSE;
}


//-------------------------------------------------------------------------
//
// DispatchMenuCommandToRaptor
//
//-------------------------------------------------------------------------
PRBool nsMacMessagePump::DispatchMenuCommandToRaptor(
													EventRecord		&anEvent,
													long					menuResult)
{
	PRBool		handled = PR_FALSE;

	if (mMessageSink->IsRaptorWindow(::FrontWindow()))
		handled = mMessageSink->DispatchMenuCommand(anEvent, menuResult);

	return handled;
}

