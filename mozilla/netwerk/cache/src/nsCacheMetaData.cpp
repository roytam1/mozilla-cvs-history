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
 * The Original Code is nsCacheMetaData.cpp, released February 22, 2001.
 * 
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are
 * Copyright (C) 2001 Netscape Communications Corporation.  All
 * Rights Reserved.
 * 
 * Contributor(s): 
 *    Gordon Sheridan, 22-February-2001
 */

#include "nsCacheMetaData.h"
#include "nsString.h"



/*
 *  nsCacheClientHashTable
 */

PLDHashTableOps
nsCacheMetaData::ops =
{
    PL_DHashAllocTable,
    PL_DHashFreeTable,
    GetKey,
    HashKey,
    MatchEntry,
    MoveEntry,
    ClearEntry,
    Finalize
};


nsCacheMetaData::nsCacheMetaData()
    : initialized(0)
{
}

nsCacheMetaData::~nsCacheMetaData()
{
    //** maybe we should finalize the table...
}


nsresult
nsCacheMetaData::Init()
{
    nsresult rv = NS_OK;
    initialized = PL_DHashTableInit(&table, &ops, nsnull,
                                    sizeof(nsCacheMetaDataHashTableEntry), 16);

    if (!initialized) rv = NS_ERROR_OUT_OF_MEMORY;
    return rv;
}


nsAReadableCString *
nsCacheMetaData::GetElement(const nsAReadableCString * key)
{
    PLDHashEntryHdr * hashEntry;
    nsCString *       result = nsnull;

    NS_ASSERTION(initialized, "nsCacheMetaDataHashTable not initialized");
    hashEntry = PL_DHashTableOperate(&table, key, PL_DHASH_LOOKUP);
    if (PL_DHASH_ENTRY_IS_BUSY(hashEntry)) {
        result = ((nsCacheMetaDataHashTableEntry *)hashEntry)->value;
    }
    return result;
}


nsresult
nsCacheMetaData::SetElement(const nsAReadableCString * key,
                            const nsAReadableCString * value)
{
  nsCacheMetaDataHashTableEntry * metaEntry;

  NS_ASSERTION(initialized, "nsCacheMetaDataHashTable not initialized");
  if (!key) return NS_ERROR_NULL_POINTER;
  //** should value == nsnull remove the key?

  metaEntry = (nsCacheMetaDataHashTableEntry *)
      PL_DHashTableOperate(&table, key, PL_DHASH_ADD);
  if (metaEntry->key == nsnull) {
      metaEntry->key = new nsCString(*key);
      if (metaEntry->key == nsnull)
          return NS_ERROR_OUT_OF_MEMORY;
  }
  if (metaEntry->value != nsnull)
      delete metaEntry->value;

  if (value) {
      metaEntry->value = new nsCString(*value);
      if (metaEntry->value == nsnull)
          return NS_ERROR_OUT_OF_MEMORY;
  } else {
      metaEntry->value = nsnull;
  }

  return NS_OK;
}


//** enumerate MetaData elements



/*
 *  hash table operation callback functions
 */

const void *
nsCacheMetaData::GetKey( PLDHashTable * /*table*/, PLDHashEntryHdr *hashEntry)
{
    return ((nsCacheMetaDataHashTableEntry *)hashEntry)->key;
}

PRBool
nsCacheMetaData::MatchEntry(PLDHashTable *       /* table */,
				     const PLDHashEntryHdr * hashEntry,
				     const void *            key)
{
    NS_ASSERTION(key !=  nsnull, "nsCacheMetaDataHashTable::MatchEntry : null key");
    nsCString * entryKey = ((nsCacheMetaDataHashTableEntry *)hashEntry)->key;
    NS_ASSERTION(entryKey, "hashEntry->key == nsnull");
    return nsStr::StrCompare(*(nsCString *)key, *entryKey, -1, PR_FALSE) == 0;
}


void
nsCacheMetaData::MoveEntry(PLDHashTable * /* table */,
				    const PLDHashEntryHdr *from,
				    PLDHashEntryHdr       *to)
{
    to->keyHash = from->keyHash;
    ((nsCacheMetaDataHashTableEntry *)to)->key =
        ((nsCacheMetaDataHashTableEntry *)from)->key;
    ((nsCacheMetaDataHashTableEntry *)to)->value =
        ((nsCacheMetaDataHashTableEntry *)from)->value;
}


void
nsCacheMetaData::ClearEntry(PLDHashTable * /* table */,
                                  PLDHashEntryHdr * hashEntry)
{
    ((nsCacheMetaDataHashTableEntry *)hashEntry)->keyHash  = 0;
    ((nsCacheMetaDataHashTableEntry *)hashEntry)->key      = 0;
    ((nsCacheMetaDataHashTableEntry *)hashEntry)->value    = 0;
}

void
nsCacheMetaData::Finalize(PLDHashTable * /* table */)
{
    //** gee, if there's anything left in the table, maybe we should get rid of it.
}

