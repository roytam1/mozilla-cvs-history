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
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 *  Simon Fraser <sfraser@netscape.com>
 */

#include <MacWindows.h>

#include "nsCommandLineServiceMac.h"
#include "nsComPtr.h"
#include "nsIBaseWindow.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsIDOMDocument.h"
#include "nsIDOMWindow.h"
#include "nsIDOMWindowInternal.h"
#include "nsIDOMElement.h"
#include "nsIDOMNode.h"
#include "nsIHTMLContent.h"
#include "nsIPrincipal.h"
#include "nsICodebasePrincipal.h"
#include "nsIServiceManager.h"
#include "nsIWebNavigation.h"
#include "nsIWebShellWindow.h"
#include "nsIWidget.h"
#include "nsIWindowMediator.h"
#include "nsIURI.h"
#include "nsIXULWindow.h"
#include "nsString.h"
#include "nsWindowUtils.h"
#include "nsMacUtils.h"
#include "nsXPIDLString.h"
#include "nsXULWindow.h"

#include "nsAEUtils.h"

// CIDs
static NS_DEFINE_CID(kWindowMediatorCID, NS_WINDOWMEDIATOR_CID);


/*----------------------------------------------------------------------------
	GetXULWindowFromWindowPtr 
	
	Get an nsIXULWindow from a WindowPtr. Returns an ADDREFFED xulWindow,
	which you must release (hint: use an nsCOMPtr).
	
	Throws on error.
----------------------------------------------------------------------------*/

static void GetXULWindowFromWindowPtr(WindowPtr inWindowPtr, nsIXULWindow **outXULWindow)
{
	*outXULWindow = NULL;
	
	if (!inWindowPtr)
		ThrowOSErr(paramErr);
	
	nsCOMPtr<nsIWindowMediator> windowMediator(do_GetService(kWindowMediatorCID));
	ThrowErrIfNil(windowMediator, paramErr);

	nsCOMPtr<nsISimpleEnumerator> windowEnumerator;
	if (NS_FAILED(windowMediator->GetXULWindowEnumerator(nsnull, getter_AddRefs(windowEnumerator))))
		ThrowOSErr(paramErr);			// need a better error

	// Find the window
	while (true)
	{
		PRBool more = false;
		windowEnumerator->HasMoreElements(&more);
		if (!more)
			break;
			
		nsCOMPtr<nsISupports> nextWindow = nsnull;
		windowEnumerator->GetNext(getter_AddRefs(nextWindow));
		nsCOMPtr<nsIBaseWindow> baseWindow(do_QueryInterface(nextWindow));
		if (NULL == baseWindow)
		    continue;
		    
		nsCOMPtr<nsIWidget> widget = nsnull;
		baseWindow->GetMainWidget(getter_AddRefs(widget));
		if (NULL == widget)
		    continue;
		
		WindowRef windowRef = (WindowRef)widget->GetNativeData(NS_NATIVE_DISPLAY);
		if ((WindowPtr)windowRef == inWindowPtr)
		{
			// !!! There really must be an easier way to do this. JavaScript?
			nsCOMPtr<nsIXULWindow> xulWindow(do_QueryInterface(nextWindow));
			if (!xulWindow)
				break;
				
			NS_ADDREF(*outXULWindow = xulWindow);
			return;				
		}		
	}
	
	// if we got here, we didn't find the window
	ThrowOSErr(paramErr);
}


/*----------------------------------------------------------------------------
	GetXULWindowTypeString
	
	Get the type string for a XUL window 
	
----------------------------------------------------------------------------*/
static void GetXULWindowTypeString(nsIXULWindow *inXULWindow, nsString& outWindowType)
{
	outWindowType.Truncate();

	if (inXULWindow)
	{
		nsCOMPtr<nsIDocShellTreeItem>		contentShell;
		inXULWindow->GetPrimaryContentShell(getter_AddRefs(contentShell));
		nsCOMPtr<nsIWebNavigation>			webNav(do_QueryInterface(contentShell));
		ThrowErrIfNil(webNav, paramErr);

		nsCOMPtr<nsIDOMDocument> domDoc;
		webNav->GetDocument(getter_AddRefs(domDoc));
		if (domDoc)
		{
	      nsCOMPtr<nsIDOMElement> element;
	      domDoc->GetDocumentElement(getter_AddRefs(element));
	      if (element)
	        element->GetAttribute(NS_LITERAL_STRING("windowtype"), outWindowType);		
		}
	}
}


/*----------------------------------------------------------------------------
	WindowKindFromTypeString

	
----------------------------------------------------------------------------*/
static TWindowKind WindowKindFromTypeString(const nsString& inWindowType)
{
	if (inWindowType.IsEmpty())
		return kAnyWindowKind;
		
	if (inWindowType.Equals(NS_LITERAL_STRING("navigator:browser")))
		return kBrowserWindowKind;

	if (inWindowType.Equals(NS_LITERAL_STRING("mail:3pane")))
		return kMailWindowKind;

	if (inWindowType.Equals(NS_LITERAL_STRING("msgcompose")))
		return kMailComposeWindowKind;

	if (inWindowType.Equals(NS_LITERAL_STRING("mail:addressbook")))
		return kAddressBookWindowKind;

	if (inWindowType.Equals(NS_LITERAL_STRING("composer:html")))
		return kComposerWindowKind;

	if (inWindowType.Equals(NS_LITERAL_STRING("composer:text")))
		return kComposerWindowKind;

	return kOtherWindowKind;
}


/*----------------------------------------------------------------------------
	GetXULWindowKind

	
----------------------------------------------------------------------------*/
static TWindowKind GetXULWindowKind(nsIXULWindow *inXULWindow)
{
	nsAutoString		windowType;
	GetXULWindowTypeString(inXULWindow, windowType);
	return WindowKindFromTypeString(windowType);
}


/*----------------------------------------------------------------------------
	CountWindowsOfKind 
	
----------------------------------------------------------------------------*/
long CountWindowsOfKind(TWindowKind windowKind)
{
	nsCOMPtr<nsIWindowMediator> windowMediator(do_GetService(kWindowMediatorCID));
	ThrowErrIfNil(windowMediator, paramErr);

	nsCOMPtr<nsISimpleEnumerator> windowEnumerator;
	if (NS_FAILED(windowMediator->GetXULWindowEnumerator(nsnull, getter_AddRefs(windowEnumerator))))
		ThrowOSErr(paramErr);			// need a better error

	long windowCount = 0L;
	PRBool more;
	windowEnumerator->HasMoreElements(&more);
	while (more)
	{
		nsCOMPtr<nsISupports> nextWindow = nsnull;
		windowEnumerator->GetNext(getter_AddRefs(nextWindow));
		nsCOMPtr<nsIXULWindow> xulWindow(do_QueryInterface(nextWindow));
		if (!xulWindow) break;
		
		if (kAnyWindowKind == windowKind)
			++windowCount;
		else
		{
			// Test window kind.
			TWindowKind	thisWindowKind = GetXULWindowKind(xulWindow);
			if (thisWindowKind == windowKind)
		 		++windowCount;
		}
		
		windowEnumerator->HasMoreElements(&more);
	}

	return windowCount;
}


WindowPtr GetNamedOrFrontmostWindow(TWindowKind windowKind, const char* windowName)
{
	// Search for window with the desired kind and name.
	nsCOMPtr<nsIWindowMediator> windowMediator(do_GetService(kWindowMediatorCID));
	ThrowErrIfNil(windowMediator, paramErr);
	
	nsCOMPtr<nsISimpleEnumerator> windowEnumerator;
	if (NS_FAILED(windowMediator->GetXULWindowEnumerator(nsnull, getter_AddRefs(windowEnumerator))))
		ThrowOSErr(paramErr);			// need a better error
   
  WindowPtr windowPtr = NULL;
	PRBool more;
	nsCString windowNameString(windowName);
	windowEnumerator->HasMoreElements(&more);
	while (more)
	{
		nsCOMPtr<nsISupports> nextWindow = nsnull;
		windowEnumerator->GetNext(getter_AddRefs(nextWindow));
		nsCOMPtr<nsIXULWindow> xulWindow(do_QueryInterface(nextWindow));
		if (!xulWindow) break;

		nsCOMPtr<nsIBaseWindow> baseWindow(do_QueryInterface(xulWindow));
		ThrowErrIfNil(baseWindow, paramErr);
		
		nsCOMPtr<nsIWidget> widget = nsnull;
		baseWindow->GetMainWidget(getter_AddRefs(widget));
		ThrowErrIfNil(widget, paramErr);

		WindowRef 	windowRef = (WindowRef)widget->GetNativeData(NS_NATIVE_DISPLAY);
		TWindowKind	thisWindowKind = GetXULWindowKind(xulWindow);
		
		// If this is the kind of window we are looking for...
		if (kAnyWindowKind == windowKind || (thisWindowKind == windowKind))
		{
			if (NULL == windowName)
			{
			    // ...see if its the frontmost of this kind.
			    PRInt32 zIndex;
    		    widget->GetZIndex(&zIndex);
    		    if (0L == zIndex)
    		    {
    		        windowPtr = (WindowPtr)windowRef;
    		        break;
    		    }
    		}
    		else
    		{
    		    // ...see if its name is the desired one.
    		  Str255 pascalTitle;
    		  GetWTitle(windowRef, pascalTitle);   					
    			if (windowNameString.EqualsWithConversion((const char*)&pascalTitle[1], PR_FALSE, pascalTitle[0]))
    			{
    				windowPtr = (WindowPtr)windowRef;		// WindowRef is the WindowPtr.
    				break;
    			}
    		}
		}
	
		windowEnumerator->HasMoreElements(&more);
	}
	
	return windowPtr;
}


WindowPtr GetIndexedWindowOfKind(TWindowKind windowKind, TAEListIndex index)
{
	nsCOMPtr<nsIWindowMediator> windowMediator(do_GetService(kWindowMediatorCID));
	ThrowErrIfNil(windowMediator, paramErr);
	
	nsCOMPtr<nsISimpleEnumerator> windowEnumerator;
	if (NS_FAILED(windowMediator->GetXULWindowEnumerator(nsnull, getter_AddRefs(windowEnumerator))))
		ThrowOSErr(paramErr);			// need a better error
   
  WindowPtr windowPtr = NULL;
	PRBool more;
	windowEnumerator->HasMoreElements(&more);
	while (more)
	{
		nsCOMPtr<nsISupports> nextWindow = nsnull;
		windowEnumerator->GetNext(getter_AddRefs(nextWindow));
		nsCOMPtr<nsIXULWindow> xulWindow(do_QueryInterface(nextWindow));
		if (!xulWindow) break;

		nsCOMPtr<nsIBaseWindow> baseWindow(do_QueryInterface(xulWindow));
		ThrowErrIfNil(baseWindow, paramErr);
		
		nsCOMPtr<nsIWidget> widget = nsnull;
		baseWindow->GetMainWidget(getter_AddRefs(widget));
		ThrowErrIfNil(widget, paramErr);

		WindowRef 	windowRef = (WindowRef)widget->GetNativeData(NS_NATIVE_DISPLAY);
		TWindowKind	thisWindowKind = GetXULWindowKind(xulWindow);
		
		// If this is the kind of window we are looking for...
		if (kAnyWindowKind == windowKind || (thisWindowKind == windowKind))
		{
			// ...decrement index and test if this is the window at that index.
			if (0L == --index)
			{
				windowPtr = (WindowPtr)windowRef;		// WindowRef is the WindowPtr.
				break;
			}
		}
	
		windowEnumerator->HasMoreElements(&more);
	}

	return windowPtr;
}


TAEListIndex GetWindowIndex(TWindowKind windowKind, WindowPtr theWindow)
{
	nsCOMPtr<nsIWindowMediator> windowMediator(do_GetService(kWindowMediatorCID));
	ThrowErrIfNil(windowMediator, paramErr);

	nsCOMPtr<nsISimpleEnumerator> windowEnumerator;
	if (NS_FAILED(windowMediator->GetXULWindowEnumerator(nsnull, getter_AddRefs(windowEnumerator))))
		ThrowOSErr(paramErr);			// need a better error

	TAEListIndex index = 0L;
	
	PRBool more;
	windowEnumerator->HasMoreElements(&more);
	while (more)
	{
		nsCOMPtr<nsISupports> nextWindow = nsnull;
		windowEnumerator->GetNext(getter_AddRefs(nextWindow));
		nsCOMPtr<nsIXULWindow> xulWindow(do_QueryInterface(nextWindow));
		if (!xulWindow) break;

		nsCOMPtr<nsIBaseWindow> baseWindow(do_QueryInterface(xulWindow));
		ThrowErrIfNil(baseWindow, paramErr);
		
		nsCOMPtr<nsIWidget> widget = nsnull;
		baseWindow->GetMainWidget(getter_AddRefs(widget));
		ThrowErrIfNil(widget, paramErr);
		
		WindowRef 	windowRef = (WindowRef)widget->GetNativeData(NS_NATIVE_DISPLAY);
		TWindowKind	thisWindowKind = GetXULWindowKind(xulWindow);

		if (kAnyWindowKind == windowKind || (thisWindowKind == windowKind))
		{
			++index;
			
			if ((WindowPtr)windowRef == theWindow)
				return index;
		}
	
		windowEnumerator->HasMoreElements(&more);
	}

	return 0L; // error: theWindow wasn't found. Return an invalid index.
}


//---------------------------------------------------------
void GetCleanedWindowName(WindowPtr wind, char* outName, long maxLen)
{
	Str255 uncleanName;
	GetWTitle(wind, uncleanName);
	CopyPascalToCString(uncleanName, outName, maxLen);
}

//---------------------------------------------------------
void GetWindowUrlString(WindowPtr wind, char** outUrlStringPtr)
{
	*outUrlStringPtr = NULL;

	nsCOMPtr<nsIXULWindow>		xulWindow;
	GetXULWindowFromWindowPtr(wind, getter_AddRefs(xulWindow));
	ThrowErrIfNil(xulWindow, paramErr);
	
	nsCOMPtr<nsIDocShellTreeItem>		contentShell;
	xulWindow->GetPrimaryContentShell(getter_AddRefs(contentShell));
	nsCOMPtr<nsIWebNavigation>			webNav(do_QueryInterface(contentShell));
	ThrowErrIfNil(webNav, paramErr);

	nsCOMPtr<nsIURI> sourceURL;
	webNav->GetCurrentURI(getter_AddRefs(sourceURL));
	ThrowErrIfNil(sourceURL, paramErr);
	
	// Now get the string;
	sourceURL->GetSpec(outUrlStringPtr);		
}


//---------------------------------------------------------
inline void GetWindowPortRect(WindowPtr wind, Rect *outRect)
{
#if OPAQUE_TOOLBOX_STRUCTS
    ::GetPortBounds(GetWindowPort(wind), outRect);
#else
	*outRect = wind->portRect;
#endif
}

/*----------------------------------------------------------------------------
	LocalToGlobalRect 
	
	Convert a rectangle from local to global coordinates.
			
	Entry:	r = pointer to rectangle.
----------------------------------------------------------------------------*/

static void LocalToGlobalRect (Rect *r)
{
	LocalToGlobal((Point*)&r->top);
	LocalToGlobal((Point*)&r->bottom);
}

void GetWindowGlobalBounds(WindowPtr wind, Rect* outBounds)
{
	GrafPtr	curPort;
	GetWindowPortRect(wind, outBounds);
	GetPort(&curPort);
	SetPortWindowPort(wind);
	LocalToGlobalRect(outBounds);
	SetPort(curPort);
}


/*----------------------------------------------------------------------------
	LoadURLInWindow 

----------------------------------------------------------------------------*/
void LoadURLInWindow(WindowPtr wind, const char* urlString)
{
	OSErr		err = noErr;
	
	if (wind == nil)
	{
		// this makes a new window
		nsMacCommandLine&  cmdLine = nsMacCommandLine::GetMacCommandLine();
		err = cmdLine.DispatchURLToNewBrowser(urlString);
		ThrowIfOSErr(err);
	}

	// existing window. Go through hoops to load a URL in it
	nsCOMPtr<nsIXULWindow>		xulWindow;
	GetXULWindowFromWindowPtr(wind, getter_AddRefs(xulWindow));
	ThrowErrIfNil(xulWindow, paramErr);
	
	nsCOMPtr<nsIDocShellTreeItem>		contentShell;
	xulWindow->GetPrimaryContentShell(getter_AddRefs(contentShell));
	ThrowErrIfNil(contentShell, paramErr);

	nsCOMPtr<nsIWebNavigation>			webNav(do_QueryInterface(contentShell));
	ThrowErrIfNil(webNav, paramErr);

	nsAutoString		urlWString; urlWString.AssignWithConversion(urlString);	
	webNav->LoadURI(urlWString.get(), nsIWebNavigation::LOAD_FLAGS_NONE);

}

#pragma mark -

/*----------------------------------------------------------------------------
	WindowIsResizeable 

----------------------------------------------------------------------------*/

Boolean WindowIsResizeable(WindowPtr wind)
{
	OSStatus		status;
	UInt32		features;
	status = GetWindowFeatures(wind, &features);
	return ((status == noErr) && ((features & kWindowCanGrow) != 0));
}

/*----------------------------------------------------------------------------
	WindowIsZoomable 

----------------------------------------------------------------------------*/
Boolean WindowIsZoomable(WindowPtr wind)
{
	OSStatus		status;
	UInt32		features;
	status = GetWindowFeatures(wind, &features);
	return ((status == noErr) && ((features & kWindowCanZoom) != 0));
}

/*----------------------------------------------------------------------------
	WindowIsZoomed 

----------------------------------------------------------------------------*/
Boolean WindowIsZoomed(WindowPtr wind)
{
	Rect			r, userRect;
	GetWindowUserState(wind, &userRect);
	GetWindowPortRect(wind, &r);
	OffsetRect(&userRect, -userRect.left, -userRect.top);
	return EqualRect(&userRect, &r);
}

/*----------------------------------------------------------------------------
	WindowHasTitleBar 
	
	This stuff only works in 8.0 and later (Appearance Mgr)
----------------------------------------------------------------------------*/
Boolean WindowHasTitleBar(WindowPtr wind)
{
	OSStatus		status;
	UInt32		features;
	status = GetWindowFeatures(wind, &features);
	return ((status == noErr) && ((features & kWindowHasTitleBar) != 0));
}

/*----------------------------------------------------------------------------
	WindowIsCloseable 
	
	This stuff only works in 8.5 and later (Appearance Mgr)
----------------------------------------------------------------------------*/
Boolean WindowIsCloseable(WindowPtr wind)
{
	if ((long)GetWindowAttributes != kUnresolvedCFragSymbolAddress)
	{
		OSStatus		status;
		UInt32		attributes;
		
		status = GetWindowAttributes(wind, &attributes);
		return ((status == noErr) && ((attributes & kWindowCloseBoxAttribute) != 0));
	}
	
	return true;
}

/*----------------------------------------------------------------------------
	WindowIsModal 
	
	This stuff only works in 8.0 and later (Appearance Mgr)
----------------------------------------------------------------------------*/
Boolean WindowIsModal(WindowPtr wind)
{
	OSStatus		status;
	UInt32		features;
	status = GetWindowFeatures(wind, &features);
	return ((status == noErr) && ((features & kWindowIsModal) != 0));
}

/*----------------------------------------------------------------------------
	WindowIsFloating 
	
----------------------------------------------------------------------------*/
Boolean WindowIsFloating(WindowPtr wind)
{
	WindowClass 	windClass;
	if (GetWindowClass(wind, &windClass) == noErr)
	{
		return (windClass == kFloatingWindowClass);
	}

	return false;
}

/*----------------------------------------------------------------------------
	WindowIsModified 
	
----------------------------------------------------------------------------*/
Boolean WindowIsModified(WindowPtr wind)
{
	// ��� write me
	return false;
}
