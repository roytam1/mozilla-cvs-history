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

#include "stdafx.h"

#include "jdefines.h"
#include <unistring.h>
#include "jutility.h"
#include "ptrarray.h"

//---------------------------------------------------------------------

nsCalUtility::nsCalUtility() {}

//---------------------------------------------------------------------

//t_int32 nsCalUtility::atot_int32(const char * nPtr,
t_int32 nsCalUtility::atot_int32(char * nPtr,
                                  t_bool & bParseError,
                                  t_int32 size)
{
    t_int32 i;
    char * ptr = nPtr;
    char c;
    for (i = 0; i < size; i++)
    {
        if (!isdigit(*ptr))
        {
            bParseError = TRUE; // or the ERROR to TRUE
            break;
        }
        ptr++;
    }
    c = nPtr[size];
    nPtr[size] = '\0';
    i = atol(nPtr);
    nPtr[size] = c;
    return i;
}
//---------------------------------------------------------------------
#if 0
t_int32 nsCalUtility::atot_int32(const char * nPtr,
                                  t_bool & bParseError,
                                  t_int32 size)
{
    t_int32 i;
    const char * ptr = nPtr;

    for (i = 0; i < size; i++)
    {
        if (!isdigit(*ptr))
        {
            bParseError |= TRUE; // or the ERROR to TRUE
            break;
        }
        ptr++;
    }
    return atol(nPtr);
}
#endif
//---------------------------------------------------------------------
t_bool nsCalUtility::checkRange(t_int32 hashCode, JAtom range[],
                                 t_int32 rangeSize)
{
    t_int32 i;
    for (i = 0; i < rangeSize; i++)
    {
        if (range[i] == hashCode)
            return TRUE;
    }
    return FALSE;
}

//---------------------------------------------------------------------

void 
nsCalUtility::stripDoubleQuotes(UnicodeString & u)
{
    if (u.size() > 0)
    {
        if ('\"' == u[(TextOffset) (u.size() - 1)])
        {
            u.remove((TextOffset)(u.size() - 1), 1);
        }
        if (u.size() > 0)
        {
            if ('\"' == u[(TextOffset) 0])
            {
                u.remove(0, 1);
            }
        }
    }
}

//---------------------------------------------------------------------

UnicodeString &
nsCalUtility::addDoubleQuotes(UnicodeString & us)
{
    if (us.size() != 0)
    {
        us += '\"';
        us.insert(0, "\"");
    }
    return us;
}

//---------------------------------------------------------------------
/*
char * nsCalUtility::unistrToStr(const UnicodeString & us)
{
    return us.toCString("");
}
*/
//---------------------------------------------------------------------
/*
UnicodeString & nsCalUtility::ToUpper(const UnicodeString & eightBitString)
{
    t_int32 i;
    //
    char c;
    for (i = 0; i < eightBitString.size(); i++)
    {
        c = (char) eightBitString[(TextOffset) i];
        if (islower(c))
        {
            c = (char) toupper(c);
            eightBitString.replace((TextOffset) i, 1, &c);
        }
    }
    //
    char * c = eightBitString.toCString("");
    for (i = 0; i < eightBitString.size(); i++)
    {
        if (islower(c[i]))
        {
            c[i] = (char) toupper(c[i]);
        }
    }
    eightBitString = c;
    return eightBitString;
}
*/
//---------------------------------------------------------------------

/*
void nsCalUtility::TRACE_DateTimeVector(JulianPtrArray & vector)
{
      for (i = 0; i < vector.GetSize(); i++)         
      {
          DateTime * dt = (DateTime *) dateVector.GetAt(i); 
          if (TRUE) TRACE("dtVec[%d] = %s\r\n", i, dt->toISO8601().toCString(""));
      }
}
*/
//---------------------------------------------------------------------


