/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- 
 * 
 * The contents of this file are subject to the Netscape Public License 
 * Version 1.0 (the "NPL"); you may not use this file except in 
 * compliance with the NPL.  You may obtain a copy of the NPL at 
 * http://www.mozilla.org/NPL/ 
 * 
 * Software distributed under the NPL is distributed on an "AS IS" basis, 
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL 
 * for the specific language governing rights and limitations under the 
 * NPL. 
 * 
 * The Initial Developer of this code under the NPL is Netscape 
 * Communications Corporation.  Portions created by Netscape are 
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights 
 * Reserved. 
 */

/* 
 * txnobj.h
 * John Sun
 * 3/10/98 11:35:47 AM
 */

#ifndef __TRANSOBJ_H_
#define __TRANSOBJ_H_

#include "nscal.h"
#include "user.h"
#include "form.h"
/*#include "xp.h"
#include "form.h"*/

/*#ifdef XP_WIN
#ifndef CAPI_READY
#define CAPI_READY 1
#endif
#endif*/

#if CAPI_READY
#include <capi.h>
#endif /* #if CAPI_READY */

#if CAPI_READY
typedef struct
{
    char * p;
    int iSize;
} CAPI_CTX;
#endif

class TransactionObject
{
public:
    enum EFetchType 
    {
        EFetchType_UID = 0,
        EFetchType_DateRange = 1,
        EFetchType_AlarmRange = 2
        ,EFetchType_NONE = 3
    };

    /**
     *  The allowable transport types.
     *  CAPI, IRIP, IMIP, and FCAPI (local file CAPI).
     */
    enum ETransportType
    {
        ETransportType_CAPI = 0,
        ETransportType_IRIP = 1,
        ETransportType_IMIP = 2,
        ETransportType_FCAPI = 3
    };

/*
    enum ETXN_TYPE
    {
        TR_TYPE_CANCEL = 0,
        TR_TYPE_COUNTER = 1,
        TR_TYPE_DECLINECOUNTER = 2,
        TR_TYPE_DELEGATE = 3,
        TR_TYPE_FORWARD = 4,
        TR_TYPE_GET = 5,
        TR_TYPE_REFRESH = 6,
        TR_TYPE_REPLY = 7,
        TR_TYPE_REQUEST = 8,
        TR_TYPE_PUBLISH = 9,
        TR_TYPE_INVALID = -1
    };
*/
    enum ETxnErrorCode
    {
        TR_OK = 0,
        TR_CAPI_LOGIN_FAILED = 100,
        TR_CAPI_HANDLE_CAPI_FAILED = 101,
        TR_CAPI_LOGOFF_FAILED = 102,
        TR_CAPI_FETCH_BAD_ARGS = 103,
        TR_IRIP_FAILED = 200,
        TR_IMIP_FAILED = 300
    };

    enum ETxnType
    {
        ETxnType_STORE = 0,
        ETxnType_DELETE = 1,
        ETxnType_GET = 2
    };

protected:

    /* calendar containing METHOD, other header info */
    NSCalendar * m_Calendar;
    
    /* vector of components in transaction */
    JulianPtrArray * m_ICalComponentVctr;

    /* capi arguments - (i.e. UIDs, date range values */
    JulianPtrArray * m_Modifiers;

    /* FROM: User - usually the logged in User */
    User * m_User;

    /* TO: Recipients */
    JulianPtrArray * m_Recipients;

    /* Attendee to filter on a reply */
    UnicodeString m_AttendeeName;

    /* subject of mail message */
    UnicodeString m_Subject;

    /* context windows to send to */
    MWContext * m_Context;

    /* julain form which contains function ptrs */
    JulianForm * m_JulianForm;

    /* TODO: move the CAPI stuff somewhere*/
  /*  char * m_userxstring;
    char * m_password;
    char * m_hostname;
    char * m_node;
    char * m_recipientxstring;*/

    /* Fetch Type */
    EFetchType m_FetchType;   

    /**
     * Prints each ICalComponent in the components vector in iCal
     * format.  The output string will depend on the method name,
     * the name of the attendee to filter on, the delegate to 
     * filter on, and whether to print as recurring or not.
     *
     * @param   components  vector of ICalComponent's to print
     * @param   method      ITIP method name of message
     * @param   name        attendee to filter on
     * @param   isRecurring TRUE = no recid, FALSE = print recid
     * @param   out         output string
     *
     * @return  output string (out)
     */
    static UnicodeString & printComponents(JulianPtrArray * components,
        UnicodeString & method, UnicodeString & name, 
        t_bool isRecurring, UnicodeString & out);
    
    /** hide from clients */
    TransactionObject();

    UnicodeString & MakeITIPMessage(UnicodeString & out);

private:
    /**
     * Run the executeCAPI, executeIMIP, executeIRIP, executeFCAPI
     * depending on order of transports.
     */
    ETxnErrorCode executeHelper(ETransportType * transports,
        t_int32 transportsSize, JulianPtrArray * out);


    /**
     * For each recipient, if status[i] is TRUE, then remove
     * recipient[i] from recipients vector.  Assumes that
     * recipients has same size as statusSize.
     * @param           JulianPtrArray * recipients
     * @param           t_bool * status
     * @param           t_int32 statusSize
     *
     * @return          static void 
     */
    static void removeSuccessfulUsers(JulianPtrArray * recipients,
                                         t_bool * status,
                                         t_int32 statusSize);


    /**
     * For each boolean in status, set status[i] to FALSE.
     */
    static void setStatusToFalse(t_bool * status, t_int32 statusSize);


    /**
     * Creates the content type header for the IMIP message in the form
     * Content-Type: text/calendar method=PUBLISH; charset=UTF-8; component=VEVENT
     * @param           UnicodeString & sMethod
     * @param           UnicodeString & sCharset
     * @param           UnicodeString & sComponentType
     * @param           UnicodeString & sContentTypeHeader
     *
     * @return          static UnicodeString 
     */
    static UnicodeString & createContentTypeHeader(UnicodeString & sMethod,
        UnicodeString & sCharset, UnicodeString & sComponentType,
        UnicodeString & sContentTypeHeader);


public:

    /* used for debugging only : TODO: remove later*/
    UnicodeString m_DebugITIPMessage;

    /**
     * Clients should not delete cal, components, user, recipients, jf, context
     * I assume components are processed and ready to go (i.e. DTStamp already set).
     * the cal arg should have the calendar that contains calendar information for the
     * components.  This includes the METHOD, PRODID, and VERSION.
     * The components contains the ICalComponents that should be sent.
     * The user is the name of the user who sends the transaction.  For CAPI, this
     * would be who logs in.  For IMIP, this would be the FROM: person.
     * The recipients vector contains the User object to store the components to.
     * For CAPI, this would be the handles to get.  For IMIP, this would be the TO: persons.
     * The subject contains the subject on an IMIP message.
     * The modifiers should contain various information depending on the CAPI call.
     * For a DELETE the modifiers should be in the following order
     *  UID, [Recurrence-ID], [("THISANDPRIOR" or "THISANDFUTURE")]
     * For a FETCH by UID, the modifiers should be in the following order
     *  UID, [Recurrence-ID], [("THISANDPRIOR", or "THISANDFUTURE")]
     * For a FETCH by DateRange and a FETCH by AlarmRange, the modifiers should be
     *  DTSTART, DTEND
     * @param           NSCalendar & cal
     * @param           JulianPtrArray & components
     * @param           User & user
     * @param           JulianPtrArray & recipients
     * @param           UnicodeString & subject
     * @param           JulianPtrArray & modifiers
     * @param           UnicodeString & attendeeName
     */
    TransactionObject(NSCalendar & cal, JulianPtrArray & components,
        User & user, JulianPtrArray & recipients, UnicodeString & subject,
        JulianPtrArray & modifiers,
        JulianForm * jf, MWContext * context,
        UnicodeString & attendeeName, EFetchType fetchType = EFetchType_NONE);

    virtual ~TransactionObject();

    /* execute the transaction.  calls the 
     * executeCAPI, executeIRIP, executeIMIP */
    void execute(JulianPtrArray * out, ETxnErrorCode & status);

    ETxnErrorCode executeIRIP(JulianPtrArray * out, t_bool * outStatus, 
        t_int32 outStatusSize);
    ETxnErrorCode executeIMIP(JulianPtrArray * out, t_bool * outStatus, 
        t_int32 outStatusSize);
    ETxnErrorCode executeFCAPI(JulianPtrArray * out, t_bool * outStatus, 
        t_int32 outStatusSize);

    virtual ETxnType GetType() const
    { 
        /* Should always assert out, pure virtual method. */
        PR_ASSERT(FALSE); return ETxnType_STORE; 
    }
#if CAPI_READY


    static ETxnErrorCode FetchEventsByRange(
        User * loggedInUser,              /* i: logged in user (use this session) */
        JulianPtrArray * usersToFetchOn,  /* i: users to fetch on (use these handles) */
        DateTime dtStart,                 /* i: start of fetch range */
        DateTime dtEnd,                   /* i: end of fetch range */
        JulianPtrArray * eventsToFillIn); /* o: fill in vector with events to fetch */

    ETxnErrorCode executeCAPI(JulianPtrArray * outCalendars, t_bool * outStatus, 
        t_int32 outStatusSize);

    /* TODO: pure virtual handleCAPI interface needs to be changed to reflect newest CAPI spec */
    virtual CAPIStatus handleCAPI(
        CAPISession & pS, 
        CAPIHandle * pH, 
        t_int32 iHandleCount, 
        t_int32 lFlags, 
        JulianPtrArray * inComponents, 
        NSCalendar * inCal,
        JulianPtrArray * modifiers, 
        JulianPtrArray * outCalendars, 
        TransactionObject::EFetchType & out)
    {
        PR_ASSERT(FALSE);
        CAPIStatus foo;
        return foo;
    }

    /* s = CAPISession
     ppH = list of CAPIHandles
     iHandleCount = number of valid handles in ppH.
     lFlags = bit flags
     modifiers should hold following:
     for StoreEvent: nothing
     for FetchByID:  uid, recurrence-ID, range modifier (THISANDPRIOR,THISANDFUTURE,NONE)
     for FetchByRange: start, end
     for DeleteEvent: uid, recurrence-ID
     for FetchByAlarmRange: start,end
     --------------------------------
     propList is list of properties to return in events
     stream is CAPIStream to read/write data*/

    /*TODO: newer CAPI interface*/
    /*
    virtual CAPIStatus handleCAPI(CAPISession * s, CAPIHandle ** ppH,
        t_int32 iHandleCount, long lFlags, JulianPtrArray * modifiers,
        JulianPtrArray * propList, CAPIStream * stream);
    */
#if 0
    void setCAPIInfo(char * userxstring, char * password,
         char * hostname, char * node, char * recipientxstring);
#endif
#endif /* #if CAPI_READY */

};

#endif /* __TRANSOBJ_H_ */

