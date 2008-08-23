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
 * The Original Code is Sun Microsystems code.
 *
 * The Initial Developer of the Original Code is Sun Microsystems.
 * Portions created by the Initial Developer are Copyright (C) 2006
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Thomas Benisch <thomas.benisch@sun.com>
 *   Philipp Kewisch <mozilla@kewis.ch>
 *   Daniel Boelzle <daniel.boelzle@sun.com>
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

var gInvitationsRequestManager = {
    mRequestStatusList: {},

    addRequestStatus: function IRM_addRequestStatus(calendar, op) {
        if (op) {
            this.mRequestStatusList[calendar.id] = op;
        }
    },

    cancelPendingRequests: function IRM_cancelPendingRequests() {
        for each (var request in this.mRequestStatusList) {
            if (request && request.isPending) {
                request.cancel(null);
            }
        }
        this.mRequestStatusList = {};
    }
};

var gInvitationsManager = null;

function getInvitationsManager() {
    if (!gInvitationsManager) {
        gInvitationsManager = new InvitationsManager();
    }
    return gInvitationsManager;
}

function InvitationsManager() {
    this.mItemList = new Array();
    this.mStartDate = null;
    this.mJobsPending = 0;
    this.mTimer = null;

    var self = this;
    window.addEventListener("unload", function() {
        // Unload handlers get removed automatically
        self.cancelInvitationsUpdate();
    }, false);
}

InvitationsManager.prototype = {
    mItemList: null,
    mStartDate: null,
    mJobsPending: 0,
    mTimer: null,

    scheduleInvitationsUpdate: function IM_scheduleInvitationsUpdate(firstDelay,
                                                                     operationListener) {
        this.cancelInvitationsUpdate();

        var self = this;
        this.mTimer = setTimeout(function startInvitationsTimer() {
            if (getPrefSafe("calendar.invitations.autorefresh.enabled", true)) {
                self.mTimer = setInterval(function repeatingInvitationsTimer() {
                    self.getInvitations(operationListener);
                    }, getPrefSafe("calendar.invitations.autorefresh.timeout", 3) * 60000);
            }
            self.getInvitations(operationListener);
        }, firstDelay);
    },

    cancelInvitationsUpdate: function IM_cancelInvitationsUpdate() {
        clearTimeout(this.mTimer);
    },

    getInvitations: function IM_getInvitations(operationListener1,
                                               operationListener2) {
        var listeners = [];
        if (operationListener1) {
            listeners.push(operationListener1);
        }
        if (operationListener2) {
            listeners.push(operationListener2);
        }

        gInvitationsRequestManager.cancelPendingRequests();
        this.updateStartDate();
        this.deleteAllItems();

        var cals = getCalendarManager().getCalendars({});

        var opListener = {
            mCount: cals.length,
            mRequestManager: gInvitationsRequestManager,
            mInvitationsManager: this,
            mHandledItems: {},

            // calIOperationListener
            onOperationComplete: function(aCalendar,
                                          aStatus,
                                          aOperationType,
                                          aId,
                                          aDetail) {
                if (--this.mCount == 0) {
                    this.mInvitationsManager.mItemList.sort(
                        function (a, b) {
                            return a.startDate.compare(b.startDate);
                        });
                    for each (var listener in listeners) {
                        try {
                            if (this.mInvitationsManager.mItemList.length) {
                                // Only call if there are actually items
                                listener.onGetResult(null,
                                                     Components.results.NS_OK,
                                                     Components.interfaces.calIItemBase,
                                                     null,
                                                     this.mInvitationsManager.mItemList.length,
                                                     this.mInvitationsManager.mItemList);
                            }
                            listener.onOperationComplete(null,
                                                         Components.results.NS_OK,
                                                         Components.interfaces.calIOperationListener.GET,
                                                         null,
                                                         null);
                        } catch (exc) {
                            ERROR(exc);
                        }
                    }
                }
            },

            onGetResult: function(aCalendar,
                                  aStatus,
                                  aItemType,
                                  aDetail,
                                  aCount,
                                  aItems) {
                if (Components.isSuccessCode(aStatus)) {
                    for each (var item in aItems) {
                        // we need to retrieve by occurrence to properly filter exceptions,
                        // should be fixed with bug 416975
                        item = item.parentItem;
                        var hid = item.hashId;
                        if (!this.mHandledItems[hid]) {
                            this.mHandledItems[hid] = true;
                            this.mInvitationsManager.addItem(item);
                        }
                    }
                }
            }
        };

        for each (var calendar in cals) {
            if (!isCalendarWritable(calendar)) {
                opListener.onOperationComplete();
                continue;
            }

            // temporary hack unless calCachedCalendar supports REQUEST_NEEDS_ACTION filter:
            calendar = calendar.getProperty("cache.uncachedCalendar");
            if (!calendar) {
                opListener.onOperationComplete();
                continue;
            }

            try {
                calendar = calendar.QueryInterface(Components.interfaces.calICalendar);
                var endDate = this.mStartDate.clone();
                endDate.year += 1;
                var op = calendar.getItems(Components.interfaces.calICalendar.ITEM_FILTER_REQUEST_NEEDS_ACTION |
                                           Components.interfaces.calICalendar.ITEM_FILTER_TYPE_ALL |
                                           // we need to retrieve by occurrence to properly filter exceptions,
                                           // should be fixed with bug 416975
                                           Components.interfaces.calICalendar.ITEM_FILTER_CLASS_OCCURRENCES,
                                           0, this.mStartDate,
                                           endDate /* we currently cannot pass null here, because of bug 416975 */,
                                           opListener);
                gInvitationsRequestManager.addRequestStatus(calendar, op);
            } catch (exc) {
                opListener.onOperationComplete();
                ERROR(exc);
            }
        }
    },

    openInvitationsDialog: function IM_openInvitationsDialog(onLoadOpListener,
                                                             finishedCallBack) {
        var args = new Object();
        args.onLoadOperationListener = onLoadOpListener;
        args.queue = new Array();
        args.finishedCallBack = finishedCallBack;
        args.requestManager = gInvitationsRequestManager;
        args.invitationsManager = this;
        // the dialog will reset this to auto when it is done loading
        window.setCursor("wait");
        // open the dialog modally
        window.openDialog(
            "chrome://calendar/content/calendar-invitations-dialog.xul",
            "_blank",
            "chrome,titlebar,modal,resizable",
            args);
    },

    processJobQueue: function IM_processJobQueue(queue,
                                                 jobQueueFinishedCallBack) {
        // TODO: undo/redo
        function operationListener(mgr, queueCallback, oldItem_) {
            this.mInvitationsManager = mgr;
            this.mJobQueueFinishedCallBack = queueCallback;
            this.mOldItem = oldItem_;
        }
        operationListener.prototype = {
            onOperationComplete: function (aCalendar,
                                           aStatus,
                                           aOperationType,
                                           aId,
                                           aDetail) {
                if (Components.isSuccessCode(aStatus) &&
                    aOperationType == Components.interfaces.calIOperationListener.MODIFY) {
                    checkAndSendItipMessage(aDetail, aOperationType, this.mOldItem);
                    this.mInvitationsManager.deleteItem(aDetail);
                    this.mInvitationsManager.addItem(aDetail);
                }
                this.mInvitationsManager.mJobsPending--;
                if (this.mInvitationsManager.mJobsPending == 0 &&
                    this.mJobQueueFinishedCallBack) {
                    this.mJobQueueFinishedCallBack();
                }
            },

            onGetResult: function(aCalendar,
                                  aStatus,
                                  aItemType,
                                  aDetail,
                                  aCount,
                                  aItems) {

            }
        };

        this.mJobsPending = 0;
        for (var i = 0; i < queue.length; i++) {
            var job = queue[i];
            var oldItem = job.oldItem;
            var newItem = job.newItem;
            switch (job.action) {
                case 'modify':
                    this.mJobsPending++;
                    newItem.calendar.modifyItem(newItem,
                                                oldItem,
                                                new operationListener(this, jobQueueFinishedCallBack, oldItem));
                    break;
                default:
                    break;
            }
        }
        if (this.mJobsPending == 0 && jobQueueFinishedCallBack) {
            jobQueueFinishedCallBack();
        }
    },

    hasItem: function IM_hasItem(item) {
        var hid = item.hashId;
        return this.mItemList.some(
            function someFunc(item_) {
                return hid == item_.hashId;
            });
    },

    addItem: function IM_addItem(item) {
        var recInfo = item.recurrenceInfo;
        if (recInfo && this.getParticipationStatus(item) != "NEEDS-ACTION") {
            // scan exceptions:
            var ids = recInfo.getExceptionIds({});
            for each (var id in ids) {
                var ex = recInfo.getExceptionFor(id, false);
                if (ex && this.validateItem(ex) && !this.hasItem(ex)) {
                    this.mItemList.push(ex);
                }
            }
        } else if (this.validateItem(item) && !this.hasItem(item)) {
            this.mItemList.push(item);
        }
    },

    deleteItem: function IM_deleteItem(item) {
        var id = item.id;
        this.mItemList.filter(
            function filterFunc(item_) {
                return id != item_.id;
            });
    },

    deleteAllItems: function IM_deleteAllItems() {
        this.mItemList = [];
    },

    getStartDate: function IM_getStartDate() {
        var date = now();
        date.second = 0;
        date.minute = 0;
        date.hour = 0;
        return date;
    },

    updateStartDate: function IM_updateStartDate() {
        if (!this.mStartDate) {
            this.mStartDate = this.getStartDate();
        } else {
            var startDate = this.getStartDate();
            if (startDate.compare(this.mStartDate) > 0) {
                this.mStartDate = startDate;
            }
        }
    },

    validateItem: function IM_validateItem(item) {
        if (item.calendar instanceof Components.interfaces.calISchedulingSupport &&
            !item.calendar.isInvitation(item)) {
            return false; // exclude if organizer has invited himself
        }
        var participationStatus = this.getParticipationStatus(item);
        var start = item[calGetStartDateProp(item)] || item[calGetEndDateProp(item)];
        return (participationStatus == "NEEDS-ACTION" &&
                start.compare(this.mStartDate) >= 0);
    },

    getParticipationStatus: function IM_getParticipationStatus(item) {
        var attendee;
        if (calInstanceOf(item.calendar, Components.interfaces.calISchedulingSupport)) {
            var attendee = item.calendar.getInvitedAttendee(item);
        }
        return (attendee ? attendee.participationStatus : null);
    }
};
