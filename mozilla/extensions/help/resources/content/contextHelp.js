/* -*- Mode: Java; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
 * The Original Code is Mozilla Communicator client code, released
 * March 31, 1998.
 * 
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation. Portions created by Netscape are
 * Copyright (C) 1998-1999 Netscape Communications Corporation. All
 * Rights Reserved.
 */

const MOZ_HELP_URI = "chrome://help/content/help.xul";
const MOZILLA_HELP = "chrome://help/locale/mozillahelp.rdf";
var helpFileURI = MOZILLA_HELP;

// Call this function to display a help topic.
// uri: [chrome uri of rdf help file][?topic]
function openHelp(topic) {
  var topWindow = locateHelpWindow(helpFileURI);
  if ( topWindow ) {
    topWindow.focus();
    topWindow.displayTopic(topic);
  } else {
    var params = Components.classes["@mozilla.org/embedcomp/dialogparam;1"]
                           .createInstance(Components.interfaces.nsIDialogParamBlock);
    params.SetNumberStrings(2);
    params.SetString(0, helpFileURI);
    params.SetString(1, topic);
    var ww = Components.classes["@mozilla.org/embedcomp/window-watcher;1"]
                       .getService(Components.interfaces.nsIWindowWatcher);
    ww.openWindow(null, MOZ_HELP_URI, "_blank", "chrome,all,alwaysRaised,dialog=no", params);
  }
}

function setHelpFileURI(rdfURI) {
  helpFileURI = rdfURI; 
}

// Locate mozilla:help window (if any) opened for this help file uri.
function locateHelpWindow(helpFileURI) {
  var windowManager = Components.classes['@mozilla.org/appshell/window-mediator;1'].getService();
  var windowManagerInterface = windowManager.QueryInterface( Components.interfaces.nsIWindowMediator);
  var iterator = windowManagerInterface.getEnumerator( "mozilla:help");
  var topWindow = null;
  while (iterator.hasMoreElements()) {
    var aWindow = iterator.getNext();
    if (aWindow.getHelpFileURI() == helpFileURI) {
      topWindow = aWindow;
      break;  
    }  
  }
  return topWindow;
}
