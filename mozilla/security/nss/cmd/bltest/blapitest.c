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

#include "blapi.h"
#include "prmem.h"
#include "prprf.h"
#include "prtime.h"
#include "nssb64.h"
#include "secutil.h"
#include "plgetopt.h"
#include "softoken.h"

char *progName;

#define CHECKERROR(rv, ln) \
	if (rv) { \
		char *errtxt = NULL; \
		if (PR_GetError() != 0) { \
		errtxt = PORT_Alloc(PR_GetErrorTextLength()); \
		PR_GetErrorText(errtxt); \
		} \
		PR_fprintf(PR_STDERR, "%s: ERR (%s) at line %d.\n", progName, \
		                       (errtxt) ? "" : errtxt, ln); \
		exit(-1); \
	}

/*  Helper functions for ascii<-->binary conversion/reading/writing */

static PRInt32
get_binary(void *arg, const unsigned char *ibuf, PRInt32 size)
{
	SECItem *binary = arg;
	SECItem *tmp;
	int index;
	if (binary->data == NULL) {
		tmp = SECITEM_AllocItem(NULL, NULL, size);
		binary->data = tmp->data;
		binary->len = tmp->len;
		index = 0;
	} else {
		SECITEM_ReallocItem(NULL, binary, binary->len, binary->len + size);
		index = binary->len;
	}
	PORT_Memcpy(&binary->data[index], ibuf, size);
	return binary->len;
}

static PRInt32
get_ascii(void *arg, const char *ibuf, PRInt32 size)
{
	SECItem *ascii = arg;
	SECItem *tmp;
	int index;
	if (ascii->data == NULL) {
		tmp = SECITEM_AllocItem(NULL, NULL, size);
		ascii->data = tmp->data;
		ascii->len = tmp->len;
		index = 0;
	} else {
		SECITEM_ReallocItem(NULL, ascii, ascii->len, ascii->len + size);
		index = ascii->len;
	}
	PORT_Memcpy(&ascii->data[index], ibuf, size);
	return ascii->len;
}

static SECStatus
atob(SECItem *ascii, SECItem *binary)
{
	SECStatus status;
	NSSBase64Decoder *cx;
	int len;
	binary->data = NULL; 
	binary->len = 0;
	len = (strcmp(&ascii->data[ascii->len-2],"\r\n"))?ascii->len:ascii->len-2;
	cx = NSSBase64Decoder_Create(get_binary, binary);
	status = NSSBase64Decoder_Update(cx, (const char *)ascii->data, len);
	status = NSSBase64Decoder_Destroy(cx, PR_FALSE);
	return status;
}

static PRInt32
output_ascii(void *arg, const char *obuf, PRInt32 size)
{
	PRFileDesc *outfile = arg;
	PRInt32 nb = PR_Write(outfile, obuf, size);
	if (nb != size) {
		PORT_SetError(SEC_ERROR_IO);
		return -1;
	}
	return nb;
}

static SECStatus
btoa(SECItem *binary, SECItem *ascii)
{
	SECStatus status;
	NSSBase64Encoder *cx;
	ascii->data = NULL;
	ascii->len = 0;
	cx = NSSBase64Encoder_Create(get_ascii, ascii);
	status = NSSBase64Encoder_Update(cx, binary->data, binary->len);
	status = NSSBase64Encoder_Destroy(cx, PR_FALSE);
	return status;
}

static SECStatus
btoa_file(SECItem *binary, PRFileDesc *outfile)
{
	SECStatus status;
	NSSBase64Encoder *cx;
	SECItem ascii;
	ascii.data = NULL; 
	ascii.len = 0;
	cx = NSSBase64Encoder_Create(output_ascii, outfile);
	status = NSSBase64Encoder_Update(cx, binary->data, binary->len);
	status = NSSBase64Encoder_Destroy(cx, PR_FALSE);
	status = PR_Write(outfile, "\r\n", 2);
	return status;
}

static SECStatus
get_and_write_random_bytes(SECItem *item, PRInt32 numbytes, char *filename)
{
	SECStatus rv;
	PRFileDesc *file;
	item->len = numbytes;
	item->data = (unsigned char *)PORT_ZAlloc(numbytes);
	RNG_GenerateGlobalRandomBytes(item->data + 1, numbytes - 1);
	file = PR_Open(filename, PR_WRONLY|PR_CREATE_FILE, 00660);
	rv = btoa_file(item, file);
	CHECKERROR((rv < 0), __LINE__);
	return (rv < 0);
}

static RSAPrivateKey *
rsakey_from_filedata(SECItem *filedata)
{
	PRArenaPool *arena;
	RSAPrivateKey *key;
	unsigned char *buf = filedata->data;
	int fpos = 0;
	int i;
	SECItem *item;
	/*  Allocate space for key structure. */
	arena = PORT_NewArena(2048);
	key = (RSAPrivateKey *)PORT_ArenaZAlloc(arena, sizeof(RSAPrivateKey));
	key->arena = arena;
	item = &key->version;
	for (i=0; i<9; i++) {
		item->len  = (buf[fpos++] & 0xff) << 24;
		item->len |= (buf[fpos++] & 0xff) << 16;
		item->len |= (buf[fpos++] & 0xff) <<  8;
		item->len |= (buf[fpos++] & 0xff);
		if (item->len > 0) {
			item->data = PORT_ArenaAlloc(arena, item->len);
			PORT_Memcpy(item->data, &buf[fpos], item->len);
		} else {
			item->data = NULL;
		}
		fpos += item->len;
		item++;
	}
	return key;
}

static void
rsakey_to_file(RSAPrivateKey *key, char *filename)
{
	PRFileDesc *file;
	SECItem *item;
	unsigned char len[4];
	int i;
	SECStatus status;
	NSSBase64Encoder *cx;
	SECItem ascii;
	ascii.data = NULL; 
	ascii.len = 0;
	file  = PR_Open(filename, PR_WRONLY|PR_CREATE_FILE, 00660);
	cx = NSSBase64Encoder_Create(output_ascii, file);
	item = &key->version;
	for (i=0; i<9; i++) {
		len[0] = (item->len >> 24) & 0xff;
		len[1] = (item->len >> 16) & 0xff;
		len[2] = (item->len >>  8) & 0xff;
		len[3] = (item->len & 0xff);
		status = NSSBase64Encoder_Update(cx, len, 4);
		status = NSSBase64Encoder_Update(cx, item->data, item->len);
		item++;
	}
	status = NSSBase64Encoder_Destroy(cx, PR_FALSE);
	status = PR_Write(file, "\r\n", 2);
	PR_Close(file);
}

static DSAPrivateKey *
dsakey_from_filedata(SECItem *filedata)
{
	PRArenaPool *arena;
	DSAPrivateKey *key;
	unsigned char *buf = filedata->data;
	int fpos = 0;
	int i;
	SECItem *item;
	/*  Allocate space for key structure. */
	arena = PORT_NewArena(2048);
	key = (DSAPrivateKey *)PORT_ArenaZAlloc(arena, sizeof(DSAPrivateKey));
	key->params.arena = arena;
	item = &key->params.prime;
	for (i=0; i<5; i++) {
		item->len  = (buf[fpos++] & 0xff) << 24;
		item->len |= (buf[fpos++] & 0xff) << 16;
		item->len |= (buf[fpos++] & 0xff) <<  8;
		item->len |= (buf[fpos++] & 0xff);
		if (item->len > 0) {
			item->data = PORT_ArenaAlloc(arena, item->len);
			PORT_Memcpy(item->data, &buf[fpos], item->len);
		} else {
			item->data = NULL;
		}
		fpos += item->len;
		item++;
	}
	return key;
}

static void
dsakey_to_file(DSAPrivateKey *key, char *filename)
{
	PRFileDesc *file;
	SECItem *item;
	unsigned char len[4];
	int i;
	SECStatus status;
	NSSBase64Encoder *cx;
	SECItem ascii;
	ascii.data = NULL; 
	ascii.len = 0;
	file  = PR_Open(filename, PR_WRONLY|PR_CREATE_FILE, 00660);
	cx = NSSBase64Encoder_Create(output_ascii, file);
	item = &key->params.prime;
	for (i=0; i<5; i++) {
		len[0] = (item->len >> 24) & 0xff;
		len[1] = (item->len >> 16) & 0xff;
		len[2] = (item->len >>  8) & 0xff;
		len[3] = (item->len & 0xff);
		status = NSSBase64Encoder_Update(cx, len, 4);
		status = NSSBase64Encoder_Update(cx, item->data, item->len);
		item++;
	}
	status = NSSBase64Encoder_Destroy(cx, PR_FALSE);
	status = PR_Write(file, "\r\n", 2);
}

/*  Multi-purpose crypto information */
typedef struct 
{
	SECItem seed;
	SECItem key;
   	SECItem iv;
   	SECItem in;
   	SECItem out;
	SECItem sigseed;
	PRInt32 keysize;
	PRInt32 bufsize;
	PRBool  encrypt;
	PRBool  useseed;
	PRBool  usesigseed;
	PRBool  performance;
	unsigned int rounds;    /* RC5 only */
	unsigned int wordsize;  /* RC5 only */
	unsigned int rsapubexp; /* RSA only */
} blapitestInfo;

/* Macros for performance timing. */
#define TIMESTART() \
	if (info->performance) \
		time1 = PR_IntervalNow();

#define TIMEFINISH(mode, bytes) \
	if (info->performance) { \
		time2 = (PRIntervalTime)(PR_IntervalNow() - time1); \
		time1 = PR_IntervalToMilliseconds(time2); \
		printf("%s %d bytes: %.2f ms\n", mode, bytes, ((float)(time1))/100); \
	}

/************************
**  DES 
************************/

/*  encrypt/decrypt for all DES */
static SECStatus
des_common(DESContext *descx, blapitestInfo *info)
{
	PRInt32 maxsize;
	SECStatus rv;
	PRIntervalTime time1, time2;
	int i, numiter;
	numiter = (info->performance) ? 100 : 1 ;
	maxsize = info->in.len;
	info->out.data = (unsigned char *)PORT_ZAlloc(maxsize);
	if (info->encrypt) {
		TIMESTART();
		for (i=0; i<numiter; i++)
			rv = DES_Encrypt(descx, info->out.data, &info->out.len, maxsize,
			                 info->in.data, info->in.len);
		TIMEFINISH("DES ENCRYPT", maxsize);
		if (rv) {
			fprintf(stderr, "%s:  Failed to encrypt!\n", progName);
			CHECKERROR(rv, __LINE__);
		}
	} else {
		TIMESTART();
		for (i=0; i<numiter; i++)
			rv = DES_Decrypt(descx, info->out.data, &info->out.len, maxsize,
			                 info->in.data, info->in.len);
		TIMEFINISH("DES DECRYPT", maxsize);
		if (rv) {
			fprintf(stderr, "%s:  Failed to decrypt!\n", progName);
			CHECKERROR(rv, __LINE__);
		}
	}
	return rv;
}

SECStatus
fillitem(SECItem *item, int numbytes, char *filename)
{
	if (item->len == 0)
		return get_and_write_random_bytes(item, numbytes, filename);
	return SECSuccess;
}

/*  DES codebook mode */
static SECStatus
des_ecb_test(blapitestInfo *info)
{
	SECStatus rv;
	DESContext *descx;
	fillitem(&info->key, DES_KEY_LENGTH, "tmp.key");
	fillitem(&info->in, info->bufsize, "tmp.pt");
	descx = DES_CreateContext(info->key.data, NULL, NSS_DES, info->encrypt);
	if (!descx) {
		fprintf(stderr,"%s:  Failed to create encryption context!\n", progName);
		return SECFailure;
	}
	rv = des_common(descx, info);
	CHECKERROR(rv, __LINE__);
	DES_DestroyContext(descx, PR_TRUE);
	return rv;
}

/*  DES chaining mode */
static SECStatus
des_cbc_test(blapitestInfo *info)
{
	SECStatus rv;
	DESContext *descx;
	fillitem(&info->key, DES_KEY_LENGTH, "tmp.key");
	fillitem(&info->in, info->bufsize, "tmp.pt");
	fillitem(&info->iv, DES_KEY_LENGTH, "tmp.iv");
	descx = DES_CreateContext(info->key.data, info->iv.data, NSS_DES_CBC, 
	                          info->encrypt);
	if (!descx) {
		PR_fprintf(PR_STDERR, 
		  "%s:  Failed to create encryption context!\n", progName);
		return SECFailure;
	}
	if (info->performance) {
		/*  In chaining mode, repeated iterations of the encryption
		 *  function using the same context will alter the final output.
		 *  So, once the performance test is done, reset the context
		 *  and perform a single iteration to obtain the correct result.
		 */
		rv = des_common(descx, info);
		DES_DestroyContext(descx, PR_TRUE);
		descx = DES_CreateContext(info->key.data, info->iv.data, NSS_DES_CBC, 
	                          info->encrypt);
		info->performance = PR_FALSE;
		rv = des_common(descx, info);
		info->performance = PR_TRUE;
	} else {
		rv = des_common(descx, info);
	}
	CHECKERROR(rv, __LINE__);
	DES_DestroyContext(descx, PR_TRUE);
	return rv;
}

/*  3-key Triple-DES codebook mode */
static SECStatus
des_ede_ecb_test(blapitestInfo *info)
{
	SECStatus rv;
	DESContext *descx;
	fillitem(&info->key, 3*DES_KEY_LENGTH, "tmp.key");
	fillitem(&info->in, info->bufsize, "tmp.pt");
	descx = DES_CreateContext(info->key.data, NULL, NSS_DES_EDE3,
	                          info->encrypt);
	if (!descx) {
		fprintf(stderr,"%s:  Failed to create encryption context!\n", progName);
		return SECFailure;
	}
	rv = des_common(descx, info);
	CHECKERROR(rv, __LINE__);
	DES_DestroyContext(descx, PR_TRUE);
	return rv;
}

/*  3-key Triple-DES chaining mode */
static SECStatus
des_ede_cbc_test(blapitestInfo *info)
{
	SECStatus rv;
	DESContext *descx;
	fillitem(&info->key, 3*DES_KEY_LENGTH, "tmp.key");
	fillitem(&info->in, info->bufsize, "tmp.pt");
	fillitem(&info->iv, DES_KEY_LENGTH, "tmp.iv");
	descx = DES_CreateContext(info->key.data, info->iv.data, NSS_DES_EDE3_CBC, 
	                          info->encrypt);
	if (!descx) {
		fprintf(stderr,"%s:  Failed to create encryption context!\n", progName);
		return SECFailure;
	}
	if (info->performance) {
		/*  In chaining mode, repeated iterations of the encryption
		 *  function using the same context will alter the final output.
		 *  So, once the performance test is done, reset the context
		 *  and perform a single iteration to obtain the correct result.
		 */
		rv = des_common(descx, info);
		DES_DestroyContext(descx, PR_TRUE);
		descx = DES_CreateContext(info->key.data, info->iv.data, 
		                          NSS_DES_EDE3_CBC, info->encrypt);
		info->performance = PR_FALSE;
		rv = des_common(descx, info);
		info->performance = PR_TRUE;
	} else {
		rv = des_common(descx, info);
	}
	CHECKERROR(rv, __LINE__);
	DES_DestroyContext(descx, PR_TRUE);
	return rv;
}

/************************
**  RC2 
************************/

/*  RC2 ECB */
static SECStatus
rc2_ecb_test(blapitestInfo *info)
{
	SECStatus rv;
	RC2Context *rc2cx;
	PRIntervalTime time1, time2;
	int i, numiter;
	numiter = (info->performance) ? 100 : 1 ;
	fillitem(&info->key, DES_KEY_LENGTH, "tmp.key");
	fillitem(&info->in, info->bufsize, "tmp.pt");
	rc2cx = RC2_CreateContext(info->key.data, info->key.len, NULL, 
	                          NSS_RC2, info->key.len);
	if (!rc2cx) {
		fprintf(stderr,"%s:  Failed to create encryption context!\n", progName);
		return SECFailure;
	}
	info->out.len = 2*info->in.len;
	info->out.data = (unsigned char *)PORT_ZAlloc(info->out.len);
	if (info->encrypt) {
		TIMESTART();
		for (i=0; i<numiter; i++)
			rv = RC2_Encrypt(rc2cx, info->out.data, &info->out.len, 
			                 info->out.len, info->in.data, info->in.len);
		TIMEFINISH("RC2 ECB ENCRYPT", info->in.len);
		CHECKERROR(rv, __LINE__);
	} else {
		TIMESTART();
		for (i=0; i<numiter; i++)
			rv = RC2_Decrypt(rc2cx, info->out.data, &info->out.len, 
			                 info->out.len, info->in.data, info->in.len);
		TIMEFINISH("RC2 ECB DECRYPT", info->in.len);
		CHECKERROR(rv, __LINE__);
	}
	RC2_DestroyContext(rc2cx, PR_TRUE);
	return rv;
}

/*  RC2 CBC */
static SECStatus
rc2_cbc_test(blapitestInfo *info)
{
	SECStatus rv;
	RC2Context *rc2cx;
	PRIntervalTime time1, time2;
	int i, numiter;
	numiter = (info->performance) ? 100 : 1 ;
	fillitem(&info->key, DES_KEY_LENGTH, "tmp.key");
	fillitem(&info->in, info->bufsize, "tmp.pt");
	fillitem(&info->iv, info->bufsize, "tmp.iv");
	rc2cx = RC2_CreateContext(info->key.data, info->key.len, info->iv.data, 
	                          NSS_RC2_CBC, info->key.len);
	if (!rc2cx) {
		fprintf(stderr,"%s:  Failed to create encryption context!\n", progName);
		return SECFailure;
	}
	info->out.len = 2*info->in.len;
	info->out.data = (unsigned char *)PORT_ZAlloc(info->out.len);
	if (info->encrypt) {
		TIMESTART();
		for (i=0; i<numiter; i++)
			rv = RC2_Encrypt(rc2cx, info->out.data, &info->out.len, 
			                 info->out.len, info->in.data, info->in.len);
		TIMEFINISH("RC2 CBC ENCRYPT", info->in.len);
		CHECKERROR(rv, __LINE__);
		if (rv) {
			fprintf(stderr, "%s:  Failed to encrypt!\n", progName);
			CHECKERROR(rv, __LINE__);
		}
	} else {
		TIMESTART();
		for (i=0; i<numiter; i++)
			rv = RC2_Decrypt(rc2cx, info->out.data, &info->out.len, 
			                 info->out.len, info->in.data, info->in.len);
		TIMEFINISH("RC2 CBC DECRYPT", info->in.len);
		if (rv) {
			fprintf(stderr, "%s:  Failed to decrypt!\n", progName);
			CHECKERROR(rv, __LINE__);
		}
	}
	RC2_DestroyContext(rc2cx, PR_TRUE);
	return rv;
}

/************************
**  RC4 
************************/

static SECStatus
rc4_test(blapitestInfo *info)
{
	SECStatus rv;
	RC4Context *rc4cx;
	PRIntervalTime time1, time2;
	int i, numiter;
	numiter = (info->performance) ? 100 : 1 ;
	fillitem(&info->key, DES_KEY_LENGTH, "tmp.key");
	fillitem(&info->in, info->bufsize, "tmp.pt");
	rc4cx = RC4_CreateContext(info->key.data, info->key.len);
	if (!rc4cx) {
		fprintf(stderr,"%s:  Failed to create encryption context!\n", progName);
		return SECFailure;
	}
	info->out.len = 2*info->in.len;
	info->out.data = (unsigned char *)PORT_ZAlloc(info->out.len);
	if (info->encrypt) {
		TIMESTART();
		for (i=0; i<numiter; i++)
			rv = RC4_Encrypt(rc4cx, info->out.data, &info->out.len, 
			                 info->out.len, info->in.data, info->in.len);
		TIMEFINISH("RC4 ENCRYPT", info->in.len);
		if (rv) {
			fprintf(stderr, "%s:  Failed to encrypt!\n", progName);
			CHECKERROR(rv, __LINE__);
		}
	} else {
		TIMESTART();
		for (i=0; i<numiter; i++)
			rv = RC4_Decrypt(rc4cx, info->out.data, &info->out.len, 
			                 info->out.len, info->in.data, info->in.len);
		TIMEFINISH("RC4 DECRYPT", info->in.len);
		if (rv) {
			fprintf(stderr, "%s:  Failed to decrypt!\n", progName);
			CHECKERROR(rv, __LINE__);
		}
	}
	RC4_DestroyContext(rc4cx, PR_TRUE);
	return rv;
}

/************************
**  RC5 
************************/

/*  RC5 ECB */
static SECStatus
rc5_ecb_test(blapitestInfo *info)
{
	SECStatus rv;
	RC5Context *rc5cx;
	PRIntervalTime time1, time2;
	int i, numiter;
	numiter = (info->performance) ? 100 : 1 ;
	fillitem(&info->key, DES_KEY_LENGTH, "tmp.key");
	fillitem(&info->in, info->bufsize, "tmp.pt");
	rc5cx = RC5_CreateContext(&info->key, info->rounds, info->wordsize, 
	                          NULL, NSS_RC5);
	if (!rc5cx) {
		fprintf(stderr,"%s:  Failed to create encryption context!\n", progName);
		return SECFailure;
	}
	info->out.len = 2*info->in.len;
	info->out.data = (unsigned char *)PORT_ZAlloc(info->out.len);
	if (info->encrypt) {
		TIMESTART();
		for (i=0; i<numiter; i++)
			rv = RC5_Encrypt(rc5cx, info->out.data, &info->out.len, 
			                 info->out.len, info->in.data, info->in.len);
		TIMEFINISH("RC5 ECB ENCRYPT", info->in.len);
		if (rv) {
			fprintf(stderr, "%s:  Failed to encrypt!\n", progName);
			CHECKERROR(rv, __LINE__);
		}
	} else {
		TIMESTART();
		for (i=0; i<numiter; i++)
			rv = RC5_Decrypt(rc5cx, info->out.data, &info->out.len, 
			                 info->out.len, info->in.data, info->in.len);
		TIMEFINISH("RC5 ECB ENCRYPT", info->in.len);
		if (rv) {
			fprintf(stderr, "%s:  Failed to decrypt!\n", progName);
			CHECKERROR(rv, __LINE__);
		}
	}
	RC5_DestroyContext(rc5cx, PR_TRUE);
	return rv;
}

/*  RC5 CBC */
static SECStatus
rc5_cbc_test(blapitestInfo *info)
{
	SECStatus rv;
	RC5Context *rc5cx;
	PRIntervalTime time1, time2;
	int i, numiter;
	numiter = (info->performance) ? 100 : 1 ;
	fillitem(&info->key, DES_KEY_LENGTH, "tmp.key");
	fillitem(&info->in, info->bufsize, "tmp.pt");
	fillitem(&info->iv, info->bufsize, "tmp.iv");
	rc5cx = RC5_CreateContext(&info->key, info->rounds, info->wordsize, 
	                          info->iv.data, NSS_RC5_CBC);
	if (!rc5cx) {
		fprintf(stderr,"%s:  Failed to create encryption context!\n", progName);
		return SECFailure;
	}
	info->out.len = 2*info->in.len;
	info->out.data = (unsigned char *)PORT_ZAlloc(info->out.len);
	if (info->encrypt) {
		TIMESTART();
		for (i=0; i<numiter; i++)
			rv = RC5_Encrypt(rc5cx, info->out.data, &info->out.len, 
			                 info->out.len, info->in.data, info->in.len);
		TIMEFINISH("RC5 CBC ENCRYPT", info->in.len);
		if (rv) {
			fprintf(stderr, "%s:  Failed to encrypt!\n", progName);
			CHECKERROR(rv, __LINE__);
		}
	} else {
		TIMESTART();
		for (i=0; i<numiter; i++)
			rv = RC5_Decrypt(rc5cx, info->out.data, &info->out.len, 
			                 info->out.len, info->in.data, info->in.len);
		TIMEFINISH("RC5 CBC DECRYPT", info->in.len);
		if (rv) {
			fprintf(stderr, "%s:  Failed to decrypt!\n", progName);
			CHECKERROR(rv, __LINE__);
		}
	}
	RC5_DestroyContext(rc5cx, PR_TRUE);
	return rv;
}

static SECStatus
rsa_test(blapitestInfo *info)
{
	RSAPrivateKey *key;
	SECItem expitem;
	SECStatus rv;
	PRIntervalTime time1, time2;
	int i, numiter;
	numiter = (info->performance) ? 100 : 1 ;
	fillitem(&info->in, info->bufsize, "tmp.pt");
	if (info->key.len > 0) {
		key = rsakey_from_filedata(&info->key);
	} else {
		expitem.len = 4;
		expitem.data = (unsigned char *)PORT_ZAlloc(4);
		expitem.data[0] = (info->rsapubexp >> 24) & 0xff;
		expitem.data[1] = (info->rsapubexp >> 16) & 0xff;
		expitem.data[2] = (info->rsapubexp >>  8) & 0xff;
		expitem.data[3] = (info->rsapubexp & 0xff);
		key = RSA_NewKey(info->keysize*8, &expitem);
		rsakey_to_file(key, "tmp.key");
	}
	info->out.len = info->in.len; 
	info->out.data = (unsigned char *)PORT_ZAlloc(info->out.len);
	if (info->encrypt) {
		RSAPublicKey pubkey;
		SECITEM_CopyItem(key->arena, &pubkey.modulus, &key->modulus);
		SECITEM_CopyItem(key->arena, &pubkey.publicExponent, 
		                             &key->publicExponent);
		TIMESTART();
		for (i=0; i<numiter; i++)
			rv = RSA_PublicKeyOp(&pubkey, info->out.data, info->in.data);
		TIMEFINISH("RSA ENCRYPT", info->in.len);
		CHECKERROR(rv, __LINE__);
	} else {
		TIMESTART();
		for (i=0; i<100; i++)
			rv = RSA_PrivateKeyOp(key, info->out.data, info->in.data);
		TIMEFINISH("RSA DECRYPT", info->in.len);
		CHECKERROR(rv, __LINE__);
	}
	PORT_FreeArena(key->arena, PR_TRUE);
	return SECSuccess;
}

static SECStatus
dsa_test(blapitestInfo *info)
{
	PQGParams *params;
	PQGVerify *verify;
	DSAPrivateKey *key;
	SECStatus rv;
	PRIntervalTime time1, time2;
	int i, numiter;
	numiter = (info->performance) ? 100 : 1 ;
	fillitem(&info->in, info->bufsize, "tmp.pt");
	if (info->key.len > 0) {
		key = dsakey_from_filedata(&info->key);
	} else {
		rv = PQG_ParamGen(info->keysize, &params, &verify);
		CHECKERROR(rv, __LINE__);
		if (info->useseed) {
			if (info->seed.len == 0)
				get_and_write_random_bytes(&info->seed, DSA_SUBPRIME_LEN, 
				                           "tmp.seed");
			rv = DSA_NewKeyFromSeed(params, info->seed.data, &key);
		} else {
			rv = DSA_NewKey(params, &key);
		}
		CHECKERROR(rv, __LINE__);
		dsakey_to_file(key, "tmp.key");
	}
	if (info->encrypt) {
		info->out.len = 48;
		info->out.data = (unsigned char *)PORT_ZAlloc(info->out.len);
		if (info->usesigseed) {
			if (info->sigseed.len == 0)
				get_and_write_random_bytes(&info->sigseed, DSA_SUBPRIME_LEN,
				                           "tmp.sigseed");
			TIMESTART();
			rv = DSA_SignDigestWithSeed(key, &info->out, &info->in, 
			                            info->sigseed.data);
			TIMEFINISH("DSA SIGN", info->in.len);
		} else {
			TIMESTART();
			for (i=0; i<numiter; i++)
				rv = DSA_SignDigest(key, &info->out, &info->in);
			TIMEFINISH("DSA SIGN", info->in.len);
		}
		CHECKERROR(rv, __LINE__);
	} else {
		DSAPublicKey pubkey;
		PRArenaPool *arena;
		arena = key->params.arena;
		SECITEM_CopyItem(arena, &pubkey.params.prime, &key->params.prime);
		SECITEM_CopyItem(arena, &pubkey.params.subPrime, &key->params.subPrime);
		SECITEM_CopyItem(arena, &pubkey.params.base, &key->params.base);
		SECITEM_CopyItem(arena, &pubkey.publicValue, &key->publicValue);
		TIMESTART();
		for (i=0; i<numiter; i++)
			rv = DSA_VerifyDigest(&pubkey, &info->out, &info->in);
		TIMEFINISH("DSA VERIFY", info->in.len);
		if (rv == SECSuccess) {
			PR_fprintf(PR_STDOUT, "Signature verified.\n");
		} else {
			PR_fprintf(PR_STDOUT, "Signature failed verification!\n");
			CHECKERROR(rv, __LINE__);
		}
	}
	PORT_FreeArena(key->params.arena, PR_TRUE);
	return SECSuccess;
}

static SECStatus
md5_perf_test(blapitestInfo *info)
{
	SECStatus rv = SECSuccess;
	PRInt64 time1, time2;
	PRInt32 tdiff;
	int i;
	if (info->in.len == 0) {
		rv = get_and_write_random_bytes(&info->in, info->bufsize, "tmp.pt");
		CHECKERROR(rv, __LINE__);
	}
	info->out.len = 16;
	info->out.data = (unsigned char *)PORT_ZAlloc(info->out.len);
	time1 = PR_Now();
	for (i=0; i<100; i++) {
		MD5_HashBuf(info->out.data, info->in.data, info->in.len);
	}
	time2 = PR_Now();
	LL_SUB(time1, time2, time1);
	LL_L2I(tdiff, time1);
	PR_fprintf(PR_STDOUT, "MD5 hash %d bytes: %d\n", info->in.len, tdiff / 100);
	return rv;
}

static SECStatus
md5_test(blapitestInfo *info)
{
	SECStatus rv = SECSuccess;
	PRInt64 time1, time2;
	PRInt32 tdiff;
	int i;
	if (info->performance) return md5_perf_test(info);
	if (info->in.len == 0) {
		rv = get_and_write_random_bytes(&info->in, info->bufsize, "tmp.pt");
		CHECKERROR(rv, __LINE__);
	}
	info->out.len = 16;
	info->out.data = (unsigned char *)PORT_ZAlloc(info->out.len);
	time1 = PR_Now();
	MD5_HashBuf(info->out.data, info->in.data, info->in.len);
	time2 = PR_Now();
	LL_SUB(time1, time2, time1);
	LL_L2I(tdiff, time1);
	PR_fprintf(PR_STDOUT, "time to hash: %d\n", tdiff);
	return rv;
}

static SECStatus
md5_multi_test(blapitestInfo *info)
{
	SECStatus rv = SECSuccess;
	MD5Context *md5cx;
	unsigned int len;
	MD5Context *foomd5cx;
	unsigned char *foomd5;
	int i;
	if (info->in.len == 0) {
		rv = get_and_write_random_bytes(&info->in, info->bufsize, "tmp.pt");
		CHECKERROR(rv, __LINE__);
	}
	md5cx = MD5_NewContext();
	if (!md5cx) {
		PR_fprintf(PR_STDERR, 
		   "%s:  Failed to create hash context!\n", progName);
		return SECFailure;
	}
	info->out.len = 16;
	info->out.data = (unsigned char *)PORT_ZAlloc(info->out.len);
	MD5_Begin(md5cx);
	for (i=0; i<info->bufsize/8; i++) {
		MD5_Update(md5cx, &info->in.data[i*8], 8);
		len = MD5_FlattenSize(md5cx);
		foomd5 = PORT_Alloc(len);
		MD5_Flatten(md5cx, foomd5);
		foomd5cx = MD5_Resurrect(foomd5, NULL);
		rv = PORT_Memcmp(foomd5cx, md5cx, len);
		if (rv != SECSuccess)
			PR_fprintf(PR_STDERR, "%s:  MD5_Resurrect failed!\n", progName);
		MD5_DestroyContext(foomd5cx, PR_TRUE);
		PORT_Free(foomd5);
	}
	MD5_End(md5cx, info->out.data, &len, 16);
	if (len != 16)
		PR_fprintf(PR_STDERR, "%s: Bad hash size %d.\n", progName, len);
	MD5_DestroyContext(md5cx, PR_TRUE);
	return rv;
}

static SECStatus
md2_perf_test(blapitestInfo *info)
{
	unsigned int len;
	MD2Context *cx = MD2_NewContext();
	SECStatus rv = SECSuccess;
	PRInt64 time1, time2;
	PRInt32 tdiff;
	int i;
	if (info->in.len == 0) {
		rv = get_and_write_random_bytes(&info->in, info->bufsize, "tmp.pt");
		CHECKERROR(rv, __LINE__);
	}
	info->out.len = 16;
	info->out.data = (unsigned char *)PORT_ZAlloc(info->out.len);
	info->in.data[info->in.len] = '\0';
	time1 = PR_Now();
	for (i=0; i<100; i++) {
		MD2_Begin(cx);
		MD2_Update(cx, info->in.data, info->in.len);
		MD2_End(cx, info->out.data, &len, 16);
	}
	time2 = PR_Now();
	LL_SUB(time1, time2, time1);
	LL_L2I(tdiff, time1);
	PR_fprintf(PR_STDOUT, "MD2 hash %d bytes: %d\n", info->in.len, tdiff / 100);
	MD2_DestroyContext(cx, PR_TRUE);
	return rv;
}

static SECStatus
md2_test(blapitestInfo *info)
{
	SECStatus rv = SECSuccess;
	PRInt64 time1, time2;
	PRInt32 tdiff = LL_Zero();
	int i;
	unsigned int len;
	if (info->performance) return md2_perf_test(info);
	if (info->in.len == 0) {
		rv = get_and_write_random_bytes(&info->in, info->bufsize, "tmp.pt");
		CHECKERROR(rv, __LINE__);
	}
	info->out.len = 16;
	info->out.data = (unsigned char *)PORT_ZAlloc(info->out.len);
	info->in.data[info->in.len] = '\0';
	time1 = PR_Now();
	MD2_Hash(info->out.data, info->in.data);
	time2 = PR_Now();
	LL_SUB(time1, time2, time1);
	LL_L2I(tdiff, time1);
	PR_fprintf(PR_STDOUT, "time to hash: %d\n", tdiff);
	return rv;
}

static SECStatus
md2_multi_test(blapitestInfo *info)
{
	SECStatus rv = SECSuccess;
	MD2Context *md2cx;
	unsigned int len;
	MD2Context *foomd2cx;
	unsigned char *foomd2;
	int i;
	if (info->in.len == 0) {
		rv = get_and_write_random_bytes(&info->in, info->bufsize, "tmp.pt");
		CHECKERROR(rv, __LINE__);
	}
	md2cx = MD2_NewContext();
	if (!md2cx) {
		PR_fprintf(PR_STDERR, 
		   "%s:  Failed to create hash context!\n", progName);
		return SECFailure;
	}
	info->out.len = 16;
	info->out.data = (unsigned char *)PORT_ZAlloc(info->out.len);
	MD2_Begin(md2cx);
	for (i=0; i<info->bufsize/8; i++) {
		MD2_Update(md2cx, &info->in.data[i*8], 8);
		len = MD2_FlattenSize(md2cx);
		foomd2 = PORT_Alloc(len);
		MD2_Flatten(md2cx, foomd2);
		foomd2cx = MD2_Resurrect(foomd2, NULL);
		rv = PORT_Memcmp(foomd2cx, md2cx, len);
		if (rv != SECSuccess)
			PR_fprintf(PR_STDERR, "%s:  MD2_Resurrect failed!\n", progName);
		MD2_DestroyContext(foomd2cx, PR_TRUE);
		PORT_Free(foomd2);
	}
	MD2_End(md2cx, info->out.data, &len, 16);
	if (len != 16)
		PR_fprintf(PR_STDERR, "%s: Bad hash size %d.\n", progName, len);
	MD2_DestroyContext(md2cx, PR_TRUE);
	return rv;
}

typedef SECStatus (* blapitestCryptoFn)(blapitestInfo *);

static blapitestCryptoFn crypto_fns[] =
{
	des_ecb_test,
	des_cbc_test,
	des_ede_ecb_test,
	des_ede_cbc_test,
	rc2_ecb_test,
	rc2_cbc_test,
	rc4_test,
	rc5_ecb_test,
	rc5_cbc_test,
	rsa_test,
	dsa_test,
	md5_test,
	md5_multi_test,
	md2_test,
	md2_multi_test
};

static char *mode_strings[] =
{
	"des_ecb",
	"des_cbc",
	"des3_ecb",
	"des3_cbc",
	"rc2_ecb",
	"rc2_cbc",
	"rc4",
	"rc5_ecb",
	"rc5_cbc",
	"rsa",
	"dsa",
	"md5",
	"md5_multi",
	"md2",
	"md2_multi"
};

static void
printmodes()
{
	int i;
	int nummodes = sizeof(mode_strings) / sizeof(char *);
	PR_fprintf(PR_STDERR, "Available modes: (specify with -m)\n", progName);
	for (i=0; i<nummodes; i++) {
		PR_fprintf(PR_STDERR, "%s\n", mode_strings[i]);
	}
}

static blapitestCryptoFn
get_test_mode(const char *modestring)
{
	int i;
	int nummodes = sizeof(mode_strings) / sizeof(char *);
	for (i=0; i<nummodes; i++)
		if (PL_strcmp(modestring, mode_strings[i]) == 0)
			return crypto_fns[i];
	PR_fprintf(PR_STDERR, "%s: invalid mode: %s\n", progName, modestring);
	exit(-1);
}

int main(int argc, char **argv) 
{
	PRFileDesc *infile, *outfile, *keyfile, *ivfile, *sigfile, *seedfile,
	           *sigseedfile;
	PRBool encrypt, decrypt, script = PR_FALSE, b64 = PR_TRUE;
	blapitestInfo info;
	blapitestCryptoFn cryptofn = NULL;
	PLOptState *optstate;
	PLOptStatus status;
	int numiter = 1;
	PRBool dofips = PR_FALSE;
	SECStatus rv;

	PORT_Memset(&info, 0, sizeof(info));
	info.bufsize = 8;
	info.keysize = 8;
	info.rsapubexp = 17;
	info.rounds = 10;
	info.wordsize = 4;
	infile=outfile=keyfile=ivfile=sigfile=seedfile=sigseedfile=NULL;
	info.performance=encrypt=decrypt=PR_FALSE;
    progName = strrchr(argv[0], '/');
    progName = progName ? progName+1 : argv[0];
	optstate = 
	  PL_CreateOptState(argc, argv, "DEFS:ab:e:g:i:o:pk:m:t:r:s:v:w:xyz:");
	while ((status = PL_GetNextOpt(optstate)) == PL_OPT_OK) {
		switch (optstate->option) {
		case 'D':  decrypt = PR_TRUE; break;
		case 'E':  encrypt = PR_TRUE; break;
		case 'F':  dofips = PR_TRUE; break;
		case 'S':  script = PR_TRUE; numiter=PORT_Atoi(optstate->value); break;
		case 'a':  b64 = PR_FALSE; break;
		case 'b':  info.bufsize = PORT_Atoi(optstate->value); break;
		case 'e':  info.rsapubexp = PORT_Atoi(optstate->value); break;
		case 'g':  info.keysize = PORT_Atoi(optstate->value); break;
		case 'i':  infile  = PR_Open(optstate->value, PR_RDONLY, 00440); break;
		case 'k':  keyfile = PR_Open(optstate->value, PR_RDONLY, 00440); break;
		case 'm':  cryptofn = get_test_mode(optstate->value); break;
		case 'o':  outfile = PR_Open(optstate->value, PR_WRONLY|PR_CREATE_FILE,
		                             00660); break;
		case 'p':  info.performance = PR_TRUE; break;
		case 'r':  info.rounds = PORT_Atoi(optstate->value); break;
		case 's':  sigfile  = PR_Open(optstate->value, PR_RDONLY, 00440); break;
		case 't':  sigseedfile=PR_Open(optstate->value, PR_RDONLY, 00440);break;
		case 'w':  info.wordsize = PORT_Atoi(optstate->value); break;
		case 'v':  ivfile  = PR_Open(optstate->value, PR_RDONLY, 00440); break;
		case 'x':  info.useseed = PR_TRUE; break;
		case 'y':  info.usesigseed = PR_TRUE; break;
		case 'z':  seedfile  = PR_Open(optstate->value, PR_RDONLY,00440); break;
		default: break;
		}
	}

	if (dofips) {
		CK_RV foo = pk11_fipsPowerUpSelfTest();
		PR_fprintf(PR_STDOUT, "CK_RV: %d.\n", foo);
		return 0;
	}

	if ((!encrypt && !decrypt) || (encrypt && decrypt))
		return -1;

	if (!cryptofn) {
		printmodes();
		return -1;
	}

	if (decrypt && !infile)
		return -1;

	info.encrypt = encrypt;

	RNG_RNGInit();

	if (keyfile) {
		SECItem asciiKey;
		rv = SECU_FileToItem(&asciiKey, keyfile);
		CHECKERROR(rv, __LINE__);
		rv = atob(&asciiKey, &info.key);
		CHECKERROR(rv, __LINE__);
	}

	if (ivfile) {
		SECItem asciiIv;
		rv = SECU_FileToItem(&asciiIv, ivfile);
		CHECKERROR(rv, __LINE__);
		rv = atob(&asciiIv, &info.iv);
		CHECKERROR(rv, __LINE__);
	}

	if (infile) {
		SECItem asciiFile;
		rv = SECU_FileToItem(&asciiFile, infile);
		CHECKERROR(rv, __LINE__);
		if (b64) {
			rv = atob(&asciiFile, &info.in);
		CHECKERROR(rv, __LINE__);
		} else {
			SECITEM_CopyItem(NULL, &info.in, &asciiFile);
		}
		info.bufsize = info.in.len;
	}

	if (sigfile) {
		SECItem asciiSig;
		rv = SECU_FileToItem(&asciiSig, sigfile);
		CHECKERROR(rv, __LINE__);
		rv = atob(&asciiSig, &info.out);
		CHECKERROR(rv, __LINE__);
	}

	if (seedfile) {
		SECItem asciiSeed;
		rv = SECU_FileToItem(&asciiSeed, seedfile);
		CHECKERROR(rv, __LINE__);
		rv = atob(&asciiSeed, &info.seed);
		CHECKERROR(rv, __LINE__);
		info.useseed = PR_TRUE;
	}

	if (sigseedfile) {
		SECItem asciiSeed;
		rv = SECU_FileToItem(&asciiSeed, sigseedfile);
		CHECKERROR(rv, __LINE__);
		rv = atob(&asciiSeed, &info.sigseed);
		CHECKERROR(rv, __LINE__);
		info.usesigseed = PR_TRUE;
	}

	rv = (*cryptofn)(&info);
		CHECKERROR(rv, __LINE__);

	if (!sigfile) {
		if (!outfile)
			outfile = PR_Open("tmp.out", PR_WRONLY|PR_CREATE_FILE, 00660);
		rv = btoa_file(&info.out, outfile);
		CHECKERROR((rv < 0), __LINE__);
	}

	RNG_RNGShutdown();

	if (infile)
		PR_Close(infile);
	if (outfile)
		PR_Close(outfile);
	if (keyfile)
		PR_Close(keyfile);
	if (ivfile)
		PR_Close(ivfile);

	return SECSuccess;
}
