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
 * Contributors: timeless
 *               slucy@objectivesw.co.uk
 */

function file_init()
{
    file_attachments();
/* file_attachments() can return false to indicate a load failure,
   but if you return false to oncreate then
   the popup menu will not display which is not a good thing.
 */
}

function file_attachments()
{
    var apChild=document.getElementById('attachmentPopup').cloneNode(true);
    if (!apChild)
        return false;
    apChild.removeAttribute('popupanchor');
    apChild.removeAttribute('popupalign');
    var amParent=document.getElementById('fileAttachmentMenu');
    if (!amParent)
        return false;
    if (apChild.childNodes.length){
        if ( amParent.childNodes.length )
            amParent.removeChild(amParent.childNodes[0]); 
        amParent.appendChild(apChild);
        amParent.removeAttribute('hidden');
    }
    else
        amParent.setAttribute('hidden',true);
    return true;
}

function view_init()
{
    var message_menuitem=document.getElementById('menu_showMessage');

    if (message_menuitem)
    {
        var message_menuitem_hidden = message_menuitem.getAttribute("hidden");
        if(message_menuitem_hidden != "true"){
            message_menuitem.setAttribute('checked',!IsThreadAndMessagePaneSplitterCollapsed());
        }
    }
    var threadColumn = document.getElementById('ThreadColumnHeader');
    var thread_menuitem=document.getElementById('menu_showThreads');
    if (threadColumn && thread_menuitem){
        thread_menuitem.setAttribute('checked',threadColumn.getAttribute('currentView')=='threaded');
    }
}

function InitViewMessagesMenu()
{
    var allMenuItem = document.getElementById("viewAllMessagesMenuItem");
    var hidden = allMenuItem.getAttribute("hidden") == "true";
    if(allMenuItem && !hidden)
        allMenuItem.setAttribute("checked", messageView.viewType == nsMsgViewType.eShowAll);

    var unreadMenuItem = document.getElementById("viewUnreadMessagesMenuItem");
    hidden = unreadMenuItem.getAttribute("hidden") == "true";
    if(unreadMenuItem && !hidden)
        unreadMenuItem.setAttribute("checked", messageView.viewType == nsMsgViewType.eShowUnread);

}

function InitMessageMenu()
{
    var aMessage = GetSelectedMessage(0);
    var isNews = false;
    if(aMessage)
    {
        isNews = GetMessageType(aMessage) == "news";
    }

    //We show reply to Newsgroups only for news messages.
    var replyNewsgroupMenuItem = document.getElementById("replyNewsgroupMainMenu");
    if(replyNewsgroupMenuItem)
    {
        replyNewsgroupMenuItem.setAttribute("hidden", isNews ? "" : "true");
    }

    //For mail messages we say reply. For news we say ReplyToSender.
    var replyMenuItem = document.getElementById("replyMainMenu");
    if(replyMenuItem)
    {
        replyMenuItem.setAttribute("hidden", !isNews ? "" : "true");
    }

    var replySenderMenuItem = document.getElementById("replySenderMainMenu");
    if(replySenderMenuItem)
    {
        replySenderMenuItem.setAttribute("hidden", isNews ? "" : "true");
    }

    //disable the move and copy menus only if there are no messages selected.
    var moveMenu = document.getElementById("moveMenu");
    if(moveMenu)
        moveMenu.setAttribute("disabled", !aMessage);

    var copyMenu = document.getElementById("copyMenu");
    if(copyMenu)
        copyMenu.setAttribute("disabled", !aMessage);
}

function GetMessageType(message)
{
    var compositeDS = GetCompositeDataSource("MessageProperty");
    var property = RDF.GetResource('http://home.netscape.com/NC-rdf#MessageType');
    var result = compositeDS.GetTarget(message, property, true);
    result = result.QueryInterface(Components.interfaces.nsIRDFLiteral);
    return result.Value;
}

function InitMessageMark()
{
    InitMarkReadItem("markReadMenuItem");
    InitMarkReadItem("markReadToolbarItem");
    InitMarkFlaggedItem("markFlaggedMenuItem");
    InitMarkFlaggedItem("markFlaggedToolbarItem");
}

function InitMarkReadItem(id)
{
    var areMessagesRead = SelectedMessagesAreRead();
    var item = document.getElementById(id);
    if(item)
        item.setAttribute("checked", areMessagesRead);
}

function InitMarkFlaggedItem(id)
{
    var areMessagesFlagged = SelectedMessagesAreFlagged();
    var item = document.getElementById(id);
    if(item)
        item.setAttribute("checked", areMessagesFlagged);
}

function SelectedMessagesAreRead()
{
    var aMessage = GetSelectedMessage(0);

    var compositeDS = GetCompositeDataSource("MarkMessageRead");
    var property = RDF.GetResource('http://home.netscape.com/NC-rdf#IsUnread');

    var areMessagesRead =false;

    if(!aMessage)
        areMessagesRead = false;
    else
    {
        var result = compositeDS.GetTarget(aMessage, property, true);
        result = result.QueryInterface(Components.interfaces.nsIRDFLiteral);
        areMessagesRead = result.Value != "true"
    }

    return areMessagesRead;
}

function SelectedMessagesAreFlagged()
{
    var aMessage = GetSelectedMessage(0);

    var compositeDS = GetCompositeDataSource("MarkMessageFlagged");
    var property = RDF.GetResource('http://home.netscape.com/NC-rdf#Flagged');

    var areMessagesFlagged = false;

    if(!aMessage)
        areMessagesFlagged = false;
    else
    {
        var result = compositeDS.GetTarget(aMessage, property, true);
        result = result.QueryInterface(Components.interfaces.nsIRDFLiteral);
        areMessagesFlagged = (result.Value == "flagged");
    }
    return areMessagesFlagged;
}

function GetFirstSelectedMsgFolder()
{
    var result = null;
    var selectedFolders = GetSelectedMsgFolders();
    if (selectedFolders.length > 0) {
        result = selectedFolders[0];
    }

    return result;
}

function GetInboxFolder(server)
{
    try {
        var rootFolder = server.RootFolder;
        var rootMsgFolder = rootFolder.QueryInterface(Components.interfaces.nsIMsgFolder);

        //now find Inbox
        var outNumFolders = new Object();
        var inboxFolder = rootMsgFolder.getFoldersWithFlag(0x1000, 1, outNumFolders);

        return inboxFolder.QueryInterface(Components.interfaces.nsIMsgFolder);
    }
    catch (ex) {
        dump(ex + "\n");
    }
    return null;
}

function GetMessagesForInboxOnServer(server)
{
    var inboxFolder = GetInboxFolder(server);
    if (!inboxFolder) return;

    var folders = new Array(1);
    folders[0] = inboxFolder;

    var compositeDataSource = GetCompositeDataSource("GetNewMessages");
    GetNewMessages(folders, compositeDataSource);
}

function MsgGetMessage() 
{
    var folders = GetSelectedMsgFolders();
    var compositeDataSource = GetCompositeDataSource("GetNewMessages");
    GetNewMessages(folders, compositeDataSource);
}

function MsgGetMessagesForAllServers(defaultServer)
{
    // now log into any server
    try 
    {
        var allServers = accountManager.allServers;
     
        for (var i=0;i<accountManager.allServers.Count();i++) 
        {
            var currentServer = accountManager.allServers.GetElementAt(i).QueryInterface(Components.interfaces.nsIMsgIncomingServer);
            var protocolinfo = Components.classes["@mozilla.org/messenger/protocol/info;1?type=" + currentServer.type].getService(Components.interfaces.nsIMsgProtocolInfo);
            if (protocolinfo.canLoginAtStartUp && currentServer.loginAtStartUp) 
            {
                if (defaultServer && defaultServer.equals(currentServer)) 
                {
                    dump(currentServer.serverURI + "...skipping, already opened\n");
                }
                else 
                {
                    // this assumes "logging is" means getting message on the inbox...is that always true?
                    GetMessagesForInboxOnServer(currentServer);
                }
            }
        }
    }
    catch(ex) 
    {
        dump(ex + "\n");
    }
}

/** 
  * Get messages for all those accounts which have the capability
  * of getting messages and have session password available i.e.,
  * curretnly logged in accounts. 
  */  
function MsgGetMessagesForAllAuthenticatedAccounts()
{
    try 
    {
        var allServers = accountManager.allServers;
     
        for (var i=0;i<allServers.Count();i++) 
        {
            var currentServer = allServers.GetElementAt(i).QueryInterface(Components.interfaces.nsIMsgIncomingServer);
            var protocolinfo = Components.classes["@mozilla.org/messenger/protocol/info;1?type=" + 
                                 currentServer.type].getService(Components.interfaces.nsIMsgProtocolInfo);
            if (protocolinfo.canGetMessages && currentServer.password) 
            {
                // Get new messages now
                GetMessagesForInboxOnServer(currentServer);
            }
        }
    }
    catch(ex) 
    {
        dump(ex + "\n");
    }
}

/** 
  * Get messages for the account selected from Menu dropdowns.
  */  
function MsgGetMessagesForAccount(aEvent)
{
    if (!aEvent)
        return;

    var uri = aEvent.target.id;
    var server = GetServer(uri);
    GetMessagesForInboxOnServer(server);
    aEvent.preventBubble();
}

function MsgGetNextNMessages()
{
    var folder = GetFirstSelectedMsgFolder();
    if(folder)
    {
        GetNextNMessages(folder)
    }
}

function MsgDeleteMessage(reallyDelete, fromToolbar)
{

    if(reallyDelete)
        dump("reallyDelete\n");
    var srcFolder = GetLoadedMsgFolder();
    // if from the toolbar, return right away if this is a news message
    // only allow cancel from the menu:  "Edit | Cancel / Delete Message"
    if (fromToolbar)
    {
        var folderResource = srcFolder.QueryInterface(Components.interfaces.nsIRDFResource);
        var uri = folderResource.Value;
        if (isNewsURI(uri))
        {
            //dump("delete ignored!\n");
            return;
        }
    }
    //dump("tree is valid\n");
    //get the selected elements

    var compositeDataSource = GetCompositeDataSource("DeleteMessages");
    var messages = GetSelectedMessages();

    SetNextMessageAfterDelete();
    DeleteMessages(compositeDataSource, srcFolder, messages, reallyDelete);
}

function MsgCopyMessage(destFolder)
{
    // Get the id for the folder we're copying into
    var destUri = destFolder.getAttribute('id');
    var destResource = RDF.GetResource(destUri);
    var destMsgFolder = destResource.QueryInterface(Components.interfaces.nsIMsgFolder);

    var srcFolder = GetLoadedMsgFolder();
    if(srcFolder)
    {
        var compositeDataSource = GetCompositeDataSource("Copy");
        var messages = GetSelectedMessages();

        CopyMessages(compositeDataSource, srcFolder, destMsgFolder, messages, false);
    }    
}

function MsgMoveMessage(destFolder)
{
    // Get the id for the folder we're copying into
    var destUri = destFolder.getAttribute('id');
    var destResource = RDF.GetResource(destUri);
    var destMsgFolder = destResource.QueryInterface(Components.interfaces.nsIMsgFolder);
    
    var srcFolder = GetLoadedMsgFolder();
    if(srcFolder)
    {
        var compositeDataSource = GetCompositeDataSource("Move");
        var messages = GetSelectedMessages();

        var srcResource = srcFolder.QueryInterface(Components.interfaces.nsIRDFResource);
        var srcUri = srcResource.Value;
        if (isNewsURI(srcUri))
        {
            CopyMessages(compositeDataSource, srcFolder, destMsgFolder, messages, false);
        }
        else
        {
            SetNextMessageAfterDelete();
            CopyMessages(compositeDataSource, srcFolder, destMsgFolder, messages, true);
        }
    }    
}

function MsgNewMessage(event)
{
  var loadedFolder = GetFirstSelectedMsgFolder();
  var messageArray = GetSelectedMessages();

  if (event && event.shiftKey)
    ComposeMessage(msgComposeType.New, msgComposeFormat.OppositeOfDefault, loadedFolder, messageArray);
  else
    ComposeMessage(msgComposeType.New, msgComposeFormat.Default, loadedFolder, messageArray);
} 

function MsgReplyMessage(event)
{
  var loadedFolder = GetLoadedMsgFolder();

  var server = loadedFolder.server;

  if(server && server.type == "nntp")
    MsgReplyGroup(event);
  else 
    MsgReplySender(event);

}

function MsgReplySender(event)
{
  var loadedFolder = GetLoadedMsgFolder();
  var messageArray = GetSelectedMessages();

  if (event && event.shiftKey)
    ComposeMessage(msgComposeType.ReplyToSender, msgComposeFormat.OppositeOfDefault, loadedFolder, messageArray);
  else
    ComposeMessage(msgComposeType.ReplyToSender, msgComposeFormat.Default, loadedFolder, messageArray);

}

function MsgReplyGroup(event)
{
  var loadedFolder = GetLoadedMsgFolder();
  var messageArray = GetSelectedMessages();

  if (event && event.shiftKey)
    ComposeMessage(msgComposeType.ReplyToGroup, msgComposeFormat.OppositeOfDefault, loadedFolder, messageArray);
  else
    ComposeMessage(msgComposeType.ReplyToGroup, msgComposeFormat.Default, loadedFolder, messageArray);
}

function MsgReplyToAllMessage(event) 
{
  var loadedFolder = GetLoadedMsgFolder();
  var messageArray = GetSelectedMessages();

  dump("\nMsgReplyToAllMessage from XUL\n");
  if (event && event.shiftKey)
    ComposeMessage(msgComposeType.ReplyAll, msgComposeFormat.OppositeOfDefault, loadedFolder, messageArray);
  else
    ComposeMessage(msgComposeType.ReplyAll, msgComposeFormat.Default, loadedFolder, messageArray);
}

function MsgForwardMessage(event)
{

  dump("\nMsgForwardMessage from XUL\n");
  var forwardType = 0;
  try {
      forwardType = pref.GetIntPref("mail.forward_message_mode");
  } catch (e) {dump ("failed to retrieve pref mail.forward_message_mode");}
  
  if (forwardType == 0)
      MsgForwardAsAttachment(event);
  else
      MsgForwardAsInline(event);
}

function MsgForwardAsAttachment(event)
{
  var loadedFolder = GetLoadedMsgFolder();
  var messageArray = GetSelectedMessages();

  dump("\nMsgForwardAsAttachment from XUL\n");
  if (event && event.shiftKey)
    ComposeMessage(msgComposeType.ForwardAsAttachment,
                   msgComposeFormat.OppositeOfDefault, loadedFolder, messageArray);
  else
    ComposeMessage(msgComposeType.ForwardAsAttachment, msgComposeFormat.Default, loadedFolder, messageArray);
}

function MsgForwardAsInline(event)
{
  var loadedFolder = GetLoadedMsgFolder();
  var messageArray = GetSelectedMessages();

 dump("\nMsgForwardAsInline from XUL\n");
  if (event && event.shiftKey)
    ComposeMessage(msgComposeType.ForwardInline,
                   msgComposeFormat.OppositeOfDefault, loadedFolder, messageArray);
  else
    ComposeMessage(msgComposeType.ForwardInline, msgComposeFormat.Default, loadedFolder, messageArray);
}


function MsgEditMessageAsNew()
{
    var loadedFolder = GetLoadedMsgFolder();
    var messageArray = GetSelectedMessages();
    ComposeMessage(msgComposeType.Template, msgComposeFormat.Default, loadedFolder, messageArray);
}

function MsgHome(url)
{
  window.open( url, "_blank", "chrome,dependent=yes,all" );
}

function MsgNewFolder()
{
    var preselectedFolder = GetFirstSelectedMsgFolder();
    var dualUseFolders = true;
    var server = null;
    var destinationFolder = null;

    if (preselectedFolder)
    {
        try {
            server = preselectedFolder.server;
            if (server)
            {
                destinationFolder = getDestinationFolder(preselectedFolder, server);

                var imapServer =
                    server.QueryInterface(Components.interfaces.nsIImapIncomingServer);
                if (imapServer)
                    dualUseFolders = imapServer.dualUseFolders;
            }
        } catch (e) {
            dump ("Exception: dualUseFolders = true\n");
        }
    }

    CreateNewSubfolder("chrome://messenger/content/newFolderDialog.xul", destinationFolder, dualUseFolders);
}


function getDestinationFolder(preselectedFolder, server)
{
    var destinationFolder = null;

    var isCreateSubfolders = preselectedFolder.canCreateSubfolders;
    if (!isCreateSubfolders) 
    {
        var tmpDestFolder = server.RootFolder;
        destinationFolder 
          = tmpDestFolder.QueryInterface(Components.interfaces.nsIMsgFolder);

        var verifyCreateSubfolders = null;
        if (destinationFolder)
            verifyCreateSubfolders = destinationFolder.canCreateSubfolders;

        // in case the server cannot have subfolders,
        // get default account and set its incoming server as parent folder
        if (!verifyCreateSubfolders) 
        {
            try {
                var account = accountManager.defaultAccount;
                var defaultServer = account.incomingServer;
                var tmpDefaultFolder = defaultServer.RootFolder;
                var defaultFolder 
                  = tmpDefaultFolder.QueryInterface(Components.interfaces.nsIMsgFolder);

                var checkCreateSubfolders = null;
                if (defaultFolder)
                    checkCreateSubfolders = defaultFolder.canCreateSubfolders;

                if (checkCreateSubfolders)
                    destinationFolder = defaultFolder;

            } catch (e) {
                dump ("Exception: defaultAccount Not Available\n");
            }
        }
    }
    else
        destinationFolder = preselectedFolder;

    return destinationFolder;
}

function MsgSubscribe()
{
    var preselectedFolder = GetFirstSelectedMsgFolder();
    Subscribe(preselectedFolder);
}

function ConfirmUnsubscribe(folder)
{
    var sBundle = srGetStrBundle("chrome://messenger/locale/messenger.properties"); 
    var titleMsg = sBundle.GetStringFromName("confirmUnsubscribeTitle");
    var dialogMsg = sBundle.formatStringFromName("confirmUnsubscribeText",
                                        [ folder.name], 1);

    var commonDialogService = nsJSComponentManager.getService("@mozilla.org/appshell/commonDialogs;1",
                                                                    "nsICommonDialogs");
    return commonDialogService.Confirm(window, titleMsg, dialogMsg);    
}

function MsgUnsubscribe()
{
    var folder = GetFirstSelectedMsgFolder();
    if (ConfirmUnsubscribe(folder)) {
    	UnSubscribe(folder);
    }
}

function MsgSaveAsFile() 
{
    dump("\MsgSaveAsFile from XUL\n");
    var messages = GetSelectedMessages();
    if (messages && messages.length == 1)
    {
        SaveAsFile(messages[0]);
    }
}


function MsgSaveAsTemplate() 
{
    dump("\MsgSaveAsTemplate from XUL\n");
    var folder = GetLoadedMsgFolder();
    var messages = GetSelectedMessages();
    if (messages && messages.length == 1)
    {
        SaveAsTemplate(messages[0], folder);
    }
}

function MsgOpenNewWindowForFolder(folderUri)
{
    if(!folderUri)
    {
        var folder = GetLoadedMsgFolder();
        var folderResource = folder.QueryInterface(Components.interfaces.nsIRDFResource);
        folderUri = folderResource.Value;
    }

    if(folderUri)
    {
        var layoutType = pref.GetIntPref("mail.pane_config");
        
        if(layoutType == 0)
            window.openDialog( "chrome://messenger/content/messenger.xul", "_blank", "chrome,all,dialog=no", folderUri );
        else
            window.openDialog("chrome://messenger/content/mail3PaneWindowVertLayout.xul", "_blank", "chrome,all,dialog=no", folderUri );
    }

}

function MsgOpenSelectedMessages()
{
  var threadTree = GetThreadTree();
  var selectedMessages = threadTree.selectedItems;
  var numMessages = selectedMessages.length;

  for(var i = 0; i < numMessages; i++) {
    var messageNode = selectedMessages[i];
    messageUri = messageNode.getAttribute("id");
    if(messageUri) {
      MsgOpenNewWindowForMessage(messageUri, null);
    }
  }
}

function MsgOpenNewWindowForMessage(messageUri, folderUri)
{
    var message;

    if(!messageUri)
    {
        message = GetSelectedMessage(0);
        var messageResource = message.QueryInterface(Components.interfaces.nsIRDFResource);
        messageUri = messageResource.Value;
    }

    if(!folderUri)
    {
        message = RDF.GetResource(messageUri);
        message = message.QueryInterface(Components.interfaces.nsIMessage);
        var folder = message.msgFolder;
        var folderResource = folder.QueryInterface(Components.interfaces.nsIRDFResource);
        folderUri = folderResource.Value;
    }

    if(messageUri && folderUri)
    {
        window.openDialog( "chrome://messenger/content/messageWindow.xul", "_blank", "chrome,all,dialog=no", messageUri, folderUri );
    }


}

function CloseMailWindow() 
{
    dump("\nClose from XUL\nDo something...\n");
    window.close();
}

function MsgMarkMsgAsRead(markRead)
{
    if(markRead == null)
    {
        markRead= !SelectedMessagesAreRead();
    }
    var selectedMessages = GetSelectedMessages();
    var compositeDataSource = GetCompositeDataSource("MarkMessageRead");

    MarkMessagesRead(compositeDataSource, selectedMessages, markRead);
}

function MsgMarkAsFlagged(markFlagged)
{
    if(markFlagged == null)
    {
        markFlagged= !SelectedMessagesAreFlagged();
    }

    var selectedMessages = GetSelectedMessages();
    var compositeDataSource = GetCompositeDataSource("MarkMessageFlagged");

    MarkMessagesFlagged(compositeDataSource, selectedMessages, markFlagged);
}

function MsgMarkAllRead()
{
    var compositeDataSource = GetCompositeDataSource("MarkAllMessagesRead");
    var folder = GetLoadedMsgFolder();

    if(folder)
        MarkAllMessagesRead(compositeDataSource, folder);
}

function MsgDownloadFlagged()
{
    var compositeDataSource = GetCompositeDataSource("DownloadFlagged");
    var folder = GetLoadedMsgFolder();

    if(folder)
        DownloadFlaggedMessages(compositeDataSource, folder);
}

function MsgDownloadSelected()
{
    var selectedMessages = GetSelectedMessages();
    var compositeDataSource = GetCompositeDataSource("DownloadSelected");

    DownloadSelectedMessages(compositeDataSource, selectedMessages);
}

function MsgMarkThreadAsRead()
{
    var messageList = GetSelectedMessages();
    if(messageList.length == 1)
    {
        var message = messageList[0];
        var compositeDataSource = GetCompositeDataSource("MarkThreadAsRead");

        MarkThreadAsRead(compositeDataSource, message);
    }
}

function MsgViewPageSource() 
{
    //dump("MsgViewPageSource(); \n ");
    var messages = GetSelectedMessages();
    ViewPageSource(messages);
}

function MsgFind() 
{
    messenger.find();
}

function MsgFindAgain() 
{
    messenger.findAgain();
}

function MsgSearchMessages() 
{
    var preselectedFolder = GetFirstSelectedMsgFolder();
    window.openDialog("chrome://messenger/content/SearchDialog.xul", "SearchMail", "chrome,resizable", { folder: preselectedFolder });
}

function MsgFilters() 
{
    var preselectedFolder = GetFirstSelectedMsgFolder();
    window.openDialog("chrome://messenger/content/FilterListDialog.xul", "FilterDialog", "chrome,resizable", { folder: preselectedFolder });
}

function MsgViewAllHeaders() 
{
    pref.SetIntPref("mail.show_headers",2);
    MsgReload();
    return true;
}

function MsgViewNormalHeaders() 
{
    pref.SetIntPref("mail.show_headers",1);
    MsgReload();
    return true;
}

function MsgViewBriefHeaders() 
{
    pref.SetIntPref("mail.show_headers",0);
    MsgReload();
    return true;
}

function MsgReload() 
{
    ReloadMessage();
}

function MsgStop()
{
    StopUrls();
}

function MsgSendUnsentMsg() 
{
    var folder = GetFirstSelectedMsgFolder();
    if(folder)
    {
        SendUnsentMessages(folder);
    }
}

function PrintEnginePrint()
{
    var messageList = GetSelectedMessages();
    numMessages = messageList.length;


    if (numMessages == 0)
    {
        dump("PrintEnginePrint(): No messages selected.\n");
        return false;
    }  

    var selectionArray = new Array(numMessages);

    for(var i = 0; i < numMessages; i++)
    {
        var messageResource = messageList[i].QueryInterface(Components.interfaces.nsIRDFResource);
        selectionArray[i] = messageResource.Value;
    }

    printEngineWindow = window.openDialog("chrome://messenger/content/msgPrintEngine.xul",
                                                        "",
                                                        "chrome,dialog=no,all",
                                                        numMessages, selectionArray, statusFeedback);
    return true;
}

function IsMailFolderSelected()
{
    var selectedFolders = GetSelectedMsgFolders();
    var numFolders = selectedFolders.length;
    if(numFolders !=1)
        return false;
        
    var folder = selectedFolders[0];
    if (!folder)
        return false;
    
    var server = folder.server;
    var serverType = server.type;
    
    if((serverType == "nntp"))
        return false;
    else return true;
}

function IsGetNewMessagesEnabled()
{
    var selectedFolders = GetSelectedMsgFolders();
    var numFolders = selectedFolders.length;
    if(numFolders !=1)
        return false;
        
    var folder = selectedFolders[0];
    if (!folder)
        return false;
    
    var server = folder.server;
    var isServer = folder.isServer;
    var serverType = server.type;
    
    if(isServer && (serverType == "nntp"))
        return false;
    else if(serverType == "none")
        return false;
    else
        return true;    
}

function IsGetNextNMessagesEnabled()
{
    var selectedFolders = GetSelectedMsgFolders();
    var numFolders = selectedFolders.length;
    if(numFolders !=1)
        return false;

    var folder = selectedFolders[0];
    if (!folder)
        return false;
   
    var server = folder.server;
    var serverType = server.type;
   
    var menuItem = document.getElementById("menu_getnextnmsg");
    if((serverType == "nntp")) {
        var newsServer = server.QueryInterface(Components.interfaces.nsINntpIncomingServer);
        var menuValue = Bundle.formatStringFromName("getNextNMessages",
                                        [ newsServer.maxArticles], 1);
        menuItem.setAttribute("value",menuValue);
        menuItem.setAttribute("hidden","false");
        return true;
    }
    else {
        menuItem.setAttribute("hidden","true");
        return false;
    }
}

function IsEmptyTrashEnabled()
{
    return IsMailFolderSelected();
}

function IsCompactFolderEnabled()
{
    return IsMailFolderSelected();
}

var gDeleteButton = null;
var gMarkButton = null;

function SetUpToolbarButtons(uri)
{
    // dump("SetUpToolbarButtons("+uri+")\n");

    // eventually, we might want to set up the toolbar differently for imap,
    // pop, and news.  for now, just tweak it based on if it is news or not.
    var forNews = isNewsURI(uri);

    if(!gMarkButton) gMarkButton = document.getElementById("button-mark");
    if(!gDeleteButton) gDeleteButton = document.getElementById("button-delete");
    
    var buttonToHide = null;
    var buttonToShow = null;

    if (forNews) {
        buttonToHide = gDeleteButton;
        buttonToShow = gMarkButton;
    }
    else {
        buttonToHide = gMarkButton;
        buttonToShow = gDeleteButton;
    }

    if (buttonToHide) {
        buttonToHide.setAttribute('hidden',true);
    }
    if (buttonToShow) {
        buttonToShow.removeAttribute('hidden');
    }
}

var gMessageBrowser;

function getMessageBrowser()
{
  if (!gMessageBrowser)
    gMessageBrowser = document.getElementById("messagepane");

  return gMessageBrowser;
}

function getMarkupDocumentViewer()
{
  return getMessageBrowser().markupDocumentViewer;
}

function MsgMarkByDate() {}
function MsgOpenAttachment() {}
function MsgUpdateMsgCount() {}
function MsgImport() {}
function MsgWorkOffline() {}
function MsgSynchronize() {}
function MsgGetSelectedMsg() {}
function MsgGetFlaggedMsg() {}
function MsgSelectThread() {}
function MsgSelectFlaggedMsg() {}
function MsgShowFolders(){}
function MsgShowLocationbar() {}
function MsgViewAttachInline() {}
function MsgWrapLongLines() {}
function MsgIncreaseFont() {}
function MsgDecreaseFont() {}
function MsgShowImages() {}
function MsgRefresh() {}
function MsgViewPageInfo() {}
function MsgFirstUnreadMessage() {}
function MsgFirstFlaggedMessage() {}
function MsgGoBack() {}
function MsgGoForward() {}
function MsgAddSenderToAddressBook() {}
function MsgAddAllToAddressBook() {}
function MsgIgnoreThread() {}
function MsgWatchThread() {}
