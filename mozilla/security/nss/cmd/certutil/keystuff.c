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

#include <stdio.h>
#include <string.h>
#include "secutil.h"

#if defined(XP_UNIX)
#include <unistd.h>
#include <sys/time.h>
#include <termios.h>
#endif

#if defined(XP_WIN) || defined (XP_PC)
#include <time.h>
#include <conio.h>
#endif

#if defined(__sun) && !defined(SVR4)
extern int fclose(FILE*);
extern int fprintf(FILE *, char *, ...);
extern int isatty(int);
extern char *sys_errlist[];
#define strerror(errno) sys_errlist[errno]
#endif

#include "nspr.h"
#include "prtypes.h"
#include "prtime.h"
#include "prlong.h"

#include "pk11func.h"
#include "secrng.h"
#include "pqgutil.h"

#define NUM_KEYSTROKES 120
#define RAND_BUF_SIZE 60

#define ERROR_BREAK rv = SECFailure;break;


static void
UpdateRNG(void)
{
    char *         randbuf;
    int            fd, i, count;
    char           c;
#ifdef XP_UNIX
    cc_t           orig_cc_min;
    cc_t           orig_cc_time;
    tcflag_t       orig_lflag;
    struct termios tio;
#endif

#define FPS fprintf(stderr, 
    FPS "\n");
    FPS "A random seed must be generated that will be used in the\n");
    FPS "creation of your key.  One of the easiest ways to create a\n");
    FPS "random seed is to use the timing of keystrokes on a keyboard.\n");
    FPS "\n");
    FPS "To begin, type keys on the keyboard until this progress meter\n");
    FPS "is full.  DO NOT USE THE AUTOREPEAT FUNCTION ON YOUR KEYBOARD!\n");
    FPS "\n");
    FPS "\n");
    FPS "Continue typing until the progress meter is full:\n\n");
    FPS "|                                                            |\r");

    /* turn off echo on stdin & return on 1 char instead of NL */
    fd = fileno(stdin);

#ifdef XP_UNIX
    tcgetattr(fd, &tio);
    orig_lflag = tio.c_lflag;
    orig_cc_min = tio.c_cc[VMIN];
    orig_cc_time = tio.c_cc[VTIME];
    tio.c_lflag &= ~ECHO;
    tio.c_lflag &= ~ICANON;
    tio.c_cc[VMIN] = 1;
    tio.c_cc[VTIME] = 0;
    tcsetattr(fd, TCSAFLUSH, &tio);
#endif

    /* Get random noise from keyboard strokes */
    randbuf = (char *) PORT_Alloc(RAND_BUF_SIZE);
    count = 0;
    while (count < NUM_KEYSTROKES+1) {
#ifdef XP_UNIX
	c = getc(stdin);
#else
	c = getch();
#endif
	RNG_GetNoise(&randbuf[1], sizeof(randbuf)-1);
	RNG_RandomUpdate(randbuf, sizeof(randbuf));
	if (c != randbuf[0]) {
	    randbuf[0] = c;
	    FPS "\r|");
	    for (i=0; i<count/(NUM_KEYSTROKES/RAND_BUF_SIZE); i++) {
		FPS "*");
	    }
	    if (count%(NUM_KEYSTROKES/RAND_BUF_SIZE) == 1)
		FPS "/");
	    count++;
	}
    }
    free(randbuf); 

    FPS "\n\n");
    FPS "Finished.  Press enter to continue: ");
    while (getc(stdin) != '\n')
	;
    FPS "\n");

#undef FPS

#ifdef XP_UNIX
    /* set back termio the way it was */
    tio.c_lflag = orig_lflag;
    tio.c_cc[VMIN] = orig_cc_min;
    tio.c_cc[VTIME] = orig_cc_time;
    tcsetattr(fd, TCSAFLUSH, &tio);
#endif
}


static unsigned char P[] = { 0x00, 0x8d, 0xf2, 0xa4, 0x94, 0x49, 0x22, 0x76,
			     0xaa, 0x3d, 0x25, 0x75, 0x9b, 0xb0, 0x68, 0x69,
			     0xcb, 0xea, 0xc0, 0xd8, 0x3a, 0xfb, 0x8d, 0x0c,
			     0xf7, 0xcb, 0xb8, 0x32, 0x4f, 0x0d, 0x78, 0x82,
			     0xe5, 0xd0, 0x76, 0x2f, 0xc5, 0xb7, 0x21, 0x0e,
			     0xaf, 0xc2, 0xe9, 0xad, 0xac, 0x32, 0xab, 0x7a,
			     0xac, 0x49, 0x69, 0x3d, 0xfb, 0xf8, 0x37, 0x24,
			     0xc2, 0xec, 0x07, 0x36, 0xee, 0x31, 0xc8, 0x02,
			     0x91 };
static unsigned char Q[] = { 0x00, 0xc7, 0x73, 0x21, 0x8c, 0x73, 0x7e, 0xc8,
			     0xee, 0x99, 0x3b, 0x4f, 0x2d, 0xed, 0x30, 0xf4,
			     0x8e, 0xda, 0xce, 0x91, 0x5f };
static unsigned char G[] = { 0x00, 0x62, 0x6d, 0x02, 0x78, 0x39, 0xea, 0x0a,
			     0x13, 0x41, 0x31, 0x63, 0xa5, 0x5b, 0x4c, 0xb5,
			     0x00, 0x29, 0x9d, 0x55, 0x22, 0x95, 0x6c, 0xef,
			     0xcb, 0x3b, 0xff, 0x10, 0xf3, 0x99, 0xce, 0x2c,
			     0x2e, 0x71, 0xcb, 0x9d, 0xe5, 0xfa, 0x24, 0xba,
			     0xbf, 0x58, 0xe5, 0xb7, 0x95, 0x21, 0x92, 0x5c,
			     0x9c, 0xc4, 0x2e, 0x9f, 0x6f, 0x46, 0x4b, 0x08,
			     0x8c, 0xc5, 0x72, 0xaf, 0x53, 0xe6, 0xd7, 0x88,
			     0x02 };

static PQGParams default_pqg_params = {
    NULL,
    { 0, P, sizeof(P) },
    { 0, Q, sizeof(Q) },
    { 0, G, sizeof(G) }
};

static PQGParams *
decode_pqg_params(char *str)
{
    char *buf;
    unsigned int len;
    PRArenaPool *arena;
    PQGParams *params;
    SECStatus status;
 
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if (arena == NULL)
        return NULL;
 
    params = PORT_ArenaZAlloc(arena, sizeof(PQGParams));
    if (params == NULL)
        goto loser;
    params->arena = arena;
 
    buf = (char *)ATOB_AsciiToData(str, &len);
    if ((buf == NULL) || (len == 0))
        goto loser;
 
    status = SEC_ASN1Decode(arena, params, SECKEY_PQGParamsTemplate, buf, len);
    if (status != SECSuccess)
        goto loser;
 
    return params;
 
loser:
    if (arena != NULL)
        PORT_FreeArena(arena, PR_FALSE);
    return NULL;
}
 
static int
pqg_prime_bits(char *str)
{
    PQGParams *params = NULL;
    int primeBits = 0, i;
 
    params = decode_pqg_params(str);
    if (params == NULL)
        goto done; /* lose */
 
    for (i = 0; params->prime.data[i] == 0; i++)
        /* empty */;
	primeBits = (params->prime.len - i) * 8;
 
done:
    if (params != NULL)
        PQG_DestroyParams(params);
    return primeBits;
}

static char *
SECU_GetpqgString(char *filename)
{
    unsigned char phrase[400];
    FILE *fh;
    char *rv;

    fh = fopen(filename,"r");
    rv = fgets ((char*) phrase, sizeof(phrase), fh);

    fclose(fh);
    if (phrase[strlen(phrase)-1] == '\n')
	phrase[strlen(phrase)-1] = '\0';
    if (rv) {
	return (char*) PORT_Strdup((char*)phrase);
    }
    fprintf(stderr,"pqg file contain no data\n");
    return NULL;
}

PQGParams*
getpqgfromfile(int keyBits, char *pqgFile)
{
    char *end, *str, *pqgString;
    int primeBits;

    pqgString = SECU_GetpqgString(pqgFile);
    if (pqgString)
	str = PORT_Strdup(pqgString);
    else
	return NULL;

    do {
	end = PORT_Strchr(str, ',');
	if (end)
	    *end = '\0';
	primeBits = pqg_prime_bits(str);
	if (keyBits == primeBits)
	    goto found_match;
	str = end + 1;
    } while (end);

    PORT_Free(pqgString);
    PORT_Free(str);
    return NULL;

found_match:
    PORT_Free(pqgString);
    PORT_Free(str);
    return decode_pqg_params(str);
}

SECKEYPrivateKey *
CERTUTIL_GeneratePrivateKey(KeyType keytype, PK11SlotInfo *slot, int size,
			    int publicExponent, char *noise, 
			    SECKEYPublicKey **pubkeyp, char *pqgFile,
                            char *passFile)
{
    CK_MECHANISM_TYPE mechanism;
    SECOidTag algtag;
    PK11RSAGenParams rsaparams;
    PQGParams *dsaparams = NULL;
    void *params;
    secuPWData pwdata = { PW_NONE, 0 };

    /*
     * Do some random-number initialization.
     */
    RNG_SystemInfoForRNG();

    if (noise) {
    	RNG_FileForRNG(noise);
    } else {
	UpdateRNG();
    }

    switch (keytype) {
      case rsaKey:
	rsaparams.keySizeInBits = size;
	rsaparams.pe = publicExponent;
	mechanism = CKM_RSA_PKCS_KEY_PAIR_GEN;
	algtag = SEC_OID_PKCS1_MD5_WITH_RSA_ENCRYPTION;
	params = &rsaparams;
	break;
      case dsaKey:
	mechanism = CKM_DSA_KEY_PAIR_GEN;
	algtag = SEC_OID_ANSIX9_DSA_SIGNATURE_WITH_SHA1_DIGEST;
	if (pqgFile) {
	    dsaparams = getpqgfromfile(size, pqgFile);
	} else {
	    dsaparams = &default_pqg_params;
	}
	params = dsaparams;
      default:
	return NULL;
    }

    if (slot == NULL)
	return NULL;

    if (passFile) {
	pwdata.source = PW_FROMFILE;
	pwdata.data = passFile;
    }

    if (PK11_Authenticate(slot, PR_TRUE, &pwdata) != SECSuccess)
	return NULL;

    fprintf(stderr, "\n\n");
    fprintf(stderr, "Generating key.  This may take a few moments...\n\n");

    return PK11_GenerateKeyPair(slot, mechanism, params, pubkeyp,
				PR_TRUE /*isPerm*/, PR_TRUE /*isSensitive*/, 
				NULL /*wincx*/);
}

/*
 * The following is all functionality moved over from keyutil, which may
 * or may not become completely obsolete.  So, some of this stuff may
 * end up being turned on from within certutil.  Some is probably not
 * even feasible anymore (Add/Delete?).
 */
#ifdef LATER

static SECStatus
ListKeys(FILE *out)
{
    int rt;

    rt = SECU_PrintKeyNames(handle, out);
    if (rt) {
	SECU_PrintError(progName, "unable to list nicknames");
	return SECFailure;
    }
    return SECSuccess;
}

static SECStatus
DumpPublicKey(char *nickname, FILE *out)
{
    SECKEYLowPrivateKey *privKey;
    SECKEYLowPublicKey *publicKey;

    /* check if key actually exists */
    if (SECU_CheckKeyNameExists(handle, nickname) == PR_FALSE) {
	SECU_PrintError(progName, "the key \"%s\" does not exist", nickname);
	return SECFailure;
    }

    /* Read in key */
    privKey = SECU_GetPrivateKey(handle, nickname);
    if (!privKey) {
	return SECFailure;
    }

    publicKey = SECKEY_LowConvertToPublicKey(privKey);

    /* Output public key (in the clear) */
    switch(publicKey->keyType) {
      case rsaKey:
	fprintf(out, "RSA Public-Key:\n");
	SECU_PrintInteger(out, &publicKey->u.rsa.modulus, "modulus", 1);
	SECU_PrintInteger(out, &publicKey->u.rsa.publicExponent,
			  "publicExponent", 1);
	break;
      case dsaKey:
	fprintf(out, "DSA Public-Key:\n");
	SECU_PrintInteger(out, &publicKey->u.dsa.params.prime, "prime", 1);
	SECU_PrintInteger(out, &publicKey->u.dsa.params.subPrime,
			  "subPrime", 1);
	SECU_PrintInteger(out, &publicKey->u.dsa.params.base, "base", 1);
	SECU_PrintInteger(out, &publicKey->u.dsa.publicValue, "publicValue", 1);
	break;
      default:
	fprintf(out, "unknown key type\n");
	break;
    }
    return SECSuccess;
}

static SECStatus
DumpPrivateKey(char *nickname, FILE *out)
{
    SECKEYLowPrivateKey *key;

    /* check if key actually exists */
    if (SECU_CheckKeyNameExists(handle, nickname) == PR_FALSE) {
	SECU_PrintError(progName, "the key \"%s\" does not exist", nickname);
	return SECFailure;
    }

    /* Read in key */
    key = SECU_GetPrivateKey(handle, nickname);
    if (!key) {
	SECU_PrintError(progName, "error retrieving key");
	return SECFailure;
    }

    switch(key->keyType) {
      case rsaKey:
	fprintf(out, "RSA Private-Key:\n");
	SECU_PrintInteger(out, &key->u.rsa.modulus, "modulus", 1);
	SECU_PrintInteger(out, &key->u.rsa.publicExponent, "publicExponent", 1);
	SECU_PrintInteger(out, &key->u.rsa.privateExponent,
			  "privateExponent", 1);
	SECU_PrintInteger(out, &key->u.rsa.prime[0], "prime[0]", 1);
	SECU_PrintInteger(out, &key->u.rsa.prime[1], "prime[1]", 1);
	SECU_PrintInteger(out, &key->u.rsa.primeExponent[0],
			  "primeExponent[0]", 1);
	SECU_PrintInteger(out, &key->u.rsa.primeExponent[1],
			  "primeExponent[1]", 1);
	SECU_PrintInteger(out, &key->u.rsa.coefficient, "coefficient", 1);
	break;
      case dsaKey:
	fprintf(out, "DSA Private-Key:\n");
	SECU_PrintInteger(out, &key->u.dsa.params.prime, "prime", 1);
	SECU_PrintInteger(out, &key->u.dsa.params.subPrime, "subPrime", 1);
	SECU_PrintInteger(out, &key->u.dsa.params.base, "base", 1);
	SECU_PrintInteger(out, &key->u.dsa.publicValue, "publicValue", 1);
	SECU_PrintInteger(out, &key->u.dsa.privateValue, "privateValue", 1);
	break;
      default:
	fprintf(out, "unknown key type\n");
	break;
    }
    return SECSuccess;
}

static SECStatus
ChangePassword(void)
{
    SECStatus rv;

    /* Write out database with a new password */
    rv = SECU_ChangeKeyDBPassword(handle);
    if (rv) {
	SECU_PrintError(progName, "unable to change key password");
    }
    return rv;
}

static SECStatus DeletePrivateKey (char *nickName)
{
    int rv;

    rv = SECU_DeleteKeyByName (keyHandle, nickName);
    if (rv != SECSuccess)
	fprintf(stderr, "%s: problem deleting private key (%s)\n",
		progName, SECU_Strerror(PR_GetError()));
    return (rv);

}

#endif /* LATER */
