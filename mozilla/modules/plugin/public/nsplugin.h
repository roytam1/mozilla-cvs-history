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
 * <P>This is the master header file that includes most of the other headers
 * you'll need to write a plugin.
 */
////////////////////////////////////////////////////////////////////////////////

/**
 * The following diagram depicts the relationships between various objects 
 * implementing the new plugin interfaces. QueryInterface can be used to switch
 * between interfaces in the same box:
 *
 *
 *         the plugin (only 1)                        
 *  +----------------------+                                             
 *  | nsIPlugin or         |<- - - - - -NSGetFactory()
 *  | nsILiveConnectPlugin |                                             
 *  +----------------------+                                            
 *    |
 *    |                                                                  
 *    |              instances (many)             streams to receive URL data (many)
 *    |          +-------------------+                  +-----------------+         
 *    |          | nsIPluginInstance |+                 | nsIPluginStream |+        
 *    |          |                   ||                 |                 ||                 
 *    |          +-------------------+|                 +-----------------+|                 
 *    |            +------|-----------+                   +------|---------+                  
 *    |                   |                                      |
 *    | PLUGIN SIDE       |peer                                  |peer                
 *~~~~|~~~~~~~~~~~~~~~~~~~|~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~|~~~~~~~~~~~~~~~~
 *    | BROWSER SIDE      |                                      |
 *    |                   v                                      v               
 *    |     +---------------------------------+    +----------------------------+
 *    |     | nsIPluginInstancePeer           |+   | nsIPluginStreamPeer        |+
 *    |     | nsIPluginInstancePeer2          ||   | nsISeekablePluginStreamPeer||      
 *    |     | nsIWindowlessPluginInstancePeer ||   | nsIPluginstreamPeer2       ||
 *    |     | nsILiveConnectPluginInstancePeer||   +----------------------------+|                                        
 *    |     | nsIPluginTagInfo                ||     +---------------------------+ 
 *    |     | nsIPluginTagInfo2               ||                  
 *    |     +---------------------------------+|                  
 *    |       +--------------------------------+                  
 *    |                                                
 *    |                                                
 *    v    the browser (only 1)                             
 *  +---------------------+                                        
 *  | nsIPluginManager    |                                        
 *  | nsIPluginManager2   |                                        
 *  | nsIFileUtilities    |                                        
 *  | nsIPref             |                                        
 *  | nsICacheManager ... |                            
 *  +---------------------+                                        
 *
 */ 

#ifndef nsplugins_h___
#define nsplugins_h___

#include "nsRepository.h"       // for NSGetFactory

////////////////////////////////////////////////////////////////////////////////
/**
 * <B>Interfaces which must be implemented by a plugin</B>
 * These interfaces have NPP equivalents in pre-5.0 browsers (see npapi.h).
 */
////////////////////////////////////////////////////////////////////////////////

/**
 * NSGetFactory is the main entry point to the plugin's DLL. The plugin manager
 * finds this symbol and calls it to create the plugin class. Once the plugin 
 * object is returned to the plugin manager, instances on the page are created 
 * by calling nsIPlugin::CreateInstance.
 */
// (Declared in nsRepository.h)
//extern "C" NS_EXPORT nsresult NSGetFactory(const nsCID &aClass,
//                                           nsIFactory **aFactory);

/**                                                               
 * A plugin object is used to create new plugin instances. It manages the
 * global state of all plugins with the same implementation.      
 */                                                               
#include "nsIPlugin.h"

/**
 * A plugin instance represents a particular activation of a plugin on a page.
 */
#include "nsIPluginInstance.h"

/**
 * A plugin stream gets instantiated when a plugin instance receives data from
 * the browser. 
 */
#include "nsIPluginStream.h"

/**
 * The nsILiveConnectPlugin interface provides additional operations that a 
 * plugin must implement if it is to be controlled by JavaScript through 
 * LiveConnect. 
 *
 * Note that this interface is part of a new JNI-based LiveConnect
 * implementation and superceeds that provided prior to Communicator 5.0.
 */
#include "nsILiveConnectPlugin.h"

////////////////////////////////////////////////////////////////////////////////
/**
 * <B>Interfaces implemented by the browser:
 * These interfaces have NPN equivalents in pre-5.0 browsers (see npapi.h).
 */
////////////////////////////////////////////////////////////////////////////////

/**
 * The plugin manager which is the main point of interaction with the browser 
 * and provides general operations needed by a plugin.
 */
#include "nsIPluginManager.h"

/**
 * A plugin instance peer gets created by the browser and associated with each
 * plugin instance to represent tag information and other callbacks needed by
 * the plugin instance.
 */
#include "nsIPluginInstancePeer.h"

/**
 * The nsIPluginTagInfo interface provides information about the html tag
 * that was used to instantiate the plugin instance. 
 *
 * To obtain: QueryInterface on nsIPluginInstancePeer
 */
#include "nsIPluginTagInfo.h"

/**
 * The nsIWindowlessPluginInstancePeer provides additional operations for 
 * windowless plugins. 
 *
 * To obtain: QueryInterface on nsIPluginInstancePeer
 */
#include "nsIWindowlessPlugInstPeer.h"

/**
 * A plugin stream peer gets create by the browser and associated with each
 * plugin stream to represent stream and URL information, and provides
 * other callbacks needed by the plugin stream.
 */
#include "nsIPluginStreamPeer.h"

/**
 * The nsISeekablePluginStreamPeer provides additional operations for seekable
 * plugin streams. 
 *
 * To obtain: QueryInterface on nsIPluginStreamPeer
 */
#include "nsISeekablePluginStreamPeer.h"

////////////////////////////////////////////////////////////////////////////////
/**
 * <B>Interfaces implemented by the browser (new for 5.0):
 */
////////////////////////////////////////////////////////////////////////////////

/**
 * The nsIPluginManager2 interface provides additional plugin manager features
 * only available in Communicator 5.0. 
 *
 * To obtain: QueryInterface on nsIPluginManager
 */
#include "nsIPluginManager2.h"

/**
 * The nsIFileUtilities interface provides operations to manage temporary
 * files and directories.
 *
 * To obtain: QueryInterface on nsIPluginManager
 */
#include "nsIFileUtilities.h"

/**
 * The nsILiveConnectPluginInstancePeer allows plugins to be manipulated
 * by JavaScript, providing basic scriptability.
 *
 * Note that this interface is part of a new JNI-based LiveConnect
 * implementation and superceeds that provided prior to Communicator 5.0.
 *
 * To obtain: QueryInterface on nsIPluginInstancePeer
 */
#include "nsILiveConnectPlugInstPeer.h"

/**
 * The nsIPluginStreamPeer2 interface provides additional plugin stream
 * peer features only available in Communicator 5.0. 
 *
 * To obtain: QueryInterface on nsIPluginStreamPeer
 */
#include "nsIPluginStreamPeer2.h"

/**
 * The nsIPluginTagInfo2 interface provides additional html tag information
 * only available in Communicator 5.0. 
 *
 * To obtain: QueryInterface on nsIPluginTagInfo
 */
#include "nsIPluginTagInfo2.h"

////////////////////////////////////////////////////////////////////////////////
#endif // nsplugins_h___
