/* -*- Mode: Java; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
 *  Alec Flett <alecf@netscape.com>
 */
// The history window uses JavaScript in bookmarks.js too.

var gHistoryOutliner;
var gLastHostname;
var gLastDomain;
var gGlobalHistory;
var gPrefService;
var gDeleteByHostname;
var gDeleteByDomain;
var gHistoryBundle;
var gHistoryStatus;
var gHistoryGrouping = "";
var gWindowManager = null;

function HistoryInit()
{
    gHistoryOutliner =  document.getElementById("historyOutliner");
    gDeleteByHostname = document.getElementById("menu_deleteByHostname");
    gDeleteByDomain =   document.getElementById("menu_deleteByDomain");
    gHistoryBundle =    document.getElementById("historyBundle");
    gHistoryStatus =    document.getElementById("statusbar-display");

    var outlinerController = new nsOutlinerController(gHistoryOutliner, document.getElementById('historyOutlinerBody'));
    var historyController = new nsHistoryController;
    gHistoryOutliner.controllers.appendController(historyController);

    if ("arguments" in window && window.arguments[0] && window.arguments.length >= 1) {
        // We have been supplied a resource URI to root the tree on
        var uri = window.arguments[0];
        setRoot(uri);
        if (uri.substring(0,5) == "find:") {
            // Update the windowtype so that future searches are directed 
            // there and the window is not re-used for bookmarks. 
            var windowNode = document.getElementById("history-window");
            windowNode.setAttribute("windowtype", "history:searchresults");
        }
        document.getElementById("groupingMenu").setAttribute("hidden", "true");
    }
    else {
        gPrefService = Components.classes["@mozilla.org/preferences;1"]
                                 .getService(Components.interfaces.nsIPref);
        try {
            gHistoryGrouping = gPrefService.getCharPref("browser.history.grouping");
        }
        catch(e) {
            gHistoryGrouping = "day";
        }
        GroupBy(gHistoryGrouping);
        if (gHistoryStatus) {  // must be the window
            switch(gHistoryGrouping) {
            case "none":
                document.getElementById("groupByNone").setAttribute("checked", "true");
                break;
            case "day":
            default:
                document.getElementById("groupByDay").setAttribute("checked", "true");
            }        
        }
    } 
    gHistoryOutliner.focus();
    gHistoryOutliner.outlinerBoxObject.view.selection.select(0);
}

function updateHistoryCommands()
{
    goUpdateCommand("cmd_deleteByHostname");
    goUpdateCommand("cmd_deleteByDomain");
}

function historyOnSelect()
{
    // every time selection changes, save the last hostname
    gLastHostname = "";
    gLastDomain = "";
    var match;
    var currentIndex = gHistoryOutliner.currentIndex;
    var rowIsContainer = gHistoryGrouping == "day" ? isContainer(gHistoryOutliner, currentIndex) : false;
    var url = gHistoryOutliner.outlinerBoxObject.view.getCellText(currentIndex, "URL");

    if (url && !rowIsContainer) {
        // matches scheme://(hostname)...
        match = url.match(/.*:\/\/([^\/:]*)/);

        if (match && match.length>1)
            gLastHostname = match[1];
      
        if (gHistoryStatus)
           gHistoryStatus.label = url;
    }
    else {
        if (gHistoryStatus)
            gHistoryStatus.label = "";
    }

    if (gLastHostname) {
        // matches the last foo.bar in foo.bar or baz.foo.bar
        match = gLastHostname.match(/([^.]+\.[^.]+$)/);
        if (match)
            gLastDomain = match[1];
    }
    document.commandDispatcher.updateCommands("select");
}

function nsHistoryController()
{
}

nsHistoryController.prototype =
{
    supportsCommand: function(command)
    {
        switch(command) {
        case "cmd_deleteByHostname":
        case "cmd_deleteByDomain":
            return true;
        default:
            return false;
        }
    },

    isCommandEnabled: function(command)
    {
        var enabled = false;
        var stringId;
        var text;
        switch(command) {
        case "cmd_deleteByHostname":
            if (gLastHostname) {
                stringId = "deleteHost";
                enabled = true;
            } else {
                stringId = "deleteHostNoSelection";
            }
            text = gHistoryBundle.stringBundle.formatStringFromName(stringId,
                                                                    [ gLastHostname ], 1);
            gDeleteByHostname.setAttribute("label", text);
            break;
        case "cmd_deleteByDomain":
            if (gLastDomain) {
                stringId = "deleteDomain";
                enabled = true;
            } else {
                stringId = "deleteDomainNoSelection";
            }
            text = gHistoryBundle.stringBundle.formatStringFromName(stringId,
                                                                    [ gLastDomain ], 1);
            gDeleteByDomain.setAttribute("label", text);
        }
        return enabled;
    },

    doCommand: function(command)
    {
        switch(command) {
        case "cmd_deleteByHostname":
            if (!gGlobalHistory)
                gGlobalHistory = Components.classes["@mozilla.org/browser/global-history;1"].getService(Components.interfaces.nsIBrowserHistory);
            gGlobalHistory.removePagesFromHost(gLastHostname, false)
            return true;

        case "cmd_deleteByDomain":
            if (!gGlobalHistory)
                gGlobalHistory = Components.classes["@mozilla.org/browser/global-history;1"].getService(Components.interfaces.nsIBrowserHistory);
            gGlobalHistory.removePagesFromHost(gLastDomain, true)
            return true;

        default:
            return false;
        }
    }
}

var historyDNDObserver = {
    onDragStart: function (aEvent, aXferData, aDragAction)
    {
        var currentIndex = gHistoryOutliner.currentIndex;
        if (isContainer(gHistoryOutliner, currentIndex))
            return false;
        var url = gHistoryOutliner.outlinerBoxObject.view.getCellText(currentIndex, "URL");
        var title = gHistoryOutliner.outlinerBoxObject.view.getCellText(currentIndex, "Name");

        var htmlString = "<A HREF='" + url + "'>" + title + "</A>";
        aXferData.data = new TransferData();
        aXferData.data.addDataForFlavour("text/unicode", url);
        aXferData.data.addDataForFlavour("text/html", htmlString);
        aXferData.data.addDataForFlavour("text/x-moz-url", url + "\n" + title);
    }
};

function OpenURL(aInNewWindow)
{
    var currentIndex = gHistoryOutliner.currentIndex;
    if (isContainer(gHistoryOutliner, currentIndex))
        return false;
      
    var url = gHistoryOutliner.outlinerBoxObject.view.getCellText(currentIndex, "URL");

    if (aInNewWindow) {
      var count = gHistoryOutliner.outlinerBoxObject.view.selection.count;
      if (count == 1)
        window.openDialog( getBrowserURL(), "_blank", "chrome,all,dialog=no", url );
      else {
        var min = new Object(); 
        var max = new Object();
        var rangeCount = gHistoryOutliner.outlinerBoxObject.view.selection.getRangeCount();
        for (var i = 0; i < rangeCount; ++i) {
          gHistoryOutliner.outlinerBoxObject.view.selection.getRangeAt(i, min, max);
          for (var k = max.value; k >= min.value; --k) {
            url = gHistoryOutliner.outlinerBoxObject.view.getCellText(k, "URL");
            window.openDialog(getBrowserURL(), "_blank", "chrome,all,dialog=no", url);
          }
        }
      }
    }        
    else
        openTopWin(url);
    return true;
}

/**
 * Root the tree on a given URI (used for displaying search results)
 */
function setRoot(root)
{
    var windowNode = document.getElementById("history-window");
    windowNode.setAttribute("title", gHistoryBundle.getString("search_results_title"));
    document.getElementById("historyOutlinerBody").setAttribute("ref", root);
}

function GroupBy(groupingType)
{
    var outlinerBody = document.getElementById("historyOutlinerBody");
    switch(groupingType) {
    case "none":
        outlinerBody.setAttribute("ref", "NC:HistoryRoot");
        break;
    case "site":
        // xxx for now
        outlinerBody.setAttribute("ref", "NC:HistoryByDate");
        break;
    case "day":
    default:
        outlinerBody.setAttribute("ref", "NC:HistoryByDate");
        break;
    }
    gPrefService.SetCharPref("browser.history.grouping", groupingType);
}

function historyAddBookmarks()
{
  var count = gHistoryOutliner.outlinerBoxObject.view.selection.count;
  var url;
  var title;
  if (count == 1) {
    var currentIndex = gHistoryOutliner.currentIndex;
    url = gHistoryOutliner.outlinerBoxObject.view.getCellText(currentIndex, "URL");
    title = gHistoryOutliner.outlinerBoxObject.view.getCellText(currentIndex, "Name");
    BookmarksUtils.addBookmark(url, title, undefined, true);
  }
  else if (count > 1) {
    var min = new Object(); 
    var max = new Object();
    var rangeCount = gHistoryOutliner.outlinerBoxObject.view.selection.getRangeCount();
    for (var i = 0; i < rangeCount; ++i) {
      gHistoryOutliner.outlinerBoxObject.view.selection.getRangeAt(i, min, max);
      for (var k = max.value; k >= min.value; --k) {
        url = gHistoryOutliner.outlinerBoxObject.view.getCellText(k, "URL");
        title = gHistoryOutliner.outlinerBoxObject.view.getCellText(k, "Name");
        BookmarksUtils.addBookmark(url, title, undefined, false);
      }
    }
  }
}


function updateItems()
{
  var count = gHistoryOutliner.outlinerBoxObject.view.selection.count;
  var openItem = document.getElementById("miOpen");
  var openItemInNewWindow = document.getElementById("miOpenInNewWindow");
  if (count > 1) {
    document.getElementById("miAddBookmark").setAttribute("label", document.getElementById('multipleBookmarks').getAttribute("label"));
    var min = new Object(); 
    var max = new Object();
    var rangeCount = gHistoryOutliner.outlinerBoxObject.view.selection.getRangeCount();
    for (var i = 0; i < rangeCount; ++i) {
      gHistoryOutliner.outlinerBoxObject.view.selection.getRangeAt(i, min, max);
      for (var k = max.value; k >= min.value; --k) {
        if (isContainer(gHistoryOutliner, k))          
          return false;
      }
    }
    openItem.setAttribute("hidden", "true");
    openItem.removeAttribute("default");
    openItemInNewWindow.setAttribute("default", "true");
  }
  else {
    document.getElementById("miAddBookmark").removeAttribute("disabled");
    document.getElementById("miAddBookmark").setAttribute("label", document.getElementById('oneBookmark').getAttribute("label"));
    var currentIndex = gHistoryOutliner.currentIndex;
    if (isContainer(gHistoryOutliner, currentIndex))
      return false;
    if (!gWindowManager) {
      gWindowManager = Components.classes['@mozilla.org/rdf/datasource;1?name=window-mediator'].getService();
      gWindowManager = gWindowManager.QueryInterface( Components.interfaces.nsIWindowMediator);
    }
    var topWindowOfType = gWindowManager.getMostRecentWindow("navigator:browser");
    if (!topWindowOfType) {
        openItem.setAttribute("hidden", "true");
        openItem.removeAttribute("default");
        openItemInNewWindow.setAttribute("default", "true");
    }
    else {
      openItem.removeAttribute("hidden");
      if (!openItem.getAttribute("default"))
        openItem.setAttribute("default", "true");
      openItemInNewWindow.removeAttribute("default");
    }
  }
  return true;
}



 