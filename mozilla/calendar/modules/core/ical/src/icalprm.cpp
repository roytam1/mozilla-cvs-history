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

// icalprm.cpp
// John Sun
// 3:41 PM February 10 1998

#include "stdafx.h"
#include "jdefines.h"

#include "icalprm.h"
#include "ptrarray.h"
#include "keyword.h"

//---------------------------------------------------------------------

ICalParameter::ICalParameter() {}

//---------------------------------------------------------------------

ICalParameter::ICalParameter(ICalParameter & that)
{
    m_ParameterName = that.m_ParameterName;
    m_ParameterValue = that.m_ParameterValue;
}

//---------------------------------------------------------------------

ICalParameter::ICalParameter(const UnicodeString & parameterName, 
                             const UnicodeString & parameterValue)
{
    m_ParameterName = parameterName;
    m_ParameterValue = parameterValue;
}
//---------------------------------------------------------------------
ICalParameter * ICalParameter::clone()
{
    return new ICalParameter(*this);
}
//---------------------------------------------------------------------

void 
ICalParameter::setParameterName(const UnicodeString & parameterName)
{
    m_ParameterName = parameterName;
}

//---------------------------------------------------------------------

void 
ICalParameter::setParameterValue(const UnicodeString & parameterValue)
{
    m_ParameterValue = parameterValue;
}

//---------------------------------------------------------------------

UnicodeString & 
ICalParameter::getParameterName(UnicodeString & retName) const
{
    retName = m_ParameterName;
    return retName;
}

//---------------------------------------------------------------------

UnicodeString & 
ICalParameter::getParameterValue(UnicodeString & retValue) const
{
    retValue = m_ParameterValue;
    return retValue;
}

//---------------------------------------------------------------------

UnicodeString & 
ICalParameter::toICALString(UnicodeString & result) const
{
    result = nsCalKeyword::Instance()->ms_sSEMICOLON_SYMBOL;
    result += m_ParameterName;
    result += '=';
    result += m_ParameterValue;
    return result;
}

//---------------------------------------------------------------------

const ICalParameter & 
ICalParameter::operator=(const ICalParameter & d)
{
    if (this != &d)
    {
        m_ParameterName = d.m_ParameterName;
        m_ParameterValue = d.m_ParameterValue;
    }
    return *this;
}

//---------------------------------------------------------------------
UnicodeString & 
ICalParameter::GetParameterFromVector(UnicodeString & paramName, 
                                      UnicodeString & retVal,
                                      JulianPtrArray * parameters)
{
    t_int32 i;
    ICalParameter * ip;
    if (parameters != 0)
    {
        UnicodeString u;
        for (i = 0; i < parameters->GetSize(); i++)
        {
            ip = (ICalParameter *) parameters->GetAt(i);
            if (paramName.compareIgnoreCase(ip->getParameterName(u)) == 0)
            {
                retVal = ip->getParameterValue(retVal);
                return retVal;
            }
        }
    }
    retVal = "";
    return retVal;
}
//---------------------------------------------------------------------

