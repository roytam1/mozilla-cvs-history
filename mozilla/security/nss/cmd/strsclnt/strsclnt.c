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
#endif
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>

#include "plgetopt.h"

#include "nspr.h"
#include "prio.h"
#include "prnetdb.h"
#include "prerror.h"

#include "pk11func.h"
#include "secitem.h"
#include "sslproto.h"
#include "nss.h"
#include "ssl.h"

#ifndef PORT_Sprintf
#define PORT_Sprintf sprintf
#endif

#ifndef PORT_Strstr
#define PORT_Strstr strstr
#endif

#ifndef PORT_Malloc
#define PORT_Malloc PR_Malloc
#endif

#define RD_BUF_SIZE (60 * 1024)

int cipherSuites[] = {
    SSL_FORTEZZA_DMS_WITH_FORTEZZA_CBC_SHA,
    SSL_FORTEZZA_DMS_WITH_RC4_128_SHA,
    SSL_RSA_WITH_RC4_128_MD5,
    SSL_RSA_WITH_3DES_EDE_CBC_SHA,
    SSL_RSA_WITH_DES_CBC_SHA,
    SSL_RSA_EXPORT_WITH_RC4_40_MD5,
    SSL_RSA_EXPORT_WITH_RC2_CBC_40_MD5,
    SSL_FORTEZZA_DMS_WITH_NULL_SHA,
    SSL_RSA_WITH_NULL_MD5,
    0
};

/* Include these cipher suite arrays to re-use tstclnt's 
 * cipher selection code.
 */

int ssl2CipherSuites[] = {
    SSL_EN_RC4_128_WITH_MD5,                    /* A */
    SSL_EN_RC4_128_EXPORT40_WITH_MD5,           /* B */
    SSL_EN_RC2_128_CBC_WITH_MD5,                /* C */
    SSL_EN_RC2_128_CBC_EXPORT40_WITH_MD5,       /* D */
    SSL_EN_DES_64_CBC_WITH_MD5,                 /* E */
    SSL_EN_DES_192_EDE3_CBC_WITH_MD5,           /* F */
    0
};

int ssl3CipherSuites[] = {
    SSL_FORTEZZA_DMS_WITH_FORTEZZA_CBC_SHA,     /* a */
    SSL_FORTEZZA_DMS_WITH_RC4_128_SHA,          /* b */
    SSL_RSA_WITH_RC4_128_MD5,                   /* c */
    SSL_RSA_WITH_3DES_EDE_CBC_SHA,              /* d */
    SSL_RSA_WITH_DES_CBC_SHA,                   /* e */
    SSL_RSA_EXPORT_WITH_RC4_40_MD5,             /* f */
    SSL_RSA_EXPORT_WITH_RC2_CBC_40_MD5,         /* g */
    SSL_FORTEZZA_DMS_WITH_NULL_SHA,             /* h */
    SSL_RSA_WITH_NULL_MD5,                      /* i */
    SSL_RSA_FIPS_WITH_3DES_EDE_CBC_SHA,         /* j */
    SSL_RSA_FIPS_WITH_DES_CBC_SHA,              /* k */
    0
};

/* This global string is so that client main can see 
 * which ciphers to use. 
 */

const char *cipherString;

int MakeCertOK;

void
disableSSL2Ciphers(void)
{
    int i;

    /* disable all the SSL2 cipher suites */
    for (i = 0; ssl2CipherSuites[i] != 0;  ++i) {
        SSL_EnableCipher(ssl2CipherSuites[i], SSL_NOT_ALLOWED);
    }
}

void
disableSSL3Ciphers(void)
{
    int i;

    /* disable all the SSL3 cipher suites */
    for (i = 0; ssl3CipherSuites[i] != 0;  ++i) {
        SSL_EnableCipher(ssl3CipherSuites[i], SSL_NOT_ALLOWED);
    }
}

char * ownPasswd( PK11SlotInfo *slot, PRBool retry, void *arg)
{
        char *passwd = NULL;

        if ( (!retry) && arg ) {
                passwd = PL_strdup((char *)arg);
        }

        return passwd;
}

int	stopping;
int	verbose;
SECItem	bigBuf;

#define PRINTF  if (verbose)  printf
#define FPRINTF if (verbose) fprintf

static void
Usage(const char *progName)
{
    fprintf(stderr, 
    	"Usage: %s [-n rsa_nickname] [-p port] [-d dbdir] [-c connections]\n"
	"          [-v] [-f fortezza_nickname] [-2 filename]\n"
	"          [-w dbpasswd] [-C cipher(s)] hostname\n",
	progName);
    exit(1);
}

static void
networkStart(void)
{
#if defined(XP_WIN) && !defined(NSPR20)

    WORD wVersionRequested;  
    WSADATA wsaData; 
    int err; 
    wVersionRequested = MAKEWORD(1, 1); 
 
    err = WSAStartup(wVersionRequested, &wsaData); 
 
    if (err != 0) {
	/* Tell the user that we couldn't find a useable winsock.dll. */ 
	fputs("WSAStartup failed!\n", stderr);
	exit(1);
    }

/* Confirm that the Windows Sockets DLL supports 1.1.*/ 
/* Note that if the DLL supports versions greater */ 
/* than 1.1 in addition to 1.1, it will still return */ 
/* 1.1 in wVersion since that is the version we */ 
/* requested. */ 
 
    if ( LOBYTE( wsaData.wVersion ) != 1 || 
         HIBYTE( wsaData.wVersion ) != 1 ) { 
	/* Tell the user that we couldn't find a useable winsock.dll. */ 
	fputs("wrong winsock version\n", stderr);
	WSACleanup(); 
	exit(1); 
    } 
    /* The Windows Sockets DLL is acceptable. Proceed. */ 

#endif
}

static void
networkEnd(void)
{
#if defined(XP_WIN) && !defined(NSPR20)
    WSACleanup();
#endif
}

static void
errWarn(char * funcString)
{
    PRErrorCode  perr      = PR_GetError();
    const char * errString = SECU_Strerror(perr);

    fprintf(stderr, "%s returned error %d:\n%s\n",
            funcString, perr, errString);
}

static void
errExit(char * funcString)
{
#if defined (XP_WIN) && !defined(NSPR20)
    int          err;
    LPVOID       lpMsgBuf;

    err = WSAGetLastError();
 
    FormatMessage(
	FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
	NULL,
	err,
	MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
	(LPTSTR) &lpMsgBuf,
	0,
	NULL 
    );

    /* Display the string. */
  /*MessageBox( NULL, lpMsgBuf, "GetLastError", MB_OK|MB_ICONINFORMATION ); */
    fprintf(stderr, "%s\n", lpMsgBuf);

    /* Free the buffer. */
    LocalFree( lpMsgBuf );
#endif

    errWarn(funcString);
    exit(1);
}

/* This invokes the "default" AuthCert handler in libssl.
** The only reason to use this one is that it prints out info as it goes. 
*/
static int
mySSLAuthCertificate(void *arg, PRFileDesc *fd, PRBool checkSig,
		     PRBool isServer)
{
    SECStatus rv;
    CERTCertificate *    peerCert;

    peerCert = SSL_PeerCertificate(fd);

    PRINTF("Subject: %s\nIssuer : %s\n", 
           peerCert->subjectName, peerCert->issuerName); 
    /* invoke the "default" AuthCert handler. */
    rv = SSL_AuthCertificate(arg, fd, checkSig, isServer);

    if (rv == SECSuccess) {
	fputs("-- SSL3: Server Certificate Validated.\n", stderr);
    } 
    /* error, if any, will be displayed by the Bad Cert Handler. */
    return rv;  
}

static int /* should be SECStatus but public prototype says int. */
myBadCertHandler( void *arg, PRFileDesc *fd)
{
    int err = PR_GetError();
    fprintf(stderr, "-- SSL: Server Certificate Invalid, err %d.\n%s\n", 
            err, SECU_Strerror(err));
    return (MakeCertOK ? SECSuccess : SECFailure);
}

/* statistics from ssl3_SendClientHello (sch) */
extern long ssl3_sch_sid_cache_hits;
extern long ssl3_sch_sid_cache_misses;
extern long ssl3_sch_sid_cache_not_ok;

/* statistics from ssl3_HandleServerHello (hsh) */
extern long ssl3_hsh_sid_cache_hits;
extern long ssl3_hsh_sid_cache_misses;
extern long ssl3_hsh_sid_cache_not_ok;

/* statistics from ssl3_HandleClientHello (hch) */
extern long ssl3_hch_sid_cache_hits;
extern long ssl3_hch_sid_cache_misses;
extern long ssl3_hch_sid_cache_not_ok;

void 
printSecurityInfo(PRFileDesc *fd)
{
    char * cp;	/* bulk cipher name */
    char * ip;	/* cert issuer DN */
    char * sp;	/* cert subject DN */
    int    op;	/* High, Low, Off */
    int    kp0;	/* total key bits */
    int    kp1;	/* secret key bits */
    int    result;

    static int only_once;

    if (! only_once++ && fd) {
	result = SSL_SecurityStatus(fd, &op, &cp, &kp0, &kp1, &ip, &sp);
	if (result != SECSuccess)
	    return;
#if 0
	PRINTF("bulk cipher %s, %d secret key bits, %d key bits, status: %d\n"
	       "subject DN: %s\n"
	       "issuer  DN: %s\n", cp, kp1, kp0, op, sp, ip);
#else
	PRINTF("bulk cipher %s, %d secret key bits, %d key bits, status: %d\n",
	       cp, kp1, kp0, op, sp, ip);
#endif
	PR_Free(cp);
	PR_Free(ip);
	PR_Free(sp);
    }

    PRINTF("%ld cache hits; %ld cache misses, %ld cache not reusable\n",
    	ssl3_hsh_sid_cache_hits, 
	ssl3_hsh_sid_cache_misses,
	ssl3_hsh_sid_cache_not_ok);

}

/**************************************************************************
** Begin thread management routines and data.
**************************************************************************/

#define MAX_THREADS 32

typedef int startFn(void *a, void *b, int c);

PRLock    * threadLock;
PRCondVar * threadStartQ;
PRCondVar * threadEndQ;

int         numUsed;
int         numRunning;

typedef enum { rs_idle = 0, rs_running = 1, rs_zombie = 2 } runState;

typedef struct perThreadStr {
    void *	a;
    void *	b;
    int         c;
    int         rv;
    startFn  *  startFunc;
    PRThread *  prThread;
    PRBool	inUse;
    runState	running;
} perThread;

perThread threads[MAX_THREADS];

void
thread_wrapper(void * arg)
{
    perThread * slot = (perThread *)arg;

    /* wait for parent to finish launching us before proceeding. */
    PR_Lock(threadLock);
    PR_Unlock(threadLock);

    slot->rv = (* slot->startFunc)(slot->a, slot->b, slot->c);

    /* Handle cleanup of thread here. */
    PRINTF("Thread in slot %d returned %d\n", slot - threads, slot->rv);

    PR_Lock(threadLock);
    slot->running = rs_idle;
    --numRunning;

    /* notify the thread launcher. */
    PR_NotifyCondVar(threadStartQ);

    PR_Unlock(threadLock);
}

SECStatus
launch_thread(
    startFn *	startFunc,
    void *	a,
    void *	b,
    int         c)
{
    perThread * slot;
    int         i;

    if (!threadStartQ) {
	threadLock = PR_NewLock();
	threadStartQ = PR_NewCondVar(threadLock);
	threadEndQ   = PR_NewCondVar(threadLock);
    }
    PR_Lock(threadLock);
    while (numRunning >= MAX_THREADS) {
    	PR_WaitCondVar(threadStartQ, PR_INTERVAL_NO_TIMEOUT);
    }
    for (i = 0; i < numUsed; ++i) {
	slot = threads + i;
    	if (slot->running == rs_idle) 
	    break;
    }
    if (i >= numUsed) {
	if (i >= MAX_THREADS) {
	    /* something's really wrong here. */
	    PORT_Assert(i < MAX_THREADS);
	    PR_Unlock(threadLock);
	    return SECFailure;
	}
	++numUsed;
	PORT_Assert(numUsed == i + 1);
	slot = threads + i;
    }

    slot->a = a;
    slot->b = b;
    slot->c = c;

    slot->startFunc = startFunc;

    slot->prThread      = PR_CreateThread(PR_USER_THREAD,
                                      thread_wrapper, slot,
				      PR_PRIORITY_NORMAL, PR_GLOBAL_THREAD,
				      PR_UNJOINABLE_THREAD, 0);
    if (slot->prThread == NULL) {
	PR_Unlock(threadLock);
	printf("Failed to launch thread!\n");
	return SECFailure;
    } 

    slot->inUse   = 1;
    slot->running = 1;
    ++numRunning;
    PR_Unlock(threadLock);
    PRINTF("Launched thread in slot %d \n", i);

    return SECSuccess;
}

/* Wait until num_running == 0 */
int 
reap_threads(void)
{
    perThread * slot;
    int         i;

    if (!threadLock)
    	return 0;
    PR_Lock(threadLock);
    while (numRunning > 0) {
    	PR_WaitCondVar(threadStartQ, PR_INTERVAL_NO_TIMEOUT);
    }

    /* Safety Sam sez: make sure count is right. */
    for (i = 0; i < numUsed; ++i) {
	slot = threads + i;
    	if (slot->running != rs_idle)  {
	    FPRINTF(stderr, "Thread in slot %d is in state %d!\n", 
	            i, slot->running);
    	}
    }
    PR_Unlock(threadLock);
    return 0;
}

void
destroy_thread_data(void)
{
    PORT_Memset(threads, 0, sizeof threads);

    if (threadEndQ) {
    	PR_DestroyCondVar(threadEndQ);
	threadEndQ = NULL;
    }
    if (threadStartQ) {
    	PR_DestroyCondVar(threadStartQ);
	threadStartQ = NULL;
    }
    if (threadLock) {
    	PR_DestroyLock(threadLock);
	threadLock = NULL;
    }
}

/**************************************************************************
** End   thread management routines.
**************************************************************************/

PRBool useModelSocket = PR_TRUE;

static const char stopCmd[] = { "GET /stop " };
static const char outHeader[] = {
    "HTTP/1.0 200 OK\r\n"
    "Server: Netscape-Enterprise/2.0a\r\n"
    "Date: Tue, 26 Aug 1997 22:10:05 GMT\r\n"
    "Content-type: text/plain\r\n"
    "\r\n"
};

struct lockedVarsStr {
    PRLock *	lock;
    int		count;
    int		waiters;
    PRCondVar *	condVar;
};

typedef struct lockedVarsStr lockedVars;

void 
lockedVars_Init( lockedVars * lv)
{
    lv->count   = 0;
    lv->waiters = 0;
    lv->lock    = PR_NewLock();
    lv->condVar = PR_NewCondVar(lv->lock);
}

void
lockedVars_Destroy( lockedVars * lv)
{
    PR_DestroyCondVar(lv->condVar);
    lv->condVar = NULL;

    PR_DestroyLock(lv->lock);
    lv->lock = NULL;
}

void
lockedVars_WaitForDone(lockedVars * lv)
{
    PR_Lock(lv->lock);
    while (lv->count > 0) {
    	PR_WaitCondVar(lv->condVar, PR_INTERVAL_NO_TIMEOUT);
    }
    PR_Unlock(lv->lock);
}

int	/* returns count */
lockedVars_AddToCount(lockedVars * lv, int addend)
{
    int rv;

    PR_Lock(lv->lock);
    rv = lv->count += addend;
    if (rv <= 0) {
	PR_NotifyCondVar(lv->condVar);
    }
    PR_Unlock(lv->lock);
    return rv;
}

int
do_writes(
    void *       a,
    void *       b,
    int          c)
{
    PRFileDesc *	ssl_sock	= (PRFileDesc *)a;
    lockedVars *	lv 		= (lockedVars *)b;
    int			sent  		= 0;
    int 		count		= 0;

    while (sent < bigBuf.len) {

	count = PR_Write(ssl_sock, bigBuf.data + sent, bigBuf.len - sent);
	if (count < 0) {
	    errWarn("PR_Write bigBuf");
	    break;
	}
	FPRINTF(stderr, "PR_Write wrote %d bytes from bigBuf\n", count );
	sent += count;
    }
    if (count >= 0) {	/* last write didn't fail. */
    	PR_Shutdown(ssl_sock, PR_SHUTDOWN_SEND);
    }

    /* notify the reader that we're done. */
    lockedVars_AddToCount(lv, -1);
    return (sent < bigBuf.len) ? SECFailure : SECSuccess;
}

int 
handle_fdx_connection( PRFileDesc * ssl_sock, int connection)
{
    SECStatus          result;
    int                firstTime = 1;
    int                countRead = 0;
    lockedVars         lv;
    char               *buf;


    lockedVars_Init(&lv);
    lockedVars_AddToCount(&lv, 1);

    /* Attempt to launch the writer thread. */
    result = launch_thread(do_writes, ssl_sock, &lv, connection);

    if (result != SECSuccess) 
    	goto cleanup;

    buf = PR_Malloc(RD_BUF_SIZE);

    if (buf) {
	do {
	    /* do reads here. */
	    PRInt32 count;

	    count = PR_Read(ssl_sock, buf, RD_BUF_SIZE);
	    if (count < 0) {
		errWarn("PR_Read");
		break;
	    }
	    countRead += count;
	    FPRINTF(stderr, "connection %d read %d bytes (%d total).\n", 
		    connection, count, countRead );
	    if (firstTime) {
		firstTime = 0;
		printSecurityInfo(ssl_sock);
	    }
	} while (lockedVars_AddToCount(&lv, 0) > 0);
	PR_Free(buf);
	buf = 0;
    }

    /* Wait for writer to finish */
    lockedVars_WaitForDone(&lv);
    lockedVars_Destroy(&lv);

    FPRINTF(stderr, 
    "connection %d read %d bytes total. -----------------------------\n", 
    	    connection, countRead);

cleanup:
    /* Caller closes the socket. */

    return SECSuccess;
}

const char request[] = {"GET /abc HTTP/1.0\r\n\r\n" };

SECStatus
handle_connection( PRFileDesc *ssl_sock, int connection)
{
    int	    countRead = 0;
    PRInt32 rv;
    char    *buf;

    buf = PR_Malloc(RD_BUF_SIZE);
    if (!buf)
	return SECFailure;

    /* compose the http request here. */

    rv = PR_Write(ssl_sock, request, strlen(request));
    if (rv <= 0) {
	errWarn("PR_Write");
	PR_Free(buf);
	buf = 0;
	return SECFailure;
    }
    printSecurityInfo(ssl_sock);

    /* read until EOF */
    while (1) {
	rv = PR_Read(ssl_sock, buf, RD_BUF_SIZE);
	if (rv == 0) {
	    break;	/* EOF */
	}
	if (rv < 0) {
	    errWarn("PR_Read");
	    break;
	}

	countRead += rv;
	FPRINTF(stderr, "connection %d read %d bytes (%d total).\n", 
		connection, rv, countRead );
    }
    PR_Free(buf);
    buf = 0;

    /* Caller closes the socket. */

    FPRINTF(stderr, 
    "connection %d read %d bytes total. -----------------------------\n", 
    	    connection, countRead);

    return SECSuccess;	/* success */
}

/* one copy of this function is launched in a separate thread for each
** connection to be made.
*/
int
do_connects(
    void *	a,
    void *	b,
    int         connection)
{
    PRNetAddr  *        addr		= (PRNetAddr *)  a;
    PRFileDesc *        model_sock	= (PRFileDesc *) b;
    PRFileDesc *        ssl_sock	= 0;
    PRFileDesc *        tcp_sock	= 0;
    PRStatus	        prStatus;
    SECStatus   	result;
    int                 rv 		= SECSuccess;
    PRSocketOptionData  opt;

retry:

    tcp_sock = PR_NewTCPSocket();
    if (tcp_sock == NULL) {
	errExit("PR_NewTCPSocket");
    }

    opt.option             = PR_SockOpt_Nonblocking;
    opt.value.non_blocking = PR_FALSE;
    prStatus = PR_SetSocketOption(tcp_sock, &opt);
    if (prStatus != PR_SUCCESS) {
    	PR_Close(tcp_sock);
	return SECSuccess;
    } 

    prStatus = PR_Connect(tcp_sock, addr, PR_INTERVAL_NO_TIMEOUT);
    if (prStatus != PR_SUCCESS) {
	PRErrorCode err = PR_GetError();
	if (err == PR_CONNECT_REFUSED_ERROR) {
	    PR_Close(tcp_sock);
	    PR_Sleep(PR_MillisecondsToInterval(10));
	    goto retry;
	}
	errWarn("PR_Connect");
	rv = SECFailure;
	goto done;
    }

    ssl_sock = SSL_ImportFD(model_sock, tcp_sock);
    /* XXX if this import fails, close tcp_sock and return. */
    if (!ssl_sock) {
    	PR_Close(tcp_sock);
	return SECSuccess;
    }

    rv = SSL_ResetHandshake(ssl_sock, /* asServer */ 0);
    if (rv != SECSuccess) {
	errWarn("SSL_ResetHandshake");
	goto done;
    }

    if (bigBuf.data != NULL) {
	result = handle_fdx_connection( ssl_sock, connection);
    } else {
	result = handle_connection( ssl_sock, connection);
    }

done:
    if (ssl_sock) {
	PR_Close(ssl_sock);
    } else if (tcp_sock) {
	PR_Close(tcp_sock);
    }
    return SECSuccess;
}

/* Returns IP address for hostname as PRUint32 in Host Byte Order.
** Since the value returned is an integer (not a string of bytes), 
** it is inherently in Host Byte Order. 
*/
PRUint32
getIPAddress(const char * hostName) 
{
    const unsigned char *p;
    PRStatus	         prStatus;
    PRUint32	         rv;
    PRHostEnt	         prHostEnt;
    char                 scratch[PR_NETDB_BUF_SIZE];

    prStatus = PR_GetHostByName(hostName, scratch, sizeof scratch, &prHostEnt);
    if (prStatus != PR_SUCCESS)
	errExit("PR_GetHostByName");

#undef  h_addr
#define h_addr  h_addr_list[0]   /* address, for backward compatibility */

    p = (const unsigned char *)(prHostEnt.h_addr); /* in Network Byte order */
    FPRINTF(stderr, "%s -> %d.%d.%d.%d\n", hostName, p[0], p[1], p[2], p[3]);
    rv = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
    return rv;
}

void
client_main(
    unsigned short      port, 
    int                 connections, 
    SECKEYPrivateKey ** privKey,
    CERTCertificate **  cert, 
    const char *	hostName,
    char *		nickName)
{
    PRFileDesc *model_sock	= NULL;
    int         i;
    int         rv;
    PRUint32	ipAddress;	/* in host byte order */
    PRNetAddr   addr;

    networkStart();

    /* Assemble NetAddr struct for connections. */
    ipAddress = getIPAddress(hostName);

    addr.inet.family = PR_AF_INET;
    addr.inet.port   = PR_htons(port);
    addr.inet.ip     = PR_htonl(ipAddress);

    /* all suites except RSA_NULL_MD5 are enabled by Domestic Policy */
    NSS_SetDomesticPolicy();

/* all the SSL2 and SSL3 cipher suites are enabled by default. */
    if (cipherString) {
        int ndx;

        /* disable all the ciphers, then enable the ones we want. */
        disableSSL2Ciphers();
        disableSSL3Ciphers();

        while (0 != (ndx = *cipherString++)) {
            int *cptr;
            int  cipher;

            if (! isalpha(ndx))
                Usage("strsclnt");
            cptr = islower(ndx) ? ssl3CipherSuites : ssl2CipherSuites;
            for (ndx &= 0x1f; (cipher = *cptr++) != 0 && --ndx > 0; )
                /* do nothing */;
            if (cipher) {
                SSL_EnableCipher(cipher, SSL_ALLOWED);
            }
        }
    }

    /* configure model SSL socket. */

    model_sock = PR_NewTCPSocket();
    if (model_sock == NULL) {
	errExit("PR_NewTCPSocket on model socket");
    }

    model_sock = SSL_ImportFD(NULL, model_sock);
    if (model_sock == NULL) {
	errExit("SSL_ImportFD");
    }

    /* do SSL configuration. */

    rv = SSL_Enable(model_sock, SSL_SECURITY, 1);
    if (rv < 0) {
	errExit("SSL_Enable SSL_SECURITY");
    }

    if (bigBuf.data) { /* doing FDX */
	rv = SSL_Enable(model_sock, SSL_ENABLE_FDX, 1);
	if (rv < 0) {
	    errExit("SSL_Enable SSL_ENABLE_FDX");
	}
    }

    SSL_SetURL(model_sock, hostName);

    SSL_AuthCertificateHook(model_sock, mySSLAuthCertificate, 
			    (void *)CERT_GetDefaultCertDB());

    SSL_BadCertHook(model_sock, myBadCertHandler, NULL);

    SSL_GetClientAuthDataHook(model_sock, NSS_GetClientAuthData, nickName);

    /* I'm not going to set the HandshakeCallback function. */

    /* end of ssl configuration. */

    rv = launch_thread(do_connects, &addr, model_sock, 1);

    if (connections > 1) {
	/* wait for the first connection to terminate, then launch the rest. */
	reap_threads();
	/* Start up the connections */
	for (i = 2; i <= connections; ++i) {

	    rv = launch_thread(do_connects, &addr, model_sock, i);

	}
    }

    reap_threads();
    destroy_thread_data();

    PR_Close(model_sock);

    networkEnd();
}

SECStatus
readBigFile(const char * fileName)
{
    PRFileInfo  info;
    PRStatus	status;
    SECStatus	rv	= SECFailure;
    int		count;
    int		hdrLen;
    PRFileDesc *local_file_fd = NULL;

    status = PR_GetFileInfo(fileName, &info);

    if (status == PR_SUCCESS &&
	info.type == PR_FILE_FILE &&
	info.size > 0 &&
	NULL != (local_file_fd = PR_Open(fileName, PR_RDONLY, 0))) {

	hdrLen      = PORT_Strlen(outHeader);
	bigBuf.len  = hdrLen + info.size;
	bigBuf.data = PORT_Malloc(bigBuf.len + 4095);
	if (!bigBuf.data) {
	    errWarn("PORT_Malloc");
	    goto done;
	}

	PORT_Memcpy(bigBuf.data, outHeader, hdrLen);

	count = PR_Read(local_file_fd, bigBuf.data + hdrLen, info.size);
	if (count != info.size) {
	    errWarn("PR_Read local file");
	    goto done;
	}
	rv = SECSuccess;
done:
	PR_Close(local_file_fd);
    }
    return rv;
}

int
main(int argc, char **argv)
{
    const char *         dir         = ".";
    char *               fNickName   = NULL;
    const char *         fileName    = NULL;
    char *               hostName    = NULL;
    char *               nickName    = NULL;
    char *               progName    = NULL;
    char *               tmp         = NULL;
    char *		 passwd      = NULL;
    CERTCertificate *    cert   [kt_kea_size] = { NULL };
    SECKEYPrivateKey *   privKey[kt_kea_size] = { NULL };
    int                  connections = 1;
    int                  exitVal;
    unsigned short       port        = 443;
    SECStatus            rv;
    PLOptState *         optstate;
    PLOptStatus          status;

    /* Call the NSPR initialization routines */
    PR_Init( PR_SYSTEM_THREAD, PR_PRIORITY_NORMAL, 1);

    tmp      = strrchr(argv[0], '/');
    tmp      = tmp ? tmp + 1 : argv[0];
    progName = strrchr(tmp, '\\');
    progName = progName ? progName + 1 : tmp;
 

    optstate = PL_CreateOptState(argc, argv, "2:C:c:d:f:n:op:vw:");
    while ((status = PL_GetNextOpt(optstate)) == PL_OPT_OK) {
	switch(optstate->option) {

	case '2':
	    fileName = optstate->value;
	    break;
	case 'C':
	    cipherString = optstate->value;
	    break;

	case 'c':
	    connections = PORT_Atoi(optstate->value);
	    break;

	case 'd':
	    dir = optstate->value;
	    break;

	case 'f':
	    fNickName = optstate->value;
	    break;

        case 'n':
	    nickName = optstate->value;
	    break;
	case 'o':
	    MakeCertOK = 1;
	    break;

	case 'p':
	    port = PORT_Atoi(optstate->value);
	    break;

        case 'v':
	    verbose++;
	    break;
	case 'w':
	    passwd = optstate->value;
	    break;
	case '\0':
	    hostName = PL_strdup(optstate->value);
	    break;
	default:
	case '?':
	    Usage(progName);
	    break;

	}
    }
    if (!hostName || status == PL_OPT_BAD)
    	Usage(progName);

    if (port == 0)
	Usage(progName);

    if (fileName)
    	readBigFile(fileName);

    /* set our password function */
    if ( passwd ) {
	PK11_SetPasswordFunc(ownPasswd);
    } else {
	PK11_SetPasswordFunc(SECU_GetModulePassword);
    }

    /* Call the libsec initialization routines */
    rv = NSS_Init(dir);
    if (rv != SECSuccess) {
    	fputs("NSS_Init failed.\n", stderr);
	exit(1);
    }

    if (nickName) {

	cert[kt_rsa] = PK11_FindCertFromNickname(nickName, passwd);
	if (cert[kt_rsa] == NULL) {
	    fprintf(stderr, "Can't find certificate %s\n", nickName);
	    exit(1);
	}

	privKey[kt_rsa] = PK11_FindKeyByAnyCert(cert[kt_rsa], passwd);
	if (privKey[kt_rsa] == NULL) {
	    fprintf(stderr, "Can't find Private Key for cert %s\n", nickName);
	    exit(1);
	}

    }
    if (fNickName) {
	cert[kt_fortezza] = PK11_FindCertFromNickname(fNickName, passwd);
	if (cert[kt_fortezza] == NULL) {
	    fprintf(stderr, "Can't find certificate %s\n", fNickName);
	    exit(1);
	}

	privKey[kt_fortezza] = PK11_FindKeyByAnyCert(cert[kt_fortezza], passwd);
	if (privKey[kt_fortezza] == NULL) {
	    fprintf(stderr, "Can't find Private Key for cert %s\n", fNickName);
	    exit(1);
	}
    }

    client_main(port, connections, privKey, cert, hostName, nickName);

    /* some final stats. */
    printf("%ld cache hits; %ld cache misses, %ld cache not reusable\n",
    	ssl3_hsh_sid_cache_hits, 
	ssl3_hsh_sid_cache_misses,
	ssl3_hsh_sid_cache_not_ok);
    exitVal = (ssl3_hsh_sid_cache_misses != 1) ||
              (ssl3_hsh_sid_cache_not_ok != 0);

    NSS_Shutdown();
    PR_Cleanup();
    return exitVal;
}

