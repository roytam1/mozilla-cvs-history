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
 * byhgntr.h
 * John Sun
 * 2/3/98 3:13:16 PM
 */

#ifndef __BYHOURGENERATOR_H_
#define __BYHOURGENERATOR_H_

#include "dategntr.h"

/**
 * The ByHourGenerator class generates dates for the BYHOUR tag.
 *
 * @see              DateGenerator
 */
class ByHourGenerator : public DateGenerator
{
public:
    
    /**
     * Default Constructor
     */
    ByHourGenerator();
    
    /**
     * Destructor
     */
    ~ByHourGenerator() {}

    /*---------------------------------------------------------------------*/
    /* --- DateGenerator interface --- */

    /**
     * Returns the interval for which the generator represents.
     * @return          the interval (HOURLY)
     */
    virtual t_int32 getInterval() const; 

    /**
     * Fills in dateVector with DateTime objects that correspond to the
     * parameters and start value and the until value.
     * DateTime generated should be after start's value and before until's
     * value.  If until is invalid, ignore it.
     *
     * @param           start       the date from which to begin generating recurrences
     * @param           dateVector  storage for the generated dates
     * @param           until       the end date for the recurrence
     *
     * @return          TRUE if until date boundary was reached, FALSE otherwise
     */
    virtual t_bool generate(DateTime * start, JulianPtrArray & dateVector, DateTime * until);
    
    /**
     * Sets the hour (i.e. -23 - +23) parameters.
     * @param           params[]    the list of hours to set to
     * @param           length      the length of params[]
     */
    virtual void setParamsArray(t_int32 params[], t_int32 length) 
    { 
        m_aiParams = params; 
        m_iParamsLen = length;
    }
    
    /**
     * Determines if a generator has been set active.
     * Usually tests if parameters have been set or not.
     *
     * @return          TRUE if parameters set, FALSE otherwise.
     */
    virtual t_bool active() 
    { 
        return (m_aiParams != 0); 
    }

    /* --- End of DateGenerator interface --- */
    /*---------------------------------------------------------------------*/

private:
    
    /** hour parameters */
    t_int32 * m_aiParams;
};

#endif /* __BYHOURGENERATOR_H_ */

