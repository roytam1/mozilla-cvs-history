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
 * Command-specific code. This stuff should be called by the widgets
 */

//The eventual goal is for this file to go away and for the functions to either be brought into
//mailCommands.js or into 3pane specific code.

var gFolderJustSwitched = false;
var gBeforeFolderLoadTime;
var gRDFNamespace = "http://www.w3.org/1999/02/22-rdf-syntax-ns#";

function OpenURL(url)
{
  //dump("\n\nOpenURL from XUL\n\n\n");
  messenger.SetWindow(window, msgWindow);
  messenger.OpenURL(url);
}

function GetMsgFolderFromNode(folderNode)
{
	var folderURI = folderNode.getAttribute("id");
	return GetMsgFolderFromURI(folderURI);
}

function GetMsgFolderFromURI(folderURI)
{
	var folderResource = RDF.GetResource(folderURI);
	if(folderResource)
	{
		var msgFolder = folderResource.QueryInterface(Components.interfaces.nsIMsgFolder);
		return msgFolder;
	}

	return null;
}

function GetServer(uri)
{
    if (!uri) return null;
    try {
        var folder = GetMsgFolderFromUri(uri);
        return folder.server;
    }
    catch (ex) {
        dump("GetServer("+uri+") failed, ex="+ex+"\n");
    }
    return null;
}

function LoadMessageByUri(uri)
{  
    dump("XXX LoadMessageByUri " + uri + " vs " + gCurrentDisplayedMessage + "\n");
	if(uri != gCurrentDisplayedMessage)
	{
        dump("fix this, get the nsIMsgDBHdr and the nsIMsgFolder from the uri...\n");
/*
		var resource = RDF.GetResource(uri);
		var message = resource.QueryInterface(Components.interfaces.nsIMessage); 
		if (message)
			setTitleFromFolder(message.msgFolder, message.mimef2DecodedSubject);

		var nsIMsgFolder = Components.interfaces.nsIMsgFolder;
		if (message.msgFolder.server.downloadOnBiff)
			message.msgFolder.biffState = nsIMsgFolder.nsMsgBiffState_NoMail;
*/

		gCurrentDisplayedMessage = uri;
		gHaveLoadedMessage = true;
		OpenURL(uri);
	}

}

function ChangeFolderByDOMNode(folderNode)
{
  var uri = folderNode.getAttribute('id');
  dump(uri + "\n");
  if (!uri) return;

  var sortType = folderNode.getAttribute('sortType');
  var sortOrder = folderNode.getAttribute('sortOrder');
  var viewFlags = folderNode.getAttribute('viewFlags');
  var viewType = folderNode.getAttribute('viewType');

  ChangeFolderByURI(uri, viewType, viewFlags, sortType, sortOrder);
}

function setTitleFromFolder(msgfolder, subject)
{
    if (!msgfolder) return;

    var title;
    var server = msgfolder.server;

    if (null != subject)
      title = subject+" - ";
    else
      title = "";

    if (msgfolder.isServer) {
            // <hostname>
            title += server.hostName;
    }
    else {
        var middle;
        var end;
        if (server.type == "nntp") {
            // <folder> on <hostname>
            middle = Bundle.GetStringFromName("titleNewsPreHost");
            end = server.hostName;
        } else {
            var identity;
            try {
                var identities = accountManager.GetIdentitiesForServer(server);

                identity = identities.QueryElementAt(0, Components.interfaces.nsIMsgIdentity);
                // <folder> for <email>
                middle = Bundle.GetStringFromName("titleMailPreHost");
                end = identity.email;
            } catch (ex) {
            }
            
        }

        title += msgfolder.prettyName;
        if (middle) title += " " + middle;
        if (end) title += " " + end;
    }

    title += " - " + BrandBundle.GetStringFromName("brandShortName");
    window.title = title;
}

function ChangeFolderByURI(uri, viewType, viewFlags, sortType, sortOrder)
{
  dump('In ChangeFolderByURI uri = ' + uri + "\n");
  if (uri == gCurrentLoadingFolderURI)
    return;
  var resource = RDF.GetResource(uri);
  var msgfolder =
      resource.QueryInterface(Components.interfaces.nsIMsgFolder);

  try {
      setTitleFromFolder(msgfolder, null);
  } catch (ex) {
      dump("error setting title: " + ex + "\n");
  }

  
  //if it's a server, clear the threadpane and don't bother trying to load.
  if(msgfolder.isServer) {
    ClearThreadPane();

    // Load AccountCentral page here.
    ShowAccountCentral(); 
	return;
  }

  // If the user clicks on msgfolder, time to display thread pane and message pane.
  // Hide AccountCentral page
  if (gAccountCentralLoaded)
  {
      HideAccountCentral();
  }
   
  if (showPerformance) { 
    gBeforeFolderLoadTime = new Date();
  }

  gCurrentLoadingFolderURI = uri;
  gNextMessageAfterDelete = null; // forget what message to select, if any

  if(msgfolder.manyHeadersToDownload())
  {
	try
	{
		SetBusyCursor(window, true);
		gCurrentFolderToReroot = uri;
		gCurrentLoadingFolderViewFlags = viewFlags;
		gCurrentLoadingFolderViewType = viewType;
		gCurrentLoadingFolderSortType = sortType;
		gCurrentLoadingFolderSortOrder = sortOrder;
		msgfolder.startFolderLoading();
		msgfolder.updateFolder(msgWindow);
	}
	catch(ex)
	{
        dump("Error loading with many headers to download: " + ex + "\n");
	}
  }
  else
  {
    SetBusyCursor(window, true);
	gCurrentFolderToReroot = "";
	gCurrentLoadingFolderViewFlags = 0;  // is this correct?
	gCurrentLoadingFolderSortType = 0;  // is this correct?
	gCurrentLoadingFolderSortOrder = 0;  // is this correct?
    gCurrentLoadingFolderViewType = 0;  // is this correct?
	RerootFolder(uri, msgfolder, viewType, viewFlags, sortType, sortOrder);

	//Need to do this after rerooting folder.  Otherwise possibility of receiving folder loaded
	//notification before folder has actually changed.
	msgfolder.updateFolder(msgWindow);
  }

  document.commandDispatcher.updateCommands('mail-toolbar');
}

function isNewsURI(uri)
{
    if (!uri || uri[0] != 'n') {
        return false;
    }
    else {
        return ((uri.substring(0,6) == "news:/") || (uri.substring(0,14) == "news_message:/"));
    }
}

function RerootFolder(uri, newFolder, viewType, viewFlags, sortType, sortOrder)
{
  dump('In reroot folder\n');
	
  // workaround for #39655
  gFolderJustSwitched = true;

  ClearThreadTreeSelection();

  //Set the window's new open folder.
  msgWindow.openFolder = newFolder;

  SetViewFlags(viewFlags);

  //Clear the new messages of the old folder
  dump("some work needed here\n");
/*
  var oldFolderURI = folder.getAttribute("ref");
  if(oldFolderURI && (oldFolderURI != "null") && (oldFolderURI !=""))
  {
   var oldFolder = GetMsgFolderFromURI(oldFolderURI);
   if(oldFolder)
   {
       if (oldFolder.hasNewMessages)
           oldFolder.clearNewMessages();
   }
  }
*/
  //the new folder being selected should have its biff state get cleared.   
  if(newFolder)
  {
    newFolder.biffState = 
          Components.interfaces.nsIMsgFolder.nsMsgBiffState_NoMail;
  }

  //Clear out the thread pane so that we can sort it with the new sort id without taking any time.
  // folder.setAttribute('ref', "");
   if (isNewsURI(uri))
       SetNewsFolderColumns(true);
   else
       SetNewsFolderColumns(false);

  // null this out, so we don't try sort.
  gDBView = null;
  SetSentFolderColumns(IsSpecialFolder(newFolder, [ "Sent", "Drafts", "Unsent Messages" ]));

  // now create the db view, which will sort it.

  CreateDBView(newFolder, viewType, viewFlags, sortType, sortOrder);
  // that should have initialized gDBView, now re-root the thread pane
  var outlinerView = gDBView.QueryInterface(Components.interfaces.nsIOutlinerView);
  if (outlinerView)
  {     
    var outliner = GetThreadOutliner();
    outliner.boxObject.QueryInterface(Components.interfaces.nsIOutlinerBoxObject).view = outlinerView; 
    dump('set outliner view\n');
  }

  SetUpToolbarButtons(uri);

  UpdateStatusMessageCounts(newFolder);
}

function SwitchView(command)
{
  gDBView = null; // close existing view.
  var viewFlags = gCurViewFlags;
  switch(command)
  {
    case "cmd_viewUnreadMsgs":

      viewFlags = viewFlags | nsMsgViewFlagsType.kUnreadOnly;
      CreateDBView(msgWindow.openFolder, nsMsgViewType.eShowAllThreads, viewFlags, 
            nsMsgViewSortType.byThread, nsMsgViewSortOrder.ascending);
    break;
    case "cmd_viewAllMsgs":
      viewFlags = viewFlags & ~nsMsgViewFlagsType.kUnreadOnly;
      CreateDBView(msgWindow.openFolder, nsMsgViewType.eShowAllThreads, viewFlags, 
            nsMsgViewSortType.byThread, nsMsgViewSortOrder.ascending);
    break;
    case "cmd_viewThreadsWithUnread":
      CreateDBView(msgWindow.openFolder, nsMsgViewType.eShowThreadsWithUnread, nsMsgViewFlagsType.kThreadedDisplay, 
            nsMsgViewSortType.byThread, nsMsgViewSortOrder.ascending);

    break;
    case "cmd_viewWatchedThreadsWithUnread":
      CreateDBView(msgWindow.openFolder, nsMsgViewType.eShowWatchedThreadsWithUnread, nsMsgViewFlagsType.kThreadedDisplay, 
            nsMsgViewSortType.byThread, nsMsgViewSortOrder.ascending);
   break;
    case "cmd_viewKilledThreads":
    break;
  }

  // that should have initialized gDBView, now re-root the thread pane
  var outlinerView = gDBView.QueryInterface(Components.interfaces.nsIOutlinerView);
  if (outlinerView)
  {     
    var outliner = GetThreadOutliner();
    outliner.boxObject.QueryInterface(Components.interfaces.nsIOutlinerBoxObject).view = outlinerView; 
    dump('set outliner view\n');
  }
}

function SetSentFolderColumns(isSentFolder)
{
	var senderColumn = document.getElementById("senderCol");

	if(isSentFolder)
	{
		senderColumn.setAttribute("value", Bundle.GetStringFromName("recipientColumnHeader"));
	}
	else
	{
		senderColumn.setAttribute("value", Bundle.GetStringFromName("senderColumnHeader"));
	}
}

function SetNewsFolderColumns(isNewsFolder)
{
  dump("fix me, I need to show lines or size depending on if the folder is news or not\n");
/* 
	var sizeColumn = document.getElementById("SizeColumnHeader");
  var sizeColumnTemplate = document.getElementById("SizeColumnTemplate");

  var memoryColumnHeader = document.getElementById("MemoryColumn");

  if (isNewsFolder)
  { 
     sizeColumn.setAttribute("value",Bundle.GetStringFromName("linesColumnHeader"));
     sizeColumn.setAttribute("onclick", "return top.MsgSortByLines();");
     sizeColumnTemplate.setAttribute("value", "rdf:http://home.netscape.com/NC-rdf#Lines");
     memoryColumnHeader.setAttribute("resource","http://home.netscape.com/NC-rdf#Lines");
  }
  else
  {
     sizeColumn.setAttribute("value", Bundle.GetStringFromName("sizeColumnHeader"));
     sizeColumn.setAttribute("onclick", "return top.MsgSortBySize();");
     sizeColumnTemplate.setAttribute("value", "rdf:http://home.netscape.com/NC-rdf#Size");
     memoryColumnHeader.setAttribute("resource","http://home.netscape.com/NC-rdf#Size");
  }
*/
} 
        
        

function UpdateStatusMessageCounts(folder)
{
	var unreadElement = GetUnreadCountElement();
	var totalElement = GetTotalCountElement();
	if(folder && unreadElement && totalElement)
	{
		var numUnread =
            Bundle.formatStringFromName("unreadMsgStatus",
                                        [ folder.getNumUnread(false)], 1);
		var numTotal =
            Bundle.formatStringFromName("totalMsgStatus",
                                        [folder.getTotalMessages(false)], 1);

		unreadElement.setAttribute("value", numUnread);
		totalElement.setAttribute("value", numTotal);

	}

}

function ConvertColumnIDToSortType(columnID)
{
  var sortKey;

  switch (columnID) {
    case "dateCol":
      sortKey = nsMsgViewSortType.byDate;
      break;
    case "senderCol":
      sortKey = nsMsgViewSortType.byAuthor;
      break;
    case "subjectCol":
      sortKey = nsMsgViewSortType.bySubject;
      break;
    case "unreadButtonColHeader":
      sortKey = nsMsgViewSortType.byUnread;
      break;
    case "statusCol":
      sortKey = nsMsgViewSortType.byStatus;
      break;
    case "sizeCol":
      sortKey = nsMsgViewSortType.bySize;
      break;
    case "priorityCol":
      sortKey = nsMsgViewSortType.byPriority;
      break;
    case "flaggedCol":
      sortKey = nsMsgViewSortType.byFlagged;
      break;
    case "threadCol":
      sortKey = nsMsgViewSortType.byThread;
      break;
    default:
      dump("unsupported sort: " + columnID + "\n");
      sortKey = 0;
      break;
  }
  return sortKey;
}

function ConvertSortTypeToColumnID(sortKey)
{
  var columnID;

  // hack to turn this into an integer, if it was a string;
  sortKey = sortKey - 0;

  switch (sortKey) {
    case nsMsgViewSortType.byDate:
      columnID = "dateCol";
      break;
    case nsMsgViewSortType.byAuthor:
      columnID = "senderCol";
      break;
    case nsMsgViewSortType.bySubject:
      columnID = "subjectCol";
      break;
    case nsMsgViewSortType.byUnread:
      columnID = "unreadButtonColHeader";
      break;
    case nsMsgViewSortType.byStatus:
      columnID = "statusCol";
      break;
    case nsMsgViewSortType.bySize:
      columnID = "sizeCol";
      break;
    case nsMsgViewSortType.byPriority:
      columnID = "priorityCol";
      break;
    case nsMsgViewSortType.byFlagged:
      columnID = "flaggedCol";
      break;
    case nsMsgViewSortType.byThread:
      columnID = "threadCol";
      break;
    default:
      dump("unsupported sort: " + sortKey + "\n");
      columnID = null;
      break;
  }
  return columnID;
}

var nsMsgViewSortType = Components.interfaces.nsMsgViewSortType;
var nsMsgViewSortOrder = Components.interfaces.nsMsgViewSortOrder;
var nsMsgViewFlagsType = Components.interfaces.nsMsgViewFlagsType;
var nsMsgViewCommandType = Components.interfaces.nsMsgViewCommandType;
var nsMsgViewType = Components.interfaces.nsMsgViewType;
var nsMsgNavigationType = Components.interfaces.nsMsgNavigationType;

var gDBView = null;
var gCurViewFlags;

// CreateDBView is called when we have a thread pane. CreateBareDBView is called when there is no
// outliner associated with the view. CreateDBView will call into CreateBareDBView...

function CreateBareDBView(msgFolder, viewType, viewFlags, sortType, sortOrder)
{
  dump("XXX CreateDBView(" + msgFolder + "," + viewType + "," + viewFlags + "," + sortType + "," + sortOrder + ")\n");

  var dbviewContractId = "@mozilla.org/messenger/msgdbview;1?type=";

  switch (viewType) {
      case nsMsgViewType.eShowThreadsWithUnread:
          dbviewContractId += "threadswithunread";
          break;
      case nsMsgViewType.eShowWatchedThreadsWithUnread:
          dbviewContractId += "watchedthreadswithunread";
          break;
      case nsMsgViewType.eShowAllThreads:
      default:
          dbviewContractId += "threaded";
          break;
  }

  dump("XXX creating " + dbviewContractId + " with: " + viewType + "," + sortType + "," + sortOrder + "\n");
  gDBView = Components.classes[dbviewContractId].createInstance(Components.interfaces.nsIMsgDBView);

  var isNews = isNewsURI(msgFolder.URI);
  if (!viewFlags) {
    if (isNews) {
      // news defaults to threaded mode
      viewFlags = nsMsgViewFlagsType.kThreadedDisplay;
    }
    else {
      viewFlags &= nsMsgViewFlagsType.kThreadedDisplay;
    }
  }

  if (!sortType) {
    if (isNews) { 
      // news defaults to threaded mode
      sortType = nsMsgViewSortType.byThread;
    }
    else {
      sortType = nsMsgViewSortType.byDate;
    }
  }

  if (!sortOrder) {
    sortOrder = nsMsgViewSortOrder.ascending;
  }

  gCurViewFlags = viewFlags;
  var count = new Object;
  if (!gThreadPaneCommandUpdater)
    gThreadPaneCommandUpdater = new nsMsgDBViewCommandUpdater();

  gDBView.init(messenger, msgWindow, gThreadPaneCommandUpdater);
  gDBView.open(msgFolder, sortType, sortOrder, viewFlags, count);
}

function CreateDBView(msgFolder, viewType, viewFlags, sortType, sortOrder)
{
  // call the inner create method
  CreateBareDBView(msgFolder, viewType, viewFlags, sortType, sortOrder);

  // now do outliner specific work

  // based on the collapsed state of the thread pane/message pane splitter,
  // supress message display if appropriate.
  var collapsed = IsThreadAndMessagePaneSplitterCollapsed();
  if (collapsed)
    gDBView.supressMsgDisplay = true;
  else
    gDBView.supressMsgDisplay = false;
    
  var colID = ConvertSortTypeToColumnID(sortType);
  if (colID) {
    var column = document.getElementById(colID);
    gDBView.sortedColumn = column;
  }

  ShowAppropriateColumns();
  PersistViewAttributesOnFolder();
}

function ShowAppropriateColumns()
{
    if (gDBView.sortType == nsMsgViewSortType.byThread) {
      // don't hide them when sorted by thread
      SetHiddenAttributeOnThreadOnlyColumns("");
    }
    else {
      // hide them when not sorted by thread
      SetHiddenAttributeOnThreadOnlyColumns("true");
    }
}

function SetViewFlags(viewFlags)
{
    if (!gDBView) return;
    dump("XXX SetViewFlags(" + viewFlags + ")\n");
    gDBView.viewFlags = viewFlags;
}

//------------------------------------------------------------
// Sets the column header sort icon based on the requested 
// column and direction.
// 
// Notes:
// (1) This function relies on the first part of the 
//     <treecell id> matching the <treecol id>.  The treecell
//     id must have a "Header" suffix.
// (2) By changing the "sortDirection" attribute, a different
//     CSS style will be used, thus changing the icon based on
//     the "sortDirection" parameter.
//------------------------------------------------------------
function UpdateSortIndicator(column,sortDirection)
{
  // this is obsolete
}

function UpdateSortMenu(currentSortColumn)
{
/*
	UpdateSortMenuitem(currentSortColumn, "sortByDateMenuitem", "DateColumn");
	UpdateSortMenuitem(currentSortColumn, "sortByFlagMenuitem", "FlaggedButtonColumn");
	UpdateSortMenuitem(currentSortColumn, "sortByOrderReceivedMenuitem", "OrderReceivedColumn");
	UpdateSortMenuitem(currentSortColumn, "sortByPriorityMenuitem", "PriorityColumn");
	UpdateSortMenuitem(currentSortColumn, "sortBySenderMenuitem", "AuthorColumn");
	UpdateSortMenuitem(currentSortColumn, "sortBySizeMenuitem", "MemoryColumn");
	UpdateSortMenuitem(currentSortColumn, "sortByStatusMenuitem", "StatusColumn");
	UpdateSortMenuitem(currentSortColumn, "sortBySubjectMenuitem", "SubjectColumn");
	UpdateSortMenuitem(currentSortColumn, "sortByUnreadMenuitem", "UnreadButtonColumn");
*/
}

function UpdateSortMenuitem(currentSortColumnID, menuItemID, columnID)
{
	var menuItem = document.getElementById(menuItemID);
	if(menuItem)
	{
		menuItem.setAttribute("checked", currentSortColumnID == columnID);
	}
}

function SortFolderPane(column, sortKey)
{
	var node = FindInSidebar(window, column);
	if(!node)
	{
		dump('Couldnt find sort column\n');
		return false;
	}
	SortColumn(node, sortKey, null, null);
	//Remove the sortActive attribute because we don't want this pane to have any
	//sort styles.
	node.setAttribute("sortActive", "false");
	return true;
}

function SortColumn(node, sortKey, secondarySortKey, direction)
{
	//dump('In SortColumn\n');
	var xulSortService = Components.classes["@mozilla.org/rdf/xul-sort-service;1"].getService();

	if (xulSortService)
	{
		xulSortService = xulSortService.QueryInterface(Components.interfaces.nsIXULSortService);
		if (xulSortService)
		{
			// sort!!!
			var sortDirection;
			if(direction)
				sortDirection = direction;
			else
			{
				var currentDirection = node.getAttribute('sortDirection');
				if (currentDirection == "ascending")
						sortDirection = "descending";
				else if (currentDirection == "descending")
						sortDirection = "ascending";
				else    sortDirection = "ascending";
			}

			try
			{
				if(secondarySortKey)
					node.setAttribute('resource2', secondarySortKey);
				xulSortService.Sort(node, sortKey, sortDirection);
			}
			catch(e)
			{
                		//dump("Sort failed: " + e + "\n");
			}
		}
	}

}

function GetSelectedFolderResource()
{
	var folderTree = GetFolderTree();
	var selectedFolderList = folderTree.selectedItems;
	var selectedFolder = selectedFolderList[0];
	var uri = selectedFolder.getAttribute('id');


	var folderResource = RDF.GetResource(uri);
	return folderResource;

}



//Called when the splitter in between the thread and message panes is clicked.
function OnClickThreadAndMessagePaneSplitter()
{
  var collapsed = IsThreadAndMessagePaneSplitterCollapsed();
  // collapsed is the previous state, so we must be expanding
  // the splitter if collapsed is true
  if (gDBView)
  {
    if (collapsed)
      gDBView.supressMsgDisplay = false;
    else
      gDBView.supressMsgDisplay = true;
  }

/*
	dump("We are in OnClickThreadAndMessagePaneSplitter()\n");
	var collapsed = IsThreadAndMessagePaneSplitterCollapsed();
	//collapsed is the previous state so we know we are opening.
	if(collapsed)
  {
		LoadSelectionIntoMessagePane();	
    setTimeout("PositionThreadPane();",0);
   }
*/                
}

function PositionThreadPane()
{
    dump("need for cross folder navigation. fix me: PositionThreadPane()\n");
/*
       var tree = GetThreadTree();
  
       var selArray = tree.selectedItems;
           
       if ( selArray && (selArray.length > 0))
       try { 
       tree.ensureElementIsVisible(selArray[0]);
       }
       catch(e) { }
*/
}

function FolderPaneSelectionChange()
{
	var tree = GetFolderTree();
	if(tree)
	{
		var selArray = tree.selectedItems;
		if ( selArray && (selArray.length == 1) )
    {
    	ChangeFolderByDOMNode(selArray[0]);
    }
		else
		{
			ClearThreadPane();
		}
	}

  if (!gAccountCentralLoaded)
    ClearMessagePane();

}

function ClearThreadPane()
{
}

function OpenFolderTreeToFolder(folderURI)
{
	var tree = GetFolderTree();
	return OpenToFolder(tree, folderURI);

}

function OpenToFolder(item, folderURI)
{

	if(item.nodeType != Node.ELEMENT_NODE)
		return null;

	var uri = item.getAttribute('id');
	dump(uri);
	dump('\n');
	if(uri == folderURI)
	{
		dump('found folder: ' + uri);
		dump('\n');
		return item;
	}

	var children = item.childNodes;
	var length = children.length;
	var i;
	dump('folder ' + uri);
	dump('has ' + length);
	dump('children\n');
	for(i = 0; i < length; i++)
	{
		var child = children[i];
		var folder = OpenToFolder(child, folderURI);
		if(folder)
		{
			child.setAttribute('open', 'true');
			return folder;
		}
	}
	return null;
}


function IsSpecialFolder(msgFolder, specialFolderNames)
{
	var db = GetFolderDatasource();
	var folderResource = msgFolder.QueryInterface(Components.interfaces.nsIRDFResource);
	if(folderResource)
	{
		var property =
			RDF.GetResource('http://home.netscape.com/NC-rdf#SpecialFolder');
		if (!property) return false;
		var result = db.GetTarget(folderResource, property , true);
		if (!result) return false;
		result = result.QueryInterface(Components.interfaces.nsIRDFLiteral);
		if (!result) return false;

		//dump("We are looking for " + specialFolderNames + "\n");
		//dump("special folder name = " + result.Value + "\n");
        
        var count = specialFolderNames.length;
        var i;
        for (i = 0; i < count; i++) {
            if(result.Value == specialFolderNames[i])
                return true;
        }
	}

	return false;
}

function SelectNextMessage(nextMessage)
{
/*
	var tree = GetThreadTree();
	ChangeSelection(tree, nextMessage);
*/
}

function GetSelectTrashUri(folder)
{
    if (!folder) return null;
    var uri = folder.getAttribute('id');
    dump (uri + "\n");
    var resource = RDF.GetResource(uri);
    var msgFolder =
        resource.QueryInterface(Components.interfaces.nsIMsgFolder);
    if (msgFolder)
    {
        dump("GetSelectTrashUri" + "\n");
        var rootFolder = msgFolder.rootFolder;
        var numFolder;
        var out = new Object();
        var trashFolder = rootFolder.getFoldersWithFlag(0x0100, 1, out); 
        numFolder = out.value;
        dump (numFolder + "\n");
        if (trashFolder)
        {
            dump (trashFolder.URI + "\n");
            return trashFolder.URI;
        }
    }
    return null;
}

function Undo()
{
    messenger.Undo(msgWindow);
}

function Redo()
{
    messenger.Redo(msgWindow);
}

function MsgToggleWorkOffline()
{
  var ioService = nsJSComponentManager.getServiceByID("{9ac9e770-18bc-11d3-9337-00104ba0fd40}", "nsIIOService");
  var broadcaster = document.getElementById("Communicator:WorkMode");
  // this is just code for my testing purposes, and doesn't have the proper UI, as in the offline spec.
  // we could use the account manager, or add a new service, the offline manager.
  // what the heck, might as well bite the bullet and add a new service.
  offlineManager = Components.classes["@mozilla.org/messenger/offline-manager;1"].getService(Components.interfaces.nsIMsgOfflineManager);
  if (ioService.offline)
  {
    ioService.offline = false;
    // really want to use a progress window here
    offlineManager.goOnline(true, true, msgWindow);
    // if we were offline, then we're going online and should playback offline operations
  }
  else // we were online, so we're going offline and should download everything for offline use
  {
    // should use progress window here. params are:
    // download news, download mail, send unsent messages, go offline when done, msg window
    offlineManager.synchronizeForOffline(true, true, true, true, msgWindow);
  } 
}
