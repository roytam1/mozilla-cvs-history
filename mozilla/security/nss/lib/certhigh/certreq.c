/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is the Netscape security libraries.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1994-2000
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "cert.h"
#include "certt.h"
#include "secder.h"
#include "key.h"
#include "secitem.h"
#include "secasn1.h"
#include "secerr.h"

const SEC_ASN1Template CERT_AttributeTemplate[] = {
    { SEC_ASN1_SEQUENCE,
	0, NULL, sizeof(CERTAttribute) },
    { SEC_ASN1_OBJECT_ID, offsetof(CERTAttribute, attrType) },
    { SEC_ASN1_SET_OF, offsetof(CERTAttribute, attrValue),
	SEC_AnyTemplate },
    { 0 }
};

const SEC_ASN1Template CERT_SetOfAttributeTemplate[] = {
    { SEC_ASN1_SET_OF, 0, CERT_AttributeTemplate },
};

const SEC_ASN1Template CERT_CertificateRequestTemplate[] = {
    { SEC_ASN1_SEQUENCE,
	  0, NULL, sizeof(CERTCertificateRequest) },
    { SEC_ASN1_INTEGER,
	  offsetof(CERTCertificateRequest,version) },
    { SEC_ASN1_INLINE,
	  offsetof(CERTCertificateRequest,subject),
	  CERT_NameTemplate },
    { SEC_ASN1_INLINE,
	  offsetof(CERTCertificateRequest,subjectPublicKeyInfo),
	  CERT_SubjectPublicKeyInfoTemplate },
    { SEC_ASN1_CONSTRUCTED | SEC_ASN1_CONTEXT_SPECIFIC | 0,
	  offsetof(CERTCertificateRequest,attributes), 
	  CERT_SetOfAttributeTemplate },
    { 0 }
};

SEC_ASN1_CHOOSER_IMPLEMENT(CERT_CertificateRequestTemplate)

CERTCertificate *
CERT_CreateCertificate(unsigned long serialNumber,
		      CERTName *issuer,
		      CERTValidity *validity,
		      CERTCertificateRequest *req)
{
    CERTCertificate *c;
    int rv;
    PRArenaPool *arena;
    
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    
    if ( !arena ) {
	return(0);
    }

    c = (CERTCertificate *)PORT_ArenaZAlloc(arena, sizeof(CERTCertificate));
    
    if (c) {
	c->referenceCount = 1;
	c->arena = arena;

	/*
	 * Default is a plain version 1.
	 * If extensions are added, it will get changed as appropriate.
	 */
	rv = DER_SetUInteger(arena, &c->version, SEC_CERTIFICATE_VERSION_1);
	if (rv) goto loser;

	rv = DER_SetUInteger(arena, &c->serialNumber, serialNumber);
	if (rv) goto loser;

	rv = CERT_CopyName(arena, &c->issuer, issuer);
	if (rv) goto loser;

	rv = CERT_CopyValidity(arena, &c->validity, validity);
	if (rv) goto loser;

	rv = CERT_CopyName(arena, &c->subject, &req->subject);
	if (rv) goto loser;
	rv = SECKEY_CopySubjectPublicKeyInfo(arena, &c->subjectPublicKeyInfo,
					  &req->subjectPublicKeyInfo);
	if (rv) goto loser;
    }
    return c;

  loser:
    CERT_DestroyCertificate(c);
    return 0;
}

/************************************************************************/

CERTCertificateRequest *
CERT_CreateCertificateRequest(CERTName *subject,
			     CERTSubjectPublicKeyInfo *spki,
			     SECItem **attributes)
{
    CERTCertificateRequest *certreq;
    PRArenaPool *arena;
    SECStatus rv;

    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( arena == NULL ) {
	return NULL;
    }
    
    certreq = (CERTCertificateRequest *)
			PORT_ArenaZAlloc(arena, sizeof(CERTCertificateRequest));

    if (certreq != NULL) {
	certreq->arena = arena;
	
	rv = DER_SetUInteger(arena, &certreq->version,
			     SEC_CERTIFICATE_REQUEST_VERSION);
	if (rv != SECSuccess)
	    goto loser;

	rv = CERT_CopyName(arena, &certreq->subject, subject);
	if (rv != SECSuccess)
	    goto loser;

	rv = SECKEY_CopySubjectPublicKeyInfo(arena,
					  &certreq->subjectPublicKeyInfo,
					  spki);
	if (rv != SECSuccess)
	    goto loser;

	/* Copy over attribute information */
	if (attributes) {
	    int i = 0;

	    /* allocate space for attributes */
	    while(attributes[i] != NULL) i++;
	    certreq->attributes = PORT_ArenaZNewArray(arena,CERTAttribute*,i+1);
	    if(!certreq->attributes) {
		goto loser;
	    }

	    /* copy attributes */
	    i = 0;
	    while(attributes[i]) {
		/*
		** Attributes are a SetOf Attribute which implies
		** lexigraphical ordering.  It is assumes that the
		** attributes are passed in sorted.  If we need to
		** add functionality to sort them, there is an
		** example in the PKCS 7 code.
		*/
		certreq->attributes[i] = (SECItem*)PORT_ArenaZAlloc(arena, 
							  	    sizeof(SECItem));
		if(!certreq->attributes[i]) {
		    goto loser;
		};
		rv = SECITEM_CopyItem(arena, certreq->attributes[i], 
				      attributes[i]);
		if (rv != SECSuccess) {
		    goto loser;
		}
		i++;
	    }
	    certreq->attributes[i] = NULL;
	} else {
	    /*
	     ** Invent empty attribute information. According to the
	     ** pkcs#10 spec, attributes has this ASN.1 type:
	     **
	     ** attributes [0] IMPLICIT Attributes
	     ** 
	     ** Which means, we should create a NULL terminated list
	     ** with the first entry being NULL;
	     */
	    certreq->attributes = (SECItem**)PORT_ArenaZAlloc(arena, sizeof(SECItem *));
	    if(!certreq->attributes) {
		goto loser;
	    }
	    certreq->attributes[0] = NULL;
	} 
    } else {
	PORT_FreeArena(arena, PR_FALSE);
    }

    return certreq;

loser:
    CERT_DestroyCertificateRequest(certreq);
    return NULL;
}

void
CERT_DestroyCertificateRequest(CERTCertificateRequest *req)
{
    if (req && req->arena) {
	PORT_FreeArena(req->arena, PR_FALSE);
    }
    return;
}

static void
setCRExt(void *o, CERTCertExtension **exts)
{
    ((CERTCertificateRequest *)o)->attributes = (struct CERTAttributeStr **)exts;
}

/*
** Set up to start gathering cert extensions for a cert request.
** The list is created as CertExtensions and converted to an
** attribute list by CERT_FinishCRAttributes().
 */
extern void *cert_StartExtensions(void *owner, PRArenaPool *ownerArena,
                       void (*setExts)(void *object, CERTCertExtension **exts));
void *
CERT_StartCertificateRequestAttributes(CERTCertificateRequest *req)
{
    return (cert_StartExtensions ((void *)req, req->arena, setCRExt));
}

/*
** At entry req->attributes actually contains an list of cert extensions--
** req-attributes is overloaded until the list is DER encoded (the first
** ...EncodeItem() below).
** We turn this into an attribute list by encapsulating it
** in a PKCS 10 Attribute structure
 */
SECStatus
CERT_FinishCertificateRequestAttributes(CERTCertificateRequest *req)
{   SECItem *extlist;
    SECOidData *oidrec;
    CERTAttribute *attribute;
   
    if (!req || !req->arena) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
        return SECFailure;
    }
    if (req->attributes == NULL)
        return SECSuccess;

    extlist = SEC_ASN1EncodeItem(req->arena, NULL, &req->attributes,
                            SEC_ASN1_GET(CERT_SequenceOfCertExtensionTemplate));
    if (extlist == NULL)
        return(SECFailure);

    oidrec = SECOID_FindOIDByTag(SEC_OID_PKCS9_EXTENSION_REQUEST);
    if (oidrec == NULL)
	return SECFailure;

    /* now change the list of cert extensions into a list of attributes
     */
    req->attributes = PORT_ArenaZNewArray(req->arena, CERTAttribute*, 2);

    attribute = PORT_ArenaZNew(req->arena, CERTAttribute);
    
    if (req->attributes == NULL || attribute == NULL ||
        SECITEM_CopyItem(req->arena, &attribute->attrType, &oidrec->oid) != 0) {
        PORT_SetError(SEC_ERROR_NO_MEMORY);
	return SECFailure;
    }
    attribute->attrValue = PORT_ArenaZNewArray(req->arena, SECItem*, 2);

    if (attribute->attrValue == NULL)
        return SECFailure;

    attribute->attrValue[0] = extlist;
    attribute->attrValue[1] = NULL;
    req->attributes[0] = attribute;
    req->attributes[1] = NULL;

    return SECSuccess;
}

SECStatus
CERT_GetCertificateRequestExtensions(CERTCertificateRequest *req,
                                        CERTCertExtension ***exts)
{
    if (req == NULL || exts == NULL) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
        return SECFailure;
    }
    
    if (req->attributes == NULL || *req->attributes == NULL)
        return SECSuccess;
    
    if ((*req->attributes)->attrValue == NULL) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
        return SECFailure;
    }

    return(SEC_ASN1DecodeItem(req->arena, exts, 
            SEC_ASN1_GET(CERT_SequenceOfCertExtensionTemplate),
            (*req->attributes)->attrValue[0]));
}
