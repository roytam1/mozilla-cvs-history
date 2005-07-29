/* -*- Mode: Java; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

const nsIWebNavigation = Components.interfaces.nsIWebNavigation;
const nsIWebProgressListener = Components.interfaces.nsIWebProgressListener;

var gURLBar = null;
var gBrowserStatusHandlerArray=new Array();
var gtabCounter=0;
var gBrowserStatusHandler;
var gSelectedTab=null;

function nsBrowserStatusHandler()
{
}

nsBrowserStatusHandler.prototype = 
{
  QueryInterface : function(aIID)
  {
    if (aIID.equals(Components.interfaces.nsIWebProgressListener) ||
        aIID.equals(Components.interfaces.nsISupportsWeakReference) ||
        aIID.equals(Components.interfaces.nsISupports))
    {
      return this;
    }
    throw Components.results.NS_NOINTERFACE;
  },

  init : function()
  {
    this.urlBar           = document.getElementById("urlbar");
    this.statusbarText    = document.getElementById("statusbar-text");
    this.stopreloadButton = document.getElementById("reload-stop-button");
    this.statusbar        = document.getElementById("statusbar");
    this.refTab           = null;                            // reference tab.
    this.transferCount    = 0;                               //
  },

  destroy : function()
  {
    this.urlBar = null;
    this.statusbarText = null;
    this.stopreloadButton = null;
    this.statusbar = null;
  },

  onStateChange : function(aWebProgress, aRequest, aStateFlags, aStatus)
  {
    if(aStateFlags & nsIWebProgressListener.STATE_TRANSFERRING) { 
      this.transferCount+=5;
      // Has to be fixed and stay within the urlBarIdentity functions. 
      //if(gSelectedTab==this.refTab) {
      document.styleSheets[1].cssRules[0].style.backgroundPosition=this.transferCount+"px 100%";
      // }
    }

    if (aStateFlags & nsIWebProgressListener.STATE_IS_NETWORK)
    {

      if (aStateFlags & nsIWebProgressListener.STATE_START)
      {
        this.transferCount=0;
        // Has to be fixed and stay within the urlBarIdentity functions. 
        // if(gSelectedTab==this.refTab) {
        document.styleSheets[1].cssRules[0].style.backgroundImage="url(chrome://minimo/skin/transfer.gif)";
        //  }
        this.stopreloadButton.image = "chrome://minimo/skin/stop.gif";
        this.stopreloadButton.onClick = "BrowserStop()";
        this.statusbar.hidden = false;
        return;
      }
      
      if (aStateFlags & nsIWebProgressListener.STATE_STOP)
      {
        // Has to be fixed and stay within the urlBarIdentity functions. 
        //    if(gSelectedTab==this.refTab) {
        document.styleSheets[1].cssRules[0].style.backgroundPosition="0px 100%";
        // }

        this.stopreloadButton.image = "chrome://minimo/skin/reload.gif";
        this.stopreloadButton.onClick = "BrowserReload()";
        
        this.statusbar.hidden = true;
        this.statusbarText.label = "";
        return;
      }
      return;
    }

    if (aStateFlags & nsIWebProgressListener.STATE_IS_DOCUMENT)
    { 
      if (aStateFlags & nsIWebProgressListener.STATE_START)
      {
        return;
      }
      
      if (aStateFlags & nsIWebProgressListener.STATE_STOP)
      {
        return;
      }
      return;
    }
  },

  onProgressChange : function(aWebProgress, aRequest, aCurSelfProgress, aMaxSelfProgress, aCurTotalProgress, aMaxTotalProgress)
  {
    this.transferCount++;
    document.styleSheets[1].cssRules[0].style.backgroundPosition=this.transferCount+"px 100%";

    //alert("aWebProgress="+aWebProgress+"aRequest.name: "+aRequest.name+"aCurSelfProgress:"+aCurSelfProgress);
  },


  onLocationChange : function(aWebProgress, aRequest, aLocation)
  {

    // Update the URL BAR only if the gSelectedTab matches this tab. 
    // if(gSelectedTab==this.refTab) {
        domWindow = aWebProgress.DOMWindow;
        // Update urlbar only if there was a load on the root docshell
        if (domWindow == domWindow.top) {
          this.urlBar.value = aLocation.spec;
        }

        // Work-in-progress, to not use the DOM store and simply the StatusHander objects..
        //this.lastLocation=aLocation.spec; // the onclick tab handler should get this URL to refresh the urlbar. 
          this.refTab.setAttribute("lastLocation",aLocation.spec);

    //}
  },

  onStatusChange : function(aWebProgress, aRequest, aStatus, aMessage)
  {
    this.statusbarText.label = aMessage;
  },

  onSecurityChange : function(aWebProgress, aRequest, aState)
  {
  }
}

/** 
  * Initial Minimo Startup 
  * 
  **/

/* moved this as global */ 


function MiniNavStartup()
{

  gURLBar = document.getElementById("urlbar");
  var currentTab=getBrowser().selectedTab;
  browserInit(currentTab);
  gSelectedTab=currentTab;
  loadURI("http://www.google.com");

}

/** 
  * Init stuff
  * 
  **/
function browserInit(refTab)
{

    var BrowserStatusHandler = new nsBrowserStatusHandler();

    BrowserStatusHandler.init();
    BrowserStatusHandler.refTab=refTab; // WARNING (was refTab);

    try {

      getBrowser().addProgressListener(BrowserStatusHandler, Components.interfaces.nsIWebProgress.NOTIFY_ALL);
      gBrowserStatusHandlerArray.push(BrowserStatusHandler);
      var refBrowser=getBrowser().getBrowserForTab(refTab);
      var webNavigation=refBrowser.webNavigation;
      webNavigation.sessionHistory = Components.classes["@mozilla.org/browser/shistory;1"].createInstance(Components.interfaces.nsISHistory);
    } catch (e) {
      alert("Error trying to startup browser.  Please report this as a bug:\n" + e);
    }

    try {
      refBrowser.markupDocumentViewer.textZoom = .75;
    } catch (e) {}
    gURLBar = document.getElementById("urlbar");

}

function MiniNavShutdown()
{
  if (gBrowserStatusHandler) gBrowserStatusHandler.destroy();
}

function getBrowser()
{
  return document.getElementById("content");
}

function getWebNavigation()
{
  return getBrowser().webNavigation;
}

function loadURI(uri)
{
  getWebNavigation().loadURI(uri, nsIWebNavigation.LOAD_FLAGS_NONE, null, null, null);
}

function BrowserLoadURL()
{
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
  getWebNavigation().stop(nsIWebNavigation.STOP_ALL);
}

function BrowserReload()
{
  getWebNavigation().reload(nsIWebNavigation.LOAD_FLAGS_NONE);
}

function BrowserAddTab() {


    var thisTab=getBrowser().addTab("http://taboca.com");

    browserInit(thisTab);
}

function BrowserOpenTab()
{
  try { 
    getBrowser().selectedTab = getBrowser().addTab('about:blank');
    browserInit(getBrowser().selectedTab);
  } catch (e) {
     alert(e);
  }
  //  if (gURLBar) setTimeout(function() { gURLBar.focus(); }, 0);

}

/** 
  * Work-in-progress, this handler is 100% generic and gets all the clicks for the entire tabbed 
  * content area. Need to fix with the approach where we handler only clicks for the actual tab. 
  * Need to fix to also handle the key action case. 
  **/
function tabbrowserAreaClick(e) {

    // When the click happens, 
    // updates the location bar with the lastLocation for the given selectedTab. 
    // Note that currently the lastLocation is stored in the DOM for the selectedTab. 

    gSelectedTab=getBrowser().selectedTab; 
    gURLBar.value=gSelectedTab.getAttribute("lastLocation");
    document.styleSheets[1].cssRules[0].style.backgroundPosition="1000px 100%";

}


/** 
  * urlbar indentity, style, progress indicator.
  **/ 
function urlbar() {
    

}
