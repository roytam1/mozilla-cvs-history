/* -*- Mode: Java; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

/*
  Script for the bookmarks properties window
*/

function BookmarkProperties()
{
  var tree = document.getElementById('bookmarksTree');
  var select_list = tree.getElementsByAttribute("selected", "true");

  if (select_list.length >= 1) {
    var props = window.open("resource://res/samples/bm-props.xul", "BookmarkProperties", "chrome");
    props.BookmarkURL = select_list[0].getAttribute("id");
  } else {
    dump("nothing selected!\n"); 
  }
}

function OpenURL(event, node)
{
	// clear any single-click/edit timeouts
	if (timerID != null)
	{
		gEditNode = null;
		clearTimeout(timerID);
		timerID = null;
	}

	if (node.getAttribute('container') == "true")
	{
		return(false);
	}

	var url = node.getAttribute('id');

	// Ignore "NC:" urls.
	if (url.substring(0, 3) == "NC:")
	{
		return(false);
	}

	window.open(url,'bookmarks');
	dump("OpenURL(" + url + ")\n");

	return(true);
}

	var htmlInput = null;
	var saveNode = null;
	var newValue = "";
	var timerID = null;
	var gEditNode = null;

function DoSingleClick(event, node)
{
	var type = node.parentNode.parentNode.getAttribute('type');
	if (type == "http://home.netscape.com/NC-rdf#BookmarkSeparator")
	{
		// don't allow inline editing of separators
		return(false);
	}

	gEditNode = node;
	
	// edit node if we don't get a double-click in less than 1/2 second
	timerID = setTimeout("OpenEditNode()", 500);
	return(true);
}

function OpenEditNode()
{
	dump("OpenEditNode entered.\n");

	// clear any single-click/edit timeouts
	if (timerID != null)
	{
		clearTimeout(timerID);
		timerID = null;
	}

	// XXX uncomment the following line to replace the whole input row we do this
	// (and, therefore, only allow editing on the name column) until we can
	// arbitrarily change the content model (bugs prevent this at the moment)
	gEditNode = gEditNode.parentNode;

	var name = gEditNode.parentNode.getAttribute("Name");
	dump("Single click on '" + name + "'\n");

	CloseEditNode();

	var theParent = gEditNode.parentNode;
	dump("Parent node is a " + theParent.nodeName + "\n\n");

	saveNode = gEditNode;

	// unselect all nodes!
//	gEditNode.removeAttribute("selected");
	var select_list = document.getElementsByAttribute("selected", "true");
	dump("# of Nodes selected: " + select_list.length + "\n\n");
	for (var nodeIndex=0; nodeIndex<select_list.length; nodeIndex++)
	{
		var node = select_list[nodeIndex];
		if (node)
		{
			node.removeAttribute("selected");
		}
	}

	// XXX for now, just remove the child from the parent
	// optimally, we'd like to not remove the child-parent relationship
	// and instead just set a "display: none;" attribute on the child node

//	gEditNode.setAttribute("style", "display: none;");
//	dump("gEditNode hidden.\n");
	theParent.removeChild(gEditNode);
	dump("gEditNode removed.\n");

	// create the html:input node
	htmlInput = document.createElement("html:input");
	htmlInput.setAttribute("value", name);
	htmlInput.setAttribute("onkeydown", "return EditNodeKeyDown(event)");
	htmlInput.setAttribute("onkeyup", "return EditNodeKeyUp(event)");

	theParent.appendChild(htmlInput);
	dump("html:input node added.\n");

	htmlInput.focus();

	dump("OpenEditNode done.\n");
	return(true);
}

function CloseEditNode(saveChangeFlag)
{
	dump("CloseEditNode entered.\n");

	if (htmlInput)
	{
		dump("  Got html input.\n");

		var theParent = htmlInput.parentNode;
		theParent.removeChild(htmlInput);
		theParent.appendChild(saveNode);
		dump("  child node appended.\n");

		if (saveChangeFlag)
		{
			var RDF = Components.classes["component://netscape/rdf/rdf-service"].getService();
			RDF = RDF.QueryInterface(Components.interfaces.nsIRDFService);
			var Bookmarks = RDF.GetDataSource("rdf:bookmarks");
			dump("Got bookmarks datasource.\n");

			// XXX once we support multi-column editing, get the property
			// from the column in the content model
			var propertyName = "http://home.netscape.com/NC-rdf#Name";
			var propertyNode = RDF.GetResource(propertyName, true);
			dump("  replacing value of property '" + propertyName + "'\n");
			
			// get the URI
			theNode = saveNode;
			var bookmarkURL = "";
			while(true)
			{
				var tag = theNode.nodeName;
				if (tag == "treeitem")
				{
					bookmarkURL = theNode.getAttribute("id");
					break;
				}
				theNode = theNode.parentNode;
			}
			dump("  uri is '" + bookmarkURL + "'\n");

			if (bookmarkURL == "")	return(false);
			var bookmarkNode = RDF.GetResource(bookmarkURL, true);


			dump("  newValue = '" + newValue + "'\n");
			newValue = (newValue != "") ? RDF.GetLiteral(newValue) : null;

			var oldValue = Bookmarks.GetTarget(bookmarkNode, propertyNode, true);
			if (oldValue)
			{
				oldValue = oldValue.QueryInterface(Components.interfaces.nsIRDFLiteral);
				dump("  oldValue = '" + oldValue + "'\n");
			}

			if (oldValue != newValue)
			{
				if (oldValue && !newValue)
				{
					Bookmarks.Unassert(bookmarkNode, propertyNode, oldValue);
					dump("  Unassert used.\n");
				}
				else if (!oldValue && newValue)
				{
					Bookmarks.Assert(bookmarkNode, propertyNode, newValue, true);
					dump("  Assert used.\n");
				}
				else if (oldValue && newValue)
				{
					Bookmarks.Change(bookmarkNode, propertyNode, oldValue, newValue);
					dump("  Change used.\n");
				}

				dump("re-writing bookmarks.html\n");
				var remote = Bookmarks.QueryInterface(Components.interfaces.nsIRDFRemoteDataSource);
				remote.Flush();
			}

			newValue = "";
			saveNode = null;
		}
		else dump("saveNode was null?\n");
		htmlInput = null;
	}
	else dump("htmlInput was null?\n");

	dump("CloseEditNode done.\n");
}

function EditNodeKeyDown(event)
{
	if (event.which == 27)
	{
		CloseEditNode(false);
		return(false);
	}
	return(true);
}

function EditNodeKeyUp(event)
{
	if (event.which == 13 || event.which == 10)
	{
		if (htmlInput)
		{
			newValue = htmlInput.value;
			dump("EditNodeKeyUp:  newValue = '" + newValue + "'\n");
		}
		CloseEditNode(true);
		return(false);
	}
	return(true);
}

function doSort(sortColName)
{
  var node = document.getElementById(sortColName);
  // determine column resource to sort on
  var sortResource = node.getAttribute('resource');
  if (!node) return(false);

  var sortDirection="ascending";
  var isSortActive = node.getAttribute('sortActive');
  if (isSortActive == "true") {
    var currentDirection = node.getAttribute('sortDirection');
    if (currentDirection == "ascending")
      sortDirection = "descending";
    else if (currentDirection == "descending")
      sortDirection = "natural";
    else
      sortDirection = "ascending";
  }

  // get RDF Core service
  var rdfCore = XPAppCoresManager.Find("RDFCore");
  if (!rdfCore) {
    rdfCore = new RDFCore();
    if (!rdfCore) {
      return(false);
    }
    rdfCore.Init("RDFCore");
//    XPAppCoresManager.Add(rdfCore);
  }
  // sort!!!
  rdfCore.doSort(node, sortResource, sortDirection);
  return(false);
}


function fillContextMenu(name)
{
	if (!name)	return(false);
	var popupNode = document.getElementById(name);
	if (!popupNode)	return(false);

	// remove the menu node (which tosses all of its kids);
	// do this in case any old command nodes are hanging around
	var menuNode = popupNode.childNodes[0];
	popupNode.removeChild(menuNode);

	// create a new menu node
	menuNode = document.createElement("menu");
	popupNode.appendChild(menuNode);

	var treeNode = document.getElementById("bookmarksTree");
	if (!treeNode)	return(false);
	var db = treeNode.database;
	if (!db)	return(false);
	
	var compositeDB = db.QueryInterface(Components.interfaces.nsIRDFDataSource);
	if (!compositeDB)	return(false);

	var isupports = Components.classes["component://netscape/rdf/rdf-service"].getService();
	if (!isupports)	return(false);
	var rdf = isupports.QueryInterface(Components.interfaces.nsIRDFService);
	if (!rdf)	return(false);

	var select_list = treeNode.getElementsByAttribute("selected", "true");
	if (select_list.length < 1)	return(false);
	
	dump("# of Nodes selected: " + select_list.length + "\n\n");

	// perform intersection of commands over selected nodes
	var cmdArray = new Array();

	for (var nodeIndex=0; nodeIndex<select_list.length; nodeIndex++)
	{
		var node = select_list[nodeIndex];
		if (!node)	break;
		var id = node.getAttribute("id");
		if (!id)	break;
		var rdfNode = rdf.GetResource(id);
		if (!rdfNode)	break;
		var cmdEnum = db.GetAllCmds(rdfNode);
		if (!cmdEnum)	break;

		var nextCmdArray = new Array();
		while (cmdEnum.HasMoreElements())
		{
			var cmd = cmdEnum.GetNext();
			if (!cmd)	break;
			if (nodeIndex == 0)
			{
				cmdArray[cmdArray.length] = cmd;
			}
			else
			{
				nextCmdArray[cmdArray.length] = cmd;
			}
		}
		if (nodeIndex > 0)
		{
			// perform command intersection calculation
			for (var cmdIndex = 0; cmdIndex < cmdArray.length; cmdIndex++)
			{
				var	cmdFound = false;
				for (var nextCmdIndex = 0; nextCmdIndex < nextCmdArray.length; nextCmdIndex++)
				{
					if (nextCmdArray[nextCmdIndex] == cmdArray[cmdIndex])
					{
						cmdFound = true;
						break;
					}
				}
				if (cmdFound == false)
				{
					cmdArray[cmdIndex] = null;
				}
			}
		}
	}

	// need a resource to ask RDF for each command's name
	var rdfNameResource = rdf.GetResource("http://home.netscape.com/NC-rdf#Name");
	if (!rdfNameResource)		return(false);

	// build up menu items
	if (cmdArray.length < 1)	return(false);

	for (var cmdIndex = 0; cmdIndex < cmdArray.length; cmdIndex++)
	{
		var cmd = cmdArray[cmdIndex];
		if (!cmd)		continue;
		var cmdResource = cmd.QueryInterface(Components.interfaces.nsIRDFResource);
		if (!cmdResource)	break;
		var cmdNameNode = compositeDB.GetTarget(cmdResource, rdfNameResource, true);
		if (!cmdNameNode)	break;
		cmdNameLiteral = cmdNameNode.QueryInterface(Components.interfaces.nsIRDFLiteral);
		if (!cmdNameLiteral)	break;
		cmdName = cmdNameLiteral.Value;
		if (!cmdName)		break;

		dump("Command #" + cmdIndex + ": id='" + cmdResource.Value + "'  name='" + cmdName + "'\n\n");

		var menuItem = document.createElement("menuitem");
		menuItem.setAttribute("name", cmdName);
		menuItem.setAttribute("onclick", "return doContextCmd('" + cmdResource.Value + "');");
		
		menuNode.appendChild(menuItem);
	}

	return(true);
}



function doContextCmd(cmdName)
{
	dump("doContextCmd start: cmd='" + cmdName + "'\n");

	var treeNode = document.getElementById("bookmarksTree");
	if (!treeNode)	return(false);
	var db = treeNode.database;
	if (!db)	return(false);
	
	var compositeDB = db.QueryInterface(Components.interfaces.nsIRDFDataSource);
	if (!compositeDB)	return(false);

	var isupports = Components.classes["component://netscape/rdf/rdf-service"].getService();
	if (!isupports)	return(false);
	var rdf = isupports.QueryInterface(Components.interfaces.nsIRDFService);
	if (!rdf)	return(false);

	// need a resource for the command
	var cmdResource = rdf.GetResource(cmdName);
	if (!cmdResource)		return(false);
	cmdResource = cmdResource.QueryInterface(Components.interfaces.nsIRDFResource);
	if (!cmdResource)		return(false);

	var select_list = treeNode.getElementsByAttribute("selected", "true");
	if (select_list.length < 1)	return(false);
	
	dump("# of Nodes selected: " + select_list.length + "\n\n");

	// build up selection nsISupportsArray
	var selectionInstance = Components.classes["component://netscape/supports-array"].createInstance();
	if (!selectionInstance)
	{
		dump("unable to create selectionInstance.\n");
		return(false);
	}
	var selectionArray = selectionInstance.QueryInterface(Components.interfaces.nsISupportsArray);
	if (!selectionArray)
	{
		dump("unable to QI to selectionArray.\n");
		return(false);
	}

	for (var nodeIndex=0; nodeIndex<select_list.length; nodeIndex++)
	{
		var node = select_list[nodeIndex];
		if (!node)	break;
		var id = node.getAttribute("id");
		if (!id)	break;

		dump("id: " + id + "\n");

		var rdfNode = rdf.GetResource(id);
		if (!rdfNode)	break;

		selectionArray.AppendElement(rdfNode);
	}

	// build up arguments nsISupportsArray
	var argumentsInstance = Components.classes["component://netscape/supports-array"].createInstance();
	if (!argumentsInstance)
	{
		dump("unable to create argumentsInstance.\n");
		return(false);
	}

	var argumentsArray = argumentsInstance.QueryInterface(Components.interfaces.nsISupportsArray);
	if (!argumentsArray)
	{
		dump("unable to QI to argumentsArray.\n");
		return(false);
	}


	compositeDB.DoCommand( selectionArray, cmdResource, argumentsArray );


	dump("doContextCmd ends: cmd='" + cmdName + "'\n");

	return(true);
}
