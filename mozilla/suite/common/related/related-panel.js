/* -*- Mode: Java; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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

  Code for the Related Links Sidebar Panel

 */

// The content window that we're supposed to be observing.
var ContentWindow = window.content;

// The related links handler
var Handler = Components.classes["component://netscape/related-links-handler"].createInstance();
Handler = Handler.QueryInterface(Components.interfaces.nsIRelatedLinksHandler);

// Our observer object
var Observer = {
    Observe: function(subject, topic, data) {
        // dump("Observer.Observe(" + subject + ", " + topic + ", " + data + ")\n");
        if (subject != ContentWindow)
            return;

        // Okay, it's a hit. Before we go out and fetch RL data, make sure that
        // the RelatedLinks folder is open.
        var root = document.getElementById('NC:RelatedLinks');

        if (root.getAttribute('open') == 'true')
        {
        	refetchRelatedLinks(Handler, data);
        }
    }
}



function refetchRelatedLinks(Handler, data)
{
	// we can only get related links data on HTTP URLs
	var newSite = "" + data;
	if (newSite.indexOf("http://") != 0)
	{
		dump("Unable to fetch related links data on non-HTTP URL.\n");
		return(false);
	}
	newSite = newSite.substr(7);			// strip off "http://" prefix
	var portOffset = newSite.indexOf(":");
	var slashOffset = newSite.indexOf("/");
	var theOffset = ((portOffset >=0) && (portOffset <= slashOffset)) ? portOffset : slashOffset;
	if (theOffset >= 0)	newSite = newSite.substr(0, theOffset);

	var currentSite = "";
	if (Handler.URL != null)
	{
		currentSite = Handler.URL.substr(7);	// strip off "http://" prefix
		portOffset = currentSite.indexOf(":");
		slashOffset = currentSite.indexOf("/");
		theOffset = ((portOffset >=0) && (portOffset <= slashOffset)) ? portOffset : slashOffset;
		if (theOffset >= 0)	currentSite = currentSite.substr(0, theOffset );
	}

	dump("Related Links:  Current top-level: " + currentSite + "    new top-level: " + newSite + "\n");

	// only request new related links data if we've got a new web site (hostname change)
	if (currentSite != newSite)
	{
		Handler.URL = data;
	}
}



function Init()
{
    // Initialize the Related Links panel

    // Create a Related Links handler, and install it in the tree
    var Tree = document.getElementById("Tree");
    Tree.database.AddDataSource(Handler.QueryInterface(Components.interfaces.nsIRDFDataSource));

    // Install the observer so we'll be notified when new content is loaded.
    var ObserverService = Components.classes["component://netscape/observer-service"].getService();
    ObserverService = ObserverService.QueryInterface(Components.interfaces.nsIObserverService);
    dump("got observer service\n");

    ObserverService.AddObserver(Observer, "EndDocumentLoad");
    dump("added observer\n");
}



function OnDblClick(treeitem, root)
{
    // Deal with a double-click

    // First, see if they're opening the related links node. If so,
    // we'll need to go out and fetch related links _now_.
    if (treeitem.getAttribute('id') == 'NC:RelatedLinks' &&
        treeitem.getAttribute('open') != 'true') {
	refetchRelatedLinks(Handler, ContentWindow.location);
        return;
    }

    // Next, check to see if it's a container. If so, then just let
    // the tree do its open and close stuff.
    if (treeitem.getAttribute('container') == 'true') {

    //    if (treeitem.getAttribute('open') == 'true') {
    //      treeitem.setAttribute('open','')
    //    } else {
    //      treeitem.setAttribute('open','true')
    //    }

        return;
    }

    if (treeitem.getAttribute('type') == 'http://home.netscape.com/NC-rdf#BookmarkSeparator') {
        return;
    }

    // Okay, it's not a container. See if it has a URL, and if so, open it.
    var id = treeitem.getAttribute('id');
    if (!id)	return(false);

	// rjc: add support for anonymous resources; if the node has
	// a "#URL" property, use it, otherwise default to using the id
	try
	{
		var rootNode = document.getElementById(root);
		var ds = null;
		if (rootNode)
		{
			ds = rootNode.database;
		}
		var rdf = Components.classes["component://netscape/rdf/rdf-service"].getService();
		if (rdf)   rdf = rdf.QueryInterface(Components.interfaces.nsIRDFService);
		if (rdf)
		{
			if (ds)
			{
				var src = rdf.GetResource(id, true);
				var prop = rdf.GetResource("http://home.netscape.com/NC-rdf#URL", true);
				var target = ds.GetTarget(src, prop, true);
				if (target)	target = target.QueryInterface(Components.interfaces.nsIRDFLiteral);
				if (target)	target = target.Value;
				if (target)	id = target;
				
			}
		}
	}
	catch(ex)
	{
	}
    ContentWindow.location = id;
}
