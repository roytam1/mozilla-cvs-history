/* -*- Mode: Java; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */


//
// Determine if d&d is on or not, off by default for beta but we want mozilla
// folks to be able to turn it on if they so desire.
//
var gDragDropEnabled = false;
var pref = null;
try {
  pref = Components.classes['component://netscape/preferences'];
  pref = pref.getService();
  pref = pref.QueryInterface(Components.interfaces.nsIPref);
}
catch (ex) {
  dump("failed to get prefs service!\n");
  pref = null;
}

try {
  gDragDropEnabled = pref.GetBoolPref("xpfe.dragdrop.enable");
}
catch (ex) {
  dump("assuming d&d is off for Bookmarks\n");
}  


// XXX this currently controls whether bookmarks D&D is enabled or not
gDragDropEnabled = true;


function TopLevelDrag ( event )
{
  dump("TOP LEVEL bookmarks window got a drag");
  return(true);
}


function BeginDragTree ( event )
{
  if ( !gDragDropEnabled )
    return;
    
  //XXX we rely on a capturer to already have determined which item the mouse was over
  //XXX and have set an attribute.
    
  // if the click is on the tree proper, ignore it. We only care about clicks on items.

  var tree = document.getElementById("bookmarksTree");
  if ( event.target == tree )
    return(true);         // continue propagating the event
    
  var childWithDatabase = tree;
  if ( ! childWithDatabase )
    return(false);
    
  var dragStarted = false;

  var trans = 
    Components.classes["component://netscape/widget/transferable"].createInstance(Components.interfaces.nsITransferable);
  if ( !trans ) return(false);

  var genData = 
    Components.classes["component://netscape/supports-wstring"].createInstance(Components.interfaces.nsISupportsWString);
  if (!genData) return(false);

  var genDataURL = 
    Components.classes["component://netscape/supports-wstring"].createInstance(Components.interfaces.nsISupportsWString);
  if (!genDataURL) return(false);

  trans.addDataFlavor("text/unicode");
  trans.addDataFlavor("moz/rdfitem");
        
  // ref/id (url) is on the <treeitem> which is two levels above the <treecell> which is
  // the target of the event.
  var id = event.target.parentNode.parentNode.getAttribute("ref");
  if (!id || id=="")
  {
	id = event.target.parentNode.parentNode.getAttribute("id");
  }
  dump("\n    ID: " + id);

  var parentID = event.target.parentNode.parentNode.parentNode.parentNode.getAttribute("ref");
  if (!parentID || parentID == "")
  {
	parentID = event.target.parentNode.parentNode.parentNode.parentNode.getAttribute("id");
  }
  dump("    Parent ID: " + parentID);

  var trueID = id;
  if (parentID != null)
  {
  	trueID += "\n" + parentID;
  }
  genData.data = trueID;
  genDataURL.data = id;

  var database = childWithDatabase.database;
  var rdf = 
    Components.classes["component://netscape/rdf/rdf-service"].getService(Components.interfaces.nsIRDFService);
  if ((!rdf) || (!database))  { dump("CAN'T GET DATABASE\n"); return(false); }

  // make sure its a bookmark, bookmark separator, or bookmark folder
  var src = rdf.GetResource(id, true);
  var prop = rdf.GetResource("http://www.w3.org/1999/02/22-rdf-syntax-ns#type", true);
  var target = database.GetTarget(src, prop, true);

  if (target) target = target.QueryInterface(Components.interfaces.nsIRDFResource);
  if (target) target = target.Value;
  if ((!target) || (target == "")) {dump("BAD\n"); return(false);}
  dump("    Type: '" + target + "'");

  if ((target != "http://home.netscape.com/NC-rdf#BookmarkSeparator") &&
    (target != "http://home.netscape.com/NC-rdf#Bookmark") &&
    (target != "http://home.netscape.com/NC-rdf#Folder")) return(false);

dump("genData is " + genData.data + " len is " + genData.data.length + "\n");
  trans.setTransferData ( "moz/rdfitem", genData, genData.data.length * 2);  // double byte data
  trans.setTransferData ( "text/unicode", genDataURL, genDataURL.data.length * 2);  // double byte data

  var transArray = 
    Components.classes["component://netscape/supports-array"].createInstance(Components.interfaces.nsISupportsArray);
  if ( !transArray )  return(false);

  // put it into the transferable as an |nsISupports|
  var genTrans = trans.QueryInterface(Components.interfaces.nsISupports);
  transArray.AppendElement(genTrans);
  
  var dragService = 
    Components.classes["component://netscape/widget/dragservice"].getService(Components.interfaces.nsIDragService);
  if ( !dragService ) return(false);

  var nsIDragService = Components.interfaces.nsIDragService;
  dragService.invokeDragSession ( transArray, null, nsIDragService.DRAGDROP_ACTION_COPY + 
                                     nsIDragService.DRAGDROP_ACTION_MOVE );
  dragStarted = true;

  return(!dragStarted);
}



function DragOverTree ( event )
{
  if ( !gDragDropEnabled )
    return(false);

  // for beta1, don't allow D&D if sorting is active
  var tree = document.getElementById("bookmarksTree");
  if (!tree)	return(false);
  var sortActive = tree.getAttribute("sortActive");
  if (sortActive == "true")
  {
  	dump("Sorry, drag&drop is currently disabled when sorting is active.\n");
  	return(false);
  }
    
  var validFlavor = false;
  var dragSession = null;
  var retVal = true;

  var dragService = 
    Components.classes["component://netscape/widget/dragservice"].getService(Components.interfaces.nsIDragService);
  if ( !dragService ) return(false);

  dragSession = dragService.getCurrentSession();
  if ( !dragSession ) return(false);

  if ( dragSession.isDataFlavorSupported("moz/rdfitem") ) validFlavor = true;
  else if ( dragSession.isDataFlavorSupported("text/unicode") ) validFlavor = true;
  //XXX other flavors here...

  // touch the attribute on the rowgroup to trigger the repaint with the drop feedback.
  if ( validFlavor )
  {
    //XXX this is really slow and likes to refresh N times per second.
    var rowGroup = event.target.parentNode.parentNode;
    rowGroup.setAttribute ( "dd-triggerrepaint", 0 );
    dragSession.canDrop = true;
    // necessary??
    retVal = false;
  }
  return(retVal);
}



function DropOnTree ( event )
{
  if ( !gDragDropEnabled )
    return(false);

  var treeRoot = document.getElementById("bookmarksTree");
  if (!treeRoot)  return(false);
  var treeDatabase = treeRoot.database;
  if (!treeDatabase)  return(false);

  // for beta1, don't allow D&D if sorting is active
  var sortActive = treeRoot.getAttribute("sortActive");
  if (sortActive == "true")
  {
  	dump("Sorry, drag&drop is currently disabled when sorting is active.\n");
  	return(false);
  }

  var RDF = 
    Components.classes["component://netscape/rdf/rdf-service"].getService(Components.interfaces.nsIRDFService);
  if (!RDF) return(false);
  var RDFC =
    Components.classes["component://netscape/rdf/container"].getService(Components.interfaces.nsIRDFContainer);
  if (!RDFC)  return(false);

  var Bookmarks = RDF.GetDataSource("rdf:bookmarks");
  if (!Bookmarks) return(false);

  // target is the <treecell>, and "ref/id" is on the <treeitem> two levels above
  var treeItem = event.target.parentNode.parentNode;
  if (!treeItem)  return(false);
  var targetID = getAbsoluteID(treeRoot, treeItem);
  if (!targetID)  return(false);
  var targetNode = RDF.GetResource(targetID, true);
  if (!targetNode)  return(false);

  // get drop hint attributes
  var dropBefore = treeItem.getAttribute("dd-droplocation");
  var dropOn = treeItem.getAttribute("dd-dropon");

  // calculate drop action
  var dropAction;
  if (dropBefore == "true") dropAction = "before";
  else if (dropOn == "true")  dropAction = "on";
  else        dropAction = "after";

  // calculate parent container node
  var containerItem = treeItem;
  if (dropAction != "on")
    containerItem = treeItem.parentNode.parentNode;

  var containerID = getAbsoluteID(treeRoot, containerItem);
  if (!containerID) return(false);
  var containerNode = RDF.GetResource(containerID);
  if (!containerNode) return(false);

  var dragService = 
    Components.classes["component://netscape/widget/dragservice"].getService(Components.interfaces.nsIDragService);
  if ( !dragService ) return(false);
  
  var dragSession = dragService.getCurrentSession();
  if ( !dragSession ) return(false);

  var trans = 
    Components.classes["component://netscape/widget/transferable"].createInstance(Components.interfaces.nsITransferable);
  if ( !trans )   return(false);
  trans.addDataFlavor("moz/rdfitem");
  trans.addDataFlavor("text/unicode");

  var dirty = false;

  for ( var i = 0; i < dragSession.numDropItems; ++i )
  {
    dragSession.getData ( trans, i );
    var dataObj = new Object();
    var bestFlavor = new Object();
    var len = new Object();
    trans.getAnyTransferData ( bestFlavor, dataObj, len );
    if ( dataObj )  dataObj = dataObj.value.QueryInterface(Components.interfaces.nsISupportsWString);
    if ( !dataObj ) continue;

    var sourceID = null;
    var parentID = null;
    var checkNameHack = false;

    if (bestFlavor.value == "moz/rdfitem")
    {
	    // pull the URL out of the data object
	    var data = dataObj.data.substring(0, len.value / 2);;

	    var cr = data.indexOf("\n");
	    if (cr >= 0)
	    {
	    	sourceID = data.substr(0, cr);
	    	parentID = data.substr(cr+1);
	    }
    }
    else
    {
    	sourceID = dataObj.data;

	// XXX for the moment, if its a text/unicode drop
	// we may need to synthesize a name (just use the URL)
	checkNameHack = true;
    }

    dump("    Node #" + i + ": drop '" + sourceID + "'\n");
    dump("             from container '" + parentID + "'\n");
    dump("             action = '" + dropAction + "'\n");
    dump("             target = '" + targetID + "'\n");

    var sourceNode = RDF.GetResource(sourceID, true);
    if (!sourceNode)  continue;

    var parentNode = null;
    if (parentID != null)
    {
    	parentNode = RDF.GetResource(parentID, true);
    }
    
    // Prevent dropping of a node before, after, or on itself
    if (sourceNode == targetNode) continue;
    // Prevent dropping of a node onto its parent container
    if ((dropAction == "on") && (containerID) && (containerID == parentID))	continue;

    RDFC.Init(Bookmarks, containerNode);

    if ((dropAction == "before") || (dropAction == "after"))
    {
      // drop before or after
      var nodeIndex;

      nodeIndex = RDFC.IndexOf(sourceNode);

      if (nodeIndex >= 1)
      {
        // moving a node around inside of the container
        // so remove, then re-add the node
        RDFC.RemoveElementAt(nodeIndex, true, sourceNode);
      }
      
      nodeIndex = RDFC.IndexOf(targetNode);

      if (nodeIndex < 1)  return(false);
      if (dropAction == "after")  ++nodeIndex;

      RDFC.InsertElementAt(sourceNode, nodeIndex, true);

	// select the newly added node
	if (parentID)
	{
	      selectDroppedItems(treeRoot, containerID, sourceID);
	}	     

      dirty = true;
    }
    else
    {
      // drop on
      RDFC.AppendElement(sourceNode);

	// select the newly added node
	if (parentID)
	{
	      selectDroppedItems(treeRoot, containerID, sourceID);
	}	     

      dirty = true;
    }

    if (checkNameHack == true)
    {
	// XXX for the moment, if its a text/unicode drop
	// we may need to synthesize a name (just use the URL)
	var srcArc = RDF.GetResource(sourceID, true);
	var propArc = RDF.GetResource("http://home.netscape.com/NC-rdf#Name", true);
	if (srcArc && propArc && treeDatabase)
	{
		var targetArc = treeDatabase.GetTarget(srcArc, propArc, true);
		if (!targetArc)
		{
			var defaultNameArc = RDF.GetLiteral(sourceID);
			if (defaultNameArc)
			{
				treeDatabase.Assert(srcArc, propArc, defaultNameArc, true); 
			}
		}
	}
    }
  }

	// should we move the node? (i.e. take it out of the source container?)
	if ((parentNode != null) && (containerNode != parentNode))
	{
		RDFC.Init(Bookmarks, parentNode);
		var nodeIndex = RDFC.IndexOf(sourceNode);

		if (nodeIndex >= 1)
		{
			RDFC.RemoveElementAt(nodeIndex, true, sourceNode);
		}
	}

  if (dirty == true)
  {
    var remote = Bookmarks.QueryInterface(Components.interfaces.nsIRDFRemoteDataSource);
    if (remote)
    {
      remote.Flush();
      dump("Wrote out bookmark changes.");
    }
  }

  return(false);
}



function selectDroppedItems(treeRoot, containerID, targetID)
{
	var select_list = treeRoot.getElementsByAttribute("id", targetID);
	for (var x=0; x<select_list.length; x++)
	{
		var node = select_list[x];
		if (!node)	continue;

		var parent = node.parentNode.parentNode;
		if (!parent)	continue;

		var id = parent.getAttribute("ref");
		if (!id || id=="")
		{
			id = parent.getAttribute("id");
		}
		if (!id || id=="")	continue;

		if (id == containerID)
		{
			node.setAttribute("selected", "true");
			break;
		}
	}
}
