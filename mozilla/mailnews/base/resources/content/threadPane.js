/* -*- Mode: Java; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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

var gOldNumSelected = 0;
function ThreadPaneOnClick(event)
{
    var t = event.originalTarget;

    if (t.localName != "treecell" &&
        t.localName != "treeitem" &&
        t.localName != "image")
        return;

    // fix for #48424.  bail out of here when the user is 
    // clicking on the column headers
    if (t.localName == "treecell") {
        if (t.parentNode && (t.parentNode.getAttribute("id") == "headRow")) {
            return;
        }
    }
   
    var targetclass = "";

    if (t.localName == "image" && (t.getAttribute('twisty') != 'true'))
      targetclass = t.parentNode.getAttribute('class');

    //dump('targetclass = ' + targetclass + '\n');

	if(targetclass.indexOf('unreadcol') != -1)
	{
		ToggleMessageRead(t.parentNode.parentNode.parentNode);
	}
	if(targetclass.indexOf('flagcol') != -1)
	{
		ToggleMessageFlagged(t.parentNode.parentNode.parentNode);
	}
    else if (t.getAttribute('twisty') == 'true') {
        // The twisty is nested three below the treeitem:
        // <treeitem>
        //   <treerow>
        //     <treecell>
        //         <button class="tree-cell-twisty"> <!-- anonymous -->
        var treeitem = t.parentNode.parentNode.parentNode;
		var open = treeitem.getAttribute('open');
		if(open == "true")
		{
			//open all of the children of the treeitem
			msgNavigationService.OpenTreeitemAndDescendants(treeitem);
		}
    }
	else if(event.detail == 2)
	{
		ThreadPaneDoubleClick();
	}
}

function ThreadPaneDoubleClick()
{
	var loadedFolder;
	var messageArray;
	var messageUri;

	if(IsSpecialFolderSelected("Drafts"))
	{
		loadedFolder = GetLoadedMsgFolder();
		messageArray = GetSelectedMessages();

		ComposeMessage(msgComposeType.Draft, msgComposeFormat.Default, loadedFolder, messageArray);
	}
	else if(IsSpecialFolderSelected("Templates"))
	{
		loadedFolder = GetLoadedMsgFolder();
		messageArray = GetSelectedMessages();
		ComposeMessage(msgComposeType.Template, msgComposeFormat.Default, loadedFolder, messageArray);
	}
	else
	{
        MsgOpenSelectedMessages();
	}
}

function ThreadPaneKeyPress(event)
{
  if (event.keyCode == 13)
	ThreadPaneDoubleClick();
  return;
}

function MsgSortByDate()
{
	SortThreadPane('DateColumn', 'http://home.netscape.com/NC-rdf#Date', null, true, null, true);
}

function MsgSortBySender()
{
	SortThreadPane('AuthorColumn', 'http://home.netscape.com/NC-rdf#Sender', 'http://home.netscape.com/NC-rdf#Date', true, null, true);
}

function MsgSortByRecipient()
{
	SortThreadPane('AuthorColumn', 'http://home.netscape.com/NC-rdf#Recipient', 'http://home.netscape.com/NC-rdf#Date', true, null, true);
}

function MsgSortByStatus()
{
	SortThreadPane('StatusColumn', 'http://home.netscape.com/NC-rdf#Status', 'http://home.netscape.com/NC-rdf#Date', true, null, true);
}

function MsgSortBySubject()
{
	SortThreadPane('SubjectColumn', 'http://home.netscape.com/NC-rdf#Subject', 'http://home.netscape.com/NC-rdf#Date', true, null, true);
}

function MsgSortByFlagged() 
{
	SortThreadPane('FlaggedButtonColumn', 'http://home.netscape.com/NC-rdf#Flagged', 'http://home.netscape.com/NC-rdf#Date', true, null, true);
}

function MsgSortByPriority()
{
	SortThreadPane('PriorityColumn', 'http://home.netscape.com/NC-rdf#Priority', 'http://home.netscape.com/NC-rdf#Date',true, null, true);
}

function MsgSortBySize() 
{
	SortThreadPane('MemoryColumn', 'http://home.netscape.com/NC-rdf#Size', 'http://home.netscape.com/NC-rdf#Date', true, null, true);
}

function MsgSortByLines() 
{
	SortThreadPane('MemoryColumn', 'http://home.netscape.com/NC-rdf#Lines', 'http://home.netscape.com/NC-rdf#Date', true, null, true);
}


function MsgSortByUnread()
{
	SortThreadPane('UnreadColumn', 'http://home.netscape.com/NC-rdf#TotalUnreadMessages','http://home.netscape.com/NC-rdf#Date', true, null, true);
}

function MsgSortByOrderReceived()
{
	SortThreadPane('OrderReceivedColumn', 'http://home.netscape.com/NC-rdf#OrderReceived','http://home.netscape.com/NC-rdf#Date', true, null, true);
}

function MsgSortByRead()
{
	SortThreadPane('UnreadButtonColumn', 'http://home.netscape.com/NC-rdf#IsUnread','http://home.netscape.com/NC-rdf#Date', true, null,true);
}

function MsgSortByTotal()
{
	SortThreadPane('TotalColumn', 'http://home.netscape.com/NC-rdf#TotalMessages', 'http://home.netscape.com/NC-rdf#Date', true, null, true);
}

function MsgSortByThread()
{
	ChangeThreadView()
}

function ChangeThreadView()
{
   var folder = GetSelectedFolder();

	var threadColumn = document.getElementById('ThreadColumnHeader');
	if(threadColumn)
	{
		var currentView = threadColumn.getAttribute('currentView');
		if(currentView== 'threaded')
		{
			ShowThreads(false);
			if(folder)
				folder.setAttribute('threaded', "false");
			SetTemplateTreeItemOpen(false);
		}
		else if(currentView == 'unthreaded')
		{
			ShowThreads(true);
			if(folder)
				folder.setAttribute('threaded', "true");
		}
		RefreshThreadTreeView();
	}
}

function IsSpecialFolderSelected(folderName)
{
	var selectedFolder = GetThreadTreeFolder();
	var id = selectedFolder.getAttribute('ref');
	var folderResource = RDF.GetResource(id);
	if(!folderResource)
		return false;

    var db = GetFolderDatasource();

	var property =
        RDF.GetResource('http://home.netscape.com/NC-rdf#SpecialFolder');
    if (!property) return false;
	var result = db.GetTarget(folderResource, property , true);
    if (!result) return false;
	result = result.QueryInterface(Components.interfaces.nsIRDFLiteral);
    if (!result) return false;
	if(result.Value == folderName)
		return true;

	return false;
}

//Called when selection changes in the thread pane.
function ThreadPaneSelectionChange()
{
	var collapsed = IsThreadAndMessagePaneSplitterCollapsed();

	if(!collapsed)
	{
		LoadSelectionIntoMessagePane();
		
	}

	var tree = GetThreadTree();
	var selectedMessages = tree.selectedItems;
	var numSelected = selectedMessages.length;
	//If the current selected is 1 or 0 then we know that a change has taken place that might
	//cause us to send out threadTree update notifications. We also care about this if the previous
	//numSelected was 0 or 1 because we might be involved in something like a SelectAll where we won't
	//get notified about the change from 0 to 1.
	if(numSelected == 0 || numSelected == 1 || gOldNumSelected == 0 || gOldNumSelected == 1)
	{
		document.commandDispatcher.updateCommands('threadTree-select');
	}
	//Store the current number selected.
	gOldNumSelected = numSelected;
}

function GetThreadTree()
{
    if (gThreadTree) return gThreadTree;
	var threadTree = document.getElementById('threadTree');
    gThreadTree = threadTree;
	return threadTree;
}

function GetThreadTreeFolder()
{
  var tree = GetThreadTree();
  return tree;
}

