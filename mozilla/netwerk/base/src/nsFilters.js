/* -*- Mode: Java; tab-width: 4; c-basic-offset: 4; -*-
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

/*
   Script for simple URI based filtering.
   To add a new filter scroll down to the 
   last function (FilterMain) in this file. 
   
       - Gagan Saksena 05/04/00 
*/

const kFILTERS_PROGID = "component://mozilla/network/filters";
const kFILTERS_CID = Components.ID("{4677ea1e-1dd2-11b2-8654-e836efb6995c}");
const nsIWebFilters = Components.interfaces.nsIWebFilters;
const nsIURI = Components.interfaces.nsIURI;

function debug(msg)
{
    dump(msg + "\n");
}

function nsFilterManager() {};

nsFilterManager.prototype = {

    allowLoading: function(uri) {
        return FilterMain(uri);
    }
}

var filterModule = new Object();

filterModule.registerSelf = 
    function (compMgr, fileSpec, location, type) {
        dump("*** Registering Web Filters (a Javascript module!)\n");
        compMgr.registerComponentWithType(kFILTERS_CID,
            "Javascript Web Filters",
            kFILTERS_PROGID,
            fileSpec, location, 
            true, true, type);
    }

filterModule.getClassObject =
    function (compMgr, cid, iid) {
        if (!cid.equals(kFILTERS_CID))
            throw Components.results.NS_ERROR_NO_INTERFACE;

        if (!iid.equals(Components.interfaces.nsIFactory))
            throw Components.results.NS_ERROR_NOT_IMPLEMENTED;

        return filtersFactory;
    }

filterModule.canUnload =
    function(compMgr) {
        dump("*** Unloading Web Filters...\n");
        return true;
    }

var filtersFactory = new Object();
filtersFactory.createInstance =
    function (outer, iid) {
        if (outer != null)
            throw Components.results.NS_ERROR_NO_AGGREGATION;

        if (!iid.equals(nsIWebFilters) &&
                !iid.equals(Components.interfaces.nsISupports)) {
            // shouldn't this be NO_INTERFACE?
            throw Components.results.NS_ERROR_INVALID_ARG;
        }
        return FilterMan;
    }

function NSGetModule(compMgr, fileSpec) {
    return filterModule;
}

var FilterMan= new nsFilterManager();

// -----------------------------------------------------------------
// Sample block hosts list...
var blockHosts = new Array(
    "ad.linkexchange.com",
    ".tv.com",
    "doubleclick.net"
);

// The main filter function! Use the arrays above to build more...
// @return true if you want it to load. 
// @return false if you don't like that uri.
function FilterMain(url) {

    uri = url.QueryInterface(nsIURI);

    try {
        /* Commented since currently only HTTP asks for filters...

        // Always let the non-relevant ones go...! 
        // otherwise it would mess up your skin (chrome/resource and such...)
        if ((uri.scheme).search("http") == -1)
            return true;
        */

        // Sample host matches- uri.host
        for (var i=0; i < blockHosts.length; ++i) {
            if (String(uri.host).search(blockHosts[i]) != -1)
                return false;
        }

    }
    catch (ex) { // any problems just continue
        return true;
    }
    return true;
}
