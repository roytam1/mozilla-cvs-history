/*
 * Verification stuff.
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
 *
 * $Id$
 */

#include <stdio.h>
#include "cryptohi.h"
#include "sechash.h"
#include "keyhi.h"
#include "secasn1.h"
#include "secoid.h"
#include "pk11func.h"
#include "secdig.h"
#include "secerr.h"

/*
** Decrypt signature block using public key (in place)
** XXX this is assuming that the signature algorithm has WITH_RSA_ENCRYPTION
*/
static SECStatus
DecryptSigBlock(int *tagp, unsigned char *digest, SECKEYPublicKey *key,
		SECItem *sig, char *wincx)
{
    SGNDigestInfo *di   = NULL;
    unsigned char *dsig = NULL;
    unsigned char *buf  = NULL;
    SECStatus      rv;
    SECOidTag      tag;
    SECItem        it;

    if (key == NULL) goto loser;

    it.len  = SECKEY_PublicKeyStrength(key);
    if (!it.len) goto loser;
    it.data = buf = (unsigned char *)PORT_Alloc(it.len);
    if (!buf) goto loser;

    /* Decrypt signature block */
    dsig = (unsigned char*) PORT_Alloc(sig->len);
    if (dsig == NULL) goto loser;

    /* decrypt the block */
    rv = PK11_VerifyRecover(key, sig, &it, wincx);
    if (rv != SECSuccess) goto loser;

    di = SGN_DecodeDigestInfo(&it);
    if (di == NULL) goto sigloser;

    /*
    ** Finally we have the digest info; now we can extract the algorithm
    ** ID and the signature block
    */
    tag = SECOID_GetAlgorithmTag(&di->digestAlgorithm);
    /* XXX Check that tag is an appropriate algorithm? */
    if (di->digest.len > 32) {
	PORT_SetError(SEC_ERROR_OUTPUT_LEN);
	goto loser;
    }
    PORT_Memcpy(digest, di->digest.data, di->digest.len);
    *tagp = tag;
    goto done;

  sigloser:
    PORT_SetError(SEC_ERROR_BAD_SIGNATURE);

  loser:
    rv = SECFailure;

  done:
    if (di   != NULL) SGN_DestroyDigestInfo(di);
    if (dsig != NULL) PORT_Free(dsig);
    if (buf  != NULL) PORT_Free(buf);
    
    return rv;
}

typedef enum { VFY_RSA, VFY_DSA} VerifyType;

struct VFYContextStr {
    SECOidTag alg;
    VerifyType type;
    SECKEYPublicKey *key;
    /* digest holds the full dsa signature... 40 bytes */
    unsigned char digest[DSA_SIGNATURE_LEN];
    void * wincx;
    void *hashcx;
    const SECHashObject *hashobj;
    SECOidTag sigAlg;
    PRBool hasSignature;
};

/*
 * decode the DSA signature from it's DER wrapping.
 */
static SECStatus
decodeDSASignature(SECOidTag algid, SECItem *sig, unsigned char *digest) {
    SECItem *dsasig = NULL;
    SECStatus rv=SECSuccess;

    /* if this is a DER encoded signature, decode it first */
    if ((algid == SEC_OID_ANSIX9_DSA_SIGNATURE_WITH_SHA1_DIGEST) ||
	(algid == SEC_OID_BOGUS_DSA_SIGNATURE_WITH_SHA1_DIGEST) ||
	(algid == SEC_OID_ANSIX9_DSA_SIGNATURE)) {
	dsasig = DSAU_DecodeDerSig(sig);
	if ((dsasig == NULL) || (dsasig->len != DSA_SIGNATURE_LEN)) {
	    rv = SECFailure;
	    goto loser;
	}
	PORT_Memcpy(digest, dsasig->data, dsasig->len);
    } else {
	if (sig->len != DSA_SIGNATURE_LEN) {
	    rv = SECFailure;
	    goto loser;
	}
	PORT_Memcpy(digest, sig->data, sig->len);
    }

loser:
    if (dsasig != NULL)
	SECITEM_FreeItem(dsasig, PR_TRUE);
    return rv;
     
}
/*
 * Pulls the hash algorithm, signing algorithm, and key type out of a
 * composite algorithm.
 *
 * alg: the composite algorithm to dissect.
 * hashalg: address of a SECOidTag which will be set with the hash algorithm.
 * signalg: address of a SECOidTag which will be set with the signing alg.
 * keyType: address of a KeyType which will be set with the key type.
 * Returns: SECSuccess if the algorithm was acceptable, SECFailure if the
 *	algorithm was not found or was not a signing algorithm.
 */
static SECStatus
decodeSigAlg(SECOidTag alg, SECOidTag *hashalg)
{
    PR_ASSERT(hashalg!=NULL);

    switch (alg) {
      /* We probably shouldn't be generating MD2 signatures either */
      case SEC_OID_PKCS1_MD2_WITH_RSA_ENCRYPTION:
        *hashalg = SEC_OID_MD2;
	return SECSuccess;
      case SEC_OID_PKCS1_MD5_WITH_RSA_ENCRYPTION:
        *hashalg = SEC_OID_MD5;
	return SECSuccess;
      case SEC_OID_PKCS1_SHA1_WITH_RSA_ENCRYPTION:
      case SEC_OID_ISO_SHA_WITH_RSA_SIGNATURE:
        *hashalg = SEC_OID_SHA1;
	return SECSuccess;
      /* what about normal DSA? */
      case SEC_OID_ANSIX9_DSA_SIGNATURE_WITH_SHA1_DIGEST:
      case SEC_OID_BOGUS_DSA_SIGNATURE_WITH_SHA1_DIGEST:
        *hashalg = SEC_OID_SHA1;
	return SECSuccess;
      case SEC_OID_MISSI_DSS:
      case SEC_OID_MISSI_KEA_DSS:
      case SEC_OID_MISSI_KEA_DSS_OLD:
      case SEC_OID_MISSI_DSS_OLD:
        *hashalg = SEC_OID_SHA1;
	return SECSuccess;
      /* we don't implement MD4 hashes */
      case SEC_OID_PKCS1_MD4_WITH_RSA_ENCRYPTION:
      default:
        return SECFailure;
    }
    PR_ASSERT(PR_FALSE); /* shouldn't get here */
}

VFYContext *
VFY_CreateContext(SECKEYPublicKey *key, SECItem *sig, SECOidTag algid,
		  void *wincx)
{
    VFYContext *cx;
    SECStatus rv;

    cx = (VFYContext*) PORT_ZAlloc(sizeof(VFYContext));
    if (cx) {
        cx->wincx = wincx;
	cx->hasSignature = (sig != NULL);
	cx->sigAlg = algid;
	rv = SECSuccess;
	switch (key->keyType) {
	  case rsaKey:
	    cx->type = VFY_RSA;
	    cx->key = SECKEY_CopyPublicKey(key); /* extra safety precautions */
	    if (sig) {
		int hashid;
	    	rv = DecryptSigBlock(&hashid, &cx->digest[0], 
						key, sig, (char*)wincx);
		cx->alg = hashid;
	    } else {
		rv = decodeSigAlg(algid,&cx->alg);
	    }
	    break;
	  case fortezzaKey:
	  case dsaKey:
	    cx->type = VFY_DSA;
	    cx->alg = SEC_OID_SHA1;
	    cx->key = SECKEY_CopyPublicKey(key);
	    if (sig) {
	    	rv = decodeDSASignature(algid,sig,&cx->digest[0]);
	    }
	    break;
	  default:
	    rv = SECFailure;
	    break;
	}
	if (rv) goto loser;
	switch (cx->alg) {
	  case SEC_OID_MD2:
	  case SEC_OID_MD5:
	  case SEC_OID_SHA1:
	    break;
	  default:
	    PORT_SetError(SEC_ERROR_INVALID_ALGORITHM);
	    goto loser;
	}
    }
    return cx;

  loser:
    VFY_DestroyContext(cx, PR_TRUE);
    return 0;
}

void
VFY_DestroyContext(VFYContext *cx, PRBool freeit)
{
    if (cx) {
	if (cx->hashcx != NULL) {
	    (*cx->hashobj->destroy)(cx->hashcx, PR_TRUE);
	    cx->hashcx = NULL;
	}
	if (cx->key) {
	    SECKEY_DestroyPublicKey(cx->key);
	}
	if (freeit) {
	    PORT_ZFree(cx, sizeof(VFYContext));
	}
    }
}

SECStatus
VFY_Begin(VFYContext *cx)
{
    if (cx->hashcx != NULL) {
	(*cx->hashobj->destroy)(cx->hashcx, PR_TRUE);
	cx->hashcx = NULL;
    }

    switch (cx->alg) {
      case SEC_OID_MD2:
	cx->hashobj = &SECHashObjects[HASH_AlgMD2];
	break;
      case SEC_OID_MD5:
	cx->hashobj = &SECHashObjects[HASH_AlgMD5];
	break;
      case SEC_OID_SHA1:
	cx->hashobj = &SECHashObjects[HASH_AlgSHA1];
	break;
      default:
	PORT_SetError(SEC_ERROR_INVALID_ALGORITHM);
	return SECFailure;
    }

    cx->hashcx = (*cx->hashobj->create)();
    if (cx->hashcx == NULL)
	return SECFailure;

    (*cx->hashobj->begin)(cx->hashcx);
    return SECSuccess;
}

SECStatus
VFY_Update(VFYContext *cx, unsigned char *input, unsigned inputLen)
{
    if (cx->hashcx == NULL) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	return SECFailure;
    }
    (*cx->hashobj->update)(cx->hashcx, input, inputLen);
    return SECSuccess;
}

SECStatus
VFY_EndWithSignature(VFYContext *cx, SECItem *sig)
{
    unsigned char final[32];
    unsigned part;
    SECItem hash,dsasig;
    unsigned char *digest;
    SECStatus rv;

    if ((cx->hasSignature == PR_FALSE) && (sig == NULL)) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	return SECFailure;
    }

    if (cx->hashcx == NULL) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	return SECFailure;
    }
    (*cx->hashobj->end)(cx->hashcx, final, &part, sizeof(final));
    switch (cx->type) {
      case VFY_DSA:
	if (sig) {
	    rv = decodeDSASignature(cx->sigAlg,sig,&cx->digest[0]);
	    if (rv != SECSuccess) {
		PORT_SetError(SEC_ERROR_BAD_SIGNATURE);
		return SECFailure;
	    }
	} 
	dsasig.data = cx->digest;
	dsasig.len = DSA_SIGNATURE_LEN; /* magic size of dsa signature */
	hash.data = final;
	hash.len = part;
	if (PK11_Verify(cx->key,&dsasig,&hash,cx->wincx) != SECSuccess) {
		PORT_SetError(SEC_ERROR_BAD_SIGNATURE);
		return SECFailure;
	}
	break;
      case VFY_RSA:
	if (sig) {
	    int hashid;
	    rv = DecryptSigBlock(&hashid, &cx->digest[0], 
					    cx->key, sig, (char*)cx->wincx);
	    if ((rv != SECSuccess) || (hashid != cx->alg)) {
		PORT_SetError(SEC_ERROR_BAD_SIGNATURE);
		return SECFailure;
	    }
	}
	if (PORT_Memcmp(final, cx->digest, part)) {
	    PORT_SetError(SEC_ERROR_BAD_SIGNATURE);
	    return SECFailure;
	}
	break;
      default:
	PORT_SetError(SEC_ERROR_BAD_SIGNATURE);
	return SECFailure; /* shouldn't happen */
    }
    return SECSuccess;
}

SECStatus
VFY_End(VFYContext *cx)
{
    return VFY_EndWithSignature(cx,NULL);
}

/************************************************************************/
/*
 * Verify that a previously-computed digest matches a signature.
 * XXX This should take a parameter that specifies the digest algorithm,
 * and we should compare that the algorithm found in the DigestInfo
 * matches it!
 */
SECStatus
VFY_VerifyDigest(SECItem *digest, SECKEYPublicKey *key, SECItem *sig,
		 SECOidTag algid, void *wincx)
{
    SECStatus rv;
    VFYContext *cx;
    SECItem dsasig;

    rv = SECFailure;

    cx = VFY_CreateContext(key, sig, algid, wincx);
    if (cx != NULL) {
	switch (key->keyType) {
	case rsaKey:
	    if (PORT_Memcmp(digest->data, cx->digest, digest->len)) {
		PORT_SetError(SEC_ERROR_BAD_SIGNATURE);
	    } else {
		rv = SECSuccess;
	    }
	    break;
	case fortezzaKey:
	case dsaKey:
	    dsasig.data = &cx->digest[0];
	    dsasig.len = DSA_SIGNATURE_LEN; /* magic size of dsa signature */
	    if (PK11_Verify(cx->key, &dsasig, digest, cx->wincx) != SECSuccess) {
		PORT_SetError(SEC_ERROR_BAD_SIGNATURE);
	    } else {
		rv = SECSuccess;
	    }
	    break;
	default:
	    break;
	}
	VFY_DestroyContext(cx, PR_TRUE);
    }
    return rv;
}

SECStatus
VFY_VerifyData(unsigned char *buf, int len, SECKEYPublicKey *key,
	       SECItem *sig, SECOidTag algid, void *wincx)
{
    SECStatus rv;
    VFYContext *cx;

    cx = VFY_CreateContext(key, sig, algid, wincx);
    if (cx == NULL)
	return SECFailure;

    rv = VFY_Begin(cx);
    if (rv == SECSuccess) {
	rv = VFY_Update(cx, buf, len);
	if (rv == SECSuccess)
	    rv = VFY_End(cx);
    }

    VFY_DestroyContext(cx, PR_TRUE);
    return rv;
}

SECStatus
SEC_VerifyFile(FILE *input, SECKEYPublicKey *key, SECItem *sig,
	       SECOidTag algid, void *wincx)
{
    unsigned char buf[1024];
    SECStatus rv;
    int nb;
    VFYContext *cx;

    cx = VFY_CreateContext(key, sig, algid, wincx);
    if (cx == NULL)
	rv = SECFailure;

    rv = VFY_Begin(cx);
    if (rv == SECSuccess) {
	/*
	 * Now feed the contents of the input file into the digest algorithm,
	 * one chunk at a time, until we have exhausted the input.
	 */
	for (;;) {
	    if (feof(input))
		break;
	    nb = fread(buf, 1, sizeof(buf), input);
	    if (nb == 0) {
		if (ferror(input)) {
		    PORT_SetError(SEC_ERROR_IO);
		    VFY_DestroyContext(cx, PR_TRUE);
		    return SECFailure;
		}
		break;
	    }
	    rv = VFY_Update(cx, buf, nb);
	    if (rv != SECSuccess)
		goto loser;
	}
    }

    /* Verify the digest */
    rv = VFY_End(cx);
    /* FALL THROUGH */

  loser:
    VFY_DestroyContext(cx, PR_TRUE);
    return rv;
}
