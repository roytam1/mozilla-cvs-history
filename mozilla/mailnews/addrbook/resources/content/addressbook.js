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
 * The Original Code is Mozilla Addressbook.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corp.
 * Portions created by the Initial Developer are Copyright (C) 1999-2001
 * the Initial Developer. All Rights Reserved.
 *
 * Original Author:
 *   Paul Hangas <hangas@netscape.com>
 *
 * Contributor(s):
 *   Seth Spitzer <sspitzer@netscape.com>
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

var cvPrefs = 0;
var addressbook = 0;
var gAddressBookBundle;
var gSearchTimer = null;
var gStatusText = null;
var gQueryURIFormat = null;
var gSearchInput;
var gPrintSettings = null;

// Constants that correspond to choices
// in Address Book->View -->Show Name as
const kDisplayName = 0;
const kLastNameFirst = 1;
const kFirstNameFirst = 2;

var gAddressBookAbListener = {
  onItemAdded: function(parentDir, item) {
    // will not be called
  },
  onItemRemoved: function(parentDir, item) {
    // will only be called when an addressbook is deleted
    try {
      var directory = item.QueryInterface(Components.interfaces.nsIAbDirectory);
      // check if the item being removed is the directory
      // that we are showing in the addressbook
      // if so, select the personal addressbook (it can't be removed)
      if (directory && directory == GetAbView().directory) {
        SelectFirstAddressBook();
      }
    }
    catch (ex) {
    }
  },
  onItemPropertyChanged: function(item, property, oldValue, newValue) {
    // will not be called
  }
};

function OnUnloadAddressBook()
{  
  var addrbookSession = Components.classes["@mozilla.org/addressbook/services/session;1"].getService().QueryInterface(Components.interfaces.nsIAddrBookSession);
  addrbookSession.removeAddressBookListener(gAddressBookAbListener);

  RemovePrefObservers();
  CloseAbView();
}

var gAddressBookAbViewListener = {
  onSelectionChanged: function() {
    ResultsPaneSelectionChanged();
  },
  onCountChanged: function(total) {
    SetStatusText(total);
  }
};

function GetAbViewListener()
{
  return gAddressBookAbViewListener;
}

const kPrefMailAddrBookLastNameFirst = "mail.addr_book.lastnamefirst";

var gMailAddrBookLastNameFirstObserver = {
  observe: function(subject, topic, value) {
    if (topic == "nsPref:changed" && value == kPrefMailAddrBookLastNameFirst) {
      UpdateCardView();
    }
  }
}

function AddPrefObservers()
{
  var prefService = Components.classes["@mozilla.org/preferences-service;1"]
                              .getService(Components.interfaces.nsIPrefService);
  var prefBranch = prefService.getBranch(null).QueryInterface(Components.interfaces.nsIPrefBranchInternal);
  prefBranch.addObserver(kPrefMailAddrBookLastNameFirst, gMailAddrBookLastNameFirstObserver, false);
}

function RemovePrefObservers()
{
  var prefService = Components.classes["@mozilla.org/preferences-service;1"]
                              .getService(Components.interfaces.nsIPrefService);
  var prefBranch = prefService.getBranch(null).QueryInterface(Components.interfaces.nsIPrefBranchInternal);
  prefBranch.removeObserver(kPrefMailAddrBookLastNameFirst, gMailAddrBookLastNameFirstObserver);
}

function OnLoadAddressBook()
{
  gAddressBookBundle = document.getElementById("bundle_addressBook");
  gSearchInput = document.getElementById("searchInput");

  verifyAccounts(null); 	// this will do migration, if we need to.

  top.addressbook = Components.classes["@mozilla.org/addressbook;1"].createInstance(Components.interfaces.nsIAddressBook);

  InitCommonJS();

  UpgradeAddressBookResultsPaneUI("mailnews.ui.addressbook_results.version");

  GetCurrentPrefs();

  AddPrefObservers();

  // FIX ME - later we will be able to use onload from the overlay
  OnLoadCardView();
	
  SetupAbCommandUpdateHandlers();

  //workaround - add setTimeout to make sure dynamic overlays get loaded first
  setTimeout('SelectFirstAddressBook()',0);
  
  // add a listener, so we can switch directories if
  // the current directory is deleted
  var addrbookSession = Components.classes["@mozilla.org/addressbook/services/session;1"].getService().QueryInterface(Components.interfaces.nsIAddrBookSession);
  // this listener only cares when a directory is removed
  addrbookSession.addAddressBookListener(gAddressBookAbListener, Components.interfaces.nsIAbListener.directoryRemoved);
}

function GetCurrentPrefs()
{
	// prefs
	if ( cvPrefs == 0 )
		cvPrefs = new Object;

	cvPrefs.prefs = gPrefs;
	
	// check "Show Name As" menu item based on pref
	var menuitemID;
	switch (gPrefs.getIntPref("mail.addr_book.lastnamefirst"))
	{
		case kFirstNameFirst:
			menuitemID = 'firstLastCmd';
			break;
		case kLastNameFirst:
			menuitemID = 'lastFirstCmd';
			break;
		case kDisplayName:
		default:
			menuitemID = 'displayNameCmd';
			break;
	}

	var menuitem = top.document.getElementById(menuitemID);
	if ( menuitem )
		menuitem.setAttribute('checked', 'true');
}


function SetNameColumn(cmd)
{
	var prefValue;
	
	switch ( cmd )
	{
		case 'firstLastCmd':
			prefValue = kFirstNameFirst;
			break;
		case 'lastFirstCmd':
			prefValue = kLastNameFirst;
			break;
		case 'displayNameCmd':
			prefValue = kDisplayName;
			break;
	}
	
	cvPrefs.prefs.setIntPref("mail.addr_book.lastnamefirst", prefValue);
}

function CommandUpdate_AddressBook()
{
	goUpdateCommand('button_delete');
}

function ResultsPaneSelectionChanged()
{
  UpdateCardView();
}

function UpdateCardView()
{
  var cards = GetSelectedAbCards();

  // display the selected card, if exactly one card is selected.
  // either no cards, or more than one card is selected, clear the pane.
  if (cards.length == 1)
    OnClickedCard(cards[0])
  else 
    ClearCardViewPane();
}

function OnClickedCard(card)
{ 
  if (card) 
    DisplayCardViewPane(card);
  else
    ClearCardViewPane();
}

function AbClose()
{
  top.close();
}

function AbNewAddressBook()
{
  var dialog = window.openDialog(
    "chrome://messenger/content/addressbook/abAddressBookNameDialog.xul", 
     "", "chrome,titlebar", {okCallback:AbCreateNewAddressBook});
}

function AbCreateNewAddressBook(name)
{
  var properties = Components.classes["@mozilla.org/addressbook/properties;1"].createInstance(Components.interfaces.nsIAbDirectoryProperties);
  properties.description = name;
  top.addressbook.newAddressBook(properties);
}

function GetPrintSettings()
{
  var prevPS = gPrintSettings;

  try {
    if (gPrintSettings == null) {
      var useGlobalPrintSettings = true;
      var pref = Components.classes["@mozilla.org/preferences-service;1"]
                           .getService(Components.interfaces.nsIPrefBranch);
      if (pref) {
        useGlobalPrintSettings = pref.getBoolPref("print.use_global_printsettings", false);
      }

      // I would rather be using nsIWebBrowserPrint API
      // but I really don't have a document at this point
      var printOptionsService = Components.classes["@mozilla.org/gfx/printoptions;1"]
                                           .getService(Components.interfaces.nsIPrintOptions);
      if (useGlobalPrintSettings) {
        gPrintSettings = printOptionsService.globalPrintSettings;
      } else {
        gPrintSettings = printOptionsService.CreatePrintSettings();
      }
    }
  } catch (e) {
    dump("GetPrintSettings "+e);
  }

  return gPrintSettings;
}

function AbPrintCard()
{
  var selectedItems = GetSelectedAbCards();
  var numSelected = selectedItems.length;

  if (!numSelected)
    return;

  var addressbook = Components.classes["@mozilla.org/addressbook;1"].createInstance(Components.interfaces.nsIAddressBook);
  var uri = GetAbViewURI();
  if (!uri)
    return;

	var statusFeedback;
	statusFeedback = Components.classes["@mozilla.org/messenger/statusfeedback;1"].createInstance();
	statusFeedback = statusFeedback.QueryInterface(Components.interfaces.nsIMsgStatusFeedback);

	var selectionArray = new Array(numSelected);

	var totalCard = 0;

	for(var i = 0; i < numSelected; i++)
	{
		var card = selectedItems[i];
    var printCardUrl = CreatePrintCardUrl(card);
		if (printCardUrl)
		{
			selectionArray[totalCard++] = printCardUrl;
		}
	}

  if (!gPrintSettings) {
    gPrintSettings = GetPrintSettings();
  }

	printEngineWindow = window.openDialog("chrome://messenger/content/msgPrintEngine.xul",
										"",
										"chrome,dialog=no,all",
										totalCard, selectionArray, statusFeedback, gPrintSettings);

	return;
}

function CreatePrintCardUrl(card)
{
  var url = "data:text/xml;base64," + card.convertToBase64EncodedXML();
  return url;
}

function AbPrintAddressBook()
{
  var addressbook = Components.classes["@mozilla.org/addressbook;1"].createInstance(Components.interfaces.nsIAddressBook);
  var uri = GetAbViewURI();
  if (!uri)
    return;

  var statusFeedback;
	statusFeedback = Components.classes["@mozilla.org/messenger/statusfeedback;1"].createInstance();
	statusFeedback = statusFeedback.QueryInterface(Components.interfaces.nsIMsgStatusFeedback);

  /*
    turn "moz-abmdbdirectory://abook.mab" into
    "addbook://moz-abmdbdirectory/abook.mab?action=print"
   */

  var abURIArr = uri.split("://");
  var printUrl = "addbook://" + abURIArr[0] + "/" + abURIArr[1] + "?action=print"

  if (!gPrintSettings) {
    gPrintSettings = GetPrintSettings();
  }

	printEngineWindow = window.openDialog("chrome://messenger/content/msgPrintEngine.xul",
										"",
										"chrome,dialog=no,all",
										1, [printUrl], statusFeedback, gPrintSettings);

	return;
}

function AbExport()
{
  try {
    var selectedItems = dirTree.selectedItems;
    if (selectedItems.length != 1)
      return;

    var selectedABURI = selectedItems[0].getAttribute('id');
    var directory = GetDirectoryFromURI(selectedABURI);
    addressbook.exportAddressBook(directory);
  }
  catch (ex) {
    var promptService = Components.classes["@mozilla.org/embedcomp/prompt-service;1"].getService(Components.interfaces.nsIPromptService);

    if (promptService) {
      var message;
      switch (ex.result) {
        case Components.results.NS_ERROR_FILE_ACCESS_DENIED:
          message = gAddressBookBundle.getString("failedToExportMessageFileAccessDenied");
          break;
        case Components.results.NS_ERROR_FILE_NO_DEVICE_SPACE:
          message = gAddressBookBundle.getString("failedToExportMessageNoDeviceSpace");
          break;
        default:
          message = ex.message;
          break;
      }

      promptService.alert(window,
        gAddressBookBundle.getString("failedToExportTitle"), 
        message);
    }
  }
}

function AbDeleteDirectory()
{
    var selArray = dirTree.selectedItems;
    var count = selArray.length;
    if (count != 1)
        return;

    var isPersonalOrCollectedAbsSelectedForDeletion = false;
    var parentArray = Components.classes["@mozilla.org/supports-array;1"].createInstance(Components.interfaces.nsISupportsArray);
    if (!parentArray) 
      return; 

    var selectedABURI = selArray[0].getAttribute("id");

    // check to see if personal or collected address books is selected for deletion.
    // if yes, prompt the user an appropriate message saying these cannot be deleted
    if ((selectedABURI != kCollectedAddressbookURI) &&
        (selectedABURI != kPersonalAddressbookURI)) {
      var parent = selArray[0].parentNode.parentNode;
      if (parent) {
        var parentId;
        if (parent == dirTree)
          parentId = "moz-abdirectory://";
        else	
          parentId = parent.getAttribute("id");

        var parentDir = GetDirectoryFromURI(parentId);
        parentArray.AppendElement(parentDir);
      }
    }
    else {
      var promptService = Components.classes["@mozilla.org/embedcomp/prompt-service;1"].getService(Components.interfaces.nsIPromptService);
      if (promptService) {
        promptService.alert(window,
          gAddressBookBundle.getString("cannotDeleteTitle"), 
          gAddressBookBundle.getString("cannotDeleteMessage"));
      }
      return;
    }

    var confirmDeleteMessage;
    
    var directory = GetDirectoryFromURI(selectedABURI);
    if (directory.isMailList) 
      confirmDeleteMessage = gAddressBookBundle.getString("confirmDeleteMailingList");
    else
      confirmDeleteMessage = gAddressBookBundle.getString("confirmDeleteAddressbook");

    if (!window.confirm(confirmDeleteMessage))
       return;

    var resourceArray = Components.classes["@mozilla.org/supports-array;1"].createInstance(Components.interfaces.nsISupportsArray);
    var selectedABResource = GetDirectoryFromURI(selectedABURI).QueryInterface(Components.interfaces.nsIRDFResource);

    resourceArray.AppendElement(selectedABResource);

    top.addressbook.deleteAddressBooks(dirTree.database, parentArray, resourceArray);
    SelectFirstAddressBook();
}

function SetStatusText(total)
{
  if (!gStatusText)
    gStatusText = document.getElementById('statusText');

  try {
    var statusText;

    if (gSearchInput.value) 
      statusText = gAddressBookBundle.getFormattedString("matchesFound", [total]);   
    else
      statusText = gAddressBookBundle.getFormattedString("totalCardStatus", [gAbView.directory.dirName, total]);   

    gStatusText.setAttribute("label", statusText);
  }
  catch(ex) {
    dump("failed to set status text:  " + ex + "\n");
  }
}

function AbResultsPaneDoubleClick(card)
{
  AbEditCard(card);
}

function onAdvancedAbSearch()
{
  var selectedItems = dirTree.selectedItems;
  if (selectedItems.length != 1)
    return;

  var selectedABURI = selectedItems[0].getAttribute('id');

  window.openDialog("chrome://messenger/content/ABSearchDialog.xul", "", 
                    "chrome,resizable,status,centerscreen,dialog=no", {directory: selectedABURI} );
}

function onEnterInSearchBar()
{
  ClearCardViewPane();

  var selectedItems = dirTree.selectedItems;
  if (selectedItems.length != 1)
    return;

  if (!gQueryURIFormat) {
    gQueryURIFormat = gPrefs.getCharPref("mail.addr_book.quicksearchquery.format");
  }

  var selectedNode = selectedItems[0];
  var sortColumn = selectedNode.getAttribute("sortColumn");
  var sortDirection = selectedNode.getAttribute("sortDirection");
  var searchURI = selectedNode.getAttribute("id");

  /*
   XXX todo, handle the case where the LDAP url
   already has a query, like 
   moz-abldapdirectory://nsdirectory.netscape.com:389/ou=People,dc=netscape,dc=com?(or(Department,=,Applications))
  */
  if (gSearchInput.value != "") {
    // replace all instances of @V with the escaped version
    // of what the user typed in the quick search text input
    searchURI += gQueryURIFormat.replace(/@V/g, escape(gSearchInput.value));
  }

  SetAbView(searchURI, sortColumn, sortDirection);
  
  // XXX todo 
  // this works for synchronous searches of local addressbooks, 
  // but not for LDAP searches
  SelectFirstCard();
}
