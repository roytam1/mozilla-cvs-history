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

// style sheet number for calendar
var gCalendarStyleSheet;

//an array of indexes to boxes for the week view
var gHeaderDateItemArray = null;

//Show only the working days (changed in different menus)
var gOnlyWorkdayChecked ;
// ShowToDoInView
var gDisplayToDoInViewChecked ;

// DAY VIEW VARIABLES
var kDayViewHourLeftStart = 105;

var kWeekViewHourHeight = 50;
var kWeekViewHourHeightDifference = 2;
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

      var catergoryPrefBranch = prefService.getBranch("calendar.category.color.");
      var prefCount = { value: 0 };
      var prefArray = catergoryPrefBranch.getChildList("", prefCount);
      for (i = 0; i < prefArray.length; ++i) {
         var prefName = prefArray[i];
         var prefValue = catergoryPrefBranch.getCharPref(prefName);
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

    gCalendar = createCalendar();

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
   
   //updateColors();
}

function updateColors()
{
   // Change made by CofC for Calendar Coloring
   // initialize calendar color style rules in the calendar's styleSheet

   // find calendar's style sheet index
   var i;
   for (i=0; i<document.styleSheets.length; i++)
   {
      if (document.styleSheets[i].href.match(/chrome.*\/skin.*\/calendar.css$/))
      {
          gCalendarStyleSheet = document.styleSheets[i];
          break;
      }
   }

   var calendarNode;
   var containerName;
   var calendarColor;

   // loop through the calendars via the rootSequence of the RDF datasource
   var seq = gCalendarWindow.calendarManager.rdf.getRootSeq("urn:calendarcontainer");
   var list = seq.getSubNodes();
   var calListItems = document.getElementById( "list-calendars-listbox" ).getElementsByTagName("listitem");

   for(i=0; i<list.length;i++)
   {

     calendarNode = gCalendarWindow.calendarManager.rdf.getNode( list[i].subject );
     
     // grab the container name and use it for the name of the style rule
     containerName = list[i].subject.split(":")[2];

     // obtain calendar color from the rdf datasource
     calendarColor = calendarNode.getAttribute("http://home.netscape.com/NC-rdf#color");

     // if the calendar had a color attribute create a style sheet for it
     if (calendarColor != null)
     {
       gCalendarStyleSheet.insertRule("." + containerName + " { background-color:" + calendarColor + "!important;}", 1);

       var calcColor = calendarColor.replace(/#/g, "");
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
       // Actually we use a treshold a bit below 50%, so colors like
       // #FF0000, #00FF00 and #0000FF still get black text which looked
       // better when we tested this.
       if (lightness < 120) {
         gCalendarStyleSheet.insertRule("." + containerName + " { color:" + " white" + "!important;}", 1);
       } else {
         gCalendarStyleSheet.insertRule("." + containerName + " { color:" + " black" + "!important;}", 1);
       }
       var calListItem = calListItems[i+1];
       if (calListItem && calListItem.childNodes[0]) {
         calListItem.childNodes[0].setAttribute("class", "calendar-list-item-class " + containerName);
       }
     }
   }

   // Setup css classes for category colors
   var catergoryPrefBranch = prefService.getBranch("");
   var pbi = catergoryPrefBranch.QueryInterface(Components.interfaces.nsIPrefBranchInternal);
   pbi.addObserver("calendar.category.color.", categoryPrefObserver, false);
   categoryPrefObserver.mCalendarStyleSheet = gCalendarStyleSheet;
   categoryPrefObserver.observe(null, null, "");

   if( ("arguments" in window) &&
       (window.arguments.length) &&
       (typeof(window.arguments[0]) == "object") &&
       ("channel" in window.arguments[0]) )
   {
      gCalendarWindow.calendarManager.checkCalendarURL( window.arguments[0].channel );
   }

   //a bit of a hack since the menulist doesn't remember the selected value     
   var value = document.getElementById( 'event-filter-menulist' ).value;
   document.getElementById( 'event-filter-menulist' ).selectedItem = document.getElementById( 'event-filter-'+value );

   gEventSource.alarmObserver.firePendingAlarms();

   //All is settled, enable feedbacks to observers
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
   pbi = pbi.QueryInterface(Components.interfaces.nsIPrefBranchInternal);
   pbi.removeObserver("calendar.category.color.", categoryPrefObserver);

   gCalendarWindow.close();

   gICalLib.removeObserver( gEventSource.alarmObserver );
}

function launchPreferences()
{
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
      gCalendarWindow.EventSelection.replaceSelection( eventBox.calendarEventDisplay.event );
   
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
   
   editEvent( eventBox.calendarEventDisplay.event );

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
        var calEvent = eventBox.calEvent;

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
   
   editEvent( eventBox.calEvent );

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
      gCalendarWindow.EventSelection.replaceSelection( eventBox.calendarEventDisplay.event );
      
      var newDate = gCalendarWindow.getSelectedDate();

      newDate.setDate( eventBox.calendarEventDisplay.event.start.day );

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
   
   editEvent( eventBox.calendarEventDisplay.event );

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
   
   editToDo( todoBox.calendarToDo );

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


function createEvent()
{
    return Components.classes["@mozilla.org/calendar/event;1"].createInstance(Components.interfaces.calIEvent);
}

function createToDo()
{
    return Components.classes["@mozilla.org/calendar/todo;1"].createInstance(Components.interfaces.calITodo);
}

function createRecurrenceInfo()
{
    return Components.classes["@mozilla.org/calendar/recurrence-info;1"].createInstance(Components.interfaces.calIRecurrenceInfo);
}

function createDateTime()
{
    return Components.classes["@mozilla.org/calendar/datetime;1"].createInstance(Components.interfaces.calIDateTime);
}

function createAttendee()
{
    return Components.classes["@mozilla.org/calendar/attendee;1"].createInstance(Components.interfaces.calIAttendee);
}

function createCalendar()
{
    var prefobj = prefService.getBranch("calendar.");
    var caltype = getCharPref(prefobj, "default-calendar.type", "memory");
    var calendar = Components.classes["@mozilla.org/calendar/calendar;1?type=" + caltype].getService(Components.interfaces.calICalendar);
    if (calendar.uri || caltype == "memory")
        return calendar;
        
    var ioservice = Components.classes["@mozilla.org/network/io-service;1"].getService(Components.interfaces.nsIIOService);

    var uri = null;
    if (caltype == "caldav") {
        var uriString = getCharPref(prefobj, "default-calendar.uri", null);
        uri = ioservice.newURI(uriString, null, null);
    } else if (caltype == "storage") {
        var pathString = getCharPref(prefobj, "default-calendar.path", null);
        var dbFile = Components.classes["@mozilla.org/file/local;1"].createInstance(Components.interfaces.nsILocalFile);
        dbFile.initWithPath(pathString);
        uri = ioservice.newFileURI(dbFile);
    }

    if (!uri) {
        throw "Calendar type " + caltype + 
            " requires that default-calendar.uri pref be set!";
    }
    calendar.uri = uri;
    return calendar;
}

function isEvent(aObject)
{
   return aObject instanceof Components.interfaces.calIEvent;
}


function isToDo(aObject)
{
   return aObject instanceof Components.interfaces.calITodo;
}


function jsDateToDateTime(date)
{
    newDate = createDateTime();
    newDate.jsDate = date;
    return newDate;
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
       
       var endDate = new Date(startDate);
       endDate.setMinutes(endDate.getMinutes() + MinutesToAddOn);
   }

   calendarEvent.endDate.jsDate = endDate

   if (allDay)
       calendarEvent.isAllDay = true;

   var server = getSelectedCalendarPathOrNull();

   editNewEvent( calendarEvent, server );
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
        calendarToDo.start.jsDate = startDate;

    if (dueDate)
        calendarToDo.dueDate.jsDate = dueDate;

    var server = getSelectedCalendarPathOrNull();
    
    editNewToDo(calendarToDo, server);
}

function getSelectedCalendarPathOrNull()
{
   //get the selected calendar
   var selectedCalendarItem = document.getElementById( "list-calendars-listbox" ).selectedItem;
   
   if ( selectedCalendarItem )
     return selectedCalendarItem.getAttribute( "calendarPath" );
   else
     return null;
}

/**
* Launch the event dialog to edit a new (created, imported, or pasted) event.
* 'server' is calendarPath.
* When the user clicks OK "addEventDialogResponse" is called
*/

function editNewEvent( calendarEvent, server )
{
  openEventDialog(calendarEvent,
                  "new",
                  self.addEventDialogResponse,
                  server);
}

/**
* Launch the todo dialog to edit a new (created, imported, or pasted) ToDo.
* 'server' is calendarPath.
* When the user clicks OK "addToDoDialogResponse" is called
*/
function editNewToDo( calendarToDo, server )
{
  openToDoDialog(calendarToDo,
                 "new",
                 self.addToDoDialogResponse,
                 server);
}

/** 
* Called when the user clicks OK in the new event dialog
*
* Update the data source, the unifinder views and the calendar views will be
* notified of the change through their respective observers
*/

function addEventDialogResponse( calendarEvent, Server )
{
   saveItem( calendarEvent, Server, "addEvent" );
}


/** 
* Called when the user clicks OK in the new to do item dialog
*
*/

function addToDoDialogResponse( calendarToDo, Server )
{
    addEventDialogResponse(calendarToDo, Server);
}


/** 
* Helper function to launch the event composer to edit an event.
* When the user clicks OK "modifyEventDialogResponse" is called
*/

function editEvent( calendarEvent )
{
  openEventDialog(calendarEvent,
                  "edit",
                  self.modifyEventDialogResponse,
                  null);
}
   
/** 
* Helper function to launch the event composer to edit an event.
* When the user clicks OK "modifyEventDialogResponse" is called
*/

function editToDo( calendarToDo )
{
  openToDoDialog(calendarToDo,
                 "edit",
                 self.modifyToDoDialogResponse,
                 null);
}
   
/** 
* Called when the user clicks OK in the edit event dialog
*
* Update the data source, the unifinder views and the calendar views will be
* notified of the change through their respective observers
*/

function modifyEventDialogResponse( calendarEvent, Server, originalEvent )
{
   saveItem( calendarEvent, Server, "modifyEvent", originalEvent );
}


/** 
* Called when the user clicks OK in the edit event dialog
*
* Update the data source, the unifinder views and the calendar views will be
* notified of the change through their respective observers
*/

function modifyToDoDialogResponse( calendarToDo, Server, originalToDo )
{
    modifyEventDialogResponse(calendarToDo, Server, originalToDo);
}


/** PRIVATE: open event dialog in mode, and call onOk if ok is clicked.
    'mode' is "new" or "edit".
    'server' is path to calendar to update.
 **/
function openEventDialog(calendarEvent, mode, onOk, server)
{
  // set up a bunch of args to pass to the dialog
  var args = new Object();
  args.calendarEvent = calendarEvent;
  args.mode = mode;
  args.onOk = onOk;

  if( server )
    args.server = server;

  // wait cursor will revert to auto in eventDialog.js loadCalendarEventDialog
  window.setCursor( "wait" );
  // open the dialog modally
  openDialog("chrome://calendar/content/eventDialog.xul", "caEditEvent", "chrome,titlebar,modal", args );
}

/** PRIVATE: open todo dialog in mode, and call onOk if ok is clicked.
    'mode' is "new" or "edit".
    'server' is path to calendar to update.
 **/
function openToDoDialog(calendarToDo, mode, onOk, server)
{
  // set up a bunch of args to pass to the dialog
  var args = new Object();
  args.calendarEvent = calendarToDo;
  args.mode = mode;
  args.onOk = onOk;
  
  if( server )
    args.server = server;
   
  // wait cursor will revert to auto in todoDialog.js loadCalendarEventDialog
  window.setCursor( "wait" );
  // open the dialog modally
  openDialog("chrome://calendar/content/toDoDialog.xul", "caEditToDo", "chrome,titlebar,modal", args );
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
         editEvent( calendarEvent );
      }
   }
}


//originalEvent is the item before edits were committed, 
//used to check if there were external changes for shared calendar
function saveItem( calendarEvent, Server, functionToRun, originalEvent )
{
    dump(functionToRun + " " + calendarEvent.title + "\n");

    if (functionToRun == 'addEvent')
        gCalendar.addItem(calendarEvent, null);

    else if (functionToRun == 'modifyEvent')
        gCalendar.modifyItem(calendarEvent, null);





    /*
   var calendarServer = gCalendarWindow.calendarManager.getCalendarByName( Server );
   var path = calendarServer.getAttribute("http://home.netscape.com/NC-rdf#path");
   var shared = (calendarServer.getAttribute("http://home.netscape.com/NC-rdf#shared" ) == "true");
   var publishAutomatically = (calendarServer.getAttribute( "http://home.netscape.com/NC-rdf#publishAutomatically" ) == "true");
   if( calendarServer ) {
   
      if( publishAutomatically ) {
         var onResponseExtra = function( ) {
            //add the event
            eval( "gICalLib."+functionToRun+"( calendarEvent, Server )" );
            gCalendarWindow.clearSelectedEvent( calendarEvent );
            //publish the changes back to the server
            gCalendarWindow.calendarManager.publishCalendar( calendarServer );
         }
         //refresh the calendar file.
         gCalendarWindow.calendarManager.retrieveAndSaveRemoteCalendar( calendarServer, onResponseExtra );
            
      } else if ( shared ) {
         
         if ( !gCalendarWindow.calendarManager.startLocalLock(calendarServer)) {
            alert(gCalendarBundle.getString( "unableToWrite" ) + path + ".lock");
            return;
         }
         
         //Do not override external changes to other events, reload all calendar and merge only modified event... 
         //check if the same event was edited externally  before edits were committed since the last reload.
         if (gCalendarWindow.calendarManager.reloadCalendar(calendarServer, true)) {
            //calendar edited externally
            var uneditedEvent = fetchItem(calendarEvent);
            if (uneditedEvent != null) {
               //not a new event
               if (!compareItems( uneditedEvent, originalEvent )) {
                  //event edited externally
                  alert(gCalendarBundle.getString("concurrentEdit"));
                  return;
               }
            }
         }
         
         //Merge single edited event and save
         eval( "gICalLib." + functionToRun + "(calendarEvent, path)" );
         gCalendarWindow.calendarManager.removeLocalLock(calendarServer);
         
         //Check if the edited event is actually in the calendar file in case the lock failed
         gCalendarWindow.calendarManager.reloadCalendar( calendarServer );
         if ( !compareItems( fetchItem( calendarEvent ), calendarEvent ) ){ 
            alert(gCalendarBundle.getString( "unableToWrite" ) + path );  
         }
         gCalendarWindow.clearSelectedEvent( calendarEvent );
         
      } else {
         //Normal local calendar
         eval("gICalLib."+functionToRun+"(calendarEvent, path)");
         gCalendarWindow.clearSelectedEvent( calendarEvent );
      }

   } else {
      eval( "gICalLib."+functionToRun+"( calendarEvent, Server )" );
      gCalendarWindow.clearSelectedEvent( calendarEvent );
   }

    */
}


/**
*  This is called from the unifinder's delete command
*
*/
function deleteItems( SelectedItems, DoNotConfirm )
{
    if (!SelectedItems)
        return;

    //Confirmation
    if (!DoNotConfirm) {
        var calendarEvent = SelectedItems[0];
        var confirmText;
        if (SelectedItems.length > 1) {
            confirmText = confirmDeleteAllEvents;
        } else if (calendarEvent.title) {
            confirmText = (confirmDeleteEvent + " " + calendarEvent.title + "?");
        } else {
            confirmText = confirmDeleteUntitledEvent;
        }
        if (!confirm(confirmText)) {
            return;
        }
    }

    ccalendar = createCalendar();

    for (i in  SelectedItems) {
        ccalendar.deleteItem(SelectedItems[i], null);
    }

   /*

   //group items to delete by calendarServer
   calendarsToPublish = new Array();
   var autoPublishEnabled = false;
   var serverInArray = false;
   for( i = 0; i < SelectedItems.length; i++ ) {
      var calendarServer = gCalendarWindow.calendarManager.getCalendarByName( SelectedItems[i].parent.server );
      var calendarId = calendarServer.getAttribute( "http://home.netscape.com/NC-rdf#serverNumber" );
      if (!(calendarId in calendarsToPublish)) {
         calendarsToPublish[calendarId] = new Object();
         calendarsToPublish[calendarId].items =new Array();
      }
      calendarsToPublish[calendarId].calendarServer = calendarServer;
      calendarsToPublish[calendarId].items.push(SelectedItems[i]);
   }
   
   //process each calendarServer
   for (calendarId in calendarsToPublish) {
      calendarServer = calendarsToPublish[calendarId].calendarServer;
      var path = calendarServer.getAttribute("http://home.netscape.com/NC-rdf#path");
      var shared = (calendarServer.getAttribute("http://home.netscape.com/NC-rdf#shared" )  == "true");
      var publishAutomatically = (calendarServer.getAttribute( "http://home.netscape.com/NC-rdf#publishAutomatically" ) == "true");
      
      //pre-processing
      if( publishAutomatically ) {
         //download
         gCalendarWindow.calendarManager.retrieveAndSaveRemoteCalendar(calendarServer);
      } else if ( shared ) {
         //lock
         if ( !gCalendarWindow.calendarManager.startLocalLock(calendarServer)){
            alert(gCalendarBundle.getString( "unableToWrite" ) + path + ".lock");
            continue; 
            //skip deleting items from this calendar, try other calendars 
         }
         //reload 
         var isModified = gCalendarWindow.calendarManager.reloadCalendar(calendarServer, true); 
      }
         
      //delete selected items of this calendarServer locally 
      gICalLib.batchMode = true;
      for (eventId in calendarsToPublish[calendarId].items) {
         var selectedItem = calendarsToPublish[calendarId].items[eventId]; //item to delte

         //item-level checks
         if ( shared ) {
            //Check if the same event was edited externally since the last reload.
            if ( isModified ) {
               //calendar edited externally
               var uneditedEvent = fetchItem(selectedItem );
               if (uneditedEvent != null ){
                  //not already deleted
                  if (!compareItems( uneditedEvent, selectedItem )){
                     //event edited externally
                     alert(gCalendarBundle.getString("concurrentEdit") + " " + selectedItem.title);
                     continue; 
                     //skip deleting this item
                  }
               }
            }
         }

         //delete item
         try {
            if( isToDo(selectedItem) ) {
               gICalLib.deleteTodo( selectedItem.id );
            }else{
               gICalLib.deleteEvent( selectedItem.id );
            }
         } catch (ex) {
            dump("*** deleteItems failed: "+ ex + "\n");
         }
      }
      gICalLib.batchMode = false;

      //post-processing 
      if ( publishAutomatically) {
         //publish
         gCalendarWindow.calendarManager.publishCalendar(calendarServer);
      } else if ( shared) {
         //unlock
         gCalendarWindow.calendarManager.removeLocalLock(calendarServer);
      }
   }
   */
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
   if( numRanges == 1 ) {
      for (t=numRanges-1; t>= 0; t--) {
         tree.view.selection.getRangeAt(t, start, end);
         for (v=end.value; v>=start.value; v--){
            var toDoItem = tree.taskView.getCalendarTaskAtRow( v );
            SelectedItems.push(toDoItem);
         }
      }
   } else {
      for (t=numRanges; t >= 0; t--) {
         tree.view.selection.getRangeAt(t,start,end);
         for (v=end.value; v >= start.value; v--){
            var toDoItem=tree.taskView.getCalendarTaskAtRow( v );
            SelectedItems.push(toDoItem);
         }
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

function playSound( ThisURL )
{
   ThisURL = "chrome://calendar/content/sound.wav";

   var url = Components.classes["@mozilla.org/network/standard-url;1"].createInstance();
   url = url.QueryInterface(Components.interfaces.nsIURL);
   url.spec = ThisURL;

   var sample = Components.classes["@mozilla.org/sound;1"].createInstance();
   
   sample = sample.QueryInterface(Components.interfaces.nsISound);

   try
   {
      sample.play( url );
   }
   catch ( ex )
   {
      sample.beep();
      //alert( ex );
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


function launchWizard()
{
   var args = new Object();

   openDialog("chrome://calendar/content/wizard.xul", "caWizard", "chrome,titlebar,modal", args );
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
      var xslDocument = domParser.parseFromString(xslContent, 'text/xml');

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
   var name = gCalendarWindow.calendarManager.getSelectedCalendarId();
   var node = gCalendarWindow.calendarManager.rdf.getNode( name );

   var remotePath = node.getAttribute( "http://home.netscape.com/NC-rdf#remotePath" );
   
   if( remotePath != "" && remotePath != null )
   {
      var publishObject = new Object( );
      publishObject.remotePath = remotePath;
      args.publishObject = publishObject;
   }
   
   openDialog("chrome://calendar/content/publishDialog.xul", "caPublishEvents", "chrome,titlebar,modal", args );
}

function publishEntireCalendarDialogResponse( CalendarPublishObject )
{
   //update the calendar object with the publish information
   var name = gCalendarWindow.calendarManager.getSelectedCalendarId();
   
   //get the node
   var node = gCalendarWindow.calendarManager.rdf.getNode( name );
   
   node.setAttribute( "http://home.netscape.com/NC-rdf#remotePath", CalendarPublishObject.remotePath );
   
    if( node.getAttribute("http://home.netscape.com/NC-rdf#publishAutomatically") != "true" )
        node.setAttribute("http://home.netscape.com/NC-rdf#publishAutomatically", "false");

   gCalendarWindow.calendarManager.rdf.flush();
      
   calendarUploadFile(node.getAttribute( "http://home.netscape.com/NC-rdf#path" ), 
                      CalendarPublishObject.remotePath, 
                      "text/calendar");

   return( false );
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
