//
// window.arguments[1...] is an array of nsIUpdateItem implementing objects 
// that are to be updated. 
//  * if the array is empty, all items are updated (like a background update
//    check)
//  * if the array contains one or two UpdateItems, with null id fields, 
//    all items of that /type/ are updated.
//
// This UI can be opened from the following places, and in the following modes:
//
// - from the Version Compatibility Checker at startup
//    as the application starts a check is done to see if the application being
//    started is the same version that was started last time. If it isn't, a
//    list of UpdateItems that are incompatible with the verison being 
//    started is generated and this UI opened with that list. This UI then
//    offers to check for updates to those UpdateItems that are compatible
//    with the version of the application being started. 
//    
//    In this mode, the wizard is opened to panel 'mismatch'. 
//
// - from the Extension Manager or Options Dialog or any UI where the user
//   directly requests to check for updates.
//    in this case the user selects UpdateItem(s) to update and this list
//    is passed to this UI. If a single item is sent with the id field set to
//    null but the type set correctly, this UI will check for updates to all 
//    items of that type.
//
//    In this mode, the wizard is opened to panel 'checking'.
//
// - from the Updates Available Status Bar notification.
//    in this case the background update checking has determined there are new
//    updates that can be installed and has prepared a list for the user to see.
//    They can update by clicking on a status bar icon which passes the list
//    to this UI which lets them choose to install updates. 
//
//    In this mode, the wizard is opened to panel 'updatesFound' if the data
//    set is immediately available, or 'checking' if the user closed the browser
//    since the last background check was performed and the check needs to be
//    performed again. 
//

const nsIUpdateItem                     = Components.interfaces.nsIUpdateItem;
const nsIUpdateService                  = Components.interfaces.nsIUpdateService;
const nsIExtensionManager               = Components.interfaces.nsIExtensionManager;

const PREF_APP_ID                       = "app.id";
const PREF_UPDATE_APP_UPDATESAVAILABLE  = "update.app.updatesAvailable";

var gSourceEvent = null;
var gUpdateTypes = null;

var gUpdateWizard = {
  // The items to check for updates for (e.g. an extension, some subset of extensions, 
  // all extensions, a list of compatible extensions, etc...)
  items: [],
  // The items that we found updates available for
  itemsToUpdate: [],
  // The items that we successfully installed updates for
  updatedCount: 0,
  appUpdatesAvailable: false,

  shouldSuggestAutoChecking: false,
  shouldAutoCheck: false,
  
  updatingApp: false,
  
  init: function ()
  {
    gUpdateTypes = window.arguments[0];
    gSourceEvent = window.arguments[1];
    for (var i = 2; i < window.arguments.length; ++i)
      this.items.push(window.arguments[i].QueryInterface(nsIUpdateItem));

    var pref = Components.classes["@mozilla.org/preferences-service;1"]
                         .getService(Components.interfaces.nsIPrefBranch);
    this.shouldSuggestAutoChecking = (gSourceEvent == nsIUpdateService.SOURCE_EVENT_MISMATCH) && 
                                      !pref.getBoolPref("update.extensions.enabled");

    if (gSourceEvent == nsIUpdateService.SOURCE_EVENT_USER) {
      document.getElementById("mismatch").setAttribute("next", "checking");
      document.documentElement.advance();
    }
    
    gMismatchPage.init();
  },
  
  uninit: function ()
  {
    gUpdatePage.uninit();
  },
  
  onWizardFinish: function ()
  {
    if (this.shouldSuggestAutoChecking) {
      var pref = Components.classes["@mozilla.org/preferences-service;1"]
                           .getService(Components.interfaces.nsIPrefBranch);
      pref.setBoolPref("update.extensions.enabled", this.shouldAutoCheck); 
    }
    
    if (this.updatingApp) {
      var updates = Components.classes["@mozilla.org/updates/update-service;1"]
                              .getService(Components.interfaces.nsIUpdateService);
# If we're not a browser, use the external protocol service to load the URI.
#ifndef MOZ_PHOENIX
      var uri = Components.classes["@mozilla.org/network/standard-url;1"]
                          .createInstance(Components.interfaces.nsIURI);
      uri.spec = updates.appUpdateURL;

      var protocolSvc = Components.classes["@mozilla.org/uriloader/external-protocol-service;1"]
                                  .getService(Components.interfaces.nsIExternalProtocolService);
      if (protocolSvc.isExposedProtocol(uri.scheme))
        protocolSvc.loadUrl(uri);
# If we're a browser, open a new browser window instead.    
#else
      var ww = Components.classes["@mozilla.org/embedcomp/window-watcher;1"]
                        .getService(Components.interfaces.nsIWindowWatcher);
      var ary = Components.classes["@mozilla.org/supports-array;1"]
                          .createInstance(Components.interfaces.nsISupportsArray);
      var url = Components.classes["@mozilla.org/supports-string;1"]
                          .createInstance(Components.interfaces.nsISupportsString);
      url.data = updates.appUpdateURL;
      ary.AppendElement(url);
      ww.openWindow(null, "chrome://browser/content/browser.xul",
                    "_blank", "chrome,all,dialog=no", ary);
#endif
    }
  },
  
  _setUpButton: function (aButtonID, aButtonKey, aDisabled)
  {
    var strings = document.getElementById("updateStrings");
    var button = document.documentElement.getButton(aButtonID);
    if (aButtonKey) {
      button.label = strings.getString(aButtonKey);
      try {
        button.accesskey = strings.getString(aButtonKey + "Accesskey");
      }
      catch (e) {
      }
    }
    button.disabled = aDisabled;
  },
  
  setButtonLabels: function (aBackButton, aBackButtonIsDisabled, 
                             aNextButton, aNextButtonIsDisabled,
                             aCancelButton, aCancelButtonIsDisabled)
  {
    
    this._setUpButton("back", aBackButton, aBackButtonIsDisabled);
    this._setUpButton("next", aNextButton, aNextButtonIsDisabled);
    this._setUpButton("cancel", aCancelButton, aCancelButtonIsDisabled);
  }
};


var gMismatchPage = {
  init: function ()
  {
    var incompatible = document.getElementById("mismatch.incompatible");
    for (var i = 0; i < gUpdateWizard.items.length; ++i) {
      var item = gUpdateWizard.items[i];
      var listitem = document.createElement("listitem");
      listitem.setAttribute("label", item.name + " " + item.version);
      incompatible.appendChild(listitem);
    }
  },
  
  onPageShow: function ()
  {
    gUpdateWizard.setButtonLabels(null, true, 
                                  "mismatchCheckNow", false, 
                                  "mismatchDontCheck", false);
    document.documentElement.getButton("next").focus();
  }
};

var gUpdatePage = {
  _completeCount: 0,
  _updateState: 0,
  _messages: ["Update:Extension:Started", 
              "Update:Extension:Ended", 
              "Update:Extension:Item-Started", 
              "Update:Extension:Item-Ended",
              "Update:Extension:Item-Error",
              "Update:App:Ended",
              "Update:Ended"],
  
  onPageShow: function ()
  {
    gUpdateWizard.setButtonLabels(null, true, 
                                  "nextButtonText", true, 
                                  "cancelButtonText", true);
    document.documentElement.getButton("next").focus();

    var os = Components.classes["@mozilla.org/observer-service;1"]
                       .getService(Components.interfaces.nsIObserverService);
    for (var i = 0; i < this._messages.length; ++i)
      os.addObserver(this, this._messages[i], false);

    var updates = Components.classes["@mozilla.org/updates/update-service;1"]
                            .getService(Components.interfaces.nsIUpdateService);
    updates.checkForUpdatesInternal(gUpdateWizard.items, gUpdateWizard.items.length, 
                                    gUpdateTypes, gSourceEvent);

    this._updateState = nsIUpdateService.UPDATED_NONE;
  },
  
  uninit: function ()
  {
    var os = Components.classes["@mozilla.org/observer-service;1"]
                       .getService(Components.interfaces.nsIObserverService);
    for (var i = 0; i < this._messages.length; ++i)
      os.removeObserver(this, this._messages[i]);
  },
  
  observe: function (aSubject, aTopic, aData)
  {
    var canFinish = false;
    switch (aTopic) {
    case "Update:Extension:Started":
      break;
    case "Update:Extension:Item-Started":
      break;
    case "Update:Extension:Item-Ended":
      gUpdateWizard.itemsToUpdate.push(aSubject);
      
      ++this._completeCount;
      
      var progress = document.getElementById("checking.progress");
      progress.value = Math.ceil(this._completeCount / gUpdateWizard.itemsToUpdate.length) * 100;
      
      break;
    case "Update:Extension:Ended":
      // If we were passed a set of extensions/themes/other to update, this
      // means we're not checking for app updates, so don't wait for the app
      // update to complete before advancing (because there is none).
      // canFinish = gUpdateWizard.items.length > 0;
      // XXXben
      break;
    case "Update:Ended":
      // If we're doing a general update check, (that is, no specific extensions/themes
      // were passed in for us to check for updates to), this notification means both
      // extension and app updates have completed.
      canFinish = true;
      break;
    case "Update:App:Ended":
      // The "Updates Found" page of the update wizard needs to know if it there are app 
      // updates so it can list them first. 
      var pref = Components.classes["@mozilla.org/preferences-service;1"]
                           .getService(Components.interfaces.nsIPrefBranch);
      gUpdateWizard.appUpdatesAvailable = pref.getBoolPref(PREF_UPDATE_APP_UPDATESAVAILABLE);
      
      if (gUpdateWizard.appUpdatesAvailable) {
        var appID = pref.getCharPref(PREF_APP_ID);
        var updates = Components.classes["@mozilla.org/updates/update-service;1"]
                                .getService(Components.interfaces.nsIUpdateService);
        
        
        var brandShortName = document.getElementById("brandStrings").getString("brandShortName");
        var item = Components.classes["@mozilla.org/updates/item;1"]
                             .createInstance(Components.interfaces.nsIUpdateItem);
        item.init(appID, updates.appUpdateVersion,
                  brandShortName, -1, updates.appUpdateURL, 
                  "chrome://mozapps/skin/update/icon32.png", 
                  nsIUpdateItem.TYPE_APP);
        gUpdateWizard.itemsToUpdate.splice(0, 0, item);
      }
      break;
    }

    if (canFinish) {    
      if (gUpdateWizard.itemsToUpdate.length > 0 || gUpdateWizard.appUpdatesAvailable)
        document.getElementById("checking").setAttribute("next", "found");
      document.documentElement.advance();
    }
  }
};

var gFoundPage = {
  _appUpdateExists: false,
  _appSelected: false, 
  _appItem: null,
  _nonAppItems: [],
  onPageShow: function ()
  {
    gUpdateWizard.setButtonLabels(null, true, 
                                  "installButtonText", false, 
                                  null, false);
    document.documentElement.getButton("next").focus();
    
    var list = document.getElementById("foundList");
    for (var i = 0; i < gUpdateWizard.itemsToUpdate.length; ++i) {
      var updateitem = document.createElement("updateitem");
      list.appendChild(updateitem);
      
      var item = gUpdateWizard.itemsToUpdate[i];
      updateitem.name = item.name + " " + item.version;
      updateitem.url = item.updateURL;

      // If we have an App entry in the list, check it and uncheck
      // the others since the two are mutually exclusive installs.
      updateitem.type = item.type;
      if (item.type == nsIUpdateItem.TYPE_APP) {
        updateitem.checked = true;
        this._appUpdateExists = true;
        this._appSelected = true;
        this._appItem = updateitem;
        document.getElementById("found").setAttribute("next", "appupdate");
      }
      else  {
        updateitem.checked = !this._appUpdateExists;
        this._nonAppItems.push(updateitem);
      }

      if (item.iconURL != "")
        updateitem.icon = item.iconURL;
    }
  },
  
  onCommand: function (aEvent)
  {
    var i;
    if (this._appUpdateExists) {
      if (aEvent.target.type == nsIUpdateItem.TYPE_APP) {
        for (i = 0; i < this._nonAppItems.length; ++i) {
          var nonAppItem = this._nonAppItems[i];
          nonAppItem.checked = !aEvent.target.checked;
        }
        document.getElementById("found").setAttribute("next", "appupdate");
      }
      else {
        this._appItem.checked = false;
        document.getElementById("found").setAttribute("next", "installing");
      }
    }
    
    var next = document.documentElement.getButton("next");
    next.disabled = true;
    var foundList = document.getElementById("foundList");
    for (i = 0; i < foundList.childNodes.length; ++i) {
      var listitem = foundList.childNodes[i];
      if (listitem.checked) {
        next.disabled = false;
        break;
      }
    }
  }
};

var gAppUpdatePage = {
  onPageShow: function ()
  {
    gUpdateWizard.setButtonLabels(null, true, 
                                  null, true, 
                                  null, true);
    gUpdateWizard.updatingApp = true;

    document.documentElement.getButton("finish").focus();
  }
};

var gInstallingPage = {
  onPageShow: function ()
  {
    gUpdateWizard.setButtonLabels(null, true, 
                                  "nextButtonText", true, 
                                  null, true);

    // Get XPInstallManager and kick off download/install 
    // process, registering us as an observer. 
    
    //XXXben
    window.advance = function()
    {
      document.getElementById("installing").setAttribute("next", "finished");
      document.documentElement.advance();
    }
    setTimeout("advance()", 2000);
  }
};

var gFinishedPage = {

  onPageShow: function ()
  {
    gUpdateWizard.setButtonLabels(null, true, null, true, null, true);
    document.documentElement.getButton("finish").focus();
    
    var iR = document.getElementById("incompatibleRemaining");
    var iR2 = document.getElementById("incompatibleRemaining2");
    var fEC = document.getElementById("finishedEnableChecking");

    if (gUpdateWizard.shouldSuggestAutoChecking) {
      iR.hidden = true;
      iR2.hidden = false;
      fEC.hidden = false;
      fEC.click();
    }
    else {
      iR.hidden = false;
      iR2.hidden = true;
      fEC.hidden = true;
    }

    if (gSourceEvent == nsIUpdateService.SOURCE_EVENT_MISMATCH) {
      document.getElementById("finishedMismatch").hidden = false;
      document.getElementById("incompatibleAlert").hidden = false;
    }
  }
};


# -*- Mode: Java; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
# The Original Code is The Update Service.
# 
# The Initial Developer of the Original Code is Ben Goodger.
# Portions created by the Initial Developer are Copyright (C) 2004
# the Initial Developer. All Rights Reserved.
# 
# Contributor(s):
#   Ben Goodger <ben@bengoodger.com>
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
