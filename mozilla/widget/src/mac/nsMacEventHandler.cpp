/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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

#include "nsMacEventHandler.h"

#include "nsWindow.h"
#include "nsMacWindow.h"
#include "prinrval.h"

#include <ToolUtils.h>
#include <DiskInit.h>
#include <LowMem.h>
#include <TextServices.h>
#include <UnicodeConverter.h>
#include <Script.h>
#include "nsIRollupListener.h"
//#define DEBUG_TSM
extern nsIRollupListener * gRollupListener;
extern nsIWidget         * gRollupWidget;


// from MacHeaders.c
#ifndef topLeft
	#define topLeft(r)	(((Point *) &(r))[0])
#endif
#ifndef botRight
	#define botRight(r)	(((Point *) &(r))[1])
#endif

PRBool	nsMacEventHandler::mInBackground = PR_FALSE;
PRBool	nsMacEventHandler::mMouseInWidgetHit = PR_FALSE;

nsMacEventDispatchHandler	gEventDispatchHandler;


//-------------------------------------------------------------------------
//
//-------------------------------------------------------------------------
nsMacEventDispatchHandler::nsMacEventDispatchHandler()
{
	mActiveWidget	= nsnull;
	mWidgetHit		= nsnull;
	mWidgetPointed	= nsnull;
}


//-------------------------------------------------------------------------
//
//-------------------------------------------------------------------------
nsMacEventDispatchHandler::~nsMacEventDispatchHandler()
{
	if (mActiveWidget)
	{
	  mActiveWidget->RemoveDeleteObserver(this);
	  mActiveWidget = nsnull;
	}

	if (mWidgetHit)
	{
	  mWidgetHit->RemoveDeleteObserver(this);
	  mWidgetHit = nsnull;
	}

	if (mWidgetPointed)
	{
	  mWidgetPointed->RemoveDeleteObserver(this);
	  mWidgetPointed = nsnull;
	}
}

//-------------------------------------------------------------------------
//
//-------------------------------------------------------------------------
void nsMacEventDispatchHandler::SetFocus(nsWindow *aFocusedWidget)
{
    // This short circut lives inside of nsEventStateManager now
	if (aFocusedWidget == mActiveWidget)
		return;
		
	nsGUIEvent guiEvent;
	guiEvent.eventStructType = NS_GUI_EVENT;
	guiEvent.point.x		= 0;
	guiEvent.point.y		= 0;
	guiEvent.time				= PR_IntervalNow();
	guiEvent.widget			= nsnull;
	guiEvent.nativeMsg	= nsnull;

	// tell the old widget it is not focused
	if (mActiveWidget)
	{
		mActiveWidget->RemoveDeleteObserver(this);

		guiEvent.message = NS_LOSTFOCUS;
		guiEvent.widget = mActiveWidget;
		mActiveWidget->DispatchWindowEvent(guiEvent);
	}

	mActiveWidget = aFocusedWidget;

	// let the new one know it got the focus
	if (mActiveWidget)
	{
		mActiveWidget->AddDeleteObserver(this);

		guiEvent.message = NS_GOTFOCUS;
		guiEvent.widget = mActiveWidget;		
		mActiveWidget->DispatchWindowEvent(guiEvent);
	}
}

//-------------------------------------------------------------------------
//
//-------------------------------------------------------------------------
void nsMacEventDispatchHandler::SetActivated(nsWindow *aActivatedWidget)
{
	if (aActivatedWidget == mActiveWidget)
		return;

	mActiveWidget = aActivatedWidget;

	// let the new one know it got activation
	if (mActiveWidget)
	{
		mActiveWidget->AddDeleteObserver(this);
		
	    // Dispatch an NS_ACTIVATE event 
		nsGUIEvent guiEvent;
		guiEvent.eventStructType = NS_GUI_EVENT;
		guiEvent.point.x	= 0;
		guiEvent.point.y	= 0;
		guiEvent.time		= PR_IntervalNow();
		guiEvent.widget		= nsnull;
		guiEvent.nativeMsg	= nsnull;
		guiEvent.message 	= NS_ACTIVATE;
		guiEvent.widget 	= mActiveWidget;
		mActiveWidget->DispatchWindowEvent(guiEvent);
	}
}

//-------------------------------------------------------------------------
//
//-------------------------------------------------------------------------
void nsMacEventDispatchHandler::SetDeactivated(nsWindow *aDeactivatedWidget)
{
	// let the old one know it lost activation
	if (mActiveWidget)
	{	
	    // Dispatch an NS_DEACTIVATE event 
		nsGUIEvent guiEvent;
		guiEvent.eventStructType = NS_GUI_EVENT;
		guiEvent.point.x	= 0;
		guiEvent.point.y	= 0;
		guiEvent.time		= PR_IntervalNow();
		guiEvent.widget		= nsnull;
		guiEvent.nativeMsg	= nsnull;
		guiEvent.message 	= NS_DEACTIVATE;
		guiEvent.widget 	= mActiveWidget;
		mActiveWidget->DispatchWindowEvent(guiEvent);
		
		mActiveWidget->RemoveDeleteObserver(this);
		mActiveWidget = nsnull;
	}
}

//-------------------------------------------------------------------------
//
//-------------------------------------------------------------------------
void nsMacEventDispatchHandler::SetWidgetHit(nsWindow *aWidgetHit)
{
	if (aWidgetHit == mWidgetHit)
		return;

	if (mWidgetHit)
	  if (! mWidgetHit->RemoveDeleteObserver(this))
	  	NS_WARNING("nsMacFocusHandler wasn't in the WidgetHit observer list");

	mWidgetHit = aWidgetHit;

	if (mWidgetHit)
	  mWidgetHit->AddDeleteObserver(this);
}


//-------------------------------------------------------------------------
//
//-------------------------------------------------------------------------
void nsMacEventDispatchHandler::SetWidgetPointed(nsWindow *aWidgetPointed)
{
	if (aWidgetPointed == mWidgetPointed)
		return;

	if (mWidgetPointed)
	  if (! mWidgetPointed->RemoveDeleteObserver(this))
	  	NS_WARNING("nsMacFocusHandler wasn't in the WidgetPointed observer list");

	mWidgetPointed = aWidgetPointed;

	if (mWidgetPointed)
	  mWidgetPointed->AddDeleteObserver(this);
}


//-------------------------------------------------------------------------
//
//-------------------------------------------------------------------------
void nsMacEventDispatchHandler::NotifyDelete(void* aDeletedObject)
{
	if (mActiveWidget == aDeletedObject)
		mActiveWidget = nsnull;
	else
	if (mWidgetHit == aDeletedObject)
		mWidgetHit = nsnull;
	else
	if (mWidgetPointed == aDeletedObject)
		mWidgetPointed = nsnull;
	else
		NS_WARNING("NotifyDelete: unknown widget");
}


#pragma mark -
//-------------------------------------------------------------------------
//
// nsMacEventHandler constructor/destructor
//
//-------------------------------------------------------------------------
nsMacEventHandler::nsMacEventHandler(nsMacWindow* aTopLevelWidget)
{
	OSErr	err;
	InterfaceTypeList supportedServices;
	
	mTopLevelWidget = aTopLevelWidget;
	
	//
	// create a TSMDocument for this window.  We are allocating a TSM document for
	// each Mac window
	//
	mTSMDocument = nsnull;
	supportedServices[0] = kTextService;
	err = ::NewTSMDocument(1,supportedServices,&mTSMDocument,(long)this);
	NS_ASSERTION(err==noErr,"nsMacEventHandler::nsMacEventHandler: NewTSMDocument failed.");

#ifdef DEBUG_TSM
	printf("nsMacEventHandler::nsMacEventHandler: created TSMDocument[%p]\n",mTSMDocument);
#endif
	
	mIMEIsComposing = PR_FALSE;
	mIMECompositionStr=nsnull;

}


nsMacEventHandler::~nsMacEventHandler()
{
	if (mTSMDocument)
		(void)::DeleteTSMDocument(mTSMDocument);
	if(nsnull != mIMECompositionStr) {
		nsAutoString::Recycle(mIMECompositionStr);
		mIMECompositionStr = nsnull;
	}
}



#pragma mark -
//-------------------------------------------------------------------------
//
// HandleOSEvent
//
//-------------------------------------------------------------------------
PRBool nsMacEventHandler::HandleOSEvent ( EventRecord& aOSEvent )
{
	PRBool retVal = PR_FALSE;

	switch (aOSEvent.what)
	{
		case keyUp:
		case keyDown:
		case autoKey:
			retVal = HandleKeyEvent(aOSEvent);
			break;

		case activateEvt:
			retVal = HandleActivateEvent(aOSEvent);
			break;

		case updateEvt:
			retVal = HandleUpdateEvent(aOSEvent);
			break;

		case mouseDown:
			retVal = HandleMouseDownEvent(aOSEvent);
			break;

		case mouseUp:
			retVal = HandleMouseUpEvent(aOSEvent);
			break;

		case osEvt:
			unsigned char eventType = ((aOSEvent.message >> 24) & 0x00ff);
			if (eventType == suspendResumeMessage)
			{
				if ((aOSEvent.message & 1) == resumeFlag) {
					mInBackground = PR_FALSE;		// resume message
				} else {
					mInBackground = PR_TRUE;		// suspend message
					if (nsnull != gRollupListener && (nsnull != gRollupWidget) ) {
						gRollupListener->Rollup();
					}
				}
				HandleActivateEvent(aOSEvent);
			}
			else {
				if (! mInBackground)
					retVal = HandleMouseMoveEvent(aOSEvent);
			}
			break;
	
		case nullEvent:
			if (! mInBackground)
				retVal = HandleMouseMoveEvent(aOSEvent);
			break;
	}

	return retVal;
}


//-------------------------------------------------------------------------
//
// Handle Menu commands
//
//-------------------------------------------------------------------------
PRBool nsMacEventHandler::HandleMenuCommand(
  EventRecord& aOSEvent,
  long         aMenuResult)
{
	// get the focused widget
	nsWindow* focusedWidget = gEventDispatchHandler.GetActive();
	if (!focusedWidget)
		focusedWidget = mTopLevelWidget;

	// nsEvent
	nsMenuEvent menuEvent;
	menuEvent.eventStructType = NS_MENU_EVENT;
	menuEvent.message		= NS_MENU_SELECTED;
	menuEvent.point.x		= aOSEvent.where.h;
	menuEvent.point.y		= aOSEvent.where.v;
	menuEvent.time			= PR_IntervalNow();

	// nsGUIEvent
	menuEvent.widget		= focusedWidget;
	menuEvent.nativeMsg	= (void*)&aOSEvent;

	// nsMenuEvent
	menuEvent.mMenuItem	= nsnull;						//�TODO: initialize mMenuItem
	menuEvent.mCommand	= aMenuResult;

	// dispatch the menu event
	PRBool eventHandled = focusedWidget->DispatchWindowEvent(menuEvent);
	
	// if the menu event is not processed by the focused widget, propagate it
	// through the different parents all the way up to the top-level window
	if (! eventHandled)
	{
		// make sure the focusedWidget wasn't changed or deleted
		// when we dispatched the event (even though if we get here, 
		// the event is supposed to not have been handled)
		if (focusedWidget == gEventDispatchHandler.GetActive())
		{
			nsCOMPtr<nsWindow> grandParent;
			nsCOMPtr<nsWindow> parent ( dont_AddRef((nsWindow*)focusedWidget->GetParent()) );
			while (parent)
			{
				menuEvent.widget = parent;
				eventHandled = parent->DispatchWindowEvent(menuEvent);
				if (eventHandled)
				{
					break;
				}
				else
				{
					grandParent = dont_AddRef((nsWindow*)parent->GetParent());
					parent = grandParent;
				}
			}
		}
	}

	return eventHandled;
}


//-------------------------------------------------------------------------
//
// DragEvent
//
//-------------------------------------------------------------------------
//
// Someone on the outside told us that something related to a drag is happening. The
// exact event type is passed in as |aMessage|. We need to send this event into Gecko 
// for processing. Create a Gecko event (using the appropriate message type) and pass
// it along.
//
// ���THIS REALLY NEEDS TO BE CLEANED UP! TOO MUCH CODE COPIED FROM ConvertOSEventToMouseEvent
//
PRBool nsMacEventHandler::DragEvent ( unsigned int aMessage, Point aMouseGlobal, UInt16 aKeyModifiers )
{
	nsMouseEvent geckoEvent;

	// convert the mouse to local coordinates. We have to do all the funny port origin
	// stuff just in case it has been changed.
	Point hitPointLocal = aMouseGlobal;
#if TARGET_CARBON
	GrafPtr grafPort = reinterpret_cast<GrafPtr>(mTopLevelWidget->GetNativeData(NS_NATIVE_GRAPHIC));
	::SetPort(grafPort);
	Rect savePortRect;
	::GetPortBounds(grafPort, &savePortRect);
#else
	GrafPtr grafPort = static_cast<GrafPort*>(mTopLevelWidget->GetNativeData(NS_NATIVE_GRAPHIC));
	::SetPort(grafPort);
	Rect savePortRect = grafPort->portRect;
#endif
	::SetOrigin(0, 0);
	::GlobalToLocal(&hitPointLocal);
	::SetOrigin(savePortRect.left, savePortRect.top);
	nsPoint widgetHitPoint(hitPointLocal.h, hitPointLocal.v);

	nsWindow* widgetHit = mTopLevelWidget->FindWidgetHit(hitPointLocal);
	if ( widgetHit ) {
		// adjust from local coordinates to window coordinates in case the top level widget
		// isn't at 0, 0
		nsRect bounds;
		mTopLevelWidget->GetBounds(bounds);
		nsPoint widgetOrigin(bounds.x, bounds.y);
		widgetHit->LocalToWindowCoordinate(widgetOrigin);
		widgetHitPoint.MoveBy(-widgetOrigin.x, -widgetOrigin.y);		
	}
	else {
	  // this is most likely the case of a drag exit, so we need to make sure
	  // we send the event to the last pointed to widget. We don't really care
	  // about the mouse coordinates because we know they're outside the window.
	  widgetHit = gEventDispatchHandler.GetWidgetPointed();
	  widgetHitPoint = nsPoint(0,0);
	}	

	// update the tracking of which widget the mouse is now over.
	gEventDispatchHandler.SetWidgetPointed(widgetHit);
	
	// nsEvent
	geckoEvent.eventStructType = NS_DRAGDROP_EVENT;
	geckoEvent.message = aMessage;
	geckoEvent.point = widgetHitPoint;
	geckoEvent.time	= PR_IntervalNow();

	// nsGUIEvent
	geckoEvent.widget = widgetHit;
	geckoEvent.nativeMsg = nsnull;

	// nsInputEvent
	geckoEvent.isShift = ((aKeyModifiers & shiftKey) != 0);
	geckoEvent.isControl = ((aKeyModifiers & controlKey) != 0);
	geckoEvent.isAlt = ((aKeyModifiers & optionKey) != 0);
	geckoEvent.isMeta = ((aKeyModifiers & cmdKey) != 0);

	// nsMouseEvent
	geckoEvent.clickCount = 1;
	
	if ( widgetHit )
		widgetHit->DispatchMouseEvent(geckoEvent);
	else
		NS_WARNING ("Oh shit, no widget to dispatch event to, we're in trouble" );
	
	return PR_TRUE;
	
} // DropOccurred


#pragma mark -

//-------------------------------------------------------------------------
//
// ConvertMacToRaptorKeyCode
//
//-------------------------------------------------------------------------


// Key code constants
enum
{
	kEscapeKeyCode			= 0x35,
	kCommandKeyCode     = 0x37,
	kShiftKeyCode				= 0x38,
	kCapsLockKeyCode		= 0x39,
	kControlKeyCode			= 0x3B,
	kOptionkeyCode			= 0x3A,		// left and right option keys
	kClearKeyCode				= 0x47,
	
	// function keys
	kF1KeyCode					= 0x7A,
	kF2KeyCode					= 0x78,
	kF3KeyCode					= 0x63,
	kF4KeyCode					= 0x76,
	kF5KeyCode					= 0x60,
	kF6KeyCode					= 0x61,
	kF7KeyCode					= 0x62,
	kF8KeyCode					= 0x64,
	kF9KeyCode					= 0x65,
	kF10KeyCode					= 0x6D,
	kF11KeyCode					= 0x67,
	kF12KeyCode					= 0x6F,
	kF13KeyCode					= 0x69,
	kF14KeyCode					= 0x6B,
	kF15KeyCode					= 0x71,
	
	kPrintScreenKeyCode	= kF13KeyCode,
	kScrollLockKeyCode	= kF14KeyCode,
	kPauseKeyCode				= kF15KeyCode,
	
	// keypad
	kKeypad0KeyCode			= 0x52,
	kKeypad1KeyCode			= 0x53,
	kKeypad2KeyCode			= 0x54,
	kKeypad3KeyCode			= 0x55,
	kKeypad4KeyCode			= 0x56,
	kKeypad5KeyCode			= 0x57,
	kKeypad6KeyCode			= 0x58,
	kKeypad7KeyCode			= 0x59,
	kKeypad8KeyCode			= 0x5B,
	kKeypad9KeyCode			= 0x5C,
	
	kKeypadMultiplyKeyCode	= 0x43,
	kKeypadAddKeyCode				= 0x45,
	kKeypadSubtractKeyCode	= 0x4E,
	kKeypadDecimalKeyCode		= 0x41,
	kKeypadDivideKeyCode		= 0x4B,
	kKeypadEqualsKeyCode		= 0x51,			// no correpsonding raptor key code
	kEnterKeyCode           = 0x4C,
	kReturnKeyCode          = 0x24,
	
	kInsertKeyCode					= 0x72,				// also help key
	kDeleteKeyCode					= 0x75,				// also forward delete key
	kTabKeyCode							= 0x30,
	kBackspaceKeyCode       = 0x33,
	kHomeKeyCode						= 0x73,	
	kEndKeyCode							= 0x77,
	kPageUpKeyCode					= 0x74,
	kPageDownKeyCode				= 0x79,
	kLeftArrowKeyCode				= 0x7B,
	kRightArrowKeyCode			= 0x7C,
	kUpArrowKeyCode					= 0x7E,
	kDownArrowKeyCode				= 0x7D
	
};

static PRUint32 ConvertMacToRaptorKeyCode(UInt32 eventMessage, UInt32 eventModifiers)
{
	UInt8			charCode = (eventMessage & charCodeMask);
	UInt8			keyCode = (eventMessage & keyCodeMask) >> 8;
	PRUint32	raptorKeyCode = 0;
	
	switch (keyCode)
	{
//	case ??							:				raptorKeyCode = NS_VK_CANCEL;		break;			// don't know what this key means. Nor does joki

// modifiers. We don't get separate events for these
		case kEscapeKeyCode:				raptorKeyCode = NS_VK_ESCAPE;					break;
		case kShiftKeyCode:					raptorKeyCode = NS_VK_SHIFT;					break;
//		case kCommandKeyCode:       raptorKeyCode = NS_VK_META;           break;
		case kCapsLockKeyCode:			raptorKeyCode = NS_VK_CAPS_LOCK;			break;
		case kControlKeyCode:				raptorKeyCode = NS_VK_CONTROL;				break;
		case kOptionkeyCode:				raptorKeyCode = NS_VK_ALT;						break;
		case kClearKeyCode:					raptorKeyCode = NS_VK_CLEAR;					break;

// function keys
		case kF1KeyCode:						raptorKeyCode = NS_VK_F1;							break;
		case kF2KeyCode:						raptorKeyCode = NS_VK_F2;							break;
		case kF3KeyCode:						raptorKeyCode = NS_VK_F3;							break;
		case kF4KeyCode:						raptorKeyCode = NS_VK_F4;							break;
		case kF5KeyCode:						raptorKeyCode = NS_VK_F5;							break;
		case kF6KeyCode:						raptorKeyCode = NS_VK_F6;							break;
		case kF7KeyCode:						raptorKeyCode = NS_VK_F7;							break;
		case kF8KeyCode:						raptorKeyCode = NS_VK_F8;							break;
		case kF9KeyCode:						raptorKeyCode = NS_VK_F9;							break;
		case kF10KeyCode:						raptorKeyCode = NS_VK_F10;						break;
		case kF11KeyCode:						raptorKeyCode = NS_VK_F11;						break;
		case kF12KeyCode:						raptorKeyCode = NS_VK_F12;						break;
//	case kF13KeyCode:						raptorKeyCode = NS_VK_F13;						break;		// clash with the 3 below
//	case kF14KeyCode:						raptorKeyCode = NS_VK_F14;						break;
//	case kF15KeyCode:						raptorKeyCode = NS_VK_F15;						break;
		case kPauseKeyCode:					raptorKeyCode = NS_VK_PAUSE;					break;
		case kScrollLockKeyCode:		raptorKeyCode = NS_VK_SCROLL_LOCK;		break;
		case kPrintScreenKeyCode:		raptorKeyCode = NS_VK_PRINTSCREEN;		break;
	
// keypad
		case kKeypad0KeyCode:				raptorKeyCode = NS_VK_NUMPAD0;				break;
		case kKeypad1KeyCode:				raptorKeyCode = NS_VK_NUMPAD1;				break;
		case kKeypad2KeyCode:				raptorKeyCode = NS_VK_NUMPAD2;				break;
		case kKeypad3KeyCode:				raptorKeyCode = NS_VK_NUMPAD3;				break;
		case kKeypad4KeyCode:				raptorKeyCode = NS_VK_NUMPAD4;				break;
		case kKeypad5KeyCode:				raptorKeyCode = NS_VK_NUMPAD5;				break;
		case kKeypad6KeyCode:				raptorKeyCode = NS_VK_NUMPAD6;				break;
		case kKeypad7KeyCode:				raptorKeyCode = NS_VK_NUMPAD7;				break;
		case kKeypad8KeyCode:				raptorKeyCode = NS_VK_NUMPAD8;				break;
		case kKeypad9KeyCode:				raptorKeyCode = NS_VK_NUMPAD9;				break;

		case kKeypadMultiplyKeyCode:	raptorKeyCode = NS_VK_MULTIPLY;			break;
		case kKeypadAddKeyCode:				raptorKeyCode = NS_VK_ADD;					break;
		case kKeypadSubtractKeyCode:	raptorKeyCode = NS_VK_SUBTRACT;			break;
		case kKeypadDecimalKeyCode:		raptorKeyCode = NS_VK_DECIMAL;			break;
		case kKeypadDivideKeyCode:		raptorKeyCode = NS_VK_DIVIDE;				break;
//	case ??								:				raptorKeyCode = NS_VK_SEPARATOR;		break;


// these may clash with forward delete and help
    case kInsertKeyCode:				raptorKeyCode = NS_VK_INSERT;					break;
    case kDeleteKeyCode:				raptorKeyCode = NS_VK_DELETE;					break;

    case kBackspaceKeyCode:     raptorKeyCode = NS_VK_BACK;           break;
    case kTabKeyCode:           raptorKeyCode = NS_VK_TAB;            break;
    case kHomeKeyCode:          raptorKeyCode = NS_VK_HOME;           break;
    case kEndKeyCode:           raptorKeyCode = NS_VK_END;            break;
		case kPageUpKeyCode:				raptorKeyCode = NS_VK_PAGE_UP;        break;
		case kPageDownKeyCode:			raptorKeyCode = NS_VK_PAGE_DOWN;      break;
		case kLeftArrowKeyCode:			raptorKeyCode = NS_VK_LEFT;           break;
		case kRightArrowKeyCode:		raptorKeyCode = NS_VK_RIGHT;          break;
		case kUpArrowKeyCode:				raptorKeyCode = NS_VK_UP;             break;
		case kDownArrowKeyCode:			raptorKeyCode = NS_VK_DOWN;           break;

		default:
				if ((eventModifiers & controlKey) != 0)
				  charCode += 64;
	  	
				// if we haven't gotten the key code already, look at the char code
				switch (charCode)
				{
					case kReturnCharCode:				raptorKeyCode = NS_VK_RETURN;				break;
					case kEnterCharCode:				raptorKeyCode = NS_VK_RETURN;				break;			// fix me!
					case ' ':										raptorKeyCode = NS_VK_SPACE;				break;
					case ';':										raptorKeyCode = NS_VK_SEMICOLON;		break;
					case '=':										raptorKeyCode = NS_VK_EQUALS;				break;
					case ',':										raptorKeyCode = NS_VK_COMMA;				break;
					case '.':										raptorKeyCode = NS_VK_PERIOD;				break;
					case '/':										raptorKeyCode = NS_VK_SLASH;				break;
					case '`':										raptorKeyCode = NS_VK_BACK_QUOTE;		break;
					case '{':
					case '[':										raptorKeyCode = NS_VK_OPEN_BRACKET;	break;
					case '\\':									raptorKeyCode = NS_VK_BACK_SLASH;		break;
					case '}':
					case ']':										raptorKeyCode = NS_VK_CLOSE_BRACKET;	break;
					case '\'':
					case '"':										raptorKeyCode = NS_VK_QUOTE;				break;
					
					default:
						
						if (charCode >= '0' && charCode <= '9')		// numerals
						{
							raptorKeyCode = charCode;
						}
						else if (charCode >= 'a' && charCode <= 'z')		// lowercase
						{
							raptorKeyCode = toupper(charCode);
						}
						else if (charCode >= 'A' && charCode <= 'Z')		// uppercase
						{
							raptorKeyCode = charCode;
						}

						break;
				}
	}

	return raptorKeyCode;
}


//-------------------------------------------------------------------------
//
// InitializeKeyEvent
//
//-------------------------------------------------------------------------

void nsMacEventHandler::InitializeKeyEvent(nsKeyEvent& aKeyEvent, EventRecord& aOSEvent, nsWindow* focusedWidget, PRUint32 message)
{
	//
	// initalize the basic message parts
	//
	aKeyEvent.eventStructType = NS_KEY_EVENT;
	aKeyEvent.message 		= message;
	aKeyEvent.point.x			= 0;
	aKeyEvent.point.y			= 0;
	aKeyEvent.time				= PR_IntervalNow();
	
	//
	// initalize the GUI event parts
	//
	aKeyEvent.widget			= focusedWidget;
	aKeyEvent.nativeMsg		= (void*)&aOSEvent;
	
	//
	// nsInputEvent parts
	//
	aKeyEvent.isShift			= ((aOSEvent.modifiers & shiftKey) != 0);
	aKeyEvent.isControl		= ((aOSEvent.modifiers & controlKey) != 0);
	aKeyEvent.isAlt				= ((aOSEvent.modifiers & optionKey) != 0);
	aKeyEvent.isMeta		= ((aOSEvent.modifiers & cmdKey) != 0);
	
	//
	// nsKeyEvent parts
	//
	if (message == NS_KEY_PRESS 
	&& !IsSpecialRaptorKey((aOSEvent.message & keyCodeMask) >> 8) )
	{
	  if ( aKeyEvent.isControl )
	  {
	    aKeyEvent.charCode = (aOSEvent.message & charCodeMask);
	    if ( aKeyEvent.charCode <= 26 )
	    {
	      if ( aKeyEvent.isShift )
	        aKeyEvent.charCode += 'A' - 1;
	      else
	        aKeyEvent.charCode += 'a' - 1;
	    }
	  }
	  else
 	  if ( !aKeyEvent.isMeta)
    {
      aKeyEvent.isShift = aKeyEvent.isControl = aKeyEvent.isAlt = aKeyEvent.isMeta = 0;
    }
    
    aKeyEvent.keyCode	= 0;
    aKeyEvent.charCode = ConvertKeyEventToUnicode(aOSEvent);
	  NS_ASSERTION(0 != aKeyEvent.charCode, "nsMacEventHandler::InitializeKeyEvent: ConvertKeyEventToUnicode returned 0.");
	}
	else
	{
    aKeyEvent.keyCode = ConvertMacToRaptorKeyCode(aOSEvent.message, aOSEvent.modifiers);
    aKeyEvent.charCode = 0;
  }
}


//-------------------------------------------------------------------------
//
// IsSpecialRaptorKey
//
//-------------------------------------------------------------------------

PRBool nsMacEventHandler::IsSpecialRaptorKey(UInt32 macKeyCode)
{
	PRBool	isSpecial;

	// 
	// this table is used to determine which keys are special and should not generate a charCode
	//	
	switch (macKeyCode)
	{
// modifiers. We don't get separate events for these
// yet
		case kEscapeKeyCode:				isSpecial = PR_TRUE; break;
		case kShiftKeyCode:					isSpecial = PR_TRUE; break;
		case kCommandKeyCode:       isSpecial = PR_TRUE; break;
		case kCapsLockKeyCode:			isSpecial = PR_TRUE; break;
		case kControlKeyCode:				isSpecial = PR_TRUE; break;
		case kOptionkeyCode:				isSpecial = PR_TRUE; break;
		case kClearKeyCode:					isSpecial = PR_TRUE; break;

// function keys
		case kF1KeyCode:					isSpecial = PR_TRUE; break;
		case kF2KeyCode:					isSpecial = PR_TRUE; break;
		case kF3KeyCode:					isSpecial = PR_TRUE; break;
		case kF4KeyCode:					isSpecial = PR_TRUE; break;
		case kF5KeyCode:					isSpecial = PR_TRUE; break;
		case kF6KeyCode:					isSpecial = PR_TRUE; break;
		case kF7KeyCode:					isSpecial = PR_TRUE; break;
		case kF8KeyCode:					isSpecial = PR_TRUE; break;
		case kF9KeyCode:					isSpecial = PR_TRUE; break;
		case kF10KeyCode:					isSpecial = PR_TRUE; break;
		case kF11KeyCode:					isSpecial = PR_TRUE; break;
		case kF12KeyCode:					isSpecial = PR_TRUE; break;
		case kPauseKeyCode:				isSpecial = PR_TRUE; break;
		case kScrollLockKeyCode:	isSpecial = PR_TRUE; break;
		case kPrintScreenKeyCode:	isSpecial = PR_TRUE; break;

		case kInsertKeyCode:        isSpecial = PR_TRUE; break;
		case kDeleteKeyCode:				isSpecial = PR_TRUE; break;
		case kTabKeyCode:						isSpecial = PR_TRUE; break;
		case kBackspaceKeyCode:     isSpecial = PR_TRUE; break;

		case kHomeKeyCode:					isSpecial = PR_TRUE; break;	
		case kEndKeyCode:						isSpecial = PR_TRUE; break;
		case kPageUpKeyCode:				isSpecial = PR_TRUE; break;
		case kPageDownKeyCode:			isSpecial = PR_TRUE; break;
		case kLeftArrowKeyCode:			isSpecial = PR_TRUE; break;
		case kRightArrowKeyCode:		isSpecial = PR_TRUE; break;
		case kUpArrowKeyCode:				isSpecial = PR_TRUE; break;
		case kDownArrowKeyCode:			isSpecial = PR_TRUE; break;
		case kReturnKeyCode:        isSpecial = PR_TRUE; break;
		case kEnterKeyCode:         isSpecial = PR_TRUE; break;

		default:							isSpecial = PR_FALSE; break;
	}
	return isSpecial;
}


//-------------------------------------------------------------------------
//
// ConvertKeyEventToUnicode
//
//-------------------------------------------------------------------------
// we currently set the following to 5. We should fix this function later...
#define UNICODE_BUFFER_SIZE_FOR_KEY 5
PRUint32 nsMacEventHandler::ConvertKeyEventToUnicode(EventRecord& aOSEvent)
{
	char charResult = aOSEvent.message & charCodeMask;
	
	//
	// get the script of text for Unicode conversion
	//
	ScriptCode textScript = (ScriptCode)GetScriptManagerVariable(smKeyScript);

	//
	// convert our script code (smKeyScript) to a TextEncoding 
	//
	TextEncoding textEncodingFromScript;
	OSErr err = ::UpgradeScriptInfoToTextEncoding(textScript, kTextLanguageDontCare, kTextRegionDontCare, nsnull,
											&textEncodingFromScript);
	NS_ASSERTION(err == noErr, "nsMacEventHandler::ConvertKeyEventToUnicode: UpgradeScriptInfoToTextEncoding failed.");
	if (err != noErr) return 0;
	
	TextToUnicodeInfo	textToUnicodeInfo;
	err = ::CreateTextToUnicodeInfoByEncoding(textEncodingFromScript,&textToUnicodeInfo);
	NS_ASSERTION(err == noErr, "nsMacEventHandler::ConvertKeyEventToUnicode: CreateUnicodeToTextInfoByEncoding failed.");
	if (err != noErr) return 0;

	//
	// convert to Unicode
	//
	ByteCount result_size, source_read;
	PRUnichar unicharResult[UNICODE_BUFFER_SIZE_FOR_KEY];
	err = ::ConvertFromTextToUnicode(textToUnicodeInfo,
									sizeof(char),&charResult,
									kUnicodeLooseMappingsMask,
									0,NULL,NULL,NULL,
									sizeof(PRUnichar)*UNICODE_BUFFER_SIZE_FOR_KEY,&source_read,
									&result_size,unicharResult);
	::DisposeTextToUnicodeInfo(&textToUnicodeInfo);
	NS_ASSERTION(err == noErr, "nsMacEventHandler::ConvertKeyEventToUnicode: ConverFromTextToUnicode failed.");
	// if we got the following result, then it mean we got more than one Unichar from it.
	NS_ASSERTION(result_size == 2, "nsMacEventHandler::ConvertKeyEventToUnicode: ConverFromTextToUnicode failed.");
	// Fix Me!!!
	// the result_size will not equal to 2 in the following cases:
	//
	// 1. Hebrew/Arabic scripts, when we convert ( ) { } [ ]  < >  etc. 
	// The TEC will produce result_size = 6 (3 PRUnichar*) :
	// one Right-To-Left-Mark, the char and one Left-To-Right-Mark
	// We should fix this later...
	// See the following URL for the details...
	// ftp://ftp.unicode.org/Public/MAPPINGS/VENDORS/APPLE/ARABIC.TXT
	// ftp://ftp.unicode.org/Public/MAPPINGS/VENDORS/APPLE/FARSI.TXT
	// ftp://ftp.unicode.org/Public/MAPPINGS/VENDORS/APPLE/HEBREW.TXT
	//
	// 2. Also, it probably won't work in Thai and Indic keyboard since one char may convert to
	// several PRUnichar. It sometimes add zwj or zwnj. See the following url for details.
	// ftp://ftp.unicode.org/Public/MAPPINGS/VENDORS/APPLE/DEVANAGA.TXT
	// ftp://ftp.unicode.org/Public/MAPPINGS/VENDORS/APPLE/GUJARATI.TXT
	// ftp://ftp.unicode.org/Public/MAPPINGS/VENDORS/APPLE/GURMUKHI.TXT
	// ftp://ftp.unicode.org/Public/MAPPINGS/VENDORS/APPLE/THAI.TXT
	
//	if (err != noErr) return 0;
// I think we should ignore the above error since we already have the result we want

	return unicharResult[0];
}


//-------------------------------------------------------------------------
//
// HandleKeyEvent
//
//-------------------------------------------------------------------------

PRBool nsMacEventHandler::HandleKeyEvent(EventRecord& aOSEvent)
{
	nsresult result;

	// get the focused widget
	nsWindow* focusedWidget = gEventDispatchHandler.GetActive();
	if (!focusedWidget)
		focusedWidget = mTopLevelWidget;
	
	// nsEvent
	nsKeyEvent	keyEvent;
	switch (aOSEvent.what)
	{
		case keyUp:
			InitializeKeyEvent(keyEvent,aOSEvent,focusedWidget,NS_KEY_UP);
			result = focusedWidget->DispatchWindowEvent(keyEvent);
			break;

		case keyDown:	
			InitializeKeyEvent(keyEvent,aOSEvent,focusedWidget,NS_KEY_DOWN);
			result = focusedWidget->DispatchWindowEvent(keyEvent);
			//if (result == PR_FALSE) // continue processing???  talk to Tague about this (key event spec)
			{
				InitializeKeyEvent(keyEvent,aOSEvent,focusedWidget,NS_KEY_PRESS);
				result = focusedWidget->DispatchWindowEvent(keyEvent);
			}
			break;
		
		case autoKey:
			InitializeKeyEvent(keyEvent,aOSEvent,focusedWidget,NS_KEY_PRESS);
			result = focusedWidget->DispatchWindowEvent(keyEvent);
			break;
	}

	return result;
}

#pragma mark -
//-------------------------------------------------------------------------
//
// HandleActivateEvent
//
//-------------------------------------------------------------------------
PRBool nsMacEventHandler::HandleActivateEvent(EventRecord& aOSEvent)
{
	OSErr	err;
 Boolean isActive;

  switch (aOSEvent.what)
  {
    case activateEvt:
      isActive = ((aOSEvent.modifiers & activeFlag) != 0);
      break;

    case osEvt:
      isActive = ! mInBackground;
      break;
  }

	if (isActive)
	{
		//
		// Activate The TSMDocument associated with this handler
		//
		if (mTSMDocument)
			err = ::ActivateTSMDocument(mTSMDocument);
		else
			err = ::UseInputWindow(NULL, true); // get this line from mozilla-classic - mozilla/cmd/macfe/central/TSMProxy.cp
#ifdef DEBUG_TSM
#if 0
		NS_ASSERTION(err==noErr,"nsMacEventHandler::HandleActivateEvent: ActivateTSMDocument failed");
#endif
		printf("nsEventHandler::HandleActivateEvent: ActivateTSMDocument[%p] %s return %d\n",mTSMDocument,
		(err==noErr)?"":"ERROR", err);
#endif
		
		//�TODO: we should restore the focus to the the widget
		//       that had it when the window was deactivated
		nsWindow*	focusedWidget = mTopLevelWidget;
		gEventDispatchHandler.SetActivated(focusedWidget);
		
		// Twiddle menu bars
		nsIMenuBar* menuBar = focusedWidget->GetMenuBar();
		if (menuBar)
		{
		  menuBar->Paint();
		}
		else
		{
		  //�TODO:	if the focusedWidget doesn't have a menubar,
		  //				look all the way up to the window
		  //				until one of the parents has a menubar
		}
	}
	else
	{

		if (nsnull != gRollupListener && (nsnull != gRollupWidget) ) {
			if( mTopLevelWidget == gRollupWidget)
			gRollupListener->Rollup();
		}
		//
		// Deactivate the TSMDocument assoicated with this EventHandler
		//
		if (mTSMDocument)
			err = ::DeactivateTSMDocument(mTSMDocument);
#ifdef DEBUG_TSM
		NS_ASSERTION((noErr==err)||(tsmDocNotActiveErr==err),"nsMacEventHandler::HandleActivateEvent: DeactivateTSMDocument failed");
		printf("nsEventHandler::HandleActivateEvent: DeactivateTSMDocument[%p] %s return %d\n",mTSMDocument,
		(err==noErr)?"":"ERROR", err);
#endif
		// Dispatch an NS_DEACTIVATE event 
		gEventDispatchHandler.SetDeactivated(mTopLevelWidget);
	}
	return PR_TRUE;
}


//-------------------------------------------------------------------------
//
// HandleUpdateEvent
//
//-------------------------------------------------------------------------
PRBool nsMacEventHandler::HandleUpdateEvent(EventRecord& aOSEvent)
{
	mTopLevelWidget->HandleUpdateEvent();

	return PR_TRUE;
}


//-------------------------------------------------------------------------
//
// HandleMouseDownEvent
//
//-------------------------------------------------------------------------
PRBool nsMacEventHandler::HandleMouseDownEvent(
														EventRecord&		aOSEvent)
{
	PRBool retVal = PR_FALSE;

	WindowPtr		whichWindow;
	short partCode = ::FindWindow(aOSEvent.where, &whichWindow);

    if(whichWindow != ::FrontWindow()) {
    	if (nsnull != gRollupListener && (nsnull != gRollupWidget) ) {
			gRollupListener->Rollup();
		}
    }
 
	switch (partCode)
	{
		case inDrag:
		{
			Point macPoint;
#if TARGET_CARBON
			Rect portRect;
			::GetPortBounds(GetWindowPort(whichWindow), &portRect);
			macPoint = topLeft(portRect);
#else
			macPoint = topLeft(whichWindow->portRect);
#endif
			::LocalToGlobal(&macPoint);
			mTopLevelWidget->Move(macPoint.h, macPoint.v);
			if (nsnull != gRollupListener && (nsnull != gRollupWidget) ) {
				gRollupListener->Rollup();
			}
			break;
		}

		case inGrow:
		{
#if TARGET_CARBON
			Rect macRect;
			::GetWindowPortBounds ( whichWindow, &macRect );
#else
			Rect macRect = whichWindow->portRect;
#endif
			::LocalToGlobal(&topLeft(macRect));
			::LocalToGlobal(&botRight(macRect));
			mTopLevelWidget->Resize(macRect.right - macRect.left + 1, macRect.bottom - macRect.top + 1, PR_FALSE);
			if (nsnull != gRollupListener && (nsnull != gRollupWidget) ) {
				gRollupListener->Rollup();
			}
			break;
		}

		case inGoAway:
		{
			if (nsnull != gRollupListener && (nsnull != gRollupWidget) ) {
				gRollupListener->Rollup();
			}
			mTopLevelWidget->Destroy();
			break;
		}

		case inContent:
		{
			nsMouseEvent mouseEvent;
			ConvertOSEventToMouseEvent(aOSEvent, mouseEvent, NS_MOUSE_LEFT_BUTTON_DOWN);
			nsWindow* widgetHit = (nsWindow*)mouseEvent.widget;
			if (widgetHit)
			{
			
#ifdef NOTNOW
				if (nsnull != gRollupListener && (nsnull != gRollupWidget) ) {
					nsRect	widgetRect,newrect;
					Point macPoint = aOSEvent.where;			
						
						widgetHit->GetBounds(widgetRect);
						widgetHit->WidgetToScreen(widgetRect,newrect);
						if( macPoint.h < newrect.x ||  macPoint.h > newrect.XMost() ||  
								macPoint.v < newrect.y ||  macPoint.v > newrect.YMost() ) {
							gRollupListener->Rollup();
						}
				}
#endif
				// set the activation and focus on the widget hit, if it accepts it
				{
					nsMouseEvent mouseActivateEvent;
			        ConvertOSEventToMouseEvent(aOSEvent, mouseActivateEvent, NS_MOUSE_ACTIVATE);
					widgetHit->DispatchMouseEvent(mouseActivateEvent);
					if( mouseActivateEvent.acceptActivation )
						gEventDispatchHandler.SetFocus(widgetHit);
				}

				// dispatch the event
				retVal = widgetHit->DispatchMouseEvent(mouseEvent);
			} 
						
			gEventDispatchHandler.SetWidgetHit(widgetHit);
			mMouseInWidgetHit = PR_TRUE;
			break;
		}


		case inZoomIn:
		case inZoomOut:
		{
			// Now that we have found the partcode it is ok to actually zoom the window
			ZoomWindow(whichWindow, partCode, (whichWindow == FrontWindow()));
			
#if TARGET_CARBON
			Rect macRect;
			::GetWindowPortBounds(whichWindow, &macRect);
#else
			Rect macRect = whichWindow->portRect;
#endif
			::LocalToGlobal(&topLeft(macRect));
			::LocalToGlobal(&botRight(macRect));
			mTopLevelWidget->Resize(macRect.right - macRect.left, macRect.bottom - macRect.top, PR_FALSE);
			break;
		}
	}
	return retVal;
}


//-------------------------------------------------------------------------
//
// HandleMouseUpEvent
//
//-------------------------------------------------------------------------
PRBool nsMacEventHandler::HandleMouseUpEvent(
														EventRecord&		aOSEvent)
{
	PRBool retVal = PR_FALSE;

	nsMouseEvent mouseEvent;
	ConvertOSEventToMouseEvent(aOSEvent, mouseEvent, NS_MOUSE_LEFT_BUTTON_UP);

	nsWindow* widgetReleased = (nsWindow*)mouseEvent.widget;
	nsWindow* widgetHit = gEventDispatchHandler.GetWidgetHit();

	if ((widgetReleased != nsnull) && (widgetReleased != widgetHit))
		retVal |= widgetReleased->DispatchMouseEvent(mouseEvent);

	if (widgetHit)
	{
		gEventDispatchHandler.SetWidgetHit(nsnull);

		mouseEvent.widget = widgetHit;
		retVal |= widgetHit->DispatchMouseEvent(mouseEvent);
	}

	return retVal;
}


//-------------------------------------------------------------------------
//
// HandleMouseMoveEvent
//
//-------------------------------------------------------------------------
PRBool nsMacEventHandler::HandleMouseMoveEvent( EventRecord& aOSEvent )
{
	nsWindow* lastWidgetHit = gEventDispatchHandler.GetWidgetHit();
	nsWindow* lastWidgetPointed = gEventDispatchHandler.GetWidgetPointed();

	PRBool retVal = PR_FALSE;

	nsMouseEvent mouseEvent;
	ConvertOSEventToMouseEvent(aOSEvent, mouseEvent, NS_MOUSE_MOVE);

	if (lastWidgetHit)
	{
		Point macPoint = aOSEvent.where;
		::GlobalToLocal(&macPoint);
		PRBool inWidgetHit = lastWidgetHit->PointInWidget(macPoint);
		if (mMouseInWidgetHit != inWidgetHit)
		{
			mMouseInWidgetHit = inWidgetHit;
			mouseEvent.message = (inWidgetHit ? NS_MOUSE_ENTER : NS_MOUSE_EXIT);
		}
		retVal |= lastWidgetHit->DispatchMouseEvent(mouseEvent);
	}
	else
	{
		nsWindow* widgetPointed = (nsWindow*)mouseEvent.widget;
		if (widgetPointed != lastWidgetPointed)
		{
			if (lastWidgetPointed)
			{
				mouseEvent.widget = lastWidgetPointed;
				mouseEvent.message = NS_MOUSE_EXIT;
				retVal |= lastWidgetPointed->DispatchMouseEvent(mouseEvent);
			}

			gEventDispatchHandler.SetWidgetPointed(widgetPointed);

			if (widgetPointed)
			{
				mouseEvent.widget = widgetPointed;
				mouseEvent.message = NS_MOUSE_ENTER;
				retVal |= widgetPointed->DispatchMouseEvent(mouseEvent);
			}
		}
		else
		{
			if (widgetPointed)
				retVal |= widgetPointed->DispatchMouseEvent(mouseEvent);
		}
	}

	return retVal;
}


#pragma mark -
//-------------------------------------------------------------------------
//
// ConvertOSEventToMouseEvent
//
//-------------------------------------------------------------------------
void nsMacEventHandler::ConvertOSEventToMouseEvent(
														EventRecord&		aOSEvent,
														nsMouseEvent&		aMouseEvent,
														PRUint32				aMessage)
{
	static UInt32	sLastMouseUp = 0;
	static Point	sLastWhere = {0};
	static SInt16	sLastClickCount = 0;
	
	// we're going to time double-clicks from mouse *up* to next mouse *down*
	if (aMessage == NS_MOUSE_LEFT_BUTTON_UP)
	{
		// remember when this happened for the next mouse down
		sLastMouseUp = aOSEvent.when;
		sLastWhere = aOSEvent.where;
	}
	else if (aMessage == NS_MOUSE_LEFT_BUTTON_DOWN)
	{
		// now look to see if we want to convert this to a double- or triple-click
		const short kDoubleClickMoveThreshold	= 5;
		
		if (((aOSEvent.when - sLastMouseUp) < ::LMGetDoubleTime()) &&
				(((abs(aOSEvent.where.h - sLastWhere.h) < kDoubleClickMoveThreshold) &&
				 	(abs(aOSEvent.where.v - sLastWhere.v) < kDoubleClickMoveThreshold))))
		{		
			sLastClickCount ++;
			
//			if (sLastClickCount == 2)
//				aMessage = NS_MOUSE_LEFT_DOUBLECLICK;
		}
		else
		{
			// reset the click count, to count *this* click
			sLastClickCount = 1;
		}
	}

	// get the widget hit and the hit point inside that widget
	Point hitPoint = aOSEvent.where;
#if TARGET_CARBON
	GrafPtr grafPort = reinterpret_cast<GrafPtr>(mTopLevelWidget->GetNativeData(NS_NATIVE_GRAPHIC));
	::SetPort(grafPort);
	Rect savePortRect;
	::GetPortBounds(grafPort, &savePortRect);
#else
	GrafPtr grafPort = static_cast<GrafPort*>(mTopLevelWidget->GetNativeData(NS_NATIVE_GRAPHIC));
	::SetPort(grafPort);
	Rect savePortRect = grafPort->portRect;
#endif
	::SetOrigin(0, 0);
	::GlobalToLocal(&hitPoint);
	::SetOrigin(savePortRect.left, savePortRect.top);
	nsPoint widgetHitPoint(hitPoint.h, hitPoint.v);

	// if the mouse button is still down, send events to the last widget hit
	nsWindow* lastWidgetHit = gEventDispatchHandler.GetWidgetHit();
	nsWindow* widgetHit = nsnull;

	if (lastWidgetHit)
	{
 		if (::StillDown() || aMessage == NS_MOUSE_LEFT_BUTTON_UP)
	 		widgetHit = lastWidgetHit;
	 	else
	 	{
	 		// Some widgets can eat mouseUp events (text widgets in TEClick, sbars in TrackControl).
	 		// In that case, stop considering this widget as being still hit.
	 		gEventDispatchHandler.SetWidgetHit(nsnull);
	 	}
	}

	// if the mouse is in the grow box, pretend like it has left the window
	WindowPtr ignored = nsnull;
	short partCode = ::FindWindow ( aOSEvent.where, &ignored );
	if ( partCode != inGrow ) {
		if (! widgetHit)
			widgetHit = mTopLevelWidget->FindWidgetHit(hitPoint);
	
		if (widgetHit)
		{
			nsRect bounds;
			widgetHit->GetBounds(bounds);
			nsPoint widgetOrigin(bounds.x, bounds.y);
			widgetHit->LocalToWindowCoordinate(widgetOrigin);
			widgetHitPoint.MoveBy(-widgetOrigin.x, -widgetOrigin.y);
		}
	}
	
	// nsEvent
	aMouseEvent.eventStructType = NS_MOUSE_EVENT;
	aMouseEvent.message		= aMessage;
	aMouseEvent.point			= widgetHitPoint;
	aMouseEvent.time			= PR_IntervalNow();

	// nsGUIEvent
	aMouseEvent.widget		= widgetHit;
	aMouseEvent.nativeMsg	= (void*)&aOSEvent;

	// nsInputEvent
	aMouseEvent.isShift		= ((aOSEvent.modifiers & shiftKey) != 0);
	aMouseEvent.isControl	= ((aOSEvent.modifiers & controlKey) != 0);
	aMouseEvent.isAlt			= ((aOSEvent.modifiers & optionKey) != 0);
	aMouseEvent.isMeta	= ((aOSEvent.modifiers & cmdKey) != 0);

	// nsMouseEvent
	aMouseEvent.clickCount = sLastClickCount;
	aMouseEvent.acceptActivation = PR_TRUE;
}

//-------------------------------------------------------------------------
//
// HandlePositionToOffsetEvent
//
//-------------------------------------------------------------------------
long nsMacEventHandler::HandlePositionToOffset(Point aPoint,short* regionClass)
{
	*regionClass = kTSMOutsideOfBody;
	return 0;
}

//-------------------------------------------------------------------------
//
// HandleOffsetToPosition Event
//
//-------------------------------------------------------------------------
nsresult nsMacEventHandler::HandleOffsetToPosition(long offset,Point* thePoint)
{
	thePoint->v = mIMEPos.y;
	thePoint->h = mIMEPos.x;
	printf("local (x,y) = (%d, %d)\n", thePoint->h, thePoint->v);
#if TARGET_CARBON
	GrafPtr grafPort = reinterpret_cast<GrafPtr>(mTopLevelWidget->GetNativeData(NS_NATIVE_GRAPHIC));
	::SetPort(grafPort);
	Rect savePortRect;
	::GetPortBounds(grafPort, &savePortRect);
#else
	GrafPtr grafPort = static_cast<GrafPort*>(mTopLevelWidget->GetNativeData(NS_NATIVE_GRAPHIC));
	::SetPort(grafPort);
	Rect savePortRect = grafPort->portRect;
#endif
	::LocalToGlobal(thePoint);
	printf("global (x,y) = (%d, %d)\n", thePoint->h, thePoint->v);

	return PR_TRUE;
}

//-------------------------------------------------------------------------
//
// HandleUpdate Event
//
//-------------------------------------------------------------------------
// See ftp://ftp.unicode.org/Public/MAPPINGS/VENDORS/APPLE/CORPCHAR.TXT for detail of IS_APPLE_HINT_IN_PRIVATE_ZONE
#define IS_APPLE_HINT_IN_PRIVATE_ZONE(u) ((0xF850 <= (u)) && ((u)<=0xF883))
nsresult nsMacEventHandler::HandleUpdateInputArea(char* text,Size text_size, ScriptCode textScript,long fixedLength,TextRangeArray* textRangeList)
{
#ifdef DEBUG_TSM
	printf("********************************************************************************\n");
	printf("nsMacEventHandler::HandleUpdateInputArea size=%d fixlen=%d\n",text_size, fixedLength);
#endif
	TextToUnicodeInfo	textToUnicodeInfo;
	TextEncoding		textEncodingFromScript;
	int					i;
	OSErr				err;
	ByteCount			source_read;
	nsresult res = NS_OK;
	//====================================================================================================
	// 0. Create Unicode Converter
	//====================================================================================================

	//
	// convert our script code  to a TextEncoding 
	//
	err = ::UpgradeScriptInfoToTextEncoding(textScript,kTextLanguageDontCare,kTextRegionDontCare,nsnull,
											&textEncodingFromScript);
	NS_ASSERTION(err==noErr,"nsMacEventHandler::UpdateInputArea: UpgradeScriptInfoToTextEncoding failed.");
	if (err!=noErr) { 
		res = NS_ERROR_FAILURE;
		return res; 
	}
	
	err = ::CreateTextToUnicodeInfoByEncoding(textEncodingFromScript,&textToUnicodeInfo);
	NS_ASSERTION(err==noErr,"nsMacEventHandler::UpdateInputArea: CreateUnicodeToTextInfoByEncoding failed.");
	if (err!=noErr) { 
		res = NS_ERROR_FAILURE;
		return res; 
	}
	//------------------------------------------------------------------------------------------------
	// if we aren't in composition mode alredy, signal the backing store w/ the mode change
	//------------------------------------------------------------------------------------------------
	if (!mIMEIsComposing) {
		res = HandleStartComposition();
		NS_ASSERTION(NS_SUCCEEDED(res),"nsMacEventHandler::UpdateInputArea: HandleStartComposition failed.");
		if(NS_FAILED(res))
			goto error;
	}
	// mIMECompositionStr should be created in the HandleStartComposition
	NS_ASSERTION(mIMECompositionStr, "do not have mIMECompositionStr"); 
	if(nsnull == mIMECompositionStr)
	{
		res = NS_ERROR_OUT_OF_MEMORY;
		goto error;
	}
	// Prepare buffer....
	mIMECompositionStr->SetCapacity(text_size+1);
	PRUnichar* ubuf = (PRUnichar*)mIMECompositionStr->GetUnicode();
	size_t len;

	//====================================================================================================
	// Note- It is possible that the UnpdateInputArea event sent both committed text and uncommitted text
	// in the same time. The easies way to do that is using Korean input method w/ "Enter by Character" option
	//====================================================================================================
	//	1. Handle the committed text
	//====================================================================================================
	long committedLen = (fixedLength == -1) ? text_size : fixedLength;
	if(0 != committedLen)
	{
#ifdef DEBUG_TSM
		printf("Have commit text from 0 to %d\n",committedLen);
#endif
		//------------------------------------------------------------------------------------------------
		// 1.1 send textEvent to commit the text
		//------------------------------------------------------------------------------------------------
		len = 0;
		err = ::ConvertFromTextToUnicode(textToUnicodeInfo,committedLen,text,kUnicodeLooseMappingsMask,
						0,NULL,NULL,NULL,
						mIMECompositionStr->mCapacity *sizeof(PRUnichar),
						&source_read,&len,ubuf);
		NS_ASSERTION(err==noErr,"nsMacEventHandler::UpdateInputArea: ConvertFromTextToUnicode failed.\n");
		if (err!=noErr)
		{
			res = NS_ERROR_FAILURE;
			goto error; 
		}
		len /= sizeof(PRUnichar);
		// 1.2 Strip off the Apple Private U+F850-U+F87F ( source hint characters, transcodeing hints
		// Metric characters
		// See ftp://ftp.unicode.org/Public/MAPPINGS/VENDORS/APPLE/CORPCHAR.TXT for detail
		PRUint32 s,d;
		for(s=d=0;s<len;s++)
		{
			if(! IS_APPLE_HINT_IN_PRIVATE_ZONE(ubuf[s]))
				ubuf[d++] = ubuf[s];
		}
		len = d;
		ubuf[len] = '\0';		 // null terminate
		mIMECompositionStr->mLength = len;
		// for committed text, set no highlight ? (Do we need to set CaretPosition here ??? )
#ifdef DEBUG_TSM
			printf("1.2====================================\n");
#endif
		res = HandleTextEvent(0,nsnull);
		NS_ASSERTION(NS_SUCCEEDED(res),"nsMacEventHandler::UpdateInputArea: HandleTextEvent failed.");
		if(NS_FAILED(res)) 
			goto error; 
		//------------------------------------------------------------------------------------------------
		// 1.3 send compositionEvent to end the comosition
		//------------------------------------------------------------------------------------------------
		res = nsMacEventHandler::HandleEndComposition();
		NS_ASSERTION(NS_SUCCEEDED(res),"nsMacEventHandler::UpdateInputArea: HandleEndComposition failed.");
		if(NS_FAILED(res)) 
			goto error; 
	}  	//	1. Handle the committed text

	//====================================================================================================
	//	2. Handle the uncommitted text
	//====================================================================================================
	if((-1 != fixedLength) && (text_size != fixedLength ))
	{	
#ifdef DEBUG_TSM
		printf("Have new uncommited text from %d to text_size(%d)\n",committedLen,text_size);
#endif
		//------------------------------------------------------------------------------------------------
		// 2.1 send compositionEvent to start the comosition
		//------------------------------------------------------------------------------------------------
		//
		// if we aren't in composition mode alredy, signal the backing store w/ the mode change
		//	
		if (!mIMEIsComposing) {
			res = HandleStartComposition();
			NS_ASSERTION(NS_SUCCEEDED(res),"nsMacEventHandler::UpdateInputArea: HandleStartComposition failed.");
			if(NS_FAILED(res))
				goto error; 
		} 	// 2.1 send compositionEvent to start the comosition
		//------------------------------------------------------------------------------------------------
		// 2.2 send textEvent for the uncommitted text
		//------------------------------------------------------------------------------------------------
		//------------------------------------------------------------------------------------------------
		// 2.2.1 make sure we have one range array
		//------------------------------------------------------------------------------------------------

		TextRangeArray rawTextRangeArray;
		TextRangeArray *rangeArray;
		if(textRangeList && textRangeList->fNumOfRanges ) {
			rangeArray = textRangeList;
		} else {
			rangeArray = &rawTextRangeArray;
			rawTextRangeArray.fNumOfRanges = 1;
			rawTextRangeArray.fRange[0].fStart = committedLen;
			rawTextRangeArray.fRange[0].fEnd = text_size;
			rawTextRangeArray.fRange[0].fHiliteStyle = NS_TEXTRANGE_RAWINPUT;			
		}

		
#ifdef DEBUG_TSM
		printf("nsMacEventHandler::HandleUpdateInputArea textRangeList is %s\n", textRangeList ? "NOT NULL" : "NULL");
#endif
		nsTextRangeArray	xpTextRangeArray  = new nsTextRange[rangeArray->fNumOfRanges];
		NS_ASSERTION(xpTextRangeArray!=NULL,"nsMacEventHandler::UpdateInputArea: xpTextRangeArray memory allocation failed.");
		if (xpTextRangeArray==NULL)
		{
			res = NS_ERROR_OUT_OF_MEMORY;
			goto error; 
		}
	
		//------------------------------------------------------------------------------------------------
		// 2.2.2 convert range array into our xp range array
		//------------------------------------------------------------------------------------------------
		//
		// the TEC offset mapping capabilities won't work here because you need to have unique, ordered offsets
		//  so instead we iterate over the range list and map each range individually.  it's probably faster than
		//  trying to do collapse all the ranges into a single offset list
		//
		for(i=0;i<rangeArray->fNumOfRanges;i++) {			
			ByteOffset			sourceOffset[2], destinationOffset[2];
			ItemCount			destinationLength;
			// 2.2.2.1 check each range item in NS_ASSERTION
			NS_ASSERTION(
				(NS_TEXTRANGE_CARETPOSITION==rangeArray->fRange[i].fHiliteStyle)||
				(NS_TEXTRANGE_RAWINPUT==rangeArray->fRange[i].fHiliteStyle)||
				(NS_TEXTRANGE_SELECTEDRAWTEXT==rangeArray->fRange[i].fHiliteStyle)||
				(NS_TEXTRANGE_CONVERTEDTEXT==rangeArray->fRange[i].fHiliteStyle)||
				(NS_TEXTRANGE_SELECTEDCONVERTEDTEXT==rangeArray->fRange[i].fHiliteStyle),
				"illegal range type");
			NS_ASSERTION( rangeArray->fRange[i].fStart <= text_size,"illegal range");
			NS_ASSERTION( rangeArray->fRange[i].fEnd <= text_size,"illegal range");

#ifdef DEBUG_TSM
			printf("nsMacEventHandler::HandleUpdateInputArea textRangeList[%d] = (%d,%d) text_size = %d\n",i,
				rangeArray->fRange[i].fStart, rangeArray->fRange[i].fEnd, text_size);
#endif			
			// 2.2.2.2 fill sourceOffset array
			typedef enum {
				kEqualToDest0,
				kEqualToDest1,
				kEqualToLength
			} rangePairType;
			rangePairType tpStart,tpEnd;
			
			if(rangeArray->fRange[i].fStart < text_size) {
				sourceOffset[0] = rangeArray->fRange[i].fStart-committedLen;
				tpStart = kEqualToDest0;
				destinationLength = 1;
				if(rangeArray->fRange[i].fStart == rangeArray->fRange[i].fEnd) {
					tpEnd = kEqualToDest0;
				} else if(rangeArray->fRange[i].fEnd < text_size) {
					sourceOffset[1] = rangeArray->fRange[i].fEnd-committedLen;
					tpEnd = kEqualToDest1;
					destinationLength++;
				} else { 
					// fEnd >= text_size
					tpEnd = kEqualToLength;
				}
			} else { 
				// fStart >= text_size
				tpStart = kEqualToLength;
				tpEnd = kEqualToLength;
				destinationLength = 0;
			} // if(rangeArray->fRange[i].fStart < text_size) 
			
			// 2.2.2.3 call unicode converter to convert the sourceOffset into destinationOffset
			len = 0;
			// Note : The TEC will return -50 if sourceOffset[0,1] >= text_size-committedLen
			err = ::ConvertFromTextToUnicode(textToUnicodeInfo,text_size-committedLen,text+committedLen,kUnicodeLooseMappingsMask,
							destinationLength,sourceOffset,&destinationLength,destinationOffset,
							mIMECompositionStr->mCapacity *sizeof(PRUnichar),
							&source_read,&len, ubuf);
			NS_ASSERTION(err==noErr,"nsMacEventHandler::UpdateInputArea: ConvertFromTextToUnicode failed.\n");
			if (err!=noErr) 
			{
				res = NS_ERROR_FAILURE;
				goto error; 
			}
			// 2.2.2.4 Convert len, destinationOffset[0,1] into the unicode of PRUnichar.
			len /= sizeof(PRUnichar);
			if(destinationLength > 0 ){
				destinationOffset[0] /= sizeof(PRUnichar);
				if(destinationLength > 1 ) {
					destinationOffset[1] /= sizeof(PRUnichar);
				}
			}
			// 2.2.2.5 Strip off the Apple Private U+F850-U+F87F ( source hint characters, transcodeing hints
			// Metric characters
			// See ftp://ftp.unicode.org/Public/MAPPINGS/VENDORS/APPLE/CORPCHAR.TXT for detail
			// If we don't do this, Trad Chinese input method won't handle ',' correctly
			PRUint32 s,d;
			for(s=d=0;s<len;s++)
			{
				if(! IS_APPLE_HINT_IN_PRIVATE_ZONE(ubuf[s]))
				{
					ubuf[d++] = ubuf[s];
				}
				else 
				{
					if(destinationLength > 0 ){
						if(destinationOffset[0] >= s) {
							destinationOffset[0]--;
						}
						if(destinationLength > 1 ) {
							if(destinationOffset[1] >= s) {
								destinationOffset[1]--;
							}
						}
					}	
				}
			}
			len = d;
						 
			// 2.2.2.6 put destinationOffset into xpTextRangeArray[i].mStartOffset 
			xpTextRangeArray[i].mRangeType = rangeArray->fRange[i].fHiliteStyle;
			switch(tpStart) {
				case kEqualToDest0:
					xpTextRangeArray[i].mStartOffset = destinationOffset[0];
				break;
				case kEqualToLength:
					xpTextRangeArray[i].mStartOffset = len;
				break;
				case kEqualToDest1:
				default:
					NS_ASSERTION(PR_FALSE, "tpStart is wrong");
				break;
			}
			switch(tpEnd) {
				case kEqualToDest0:
					xpTextRangeArray[i].mEndOffset = destinationOffset[0];
				break;
				case kEqualToDest1:
					xpTextRangeArray[i].mEndOffset = destinationOffset[1];
				break;
				case kEqualToLength:
					xpTextRangeArray[i].mEndOffset = len;
				break;
				default:
					NS_ASSERTION(PR_FALSE, "tpEnd is wrong");
				break;
			}
			// 2.2.2.7 Check the converted result in NS_ASSERTION
			NS_ASSERTION(xpTextRangeArray[i].mStartOffset <= len,"illegal range");
			NS_ASSERTION(xpTextRangeArray[i].mEndOffset <= len,"illegal range");
#ifdef DEBUG_TSM
			printf("nsMacEventHandler::HandleUpdateInputArea textRangeList[%d] => type=%d (%d,%d)\n",i,
				xpTextRangeArray[i].mRangeType,
				xpTextRangeArray[i].mStartOffset, xpTextRangeArray[i].mEndOffset);
#endif			

			NS_ASSERTION((NS_TEXTRANGE_CARETPOSITION!=xpTextRangeArray[i].mRangeType) ||
						 (xpTextRangeArray[i].mStartOffset == xpTextRangeArray[i].mEndOffset),
						 "start != end in CaretPosition");
			
		}
		//------------------------------------------------------------------------------------------------
		// 2.2.3 null terminate the uncommitted text
		//------------------------------------------------------------------------------------------------
		ubuf[len] = '\0';		 // null terminate // we convert the text in 2.2.2 ...
		mIMECompositionStr->mLength = len;			
		//------------------------------------------------------------------------------------------------
		// 2.2.4 send the text event
		//------------------------------------------------------------------------------------------------
#ifdef DEBUG_TSM
			printf("2.2.4====================================\n");
#endif			

		res = HandleTextEvent(rangeArray->fNumOfRanges,xpTextRangeArray);
		NS_ASSERTION(NS_SUCCEEDED(res), "nsMacEventHandler::UpdateInputArea: HandleTextEvent failed.");
		if(NS_FAILED(res)) 
			goto error; 
		if(xpTextRangeArray) 
			delete [] xpTextRangeArray;
	} //	2. Handle the uncommitted text
	else if((0==text_size) && (0==fixedLength))
	{
		// 3. Handle empty text event
		// This is needed when we input some uncommitted text, and then delete all of them
		// When the last delete come, we will got a text_size = 0 and fixedLength = 0
		// In that case, we need to send a text event to clean un the input hole....
		ubuf[0] = '\0';		 // null terminate
		mIMECompositionStr->mLength = 0;			
#ifdef DEBUG_TSM
			printf("3.====================================\n");
#endif
		// 3.1 send the empty text event.
		res = HandleTextEvent(0,nsnull);
		NS_ASSERTION(NS_SUCCEEDED(res),"nsMacEventHandler::UpdateInputArea: HandleTextEvent failed.");
		if(NS_FAILED(res)) 
			goto error; 
		// 3.2 send an endComposition event, we need this to make sure the delete after this work properly.
		res = nsMacEventHandler::HandleEndComposition();
		NS_ASSERTION(NS_SUCCEEDED(res),"nsMacEventHandler::UpdateInputArea: HandleEndComposition failed.");
		if(NS_FAILED(res)) 
			goto error; 		
	}
	return res;
error:
	::DisposeTextToUnicodeInfo(&textToUnicodeInfo); 
	return res; 
}

//-------------------------------------------------------------------------
//
// HandleStartComposition
//
//-------------------------------------------------------------------------
nsresult nsMacEventHandler::HandleStartComposition(void)
{
#ifdef DEBUG_TSM
	printf("HandleStartComposition\n");
#endif
	mIMEIsComposing = PR_TRUE;
	if(nsnull == mIMECompositionStr)
		mIMECompositionStr = new nsAutoString();
	NS_ASSERTION(mIMECompositionStr, "cannot allocate mIMECompositionStr");
	if(nsnull == mIMECompositionStr)
	{
		return NS_ERROR_OUT_OF_MEMORY;
	}
	// 
	// get the focused widget [tague: may need to rethink this later]
	//
	nsWindow* focusedWidget = gEventDispatchHandler.GetActive();
	if (!focusedWidget)
		focusedWidget = mTopLevelWidget;
	
	//
	// create the nsCompositionEvent
	//
	nsCompositionEvent		compositionEvent;

	compositionEvent.eventStructType = NS_COMPOSITION_START;
	compositionEvent.message = NS_COMPOSITION_START;
	compositionEvent.point.x = 0;
	compositionEvent.point.y = 0;
	compositionEvent.time = PR_IntervalNow();

	//
	// nsGUIEvent parts
	//
	compositionEvent.widget	= focusedWidget;
	compositionEvent.nativeMsg	= (void*)nsnull;		// no native message for this

	nsresult res = focusedWidget->DispatchWindowEvent(compositionEvent);
	if(NS_SUCCEEDED(res)) {
		mIMEPos = compositionEvent.theReply.mCursorPosition;
		focusedWidget->LocalToWindowCoordinate(mIMEPos);
#ifdef DEBUG_TSM
		printf("HandleStartComposition reply (%d,%d)\n", mIMEPos.x , mIMEPos.y);
#endif
	}
	return res;
}

//-------------------------------------------------------------------------
//
// HandleEndComposition
//
//-------------------------------------------------------------------------
nsresult nsMacEventHandler::HandleEndComposition(void)
{
#ifdef DEBUG_TSM
	printf("HandleEndComposition\n");
#endif
	mIMEIsComposing = PR_FALSE;
	// 
	// get the focused widget [tague: may need to rethink this later]
	//
	nsWindow* focusedWidget = gEventDispatchHandler.GetActive();
	if (!focusedWidget)
		focusedWidget = mTopLevelWidget;
	
	//
	// create the nsCompositionEvent
	//
	nsCompositionEvent		compositionEvent;

	compositionEvent.eventStructType = NS_COMPOSITION_END;
	compositionEvent.message = NS_COMPOSITION_END;
	compositionEvent.point.x = 0;
	compositionEvent.point.y = 0;
	compositionEvent.time = PR_IntervalNow();

	//
	// nsGUIEvent parts
	//
	compositionEvent.widget	= focusedWidget;
	compositionEvent.nativeMsg	= (void*)nsnull;		// no native message for this
	return(focusedWidget->DispatchWindowEvent(compositionEvent));
}

//-------------------------------------------------------------------------
//
// HandleTextEvent
//
//-------------------------------------------------------------------------
nsresult nsMacEventHandler::HandleTextEvent(PRUint32 textRangeCount, nsTextRangeArray textRangeArray)
{
#ifdef DEBUG_TSM
	printf("HandleTextEvent\n");
	PRUint32 i;
	printf("text event \n[");
	const PRUnichar *ubuf = mIMECompositionStr->GetUnicode();
	for(i=0; '\0' != *ubuf; i++)
		printf("U+%04X ", *ubuf++);
	printf("] len = %d\n",i);
	if(textRangeCount > 0)
	{
		for(i=0;i<textRangeCount;i++ )
		{
			NS_ASSERTION((NS_TEXTRANGE_CARETPOSITION!=textRangeArray[i].mRangeType) ||
						 (textRangeArray[i].mStartOffset == textRangeArray[i].mEndOffset),
						 "start != end in CaretPosition");
			NS_ASSERTION(
				(NS_TEXTRANGE_CARETPOSITION==textRangeArray[i].mRangeType)||
				(NS_TEXTRANGE_RAWINPUT==textRangeArray[i].mRangeType)||
				(NS_TEXTRANGE_SELECTEDRAWTEXT==textRangeArray[i].mRangeType)||
				(NS_TEXTRANGE_CONVERTEDTEXT==textRangeArray[i].mRangeType)||
				(NS_TEXTRANGE_SELECTEDCONVERTEDTEXT==textRangeArray[i].mRangeType),
				"illegal range type");
			static char *name[6] =
			{
				"Unknown",
				"CaretPosition", 
				"RawInput", 
				"SelectedRawText",
				"ConvertedText",
				"SelectedConvertedText"
			};
			printf("[%d,%d]=%s\n", 
			textRangeArray[i].mStartOffset, 
			textRangeArray[i].mEndOffset,
			((textRangeArray[i].mRangeType<=NS_TEXTRANGE_SELECTEDCONVERTEDTEXT) ?
				name[textRangeArray[i].mRangeType] : name[0])
			);
		}
	}
#endif
	// 
	// get the focused widget [tague: may need to rethink this later]
	//
	nsWindow* focusedWidget = gEventDispatchHandler.GetActive();
	if (!focusedWidget)
		focusedWidget = mTopLevelWidget;
	
	//
	// create the nsCompositionEvent
	//
	nsTextEvent		textEvent;

	textEvent.eventStructType = NS_TEXT_EVENT;
	textEvent.message = NS_TEXT_EVENT;
	textEvent.point.x = 0;
	textEvent.point.y = 0;
	textEvent.time = PR_IntervalNow();
	textEvent.theText = (PRUnichar*)mIMECompositionStr->GetUnicode();
	textEvent.rangeCount = textRangeCount;
	textEvent.rangeArray = textRangeArray;

	//
	// nsGUIEvent parts
	//
	textEvent.widget	= focusedWidget;
	textEvent.nativeMsg	= (void*)nsnull;		// no native message for this

	nsresult res = NS_OK;
	if (NS_SUCCEEDED(res = focusedWidget->DispatchWindowEvent(textEvent))) {
		mIMEPos = textEvent.theReply.mCursorPosition;
		focusedWidget->LocalToWindowCoordinate(mIMEPos);
#ifdef DEBUG_TSM
		printf("HandleTextEvent reply (%d,%d)\n", mIMEPos.x , mIMEPos.y);
#endif
	} 
	return res;
}
