/*
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
 * License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is the Mozilla OS/2 libraries.
 *
 * The Initial Developer of the Original Code is John Fairhurst,
 * <john_fairhurst@iname.com>.  Portions created by John Fairhurst are
 * Copyright (C) 1999 John Fairhurst. All Rights Reserved.
 *
 * Contributor(s): 
 *
 */

#ifndef _nsdatetimeformatos2_h_
#define _nsdatetimeformatos2_h_

#include "nsIDateTimeFormat.h"

class nsILocaleOS2;

class nsDateTimeFormatOS2 : public nsIDateTimeFormat
{
 public:
   nsDateTimeFormatOS2();
   virtual ~nsDateTimeFormatOS2();

   // nsISupports
   NS_DECL_ISUPPORTS

   // nsIDateTimeFormat

   // performs a locale sensitive date formatting operation on a time_t
   NS_IMETHOD FormatTime( nsILocale                  *aLocale, 
                          const nsDateFormatSelector  aDateFormatSelector, 
                          const nsTimeFormatSelector  aTimeFormatSelector, 
                          const time_t                aTime,
                          nsString                   &aStringOut);

   // performs a locale sensitive date formatting operation on the struct tm parameter
   NS_IMETHOD FormatTMTime( nsILocale                  *aLocale, 
                            const nsDateFormatSelector  aDateFormatSelector, 
                            const nsTimeFormatSelector  aTimeFormatSelector, 
                            const struct tm            *aTime, 
                            nsString                   &aStringOut);

  // performs a locale sensitive date formatting operation on the PRTime parameter
  NS_IMETHOD FormatPRTime(nsILocale* locale, 
                          const nsDateFormatSelector  dateFormatSelector, 
                          const nsTimeFormatSelector timeFormatSelector, 
                          const PRTime  prTime, 
                          nsString& stringOut);

  // performs a locale sensitive date formatting operation on the PRExplodedTime parameter
  NS_IMETHOD FormatPRExplodedTime(nsILocale* locale, 
                                  const nsDateFormatSelector  dateFormatSelector, 
                                  const nsTimeFormatSelector timeFormatSelector, 
                                  const PRExplodedTime*  explodedTime, 
                                  nsString& stringOut); 

};

#endif
