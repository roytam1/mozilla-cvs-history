/* -*- Mode: IDL; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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

#include "nsNetModuleMgr.h"
#include "nsNetModRegEntry.h"
#include "nsEnumeratorUtils.h" // for nsArrayEnumerator
#include "nsString2.h"
#include "nsIEventQueue.h"

// Entry routines.
static PRBool DeleteEntry(nsISupports *aElement, void *aData) {
    NS_ASSERTION(aElement, "null pointer");
    NS_RELEASE(aElement);
    return PR_TRUE;
}


///////////////////////////////////
//// nsISupports
///////////////////////////////////

NS_IMPL_ISUPPORTS(nsNetModuleMgr, nsINetModuleMgr::GetIID());


///////////////////////////////////
//// nsINetModuleMgr
///////////////////////////////////

NS_IMETHODIMP
nsNetModuleMgr::RegisterModule(const char *aTopic, nsIEventQueue *aEventQueue, nsINetNotify *aNotify, const nsCID * aCID) {
    nsresult rv;
    PRUint32 cnt;

    nsINetModRegEntry* newEntryI = nsnull;
    nsNetModRegEntry *newEntry =
        new nsNetModRegEntry(aTopic, aEventQueue, aNotify, *aCID);
    if (!newEntry)
        return NS_ERROR_OUT_OF_MEMORY;

    rv = newEntry->QueryInterface(nsINetModRegEntry::GetIID(), (void**)&newEntryI);
    if (NS_FAILED(rv)) return rv;

    // Check for a previous registration
    PR_Lock(mLock);
    mEntries->Count(&cnt);
    for (PRUint32 i = 0; i < cnt; i++) {
        nsINetModRegEntry* curEntry = NS_STATIC_CAST(nsINetModRegEntry*, mEntries->ElementAt(i));
        PRBool same = PR_FALSE;
        rv = newEntryI->Equals(curEntry, &same);
        if (NS_FAILED(rv)) return rv;

        // if we've already got this one registered, yank it, and replace it with the new one
        if (same) {
            NS_RELEASE(curEntry);
            mEntries->DeleteElementAt(i);
            break;
        }
    }

    mEntries->AppendElement(NS_STATIC_CAST(nsISupports*, newEntryI));
    PR_Unlock(mLock);
    NS_RELEASE(newEntryI);
    return NS_OK;
}

NS_IMETHODIMP
nsNetModuleMgr::UnregisterModule(const char *aTopic, nsIEventQueue *aEventQueue, nsINetNotify *aNotify, const nsCID * aCID) {

    PR_Lock(mLock);
    nsresult rv;
    PRUint32 cnt;

    nsINetModRegEntry* tmpEntryI = nsnull;
    nsNetModRegEntry *tmpEntry =
        new nsNetModRegEntry(aTopic, aEventQueue, aNotify, *aCID);
    if (!tmpEntry)
        return NS_ERROR_OUT_OF_MEMORY;

    rv = tmpEntry->QueryInterface(nsINetModRegEntry::GetIID(), (void**)&tmpEntryI);
    if (NS_FAILED(rv)) return rv;

    mEntries->Count(&cnt);
    for (PRUint32 i = 0; i < cnt; i++) {
        nsINetModRegEntry* curEntry = NS_STATIC_CAST(nsINetModRegEntry*, mEntries->ElementAt(i));
        NS_ADDREF(curEntry); // get our ref to it
        PRBool same = PR_FALSE;
        rv = tmpEntryI->Equals(curEntry, &same);
        if (NS_FAILED(rv)) return rv;
        if (same) {
            NS_RELEASE(curEntry);
            mEntries->DeleteElementAt(i);
        }
        NS_RELEASE(curEntry); // ditch our ref to it
    }
    PR_Unlock(mLock);
    NS_RELEASE(tmpEntryI);
    return NS_OK;
}

NS_IMETHODIMP
nsNetModuleMgr::EnumerateModules(const char *aTopic, nsISimpleEnumerator **aEnumerator) {

    nsresult rv;
    PRUint32 cnt;
    char *topic = nsnull;

    // get all the entries for this topic
    
    rv = mEntries->Count(&cnt);
    if (NS_FAILED(rv)) return rv;

    // create the new array
    nsISupportsArray *topicEntries = nsnull;
    rv = NS_NewISupportsArray(&topicEntries);
    if (NS_FAILED(rv)) return rv;

    // run through the main entry array looking for topic matches.
    for (PRUint32 i = 0; i < cnt; i++) {
        nsINetModRegEntry *entry = NS_STATIC_CAST(nsINetModRegEntry*, mEntries->ElementAt(i));

        rv = entry->GetMTopic(&topic);
        if (NS_FAILED(rv)) {
            NS_RELEASE(topicEntries);
            NS_RELEASE(entry);
            return rv;
        }

        if (!PL_strcmp(aTopic, topic)) {
            delete [] topic;
            topic = nsnull;
            // found a match, add it to the list
            rv = topicEntries->AppendElement(NS_STATIC_CAST(nsISupports*, entry));
            if (NS_FAILED(rv)) {
                NS_RELEASE(topicEntries);
                NS_RELEASE(entry);
                return rv;
            }
        }
        delete [] topic;
        topic = nsnull;
        NS_RELEASE(entry);
    }

    nsISimpleEnumerator *outEnum = nsnull;
    nsArrayEnumerator *arrEnum = new nsArrayEnumerator(topicEntries);
    NS_RELEASE(topicEntries);

    if (!arrEnum) {
        return NS_ERROR_OUT_OF_MEMORY;
    }

    rv = arrEnum->QueryInterface(nsISimpleEnumerator::GetIID(), (void**)&outEnum);
    if (NS_FAILED(rv)) {
        delete arrEnum;
        return rv;
    }
    *aEnumerator = outEnum;
    return NS_OK;
}


///////////////////////////////////
//// nsNetModuleMgr
///////////////////////////////////

nsNetModuleMgr::nsNetModuleMgr() {
    NS_INIT_REFCNT();
    NS_NewISupportsArray(&mEntries);
    mLock    = PR_NewLock();
}

nsNetModuleMgr::~nsNetModuleMgr() {
    if (mEntries) {
        mEntries->EnumerateForwards(DeleteEntry, nsnull);
        NS_RELEASE(mEntries);
    }
    PR_DestroyLock(mLock);
}

NS_METHOD
nsNetModuleMgr::Create(nsISupports *aOuter, REFNSIID aIID, void **aResult)
{
    if (aOuter)
        return NS_ERROR_NO_AGGREGATION;

    nsNetModuleMgr* mgr = new nsNetModuleMgr();
    if (mgr == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(mgr);
    nsresult rv = mgr->QueryInterface(aIID, aResult);
    NS_RELEASE(mgr);
    return rv;
}
