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
 *   Ben Goodger <ben@netscape.com> (Original Author)
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

var NC_NS  = "http://home.netscape.com/NC-rdf#";
var RDF_NS = "http://www.w3.org/1999/02/22-rdf-syntax-ns#";
const NC_NS_CMD = NC_NS + "command?cmd=";

/**
 * XXX - 04/16/01
 *  ACK! massive command name collision problems are causing big issues
 *  in getting this stuff to work in the Navigator window. For sanity's 
 *  sake, we need to rename all the commands to be of the form cmd_bm_*
 *  otherwise there'll continue to be problems. For now, we're just 
 *  renaming those that affect the personal toolbar (edit operations,
 *  which were clashing with the textfield controller)
 *
 * There are also several places that need to be updated if you need
 * to change a command name. 
 *   1) the controller in ALL clients (bookmarksTree.js, personalToolbar.js)
 *   2) the command nodes in the overlay
 *   3) the command human-readable name key in bookmark.properties
 *   4) the function 'getAllCmds' in bookmarksOverlay.js
 *   5) the function 'execCommand' in bookmarksOverlay.js
 * Yes, this blows crusty dead goats through straws, and I should probably
 * create some constants somewhere to bring this number down to 3. 
 * However, if you fail to do one of these, you WILL break something
 * and I WILL come after you with a knife. 
 */

function LITERAL (aDB, aElement, aPropertyID)
{
  var RDF = BookmarksUIElement.prototype.RDF;
  var rSource = RDF.GetResource(aElement.id);
  var rProperty = RDF.GetResource(aPropertyID);
  var node = aDB.GetTarget(rSource, rProperty, true);
  return node ? node.QueryInterface(Components.interfaces.nsIRDFLiteral).Value : "";
}

function BookmarksUIElement () { }
BookmarksUIElement.prototype = {
  _rdf: null,
  get RDF ()
  {
    if (!this._rdf) {
      const kRDFContractID = "@mozilla.org/rdf/rdf-service;1";
      const kRDFIID = Components.interfaces.nsIRDFService;
      this._rdf = Components.classes[kRDFContractID].getService(kRDFIID);
    }
    return this._rdf;
  },

  propertySet: function (sourceID, propertyID, newValue)
  {
    if (!newValue) return;
    const kRDFContractID = "@mozilla.org/rdf/rdf-service;1";
    const kRDFIID = Components.interfaces.nsIRDFService;
    const kRDF = Components.classes[kRDFContractID].getService(kRDFIID);
    // need to shuffle this into an API. 
    const kBMDS = kRDF.GetDataSource("rdf:bookmarks");
    const krProperty = kRDF.GetResource(propertyID);
    const krItem = kRDF.GetResource(sourceID);
    var rCurrValue = kBMDS.GetTarget(krItem, krProperty, true);
    const krNewValue = kRDF.GetLiteral(newValue);
    if (!rCurrValue)
      kBMDS.Assert(krItem, krProperty, krNewValue, true);
    else {
      rCurrValue = rCurrValue.QueryInterface(Components.interfaces.nsIRDFLiteral);
      if (rCurrValue.Value != newValue) 
        kBMDS.Change(krItem, krProperty, rCurrValue, krNewValue);
    }
  },

  /////////////////////////////////////////////////////////////////////////////
  // Fill a context menu popup with menuitems that are appropriate for the current
  // selection.
  createContextMenu: function (aEvent)
  {
    var popup = aEvent.target;
    // clear out the old context menu contents (if any)
    while (popup.hasChildNodes()) 
      popup.removeChild(popup.firstChild);
    
    var popupNode = document.popupNode;
    
    if (!("findRDFNode" in this))
      throw "Clients must implement findRDFNode!";
    var itemNode = this.findRDFNode(popupNode, true);
    if (!itemNode || !itemNode.getAttributeNS(RDF_NS, "type") || itemNode.getAttribute("mode") == "edit") {
      aEvent.preventDefault();
      return;
    }
    if (!("getContextSelection" in this)) 
      throw "Clients must implement getContextSelection!";
    var selection = this.getContextSelection (itemNode);
    var commonCommands = [];
    for (var i = 0; i < selection.length; ++i) {
      var commands = this.getAllCmds(selection[i].id);
      if (!commands) {
        aEvent.preventDefault();
        return;
      }
      commands = this.flattenEnumerator(commands);
      if (!commonCommands.length) commonCommands = commands;
      commonCommands = this.findCommonNodes(commands, commonCommands);
    }

    if (!commonCommands.length) {
      aEvent.preventDefault();
      return;
    }
    
    // Now that we should have generated a list of commands that is valid
    // for the entire selection, build a context menu.
    for (i = 0; i < commonCommands.length; ++i) {
      const kXULNS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";
      var currCommand = commonCommands[i].QueryInterface(Components.interfaces.nsIRDFResource).Value;
      var element = null;
      if (currCommand != NC_NS_CMD + "bm_separator") {
        var commandName = this.getCommandName(currCommand);
        element = this.createMenuItem(commandName, currCommand, itemNode);
      }
      else if (i != 0 && i < commonCommands.length-1) {
        // Never append a separator as the first or last element in a context
        // menu.
        element = document.createElementNS(kXULNS, "menuseparator");
      }
      
      if (element) 
        popup.appendChild(element);
    }
    return;
  },
  
  /////////////////////////////////////////////////////////////////////////////
  // Given two unique arrays, return an array that contains only the elements
  // common to both. 
  findCommonNodes: function (aNewArray, aOldArray)
  {
    var common = [];
    for (var i = 0; i < aNewArray.length; ++i) {
      for (var j = 0; j < aOldArray.length; ++j) {
        if (common.length > 0 && common[common.length-1] == aNewArray[i])
          continue;
        if (aNewArray[i] == aOldArray[j])
          common.push(aNewArray[i]);
      }
    }
    return common;
  },

  flattenEnumerator: function (aEnumerator)
  {
    if ("_index" in aEnumerator)
      return aEnumerator._inner;
    
    var temp = [];
    while (aEnumerator.hasMoreElements()) 
      temp.push(aEnumerator.getNext());
    return temp;
  },
  
  /////////////////////////////////////////////////////////////////////////////
  // For a given URI (a unique identifier of a resource in the graph) return 
  // an enumeration of applicable commands for that URI. 
  getAllCmds: function (aNodeID)
  {
    var type = BookmarksUtils.resolveType(aNodeID);
    if (!type) {
      if (aNodeID == "NC:PersonalToolbarFolder" || aNodeID == "NC:BookmarksRoot")
        type = "http://home.netscape.com/NC-rdf#Folder";
      else
        return null;
    }
    var commands = [];
    // menu order:
    // 
    // bm_open
    // bm_openfolder
    // bm_openinnewwindow
    // bm_openinnewtab 
    // ---------------------
    // /* bm_find removed */
    // bm_newfolder
    // ---------------------
    // bm_cut
    // bm_copy
    // bm_paste
    // bm_fileBookmark
    // ---------------------
    // bm_delete
    // bm_rename
    // ---------------------
    // bm_properties
    switch (type) {
    case "http://home.netscape.com/NC-rdf#BookmarkSeparator":
      commands = ["bm_newfolder", "bm_separator", 
                  "bm_cut", "bm_copy", "bm_paste", "bm_separator",
                  "bm_delete"];
      break;
    case "http://home.netscape.com/NC-rdf#Bookmark":
      commands = ["bm_open", "bm_openinnewwindow", "bm_openinnewtab", "bm_separator",
                  "bm_newfolder", "bm_separator",
                  "bm_cut", "bm_copy", "bm_paste", "bm_fileBookmark", "bm_separator",
                  "bm_delete", "bm_rename", "bm_separator",
                  "bm_properties"];
      break;
    case "http://home.netscape.com/NC-rdf#Folder":
      commands = ["bm_openfolder", "bm_openinnewwindow", "bm_separator", 
                  "bm_newfolder", "bm_separator",
                  "bm_cut", "bm_copy", "bm_paste", "bm_fileBookmark", "bm_separator",
                  "bm_delete", "bm_rename", "bm_separator",
                  "bm_properties"];
      break;
    case "http://home.netscape.com/NC-rdf#IEFavoriteFolder":
      commands = ["bm_openfolder", "bm_separator",
                  "bm_delete"];
      break;
    case "http://home.netscape.com/NC-rdf#IEFavorite":
      commands = ["bm_open", "bm_openinnewwindow", /* "bm_openinnewtab", */ "bm_separator",
                  "bm_copy"];
      break;
    case "http://home.netscape.com/NC-rdf#FileSystemObject":
      commands = ["bm_open", "bm_openinnewwindow", /* "bm_openinnewtab", */ "bm_separator",
                  "bm_copy"];
      break;
    default: 
      var source = this.RDF.GetResource(aNodeID);
      return this.db.GetAllCmds(source);
    }
    return new CommandArrayEnumerator(commands);
  },
  
  /////////////////////////////////////////////////////////////////////////////
  // Retrieve the human-readable name for a particular command. Used when 
  // manufacturing a UI to invoke commands.
  getCommandName: function (aCommand) 
  {
    var cmdName = aCommand.substring(NC_NS_CMD.length);
    try {
      // Note: this will succeed only if there's a string in the bookmarks
      //       string bundle for this command name. Otherwise, <xul:stringbundle/>
      //       will throw, we'll catch & stifle the error, and look up the command
      //       name in the datasource. 
      return BookmarksUtils.getLocaleString ("cmd_" + cmdName);
    }
    catch (e) {
    }   
    // XXX - WORK TO DO HERE! (rjc will cry if we don't fix this) 
    // need to ask the ds for the commands for this node, however we don't
    // have the right params. This is kind of a problem. 
    dump("*** BAD! EVIL! WICKED! NO! ACK! ARGH! ORGH!\n");
    const rName = this.RDF.GetResource(NC_NS + "Name");
    const rSource = this.RDF.GetResource(aNodeID);
    return this.db.GetTarget(rSource, rName, true).Value;
  },
  
  /////////////////////////////////////////////////////////////////////////////
  // Perform a command based on a UI event. XXX - work to do here. 
  preExecCommand: function (aEvent)
  {
    var commandID = aEvent.target.getAttribute("cmd");
    if (!commandID) return;
    goDoCommand("cmd_" + commandID.substring(NC_NS_CMD.length));
  },
  
  execCommand: function (aCommandID) 
  {
    var args = [];
    var selection = this.getSelection ();
    if (selection.length >= 1) 
      var selectedItem = selection[0];
    switch (aCommandID) {
    case "bm_open":
      this.open(null, selectedItem, "current_window");
      break;
    case "bm_openfolder":
      this.commands.openFolder(selectedItem);
      break;
    case "bm_openinnewwindow":
      if (BookmarksUtils.resolveType(selectedItem.id) == NC_NS + "Folder")
        this.openFolderInNewWindow(selectedItem);
      else
        this.open(null, selectedItem, "new_window");
      break;
    case "bm_openinnewtab":
      this.open(null, selectedItem, "new_tab");
      break;
    case "bm_rename":
      // XXX - this is SO going to break if we ever do column re-ordering.
      this.commands.editCell(selectedItem, 0);
      break;
    case "bm_editurl":
      this.commands.editCell(selectedItem, 1);
      break;
    case "bm_setnewbookmarkfolder":
    case "bm_setpersonaltoolbarfolder":
    case "bm_setnewsearchfolder":
      BookmarksUtils.doBookmarksCommand(selectedItem.id, NC_NS_CMD + aCommandID, args);
      // XXX - The containing node seems to be closed here and the 
      //       focus/selection is destroyed.
      this.selectElement(selectedItem);
      break;
    case "bm_properties":
      this.showPropertiesForNode(selectedItem);
      break;
    case "bm_find":
      this.findInBookmarks();
      break;
    case "bm_cut":
      this.copySelection(selection);
      this.deleteSelection(selection);
      break;
    case "bm_copy":
      this.copySelection(selection);
      break;
    case "bm_paste":
      this.paste(selection);
      break;
    case "bm_delete":
      this.deleteSelection(selection);
      break;
    case "bm_fileBookmark":
      var rv = { selectedFolder: null };      
      openDialog("chrome://communicator/content/bookmarks/addBookmark.xul", "", 
                 "centerscreen,chrome,modal=yes,dialog=yes,resizable=yes", null, null, folder, null, "selectFolder", rv);
      if (rv.selectedFolder) {
        for (var k = 0; k < selection.length; ++k) {
          if (selection[k].id == rv.selectedFolder)
            return; // Selection contains the target folder. Just fail silently.
        }
        var additiveFlag = false;
        var selectedItems = [].concat(this.getSelection())
        for (var i = 0; i < selectedItems.length; ++i) {
          var currItem = selectedItems[i];
          var currURI = currItem.id;
          var parent = gBookmarksShell.findRDFNode(currItem, false);
          gBookmarksShell.moveBookmark(currURI, parent.id, rv.selectedFolder);
          gBookmarksShell.selectFolderItem(rv.selectedFolder, currURI, additiveFlag);
          if (!additiveFlag) additiveFlag = true;
        }
        gBookmarksShell.flushDataSource();
      }
      break;
    case "bm_newfolder":
      var nfseln = document.popupNode;
      this.commands.createBookmarkItem("folder", nfseln);
      break;
    case "bm_newbookmark":
      var folder = this.getSelectedFolder();
      openDialog("chrome://communicator/content/bookmarks/addBookmark.xul", "", 
                 "centerscreen,chrome,modal=yes,dialog=yes,resizable=no", null, null, folder, null, "newBookmark");
      break;
    case "bm_newseparator":
      nfseln = this.getBestItem();
      var parentNode = this.findRDFNode(nfseln, false);
      args = [{ property: NC_NS + "parent", 
                resource: parentNode.id }];
      BookmarksUtils.doBookmarksCommand(nfseln.id, NC_NS_CMD + "newseparator", args);
      break;
    case "bm_import":
    case "bm_export":
      const isImport = aCommandID == "bm_import";
      try {
        const kFilePickerContractID = "@mozilla.org/filepicker;1";
        const kFilePickerIID = Components.interfaces.nsIFilePicker;
        const kFilePicker = Components.classes[kFilePickerContractID].createInstance(kFilePickerIID);
        
        const kTitle = BookmarksUtils.getLocaleString(isImport ? "SelectImport": "EnterExport");
        kFilePicker.init(window, kTitle, kFilePickerIID[isImport ? "modeOpen" : "modeSave"]);
        kFilePicker.appendFilters(kFilePickerIID.filterHTML | kFilePickerIID.filterAll);
        if (!isImport) kFilePicker.defaultString = "bookmarks.html";
        if (kFilePicker.show() != kFilePickerIID.returnCancel) {
          var fileName = kFilePicker.fileURL.spec;
          if (!fileName) break;
        }
        else break;
      }
      catch (e) {
        break;
      }
      var seln = this.getBestItem();
      args = [{ property: NC_NS + "URL", literal: fileName}];
      BookmarksUtils.doBookmarksCommand(seln.id, NC_NS_CMD + aCommandID, args);
      break;
    }
  },
  
  openFolderInNewWindow: function (aSelectedItem)
  {
    openDialog("chrome://communicator/content/bookmarks/bookmarks.xul", 
               "", "chrome,all,dialog=no", aSelectedItem.id);
  },
  
  copySelection: function (aSelection)
  {
    const kSuppArrayContractID = "@mozilla.org/supports-array;1";
    const kSuppArrayIID = Components.interfaces.nsISupportsArray;
    var itemArray = Components.classes[kSuppArrayContractID].createInstance(kSuppArrayIID);

    const kSuppWStringContractID = "@mozilla.org/supports-string;1";
    const kSuppWStringIID = Components.interfaces.nsISupportsString;
    var bmstring = Components.classes[kSuppWStringContractID].createInstance(kSuppWStringIID);
    var unicodestring = Components.classes[kSuppWStringContractID].createInstance(kSuppWStringIID);
    var htmlstring = Components.classes[kSuppWStringContractID].createInstance(kSuppWStringIID);
  
    var sBookmarkItem = ""; var sTextUnicode = ""; var sTextHTML = "";
    for (var i = 0; i < aSelection.length; ++i) {
      var url = LITERAL(this.db, aSelection[i], NC_NS + "URL");
      var name = LITERAL(this.db, aSelection[i], NC_NS + "Name");
      sBookmarkItem += aSelection[i].id + "\n";
      sTextUnicode += url + "\n";
      sTextHTML += "<A HREF=\"" + url + "\">" + name + "</A>";
    }    
    
    const kXferableContractID = "@mozilla.org/widget/transferable;1";
    const kXferableIID = Components.interfaces.nsITransferable;
    var xferable = Components.classes[kXferableContractID].createInstance(kXferableIID);

    xferable.addDataFlavor("moz/bookmarkclipboarditem");
    bmstring.data = sBookmarkItem;
    xferable.setTransferData("moz/bookmarkclipboarditem", bmstring, sBookmarkItem.length*2)
    
    xferable.addDataFlavor("text/html");
    htmlstring.data = sTextHTML;
    xferable.setTransferData("text/html", htmlstring, sTextHTML.length*2)
    
    xferable.addDataFlavor("text/unicode");
    unicodestring.data = sTextUnicode;
    xferable.setTransferData("text/unicode", unicodestring, sTextUnicode.length*2)
    
    const kClipboardContractID = "@mozilla.org/widget/clipboard;1";
    const kClipboardIID = Components.interfaces.nsIClipboard;
    var clipboard = Components.classes[kClipboardContractID].getService(kClipboardIID);
    clipboard.setData(xferable, null, kClipboardIID.kGlobalClipboard);
  },

  paste: function (aSelection)
  {
    const kXferableContractID = "@mozilla.org/widget/transferable;1";
    const kXferableIID = Components.interfaces.nsITransferable;
    var xferable = Components.classes[kXferableContractID].createInstance(kXferableIID);
    xferable.addDataFlavor("moz/bookmarkclipboarditem");
    xferable.addDataFlavor("text/x-moz-url");
    xferable.addDataFlavor("text/unicode");

    const kClipboardContractID = "@mozilla.org/widget/clipboard;1";
    const kClipboardIID = Components.interfaces.nsIClipboard;
    var clipboard = Components.classes[kClipboardContractID].getService(kClipboardIID);
    clipboard.getData(xferable, kClipboardIID.kGlobalClipboard);
    
    var flavour = { };
    var data = { };
    var length = { };
    xferable.getAnyTransferData(flavour, data, length);
    var nodes = []; var names = [];
    data = data.value.QueryInterface(Components.interfaces.nsISupportsString).data;
    switch (flavour.value) {
    case "moz/bookmarkclipboarditem":
      nodes = data.split("\n");
      break;
    case "text/x-moz-url":
      var ix = data.indexOf("\n");
      nodes.push(data.substring(0, ix != -1 ? ix : data.length));
      names.push(data.substring(ix));
      break;
    default: 
      return;
    }
    
    const lastSelected = aSelection[aSelection.length-1];  
    const kParentNode = this.resolvePasteFolder(aSelection);
    const krParent = this.RDF.GetResource(kParentNode.id);
    const krSource = this.RDF.GetResource(lastSelected.id);
    
    const kRDFCContractID = "@mozilla.org/rdf/container;1";
    const kRDFCIID = Components.interfaces.nsIRDFContainer;
    const ksRDFC = Components.classes[kRDFCContractID].getService(kRDFCIID);
    const kBMDS = this.RDF.GetDataSource("rdf:bookmarks");

    var additiveFlag = false;
    for (var i = 0; i < nodes.length; ++i) {
      if (!nodes[i]) continue;
      var rCurrent = this.RDF.GetResource(nodes[i]);
      const krTypeProperty = this.RDF.GetResource(RDF_NS + "type");
      var rType = this.db.GetTarget(rCurrent, krTypeProperty, true);
      try {
        rType = rType.QueryInterface(Components.interfaces.nsIRDFResource);
      }
      catch (e) {
        try {
          rType = rType.QueryInterface(Components.interfaces.nsIRDFLiteral);
        }
        catch (e) {
          // OK, no type exists, so node does not exist in the graph. 
          // (e.g. user pastes url as text)
          // Do some housekeeping. 
          const krName = this.RDF.GetResource(names[i]);
          const krNameProperty = this.RDF.GetResource(NC_NS + "Name");
          const krBookmark = this.RDF.GetResource(NC_NS + "Bookmark");
          kBMDS.Assert(rCurrent, krNameProperty, krName, true);
          kBMDS.Assert(rCurrent, krTypeProperty, krBookmark, true);
        }
      }

      // If the node is a folder, then we need to create a new anonymous 
      // resource and copy all the arcs over.
      if (rType && rType.Value == NC_NS + "Folder")
        rCurrent = BookmarksUtils.cloneFolder(rCurrent, krParent, krSource);

      // If this item already exists in this container, don't paste, as 
      // this will result in the creation of multiple copies in the datasource
      // but will not result in an update of the UI. (In Short: we don't
      // handle multiple bookmarks well)
      ksRDFC.Init(kBMDS, krParent);
      ix = ksRDFC.IndexOf(rCurrent);
      if (ix != -1)
        continue;

      ix = ksRDFC.IndexOf(krSource);
      if (ix != -1) 
        ksRDFC.InsertElementAt(rCurrent, ix+1, true);
      else
        ksRDFC.AppendElement(rCurrent);
      this.selectFolderItem(krSource.Value, rCurrent.Value, additiveFlag);
      if (!additiveFlag) additiveFlag = true;

      var rds = kBMDS.QueryInterface(Components.interfaces.nsIRDFRemoteDataSource);
      rds.Flush();
    }
  },
  
  /////////////////////////////////////////////////////////////////////////////
  // For the given selection, determines the element that should form the 
  // container to paste items into.
  resolvePasteFolder: function (aSelection)
  {
    const lastSelected = aSelection[aSelection.length-1];  
    if (lastSelected.getAttribute("container") == "true" &&
        aSelection.length == 1)
      return lastSelected;
    return this.findRDFNode(lastSelected, false);
  },
  
  canPaste: function ()
  {
    const kClipboardContractID = "@mozilla.org/widget/clipboard;1";
    const kClipboardIID = Components.interfaces.nsIClipboard;
    var clipboard = Components.classes[kClipboardContractID].getService(kClipboardIID);
    const kSuppArrayContractID = "@mozilla.org/supports-array;1";
    const kSuppArrayIID = Components.interfaces.nsISupportsArray;
    var flavourArray = Components.classes[kSuppArrayContractID].createInstance(kSuppArrayIID);
    const kSuppStringContractID = "@mozilla.org/supports-cstring;1";
    const kSuppStringIID = Components.interfaces.nsISupportsCString;
    
    var flavours = ["moz/bookmarkclipboarditem", "text/x-moz-url"];
    for (var i = 0; i < flavours.length; ++i) {
      const kSuppString = Components.classes[kSuppStringContractID].createInstance(kSuppStringIID);
      kSuppString.data = flavours[i];
      flavourArray.AppendElement(kSuppString);
    }
    var hasFlavours = clipboard.hasDataMatchingFlavors(flavourArray, kClipboardIID.kGlobalClipboard);
    return hasFlavours;
  },
  
  /////////////////////////////////////////////////////////////////////////////
  // aSelection is a mutable array, not a NodeList. 
  deleteSelection: function (aSelection)
  {
    const kRDFCContractID = "@mozilla.org/rdf/container;1";
    const kRDFCIID = Components.interfaces.nsIRDFContainer;
    const ksRDFC = Components.classes[kRDFCContractID].getService(kRDFCIID);

    var nextElement;
    var count = 0;

    var selectionLength = aSelection.length;
    while (aSelection.length && aSelection[count]) {
      const currParent = this.findRDFNode(aSelection[count], false);
      const kSelectionURI = aSelection[count].id;

      // Disallow the removal of certain 'special' nodes
      if (kSelectionURI == "NC:BookmarksRoot") {
        aSelection.splice(count++,1);
        continue;
      }

      // If the current bookmark is the IE Favorites folder, we have a little
      // extra work to do - set the pref |browser.bookmarks.import_system_favorites|
      // to ensure that we don't re-import next time. 
      if (aSelection[count].getAttribute("type") == (NC_NS + "IEFavoriteFolder")) {
        const kPrefSvcContractID = "@mozilla.org/preferences-service;1";
        const kPrefSvcIID = Components.interfaces.nsIPrefBranch;
        const kPrefSvc = Components.classes[kPrefSvcContractID].getService(kPrefSvcIID);
        kPrefSvc.setBoolPref("browser.bookmarks.import_system_favorites", false);
      }
        
      const krParent = this.RDF.GetResource(currParent.id);
      const krBookmark = this.RDF.GetResource(kSelectionURI);
      const kBMDS = this.RDF.GetDataSource("rdf:bookmarks");

      ksRDFC.Init(kBMDS, krParent);
      nextElement = this.getNextElement(aSelection[count]);
      ksRDFC.RemoveElement(krBookmark, true);

      try {
        // XXX - UGH. Template builder is NOT removing the element from the
        //       tree, and so selection remains non-zero in length and we go into
        //       an infinite loop here. Tear the node out of the document. 
        var parent = aSelection[count].parentNode;
        parent.removeChild(aSelection[count]);
      }
      catch (e) {
      }
      // Manipulate the selection array ourselves. 
      aSelection.splice(count,1);
    }
    this.selectElement(nextElement);
  },

  moveBookmark: function (aBookmarkURI, aFromFolderURI, aToFolderURI)
  {
    const kBMDS = this.RDF.GetDataSource("rdf:bookmarks");
    const kRDFCContractID = "@mozilla.org/rdf/container;1";
    const kRDFCIID = Components.interfaces.nsIRDFContainer;
    const kRDFC = Components.classes[kRDFCContractID].getService(kRDFCIID);
    const krSrc = this.RDF.GetResource(aBookmarkURI);
    const krOldParent = this.RDF.GetResource(aFromFolderURI);
    const krNewParent = this.RDF.GetResource(aToFolderURI);
    kRDFC.Init(kBMDS, krNewParent);
    kRDFC.AppendElement(krSrc);
    kRDFC.Init(kBMDS, krOldParent);
    kRDFC.RemoveElement(krSrc, true);
  },
  
  open: function (aEvent, aRDFNode, aTarget) 
  { 
    var urlValue = LITERAL(this.db, aRDFNode, NC_NS + "URL");
    
    // Ignore "NC:" and empty urls.
    if (urlValue.substring(0,3) == "NC:" || !urlValue) return;
    
    if (aEvent && aEvent.altKey)   
      this.showPropertiesForNode (aRDFNode);
    else if (aTarget == "new_window")
      openDialog (getBrowserURL(), "_blank", "chrome,all,dialog=no", urlValue);
    else if (aTarget == "new_tab") {
      var browser = null;
      if ("getBrowser" in window)
        browser = getBrowser();
      if (browser && "localName" in browser && browser.localName == "tabbrowser") {
        var theTab, loadInBackground;
        theTab = browser.addTab(urlValue); // open link in a new tab
        try {
          if (pref)
            loadInBackground = pref.getBoolPref("browser.tabs.loadInBackground");
          if (!loadInBackground)
            browser.selectedTab = theTab;
        }
        catch (e) {
        }
      }
    }  
    else
      openTopWin (urlValue);
    if (aEvent) 
      aEvent.preventBubble();
  },

  showPropertiesForNode: function (aBookmarkItem) 
  {
    if (aBookmarkItem.getAttribute("type") != NC_NS + "BookmarkSeparator") 
      openDialog("chrome://communicator/content/bookmarks/bm-props.xul",
                 "", "centerscreen,chrome,resizable=no", aBookmarkItem.id);
  },

  findInBookmarks: function ()
  {
    openDialog("chrome://communicator/content/bookmarks/findBookmark.xul",
               "FindBookmarksWindow",
               "centerscreen,resizable=no,chrome,dependent");
  },

  flushDataSource: function ()
  {
    const kBMDS = this.RDF.GetDataSource("rdf:bookmarks");
    var remoteDS = kBMDS.QueryInterface(Components.interfaces.nsIRDFRemoteDataSource);
    remoteDS.Flush();
  },

  /////////////////////////////////////////////////////////////////////////////
  // takes a node and adds the appropriate adornments for a bookmark container. 
  createBookmarkFolderDecorations: function (aNode)
  {
    aNode.setAttribute("type", "http://home.netscape.com/NC-rdf#Folder");
    aNode.setAttribute("container", "true");
    return aNode;
  }
};

function CommandArrayEnumerator (aCommandArray)
{
  this._inner = [];
  const kRDFContractID = "@mozilla.org/rdf/rdf-service;1";
  const kRDFIID = Components.interfaces.nsIRDFService;
  const RDF = Components.classes[kRDFContractID].getService(kRDFIID);
  for (var i = 0; i < aCommandArray.length; ++i)
    this._inner.push(RDF.GetResource(NC_NS_CMD + aCommandArray[i]));
    
  this._index = 0;
}

CommandArrayEnumerator.prototype = {
  getNext: function () 
  {
    return this._inner[this._index];
  },
  
  hasMoreElements: function ()
  {
    return this._index < this._inner.length;
  }
};

var BookmarksUtils = {

  _rdf: null,
  get RDF ()
  {
    if (!this._rdf) {
      const kRDFContractID = "@mozilla.org/rdf/rdf-service;1";
      const kRDFIID = Components.interfaces.nsIRDFService;
      this._rdf = Components.classes[kRDFContractID].getService(kRDFIID);
    }
    return this._rdf;
  },

  _bmds: null,
  get BMDS ()
  {
    if (!this._bmds) {
      this._bmds = this.RDF.GetDataSource("rdf:bookmarks");
    }
    return this._bmds;
  },

  _bundle        : null,
  _brandShortName: null,

  /////////////////////////////////////////////////////////////////////////////////////
  // returns a property from chrome://communicator/locale/bookmarks/bookmark.properties
  getLocaleString: function (aStringKey)
  {
    if (!this._bundle) {
      var LOCALESVC = Components.classes["@mozilla.org/intl/nslocaleservice;1"]
                                .getService(Components.interfaces.nsILocaleService);
      var BUNDLESVC = Components.classes["@mozilla.org/intl/stringbundle;1"]
                                .getService(Components.interfaces.nsIStringBundleService);
      var bookmarksBundle  = "chrome://communicator/locale/bookmarks/bookmark.properties";
      this._bundle         = BUNDLESVC.createBundle(bookmarksBundle, LOCALESVC.GetApplicationLocale());
      var brandBundle      = "chrome://global/locale/brand.properties";
      this._brandShortName = BUNDLESVC.createBundle(brandBundle,     LOCALESVC.GetApplicationLocale())
                                      .GetStringFromName("brandShortName");
    }
   
    var bundle = this._bundle.GetStringFromName(aStringKey);
    bundle = bundle.replace(/%brandShortName%/, this._brandShortName);
    return bundle;
  },
    
  /////////////////////////////////////////////////////////////////////////////
  // Determine the rdf:type property for the given resource.
  resolveType: function (aResource)
  {
    var rElement;
    if (typeof(aResource) == "string")
      rElement = this.RDF.GetResource(aResource);
    else
      rElement = aResource;

    const typeArc = this.RDF.GetResource(RDF_NS + "type");
    const type    = this.BMDS.GetTarget(rElement, typeArc, true);
    try {
      return type.QueryInterface(Components.interfaces.nsIRDFResource).Value;
    }
    catch (e) {
      try { 
        return type.QueryInterface(Components.interfaces.nsIRDFLiteral).Value;
      }
      catch (e) {
        return null;
      }
    }    
  },

  ///////////////////////////////////////////////////////////////////////////
  // Execute a command with the given source and arguments
  doBookmarksCommand: function (aSourceURI, aCommand, aArgumentsArray)
  {
    var rCommand = this.RDF.GetResource(aCommand);
  
    var kSuppArrayContractID = "@mozilla.org/supports-array;1";
    var kSuppArrayIID = Components.interfaces.nsISupportsArray;
    var sourcesArray = Components.classes[kSuppArrayContractID].createInstance(kSuppArrayIID);
    if (aSourceURI) {
      var rSource = this.RDF.GetResource(aSourceURI);
      sourcesArray.AppendElement (rSource);
    }
  
    var argsArray = Components.classes[kSuppArrayContractID].createInstance(kSuppArrayIID);
    for (var i = 0; i < aArgumentsArray.length; ++i) {
      var rArc = this.RDF.GetResource(aArgumentsArray[i].property);
      argsArray.AppendElement(rArc);
      var rValue = null;
      if ("resource" in aArgumentsArray[i]) { 
        rValue = this.RDF.GetResource(aArgumentsArray[i].resource);
      }
      else
        rValue = this.RDF.GetLiteral(aArgumentsArray[i].literal);
      argsArray.AppendElement(rValue);
    }

    // Exec the command in the Bookmarks datasource. 
    const kBMDS = this.RDF.GetDataSource("rdf:bookmarks");
    kBMDS.DoCommand(sourcesArray, rCommand, argsArray);
  },

  cloneFolder: function (aFolder, aParent, aRelativeItem) 
  {
    var BMDS = this.RDF.GetDataSource("rdf:bookmarks");
    
    var nameArc = this.RDF.GetResource(NC_NS + "Name");
    var rName = BMDS.GetTarget(aFolder, nameArc, true);
    rName = rName.QueryInterface(Components.interfaces.nsIRDFLiteral);
    
    var newFolder = this.createFolderWithID(rName.Value, aRelativeItem, aParent);
    
    // Now need to append kiddies. 
    try {
      const kRDFCContractID = "@mozilla.org/rdf/container;1";
      const kRDFCIID = Components.interfaces.nsIRDFContainer;
      var RDFC = Components.classes[kRDFCContractID].getService(kRDFCIID);
      const kRDFCUContractID = "@mozilla.org/rdf/container-utils;1";
      const kRDFCUIID = Components.interfaces.nsIRDFContainerUtils;
      var RDFCU = Components.classes[kRDFCUContractID].getService(kRDFCUIID);

      RDFC.Init(BMDS, aFolder);
      var elts = RDFC.GetElements();
      RDFC.Init(BMDS, newFolder);

      while (elts.hasMoreElements()) {
        var curr = elts.getNext().QueryInterface(Components.interfaces.nsIRDFResource);
        if (RDFCU.IsContainer(BMDS, curr))
          BookmarksUtils.cloneFolder(curr, newFolder);
        else
          RDFC.AppendElement(curr);
      }
    }
    catch (e) {
    }
    return newFolder;
  },
  
  createFolderWithID: function (aTitle, aRelativeItem, aParentFolder)
  {
    const kRDFCContractID = "@mozilla.org/rdf/container;1";
    const kRDFCIID = Components.interfaces.nsIRDFContainer;
    var RDFC = Components.classes[kRDFCContractID].createInstance(kRDFCIID);
    var BMDS = this.RDF.GetDataSource("rdf:bookmarks");
    try {
      RDFC.Init(BMDS, aParentFolder);
    }
    catch (e) {
      return null;
    }

    var ix = RDFC.IndexOf(aRelativeItem);
    var BMSvc = BMDS.QueryInterface(Components.interfaces.nsIBookmarksService);
    return BMSvc.createFolderInContainer(aTitle, aParentFolder, ix);
  },

  addBookmarkForTabBrowser: function( aTabBrowser, aSelect )
  {
    var tabsInfo = [];
    var currentTabInfo = { name: "", url: "", charset: null };

    const activeBrowser = aTabBrowser.selectedBrowser;
    const browsers = aTabBrowser.browsers;
    for (var i = 0; i < browsers.length; ++i) {
      var webNav = browsers[i].webNavigation;
      var url = webNav.currentURI.spec;
      var name = "";
      var charset;
      try {
        var doc = webNav.document;
        name = doc.title || url;
        charset = doc.characterSet;
      } catch (e) {
        name = url;
      }

      tabsInfo[i] = { name: name, url: url, charset: charset };

      if (browsers[i] == activeBrowser)
        currentTabInfo = tabsInfo[i];
    }

    openDialog("chrome://communicator/content/bookmarks/addBookmark.xul", "",
               "centerscreen,chrome,dialog=yes,resizable,dependent",
               currentTabInfo.name, currentTabInfo.url, null,
               currentTabInfo.charset, "addGroup" + (aSelect ? ",group" : ""), tabsInfo);
  },

  addBookmarkForBrowser: function (aDocShell, aShowDialog)
  {
    // Bug 52536: We obtain the URL and title from the nsIWebNavigation 
    //            associated with a <browser/> rather than from a DOMWindow.
    //            This is because when a full page plugin is loaded, there is
    //            no DOMWindow (?) but information about the loaded document
    //            may still be obtained from the webNavigation. 
    var url = aDocShell.currentURI.spec;
    var title, docCharset = null;
    try {
      title = aDocShell.document.title || url;
      docCharset = aDocShell.document.characterSet;
    }
    catch (e) {
      title = url;
    }

    this.addBookmark(url, title, docCharset, aShowDialog);
  },
  
  addBookmark: function (aURL, aTitle, aCharset, aShowDialog)
  {
    if (aCharset === undefined) {
      var fw = document.commandDispatcher.focusedWindow;
      aCharset = fw.document.characterSet;
    }

    if (aShowDialog)
      openDialog("chrome://communicator/content/bookmarks/addBookmark.xul", "", 
                 "centerscreen,chrome,dialog=yes,resizable,dependent", aTitle, aURL, null, aCharset);
    else {
      // User has elected to override the file dialog and always file bookmarks
      // into the default bookmark folder. 
      const kBMSvcContractID = "@mozilla.org/browser/bookmarks-service;1";
      const kBMSvcIID = Components.interfaces.nsIBookmarksService;
      const kBMSvc = Components.classes[kBMSvcContractID].getService(kBMSvcIID);
      kBMSvc.addBookmarkImmediately(aURL, aTitle, kBMSvcIID.BOOKMARK_DEFAULT_TYPE, aCharset);
    }
  },

  getSpecialFolder: function (aID)
  {
    var sources = this.BMDS.GetSources(this.RDF.GetResource("http://home.netscape.com/NC-rdf#FolderType"),
                                       this.RDF.GetResource(aID), true);
    var folder = null;
    if (sources.hasMoreElements())
      folder = sources.getNext();
    else 
      folder = this.RDF.GetResource("NC:BookmarksRoot");
    return folder;
  },

  getNewBookmarkFolder: function()
  {
    return this.getSpecialFolder("NC:NewBookmarkFolder");
  },

  getNewSearchFolder: function()
  {
    return this.getSpecialFolder("NC:NewSearchFolder");
  }

}

var ContentUtils = {
  childByLocalName: function (aSelectedItem, aLocalName)
  {
    var temp = aSelectedItem.firstChild;
    while (temp) {
      if (temp.localName == aLocalName)
        return temp;
      temp = temp.nextSibling;
    }
    return null;
  }
};






