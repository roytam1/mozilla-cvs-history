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
#ifndef MacWindow_h__
#define MacWindow_h__

#include <memory>	// for auto_ptr
#include "nsIKBStateControl.h"

#include "nsWindow.h"
#include "nsMacEventHandler.h"

class nsMacEventHandler;

//-------------------------------------------------------------------------
//
// nsMacWindow
//
//-------------------------------------------------------------------------
//	MacOS native window

class nsMacWindow : public nsChildWindow, public nsIKBStateControl
{
private:
	typedef nsChildWindow Inherited;

public:
    nsMacWindow();
    virtual ~nsMacWindow();
	// nsISupports
	NS_IMETHOD_(nsrefcnt) AddRef();
	NS_IMETHOD_(nsrefcnt) Release();
	NS_IMETHOD QueryInterface(const nsIID& aIID, void** aInstancePtr);

/*
    // nsIWidget interface
    NS_IMETHOD            Create(nsIWidget *aParent,
                                     const nsRect &aRect,
                                     EVENT_CALLBACK aHandleEventFunction,
                                     nsIDeviceContext *aContext,
                                     nsIAppShell *aAppShell = nsnull,
                                     nsIToolkit *aToolkit = nsnull,
                                     nsWidgetInitData *aInitData = nsnull);
*/
    NS_IMETHOD              Create(nsNativeWidget aParent,
                                     const nsRect &aRect,
                                     EVENT_CALLBACK aHandleEventFunction,
                                     nsIDeviceContext *aContext,
                                     nsIAppShell *aAppShell = nsnull,
                                     nsIToolkit *aToolkit = nsnull,
                                     nsWidgetInitData *aInitData = nsnull);

     // Utility method for implementing both Create(nsIWidget ...) and
     // Create(nsNativeWidget...)

    virtual nsresult        StandardCreate(nsIWidget *aParent,
				                            const nsRect &aRect,
				                            EVENT_CALLBACK aHandleEventFunction,
				                            nsIDeviceContext *aContext,
				                            nsIAppShell *aAppShell,
				                            nsIToolkit *aToolkit,
				                            nsWidgetInitData *aInitData,
				                            nsNativeWidget aNativeParent = nsnull);

    NS_IMETHOD              Show(PRBool aState);
    NS_IMETHOD            	Move(PRInt32 aX, PRInt32 aY);
    NS_IMETHOD            	Resize(PRInt32 aWidth,PRInt32 aHeight, PRBool aRepaint);
    virtual PRBool          OnPaint(nsPaintEvent &event);

		NS_IMETHOD              SetTitle(const nsString& aTitle);

		virtual PRBool					HandleOSEvent(
																		EventRecord&		aOSEvent);

		virtual PRBool					HandleMenuCommand(
																		EventRecord&		aOSEvent,
																		long						aMenuResult);

		// be notified that a some form of drag event needs to go into Gecko
	virtual PRBool 			DragEvent ( unsigned int aMessage, Point aMouseGlobal, UInt16 aKeyModifiers ) ;

  	// nsIKBStateControl interface
  	NS_IMETHOD ResetInputState();
    NS_IMETHOD PasswordFieldInit();

protected:

	pascal static OSErr DragTrackingHandler ( DragTrackingMessage theMessage, WindowPtr theWindow, 
										void *handlerRefCon, DragReference theDrag );
	pascal static OSErr DragReceiveHandler (WindowPtr theWindow,
												void *handlerRefCon, DragReference theDragRef) ;
	static DragTrackingHandlerUPP sDragTrackingHandlerUPP;
	static DragReceiveHandlerUPP sDragReceiveHandlerUPP;


	
	PRBool							mWindowMadeHere; // true if we created the window
	PRBool							mIsDialog;       // true if the window is a dialog
	auto_ptr<nsMacEventHandler>		mMacEventHandler;
	nsWindowType 					mWindowType;     // normal,pop up, dialog, etc
	nsIWidget                      *mOffsetParent;
	PRBool                          mAcceptsActivation;
};

#endif // MacWindow_h__
