/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
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
 * NT-specific semaphore handling code.
 *
 */


#include "primpl.h"


void 
_PR_MD_NEW_SEM(_MDSemaphore *md, PRUintn value)
{
    md->sem = CreateSemaphore(NULL, value, 0x7fffffff, NULL);
}

void 
_PR_MD_DESTROY_SEM(_MDSemaphore *md)
{
    CloseHandle(md->sem);
}

PRStatus 
_PR_MD_TIMED_WAIT_SEM(_MDSemaphore *md, PRIntervalTime ticks)
{
    int rv;

    rv = WaitForSingleObject(md->sem, PR_IntervalToMilliseconds(ticks));

    if (rv == WAIT_OBJECT_0)
        return PR_SUCCESS;
    else
        return PR_FAILURE;
}

PRStatus 
_PR_MD_WAIT_SEM(_MDSemaphore *md)
{
    return _PR_MD_TIMED_WAIT_SEM(md, PR_INTERVAL_NO_TIMEOUT);
}

void 
_PR_MD_POST_SEM(_MDSemaphore *md)
{
    int old_count;

    ReleaseSemaphore(md->sem, 1, &old_count);
}
