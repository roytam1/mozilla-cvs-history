var msgAppCore;
var composeAppCore;

function FindMsgAppCore()
{
  msgAppCore = XPAppCoresManager.Find("MsgAppCore");
  if (msgAppCore == null) {
    dump("FindMsgAppCore: Creating AppCore\n");
    msgAppCore = new MsgAppCore();
    dump("Initializing MsgAppCore and setting Window\n");
    msgAppCore.Init("MsgAppCore");
  }
  return msgAppCore;
}

function OpenURL(url)
{
  dump("\n\nOpenURL from XUL\n\n\n");
  var appCore = FindMsgAppCore();
  if (appCore != null) {
    appCore.SetWindow(window);
    appCore.OpenURL(url);
  }
}

function ComposeMessage(tree, nodeList, msgAppCore, type)
{
	dump("\nComposeMessage from XUL\n");

	// Generate a unique number, do we have a better way?
	var date = new Date();
	sessionID = date.getTime() + Math.random();
	
	var composeAppCoreName = "ComposeAppCore:" + sessionID;
	var composeAppCore = XPAppCoresManager.Find(composeAppCoreName);
	if (! composeAppCore)
	{
		composeAppCore = new ComposeAppCore();
		if (composeAppCore)
		{
			composeAppCore.Init(composeAppCoreName);
			//argument:
			//	name=<name of the appcore>
			//	editorType=[default | html | text]			; default means use the prefs value send_html
			var args = "name=" + composeAppCoreName + ",editorType=default";
			composeAppCore.NewMessage("chrome://messengercompose/content/", args, tree, nodeList, msgAppCore, type);
			dump("Created a compose appcore from Messenger, " + args);
		}
	}
}

function NewMessage()
{

  dump("\n\nnewMsg from XUL\n\n\n");
  ComposeMessage(null, null, null, 0);
}

function GetNewMessages()
{
	var folderTree = frames[0].frames[0].document.getElementById('folderTree'); 
	var selectedFolderList = folderTree.getElementsByAttribute("selected", "true");
	if(selectedFolderList.length > 0)
	{
		var selectedFolder = selectedFolderList[0];
		
		var appCore = FindMsgAppCore();
		if (appCore != null) {
			appCore.SetWindow(window);
			appCore.GetNewMessages(folderTree.database, selectedFolder);
		}
	}
	else {
		dump("Nothing was selected\n");
	}
}

function MsgAccountManager()
{
  var appCore = FindMsgAppCore();
  if (appCore != null) {
    dump('Opening account manager..\n');
    appCore.AccountManager(window);
  }
}

function LoadMessage(messageNode)
{
  var uri = messageNode.getAttribute('id');
  dump(uri);
  OpenURL(uri);
}

function ChangeFolderByDOMNode(folderNode)
{
  var uri = folderNode.getAttribute('id');
  dump(uri + "\n");
  ChangeFolderByURI(uri);
}

function ChangeFolderByURI(uri)
{
  var tree = frames[0].frames[1].document.getElementById('threadTree');
  tree.childNodes[5].setAttribute('id', uri);
}

function ComposeMessageWithType(type)
{
  dump("\nMsgReplyMessage from XUL\n");
  var tree = frames[0].frames[1].document.getElementById('threadTree');
  if(tree) {
    dump("tree is valid\n");
    var nodeList = tree.getElementsByAttribute("selected", "true");
    var appCore = FindMsgAppCore();
    dump("message type ");
    dump(type);
    dump("\n");
    if (appCore && nodeList)
      appCore.SetWindow(window);
      ComposeMessage(tree, nodeList, appCore, type);
  }
}

function SortThreadPane(column, sortKey)
{
	var node = frames[0].frames[1].document.getElementById(column);
	if(!node)
		return false;

	var rdfCore = XPAppCoresManager.Find("RDFCore");
	if (!rdfCore)
	{
		rdfCore = new RDFCore();
		if (!rdfCore)
		{
			return(false);
		}

		rdfCore.Init("RDFCore");

	}

	// sort!!!
	sortDirection = "ascending";
    var currentDirection = node.getAttribute('sortDirection');
    if (currentDirection == "ascending")
            sortDirection = "descending";
    else if (currentDirection == "descending")
            sortDirection = "ascending";
    else    sortDirection = "ascending";

    rdfCore.doSort(node, sortKey, sortDirection);

    return(true);


}

function MsgPreferences()
{
  var prefsCore = XPAppCoresManager.Find("PrefsCore");
  if (!prefsCore) {
    prefsCore = new PrefsCore();
    if (prefsCore) {
      prefsCore.Init("PrefsCore");
    }
  }
  if (prefsCore) {
    prefsCore.ShowWindow(window);
  }
}

