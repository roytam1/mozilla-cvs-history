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

/* 
* nsMemModule. The memory based cache module.  
*
* Gagan Saksena
* 02/03/98
* 
*/

#ifndef _nsMemModule_H_
#define _nsMemModule_H_

#include "nsCacheModule.h"
#include "nsMemCacheObject.h"
#include "nsCachePref.h"

//#include "nsHash.h" // TODO - replace with nsHashtable when the XPCOM_BRANCH merges

//#include "nsHashtable.h"
/*
// {5D51B250-E6C2-11d1-AFE5-006097BFC036}
static const NS_MEMMODULE_IID = 
{ 0x5d51b250, 0xe6c2, 0x11d1, { 0xaf, 0xe5, 0x0, 0x60, 0x97, 0xbf, 0xc0, 0x36 } };
*/

class nsMemModule : public nsCacheModule
{

public:
    nsMemModule(const PRUint32 size=nsCachePref::GetInstance()->MemCacheSize());
    ~nsMemModule();

/*
    NS_IMETHOD              QueryInterface(const nsIID& aIID, 
                                           void** aInstancePtr);
    NS_IMETHOD_(nsrefcnt)   AddRef(void);
    NS_IMETHOD_(nsrefcnt)   Release(void);

*/
    PRBool          AddObject(nsCacheObject* io_pObject);
    
    PRBool          Contains(nsCacheObject* io_pObject) const;
    PRBool          Contains(const char* i_url) const;
    
    void            GarbageCollect(void);

    nsCacheObject*  GetObject(const PRUint32 i_index) const;
    nsCacheObject*  GetObject(const char* i_url) const;

    nsStream*       GetStreamFor(const nsCacheObject* i_pObject);

    PRBool          ReduceSizeTo(const PRUint32 i_NewSize);

    PRBool          Remove(const char* i_url);
    PRBool          Remove(const PRUint32 i_index);

//    PRBool          RemoveAll(void);
    
    PRBool          Revalidate(void);


    // Start of nsMemModule specific stuff...
    // Here is a sample implementation using linked list
protected:
    nsMemCacheObject* LastObject(void) const;

private:
    nsMemCacheObject* m_pFirstObject;

    //nsHash m_ht; //TODO replace with nsHashtable
    //Optimization
    nsMemCacheObject* m_pLastObject;

    nsMemModule(const nsMemModule& mm);
    nsMemModule& operator=(const nsMemModule& mm);

/*    
    class nsMemKey : public nsHashKey
    {
    public:
                    nsMemKey();
                    ~nsMemKey();
      PRUint32      HashValue();
      PRBool        Equals(nsHashKey *aKey);
      nsHashKey*    Clone();
    };
*/    
};

inline
PRBool nsMemModule::Revalidate(void)
{
    /* Mem module elements are never revalidated */
    return PR_FALSE; 
}

#endif
