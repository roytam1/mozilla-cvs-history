
function doSearch()
{
	// get user text to find
	var textNode = document.getElementById("searchtext");
	if (!textNode)	return(false);
	var text = textNode.value;
	if (!text)	return(false);
	dump("Search text: " + text + "\n");

	// get selected search engines
	var treeBody = document.getElementById("NC:SearchEngineRoot");
	if (!treeBody)	return(false);

	var searchURL="";
	var foundEngine = false;

	var numEngines = treeBody.childNodes.length;
	dump("Found treebody, it has " + numEngines + " kids\n");
	for (var x = 0; x<numEngines; x++)
	{
		var treeItem = treeBody.childNodes[x];
		if (!treeItem)	continue;
		// XXX when its fully implemented, instead use
		//     var engines = document.getElementsByTagName("checkbox");
		if (treeItem.childNodes[0].childNodes[0].getAttribute("value") == "1")
		{
			var engineURI = treeItem.getAttribute("id");
			if (!engineURI)	continue;
			dump ("# " + x + ":  " + engineURI + "\n");

			if (searchURL == "")
			{
				searchURL = "internetsearch:";
			}
			else
			{
				searchURL += "&";
			}
			searchURL += "engine=" + engineURI;
			foundEngine = true;
		}
	}
	if (foundEngine == false)	return(false);

	searchURL += "&text=" + escape(text);
	dump("Internet Search URL: " + searchURL + "\n");

	// set text in results pane
	var summaryNode = parent.frames[1].document.getElementById("internetresultssummary");
	if (summaryNode)
	{
		var summaryText = "Results: term contains '";
		summaryText += text + "'";
		summaryNode.setAttribute("value", summaryText);
	}

	// load find URL into results pane
	var resultsTree = parent.frames[1].document.getElementById("internetresultstree");
	if (!resultsTree)	return(false);
	x = resultsTree.childNodes.length;
	if (x < 1)		return(false);
	// XXX shouldn't assume that treebody is the last child node in the tree!
	resultsTree.childNodes[x-1].setAttribute("id", searchURL);

	dump("doSearch done.\n");

	return(true);
}

function doUncheckAll()
{
	// get selected search engines
	var treeBody = document.getElementById("NC:SearchEngineRoot");
	if (!treeBody)	return(false);

	var numEngines = treeBody.childNodes.length;
	dump("Found treebody, it has " + numEngines + " kids\n");
	for (var x = 0; x<numEngines; x++)
	{
		var treeItem = treeBody.childNodes[x];
		if (!treeItem)	continue;
		// XXX when its fully implemented, instead use
		//     var engines = document.getElementsByTagName("checkbox");
		treeItem.childNodes[0].childNodes[0].setAttribute("value", "0");
	}

	dump("doUncheckAll() done.\n");

	return(true);
}



function saveSearch()
{
	var resultsTree = parent.frames[1].document.getElementById("internetresultstree");
	if (!resultsTree)	return(false);
	x = resultsTree.childNodes.length;
	if (x < 1)		return(false);
	// XXX shouldn't assume that treebody is the last child node in the tree!
	var searchURL = resultsTree.childNodes[x-1].getAttribute("id");
	if (!searchURL)		return(false);

	dump("Bookmark search URL: " + searchURL + "\n");

	var bmks = Components.classes["component://netscape/browser/bookmarks-service"].getService();
	bmks = bmks.QueryInterface(Components.interfaces.nsIBookmarksService);
	// XXX should construct a more interesting/useful title
	bmks.AddBookmark(searchURL, "Saved Internet Search");

	return(true);
}
