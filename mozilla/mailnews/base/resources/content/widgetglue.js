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
 *
 * Contributor(s):
 *   H�kan Waara (hwaara@chello.se)
 *   Jan Varga (varga@utcru.sk)
 */

/*
 * widget-specific wrapper glue. There should be one function for every
 * widget/menu item, which gets some context (like the current selection)
 * and then calls a function/command in commandglue
 */
 
//The eventual goal is for this file to go away and its contents to be brought into
//mailWindowOverlay.js.  This is currently being done.

//NOTE: gMessengerBundle must be defined and set or this Overlay won't work

/* not used
function ConvertDOMListToResourceArray(nodeList)
{
    var result = Components.classes["@mozilla.org/supports-array;1"].createInstance(Components.interfaces.nsISupportsArray);

    for (var i=0; i<nodeList.length; i++) {
        result.AppendElement(nodeList[i].resource);
    }

    return result;
}
*/

function GetSelectedFolderURI()
{
    var uri = null;
    var folderOutliner = GetFolderOutliner();
    var startRange = {value: 0};
    var endRange = {value: 0};
    folderOutliner.outlinerBoxObject.selection.getRangeAt(0, startRange, endRange);
    
    //  you can only select one folder / server to add new folder / subscribe to
    if (folderOutliner.outlinerBoxObject.selection.count == 1)
    {
        var resource = GetFolderResource(startRange.value);
        if (resource)
            uri = resource.Value;
    }
  
    return uri;
}


function MsgRenameFolder() 
{
    var preselectedURI = GetSelectedFolderURI();
    var folderOutliner = GetFolderOutliner();

    var name = GetFolderNameFromUri(preselectedURI, folderOutliner);

    dump("preselectedURI = " + preselectedURI + "\n");
    var dialog = window.openDialog(
                 "chrome://messenger/content/renameFolderDialog.xul",
                 "newFolder",
                 "chrome,titlebar,modal",
                 {preselectedURI: preselectedURI, 
                  okCallback: RenameFolder, name: name});
}

function RenameFolder(name,uri)
{
    dump("uri,name = " + uri + "," + name + "\n");
    var folderOutliner = GetFolderOutliner();
    if (folderOutliner)
    {
    if (uri && (uri != "") && name && (name != "")) {
                var selectedFolder = GetResourceFromUri(uri);
                try
                {
                    messenger.RenameFolder(GetFolderDatasource(), selectedFolder, name);
                }
                catch(e)
                {
                    throw(e); // so that the dialog does not automatically close
                    dump ("Exception : RenameFolder \n");
                }

                ClearThreadPane();
                ClearMessagePane();
                folderOutliner.outlinerBoxObject.selection.clearSelection();
        }
        else {
                dump("no name or nothing selected\n");
        }   
    }
    else {
    dump("no folder tree\n");
    }
}

function MsgEmptyTrash() 
{
    var outliner = GetFolderOutliner();
    var folderList = GetSelectedMsgFolders();

    if (folderList)
    {
        var folder = folderList[0];       

        if (folder)
        {
            var trashUri = GetSelectTrashUri(folder);
            var folderResource = folder.QueryInterface(Components.interfaces.nsIRDFResource);

            if (trashUri)
            {
                var trashElement = document.getElementById(trashUri);
                if (trashElement)
                {
                    //dump ('found trash folder\n');
                    trashElement.setAttribute('open','');
                }
                
                var trashSelected = IsSpecialFolderSelected(MSG_FOLDER_FLAG_TRASH);

                if(trashSelected)
                    outliner.outlinerBoxObject.selection.clearSelection();

                try 
                {
                    messenger.EmptyTrash(GetFolderDatasource(), folderResource);
                }
                catch(e)
                {  
                    dump ("Exception : messenger.EmptyTrash \n");
                }
                if (trashSelected)
                {
                    trashElement = document.getElementById(trashUri);
                    if (trashElement)
                        ChangeSelection(outliner, trashElement);
                }
            }
        }
    }
}

function MsgCompactFolder(isAll) 
{
    //get the selected elements
    var outliner = GetFolderOutliner();
    if (outliner)
    {
        var folderList = GetSelectedMsgFolders();
        if (folderList)
        {
            var selctedFolderUri = "";
            var isImap = false;
            if (folderList.length == 1)
            {
                selectedFolderUri = folderList[0].getAttribute('id');
                if (selectedFolderUri.indexOf("imap:") != -1)
                    isImap = true;
                else
                {
                    ClearThreadPaneSelection();
                    ClearMessagePane();
                }
            }
            var i;
            var folder;
            for(i = 0; i < folderList.length; i++)
            {
                folder = folderList[i];
                if (folder)
                {
                    folderuri = folder.getAttribute('id');
                    dump(folderuri + "\n");
                    dump("folder = " + folder.localName + "\n"); 
                    try
                    {
                      messenger.CompactFolder(GetFolderDatasource(), folder.resource, isAll);
                    }
                    catch(e)
                    {
                      dump ("Exception : messenger.CompactFolder \n");
                    }
                }
            }
            if (!isImap && selectedFolderUri && selectedFolderUri != "")
            {
                /* this doesn't work; local compact is now an async operation
                dump("selected folder = " + selectedFolderUri + "\n");
                var selectedFolder =
                    document.getElementById(selectedFolderUri);
                ChangeSelection(tree, selectedFolder);
                */
                outliner.outlinerBoxObject.selection.clearSelection();
            }
        }
    }
}

function MsgFolderProperties() 
{
    var preselectedURI = GetSelectedFolderURI();
    var folderOutliner = GetFolderOutliner();

    var name = GetFolderNameFromUri(preselectedURI, folderOutliner);

    dump("preselectedURI = " + preselectedURI + "\n");
    var windowTitle = gMessengerBundle.getString("folderProperties");
    var dialog = window.openDialog(
                    "chrome://messenger/content/imapFolderProps.xul",
                    "",
                    "chrome,centerscreen,titlebar,modal",
                    {preselectedURI:preselectedURI, title:windowTitle,
                    okCallback:FolderProperties, 
                    tabID:"", tabIndex:0, name:name});
}

function FolderProperties(name, uri)
{
}

function MsgToggleMessagePane()
{
    //OnClickThreadAndMessagePaneSplitter is based on the value before the splitter is toggled.
    OnClickThreadAndMessagePaneSplitter();
    MsgToggleSplitter("threadpane-splitter");
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


function NotifyQuitApplication()
{
    var ObserverService = Components.classes["@mozilla.org/observer-service;1"].getService();
    ObserverService = ObserverService.QueryInterface(Components.interfaces.nsIObserverService);
    if (ObserverService)
    {
        try 
        {
            ObserverService.Notify(null, "quit-application", null);
        } 
        catch (ex) 
        {
            // dump("no observer found \n");
        }
    }
}

function LastToClose()
{
    var windowManager = Components.classes['@mozilla.org/rdf/datasource;1?name=window-mediator'].getService();
    var windowManagerInterface = windowManager.QueryInterface( Components.interfaces.nsIWindowMediator);
    var enumerator = windowManagerInterface.getEnumerator(null);
    var count = 0;
    while ( enumerator.hasMoreElements() && count < 2 )
    {
        var windowToClose = enumerator.getNext();
        count++;
    }
    if (count == 1)
        return true;
    else
        return false;
}

function MsgSetFolderCharset() 
{
  var preselectedURI = GetSelectedFolderURI();
  var folderOutliner = GetFolderOutliner();
  var name = GetFolderNameFromUri(preselectedURI, folderOutliner);
  var windowTitle = gMessengerBundle.getString("folderProperties");
  var dialog = window.openDialog(
                      "chrome://messenger/content/imapFolderProps.xul",
                      "",
                      "chrome,centerscreen,titlebar,modal",
                      {preselectedURI:preselectedURI, title:windowTitle,
                      okCallback:FolderProperties, 
                      tabID:"folderCharsetTab",  tabIndex:2, name:name});
}
