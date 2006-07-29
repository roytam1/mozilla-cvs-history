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
 * Contributor(s): Michael Lowe <michael.lowe@bigfoot.com>
 *                 Blake Ross   <blakeross@telocity.com>
 */

var browser;
var dialog;
var bundle;
var pref = null;
try {
  pref = Components.classes["@mozilla.org/preferences;1"]
                   .getService(Components.interfaces.nsIPref);
} catch (ex) {
  // not critical, remain silent
}

function onLoad()
{
  bundle = srGetStrBundle("chrome://communicator/locale/openLocation.properties");

  dialog                = new Object;
  dialog.input          = document.getElementById("dialog.input");
  dialog.help           = document.getElementById("dialog.help");
  dialog.open           = document.getElementById("ok");
  dialog.openAppList    = document.getElementById("openAppList");
  dialog.openTopWindow  = document.getElementById("currentWindow");
  dialog.openEditWindow = document.getElementById("editWindow");

  browser = window.arguments[0];
  if (!browser) {
    // No browser supplied - we are calling from Composer
    dialog.openAppList.selectedItem = dialog.openEditWindow;
    dialog.openTopWindow.setAttribute("disabled", "true");
  }
  else {
    dialog.openAppList.selectedItem = dialog.openTopWindow;
  }

  // change OK button text to 'open'
  dialog.open.label = bundle.GetStringFromName("openButtonLabel");

  doSetOKCancel(open, 0, 0, 0);

  dialog.input.focus();
  if (pref) {
    try {
      var value = pref.GetIntPref("general.open_location.last_window_choice");
      var element = dialog.openAppList.getElementsByAttribute("value", value)[0];
      if (element)
        dialog.openAppList.selectedItem = element;
      dialog.input.value = pref.CopyUnicharPref("general.open_location.last_url");
    }
    catch(ex) {
    }
    if (dialog.input.value)
      dialog.input.select(); // XXX should probably be done automatically
  }

  doEnabling();
}

function doEnabling()
{
    dialog.open.disabled = !dialog.input.value;
}

function open()
{
  var url = browser.getShortcutOrURI(dialog.input.value);
  try {
    switch (dialog.openAppList.value) {
      case "0":
        browser.loadURI(url);
        break;
      case "1":
        window.opener.delayedOpenWindow(getBrowserURL(), "all,dialog=no", url);
        break;
      case "2":
        // editPage is in utilityOverlay.js (all editor openers with URL should use this)
        // 3rd param tells editPage to use "delayedOpenWindow"
        editPage(url, window.opener, true);
        break;
    }
  }
  catch(exception) {
  }

  if (pref) {
    pref.SetUnicharPref("general.open_location.last_url", dialog.input.value);
    pref.SetIntPref("general.open_location.last_window_choice", dialog.openAppList.value);
  }

  // Delay closing slightly to avoid timing bug on Linux.
  window.close();
  return false;
}

function createInstance(contractid, iidName)
{
  var iid = Components.interfaces[iidName];
  return Components.classes[contractid].createInstance(iid);
}

const nsIFilePicker = Components.interfaces.nsIFilePicker;
function onChooseFile()
{
  try {
    var fp = Components.classes["@mozilla.org/filepicker;1"].createInstance(nsIFilePicker);
    fp.init(window, bundle.GetStringFromName("chooseFileDialogTitle"), nsIFilePicker.modeOpen);

    if (dialog.openAppList.value == "2") {
      // When loading into Composer, direct user to prefer HTML files and text files,
      // so we call separately to control the order of the filter list
      fp.appendFilters(nsIFilePicker.filterHTML | nsIFilePicker.filterText);
      fp.appendFilters(nsIFilePicker.filterText);
      fp.appendFilters(nsIFilePicker.filterAll);
    }
    else {
      fp.appendFilters(nsIFilePicker.filterHTML | nsIFilePicker.filterText |
                       nsIFilePicker.filterAll | nsIFilePicker.filterImages | nsIFilePicker.filterXML);
    }

    if (fp.show() == nsIFilePicker.returnOK && fp.fileURL.spec && fp.fileURL.spec.length > 0)
      dialog.input.value = fp.fileURL.spec;
  }
  catch(ex) {
  }
  doEnabling();
}
