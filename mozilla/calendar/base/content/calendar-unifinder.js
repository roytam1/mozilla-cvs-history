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
 * The Original Code is OEone Calendar Code, released October 31st, 2001.
 *
 * The Initial Developer of the Original Code is
 * OEone Corporation.
 * Portions created by the Initial Developer are Copyright (C) 2001
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Garth Smedley <garths@oeone.com>
 *   Mike Potter <mikep@oeone.com>
 *   Chris Charabaruk <coldacid@meldstar.com>
 *   Colin Phillips <colinp@oeone.com>
 *   ArentJan Banck <ajbanck@planet.nl>
 *   Eric Belhaire <eric.belhaire@ief.u-psud.fr>
 *   Matthew Willis <mattwillis@gmail.com>
 *   Michiel van Leeuwen <mvl@exedo.nl>
 *   Joey Minta <jminta@gmail.com>
 *   Dan Mosedale <dan.mosedale@oracle.com>
 *   Michael Buettner <michael.buettner@sun.com>
 *   Philipp Kewisch <mozilla@kewis.ch>
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

/**
 * U N I F I N D E R
 *
 * This is a hacked in interface to the unifinder. We will need to
 * improve this to make it usable in general.
 */
var kEventStatusOrder = ["TENTATIVE", "CONFIRMED", "CANCELLED"];

// Set this to true when the calendar event tree is clicked to allow for
// multiple selection
var gCalendarEventTreeClicked = false;

// Store the start and enddate, because the providers can't be trusted when
// dealing with all-day events. So we need to filter later. See bug 306157
var gStartDate;
var gEndDate;

var kDefaultTimezone;
var gUnifinderNeedsRefresh = true;

function isUnifinderHidden() {
    return document.getElementById("bottom-events-box").hidden;
}

// Extra check to see if the events are in the daterange. Some providers
// are broken when looking at all-day events.
function fixAlldayDates(aItem) {
    // Using .compare on the views start and end, not on the events dates,
    // because .compare uses the timezone of the datetime it is called on.
    // The view's timezone is what is important here.
    return ((!gEndDate || gEndDate.compare(aItem.startDate) >= 0) &&
            (!gStartDate || gStartDate.compare(aItem.endDate) < 0));
}

function getCurrentUnifinderFilter() {
    return document.getElementById("event-filter-menulist").selectedItem.value;
}

/**
 * Observer for the calendar event data source. This keeps the unifinder
 * display up to date when the calendar event data is changed
 */

var unifinderObserver = {
    mInBatch: false,

    QueryInterface: function uO_QueryInterface (aIID) {
        if (!aIID.equals(Components.interfaces.nsISupports) &&
            !aIID.equals(Components.interfaces.calICompositeObserver) &&
            !aIID.equals(Components.interfaces.calIObserver)) {
            throw Components.results.NS_ERROR_NO_INTERFACE;
        }

        return this;
    },

    // calIObserver:
    onStartBatch: function uO_onStartBatch() {
        this.mInBatch = true;
    },

    onEndBatch: function uO_onEndBatch() {
        this.mInBatch = false;
        refreshEventTree();
    },

    onLoad: function uO_onLoad() {
        if (isUnifinderHidden() && !gUnifinderNeedsRefresh) {
            // If the unifinder is hidden, all further item operations might
            // produce invalid entries in the unifinder. From now on, ignore
            // those operations and refresh as soon as the unifinder is shown
            // again.
            gUnifinderNeedsRefresh = true;
            unifinderTreeView.clearItems();
        }
        if (!this.mInBatch) {
            refreshEventTree();
        }
    },

    onAddItem: function uO_onAddItem(aItem) {
        if (isEvent(aItem) &&
            !this.mInBatch &&
            !gUnifinderNeedsRefresh &&
            isItemInFilter(aItem)) {
            this.addItemToTree(aItem);
        }
    },

    onModifyItem: function uO_onModifyItem(aNewItem, aOldItem) {
        this.onDeleteItem(aOldItem);
        this.onAddItem(aNewItem);
    },

    onDeleteItem: function uO_onDeleteItem(aDeletedItem) {
        if (isEvent(aDeletedItem) && !this.mInBatch && !gUnifinderNeedsRefresh) {
            this.removeItemFromTree(aDeletedItem);
        }
    },

    // It is safe to call these for any event.  The functions will determine
    // whether or not anything actually needs to be done to the tree
    addItemToTree: function uO_addItemToTree(aItem) {
        var items;
        if (gStartDate && gEndDate) {
            items = aItem.getOccurrencesBetween(gStartDate, gEndDate, {});
        } else {
            items = [aItem];
        }
        unifinderTreeView.addItems(items.filter(fixAlldayDates));
    },
    removeItemFromTree: function uO_removeItemFromTree(aItem) {
        var items;
        if (gStartDate && gEndDate && (aItem.parentItem == aItem)) {
            items = aItem.getOccurrencesBetween(gStartDate, gEndDate, {});
        } else {
            items = [aItem];
        }
        unifinderTreeView.removeItems(items.filter(fixAlldayDates));
    },

    onError: function uO_onError(aCalendar, aErrNo, aMessage) {},

    onPropertyChanged: function uO_onPropertyChanged(aCalendar, aName, aValue, aOldValue) {
        switch (aName) {
            case "disabled":
                refreshEventTree();
                break;
        }
    },

    onPropertyDeleting: function uO_onPropertyDeleting(aCalendar, aName) {
      this.onPropertyChanged(aCalendar, aName, null, null);
    },

    // calICompositeObserver:
    onCalendarAdded: function uO_onCalendarAdded(aAddedCalendar) {
        if (!this.mInBatch && !aAddedCalendar.getProperty("disabled")) {
            refreshEventTree();
        }
    },

    onCalendarRemoved: function uO_onCalendarRemoved(aDeletedCalendar) {
        // TODO only remove such items that belong to the calendar
        if (!this.mInBatch && !aDeletedCalendar.getProperty("disabled")) {
            refreshEventTree();
        }
    },

    onDefaultCalendarChanged: function uO_onDefaultCalendarChanged(aNewDefaultCalendar) {}
};

/**
 * Called when the calendar is loaded
 */
function prepareCalendarUnifinder() {
    // Only load once
    window.removeEventListener("load", prepareCalendarUnifinder, false);
    var unifinderTree = document.getElementById("unifinder-search-results-tree");

    // Check if this is not the hidden window, which has no UI elements
    if (unifinderTree) {
        // set up our calendar event observer
        var ccalendar = getCompositeCalendar();
        ccalendar.addObserver(unifinderObserver);

        kDefaultTimezone = calendarDefaultTimezone();

        // Set up the unifinder views.
        unifinderTreeView.treeElement = unifinderTree;
        unifinderTree.view = unifinderTreeView;

        // Listen for changes in the selected day, so we can update if need be
        var viewDeck = getViewDeck();
        viewDeck.addEventListener("dayselect", unifinderDaySelect, false);
        viewDeck.addEventListener("itemselect", unifinderItemSelect, true);

        // Set up sortDirection and sortActive, in case it persisted
        var active = document.getElementById("unifinder-search-results-tree-cols")
                             .getElementsByAttribute("sortActive", "true");
        if (active.length > 0) {
            unifinderTreeView.selectedColumn = active[0].id;
            unifinderTreeView.sortDirection = active[0].getAttribute("sortDirection");
        }

        // Display something upon first load. onLoad doesn't work properly for
        // observers
        if (!isUnifinderHidden()) {
            gUnifinderNeedsRefresh = false;
            refreshEventTree();
        }
    }
}

/**
 * Called when the calendar is unloaded
 */
function finishCalendarUnifinder() {
    var ccalendar = getCompositeCalendar();
    ccalendar.removeObserver(unifinderObserver);

    var viewDeck = getViewDeck();
    if (viewDeck) {
        viewDeck.removeEventListener("dayselect", unifinderDaySelect, false);
        viewDeck.removeEventListener("itemselect", unifinderItemSelect, true);
    }
}

/**
 * Event listeners for dayselect and itemselect events
 */
function unifinderDaySelect() {
    if (getCurrentUnifinderFilter() == "current") {
        refreshEventTree();
    }
}

function unifinderItemSelect(aEvent) {
    unifinderTreeView.setSelectedItems(aEvent.detail);
}

/**
 * Helper function to display event datetimes in the unifinder
 */
function formatUnifinderEventDateTime(aDatetime) {
    var dateFormatter = Components.classes["@mozilla.org/calendar/datetime-formatter;1"]
                                  .getService(Components.interfaces.calIDateTimeFormatter);
    return dateFormatter.formatDateTime(aDatetime.getInTimezone(kDefaultTimezone));
}

/**
 *  This is called from the unifinder when a key is pressed in the search field
 */
var gSearchTimeout = null;

function searchKeyPress(searchTextItem, event) {
    // 13 == return
    if (event && event.keyCode == 13) {
        clearSearchTimer();
        refreshEventTree();
        return;
    }

    // Always clear the old one first
    clearSearchTimer();

    // Make a new timer
    gSearchTimeout = setTimeout("refreshEventTree()", 400);
}

function clearSearchTimer() {
   if (gSearchTimeout) {
      clearTimeout(gSearchTimeout);
      gSearchTimeout = null;
   }
}

/**
 * Unifinder event handlers (click,select,etc)
 */
function unifinderDoubleClick(event) {
    // We only care about button 0 (left click) events
    if (event.button != 0) {
        return;
    }

    // find event by id
    var calendarEvent = unifinderTreeView.getItemFromEvent(event);

    if (calendarEvent != null) {
        modifyEventWithDialog(calendarEvent, null, true);
    } else {
        createEventWithDialog();
    }
}

function unifinderSelect(event) {
    var tree = unifinderTreeView.treeElement;
    if (!tree.view.selection || tree.view.selection.getRangeCount() == 0) {
        return;
    }

    var selectedItems = [];
    gCalendarEventTreeClicked = true;

    // Get the selected events from the tree
    var start = {};
    var end = {};
    var numRanges = tree.view.selection.getRangeCount();

    for (var t = 0; t < numRanges; t++) {
        tree.view.selection.getRangeAt(t, start, end);

        for (var v = start.value; v <= end.value; v++) {
            try {
                selectedItems.push(unifinderTreeView.getItemAt(v));
            } catch (e) {
               WARN("Error getting Event from row: " + e + "\n");
            }
        }
    }

    if (selectedItems.length == 1) {
        // Go to the day of the selected item in the current view.
        currentView().goToDay(selectedItems[0].startDate);
    }

    // Set up the selected items in the view. Pass in true, so we don't end
    // up in a circular loop
    currentView().setSelectedItems(selectedItems.length, selectedItems, true);
    currentView().centerSelectedItems();
    calendarController.onSelectionChanged({detail: selectedItems});
}

function unifinderKeyPress(aEvent) {
    const kKE = Components.interfaces.nsIDOMKeyEvent;
    switch (aEvent.keyCode) {
        case 13:
            // Enter, edit the event
            editSelectedEvents();
            aEvent.stopPropagation();
            aEvent.preventDefault();
            break;
        case kKE.DOM_VK_BACK_SPACE:
        case kKE.DOM_VK_DELETE:
            deleteSelectedEvents();
            aEvent.stopPropagation();
            aEvent.preventDefault();
            break;
    }
}

/**
 * Tree controller for unifinder search results
 */
var unifinderTreeView = {

    tree: null,
    treeElement: null,
    doingSelection: false,

    /**
     * Event functions
     */

    eventArray: [],
    eventIndexMap: {},

    addItems: function uTV_addItems(aItemArray, aDontSort) {
        this.eventArray = this.eventArray.concat(aItemArray);
        if (this.tree) {
            var newCount = this.eventArray.length - aItemArray.length - 1;
            this.tree.rowCountChanged(newCount, aItemArray.length);
        }

        if (aDontSort) {
            this.calculateIndexMap();
        } else {
            this.sortItems();
        }
    },

    removeItems: function uTV_removeItems(aItemArray) {
        for each (var item in aItemArray) {
            var row = this.getItemRow(item);
            if (row > -1) {
                this.eventArray.splice(row, 1);
                if (this.tree) {
                    this.tree.rowCountChanged(row, -1);
                }
            }
        }
        this.calculateIndexMap();
    },

    clearItems: function uTV_clearItems() {
        var oldCount = this.eventArray.length;
        this.eventArray = [];
        if (this.tree) {
            this.tree.rowCountChanged(0, -oldCount);
        }
        this.calculateIndexMap();
    },

    setItems: function uTV_setItems(aItemArray, aDontSort) {
        var oldCount = this.eventArray.length;
        this.eventArray = aItemArray.slice(0);
        if (this.tree) {
            this.tree.rowCountChanged(0, (this.eventArray.length - oldCount));
        }
       
        if (aDontSort) {
            //this.calculateIndexMap();
        } else {
            this.sortItems();
        }
    },

    calculateIndexMap: function uTV_calculateIndexMap() {
        this.eventIndexMap = {};
        for (var i = 0 ; i < this.eventArray.length; i++) {
            this.eventIndexMap[this.eventArray[i].hashId] = i;
        }

        if (this.tree) {
            this.tree.invalidate();
        }
    },

    sortItems: function uTV_sortItems() {
        // Get a current locale string collator for compareEvents
        if (!this.localeCollator) {
            var localeService =
                Components
                .classes["@mozilla.org/intl/nslocaleservice;1"]
                .getService(Components.interfaces.nsILocaleService);
            this.localeCollator =
                Components
                .classes["@mozilla.org/intl/collation-factory;1"]
                .getService(Components.interfaces.nsICollationFactory)
                .CreateCollation(localeService.getApplicationLocale());
        }

        this.sortStartedTime = new Date().getTime(); // for null/0 dates in sort

        // sort (key,item) entries
        var entries = this.eventArray.map(sortEntry);
        entries.sort(sortEntryComparer(this));
        this.eventArray = entries.map(sortEntryItem);

        this.calculateIndexMap();
    },

    getItemRow: function uTV_getItemRow(item) {
        if (this.eventIndexMap[item.hashId] === undefined) {
            return -1;
        }
        return this.eventIndexMap[item.hashId];
    },

    getItemAt: function uTV_getItemAt(aRow) {
        return this.eventArray[aRow];
    },

    /**
     * Get the calendar item from the given event
     */
    getItemFromEvent: function uTV_getItemFromEvent(event) {
        var row = this.tree.getRowAt(event.clientX, event.clientY);

        if (row > -1) {
            return this.getItemAt(row);
        }
        return null;
    },

    setSelectedItems: function uTV_setSelectedItems(aItemArray) {
        if (this.doingSelection || !this.tree) {
            return;
        }

        this.doingSelection = true;

        // If no items were passed, get the selected items from the view.
        aItemArray = aItemArray || currentView().getSelectedItems({});

        /**
         * The following is a brutal hack, caused by
         * http://lxr.mozilla.org/mozilla1.0/source/layout/xul/base/src/tree/src/nsTreeSelection.cpp#555
         * and described in bug 168211
         * http://bugzilla.mozilla.org/show_bug.cgi?id=168211
         * Do NOT remove anything in the next 3 lines, or the selection in the tree will not work.
         */
        this.treeElement.onselect = null;
        this.treeElement.removeEventListener("select", unifinderSelect, true);
        this.tree.view.selection.selectEventsSuppressed = true;
        this.tree.view.selection.clearSelection();

        if (aItemArray && aItemArray.length == 1) {
            // If only one item is selected, scroll to it
            var rowToScrollTo = this.getItemRow(aItemArray[0]);
            if (rowToScrollTo > -1) {
               this.tree.ensureRowIsVisible(rowToScrollTo);
               this.tree.view.selection.select(rowToScrollTo);
            }
        } else if (aItemArray && aItemArray.length > 1) {
            // If there is more than one item, just select them all.
            for (var i in aItemArray) {
                var row = this.getItemRow(aItemArray[i]);
                this.tree.view.selection.rangedSelect(row, row, true);
            }
        }

        // This needs to be in a setTimeout
        setTimeout("unifinderTreeView.resetAllowSelection()", 1);
    },

    resetAllowSelection: function uTV_resetAllowSelection() {
        if (!this.tree) {
            return;
        }
        /**
         * Do not change anything in the following lines, they are needed as
         * described in the selection observer above
         */
        this.doingSelection = false;

        this.tree.view.selection.selectEventsSuppressed = false;
        this.treeElement.addEventListener("select", unifinderSelect, true);
    },

    /**
     * Tree View Implementation
     */
    get rowCount uTV_getRowCount() {
        return this.eventArray.length;
    },


    // TODO this code is currently identical to the task tree. We should create
    // an itemTreeView that these tree views can inherit, that contains this
    // code, and possibly other code related to sorting and storing items. See
    // bug 432582 for more details.
    getCellProperties: function uTV_getCellProperties(aRow, aCol, aProps) {
        this.getRowProperties(aRow, aProps);
        this.getColumnProperties(aCol, aProps);
    },
    getRowProperties: function uTV_getRowProperties(aRow, aProps) {
        var item = this.eventArray[aRow];
        if (item.priority > 0 && item.priority < 5) {
            aProps.AppendElement(getAtomFromService("highpriority"));
        } else if (item.priority > 5 && item.priority < 10) {
            aProps.AppendElement(getAtomFromService("lowpriority"));
        }

        // Add calendar name atom
        var calendarAtom = "calendar-" + formatStringForCSSRule(item.calendar.name);
        aProps.AppendElement(getAtomFromService(calendarAtom));

        // Add item status atom
        if (item.status) {
            aProps.AppendElement(getAtomFromService("status-" + item.status.toLowerCase()));
        }

        // Alarm status atom
        if (item.alarmOffset) {
            aProps.AppendElement(getAtomFromService("alarm"));
        }

        // Task categories
        item.getCategories({}).map(formatStringForCSSRule)
                              .map(getAtomFromService)
                              .forEach(aProps.AppendElement, aProps);
    },
    getColumnProperties: function uTV_getColumnProperties(aCol, aProps) {},

    isContainer: function uTV_isContainer() {
        return false;
    },

    isContainerOpen: function uTV_isContainerOpen(aRow) {
        return false;
    },

    isContainerEmpty: function uTV_isContainerEmpty(aRow) {
        return false;
    },

    isSeparator: function uTV_isSeparator(aRow) {
        return false;
    },

    isSorted: function uTV_isSorted(aRow) {
        return false;
    },

    canDrop: function uTV_canDrop(aRow, aOrientation) {
        return false;
    },

    drop: function uTV_drop(aRow, aOrientation) {},

    getParentIndex: function uTV_getParentIndex(aRow) {
        return -1;
    },

    hasNextSibling: function uTV_hasNextSibling(aRow, aAfterIndex) {},

    getLevel: function uTV_getLevel(aRow) {
        return 0;
    },

    getImageSrc: function uTV_getImageSrc(aRow, aOrientation) {},

    getProgressMode: function uTV_getProgressMode(aRow, aCol) {},

    getCellValue: function uTV_getCellValue(aRow, aCol) {
        return null;
    },

    getCellText: function uTV_getCellText(row, column) {
        calendarEvent = this.eventArray[row];

        switch (column.id) {
            case "unifinder-search-results-tree-col-title":
                return calendarEvent.title;

            case "unifinder-search-results-tree-col-startdate":
                return formatUnifinderEventDateTime(calendarEvent.startDate);

            case "unifinder-search-results-tree-col-enddate":
                var eventEndDate = calendarEvent.endDate.clone();
                // XXX reimplement
                //var eventEndDate = getCurrentNextOrPreviousRecurrence(calendarEvent);
                if (calendarEvent.startDate.isDate) {
                    // display enddate is ical enddate - 1
                    eventEndDate.day = eventEndDate.day - 1;
                }
                return formatUnifinderEventDateTime(eventEndDate);

            case "unifinder-search-results-tree-col-categories":
                return calendarEvent.getCategories({}).join(", ");

            case "unifinder-search-results-tree-col-location":
                return calendarEvent.getProperty("LOCATION");

            case "unifinder-search-results-tree-col-status":
                return getEventStatusString(calendarEvent);

            case "unifinder-search-results-tree-col-calendarname":
                return calendarEvent.calendar.name;

            default:
                return false;
        }
    },

    setTree: function uTV_setTree(tree) {
        this.tree = tree;
    },

    toggleOpenState: function uTV_toggleOpenState(aRow) {},

    cycleHeader: function uTV_cycleHeader(col) {

        var sortActive = col.element.getAttribute("sortActive");
        this.selectedColumn = col.id;
        this.sortDirection = col.element.getAttribute("sortDirection");

        if (sortActive != "true") {
            var unifinder = document.getElementById("unifinder-search-results-tree");
            var treeCols = unifinder.getElementsByTagName("treecol");
            for (var i = 0; i < treeCols.length; i++) {
                treeCols[i].removeAttribute("sortActive");
                treeCols[i].removeAttribute("sortDirection");
            }
            this.sortDirection = "ascending";
        } else {
            if (!this.sortDirection || this.sortDirection == "descending") {
                this.sortDirection = "ascending";
            } else {
                this.sortDirection = "descending";
            }
        }
        col.element.setAttribute("sortActive", "true");
        col.element.setAttribute("sortDirection", this.sortDirection);

        this.sortItems();
    },

    isEditable: function uTV_isEditable(aRow, aCol) {
        return false;
    },

    setCellValue: function uTV_setCellValue(aRow, aCol, aValue) {},
    setCellText: function uTV_setCellText(aRow, aCol, aValue) {},

    performAction: function uTV_performAction(aAction) {},

    performActionOnRow: function uTV_performActionOnRow(aAction, aRow) {},

    performActionOnCell: function uTV_performActionOnCell(aAction, aRow, aCol) {},

    selectedColumn: null,
    sortDirection: null,
    sortStartedTime: new Date().getTime(), // updated just before sort
    outParameter: new Object() // used to obtain dates during sort
};

function getEventSortKey(calEvent) {
    switch(unifinderTreeView.selectedColumn) {
        case "unifinder-search-results-tree-col-title":
            return calEvent.title || "";

        case "unifinder-search-results-tree-col-startdate":
            return nativeTimeOrNow(calEvent.startDate);

        case "unifinder-search-results-tree-col-enddate":
            return nativeTimeOrNow(calEvent.endDate);

        case "unifinder-search-results-tree-col-categories":
            return calEvent.getCategories({}).join(", ");

        case "unifinder-search-results-tree-col-location":
            return calEvent.getProperty("LOCATION") || "";

        case "unifinder-search-results-tree-col-status":
            return calEvent.status || "";

        case "unifinder-search-results-tree-col-calendarname":
            return calEvent.calendar.name || "";

        default:
            return null;
    }
}

function sortEntry (aItem) {
    return {mSortKey : getEventSortKey(aItem), mItem: aItem};
}
function sortEntryItem(sortEntry) { 
    return sortEntry.mItem;
}
function sortEntryKey(sortEntry) {
    return sortEntry.mSortKey;
}


function sortEntryComparer(unifinderTreeView) {
    var modifier = (unifinderTreeView.sortDirection == "descending" ? -1 : 1);
    var collator = unifinderTreeView.localeCollator;
    switch (unifinderTreeView.selectedColumn) {
        case "unifinder-search-results-tree-col-startdate":
        case "unifinder-search-results-tree-col-enddate":
            function compareTimes(sortEntryA, sortEntryB) { 
                var nsA = sortEntryKey(sortEntryA);
                var nsB = sortEntryKey(sortEntryB);
                return compareNativeTime(nsA, nsB) * modifier;
            }
            return compareTimes;

        case "unifinder-search-results-tree-col-title":
        case "unifinder-search-results-tree-col-categories":
        case "unifinder-search-results-tree-col-location":
        case "unifinder-search-results-tree-col-status":
        case "unifinder-search-results-tree-col-calendarname":
            function compareStrings(sortEntryA, sortEntryB) { 
                var sA = sortEntryKey(sortEntryA);
                var sB = sortEntryKey(sortEntryB);
                if (sA.length == 0 || sB.length == 0) {
                    // sort empty values to end (so when users first sort by a
                    // column, they can see and find the desired values in that
                    // column without scrolling past all the empty values).
                    return -(sA.length - sB.length) * modifier;
                }
                var comparison = collator.compareString(0, sA, sB);
                return comparison * modifier;
            }
            return compareStrings;

        default:
            function compareOther(sortEntryA, sortEntryB) {
                return 0;
            }
            return compareOther;
    }
}

function compareNativeTime(a, b) {
    return (a < b ? -1 :
            a > b ?  1 : 0);
}

function nativeTimeOrNow(calDateTime) {
    // Treat null/0 as 'now' when sort started, so incomplete tasks stay current.
    // Time is computed once per sort (just before sort) so sort is stable.
    if (calDateTime == null) {
        return unifinderTreeView.sortStartedTime;
    }
    var ns = calDateTime.nativeTime;
    if (ns == -62168601600000000) { // ns value for (0000/00/00 00:00:00)
        return unifinderTreeView.sortStartedTime;
    }
    return ns;
}

function refreshEventTree() {
    if (isUnifinderHidden()) {
        // If the unifinder is hidden, don't refresh the events to reduce needed
        // getItems calls.
        return;
    }
    var savedThis = this;
    var refreshListener = {
        mEventArray: new Array(),

        onOperationComplete: function rET_onOperationComplete(aCalendar,
                                                              aStatus,
                                                              aOperationType,
                                                              aId,
                                                              aDateTime) {
            var refreshTreeInternalFunc = function() {
                refreshEventTreeInternal(refreshListener.mEventArray);
            };
            setTimeout(refreshTreeInternalFunc, 0);
        },

        onGetResult: function rET_onGetResult(aCalendar,
                                              aStatus,
                                              aItemType,
                                              aDetail,
                                              aCount,
                                              aItems) {
            for (var i = 0; i < aCount; i++) {
                refreshListener.mEventArray.push(aItems[i]);
            }
        }
    };


    var ccalendar = getCompositeCalendar();
    var filter = 0;

    filter |= ccalendar.ITEM_FILTER_TYPE_EVENT;

    // Not all xul might be there yet...
    if (!document.getElementById("event-filter-menulist")) {
        return;
    }
    var [StartDate, EndDate] = getDatesForFilter(getCurrentUnifinderFilter());

    gStartDate = StartDate  ? jsDateToDateTime(StartDate, calendarDefaultTimezone()) : null;
    gEndDate = EndDate ? jsDateToDateTime(EndDate, calendarDefaultTimezone()) : null;
    if (StartDate && EndDate) {
        filter |= ccalendar.ITEM_FILTER_CLASS_OCCURRENCES;
    }

    ccalendar.getItems(filter, 0, gStartDate, gEndDate, refreshListener);
}

/**
 * Get the dates for a certain filter. This function makes it easy to extend the
 * unifinder. To add a new view, just overwrite this function with your own. Be
 * sure to call this function afterwards though.
 */
function getDatesForFilter(aFilter) {
    var Today = new Date();
    // Do this to allow all day events to show up all day long.
    var StartDate = new Date(Today.getFullYear(),
                             Today.getMonth(),
                             Today.getDate(),
                             0, 0, 0);
    var EndDate;
    switch (aFilter) {
        case "all":
            StartDate = null;
            EndDate = null;
            break;

        case "today":
            EndDate = new Date(StartDate.getTime() + (1000 * 60 * 60 * 24) - 1);
            break;

        case "next7Days":
            EndDate = new Date(StartDate.getTime() + (1000 * 60 * 60 * 24 * 8));
            break;

        case "next14Days":
            EndDate = new Date(StartDate.getTime() + (1000 * 60 * 60 * 24 * 15));
            break;

        case "next31Days":
            EndDate = new Date(StartDate.getTime() + (1000 * 60 * 60 * 24 * 32));
            break;

        case "thisCalendarMonth":
            // midnight on first day of this month
            var startOfMonth = new Date(Today.getFullYear(), Today.getMonth(), 1, 0, 0, 0);
            // midnight on first day of next month
            var startOfNextMonth = new Date(Today.getFullYear(), (Today.getMonth() + 1), 1, 0, 0, 0);
            // 23:59:59 on last day of this month
            EndDate = new Date(startOfNextMonth.getTime() - 1000);
            StartDate = startOfMonth;
            break;

        case "future":
            EndDate = null;
            break;

        case "current":
            var SelectedDate = currentView().selectedDay.jsDate;
            StartDate = new Date(SelectedDate.getFullYear(), SelectedDate.getMonth(), SelectedDate.getDate(), 0, 0, 0);
            EndDate = new Date(StartDate.getTime() + (1000 * 60 * 60 * 24) - 1000);
            break;
    }
    return [StartDate, EndDate];
}

function refreshEventTreeInternal(eventArray) {
    var searchText = document.getElementById("unifinder-search-field").value;

    unifinderTreeView.setItems(eventArray.filter(isItemInFilter));

    // Select selected events in the tree. Not passing the argument gets the
    // items from the view.
    unifinderTreeView.setSelectedItems();
}

function isItemInFilter(aItem) {
    var searchText = document.getElementById("unifinder-search-field")
                             .value.toLowerCase();

    if (!searchText.length || searchText.match(/^\s*$/)) {
        return true;
    }

    const fieldsToSearch = ["SUMMARY", "DESCRIPTION", "LOCATION", "URL"];
    if (!fixAlldayDates(aItem)) {
        return false;
    }

    for each (var field in fieldsToSearch) {
        var val = aItem.getProperty(field);
        if (val && val.toLowerCase().indexOf(searchText) != -1) {
            return true;
        }
    }

    return aItem.getCategories({}).some(
        function someFunc(cat) {
            return (cat.toLowerCase().indexOf(searchText) != -1);
        });
}

function focusSearch() {
    document.getElementById("unifinder-search-field").focus();
}

function toggleUnifinder() {
    // Toggle the elements
    goToggleToolbar('bottom-events-box', 'calendar_show_unifinder_command');
    goToggleToolbar('calendar-view-splitter');

    unifinderTreeView.treeElement.view = unifinderTreeView;

    // When the unifinder is hidden, refreshEventTree is not called. Make sure
    // the event tree is refreshed now.
    if (!isUnifinderHidden() && gUnifinderNeedsRefresh) {
        gUnifinderNeedsRefresh = false;
        refreshEventTree();
    }

    // Make sure the selection is correct
    if (unifinderTreeView.doingSelection) {
        unifinderTreeView.resetAllowSelection();
    }
    unifinderTreeView.setSelectedItems();
}

window.addEventListener("load", prepareCalendarUnifinder, false);
window.addEventListener("unload", finishCalendarUnifinder, false);
