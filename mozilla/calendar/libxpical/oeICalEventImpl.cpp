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
 * Contributor(s): Mostafa Hosseini <mostafah@oeone.com>
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

#ifndef WIN32
#include <unistd.h>
#endif

#include "oeICalEventImpl.h"
#include "nsMemory.h"
//#include "stdlib.h"
#include "nsCOMPtr.h"

#define strcasecmp strcmp

#define RECUR_NONE 0
#define RECUR_DAILY 1
#define RECUR_WEEKLY 2
#define RECUR_MONTHLY_MDAY 3
#define RECUR_MONTHLY_WDAY 4
#define RECUR_YEARLY 5

char *EmptyReturn() {
    return (char*) nsMemory::Clone( "", 1 );
}

icaltimetype ConvertFromPrtime( PRTime indate ) {
    icaltimetype outdate;

    time_t seconds = indate /1000;
    struct tm t = *(localtime(&seconds));

    outdate.year = t.tm_year+ 1900;
    outdate.month = t.tm_mon + 1;
    outdate.day = t.tm_mday;
    outdate.hour = t.tm_hour;
    outdate.minute = t.tm_min;
    outdate.second = t.tm_sec;
    return outdate;
}


//////////////////////////////////////////////////
//   ICalEvent Factory
//////////////////////////////////////////////////

/* Implementation file */
NS_IMPL_ISUPPORTS1(oeICalEventImpl, oeIICalEvent)

nsresult
NS_NewICalEvent( oeIICalEvent** inst )
{
    NS_PRECONDITION(inst != nsnull, "null ptr");
    if (! inst)
        return NS_ERROR_NULL_POINTER;

    *inst = new oeICalEventImpl();
    if (! *inst)
        return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(*inst);
    return NS_OK;
}

oeICalEventImpl::oeICalEventImpl()
{
#ifdef ICAL_DEBUG
    printf( "oeICalEventImpl::oeICalEventImpl()\n" );
#endif
  NS_INIT_ISUPPORTS();
  /* member initializers and constructor code */
    nsresult rv;
	if( NS_FAILED( rv = NS_NewDateTime((oeIDateTime**) &m_start ))) {
        m_start = NULL;
	}
	if( NS_FAILED( rv = NS_NewDateTime((oeIDateTime**) &m_end ))) {
        m_end = NULL;
	}
	if( NS_FAILED( rv = NS_NewDateTime((oeIDateTime**) &m_recurend ))) {
        m_recurend = NULL;
	}
    m_id = 0;
    m_title = NULL;
    m_description = NULL;
    m_location = NULL;
    m_category = NULL;
    m_isprivate = true;
    m_syncid = 0;
    m_allday = false;
    m_hasalarm = false;
    m_alarmlength = 0;
    m_alarmemail = NULL;
    m_inviteemail = NULL;
    m_recurinterval = 1;
    m_recur = false;
    m_recurforever = true;
    m_alarmunits = NULL;
    m_recurunits = NULL;
    m_recurweekdays = 0;
    m_recurweeknumber = 0;
    m_lastalarmack = icaltime_null_time();
    SetAlarmUnits( "minutes" );
    SetRecurUnits( "weeks" );
}

oeICalEventImpl::~oeICalEventImpl()
{
#ifdef ICAL_DEBUG
    printf( "oeICalEventImpl::~oeICalEventImpl( %d )\n", mRefCnt );
#endif
  /* destructor code */
  if( m_title )
      nsMemory::Free( m_title );
  if( m_description )
      nsMemory::Free( m_description );
  if( m_location )
      nsMemory::Free( m_location );
  if( m_category )
      nsMemory::Free( m_category );
  if( m_alarmunits )
      nsMemory::Free( m_alarmunits );
  if( m_alarmemail  )
      nsMemory::Free( m_alarmemail );
  if( m_inviteemail  )
      nsMemory::Free( m_inviteemail );
  if( m_recurunits  )
      nsMemory::Free( m_recurunits );
  
  if( m_start )
      m_start->Release();
  if( m_end )
      m_end->Release();
  if( m_recurend )
    m_recurend->Release();
}

/* attribute long Id; */
NS_IMETHODIMP oeICalEventImpl::GetId(PRUint32 *aRetVal)
{
#ifdef ICAL_DEBUG_ALL
    printf( "GetId() = " );
#endif
    *aRetVal= m_id;
#ifdef ICAL_DEBUG_ALL
    printf( "%lu\n", *aRetVal );
#endif
    return NS_OK;
}

NS_IMETHODIMP oeICalEventImpl::SetId(PRUint32 aNewVal)
{
#ifdef ICAL_DEBUG_ALL
    printf( "SetId( %lu )\n", aNewVal );
#endif
    m_id = aNewVal;
    return NS_OK;
}

/* attribute string Title; */
NS_IMETHODIMP oeICalEventImpl::GetTitle(char **aRetVal)
{
#ifdef ICAL_DEBUG_ALL
    printf( "GetTitle() = " );
#endif
    
    if( m_title ) {
        *aRetVal= (char*) nsMemory::Clone( m_title, strlen(m_title)+1);
        if( *aRetVal == NULL )
            return  NS_ERROR_OUT_OF_MEMORY;
    } else
        *aRetVal= EmptyReturn();

#ifdef ICAL_DEBUG_ALL
    printf( "\"%s\"\n", *aRetVal );
#endif
    return NS_OK;
}

NS_IMETHODIMP oeICalEventImpl::SetTitle(const char * aNewVal)
{
#ifdef ICAL_DEBUG_ALL
    printf( "SetTitle( %s )\n", aNewVal );
#endif
    
    if( m_title )
        nsMemory::Free( m_title );
    
    if( aNewVal )
        m_title= (char*) nsMemory::Clone( aNewVal, strlen(aNewVal)+1);
    else
        m_title = NULL;

    return NS_OK;
}

/* attribute string Description; */
NS_IMETHODIMP oeICalEventImpl::GetDescription(char * *aRetVal)
{
#ifdef ICAL_DEBUG_ALL
    printf( "GetDescription() = " );
#endif
    if( m_description ) {
        *aRetVal= (char*) nsMemory::Clone( m_description , strlen(m_description)+1);
        if( *aRetVal == NULL )
            return  NS_ERROR_OUT_OF_MEMORY;
    } else
        *aRetVal= EmptyReturn();
    
#ifdef ICAL_DEBUG_ALL
    printf( "\"%s\"\n", *aRetVal );
#endif
    return NS_OK;
}

NS_IMETHODIMP oeICalEventImpl::SetDescription(const char * aNewVal)
{
#ifdef ICAL_DEBUG_ALL
    printf( "SetDescription( %s )\n", aNewVal );
#endif

    if( m_description )
        nsMemory::Free( m_description );
    
    if( aNewVal )
        m_description= (char*) nsMemory::Clone( aNewVal, strlen(aNewVal)+1);
    else
        m_description = NULL;
    
    return NS_OK;
}

/* attribute string Location; */
NS_IMETHODIMP oeICalEventImpl::GetLocation(char **aRetVal)
{
#ifdef ICAL_DEBUG_ALL
    printf( "GetLocation() = " );
#endif

    if( m_location ) {
        *aRetVal= (char*) nsMemory::Clone( m_location , strlen(m_location)+1);
        if( *aRetVal == NULL )
            return  NS_ERROR_OUT_OF_MEMORY;
    } else
        *aRetVal= EmptyReturn();

#ifdef ICAL_DEBUG_ALL
    printf( "\"%s\"\n", *aRetVal );
#endif
   return NS_OK;
}
NS_IMETHODIMP oeICalEventImpl::SetLocation(const char * aNewVal)
{
#ifdef ICAL_DEBUG_ALL
    printf( "SetLocation( %s )\n", aNewVal );
#endif

    if( m_location )
        nsMemory::Free( m_location );
    
    if( aNewVal )
        m_location= (char*) nsMemory::Clone( aNewVal, strlen(aNewVal)+1);
    else
        m_location = NULL;

    return NS_OK;
}

/* attribute string Category; */
NS_IMETHODIMP oeICalEventImpl::GetCategory(char **aRetVal)
{
#ifdef ICAL_DEBUG_ALL
    printf( "GetCategory() = " );
#endif
    
    if( m_category ) {
        *aRetVal= (char*) nsMemory::Clone( m_category , strlen(m_category)+1);
        if( *aRetVal == NULL )
            return  NS_ERROR_OUT_OF_MEMORY;
    } else
        *aRetVal= EmptyReturn();

#ifdef ICAL_DEBUG_ALL
    printf( "\"%s\"\n", *aRetVal );
#endif
   return NS_OK;
}

NS_IMETHODIMP oeICalEventImpl::SetCategory(const char * aNewVal)
{
#ifdef ICAL_DEBUG_ALL
    printf( "SetCategory( %s )\n", aNewVal );
#endif
    if( m_category )
        nsMemory::Free( m_category );
    
    if( aNewVal )
        m_category= (char*) nsMemory::Clone( aNewVal, strlen(aNewVal)+1);
    else
        m_category = NULL;
    return NS_OK;
}

/* attribute boolean PrivateEvent; */
NS_IMETHODIMP oeICalEventImpl::GetPrivateEvent(PRBool *aRetVal)
{
#ifdef ICAL_DEBUG_ALL
    printf( "GetPrivateEvent( ) = " );
#endif

    *aRetVal = m_isprivate;

#ifdef ICAL_DEBUG_ALL
    printf( "%d\n", *aRetVal );
#endif
    return NS_OK;
}

NS_IMETHODIMP oeICalEventImpl::SetPrivateEvent(PRBool aNewVal)
{
#ifdef ICAL_DEBUG_ALL
    printf( "SetPrivateEvent( %d )\n", aNewVal );
#endif
    m_isprivate = aNewVal;
    return NS_OK;
}

/* attribute long SyncId; */
NS_IMETHODIMP oeICalEventImpl::GetSyncId(PRUint32 *aRetVal)
{
#ifdef ICAL_DEBUG_ALL
    printf( "GetSyncId()\n" );
#endif
    *aRetVal = m_syncid;
    return NS_OK;
}

NS_IMETHODIMP oeICalEventImpl::SetSyncId(PRUint32 aNewVal)
{
#ifdef ICAL_DEBUG_ALL
    printf( "SetSyncId(%lu)\n", aNewVal );
#endif
    m_syncid = aNewVal;
    return NS_OK;
}

/* attribute boolean AllDay; */
NS_IMETHODIMP oeICalEventImpl::GetAllDay(PRBool *aRetVal)
{
#ifdef ICAL_DEBUG_ALL
    printf( "GetAllDay()\n" );
#endif
    *aRetVal = m_allday;
    return NS_OK;
}
NS_IMETHODIMP oeICalEventImpl::SetAllDay(PRBool aNewVal)
{
#ifdef ICAL_DEBUG_ALL
    printf( "SetAllDay( %d )\n", aNewVal );
#endif
    m_allday = aNewVal;
    return NS_OK;
}

/* attribute boolean Alarm; */
NS_IMETHODIMP oeICalEventImpl::GetAlarm(PRBool *aRetVal)
{
#ifdef ICAL_DEBUG_ALL
    printf( "GetAlarm()\n" );
#endif
    *aRetVal = m_hasalarm;
    return NS_OK;
}

NS_IMETHODIMP oeICalEventImpl::SetAlarm(PRBool aNewVal)
{
#ifdef ICAL_DEBUG_ALL
    printf( "SetAlarm()\n" );
#endif
    m_hasalarm = aNewVal;
    return NS_OK;
}

/* attribute string AlarmUnits; */
NS_IMETHODIMP oeICalEventImpl::GetAlarmUnits(char * *aRetVal)
{
#ifdef ICAL_DEBUG_ALL
    printf( "GetAlarmUnits()\n" );
#endif
    if( m_alarmunits ) {
        *aRetVal= (char*) nsMemory::Clone( m_alarmunits, strlen(m_alarmunits)+1);
        if( *aRetVal == NULL )
            return  NS_ERROR_OUT_OF_MEMORY;
    } else
        *aRetVal= EmptyReturn();
   return NS_OK;
}

NS_IMETHODIMP oeICalEventImpl::SetAlarmUnits(const char * aNewVal)
{
#ifdef ICAL_DEBUG_ALL
    printf( "SetAlarmUnits( %s )\n", aNewVal );
#endif
    if( m_alarmunits )
        nsMemory::Free( m_alarmunits );
    
    if( aNewVal )
        m_alarmunits= (char*) nsMemory::Clone( aNewVal, strlen(aNewVal)+1);
    else
        m_alarmunits = NULL;
    return NS_OK;
}

/* attribute long AlarmLength; */
NS_IMETHODIMP oeICalEventImpl::GetAlarmLength(PRUint32 *aRetVal)
{
#ifdef ICAL_DEBUG_ALL
    printf( "GetAlarmLength()\n" );
#endif
    *aRetVal = m_alarmlength;
    return NS_OK;
}

NS_IMETHODIMP oeICalEventImpl::SetAlarmLength(PRUint32 aNewVal)
{
#ifdef ICAL_DEBUG_ALL
    printf( "SetAlarmLength(%lu)\n", aNewVal );
#endif
    m_alarmlength = aNewVal;
    return NS_OK;
}

/* attribute string AlarmEmailAddress; */
NS_IMETHODIMP oeICalEventImpl::GetAlarmEmailAddress(char * *aRetVal)
{
#ifdef ICAL_DEBUG_ALL
    printf( "GetAlarmEmailAddres()\n" );
#endif
    if( m_alarmemail ) {
        *aRetVal= (char*) nsMemory::Clone( m_alarmemail, strlen(m_alarmemail)+1);
        if( *aRetVal == NULL )
            return  NS_ERROR_OUT_OF_MEMORY;
    } else
        *aRetVal= EmptyReturn();
   return NS_OK;
}

NS_IMETHODIMP oeICalEventImpl::SetAlarmEmailAddress(const char * aNewVal)
{
#ifdef ICAL_DEBUG_ALL
    printf( "SetAlarmEmailAddres()\n" );
#endif
    if( m_alarmemail )
        nsMemory::Free( m_alarmemail );
    
    if( aNewVal )
        m_alarmemail= (char*) nsMemory::Clone( aNewVal, strlen(aNewVal)+1);
    else
        m_alarmemail = NULL;
    return NS_OK;
}

/* attribute string InviteEmailAddress; */
NS_IMETHODIMP oeICalEventImpl::GetInviteEmailAddress(char * *aRetVal)
{
#ifdef ICAL_DEBUG_ALL
    printf( "GetInviteEmailAddres()\n" );
#endif
    if( m_inviteemail ) {
        *aRetVal= (char*) nsMemory::Clone( m_inviteemail, strlen(m_inviteemail)+1);
        if( *aRetVal == NULL )
            return  NS_ERROR_OUT_OF_MEMORY;
    } else
        *aRetVal= EmptyReturn();
   return NS_OK;
}
NS_IMETHODIMP oeICalEventImpl::SetInviteEmailAddress(const char * aNewVal)
{
#ifdef ICAL_DEBUG_ALL
    printf( "SetInviteEmailAddres()\n" );
#endif
    if( m_inviteemail )
        nsMemory::Free( m_inviteemail );
    
    if( aNewVal )
        m_inviteemail= (char*) nsMemory::Clone( aNewVal, strlen(aNewVal)+1);
    else
        m_inviteemail = NULL;
    return NS_OK;
}

/* attribute boolean RecurInterval; */
NS_IMETHODIMP oeICalEventImpl::GetRecurInterval(PRUint32 *aRetVal)
{
#ifdef ICAL_DEBUG_ALL
    printf( "GetRecurInterval()\n" );
#endif
    *aRetVal = m_recurinterval;
    return NS_OK;
}
NS_IMETHODIMP oeICalEventImpl::SetRecurInterval(PRUint32 aNewVal )
{
#ifdef ICAL_DEBUG_ALL
    printf( "SetRecurInterval()\n" );
#endif
    m_recurinterval = aNewVal;
    return NS_OK;
}

/* attribute string RecurUnits; */

NS_IMETHODIMP oeICalEventImpl::GetRecurUnits(char **aRetVal)
{
#ifdef ICAL_DEBUG_ALL
    printf( "GetRecurUnits()\n" );
#endif
    if( m_recurunits ) {
        *aRetVal= (char*) nsMemory::Clone( m_recurunits, strlen(m_recurunits)+1);
        if( *aRetVal == NULL )
            return  NS_ERROR_OUT_OF_MEMORY;
    } else
        *aRetVal= EmptyReturn();
    return NS_OK;
}
NS_IMETHODIMP oeICalEventImpl::SetRecurUnits(const char * aNewVal)
{
#ifdef ICAL_DEBUG_ALL
    printf( "SetRecurUnits()\n" );
#endif
    if( m_recurunits )
        nsMemory::Free( m_recurunits );
    
    if( aNewVal )
        m_recurunits= (char*) nsMemory::Clone( aNewVal, strlen(aNewVal)+1);
    else
        m_recurunits = NULL;
    return NS_OK;
}

/* attribute boolean Recur; */

NS_IMETHODIMP oeICalEventImpl::GetRecur(PRBool *aRetVal)
{
#ifdef ICAL_DEBUG_ALL
    printf( "GetRecur()\n" );
#endif
    *aRetVal = m_recur;
    return NS_OK;
}
NS_IMETHODIMP oeICalEventImpl::SetRecur(PRBool aNewVal)
{
#ifdef ICAL_DEBUG_ALL
    printf( "SetRecur()\n" );
#endif
    m_recur = aNewVal;
    return NS_OK;
}

/* attribute boolean RecurForever; */

NS_IMETHODIMP oeICalEventImpl::GetRecurForever(PRBool *aRetVal)
{
#ifdef ICAL_DEBUG_ALL
    printf( "GetRecurForever()\n" );
#endif
    *aRetVal = m_recurforever;
    return NS_OK;
}
NS_IMETHODIMP oeICalEventImpl::SetRecurForever(PRBool aNewVal)
{
#ifdef ICAL_DEBUG_ALL
    printf( "SetRecurForever()\n" );
#endif
    m_recurforever = aNewVal;
    return NS_OK;
}

NS_IMETHODIMP oeICalEventImpl::GetLastAlarmAck(PRTime *aRetVal)
{
#ifdef ICAL_DEBUG_ALL
    printf( "GetLastAlarmAck()\n" );
#endif
    *aRetVal = icaltime_as_timet( m_lastalarmack );
    *aRetVal /= 1000;
    return NS_OK;
}
NS_IMETHODIMP oeICalEventImpl::SetLastAlarmAck(PRTime aNewVal)
{
#ifdef ICAL_DEBUG_ALL
    printf( "SetLastAlarmAck()\n" );
#endif

    m_lastalarmack = ConvertFromPrtime( aNewVal );
    return NS_OK;
}

NS_IMETHODIMP oeICalEventImpl::GetNextRecurrence( PRTime begin, PRTime *retval, PRBool *isvalid ) {
#ifdef ICAL_DEBUG_ALL
    printf( "GetNextRecurrence()\n" );
#endif
    //for non recurring events
    *isvalid = false;
    if( !m_recur ) {
        PRTime start;
        m_start->GetTime( &start );
        if( (start/1000) > (begin/1000) ) {
            *retval = start;
            *isvalid = true;
        }
        return NS_OK;
    }

    //for recurring events
    icalcomponent *vcalendar = AsIcalComponent();

    icalcomponent *vevent = icalcomponent_get_first_component( vcalendar, ICAL_VEVENT_COMPONENT );
    assert( vevent != 0);

    icalproperty *prop = icalcomponent_get_first_property( vevent, ICAL_RRULE_PROPERTY );
    if ( prop != 0) {
        struct icalrecurrencetype recur = icalproperty_get_rrule(prop);
//        printf("#### %s\n",icalrecurrencetype_as_string(&recur));
        icalrecur_iterator* ritr = icalrecur_iterator_new(recur,m_start->m_datetime);
        struct icaltimetype next;
        for(next = icalrecur_iterator_next(ritr);
            !icaltime_is_null_time(next);
            next = icalrecur_iterator_next(ritr)){

            next.is_date = false;
            next = icaltime_normalize( next );

//            printf( "recur: %d-%d-%d %d:%d:%d\n" , next.year, next.month, next.day, next.hour, next.minute, next.second );
            
            //quick fix for the recurrence getting out of the end of the month into the next month
            //like 31st of each month but when you get February
            if( recur.freq == ICAL_MONTHLY_RECURRENCE && !m_recurweeknumber && next.day != m_start->m_datetime.day) {
                printf( "Wrong day in month\n" );
                continue;
            }
            PRTime nextinms = icaltime_as_timet( next );
            nextinms *= 1000;
            if( (nextinms > begin) && !IsExcepted( nextinms ) ) {
//                printf( "Result: %d-%d-%d %d:%d\n" , next.year, next.month, next.day, next.hour, next.minute );
                *retval = nextinms;
                *isvalid = true;
                break;
            }
        }
        icalrecur_iterator_free(ritr);
    }

    icalcomponent_free( vcalendar );
    return NS_OK;
}

icaltimetype oeICalEventImpl::GetNextRecurrence( icaltimetype begin ) {
    icaltimetype result = icaltime_null_time();
    PRTime begininms = icaltime_as_timet( begin );
    begininms *= 1000;
    PRTime resultinms;
    PRBool isvalid;
    GetNextRecurrence( begininms ,&resultinms, &isvalid );
    if( !isvalid )
        return result;

    result = ConvertFromPrtime( resultinms );
    return result;
}

icaltimetype oeICalEventImpl::GetNextAlarmTime( icaltimetype begin ) {
#ifdef ICAL_DEBUG_ALL
    printf( "oeICalEventImpl::GetNextAlarmTime()\n" );
#endif
    icaltimetype result = icaltime_null_time();

    if( !m_hasalarm )
        return result;

    icaltimetype starting;
    if( icaltime_is_null_time( m_lastalarmack ) )
        starting = begin;
    else
        starting = m_lastalarmack;
    icaltimetype checkloop = starting;
    do {
        checkloop = GetNextRecurrence( checkloop );
        if( icaltime_is_null_time( checkloop ) )
            return checkloop;
        result = checkloop;
        result = CalculateAlarmTime( result );
    } while ( icaltime_compare( starting, result ) >= 0 );
    return result;
}

icaltimetype oeICalEventImpl::CalculateAlarmTime( icaltimetype date ) {
    icaltimetype result = date;
    if( strcasecmp( m_alarmunits, "days" ) == 0 )
        icaltime_adjust( &result, -m_alarmlength, 0, 0, 0 );
    else if( strcasecmp( m_alarmunits, "hours" ) == 0 )
        icaltime_adjust( &result, 0, -m_alarmlength, 0, 0 );
    else
        icaltime_adjust( &result, 0, 0, -m_alarmlength, 0 );

    return result;
}

NS_IMETHODIMP oeICalEventImpl::GetStart(oeIDateTime * *start)
{
    *start = m_start;
    NS_ADDREF(*start);
    return NS_OK;
}

NS_IMETHODIMP oeICalEventImpl::GetEnd(oeIDateTime * *end)
{
    *end = m_end;
    NS_ADDREF(*end);
    return NS_OK;
}

NS_IMETHODIMP oeICalEventImpl::GetRecurEnd(oeIDateTime * *recurend)
{
    *recurend = m_recurend;
    NS_ADDREF(*recurend);
    return NS_OK;
}

NS_IMETHODIMP oeICalEventImpl::GetRecurWeekdays(PRInt16 *aRetVal)
{
#ifdef ICAL_DEBUG_ALL
    printf( "GetRecurWeekdays()\n" );
#endif
    *aRetVal = m_recurweekdays;
    return NS_OK;
}
NS_IMETHODIMP oeICalEventImpl::SetRecurWeekdays(PRInt16 aNewVal)
{
#ifdef ICAL_DEBUG_ALL
    printf( "SetRecurWeekdays()\n" );
#endif
    m_recurweekdays = aNewVal;
    return NS_OK;
}

NS_IMETHODIMP oeICalEventImpl::GetRecurWeekNumber(PRInt16 *aRetVal)
{
#ifdef ICAL_DEBUG_ALL
    printf( "GetRecurWeekNumber()\n" );
#endif
    *aRetVal = m_recurweeknumber;
    return NS_OK;
}
NS_IMETHODIMP oeICalEventImpl::SetRecurWeekNumber(PRInt16 aNewVal)
{
#ifdef ICAL_DEBUG_ALL
    printf( "SetRecurWeekNumber()\n" );
#endif
    m_recurweeknumber = aNewVal;
    return NS_OK;
}

NS_IMETHODIMP oeICalEventImpl::GetIcalString(char **aRetVal)
{
#ifdef ICAL_DEBUG_ALL
    printf( "GetIcalString() = " );
#endif
    
    icalcomponent *vcalendar = AsIcalComponent();
    char *str = icalcomponent_as_ical_string( vcalendar );
    if( str ) {
        *aRetVal= (char*) nsMemory::Clone( str, strlen(str)+1);
        if( *aRetVal == NULL )
            return  NS_ERROR_OUT_OF_MEMORY;
    } else
        *aRetVal= EmptyReturn();
    icalcomponent_free( vcalendar );

#ifdef ICAL_DEBUG_ALL
    printf( "\"%s\"\n", *aRetVal );
#endif
    return NS_OK;
}

NS_IMETHODIMP oeICalEventImpl::ParseIcalString(const char *aNewVal)
{
#ifdef ICAL_DEBUG_ALL
    printf( "ParseIcalString( %s )\n", aNewVal );
#endif
    
    icalcomponent *vcalendar = icalparser_parse_string( aNewVal );
    ParseIcalComponent( vcalendar );
    icalcomponent_free( vcalendar );

    return NS_OK;
}

NS_IMETHODIMP oeICalEventImpl::AddException( PRTime exdate )
{
#ifdef ICAL_DEBUG_ALL
    printf( "oeICalEventImpl::AddException()\n" );
#endif
    icaltimetype tmpexdate = ConvertFromPrtime( exdate );
    tmpexdate.hour = 0;
    tmpexdate.minute = 0;
    tmpexdate.second = 0;
    exdate = icaltime_as_timet( tmpexdate );
    exdate *= 1000;
    m_exceptiondates.push_back( exdate );
    return NS_OK;
}

NS_IMETHODIMP
oeICalEventImpl::GetExceptions(nsISimpleEnumerator **datelist )
{
#ifdef ICAL_DEBUG_ALL
    printf( "oeICalEventImpl::GetExceptions()\n" );
#endif
    nsCOMPtr<oeDateEnumerator> dateEnum = new oeDateEnumerator();
    
    if (!dateEnum)
        return NS_ERROR_OUT_OF_MEMORY;

    for( int i=0; i<m_exceptiondates.size(); i++ ) {
        dateEnum->AddDate( m_exceptiondates[i] );
    }
    // bump ref count
    return dateEnum->QueryInterface(NS_GET_IID(nsISimpleEnumerator), (void **)datelist);
}

bool oeICalEventImpl::IsExcepted( PRTime date ) {
#ifdef ICAL_DEBUG_ALL
    printf( "oeICalEventImpl::IsExcepted() = " );
#endif
    icaltimetype tmpexdate = ConvertFromPrtime( date );
    tmpexdate.hour = 0;
    tmpexdate.minute = 0;
    tmpexdate.second = 0;
    date = icaltime_as_timet( tmpexdate );
    date *= 1000;
    
    bool result = false;
    for( int i=0; i<m_exceptiondates.size(); i++ ) {
        if( m_exceptiondates[i] == date ) {
            result = true;
            break;
        }
    }
#ifdef ICAL_DEBUG_ALL
    printf( "%d\n", result );
#endif
    return result;
}

void oeICalEventImpl::ParseIcalComponent( icalcomponent *vcalendar )
{
#ifdef ICAL_DEBUG_ALL
    printf( "ParseIcalComponent()\n" );
#endif

    icalcomponent *vevent = icalcomponent_get_first_component( vcalendar, ICAL_VEVENT_COMPONENT );
    assert( vevent != 0);

//id    
    icalproperty *prop = icalcomponent_get_first_property( vevent, ICAL_UID_PROPERTY );
    assert( prop != 0);
    const char *tmpstr = icalproperty_get_value_as_string( prop );
    m_id = atol( tmpstr );

//title
    if( m_title )
        nsMemory::Free( m_title );
    prop = icalcomponent_get_first_property( vevent, ICAL_SUMMARY_PROPERTY );
    if ( prop != 0) {
        tmpstr = icalproperty_get_value_as_string( prop );
        SetTitle( tmpstr );
    } else
        m_title = NULL;

//description
    if( m_description )
        nsMemory::Free( m_description );
    prop = icalcomponent_get_first_property( vevent, ICAL_DESCRIPTION_PROPERTY );
    if ( prop != 0) {
        tmpstr = icalproperty_get_value_as_string( prop );
        SetDescription( tmpstr );
    } else
        m_description = NULL;

//location
    if( m_location )
        nsMemory::Free( m_location );
    prop = icalcomponent_get_first_property( vevent, ICAL_LOCATION_PROPERTY );
    if ( prop != 0) {
        tmpstr = icalproperty_get_value_as_string( prop );
        SetLocation( tmpstr );
    } else
        m_location = NULL;

//category
    if( m_category )
        nsMemory::Free( m_category );
    prop = icalcomponent_get_first_property( vevent, ICAL_CATEGORIES_PROPERTY );
    if ( prop != 0) {
        tmpstr = (char *)icalproperty_get_value_as_string( prop );
        SetCategory( tmpstr );
    } else
        m_category= NULL;

//isprivate
    prop = icalcomponent_get_first_property( vevent, ICAL_CLASS_PROPERTY );
    if ( prop != 0) {
        tmpstr = icalproperty_get_value_as_string( prop );
        if( strcmp( tmpstr, "PUBLIC" ) == 0 )
            m_isprivate= false;
        else
            m_isprivate= true;
    } else
        m_isprivate= false;

//syncid
    for( prop = icalcomponent_get_first_property( vevent, ICAL_X_PROPERTY );
            prop != 0 ;
            prop = icalcomponent_get_next_property( vevent, ICAL_X_PROPERTY ) ) {
            icalparameter *tmppar = icalproperty_get_first_parameter( prop, ICAL_MEMBER_PARAMETER );
            if ( tmppar != 0 ) {
                tmpstr = (char *)icalparameter_get_member( tmppar );
                if( strcmp( tmpstr, "SyncId" ) == 0 )
                    break;
            }
    }
    
    if ( prop != 0) {
        tmpstr = (char *)icalproperty_get_value_as_string( prop );
        m_syncid= atol( tmpstr );
    } else
        m_syncid = 0;

 //allday
    for( prop = icalcomponent_get_first_property( vevent, ICAL_X_PROPERTY );
         prop != 0 ;
         prop = icalcomponent_get_next_property( vevent, ICAL_X_PROPERTY ) ) {
        icalparameter *tmppar = icalproperty_get_first_parameter( prop, ICAL_MEMBER_PARAMETER );
        if ( tmppar != 0 ) {
            tmpstr = icalparameter_get_member( tmppar );
            if( strcmp( tmpstr, "AllDay" ) == 0 )
                break;
        }
    }
    
    if ( prop != 0) {
        tmpstr = (char *)icalproperty_get_value_as_string( prop );
        if( strcmp( tmpstr, "TRUE" ) == 0 )
            m_allday= true;
        else
            m_allday= false;
    } else
        m_allday= false;

//alarm
    icalcomponent *valarm = icalcomponent_get_first_component( vevent, ICAL_VALARM_COMPONENT );
    
    if ( valarm != 0)
        m_hasalarm= true;
    else
        m_hasalarm= false;

//alarmunits
    for( prop = icalcomponent_get_first_property( vevent, ICAL_X_PROPERTY );
            prop != 0 ;
            prop = icalcomponent_get_next_property( vevent, ICAL_X_PROPERTY ) ) {
            icalparameter *tmppar = icalproperty_get_first_parameter( prop, ICAL_MEMBER_PARAMETER );
            if ( tmppar != 0 ) {
                tmpstr = icalparameter_get_member( tmppar );
                if( strcmp( tmpstr, "AlarmUnits" ) == 0 )
                    break;
            }
    }
    
    if ( prop != 0) {
        tmpstr = icalproperty_get_value_as_string( prop );
        SetAlarmUnits( tmpstr );
    } else
        SetAlarmUnits( "minutes" );
//alarmlength
    for( prop = icalcomponent_get_first_property( vevent, ICAL_X_PROPERTY );
            prop != 0 ;
            prop = icalcomponent_get_next_property( vevent, ICAL_X_PROPERTY ) ) {
            icalparameter *tmppar = icalproperty_get_first_parameter( prop, ICAL_MEMBER_PARAMETER );
            if ( tmppar != 0 ) {
                tmpstr = (char *)icalparameter_get_member( tmppar );
                if( strcmp( tmpstr, "AlarmLength" ) == 0 )
                    break;
            }
    }
    
    if ( prop != 0) {
        tmpstr = (char *)icalproperty_get_value_as_string( prop );
        m_alarmlength= atol( tmpstr );
    } else
        m_alarmlength = 0;

//alarmemail
    for( prop = icalcomponent_get_first_property( vevent, ICAL_X_PROPERTY );
            prop != 0 ;
            prop = icalcomponent_get_next_property( vevent, ICAL_X_PROPERTY ) ) {
            icalparameter *tmppar = icalproperty_get_first_parameter( prop, ICAL_MEMBER_PARAMETER );
            if ( tmppar != 0 ) {
                tmpstr = (char *)icalparameter_get_member( tmppar );
                if( strcmp( tmpstr, "AlarmEmailAddress" ) == 0 )
                    break;
            }
    }
    
    if ( prop != 0) {
        tmpstr = icalproperty_get_value_as_string( prop );
        SetAlarmEmailAddress( tmpstr );
    } else
        m_alarmemail= NULL;

    //lastalarmack
    prop = icalcomponent_get_first_property( vevent, ICAL_DTSTAMP_PROPERTY );
    if ( prop != 0) {
        m_lastalarmack = icalproperty_get_dtstart( prop );
    }

//inviteemail
    for( prop = icalcomponent_get_first_property( vevent, ICAL_X_PROPERTY );
            prop != 0 ;
            prop = icalcomponent_get_next_property( vevent, ICAL_X_PROPERTY ) ) {
            icalparameter *tmppar = icalproperty_get_first_parameter( prop, ICAL_MEMBER_PARAMETER );
            if ( tmppar != 0 ) {
                tmpstr = icalparameter_get_member( tmppar );
                if( strcmp( tmpstr, "InviteEmailAddress" ) == 0 )
                    break;
            }
    }
    
    if ( prop != 0) {
        tmpstr = (char *)icalproperty_get_value_as_string( prop );
        SetInviteEmailAddress( tmpstr );
    } else
        m_inviteemail = NULL;

//recurinterval
    for( prop = icalcomponent_get_first_property( vevent, ICAL_X_PROPERTY );
            prop != 0 ;
            prop = icalcomponent_get_next_property( vevent, ICAL_X_PROPERTY ) ) {
            icalparameter *tmppar = icalproperty_get_first_parameter( prop, ICAL_MEMBER_PARAMETER );
            if ( tmppar != 0 ) {
                tmpstr = icalparameter_get_member( tmppar );
                if( strcmp( tmpstr, "RecurInterval" ) == 0 )
                    break;
            }
    }
    
    if ( prop != 0) {
        tmpstr = (char *)icalproperty_get_value_as_string( prop );
        m_recurinterval= atol( tmpstr );
    } else
        m_recurinterval= 0;

//startdate
    prop = icalcomponent_get_first_property( vevent, ICAL_DTSTART_PROPERTY );
    if ( prop != 0) {
        m_start->m_datetime = icalproperty_get_dtstart( prop );
    }
//enddate
    prop = icalcomponent_get_first_property( vevent, ICAL_DTEND_PROPERTY );
    if ( prop != 0) {
        icaltimetype end;
        end = icalproperty_get_dtstart( prop );
        m_end->m_datetime = end;
    } else {
        m_end->m_datetime = m_start->m_datetime;
    }
//recurenddate & recurforever & recur & recurweekday & recurweeknumber
    m_recur = false;
    m_recurforever = false;
    prop = icalcomponent_get_first_property( vevent, ICAL_RRULE_PROPERTY );
    if ( prop != 0) {
        m_recur = true;
        struct icalrecurrencetype recur;
        recur = icalproperty_get_rrule(prop);
        m_recurend->m_datetime = recur.until;
        m_recurend->m_datetime.is_date = false;
        m_recurend->m_datetime.is_utc = false;
        if( icaltime_is_null_time( recur.until ) )
            m_recurforever = true;
        if( recur.freq == ICAL_WEEKLY_RECURRENCE ) {
            int k=0;
            while( recur.by_day[k] != ICAL_RECURRENCE_ARRAY_MAX ) {
                m_recurweekdays += 1 << (recur.by_day[k]-1);
                k++;
            }
        } else if( recur.freq == ICAL_MONTHLY_RECURRENCE ) {
            if( recur.by_day[0] != ICAL_RECURRENCE_ARRAY_MAX )
                m_recurweeknumber = icalrecurrencetype_day_position(recur.by_day[0]);
            if( m_recurweeknumber < 0 )
                m_recurweeknumber = 5;
        }
    }
//recurunits
    for( prop = icalcomponent_get_first_property( vevent, ICAL_X_PROPERTY );
            prop != 0 ;
            prop = icalcomponent_get_next_property( vevent, ICAL_X_PROPERTY ) ) {
            icalparameter *tmppar = icalproperty_get_first_parameter( prop, ICAL_MEMBER_PARAMETER );
            if ( tmppar != 0 ) {
                tmpstr = icalparameter_get_member( tmppar );
                if( strcmp( tmpstr, "RecurUnits" ) == 0 )
                    break;
            }
    }
    
    if ( prop != 0) {
        tmpstr = (char *)icalproperty_get_value_as_string( prop );
        SetRecurUnits( tmpstr );
    } else
        SetRecurUnits( "weeks" );

    //recur exceptions
    for( prop = icalcomponent_get_first_property( vevent, ICAL_RECURRENCEID_PROPERTY );
            prop != 0 ;
            prop = icalcomponent_get_next_property( vevent, ICAL_RECURRENCEID_PROPERTY ) ) {
        icaltimetype exdate = icalproperty_get_recurrenceid( prop );
        PRTime exdateinms = icaltime_as_timet( exdate );
        exdateinms *= 1000;
        m_exceptiondates.push_back( exdateinms );
    }
}

#define ICALEVENT_VERSION "1.1"

icalcomponent* oeICalEventImpl::AsIcalComponent()
{
#ifdef ICAL_DEBUG_ALL
    printf( "AsIcalComponent()\n" );
#endif
    icalcomponent *newcalendar;
    
    newcalendar = icalcomponent_new_vcalendar();
    assert(newcalendar != 0);

    //version
    icalproperty *prop = icalproperty_new_version( ICALEVENT_VERSION );
    icalcomponent_add_property( newcalendar, prop );

    icalcomponent *vevent = icalcomponent_new_vevent();

    //id
    char tmpstr[20];
    sprintf( tmpstr, "%lu", m_id );
    prop = icalproperty_new_uid( tmpstr );
    icalcomponent_add_property( vevent, prop );

    //title
    if( m_title && strlen( m_title ) != 0 ){
        prop = icalproperty_new_summary( m_title );
        icalcomponent_add_property( vevent, prop );
    }
    //description
    if( m_description && strlen( m_description ) != 0 ){
        prop = icalproperty_new_description( m_description );
        icalcomponent_add_property( vevent, prop );
    }

    //location
    if( m_location && strlen( m_location ) != 0 ){
        prop = icalproperty_new_location( m_location );
        icalcomponent_add_property( vevent, prop );
    }

    //category
    if( m_category && strlen( m_category ) != 0 ){
        prop = icalproperty_new_categories( m_category );
        icalcomponent_add_property( vevent, prop );
    }

    //isprivate
    if( m_isprivate )
        prop = icalproperty_new_class( ICAL_CLASS_PRIVATE );
    else
        prop = icalproperty_new_class( ICAL_CLASS_PUBLIC );
    icalcomponent_add_property( vevent, prop );

    icalparameter *tmppar;
    //syncId
    if( m_syncid ) {
        sprintf( tmpstr, "%lu", m_syncid );
        tmppar = icalparameter_new_member( "SyncId" );
        prop = icalproperty_new_x( tmpstr );
        icalproperty_add_parameter( prop, tmppar );
        icalcomponent_add_property( vevent, prop );
    }

    //allday
    if( m_allday ) {
        tmppar = icalparameter_new_member( "AllDay" );
        prop = icalproperty_new_x( "TRUE" );
        icalproperty_add_parameter( prop, tmppar );
        icalcomponent_add_property( vevent, prop );
    }

    //alarm
    if( m_hasalarm ) {
        struct icaltriggertype trig;
        icalcomponent *valarm = icalcomponent_new_valarm();
        trig.time.year = trig.time.month = trig.time.day = trig.time.hour = trig.time.minute = trig.time.second = 0;
        trig.duration.is_neg = true;
        trig.duration.days = trig.duration.weeks = trig.duration.hours = trig.duration.minutes = trig.duration.seconds = 0;
        if( m_alarmunits ) {
            if( strcasecmp( m_alarmunits, "days" ) == 0 )
                trig.duration.days = m_alarmlength;
            else if( strcasecmp( m_alarmunits, "hours" ) == 0 )
                trig.duration.hours = m_alarmlength;
            else
                trig.duration.minutes = m_alarmlength;
        } else
            trig.duration.minutes = m_alarmlength;

        if( m_alarmlength == 0 )
            trig.duration.seconds = 1;

        prop = icalproperty_new_trigger( trig );
        icalcomponent_add_property( valarm, prop );
        icalcomponent_add_component( vevent, valarm );
    }

    //alarmunits
    if( m_alarmunits && strlen( m_alarmunits ) != 0 ){
        icalparameter *tmppar = icalparameter_new_member( "AlarmUnits" );
        prop = icalproperty_new_x( m_alarmunits );
        icalproperty_add_parameter( prop, tmppar );
        icalcomponent_add_property( vevent, prop );
    }

    //alarmlength
    if( m_alarmlength ) {
        sprintf( tmpstr, "%lu", m_alarmlength );
        tmppar = icalparameter_new_member( "AlarmLength" );
        prop = icalproperty_new_x( tmpstr );
        icalproperty_add_parameter( prop, tmppar );
        icalcomponent_add_property( vevent, prop );
    }

    //alarmemail
    if( m_alarmemail && strlen( m_alarmemail ) != 0 ){
        icalparameter *tmppar = icalparameter_new_member( "AlarmEmailAddress" );
        prop = icalproperty_new_x( m_alarmemail );
        icalproperty_add_parameter( prop, tmppar );
        icalcomponent_add_property( vevent, prop );
    }

    //lastalarmack
    if( !icaltime_is_null_time( m_lastalarmack ) ) {
        prop = icalproperty_new_dtstamp( m_lastalarmack );
        icalcomponent_add_property( vevent, prop );
    }

    //inviteemail
    if( m_inviteemail && strlen( m_inviteemail ) != 0 ){
        icalparameter *tmppar = icalparameter_new_member( "InviteEmailAddress" );
        prop = icalproperty_new_x( m_inviteemail );
        icalproperty_add_parameter( prop, tmppar );
        icalcomponent_add_property( vevent, prop );
    }

    //Create enddate if does not exist
    if( icaltime_is_null_time( m_end->m_datetime ) ) {
        //Set to the same as start date 23:59
        m_end->m_datetime = m_start->m_datetime;
        m_end->SetHour( 23 ); m_end->SetMinute( 59 );
    }

    //recurunits
    if( m_recurunits && strlen( m_recurunits ) != 0 ){
        icalparameter *tmppar = icalparameter_new_member( "RecurUnits" );
        prop = icalproperty_new_x( m_recurunits );
        icalproperty_add_parameter( prop, tmppar );
        icalcomponent_add_property( vevent, prop );
    }

    //recurinterval
    sprintf( tmpstr, "%lu", m_recurinterval );
    tmppar = icalparameter_new_member( "RecurInterval" );
    prop = icalproperty_new_x( tmpstr );
    icalproperty_add_parameter( prop, tmppar );
    icalcomponent_add_property( vevent, prop );

    //recurrence

    if( m_recur ) {
        int recurtype = RECUR_NONE;
        int interval = m_recurinterval;
        if( interval == 0 )
            interval = 1;
        if( m_recurunits ) {
            if( strcasecmp( m_recurunits , "days" ) == 0 ) {
               recurtype = RECUR_DAILY;
            } else if( strcasecmp( m_recurunits , "weeks" ) == 0 ) {
               recurtype = RECUR_WEEKLY;
//               recurtype = RECUR_DAILY;
//               interval = 7 * m_recurinterval;
            } else if( strcasecmp( m_recurunits , "months" ) == 0 ) {
                recurtype = RECUR_MONTHLY_MDAY;
            } else if( strcasecmp( m_recurunits, "months_day" ) == 0 ) {
                recurtype = RECUR_MONTHLY_WDAY;
            } else if( strcasecmp( m_recurunits , "years" ) == 0 ) {
                recurtype = RECUR_YEARLY;
            }
        }
        
        struct icalrecurrencetype recur;
        icalrecurrencetype_clear( &recur );
        //    for( int i=0; i<ICAL_BY_MONTH_SIZE; i++ )
        //        recur.by_month[i] = i;
        recur.interval = interval;
        recur.until.is_utc = false;
        recur.until.is_date = true;
        if( m_recurforever ) {
            recur.until.year = 0;
            recur.until.month = 0;
            recur.until.day = 0;
            recur.until.hour = 0;
            recur.until.minute = 0;
            recur.until.second = 0;
        } else {
            recur.until.year = m_recurend->m_datetime.year;
            recur.until.month = m_recurend->m_datetime.month;
            recur.until.day = m_recurend->m_datetime.day;
            recur.until.hour = 23;
            recur.until.minute = 59;
            recur.until.second = 59;
        }

        switch ( recurtype ) {
            case RECUR_NONE:
                break;
            case RECUR_DAILY:
                recur.freq = ICAL_DAILY_RECURRENCE;
                prop = icalproperty_new_rrule( recur );
                icalcomponent_add_property( vevent, prop );
                break;
            case RECUR_WEEKLY:{
                recur.freq = ICAL_WEEKLY_RECURRENCE;
                short weekdaymask = m_recurweekdays;
                int k=0;
//                bool weekdaymatchesstartdate=false;
                for( int i=0; i<7; i++ ) {
                    if( weekdaymask & 1 ) {
                        recur.by_day[k++]=i+1;
//                        if( !weekdaymatchesstartdate ) {
//                            m_start->AdjustToWeekday( i+1 );
//                            m_end->AdjustToWeekday( i+1 );
//                            weekdaymatchesstartdate=true;
//                        }
                    }
                    weekdaymask >>= 1;
                }
                prop = icalproperty_new_rrule( recur );
                icalcomponent_add_property( vevent, prop );
                break;
            }
            case RECUR_MONTHLY_MDAY:
                recur.freq = ICAL_MONTHLY_RECURRENCE;
                if( m_recurweeknumber ) {
//                    printf( "DAY: %d\n" , icaltime_day_of_week( m_start->m_datetime ) );
//                    printf( "WEEKNUMBER: %d\n" , m_recurweeknumber );
                    if( m_recurweeknumber != 5 )
                        recur.by_day[0] = icaltime_day_of_week( m_start->m_datetime ) + m_recurweeknumber*8;
                    else
                        recur.by_day[0] = - icaltime_day_of_week( m_start->m_datetime ) - 8 ;
                }
                prop = icalproperty_new_rrule( recur );
                icalcomponent_add_property( vevent, prop );
                break;
            case RECUR_MONTHLY_WDAY:
                recur.freq = ICAL_MONTHLY_RECURRENCE;
                prop = icalproperty_new_rrule( recur );
                icalcomponent_add_property( vevent, prop );
                break;
            case RECUR_YEARLY:
                recur.by_month[0] = m_start->m_datetime.month;
                recur.freq = ICAL_YEARLY_RECURRENCE;
                prop = icalproperty_new_rrule( recur );
                icalcomponent_add_property( vevent, prop );
                break;
        }

        //exceptions
        for( int i=0; i<m_exceptiondates.size(); i++ ) {
            icaltimetype exdate = ConvertFromPrtime( m_exceptiondates[i] );
            prop = icalproperty_new_recurrenceid( exdate );
            icalcomponent_add_property( vevent, prop );
        }
    }

    //startdate
    
    if( m_allday ) {
        m_start->SetHour( 0 );
        m_start->SetMinute( 0 );
    }
    prop = icalproperty_new_dtstart( m_start->m_datetime );
    icalcomponent_add_property( vevent, prop );

    //enddate
    if( m_allday ) {
        m_end->SetHour( 23 );
        m_end->SetMinute( 59 );
    }
    prop = icalproperty_new_dtend( m_end->m_datetime );
    icalcomponent_add_property( vevent, prop );

    //add event to newcalendar
    icalcomponent_add_component( newcalendar, vevent );
    return newcalendar;
}
