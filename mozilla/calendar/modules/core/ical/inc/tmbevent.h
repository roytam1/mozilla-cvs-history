/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- 
 * 
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape 
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

/* 
 * tmbevent.h
 * John Sun
 * 2/6/98 2:59:24 AM
 *
 * modified: 8/31/98  sman  -  added curl support
 */

#ifndef __TIMEBASEDEVENT_H_
#define __TIMEBASEDEVENT_H_

#include "ptrarray.h"
#include "datetime.h"
#include "attendee.h"
#include "icalredr.h"
#include "prprty.h"
#include "duration.h"
#include "jlog.h"
#include "valarm.h"
#include "julnstr.h"
#include "nscalcoreicalexp.h"

/**
 *  TimeBasedEvent is the superclass of VEvent, VTodo, and VJournal.
 *  It encapsulates the similar state between the 3 classes.  This
 *  includes recurrence handling, Alarm handling, as well as containing
 *  all properties that exist in all three properties.
 *  NOTE:  No ownership taken anymore.  (always pass by value)
 */
class NS_CAL_CORE_ICAL TimeBasedEvent : public ICalComponent
{
protected:
#if 0

    /**
     * Default Constructor.  Hide from clients.
     */
    TimeBasedEvent();
#endif


    /**
     * Copy constructor.
     * @param           that        TimeBasedEvent to copy.
     */
    TimeBasedEvent(TimeBasedEvent & that);
public:
    /*TimeBasedEvent(NSCalendar parent);*/

    /**
     * Returns the curl associated with this nsCalendar. The
     * curl points to the calendar store where components in
     * this nsCalendar are to be stored. It is required
     * for an nsCalendar to have an associated curl if any
     * of its components will be persisted in a calendar store.
     *
     * @return  a JulianString containing the curl 
     */
    JulianString getCurl() const {return m_sCurl;}

    /**
     * Set the default curl for this nsCalendar. 
     */
    void setCurl(const char* ps) {m_sCurl = ps;}

    /**
     * Set the default curl for this nsCalendar. 
     */
    void setCurl(const JulianString& s) {m_sCurl = s;}

    /**
     * Constructor.  Create TimeBasedEvent with initial log file set to initLog.
     * @param           initLog     initial log file pointer
     */
    TimeBasedEvent(JLog * initLog = 0);

    /**
     * Destructor.
     */
    virtual ~TimeBasedEvent();
 

    /**
     * Pure virtual method.  Calculate difference from start, end time
     * For VEvent, this would be DTEnd - DTStart.
     *
     * @return          virtual Date 
     */
    virtual Date difference() { PR_ASSERT(FALSE); return 0; }

    /**
     *  set RSVP on all attendees set to TRUE
     */
    /*void rsvpAll();*/
     
    /**
     * Set each attendee's status parameter to needs action (except owner)
     */
    /*void needsAction();*/

    /*UnicodeString getOrganizer();*/
    /*UnicodeString getOwner();*/

    /*UnicodeString toString(UnicodeString strFmt);*/

    /*  -- Start of ICALComponent interface -- */

    /**
     * The parse method is the standard interface for ICalComponent
     * subclasses to parse from an ICalReader object 
     * and from the ITIP method type (i.e. PUBLISH, REQUEST, CANCEL, etc.).
     * The method type can be set to \"\" if there is no method to be loaded.
     * Also accepts a vector of VTimeZone objects to apply to
     * the DateTime properties of the component.
     * 4-2-98:  Added bIgnoreBeginError.  If desired to start parsing in Component level,
     * it is useful to ignore first "BEGIN:".   Setting bIgnoreBeginError to TRUE allows for
     * this.
     *     
     * @param   brFile              ICalReader to load from
     * @param   sType               name of component (i.e. VEVENT, VTODO, VTIMEZONE)
     * @param   parseStatus         return parse error status string (normally return "OK")
     * @param   vTimeZones          vector of VTimeZones to apply to datetimes
     * @param   bIgnoreBeginError   TRUE = ignore first "BEGIN:VEVENT", FALSE otherwise
     * @param   encoding            the encoding of the stream,  default is 7bit.
     *
     * @return  parse error status string (parseStatus)
     */
    virtual UnicodeString & parse(ICalReader * brFile, UnicodeString & sMethod,
        UnicodeString & parseStatus, JulianPtrArray * vTimeZones = 0,
        t_bool bIgnoreBeginError = FALSE, nsCalUtility::MimeEncoding encoding =
        nsCalUtility::MimeEncoding_7bit) 
    {
        PR_ASSERT(FALSE);
        if (brFile != NULL && vTimeZones != NULL && sMethod.size() > 0
            && parseStatus.size() > 0 && bIgnoreBeginError && encoding > 0)
        {
        }
        return parseStatus;
    }   

    /**
     * Returns a clone of this object
     * @param           initLog     log file to write errors to
     *
     * @return          clone of this object
     */
    virtual ICalComponent * clone(JLog * initLog) { PR_ASSERT(FALSE); if (initLog) {} return 0; }

    /**
     * Return TRUE if component is valid, FALSE otherwise
     *
     * @return          TRUE if is valid, FALSE otherwise
     */
    virtual t_bool isValid();

    /**
     * Print all contents of ICalComponent to iCal export format.
     *
     * @return          string containing iCal export format of ICalComponent
     */
    UnicodeString toICALString();
    
    /*UnicodeString toICALString(UnicodeString sMethod);*/

    /**
     * Print all contents of ICalComponent to iCal export format, depending
     * on attendee name, attendee delegated-to, and where to include recurrence-id or not
     *
     * @param           method          method name (REQUEST,PUBLISH, etc.)
     * @param           name            attendee name to filter with
     * @param           isRecurring     TRUE = no recurrenceid, FALSE = include recurrenceid
     *
     * @return          string containing iCal export format of ICalComponent
     */
    UnicodeString toICALString(UnicodeString sMethod, 
        UnicodeString sName, t_bool isRecurring = FALSE);

    /**
     * Depending on character passed in, returns a string that represents
     * the ICAL export string of that property that the character represents, if 
     * other paramaters not null, then print out information is filtered in several ways
     * Attendee print out can be filtered so that only attendee with certain name is printed
     * also, can print out the relevant attendees in a delegation message
     * (ie. owner, delegate-to, delegate-from) 
     *
     * @param       c                   char of what property to print
     * @param       sFilterAttendee     name of attendee to print
     * @param       bDelegateRequest    TRUE if a delegate request, FALSE if not
     * @return                          ICAL export format of that property
     */
    virtual UnicodeString formatChar(t_int32 c, UnicodeString sFilterAttendee,
        t_bool delegateRequest = FALSE);

    /**
     * convert a character to the content of a property in string 
     * human-readable format
     * @param           c           a character represents a property
     * @param           dateFmt     for formatting datetimes
     *
     * @return          property in human-readable string
     */
    virtual UnicodeString toStringChar(t_int32 c, UnicodeString & dateFmt);

    /**
     * Returns the ICAL_COMPONENT enumeration value of this ICalComponent.
     * Each ICalComponent subclass must return a unique ICAL_COMPONENT value.
     * 
     * @return          ICAL_COMPONENT value of this component
     */
    virtual ICAL_COMPONENT GetType() const { PR_ASSERT(FALSE); return ICAL_COMPONENT_VEVENT; }

    /* OVERRIDES ICalComponent::MatchUID_seqNO */
    /**
     * check if this event matches the given UID and sequence number 
     * @return TRUE if this event matches the given UID and sequence number, FALSE otherwise
     */
    virtual t_bool MatchUID_seqNO(UnicodeString uid, t_int32 iSeqNo);

    /*  -- Start of ICALComponent interface -- */
   
    /**
     * Pure virtual method used as wrapper to ICalComponent::format method. 
     * @param           strFmt              iCal format string 
     * @param           sFilterAttendee     attendee to filter
     * @param           delegateRequest     delegate request = TRUE, FALSE otherwise
     *
     * @return          output iCal formatted export string
     */
    virtual UnicodeString formatHelper(UnicodeString & strFmt, 
        UnicodeString sFilterAttendee, t_bool delegateRequest = FALSE)
    {
        PR_ASSERT(FALSE); 
        if (strFmt.size() == 0 && sFilterAttendee.size() == 0 && delegateRequest) {}
        return "";
    }
    
    /**
     * Parse the TimeBasedEvent from a ICalReader object, given the component name in sType.
     * Takes in a method name since, this affects whether event is valid or not.
     * Also takes in vector of timezones to apply to datetimes with TZID values.
     * The bIgnoreBeginError flag is used to ignore the first BEGIN:VEVENT(VTODO,VJOURNAL).
     * This is useful when this method is called somewhere besides the NSCalendar::parse().
     * For example, when receiving data a from CAPI, it returns events, not calendars.  
     * So to parse event, the first BEGIN:VEVENT should be ignored.
     * If then parse terminates correctly with an END:VEVENT(VTODO, VJOURNAL) then parseStatus
     * is set to OK and returned.  However is an incorrect termination of parse occurs,
     * then parseStatus returns the cause of the incorrect termination.  An incorrect
     * termination of parse can occur if the parse reaches an "END:VCALENDAR" before and
     * "END:VEVENT" for example.  In this case, parseStatus would return "END:VCALENDAR".
     * Other incorrect termination causes are a "BEGIN:VCALENDAR", or another "BEGIN:VEVENT"
     * (TODO: finish incorrect termination handling).
     * 
     * @param           sType       component name (i.e. VEVENT, VTODO, VJOURNAL)
     * @param           brFile      ICalReader object to parse from
     * @param           sMethod     calendar method name (PUBLISH, REQUEST, etc.)
     * @param           parseStatus return OK if no error, otherwise an error
     * @param           vTimeZones  vector of timezones to apply to datetimes
     * @param           bIgnoreBeginError   if parsing just a vevent (not from an calendar, set to TRUE)
     * @param           encoding            the encoding of the stream,  default is 7bit.
     *
     * @return          parseStatus (OK if no error, otherwise an error)
     */
    UnicodeString & parseType(UnicodeString & sType, ICalReader * brFile,
        UnicodeString & sMethod, UnicodeString & parseStatus, JulianPtrArray * vTimeZones,
        t_bool bIgnoreBeginError = FALSE,
        nsCalUtility::MimeEncoding encoding = nsCalUtility::MimeEncoding_7bit);

    /**
     * Prints properties of an CANCEL message, specifically
     *  
     * @return a string with COMMENT, DTSTAMP, STATUS, STATUS=CANCELLED, SEQUENCE, UID properties
     */
    virtual UnicodeString cancelMessage() { PR_ASSERT(FALSE); return ""; }
  
    /**
     * Prints properties of an REQUEST message
     * @return string with request message required properties
     */
    virtual UnicodeString requestMessage() { PR_ASSERT(FALSE); return ""; }
    
    /**
     * Prints properties of an REQUEST message except Recur-id
     * @return string with request message required properties
     */
    virtual UnicodeString requestRecurMessage() { PR_ASSERT(FALSE); return ""; }
    
    /**
     * Prints properties of an COUNTER message
     * @return string with counter message required properties
     */
    virtual UnicodeString counterMessage() { PR_ASSERT(FALSE); return ""; }
    
    /**
     * Prints properties of an DECLINECOUNTER message
     * @return string with decline-counter message required properties
     */
    virtual UnicodeString declineCounterMessage() { PR_ASSERT(FALSE); return ""; }
    
    /**
     * Prints properties of an ADD message
     * @return string with add message required properties
     */
    virtual UnicodeString addMessage() { PR_ASSERT(FALSE); return ""; }

    /**
     * Prints properties of an REFRESH message
     * @param sAttendeeFilter name of requestor of refresh
     * @return string with refresh message required properties
     */
    virtual UnicodeString refreshMessage(UnicodeString sAttendeeFilter)
    {
        PR_ASSERT(FALSE);
        if (sAttendeeFilter.size() > 0) {} return ""; 
    }
    
    /**
     * Prints properties of an ALL PROPERTIES message (for debugging method)
     * @return string with all properties
     */
    virtual UnicodeString allMessage() { PR_ASSERT(FALSE); return ""; }
     
    /**
     * Prints properties of an REPLY message
     * @param sAttendeeFilter name of replier
     * @return string with reply message required properties
     */
    virtual UnicodeString replyMessage(UnicodeString sAttendeeFilter)
    {
        PR_ASSERT(FALSE);
        if (sAttendeeFilter.size() > 0) {} return ""; 
    }

    /**
     * Prints properties of an PUBLISH message
     * @return string with publish message required properties
     */
    virtual UnicodeString publishMessage() { PR_ASSERT(FALSE); return ""; }

    /**
     * Prints properties of an PUBLISH message except recur-id
     * @return string with publish message required properties
     */
    virtual UnicodeString publishRecurMessage() { PR_ASSERT(FALSE); return ""; }

    /* --  delegate methods -- */
    /*
    virtual UnicodeString delegateRequestMessage(UnicodeString sAttendeeFilter,
        UnicodeString sDelegateTo, t_bool bRecur = FALSE);
        */
/*
    {
        PR_ASSERT(FALSE);
        if (sAttendeeFilter.size() > 0) {return ""; }
        if (sDelegateTo.size() > 0 && t_bool) {return ""; }
        return ""; 
    }
    */
/*
    virtual UnicodeString delegateRequestRecurMessage(UnicodeString sAttendeeFilter,
        UnicodeString sDelegateTo, t_bool bRecur)
    {
        PR_ASSERT(FALSE);
        if (sAttendeeFilter.size() > 0) {return ""; }
        return ""; 
    }
    */
   /*
    virtual UnicodeString delegateReplyMessage(UnicodeString sAttendeeFilter,
        UnicodeString sDelegateTo, t_bool bRecur = FALSE);
        */
    /*
    {
        PR_ASSERT(FALSE);
        if (sAttendeeFilter.size() > 0) {return ""; }
        if (sDelegateTo.size() > 0 && t_bool) {return ""; }
        return ""; 
    }
    */
    /*
    virtual UnicodeString delegateReplyRecurMessage(UnicodeString sAttendeeFilter,
        UnicodeString sDelegateTo, t_bool bRecur)
    {
        PR_ASSERT(FALSE);
        if (sAttendeeFilter.size() > 0) {return ""; }
        return ""; 
    }
    */

#if 0
    static void setDateTimeValue(ICalProperty ** dateTimePropertyPtr,
        DateTime inVal, JulianPtrArray * inParameters);
    static void getDateTimeValue(ICalProperty ** dateTimePropertyPtr, DateTime & outVal);
#endif

    /* -- Getters and Setters -- */

    /**
     * Return TRUE if event is an anniversary event, FALSE otherwise.
     * An anniversary event is an event that does not represent a 
     * scheduled amount of time, but more of a daily reminder.
     * During parsing, this is set when the VALUE=DATE is parsed 
     * on the DTSTART property.  An anniversary event can span multiple
     * days. See spec for better definition.
     *
     * @return          TRUE if anniversary event, FALSE otherwise
     */
    t_bool isAllDayEvent() { return m_bAllDayEvent; }
    void setAllDayEvent(t_bool b)    { m_bAllDayEvent = b; }
    
    /* --- Getters and Setters ---*/ 
    
    /* LAST-MODIFIED */
    DateTime getLastModified() const;
    void setLastModified(DateTime s, JulianPtrArray * parameters = 0);
    ICalProperty * getLastModifiedProperty() const { return m_LastModified; }

    /* CREATED */
    DateTime getCreated() const;
    void setCreated(DateTime s, JulianPtrArray * parameters = 0);
    ICalProperty * getCreatedProperty() const { return m_Created; }
    
    /* DTSTAMP */
    DateTime getDTStamp() const;
    void setDTStamp(DateTime s, JulianPtrArray * parameters = 0);
    ICalProperty * getDTStampProperty() const { return m_DTStamp; }
    
    /* DTSTART */
    DateTime getDTStart() const;
    void setDTStart(DateTime s, JulianPtrArray * parameters = 0);
    ICalProperty * getDTStartProperty() const { return m_DTStart; }

    /* DESCRIPTION */
    UnicodeString getDescription() const;
    void setDescription(UnicodeString s, JulianPtrArray * parameters = 0);
    ICalProperty * getDescriptionProperty() const { return m_Description; }

    /* URL */
    UnicodeString getURL() const;
    void setURL(UnicodeString s, JulianPtrArray * parameters = 0);
    ICalProperty * getURLProperty() const { return m_URL; }

    /* SUMMARY */
    UnicodeString getSummary() const;
    void setSummary(UnicodeString s, JulianPtrArray * parameters = 0);
    ICalProperty * getSummaryProperty() const { return m_Summary; }

    /* CLASS */
    UnicodeString getClass() const;
    void setClass(UnicodeString s, JulianPtrArray * parameters = 0);
    ICalProperty * getClassProperty() const { return m_Class; }

    /* STATUS */
    UnicodeString getStatus() const;
    void setStatus(UnicodeString s, JulianPtrArray * parameters = 0);
    ICalProperty * getStatusProperty() const { return m_Status; }

    /* REQUESTSTATUS */
    /* UnicodeString getRequestStatus() const;
    void setRequestStatus(UnicodeString s, JulianPtrArray * parameters = 0);
    ICalProperty * getRequestStatusProperty() const { return m_RequestStatus; }*/
    void addRequestStatus(UnicodeString s, JulianPtrArray * parameters = 0);
    void addRequestStatusProperty(ICalProperty * prop);
    JulianPtrArray * getRequestStatus() const              { return m_RequestStatusVctr; }

    /* UID */
    virtual UnicodeString getUID() const;
    void setUID(UnicodeString s, JulianPtrArray * parameters = 0);
    ICalProperty * getUIDProperty() const { return m_UID; }

    /* SEQUENCE */
    virtual t_int32 getSequence() const;
    void setSequence(t_int32 i, JulianPtrArray * parameters = 0);
    ICalProperty * getSequenceProperty() const { return m_Sequence; }

    /* COMMENT */
    void addComment(UnicodeString s, JulianPtrArray * parameters = 0);
    void addCommentProperty(ICalProperty * prop);
    JulianPtrArray * getComment() const              { return m_CommentVctr; }

    /* remove all previous comments, then adds new comment s */
    void setNewComments(UnicodeString s);

    /* ATTACH */
    void addAttach(UnicodeString s, JulianPtrArray * parameters = 0);
    void addAttachProperty(ICalProperty * prop);      
    JulianPtrArray * getAttach() const               { return m_AttachVctr; }

    /* RELATED-TO (not a vector anymore, now just a property) */
    void addRelatedTo(UnicodeString s, JulianPtrArray * parameters = 0);
    void addRelatedToProperty(ICalProperty * prop);
    JulianPtrArray * getRelatedTo() const            { return m_RelatedToVctr; }

    /* CONTACT */
    void addContact(UnicodeString s, JulianPtrArray * parameters = 0);
    void addContactProperty(ICalProperty * prop);
    JulianPtrArray * getContact() const                    { return m_ContactVctr; }

    /* RDATE */
    void addRDate(UnicodeString s, JulianPtrArray * parameters = 0);
    void addRDateProperty(ICalProperty * prop);
    JulianPtrArray * getRDates() const { return m_RDateVctr; }

    /* EXDATE */
    void addExDate(UnicodeString s, JulianPtrArray * parameters = 0);
    void addExDateProperty(ICalProperty * prop);
    JulianPtrArray * getExDates() const { return m_ExDateVctr; }

    /* CATEGORIES */
    void addCategories(UnicodeString s, JulianPtrArray * parameters = 0);
    void addCategoriesProperty(ICalProperty * prop);
    JulianPtrArray * getCategories()  const          { return m_CategoriesVctr; }
    void addCategoriesPropertyVector(UnicodeString & propVal, JulianPtrArray * parameters);
    
    /* ORGANIZER */
    UnicodeString getOrganizer() const;
    void setOrganizer(UnicodeString s, JulianPtrArray * parameters = 0);
    ICalProperty * getOrganizerProperty() const { return m_Organizer; }

    /* RECURRENCEID */
    ICalProperty * getRecurrenceIDProperty() const { return m_RecurrenceID; }
    void setRecurrenceID(DateTime d, JulianPtrArray * parameters = 0);
    DateTime getRecurrenceID() const;

    /* XTOKENS: NOTE: a vector of strings, not a vector of ICalProperties */
    void addXTokens(UnicodeString s);
    JulianPtrArray * getXTokens() const { return m_XTokensVctr; }

    /* EXRULE: NOTE: a vector of strings, not a vector of ICalProperties */
    void addExRuleString(UnicodeString s);
    JulianPtrArray * getExRules() const { return m_ExRuleVctr; }

    /* RRULE: NOTE: a vector of strings, not a vector of ICalProperties */
    void addRRuleString(UnicodeString s);
    JulianPtrArray * getRRules() const { return m_RRuleVctr; }

    /* ATTENDEE */
    void addAttendee(Attendee * a); 
    JulianPtrArray * getAttendees() const { return m_AttendeesVctr ; }

    /*---------------------------------------------------------------------*/
    
    /* ALARMS */
    JulianPtrArray * getAlarms() const { return m_AlarmsVctr ; }
    void addAlarm(VAlarm * a);

    /* ORIGSTART */ 
    void setOrigStart(DateTime d) { m_origDTStart = d; }
    DateTime getOrigStart() { return m_origDTStart; }

    /* MYORIGSTART */
    void setMyOrigStart(DateTime d) { m_origMyDTStart = d; }
    DateTime getMyOrigStart() { return m_origMyDTStart; }

    /* METHOD */
    void setMethod(UnicodeString & s);
    UnicodeString & getMethod() { return m_sMethod; }

    /* BOUND */
    t_int32 getBound() const { return ms_iBound; }
    void setBound(t_int32 i) { ms_iBound = i; }

    /**
     * Set DTStamp to current date time.
     */
    void stamp();

    /**
     * Return the Attendee in attendee vector with name equal
     * to sAttendee.  Return 0 if no attendee with that name.
     * @param           sAttendee   name of attendee
     *
     * @return          Attendee whose name is sAttendee, 0 if not found
     */
    Attendee * getAttendee(UnicodeString sAttendee);
     
    /**
     * Checks is this component is expandable.  An expandable component
     * is a component that has something in the RRULE or RDATE vector 
     * and does NOT have a RecurrenceID.
     *
     * @return          TRUE if is expandable component, FALSE otherwise.
     */
    t_bool isExpandableEvent() const;

    /**
     * Helper method.  Pure virtual method.  Used by subclasses to
     * populate recurrence-dependent data.  For example, a recurring
     * VEvent needs to have DTEnd calculated from recurring 
     * DTStart.  VEvent will overwrite method set DTEnd correctly.
     * Abstract difference() method is used to calculate ldiff.
     * Need to pass vector of RDate periods in case end value must
     * be adjusted in RDATE is a PERIOD value.
     * @param           start       recurrence instance starting time
     * @param           ldiff       abstract difference value
     * @param           vPer        vector of RDate periods
     */
    virtual void populateDatesHelper(DateTime start, Date ldiff, JulianPtrArray * vPer)
    {
        PR_ASSERT(FALSE); if(start.getTime() > ldiff && vPer) {}
    }
    
      
    /**
     * takes the vector of rrule, rdate, exrule, and exdates, dtstart, 
     * and bound and returns a vector of generated recurrence events in vOut.  
     * Does this in two parts
     * The first part generates a vector of DateTimes that represent 
     * the dates of the recurrence.
     * The second part takes these dates and creates an event with them.
     * @param           vOut        fill-in vector with generated recurrence events
     * @param           vTimeZones  vector of timezones
     */
    void createRecurrenceEvents(JulianPtrArray * vOut, 
        JulianPtrArray * vTimeZones);
    
    /*
    void addDelegate(UnicodeString & sAttendeeFilter,
        UnicodeString & sDelegateTo);
    */

    /**
     * Sets Attendee with the name sAttendeeFilter to status.
     * If the status is Delegated, then adds each element in delegatedTo
     * to the delegatedTo vector of that attendee.
     * If that attendee does not exist in the attendee list, then add
     * a new Attendee to the attendee list with his/her PARTSTAT set to status.
     * @param           sAttendeeFilter     attendee to set status on
     * @param           status              status to set 
     * @param           delegatedTo         vector of delegatedTo names to set to 
     */
    void setAttendeeStatus(UnicodeString & sAttendeeFilter, Attendee::STATUS status,
        JulianPtrArray * delegatedTo = 0);
    void setAttendeeStatusInt(UnicodeString & sAttendeeFilter, t_int32 status,
        JulianPtrArray * delegatedTo = 0);

    /**
     * TODO: doesn't do smart overriding for now, does simple overwrite
     *  Updates this component with the data from the updatedComponent.
     *  This method must be overridden by subclasses.
     *  If updatedComponent is older that current compoenent, don't update current component
     *  How:
     *     make sure (UID, RecurID) pair are same.
     *     if sequence number of updatedComponent if greater that this.sequence ||
     *     if sequence numbers are equal and update.DTSTAMP > this.DTSTAMP
     *        update properties
     *     else
     *         do nothing.
     * Return TRUE if component was changed, FALSE otherwise
     *
     */
    t_bool updateComponent(ICalComponent * updatedComponent);

    /**
     * Helper method called by updateComponent to actually replace
     * the property data-members with updatedComponent's data-members.
     * @param           ICalComponent * updatedComponent
     *
     * @return          virtual void 
     */
    virtual void updateComponentHelper(TimeBasedEvent * updatedComponent);

    /**
     * Compare TimeBasedEvents by UID.  Used by JulianPtrArray::QuickSort method.
     * @param           a       first TimeBasedEvent
     * @param           b       second TimeBasedEvent
     *
     * @return          a.getUID().compareTo(b.getUID())
     */
    static int CompareTimeBasedEventsByUID(const void * a, const void * b);
    
    /**
     * Compare TimeBasedEvents by DTSTART.  Used by JulianPtrArray::QuickSort method.
     * @param           a       first TimeBasedEvent
     * @param           b       second TimeBasedEvent
     *
     * @return          a.getDTSTART().compareTo(b.getDTSTART())
     */
    static int CompareTimeBasedEventsByDTStart(const void * a, const void * b);


protected:
  
    /**
     * store the data, depending on property name, property value
     * parameter names, parameter values, and the current line.
     * subclasses should overwrite this method, but call this method
     * to handle most properties.  If this method returns FALSE, subclass
     * then should handles specific properties of that class.
     *
     * @param       strLine         current line to process
     * @param       propName        name of property
     * @param       propVal         value of property
     * @param       parameters      property's parameters
     * @param       vTimeZones      vector of timezones
     * @return      TRUE if line was handled, FALSE otherwise
     */
    virtual t_bool storeData(UnicodeString & strLine, 
        UnicodeString & propName, UnicodeString & propVal,
        JulianPtrArray * parameters, JulianPtrArray * vTimeZones);
public:
    typedef void (TimeBasedEvent::*SetOp) (UnicodeString & strLine, UnicodeString & propVal, 
        JulianPtrArray * parameters, JulianPtrArray * vTimeZones);

    /* Clients should NOT call below methods */
    void storeAttach(UnicodeString & strLine, UnicodeString & propVal,
        JulianPtrArray * parameters, JulianPtrArray * vTimeZones);
    void storeAttendees(UnicodeString & strLine, UnicodeString & propVal,
        JulianPtrArray * parameters, JulianPtrArray * vTimeZones);
    void storeCategories(UnicodeString & strLine, UnicodeString & propVal,
        JulianPtrArray * parameters, JulianPtrArray * vTimeZones);
    void storeClass(UnicodeString & strLine, UnicodeString & propVal,
        JulianPtrArray * parameters, JulianPtrArray * vTimeZones);
    void storeComment(UnicodeString & strLine, UnicodeString & propVal,
        JulianPtrArray * parameters, JulianPtrArray * vTimeZones);
    void storeContact(UnicodeString & strLine, UnicodeString & propVal,
        JulianPtrArray * parameters, JulianPtrArray * vTimeZones);
    void storeCreated(UnicodeString & strLine, UnicodeString & propVal,
        JulianPtrArray * parameters, JulianPtrArray * vTimeZones);
    void storeDescription(UnicodeString & strLine, UnicodeString & propVal,
        JulianPtrArray * parameters, JulianPtrArray * vTimeZones);
    void storeDTStart(UnicodeString & strLine, UnicodeString & propVal,
        JulianPtrArray * parameters, JulianPtrArray * vTimeZones);
    void storeDTStamp(UnicodeString & strLine, UnicodeString & propVal,
        JulianPtrArray * parameters, JulianPtrArray * vTimeZones);
    void storeExDate(UnicodeString & strLine, UnicodeString & propVal,
        JulianPtrArray * parameters, JulianPtrArray * vTimeZones);
    void storeExRule(UnicodeString & strLine, UnicodeString & propVal,
        JulianPtrArray * parameters, JulianPtrArray * vTimeZones);
    void storeLastModified(UnicodeString & strLine, UnicodeString & propVal,
        JulianPtrArray * parameters, JulianPtrArray * vTimeZones);
    void storeOrganizer(UnicodeString & strLine, UnicodeString & propVal,
        JulianPtrArray * parameters, JulianPtrArray * vTimeZones);
    void storeRDate(UnicodeString & strLine, UnicodeString & propVal,
        JulianPtrArray * parameters, JulianPtrArray * vTimeZones);
    void storeRRule(UnicodeString & strLine, UnicodeString & propVal,
        JulianPtrArray * parameters, JulianPtrArray * vTimeZones);
    void storeRecurrenceID(UnicodeString & strLine, UnicodeString & propVal,
        JulianPtrArray * parameters, JulianPtrArray * vTimeZones);
    void storeRelatedTo(UnicodeString & strLine, UnicodeString & propVal,
        JulianPtrArray * parameters, JulianPtrArray * vTimeZones);
    void storeRequestStatus(UnicodeString & strLine, UnicodeString & propVal,
        JulianPtrArray * parameters, JulianPtrArray * vTimeZones);
    void storeSequence(UnicodeString & strLine, UnicodeString & propVal,
        JulianPtrArray * parameters, JulianPtrArray * vTimeZones);
    void storeStatus(UnicodeString & strLine, UnicodeString & propVal,
        JulianPtrArray * parameters, JulianPtrArray * vTimeZones);
    void storeSummary(UnicodeString & strLine, UnicodeString & propVal,
        JulianPtrArray * parameters, JulianPtrArray * vTimeZones);
    void storeUID(UnicodeString & strLine, UnicodeString & propVal,
        JulianPtrArray * parameters, JulianPtrArray * vTimeZones);
    void storeURL(UnicodeString & strLine, UnicodeString & propVal,
        JulianPtrArray * parameters, JulianPtrArray * vTimeZones);

    void ApplyStoreOp(void (TimeBasedEvent::*op) (UnicodeString & strLine, 
        UnicodeString & propVal, JulianPtrArray * parameters, JulianPtrArray * vTimeZones), 
        UnicodeString & strLine, UnicodeString & propVal, 
        JulianPtrArray * parameters, JulianPtrArray * vTimeZones)
    {
        (this->*op)(strLine, propVal, parameters, vTimeZones);
    }
protected:
    /**
     * Sets default data.  Currently does following:
     * 1) Sets Summary to first 60 characters of Description if Summary is empty.
     * 2) Sets sequence number to 0 if sequence number is less than 0.
     * 3) Sets CLASS to PUBLIC if Class is empty or out of range.
     */
    virtual void selfCheck();

    /* 
    void checkRecurrence();
    virtual void checkRange();
    void setDefaultProps(UnicodeString sPropName);
    */
    
private:

    /**
     * Generates vector of DateTimes representing dates of 
     * recurrence.  Does this given starting time, vector of rrule,
     * rdate, exrule, exdate and bound.  Also takes optional
     * timezones vector to adjust rdate, exdate and rrule, exrule 
     * (TODO: make rrules, exrules use timezones)
     * and a log file to log recurrence errors.
     * Used by createRecurrenceEvents method.
     * @param           vOut            output vector of dates of recurrence
     * @param           start           starting time of recurrence
     * @param           vRRules         vector of RRULES (UnicodeString)
     * @param           vExRules        vector of EXRULES (UnicodeString)
     * @param           vRDatesProp     vector of RDATES (ICalProperty)
     * @param           vExDatesProp    vector of EXDATES  (ICalProperty)
     * @param           iBound          bound value
     * @param           vTimeZones      vector of timezones 
     * @param           log             log file
     */
    static void generateDates(JulianPtrArray * vOut, DateTime start, 
        JulianPtrArray * vRRules, JulianPtrArray * vExRules,
        JulianPtrArray * vRDatesProp, JulianPtrArray * vExDatesProp, t_int32 iBound,
        JulianPtrArray * vTimeZones = 0, JLog * log = 0);

    /**
     * Takes in either RDate or ExDate ICalProperty vector, and fills in vOut with
     * DateTime strings from vector.  Also applies TimeZone information to
     * strings when necessary.  A flag to check if vector is RDate or ExDate 
     * is necessary because RDate can have Period, Date info, while ExDate cannot.
     * For example, if RDate vector contains RDATE;TZID=America/New-York:19971110T070000,
     * then bIsRDate must be TRUE (caller responsible for this), and vOut contains
     * the string "19971110T120000Z" (note timezones is applied).
     * Used by generateDates method.
     * @param           vOut        output vector of DateTime strings
     * @param           vDateProp   ICalProperty vector to split.
     * @param           bIsRDate    whether vector if RDate or EXDate
     * @param           vTimeZones  vector of timezones
     */
    static void splitDates(JulianPtrArray * vOut,
        JulianPtrArray * vDateProp, t_bool bIsRDate,
        JulianPtrArray * vTimeZones);


    /*
    JulianPtrArray * getPeriodDatesHelper(JulianPtrArray * vDates);
    JulianPtrArray * getPeriodDates(JulianPtrArray * vTimeZones);
    */

    /**
     * Takes RDate vector and returns all strings of RDates that are
     * of PERIOD value.  For example if RDATE;VALUE=PERIOD:19971110T112233Z/PT1H,
     * return the string "19971110T112233Z/PT1H".
     * @param           out     output vector of strings of PERIOD RDates 
     */
    void getPeriodRDates(JulianPtrArray * out);


    /**
     * Takes vector of recurrences dates and generates recurrence events in vOut.
     * Used by createRecurrenceEvents method.
     * @param           vOut        output vector of recurrence events
     * @param           dates       vector of recurrence dates
     * @param           origStart   starting time of original compressed event
     * @param           vTimeZones  vector of timezones
     */
    void populateDates(JulianPtrArray * vOut, JulianPtrArray * dates, 
        DateTime origStart, JulianPtrArray * vTimeZones);

    /**
     * Returns TRUE if this TimeBasedEvent's ID is the same as the component's
     * ID.  ID is represented as the (UID, RecurrenceID) pair.
     * If UID's on this and component do NOT match return FALSE;
     * If there is no RecurrenceID on this TBE and component and UID matches,
     * return TRUE;
     * If there is a RecurrenceID on EITHER this or component, if both have
     * RecurrenceID and they MATCH return TRUE.  if don't match return FALSE.
     * If one has RecurrenceID and other doesn't return FALSE.
     * @param           TimeBasedEvent * component
     *
     * @return          t_bool 
     */
    t_bool isExactMatchingID(TimeBasedEvent * component);

#if 0
    /**
     * Returns TRUE if this component is more recent than component.
     * Rules To define recent:
     *          Returns compare(Sequence Num) else if equal
     *             Returns compare(DTSTAMP)
     *          
     * @param           TimeBasedEvent * component
     *
     * @return          t_bool 
     */
    t_bool isMoreRecent(TimeBasedEvent * component);
#endif

    /* -- DATA MEMBERS */

protected:
    /* log file ptr */
    JLog * m_Log;

    JulianString m_sCurl;   /* the calendar url of this component, identifies its cal store */

private:

    /* recurrence bound value */
    static t_int32 ms_iBound;  

    /* method name of calendar */
    UnicodeString m_sMethod;

    /* file loaded from */
    UnicodeString m_sFileName;

    /* DTStart of first instance of a recurrence */
    DateTime m_origDTStart;

    /* DTStart of my instance of a recurrence */
    DateTime m_origMyDTStart;
  
    /* TRUE = Anniversary event, default is FALSE */
    t_bool  m_bAllDayEvent;

    /*------------ SUBCOMPONENTS ---------------*/
    JulianPtrArray *  m_AlarmsVctr;
    /*------------ PROPERTIES --- (VALUE VALUE TYPES in COMMENTS) -------------------------*/

    JulianPtrArray *    m_AttachVctr;       /* URL */
    JulianPtrArray *    m_AttendeesVctr;    /* CAL-ADDRESS */
    JulianPtrArray *    m_CategoriesVctr;   /* TEXT */
    ICalProperty *      m_Class;            /* class keyword */
    JulianPtrArray *    m_CommentVctr;      /* TEXT */
    JulianPtrArray *    m_ContactVctr;      /* TEXT OR URL */
    ICalProperty *      m_Created;          /* DATETIME */
    ICalProperty *      m_Description;      /* TEXT */  
    ICalProperty *      m_DTStart;          /* DATETIME OR DATE */
    ICalProperty *      m_DTStamp;          /* DATETIME */
    JulianPtrArray *    m_ExDateVctr;       /* DATETIME */
    JulianPtrArray *    m_ExRuleVctr;       /* RECURRENCE */ 
    ICalProperty *      m_LastModified;     /* DATETIME */
    ICalProperty *      m_Organizer;        /* CAL-ADDRESS */
    JulianPtrArray *    m_RDateVctr;        /* DATETIME, DATE, OR PERIOD */
    JulianPtrArray *    m_RRuleVctr;        /* RECURRENCE */
    ICalProperty *      m_RecurrenceID;     /* DATETIME */
    JulianPtrArray *    m_RelatedToVctr;    /* TEXT (must be a UID) */
    /*ICalProperty *      m_RequestStatus; */
    JulianPtrArray *    m_RequestStatusVctr;    /* 3digit number, error Description UnicodeString, optional exception text */
    ICalProperty *      m_Sequence;         /* INTEGER >= 0 */
    ICalProperty *	    m_Status;           /* status keyword */   
    ICalProperty *      m_Summary;          /* TEXT */
    ICalProperty *      m_UID;              /* TEXT */
    ICalProperty *      m_URL;              /* TEXT OR URL */
    JulianPtrArray *    m_XTokensVctr;      /* TEXT */

  /*------------ END PROPERTIES ------------------------------------------*/
};

#endif /* __TIMEBASEDEVENT_H_ */

