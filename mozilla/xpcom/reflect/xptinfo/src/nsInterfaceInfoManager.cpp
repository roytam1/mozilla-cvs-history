/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
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
 * Copyright (C) 1999 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

/* Implementation of nsIInterfaceInfo. */

#ifdef XP_MAC
#include <stat.h>
#else
#include <sys/stat.h>
#endif
#include "nscore.h"

#include "nsSpecialSystemDirectory.h" 

#include "nsISupports.h"
#include "nsIInterfaceInfoManager.h"
#include "nsIInterfaceInfo.h"
#include "nsIServiceManager.h"
#include "nsIAllocator.h"

#include "nsInterfaceInfoManager.h"
#include "nsInterfaceInfo.h"
#include "xptinfo.h"

#include "prio.h"
#include "plstr.h"
#include "prenv.h"
#include "prlog.h"
#include "prprf.h"

// this after nsISupports, to pick up IID
// so that xpt stuff doesn't try to define it itself...
#include "xpt_xdr.h"

#ifdef DEBUG_mccabe
#define TRACE(x) fprintf x
#else
#define TRACE(x)
#endif


static NS_DEFINE_IID(kIIIManagerIID, NS_IINTERFACEINFO_MANAGER_IID);
NS_IMPL_ISUPPORTS(nsInterfaceInfoManager, kIIIManagerIID);

// static
nsInterfaceInfoManager*
nsInterfaceInfoManager::GetInterfaceInfoManager()
{
    static nsInterfaceInfoManager* impl = NULL;
    if(!impl)
    {
        impl = new nsInterfaceInfoManager();
        if(!impl->ctor_succeeded)
            NS_RELEASE(impl);
    }
    if(impl)
        NS_ADDREF(impl);
    return impl;
}

// static
nsIAllocator*
nsInterfaceInfoManager::GetAllocator(nsInterfaceInfoManager* iim /*= NULL*/)
{
    nsIAllocator* al;
    nsInterfaceInfoManager* iiml = iim;

    if(!iiml && !(iiml = GetInterfaceInfoManager()))
        return NULL;
    if(NULL != (al = iiml->allocator))
        NS_ADDREF(al);
    if(!iim)
        NS_RELEASE(iiml);
    return al;
}

static NS_DEFINE_IID(kAllocatorCID, NS_ALLOCATOR_CID);
static NS_DEFINE_IID(kIAllocatorIID, NS_IALLOCATOR_IID);

nsInterfaceInfoManager::nsInterfaceInfoManager()
    : allocator(NULL), typelibRecords(NULL), ctor_succeeded(PR_FALSE)
{
    NS_INIT_REFCNT();
    NS_ADDREF_THIS();

    nsServiceManager::GetService(kAllocatorCID,
                                 kIAllocatorIID,
                                 (nsISupports **)&this->allocator);

    PR_ASSERT(this->allocator != NULL);

    if(NS_SUCCEEDED(this->initInterfaceTables()))
        ctor_succeeded = PR_TRUE;
}

static
XPTHeader *getHeader(const char *filename, nsIAllocator *al) {
    XPTState *state = NULL;
    XPTCursor curs, *cursor = &curs;
    XPTHeader *header = NULL;
    PRFileInfo fileinfo;
    PRUint32 flen;
    char *whole = NULL;
    PRFileDesc *in = NULL;

    if (PR_GetFileInfo(filename, &fileinfo) != PR_SUCCESS) {
        NS_ERROR("PR_GetFileInfo failed");
        return NULL;
    }
    flen = fileinfo.size;

    whole = (char *)al->Alloc(flen);
    if (!whole) {
        NS_ERROR("FAILED: allocation for whole");
        return NULL;
    }

    in = PR_Open(filename, PR_RDONLY, 0);
    if (!in) {
        NS_ERROR("FAILED: fopen");
        goto out;
    }

    if (flen > 0) {
       	PRInt32 howmany = PR_Read(in, whole, flen);
       	if (howmany < 0) {
           	NS_ERROR("FAILED: reading typelib file");
           	goto out;
       	}

#ifdef XP_MAC
		// Mac can lie about the filesize, because it includes the resource fork
		// (where our CVS information lives). So we'll just believe it if it read
		// less than flen bytes.
		flen = howmany;
#else
		if (howmany < flen) {
			NS_ERROR("typelib short read");
			goto out;
		}
#endif

        state = XPT_NewXDRState(XPT_DECODE, whole, flen);
        if (!XPT_MakeCursor(state, XPT_HEADER, 0, cursor)) {
            NS_ERROR("MakeCursor failed\n");
            goto out;
        }
        if (!XPT_DoHeader(cursor, &header)) {
            NS_ERROR("DoHeader failed\n");
            goto out;
        }
    }

 out:
    if (state != NULL)
        XPT_DestroyXDRState(state);
    if (whole != NULL)
        al->Free(whole);
    if (in != NULL)
        PR_Close(in);
    return header;
}

nsresult
nsInterfaceInfoManager::indexify_file(const char *filename)
{
    XPTHeader *header = getHeader(filename, this->allocator);
    if (header == NULL) {
        // XXX glean something more meaningful from getHeader?
        return NS_ERROR_FAILURE;
    }

    int limit = header->num_interfaces;
    nsTypelibRecord *tlrecord = new nsTypelibRecord(limit, this->typelibRecords,
                                                    header, this->allocator);
    this->typelibRecords = tlrecord; // add it to the list of typelibs

    for (int i = 0; i < limit; i++) {
        XPTInterfaceDirectoryEntry *current = header->interface_directory + i;

        // find or create an interface record, and set the appropriate
        // slot in the nsTypelibRecord array.
        nsID zero =
        { 0x0, 0x0, 0x0, { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 } };
        nsInterfaceRecord *record = NULL;

        PRBool iidIsZero = current->iid.Equals(zero);

        // XXX fix bogus repetitive logic.
        PRBool foundInIIDTable = PR_FALSE;

        if (iidIsZero == PR_FALSE) {
            // prefer the iid.
            nsIDKey idKey(current->iid);
            record = (nsInterfaceRecord *)this->IIDTable->Get(&idKey);
        } else {
            foundInIIDTable = PR_TRUE;
            // resort to using the name.  Warn?
            record = (nsInterfaceRecord *)PL_HashTableLookup(this->nameTable,
                                                             current->name);
        }

        // if none was found, create one and tuck it into the appropriate places.
        if (record == NULL) {
            record = new nsInterfaceRecord();
            record->typelibRecord = NULL;
            record->interfaceDescriptor = NULL;
            record->info = NULL;

            // XXX copy these values?
            record->name = current->name;
            record->name_space = current->name_space;

            // add it to the name->interfaceRecord table
            // XXX check that result (PLHashEntry *) isn't NULL
            PL_HashTableAdd(this->nameTable, current->name, record);

            if (iidIsZero == PR_FALSE) {
                // Add it to the iid table too, if we have an iid.
                // don't check against old value, b/c we shouldn't have one.
                foundInIIDTable = PR_TRUE;
                nsIDKey idKey(current->iid);
                this->IIDTable->Put(&idKey, record);
            }
        }
                
        // Is the entry we're looking at resolved?
        if (current->interface_descriptor != NULL) {
            if (record->interfaceDescriptor != NULL) {
                char *warnstr = PR_smprintf
                    ("interface %s in typelib %s overrides previous definition",
                     current->name, filename);
                NS_WARNING(warnstr);
                PR_smprintf_free(warnstr);
            }
            record->interfaceDescriptor = current->interface_descriptor;
            record->typelibRecord = tlrecord;
            record->iid = current->iid;

            if (foundInIIDTable == PR_FALSE) {
                nsIDKey idKey(current->iid);
                this->IIDTable->Put(&idKey, record);
            }
        }

        // all fixed up?  Put a pointer to the interfaceRecord we
        // found/made into the appropriate place.
        *(tlrecord->interfaceRecords + i) = record;
    }
    return NS_OK;
}

// as many InterfaceDirectoryEntries as we expect to see.
#define XPT_HASHSIZE 64

#ifdef DEBUG
PRIntn check_nametable_enumerator(PLHashEntry *he, PRIntn index, void *arg) {
    char *key = (char *)he->key;
    nsInterfaceRecord *value = (nsInterfaceRecord *)he->value;
    nsHashtable *iidtable = (nsHashtable *)arg;

    TRACE((stderr, "name table has %s\n", key));

    if (value->interfaceDescriptor == NULL) {
        TRACE((stderr, "unresolved interface %s\n", key));
    } else {
        nsIDKey idKey(value->iid);
        char *name_from_iid = (char *)iidtable->Get(&idKey);
        NS_ASSERTION(name_from_iid != NULL,
                     "no name assoc'd with iid for entry for name?");

        // XXX note that below is only ncc'ly the case if xdr doesn't give us
        // duplicated strings.
//          NS_ASSERTION(name_from_iid == key,
//                       "key and iid name xpected to be same");
    }
    return HT_ENUMERATE_NEXT;
}

PRBool check_iidtable_enumerator(nsHashKey *aKey, void *aData, void *closure) {
//      PLHashTable *nameTable = (PLHashTable *)closure;
    nsInterfaceRecord *record = (nsInterfaceRecord *)aData;
    // can I do anything with the key?
    TRACE((stderr, "record has name %s, iid %s\n",
            record->name, record->iid.ToString()));
    return PR_TRUE;
}

#endif

nsresult
nsInterfaceInfoManager::initInterfaceTables()
{
    // make a hashtable to map names to interface records.
    this->nameTable = PL_NewHashTable(XPT_HASHSIZE,
                                       PL_HashString,  // hash keys
                                       PL_CompareStrings, // compare keys
                                       PL_CompareValues, // comp values
                                       NULL, NULL);
    if (this->nameTable == NULL)
        return NS_ERROR_FAILURE;

    // make a hashtable to map iids to interface records.
    this->IIDTable = new nsHashtable(XPT_HASHSIZE);
    if (this->IIDTable == NULL) {
        PL_HashTableDestroy(this->nameTable);
        return NS_ERROR_FAILURE;
    }

    // this code stolen from SetupRegistry; it might bear further
    // examination, as the code there doesn't look quite done.
    nsSpecialSystemDirectory sysdir(nsSpecialSystemDirectory::OS_CurrentProcessDirectory);
    sysdir += "components";
//    const char *xptdirname = sysdir.GetCString(); // native path
    
#ifdef XP_MAC
	PRBool wasAlias;
	sysdir.ResolveAlias(wasAlias);
#endif

#ifdef DEBUG
    int which = 0;
#endif

	for (nsDirectoryIterator i(sysdir); i.Exists(); i++) {
		// XXX need to copy?
		nsFileSpec spec = i.Spec();
		
#ifdef XP_MAC
		spec.ResolveAlias(wasAlias);
#endif
		
		if (! spec.IsFile())
			continue;
		
		// XXX this assumes ASCII strings are returned from nsFileSpec. Is this valid?
		nsNSPRPath path(spec);
		const char* fullname = path; // path needs to stay in scope, beacuse fullname refers to an internal member
		int flen = PL_strlen(fullname);
        if (flen < 4 || PL_strcasecmp(&(fullname[flen - 4]), ".xpt"))
            continue;

        // it's a valid file, read it in.
#ifdef DEBUG
        which++;
        TRACE((stderr, "%d %s\n", which, fullname));
#endif

        nsresult nsr = this->indexify_file(fullname);
        if (NS_FAILED(nsr)) {
            char *warnstr = PR_smprintf("failed to process typelib file %s",
                                        fullname);
            NS_WARNING(warnstr);
            PR_smprintf_free(warnstr);
        }
    }

#ifdef DEBUG
    TRACE((stderr, "\nchecking name table for unresolved entries...\n"));
    // scan here to confirm that all interfaces are resolved.
    PL_HashTableEnumerateEntries(this->nameTable,
                                 check_nametable_enumerator,
                                 this->IIDTable);

    TRACE((stderr, "\nchecking iid table for unresolved entries...\n"));
    IIDTable->Enumerate(check_iidtable_enumerator, this->nameTable);

#endif
    return NS_OK;
}

nsInterfaceInfoManager::~nsInterfaceInfoManager()
{
    // let the singleton leak
}

NS_IMETHODIMP
nsInterfaceInfoManager::GetInfoForIID(const nsIID* iid,
                                      nsIInterfaceInfo **info)
{
    nsIDKey idKey(*iid);
    nsInterfaceRecord *record =
        (nsInterfaceRecord *)this->IIDTable->Get(&idKey);
    if (record == NULL) {
        *info = NULL;
        return NS_ERROR_FAILURE;
    }
    
    return record->GetInfo((nsInterfaceInfo **)info);
}

NS_IMETHODIMP
nsInterfaceInfoManager::GetInfoForName(const char* name,
                                       nsIInterfaceInfo **info)
{
    nsInterfaceRecord *record =
        (nsInterfaceRecord *)PL_HashTableLookup(this->nameTable, name);
    if (record == NULL) {
        *info = NULL;
        return NS_ERROR_FAILURE;
    }

    return record->GetInfo((nsInterfaceInfo **)info);
}

NS_IMETHODIMP
nsInterfaceInfoManager::GetIIDForName(const char* name, nsIID** iid)
{
    nsInterfaceRecord *record =
        (nsInterfaceRecord *)PL_HashTableLookup(this->nameTable, name);
    if (record == NULL) {
        *iid = NULL;
        return NS_ERROR_FAILURE;
    }
    
    return record->GetIID(iid);
}

NS_IMETHODIMP
nsInterfaceInfoManager::GetNameForIID(const nsIID* iid, char** name)
{
    nsIDKey idKey(*iid);
    nsInterfaceRecord *record =
        (nsInterfaceRecord *)this->IIDTable->Get(&idKey);
    if (record == NULL) {
        *name = NULL;
        return NS_ERROR_FAILURE;
    }

#ifdef DEBUG
    // Note that this might fail for same-name, different-iid interfaces!
    nsIID *newid;
    nsresult isok = GetIIDForName(record->name, &newid);
    PR_ASSERT(!(NS_FAILED(isok)));
    PR_ASSERT(newid->Equals(*newid));
#endif
    PR_ASSERT(record->name != NULL);
    
    char *p;
    int len = strlen(record->name) + 1;
    if((p = (char *)this->allocator->Alloc(len)) == NULL) {
        *name = NULL;
        return NS_ERROR_FAILURE;
    }
    memcpy(p, record->name, len);
    *name = p;
    return NS_OK;
}    

XPTI_PUBLIC_API(nsIInterfaceInfoManager*)
XPTI_GetInterfaceInfoManager()
{
    return nsInterfaceInfoManager::GetInterfaceInfoManager();
}
