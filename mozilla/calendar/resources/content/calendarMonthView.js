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
 *                 Karl Guertin <grayrest@grayrest.com> 
 *                 Colin Phillips <colinp@oeone.com> 
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

/*-----------------------------------------------------------------
*   MonthView Class  subclass of CalendarView
*
*  Calendar month view class
*
* PROPERTIES
*     selectedEventBox     - Events are displayed in dynamically created event boxes
*                            this is the selected box, or null
*   
*     showingLastDay       - When the user changes to a new month we select
*                            the same day in the new month that was selected in the original month. If the
*                            new month does not have that day ( i.e. 31 was selected and the new month has 
*                            only 30 days ) we move the selection to the last day. When this happens we turn on
*                            the 'showingLastDay' flag. Now we will always select the last day when the month
*                            is changed so that if they go back to the original month, 31 is selected again.
*                            'showingLastDay' is turned off when the user selects a new day or changes the view.
*                       
*                            
*     dayNumberItemArray   - An array [ 0 to 41 ] of text boxes that hold the day numbers in the month view.
*                            We set the value attribute to the day number, or "" for boxes that are not in the month.
*                            In the XUL they have id's of the form  month-week-<row_number>-day-<column_number>
*                            where row_number is 1 - 6 and column_number is 1 - 7.
*
*     dayBoxItemArray      - An array [ 0 to 41 ] of  boxes, one for each day in the month view. These boxes
*                            are selected when a day is selected. They contain a dayNumberItem and event boxes.
*                            In the XUL they have id's of the form  month-week-<row_number>-day-<column_number>-box
*                            where row_number is 1 - 6 and column_number is 1 - 7.
*
*    dayBoxItemByDateArray - This array is reconstructed whenever the month changes ( and probably more
*                            often than that ) It contains day box items, just like the dayBoxItemArray above,
*                            except this array contains only those boxes that belong to the current month
*                            and is indexed by date. So for a 30 day month that starts on a Wednesday, 
*                            dayBoxItemByDateArray[0]  === dayBoxItemArray[3] and 
*                            dayBoxItemByDateArray[29] === dayBoxItemArray[36]
*
*    kungFooDeathGripOnEventBoxes - This is to keep the event box javascript objects around so when we get 
*                                   them back they still have the calendar event property on them.
* 
*
*    NOTES
*
*       Events are displayed in dynamically created event boxes. these boxes have a property added to them
*       called "calendarEvent" which contains the event represented by the box. 
*
*       There is one day box item for every day box in the month grid. These have an attribute
*       called "empty" which is set to "true", like so: 
*
*                                   dayBoxItem.setAttribute( "empty" , "true" );
*
*       when the day box is not in the month. This allows the display to be controlled from css.
*
*       The day boxes also have a couple of properties added to them:
*
*            dayBoxItem.dayNumber  - null when day is not in month.
*                                  - the date, 1 to 31, otherwise,
*           
*            dayBoxItem.numEvents  - The number of events for the day, used to limit the number displayed
*                                    since there is only room for 3.
* 
*/

// Make MonthView inherit from CalendarView

MonthView.prototype = new CalendarView();
MonthView.prototype.constructor = MonthView;


/**
*   MonthView Constructor.
* 
* PARAMETERS
*      calendarWindow     - the owning instance of CalendarWindow.
*
*/

function MonthView( calendarWindow )
{
   // call the super constructor
   
   this.superConstructor( calendarWindow );
   
   this.numberOfEventsToShow = false;

   var monthViewEventSelectionObserver = 
   {
      onSelectionChanged : function( EventSelectionArray )
      {
         
         dump( "\nIn Month view, on selection changed");
         if( EventSelectionArray.length > 0 )
         {
            //if there are selected events.
            
            //for some reason, this function causes the tree to go into a select / unselect loop
            //putting it in a settimeout fixes this.
            setTimeout( "gCalendarWindow.monthView.clearSelectedDate();", 1 );

            gCalendarWindow.monthView.clearSelectedBoxes();
            
            //dump( "\nIn Month view, eventSelectionArray.length is "+EventSelectionArray.length );
            var i = 0;
            
            for( i = 0; i < EventSelectionArray.length; i++ )
            {
               //dump( "\nin Month view, going to try and get the event boxes with name 'month-view-event-box-"+EventSelectionArray[i].id+"'" );
               var EventBoxes = document.getElementsByAttribute( "name", "month-view-event-box-"+EventSelectionArray[i].id );
               //dump( "\nIn Month view, found "+EventBoxes.length+" matches for the selected event." );
               for ( j = 0; j < EventBoxes.length; j++ ) 
               {
                  EventBoxes[j].setAttribute( "eventselected", "true" );
               }
            }
            //dump( "\nAll Done in Selection for Month View" );
         }
         else
         {
            //select the proper day
            gCalendarWindow.monthView.hiliteSelectedDate();
         }
      }
   }
      
   calendarWindow.EventSelection.addObserver( monthViewEventSelectionObserver );
   
   this.showingLastDay = false;
  
   // set up month day box's and day number text items, see notes above
   
   this.dayNumberItemArray = new Array();
   this.dayBoxItemArray = new Array();
   this.kungFooDeathGripOnEventBoxes = new Array();
   this.dayBoxItemByDateArray = new Array();
   
   var dayItemIndex = 0;
   
   for( var weekIndex = 1; weekIndex <= 6; ++weekIndex )
   {
      for( var dayIndex = 1; dayIndex <= 7; ++dayIndex )
      {
         // add the day text item to an array[0..41]
         
         var dayNumberItem = document.getElementById( "month-week-" + weekIndex + "-day-" + dayIndex );
         this.dayNumberItemArray[ dayItemIndex ] = dayNumberItem;
         
         // add the day box to an array[0..41]
         
         var dayBoxItem = document.getElementById( "month-week-" + weekIndex + "-day-" + dayIndex + "-box" );
         this.dayBoxItemArray[ dayItemIndex ] = dayBoxItem;
         
         // set on click of day boxes
         
         dayBoxItem.setAttribute( "onclick", "gCalendarWindow.monthView.clickDay( event )" );
         dayBoxItem.setAttribute( "oncontextmenu", "gCalendarWindow.monthView.contextClickDay( event )" );

         //set the drop
         dayBoxItem.setAttribute( "ondragdrop", "nsDragAndDrop.drop(event,monthViewEventDragAndDropObserver)" );
         dayBoxItem.setAttribute( "ondragover", "nsDragAndDrop.dragOver(event,monthViewEventDragAndDropObserver)" );
         
         //set the double click of day boxes
         dayBoxItem.setAttribute( "ondblclick", "gCalendarWindow.monthView.doubleClickDay( event )" );

         // array index
         
         ++dayItemIndex;
      }
   }
}

/** PUBLIC
*
*   Redraw the events for the current month
* 
*   We create XUL boxes dynamically and insert them into the XUL. 
*   To refresh the display we remove all the old boxes and make new ones.
*/
MonthView.prototype.refreshEvents = function monthView_refreshEvents( )
{
   // get this month's events and display them
  
   var monthEventList = this.calendarWindow.eventSource.getEventsForMonth( this.calendarWindow.getSelectedDate() );
   
   // remove old event boxes
   
   var eventBoxList = document.getElementsByAttribute( "eventbox", "monthview" );

   var eventBox = null;

   for( var eventBoxIndex = 0;  eventBoxIndex < eventBoxList.length; ++eventBoxIndex )
   {
      eventBox = eventBoxList[ eventBoxIndex ];
      
      eventBox.parentNode.removeChild( eventBox );
   }
   
   // clear calendarEvent counts, we only display 3 events per day 
   // count them by adding a property to the dayItem, which is zeroed here
      
   for( var dayItemIndex = 0; dayItemIndex < this.dayBoxItemArray.length; ++dayItemIndex )
   {
      var dayItem = this.dayBoxItemArray[ dayItemIndex ];
      
      dayItem.numEvents = 0;
   }  
   
   this.kungFooDeathGripOnEventBoxes = new Array();
   
   // add each calendarEvent
   for( var eventIndex = 0; eventIndex < monthEventList.length; ++eventIndex )
   {
      var calendarEventDisplay = monthEventList[ eventIndex ];
      
      var eventDate = calendarEventDisplay.displayDate;
      // get the day box for the calendarEvent's day
      var eventDayInMonth = eventDate.getDate();
      
      var dayBoxItem = this.dayBoxItemByDateArray[ eventDayInMonth ];
            
      if( !dayBoxItem )
         break;

      // Display no more than three, show dots for the events > 3
      
      dayBoxItem.numEvents +=  1;
      
      if( this.numberOfEventsToShow == false && dayBoxItem.numEvents > 1 )
      {
         this.setNumberOfEventsToShow();
      }

      if( dayBoxItem.numEvents == 1 || dayBoxItem.numEvents < this.numberOfEventsToShow )
      {
         // Make a box item to hold the event
         eventBox = document.createElement( "box" );
         eventBox.setAttribute( "id", "month-view-event-box-"+calendarEventDisplay.event.id );
         eventBox.setAttribute( "name", "month-view-event-box-"+calendarEventDisplay.event.id );
         eventBox.setAttribute( "event"+calendarEventDisplay.event.id, true );
         eventBox.setAttribute( "class", "month-day-event-box-class" );
         if( calendarEventDisplay.event.categories && calendarEventDisplay.event.categories != "" )
         {
            eventBox.setAttribute( calendarEventDisplay.event.categories, "true" );
         }
            
         eventBox.setAttribute( "eventbox", "monthview" );
         eventBox.setAttribute( "onclick", "monthEventBoxClickEvent( this, event )" );
         eventBox.setAttribute( "ondblclick", "monthEventBoxDoubleClickEvent( this, event )" );
         eventBox.setAttribute( "onmouseover", "gCalendarWindow.changeMouseOverInfo( calendarEventDisplay, event )" );
         eventBox.setAttribute( "tooltip", "savetip" );
         eventBox.setAttribute( "ondraggesture", "nsDragAndDrop.startDrag(event,monthViewEventDragAndDropObserver);" );
         // add a property to the event box that holds the calendarEvent that the
         // box represents

         eventBox.calendarEventDisplay = calendarEventDisplay;
         
         this.kungFooDeathGripOnEventBoxes.push( eventBox );
         
         // Make a text item to show the event title
         
         var eventBoxText = document.createElement( "label" );
         eventBoxText.setAttribute( "crop", "end" );
         eventBoxText.setAttribute( "class", "month-day-event-text-class" );
         eventBoxText.setAttribute( "value", calendarEventDisplay.event.title );
         //you need this flex in order for text to crop
         eventBoxText.setAttribute( "flex", "1" );
         eventBoxText.setAttribute( "ondraggesture", "nsDragAndDrop.startDrag(event,monthViewEventDragAndDropObserver);" );

         // add the text to the event box and the event box to the day box
         
         eventBox.appendChild( eventBoxText );        
         
         dayBoxItem.appendChild( eventBox );
      }
      else
      {
         //if there is not a box to hold the little dots for this day...
         if ( !document.getElementById( "dotboxholder"+calendarEventDisplay.event.start.day ) )
         {
            //make one
            dotBoxHolder = document.createElement( "hbox" );
            
            dotBoxHolder.setAttribute( "id", "dotboxholder"+calendarEventDisplay.event.start.day );
            
            dotBoxHolder.setAttribute( "eventbox", "monthview" );
                        
            //add the box to the day.
            dayBoxItem.appendChild( dotBoxHolder );
         }
         else
         {
            //otherwise, get the box
            
            dotBoxHolder = document.getElementById( "dotboxholder"+calendarEventDisplay.event.start.day );

         }
         
         if( dotBoxHolder.childNodes.length < kMAX_NUMBER_OF_DOTS_IN_MONTH_VIEW )
         {
            eventDotBox = document.createElement( "box" );
            eventDotBox.setAttribute( "eventbox", "monthview" );
            
            //show a dot representing an event.
            
            //NOTE: This variable is named eventBox because it needs the same name as 
            // the regular boxes, for the next part of the function!
            
            eventBox = document.createElement( "image" );
            
            eventBox.setAttribute( "class", "month-view-event-dot-class" );
            
            eventBox.setAttribute( "id", "month-view-event-box-"+calendarEventDisplay.event.id );
            eventBox.setAttribute( "name", "month-view-event-box-"+calendarEventDisplay.event.id );
            
            eventBox.calendarEventDisplay = calendarEventDisplay;
            
            this.kungFooDeathGripOnEventBoxes.push( eventBox );
            
            eventBox.setAttribute( "onmouseover", "gCalendarWindow.changeMouseOverInfo( calendarEventDisplay, event )" );
            eventBox.setAttribute( "onclick", "monthEventBoxClickEvent( this, event )" );
            eventBox.setAttribute( "ondblclick", "monthEventBoxDoubleClickEvent( this, event )" );
   
            eventBox.setAttribute( "tooltip", "savetip" );
   
            //add the dot to the extra box.
            eventDotBox.appendChild( eventBox );
            dotBoxHolder.appendChild( eventDotBox );
         }
         
         
      }

      // mark the box as selected, if the event is
         
      if( this.calendarWindow.EventSelection.isSelectedEvent( calendarEventDisplay.event ) )
      {
         this.selectBoxForEvent( calendarEventDisplay.event ); 
      } 
   }
}


/** PUBLIC
*
*   Called when the user switches to a different view
*/

MonthView.prototype.switchFrom = function monthView_switchFrom( )
{
   
}


/** PUBLIC
*
*   Called when the user switches to the month view
*/

MonthView.prototype.switchTo = function monthView_switchTo( )
{
   // see showingLastDay notes above
   
   this.showingLastDay = false;
      
   // disable/enable view switching buttons   
   
   var weekViewButton = document.getElementById( "week_view_command" );
   var monthViewButton = document.getElementById( "month_view_command" );
   var dayViewButton = document.getElementById( "day_view_command" );
   
   monthViewButton.setAttribute( "disabled", "true" );
   weekViewButton.removeAttribute( "disabled" );
   dayViewButton.removeAttribute( "disabled" );

   // switch views in the deck
   
   var calendarDeckItem = document.getElementById( "calendar-deck" );
   calendarDeckItem.selectedIndex = 0;
}


/** PUBLIC
*
*   Redraw the display, but not the events
*/

MonthView.prototype.refreshDisplay = function monthView_refreshDisplay( )
{ 
   // set the month/year in the header
   
   var selectedDate = this.calendarWindow.getSelectedDate();
   var newMonth = selectedDate.getMonth();
   var newYear =  selectedDate.getFullYear();
   var titleMonthArray = new Array();
   var titleYearArray = new Array();
   var toDebug = "";
   for (var i=-2; i < 3; i++){
      titleMonthArray[i] = newMonth + i;
      titleMonthArray[i] = (titleMonthArray[i] >= 0)? titleMonthArray[i] % 12 : titleMonthArray[i] + 12;
      titleMonthArray[i] = this.calendarWindow.dateFormater.getMonthName( titleMonthArray[i] );
      var idName = i + "-month-title";
      document.getElementById( idName ).setAttribute( "value" , titleMonthArray[i] );
   }
	document.getElementById( "0-year-title" ).setAttribute( "value" , newYear );
   
   var Offset = getIntPref(this.calendarWindow.calendarPreferences.calendarPref, "week.start", 0 );
   
   NewArrayOfDayNames = new Array();
   
   for( i = 0; i < ArrayOfDayNames.length; i++ )
   {
      NewArrayOfDayNames[i] = ArrayOfDayNames[i];
   }

   for( i = 0; i < Offset; i++ )
   {
      var FirstElement = NewArrayOfDayNames.shift();

      NewArrayOfDayNames.push( FirstElement );
   }

   //set the day names 
   for( i = 1; i <= 7; i++ )
   {
      document.getElementById( "month-view-header-day-"+i ).value = NewArrayOfDayNames[ (i-1) ];
   }
   

   // Write in all the day numbers and create the dayBoxItemByDateArray, see notes above
   
   // figure out first and last days of the month
   
   var firstDate = new Date( newYear, newMonth, 1 );
   var firstDayOfWeek = firstDate.getDay() - Offset;
   if( firstDayOfWeek < 0 )
      firstDayOfWeek+=7;

   var lastDayOfMonth = DateUtils.getLastDayOfMonth( newYear, newMonth );
   
   // prepare the dayBoxItemByDateArray, we will be filling this in
   
   this.dayBoxItemByDateArray = new Array();
   
   // loop through all the day boxes
   
   var dayNumber = 1;
   
   for( var dayIndex = 0; dayIndex < this.dayNumberItemArray.length; ++dayIndex )
   {
      var dayNumberItem = this.dayNumberItemArray[ dayIndex ];
      var dayBoxItem = this.dayBoxItemArray[ dayIndex ];
      
      if( dayIndex < firstDayOfWeek || dayNumber > lastDayOfMonth )
      {
         // this day box is NOT in the month, 
         dayBoxItem.dayNumber = null;

         dayBoxItem.setAttribute( "empty" , "true" );  
         dayBoxItem.removeAttribute( "weekend" );
         
         if( dayIndex < firstDayOfWeek )
         {
            var thisDate = new Date( newYear, newMonth, 1-(firstDayOfWeek - dayIndex ) );
            
            dayBoxItem.date = thisDate;
            
            dayNumberItem.setAttribute( "value" , thisDate.getDate() );  

         }
         else
         {
            var thisDate = new Date( newYear, newMonth, lastDayOfMonth+( dayIndex - lastDayOfMonth - firstDayOfWeek + 1 ) );
            
            dayBoxItem.date = thisDate;
            dayBoxItem.setAttribute( "date", thisDate );

            dayNumberItem.setAttribute( "value" , thisDate.getDate() );  
         }
      }  
      else
      {
         dayNumberItem.setAttribute( "value" , dayNumber );
         
         dayBoxItem.removeAttribute( "empty" ); 
         var thisDate = new Date( newYear, newMonth, dayNumber );
         if( thisDate.getDay() == 0 | thisDate.getDay() == 6 )
         {
            dayBoxItem.setAttribute( "weekend", "true" );
         }
         else
            dayBoxItem.removeAttribute( "weekend" ); 

         dayBoxItem.dayNumber = dayNumber;
         
         this.dayBoxItemByDateArray[ dayNumber ] = dayBoxItem; 
         ++dayNumber;  
      }
   }
  
   // if we aren't showing an event, highlite the selected date.
   if ( this.calendarWindow.EventSelection.selectedEvents.length < 1 ) 
   {
      this.hiliteSelectedDate( );
   }
   
   //always highlight today's date.
   this.hiliteTodaysDate( );
}


/** PRIVATE
*
*   Mark the selected date, also unmark the old selection if there was one
*/

MonthView.prototype.hiliteSelectedDate = function monthView_hiliteSelectedDate( )
{
   // Clear the old selection if there was one

   this.clearSelectedDate();

   this.clearSelectedBoxes();

   // Set the background for selection
   
   var ThisBox = this.dayBoxItemByDateArray[ this.calendarWindow.getSelectedDate().getDate() ];
   
   if( ThisBox )
      ThisBox.setAttribute( "monthselected" , "true" );
}


/** PUBLIC
*
*  Unmark the selected date if there is one.
*/

MonthView.prototype.clearSelectedDate = function monthView_clearSelectedDate( )
{
   var SelectedBoxes = document.getElementsByAttribute( "monthselected", "true" );
   
   for( i = 0; i < SelectedBoxes.length; i++ )
   {
      SelectedBoxes[i].removeAttribute( "monthselected" );
   }
}

/** PUBLIC
*
*  Unmark the selected date if there is one.
*/

MonthView.prototype.clearSelectedBoxes = function monthView_clearSelectedBoxes( )
{
   var SelectedBoxes = document.getElementsByAttribute( "eventselected", "true" );
   
   for( i = 0; i < SelectedBoxes.length; i++ )
   {
      SelectedBoxes[i].removeAttribute( "eventselected" );
   }
}

/** PRIVATE
*
*  Mark today as selected, also unmark the old today if there was one.
*/

MonthView.prototype.hiliteTodaysDate = function monthView_hiliteTodaysDate( )
{
   var Month = this.calendarWindow.getSelectedDate().getMonth();
   
   var Year = this.calendarWindow.getSelectedDate().getFullYear();

   // Clear the old selection if there was one
   var TodayBox = document.getElementsByAttribute( "today", "true" );
   
   for( i = 0; i < TodayBox.length; i++ )
   {
      TodayBox[i].removeAttribute( "today" );
   }

   //highlight today.
   var Today = new Date( );
   if ( Year == Today.getFullYear() && Month == Today.getMonth() ) 
   {
      var ThisBox = this.dayBoxItemByDateArray[ Today.getDate() ];
      if( ThisBox )
         ThisBox.setAttribute( "today", "true" );
   }
}


/** PUBLIC
*
*   This is called when we are about the make a new event
*   and we want to know what the default start date should be for the event.
*/

MonthView.prototype.getNewEventDate = function monthView_getNewEventDate( )
{
   // use the selected year, month and day
   // and the current hours and minutes
   
   var now = new Date();
   var start = new Date( this.calendarWindow.getSelectedDate() );
   
   start.setHours( now.getHours() );
   start.setMinutes( Math.ceil( now.getMinutes() / 5 ) * 5 );
   start.setSeconds( 0 );
   
   return start;     
}


/** PUBLIC
*
*   Moves goMonths months in the future, goes to next month if no argument.
*/

MonthView.prototype.goToNext = function monthView_goToNext( goMonths )
{  
   if(goMonths){
      var nextMonth = new Date(  this.calendarWindow.selectedDate.getFullYear(),  this.calendarWindow.selectedDate.getMonth() + goMonths, 1 );
      this.adjustNewMonth( nextMonth );  
      this.goToDay( nextMonth );
   }else{
      var nextMonth = new Date(  this.calendarWindow.selectedDate.getFullYear(),  this.calendarWindow.selectedDate.getMonth() + 1, 1 );
      this.adjustNewMonth( nextMonth );  
      this.goToDay( nextMonth );
   }
}


/** PUBLIC
*
*   Goes goMonths months into the past, goes to the previous month if no argument.
*/

MonthView.prototype.goToPrevious = function monthView_goToPrevious( goMonths )
{
   if(goMonths){
      var prevMonth = new Date(  this.calendarWindow.selectedDate.getFullYear(),  this.calendarWindow.selectedDate.getMonth() - goMonths, 1 );
      this.adjustNewMonth( prevMonth );  
      this.goToDay( prevMonth );
   }else{
      var prevMonth = new Date(  this.calendarWindow.selectedDate.getFullYear(),  this.calendarWindow.selectedDate.getMonth() - 1, 1 );
      this.adjustNewMonth( prevMonth );  
      this.goToDay( prevMonth );
   }
   
}

/** PRIVATE
*
*   Helper function for goToNext and goToPrevious
*
*   When the user changes to a new month the new month may not have the selected day in it.
*   ( i.e. 31 was selected and the new month has only 30 days ). 
*   In that case our addition, or subtraction, in  goToNext or goToPrevious will cause the
*   date to jump a month. ( Say the date starts at May 31, we add 1 to the month, Now the
*   date would be June 31, but the Date object knows there is no June 31, so it sets itself
*   to July 1. )
*
*   In goToNext or goToPrevious we set the date to be 1, so the month will be correct. Here
*   we set the date to be the selected date, making adjustments if the selected date is not in the month.
*/

MonthView.prototype.adjustNewMonth = function monthView_adjustNewMonth( newMonth )
{
   // Don't let a date beyond the end of the month make us jump
   // too many or too few months
  
   var lastDayOfMonth = DateUtils.getLastDayOfMonth( newMonth.getFullYear(), newMonth.getMonth() );
   
   if( this.calendarWindow.selectedDate.getDate() > lastDayOfMonth )
   {
      // The selected date is NOT in the month
      // set it to the last day of the month and turn on showingLastDay, see notes in MonthView class
      
      newMonth.setDate( lastDayOfMonth )
      
      this.showingLastDay = true;
   }
   else if( this.showingLastDay )
   {
      // showingLastDay is on so select the last day of the month, see notes in MonthView class
      
      newMonth.setDate( lastDayOfMonth )
   }
   else
   {
      // date is NOT beyond the last.
      
      newMonth.setDate(  this.calendarWindow.selectedDate.getDate() )
   }
}




/** PUBLIC  -- monthview only
*
*   Called when a day box item is single clicked
*/

MonthView.prototype.clickDay = function monthView_clickDay( event )
{
   if( event.button > 0 )
      return;
  
   var dayBoxItem = event.currentTarget;
   
   if( dayBoxItem.dayNumber != null && event.detail == 1 )
   {
      // turn off showingLastDay - see notes in MonthView class
      this.showingLastDay = false;
   
      // change the selected date and redraw it
      var newDate = this.calendarWindow.getSelectedDate();

      newDate.setDate( dayBoxItem.dayNumber );
      
      this.calendarWindow.setSelectedDate( newDate );

      //changing the selection will redraw the day as selected (colored blue) in the month view.
      //therefor, this has to happen after setSelectedDate
      gCalendarWindow.EventSelection.emptySelection();
   }
}

/** PUBLIC  -- monthview only
*
*   Called when a day box item is single clicked
*/
MonthView.prototype.contextClickDay = function monthView_contextClickDay( event )
{
   var dayBoxItem = event.currentTarget;
   
   if( dayBoxItem.dayNumber != null )
   {
      // turn off showingLastDay - see notes in MonthView class
      
      this.showingLastDay = false;
   
      // change the selected date and redraw it
      
      gNewDateVariable = gCalendarWindow.getSelectedDate();

      gNewDateVariable.setDate( dayBoxItem.dayNumber );
   }
}

/*
** Don't forget that clickDay gets called before double click day gets called
*/

MonthView.prototype.doubleClickDay = function monthView_doubleClickDay( event )
{
   if( event.button > 0 )
      return;
   
   if ( event.currentTarget.dayNumber != null ) 
   {
      // change the selected date and redraw it

      var startDate = this.getNewEventDate();
      
      newEvent( startDate, false );

   }
   else
   {
      newEvent( dayBoxItem.date, false );
   }
}


MonthView.prototype.clearSelectedEvent = function monthView_clearSelectedEvent( )
{
   var ArrayOfBoxes = document.getElementsByAttribute( "eventselected", "true" );

   for( i = 0; i < ArrayOfBoxes.length; i++ )
   {
      ArrayOfBoxes[i].removeAttribute( "eventselected" );   
   }
}


MonthView.prototype.getVisibleEvent = function monthView_getVisibleEvent( calendarEvent )
{
   var eventBox = document.getElementById( "month-view-event-box-"+calendarEvent.id );

   if ( eventBox ) 
   {
      return eventBox;
   }
   else
      return null;
}

MonthView.prototype.selectBoxForEvent = function monthView_selectBoxForEvent( calendarEvent )
{
   var EventBoxes = document.getElementsByAttribute( "name", "month-view-event-box-"+calendarEvent.id );
            
   for ( j = 0; j < EventBoxes.length; j++ ) 
   {
      EventBoxes[j].setAttribute( "eventselected", "true" );
   }
}

/*Just calls setCalendarSize, it's here so it can be implemented on the other two views without difficulty.*/
MonthView.prototype.doResize = function monthView_doResize( )
{
   this.setCalendarSize(this.getViewHeight());

   this.setNumberOfEventsToShow();
}

/*Takes in a height, sets the calendar's container box to that height, the grid expands and contracts to fit it.*/
MonthView.prototype.setCalendarSize = function monthView_setCalendarSize( height )
{
    var offset = document.defaultView.getComputedStyle(document.getElementById("month-controls-box"), "").getPropertyValue("height");
    offset = parseInt( offset );
    height = (height-offset)+"px";
    document.getElementById( "month-content-box" ).setAttribute( "height", height );
} 

/*returns the height of the current view in pixels*/ 
MonthView.prototype.getViewHeight = function monthView_getViewHeight( )
{
    toReturn = document.defaultView.getComputedStyle(document.getElementById("month-view-box"), "").getPropertyValue("height");
    toReturn = parseInt( toReturn ); //strip off the px at the end
    return toReturn;
}


MonthView.prototype.setNumberOfEventsToShow = function monthView_getNumberOfEventsToShow( )
{
   //get the style height of the month view box.
   var MonthViewBoxHeight = document.defaultView.getComputedStyle(document.getElementById("month-week-4-day-4-box"), "").getPropertyValue("height");
   MonthViewBoxHeight = parseInt( MonthViewBoxHeight ); //strip off the px at the end
   
   //get the height of an event box.
   var Element = document.getElementsByAttribute( "eventbox", "monthview" )[0];
   if( !Element )
      return;

   var EventBoxHeight = document.defaultView.getComputedStyle( Element, "" ).getPropertyValue( "height" );
   EventBoxHeight = parseInt( EventBoxHeight ); //strip off the px at the end

   //calculate the number of events to show.
   dump( "\n\n"+( MonthViewBoxHeight - EventBoxHeight ) / EventBoxHeight );
   dump( "\n"+MonthViewBoxHeight );
   dump( "\n"+EventBoxHeight );
   this.numberOfEventsToShow = parseInt( ( MonthViewBoxHeight - EventBoxHeight ) / EventBoxHeight ); 
}


/*
drag and drop stuff 
*/
gEventBeingDragged = false;
gBoxBeingDroppedOn = false;

var monthViewEventDragAndDropObserver  = {
  onDragStart: function (evt, transferData, action){
      if( evt.target.calendarEventDisplay ) 
         gEventBeingDragged = evt.target.calendarEventDisplay.event;
      else if( evt.target.parentNode.calendarEventDisplay )
         gEventBeingDragged = evt.target.parentNode.calendarEventDisplay.event;

      //dump( "\nEvent being dragged is "+gEventBeingDragged );
      transferData.data=new TransferData();
      transferData.data.addDataForFlavour("text/unicode",0);
  },
  getSupportedFlavours : function () {
    var weekflavours = new FlavourSet();
    weekflavours.appendFlavour("text/unicode");
    return weekflavours;
  },
  onDragOver: function (evt,flavour,session){
    //dump( "on dragged over "+evt.target.getAttribute( "id" )+"\n" );
    gBoxBeingDroppedOn = document.getElementById( evt.target.getAttribute( "id" ) );
    //dump( evt.target.getAttribute( "id" ) );
  },
  onDrop: function (evt,dropdata,session){
      //get the date of the current event box.
      //dump( "\n\nDROP EVNET->\n"+gEventBeingDragged.start );
      var newDay = gBoxBeingDroppedOn.dayNumber;
      if( newDay == null )
         return;

      gEventBeingDragged.start.day = newDay;
      gEventBeingDragged.end.day = newDay;

      //edit the event being dragged to change its start and end date
      //don't change the start and end time though.
      gICalLib.modifyEvent( gEventBeingDragged, gEventBeingDragged.parent.server );
      
      //refresh the view
  }
};
