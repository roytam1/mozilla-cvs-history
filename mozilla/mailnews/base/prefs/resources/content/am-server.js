/* -*- Mode: Java; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
 * Copyright (C) 1998-2001 Netscape Communications Corporation. All
 * Rights Reserved.
 * 
 * Contributors:
 *   alecf@netscape.com
 *   sspitzer@netscape.com
 *   racham@netscape.com
 *   hwaara@chello.se
 */

var gRedirectorType = "";

function onInit() 
{
    initServerType();

    setupBiffUI();
    setupMailOnServerUI();
    setupFixedUI();
    setupNotifyUI();
}

function onPreInit(account, accountValues)
{
    var type = parent.getAccountValue(account, accountValues, "server", "type", null, false);
    gRedirectorType = parent.getAccountValue(account, accountValues, "server", "redirectorType", null, false);
    hideShowControls(type);

    if(!(account.incomingServer.isSecureServer))
      document.getElementById("server.isSecure").setAttribute("hidden", "true");
    else
      document.getElementById("server.isSecure").removeAttribute("hidden");
    
    if(!account.incomingServer.canEmptyTrashOnExit)
    {
      document.getElementById("server.emptyTrashOnExit").setAttribute("hidden", "true");
      document.getElementById("imap.deleteModel.box").setAttribute("hidden", "true");
    }
    var hideButton = false;

    try {
      if (gRedirectorType) {
        var prefs = Components.classes["@mozilla.org/preferences-service;1"].getService(Components.interfaces.nsIPrefBranch);
        var prefString = "mail.accountmanager." + gRedirectorType + ".hide_advanced_button";
        hideButton = prefs.getBoolPref(prefString);
      }
    }
    catch (ex) { }
    if (hideButton)
      document.getElementById("server.advancedbutton").setAttribute("hidden", "true");  
    else 
      document.getElementById("server.advancedbutton").removeAttribute("hidden");  
}

function initServerType() {
  var serverType = document.getElementById("server.type").getAttribute("value");
  
  var propertyName = "serverType-" + serverType;

  var messengerBundle = document.getElementById("bundle_messenger");
  var verboseName = messengerBundle.getString(propertyName);

  setDivText("servertype.verbose", verboseName);
}

function hideShowControls(serverType)
{
    var controls = document.getElementsByAttribute("hidable", "true");
    var len = controls.length;
    for (var i=0; i<len; i++) {
        var control = controls[i];

        var hideFor = control.getAttribute("hidefor");
        if (!hideFor)
            throw "this should not happen, things that are hidable should have hidefor set";

        var box = getEnclosingContainer(control);

        if (!box)
            throw "this should not happen, things that are hidable should be in a box";

        // hide unsupported server type
        // adding support for hiding multiple server types using hideFor="server1,server2"
        var hideForBool = false;
        var hideForTokens = hideFor.split(",");
        for (var j = 0; j < hideForTokens.length; j++) {
            if (hideForTokens[j] == serverType) {
                hideForBool = true;
                break;
            }
        }

        if (hideForBool) {
            box.setAttribute("hidden", "true");
        }
        else {
            box.removeAttribute("hidden");
        }
    }
}


function setDivText(divname, value) {
    var div = document.getElementById(divname);
    if (!div) return;
    div.setAttribute("value", value);
}


function openImapAdvanced()
{
    var imapServer = getImapServer();
    dump("Opening dialog..\n");
    window.openDialog("chrome://messenger/content/am-imap-advanced.xul",
                      "_blank",
                      "chrome,modal,titlebar", imapServer);

    saveServerLocally(imapServer);
}

function getImapServer() {
    var imapServer = new Array;

    imapServer.dualUseFolders = document.getElementById("imap.dualUseFolders").checked

    imapServer.usingSubscription = document.getElementById("imap.usingSubscription").checked;

    imapServer.maximumConnectionsNumber = document.getElementById("imap.maximumConnectionsNumber").getAttribute("value");
    // string prefs
    imapServer.personalNamespace = document.getElementById("imap.personalNamespace").getAttribute("value");
    imapServer.publicNamespace = document.getElementById("imap.publicNamespace").getAttribute("value");
    imapServer.serverDirectory = document.getElementById("imap.serverDirectory").getAttribute("value");
    imapServer.otherUsersNamespace = document.getElementById("imap.otherUsersNamespace").getAttribute("value");

    imapServer.overrideNamespaces = document.getElementById("imap.overrideNamespaces").checked;
    return imapServer;
}

function saveServerLocally(imapServer)
{
    document.getElementById("imap.dualUseFolders").checked = imapServer.dualUseFolders;
    document.getElementById("imap.usingSubscription").checked = imapServer.usingSubscription;

    document.getElementById("imap.maximumConnectionsNumber").setAttribute("value", imapServer.maximumConnectionsNumber);
    // string prefs
    document.getElementById("imap.personalNamespace").setAttribute("value", imapServer.personalNamespace);
    document.getElementById("imap.publicNamespace").setAttribute("value", imapServer.publicNamespace);
    document.getElementById("imap.serverDirectory").setAttribute("value", imapServer.serverDirectory);
    document.getElementById("imap.otherUsersNamespace").setAttribute("value", imapServer.otherUsersNamespace);

    document.getElementById("imap.overrideNamespaces").checked = imapServer.overrideNamespaces;

}

function getEnclosingContainer(startNode) {

    var parent = startNode;
    var box;
    
    while (parent && parent != document) {

        var isContainer =
            (parent.getAttribute("iscontrolcontainer") == "true");
          
        // remember the FIRST container we encounter, or the first
        // controlcontainer
        if (!box || isContainer)
            box=parent;
        
        // break out with a controlcontainer
        if (isContainer)
            break;
        parent = parent.parentNode;
    }
    
    return box;
}

function secureSelect() {
    var serverType   = document.getElementById("server.type").getAttribute("value");
    var protocolInfo = Components.classes["@mozilla.org/messenger/protocol/info;1?type=" + serverType].getService(Components.interfaces.nsIMsgProtocolInfo);

    // If the secure option is checked, protocolInfo returns a secure port value
	// for the corresponding protocol. Otherwise, a default value is returned.
    if (document.getElementById("server.isSecure").checked)
        document.getElementById("server.port").value = protocolInfo.getDefaultServerPort(true);
    else
        document.getElementById("server.port").value = protocolInfo.getDefaultServerPort(false);
}

function setupBiffUI()
{ 
   var broadcaster = document.getElementById("broadcaster_doBiff");

   var dobiff = document.getElementById("server.doBiff");
   var checked = dobiff.checked;
   var locked = getAccountValueIsLocked(dobiff);

   if (checked)
     broadcaster.removeAttribute("disabled");
   else
     broadcaster.setAttribute("disabled", "true");
   if (locked)
     broadcaster.setAttribute("disabled","true");
}

function setupMailOnServerUI()
{ 
   var checked = document.getElementById("pop3.leaveMessagesOnServer").checked;
   var locked = getAccountValueIsLocked(document.getElementById("pop3.leaveMessagesOnServer"));
   document.getElementById("pop3.deleteMailLeftOnServer").disabled = locked || !checked ;
}

function setupFixedUI()
{
  // if the redirectorType is non-empty, then the fixedValue elements should be shown
  //
  // one day, add code that allows for the redirector type to specify
  // to show fixed values or not.  see bug #132737
  // use gRedirectorType to get a pref like
  // "mail.accountmanager." + gRedirectorType + ".show_fixed_values"
  //
  // but for now, this isn't needed.  we'll assume those who implement
  // redirector types want to show fixed values.
  var showFixedValues = gRedirectorType ? true : false;

  var controls = [document.getElementById("fixedServerName"), 
                  document.getElementById("fixedUserName"),
                  document.getElementById("fixedServerPort")];

  var len = controls.length;  
  for (var i=0; i<len; i++) {
    var fixedElement = controls[i];
    var otherElement = document.getElementById(fixedElement.getAttribute("use"));

    if (showFixedValues) {
      // get the value from the non-fixed element.
      var fixedElementValue = otherElement.value;
      fixedElement.setAttribute("value", fixedElementValue);

      otherElement.setAttribute("collapsed","true");
      fixedElement.removeAttribute("collapsed");
    }
    else {
      fixedElement.setAttribute("collapsed","true");
      otherElement.removeAttribute("collapsed");
    }
  }
}

function setupNotifyUI()
{ 
    var broadcaster = document.getElementById("broadcaster_notify");

    var notify = document.getElementById("nntp.notifyOn");
    var checked = notify.checked;
    var locked = getAccountValueIsLocked(notify);

    if (checked)
      broadcaster.removeAttribute("disabled");
    else
      broadcaster.setAttribute("disabled", "true");
    if (locked)
      broadcaster.setAttribute("disabled","true");
}
