/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
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
 * nsCacheModule. A class that defines the way a cache module 
 * should be written. Its the super class for any new modules. 
 * Two sample modules derived from this one are nsMemModule and nsDiskModule.
 *
 * Gagan Saksena 02/03/98
 * 
 */

#ifndef nsCacheModule_h__
#define nsCacheModule_h__

//#include <nsISupports.h>
#include "nsCacheObject.h"
#include "nsEnumeration.h"
#include "nsMonitorable.h"

/* Why in the world is forward decl. not working? */
//class nsCacheObject;

/*
// {5D51B24F-E6C2-11d1-AFE5-006097BFC036}
static const NS_CACHEMODULE_ID = 
{ 0x5d51b24f, 0xe6c2, 0x11d1, { 0xaf, 0xe5, 0x0, 0x60, 0x97, 0xbf, 0xc0, 0x36 } };
*/

class nsCacheIterator;
class nsCacheModule : public nsMonitorable /*: public nsISupports */
{

public:
    nsCacheModule(const PRUint32 i_size /*= DEFAULT_SIZE */);

    virtual
        ~nsCacheModule();

    virtual 
        PRBool          AddObject(nsCacheObject* i_pObject)=0;
    
    virtual
        PRBool          Contains(const char* i_url) const=0;
    virtual 
        PRBool          Contains(nsCacheObject* i_pObject) const=0;
    
    void                Enable(PRBool i_Enable);
    
    const PRUint32      Entries(void) const;

    nsEnumeration*      Enumeration(void) const;
    /* Enumerations wiht a function pointer - TODO */

    //TODO move to own interface for both Garbage Collection and Revalidation
    virtual
        void            GarbageCollect(void);

    virtual
        nsCacheObject*  GetObject(const char* i_url) const=0;

    virtual
        nsCacheObject*  GetObject(const PRUint32 i_index) const =0;

    virtual
        nsStream*       GetStreamFor(const nsCacheObject* i_pObject)=0;

    PRBool              IsEnabled(void) const;

    /* Cant do additions, deletions, validations, expirations */
    PRBool              IsReadOnly(void) const; 

    nsCacheModule*      NextModule(void) const;
    void                NextModule(nsCacheModule*);

    virtual
        PRBool          Remove(const char* i_url) = 0;

    virtual
        PRBool          Remove(const PRUint32 i_index) = 0;

    virtual
        PRBool          RemoveAll(void);

    virtual
        PRBool          Revalidate(void) = 0;

    const PRUint32      Size(void) const;

    virtual
        void            SetSize(const PRUint32 i_size);

    PRUint32            SizeInUse(void) const;

    const char*         Trace(void) const;

protected:

    virtual
        PRBool          ReduceSizeTo(const PRUint32 i_NewSize);

    PRUint32            m_Entries;
    PRUint32            m_Size;
    PRUint32            m_SizeInUse;
    PRBool              m_Enabled;
    nsEnumeration*      m_pEnumeration;
    nsCacheIterator*    m_pIterator;
    nsCacheModule*      m_pNext;

private:
    nsCacheModule(const nsCacheModule& cm);
    nsCacheModule& operator=(const nsCacheModule& cm);
};

inline void nsCacheModule::Enable(PRBool i_Enable)
{
    m_Enabled = i_Enable;
}

inline const PRUint32 nsCacheModule::Entries() const 
{
    return m_Entries;
}

inline 
nsEnumeration* nsCacheModule::Enumeration(void) const
{
    MonitorLocker ml((nsMonitorable*)this);
    if (!m_pEnumeration)
    {
        PR_ASSERT(m_pIterator);
        ((nsCacheModule*)this)->m_pEnumeration = new nsEnumeration((nsIterator*)m_pIterator); 
    }
    else
        ((nsCacheModule*)this)->m_pEnumeration->Reset();
    return m_pEnumeration;
}

inline PRBool nsCacheModule::IsEnabled(void) const
{
    return m_Enabled;
}

inline PRBool nsCacheModule::IsReadOnly(void) const
{
    return PR_FALSE;
}

inline nsCacheModule* nsCacheModule::NextModule(void) const 
{
    return m_pNext;
}

inline void nsCacheModule::NextModule(nsCacheModule* pNext) 
{
    /* No overwriting */
    PR_ASSERT(m_pNext == 0);
    if (m_pNext)
    {
        /* ERROR */
        delete m_pNext; //Worst case. 

    }
    m_pNext = pNext;
}

inline const PRUint32 nsCacheModule::Size() const
{
    return m_Size;
}

inline void nsCacheModule::SetSize(const PRUint32 size)
{
    m_Size = size;
}

inline PRUint32 nsCacheModule::SizeInUse(void) const
{
    return m_SizeInUse;
}

#endif // nsCacheModule_h__
