/**
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
 * The Original Code is the Calendar Preferences.
 *
 * The Initial Developer of the Original Code is
 *   Joey Minta <jminta@gmail.com>.
 * Portions created by the Initial Developer are Copyright (C) 2006
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
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
 * ***** END LICENSE BLOCK *****
 */

var gViewsPane = {
    init: function prefViewInit() {
        this.updateViewEndMenu(document.getElementById("daystarthour").value);
        this.updateViewStartMenu(document.getElementById("dayendhour").value);
        this.updateViewWorkDayCheckboxes(document.getElementById("weekstarts").value);
        this.initializeViewStartEndMenus();
    },

    initializeViewStartEndMenus: function() {
        var labelIdStart;
        var labelIdEnd;
        var timeFormatter = Components.classes["@mozilla.org/intl/scriptabledateformat;1"]
                                      .getService(Components.interfaces.nsIScriptableDateFormat);
        // 1 to 23 instead of 0 to 24 to keep midnight & noon as the localized strings
        for (var theHour = 1; theHour <= 23; theHour++) {
            var time = timeFormatter.FormatTime("", Components.interfaces.nsIScriptableDateFormat
                                    .timeFormatNoSeconds, theHour, 0, 0);

            labelIdStart = "timeStart" + theHour;
            labelIdEnd = "timeEnd" + theHour;
            // This if block to keep Noon as the localized string, instead of as a number.
            if (theHour != 12) {
                document.getElementById(labelIdStart).setAttribute("label", time);
                document.getElementById(labelIdEnd).setAttribute("label", time);
            }
        }
        // Deselect and reselect to update visible item title
        updateSelectedLabel("daystarthour");
        updateSelectedLabel("dayendhour");
    },

    updateViewEndMenu: function prefUpdateViewEnd(aStartValue) {
        var endMenuKids = document.getElementById("dayendhourpopup")
                                  .childNodes;
        for (var i = 0; i < endMenuKids.length; i++) {
            if (Number(endMenuKids[i].value) <= Number(aStartValue)) {
                endMenuKids[i].setAttribute("hidden", true);
            } else {
                endMenuKids[i].removeAttribute("hidden");
            }
        }
    },

    updateViewStartMenu: function prefUpdateViewStart(aEndValue) {
        var startMenuKids = document.getElementById("daystarthourpopup")
                                  .childNodes;
        for (var i = 0; i < startMenuKids.length; i++) {
            if (Number(startMenuKids[i].value) >= Number(aEndValue)) {
                startMenuKids[i].setAttribute("hidden", true);
            } else {
                startMenuKids[i].removeAttribute("hidden");
            }
        }
    },

    updateViewWorkDayCheckboxes: function prefUpdateViewWorkDays(weekStart) {
        weekStart = Number(weekStart);
        for (var i = weekStart; i < weekStart + 7; i++) {
            var checkbox = document.getElementById("dayoff" + (i % 7));
            checkbox.parentNode.appendChild(checkbox);
        }
    }
};
