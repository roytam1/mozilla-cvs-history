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

#ifndef nsDuration_h___
#define nsDuration_h___

#include "nsIDuration.h"
#include "jdefines.h"
#include "unistring.h"
#include "duration.h"
#include "datetime.h"

class nsDuration : public nsIDuration
{
public:
  nsDuration();

  NS_DECL_ISUPPORTS

  NS_IMETHOD            Init() ;

  NS_IMETHOD_(PRUint32) GetYear();
  NS_IMETHOD_(PRUint32) GetMonth();
  NS_IMETHOD_(PRUint32) GetDay();
  NS_IMETHOD_(PRUint32) GetHour();
  NS_IMETHOD_(PRUint32) GetMinute();
  NS_IMETHOD_(PRUint32) GetSecond();

  NS_IMETHOD SetYear(PRUint32 aYear);
  NS_IMETHOD SetMonth(PRUint32 aMonth);
  NS_IMETHOD SetDay(PRUint32 aDay);
  NS_IMETHOD SetHour(PRUint32 aHour);
  NS_IMETHOD SetMinute(PRUint32 aMinute);
  NS_IMETHOD SetSecond(PRUint32 aSecond);

  NS_IMETHOD_(nsCalDuration&) GetDuration();

protected:
  ~nsDuration();

private:
  nsCalDuration * mDuration;

};

#endif /* nsDuration_h___ */
