/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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

////////////////////////////////////////////////////////////////////////////////
/**
 * <B>INTERFACE TO NETSCAPE COMMUNICATOR PLUGINS (NEW C++ API).</B>
 *
 * <P>This superscedes the old plugin API (npapi.h, npupp.h), and 
 * eliminates the need for glue files: npunix.c, npwin.cpp and npmac.cpp that
 * get linked with the plugin. You will however need to link with the "backward
 * adapter" (badapter.cpp) in order to allow your plugin to run in pre-5.0
 * browsers. 
 *
 * <P>See nsplugin.h for an overview of how this interface fits with the 
 * overall plugin architecture.
 */
////////////////////////////////////////////////////////////////////////////////

#ifndef nsIPluginManager2_h___
#define nsIPluginManager2_h___

#include "nsIPluginManager.h"

////////////////////////////////////////////////////////////////////////////////
// Plugin Manager 2 Interface
// These extensions to nsIPluginManager are only available in Communicator 5.0.

class nsIPluginManager2 : public nsIPluginManager {
public:

    /**
     * Puts up a wait cursor.
     *
     * @result - NS_OK if this operation was successful
     */
    NS_IMETHOD
    BeginWaitCursor(void) = 0;

    /**
     * Restores the previous (non-wait) cursor.
     *
     * @result - NS_OK if this operation was successful
     */
    NS_IMETHOD
    EndWaitCursor(void) = 0;

    /**
     * Returns true if a URL protocol (e.g. "http") is supported.
     *
     * @param protocol - the protocol name
     * @param result - true if the protocol is supported
     * @result - NS_OK if this operation was successful
     */
    NS_IMETHOD
    SupportsURLProtocol(const char* protocol, PRBool *result) = 0;

    /**
     * This method may be called by the plugin to indicate that an error 
     * has occurred, e.g. that the plugin has failed or is shutting down 
     * spontaneously. This allows the browser to clean up any plugin-specific 
     * state.
     *
     * @param plugin - the plugin whose status is changing
     * @param errorStatus - the the error status value
     * @result - NS_OK if this operation was successful
     */
    NS_IMETHOD
    NotifyStatusChange(nsIPlugin* plugin, nsresult errorStatus) = 0;
    
    ////////////////////////////////////////////////////////////////////////////
    // New top-level window handling calls for Mac:
    
    /**
     * Registers a top-level window with the browser. Events received by that
     * window will be dispatched to the event handler specified.
     * 
     * @param handler - the event handler for the window
     * @param window - the platform window reference
     * @result - NS_OK if this operation was successful
     */
    NS_IMETHOD
    RegisterWindow(nsIEventHandler* handler, nsPluginPlatformWindowRef window) = 0;
    
    /**
     * Unregisters a top-level window with the browser. The handler and window pair
     * should be the same as that specified to RegisterWindow.
     *
     * @param handler - the event handler for the window
     * @param window - the platform window reference
     * @result - NS_OK if this operation was successful
     */
    NS_IMETHOD
    UnregisterWindow(nsIEventHandler* handler, nsPluginPlatformWindowRef window) = 0;

	/**
     * Allocates a new menu ID (for the Mac).
     *
     * @param handler - the event handler for the window
     * @param isSubmenu - whether this is a sub-menu ID or not
     * @param result - the resulting menu ID
     * @result - NS_OK if this operation was successful
     */
    NS_IMETHOD
	AllocateMenuID(nsIEventHandler* handler, PRBool isSubmenu, PRInt16 *result) = 0;

	/**
     * Deallocates a menu ID (for the Mac).
     *
     * @param handler - the event handler for the window
     * @param menuID - the menu ID
     * @result - NS_OK if this operation was successful
     */
    NS_IMETHOD
	DeallocateMenuID(nsIEventHandler* handler, PRInt16 menuID) = 0;

	/**
	 * Indicates whether this event handler has allocated the given menu ID.
     *
     * @param handler - the event handler for the window
     * @param menuID - the menu ID
     * @param result - returns PR_TRUE if the menu ID is allocated
     * @result - NS_OK if this operation was successful
     */
    NS_IMETHOD
    HasAllocatedMenuID(nsIEventHandler* handler, PRInt16 menuID, PRBool *result);

	/**
     * This operation causes the next browser event to be processed. This is
     * handy for implement nested event loops where some other activity must
     * be performed each time around the loop. 
     *
     * On the Mac (and most likely on Win16), network activity can only occur on
     * the main thread. Therefore, we provide a hook here for the case that the
     * main thread needs to process events while waiting for network activity to
     * complete.
     *
     * @param result - a boolean indicating whether an event was processed on the
     * main thread. If not on the main browser thread, PR_FALSE is returned.
     * @result - NS_OK if this operation was successful
     */
    NS_IMETHOD
    ProcessNextEvent(PRBool *result) = 0;

};

#define NS_IPLUGINMANAGER2_IID                       \
{ /* 29c4ae70-019a-11d2-815b-006008119d7a */         \
    0x29c4ae70,                                      \
    0x019a,                                          \
    0x11d2,                                          \
    {0x81, 0x5b, 0x00, 0x60, 0x08, 0x11, 0x9d, 0x7a} \
}

////////////////////////////////////////////////////////////////////////////////

#endif /* nsIPluginManager2_h___ */
