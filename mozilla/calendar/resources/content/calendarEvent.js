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

/***** calendar/ca-calender-event.js
* AUTHOR
*   Garth Smedley
* REQUIRED INCLUDES 
*
* NOTES
*   CalendarEventDataSource class:
*      Saves and loads calendar events, provides methods
*      for adding, deleting, modifying and searching calendar events.
*
*   CalendarEvent class:
*      Contains info about calendar events.
*
*   Also provides an observer interface for clients that need to be 
*   notified when calendar event data changes.
*  
* IMPLEMENTATION NOTES 
*   Currently uses the Ical library to store events.
*   Access to ical is through the libxpical xpcom object.
*
**********
*/

/*-----------------------------------------------------------------
*  G L O B A L     V A R I A B L E S
*/

/*-----------------------------------------------------------------
*   CalendarEventDataSource Class
*
*  Maintains all of the calendar events.
*
* PROPERTIES
*  observerList   - array of observers, see constructor notes.
*/

/**
*   CalendarEventDataSource Constructor.
* 
* PARAMETERS
*      observer     - An object that implements methods that are called when the 
*                     data source is modified. The following methods should be implemented 
*                     by the observer object.
*
                     {
                        onLoad       : function() {},                // called when server is ready
                        onAddItem    : function( calendarEvent ) {}, 
                        onModifyItem : function( calendarEvent, originalEvent ) {},
                        onDeleteItem : function( calendarEvent ) {},
                        onAlarm      : function( calendarEvent ) {},
                     };
                     
                     These methods are now called synchronously, if you add an event the onAddItem
                     method will be called during your call to add the event.

*
* NOTES
*   Is a singleton, the first invocation creates an instance, subsequent calls return the same instance.
*/

function CalendarEventDataSource( observer, UserPath, syncPath )
{
      try {
          var iCalLibComponent = Components.classes["@mozilla.org/ical;1"].createInstance();
      } catch ( e ) {
          alert( "The ICAL Componenet is not registered properly. Please follow the instructions below:\n"+
				 "Windows Users:\n"+
				 "-Please quit Mozilla, run regxpcom and run Mozilla again.\n\n"+
				 "Linux users:\n"+
				 "1)Make sure you have write access to the component.reg file.\n"+
				 "2)Make sure libical is installed properly. (See http://www.mozilla.org/projects/calendar/ for detailed instructions)\n"+
				 "3)Quit Mozilla, cd to your mozilla/bin directory. Run ./regxpcom and run Mozilla again.\n"+
				 "Note: If you get this error:\n"+
				 "'./regxpcom: error while loading shared libraries: libxpcom.so: cannot open\n"+
				 "shared object file: No such file or directory',\n"+
				 "set LD_LIBRARY_PATH and MOZILLA_FIVE_HOME to your mozilla/bin directory first.\n"+
                 "Example: export LD_LIBRARY_PATH=/home/user/mozilla/dist/bin\n"+
				 "Example: export MOZILLA_FIVE_HOME=/home/user/mozilla/dist/bin\n\n"+
				 "If these instructions don't solve the problem, please add yourself to the cc list of\n"+
				 "bug 134432 at http://bugzilla.mozilla.org/show_bug.cgi?id=134432.\n"+
				 "and give more feedback on your platform, Mozilla version, calendar install type:\n"+
				 "(build or xpi), any errors you see on the console that you think is related and any\n"+
				 " other problems you come across when following the above insructions.\n" );
      }
            
      this.gICalLib = iCalLibComponent.QueryInterface(Components.interfaces.oeIICal);
        
      var profileComponent = Components.classes["@mozilla.org/profile/manager;1"].createInstance();
      
      var profileInternal = profileComponent.QueryInterface(Components.interfaces.nsIProfileInternal);
 
      var profileFile = profileInternal.getProfileDir(profileInternal.currentProfile);
      
      profileFile.append("CalendarDataFile.ics");
                      
      this.gICalLib.setServer( profileFile.path );

      this.gICalLib.addObserver( observer );
      
      this.prepareAlarms( );

      this.onlyFutureEvents = false;
}



CalendarEventDataSource.InitService = function( root )
{
    return new CalendarEventDataSource( null, root.getUserPath() );
}

// turn on/off debuging
CalendarEventDataSource.gDebug = true;

// Singleton CalendarEventDataSource variable.

CalendarEventDataSource.debug = function( str )
{
    if( CalendarEventDataSource.gDebug )
    {
        dump( "\n CalendarEventDataSource DEBUG: "+ str + "\n");
    }
}


/** PUBLIC
*
* NOTES
*   Called at start up after all services have been inited. 
*/

CalendarEventDataSource.prototype.onServiceStartup = function( root )
{
}



/** PUBLIC
*
*   CalendarEventDataSource/search.
*
*      First hacked in implementation of search
*
* NOTES
*   This should be search ing by all content, not just the beginning
*   of the title field, A LOT remains to be done.. 
*/

CalendarEventDataSource.prototype.search = function( searchText, fieldName )
{
   searchText = searchText.toLowerCase();
   
   /*
   ** Try to get rid of all the spaces in the search text.
   ** At present, this only gets rid of one.. I don't know why.
   */
   var regexp = "\s+";
   searchText = searchText.replace( regexp, "" );
   
   var searchEventTable = new Array();
   
   if( searchText != "" )
   {
      var eventTable = this.getCurrentEvents();   
      
      for( var index = 0; index < eventTable.length; ++index )
      {
         var calendarEvent = eventTable[ index ];
         
         if ( typeof fieldName == "string") 
         {
            var value = calendarEvent[ fieldName ].toLowerCase();
         
            if( value )
            {
               if( value.indexOf( searchText ) != -1 )
               {
                  searchEventTable[ searchEventTable.length ]  =  calendarEvent; 
                  break;
               }
            }
         }
         else if ( typeof fieldName == "object" ) 
         {
            for ( i in fieldName ) 
            {
               var objValue = calendarEvent[ fieldName[i] ].toLowerCase();
         
               if( objValue )
               {
                  if( objValue.indexOf( searchText ) != -1 )
                  {
                     searchEventTable[ searchEventTable.length ]  =  calendarEvent; 
                     break;
                  }
               }
            }
         }
      }
   }
   searchEventTable.sort( this.orderRawEventsByDate );

   return searchEventTable;
}

CalendarEventDataSource.prototype.searchBySql = function( Query )
{
   var eventDisplays = new Array();

   var eventList = this.gICalLib.searchBySQL( Query );

   while( eventList.hasMoreElements() )
   {
      eventDisplays[ eventDisplays.length ] = eventList.getNext().QueryInterface(Components.interfaces.oeIICalEvent);
   }

   eventDisplays.sort( this.orderRawEventsByDate );
   
   return eventDisplays;
}

/** PUBLIC
*
*   CalendarEventDataSource/getEventsForDay.
*
* PARAMETERS
*      date     - Date object, uses the month and year and date  to get all events 
*                 for the given day.
* RETURN
*      array    - of events for the day
*/


CalendarEventDataSource.prototype.getEventsForDay = function( date )
{
   var eventDisplays =  new Array();

   var displayDates =  new Object();
   
   var eventList = this.gICalLib.getEventsForDay( date, displayDates );
   
   while( eventList.hasMoreElements() )
   {
      var tmpevent = eventList.getNext().QueryInterface(Components.interfaces.oeIICalEvent);
      
      var displayDate = new Date( displayDates.value.getNext().QueryInterface(Components.interfaces.nsISupportsPRTime).data );
      
      var EventObject = new Object;
      
      EventObject.event = tmpevent;
      
      EventObject.displayDate = displayDate;
      
      eventDisplays[ eventDisplays.length ] = EventObject;
   }

   eventDisplays.sort( this.orderEventsByDate );

   return eventDisplays;
}


/** PUBLIC
*
*   CalendarEventDataSource/getEventsForWeek.
*
* PARAMETERS
*      date     - Date object, uses the month and year and date  to get all events 
*                 for the given day.
* RETURN
*      array    - of events for the day
*/


CalendarEventDataSource.prototype.getEventsForWeek = function( date )
{
   var eventDisplays =  new Array();

   var displayDates =  new Object();
   
   var eventList = this.gICalLib.getEventsForWeek( date, displayDates );
   
   while( eventList.hasMoreElements() )
   {
      var tmpevent = eventList.getNext().QueryInterface(Components.interfaces.oeIICalEvent);
      
      var displayDate = new Date( displayDates.value.getNext().QueryInterface(Components.interfaces.nsISupportsPRTime).data );
      
      var EventObject = new Object;
      
      EventObject.event = tmpevent;
      
      EventObject.displayDate = displayDate;
      
      eventDisplays[ eventDisplays.length ] = EventObject;
   }

   eventDisplays.sort( this.orderEventsByDate );

   return eventDisplays;
}

/** PUBLIC
*
*   CalendarEventDataSource/getEventsForMonth.
*
* PARAMETERS
*      date     - Date object, uses the month and year to get all events 
*                 for the given month.
* RETURN
*      array    - of events for the month
*/

CalendarEventDataSource.prototype.getEventsForMonth = function( date )
{
   var eventDisplays =  new Array();

   var displayDates =  new Object();
   
   var eventList = this.gICalLib.getEventsForMonth( date, displayDates );
   
   while( eventList.hasMoreElements() )
   {
      var tmpevent = eventList.getNext().QueryInterface(Components.interfaces.oeIICalEvent);
      
      var displayDate = new Date( displayDates.value.getNext().QueryInterface(Components.interfaces.nsISupportsPRTime).data );
      
      var EventObject = new Object;
      
      EventObject.event = tmpevent;
      
      EventObject.displayDate = displayDate;
      
      eventDisplays[ eventDisplays.length ] = EventObject;
   }

   eventDisplays.sort( this.orderEventsByDate );

   return eventDisplays;
}

/** PUBLIC
*
*   CalendarEventDataSource/getNextEvents.
*
* PARAMETERS
*      EventsToGet   - The number of events to return 
*
* RETURN
*      array    - of the next "EventsToGet" events
*/
CalendarEventDataSource.prototype.getNextEvents = function( EventsToGet )
{
   var eventDisplays =  new Array();
   
   var today = new Date();
   
   var displayDates =  new Object();
   
   var eventList = this.gICalLib.getNextNEvents( today, EventsToGet, displayDates );
   
   while( eventList.hasMoreElements() )
   {
      var tmpevent = eventList.getNext().QueryInterface(Components.interfaces.oeIICalEvent);
      
      var displayDate = new Date( displayDates.value.getNext().QueryInterface(Components.interfaces.nsISupportsPRTime).data );
      
      var EventObject = new Object;
      
      EventObject.event = tmpevent;
      
      EventObject.displayDate = displayDate;
      
      eventDisplays[ eventDisplays.length ] = EventObject;
   }
   eventDisplays.sort( this.orderRawEventsByDate );

   return eventDisplays;
}


CalendarEventDataSource.prototype.getCurrentEvents = function( )
{
   if( this.onlyFutureEvents == true )
   {
      return( this.getAllFutureEvents() );
   }
   else
   {
      return( this.getAllEvents() );
   }
      
}

/** PUBLIC
*
*   CalendarEventDataSource/getAllEvents.
*
* RETURN
*      array    - of ALL events
*/

CalendarEventDataSource.prototype.getAllEvents = function( )
{
   // clone the array in case the caller messes with it
   
   var eventList = this.gICalLib.getAllEvents();

   var eventArray = new Array();
   
   while( eventList.hasMoreElements() )
   {
      var tmpevent = eventList.getNext().QueryInterface(Components.interfaces.oeIICalEvent);
      
      eventArray[ eventArray.length ] = tmpevent;
   }
   eventArray.sort( this.orderRawEventsByDate );

   return eventArray;
}


CalendarEventDataSource.prototype.getEventsWithAlarms = function()
{
   //return( this.searchBySql( "SELECT * FROM VEVENT WHERE VALARM.DTSTART" ) );
}


CalendarEventDataSource.prototype.getAllFutureEvents = function()
{
   var Today = new Date();
   
   var Infinity = new Date( 9999, 31, 12 );

   var eventList = this.gICalLib.getFirstEventsForRange( Today, Infinity );
   
   var eventArray = new Array();
   
   while( eventList.hasMoreElements() )
   {
      var tmpevent = eventList.getNext().QueryInterface(Components.interfaces.oeIICalEvent);
      
      eventArray[ eventArray.length ] = tmpevent;
   }
   eventArray.sort( this.orderRawEventsByDate );

   return eventArray;
}

CalendarEventDataSource.prototype.getICalLib = function()
{
   return this.gICalLib;
}

/** PACKAGE STATIC
*   CalendarEvent orderEventsByDate.
* 
* NOTES
*   Used to sort table by date
*/

CalendarEventDataSource.prototype.orderEventsByDate = function( eventA, eventB )
{
    /*
    return( eventA.event.start.getTime() - eventB.event.start.getTime() );
    */
    return( eventA.displayDate.getTime() - eventB.displayDate.getTime() );
}


/** PACKAGE STATIC
*   CalendarEvent orderRawEventsByDate.
* 
* NOTES
*   Used to sort table by date
*/

CalendarEventDataSource.prototype.orderRawEventsByDate = function( eventA, eventB )
{
    return( getNextOrPreviousRecurrence( eventA ).getTime() - getNextOrPreviousRecurrence( eventB ).getTime() );
}

function getNextOrPreviousRecurrence( calendarEvent )
{
   if( calendarEvent.recur )
   {
      var now = new Date();

      var result = new Object();

      var isValid = calendarEvent.getNextRecurrence( now.getTime(), result );

      if( isValid )
      {
         var eventStartDate = new Date( result.value );
      }
      else
      {
         isValid = calendarEvent.getPreviousOccurrence( now.getTime(), result );
         
         var eventStartDate = new Date( result.value );
      }
   }
   
   if( !isValid || !calendarEvent.recur )
   {
      var eventStartDate = new Date( calendarEvent.start.getTime() );
   }
      
   return eventStartDate;
}

/******************************************************************************************************
******************************************************************************************************

ALARM RELATED CODE

******************************************************************************************************
*******************************************************************************************************/
CalendarEventDataSource.prototype.prepareAlarms = function( )
{
    this.alarmObserver =  new CalendarAlarmObserver( this );
    
    this.gICalLib.addObserver( this.alarmObserver );
    
    this.alarmObserver.firePendingAlarms( this.alarmObserver );
}

function CalendarAlarmObserver( calendarService )
{
    this.pendingAlarmList = new Array();
    this.addToPending = true;
    this.calendarService = calendarService;
}

CalendarAlarmObserver.prototype.firePendingAlarms = function( observer )
{
    this.addToPending = false;
    
    for( var i in this.pendingAlarmList )
    {
        this.fireAlarm( this.pendingAlarmList[ i ] );
        
        observer.onAlarm( this.pendingAlarmList[ i ] ); 
    }
    
    this.pendingAlarmList = null;
    
    
}

CalendarAlarmObserver.prototype.onStartBatch = function()
{
}

CalendarAlarmObserver.prototype.onEndBatch = function()
{
}

CalendarAlarmObserver.prototype.onLoad = function( calendarEvent )
{
}

CalendarAlarmObserver.prototype.onAddItem = function( calendarEvent )
{
}

CalendarAlarmObserver.prototype.onModifyItem = function( calendarEvent, originalEvent )
{
}

CalendarAlarmObserver.prototype.onDeleteItem = function( calendarEvent )
{
}


CalendarAlarmObserver.prototype.onAlarm = function( calendarEvent )
{
    debug( "caEvent.alarmWentOff is "+ calendarEvent );
    
    if( this.addToPending )
    {
        debug( "defering alarm "+ calendarEvent );
        this.pendingAlarmList.push( calendarEvent );
    }
    else
    {
        this.fireAlarm( calendarEvent )
    }
}

CalendarAlarmObserver.prototype.fireAlarm = function( calendarEvent )
{
   debug( "Fire alarm "+ calendarEvent );
   
   if( gCalendarWindow.calendarPreferences.getPref( "alarmsplaysound" ) )
   {
      playSound();
   }
   
   addEventToDialog(  calendarEvent );

   if ( calendarEvent.alarmEmailAddress )
   {
      //send an email for the event
      // TO DO
   }
}


function debug( str )
{
    dump( "\n CalendarEvent.js DEBUG: "+ str + "\n");
}

