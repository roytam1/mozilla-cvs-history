/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
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
 * The Original Code is nsDiskCacheBindData.h, released May 10, 2001.
 * 
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are
 * Copyright (C) 2001 Netscape Communications Corporation.  All
 * Rights Reserved.
 * 
 * Contributor(s): 
 *    Gordon Sheridan  <gordon@netscape.com>
 *    Patrick C. Beard <beard@netscape.com>
 */


#ifndef _nsDiskCacheBindData_h_
#define _nsDiskCacheBindData_h_

#include "nspr.h"
#include "pldhash.h"

#include "nsISupports.h"
#include "nsCacheEntry.h"

#ifdef MOZ_NEW_CACHE_REUSE_TRANSPORTS
#include "nsITransport.h"
#endif


/******************************************************************************
 *  nsDiskCacheBindData
 *
 *  Created for disk cache specific data and stored in nsCacheEntry.mData as
 *  an nsISupports.  Also stored in nsDiskCacheHashTable, with collisions
 *  linked by the PRCList.
 *
 *****************************************************************************/

class nsDiskCacheBindData : public nsISupports, public PRCList {
public:
    NS_DECL_ISUPPORTS

    nsDiskCacheBindData(nsCacheEntry* entry)
        :   mCacheEntry(entry),
            mGeneration(0)
    {
        NS_INIT_ISUPPORTS();
        PR_INIT_CLIST(this);
        mHashNumber = Hash(entry->Key()->get());
    }

    virtual ~nsDiskCacheBindData()
    {
        PR_REMOVE_LINK(this);
    }

#ifdef MOZ_NEW_CACHE_REUSE_TRANSPORTS
    /**
     * Maps a cache access mode to a cached nsITransport for that access
     * mode. We keep these cached to avoid repeated trips to the
     * file transport service.
     */
    nsCOMPtr<nsITransport>& getTransport(nsCacheAccessMode mode)
    {
        return mTransports[mode - 1];
    }
#endif
    
    nsCacheEntry* getCacheEntry()
    {
        return mCacheEntry;
    }
    
    PRUint32 getGeneration()
    {
        return mGeneration;
    }
    
    void setGeneration(PRUint32 generation)
    {
        mGeneration = generation;
    }
    
    PLDHashNumber getHashNumber()
    {
        return mHashNumber;
    }
    
    nsrefcnt getRefCount()
    {
        return mRefCnt;
    }
    
    static PLDHashNumber Hash(const char* key);
    
private:
#ifdef MOZ_NEW_CACHE_REUSE_TRANSPORTS
    nsCOMPtr<nsITransport>  mTransports[3];
#endif
    nsCacheEntry*           mCacheEntry;    // back pointer to parent nsCacheEntry
    PRUint32                mGeneration;    // XXX part of nsDiskCacheRecord
    PLDHashNumber           mHashNumber;    // XXX part of nsDiskCacheRecord
// XXX    nsDiskCacheRecord       mMapRecord;
};


/******************************************************************************
 *  nsDiskCacheHashTable
 *
 *  Used to keep track of nsDiskCacheEntries associated with active/bound (and
 *  possibly doomed) entries.  Lookups on 4 byte disk hash to find collisions
 *  (which need to be doomed, instead of just evicted.  Collisions are linked
 *  using a PRCList to keep track of current generation number.
 *
 *****************************************************************************/

class nsDiskCacheHashTable {
public:
    nsDiskCacheHashTable();
    ~nsDiskCacheHashTable();

    nsresult                Init();

    nsDiskCacheBindData *   GetEntry(const char *              key);
    nsDiskCacheBindData *   GetEntry(PLDHashNumber             key);
    nsresult                AddEntry(nsDiskCacheBindData *     entry);
    void                    RemoveEntry(nsDiskCacheBindData *  entry);
    
    class Visitor {
    public:
        virtual PRBool      VisitEntry(nsDiskCacheBindData *       entry) = 0;
    };
    
    void                    VisitEntries(Visitor *              visitor);
    
private:
    struct HashTableEntry : PLDHashEntryHdr {
        nsDiskCacheBindData *  mDiskCacheBindData;                    // STRONG ref?
    };

    // PLDHashTable operation callbacks
    static const void *     PR_CALLBACK GetKey(PLDHashTable *               table,
                                               PLDHashEntryHdr *            entry);

    static PLDHashNumber    PR_CALLBACK HashKey(PLDHashTable *              table,
                                                const void *                key);

    static PRBool           PR_CALLBACK MatchEntry(PLDHashTable *           table,
                                                   const PLDHashEntryHdr *  entry,
                                                   const void *             key);

    static void             PR_CALLBACK MoveEntry(PLDHashTable *            table,
                                                  const PLDHashEntryHdr *   from,
                                                  PLDHashEntryHdr       *   to);

    static void             PR_CALLBACK ClearEntry(PLDHashTable *           table,
                                                   PLDHashEntryHdr *        entry);

    static void             PR_CALLBACK Finalize(PLDHashTable *table);

    static
    PLDHashOperator         PR_CALLBACK FreeCacheEntries(PLDHashTable *     table,
                                                         PLDHashEntryHdr *  hdr,
                                                         PRUint32           number,
                                                         void *             arg);
    static
    PLDHashOperator         PR_CALLBACK VisitEntry(PLDHashTable *           table,
                                                   PLDHashEntryHdr *        hdr,
                                                   PRUint32                 number,
                                                   void *                   arg);
                                     
    // member variables
    static PLDHashTableOps ops;
    PLDHashTable           table;
    PRBool                 initialized;
};

#endif /* _nsDiskCacheBindData_h_ */