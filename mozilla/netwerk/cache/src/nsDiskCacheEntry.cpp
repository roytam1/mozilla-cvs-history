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
 * The Original Code is nsDiskCacheEntry.cpp, released May 10, 2001.
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

#include "nsDiskCache.h"
#include "nsDiskCacheEntry.h"
#include "nsDiskCacheBinding.h"
#include "nsDiskCacheMap.h"

#include "nsCache.h"


/******************************************************************************
 *  nsDiskCacheEntry
 *****************************************************************************/

/**
 *  CreateCacheEntry()
 *
 *  Creates an nsCacheEntry and sets all fields except for the binding.
 */
nsCacheEntry *
nsDiskCacheEntry::CreateCacheEntry(nsCacheDevice *  device)
{
    nsCacheEntry * entry = nsnull;
    nsresult       rv = nsCacheEntry::Create(mKeyStart,
                                             nsICache::STREAM_BASED,
                                             nsICache::STORE_ON_DISK,
                                             device,
                                             &entry);
    if (NS_FAILED(rv) || !entry) return nsnull;
    
    entry->SetFetchCount(mFetchCount);
    entry->SetLastFetched(mLastFetched);
    entry->SetLastModified(mLastModified);
    entry->SetExpirationTime(mExpirationTime);
    entry->SetCacheDevice(device);
    // XXX why does nsCacheService have to fill out device in BindEntry()?
    entry->SetDataSize(mDataSize);
    
    rv = entry->UnflattenMetaData(&mKeyStart[mKeySize], mMetaDataSize);
    if (NS_FAILED(rv)) {
        delete entry;
        return nsnull;
    }
    
    return entry;                      
}


/**
 *  CheckConsistency()
 *
 *  Perform a few simple checks to verify the data looks reasonable.
 */
PRBool
nsDiskCacheEntry::CheckConsistency(PRUint32  size)
{
    if ((mHeaderVersion != nsDiskCache::kCurrentVersion) ||
        (Size() > size) ||
        (mKeySize == 0) ||
        (mKeyStart[mKeySize - 1] != 0)) // key is null terminated
        return PR_FALSE;
    
    return PR_TRUE;
}


/**
 *  CreateDiskCacheEntry(nsCacheEntry * entry)
 *
 *  Prepare an nsCacheEntry for writing to disk
 */
nsDiskCacheEntry *
CreateDiskCacheEntry(nsDiskCacheBinding *  binding)
{
    nsCacheEntry * entry = binding->mCacheEntry;
    if (!entry)  return nsnull;
    
    PRUint32  keySize = entry->Key()->Length() + 1;
    PRUint32  size = sizeof(nsDiskCacheEntry) +
                     keySize + entry->MetaDataSize();
    
    // pad size so we can write to block files without overrunning buffer
    PRInt32 pad = size;
    if      (pad <  1024) pad =  1024;
    else if (pad <  4096) pad =  4096;
    else if (pad < 16384) pad = 16384;
    // XXX be more precise
    
    nsDiskCacheEntry * diskEntry = (nsDiskCacheEntry *)new char[pad];
    if (!diskEntry)  return nsnull;
    
    diskEntry->mHeaderVersion   = nsDiskCache::kCurrentVersion;
    diskEntry->mMetaLocation    = binding->mRecord.MetaLocation();
    diskEntry->mFetchCount      = entry->FetchCount();
    diskEntry->mLastFetched     = entry->LastFetched();
    diskEntry->mLastModified    = entry->LastModified();
    diskEntry->mExpirationTime  = entry->ExpirationTime();
    diskEntry->mDataSize        = entry->DataSize();
    diskEntry->mKeySize         = keySize;
    diskEntry->mMetaDataSize    = entry->MetaDataSize();
    
    nsCRT::memcpy(diskEntry->mKeyStart, entry->Key()->get(),keySize);
    
    // XXX FIXME FlattenMetaData should not allocate a buffer
    char *    metaData = nsnull;
    PRUint32  metaSize = 0;
    nsresult rv = entry->FlattenMetaData(&metaData, &metaSize);
    if (NS_FAILED(rv)) {
        delete diskEntry;
        return nsnull;
    }
    
    diskEntry->mMetaDataSize    = metaSize;
    if (metaSize)
        nsCRT::memcpy(&diskEntry->mKeyStart[keySize], metaData, metaSize);
    
    delete metaData;
    return  diskEntry;
}


/******************************************************************************
 *  nsDiskCacheEntryInfo
 *****************************************************************************/

NS_IMPL_ISUPPORTS1(nsDiskCacheEntryInfo, nsICacheEntryInfo);

NS_IMETHODIMP nsDiskCacheEntryInfo::GetClientID(char ** clientID)
{
    NS_ENSURE_ARG_POINTER(clientID);
    return ClientIDFromCacheKey(nsLiteralCString(mDiskEntry->mKeyStart), clientID);
}

extern const char DISK_CACHE_DEVICE_ID[];
NS_IMETHODIMP nsDiskCacheEntryInfo::GetDeviceID(char ** deviceID)
{
    NS_ENSURE_ARG_POINTER(deviceID);
    *deviceID = nsCRT::strdup(mDeviceID);
    return *deviceID ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}


NS_IMETHODIMP nsDiskCacheEntryInfo::GetKey(char ** clientKey)
{
    NS_ENSURE_ARG_POINTER(clientKey);
    return ClientKeyFromCacheKey(nsLiteralCString(mDiskEntry->mKeyStart), clientKey);
}

NS_IMETHODIMP nsDiskCacheEntryInfo::GetFetchCount(PRInt32 *aFetchCount)
{
    NS_ENSURE_ARG_POINTER(aFetchCount);
    *aFetchCount = mDiskEntry->mFetchCount;
    return NS_OK;
}

NS_IMETHODIMP nsDiskCacheEntryInfo::GetLastFetched(PRUint32 *aLastFetched)
{
    NS_ENSURE_ARG_POINTER(aLastFetched);
    *aLastFetched = mDiskEntry->mLastFetched;
    return NS_OK;
}

NS_IMETHODIMP nsDiskCacheEntryInfo::GetLastModified(PRUint32 *aLastModified)
{
    NS_ENSURE_ARG_POINTER(aLastModified);
    *aLastModified = mDiskEntry->mLastModified;
    return NS_OK;
}

NS_IMETHODIMP nsDiskCacheEntryInfo::GetExpirationTime(PRUint32 *aExpirationTime)
{
    NS_ENSURE_ARG_POINTER(aExpirationTime);
    *aExpirationTime = mDiskEntry->mExpirationTime;
    return NS_OK;
}

NS_IMETHODIMP nsDiskCacheEntryInfo::IsStreamBased(PRBool *aStreamBased)
{
    NS_ENSURE_ARG_POINTER(aStreamBased);
    *aStreamBased = PR_TRUE;
    return NS_OK;
}

NS_IMETHODIMP nsDiskCacheEntryInfo::GetDataSize(PRUint32 *aDataSize)
{
    NS_ENSURE_ARG_POINTER(aDataSize);
    *aDataSize = mDiskEntry->mDataSize;
    return NS_OK;
}
