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
 * Alec Flett <alecf@netscape.com>
 */
// The returnmycall is used as a global variable that is set during a callback.
var returnmycall=false;
var accountManagerContractID   = "@mozilla.org/messenger/account-manager;1";
var messengerMigratorContractID   = "@mozilla.org/messenger/migrator;1";

// returns the first account with an invalid server or identity

function getInvalidAccounts(accounts)
{
    var numAccounts = accounts.Count();
    var invalidAccounts = new Array;
    for (var i=0; i<numAccounts; i++) {
        var account = accounts.QueryElementAt(i, Components.interfaces.nsIMsgAccount);
        try {
            if (!account.incomingServer.valid) {
                invalidAccounts[invalidAccounts.length] = account;
                // skip to the next account
                continue;
            }
        } catch (ex) {
            // this account is busted, just keep going
            continue;
        }

        var identities = account.identities;
        var numIdentities = identities.Count();

        for (var j=0; j<numIdentities; j++) {
            var identity = identities.QueryElementAt(j, Components.interfaces.nsIMsgIdentity);
            if (!identity.valid) {
                invalidAccounts[invalidAccounts.length] = account;
                continue;
            }
        }
    }

    return invalidAccounts;
}

// This function gets called from verifyAccounts.
// We do not have to do anything on
// unix and mac but on windows we have to bring up a 
// dialog based on the settings the user has.
// This will bring up the dialog only once per session and only if we
// are not the default mail client.
function showMailIntegrationDialog() {
      try {
        var mapiRegistry = Components.classes[ "@mozilla.org/mapiregistry;1" ].
                       getService( Components.interfaces.nsIMapiRegistry );
    }
    catch (ex) { 
        mapiRegistry = null;
    }
    // showDialog is TRUE only if we did not bring up this dialog already
    // and we are not the default mail client
    var prefLocked = false;
    if (mapiRegistry && mapiRegistry.showDialog) {
        const prefbase = "system.windows.lock_ui.";
        try {
            prefService = Components.classes["@mozilla.org/preferences-service;1"]
                          .getService()
                          .QueryInterface(Components.interfaces.nsIPrefService);
            prefBranch = prefService.getBranch(prefbase);
        
            if (prefBranch && prefBranch.prefIsLocked("defaultMailClient")) {
                prefLocked = true;
                if (prefBranch.getBoolPref("defaultMailClient"))
                    mapiRegistry.setDefaultMailClient();
                else
                    mapiRegistry.unsetDefaultMailClient();
            }
        }
        catch (ex) {}
        if (!prefLocked && !mapiRegistry.isDefaultMailClient)
            mapiRegistry.showMailIntegrationDialog();
    }
}

function verifyAccounts(wizardcallback) {
//check to see if the function is called with the callback and if so set the global variable returnmycall to true
    if(wizardcallback)
		returnmycall = true;
	var openWizard = false;
    var prefillAccount;
	var state=true;
	var ret = true;
    
    try {
        var am = Components.classes[accountManagerContractID].getService(Components.interfaces.nsIMsgAccountManager);

        var accounts = am.accounts;

        // as long as we have some accounts, we're fine.
        var accountCount = accounts.Count();
        var invalidAccounts = getInvalidAccounts(accounts);
        if (invalidAccounts.length > 0) {
            prefillAccount = invalidAccounts[0];
        } else {
        }

        // if there are no accounts, or all accounts are "invalid"
        // then kick off the account migration
        if (accountCount == invalidAccounts.length) {
            try {
                  var messengerMigrator = Components.classes[messengerMigratorContractID].getService(Components.interfaces.nsIMessengerMigrator); 
                  messengerMigrator.UpgradePrefs();
                  // if there is a callback mechanism then inform parent window to shut itself down
                  if (wizardcallback){
                      state = false;
                      WizCallback(state);
                  }
                  ret = false;
            }
            catch (ex) {
                  // upgrade prefs failed, so open account wizard
                  openWizard = true;
            }
        }

        if (openWizard || prefillAccount) {
            MsgAccountWizard(prefillAccount);
		        ret = false;
        }
        showMailIntegrationDialog();
        return ret;
    }
    catch (ex) {
        dump("error verifying accounts " + ex + "\n");
        return false;
    }
}

// we do this from a timer because if this is called from the onload=
// handler, then the parent window doesn't appear until after the wizard
// has closed, and this is confusing to the user
function MsgAccountWizard()
{
    setTimeout("msgOpenAccountWizard();", 0);
}

function msgOpenAccountWizard()
{
// Check to see if the verify accounts function was called with callback or not.
  if (returnmycall)
      window.openDialog("chrome://messenger/content/AccountWizard.xul",
                        "AccountWizard", "chrome,modal,titlebar,resizable", {okCallback:WizCallback});
  else
      window.openDialog("chrome://messenger/content/AccountWizard.xul",
                        "AccountWizard", "chrome,modal,titlebar,resizable");

}

// selectPage: the xul file name for the viewing page, 
// null for the account main page, other pages are
// 'am-server.xul', 'am-copies.xul', 'am-offline.xul', 
// 'am-addressing.xul','am-advanced.xul', 'am-smtp.xul'
function MsgAccountManager(selectPage)
{
    var server;
    try {
        var folderURI = GetSelectedFolderURI();
        server = GetServer(folderURI);
    } catch (ex) { /* functions might not be defined */}
    
    window.openDialog("chrome://messenger/content/AccountManager.xul",
                      "AccountManager", "chrome,modal,titlebar,resizable",
                      { server: server, selectPage: selectPage });
}
