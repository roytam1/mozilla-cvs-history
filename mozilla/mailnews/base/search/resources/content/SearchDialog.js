/* -*- Mode: Java; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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

var rdfDatasourcePrefix = "component://netscape/rdf/datasource?name=";
var rdfServiceProgID    = "component://netscape/rdf/rdf-service";
var searchSessionProgID = "component://netscape/messenger/searchSession";
var gSearchDatasource;

var nsIMsgFolder = Components.interfaces.nsIMsgFolder;
var nsMsgSearchScope = Components.interfaces.nsMsgSearchScope;

var gFolderPicker;
var gStatusFeedback = new nsMsgStatusFeedback;
var RDF;

function searchOnLoad()
{
    initializeSearchWidgets();
    initializeSearchWindowWidgets();

    setupDatasource();
    onMore(null);

    
}

function initializeSearchWindowWidgets()
{
    gFolderPicker = document.getElementById("searchableFolders");
    gResultsTree = document.getElementById("threadTree");

    msgWindow = Components.classes[msgWindowProgID].createInstance(Components.interfaces.nsIMsgWindow);
    msgWindow.statusFeedback = gStatusFeedback;
}


function onSearchStop() {

}

function onReset() {

}

function onChooseFolder(event) {
    dump("event.target = " + event.target.id + "\n");

    SetFolderPicker(event.target.id, gFolderPicker.id);

    // use the URI to get the real folder
    gCurrentFolder =
        RDF.GetResource(event.target.id).QueryInterface(nsIMsgFolder);

    
    setSearchScope(GetScopeForFolder(gCurrentFolder));

}

function onSearch(event)
{
    gSearchSession.clearScopes();
    // tell the search session what the new scope is
    gSearchSession.addScopeTerm(GetScopeForFolder(gCurrentFolder),
                                gCurrentFolder);
    
    saveSearchTerms(gSearchSession.searchTerms, gSearchSession);

    gSearchSession.search(msgWindow);
}


function GetScopeForFolder(folder) {
    if (folder.server.type == "nntp")
        return nsMsgSearchScope.Newsgroup;
    else
        return nsMsgSearchScope.MailFolder;
}
    
function setupDatasource() {

    RDF = Components.classes[rdfServiceProgID].getService(Components.interfaces.nsIRDFService);
    
    gSearchDatasource = Components.classes[rdfDatasourcePrefix + "msgsearch"].createInstance(Components.interfaces.nsIRDFDataSource);

    dump("The root is " + gSearchDatasource.URI + "\n");
    gResultsTree.setAttribute("ref", gSearchDatasource.URI);
    
    // the thread pane needs to use the search datasource (to get the
    // actual list of messages) and the message datasource (to get any
    // attributes about each message)
    gSearchSession = Components.classes[searchSessionProgID].createInstance(Components.interfaces.nsIMsgSearchSession);
    
    gResultsTree.database.AddDataSource(gSearchDatasource);

    var messageDatasource = Components.classes[rdfDatasourcePrefix + "mailnewsmessages"].createInstance(Components.interfaces.nsIRDFDataSource);
    gResultsTree.database.AddDataSource(messageDatasource);
    
    // the datasource is a listener on the search results
    searchListener = gSearchDatasource.QueryInterface(Components.interfaces.nsIMsgSearchNotify);
    gSearchSession.registerListener(searchListener);

}


// this is test stuff only, ignore for now
function onTesting(event)
{
    var testattr;
    
    DumpDOM(document.getElementById("searchTermTree"));
    testattr = document.getElementById("searchAttr");
    testelement(testattr);

    testattr = document.getElementById("searchAttr0");
    testelement(testattr);
    
    testattr = document.getElementById("searchAttr99");
    testelement(testattr);

}

function testelement(element)
{
    dump(element.id + " = " + element + "\n");
    dump(element.id + ".searchScope = " + element.searchScope + "\n");
    element.searchScope = 0;
    dump(element.id + ".searchScope = " + element.searchScope + "\n");

}
