/* -*- Mode: Java; tab-width: 4; c-basic-offset: 4; -*-
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

/*

   Script for the directory window

*/

// By the time this runs, The 'HTTPIndex' variable will have been
// magically set on the global object by the native code.

function debug(msg)
{
    // Uncomment to print out debug info.
    dump(msg);
}

function Init()
{
    debug("directory.js: Init()\n");

    // Add the HTTPIndex datasource into the tree
    var tree = document.getElementById('tree');

    // Initialize the tree's base URL to whatever the HTTPIndex is rooted at
    var baseURI = HTTPIndex.BaseURL;

    // fix bug # 37102: if its a FTP directory
    // ensure it ends with a trailing slash
    if (baseURI && (baseURI.indexOf("ftp://") == 0) &&
    	(baseURI.substr(baseURI.length - 1) != "/"))
    {
    	debug("append traiing slash to FTP directory URL\n");
    	baseURI += "/";
    }
    debug("base URL = " + baseURI + "\n");
    
    // Note: set encoding for FTP URLs BEFORE setting "ref"
    HTTPIndex.encoding = "ISO-8859-1";

    // re-root the tree
    tree.setAttribute("ref", baseURI);
}



function OnClick(event)
{
    if( event.type == "click" &&
        ( event.button != 1 || event.detail != 2 ) )
      return false;
    if( event.type == "keypress" && event.which != 13 )
      return false;
    
    var tree = document.getElementById("tree");
    if( tree.selectedItems.length == 1 ) 
      {
        var selectedItem = tree.selectedItems[0];
    
        //if( selectedItem.getAttribute( "type" ) == "FILE" )
            window._content.location.href =  selectedItem.getAttribute('id');
      }
}


// We need this hack because we've completely circumvented the onload() logic.
function Boot()
{
    if (document.getElementById('tree')) {
        Init();
    }
    else {
        setTimeout("Boot()", 500);
    }
}

setTimeout("Boot()", 0);


function doSort(sortColName)
{
	var node = document.getElementById(sortColName);
	if (!node) return(false);

	// determine column resource to sort on
	var sortResource = node.getAttribute('resource');

	// switch between ascending & descending sort (no natural order support)
	var sortDirection="ascending";
	var isSortActive = node.getAttribute('sortActive');
	if (isSortActive == "true")
	{
		var currentDirection = node.getAttribute('sortDirection');
		if (currentDirection == "ascending")
		{
			sortDirection = "descending";
		}
	}

	try
	{
		var isupports = Components.classes["component://netscape/rdf/xul-sort-service"].getService();
		if (!isupports)    return(false);
		var xulSortService = isupports.QueryInterface(Components.interfaces.nsIXULSortService);
		if (!xulSortService)    return(false);
		xulSortService.Sort(node, sortResource, sortDirection);
	}
	catch(ex)
	{
	}
	return(false);
}
