/* -*- Mode: Java; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are
 * Copyright (C) 2000 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 *  Stuart Parmenter <pavlov@netscape.com>
 *  Brian Ryner <bryner@netscape.com>
 *  Jan Varga <varga@utcru.sk>
 *  Peter Annema <disttsc@bart.nl>
 */

const nsILocalFile        = Components.interfaces.nsILocalFile;
const nsILocalFile_CONTRACTID = "@mozilla.org/file/local;1";
const nsIFilePicker       = Components.interfaces.nsIFilePicker;
const nsIDirectoryServiceProvider = Components.interfaces.nsIDirectoryServiceProvider;
const nsIDirectoryServiceProvider_CONTRACTID = "@mozilla.org/file/directory_service;1";
const nsStdURL_CONTRACTID     = "@mozilla.org/network/standard-url;1";
const nsIFileURL          = Components.interfaces.nsIFileURL;
const NC_NAMESPACE_URI = "http://home.netscape.com/NC-rdf#";

var sfile = Components.classes[nsILocalFile_CONTRACTID].createInstance(nsILocalFile);
var retvals;
var filePickerMode;
var currentFilter;
var dirHistory;
var homeDir;

var directoryTree;
var textInput;
var okButton;

var bundle = srGetStrBundle("chrome://global/locale/filepicker.properties");   

function onLoad() {
  dirHistory = new Array();

  directoryTree = document.getElementById("directoryTree");
  textInput = document.getElementById("textInput");
  okButton = document.getElementById("ok");

  if (window.arguments) {
    var o = window.arguments[0];
    retvals = o.retvals; /* set this to a global var so we can set return values */
    const title = o.title;
    filePickerMode = o.mode;
    if (o.displayDirectory) {
      const directory = o.displayDirectory.unicodePath;
    }
    const initialText = o.defaultString;
    const filterTitles = o.filters.titles;
    const filterTypes = o.filters.types;
    const numFilters = filterTitles.length;

    window.title = title;

    if (initialText) {
      textInput.value = initialText;
    }
  }

  if ((filePickerMode == nsIFilePicker.modeOpen) ||
      (filePickerMode == nsIFilePicker.modeSave)) {

    currentFilter = filterTypes[0];
    applyFilter();

    /* build filter popup */
    var filterPopup = document.createElement("menupopup");

    for (var i = 0; i < numFilters; i++) {
      var menuItem = document.createElement("menuitem");
      menuItem.setAttribute("value", filterTitles[i] + " (" + filterTypes[i] + ")");
      menuItem.setAttribute("filters", filterTypes[i]);
      filterPopup.appendChild(menuItem);
    }

    var filterMenuList = document.getElementById("filterMenuList");
    filterMenuList.appendChild(filterPopup);
    var filterBox = document.getElementById("filterBox");
    filterBox.removeAttribute("hidden");
  } else if (filePickerMode == nsIFilePicker.modeGetFolder) {
     // This applies a special filter to only show directories
     applyDirectoryFilter();
  }

  try {
    var buttonLabel;
    switch (filePickerMode) {
      case nsIFilePicker.modeOpen:
        buttonLabel = bundle.GetStringFromName("openButtonLabel");
        break;
      case nsIFilePicker.modeSave:
        buttonLabel = bundle.GetStringFromName("saveButtonLabel");
        break;
      case nsIFilePicker.modeGetFolder:
        buttonLabel = bundle.GetStringFromName("selectFolderButtonLabel");
        break;
    }

    if (buttonLabel) {
      okButton.setAttribute("value", buttonLabel);
    }
  } catch (exception) {
    // keep it set to "OK"
  }

  // setup the dialogOverlay.xul button handlers
  doSetOKCancel(onOK, onCancel);

  // get the home dir
  var dirServiceProvider = Components.classes[nsIDirectoryServiceProvider_CONTRACTID].getService().QueryInterface(nsIDirectoryServiceProvider);
  var persistent = new Object();
  homeDir = dirServiceProvider.getFile("Home", persistent);

  if (directory) {
    sfile.initWithUnicodePath(directory);
  }
  if (!directory || !(sfile.exists() && sfile.isDirectory())) {
    // Start in the user's home directory
    sfile.initWithUnicodePath(homeDir.path);
  }

  retvals.buttonStatus = nsIFilePicker.returnCancel;

  gotoDirectory(sfile);
  doEnabling();
  textInput.focus();
}

function onFilterChanged(target)
{
  var filterTypes = target.getAttribute("filters");
  currentFilter = filterTypes;
  applyFilter();
}

function applyFilter()
{
  /* This is where we manipulate the DOM to create new <rule>s */
  var splitFilters = currentFilter.split("; ");
  var matchAllFiles = false;
  var ruleNode;

  /* get just the extensions for each of the filters */
  var extensions = new Array(splitFilters.length);
  for (var j = 0; j < splitFilters.length; j++) {
    var tmpStr = splitFilters[j];
    if (tmpStr == "*") {
      matchAllFiles = true;
      break;
    } else
      extensions[j] = tmpStr.substring(1); /* chop off the '*' */
  }

  /* delete all rules except the first one */
  for (j = 1;; j++) {
    ruleNode = document.getElementById("matchRule."+j);
    if (ruleNode) {
      ruleNode.parentNode.removeChild(ruleNode);
    } else {
      break;
    }
  }

  /* if we are matching all files, just clear the extension attribute
     on the first match rule and we're done */
  var rule0 = document.getElementById("matchRule.0");
  if (matchAllFiles) {
    rule0.removeAttributeNS(NC_NAMESPACE_URI, "extension");
    directoryTree.builder.rebuild();
    return;
  }

  /* rule 0 is special */
  rule0.setAttributeNS(NC_NAMESPACE_URI, "extension" , extensions[0]);

  /* iterate through the remaining extensions, creating new rules */
  ruleNode = document.getElementById("fileFilter");

  for (var k=1; k < extensions.length; k++) {
    var newRule = rule0.cloneNode(true);
    newRule.setAttribute("id", "matchRule."+k);
    newRule.setAttributeNS(NC_NAMESPACE_URI, "extension", extensions[k]);
    ruleNode.appendChild(newRule);
  }

  directoryTree.builder.rebuild();
}

function onOK()
{
  var ret = nsIFilePicker.returnCancel;

  var isDir = false;
  var isFile = false;

  var input = textInput.value;
  if (input[0] == '~') // XXX XP?
    input  = homeDir.unicodePath�+ input.substring(1);

  var file = sfile.clone().QueryInterface(nsILocalFile);
  if (!file)
    return false;

  /* XXX we need an XP way to test for an absolute path! */
  if (input[0] == '/')   /* an absolute path was entered */
    file.initWithUnicodePath(input);
  else {
    try {
      file.appendRelativeUnicodePath(input);
    } catch (e) {
      dump("Can't append relative path '"+input+"':\n");
      return false;
    }
  }

  if (!file.exists() && (filePickerMode != nsIFilePicker.modeSave)) {
    return false;
  }

  if (file.exists()) {
    isDir = file.isDirectory();
    isFile = file.isFile();
  }

  switch(filePickerMode) {
  case nsIFilePicker.modeOpen:
    if (isFile) {
      retvals.directory = file.parent.unicodePath;
      ret = nsIFilePicker.returnOK;
    } else if (isDir) {
      if (!sfile.equals(file)) {
        gotoDirectory(file);
      }
      textInput.value = "";
      doEnabling();
      ret = nsIFilePicker.returnCancel;
    }
    break;
  case nsIFilePicker.modeSave:
    if (isFile) { // can only be true if file.exists()
      // we need to pop up a dialog asking if you want to save
      rv = window.confirm(file.unicodePath + " " + bundle.GetStringFromName("confirmFileReplacing"));
      if (rv) {
        ret = nsIFilePicker.returnReplace;
        retvals.directory = file.parent.unicodePath;
      } else {
        ret = nsIFilePicker.returnCancel;
      }
    } else if (isDir) {
      if (!sfile.equals(file)) {
        gotoDirectory(file);
      }
      textInput.value = "";
      doEnabling();
      ret = nsIFilePicker.returnCancel;
    } else {
      var parent = file.parent;
      if (parent.exists() && parent.isDirectory()) {
        ret = nsIFilePicker.returnOK;
        retvals.directory = parent.unicodePath;
      } else {
        // See bug 55026, do nothing for now, leaves typed text as clue.
        // window.alert("Directory "+parent.path+" doesn't seem to exist, can't save "+file.unicodePath);
        ret = nsIFilePicker.returnCancel;
      }
    }
    break;
  case nsIFilePicker.modeGetFolder:
    if (isDir) {
      retvals.directory = file.parent.unicodePath;
    } else { // if nothing selected, the current directory will be fine
      retvals.directory = sfile.unicodePath;
    }
    ret = nsIFilePicker.returnOK;
    break;
  }

  retvals.file = file;
  retvals.buttonStatus = ret;

  if (ret == nsIFilePicker.returnCancel)
    return false;
  else
    return true;
}

function onCancel()
{
  // Close the window.
  retvals.buttonStatus = nsIFilePicker.returnCancel;
  retvals.file = null;
  return true;
}

function onClick(e) {
  if ( e.detail == 2 ) {
    var path = e.target.parentNode.getAttribute("path");

    if (path) {
      var file = URLpathToFile(path);
      if (file) {
        if (file.isDirectory()) {
          gotoDirectory(file);
        }
        else if (file.isFile()) {
          doOKButton();
        }
      }
    }
  }
}

function onKeypress(e) {
  if (e.keyCode == 8) /* backspace */
    goUp();
}

function doEnabling() {
  // Maybe add check if textInput.value would resolve to an existing
  // file or directory in .modeOpen. Too costly I think.
  var enable = (textInput.value != "");

  if (enable) {
    if (okButton.getAttribute("disabled")) {
      okButton.removeAttribute("disabled");
    }
  } else {
    if (!okButton.getAttribute("disabled")) {
      okButton.setAttribute("disabled","true");
    }
  }
}

function onSelect(e) {
  if (e.target.selectedItems.length != 1)
    return;
  var path = e.target.selectedItems[0].firstChild.getAttribute("path");

  if (path) {
    var file = URLpathToFile(path);
    if (file) {
      /* Put the leafName of the selected item in the input field if:
         - GetFolder mode   : a directory was selected (only option)
         - Open or Save mode: a file was selected                    */
      if ((filePickerMode == nsIFilePicker.modeGetFolder) || file.isFile()) {
        textInput.value = file.unicodeLeafName;
        doEnabling();
      }
    }
  }
}

function onDirectoryChanged(target)
{
  var path = target.getAttribute("value");

  var file = Components.classes[nsILocalFile_CONTRACTID].createInstance(nsILocalFile);
  file.initWithUnicodePath(path);

  if (!sfile.equals(file)) {
    gotoDirectory(file);
  }
}

function addToHistory(directoryName) {
  var found = false;
  var i = 0;
  while (!found && i<dirHistory.length) {
    if (dirHistory[i] == directoryName)
      found = true;
    else
      i++;
  }

  if (found) {
    if (i!=0) {
      dirHistory.splice(i, 1);
      dirHistory.splice(0, 0, directoryName);
    }
  } else {
    dirHistory.splice(0, 0, directoryName);
  }

  var menu = document.getElementById("lookInMenu");

  var children = menu.childNodes;
  for (i=0; i < children.length; i++)
    menu.removeChild(children[i]);

  for (i=0; i < dirHistory.length; i++) {
    var menuItem = document.createElement("menuitem");
    menuItem.setAttribute("value", dirHistory[i]);
    menu.appendChild(menuItem);
  }

  var menuList = document.getElementById("lookInMenuList");
  menuList.selectedIndex = 0;
}

function goUp() {
  try {
    var parent = sfile.parent;
  } catch(ex) { dump("can't get parent directory\n"); }

  if (parent) {
    gotoDirectory(parent);
  }
}

function gotoDirectory(directory) {
  addToHistory(directory.unicodePath);
  directoryTree.setAttribute("ref", fileToURL(directory).spec);
  sfile = directory;
}

function fileToURL(aFile) {
  var newDirectoryURL = Components.classes[nsStdURL_CONTRACTID].createInstance().QueryInterface(nsIFileURL);
  newDirectoryURL.file = aFile;
  return newDirectoryURL;
}

function URLpathToFile(aURLstr) {
  var fileURL = Components.classes[nsStdURL_CONTRACTID].createInstance().QueryInterface(nsIFileURL);
  fileURL.spec = aURLstr;
  return fileURL.file;
}

function applyDirectoryFilter() {
  var ruleNode = document.getElementById("matchRule.0");

  // A file can never have an extension of ".", because the extension is
  // by definition everything after the last dot.  So, this rule will
  // cause only directories to show up.
  ruleNode.setAttributeNS(NC_NAMESPACE_URI, "extension", ".");

  directoryTree.builder.rebuild();
}

