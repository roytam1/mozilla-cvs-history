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
// NETSCAPE JAVA VM PLUGIN EXTENSIONS
// 
// This interface allows a Java virtual machine to be plugged into
// Communicator to implement the APPLET tag and host applets.
// 
// Note that this is the C++ interface that the plugin sees. The browser
// uses a specific implementation of this, nsJVMPlugin, found in jvmmgr.h.
////////////////////////////////////////////////////////////////////////////////

#ifndef nsIJVMPrefsWindow_h___
#define nsIJVMPrefsWindow_h___

#include "nsIJVMWindow.h"

////////////////////////////////////////////////////////////////////////////////
// JVM Preferences Window Interface
// This interface defines the API the browser needs to show and hide the JVM's
// preference Window. The JVM's preference Window is used by the plugin to display
// Java VM-specific preferences.

class nsIJVMPrefsWindow : public nsIJVMWindow {
public:
    
    // QueryInterface on nsIJVMPlugin to get this.

    // No additional methods yet.

};

#define NS_IJVMPREFSWINDOW_IID                       \
{ /* 20330d70-4ec9-11d2-8164-006008119d7a */         \
    0x20330d70,                                      \
    0x4ec9,                                          \
    0x11d2,                                          \
    {0x81, 0x64, 0x00, 0x60, 0x08, 0x11, 0x9d, 0x7a} \
}

#define NS_JVMPREFSWINDOW_CID                        \
{ /* e9c1ef10-6304-11d2-8164-006008119d7a */         \
    0xe9c1ef10,                                      \
    0x6304,                                          \
    0x11d2,                                          \
    {0x81, 0x64, 0x00, 0x60, 0x08, 0x11, 0x9d, 0x7a} \
}

////////////////////////////////////////////////////////////////////////////////

#endif /* nsIJVMPrefsWindow_h___ */
