/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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

#ifndef nsCalAttendeeProperty_h___
#define nsCalAttendeeProperty_h___

#include "nsCalProperty.h"
#include "attendee.h"

class NS_CAL_CORE_ICAL nsCalAttendeeProperty : public nsCalProperty
{
public:
  nsCalAttendeeProperty();
  ~nsCalAttendeeProperty();

public:

  NS_DECL_ISUPPORTS
  
  NS_IMETHOD Init();

  NS_IMETHOD SetParameter(nsString & aKey, nsString & aValue);
  
  NS_IMETHOD_(JulianPtrArray *) GetParameters();
  NS_IMETHOD SetParameters(JulianPtrArray * parameters);
  NS_IMETHOD_(void *) GetValue() const;
  NS_IMETHOD SetValue(void * value);
  NS_IMETHOD_(nsICalProperty *) Clone();
  NS_IMETHOD_(PRBool) IsValid();
  NS_IMETHOD_(nsString &) ToString(nsString & out);
  NS_IMETHOD_(nsString &) ToICALString(nsString & out);
  NS_IMETHOD_(nsString &) ToICALString(nsString & sProp, nsString & out);
};

#endif
