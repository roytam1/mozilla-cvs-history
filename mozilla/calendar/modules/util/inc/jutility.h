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
 * jutility.h
 * John Sun
 * 2/3/98 10:12:32 AM
 */

#ifndef __NSCALUTILITY_H_
#define __NSCALUTILITY_H_

#include <unistring.h>
#include <calendar.h>
#include "ptrarray.h"
#include "jatom.h"
#include "nscalutilexp.h"

/** 
 * A class that contains static utility methods.
 * Also defines the RecurrenceType, which is used by the
 * generator and Recurrence classes for defining span and interval lengths.
 */
class NS_CAL_UTIL nsCalUtility
{
private:

    /**
     * Hide constructor from clients 
     */
    nsCalUtility();

public:
    
    /**
     * Define the Valid Mime-Encoding types for ICAL.
     */
    enum MimeEncoding
    {
        MimeEncoding_7bit = 0,
        MimeEncoding_Base64 = 1,
        MimeEncoding_QuotedPrintable = 2
    };

    /**
     * Define the types used by Recurrence and generator class for
     * defining span and interval lengths.
     */
    enum RecurrenceType
    {
        RT_NONE = 0,
        RT_MINUTELY = Calendar::MINUTE,
        RT_HOURLY = Calendar::HOUR,
        RT_DAILY = Calendar::DAY_OF_YEAR,
        RT_WEEKLY = Calendar::WEEK_OF_YEAR,
        RT_MONTHLY = Calendar::MONTH,
        RT_YEARLY = Calendar::YEAR
    };

#if 0
    /**
     * a wrapper around atoi(or atol).
     * @param   nPtr            the string to translate
     * @param   bParseError     return TRUE if error in parse, otherwise, no -change
     * @param   size            length of string
     * @return                  the value of the string in a 32-bit number
     */
    static t_int32 atot_int32(const char * nPtr, t_bool & bParseError, t_int32 size);
#endif

    /**
     * a wrapper around atoi(or atol).  Faster version.
     * @param   nPtr            the string to translate
     * @param   bParseError     return TRUE if error in parse, otherwise, no -change
     * @param   size            length of string
     * @return                  the value of the string in a 32-bit number
     */
    static t_int32 atot_int32(char * nPtr, t_bool & bParseError, t_int32 size);
    

    /**
     * Checks to see if hashCode value is equal to any element in range. 
     * @param           hashCode    hashCode to search for
     * @param           range[]     range of atoms to search from
     * @param           rangeSize   length of range
     *
     * @return          TRUE if hashCode in range, FALSE otherwise
     */
    static t_bool checkRange(t_int32 hashCode, JAtom range[], t_int32 rangeSize);

    /*static char * unistrToStr(const UnicodeString & us);
    static void TRACE_DateTimeVector(JulianPtrArray & vector);
    static const UnicodeString & ToUpper(const UnicodeString & eightBitString);*/

    /**
     * Strip double quotest from the beginning and end of string
     * if it has double-quotes at beginning and end.
     * @param           u       string to strip double-quotes from
     */
    static void stripDoubleQuotes(UnicodeString & u);

    /**
     * Inserts double quotes at start and end of string if 
     * string != "".
     * @param           us      string to add double-quotes to
     *
     * @return          string with double-quotes at start, end (us)
     */
    static UnicodeString & addDoubleQuotes(UnicodeString & us);
};

#endif /* __NSCALUTILITY_H_ */

