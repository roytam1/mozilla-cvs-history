
function MsgStartUp()
{
  dump("StartUp: MsgAppCore\n");
  var appCore = FindMsgAppCore();
  if (appCore != null) {
    dump("Initializing AppCore and setting Window\n");
    appCore.SetWindow(window);
  }
}

function MsgLoadNewsMessage(url)
{
  dump("\n\nMsgLoadNewsMessage from XUL\n");
  OpenURL(url);
}

function MsgHome(url)
{
  var toolkitCore = XPAppCoresManager.Find("ToolkitCore");
  if (!toolkitCore) {
      toolkitCore = new ToolkitCore();
      if (toolkitCore) {
        toolkitCore.Init("ToolkitCore");
      }
    }
  if (toolkitCore) {
    toolkitCore.ShowWindow(url, window);
  }
}

function MsgNewMessage() 
{
  dump("\n\nMsgNewMessage from XUL\n");
  NewMessage();
} 

function MsgGetMessage() 
{
  GetNewMail();
}

function MsgDeleteMessage()
{
  dump("\nMsgDeleteMessage from XUL\n");
  var tree = frames[0].frames[1].document.getElementById('threadTree');
  if(tree)
    dump("tree is valid\n");
  var appCore = FindMsgAppCore();
  if (appCore != null) {
    dump("\nAppcore isn't null in MsgDeleteMessage\n");
    appCore.SetWindow(window);
    var NodeList = tree.getElementsByAttribute("selected", "true");
    appCore.DeleteMessage(tree, NodeList);
  }
}

function MsgReplyMessage()
{
  ComposeMessageWithType(0);
}

function MsgReplyToAllMessage() 
{
  ComposeMessageWithType(1);
}

function MsgForwardMessage()
{
  ComposeMessageWithType(3);
}

function MsgForwardAsAttachment()
{
  ComposeMessageWithType(2);
}

function MsgForwardAsInline()
{
  ComposeMessageWithType(3);
}

function MsgForwardAsQuoted()
{
  ComposeMessageWithType(4);
}

function MsgCopyMessage(destFolder)
{
    uri = destFolder.getAttribute('id');

	dump(uri);

}

function MsgNewFolder() {}
function MsgOpenAttachment() {}
function MsgSaveAsFile() {}
function MsgSaveAsTemplate() {}
function MsgSendUnsentMsg() {}
function MsgUpdateMsgCount() {}
function MsgSubscribe() {}
function MsgRenameFolder() {}
function MsgEmptyTrash() {}
function MsgCompactFolders() {}
function MsgImport() {}
function MsgWorkOffline() {}
function MsgSynchronize() {}
function MsgGetSelectedMsg() {}
function MsgGetFlaggedMsg() {}
function MsgEditUndo() {}
function MsgEditRedo() {}
function MsgEditCut() {}
function MsgEditCopy() {}
function MsgEditPaste() {}
function MsgSelectAll() {}
function MsgSelectThread() {}
function MsgSelectFlaggedMsg() {}
function MsgFind() {}
function MsgFindAgain() {}
function MsgSearchMessages() {}
function MsgFilters() {}
function MsgFolderProperties() {}
function MsgPreferences() {}
function MsgShowMsgToolbar() {}
function MsgShowLocationbar() {}
function MsgShowMessage() {}
function MsgShowFolders() {}
function MsgSortByDate() {}
function MsgSortByFlag() {}
function MsgSortByPriority() {}
function MsgSortBySender() {}
function MsgSortBySize() {}
function MsgSortByStatus() {}
function MsgSortBySubject() {}
function MsgSortByThread() {}
function MsgSortByUnread() {}
function MsgSortByOrderReceived() {}
function MsgSortAscending() {}
function MsgSortDescending() {}
function MsgViewAllMsgs() {}
function MsgViewUnreadMsg() {}
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
function MsgStop() {}
function MsgViewPageSource() {}
function MsgViewPageInfo() {}
function MsgFirstUnreadMessage() {}
function MsgFirstFlaggedMessage() {}
function MsgNextMessage() {}
function MsgNextUnreadMessage() {}
function MsgNextFlaggedMessage() {}
function MsgPreviousMessage() {}
function MsgPreviousUnreadMessage() {}
function MsgPreviousFlaggedMessage() {}
function MsgGoBack() {}
function MsgGoForward() {}
function MsgEditMessageAsNew() {}
function MsgMoveMessage() {}
function MsgAddSenderToAddressBook() {}
function MsgAddAllToAddressBook() {}
function MsgMarkMsgAsRead() {}
function MsgMarkThreadAsRead() {}
function MsgMarkByDate() {}
function MsgMarkAllRead() {}
function MsgMarkAsFlagged() {}
function MsgIgnoreThread() {}
function MsgWatchThread() {}
