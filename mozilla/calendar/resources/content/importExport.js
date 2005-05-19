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
 * The Original Code is Mozilla Calendar code.
 *
 * The Initial Developer of the Original Code is
 * ArentJan Banck <ajbanck@planet.nl>.
 * Portions created by the Initial Developer are Copyright (C) 2002
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s): ArentJan Banck <ajbanck@planet.nl>
 *                 Steve Hampton <mvgrad78@yahoo.com>
 *                 Eric Belhaire <belhaire@ief.u-psud.fr>
 *                 Jussi Kukkonen <jussi.kukkonen@welho.com>
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
 

 
/**** calendarImportExport
 * Unit with functions to convert calendar events to and from different formats.
 *
 Requires dateUtils.js
    <script type="application/x-javascript"
     src="chrome://calendar/content/dateUtils.js"/>
 ****/

// XSL stylesheet directory
var convertersDirectory = "chrome://calendar/content/converters/";

// File constants copied from file-utils.js
const MODE_RDONLY   = 0x01;
const MODE_WRONLY   = 0x02;
const MODE_RDWR     = 0x04;
const MODE_CREATE   = 0x08;
const MODE_APPEND   = 0x10;
const MODE_TRUNCATE = 0x20;
const MODE_SYNC     = 0x40;
const MODE_EXCL     = 0x80;

const filterCalendar    = gCalendarBundle.getString( "filterCalendar" );
const extensionCalendar = ".ics";
const filtervCalendar    = gCalendarBundle.getString( "filtervCalendar" );
const extensionvCalendar = ".vcs";
const filterXcs         = gCalendarBundle.getString("filterXcs");
const extensionXcs      = ".xcs";
const filterXml         = gCalendarBundle.getString("filterXml");
const extensionXml      = ".xml";
const filterRtf         = gCalendarBundle.getString("filterRtf");
const extensionRtf      = ".rtf";
const filterHtml        = gCalendarBundle.getString("filterHtml");
const extensionHtml     = ".html";
const filterCsv         = gCalendarBundle.getString("filterCsv");
const filterOutlookCsv  = gCalendarBundle.getString("filterOutlookCsv");
const extensionCsv      = ".csv";
const filterRdf         = gCalendarBundle.getString("filterRdf");
const extensionRdf      = ".rdf";

if( opener && "gICalLib" in opener && opener.gICalLib )
   gICalLib = opener.gICalLib;

// convert to and from Unicode for file i/o
function convertFromUnicode( aCharset, aSrc )
{
   // http://lxr.mozilla.org/mozilla/source/intl/uconv/idl/nsIScriptableUConv.idl
   var unicodeConverter = Components.classes["@mozilla.org/intl/scriptableunicodeconverter"].createInstance(Components.interfaces.nsIScriptableUnicodeConverter);
   unicodeConverter.charset = aCharset;
   return unicodeConverter.ConvertFromUnicode( aSrc );
}

function convertToUnicode(aCharset, aSrc )
{
   // http://lxr.mozilla.org/mozilla/source/intl/uconv/idl/nsIScriptableUConv.idl
   var unicodeConverter = Components.classes["@mozilla.org/intl/scriptableunicodeconverter"].createInstance(Components.interfaces.nsIScriptableUnicodeConverter);
   unicodeConverter.charset = aCharset;
   return unicodeConverter.ConvertToUnicode( aSrc );
}

/**** loadEventsFromFile
 * shows a file dialog, reads the selected file(s) and tries to parse events from it.
 */

function loadEventsFromFile()
{
    const nsIFilePicker = Components.interfaces.nsIFilePicker;
  
    var fp = Components.classes["@mozilla.org/filepicker;1"].createInstance(nsIFilePicker);
    fp.init(window, gCalendarBundle.getString("Open"), nsIFilePicker.modeOpenMultiple);
    fp.defaultExtension = "ics";

    fp.appendFilter( filterCalendar, "*" + extensionCalendar );
    fp.appendFilter( filterXcs, "*" + extensionXcs );
    fp.appendFilter( filterOutlookCsv, "*" + extensionCsv );
    fp.appendFilter( filtervCalendar, "*" + extensionvCalendar );
    fp.show();
    var filesToAppend = fp.files;

    if (filesToAppend && filesToAppend.hasMoreElements()) {
        var calendarEventArray = new Array();
        var duplicateEventArray = new Array();
        var calendarToDoArray = new Array();
        var duplicateToDoArray = new Array();
        var currentFile;
        var aDataStream;
        var i;
        var parsedEventArray = null, parsedToDoArray = null;
        var date = new Date();

        var promptService = Components.classes["@mozilla.org/embedcomp/prompt-service;1"].getService(); 
        promptService = promptService.QueryInterface(Components.interfaces.nsIPromptService); 
        var flags = ( promptService.BUTTON_TITLE_IS_STRING * promptService.BUTTON_POS_0 ) + 
            ( promptService.BUTTON_TITLE_IS_STRING * promptService.BUTTON_POS_1 ) + 
            ( promptService.BUTTON_TITLE_IS_STRING * promptService.BUTTON_POS_2 );
        var intoCalName = getSelectedCalendarNameOrDefault();
        var importAllStr = gCalendarBundle.getString( "importAll" );
        var promptStr = gCalendarBundle.getString( "promptForEach" );
        var discardAllStr = gCalendarBundle.getString( "discardAll" );
        var importNewEventsTitle = gCalendarBundle.getString( "aboutToImportNewEventsTitle" );
        var importDupEventsTitle = gCalendarBundle.getString( "aboutToImportDupEventsTitle" );
        var importNewTasksTitle = gCalendarBundle.getString( "aboutToImportNewTasksTitle" );
        var importDupTasksTitle = gCalendarBundle.getString( "aboutToImportDupTasksTitle" );
        var fromFileNames = "";

        // create a temp memory calendar to put all the events and todos in to
        var tmpCalendar = getCalendarManager().createCalendar("memory", null);

        while (filesToAppend.hasMoreElements()) {
            currentFile = filesToAppend.getNext().QueryInterface(Components.interfaces.nsILocalFile);
            fromFileNames += (fromFileNames == "" ? "" : ", ") + currentFile.leafName;
            aDataStream = readDataFromFile( currentFile.path, "UTF-8" );

            switch (fp.filterIndex) {
            case 1 : // xcs: transform data into ics data
                aDataStream = transformXCSData( aDataStream );
                // fall thru to process ics data
            case 0 : // ics
            case 3 : // vcs
                parseIcalEvents(tmpCalendar, aDataStream);
                parseIcalToDos(tmpCalendar, aDataStream);
                break;
            case 2: // csv
                parseOutlookCSVEvents(tmpCalendar, aDataStream);
                break;
            default:
                break;
            }
        }

        var listener = {
            onOperationComplete: function(aCalendar, aStatus, aOperationType, aId, aDetail)
            {
                // delete the new calendar now that we're done with it
                getCalendarManager().deleteCalendar(tmpCalendar);
            }
        };


        appendCalendars(getDefaultCalendar(), [tmpCalendar], listener);

        return true;

        // XXX handle all this stuff

        var result = {value:0}; 
        var buttonPressed;

        // EVENTS:
        if (calendarEventArray.length > 0) {
            // Ask user what to import (all / prompt each / none)
            var importNewEventsText = gCalendarBundle.getFormattedString( "aboutToImportNewEvents",
                                                                          [calendarEventArray.length,
                                                                           intoCalName, fromFileNames]);
            buttonPressed = promptService.confirmEx( window, importNewEventsTitle, importNewEventsText,
                                                     flags, importAllStr, discardAllStr, promptStr,
                                                     null, result );
        
            if(buttonPressed == 0) // Import all
                addEventsToCalendar( calendarEventArray, true );
            else if(buttonPressed == 2) // prompt
                addEventsToCalendar( calendarEventArray );
            //else if(buttonPressed == 1) // discard all
        }
      
        if (duplicateEventArray.length > 0) {
            // Ask user what to do with duplicates
            var importDupEventsText = gCalendarBundle.getFormattedString( "aboutToImportDupEvents",
                                                                          [duplicateEventArray.length,
                                                                           intoCalName, fromFileNames]);
            buttonPressed = promptService.confirmEx( window, importDupEventsTitle, importDupEventsText, flags,
                                                     importAllStr, discardAllStr, promptStr,
                                                     null, result );
            if(buttonPressed == 0) // Import all
                addEventsToCalendar( duplicateEventArray, true );
            else if(buttonPressed == 2) // Prompt for each
                addEventsToCalendar( duplicateEventArray ); 
            //else if(buttonPressed == 1) // Discard all
        }
        
        // TODOS
        if (calendarToDoArray.length > 0) {
            // Ask user what to import (all / prompt each / none)
            var importNewTasksText = gCalendarBundle.getFormattedString( "aboutToImportNewTasks", [calendarToDoArray.length, intoCalName, fromFileNames]);
            buttonPressed = promptService.confirmEx( window, importNewTasksTitle, importNewTasksText, flags,
                                                     importAllStr, discardAllStr, promptStr,
                                                     null, result );
            
            if(buttonPressed == 0) // Import all
                addToDosToCalendar( calendarToDoArray, true );
            else if(buttonPressed == 2) // prompt
                addToDosToCalendar( calendarToDoArray );
            //else if(buttonPressed == 1) // discard all
        }
        
        // Ask user what to do with duplicates
        if (duplicateToDoArray.length > 0) {
            var importDupTasksText = gCalendarBundle.getFormattedString( "aboutToImportDupTasks", [duplicateToDoArray.length, intoCalName, fromFileNames]);
            buttonPressed = promptService.confirmEx( window, importDupTasksTitle, importDupTasksText, flags,
                                                     importAllStr, discardAllStr, promptStr,
                                                     null, result );
            if(buttonPressed == 0) // Import all
                addToDosToCalendar( duplicateToDoArray, true );
            else if(buttonPressed == 2) // Prompt for each
                addToDosToCalendar( duplicateToDoArray ); 
            //else if(buttonPressed == 1) // Discard all
        }

        // If there were no events or todos to import, let the user know
        //
        if (calendarEventArray.length == 0 && duplicateEventArray.length == 0 &&
            calendarToDoArray.length == 0 && duplicateToDoArray.length == 0)
            alert( gCalendarBundle.getFormattedString( "noEventsOrTasksToImport", [fromFileNames] ) );
    }
    return true;
}


/**** createUniqueID
 *
 * Creates a new unique ID. Format copied from the oeICalImpl.cpp AddEvent function
 */

function createUniqueID()
{
   var newID = "";
   while( (newID == "") || (gICalLib.fetchEvent( newID ) != null) )
     newID = Math.round(900000000 + (Math.random() * 100000000));
   return newID;
}


/**** 
 * calendarEventArray: array of calendar event objects.
 * silent: If silent, adds them all to selected (or default) calendar.
 *   else shows new event dialog on each event, using selected (or default)
 *   calendar as the initial calendar in dialog.
 * calendarPath (optional): if present, overrides selected calendar.
 *   Value is calendarPath from another item in calendar list.
 */
function addEventsToCalendar( calendarEventArray, silent, calendarPath )
{
  if( ! calendarPath ) // null, "", or false
  {
    calendarPath = getSelectedCalendarPathOrDefault();
  }

  gICalLib.batchMode = true;
  try
  { 

    for(var i = 0; i < calendarEventArray.length; i++)
    {
      calendarEvent = calendarEventArray[i];

      // Check if event with same ID already in Calendar. If so, import event with new ID.
      if( gICalLib.fetchEvent( calendarEvent.id ) != null )
      {
        calendarEvent.id = createUniqueID( );
      }

      // the start time is in zulu time, need to convert to current time
      if(calendarEvent.allDay != true)
      {
        convertZuluToLocalEvent( calendarEvent );
      }

      if( silent )
      {
        // LINAGORA (We need to see the new added event in the window and to update remote cal)
        addEventDialogResponse( calendarEvent, calendarPath );
        /* gICalLib.addEvent( calendarEvent, calendarPath ); */
      }
      else
      {
        // open the event dialog with the event to add, calls addEventDialogResponse on OK.
        editNewEvent( calendarEvent, calendarPath );
      }
    }
  } 
  finally
  {
    gICalLib.batchMode = false;
  }
}

/**** 
 * calendarToDoArray: array of calendar toDo objects.
 * silent: If silent, adds them all to selected (or default) calendar.
 *   else shows new toDo dialog on each toDo, using selected (or default)
 *   calendar as the initial calendar in dialog.
 * calendarPath (optional): if present, overrides selected calendar.
 *   Value is calendarPath from another item in calendar list.
 */
function addToDosToCalendar( calendarToDoArray, silent, calendarPath )
{
  if( ! calendarPath ) // null, "", or false
  {
    calendarPath = getSelectedCalendarPathOrDefault();
  }

  gICalLib.batchMode = true;
  try
  { 

    for(var i = 0; i < calendarToDoArray.length; i++)
    {
      var calendarToDo = calendarToDoArray[i];

      // Check if toDo with same ID already in Calendar. If so, import toDo with new ID.
      if( gICalLib.fetchTodo( calendarToDo.id ) != null )
      {
        calendarToDo.id = createUniqueID( );
      }

      // the start time is in zulu time, need to convert to current time
      convertZuluToLocalToDo( calendarToDo );

      if( silent ) 
      {
        // LINAGORA (We need to see the new added toDo in the window and to update remote cal)
        addToDoDialogResponse( calendarToDo, calendarPath );
        /* gICalLib.addToDo( calendarToDo, calendarPath ); */
      }
      else
      {
        // open the toDo dialog with the toDo to add, calls addToDoDialogResponse on OK.
        editNewToDo( calendarToDo, calendarPath );
      }
    }
  } 
  finally
  {
    gICalLib.batchMode = false;
  }
}

/** Return the calendarPath of the calendar selected in list-calendars-listbox,
    or the default calendarPath if none selected. **/
function getSelectedCalendarPathOrDefault()
{
  var calendarPath = null;
  //see if there's a server selected in the calendar window first
  //get the selected calendar
  if( document.getElementById( "list-calendars-listbox" ) )
  {
    var selectedCalendarItem = document.getElementById( "list-calendars-listbox" ).selectedItem;
    if( selectedCalendarItem )
    {
      calendarPath = selectedCalendarItem.getAttribute( "calendarPath" );
    }
  }
  if( ! calendarPath ) // null, "", or false
  {
    calendarPath = gCalendarWindow.calendarManager.getDefaultServer();
  }
  return calendarPath;
}


/** Return the calendarPath of the calendar selected in list-calendars-listbox,
    or the default calendarPath if none selected. **/
function getSelectedCalendarNameOrDefault()
{
  var calendarName = null;
  //see if there's a server selected in the calendar window first
  //get the selected calendar
  if( document.getElementById( "list-calendars-listbox" ) )
  {
    var selectedCalendarItem = document.getElementById( "list-calendars-listbox" ).selectedItem;
    if( selectedCalendarItem )
    {
      var listCell = selectedCalendarItem.firstChild;
      if ( listCell ) 
        calendarName = listCell.getAttribute( "label" );
    }
  }
  if( ! calendarName ) // null, "", or false
  {
    calendarName = gCalendarWindow.calendarManager.getDefaultCalendarName();
  }
  return calendarName;
}


/** oeDateTime is an oeDateTime object, not a javascript date **/
function convertZuluToLocalOEDateTime( oeDateTime )
{
  if (oeDateTime.utc == true)
  {
    // At zulu (utc) time, compute offset from zulu time to local time.
    // Offset depends on datetime because of daylight-time/summer-time changes.
    var zuluMillis = oeDateTime.getTime();
    var offsetMillisAtZuluTime = new Date(zuluMillis).getTimezoneOffset() * 60 * 1000;
    oeDateTime.setTime(oeDateTime.getTime() - offsetMillisAtZuluTime);
    oeDateTime.utc = false;
  }
}
/** oeDateTime is an oeDateTime object, not a javascript date **/
function convertLocalToZuluOEDateTime( oeDateTime )
{
  if (oeDateTime.utc == false)
  {
    // At local time, compute offset from zulu time to local time.
    // Offset depends on datetime because of daylight-time/summer-time changes.
    var localJSDate = new Date(oeDateTime.year,
                               oeDateTime.month,
                               oeDateTime.day,
                               oeDateTime.hour,
                               oeDateTime.minute);
    var offsetMillisAtLocalTime = localJSDate.getTimezoneOffset() * 60 * 1000;
    oeDateTime.setTime(oeDateTime.getTime() + offsetMillisAtLocalTime);
    oeDateTime.utc = true;
  }
}

function convertZuluToLocalEvent( calendarEvent )
{
  convertZuluToLocalOEDateTime(calendarEvent.start);
  convertZuluToLocalOEDateTime(calendarEvent.end);
}

function convertLocalToZuluEvent( calendarEvent )
{
  convertLocalToZuluOEDateTime(calendarEvent.start);
  convertLocalToZuluOEDateTime(calendarEvent.end);
}

function convertZuluToLocalToDo( calendarToDo )
{
  convertZuluToLocalOEDateTime(calendarToDo.start);
  convertZuluToLocalOEDateTime(calendarToDo.due);
}

function convertLocalToZuluToDo( calendarToDO )
{
  convertLocalToZuluOEDateTime(calendarToDo.start);
  convertLocalToZuluOEDateTime(calendarToDo.due);
}

/** 
* Initialize an event with a start and end date.
*/

function initCalendarEvent( calendarEvent )
{
   var startDate = gCalendarWindow.currentView.getNewEventDate();

   var Minutes = Math.ceil( startDate.getMinutes() / 5 ) * 5 ;

   startDate = new Date( startDate.getFullYear(),
                         startDate.getMonth(),
                         startDate.getDate(),
                         startDate.getHours(),
                         Minutes,
                         0);

   calendarEvent.start.setTime( startDate );
   
   var MinutesToAddOn = getIntPref(gCalendarWindow.calendarPreferences.calendarPref, "event.defaultlength", 60 );

   var endDateTime = startDate.getTime() + ( 1000 * 60 * MinutesToAddOn );

   calendarEvent.end.setTime( endDateTime );
}

function initCalendarToDo( calendarToDo )
{
   var startDate = gCalendarWindow.currentView.getNewEventDate();

   var Minutes = Math.ceil( startDate.getMinutes() / 5 ) * 5 ;

   startDate = new Date( startDate.getFullYear(),
                         startDate.getMonth(),
                         startDate.getDate(),
                         startDate.getHours(),
                         Minutes,
                         0);

   calendarToDo.start.setTime( startDate );
   
   var MinutesToAddOn = getIntPref(gCalendarWindow.calendarPreferences.calendarPref, "event.defaultlength", 60 );

   var endDateTime = startDate.getTime() + ( 1000 * 60 * MinutesToAddOn );

   calendarToDo.due.setTime( endDateTime );
}


function datesAreEqual(icalDate, date) {

  if (
      icalDate.month  == date.getMonth() &&
      icalDate.day    == date.getDate()  &&
      icalDate.year   == date.getFullYear() &&
      icalDate.hour   == date.getHours() &&
      icalDate.minute == date.getMinutes())
    return true;
  else
    return false;
}

function eventExists( date, subject) {

  var events = gEventSource.getEventsForDay( date );

  var ret = false;

  if (events.length == 0)
    return false;

  for (var i = 0; i < events.length; i++) {

    var event = events[i].event;

    if ( event.title == subject && datesAreEqual(event.start, date)) {
      ret = true;
      break;
    }
  }

  return ret;
}

function toDoExists( dueDate, subject) {

  var date = new Date(dueDate.getFullYear(), dueDate.getMonth(), dueDate.getDate());
  var toDos = gEventSource.getToDosForRange( date, date );

  if (toDos.length == 0)
    return false;

  for (var i = 0; i < toDos.length; i++) {

    var toDo = toDos[i];

    if ( toDo.title == subject && datesAreEqual(toDo.due, dueDate))
      return true;
  }

  return false;
}

function promptToKeepEntry(title, startTime, endTime) 
{
  return confirm(
                 gCalendarBundle.getString( "addDuplicate" )+"\n\n" + 
                 gCalendarBundle.getString( "eventTitle" )+ title + "\n" +
                 gCalendarBundle.getString( "eventStartTime" )+ startTime.toString() + "\n" +
                 gCalendarBundle.getString( "eventEndTime" )+ endTime.toString() + "\n" 
                 );

}

/**** parseOutlookCSVEvents
 *
 * Takes a text block of Outlook-exported Comma Separated Values and tries to 
 * parse that into individual events.  
 * 
 * First line is field names, all quoted with double quotes.  Field names are
 * locale dependendent.  In English the recognized field names are:
 *   "Title","Start Date","Start Time","End Date","End Time","All day event",
 *   "Reminder on/off","Reminder Date","Reminder Time","Categories",
 *   "Description","Location","Private"
 * Not all fields are necessary.  If some fields do not match known field names,
 * a dialog is presented to the user to match fields.
 * 
 * The rest of the lines are events, one event per line, with fields in the
 * order descibed by the first line.   All non-empty values must be quoted.
 * 
 * Returns: an array of parsed calendarEvents.  
 *   If the parse is cancelled, a zero length array is returned.
 */ 
function parseOutlookCSVEvents( outlookCsvStr ) {

  parse: { 
    // parse header line of quoted comma separated column names.
    var trimEndQuotesRegExp = /^"(.*)"$/m;
    var trimResults = trimEndQuotesRegExp.exec( outlookCsvStr );
    var header = trimResults && trimResults[1].split(/","/);
    if( header == null )
      break parse;
  
    //strip header from string
    outlookCsvStr = outlookCsvStr.slice(trimResults[0].length);
      
    var args = new Object();
    //args.cancelled is about window cancel, not about event
    args.cancelled = false;
    //args.fieldList contains the field names from the first row of CSV
    args.fieldList = header; 
      
    // set indexes if Outlook language happened to be same as locale
    const outlookCSVTitle       = gCalendarBundle.getString("outlookCSVTitle");
    const outlookCSVStartDate   = gCalendarBundle.getString("outlookCSVStartDate");
    const outlookCSVStartTime   = gCalendarBundle.getString("outlookCSVStartTime");
    const outlookCSVEndDate     = gCalendarBundle.getString("outlookCSVEndDate");
    const outlookCSVEndTime     = gCalendarBundle.getString("outlookCSVEndTime");
    const outlookCSVAllDayEvent = gCalendarBundle.getString("outlookCSVAllDayEvent");
    const outlookCSVAlarm       = gCalendarBundle.getString("outlookCSVAlarm");
    const outlookCSVAlarmDate   = gCalendarBundle.getString("outlookCSVAlarmDate");
    const outlookCSVAlarmTime   = gCalendarBundle.getString("outlookCSVAlarmTime");
    const outlookCSVCategories  = gCalendarBundle.getString("outlookCSVCategories");
    const outlookCSVDescription = gCalendarBundle.getString("outlookCSVDescription");
    const outlookCSVLocation    = gCalendarBundle.getString("outlookCSVLocation");
    const outlookCSVPrivate     = gCalendarBundle.getString("outlookCSVPrivate");
    const outlookCSVValueTrue   = gCalendarBundle.getString("outlookCSVValueTrue");
    const outlookCSVValueFalse  = gCalendarBundle.getString("outlookCSVValueFalse");

    var knownIndxs = 0;
    for( var i = 1; i <= header.length; i++) {
      switch( header[i-1] ) {
        case outlookCSVTitle:        args.titleIndex = i;       knownIndxs++; break;
        case outlookCSVStartDate:    args.startDateIndex = i;   knownIndxs++; break;
        case outlookCSVStartTime:    args.startTimeIndex = i;   knownIndxs++; break;
        case outlookCSVEndDate:      args.endDateIndex = i;     knownIndxs++; break;
        case outlookCSVEndTime:      args.endTimeIndex = i;     knownIndxs++; break;
        case outlookCSVAllDayEvent:  args.allDayIndex = i;      knownIndxs++; break;
        case outlookCSVAlarm:        args.alarmIndex = i;       knownIndxs++; break;
        case outlookCSVAlarmDate:    args.alarmDateIndex = i;   knownIndxs++; break;
        case outlookCSVAlarmTime:    args.alarmTimeIndex = i;   knownIndxs++; break;
        case outlookCSVCategories:   args.categoriesIndex = i;  knownIndxs++; break;
        case outlookCSVDescription:  args.descriptionIndex = i; knownIndxs++; break;
        case outlookCSVLocation:     args.locationIndex = i;    knownIndxs++; break;
        case outlookCSVPrivate:      args.privateIndex = i;     knownIndxs++; break;
      }
    }

    if (knownIndxs == 0 && header.length == 22) {
      // set default indexes for a default Outlook2000 CSV file
      args.titleIndex = 1;
      args.startDateIndex = 2;
      args.startTimeIndex = 3;
      args.endDateIndex = 4;
      args.endTimeIndex = 5;
      args.allDayIndex = 6;
      args.alarmIndex = 7;
      args.alarmDateIndex = 8;
      args.alarmTimeIndex = 9;
      args.categoriesIndex = 15;
      args.descriptionIndex = 16;
      args.locationIndex = 17;
      args.privateIndex = 20;
    }  

    // show field select dialog if not all headers matched
    if(knownIndxs != header.length) {
      window.setCursor( "wait" );
      openDialog( "chrome://calendar/content/outlookImportDialog.xul", "caOutlookImport", "chrome,modal,resizable=yes", args );
      if( args.cancelled )
        break parse;
    }
      
    // Construct event regexp according to field indexes. The regexp can
    // be made stricter, if it seems this matches too loosely.
    var regExpStr = "^";
    for( i = 1; i <= header.length; i++ ) {
      if( i > 1 ) regExpStr += ",";
      regExpStr += "(?:\"((?:[^\"]|\"\")*)\")?"; 
    }
    regExpStr += "$";
        
    // eventRegExp: regexp for reading events (this one'll be constructed on fly)
    const eventRegExp = new RegExp( regExpStr, "gm" );

    // match first line
    var eventFields = eventRegExp( outlookCsvStr );

    if( eventFields == null )
      break parse;
            
    // if boolean field used, find boolean value based on selected indexes
    if (args.allDayIndex || args.alarmIndex || args.privateIndex) { 
      // get a sample boolean value from any boolean column of the first event.
      // again, if imported language is same as locale...
      args.boolStr = ( eventFields[args.allDayIndex] ||
                       eventFields[args.alarmIndex] ||
                       eventFields[args.privateIndex] );

      if (args.boolStr) {  // if not all empty string, test for true/false
        if(     args.boolStr.toLowerCase() == outlookCSVValueTrue.toLowerCase())
          args.boolIsTrue = true;
        else if(args.boolStr.toLowerCase() == outlookCSVValueFalse.toLowerCase())
          args.boolIsTrue = false;
        else {
          window.setCursor( "wait" );
          openDialog( "chrome://calendar/content/outlookImportBooleanDialog.xul", "caOutlookImport", "chrome,modal,resizable=yes", args );
          if( args.cancelled )
            break parse;
        }
      } else {             // else field is empty string, treat it as false
        args.boolIsTrue = false;
      }
    } else { // no boolean columns, just set default
      args.boolStr = outlookCSVValueTrue;
      args.boolIsTrue = true; 
    }

    var dateParseConfirmed = false;
    var eventArray = new Array();
    const dateFormat = new DateFormater();
    do {
      // At this point eventFields contains following fields. Position
      // of fields is in args.[fieldname]Index.
      //    subject, start date, start time, end date, end time,
      //    all day?, alarm?, alarm date, alarm time,
      //    Description, Categories, Location, Private?
      // Unused fields (could maybe be copied to Description):
      //    Meeting Organizer, Required Attendees, Optional Attendees,
      //    Meeting Resources, Billing Information, Mileage, Priority,
      //    Sensitivity, Show time as
   
      //parseShortDate magically decides the format (locale) of dates/times
      var title = ("titleIndex" in args
                   ? parseOutlookTextField(args, "titleIndex", eventFields) : "");
      var sDate = parseOutlookDateTimeFields(args, "startDateIndex", "startTimeIndex",
                                             eventFields, dateFormat);
      var eDate = parseOutlookDateTimeFields(args, "endDateIndex", "endTimeIndex",
                                             eventFields, dateFormat);
      var alarmDate = parseOutlookDateTimeFields(args, "alarmDateIndex", "alarmTimeIndex",
                                                 eventFields, dateFormat);
      if ( title || sDate ) {
              
        if (!dateParseConfirmed) { 
          // Check if parsing with current date format is acceptable by
          // checking that each parsed date formats to same string.  This is
          // inside event loop in case the first event's dates are ambiguous.
          // For example, 2/3/2004 works with both D/M/YYYY or M/D/YYYY so it
          // is ambiguous, but 20/3/2004 only works with D/M/YYYY, and would
          // produce 8/3/2005 with M/D/YYYY.
          var startDateString = ("startDateIndex" in args? eventFields[args.startDateIndex] : "");
          var endDateString   = ("endDateIndex"   in args? eventFields[args.endDateIndex]   : "");
          var alarmDateString = ("alarmDateIndex" in args? eventFields[args.alarmDateIndex] : "");
          var dateIndexes = ["startDateIndex", "endDateIndex", "alarmDateIndex"];
          var parsedDates = [sDate,            eDate,          alarmDate];
          for (var j = 0; j < dateIndexes.length; j++) {
            var indexName = dateIndexes[j];
            var dateString = (indexName in args? eventFields[args[indexName]] : "");
            var parsedDate = parsedDates[j];
            var formattedDate = null;
            if (dateString &&
                (parsedDate == null ||
                 dateString != (formattedDate = dateFormat.getShortFormatedDate(parsedDate)))) {
              // A difference found, or an unparseable date found, so ask user to confirm date format.
              //"A date in this file is formatted as "%1$S".\n\
              // The operating system is set to parse this date as "%2$S".\n\
              // Is this OK?\n\
              // (If not, adjust settings so format matches, then restart this application.)"
              const outlookCSVDateParseConfirm =
                gCalendarBundle.getFormattedString("outlookCSVDateParseConfirm",
                                                   [startDateString, formattedDate]);
              dateParseConfirmed = confirm(outlookCSVDateParseConfirm);
              if (! dateParseConfirmed)
                break parse; // parsed date not acceptable, abort parse.
              else
                break; // parsed date format acceptable, no need to check more dates.
            }
          }
        }
        var calendarEvent = createEvent();
              
        calendarEvent.id = createUniqueID();
        calendarEvent.title = title;

        if ("allDayIndex" in args)
          calendarEvent.allDay       = (args.boolStr == eventFields[args.allDayIndex]
                                        ? args.boolIsTrue : !args.boolIsTrue);
        if ("alarmIndex" in args)
          calendarEvent.alarm        = (args.boolStr == eventFields[args.alarmIndex]
                                        ? args.boolIsTrue : !args.boolIsTrue);
        if ("privateIndex" in args)
          calendarEvent.privateEvent = (args.boolStr == eventFields[args.privateIndex]
                                        ? args.boolIsTrue : !args.boolIsTrue);

        if (!eDate && sDate) {
          eDate = new Date(sDate);
          if (calendarEvent.allDay)
            // end date is exclusive, so set to next day after start.
            eDate.setDate(eDate.getDate() + 1);
        }
        if (sDate) 
          calendarEvent.start.setTime( sDate );
        if (eDate) 
          calendarEvent.end.setTime( eDate );
        if (alarmDate) {
          var len, units;
          var minutes = Math.round( ( sDate - alarmDate ) / kDate_MillisecondsInMinute );
          var hours = Math.round(minutes / 60 );
          if (minutes != hours*60) {
            len = minutes;
            units = "minutes";
          } else {
            var days = Math.round(hours / 24);
            if (hours != days * 24) {
              len = hours;
              units = "hours";
            } else {
              len = days;
              units = "days";
            }
          }
          calendarEvent.alarmLength = len;
          calendarEvent.alarmUnits = units;
          calendarEvent.setParameter( "ICAL_RELATED_PARAMETER", "ICAL_RELATED_START" );
        }
        if ("descriptionIndex" in args)
          calendarEvent.description = parseOutlookTextField(args, "descriptionIndex", eventFields);
        if ("categoriesIndex" in args)
          calendarEvent.categories  = parseOutlookTextField(args, "categoriesIndex", eventFields);
        if ("locationIndex" in args)
          calendarEvent.location    = parseOutlookTextField(args, "locationIndex", eventFields);
              
        //save the event into return array
        eventArray[eventArray.length] = calendarEvent;
      }

      //get next events fields
      eventFields = eventRegExp( outlookCsvStr );

    } while( eventRegExp.lastIndex !=0 );

    // return results
    return eventArray;

  } // end parse
  return new Array(); // parse cancelled, return empty array
}

/** PRIVATE **/
function parseOutlookDateTimeFields(args, dateIndexName, timeIndexName, eventFields, dateFormat)
{
  var startDateString = (dateIndexName in args? eventFields[args[dateIndexName]] : "");
  var startTimeString = (timeIndexName in args? eventFields[args[timeIndexName]] : "");
  if (startDateString)
    if (startTimeString)
      return dateFormat.parseShortDate( startDateString + " " + startTimeString );
    else
      return dateFormat.parseShortDate( startDateString );
  else
    if (startTimeString)
      return dateFormat.parseTimeOfDay( startTimeString );
    else
      return null;
}
/** PRIVATE **/
function parseOutlookTextField(args, textIndexName, eventFields)
{
  var textString = (textIndexName in args? eventFields[args[textIndexName]] : "");
  if (textString)
    return textString.replace(/""/g, "\"");
  else
    return textString; // null or empty
}

/**** parseIcalEvents
 *
 * Takes a text block of iCalendar events and tries to split that into individual events.
 * Parses those events and adds them to a calendar.
 */

function parseIcalEvents(calendar, icalStr)
{
    for (var i=0, j=0; (i = icalStr.indexOf("BEGIN:VEVENT", j)) != -1; ) {
        // try to find the begin and end of an event. ParseIcalString does not support VCALENDAR
        j = icalStr.indexOf("END:VEVENT", i + "BEGIN:VEVENT".length);
        j = (j == -1? icalStr.length : j + "END:VEVENT".length);

        var eventData = icalStr.substring(i, j);
        var calendarEvent = createEvent();
        calendarEvent.icalString = eventData;

        calendar.addItem(calendarEvent, null);
    }
}

/**** parseIcalToDos
 *
 * Takes a text block of iCalendar todos and tries to split that into individual todos.
 * Parses those toDos and returns an array of calendarToDos.
 */
 
function parseIcalToDos(calendar, icalStr)
{
   for(var i=0, j=0; (i = icalStr.indexOf("BEGIN:VTODO", j)) != -1; ) { 
      // try to find the begin and end of an toDo. ParseIcalString does not support VCALENDAR
      j = icalStr.indexOf("END:VTODO", i + "BEGIN:VTODO".length);
      j = (j == -1? icalStr.length : j + "END:VTODO".length);

      var eventData = icalStr.substring(i, j);
      var calendarToDo = createToDo();
      calendarToDo.icalString = eventData;

      calendar.addItem(calendarToDo, null);
   }
}

/**** transformXCSData: transform into ics data
 *
 */
function transformXCSData( xcsString )
{
   var gParser = new DOMParser;
   var xmlDocument = gParser.parseFromString(xcsString, 'application/xml');   

   return serializeDocument(xmlDocument, "xcs2ics.xsl");
}


/**** readDataFromFile
 *
 * read data from a file. Returns the data read.
 */

function readDataFromFile( aFilePath, charset )
{
   const LOCALFILE_CTRID = "@mozilla.org/file/local;1";
   const FILEIN_CTRID = "@mozilla.org/network/file-input-stream;1";
   const SCRIPTSTREAM_CTRID = "@mozilla.org/scriptableinputstream;1";   
   const nsILocalFile = Components.interfaces.nsILocalFile;
   const nsIFileInputStream = Components.interfaces.nsIFileInputStream;
   const nsIScriptableInputStream = Components.interfaces.nsIScriptableInputStream;

   var localFileInstance = Components.classes[LOCALFILE_CTRID].createInstance( nsILocalFile );
   localFileInstance.initWithPath( aFilePath );

   var inputStream = Components.classes[FILEIN_CTRID].createInstance( nsIFileInputStream );
   try
   {
      var tmp; // not sure what the use is for this
      inputStream.init( localFileInstance, MODE_RDONLY, 0444, tmp );
      
      var scriptableInputStream = Components.classes[SCRIPTSTREAM_CTRID].createInstance( nsIScriptableInputStream);
      scriptableInputStream.init( inputStream );

      var aDataStream = scriptableInputStream.read( -1 );
      scriptableInputStream.close();
      inputStream.close();
      
      if( charset)
         aDataStream = convertToUnicode( charset, aDataStream );
   }
   catch(ex)
   {
      alert( gCalendarBundle.getString( "unableToRead" ) + aFilePath + "\n"+ex );
   }

   return aDataStream;
}


/**** saveEventsToFile
 *
 * Save data to a file. Create the file or overwrite an existing file.
 * Input an array of calendar events, or no parameter for selected events.
 */

function saveEventsToFile( calendarEventArray )
{
   if( !calendarEventArray)
      calendarEventArray = gCalendarWindow.EventSelection.selectedEvents;

   if (calendarEventArray.length == 0)
   {
      alert( gCalendarBundle.getString( "noEventsToSave" ) );
      return;
   }

   // Show the 'Save As' dialog and ask for a filename to save to
   const nsIFilePicker = Components.interfaces.nsIFilePicker;

   var fp = Components.classes["@mozilla.org/filepicker;1"].createInstance(nsIFilePicker);

   // caller can force disable of sand box, even if ON globally

   fp.init(window,  gCalendarBundle.getString("SaveAs"), nsIFilePicker.modeSave);

   if(calendarEventArray.length == 1 && calendarEventArray[0].title)
      fp.defaultString = calendarEventArray[0].title;
   else
      fp.defaultString = gCalendarBundle.getString( "defaultFileName" );

   fp.defaultExtension = "ics";

   fp.appendFilter( filterCalendar, "*" + extensionCalendar );
   fp.appendFilter( filterRtf, "*" + extensionRtf );
   fp.appendFilters(nsIFilePicker.filterHTML);
   fp.appendFilter( filterCsv, "*" + extensionCsv );
   fp.appendFilter( filterXcs, "*" + extensionXcs );
   fp.appendFilter( filterRdf, "*" + extensionRdf );
   fp.appendFilter( filtervCalendar, "*" + extensionvCalendar );

   fp.show();

   // Now find out as what to save, convert the events and save to file.
   if (fp.file && fp.file.path.length > 0) 
   {
      const UTF8 = "UTF-8";
      var aDataStream;
      var extension;
      var charset;
      switch (fp.filterIndex) {
      case 0 : // ics
         aDataStream = eventArrayToICalString( calendarEventArray, true );
         extension   = extensionCalendar;
         charset = "UTF-8";
         break;
      case 1 : // rtf
         aDataStream = eventArrayToRTF( calendarEventArray );
         extension   = extensionRtf;
         break;
      case 2 : // html
         aDataStream = eventArrayToHTML( calendarEventArray );
         extension   = ".htm";
         charset = "UTF-8";
         break;
      case 3 : // csv
         aDataStream = eventArrayToCsv( calendarEventArray );
         extension   = extensionCsv;
         charset = "UTF-8";
         break;
      case 4 : // xCal
         aDataStream = eventArrayToXCS( calendarEventArray );
         extension   = extensionXcs;
         charset = "UTF-8";
         break;
      case 5 : // rdf
         aDataStream = eventArrayToRdf( calendarEventArray );
         extension   = extensionRdf;
         charset = "UTF-8";
         break;
      case 6 : // vcs
         aDataStream = eventArrayToICalString( calendarEventArray, true );
         aDataStream = patchICalStringForVCal( aDataStream );
         extension   = extensionvCalendar;
         charset = "UTF-8";
         break;

      }
      var filePath = fp.file.path;
      if(filePath.indexOf(".") == -1 )
          filePath += extension;

      saveDataToFile( filePath, aDataStream, charset );
   }
}


/**** eventArrayToICalString
 * Converts a array of events to iCalendar text
 * If doPatchForExport is true:
 * - If all events have same method, merges components into one VCALENDAR.
 *   Times converted to Zulu, so no VTIMEZONEs expected (so no dup VTIMEZONES).
 * - Patches TRIGGER syntax for Outlook compatibility. 
 * - Converts line terminators to full \r\n as specified by RFC2445.
 */

function eventArrayToICalString( calendarEventArray, doPatchForExport )
{   
   if( !calendarEventArray)
      calendarEventArray = gCalendarWindow.EventSelection.selectedEvents;

   var doMerge = doPatchForExport;
   var eventArrayIndex;
   if (doPatchForExport && calendarEventArray.length > 0) 
   {
     // will merge into one VCALENDAR if all events have same method
     var firstMethod = calendarEventArray[0].method;
     for( eventArrayIndex = 1;  eventArrayIndex < calendarEventArray.length; ++eventArrayIndex )
     {
       if (calendarEventArray[eventArrayIndex].method != firstMethod)
       {
         doMerge = false;
         break;
       }
     }
   }

   var eventStrings = new Array(calendarEventArray.length);
   for( eventArrayIndex = 0;  eventArrayIndex < calendarEventArray.length; ++eventArrayIndex )
   {
      var calendarEvent = calendarEventArray[ eventArrayIndex ].clone();
      calendarEvent = calendarEvent.QueryInterface(Components.interfaces.calIEvent);
      
      // convert time to represent local to produce correct DTSTART and DTEND
      if(calendarEvent.isAllDay != true) {
	var startDate = calendarEvent.startDate;
	var endDate = calendarEvent.endDate;
	calendarEvent.startDate = startDate.getInTimezone("UTC");
	calendarEvent.endDate = endDate.getInTimezone("UTC");
      }
      // check if all required properties are available
      /*
      if( calendarEvent.method == 0 )
         calendarEvent.method = calendarEvent.ICAL_METHOD_PUBLISH;
      if( calendarEvent.stamp.year ==  0 )
         calendarEvent.stamp.setTime( new Date() );
      */
      var eventString = calendarEvent.icalString;
      if ( doPatchForExport )
      { 
        if (doMerge)
        {
          // include VCALENDAR version, prodid, method only on first component
          var begin = (eventArrayIndex == 0
                       ? 0
                       : eventString.indexOf("BEGIN:", 15+eventString.indexOf("BEGIN:VCALENDAR")));
          // include END:VCALENDAR only on last component
          var end = (eventArrayIndex == calendarEventArray.length - 1
                     ? eventString.length
                     : eventString.lastIndexOf("END:VCALENDAR"));
          // Include components between begin and end.
          // (Since times are all Zulu times, no VTIMEZONEs are expected,
          // so safe to assume no duplicate VTIMEZONES need to be removed.)
          eventString = eventString.slice(begin, end);
        }
        // patch TRIGGER for Outlook compatibility (before \r\n fix)
        eventString = patchICalStringForExport(eventString);
        // make sure all line terminators are full \r\n as required by rfc2445
        eventString = eventString.replace(/\r\n|\n|\r/g, "\r\n");
      }
      // collect result in array, will join at end
      eventStrings[eventArrayIndex] = eventString;
   }
   // concatenate all at once to avoid excess string copying on long calendars.

   return eventStrings.join("");
}


/**** patchICalStringForExport
 * Function to hack an iCalendar text block for use in other applications
 *  Patch TRIGGER field for Outlook
 */
 
function patchICalStringForExport( sTextiCalendar )
{
   // HACK: TRIGGER patch hack for Outlook 2000
   var i = sTextiCalendar.indexOf("TRIGGER\n ;VALUE=DURATION\n :-");
   if(i != -1) {
      sTextiCalendar =
         sTextiCalendar.substring(0,i+27) + sTextiCalendar.substring(i+28, sTextiCalendar.length);
   }

  return sTextiCalendar;
}

/**
 * patchICalStringForVCal:
 *   iCal (v2.0) [rfc2445sec4.1] says lines can be broken anywhere
 *     to comply with 75-octet line length (SHOULD NOT be longer)
 *   vCal (v1.0) [vCalendar1.0sec 2.1.3] says lines can only be broken
 *     at places where there may be linear white space, and does not give 
 *     a general max line length, though says quoted-printable lines should be
 *     less than 76 characters. 
 * 
 *   Quoted-printable lines are continued with an = just before the crlf.
 *
 *   So approach is to eliminate breaks in lines except quoted-printable ones.
 *   Replace      DESCRIPTION:line1\nline2
 *   with         DESCRIPTION;ENCODING=QUOTED-PRINTABLE:=
 *                 line1=0D=0A=
 *                 line2
 *
 *   Also breaks single long values in aribrary text values
 *   (COMMENT,DESCRIPTION,LOCATION,SUMMARY,CONTACT,RESOURCES)
 *   using quoted-printable line breaks (end line with =), and
 *   replaces \, with , in these values.
 *
 *   Converts version number to 1.0, as ENCODING=QUOTED-PRINTABLE is not
 *   part of the version 2.0 standard (it defers to transport MIME encoding).
 */
function patchICalStringForVCal( sTextiCalendar )
{
  // replace "\r\n " or "\r\n\t" with "" except when it follows "=".
  // i.e., replace [char-other-than-"="] followed by \r\n then [space-or-tab],
  // with just the captured ([char-other-than-"="])
  var unfolded = sTextiCalendar.replace(/([^=])\r\n[ \t]/g, "$1");
  var lines = unfolded.split("\r\n");
  // Replace "\\n" in TEXT values with "=0D=0A\r\n ":
  // - look for (property name and params) folowed by colon
  //   followed by (text with 'n' immediately preceded by an odd number of '\').
  //   Assumes params may not contain a colon.
  //   Assumes only text values contain "\\n"
  var multilinePropertyRegExp = /^([^:]+):((?:.*[^\\](?:[\\][\\])*)?\\n.*)$/;
  var longTextPropertyRegExp =
    /^((?:COMMENT|DESCRIPTION|LOCATION|SUMMARY|CONTACT|RESOURCES)[^:]*):(.*)$/;
  for (var i = 0; i < lines.length; i++)
  {
    var matchedMultiLineParts = lines[i].match(multilinePropertyRegExp);
    if (matchedMultiLineParts)
    {
      var propertyAndParams = matchedMultiLineParts[1];
      propertyAndParams = propertyAndParams+";ENCODING=QUOTED-PRINTABLE";

      var value = matchedMultiLineParts[2];
      // Replace "\\n" in text values with "=0D=0A\r\n ":
      value = value.replace(/\\n/g,
                            //replace if odd number of backslashes precede n
                            function(match, offset, whole) {
                              if (hasOddBackslashCount(whole, offset)){
                                return "=0D=0A=\r\n "; // odd, so replace
                              } else {
                                return match;          // even, so keep \n
                              }
                            });
      // trim if ends with "=0D=0A=\r\n ", to "=0D=0A"
      if (endsWith(value, "=0D=0A=\r\n "))
        value = value.substring(0, value.length - "=\r\n ".length);
      // Replace "\\," in text values with ",":
      value = replaceBackslashComma(value);
      // limit line length
      value = breakIntoContinuationLines(value, true);

      lines[i] = propertyAndParams + ":=\r\n " + value;
      continue;
    } 

    var matchedLongTextParts = lines[i].match(longTextPropertyRegExp);
    if (matchedLongTextParts) // maxlen
    {
      /*var*/ propertyAndParams = matchedLongTextParts[1];
      /*var*/ value = matchedLongTextParts[2];

      value = replaceBackslashComma(value);

      if (propertyAndParams.length + ":".length + value.length > 75)
        value = "\r\n "+breakIntoContinuationLines(value, false);

      lines[i] = propertyAndParams + ":" + value;
      continue;
    }

    if (lines[i]=="VERSION:2.0")
    {
      lines[i] = "VERSION:1.0";
      continue;
    }
  }
  return lines.join("\r\n")+"\r\n";
}

/** Replace \, with , if there is an odd number of backslashes. */
function replaceBackslashComma(value) 
{
  return value.replace(/\\,/g,
                       //replace if odd number of backslashes precede n
                       function(match, offset, whole) {
                         if (hasOddBackslashCount(whole, offset)){
                           return ",";            // odd, so replace
                         } else {
                           return match;          // even, so keep \,
                         }
                       });
}


/** whole is string.  Offset is position to look for last backslash.
    Returns true if there are an odd number of preceding backslashes,
    ending with the one at offset.  Returns false if offset does not
    point to a backslash, or there are an even number before offset inclusive.
 **/
function hasOddBackslashCount(whole, offset)
{
  var backSlashCount = 0;
  for (var i = offset; i >= 0; i--) {
    if (whole.charAt(i) == "\\")
      ++backSlashCount;
    else
      break;
  }
  return backSlashCount & 1;
}

/**
 * Break long lines in string value into shorter continuations.
 * If useQuotedPrintable, then lines are delimited by "=0D=0A=\r\n ",
 * else they are delimited by "\r\n ";
 * If useQUotedPrintable, then continuations are separated with "=\r\n ",
 * else they are separated with "\r\n ";
 */
function breakIntoContinuationLines(value, useQuotedPrintable)
{
  var lineDelimiter = (useQuotedPrintable? "=0D=0A=\r\n " : "\r\n ");
  var lineContinuer = (useQuotedPrintable? "=\r\n " : "\r\n ");
  // leave room to add "=\r\n " or "\r\n " and still be 75 chars or less.
  var maxLen = 75 - lineContinuer.length;
  if (value.length > maxLen) {
    var lines = value.split(lineDelimiter); // existing split
    for (var i = 0; i < lines.length; i++) {
      var line = lines[i];
      if (line.length > maxLen) {
        var lineParts = new Array();
        var start = 0; // start of part of line
        for (var end = maxLen; end < line.length; end+=maxLen) {
          // for clarity, try to break on whitespace.
          for(; end > 0; end--) { 
            var c = line.charAt(end - 1);
            if (c == " " || c == "\t")
              break;
          }
          if (end == start) // line was filled with nonbreaks
            end = start + maxLen;
          lineParts[lineParts.length] = line.substring(start, end);
          start = end;
        }
        lineParts[lineParts.length] = line.substring(start);
        lines[i] = lineParts.join(lineContinuer);
      }
    }
    value = lines.join(lineDelimiter);
  }
  return value;
}

function endsWith(whole, part) {
  if (part.length <= whole.length) {
    for (var i = 1; i <=part.length; i++)
      if (part[part.length - i] != whole[whole.length - i])
        return false;
    return true;
  } else {
    return false;
  }
}

/** 
* Converts a array of events to a block of HTML code
* Sample:
*    Summary: Phone Conference
*    When: Thursday, November 09, 2000 11:00 PM -- 11:30 PM
*    Where: San Francisco
*    Organizer: foo1@example.com
*
*    Agenda [Description]
*    1. Progress
*      a. marketing
*      b. engineering
*    2. Competition
*
* Description may be preformatted text with line breaks.
* If contains no indentation, then HTML <br> used for line breaks.
* otherwise description is enclosed in HTML <pre>...</pre>
* In When, plain text N-dash (--) converted to HTML &ndash;.
*/

function eventArrayToHTML( calendarEventArray )
{
   sHTMLHeader = 
      "<html>\n" + "<head>\n" + "<title>"+gCalendarBundle.getString( "HTMLTitle" )+"</title>\n" +
      "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\">\n" +
      "</head>\n"+ "<body bgcolor=\"#FFFFFF\" text=\"#000000\">\n";
   sHTMLFooter =
      "\n</body>\n</html>\n";

   var sHTMLText = sHTMLHeader;
   var dateFormat = new DateFormater();

   for( var eventArrayIndex = 0;  eventArrayIndex < calendarEventArray.length; ++eventArrayIndex )
   {
      var calendarEvent = calendarEventArray[ eventArrayIndex ];
      var start = new Date(calendarEvent.start.getTime());
      var end = new Date(calendarEvent.end.getTime());
      var when = dateFormat.formatInterval(start, end, calendarEvent.allDay);
      var desc = calendarEvent.description;
      if (desc == null)
        desc = "";
      if (desc.length > 0) { 
        if (desc.indexOf("\n ") >= 0 || desc.indexOf("\n\t") >= 0 ||
            desc.indexOf(" ") == 0 || desc.indexOf("\t") == 0)
          // (RegExp /^[ \t]/ doesn't work.)
          // contains indented preformatted text after beginning or newline
          // so preserve indentation with PRE.
          desc = "<PRE>"+desc+"</PRE>\n";
        else
          // no indentation, so preserve text line breaks in html with BR
          desc = "<P>"+desc.replace(/\n/g, "<BR>\n")+"</P>\n";
      }
      // use div around each event so events are navigable via DOM.
      sHTMLText += "<div><p>";
      sHTMLText += "<B>"+gCalendarBundle.getString( "eventTitle" )+"</B>\t" + calendarEvent.title + "<BR>\n";
      sHTMLText += "<B>"+gCalendarBundle.getString( "eventWhen" )+"</B>\t" + when.replace("--", "&ndash;") + "<BR>\n";
      sHTMLText += "<B>"+gCalendarBundle.getString( "eventWhere" )+"</B>\t" + calendarEvent.location + "<BR>\n";
      // sHTMLText += "<B>Organiser: </B>\t" + Event.???
      sHTMLText += "</p>\n";
      sHTMLText += desc; // may be empty
      sHTMLText += "</div>\n";
   }
   sHTMLText += sHTMLFooter;
   return sHTMLText;
}


/**** eventArrayToRTF
* Converts a array of events to a block of text in Rich Text Format
* Sample:
*    Summary: Phone Conference
*    When: Thursday, November 09, 2000 11:00 PM -- 11:30 PM
*    Where: San Francisco
*    Organizer: foo1@example.com
*
*    Agenda [Description]
*    1. Progress
*      a. marketing
*      b. engineering
*    2. Competition
*
* Description may be preformatted text with line breaks.
* In when, plain text N-dash (--) converted to RTF \endash.
*/

function eventArrayToRTF( calendarEventArray )
{
   sRTFHeader = 
      "{\\rtf1\\ansi\\ansicpg1252\\deff0\\deflang1033{\\fonttbl{\\f0\\fswiss\\fcharset0 Arial;}{\\f1\\fmodern\\fcharset0 Courier New;}}" +
      "{\\colortbl ;\\red0\\green0\\blue0;}" +
      "\\viewkind4\\uc1\\pard\\fi-1800\\li1800\\tx1800\\cf1";
   sRTFFooter =
      "\\pard\\fi-1800\\li1800\\tx1800\\cf1\\f0\\par}";

   var sRTFText = sRTFHeader;
   var dateFormat = new DateFormater();

   for( var eventArrayIndex = 0;  eventArrayIndex < calendarEventArray.length; ++eventArrayIndex )
   {
      var calendarEvent = calendarEventArray[ eventArrayIndex ];
      var start = new Date(calendarEvent.start.getTime());
      var end = new Date(calendarEvent.end.getTime());
      var when = dateFormat.formatInterval(start, end, calendarEvent.allDay);
      var desc = calendarEvent.description;
      if (desc == null)
        desc = "";
      if (desc.length > 0) {
        if (desc.charAt(desc.length - 1) != "\n") 
          desc = desc+"\n"; // add final newline if doesn't end with newline
        desc += "\n"; // add blank line after non-empty description
      }
      sRTFText += "\\b\\f0\\fs20 " + gCalendarBundle.getString( "eventTitle" ) + "\\b0\\tab " + calendarEvent.title + "\\par\n";
      sRTFText += "\\b " + gCalendarBundle.getString( "eventWhen" ) + "\\b0\\tab " + when.replace("--", "\\endash ") + "\\par\n";
      sRTFText += "\\b " + gCalendarBundle.getString( "eventWhere" ) + "\\b0\\tab " + calendarEvent.location + "\\par\n";
      sRTFText += "\\par\n";
      sRTFText += desc.replace(/\n/g, "\\par\n"); // preserve text line breaks in rtf with \\par
   }
   sRTFText += sRTFFooter;
   return sRTFText;
}


/**** eventArrayToXML
* Converts a array of events to a XML string
*/
/*
function eventArrayToXML( calendarEventArray )
{
   var xmlDocument = getXmlDocument( calendarEventArray );
   var serializer = new XMLSerializer;
   
   return serializer.serializeToString (xmlDocument )
}
*/

/**** eventArrayToCsv
* Converts a array of events to comma delimited  text.
*/

function eventArrayToCsv( calendarEventArray )
{
   var xcsDocument = getXcsDocument( calendarEventArray );

   return serializeDocument( xcsDocument, "xcs2csv.xsl" );
}


/**** eventArrayToRdf
* Converts a array of events to RDF
*/

function eventArrayToRdf( calendarEventArray )
{
   var xcsDocument = getXcsDocument( calendarEventArray );

   return serializeDocument( xcsDocument, "xcs2rdf.xsl" );
}


/**** eventArrayToXCS
* Converts a array of events to a xCal string
*/

function eventArrayToXCS( calendarEventArray )
{
   var xmlDoc = getXmlDocument( calendarEventArray );
   var xcsDoc = transformXML( xmlDoc, "xml2xcs.xsl" );
   // add the doctype
/* Doctype uri blocks excel import
   var newdoctype = xcsDoc.implementation.createDocumentType(
       "iCalendar", 
       "-//IETF//DTD XCAL//iCalendar XML//EN",
       "http://www.ietf.org/internet-drafts/draft-ietf-calsch-many-xcal-02.txt");
   if (newdoctype)
      xcsDoc.insertBefore(newdoctype, xcsDoc.firstChild);
*/
   var serializer = new XMLSerializer;

   // XXX MAJOR UGLY HACK!! Serializer doesn't insert XML Declaration
   // http://bugzilla.mozilla.org/show_bug.cgi?id=63558
   var serialDocument = serializer.serializeToString ( xcsDoc );

   if( serialDocument.indexOf( "<?xml" ) == -1 )
      serialDocument = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" + serialDocument;

   return serialDocument;
   // return serializer.serializeToString ( xcsDoc )
}


/**** saveDataToFile
 *
 * Save data to a file. Creates a new file or overwrites an existing file.
 */

function saveDataToFile(aFilePath, aDataStream, charset)
{
   const LOCALFILE_CTRID = "@mozilla.org/file/local;1";
   const FILEOUT_CTRID = "@mozilla.org/network/file-output-stream;1";
   const nsILocalFile = Components.interfaces.nsILocalFile;
   const nsIFileOutputStream = Components.interfaces.nsIFileOutputStream;

   var outputStream;
   
   var localFileInstance = Components.classes[LOCALFILE_CTRID].createInstance(nsILocalFile);
   localFileInstance.initWithPath(aFilePath);

   outputStream = Components.classes[FILEOUT_CTRID].createInstance(nsIFileOutputStream);
   try
   {
      if(charset)
         aDataStream = convertFromUnicode( charset, aDataStream );

      outputStream.init(localFileInstance, MODE_WRONLY | MODE_CREATE | MODE_TRUNCATE, 0664, 0);
      outputStream.write(aDataStream, aDataStream.length);
      // outputStream.flush();
      outputStream.close();
   }
   catch(ex)
   {
      alert(gCalendarBundle.getString( "unableToWrite" ) + aFilePath );
   }
}


////////////////////////////////////
// XML/XSL functions              //
////////////////////////////////////

/**
*   Get the local path to the chrome directory
*/
/*
function getChromeDir()
{
    const JS_DIR_UTILS_FILE_DIR_CID = "@mozilla.org/file/directory_service;1";
    const JS_DIR_UTILS_I_PROPS      = "nsIProperties";
    const JS_DIR_UTILS_DIR          = new Components.Constructor(JS_DIR_UTILS_FILE_DIR_CID, JS_DIR_UTILS_I_PROPS);
    const JS_DIR_UTILS_CHROME_DIR                          = "AChrom";

    var rv;

    try
    {
      rv=(new JS_DIR_UTILS_DIR()).get(JS_DIR_UTILS_CHROME_DIR, Components.interfaces.nsIFile);
    }

    catch (e)
    {
       //jslibError(e, "(unexpected error)", "NS_ERROR_FAILURE", JS_DIR_UTILS_FILE+":getChromeDir");
       rv=null;
    }

    return rv;
}

function getTemplatesPath( templateName )
{
   var templatesDir = getChromeDir();
   templatesDir.append( "calendar" );
   templatesDir.append( "content" );
   templatesDir.append( "templates" );
   templatesDir.append( templateName );
   return templatesDir.path;
}

function xslt(xmlUri, xslUri)
{
   // TODO: Check uri's for CHROME:// and convert path to local path
   var xslProc = new XSLTProcessor();

   var result = document.implementation.createDocument("", "", null);
   var xmlDoc = document.implementation.createDocument("", "", null);
   var xslDoc = document.implementation.createDocument("", "", null);

   xmlDoc.load(xmlUri, "application/xml");
   xslDoc.load(xslUri, "application/xml");
   xslProc.transformDocument(xmlDoc, xslDoc, result, null);

   return result;
}

*/

/** PRIVATE
*
*   Opens a file and returns the content
*   It supports Uri's like chrome://
*/

function loadFile(aUriSpec)
{
    netscape.security.PrivilegeManager.enablePrivilege('UniversalXPConnect');
    var serv = Components.classes["@mozilla.org/network/io-service;1"].
        getService(Components.interfaces.nsIIOService);
    if (!serv) {
        throw Components.results.ERR_FAILURE;
    }
    var chan = serv.newChannel(aUriSpec, null, null);
    var instream = 
        Components.classes["@mozilla.org/scriptableinputstream;1"].createInstance(Components.interfaces.nsIScriptableInputStream);
    instream.init(chan.open());

    return instream.read(instream.available());
}

/** PUBLIC GetXcsDocument
*
*/

function getXcsDocument( calendarEventArray )
{
   var xmlDocument = getXmlDocument( calendarEventArray );

   var xcsDocument = transformXML( xmlDocument, "xml2xcs.xsl" );

   var processingInstruction = xcsDocument.createProcessingInstruction(
      "xml", "version=\"1.0\" encoding=\"UTF-8\"");
   // xcsDocument.insertBefore(processingInstruction, xcsDocument.firstChild);
   
   return xcsDocument;
}


/** PUBLIC
*
*   Opens a xsl transformation file and applies it to xmlDocuments.
*   xslFile can be in the convertersDirectory, or a full Uri
*   Returns the resulting document
*/

function transformXML( xmlDocument, xslFilename )
{
   var xslProc = new XSLTProcessor();
   var gParser = new DOMParser;
   // .load isn't synchrone
   // var xslDoc = document.implementation.createDocument("", "", null);
   // xslDoc.load(path, "application/xml");

   // if only passsed a filename, assume it is a file in the default directory
   if( xslFilename.indexOf( ":" ) == -1 )
     xslFilename = convertersDirectory + xslFilename;

   var xslContent = loadFile( xslFilename );
   var xslDocument = gParser.parseFromString(xslContent, 'application/xml');
   var result = document.implementation.createDocument("", "", null);

   xslProc.transformDocument(xmlDocument, xslDocument, result, null);

   return result;
}

/** PUBLIC
*
*   Serializes a DOM document.if stylesheet is null, returns the document serialized
*   Applies the stylesheet when available, and return the serialized transformed document,
*   or the transformiix data
*/

function serializeDocument( xmlDocument, stylesheet )
{
   var serializer = new XMLSerializer;
   if( stylesheet )
   {
      var resultDocument = transformXML( xmlDocument, stylesheet );
      var transformiixResult = resultDocument.getElementById( "transformiixResult" );
      if( transformiixResult && transformiixResult.hasChildNodes() )
      {  // It's a document starting with:
         // <a0:html xmlns:a0="http://www.w3.org/1999/xhtml">
         // <a0:head/><a0:body><a0:pre id="transformiixResult">
         var textNode = transformiixResult.firstChild;
         if( textNode.nodeType == textNode.TEXT_NODE )
            return textNode.nodeValue;
         else
            return serializer.serializeString( transformiixResult );
      }
      else
         // No transformiixResult, return the serialized transformed document
         return serializer.serializeToString ( resultDocument );
   }
   else
      // No transformation, return the serialized xmlDocument
      return serializer.serializeToString ( xmlDocument );
}

/** PUBLIC
*
*   not used?
*/

function deserializeDocument(text, stylesheet )
{
   var gParser = new DOMParser;
   var xmlDocument = gParser.parseFromString(text, 'application/xml');
}

/** PRIVATE
*
*   transform a calendar event into a dom node.
*   hack, this is going to be part of libical
*/

function makeXmlNode( xmlDocument, calendarEvent )
{

   // Adds a property node to a event node
   //  do not add empty properties, valueType is optional
   var addPropertyNode = function( xmlDocument, eventNode, name, value, valueType )
   {
      if(!value)
         return;

      var propertyNode = xmlDocument.createElement( "property" );
      propertyNode.setAttribute( "name", name );

      var valueNode = xmlDocument.createElement( "value" );
      var textNode  = xmlDocument.createTextNode( value );

      if( valueType )
         valueNode.setAttribute( "value", valueType );

      valueNode.appendChild( textNode );
      propertyNode.appendChild( valueNode );
      eventNode.appendChild( propertyNode );
   }

   var addAlarmNode = function( xmlDocument, eventNode, triggerTime, triggerUnits )
   {
      if(triggerUnits == "minutes")
        triggerIcalUnits = "M";
      else  if(triggerUnits == "hours")
        triggerIcalUnits = "H";
      else  if(triggerUnits == "days")
        triggerIcalUnits = "D";

      var valarmNode = xmlDocument.createElement( "component" );
      valarmNode.setAttribute( "name", "VALARM" );

      var propertyNode = xmlDocument.createElement( "property" );
      propertyNode.setAttribute( "name", "TRIGGER" );

      var valueNode = xmlDocument.createElement( "value" );
      //valueNode.setAttribute( "value", "DURATION" );

      var textNode  = xmlDocument.createTextNode( "-PT" + triggerTime + triggerIcalUnits );

      valueNode.appendChild( textNode );
      propertyNode.appendChild( valueNode );
      valarmNode.appendChild( propertyNode )
      eventNode.appendChild( valarmNode );
   }
   
   var addRRuleNode = function( xmlDocument, eventNode )
   {
      // extremly ugly hack, but this will be done in libical soon
      var ruleText = "";
      var eventText = calendarEvent.getIcalString();
      var i = eventText.indexOf("RRULE");
      if( i > -1)
      {
         ruleText = eventText.substring(i+8);      
         ruleText = ruleText.substring(0, ruleText.indexOf("\n"));
      }
      if( ruleText && ruleText.length > 0)
      {
         var propertyNode = xmlDocument.createElement( "property" );
         propertyNode.setAttribute( "name", "RRULE" );

         var valueNode = xmlDocument.createElement( "value" );
         var textNode  = xmlDocument.createTextNode( ruleText );

         valueNode.appendChild( textNode );
         propertyNode.appendChild( valueNode );
         eventNode.appendChild( propertyNode );
      }
   }

     var checkString = function( str )
    {
        if( typeof( str ) == "string" )
            return str;
        else
            return ""
    }

    var checkNumber = function( num )
    {
        if( typeof( num ) == "undefined" || num == null )
            return "";
        else
            return num
    }

    var checkBoolean = function( bool )
    {
        if( bool == "false")
            return "false"
        else if( bool )      // this is false for: false, 0, undefined, null, ""
            return "true";
        else
            return "false"
    }

    // create a string in the iCalendar format, UTC time '20020412T121314Z'
    var checkDate = function ( dt, isDate )
    {   
        var dateObj = new Date( dt.getTime() );
        var result = "";

        if( isDate )
        {
            result += dateObj.getFullYear();
            if( dateObj.getMonth() + 1 < 10 )
               result += "0";
            result += dateObj.getMonth() + 1;

            if( dateObj.getDate() < 10 )
               result += "0";
            result += dateObj.getDate();
        }
        else
        {
           result += dateObj.getUTCFullYear();

           if( dateObj.getUTCMonth() + 1 < 10 )
              result += "0";
           result += dateObj.getUTCMonth() + 1;

           if( dateObj.getUTCDate() < 10 )
              result += "0";
           result += dateObj.getUTCDate();

           result += "T"

           if( dateObj.getUTCHours() < 10 )
              result += "0";
           result += dateObj.getUTCHours();

           if( dateObj.getUTCMinutes() < 10 )
              result += "0";
           result += dateObj.getUTCMinutes();

           if( dateObj.getUTCSeconds() < 10 )
              result += "0";
           result += dateObj.getUTCSeconds();
           result += "Z";
        }

        return result;
    }

    // make the event tag
    var calendarNode = xmlDocument.createElement( "component" );
    calendarNode.setAttribute( "name", "VCALENDAR" );

    addPropertyNode( xmlDocument, calendarNode, "PRODID", "-//Mozilla.org/NONSGML Mozilla Calendar V 1.0 //EN" );
    addPropertyNode( xmlDocument, calendarNode, "VERSION", "2.0" );
    if(calendarEvent.method == calendarEvent.ICAL_METHOD_PUBLISH)
       addPropertyNode( xmlDocument, calendarNode, "METHOD", "PUBLISH" );
    else if(calendarEvent.method == calendarEvent.ICAL_METHOD_REQUEST )
       addPropertyNode( xmlDocument, calendarNode, "METHOD", "REQUEST" );

    var eventNode = xmlDocument.createElement( "component" );
    eventNode.setAttribute( "name", "VEVENT" );

    addPropertyNode( xmlDocument, eventNode, "UID", calendarEvent.id );
    addPropertyNode( xmlDocument, eventNode, "SUMMARY", checkString( calendarEvent.title ) );
    addPropertyNode( xmlDocument, eventNode, "DTSTAMP", checkDate( calendarEvent.stamp ) );
    if( calendarEvent.allDay )
       addPropertyNode( xmlDocument, eventNode, "DTSTART", checkDate( calendarEvent.start, true ), "DATE" );
    else
    {
       addPropertyNode( xmlDocument, eventNode, "DTSTART", checkDate( calendarEvent.start ) );
       addPropertyNode( xmlDocument, eventNode, "DTEND", checkDate( calendarEvent.end ) );
    }

    addPropertyNode( xmlDocument, eventNode, "DESCRIPTION", checkString( calendarEvent.description ) );
    addPropertyNode( xmlDocument, eventNode, "CATEGORIES", checkString( calendarEvent.categories ) );
    addPropertyNode( xmlDocument, eventNode, "LOCATION", checkString( calendarEvent.location ) );
    addPropertyNode( xmlDocument, eventNode, "PRIVATEEVENT", checkString( calendarEvent.privateEvent ) );
    addPropertyNode( xmlDocument, eventNode, "URL", checkString( calendarEvent.url ) );
    addPropertyNode( xmlDocument, eventNode, "PRIORITY", checkNumber( calendarEvent.priority ) );

    addAlarmNode( xmlDocument, eventNode, calendarEvent.alarmLength, calendarEvent.alarmUnits );
    addRRuleNode( xmlDocument, eventNode );

    calendarNode.appendChild( eventNode );
    return calendarNode;
}

/** PUBLIC
*
*   Transforms an array of calendar events into a dom-document
*   hack, this is going to be part of libical
*/

function getXmlDocument ( eventList )
{
    // use the domparser to create the XML 
    var domParser = new DOMParser;
    // start with one tag
    var xmlDocument = domParser.parseFromString( "<libical/>", "application/xml" );
    
    // get the top tag, there will only be one.
    var topNodeList = xmlDocument.getElementsByTagName( "libical" );
    var topNode = topNodeList[0];


    // add each event as an element
 
    for( var index = 0; index < eventList.length; ++index )
    {
        var calendarEvent = eventList[ index ];
        
        var eventNode = this.makeXmlNode( xmlDocument, calendarEvent );
        
        topNode.appendChild( eventNode );
    }
    return xmlDocument;
}

function startImport() {
  /*
    var ImportExportErrorHandler = {
        errorreport : "",
        onLoad   : function() {},
        onStartBatch   : function() {},
        onEndBatch   : function() {},
        onAddItem : function( calendarEvent ) {},
        onModifyItem : function( calendarEvent, originalEvent ) {},
        onDeleteItem : function( calendarEvent, nextEvent ) {},
        onAlarm : function( calendarEvent ) {},
        onError : function( severity, errorid, errorstring ) 
        {
            this.errorreport=this.errorreport+gCalendarBundle.getString( errorid )+"\n";
        },
        showErrors : function () {
            if( this.errorreport != "" )
                alert( "Errors:\n"+this.errorreport );
        }

    }
    gICalLib.addObserver( ImportExportErrorHandler );
    ImportExportErrorHandler.showErrors();
    gICalLib.removeObserver( ImportExportErrorHandler );
  */

    loadEventsFromFile();

}
