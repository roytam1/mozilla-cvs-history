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

#ifndef nsIJRILiveConnectPlugin_h__
#define nsIJRILiveConnectPlugin_h__

#include "nsIPlugin.h"

////////////////////////////////////////////////////////////////////////////////
// JRI-Based LiveConnect Classes
////////////////////////////////////////////////////////////////////////////////
//
// This stuff is here so that the browser can support older JRI-based
// LiveConnected plugins (when using old plugin to new C++-style plugin
// adapter code). 
//
// Warning: Don't use this anymore, unless you're sure that you have to!
////////////////////////////////////////////////////////////////////////////////

#include "jri.h"

/** 
 * The nsIJRILiveConnectPlugin interface defines additional entry points that a
 * plugin developer needs to implement in order for the plugin to support 
 * JRI-based LiveConnect, as opposed to the standard JNI-based LiveConnect
 * (which new in 5.0).
 *
 * Plugin developers requiring this capability should implement this interface
 * in addition to the basic nsIPlugin interface.
 */
class nsIJRILiveConnectPlugin : public nsIPlugin {
public:

    /**
     * Returns the class of the Java instance to be associated with the
     * plugin.
     *
     * (Corresponds to NPP_GetJavaClass.)
     *
     * @param resultingClass - a resulting reference to the Java class
     * @result - NS_OK if this operation was successful
     */
    NS_IMETHOD
    GetJavaClass(jref *resultingClass) = 0;

};

#define NS_IJRILIVECONNECTPLUGIN_IID                 \
{ /* c94058e0-f772-11d1-815b-006008119d7a */         \
    0xc94058e0,                                      \
    0xf772,                                          \
    0x11d1,                                          \
    {0x81, 0x5b, 0x00, 0x60, 0x08, 0x11, 0x9d, 0x7a} \
}

////////////////////////////////////////////////////////////////////////////////

#endif /* nsIJRILiveConnectPlugin_h__ */
