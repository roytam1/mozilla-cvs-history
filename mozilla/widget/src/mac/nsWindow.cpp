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


// ***IMPORTANT***
// On all platforms, we are assuming in places that the implementation of |nsIWidget|
// is really |nsWindow| and then calling methods specific to nsWindow to finish the job.
// This is by design and the assumption is safe because an nsIWidget can only be created through
// our Widget factory where all our widgets, including the XP widgets, inherit from nsWindow.
// Still, in the places (or most of them) where this assumption is done, a |static_cast| has been used.
// A similar warning is in nsWidgetFactory.cpp.


#include "nsWindow.h"
#include "nsIFontMetrics.h"
#include "nsIDeviceContext.h"
#include "nsCOMPtr.h"
#include "nsToolkit.h"
#include "nsIEnumerator.h"

#include <Appearance.h>
#include <Timer.h>

#include "nsplugindefs.h"
#include "nsMacEventHandler.h"
#include "nsMacResources.h"
#include "nsRegionMac.h"
#include "nsIRollupListener.h"

#include "nsCarbonHelpers.h"


////////////////////////////////////////////////////
nsIRollupListener * gRollupListener = nsnull;
nsIWidget         * gRollupWidget   = nsnull;

// Since we only want a single notification pending for the app we'll declare
// these static
static NMRec	gNMRec;
static Boolean	gNotificationInstalled = false;

#pragma mark -

// #define PAINT_DEBUGGING
// #define BLINK_DEBUGGING

#ifdef BLINK_DEBUGGING
static void blinkRect(Rect* r);
static void blinkRgn(RgnHandle rgn);
#endif


//-------------------------------------------------------------------------
//
// nsWindow constructor
//
//-------------------------------------------------------------------------
nsWindow::nsWindow() : nsBaseWidget() , nsDeleteObserved(this), nsIKBStateControl()
{
  gInstanceClassName = "nsWindow";

  mParent = nsnull;
  mBounds.SetRect(0,0,0,0);

  mResizingChildren = PR_FALSE;
  mVisible = PR_FALSE;
  mEnabled = PR_TRUE;
  SetPreferredSize(0,0);

  mFontMetrics = nsnull;
  mMenuBar = nsnull;
  mTempRenderingContext = nsnull;

  mWindowRegion = nsnull;
  mVisRegion = nsnull;
  mWindowPtr = nsnull;
  mDrawing = PR_FALSE;
  mDestroyCalled = PR_FALSE;
  mDestructorCalled = PR_FALSE;

  SetBackgroundColor(NS_RGB(255, 255, 255));
  SetForegroundColor(NS_RGB(0, 0, 0));

  mPluginPort = nsnull;

  AcceptFocusOnClick(PR_TRUE);
}


//-------------------------------------------------------------------------
//
// nsWindow destructor
//
//-------------------------------------------------------------------------
nsWindow::~nsWindow()
{
	// notify the children that we're gone
	nsCOMPtr<nsIEnumerator> children ( getter_AddRefs(GetChildren()) );
	if (children)
	{
		children->First();
		do
		{
			nsISupports* child;
			if (NS_SUCCEEDED(children->CurrentItem(&child)))
			{
				nsWindow* childWindow = static_cast<nsWindow*>(static_cast<nsIWidget*>(child));
				NS_RELEASE(child);

				childWindow->mParent = nsnull;
    	}
		} while (NS_SUCCEEDED(children->Next()));			
	}

	mDestructorCalled = PR_TRUE;

	//Destroy();

	if (mWindowRegion)
	{
		::DisposeRgn(mWindowRegion);
		mWindowRegion = nsnull;	
	}

	if (mVisRegion)
	{
		::DisposeRgn(mVisRegion);
		mVisRegion = nsnull;	
	}
			
	NS_IF_RELEASE(mTempRenderingContext);
	
	NS_IF_RELEASE(mFontMetrics);
	NS_IF_RELEASE(mMenuBar);
	NS_IF_RELEASE(mMenuListener);
	
	if (mPluginPort) {
		delete mPluginPort;
	}
}

NS_IMPL_ADDREF(nsWindow);
NS_IMPL_RELEASE(nsWindow);
nsresult nsWindow::QueryInterface(const nsIID& aIID, void** aInstancePtr)
{
	if (NULL == aInstancePtr) {
	    return NS_ERROR_NULL_POINTER;
	}

	if (aIID.Equals(nsIKBStateControl::GetIID())) {
	    *aInstancePtr = (void*) ((nsIKBStateControl*)this);
	    AddRef();
	    return NS_OK;
	}

	return nsBaseWidget::QueryInterface(aIID,aInstancePtr);
}

//-------------------------------------------------------------------------
//
// Utility method for implementing both Create(nsIWidget ...) and
// Create(nsNativeWidget...)
//-------------------------------------------------------------------------

nsresult nsWindow::StandardCreate(nsIWidget *aParent,
                      const nsRect &aRect,
                      EVENT_CALLBACK aHandleEventFunction,
                      nsIDeviceContext *aContext,
                      nsIAppShell *aAppShell,
                      nsIToolkit *aToolkit,
                      nsWidgetInitData *aInitData,
                      nsNativeWidget aNativeParent)	// should always be nil here
{
	mParent = aParent;
	mBounds = aRect;
	CalcWindowRegions();

	BaseCreate(aParent, aRect, aHandleEventFunction, 
							aContext, aAppShell, aToolkit, aInitData);

	if (mParent)
	{
		SetBackgroundColor(mParent->GetBackgroundColor());
		SetForegroundColor(mParent->GetForegroundColor());
	}

	if (mWindowPtr == nsnull) {
		if (aParent)
			mWindowPtr = (WindowPtr)aParent->GetNativeData(NS_NATIVE_DISPLAY);
/* this is always null
		else if (aAppShell)
			mWindowPtr = (WindowPtr)aAppShell->GetNativeData(NS_NATIVE_SHELL);
*/
	}
	return NS_OK;
}

//-------------------------------------------------------------------------
//
// create a nswindow
//
//-------------------------------------------------------------------------
NS_IMETHODIMP nsWindow::Create(nsIWidget *aParent,
                      const nsRect &aRect,
                      EVENT_CALLBACK aHandleEventFunction,
                      nsIDeviceContext *aContext,
                      nsIAppShell *aAppShell,
                      nsIToolkit *aToolkit,
                      nsWidgetInitData *aInitData)
{	 
	return(StandardCreate(aParent, aRect, aHandleEventFunction,
													aContext, aAppShell, aToolkit, aInitData,
														nsnull));
}

//-------------------------------------------------------------------------
//
// Creates a main nsWindow using a native widget
//
//-------------------------------------------------------------------------
NS_IMETHODIMP nsWindow::Create(nsNativeWidget aNativeParent,		// this is a nsWindow*
                      const nsRect &aRect,
                      EVENT_CALLBACK aHandleEventFunction,
                      nsIDeviceContext *aContext,
                      nsIAppShell *aAppShell,
                      nsIToolkit *aToolkit,
                      nsWidgetInitData *aInitData)
{
	// On Mac, a native widget is a nsWindow* because 
	// nsWindow::GetNativeData(NS_NATIVE_WIDGET) returns 'this'
	nsIWidget* aParent = (nsIWidget*)aNativeParent;
	
	return(Create(aParent, aRect, aHandleEventFunction,
									aContext, aAppShell, aToolkit, aInitData));
}

//-------------------------------------------------------------------------
//
// Close this nsWindow
//
//-------------------------------------------------------------------------
NS_IMETHODIMP nsWindow::Destroy()
{
	if (mDestroyCalled)
		return NS_OK;
	mDestroyCalled = PR_TRUE;

	nsBaseWidget::OnDestroy();
	nsBaseWidget::Destroy();
	
	NS_IF_RELEASE(mMenuBar);
	SetMenuBar(nsnull);

	ReportDestroyEvent();	// beard: this seems to cause the window to be deleted. moved all release code to destructor.

	return NS_OK;
}

#pragma mark -
//-------------------------------------------------------------------------
//
// Get this nsWindow parent
//
//-------------------------------------------------------------------------
nsIWidget* nsWindow::GetParent(void)
{
  NS_IF_ADDREF(mParent);
  return  mParent;
}

//-------------------------------------------------------------------------
//
// Return some native data according to aDataType
//
//-------------------------------------------------------------------------
void* nsWindow::GetNativeData(PRUint32 aDataType)
{
	nsPoint		point;
	void*		retVal = nsnull;

  switch (aDataType) 
	{
	case NS_NATIVE_WIDGET:
    case NS_NATIVE_WINDOW:
    	retVal = (void*)this;
    	break;

    case NS_NATIVE_GRAPHIC:
    // pinkerton
    // Windows and GrafPorts are VERY different under Carbon, and we can no
    // longer pass them interchagably. When we ask for a GrafPort, we cannot
    // return a window or vice versa.
      retVal = (void*)::GetWindowPort(mWindowPtr);
      break;
      
    case NS_NATIVE_DISPLAY:
      retVal = (void*)mWindowPtr;
    	break;

    case NS_NATIVE_REGION:
		retVal = (void*)mVisRegion;
    	break;

    case NS_NATIVE_COLORMAP:
    	//�TODO
    	break;

    case NS_NATIVE_OFFSETX:
    	point.MoveTo(mBounds.x, mBounds.y);
    	LocalToWindowCoordinate(point);
    	retVal = (void*)point.x;
     	break;

    case NS_NATIVE_OFFSETY:
    	point.MoveTo(mBounds.x, mBounds.y);
    	LocalToWindowCoordinate(point);
    	retVal = (void*)point.y;
    	break;
    
    case NS_NATIVE_PLUGIN_PORT:
    	// this needs to be a combination of the port and the offsets.
    	if (mPluginPort == nsnull)
    		mPluginPort = new nsPluginPort;
    		
		point.MoveTo(mBounds.x, mBounds.y);
		LocalToWindowCoordinate(point);

		// for compatibility with 4.X, this origin is what you'd pass
		// to SetOrigin.
		mPluginPort->port = ::GetWindowPort(mWindowPtr);
		mPluginPort->portx = -point.x;
		mPluginPort->porty = -point.y;
		
    	retVal = (void*)mPluginPort;
	}

  return retVal;
}

#pragma mark -
//-------------------------------------------------------------------------
//
// Return PR_TRUE if the whether the component is visible, PR_FALSE otherwise
//
//-------------------------------------------------------------------------
NS_METHOD nsWindow::IsVisible(PRBool & bState)
{
  bState = mVisible;
  return NS_OK;
}

//-------------------------------------------------------------------------
//
// Hide or show this component
//
//-------------------------------------------------------------------------
NS_IMETHODIMP nsWindow::Show(PRBool bState)
{
  mVisible = bState;
  return NS_OK;
}

    
NS_IMETHODIMP nsWindow::ModalEventFilter(PRBool aRealEvent, void *aEvent,
                                         PRBool *aForWindow)
{
	*aForWindow = PR_FALSE;
	EventRecord *theEvent = (EventRecord *) aEvent;

	if (aRealEvent && theEvent->what != nullEvent ) {

		WindowPtr window = (WindowPtr) GetNativeData(NS_NATIVE_DISPLAY),
		          rollupWindow = gRollupWidget ? (WindowPtr) gRollupWidget->GetNativeData(NS_NATIVE_DISPLAY) : 0;
		WindowPtr eventWindow = nsnull;
		PRInt16 where = ::FindWindow ( theEvent->where, &eventWindow );
		bool inWindow = eventWindow && (eventWindow == window || eventWindow == rollupWindow);

		switch ( theEvent->what ) {
			// is it a mouse event?
			case mouseDown:
			case mouseUp:
				// is it in the given window?
				// (note we also let some events questionable for modal dialogs pass through.
				// but it makes sense that the draggability et.al. of a modal window should
				// be controlled by whether the window has a drag bar).
				if ( inWindow &&
				     ( where == inContent || where == inDrag || where == inGrow ||
				       where == inGoAway || where == inZoomIn || where == inZoomOut ))
					*aForWindow = PR_TRUE;
				break;
			case keyDown:
			case keyUp:
			case autoKey:
				if ( inWindow )
					*aForWindow = PR_TRUE;
				break;

			case diskEvt:
			    // I think dialogs might want to support floppy insertion, and it doesn't
			    // interfere with modality...
			case updateEvt:
				// always let update events through, because if we don't handle them, we're
				// doomed!
			case activateEvt:
				// certainly we have to let the obvious activate events through. hopefully
				// our consumption of other events will keep any unwanted activate events
				// from even getting this far
				*aForWindow = PR_TRUE;
				break;

			case osEvt:
				// check for mouseMoved or suspend/resume events. We especially need to
				// let suspend/resume events through in order to make sure the clipboard is
				// converted correctly.
				unsigned char eventType = (theEvent->message >> 24) & 0x00ff;
				if (eventType == mouseMovedMessage) {
					// I'm guessing we don't want to let these through unless the mouse is
					// in the modal dialog so we don't see rollover feedback in windows behind
					// the dialog.
					if ( where == inContent && inWindow )
						*aForWindow = PR_TRUE;
				}
				if ( eventType == suspendResumeMessage )
					*aForWindow = PR_TRUE;
				break;
		} // case of which event type
	} else
		*aForWindow = PR_TRUE;

	return NS_OK;
}

//-------------------------------------------------------------------------
//
// Enable/disable this component
//
//-------------------------------------------------------------------------
NS_IMETHODIMP nsWindow::Enable(PRBool bState)
{
	mEnabled = bState;
	return NS_OK;
}

    
static Boolean we_are_front_process()
{
	ProcessSerialNumber	thisPSN;
	ProcessSerialNumber	frontPSN;
	(void)::GetCurrentProcess(&thisPSN);
	if (::GetFrontProcess(&frontPSN) == noErr)
	{
		if ((frontPSN.highLongOfPSN == thisPSN.highLongOfPSN) &&
			(frontPSN.lowLongOfPSN == thisPSN.lowLongOfPSN))
			return true;
	}
	return false;
}

//-------------------------------------------------------------------------
//
// Set the focus on this component
//
//-------------------------------------------------------------------------
NS_IMETHODIMP nsWindow::SetFocus(void)
{
	gEventDispatchHandler.SetFocus(this);
	
	// Here's where we see if there's a notification we need to remove
	if (gNotificationInstalled && we_are_front_process())
	{
		(void)::NMRemove(&gNMRec);
		gNotificationInstalled = false;
	}
	
	return NS_OK;
}

//-------------------------------------------------------------------------
//
// Get this component font
//
//-------------------------------------------------------------------------
nsIFontMetrics* nsWindow::GetFont(void)
{
	return mFontMetrics;
}

    
//-------------------------------------------------------------------------
//
// Set this component font
//
//-------------------------------------------------------------------------
NS_IMETHODIMP nsWindow::SetFont(const nsFont &aFont)
{
	NS_IF_RELEASE(mFontMetrics);
	if (mContext)
		mContext->GetMetricsFor(aFont, mFontMetrics);
 	return NS_OK;
}


//-------------------------------------------------------------------------
//
// Set the colormap of the window
//
//-------------------------------------------------------------------------
NS_IMETHODIMP nsWindow::SetColorMap(nsColorMap *aColorMap)
{
	//�TODO
	// We may need to move this to nsMacWindow:
	// I'm not sure all the individual widgets
	// can have each their own colorMap on Mac.
	return NS_OK;
}

//-------------------------------------------------------------------------
//
// Set the widget's MenuBar.
// Must be called after Create.
// Releases a previously set nsIMenuBar
// AddRefs the passed in nsIMenuBar
// @param aMenuBar a pointer to an nsIMenuBar interface on an object
//
//-------------------------------------------------------------------------
NS_IMETHODIMP nsWindow::SetMenuBar(nsIMenuBar * aMenuBar)
{
  if (mMenuBar)
    mMenuBar->SetParent(nsnull);
  NS_IF_RELEASE(mMenuBar);
  NS_IF_ADDREF(aMenuBar);
  mMenuBar = aMenuBar;
  return NS_OK;
}

NS_IMETHODIMP nsWindow::ShowMenuBar(PRBool aShow)
{
  // this may never be implemented on the Mac
  return NS_ERROR_FAILURE;
}

//-------------------------------------------------------------------------
//
// Get the widget's MenuBar.
//
//-------------------------------------------------------------------------
nsIMenuBar* nsWindow::GetMenuBar()
{
  return mMenuBar;
}


//
// SetCursor
//
// Override to set the cursor on the mac
//
NS_METHOD nsWindow::SetCursor(nsCursor aCursor)
{
  nsBaseWidget::SetCursor(aCursor);
	
  // mac specific cursor manipulation
	//�TODO: We need a way to get non-os cursors here.
	if (nsToolkit::HasAppearanceManager())
	{
		short cursor = -1;
		bool localResource = false;
	  switch (aCursor)
	  {
	    case eCursor_standard:	cursor = kThemeArrowCursor;		break;
	    case eCursor_wait:			cursor = kThemeWatchCursor;		break;
	    case eCursor_select:		cursor = kThemeIBeamCursor;		break;
	    case eCursor_hyperlink:	cursor = kThemePointingHandCursor;		break;
			case eCursor_sizeWE:		cursor = kThemeResizeLeftRightCursor;	break;
			case eCursor_sizeNS:		cursor = 129;	localResource = true; 	break;
	  }
	  if (cursor >= 0)
	  {
	  	if (localResource)
	  	{
				nsMacResources::OpenLocalResourceFile();
		  	::SetCursor(*(::GetCursor(cursor)));
				nsMacResources::CloseLocalResourceFile();
			}
			else
	  		::SetThemeCursor(cursor);
	  }
  }
  else
  {
		short cursor = -1;
		bool localResource = false;
	  switch (aCursor)
	  {
	    case eCursor_standard:	::InitCursor();					break;
	    case eCursor_wait:			cursor = watchCursor;		break;
	    case eCursor_select:		cursor = iBeamCursor;		break;
	    case eCursor_hyperlink:	cursor = plusCursor;		break;
			case eCursor_sizeWE:		cursor = 128;	localResource = true; 	break;
			case eCursor_sizeNS:		cursor = 129;	localResource = true; 	break;
	  }
	  if (cursor > 0)
	  {
	  	if (localResource)
	  	{
				nsMacResources::OpenLocalResourceFile();
		  	::SetCursor(*(::GetCursor(cursor)));
				nsMacResources::CloseLocalResourceFile();
			}
			else
			  	::SetCursor(*(::GetCursor(cursor)));
	  }
  }
 
  return NS_OK;
  
} // nsWindow :: SetCursor

#pragma mark -
//-------------------------------------------------------------------------
//
// Get this component dimension
//
//-------------------------------------------------------------------------
NS_IMETHODIMP nsWindow::GetBounds(nsRect &aRect)
{
  aRect = mBounds;
  return NS_OK;
}


NS_METHOD nsWindow::SetBounds(const nsRect &aRect)
{
printf("\nsetting bounds to x y w h %ld %ld %ld %ld\n\n", aRect.x, aRect.y, aRect.width, aRect.height );
  nsresult rv = Inherited::SetBounds(aRect);
  if ( NS_SUCCEEDED(rv) )
    CalcWindowRegions();

  return rv;
}


//-------------------------------------------------------------------------
//
// Move this component
// aX and aY are in the parent widget coordinate system
//-------------------------------------------------------------------------
NS_IMETHODIMP nsWindow::Move(PRInt32 aX, PRInt32 aY)
{
	if ((mBounds.x != aX) || (mBounds.y != aY))
	{
		// Invalidate the current location (unless it's the top-level window)
		if (mParent != nsnull)
			Invalidate(PR_FALSE);
	  	
		// Set the bounds
		mBounds.x = aX;
		mBounds.y = aY;

		// Recalculate the regions
		CalcWindowRegions();

		// Report the event
		ReportMoveEvent();
	}
	return NS_OK;
}

//-------------------------------------------------------------------------
//
// Resize this component
//
//-------------------------------------------------------------------------
NS_IMETHODIMP nsWindow::Resize(PRInt32 aWidth, PRInt32 aHeight, PRBool aRepaint)
{
  if ((mBounds.width != aWidth) || (mBounds.height != aHeight))
  {
    // Set the bounds
    mBounds.width  = aWidth;
    mBounds.height = aHeight;

	// Recalculate the regions
	CalcWindowRegions();
	
    // Invalidate the new location
    if (aRepaint)
      Invalidate(PR_FALSE);

    // Report the event
    ReportSizeEvent();
  }
  else {
    // Recalculate the regions. We always need to do this, our parents may have
    // changed, hence changing our notion of visibility. We then also should make
    // sure that we invalidate ourselves correctly. Fixes bug 18240 (pinkerton).
    CalcWindowRegions();
    if (aRepaint)
      Invalidate(PR_FALSE);
  }

  return NS_OK;
}

//-------------------------------------------------------------------------
//
// Resize this component
//
//-------------------------------------------------------------------------
NS_IMETHODIMP nsWindow::Resize(PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight, PRBool aRepaint)
{
	nsWindow::Move(aX, aY);
	nsWindow::Resize(aWidth, aHeight, aRepaint);
	return NS_OK;
}


NS_METHOD nsWindow::GetPreferredSize(PRInt32& aWidth, PRInt32& aHeight)
{
  aWidth  = mPreferredWidth;
  aHeight = mPreferredHeight;
  return NS_ERROR_FAILURE;
}

NS_METHOD nsWindow::SetPreferredSize(PRInt32 aWidth, PRInt32 aHeight)
{
  mPreferredWidth  = aWidth;
  mPreferredHeight = aHeight;
  return NS_OK;
}

//-------------------------------------------------------------------------
// 
//
//-------------------------------------------------------------------------
NS_IMETHODIMP nsWindow::BeginResizingChildren(void)
{
	mResizingChildren = PR_TRUE;
	mSaveVisible = mVisible;
	mVisible = PR_FALSE;

	return NS_OK;
}

//-------------------------------------------------------------------------
// 
//
//-------------------------------------------------------------------------
NS_IMETHODIMP nsWindow::EndResizingChildren(void)
{
	mResizingChildren = PR_FALSE;
	mVisible = mSaveVisible;

	CalcWindowRegions();
	return NS_OK;
}

#pragma mark -

#ifdef BLINK_DEBUGGING

static Boolean caps_lock()
{
	EventRecord event;
	::OSEventAvail(0, &event);
	return ((event.modifiers & alphaLock) != 0);
}

static void blinkRect(Rect* r)
{
	StRegionFromPool oldClip;
	if (oldClip != NULL)
		::GetClip(oldClip);

	::ClipRect(r);
	::InvertRect(r);
	UInt32 end = ::TickCount() + 5;
	while (::TickCount() < end) ;
	::InvertRect(r);

	if (oldClip != NULL)
		::SetClip(oldClip);
}

static void blinkRgn(RgnHandle rgn)
{
	StRegionFromPool oldClip;
	if (oldClip != NULL)
		::GetClip(oldClip);

	::SetClip(rgn);
	::InvertRgn(rgn);
	UInt32 end = ::TickCount() + 5;
	while (::TickCount() < end) ;
	::InvertRgn(rgn);

	if (oldClip != NULL)
		::SetClip(oldClip);
}

#endif

//-------------------------------------------------------------------------
//
// Invalidate this component's visible area
//
//-------------------------------------------------------------------------
NS_IMETHODIMP nsWindow::Invalidate(PRBool aIsSynchronous)
{
	nsRect area = mBounds;
	area.x = area.y = 0;
	nsWindow::Invalidate(area, aIsSynchronous);
	return NS_OK;
}

//-------------------------------------------------------------------------
//
// Invalidate this component's visible area
//
//-------------------------------------------------------------------------
NS_IMETHODIMP nsWindow::Invalidate(const nsRect &aRect, PRBool aIsSynchronous)
{
	if (!mWindowPtr)
		return NS_OK;

	nsRect wRect = aRect;
	wRect.MoveBy(mBounds.x, mBounds.y);				// beard:  this is required, see GetNativeData(NS_NATIVE_OFFSETX).
	LocalToWindowCoordinate(wRect);
	Rect macRect;
	nsRectToMacRect(wRect, macRect);

	StPortSetter portSetter(mWindowPtr);
	Rect savePortRect;
	::GetWindowPortBounds(mWindowPtr, &savePortRect);
	::SetOrigin(0, 0);

#ifdef BLINK_DEBUGGING
	if (caps_lock())
		::blinkRect(&macRect);
#endif

	::InvalWindowRect(mWindowPtr, &macRect);
	::SetOrigin(savePortRect.left, savePortRect.top);

	return NS_OK;
}

//-------------------------------------------------------------------------
//
// Invalidate this component's visible area
//
//-------------------------------------------------------------------------

NS_IMETHODIMP nsWindow::InvalidateRegion(const nsIRegion *aRegion, PRBool aIsSynchronous)
{
	if (!mWindowPtr)
		return NS_OK;

	// copy invalid region into a working region.
	void* nativeRgn;
	aRegion->GetNativeRegion(nativeRgn);
	StRegionFromPool windowRgn;
	::CopyRgn(RgnHandle(nativeRgn), windowRgn);

	// translate this region into window coordinates.
	PRInt32	offX, offY;
	this->CalcOffset(offX, offY);
	::OffsetRgn(windowRgn, mBounds.x + offX, mBounds.y + offY);
	
	StPortSetter portSetter(mWindowPtr);
	Rect savePortRect;
	::GetWindowPortBounds(mWindowPtr, &savePortRect);
	::SetOrigin(0, 0);

#ifdef BLINK_DEBUGGING
	if (caps_lock())
		::blinkRgn(windowRgn);
#endif

	::InvalWindowRgn(mWindowPtr, windowRgn);
	::SetOrigin(savePortRect.left, savePortRect.top);

	return NS_OK;
}

inline PRUint16 COLOR8TOCOLOR16(PRUint8 color8)
{
	// return (color8 == 0xFF ? 0xFFFF : (color8 << 8));
	return (color8 << 8) | color8;	/* (color8 * 257) == (color8 * 0x0101) */
}

//-------------------------------------------------------------------------
//	StartDraw
//
//-------------------------------------------------------------------------
void nsWindow::StartDraw(nsIRenderingContext* aRenderingContext)
{
	if (mDrawing)
		return;
	mDrawing = PR_TRUE;

	CalcWindowRegions();	//�REVISIT

	if (aRenderingContext == nsnull)
	{
		// make sure we have a rendering context
		mTempRenderingContext = GetRenderingContext();
		mTempRenderingContextMadeHere = PR_TRUE;
	}
	else
	{
		// if we already have a rendering context, save its state
		NS_IF_ADDREF(aRenderingContext);
		mTempRenderingContext = aRenderingContext;
		mTempRenderingContextMadeHere = PR_FALSE;
		mTempRenderingContext->PushState();

		// set the environment to the current widget
		mTempRenderingContext->Init(mContext, this);
	}

	// set the widget font. nsMacControl implements SetFont, which is where
	// the font should get set.
	if (mFontMetrics)
	{
		mTempRenderingContext->SetFont(mFontMetrics);
	}

	// set the widget background and foreground colors
	nscolor color = GetBackgroundColor();
	RGBColor macColor;
	macColor.red   = COLOR8TOCOLOR16(NS_GET_R(color));
	macColor.green = COLOR8TOCOLOR16(NS_GET_G(color));
	macColor.blue  = COLOR8TOCOLOR16(NS_GET_B(color));
	::RGBBackColor(&macColor);

	color = GetForegroundColor();
	macColor.red   = COLOR8TOCOLOR16(NS_GET_R(color));
	macColor.green = COLOR8TOCOLOR16(NS_GET_G(color));
	macColor.blue  = COLOR8TOCOLOR16(NS_GET_B(color));
	::RGBForeColor(&macColor);

	mTempRenderingContext->SetColor(color);				// just in case, set the rendering context color too
}


//-------------------------------------------------------------------------
//	EndDraw
//
//-------------------------------------------------------------------------
void nsWindow::EndDraw()
{
	if (! mDrawing)
		return;
	mDrawing = PR_FALSE;

	if (mTempRenderingContextMadeHere)
	{
		PRBool clipEmpty;
		mTempRenderingContext->PopState(clipEmpty);
	}
	NS_RELEASE(mTempRenderingContext);
}


//-------------------------------------------------------------------------
//
//
//-------------------------------------------------------------------------
void
nsWindow::Flash(nsPaintEvent	&aEvent)
{
#ifdef NS_DEBUG
	Rect flashRect;
	if (debug_WantPaintFlashing() && aEvent.rect ) {
		::SetRect ( &flashRect, aEvent.rect->x, aEvent.rect->y, aEvent.rect->x + aEvent.rect->width,
	          aEvent.rect->y + aEvent.rect->height );
		::InvertRect ( &flashRect );
		for (int x = 0; x < 1000000; x++) ;
		::InvertRect ( &flashRect );
		for (int x = 0; x < 1000000; x++) ;    
		::InvertRect ( &flashRect );
		for (int x = 0; x < 1000000; x++) ;    
		::InvertRect ( &flashRect );
		for (int x = 0; x < 1000000; x++) ;    
	}
#endif
}


//
// OnPaint
//
// Dummy impl, meant to be overridden
//
PRBool
nsWindow::OnPaint(nsPaintEvent &event)
{
	return PR_TRUE;
}


//-------------------------------------------------------------------------
//	Update
//
//		Redraw this widget.
//
//		We draw the widget between BeginUpdate and EndUpdate because some
//		operations go much faster when the visRgn contains what needs to be
//		painted. Then we restore the original updateRgn and validate this
//		widget's rectangle.
//-------------------------------------------------------------------------
NS_IMETHODIMP	nsWindow::Update()
{
	if (! mVisible || !mWindowPtr)
		return NS_OK;

	static PRBool  reentrant = PR_FALSE;

	if (reentrant)
		HandleUpdateEvent();
	else
	{
		reentrant = PR_TRUE;

		// make a copy of the window update rgn
		StRegionFromPool saveUpdateRgn;
		if (!saveUpdateRgn)
			return NS_ERROR_OUT_OF_MEMORY;
		if(mWindowPtr)
			GetWindowUpdateRegion ( mWindowPtr, saveUpdateRgn );

		// draw the widget
		StPortSetter portSetter(mWindowPtr);

		::BeginUpdate(mWindowPtr);
		HandleUpdateEvent();
		::EndUpdate(mWindowPtr);

		// restore the window update rgn
#if TARGET_CARBON
		//�PINK - hrm, can't do this in Carbon for re-entrancy reasons
		// ::CopyRgn(saveUpdateRgn, ((WindowRecord*)mWindowPtr)->updateRgn);
		::InvalWindowRgn(mWindowPtr, saveUpdateRgn);
#else
		::CopyRgn(saveUpdateRgn, ((WindowRecord*)mWindowPtr)->updateRgn);
#endif

		// validate the rect of the widget we have just drawn
		nsRect bounds = mBounds;
		LocalToWindowCoordinate(bounds);
		Rect macRect;
		nsRectToMacRect(bounds, macRect);

		::ValidWindowRect(mWindowPtr, &macRect);

		reentrant = PR_FALSE;
	}

	return NS_OK;
}

static Boolean control_key_down()
{
	EventRecord event;
	::OSEventAvail(0, &event);
	return (event.modifiers & controlKey) != 0;
}

static long long microseconds()
{
	unsigned long long micros;
	Microseconds((UnsignedWide*)&micros);
	return micros;
}

//-------------------------------------------------------------------------
//	HandleUpdateEvent
//
//		Called by the event handler to redraw the top-level widget.
//		Must be called between BeginUpdate/EndUpdate: the window visRgn
//		is expected to be set to whatever needs to be drawn.
//-------------------------------------------------------------------------
nsresult nsWindow::HandleUpdateEvent()
{
	if (! mVisible)
		return NS_OK;

	// make sure the port is set and origin is (0, 0).
	StPortSetter portSetter(mWindowPtr);
	::SetOrigin(0, 0);
	
	// get the damaged region from the OS
	StRegionFromPool damagedRgn;
	if (!damagedRgn)
		return NS_ERROR_OUT_OF_MEMORY;
	::GetPortVisibleRegion(GrafPtr(GetWindowPort(mWindowPtr)), damagedRgn);

#ifdef PAINT_DEBUGGING	
	blinkRgn(damagedRgn);
#endif

	// calculate the update region relatively to the window port rect
	// (at this point, the grafPort origin should always be 0,0
	// so mWindowRegion has to be converted to window coordinates)
	StRegionFromPool updateRgn;
	if (!updateRgn)
		return NS_ERROR_OUT_OF_MEMORY;
	::CopyRgn(mWindowRegion, updateRgn);

	nsRect bounds = mBounds;
	LocalToWindowCoordinate(bounds);
	::OffsetRgn(updateRgn, bounds.x, bounds.y);

#ifdef PAINT_DEBUGGING
	blinkRgn(updateRgn);
#endif
	
	// check if the update region is visible
	::SectRgn(damagedRgn, updateRgn, updateRgn);
	if (!::EmptyRgn(updateRgn))
	{
		nsIRenderingContext* renderingContext = GetRenderingContext();
		if (renderingContext)
		{
			// determine the rect to draw
			nsRect rect;
			Rect macRect;
			::GetRegionBounds(updateRgn, &macRect);
			::OffsetRect(&macRect, -bounds.x, -bounds.y);
			rect.SetRect(macRect.left, macRect.top, macRect.right - macRect.left, macRect.bottom - macRect.top);

#if DEBUG
			// measure the time it takes to refresh the window, if the control key is down.
			unsigned long long start, finish;
			Boolean measure_duration = control_key_down();
			if (measure_duration)
				start = microseconds();
#endif

			// update the widget
			UpdateWidget(rect, renderingContext);

#if DEBUG
			if (measure_duration) {
				finish = microseconds();
				printf("update took %g microseconds.\n", double(finish - start));
			}
#endif

			NS_RELEASE(renderingContext);
		}
	}
	return NS_OK;
}


//-------------------------------------------------------------------------
//
//
//-------------------------------------------------------------------------
void nsWindow::UpdateWidget(nsRect& aRect, nsIRenderingContext* aContext)
{
	if (! mVisible)
		return;

	// initialize the paint event
	nsPaintEvent paintEvent;
	paintEvent.eventStructType			= NS_PAINT_EVENT;		// nsEvent
	paintEvent.message					= NS_PAINT;
	paintEvent.widget					= this;					// nsGUIEvent
	paintEvent.nativeMsg				= NULL;
	paintEvent.renderingContext			= aContext;				// nsPaintEvent
	paintEvent.rect						= &aRect;

	// draw the widget
	StartDraw(aContext);
	if ( OnPaint(paintEvent) ) {
		nsEventStatus	eventStatus;
		DispatchWindowEvent(paintEvent,eventStatus);
		if(eventStatus != nsEventStatus_eIgnore)
			Flash(paintEvent);
	}
	EndDraw();

	// beard:  Since we clip so aggressively, drawing from front to back should work,
	// and it does for the most part. However; certain cases, such as overlapping
	// areas that are handled by different view managers, don't properly clip siblings.
#ifdef FRONT_TO_BACK
#	define FIRST_CHILD(children) (children->Last())
#	define NEXT_CHILD(children) (children->Prev())
#else
#	define FIRST_CHILD(children) (children->First())
#	define NEXT_CHILD(children) (children->Next())
#endif

	// recursively draw the children
	nsCOMPtr<nsIBidirectionalEnumerator> children(getter_AddRefs((nsIBidirectionalEnumerator*)GetChildren()));
	if (children) {
		FIRST_CHILD(children);
		do {
			nsISupports* child;
			if (NS_SUCCEEDED(children->CurrentItem(&child))) {
				nsWindow* childWindow = static_cast<nsWindow*>(static_cast<nsIWidget*>(child));

				nsRect childBounds;
				childWindow->GetBounds(childBounds);

				// redraw only the intersection of the child rect and the update rect
				nsRect intersection;
				if (intersection.IntersectRect(aRect, childBounds))
				{
					intersection.MoveBy(-childBounds.x, -childBounds.y);
					childWindow->UpdateWidget(intersection, aContext);
				}
				
				NS_RELEASE(child);
    		}
		} while (NS_SUCCEEDED(NEXT_CHILD(children)));
	}

#undef FIRST_CHILD
#undef NEXT_CHILD
}


//
// ScrollBits
//
// ::ScrollRect() unfortunately paints the invalidated area with the 
// background pattern. This causes lots of ugly flashing and makes us look 
// pretty bad. Instead, we roll our own ::ScrollRect() by using ::CopyBits() 
// to scroll the image in the view and then set the update
// rgn appropriately so that the compositor can blit it to the screen.
//
// This will also work with system floating windows over the area that is
// scrolling.
//
// ���� This routine really needs to be Carbonated!!!! It is nowhere close,
// ���� even though there are a couple of carbon ifdefs here already.
//
void
nsWindow :: ScrollBits ( Rect & inRectToScroll, PRInt32 inLeftDelta, PRInt32 inTopDelta )
{	
	// Get Frame in local coords from clip rect (there might be a border around view)
	StRegionFromPool clipRgn;
	if ( !clipRgn ) return;
	::GetClip(clipRgn);
	::SectRgn(clipRgn, mVisRegion, clipRgn);

	Rect frame;
	::GetRegionBounds(clipRgn, &frame);

	StRegionFromPool totalVisRgn;
	if ( !totalVisRgn ) return;
	::RectRgn(totalVisRgn, &frame);
	
		// compute the source and destination of copybits
	Rect source = inRectToScroll;
	SectRect(&source, &frame, &source);

	Rect dest = source;
	::OffsetRect(&dest, inLeftDelta, inTopDelta);

	// compute the area that is to be updated by subtracting the dest from the visible area
	StRegionFromPool destRgn;
	if ( !destRgn ) return;
	::RectRgn(destRgn, &dest);		

	StRegionFromPool updateRgn;
	if ( !updateRgn ) return;
	::RectRgn(updateRgn, &frame);
	::DiffRgn (updateRgn, destRgn, updateRgn);
		
	if(::EmptyRgn(mWindowPtr->visRgn))		
	{
		::CopyBits ( 
			&mWindowPtr->portBits, 
			&mWindowPtr->portBits, 
			&source, 
			&dest, 
			srcCopy, 
			nil);
	}
	else
	{
		// compute the non-visable region
		StRegionFromPool nonVisableRgn;
		if ( !nonVisableRgn ) return;
		::DiffRgn ( totalVisRgn, mWindowPtr->visRgn, nonVisableRgn );
		
		// compute the extra area that may need to be updated
		// scoll the non-visable region to determine what needs updating
		::OffsetRgn ( nonVisableRgn, inLeftDelta, inTopDelta );
		
		// calculate a mask region to not copy the non-visble portions of the window from the port
		StRegionFromPool copyMaskRgn;
		if ( !copyMaskRgn ) return;
		::DiffRgn(totalVisRgn, nonVisableRgn, copyMaskRgn);
		
		// use copybits to simulate a ScrollRect()
		RGBColor black = { 0, 0, 0 };
		RGBColor white = { 0xFFFF, 0xFFFF, 0xFFFF } ;
		::RGBForeColor(&black);
		::RGBBackColor(&white);
		::PenNormal();	

		::CopyBits ( 
			&mWindowPtr->portBits, 
			&mWindowPtr->portBits, 
			&source, 
			&dest, 
			srcCopy, 
			copyMaskRgn);
			
		// union the update regions together and invalidate them
		::UnionRgn(nonVisableRgn, updateRgn, updateRgn);
	}
	
	::InvalWindowRgn(mWindowPtr, updateRgn);
}


//-------------------------------------------------------------------------
//
// Scroll the bits of a window
//
//-------------------------------------------------------------------------
NS_IMETHODIMP nsWindow::Scroll(PRInt32 aDx, PRInt32 aDy, nsRect *aClipRect)
{
	if (! mVisible)
		return NS_OK;
	
	// If the clipping region is non-rectangular, just force a full update, sorry.
	if (IsRegionComplex(mWindowRegion)) {
		Invalidate(PR_TRUE);
		goto scrollChildren;
	}

	//--------
	// Scroll this widget
	nsRect scrollRect;
	if (aClipRect)
		scrollRect = *aClipRect;
	else
	{
		scrollRect = mBounds;
		scrollRect.x = scrollRect.y = 0;
	}

	Rect macRect;
	nsRectToMacRect(scrollRect, macRect);


	StartDraw();

#if 1
		// Clip to the windowRegion instead of the visRegion (note: the visRegion
		// is equal to the windowRegion minus the children). The result is that
		// ScrollRect() scrolls the visible bits of this widget as well as its children.
		::SetClip(mWindowRegion);

		// Scroll the bits now. We've rolled our own because ::ScrollRect looks ugly
		ScrollBits(macRect,aDx,aDy);
#else
		StRegionFromPool updateRgn;
		::ScrollRect(&macRect, aDx, aDy, updateRgn);
		::InvalRgn(updateRgn);
#endif
	EndDraw();

scrollChildren:
	//--------
	// Scroll the children
	nsCOMPtr<nsIEnumerator> children ( getter_AddRefs(GetChildren()) );
	if (children)
	{
		children->First();
		do
		{
			nsISupports* child;
			if (NS_SUCCEEDED(children->CurrentItem(&child)))
			{
				nsWindow* childWindow = static_cast<nsWindow*>(static_cast<nsIWidget*>(child));
				NS_RELEASE(child);

				nsRect bounds;
				childWindow->GetBounds(bounds);
				bounds.x += aDx;
				bounds.y += aDy;
				childWindow->SetBounds(bounds);
  		}
		} while (NS_SUCCEEDED(children->Next()));			
	}

	// recalculate the window regions
	CalcWindowRegions();

	return NS_OK;
}

//-------------------------------------------------------------------------
//
//
//-------------------------------------------------------------------------

PRBool nsWindow::ConvertStatus(nsEventStatus aStatus)
{
  switch (aStatus)
  {
    case nsEventStatus_eIgnore:							return(PR_FALSE);
    case nsEventStatus_eConsumeNoDefault:		return(PR_TRUE);	// don't do default processing
    case nsEventStatus_eConsumeDoDefault:		return(PR_FALSE);
    default:
      NS_ASSERTION(0, "Illegal nsEventStatus enumeration value");
      break;
  }
  return(PR_FALSE);
}

//-------------------------------------------------------------------------
//
// Invokes callback and  ProcessEvent method on Event Listener object
//
//-------------------------------------------------------------------------
NS_IMETHODIMP nsWindow::DispatchEvent(nsGUIEvent* event, nsEventStatus& aStatus)
{
  aStatus = nsEventStatus_eIgnore;
	if (! mDestructorCalled)
	{
		nsIWidget* aWidget = event->widget;
		NS_IF_ADDREF(aWidget);
	  
	  if (nsnull != mMenuListener){
	    if(NS_MENU_EVENT == event->eventStructType)
	  	  aStatus = mMenuListener->MenuSelected( static_cast<nsMenuEvent&>(*event) );
	  }
	  if (mEventCallback)
	    aStatus = (*mEventCallback)(event);

		// Dispatch to event listener if event was not consumed
	  if ((aStatus != nsEventStatus_eConsumeNoDefault) && (mEventListener != nsnull))
	    aStatus = mEventListener->ProcessEvent(*event);

		NS_IF_RELEASE(aWidget);
	}
  return NS_OK;
}

//-------------------------------------------------------------------------
PRBool nsWindow::DispatchWindowEvent(nsGUIEvent &event)
{
  nsEventStatus status;
  DispatchEvent(&event, status);
  return ConvertStatus(status);
}

//-------------------------------------------------------------------------
PRBool nsWindow::DispatchWindowEvent(nsGUIEvent &event,nsEventStatus &aStatus)
{
  DispatchEvent(&event, aStatus);
  return ConvertStatus(aStatus);
}

//-------------------------------------------------------------------------
//
// Deal with all sort of mouse event
//
//-------------------------------------------------------------------------
PRBool nsWindow::DispatchMouseEvent(nsMouseEvent &aEvent)
{

  PRBool result = PR_FALSE;
  if (nsnull == mEventCallback && nsnull == mMouseListener) {
    return result;
  }

  // call the event callback 
  if (nsnull != mEventCallback) 
  	{
    result = (DispatchWindowEvent(aEvent));
    return result;
  	}

  if (nsnull != mMouseListener) {
    switch (aEvent.message) {
      case NS_MOUSE_MOVE: {
        result = ConvertStatus(mMouseListener->MouseMoved(aEvent));
        nsRect rect;
        GetBounds(rect);
        if (rect.Contains(aEvent.point.x, aEvent.point.y)) 
        	{
          //if (mWindowPtr == NULL || mWindowPtr != this) 
          	//{
            printf("Mouse enter");
            //mCurrentWindow = this;
          	//}
        	} 
        else 
        	{
          printf("Mouse exit");
        	}

      } break;

      case NS_MOUSE_LEFT_BUTTON_DOWN:
      case NS_MOUSE_MIDDLE_BUTTON_DOWN:
      case NS_MOUSE_RIGHT_BUTTON_DOWN:
        result = ConvertStatus(mMouseListener->MousePressed(aEvent));
        break;

      case NS_MOUSE_LEFT_BUTTON_UP:
      case NS_MOUSE_MIDDLE_BUTTON_UP:
      case NS_MOUSE_RIGHT_BUTTON_UP:
        result = ConvertStatus(mMouseListener->MouseReleased(aEvent));
        result = ConvertStatus(mMouseListener->MouseClicked(aEvent));
        break;
    } // switch
  } 
  return result;
}

#pragma mark -

//-------------------------------------------------------------------------
//
//
//-------------------------------------------------------------------------
PRBool nsWindow::ReportDestroyEvent()
{
	// nsEvent
	nsGUIEvent moveEvent;
	moveEvent.eventStructType = NS_GUI_EVENT;
	moveEvent.message			= NS_DESTROY;
	moveEvent.point.x			= 0;
	moveEvent.point.y			= 0;
	moveEvent.time				= PR_IntervalNow();

	// nsGUIEvent
	moveEvent.widget			= this;
	moveEvent.nativeMsg		= nsnull;

	// dispatch event
	return (DispatchWindowEvent(moveEvent));
}

//-------------------------------------------------------------------------
//
//
//-------------------------------------------------------------------------
PRBool nsWindow::ReportMoveEvent()
{
	// nsEvent
	nsGUIEvent moveEvent;
	moveEvent.eventStructType = NS_GUI_EVENT;
	moveEvent.message			= NS_MOVE;
	moveEvent.point.x			= mBounds.x;
	moveEvent.point.y			= mBounds.y;
	moveEvent.time				= PR_IntervalNow();

	// nsGUIEvent
	moveEvent.widget			= this;
	moveEvent.nativeMsg		= nsnull;

	// dispatch event
	return (DispatchWindowEvent(moveEvent));
}

//-------------------------------------------------------------------------
//
//
//-------------------------------------------------------------------------
PRBool nsWindow::ReportSizeEvent()
{
	// nsEvent
	nsSizeEvent sizeEvent;
	sizeEvent.eventStructType = NS_SIZE_EVENT;
	sizeEvent.message			= NS_SIZE;
	sizeEvent.point.x			= 0;
	sizeEvent.point.y			= 0;
	sizeEvent.time				= PR_IntervalNow();

	// nsGUIEvent
	sizeEvent.widget			= this;
	sizeEvent.nativeMsg		= nsnull;

	// nsSizeEvent
	sizeEvent.windowSize	= &mBounds;
	sizeEvent.mWinWidth		= mBounds.width;
	sizeEvent.mWinHeight	= mBounds.height;
  
	// dispatch event
	return(DispatchWindowEvent(sizeEvent));
}



#pragma mark -

//-------------------------------------------------------------------------
//
//
//-------------------------------------------------------------------------
void nsWindow::CalcWindowRegions()
{
	//------
	// calculate the window region
	if (mWindowRegion == nsnull)
	{
		mWindowRegion = ::NewRgn();
		if (mWindowRegion == nsnull)
			return;
	}
 	::SetRectRgn(mWindowRegion, 0, 0, mBounds.width, mBounds.height);

	// intersect with all the parents
	nsWindow* parent = (nsWindow*)mParent;
	nsPoint origin(-mBounds.x, -mBounds.y);
	while (parent)
	{
		if (parent->mWindowRegion)
		{
			::OffsetRgn(parent->mWindowRegion, origin.x, origin.y);
			::SectRgn(mWindowRegion, parent->mWindowRegion, mWindowRegion);
			::OffsetRgn(parent->mWindowRegion, -origin.x, -origin.y);
		}
		origin.x -= parent->mBounds.x;
		origin.y -= parent->mBounds.y;
		parent = (nsWindow*)parent->mParent;
	}

	//------
	// calculate the visible region
	if (mVisRegion == nsnull)
	{
		mVisRegion = ::NewRgn();
		if (mVisRegion == nsnull)
			return;
	}
	::CopyRgn(mWindowRegion, mVisRegion);

	// clip the children out of the visRegion
	nsCOMPtr<nsIEnumerator> children ( getter_AddRefs(GetChildren()) );
	if (children)
	{
		StRegionFromPool childRgn;
		if (childRgn != nsnull) {
			children->First();
			do
			{
				nsISupports* child;
				if (NS_SUCCEEDED(children->CurrentItem(&child)))
				{
					nsWindow* childWindow = static_cast<nsWindow*>(static_cast<nsIWidget*>(child));
					NS_RELEASE(child);
					
					PRBool visible;
					childWindow->IsVisible(visible);
					if (visible) {
						nsRect childRect;
						childWindow->GetBounds(childRect);

						Rect macRect;
						::SetRect(&macRect, childRect.x, childRect.y, childRect.XMost(), childRect.YMost());
						::RectRgn(childRgn, &macRect);
						::DiffRgn(mVisRegion, childRgn, mVisRegion);
					}
				}
			} while (NS_SUCCEEDED(children->Next()));
		}
	}
}

//-------------------------------------------------------------------------
/*
 *  @update  dc 08/28/98
 *  @param   aTheRegion -- The region to intersect with for this widget
 *  @return  PR_TRUE if the these regions intersect
 */

PRBool nsWindow::RgnIntersects(RgnHandle aTheRegion, RgnHandle aIntersectRgn)
{
	::SectRgn(aTheRegion, this->mWindowRegion, aIntersectRgn);
	return (::EmptyRgn(aIntersectRgn) != false);
}



//-------------------------------------------------------------------------
/*  Calculate the x and y offsets for this particular widget
 *  @update  ps 09/22/98
 *  @param   aX -- x offset amount
 *  @param   aY -- y offset amount 
 *  @return  NOTHING
 */
 
NS_IMETHODIMP nsWindow::CalcOffset(PRInt32 &aX,PRInt32 &aY)
{
	aX = aY = 0;
	nsIWidget* grandParent;
	nsIWidget* theParent = this->GetParent();
	while (theParent)
	{
		nsRect theRect;
		theParent->GetBounds(theRect);
		aX += theRect.x;
		aY += theRect.y;

		grandParent = theParent->GetParent();
		NS_IF_RELEASE(theParent);
		theParent = grandParent;
	}
	return NS_OK;
}


//-------------------------------------------------------------------------
// PointInWidget
//		Find if a point in local coordinates is inside this object
//-------------------------------------------------------------------------
PRBool nsWindow::PointInWidget(Point aThePoint)
{
	// get the origin in local coordinates
	nsPoint widgetOrigin(0, 0);
	LocalToWindowCoordinate(widgetOrigin);

	// get rectangle relatively to the parent
	nsRect widgetRect;
	GetBounds(widgetRect);

	// convert the topLeft corner to local coordinates
	widgetRect.MoveBy(widgetOrigin.x, widgetOrigin.y);

	// finally tell whether it's a hit
	return(widgetRect.Contains(aThePoint.h, aThePoint.v));
}


//-------------------------------------------------------------------------
// FindWidgetHit
//		Recursively look for the widget hit
//		@param aParent   -- parent widget. 
//		@param aThePoint -- a point in local coordinates to test for the hit. 
//-------------------------------------------------------------------------
nsWindow*  nsWindow::FindWidgetHit(Point aThePoint)
{
	if (!mVisible || !PointInWidget(aThePoint))
		return nsnull;

	nsWindow* widgetHit = this;

	nsCOMPtr<nsIEnumerator> normalEnum ( getter_AddRefs(GetChildren()) );
	nsCOMPtr<nsIBidirectionalEnumerator> children ( do_QueryInterface(normalEnum) );
	if (children)
	{
		// traverse through all the nsWindows to find out who got hit, lowest level of course
		children->Last();
		do
		{
			nsISupports* child;
			if (NS_SUCCEEDED(children->CurrentItem(&child)))
      {
      	nsWindow* childWindow = static_cast<nsWindow*>(static_cast<nsIWidget*>(child));
				NS_RELEASE(child);

			  nsWindow* deeperHit = childWindow->FindWidgetHit(aThePoint);
			  if (deeperHit)
			  {
				  widgetHit = deeperHit;
				  break;
			  }
      }
		}
    while (NS_SUCCEEDED(children->Prev()));
	}

	return widgetHit;
}

#pragma mark -


//-------------------------------------------------------------------------
// WidgetToScreen
//		Walk up the parent tree, converting the given rect to global coordinates.
//      This is similiar to CalcOffset() but we can't use GetBounds() because it
//      only knows how to give us local coordinates.
//		@param aLocalRect  -- rect in local coordinates of this widget
//		@param aGlobalRect -- |aLocalRect| in global coordinates
//-------------------------------------------------------------------------
NS_IMETHODIMP nsWindow::WidgetToScreen(const nsRect& aLocalRect, nsRect& aGlobalRect)
{	
	aGlobalRect = aLocalRect;
	nsIWidget* theParent = this->GetParent();
	if ( theParent ) {
		// Recursive case
		//
		// Convert the local rect to global, except for this level.
		theParent->WidgetToScreen(aLocalRect, aGlobalRect);
	  NS_RELEASE(theParent);

		// the offset from our parent is in the x/y of our bounding rect
		nsRect myBounds;
		GetBounds(myBounds);
		aGlobalRect.MoveBy(myBounds.x, myBounds.y);
	}
	else {
		// Base case of recursion
		//
		// When there is no parent, we're at the top level window. Use
		// the origin (shifted into global coordinates) to find the offset.
		StPortSetter	portSetter(mWindowPtr);
		::SetOrigin(0,0);
		
		// convert origin into global coords and shift output rect by that ammount
		Point origin = {0, 0};
		::LocalToGlobal ( &origin );
		aGlobalRect.MoveBy ( origin.h, origin.v );
	}
	
	return NS_OK;
}



//-------------------------------------------------------------------------
// ScreenToWidget
//		Walk up the parent tree, converting the given rect to local coordinates.
//		@param aGlobalRect  -- rect in screen coordinates 
//		@param aLocalRect -- |aGlobalRect| in coordinates of this widget
//-------------------------------------------------------------------------
NS_IMETHODIMP nsWindow::ScreenToWidget(const nsRect& aGlobalRect, nsRect& aLocalRect)
{
	aLocalRect = aGlobalRect;
	nsIWidget* theParent = this->GetParent();
	if ( theParent ) {
		// Recursive case
		//
		// Convert the local rect to global, except for this level.
		theParent->WidgetToScreen(aGlobalRect, aLocalRect);
	  NS_RELEASE(theParent);
	  
		// the offset from our parent is in the x/y of our bounding rect
		nsRect myBounds;
		GetBounds(myBounds);
		aLocalRect.MoveBy(myBounds.x, myBounds.y);
	}
	else {
		// Base case of recursion
		//
		// When there is no parent, we're at the top level window. Use
		// the origin (shifted into local coordinates) to find the offset.
		StPortSetter	portSetter(mWindowPtr);
		::SetOrigin(0,0);
		
		// convert origin into local coords and shift output rect by that ammount
		Point origin = {0, 0};
		::GlobalToLocal ( &origin );
		aLocalRect.MoveBy ( origin.h, origin.v );
	}
	
	return NS_OK;
} 


/*
 *  Set a Mac Rect to the value of an nsRect 
 *  The source rect is assumed to be in pixels not TWIPS
 *  @update  gpk 08/27/98
 *  @param   aRect -- The nsRect that is the source
 *  @param   aMacRect -- The Mac Rect destination
 */
void nsWindow::nsRectToMacRect(const nsRect& aRect, Rect& aMacRect) const
{
		aMacRect.left = aRect.x;
		aMacRect.top = aRect.y;
		aMacRect.right = aRect.x + aRect.width;
		aMacRect.bottom = aRect.y + aRect.height;
}


//=================================================================
/*  Convert the coordinates to some device coordinates so GFX can draw.
 *  @update  dc 09/16/98
 *  @param   nscoord -- X coordinate to convert
 *  @param   nscoord -- Y coordinate to convert
 *  @return  NONE
 */
void  nsWindow::ConvertToDeviceCoordinates(nscoord &aX, nscoord &aY)
{
	PRInt32	offX, offY;
	this->CalcOffset(offX,offY);

	aX += offX;
	aY += offY;
}

NS_IMETHODIMP nsWindow::CaptureRollupEvents(nsIRollupListener * aListener, 
                                            PRBool aDoCapture, 
                                            PRBool aConsumeRollupEvent)
{
  if (aDoCapture) {
    NS_IF_RELEASE(gRollupListener);
    NS_IF_RELEASE(gRollupWidget);
    gRollupListener = aListener;
    NS_ADDREF(aListener);
    gRollupWidget = this;
    NS_ADDREF(this);
  } else {
    NS_IF_RELEASE(gRollupListener);
    //gRollupListener = nsnull;
    NS_IF_RELEASE(gRollupWidget);
  }

  return NS_OK;
}

NS_IMETHODIMP nsWindow::SetTitle(const nsString& title)
{
  NS_ASSERTION(0, "Would some Mac person please implement me? Thanks.");
  return NS_OK;
}

NS_IMETHODIMP nsWindow::GetAttention()
{
	// Since the Mac doesn't consider each window a seperate process this call functions
	// slightly different than on other platforms.  We first check to see if we're the
	// foreground process and, if so, ignore the call.  We also check to see if a notification
	// is already pending and, if so, remove it since we only have one notification per process.
	// After all that checking we install a notification manager request to mark the app's icon
	// in the process menu and play the default alert sound
  
	if (we_are_front_process())
		return NS_OK;
  
	if (gNotificationInstalled)
	{
		(void)::NMRemove(&gNMRec);
		gNotificationInstalled = false;
	}
	
	// Setup and install the notification manager rec
	gNMRec.qType		= nmType;
	gNMRec.nmMark		= 1;			// Flag the icon in the process menu
	gNMRec.nmIcon		= NULL;			// It'd be nice if we had an icon to flash - maybe later
	gNMRec.nmSound		= (Handle)-1L;	// Use the default alert sound
	gNMRec.nmStr		= NULL;			// No alert/window so no text
	gNMRec.nmResp		= NULL;			// No response proc, use the default behavior
	gNMRec.nmRefCon	= NULL;
	if (::NMInstall(&gNMRec) == noErr)
		gNotificationInstalled = true;

	return NS_OK;
}

#pragma mark -


NS_IMETHODIMP nsWindow::ResetInputState()
{
	// currently, the nsMacEventHandler is owned by nsMacWindow, which is the top level window
	// we deletgate this call to it's parent
  nsCOMPtr<nsIWidget> parent = getter_AddRefs(GetParent());
  NS_ASSERTION(parent, "cannot get parent");
  if(parent)
  {
  	nsCOMPtr<nsIKBStateControl> kb = do_QueryInterface(parent);
 	  NS_ASSERTION(kb, "cannot get parent");
  	if(kb) {
  		return kb->ResetInputState();
  	}
  }
	return NS_ERROR_ABORT;
}
NS_IMETHODIMP nsWindow::PasswordFieldInit()
{
	// currently, the nsMacEventHandler is owned by nsMacWindow, which is the top level window
	// we deletgate this call to it's parent
  nsCOMPtr<nsIWidget> parent = getter_AddRefs(GetParent());
  NS_ASSERTION(parent, "cannot get parent");
  if(parent)
  {
  	nsCOMPtr<nsIKBStateControl> kb = do_QueryInterface(parent);
 	  NS_ASSERTION(kb, "cannot get parent");
  	if(kb) {
  		return kb->PasswordFieldInit();
  	}
  }
	return NS_ERROR_ABORT;
}
