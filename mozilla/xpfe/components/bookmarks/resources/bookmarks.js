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
 *   Ben Goodger <ben@netscape.com> (Original Author, v3.0)
 */

var gBookmarksShell = null; 
 
////////////////////////////////////////////////////////////////////////////////
// Initialize the command controllers, set focus, outliner root, 
// window title state, etc. 
function Startup()
{
  var bookmarksView = document.getElementById("bookmarks-outliner");
  var bookmarksBody = document.getElementById("bookmarks-outlinerbody");
  
  // Set up the outliner controller
  gBookmarksShell = new nsBookmarksShell("bookmarks-outliner", "bookmarks-outlinerbody");
  var rdflinerObserver = new nsBookmarksRDFLinerObserver();
  var builder = bookmarksBody.builder.QueryInterface(Components.interfaces.nsIXULOutlinerBuilder);
  builder.addObserver(rdflinerObserver);

  const windowNode = document.getElementById("bookmark-window");
  // If we've been opened with a parameter, root the outliner on it.
  if ("arguments" in window && window.arguments[0]) {
    var uri = window.arguments[0];
    bookmarksBody.setAttribute("ref", uri);
    var title = "";
    if (uri.substring(0,5) == "find:") {
      title = gBookmarksShell.getLocaleString("search_results_title");
      // Update the windowtype so that future searches are directed 
      // there and the window is not re-used for bookmarks. 
      windowNode.setAttribute("windowtype", "bookmarks:searchresults");
    }
    else {
      const krNameArc = gBookmarksShell.RDF.GetResource(NC_NS + "Name");
      const krRoot = gBookmarksShell.RDF.GetResource(window.arguments[0]);
      var rName = gBookmarksShell.db.GetTarget(krRoot, krNameArc, true);
      title = rName.QueryInterface(Components.interfaces.nsIRDFLiteral).Value;
    }
    const titleString = gBookmarksShell.getLocaleString("window_title");
    windowNode.setAttribute("title", titleString.replace(/%folder_name%/gi, title));
  }
  else {
    var rootfoldername = gBookmarksShell.getLocaleString("bookmarks_root");
    const kProfileContractID = "@mozilla.org/profile/manager;1";
    const kProfileIID = Components.interfaces.nsIProfile;
    const kProfile = Components.classes[kProfileContractID].getService(kProfileIID);
    rootfoldername = rootfoldername.replace(/%user_name%/, kProfile.currentProfile);
    windowNode.setAttribute("title", rootfoldername);
  }

  // XXX - reinit sort
  
  bookmarksView.outlinerBoxObject.selection.select(0);
  bookmarksView.focus();
}

function Shutdown ()
{
  // Store current window position and size in window attributes (for persistence).
  var win = document.getElementById("bookmark-window");
  win.setAttribute("x", screenX);
  win.setAttribute("y", screenY);
  win.setAttribute("height", outerHeight);
  win.setAttribute("width", outerWidth);

  gBookmarksShell.flushDataSource();
}

