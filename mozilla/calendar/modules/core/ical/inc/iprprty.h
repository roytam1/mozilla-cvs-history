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
 * iprprty.h
 * John Sun
 * 2/12/98 3:26:51 PM
 */

#include <unistring.h>
#include "sdprprty.h"

#ifndef __INTEGERPROPERTY_H_
#define __INTEGERPROPERTY_H_

/**
 * IntegerProperty is a subclass of StandardProperty.  It implements the ICalProperty
 * interface for ICAL properties with TYPE=INTEGER.
 */
class IntegerProperty: public StandardProperty
{
private:
    /** the Integer value of the property */
    t_int32 m_Integer;

    /**
     * Default constructor
     */
    IntegerProperty();
    
    /**
     * Copy constructor
     * @param           that    property to copy
     */
    IntegerProperty(const IntegerProperty & that);

public:
    
    /**
     * Constructor that sets value of property to contents of value
     * and makes a copy of the contents of parameters ptr.
     * @param           value       value of property
     * @param           parameters  parameters of property
     */
    IntegerProperty(t_int32 value, JulianPtrArray * parameters);
    
    /**
     * Destructor
     */
    ~IntegerProperty();

    /**
     * Return the value of the property 
     * @return      void * ptr to value of the property
     */
    virtual void * getValue() const;
    
    /**
     * Set the value of the property
     * @param           value   the value to set property to
     */
    virtual void setValue(void * value);
    
    /**
     * Returns a clone of this property.  Clients are responsible for deleting
     * the returned object.
     *
     * @return          a clone of this property. 
     */
    virtual ICalProperty * clone(JLog * initLog);
    
    /**
     * Return TRUE if property is valid, FALSE otherwise
     *
     * @return          TRUE if is valid, FALSE otherwise
     */
    virtual t_bool isValid();
    
    /**
     * Return a string in human-readable form of the property.
     * @param           out     contains output human-readable string
     *
     * @return          the property in its human-readable string format
     */
    virtual UnicodeString & toString(UnicodeString & out);
    
    /**
     * Return a string in human-readable form of the property.
     * @param           dateFmt     format string options
     * @param           out         contains output human-readable string
     *
     * @return          the property in its human-readable string format
     */
    virtual UnicodeString & toString(UnicodeString & dateFmt, UnicodeString & out);
    
    /**
     * Return the property's string in the ICAL format.
     * @param           out     contains output ICAL string
     *
     * @return          the property in its ICAL string format
     */
    virtual UnicodeString & toExportString(UnicodeString & out);
};
#endif /* __INTEGERPROPERTY_H_ */


