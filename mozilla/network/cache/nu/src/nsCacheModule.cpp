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

#include "nsCacheModule.h"
#include "nsCacheTrace.h"
#include "nsCacheIterator.h"

/* 
 * nsCacheModule
 *
 * Gagan Saksena 02/02/98
 * 
 */


#define DEFAULT_SIZE 10*0x100000L

nsCacheModule::nsCacheModule(const PRUint32 i_size=DEFAULT_SIZE):
    m_Size(i_size),
    m_SizeInUse(0),
    m_pEnumeration(0),
    m_pNext(0),
    m_Entries(0)
{
    m_pIterator = new nsCacheIterator(this);
}

nsCacheModule::~nsCacheModule()
{
    if (m_pNext)
    {
        delete m_pNext;
        m_pNext = 0;
    }
    if (m_pIterator)
    {
        delete m_pIterator;
        m_pIterator = 0;
    }
    if (m_pEnumeration)
    {
        delete m_pEnumeration;
        m_pEnumeration = 0;
    }
}

void nsCacheModule::GarbageCollect(void) 
{
}

PRBool nsCacheModule::ReduceSizeTo(const PRUint32 i_NewSize)
{
	MonitorLocker ml(this);
    //TODO
    return PR_TRUE;
}

PRBool nsCacheModule::RemoveAll(void)
{
	MonitorLocker ml(this);
    PRBool status = PR_TRUE;
    while (m_Entries > 0)
    {
        status &= Remove(--m_Entries);
    }
    return status;
}

#if 0
// Caller must free this 
const char* nsCacheModule::Trace() const
{
    char linebuffer[128];
    char* total;

    PR_sprintf(linebuffer, "nsCacheModule: Objects = %d\n", Entries());

    total = new char[PR_strlen(linebuffer) + 1];
    strcpy(total, linebuffer);

    return total;
}
#endif
