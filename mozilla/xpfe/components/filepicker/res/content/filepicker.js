/* -*- Mode: Java; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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

const nsIFilePicker       = Components.interfaces.nsIFilePicker;
const nsIDirectoryServiceProvider = Components.interfaces.nsIDirectoryServiceProvider;
const nsIDirectoryServiceProvider_CONTRACTID = "@mozilla.org/file/directory_service;1";
const nsStdURL_CONTRACTID     = "@mozilla.org/network/standard-url;1";
const nsIFileURL          = Components.interfaces.nsIFileURL;
const nsIOutlinerBoxObject = Components.interfaces.nsIOutlinerBoxObject;

var sfile = Components.classes[nsILocalFile_CONTRACTID].createInstance(nsILocalFile);
var retvals;
var filePickerMode;
var dirHistory;
var homeDir;
var outlinerView;

var textInput;
var okButton;

var gFilePickerBundle;

function filepickerLoad() {
  gFilePickerBundle = document.getElementById("bundle_filepicker");

  dirHistory = new Array();

  textInput = document.getElementById("textInput");
  okButton = document.getElementById("ok");
  outlinerView = new nsFileView();
  outlinerView.selectionCallback = onSelect;

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

    outlinerView.setFilter(filterTypes[0]);

    /* build filter popup */
    var filterPopup = document.createElement("menupopup");

    for (var i = 0; i < numFilters; i++) {
      var menuItem = document.createElement("menuitem");
      menuItem.setAttribute("label", filterTitles[i] + " (" + filterTypes[i] + ")");
      menuItem.setAttribute("filters", filterTypes[i]);
      filterPopup.appendChild(menuItem);
    }

    var filterMenuList = document.getElementById("filterMenuList");
    filterMenuList.appendChild(filterPopup);
    if (numFilters > 0)
      filterMenuList.selectedIndex = 0;
    var filterBox = document.getElementById("filterBox");
    filterBox.removeAttribute("hidden");
  } else if (filePickerMode == nsIFilePicker.modeGetFolder) {
    outlinerView.showOnlyDirectories = true;
  }

  // start out with a filename sort
  handleColumnClick("FilenameColumn");

  try {
    var buttonLabel;
    switch (filePickerMode) {
      case nsIFilePicker.modeOpen:
        buttonLabel = gFilePickerBundle.getString("openButtonLabel");
        break;
      case nsIFilePicker.modeSave:
        buttonLabel = gFilePickerBundle.getString("saveButtonLabel");
        break;
      case nsIFilePicker.modeGetFolder:
        buttonLabel = gFilePickerBundle.getString("selectFolderButtonLabel");
        break;
    }

    if (buttonLabel) {
      okButton.setAttribute("label", buttonLabel);
    }
  } catch (exception) {
    // keep it set to "OK"
  }

  // setup the dialogOverlay.xul button handlers
  doSetOKCancel(onOK, onCancel);

  // get the home dir
  var dirServiceProvider = Components.classes[nsIDirectoryServiceProvider_CONTRACTID]
                                     .getService(nsIDirectoryServiceProvider);
  var persistent = new Object();
  homeDir = dirServiceProvider.getFile("Home", persistent);

  if (directory) {
    sfile.initWithUnicodePath(directory);
  }
  if (!directory || !(sfile.exists() && sfile.isDirectory())) {
    // Start in the user's home directory
    sfile.initWithUnicodePath(homeDir.unicodePath);
  }

  retvals.buttonStatus = nsIFilePicker.returnCancel;

  gotoDirectory(sfile);
  var outliner = document.getElementById("directoryOutliner");
  outliner.boxObject.QueryInterface(nsIOutlinerBoxObject).view = outlinerView;

  doEnabling();
  textInput.focus();
}

function onFilterChanged(target)
{
  var filterTypes = target.getAttribute("filters");
  outlinerView.setFilter(filterTypes);
}

function onOK()
{
  var errorTitle, errorMessage, promptService;
  var ret = nsIFilePicker.returnCancel;

  var isDir = false;
  var isFile = false;

  var input = textInput.value;
  if (input[0] == '~') // XXX XP?
    input  = homeDir.unicodePath + input.substring(1);

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
    errorTitle = gFilePickerBundle.getFormattedString("errorOpenFileDoesntExistTitle",
                                                      [file.unicodePath]);
    errorMessage = gFilePickerBundle.getFormattedString("errorOpenFileDoesntExistMessage",
                                                        [file.unicodePath]);
    promptService = Components.classes["@mozilla.org/embedcomp/prompt-service;1"]
                              .getService(Components.interfaces.nsIPromptService);
    promptService.alert(window, errorTitle, errorMessage);
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
      var message = gFilePickerBundle.getFormattedString("confirmFileReplacing",
                                                         [file.unicodePath]);
      var rv = window.confirm(message);
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
        var oldParent = parent;
        while (!parent.exists()) {
          oldParent = parent;
          parent = parent.parent;
        }
        errorTitle = gFilePickerBundle.getFormattedString("errorSavingFileTitle",
                                                          [file.unicodePath]);
        if (parent.isFile()) {
          errorMessage = gFilePickerBundle.getFormattedString("saveParentIsFileMessage",
                                                              [parent.unicodePath, file.unicodePath]);
        } else {
          errorMessage = gFilePickerBundle.getFormattedString("saveParentDoesntExistMessage",
                                                              [oldParent.unicodePath, file.unicodePath]);
        }
        promptService = Components.classes["@mozilla.org/embedcomp/prompt-service;1"]
                                  .getService(Components.interfaces.nsIPromptService);
        promptService.alert(window, errorTitle, errorMessage);
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
  var t = e.originalTarget;
  if (t.localName == "outlinercol") {
    handleColumnClick(t.id);
  } else if (e.detail == 2 && t.localName == "outlinerbody") {
    var file = outlinerView.getSelectedFile();
    if (file) {
      if (file.isSymlink()) {
        var targetFile = Components.classes[nsILocalFile_CONTRACTID].createInstance(nsILocalFile);
        targetFile.initWithUnicodePath(file.unicodeTarget);
        file = targetFile;
      }
      if (file.isDirectory()) {
        gotoDirectory(file);
      }
      else if (file.isFile()) {
        doOKButton();
      }
    }
  }
}

function convertColumnIDtoSortType(columnID) {
  var sortKey;
  
  switch (columnID) {
  case "FilenameColumn":
    sortKey = sortType_name;
    break;
  case "FileSizeColumn":
    sortKey = sortType_size;
    break;
  case "LastModifiedColumn":
    sortKey = sortType_date;
    break;
  default:
    dump("unsupported sort column: " + columnID + "\n");
    sortKey = 0;
    break;
  }
  
  return sortKey;
}

function handleColumnClick(columnID) {
  var sortType = convertColumnIDtoSortType(columnID);
  if (outlinerView.sortType == sortType) {
    // reverse the current sort
    outlinerView.sort(outlinerView.sortType, !outlinerView.reverseSort, false);
  } else {
    outlinerView.sort(sortType, false, false);
  }
  
  // set the sort indicator on the column we are sorted by
  var sortedColumn = document.getElementById(columnID);
  if (outlinerView.reverseSort) {
    sortedColumn.setAttribute("sortDirection", "descending");
  } else {
    sortedColumn.setAttribute("sortDirection", "ascending");
  }
  
  // remove the sort indicator from the rest of the columns
  var currCol = document.getElementById("directoryOutliner").firstChild;
  while (currCol) {
    while (currCol && currCol.localName != "outlinercol")
      currCol = currCol.nextSibling;
    if (currCol) {
      if (currCol != sortedColumn) {
        currCol.removeAttribute("sortDirection");
      }
      currCol = currCol.nextSibling;
    }
  }
}

function updateSortIndicators(foo, bar) {
}

function reverseSort() {
  outlinerView.sort(outlinerView.sortType, !outlinerView.reverseSort);
  updateSortIndicators(outlinerView.sortType, outlinerView.reverseSort);
}

function doSort(sortType) {
  outlinerView.sort(sortType, false);
}

function onKeypress(e) {
  if (e.keyCode == 8) /* backspace */
    goUp();
}

function doEnabling() {
  // Maybe add check if textInput.value would resolve to an existing
  // file or directory in .modeOpen. Too costly I think.
  var enable = (textInput.value != "");

  okButton.disabled = !enable;
}

function onSelect(file) {
  var path = file.unicodeLeafName;

  if (path) {
    if ((filePickerMode == nsIFilePicker.modeGetFolder) || file.isFile()) {
      textInput.value = path;
      doEnabling();
    }
  }
}

function onDirectoryChanged(target)
{
  var path = target.getAttribute("label");

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
    menuItem.setAttribute("label", dirHistory[i]);
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
  outlinerView.setDirectory(directory.unicodePath);
  sfile = directory;
}

