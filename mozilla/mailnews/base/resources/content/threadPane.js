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

var gLastMessageUriToLoad = null;
var gThreadPaneCommandUpdater = null;

function ThreadPaneOnClick(event)
{
    // we are already handling marking as read and flagging
    // in nsMsgDBView.cpp
    // so all we need to worry about here is double clicks
    // and column header.
    //
    // we get in here for clicks on the "outlinercol" (headers)
    // and the "scrollbarbutton" (scrollbar buttons)
    // we don't want those events to cause a "double click"

    var t = event.originalTarget;

    if (t.localName == "outlinercol") {
       HandleColumnClick(t.id);
    }
    else if (event.detail == 2 && t.localName == "outlinerbody") {
       var row = new Object;
       var colID = new Object;
       var childElt = new Object;

       var outliner = GetThreadOutliner();
       // figure out what cell the click was in
       outliner.boxObject.QueryInterface(Components.interfaces.nsIOutlinerBoxObject).getCellAt(event.clientX, event.clientY, row, colID, childElt);

       // if the cell is in a "cycler" column
       // don't open the message in a new window
       var col = document.getElementById(colID.value);
       if (col && col.getAttribute("cycler") != "true") {
         ThreadPaneDoubleClick();
       }
    }
}

function nsMsgDBViewCommandUpdater()
{}

nsMsgDBViewCommandUpdater.prototype = 
{
  updateCommandStatus : function()
    {
      // the back end is smart and is only telling us to update command status
      // when the # of items in the selection has actually changed.
		  document.commandDispatcher.updateCommands('mail-toolbar');
    },

  displayMessageChanged : function(aFolder, aSubject)
  {
    setTitleFromFolder(aFolder, aSubject);
    gHaveLoadedMessage = true;
  },

  QueryInterface : function(iid)
   {
     if(iid.equals(Components.interfaces.nsIMsgDBViewCommandUpdater))
	    return this;
	  
     throw Components.results.NS_NOINTERFACE;
     return null;
    }
}

function HandleColumnClick(columnID)
{
  var sortType = ConvertColumnIDToSortType(columnID);

  // if sortType is 0, this is an unsupported sort type
  // return, since we can't sort by that column.
  if (sortType == 0) {
    return;
  }

  var dbview = GetDBView();
  if (dbview.sortType == sortType) {
    MsgReverseSortThreadPane();
  }
  else {
    MsgSortThreadPane(sortType);
  }
}

function PersistViewAttributesOnFolder()
{
  var folder = GetSelectedFolder();

  if (folder) {
    folder.setAttribute("viewType", gDBView.viewType);
    folder.setAttribute("viewFlags", gDBView.viewFlags);
    folder.setAttribute("sortType", gDBView.sortType);
    folder.setAttribute("sortOrder", gDBView.sortOrder);
  }
}

function MsgComposeDraftMessage()
{
    var loadedFolder = GetLoadedMsgFolder();
    var messageArray = GetSelectedMessages();

    ComposeMessage(msgComposeType.Draft, msgComposeFormat.Default, loadedFolder, messageArray);
}

function ThreadPaneDoubleClick()
{
	if (IsSpecialFolderSelected(MSG_FOLDER_FLAG_DRAFTS)) {
		MsgComposeDraftMessage();
	}
	else if(IsSpecialFolderSelected(MSG_FOLDER_FLAG_TEMPLATES)) {
		var loadedFolder = GetLoadedMsgFolder();
		var messageArray = GetSelectedMessages();
		ComposeMessage(msgComposeType.Template, msgComposeFormat.Default, loadedFolder, messageArray);
	}
	else {
        MsgOpenSelectedMessages();
	}
}

function ThreadPaneKeyPress(event)
{
    if (event.keyCode == 13)
      ThreadPaneDoubleClick();
}

function MsgSortByDate()
{
    MsgSortThreadPane(nsMsgViewSortType.byDate);
}

function MsgSortBySenderOrRecipient()
{
    if (IsSpecialFolderSelected(MSG_FOLDER_FLAG_SENTMAIL | MSG_FOLDER_FLAG_DRAFTS | MSG_FOLDER_FLAG_QUEUE)) {
      MsgSortThreadPane(nsMsgViewSortType.byRecipient);
    }
    else {
      MsgSortThreadPane(nsMsgViewSortType.byAuthor);
    }
}

function MsgSortByStatus()
{
    MsgSortThreadPane(nsMsgViewSortType.byStatus);
}

function MsgSortBySubject()
{
    MsgSortThreadPane(nsMsgViewSortType.bySubject);
}

function MsgSortByLocation()
{
    MsgSortThreadPane(nsMsgViewSortType.byLocation);
}


function MsgSortByFlagged() 
{
    MsgSortThreadPane(nsMsgViewSortType.byFlagged);
}

function MsgSortByPriority()
{
    MsgSortThreadPane(nsMsgViewSortType.byPriority);
}

function MsgSortBySize() 
{
    MsgSortThreadPane(nsMsgViewSortType.bySize);
}

function MsgSortByUnread()
{
    MsgSortThreadPane(nsMsgViewSortType.byUnread);
}

function MsgSortByOrderReceived()
{
    MsgSortThreadPane(nsMsgViewSortType.byId);
}

function MsgSortByTotal()
{
    dump("XXX fix MsgSortByTotal\n");
    //MsgSortThreadPane(nsMsgViewSortType.byTotal);
}

function MsgSortByThread()
{
    MsgSortThreadPane(nsMsgViewSortType.byThread);
}

function MsgSortThreadPane(sortType)
{
    var dbview = GetDBView();
    dbview.sort(sortType, nsMsgViewSortOrder.ascending);
    UpdateSortIndicators(sortType, nsMsgViewSortOrder.ascending);
    PersistViewAttributesOnFolder();
}

function MsgReverseSortThreadPane()
{
  var dbview = GetDBView();
  if (dbview.sortOrder == nsMsgViewSortOrder.ascending) {
    MsgSortDescending();
  }
  else {
    MsgSortAscending();
  }
}

function MsgSortAscending()
{
  var dbview = GetDBView();
  dbview.sort(dbview.sortType, nsMsgViewSortOrder.ascending);
  UpdateSortIndicators(dbview.sortType, nsMsgViewSortOrder.ascending);
  PersistViewAttributesOnFolder();
}

function MsgSortDescending()
{
  var dbview = GetDBView();
  dbview.sort(dbview.sortType, nsMsgViewSortOrder.descending);
  UpdateSortIndicators(dbview.sortType, nsMsgViewSortOrder.descending);
  PersistViewAttributesOnFolder();
}

function UpdateSortIndicators(sortType, sortOrder)
{
  var colID = ConvertSortTypeToColumnID(sortType);
  var sortedColumn;

  // set the sort indicator on the column we are sorted by
  if (colID) {
    sortedColumn = document.getElementById(colID);
    if (sortedColumn) {
      if (sortOrder == nsMsgViewSortOrder.ascending) {
        sortedColumn.setAttribute("sortDirection","ascending");
      }
      else {
        sortedColumn.setAttribute("sortDirection","descending");
      }
    }
  }

  // remove the sort indicator from all the columns
  // except the one we are sorted by
  var currCol = GetThreadOutliner().firstChild;
  while (currCol) {
    while (currCol && currCol.localName != "outlinercol")
      currCol = currCol.nextSibling;
    if (currCol && (currCol != sortedColumn)) {
      currCol.removeAttribute("sortDirection");
    }
    if (currCol) 
      currCol = currCol.nextSibling;
  }
}

function IsSpecialFolderSelected(flags)
{
    var selectedFolder = GetThreadPaneFolder();
    if (!selectedFolder || ((selectedFolder.flags & flags) == 0)) {
        return false;
    }
    else {
        return true;
    }
}

function GetThreadOutliner()
{
  if (gThreadOutliner) return gThreadOutliner;
	gThreadOutliner = document.getElementById('threadOutliner');
	return gThreadOutliner;
}

function GetThreadPaneFolder()
{
  try {
    return gDBView.msgFolder;
  }
  catch (ex) {
    return null;
  }
}

function EnsureRowInThreadOutlinerIsVisible(index)
{
  var outliner = GetThreadOutliner();
  outliner.boxObject.QueryInterface(Components.interfaces.nsIOutlinerBoxObject).ensureRowIsVisible(index); 
}
