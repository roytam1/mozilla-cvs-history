/* -*- Mode: javascript; tab-width: 20; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
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
 * The Original Code is Oracle Corporation code.
 *
 * The Initial Developer of the Original Code is
 *  Oracle Corporation
 * Portions created by the Initial Developer are Copyright (C) 2004
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Vladimir Vukicevic <vladimir.vukicevic@oracle.com>
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

//
// calCompositeCalendar.js
//

const calIOperationListener = Components.interfaces.calIOperationListener;

function calCompositeCalendarObserverHelper (compCalendar) {
    this.compCalendar = compCalendar;
}

calCompositeCalendarObserverHelper.prototype = {
    QueryInterface: function (aIID) {
        if (!aIID.equals(Components.interfaces.calIObserver) &&
            !aIID.equals(Components.interfaces.nsISupports))
        {
            throw Components.results.NS_ERROR_NO_INTERFACE;
        }

        return this;
    },

    onStartBatch: function() {
        for each (obs in this.compCalendar.mObservers)
            obs.onStartBatch();
    },

    onEndBatch: function() {
        for each (obs in this.compCalendar.mObservers)
            obs.onEndBatch();
    },

    onLoad: function() {
        for each (obs in this.compCalendar.mObservers)
            obs.onLoad();
    },

    onAddItem: function(aItem) {
        for each (obs in this.compCalendar.mObservers)
            obs.onAddItem(aItem);
    },

    onModifyItem: function(aNewItem, aOldItem) {
        for each (obs in this.compCalendar.mObservers)
            obs.onModifyItem(aNewItem, aOldItem);
    },

    onDeleteItem: function(aDeletedItem) {
        for each (obs in this.compCalendar.mObservers)
            obs.onDeleteItem(aDeletedItem);
    },

    onAlarm: function(aAlarmItem) {
        for each (obs in this.compCalendar.mObservers)
            obs.onAlarm(aAlarmItem);
    },

    onError: function(aErrNo, aMessage) {
        for each (obs in this.compCalendar.mObservers)
            obs.onError(aErrNo, aMessage);
    }
};

function calCompositeCalendar () {
    this.mObserverHelper = new calCompositeCalendarObserverHelper(this);
    this.wrappedJSObject = this;
}

calCompositeCalendar.prototype = {
    //
    // private members
    //
    mCalendars: Array(),
    mDefaultCalendar: null,

    //
    // nsISupports interface
    //
    QueryInterface: function (aIID) {
        if (!aIID.equals(Components.interfaces.nsISupports) &&
            !aIID.equals(Components.interfaces.calICalendar) &&
            !aIID.equals(Components.interfaces.calICompositeCalendar))
        {
            throw Components.results.NS_ERROR_NO_INTERFACE;
        }

        return this;
    },

    //
    // calICompositeCalendar interface
    //

    mCalendars: Array(),
    mDefaultCalendar: null,

    addCalendar: function (aCalendar) {
        // check if the calendar already exists
        for each (cal in this.mCalendars) {
            if (aCalendar.uri.equals(cal.uri)) {
                // throw exception if calendar already exists?
                return;
            }
        }

        // add our observer helper
        aCalendar.addObserver(this.mObserverHelper);

        this.mCalendars.push(aCalendar);

        this.observeCalendarAdded(aCalendar);

        // if we have no default calendar, we need one here
        if (this.mDefaultCalendar == null) {
            this.mDefaultCalendar = aCalendar;
            this.observeDefaultCalendarChanged(aCalendar);
        }
    },

    removeCalendar: function (aServer) {
        var newCalendars = Array();
        var calToRemove = null;
        for each (cal in this.mCalendars) {
            if (!aServer.equals(cal.uri))
                newCalendars.push(cal);
            else
                calToRemove = cal;
        }

        if (calToRemove) {
            this.mCalendars = newCalendars;
            calToRemove.removeObserver(this.mObserverHelper);
            this.observeCalendarRemoved(calToRemove);
        }
    },

    getCalendar: function (aServer) {
        for each (cal in this.mCalendars) {
            if (aServer.equals(cal.uri))
                return cal;
        }

        return null;
    },

    get calendars() {
        // return a nsISimpleEnumerator of this array.  This sucks.
        return null;
    },

    get defaultCalendar() { 
        return this.mDefaultCalendar;
    },

    set defaultCalendar(v) {
        if (this.mDefaultCalendar != v) {
            this.mDefaultCalendar = v;
            this.observeDefaultCalendarChanged (v);
        }
    },

    //
    // calICalendar interface
    //
    // Write operations here are forwarded to either the item's
    // parent calendar, or to the default calendar if one is set.
    // Get operations are sent to each calendar.
    //

    // this could, at some point, return some kind of URI identifying
    // all the child calendars, thus letting us create nifty calendar
    // trees.
    get uri() {
        throw Components.results.NS_ERROR_NOT_IMPLEMENTED;
    },
    set uri(v) {
        throw Components.results.NS_ERROR_NOT_IMPLEMENTED;
    },

    // void addObserver( in calIObserver observer );
    mCompositeObservers: Array(),
    mObservers: Array(),
    addObserver: function (aObserver) {
        const calICompositeObserver = Components.interfaces.calICompositeObserver;
        if (aObserver instanceof calICompositeObserver) {
            if (this.mCompositeObservers.indexOf(aObserver) == -1) {
                var compobs = aObserver.QueryInterface (calICompositeObserver);
                this.mCompositeObservers.push(compobs);
            }
        }

        if (this.mObservers.indexOf(aObserver) == -1)
            this.mObservers.push(aObserver);
    },

    // void removeObserver( in calIObserver observer );
    removeObserver: function (aObserver) {
        const calICompositeObserver = Components.interfaces.calICompositeObserver;
        if (aObserver instanceof calICompositeObserver)
            this.mCompositeObservers = this.mCompositeObservers.filter( function (v) { return v != aObserver; } );

        this.mObservers = this.mObservers.filter( function (v) { return v != aObserver; } );
    },

    // void modifyItem( in calIItemBase aItem, in calIOperationListener aListener );
    modifyItem: function (aItem, aListener) {
        if (aItem.parent == null) {
            // XXX Can't modify item with NULL parent
            throw Components.results.NS_ERROR_FAILURE;
        }

        aItem.parent.modifyItem (aItem, aListener);
    },

    // void deleteItem( in string id, in calIOperationListener aListener );
    deleteItem: function (aItem, aListener) {
        if (aItem.parent == null) {
            // XXX Can't delete item with NULL parent
            throw Components.results.NS_ERROR_FAILURE;
        }

        aItem.parent.deleteItem (aItem, aListener);
    },

    // void addItem( in calIItemBase aItem, in calIOperationListener aListener );
    addItem: function (aItem, aListener) {
        this.mDefaultCalendar.addItem (aItem, aListener);
    },

    // void getItem( in string aId, in calIOperationListener aListener );
    getItem: function (aId, aListener) {
        var cmpListener = new calCompositeGetListenerHelper(this.mCalendars.length, aListener);
        for each (cal in this.mCalendars) {
            cal.getItem (aId, cmpListener);
        }
    },

    // void getItems( in unsigned long aItemFilter, in unsigned long aCount, 
    //                in calIDateTime aRangeStart, in calIDateTime aRangeEnd,
    //                in calIOperationListener aListener );
    getItems: function (aItemFilter, aCount, aRangeStart, aRangeEnd, aListener) {
        var cmpListener = new calCompositeGetListenerHelper(this.mCalendars.length, aListener, aCount);
        for (cal in this.mCalendars) {
            this.mCalendars[cal].getItems (aItemFilter, aCount, aRangeStart, aRangeEnd, cmpListener);
        }
    },

    //
    // observer helpers
    //
    observeCalendarAdded: function (aCalendar) {
        for each (obs in this.mCompositeObservers)
            obs.onCalendarAdded (aCalendar);
    },

    observeCalendarRemoved: function (aCalendar) {
        for each (obs in this.mCompositeObservers)
            obs.onCalendarRemoved (aCalendar);
    },

    observeDefaultCalendarChanged: function (aCalendar) {
        for each (obs in this.mCompositeObservers)
            obs.onDefaultCalendarChanged (aCalendar);
    }
};

// composite listener helper
function calCompositeGetListenerHelper(aNumQueries, aRealListener, aMaxItems) {
    this.wrappedJSObject = this;
    this.mNumQueries = aNumQueries;
    this.mRealListener = aRealListener;
    this.mMaxItems = aMaxItems;
}

calCompositeGetListenerHelper.prototype = {
    mNumQueries: 0,
    mRealListener: null,
    mReceivedCompletes: 0,
    mFinished: false,
    mMaxItems: 0,
    mItemsReceived: 0,

    QueryInterface: function (aIID) {
        if (!aIID.equals(Components.interfaces.nsISupports) &&
            !aIID.equals(Components.interfaces.calIOperationListener))
        {
            throw Components.results.NS_ERROR_NO_INTERFACE;
        }

        return this;
    },

    onOperationComplete: function (aCalendar, aStatus, aOperationType, aId, aDetail) {
        if (this.mFinished) {
            dump ("+++ calCompositeGetListenerHelper.onOperationComplete: called with mFinished == true!");
            return;
        }

        if (!Components.isSuccessCode(aStatus)) {
            // proxy this to a onGetResult
            // XXX - do we want to give the real calendar? or this?
            this.mRealListener.onGetResult (aCalendar, aStatus, null, aDetail, 0, []);
        }

        this.mReceivedCompletes++;

        if (this.mReceivedCompletes == this.mNumQueries) {
            // we're done here.
            this.mRealListener.onOperationComplete (this,
                                                    aOperationType,
                                                    calIOperationListener.GET,
                                                    null,
                                                    null);
            this.mFinished = true;
        }
    },

    onGetResult: function (aCalendar, aStatus, aItemType, aDetail, aCount, aItems) {
        if (this.mFinished) {
            dump ("+++ calCompositeGetListenerHelper.onGetResult: called with mFinished == true!");
            return;
        }

        // ignore if we have a max and we're past it
        if (this.mMaxItems && this.mItemsReceived >= this.mMaxItems)
            return;

        if (Components.isSuccessCode(aStatus) &&
            this.mMaxItems &&
            ((this.mItemsReceived + aCount) > this.mMaxItems))
        {
            // this will blow past the limit
            aCount = this.mMaxItems - this.mItemsReceived;
            aItems = aItems.slice(0, numToSend);
        }

        // send GetResults to the real listener
        this.mRealListener.onGetResult (aCalendar, aStatus, aItemType, aDetail, aCount, aItems);
        this.mItemsReceived += aCount;
    }

};



/****
 **** module registration
 ****/

var calCompositeCalendarModule = {
    mCID: Components.ID("{aeff788d-63b0-4996-91fb-40a7654c6224}"),
    mContractID: "@mozilla.org/calendar/calendar;1?type=composite",
    
    registerSelf: function (compMgr, fileSpec, location, type) {
        compMgr = compMgr.QueryInterface(Components.interfaces.nsIComponentRegistrar);
        compMgr.registerFactoryLocation(this.mCID,
                                        "Calendar composite provider",
                                        this.mContractID,
                                        fileSpec,
                                        location,
                                        type);
    },

    getClassObject: function (compMgr, cid, iid) {
        if (!cid.equals(this.mCID))
            throw Components.results.NS_ERROR_NO_INTERFACE;

        if (!iid.equals(Components.interfaces.nsIFactory))
            throw Components.results.NS_ERROR_NOT_IMPLEMENTED;

        return this.mFactory;
    },

    mFactory: {
        createInstance: function (outer, iid) {
            if (outer != null)
                throw Components.results.NS_ERROR_NO_AGGREGATION;
            return (new calCompositeCalendar()).QueryInterface(iid);
        }
    },

    canUnload: function(compMgr) {
        return true;
    }
};

function NSGetModule(compMgr, fileSpec) {
    return calCompositeCalendarModule;
}
