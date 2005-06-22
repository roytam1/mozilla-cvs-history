# -*- Mode: Java; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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

const nsIUpdateItem       = Components.interfaces.nsIUpdateItem;

const PREF_UPDATE_EXTENSIONS_ENABLED            = "extensions.update.enabled";
const PREF_UPDATE_EXTENSIONS_AUTOUPDATEENABLED  = "extensions.update.autoUpdateEnabled";
const PREF_UPDATE_EXTENSIONS_COUNT              = "extensions.update.count";
const PREF_UPDATE_EXTENSIONS_SEVERITY_THRESHOLD = "extensions.update.severity.threshold";

const PREF_UPDATE_SEVERITY                      = "update.severity";

var gShowMismatch = null;
var gUpdateTypes  = null;

var gUpdateWizard = {
  // The items to check for updates for (e.g. an extension, some subset of extensions, 
  // all extensions, a list of compatible extensions, etc...)
  items: [],
  // The items that we found updates available for
  itemsToUpdate: [],
  // The items that we successfully installed updates for
  updatedCount: 0,
  shouldSuggestAutoChecking: false,
  shouldAutoCheck: false,
  
  remainingExtensionUpdateCount: 0,
  
  succeeded: true,
  
  init: function ()
  {
    gUpdateTypes = window.arguments[0];
    gShowMismatch = window.arguments[1];

    var items = window.arguments;
    if (window.arguments.length == 2) {
      var em = Components.classes["@mozilla.org/extensions/manager;1"]
                         .getService(Components.interfaces.nsIExtensionManager);
      items = [0,0].concat(em.getItemList(nsIUpdateItem.TYPE_ADDON, { }));
    }
    
    for (var i = 2; i < items.length; ++i)
      this.items.push(items[i].QueryInterface(nsIUpdateItem));

    var pref = Components.classes["@mozilla.org/preferences-service;1"]
                         .getService(Components.interfaces.nsIPrefBranch);
    this.shouldSuggestAutoChecking = gShowMismatch && 
                                     !pref.getBoolPref(PREF_UPDATE_EXTENSIONS_AUTOUPDATEENABLED);

    if (gShowMismatch) 
      gMismatchPage.init();
    else
      document.documentElement.advance();
  },
  
  uninit: function ()
  {
    // Ensure all observers are unhooked, just in case something goes wrong or the
    // user aborts. 
    gUpdatePage.uninit();  
  },
  
  onWizardFinish: function ()
  {
    var pref = Components.classes["@mozilla.org/preferences-service;1"]
                         .getService(Components.interfaces.nsIPrefBranch);
    if (this.shouldSuggestAutoChecking)
      pref.setBoolPref(PREF_EXTENSIONS_UPDATE_ENABLED, this.shouldAutoCheck); 
    
    if (this.succeeded) {
      // Downloading and Installed Extension
      this.clearExtensionUpdatePrefs();
    }

    // Send an event to refresh any FE notification components. 
    var os = Components.classes["@mozilla.org/observer-service;1"]
                       .getService(Components.interfaces.nsIObserverService);
    os.notifyObservers(null, "Update:Ended", "1");
  },
  
  clearExtensionUpdatePrefs: function ()
  {
    var pref = Components.classes["@mozilla.org/preferences-service;1"]
                         .getService(Components.interfaces.nsIPrefBranch);
    if (pref.prefHasUserValue(PREF_UPDATE_EXTENSIONS_COUNT)) 
      pref.clearUserPref(PREF_UPDATE_EXTENSIONS_COUNT);
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
  },
  
  /////////////////////////////////////////////////////////////////////////////
  // Update Errors
  errorItems: [],
  showErrors: function (aState, aErrors)
  {
    openDialog("chrome://mozapps/content/update/errors.xul", "", 
               "modal", { state: aState, errors: aErrors });
  },
  
  showUpdateCheckErrors: function ()
  {
    var errors = [];
    for (var i = 0; i < this.errorItems.length; ++i)
      errors.push({ name: this.errorItems[i].name, error: true, 
                    item: this.errorItems[i] });
    this.showErrors("checking", errors);
  },

  checkForErrors: function (aElementIDToShow)
  {
    if (this.errorOnGeneric || this.errorItems.length > 0)
      document.getElementById(aElementIDToShow).hidden = false;
  },
  
  onWizardClose: function (aEvent)
  {
    if (gInstallingPage._installing) {
      var os = Components.classes["@mozilla.org/observer-service;1"]
                         .getService(Components.interfaces.nsIObserverService);
      os.notifyObservers(null, "xpinstall-progress", "cancel");
      return false;
    }    
    return true;
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
  _messages: ["Update:Extension:Started", 
              "Update:Extension:Ended", 
              "Update:Extension:Item-Started", 
              "Update:Extension:Item-Ended",
              "Update:Extension:Item-Error",
              "Update:Ended"],
  
  onPageShow: function ()
  {
    gUpdateWizard.setButtonLabels(null, true, 
                                  "nextButtonText", true, 
                                  "cancelButtonText", false);
    document.documentElement.getButton("next").focus();

    var os = Components.classes["@mozilla.org/observer-service;1"]
                       .getService(Components.interfaces.nsIObserverService);
    for (var i = 0; i < this._messages.length; ++i)
      os.addObserver(this, this._messages[i], false);

    gUpdateWizard.errorItems = [];
    
    var em = Components.classes["@mozilla.org/extensions/manager;1"]
                       .getService(Components.interfaces.nsIExtensionManager);
    em.update(gUpdateWizard.items, gUpdateWizard.items.length, false);
  },

  _destroyed: false,  
  uninit: function ()
  {
    if (this._destroyed)
      return;
  
    var os = Components.classes["@mozilla.org/observer-service;1"]
                       .getService(Components.interfaces.nsIObserverService);
    for (var i = 0; i < this._messages.length; ++i)
      os.removeObserver(this, this._messages[i]);

    this._destroyed = true;
  },

  _totalCount: 0,
  get totalCount()
  {
    if (!this._totalCount) {
      this._totalCount = gUpdateWizard.items.length;
      if (this._totalCount == 0) {
        var em = Components.classes["@mozilla.org/extensions/manager;1"]
                           .getService(Components.interfaces.nsIExtensionManager);
        var extensionCount = em.getItemList(nsIUpdateItem.TYPE_EXTENSION, {}).length;
        var themeCount = em.getItemList(nsIUpdateItem.TYPE_THEME, {}).length;

        this._totalCount = extensionCount + themeCount;
      }
    }
    return this._totalCount;
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
      var item = aSubject.QueryInterface(Components.interfaces.nsIUpdateItem);
      if (aData == "update-check-success")
        gUpdateWizard.itemsToUpdate.push(item);
      ++this._completeCount;

      // Update the status text and progress bar
      var updateStrings = document.getElementById("updateStrings");
      var status = document.getElementById("checking.status");
      var statusString = updateStrings.getFormattedString("checkingPrefix", [item.name]);
      status.setAttribute("value", statusString);

      var progress = document.getElementById("checking.progress");
      progress.value = Math.ceil((this._completeCount / this.totalCount) * 100);
      
      break;
    case "Update:Extension:Item-Error":
      var item = aSubject.QueryInterface(Components.interfaces.nsIUpdateItem);
      gUpdateWizard.errorItems.push(item);
      ++this._completeCount;

      // Update the status text and progress bar
      var updateStrings = document.getElementById("updateStrings");
      var status = document.getElementById("checking.status");
      var statusString = updateStrings.getFormattedString("checkingPrefix", [item.name]);
      status.setAttribute("value", statusString);

      var progress = document.getElementById("checking.progress");
      progress.value = Math.ceil((this._completeCount / this.totalCount) * 100);

      break;
    case "Update:Extension:Ended":
      canFinish = gUpdateWizard.items.length > 0;
      break;
    case "Update:Ended":
      // If we're doing a general update check, (that is, no specific extensions/themes
      // were passed in for us to check for updates to), this notification means both
      // extension and app updates have completed.
      canFinish = true;
      break;
    }

    if (canFinish) {
      gUpdatePage.uninit();
      if ((gUpdateTypes & nsIUpdateItem.TYPE_ADDON && gUpdateWizard.itemsToUpdate.length > 0))
        document.getElementById("checking").setAttribute("next", "found");
      document.documentElement.advance();
    }
  }
};

var gFoundPage = {
  _nonAppItems: [],
  
  _newestInfo: null,

  buildAddons: function ()
  {
    var hasExtensions = false;
    var foundAddonsList = document.getElementById("found.addons.list");
    var uri = Components.classes["@mozilla.org/network/standard-url;1"]
                        .createInstance(Components.interfaces.nsIURI);
    var itemCount = gUpdateWizard.itemsToUpdate.length;
    for (var i = 0; i < itemCount; ++i) {
      var item = gUpdateWizard.itemsToUpdate[i];
      var checkbox = document.createElement("checkbox");
      foundAddonsList.appendChild(checkbox);
      checkbox.setAttribute("type", "update");
      checkbox.label        = item.name + " " + item.version;
      checkbox.setAttribute("URL", item.xpiURL);
      checkbox.infoURL      = "";
      checkbox.internalName = "";
      uri.spec              = item.xpiURL;
      checkbox.setAttribute("source", uri.host);
      checkbox.checked      = true;
      hasExtensions         = true;
    }

    if (hasExtensions) {
      var addonsHeader = document.getElementById("addons");
      var strings = document.getElementById("updateStrings");
      addonsHeader.label = strings.getFormattedString("updateTypeExtensions", [itemCount]);
      addonsHeader.collapsed = false;
    }
  },

  _initialized: false,
  onPageShow: function ()
  {
    gUpdateWizard.setButtonLabels(null, true, 
                                  "installButtonText", false, 
                                  null, false);
    document.documentElement.getButton("next").focus();
    
    var updates = document.getElementById("found.updates");
    if (!this._initialized) {
      this._initialized = true;
      
      updates.computeSizes();

      if (gUpdateTypes & nsIUpdateItem.TYPE_ADDON)
        this.buildAddons();
    }
        
    var kids = updates._getRadioChildren();
    for (var i = 0; i < kids.length; ++i) {
      if (kids[i].collapsed == false) {
        updates.selectedIndex = i;
        break;
      }
    }
  },
    
  onSelect: function (aEvent)
  {
    var updates = document.getElementById("found.updates");
    var oneChecked = false;
    var items = updates.selectedItem.getElementsByTagName("checkbox");
    for (var i = 0; i < items.length; ++i) {  
      if (items[i].checked) {
        oneChecked = true;
        break;
      }
    }

    var strings = document.getElementById("updateStrings");
    gUpdateWizard.setButtonLabels(null, true, 
                                  "installButtonText", true, 
                                  null, false);
    var text = strings.getString("foundInstructions");
    document.getElementById("found").setAttribute("next", "installing"); 
    document.documentElement.getButton("next").disabled = !oneChecked;

    var foundInstructions = document.getElementById("foundInstructions");
    while (foundInstructions.hasChildNodes())
      foundInstructions.removeChild(foundInstructions.firstChild);
    foundInstructions.appendChild(document.createTextNode(text));
  }
};

var gInstallingPage = {
  _installing       : false,
  _restartRequired  : false,
  _objs             : [],
  
  onPageShow: function ()
  {
    gUpdateWizard.setButtonLabels(null, true, 
                                  "nextButtonText", true, 
                                  null, true);

    // Get XPInstallManager and kick off download/install 
    // process, registering us as an observer. 
    var items = [];
    this._objs = [];
    
    this._restartRequired = false;
    
    gUpdateWizard.remainingExtensionUpdateCount = gUpdateWizard.itemsToUpdate.length;

    var updates = document.getElementById("found.updates");
    var checkboxes = updates.selectedItem.getElementsByTagName("checkbox");
    for (var i = 0; i < checkboxes.length; ++i) {
      if (checkboxes[i].type == "update" && checkboxes[i].checked) {
        items.push(checkboxes[i].URL);
        this._objs.push({ name: checkboxes[i].label });
      }
    }
    
    var xpimgr = Components.classes["@mozilla.org/xpinstall/install-manager;1"]
                           .createInstance(Components.interfaces.nsIXPInstallManager);
    xpimgr.initManagerFromChrome(items, items.length, this);
  },
  
  /////////////////////////////////////////////////////////////////////////////
  // nsIXPIProgressDialog
  onStateChange: function (aIndex, aState, aValue)
  {
    var strings = document.getElementById("updateStrings");

    const nsIXPIProgressDialog = Components.interfaces.nsIXPIProgressDialog;
    switch (aState) {
    case nsIXPIProgressDialog.DOWNLOAD_START:
      var label = strings.getFormattedString("downloadingPrefix", [this._objs[aIndex].name]);
      var actionItem = document.getElementById("actionItem");
      actionItem.value = label;
      break;
    case nsIXPIProgressDialog.DOWNLOAD_DONE:
    case nsIXPIProgressDialog.INSTALL_START:
      var label = strings.getFormattedString("installingPrefix", [this._objs[aIndex].name]);
      var actionItem = document.getElementById("actionItem");
      actionItem.value = label;
      this._installing = true;
      break;
    case nsIXPIProgressDialog.INSTALL_DONE:
      switch (aValue) {
      case 999: 
        this._restartRequired = true;
        break;
      case 0: 
        --gUpdateWizard.remainingExtensionUpdateCount;
        break;
      }
      break;
    case nsIXPIProgressDialog.DIALOG_CLOSE:
      this._installing = false;
      var nextPage = this._errors ? "errors" : (this._restartRequired ? "restart" : "finished");
      document.getElementById("installing").setAttribute("next", nextPage);
      document.documentElement.advance();
      break;
    }
  },
  
  _objs: [],
  _errors: false,
  
  onProgress: function (aIndex, aValue, aMaxValue)
  {
    var downloadProgress = document.getElementById("downloadProgress");
    downloadProgress.value = Math.ceil((aValue/aMaxValue) * 100);
  }
};

var gErrorsPage = {
  onPageShow: function ()
  {
    document.documentElement.getButton("finish").focus();
    gUpdateWizard.succeeded = false;
  },
  
  onShowErrors: function ()
  {
    gUpdateWizard.showErrors("install", gInstallingPage._objs);
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
    
    if (gShowMismatch) {
      document.getElementById("finishedMismatch").hidden = false;
      document.getElementById("incompatibleAlert").hidden = false;
    }
  }
};

var gNoUpdatesPage = {
  onPageShow: function (aEvent)
  {
    gUpdateWizard.setButtonLabels(null, true, null, true, null, true);
    document.documentElement.getButton("finish").focus();
    if (gShowMismatch) {
      document.getElementById("introUser").hidden = true;
      document.getElementById("introMismatch").hidden = false;
      document.getElementById("mismatchNoUpdates").hidden = false;
        
      if (gUpdateWizard.shouldSuggestAutoChecking) {
        document.getElementById("mismatchIncompatibleRemaining").hidden = true;
        document.getElementById("mismatchIncompatibleRemaining2").hidden = false;
        document.getElementById("mismatchFinishedEnableChecking").hidden = false;
      }
    }

    gUpdateWizard.succeeded = false;
    gUpdateWizard.checkForErrors("updateCheckErrorNotFound");
  }
};

