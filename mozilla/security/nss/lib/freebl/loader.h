/*
 * loader.h - load platform dependent DSO containing freebl implementation.
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
 * Copyright (C) 2000 Netscape Communications Corporation.  All
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

#ifndef _LOADER_H_
#define _LOADER_H_ 1

#include "blapi.h"

#define FREEBL_VERSION 0x0302

struct FREEBLVectorStr {

  unsigned short length;  /* of this struct in bytes */
  unsigned short version; /* of this struct. */

  RSAPrivateKey * (* p_RSA_NewKey)(int         keySizeInBits,
				 SECItem *   publicExponent);

  SECStatus (* p_RSA_PublicKeyOp) (RSAPublicKey *   key,
				 unsigned char *  output,
				 const unsigned char *  input);

  SECStatus (* p_RSA_PrivateKeyOp)(RSAPrivateKey *  key,
				  unsigned char *  output,
				  const unsigned char *  input);

  SECStatus (* p_DSA_NewKey)(const PQGParams *    params, 
		            DSAPrivateKey **      privKey);

  SECStatus (* p_DSA_SignDigest)(DSAPrivateKey *   key,
				SECItem *         signature,
				const SECItem *   digest);

  SECStatus (* p_DSA_VerifyDigest)(DSAPublicKey *  key,
				  const SECItem *  signature,
				  const SECItem *  digest);

  SECStatus (* p_DSA_NewKeyFromSeed)(const PQGParams *params, 
				     const unsigned char * seed,
                                     DSAPrivateKey **privKey);

  SECStatus (* p_DSA_SignDigestWithSeed)(DSAPrivateKey * key,
				        SECItem *             signature,
				        const SECItem *       digest,
				        const unsigned char * seed);

 SECStatus (* p_DH_GenParam)(int primeLen, DHParams ** params);

 SECStatus (* p_DH_NewKey)(DHParams *           params, 
                           DHPrivateKey **	privKey);

 SECStatus (* p_DH_Derive)(SECItem *    publicValue, 
		           SECItem *    prime, 
			   SECItem *    privateValue, 
			   SECItem *    derivedSecret,
			   unsigned int maxOutBytes);

 SECStatus (* p_KEA_Derive)(SECItem *prime, 
                            SECItem *public1, 
                            SECItem *public2, 
			    SECItem *private1, 
			    SECItem *private2,
			    SECItem *derivedSecret);

 PRBool (* p_KEA_Verify)(SECItem *Y, SECItem *prime, SECItem *subPrime);

 RC4Context * (* p_RC4_CreateContext)(unsigned char *key, int len);

 void (* p_RC4_DestroyContext)(RC4Context *cx, PRBool freeit);

 SECStatus (* p_RC4_Encrypt)(RC4Context *cx, unsigned char *output,
			    unsigned int *outputLen, unsigned int maxOutputLen,
			    const unsigned char *input, unsigned int inputLen);

 SECStatus (* p_RC4_Decrypt)(RC4Context *cx, unsigned char *output,
			    unsigned int *outputLen, unsigned int maxOutputLen,
			    const unsigned char *input, unsigned int inputLen);

 RC2Context * (* p_RC2_CreateContext)(unsigned char *key, unsigned int len,
		     unsigned char *iv, int mode, unsigned effectiveKeyLen);

 void (* p_RC2_DestroyContext)(RC2Context *cx, PRBool freeit);

 SECStatus (* p_RC2_Encrypt)(RC2Context *cx, unsigned char *output,
			    unsigned int *outputLen, unsigned int maxOutputLen,
			    const unsigned char *input, unsigned int inputLen);

 SECStatus (* p_RC2_Decrypt)(RC2Context *cx, unsigned char *output,
			    unsigned int *outputLen, unsigned int maxOutputLen,
			    const unsigned char *input, unsigned int inputLen);

 RC5Context *(* p_RC5_CreateContext)(SECItem *key, unsigned int rounds,
                     unsigned int wordSize, unsigned char *iv, int mode);

 void (* p_RC5_DestroyContext)(RC5Context *cx, PRBool freeit);

 SECStatus (* p_RC5_Encrypt)(RC5Context *cx, unsigned char *output,
                            unsigned int *outputLen, unsigned int maxOutputLen,
                            const unsigned char *input, unsigned int inputLen);

 SECStatus (* p_RC5_Decrypt)(RC5Context *cx, unsigned char *output,
                            unsigned int *outputLen, unsigned int maxOutputLen,
                            const unsigned char *input, unsigned int inputLen);

 DESContext *(* p_DES_CreateContext)(unsigned char *key, unsigned char *iv,
				     int mode, PRBool encrypt);

 void (* p_DES_DestroyContext)(DESContext *cx, PRBool freeit);

 SECStatus (* p_DES_Encrypt)(DESContext *cx, unsigned char *output,
			    unsigned int *outputLen, unsigned int maxOutputLen,
			    const unsigned char *input, unsigned int inputLen);

 SECStatus (* p_DES_Decrypt)(DESContext *cx, unsigned char *output,
			    unsigned int *outputLen, unsigned int maxOutputLen,
			    const unsigned char *input, unsigned int inputLen);

 AESContext * (* p_AES_CreateContext)(unsigned char *key, unsigned char *iv, 
			    int mode, int encrypt, unsigned int keylen, 
			    unsigned int blocklen);

 void (* p_AES_DestroyContext)(AESContext *cx, PRBool freeit);

 SECStatus (* p_AES_Encrypt)(AESContext *cx, unsigned char *output,
			    unsigned int *outputLen, unsigned int maxOutputLen,
			    const unsigned char *input, unsigned int inputLen);

 SECStatus (* p_AES_Decrypt)(AESContext *cx, unsigned char *output,
			    unsigned int *outputLen, unsigned int maxOutputLen,
			    const unsigned char *input, unsigned int inputLen);

 SECStatus (* p_MD5_Hash)(unsigned char *dest, const char *src);

 SECStatus (* p_MD5_HashBuf)(unsigned char *dest, const unsigned char *src,
			     uint32 src_length);

 MD5Context *(* p_MD5_NewContext)(void);

 void (* p_MD5_DestroyContext)(MD5Context *cx, PRBool freeit);

 void (* p_MD5_Begin)(MD5Context *cx);

 void (* p_MD5_Update)(MD5Context *cx,
		       const unsigned char *input, unsigned int inputLen);

 void (* p_MD5_End)(MD5Context *cx, unsigned char *digest,
		    unsigned int *digestLen, unsigned int maxDigestLen);

 unsigned int (* p_MD5_FlattenSize)(MD5Context *cx);

 SECStatus (* p_MD5_Flatten)(MD5Context *cx,unsigned char *space);

 MD5Context * (* p_MD5_Resurrect)(unsigned char *space, void *arg);

 void (* p_MD5_TraceState)(MD5Context *cx);

 SECStatus (* p_MD2_Hash)(unsigned char *dest, const char *src);

 MD2Context *(* p_MD2_NewContext)(void);

 void (* p_MD2_DestroyContext)(MD2Context *cx, PRBool freeit);

 void (* p_MD2_Begin)(MD2Context *cx);

 void (* p_MD2_Update)(MD2Context *cx,
		       const unsigned char *input, unsigned int inputLen);

 void (* p_MD2_End)(MD2Context *cx, unsigned char *digest,
		    unsigned int *digestLen, unsigned int maxDigestLen);

 unsigned int (* p_MD2_FlattenSize)(MD2Context *cx);

 SECStatus (* p_MD2_Flatten)(MD2Context *cx,unsigned char *space);

 MD2Context * (* p_MD2_Resurrect)(unsigned char *space, void *arg);

 SECStatus (* p_SHA1_Hash)(unsigned char *dest, const char *src);

 SECStatus (* p_SHA1_HashBuf)(unsigned char *dest, const unsigned char *src,
			      uint32 src_length);

 SHA1Context *(* p_SHA1_NewContext)(void);

 void (* p_SHA1_DestroyContext)(SHA1Context *cx, PRBool freeit);

 void (* p_SHA1_Begin)(SHA1Context *cx);

 void (* p_SHA1_Update)(SHA1Context *cx, const unsigned char *input,
			unsigned int inputLen);

 void (* p_SHA1_End)(SHA1Context *cx, unsigned char *digest,
		     unsigned int *digestLen, unsigned int maxDigestLen);

 void (* p_SHA1_TraceState)(SHA1Context *cx);

 unsigned int (* p_SHA1_FlattenSize)(SHA1Context *cx);

 SECStatus (* p_SHA1_Flatten)(SHA1Context *cx,unsigned char *space);

 SHA1Context * (* p_SHA1_Resurrect)(unsigned char *space, void *arg);

 SECStatus (* p_RNG_RNGInit)(void);

 SECStatus (* p_RNG_RandomUpdate)(const void *data, size_t bytes);

 SECStatus (* p_RNG_GenerateGlobalRandomBytes)(void *dest, size_t len);

 void  (* p_RNG_RNGShutdown)(void);

 SECStatus (* p_PQG_ParamGen)(unsigned int j, PQGParams **pParams,  
	                      PQGVerify **pVfy);    

 SECStatus (* p_PQG_ParamGenSeedLen)( unsigned int j, unsigned int seedBytes, 
                                     PQGParams **pParams, PQGVerify **pVfy); 

 SECStatus (* p_PQG_VerifyParams)(const PQGParams *params, 
                                  const PQGVerify *vfy, SECStatus *result);

  SECStatus (* p_RSA_PrivateKeyOpDoubleChecked)(RSAPrivateKey *key,
                              unsigned char *output,
                              const unsigned char *input);

  SECStatus (* p_RSA_PrivateKeyCheck)(RSAPrivateKey *key);

  void (* p_BL_Cleanup)(void);

};

typedef struct FREEBLVectorStr FREEBLVector;

SEC_BEGIN_PROTOS

typedef const FREEBLVector * FREEBLGetVectorFn(void);

extern FREEBLGetVectorFn FREEBL_GetVector;

SEC_END_PROTOS

#endif
