/* 
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
 * The Original Code is OEone Corporation.
 *
 * The Initial Developer of the Original Code is
 * OEone Corporation.
 * Portions created by OEone Corporation are Copyright (C) 2001
 * OEone Corporation. All Rights Reserved.
 *
 * Contributor(s): Mostafa Hosseini (mostafah@oeone.com)
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
 * 
*/

const DEFAULT_TITLE="Lunch Time";
const DEFAULT_DESCRIPTION = "Will be out for one hour";
const DEFAULT_LOCATION = "Restaurant";
const DEFAULT_CATEGORY = "Personal";
const DEFAULT_EMAIL = "mostafah@oeone.com";
const DEFAULT_PRIVATE = false;
const DEFAULT_ALLDAY = false;
const DEFAULT_ALARM = true;
const DEFAULT_ALARMUNITS = "minutes";
const DEFAULT_ALARMLENGTH = 5;
const DEFAULT_RECUR = true;
const DEFAULT_RECURINTERVAL = 7;
const DEFAULT_RECURUNITS = "days";
const DEFAULT_RECURFOREVER = true;

function Test()
{
    netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
    var iCalLibComponent = Components.classes["@mozilla.org/ical;1"].createInstance();
    
    this.iCalLib = iCalLibComponent.QueryInterface(Components.interfaces.oeIICal);
    
    this.iCalLib.SetServer( "/tmp/.oecalendar" );
    
    this.iCalLib.Test();
}

function TestAll()
{
    var id = TestAddEvent();
    var iCalEvent = TestFetchEvent( id );
    id = TestUpdateEvent( iCalEvent );
//    TestSearchEvent();
    TestDeleteEvent( id );
    TestRecurring();
    alert( "Test Successfull" );
}

function TestAddEvent()
{
    netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
    var iCalLibComponent = Components.classes["@mozilla.org/ical;1"].createInstance();
    
    this.iCalLib = iCalLibComponent.QueryInterface(Components.interfaces.oeIICal);
    
    var iCalEventComponent = Components.classes["@mozilla.org/icalevent;1"].createInstance();
    
    this.iCalEvent = iCalEventComponent.QueryInterface(Components.interfaces.oeIICalEvent);

    this.iCalLib.SetServer( "/tmp/.oecalendar" );
    
    iCalEvent.title = DEFAULT_TITLE;
    iCalEvent.description = DEFAULT_DESCRIPTION;
    iCalEvent.location = DEFAULT_LOCATION;
    iCalEvent.category = DEFAULT_CATEGORY;
    iCalEvent.privateEvent = DEFAULT_PRIVATE;
    iCalEvent.allDay = DEFAULT_ALLDAY;
    iCalEvent.alarm = DEFAULT_ALARM;
    iCalEvent.alarmUnits = DEFAULT_ALARMUNITS;
    iCalEvent.alarmLength = DEFAULT_ALARMLENGTH;
    iCalEvent.alarmEmailAddress = DEFAULT_EMAIL;
    iCalEvent.inviteEmailAddress = DEFAULT_EMAIL;

    iCalEvent.recur = DEFAULT_RECUR;
    iCalEvent.recurInterval = DEFAULT_RECURINTERVAL;
    iCalEvent.recurUnits = DEFAULT_RECURUNITS;
    iCalEvent.recurForever = DEFAULT_RECURFOREVER;

    iCalEvent.start.year = 2001;
    iCalEvent.start.month = 10; //November
    iCalEvent.start.day = 1;
    iCalEvent.start.hour = 12;
    iCalEvent.start.minute = 24;

    iCalEvent.end.year = 2001;
    iCalEvent.end.month = 10; //November
    iCalEvent.end.day = 1;
    iCalEvent.end.hour = 13;
    iCalEvent.end.minute = 24;

    var id = this.iCalLib.addEvent( iCalEvent );
    
    if( id == null )
       alert( "Invalid Id" );
    if( iCalEvent.title != DEFAULT_TITLE )
       alert( "Invalid Title" );
    if( iCalEvent.description != DEFAULT_DESCRIPTION )
       alert( "Invalid Description" );
    if( iCalEvent.location != DEFAULT_LOCATION )
       alert( "Invalid Location" );
    if( iCalEvent.category != DEFAULT_CATEGORY )
       alert( "Invalid Category" );
    if( iCalEvent.privateEvent != DEFAULT_PRIVATE )
       alert( "Invalid PrivateEvent Setting" );
    if( iCalEvent.allDay != DEFAULT_ALLDAY )
       alert( "Invalid AllDay Setting" );
    if( iCalEvent.alarm != DEFAULT_ALARM )
       alert( "Invalid Alarm Setting" );
    if( iCalEvent.alarmUnits != DEFAULT_ALARMUNITS )
       alert( "Invalid Alarm Units" );
    if( iCalEvent.alarmLength != DEFAULT_ALARMLENGTH )
       alert( "Invalid Alarm Length" );
    if( iCalEvent.alarmEmailAddress != DEFAULT_EMAIL )
       alert( "Invalid Alarm Email Address" );
    if( iCalEvent.inviteEmailAddress != DEFAULT_EMAIL )
       alert( "Invalid Invite Email Address" );
    if( iCalEvent.recur != DEFAULT_RECUR )
       alert( "Invalid Recur Setting" );
    if( iCalEvent.recurInterval != DEFAULT_RECURINTERVAL )
       alert( "Invalid Recur Interval" );
    if( iCalEvent.recurUnits != DEFAULT_RECURUNITS )
       alert( "Invalid Recur Units" );
    if( iCalEvent.recurForever != DEFAULT_RECURFOREVER )
       alert( "Invalid Recur Forever" );

    //TODO: Check for start and end date

    return id;
}

function TestFetchEvent( id )
{
    netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
    var iCalLibComponent = Components.classes["@mozilla.org/ical;1"].createInstance();
    
    this.iCalLib = iCalLibComponent.QueryInterface(Components.interfaces.oeIICal);
    
    this.iCalLib.SetServer( "/tmp/.oecalendar" );

    var iCalEvent = iCalLib.fetchEvent( id );
    if( id == null )
       alert( "Invalid Id" );
    if( iCalEvent.title != DEFAULT_TITLE )
       alert( "Invalid Title" );
    if( iCalEvent.description != DEFAULT_DESCRIPTION )
       alert( "Invalid Description" );
    if( iCalEvent.location != DEFAULT_LOCATION )
       alert( "Invalid Location" );
    if( iCalEvent.category != DEFAULT_CATEGORY )
       alert( "Invalid Category" );
    if( iCalEvent.privateEvent != DEFAULT_PRIVATE )
       alert( "Invalid PrivateEvent Setting" );
    if( iCalEvent.allDay != DEFAULT_ALLDAY )
       alert( "Invalid AllDay Setting" );
    if( iCalEvent.alarm != DEFAULT_ALARM )
       alert( "Invalid Alarm Setting" );
    if( iCalEvent.alarmUnits != DEFAULT_ALARMUNITS )
       alert( "Invalid Alarm Units" );
    if( iCalEvent.alarmLength != DEFAULT_ALARMLENGTH )
       alert( "Invalid Alarm Length" );
    if( iCalEvent.alarmEmailAddress != DEFAULT_EMAIL )
       alert( "Invalid Alarm Email Address" );
    if( iCalEvent.inviteEmailAddress != DEFAULT_EMAIL )
       alert( "Invalid Invite Email Address" );
    if( iCalEvent.recur != DEFAULT_RECUR )
       alert( "Invalid Recur Setting" );
    if( iCalEvent.recurInterval != DEFAULT_RECURINTERVAL )
       alert( "Invalid Recur Interval" );
    if( iCalEvent.recurUnits != DEFAULT_RECURUNITS )
       alert( "Invalid Recur Units" );
    if( iCalEvent.recurForever != DEFAULT_RECURFOREVER )
       alert( "Invalid Recur Forever" );

    //TODO: Check for start and end date

    return iCalEvent;
}

function TestUpdateEvent( iCalEvent )
{
    netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
    var iCalLibComponent = Components.classes["@mozilla.org/ical;1"].createInstance();
    
    this.iCalLib = iCalLibComponent.QueryInterface(Components.interfaces.oeIICal);
    
    this.iCalLib.SetServer( "/tmp/.oecalendar" );
    
    iCalEvent.title = DEFAULT_TITLE+"*NEW*";
    iCalEvent.description = DEFAULT_DESCRIPTION+"*NEW*";
    iCalEvent.location = DEFAULT_LOCATION+"*NEW*";
    iCalEvent.category = DEFAULT_CATEGORY+"*NEW*";
    iCalEvent.privateEvent = !DEFAULT_PRIVATE;
    iCalEvent.allDay = !DEFAULT_ALLDAY;
    iCalEvent.alarm = !DEFAULT_ALARM;

    iCalEvent.recur = !DEFAULT_RECUR;

    iCalEvent.start.year = 2002;
    iCalEvent.start.month = 11; //December
    iCalEvent.start.day = 2;
    iCalEvent.start.hour = 13;
    iCalEvent.start.minute = 25;

    iCalEvent.end.year = 2002;
    iCalEvent.end.month = 11; //December
    iCalEvent.end.day = 2;
    iCalEvent.end.hour = 14;
    iCalEvent.end.minute = 25;

    var id = this.iCalLib.modifyEvent( iCalEvent );
    
    if( id == null )
       alert( "Invalid Id" );
    if( iCalEvent.title != DEFAULT_TITLE+"*NEW*" )
       alert( "Invalid Title" );
    if( iCalEvent.description != DEFAULT_DESCRIPTION+"*NEW*" )
       alert( "Invalid Description" );
    if( iCalEvent.location != DEFAULT_LOCATION+"*NEW*" )
       alert( "Invalid Location" );
    if( iCalEvent.category != DEFAULT_CATEGORY+"*NEW*" )
       alert( "Invalid Category" );
    if( iCalEvent.privateEvent != !DEFAULT_PRIVATE )
       alert( "Invalid PrivateEvent Setting" );
    if( iCalEvent.allDay != !DEFAULT_ALLDAY )
       alert( "Invalid AllDay Setting" );
    if( iCalEvent.alarm != !DEFAULT_ALARM )
       alert( "Invalid Alarm Setting" );
    if( iCalEvent.recur != !DEFAULT_RECUR )
       alert( "Invalid Recur Setting" );

    //TODO check start and end dates

    return id;
}

function TestSearchEvent()
{
    netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
    var iCalLibComponent = Components.classes["@mozilla.org/ical;1"].createInstance();
    
    this.iCalLib = iCalLibComponent.QueryInterface(Components.interfaces.oeIICal);
    
    this.iCalLib.SetServer( "/tmp/.oecalendar" );
    
    var result = this.iCalLib.SearchByDate( 2000,01,01,00,00,2002,01,01,00,00 );
    result = this.iCalLib.SearchBySQL( "SELECT * FROM VEVENT WHERE CATEGORIES = 'Personal'" );
    result = this.iCalLib.SearchAlarm( 2001,9,22,11,30 );
//    alert( "Result : " + result );
}

function TestDeleteEvent( id )
{
    netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
    var iCalLibComponent = Components.classes["@mozilla.org/ical;1"].createInstance();
    
    this.iCalLib = iCalLibComponent.QueryInterface(Components.interfaces.oeIICal);
    
    this.iCalLib.SetServer( "/tmp/.oecalendar" );
    
    iCalLib.deleteEvent( id );

    var iCalEvent = iCalLib.fetchEvent( id );

    if( iCalEvent != null )
       alert( "Delete failed" );
}

function TestRecurring() {
   netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
   var iCalLibComponent = Components.classes["@mozilla.org/ical;1"].createInstance();

   this.iCalLib = iCalLibComponent.QueryInterface(Components.interfaces.oeIICal);

   var iCalEventComponent = Components.classes["@mozilla.org/icalevent;1"].createInstance();

   this.iCalEvent = iCalEventComponent.QueryInterface(Components.interfaces.oeIICalEvent);

   this.iCalLib.SetServer( "/tmp/.oecalendar" );

   iCalEvent.allDay = true;
   iCalEvent.recur = true;
   iCalEvent.recurInterval = 1;
   iCalEvent.recurUnits = "years";
   iCalEvent.recurForever = true;

   iCalEvent.start.year = 2001;
   iCalEvent.start.month = 0;
   iCalEvent.start.day = 1;
   iCalEvent.start.hour = 0;
   iCalEvent.start.minute = 0;

   iCalEvent.end.year = 2001;
   iCalEvent.end.month = 0;
   iCalEvent.end.day = 1;
   iCalEvent.end.hour = 23;
   iCalEvent.end.minute = 59;

   this.iCalLib.addEvent( iCalEvent );

   var displayDates =  new Object();
   var checkdate = new Date( 2002, 0, 1, 0, 0, 0 );
   var eventList = gICalLib.GetEventsForDay( checkdate, displayDates );

   if( !eventList.hasMoreElements() )
      alert( "Yearly Recur Test Failed" );
   
   var displayDate = new Date( displayDates.value.getNext().QueryInterface(Components.interfaces.nsISupportsPRTime).data );
}
