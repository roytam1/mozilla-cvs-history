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
 *  Bill Law    <law@netscape.com>
 */
#ifndef nsINativeAppSupport_h__
#define nsINativeAppSupport_h__
 
#include "nsISupports.h"
#include "nsISplashScreen.h"

// 5fdf8480-1f98-11d4-8077-00600811a9c3
#define NS_INATIVEAPPSUPPORT_IID \
    { 0x5fdf8480, 0x1f98, 0x11d4, { 0x80, 0x77, 0x00, 0x60, 0x08, 0x11, 0xa9, 0xc3 } }

/* nsINativeAppSupport
 *
 * This "pseudo" (in the XPCOM sense) interface provides for
 * platform-specific general aplication support:
 *  o It subsumes the old "NS_CanRun" and "NS_CreateSplashScreen" 
 *    functions that managed display of the application splash 
 *    screen at startup.
 *  o It manages the details of the simple DDE communication 
 *    supported on the Win32 platform (it is the addition of this 
 *    item that prompted the creation of this interface.
 *
 * Due to the nature of the beast, this interface is not a full-blown
 * XPCOM component.  The primary reason is that objects that implement
 * this interface generally must be operational *before* XPCOM (or any
 * of the rest of Mozilla) are initialized.  As a result, this 
 * interface is instantiated by somewhat unconventional means.
 *
 * To create the implementor of this interface, you call the function
 * NS_CreateNativeAppSupport.
 *
 * The interface provides these functions:
 *  Start - You call this to inform the native app support that the  
 *          application is starting.  In addition, it serves as a
 *          query as to whether the application should continue to
 *          run.  In that respect, it is rougly equivalent to the
 *          NS_CanStart function, which it replaces.
 *
 *          If the returned boolean result is PR_FALSE, then the
 *          application should exit without further processing.  In
 *          such cases, the returned nsresult indicates whether the
 *          reason to exit is due to an error or not.
 *
 *          Win32 Note: In the case of starting a second instance
 *                      of this executable, this function will return
 *                      PR_FALSE and nsresult==NS_OK.  This means that
 *                      the command line arguments have been
 *                      successfully passed to the instance of the
 *                      application acting as a DDE server.
 *
 *  Stop - You call this to inform the native app support that the
 *         application *wishes* to terminate.  If the returned boolean
 *         value is PR_FALSE, then the application should continue
 *         (as if there were still additional top-level windows open).
 *         
 *         Win32 Note: If this is the instance of the application
 *                     acting as the DDE server, and there are current
 *                     DDE conversations active with other instances
 *                     acting as DDE clients, then this function will
 *                     return PR_FALSE.
 * 
 *  Quit - Like Stop, but this method *forces* termination (or more 
 *         precisely, indicates that the application is about to be
 *         terminated regardless of what a call to Stop might have
 *         returned.
 *
 *         This method is intended to be called when the user selects
 *         the "Quit" option (close all windows and exit).
 *
 *         Win32 Note: Stop is problematic in the case of "Quit" (close
 *                     all windows and exit the application) because
 *                     either we don't Quit or (potentially) we lose
 *                     requests coming from other instances of the
 *                     application.  The strategy is to give preference
 *                     to the user's explicit Quit request.  In the
 *                     unlikely event that a request is pending from
 *                     another instance of the application, then such
 *                     requests are essentially ignored.  This is
 *                     roughly equivalent to handling that request by
 *                     opening a new window, followed by immediately
 *                     closing it.  Since this is the same as if the
 *                     request came in immediately before the Quit
 *                     call (versus immediately after it), no harm.
 *
 *                     There is an exposure here: Upon return from this
 *                     function, any DDE connect request (for Mozilla)
 *                     will fail and other instances of the application
 *                     will start up as a DDE server.  In that case,
 *                     those instances may do things that conflict with
 *                     the subsequent shutting down of the instance that
 *                     is quitting.  For this reason, the call to Quit
 *                     should be deferred as long as possible.
 *
 *  ShowSplashScreen - Causes the platform-specific splash screen to be
 *                     displayed.  This is a replacement for the old
 *                     method of invoking the Show() method on the
 *                     nsISplashScreen interface obtained by calling
 *                     NS_CreateSplashScreen.
 *
 *  HideSplashScreen - Causes the splash screen to be removed (if it is
 *                     being shown).  This replaces the old method of
 *                     invoking the Hide() method on the nsISplashScreen
 *                     interface maintained by the app shell service.
 *
 */
class nsINativeAppSupport : public nsISupports {
public:
    NS_DEFINE_STATIC_IID_ACCESSOR( NS_INATIVEAPPSUPPORT_IID )

    // Startup/shutdown.
    NS_IMETHOD Start( PRBool *result ) = 0;
    NS_IMETHOD Stop( PRBool *result ) = 0;
    NS_IMETHOD Quit() = 0;

    // Splash screen functions.
    NS_IMETHOD ShowSplashScreen() = 0;
    NS_IMETHOD HideSplashScreen() = 0;

}; // class nsINativeAppSupport

#endif // nsINativeAppSupport_h__
