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
 * The Original Code is Mozilla Communicator client code, released March
 * 31, 1998.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation. Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 * Seth Spitzer <sspitzer@netscape.com>
 * Shane Culpepper <pepper@netscape.com>
 */

var remoteControlProgID	= "component://netscape/browser/remote-browser-control";

var nsIRemoteBrowserControl = Components.interfaces.nsIRemoteBrowserControl;

function BrowserRemoteControl() {
  return browserRemoteControl;
}

// We need to implement nsIRemoteBrowserControl
	
var browserRemoteControl = {
    openURL: function(aURL, newWindow)
    {
        dump("openURL(" + aURL + "," + newWindow + ")\n");
        return;
    },
    
    openFile: function(aURL)
    {
        dump("openFile(" + aURL + ")\n");
        return;
    },

    saveAs: function(aURL)
    {
        dump("saveAs(" + aURL + ")\n");
        var saveAsDialog = Components.classes["component://xfer"].getService();
        saveAsDialog = saveAsDialog.QueryInterface(Components.interfaces.nsIMsgComposeService);
        if ( !saveAsDialog ) return(false);
       
        saveAsDialog.SelectFileAndTransferLocation(aURL, null); 
        return(true);
    },

    mailTo: function(mailToList)
    {
        dump("mailto(" + mailToList + ")\n");
        var msgComposeService = Components.classes["component://netscape/messengercompose"].getService();
        msgComposeService = msgComposeService.QueryInterface(Components.interfaces.nsIMsgComposeService);
        if ( !msgComposeService ) return(false);

        if ( mailToList )
        {
            msgComposeService.OpenComposeWindowWithValues(null,
                                                          Components.interfaces.nsIMsgCompType.New,
                                                          Components.interfaces.nsIMsgCompFormat.Default,
                                                          mailToList,
                                                          null,
                                                          null,
                                                          null,
                                                          null,
                                                          null,
                                                          null,
                                                          null);
        }
        else
        {
            msgComposeService.OpenComposeWindow(null,
                                                null,
                                                Components.interfaces.nsIMsgCompType.New,
                                                Components.interfaces.nsIMsgCompFormat.Default,
                                                null);
        }
        return(true);
    },

    addBookmark: function(aURL, aTitle)
    {
        dump("addBookmark(" + aURL + "," + aTitle + ")\n");
        var bookmarkService = Components.classes["component://netscape/browser/bookmarks-service"].getService();
        bookmarkService = bookmarkService.QueryInterface(Components.interfaces.nsIBookmarksService);
        if ( !bookmarkService ) return(false);

        if ( !aURL ) return(false);
        if ( aTitle )
        {
            bookmarkService.AddBookmark(aURL, aTitle);
        }
        else
        {
            bookmarkService.AddBookmark(aURL, null);
        }
        return(true);
    }
};

var module = {
    registerSelf: function (compMgr, fileSpec, location, type) {
        dump("registerSelf for remoteControl\n");
        compMgr.registerComponentWithType(this.myCID,
                                          "Browser Remote Control",
                                          remoteControlProgID,
                                          fileSpec, location, true, true, 
                                          type);
    },

    getClassObject: function (compMgr, cid, iid) {
        if (!cid.equals(this.myCID))
            throw Components.results.NS_ERROR_NO_INTERFACE;
        
        if (!iid.equals(Components.interfaces.nsIFactory))
            throw Components.results.NS_ERROR_NOT_IMPLEMENTED;

        return this.myFactory;
    },

    canUnload: function () {
    },

    myCID: Components.ID("{97c8d0de-1dd1-11b2-bc64-86a3aaf8f5c5}"),

    myFactory: {
        CreateInstance: function (outer, iid) {
            if (outer != null)
                throw Components.results.NS_ERROR_NO_AGGREGATION;
            
            if (!(iid.equals(nsIRemoteBrowserControl) ||
                  iid.equals(Components.interfaces.nsISupports))) {
                throw Components.results.NS_ERROR_INVALID_ARG;
            }

            return new BrowserRemoteControl();
        }
    }
};

function NSGetModule(compMgr, fileSpec) { return module; }
