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
 * Contributor(s): Garth Smedley <garths@oeone.com>
 *                 Mike Potter <mikep@oeone.com>
 *                 Colin Phillips <colinp@oeone.com>
 *                 Karl Guertin <grayrest@grayrest.com> 
 *                 Mike Norton <xor@ivwnet.com>
 *                 ArentJan Banck <ajbanck@planet.nl> 
 *                 Eric Belhaire <belhaire@ief.u-psud.fr>
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
*        <script type="application/x-javascript" src="chrome://global/content/strres.js"/>
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

// Show event details on mouseover (sometimes this is false in the code)
var showTooltip = true;

//Show only the working days (changed in different menus)
var gOnlyWorkdayChecked ;

// DAY VIEW VARIABLES
var kDayViewHourLeftStart = 105;

var kWeekViewHourHeight = 50;
var kWeekViewHourHeightDifference = 2;
var kDaysInWeek = 7;

const kMAX_NUMBER_OF_DOTS_IN_MONTH_VIEW = "8"; //the maximum number of dots that fit in the month view


var prefService = Components.classes["@mozilla.org/preferences-service;1"]
                            .getService(Components.interfaces.nsIPrefService);
var rootPrefNode = prefService.getBranch(null); // preferences root node

/*-----------------------------------------------------------------
*  G L O B A L     C A L E N D A R      F U N C T I O N S
*/


/** 
* Called from calendar.xul window onload.
*/

function calendarInit() 
{
	// get the calendar event data source
   gEventSource = new CalendarEventDataSource();
   
   // get the Ical Library
   gICalLib = gEventSource.getICalLib();
   
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

   if( window.arguments && window.arguments[0].channel )
   {
      gCalendarWindow.calendarManager.checkCalendarURL( window.arguments[0].channel );
   }

   //a bit of a hack since the menulist doesn't remember the selected value     
   var value = document.getElementById( 'event-filter-menulist' ).value;
   document.getElementById( 'event-filter-menulist' ).selectedItem = document.getElementById( 'event-filter-'+value );
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

   gCalendarWindow.close();

   gICalLib.removeObserver( gEventSource.alarmObserver );
}

function launchPreferences()
{
   goPreferences( "calendarPanel", "chrome://calendar/content/pref/calendarPref.xul", "calendar" );
}


/** 
* Called to set up the date picker from the go to day button
*/

function prepareChooseDate()
{
   // the value attribute of the datePickerPopup is the initial date shown
   
   var datePickerPopup = document.getElementById( "oe-date-picker-popup" );   
   
   datePickerPopup.setAttribute( "value", gCalendarWindow.getSelectedDate() );
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

function weekEventItemClick( eventBox, event )
{
   //do this check, otherwise on double click you get into an infinite loop
   if( event.detail == 1 )
   {
      gCalendarWindow.EventSelection.replaceSelection( eventBox.calendarEventDisplay.event );

      var newDate = gCalendarWindow.getSelectedDate();

      newDate.setDate( eventBox.calendarEventDisplay.event.start.day );

      gCalendarWindow.setSelectedDate( newDate );
   }

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

function weekEventItemDoubleClick( eventBox, event )
{
   // we only care about button 0 (left click) events
   if (event.button != 0) return;
   
   editEvent( eventBox.calendarEventDisplay.event );

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

      newDate = new Date( gHeaderDateItemArray[dayIndex].getAttribute( "date" ) );

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
   var startDate;

   if( gNewDateVariable != null )
   {
      startDate = gNewDateVariable;
   }
   else
      startDate = gCalendarWindow.currentView.getNewEventDate();

   var Minutes = Math.ceil( startDate.getMinutes() / 5 ) * 5 ;
   
   startDate = new Date( startDate.getFullYear(),
                         startDate.getMonth(),
                         startDate.getDate(),
                         startDate.getHours(),
                         Minutes,
                         0);
   newEvent( startDate );
}


/** 
* Called when the new event button is clicked
*/

function newToDoCommand()
{
   var calendarToDo = createToDo();

   var dueDate = gCalendarWindow.currentView.getNewEventDate();
   
   
   var Minutes = Math.ceil( dueDate.getMinutes() / 5 ) * 5 ;
   
   dueDate = new Date( dueDate.getFullYear(),
                       dueDate.getMonth(),
                       dueDate.getDate(),
                       dueDate.getHours(),
                       Minutes,
                       0);
   

   calendarToDo.due.setTime( dueDate );

   calendarToDo.start.setTime( dueDate );
   
   var args = new Object();
   args.mode = "new";
   args.onOk =  self.addToDoDialogResponse;
   args.calendarToDo = calendarToDo;

   window.setCursor( "wait" );
   // open the dialog modally
   openDialog("chrome://calendar/content/toDoDialog.xul", "caEditEvent", "chrome,modal", args );
}


function createEvent ()
{
   var iCalEventComponent = Components.classes["@mozilla.org/icalevent;1"].createInstance();
   var iCalEvent = iCalEventComponent.QueryInterface(Components.interfaces.oeIICalEvent);
   return iCalEvent;
}


function createToDo ()
{
   var iCalToDoComponent = Components.classes["@mozilla.org/icaltodo;1"].createInstance();
   var iCalToDo = iCalToDoComponent.QueryInterface(Components.interfaces.oeIICalTodo);
   return iCalToDo;
}


function isEvent ( aObject )
{
   return aObject instanceof Components.interfaces.oeIICalEvent;
}


function isToDo ( aObject )
{
   return aObject instanceof Components.interfaces.oeIICalTodo;
}



/** 
* Helper function to launch the event composer to create a new event.
* When the user clicks OK "addEventDialogResponse" is called
*/

function newEvent( startDate, endDate )
{
   // create a new event to be edited and added
   var calendarEvent = createEvent();
   
   calendarEvent.start.setTime( startDate );
   
   if( !endDate )
   {
      var categoriesStringBundle = srGetStrBundle("chrome://calendar/locale/calendar.properties");
   
      var MinutesToAddOn = getIntPref(gCalendarWindow.calendarPreferences.calendarPref, "event.defaultlength", categoriesStringBundle.GetStringFromName("defaultEventLength" ) );
   
      var endDateTime = startDate.getTime() + ( 1000 * 60 * MinutesToAddOn );
   
      calendarEvent.end.setTime( endDateTime );
   }
   else
   {
      calendarEvent.end.setTime( endDate.getTime() );
   }
   
   //get the selected calendar
   var selectedCalendarItem = document.getElementById( "list-calendars-listbox" ).selectedItem;
   
   var server = null;

   if( selectedCalendarItem )
   {
      server = selectedCalendarItem.getAttribute( "calendarPath" );
   }
   
   editNewEvent( calendarEvent, server );
}


/**
* Helper function to launch the event composer to edit a new event.
* When the user clicks OK "addEventDialogResponse" is called
*/

function editNewEvent( calendarEvent, server )
{
   // set up a bunch of args to pass to the dialog

   var args = new Object();
   args.mode = "new";
   args.onOk =  self.addEventDialogResponse;
   args.calendarEvent = calendarEvent;
   
   if( server )
      args.server = server;

   window.setCursor( "wait" );
   // open the dialog modally
   openDialog("chrome://calendar/content/eventDialog.xul", "caEditEvent", "chrome,modal", args );
}


/** 
* Called when the user clicks OK in the new event dialog
*
* Update the data source, the unifinder views and the calendar views will be
* notified of the change through their respective observers
*/

function addEventDialogResponse( calendarEvent, Server )
{
   refreshRemoteCalendarAndRunFunction( calendarEvent, Server, "addEvent" );
}


/** 
* Called when the user clicks OK in the new to do item dialog
*
*/

function addToDoDialogResponse( calendarToDo, Server )
{
   gICalLib.addTodo( calendarToDo, Server );
}


/** 
* Helper function to launch the event composer to edit an event.
* When the user clicks OK "modifyEventDialogResponse" is called
*/

function editEvent( calendarEvent )
{
   // set up a bunch of args to pass to the dialog
   
   var args = new Object();
   args.mode = "edit";
   args.onOk = self.modifyEventDialogResponse;           
   args.calendarEvent = calendarEvent;

   // open the dialog modally
   
   window.setCursor( "wait" );
   openDialog("chrome://calendar/content/eventDialog.xul", "caEditEvent", "chrome,modal", args );
}
   

/** 
* Helper function to launch the event composer to edit an event.
* When the user clicks OK "modifyEventDialogResponse" is called
*/

function editToDo( calendarToDo )
{
   // set up a bunch of args to pass to the dialog
   
   var args = new Object();
   args.mode = "edit";
   args.onOk = self.modifyToDoDialogResponse;           
   args.calendarToDo = calendarToDo;
   
   window.setCursor( "wait" );
   // open the dialog modally
   openDialog("chrome://calendar/content/toDoDialog.xul", "caEditToDo", "chrome,modal", args );
}
   

/** 
* Called when the user clicks OK in the edit event dialog
*
* Update the data source, the unifinder views and the calendar views will be
* notified of the change through their respective observers
*/

function modifyEventDialogResponse( calendarEvent, Server )
{
   refreshRemoteCalendarAndRunFunction( calendarEvent, Server, "modifyEvent" );
}


/** 
* Called when the user clicks OK in the edit event dialog
*
* Update the data source, the unifinder views and the calendar views will be
* notified of the change through their respective observers
*/

function modifyToDoDialogResponse( calendarToDo, Server )
{
   gICalLib.modifyTodo( calendarToDo, Server );
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


function refreshRemoteCalendarAndRunFunction( calendarEvent, Server, functionToRun )
{
   var calendarServer = gCalendarWindow.calendarManager.getCalendarByName( Server )
   
   if( calendarServer )
   {
      if( calendarServer.getAttribute( "http://home.netscape.com/NC-rdf#publishAutomatically" ) == "true" )
      {
         var onResponseExtra = function( )
         {
            //add the event
            eval( "gICalLib."+functionToRun+"( calendarEvent, Server )" );
   
            gCalendarWindow.clearSelectedEvent( calendarEvent );
            
            //publish the changes back to the server
            if( calendarServer.getAttribute( "http://home.netscape.com/NC-rdf#publishAutomatically" ) == "true" )
            {
               gCalendarWindow.calendarManager.publishCalendar( calendarServer );
            }
         }
   
         //refresh the calendar file.
         gCalendarWindow.calendarManager.retrieveAndSaveRemoteCalendar( calendarServer, onResponseExtra );
      }
      else
      {
         eval( "gICalLib."+functionToRun+"( calendarEvent, Server )" );
   
         gCalendarWindow.clearSelectedEvent( calendarEvent );
      }
   }
   else
   {
      eval( "gICalLib."+functionToRun+"( calendarEvent, Server )" );
   
      gCalendarWindow.clearSelectedEvent( calendarEvent );
   }
}


/**
*  This is called from the unifinder's delete command
*/

function deleteEventCommand( DoNotConfirm )
{
   var SelectedItems = gCalendarWindow.EventSelection.selectedEvents;
   
   if( SelectedItems.length == 1 )
   {
      var calendarEvent = SelectedItems[0];

      if ( calendarEvent.title != "" ) {
         if( !DoNotConfirm ) {        
            if ( confirm( confirmDeleteEvent+" "+calendarEvent.title+"?" ) ) 
            {
               refreshRemoteCalendarAndRunFunction( calendarEvent.id, calendarEvent.parent.server, "deleteEvent" );
            }
         }
         else
         {
            refreshRemoteCalendarAndRunFunction( calendarEvent.id, calendarEvent.parent.server, "deleteEvent" );
            
            gCalendarWindow.clearSelectedEvent( calendarEvent );
         }
      }
      else
      {
         if( !DoNotConfirm ) {        
            if ( confirm( confirmDeleteUntitledEvent ) ) {
               refreshRemoteCalendarAndRunFunction( calendarEvent.id, calendarEvent.parent.server, "deleteEvent" );

               gCalendarWindow.clearSelectedEvent( calendarEvent );
            }
         }
         else
         {
            refreshRemoteCalendarAndRunFunction( calendarEvent.id, calendarEvent.parent.server, "deleteEvent" );

            gCalendarWindow.clearSelectedEvent( calendarEvent );
         }
      }
   }
   else if( SelectedItems.length > 1 )
   {
      var NumberOfEventsToDelete = SelectedItems.length;

      gICalLib.batchMode = true;
      
      var ThisItem;

      if( !DoNotConfirm )
      {
         if( confirm( "Are you sure you want to delete all selected events?" ) )
         {
            gCalendarWindow.clearSelectedEvent( calendarEvent );
            
            while( SelectedItems.length )
            {
               ThisItem = SelectedItems.pop();
               
               gICalLib.deleteEvent( ThisItem.id );
            }
         }
      }
      else
      {
         gCalendarWindow.clearSelectedEvent( calendarEvent );
            
         while( SelectedItems.length )
         {
            ThisItem = SelectedItems.pop();
            
            gICalLib.deleteEvent( ThisItem.id );
         }
      }

      gICalLib.batchMode = false;

      /*
      var NumberOfTotalEvents  = gCalendarWindow.calendarEvent.getAllEvents().length;

      if( NumberOfTotalEvents = NumberOfEventsToDelete )
      {
         //highlight today's date
         gCalendarWindow.currentView.hiliteTodaysDate();
      }
      */
   }
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

function displayCalendarVersion()
{
   window.openDialog( getBrowserURL(), "_blank", "chrome,all,dialog=no", 'chrome://calendar/content/about.html' );
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

   openDialog("chrome://calendar/content/wizard.xul", "caWizard", "chrome,modal", args );
}

/**
*  Called when a user hovers over a todo element and the text for the mouse over is changed.
*/

function getPreviewTextForTask( calendarEventDisplay )
{
  var toDoItem = calendarEventDisplay ;

   var HolderBox = document.createElement( "vbox" );

   if( toDoItem )
   {
      showTooltip = true; //needed to show the tooltip.
         
      if (toDoItem.title)
      {
         var TitleHtml = document.createElement( "description" );
         var TitleText = document.createTextNode( "Title: "+toDoItem.title );
         TitleHtml.appendChild( TitleText );
         HolderBox.appendChild( TitleHtml );
      }
   
      var DateHtml = document.createElement( "description" );
      var startDate = new Date( toDoItem.start.getTime() );
      var DateText = document.createTextNode( "Start Date: "+gCalendarWindow.dateFormater.getFormatedDate( startDate ) );
      DateHtml.appendChild( DateText );
      HolderBox.appendChild( DateHtml );
   
      DateHtml = document.createElement( "description" );
      var dueDate = new Date( toDoItem.due.getTime() );
      DateText = document.createTextNode( "Due Date: "+gCalendarWindow.dateFormater.getFormatedDate( dueDate ) );
      DateHtml.appendChild( DateText );
      HolderBox.appendChild( DateHtml );
   
      if (toDoItem.description)
      {
	var text = "Description: "+toDoItem.description ;
	var lines = text.split("\n");
	var nbmaxlines = 5 ;
	var nblines = lines.length ;
	if( nblines > nbmaxlines ) {
	  var nblines = nbmaxlines ;
	  lines[ nblines - 1 ] = "..." ;
	}
	
	for (var i = 0; i < nblines; i++) {
	  var DescriptionHtml = document.createElement("description");
	  var DescriptionText = document.createTextNode(lines[i]);
	  DescriptionHtml.appendChild(DescriptionText);
	  HolderBox.appendChild(DescriptionHtml);
	}
//          var DescriptionHtml = document.createElement( "description" );
//          var DescriptionText = document.createTextNode( "Description: "+toDoItem.description );
//          DescriptionHtml.appendChild( DescriptionText );
//          HolderBox.appendChild( DescriptionHtml );
      }
      
      return ( HolderBox );
   } 
   else
   {
      showTooltip = false; //Don't show the tooltip
   }
}

/**
*  Called when a user hovers over an element and the text for the mouse over is changed.
*/

function getPreviewTextForRepeatingEvent( calendarEventDisplay )
{
	showTooltip = true;
      
   var HolderBox = document.createElement( "vbox" );
    
   if (calendarEventDisplay.event.title)
   {
      var TitleHtml = document.createElement( "description" );
      var TitleText = "Title: "+calendarEventDisplay.event.title;
      
      /*
      if( calendarEventDisplay.event.recurUnits == "years" )
      {
         //count the number of years to figure out
         
         TitleText = TitleText+" "+getNumberOfRepeatTimes( calendarEventDisplay.event, false );
      }*/
      var TitleTextNode = document.createTextNode( TitleText );
      TitleHtml.appendChild( TitleTextNode );
      HolderBox.appendChild( TitleHtml );
   }

   var DateHtml = document.createElement( "description" );
   var startDate = new Date( calendarEventDisplay.displayDate );
   var DateText = document.createTextNode( "Start: "+gCalendarWindow.dateFormater.getFormatedDate( startDate )+" "+gCalendarWindow.dateFormater.getFormatedTime( startDate ) );
   DateHtml.appendChild( DateText );
   HolderBox.appendChild( DateHtml );

   DateHtml = document.createElement( "description" );
   startDate = new Date( calendarEventDisplay.displayEndDate );
   DateText = document.createTextNode( "End: "+gCalendarWindow.dateFormater.getFormatedDate( startDate )+" "+gCalendarWindow.dateFormater.getFormatedTime( startDate ) );
   DateHtml.appendChild( DateText );
   HolderBox.appendChild( DateHtml );
   

   if (calendarEventDisplay.event.description)
   {
     var Description =  "Description: "+calendarEventDisplay.event.description;
     var lines = Description.split("\n");
     var nbmaxlines = 5 ;
     var nblines = lines.length ;
     if( nblines > nbmaxlines ) {
       var nblines = nbmaxlines ;
       lines[ nblines - 1 ] = "..." ;
     }
  
     for (var i = 0; i < nblines; i++) {
       var DescriptionHtml = document.createElement("description");
       var DescriptionText = document.createTextNode(lines[i]);
       DescriptionHtml.appendChild(DescriptionText);
       HolderBox.appendChild(DescriptionHtml);
     }
   }

   return ( HolderBox );
}

function getNumberOfRepeatTimes( Event, DateToCompare )
{
   if( !DateToCompare )
   {
      DateToCompare = new Date();
   }
   
   var startDate = new Date( Event.start.getTime() );
   
   //get the difference in the number of years from now.
   var NumberOfYears = DateToCompare.getFullYear() - startDate.getFullYear();

   //find out if the event has happened this year or not.

   //add on the proper extension.
   
   return( NumberOfYears );
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

   var categoriesStringBundle = srGetStrBundle("chrome://calendar/locale/calendar.properties");
   var defaultWeekStart = categoriesStringBundle.GetStringFromName("defaultWeekStart" );
   var Offset = getIntPref(gCalendarWindow.calendarPreferences.calendarPref, "week.start", defaultWeekStart );
   var defaultWeeksInView = categoriesStringBundle.GetStringFromName("defaultWeeksInView" );
   var WeeksInView = getIntPref(gCalendarWindow.calendarPreferences.calendarPref, "weeks.inview", defaultWeeksInView );
   WeeksInView = ( WeeksInView >= 6 ) ? 6 : WeeksInView ;

   var defaultPreviousWeeksInView = categoriesStringBundle.GetStringFromName("defaultPreviousWeeksInView" );
   var PreviousWeeksInView = getIntPref(gCalendarWindow.calendarPreferences.calendarPref, "previousweeks.inview", defaultPreviousWeeksInView );
   PreviousWeeksInView = ( PreviousWeeksInView >= WeeksInView - 1 ) ? WeeksInView - 1 : PreviousWeeksInView ;

   args.startOfWeek=Offset;
   args.weeksInView=WeeksInView;
   args.prevWeeksInView=PreviousWeeksInView;

   window.openDialog("chrome://calendar/content/printDialog.xul","printdialog","chrome",args);
   
   //printEventArray( gCalendarWindow.EventSelection.selectedEvents, "chrome://calendar/content/converters/sortEvents.xsl" );
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
      publishObject.username = node.getAttribute( "http://home.netscape.com/NC-rdf#username" );
      
      publishObject.remotePath = remotePath;
      
      publishObject.password = node.getAttribute( "http://home.netscape.com/NC-rdf#password" );
      args.publishObject = publishObject;
   }
   
   openDialog("chrome://calendar/content/publishDialog.xul", "caPublishEvents", "chrome,modal", args );
}

function publishEntireCalendarDialogResponse( CalendarPublishObject )
{
   //update the calendar object with the publish information
   var name = gCalendarWindow.calendarManager.getSelectedCalendarId();
   
   //get the node
   var node = gCalendarWindow.calendarManager.rdf.getNode( name );
   
   node.setAttribute("http://home.netscape.com/NC-rdf#username", CalendarPublishObject.username);
   
   node.setAttribute("http://home.netscape.com/NC-rdf#password", CalendarPublishObject.password);
   
   node.setAttribute( "http://home.netscape.com/NC-rdf#remotePath", CalendarPublishObject.remotePath );
   
   node.setAttribute("http://home.netscape.com/NC-rdf#publishAutomatically", "false");

   gCalendarWindow.calendarManager.rdf.flush();
      
   calendarUploadFile(node.getAttribute( "http://home.netscape.com/NC-rdf#path" ), 
                      CalendarPublishObject.remotePath, 
                      CalendarPublishObject.username, 
                      CalendarPublishObject.password, 
                      "text/calendar");

   return( false );
}

function publishCalendarData()
{
   var args = new Object();
   
   args.onOk =  self.publishCalendarDataDialogResponse;
   
   openDialog("chrome://calendar/content/publishDialog.xul", "caPublishEvents", "chrome,modal", args );
}

function publishCalendarDataDialogResponse( CalendarPublishObject )
{
   var calendarString = eventArrayToICalString( gCalendarWindow.EventSelection.selectedEvents );
   
   calendarPublish(calendarString, CalendarPublishObject.remotePath, CalendarPublishObject.username, CalendarPublishObject.password, "text/calendar");
}

/*
** A little function to see if we can show the tooltip
*/
function checkTooltip( event )
{
   //returns true if you can show the tooltip
   //or false if the tooltip should not be shown
   
   return( showTooltip );
}


function getCharPref (prefObj, prefName, defaultValue)
{
    try
    {
        return prefObj.getCharPref (prefName);
    }
    catch (e)
    {
       prefObj.setCharPref( prefName, defaultValue );  
       return defaultValue;
    }
}

function getIntPref (prefObj, prefName, defaultValue)
{
    try
    {
        return prefObj.getIntPref (prefName);
    }
    catch (e)
    {
       prefObj.setIntPref( prefName, defaultValue );  
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
  switch(menuindex){
  case 1:
    var changemenu = 2 ;
    break;
  case 2:
    var changemenu = 1 ;
    break;
  default:
    return(false);
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
}

/* Change the display-todo-inview checkbox */
function changeDisplayToDoInViewCheckbox( menuindex ) {
  var check = document.getElementById( "display-todo-inview-checkbox-" + menuindex ).getAttribute("checked") ;
  switch(menuindex){
  case 1:
    var changemenu = 2 ;
    break;
  case 2:
    var changemenu = 1 ;
    break;
  default:
    return(false);
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
