/* 
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
 * The Original Code is the Netscape security libraries.
 * 
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are 
 * Copyright (C) 1994-2000 Netscape Communications Corporation.  All
 * Rights Reserved.
 * 
 * Contributor(s):
 * 
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU General Public License Version 2 or later (the
 * "GPL"), in which case the provisions of the GPL are applicable 
 * instead of those above.  If you wish to allow use of your 
 * version of this file only under the terms of the GPL and not to
 * allow others to use your version of this file under the MPL,
 * indicate your decision by deleting the provisions above and
 * replace them with the notice and other provisions required by
 * the GPL.  If you do not delete the provisions above, a recipient
 * may use your version of this file under either the MPL or the
 * GPL.
 */

#ifdef DEBUG
static const char CVS_ID[] = "@(#) $RCSfile$ $Revision$ $Date$ $Name$";
#endif /* DEBUG */

#ifndef PKIM_H
#include "pkim.h"
#endif /* PKIM_H */

#ifndef PKI_H
#include "pki.h"
#endif /* PKI_H */

#ifndef NSSPKI_H
#include "nsspki.h"
#endif /* NSSPKI_H */

#ifndef BASE_H
#include "base.h"
#endif /* BASE_H */

#ifndef PKISTORE_H
#include "pkistore.h"
#endif /* PKISTORE_H */

/* 
 * Certificate Store
 *
 * This differs from the cache in that it is a true storage facility.  Items
 * stay in until they are explicitly removed.  It is only used by crypto
 * contexts at this time, but may be more generally useful...
 *
 */

struct nssCertificateStoreStr 
{
    PRBool i_alloced_arena;
    NSSArena *arena;
    PZLock *lock;
    nssHash *subject;
    nssHash *issuer_and_serial;
};

typedef struct certificate_hash_entry_str certificate_hash_entry;

struct certificate_hash_entry_str
{
    NSSCertificate *cert;
    NSSTrust *trust;
    nssSMIMEProfile *profile;
};

/* XXX This a common function that should be moved out, possibly an
 *     nssSubjectCertificateList should be created?
 */
/* sort the subject list from newest to oldest */
static PRIntn subject_list_sort(void *v1, void *v2)
{
    NSSCertificate *c1 = (NSSCertificate *)v1;
    NSSCertificate *c2 = (NSSCertificate *)v2;
    nssDecodedCert *dc1 = nssCertificate_GetDecoding(c1);
    nssDecodedCert *dc2 = nssCertificate_GetDecoding(c2);
    if (dc1->isNewerThan(dc1, dc2)) {
	return -1;
    } else {
	return 1;
    }
}

NSS_IMPLEMENT nssCertificateStore *
nssCertificateStore_Create
(
  NSSArena *arenaOpt
)
{
    NSSArena *arena;
    nssCertificateStore *store;
    PRBool i_alloced_arena;
    if (arenaOpt) {
	arena = arenaOpt;
	i_alloced_arena = PR_FALSE;
    } else {
	arena = nssArena_Create();
	if (!arena) {
	    return NULL;
	}
	i_alloced_arena = PR_TRUE;
    }
    store = nss_ZNEW(arena, nssCertificateStore);
    if (!store) {
	goto loser;
    }
    store->lock = PZ_NewLock(nssILockOther);
    if (!store->lock) {
	goto loser;
    }
    /* Create the issuer/serial --> {cert, trust, S/MIME profile } hash */
    store->issuer_and_serial = nssHash_CreateCertificate(arena, 0);
    if (!store->issuer_and_serial) {
	goto loser;
    }
    /* Create the subject DER --> subject list hash */
    store->subject = nssHash_CreateItem(arena, 0);
    if (!store->subject) {
	goto loser;
    }
    store->arena = arena;
    store->i_alloced_arena = i_alloced_arena;
    return store;
loser:
    if (store) {
	if (store->lock) {
	    PZ_DestroyLock(store->lock);
	}
	if (store->issuer_and_serial) {
	    nssHash_Destroy(store->issuer_and_serial);
	}
	if (store->subject) {
	    nssHash_Destroy(store->subject);
	}
    }
    if (i_alloced_arena) {
	nssArena_Destroy(arena);
    }
    return NULL;
}

NSS_IMPLEMENT void
nssCertificateStore_Destroy
(
  nssCertificateStore *store
)
{
    PZ_DestroyLock(store->lock);
    nssHash_Destroy(store->issuer_and_serial);
    nssHash_Destroy(store->subject);
    if (store->i_alloced_arena) {
	nssArena_Destroy(store->arena);
    } else {
	nss_ZFreeIf(store);
    }
}

static PRStatus
add_certificate_entry
(
  nssCertificateStore *store,
  NSSCertificate *cert
)
{
    PRStatus nssrv;
    certificate_hash_entry *entry;
    entry = nss_ZNEW(store->arena, certificate_hash_entry);
    if (!entry) {
	return PR_FAILURE;
    }
    entry->cert = cert;
    nssrv = nssHash_Add(store->issuer_and_serial, cert, entry);
    if (nssrv != PR_SUCCESS) {
	nss_ZFreeIf(entry);
    }
    return nssrv;
}

static PRStatus
add_subject_entry
(
  nssCertificateStore *store,
  NSSCertificate *cert
)
{
    PRStatus nssrv;
    nssList *subjectList;
    subjectList = (nssList *)nssHash_Lookup(store->subject, &cert->subject);
    if (subjectList) {
	/* The subject is already in, add this cert to the list */
	nssrv = nssList_AddUnique(subjectList, cert);
    } else {
	/* Create a new subject list for the subject */
	subjectList = nssList_Create(store->arena, PR_FALSE);
	if (!subjectList) {
	    return PR_FAILURE;
	}
	nssList_SetSortFunction(subjectList, subject_list_sort);
	/* Add the cert entry to this list of subjects */
	nssrv = nssList_Add(subjectList, cert);
	if (nssrv != PR_SUCCESS) {
	    return nssrv;
	}
	/* Add the subject list to the cache */
	nssrv = nssHash_Add(store->subject, &cert->subject, subjectList);
    }
    return nssrv;
}

/* declared below */
static void
remove_certificate_entry
(
  nssCertificateStore *store,
  NSSCertificate *cert
);

NSS_IMPLEMENT PRStatus
nssCertificateStore_Add
(
  nssCertificateStore *store,
  NSSCertificate *cert
)
{
    PRStatus nssrv;
    PZ_Lock(store->lock);
    nssrv = add_certificate_entry(store, cert);
    if (nssrv == PR_SUCCESS) {
	nssrv = add_subject_entry(store, cert);
	if (nssrv == PR_SUCCESS) {
	    nssCertificate_AddRef(cert); /* obtain a reference for the store */
	} else {
	    remove_certificate_entry(store, cert);
	}
    }
    PZ_Unlock(store->lock);
    return nssrv;
}

static void
remove_certificate_entry
(
  nssCertificateStore *store,
  NSSCertificate *cert
)
{
    certificate_hash_entry *entry;
    entry = (certificate_hash_entry *)
                             nssHash_Lookup(store->issuer_and_serial, cert);
    if (entry) {
	nssHash_Remove(store->issuer_and_serial, cert);
	if (entry->trust) {
	    nssPKIObject_Destroy(&entry->trust->object);
	}
	if (entry->profile) {
	    nssPKIObject_Destroy(&entry->profile->object);
	}
	nss_ZFreeIf(entry);
    }
}

static void
remove_subject_entry
(
  nssCertificateStore *store,
  NSSCertificate *cert
)
{
    nssList *subjectList;
    /* Get the subject list for the cert's subject */
    subjectList = (nssList *)nssHash_Lookup(store->subject, &cert->subject);
    if (subjectList) {
	/* Remove the cert from the subject hash */
	nssList_Remove(subjectList, cert);
	if (nssList_Count(subjectList) == 0) {
	    nssHash_Remove(store->subject, &cert->subject);
	    nssList_Destroy(subjectList);
	}
    }
}

NSS_IMPLEMENT void
nssCertificateStore_Remove
(
  nssCertificateStore *store,
  NSSCertificate *cert
)
{
    PZ_Lock(store->lock);
    if (nssHash_Exists(store->issuer_and_serial, cert)) {
	remove_certificate_entry(store, cert);
	remove_subject_entry(store, cert);
	NSSCertificate_Destroy(cert); /* release the store's reference */
    }
    PZ_Unlock(store->lock);
}

NSS_IMPLEMENT NSSCertificate **
nssCertificateStore_FindCertificatesBySubject
(
  nssCertificateStore *store,
  NSSDER *subject,
  NSSCertificate *rvOpt[],
  PRUint32 maximumOpt,
  NSSArena *arenaOpt
)
{
    PRUint32 i, count;
    NSSCertificate **rvArray = NULL;
    nssList *subjectList;
    PZ_Lock(store->lock);
    subjectList = (nssList *)nssHash_Lookup(store->subject, subject);
    PZ_Unlock(store->lock);
    if (subjectList) {
	count = nssList_Count(subjectList);
	if (maximumOpt > 0) {
	    count = PR_MIN(maximumOpt, count);
	}
	if (rvOpt) {
	    nssList_GetArray(subjectList, (void **)rvOpt, count);
	    for (i=0; i<count; i++) nssCertificate_AddRef(rvOpt[i]);
	} else {
	    rvArray = nss_ZNEWARRAY(arenaOpt, NSSCertificate *, count + 1);
	    if (!rvArray) {
		return (NSSCertificate **)NULL;
	    }
	    nssList_GetArray(subjectList, (void **)rvArray, count);
	    for (i=0; i<count; i++) nssCertificate_AddRef(rvArray[i]);
	}
    }
    return rvArray;
}

/* Because only subject indexing is implemented, all other lookups require
 * full traversal (unfortunately, PLHashTable doesn't allow you to exit
 * early from the enumeration).  The assumptions are that 1) lookups by 
 * fields other than subject will be rare, and 2) the hash will not have
 * a large number of entries.  These assumptions will be tested.
 *
 * XXX
 * For NSS 3.4, it is worth consideration to do all forms of indexing,
 * because the only crypto context is global and persistent.
 */

struct nickname_template_str
{
    NSSUTF8 *nickname;
    nssList *subjectList;
};

static void match_nickname(const void *k, void *v, void *a)
{
    PRStatus nssrv;
    NSSCertificate *c;
    nssList *subjectList = (nssList *)v;
    struct nickname_template_str *nt = (struct nickname_template_str *)a;
    nssrv = nssList_GetArray(subjectList, (void **)&c, 1);
    if (nssrv == PR_SUCCESS && 
         nssUTF8_Equal(c->nickname, nt->nickname, &nssrv)) 
    {
	nt->subjectList = subjectList;
    }
}

/*
 * Find all cached certs with this label.
 */
NSS_IMPLEMENT NSSCertificate **
nssCertificateStore_FindCertificatesByNickname
(
  nssCertificateStore *store,
  NSSUTF8 *nickname,
  NSSCertificate *rvOpt[],
  PRUint32 maximumOpt,
  NSSArena *arenaOpt
)
{
    PRUint32 i, count;
    NSSCertificate **rvArray = NULL;
    struct nickname_template_str nt;
    nt.nickname = nickname;
    nt.subjectList = NULL;
    PZ_Lock(store->lock);
    nssHash_Iterate(store->subject, match_nickname, &nt);
    PZ_Unlock(store->lock);
    if (nt.subjectList) {
	count = nssList_Count(nt.subjectList);
	if (maximumOpt > 0) {
	    count = PR_MIN(maximumOpt, count);
	}
	if (rvOpt) {
	    nssList_GetArray(nt.subjectList, (void **)rvOpt, count);
	    for (i=0; i<count; i++) nssCertificate_AddRef(rvOpt[i]);
	} else {
	    rvArray = nss_ZNEWARRAY(arenaOpt, NSSCertificate *, count + 1);
	    if (!rvArray) {
		return (NSSCertificate **)NULL;
	    }
	    nssList_GetArray(nt.subjectList, (void **)rvArray, count);
	    for (i=0; i<count; i++) nssCertificate_AddRef(rvArray[i]);
	}
    }
    return rvArray;
}

struct email_template_str
{
    NSSASCII7 *email;
    nssList *emailList;
};

static void match_email(const void *k, void *v, void *a)
{
    PRStatus nssrv;
    NSSCertificate *c;
    nssList *subjectList = (nssList *)v;
    struct email_template_str *et = (struct email_template_str *)a;
    nssrv = nssList_GetArray(subjectList, (void **)&c, 1);
    if (nssrv == PR_SUCCESS && 
         nssUTF8_Equal(c->email, et->email, &nssrv)) 
    {
	nssListIterator *iter = nssList_CreateIterator(subjectList);
	if (iter) {
	    for (c  = (NSSCertificate *)nssListIterator_Start(iter);
	         c != (NSSCertificate *)NULL;
	         c  = (NSSCertificate *)nssListIterator_Next(iter))
	    {
		nssList_Add(et->emailList, c);
	    }
	    nssListIterator_Finish(iter);
	    nssListIterator_Destroy(iter);
	}
    }
}

/*
 * Find all cached certs with this email address.
 */
NSS_IMPLEMENT NSSCertificate **
nssCertificateStore_FindCertificatesByEmail
(
  nssCertificateStore *store,
  NSSASCII7 *email,
  NSSCertificate *rvOpt[],
  PRUint32 maximumOpt,
  NSSArena *arenaOpt
)
{
    PRUint32 i, count;
    NSSCertificate **rvArray = NULL;
    struct email_template_str et;
    et.email = email;
    et.emailList = nssList_Create(store->arena, PR_FALSE);
    if (!et.emailList) {
	return NULL;
    }
    PZ_Lock(store->lock);
    nssHash_Iterate(store->subject, match_email, &et);
    PZ_Unlock(store->lock);
    if (et.emailList) {
	count = nssList_Count(et.emailList);
	if (maximumOpt > 0) {
	    count = PR_MIN(maximumOpt, count);
	}
	if (rvOpt) {
	    nssList_GetArray(et.emailList, (void **)rvOpt, count);
	    for (i=0; i<count; i++) nssCertificate_AddRef(rvOpt[i]);
	} else {
	    rvArray = nss_ZNEWARRAY(arenaOpt, NSSCertificate *, count + 1);
	    if (!rvArray) {
		return (NSSCertificate **)NULL;
	    }
	    nssList_GetArray(et.emailList, (void **)rvArray, count);
	    for (i=0; i<count; i++) nssCertificate_AddRef(rvArray[i]);
	}
    }
    return rvArray;
}

NSS_IMPLEMENT NSSCertificate *
nssCertificateStore_FindCertificateByIssuerAndSerialNumber
(
  nssCertificateStore *store,
  NSSDER *issuer,
  NSSDER *serial
)
{
    certificate_hash_entry *entry;
    NSSCertificate index;
    index.issuer = *issuer;
    index.serial = *serial;
    PZ_Lock(store->lock);
    entry = (certificate_hash_entry *)
                           nssHash_Lookup(store->issuer_and_serial, &index);
    PZ_Unlock(store->lock);
    if (entry) {
	return nssCertificate_AddRef(entry->cert);
    }
    return NULL;
}

/* XXX Get this to use issuer/serial! */

struct der_template_str
{
    NSSDER *encoding;
    NSSCertificate *cert;
};

static void match_encoding(const void *k, void *v, void *a)
{
    PRStatus nssrv;
    NSSCertificate *c;
    nssList *subjectList = (nssList *)v;
    struct der_template_str *der = (struct der_template_str *)a;
    nssListIterator *iter = nssList_CreateIterator(subjectList);
    if (iter) {
	for (c  = (NSSCertificate *)nssListIterator_Start(iter);
	     c != (NSSCertificate *)NULL;
	     c  = (NSSCertificate *)nssListIterator_Next(iter))
	{
	    if (nssItem_Equal(&c->encoding, der->encoding, &nssrv)) {
		der->cert = c;
	    }
	}
	nssListIterator_Finish(iter);
	nssListIterator_Destroy(iter);
    }
}

NSS_IMPLEMENT NSSCertificate *
nssCertificateStore_FindCertificateByEncodedCertificate
(
  nssCertificateStore *store,
  NSSDER *encoding
)
{
    struct der_template_str der;
    der.encoding = encoding;
    der.cert = NULL;
    PZ_Lock(store->lock);
    nssHash_Iterate(store->subject, match_encoding, &der);
    PZ_Unlock(store->lock);
    return nssCertificate_AddRef(der.cert);
}

NSS_EXTERN PRStatus
nssCertificateStore_AddTrust
(
  nssCertificateStore *store,
  NSSTrust *trust
)
{
    NSSCertificate *cert;
    certificate_hash_entry *entry;
    cert = trust->certificate;
    PZ_Lock(store->lock);
    entry = (certificate_hash_entry *)
                              nssHash_Lookup(store->issuer_and_serial, cert);
    if (entry) {
	entry->trust = trust;
    }
    PZ_Unlock(store->lock);
    return (entry) ? PR_SUCCESS : PR_FAILURE;
}

NSS_IMPLEMENT NSSTrust *
nssCertificateStore_FindTrustForCertificate
(
  nssCertificateStore *store,
  NSSCertificate *cert
)
{
    certificate_hash_entry *entry;
    PZ_Lock(store->lock);
    entry = (certificate_hash_entry *)
                              nssHash_Lookup(store->issuer_and_serial, cert);
    PZ_Unlock(store->lock);
    if (entry) {
	return entry->trust;
    }
    return NULL;
}

NSS_EXTERN PRStatus
nssCertificateStore_AddSMIMEProfile
(
  nssCertificateStore *store,
  nssSMIMEProfile *profile
)
{
    NSSCertificate *cert;
    certificate_hash_entry *entry;
    cert = profile->certificate;
    PZ_Lock(store->lock);
    entry = (certificate_hash_entry *)
                              nssHash_Lookup(store->issuer_and_serial, cert);
    if (entry) {
	entry->profile = profile;
    }
    PZ_Unlock(store->lock);
    return (entry) ? PR_SUCCESS : PR_FAILURE;
}

NSS_IMPLEMENT nssSMIMEProfile *
nssCertificateStore_FindSMIMEProfileForCertificate
(
  nssCertificateStore *store,
  NSSCertificate *cert
)
{
    certificate_hash_entry *entry;
    PZ_Lock(store->lock);
    entry = (certificate_hash_entry *)
                              nssHash_Lookup(store->issuer_and_serial, cert);
    PZ_Unlock(store->lock);
    if (entry) {
	return entry->profile;
    }
    return NULL;
}

/* XXX this is also used by cache and should be somewhere else */

static PLHashNumber
nss_certificate_hash
(
  const void *key
)
{
    int i;
    PLHashNumber h;
    NSSCertificate *c = (NSSCertificate *)key;
    h = 0;
    for (i=0; i<c->issuer.size; i++)
	h = (h >> 28) ^ (h << 4) ^ ((unsigned char *)c->issuer.data)[i];
    for (i=0; i<c->serial.size; i++)
	h = (h >> 28) ^ (h << 4) ^ ((unsigned char *)c->serial.data)[i];
    return h;
}

static int
nss_compare_certs(const void *v1, const void *v2)
{
    PRStatus ignore;
    NSSCertificate *c1 = (NSSCertificate *)v1;
    NSSCertificate *c2 = (NSSCertificate *)v2;
    return (int)(nssItem_Equal(&c1->issuer, &c2->issuer, &ignore) &&
                 nssItem_Equal(&c1->serial, &c2->serial, &ignore));
}

NSS_IMPLEMENT nssHash *
nssHash_CreateCertificate
(
  NSSArena *arenaOpt,
  PRUint32 numBuckets
)
{
    return nssHash_Create(arenaOpt, 
                          numBuckets, 
                          nss_certificate_hash, 
                          nss_compare_certs, 
                          PL_CompareValues);
}

