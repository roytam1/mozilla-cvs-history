/* ***** BEGIN LICENSE BLOCK *****
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
 * The Original Code is OEone Calendar Code, released October 31st, 2001.
 *
 * The Initial Developer of the Original Code is
 * OEone Corporation.
 * Portions created by the Initial Developer are Copyright (C) 2001
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s): Mostafa Hosseini <mostafah@oeone.com>
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
 * ***** END LICENSE BLOCK ***** */

#include "oeIICal.h"
#include <vector>
#include "nsITimer.h"
#include "oeICalEventImpl.h"

#define OE_ICAL_CID \
{ 0x0a8c5de7, 0x0d19, 0x4b95, { 0x82, 0xf4, 0xe0, 0xaf, 0x92, 0x45, 0x32, 0x27 } }

#define OE_ICAL_CONTRACTID "@mozilla.org/ical;1"

class EventList {
public:
    oeIICalEvent* event;
    EventList* next;
    EventList() {
        event = nsnull;
        next = nsnull;
    }
    ~EventList() {
        if( event )
            event->Release();
        if( next )
            delete next;
    }
    void Add( oeIICalEvent* e) {
        if( !event ) {
            event = e;
        } else {
            if( !next ) {
                next = new EventList();
            }
            next->Add( e );
        }
    }
    oeIICalEvent* GetEventById( const char *id ) {
        if( !event )
            return nsnull;
        if( ((oeICalEventImpl *)event)->matchId( id ) )
            return event;
        if( next )
            return next->GetEventById( id );
        return nsnull;
    }
    void Remove( const char *id ) {
        if( !event )
            return;
        if( ((oeICalEventImpl *)event)->matchId( id ) ) {
            event->Release();
            if( next ) {
                event = next->event;
                EventList *tmp = next;
                next = next->next;
                tmp->next = nsnull;
                tmp->event = nsnull;
                delete tmp;
            } else {
                event = nsnull;
            }
        } else {
            if( next )
                next->Remove( id );
        }
    }
/*    int Count() {
        int result=0;
        if( !event )
            return 0;
        result++;
        if( next )
            result += next->Count();
        return result;
    }*/
};

class oeICalImpl : public oeIICal
{
 public:
        oeICalImpl();
        virtual ~oeICalImpl();
        
        /**
        * This macro expands into a declaration of the nsISupports interface.
        * Every XPCOM component needs to implement nsISupports, as it acts
        * as the gateway to other interfaces this component implements.  You
        * could manually declare QueryInterface, AddRef, and Release instead
        * of using this macro, but why?
        */
        // nsISupports interface
        NS_DECL_ISUPPORTS

        /**
        * This macro is defined in the nsISample.h file, and is generated
        * automatically by the xpidl compiler.  It expands to
        * declarations of all of the methods required to implement the
        * interface.  xpidl will generate a NS_DECL_[INTERFACENAME] macro
        * for each interface that it processes.
        *
        * The methods of nsISample are discussed individually below, but
        * commented out (because this macro already defines them.)
        */
        NS_DECL_OEIICAL
        void SetupAlarmManager();
private:
    std::vector<oeIICalObserver*> m_observerlist;
    bool m_batchMode;
    EventList m_eventlist;
    nsITimer *m_alarmtimer;
    char serveraddr[200];
};
