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
 * The Original Code is Lightning code.
 *
 * The Initial Developer of the Original Code is Oracle Corporation
 * Portions created by the Initial Developer are Copyright (C) 2005
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Mike Shaver <shaver@mozilla.org>
 *   Vladimir Vukicevic <vladimir@pobox.com>
 *   Stuart Parmenter <stuart.parmenter@oracle.com>
 *   Dan Mosedale <dmose@mozilla.org>
 *   Joey Minta <jminta@gmail.com>
 *   Simon Paquet <bugzilla@babylonsounds.com>
 *   Stefan Sitter <ssitter@googlemail.com>
 *   Thomas Benisch <thomas.benisch@sun.com>
 *   Michael Buettner <michael.buettner@sun.com>
 *   Philipp Kewisch <mozilla@kewis.ch>
 *   Berend Cornelius <berend.cornelius@sun.com>
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

var gLastShownCalendarView = null;

function yesterday()
{
    var d = now();
    d.day--;
    return d;
}

function nextMonth(dt)
{
    var d = new Date(dt);
    d.setDate(1); // make sure we avoid "June 31" when we bump the month

    var mo = d.getMonth();
    if (mo == 11) {
        d.setMonth(0);
        d.setYear(d.getYear() + 1);
    } else {
        d.setMonth(mo + 1);
    }

    return d;
}

var gMiniMonthLoading = false;
function ltnMinimonthPick(minimonth) {
    if (gMiniMonthLoading || gCurrentMode != "calendar") {
        return;
    }
    if (document.getElementById("displayDeck").selectedPanel !=
        document.getElementById("calendar-view-box")) {
        ltnShowCalendarView(gLastShownCalendarView);
    }
    var jsDate = minimonth.value;
    document.getElementById("ltnDateTextPicker").value = jsDate;
    var cdt = jsDateToDateTime(jsDate, currentView().timezone);
    cdt.isDate = true;
    currentView().goToDay(cdt);
}

function ltnOnLoad(event) {
    // take the existing folderPaneBox (that's what thunderbird displays
    // at the left side of the application window) and stuff that inside
    // of the deck we're introducing with the contentPanel. this essentially
    // rearranges the DOM tree and allows us to switch between content that
    // lives inside of the left pane.
    var folderPaneBox = document.getElementById("folderPaneBox");
    var contentPanel = document.getElementById("contentPanel");
    contentPanel.insertBefore(folderPaneBox, contentPanel.firstChild);

    // we're taking care of the mode toolbar (that's the small toolbar on
    // the lower left with the 'mail', 'calendar', 'task' buttons on it).
    // since we want to have this particular toolbar displayed at the
    // top or bottom inside the folderPaneBox (basically on top or bottom
    // of the window) we need to go to great length in order to get this
    // damned thing working. i decided to dynamically place the toolbox
    // inside the DOM tree, that appears to be the most clean solution to
    // the problem. unfortunately, it didn't work out that easy. as soon
    // as we call insertBefore() to place the node somewhere different, the
    // constructor of the appropriate binding gets called again. this has
    // the nasty side-effect that the toolbar get a bit confused, since it
    // thinks it is customized, which just isn't the case. that's why we need
    // to carry some internal toolbar properties over. and that's what those
    // functions retrieveToolbarProperties() and restoreToolbarProperties()
    // are all about.
    var retrieveToolbarProperties = function(toolbox)
    {
      var toolbars = {};
      var toolbar = toolbox.firstChild;
      while (toolbar) {
        if (toolbar.localName == "toolbar") {
          if (toolbar.getAttribute("customizable") == "true") {
            if (!toolbar.hasAttribute("customindex")) {
              var propertybag = {};
              propertybag.firstPermanentChild = toolbar.firstPermanentChild;
              propertybag.lastPermanentChild = toolbar.lastPermanentChild;
              toolbars[toolbar.id] = propertybag;
            }
          }
        }
        toolbar = toolbar.nextSibling;
      }
      return toolbars;
    }

    var restoreToolbarProperties = function(toolbox,toolbars)
    {
      var toolbar = toolbox.firstChild;
      while (toolbar) {
        if (toolbar.localName == "toolbar") {
          if (toolbar.getAttribute("customizable") == "true") {
            if (!toolbar.hasAttribute("customindex")) {
              var propertybag = toolbars[toolbar.id];
              toolbar.firstPermanentChild = propertybag.firstPermanentChild;
              toolbar.lastPermanentChild = propertybag.lastPermanentChild;
            }
          }
        }
        toolbar = toolbar.nextSibling;
      }
    }

    // DOMAttrModified handler that listens on the toolbox element
    var onModified = function(aEvent) {
      if(aEvent.attrName == "location") {
        var modeToolbox = document.getElementById("mode-toolbox");
        modeToolbox.removeEventListener("DOMAttrModified", onModified, false);
        var onLocationHandler = function() {
          var contentPanel = document.getElementById("contentPanel");
          var palette = modeToolbox.palette;
          var bag = retrieveToolbarProperties(modeToolbox);
          if(aEvent.newValue == "top" && !aEvent.prevValue || aEvent.prevValue == "bottom") {
            // place the mode toolbox at the top of the left pane
            modeToolbox = contentPanel.parentNode.insertBefore(modeToolbox, contentPanel);
          } else if(aEvent.newValue == "bottom" && aEvent.prevValue == "top") {
            // place the mode toolbox at the bottom of the left pane
            modeToolbox = contentPanel.parentNode.appendChild(modeToolbox);
          }
          restoreToolbarProperties(modeToolbox,bag);
          modeToolbox.addEventListener("DOMAttrModified", onModified, false);
          modeToolbox.palette = palette;
        }

        setTimeout(onLocationHandler,1);
      }
    }

    // install the handler that listens for modified 'location' attribute
    // on the toolbox. the value is changed by the toolbar customize dialog.
    var modeToolbox = document.getElementById("mode-toolbox");
    if(modeToolbox.getAttribute("location") != "bottom") {
      var palette = modeToolbox.palette;
      var bag = retrieveToolbarProperties(modeToolbox);
      modeToolbox = contentPanel.parentNode.insertBefore(modeToolbox, contentPanel);
      modeToolbox.palette = palette;
      restoreToolbarProperties(modeToolbox,bag);
    }
    modeToolbox.addEventListener("DOMAttrModified", onModified, false);

    // To make sure the folder pane doesn't disappear without any possibility
    // to get it back, we need to reveal it when the mode box is collapsed.
    function modeBoxAttrModified(event) {
        if (event.attrName == "collapsed") {
            document.getElementById("folderPaneBox")
                    .removeAttribute("collapsed");
        }
    }

    document.getElementById("ltnModeBox").addEventListener("DOMAttrModified",
                                                           modeBoxAttrModified,
                                                           true);

    // Take care of common initialization
    commonInitCalendar();

    // nuke the onload, or we get called every time there's
    // any load that occurs
    document.removeEventListener("load", ltnOnLoad, true);

    // Hide the calendar view so it doesn't push the status-bar offscreen
    collapseElement(document.getElementById("calendar-view-box"));

    // Add an unload function to the window so we don't leak any listeners
    window.addEventListener("unload", ltnFinish, false);

    // Set up invitations manager
    scheduleInvitationsUpdate(FIRST_DELAY_STARTUP);
    getCalendarManager().addObserver(gInvitationsCalendarManagerObserver);

    var filter = document.getElementById("task-tree-filtergroup");
    filter.value = filter.value || "all";
    document.getElementById("modeBroadcaster").setAttribute("mode", gCurrentMode);
    ltnInitTodayPane();
}

/* Called at midnight to tell us to redraw date-specific widgets.  Do NOT call
 * this for normal refresh, since it also calls scheduleMidnightRefresh.
 */
function refreshUIBits() {
    try {
        getMinimonth().refreshDisplay();

        // refresh the current view, if it has ever been shown
        var cView = currentView();
        if (cView.initialized) {
            cView.goToDay(cView.selectedDay);
        }

        if (!TodayPane.showsToday()) {
            TodayPane.setDay(now());
        }

        // update the unifinder
        refreshEventTree();
    } catch (exc) {
        ASSERT(false, exc);
    }

    // schedule our next update...
    scheduleMidnightUpdate(refreshUIBits);
}

/**
 * Select the calendar view in the background, not switching to calendar mode if
 * in mail mode.
 */
function ltnSelectCalendarView(type) {
    gLastShownCalendarView = type;

    // Sunbird/Lightning Common view switching code
    switchToView(type);

}

function toggleControlDisplay(aCommandId, aControlId) {
    var control = document.getElementById(aControlId);
    var command = document.getElementById(aCommandId);
    if (control.getAttribute("collapsedinMode") == "false") {
        if (control.hasAttribute("collapsed")) {
            control.removeAttribute("collapsed");
            command.setAttribute("checked", "true");
            return;
        }
    }
    command.setAttribute("checked", "false");
}

function toggleControlinMode(aCommandId, aControlId) {
    var control = document.getElementById(aControlId);
    var command = document.getElementById(aCommandId);
    if (control.hasAttribute("collapsed")) {
        control.removeAttribute("collapsed");
        control.setAttribute("collapsedinMode", "false");
        command.setAttribute("checked","true");
    }
    else {
        control.setAttribute("collapsed", "true");
        control.setAttribute("collapsedinMode", "true");
        command.setAttribute("checked", "false");
    }
}

function toggleToolbar(aCommandId, aToolbarId) {
    var toolBar = document.getElementById(aToolbarId);
    var command = document.getElementById(aCommandId);
    if (toolBar.hasAttribute("collapsed")) {
       toolBar.removeAttribute("collapsed");
       command.setAttribute("checked", "true");
    }
    else {
       toolBar.setAttribute("collapsed", "true");
       command.setAttribute("checked", "false");
    }
 }



/**
 * Show the calendar view, also switching to calendar mode if in mail mode
 */
function ltnShowCalendarView(type)
{
    gLastShownCalendarView = type;

    if (gCurrentMode != 'calendar') {
        // This function in turn calls showCalendarView(), so return afterwards.
        ltnSwitch2Calendar();
        return;
    }

    ltnSelectCalendarView(type);
}


/**
 * This function has the sole responsibility to switch back to
 * mail mode (by calling ltnSwitch2Mail()) if we are getting
 * notifications from other panels (besides the calendar views)
 * but find out that we're not in mail mode. This situation can
 * for example happen if we're in calendar mode but the 'new mail'
 * slider gets clicked and wants to display the appropriate mail.
 * All necessary logic for switching between the different modes
 * should live inside of the corresponding functions:
 * - ltnSwitch2Mail()
 * - ltnSwitch2Calendar()
 * - ltnSwitch2Task()
 */
function LtnObserveDisplayDeckChange(event) {
    var deck = event.target;

    // Bug 309505: The 'select' event also fires when we change the selected
    // panel of calendar-view-box.  Workaround with this check.
    if (deck.id != "displayDeck") {
        return;
    }

    var id = null;
    try { id = deck.selectedPanel.id } catch (e) { }

    // Switch back to mail mode in case we find that this
    // notification has been fired but we're still in calendar or task mode.
    // Specifically, switch back if we're *not* in mail mode but the notification
    // did *not* come from either the "calendar-view-box" or the "calendar-task-box".
    if (gCurrentMode != 'mail') {
        if (id != "calendar-view-box" && id != "calendar-task-box") {
            ltnSwitch2Mail();
        }
    }
}

function ltnFinish() {
    getCalendarManager().removeObserver(gInvitationsCalendarManagerObserver);

    // Common finish steps
    commonFinishCalendar();
}

// After 1.5 was released, the search box was moved into an optional toolbar
// item, with a different ID.  This function keeps us compatible with both.
function findMailSearchBox() {
    var tb15Box = document.getElementById("searchBox");
    if (tb15Box) {
        return tb15Box;
    }

    var tb2Box = document.getElementById("searchInput");
    if (tb2Box) {
        return tb2Box;
    }

    // In later versions, it's possible that a user removed the search box from
    // the toolbar.
    return null;
}

var gSelectFolder = SelectFolder;
var gSelectMessage = SelectMessage;

SelectFolder = function(folderUri) {
    document.getElementById("switch2mail").doCommand();
    gSelectFolder(folderUri);
}

var calendarpopuplist = new Array();
var taskpopuplist = new Array();
var mailpopuplist = new Array();
var menulist = new Array();
#ifdef XP_MACOSX
    var quitMenu = null;
    var prefMenu = null;
#endif

function ltnInitializeMenus(){
#ifdef XP_MACOSX
    // The following Mac specific code-lines became necessary due to bug 409845
    prefMenu = document.getElementById("menu_preferences");
    prefMenu.setAttribute("mode", "system");
    quitMenu = document.getElementById("menu_FileQuitItem");
    quitMenu.setAttribute("mode", "system");
#endif
    copyPopupMenus();
    ltnRemoveMailOnlyItems(calendarpopuplist, "calendar");
    ltnRemoveMailOnlyItems(taskpopuplist, "task");
    var modeToolbar = document.getElementById("mode-toolbar");
    var visible = !modeToolbar.hasAttribute("collapsed");
    document.getElementById("modeBroadcaster").setAttribute("checked", visible);
    document.getElementById("calendar-toolbar").setAttribute("collapsed", gCurrentMode!="calendar");
    document.getElementById("task-toolbar").setAttribute("collapsed", gCurrentMode!="task");
}

function getMenuElementById(aElementId, aMenuPopup) {
        var element = null;
        var elements = aMenuPopup.getElementsByAttribute("id", aElementId);
        if (elements.length > 0) {
            element = elements[0];
        }
        return element;
    }

/**  removes all succeedingmenu elements of a container up to the next
*    menuseparator that thus denotes the end of the section. Elements with the
*    attribute mode == 'calendar' are ignored
*/
function removeMenuElementsInSection(aElement, aExcludeMode) {
    var element = aElement;
    if (element) {
        var bleaveloop = false;
        while (!bleaveloop) {
            var ignore = false;
            bleaveloop = element.localName == "menuseparator";
            if (bleaveloop) {
                // we delete the menuseparator only if it's the last element
                // within its container
                bleaveloop = (element.nextSibling != null);
            }
            if (element.hasAttribute("mode")) {
            ignore = element.getAttribute("mode") == aExcludeMode ||
                     element.getAttribute("mode") == "calendar,task";
            }
            var nextMenuElement = element.nextSibling;
            if (!ignore) {
                try {
                    element.parentNode.removeChild(element);
                } catch (e) {
                    dump("Element '" + element.getAttribute("id") + "' could not be removed\n");
                }
            }
            if (!bleaveloop) {
                element = nextMenuElement;
                bleaveloop = (element == null);
            }
        }
    }
}

function removeElements(aElementList) {
    aElementList.forEach(function(element) {
        try {
            if (element) {
                element.parentNode.removeChild(element);
            }
        } catch (e) {
            dump("Element '" + element.getAttribute("id") + "' could not be removed\n");
        }
    });
}

function addToPopupList(aMenuElement, aNewPopupMenu, aPopupList, aExcludedModes, aClone, aRemovePopupShowing) {
    var child = aMenuElement.firstChild;
    if (child) {
        if (child.localName == "menupopup") {
            if (aNewPopupMenu) {
                var newPopupMenu = aNewPopupMenu;
            } else {
                var newPopupMenu = child;
            }
            if (aClone) {
                newPopupMenu = newPopupMenu.cloneNode(true);
                if (aRemovePopupShowing) {
                    newPopupMenu.removeAttribute("onpopupshowing");
                }
            }
            removeMenuElements(newPopupMenu, aExcludedModes);
            aPopupList.push(newPopupMenu);
        }
    }
}

function copyPopupMenus() {
    // define menuList...
    menulist.push(document.getElementById("menu_File"));
    menulist.push(document.getElementById("menu_Edit"));
    var menuView = document.getElementById("menu_View");
    menulist.push(menuView);
    menulist.push(menuView.nextSibling); // id-less menu_Go
    menulist.push(document.getElementById("messageMenu"));
    menulist.push(document.getElementById("tasksMenu"));

    // define PopupMenus for calendar mode...
    var excludeList = new Array("mail", "task", "system");
    addToPopupList(menulist[0], null, calendarpopuplist, excludeList, true, true);
    addToPopupList(menulist[1], null, calendarpopuplist, excludeList, true, false);
    addToPopupList(menulist[2], null, calendarpopuplist, excludeList, true, true);
    addToPopupList(menulist[3], document.getElementById("calendar-GoPopupMenu"), calendarpopuplist, excludeList, true, false);
    addToPopupList(menulist[4], document.getElementById("calendarCalendarPopupMenu"), calendarpopuplist, excludeList, true, false);
    addToPopupList(menulist[5], null, calendarpopuplist, excludeList, true, false);

    // define PopupMenus for task mode...
    var excludeList = new Array("mail", "calendar", "system");
    addToPopupList(menulist[0], null, taskpopuplist, excludeList, true, true);
    addToPopupList(menulist[1], null, taskpopuplist, excludeList, true, false);
    addToPopupList(menulist[2], null, taskpopuplist, excludeList, true, true);
    addToPopupList(menulist[3], document.getElementById("calendar-GoPopupMenu"), taskpopuplist, excludeList, true, false);
    var tasksViewMenuPopup = clonePopupMenu("taskitem-context-menu", "taskitem-menu", "menu-");
    tasksViewMenuPopup.removeChild(getMenuElementById("menu-" + "task-context-menu-modify", tasksViewMenuPopup));
    tasksViewMenuPopup.removeChild(getMenuElementById("menu-" + "task-context-menu-delete", tasksViewMenuPopup));
    addToPopupList(menulist[4], tasksViewMenuPopup, taskpopuplist, excludeList, false, false);
    addToPopupList(menulist[5], null, taskpopuplist, excludeList, true, true);

    // define PopupMenus for mail mode...
    var excludeList = new Array("calendar", "task", "calendar,task");
    addToPopupList(menulist[0], null, mailpopuplist, excludeList, false, false);
    addToPopupList(menulist[1], null, mailpopuplist, excludeList, false, false);
    addToPopupList(menulist[2], null, mailpopuplist, excludeList, false, false);
    // copy calendar-GoPopupMenu into Thunderbird's GoPopupMenu to switch modes
    var tbGoPopupMenu = menulist[3].lastChild;
    var calGoPopupMenu = document.getElementById("calendar-GoPopupMenu").cloneNode(true);
    var calGoItem;
    while ((calGoItem = calGoPopupMenu.firstChild)) {
        tbGoPopupMenu.appendChild(calGoPopupMenu.removeChild(calGoItem));
    }
    addToPopupList(menulist[3], null, mailpopuplist, excludeList, false, false);
    addToPopupList(menulist[4], null, mailpopuplist, excludeList, false, false);
    addToPopupList(menulist[5], null, mailpopuplist, excludeList, false, false);
}

function removeNeedlessSeparators(aMenuPopupList) {
    aMenuPopupList.forEach(function(aMenuPopup) {
        var child = aMenuPopup.firstChild;
        if (child) {
            if (child.localName == "menuseparator") {
                try {
                    aMenuPopup.removeChild(child)
                } catch (e) {
                    dump("Element '" + child.getAttribute("id") + "' could not be removed\n");
                }
            }
        }
        child = aMenuPopup.lastChild;
        if (child) {
            if (child.localName == "menuseparator") {
                try {
                    aMenuPopup.removeChild(child)
                } catch (e) {
                    dump("Element '" + child.getAttribute("id") + "' could not be removed\n");
                }
            }
        }
    });
}

function ltnRemoveMailOnlyItems(aMenuPopupList, aExcludeMode) {
    removeElements(
// "File" - menu
    [getMenuElementById("openMessageFileMenuitem", aMenuPopupList[0]),
     getMenuElementById("newAccountMenuItem", aMenuPopupList[0]),
     getMenuElementById("fileAttachmentMenu", aMenuPopupList[0]),
     getAdjacentSibling(getMenuElementById("menu_saveAs", aMenuPopupList[0]), 2),

// "Edit" - menu
     getMenuElementById("menu_find", aMenuPopupList[1]),
     getMenuElementById("menu_favoriteFolder", aMenuPopupList[1]),
     getMenuElementById("menu_properties", aMenuPopupList[1]),
     getMenuElementById("menu_accountmgr", aMenuPopupList[1]),

// "View"-menu
     getMenuElementById("menu_showMessengerToolbar", aMenuPopupList[2]),

// "Tools"-menu
     getMenuElementById("tasksMenuMail", aMenuPopupList[5]),
     getMenuElementById("menu_import", aMenuPopupList[5])]);

     removeNeedlessSeparators(aMenuPopupList);

// "File" - menu
    [getMenuElementById("menu_newFolder", aMenuPopupList[0]),
     getMenuElementById("menu_saveAs", aMenuPopupList[0]),
     getMenuElementById("menu_getnextnmsg", aMenuPopupList[0]),
     getMenuElementById("menu_renameFolder", aMenuPopupList[0]),
//     getMenuElementById("offlineMenuItem", aMenuPopupList[0]),

// "Edit" - menu
     getMenuElementById("menu_delete", aMenuPopupList[1]),
     getMenuElementById("menu_select", aMenuPopupList[1]),

// "View"-menu
     getMenuElementById("menu_MessagePaneLayout", aMenuPopupList[2]),
     getMenuElementById("viewSortMenu", aMenuPopupList[2]),
     getMenuElementById("viewheadersmenu", aMenuPopupList[2]),
     getMenuElementById("viewTextSizeMenu", aMenuPopupList[2]),
     getMenuElementById("pageSourceMenuItem", aMenuPopupList[2]),

// "Tools"-menu
     getMenuElementById("filtersCmd", aMenuPopupList[5]),
     getMenuElementById("runJunkControls", aMenuPopupList[5])].forEach(function(element){
        removeMenuElementsInSection(element, aExcludeMode);
    });
}

function swapPopupMenus() {
    var showStatusbar = document.getElementById("menu_showTaskbar").getAttribute("checked");
    var newmenupopuplist = null;
    if (gCurrentMode == "mail") {
        newmenupopuplist = mailpopuplist;
    } else if (gCurrentMode == "calendar") {
        newmenupopuplist = calendarpopuplist;
    } else if (gCurrentMode == "task") {
        newmenupopuplist = taskpopuplist;
    }
    for (var i = 0; i < menulist.length; i++) {
        var menu = menulist[i];
        var oldmenupopup = menu.firstChild;
        if (oldmenupopup) {
            menu.replaceChild(newmenupopuplist[i], oldmenupopup);
        }
    }
#ifdef XP_MACOSX
    document.getElementById("menu_File").firstChild.appendChild(quitMenu);
    document.getElementById("tasksMenu").firstChild.appendChild(prefMenu);
#endif
    document.getElementById("menu_showTaskbar").setAttribute("checked", showStatusbar);
    var messageMenu = document.getElementById("messageMenu");
    if (gCurrentMode == "mail") {
        messageMenu.setAttribute("label", messagemenulabel);
        messageMenu.setAttribute("accesskey", messagemenuaccesskey);
    } else if (gCurrentMode == "calendar"){
        messageMenu.setAttribute("label", calendarmenulabel);
        messageMenu.setAttribute("accesskey", calendarmenuaccesskey);
    } else if (gCurrentMode == "task"){
        messageMenu.setAttribute("label", tasksmenulabel);
        messageMenu.setAttribute("accesskey", tasksmenuaccesskey);
    }
}

function removeMenuElements(aRoot, aModeValue) {
    for (var n = 0; n < aModeValue.length; n++) {
        var modeElements = aRoot.getElementsByAttribute("mode", aModeValue[n]);
        if (modeElements.length > 0) {
            for (var i = modeElements.length-1; i >=0; i--) {
                var element = modeElements[i];
                if (element) {
                    var localName = element.localName;
                    if (localName =="menuitem" || localName == "menuseparator" || localName == "menu"){
                        element.parentNode.removeChild(element);
                    }
                }
            }
        }
    }
}

// == invitations link
const FIRST_DELAY_STARTUP = 100;
const FIRST_DELAY_RESCHEDULE = 100;
const FIRST_DELAY_REGISTER = 10000;
const FIRST_DELAY_UNREGISTER = 0;

var gInvitationsOperationListener = {
    mCount: 0,

    onOperationComplete: function sBOL_onOperationComplete(aCalendar,
                                                           aStatus,
                                                           aOperationType,
                                                           aId,
                                                           aDetail) {
        if (Components.isSuccessCode(aStatus)) {
            var invitationsBox = document.getElementById("invitations");
            var value = invitationsLabel + " (" + this.mCount + ")"; // XXX l10n-unfriendly
            invitationsBox.setAttribute("value", value);
            invitationsBox.removeAttribute("hidden");
        } else {
            var invitationsBox = document.getElementById("invitations");
            invitationsBox.setAttribute("hidden", "true");
        }
        this.mCount = 0;
    },

    onGetResult: function sBOL_onGetResult(aCalendar,
                                           aStatus,
                                           aItemType,
                                           aDetail,
                                           aCount,
                                           aItems) {
        if (Components.isSuccessCode(aStatus)) {
            this.mCount += aCount;
        }
    }
};

var gInvitationsCalendarManagerObserver = {
    mSideBar: this,

    onCalendarRegistered: function cMO_onCalendarRegistered(aCalendar) {
        this.mSideBar.rescheduleInvitationsUpdate(FIRST_DELAY_REGISTER);
    },

    onCalendarUnregistering: function cMO_onCalendarUnregistering(aCalendar) {
        this.mSideBar.rescheduleInvitationsUpdate(FIRST_DELAY_UNREGISTER);
    },

    onCalendarDeleting: function cMO_onCalendarDeleting(aCalendar) {
    }
};

function scheduleInvitationsUpdate(firstDelay) {
    gInvitationsCalendarManagerObserver.mCount = 0;
    getInvitationsManager().scheduleInvitationsUpdate(firstDelay,
                                                      gInvitationsOperationListener);
}

function rescheduleInvitationsUpdate(firstDelay) {
    getInvitationsManager().cancelInvitationsUpdate();
    scheduleInvitationsUpdate(firstDelay);
}

function openInvitationsDialog() {
    getInvitationsManager().cancelInvitationsUpdate();
    gInvitationsCalendarManagerObserver.mCount = 0;
    getInvitationsManager().openInvitationsDialog(
        gInvitationsOperationListener,
        function oiD_callback() {
            scheduleInvitationsUpdate(FIRST_DELAY_RESCHEDULE);
        });
}

SelectMessage = function(messageUri) {
    document.getElementById("switch2mail").doCommand();
    gSelectMessage(messageUri);
}

document.getElementById("displayDeck").
    addEventListener("select", LtnObserveDisplayDeckChange, true);

document.addEventListener("load", ltnOnLoad, true);

/**
 * Sets up the message pane context menu. Even though the actual context menu
 * is in messenger-overlay-toolbar.xul, this needs to be in a file that
 * directly overlays messenger.xul or the functions will not be defined.
 */
function calSetupMsgPaneContext() {
    var hasSelection = (GetFirstSelectedMessage() != null);

    // Disable the convert menu altogether
    setElementValue("messagePaneContext-calendar-convert-menu",
                    !hasSelection && "true",
                    "hidden");

    return calSetupMsgPaneContext.tbSetupMsgPaneContext();
}
calSetupMsgPaneContext.tbSetupMsgPaneContext = fillMessagePaneContextMenu;
var fillMessagePaneContextMenu = calSetupMsgPaneContext;

/**
 * Sets up the thread pane context menu. Even though the actual context menu
 * is in messenger-overlay-toolbar.xul, this needs to be in a file that
 * directly overlays messenger.xul or the functions will not be defined.
 */
function calSetupThreadPaneContext() {
    var hasSelection = (GetFirstSelectedMessage() != null);

    // Disable the convert menu altogether
    setElementValue("threadPaneContext-calendar-convert-menu",
                    !hasSelection && "true",
                    "disabled");

    return calSetupThreadPaneContext.tbSetupThreadPaneContext();
}
calSetupThreadPaneContext.tbSetupThreadPaneContext = fillThreadPaneContextMenu;
var fillThreadPaneContextMenu = calSetupThreadPaneContext;
