/* -*- Mode: Java; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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

const nsIWebNavigation = Components.interfaces.nsIWebNavigation;

var appCore = null;
var locationFld = null;
var commandHandler = null;
var gURLBar = null;

function nsCommandHandler()
{
}

nsCommandHandler.prototype = 
{
  QueryInterface : function(iid)
    {
      if (iid.equals(Components.interfaces.nsICommandHandler))
      {
        return this;
      }
      throw Components.results.NS_NOINTERFACE;
    },

    exec : function(command, params)
      {
      },
    query : function(command, params, result)
      {
        result = "";
      }
}

//

function nsXULBrowserWindow()
{
}

nsXULBrowserWindow.prototype = 
{
  QueryInterface : function(iid)
    {
    if(iid.equals(Components.interfaces.nsIXULBrowserWindow))
      return this;
    throw Components.results.NS_NOINTERFACE;
    },
  setJSStatus : function(status)
    {
    },
  setJSDefaultStatus : function(status)
    {
    },
  setDefaultStatus : function(status)
    {
    },
  setOverLink : function(link)
    {
    },
  onProgress : function (channel, current, max)
    {
    },
  onStateChange : function (progress, request, state, status)
    {
    },
  onStatus : function(url, message)
    {
    },
  onLocationChange : function(location)
    {
      if(!locationFld)
        locationFld = document.getElementById("urlbar");

      // We should probably not do this if the value has changed since the user 
      // searched
      locationFld.setAttribute("value", location);
    }
}


function MiniNavStartup()
{
  dump("*** MiniNavStartup\n");
  window.XULBrowserWindow = new nsXULBrowserWindow();

  var succeeded = false;
  var webNavigation = getWebNavigation();
  try {
    // Create the browser instance component.
    appCore = Components.classes["@mozilla.org/appshell/component/browser/instance;1"]
                        .createInstance(Components.interfaces.nsIBrowserInstance);

    webNavigation.sessionHistory = Components.classes["@mozilla.org/browser/shistory;1"]
                                             .createInstance(Components.interfaces.nsISHistory);
    succeeded = true;
  } catch (e) {
  }

  if (!succeeded) {
    // Give up.
    dump("Error creating browser instance\n");
    window.close();
  }
  // Initialize browser instance..
  appCore.setWebShellWindow(window);
  _content.appCore = appCore;

  // create the embedding command handler
  netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
  var commandHandlerInit = Components
      .classes["@mozilla.org/embedding/browser/nsCommandHandler;1"]
      .createInstance(Components.interfaces.nsICommandHandlerInit);

  // Attach it to the window
  commandHandlerInit.window = window;
  commandHandler = commandHandlerInit.QueryInterface(Components.interfaces.nsICommandHandler);

  gURLBar = document.getElementById("urlbar");
  dump("gURLBar " + gURLBar + "\n");
}

function MiniNavShutdown()
{
  dump("*** MiniNavShutdown\n");
  // Close the app core.
  if ( appCore )
    appCore.close();
}

function getBrowser()
{
  return document.getElementById("content");
}

function getWebNavigation()
{
  return getBrowser().webNavigation;
}

function CHExecTest()
{
  if (commandHandler != null)
  {
    commandHandler.exec("hello", "xxx");
  }
}

function CHQueryTest()
{
  if (commandHandler != null)
  {
    var result = commandHandler.query("hello", "xxx");
  }
}

function InitContextMenu(xulMenu)
{
  // Back determined by canGoBack broadcaster.
  InitMenuItemAttrFromNode( "context-back", "disabled", "canGoBack" );

  // Forward determined by canGoForward broadcaster.
  InitMenuItemAttrFromNode( "context-forward", "disabled", "canGoForward" );
}

function InitMenuItemAttrFromNode( item_id, attr, other_id )
{
  var elem = document.getElementById( other_id );
  if ( elem && elem.getAttribute( attr ) == "true" ) {
    SetMenuItemAttr( item_id, attr, "true" );
  } else {
    SetMenuItemAttr( item_id, attr, null );
  }
}

function SetMenuItemAttr( id, attr, val )
{
  var elem = document.getElementById( id );
  if ( elem ) {
    if ( val == null ) {
      // null indicates attr should be removed.
      elem.removeAttribute( attr );
    } else {
      // Set attr=val.
      elem.setAttribute( attr, val );
    }
  }
}

function loadURI(uri)
{
  getWebNavigation().loadURI(uri, nsIWebNavigation.LOAD_FLAGS_NONE);
}

function BrowserLoadURL()
{
  dump("browserloadurl: " + gURLBar.value + '\n');
  try {
    loadURI(gURLBar.value);
  }
  catch(e) {
  }
}

function BrowserBack()
{
  getWebNavigation().goBack();
}

function BrowserForward()
{
  getWebNavigation().goForward();
}

function BrowserStop()
{
  getWebNavigation().stop();
}

function BrowserReload()
{
  getWebNavigation().reload(nsIWebNavigation.LOAD_FLAGS_NONE);
}

