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

#ifndef BASE_H
#include "base.h"
#endif /* BASE_H */

#ifndef DEV_H
#include "dev.h"
#endif /* DEV_H */

#ifndef PKIM_H
#include "pkim.h"
#endif /* PKIM_H */

#include "nsst.h"

typedef struct 
{
  void *data;
  NSSCertMethods *methods;
  PRBool haveValidity;
  NSSTime notBefore;
  NSSTime notAfter;
  PRBool haveUsages;
  NSSUsages usages;
  NSSPolicies *policies;
}
nssCertDecoding;

struct NSSCertStr
{
  nssPKIObject object;
  NSSCertType kind;
  NSSItem id;
  NSSBER encoding;
  NSSDER issuer;
  NSSDER subject;
  NSSDER serial;
  NSSASCII7 *email;
  NSSPublicKey *bk; /* for ephemeral decoded pubkeys */
  nssCertDecoding decoding;
};


NSS_EXTERN NSSCertMethods *
nss_GetMethodsForType (
  NSSCertType certType
);

/* Creates a certificate from a base object */
NSS_IMPLEMENT NSSCert *
nssCert_Create (
  nssPKIObject *object
)
{
    PRStatus status;
    NSSCert *rvCert;
    /* mark? */
    NSSArena *arena = object->arena;
    PR_ASSERT(object->instances != NULL && object->numInstances > 0);
    rvCert = nss_ZNEW(arena, NSSCert);
    if (!rvCert) {
	return (NSSCert *)NULL;
    }
    rvCert->object = *object;
    /* XXX should choose instance based on some criteria */
    status = nssCryptokiCert_GetAttributes(object->instances[0],
                                                  arena,
                                                  &rvCert->kind,
                                                  &rvCert->id,
                                                  &rvCert->encoding,
                                                  &rvCert->issuer,
                                                  &rvCert->serial,
                                                  &rvCert->subject,
                                                  &rvCert->email);
    if (status != PR_SUCCESS) {
	return (NSSCert *)NULL;
    }
    /* all certs need an encoding value */
    if (rvCert->encoding.data == NULL) {
	return (NSSCert *)NULL;
    }
    rvCert->decoding.methods = nss_GetMethodsForType(rvCert->kind);
    if (!rvCert->decoding.methods) {
	return (NSSCert *)NULL;
    }
    return rvCert;
}

NSS_IMPLEMENT NSSCert *
nssCert_Decode (
  NSSBER *ber
)
{
    NSSArena *arena;
    NSSCert *rvCert;
    NSSCertMethods *decoder;
    void *decoding;
    NSSItem *it;

    /* create the PKIObject */
    arena = nssArena_Create();
    if (!arena) {
	return (NSSCert *)NULL;
    }
    rvCert = nss_ZNEW(arena, NSSCert);
    if (!rvCert) {
	goto loser;
    }
    rvCert->object.arena = arena;
    rvCert->object.refCount = 1;
    rvCert->object.lock = PZ_NewLock(nssILockOther);
    if (!rvCert->object.lock) {
	goto loser;
    }
    /* try to decode it */
    decoder = nss_GetMethodsForType(NSSCertType_PKIX);
    if (!decoder) {
	/* nss_SetError(UNKNOWN_CERT_TYPE); */
	goto loser;
    }
    decoding = decoder->decode(arena, ber);
    if (decoding) {
	/* it's a PKIX cert */
	rvCert->decoding.methods = decoder;
	rvCert->decoding.data = decoding;
	rvCert->kind = NSSCertType_PKIX;
    } else {
	goto loser;
    }
    /* copy the BER encoding */
    it = nssItem_Duplicate(ber, arena, &rvCert->encoding);
    if (!it) {
	goto loser;
    }
    /* obtain the issuer from the decoding */
    it = decoder->getIssuer(decoding);
    if (!it) {
	goto loser;
    }
    rvCert->issuer = *it;
    /* obtain the serial number from the decoding */
    it = decoder->getSerialNumber(decoding);
    if (!it) {
	goto loser;
    }
    rvCert->serial = *it;
    /* obtain the subject from the decoding */
    it = decoder->getSubject(decoding);
    if (!it) {
	goto loser;
    }
    rvCert->subject = *it;
    /* obtain the email address from the decoding */
    rvCert->email = decoder->getEmailAddress(decoding);
    return rvCert;
loser:
    nssArena_Destroy(arena);
    return (NSSCert *)NULL;
}

/* XXX */
NSS_IMPLEMENT NSSCert *
nssCert_CreateIndexCert (
  NSSDER *issuer,
  NSSDER *serial
)
{
    NSSCert *c = nss_ZNEW(NULL, NSSCert);
    if (c) {
	c->issuer = *issuer;
	c->serial = *serial;
    }
    return c;
}

NSS_IMPLEMENT NSSCert *
nssCert_AddRef (
  NSSCert *c
)
{
    if (c) {
	nssPKIObject_AddRef(&c->object);
    }
    return c;
}

NSS_IMPLEMENT PRStatus
nssCert_Destroy (
  NSSCert *c
)
{
    PRBool destroyed;
    if (c) {
	void *dc = c->decoding.data;
	NSSCertMethods *methods = c->decoding.methods;
	destroyed = nssPKIObject_Destroy(&c->object);
	if (destroyed) {
	    if (dc) {
		methods->destroy(dc);
	    }
	}
    }
    return PR_SUCCESS;
}

NSS_IMPLEMENT PRStatus
NSSCert_Destroy (
  NSSCert *c
)
{
    return nssCert_Destroy(c);
}

NSS_IMPLEMENT PRUint32
nssCert_Hash (
  NSSCert *c
)
{
    PRUint32 i;
    PRUint32 h = 0;
    for (i=0; i<c->issuer.size; i++)
	h = (h >> 28) ^ (h << 4) ^ ((unsigned char *)c->issuer.data)[i];
    for (i=0; i<c->serial.size; i++)
	h = (h >> 28) ^ (h << 4) ^ ((unsigned char *)c->serial.data)[i];
    return h;
}

NSS_IMPLEMENT NSSDER *
nssCert_GetEncoding (
  NSSCert *c
)
{
    if (c->encoding.size > 0 && c->encoding.data) {
	return &c->encoding;
    } else {
	return (NSSDER *)NULL;
    }
}

NSS_IMPLEMENT NSSDER *
nssCert_GetIssuer (
  NSSCert *c
)
{
    if (c->issuer.size > 0 && c->issuer.data) {
	return &c->issuer;
    } else {
	return (NSSDER *)NULL;
    }
}

NSS_IMPLEMENT NSSDER *
NSSCert_GetIssuer (
  NSSCert *c
)
{
    return nssCert_GetIssuer(c);
}

NSS_IMPLEMENT NSSDER *
nssCert_GetSerialNumber (
  NSSCert *c
)
{
    if (c->serial.size > 0 && c->serial.data) {
	return &c->serial;
    } else {
	return (NSSDER *)NULL;
    }
}

NSS_IMPLEMENT NSSDER *
NSSCert_GetSerialNumber (
  NSSCert *c
)
{
    return nssCert_GetSerialNumber(c);
}

NSS_IMPLEMENT NSSDER *
nssCert_GetSubject (
  NSSCert *c
)
{
    if (c->subject.size > 0 && c->subject.data) {
	return &c->subject;
    } else {
	return (NSSDER *)NULL;
    }
}

NSS_IMPLEMENT PRStatus
nssCert_SetNickname (
  NSSCert *c,
  NSSToken *tokenOpt,
  NSSUTF8 *nickname
)
{
    return nssPKIObject_SetNickname(&c->object, tokenOpt, nickname);
}

NSS_IMPLEMENT NSSUTF8 *
nssCert_GetNickname (
  NSSCert *c,
  NSSToken *tokenOpt
)
{
    return nssPKIObject_GetNickname(&c->object, tokenOpt);
}

NSS_IMPLEMENT NSSToken *
nssCert_GetWriteToken (
  NSSCert *c,
  nssSession **rvSessionOpt
)
{
    return nssPKIObject_GetWriteToken(&c->object, rvSessionOpt);
}

NSS_IMPLEMENT NSSUTF8 *
NSSCert_GetNickname (
  NSSCert *c,
  NSSToken *tokenOpt
)
{
    return nssCert_GetNickname(c, tokenOpt);
}

NSS_IMPLEMENT NSSASCII7 *
nssCert_GetEmailAddress (
  NSSCert *c
)
{
    return c->email;
}

static nssCertDecoding *
nssCert_GetDecoding (
  NSSCert *c
)
{
    if (!c->decoding.data) {
	c->decoding.data = c->decoding.methods->decode(NULL, &c->encoding);
    }
    return &c->decoding;
}

NSS_IMPLEMENT void *
NSSCert_GetDecoding (
  NSSCert *c
)
{
    nssCertDecoding *dc;

    dc = nssCert_GetDecoding(c);
    if (dc) {
	return dc->data;
    }
    return (void *)NULL;
}

NSS_EXTERN NSSCertType
NSSCert_GetType (
  NSSCert *c
)
{
    return c->kind;
}

NSS_IMPLEMENT NSSUsages *
nssCert_GetUsages (
  NSSCert *c,
  PRStatus *statusOpt
)
{
    PRStatus status;
    nssCertDecoding *dc = nssCert_GetDecoding(c);
    if (statusOpt) *statusOpt = PR_SUCCESS;
    if (dc) {
	if (!dc->haveUsages) {
	    status = dc->methods->getUsages(dc->data, &dc->usages);
	    if (statusOpt) *statusOpt = status;
	}
    } else {
	if (statusOpt) *statusOpt = PR_FAILURE;
	return 0; /* XXX */
    }
    return &dc->usages;
}

static PRStatus
get_validity_period(nssCertDecoding *dc)
{
    if (!dc->haveValidity) {
	return dc->methods->getValidityPeriod(dc->data, 
	                                      &dc->notBefore, 
	                                      &dc->notAfter);
    }
    return PR_SUCCESS;
}

/* XXX */
NSS_IMPLEMENT PRBool
nssCert_IsValidAtTime (
  NSSCert *c,
  NSSTime time,
  PRStatus *statusOpt
)
{
    PRStatus status;
    nssCertDecoding *dc = nssCert_GetDecoding(c);
    if (statusOpt) *statusOpt = PR_FAILURE;
    if (dc) {
	status = get_validity_period(dc);
	if (status == PR_FAILURE) {
	    return PR_FALSE;
	}
	if (statusOpt) *statusOpt = PR_SUCCESS;
	if (nssTime_WithinRange(time, dc->notBefore, dc->notAfter)) {
	    return PR_TRUE;
	}
    }
    return PR_FALSE;
}

/* XXX */
/* note this isn't the same as CERT_IsNewer, but doesn't intend to be */
NSS_IMPLEMENT PRBool
nssCert_IsNewer (
  NSSCert *c1,
  NSSCert *c2,
  PRStatus *statusOpt
)
{
    nssCertDecoding *dc1 = nssCert_GetDecoding(c1);
    nssCertDecoding *dc2 = nssCert_GetDecoding(c2);
    if (statusOpt) *statusOpt = PR_SUCCESS;
    /* get the times from the decoding */
    if (get_validity_period(dc1) == PR_FAILURE) {
	if (statusOpt) *statusOpt = PR_FAILURE;
	return PR_FALSE;
    }
    if (get_validity_period(dc2) == PR_FAILURE) {
	if (statusOpt) *statusOpt = PR_FAILURE;
	return PR_FALSE;
    }
    return nssTime_IsAfter(dc1->notBefore, dc2->notBefore);
}

NSS_IMPLEMENT PRBool
nssCert_IssuerAndSerialEqual (
  NSSCert *c1,
  NSSCert *c2
)
{
    return (nssItem_Equal(&c1->issuer, &c2->issuer, NULL) &&
            nssItem_Equal(&c1->serial, &c2->serial, NULL));
}

NSS_IMPLEMENT void
nssCert_SetVolatileDomain (
  NSSCert *c,
  NSSVolatileDomain *vd
)
{
    c->object.vd = vd;
    c->object.td = nssVolatileDomain_GetTrustDomain(vd);
}

NSS_IMPLEMENT NSSVolatileDomain *
nssCert_GetVolatileDomain (
  NSSCert *c
)
{
    return c->object.vd;
}

NSS_IMPLEMENT NSSTrustDomain *
nssCert_GetTrustDomain (
  NSSCert *c
)
{
    return c->object.td;
}

NSS_IMPLEMENT NSSTrustDomain *
NSSCert_GetTrustDomain (
  NSSCert *c
)
{
    return nssCert_GetTrustDomain(c);
}

NSS_IMPLEMENT NSSToken **
nssCert_GetTokens (
  NSSCert *c,
  PRStatus *statusOpt
)
{
    return nssPKIObject_GetTokens(&c->object, statusOpt);
}

NSS_IMPLEMENT NSSToken **
NSSCert_GetTokens (
  NSSCert *c,
  PRStatus *statusOpt
)
{
    return nssCert_GetTokens(c, statusOpt);
}

NSS_IMPLEMENT NSSSlot *
NSSCert_GetSlot (
  NSSCert *c,
  PRStatus *statusOpt
)
{
    return (NSSSlot *)NULL;
}

NSS_IMPLEMENT NSSModule *
NSSCert_GetModule (
  NSSCert *c,
  PRStatus *statusOpt
)
{
    return (NSSModule *)NULL;
}

NSS_IMPLEMENT nssCryptokiObject *
nssCert_FindInstanceForAlgorithm (
  NSSCert *c,
  NSSAlgNParam *ap
)
{
    return nssPKIObject_FindInstanceForAlgorithm(&c->object, ap);
}

NSS_IMPLEMENT PRStatus
nssCert_DeleteStoredObject (
  NSSCert *c,
  NSSCallback *uhh
)
{
    return nssPKIObject_DeleteStoredObject(&c->object, uhh, PR_TRUE);
}

NSS_IMPLEMENT PRStatus
NSSCert_DeleteStoredObject (
  NSSCert *c,
  NSSCallback *uhh
)
{
    return nssCert_DeleteStoredObject(c, uhh);
}

NSS_IMPLEMENT PRStatus
nssCert_CopyToToken (
  NSSCert *c,
  NSSToken *token,
  NSSUTF8 *nicknameOpt
)
{
    PRStatus status;
    nssCryptokiObject *instance;
    nssSession *rwSession;

    rwSession = nssToken_CreateSession(token, PR_TRUE);
    if (!rwSession) {
	return PR_FAILURE;
    }
    instance = nssToken_ImportCert(token, rwSession,
                                          c->kind, NULL, nicknameOpt,
                                          &c->encoding, &c->issuer, 
                                          &c->subject, &c->serial,
                                          c->email, PR_TRUE);
    nssSession_Destroy(rwSession);
    if (!instance) {
	return PR_FAILURE;
    }
    status = nssPKIObject_AddInstance(&c->object, instance);
    if (status == PR_FAILURE) {
	return PR_FAILURE;
    }
    return PR_SUCCESS;
}

static NSSUsage
get_trusted_usage (
  NSSCert *c,
  PRBool asCA,
  PRStatus *status
)
{
    nssTrust *trust;
    nssTrustLevel checkLevel;
    NSSUsage usage = 0;

    *status = PR_SUCCESS;
    checkLevel = asCA ? nssTrustLevel_TrustedDelegator :
                        nssTrustLevel_Trusted;
    /* XXX needs to be cached with cert */
    trust = nssTrustDomain_FindTrustForCert(c->object.td, c);
    if (!trust) {
	if (NSS_GetError() == NSS_ERROR_NO_ERROR) {
	    *status = PR_SUCCESS;
	} else {
	    *status = PR_FAILURE;
	}
	return 0;
    }
    if (trust->clientAuth == checkLevel) {
	usage |= NSSUsage_SSLClient;
    }
    if (trust->serverAuth == checkLevel) {
	usage |= NSSUsage_SSLServer;
    }
    if (trust->emailProtection == checkLevel) {
	usage |= NSSUsage_EmailSigner | NSSUsage_EmailRecipient;
    }
    if (trust->codeSigning == checkLevel) {
	usage |= NSSUsage_CodeSigner;
    }
    nssTrust_Destroy(trust);
    /* XXX should check user cert */
    return usage;
}

static PRStatus
validate_and_discover_trust (
  NSSCert *c,
  NSSTime time,
  NSSUsage usage,
  NSSPolicies *policiesOpt,
  PRBool asCA,
  PRBool *trusted
)
{
    PRStatus status;
    NSSUsage trustedUsage;
    NSSUsages *certUsages;
    PRBool valid;

    *trusted = PR_FALSE;

    /* First verify the time is within the cert's validity period */
    if (!nssCert_IsValidAtTime(c, time, &status)) {
	if (status == PR_SUCCESS) {
	    /* The function was successful, so we own the error */
	    nss_SetError(NSS_ERROR_CERTIFICATE_NOT_VALID_AT_TIME);
	} /* else the function failed and owns the error */
	return PR_FAILURE;
    }

    /* See if the cert is trusted, overrides cert's usage */
    trustedUsage = get_trusted_usage(c, asCA, &status);
    if (trustedUsage && (trustedUsage & usage) == usage) {
	*trusted = PR_TRUE;
	return PR_SUCCESS;
    }

    /* Verify the cert is capable of the desired set of usages */
    certUsages = nssCert_GetUsages(c, &status);
    if (status == PR_FAILURE) {
	return PR_FAILURE;
    }
    if (asCA) {
	valid = ((certUsages->ca & usage) == usage);
    } else {
	valid = ((certUsages->peer & usage) == usage);
    }
    if (!valid) {
	nss_SetError(NSS_ERROR_CERTIFICATE_USAGE_INSUFFICIENT);
	return PR_FAILURE;
    }

    return status;
}

static PRStatus
validate_chain_link (
  NSSCert *subjectCert,
  NSSCert *issuerCert,
  void **vData
)
{
    PRStatus status;
    nssCertDecoding *dcs;

    dcs = nssCert_GetDecoding(subjectCert);
    if (!dcs) {
	return PR_FAILURE;
    }

    if (!*vData) {
	*vData = dcs->methods->startChainValidation();
#if 0
	if (!*vData) {
	    return PR_FAILURE;
	}
#endif
    }

    status = dcs->methods->validateChainLink(dcs->data, issuerCert, *vData);

#if 0
    if (*finished) {
	dcs->methods->freeChainValidationData(*vData);
	*vData = NULL;
    }
#endif
    return status;
}

#if 0
static PRBool
cert_in_chain_revoked (
  NSSCert **chain,
  PRStatus *status
)
{
    NSSCert **cp;
    nssCRL *crl;
    for (cp = chain; *cp; cp++) {
	crl = nssTrustDomain_FindCRLBySubject(td, subject);
	if (crl) {
	    status = nssCRL_FindCert(*cp);
	}
    }
    /* If OCSP is enabled, check revocation status of the cert */
    if (NSS_IsOCSPEnabled()) {
	nssOCSPResponder *responder = get_ocsp_responder(chain[0]);
	if (responder) {
	    status = nssOCSPResponder_CheckStatus(responder, chain[0]);
	}
    }
}
#endif

NSS_IMPLEMENT PRStatus
nssCert_Validate (
  NSSCert *c,
  NSSTime time,
  NSSUsages *usages,
  NSSPolicies *policiesOpt
)
{
    PRStatus status;
    PRBool asCA;
    PRBool trusted = PR_FALSE;
    PRBool atRoot = PR_FALSE;
    NSSCert **cp, **chain;
    NSSCert *subjectCert = NULL;
    NSSCert *issuerCert = NULL;
    NSSUsage usage;
    void *vData = NULL;

    /* Build the chain (this cert will be first) */
    chain = nssCert_BuildChain(c, time, usages, policiesOpt,
                                      NULL, 0, NULL, &status);
    if (status == PR_FAILURE) {
	return PR_FAILURE;
    }
    /* XXX restrict to ca || peer for now */
    if (usages->ca) {
	usage = usages->ca;
	asCA = PR_TRUE;
    } else {
	usage = usages->peer;
	asCA = PR_FALSE;
    }
    /* Validate the chain */
    subjectCert = chain[0];
    for (cp = chain + 1; !atRoot; cp++) {
	if (*cp) {
	    issuerCert = *cp;
	} else {
	    atRoot = PR_TRUE;
	}
	status = validate_and_discover_trust(subjectCert,
	                                     time, usage, policiesOpt,
	                                     asCA, &trusted);
	if (status == PR_FAILURE) {
	    goto done;
	}
	if (trusted) {
	    if (subjectCert == chain[0]) {
		/* The cert we are validating is explicitly trusted */
		goto done;
	    } else {
		/* Some cert in the chain is explicitly trusted, still
		 * need to check OCSP and/or CRL's
		 */
		goto check_revocation;
	    }
	}
	if (atRoot) {
	    break;
	}
	status = validate_chain_link(subjectCert, issuerCert, &vData);
	if (status == PR_FAILURE) {
	    goto done;
	}
	asCA = PR_TRUE;
	subjectCert = issuerCert;
    }

check_revocation:
    if (!trusted) {
	/* the last cert checked in the chain must be trusted */
	nss_SetError(NSS_ERROR_CERTIFICATE_HAS_NO_TRUSTED_ISSUER);
	status = PR_FAILURE;
    }
#if 0
    if (cert_in_chain_revoked(chain, &status)) {
	if (status == PR_SUCCESS) {
	    /* The status check succeeded, set the error */
	    nss_SetError(NSS_ERROR_CERTIFICATE_REVOKED);
	}
    }
#endif

done:
    nssCertArray_Destroy(chain);
    return status;
}

NSS_IMPLEMENT PRStatus
NSSCert_Validate (
  NSSCert *c,
  NSSTime time,
  NSSUsages *usages,
  NSSPolicies *policiesOpt
)
{
    return nssCert_Validate(c, time, usages, policiesOpt);
}

#if 0
struct NSSValidationErrorStr
{
  NSSCert *c;
  NSSUsage usage;
  NSSError error;
  PRUint32 level;
};
#endif

NSS_IMPLEMENT void ** /* void *[] */
NSSCert_ValidateCompletely (
  NSSCert *c,
  NSSTime time, /* NULL for "now" */
  NSSUsages *usages,
  NSSPolicies *policiesOpt, /* NULL for none */
  void **rvOpt, /* NULL for allocate */
  PRUint32 rvLimit, /* zero for no limit */
  NSSArena *arenaOpt /* NULL for heap */
)
{
    nss_SetError(NSS_ERROR_NOT_FOUND);
    return NULL;
}

NSS_IMPLEMENT PRStatus
NSSCert_ValidateAndDiscoverUsagesAndPolicies (
  NSSCert *c,
  NSSTime **notBeforeOutOpt,
  NSSTime **notAfterOutOpt,
  void *allowedUsages,
  void *disallowedUsages,
  void *allowedPolicies,
  void *disallowedPolicies,
  /* more args.. work on this fgmr */
  NSSArena *arenaOpt
)
{
    nss_SetError(NSS_ERROR_NOT_FOUND);
    return PR_FAILURE;
}

NSS_IMPLEMENT NSSUsages *
nssCert_GetTrustedUsages (
  NSSCert *c,
  NSSUsages *usagesOpt
)
{
    PRStatus status;
    PRBool freeIt = PR_FALSE;
    if (!usagesOpt) {
	usagesOpt = nss_ZNEW(NULL, NSSUsages);
	if (!usagesOpt) {
	    return (NSSUsages *)NULL;
	}
	freeIt = PR_TRUE;
    }
    usagesOpt->ca = get_trusted_usage(c, PR_TRUE, &status);
    if (status == PR_FAILURE) {
	if (freeIt) nss_ZFreeIf(usagesOpt);
	return (NSSUsages *)NULL;
    }
    usagesOpt->peer = get_trusted_usage(c, PR_FALSE, &status);
    if (status == PR_FAILURE) {
	if (freeIt) nss_ZFreeIf(usagesOpt);
	return (NSSUsages *)NULL;
    }
    return usagesOpt;
}

NSS_IMPLEMENT NSSUsages *
NSSCert_GetTrustedUsages (
  NSSCert *c,
  NSSUsages *usagesOpt
)
{
    return nssCert_GetTrustedUsages(c, usagesOpt);
}

NSS_IMPLEMENT PRBool
nssCert_IsTrustedForUsages (
  NSSCert *c,
  NSSUsages *usages,
  PRStatus *statusOpt
)
{
    NSSUsages certUsages;
    if (nssCert_GetTrustedUsages(c, &certUsages) == NULL) {
	if (statusOpt) *statusOpt = PR_FAILURE;
	return PR_FALSE;
    }
    return nssUsages_Match(usages, &certUsages);
}

static void
set_trust_for_usage (
  NSSUsage usage,
  nssTrust *trust,
  nssTrustLevel setLevel
)
{
    if (usage & NSSUsage_SSLClient) {
	trust->clientAuth = setLevel;
    }
    if (usage & NSSUsage_SSLServer) {
	trust->serverAuth = setLevel;
    }
    if (usage & (NSSUsage_EmailSigner | NSSUsage_EmailRecipient)) {
	trust->emailProtection = setLevel;
    }
    if (usage & NSSUsage_CodeSigner) {
	trust->codeSigning = setLevel;
    }
}

/* XXX move */
NSS_IMPLEMENT NSSToken *
nssTrust_GetWriteToken (
  nssTrust *t,
  nssSession **rvSessionOpt
)
{
    return nssPKIObject_GetWriteToken(&t->object, rvSessionOpt);
}

/* XXX move */
NSS_IMPLEMENT void
nssTrust_Clear (
  nssTrust *trust
)
{
    trust->clientAuth = nssTrustLevel_NotTrusted;
    trust->serverAuth = nssTrustLevel_NotTrusted;
    trust->emailProtection = nssTrustLevel_NotTrusted;
    trust->codeSigning = nssTrustLevel_NotTrusted;
}

/* XXX move */
NSS_IMPLEMENT nssTrust *
nssTrust_CreateNull (
  NSSTrustDomain *td
)
{
    nssPKIObject *pkio;
    nssTrust *trust = NULL;
    pkio = nssPKIObject_Create(NULL, NULL, td, NULL);
    if (pkio) {
	trust = nss_ZNEW(pkio->arena, nssTrust);
	if (trust) {
	    trust->object = *pkio;
	    nssTrust_Clear(trust);
	} else {
	    nssPKIObject_Destroy(pkio);
	}
    }
    return trust;
}

NSS_IMPLEMENT PRStatus
nssCert_SetTrustedUsages (
  NSSCert *c,
  NSSUsages *usages
)
{
    PRStatus status;
    nssTrust *trust;
    NSSToken *token;
    nssSession *session;
    nssCryptokiObject *instance;

    /* XXX needs to be cached with cert */
    trust = nssTrustDomain_FindTrustForCert(c->object.td, c);
    if (trust) {
	token = nssTrust_GetWriteToken(trust, &session);
	nssTrust_Clear(trust);
    } else {
	if (NSS_GetError() != NSS_ERROR_NO_ERROR) {
	    return PR_FAILURE;
	}
	/* XXX something better */
	/* create a new trust object */
	trust = nssTrust_CreateNull(c->object.td);
	if (!trust) {
	    return PR_FAILURE;
	}
	token = nssCert_GetWriteToken(c, &session);
	if (!token) {
	    /* XXX should extract from trust domain */
	    PR_ASSERT(0);
	    return PR_FAILURE;
	}
    }
    /* set the new trust values */
    set_trust_for_usage(usages->ca, trust, nssTrustLevel_TrustedDelegator);
    set_trust_for_usage(usages->peer, trust, nssTrustLevel_Trusted);
    /* import (set) the trust values on the token */
    instance = nssToken_ImportTrust(token, session, &c->encoding,
                                    &c->issuer, &c->serial,
                                    trust->serverAuth,
                                    trust->clientAuth,
                                    trust->codeSigning,
                                    trust->emailProtection,
                                    PR_TRUE);
    /* clean up */
    nssSession_Destroy(session);
    nssToken_Destroy(token);
    if (instance) {
	nssCryptokiObject_Destroy(instance);
	status = PR_SUCCESS;
    } else {
	status = PR_FAILURE;
    }
    nssTrust_Destroy(trust);
    return status;
}

NSS_IMPLEMENT PRStatus
NSSCert_SetTrustedUsages (
  NSSCert *c,
  NSSUsages *usages
)
{
    return nssCert_SetTrustedUsages(c, usages);
}

NSS_IMPLEMENT NSSDER *
nssCert_Encode (
  NSSCert *c,
  NSSDER *rvOpt,
  NSSArena *arenaOpt
)
{
    return nssItem_Duplicate((NSSItem *)&c->encoding, arenaOpt, rvOpt);
}

NSS_IMPLEMENT NSSDER *
NSSCert_Encode (
  NSSCert *c,
  NSSDER *rvOpt,
  NSSArena *arenaOpt
)
{
    return nssCert_Encode(c, rvOpt, arenaOpt);
}

static NSSCert *
filter_subject_certs_for_id (
  NSSCert **subjectCerts, 
  void *id
)
{
    NSSCert **si;
    NSSCert *rvCert = NULL;
    /* walk the subject certs */
    for (si = subjectCerts; *si; si++) {
	nssCertDecoding *dcp = nssCert_GetDecoding(*si);
	if (dcp->methods->isMyIdentifier(dcp->data, id)) {
	    /* this cert has the correct identifier */
	    rvCert = nssCert_AddRef(*si);
	    break;
	}
    }
    return rvCert;
}

static NSSCert *
find_cert_issuer (
  NSSCert *c,
  NSSTime time,
  NSSUsages *usagesOpt,
  NSSPolicies *policiesOpt
)
{
    NSSCert **issuers = NULL;
    NSSCert *issuer = NULL;
    NSSTrustDomain *td;
    NSSVolatileDomain *vd;
    vd = nssCert_GetVolatileDomain(c);
    td = nssCert_GetTrustDomain(c);
    if (vd) {
	issuers = nssVolatileDomain_FindCertsBySubject(vd,
	                                                      &c->issuer,
	                                                      NULL,
	                                                      0,
	                                                      NULL);
    } else {
	issuers = nssTrustDomain_FindCertsBySubject(td,
                                                           &c->issuer,
                                                           NULL,
                                                           0,
                                                           NULL);
    }
    if (issuers) {
	nssCertDecoding *dc = NULL;
	void *issuerID = NULL;
	dc = nssCert_GetDecoding(c);
	if (dc) {
	    issuerID = dc->methods->getIssuerIdentifier(dc->data);
	}
	if (issuerID) {
	    issuer = filter_subject_certs_for_id(issuers, issuerID);
	    dc->methods->freeIdentifier(issuerID);
	} else {
	    issuer = nssCertArray_FindBestCert(issuers,
	                                                     time,
	                                                     usagesOpt,
	                                                     policiesOpt);
	}
	nssCertArray_Destroy(issuers);
    }
    return issuer;
}

/* XXX review based on CERT_FindCertIssuer
 * this function is not using the authCertIssuer field as a fallback
 * if authority key id does not exist
 */
NSS_IMPLEMENT NSSCert **
nssCert_BuildChain (
  NSSCert *c,
  NSSTime time,
  NSSUsages *usagesOpt,
  NSSPolicies *policiesOpt,
  NSSCert **rvOpt,
  PRUint32 rvLimit,
  NSSArena *arenaOpt,
  PRStatus *statusOpt
)
{
    PRStatus status;
    NSSCert **rvChain;
    NSSTrustDomain *td;
    nssPKIObjectCollection *collection;
    NSSUsages usages = { 0 };

    td = NSSCert_GetTrustDomain(c);
    if (statusOpt) *statusOpt = PR_SUCCESS;

    /* initialize the collection with the current cert */
    collection = nssCertCollection_Create(td, NULL);
    if (!collection) {
	if (statusOpt) *statusOpt = PR_FAILURE;
	return (NSSCert **)NULL;
    }
    nssPKIObjectCollection_AddObject(collection, (nssPKIObject *)c);
    if (rvLimit == 1) {
	goto finish;
    }
    /* going from peer to CA */
    if (usagesOpt) {
	usages.ca = usagesOpt->peer;
	usagesOpt = &usages;
    }
    /* walk the chain */
    while (!nssItem_Equal(&c->subject, &c->issuer, &status)) {
	c = find_cert_issuer(c, time, usagesOpt, policiesOpt);
	if (c) {
	    nssPKIObjectCollection_AddObject(collection, (nssPKIObject *)c);
	    nssCert_Destroy(c); /* collection has it */
	    if (rvLimit > 0 &&
	        nssPKIObjectCollection_Count(collection) == rvLimit) 
	    {
		break;
	    }
	} else {
	    nss_SetError(NSS_ERROR_CERTIFICATE_ISSUER_NOT_FOUND);
	    if (statusOpt) *statusOpt = PR_FAILURE;
	    break;
	}
    }
finish:
    rvChain = nssPKIObjectCollection_GetCerts(collection, 
                                                     rvOpt, 
                                                     rvLimit, 
                                                     arenaOpt);
    nssPKIObjectCollection_Destroy(collection);
    return rvChain;
}

NSS_IMPLEMENT NSSCert **
NSSCert_BuildChain (
  NSSCert *c,
  NSSTime time,
  NSSUsages *usagesOpt,
  NSSPolicies *policiesOpt,
  NSSCert **rvOpt,
  PRUint32 rvLimit, /* zero for no limit */
  NSSArena *arenaOpt,
  PRStatus *statusOpt
)
{
    return nssCert_BuildChain(c, time, usagesOpt, policiesOpt,
                                     rvOpt, rvLimit, arenaOpt, statusOpt);
}

NSS_IMPLEMENT NSSItem *
NSSCert_Encrypt (
  NSSCert *c,
  const NSSAlgNParam *apOpt,
  NSSItem *data,
  NSSTime time,
  NSSUsages *usages,
  NSSPolicies *policiesOpt,
  NSSCallback *uhh,
  NSSItem *rvOpt,
  NSSArena *arenaOpt
)
{
    nss_SetError(NSS_ERROR_NOT_FOUND);
    return NULL;
}

NSS_IMPLEMENT PRStatus
NSSCert_Verify (
  NSSCert *c,
  const NSSAlgNParam *apOpt,
  NSSItem *data,
  NSSItem *signature,
  NSSTime time,
  NSSUsages *usages,
  NSSPolicies *policiesOpt,
  NSSCallback *uhh
)
{
    nss_SetError(NSS_ERROR_NOT_FOUND);
    return PR_FAILURE;
}

NSS_IMPLEMENT NSSItem *
NSSCert_VerifyRecover (
  NSSCert *c,
  const NSSAlgNParam *apOpt,
  NSSItem *signature,
  NSSTime time,
  NSSUsages *usages,
  NSSPolicies *policiesOpt,
  NSSCallback *uhh,
  NSSItem *rvOpt,
  NSSArena *arenaOpt
)
{
    nss_SetError(NSS_ERROR_NOT_FOUND);
    return NULL;
}

NSS_IMPLEMENT NSSItem *
nssCert_WrapSymKey (
  NSSCert *c,
  const NSSAlgNParam *ap,
  NSSSymKey *keyToWrap,
  NSSTime time,
  NSSUsages *usages,
  NSSPolicies *policiesOpt,
  NSSCallback *uhh,
  NSSItem *rvOpt,
  NSSArena *arenaOpt
)
{
    NSSPublicKey *pubKey;
    NSSItem *wrap;

    /* XXX do some validation */

    pubKey = nssCert_GetPublicKey(c);
    if (!pubKey) {
	return (NSSItem *)NULL;
    }

    wrap = nssPublicKey_WrapSymKey(pubKey, ap, keyToWrap,
                                         uhh, rvOpt, arenaOpt);
    nssPublicKey_Destroy(pubKey);
    return wrap;
}

NSS_IMPLEMENT NSSItem *
NSSCert_WrapSymKey (
  NSSCert *c,
  const NSSAlgNParam *ap,
  NSSSymKey *keyToWrap,
  NSSTime time,
  NSSUsages *usages,
  NSSPolicies *policiesOpt,
  NSSCallback *uhh,
  NSSItem *rvOpt,
  NSSArena *arenaOpt
)
{
    return nssCert_WrapSymKey(c, ap, keyToWrap,
                                           time, usages, policiesOpt,
                                           uhh, rvOpt, arenaOpt);
}

NSS_IMPLEMENT NSSCryptoContext *
NSSCert_CreateCryptoContext (
  NSSCert *c,
  const NSSAlgNParam *apOpt,
  NSSTime time,
  NSSUsages *usages,
  NSSPolicies *policiesOpt,
  NSSCallback *uhh  
)
{
    nss_SetError(NSS_ERROR_NOT_FOUND);
    return NULL;
}

NSS_IMPLEMENT NSSPublicKey *
nssCert_GetPublicKey (
  NSSCert *c
)
{
    PRStatus status;
    NSSToken **tokens, **tp;
    nssCryptokiObject *instance = NULL;
    NSSTrustDomain *td = nssCert_GetTrustDomain(c);
    NSSVolatileDomain *vd = nssCert_GetVolatileDomain(c);

    /* first look for a persistent object in the trust domain */
    tokens = nssPKIObject_GetTokens(&c->object, &status);
    if (tokens) {
	for (tp = tokens; *tp; tp++) {
	    /* XXX need to iterate over cert instances to have session */
	    nssSession *session = nssToken_CreateSession(*tp, PR_FALSE);
	    if (!session) {
		break;
	    }
	    instance = nssToken_FindPublicKeyByID(*tp, session, &c->id);
	    nssSession_Destroy(session);
	    if (instance) {
		break;
	    }
	}
	/* also search on other tokens? */
	nssTokenArray_Destroy(tokens);
    }
    if (instance) {
	NSSPublicKey *bk = NULL;
	/* found a persistent instance of the pubkey, return it */
	bk = nssPublicKey_CreateFromInstance(instance, td, vd, NULL);
	if (!bk) {
	    nssCryptokiObject_Destroy(instance);
	    return (NSSPublicKey *)NULL;
	}
	return bk;
    } else {
	NSSOID *keyAlg;
	NSSBitString keyBits;
	nssCertDecoding *dc = nssCert_GetDecoding(c);

	/* create an ephemeral pubkey object, either in the cert's
	 * volatile domain (if it exists), or as a standalone object
	 * that will be destroyed with the cert
	 */
	status = dc->methods->getPublicKeyInfo(dc->data, &keyAlg, &keyBits);
	if (status == PR_SUCCESS) {
	    c->bk = nssPublicKey_CreateFromInfo(td, vd, keyAlg, &keyBits);
	    return nssPublicKey_AddRef(c->bk);
	}
    }
    return (NSSPublicKey *)NULL;
}

NSS_IMPLEMENT NSSPublicKey *
NSSCert_GetPublicKey (
  NSSCert *c
)
{
    return nssCert_GetPublicKey(c);
}

NSS_IMPLEMENT NSSPrivateKey *
nssCert_FindPrivateKey (
  NSSCert *c,
  NSSCallback *uhh
)
{
    PRStatus status;
    NSSToken **tokens, **tp;
    nssCryptokiObject *instance;
    NSSTrustDomain *td = nssCert_GetTrustDomain(c);

    tokens = nssPKIObject_GetTokens(&c->object, &status);
    if (!tokens) {
	return PR_FALSE; /* actually, should defer to crypto context */
    }
    for (tp = tokens; *tp; tp++) {
	NSSSlot *slot = nssToken_GetSlot(*tp);
	NSSCallback *pwcb = uhh ? 
	                    uhh : 
	                    nssTrustDomain_GetDefaultCallback(td, NULL);
	status = nssSlot_Login(slot, pwcb);
	nssSlot_Destroy(slot);
	if (status != PR_SUCCESS) {
	    break;
	}
	/* XXX need to iterate over cert instances to have session */
	{
	    nssSession *session = nssToken_CreateSession(*tp, PR_FALSE);
	    instance = nssToken_FindPrivateKeyByID(*tp, session, &c->id);
	    nssSession_Destroy(session);
	    if (instance) {
		break;
	    }
	}
    }
    /* also search on other tokens? */
    nssTokenArray_Destroy(tokens);
    if (instance) {
	nssPKIObject *pkio;
	NSSPrivateKey *vk = NULL;
	pkio = nssPKIObject_Create(NULL, instance, td, /* XXX cc */ NULL);
	if (!pkio) {
	    nssCryptokiObject_Destroy(instance);
	    return (NSSPrivateKey *)NULL;
	}
	vk = nssPrivateKey_Create(pkio);
	if (!vk) {
	    nssPKIObject_Destroy(pkio);
	    return (NSSPrivateKey *)NULL;
	}
	return vk;
    }
    return (NSSPrivateKey *)NULL;
}

NSS_IMPLEMENT NSSPrivateKey *
NSSCert_FindPrivateKey (
  NSSCert *c,
  NSSCallback *uhh
)
{
    return nssCert_FindPrivateKey(c, uhh);
}

NSS_IMPLEMENT PRBool
nssCert_IsPrivateKeyAvailable (
  NSSCert *c,
  NSSCallback *uhh,
  PRStatus *statusOpt
)
{
    PRStatus status;
    NSSToken **tokens, **tp;
    nssCryptokiObject *instance = NULL;
    NSSTrustDomain *td = nssCert_GetTrustDomain(c);
    PRBool isLoggedIn;
    tokens = nssPKIObject_GetTokens(&c->object, &status);
    if (!tokens) {
	return PR_FALSE; /* can't have private key w/o a token instance */
    }
    for (tp = tokens; *tp; tp++) {
	NSSSlot *slot;
	/* XXX need to iterate over cert instances to have session */
	nssSession *session = nssToken_CreateSession(*tp, PR_FALSE);
	if (!session) {
	    break;
	}
	slot = nssToken_GetSlot(*tp);
	isLoggedIn = nssSlot_IsLoggedIn(slot);
	nssSlot_Destroy(slot);
	if (isLoggedIn) {
	    instance = nssToken_FindPrivateKeyByID(*tp, session, &c->id);
	} else {
	    instance = nssToken_FindPublicKeyByID(*tp, session, &c->id);
	}
	nssSession_Destroy(session);
	if (instance) {
	    break;
	}
    }
    nssTokenArray_Destroy(tokens);
    if (instance) {
	nssCryptokiObject_Destroy(instance);
	return PR_TRUE;
    } else {
	return PR_FALSE;
    }
}

NSS_IMPLEMENT PRBool
NSSCert_IsPrivateKeyAvailable (
  NSSCert *c,
  NSSCallback *uhh,
  PRStatus *statusOpt
)
{
    return nssCert_IsPrivateKeyAvailable(c, uhh, statusOpt);
}

NSS_IMPLEMENT PRBool
NSSUserCert_IsStillPresent (
  NSSUserCert *uc,
  PRStatus *statusOpt
)
{
    nss_SetError(NSS_ERROR_NOT_FOUND);
    return PR_FALSE;
}

NSS_IMPLEMENT NSSItem *
NSSUserCert_Decrypt (
  NSSUserCert *uc,
  const NSSAlgNParam *apOpt,
  NSSItem *data,
  NSSTime time,
  NSSUsages *usages,
  NSSPolicies *policiesOpt,
  NSSCallback *uhh,
  NSSItem *rvOpt,
  NSSArena *arenaOpt
)
{
    nss_SetError(NSS_ERROR_NOT_FOUND);
    return NULL;
}

NSS_IMPLEMENT NSSItem *
NSSUserCert_Sign (
  NSSUserCert *uc,
  const NSSAlgNParam *apOpt,
  NSSItem *data,
  NSSTime time,
  NSSUsages *usages,
  NSSPolicies *policiesOpt,
  NSSCallback *uhh,
  NSSItem *rvOpt,
  NSSArena *arenaOpt
)
{
    nss_SetError(NSS_ERROR_NOT_FOUND);
    return NULL;
}

NSS_IMPLEMENT NSSItem *
NSSUserCert_SignRecover (
  NSSUserCert *uc,
  const NSSAlgNParam *apOpt,
  NSSItem *data,
  NSSTime time,
  NSSUsages *usages,
  NSSPolicies *policiesOpt,
  NSSCallback *uhh,
  NSSItem *rvOpt,
  NSSArena *arenaOpt
)
{
    nss_SetError(NSS_ERROR_NOT_FOUND);
    return NULL;
}

NSS_IMPLEMENT NSSSymKey *
NSSUserCert_UnwrapSymKey (
  NSSUserCert *uc,
  const NSSAlgNParam *apOpt,
  NSSItem *wrappedKey,
  NSSTime time,
  NSSUsages *usages,
  NSSPolicies *policiesOpt,
  NSSCallback *uhh,
  NSSItem *rvOpt,
  NSSArena *arenaOpt
)
{
    nss_SetError(NSS_ERROR_NOT_FOUND);
    return NULL;
}

NSS_IMPLEMENT NSSSymKey *
NSSUserCert_DeriveSymKey (
  NSSUserCert *uc, /* provides private key */
  NSSCert *c, /* provides public key */
  const NSSAlgNParam *apOpt,
  NSSOID *target,
  PRUint32 keySizeOpt, /* zero for best allowed */
  NSSOperations operations,
  NSSCallback *uhh
)
{
    nss_SetError(NSS_ERROR_NOT_FOUND);
    return NULL;
}

NSS_IMPLEMENT nssTrust *
nssTrust_Create (
  nssPKIObject *object
)
{
    PRStatus status;
    PRUint32 i;
    PRUint32 lastTrustOrder, myTrustOrder;
    NSSModule *module;
    nssTrust *rvt;
    nssCryptokiObject *instance;
    nssTrustLevel serverAuth, clientAuth, codeSigning, emailProtection;
    lastTrustOrder = 1<<16; /* just make it big */
    PR_ASSERT(object->instances != NULL && object->numInstances > 0);
    rvt = nss_ZNEW(object->arena, nssTrust);
    if (!rvt) {
	return (nssTrust *)NULL;
    }
    rvt->object = *object;
    /* trust has to peek into the base object members */
    PZ_Lock(object->lock);
    for (i=0; i<object->numInstances; i++) {
	/* get the trust order from the token's module */
	instance = object->instances[i];
	module = nssToken_GetModule(instance->token);
	myTrustOrder = nssModule_GetTrustOrder(module);
	nssModule_Destroy(module);
	/* get the trust values from this token */
	status = nssCryptokiTrust_GetAttributes(instance,
	                                        &serverAuth,
	                                        &clientAuth,
	                                        &codeSigning,
	                                        &emailProtection);
	if (status != PR_SUCCESS) {
	    PZ_Unlock(object->lock);
	    return (nssTrust *)NULL;
	}
	if (rvt->serverAuth == nssTrustLevel_Unknown ||
	    myTrustOrder < lastTrustOrder) 
	{
	    rvt->serverAuth = serverAuth;
	}
	if (rvt->clientAuth == nssTrustLevel_Unknown ||
	    myTrustOrder < lastTrustOrder) 
	{
	    rvt->clientAuth = clientAuth;
	}
	if (rvt->emailProtection == nssTrustLevel_Unknown ||
	    myTrustOrder < lastTrustOrder) 
	{
	    rvt->emailProtection = emailProtection;
	}
	if (rvt->codeSigning == nssTrustLevel_Unknown ||
	    myTrustOrder < lastTrustOrder) 
	{
	    rvt->codeSigning = codeSigning;
	}
	lastTrustOrder = myTrustOrder;
    }
    PZ_Unlock(object->lock);
    return rvt;
}

NSS_IMPLEMENT nssTrust *
nssTrust_AddRef (
  nssTrust *trust
)
{
    if (trust) {
	nssPKIObject_AddRef(&trust->object);
    }
    return trust;
}

NSS_IMPLEMENT PRStatus
nssTrust_Destroy (
  nssTrust *trust
)
{
    if (trust) {
	(void)nssPKIObject_Destroy(&trust->object);
    }
    return PR_SUCCESS;
}

struct nssSMIMEProfileStr
{
    nssPKIObject object;
    NSSCert *certificate;
    NSSASCII7 *email;
    NSSDER *subject;
    NSSItem *profileTime;
    NSSItem *profileData;
};

NSS_IMPLEMENT nssSMIMEProfile *
nssSMIMEProfile_Create (
  NSSCert *cert,
  NSSItem *profileTime,
  NSSItem *profileData
)
{
#if 0
    NSSArena *arena;
    nssSMIMEProfile *rvProfile;
    nssPKIObject *object;
    NSSTrustDomain *td = nssCert_GetTrustDomain(cert);
    NSSCryptoContext *cc = nssCert_GetCryptoContext(cert);
    arena = nssArena_Create();
    if (!arena) {
	return NULL;
    }
    object = nssPKIObject_Create(arena, NULL, td, cc);
    if (!object) {
	goto loser;
    }
    rvProfile = nss_ZNEW(arena, nssSMIMEProfile);
    if (!rvProfile) {
	goto loser;
    }
    rvProfile->object = *object;
    rvProfile->certificate = cert;
    rvProfile->email = nssUTF8_Duplicate(cert->email, arena);
    rvProfile->subject = nssItem_Duplicate(&cert->subject, arena, NULL);
    if (profileTime) {
	rvProfile->profileTime = nssItem_Duplicate(profileTime, arena, NULL);
    }
    if (profileData) {
	rvProfile->profileData = nssItem_Duplicate(profileData, arena, NULL);
    }
    return rvProfile;
loser:
    nssPKIObject_Destroy(object);
#endif
    return (nssSMIMEProfile *)NULL;
}

NSS_IMPLEMENT nssSMIMEProfile *
nssSMIMEProfile_AddRef (
  nssSMIMEProfile *profile
)
{
    if (profile) {
	nssPKIObject_AddRef(&profile->object);
    }
    return profile;
}

NSS_IMPLEMENT PRStatus
nssSMIMEProfile_Destroy (
  nssSMIMEProfile *profile
)
{
    if (profile) {
	(void)nssPKIObject_Destroy(&profile->object);
    }
    return PR_SUCCESS;
}

struct NSSCRLStr {
  nssPKIObject object;
  NSSDER encoding;
  NSSUTF8 *url;
  PRBool isKRL;
};

NSS_IMPLEMENT NSSCRL *
nssCRL_Create (
  nssPKIObject *object
)
{
    PRStatus status;
    NSSCRL *rvCRL;
    NSSArena *arena = object->arena;
    PR_ASSERT(object->instances != NULL && object->numInstances > 0);
    rvCRL = nss_ZNEW(arena, NSSCRL);
    if (!rvCRL) {
	return (NSSCRL *)NULL;
    }
    rvCRL->object = *object;
    /* XXX should choose instance based on some criteria */
    status = nssCryptokiCRL_GetAttributes(object->instances[0],
                                          arena,
                                          &rvCRL->encoding,
                                          &rvCRL->url,
                                          &rvCRL->isKRL);
    if (status != PR_SUCCESS) {
	return (NSSCRL *)NULL;
    }
    return rvCRL;
}

NSS_IMPLEMENT NSSCRL *
nssCRL_AddRef (
  NSSCRL *crl
)
{
    if (crl) {
	nssPKIObject_AddRef(&crl->object);
    }
    return crl;
}

NSS_IMPLEMENT PRStatus
nssCRL_Destroy (
  NSSCRL *crl
)
{
    if (crl) {
	(void)nssPKIObject_Destroy(&crl->object);
    }
    return PR_SUCCESS;
}

NSS_IMPLEMENT PRStatus
nssCRL_DeleteStoredObject (
  NSSCRL *crl,
  NSSCallback *uhh
)
{
    return nssPKIObject_DeleteStoredObject(&crl->object, uhh, PR_TRUE);
}

NSS_IMPLEMENT NSSDER *
nssCRL_GetEncoding (
  NSSCRL *crl
)
{
    if (crl->encoding.data != NULL && crl->encoding.size > 0) {
	return &crl->encoding;
    } else {
	return (NSSDER *)NULL;
    }
}
