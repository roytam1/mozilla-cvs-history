/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
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
 * Copyright (C) 1999 Netscape Communications Corporation.  All
 * Rights Reserved.
 * 
 * Contributor(s): Martijn Pieters <mj@digicool.com>
 */

/*
 * -jsconsole commandline handler; starts up the JavaScript console.
 */

/*
 * Constants
 */

const JSCONSOLEHANDLER_PROGID =
    "component://netscape/commandlinehandler/general-startup-jsconsole";

const JSCONSOLEHANDLER_CID = 
    Components.ID('{1698ef18-c128-41a1-b4d0-7f9acd2ae86c}');

/*
 * Classes
 */

/* jsConsoleHandler class constructor */
function jsConsoleHandler() {}

/* jsConsoleHandler class def */
jsConsoleHandler.prototype = {
    commandLineArgument: '-jsconsole',
    prefNameForStartup: 'general.startup.jsconsole',
    chromeUrlForTask: 'chrome://global/content/console.xul',
    helpText: 'Start with Javascript Console',
    handlesArgs: false,
    defaultArgs: null,
    openWindowWithArgs: false
};

/*
 * Objects
 */

/* jsConsoleHandler Module (for XPCOM registration) */
var jsConsoleHandlerModule = {
    registerSelf: function(compMgr, fileSpec, location, type) {
        compMgr.registerComponentWithType(JSCONSOLEHANDLER_CID, 
            'JS Console Commandline Handler component',
            JSCONSOLEHANDLER_PROGID, fileSpec,
            location, true, true, type);
        var catman = Components.classes["mozilla.categorymanager.1"]
            .getService(Components.interfaces.nsICategoryManager);
        catman.addCategoryEntry("command-line-argument-handlers",
            JSCONSOLEHANDLER_PROGID, "jsconsole command line handler",
            true, true);
    },

    unregisterSelf: function(compMgr, fileSpec, location) {
        compMgr.unregisterComponentSpec(JSCONSOLEHANDLER_CID, fileSpec);
        var catman = Components.classes["mozilla.categorymanager.1"]
            .getService(Components.interfaces.nsICategoryManager);
        catman.deleteCategoryEntry("command-line-argument-handlers",
            JSCONSOLEHANDLER_PROGID, true);
    },

    getClassObject: function(compMgr, cid, iid) {
        if (!cid.equals(JSCONSOLEHANDLER_CID))
            throw Components.results.NS_ERROR_NO_INTERFACE;

        if (!iid.equals(Components.interfaces.nsIFactory))
            throw Components.results.NS_ERROR_NOT_IMPLEMENTED;

        return jsConsoleHandlerFactory;
    },

    canUnload: function(compMgr) { return true; }
};

/* jsConsoleHandler Class Factory */
var jsConsoleHandlerFactory = {
    CreateInstance: function(outer, iid) {
        if (outer != null)
            throw Components.results.NS_ERROR_NO_AGGREGATION;
    
        if (!iid.equals(Components.interfaces.nsICmdLineHandler) &&
            !iid.equals(Components.interfaces.nsISupports))
            throw Components.results.NS_ERROR_INVALID_ARG;

        return new jsConsoleHandler();
    }
}

/*
 * Functions
 */

/* module initialisation */
function NSGetModule(comMgr, fileSpec) { return jsConsoleHandlerModule; }

// vim:sw=4:sr:sta:et:sts:
