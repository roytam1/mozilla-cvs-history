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

// attendee.cpp
// John Sun
// 3:21 PM February 6 1998

#include "stdafx.h"
#include "jdefines.h"

#include "icalcomp.h"
#include "attendee.h"
#include "jutility.h"
#include "unistrto.h"
#include "jlog.h"
#include "keyword.h"
#include "uri.h"

//--------------------------------------------------------------------
const t_int32 Attendee::ms_iDEFAULT_ROLE = ROLE_REQ_PARTICIPANT; //ROLE_ATTENDEE;
const t_int32 Attendee::ms_iDEFAULT_TYPE= TYPE_INDIVIDUAL; //TYPE_UNKNOWN;
const t_int32 Attendee::ms_iDEFAULT_STATUS= STATUS_NEEDSACTION;
const t_int32 Attendee::ms_iDEFAULT_RSVP= RSVP_FALSE;
const t_int32 Attendee::ms_iDEFAULT_EXPECT= EXPECT_FYI;

const t_int32 Attendee::ms_cAttendeeName          = 'N';
const t_int32 Attendee::ms_cAttendeeRole          = 'R';
const t_int32 Attendee::ms_cAttendeeStatus        = 'S';
const t_int32 Attendee::ms_cAttendeeRSVP          = 'V';
const t_int32 Attendee::ms_cAttendeeType          = 'T';
const t_int32 Attendee::ms_cAttendeeExpect        = 'E';
const t_int32 Attendee::ms_cAttendeeDelegatedTo   = 'D';
const t_int32 Attendee::ms_cAttendeeDelegatedFrom = 'd';
const t_int32 Attendee::ms_cAttendeeMember        = 'M';
const t_int32 Attendee::ms_cAttendeeDir           = 'l'; // 'el'
const t_int32 Attendee::ms_cAttendeeSentBy        = 's';
const t_int32 Attendee::ms_cAttendeeCN            = 'C';
const t_int32 Attendee::ms_cAttendeeLanguage      = 'm';
const t_int32 Attendee::ms_cAttendeeDisplayName   = 'z';
// ---- END OF  STATIC DATA ---- //
//---------------------------------------------------------------------

t_bool Attendee::isValidStatus(ICalComponent::ICAL_COMPONENT compType,
                               Attendee::STATUS status)
{
    if (compType == ICalComponent::ICAL_COMPONENT_VEVENT)
    {
        if (status >= STATUS_NEEDSACTION && status <= STATUS_DELEGATED)
            return TRUE;
        else return FALSE;
    }
    else if (compType == ICalComponent::ICAL_COMPONENT_VTODO)
    {
        if (status >= STATUS_NEEDSACTION && status <= STATUS_INPROCESS)
            return TRUE;
        else return FALSE;
    }
    else 
    {
        // for VJournal and VFreebusy
        if (status >= STATUS_NEEDSACTION && status <= STATUS_DECLINED)
            return TRUE;
        else return FALSE;
    }
}

//---------------------------------------------------------------------

Attendee::Attendee()
{
    // private, NEVER use
    PR_ASSERT(FALSE);
}
  
//---------------------------------------------------------------------

Attendee::Attendee(ICalComponent::ICAL_COMPONENT componentType, JLog * initLog) 
: m_vsMember(0), m_vsDelegatedTo(0), m_vsDelegatedFrom(0),
  m_iRole(-1), m_iType(-1), m_iStatus(-1), m_iRSVP(-1),
  m_iExpect(-1), m_Log(initLog), m_ComponentType(componentType)
{
}

//---------------------------------------------------------------------

Attendee::Attendee(Attendee & that)
: m_vsMember(0), m_vsDelegatedTo(0), m_vsDelegatedFrom(0)
{
    t_int32 i, size;

    m_iRole = that.m_iRole;
    m_iType = that.m_iType;
    m_iStatus = that.m_iStatus;
    m_iRSVP = that.m_iRSVP;
    m_iExpect = that.m_iExpect;
    
    m_CN = that.m_CN;
    m_Language = that.m_Language;
    m_SentBy = that.m_SentBy;
    m_Dir = that.m_Dir;

    m_ComponentType = that.m_ComponentType;

    m_sName = that.m_sName;
    
    if (that.m_vsMember != 0)
    {
        m_vsMember = new JulianPtrArray(); PR_ASSERT(m_vsMember != 0);
        if (m_vsMember != 0)
        {
            size = that.m_vsMember->GetSize();
            for (i = 0; i < size; i++)
            {
                addMember(*((UnicodeString *) that.m_vsMember->GetAt(i)));
            }
        }
    }

    if (that.m_vsDelegatedTo != 0)
    {
        m_vsDelegatedTo = new JulianPtrArray(); PR_ASSERT(m_vsDelegatedTo != 0);
        if (m_vsDelegatedTo != 0)
        {
            size = that.m_vsDelegatedTo->GetSize();
            for (i = 0; i < size; i++)
            {
                addDelegatedTo(*((UnicodeString *) that.m_vsDelegatedTo->GetAt(i)));
            }
        }
    }

    if (that.m_vsDelegatedFrom != 0)
    {
        m_vsDelegatedFrom = new JulianPtrArray(); PR_ASSERT(m_vsDelegatedFrom != 0);
        if (m_vsDelegatedFrom != 0)
        {
            size = that.m_vsDelegatedFrom->GetSize();
            for (i = 0; i < size; i++)
            {
                addDelegatedFrom(*((UnicodeString *) that.m_vsDelegatedFrom->GetAt(i)));
            }
        }
    }

}

//---------------------------------------------------------------------

Attendee::~Attendee()
{
    if (m_vsDelegatedTo != 0) 
    { 
        ICalComponent::deleteUnicodeStringVector(m_vsDelegatedTo);
        delete m_vsDelegatedTo; m_vsDelegatedTo = 0; 
    }
    if (m_vsDelegatedFrom != 0) 
    { 
        ICalComponent::deleteUnicodeStringVector(m_vsDelegatedFrom);
        delete m_vsDelegatedFrom; m_vsDelegatedFrom = 0; 
    }
    if (m_vsMember != 0) 
    { 
        ICalComponent::deleteUnicodeStringVector(m_vsMember);
        delete m_vsMember; m_vsMember = 0; 
    }
}

//---------------------------------------------------------------------

ICalProperty * 
Attendee::clone(JLog * initLog)
{
    m_Log = initLog; 
    return (ICalProperty *) new Attendee(*this);
}

//---------------------------------------------------------------------

Attendee * Attendee::getDefault(ICalComponent::ICAL_COMPONENT compType,
                                JLog * initLog)
{
    Attendee * a = new Attendee(compType, initLog);   PR_ASSERT(a != 0);
    if (a != 0)
    {
        a->setRole((Attendee::ROLE) ms_iDEFAULT_ROLE);
        a->setType((Attendee::TYPE) ms_iDEFAULT_TYPE);
        a->setStatus((Attendee::STATUS) ms_iDEFAULT_STATUS);
        a->setRSVP((Attendee::RSVP) ms_iDEFAULT_RSVP);
        a->setExpect((Attendee::EXPECT) ms_iDEFAULT_EXPECT);
    }
    return a;
}

//---------------------------------------------------------------------

void Attendee::parse(UnicodeString & propVal, 
                     JulianPtrArray * parameters)
{
    t_int32 i;
    ICalParameter * param;
    UnicodeString pName, pVal;
    if (propVal.size() == 0)
    {
        return;
    }
    setName(propVal);
    if (parameters != 0)
    {
        for (i = 0; i < parameters->GetSize(); i++)
        {
            param = (ICalParameter *) parameters->GetAt(i);
           
            setParam(param->getParameterName(pName), 
                param->getParameterValue(pVal));
        }   
    }
    selfCheck();
}

//---------------------------------------------------------------------
void Attendee::addParamValList(UnicodeString & paramValList, t_int32 hashCode)
{
    UnicodeStringTokenizer * stMult = new UnicodeStringTokenizer(paramValList, 
        nsCalKeyword::Instance()->ms_sCOMMA_SYMBOL);
    PR_ASSERT(stMult != 0);
    if (stMult != 0)
    {
        UnicodeString u;
        UnicodeString uNext;
        ErrorCode status = ZERO_ERROR;
        while (stMult->hasMoreTokens())
        {
            if (uNext.size() == 0)  
                u = stMult->nextToken(u, status);
            else 
                u = uNext;
            if ('\"' != u[(TextOffset) (u.size() - 1)])     
            {
                while (stMult->hasMoreTokens())
                {
                    uNext = stMult->nextToken(uNext, status);
                    if ('\"' != uNext[(TextOffset) 0])
                    {
                        u += ',';
                        u += uNext;
                        uNext = "";
                    }
                    else
                        break;
                }
            }
            nsCalUtility::stripDoubleQuotes(u);  // double quote property
            
            if (nsCalKeyword::Instance()->ms_ATOM_DELEGATED_TO == hashCode)
            {
                addDelegatedTo(u);
            }
            else if (nsCalKeyword::Instance()->ms_ATOM_DELEGATED_FROM == hashCode)
            {
                addDelegatedFrom(u);
            }
            else
            {
                addMember(u);
            }
        }
        delete stMult; stMult = 0;
    }
}

//---------------------------------------------------------------------

void Attendee::setParam(UnicodeString & paramName, 
                        UnicodeString & paramVal)
{
    t_int32 i;
    
    //if (FALSE) TRACE("(%s, %s)\r\n", paramName.toCString(""), paramVal.toCString(""));
    if (paramName.size() == 0)
    {
        if (m_Log) m_Log->logError(
            nsCalLogErrorMessage::Instance()->ms_iInvalidParameterName, 
            nsCalKeyword::Instance()->ms_sATTENDEE, paramName, 200);
    }
    else
    {
        t_int32 hashCode = paramName.hashCode();

        if (nsCalKeyword::Instance()->ms_ATOM_ROLE == hashCode)
        {
            i = stringToRole(paramVal);
            if (i < 0)
            {
                if (m_Log) m_Log->logError(
                    nsCalLogErrorMessage::Instance()->ms_iInvalidParameterValue, 
                    nsCalKeyword::Instance()->ms_sATTENDEE, 
                    paramName, paramVal, 200);
            }
            else 
            {
                if (getRole() >= 0)
                {
                    if (m_Log) m_Log->logError(
                        nsCalLogErrorMessage::Instance()->ms_iDuplicatedParameter, 
                        nsCalKeyword::Instance()->ms_sATTENDEE, 
                        paramName, 100);
                }   
                setRole((Attendee::ROLE) i);
            }
        }
        else if (nsCalKeyword::Instance()->ms_ATOM_CUTYPE == hashCode)
        {
            i = stringToType(paramVal);
            if (i < 0)
            {
                if (m_Log) m_Log->logError(
                    nsCalLogErrorMessage::Instance()->ms_iInvalidParameterValue,
                    nsCalKeyword::Instance()->ms_sATTENDEE, 
                    paramName, paramVal, 200);
            }
            else 
            {
                if (getType() >= 0)
                {
                    if (m_Log) m_Log->logError(
                        nsCalLogErrorMessage::Instance()->ms_iDuplicatedParameter,
                        nsCalKeyword::Instance()->ms_sATTENDEE, 
                        paramName, 100);
                }
                setType((Attendee::TYPE) i);
            }
        }
        else if (nsCalKeyword::Instance()->ms_ATOM_PARTSTAT == hashCode)
        {
            i = stringToStatus(paramVal);
            if (i < 0 || !isValidStatus(m_ComponentType, (Attendee::STATUS) i))
            {
                if (m_Log) m_Log->logError(
                    nsCalLogErrorMessage::Instance()->ms_iInvalidParameterValue,
                    nsCalKeyword::Instance()->ms_sATTENDEE, 
                    paramName, paramVal, 200);
            }
            else 
            {
                if (getStatus() >= 0)
                {
                    if (m_Log) m_Log->logError(
                        nsCalLogErrorMessage::Instance()->ms_iDuplicatedParameter,
                        nsCalKeyword::Instance()->ms_sATTENDEE, 
                        paramName, 100);
                }
                setStatus((Attendee::STATUS) i);
            }
        }
        else if (nsCalKeyword::Instance()->ms_ATOM_EXPECT == hashCode)
        {
            i = stringToExpect(paramVal);
            if (i < 0)
            {
                if (m_Log) m_Log->logError(
                    nsCalLogErrorMessage::Instance()->ms_iInvalidParameterValue,
                    nsCalKeyword::Instance()->ms_sATTENDEE, 
                    paramName, paramVal, 200);
            }
            else 
            {
                if (getExpect() >= 0)
                {
                    if (m_Log) m_Log->logError(
                        nsCalLogErrorMessage::Instance()->ms_iDuplicatedParameter,
                        nsCalKeyword::Instance()->ms_sATTENDEE, 
                        paramName, 100);
                }
                setExpect((Attendee::EXPECT) i);
            }
        }
        else if (nsCalKeyword::Instance()->ms_ATOM_RSVP == hashCode)
        {
            i = stringToRSVP(paramVal);
            if (i < 0)
            {
                if (m_Log) m_Log->logError(
                    nsCalLogErrorMessage::Instance()->ms_iInvalidParameterValue,
                    nsCalKeyword::Instance()->ms_sATTENDEE, 
                    paramName, paramVal, 200);
            }
            else 
            {
                if (getRSVP() >= 0)
                {
                    if (m_Log) m_Log->logError(
                        nsCalLogErrorMessage::Instance()->ms_iDuplicatedParameter,
                        nsCalKeyword::Instance()->ms_sATTENDEE, 
                        paramName, 100);
                }
                setRSVP((Attendee::RSVP) i);
            }
        }
        else if (nsCalKeyword::Instance()->ms_ATOM_DELEGATED_TO == hashCode)
        {
            addParamValList(paramVal, hashCode);
        }
        else if (nsCalKeyword::Instance()->ms_ATOM_DELEGATED_FROM == hashCode)
        {
            addParamValList(paramVal, hashCode);
        }
        else if (nsCalKeyword::Instance()->ms_ATOM_MEMBER == hashCode)
        {
            addParamValList(paramVal, hashCode);
        }

        // Newer properties 3-23-98 (cn, sent-by (double-quote), dir (double-quote))
        else if (nsCalKeyword::Instance()->ms_ATOM_CN == hashCode)
        {
            if (getCN().size() != 0)
            {
                 if (m_Log) m_Log->logError(nsCalLogErrorMessage::Instance()->ms_iDuplicatedParameter,
                        nsCalKeyword::Instance()->ms_sATTENDEE, paramName, 100);
            }
            setCN(paramVal);
        }
        else if (nsCalKeyword::Instance()->ms_ATOM_LANGUAGE == hashCode)
        {
            if (getLanguage().size() != 0)
            {
                 if (m_Log) m_Log->logError(nsCalLogErrorMessage::Instance()->ms_iDuplicatedParameter,
                        nsCalKeyword::Instance()->ms_sATTENDEE, paramName, 100);
            }
            setLanguage(paramVal);
        }
        else if (nsCalKeyword::Instance()->ms_ATOM_SENTBY == hashCode)
        {
            if (getSentBy().size() != 0)
            {
                 if (m_Log) m_Log->logError(nsCalLogErrorMessage::Instance()->ms_iDuplicatedParameter,
                        nsCalKeyword::Instance()->ms_sATTENDEE, paramName, 100);
            }
            nsCalUtility::stripDoubleQuotes(paramVal);  // double quote property
            setSentBy(paramVal);
        }
        else if (nsCalKeyword::Instance()->ms_ATOM_DIR == hashCode)
        {
            if (getDir().size() != 0)
            {
                 if (m_Log) m_Log->logError(nsCalLogErrorMessage::Instance()->ms_iDuplicatedParameter,
                        nsCalKeyword::Instance()->ms_sATTENDEE, paramName, 100);
            }
            nsCalUtility::stripDoubleQuotes(paramVal);  // double quote property
            setDir(paramVal);
        }
        else if (ICalProperty::IsXToken(paramName))
        {
            if (m_Log) m_Log->logError(nsCalLogErrorMessage::Instance()->ms_iXTokenParamIgnored,
                        nsCalKeyword::Instance()->ms_sATTENDEE, paramName, 100);
        }
        else 
        {
            if (m_Log) m_Log->logError(nsCalLogErrorMessage::Instance()->ms_iInvalidParameterName,
                        nsCalKeyword::Instance()->ms_sATTENDEE, paramName, 200);
        }
    }
}

//---------------------------------------------------------------------

UnicodeString & Attendee::toICALString(UnicodeString & out) 
{
    out = "";
    //UnicodeString sName = getName();
    //UnicodeString sStatus;
    //sStatus = statusToString(getStatus(), sStatus);
    //JulianPtrArray * vsDelegatedTo = getDelegatedTo();
    //JulianPtrArray * vsDelegatedFrom = getDelegatedFrom();
    
    if (getStatus() != STATUS_NEEDSACTION)
    {
#if 0
        //if (sStatus.compareIgnoreCase(nsCalKeyword::Instance()->ms_sNEEDSACTION) != 0) {
        /*
        if (vsDelegatedTo != NULL && vsDelegatedTo.contains(sName))
        return formatDoneDelegateFromOnly();
        else if (vsDelegatedFrom != null && vsDelegatedFrom.contains(sName))
        return formatDoneDelegateToOnly();
        else 
        */
#endif
        out = formatDoneAction();
    } 
    else 
    {
#if 0
        /*
        if (vsDelegatedTo != null && vsDelegatedTo.contains(sName))
	    return formatDelegateFromOnly();
        else if (vsDelegatedFrom != null && vsDelegatedFrom.contains(sName))
	    return formatDelegateToOnly();
        else
        */
#endif
	    out = formatNeedsAction();
    }
    return out;
}
//---------------------------------------------------------------------

UnicodeString & Attendee::toICALString(UnicodeString & sProp, 
                                       UnicodeString & out)
{
    // NOTE: to eliminate stupid warning
    if (sProp.size() > 0) {}
    return toICALString(out);
}

//---------------------------------------------------------------------

UnicodeString & Attendee::toString(UnicodeString & out)  
{ 
    out = toString(nsCalFormatString::Instance()->ms_AttendeeStrDefaultFmt, out);
    return out;
}

//---------------------------------------------------------------------

UnicodeString & Attendee::toString(UnicodeString & strFmt, 
                                   UnicodeString & out) 
{
    if (strFmt.size() == 0 && 
        nsCalFormatString::Instance()->ms_AttendeeStrDefaultFmt.size() > 0)
    {
        // if empty format string, use default
        return toString(out);
    }

    UnicodeString into;
    t_int32 i,j;    
    //if (FALSE) TRACE("strFmt = %s\r\n", strFmt.toCString(""));
    out = "";
    for ( i = 0; i < strFmt.size(); )
    {
	
        // NOTE: changed from % to ^ for attendee
        ///
	    ///  If there's a special formatting character,
	    ///  handle it.  Otherwise, just emit the supplied
	    ///  character.
	    ///

        j = strFmt.indexOf('^', i);
        if ( -1 != j)
        {
	        if (j > i)
            {
	            out += strFmt.extractBetween(i,j,into);
            }
            i = j + 1;
            if ( strFmt.size() > i)
            {
                out += toStringChar(strFmt[(TextOffset) i]); 
                i++;
            }
	    }
	    else
        {
          out += strFmt.extractBetween(i, strFmt.size(),into);
          break;
        }
    }
    return out;
}// end 

//---------------------------------------------------------------------

UnicodeString Attendee::toStringChar(t_int32 c)
{

    UnicodeString u;
    switch ( c )
    {
      case ms_cAttendeeName:
          return getName();
      case ms_cAttendeeRole:
          return roleToString((Attendee::ROLE) getRole(), u);
      case ms_cAttendeeType:
          return typeToString((Attendee::TYPE) getType(), u);
      case ms_cAttendeeStatus:
          return statusToString((Attendee::STATUS) getStatus(), u);
      case ms_cAttendeeExpect:
          return expectToString((Attendee::EXPECT) getExpect(), u);
      case ms_cAttendeeRSVP:
          return rsvpToString((Attendee::RSVP) getRSVP(), u);
      case ms_cAttendeeDir:
          return m_Dir;
      case ms_cAttendeeSentBy:
          return m_SentBy;
      case ms_cAttendeeCN:
          return m_CN;
      case ms_cAttendeeLanguage:
          return m_Language;
      case ms_cAttendeeDisplayName:
          // return CN is CN != "", else
          // return the AttendeeName after the ':'
          if (m_CN.size() > 0)
              return m_CN;
          else
          {
              t_int32 i = m_sName.indexOf(':');
              if (i >= 0)
              {
                  u = getName().extractBetween(i + 1, m_sName.size(), u);
                  return u;
              }
              else 
                  return "";
          }
      default:
	    return "";
    }
}// end of 

//---------------------------------------------------------------------

t_bool Attendee::isValid()
{
    /*
    UnicodeString mailto;
    if (m_sName.size() < 7)
        return FALSE;
    // change to URL, must have "MAILTO:" in front
    mailto = m_sName.extractBetween(0, 7, mailto);
    if (mailto.compareIgnoreCase(nsCalKeyword::Instance()->ms_sMAILTO_COLON) != 0)
        return FALSE;
    else
        return TRUE;
    */
    return URI::IsValidURI(m_sName);
}

//---------------------------------------------------------------------

void Attendee::selfCheckHelper(Attendee::PROPS prop, t_int32 param)
{
    if (prop == PROPS_ROLE && (param < 0 || param >= ROLE_LENGTH))
        setRole((Attendee::ROLE) ms_iDEFAULT_ROLE);
    else if (prop == PROPS_TYPE && (param < 0 || param >= TYPE_LENGTH))
        setType((Attendee::TYPE) ms_iDEFAULT_TYPE);
    else if (prop == PROPS_STATUS && (param < 0 || param >= STATUS_LENGTH))
        setStatus((Attendee::STATUS) ms_iDEFAULT_STATUS);
    else if (prop == PROPS_EXPECT && (param < 0 || param >= EXPECT_LENGTH))
        setExpect((Attendee::EXPECT) ms_iDEFAULT_EXPECT);
    else if (prop == PROPS_RSVP && (param < 0 || param >= RSVP_LENGTH))
        setRSVP((Attendee::RSVP) ms_iDEFAULT_RSVP);
}

//---------------------------------------------------------------------
  
void Attendee::selfCheck()
{
    selfCheckHelper(PROPS_ROLE, (t_int32) getRole());
    selfCheckHelper(PROPS_TYPE, (t_int32) getType());
    selfCheckHelper(PROPS_STATUS, (t_int32) getStatus());
    selfCheckHelper(PROPS_EXPECT, (t_int32) getExpect());
    selfCheckHelper(PROPS_RSVP, (t_int32) getRSVP());		
}

//---------------------------------------------------------------------

UnicodeString Attendee::format(UnicodeString strFmt) 
{
    UnicodeString s, info;
    t_int32 i,j;    
    s += nsCalKeyword::Instance()->ms_sATTENDEE;

    for ( i = 0; i < strFmt.size(); )
    {

    /*-
	***  If there's a special formatting character,
	***  handle it.  Otherwise, just emit the supplied
	***  character.
	**/
        j = strFmt.indexOf('%', i);
    	if ( -1 != j)
        {
    	    if (j > i)
            {
	          s += strFmt.extractBetween(i,j,info);
            }
            i = j + 1;
            if (strFmt.size() > i)
            { 
    		    s += formatChar(strFmt[(TextOffset) i]); 
        		i++;
	        }
	    }
	    else
	    {
	        s += strFmt.extractBetween(i, strFmt.size(),info);
            break;
        }
    }
    return s;
}

//---------------------------------------------------------------------

UnicodeString Attendee::formatChar(t_int32 c) 
{
    UnicodeString s, u;
    JulianPtrArray * xp;
    switch ( c )
    {  
    case ms_cAttendeeName: {
        // ALWAYS MUST BE LAST OF PARAMS
        s = ":"; //ms_sCOLON_SYMBOL;
        s += getName();
        s += nsCalKeyword::Instance()->ms_sLINEBREAK; 
	    return s;
    }
    case ms_cAttendeeRole: 
        s = roleToString(getRole(), s);
        return ICalProperty::parameterToCalString(
            nsCalKeyword::Instance()->ms_sROLE, s, u);
    case ms_cAttendeeStatus: 
        s = statusToString(getStatus(), s);
        return ICalProperty::parameterToCalString(
            nsCalKeyword::Instance()->ms_sPARTSTAT, s, u);
    case ms_cAttendeeRSVP:
        s = rsvpToString(getRSVP(), s);
        return ICalProperty::parameterToCalString(
            nsCalKeyword::Instance()->ms_sRSVP, s, u);
    case ms_cAttendeeType: 
        s = typeToString(getType(), s);
	    return ICalProperty::parameterToCalString(
            nsCalKeyword::Instance()->ms_sCUTYPE, s, u);
    case ms_cAttendeeExpect:
        s = expectToString(getExpect(), s);
        return ICalProperty::parameterToCalString(
            nsCalKeyword::Instance()->ms_sEXPECT, s, u);
    case ms_cAttendeeDelegatedTo:
        xp = getDelegatedTo();
	    return printMailToVector(
            nsCalKeyword::Instance()->ms_sDELEGATED_TO, xp, u);
    case ms_cAttendeeDelegatedFrom: 
        xp = getDelegatedFrom();
	    return printMailToVector(
            nsCalKeyword::Instance()->ms_sDELEGATED_FROM, xp, u);
    case ms_cAttendeeMember: 
        xp = getMember();
        return printMailToVector(
            nsCalKeyword::Instance()->ms_sMEMBER, xp, u);
    case ms_cAttendeeDir:
        s = m_Dir;
        s = nsCalUtility::addDoubleQuotes(s);
        return ICalProperty::parameterToCalString(
            nsCalKeyword::Instance()->ms_sDIR, s, u);
    case ms_cAttendeeSentBy:
        s = m_SentBy;
        s = nsCalUtility::addDoubleQuotes(s);
        return ICalProperty::parameterToCalString(
            nsCalKeyword::Instance()->ms_sSENTBY, s, u);
    case ms_cAttendeeCN:
        return ICalProperty::parameterToCalString(
            nsCalKeyword::Instance()->ms_sCN, m_CN, u);
    case ms_cAttendeeLanguage:
        return ICalProperty::parameterToCalString(
            nsCalKeyword::Instance()->ms_sLANGUAGE, m_Language, u);
    default:
        return "";
    }
}

//---------------------------------------------------------------------

UnicodeString &
Attendee::printMailToVector(UnicodeString & propName, JulianPtrArray * mailto,
                            UnicodeString & out)
{
    out = "";
    if (mailto != 0 && mailto->GetSize() > 0)
    {
        t_int32 i;
        out += ';';
        out += propName;
        out += '=';
        UnicodeString aMailto;
        for (i = 0; i < mailto->GetSize(); i++)
        {   
            aMailto = *((UnicodeString *) mailto->GetAt(i));
            out += '\"';
            out += aMailto;
            out += '\"';
            if (i < mailto->GetSize() - 1)
            {
                out += ',';
            }
        }
    }
    return out;
}

//---------------------------------------------------------------------

UnicodeString Attendee::formatDoneAction() 
{    
    return format(nsCalFormatString::Instance()->ms_sAttendeeDoneActionMessage); 
}

//---------------------------------------------------------------------
#if 0
UnicodeString Attendee::formatDoneDelegateToOnly() 
{ 
    return format(nsCalFormatString::Instance()->ms_sAttendeeDoneDelegateToOnly);
}

//---------------------------------------------------------------------

UnicodeString Attendee::formatDoneDelegateFromOnly() 
{ 
    return format(nsCalFormatString::Instance()->ms_sAttendeeDoneDelegateFromOnly); 
}
#endif
//---------------------------------------------------------------------
// BELOW METHODS ALL ARE NEEDS ACTION
// no need for %S, assumed to be NEEDS-ACTION
UnicodeString Attendee::formatNeedsAction() 
{ 
    return format(nsCalFormatString::Instance()->ms_sAttendeeNeedsActionMessage); 
}

//---------------------------------------------------------------------
#if 0
UnicodeString Attendee::formatDelegateToOnly() 
{ 
    return format(nsCalFormatString::Instance()->ms_sAttendeeNeedsActionDelegateToOnly); 
}

//---------------------------------------------------------------------

UnicodeString Attendee::formatDelegateFromOnly() 
{ 
    return format(nsCalFormatString::Instance()->ms_sAttendeeNeedsActionDelegateFromOnly); 
}  
#endif
//---------------------------------------------------------------------

void Attendee::setParameters(JulianPtrArray * parameters)
{
    t_int32 i;
    ICalParameter * param;
    UnicodeString pName, pVal;
    if (parameters != 0)
    {
        for (i = 0; i < parameters->GetSize(); i++)
        {
            param = (ICalParameter *) parameters->GetAt(i);
            setParam(param->getParameterName(pName), 
                param->getParameterValue(pVal));
        }   
    }
}

//---------------------------------------------------------------------

void Attendee::setName(UnicodeString sName) 
{ 
    m_sName = sName; 
}
  
//---------------------------------------------------------------------

void Attendee::addDelegatedTo(UnicodeString sDelegatedTo) 
{
    if (m_vsDelegatedTo == 0)
        m_vsDelegatedTo = new JulianPtrArray(); 
    PR_ASSERT(m_vsDelegatedTo != 0);
    if (m_vsDelegatedTo != 0)
    {
        m_vsDelegatedTo->Add(new UnicodeString(sDelegatedTo));
    }
}

//---------------------------------------------------------------------

void Attendee::addDelegatedFrom(UnicodeString sDelegatedFrom) 
{
    if (m_vsDelegatedFrom == 0)
        m_vsDelegatedFrom = new JulianPtrArray(); 
    PR_ASSERT(m_vsDelegatedFrom != 0);
    if (m_vsDelegatedFrom != 0)
    {
        m_vsDelegatedFrom->Add(new UnicodeString(sDelegatedFrom));
    }
}

//---------------------------------------------------------------------

void Attendee::addMember(UnicodeString sMember) 
{
    if (m_vsMember == 0)
        m_vsMember = new JulianPtrArray(); 
    PR_ASSERT(m_vsMember != 0);
    if (m_vsMember != 0)
    {
        m_vsMember->Add(new UnicodeString(sMember)); 
    }
}
//---------------------------------------------------------------------

Attendee * Attendee::getAttendee(JulianPtrArray * vAttendees, UnicodeString sAttendee)
{
    Attendee * att;
    t_int32 i;
    if (vAttendees != 0 && vAttendees->GetSize() > 0)
    {
        for (i = 0; i < vAttendees->GetSize(); i++)
        {
            att = (Attendee *) vAttendees->GetAt(i);
            if ((att->getName()).compareIgnoreCase(sAttendee) == 0)
            {  
    	        return att;
            }
        }				
    }
    return 0;
}

//---------------------------------------------------------------------

Attendee::ROLE
Attendee::stringToRole(UnicodeString & sRole)
{
    sRole.toUpper();
    t_int32 hashCode = sRole.hashCode();

#if 0
    /* Old keywords */
    if (nsCalKeyword::Instance()->ms_ATOM_ATTENDEE == hashCode) return ROLE_ATTENDEE;
    else if (nsCalKeyword::Instance()->ms_ATOM_ORGANIZER == hashCode) return ROLE_ORGANIZER;
    else if (nsCalKeyword::Instance()->ms_ATOM_OWNER == hashCode) return ROLE_OWNER;
    else if (nsCalKeyword::Instance()->ms_ATOM_DELEGATE == hashCode) return ROLE_DELEGATE;
#endif

    if (nsCalKeyword::Instance()->ms_ATOM_CHAIR == hashCode) return ROLE_CHAIR;
    else if (nsCalKeyword::Instance()->ms_ATOM_REQ_PARTICIPANT == hashCode) return ROLE_REQ_PARTICIPANT;
    else if (nsCalKeyword::Instance()->ms_ATOM_OPT_PARTICIPANT == hashCode) return ROLE_OPT_PARTICIPANT;
    else if (nsCalKeyword::Instance()->ms_ATOM_NON_PARTICIPANT == hashCode) return ROLE_NON_PARTICIPANT;
    else if (ICalProperty::IsXToken(sRole)) return ROLE_XPARAMVAL;
    else return ROLE_INVALID;
}

//---------------------------------------------------------------------

Attendee::TYPE
Attendee::stringToType(UnicodeString & sType)
{
    sType.toUpper();
    t_int32 hashCode = sType.hashCode();

    if (nsCalKeyword::Instance()->ms_ATOM_INDIVIDUAL == hashCode) return TYPE_INDIVIDUAL;
    else if (nsCalKeyword::Instance()->ms_ATOM_GROUP == hashCode) return TYPE_GROUP;
    else if (nsCalKeyword::Instance()->ms_ATOM_RESOURCE == hashCode) return TYPE_RESOURCE;
    else if (nsCalKeyword::Instance()->ms_ATOM_ROOM == hashCode) return TYPE_ROOM;
    else if (nsCalKeyword::Instance()->ms_ATOM_UNKNOWN == hashCode) return TYPE_UNKNOWN;
    else if (ICalProperty::IsXToken(sType)) return TYPE_XPARAMVAL;
    else return TYPE_INVALID;
}

//---------------------------------------------------------------------

Attendee::STATUS
Attendee::stringToStatus(UnicodeString & sStatus)
{
    sStatus.toUpper();
    t_int32 hashCode = sStatus.hashCode();

    if (nsCalKeyword::Instance()->ms_ATOM_NEEDSACTION == hashCode) return STATUS_NEEDSACTION;
    else if (nsCalKeyword::Instance()->ms_ATOM_VCALNEEDSACTION == hashCode) return STATUS_NEEDSACTION;
    
    else if (nsCalKeyword::Instance()->ms_ATOM_ACCEPTED == hashCode) return STATUS_ACCEPTED;
    else if (nsCalKeyword::Instance()->ms_ATOM_DECLINED == hashCode) return STATUS_DECLINED;
    else if (nsCalKeyword::Instance()->ms_ATOM_TENTATIVE == hashCode) return STATUS_TENTATIVE;
    else if (nsCalKeyword::Instance()->ms_ATOM_COMPLETED == hashCode) return STATUS_COMPLETED;
    else if (nsCalKeyword::Instance()->ms_ATOM_INPROCESS == hashCode) return STATUS_INPROCESS;
    else if (nsCalKeyword::Instance()->ms_ATOM_DELEGATED == hashCode) return STATUS_DELEGATED;
    else if (ICalProperty::IsXToken(sStatus)) return STATUS_XPARAMVAL;
    else return STATUS_INVALID;
}

//---------------------------------------------------------------------

Attendee::RSVP
Attendee::stringToRSVP(UnicodeString & sRSVP)
{
    sRSVP.toUpper();
    t_int32 hashCode = sRSVP.hashCode();

    if (nsCalKeyword::Instance()->ms_ATOM_FALSE == hashCode) return RSVP_FALSE;
    else if (nsCalKeyword::Instance()->ms_ATOM_TRUE == hashCode) return RSVP_TRUE;
    else return RSVP_INVALID;
}

//---------------------------------------------------------------------

Attendee::EXPECT
Attendee::stringToExpect(UnicodeString & sExpect)
{
    sExpect.toUpper();
    t_int32 hashCode = sExpect.hashCode();
    
    if (nsCalKeyword::Instance()->ms_ATOM_FYI == hashCode) return EXPECT_FYI;
    else if (nsCalKeyword::Instance()->ms_ATOM_REQUIRE == hashCode) return EXPECT_REQUIRE;
    else if (nsCalKeyword::Instance()->ms_ATOM_REQUEST == hashCode) return EXPECT_REQUEST;
    else return EXPECT_INVALID;
}

//---------------------------------------------------------------------

UnicodeString & 
Attendee::roleToString(Attendee::ROLE role, UnicodeString & out)
{
    switch(role)
    {

#if 0
        /* Old Keywords */
    case ROLE_ATTENDEE: out = nsCalKeyword::Instance()->ms_sATTENDEE; break;
    case ROLE_ORGANIZER: out = nsCalKeyword::Instance()->ms_sORGANIZER; break;
    case ROLE_OWNER: out = nsCalKeyword::Instance()->ms_sOWNER; break;
    case ROLE_DELEGATE: out = nsCalKeyword::Instance()->ms_sDELEGATE; break;
#endif

    case ROLE_CHAIR: out = nsCalKeyword::Instance()->ms_sCHAIR; break;
    case ROLE_REQ_PARTICIPANT: out = nsCalKeyword::Instance()->ms_sREQ_PARTICIPANT; break;
    case ROLE_OPT_PARTICIPANT: out = nsCalKeyword::Instance()->ms_sOPT_PARTICIPANT; break;
    case ROLE_NON_PARTICIPANT: out = nsCalKeyword::Instance()->ms_sNON_PARTICIPANT; break;
    case ROLE_XPARAMVAL: out = nsCalKeyword::Instance()->ms_sXPARAMVAL; break; 
    default:
        // default return req participant
        out = nsCalKeyword::Instance()->ms_sREQ_PARTICIPANT;
        break;
    }
    return out;
}

//---------------------------------------------------------------------

UnicodeString & 
Attendee::typeToString(Attendee::TYPE type, UnicodeString & out)
{
    switch(type)
    {
    case TYPE_INDIVIDUAL: out = nsCalKeyword::Instance()->ms_sINDIVIDUAL; break;
    case TYPE_GROUP: out = nsCalKeyword::Instance()->ms_sGROUP; break;
    case TYPE_RESOURCE: out = nsCalKeyword::Instance()->ms_sRESOURCE; break;
    case TYPE_ROOM: out = nsCalKeyword::Instance()->ms_sROOM; break;
    case TYPE_UNKNOWN: out = nsCalKeyword::Instance()->ms_sUNKNOWN; break;
    case TYPE_XPARAMVAL: out = nsCalKeyword::Instance()->ms_sXPARAMVAL; break; 
    default:
        // default return individual
        out = nsCalKeyword::Instance()->ms_sINDIVIDUAL;
        break;
    }
    return out;
}

//---------------------------------------------------------------------

UnicodeString & 
Attendee::statusToString(Attendee::STATUS status, UnicodeString & out)
{
    switch(status)
    {
        case STATUS_NEEDSACTION: out = nsCalKeyword::Instance()->ms_sNEEDSACTION; break;
        case STATUS_ACCEPTED: out = nsCalKeyword::Instance()->ms_sACCEPTED; break; 
        case STATUS_DECLINED: out = nsCalKeyword::Instance()->ms_sDECLINED; break; 
        case STATUS_TENTATIVE: out = nsCalKeyword::Instance()->ms_sTENTATIVE; break; 
        case STATUS_COMPLETED: out = nsCalKeyword::Instance()->ms_sCOMPLETED; break; 
        case STATUS_DELEGATED: out = nsCalKeyword::Instance()->ms_sDELEGATED; break; 
        case STATUS_INPROCESS: out = nsCalKeyword::Instance()->ms_sINPROCESS; break; 
        case STATUS_XPARAMVAL: out = nsCalKeyword::Instance()->ms_sXPARAMVAL; break; 
        default:
            // default return needs-action
            out = nsCalKeyword::Instance()->ms_sNEEDSACTION;
            break;
    }
    return out;
}

//---------------------------------------------------------------------

UnicodeString & 
Attendee::rsvpToString(Attendee::RSVP rsvp, UnicodeString & out)
{
    switch(rsvp)
    {
        case RSVP_FALSE: out = nsCalKeyword::Instance()->ms_sFALSE; break; 
        case RSVP_TRUE: out = nsCalKeyword::Instance()->ms_sTRUE; break; 
        default:
            // default return false
            out = nsCalKeyword::Instance()->ms_sFALSE;
            break;
    }
    return out;
}

//---------------------------------------------------------------------

UnicodeString & 
Attendee::expectToString(Attendee::EXPECT expect, UnicodeString & out)
{
    switch(expect)
    {
        case EXPECT_FYI: out = nsCalKeyword::Instance()->ms_sFYI; break; 
        case EXPECT_REQUIRE: out = nsCalKeyword::Instance()->ms_sREQUIRE; break; 
        case EXPECT_REQUEST: out = nsCalKeyword::Instance()->ms_sREQUEST; break;
        default:
            // default return fyi
            out = nsCalKeyword::Instance()->ms_sFYI;
            break;
    }
    return out;
}

//---------------------------------------------------------------------

