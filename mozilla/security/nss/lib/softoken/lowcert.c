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

/*
 * Certificate handling code
 *
 * $Id$
 */

#include "seccomon.h"
#include "secder.h"
#include "nssilock.h"
#include "prmon.h"
#include "prtime.h"
#include "lowkeyi.h"
#include "pcert.h"
#include "secasn1.h"
#include "secoid.h"

/* should have been in a 'util' header */
extern const SEC_ASN1Template CERT_ValidityTemplate[];

static const SEC_ASN1Template nsslowcert_CertKeyTemplate[] = {
    { SEC_ASN1_SEQUENCE,
	  0, NULL, sizeof(NSSLOWCERTCertKey) },
    { SEC_ASN1_EXPLICIT | SEC_ASN1_OPTIONAL | SEC_ASN1_CONSTRUCTED | 
	  SEC_ASN1_CONTEXT_SPECIFIC | 0, 0, SEC_SkipTemplate },	/* version */ 
    { SEC_ASN1_INTEGER, offsetof(NSSLOWCERTCertKey,serialNumber) },
    { SEC_ASN1_SKIP },		/* signature algorithm */
    { SEC_ASN1_ANY, offsetof(NSSLOWCERTCertKey,derIssuer) },
    { SEC_ASN1_SKIP_REST },
    { 0 }
};

const SEC_ASN1Template nsslowcert_SubjectPublicKeyInfoTemplate[] = {
    { SEC_ASN1_SEQUENCE, 0, NULL, sizeof(NSSLOWCERTSubjectPublicKeyInfo) },
    { SEC_ASN1_INLINE, offsetof(NSSLOWCERTSubjectPublicKeyInfo,algorithm),
          SECOID_AlgorithmIDTemplate },
    { SEC_ASN1_BIT_STRING,
          offsetof(NSSLOWCERTSubjectPublicKeyInfo,subjectPublicKey), },
    { 0, }
};

const SEC_ASN1Template nsslowcert_CertificateTemplate[] = {
    { SEC_ASN1_SEQUENCE,
      0, NULL, sizeof(NSSLOWCERTCertificate) },
    { SEC_ASN1_EXPLICIT | SEC_ASN1_OPTIONAL | SEC_ASN1_CONSTRUCTED |
          SEC_ASN1_CONTEXT_SPECIFIC | 0, 0, SEC_SkipTemplate }, /* version */
    { SEC_ASN1_INTEGER, offsetof(NSSLOWCERTCertificate,serialNumber) },
    { SEC_ASN1_SKIP }, /* Signature algorithm */
    { SEC_ASN1_ANY, offsetof(NSSLOWCERTCertificate,derIssuer) },
    { SEC_ASN1_INLINE,
          offsetof(NSSLOWCERTCertificate,validity),
          CERT_ValidityTemplate },
    { SEC_ASN1_ANY, offsetof(NSSLOWCERTCertificate,derSubject) },
    { SEC_ASN1_INLINE,
          offsetof(NSSLOWCERTCertificate,subjectPublicKeyInfo),
          nsslowcert_SubjectPublicKeyInfoTemplate },
    { SEC_ASN1_SKIP_REST },
    { 0 }
};
const SEC_ASN1Template nsslowcert_SignedCertificateTemplate[] =
{
    { SEC_ASN1_SEQUENCE, 0, NULL, sizeof(NSSLOWCERTCertificate) },
    { SEC_ASN1_INLINE, 0, nsslowcert_CertificateTemplate },
    { SEC_ASN1_SKIP_REST },
    { 0 }
};
const SEC_ASN1Template nsslowcert_SignedDataTemplate[] = {
    { SEC_ASN1_SEQUENCE, 0, NULL, sizeof(NSSLOWCERTSignedData) },
    { SEC_ASN1_ANY, offsetof(NSSLOWCERTSignedData,data), },
    { SEC_ASN1_SKIP_REST },
    { 0, }
};
const SEC_ASN1Template nsslowcert_RSAPublicKeyTemplate[] = {
    { SEC_ASN1_SEQUENCE, 0, NULL, sizeof(NSSLOWKEYPublicKey) },
    { SEC_ASN1_INTEGER, offsetof(NSSLOWKEYPublicKey,u.rsa.modulus), },
    { SEC_ASN1_INTEGER, offsetof(NSSLOWKEYPublicKey,u.rsa.publicExponent), },
    { 0, }
};
const SEC_ASN1Template nsslowcert_DSAPublicKeyTemplate[] = {
    { SEC_ASN1_INTEGER, offsetof(NSSLOWKEYPublicKey,u.dsa.publicValue), },
    { 0, }
};
const SEC_ASN1Template nsslowcert_DHPublicKeyTemplate[] = {
    { SEC_ASN1_INTEGER, offsetof(NSSLOWKEYPublicKey,u.dh.publicValue), },
    { 0, }
};

/*
 * See bugzilla bug 125359
 * Since NSS (via PKCS#11) wants to handle big integers as unsigned ints,
 * all of the templates above that en/decode into integers must be converted
 * from ASN.1's signed integer type.  This is done by marking either the
 * source or destination (encoding or decoding, respectively) type as
 * siUnsignedInteger.
 */

static void
prepare_low_rsa_pub_key_for_asn1(NSSLOWKEYPublicKey *pubk)
{
    pubk->u.rsa.modulus.type = siUnsignedInteger;
    pubk->u.rsa.publicExponent.type = siUnsignedInteger;
}

static void
prepare_low_dsa_pub_key_for_asn1(NSSLOWKEYPublicKey *pubk)
{
    pubk->u.dsa.publicValue.type = siUnsignedInteger;
    pubk->u.dsa.params.prime.type = siUnsignedInteger;
    pubk->u.dsa.params.subPrime.type = siUnsignedInteger;
    pubk->u.dsa.params.base.type = siUnsignedInteger;
}

static void
prepare_low_dh_pub_key_for_asn1(NSSLOWKEYPublicKey *pubk)
{
    pubk->u.dh.prime.type = siUnsignedInteger;
    pubk->u.dh.base.type = siUnsignedInteger;
    pubk->u.dh.publicValue.type = siUnsignedInteger;
}

/*
 * Allow use of default cert database, so that apps(such as mozilla) don't
 * have to pass the handle all over the place.
 */
static NSSLOWCERTCertDBHandle *default_pcert_db_handle = 0;

void
nsslowcert_SetDefaultCertDB(NSSLOWCERTCertDBHandle *handle)
{
    default_pcert_db_handle = handle;
    
    return;
}

NSSLOWCERTCertDBHandle *
nsslowcert_GetDefaultCertDB(void)
{
    return(default_pcert_db_handle);
}

/*
 * simple cert decoder to avoid the cost of asn1 engine
 */ 
static unsigned char *
nsslowcert_dataStart(unsigned char *buf, int length, 
				 int *data_length, PRBool includeTag) {
    unsigned char tag;
    int used_length= 0;

    tag = buf[used_length++];

    /* blow out when we come to the end */
    if (tag == 0) {
	return NULL;
    }

    *data_length = buf[used_length++];

    if (*data_length&0x80) {
	int  len_count = *data_length & 0x7f;

	*data_length = 0;

	while (len_count-- > 0) {
	    *data_length = (*data_length << 8) | buf[used_length++];
	} 
    }

    if (*data_length > (length-used_length) ) {
	*data_length = length-used_length;
	return NULL;
    }
    if (includeTag) *data_length += used_length;

    return (buf + (includeTag ? 0 : used_length));	
}

static int
nsslowcert_GetValidityFields(unsigned char *buf,int buf_length,
	SECItem *notBefore, SECItem *notAfter)
{
    notBefore->data = nsslowcert_dataStart(buf,buf_length,
						&notBefore->len,PR_FALSE);
    if (notBefore->data == NULL) return SECFailure;
    buf_length -= (notBefore->data-buf) + notBefore->len;
    buf = notBefore->data + notBefore->len;
    notAfter->data = nsslowcert_dataStart(buf,buf_length,
						&notAfter->len,PR_FALSE);
    if (notAfter->data == NULL) return SECFailure;
    return SECSuccess;
}

static int
nsslowcert_GetCertFields(unsigned char *cert,int cert_length,
	SECItem *issuer, SECItem *serial, SECItem *derSN, SECItem *subject,
	SECItem *valid, SECItem *subjkey)
{
    unsigned char *buf;
    int buf_length;
    unsigned char *dummy;
    int dummylen;

    /* get past the signature wrap */
    buf = nsslowcert_dataStart(cert,cert_length,&buf_length,PR_FALSE);
    if (buf == NULL) return SECFailure;
    /* get into the raw cert data */
    buf = nsslowcert_dataStart(buf,buf_length,&buf_length,PR_FALSE);
    if (buf == NULL) return SECFailure;
    /* skip past any optional version number */
    if ((buf[0] & 0xa0) == 0xa0) {
	dummy = nsslowcert_dataStart(buf,buf_length,&dummylen,PR_FALSE);
	if (dummy == NULL) return SECFailure;
	buf_length -= (dummy-buf) + dummylen;
	buf = dummy + dummylen;
    }
    /* serial number */
    if (derSN) {
	derSN->data=nsslowcert_dataStart(buf,buf_length,&derSN->len,PR_TRUE);
    }
    serial->data = nsslowcert_dataStart(buf,buf_length,&serial->len,PR_FALSE);
    if (serial->data == NULL) return SECFailure;
    buf_length -= (serial->data-buf) + serial->len;
    buf = serial->data + serial->len;
    /* skip the OID */
    dummy = nsslowcert_dataStart(buf,buf_length,&dummylen,PR_FALSE);
    if (dummy == NULL) return SECFailure;
    buf_length -= (dummy-buf) + dummylen;
    buf = dummy + dummylen;
    /* issuer */
    issuer->data = nsslowcert_dataStart(buf,buf_length,&issuer->len,PR_TRUE);
    if (issuer->data == NULL) return SECFailure;
    buf_length -= (issuer->data-buf) + issuer->len;
    buf = issuer->data + issuer->len;

    /* only wanted issuer/SN */
    if (valid == NULL) {
	return SECSuccess;
    }
    /* validity */
    valid->data = nsslowcert_dataStart(buf,buf_length,&valid->len,PR_FALSE);
    if (valid->data == NULL) return SECFailure;
    buf_length -= (valid->data-buf) + valid->len;
    buf = valid->data + valid->len;
    /*subject */
    subject->data=nsslowcert_dataStart(buf,buf_length,&subject->len,PR_TRUE);
    if (subject->data == NULL) return SECFailure;
    buf_length -= (subject->data-buf) + subject->len;
    buf = subject->data + subject->len;
    /* subject  key info */
    subjkey->data=nsslowcert_dataStart(buf,buf_length,&subjkey->len,PR_TRUE);
    if (subjkey->data == NULL) return SECFailure;
    buf_length -= (subjkey->data-buf) + subjkey->len;
    buf = subjkey->data + subjkey->len;
    return SECSuccess;
}

SECStatus
nsslowcert_GetCertTimes(NSSLOWCERTCertificate *c, PRTime *notBefore, PRTime *notAfter)
{
    int rv;
    NSSLOWCERTValidity validity;

    rv = nsslowcert_GetValidityFields(c->validity.data,c->validity.len,
				&validity.notBefore,&validity.notAfter);
    if (rv != SECSuccess) {
	return rv;
    }
    
    /* convert DER not-before time */
    rv = DER_UTCTimeToTime(notBefore, &validity.notBefore);
    if (rv) {
	return(SECFailure);
    }
    
    /* convert DER not-after time */
    rv = DER_UTCTimeToTime(notAfter, &validity.notAfter);
    if (rv) {
	return(SECFailure);
    }

    return(SECSuccess);
}

/*
 * is certa newer than certb?  If one is expired, pick the other one.
 */
PRBool
nsslowcert_IsNewer(NSSLOWCERTCertificate *certa, NSSLOWCERTCertificate *certb)
{
    PRTime notBeforeA, notAfterA, notBeforeB, notAfterB, now;
    SECStatus rv;
    PRBool newerbefore, newerafter;
    
    rv = nsslowcert_GetCertTimes(certa, &notBeforeA, &notAfterA);
    if ( rv != SECSuccess ) {
	return(PR_FALSE);
    }
    
    rv = nsslowcert_GetCertTimes(certb, &notBeforeB, &notAfterB);
    if ( rv != SECSuccess ) {
	return(PR_TRUE);
    }

    newerbefore = PR_FALSE;
    if ( LL_CMP(notBeforeA, >, notBeforeB) ) {
	newerbefore = PR_TRUE;
    }

    newerafter = PR_FALSE;
    if ( LL_CMP(notAfterA, >, notAfterB) ) {
	newerafter = PR_TRUE;
    }
    
    if ( newerbefore && newerafter ) {
	return(PR_TRUE);
    }
    
    if ( ( !newerbefore ) && ( !newerafter ) ) {
	return(PR_FALSE);
    }

    /* get current UTC time */
    now = PR_Now();

    if ( newerbefore ) {
	/* cert A was issued after cert B, but expires sooner */
	/* if A is expired, then pick B */
	if ( LL_CMP(notAfterA, <, now ) ) {
	    return(PR_FALSE);
	}
	return(PR_TRUE);
    } else {
	/* cert B was issued after cert A, but expires sooner */
	/* if B is expired, then pick A */
	if ( LL_CMP(notAfterB, <, now ) ) {
	    return(PR_TRUE);
	}
	return(PR_FALSE);
    }
}

#define SOFT_DEFAULT_CHUNKSIZE 2048


static SECStatus
nsslowcert_KeyFromIssuerAndSN(PRArenaPool *arena, SECItem *issuer, SECItem *sn,
			SECItem *key)
{
    unsigned int len = sn->len + issuer->len;


    if (arena) {
	key->data = (unsigned char*)PORT_ArenaAlloc(arena, len);
    } else {
	if (len > key->len) {
	    key->data = (unsigned char*)PORT_ArenaAlloc(arena, len);
	}
    }
    if ( !key->data ) {
	goto loser;
    }

    key->len = len;
    /* copy the serialNumber */
    PORT_Memcpy(key->data, sn->data, sn->len);

    /* copy the issuer */
    PORT_Memcpy(&key->data[sn->len], issuer->data, issuer->len);

    return(SECSuccess);

loser:
    return(SECFailure);
}



/*
 * take a DER certificate and decode it into a certificate structure
 */
NSSLOWCERTCertificate *
nsslowcert_DecodeDERCertificate(SECItem *derSignedCert, char *nickname)
{
    NSSLOWCERTCertificate *cert;
    int rv;

    /* allocate the certificate structure */
    cert = nsslowcert_CreateCert();
    
    if ( !cert ) {
	goto loser;
    }
    
	/* point to passed in DER data */
    cert->derCert = *derSignedCert;
    cert->nickname = NULL;
    cert->certKey.data = NULL;
    cert->referenceCount = 1;

    /* decode the certificate info */
    rv = nsslowcert_GetCertFields(cert->derCert.data, cert->derCert.len,
	&cert->derIssuer, &cert->serialNumber, &cert->derSN, &cert->derSubject,
	&cert->validity, &cert->derSubjKeyInfo);

    /* cert->subjectKeyID;	 x509v3 subject key identifier */
    cert->subjectKeyID.data = NULL;
    cert->subjectKeyID.len = 0;
    cert->dbEntry = NULL;
    cert ->trust = NULL;

    /* generate and save the database key for the cert */
    cert->certKey.data = cert->certKeySpace;
    cert->certKey.len = sizeof(cert->certKeySpace);
    rv = nsslowcert_KeyFromIssuerAndSN(NULL, &cert->derIssuer, 
					&cert->serialNumber, &cert->certKey);
    if ( rv ) {
	goto loser;
    }

    /* set the nickname */
    if ( nickname == NULL ) {
	cert->nickname = NULL;
    } else {
	/* copy and install the nickname */
	cert->nickname = pkcs11_copyNickname(nickname,cert->nicknameSpace,
				sizeof(cert->nicknameSpace));
    }

#ifdef FIXME
    /* initialize the subjectKeyID */
    rv = cert_GetKeyID(cert);
    if ( rv != SECSuccess ) {
	goto loser;
    }

    /* set the email address */
    cert->emailAddr = CERT_GetCertificateEmailAddress(cert);
    
#endif
    
    cert->referenceCount = 1;
    
    return(cert);
    
loser:
    if (cert) {
	nsslowcert_DestroyCertificate(cert);
    }
    
    return(0);
}

char *
nsslowcert_FixupEmailAddr(char *emailAddr)
{
    char *retaddr;
    char *str;

    if ( emailAddr == NULL ) {
	return(NULL);
    }
    
    /* copy the string */
    str = retaddr = PORT_Strdup(emailAddr);
    if ( str == NULL ) {
	return(NULL);
    }
    
    /* make it lower case */
    while ( *str ) {
	*str = tolower( *str );
	str++;
    }
    
    return(retaddr);
}


/*
 * Generate a database key, based on serial number and issuer, from a
 * DER certificate.
 */
SECStatus
nsslowcert_KeyFromDERCert(PRArenaPool *arena, SECItem *derCert, SECItem *key)
{
    int rv;
    NSSLOWCERTCertKey certkey;

    PORT_Memset(&certkey, 0, sizeof(NSSLOWCERTCertKey));    

    rv = nsslowcert_GetCertFields(derCert->data, derCert->len,
	&certkey.derIssuer, &certkey.serialNumber, NULL, NULL, NULL, NULL);

    if ( rv ) {
	goto loser;
    }

    return(nsslowcert_KeyFromIssuerAndSN(arena, &certkey.derIssuer,
				   &certkey.serialNumber, key));
loser:
    return(SECFailure);
}

NSSLOWKEYPublicKey *
nsslowcert_ExtractPublicKey(NSSLOWCERTCertificate *cert)
{
    NSSLOWCERTSubjectPublicKeyInfo spki;
    NSSLOWKEYPublicKey *pubk;
    SECItem os;
    SECStatus rv;
    PRArenaPool *arena;
    SECOidTag tag;

    arena = PORT_NewArena (DER_DEFAULT_CHUNKSIZE);
    if (arena == NULL)
        return NULL;

    pubk = (NSSLOWKEYPublicKey *) 
		PORT_ArenaZAlloc(arena, sizeof(NSSLOWKEYPublicKey));
    if (pubk == NULL) {
        PORT_FreeArena (arena, PR_FALSE);
        return NULL;
    }

    pubk->arena = arena;
    PORT_Memset(&spki,0,sizeof(spki));

    /* we haven't bothered decoding the spki struct yet, do it now */
    rv = SEC_ASN1DecodeItem(arena, &spki, 
		nsslowcert_SubjectPublicKeyInfoTemplate, &cert->derSubjKeyInfo);
    if (rv != SECSuccess) {
 	PORT_FreeArena (arena, PR_FALSE);
 	return NULL;
    }

    /* Convert bit string length from bits to bytes */
    os = spki.subjectPublicKey;
    DER_ConvertBitString (&os);

    tag = SECOID_GetAlgorithmTag(&spki.algorithm);
    switch ( tag ) {
      case SEC_OID_X500_RSA_ENCRYPTION:
      case SEC_OID_PKCS1_RSA_ENCRYPTION:
        pubk->keyType = NSSLOWKEYRSAKey;
        prepare_low_rsa_pub_key_for_asn1(pubk);
        rv = SEC_ASN1DecodeItem(arena, pubk, 
				nsslowcert_RSAPublicKeyTemplate, &os);
        if (rv == SECSuccess)
            return pubk;
        break;
      case SEC_OID_ANSIX9_DSA_SIGNATURE:
        pubk->keyType = NSSLOWKEYDSAKey;
        prepare_low_dsa_pub_key_for_asn1(pubk);
        rv = SEC_ASN1DecodeItem(arena, pubk,
				 nsslowcert_DSAPublicKeyTemplate, &os);
        if (rv == SECSuccess) return pubk;
        break;
      case SEC_OID_X942_DIFFIE_HELMAN_KEY:
        pubk->keyType = NSSLOWKEYDHKey;
        prepare_low_dh_pub_key_for_asn1(pubk);
        rv = SEC_ASN1DecodeItem(arena, pubk,
				 nsslowcert_DHPublicKeyTemplate, &os);
        if (rv == SECSuccess) return pubk;
        break;
      default:
        rv = SECFailure;
        break;
    }

    nsslowkey_DestroyPublicKey (pubk);
    return NULL;
}

