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

#include "prtypes.h"
#include "plstr.h"
#include "prlog.h"

#include "nsMemModule.h"
#include "nsMemCacheObject.h"
#include "nsCacheManager.h"
#include "nsMemStream.h"

/* 
 * nsMemModule
 *
 * Gagan Saksena 02/02/98
 * 
 */

//NS_DEFINE_IID(kMemModuleIID, NS_MEMMODULE_IID);

nsMemModule::nsMemModule(const PRUint32 size): 
    m_pFirstObject(0),
    nsCacheModule(size)
{
    SetSize(size);
}

nsMemModule::~nsMemModule()
{
    if (m_pFirstObject) {
        delete m_pFirstObject;
        m_pFirstObject = 0;
    }
}

PRBool nsMemModule::AddObject(nsCacheObject* io_pObject)
{

#if 0
    if (io_pObject)
    {
        m_ht.Put(io_pObject->Address(), io_pObject);
    }
    return PR_FALSE;
#endif

    if (io_pObject)
    {
        MonitorLocker ml(this);
        PR_ASSERT(io_pObject->Stream()); // A valid stream does exist for this 
        if (m_pFirstObject) 
        {
            LastObject()->Next(new nsMemCacheObject(io_pObject)); 
        }
        else
        {
            m_pFirstObject = new nsMemCacheObject(io_pObject);
        }
        m_Entries++;

        io_pObject->Module(nsCacheManager::MEM);

        return PR_TRUE;
    }
    return PR_FALSE;
}

PRBool nsMemModule::Contains(const char* i_url) const
{
    MonitorLocker ml((nsMonitorable*)this);

    if (m_pFirstObject && i_url && *i_url)
    {
        nsMemCacheObject* pObj = m_pFirstObject;
        do
        {
            if (0 == PL_strcasecmp(pObj->ThisObject()->Address(), i_url))
                return PR_TRUE;
            pObj = pObj->Next();
        }
        while (pObj);
    }
    return PR_FALSE;
}

PRBool nsMemModule::Contains(nsCacheObject* i_pObject) const
{
    MonitorLocker ml((nsMonitorable*)this);

    if (i_pObject && *i_pObject->Address())
    {
        return this->Contains(i_pObject->Address());
    }
    return 0;
}

void nsMemModule::GarbageCollect(void)
{
    MonitorLocker ml(this);

    if (m_Entries > 0)
    {
        nsEnumeration* pEnum = Enumeration();
        PRUint32 index = 0;
        while (pEnum->HasMoreElements())
        {
            nsCacheObject* pObj = (nsCacheObject*) pEnum->NextElement();
            if (pObj && pObj->IsExpired())
            {
                PRBool status = Remove(index);
                PR_ASSERT(status == PR_TRUE);
            }
            ++index;
        }
    }
}

nsCacheObject* nsMemModule::GetObject(const PRUint32 i_index) const
{
    MonitorLocker ml((nsMonitorable*)this);
    nsMemCacheObject* pNth = 0;
    if (m_pFirstObject)
    {
        PRUint32 index = 0;
        pNth = m_pFirstObject;
        while (pNth->Next() && (index++ != i_index ))
        {
            pNth = pNth->Next();
        }
    }
    return pNth->ThisObject();
}

nsCacheObject* nsMemModule::GetObject(const char* i_url) const
{
    MonitorLocker ml((nsMonitorable*)this);
    if (m_pFirstObject && i_url && *i_url)
    {
        nsMemCacheObject* pObj = m_pFirstObject;
        do
        {
            if (0 == PL_strcasecmp(pObj->ThisObject()->Address(), i_url))
                return pObj->ThisObject();
            pObj = pObj->Next();
        }
        while (pObj);
    }
    return 0;
}

nsStream* nsMemModule::GetStreamFor(const nsCacheObject* i_pObject)
{
    MonitorLocker ml(this);
    if (i_pObject)
    {
        if (Contains((nsCacheObject*)i_pObject))
        {
            nsStream* pStream = i_pObject->Stream();
            if (pStream)
                return pStream;
        }
        // Set up a new stream for this object
        return new nsMemStream();
    }
    return 0;
}

nsMemCacheObject* nsMemModule::LastObject(void) const
{
    MonitorLocker ml((nsMonitorable*)this);
    
    nsMemCacheObject* pLast = 0;
    if (m_pFirstObject)
    {
        pLast = m_pFirstObject;
        while (pLast->Next())
            pLast = pLast->Next();
    }
    return pLast;
}

PRBool nsMemModule::ReduceSizeTo(const PRUint32 i_NewSize)
{
    //TODO
    return PR_TRUE;
}

PRBool nsMemModule::Remove(const char* i_url)
{
    //TODO
    return PR_FALSE;
}

PRBool nsMemModule::Remove(const PRUint32 i_index)
{
    //TODO
    return PR_FALSE;
}

/*
NS_IMETHOD nsMemModule::QueryInterface(const nsIID& aIID, void** aInstancePtr)
{

}
NS_IMETHOD_(nsrefcnt) nsMemModule::AddRef(void)
{

}

NS_IMETHOD_(nsrefcnt) nsMemModule::Release(void)
{

}
*/

/*
PRUint32 nsMemModule::nsMemKey::HashValue()
{
    return 0;
}
PRBool nsMemModule::nsMemKey::Equals(nsHashKey *aKey)
{
    return PR_FALSE;
}

nsHashKey* nsMemModule::nsMemKey::Clone()
{
    return new nsMemModule::nsMemKey();
}

nsMemModule::nsMemKey::neMemKey()
{
}

  */
