var gSubscribeTree = null;
var okCallback = null;
var gChangeTable = {};
var gServerURI = null;
var RDF = null;
var gSubscribeDS = null;
var gStatusBar = null;
var gNameField = null;
var gFolderDelimiter = ".";

function SetUpRDF()
{
	if (!RDF) {
			RDF = Components.classes["component://netscape/rdf/rdf-service"].getService();
			RDF = RDF.QueryInterface(Components.interfaces.nsIRDFService);
	}
		
	if (!gSubscribeDS) {
		gSubscribeDS = RDF.GetDataSource("rdf:subscribe");
	}
}

function Stop()
{
	//dump("Stop()\n");
	dump("xxx todo: we need to stop the url that is running.\n");
}

var Bundle = srGetStrBundle("chrome://messenger/locale/subscribe.properties");

function SetServerTypeSpecificTextValues()
{
	var serverType = GetMsgFolderFromUri(gServerURI).server.type;
	//dump("serverType="+serverType+"\n");
	
	/* set the server specific ui elements */
    var element = document.getElementById("foldertextlabel");
	var stringName = "foldertextfor-" + serverType;
	var stringval = Bundle.GetStringFromName(stringName);
	element.setAttribute('value',stringval);

	stringName = "foldersheaderfor-" + serverType;
	stringval = Bundle.GetStringFromName(stringName);
    element = document.getElementById("foldersheaderlabel");
	element.setAttribute('value',stringval);

	// XXX todo, fix this hack
	// qi the server to get a nsISubscribable server
	// and ask it for the delimiter
	if (serverType == "nntp") {
		gFolderDelimiter = ".";
	}
	else {
		gFolderDelimiter = "/";
	}
}

function onServerClick(event)
{
	var item = event.target;
	gServerURI = item.id;
	//dump("gServerURI="+gServerURI+"\n");

	SetServerTypeSpecificTextValues();
	SetUpTree(false);
}

function SetUpServerMenu()
{
	//dump("SetUpServerMenu()\n");

    var serverMenu = document.getElementById("serverMenu");
    var menuitems = serverMenu.getElementsByAttribute("id", gServerURI);

	try {
		//dump("gServerURI="+gServerURI+"\n");
		//dump("menuitems="+menuitems+"\n");
		//dump("menuitems[0]="+menuitems[0]+"\n");
		//dump("serverMenu="+serverMenu+"\n");
    	serverMenu.selectedItem = menuitems[0];
	}
	catch (ex) {
		dump("failed to set the selected server: " + ex + "\n");
	}

	SetServerTypeSpecificTextValues();
}

var MySubscribeListener = {
	OnStopPopulating: function() {
		//dump("root subscribe tree at: "+ gServerURI +"\n");
		gSubscribeTree.setAttribute('ref',gServerURI);
		// Turn progress meter off.
      	gStatusBar.setAttribute("mode","normal");	
	}
};

function SetUpTree(forceToServer)
{
	//dump("SetUpTree()\n");
	SetUpRDF();
	
	gSubscribeTree.setAttribute('ref',null);

	var folder = GetMsgFolderFromUri(gServerURI);
	var server = folder.server;

	try {
		subscribableServer = server.QueryInterface(Components.interfaces.nsISubscribableServer);

		subscribableServer.subscribeListener = MySubscribeListener;

		// Turn progress meter on.
      	gStatusBar.setAttribute("mode","undetermined");	

		subscribableServer.populateSubscribeDatasource(null /* eventually, a nsIMsgWindow */, forceToServer);
	}
	catch (ex) {
		dump("failed to populate subscribe ds: " + ex + "\n");
	}
}

function SubscribeOnLoad()
{
	//dump("SubscribeOnLoad()\n");
	
  gSubscribeTree = document.getElementById('subscribetree');
	gStatusBar = document.getElementById('statusbar-icon');
	gNameField = document.getElementById('namefield');

	doSetOKCancel(subscribeOK,subscribeCancel);

	// look in arguments[0] for parameters
	if (window.arguments && window.arguments[0]) {
		if ( window.arguments[0].title ) {
			top.window.title = window.arguments[0].title;
		}
		
		if ( window.arguments[0].okCallback ) {
			top.okCallback = window.arguments[0].okCallback;
		}
	}
	
	if (window.arguments[0].preselectedURI) {
		var uri = window.arguments[0].preselectedURI;
		//dump("subscribe: got a uri," + uri + "\n");
		folder = GetMsgFolderFromUri(uri);
		dump("xxx todo:  make sure this is a subscribable server\n");
		//dump("folder="+folder+"\n");
		//dump("folder.server="+folder.server+"\n");
		gServerURI = folder.server.serverURI;
		//dump("gServerURI="+gServerURI+"\n");
	}
	else {
		dump("subscribe: no uri\n");
		dump("xxx todo:  use the default news server\n");
		var serverMenu = document.getElementById("serverMenu");
		var menuitems = serverMenu.getElementsByTagName("menuitem");
		gServerURI = menuitems[1].id;
	}

	SetUpServerMenu();
	SetUpTree(false);

  
  gNameField.focus();
}

function subscribeOK()
{
	//dump("in subscribeOK()\n")
	if (top.okCallback) {
		top.okCallback(top.gServerURI,top.gChangeTable);
	}
	return true;
}

function subscribeCancel()
{
	//dump("in subscribeCancel()\n");
	Stop();
	return true;
}

function SetState(uri,name,state,stateStr)
{
	//dump("SetState(" + uri +"," + name + "," + state + "," + stateStr + ")\n");
	if (!uri || !stateStr) return;

	try {
		var src = RDF.GetResource(uri, true);
		var prop = RDF.GetResource("http://home.netscape.com/NC-rdf#Subscribed", true);
		var oldLiteral = gSubscribeDS.GetTarget(src, prop, true);
		//dump("oldLiteral="+oldLiteral+"\n");

		oldValue = oldLiteral.QueryInterface(Components.interfaces.nsIRDFLiteral).Value;
		//dump("oldLiteral.Value="+oldValue+"\n");
		if (oldValue != stateStr) {
			var newLiteral = RDF.GetLiteral(stateStr);
			gSubscribeDS.Change(src, prop, oldLiteral, newLiteral);
			StateChanged(name,state);
		}
	}
	catch (ex) {
		dump("failed: " + ex + "\n");
	}
}

function StateChanged(name,state)
{
	//dump("StateChanged(" + name + "," + state + ")\n");
	//dump("start val=" +gChangeTable[name] + "\n");

	if (gChangeTable[name] == undefined) {
		//dump(name+" not in table yet\n");
		gChangeTable[name] = state;
	}
	else {
		var oldValue = gChangeTable[name];
		//dump(name+" already in table\n");
		if (oldValue != state) {
			gChangeTable[name] = undefined;
		}
	}

	//dump("end val=" +gChangeTable[name] + "\n");
}

function SetSubscribeState(state)
{
  //dump("SetSubscribedState()\n");
 
  var stateStr;
  if (state) {
  	stateStr = 'true';
  }
  else {
	stateStr = 'false';
  }

  try {
	//dump("subscribe button clicked\n");
	
	var groupList = gSubscribeTree.selectedItems;
	for (i=0;i<groupList.length;i++) {
		group = groupList[i];
		uri = group.getAttribute('id');
		//dump(uri + "\n");
		name = group.getAttribute('name');
		//dump(name + "\n");
		SetState(uri,name,state,stateStr);
	}
  }
  catch (ex) {
	dump("SetSubscribedState failed:  " + ex + "\n");
  }
}

function ReverseStateFromNode(node)
{
	var stateStr;
	var state;

	if (node.getAttribute('Subscribed') == "true") {
		stateStr = "false";
		state = false;
	}
	else {
		stateStr = "true";
		state = true;
	}
	
	var uri = node.getAttribute('id');
	var	name = node.getAttribute('name');
	SetState(uri, name, state, stateStr);
}

function ReverseState(uri)
{
	//dump("ReverseState("+uri+")\n");
}

function SubscribeOnClick(event)
{
	if (event.detail == 2) {
		ReverseStateFromNode(event.target.parentNode.parentNode);
	}
	else {
 		var targetclass = event.target.getAttribute('class');
		if (targetclass != 'tree-cell-twisty') {
			var name = event.target.parentNode.parentNode.getAttribute('name');
			gNameField.setAttribute('value',name);
		}
	}
}

function RefreshList()
{
	// force it to talk to the server
	SetUpTree(true);
}

function trackGroupInTree()
{
  var portion = gNameField.value;
  selectNodeByName( portion );  
}

function selectNodeByName( aMatchString )
{
  var lastDot = aMatchString.lastIndexOf(gFolderDelimiter);
  var nodeValue = lastDot != -1 ? aMatchString.substring(0, lastDot) : aMatchString;
  
  var chain = aMatchString.split(gFolderDelimiter);
  if( chain.length == 1 ) {
    var node = getTreechildren(gSubscribeTree);
    if( !node )
      return;
  }
  else {
    var node = gSubscribeTree.getElementsByAttribute("name",nodeValue)[0];
    if( node.getAttribute("container") == "true" && 
        node.getAttribute("open") != "true" )
      node.setAttribute("open","true");
    node = getTreechildren(node);
    dump("*** node = " + node.nodeName + "\n");
  }
  
  for( var i = 0; i < node.childNodes.length; i++ )
  {
    var currItem = node.childNodes[i];
    dump("*** chain = " + chain[chain.length-1] + "\n");
    if( !currItem.getAttribute("name").indexOf( aMatchString ) ) {
      gSubscribeTree.selectItem( currItem );
      gSubscribeTree.ensureElementIsVisible( currItem );
      return;
    }
  }      
}

function getTreechildren( aTreeNode )
{
  if( aTreeNode.childNodes ) 
    {
      for( var i = 0; i < aTreeNode.childNodes.length; i++ )
      {
        if( aTreeNode.childNodes[i].nodeName.toLowerCase() == "treechildren" )
          return aTreeNode.childNodes[i];
      }
      return aTreeNode;
    }
  return null;
}
