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
 * The Original Code is nsCacheEntry.h, released February 22, 2001.
 * 
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are
 * Copyright (C) 2001 Netscape Communications Corporation.  All
 * Rights Reserved.
 * 
 * Contributor(s): 
 *    Gordon Sheridan, 22-February-2001
 */

#ifndef _nsCacheEntry_h_
#define _nsCacheEntry_h_

#include "nspr.h"
#include "pldhash.h"
#include "nscore.h"
#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsAReadableString.h"

#include "nsICache.h"
#include "nsICacheEntryDescriptor.h"
#include "nsCacheMetaData.h"


class nsCacheDevice;
class nsCacheMetaData;
class nsCacheRequest;
class nsCacheEntryDescriptor;


class nsCacheEntry : public PRCList
{
public:

    nsCacheEntry(nsCString *          key,
                 PRBool               streamBased,
                 nsCacheStoragePolicy storagePolicy);
    ~nsCacheEntry();

    nsCacheEntry *
    Create(nsCString *           key,
           PRBool                streamBased,
           nsCacheStoragePolicy  storagePolicy,
           nsCacheDevice *       cacheDevice);

    nsCString *  Key(void)  { return mKey; }

    PRInt32  FetchCount(void)                          { return mFetchCount;}
    void     SetFetchCount( PRInt32   count)           { mFetchCount = count;}

    PRUint32 LastFetched(void)                         { return mLastFetched;}
    void     SetLastFetched( PRUint32  lastFetched)    { mLastFetched = lastFetched;}

    PRUint32 LastModified(void)                        { return mLastModified;}
    void     SetLastModified( PRUint32 lastModified)   { mLastModified = lastModified;}

    PRUint32 LastValidated(void)                       { return mLastValidated;}
    void     SetLastValidated( PRUint32 lastValidated) { mLastValidated = lastValidated;}

    PRUint32 ExpirationTime(void)                 { return mExpirationTime;}
    void     SetExpirationTime( PRUint32 expires) { mExpirationTime = expires;}

    PRUint32 DataSize(void)                       { return mDataSize;}
    void     SetDataSize( PRUint32  size);

    PRUint32 MetaDataSize(void)                   { return mMetaSize;}

    PRUint32 Size(void)                           { return mDataSize + mMetaSize; }

    nsCacheDevice * CacheDevice(void)                        { return mCacheDevice;}
    void            SetCacheDevice( nsCacheDevice * device)  { mCacheDevice = device;}

    nsresult GetSecurityInfo( nsISupports ** result);
    nsresult SetSecurityInfo( nsISupports *  info);

    nsresult GetData( nsISupports ** result);
    nsresult SetData( nsISupports *  data);

    nsresult GetMetaDataElement( const nsAReadableCString&  key,
                                 nsAReadableCString **      value);
    nsresult SetMetaDataElement( const nsAReadableCString&  key,
                                 const nsAReadableCString&  value);

    nsresult FlattenMetaData(char ** data, PRUint32 * size);
    nsresult UnflattenMetaData(char * data, PRUint32 size);

    // XXX enumerate MetaData method


    enum CacheEntryFlags {
        eStoragePolicyMask   = 0x000000FF,
        eDoomedMask          = 0x00000100,
        eEntryDirtyMask      = 0x00000200,
        eDataDirtyMask       = 0x00000400,
        eMetaDataDirtyMask   = 0x00000800,
        eStreamDataMask      = 0x00001000,
        eActiveMask          = 0x00002000,
        eInitializedMask     = 0x00004000,
        eValidMask           = 0x00008000
    };

    void MarkEntryDirty()      { mFlags |=  eEntryDirtyMask; }
    void MarkEntryClean()      { mFlags &= ~eEntryDirtyMask; }
    void MarkDataDirty()       { mFlags |=  eDataDirtyMask; }
    void MarkDataClean()       { mFlags &= ~eDataDirtyMask; }
    void MarkMetaDataDirty()   { mFlags |=  eMetaDataDirtyMask; }
    void MarkMetaDataClean()   { mFlags &= ~eMetaDataDirtyMask; }
    void MarkStreamData()      { mFlags |=  eStreamDataMask; }
    void MarkValid()           { mFlags |=  eValidMask; }
    void MarkInvalid()         { mFlags &= ~eValidMask; }
    //    void MarkAllowedInMemory() { mFlags |=  eAllowedInMemoryMask; }
    //    void MarkAllowedOnDisk()   { mFlags |=  eAllowedOnDiskMask; }

    PRBool IsDoomed()          { return (mFlags & eDoomedMask) != 0; }
    PRBool IsEntryDirty()      { return (mFlags & eEntryDirtyMask) != 0; }
    PRBool IsDataDirty()       { return (mFlags & eDataDirtyMask) != 0; }
    PRBool IsMetaDataDirty()   { return (mFlags & eMetaDataDirtyMask) != 0; }
    PRBool IsStreamData()      { return (mFlags & eStreamDataMask) != 0; }
    PRBool IsActive()          { return (mFlags & eActiveMask) != 0; }
    PRBool IsInitialized()     { return (mFlags & eInitializedMask) != 0; }
    PRBool IsValid()           { return (mFlags & eValidMask) != 0; }
    PRBool IsInvalid()         { return (mFlags & eValidMask) == 0; }
    PRBool IsInUse()           { return !(PR_CLIST_IS_EMPTY(&mRequestQ) &&
                                          PR_CLIST_IS_EMPTY(&mDescriptorQ)); }
    PRBool IsNotInUse()        { return (PR_CLIST_IS_EMPTY(&mRequestQ) &&
                                         PR_CLIST_IS_EMPTY(&mDescriptorQ)); }


    PRBool IsAllowedInMemory()
    {
        return (StoragePolicy() ==  nsICache::STORE_ANYWHERE) ||
            (StoragePolicy() == nsICache::STORE_IN_MEMORY);
    }

    PRBool IsAllowedOnDisk()
    {
        return (StoragePolicy() == nsICache::STORE_ON_DISK) ||
            (StoragePolicy() == nsICache::STORE_ON_DISK_AS_FILE);
    }

    nsCacheStoragePolicy  StoragePolicy()
    {
        return (nsCacheStoragePolicy)(mFlags & eStoragePolicyMask);
    }

    void SetStoragePolicy(nsCacheStoragePolicy policy)
    {
        NS_ASSERTION(policy <= 0xFF, "too many bits in nsCacheStoragePolicy");
        mFlags &= ~eStoragePolicyMask; // clear storage policy bits
        mFlags |= policy;
    }


    // methods for nsCacheService
    nsresult RequestAccess( nsCacheRequest * request, nsCacheAccessMode *accessGranted);
    nsresult CreateDescriptor( nsCacheRequest *           request,
                               nsCacheAccessMode          accessGranted,
                               nsICacheEntryDescriptor ** result);

    //    nsresult Open(nsCacheRequest *request, nsICacheEntryDescriptor ** result);
    //    nsresult AsyncOpen(nsCacheRequest *request);
    PRBool   RemoveRequest( nsCacheRequest * request);
    PRBool   RemoveDescriptor( nsCacheEntryDescriptor * descriptor);
    
private:
    friend class nsCacheEntryHashTable;
    friend class nsCacheService;

    void     DetachDescriptors(void);

    // internal methods
    nsresult CommonOpen(nsCacheRequest * request, nsCacheAccessMode *accessGranted);
    void MarkDoomed()          { mFlags |=  eDoomedMask; }
    void MarkStreamBased()     { mFlags |=  eStreamDataMask; }
    void MarkInitialized()     { mFlags |=  eInitializedMask; }
    void MarkActive()          { mFlags |=  eActiveMask; }
    void MarkInactive()        { mFlags &= ~eActiveMask; }

    nsCString *            mKey;            // 4  // XXX ask scc about const'ness
    PRUint32               mFetchCount;     // 4
    PRUint32               mLastFetched;    // 4
    PRUint32               mLastModified;   // 4
    PRUint32               mLastValidated;  // 4
    PRUint32               mExpirationTime; // 4
    PRUint32               mFlags;          // 4
    PRUint32               mDataSize;       // 4
    PRUint32               mMetaSize;       // 4
    nsCacheDevice *        mCacheDevice;    // 4
    nsCOMPtr<nsISupports>  mSecurityInfo;   // 
    nsCOMPtr<nsISupports>  mData;           // 
    nsCacheMetaData *      mMetaData;       // 4
    PRCList                mRequestQ;       // 8
    PRCList                mDescriptorQ;    // 8
};


typedef struct {
    PLDHashNumber  keyHash;
    nsCacheEntry  *cacheEntry;
} nsCacheEntryHashTableEntry;


class nsCacheEntryHashTable
{
public:
    nsCacheEntryHashTable();
    ~nsCacheEntryHashTable();

    nsresult      Init();

    nsCacheEntry *GetEntry( const nsCString * key);
    nsresult      AddEntry( nsCacheEntry *entry);
    void          RemoveEntry( nsCacheEntry *entry);
    
    // XXX enumerate entries?
    class Visitor {
    public:
        virtual PRBool VisitEntry( nsCacheEntry *entry) = 0;
    };
    
    void          VisitEntries( Visitor *visitor);
    
private:
    friend class nsCacheService; // XXX redefine interface so this isn't necessary

    // PLDHashTable operation callbacks
    static const void *   GetKey( PLDHashTable *table, PLDHashEntryHdr *entry);

    static PLDHashNumber  HashKey( PLDHashTable *table, const void *key);

    static PRBool         MatchEntry( PLDHashTable *           table,
                                      const PLDHashEntryHdr *  entry,
                                      const void *             key);

    static void           MoveEntry( PLDHashTable *table,
                                     const PLDHashEntryHdr *from,
                                     PLDHashEntryHdr       *to);

    static void           ClearEntry( PLDHashTable *table, PLDHashEntryHdr *entry);

    static void           Finalize( PLDHashTable *table);

    static
    PLDHashOperator       FreeCacheEntries(PLDHashTable *    table,
                                           PLDHashEntryHdr * hdr,
                                           PRUint32          number,
                                           void *            arg);
    static
    PLDHashOperator       VisitEntry(PLDHashTable *         table,
                                     PLDHashEntryHdr *      hdr,
                                     PRUint32               number,
                                     void *                 arg);
                                     
    // member variables
    static PLDHashTableOps ops;
    PLDHashTable           table;
    PRBool                 initialized;
};

#endif // _nsCacheEntry_h_

