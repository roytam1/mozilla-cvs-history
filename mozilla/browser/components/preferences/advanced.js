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
# The Original Code is the Firefox Preferences System.
# 
# The Initial Developer of the Original Code is Ben Goodger.
# Portions created by the Initial Developer are Copyright (C) 2005
# the Initial Developer. All Rights Reserved.
# 
# Contributor(s):
#   Ben Goodger <ben@mozilla.org>
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

var gAdvancedPane = {
  _inited: false,
  init: function ()
  {
    this._inited = true;
    var advancedPrefs = document.getElementById("advancedPrefs");
    var preference = document.getElementById("browser.preferences.advanced.selectedTabIndex");
    if (preference.value === null)
      return;
    advancedPrefs.selectedIndex = preference.value;
  },
  
  tabSelectionChanged: function ()
  {
    if (!this._inited)
      return;
    var advancedPrefs = document.getElementById("advancedPrefs");
    var preference = document.getElementById("browser.preferences.advanced.selectedTabIndex");
    preference.valueFromPreferences = advancedPrefs.selectedIndex;
  },
  
  updateButtons: function (aButtonID, aPreferenceID)
  {
    var button = document.getElementById(aButtonID);
    var preference = document.getElementById(aPreferenceID);
    // This is actually before the value changes, so the value is not as you expect. 
    button.disabled = preference.value == true;
    return undefined;
  },

  showCertificates: function ()
  {
    document.documentElement.openWindow("mozilla:certmanager",
                                        "chrome://pippki/content/certManager.xul",
                                        "width=600,height=400", null);
  },
  
  showCRLs: function ()
  {
    document.documentElement.openWindow("Mozilla:CRLManager", 
                                        "chrome://pippki/content/crlManager.xul",
                                        "width=600,height=400", null);
  },
  
  showOCSP: function ()
  {
    document.documentElement.openSubDialog("chrome://browser/content/preferences/ocsp.xul",
                                           "", null);
  },
  
  showSecurityDevices: function ()
  {
    document.documentElement.openWindow("mozilla:devicemanager",
                                        "chrome://pippki/content/device_manager.xul",
                                        "width=600,height=400", null);
  },
  
  updateAppUpdateUI: function ()
  {
    var preference = document.getElementById("app.update.autoUpdateEnabled");
    var ids = ["enableAutoInstall", "autoInstallMode", "updateAnd"];
    if (!preference.value)
      for (var i = 0; i < ids.length; ++i)
        document.getElementById(ids[i]).disabled = true;
    else {
      document.getElementById("enableAutoInstall").disabled = false;
      this.updateAutoInstallUI();
    }
    return undefined;
  },
  
  updateAutoInstallUI: function ()
  {
    var preference = document.getElementById("app.update.autoInstallEnabled");
    var ids = ["autoInstallMode", "updateAnd"];
    for (var i = 0; i < ids.length; ++i)
      document.getElementById(ids[i]).disabled = !preference.value;
    return undefined;
  },
  
  checkForUpdates: function (aType)
  {
    var updates = Components.classes["@mozilla.org/updates/update-service;1"]
                            .getService(Components.interfaces.nsIUpdateService);
    updates.checkForUpdates([], 0, aType, 
                            Components.interfaces.nsIUpdateService.SOURCE_EVENT_USER,
                            null);  
  },
  
  showLanguages: function ()
  {
    document.documentElement.openSubDialog("chrome://browser/content/preferences/languages.xul",
                                           "", null);  
  }
};

