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
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 2000 Netscape Communications Corporation.  All
 * Rights Reserved.
 *
 * Contributor(s):
 *  Brian Ryner <bryner@netscape.com>
 *
 */

/* This file implements an nsIOutlinerView for the filepicker */

const nsILocalFile = Components.interfaces.nsILocalFile;
const nsILocalFile_CONTRACTID = "@mozilla.org/file/local;1";
const nsIFile = Components.interfaces.nsIFile;
const nsIScriptableDateFormat = Components.interfaces.nsIScriptableDateFormat;
const nsScriptableDateFormat_CONTRACTID = "@mozilla.org/intl/scriptabledateformat;1";
const nsIAtomService = Components.interfaces.nsIAtomService;
const nsAtomService_CONTRACTID = "@mozilla.org/atom-service;1";

var gDateService = null;

function numMatchingChars(str1, str2) {
  for (var i = 0; ((i < Math.min(str1.length, str2.length)) && (str1[i] == str2[i])); i++);
  return i;
}

function sortFilename(a, b) {
  if (a.cachedName < b.cachedName) {
    return -1;
  } else {
    return 1;
  }
}
  
function sortSize(a, b) {
  if (a.cachedSize < b.cachedSize) {
    return -1;
  } else if (a.cachedSize > b.cachedSize) {
    return 1;
  } else {
    return 0;
  }
}

function sortDate(a, b) {
  if (a.cachedDate < b.cachedDate) {
    return -1;
  } else if (a.cachedDate > b.cachedDate) {
    return 1;
  } else {
    return 0;
  }
}

function nsFileView() {
  this.mShowHiddenFiles = false;
  this.mDirectoryFilter = false;
  this.mFileList = [];
  this.mDirList = [];
  this.mFilteredFiles = [];
  this.mCurrentFilter = ".*";
  this.mSelectionCallback = null;
  this.mOutliner = null;
  this.mReverseSort = false;
  this.mSortType = 0;
  this.mTotalRows = 0;

  if (!gDateService) {
    gDateService = Components.classes[nsScriptableDateFormat_CONTRACTID]
      .getService(nsIScriptableDateFormat);
  }

  var atomService = Components.classes[nsAtomService_CONTRACTID]
                      .getService(nsIAtomService);
  this.mDirectoryAtom = atomService.getAtom("directory");
  this.mFileAtom = atomService.getAtom("file");
}

nsFileView.prototype = {

  SORTTYPE_NAME: 1,
  SORTTYPE_SIZE: 2,
  SORTTYPE_DATE: 3,

  /* readonly attribute long rowCount; */
  set rowCount(c) { throw "readonly property"; },
  get rowCount() { return this.mTotalRows; },

  /* attribute nsIOutlinerSelection selection; */
  set selection(s) { this.mSelection = s; },
  get selection() { return this.mSelection; },

  set selectionCallback(f) { this.mSelectionCallback = f; },
  get selectionCallback() { return this.mSelectionCallback; },

  /* nsISupports methods */

  /* void QueryInterface(in nsIIDRef uuid, 
     [iid_is(uuid),retval] out nsQIResult result); */
  QueryInterface: function(iid) {
    if (!iid.equals(nsIOutlinerView) &&
        !iid.equals(nsISupports)) {
          throw Components.results.NS_ERROR_NO_INTERFACE;
        }
    return this;
  },

  /* nsIOutlinerView methods */

  /* void getRowProperties(in long index, in nsISupportsArray properties); */
  getRowProperties: function(index, properties) { },

  /* void getCellProperties(in long row, in wstring colID, in nsISupportsArray properties); */
  getCellProperties: function(row, colID, properties) {
    if (row < this.mDirList.length)
      properties.AppendElement(this.mDirectoryAtom);
    else if ((row - this.mDirList.length) < this.mFilteredFiles.length)
      properties.AppendElement(this.mFileAtom);
  },

  /* void getColumnProperties(in wstring colID, in nsIDOMElement colElt,
     in nsISupportsArray properties); */
  getColumnProperties: function(colID, colElt, properties) { },

  /* boolean isContainer(in long index); */
  isContainer: function(index) { return false; },

  /* boolean isContainerOpen(in long index); */
  isContainerOpen: function(index) { return false;},

  /* boolean isContainerEmpty(in long index); */
  isContainerEmpty: function(index) { return false; },

  /* boolean isSorted (); */
  isSorted: function() { return (this.mSortType > 0); },

  /* boolean canDropOn (in long index); */
  canDropOn: function(index) { return false; },

  /* boolean canDropBeforeAfter (in long index, in boolean before); */
  canDropBeforeAfter: function(index, before) { return false; },

  /* void drop (in long row, in long orientation); */
  drop: function(row, orientation) { },

  /* long getParentIndex(in long rowIndex); */
  getParentIndex: function(rowIndex) { return -1; },

  /* boolean hasNextSibling(in long rowIndex, in long afterIndex); */
  hasNextSibling: function(rowIndex, afterIndex) {
    if (afterIndex < (this.mTotalRows -1)) {
      return true;
    } else {
      return false;
    }
  },

  /* long getLevel(in long index); */
  getLevel: function(index) { return 0; },
  
  /* wstring getCellText(in long row, in wstring colID); */
  getCellText: function(row, colID) {
    /* we cache the file size and last modified dates -
       this function must be very fast since it's called
       whenever the cell needs repainted */
    var file, isdir = false;
    if (row < this.mDirList.length) {
      isdir = true;
      file = this.mDirList[row];
    } else if ((row - this.mDirList.length) < this.mFilteredFiles.length) {
      file = this.mFilteredFiles[row - this.mDirList.length];
    } else {
      return "";
    }

    if (colID == "FilenameColumn") {
      if (!file.cachedName) {
        file.cachedName = file.file.unicodeLeafName;
      }
      return file.cachedName;
    } else if (colID == "LastModifiedColumn") {
      if (!file.cachedDate) {
        // perhaps overkill, but lets get the right locale handling
        var modDate = new Date(file.file.lastModificationDate);
        file.cachedDate = gDateService.FormatDateTime("", gDateService.dateFormatShort,
                                                      gDateService.timeFormatSeconds,
                                                      modDate.getFullYear(), modDate.getMonth()+1,
                                                      modDate.getDate(), modDate.getHours(),
                                                      modDate.getMinutes(), modDate.getSeconds());
      }
      return file.cachedDate;
    } else if (colID == "FileSizeColumn") {
      if (isdir) {
        return "";
      } else {
        if (!file.cachedSize) {
          file.cachedSize = String(file.file.fileSize);
        }
      }
      return file.cachedSize;
    }
    return "";
  },

  /* void setOutliner(in nsIOutlinerBoxObject outliner); */
  setOutliner: function(outliner) { this.mOutliner = outliner; },

  /* void toggleOpenState(in long index); */
  toggleOpenState: function(index) { },

  /* void cycleHeader(in wstring colID, in nsIDOMElement elt); */
  cycleHeader: function(colID, elt) { },

  /* void selectionChanged(); */
  selectionChanged: function() {
    if (this.mSelectionCallback && this.mSelection.currentIndex > 0) {
      var file;
      if (this.mSelection.currentIndex < this.mDirList.length) {
        file = this.mDirList[this.mSelection.currentIndex].file;
      } else if ((this.mSelection.currentIndex - this.mDirList.length) < this.mFilteredFiles.length) {
        file = this.mFilteredFiles[this.mSelection.currentIndex - this.mDirList.length].file;
      }

      if (file) {
        this.mSelectionCallback(file);
      }
    }
  },

  /* void cycleCell(in long row, in wstring colID); */
  cycleCell: function(row, colID) { },

  /* boolean isEditable(in long row, in wstring colID); */
  isEditable: function(row, colID) { return false; },

  /* void setCellText(in long row, in wstring colID, in wstring value); */
  setCellText: function(row, colID, value) { },

  /* void performAction(in wstring action); */
  performAction: function(action) { },

  /* void performActionOnRow(in wstring action, in long row); */
  performActionOnRow: function(action, row) { },

  /* void performActionOnCell(in wstring action, in long row, in wstring colID); */
  performActionOnCell: function(action, row, colID) { },

  /* private attributes */

  /* attribute boolean showHiddenFiles */
  set showHiddenFiles(s) {
    this.mShowHiddenFiles = s;
    this.setDirectory(this.mDirectoryPath);
  },

  get showHiddenFiles() { return this.mShowHiddenFiles; },

  /* attribute boolean showOnlyDirectories */
  set showOnlyDirectories(s) {
    this.mDirectoryFilter = s;
    this.filterFiles();
  },

  get showOnlyDirectories() { return this.mDirectoryFilter; },

  /* readonly attribute short sortType */
  set sortType(s) { throw "readonly property"; },
  get sortType() { return this.mSortType; },

  /* readonly attribute boolean reverseSort */
  set reverseSort(s) { throw "readonly property"; },
  get reverseSort() { return this.mReverseSort; },

  /* private methods */
  sort: function(sortType, reverseSort, forceSort) {
    if (sortType == this.mSortType && reverseSort != this.mReverseSort && !forceSort) {
      this.mDirList.reverse();
      this.mFilteredFiles.reverse();
    } else {
      var compareFunc, i;
      
      /* We pre-fetch all the data we are going to sort on, to avoid
         calling into C++ on every comparison */

      switch (sortType) {
      case 0:
        /* no sort has been set yet */
        return;
      case nsFileView.SORTTYPE_NAME:
        for (i = 0; i < this.mDirList.length; i++) {
          this.mDirList[i].cachedName = this.mDirList[i].file.unicodeLeafName;
        }
        for (i = 0; i < this.mFilteredFiles.length; i++) {
          this.mFilteredFiles[i].cachedName = this.mFilteredFiles[i].file.unicodeLeafName;
        }
        compareFunc = sortFilename;
        break;
      case nsFileView.SORTTYPE_SIZE:
        for (i = 0; i < this.mDirList.length; i++) {
          this.mDirList[i].cachedSize = this.mDirList[i].file.fileSize;
        }
        for (i = 0; i < this.mFilteredFiles.length; i++) {
          this.mFilteredFiles[i].cachedSize = this.mFilteredFiles[i].file.fileSize;
        }
        compareFunc = sortSize;
        break;
      case nsFileView.SORTTYPE_DATE:
        for (i = 0; i < this.mDirList.length; i++) {
          this.mDirList[i].cachedDate = this.mDirList[i].file.lastModificationDate;
        }
        for (i = 0; i < this.mFilteredFiles.length; i++) {
          this.mFilteredFiles[i].cachedDate = this.mFilteredFiles[i].file.lastModificationDate;
        }
        compareFunc = sortDate;
        break;
      default:
        throw("Unsupported sort type " + sortType);
        break;
      }
      this.mDirList.sort(compareFunc);
      this.mFilteredFiles.sort(compareFunc);
    }

    this.mSortType = sortType;
    this.mReverseSort = reverseSort;
    if (this.mOutliner) {
      this.mOutliner.invalidate();
    }
  },

  setDirectory: function(directory) {
    this.mDirectoryPath = directory;
    this.mFileList = [];
    this.mDirList = [];

    var dir = Components.classes[nsILocalFile_CONTRACTID].createInstance(nsILocalFile);
    dir.followLinks = false;
    dir.initWithUnicodePath(directory);
    var dirEntries = dir.QueryInterface(nsIFile).directoryEntries;
    var nextFile;
    var fileobj;
    var time = new Date();

    while (dirEntries.hasMoreElements()) {
      nextFile = dirEntries.getNext().QueryInterface(nsIFile);
      if (nextFile.isDirectory()) {
        if (!nextFile.isHidden() || this.mShowHiddenFiles) {
          fileobj = new Object();
          fileobj.file = nextFile;
          this.mDirList[this.mDirList.length] = fileobj;
        }
      } else if (!this.mDirectoryFilter) {
        this.mFileList[this.mFileList.length] = nextFile;
      }
    }

    time = new Date() - time;
    dump("load time: " + time/1000 + " seconds\n");

    this.mFilteredFiles = [];

    if (this.mOutliner) {
      var oldRows = this.mTotalRows;
      this.mTotalRows = this.mDirList.length;
      dump("rowCountChanged(0, " + (this.mDirList.length - oldRows) + ")\n");
      if (this.mDirList.length != oldRows) {
        this.mOutliner.rowCountChanged(0, this.mDirList.length - oldRows);
      }
      this.mOutliner.invalidate();
    }

    time = new Date();
    this.filterFiles();
    time = new Date() - time;
    dump("filter time: " + time/1000 + " seconds\n");
    time = new Date();
    this.sort(this.mSortType, this.mReverseSort);
    time = new Date() - time;
    dump("sort time: " + time/1000 + " seconds\n");
  },

  setFilter: function(filter) {
    // The filter may contain several components, i.e.:
    // *.html; *.htm
    // First separate it into its components
    var filterList = filter.split(/;[ ]*/);

    if (filterList.length == 0) {
      // this shouldn't happen
      return;
    }

    // Transform everything in the array to a regexp
    var tmp = filterList[0].replace(/\./g, "\\.");
    filterList[0] = tmp.replace(/\*/g, ".*");
    var shortestPrefix = filterList[0];
    
    for (var i = 1; i < filterList.length; i++) {
      // * becomes .*, and we escape all .'s with \
      tmp = filterList[i].replace(/\./g, "\\.");
      filterList[i] = tmp.replace(/\*/g, ".*");
      shortestPrefix = shortestPrefix.substr(0, numMatchingChars(shortestPrefix, filterList[i]));
    }
    
    var filterStr = shortestPrefix+"(";
    var startpos = shortestPrefix.length;
    for (i = 0; i < filterList.length; i++) {
      filterStr += filterList[i].substr(shortestPrefix.length) + "|";
    }

    this.mCurrentFilter = new RegExp(filterStr.substr(0, (filterStr.length) - 1) + ")");
    this.mFilteredFiles = [];

    if (this.mOutliner) {
      var rowDiff = -(this.mTotalRows - this.mDirList.length);
      this.mTotalRows = this.mDirList.length;
      this.mOutliner.rowCountChanged(this.mDirList.length, rowDiff);
      this.mOutliner.invalidate();
    }
    this.filterFiles();
    this.sort(this.mSortType, this.mReverseSort, true);
  },

  filterFiles: function() {
    for(var i = 0; i < this.mFileList.length; i++) {
      var file = this.mFileList[i];
      var fileobj;

      if ((this.mShowHiddenFiles || !file.isHidden()) &&
          file.unicodeLeafName.search(this.mCurrentFilter) == 0) {
        fileobj = new Object();
        fileobj.file = file;
        this.mFilteredFiles[this.mFilteredFiles.length] = fileobj;
      }
    }

    this.mTotalRows = this.mDirList.length + this.mFilteredFiles.length;

    // Tell the outliner how many rows we just added
    if (this.mOutliner) {
      this.mOutliner.rowCountChanged(this.mDirList.length, this.mFilteredFiles.length);
    }
  },

  getSelectedFile: function() {
    if (0 <= this.mSelection.currentIndex) {
      if (this.mSelection.currentIndex < this.mDirList.length) {
        return this.mDirList[this.mSelection.currentIndex].file;
      } else if ((this.mSelection.currentIndex - this.mDirList.length) < this.mFilteredFiles.length) {
        return this.mFilteredFiles[this.mSelection.currentIndex - this.mDirList.length].file;
      }
    }

    return null;
  }
}

