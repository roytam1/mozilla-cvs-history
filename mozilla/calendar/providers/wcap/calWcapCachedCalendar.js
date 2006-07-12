/* -*- Mode: javascript; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: NPL 1.1/GPL 2.0/LGPL 2.1
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
 * The Initial Developer of the Original Code is Sun Microsystems, Inc.
 * Portions created by Sun Microsystems are Copyright (C) 2006 Sun
 * Microsystems, Inc. All Rights Reserved.
 *
 * Original Author: Daniel Boelzle (daniel.boelzle@sun.com)
 *
 * Contributor(s):
 *
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the NPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

/**
 * PROTOTYPE!
 */

// globals:
var g_localCals = {};
var g_localCalsLastSync = {};

function calWcapCachedCalendar() {
    this.wrappedJSObject = this;
    this.m_observers = [];
    this.m_observerMultiplexer = new ObserverMultiplexer(this);
    this.m_syncQueue = new RequestQueue();
}
calWcapCachedCalendar.prototype = {
    m_ifaces: [ Components.interfaces.calIWcapCalendar,
                Components.interfaces.calICalendar,
                Components.interfaces.nsIInterfaceRequestor,
                Components.interfaces.nsIClassInfo,
                Components.interfaces.nsISupports ],
    
    // nsISupports:
    QueryInterface:
    function( iid )
    {
        for each ( var iface in this.m_ifaces ) {
            if (iid.equals( iface ))
                return this;
        }
        throw Components.results.NS_ERROR_NO_INTERFACE;
    },
    
    // nsIClassInfo:
    getInterfaces:
    function( count )
    {
        count.value = this.m_ifaces.length;
        return this.m_ifaces;
    },
    get classDescription() {
        return calWcapCalendarModule.WcapCalendarInfo.classDescription;
    },
    get contractID() {
        return calWcapCalendarModule.WcapCalendarInfo.contractID;
    },
    get classID() {
        return calWcapCalendarModule.WcapCalendarInfo.classID;
    },
    getHelperForLanguage: function( language ) { return null; },
    implementationLanguage:
    Components.interfaces.nsIProgrammingLanguage.JAVASCRIPT,
    flags: 0,
    
    // nsIInterfaceRequestor:
    getInterface:
    function( iid, instance )
    {
        return this.remoteCal.getInterface( iid, instance );
    },
    
    m_observerMultiplexer: null,    
    m_remoteCal: null,
    get remoteCal() {
        if (this.m_remoteCal == null)
            this.remoteCal = new calWcapCalendar();
        return this.m_remoteCal;
    },
    set remoteCal( cal ) {
        if (this.m_remoteCal != null)
            this.m_remoteCal.removeObserver( this.m_observerMultiplexer );
        if (cal != null) {
            cal.addObserver( this.m_observerMultiplexer );
            cal.superCalendar = this;
        }
        this.m_remoteCal = cal;
    },
    
    getCalKey:
    function()
    {
        // xxx todo: better eoncoding?
        return encodeURIComponent( this.uri.spec /* i.e. remote uri */ +
                                   "?calid=" + this.calId );
    },
    
    m_localCal: null,
    get localCal() {
        if (this.m_localCal == null) {
            // xxx todo: sharing? storage cal_id: STRING?
            // assure logged in, so userId/calId is properly set:
            this.remoteCal.session.getSessionId();
            // xxx todo: better eoncoding?
            var key = this.getCalKey();
            var cal = g_localCals[key];
            if (!cal) {
                this.log( "creating cache calendar for calId " + this.calId );
                var uri;
                if (CACHE == "memory") { // in-memory caching
                    uri = this.uri.clone();
                    uri.path += ("?calid=" + this.calId);
                }
                else {
                    // xxx todo: change storage cal to support STRING cal_id
                    var dbPath = CACHE_DIR.clone();
                    dbPath.append( key + ".sdb" );
                    uri = getIoService().newFileURI( dbPath );
                }
                this.log( "local cal uri: " + uri.spec );
                try {
                    cal = getCalendarManager().createCalendar( CACHE, uri );
                    g_localCals[key] = cal;
                }
                catch (exc) {
                    this.notifyError( exc );
                }
            }
            this.localCal = cal;
        }
        return this.m_localCal;
    },
    set localCal( cal ) {
        if (cal != null) {
            cal.suppressAlarms = this.remoteCal.suppressAlarms; // sync setting
            cal.superCalendar = this;
        }
        this.m_localCal = cal;
    },
    
    toString:
    function()
    {
        return "cached-wcap | " + this.remoteCal.toString();
    },
    log:
    function( msg, context )
    {
        return this.remoteCal.log( msg, context ? context : this.toString() );
    },
    logError:
    function( err, context )
    {
        return this.remoteCal.logError(
            err, context ? context : this.toString() );
    },
    notifyError:
    function( err )
    {
        debugger;
        var str = this.logError( err );
        this.notifyObservers( "onError",
                              [err instanceof Error ? -1 : err, str] );
    },
    
    // calIWcapCalendar:
    // xxx todo: generic facade helpers for most function delegates
    
    getWcapErrorString:
    function( rc )
    {
        return this.remoteCal.getWcapErrorString(rc);
    },
    
    // xxx todo: which userId is used when for offline scheduling?
    //           if not logged in, no calId/userId is known... => UI
    get calId() { return this.remoteCal.calId; },
    set calId( id ) {
        this.localCal = null; // disconnect
        this.remoteCal.calId = id;
    },
    get userId() { return this.remoteCal.userId; },
    
    get isOwnedCalendar() { return this.remoteCal.isOwnedCalendar; },
    
    createCalendar:
    function( calId, name, bAllowDoubleBooking, bSetCalProps, bAddToSubscribed )
    {
        return this.remoteCal.createCalendar(
            calId, name, bAllowDoubleBooking, bSetCalProps, bAddToSubscribed );
    },
    
    deleteCalendar:
    function( calId, bRemoveFromSubscribed )
    {
        this.remoteCal.deleteCalendar( calId, bRemoveFromSubscribed );
    },
    
    getOwnedCalendars:
    function( out_count )
    {
        return this.remoteCal.getOwnedCalendars( out_count );
    },
    
    getSubscribedCalendars:
    function( out_count )
    {
        return this.remoteCal.getSubscribedCalendars( out_count );
    },
    
    subscribeToCalendars:
    function( count, calIds )
    {
        this.remoteCal.subscribeToCalendars( count, calIds );
    },
    
    unsubscribeFromCalendars:
    function( count, calIds )
    {
        this.remoteCal.unsubscribeFromCalendars( count, calIds );
    },
    
    getFreeBusyTimes:
    function( calId, rangeStart, rangeEnd, bBusyOnly, iListener,
              bAsync, requestId )
    {
        return this.remoteCal.getFreeBusyTimes(
            calId, rangeEnd, rangeEnd, bBusyOnly, iListener, bAsync, requestId);
    },
    
    get defaultTimezone() {
        return this.remoteCal.defaultTimezone;
    },
//     set defaultTimezone( tzid ) {
//     },    
    
    // calICalendar:
    get name() {
        return getCalendarManager().getCalendarPref( this, "NAME" );
    },
    set name( name ) {
        getCalendarManager().setCalendarPref( this, "NAME", name );
    },
    
    get type() { return "wcap"; },
    
    m_bReadOnly: false,
    get readOnly() { return (this.m_bReadOnly || this.remoteCal.readOnly); },
    set readOnly( bReadOnly ) { this.m_bReadOnly = bReadOnly; },
    
    get uri() { return this.remoteCal.uri; },
    set uri( thatUri ) {
        this.localCal = null; // disconnect
        this.remoteCal.uri = thatUri;
    },
    
    m_superCalendar: null,
    get superCalendar() { return this.m_superCalendar || this; },
    set superCalendar( cal ) { this.m_superCalendar = cal; },
    
    m_observers: null,
    notifyObservers:
    function( func, args )
    {
        this.m_observers.forEach(
            function( obj ) {
                try {
                    obj[func].apply( obj, args );
                }
                catch (exc) {
                    // don't call notifyError() here:
                    Components.utils.reportError( exc );
                }
            } );
    },
    
    addObserver:
    function( observer )
    {
        if (this.m_observers.indexOf( observer ) == -1) {
            this.m_observers.push( observer );
        }
    },
    
    removeObserver:
    function( observer )
    {
        this.m_observers = this.m_observers.filter(
            function(x) { return x != observer; } );
    },
    
    // xxx todo: batch currently not used
    startBatch: function() { this.notifyObservers( "onStartBatch", [] ); },
    endBatch: function() { this.notifyObservers( "onEndBatch", [] ); },
    
    get suppressAlarms() { return this.remoteCal.suppressAlarms; },
    set suppressAlarms( bSuppressAlarms ) {
        this.remoteCal.suppressAlarms = bSuppressAlarms;
        this.localCal.suppressAlarms = this.remoteCal.suppressAlarms;
    },
    
    get canRefresh() { return true; },
    
    refresh:
    function()
    {
        this.log( "refresh() call." );
        if (this.remoteCal.canRefresh)
            this.remoteCal.refresh();
        if (this.localCal.canRefresh)
            this.localCal.refresh();
        // sync in changes to local calendar:
        this.sync(null);
        this.log( "refresh() returning." );
    },
    
    adoptItem:
    function( item, iListener )
    {
        this.remoteCal.adoptItem( item, iListener );
    },
    
    addItem:
    function( item, iListener )
    {
        this.remoteCal.addItem( item, iListener );
    },
    
    modifyItem:
    function( newItem, oldItem, iListener )
    {
        this.remoteCal.modifyItem( newItem, oldItem, iListener );
    },
    
    deleteItem:
    function( item, iListener )
    {
        this.remoteCal.deleteItem( item, iListener );
    },
    
    getItem:
    function( id, iListener )
    {
        // xxx todo: testing
        this.log( ">>>>>>>>>>>>>>>> getItem() call!");
        this.refresh();
        this.localCal.getItem( id, iListener );
        this.log( "getItem() returning." );
    },
    
    getItems:
    function( itemFilter, maxResult, rangeStart, rangeEnd, iListener )
    {
        this.log( "getItems():\n\titemFilter=" + itemFilter +
                  ",\n\tmaxResult=" + maxResult +
                  ",\n\trangeStart=" + getIcalUTC(rangeStart) +
                  ",\n\trangeEnd=" + getIcalUTC(rangeEnd) );
        var this_ = this;
        this.sync(
            { // calIOperationListener:
                onOperationComplete:
                function( calendar, status, opType, id, detail )
                {
                    if (iListener != null) {
                        if (status == Components.results.NS_OK) {
                            // delegate to local cal:
                            this_.log("begin localCal.getItems().");
                            this_.localCal.getItems(
                                itemFilter, maxResult, rangeStart, rangeEnd,
                                iListener );
                            this_.log("end localCal.getItems().");
                        }
                        else {
                            iListener.onOperationComplete(
                                calendar, status, opType, id, detail );
                        }
                    }
                },
                onGetResult:
                function( calendar, status, itemType, detail, count, items )
                {
                    this_.notifyError( "unexpected onGetResult upon sync!" );
                }
            } );
    },
    
    getStampFile:
    function()
    {
        var stampFile = this.localCal.uri.QueryInterface(
            Components.interfaces.nsIFileURL ).file.clone();
        stampFile.leafName += ".last_sync";
        return stampFile;
    },
    
    getStamp:
    function()
    {
        var dtStamp = null;
        if (CACHE == "memory") {
            var stamp = g_localCalsLastSync[this.getCalKey()];
            if (stamp) // return null if undefined
                dtStamp = stamp;
        }
        else {
            var stampFile = this.getStampFile();
            if (stampFile.exists()) {
                dtStamp = new CalDateTime();
                dtStamp.jsDate = new Date(stampFile.lastModifiedTime); // is UTC
            }
        }
        return dtStamp;
    },
    
    updateStamp:
    function( dtStamp )
    {
        if (CACHE == "memory") {
            g_localCalsLastSync[this.getCalKey()] = dtStamp.clone();
        }
        else {
            var stampFile = this.getStampFile();
            // xxx todo: setting lastModifiedTime does not work
            //           (at least not on Windows)
//         stampFile.lastModifiedTime = (dtStamp.nativeTime / 1000);
            // xxx todo: changes inbetween get lost!
            if (stampFile.exists())
                stampFile.remove(false);
            stampFile.create(
                Components.interfaces.nsIFile.NORMAL_FILE_TYPE, 0700 );
        }
        if (LOG_LEVEL > 0) {
            var st = dtStamp;
            if (stampFile) {
                st = new CalDateTime();
                st.jsDate = new Date(stampFile.lastModifiedTime); // is UTC
            }
            this.log( "updated stamp to " + dtStamp + "\n\tnew stamp: " + st );
        }
    },
    
    m_syncQueue: null,
    sync:
    function( iListener )
    {
        this.log( "sync(): queueing request." );
        // serialize sync() calls into queue:
        var this_ = this;
        this.m_syncQueue.postRequest(
            function( requestToken ) {
                this_.sync_req( requestToken, iListener );
            } );
    },
    sync_req:
    function( requestToken, iListener )
    {
        try {
            var this_ = this;
            var localCal = this.localCal;
            var remoteCal = this.remoteCal;
            
            var dtFrom = this.getStamp();
            const SYNC = Components.interfaces.calIWcapCalendar.SYNC;
            
            // first sync in changes from remote, then get items from locally:
            remoteCal.syncChangesTo(
                localCal, dtFrom,
                { // calIOperationListener:
                    onOperationComplete:
                    function( calendar, status, opType, id, detail )
                    {
                        try {
                            if (status == Components.results.NS_OK) {
                                if (opType == SYNC) {
                                    // write stamp: only if necessary
                                    if (dtFrom == null ||
                                        dtFrom.compare(detail) != 0) {
                                        this_.updateStamp( detail );
                                    }
                                    if (iListener != null) {
                                        iListener.onOperationComplete(
                                            this_.superCalendar,
                                            Components.results.NS_OK,
                                            SYNC, null, null );
                                    }
                                }
                                else {
                                    throw new Error(
                                        "unexpected operation type! " +
                                        "(expected SYNC)" );
                                }
                            }
                            else {
                                if (iListener != null) // forward errors:
                                    iListener.onOperationComplete(
                                        calendar, status,
                                        opType, id, detail );
                                // already notified in wcap cal:
//                                 this_.notifyError( detail );
                            }
                        }
                        catch (exc) {
                            if (iListener != null) {
                                iListener.onOperationComplete(
                                    this_.superCalendar,
                                    Components.results.NS_ERROR_FAILURE,
                                    SYNC, null, exc );
                            }
                            this_.notifyError( exc );
                        }
                        this_.m_syncQueue.requestCompleted( requestToken );
                    },
                    onGetResult:
                    function( calendar, status, itemType, detail, count, items )
                    {
                        this_.notifyError( "unexpected onGetResult upon " +
                                           "calling syncChangesTo()!" );
                    }
                } );
        }
        catch (exc) {
            if (iListener != null) {
                iListener.onOperationComplete(
                    this.superCalendar, Components.results.NS_ERROR_FAILURE,
                    SYNC, null, exc );
            }
            this.notifyError( exc );
            this.m_syncQueue.requestCompleted( requestToken );
        }
        this.log( "sync_req() returning." );
    }
};

function ObserverMultiplexer( calendar ) {
    this.wrappedJSObject = this;
    this.m_calendar = calendar;
}
ObserverMultiplexer.prototype = {
    m_calendar: null,
    
    // calIObserver:
    onStartBatch:
    function()
    {
        this.m_calendar.notifyObservers( "onStartBatch", [] );
    },
    onEndBatch:
    function()
    {
        this.m_calendar.notifyObservers( "onEndBatch", [] );
    },
    onLoad:
    function()
    {
        this.m_calendar.notifyObservers( "onLoad", [] );
    },
    onAddItem:
    function( item )
    {
        this.m_calendar.notifyObservers( "onAddItem", [item] );
    },
    onModifyItem:
    function( newItem, oldItem )
    {
        this.m_calendar.notifyObservers( "onModifyItem", [newItem, oldItem] );
    },
    onDeleteItem:
    function( item )
    {
        this.m_calendar.notifyObservers( "onDeleteItem", [item] );
    },
    onAlarm:
    function( item )
    {
        this.m_calendar.notifyObservers( "onAlarm", [item] );
    },
    onError:
    function( errNo, msg )
    {
        this.m_calendar.notifyObservers( "onError", [errNo, msg] );
    }
};

