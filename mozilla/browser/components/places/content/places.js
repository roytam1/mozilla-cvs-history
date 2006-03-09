//* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
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
 * The Original Code is Mozilla Places System.
 *
 * The Initial Developer of the Original Code is Google Inc.
 * Portions created by the Initial Developer are Copyright (C) 2005
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Ben Goodger <beng@google.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
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

const PREF_PLACES_GROUPING_GENERIC = "browser.places.grouping.generic";
const PREF_PLACES_GROUPING_BOOKMARK = "browser.places.grouping.bookmark";

// Default Search Queries
const INDEX_HISTORY = 0;
const INDEX_BOOKMARKS = 1;

var PlacesOrganizer = {
  _content: null,
  _places: null,
  
  init: function PP_init() {
    // Attach the Command Controller to the Places Views. 
    this._places = document.getElementById("placesList");
    this._content = document.getElementById("placeContent");  
    this._places.controllers.appendController(PlacesController);
    this._content.controllers.appendController(PlacesController);
    
    this._places.init(new ViewConfig([TYPE_X_MOZ_PLACE_CONTAINER],
                                     ViewConfig.GENERIC_DROP_TYPES,
                                     true, false, 4, true));
    this._content.init(new ViewConfig(ViewConfig.GENERIC_DROP_TYPES,
                                      ViewConfig.GENERIC_DROP_TYPES,
                                      false, false, 0, true));

    PlacesController.groupableView = this._content;

    var GroupingSerializer = {
      serialize: function GS_serialize(raw) {
        return raw.join(",");
      },
      deserialize: function GS_deserialize(str) {
        return str === "" ? [] : str.split(",");
      }
    };
    PlacesController.groupers.generic = 
      new PrefHandler(PREF_PLACES_GROUPING_GENERIC, 
        [Ci.nsINavHistoryQueryOptions.GROUP_BY_DOMAIN], GroupingSerializer);
    PlacesController.groupers.bookmark = 
      new PrefHandler(PREF_PLACES_GROUPING_BOOKMARK,
        [Ci.nsINavHistoryQueryOptions.GROUP_BY_FOLDER], GroupingSerializer);
    
    // Attach the Places model to the Place View
    // XXXben - move this to an attribute/property on the tree view
    this._places.loadFolder(PlacesController.bookmarks.placesRoot);
    
    // Now load the appropriate folder in the Content View, and select the 
    // corresponding entry in the Places View. This is a little fragile. 
    var params = window.arguments[0];
    var index = params == "history" ? INDEX_HISTORY : INDEX_BOOKMARKS;
    this._places.view.selection.select(index);
    
    // Set up the search UI.
    PlacesSearchBox.init();
    
    // Set up the advanced query builder UI
    PlacesQueryBuilder.init();
  },

  /**
   * A range has been selected from the calendar picker. Update the view
   * to show only those results within the selected range. 
   */
  rangeSelected: function PP_rangeSelected() {
    var result = this._content.getResult();
    var queries = this.getCurrentQueries();

    var calendar = document.getElementById("historyCalendar");
    var begin = calendar.beginrange.getTime();
    var end = calendar.endrange.getTime();

    // The calendar restuns values in terms of whole days at midnight, inclusive.
    // The end time range therefor must be moved to the evening of the end
    // include that day in the query.
    const DAY_MSEC = 86400000;
    end += DAY_MSEC;

    var newQueries = [];
    for (var i = 0; i < queries.length; ++i) {
      var query = queries[i].clone();
      query.beginTimeReference = Ci.nsINavHistoryQuery.TIME_RELATIVE_EPOCH;
      query.beginTime = begin * 1000;
      query.endTimeReference = Ci.nsINavHistoryQuery.TIME_RELATIVE_EPOCH;
      query.endTime = end * 1000;
      newQueries.push(query);
    }
    
    this._content.load(newQueries, this.getCurrentOptions());
    
    return true;
  },

  /**
   * Fill the header with information about what view is being displayed.
   */
  _setHeader: function(type, text) {
    var bundle = document.getElementById("placeBundle");
    var key = null;
    var isSearch = false;
    switch(type) {
      case "showing":
        key = "headerTextShowing";
        break;
      case "results":
        isSearch = true;
        key = "headerTextResultsFor";
        break;
      case "advanced":
        isSearch = true;
        key = "headerTextAdvancedSearch";
        break;
    }
    var showingPrefix = document.getElementById("showingPrefix");
    showingPrefix.setAttribute("value", bundle.getString(key));
    
    var contentTitle = document.getElementById("contentTitle");
    contentTitle.setAttribute("value", text);
    
    var searchModifiers = document.getElementById("searchModifiers");
    searchModifiers.hidden = !isSearch;
  },  
  
  /**
   * Run a search for the specified text, over the collection specified by
   * the dropdown arrow. The default is all bookmarks and history, but can be
   * localized to the active collection. 
   * @param   filterString
   *          The text to search for. 
   */
  search: function PP_applyFilter(filterString) {
    switch (PlacesSearchBox.filterCollection) {
    case "collection":
      var folder = this._content.getResult().root.QueryInterface(Ci.nsINavHistoryFolderResultNode).folderId;
      this._content.applyFilter(filterString, true, folder);
      this._setHeader("results", filterString);
      break;
    case "all":
      this._content.filterString = filterString;
      this._setHeader("results", filterString);
      break;
    }
  },
  
  /**
   * Called when a place folder is selected in the left pane.
   */
  onPlaceSelected: function PP_onPlaceSelected(event) {
    var node = asQuery(this._places.selectedNode);
    if (!node)
      return;
      
    var sortingMode = node.queryOptions.sortingMode;    
    var groupings = PlacesController.groupers.generic;
    if (PlacesController.nodeIsFolder(node)) 
      groupings = PlacesController.groupers.bookmark;
    PlacesController.loadNodeIntoView(this._content, node, groupings, 
                                      sortingMode);

    this._setHeader("showing", node.title);
  },

  /**
   * Shows all subscribed feeds (Live Bookmarks) grouped under their parent 
   * feed.
   */
  groupByFeed: function PP_groupByFeed() {
    var groupings = [Ci.nsINavHistoryQueryOptions.GROUP_BY_FOLDER];
    var sortingMode = Ci.nsINavHistoryQueryOptions.SORT_BY_DATE_DESCENDING;
    PlacesController.groupByAnnotation("livemark/feedURI", [], 0);
  },
  
  /**
   * Shows all subscribed feed (Live Bookmarks) content in a flat list
   */
  groupByPost: function PP_groupByPost() {
    var groupings = [Ci.nsINavHistoryQueryOptions.GROUP_BY_FOLDER];
    var sortingMode = Ci.nsINavHistoryQueryOptions.SORT_BY_DATE_DESCENDING;
    PlacesController.groupByAnnotation("livemark/bookmarkFeedURI", [], 0);
  },
  
  
  /**
   * Updates the calendar widget to show the range of dates selected in the
   * current result. 
   */
  _updateCalendar: function PP__updateCalendar() {
    var calendar = document.getElementById("historyCalendar");

    var result = this._content.getResult();
    if (result.root.type == Ci.nsINavHistoryResultNode.RESULT_TYPE_QUERY)
      result.root.QueryInterface(Ci.nsINavHistoryQueryResultNode);
    else if (result.root.type == Ci.nsINavHistoryResultNode.RESULT_TYPE_FOLDER)
      result.root.QueryInterface(Ci.nsINavHistoryFolderResultNode);
    else {
      calendar.selectNothing = true;
      return;
    }
    var queries = this.getCurrentQueries();

    // if there is more than one query, make sure that they all specify the
    // same date range. If there isn't a unique date range, don't display
    // anything.
    if (queries.length < 1) {
      calendar.selectNothing = true;
      return;
    }
    var absBegin = queries[0].absoluteBeginTime;
    var absEnd = queries[0].absoluteEndTime;
    for (var i = 1; i < queries.length; i ++) {
      if (queries[i].absoluteBeginTime != absBegin ||
          queries[i].absoluteEndTime != absEnd) {
        calendar.selectNothing = true;
        return;
      }
    }

    var query = queries[0];

    // Make sure that by updating the calendar widget we don't fire selection
    // events and cause the UI to infinitely reload.
    calendar.suppressRangeEvents = true;

    // begin
    var beginRange = null;
    if (query.hasBeginTime)
      beginRange = new Date(query.absoluteBeginTime / 1000);

    // end
    var endRange = null;
    if (query.hasEndTime) {
      endRange = new Date(query.absoluteEndTime / 1000);

      // here, we have to do a little work. Normally a day query will start
      // at midnight and end exactly 24 hours later. However, this spans two
      // actual days, and will show up as such in the calendar. Therefore, if
      // the end day is exactly midnight, we will bump it back a day.
      if (endRange.getHours() == 0 && endRange.getMinutes() == 0 &&
          endRange.getSeconds() == 0 && endRange.getMilliseconds() == 0) {
        // Here, we have to be careful to not set the end range to before the
        // beginning. Somebody stupid might set them to be the same, and we
        // don't want to suddenly make an invalid range.
        if (! beginRange ||
            (beginRange && beginRange.getTime() != endRange.getTime()))
          endRange.setTime(endRange.getTime() - 1);
      }
    }
    calendar.setRange(beginRange, endRange, true);

    // Allow user selection events once again.
    calendar.suppressRangeEvents = false;
  },

  /**
   * Update the Places UI when the content of the right tree changes. 
   */
  onContentChanged: function PP_onContentChanged() {
    var panelID = "commands_history";
    
    var isBookmarks = this._content.isBookmarks;
    var node = asQuery(this._content.getResult().root);
    var queries = node.getQueries({});
    if (queries[0].annotation.indexOf("livemark/") > -1)
      panelID = "commands_feed";
    else if (isBookmarks)
      panelID = "commands_bookmark";

    var commandBar = document.getElementById("commandBar");
    commandBar.selectedPanel = document.getElementById(panelID);

    // Hide the Calendar for Bookmark queries. 
    document.getElementById("historyCalendar").setAttribute("hidden", isBookmarks);
    
    // Update the calendar with the current date range, if applicable. 
    if (!isBookmarks) {
      this._updateCalendar();
    }
  },

  /**
   * Returns the query array associated with the query currently loaded in
   * the main places pane.
   */
  getCurrentQueries: function PP_getCurrentQueries() {
    var result = this._content.getResult();
    return result.root.QueryInterface(Ci.nsINavHistoryQueryResultNode).getQueries({});
  },

  /**
   * Returns the options associated with the query currently loaded in the
   * main places pane.
   */
  getCurrentOptions: function PP_getCurrentOptions() {
    var result = this._content.getResult();
    return result.root.QueryInterface(Ci.nsINavHistoryQueryResultNode).queryOptions;
  }
};

/**
 * A set of utilities relating to search within Bookmarks and History. 
 */
var PlacesSearchBox = {
  /**
   * Gets/sets the active collection from the dropdown menu.
   */
  get filterCollection() {
    var searchFilter = document.getElementById("searchFilter");
    return searchFilter.getAttribute("collection");
  },
  set filterCollection(collectionName) {
    var searchFilter = document.getElementById("searchFilter");
    searchFilter.setAttribute("collection", collectionName);
    return collectionName;
  },
  
  /**
   * Focus the search box
   */
  focus: function PS_focus() {
    var searchFilter = document.getElementById("searchFilter");
    searchFilter.focus();
  },
  
  /**
   * When the field is activated, if the contents are the gray text, clear
   * the field, otherwise select the contents. 
   */
  onFocus: function PS_onFocus() {
    var searchFilter = document.getElementById("searchFilter");
    var placeBundle = document.getElementById("placeBundle");
    
    var searchDefault = placeBundle.getString("searchDefault");
    if (searchFilter.value == searchDefault) {
      searchFilter.removeAttribute("empty");
      searchFilter.value = "";
    }
    else
      searchFilter.select();
  },
  
  /**
   * When the field is deactivated, reset the gray text if the value is
   * empty or has the gray text value. 
   */
  onBlur: function PS_onBlur() {
    var placeBundle = document.getElementById("placeBundle");
    var searchDefault = placeBundle.getString("searchDefault");
    var searchFilter = document.getElementById("searchFilter");
    
    if (searchFilter.value == searchDefault || !searchFilter.value) {
      searchFilter.setAttribute("empty", "true");
      searchFilter.value = searchDefault;
    }    
  },
  
  /** 
   * Set up the gray text in the search bar as the Places View loads. 
   */
  init: function PS_init() {
    var placeBundle = document.getElementById("placeBundle");
    var searchDefault = placeBundle.getString("searchDefault");
    var searchFilter = document.getElementById("searchFilter");
    searchFilter.value = searchDefault;
    searchFilter.setAttribute("empty", "true");
    searchFilter.focus();
  }

};

/**
 * Functions and data for advanced query builder
 */
var PlacesQueryBuilder = {

  _numRows: 1,
  _maxRows: 4,
  
  _keywordSearch: {
    advancedSearch_N_Subject: "advancedSearch_N_SubjectKeyword",
    advancedSearch_N_LocationMenulist: false,
    advancedSearch_N_TimeMenulist: false,
    advancedSearch_N_Textbox: "",
    advancedSearch_N_TimePicker: false,
    advancedSearch_N_TimeMenulist2: false
  },
  _locationSearch: {
    advancedSearch_N_Subject: "advancedSearch_N_SubjectLocation",
    advancedSearch_N_LocationMenulist: "advancedSearch_N_LocationMenuSelected",
    advancedSearch_N_TimeMenulist: false,
    advancedSearch_N_Textbox: "",
    advancedSearch_N_TimePicker: false,
    advancedSearch_N_TimeMenulist2: false
  },
  _timeSearch: {
    advancedSearch_N_Subject: "advancedSearch_N_SubjectVisited",
    advancedSearch_N_LocationMenulist: false,
    advancedSearch_N_TimeMenulist: true,
    advancedSearch_N_Textbox: false,
    advancedSearch_N_TimePicker: "date",
    advancedSearch_N_TimeMenulist2: false
  },
  _timeInLastSearch: {
    advancedSearch_N_Subject: "advancedSearch_N_SubjectVisited",
    advancedSearch_N_LocationMenulist: false,
    advancedSearch_N_TimeMenulist: true,
    advancedSearch_N_Textbox: "7",
    advancedSearch_N_TimePicker: false,
    advancedSearch_N_TimeMenulist2: true
  },
  _nextSearch: null,
  _queryBuilders: null,
  
  init: function PQB_init() {
    // Initialize advanced search
    this._nextSearch = {
      "keyword": this._timeSearch,
      "visited": this._hostSearch,
      "location": this._locationSearch
    };
    
    this._queryBuilders = {
      "keyword": this.setKeywordQuery,
      "visited": this.setVisitedQuery,
      "location": this.setLocationQuery
    };
    
    this._dateService = Cc["@mozilla.org/intl/scriptabledateformat;1"].
                          getService(Ci.nsIScriptableDateFormat);
  },

  toggle: function PQB_toggle() {
    var advancedSearch = document.getElementById("advancedSearch");
    if (advancedSearch.collapsed) {
      // Need to expand the advanced search box and initialize it.
      
      // Should have one row, containing a keyword search with
      // the keyword from the basic search box pre-filled.
      while (this._numRows > 1)
        this.removeRow();
      this.showSearch(1, this._keywordSearch);
      var searchbox = document.getElementById("searchFilter");
      var keywordbox = document.getElementById("advancedSearch1Textbox");
      keywordbox.value = searchbox.value;
      advancedSearch.collapsed = false;
      
      // Update the +/- button and the header.
      var button = document.getElementById("moreCriteria");
      var placeBundle = document.getElementById("placeBundle");
      button.label = placeBundle.getString("lessCriteria.label");
      PlacesOrganizer._setHeader("advanced", "");
    }
    else {
      // Need to collapse the advanced search box.
      advancedSearch.collapsed = true;
      
      // Update the +/- button
      var button = document.getElementById("moreCriteria");
      var placeBundle = document.getElementById("placeBundle");
      button.label = placeBundle.getString("moreCriteria.label");
    }
  },
  
  setRowId: function PQB_setRowId(element, rowId) {
    if (element.id)
      element.id = element.id.replace("advancedSearch1", "advancedSearch" + rowId);
    if (element.hasAttribute('rowid'))
      element.setAttribute('rowid', rowId);
    for (var i = 0; i < element.childNodes.length; i++) {
      this.setRowId(element.childNodes[i], rowId);
    }
  },
  
  updateUIForRowChange: function PQB_updateUIForRowChange() {
    // Titlebar should show "match any/all" iff there are > 1 queries visible.
    var matchUI = document.getElementById("titlebarMatch");
    matchUI.hidden = (this._numRows <= 1);
    PlacesOrganizer._setHeader("advanced", this._numRows <= 1 ? "" : ",");
    
    // Disable the + buttons if there are max advanced search rows
    // Disable the - button is there is only one advanced search row
    for (var i = 1; i <= this._numRows; i++) {
      var plus = document.getElementById("advancedSearch" + i + "Plus");
      plus.disabled = (this._numRows >= this._maxRows);
      var minus = document.getElementById("advancedSearch" + i + "Minus");
      minus.disabled = (this._numRows == 1);
    }
  },
  
  addRow: function PQB_addRow() {
    if (this._numRows >= this._maxRows)
      return;
    
    var gridRows = document.getElementById("advancedSearchRows");
    var newRow = gridRows.firstChild.cloneNode(true);

    var searchType = this._keywordSearch;
    var lastMenu = document.getElementById("advancedSearch" +
                                           this._numRows +
                                           "Subject");
    if (lastMenu && lastMenu.selectedItem) {
      searchType = this._nextSearch[lastMenu.selectedItem.value];
    }
    
    this._numRows++;
    this.setRowId(newRow, this._numRows);
    this.showSearch(this._numRows, searchType);
    gridRows.appendChild(newRow);
    this.updateUIForRowChange();
  },
  
  removeRow: function PQB_removeRow() {
    if (this._numRows <= 1)
      return;

    var row = document.getElementById("advancedSearch" + this._numRows + "Row");
    row.parentNode.removeChild(row);
    this._numRows--;
    
    this.updateUIForRowChange();
  },
  
  onDateTyped: function PQB_onDateTyped(event, row) {
    var textbox = document.getElementById("advancedSearch" + row + "TimePicker");
    var dateString = textbox.value;
    var dateArr = dateString.split("-");
    // The date can be split into a range by the '-' character, i.e.
    // 9/5/05 - 10/2/05.  Unfortunately, dates can also be written like
    // 9-5-05 - 10-2-05.  Try to parse the date based on how many hyphens
    // there are.
    var d0 = null;
    var d1 = null;
    // If there are an even number of elements in the date array, try to 
    // parse it as a range of two dates.
    if ((dateArr.length & 1) == 0) {
      var mid = dateArr.length / 2;
      var dateStr0 = dateArr[0];
      var dateStr1 = dateArr[mid];
      for (var i = 1; i < mid; i++) {
        dateStr0 += "-" + dateArr[i];
        dateStr1 += "-" + dateArr[i + mid];
      }
      d0 = new Date(dateStr0);
      d1 = new Date(dateStr1);
    }
    // If that didn't work, try to parse it as a single date.
    if (d0 == null || d0 == "Invalid Date") {
      d0 = new Date(dateString);
    }
    
    if (d0 != null && d0 != "Invalid Date") {
      // Parsing succeeded -- update the calendar.
      var calendar = document.getElementById("advancedSearch" + row + "Calendar");
      if (d0.getFullYear() < 2000)
        d0.setFullYear(2000 + (d0.getFullYear() % 100));
      if (d1 != null && d1 != "Invalid Date") {
        if (d1.getFullYear() < 2000)
          d1.setFullYear(2000 + (d1.getFullYear() % 100));
        calendar.updateSelection(d0, d1);
      }
      else {
        calendar.updateSelection(d0, d0);
      }
      
      // And update the search.
      this.doSearch();
    }
  },
  
  onCalendarChanged: function PQB_onCalendarChanged(event, row) {
    var calendar = document.getElementById("advancedSearch" + row + "Calendar");
    var begin = calendar.beginrange;
    var end = calendar.endrange;
    
    // If the calendar doesn't have a begin/end, don't change the textbox.
    if (begin == null || end == null)
      return true;
      
    // If the begin and end are the same day, only fill that into the textbox.
    var textbox = document.getElementById("advancedSearch" + row + "TimePicker");
    var beginDate = begin.getDate();
    var beginMonth = begin.getMonth() + 1;
    var beginYear = begin.getFullYear();
    var endDate = end.getDate();
    var endMonth = end.getMonth() + 1;
    var endYear = end.getFullYear();
    if (beginDate == endDate && beginMonth == endMonth && beginYear == endYear) {
      // Just one date.
      textbox.value = this._dateService.FormatDate("",
                                                   this._dateService.dateFormatShort,
                                                   beginYear,
                                                   beginMonth,
                                                   beginDate);
    }
    else
    {
      // Two dates.
      var beginStr = this._dateService.FormatDate("",
                                                   this._dateService.dateFormatShort,
                                                   beginYear,
                                                   beginMonth,
                                                   beginDate);
      var endStr = this._dateService.FormatDate("",
                                                this._dateService.dateFormatShort,
                                                endYear,
                                                endMonth,
                                                endDate);
      textbox.value = beginStr + " - " + endStr;
    }
    
    // Update the search.
    this.doSearch();
    
    return true;
  },
  
  handleTimePickerClick: function PQB_handleTimePickerClick(event, row) {
    var popup = document.getElementById("advancedSearch" + row + "DatePopup");
    if (popup.showing)
      popup.hidePopup();
    else {
      var textbox = document.getElementById("advancedSearch" + row + "TimePicker");
      popup.showPopup(textbox, -1, -1, "popup", "bottomleft", "topleft");
    }
  },
  
  showSearch: function PQB_showSearch(row, values) {
    for (val in values) {
      var id = val.replace("_N_", row);
      var element = document.getElementById(id);
      if (values[val] || typeof(values[val]) == "string") {
        if (typeof(values[val]) == "string") {
          if (values[val] == "date") {
            // "date" means that the current date should be filled into the
            // textbox, and the calendar for the row updated.
            var d = new Date();
            element.value = this._dateService.FormatDate("",
                                                         this._dateService.dateFormatShort,
                                                         d.getFullYear(),
                                                         d.getMonth() + 1,
                                                         d.getDate());
            var calendar = document.getElementById("advancedSearch" + row + "Calendar");
            calendar.updateSelection(d, d);
          }
          else if (element.nodeName == "textbox") {
            // values[val] is the initial value of the textbox.
            element.value = values[val];
          } else {
            // values[val] is the menuitem which should be selected.
            var itemId = values[val].replace("_N_", row);
            var item = document.getElementById(itemId);
            element.selectedItem = item;
          }
        }
        element.hidden = false;
      }
      else {
        element.hidden = true;
      }
    }
    
    this.doSearch();
  },
  
  setKeywordQuery: function PQB_setKeywordQuery(query, prefix) {
    query.searchTerms += document.getElementById(prefix + "Textbox").value + " ";
  },
  
  setLocationQuery: function PQB_setLocationQuery(query, prefix) {
    var type = document.getElementById(prefix + "LocationMenulist").selectedItem.value;
    if (type == "onsite") {
      query.domain = document.getElementById(prefix + "Textbox").value;
    }
    else {
      query.uriIsPrefix = (type == "startswith");
      var ios = Cc["@mozilla.org/network/io-service;1"].
                  getService(Ci.nsIIOService);
      var spec = document.getElementById(prefix + "Textbox").value;
      query.uri = ios.newURI(spec, null, null);
    }
  },
  
  setVisitedQuery: function PQB_setVisitedQuery(query, prefix) {
    var searchType = document.getElementById(prefix + "TimeMenulist").selectedItem.value;
    const DAY_MSEC = 86400000;
    switch (searchType) {
      case "on":
        var calendar = document.getElementById(prefix + "Calendar");
        var begin = calendar.beginrange.getTime();
        var end = calendar.endrange.getTime();
        if (begin == end) {
          end = begin + DAY_MSEC;
        }
        query.beginTime = begin * 1000;
        query.endTime = end * 1000;
        break;
      case "before":
        var calendar = document.getElementById(prefix + "Calendar");
        var time = calendar.beginrange.getTime();
        query.endTime = time * 1000;
        break;
      case "after":
        var calendar = document.getElementById(prefix + "Calendar");
        var time = calendar.endrange.getTime();
        query.beginTime = time * 1000;
        break;
      case "inLast":
        var textbox = document.getElementById(prefix + "Textbox");
        var amount = parseInt(textbox.value);
        amount = amount * DAY_MSEC;
        var menulist = document.getElementById(prefix + "TimeMenulist2");
        if (menulist.selectedItem.value == "weeks")
          amount = amount * 7;
        else if (menulist.selectedItem.value == "months")
          amount = amount * 30;
        var now = new Date();
        now = now - amount;
        query.beginTime = now * 1000;
        break;
    }
  },
  
  doSearch: function PQB_doSearch() {
    // Create the individual queries.
    var queryType = document.getElementById("advancedSearchType").selectedItem.value;
    var queries = [];
    if (queryType == "and")
      queries.push(PlacesController.history.getNewQuery());
    for (var i = 1; i <= this._numRows; i++) {
      var prefix = "advancedSearch" + i;
      
      // If the queries are being AND-ed, put all the rows in one query.
      // If they're being OR-ed, add a separate query for each row.
      var query;
      if (queryType == "and")
        query = queries[0];
      else
        query = PlacesController.history.getNewQuery();
      
      var querySubject = document.getElementById(prefix + "Subject").value;
      this._queryBuilders[querySubject](query, prefix);
      
      if (queryType == "or")
        queries.push(query);
    }
    
    // Make sure we're getting uri results, not visits
    var options = PlacesOrganizer.getCurrentOptions();
    options.resultType = options.RESULT_TYPE_URI;

    PlacesOrganizer._content.load(queries, options);
  }
};
