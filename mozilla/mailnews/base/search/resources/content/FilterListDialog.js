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
 */

var rdf;

var editButton;
var deleteButton;

function onLoad()
{
    rdf = Components.classes["component://netscape/rdf/rdf-service"].getService(Components.interfaces.nsIRDFService);

    editButton = document.getElementById("editButton");
    deleteButton = document.getElementById("deleteButton");

    updateButtons();
    
    var firstitem;
    var args = window.arguments;
    if (args && args[0])
        firstItem = args[0].initialServerUri;
    
    else {
        var serverMenu = document.getElementById("serverMenu");
        var menuitems = serverMenu.getElementsByTagName("menuitem");
        firstItem = menuitems[1].id;
    }
    
    selectServer(firstItem);
}

function onServerClick(event)
{
    var item = event.target;
    setServer(item.id);
    updateButtons();
}

// roots the tree at the specified server
function setServer(uri)
{
    var tree = document.getElementById("filterTree");
    tree.setAttribute("ref", uri);
}

// sets up the menulist and the tree
function selectServer(uri)
{
    // update the server menu
    var serverMenu = document.getElementById("serverMenu");
    var menuitems = serverMenu.getElementsByAttribute("id", uri);
    
    serverMenu.selectedItem = menuitems[0];
    
    setServer(uri);
}

function currentFilter()
{
    var selection = document.getElementById("filterTree").selectedItems;
    if (!selection || selection.length <=0)
        return null;

    var filter;
    try {
        var filterResource = rdf.GetResource(selection[0].id);
        filter = filterResource.GetDelegate("filter",
                                            Components.interfaces.nsIMsgFilter);
    } catch (ex) {
        dump(ex);
        dump("no filter selected!\n");
    }
    return filter;
}

function currentFilterList()
{
    var serverMenu = document.getElementById("serverMenu");
    var serverUri = serverMenu.data;

    var filterList = rdf.GetResource(serverUri).GetDelegate("filter", Components.interfaces.nsIMsgFilterList);

    return filterList;
}

function onFilterSelect(event)
{
    updateButtons();
}

function EditFilter() {

    var selectedFilter = currentFilter();

    var args = {filter: selectedFilter};
    
    window.openDialog("chrome://messenger/content/FilterEditor.xul", "FilterEditor", "chrome,modal", args);

    if (args.refresh)
        refreshFilterList();
}

function NewFilter()
{
    var curFilterList = currentFilterList();
    var args = {filterList: curFilterList };
    
  window.openDialog("chrome://messenger/content/FilterEditor.xul", "FilterEditor", "chrome,modal", args);

  if (args.refresh) refreshFilterList();
  
}

function refreshFilterList() {
    var tree = document.getElementById("filterTree");
    if (!tree) return;

    tree.setAttribute("ref", tree.getAttribute("ref"));
}

function updateButtons()
{
    var filter = currentFilter();
    if (filter) {
        editButton.removeAttribute("disabled");
        deleteButton.removeAttribute("disabled");
    } else {
        editButton.setAttribute("disabled", "true");
        deleteButton.setAttribute("disabled", "true");
    }                      
}
