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

#ifndef nsIPlugin_h___
#define nsIPlugin_h___

#include "nsplugindefs.h"
#include "nsIFactory.h"

// {df773070-0199-11d2-815b-006008119d7a}
#define NS_IPLUGIN_IID \
{ 0xdf773070, 0x0199, 0x11d2, { 0x81, 0x5b, 0x00, 0x60, 0x08, 0x11, 0x9d, 0x7a } }

// {ffc63200-cf09-11d2-a5a0-bc8f7ad21dfc}
#define NS_PLUGIN_CID \
{ 0xffc63200, 0xcf09, 0x11d2, { 0xa5, 0xa0, 0xbc, 0x8f, 0x7a, 0xd2, 0x1d, 0xfc } }

/**
 * The nsIPlugin interface is the minimum interface plugin developers need to 
 * support in order to implement a plugin. The plugin manager may QueryInterface 
 * for more specific plugin types, e.g. nsILiveConnectPlugin.
 *
 * The old NPP_New plugin operation is now subsumed by two operations:
 *
 * CreateInstance -- called once, after the plugin instance is created. This 
 * method is used to initialize the new plugin instance (although the actual
 * plugin instance object will be created by the plugin manager).
 *
 * nsIPluginInstance::Start -- called when the plugin instance is to be
 * started. This happens in two circumstances: (1) after the plugin instance
 * is first initialized, and (2) after a plugin instance is returned to
 * (e.g. by going back in the window history) after previously being stopped
 * by the Stop method. 
 */
class nsIPlugin : public nsIFactory {
public:
	NS_DEFINE_STATIC_IID_ACCESSOR(NS_IPLUGIN_IID)

	/**
	 * Creates a new plugin instance, based on a MIME type. This
	 * allows different impelementations to be created depending on
	 * the specified MIME type.
	 */
    NS_IMETHOD CreatePluginInstance(nsISupports *aOuter, REFNSIID aIID, 
                                    const char* aPluginMIMEType,
                                    void **aResult) = 0;

    /**
     * Initializes the plugin and will be called before any new instances are
     * created. It is passed browserInterfaces on which QueryInterface
     * may be used to obtain an nsIPluginManager, and other interfaces.
     *
     * @param browserInterfaces - an object that allows access to other browser
     * interfaces via QueryInterface
     * @result - NS_OK if this operation was successful
     */
    NS_IMETHOD
    Initialize() = 0;

    /**
     * Called when the browser is done with the plugin factory, or when
     * the plugin is disabled by the user.
     *
     * (Corresponds to NPP_Shutdown.)
     *
     * @result - NS_OK if this operation was successful
     */
    NS_IMETHOD
    Shutdown(void) = 0;

    /**
     * Returns the MIME description for the plugin. The MIME description 
     * is a colon-separated string containg the plugin MIME type, plugin
     * data file extension, and plugin name, e.g.:
     *
     * "application/x-simple-plugin:smp:Simple LiveConnect Sample Plug-in"
     *
     * (Corresponds to NPP_GetMIMEDescription.)
     *
     * @param resultingDesc - the resulting MIME description 
     * @result - NS_OK if this operation was successful
     */
    NS_IMETHOD
    GetMIMEDescription(const char* *resultingDesc) = 0;

    /**
     * Returns the value of a variable associated with the plugin.
     *
     * (Corresponds to NPP_GetValue.)
     *
     * @param variable - the plugin variable to get
     * @param value - the address of where to store the resulting value
     * @result - NS_OK if this operation was successful
     */
    NS_IMETHOD
    GetValue(nsPluginVariable variable, void *value) = 0;
};

////////////////////////////////////////////////////////////////////////////////

#endif /* nsIPlugin_h___ */
