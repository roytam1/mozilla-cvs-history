/* -*- Mode: Java; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Mozilla Communicator client code, released
 * March 31, 1998.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998-1999
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Seth Spitzer <sspitzer@netscape.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either of the GNU General Public License Version 2 or later (the "GPL"),
 * or the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

var gSearchSession = null;
var gPreQuickSearchView = null;
var gSearchTimer = null;
var gViewSearchListener;
var gSearchBundle;
var gStatusBar = null;
var gSearchInProgress = false;
var gSearchInput = null;
var gClearButton = null;
var gDefaultSearchViewTerms = null;
var gQSViewIsDirty = false;

function SetQSStatusText(aNumHits)
{
  var statusMsg;
  // if there are no hits, it means no matches were found in the search.
  if (aNumHits == 0)
    statusMsg = gSearchBundle.getString("searchFailureMessage");
  else 
  {
    if (aNumHits == 1) 
      statusMsg = gSearchBundle.getString("searchSuccessMessage");
    else
      statusMsg = gSearchBundle.getFormattedString("searchSuccessMessages", [aNumHits]);
  }

  statusFeedback.showStatusString(statusMsg);
}

// nsIMsgSearchNotify object
var gSearchNotificationListener =
{
    onSearchHit: function(header, folder)
    {
        // XXX todo
        // update status text?
    },

    onSearchDone: function(status)
    {
        SetQSStatusText(gDBView.QueryInterface(Components.interfaces.nsITreeView).rowCount)
        statusFeedback.showProgress(0);
        gStatusBar.setAttribute("mode","normal");
        gSearchInProgress = false;
    },

    onNewSearch: function()
    {
      statusFeedback.showProgress(0);
      statusFeedback.showStatusString(gSearchBundle.getString("searchingMessage"));
      gStatusBar.setAttribute("mode","undetermined");
      gSearchInProgress = true;
    }
}

function getDocumentElements()
{
  gSearchBundle = document.getElementById("bundle_search");  
  gStatusBar = document.getElementById('statusbar-icon');
  gClearButton = document.getElementById('clearButton');
  GetSearchInput();
}

function addListeners()
{
  gViewSearchListener = gDBView.QueryInterface(Components.interfaces.nsIMsgSearchNotify);
  gSearchSession.registerListener(gViewSearchListener);
}

function removeListeners()
{
  gSearchSession.unregisterListener(gViewSearchListener);
}

function removeGlobalListeners()
{
  removeListeners();
  gSearchSession.unregisterListener(gSearchNotificationListener); 
}

function initializeGlobalListeners()
{
  // Setup the javascript object as a listener on the search results
  gSearchSession.registerListener(gSearchNotificationListener);
}

function createQuickSearchView()
{
  if (gDBView.viewType != nsMsgViewType.eShowQuickSearchResults)  //otherwise we are already in quick search view
  {
    var treeView = gDBView.QueryInterface(Components.interfaces.nsITreeView);  //clear selection
    treeView.selection.clearSelection();
    gPreQuickSearchView = gDBView;
    CreateDBView(gDBView.msgFolder, nsMsgViewType.eShowQuickSearchResults, nsMsgViewFlagsType.kNone, gDBView.sortType, gDBView.sortOrder);
  }
}

function initializeSearchBar()
{
   createQuickSearchView();
   if (!gSearchSession)
   {
     getDocumentElements();
     var searchSessionContractID = "@mozilla.org/messenger/searchSession;1";
     gSearchSession = Components.classes[searchSessionContractID].createInstance(Components.interfaces.nsIMsgSearchSession);
     initializeGlobalListeners();
   }
   else
   {
     if (gSearchInProgress)
     {
       onSearchStop();
       gSearchInProgress = false;
     }
     removeListeners();
   }
   addListeners();
}

function onEnterInSearchBar()
{
   if (gSearchInput.value == "") 
   {
     if (gDBView.viewType == nsMsgViewType.eShowQuickSearchResults)
     {
       statusFeedback.showStatusString("");
       disableQuickSearchClearButton();

       if (gDefaultSearchViewTerms)
       {
         if (gQSViewIsDirty)
         {
           initializeSearchBar();
           onSearch(gDefaultSearchViewTerms);
         }
       }
       else
        restorePreSearchView();
     }
     
     gQSViewIsDirty = false;
     return;
   }

   initializeSearchBar();

   gClearButton.setAttribute("disabled", false); //coming into search enable clear button   

   ClearThreadPaneSelection();
   ClearMessagePane();

   onSearch(null);
   gQSViewIsDirty = false;
}

function restorePreSearchView()
{
  var selectedHdr = null;
  //save selection
  try 
  {
    selectedHdr = gDBView.hdrForFirstSelectedMessage;
  }
  catch (ex)
  {}

  //we might have to sort the view coming out of quick search
  var sortType = gDBView.sortType;
  var sortOrder = gDBView.sortOrder;
  var viewFlags = gDBView.viewFlags;
  var folder = gDBView.msgFolder;

  gDBView.close();
  gDBView = null; 

  if (gPreQuickSearchView)
  {
    gDBView = gPreQuickSearchView;

    if (sortType != gDBView.sortType || sortOrder != gDBView.sortOrder)
    {
      gDBView.sort(sortType, sortOrder);
    }
    UpdateSortIndicators(sortType, sortOrder);

    gPreQuickSearchView = null;    
  }
  else //create default view type
    CreateDBView(folder, nsMsgViewType.eShowAllThreads, viewFlags, sortType, sortOrder);

  RerootThreadPane();
   
  var scrolled = false;
  
  // now restore selection
  if (selectedHdr)
  {
    gDBView.selectMsgByKey(selectedHdr.messageKey);
    var treeView = gDBView.QueryInterface(Components.interfaces.nsITreeView);
    var selectedIndex = treeView.selection.currentIndex;
    if (selectedIndex >= 0) 
    {
      // scroll
      EnsureRowInThreadTreeIsVisible(selectedIndex);
      scrolled = true;
    }
    else
      ClearMessagePane();
  }

  // NOTE,
  // if you change the scrolling code below,
  // double check the scrolling logic in
  // msgMail3PaneWindow.js, "FolderLoaded" event code
  if (!scrolled)
  {
    // if we didn't just scroll, 
    // scroll to the first new message
    // but don't select it
    scrolled = ScrollToMessage(nsMsgNavigationType.firstNew, true, false /* selectMessage */);
    if (!scrolled) 
    {
      // if we still haven't scrolled,
      // scroll to the newest, which might be the top or the bottom
      // depending on our sort order and sort type
      if (sortOrder == nsMsgViewSortOrder.ascending) 
      {
        switch (sortType) 
        {
          case nsMsgViewSortType.byDate: 
          case nsMsgViewSortType.byId: 
          case nsMsgViewSortType.byThread: 
            scrolled = ScrollToMessage(nsMsgNavigationType.lastMessage, true, false /* selectMessage */);
            break;
        }
      }
      // if still we haven't scrolled,
      // scroll to the top.
      if (!scrolled)
        EnsureRowInThreadTreeIsVisible(0);
    }
  }
  // NOTE,
  // if you change the scrolling code above,
  // double check the scrolling logic in
  // msgMail3PaneWindow.js, "FolderLoaded" event code
}

function onSearch(aSearchTerms)
{
    RerootThreadPane();

    if (aSearchTerms)
      createSearchTermsWithList(aSearchTerms);
    else
      createSearchTerms();

    gDBView.searchSession = gSearchSession;
    try
    {
      gSearchSession.search(msgWindow);
    }
    catch(ex)
    {
      dump("Search Exception\n");
    }
}

function createSearchTermsWithList(aTermsArray)
{
  var nsMsgSearchScope = Components.interfaces.nsMsgSearchScope;
  var nsMsgSearchAttrib = Components.interfaces.nsMsgSearchAttrib;
  var nsMsgSearchOp = Components.interfaces.nsMsgSearchOp;

  gSearchSession.clearScopes();
  var searchTerms = gSearchSession.searchTerms;
  var searchTermsArray = searchTerms.QueryInterface(Components.interfaces.nsISupportsArray);
  searchTermsArray.Clear();

  var selectedFolder = GetThreadPaneFolder();
  gSearchSession.addScopeTerm(nsMsgSearchScope.offlineMail, selectedFolder);

  // add each item in termsArray to the search session

  var termsArray = aTermsArray.QueryInterface(Components.interfaces.nsISupportsArray);
  for (var i = 0; i < termsArray.Count(); i++)
    gSearchSession.appendTerm(termsArray.GetElementAt(i).QueryInterface(Components.interfaces.nsIMsgSearchTerm));
}

function createSearchTerms()
{
  var nsMsgSearchScope = Components.interfaces.nsMsgSearchScope;
  var nsMsgSearchAttrib = Components.interfaces.nsMsgSearchAttrib;
  var nsMsgSearchOp = Components.interfaces.nsMsgSearchOp;

  // create an i supports array to store our search terms 
  var searchTermsArray = Components.classes["@mozilla.org/supports-array;1"].createInstance(Components.interfaces.nsISupportsArray);
  var selectedFolder = GetThreadPaneFolder();

  var searchAttrib = (IsSpecialFolder(selectedFolder, MSG_FOLDER_FLAG_SENTMAIL | MSG_FOLDER_FLAG_DRAFTS | MSG_FOLDER_FLAG_QUEUE)) ? nsMsgSearchAttrib.ToOrCC : nsMsgSearchAttrib.Sender;
  // implement | for QS
  // does this break if the user types "foo|bar" expecting to see subjects with that string?
  // I claim no, since "foo|bar" will be a hit for "foo" || "bar"
  // they just might get more false positives
  var termList = gSearchInput.value.split("|");
  for (var i = 0; i < termList.length; i ++)
  {
    // if the term is empty, skip it
    if (termList[i] == "")
      continue;

    // create, fill, and append the subject term
    var term = gSearchSession.createTerm();
    var value = term.value;
    value.str = termList[i];
    term.value = value;
    term.attrib = nsMsgSearchAttrib.Subject;
    term.op = nsMsgSearchOp.Contains;
    term.booleanAnd = false;
    searchTermsArray.AppendElement(term);

    // create, fill, and append the sender (or recipient) term
    term = gSearchSession.createTerm();
    value = term.value;
    value.str = termList[i];
    term.value = value;
    term.attrib = searchAttrib;
    term.op = nsMsgSearchOp.Contains; 
    term.booleanAnd = false;
    searchTermsArray.AppendElement(term);
  }

  // now append the default view criteria to the quick search so we don't lose any default
  // view information
  if (gDefaultSearchViewTerms)
  {
    var isupports = null;
    var searchTerm; 
    var termsArray = gDefaultSearchViewTerms.QueryInterface(Components.interfaces.nsISupportsArray);
    for (i = 0; i < termsArray.Count(); i++)
    {
      isupports = termsArray.GetElementAt(i);
      searchTerm = isupports.QueryInterface(Components.interfaces.nsIMsgSearchTerm);
      searchTermsArray.AppendElement(searchTerm);
    }
  }
  
  createSearchTermsWithList(searchTermsArray);
  
  // now that we've added the terms, clear out our input array
  searchTermsArray.Clear();
}

function onAdvancedSearch()
{
  MsgSearchMessages();
}

function onSearchStop() 
{
  gSearchSession.interruptSearch();
}

function onSearchKeyPress(event)
{
  // 13 == return
  if (event && event.keyCode == 13)
    onSearchInput(true);
}

function onSearchInput(returnKeyHit)
{
  if (gSearchTimer) {
    clearTimeout(gSearchTimer); 
    gSearchTimer = null;
  }

  // only select the text when the return key was hit
  if (returnKeyHit) {
    GetSearchInput();
    gSearchInput.select();
    onEnterInSearchBar();
  }
  else {
    gSearchTimer = setTimeout("onEnterInSearchBar();", 800);
  }
}

function onClearSearch()
{
  var focusedElement = gLastFocusedElement;  //save of the last focused element so that focus can be restored
  Search("");
  focusedElement.focus();
}

function disableQuickSearchClearButton()
{
 if (gClearButton)
   gClearButton.setAttribute("disabled", true); //going out of search disable clear button
}

function ClearQSIfNecessary()
{
  GetSearchInput();

  if (gSearchInput.value == "")
    return;

  Search("");
}

function Search(str)
{
  GetSearchInput();

  if (str != gSearchInput.value)
    gQSViewIsDirty = true; 

  gSearchInput.value = str;  //on input does not get fired for some reason
  onSearchInput(true);
}
