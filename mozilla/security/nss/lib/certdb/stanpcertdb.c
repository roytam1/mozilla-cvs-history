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

#include "prtime.h"

#include "cert.h"
#include "mcom_db.h"
#include "certdb.h"
#include "secitem.h"
#include "secder.h"

/* Call to PK11_FreeSlot below */

#include "secasn1.h"
#include "secerr.h"
#include "nssilock.h"
#include "prmon.h"
#include "nsslocks.h"
#include "base64.h"
#include "sechash.h"
#include "plhash.h"
#include "pk11func.h" /* sigh */

#ifndef NSS_3_4_CODE
#define NSS_3_4_CODE
#endif /* NSS_3_4_CODE */
#include "nsspki.h"
#include "pki.h"
#include "pkim.h"
#include "pki3hack.h"
#include "ckhelper.h"
#include "base.h"
#include "pkistore.h"
#include "dev3hack.h"
#include "dev.h"

PRBool
SEC_CertNicknameConflict(char *nickname, SECItem *derSubject,
			 CERTCertDBHandle *handle)
{
    CERTCertificate *cert;
    PRBool conflict = PR_FALSE;

    cert=CERT_FindCertByNickname(handle, nickname);

    if (!cert) {
	return conflict;
    }

    conflict = !SECITEM_ItemsAreEqual(derSubject,&cert->derSubject);
    CERT_DestroyCertificate(cert);
    return conflict;
}

SECStatus
SEC_DeletePermCertificate(CERTCertificate *cert)
{
    PRStatus nssrv;
    NSSCertificate *c = STAN_GetNSSCertificate(cert);
    nssrv = NSSCertificate_DeleteStoredObject(c, NULL);
    return (nssrv == PR_SUCCESS) ? SECSuccess : SECFailure;
}

SECStatus
CERT_GetCertTrust(CERTCertificate *cert, CERTCertTrust *trust)
{
    SECStatus rv;
    CERT_LockCertTrust(cert);
    if ( cert->trust == NULL ) {
	rv = SECFailure;
    } else {
	*trust = *cert->trust;
	rv = SECSuccess;
    }
    CERT_UnlockCertTrust(cert);
    return(rv);
}

#ifdef notdef
static char *
cert_parseNickname(char *nickname)
{
    char *cp;
    for (cp=nickname; *cp && *cp != ':'; cp++);
    if (*cp == ':') return cp+1;
    return nickname;
}
#endif

SECStatus
CERT_ChangeCertTrust(CERTCertDBHandle *handle, CERTCertificate *cert,
		    CERTCertTrust *trust)
{
    SECStatus rv = SECFailure;
    PRStatus ret;

    CERT_LockCertTrust(cert);
    ret = STAN_ChangeCertTrust(cert, trust);
    rv = (ret == PR_SUCCESS) ? SECSuccess : SECFailure;
    CERT_UnlockCertTrust(cert);
    return rv;
}

SECStatus
__CERT_AddTempCertToPerm(CERTCertificate *cert, char *nickname,
		       CERTCertTrust *trust)
{
    NSSUTF8 *stanNick;
    PK11SlotInfo *slot;
    NSSToken *internal;
    NSSCryptoContext *context;
    nssCryptokiObject *permInstance;
    NSSCertificate *c = STAN_GetNSSCertificate(cert);
    context = c->object.cryptoContext;
    if (!context) {
	return SECFailure; /* wasn't a temp cert */
    }
    stanNick = nssCertificate_GetNickname(c, NULL);
    if (stanNick && nickname && strcmp(nickname, stanNick) != 0) {
	/* take the new nickname */
	cert->nickname = NULL;
	stanNick = NULL;
    }
    if (!stanNick && nickname) {
	stanNick = nssUTF8_Duplicate((NSSUTF8 *)nickname, c->object.arena);
    }
    /* Delete the temp instance */
    nssCertificateStore_Remove(context->certStore, c);
    c->object.cryptoContext = NULL;
    /* Import the perm instance onto the internal token */
    slot = PK11_GetInternalKeySlot();
    internal = PK11Slot_GetNSSToken(slot);
    permInstance = nssToken_ImportCertificate(internal, NULL,
                                              NSSCertificateType_PKIX,
                                              &c->id,
                                              stanNick,
                                              &c->encoding,
                                              &c->issuer,
                                              &c->subject,
                                              &c->serial,
                                              PR_TRUE);
    PK11_FreeSlot(slot);
    if (!permInstance) {
	return SECFailure;
    }
    nssPKIObject_AddInstance(&c->object, permInstance);
    nssTrustDomain_AddCertsToCache(STAN_GetDefaultTrustDomain(), &c, 1);
    /* reset the CERTCertificate fields */
    cert->nssCertificate = NULL;
    cert = STAN_GetCERTCertificate(c); /* will return same pointer */
    cert->istemp = PR_FALSE;
    cert->isperm = PR_TRUE;
    if (!trust) {
	return PR_SUCCESS;
    }
    return (STAN_ChangeCertTrust(cert, trust) == PR_SUCCESS) ? 
							SECSuccess: SECFailure;
}

SECStatus
CERT_AddTempCertToPerm(CERTCertificate *cert, char *nickname,
		       CERTCertTrust *trust)
{
    return __CERT_AddTempCertToPerm(cert, nickname, trust);
}

CERTCertificate *
__CERT_NewTempCertificate(CERTCertDBHandle *handle, SECItem *derCert,
			  char *nickname, PRBool isperm, PRBool copyDER)
{
    PRStatus nssrv;
    NSSCertificate *c;
    CERTCertificate *cc;
    NSSCertificate *tempCert;
    nssPKIObject *pkio;
    NSSCryptoContext *gCC = STAN_GetDefaultCryptoContext();
    NSSTrustDomain *gTD = STAN_GetDefaultTrustDomain();
    if (!isperm) {
	NSSDER encoding;
	NSSITEM_FROM_SECITEM(&encoding, derCert);
	/* First, see if it is already a temp cert */
	c = NSSCryptoContext_FindCertificateByEncodedCertificate(gCC, 
	                                                       &encoding);
	if (!c) {
	    /* Then, see if it is already a perm cert */
	    c = NSSTrustDomain_FindCertificateByEncodedCertificate(handle, 
	                                                           &encoding);
	}
	if (c) {
	    return STAN_GetCERTCertificate(c);
	}
    }
    pkio = nssPKIObject_Create(NULL, NULL, gTD, gCC);
    if (!pkio) {
	return NULL;
    }
    c = nss_ZNEW(pkio->arena, NSSCertificate);
    if (!c) {
	nssPKIObject_Destroy(pkio);
	return NULL;
    }
    c->object = *pkio;
    NSSITEM_FROM_SECITEM(&c->encoding, derCert);
    /* Forces a decoding of the cert in order to obtain the parts used
     * below
     */
    cc = STAN_GetCERTCertificate(c);
    nssItem_Create(c->object.arena, 
                   &c->issuer, cc->derIssuer.len, cc->derIssuer.data);
    nssItem_Create(c->object.arena, 
                   &c->subject, cc->derSubject.len, cc->derSubject.data);
    if (PR_TRUE) {
	/* CERTCertificate stores serial numbers decoded.  I need the DER
	* here.  sigh.
	*/
	SECItem derSerial = { 0 };
	CERT_SerialNumberFromDERCert(&cc->derCert, &derSerial);
	if (!derSerial.data) goto loser;
	nssItem_Create(c->object.arena, &c->serial, derSerial.len, derSerial.data);
	PORT_Free(derSerial.data);
    }
    if (nickname) {
	c->object.tempName = nssUTF8_Create(c->object.arena, 
                                            nssStringType_UTF8String, 
                                            (NSSUTF8 *)nickname, 
                                            PORT_Strlen(nickname));
    }
    if (cc->emailAddr) {
	c->email = nssUTF8_Create(c->object.arena, 
	                          nssStringType_PrintableString, 
	                          (NSSUTF8 *)cc->emailAddr, 
	                          PORT_Strlen(cc->emailAddr));
    }
    /* this function cannot detect if the cert exists as a temp cert now, but
     * didn't when CERT_NewTemp was first called.
     */
    nssrv = NSSCryptoContext_ImportCertificate(gCC, c);
    if (nssrv != PR_SUCCESS) {
	goto loser;
    }
    /* so find the entry in the temp store */
    tempCert = NSSCryptoContext_FindCertificateByIssuerAndSerialNumber(gCC,
                                                                   &c->issuer,
                                                                   &c->serial);
    /* destroy the copy */
    NSSCertificate_Destroy(c);
    if (tempCert) {
	/* and use the "official" entry */
	c = tempCert;
	cc = STAN_GetCERTCertificate(c);
    } else {
	return NULL;
    }
    cc->istemp = PR_TRUE;
    cc->isperm = PR_FALSE;
    return cc;
loser:
    nssPKIObject_Destroy(&c->object);
    return NULL;
}

CERTCertificate *
CERT_NewTempCertificate(CERTCertDBHandle *handle, SECItem *derCert,
			char *nickname, PRBool isperm, PRBool copyDER)
{
    return( __CERT_NewTempCertificate(handle, derCert, nickname,
                                      isperm, copyDER) );
}

/* maybe all the wincx's should be some const for internal token login? */
CERTCertificate *
CERT_FindCertByIssuerAndSN(CERTCertDBHandle *handle, CERTIssuerAndSN *issuerAndSN)
{
    PK11SlotInfo *slot;
    CERTCertificate *cert;

    cert = PK11_FindCertByIssuerAndSN(&slot,issuerAndSN,NULL);
    if (cert && slot) {
        PK11_FreeSlot(slot);
    }

    return cert;
}

static NSSCertificate *
get_best_temp_or_perm(NSSCertificate *ct, NSSCertificate *cp)
{
    NSSUsage usage;
    NSSCertificate *arr[3];
    if (!ct) {
	return nssCertificate_AddRef(cp);
    } else if (!cp) {
	return nssCertificate_AddRef(ct);
    }
    arr[0] = ct;
    arr[1] = cp;
    arr[2] = NULL;
    usage.anyUsage = PR_TRUE;
    return nssCertificateArray_FindBestCertificate(arr, NULL, &usage, NULL);
}

CERTCertificate *
CERT_FindCertByName(CERTCertDBHandle *handle, SECItem *name)
{
    NSSCertificate *cp, *ct, *c;
    NSSDER subject;
    NSSUsage usage;
    NSSCryptoContext *cc;
    NSSITEM_FROM_SECITEM(&subject, name);
    usage.anyUsage = PR_TRUE;
    cc = STAN_GetDefaultCryptoContext();
    ct = NSSCryptoContext_FindBestCertificateBySubject(cc, &subject, 
                                                       NULL, &usage, NULL);
    cp = NSSTrustDomain_FindBestCertificateBySubject(handle, &subject, 
                                                     NULL, &usage, NULL);
    c = get_best_temp_or_perm(ct, cp);
    if (ct) {
	CERTCertificate *cert = STAN_GetCERTCertificate(ct);
	CERT_DestroyCertificate(cert);
    }
    if (cp) {
	CERTCertificate *cert = STAN_GetCERTCertificate(cp);
	CERT_DestroyCertificate(cert);
    }
    if (c) {
	return STAN_GetCERTCertificate(c);
    } else {
	return NULL;
    }
}

CERTCertificate *
CERT_FindCertByKeyID(CERTCertDBHandle *handle, SECItem *name, SECItem *keyID)
{
   CERTCertList *list =
                        CERT_CreateSubjectCertList(NULL,handle,name,0,PR_FALSE);
    CERTCertificate *cert = NULL;
    CERTCertListNode *node = CERT_LIST_HEAD(list);

    if (list == NULL) return NULL;

    for (node = CERT_LIST_HEAD(list); node ; node = CERT_LIST_NEXT(node)) {
        if (SECITEM_ItemsAreEqual(&cert->subjectKeyID, keyID) ) {
            cert = CERT_DupCertificate(node->cert);
            break;
        }
    }
    return cert;
}

CERTCertificate *
CERT_FindCertByNickname(CERTCertDBHandle *handle, char *nickname)
{
    NSSCryptoContext *cc;
    NSSCertificate *c, *ct;
    CERTCertificate *cert;
    NSSUsage usage;
    usage.anyUsage = PR_TRUE;
    cc = STAN_GetDefaultCryptoContext();
    ct = NSSCryptoContext_FindBestCertificateByNickname(cc, nickname, 
                                                       NULL, &usage, NULL);
    cert = PK11_FindCertFromNickname(nickname, NULL);
    c = NULL;
    if (cert) {
	c = get_best_temp_or_perm(ct, STAN_GetNSSCertificate(cert));
	CERT_DestroyCertificate(cert);
	if (ct) {
	    CERTCertificate *cert2 = STAN_GetCERTCertificate(ct);
	    CERT_DestroyCertificate(cert2);
	}
    } else {
	c = ct;
    }
    if (c) {
	return STAN_GetCERTCertificate(c);
    } else {
	return NULL;
    }
}

CERTCertificate *
CERT_FindCertByDERCert(CERTCertDBHandle *handle, SECItem *derCert)
{
    NSSCryptoContext *cc;
    NSSCertificate *c;
    NSSDER encoding;
    NSSITEM_FROM_SECITEM(&encoding, derCert);
    cc = STAN_GetDefaultCryptoContext();
    c = NSSCryptoContext_FindCertificateByEncodedCertificate(cc, &encoding);
    if (!c) {
	c = NSSTrustDomain_FindCertificateByEncodedCertificate(handle, 
	                                                       &encoding);
	if (!c) return NULL;
    }
    return STAN_GetCERTCertificate(c);
}

CERTCertificate *
CERT_FindCertByNicknameOrEmailAddr(CERTCertDBHandle *handle, char *name)
{
    NSSCryptoContext *cc;
    NSSCertificate *c, *ct;
    CERTCertificate *cert;
    NSSUsage usage;
    usage.anyUsage = PR_TRUE;
    cc = STAN_GetDefaultCryptoContext();
    ct = NSSCryptoContext_FindBestCertificateByNickname(cc, name, 
                                                       NULL, &usage, NULL);
    if (!ct) {
	ct = NSSCryptoContext_FindBestCertificateByEmail(cc, name, 
	                                                NULL, &usage, NULL);
    }
    cert = PK11_FindCertFromNickname(name, NULL);
    if (cert) {
	c = get_best_temp_or_perm(ct, STAN_GetNSSCertificate(cert));
	CERT_DestroyCertificate(cert);
	if (ct) {
	    CERTCertificate *cert2 = STAN_GetCERTCertificate(ct);
	    CERT_DestroyCertificate(cert2);
	}
    } else {
	c = ct;
    }
    if (c) {
	return STAN_GetCERTCertificate(c);
    }
    return NULL;
}

static void 
add_to_subject_list(CERTCertList *certList, CERTCertificate *cert,
                    PRBool validOnly, int64 sorttime)
{
    SECStatus secrv;
    if (!validOnly ||
	CERT_CheckCertValidTimes(cert, sorttime, PR_FALSE) 
	 == secCertTimeValid) {
	    secrv = CERT_AddCertToListSorted(certList, cert, 
	                                     CERT_SortCBValidity, 
	                                     (void *)&sorttime);
	    if (secrv != SECSuccess) {
		CERT_DestroyCertificate(cert);
	    }
    } else {
	CERT_DestroyCertificate(cert);
    }
}

CERTCertList *
CERT_CreateSubjectCertList(CERTCertList *certList, CERTCertDBHandle *handle,
			   SECItem *name, int64 sorttime, PRBool validOnly)
{
    NSSCryptoContext *cc;
    NSSCertificate **tSubjectCerts, **pSubjectCerts;
    NSSCertificate **ci;
    CERTCertificate *cert;
    NSSDER subject;
    PRBool myList = PR_FALSE;
    cc = STAN_GetDefaultCryptoContext();
    NSSITEM_FROM_SECITEM(&subject, name);
    /* Collect both temp and perm certs for the subject */
    tSubjectCerts = NSSCryptoContext_FindCertificatesBySubject(cc,
                                                               &subject,
                                                               NULL,
                                                               0,
                                                               NULL);
    pSubjectCerts = NSSTrustDomain_FindCertificatesBySubject(handle,
                                                             &subject,
                                                             NULL,
                                                             0,
                                                             NULL);
    if (!tSubjectCerts && !pSubjectCerts) {
	return NULL;
    }
    if (certList == NULL) {
	certList = CERT_NewCertList();
	myList = PR_TRUE;
	if (!certList) goto loser;
    }
    /* Iterate over the matching temp certs.  Add them to the list */
    ci = tSubjectCerts;
    while (ci && *ci) {
	cert = STAN_GetCERTCertificate(*ci);
	add_to_subject_list(certList, cert, validOnly, sorttime);
	ci++;
    }
    /* Iterate over the matching perm certs.  Add them to the list */
    ci = pSubjectCerts;
    while (ci && *ci) {
	cert = STAN_GetCERTCertificate(*ci);
	add_to_subject_list(certList, cert, validOnly, sorttime);
	ci++;
    }
    nss_ZFreeIf(tSubjectCerts);
    nss_ZFreeIf(pSubjectCerts);
    return certList;
loser:
    nss_ZFreeIf(tSubjectCerts);
    nss_ZFreeIf(pSubjectCerts);
    if (myList && certList != NULL) {
	CERT_DestroyCertList(certList);
    }
    return NULL;
}

void
CERT_DestroyCertificate(CERTCertificate *cert)
{
    int refCount;
    CERTCertDBHandle *handle;
    if ( cert ) {
	NSSCertificate *tmp = STAN_GetNSSCertificate(cert);
	handle = cert->dbhandle;
#ifdef NSS_CLASSIC
        CERT_LockCertRefCount(cert);
	PORT_Assert(cert->referenceCount > 0);
	refCount = --cert->referenceCount;
        CERT_UnlockCertRefCount(cert);
	if ( ( refCount == 0 ) && !cert->keepSession ) {
	    PRArenaPool *arena  = cert->arena;
	    /* zero cert before freeing. Any stale references to this cert
	     * after this point will probably cause an exception.  */
	    PORT_Memset(cert, 0, sizeof *cert);
	    cert = NULL;
	    /* free the arena that contains the cert. */
	    PORT_FreeArena(arena, PR_FALSE);
        }
#else
	if (tmp) {
	    NSSTrustDomain *td = STAN_GetDefaultTrustDomain();
	    refCount = (int)tmp->object.refCount;
	    /* This is a hack.  For 3.4, there are persistent references
	     * to 4.0 certificates during the lifetime of a cert.  In the
	     * case of a temp cert, the persistent reference is in the
	     * cert store of the global crypto context.  For a perm cert,
	     * the persistent reference is in the cache.  Thus, the last
	     * external reference is really the penultimate NSS reference.
	     * When the count drops to two, it is really one, but the
	     * persistent reference must be explicitly deleted.  In 4.0,
	     * this ugliness will not appear.  Crypto contexts will remove
	     * their own cert references, and the cache will have its
	     * own management code also.
	     */
	    if (refCount == 2) {
		NSSCryptoContext *cc = tmp->object.cryptoContext;
		if (cc != NULL) {
		    nssCertificateStore_Remove(cc->certStore, tmp);
		} else {
		    nssTrustDomain_RemoveCertFromCache(td, tmp);
		}
	    }
	    /* delete the NSSCertificate */
	    NSSCertificate_Destroy(tmp);
	} 
#endif
    }
    return;
}

#ifdef notdef
SECStatus
CERT_ChangeCertTrustByUsage(CERTCertDBHandle *certdb,
			    CERTCertificate *cert, SECCertUsage usage)
{
    SECStatus rv;
    CERTCertTrust trust;
    CERTCertTrust tmptrust;
    unsigned int certtype;
    PRBool saveit;
    
    saveit = PR_TRUE;
    
    PORT_Memset((void *)&trust, 0, sizeof(trust));

    certtype = cert->nsCertType;

    /* if no app bits in cert type, then set all app bits */
    if ( ! ( certtype & NS_CERT_TYPE_APP ) ) {
	certtype |= NS_CERT_TYPE_APP;
    }

    switch ( usage ) {
      case certUsageEmailSigner:
      case certUsageEmailRecipient:
	if ( certtype & NS_CERT_TYPE_EMAIL ) {
	     trust.emailFlags = CERTDB_VALID_PEER;
	     if ( ! ( cert->rawKeyUsage & KU_KEY_ENCIPHERMENT ) ) {
		/* don't save it if KeyEncipherment is not allowed */
		saveit = PR_FALSE;
	    }
	}
	break;
      case certUsageUserCertImport:
	if ( certtype & NS_CERT_TYPE_EMAIL ) {
	    trust.emailFlags = CERTDB_VALID_PEER;
	}
	/* VALID_USER is already set if the cert was imported, 
	 * in the case that the cert was already in the database
	 * through SMIME or other means, we should set the USER
	 * flags, if they are not already set.
	 */
	if( cert->isperm ) {
	    if ( certtype & NS_CERT_TYPE_SSL_CLIENT ) {
		if( !(cert->trust->sslFlags & CERTDB_USER) ) {
		    trust.sslFlags |= CERTDB_USER;
		}
	    }
	    
	    if ( certtype & NS_CERT_TYPE_EMAIL ) {
		if( !(cert->trust->emailFlags & CERTDB_USER) ) {
		    trust.emailFlags |= CERTDB_USER;
		}
	    }
	    
	    if ( certtype & NS_CERT_TYPE_OBJECT_SIGNING ) {
		if( !(cert->trust->objectSigningFlags & CERTDB_USER) ) {
		    trust.objectSigningFlags |= CERTDB_USER;
		}
	    }
	}
	break;
      default:	/* XXX added to quiet warnings; no other cases needed? */
	break;
    }

    if ( (trust.sslFlags | trust.emailFlags | trust.objectSigningFlags) == 0 ){
	saveit = PR_FALSE;
    }

    if ( saveit && cert->isperm ) {
	/* Cert already in the DB.  Just adjust flags */
	tmptrust = *cert->trust;
	tmptrust.sslFlags |= trust.sslFlags;
	tmptrust.emailFlags |= trust.emailFlags;
	tmptrust.objectSigningFlags |= trust.objectSigningFlags;
	    
	rv = CERT_ChangeCertTrust(cert->dbhandle, cert,
				  &tmptrust);
	if ( rv != SECSuccess ) {
	    goto loser;
	}
    }

    rv = SECSuccess;
    goto done;

loser:
    rv = SECFailure;
done:

    return(rv);
}
#endif

int
CERT_GetDBContentVersion(CERTCertDBHandle *handle)
{
    /* should read the DB content version from the pkcs #11 device */
    return 0;
}

/*
 *
 * Manage S/MIME profiles
 *
 */

SECStatus
CERT_SaveSMimeProfile(CERTCertificate *cert, SECItem *emailProfile,
		      SECItem *profileTime)
{
    int64 oldtime;
    int64 newtime;
    SECStatus rv = SECFailure;
    PRBool saveit;
    char *emailAddr;
    SECItem oldprof, oldproftime;
    SECItem *oldProfile = NULL;
    SECItem *oldProfileTime = NULL;
    PK11SlotInfo *slot = NULL;
    NSSCertificate *c;
    NSSCryptoContext *cc;
    nssSMIMEProfile *stanProfile = NULL;
    PRBool freeOldProfile = PR_FALSE;
    
    emailAddr = cert->emailAddr;
    
    if ( emailAddr == NULL ) {
	goto loser;
    }

    c = STAN_GetNSSCertificate(cert);
    if (!c) return SECFailure;
    cc = c->object.cryptoContext;
    if (cc != NULL) {
	stanProfile = nssCryptoContext_FindSMIMEProfileForCertificate(cc, c);
	if (stanProfile) {
	    PORT_Assert(stanProfile->profileData);
	    SECITEM_FROM_NSSITEM(&oldprof, stanProfile->profileData);
	    oldProfile = &oldprof;
	    SECITEM_FROM_NSSITEM(&oldproftime, stanProfile->profileTime);
	    oldProfileTime = &oldproftime;
	}
    } else {
	oldProfile = PK11_FindSMimeProfile(&slot, emailAddr, &cert->derSubject, 
							&oldProfileTime); 
	freeOldProfile = PR_TRUE;
    }

    saveit = PR_FALSE;
    
    /* both profileTime and emailProfile have to exist or not exist */
    if ( emailProfile == NULL ) {
	profileTime = NULL;
    } else if ( profileTime == NULL ) {
	emailProfile = NULL;
    }
   
    if ( oldProfileTime == NULL ) {
	saveit = PR_TRUE;
    } else {
	/* there was already a profile for this email addr */
	if ( profileTime ) {
	    /* we have an old and new profile - save whichever is more recent*/
	    if ( oldProfileTime->len == 0 ) {
		/* always replace if old entry doesn't have a time */
		oldtime = LL_MININT;
	    } else {
		rv = DER_UTCTimeToTime(&oldtime, oldProfileTime);
		if ( rv != SECSuccess ) {
		    goto loser;
		}
	    }
	    
	    rv = DER_UTCTimeToTime(&newtime, profileTime);
	    if ( rv != SECSuccess ) {
		goto loser;
	    }
	
	    if ( LL_CMP(newtime, >, oldtime ) ) {
		/* this is a newer profile, save it and cert */
		saveit = PR_TRUE;
	    }
	} else {
	    saveit = PR_TRUE;
	}
    }


    if (saveit) {
	if (cc) {
	    if (stanProfile) {
		/* stanProfile is already stored in the crypto context,
		 * overwrite the data
		 */
		NSSArena *arena = stanProfile->object.arena;
		stanProfile->profileTime = nssItem_Create(arena, 
		                                          NULL,
		                                          profileTime->len,
		                                          profileTime->data);
		stanProfile->profileData = nssItem_Create(arena, 
		                                          NULL,
		                                          emailProfile->len,
		                                          emailProfile->data);
	    } else if (profileTime && emailProfile) {
		PRStatus nssrv;
		NSSDER subject;
		NSSItem profTime, profData;
		NSSItem *pprofTime, *pprofData;
		NSSITEM_FROM_SECITEM(&subject, &cert->derSubject);
		if (profileTime) {
		    NSSITEM_FROM_SECITEM(&profTime, profileTime);
		    pprofTime = &profTime;
		} else {
		    pprofTime = NULL;
		}
		if (emailProfile) {
		    NSSITEM_FROM_SECITEM(&profData, emailProfile);
		    pprofData = &profData;
		} else {
		    pprofData = NULL;
		}
		stanProfile = nssSMIMEProfile_Create(c, pprofTime, pprofData);
		if (!stanProfile) goto loser;
		nssrv = nssCryptoContext_ImportSMIMEProfile(cc, stanProfile);
		rv = (nssrv == PR_SUCCESS) ? SECSuccess : SECFailure;
	    }
	} else {
	    rv = PK11_SaveSMimeProfile(slot, emailAddr, &cert->derSubject, 
						emailProfile, profileTime);
	}
    } else {
	rv = SECSuccess;
    }

loser:
    if (oldProfile && freeOldProfile) {
    	SECITEM_FreeItem(oldProfile,PR_TRUE);
    }
    if (oldProfileTime && freeOldProfile) {
    	SECITEM_FreeItem(oldProfileTime,PR_TRUE);
    }
    if (stanProfile) {
	nssSMIMEProfile_Destroy(stanProfile);
    }
    if (slot) {
	PK11_FreeSlot(slot);
    }
    
    return(rv);
}

SECItem *
CERT_FindSMimeProfile(CERTCertificate *cert)
{
    PK11SlotInfo *slot = NULL;
    NSSCertificate *c;
    NSSCryptoContext *cc;
    SECItem *rvItem = NULL;

    c = STAN_GetNSSCertificate(cert);
    if (!c) return NULL;
    cc = c->object.cryptoContext;
    if (cc != NULL) {
	nssSMIMEProfile *stanProfile;
	stanProfile = nssCryptoContext_FindSMIMEProfileForCertificate(cc, c);
	if (stanProfile) {
	    rvItem = SECITEM_AllocItem(NULL, NULL, 
	                               stanProfile->profileData->size);
	    if (rvItem) {
		rvItem->data = stanProfile->profileData->data;
	    }
	    nssSMIMEProfile_Destroy(stanProfile);
	}
	return rvItem;
    }
    rvItem =
	PK11_FindSMimeProfile(&slot, cert->emailAddr, &cert->derSubject, NULL);
    PK11_FreeSlot(slot);
    return rvItem;
}

/*
 * depricated functions that are now just stubs.
 */
/*
 * Close the database
 */
void
__CERT_ClosePermCertDB(CERTCertDBHandle *handle)
{
    PORT_Assert("CERT_ClosePermCertDB is Depricated" == NULL);
    return;
}

SECStatus
CERT_OpenCertDBFilename(CERTCertDBHandle *handle, char *certdbname,
                        PRBool readOnly)
{
    PORT_Assert("CERT_OpenCertDBFilename is Depricated" == NULL);
    return SECFailure;
}

SECItem *
SECKEY_HashPassword(char *pw, SECItem *salt)
{
    PORT_Assert("SECKEY_HashPassword is Depricated" == NULL);
    return NULL;
}

SECStatus
__CERT_TraversePermCertsForSubject(CERTCertDBHandle *handle,
                                 SECItem *derSubject,
                                 void *cb, void *cbarg)
{
    PORT_Assert("CERT_TraversePermCertsForSubject is Depricated" == NULL);
    return SECFailure;
}


SECStatus
__CERT_TraversePermCertsForNickname(CERTCertDBHandle *handle, char *nickname,
                                  void *cb, void *cbarg)
{
    PORT_Assert("CERT_TraversePermCertsForNickname is Depricated" == NULL);
    return SECFailure;
}



