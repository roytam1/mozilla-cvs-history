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

/* This is where functions related to the 3 pane window are kept */

/* globals for a particular window */
var messengerProgID        = "component://netscape/messenger";
var statusFeedbackProgID   = "component://netscape/messenger/statusfeedback";
var messageViewProgID      = "component://netscape/messenger/messageview";
var mailSessionProgID      = "component://netscape/messenger/services/session";
var accountManagerProgID   = "component://netscape/messenger/account-manager";
var prefProgID             = "component://netscape/preferences";
var msgWindowProgID		   = "component://netscape/messenger/msgwindow";

var datasourceProgIDPrefix = "component://netscape/rdf/datasource?name=";
var accountManagerDSProgID = datasourceProgIDPrefix + "msgaccountmanager";
var folderDSProgID         = datasourceProgIDPrefix + "mailnewsfolders";
var messageDSProgID        = datasourceProgIDPrefix + "mailnewsmessages";

var gFolderTree;
var gThreadTree;
var gThreadAndMessagePaneSplitter = null;
var gUnreadCount = null;
var gTotalCount = null;

var gCurrentLoadingFolderURI;
var gCurrentFolderToReroot;
var gCurrentLoadingFolderIsThreaded = false;
var gCurrentLoadingFolderSortID ="";


// get the messenger instance
var messenger = Components.classes[messengerProgID].createInstance();
messenger = messenger.QueryInterface(Components.interfaces.nsIMessenger);

//Create datasources
var accountManagerDataSource = Components.classes[accountManagerDSProgID].createInstance();
var folderDataSource         = Components.classes[folderDSProgID].createInstance();
var messageDataSource        = Components.classes[messageDSProgID].createInstance();

var pref = Components.classes[prefProgID].getService(Components.interfaces.nsIPref);

//Create windows status feedback
var statusFeedback           = Components.classes[statusFeedbackProgID].createInstance();
statusFeedback = statusFeedback.QueryInterface(Components.interfaces.nsIMsgStatusFeedback);

//Create message view object
var messageView = Components.classes[messageViewProgID].createInstance();
messageView = messageView.QueryInterface(Components.interfaces.nsIMessageView);

//Create message window object
var msgWindow = Components.classes[msgWindowProgID].createInstance();
msgWindow = msgWindow.QueryInterface(Components.interfaces.nsIMsgWindow);

// the folderListener object
var folderListener = {
    OnItemAdded: function(parentItem, item, view) {},

	OnItemRemoved: function(parentItem, item, view){},

	OnItemPropertyChanged: function(item, property, oldValue, newValue) {},

	OnItemIntPropertyChanged: function(item, property, oldValue, newValue)
	{
		if(property == "TotalMessages" || property == "TotalUnreadMessages")
		{
			folder = item.QueryInterface(Components.interfaces.nsIMsgFolder);
			if(folder)
			{
				var folderResource = folder.QueryInterface(Components.interfaces.nsIRDFResource);
				if(folderResource)
				{
					var folderURI = folderResource.Value;
					var currentLoadedFolder = GetThreadTreeFolder();
					var currentURI = currentLoadedFolder.getAttribute('ref');
					if(currentURI == folderURI)
					{
						UpdateStatusMessageCounts(folder);
					}
				}


			}



		}
	
	},

	OnItemBoolPropertyChanged: function(item, property, oldValue, newValue) {},

	OnItemPropertyFlagChanged: function(item, property, oldFlag, newFlag) {},

	OnFolderLoaded: function (folder)
	{
		if(folder)
		{
			var resource = folder.QueryInterface(Components.interfaces.nsIRDFResource);
			if(resource)
			{
				var uri = resource.Value;
				dump('In OnFolderLoaded for ' + uri);
				dump('\n');
				if(uri == gCurrentFolderToReroot)
				{
					gCurrentFolderToReroot="";
					var msgFolder = folder.QueryInterface(Components.interfaces.nsIMsgFolder);
					if(msgFolder)
					{
						msgFolder.endFolderLoading();
						dump("before reroot in OnFolderLoaded\n");
						RerootFolder(uri, msgFolder, gCurrentLoadingFolderIsThreaded, gCurrentLoadingFolderSortID);
						gCurrentLoadingFolderIsThreaded = false;
						gCurrentLoadingFolderSortID = "";
					}
				}
				if(uri == gCurrentLoadingFolderURI)
				{
				  gCurrentLoadingFolderURI = "";
				  //Now let's select the first new message if there is one
				  SelectFirstNewMessage();
				}
			}

		}
	}
}

/* Functions related to startup */
function OnLoadMessenger()
{
  verifyAccounts();
    
  loadStartPage();
	InitMsgWindow();

	messenger.SetWindow(window, msgWindow);

	AddDataSources();
	InitPanes();

  loadStartFolder();

  AddToSession();

  // FIX ME - later we will be able to use onload from the overlay
  OnLoadMsgHeaderPane();

	var id = null;
	var headerchoice = null;

	try {
		headerchoice = pref.GetIntPref("mail.show_headers");
	}
	catch (ex) {
		dump("failed to get the header pref\n");
	}

	switch (headerchoice) {
		case 2:	
			id = "viewallheaders";
			break;
		case 0:
			id = "viewbriefheaders";
			break;
		case 1:	
			id = "viewnormalheaders";
			break;
		default:
			id = "viewnormalheaders";
			break;
	}

	var menuitem = document.getElementById(id);

	try {
		// not working right yet.  see bug #??????
		// menuitem.setAttribute("checked", "true"); 
	}
	catch (ex) {
		dump("failed to set the view headers menu item\n");
	}
}

function OnUnloadMessenger()
{
	dump("\nOnUnload from XUL\nClean up ...\n");
	var mailSession = Components.classes[mailSessionProgID].getService();
	if(mailSession)
	{
		mailSession = mailSession.QueryInterface(Components.interfaces.nsIMsgMailSession);
		if(mailSession)
		{
			mailSession.RemoveFolderListener(folderListener);
			mailSession.RemoveMsgWindow(msgWindow);
			messenger.SetWindow(null, null);
		}
	}

    saveWindowPosition();
}

function saveWindowPosition()
{
    // Get the current window position/size.
    var x = window.screenX;
    var y = window.screenY;
    var h = window.outerHeight;
    var w = window.outerWidth;

    // Store these into the window attributes (for persistence).
    var win = document.getElementById( "messengerWindow" );
    win.setAttribute( "x", x );
    win.setAttribute( "y", y );
    win.setAttribute( "height", h );
    win.setAttribute( "width", w );
    // save x, y, width, height
}


function verifyAccounts() {
    try {
        var am = Components.classes[accountManagerProgID].getService(Components.interfaces.nsIMsgAccountManager);

        var accounts = am.accounts;

        // as long as we have some accounts, we're fine.
        if (accounts.Count() > 0) return;

        try {
            dump("attempt to UpgradePrefs.  If that fails, open the account wizard.\n");
            am.UpgradePrefs();
            refreshFolderPane();
        }
        catch (ex) {
            // upgrade prefs failed, so open account wizard
            MsgAccountWizard();
        }
        

    }
    catch (ex) {
        dump("error verifying accounts " + ex + "\n");
        return;
    }
}


function loadStartPage() {

	var startpage = "about:blank";

    try {
		startpageenabled= pref.GetBoolPref("mailnews.start_page.enabled");
        
		if (startpageenabled)
			startpage = pref.CopyCharPref("mailnews.start_page.url");
        window.frames["messagepane"].location = startpage;

        dump("start message pane with: " + startpage + "\n");
	}
    catch (ex) {
        dump("Error loading start page.\n");
        return;
    }
}

function loadStartFolder()
{
	//Load StartFolder
    try {
        var startFolder = pref.CopyCharPref("mailnews.start_folder");
        //ChangeFolderByURI(startFolder);
		//	var folder = OpenFolderTreeToFolder(startFolder);
    }
    catch(ex) {

    }

}

function AddToSession()
{
    try {
        var mailSession = Components.classes[mailSessionProgID].getService(Components.interfaces.nsIMsgMailSession);
        
        mailSession.AddFolderListener(folderListener);
        mailSession.AddMsgWindow(msgWindow);
	} catch (ex) {
        dump("Error adding to session\n");
    }
}

function InitMsgWindow()
{
	msgWindow.statusFeedback = statusFeedback;
	msgWindow.messageView = messageView;
	msgWindow.msgHeaderSink = messageHeaderSink;
  msgWindow.SetDOMWindow(window);
}

function AddDataSources()
{

	//to move menu item
	accountManagerDataSource = accountManagerDataSource.QueryInterface(Components.interfaces.nsIRDFDataSource);
	folderDataSource = folderDataSource.QueryInterface(Components.interfaces.nsIRDFDataSource);
	var moveMenu = document.getElementById('moveMenu');
	if(moveMenu)
	{
		moveMenu.database.AddDataSource(accountManagerDataSource);
		moveMenu.database.AddDataSource(folderDataSource);
		moveMenu.setAttribute('ref', 'msgaccounts:/');
	}

	//to copy menu item
	var copyMenu = document.getElementById('copyMenu');
	if(copyMenu)
	{
		copyMenu.database.AddDataSource(accountManagerDataSource);
		copyMenu.database.AddDataSource(folderDataSource);
		copyMenu.setAttribute('ref', 'msgaccounts:/');
	}
	//Add statusFeedback

	var msgDS = folderDataSource.QueryInterface(Components.interfaces.nsIMsgRDFDataSource);
	msgDS.window = msgWindow;

	msgDS = messageDataSource.QueryInterface(Components.interfaces.nsIMsgRDFDataSource);
	msgDS.window = msgWindow;

	msgDS = accountManagerDataSource.QueryInterface(Components.interfaces.nsIMsgRDFDataSource);
	msgDS.window = msgWindow;

}	

function InitPanes()
{
	var threadTree = GetThreadTree();
	if(threadTree);
		OnLoadThreadPane(threadTree);

	var folderTree = GetFolderTree();
	if(folderTree)
		OnLoadFolderPane(folderTree);
		
	SetupCommandUpdateHandlers();
}

function OnLoadFolderPane(folderTree)
{
	dump('In onLoadfolderPane\n');
    gFolderTree = folderTree;
	SortFolderPane('FolderColumn', 'http://home.netscape.com/NC-rdf#FolderTreeName');

	//Add folderDataSource and accountManagerDataSource to folderPane
	accountManagerDataSource = accountManagerDataSource.QueryInterface(Components.interfaces.nsIRDFDataSource);
	folderDataSource = folderDataSource.QueryInterface(Components.interfaces.nsIRDFDataSource);
	var database = folderTree.database;

	database.AddDataSource(accountManagerDataSource);
    database.AddDataSource(folderDataSource);
	folderTree.setAttribute('ref', 'msgaccounts:/');
}

function OnLoadThreadPane(threadTree)
{
    gThreadTree = threadTree;
	//Sort by date by default
	MsgSortByDate();
	// add folderSource to thread pane
	folderDataSource = folderDataSource.QueryInterface(Components.interfaces.nsIRDFDataSource);
	threadTree.database.AddDataSource(folderDataSource);

	//Add message data source
	messageDataSource = messageDataSource.QueryInterface(Components.interfaces.nsIRDFDataSource);
	threadTree.database.AddDataSource(messageDataSource);

	ShowThreads(false);
}

/* Functions for accessing particular parts of the window*/
function GetFolderTree()
{
    if (gFolderTree) return gFolderTree;
    
	var folderTree = document.getElementById('folderTree');
    gFolderTree = folderTree;
	return folderTree;
}

function FindInSidebar(currentWindow, id)
{
	var item = currentWindow.document.getElementById(id);
	if(item)
		return item;

	for(var i = 0; i < currentWindow.frames.length; i++)
	{
		var frameItem = FindInSidebar(currentWindow.frames[i], id);
		if(frameItem)
			return frameItem;
	}
}

function GetThreadTree()
{
    if (gThreadTree) return gThreadTree;
	var threadTree = document.getElementById('threadTree');
	if(!threadTree)
		dump('thread tree is null\n');
    gThreadTree = threadTree;
	return threadTree;
}

function GetThreadTreeFolder()
{
  var tree = GetThreadTree();
  return tree;
}

function GetThreadAndMessagePaneSplitter()
{
	if(gThreadAndMessagePaneSplitter) return gThreadAndMessagePaneSplitter;
	var splitter = document.getElementById('gray_horizontal_splitter');
	gThreadAndMessagePaneSplitter = splitter;
	return splitter;
}

function GetUnreadCountElement()
{
	if(gUnreadCount) return gUnreadCount;
	var unreadCount = document.getElementById('unreadMessageCount');
	gUnreadCount = unreadCount;
	return unreadCount;
}
function GetTotalCountElement()
{
	if(gTotalCount) return gTotalCount;
	var totalCount = document.getElementById('totalMessageCount');
	gTotalCount = totalCount;
	return totalCount;
}
function IsThreadAndMessagePaneSplitterCollapsed()
{
	var splitter = GetThreadAndMessagePaneSplitter();
	if(splitter)
	{
		var state  = splitter.getAttribute('state');
		return (state == "collapsed");
	}
	else
		return false;
}

function FindMessenger()
{
  return messenger;
}

function RefreshThreadTreeView()
{
	var currentFolder = GetThreadTreeFolder();  
	var currentFolderID = currentFolder.getAttribute('ref');
	//This will make us lose selection when this happens.
	//need to figure out if we have to save off selection or if
	//tree widget is responsible for this.
	ClearThreadTreeSelection();
	currentFolder.setAttribute('ref', currentFolderID);
}

function ClearThreadTreeSelection()
{
	var tree = GetThreadTree();
	if(tree)
	{
		tree.clearItemSelection();
	}

}

function ClearMessagePane()
{
    if (window.frames["messagepane"].location != "about:blank")
        window.frames["messagepane"].location = "about:blank";
    // hide the message header view AND the message pane...
    HideMessageHeaderPane();
}

function StopUrls()
{
	msgWindow.StopUrls();
}

function GetSelectedFolder()
{
	var tree = GetFolderTree();
	var selection = tree.selectedItems;
	if(selection.length > 0)
		return selection[0];
	else
		return null;

}

function ThreadPaneOnClick(event)
{
    var targetclass = event.target.getAttribute('class');
    debug('targetclass = ' + targetclass + '\n');

    if (targetclass == 'twisty') {
        // The twisty is nested three below the treeitem:
        // <treeitem>
        //   <treerow>
        //     <treecell>
        //       <box> <!-- anonymous -->
        //         <titledbutton class="twisty"> <!-- anonymous -->
        var treeitem = event.target.parentNode.parentNode.parentNode.parentNode;
		var open = treeitem.getAttribute('open');
		if(open == "true")
		{
			//open all of the children of the treeitem
			OpenThread(treeitem);
		}
		dump('clicked on a twisty\n');
    }

}

function OpenThread(treeitem)
{
	treeitem.setAttribute('notreadytodisplay', 'true');
	OpenTreeItemAndDescendants(treeitem);
	treeitem.setAttribute('notreadytodisplay', 'false');
}

function OpenTreeItemAndDescendants(treeitem)
{
	var open = treeitem.getAttribute('open');
	if(open != "true")
	{
		treeitem.setAttribute('open', 'true');
	}

	var treeitemChildNodes = treeitem.childNodes;
	var numTreeitemChildren = treeitemChildNodes.length;

	//if there's only one child then there are no treechildren so close it.
	if(numTreeitemChildren == 1)
		treeitem.setAttribute('open', '');
	else
	{
		for(var i = 0; i < numTreeitemChildren; i++)
		{
			var treeitemChild = treeitemChildNodes[i];
			if(treeitemChild.nodeName == 'treechildren')
			{
				var treechildrenChildNodes = treeitemChildNodes[i].childNodes;
				var numTreechildrenChildren = treechildrenChildNodes.length;
				
				for(var j = 0; j < numTreechildrenChildren; j++)
				{
					var treechildrenChild = treechildrenChildNodes[j];
					if(treechildrenChild.nodeName == 'treeitem')
					{
						treechildrenChild.setAttribute('open', 'true');
						//Open up all of this items
						OpenTreeItemAndDescendants(treechildrenChild);
					}
				}
			}
		}
	}

}
