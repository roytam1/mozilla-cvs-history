/* -*- Mode: Java; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: NPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
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
 * Alec Flett <alecf@netscape.com>
 * H�kan Waara <hwaara@chello.se>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or 
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the NPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

// the actual filter that we're editing
var gFilter;
// cache the key elements we need
var gFilterList;
var gFilterNameElement;
var gActionElement;
var gActionTargetElement;
var gActionValueDeck;
var gActionPriority;
var gActionLabel;
var gFilterBundle;
var gPreFillName;
var nsMsgSearchScope = Components.interfaces.nsMsgSearchScope;
var gPref;
var gPrefBranch;
var gMailSession = null;

var nsMsgFilterAction = Components.interfaces.nsMsgFilterAction;

var gFilterEditorMsgWindow=null;
     
function filterEditorOnLoad()
{
    initializeSearchWidgets();
    initializeFilterWidgets();

    gPref = Components.classes["@mozilla.org/preferences;1"].getService(Components.interfaces.nsIPref);
    gPrefBranch = gPref.getDefaultBranch(null);
    gFilterBundle = document.getElementById("bundle_filter");
    if ("arguments" in window && window.arguments[0]) {
        var args = window.arguments[0];

        if ("filter" in args) {
          // editing a filter
          gFilter = window.arguments[0].filter;
          initializeDialog(gFilter);
        } 
        else {
          gFilterList = args.filterList;
          if (gFilterList)
              setSearchScope(getScopeFromFilterList(gFilterList));

          // if doing prefill filter create a new filter and populate it.
          if ("filterName" in args) {
            gPreFillName = args.filterName;
            gFilter = gFilterList.createFilter(gPreFillName);
            var term = gFilter.createTerm();

            term.attrib = Components.interfaces.nsMsgSearchAttrib.Sender;
            term.op = Components.interfaces.nsMsgSearchOp.Is;

            var termValue = term.value;
            termValue.attrib = term.attrib;
            termValue.str = gPreFillName;

            term.value = termValue;

            gFilter.appendTerm(term);
            gFilter.action = nsMsgFilterAction.MoveToFolder;

            initializeDialog(gFilter);
          }
          else{
            // fake the first more button press
            onMore(null);
          }

        }
    }

    if (!gFilter)
    {
      var stub = gFilterBundle.getString("untitledFilterName");
      var count = 1;
      var name = stub;

      // Set the default filter name to be "untitled filter"
      while (duplicateFilterNameExists(name)) 
      {
        count++;
        name = stub + " " + count.toString();
      }
      gFilterNameElement.value = name;

      // initialize priority
      var defaultValue = String(Components.interfaces.nsMsgPriority.Default);
      gActionPriority.selectedItem = gActionPriority.getElementsByAttribute("value", defaultValue)[0]
    }

    gFilterNameElement.select();
    moveToAlertPosition();
}

function filterEditorOnUnload()
{
  if (gMailSession)
    gMailSession.RemoveFolderListener(gFolderListener);
}

function onEnterInSearchTerm()
{
  // do nothing.  onOk() will get called since this is a dialog
}

function onAccept()
{
    if (duplicateFilterNameExists(gFilterNameElement.value))
    {
        var promptService = Components.classes["@mozilla.org/embedcomp/prompt-service;1"].getService();
        promptService = promptService.QueryInterface(Components.interfaces.nsIPromptService);

        if (promptService)
        {
            promptService.alert(window,
                gFilterBundle.getString("cannotHaveDuplicateFilterTitle"),
                gFilterBundle.getString("cannotHaveDuplicateFilterMessage")
            );
        }

        return false;
    }


    if (!saveFilter()) return false;

    // parent should refresh filter list..
    // this should REALLY only happen when some criteria changes that
    // are displayed in the filter dialog, like the filter name
    window.arguments[0].refresh = true;
    return true;
}

// the folderListener object
var gFolderListener = {
    OnItemAdded: function(parentItem, item, view) {},

    OnItemRemoved: function(parentItem, item, view){},

    OnItemPropertyChanged: function(item, property, oldValue, newValue) {},

    OnItemIntPropertyChanged: function(item, property, oldValue, newValue) {},

    OnItemBoolPropertyChanged: function(item, property, oldValue, newValue) {},

    OnItemUnicharPropertyChanged: function(item, property, oldValue, newValue){},
    OnItemPropertyFlagChanged: function(item, property, oldFlag, newFlag) {},

    OnItemEvent: function(folder, event) {
        var eventType = event.GetUnicode();

        if (eventType == "FolderCreateCompleted") {
            SetFolderPicker(folder.URI, gActionTargetElement.id);
            SetBusyCursor(window, false);
        }     
        else if (eventType == "FolderCreateFailed") {
            SetBusyCursor(window, false);
        }

    }
}

function duplicateFilterNameExists(filterName)
{
    var args = window.arguments[0];
    var filterList;
    if ("filterList" in args)
      filterList = args.filterList;
    if (filterList)
      for (var i = 0; i < filterList.filterCount; i++)
     {
       if (filterName == filterList.getFilterAt(i).filterName)
         return true;
     }
    return false;   
}

function getScopeFromFilterList(filterList)
{
    if (!filterList) {
      dump("yikes, null filterList\n");
      return nsMsgSearchScope.offlineMail;
}

    return filterList.folder.server.filterScope;
}

function getScope(filter) 
{
    return getScopeFromFilterList(filter.filterList);
}

function setLabelAttributes(labelID, menuItemID)
{
    var color;
    var prefString;

    try
    {
        color = gPrefBranch.getCharPref("mailnews.labels.color." + labelID);
        prefString = gPref.getComplexValue("mailnews.labels.description." + labelID,
                                           Components.interfaces.nsIPrefLocalizedString);
    }
    catch(ex)
    {
        dump("bad! " + ex + "\n");
    }

    document.getElementById(menuItemID).setAttribute("label", prefString);

    // the following is commented out for now until UE decides on how to show the
    // Labels menu items in the drop down list menu.
    //document.getElementById(menuItemID).setAttribute("style", ("color: " + color));
}

function initializeFilterWidgets()
{
    gFilterNameElement = document.getElementById("filterName");
    gActionElement = document.getElementById("actionMenu");
    gActionTargetElement = document.getElementById("actionTargetFolder");
    gActionValueDeck = document.getElementById("actionValueDeck");
    gActionPriority = document.getElementById("actionValuePriority");
    gActionLabel = document.getElementById("actionValueLabel");
}

function initializeDialog(filter)
{
    var selectedPriority;
    gFilterNameElement.value = filter.filterName;

    gActionElement.selectedItem=gActionElement.getElementsByAttribute("value", filter.action)[0];
    showActionElementFor(gActionElement.selectedItem);

    if (filter.action == nsMsgFilterAction.MoveToFolder) {
        // preselect target folder
        var target = filter.actionTargetFolderUri;

        if (target) {
            SetFolderPicker(target, gActionTargetElement.id);
        }
    } else if (filter.action == nsMsgFilterAction.ChangePriority) {
        // initialize priority
        selectedPriority = gActionPriority.getElementsByAttribute("value", filter.actionPriority);

        if (selectedPriority && selectedPriority.length > 0) {
            selectedPriority = selectedPriority[0];
            gActionPriority.selectedItem = selectedPriority;
        }
    }

    else if (filter.action == nsMsgFilterAction.Label) {
      var selectedLabel;
      // initialize label
      selectedLabel = gActionLabel.getElementsByAttribute("value", filter.actionLabel);
        if (selectedLabel && selectedLabel.length > 0) {
            selectedLabel = selectedLabel[0];
            gActionLabel.selectedItem = selectedLabel;
        }
        gFilter.actionLabel = gActionLabel.selectedItem.getAttribute("value");
        // Set the label text on dialog initialization.
        setLabelAttributes(gFilter.actionLabel, "actionValueLabel");
    }

    var scope = getScope(filter);
    setSearchScope(scope);
    initializeSearchRows(scope, filter.searchTerms);
    if (filter.searchTerms.Count() > 1)
        gSearchLessButton.removeAttribute("disabled", "false");

}

function InitMessageLabel()
{
    /* this code gets the label strings and changes the menu labels */
    var lastLabel = 5;

    // start with 1 because there is no None label (id 0) as an filtering
    // option to filter to.
    for (var i = 1; i <= lastLabel; i++)
    {
        setLabelAttributes(i, "labelMenuItem" + i);
    }

    document.commandDispatcher.updateCommands('create-menu-label');
}


// move to overlay

function saveFilter() {
    var isNewFilter;
    var str;

    var filterName= gFilterNameElement.value;
    if (!filterName || filterName == "") {
        str = gFilterBundle.getString("mustEnterName");
        window.alert(str);
        gFilterNameElement.focus();
        return false;
    }

    var targetUri;
    var action = gActionElement.selectedItem.getAttribute("value");

    if (action == nsMsgFilterAction.MoveToFolder) {
        if (gActionTargetElement)
            targetUri = gActionTargetElement.getAttribute("uri");
        if (!targetUri || targetUri == "") {
            str = gFilterBundle.getString("mustSelectFolder");
            window.alert(str);
            return false;
        }
    }

    else if (action == nsMsgFilterAction.ChangePriority) {
        if (!gActionPriority.selectedItem) {
            str = gFilterBundle.getString("mustSelectPriority");
            window.alert(str);
            return false;
        }
    }

    else if (action == nsMsgFilterAction.Label) {
        if (!gActionLabel.selectedItem) {
            str = gFilterBundle.getString("mustSelectLabel");
            window.alert(str);
            return false;
        }
    }
    // this must happen after everything has
    if (!gFilter) {
        gFilter = gFilterList.createFilter(gFilterNameElement.value);
        isNewFilter = true;
        gFilter.enabled=true;
    } else {
        gFilter.filterName = gFilterNameElement.value;
        
        //Prefilter is treated as a new filter.
        if (gPreFillName) {
          isNewFilter = true;
          gFilter.enabled=true;
        }
        else {
          isNewFilter = false;
        }
    }

    gFilter.action = action;
    if (action == nsMsgFilterAction.MoveToFolder)
        gFilter.actionTargetFolderUri = targetUri;
    else if (action == nsMsgFilterAction.ChangePriority)
        gFilter.actionPriority = gActionPriority.selectedItem.getAttribute("value");
    else if (action == nsMsgFilterAction.Label)
        gFilter.actionLabel = gActionLabel.selectedItem.getAttribute("value");

    saveSearchTerms(gFilter.searchTerms, gFilter);

    if (isNewFilter) {
        // new filter - insert into gFilterList
        gFilterList.insertFilterAt(0, gFilter);
    }

    // success!
    return true;
}

function onTargetFolderSelected(event)
{
    SetFolderPicker(event.target.id, gActionTargetElement.id);
}


function onActionChanged(event)
{
    var menuitem = event.target;
    showActionElementFor(menuitem);
}

function showActionElementFor(menuitem)
{
    if (!menuitem) return;
    var indexValue = menuitem.getAttribute("actionvalueindex");

    gActionValueDeck.setAttribute("selectedIndex", indexValue);

    // If it's the Label menuItem selected, we need to fill the menulist
    // associated with this because the very first time it is selected,
    // it is not filled.
    // 2 indicates the Labels menu item.
    if (indexValue == "2")
    {
        var labelID = gActionLabel.selectedItem.getAttribute("value");

        setLabelAttributes(labelID, "actionValueLabel");
    }

    // Disable the "New Folder..." button if any other action than MoveToFolder is chosen
    document.getElementById("newFolderButton").disabled = (indexValue != "0");
}

function onLabelListChanged(event)
{
    var menuitem = event.target;
    showLabelColorFor(menuitem);
}

function showLabelColorFor(menuitem)
{
    if (!menuitem) return;
    var indexValue = menuitem.getAttribute("value");
    var labelID = gActionLabel.selectedItem.getAttribute("value");

    gActionLabel.setAttribute("selectedIndex", indexValue);
    setLabelAttributes(labelID, "actionValueLabel");
}

function GetFirstSelectedMsgFolder()
{
    var selectedFolder = gActionTargetElement.getAttribute("uri");

    var msgFolder = GetMsgFolderFromUri(selectedFolder, true);
    return msgFolder;
}

function SearchNewFolderOkCallback(name,uri)
{
  var msgFolder = GetMsgFolderFromUri(uri, true);
  var imapFolder = null;
  try
  {
    imapFolder = msgFolder.QueryInterface(Components.interfaces.nsIMsgImapMailFolder);
  }
  catch(ex) {}
  var mailSessionContractID = "@mozilla.org/messenger/services/session;1";
  if (imapFolder) //imapFolder creation is asynchronous.
  {
    if (!gMailSession)
      gMailSession = Components.classes[mailSessionContractID].getService(Components.interfaces.nsIMsgMailSession);
    try 
    {
      var nsIFolderListener = Components.interfaces.nsIFolderListener;
      var notifyFlags = nsIFolderListener.event;
      gMailSession.AddFolderListener(gFolderListener, notifyFlags);
    } 
    catch (ex) 
    {
      dump("Error adding to session: " +ex + "\n");
    }
  }

  var msgWindow = GetFilterEditorMsgWindow();

  if (imapFolder)
    SetBusyCursor(window, true);

  msgFolder.createSubfolder(name, msgWindow);
  
  if (!imapFolder)
  {
    var curFolder = uri+"/"+escape(name);
    SetFolderPicker(curFolder, gActionTargetElement.id);
  }
}

function UpdateAfterCustomHeaderChange()
{
  updateSearchAttributes();
}

//if you use msgWindow, please make sure that destructor gets called when you close the "window"
function GetFilterEditorMsgWindow()
{
  if (!gFilterEditorMsgWindow)
  {
    var msgWindowContractID = "@mozilla.org/messenger/msgwindow;1";
    var nsIMsgWindow = Components.interfaces.nsIMsgWindow;
    gFilterEditorMsgWindow = Components.classes[msgWindowContractID].createInstance(nsIMsgWindow);
    gFilterEditorMsgWindow.SetDOMWindow(window); 
  }
  return gFilterEditorMsgWindow;
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
}

function doHelpButton()
{
  openHelp("mail-filters");
}
