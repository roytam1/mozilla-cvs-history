/* 
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is OEone Corporation.
 *
 * The Initial Developer of the Original Code is
 * OEone Corporation.
 * Portions created by OEone Corporation are Copyright (C) 2001
 * OEone Corporation. All Rights Reserved.
 *
 * Contributor(s): Mostafa Hosseini (mostafah@oeone.com)
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * 
*/

#include "oeIICal.h"
extern "C" {
    #include "ical.h"
}

#define OE_ICALEVENT_CID \
{ 0x31bda500, 0xee5e, 0x4a4a, { 0x9a, 0xb4, 0x7b, 0x4b, 0x3e, 0x87, 0x40, 0x05 } }

#define OE_ICALEVENT_CONTRACTID "@mozilla.org/icalevent;1"


/* oeIcalEvent Header file */
class oeICalEventImpl : public oeIICalEvent
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_OEIICALEVENT
  icalcomponent *vcalendar;
  oeICalEventImpl();
  virtual ~oeICalEventImpl();
  /* additional members */
};


