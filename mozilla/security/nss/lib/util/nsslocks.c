/*
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is the Netscape security libraries.
 * 
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are 
 * Copyright (C) 1994-2000 Netscape Communications Corporation.  All
 * Rights Reserved.
 * 
 * Contributor(s):
 * 
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU General Public License Version 2 or later (the
 * "GPL"), in which case the provisions of the GPL are applicable 
 * instead of those above.  If you wish to allow use of your 
 * version of this file only under the terms of the GPL and not to
 * allow others to use your version of this file under the MPL,
 * indicate your decision by deleting the provisions above and
 * replace them with the notice and other provisions required by
 * the GPL.  If you do not delete the provisions above, a recipient
 * may use your version of this file under either the MPL or the
 * GPL.
 */

/*
 * nsslocks.h - threadsafe functions to initialize lock pointers.
 *
 * NOTE - These are not public interfaces
 *
 * $Id$
 */

#ifdef STAN_BUILD
#include "nssbase.h"
#else
#include "seccomon.h"
#endif
#include "nsslocks.h"
#include "pratom.h"
#include "prthread.h"

/* Given the address of a (global) pointer to a PZLock, 
 * atomicly create the lock and initialize the (global) pointer, 
 * if it is not already created/initialized.
 */

#ifdef STAN_BUILD
PRStatus 
#else
SECStatus
#endif
__nss_InitLock(   PZLock    **ppLock, nssILockType ltype )
{
    static PRInt32  initializers;

    PR_ASSERT( ppLock != NULL);

    /* atomically initialize the lock */
    while (!*ppLock) {
        PRInt32 myAttempt = PR_AtomicIncrement(&initializers);
        if (myAttempt == 1) {
	    *ppLock = PZ_NewLock(ltype);
            (void) PR_AtomicDecrement(&initializers);
            break;
        }
        PR_Sleep(PR_INTERVAL_NO_WAIT);          /* PR_Yield() */
        (void) PR_AtomicDecrement(&initializers);
    }

    return (*ppLock != NULL) ? PR_SUCCESS : PR_FAILURE;
}

#ifdef STAN_BUILD
PRStatus 
#else
SECStatus
#endif
nss_InitLock(   PZLock    **ppLock, nssILockType ltype )
{
    return __nss_InitLock(ppLock, ltype);
}

/* Given the address of a (global) pointer to a PZMonitor, 
 * atomicly create the monitor and initialize the (global) pointer, 
 * if it is not already created/initialized.
 */

#ifdef STAN_BUILD
PRStatus 
#else
SECStatus
#endif
nss_InitMonitor(PZMonitor **ppMonitor, nssILockType ltype )
{
    static PRInt32  initializers;

    PR_ASSERT( ppMonitor != NULL);

    /* atomically initialize the lock */
    while (!*ppMonitor) {
        PRInt32 myAttempt = PR_AtomicIncrement(&initializers);
        if (myAttempt == 1) {
	    *ppMonitor = PZ_NewMonitor(ltype);
            (void) PR_AtomicDecrement(&initializers);
            break;
        }
        PR_Sleep(PR_INTERVAL_NO_WAIT);          /* PR_Yield() */
        (void) PR_AtomicDecrement(&initializers);
    }

    return (*ppMonitor != NULL) ? PR_SUCCESS : PR_FAILURE;
}
