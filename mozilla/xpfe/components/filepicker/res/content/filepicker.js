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
const nsIOutlinerBoxObject = Components.interfaces.nsIOutlinerBoxObject;
const nsIFileView = Components.interfaces.nsIFileView;
const nsFileView_CONTRACTID = "@mozilla.org/filepicker/fileview;1";
const nsIOutlinerView = Components.interfaces.nsIOutlinerView;
const nsILocalFile = Components.interfaces.nsILocalFile;
const nsLocalFile_CONTRACTID = "@mozilla.org/file/local;1";

var sfile = Components.classes[nsLocalFile_CONTRACTID].createInstance(nsILocalFile);
var retvals;
var filePickerMode;
var homeDir;
var outlinerView;

var textInput;
var okButton;

var gFilePickerBundle;

function filepickerLoad() {
  gFilePickerBundle = document.getElementById("bundle_filepicker");

  textInput = document.getElementById("textInput");
  okButton = document.documentElement.getButton("accept");
  outlinerView = Components.classes[nsFileView_CONTRACTID].createInstance(nsIFileView);

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

    filterMenuList.selectedIndex = o.filterIndex;
  } else if (filePickerMode == nsIFilePicker.modeGetFolder) {
    outlinerView.showOnlyDirectories = true;
  }

  // start out with a filename sort
  handleColumnClick("FilenameColumn");

  document.documentElement.setAttribute("ondialogcancel", "return onCancel();");
  try {
    var buttonLabel = getOKAction();
    okButton.setAttribute("label", buttonLabel);
  } catch (exception) {
    // keep it set to "OK"
  }

  // setup the dialogOverlay.xul button handlers
  retvals.buttonStatus = nsIFilePicker.returnCancel;

  var outliner = document.getElementById("directoryOutliner");
  outliner.outlinerBoxObject.view = outlinerView;

  // Start out with the ok button disabled since nothing will be
  // selected and nothing will be in the text field.
  okButton.disabled = true;
  textInput.focus();

  // This allows the window to show onscreen before we begin
  // loading the file list

  setTimeout(setInitialDirectory, 0, directory);
}

function setInitialDirectory(directory)
{
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

  gotoDirectory(sfile);
}

function onFilterChanged(target)
{
  // Do this on a timeout callback so the filter list can roll up
  // and we don't keep the mouse grabbed while we are refiltering.

  setTimeout(changeFilter, 0, target.getAttribute("filters"));
}

function changeFilter(filterTypes)
{
  window.setCursor("wait");
  outlinerView.setFilter(filterTypes);
  window.setCursor("auto");
}

function showFilePermissionsErrorDialog(titleStrName, messageStrName, file)
{
  var errorTitle =
    gFilePickerBundle.getFormattedString(titleStrName, [file.unicodePath]);
  var errorMessage =
    gFilePickerBundle.getFormattedString(messageStrName, [file.unicodePath]);
  var promptService =
    Components.classes["@mozilla.org/embedcomp/prompt-service;1"].getService(Components.interfaces.nsIPromptService);

  promptService.alert(window, errorTitle, errorMessage)
}

function openOnOK()
{
  var dir = outlinerView.getSelectedFile();
  if (!dir.isReadable()) {
    showFilePermissionsErrorDialog("errorOpenFileDoesntExistTitle",
                                   "errorDirNotReadableMessage",
                                   dir);
    return false;
  }

  if (dir)
    gotoDirectory(dir);
  retvals.file = dir;
  retvals.buttonStatus = nsIFilePicker.returnCancel;
  
  var filterMenuList = document.getElementById("filterMenuList");
  retvals.filterIndex = filterMenuList.selectedIndex;
  
  return false;
}

function selectOnOK()
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
  else if ((input.indexOf("/../") > 0) ||
           (input.substr(-3) == "/..") ||
           (input.substr(0,3) == "../") ||
           (input == "..")) {
    /* appendRelativePath doesn't allow .. */
    file.initWithUnicodePath(file.unicodePath + "/" + input);
    file.normalize();
  }
  else {
    try {
      file.appendRelativeUnicodePath(input);
    } catch (e) {
      dump("Can't append relative path '"+input+"':\n");
      return false;
    }
  }

  if (!file.exists() && (filePickerMode != nsIFilePicker.modeSave)) {
    showFilePermissionsErrorDialog("errorOpenFileDoesntExistTitle",
                                   "errorOpenFileDoesntExistMessage",
                                   file);
    return false;
  }

  if (file.exists()) {
    isDir = file.isDirectory();
    isFile = file.isFile();
  }

  switch(filePickerMode) {
  case nsIFilePicker.modeOpen:
    if (isFile) {
      if (file.isReadable()) {
        retvals.directory = file.parent.unicodePath;
        ret = nsIFilePicker.returnOK;
      } else {
        showFilePermissionsErrorDialog("errorOpeningFileTitle",
                                       "openWithoutPermissionMessage_file",
                                       file);
        ret = nsIFilePicker.returnCancel;
      }
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
      if (!file.isWritable()) {
        showFilePermissionsErrorDialog("errorSavingFileTitle",
                                       "saveWithoutPermissionMessage_file",
                                       file);
        ret = nsIFilePicker.returnCancel;
      } else {
        // we need to pop up a dialog asking if you want to save
        var message =
          gFilePickerBundle.getFormattedString("confirmFileReplacing",
                                               [file.unicodePath]);

        var rv = window.confirm(message);
        if (rv) {
          ret = nsIFilePicker.returnReplace;
          retvals.directory = file.parent.unicodePath;
        } else {
          ret = nsIFilePicker.returnCancel;
        }
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
      if (parent.exists() && parent.isDirectory() && parent.isWritable()) {
        ret = nsIFilePicker.returnOK;
        retvals.directory = parent.unicodePath;
      } else {
        var oldParent = parent;
        while (!parent.exists()) {
          oldParent = parent;
          parent = parent.parent;
        }
        var errorTitle =
          gFilePickerBundle.getFormattedString("errorSavingFileTitle",
                                               [file.unicodePath]);
        var errorMessage;
        if (parent.isFile()) {
          errorMessage =
            gFilePickerBundle.getFormattedString("saveParentIsFileMessage",
                                                 [parent.unicodePath, file.unicodePath]);
        } else {
          errorMessage =
            gFilePickerBundle.getFormattedString("saveParentDoesntExistMessage",
                                                 [oldParent.unicodePath, file.unicodePath]);
        }
        if (!parent.isWritable()) {
          errorMessage =
            gFilePickerBundle.getFormattedString("saveWithoutPermissionMessage_dir", [parent.unicodePath]);
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

  var filterMenuList = document.getElementById("filterMenuList");
  retvals.filterIndex = filterMenuList.selectedIndex;
  
  return (ret != nsIFilePicker.returnCancel);
}

function onCancel()
{
  // Close the window.
  retvals.buttonStatus = nsIFilePicker.returnCancel;
  retvals.file = null;
  return true;
}

function onDblClick(e) {
  var t = e.originalTarget;
  if (t.localName != "outlinerchildren")
    return;

  openSelectedFile();
}

function openSelectedFile() {
  var file = outlinerView.getSelectedFile();
  if (!file)
    return;

  if (file.isDirectory())
    gotoDirectory(file);
  else if (file.isFile())
    document.documentElement.acceptDialog();
}

function onClick(e) {
  var t = e.originalTarget;
  if (t.localName == "outlinercol")
    handleColumnClick(t.id);
}

function convertColumnIDtoSortType(columnID) {
  var sortKey;
  
  switch (columnID) {
  case "FilenameColumn":
    sortKey = nsIFileView.sortName;
    break;
  case "FileSizeColumn":
    sortKey = nsIFileView.sortSize;
    break;
  case "LastModifiedColumn":
    sortKey = nsIFileView.sortDate;
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
  var sortOrder = (outlinerView.sortType == sortType) ? !outlinerView.reverseSort : false;
  outlinerView.sort(sortType, sortOrder);
  
  // set the sort indicator on the column we are sorted by
  var sortedColumn = document.getElementById(columnID);
  if (outlinerView.reverseSort) {
    sortedColumn.setAttribute("sortDirection", "descending");
  } else {
    sortedColumn.setAttribute("sortDirection", "ascending");
  }
  
  // remove the sort indicator from the rest of the columns
  var currCol = sortedColumn.parentNode.firstChild;
  while (currCol) {
    if (currCol != sortedColumn && currCol.localName == "outlinercol")
      currCol.removeAttribute("sortDirection");
    currCol = currCol.nextSibling;
  }
}

function onKeypress(e) {
  if (e.keyCode == 8) /* backspace */
    goUp();
  else if (e.keyCode == 13) { /* enter */
    var file = outlinerView.getSelectedFile();
    if (file) {
      if (file.isDirectory()) {
        gotoDirectory(file);
        e.preventDefault();
      }
    }
  }
}

function doEnabling() {
  // Maybe add check if textInput.value would resolve to an existing
  // file or directory in .modeOpen. Too costly I think.
  var enable = (textInput.value != "");

  okButton.disabled = !enable;
}

function onOutlinerFocus(event) {
  // Reset the button label and enabled/disabled state.
  onFileSelected(outlinerView.getSelectedFile());
}

function getOKAction(file) {
  var buttonLabel;

  if (file && file.isDirectory() && filePickerMode != nsIFilePicker.modeGetFolder) {
    document.documentElement.setAttribute("ondialogaccept", "return openOnOK();");
    buttonLabel = gFilePickerBundle.getString("openButtonLabel");
  }
  else {
    document.documentElement.setAttribute("ondialogaccept", "return selectOnOK();");
    switch(filePickerMode) {
    case nsIFilePicker.modeGetFolder:
      buttonLabel = gFilePickerBundle.getString("selectFolderButtonLabel");
      break;
    case nsIFilePicker.modeOpen:
      buttonLabel = gFilePickerBundle.getString("openButtonLabel");
      break;
    case nsIFilePicker.modeSave:
      buttonLabel = gFilePickerBundle.getString("saveButtonLabel");
      break;
    }
  }

  return buttonLabel;
}

function onSelect(event) {
  onFileSelected(outlinerView.getSelectedFile());
}

function onFileSelected(file) {
  if (file) {
    var path = file.unicodeLeafName;
    
    if (path) {
      if ((filePickerMode == nsIFilePicker.modeGetFolder) || !file.isDirectory())
        textInput.value = path;
      
      var buttonLabel = getOKAction(file);
      okButton.setAttribute("label", buttonLabel);
      okButton.disabled = false;
      return;
    }
  }

  okButton.disabled = (textInput.value == "");
}

function onTextFieldFocus() {
  var buttonLabel = getOKAction(null);
  okButton.setAttribute("label", buttonLabel);
  doEnabling();
}

function onDirectoryChanged(target)
{
  var path = target.getAttribute("label");

  var file = Components.classes[nsLocalFile_CONTRACTID].createInstance(nsILocalFile);
  file.initWithUnicodePath(path);

  if (!sfile.equals(file)) {
    // Do this on a timeout callback so the directory list can roll up
    // and we don't keep the mouse grabbed while we are loading.

    setTimeout(gotoDirectory, 0, file);
  }
}

function populateAncestorList(directory) {
  var menu = document.getElementById("lookInMenu");

  while (menu.hasChildNodes()) {
    menu.removeChild(menu.firstChild);
  }
  
  var menuItem = document.createElement("menuitem");
  menuItem.setAttribute("label", directory.unicodePath);
  menuItem.setAttribute("crop", "start");
  menu.appendChild(menuItem);

  // .parent is _sometimes_ null, see bug 121489.  Do a dance around that.
  var parent = directory.parent;
  while (parent && !parent.equals(directory)) {
    menuItem = document.createElement("menuitem");
    menuItem.setAttribute("label", parent.unicodePath);
    menuItem.setAttribute("crop", "start");
    menu.appendChild(menuItem);
    directory = parent;
    parent = directory.parent;
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

function goHome() {
  gotoDirectory(homeDir);
}

function gotoDirectory(directory) {
  window.setCursor("wait");
  try {
    populateAncestorList(directory);
    outlinerView.setDirectory(directory);
    document.getElementById("errorShower").selectedIndex = 0;
  } catch(ex) {
    document.getElementById("errorShower").selectedIndex = 1;
  }

  window.setCursor("auto");

  outlinerView.QueryInterface(nsIOutlinerView).selection.clearSelection();
  textInput.focus();
  sfile = directory;
}

function toggleShowHidden(event) {
  outlinerView.showHiddenFiles = !outlinerView.showHiddenFiles;
}
