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
 * Copyright (C) 2001 Netscape Communications Corporation.  All 
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

#include <string.h>

#include "ssl.h"
#include "sslimpl.h"
#include "sslproto.h"

SECStatus 
SSL_GetChannelInfo(PRFileDesc *fd, SSLChannelInfo *info, PRUintn len)
{
    sslSocket *      ss;
    SSLChannelInfo   inf;
    sslSessionID *   sid;

    if (!info || len < sizeof inf.length) { 
    	return SECSuccess;
    }

    ss = ssl_FindSocket(fd);
    if (!ss) {
	SSL_DBG(("%d: SSL[%d]: bad socket in SSL_GetChannelInfo",
		 SSL_GETPID(), fd));
	return SECFailure;
    }

    memset(&inf, 0, sizeof inf);
    inf.length = PR_MIN(sizeof inf, len);

    if (ss->useSecurity && ss->firstHsDone) {
        sid = ss->sec.ci.sid;
	inf.protocolVersion  = ss->version;
	inf.authKeyBits      = ss->sec.authKeyBits;
	inf.keaKeyBits       = ss->sec.keaKeyBits;
	if (ss->version < SSL_LIBRARY_VERSION_3_0) { /* SSL2 */
	    inf.cipherSuite      = ss->sec.cipherType | 0xff00;
	} else if (ss->ssl3) { 		/* SSL3 and TLS */

	    /* XXX  These should come from crSpec */
	    inf.cipherSuite      = ss->ssl3->hs.cipher_suite;
#if 0
	    /* misc */
	    inf.isFIPS =  (inf.symCipher == ssl_calg_des   || inf.symCipher == ssl_calg_3des)
		       && (inf.macAlgorithm == ssl_mac_sha || inf.macAlgorithm == ssl_hmac_sha)
		       && (inf.protocolVersion > SSL_LIBRARY_VERSION_3_0 ||
		           inf.cipherSuite >= 0xfef0);
#endif
	}
	if (sid) {
	    inf.creationTime   = sid->creationTime;
	    inf.lastAccessTime = sid->lastAccessTime;
	    inf.expirationTime = sid->expirationTime;
	    if (ss->version < SSL_LIBRARY_VERSION_3_0) { /* SSL2 */
	        inf.sessionIDLength = SSL2_SESSIONID_BYTES;
		memcpy(inf.sessionID, sid->u.ssl2.sessionID, SSL2_SESSIONID_BYTES);
	    } else {
		unsigned int sidLen = sid->u.ssl3.sessionIDLength;
	        sidLen = PR_MIN(sidLen, sizeof inf.sessionID);
	        inf.sessionIDLength = sidLen;
		memcpy(inf.sessionID, sid->u.ssl3.sessionID, sidLen);
	    }
	}
    }

    memcpy(info, &inf, inf.length);

    return SECSuccess;
}

#define ssl_kea_kea ssl_kea_fortezza
#define ssl_calg_sj ssl_calg_fortezza

#define CS(x) x, #x
#define CK(x) x | 0xff00, #x

#define S_DSA   "DSA", ssl_auth_dsa
#define S_RSA	"RSA", ssl_auth_rsa
#define S_KEA   "KEA", ssl_auth_kea

#define K_DHE	"DHE", ssl_kea_dh
#define K_RSA	"RSA", ssl_kea_rsa
#define K_KEA	"KEA", ssl_kea_kea

#define C_AES	"AES", ssl_calg_aes
#define C_RC4	"RC4", ssl_calg_rc4
#define C_RC2	"RC2", ssl_calg_rc2
#define C_DES	"DES", ssl_calg_des
#define C_3DES	"3DES", ssl_calg_3des
#define C_NULL  "NULL", ssl_calg_null
#define C_SJ 	"SKIPJACK", ssl_calg_sj

#define B_256	256, 256, 256
#define B_128	128, 128, 128
#define B_3DES  192, 156, 112
#define B_SJ     96,  80,  80
#define B_DES    64,  56,  56
#define B_56    128,  56,  56
#define B_40    128,  40,  40
#define B_0  	  0,   0,   0

#define M_SHA	"SHA1", ssl_mac_sha, 160
#define M_MD5	"MD5",  ssl_mac_md5, 128

static const SSLCipherSuiteInfo suiteInfo[] = {
/* <------ Cipher suite --------------------> <auth> <KEA>  <bulk cipher> <MAC> <FIPS> */
{0,CS(TLS_DHE_RSA_WITH_AES_256_CBC_SHA),      S_RSA, K_DHE, C_AES, B_256, M_SHA, 0, 0, 0, },
{0,CS(TLS_DHE_DSS_WITH_AES_256_CBC_SHA),      S_DSA, K_DHE, C_AES, B_256, M_SHA, 0, 0, 0, },
{0,CS(TLS_RSA_WITH_AES_256_CBC_SHA),          S_RSA, K_RSA, C_AES, B_256, M_SHA, 0, 0, 0, },

{0,CS(SSL_FORTEZZA_DMS_WITH_RC4_128_SHA),     S_KEA, K_KEA, C_RC4, B_128, M_SHA, 0, 0, 0, },
{0,CS(TLS_DHE_DSS_WITH_RC4_128_SHA),          S_DSA, K_DHE, C_RC4, B_128, M_SHA, 0, 0, 0, },
{0,CS(TLS_DHE_RSA_WITH_AES_128_CBC_SHA),      S_RSA, K_DHE, C_AES, B_128, M_SHA, 0, 0, 0, },
{0,CS(TLS_DHE_DSS_WITH_AES_128_CBC_SHA),      S_DSA, K_DHE, C_AES, B_128, M_SHA, 0, 0, 0, },
{0,CS(SSL_RSA_WITH_RC4_128_MD5),              S_RSA, K_RSA, C_RC4, B_128, M_MD5, 0, 0, 0, },
{0,CS(SSL_RSA_WITH_RC4_128_SHA),              S_RSA, K_RSA, C_RC4, B_128, M_SHA, 0, 0, 0, },
{0,CS(TLS_RSA_WITH_AES_128_CBC_SHA),          S_RSA, K_RSA, C_AES, B_128, M_SHA, 0, 0, 0, },

{0,CS(SSL_DHE_RSA_WITH_3DES_EDE_CBC_SHA),     S_RSA, K_DHE, C_3DES,B_3DES,M_SHA, 0, 0, 0, },
{0,CS(SSL_DHE_DSS_WITH_3DES_EDE_CBC_SHA),     S_DSA, K_DHE, C_3DES,B_3DES,M_SHA, 0, 0, 0, },
{0,CS(SSL_RSA_FIPS_WITH_3DES_EDE_CBC_SHA),    S_RSA, K_RSA, C_3DES,B_3DES,M_SHA, 1, 0, 1, },
{0,CS(SSL_RSA_WITH_3DES_EDE_CBC_SHA),         S_RSA, K_RSA, C_3DES,B_3DES,M_SHA, 1, 0, 0, },

{0,CS(SSL_FORTEZZA_DMS_WITH_FORTEZZA_CBC_SHA),S_KEA, K_KEA, C_SJ,  B_SJ,  M_SHA, 1, 0, 0, },
{0,CS(SSL_DHE_RSA_WITH_DES_CBC_SHA),          S_RSA, K_DHE, C_DES, B_DES, M_SHA, 0, 0, 0, },
{0,CS(SSL_DHE_DSS_WITH_DES_CBC_SHA),          S_DSA, K_DHE, C_DES, B_DES, M_SHA, 0, 0, 0, },
{0,CS(SSL_RSA_FIPS_WITH_DES_CBC_SHA),         S_RSA, K_RSA, C_DES, B_DES, M_SHA, 1, 0, 1, },
{0,CS(SSL_RSA_WITH_DES_CBC_SHA),              S_RSA, K_RSA, C_DES, B_DES, M_SHA, 1, 0, 0, },

{0,CS(TLS_RSA_EXPORT1024_WITH_RC4_56_SHA),    S_RSA, K_RSA, C_RC4, B_56,  M_SHA, 0, 1, 0, },
{0,CS(TLS_RSA_EXPORT1024_WITH_DES_CBC_SHA),   S_RSA, K_RSA, C_DES, B_DES, M_SHA, 1, 1, 0, },
{0,CS(SSL_RSA_EXPORT_WITH_RC4_40_MD5),        S_RSA, K_RSA, C_RC4, B_40,  M_MD5, 0, 1, 0, },
{0,CS(SSL_RSA_EXPORT_WITH_RC2_CBC_40_MD5),    S_RSA, K_RSA, C_RC2, B_40,  M_MD5, 0, 1, 0, },
{0,CS(SSL_FORTEZZA_DMS_WITH_NULL_SHA),        S_KEA, K_KEA, C_NULL,B_0,   M_SHA, 0, 1, 0, },
{0,CS(SSL_RSA_WITH_NULL_SHA),                 S_RSA, K_RSA, C_NULL,B_0,   M_SHA, 0, 1, 0, },
{0,CS(SSL_RSA_WITH_NULL_MD5),                 S_RSA, K_RSA, C_NULL,B_0,   M_MD5, 0, 1, 0, },

/* SSL 2 table */
{0,CK(SSL_CK_RC4_128_WITH_MD5),               S_RSA, K_RSA, C_RC4, B_128, M_MD5, 0, 0, 0, },
{0,CK(SSL_CK_RC2_128_CBC_WITH_MD5),           S_RSA, K_RSA, C_RC2, B_128, M_MD5, 0, 0, 0, },
{0,CK(SSL_CK_DES_192_EDE3_CBC_WITH_MD5),      S_RSA, K_RSA, C_3DES,B_3DES,M_MD5, 0, 0, 0, },
{0,CK(SSL_CK_DES_64_CBC_WITH_MD5),            S_RSA, K_RSA, C_DES, B_DES, M_MD5, 0, 0, 0, },
{0,CK(SSL_CK_RC4_128_EXPORT40_WITH_MD5),      S_RSA, K_RSA, C_RC4, B_40,  M_MD5, 0, 1, 0, },
{0,CK(SSL_CK_RC2_128_CBC_EXPORT40_WITH_MD5),  S_RSA, K_RSA, C_RC2, B_40,  M_MD5, 0, 1, 0, }
};

#define NUM_SUITEINFOS ((sizeof suiteInfo) / (sizeof suiteInfo[0]))


SECStatus SSL_GetCipherSuiteInfo(PRUint16 cipherSuite, 
                                 SSLCipherSuiteInfo *info, PRUintn len)
{
    unsigned int i;

    len = PR_MIN(len, sizeof suiteInfo[0]);
    if (!info || len < sizeof suiteInfo[0].length) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
    	return SECFailure;
    }
    for (i = 0; i < NUM_SUITEINFOS; i++) {
    	if (suiteInfo[i].cipherSuite == cipherSuite) {
	    memcpy(info, &suiteInfo[i], len);
	    info->length = len;
	    return SECSuccess;
	}
    }
    PORT_SetError(SEC_ERROR_INVALID_ARGS);
    return SECFailure;
}
