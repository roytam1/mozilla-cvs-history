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
# The Original Code is Mozilla Communicator client code, released
# March 31, 1998.
#
# The Initial Developer of the Original Code is
# Netscape Communications Corporation.
# Portions created by the Initial Developer are Copyright (C) 1998-1999
# the Initial Developer. All Rights Reserved.
#
# Contributor(s):
#   Jan Varga (varga@nixcorp.com)
#   Håkan Waara (hwaara@chello.se)
#   Neil Rashbrook (neil@parkwaycc.co.uk)
#   Seth Spitzer <sspitzer@netscape.com>
#   David Bienvenu <bienvenu@nventure.com>
#   Jeremy Morton <bugzilla@game-point.net>
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

/* This is where functions related to the 3 pane window are kept */

// from MailNewsTypes.h
const nsMsgKey_None = 0xFFFFFFFF;
const nsMsgViewIndex_None = 0xFFFFFFFF;
const kMailCheckOncePrefName = "mail.startup.enabledMailCheckOnce";

const kStandardPaneConfig = 0;
const kWidePaneConfig = 1;
const kVerticalPaneConfig = 2;

const kNumFolderViews = 4; // total number of folder views

// from nsMsgFolderFlags.h
const MSG_FOLDER_FLAG_ELIDED = 0x10;

var gFolderTree;
var gMessagePane;
var gThreadTree;
var gSearchInput;

var gThreadAndMessagePaneSplitter = null;
var gUnreadCount = null;
var gTotalCount = null;
var gCurrentFolderView;
var gCurrentLoadingFolderURI;
var gCurrentFolderToReroot;
var gCurrentLoadingFolderSortType = 0;
var gCurrentLoadingFolderSortOrder = 0;
var gCurrentLoadingFolderViewType = 0;
var gCurrentLoadingFolderViewFlags = 0;
var gRerootOnFolderLoad = false;
var gNextMessageAfterDelete = null;
var gNextMessageAfterLoad = null;
var gNextMessageViewIndexAfterDelete = -2;
var gSelectedIndexWhenDeleting = -1;
var gCurrentlyDisplayedMessage=nsMsgViewIndex_None;
var gStartFolderUri = null;
var gStartMsgKey = nsMsgKey_None;
var gSearchEmailAddress = null;
var gRightMouseButtonDown = false;
// Global var to keep track of which row in the thread pane has been selected
// This is used to make sure that the row with the currentIndex has the selection
// after a Delete or Move of a message that has a row index less than currentIndex.
var gThreadPaneCurrentSelectedIndex = -1;
var gLoadStartFolder = true;
var gNewAccountToLoad = null;

// Global var to keep track of if the 'Delete Message' or 'Move To' thread pane
// context menu item was triggered.  This helps prevent the tree view from
// not updating on one of those menu item commands.
var gThreadPaneDeleteOrMoveOccurred = false;

//If we've loaded a message, set to true.  Helps us keep the start page around.
var gHaveLoadedMessage;

var gDisplayStartupPage = false;

var gNotifyDefaultInboxLoadedOnStartup = false;

function SelectAndScrollToKey(aMsgKey)
{
  // select the desired message
  // if the key isn't found, we won't select anything
  gDBView.selectMsgByKey(aMsgKey);

  // is there a selection?
  // if not, bail out.
  var indicies = GetSelectedIndices(gDBView);
  if (!indicies || !indicies.length)
    return false;

  // now scroll to it
  EnsureRowInThreadTreeIsVisible(indicies[0]);
  return true;
}

// A helper routine called after a folder is loaded to make sure
// we select and scroll to the correct message (could be the first new message,
// could be the last displayed message, etc.)
function ScrollToMessageAfterFolderLoad(folder)
{
  var scrolled = gPrefBranch.getBoolPref("mailnews.scroll_to_new_message") &&
      ScrollToMessage(nsMsgNavigationType.firstNew, true, false /* selectMessage */);
  if (!scrolled && folder && gPrefBranch.getBoolPref("mailnews.remember_selected_message"))
  {
    // If we failed to scroll to a new message,
    // reselect the last selected message
    var lastMessageLoaded = folder.lastMessageLoaded;
    if (lastMessageLoaded != nsMsgKey_None)
      scrolled = SelectAndScrollToKey(lastMessageLoaded);
  }

  if (!scrolled)
  {
    // if we still haven't scrolled,
    // scroll to the newest, which might be the top or the bottom
    // depending on our sort order and sort type
    if (gDBView.sortOrder == nsMsgViewSortOrder.ascending)
    {
      switch (gDBView.sortType)
      {
        case nsMsgViewSortType.byDate:
        case nsMsgViewSortType.byReceived:
        case nsMsgViewSortType.byId:
        case nsMsgViewSortType.byThread:
         scrolled = ScrollToMessage(nsMsgNavigationType.lastMessage, true, false /* selectMessage */);
         break;
      }
    }

    // if still we haven't scrolled,
    // scroll to the top.
    if (!scrolled)
      EnsureRowInThreadTreeIsVisible(0);
  }
}

// the folderListener object
var folderListener = {
    OnItemAdded: function(parentItem, item) { },

    OnItemRemoved: function(parentItem, item) { },

    OnItemPropertyChanged: function(item, property, oldValue, newValue) { },

    OnItemIntPropertyChanged: function(item, property, oldValue, newValue) {
      if (item == gMsgFolderSelected) {
        if(property.toString() == "TotalMessages" || property.toString() == "TotalUnreadMessages") {
          UpdateStatusMessageCounts(gMsgFolderSelected);
          UpdateFolderLocationPicker(item);
        }
      }
    },

    OnItemBoolPropertyChanged: function(item, property, oldValue, newValue) { },

    OnItemUnicharPropertyChanged: function(item, property, oldValue, newValue) { },
    OnItemPropertyFlagChanged: function(item, property, oldFlag, newFlag) { },

    OnItemEvent: function(folder, event) {
      var eventType = event.toString();
      if (eventType == "FolderLoaded") {
        if (folder) {
          var scrolled = false;
          var msgFolder = folder.QueryInterface(Components.interfaces.nsIMsgFolder);
          var uri = folder.URI;
          var rerootingFolder = (uri == gCurrentFolderToReroot);
          if (rerootingFolder) {
            viewDebug("uri = gCurrentFolderToReroot, setting gQSViewIsDirty\n");
            gQSViewIsDirty = true;
            gCurrentFolderToReroot = null;
            if (msgFolder) {
              msgFolder.endFolderLoading();
              UpdateStatusQuota(msgFolder);
              // Suppress command updating when rerooting the folder.
              // When rerooting, we'll be clearing the selection
              // which will cause us to update commands.
              if (gDBView) {
                gDBView.suppressCommandUpdating = true;
                // If the db's view isn't set, something went wrong and we
                // should reroot the folder, which will re-open the view.
                if (!gDBView.db)
                  gRerootOnFolderLoad = true;
              }
              if (gRerootOnFolderLoad)
                RerootFolder(uri, msgFolder, gCurrentLoadingFolderViewType, gCurrentLoadingFolderViewFlags, gCurrentLoadingFolderSortType, gCurrentLoadingFolderSortOrder);

              var db = msgFolder.getMsgDatabase(msgWindow);
              if (db)
                db.resetHdrCacheSize(100);

              if (gDBView) {
                gDBView.suppressCommandUpdating = false;
              }

              gCurrentLoadingFolderSortType = 0;
              gCurrentLoadingFolderSortOrder = 0;
              gCurrentLoadingFolderViewType = 0;
              gCurrentLoadingFolderViewFlags = 0;

              // Used for rename folder msg loading after folder is loaded.
              scrolled = LoadCurrentlyDisplayedMessage();

              if (gStartMsgKey != nsMsgKey_None) {
                scrolled = SelectAndScrollToKey(gStartMsgKey);
                gStartMsgKey = nsMsgKey_None;
              }

              if (gNextMessageAfterLoad) {
                var type = gNextMessageAfterLoad;
                gNextMessageAfterLoad = null;

                // Scroll to and select the proper message.
                scrolled = ScrollToMessage(type, true, true /* selectMessage */);
              }
            }
          }
          if (uri == gCurrentLoadingFolderURI) {
            viewDebug("uri == current loading folder uri\n");
            gCurrentLoadingFolderURI = "";
            // Scroll to message for virtual folders is done in
            // gSearchNotificationListener.OnSearchDone (see searchBar.js).
            if (!scrolled && !(gMsgFolderSelected.flags & MSG_FOLDER_FLAG_VIRTUAL))
              ScrollToMessageAfterFolderLoad(msgFolder);
            SetBusyCursor(window, false);
          }
          if (gNotifyDefaultInboxLoadedOnStartup && (folder.flags & 0x1000))
          {
            var defaultAccount = accountManager.defaultAccount;
            defaultServer = defaultAccount.incomingServer;
            var inboxFolder = GetInboxFolder(defaultServer);
            if (inboxFolder && inboxFolder.URI == folder.URI)
            {
              NotifyObservers(null,"defaultInboxLoadedOnStartup",null);
              gNotifyDefaultInboxLoadedOnStartup = false;
            }
          }
          // Folder loading is over,
          // now issue quick search if there is an email address.
          viewDebug("in folder loaded gVirtualFolderTerms = " + gVirtualFolderTerms + "\n");
          viewDebug("in folder loaded gMsgFolderSelected = " + gMsgFolderSelected.URI + "\n");
          if (rerootingFolder)
          {
            if (gSearchEmailAddress)
            {
              Search(gSearchEmailAddress);
              gSearchEmailAddress = null;
            }
            else if (gVirtualFolderTerms)
            {
              gDefaultSearchViewTerms = null;
              viewDebug("searching gVirtualFolderTerms\n");
              gDBView.viewFolder = gMsgFolderSelected;
              ViewChangeByFolder(gMsgFolderSelected);
            }
            else if (gMsgFolderSelected.flags & MSG_FOLDER_FLAG_VIRTUAL)
            {
              viewDebug("selected folder is virtual\n");
              gDefaultSearchViewTerms = null;
            }
            else
            {
              // Get the view value from the folder.
              if (msgFolder)
              {
                // If our new view is the same as the old view and we already
                // have the list of search terms built up for the old view,
                // just re-use it.
                var result = GetMailViewForFolder(msgFolder);
                if (GetSearchInput() && gCurrentViewValue == result && gDefaultSearchViewTerms)
                {
                  viewDebug("searching gDefaultSearchViewTerms and rerootingFolder\n");
                  Search("");
                }
                else if (document.getElementById("mailviews-container")) // Only load the folder view if the views toolbar is visible.
                {
                  viewDebug("changing view by value\n");
                  ViewChangeByValue(result);
                }
              }
            }
          }
        }
      }
      else if (eventType == "ImapHdrDownloaded") {
        if (folder) {
          var imapFolder = folder.QueryInterface(Components.interfaces.nsIMsgImapMailFolder);
          if (imapFolder) {
            var hdrParser = imapFolder.hdrParser;
            if (hdrParser) {
              var msgHdr = hdrParser.GetNewMsgHdr();
              if (msgHdr)
              {
                var hdrs = hdrParser.headers;
                if (hdrs && hdrs.indexOf("X-attachment-size:") > 0) {
                  msgHdr.OrFlags(0x10000000);  // 0x10000000 is MSG_FLAG_ATTACHMENT
                }
                if (hdrs && hdrs.indexOf("X-image-size:") > 0) {
                  msgHdr.setStringProperty("imageSize", "1");
                }
              }
            }
          }
        }
      }
      else if (eventType == "DeleteOrMoveMsgCompleted") {
        HandleDeleteOrMoveMsgCompleted(folder);
      }
      else if (eventType == "DeleteOrMoveMsgFailed") {
        HandleDeleteOrMoveMsgFailed(folder);
      }
      else if (eventType == "AboutToCompact") {
        if (gDBView)
          gCurrentlyDisplayedMessage = gDBView.currentlyDisplayedMessage;
      }
      else if (eventType == "CompactCompleted") {
        HandleCompactCompleted(folder);
      }
      else if (eventType == "RenameCompleted") {
        SelectFolder(folder.URI);
      }
      else if (eventType == "JunkStatusChanged") {
        HandleJunkStatusChanged(folder);
      }
    }
}

var folderObserver = {
    canDrop: function(index, orientation)
    {
        return CanDropOnFolderTree(index, orientation);
    },

    onDrop: function(row, orientation)
    {
        DropOnFolderTree(row, orientation);
    },

    onToggleOpenState: function(index)
    {
      var folderTree = GetFolderTree();

      // Nothing to do when collapsing an item.
      if (folderTree.view.isContainerOpen(index))
        return;

      var folderResource = GetFolderResource(folderTree, index);

      if (folderTree.view.getLevel(index) == 0)
      {
        // (Imap/Nntp/Pop) Account item.

        folderResource.QueryInterface(Components.interfaces.nsIMsgFolder)
                      .server.performExpand(msgWindow);
      }
      else if (folderResource instanceof Components.interfaces.nsIMsgImapMailFolder)
      {
        // Imap message folder item.

        folderResource.performExpand(msgWindow);
      }
    },

    onCycleHeader: function(colID, elt)
    {
    },

    onCycleCell: function(row, colID)
    {
    },

    onSelectionChanged: function()
    {
    },

    onPerformAction: function(action)
    {
    },

    onPerformActionOnRow: function(action, row)
    {
    },

    onPerformActionOnCell: function(action, row, col)
    {
    }
}

function HandleDeleteOrMoveMsgFailed(folder)
{
  gDBView.onDeleteCompleted(false);
  if(IsCurrentLoadedFolder(folder)) {
    if(gNextMessageAfterDelete) {
      gNextMessageAfterDelete = null;
      gNextMessageViewIndexAfterDelete = -2;
    }
  }

  // fix me???
  // ThreadPaneSelectionChange(true);
}

// WARNING
// this is a fragile and complicated function.
// be careful when hacking on it.
// Don't forget about things like different imap
// delete models, multiple views (from multiple thread panes,
// search windows, stand alone message windows)
function HandleDeleteOrMoveMsgCompleted(folder)
{
  // you might not have a db view.  this can happen if
  // biff fires when the 3 pane is set to account central.
  if (!gDBView)
    return;

  gDBView.onDeleteCompleted(true);

  if (!IsCurrentLoadedFolder(folder)) {
    // default value after delete/move/copy is over
    gNextMessageViewIndexAfterDelete = -2;
    return;
  }

  var treeView = gDBView.QueryInterface(Components.interfaces.nsITreeView);
  var treeSelection = treeView.selection;

  if (gNextMessageViewIndexAfterDelete == -2) {
    // a move or delete can cause our selection can change underneath us.
    // this can happen when the user
    // deletes message from the stand alone msg window
    // or the search view, or another 3 pane
    if (treeSelection.count == 0) {
      // this can happen if you double clicked a message
      // in the thread pane, and deleted it from the stand alone msg window
      // see bug #172392
      treeSelection.clearSelection();
      setTitleFromFolder(folder, null);
      ClearMessagePane();
      UpdateMailToolbar("delete from another view, 0 rows now selected");
    }
    else if (treeSelection.count == 1) {
      // this can happen if you had two messages selected
      // in the thread pane, and you deleted one of them from another view
      // (like the view in the stand alone msg window)
      // since one item is selected, we should load it.
      var startIndex = {};
      var endIndex = {};
      treeSelection.getRangeAt(0, startIndex, endIndex);

      // select the selected item, so we'll load it
      treeSelection.select(startIndex.value);
      treeView.selectionChanged();

      EnsureRowInThreadTreeIsVisible(startIndex.value);

      UpdateMailToolbar("delete from another view, 1 row now selected");
    }
    else {
      // this can happen if you have more than 2 messages selected
      // in the thread pane, and you deleted one of them from another view
      // (like the view in the stand alone msg window)
      // since multiple messages are still selected, do nothing.
    }
  }
  else {
    if (gNextMessageViewIndexAfterDelete != nsMsgViewIndex_None)
    {
      var viewSize = treeView.rowCount;
      if (gNextMessageViewIndexAfterDelete >= viewSize)
      {
        if (viewSize > 0)
          gNextMessageViewIndexAfterDelete = viewSize - 1;
        else
        {
          gNextMessageViewIndexAfterDelete = nsMsgViewIndex_None;

          // there is nothing to select since viewSize is 0
          treeSelection.clearSelection();
          setTitleFromFolder(folder, null);
          ClearMessagePane();
          UpdateMailToolbar("delete from current view, 0 rows left");
        }
      }
    }

    // if we are about to set the selection with a new element then DON'T clear
    // the selection then add the next message to select. This just generates
    // an extra round of command updating notifications that we are trying to
    // optimize away.
    if (gNextMessageViewIndexAfterDelete != nsMsgViewIndex_None)
    {
      // When deleting a message we don't update the commands
      // when the selection goes to 0
      // (we have a hack in nsMsgDBView which prevents that update)
      // so there is no need to
      // update commands when we select the next message after the delete;
      // the commands already
      // have the right update state...
      gDBView.suppressCommandUpdating = true;

      // This check makes sure that the tree does not perform a
      // selection on a non selected row (row < 0), else assertions will
      // be thrown.
      if (gNextMessageViewIndexAfterDelete >= 0)
        treeSelection.select(gNextMessageViewIndexAfterDelete);

      // If gNextMessageViewIndexAfterDelete has the same value
      // as the last index we had selected, the tree won't generate a
      // selectionChanged notification for the tree view. So force a manual
      // selection changed call.
      // (don't worry it's cheap if we end up calling it twice).
      if (treeView)
        treeView.selectionChanged();

      EnsureRowInThreadTreeIsVisible(gNextMessageViewIndexAfterDelete);
      gDBView.suppressCommandUpdating = false;

      // hook for extra toolbar items
      // XXX TODO
      // I think there is a bug in the suppression code above.
      // What if I have two rows selected, and I hit delete,
      // and so we load the next row.
      // What if I have commands that only enable where
      // exactly one row is selected?
      UpdateMailToolbar("delete from current view, at least one row selected");
    }
  }

  // default value after delete/move/copy is over
  gNextMessageViewIndexAfterDelete = -2;
}

function HandleCompactCompleted(folder)
{
  if (folder)
  {
    var resource = folder.QueryInterface(Components.interfaces.nsIRDFResource);
    if (resource)
    {
      var uri = resource.Value;
      var msgFolder = msgWindow.openFolder;
      if (msgFolder && uri == msgFolder.URI)
      {
        var msgdb = msgFolder.getMsgDatabase(msgWindow);
        if (msgdb)
        {
          var dbFolderInfo = msgdb.dBFolderInfo;
          sortType = dbFolderInfo.sortType;
          sortOrder = dbFolderInfo.sortOrder;
          viewFlags = dbFolderInfo.viewFlags;
          viewType = dbFolderInfo.viewType;
          dbFolderInfo = null;
        }

        RerootFolder(uri, msgFolder, viewType, viewFlags, sortType, sortOrder);
        LoadCurrentlyDisplayedMessage();
      }
    }
  }
}

function LoadCurrentlyDisplayedMessage()
{
  var scrolled = (gCurrentlyDisplayedMessage != nsMsgViewIndex_None);
  if (scrolled)
  {
    var treeView = gDBView.QueryInterface(Components.interfaces.nsITreeView);
    var treeSelection = treeView.selection;
    treeSelection.select(gCurrentlyDisplayedMessage);
    if (treeView)
      treeView.selectionChanged();
    EnsureRowInThreadTreeIsVisible(gCurrentlyDisplayedMessage);
    SetFocusThreadPane();
    gCurrentlyDisplayedMessage = nsMsgViewIndex_None; //reset
  }
  return scrolled;
}

function IsCurrentLoadedFolder(folder)
{
  var msgfolder = folder.QueryInterface(Components.interfaces.nsIMsgFolder);
  if(msgfolder)
  {
    var folderResource = msgfolder.QueryInterface(Components.interfaces.nsIRDFResource);
    if(folderResource)
    {
      var folderURI = folderResource.Value;
      var currentLoadedFolder = GetThreadPaneFolder();
      if (currentLoadedFolder.flags & MSG_FOLDER_FLAG_VIRTUAL)
      {
        var msgDatabase = currentLoadedFolder.getMsgDatabase(msgWindow);
        var dbFolderInfo = msgDatabase.dBFolderInfo;
        var srchFolderUri = dbFolderInfo.getCharProperty("searchFolderUri");
        var re = new RegExp("^" + folderURI + "$|^" + folderURI + "\||\|" + folderURI + "$|\|" + folderURI +"\|");
        var retval = (currentLoadedFolder.URI.match(re));
        return retval;

      }
      var currentURI = currentLoadedFolder.URI;
      return(currentURI == folderURI);
    }
  }

  return false;
}

function ServerContainsFolder(server, folder)
{
  if (!folder || !server)
    return false;

  return server.equals(folder.server);
}

function SelectServer(server)
{
  SelectFolder(server.rootFolder.URI);
}

// we have this incoming server listener in case we need to
// alter the folder pane selection when a server is removed
// or changed (currently, when the real username or real hostname change)
var gThreePaneIncomingServerListener = {
    onServerLoaded: function(server) {},
    onServerUnloaded: function(server) {
      var selectedFolders = GetSelectedMsgFolders();
      for (var i = 0; i < selectedFolders.length; i++) {
        if (ServerContainsFolder(server, selectedFolders[i])) {
          SelectServer(accountManager.defaultAccount.incomingServer);
          // we've made a new selection, we're done
          return;
        }
      }

      // if nothing is selected at this point, better go select the default
      // this could happen if nothing was selected when the server was removed
      selectedFolders = GetSelectedMsgFolders();
      if (selectedFolders.length == 0) {
        SelectServer(accountManager.defaultAccount.incomingServer);
      }
    },
    onServerChanged: function(server) {
      // if the current selected folder is on the server that changed
      // and that server is an imap or news server,
      // we need to update the selection.
      // on those server types, we'll be reconnecting to the server
      // and our currently selected folder will need to be reloaded
      // or worse, be invalid.
      if (server.type != "imap" && server.type !="nntp")
        return;

      var selectedFolders = GetSelectedMsgFolders();
      for (var i = 0; i < selectedFolders.length; i++) {
        // if the selected item is a server, we don't have to update
        // the selection
        if (!(selectedFolders[i].isServer) && ServerContainsFolder(server, selectedFolders[i])) {
          SelectServer(server);
          // we've made a new selection, we're done
          return;
        }
      }
    }
}

// aMsgWindowInitialized: false if we are calling from the onload handler, otherwise true
function UpdateMailPaneConfig(aMsgWindowInitialized) {
  const dynamicIds = ["messagesBox", "mailContent", "threadPaneBox"];
  var desiredId = dynamicIds[gPrefBranch.getIntPref("mail.pane_config.dynamic")];
  var messagePane = GetMessagePane();
  if (messagePane.parentNode.id != desiredId) {
    ClearAttachmentList();
    var messagePaneSplitter = GetThreadAndMessagePaneSplitter();
    var desiredParent = document.getElementById(desiredId);
    // See Bug 381992. The ctor for the browser element will fire again when we
    // re-insert the messagePaneBox back into the document.
    // But the dtor doesn't fire when the element is removed from the document.
    // Manually call destroy here to avoid a nasty leak.
    getMessageBrowser().destroy();
    desiredParent.appendChild(messagePaneSplitter);
    desiredParent.appendChild(messagePane);
    messagePaneSplitter.orient = desiredParent.orient;
    if (aMsgWindowInitialized)
    {
      messenger.setWindow(null, null);
      messenger.setWindow(window, msgWindow);
      if (gDBView && GetNumSelectedMessages() == 1)
        gDBView.reloadMessage();
    }
  }
}

const MailPrefObserver = {
  observe: function(subject, topic, prefName) {
    // verify that we're changing the mail pane config pref
    if (topic == "nsPref:changed")
    {
      if (prefName == "mail.pane_config.dynamic")
        UpdateMailPaneConfig(true);
      else if (prefName == "mail.showFolderPaneColumns")
        UpdateFolderColumnVisibility();
    }
  }
};

/**
 * Called on startup to initialize various parts of the main window
 */
function OnLoadMessenger()
{
  // update the pane config before we exit onload otherwise the user may see a flicker if we poke the document
  // in delayedOnLoadMessenger...
  UpdateMailPaneConfig(false);
  document.loadBindingDocument('chrome://global/content/bindings/textbox.xml');

  // Set a sane starting width/height for all resolutions on new profiles. Do this before the window loads
  if (!document.documentElement.hasAttribute("width"))
  {
    var defaultWidth, defaultHeight;
    if (screen.availHeight <= 600)
    {
      document.documentElement.setAttribute("sizemode", "maximized");
      defaultWidth = 800;
      defaultHeight = 565;
    }
    else // for higher resolution displays, use larger values for height and width
    {
      defaultWidth = screen.availWidth <= 1024 ? screen.availWidth * .95 : screen.availWidth * .8;
      defaultHeight = screen.availHeight * .8;
    }

    document.documentElement.setAttribute("width", defaultWidth);
    document.documentElement.setAttribute("height", defaultHeight);
  }

  gPrefBranch.QueryInterface(Components.interfaces.nsIPrefBranch2);
  gPrefBranch.addObserver("mail.pane_config.dynamic", MailPrefObserver, false);
  gPrefBranch.addObserver("mail.showFolderPaneColumns", MailPrefObserver, false);

  MailOfflineMgr.init();
  CreateMailWindowGlobals();
  GetMessagePane().collapsed = true;
  verifyAccounts(null);

  InitMsgWindow();
  messenger.setWindow(window, msgWindow);

  InitializeDataSources();
  InitPanes();
  MigrateAttachmentDownloadStore();
  MigrateJunkMailSettings();
  MigrateFolderViews();

  accountManager.setSpecialFolders();
  accountManager.loadVirtualFolders();
  accountManager.addIncomingServerListener(gThreePaneIncomingServerListener);

  gPhishingDetector.init();

  AddToSession();

  //need to add to session before trying to load start folder otherwise listeners aren't
  //set up correctly.
  // argument[0] --> folder uri
  // argument[1] --> optional message key
  // argument[2] --> optional email address; // Will come from aim; needs to show msgs from buddy's email address.
  if ("arguments" in window)
  {
    // filter our any feed urls that came in as arguments to the new window...
    if (window.arguments.length && /^feed:/i.test(window.arguments[0] ))
    {
      var feedHandler = Components.classes["@mozilla.org/newsblog-feed-downloader;1"].getService(Components.interfaces.nsINewsBlogFeedDownloader);
      if (feedHandler)
        feedHandler.subscribeToFeed(window.arguments[0], null, msgWindow);
      gStartFolderUri = null;
    }
    else
      gStartFolderUri = (window.arguments.length > 0) ? window.arguments[0] : null;
    gStartMsgKey = (window.arguments.length > 1) ? window.arguments[1]: nsMsgKey_None;
    gSearchEmailAddress = (window.arguments.length > 2) ? window.arguments[2] : null;
  }

#ifdef HAVE_SHELL_SERVICE
  var nsIShellService = Components.interfaces.nsIShellService;
  var shellService;
  var defaultAccount;
  try {
    shellService = Components.classes["@mozilla.org/mail/shell-service;1"].getService(nsIShellService);
    defaultAccount = accountManager.defaultAccount;
  } catch (ex) {}

  function showDefaultClientDialog() {
    window.openDialog("chrome://messenger/content/defaultClientDialog.xul",
                      "DefaultClient", "modal,centerscreen,chrome,resizable=no");
  }

  // Show the default client dialog only if we have at least one account,
  // we should check for the default client, and we aren't already the default
  // for mail.
  // Needs to be shown outside the he normal load sequence so it doesn't appear
  // before any other displays, in the wrong place of the screen.
  if (shellService && defaultAccount && shellService.shouldCheckDefaultClient
      && !shellService.isDefaultClient(true, nsIShellService.MAIL))
    setTimeout(showDefaultClientDialog, 0);
#endif

  // FIX ME - later we will be able to use onload from the overlay
  OnLoadMsgHeaderPane();

  gHaveLoadedMessage = false;

  gNotifyDefaultInboxLoadedOnStartup = true;

  //Set focus to the Thread Pane the first time the window is opened.
  SetFocusThreadPane();

  // initialize the customizeDone method on the customizeable toolbar
  var toolbox = document.getElementById("mail-toolbox");
  toolbox.customizeDone = function(aEvent) { MailToolboxCustomizeDone(aEvent, "CustomizeMailToolbar"); };

  var toolbarset = document.getElementById('customToolbars');
  toolbox.toolbarset = toolbarset;

  loadStartFolder(gStartFolderUri);
}

function OnUnloadMessenger()
{
  OnLeavingFolder(gMsgFolderSelected);  // mark all read in current folder
  accountManager.removeIncomingServerListener(gThreePaneIncomingServerListener);
  gPrefBranch.QueryInterface(Components.interfaces.nsIPrefBranch2);
  gPrefBranch.removeObserver("mail.pane_config.dynamic", MailPrefObserver);
  gPrefBranch.removeObserver("mail.showFolderPaneColumns", MailPrefObserver);
  document.getElementById('tabmail').closeTabs();

  gPhishingDetector.shutdown();

  // FIX ME - later we will be able to use onload from the overlay
  OnUnloadMsgHeaderPane();

  UnloadPanes();

  OnMailWindowUnload();
}

function NotifyObservers(aSubject, aTopic, aData)
{
  var observerService = Components.classes["@mozilla.org/observer-service;1"].getService(Components.interfaces.nsIObserverService);
  observerService.notifyObservers(aSubject, aTopic, aData);
}

function loadStartFolder(initialUri)
{
    var folderTree = GetFolderTree();
    var defaultServer = null;
    var startFolderResource = null;
    var isLoginAtStartUpEnabled = false;

    //First get default account
    try
    {
        if(initialUri)
            startFolderResource = GetMsgFolderFromUri(initialUri).QueryInterface(Components.interfaces.nsIRDFResource);
        else
        {
            var defaultAccount = accountManager.defaultAccount;

            defaultServer = defaultAccount.incomingServer;
            var rootMsgFolder = defaultServer.rootMsgFolder;

            startFolderResource = rootMsgFolder.QueryInterface(Components.interfaces.nsIRDFResource);

            // Enable check new mail once by turning checkmail pref 'on' to bring
            // all users to one plane. This allows all users to go to Inbox. User can
            // always go to server settings panel and turn off "Check for new mail at startup"
            if (!gPrefBranch.getBoolPref(kMailCheckOncePrefName))
            {
                gPrefBranch.setBoolPref(kMailCheckOncePrefName, true);
                defaultServer.loginAtStartUp = true;
            }

            // Get the user pref to see if the login at startup is enabled for default account
            isLoginAtStartUpEnabled = defaultServer.loginAtStartUp;

            // Get Inbox only if login at startup is enabled.
            if (isLoginAtStartUpEnabled)
            {
                //now find Inbox
                var outNumFolders = new Object();
                var inboxFolder = rootMsgFolder.getFolderWithFlags(0x1000);
                if (!inboxFolder) return;

                startFolderResource = inboxFolder.QueryInterface(Components.interfaces.nsIRDFResource);
            }
        }

        var startFolder = startFolderResource.QueryInterface(Components.interfaces.nsIMsgFolder);

        // it is possible we were given an initial uri and we need to subscribe or try to add
        // the folder. i.e. the user just clicked on a news folder they aren't subscribed to from a browser
        // the news url comes in here.

        // Perform biff on the server to check for new mail, except for imap
        // or a pop3 account that is deferred or deferred to,
        // or the case where initialUri is non-null (non-startup)
        if (!initialUri && isLoginAtStartUpEnabled && gLoadStartFolder
            && !defaultServer.isDeferredTo &&
            defaultServer.rootFolder == defaultServer.rootMsgFolder)
          defaultServer.performBiff(msgWindow);

        SelectFolder(startFolder.URI);
    }
    catch(ex)
    {
      // this is the case where we're trying to auto-subscribe to a folder.
      if (initialUri && !startFolder.parent)
      {
        // hack to force display of thread pane.
        ShowingThreadPane();
        messenger.loadURL(window, initialUri);
        return;
      }

      dump(ex);
      dump('Exception in LoadStartFolder caused by no default account.  We know about this\n');
    }

    // if gLoadStartFolder is true, then we must have just created a POP3 account
    // and we aren't supposed to initially download mail. (Bug #270743)
    if (gLoadStartFolder)
      MsgGetMessagesForAllServers(defaultServer);

    // If appropriate, send unsent messages. This may end up prompting the user,
    // so we need to get it out of the flow of the normal load sequence.
    function checkUnsent() {
      if (MailOfflineMgr.isOnline() && MailOfflineMgr.shouldSendUnsentMessages())
        SendUnsentMessages();
    }
    setTimeout(checkUnsent, 0);
}

function AddToSession()
{
  try {
    var mailSession = Components.classes[mailSessionContractID]
                                .getService(Components.interfaces.nsIMsgMailSession);
    var nsIFolderListener = Components.interfaces.nsIFolderListener;
    var notifyFlags = nsIFolderListener.intPropertyChanged | nsIFolderListener.event;
    mailSession.AddFolderListener(folderListener, notifyFlags);
  } catch (ex) {
    dump("Error adding to session\n");
  }
}

function InitPanes()
{
  OnLoadFolderPane();
  OnLoadThreadPane();
  SetupCommandUpdateHandlers();
}

function UnloadPanes()
{
  OnUnloadFolderPane();
  UnloadCommandUpdateHandlers();
}

function InitializeDataSources()
{
  //Setup common mailwindow stuff.
  AddDataSources();
}

function OnFolderUnreadColAttrModified(event)
{
  if (event.attrName == "hidden")
  {
    var folderNameCell = document.getElementById("folderNameCell");
    var label = {"true": "?folderTreeName", "false": "?folderTreeSimpleName"};
    folderNameCell.setAttribute("label", label[event.newValue]);
  }
}

function UpdateFolderColumnVisibility()
{
  var folderNameCol = document.getElementById("folderNameCol");
  var showColumns = gPrefBranch.getBoolPref("mail.showFolderPaneColumns");
  var folderUnreadCol = document.getElementById("folderUnreadCol");
  var folderColumnLabel = document.getElementById("folderColumnLabel");
  if (!showColumns)
  {
    var folderTotalCol = document.getElementById("folderTotalCol");
    var folderSizeCol = document.getElementById("folderSizeCol");
    folderUnreadCol.setAttribute("hidden", "true");
    folderTotalCol.setAttribute("hidden", "true");
    folderSizeCol.setAttribute("hidden", "true");
    folderNameCol.removeAttribute("label");
  }
  else
  {
    folderNameCol.setAttribute("label", folderColumnLabel.value);
  }

  folderNameCol.setAttribute("hideheader", showColumns ? "false" : "true");
  var folderTree = document.getElementById("folderTree");
  folderTree.setAttribute("hidecolumnpicker", showColumns ? "false" : "true");
  var hidden = folderUnreadCol.getAttribute("hidden");
  if (hidden != "true")
  {
    var folderNameCell = document.getElementById("folderNameCell");
    folderNameCell.setAttribute("label", "?folderTreeSimpleName");
  }
}

// loadFolderViewForTree -- a helper routine split away from
// loadFolderView.
// returns a localized string corresponding to the name of the new view
function loadFolderViewForTree(aNewFolderView, aFolderTree)
{
  var folderPaneHeader = document.getElementById('folderpane-title');
  var database = aFolderTree.database;
  var nsIRDFDataSource = Components.interfaces.nsIRDFDataSource;

  // Each folder pane view has the following properties:
  // ref-> the ref attribute for the folderTree
  // label -> the UI label associated with the folder view
  // datasources -> array of the data sources associated with the view

  var folderViews = [
                      {ref:"msgaccounts:/",           label:"folderPaneHeader",           dataSources: [accountManagerDataSource.QueryInterface(nsIRDFDataSource),
                                                                                                        folderDataSource.QueryInterface(nsIRDFDataSource)] },
                      {ref:"mailnewsunreadfolders:/", label:"folderPaneHeader_unread",    dataSources: [unreadFolderDataSource.QueryInterface(nsIRDFDataSource)]},
                      {ref:"mailnewsfavefolders:/",   label:"folderPaneHeader_favorites", dataSources: [favoriteFoldersDataSource.QueryInterface(nsIRDFDataSource)]},
                      {ref:"mailnewsrecentfolders:/", label:"folderPaneHeader_recent",    dataSources: [recentFoldersDataSource.QueryInterface(nsIRDFDataSource)]},
                    ];

  // unload the current data sources
  if (gCurrentFolderView != undefined)
  {
    var dataSourcesToUnload = folderViews[gCurrentFolderView].dataSources;
    for (index in dataSourcesToUnload)
      database.RemoveDataSource(dataSourcesToUnload[index]);
  }

  // add the new data sources
  var dataSourcesToAdd = folderViews[aNewFolderView].dataSources;
  for (index in dataSourcesToAdd)
  {
    database.AddDataSource(dataSourcesToAdd[index]);
  }

  aFolderTree.setAttribute('ref', folderViews[aNewFolderView].ref);
  return gMessengerBundle.getString(folderViews[aNewFolderView].label);
}

function loadFolderView(aNewFolderView)
{
  if (gCurrentFolderView && (gCurrentFolderView == aNewFolderView))
    return;

  var folderTree = GetFolderTree();

  var folderPaneHeader = document.getElementById('folderpane-title');
  var folderTree = GetFolderTree();
  var database = GetFolderDatasource();

  // load the folder view into the folder pane
  folderPaneHeader.value = loadFolderViewForTree(aNewFolderView, GetFolderTree());

  // if the folder location picker is visible, load the folder view into the location
  // picker as well.
  var folderLocationPicker = document.getElementById('folder-location-container');
  if (folderLocationPicker)
    loadFolderViewForTree(aNewFolderView, document.getElementById('folderLocationPopup').tree);

  // now reflect the new value back into prefs
  gPrefBranch.setIntPref('mail.ui.folderpane.view', gCurrentFolderView = aNewFolderView);
}

// we can cycle the folder view forward or backwards
function CycleFolderView(aCycleForward)
{
  // pass the call onto loadFolderView...
  var offset = aCycleForward ? 1 : kNumFolderViews - 1;
  loadFolderView((gCurrentFolderView + offset) % kNumFolderViews);
}

function OnLoadFolderPane()
{
  // want to start up showing the folder pane, in case we shut down
  // with the 3-pane showing a single message.
  document.getElementById("folderpane_splitter").collapsed = false;
  document.getElementById("folderPaneBox").collapsed = false;
  UpdateFolderColumnVisibility();
  var folderUnreadCol = document.getElementById("folderUnreadCol");
  folderUnreadCol.addEventListener("DOMAttrModified", OnFolderUnreadColAttrModified, false);

  loadFolderView(gPrefBranch.getIntPref('mail.ui.folderpane.view'));

  var folderTree = GetFolderTree();
  var folderTreeBuilder = folderTree.builder.QueryInterface(Components.interfaces.nsIXULTreeBuilder);
  folderTreeBuilder.addObserver(folderObserver);
  folderTree.addEventListener("click",FolderPaneOnClick,true);
  folderTree.addEventListener("mousedown",TreeOnMouseDown,true);
}

function OnUnloadFolderPane()
{
  var folderTree = GetFolderTree();
  var folderTreeBuilder = folderTree.builder.QueryInterface(Components.interfaces.nsIXULTreeBuilder);
  folderTreeBuilder.removeObserver(folderObserver);
  var folderUnreadCol = document.getElementById("folderUnreadCol");
  folderUnreadCol.removeEventListener("DOMAttrModified", OnFolderUnreadColAttrModified, false);
  folderTree.removeEventListener("click",FolderPaneOnClick,true);
  folderTree.removeEventListener("mousedown",TreeOnMouseDown,true);
}

// builds prior to 12-08-2001 did not have the labels column
// in the thread pane.  so if a user ran an old build, and then
// upgraded, they get the new column, and this causes problems.
// We're trying to avoid a similar problem to bug #96979.
// to work around this, we hide the column once, using the
// "mailnews.ui.threadpane.version" pref.
function UpgradeThreadPaneUI()
{
  var threadPaneUIVersion;

  try {

    threadPaneUIVersion = gPrefBranch.getIntPref("mailnews.ui.threadpane.version");

    // Note: threadTree._reorderColumn will throw an ERROR if the columns specified are already in the same order!

    if (threadPaneUIVersion < 6)
    {
      var threadTree = document.getElementById("threadTree");
      var dateCol = document.getElementById("dateCol");
      var receivedCol = document.getElementById("receivedCol");
      var junkCol = document.getElementById("junkStatusCol");

      if (threadPaneUIVersion < 5)
      {
        if (threadPaneUIVersion < 4)
        {
          if (threadPaneUIVersion < 3)
          {

            // in thunderbird, we are inserting the junk column just before the
            // date column.
            threadTree._reorderColumn(junkCol, dateCol, true);
          }

          var senderCol = document.getElementById("senderCol");
          var recipientCol = document.getElementById("recipientCol");
          threadTree._reorderColumn(recipientCol, junkCol, true);
          threadTree._reorderColumn(senderCol, recipientCol, true);

        } // version 4 upgrades

        // version 5 adds a new column called attachments
        var attachmentCol = document.getElementById("attachmentCol");
        var subjectCol = document.getElementById("subjectCol");

        threadTree._reorderColumn(attachmentCol, subjectCol, true);

      } // version 5 upgrades

      if (dateCol)
        threadTree._reorderColumn(receivedCol, dateCol, true);
      else
        threadTree._reorderColumn(receivedCol, junkCol, false);

      gPrefBranch.setIntPref("mailnews.ui.threadpane.version", 6);

    } // version 6 upgrades
  }
  catch (ex) {
    dump("UpgradeThreadPane: ex = " + ex + "\n");
  }
}

function OnLoadThreadPane()
{
  UpgradeThreadPaneUI();
}

// folderLocationPickerOnLoad can be called multiple times
// and it can be called when the location picker isn't in the toolbar
function folderLocationPickerOnLoad()
{
  var folderLocationPicker = document.getElementById('folder-location-container');
  if (!folderLocationPicker)
    return;

  var locationTree = document.getElementById('folderLocationPopup').tree;
  locationTree.database.AddDataSource(accountManagerDataSource);
  locationTree.database.AddDataSource(folderDataSource);
  locationTree.setAttribute("ref", "msgaccounts:/");
}

function OnLocationTreeSelect(menulist)
{
  SelectFolder(menulist.getAttribute('uri'));
}

function UpdateFolderLocationPicker(resource)
{
  var folderLocationPicker = document.getElementById('folder-location-container');
  if (!folderLocationPicker)
    return;

  var tree = GetFolderTree();
  var folders = document.getElementById('locationFolders');
  var properties = ['BiffState', 'NewMessages', 'HasUnreadMessages',
                    'SpecialFolder', 'IsServer', 'IsSecure', 'ServerType', 'NoSelect'];
  var folder = resource.QueryInterface(Components.interfaces.nsIMsgFolder);
  var label = folder.prettyName;
  folders.setAttribute("label", label);

  var rdfService = Components.classes["@mozilla.org/rdf/rdf-service;1"]
                             .getService(Components.interfaces.nsIRDFService);
  function GetFolderAttribute(tree, source, attribute) {
    var property = rdfService.GetResource("http://home.netscape.com/NC-rdf#" + attribute);
    var target = tree.database.GetTarget(source, property, true);
    if (target)
      target = target.QueryInterface(Components.interfaces.nsIRDFLiteral).Value;
    return target;
  }
  for (var i in properties)
  {
    var property = properties[i];
    var value = GetFolderAttribute(tree, resource, property);
    folders.setAttribute(property, value);
  }
  folders.setAttribute('uri', resource.Value);
}

function GetFolderDatasource()
{
  var folderTree = GetFolderTree();
  return folderTree.database;
}

/* Functions for accessing particular parts of the window*/
function GetFolderTree()
{
  if (!gFolderTree)
    gFolderTree = document.getElementById("folderTree");
  return gFolderTree;
}

function GetSearchInput()
{
  if (!gSearchInput)
    gSearchInput = document.getElementById("searchInput");
  return gSearchInput;
}

function GetMessagePane()
{
  if (!gMessagePane)
    gMessagePane = document.getElementById("messagepanebox");
  return gMessagePane;
}

function GetMessagePaneFrame()
{
  return window.content;
}

function getMailToolbox()
{
  return document.getElementById("mail-toolbox");
}

function FindInSidebar(currentWindow, id)
{
  var item = currentWindow.document.getElementById(id);
  if (item)
    return item;

  for (var i = 0; i < currentWindow.frames.length; ++i)
  {
    var frameItem = FindInSidebar(currentWindow.frames[i], id);
    if (frameItem)
      return frameItem;
  }

  return null;
}

function GetThreadAndMessagePaneSplitter()
{
  if (!gThreadAndMessagePaneSplitter)
    gThreadAndMessagePaneSplitter = document.getElementById('threadpane-splitter');
  return gThreadAndMessagePaneSplitter;
}

function GetUnreadCountElement()
{
  if (!gUnreadCount)
    gUnreadCount = document.getElementById('unreadMessageCount');
  return gUnreadCount;
}

function GetTotalCountElement()
{
  if (!gTotalCount)
    gTotalCount = document.getElementById('totalMessageCount');
  return gTotalCount;
}

function IsMessagePaneCollapsed()
{
  return GetMessagePane().collapsed;
}

function IsFolderPaneCollapsed()
{
  return GetFolderTree().parentNode.collapsed;
}

function ClearThreadPaneSelection()
{
  try {
    if (gDBView) {
      var treeView = gDBView.QueryInterface(Components.interfaces.nsITreeView);
      var treeSelection = treeView.selection;
      if (treeSelection)
        treeSelection.clearSelection();
    }
  }
  catch (ex) {
    dump("ClearThreadPaneSelection: ex = " + ex + "\n");
  }
}

function ClearMessagePane()
{
  if(gHaveLoadedMessage)
  {
    gHaveLoadedMessage = false;
    if (GetMessagePaneFrame().location.href != "about:blank")
        GetMessagePaneFrame().location.href = "about:blank";

    // hide the message header view AND the message pane...
    HideMessageHeaderPane();
    gMessageNotificationBar.clearMsgNotifications();
    ClearPendingReadTimer();
  }
}

// Function to change the highlighted row to where the mouse was clicked
// without loading the contents of the selected row.
// It will also keep the outline/dotted line in the original row.
function ChangeSelectionWithoutContentLoad(event, tree)
{
    var treeBoxObj = tree.treeBoxObject;
    var treeSelection = treeBoxObj.view.selection;

    var row = treeBoxObj.getRowAt(event.clientX, event.clientY);
    // make sure that row.value is valid so that it doesn't mess up
    // the call to ensureRowIsVisible().
    if((row >= 0) && !treeSelection.isSelected(row))
    {
        var saveCurrentIndex = treeSelection.currentIndex;
        treeSelection.selectEventsSuppressed = true;
        treeSelection.select(row);
        treeSelection.currentIndex = saveCurrentIndex;
        treeBoxObj.ensureRowIsVisible(row);
        treeSelection.selectEventsSuppressed = false;

        // Keep track of which row in the thread pane is currently selected.
        if(tree.id == "threadTree")
          gThreadPaneCurrentSelectedIndex = row;
    }
    event.stopPropagation();
}

function TreeOnMouseDown(event)
{
    // Detect right mouse click and change the highlight to the row
    // where the click happened without loading the message headers in
    // the Folder or Thread Pane.
    if (event.button == 2)
    {
      gRightMouseButtonDown = true;
      ChangeSelectionWithoutContentLoad(event, event.target.parentNode);
    }
    else
      gRightMouseButtonDown = false;
}

function FolderPaneOnClick(event)
{
    // we only care about button 0 (left click) events
    if (event.button != 0)
        return;

    var folderTree = GetFolderTree();
    var row = {};
    var col = {};
    var elt = {};
    folderTree.treeBoxObject.getCellAt(event.clientX, event.clientY, row, col, elt);
    if (row.value == -1) {
      if (event.originalTarget.localName == "treecol")
      {
        // clicking on the name column in the folder pane should not sort
        event.stopPropagation();
      }
    }
    else if ((event.originalTarget.localName == "slider") ||
             (event.originalTarget.localName == "scrollbarbutton")) {
      event.stopPropagation();
    }
    else if ((event.detail == 2) && (elt.value != "twisty") &&
             (folderTree.view.getLevel(row.value) != 0)) {
      FolderPaneDoubleClick(row.value, event);
    }
}

function FolderPaneDoubleClick(folderIndex, event)
{
    if (!gPrefBranch.getBoolPref("mailnews.reuse_thread_window2"))
    {
      var folderResource = GetFolderResource(GetFolderTree(), folderIndex);
      // Open a new msg window only if we are double clicking on
      // folders or newsgroups.
      MsgOpenNewWindowForFolder(folderResource.Value, -1 /* key */);

      // double clicking should not toggle the open / close state of the
      // folder.  this will happen if we don't prevent the event from
      // bubbling to the default handler in tree.xml
      event.stopPropagation();
    }
}

function ChangeSelection(tree, newIndex)
{
    if(newIndex >= 0)
    {
        tree.view.selection.select(newIndex);
        tree.treeBoxObject.ensureRowIsVisible(newIndex);
    }
}

//xxx this function should go away
function GetSelectedFolders()
{
    return GetSelectedMsgFolders();
}

function GetSelectedMsgFolders()
{
    var folderArray = [];
    var folderTree = GetFolderTree();
    var rangeCount = folderTree.view.selection.getRangeCount();

    for (var i = 0; i < rangeCount; i++)
    {
        var startIndex = {};
        var endIndex = {};
        folderTree.view.selection.getRangeAt(i, startIndex, endIndex);
        for (var j = startIndex.value; j <= endIndex.value; j++)
        {
          var folderResource = GetFolderResource(folderTree, j);
          var msgFolder = folderResource.QueryInterface(Components.interfaces.nsIMsgFolder);
          folderArray.push(msgFolder);
        }
    }

    return folderArray;
}

function GetFirstSelectedMessage()
{
    try {
        return gDBView.URIForFirstSelectedMessage;
    }
    catch (ex) {
        return null;
    }
}

function GetSelectedIndices(dbView)
{
  try {
    return dbView.getIndicesForSelection({});
  }
  catch (ex) {
    dump("ex = " + ex + "\n");
    return null;
  }
}

function GetSelectedMessages()
{
  try {
    var messageArray = GetDBView().getURIsForSelection({});
    return messageArray.length ? messageArray : null;
  }
  catch (ex) {
    dump("ex = " + ex + "\n");
    return null;
  }
}

function GetLoadedMsgFolder()
{
    if (!gDBView) return null;
    return gDBView.msgFolder;
}

function GetLoadedMessage()
{
    try {
        return gDBView.URIForFirstSelectedMessage;
    }
    catch (ex) {
        return null;
    }
}

//Clear everything related to the current message. called after load start page.
function ClearMessageSelection()
{
  ClearThreadPaneSelection();
}

// Figures out how many messages are selected (hilighted - does not necessarily
// have the dotted outline) above a given index row value in the thread pane.
function NumberOfSelectedMessagesAboveCurrentIndex(index)
{
  var numberOfMessages = 0;
  var indicies = GetSelectedIndices(gDBView);

  if (indicies && indicies.length)
  {
    for (var i = 0; i < indicies.length; i++)
    {
      if (indicies[i] < index)
        ++numberOfMessages;
      else
        break;
    }
  }
  return numberOfMessages;
}

function SetNextMessageAfterDelete()
{
  var treeSelection = GetThreadTree().view.selection;

  if (treeSelection.isSelected(treeSelection.currentIndex))
  {
    gNextMessageViewIndexAfterDelete = gDBView.msgToSelectAfterDelete;
    gSelectedIndexWhenDeleting = treeSelection.currentIndex;
  }
  else if(gDBView.removeRowOnMoveOrDelete)
  {
    // Only set gThreadPaneDeleteOrMoveOccurred to true if the message was
    // truly moved to the trash or deleted, as opposed to an IMAP delete
    // (where it is only "marked as deleted".  This will prevent bug 142065.
    //
    // If it's an IMAP delete, then just set gNextMessageViewIndexAfterDelete
    // to treeSelection.currentIndex (where the outline is at) because nothing
    // was moved or deleted from the folder.
    gThreadPaneDeleteOrMoveOccurred = true;
    gNextMessageViewIndexAfterDelete = treeSelection.currentIndex - NumberOfSelectedMessagesAboveCurrentIndex(treeSelection.currentIndex);
  }
  else
    gNextMessageViewIndexAfterDelete = treeSelection.currentIndex;
}

function EnsureFolderIndex(builder, msgFolder)
{
  // try to get the index of the folder in the tree
  var index = builder.getIndexOfResource(msgFolder);
  if (index == -1) {
    // if we couldn't find the folder, open the parent
    builder.toggleOpenState(EnsureFolderIndex(builder, msgFolder.parent));
    index = builder.getIndexOfResource(msgFolder);
  }
  return index;
}

function SelectFolder(folderUri)
{
  var folderTree = GetFolderTree();
  var msgFolder = GetMsgFolderFromUri(folderUri);

  // Before we can select a folder, we need to make sure it is "visible"
  // in the tree. To do that, we need to ensure that all its
  // ancestors are expanded.
  var folderIndex = EnsureFolderIndex(folderTree.builderView, msgFolder);
  ChangeSelection(folderTree, folderIndex);
}

function SelectMessage(messageUri)
{
  var msgHdr = messenger.messageServiceFromURI(messageUri).messageURIToMsgHdr(messageUri);
  if (msgHdr)
    gDBView.selectMsgByKey(msgHdr.messageKey);
}

function ReloadMessage()
{
  gDBView.reloadMessage();
}

function GetDBView()
{
  return gDBView;
}

function GetFolderResource(tree, index)
{
  return tree.builderView.getResourceAtIndex(index);
}

function LoadNavigatedToMessage(msgHdr, folder, folderUri)
{
  if (IsCurrentLoadedFolder(folder))
  {
    gDBView.selectMsgByKey(msgHdr.messageKey);
  }
  else
  {
    gStartMsgKey = msgHdr.messageKey;
    SelectFolder(folderUri);
  }
}

// Some of the per account junk mail settings have been
// converted to global prefs. Let's try to migrate some
// of those settings from the default account.
function MigrateJunkMailSettings()
{
  var junkMailSettingsVersion = gPrefBranch.getIntPref("mail.spam.version");
  if (!junkMailSettingsVersion)
  {
    // Get the default account, check to see if we have values for our
    // globally migrated prefs.
    var defaultAccount;
    try {
      defaultAccount = accountManager.defaultAccount;
    } catch (ex) {}
    if (defaultAccount && defaultAccount.incomingServer)
    {
      // we only care about
      var prefix = "mail.server." + defaultAccount.incomingServer.key + ".";
      if (gPrefBranch.prefHasUserValue(prefix + "manualMark"))
        gPrefBranch.setBoolPref("mail.spam.manualMark", pref.getBoolPref(prefix + "manualMark"));
      if (gPrefBranch.prefHasUserValue(prefix + "manualMarkMode"))
        gPrefBranch.setIntPref("mail.spam.manualMarkMode", pref.getIntPref(prefix + "manualMarkMode"));
      if (gPrefBranch.prefHasUserValue(prefix + "spamLoggingEnabled"))
        gPrefBranch.setBoolPref("mail.spam.logging.enabled", pref.getBoolPref(prefix + "spamLoggingEnabled"));
      if (gPrefBranch.prefHasUserValue(prefix + "markAsReadOnSpam"))
        gPrefBranch.setBoolPref("mail.spam.markAsReadOnSpam", pref.getBoolPref(prefix + "markAsReadOnSpam"));
    }
    // bump the version so we don't bother doing this again.
    gPrefBranch.setIntPref("mail.spam.version", 1);
  }
}

// The first time a user runs a build that supports folder views, pre-populate the favorite folders list
// with the existing INBOX folders.
function MigrateFolderViews()
{
  var folderViewsVersion = gPrefBranch.getIntPref("mail.folder.views.version");
  if (!folderViewsVersion)
  {
     var servers = accountManager.allServers;
     var server;
     var inbox;
     for (var index = 0; index < servers.Count(); index++)
     {
       server = servers.QueryElementAt(index, Components.interfaces.nsIMsgIncomingServer);
       if (server)
       {
         inbox = GetInboxFolder(server);
         if (inbox)
           inbox.setFlag(MSG_FOLDER_FLAG_FAVORITE);
       }
     }
    gPrefBranch.setIntPref("mail.folder.views.version", 1);
  }
}

// Thunderbird has been storing old attachment download meta data in downloads.rdf
// even though there was no way to show or clean up this data. Now that we are using
// the new download manager in toolkit, we don't want to present this old data.
// To migrate to the new download manager, remove downloads.rdf.
function MigrateAttachmentDownloadStore()
{
  var attachmentStoreVersion = gPrefBranch.getIntPref("mail.attachment.store.version");
  if (!attachmentStoreVersion)
  {
    var dirService = Components.classes["@mozilla.org/file/directory_service;1"]
                     .getService(Components.interfaces.nsIProperties);
    var downloadsFile = dirService.get("DLoads", Components.interfaces.nsIFile);
    if (downloadsFile && downloadsFile.exists())
      downloadsFile.remove(false);

    // bump the version so we don't bother doing this again.
    gPrefBranch.setIntPref("mail.attachment.store.version", 1);
  }
}

/**
 * Returns a string representation of a folder's specialFolder attribute.
 *
 * @param aFolder  the folder whose specialFolder attribute to return
 */
function getSpecialFolderString(aFolder) {
  if (aFolder.flags & 0x1000) // MSG_FOLDER_FLAG_INBOX
    return "Inbox";
  else if (aFolder.flags & 0x0100) // MSG_FOLDER_FLAG_TRASH
    return "Trash";
  else if (aFolder.flags & 0x0800) // MSG_FOLDER_FLAG_QUEUE
    return "Unsent Messages";
  else if (aFolder.flags & 0x0200) // MSG_FOLDER_FLAG_SENTMAIL
    return "Sent";
  else if (aFolder.flags & 0x0400) // MSG_FOLDER_FLAG_DRAFTS
    return "Drafts";
  else if (aFolder.flags & 0x400000) // MSG_FOLDER_FLAG_TEMPLATES
    return "Templates";
  else if (aFolder.flags & 0x40000000) // MSG_FOLDER_FLAG_JUNK
    return "Junk";
  else if (aFolder.flags & 0x0020) // MSG_FOLDER_FLAG_VIRTUAL
    return "Virtual";
  else
    return "none";
}

