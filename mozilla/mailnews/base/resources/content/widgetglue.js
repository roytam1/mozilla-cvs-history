/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
 * The Original Code is Mozilla Communicator client code, released
 * March 31, 1998.
 * 
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation. Portions created by Netscape are
 * Copyright (C) 1998-1999 Netscape Communications Corporation. All
 * Rights Reserved.
 */

/*
 * widget-specific wrapper glue. There should be one function for every
 * widget/menu item, which gets some context (like the current selection)
 * and then calls a function/command in commandglue
 */

// Default Controller object
var DefaultController =
{
	IsCommandEnabled: function(command)
	{
		switch ( command )
		{
			case "cmd_undo":
				return true;
			
			case "cmd_redo":
				return true;
			
			default:
				return false;
		}
	},

	DoCommand: function(command)
	{
		switch ( command )
		{
			case "cmd_undo":
				messenger.Undo();
				break;
			
			case "cmd_redo":
				messenger.Redo();
				break;
		}
	}
};


// Controller object for folder pane
var FolderPaneController =
{
	IsCommandEnabled: function(command)
	{
		dump("FolderPaneController.IsCommandEnabled\n");
		switch ( command )
		{
			case "cmd_selectAll":
				return true;
			
			case "cmd_delete":
				goSetMenuValue(command, 'valueFolder');
				return true;
			
			default:
				return false;
		}
	},

	DoCommand: function(command)
	{
		switch ( command )
		{
			case "cmd_selectAll":
				var folderTree = GetFolderTree();
				if ( folderTree )
				{
					dump("select all now!!!!!!" + "\n");
					folderTree.selectAll();
				}
				break;
			
			case "cmd_delete":
				MsgDeleteFolder();
				break;
		}
	}
};


// Controller object for thread pane
var ThreadPaneController =
{
	IsCommandEnabled: function(command)
	{
		dump("ThreadPaneController.IsCommandEnabled\n");
		switch ( command )
		{
			case "cmd_selectAll":
				return true;
			
			case "cmd_delete":
				goSetMenuValue(command, 'valueMessage');
				return true;
			
			default:
				return false;
		}
	},

	DoCommand: function(command)
	{
		switch ( command )
		{
			case "cmd_selectAll":
				var threadTree = GetThreadTree();
				if ( threadTree )
				{
					dump("select all now!!!!!!" + "\n");
					threadTree.selectAll();
				}
				break;
			
			case "cmd_delete":
				MsgDeleteMessage(false);
				break;
		}
	}
};


function SetupCommandUpdateHandlers()
{
	dump("SetupCommandUpdateHandlers\n");

	var widget;
	
	// folder pane
	widget = GetFolderTree();
	if ( widget )
		widget.controller = FolderPaneController;
	
	// thread pane
	widget = GetThreadTree();
	if ( widget )
		widget.controller = ThreadPaneController;
}


var viewShowAll =0;
var viewShowRead = 1;
var viewShowUnread =2;
var viewShowWatched = 3;


function MsgLoadNewsMessage(url)
{
  dump("\n\nMsgLoadNewsMessage from XUL\n");
  OpenURL(url);
}

function MsgHome(url)
{
  window.open( url, "_blank", "chrome,dependent=yes,all" );
}

function MsgGetMessage() 
{
  GetNewMessages();
}

function MsgDeleteMessage(fromToolbar)
{
  //dump("\nMsgDeleteMessage from XUL\n");
  //dump("from toolbar? " + fromToolbar + "\n");

  var tree = GetThreadTree();
  if(tree) {
	var srcFolder = GetThreadTreeFolder();
	// if from the toolbar, return right away if this is a news message
	// only allow cancel from the menu:  "Edit | Cancel / Delete Message"
	if (fromToolbar) {
		uri = srcFolder.getAttribute('ref');
		//dump("uri[0:6]=" + uri.substring(0,6) + "\n");
		if (uri.substring(0,6) == "news:/") {
			//dump("delete ignored!\n");
			return;
		}
	}
	dump("tree is valid\n");
	//get the selected elements
	var messageList = tree.selectedItems;
	var nextMessage = GetNextMessageAfterDelete(messageList);
	//get the current folder
	messenger.DeleteMessages(tree, srcFolder, messageList);
	SelectNextMessage(nextMessage);
  }
}

function MsgDeleteFolder()
{
	//get the selected elements
	var tree = GetFolderTree();
	var folderList = tree.selectedItems;
	var i;
	var folder, parent;
	for(i = 0; i < folderList.length; i++)
	{
		folder = folderList[i];
	    folderuri = folder.getAttribute('id');
		dump(folderuri);
		parent = folder.parentNode.parentNode;	
	    var parenturi = parent.getAttribute('id');
		if(parenturi)
			dump(parenturi);
		else
			dump("No parenturi");
		dump("folder = " + folder.nodeName + "\n"); 
		dump("parent = " + parent.nodeName + "\n"); 
		messenger.DeleteFolders(tree.database, parent, folder);
	}


}

function MsgNewMessage(event)
{
  ComposeMessage(0, 0);
} 

function MsgReplyMessage(event)
{
  dump("\nMsgReplyMessage from XUL\n");
  ComposeMessage(1, 0);
}

function MsgReplyToAllMessage(event) 
{
  dump("\nMsgReplyToAllMessage from XUL\n");
  ComposeMessage(2, 0);
}

function MsgForwardMessage(event)
{
  dump("\nMsgForwardMessage from XUL\n");
  var tree = GetThreadTree();
  //get the selected elements
  var messageList = tree.selectedItems;
  messenger.forwardMessages(messageList, -1);
}

function MsgForwardAsAttachment(event)
{
  dump("\nMsgForwardAsAttachment from XUL\n");
  var tree = GetThreadTree();
  //get the selected elements
  var messageList = tree.selectedItems;
  messenger.forwardMessages(messageList, 0);
}

function MsgForwardAsInline(event)
{
  dump("\nMsgForwardAsInline from XUL\n");
  var tree = GetThreadTree();
  //get the selected elements
  var messageList = tree.selectedItems;
  messenger.forwardMessages(messageList, 1);
}

function MsgCopyMessage(destFolder)
{
	// Get the id for the folder we're copying into
    destUri = destFolder.getAttribute('id');
	dump(destUri);

	var tree = GetThreadTree();
	if(tree)
	{
		//Get the selected messages to copy
		var messageList = tree.selectedItems;
		//get the current folder

	//	dump('In copy messages.  Num Selected Items = ' + messageList.length);
	//	dump('\n');
		var srcFolder = GetThreadTreeFolder();
		messenger.CopyMessages(tree.database, srcFolder, destFolder, messageList, false);
	}	
}

function MsgMoveMessage(destFolder)
{
	// Get the id for the folder we're copying into
    destUri = destFolder.getAttribute('id');
	dump(destUri);

	var tree = GetThreadTree();
	if(tree)
	{
		//Get the selected messages to copy
		var messageList = tree.selectedItems;
		//get the current folder
		var nextMessage = GetNextMessageAfterDelete(messageList);
		var srcFolder = GetThreadTreeFolder();
		messenger.CopyMessages(tree.database, srcFolder, destFolder, messageList, true);
		SelectNextMessage(nextMessage);
	}	
}

function MsgViewAllMsgs() 
{
	dump("MsgViewAllMsgs");

	if(messageView)
	{
		messageView.viewType = viewShowAll;
		messageView.showThreads = false;
	}
	RefreshThreadTreeView();
}

function MsgViewUnreadMsg()
{
	dump("MsgViewUnreadMsgs");

	if(messageView)
	{
		messageView.viewType = viewShowUnread;
		view.showThreads = false;
	}

	RefreshThreadTreeView();
}

function MsgViewAllThreadMsgs()
{
	dump("MsgViewAllMessagesThreaded");

	if(messageView)
	{
		view.viewType = viewShowAll;
		view.showThreads = true;
	}
	RefreshThreadTreeView();
}

function MsgSortByDate()
{
	SortThreadPane('DateColumn', 'http://home.netscape.com/NC-rdf#Date');
}

function MsgSortBySender()
{
	SortThreadPane('AuthorColumn', 'http://home.netscape.com/NC-rdf#Sender');
}

function MsgSortByStatus()
{
	SortThreadPane('StatusColumn', 'http://home.netscape.com/NC-rdf#Status');
}

function MsgSortBySubject()
{
	SortThreadPane('SubjectColumn', 'http://home.netscape.com/NC-rdf#Subject');
}

function MsgNewFolder()
{
	MsgNewSubfolder("chrome://messenger/content/newFolderNameDialog.xul","New Folder");
}

function MsgSubscribe()
{
	MsgNewSubfolder("chrome://messenger/content/subscribeDialog.xul","Subscribe");
}

function MsgNewSubfolder(chromeWindowURL,windowTitle)
{
	try {
		var folderTree = GetFolderTree(); 
		var selectedFolderList = folderTree.selectedItems;
	
		if (selectedFolderList.length != 1) {
			// dump("ERROR:  you can only select one folder / server to add new folder / subscribe to.\n");
			return;
		}
		var selectedFolder = selectedFolderList[0];
		if (selectedFolder)
		{
			preselectedURI = selectedFolder.getAttribute('id');
			// dump("folder to preselect: " + preselectedURI + "\n");
			var dialog = window.openDialog(
				chromeWindowURL,
				"",
				"chrome",
				{preselectedURI:preselectedURI, title:windowTitle,
				okCallback:NewFolder});
		}
	}
	catch (ex) {
		// dump("ERROR:  perhaps nothing in the folder pane is selected?\n")
	}
}

function NewFolder(name)
{
    var folderTree = GetFolderTree(); 
	var selectedFolderList = folderTree.selectedItems;
	var selectedFolder = selectedFolderList[0];

	messenger.NewFolder(folderTree.database, selectedFolder, name);
}


function MsgAccountManager()
{
    var result = {refresh: false};
    window.openDialog("chrome://messenger/content/AccountManager.xul",
                      "AccountManager", "chrome,modal", result);
    if (result.refresh)
        refreshFolderPane();
}

function MsgAccountWizard()
{
    var result = {refresh: false};
    window.openDialog("chrome://messenger/content/AccountWizard.xul",
                      "AccountWizard", "chrome,modal", result);
    if(result.refresh)
        refreshFolderPane();
}

// refresh the folder tree by rerooting it
// hack until the account manager can tell RDF that new accounts
// have been created.
function refreshFolderPane()
{
    var folderTree = GetFolderTree();
    if (folderTree) {
        folderTree.clearItemSelection();
        var root = folderTree.getAttribute('ref');
        folderTree.setAttribute('ref', root);
    }
}

function MsgOpenAttachment() {}

function MsgSaveAsFile() 
{
  dump("\MsgSaveAsFile from XUL\n");
  var tree = GetThreadTree();
  //get the selected elements
  var messageList = tree.selectedItems;
  if (messageList && messageList.length == 1)
  {
      var uri = messageList[0].getAttribute('id');
      dump (uri);
      if (uri)
          messenger.saveAs(uri, true);
  }
}

function MsgSaveAsTemplate() 
{
  dump("\MsgSaveAsTemplate from XUL\n");
  var tree = GetThreadTree();
  //get the selected elements
  var messageList = tree.selectedItems;
  if (messageList && messageList.length == 1)
  {
      var uri = messageList[0].getAttribute('id');
      dump (uri);
      if (uri)
          messenger.saveAs(uri, false);
  }
}
function MsgSendUnsentMsg() 
{
	messenger.SendUnsentMessages();
}
function MsgLoadFirstDraft() 
{
	messenger.LoadFirstDraft();
}
function MsgUpdateMsgCount() {}

function MsgRenameFolder() 
{
    var tree = GetFolderTree();
    if (tree)
    {
        var folderList = tree.selectedItems;
        if (folderList && folderList.length == 1)
        {
            var folder = folderList[0];
            if (folder)
            {
                var dialog = window.openDialog(
                    "chrome://messenger/content/newFolderNameDialog.xul",
                    "newFolder",
                    "chrome,modal",
                    {title:"Rename Folder",
                              okCallback:RenameFolder});
            }
        }
    }
}

function RenameFolder(name)
{
    var tree = GetFolderTree();
    if (tree)
    {
        var folderList = tree.selectedItems;
        if (folderList && folderList.length == 1)
        {
            var folder = folderList[0];
            if (folder)
                messenger.RenameFolder(tree.database, folder, name);
        }
    }
}

function MsgEmptyTrash() 
{
    var tree = GetFolderTree();
    if (tree)
    {
        var folderList = tree.selectedItems;
        if (folderList)
        {
            var folder;
            folder = folderList[0];
            if (folder)
			{
                messenger.EmptyTrash(tree.database, folder);
				if(IsSpecialFolderSelected('Trash'))
				{
					RefreshThreadTreeView()
				}
			}
        }
    }
}

function MsgCompactFolder() 
{
	//get the selected elements
	var tree = GetFolderTree();
    if (tree)
    {
        var folderList = tree.selectedItems;
        if (folderList)
        {
            var i;
            var folder;
            for(i = 0; i < folderList.length; i++)
            {
                folder = folderList[i];
                if (folder)
                {
                    folderuri = folder.getAttribute('id');
                    dump(folderuri);
                    dump("folder = " + folder.nodeName + "\n"); 
                    messenger.CompactFolder(tree.database, folder);
                }
            }
        }
	}
}

function MsgImport() {}
function MsgWorkOffline() {}
function MsgSynchronize() {}
function MsgGetSelectedMsg() {}
function MsgGetFlaggedMsg() {}

function MsgSelectThread() {}
function MsgSelectFlaggedMsg() {}
function MsgFind() {}
function MsgFindAgain() {}

function MsgSearchMessages() {
    window.openDialog("chrome://messenger/content/SearchDialog.xul", "SearchMail", "chrome");
}

function MsgFilters() {
    window.openDialog("chrome://messenger/content/FilterListDialog.xul", "FilterDialog", "chrome");
}

function MsgToggleMessagePane()
{
    MsgToggleSplitter("messagePaneSplitter");
}

function MsgToggleFolderPane()
{
    MsgToggleSplitter("sidebarsplitter");
}

function MsgToggleSplitter(id)
{
    var splitter = document.getElementById(id);
    var state = splitter.getAttribute("state");
    if (state == "collapsed")
        splitter.setAttribute("state", null);
    else
        splitter.setAttribute("state", "collapsed")
}

function MsgShowFolders()
{


}

function MsgFolderProperties() {}

function MsgShowLocationbar() {}
function MsgSortByFlag() {}
function MsgSortByPriority() {}
function MsgSortBySize() {}
function MsgSortByThread() {}
function MsgSortByUnread() {}
function MsgSortByOrderReceived() {}
function MsgSortAscending() {}
function MsgSortDescending() {}
function MsgViewThreadsUnread() {}
function MsgViewWatchedThreadsUnread() {}
function MsgViewIgnoreThread() {}
function MsgViewAllHeaders() {}
function MsgViewNormalHeaders() {}
function MsgViewBriefHeaders() {}
function MsgViewAttachInline() {}
function MsgWrapLongLines() {}
function MsgIncreaseFont() {}
function MsgDecreaseFont() {}
function MsgReload() {}
function MsgShowImages() {}
function MsgRefresh() {}
function MsgViewPageSource() {}
function MsgViewPageInfo() {}
function MsgFirstUnreadMessage() {}
function MsgFirstFlaggedMessage() {}

function MsgStop() {
	dump("sorry, stop doesn't work yet.\n");
}

function MsgNextMessage()
{
	GoNextMessage(GoMessage, false);
}

function MsgNextUnreadMessage()
{
	GoNextMessage(GoUnreadMessage, true);
}
function MsgNextFlaggedMessage()
{
	GoNextMessage(GoFlaggedMessage, true);
}

function MsgPreviousMessage()
{
	GoPreviousMessage(GoMessage, false);
}

function MsgPreviousUnreadMessage()
{
	GoPreviousMessage(GoUnreadMessage, true);
}

function MsgPreviousFlaggedMessage()
{
	GoPreviousMessage(GoFlaggedMessage, true);
}

function MsgGoBack() {}
function MsgGoForward() {}
function MsgEditMessageAsNew() {}
function MsgAddSenderToAddressBook() {}
function MsgAddAllToAddressBook() {}

function MsgMarkMsgAsRead(markRead)
{
  dump("\MsgMarkMsgAsRead from XUL\n");
  var tree = GetThreadTree();
  //get the selected elements
  var messageList = tree.selectedItems;
  messenger.MarkMessagesRead(tree.database, messageList, markRead);
}

function MsgMarkThreadAsRead() {}
function MsgMarkByDate() {}
function MsgMarkAllRead()
{
	var folderTree = GetFolderTree();; 
	var selectedFolderList = folderTree.selectedItems;
	if(selectedFolderList.length > 0)
	{
		var selectedFolder = selectedFolderList[0];
		messenger.MarkAllMessagesRead(folderTree.database, selectedFolder);
	}
	else {
		dump("Nothing was selected\n");
	}
}

function MsgMarkAsFlagged(markFlagged)
{
  dump("\MsgMarkMsgAsFlagged from XUL\n");
  var tree = GetThreadTree();
  //get the selected elements
  var messageList = tree.selectedItems;
  messenger.MarkMessagesFlagged(tree.database, messageList, markFlagged);

}

function MsgIgnoreThread() {}
function MsgWatchThread() {}

var gStatusObserver;
        var bindCount = 0;
        function onStatus() {
            if (!gStatusObserver)
                gStatusObserver = document.getElementById("Messenger:Status");
            if ( gStatusObserver ) {
                var text = gStatusObserver.getAttribute("value");
                if ( text == "" ) {
                    text = defaultStatus;
                }
                var statusText = document.getElementById("statusText");
                if ( statusText ) {
                    statusText.setAttribute( "value", text );
                }
            } else {
                dump("Can't find status broadcaster!\n");
            }
        }

var gThrobberObserver;
var gMeterObserver;
		var startTime = 0;
        function onProgress() {
            if (!gThrobberObserver)
                gThrobberObserver = document.getElementById("Messenger:Throbber");
            if (!gMeterObserver)
                gMeterObserver = document.getElementById("Messenger:LoadingProgress");
            if ( gThrobberObserver && gMeterObserver ) {
                var busy = gThrobberObserver.getAttribute("busy");
                var wasBusy = gMeterObserver.getAttribute("mode") == "undetermined" ? "true" : "false";
                if ( busy == "true" ) {
                    if ( wasBusy == "false" ) {
                        // Remember when loading commenced.
    				    startTime = (new Date()).getTime();
                        // Turn progress meter on.
                        gMeterObserver.setAttribute("mode","undetermined");
                    }
                    // Update status bar.
                } else if ( busy == "false" && wasBusy == "true" ) {
                    // Record page loading time.
                    if (!gStatusObserver)
                        gStatusObserver = document.getElementById("Messenger:Status");
                    if ( gStatusObserver ) {
						var elapsed = ( (new Date()).getTime() - startTime ) / 1000;
						var msg = "Document: Done (" + elapsed + " secs)";
						dump( msg + "\n" );
                        gStatusObserver.setAttribute("value",msg);
                        defaultStatus = msg;
                    }
                    // Turn progress meter off.
                    gMeterObserver.setAttribute("mode","normal");
                }
            }
        }
        function dumpProgress() {
            var broadcaster = document.getElementById("Messenger:LoadingProgress");

            dump( "bindCount=" + bindCount + "\n" );
            dump( "broadcaster mode=" + broadcaster.getAttribute("mode") + "\n" );
            dump( "broadcaster value=" + broadcaster.getAttribute("value") + "\n" );
            dump( "meter mode=" + meter.getAttribute("mode") + "\n" );
            dump( "meter value=" + meter.getAttribute("value") + "\n" );
        }
