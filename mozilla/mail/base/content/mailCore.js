# -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
# ***** BEGIN LICENSE BLOCK *****
# Version: MPL 1.1/GPL 2.0/LGPL 2.1
#
# The contents of this file are subject to the Mozilla Public License Version
# 1.1 (the "License"); you may not use this file except in compliance with
# the License. You may obtain a copy of the License at
# http://www.mozilla.org/MPL/
#
# Software distributed under the License is distributed on an "AS IS" basis,
# WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
# for the specific language governing rights and limitations under the
# License.
#
# The Original Code is Mozilla Communicator client code, released
# March 31, 1998.
#
# The Initial Developer of the Original Code is
# Netscape Communications Corporation.
# Portions created by the Initial Developer are Copyright (C) 2001
# the Initial Developer. All Rights Reserved.
#
# Contributor(s):
#
# Alternatively, the contents of this file may be used under the terms of
# either the GNU General Public License Version 2 or later (the "GPL"), or
# the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
# in which case the provisions of the GPL or the LGPL are applicable instead
# of those above. If you wish to allow use of your version of this file only
# under the terms of either the GPL or the LGPL, and not to allow others to
# use your version of this file under the terms of the MPL, indicate your
# decision by deleting the provisions above and replace them with the notice
# and other provisions required by the GPL or the LGPL. If you do not delete
# the provisions above, a recipient may use your version of this file under
# the terms of any one of the MPL, the GPL or the LGPL.
#
# ***** END LICENSE BLOCK *****

/*
 * Core mail routines used by all of the major mail windows (address book, 3-pane, compose and stand alone message window).
 * Routines to support custom toolbars in mail windows, opening up a new window of a particular type all live here. 
 * Before adding to this file, ask yourself, is this a JS routine that is going to be used by all of the main mail windows?
 */

function CustomizeMailToolbar(toolboxId, customizePopupId)
{
  // Disable the toolbar context menu items
  var menubar = document.getElementById("mail-menubar");
  for (var i = 0; i < menubar.childNodes.length; ++i)
    menubar.childNodes[i].setAttribute("disabled", true);

  var customizePopup = document.getElementById(customizePopupId);
  customizePopup.setAttribute("disabled", "true");

  var toolbox = document.getElementById(toolboxId);

  var customizeURL = "chrome://global/content/customizeToolbar.xul";
#ifdef TOOLBAR_CUSTOMIZATION_SHEET
  var sheetFrame = document.getElementById("customizeToolbarSheetIFrame");
  sheetFrame.hidden = false;

  // The document might not have been loaded yet, if this is the first time.
  // If it is already loaded, reload it so that the onload intialization code
  // re-runs.
  if (sheetFrame.getAttribute("src") == customizeURL)
    sheetFrame.contentWindow.location.reload()
  else
    sheetFrame.setAttribute("src", customizeURL);

  var sheetWidth = sheetFrame.style.width.match(/([0-9]+)px/)[1];
  document.getElementById("customizeToolbarSheetPopup")
          .openPopup(toolbox, "after_start",
                     (window.innerWidth - sheetWidth) / 2, 0);
#else
  var wintype = document.documentElement.getAttribute("windowtype");
  wintype = wintype.replace(/:/g, "");

  window.openDialog(customizeURL,
                    "CustomizeToolbar"+wintype,
                    "chrome,all,dependent", toolbox);
#endif
}

function MailToolboxCustomizeDone(aEvent, customizePopupId)
{
#ifdef TOOLBAR_CUSTOMIZATION_SHEET
  document.getElementById("customizeToolbarSheetIFrame").hidden = true;
  document.getElementById("customizeToolbarSheetPopup").hidePopup();
#endif

  // Update global UI elements that may have been added or removed

  // Re-enable parts of the UI we disabled during the dialog
  var menubar = document.getElementById("mail-menubar");
  for (var i = 0; i < menubar.childNodes.length; ++i)
    menubar.childNodes[i].setAttribute("disabled", false);

  // make sure the mail views search box is initialized
  if (document.getElementById("mailviews-container"))
    ViewPickerOnLoad();

  // make sure the folder location picker is initialized
  if (document.getElementById("folder-location-container"))
  {
    loadFolderViewForTree(gCurrentFolderView, document.getElementById('folderLocationPopup').tree);
    UpdateFolderLocationPicker(gMsgFolderSelected);
  }

  gSearchInput = null;
  if (document.getElementById("search-container"))
    GetSearchInput();

  var customizePopup = document.getElementById(customizePopupId);
  customizePopup.removeAttribute("disabled");

  // make sure our toolbar buttons have the correct enabled state restored to them...
  if (this.UpdateMailToolbar != undefined)
    UpdateMailToolbar(focus); 
}

function onViewToolbarCommand(aEvent, toolboxId)
{
  var toolbox = document.getElementById(toolboxId);
  var index = aEvent.originalTarget.getAttribute("toolbarindex");
  var toolbar = toolbox.childNodes[index];

  toolbar.collapsed = aEvent.originalTarget.getAttribute("checked") != "true";
  document.persist(toolbar.id, "collapsed");
}

function onViewToolbarsPopupShowing(aEvent, toolboxId)
{
  var popup = aEvent.target;

  // Empty the menu
  for (var i = popup.childNodes.length-1; i >= 0; --i) {
    var deadItem = popup.childNodes[i];
    if (deadItem.hasAttribute("toolbarindex"))
      popup.removeChild(deadItem);
  }

  var firstMenuItem = popup.firstChild;

  var toolbox = document.getElementById(toolboxId);
  for (var i = 0; i < toolbox.childNodes.length; ++i) {
    var toolbar = toolbox.childNodes[i];
    var toolbarName = toolbar.getAttribute("toolbarname");
    var type = toolbar.getAttribute("type");
    if (toolbarName && type != "menubar") {
      var menuItem = document.createElement("menuitem");
      menuItem.setAttribute("toolbarindex", i);
      menuItem.setAttribute("type", "checkbox");
      menuItem.setAttribute("label", toolbarName);
      menuItem.setAttribute("accesskey", toolbar.getAttribute("accesskey"));
      menuItem.setAttribute("checked", toolbar.getAttribute("collapsed") != "true");
      popup.insertBefore(menuItem, firstMenuItem);
      menuItem.addEventListener("command",
        function(aEvent) { onViewToolbarCommand(aEvent, toolboxId); }, false);
    }
    toolbar = toolbar.nextSibling;
  }
}

function toJavaScriptConsole()
{
    toOpenWindowByType("global:console", "chrome://global/content/console.xul");
}

function toOpenWindowByType( inType, uri )
{
  const Cc = Components.classes;
  const Ci = Components.interfaces;
  var windowManager = Cc['@mozilla.org/appshell/window-mediator;1'].getService();
  var windowManagerInterface = windowManager.QueryInterface(Ci.nsIWindowMediator);

  var topWindow = windowManagerInterface.getMostRecentWindow( inType );
  if ( topWindow )
    topWindow.focus();
  else
    window.open(uri, "_blank", "chrome,extrachrome,menubar,resizable,scrollbars,status,toolbar");
}

function toMessengerWindow()
{
  toOpenWindowByType("mail:3pane", "chrome://messenger/content/messenger.xul");
}

function toAddressBook() 
{
  toOpenWindowByType("mail:addressbook", "chrome://messenger/content/addressbook/addressbook.xul");
}

function toImport()
{
  window.openDialog("chrome://messenger/content/importDialog.xul","importDialog","chrome, modal, titlebar, centerscreen");
}

// aPaneID
function openOptionsDialog(aPaneID, aTabID)
{
  var prefsService = Components.classes["@mozilla.org/preferences-service;1"].getService(Components.interfaces.nsIPrefService).getBranch(null);
  var instantApply = prefsService.getBoolPref("browser.preferences.instantApply");
  var features = "chrome,titlebar,toolbar,centerscreen" + (instantApply ? ",dialog=no" : ",modal");

  var wm = Components.classes["@mozilla.org/appshell/window-mediator;1"]
           .getService(Components.interfaces.nsIWindowMediator);
  
  var win = wm.getMostRecentWindow("Mail:Preferences");
  if (win)
  {
    win.focus();
    if (aPaneID)
    {
      var pane = win.document.getElementById(aPaneID);
      win.document.documentElement.showPane(pane);
      
      // I don't know how to support aTabID for an arbitrary panel when the dialog is already open
      // This is complicated because showPane is asynchronous (it could trigger a dynamic overlay)
      // so our tab element may not be accessible right away...
    }
  }
  else 
    openDialog("chrome://messenger/content/preferences/preferences.xul","Preferences", features, aPaneID, aTabID);
}

function openAddonsMgr()
{
  const EMTYPE = "Extension:Manager";
  var wm = Components.classes["@mozilla.org/appshell/window-mediator;1"]
                     .getService(Components.interfaces.nsIWindowMediator);
  var theEM = wm.getMostRecentWindow(EMTYPE);
  if (theEM) {
    theEM.focus();
    return;
  }

  const EMURL = "chrome://mozapps/content/extensions/extensions.xul";
  const EMFEATURES = "chrome,menubar,extra-chrome,toolbar,dialog=no,resizable";
  window.openDialog(EMURL, "", EMFEATURES);
}

function SetBusyCursor(window, enable)
{
    // setCursor() is only available for chrome windows.
    // However one of our frames is the start page which 
    // is a non-chrome window, so check if this window has a
    // setCursor method
    if ("setCursor" in window) {
        if (enable)
            window.setCursor("wait");
        else
            window.setCursor("auto");
    }

  var numFrames = window.frames.length;
  for(var i = 0; i < numFrames; i++)
    SetBusyCursor(window.frames[i], enable);
}

function openAboutDialog()
{
#ifdef XP_MACOSX
  var wm = Components.classes["@mozilla.org/appshell/window-mediator;1"]
                     .getService(Components.interfaces.nsIWindowMediator);
  var win = wm.getMostRecentWindow("Mail:About");
  if (win)  // If we have an open about dialog, just focus it.
    win.focus();
  else {
    // Define minimizable=no although it does nothing on OS X
    // (see Bug 287162); remove this comment once Bug 287162 is fixed...
    window.open("chrome://messenger/content/aboutDialog.xul", "About",
                "chrome, resizable=no, minimizable=no");
  }
#else
  window.openDialog("chrome://messenger/content/aboutDialog.xul", "About", "modal,centerscreen,chrome,resizable=no");
#endif
}

/**
 * Opens region specific web pages for the application like the release notes, the help site, etc. 
 *   aResourceName --> the string resource ID in region.properties to load. 
 */
function openRegionURL(aResourceName)
{
  var appInfo = Components.classes["@mozilla.org/xre/app-info;1"]
                          .getService(Components.interfaces.nsIXULAppInfo);
  try {
    var strBundleService = Components.classes["@mozilla.org/intl/stringbundle;1"].getService(Components.interfaces.nsIStringBundleService);
    var regionBundle = strBundleService.createBundle("chrome://messenger-region/locale/region.properties");
    // the release notes are special and need to be formatted with the app version
    var urlToOpen;
    if (aResourceName == "releaseNotesURL")
      urlToOpen = regionBundle.formatStringFromName(aResourceName, [appInfo.version], 1);
    else
      urlToOpen = regionBundle.GetStringFromName(aResourceName);
      
    var uri = Components.classes["@mozilla.org/network/io-service;1"]
              .getService(Components.interfaces.nsIIOService)
              .newURI(urlToOpen, null, null);

    var protocolSvc = Components.classes["@mozilla.org/uriloader/external-protocol-service;1"]
                      .getService(Components.interfaces.nsIExternalProtocolService);
    protocolSvc.loadUrl(uri);
  } catch (ex) {}
}

/**
 *  Fetches the url for the passed in pref name, formats it and then loads it in the default
 *  browser.
 *
 *  @param aPrefName - name of the pref that holds the url we want to format and open
 */
function openFormattedURL(aPrefName)
{
  var formattedUrl = getFormattedURL(aPrefName);
  
  var uri = Components.classes["@mozilla.org/network/io-service;1"].
                       getService(Components.interfaces.nsIIOService).
                       newURI(formattedUrl, null, null);

  var protocolSvc = Components.classes["@mozilla.org/uriloader/external-protocol-service;1"].
                               getService(Components.interfaces.nsIExternalProtocolService);
  protocolSvc.loadUrl(uri);  
}

/**
 *  Fetches the url for the passed in pref name and uses the URL formatter service to 
 *    format  it.
 *
 *  @param aPrefName - name of the pref that holds the url we want to format and open
 *  @returns the formatted url string
 */
function getFormattedURL(aPrefName)
{
  var prefBranch = Components.classes["@mozilla.org/preferences-service;1"].getService(Components.interfaces.nsIPrefBranch);
  var format = prefBranch.getComplexValue(aPrefName, Components.interfaces.nsISupportsString).data;
  var formatter = Components.classes["@mozilla.org/toolkit/URLFormatterService;1"].
                             getService(Components.interfaces.nsIURLFormatter);
  return formatter.formatURL(format);
}
