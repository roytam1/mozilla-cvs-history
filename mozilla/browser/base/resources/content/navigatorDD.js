/* -*- Mode: Java; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: NPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is 
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *  - Kevin Puetz (puetzk@iastate.edu)
 *  - Ben Goodger <ben@netscape.com> 
 *  - Blake Ross <blaker@netscape.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or 
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the NPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

var DROP_BEFORE = -1;
var DROP_ON = 0;
var DROP_AFTER = 1;
function _RDF(aType)
  {
    return "http://www.w3.org/1999/02/22-rdf-syntax-ns#" + aType;
  }
function NC_RDF(aType)
  {
    return "http://home.netscape.com/NC-rdf#" + aType;
  }

var RDFUtils = {
  getResource: function(aString)
    {
      return this.rdf.GetResource(aString, true);
    },

  getTarget: function(aDS, aSourceID, aPropertyID)
    {
      var source = this.getResource(aSourceID);
      var property = this.getResource(aPropertyID);
      return aDS.GetTarget(source, property, true);
    },

  getValueFromResource: function(aResource)
    {
      aResource = aResource.QueryInterface(Components.interfaces.nsIRDFResource);
      return aResource ? aResource.Value : null;
    },
  _rdf: null,
  get rdf() {
    if (!this._rdf) {
      this._rdf = Components.classes["@mozilla.org/rdf/rdf-service;1"]
                            .getService(Components.interfaces.nsIRDFService);
    }
    return this._rdf;
  }
};

function isBookmark(aURI)
  {
    var db = document.getElementById("innermostBox").database;
    var typeValue = RDFUtils.getTarget(db, aURI, _RDF("type"));
    typeValue = RDFUtils.getValueFromResource(typeValue);
    return (typeValue == NC_RDF("BookmarkSeparator") ||
            typeValue == NC_RDF("Bookmark") ||
            typeValue == NC_RDF("Folder"))
  }

var personalToolbarObserver = {
  onDragStart: function (aEvent, aXferData, aDragAction)
    {
      // Prevent dragging out of menus on non Win32 platforms. 
      // a) on Mac drag from menus is generally regarded as being satanic
      // b) on Linux, this causes an X-server crash, see bug 79003. 
      // Since we're not doing D&D into menus properly at this point, it seems
      // fair enough to disable it on non-Win32 platforms. There is no hang
      // or crash associated with this on Windows, so we'll leave the functionality
      // there. 
      if (navigator.platform != "Win32" && aEvent.target.localName != "toolbarbutton")
        return;


      if (aEvent.target.localName == "menu" || 
         (aEvent.target.localName == "toolbarbutton" && aEvent.target.getAttribute("type") == "menu")) {
        if (aEvent.target.getAttribute("type") == "http://home.netscape.com/NC-rdf#Folder") {
          var child = aEvent.target.childNodes[0];                               
          if (child && child.localName == "menupopup")                                     
            child.hidePopup();                                                  
          else {                                                                 
            var parent = aEvent.target.parentNode;                               
            if (parent && parent.localName == "menupopup")                                 
            parent.hidePopup();                                               
          }                                                                      
        }
      }                                                                         

      var personalToolbar = document.getElementById("PersonalToolbar");
      if (aEvent.target == personalToolbar) return;

      var db = document.getElementById("innermostBox").database;
      var uri = aEvent.target.id;
      if (!isBookmark(uri)) return;
      var url = RDFUtils.getTarget(db, uri, NC_RDF("URL"));
      if (url)
        url = url.QueryInterface(Components.interfaces.nsIRDFLiteral).Value;
      else
        url = "";
      var name = RDFUtils.getTarget(db, uri, NC_RDF("Name"));
      if (name)
        name = name.QueryInterface(Components.interfaces.nsIRDFLiteral).Value;
      else
        name = "";
      var urlString = url + "\n" + name;
      var htmlString = "<A HREF='" + uri + "'>" + name + "</A>";
      aXferData.data = new TransferData();
      aXferData.data.addDataForFlavour("moz/rdfitem", uri);
      aXferData.data.addDataForFlavour("text/x-moz-url", urlString);
      aXferData.data.addDataForFlavour("text/html", htmlString);
      aXferData.data.addDataForFlavour("text/unicode", url);
    },

  onDrop: function (aEvent, aXferData, aDragSession)
    {
      var xferData = aXferData.data.split("\n");
      var elementRes = RDFUtils.getResource(xferData[0]);
      var personalToolbarRes = RDFUtils.getResource("NC:PersonalToolbarFolder");

      var inner = document.getElementById("innermostBox");
      var childDB = inner.database;
      const kCtrContractID = "@mozilla.org/rdf/container;1";
      const kCtrIID = Components.interfaces.nsIRDFContainer;
      var rdfContainer = Components.classes[kCtrContractID].createInstance(kCtrIID);

      // Ensure that the Personal Toolbar Folder actually exists...
      try {
        rdfContainer.Init(childDB, personalToolbarRes);
      }
      catch (e) {
        // No Personal Toolbar Folder, we must create a new one.
        const kBMSContractID = "@mozilla.org/browser/bookmarks-service;1";
        const kBMSIID = Components.interfaces.nsIBookmarksService;
        var bms = Components.classes[kBMSContractID].getService(kBMSIID);
        if (bms) {
          var bookmarksRoot = RDFUtils.getResource("NC:BookmarksRoot");
          var bookmarksBundle = document.getElementById("bookmarksbundle");
          var ptFolderName = bookmarksBundle.getString("DefaultPersonalToolbarFolder");
          var ptFolder = bms.createFolder(ptFolderName, bookmarksRoot);
          BookmarksUtils.doBookmarksCommand(ptFolder.Value, NC_NS_CMD + "setpersonaltoolbarfolder", []);
        }
      }

      // if dragged url is already bookmarked, remove it from current location first
      var parentContainer = findParentContainer(aDragSession.sourceNode);
      if (parentContainer)
        {
          rdfContainer.Init(childDB, parentContainer);
          rdfContainer.RemoveElement(elementRes, false);
        }

      // determine charset of link
      var linkCharset = aDragSession.sourceDocument ? aDragSession.sourceDocument.characterSet : null;
      // determine title of link
      var linkTitle;
      
      // look it up in bookmarks
      var bookmarksDS = RDFUtils.rdf.GetDataSource("rdf:bookmarks");
      var nameRes = RDFUtils.getResource(NC_RDF("Name"));
      var nameFromBookmarks = bookmarksDS.GetTarget(elementRes, nameRes, true);
      if (nameFromBookmarks)
        nameFromBookmarks = nameFromBookmarks.QueryInterface(Components.interfaces.nsIRDFLiteral);
      if (nameFromBookmarks) {
        linkTitle = nameFromBookmarks.Value;
      }
      else if (xferData.length >= 2)
        linkTitle = xferData[1]
      else
        {
          // look up this URL's title in global history
          var potentialTitle = null;
          var historyDS = RDFUtils.rdf.GetDataSource("rdf:history");
          var titlePropRes = RDFUtils.getResource(NC_RDF("Name"));
          var titleFromHistory = historyDS.GetTarget(elementRes, titlePropRes, true);
          if (titleFromHistory)
            titleFromHistory = titleFromHistory.QueryInterface(Components.interfaces.nsIRDFLiteral);
          if (titleFromHistory)
            potentialTitle = titleFromHistory.Value;
          linkTitle = potentialTitle;
        }

      var dropElement = aEvent.target.id;
      var dropElementRes, dropIndex, dropPosition;
      if (dropElement == "innermostBox") 
        {
          dropElementRes = personalToolbarRes;
          dropPosition = DROP_ON;
        }
      else
        {
          dropElementRes = RDFUtils.getResource(dropElement);
          rdfContainer.Init(childDB, personalToolbarRes);
          dropIndex = rdfContainer.IndexOf(dropElementRes);
          if (dropPosition == undefined)
            dropPosition = determineDropPosition(aEvent, true);
        }
      
      switch (dropPosition) {
      case DROP_BEFORE:
        if (dropIndex<1) dropIndex = 1;
        insertBookmarkAt(xferData[0], linkTitle, linkCharset, personalToolbarRes, dropIndex);
        break;
      case DROP_ON:
        insertBookmarkAt(xferData[0], linkTitle, linkCharset, dropElementRes, -1);
        break;
      case DROP_AFTER:
      default:
        // compensate for badly calculated dropIndex
        rdfContainer.Init(childDB, personalToolbarRes);
        if (dropIndex < rdfContainer.GetCount()) ++dropIndex;
        
        if (dropIndex<0) dropIndex = 0;
        insertBookmarkAt(xferData[0], linkTitle, linkCharset, personalToolbarRes, dropIndex);
        break;
      }
      
      return true;
    },

  mCurrentDragOverButton: null,
  mCurrentDragPosition: null,

  onDragExit: function (aEvent, aDragSession)
    {
      if (this.mCurrentDragOverButton)
        {
          this.mCurrentDragOverButton.removeAttribute("dragover-left");
          this.mCurrentDragOverButton.removeAttribute("dragover-right");
          this.mCurrentDragOverButton.removeAttribute("dragover-top");
          this.mCurrentDragOverButton.removeAttribute("open");
        }
    },

  onDragOver: function (aEvent, aFlavour, aDragSession)
    {
       var dropPosition = determineDropPosition(aEvent, true);

      // bail if drop target is not a valid bookmark item or folder
      var inner = document.getElementById("innermostBox");
      if (aEvent.target.parentNode != inner && aEvent.target != inner) 
        {
          aDragSession.canDrop = false;
          return false;
        }
      
      if (this.mCurrentDragOverButton != aEvent.target ||
          (this.mCurrentDragOverButton == aEvent.target &&
           this.mCurrentDragPosition != dropPosition))
        {
          if (this.mCurrentDragOverButton)
            {
              this.mCurrentDragOverButton.removeAttribute("dragover-left");
              this.mCurrentDragOverButton.removeAttribute("dragover-right");
              this.mCurrentDragOverButton.removeAttribute("dragover-top");
              this.mCurrentDragOverButton.removeAttribute("open");
            }
          this.mCurrentDragOverButton = aEvent.target;
          this.mCurrentDragPosition = dropPosition;
        }

      switch (dropPosition)
        {
          case DROP_BEFORE: 
            aEvent.target.setAttribute("dragover-left", "true");
            break;
          case DROP_AFTER:
            aEvent.target.setAttribute("dragover-right", "true");
            break;
          case DROP_ON:
          default:
            if (aEvent.target.getAttribute("container") == "true") {
              aEvent.target.setAttribute("dragover-top", "true");
              //cant open a menu during a drag! suck!
              //aEvent.target.setAttribute("open", "true");
            }
            break;
        }

       return true;
    },

  getSupportedFlavours: function ()
    {
      var flavourSet = new FlavourSet();
      flavourSet.appendFlavour("moz/rdfitem");
      // application/x-moz-file
      flavourSet.appendFlavour("text/unicode");
      return flavourSet;
    }
};

var proxyIconDNDObserver = {
  onDragStart: function (aEvent, aXferData, aDragAction)
    {
      var urlBar = document.getElementById("urlbar");

      // XXX - do we want to allow the user to set a blank page to their homepage?
      //       if so then we want to modify this a little to set about:blank as
      //       the homepage in the event of an empty urlbar.
      if (!urlBar.value) return;

      var urlString = urlBar.value + "\n" + window._content.document.title;
      var htmlString = "<a href=\"" + urlBar.value + "\">" + urlBar.value + "</a>";

      aXferData.data = new TransferData();
      aXferData.data.addDataForFlavour("text/x-moz-url", urlString);
      aXferData.data.addDataForFlavour("text/unicode", urlBar.value);
      aXferData.data.addDataForFlavour("text/html", htmlString);
    }
};

var homeButtonObserver = {
  onDragStart: function (aEvent, aXferData, aDragAction)
    {
      var homepage = nsPreferences.getLocalizedUnicharPref("browser.startup.homepage", "about:blank");

      if (homepage)
        {
          // XXX find a readable title string for homepage, perhaps do a history lookup.
          var htmlString = "<a href=\"" + homepage + "\">" + homepage + "</a>";
          aXferData.data = new TransferData();
          aXferData.data.addDataForFlavour("text/x-moz-url", homepage + "\n" + homepage);
          aXferData.data.addDataForFlavour("text/html", htmlString);
          aXferData.data.addDataForFlavour("text/unicode", homepage);
        }
    },

  onDrop: function (aEvent, aXferData, aDragSession)
    {
      var url = retrieveURLFromData(aXferData.data, aXferData.flavour.contentType);
      setTimeout(openHomeDialog, 0, url);
    },

  onDragOver: function (aEvent, aFlavour, aDragSession)
    {
      var statusTextFld = document.getElementById("statusbar-display");
      statusTextFld.label = gNavigatorBundle.getString("droponhomebutton");
      aDragSession.dragAction = Components.interfaces.nsIDragService.DRAGDROP_ACTION_LINK;
    },

  onDragExit: function (aEvent, aDragSession)
    {
      var statusTextFld = document.getElementById("statusbar-display");
      statusTextFld.label = "";
    },

  getSupportedFlavours: function ()
    {
      var flavourSet = new FlavourSet();
      flavourSet.appendFlavour("application/x-moz-file", "nsIFile");
      flavourSet.appendFlavour("text/x-moz-url");
      flavourSet.appendFlavour("text/unicode");
      return flavourSet;
    }
};

function openHomeDialog(aURL)
{
  var promptService = Components.classes["@mozilla.org/embedcomp/prompt-service;1"].getService(Components.interfaces.nsIPromptService);
  var pressedVal = { };
  var promptTitle = gNavigatorBundle.getString("droponhometitle");
  var promptMsg   = gNavigatorBundle.getString("droponhomemsg");
  var okButton    = gNavigatorBundle.getString("droponhomeokbutton");

  promptService.confirmEx(window, promptTitle, promptMsg,
                          (promptService.BUTTON_TITLE_IS_STRING * promptService.BUTTON_POS_0) +
                          (promptService.BUTTON_TITLE_CANCEL * promptService.BUTTON_POS_1),
                          okButton, null, null, null, {value:0}, pressedVal);

  if (pressedVal.value == 0) {
    nsPreferences.setUnicharPref("browser.startup.homepage", aURL);
    setTooltipText("home-button", aURL);
  }
}

var goButtonObserver = {
  onDragOver: function(aEvent, aFlavour, aDragSession)
    {
      aEvent.target.setAttribute("dragover", "true");
      return true;
    },
  onDragExit: function (aEvent, aDragSession)
    {
      aEvent.target.removeAttribute("dragover");
    },
  onDrop: function (aEvent, aXferData, aDragSession)
    {
      var xferData = aXferData.data.split("\n");
      var uri = xferData[0] ? xferData[0] : xferData[1];
      if (uri)
        loadURI(uri);
    },
  getSupportedFlavours: function ()
    {
      var flavourSet = new FlavourSet();
      flavourSet.appendFlavour("application/x-moz-file", "nsIFile");
      flavourSet.appendFlavour("text/x-moz-url");
      flavourSet.appendFlavour("text/unicode");
      return flavourSet;
    }
};

var searchButtonObserver = {
  onDragOver: function(aEvent, aFlavour, aDragSession)
    {
      aEvent.target.setAttribute("dragover", "true");
      return true;
    },
  onDragExit: function (aEvent, aDragSession)
    {
      aEvent.target.removeAttribute("dragover");
    },
  onDrop: function (aEvent, aXferData, aDragSession)
    {
      var xferData = aXferData.data.split("\n");
      var uri = xferData[1] ? xferData[1] : xferData[0];
      if (uri)
        OpenSearch('internet',false, uri);
    },
  getSupportedFlavours: function ()
    {
      var flavourSet = new FlavourSet();
      flavourSet.appendFlavour("application/x-moz-file", "nsIFile");
      flavourSet.appendFlavour("text/x-moz-url");
      flavourSet.appendFlavour("text/unicode");
      return flavourSet;
    }
};
var gOpenFolder = null;
var folderObserver = {
  onDragOver: function(aEvent, aFlavour, aDragSession)
    {
      if (aEvent.target.getAttribute("open") == "true")
        return false;

      aEvent.target.setAttribute("dragover", "true");
      aEvent.target.firstChild.showPopup(aEvent.target, -1, -1, "menupopup", "bottomleft", "bottomleft");
      return true;
    },
  getSupportedFlavours: function ()
    {
      var flavourSet = new FlavourSet();
      flavourSet.appendFlavour("application/x-moz-file", "nsIFile");
      flavourSet.appendFlavour("text/x-moz-url");
      return flavourSet;
    }
};

var gCurrentTarget = null;
var gCurrentDragOverMenu = null;
function closeOpenMenu()
{
  if (gCurrentDragOverMenu && gCurrentTarget.firstChild != gCurrentDragOverMenu) {
    if (gCurrentTarget.parentNode != gCurrentDragOverMenu) {
      gCurrentDragOverMenu.hidePopup();
      gCurrentDragOverMenu = null;
    }
  }
}
        
var menuDNDObserver = {
  onDragOver: function(aEvent, aFlavour, aDragSession) 
  {
    // if we're a folder just one level deep, open it
    var isOneLevelDeep = aEvent.target.parentNode.parentNode.getAttribute("open") == "true" && aEvent.target.parentNode.parentNode.localName == "toolbarbutton";
    var dropPosition = determineDropPosition(aEvent, !isOneLevelDeep);
    gCurrentTarget = aEvent.target;
    if (aEvent.target.firstChild && aEvent.target.firstChild.localName == "menupopup") {
      if (isOneLevelDeep) {
        if (gCurrentDragOverMenu && gCurrentDragOverMenu != aEvent.target.firstChild)
          gCurrentDragOverMenu.hidePopup();
        if (!gCurrentDragOverMenu) {
          aEvent.target.firstChild.showPopup(aEvent.target, -1, -1, "menupopup", "topright, topright");
          gCurrentDragOverMenu = aEvent.target.firstChild;
        }
      }
      else {
        aEvent.target.setAttribute("menuactive", "true"); 
      }
    }
  
    // remove drag attributes from old item once we move to a new item
    if (this.mCurrentDragOverItem != aEvent.target) {
      if (this.mCurrentDragOverItem) {
        this.mCurrentDragOverItem.removeAttribute("dragover-top");
        this.mCurrentDragOverItem.removeAttribute("dragover-bottom");
        this.mCurrentDragOverItem.removeAttribute("menuactive");
      }
      this.mCurrentDragOverItem = aEvent.target;
    }
    
    // if there's an open submenu and we're not over it or one of its children, close it
    if (gCurrentDragOverMenu && aEvent.target.firstChild != gCurrentDragOverMenu) {
      if (aEvent.target.parentNode != gCurrentDragOverMenu) {
        setTimeout(function() { closeOpenMenu(); },500);
      }
    }

    // ensure appropriate feedback
    switch (dropPosition) {
      case DROP_BEFORE: 
        aEvent.target.setAttribute("dragover-bottom", "true");
        break;
      case DROP_AFTER:
        aEvent.target.setAttribute("dragover-top", "true");
        break;
    }
  },
  mCurrentDragOverItem: null,
  onDragExit: function (aEvent, aDragSession)
  {
    // remove drag attribute from current item once we leave the popup
    if (this.mCurrentDragOverItem) {
      this.mCurrentDragOverItem.removeAttribute("dragover-top");
      this.mCurrentDragOverItem.removeAttribute("dragover-bottom");
    }
  },
  onDrop: function (aEvent, aXferData, aDragSession)
  {
    var xferData = aXferData.data.split("\n");
    var elementRes = RDFUtils.getResource(xferData[0]);

    var bookmarksButton = document.getElementById("bookmarks-button");
    var childDB = bookmarksButton.database;
    var rdfContainer = Components.classes["@mozilla.org/rdf/container;1"].createInstance(Components.interfaces.nsIRDFContainer);

    // if dragged url is already bookmarked, remove it from current location first
    var parentContainer = findParentContainer(aDragSession.sourceNode);
    if (parentContainer) {
      rdfContainer.Init(childDB, parentContainer);
      rdfContainer.RemoveElement(elementRes, false);
    }
    parentContainer = findParentContainer(aEvent.target);
    // determine charset of link
    var linkCharset = aDragSession.sourceDocument ? aDragSession.sourceDocument.characterSet : null;
    // determine title of link
    var linkTitle;
    // look it up in bookmarks
    var bookmarksDS = RDFUtils.rdf.GetDataSource("rdf:bookmarks");
    var nameRes = RDFUtils.getResource(NC_RDF("Name"));
    var nameFromBookmarks = bookmarksDS.GetTarget(elementRes, nameRes, true);
    if (nameFromBookmarks)
      nameFromBookmarks = nameFromBookmarks.QueryInterface(Components.interfaces.nsIRDFLiteral);

    if (nameFromBookmarks)
      linkTitle = nameFromBookmarks.Value;
    else if (xferData.length >= 2)
      linkTitle = xferData[1]
    else {
      // look up this URL's title in global history
      var historyDS = RDFUtils.rdf.GetDataSource("rdf:history");
      var titlePropRes = RDFUtils.getResource(NC_RDF("Name"));
      var titleFromHistory = historyDS.GetTarget(elementRes, titlePropRes, true);
      if (titleFromHistory)
        titleFromHistory = titleFromHistory.QueryInterface(Components.interfaces.nsIRDFLiteral);
      if (titleFromHistory)
        linkTitle = titleFromHistory.Value;
    }

    var dropElement = aEvent.target.id;
    var dropElementRes, dropIndex, dropPosition;
    dropElementRes = RDFUtils.getResource(dropElement);
    rdfContainer.Init(childDB, parentContainer);
    dropIndex = rdfContainer.IndexOf(dropElementRes);
    var isOneLevelDeep = aEvent.target.parentNode.parentNode.getAttribute("open") == "true" && aEvent.target.parentNode.parentNode.localName == "toolbarbutton";
    dropPosition = determineDropPosition(aEvent, !isOneLevelDeep);
    switch (dropPosition) {
      case DROP_BEFORE:
        --dropIndex;
        if (dropIndex<1) dropIndex = 1;
          insertBookmarkAt(xferData[0], linkTitle, linkCharset, parentContainer, dropIndex);
        break;
      case DROP_ON:
        insertBookmarkAt(xferData[0], linkTitle, linkCharset, dropElementRes, -1);
        break;
      case DROP_AFTER:
      default:
        // compensate for badly calculated dropIndex
        if (dropIndex < rdfContainer.GetCount()) ++dropIndex;
         
        if (dropIndex<0) dropIndex = 0;
        --dropIndex;
        insertBookmarkAt(xferData[0], linkTitle, linkCharset, parentContainer, dropIndex);
        break;
    }
    
    // if user isn't rearranging within the menu, close it
    if (aDragSession.sourceNode.localName != "menuitem" && aDragSession.sourceNode.localName != "menu")
      setTimeout(function() { if (gCurrentDragOverMenu) gCurrentDragOverMenu.hidePopup(); document.getElementById("bookmarks-button").firstChild.hidePopup(); gDidOpen = false; }, 190);    
    
    return true;
  },

  getSupportedFlavours: function () {
    var flavourSet = new FlavourSet();
    flavourSet.appendFlavour("application/x-moz-file", "nsIFile");
    flavourSet.appendFlavour("text/x-moz-url");
    return flavourSet;
  }
};

function determineDropPosition(aEvent, aAllowDropOn)
{
  var overButtonBoxObject = aEvent.target.boxObject.QueryInterface(Components.interfaces.nsIBoxObject);
  // most things only drop on the left or right
  var regionCount = 2;

  // you can drop ONTO containers, so there is a "middle" region
  if (aAllowDropOn && aEvent.target.getAttribute("container") == "true" &&
      aEvent.target.getAttribute("type") == "menu")
    return DROP_ON;
      
  var measure;
  var coordValue;
  var clientCoordValue;
  if (aEvent.target.localName == "menuitem" || aEvent.target.localName == "menu") {
    measure = overButtonBoxObject.height/regionCount;
    coordValue = overButtonBoxObject.y;
    clientCoordValue = aEvent.clientY;
  }
  else if (aEvent.target.localName == "toolbarbutton") {
    measure = overButtonBoxObject.width/regionCount;
    coordValue = overButtonBoxObject.x;
    clientCoordValue = aEvent.clientX;
  }
  else
    return 0;

      
  // in the first region?
  if (clientCoordValue < (coordValue + measure))
    return DROP_BEFORE;

  // in the last region?
  if (clientCoordValue >= (coordValue + (regionCount - 1)*measure))
    return DROP_AFTER;

  // must be in the middle somewhere
  return DROP_ON;
}

// returns the parent resource of the dragged element. This is determined
// by inspecting the source element of the drag and walking up the DOM tree
// to find the appropriate containing node.
function findParentContainer(aElement)
{
  if (!aElement) return null;
  switch (aElement.localName) {
    case "toolbarbutton":
      var box = aElement.parentNode;
      return RDFUtils.getResource(box.getAttribute("ref"));
    case "menu":
    case "menuitem":
      var parentNode = aElement.parentNode.parentNode;
     
      if (parentNode.getAttribute("type") != NC_RDF("Folder") &&
          parentNode.getAttributeNS("http://www.w3.org/1999/02/22-rdf-syntax-ns#", "type") != "http://home.netscape.com/NC-rdf#Folder")
        return RDFUtils.getResource("NC:BookmarksRoot");
      return RDFUtils.getResource(parentNode.id);
    case "treecell":
      var treeitem = aElement.parentNode.parentNode.parentNode.parentNode;
      var res = treeitem.getAttribute("ref");
      if (!res)
        res = treeitem.id;            
      return RDFUtils.getResource(res);
  }
  return null;
}

function insertBookmarkAt(aURL, aTitle, aCharset, aFolderRes, aIndex)
{
  const kBMSContractID = "@mozilla.org/browser/bookmarks-service;1";
  const kBMSIID = Components.interfaces.nsIBookmarksService;
  const kBMS = Components.classes[kBMSContractID].getService(kBMSIID);
  kBMS.createBookmarkWithDetails(aTitle, aURL, aCharset, aFolderRes, aIndex);
}

