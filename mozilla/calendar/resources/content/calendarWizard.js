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
 * Contributor(s): Mike Potter <mikep@oeone.com>
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

var gWizardType;

function checkInitialPage()
{
   gWizardType = document.getElementById( "initial-radiogroup" ).selectedItem.value;
   
   document.getElementsByAttribute( "pageid", "initialPage" )[0].setAttribute( "next", gWizardType );
}


function buildCalendarsListbox( ListBoxId )
{
   for( var i = 0; i < opener.gCalendarWindow.calendarManager.calendars.length; i++ )
   {
      addCalendarToListBox( opener.gCalendarWindow.calendarManager.calendars[i], ListBoxId );
   }
}

function addCalendarToListBox( ThisCalendarObject, ListBoxId )
{
   var calendarListItem = document.createElement( "listitem" );
   calendarListItem.calendarObject = ThisCalendarObject;

   calendarListItem.setAttribute( "id", "calendar-list-item-"+ThisCalendarObject.serverNumber );
   calendarListItem.setAttribute( "class", "calendar-list-item-class" );
   calendarListItem.setAttribute( "src", "chrome://calendar/skin/synch_animated.gif" );
   calendarListItem.setAttribute( "label", ThisCalendarObject.name );
   calendarListItem.setAttribute( "flex", "1" );
   calendarListItem.setAttribute( "calendarPath", ThisCalendarObject.path );
   
   document.getElementById( ListBoxId ).appendChild( calendarListItem );
}

function doWizardFinish( )
{
   switch( gWizardType )
   {
   case "import":
      return doWizardImport();
      break;

   case "export":
      return doWizardExport();
      break;

   case "subscribe":
      return doWizardSubscribe();
      break;

   case "publish":
      return doWizardPublish();
      break;
   }
}


function doWizardImport()
{
   var calendarEventArray;

   var fileName = document.getElementById( "import-path-textbox" ).value;

   var aDataStream = readDataFromFile( fileName, "UTF-8" );

   if( fileName.indexOf( ".ics" ) == -1 )
   {
      calendarEventArray = parseIcalData( aDataStream );   
   }
   else if( fileName.indexOf( ".xcs" ) == -1 )
   {
      calendarEventArray = parseXCSData( aDataStream );
   }
        
   if( document.getElementById( "import-2-radiogroup" ).selectedItem.value == "silent" )
   {
      addEventsToCalendar( calendarEventArray, true );
   }
   else
   {
      addEventsToCalendar( calendarEventArray, true );
   }
   

   return( false ); //true will close the window
}


function doWizardExport()
{
   const UTF8 = "UTF-8";
   var aDataStream;
   var extension;
   var charset;
   
   var fileName = document.getElementById( "export-path-textbox" ).value;

   if( fileName.indexOf( ".ics" ) == -1 )
   {

      aDataStream = eventArrayToICalString( calendarEventArray, true );
      extension   = extensionCalendar;
      charset = "UTF-8";
   }
   else if( fileName.indexOf( ".rtf" ) == -1 )
   {
      aDataStream = eventArrayToRTF( calendarEventArray );
      extension   = extensionRtf;
   }
   else if( fileName.indexOf( ".htm" ) == -1 )
   {
      aDataStream = eventArrayToHTML( calendarEventArray );
      extension   = ".htm";
      charset = "UTF-8";
   }
   else if( fileName.indexOf( ".csv" ) == -1 )
   {
      aDataStream = eventArrayToCsv( calendarEventArray );
      extension   = extensionCsv;
      charset = "UTF-8";
   }
   else if( fileName.indexOf( ".xml" ) == -1 )
   {
      aDataStream = eventArrayToXCS( calendarEventArray );
      extension   = extensionXcs;
      charset = "UTF-8";
   }
   else if( fileName.indexOf( ".rdf" ) == -1 )
   {
      aDataStream = eventArrayToRdf( calendarEventArray );
      extension   = extensionRdf;
      charset = "UTF-8";
   }

   saveDataToFile( filePath, aDataStream, charset );
}

function launchFilePicker( Mode, ElementToGiveValueTo )
{
   // No show the 'Save As' dialog and ask for a filename to save to
   const nsIFilePicker = Components.interfaces.nsIFilePicker;

   var fp = Components.classes["@mozilla.org/filepicker;1"].createInstance(nsIFilePicker);

   // caller can force disable of sand box, even if ON globally
   switch( Mode )
   {
   case "open":
      fp.defaultExtension = "ics";

      fp.appendFilter( filterCalendar, "*" + extensionCalendar );
      fp.appendFilter( filterXcs, "*" + extensionXcs );
   
      fp.init(window, "Open", nsIFilePicker.modeOpen);
      break;

   case "save":
      fp.init(window, "Save", nsIFilePicker.modeSave);
      //if(calendarEventArray.length == 1 && calendarEventArray[0].title)
      //   fp.defaultString = calendarEventArray[0].title;
      //else
         fp.defaultString = "Mozilla Calendar events";
   
      fp.defaultExtension = "ics";
   
      fp.appendFilter( filterCalendar, "*" + extensionCalendar );
      fp.appendFilter( filterRtf, "*" + extensionRtf );
      fp.appendFilters(nsIFilePicker.filterHTML);
      fp.appendFilter( filterCsv, "*" + extensionCsv );
      fp.appendFilter( filterXcs, "*" + extensionXcs );
      fp.appendFilter( filterRdf, "*" + extensionRdf );

      break;
   }
   
   fp.show();

   if (fp.file && fp.file.path.length > 0)
   {
      /*if(filePath.indexOf(".") == -1 )
          filePath += extension;
      */
      document.getElementById( ElementToGiveValueTo ).value = fp.file.path;
   }
}
