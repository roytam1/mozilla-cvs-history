/* -*- Mode: javascript; tab-width: 20; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
 * The Original Code is OEone Calendar Code, released October 31st, 2001.
 *
 * The Initial Developer of the Original Code is
 * OEone Corporation.
 * Portions created by the Initial Developer are Copyright (C) 2001
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s): Garth Smedley <garths@oeone.com>
 *                 Mike Potter <mikep@oeone.com>
 *                 Colin Phillips <colinp@oeone.com>
 *                 Karl Guertin <grayrest@grayrest.com> 
 *                 Mike Norton <xor@ivwnet.com>
 *                 ArentJan Banck <ajbanck@planet.nl> 
 *                 Eric Belhaire <belhaire@ief.u-psud.fr>
 *                 Matthew Willis <mattwillis@gmail.com>
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

/***** calendar
* AUTHOR
*   Garth Smedley
*
* REQUIRED INCLUDES 
*        <script type="application/x-javascript" src="chrome://calendar/content/calendarEvent.js"/>
*
* NOTES
*   Code for the calendar.
*
*   What is in this file:
*     - Global variables and functions - Called directly from the XUL
*     - Several classes:
*  
* IMPLEMENTATION NOTES 
*
**********
*/






/*-----------------------------------------------------------------
*  G L O B A L     V A R I A B L E S
*/

var gCalendar = null;

//the next line needs XX-DATE-XY but last X instead of Y
var gDateMade = "2002052213-cal"

// turn on debuging
var gDebugCalendar = false;

// ICal Library
var gICalLib = null;

// calendar event data source see penCalendarEvent.js
var gEventSource = null;

// single global instance of CalendarWindow
var gCalendarWindow;

//an array of indexes to boxes for the week view
var gHeaderDateItemArray = null;

//Show only the working days (changed in different menus)
var gOnlyWorkdayChecked ;
// ShowToDoInView
var gDisplayToDoInViewChecked ;

// DAY VIEW VARIABLES
var kDayViewHourLeftStart = 105;

var kDaysInWeek = 7;

const kMAX_NUMBER_OF_DOTS_IN_MONTH_VIEW = "8"; //the maximum number of dots that fit in the month view

var prefService = Components.classes["@mozilla.org/preferences-service;1"]
                            .getService(Components.interfaces.nsIPrefService);
var rootPrefNode = prefService.getBranch(null); // preferences root node

/*To log messages in the JSconsole */
var logMessage;
if( gDebugCalendar == true ) {
  var aConsoleService = Components.classes["@mozilla.org/consoleservice;1"].
    getService(Components.interfaces.nsIConsoleService);
  logMessage = aConsoleService.logStringMessage ;
} else 
{
  logMessage = function(){} ;  
}

/*To recognize the application running calendar*/
var applicationName = navigator.vendor ;
if(applicationName == "" ) applicationName = "Mozilla" ;
logMessage("application : " + applicationName);

var calendarsToPublish = new Array();


/*-----------------------------------------------------------------
*  G L O B A L     C A L E N D A R      F U N C T I O N S
*/

/**
 * This obsevers changes in calendar color prefs.
 * It removes all the style rules, and creates them
 * all again
 */

var categoryPrefObserver =
{
   mCalendarStyleSheet: null,
   observe: function(aSubject, aTopic, aPrefName)
   {
      var i;
      for (i = 0; i < this.mCalendarStyleSheet.cssRules.length ; ++i) {
        if (this.mCalendarStyleSheet.cssRules[i].selectorText.indexOf(".event-category-") == 0) {
          dump(this.mCalendarStyleSheet.cssRules+" "+this.mCalendarStyleSheet.cssRules[i].selectorText+"\n");
          this.mCalendarStyleSheet.deleteRule(i);
          --i;
        }
      }

      var categoryPrefBranch = prefService.getBranch("calendar.category.color.");
      var prefCount = { value: 0 };
      var prefArray = categoryPrefBranch.getChildList("", prefCount);
      for (i = 0; i < prefArray.length; ++i) {
         var prefName = prefArray[i];
         var prefValue = categoryPrefBranch.getCharPref(prefName);
         this.mCalendarStyleSheet.insertRule(".event-category-" + prefName + " { border-color: " + prefValue +" !important; }", 1);
      }
   }
}

/** 
* Called from calendar.xul window onload.
*/

function calendarInit() 
{
    // XXX remove this eventually
    gICalLib = new Object();

   // set up the CalendarWindow instance
   
   gCalendarWindow = new CalendarWindow();
   
   //when you switch to a view, it takes care of refreshing the events, so that call is not needed.
   gCalendarWindow.currentView.switchTo( gCalendarWindow.currentView );

   // set up the checkboxes variables
   gOnlyWorkdayChecked = document.getElementById( "only-workday-checkbox-1" ).getAttribute("checked") ;
   gDisplayToDoInViewChecked = document.getElementById( "display-todo-inview-checkbox-1" ).getAttribute("checked") ;

   // set up the unifinder
   
   prepareCalendarUnifinder();

   prepareCalendarToDoUnifinder();
   
   update_date();

   checkForMailNews();

   initCalendarManager();

   //XXX Reimplement this function so that eventboxes will be colored
   //updateColors();
}

function updateColors()
{
    // Change made by CofC for Calendar Coloring
    // initialize calendar color style rules in the calendar's styleSheet

    // find calendar's style sheet index
    for (var i in document.styleSheets) {
        if (document.styleSheets[i].href.match(
                /chrome.*\/skin.*\/calendar.css$/ )) {
            var calStyleSheet = document.styleSheets[i];
            break;
        }
    }

    // get the list of all calendars from the manager
    const calMgr = getCalendarManager();
    var count = {};
    var calendars = calMgr.getCalendars(count);
    
    var calListItems = document.getElementById( "list-calendars-listbox" )
        .getElementsByTagName("listitem");

    for(i in calendars) {
        // XXX need to get this from the calendar prefs
        const containerName = "default";

        // XXX need to get this from the calendar prefs
        const calendarColor = "#FFFFFF"; // XXX what is default?

        // if the calendar had a color attribute create a style sheet for it
        if (calendarColor != null) {
            calStyleSheet.insertRule("." + containerName
                                     + " { background-color:"
                                     + calendarColor + "!important;}", 1);
            calStyleSheet.insertRule("." + containerName + " { color:"
                                     + getContrastingTextColor(calendarColor)
                                     + "!important;}", 1);
            //dump("calListItems[0] = " + calListItems[0] + "\n");
            //dump("calListItems[1] = " + calListItems[1] + "\n");
            var calListItem = calListItems[i];
            if (calListItem && calListItem.childNodes[0]) {
                calListItem.childNodes[0]
                    .setAttribute("class", "calendar-list-item-class "
                                  + containerName);
            }
        }
    }

    // Setup css classes for category colors
    var catergoryPrefBranch = prefService.getBranch("");
    var pbi = catergoryPrefBranch.QueryInterface(
        Components.interfaces.nsIPrefBranch2);
    pbi.addObserver("calendar.category.color.", categoryPrefObserver, false);
    categoryPrefObserver.mCalendarStyleSheet = calStyleSheet;
    categoryPrefObserver.observe(null, null, "");

    if( ("arguments" in window) && (window.arguments.length) &&
        (typeof(window.arguments[0]) == "object") &&
        ("channel" in window.arguments[0]) ) {
        gCalendarWindow.calendarManager.checkCalendarURL( 
            window.arguments[0].channel );
    }

    // a bit of a hack since the menulist doesn't remember the selected value
    var value = document.getElementById( 'event-filter-menulist' ).value;
    document.getElementById( 'event-filter-menulist' ).selectedItem = 
        document.getElementById( 'event-filter-'+value );

    // XXX busted somehow, so I've commented it out for now.  also, why the
    // heck are we doing this here?
    //gEventSource.alarmObserver.firePendingAlarms();

    // All is settled, enable feedbacks to observers
    gICalLib.batchMode = false;

    var toolbox = document.getElementById("calendar-toolbox");
    toolbox.customizeDone = CalendarToolboxCustomizeDone;
}

// Set the date and time on the clock and set up a timeout to refresh the clock when the 
// next minute ticks over

function update_date()
{
   // get the current time
   var now = new Date();
   
   var tomorrow = new Date( now.getFullYear(), now.getMonth(), ( now.getDate() + 1 ) );
   
   var milliSecsTillTomorrow = tomorrow.getTime() - now.getTime();
   
   gCalendarWindow.currentView.hiliteTodaysDate();

   setTimeout( "update_date()", milliSecsTillTomorrow ); 
}

/** 
* Called from calendar.xul window onunload.
*/

function calendarFinish()
{
   finishCalendarUnifinder();
   
   finishCalendarToDoUnifinder();

   var pbi = prefService.getBranch("");
   pbi = pbi.QueryInterface(Components.interfaces.nsIPrefBranch2);
   pbi.removeObserver("calendar.category.color.", categoryPrefObserver);

   gCalendarWindow.close();

   gICalLib.removeObserver( gEventSource.alarmObserver );
}

function launchPreferences()
{
    if (applicationName == "Mozilla" || applicationName == "Firebird")
        goPreferences( "calendarPanel", "chrome://calendar/content/pref/calendarPref.xul", "calendarPanel" );
    else
        window.openDialog("chrome://calendar/content/pref/prefBird.xul", "PrefWindow", "chrome,titlebar,resizable,modal");
}

/** 
* Called on double click in the day view all-day area
* Could be used for week view too...
*
*/
function dayAllDayDoubleClick( event )
{
  if( event ) {
    if( event.button == 0 )
      newEvent( null, null, true );
    event.stopPropagation();
  }
}

/** 
* Called on single click in the day view, select an event
*
* PARAMETERS
*    hourNumber - 0-23 hard-coded in the XUL
*    event      - the click event, Not used yet 
*/

function dayEventItemClick( eventBox, event )
{
   //do this check, otherwise on double click you get into an infinite loop
   if( event.detail == 1 )
      gCalendarWindow.EventSelection.replaceSelection( eventBox.event );
   
   if ( event ) 
   {
      event.stopPropagation();
   }
}


/** 
* Called on double click in the day view, edit an existing event
* or create a new one.
*
* PARAMETERS
*    hourNumber - 0-23 hard-coded in the XUL
*    event      - the click event, Not used yet 
*/

function dayEventItemDoubleClick( eventBox, event )
{
   // we only care about button 0 (left click) events
   if (event.button != 0) return;
   
   editEvent( eventBox.event.parentItem );

   if ( event ) 
   {
      event.stopPropagation();
   }
}


/** 
* Called on single click in the hour area in the day view
*
* PARAMETERS
*    hourNumber - 0-23 hard-coded in the XUL
*    event      - the click event, Not used yet 
*/

function dayViewHourClick( event )
{
   if( event.detail == 1 )
      gCalendarWindow.setSelectedHour( event.target.getAttribute( "hour" ) );
}


/** 
* Called on single click in the hour area in the day view
*
* PARAMETERS
*    hourNumber - 0-23 hard-coded in the XUL
*    event      - the click event, Not used yet 
*/

function dayViewHourContextClick( event )
{
   var dayIndex = event.target.getAttribute( "day" );

   gNewDateVariable = gCalendarWindow.getSelectedDate();
   
   gNewDateVariable.setHours( event.target.getAttribute( "hour" ) );

   gNewDateVariable.setMinutes( 0 );
}


/**
* Called on double click of an hour box.
*/

function dayViewHourDoubleClick( event )
{
   // we only care about button 0 (left click) events
   if (event.button != 0) return;
   
   var startDate = gCalendarWindow.dayView.getNewEventDate();
   
   newEvent( startDate );
}


/** 
* Called on single click in the day view, select an event
*
* PARAMETERS
*    hourNumber - 0-23 hard-coded in the XUL
*    event      - the click event, Not used yet 
*/

function weekEventItemClick(eventBox, event)
{
    //do this check, otherwise on double click you get into an infinite loop
    if (event.detail == 1) {
        var calEvent = eventBox.event;

        gCalendarWindow.EventSelection.replaceSelection(calEvent);

        var newDate = new Date(calEvent.startDate.jsDate);

        gCalendarWindow.setSelectedDate(newDate, false);
    }

    if (event) {
        event.stopPropagation();
    }
}


/** 
* Called on double click in the day view, edit an existing event
* or create a new one.
*
* PARAMETERS
*    hourNumber - 0-23 hard-coded in the XUL
*    event      - the click event, Not used yet 
*/

function weekEventItemDoubleClick( eventBox, event )
{
   // we only care about button 0 (left click) events
   if (event.button != 0) return;
   
   editEvent( eventBox.event.parentItem );

   if ( event ) 
   {
      event.stopPropagation();
   }
}

/** ( event )
* Called on single click in the hour area in the day view
*
* PARAMETERS
*    hourNumber - 0-23 hard-coded in the XUL
*    event      - the click event, Not used yet 
*/

function weekViewHourClick( event )
{
   if( event.detail == 1 )
   {
      var dayIndex = event.target.getAttribute( "day" );

      var newDate = new Date( gHeaderDateItemArray[dayIndex].getAttribute( "date" ) );

      newDate.setHours( event.target.getAttribute( "hour" ) );

      gCalendarWindow.setSelectedDate( newDate );
   }
}


/** ( event )
* Called on single click in the hour area in the day view
*
* PARAMETERS
*    hourNumber - 0-23 hard-coded in the XUL
*    event      - the click event, Not used yet 
*/

function weekViewContextClick( event )
{
   var dayIndex = event.target.getAttribute( "day" );

   gNewDateVariable = new Date( gHeaderDateItemArray[dayIndex].getAttribute( "date" ) );
   
   gNewDateVariable.setHours( event.target.getAttribute( "hour" ) );
}


/**
* Called on double click of an hour box.
*/

function weekViewHourDoubleClick( event )
{
   // we only care about button 0 (left click) events
   if (event.button != 0) return;
        
   var startDate = gCalendarWindow.weekView.getNewEventDate();
   
   newEvent( startDate );
}


/** 
* Called on single click on an event box in the month view
*
* PARAMETERS
*    eventBox - The XUL box clicked on
*    event      - the click event
*/

function monthEventBoxClickEvent( eventBox, event )
{
   //do this check, otherwise on double click you get into an infinite loop
   if( event.detail == 1 )
   {
      gCalendarWindow.EventSelection.replaceSelection( eventBox.event );
      
      var newDate = gCalendarWindow.getSelectedDate();

      newDate.setDate( eventBox.event.startDate.day );

      gCalendarWindow.setSelectedDate( newDate, false );
   }

   if ( event ) 
   {
      event.stopPropagation();
   }
}


/** 
* Called on double click on an event box in the month view, 
* launches the edit dialog on the event
*
* PARAMETERS
*    eventBox - The XUL box clicked on
*/

function monthEventBoxDoubleClickEvent( eventBox, event )
{
   // we only care about button 0 (left click) events
   if (event.button != 0) return;
   
   gCalendarWindow.monthView.clearSelectedDate();
   
   editEvent( eventBox.event.parentItem );

   if ( event ) 
   {
      event.stopPropagation();
   }
}
   

/** 
* Called on single click on an todo box in the multiweek view
*
* PARAMETERS
*    todoBox - The XUL box clicked on
*    event      - the click event
*/

function multiweekToDoBoxClickEvent( todoBox, event )
{
   //do this check, otherwise on double click you get into an infinite loop
   if( event.detail == 1 )
   {
      gCalendarWindow.EventSelection.replaceSelection( todoBox.calendarToDo );
      
      var newDate = gCalendarWindow.getSelectedDate();

      newDate.setDate( todoBox.calendarToDo.due.day );

      gCalendarWindow.setSelectedDate( newDate, false );
   }

   if ( event ) 
   {
      event.stopPropagation();
   }
}


/** 
* Called on double click on an todo box in the multiweek view
* launches the edit dialog on the event
*
* PARAMETERS
*    todoBox - The XUL box clicked on
*    event      - the click event
*/

function multiweekToDoBoxDoubleClickEvent( todoBox, event )
{
   // we only care about button 0 (left click) events
   if (event.button != 0) return;
   
   gCalendarWindow.multiweekView.clearSelectedDate();
   
   editEvent( todoBox.calendarToDo.parentItem );

   if ( event ) 
   {
      event.stopPropagation();
   }
}

/** 
* Called when the new event button is clicked
*/
var gNewDateVariable = null;

function newEventCommand( event )
{
   newEvent();
}


/** 
* Called when the new task button is clicked
*/

function newToDoCommand()
{
  newToDo( null, null ); // new task button defaults to undated todo
}

function newCalendarDialog()
{
    openCalendarWizard();
}

function editCalendarDialog(event)
{
    openCalendarProperties(document.popupNode.calendar, null);
}

function calendarListboxDoubleClick(event) {
    if(event.target.calendar)
        openCalendarProperties(event.target.calendar, null);
    else
        openCalendarWizard();
}

function checkCalListTarget() {
    if(!document.popupNode.calendar) {
        document.getElementById("calpopup-edit").setAttribute("disabled", "true");
        document.getElementById("calpopup-delete").setAttribute("disabled", "true");
        document.getElementById("calpopup-publish").setAttribute("disabled", "true");
    }
    else {
        document.getElementById("calpopup-edit").removeAttribute("disabled");
        document.getElementById("calpopup-delete").removeAttribute("disabled");
        document.getElementById("calpopup-publish").removeAttribute("disabled");
    }
}

function deleteCalendar(event)
{
    var cal = document.popupNode.calendar
    getDisplayComposite().removeCalendar(cal.uri);
    var calMgr = getCalendarManager();
    calMgr.unregisterCalendar(cal);
    // Delete file?
    //calMgr.deleteCalendar(cal);
}


function appendCalendars(to, froms, listener)
{
    var getListener = {
        onOperationComplete: function(aCalendar, aStatus, aOperationType, aId, aDetail)
        {
            if (listener)
                listener.onOperationComplete(aCalendar, aStatus, aOperationType,
                                             aId, aDetail);
        },
        onGetResult: function(aCalendar, aStatus, aItemType, aDetail, aCount, aItems)
        {
            if (!Components.isSuccessCode(aStatus)) {
                aborted = true;
                return;
            }
            if (aCount) {
                for (var i=0; i<aCount; ++i) {
                    // Store a (short living) reference to the item.
                    var itemCopy = aItems[i].clone();
                    to.addItem(itemCopy, null);
                }  
            }
        }
    };


    for each(var from in froms) {
        from.getItems(Components.interfaces.calICalendar.ITEM_FILTER_TYPE_ALL,
                      0, null, null, getListener);
    }
}



/* 
* returns true if lastModified match
*
*If a client for some reason modifies the item without
*touching lastModified this will obviously fail
*/
function compareItems( aObject1, aObject2 ){
   if ( aObject1 == null || aObject2 == null){
      return false;
   }
   //workaround Bug 270644, normally I should only check lastModified
   //but on new events such property throws an error and need a try
   try{ 
      return (aObject1.lastModified == aObject2.lastModified);
   }catch(er){
      return (aObject1.stamp.getTime() == aObject2.stamp.getTime());
   }
}


/* 
* useful to get the new version of an item
* in order to check if it changed 
*/
function fetchItem( aObject ){
   try{
      if ( isToDo(aObject) ) {
         return gICalLib.fetchTodo( aObject.id ); 
      } else {
         return gICalLib.fetchEvent( aObject.id );
      }
   }catch(er){
   }
   return null;
}

/** 
* Defaults null start/end date based on selected date in current view.
* Defaults calendarFile to the selected calendar file.
* Calls editNewEvent. 
*/

function newEvent(startDate, endDate, allDay)
{
   // create a new event to be edited and added
   var calendarEvent = createEvent();

   if (!startDate) {
       startDate = gCalendarWindow.currentView.getNewEventDate();
   }

   calendarEvent.startDate.jsDate = startDate;

   if (!endDate) {
       var MinutesToAddOn = getIntPref(gCalendarWindow.calendarPreferences.calendarPref, "event.defaultlength", gCalendarBundle.getString("defaultEventLength" ) );
       
       endDate = new Date(startDate);
       endDate.setMinutes(endDate.getMinutes() + MinutesToAddOn);
   }

   calendarEvent.endDate.jsDate = endDate

   if (allDay)
       calendarEvent.startDate.isDate = true;

   var calendar = getSelectedCalendarOrNull();

   editNewEvent( calendarEvent, calendar );
}

/*
* Defaults null start/due date to the no_date date.
* Defaults calendarFile to the selected calendar file.
* Calls editNewToDo.
*/
function newToDo ( startDate, dueDate ) 
{
    var calendarToDo = createToDo();
   
    // created todo has no start or due date unless user wants one
    if (startDate) 
        calendarToDo.entryTime.jsDate = startDate;

    if (dueDate)
        calendarToDo.dueDate.jsDate = dueDate;

    var calendar = getSelectedCalendarOrNull();
    
    editNewToDo(calendarToDo, calendar);
}

/**
 * Get the default calendar selected in the calendars tab.
 * Returns a calICalendar object, or null if none selected.
 */
function getSelectedCalendarOrNull()
{
   var selectedCalendarItem = document.getElementById( "list-calendars-listbox" ).selectedItem;
   
   if ( selectedCalendarItem )
     return selectedCalendarItem.calendar;
   else
     return null;
}

/**
* Launch the event dialog to edit a new (created, imported, or pasted) event.
* 'calendar' is a calICalendar object.
* When the user clicks OK "addEventDialogResponse" is called
*/

function editNewEvent( calendarEvent, calendar )
{
  openEventDialog(calendarEvent,
                  "new",
                  self.addEventDialogResponse,
                  calendar);
}

/**
* Launch the todo dialog to edit a new (created, imported, or pasted) ToDo.
* 'calendar' is a calICalendar object.
* When the user clicks OK "addToDoDialogResponse" is called
*/
function editNewToDo( calendarToDo, calendar )
{
  openEventDialog(calendarToDo,
                  "new",
                  self.addToDoDialogResponse,
                  calendar);
}

/** 
* Called when the user clicks OK in the new event dialog
* 'calendar' is a calICalendar object.
*
* Updates the data source.  The unifinder views and the calendar views will be
* notified of the change through their respective observers.
*/

function addEventDialogResponse( calendarEvent, calendar )
{
   saveItem( calendarEvent, calendar, "addEvent" );
}


/** 
* Called when the user clicks OK in the new to do item dialog
* 'calendar' is a calICalendar object.
*/

function addToDoDialogResponse( calendarToDo, calendar )
{
    addEventDialogResponse(calendarToDo, calendar);
}


/** 
* Helper function to launch the event dialog to edit an event.
* When the user clicks OK "modifyEventDialogResponse" is called
*/

function editEvent( calendarEvent )
{
  openEventDialog(calendarEvent,
                  "edit",
                  self.modifyEventDialogResponse,
                  null);
}

function editToDo( calendarTodo )
{
  openEventDialog(calendarTodo,
                  "edit",
                  self.modifyEventDialogResponse,
                  null);
}
   
/** 
* Called when the user clicks OK in the edit event dialog
* 'calendar' is a calICalendar object.
*
* Update the data source, the unifinder views and the calendar views will be
* notified of the change through their respective observers
*/

function modifyEventDialogResponse( calendarEvent, calendar, originalEvent )
{
   saveItem( calendarEvent, calendar, "modifyEvent", originalEvent );
}


/** 
* Called when the user clicks OK in the edit event dialog
* 'calendar' is a calICalendar object.
*
* Update the data source, the unifinder views and the calendar views will be
* notified of the change through their respective observers
*/

function modifyToDoDialogResponse( calendarToDo, calendar, originalToDo )
{
    modifyEventDialogResponse(calendarToDo, calendar, originalToDo);
}


/** PRIVATE: open event dialog in mode, and call onOk if ok is clicked.
    'mode' is "new" or "edit".
    'calendar' is default calICalendar, typically from getSelectedCalendarOrNull
 **/
function openEventDialog(calendarEvent, mode, onOk, calendar)
{
  // set up a bunch of args to pass to the dialog
  var args = new Object();
  args.calendarEvent = calendarEvent;
  args.mode = mode;
  args.onOk = onOk;

  if( calendar )
    args.calendar = calendar;

  // wait cursor will revert to auto in eventDialog.js loadCalendarEventDialog
  window.setCursor( "wait" );
  // open the dialog modally
  openDialog("chrome://calendar/content/eventDialog.xul", "caEditEvent", "chrome,titlebar,modal", args );
}

/**
*  This is called from the unifinder's edit command
*/

function editEventCommand()
{
   if( gCalendarWindow.EventSelection.selectedEvents.length == 1 )
   {
      var calendarEvent = gCalendarWindow.EventSelection.selectedEvents[0];

      if( calendarEvent != null )
      {
         editEvent( calendarEvent.parentItem );
      }
   }
}


//originalEvent is the item before edits were committed, 
//used to check if there were external changes for shared calendar
function saveItem( calendarEvent, calendar, functionToRun, originalEvent )
{
    dump(functionToRun + " " + calendarEvent.title + "\n");

    if (functionToRun == 'addEvent') {
        doTransaction('add', calendarEvent, calendar, null, null);
    } else if (functionToRun == 'modifyEvent') {
        // compare cal.uri because there may be multiple instances of
        // calICalendar or uri for the same spec, and those instances are
        // not ==.
        if (!originalEvent.calendar || 
            (originalEvent.calendar.uri.equals(calendar.uri)))
            doTransaction('modify', calendarEvent, calendar, originalEvent, null);
        else
            doTransaction('move', calendarEvent, calendar, originalEvent, null);
    }
}


/**
*  This is called from the unifinder's delete command
*
*/
function deleteItems( SelectedItems, DoNotConfirm )
{
    if (!SelectedItems)
        return;

    startBatchTransaction();
    for (i in SelectedItems) {
        doTransaction('delete', SelectedItems[i], SelectedItems[i].calendar, null, null);
    }
    endBatchTransaction();
}


/**
*  Delete the current selected item with focus from the ToDo unifinder list
*/
function deleteEventCommand( DoNotConfirm )
{
   var SelectedItems = gCalendarWindow.EventSelection.selectedEvents;
   deleteItems( SelectedItems, DoNotConfirm );
   for ( i in  SelectedItems) {
      gCalendarWindow.clearSelectedEvent( SelectedItems[i] );
   }
}


/**
*  Delete the current selected item with focus from the ToDo unifinder list
*/
function deleteToDoCommand( DoNotConfirm )
{
   var SelectedItems = new Array();
   var tree = document.getElementById( ToDoUnifinderTreeName );
   var start = new Object();
   var end = new Object();
   var numRanges = tree.view.selection.getRangeCount();
   var t;
   var v;
   var toDoItem;
   for (t = 0; t < numRanges; t++) {
      tree.view.selection.getRangeAt(t, start, end);
      for (v = start.value; v <= end.value; v++) {
         toDoItem = tree.taskView.getCalendarTaskAtRow( v );
         SelectedItems.push( toDoItem );
      }
   }
   deleteItems( SelectedItems, DoNotConfirm );
   tree.view.selection.clearSelection();
}


function goFindNewCalendars()
{
   //launch the browser to http://www.apple.com/ical/library/
   var browserService = penapplication.getService( "org.penzilla.browser" );
   if(browserService)
   {
       browserService.setUrl("http://www.icalshare.com/");
       browserService.focusBrowser();
   }
}

var gSelectAll = false;

function selectAllEvents()
{
   gSelectAll = true;

   gCalendarWindow.EventSelection.setArrayToSelection( gEventSource.currentEvents );
}

function closeCalendar()
{
   self.close();
}


function reloadApplication()
{
    gEventSource.calendarManager.refreshAllRemoteCalendars();
}


/** PUBLIC
*
*   Print events using a stylesheet.
*   Mostly Hack to get going, Should probably be rewritten later when stylesheets are available
*/

function printEventArray( calendarEventArray, stylesheetName )
{
   var xslProcessor = new XSLTProcessor();
   var domParser = new DOMParser;
   var xcsDocument = getXcsDocument( calendarEventArray );

   printWindow = window.open( "", "CalendarPrintWindow");
   if( printWindow )
   {
      // if only passsed a filename, assume it is a file in the default directory
      if( stylesheetName.indexOf( ":" ) == -1 )
         stylesheetName = convertersDirectory + stylesheetName;

      var stylesheetUrl = Components.classes["@mozilla.org/network/standard-url;1"].createInstance(Components.interfaces.nsIURI);
      stylesheetUrl.spec = convertersDirectory;

      domParser.baseURI = stylesheetUrl;
      var xslContent = loadFile( stylesheetName );
      var xslDocument = domParser.parseFromString(xslContent, 'application/xml');

      // hack, might be cleaner to assing xml document directly to printWindow.document
      // var elementNode = xcsDocument.documentElement;
      // result.appendChild(elementNode); // doesn't work

      xslProcessor.transformDocument(xcsDocument, xslDocument, printWindow.document, null);

      printWindow.locationbar.visible = false;
      printWindow.personalbar.visible = false;
      printWindow.statusbar.visible = false;
      printWindow.toolbar.visible = false;

      printWindow.print();
      printWindow.close();
   }
}

function print()
{
   var args = new Object();

   args.eventSource = gEventSource;
   args.selectedEvents = gCalendarWindow.EventSelection.selectedEvents ;
   args.selectedDate=gNewDateVariable = gCalendarWindow.getSelectedDate();

   var Offset = getIntPref(gCalendarWindow.calendarPreferences.calendarPref, 
                           "week.start", 
                           gCalendarBundle.getString("defaultWeekStart" ) );
   var WeeksInView = getIntPref(gCalendarWindow.calendarPreferences.calendarPref, 
                                "weeks.inview", 
                                gCalendarBundle.getString("defaultWeeksInView" ) );
   WeeksInView = ( WeeksInView >= 6 ) ? 6 : WeeksInView ;

   var PreviousWeeksInView = getIntPref(gCalendarWindow.calendarPreferences.calendarPref, 
                                        "previousweeks.inview", 
                                        gCalendarBundle.getString("defaultPreviousWeeksInView" ) );
   PreviousWeeksInView = ( PreviousWeeksInView >= WeeksInView - 1 ) ? WeeksInView - 1 : PreviousWeeksInView ;

   args.startOfWeek=Offset;
   args.weeksInView=WeeksInView;
   args.prevWeeksInView=PreviousWeeksInView;

   window.openDialog("chrome://calendar/content/printDialog.xul","printdialog","chrome",args);
}


function publishEntireCalendar()
{
    var args = new Object();

    args.onOk =  self.publishEntireCalendarDialogResponse;

    var remotePath = ""; // get a remote path as a pref of the calendar

    if (remotePath != "" && remotePath != null) {
        var publishObject = new Object( );
        publishObject.remotePath = remotePath;
        args.publishObject = publishObject;
    }

    openDialog("chrome://calendar/content/publishDialog.xul", "caPublishEvents", "chrome,titlebar,modal", args );
}

function publishEntireCalendarDialogResponse( CalendarPublishObject )
{
    var icsURL = makeURL(CalendarPublishObject.remotePath);

    var oldCalendar = getDefaultCalendar(); // get the currently selected calendar

    // create an ICS calendar, but don't register it
    var calManager = getCalendarManager();
    try {
        var newCalendar = calManager.createCalendar("ics", icsURL);
    } catch (ex) {
        dump(ex);
        return;
    }

    var getListener = {
        onOperationComplete: function(aCalendar, aStatus, aOperationType, aId, aDetail)
        {
            // delete the new calendar now that we're done with it
            calManager.deleteCalendar(newCalendar);
        }
    };

    appendCalendars(newCalendar, [oldCalendar], getListener);
}

function publishCalendarData()
{
   var args = new Object();
   
   args.onOk =  self.publishCalendarDataDialogResponse;
   
   openDialog("chrome://calendar/content/publishDialog.xul", "caPublishEvents", "chrome,titlebar,modal", args );
}

function publishCalendarDataDialogResponse( CalendarPublishObject )
{
   var calendarString = eventArrayToICalString( gCalendarWindow.EventSelection.selectedEvents );
   
   calendarPublish(calendarString, CalendarPublishObject.remotePath, "text/calendar");
}



function getCharPref (prefObj, prefName, defaultValue)
{
    try {
        return prefObj.getCharPref (prefName);
    } catch (e) {
        prefObj.setCharPref( prefName, defaultValue );  
        return defaultValue;
    }
}

function getIntPref(prefObj, prefName, defaultValue)
{
    try {
        return prefObj.getIntPref(prefName);
    } catch (e) {
        prefObj.setIntPref(prefName, defaultValue);  
        return defaultValue;
    }
}

function getBoolPref (prefObj, prefName, defaultValue)
{
    try
    {
        return prefObj.getBoolPref (prefName);
    }
    catch (e)
    {
       prefObj.setBoolPref( prefName, defaultValue );  
       return defaultValue;
    }
}

function GetUnicharPref(prefObj, prefName, defaultValue)
{
    try {
      return prefObj.getComplexValue(prefName, Components.interfaces.nsISupportsString).data;
    }
    catch(e)
    {
      SetUnicharPref(prefObj, prefName, defaultValue);
        return defaultValue;
    }
}

function SetUnicharPref(aPrefObj, aPrefName, aPrefValue)
{
    try {
      var str = Components.classes["@mozilla.org/supports-string;1"]
                          .createInstance(Components.interfaces.nsISupportsString);
      str.data = aPrefValue;
      aPrefObj.setComplexValue(aPrefName, Components.interfaces.nsISupportsString, str);
    }
    catch(e) {}
}

/* Change the only-workday checkbox */
function changeOnlyWorkdayCheckbox( menuindex ) {
  var check = document.getElementById( "only-workday-checkbox-" + menuindex ).getAttribute("checked") ;
  var changemenu ;
  switch(menuindex){
  case 1:
    changemenu = 2 ;
    break;
  case 2:
    changemenu = 1 ;
    break;
  default:
    return;
  }
  if(check == "true") {
    document.getElementById( "only-workday-checkbox-" + changemenu ).setAttribute("checked","true");
    gOnlyWorkdayChecked = "true" ;
  }
  else {
    document.getElementById( "only-workday-checkbox-" + changemenu ).removeAttribute("checked");
    gOnlyWorkdayChecked = "false" ;
  }
  gCalendarWindow.currentView.refreshDisplay( );
  gCalendarWindow.currentView.refreshEvents( );
}

/* Change the display-todo-inview checkbox */
function changeDisplayToDoInViewCheckbox( menuindex ) {
  var check = document.getElementById( "display-todo-inview-checkbox-" + menuindex ).getAttribute("checked") ;
  var changemenu ;
  switch(menuindex){
  case 1:
    changemenu = 2 ;
    break;
  case 2:
    changemenu = 1 ;
    break;
  default:
    return;
  }
  if(check == "true") {
    document.getElementById( "display-todo-inview-checkbox-" + changemenu ).setAttribute("checked","true");
    gDisplayToDoInViewChecked = "true" ;
  }
  else {
    document.getElementById( "display-todo-inview-checkbox-" + changemenu ).removeAttribute("checked");
    gDisplayToDoInViewChecked = "false" ;
  }
  gCalendarWindow.currentView.refreshEvents( );
}

// about Calendar dialog
function displayCalendarVersion()
{
  // uses iframe, but iframe does not auto-size per bug 80713, so provide height
  window.openDialog("chrome://calendar/content/about.xul", "About","modal,centerscreen,chrome,width=500,height=450,resizable=yes");
}


// about Sunbird dialog
function openAboutDialog()
{
  window.openDialog("chrome://calendar/content/aboutDialog.xul", "About", "modal,centerscreen,chrome,resizable=no");
}

function openPreferences()
{
  openDialog("chrome://calendar/content/pref/pref.xul","PrefWindow",
             "chrome,titlebar,resizable,modal");
}

// Next two functions make the password manager menu option
// only show up if there is a wallet component. Assume that
// the existence of a wallet component means wallet UI is there too.
function checkWallet()
{
  if ('@mozilla.org/wallet/wallet-service;1' in Components.classes) {
    document.getElementById("password-manager-menu")
            .removeAttribute("hidden");
  }
}

function openWalletPasswordDialog()
{
  window.openDialog("chrome://communicator/content/wallet/SignonViewer.xul",
                    "_blank","chrome,resizable=yes","S");
}

var strBundleService = null;
function srGetStrBundle(path)
{
  var strBundle = null;

  if (!strBundleService) {
      try {
          strBundleService =
              Components.classes["@mozilla.org/intl/stringbundle;1"].getService();
          strBundleService =
              strBundleService.QueryInterface(Components.interfaces.nsIStringBundleService);
      } catch (ex) {
          dump("\n--** strBundleService failed: " + ex + "\n");
          return null;
      }
  }

  strBundle = strBundleService.createBundle(path);
  if (!strBundle) {
        dump("\n--** strBundle createInstance failed **--\n");
  }
  return strBundle;
}

function CalendarCustomizeToolbar()
{
  // Disable the toolbar context menu items
  var menubar = document.getElementById("main-menubar");
  for (var i = 0; i < menubar.childNodes.length; ++i)
    menubar.childNodes[i].setAttribute("disabled", true);
    
  var cmd = document.getElementById("cmd_CustomizeToolbars");
  cmd.setAttribute("disabled", "true");
  
  window.openDialog("chrome://calendar/content/customizeToolbar.xul", "CustomizeToolbar",
                    "chrome,all,dependent", document.getElementById("calendar-toolbox"));
}

function CalendarToolboxCustomizeDone(aToolboxChanged)
{
  // Re-enable parts of the UI we disabled during the dialog
  var menubar = document.getElementById("main-menubar");
  for (var i = 0; i < menubar.childNodes.length; ++i)
    menubar.childNodes[i].setAttribute("disabled", false);
  var cmd = document.getElementById("cmd_CustomizeToolbars");
  cmd.removeAttribute("disabled");

  // XXX Shouldn't have to do this, but I do
  window.focus();
}

/**
 * Pick whichever of "black" or "white" will look better when used as a text
 * color against a background of bgColor. 
 *
 * @param bgColor   the background color as a "#RRGGBB" string
 */
function getContrastingTextColor(bgColor)
{
    var calcColor = bgColor.replace(/#/g, "");
    var red = parseInt(calcColor.substring(0, 2), 16);
    var green = parseInt(calcColor.substring(2, 4), 16);
    var blue = parseInt(calcColor.substring(4, 6), 16);

    // Calculate the L(ightness) value of the HSL color system.
    // L = (max(R, G, B) + min(R, G, B)) / 2
    var max = Math.max(Math.max(red, green), blue);
    var min = Math.min(Math.min(red, green), blue);
    var lightness = (max + min) / 2;

    // Consider all colors with less than 50% Lightness as dark colors
    // and use white as the foreground color; otherwise use black.
    // Actually we use a threshold a bit below 50%, so colors like
    // #FF0000, #00FF00 and #0000FF still get black text which looked
    // better when we tested this.
    if (lightness < 120) {
        return "white";
    }
    
    return "black";
}

var gTransactionMgr = Components.classes["@mozilla.org/transactionmanager;1"]
                                .getService(Components.interfaces.nsITransactionManager);
function doTransaction(aAction, aItem, aCalendar, aOldItem, aListener) {
    var txn = new calTransaction(aAction, aItem, aCalendar, aOldItem, aListener);
    gTransactionMgr.doTransaction(txn);
    updateUndoRedoMenu();
}

function undo() {
    gTransactionMgr.undoTransaction();
    updateUndoRedoMenu();
}

function redo() {
    gTransactionMgr.redoTransaction();
    updateUndoRedoMenu();
}

function startBatchTransaction() {
    gTransactionMgr.beginBatch();
}
function endBatchTransaction() {
    gTransactionMgr.endBatch();
    updateUndoRedoMenu();
}

function canUndo() {
    return (gTransactionMgr.numberOfUndoItems > 0);
}
function canRedo() {
    return (gTransactionMgr.numberOfRedoItems > 0);
}

function updateUndoRedoMenu() {
    if (gTransactionMgr.numberOfUndoItems)
        document.getElementById('undo_command').removeAttribute('disabled');
    else    
        document.getElementById('undo_command').setAttribute('disabled', true);

    if (gTransactionMgr.numberOfRedoItems)
        document.getElementById('redo_command').removeAttribute('disabled');
    else    
        document.getElementById('redo_command').setAttribute('disabled', true);
}

// Valid values for aAction: 'add', 'modify', 'delete', 'move'
// aOldItem is only needed for aAction == 'modify'
function calTransaction(aAction, aItem, aCalendar, aOldItem, aListener) {
    this.mAction = aAction;
    this.mItem = aItem;
    this.mCalendar = aCalendar;
    this.mOldItem = aOldItem;
    this.mListener = aListener;
}

calTransaction.prototype = {
    mAction: null,
    mItem: null,
    mCalendar: null,
    mOldItem: null,
    mOldCalendar: null,
    mListener: null,

    QueryInterface: function (aIID) {
        if (!aIID.equals(Components.interfaces.nsISupports) &&
            !aIID.equals(Components.interfaces.nsITransaction))
        {
            throw Components.results.NS_ERROR_NO_INTERFACE;
        }
        return this;
    },

    doTransaction: function () {
        switch (this.mAction) {
            case 'add':
                this.mCalendar.addItem(this.mItem, this.mListener);
                break;
            case 'modify':
                this.mCalendar.modifyItem(this.mItem, this.mOldItem,
                                          this.mListener);
                break;
            case 'delete':
                this.mCalendar.deleteItem(this.mItem, this.mListener);
                break;
            case 'move':
                this.mOldCalendar = this.mOldItem.calendar;
                this.mCalendar.addItem(this.mItem, this.mListener);
                this.mOldCalendar.deleteItem(this.mOldItem, this.mListener);
                break;
        }
    },
    undoTransaction: function () {
        switch (this.mAction) {
            case 'add':
                this.mCalendar.deleteItem(this.mItem, null);
                break;
            case 'modify':
                this.mCalendar.modifyItem(this.mOldItem, this.mItem, null);
                break;
            case 'delete':
                this.mCalendar.addItem(this.mItem, null);
                break;
            case 'move':
                this.mOldCalendar.addItem(this.mOldItem, this.mListener);
                this.mCalendar.deleteItem(this.mItem, this.mListener);
                break;
        }
    },
    redoTransaction: function () {
        this.doTransaction();
    },
    isTransient: false,
    
    merge: function (aTransaction) {
        // No support for merging
        return false;
    }
}
