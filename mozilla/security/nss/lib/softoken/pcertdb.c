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
 * Permanent Certificate database handling code 
 *
 * $Id$
 */
#include "prtime.h"

#include "lowkeyti.h"
#include "pcert.h"
#include "mcom_db.h"
#include "pcert.h"
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

#include "cdbhdl.h"

/* forward declaration */
NSSLOWCERTCertificate *
nsslowcert_FindCertByDERCertNoLocking(NSSLOWCERTCertDBHandle *handle, SECItem *derCert);

/*
 * the following functions are wrappers for the db library that implement
 * a global lock to make the database thread safe.
 */
static PZLock *dbLock = NULL;

void
certdb_InitDBLock(NSSLOWCERTCertDBHandle *handle)
{
    if (dbLock == NULL) {
	nss_InitLock(&dbLock, nssILockCertDB);
	PORT_Assert(dbLock != NULL);
    }

    return;
}

/*
 * Acquire the global lock on the cert database.
 * This lock is currently used for the following operations:
 *	adding or deleting a cert to either the temp or perm databases
 *	converting a temp to perm or perm to temp
 *	changing (maybe just adding!?) the trust of a cert
 *      chaning the DB status checking Configuration
 */
static void
nsslowcert_LockDB(NSSLOWCERTCertDBHandle *handle)
{
    PZ_EnterMonitor(handle->dbMon);
    return;
}

/*
 * Free the global cert database lock.
 */
static void
nsslowcert_UnlockDB(NSSLOWCERTCertDBHandle *handle)
{
    PRStatus prstat;
    
    prstat = PZ_ExitMonitor(handle->dbMon);
    
    PORT_Assert(prstat == PR_SUCCESS);
    
    return;
}

static PZLock *certRefCountLock = NULL;

/*
 * Acquire the cert reference count lock
 * There is currently one global lock for all certs, but I'm putting a cert
 * arg here so that it will be easy to make it per-cert in the future if
 * that turns out to be necessary.
 */
static void
nsslowcert_LockCertRefCount(NSSLOWCERTCertificate *cert)
{
    if ( certRefCountLock == NULL ) {
	nss_InitLock(&certRefCountLock, nssILockRefLock);
	PORT_Assert(certRefCountLock != NULL);
    }
    
    PZ_Lock(certRefCountLock);
    return;
}

/*
 * Free the cert reference count lock
 */
static void
nsslowcert_UnlockCertRefCount(NSSLOWCERTCertificate *cert)
{
    PRStatus prstat;

    PORT_Assert(certRefCountLock != NULL);
    
    prstat = PZ_Unlock(certRefCountLock);
    
    PORT_Assert(prstat == PR_SUCCESS);

    return;
}

static PZLock *certTrustLock = NULL;

/*
 * Acquire the cert trust lock
 * There is currently one global lock for all certs, but I'm putting a cert
 * arg here so that it will be easy to make it per-cert in the future if
 * that turns out to be necessary.
 */
void
nsslowcert_LockCertTrust(NSSLOWCERTCertificate *cert)
{
    if ( certTrustLock == NULL ) {
	nss_InitLock(&certTrustLock, nssILockCertDB);
	PORT_Assert(certTrustLock != NULL);
    }
    
    PZ_Lock(certTrustLock);
    return;
}

/*
 * Free the cert trust lock
 */
void
nsslowcert_UnlockCertTrust(NSSLOWCERTCertificate *cert)
{
    PRStatus prstat;

    PORT_Assert(certTrustLock != NULL);
    
    prstat = PZ_Unlock(certTrustLock);
    
    PORT_Assert(prstat == PR_SUCCESS);

    return;
}

NSSLOWCERTCertificate *
nsslowcert_DupCertificate(NSSLOWCERTCertificate *c)
{
    if (c) {
	nsslowcert_LockCertRefCount(c);
	++c->referenceCount;
	nsslowcert_UnlockCertRefCount(c);
    }
    return c;
}

static int
certdb_Get(DB *db, DBT *key, DBT *data, unsigned int flags)
{
    PRStatus prstat;
    int ret;
    
    PORT_Assert(dbLock != NULL);
    PZ_Lock(dbLock);
    
    ret = (* db->get)(db, key, data, flags);

    prstat = PZ_Unlock(dbLock);

    return(ret);
}

static int
certdb_Put(DB *db, DBT *key, DBT *data, unsigned int flags)
{
    PRStatus prstat;
    int ret;

    PORT_Assert(dbLock != NULL);
    PZ_Lock(dbLock);

    ret = (* db->put)(db, key, data, flags);
    
    prstat = PZ_Unlock(dbLock);

    return(ret);
}

static int
certdb_Sync(DB *db, unsigned int flags)
{
    PRStatus prstat;
    int ret;

    PORT_Assert(dbLock != NULL);
    PZ_Lock(dbLock);

    ret = (* db->sync)(db, flags);
    
    prstat = PZ_Unlock(dbLock);

    return(ret);
}

static int
certdb_Del(DB *db, DBT *key, unsigned int flags)
{
    PRStatus prstat;
    int ret;

    PORT_Assert(dbLock != NULL);
    PZ_Lock(dbLock);

    ret = (* db->del)(db, key, flags);
    
    prstat = PZ_Unlock(dbLock);

    return(ret);
}

static int
certdb_Seq(DB *db, DBT *key, DBT *data, unsigned int flags)
{
    PRStatus prstat;
    int ret;
    
    PORT_Assert(dbLock != NULL);
    PZ_Lock(dbLock);
    
    ret = (* db->seq)(db, key, data, flags);

    prstat = PZ_Unlock(dbLock);

    return(ret);
}

static void
certdb_Close(DB *db)
{
    PRStatus prstat;

    PORT_Assert(dbLock != NULL);
    PZ_Lock(dbLock);

    (* db->close)(db);
    
    prstat = PZ_Unlock(dbLock);

    return;
}

/* forward references */
static void nsslowcert_DestroyCertificateNoLocking(NSSLOWCERTCertificate *cert);

static SECStatus
DeleteDBEntry(NSSLOWCERTCertDBHandle *handle, certDBEntryType type, SECItem *dbkey)
{
    DBT key;
    int ret;

    /* init the database key */
    key.data = dbkey->data;
    key.size = dbkey->len;
    
    dbkey->data[0] = (unsigned char)type;

    /* delete entry from database */
    ret = certdb_Del(handle->permCertDB, &key, 0 );
    if ( ret != 0 ) {
	PORT_SetError(SEC_ERROR_BAD_DATABASE);
	goto loser;
    }

    ret = certdb_Sync(handle->permCertDB, 0);
    if ( ret ) {
	PORT_SetError(SEC_ERROR_BAD_DATABASE);
	goto loser;
    }

    return(SECSuccess);
    
loser:
    return(SECFailure);
}

static SECStatus
ReadDBEntry(NSSLOWCERTCertDBHandle *handle, certDBEntryCommon *entry,
	    SECItem *dbkey, SECItem *dbentry, PRArenaPool *arena)
{
    DBT data, key;
    int ret;
    unsigned char *buf;
    
    /* init the database key */
    key.data = dbkey->data;
    key.size = dbkey->len;
    
    dbkey->data[0] = (unsigned char)entry->type;

    /* read entry from database */
    ret = certdb_Get(handle->permCertDB, &key, &data, 0 );
    if ( ret != 0 ) {
	PORT_SetError(SEC_ERROR_BAD_DATABASE);
	goto loser;
    }
    
    /* validate the entry */
    if ( data.size < SEC_DB_ENTRY_HEADER_LEN ) {
	PORT_SetError(SEC_ERROR_BAD_DATABASE);
	goto loser;
    }
    buf = (unsigned char *)data.data;
    if ( buf[0] != (unsigned char)CERT_DB_FILE_VERSION ) {
	PORT_SetError(SEC_ERROR_BAD_DATABASE);
	goto loser;
    }
    if ( buf[1] != (unsigned char)entry->type ) {
	PORT_SetError(SEC_ERROR_BAD_DATABASE);
	goto loser;
    }

    /* copy out header information */
    entry->version = (unsigned int)buf[0];
    entry->type = (certDBEntryType)buf[1];
    entry->flags = (unsigned int)buf[2];
    
    /* format body of entry for return to caller */
    dbentry->len = data.size - SEC_DB_ENTRY_HEADER_LEN;
    if ( dbentry->len ) {
	dbentry->data = (unsigned char *)PORT_ArenaAlloc(arena, dbentry->len);
	if ( dbentry->data == NULL ) {
	    PORT_SetError(SEC_ERROR_NO_MEMORY);
	    goto loser;
	}
    
	PORT_Memcpy(dbentry->data, &buf[SEC_DB_ENTRY_HEADER_LEN],
		  dbentry->len);
    } else {
	dbentry->data = NULL;
    }
    
    return(SECSuccess);

loser:
    return(SECFailure);
}

/**
 ** Implement low level database access
 **/
static SECStatus
WriteDBEntry(NSSLOWCERTCertDBHandle *handle, certDBEntryCommon *entry,
	     SECItem *dbkey, SECItem *dbentry)
{
    int ret;
    DBT data, key;
    unsigned char *buf;
    
    data.data = dbentry->data;
    data.size = dbentry->len;
    
    buf = (unsigned char*)data.data;
    
    buf[0] = (unsigned char)entry->version;
    buf[1] = (unsigned char)entry->type;
    buf[2] = (unsigned char)entry->flags;
    
    key.data = dbkey->data;
    key.size = dbkey->len;
    
    dbkey->data[0] = (unsigned char)entry->type;

    /* put the record into the database now */
    ret = certdb_Put(handle->permCertDB, &key, &data, 0);

    if ( ret != 0 ) {
	goto loser;
    }

    ret = certdb_Sync( handle->permCertDB, 0 );
    
    if ( ret ) {
	goto loser;
    }

    return(SECSuccess);

loser:
    return(SECFailure);
}

/*
 * encode a database cert record
 */
static SECStatus
EncodeDBCertEntry(certDBEntryCert *entry, PRArenaPool *arena, SECItem *dbitem)
{
    unsigned int nnlen;
    unsigned char *buf;
    char *nn;
    char zbuf = 0;
    
    if ( entry->nickname ) {
	nn = entry->nickname;
    } else {
	nn = &zbuf;
    }
    nnlen = PORT_Strlen(nn) + 1;
    
    /* allocate space for encoded database record, including space
     * for low level header
     */
    dbitem->len = entry->derCert.len + nnlen + DB_CERT_ENTRY_HEADER_LEN +
	SEC_DB_ENTRY_HEADER_LEN;
    
    dbitem->data = (unsigned char *)PORT_ArenaAlloc(arena, dbitem->len);
    if ( dbitem->data == NULL) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }
    
    /* fill in database record */
    buf = &dbitem->data[SEC_DB_ENTRY_HEADER_LEN];
    
    buf[0] = ( entry->trust.sslFlags >> 8 ) & 0xff;
    buf[1] = entry->trust.sslFlags & 0xff;
    buf[2] = ( entry->trust.emailFlags >> 8 ) & 0xff;
    buf[3] = entry->trust.emailFlags & 0xff;
    buf[4] = ( entry->trust.objectSigningFlags >> 8 ) & 0xff;
    buf[5] = entry->trust.objectSigningFlags & 0xff;
    buf[6] = ( entry->derCert.len >> 8 ) & 0xff;
    buf[7] = entry->derCert.len & 0xff;
    buf[8] = ( nnlen >> 8 ) & 0xff;
    buf[9] = nnlen & 0xff;
    
    PORT_Memcpy(&buf[DB_CERT_ENTRY_HEADER_LEN], entry->derCert.data,
	      entry->derCert.len);

    PORT_Memcpy(&buf[DB_CERT_ENTRY_HEADER_LEN + entry->derCert.len],
	      nn, nnlen);

    return(SECSuccess);

loser:
    return(SECFailure);
}

/*
 * encode a database key for a cert record
 */
static SECStatus
EncodeDBCertKey(SECItem *certKey, PRArenaPool *arena, SECItem *dbkey)
{
    dbkey->len = certKey->len + SEC_DB_KEY_HEADER_LEN;
    dbkey->data = (unsigned char *)PORT_ArenaAlloc(arena, dbkey->len);
    if ( dbkey->data == NULL ) {
	goto loser;
    }
    PORT_Memcpy(&dbkey->data[SEC_DB_KEY_HEADER_LEN],
	      certKey->data, certKey->len);
    dbkey->data[0] = certDBEntryTypeCert;

    return(SECSuccess);
loser:
    return(SECFailure);
}

static SECStatus
EncodeDBGenericKey(SECItem *certKey, PRArenaPool *arena, SECItem *dbkey, 
				certDBEntryType entryType)
{
    /*
     * we only allow _one_ KRL key!
     */
    if (entryType == certDBEntryTypeKeyRevocation) {
	dbkey->len = SEC_DB_KEY_HEADER_LEN;
 	dbkey->data = (unsigned char *)PORT_ArenaAlloc(arena, dbkey->len);
	if ( dbkey->data == NULL ) {
	    goto loser;
	}
        dbkey->data[0] = (unsigned char) entryType;
        return(SECSuccess);
    }
    

    dbkey->len = certKey->len + SEC_DB_KEY_HEADER_LEN;
    dbkey->data = (unsigned char *)PORT_ArenaAlloc(arena, dbkey->len);
    if ( dbkey->data == NULL ) {
	goto loser;
    }
    PORT_Memcpy(&dbkey->data[SEC_DB_KEY_HEADER_LEN],
	      certKey->data, certKey->len);
    dbkey->data[0] = (unsigned char) entryType;

    return(SECSuccess);
loser:
    return(SECFailure);
}

static SECStatus
DecodeDBCertEntry(certDBEntryCert *entry, SECItem *dbentry)
{
    unsigned int nnlen;
    int headerlen;
    int lenoff;

    /* allow updates of old versions of the database */
    switch ( entry->common.version ) {
      case 5:
	headerlen = DB_CERT_V5_ENTRY_HEADER_LEN;
	lenoff = 3;
	break;
      case 6:
	/* should not get here */
	PORT_Assert(0);
	headerlen = DB_CERT_V6_ENTRY_HEADER_LEN;
	lenoff = 3;
	break;
      case 7:
	headerlen = DB_CERT_ENTRY_HEADER_LEN;
	lenoff = 6;
	break;
      default:
	/* better not get here */
	PORT_Assert(0);
	headerlen = DB_CERT_V5_ENTRY_HEADER_LEN;
	lenoff = 3;
	break;
    }
    
    /* is record long enough for header? */
    if ( dbentry->len < headerlen ) {
	PORT_SetError(SEC_ERROR_BAD_DATABASE);
	goto loser;
    }
    
    /* is database entry correct length? */
    entry->derCert.len = ( ( dbentry->data[lenoff] << 8 ) |
			  dbentry->data[lenoff+1] );
    nnlen = ( ( dbentry->data[lenoff+2] << 8 ) | dbentry->data[lenoff+3] );
    if ( ( entry->derCert.len + nnlen + headerlen )
	!= dbentry->len) {
	PORT_SetError(SEC_ERROR_BAD_DATABASE);
	goto loser;
    }
    
    /* copy the dercert */
    entry->derCert.data = (unsigned char *)PORT_ArenaAlloc(entry->common.arena,
							   entry->derCert.len);
    if ( entry->derCert.data == NULL ) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }
    PORT_Memcpy(entry->derCert.data, &dbentry->data[headerlen],
	      entry->derCert.len);

    /* copy the nickname */
    if ( nnlen > 1 ) {
	entry->nickname = (char *)PORT_ArenaAlloc(entry->common.arena, nnlen);
	if ( entry->nickname == NULL ) {
	    PORT_SetError(SEC_ERROR_NO_MEMORY);
	    goto loser;
	}
	PORT_Memcpy(entry->nickname,
		    &dbentry->data[headerlen +
				   entry->derCert.len],
		    nnlen);
    } else {
	entry->nickname = NULL;
    }
    
    if ( entry->common.version < 7 ) {
	/* allow updates of v5 db */
	entry->trust.sslFlags = dbentry->data[0];
	entry->trust.emailFlags = dbentry->data[1];
	entry->trust.objectSigningFlags = dbentry->data[2];
    } else {
	entry->trust.sslFlags = ( dbentry->data[0] << 8 ) | dbentry->data[1];
	entry->trust.emailFlags = ( dbentry->data[2] << 8 ) | dbentry->data[3];
	entry->trust.objectSigningFlags =
	    ( dbentry->data[4] << 8 ) | dbentry->data[5];
    }
    
    return(SECSuccess);
loser:
    return(SECFailure);
}


/*
 * Create a new certDBEntryCert from existing data
 */
static certDBEntryCert *
NewDBCertEntry(SECItem *derCert, char *nickname,
	       NSSLOWCERTCertTrust *trust, int flags)
{
    certDBEntryCert *entry;
    PRArenaPool *arena = NULL;
    int nnlen;
    
    arena = PORT_NewArena( DER_DEFAULT_CHUNKSIZE );

    if ( !arena ) {
	goto loser;
    }
	
    entry = (certDBEntryCert *)PORT_ArenaZAlloc(arena, sizeof(certDBEntryCert));

    if ( entry == NULL ) {
	goto loser;
    }
    
    /* fill in the dbCert */
    entry->common.arena = arena;
    entry->common.type = certDBEntryTypeCert;
    entry->common.version = CERT_DB_FILE_VERSION;
    entry->common.flags = flags;
    
    if ( trust ) {
	entry->trust = *trust;
    }

    entry->derCert.data = (unsigned char *)PORT_ArenaAlloc(arena, derCert->len);
    if ( !entry->derCert.data ) {
	goto loser;
    }
    entry->derCert.len = derCert->len;
    PORT_Memcpy(entry->derCert.data, derCert->data, derCert->len);
    
    nnlen = ( nickname ? strlen(nickname) + 1 : 0 );
    
    if ( nnlen ) {
	entry->nickname = (char *)PORT_ArenaAlloc(arena, nnlen);
	if ( !entry->nickname ) {
	    goto loser;
	}
	PORT_Memcpy(entry->nickname, nickname, nnlen);
	
    } else {
	entry->nickname = 0;
    }

    return(entry);

loser:
    
    /* allocation error, free arena and return */
    if ( arena ) {
	PORT_FreeArena(arena, PR_FALSE);
    }
    
    PORT_SetError(SEC_ERROR_NO_MEMORY);
    return(0);
}

/*
 * Decode a version 4 DBCert from the byte stream database format
 * and construct a current database entry struct
 */
static certDBEntryCert *
DecodeV4DBCertEntry(unsigned char *buf, int len)
{
    certDBEntryCert *entry;
    int certlen;
    int nnlen;
    PRArenaPool *arena;
    
    /* make sure length is at least long enough for the header */
    if ( len < DBCERT_V4_HEADER_LEN ) {
	PORT_SetError(SEC_ERROR_BAD_DATABASE);
	return(0);
    }

    /* get other lengths */
    certlen = buf[3] << 8 | buf[4];
    nnlen = buf[5] << 8 | buf[6];
    
    /* make sure DB entry is the right size */
    if ( ( certlen + nnlen + DBCERT_V4_HEADER_LEN ) != len ) {
	PORT_SetError(SEC_ERROR_BAD_DATABASE);
	return(0);
    }

    /* allocate arena */
    arena = PORT_NewArena( DER_DEFAULT_CHUNKSIZE );

    if ( !arena ) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	return(0);
    }
	
    /* allocate structure and members */
    entry = (certDBEntryCert *)  PORT_ArenaAlloc(arena, sizeof(certDBEntryCert));

    if ( !entry ) {
	goto loser;
    }

    entry->derCert.data = (unsigned char *)PORT_ArenaAlloc(arena, certlen);
    if ( !entry->derCert.data ) {
	goto loser;
    }
    entry->derCert.len = certlen;
    
    if ( nnlen ) {
	entry->nickname = (char *) PORT_ArenaAlloc(arena, nnlen);
	if ( !entry->nickname ) {
	    goto loser;
	}
    } else {
	entry->nickname = 0;
    }

    entry->common.arena = arena;
    entry->common.version = CERT_DB_FILE_VERSION;
    entry->common.type = certDBEntryTypeCert;
    entry->common.flags = 0;
    entry->trust.sslFlags = buf[0];
    entry->trust.emailFlags = buf[1];
    entry->trust.objectSigningFlags = buf[2];

    PORT_Memcpy(entry->derCert.data, &buf[DBCERT_V4_HEADER_LEN], certlen);
    PORT_Memcpy(entry->nickname, &buf[DBCERT_V4_HEADER_LEN + certlen], nnlen);

    if (PORT_Strcmp(entry->nickname,"Server-Cert") == 0) {
	entry->trust.sslFlags |= CERTDB_USER;
    }

    return(entry);
    
loser:
    PORT_FreeArena(arena, PR_FALSE);
    PORT_SetError(SEC_ERROR_NO_MEMORY);
    return(0);
}

/*
 * Encode a Certificate database entry into byte stream suitable for
 * the database
 */
static SECStatus
WriteDBCertEntry(NSSLOWCERTCertDBHandle *handle, certDBEntryCert *entry)
{
    SECItem dbitem, dbkey;
    PRArenaPool *tmparena = NULL;
    SECItem tmpitem;
    SECStatus rv;
    
    tmparena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( tmparena == NULL ) {
	goto loser;
    }
    
    rv = EncodeDBCertEntry(entry, tmparena, &dbitem);
    if ( rv != SECSuccess ) {
	goto loser;
    }

    /* get the database key and format it */
    rv = nsslowcert_KeyFromDERCert(tmparena, &entry->derCert, &tmpitem);
    if ( rv == SECFailure ) {
	goto loser;
    }

    rv = EncodeDBCertKey(&tmpitem, tmparena, &dbkey);
    if ( rv == SECFailure ) {
	goto loser;
    }
    
    /* now write it to the database */
    rv = WriteDBEntry(handle, &entry->common, &dbkey, &dbitem);
    if ( rv != SECSuccess ) {
	goto loser;
    }
    
    PORT_FreeArena(tmparena, PR_FALSE);
    return(SECSuccess);

loser:
    if ( tmparena ) {
	PORT_FreeArena(tmparena, PR_FALSE);
    }
    return(SECFailure);
}


/*
 * delete a certificate entry
 */
static SECStatus
DeleteDBCertEntry(NSSLOWCERTCertDBHandle *handle, SECItem *certKey)
{
    SECItem dbkey;
    PRArenaPool *arena = NULL;
    SECStatus rv;
    
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( arena == NULL ) {
	goto loser;
    }

    rv = EncodeDBCertKey(certKey, arena, &dbkey);
    if ( rv != SECSuccess ) {
	goto loser;
    }
    
    rv = DeleteDBEntry(handle, certDBEntryTypeCert, &dbkey);
    if ( rv == SECFailure ) {
	goto loser;
    }

    PORT_FreeArena(arena, PR_FALSE);
    return(SECSuccess);

loser:
    if ( arena ) {
	PORT_FreeArena(arena, PR_FALSE);
    }
    
    return(SECFailure);
}

/*
 * Read a certificate entry
 */
static certDBEntryCert *
ReadDBCertEntry(NSSLOWCERTCertDBHandle *handle, SECItem *certKey)
{
    PRArenaPool *arena = NULL;
    PRArenaPool *tmparena = NULL;
    certDBEntryCert *entry;
    SECItem dbkey;
    SECItem dbentry;
    SECStatus rv;
    
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( arena == NULL ) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }

    tmparena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( tmparena == NULL ) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }
    
    entry = (certDBEntryCert *)PORT_ArenaAlloc(arena, sizeof(certDBEntryCert));
    if ( entry == NULL ) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }
    entry->common.arena = arena;
    entry->common.type = certDBEntryTypeCert;

    rv = EncodeDBCertKey(certKey, tmparena, &dbkey);
    if ( rv != SECSuccess ) {
	goto loser;
    }
    
    rv = ReadDBEntry(handle, &entry->common, &dbkey, &dbentry, tmparena);
    if ( rv == SECFailure ) {
	goto loser;
    }

    rv = DecodeDBCertEntry(entry, &dbentry);
    if ( rv != SECSuccess ) {
	goto loser;
    }
    
    PORT_FreeArena(tmparena, PR_FALSE);
    return(entry);
    
loser:
    if ( tmparena ) {
	PORT_FreeArena(tmparena, PR_FALSE);
    }
    if ( arena ) {
	PORT_FreeArena(arena, PR_FALSE);
    }
    
    return(NULL);
}

/*
 * encode a database cert record
 */
static SECStatus
EncodeDBCrlEntry(certDBEntryRevocation *entry, PRArenaPool *arena, SECItem *dbitem)
{
    unsigned int nnlen = 0;
    unsigned char *buf;
  
    if (entry->url) {  
	nnlen = PORT_Strlen(entry->url) + 1;
    }
    
    /* allocate space for encoded database record, including space
     * for low level header
     */
    dbitem->len = entry->derCrl.len + nnlen 
		+ SEC_DB_ENTRY_HEADER_LEN + DB_CRL_ENTRY_HEADER_LEN;
    
    dbitem->data = (unsigned char *)PORT_ArenaAlloc(arena, dbitem->len);
    if ( dbitem->data == NULL) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }
    
    /* fill in database record */
    buf = &dbitem->data[SEC_DB_ENTRY_HEADER_LEN];
    
    buf[0] = ( entry->derCrl.len >> 8 ) & 0xff;
    buf[1] = entry->derCrl.len & 0xff;
    buf[2] = ( nnlen >> 8 ) & 0xff;
    buf[3] = nnlen & 0xff;
    
    PORT_Memcpy(&buf[DB_CRL_ENTRY_HEADER_LEN], entry->derCrl.data,
	      entry->derCrl.len);

    if (nnlen != 0) {
	PORT_Memcpy(&buf[DB_CRL_ENTRY_HEADER_LEN + entry->derCrl.len],
	      entry->url, nnlen);
    }

    return(SECSuccess);

loser:
    return(SECFailure);
}

static SECStatus
DecodeDBCrlEntry(certDBEntryRevocation *entry, SECItem *dbentry)
{
    unsigned int nnlen;
    
    /* is record long enough for header? */
    if ( dbentry->len < DB_CRL_ENTRY_HEADER_LEN ) {
	PORT_SetError(SEC_ERROR_BAD_DATABASE);
	goto loser;
    }
    
    /* is database entry correct length? */
    entry->derCrl.len = ( ( dbentry->data[0] << 8 ) | dbentry->data[1] );
    nnlen = ( ( dbentry->data[2] << 8 ) | dbentry->data[3] );
    if ( ( entry->derCrl.len + nnlen + DB_CRL_ENTRY_HEADER_LEN )
	!= dbentry->len) {
      /* CRL entry is greater than 64 K. Hack to make this continue to work */
      if (dbentry->len >= (0xffff - DB_CRL_ENTRY_HEADER_LEN) - nnlen) {
          entry->derCrl.len = 
                      (dbentry->len - DB_CRL_ENTRY_HEADER_LEN) - nnlen;
      } else {
          PORT_SetError(SEC_ERROR_BAD_DATABASE);
          goto loser;
      }    
    }
    
    /* copy the dercert */
    entry->derCrl.data = (unsigned char *)PORT_ArenaAlloc(entry->common.arena,
							 entry->derCrl.len);
    if ( entry->derCrl.data == NULL ) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }
    PORT_Memcpy(entry->derCrl.data, &dbentry->data[DB_CRL_ENTRY_HEADER_LEN],
	      entry->derCrl.len);

    /* copy the url */
    entry->url = NULL;
    if (nnlen != 0) {
	entry->url = (char *)PORT_ArenaAlloc(entry->common.arena, nnlen);
	if ( entry->url == NULL ) {
	    PORT_SetError(SEC_ERROR_NO_MEMORY);
	    goto loser;
	}
	PORT_Memcpy(entry->url,
	      &dbentry->data[DB_CRL_ENTRY_HEADER_LEN + entry->derCrl.len],
	      nnlen);
    }
    
    return(SECSuccess);
loser:
    return(SECFailure);
}

/*
 * Create a new certDBEntryRevocation from existing data
 */
static certDBEntryRevocation *
NewDBCrlEntry(SECItem *derCrl, char * url, certDBEntryType crlType, int flags)
{
    certDBEntryRevocation *entry;
    PRArenaPool *arena = NULL;
    int nnlen;
    
    arena = PORT_NewArena( DER_DEFAULT_CHUNKSIZE );

    if ( !arena ) {
	goto loser;
    }
	
    entry = (certDBEntryRevocation*)
			PORT_ArenaZAlloc(arena, sizeof(certDBEntryRevocation));

    if ( entry == NULL ) {
	goto loser;
    }
    
    /* fill in the dbRevolcation */
    entry->common.arena = arena;
    entry->common.type = crlType;
    entry->common.version = CERT_DB_FILE_VERSION;
    entry->common.flags = flags;
    

    entry->derCrl.data = (unsigned char *)PORT_ArenaAlloc(arena, derCrl->len);
    if ( !entry->derCrl.data ) {
	goto loser;
    }

    if (url) {
	nnlen = PORT_Strlen(url) + 1;
	entry->url  = (char *)PORT_ArenaAlloc(arena, nnlen);
	if ( !entry->url ) {
	    goto loser;
	}
	PORT_Memcpy(entry->url, url, nnlen);
    } else {
	entry->url = NULL;
    }

	
    entry->derCrl.len = derCrl->len;
    PORT_Memcpy(entry->derCrl.data, derCrl->data, derCrl->len);

    return(entry);

loser:
    
    /* allocation error, free arena and return */
    if ( arena ) {
	PORT_FreeArena(arena, PR_FALSE);
    }
    
    PORT_SetError(SEC_ERROR_NO_MEMORY);
    return(0);
}


static SECStatus
WriteDBCrlEntry(NSSLOWCERTCertDBHandle *handle, certDBEntryRevocation *entry,
				SECItem *crlKey )
{
    SECItem dbkey;
    PRArenaPool *tmparena = NULL;
    SECItem encodedEntry;
    SECStatus rv;
    
    tmparena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( tmparena == NULL ) {
	goto loser;
    }

    rv = EncodeDBCrlEntry(entry, tmparena, &encodedEntry);
    if ( rv == SECFailure ) {
	goto loser;
    }

    rv = EncodeDBGenericKey(crlKey, tmparena, &dbkey, entry->common.type);
    if ( rv == SECFailure ) {
	goto loser;
    }
    
    /* now write it to the database */
    rv = WriteDBEntry(handle, &entry->common, &dbkey, &encodedEntry);
    if ( rv != SECSuccess ) {
	goto loser;
    }
    
    PORT_FreeArena(tmparena, PR_FALSE);
    return(SECSuccess);

loser:
    if ( tmparena ) {
	PORT_FreeArena(tmparena, PR_FALSE);
    }
    return(SECFailure);
}
/*
 * delete a crl entry
 */
static SECStatus
DeleteDBCrlEntry(NSSLOWCERTCertDBHandle *handle, SECItem *crlKey, 
						certDBEntryType crlType)
{
    SECItem dbkey;
    PRArenaPool *arena = NULL;
    SECStatus rv;
    
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( arena == NULL ) {
	goto loser;
    }

    rv = EncodeDBGenericKey(crlKey, arena, &dbkey, crlType);
    if ( rv != SECSuccess ) {
	goto loser;
    }
    
    rv = DeleteDBEntry(handle, crlType, &dbkey);
    if ( rv == SECFailure ) {
	goto loser;
    }

    PORT_FreeArena(arena, PR_FALSE);
    return(SECSuccess);

loser:
    if ( arena ) {
	PORT_FreeArena(arena, PR_FALSE);
    }
    
    return(SECFailure);
}

/*
 * Read a certificate entry
 */
static certDBEntryRevocation *
ReadDBCrlEntry(NSSLOWCERTCertDBHandle *handle, SECItem *certKey,
						certDBEntryType crlType)
{
    PRArenaPool *arena = NULL;
    PRArenaPool *tmparena = NULL;
    certDBEntryRevocation *entry;
    SECItem dbkey;
    SECItem dbentry;
    SECStatus rv;
    
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( arena == NULL ) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }

    tmparena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( tmparena == NULL ) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }
    
    entry = (certDBEntryRevocation *)
			PORT_ArenaAlloc(arena, sizeof(certDBEntryRevocation));
    if ( entry == NULL ) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }
    entry->common.arena = arena;
    entry->common.type = crlType;

    rv = EncodeDBGenericKey(certKey, tmparena, &dbkey, crlType);
    if ( rv != SECSuccess ) {
	goto loser;
    }
    
    rv = ReadDBEntry(handle, &entry->common, &dbkey, &dbentry, tmparena);
    if ( rv == SECFailure ) {
	goto loser;
    }

    rv = DecodeDBCrlEntry(entry, &dbentry);
    if ( rv != SECSuccess ) {
	goto loser;
    }
    
    PORT_FreeArena(tmparena, PR_FALSE);
    return(entry);
    
loser:
    if ( tmparena ) {
	PORT_FreeArena(tmparena, PR_FALSE);
    }
    if ( arena ) {
	PORT_FreeArena(arena, PR_FALSE);
    }
    
    return(NULL);
}

/*
 * destroy a database entry
 */
static void
DestroyDBEntry(certDBEntry *entry)
{
    PRArenaPool *arena = entry->common.arena;

    /* Zero out the entry struct, so that any further attempts to use it
     * will cause an exception (e.g. null pointer reference). */
    PORT_Memset(&entry->common, 0, sizeof entry->common);
    PORT_FreeArena(arena, PR_FALSE);

    return;
}

void
nsslowcert_DestroyDBEntry(certDBEntry *entry)
{
    DestroyDBEntry(entry);
    return;
}

/*
 * Encode a database nickname record
 */
static SECStatus
EncodeDBNicknameEntry(certDBEntryNickname *entry, PRArenaPool *arena,
		      SECItem *dbitem)
{
    unsigned char *buf;
    
    /* allocate space for encoded database record, including space
     * for low level header
     */
    dbitem->len = entry->subjectName.len + DB_NICKNAME_ENTRY_HEADER_LEN +
	SEC_DB_ENTRY_HEADER_LEN;
    
    dbitem->data = (unsigned char *)PORT_ArenaAlloc(arena, dbitem->len);
    if ( dbitem->data == NULL) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }
    
    /* fill in database record */
    buf = &dbitem->data[SEC_DB_ENTRY_HEADER_LEN];
    
    buf[0] = ( entry->subjectName.len >> 8 ) & 0xff;
    buf[1] = entry->subjectName.len & 0xff;
    
    PORT_Memcpy(&buf[DB_NICKNAME_ENTRY_HEADER_LEN], entry->subjectName.data,
	      entry->subjectName.len);

    return(SECSuccess);

loser:
    return(SECFailure);
}

/*
 * Encode a database key for a nickname record
 */
static SECStatus
EncodeDBNicknameKey(char *nickname, PRArenaPool *arena,
		    SECItem *dbkey)
{
    unsigned int nnlen;
    
    nnlen = PORT_Strlen(nickname) + 1; /* includes null */

    /* now get the database key and format it */
    dbkey->len = nnlen + SEC_DB_KEY_HEADER_LEN;
    dbkey->data = (unsigned char *)PORT_ArenaAlloc(arena, dbkey->len);
    if ( dbkey->data == NULL ) {
	goto loser;
    }
    PORT_Memcpy(&dbkey->data[SEC_DB_KEY_HEADER_LEN], nickname, nnlen);
    dbkey->data[0] = certDBEntryTypeNickname;

    return(SECSuccess);

loser:
    return(SECFailure);
}

static SECStatus
DecodeDBNicknameEntry(certDBEntryNickname *entry, SECItem *dbentry,
                      char *nickname)
{
    /* is record long enough for header? */
    if ( dbentry->len < DB_NICKNAME_ENTRY_HEADER_LEN ) {
	PORT_SetError(SEC_ERROR_BAD_DATABASE);
	goto loser;
    }
    
    /* is database entry correct length? */
    entry->subjectName.len = ( ( dbentry->data[0] << 8 ) | dbentry->data[1] );
    if (( entry->subjectName.len + DB_NICKNAME_ENTRY_HEADER_LEN ) !=
	dbentry->len ){
	PORT_SetError(SEC_ERROR_BAD_DATABASE);
	goto loser;
    }
    
    /* copy the certkey */
    entry->subjectName.data =
	(unsigned char *)PORT_ArenaAlloc(entry->common.arena,
					 entry->subjectName.len);
    if ( entry->subjectName.data == NULL ) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }
    PORT_Memcpy(entry->subjectName.data,
	      &dbentry->data[DB_NICKNAME_ENTRY_HEADER_LEN],
	      entry->subjectName.len);
    
    entry->nickname = (char *)PORT_ArenaAlloc(entry->common.arena, 
                                              PORT_Strlen(nickname)+1);
    if ( entry->nickname ) {
	PORT_Strcpy(entry->nickname, nickname);
    }
    
    return(SECSuccess);

loser:
    return(SECFailure);
}

/*
 * create a new nickname entry
 */
static certDBEntryNickname *
NewDBNicknameEntry(char *nickname, SECItem *subjectName, unsigned int flags)
{
    PRArenaPool *arena = NULL;
    certDBEntryNickname *entry;
    int nnlen;
    SECStatus rv;
    
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( arena == NULL ) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }

    entry = (certDBEntryNickname *)PORT_ArenaAlloc(arena,
						 sizeof(certDBEntryNickname));
    if ( entry == NULL ) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }

    /* init common fields */
    entry->common.arena = arena;
    entry->common.type = certDBEntryTypeNickname;
    entry->common.version = CERT_DB_FILE_VERSION;
    entry->common.flags = flags;

    /* copy the nickname */
    nnlen = PORT_Strlen(nickname) + 1;
    
    entry->nickname = (char*)PORT_ArenaAlloc(arena, nnlen);
    if ( entry->nickname == NULL ) {
	goto loser;
    }
    
    PORT_Memcpy(entry->nickname, nickname, nnlen);
    
    rv = SECITEM_CopyItem(arena, &entry->subjectName, subjectName);
    if ( rv != SECSuccess ) {
	goto loser;
    }
    
    return(entry);
loser:
    if ( arena ) {
	PORT_FreeArena(arena, PR_FALSE);
    }
    
    return(NULL);
}

/*
 * delete a nickname entry
 */
static SECStatus
DeleteDBNicknameEntry(NSSLOWCERTCertDBHandle *handle, char *nickname)
{
    PRArenaPool *arena = NULL;
    SECStatus rv;
    SECItem dbkey;
    
    if ( nickname == NULL ) {
	return(SECSuccess);
    }
    
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( arena == NULL ) {
	goto loser;
    }

    rv = EncodeDBNicknameKey(nickname, arena, &dbkey);
    if ( rv != SECSuccess ) {
	goto loser;
    }

    rv = DeleteDBEntry(handle, certDBEntryTypeNickname, &dbkey);
    if ( rv == SECFailure ) {
	goto loser;
    }

    PORT_FreeArena(arena, PR_FALSE);
    return(SECSuccess);

loser:
    if ( arena ) {
	PORT_FreeArena(arena, PR_FALSE);
    }
    
    return(SECFailure);
}

/*
 * Read a nickname entry
 */
static certDBEntryNickname *
ReadDBNicknameEntry(NSSLOWCERTCertDBHandle *handle, char *nickname)
{
    PRArenaPool *arena = NULL;
    PRArenaPool *tmparena = NULL;
    certDBEntryNickname *entry;
    SECItem dbkey;
    SECItem dbentry;
    SECStatus rv;
    
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( arena == NULL ) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }

    tmparena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( tmparena == NULL ) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }
    
    entry = (certDBEntryNickname *)PORT_ArenaAlloc(arena,
						 sizeof(certDBEntryNickname));
    if ( entry == NULL ) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }
    entry->common.arena = arena;
    entry->common.type = certDBEntryTypeNickname;

    rv = EncodeDBNicknameKey(nickname, tmparena, &dbkey);
    if ( rv != SECSuccess ) {
	goto loser;
    }
    
    rv = ReadDBEntry(handle, &entry->common, &dbkey, &dbentry, tmparena);
    if ( rv == SECFailure ) {
	goto loser;
    }

    /* is record long enough for header? */
    if ( dbentry.len < DB_NICKNAME_ENTRY_HEADER_LEN ) {
	PORT_SetError(SEC_ERROR_BAD_DATABASE);
	goto loser;
    }

    rv = DecodeDBNicknameEntry(entry, &dbentry, nickname);
    if ( rv != SECSuccess ) {
	goto loser;
    }
    
    PORT_FreeArena(tmparena, PR_FALSE);
    return(entry);
    
loser:
    if ( tmparena ) {
	PORT_FreeArena(tmparena, PR_FALSE);
    }
    if ( arena ) {
	PORT_FreeArena(arena, PR_FALSE);
    }
    
    return(NULL);
}

/*
 * Encode a nickname entry into byte stream suitable for
 * the database
 */
static SECStatus
WriteDBNicknameEntry(NSSLOWCERTCertDBHandle *handle, certDBEntryNickname *entry)
{
    SECItem dbitem, dbkey;
    PRArenaPool *tmparena = NULL;
    SECStatus rv;
    
    tmparena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( tmparena == NULL ) {
	goto loser;
    }
    
    rv = EncodeDBNicknameEntry(entry, tmparena, &dbitem);
    if ( rv != SECSuccess ) {
	goto loser;
    }

    rv = EncodeDBNicknameKey(entry->nickname, tmparena, &dbkey);
    if ( rv != SECSuccess ) {
	goto loser;
    }

    /* now write it to the database */
    rv = WriteDBEntry(handle, &entry->common, &dbkey, &dbitem);
    if ( rv != SECSuccess ) {
	goto loser;
    }
    
    PORT_FreeArena(tmparena, PR_FALSE);
    return(SECSuccess);

loser:
    if ( tmparena ) {
	PORT_FreeArena(tmparena, PR_FALSE);
    }
    return(SECFailure);
    
}

SECStatus
EncodeDBSMimeEntry(certDBEntrySMime *entry, PRArenaPool *arena,
		   SECItem *dbitem)
{
    unsigned char *buf;
    
    /* allocate space for encoded database record, including space
     * for low level header
     */
    dbitem->len = entry->subjectName.len + entry->smimeOptions.len +
	entry->optionsDate.len +
	DB_SMIME_ENTRY_HEADER_LEN + SEC_DB_ENTRY_HEADER_LEN;
    
    dbitem->data = (unsigned char *)PORT_ArenaAlloc(arena, dbitem->len);
    if ( dbitem->data == NULL) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }
    
    /* fill in database record */
    buf = &dbitem->data[SEC_DB_ENTRY_HEADER_LEN];
    
    buf[0] = ( entry->subjectName.len >> 8 ) & 0xff;
    buf[1] = entry->subjectName.len & 0xff;
    buf[2] = ( entry->smimeOptions.len >> 8 ) & 0xff;
    buf[3] = entry->smimeOptions.len & 0xff;
    buf[4] = ( entry->optionsDate.len >> 8 ) & 0xff;
    buf[5] = entry->optionsDate.len & 0xff;

    /* if no smime options, then there should not be an options date either */
    PORT_Assert( ! ( ( entry->smimeOptions.len == 0 ) &&
		    ( entry->optionsDate.len != 0 ) ) );
    
    PORT_Memcpy(&buf[DB_SMIME_ENTRY_HEADER_LEN], entry->subjectName.data,
	      entry->subjectName.len);
    if ( entry->smimeOptions.len ) {
	PORT_Memcpy(&buf[DB_SMIME_ENTRY_HEADER_LEN+entry->subjectName.len],
		    entry->smimeOptions.data,
		    entry->smimeOptions.len);
	PORT_Memcpy(&buf[DB_SMIME_ENTRY_HEADER_LEN + entry->subjectName.len +
			 entry->smimeOptions.len],
		    entry->optionsDate.data,
		    entry->optionsDate.len);
    }

    return(SECSuccess);

loser:
    return(SECFailure);
}

/*
 * Encode a database key for a SMIME record
 */
static SECStatus
EncodeDBSMimeKey(char *emailAddr, PRArenaPool *arena,
		 SECItem *dbkey)
{
    unsigned int addrlen;
    
    addrlen = PORT_Strlen(emailAddr) + 1; /* includes null */

    /* now get the database key and format it */
    dbkey->len = addrlen + SEC_DB_KEY_HEADER_LEN;
    dbkey->data = (unsigned char *)PORT_ArenaAlloc(arena, dbkey->len);
    if ( dbkey->data == NULL ) {
	goto loser;
    }
    PORT_Memcpy(&dbkey->data[SEC_DB_KEY_HEADER_LEN], emailAddr, addrlen);
    dbkey->data[0] = certDBEntryTypeSMimeProfile;

    return(SECSuccess);

loser:
    return(SECFailure);
}

/*
 * Decode a database SMIME record
 */
static SECStatus
DecodeDBSMimeEntry(certDBEntrySMime *entry, SECItem *dbentry, char *emailAddr)
{
    /* is record long enough for header? */
    if ( dbentry->len < DB_SMIME_ENTRY_HEADER_LEN ) {
	PORT_SetError(SEC_ERROR_BAD_DATABASE);
	goto loser;
    }
    
    /* is database entry correct length? */
    entry->subjectName.len = ( ( dbentry->data[0] << 8 ) | dbentry->data[1] );
    entry->smimeOptions.len = ( ( dbentry->data[2] << 8 ) | dbentry->data[3] );
    entry->optionsDate.len = ( ( dbentry->data[4] << 8 ) | dbentry->data[5] );
    if (( entry->subjectName.len + entry->smimeOptions.len +
	 entry->optionsDate.len + DB_SMIME_ENTRY_HEADER_LEN ) != dbentry->len){
	PORT_SetError(SEC_ERROR_BAD_DATABASE);
	goto loser;
    }
    
    /* copy the subject name */
    entry->subjectName.data =
	(unsigned char *)PORT_ArenaAlloc(entry->common.arena,
					 entry->subjectName.len);
    if ( entry->subjectName.data == NULL ) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }
    PORT_Memcpy(entry->subjectName.data,
	      &dbentry->data[DB_SMIME_ENTRY_HEADER_LEN],
	      entry->subjectName.len);

    /* copy the smime options */
    if ( entry->smimeOptions.len ) {
	entry->smimeOptions.data =
	    (unsigned char *)PORT_ArenaAlloc(entry->common.arena,
					     entry->smimeOptions.len);
	if ( entry->smimeOptions.data == NULL ) {
	    PORT_SetError(SEC_ERROR_NO_MEMORY);
	    goto loser;
	}
	PORT_Memcpy(entry->smimeOptions.data,
		    &dbentry->data[DB_SMIME_ENTRY_HEADER_LEN +
				   entry->subjectName.len],
		    entry->smimeOptions.len);
    }
    if ( entry->optionsDate.len ) {
	entry->optionsDate.data =
	    (unsigned char *)PORT_ArenaAlloc(entry->common.arena,
					     entry->optionsDate.len);
	if ( entry->optionsDate.data == NULL ) {
	    PORT_SetError(SEC_ERROR_NO_MEMORY);
	    goto loser;
	}
	PORT_Memcpy(entry->optionsDate.data,
		    &dbentry->data[DB_SMIME_ENTRY_HEADER_LEN +
				   entry->subjectName.len +
				   entry->smimeOptions.len],
		    entry->optionsDate.len);
    }

    /* both options and options date must either exist or not exist */
    if ( ( ( entry->optionsDate.len == 0 ) ||
	  ( entry->smimeOptions.len == 0 ) ) &&
	entry->smimeOptions.len != entry->optionsDate.len ) {
	PORT_SetError(SEC_ERROR_BAD_DATABASE);
	goto loser;
    }

    entry->emailAddr = (char *)PORT_ArenaAlloc(entry->common.arena,
						PORT_Strlen(emailAddr)+1);
    if ( entry->emailAddr ) {
	PORT_Strcpy(entry->emailAddr, emailAddr);
    }
    
    return(SECSuccess);

loser:
    return(SECFailure);
}

/*
 * create a new SMIME entry
 */
static certDBEntrySMime *
NewDBSMimeEntry(char *emailAddr, SECItem *subjectName, SECItem *smimeOptions,
		SECItem *optionsDate, unsigned int flags)
{
    PRArenaPool *arena = NULL;
    certDBEntrySMime *entry;
    int addrlen;
    SECStatus rv;
    
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( arena == NULL ) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }

    entry = (certDBEntrySMime *)PORT_ArenaAlloc(arena,
						sizeof(certDBEntrySMime));
    if ( entry == NULL ) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }

    /* init common fields */
    entry->common.arena = arena;
    entry->common.type = certDBEntryTypeSMimeProfile;
    entry->common.version = CERT_DB_FILE_VERSION;
    entry->common.flags = flags;

    /* copy the email addr */
    addrlen = PORT_Strlen(emailAddr) + 1;
    
    entry->emailAddr = (char*)PORT_ArenaAlloc(arena, addrlen);
    if ( entry->emailAddr == NULL ) {
	goto loser;
    }
    
    PORT_Memcpy(entry->emailAddr, emailAddr, addrlen);
    
    /* copy the subject name */
    rv = SECITEM_CopyItem(arena, &entry->subjectName, subjectName);
    if ( rv != SECSuccess ) {
	goto loser;
    }

    /* copy the smime options */
    if ( smimeOptions ) {
	rv = SECITEM_CopyItem(arena, &entry->smimeOptions, smimeOptions);
	if ( rv != SECSuccess ) {
	    goto loser;
	}
    } else {
	PORT_Assert(optionsDate == NULL);
	entry->smimeOptions.data = NULL;
	entry->smimeOptions.len = 0;
    }

    /* copy the options date */
    if ( optionsDate ) {
	rv = SECITEM_CopyItem(arena, &entry->optionsDate, optionsDate);
	if ( rv != SECSuccess ) {
	    goto loser;
	}
    } else {
	PORT_Assert(smimeOptions == NULL);
	entry->optionsDate.data = NULL;
	entry->optionsDate.len = 0;
    }
    
    return(entry);
loser:
    if ( arena ) {
	PORT_FreeArena(arena, PR_FALSE);
    }
    
    return(NULL);
}

/*
 * delete a SMIME entry
 */
static SECStatus
DeleteDBSMimeEntry(NSSLOWCERTCertDBHandle *handle, char *emailAddr)
{
    PRArenaPool *arena = NULL;
    SECStatus rv;
    SECItem dbkey;
    
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( arena == NULL ) {
	goto loser;
    }

    rv = EncodeDBSMimeKey(emailAddr, arena, &dbkey);
    if ( rv != SECSuccess ) {
	goto loser;
    }

    rv = DeleteDBEntry(handle, certDBEntryTypeSMimeProfile, &dbkey);
    if ( rv == SECFailure ) {
	goto loser;
    }

    PORT_FreeArena(arena, PR_FALSE);
    return(SECSuccess);

loser:
    if ( arena ) {
	PORT_FreeArena(arena, PR_FALSE);
    }
    
    return(SECFailure);
}

/*
 * Read a SMIME entry
 */
certDBEntrySMime *
nsslowcert_ReadDBSMimeEntry(NSSLOWCERTCertDBHandle *handle, char *emailAddr)
{
    PRArenaPool *arena = NULL;
    PRArenaPool *tmparena = NULL;
    certDBEntrySMime *entry;
    SECItem dbkey;
    SECItem dbentry;
    SECStatus rv;
    
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( arena == NULL ) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }

    tmparena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( tmparena == NULL ) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }
    
    entry = (certDBEntrySMime *)PORT_ArenaAlloc(arena,
						sizeof(certDBEntrySMime));
    if ( entry == NULL ) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }
    entry->common.arena = arena;
    entry->common.type = certDBEntryTypeSMimeProfile;

    rv = EncodeDBSMimeKey(emailAddr, tmparena, &dbkey);
    if ( rv != SECSuccess ) {
	goto loser;
    }
    
    rv = ReadDBEntry(handle, &entry->common, &dbkey, &dbentry, tmparena);
    if ( rv == SECFailure ) {
	goto loser;
    }

    /* is record long enough for header? */
    if ( dbentry.len < DB_SMIME_ENTRY_HEADER_LEN ) {
	PORT_SetError(SEC_ERROR_BAD_DATABASE);
	goto loser;
    }

    rv = DecodeDBSMimeEntry(entry, &dbentry, emailAddr);
    if ( rv != SECSuccess ) {
	goto loser;
    }
    
    PORT_FreeArena(tmparena, PR_FALSE);
    return(entry);
    
loser:
    if ( tmparena ) {
	PORT_FreeArena(tmparena, PR_FALSE);
    }
    if ( arena ) {
	PORT_FreeArena(arena, PR_FALSE);
    }
    
    return(NULL);
}

/*
 * Encode a SMIME entry into byte stream suitable for
 * the database
 */
static SECStatus
WriteDBSMimeEntry(NSSLOWCERTCertDBHandle *handle, certDBEntrySMime *entry)
{
    SECItem dbitem, dbkey;
    PRArenaPool *tmparena = NULL;
    SECStatus rv;
    
    tmparena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( tmparena == NULL ) {
	goto loser;
    }
    
    rv = EncodeDBSMimeEntry(entry, tmparena, &dbitem);
    if ( rv != SECSuccess ) {
	goto loser;
    }

    rv = EncodeDBSMimeKey(entry->emailAddr, tmparena, &dbkey);
    if ( rv != SECSuccess ) {
	goto loser;
    }

    /* now write it to the database */
    rv = WriteDBEntry(handle, &entry->common, &dbkey, &dbitem);
    if ( rv != SECSuccess ) {
	goto loser;
    }
    
    PORT_FreeArena(tmparena, PR_FALSE);
    return(SECSuccess);

loser:
    if ( tmparena ) {
	PORT_FreeArena(tmparena, PR_FALSE);
    }
    return(SECFailure);
    
}

/*
 * Encode a database subject record
 */
static SECStatus
EncodeDBSubjectEntry(certDBEntrySubject *entry, PRArenaPool *arena,
		     SECItem *dbitem)
{
    unsigned char *buf;
    int len;
    unsigned int ncerts;
    unsigned int i;
    unsigned char *tmpbuf;
    unsigned int nnlen = 0;
    unsigned int eaddrlen = 0;
    int keyidoff;
    SECItem *certKeys;
    SECItem *keyIDs;
    
    if ( entry->nickname ) {
	nnlen = PORT_Strlen(entry->nickname) + 1;
    }
    if ( entry->emailAddr ) {
	eaddrlen = PORT_Strlen(entry->emailAddr) + 1;
    }
    
    ncerts = entry->ncerts;
    
    /* compute the length of the entry */
    keyidoff = DB_SUBJECT_ENTRY_HEADER_LEN + nnlen + eaddrlen;
    len = keyidoff + 4 * ncerts;
    for ( i = 0; i < ncerts; i++ ) {
	len += entry->certKeys[i].len;
	len += entry->keyIDs[i].len;
    }
    
    /* allocate space for encoded database record, including space
     * for low level header
     */
    dbitem->len = len + SEC_DB_ENTRY_HEADER_LEN;
    
    dbitem->data = (unsigned char *)PORT_ArenaAlloc(arena, dbitem->len);
    if ( dbitem->data == NULL) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }
    
    /* fill in database record */
    buf = &dbitem->data[SEC_DB_ENTRY_HEADER_LEN];
    
    buf[0] = ( ncerts >> 8 ) & 0xff;
    buf[1] = ncerts & 0xff;
    buf[2] = ( nnlen >> 8 ) & 0xff;
    buf[3] = nnlen & 0xff;
    buf[4] = ( eaddrlen >> 8 ) & 0xff;
    buf[5] = eaddrlen & 0xff;

    PORT_Memcpy(&buf[DB_SUBJECT_ENTRY_HEADER_LEN], entry->nickname, nnlen);
    PORT_Memcpy(&buf[DB_SUBJECT_ENTRY_HEADER_LEN+nnlen], entry->emailAddr,
		eaddrlen);
    
    for ( i = 0; i < ncerts; i++ ) {

	certKeys = entry->certKeys;
	keyIDs = entry->keyIDs;

	buf[keyidoff+i*2] = ( certKeys[i].len >> 8 ) & 0xff;
	buf[keyidoff+1+i*2] = certKeys[i].len & 0xff;
	buf[keyidoff+ncerts*2+i*2] = ( keyIDs[i].len >> 8 ) & 0xff;
	buf[keyidoff+1+ncerts*2+i*2] = keyIDs[i].len & 0xff;
    }
    
    /* temp pointer used to stuff certkeys and keyids into the buffer */
    tmpbuf = &buf[keyidoff+ncerts*4];

    for ( i = 0; i < ncerts; i++ ) {
	certKeys = entry->certKeys;
	PORT_Memcpy(tmpbuf, certKeys[i].data, certKeys[i].len);
	tmpbuf = tmpbuf + certKeys[i].len;
    }
    
    for ( i = 0; i < ncerts; i++ ) {
	keyIDs = entry->keyIDs;
	PORT_Memcpy(tmpbuf, keyIDs[i].data, keyIDs[i].len);
	tmpbuf = tmpbuf + keyIDs[i].len;
    }

    PORT_Assert(tmpbuf == &buf[len]);
    
    return(SECSuccess);

loser:
    return(SECFailure);
}

/*
 * Encode a database key for a subject record
 */
static SECStatus
EncodeDBSubjectKey(SECItem *derSubject, PRArenaPool *arena,
		   SECItem *dbkey)
{
    dbkey->len = derSubject->len + SEC_DB_KEY_HEADER_LEN;
    dbkey->data = (unsigned char *)PORT_ArenaAlloc(arena, dbkey->len);
    if ( dbkey->data == NULL ) {
	goto loser;
    }
    PORT_Memcpy(&dbkey->data[SEC_DB_KEY_HEADER_LEN], derSubject->data,
	      derSubject->len);
    dbkey->data[0] = certDBEntryTypeSubject;

    return(SECSuccess);

loser:
    return(SECFailure);
}

static SECStatus
DecodeDBSubjectEntry(certDBEntrySubject *entry, SECItem *dbentry,
		     SECItem *derSubject)
{
    unsigned int ncerts;
    PRArenaPool *arena;
    unsigned int len, itemlen;
    unsigned char *tmpbuf;
    unsigned int i;
    SECStatus rv;
    unsigned int keyidoff;
    unsigned int nnlen, eaddrlen;
    
    arena = entry->common.arena;

    rv = SECITEM_CopyItem(arena, &entry->derSubject, derSubject);
    if ( rv != SECSuccess ) {
	goto loser;
    }

    /* is record long enough for header? */
    if ( dbentry->len < DB_SUBJECT_ENTRY_HEADER_LEN ) {
	PORT_SetError(SEC_ERROR_BAD_DATABASE);
	goto loser;
    }
    
    entry->ncerts = ncerts = ( ( dbentry->data[0] << 8 ) | dbentry->data[1] );
    nnlen = ( ( dbentry->data[2] << 8 ) | dbentry->data[3] );
    eaddrlen = ( ( dbentry->data[4] << 8 ) | dbentry->data[5] );
    if ( dbentry->len < ( ncerts * 4 + DB_SUBJECT_ENTRY_HEADER_LEN +
			 nnlen + eaddrlen) ) {
	PORT_SetError(SEC_ERROR_BAD_DATABASE);
	goto loser;
    }
    
    entry->certKeys = (SECItem *)PORT_ArenaAlloc(arena,
						 sizeof(SECItem) * ncerts);
    entry->keyIDs = (SECItem *)PORT_ArenaAlloc(arena,
					       sizeof(SECItem) * ncerts);

    if ( ( entry->certKeys == NULL ) || ( entry->keyIDs == NULL ) ) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }

    if ( nnlen > 1 ) { /* null terminator is stored */
	entry->nickname = (char *)PORT_ArenaAlloc(arena, nnlen);
	if ( entry->nickname == NULL ) {
	    PORT_SetError(SEC_ERROR_NO_MEMORY);
	    goto loser;
	}
	PORT_Memcpy(entry->nickname,
		    &dbentry->data[DB_SUBJECT_ENTRY_HEADER_LEN],
		    nnlen);
    } else {
	entry->nickname = NULL;
    }
    
    if ( eaddrlen > 1 ) { /* null terminator is stored */
	entry->emailAddr = (char *)PORT_ArenaAlloc(arena, eaddrlen);
	if ( entry->emailAddr == NULL ) {
	    PORT_SetError(SEC_ERROR_NO_MEMORY);
	    goto loser;
	}
	PORT_Memcpy(entry->emailAddr,
		    &dbentry->data[DB_SUBJECT_ENTRY_HEADER_LEN+nnlen],
		    eaddrlen);
    } else {
	entry->emailAddr = NULL;
    }
    
    /* collect the lengths of the certKeys and keyIDs, and total the
     * overall length.
     */
    keyidoff = DB_SUBJECT_ENTRY_HEADER_LEN + nnlen + eaddrlen;
    len = keyidoff + 4 * ncerts;

    tmpbuf = &dbentry->data[0];
    
    for ( i = 0; i < ncerts; i++ ) {

	itemlen = ( tmpbuf[keyidoff + 2*i] << 8 ) | tmpbuf[keyidoff + 1 + 2*i] ;
	len += itemlen;
	entry->certKeys[i].len = itemlen;

	itemlen = ( tmpbuf[keyidoff + 2*ncerts + 2*i] << 8 ) |
	    tmpbuf[keyidoff + 1 + 2*ncerts + 2*i] ;
	len += itemlen;
	entry->keyIDs[i].len = itemlen;
    }
    
    /* is database entry correct length? */
    if ( len != dbentry->len ){
	PORT_SetError(SEC_ERROR_BAD_DATABASE);
	goto loser;
    }
    
    tmpbuf = &tmpbuf[keyidoff + 4*ncerts];
    for ( i = 0; i < ncerts; i++ ) {
	entry->certKeys[i].data =
	    (unsigned char *)PORT_ArenaAlloc(arena, entry->certKeys[i].len);
	if ( entry->certKeys[i].data == NULL ) {
	    PORT_SetError(SEC_ERROR_NO_MEMORY);
	    goto loser;
	}
	PORT_Memcpy(entry->certKeys[i].data, tmpbuf, entry->certKeys[i].len);
	tmpbuf = &tmpbuf[entry->certKeys[i].len];
    }

    for ( i = 0; i < ncerts; i++ ) {
	entry->keyIDs[i].data =
	    (unsigned char *)PORT_ArenaAlloc(arena, entry->keyIDs[i].len);
	if ( entry->keyIDs[i].data == NULL ) {
	    PORT_SetError(SEC_ERROR_NO_MEMORY);
	    goto loser;
	}
	PORT_Memcpy(entry->keyIDs[i].data, tmpbuf, entry->keyIDs[i].len);
	tmpbuf = &tmpbuf[entry->keyIDs[i].len];
    }
    
    PORT_Assert(tmpbuf == &dbentry->data[dbentry->len]);
    
    return(SECSuccess);

loser:
    return(SECFailure);
}

/*
 * create a new subject entry with a single cert
 */
static certDBEntrySubject *
NewDBSubjectEntry(SECItem *derSubject, SECItem *certKey,
		  SECItem *keyID, char *nickname, char *emailAddr,
		  unsigned int flags)
{
    PRArenaPool *arena = NULL;
    certDBEntrySubject *entry;
    SECStatus rv;
    unsigned int nnlen;
    unsigned int eaddrlen;
    
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( arena == NULL ) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }

    entry = (certDBEntrySubject *)PORT_ArenaAlloc(arena,
						  sizeof(certDBEntrySubject));
    if ( entry == NULL ) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }

    /* init common fields */
    entry->common.arena = arena;
    entry->common.type = certDBEntryTypeSubject;
    entry->common.version = CERT_DB_FILE_VERSION;
    entry->common.flags = flags;

    /* copy the subject */
    rv = SECITEM_CopyItem(arena, &entry->derSubject, derSubject);
    if ( rv != SECSuccess ) {
	goto loser;
    }
    
    entry->ncerts = 1;
    /* copy nickname */
    if ( nickname && ( *nickname != '\0' ) ) {
	nnlen = PORT_Strlen(nickname) + 1;
	entry->nickname = (char *)PORT_ArenaAlloc(arena, nnlen);
	if ( entry->nickname == NULL ) {
	    goto loser;
	}
						  
	PORT_Memcpy(entry->nickname, nickname, nnlen);
    } else {
	entry->nickname = NULL;
    }
    
    /* copy email addr */
    if ( emailAddr && ( *emailAddr != '\0' ) ) {
	emailAddr = nsslowcert_FixupEmailAddr(emailAddr);
	if ( emailAddr == NULL ) {
	    entry->emailAddr = NULL;
	    goto loser;
	}
	
	eaddrlen = PORT_Strlen(emailAddr) + 1;
	entry->emailAddr = (char *)PORT_ArenaAlloc(arena, eaddrlen);
	if ( entry->emailAddr == NULL ) {
	    PORT_Free(emailAddr);
	    goto loser;
	}
	
	PORT_Memcpy(entry->emailAddr, emailAddr, eaddrlen);
	PORT_Free(emailAddr);
    } else {
	entry->emailAddr = NULL;
    }
    
    /* allocate space for certKeys and keyIDs */
    entry->certKeys = (SECItem *)PORT_ArenaAlloc(arena, sizeof(SECItem));
    entry->keyIDs = (SECItem *)PORT_ArenaAlloc(arena, sizeof(SECItem));
    if ( ( entry->certKeys == NULL ) || ( entry->keyIDs == NULL ) ) {
	goto loser;
    }

    /* copy the certKey and keyID */
    rv = SECITEM_CopyItem(arena, &entry->certKeys[0], certKey);
    if ( rv != SECSuccess ) {
	goto loser;
    }
    rv = SECITEM_CopyItem(arena, &entry->keyIDs[0], keyID);
    if ( rv != SECSuccess ) {
	goto loser;
    }
    
    return(entry);
loser:
    if ( arena ) {
	PORT_FreeArena(arena, PR_FALSE);
    }
    
    return(NULL);
}

/*
 * delete a subject entry
 */
static SECStatus
DeleteDBSubjectEntry(NSSLOWCERTCertDBHandle *handle, SECItem *derSubject)
{
    SECItem dbkey;
    PRArenaPool *arena = NULL;
    SECStatus rv;
    
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( arena == NULL ) {
	goto loser;
    }
    
    rv = EncodeDBSubjectKey(derSubject, arena, &dbkey);
    if ( rv != SECSuccess ) {
	goto loser;
    }
    
    rv = DeleteDBEntry(handle, certDBEntryTypeSubject, &dbkey);
    if ( rv == SECFailure ) {
	goto loser;
    }

    PORT_FreeArena(arena, PR_FALSE);
    return(SECSuccess);

loser:
    if ( arena ) {
	PORT_FreeArena(arena, PR_FALSE);
    }
    
    return(SECFailure);
}

/*
 * Read the subject entry
 */
static certDBEntrySubject *
ReadDBSubjectEntry(NSSLOWCERTCertDBHandle *handle, SECItem *derSubject)
{
    PRArenaPool *arena = NULL;
    PRArenaPool *tmparena = NULL;
    certDBEntrySubject *entry;
    SECItem dbkey;
    SECItem dbentry;
    SECStatus rv;
    
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( arena == NULL ) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }

    tmparena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( tmparena == NULL ) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }
    
    entry = (certDBEntrySubject *)PORT_ArenaAlloc(arena,
						sizeof(certDBEntrySubject));
    if ( entry == NULL ) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }
    entry->common.arena = arena;
    entry->common.type = certDBEntryTypeSubject;

    rv = EncodeDBSubjectKey(derSubject, tmparena, &dbkey);
    if ( rv != SECSuccess ) {
	goto loser;
    }
    
    rv = ReadDBEntry(handle, &entry->common, &dbkey, &dbentry, tmparena);
    if ( rv == SECFailure ) {
	goto loser;
    }

    rv = DecodeDBSubjectEntry(entry, &dbentry, derSubject);
    if ( rv == SECFailure ) {
	goto loser;
    }
    
    PORT_FreeArena(tmparena, PR_FALSE);
    return(entry);
    
loser:
    if ( tmparena ) {
	PORT_FreeArena(tmparena, PR_FALSE);
    }
    if ( arena ) {
	PORT_FreeArena(arena, PR_FALSE);
    }
    
    return(NULL);
}

/*
 * Encode a subject name entry into byte stream suitable for
 * the database
 */
static SECStatus
WriteDBSubjectEntry(NSSLOWCERTCertDBHandle *handle, certDBEntrySubject *entry)
{
    SECItem dbitem, dbkey;
    PRArenaPool *tmparena = NULL;
    SECStatus rv;
    
    tmparena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( tmparena == NULL ) {
	goto loser;
    }
    
    rv = EncodeDBSubjectEntry(entry, tmparena, &dbitem);
    if ( rv != SECSuccess ) {
	goto loser;
    }
    
    rv = EncodeDBSubjectKey(&entry->derSubject, tmparena, &dbkey);
    if ( rv != SECSuccess ) {
	goto loser;
    }

    /* now write it to the database */
    rv = WriteDBEntry(handle, &entry->common, &dbkey, &dbitem);
    if ( rv != SECSuccess ) {
	goto loser;
    }
    
    PORT_FreeArena(tmparena, PR_FALSE);
    return(SECSuccess);

loser:
    if ( tmparena ) {
	PORT_FreeArena(tmparena, PR_FALSE);
    }
    return(SECFailure);
    
}

static SECStatus
UpdateSubjectWithEmailAddr(NSSLOWCERTCertDBHandle *dbhandle, 
					SECItem *derSubject, char *emailAddr)
{
    PRBool save = PR_FALSE, delold = PR_FALSE;
    certDBEntrySubject *entry = NULL;
    SECStatus rv;
   
    if (emailAddr) { 
	emailAddr = nsslowcert_FixupEmailAddr(emailAddr);
	if (emailAddr == NULL) {
	    return SECFailure;
	}
    }

    entry = ReadDBSubjectEntry(dbhandle,derSubject);    
    if (entry == NULL) {
	goto loser;
    }
    
    if ( entry->emailAddr ) {
	if ( (emailAddr == NULL) || 
		(PORT_Strcmp(entry->emailAddr, emailAddr) != 0) ) {
	    save = PR_TRUE;
	    delold = PR_TRUE;
	}
    } else if (emailAddr) {
	save = PR_TRUE;
    }

    if ( delold ) {
	/* delete the old smime entry, because this cert now has a new
	 * smime entry pointing to it
	 */
	PORT_Assert(save);
	PORT_Assert(entry->emailAddr != NULL);
	DeleteDBSMimeEntry(dbhandle, entry->emailAddr);
    }

    if ( save ) {
	unsigned int len;
	
	PORT_Assert(entry != NULL);
	if (emailAddr) {
	    len = PORT_Strlen(emailAddr) + 1;
	    entry->emailAddr = (char *)PORT_ArenaAlloc(entry->common.arena, len);
	    if ( entry->emailAddr == NULL ) {
		goto loser;
	    }
	    PORT_Memcpy(entry->emailAddr, emailAddr, len);
	} else {
	    entry->emailAddr = NULL;
	}
	
	/* delete the subject entry */
	DeleteDBSubjectEntry(dbhandle, derSubject);

	/* write the new one */
	rv = WriteDBSubjectEntry(dbhandle, entry);
	if ( rv != SECSuccess ) {
	    goto loser;
	}
    }

    DestroyDBEntry((certDBEntry *)entry);
    if (emailAddr) PORT_Free(emailAddr);
    return(SECSuccess);

loser:
    if (entry) DestroyDBEntry((certDBEntry *)entry);
    if (emailAddr) PORT_Free(emailAddr);
    return(SECFailure);
}

/*
 * writes a nickname to an existing subject entry that does not currently
 * have one
 */
static SECStatus
AddNicknameToSubject(NSSLOWCERTCertDBHandle *dbhandle,
			NSSLOWCERTCertificate *cert, char *nickname)
{
    certDBEntrySubject *entry;
    SECStatus rv;
    
    if ( nickname == NULL ) {
	return(SECFailure);
    }
    
    entry = ReadDBSubjectEntry(dbhandle,&cert->derSubject);
    PORT_Assert(entry != NULL);
    if ( entry == NULL ) {
	goto loser;
    }
    
    PORT_Assert(entry->nickname == NULL);
    if ( entry->nickname != NULL ) {
	goto loser;
    }
    
    entry->nickname = (nickname) ? 
			PORT_ArenaStrdup(entry->common.arena, nickname) : NULL;
    
    if ( entry->nickname == NULL ) {
	goto loser;
    }
	
    /* delete the subject entry */
    DeleteDBSubjectEntry(dbhandle, &cert->derSubject);

    /* write the new one */
    rv = WriteDBSubjectEntry(dbhandle, entry);
    if ( rv != SECSuccess ) {
	goto loser;
    }

    return(SECSuccess);

loser:
    return(SECFailure);
}

/*
 * create a new version entry
 */
static certDBEntryVersion *
NewDBVersionEntry(unsigned int flags)
{
    PRArenaPool *arena = NULL;
    certDBEntryVersion *entry;
    
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( arena == NULL ) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }

    entry = (certDBEntryVersion *)PORT_ArenaAlloc(arena,
					       sizeof(certDBEntryVersion));
    if ( entry == NULL ) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }
    entry->common.arena = arena;
    entry->common.type = certDBEntryTypeVersion;
    entry->common.version = CERT_DB_FILE_VERSION;
    entry->common.flags = flags;

    return(entry);
loser:
    if ( arena ) {
	PORT_FreeArena(arena, PR_FALSE);
    }
    
    return(NULL);
}

/*
 * Read the version entry
 */
static certDBEntryVersion *
ReadDBVersionEntry(NSSLOWCERTCertDBHandle *handle)
{
    PRArenaPool *arena = NULL;
    PRArenaPool *tmparena = NULL;
    certDBEntryVersion *entry;
    SECItem dbkey;
    SECItem dbentry;
    
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( arena == NULL ) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }

    tmparena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( tmparena == NULL ) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }
    
    entry = (certDBEntryVersion *)PORT_ArenaAlloc(arena,
						sizeof(certDBEntryVersion));
    if ( entry == NULL ) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }
    entry->common.arena = arena;
    entry->common.type = certDBEntryTypeVersion;

    /* now get the database key and format it */
    dbkey.len = SEC_DB_VERSION_KEY_LEN + SEC_DB_KEY_HEADER_LEN;
    dbkey.data = (unsigned char *)PORT_ArenaAlloc(tmparena, dbkey.len);
    if ( dbkey.data == NULL ) {
	goto loser;
    }
    PORT_Memcpy(&dbkey.data[SEC_DB_KEY_HEADER_LEN], SEC_DB_VERSION_KEY,
	      SEC_DB_VERSION_KEY_LEN);

    ReadDBEntry(handle, &entry->common, &dbkey, &dbentry, tmparena);

    PORT_FreeArena(tmparena, PR_FALSE);
    return(entry);
    
loser:
    if ( tmparena ) {
	PORT_FreeArena(tmparena, PR_FALSE);
    }
    if ( arena ) {
	PORT_FreeArena(arena, PR_FALSE);
    }
    
    return(NULL);
}


/*
 * Encode a version entry into byte stream suitable for
 * the database
 */
static SECStatus
WriteDBVersionEntry(NSSLOWCERTCertDBHandle *handle, certDBEntryVersion *entry)
{
    SECItem dbitem, dbkey;
    PRArenaPool *tmparena = NULL;
    SECStatus rv;
    
    tmparena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( tmparena == NULL ) {
	goto loser;
    }
    
    /* allocate space for encoded database record, including space
     * for low level header
     */
    dbitem.len = SEC_DB_ENTRY_HEADER_LEN;
    
    dbitem.data = (unsigned char *)PORT_ArenaAlloc(tmparena, dbitem.len);
    if ( dbitem.data == NULL) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }
    
    /* now get the database key and format it */
    dbkey.len = SEC_DB_VERSION_KEY_LEN + SEC_DB_KEY_HEADER_LEN;
    dbkey.data = (unsigned char *)PORT_ArenaAlloc(tmparena, dbkey.len);
    if ( dbkey.data == NULL ) {
	goto loser;
    }
    PORT_Memcpy(&dbkey.data[SEC_DB_KEY_HEADER_LEN], SEC_DB_VERSION_KEY,
	      SEC_DB_VERSION_KEY_LEN);

    /* now write it to the database */
    rv = WriteDBEntry(handle, &entry->common, &dbkey, &dbitem);
    if ( rv != SECSuccess ) {
	goto loser;
    }
    
    PORT_FreeArena(tmparena, PR_FALSE);
    return(SECSuccess);

loser:
    if ( tmparena ) {
	PORT_FreeArena(tmparena, PR_FALSE);
    }
    return(SECFailure);
}

/*
 * cert is no longer a perm cert, but will remain a temp cert
 */
static SECStatus
RemovePermSubjectNode(NSSLOWCERTCertificate *cert)
{
    certDBEntrySubject *entry;
    unsigned int i;
    SECStatus rv;
    
    entry = ReadDBSubjectEntry(cert->dbhandle,&cert->derSubject);
    PORT_Assert(entry);
    if ( entry == NULL ) {
	return(SECFailure);
    }

    PORT_Assert(entry->ncerts);
    rv = SECFailure;
    
    if ( entry->ncerts > 1 ) {
	for ( i = 0; i < entry->ncerts; i++ ) {
	    if ( SECITEM_CompareItem(&entry->certKeys[i], &cert->certKey) ==
		SECEqual ) {
		/* copy rest of list forward one entry */
		for ( i = i + 1; i < entry->ncerts; i++ ) {
		    entry->certKeys[i-1] = entry->certKeys[i];
		    entry->keyIDs[i-1] = entry->keyIDs[i];
		}
		entry->ncerts--;
		DeleteDBSubjectEntry(cert->dbhandle, &cert->derSubject);
		rv = WriteDBSubjectEntry(cert->dbhandle, entry);
		break;
	    }
	}
    } else {
	/* no entries left, delete the perm entry in the DB */
	if ( entry->emailAddr ) {
	    /* if the subject had an email record, then delete it too */
	    DeleteDBSMimeEntry(cert->dbhandle, entry->emailAddr);
	}
	
	DeleteDBSubjectEntry(cert->dbhandle, &cert->derSubject);
    }
    DestroyDBEntry((certDBEntry *)entry);

    return(rv);
}

/*
 * add a cert to the perm subject list
 */
static SECStatus
AddPermSubjectNode(certDBEntrySubject *entry, NSSLOWCERTCertificate *cert, 
								char *nickname)
{
    SECItem *newCertKeys, *newKeyIDs;
    int i;
    SECStatus rv;
    NSSLOWCERTCertificate *cmpcert;
    unsigned int nnlen;
    int ncerts;
    

    PORT_Assert(entry);    
    ncerts = entry->ncerts;
	
    if ( nickname && entry->nickname ) {
	/* nicknames must be the same */
	PORT_Assert(PORT_Strcmp(nickname, entry->nickname) == 0);
    }

    if ( ( entry->nickname == NULL ) && ( nickname != NULL ) ) {
	/* copy nickname into the entry */
	nnlen = PORT_Strlen(nickname) + 1;
	entry->nickname = (char *)PORT_ArenaAlloc(entry->common.arena,nnlen);
	if ( entry->nickname == NULL ) {
	    return(SECFailure);
	}
	PORT_Memcpy(entry->nickname, nickname, nnlen);
    }
	
    /* a DB entry already exists, so add this cert */
    newCertKeys = (SECItem *)PORT_ArenaAlloc(entry->common.arena,
					 sizeof(SECItem) * ( ncerts + 1 ) );
    newKeyIDs = (SECItem *)PORT_ArenaAlloc(entry->common.arena,
					 sizeof(SECItem) * ( ncerts + 1 ) );

    if ( ( newCertKeys == NULL ) || ( newKeyIDs == NULL ) ) {
	    return(SECFailure);
    }

    for ( i = 0; i < ncerts; i++ ) {
	cmpcert = nsslowcert_FindCertByKey(cert->dbhandle,
						  &entry->certKeys[i]);
	PORT_Assert(cmpcert);
	if ( nsslowcert_IsNewer(cert, cmpcert) ) {
	    /* insert before cmpcert */
	    rv = SECITEM_CopyItem(entry->common.arena, &newCertKeys[i],
				      &cert->certKey);
	    if ( rv != SECSuccess ) {
		return(SECFailure);
	    }
	    rv = SECITEM_CopyItem(entry->common.arena, &newKeyIDs[i],
				      &cert->subjectKeyID);
	    if ( rv != SECSuccess ) {
		return(SECFailure);
	    }
	    /* copy the rest of the entry */
	    for ( ; i < ncerts; i++ ) {
		newCertKeys[i+1] = entry->certKeys[i];
		newKeyIDs[i+1] = entry->keyIDs[i];
	    }

	    /* update certKeys and keyIDs */
	    entry->certKeys = newCertKeys;
	    entry->keyIDs = newKeyIDs;
		
	    /* increment count */
	    entry->ncerts++;
	    break;
	}
	/* copy this cert entry */
	newCertKeys[i] = entry->certKeys[i];
	newKeyIDs[i] = entry->keyIDs[i];
    }

    if ( entry->ncerts == ncerts ) {
	/* insert new one at end */
	rv = SECITEM_CopyItem(entry->common.arena, &newCertKeys[ncerts],
				  &cert->certKey);
	if ( rv != SECSuccess ) {
	    return(SECFailure);
	}
	rv = SECITEM_CopyItem(entry->common.arena, &newKeyIDs[ncerts],
				  &cert->subjectKeyID);
	if ( rv != SECSuccess ) {
	    return(SECFailure);
	}

	/* update certKeys and keyIDs */
	entry->certKeys = newCertKeys;
	entry->keyIDs = newKeyIDs;
		
	/* increment count */
	entry->ncerts++;
    }
    DeleteDBSubjectEntry(cert->dbhandle, &cert->derSubject);
    rv = WriteDBSubjectEntry(cert->dbhandle, entry);
    return(rv);
}


SECStatus
nsslowcert_TraversePermCertsForSubject(NSSLOWCERTCertDBHandle *handle,
				 SECItem *derSubject,
				 NSSLOWCERTCertCallback cb, void *cbarg)
{
    certDBEntrySubject *entry;
    int i;
    NSSLOWCERTCertificate *cert;
    SECStatus rv = SECSuccess;
    
    entry = ReadDBSubjectEntry(handle, derSubject);

    if ( entry == NULL ) {
	return(SECFailure);
    }
    
    for( i = 0; i < entry->ncerts; i++ ) {
	cert = nsslowcert_FindCertByKey(handle, &entry->certKeys[i]);
	rv = (* cb)(cert, cbarg);
	nsslowcert_DestroyCertificate(cert);
	if ( rv == SECFailure ) {
	    break;
	}
    }

    DestroyDBEntry((certDBEntry *)entry);

    return(rv);
}

int
nsslowcert_NumPermCertsForSubject(NSSLOWCERTCertDBHandle *handle,
							 SECItem *derSubject)
{
    certDBEntrySubject *entry;
    int ret;
    
    entry = ReadDBSubjectEntry(handle, derSubject);

    if ( entry == NULL ) {
	return(SECFailure);
    }

    ret = entry->ncerts;
    
    DestroyDBEntry((certDBEntry *)entry);
    
    return(ret);
}

SECStatus
nsslowcert_TraversePermCertsForNickname(NSSLOWCERTCertDBHandle *handle,
		 	char *nickname, NSSLOWCERTCertCallback cb, void *cbarg)
{
    certDBEntryNickname *nnentry = NULL;
    certDBEntrySMime *smentry = NULL;
    SECStatus rv;
    SECItem *derSubject = NULL;
    
    nnentry = ReadDBNicknameEntry(handle, nickname);
    if ( nnentry ) {
	derSubject = &nnentry->subjectName;
    } else {
	smentry = nsslowcert_ReadDBSMimeEntry(handle, nickname);
	if ( smentry ) {
	    derSubject = &smentry->subjectName;
	}
    }
    
    if ( derSubject ) {
	rv = nsslowcert_TraversePermCertsForSubject(handle, derSubject,
					      cb, cbarg);
    } else {
	rv = SECFailure;
    }

    if ( nnentry ) {
	DestroyDBEntry((certDBEntry *)nnentry);
    }
    if ( smentry ) {
	DestroyDBEntry((certDBEntry *)smentry);
    }
    
    return(rv);
}

int
nsslowcert_NumPermCertsForNickname(NSSLOWCERTCertDBHandle *handle, 
								char *nickname)
{
    certDBEntryNickname *entry;
    int ret;
    
    entry = ReadDBNicknameEntry(handle, nickname);
    
    if ( entry ) {
	ret = nsslowcert_NumPermCertsForSubject(handle, &entry->subjectName);
	DestroyDBEntry((certDBEntry *)entry);
    } else {
	ret = 0;
    }
    return(ret);
}

/*
 * add a nickname to a cert that doesn't have one
 */
static SECStatus
AddNicknameToPermCert(NSSLOWCERTCertDBHandle *dbhandle,
				NSSLOWCERTCertificate *cert, char *nickname)
{
    certDBEntryCert *entry;
    int rv;

    entry = cert->dbEntry;
    PORT_Assert(entry != NULL);
    if ( entry == NULL ) {
	goto loser;
    }

    entry->nickname = PORT_ArenaStrdup(entry->common.arena, nickname);

    rv = WriteDBCertEntry(dbhandle, entry);
    if ( rv ) {
	goto loser;
    }

    cert->nickname = PORT_ArenaStrdup(cert->arena, nickname);
    return(SECSuccess);
    
loser:
    return(SECFailure);
}

/*
 * add a nickname to a cert that is already in the perm database, but doesn't
 * have one yet (it is probably an e-mail cert).
 */
SECStatus
nsslowcert_AddPermNickname(NSSLOWCERTCertDBHandle *dbhandle,
				NSSLOWCERTCertificate *cert, char *nickname)
{
    SECStatus rv = SECFailure;
    certDBEntrySubject *entry = NULL;
    
    nsslowcert_LockDB(dbhandle);
    
    PORT_Assert(cert->nickname == NULL);
    
    if ( cert->nickname != NULL ) {
	rv = SECSuccess;
	goto loser;
    }

    entry = ReadDBSubjectEntry(dbhandle, &cert->derSubject);
    if (entry == NULL) goto loser;

    if ( entry->nickname == NULL ) {
	/* no nickname for subject */
	rv = AddNicknameToSubject(dbhandle, cert, nickname);
	if ( rv != SECSuccess ) {
	    goto loser;
	}
	rv = AddNicknameToPermCert(dbhandle, cert, nickname);
	if ( rv != SECSuccess ) {
	    goto loser;
	}
    } else {
	/* subject already has a nickname */
	rv = AddNicknameToPermCert(dbhandle, cert, entry->nickname);
	if ( rv != SECSuccess ) {
	    goto loser;
	}
    }
    rv = SECSuccess;

loser:
    if (entry) {
	DestroyDBEntry((certDBEntry *)entry);
    }
    nsslowcert_UnlockDB(dbhandle);
    return(rv);
}

static certDBEntryCert *
AddCertToPermDB(NSSLOWCERTCertDBHandle *handle, NSSLOWCERTCertificate *cert,
		char *nickname, NSSLOWCERTCertTrust *trust)
{
    certDBEntryCert *certEntry = NULL;
    certDBEntryNickname *nicknameEntry = NULL;
    certDBEntrySubject *subjectEntry = NULL;
    int state = 0;
    SECStatus rv;
    PRBool donnentry = PR_FALSE;

    if ( nickname ) {
	donnentry = PR_TRUE;
    }

    subjectEntry = ReadDBSubjectEntry(handle, &cert->derSubject);
	
    if ( subjectEntry ) {
	donnentry = PR_FALSE;
	nickname = subjectEntry->nickname;
    }
    
    certEntry = NewDBCertEntry(&cert->derCert, nickname, trust, 0);
    if ( certEntry == NULL ) {
	goto loser;
    }
    
    if ( donnentry ) {
	nicknameEntry = NewDBNicknameEntry(nickname, &cert->derSubject, 0);
	if ( nicknameEntry == NULL ) {
	    goto loser;
	}
    }
    
    rv = WriteDBCertEntry(handle, certEntry);
    if ( rv != SECSuccess ) {
	goto loser;
    }
    state = 1;
    
    if ( nicknameEntry ) {
	rv = WriteDBNicknameEntry(handle, nicknameEntry);
	if ( rv != SECSuccess ) {
	    goto loser;
	}
    }
    
    state = 2;

    cert->dbhandle = handle;
    
    /* add to or create new subject entry */
    if ( subjectEntry ) {
	/* REWRITE BASED ON SUBJECT ENTRY */
	cert->dbhandle = handle;
	rv = AddPermSubjectNode(subjectEntry, cert, nickname);
	if ( rv != SECSuccess ) {
	    goto loser;
	}
    } else {
	/* make a new subject entry - this case is only used when updating
	 * an old version of the database.  This is OK because the oldnickname
	 * db format didn't allow multiple certs with the same subject.
	 */
	/* where does subjectKeyID and certKey come from? */
	subjectEntry = NewDBSubjectEntry(&cert->derSubject, &cert->certKey,
					 &cert->subjectKeyID, nickname,
					 NULL, 0);
	if ( subjectEntry == NULL ) {
	    goto loser;
	}
	rv = WriteDBSubjectEntry(handle, subjectEntry);
	if ( rv != SECSuccess ) {
	    goto loser;
	}
    }
    
    state = 3;
    
    if ( nicknameEntry ) {
	DestroyDBEntry((certDBEntry *)nicknameEntry);
    }
    
    if ( subjectEntry ) {
	DestroyDBEntry((certDBEntry *)subjectEntry);
    }

    return(certEntry);

loser:
    /* don't leave partial entry in the database */
    if ( state > 0 ) {
	rv = DeleteDBCertEntry(handle, &cert->certKey);
    }
    if ( ( state > 1 ) && donnentry ) {
	rv = DeleteDBNicknameEntry(handle, nickname);
    }
    if ( state > 2 ) {
	rv = DeleteDBSubjectEntry(handle, &cert->derSubject);
    }
    if ( certEntry ) {
	DestroyDBEntry((certDBEntry *)certEntry);
    }
    if ( nicknameEntry ) {
	DestroyDBEntry((certDBEntry *)nicknameEntry);
    }
    if ( subjectEntry ) {
	DestroyDBEntry((certDBEntry *)subjectEntry);
    }

    return(NULL);
}

/*
 * NOTE - Version 6 DB did not go out to the real world in a release,
 * so we can remove this function in a later release.
 */
static SECStatus
UpdateV6DB(NSSLOWCERTCertDBHandle *handle, DB *updatedb)
{
    int ret;
    DBT key, data;
    unsigned char *buf, *tmpbuf = NULL;
    certDBEntryType type;
    certDBEntryNickname *nnEntry = NULL;
    certDBEntrySubject *subjectEntry = NULL;
    certDBEntrySMime *emailEntry = NULL;
    char *nickname;
    char *emailAddr;
    SECStatus rv;
    
    /*
     * Sequence through the old database and copy all of the entries
     * to the new database.  Subject name entries will have the new
     * fields inserted into them (with zero length).
     */
    ret = (* updatedb->seq)(updatedb, &key, &data, R_FIRST);
    if ( ret ) {
	return(SECFailure);
    }

    do {
	buf = (unsigned char *)data.data;
	
	if ( data.size >= 3 ) {
	    if ( buf[0] == 6 ) { /* version number */
		type = (certDBEntryType)buf[1];
		if ( type == certDBEntryTypeSubject ) {
		    /* expando subjecto entrieo */
		    tmpbuf = (unsigned char *)PORT_Alloc(data.size + 4);
		    if ( tmpbuf ) {
			/* copy header stuff */
			PORT_Memcpy(tmpbuf, buf, SEC_DB_ENTRY_HEADER_LEN + 2);
			/* insert 4 more bytes of zero'd header */
			PORT_Memset(&tmpbuf[SEC_DB_ENTRY_HEADER_LEN + 2],
				    0, 4);
			/* copy rest of the data */
			PORT_Memcpy(&tmpbuf[SEC_DB_ENTRY_HEADER_LEN + 6],
				    &buf[SEC_DB_ENTRY_HEADER_LEN + 2],
				    data.size - (SEC_DB_ENTRY_HEADER_LEN + 2));

			data.data = (void *)tmpbuf;
			data.size += 4;
			buf = tmpbuf;
		    }
		} else if ( type == certDBEntryTypeCert ) {
		    /* expando certo entrieo */
		    tmpbuf = (unsigned char *)PORT_Alloc(data.size + 3);
		    if ( tmpbuf ) {
			/* copy header stuff */
			PORT_Memcpy(tmpbuf, buf, SEC_DB_ENTRY_HEADER_LEN);

			/* copy trust flage, setting msb's to 0 */
			tmpbuf[SEC_DB_ENTRY_HEADER_LEN] = 0;
			tmpbuf[SEC_DB_ENTRY_HEADER_LEN+1] =
			    buf[SEC_DB_ENTRY_HEADER_LEN];
			tmpbuf[SEC_DB_ENTRY_HEADER_LEN+2] = 0;
			tmpbuf[SEC_DB_ENTRY_HEADER_LEN+3] =
			    buf[SEC_DB_ENTRY_HEADER_LEN+1];
			tmpbuf[SEC_DB_ENTRY_HEADER_LEN+4] = 0;
			tmpbuf[SEC_DB_ENTRY_HEADER_LEN+5] =
			    buf[SEC_DB_ENTRY_HEADER_LEN+2];
			
			/* copy rest of the data */
			PORT_Memcpy(&tmpbuf[SEC_DB_ENTRY_HEADER_LEN + 6],
				    &buf[SEC_DB_ENTRY_HEADER_LEN + 3],
				    data.size - (SEC_DB_ENTRY_HEADER_LEN + 3));

			data.data = (void *)tmpbuf;
			data.size += 3;
			buf = tmpbuf;
		    }

		}

		/* update the record version number */
		buf[0] = CERT_DB_FILE_VERSION;

		/* copy to the new database */
		ret = certdb_Put(handle->permCertDB, &key, &data, 0);
		if ( tmpbuf ) {
		    PORT_Free(tmpbuf);
		    tmpbuf = NULL;
		}
	    }
	}
    } while ( (* updatedb->seq)(updatedb, &key, &data, R_NEXT) == 0 );

    ret = certdb_Sync(handle->permCertDB, 0);

    ret = (* updatedb->seq)(updatedb, &key, &data, R_FIRST);
    if ( ret ) {
	return(SECFailure);
    }

    do {
	buf = (unsigned char *)data.data;
	
	if ( data.size >= 3 ) {
	    if ( buf[0] == CERT_DB_FILE_VERSION ) { /* version number */
		type = (certDBEntryType)buf[1];
		if ( type == certDBEntryTypeNickname ) {
		    nickname = &((char *)key.data)[1];

		    /* get the matching nickname entry in the new DB */
		    nnEntry = ReadDBNicknameEntry(handle, nickname);
		    if ( nnEntry == NULL ) {
			goto endloop;
		    }
		    
		    /* find the subject entry pointed to by nickname */
		    subjectEntry = ReadDBSubjectEntry(handle,
						      &nnEntry->subjectName);
		    if ( subjectEntry == NULL ) {
			goto endloop;
		    }
		    
		    subjectEntry->nickname =
			(char *)PORT_ArenaAlloc(subjectEntry->common.arena,
						key.size - 1);
		    if ( subjectEntry->nickname ) {
			PORT_Memcpy(subjectEntry->nickname, nickname,
				    key.size - 1);
			rv = WriteDBSubjectEntry(handle, subjectEntry);
		    }
		} else if ( type == certDBEntryTypeSMimeProfile ) {
		    emailAddr = &((char *)key.data)[1];

		    /* get the matching smime entry in the new DB */
		    emailEntry = nsslowcert_ReadDBSMimeEntry(handle, emailAddr);
		    if ( emailEntry == NULL ) {
			goto endloop;
		    }
		    
		    /* find the subject entry pointed to by nickname */
		    subjectEntry = ReadDBSubjectEntry(handle,
						      &emailEntry->subjectName);
		    if ( subjectEntry == NULL ) {
			goto endloop;
		    }
		    
		    subjectEntry->nickname =
			(char *)PORT_ArenaAlloc(subjectEntry->common.arena,
						key.size - 1);
		    if ( subjectEntry->emailAddr ) {
			PORT_Memcpy(subjectEntry->emailAddr, emailAddr,
				    key.size - 1);
			rv = WriteDBSubjectEntry(handle, subjectEntry);
		    }
		}
		
endloop:
		if ( subjectEntry ) {
		    DestroyDBEntry((certDBEntry *)subjectEntry);
		    subjectEntry = NULL;
		}
		if ( nnEntry ) {
		    DestroyDBEntry((certDBEntry *)nnEntry);
		    nnEntry = NULL;
		}
		if ( emailEntry ) {
		    DestroyDBEntry((certDBEntry *)emailEntry);
		    emailEntry = NULL;
		}
	    }
	}
    } while ( (* updatedb->seq)(updatedb, &key, &data, R_NEXT) == 0 );

    ret = certdb_Sync(handle->permCertDB, 0);

    (* updatedb->close)(updatedb);
    return(SECSuccess);
}


static SECStatus
updateV5Callback(NSSLOWCERTCertificate *cert, SECItem *k, void *pdata)
{
    NSSLOWCERTCertDBHandle *handle;
    certDBEntryCert *entry;
    NSSLOWCERTCertTrust *trust;
    
    handle = (NSSLOWCERTCertDBHandle *)pdata;
    trust = &cert->dbEntry->trust;

    /* SSL user certs can be used for email if they have an email addr */
    if ( cert->emailAddr && ( trust->sslFlags & CERTDB_USER ) &&
	( trust->emailFlags == 0 ) ) {
	trust->emailFlags = CERTDB_USER;
    }
    /* servers didn't set the user flags on the server cert.. */
    if (PORT_Strcmp(cert->dbEntry->nickname,"Server-Cert") == 0) {
	trust->sslFlags |= CERTDB_USER;
    }
    
    entry = AddCertToPermDB(handle, cert, cert->dbEntry->nickname,
			    &cert->dbEntry->trust);
    if ( entry ) {
	DestroyDBEntry((certDBEntry *)entry);
    }
    
    return(SECSuccess);
}

static SECStatus
UpdateV5DB(NSSLOWCERTCertDBHandle *handle, DB *updatedb)
{
    NSSLOWCERTCertDBHandle updatehandle;
    SECStatus rv;
    
    updatehandle.permCertDB = updatedb;
    updatehandle.dbMon = PZ_NewMonitor(nssILockCertDB);
    
    rv = nsslowcert_TraversePermCerts(&updatehandle, updateV5Callback,
			       (void *)handle);
    
    PZ_DestroyMonitor(updatehandle.dbMon);

    (* updatedb->close)(updatedb);
    return(SECSuccess);
}

static PRBool
isV4DB(DB *db) {
    DBT key,data;
    int ret;

    key.data = "Version";
    key.size = 7;

    ret = (*db->get)(db, &key, &data, 0);
    if (ret) {
	return PR_FALSE;
    }

    if ((data.size == 1) && (*(unsigned char *)data.data <= 4))  {
	return PR_TRUE;
    }

    return PR_FALSE;
}

static SECStatus
UpdateV4DB(NSSLOWCERTCertDBHandle *handle, DB *updatedb)
{
    DBT key, data;
    certDBEntryCert *entry, *entry2;
    SECItem derSubject;
    int ret;
    PRArenaPool *arena = NULL;
    NSSLOWCERTCertificate *cert;

    ret = (* updatedb->seq)(updatedb, &key, &data, R_FIRST);

    if ( ret ) {
	return(SECFailure);
    }

    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if (arena == NULL) {
	return(SECFailure);
    }
    
    do {
	if ( data.size != 1 ) { /* skip version number */

	    /* decode the old DB entry */
	    entry = (certDBEntryCert *)DecodeV4DBCertEntry((unsigned char*)data.data, data.size);
	    derSubject.data = NULL;
	    
	    if ( entry ) {
		cert = nsslowcert_DecodeDERCertificate(&entry->derCert, PR_TRUE,
						 entry->nickname);

		if ( cert != NULL ) {
		    /* add to new database */
		    entry2 = AddCertToPermDB(handle, cert, entry->nickname,
					     &entry->trust);
		    
		    nsslowcert_DestroyCertificate(cert);
		    if ( entry2 ) {
			DestroyDBEntry((certDBEntry *)entry2);
		    }
		}
		DestroyDBEntry((certDBEntry *)entry);
	    }
	}
    } while ( (* updatedb->seq)(updatedb, &key, &data, R_NEXT) == 0 );

    PORT_FreeArena(arena, PR_FALSE);
    (* updatedb->close)(updatedb);
    return(SECSuccess);
}


/*
 * return true if a database key conflict exists
 */
PRBool
nsslowcert_CertDBKeyConflict(SECItem *derCert, NSSLOWCERTCertDBHandle *handle)
{
    SECStatus rv;
    DBT tmpdata;
    DBT namekey;
    int ret;
    SECItem keyitem;
    PRArenaPool *arena = NULL;
    SECItem derKey;
    
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( arena == NULL ) {
	goto loser;
    }

    /* get the db key of the cert */
    rv = nsslowcert_KeyFromDERCert(arena, derCert, &derKey);
    if ( rv != SECSuccess ) {
        goto loser;
    }

    rv = EncodeDBCertKey(&derKey, arena, &keyitem);
    if ( rv != SECSuccess ) {
	goto loser;
    }
    
    namekey.data = keyitem.data;
    namekey.size = keyitem.len;
    
    ret = certdb_Get(handle->permCertDB, &namekey, &tmpdata, 0);
    if ( ret == 0 ) {
	goto loser;
    }

    PORT_FreeArena(arena, PR_FALSE);
    
    return(PR_FALSE);
loser:
    if ( arena ) {
	PORT_FreeArena(arena, PR_FALSE);
    }
    
    return(PR_TRUE);
}

/*
 * return true if a nickname conflict exists
 * NOTE: caller must have already made sure that this exact cert
 * doesn't exist in the DB
 */
static PRBool
nsslowcert_CertNicknameConflict(char *nickname, SECItem *derSubject,
			 NSSLOWCERTCertDBHandle *handle)
{
    PRBool rv;
    certDBEntryNickname *entry;
    
    if ( nickname == NULL ) {
	return(PR_FALSE);
    }
    
    entry = ReadDBNicknameEntry(handle, nickname);

    if ( entry == NULL ) {
	/* no entry for this nickname, so no conflict */
	return(PR_FALSE);
    }

    rv = PR_TRUE;
    if ( SECITEM_CompareItem(derSubject, &entry->subjectName) == SECEqual ) {
	/* if subject names are the same, then no conflict */
	rv = PR_FALSE;
    }

    DestroyDBEntry((certDBEntry *)entry);
    return(rv);
}

/*
 * Open the certificate database and index databases.  Create them if
 * they are not there or bad.
 */
static SECStatus
nsslowcert_OpenPermCertDB(NSSLOWCERTCertDBHandle *handle, PRBool readOnly,
		   NSSLOWCERTDBNameFunc namecb, void *cbarg)
{
    SECStatus rv;
    int openflags;
    certDBEntryVersion *versionEntry = NULL;
    DB *updatedb = NULL;
    char *tmpname;
    char *certdbname;
    PRBool updated = PR_FALSE;
    PRBool forceUpdate = PR_FALSE;
    
    certdbname = (* namecb)(cbarg, CERT_DB_FILE_VERSION);
    if ( certdbname == NULL ) {
	return(SECFailure);
    }
    
    if ( readOnly ) {
	openflags = O_RDONLY;
    } else {
	openflags = O_RDWR;
    }
    
    /*
     * first open the permanent file based database.
     */
    handle->permCertDB = dbopen( certdbname, openflags, 0600, DB_HASH, 0 );

    /* check for correct version number */
    if ( handle->permCertDB ) {
	versionEntry = ReadDBVersionEntry(handle);

	if ( versionEntry == NULL ) {
	    /* no version number */
	    certdb_Close(handle->permCertDB);
	    handle->permCertDB = 0;
	} else if ( versionEntry->common.version != CERT_DB_FILE_VERSION ) {
	    /* wrong version number, can't update in place */
	    DestroyDBEntry((certDBEntry *)versionEntry);
	    PORT_Free(certdbname);
	    return(SECFailure);
	} else {
	    DestroyDBEntry((certDBEntry *)versionEntry);
	    versionEntry = NULL;
	}
    }


    /* if first open fails, try to create a new DB */
    if ( handle->permCertDB == NULL ) {

	/* don't create if readonly */
	if ( readOnly ) {
	    goto loser;
	}
	
	handle->permCertDB = dbopen(certdbname,
				    O_RDWR | O_CREAT | O_TRUNC,
				    0600, DB_HASH, 0);

	/* if create fails then we lose */
	if ( handle->permCertDB == 0 ) {
	    goto loser;
	}

	versionEntry = NewDBVersionEntry(0);
	if ( versionEntry == NULL ) {
	    goto loser;
	}
	
	rv = WriteDBVersionEntry(handle, versionEntry);

	DestroyDBEntry((certDBEntry *)versionEntry);

	if ( rv != SECSuccess ) {
	    goto loser;
	}

	/* try to upgrade old db here */
	tmpname = (* namecb)(cbarg, 6);	/* get v6 db name */
	if ( tmpname ) {
	    updatedb = dbopen( tmpname, O_RDONLY, 0600, DB_HASH, 0 );
	    PORT_Free(tmpname);
	    if ( updatedb ) {
		rv = UpdateV6DB(handle, updatedb);
		if ( rv != SECSuccess ) {
		    goto loser;
		}
		updated = PR_TRUE;
	    } else { /* no v6 db, so try v5 db */
		tmpname = (* namecb)(cbarg, 5);	/* get v5 db name */
		if ( tmpname ) {
		    updatedb = dbopen( tmpname, O_RDONLY, 0600, DB_HASH, 0 );
		    PORT_Free(tmpname);
		    if ( updatedb ) {
			rv = UpdateV5DB(handle, updatedb);
			if ( rv != SECSuccess ) {
			    goto loser;
			}
			updated = PR_TRUE;
		    } else { /* no v5 db, so try v4 db */
			/* try to upgrade v4 db */
			tmpname = (* namecb)(cbarg, 4);	/* get v4 db name */
			if ( tmpname ) {
			    updatedb = dbopen( tmpname, O_RDONLY, 0600,
					      DB_HASH, 0 );
			    PORT_Free(tmpname);
			    if ( updatedb ) {
				/* NES has v5 db's with v4 db names! */
				if (isV4DB(updatedb)) {
				    rv = UpdateV4DB(handle, updatedb);
				} else {
				    rv = UpdateV5DB(handle, updatedb);
				}
				if ( rv != SECSuccess ) {
				    goto loser;
				}
				forceUpdate = PR_TRUE;
				updated = PR_TRUE;
			    }
			}
		    }
		}
	    }
	}

	/* Root certs are no longer automatically added to the DB. They
	 * come from and external PKCS #11 file.
	 */
    }

    PORT_Free(certdbname);
    
    return (SECSuccess);
    
loser:

    PORT_SetError(SEC_ERROR_BAD_DATABASE);
    
    if ( handle->permCertDB ) {
	certdb_Close(handle->permCertDB);
	handle->permCertDB = 0;
    }

    PORT_Free(certdbname);

    return(SECFailure);
}

/*
 * delete all DB records associated with a particular certificate
 */
static SECStatus
DeletePermCert(NSSLOWCERTCertificate *cert)
{
    SECStatus rv;
    SECStatus ret;

    ret = SECSuccess;
    
    rv = DeleteDBCertEntry(cert->dbhandle, &cert->certKey);
    if ( rv != SECSuccess ) {
	ret = SECFailure;
    }
    
    if ( cert->nickname ) {
	rv = DeleteDBNicknameEntry(cert->dbhandle, cert->nickname);
	if ( rv != SECSuccess ) {
	    ret = SECFailure;
	}
    }
    
    rv = RemovePermSubjectNode(cert);

    return(ret);
}

/*
 * Delete a certificate from the permanent database.
 */
SECStatus
nsslowcert_DeletePermCertificate(NSSLOWCERTCertificate *cert)
{
    SECStatus rv;
    
    nsslowcert_LockDB(cert->dbhandle);
    /* delete the records from the permanent database */
    rv = DeletePermCert(cert);

    /* get rid of dbcert and stuff pointing to it */
    DestroyDBEntry((certDBEntry *)cert->dbEntry);
    cert->dbEntry = NULL;
    cert->trust = NULL;

    nsslowcert_UnlockDB(cert->dbhandle);
    return(rv);
}

/*
 * Traverse all of the entries in the database of a particular type
 * call the given function for each one.
 */
SECStatus
nsslowcert_TraverseDBEntries(NSSLOWCERTCertDBHandle *handle,
		      certDBEntryType type,
		      SECStatus (* callback)(SECItem *data, SECItem *key,
					    certDBEntryType type, void *pdata),
		      void *udata )
{
    DBT data;
    DBT key;
    SECStatus rv;
    int ret;
    SECItem dataitem;
    SECItem keyitem;
    unsigned char *buf;
    unsigned char *keybuf;
    
    ret = certdb_Seq(handle->permCertDB, &key, &data, R_FIRST);

    if ( ret ) {
	return(SECFailure);
    }
    
    do {
	buf = (unsigned char *)data.data;
	
	if ( buf[1] == (unsigned char)type ) {
	    dataitem.len = data.size;
	    dataitem.data = buf;
            dataitem.type = siBuffer;
	    keyitem.len = key.size - SEC_DB_KEY_HEADER_LEN;
	    keybuf = (unsigned char *)key.data;
	    keyitem.data = &keybuf[SEC_DB_KEY_HEADER_LEN];
            keyitem.type = siBuffer;
	    
	    rv = (* callback)(&dataitem, &keyitem, type, udata);
	    if ( rv != SECSuccess ) {
		return(rv);
	    }
	}
    } while ( certdb_Seq(handle->permCertDB, &key, &data, R_NEXT) == 0 );

    return(SECSuccess);
}
/*
 * Decode a certificate and enter it into the temporary certificate database.
 * Deal with nicknames correctly
 *
 * This is the private entry point.
 */
static NSSLOWCERTCertificate *
DecodeACert(NSSLOWCERTCertDBHandle *handle, certDBEntryCert *entry)
{
    NSSLOWCERTCertificate *cert = NULL;
    
    cert = nsslowcert_DecodeDERCertificate(&entry->derCert, PR_TRUE,
							 entry->nickname );
    
    if ( cert == NULL ) {
	goto loser;
    }

    cert->dbhandle = handle;
    cert->dbEntry = entry;
    cert->trust = &entry->trust;

    return(cert);

loser:
    return(0);
}

typedef struct {
    PermCertCallback certfunc;
    NSSLOWCERTCertDBHandle *handle;
    void *data;
} PermCertCallbackState;

/*
 * traversal callback to decode certs and call callers callback
 */
static SECStatus
certcallback(SECItem *dbdata, SECItem *dbkey, certDBEntryType type, void *data)
{
    PermCertCallbackState *mystate;
    SECStatus rv;
    certDBEntryCert *entry;
    SECItem entryitem;
    NSSLOWCERTCertificate *cert;
    PRArenaPool *arena = NULL;
    
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( arena == NULL ) {
	goto loser;
    }
    
    entry = (certDBEntryCert *)PORT_ArenaAlloc(arena, sizeof(certDBEntryCert));
    mystate = (PermCertCallbackState *)data;
    entry->common.version = (unsigned int)dbdata->data[0];
    entry->common.type = (certDBEntryType)dbdata->data[1];
    entry->common.flags = (unsigned int)dbdata->data[2];
    entry->common.arena = arena;
    
    entryitem.len = dbdata->len - SEC_DB_ENTRY_HEADER_LEN;
    entryitem.data = &dbdata->data[SEC_DB_ENTRY_HEADER_LEN];
    
    rv = DecodeDBCertEntry(entry, &entryitem);
    if (rv != SECSuccess ) {
	goto loser;
    }
    entry->derCert.type = siBuffer;
   
    /* note: Entry is 'inheritted'.  */
    cert = DecodeACert(mystate->handle, entry);

    rv = (* mystate->certfunc)(cert, dbkey, mystate->data);

    /* arena stored in entry destroyed by nsslowcert_DestroyCertificate */
    nsslowcert_DestroyCertificateNoLocking(cert);

    return(rv);

loser:
    if ( arena ) {
	PORT_FreeArena(arena, PR_FALSE);
    }
    return(SECFailure);
}

/*
 * Traverse all of the certificates in the permanent database and
 * call the given function for each one; expect the caller to have lock.
 */
static SECStatus
TraversePermCertsNoLocking(NSSLOWCERTCertDBHandle *handle,
			   SECStatus (* certfunc)(NSSLOWCERTCertificate *cert,
						  SECItem *k,
						  void *pdata),
			   void *udata )
{
    SECStatus rv;
    PermCertCallbackState mystate;

    mystate.certfunc = certfunc;
    mystate.handle = handle;
    mystate.data = udata;
    rv = nsslowcert_TraverseDBEntries(handle, certDBEntryTypeCert, certcallback,
			       (void *)&mystate);
    
    return(rv);
}

/*
 * Traverse all of the certificates in the permanent database and
 * call the given function for each one.
 */
SECStatus
nsslowcert_TraversePermCerts(NSSLOWCERTCertDBHandle *handle,
		      SECStatus (* certfunc)(NSSLOWCERTCertificate *cert, SECItem *k,
					    void *pdata),
		      void *udata )
{
    SECStatus rv;

    nsslowcert_LockDB(handle);
    rv = TraversePermCertsNoLocking(handle, certfunc, udata);
    nsslowcert_UnlockDB(handle);
    
    return(rv);
}



/*
 * Close the database
 */
void
nsslowcert_ClosePermCertDB(NSSLOWCERTCertDBHandle *handle)
{
    if ( handle ) {
	if ( handle->permCertDB ) {
	    certdb_Close( handle->permCertDB );
	    handle->permCertDB = NULL;
	}
	if (handle->dbMon) {
    	    PZ_DestroyMonitor(handle->dbMon);
	    handle->dbMon = NULL;
	}
    }
    return;
}

/*
 * Get the trust attributes from a certificate
 */
SECStatus
nsslowcert_GetCertTrust(NSSLOWCERTCertificate *cert, NSSLOWCERTCertTrust *trust)
{
    SECStatus rv;
    
    nsslowcert_LockCertTrust(cert);
    
    if ( cert->trust == NULL ) {
	rv = SECFailure;
    } else {
	*trust = *cert->trust;
	rv = SECSuccess;
    }
    
    nsslowcert_UnlockCertTrust(cert);
    return(rv);
}

/*
 * Change the trust attributes of a certificate and make them permanent
 * in the database.
 */
SECStatus
nsslowcert_ChangeCertTrust(NSSLOWCERTCertDBHandle *handle, 
	 	 	NSSLOWCERTCertificate *cert, NSSLOWCERTCertTrust *trust)
{
    certDBEntryCert *entry;
    int rv;
    SECStatus ret;
    
    nsslowcert_LockDB(handle);
    nsslowcert_LockCertTrust(cert);
    /* only set the trust on permanent certs */
    if ( cert->trust == NULL ) {
	ret = SECFailure;
	goto done;
    }

    *cert->trust = *trust;
    if ( cert->dbEntry == NULL ) {
	ret = SECSuccess; /* not in permanent database */
	goto done;
    }
    
    entry = cert->dbEntry;
    entry->trust = *trust;
    
    rv = WriteDBCertEntry(handle, entry);
    if ( rv ) {
	ret = SECFailure;
	goto done;
    }

    ret = SECSuccess;
    
done:
    nsslowcert_UnlockCertTrust(cert);
    nsslowcert_UnlockDB(handle);
    return(ret);
}


SECStatus
nsslowcert_AddPermCert(NSSLOWCERTCertDBHandle *dbhandle,
    NSSLOWCERTCertificate *cert, char *nickname, NSSLOWCERTCertTrust *trust)
{
    char *oldnn;
    certDBEntryCert *entry;
    PRBool conflict;
    SECStatus ret;

    nsslowcert_LockDB(dbhandle);
    
    PORT_Assert(!cert->dbEntry);

    /* don't add a conflicting nickname */
    conflict = nsslowcert_CertNicknameConflict(nickname, &cert->derSubject,
					dbhandle);
    if ( conflict ) {
	ret = SECFailure;
	goto done;
    }
    
    /* save old nickname so that we can delete it */
    oldnn = cert->nickname;

    entry = AddCertToPermDB(dbhandle, cert, nickname, trust);
    
    if ( entry == NULL ) {
	ret = SECFailure;
	goto done;
    }
    
    cert->nickname = (entry->nickname) ? PORT_ArenaStrdup(cert->arena,entry->nickname) : NULL;
    cert->trust = &entry->trust;
    cert->dbEntry = entry;
    
    ret = SECSuccess;
done:
    nsslowcert_UnlockDB(dbhandle);
    return(ret);
}

/*
 * Open the certificate database and index databases.  Create them if
 * they are not there or bad.
 */
SECStatus
nsslowcert_OpenCertDB(NSSLOWCERTCertDBHandle *handle, PRBool readOnly,
		NSSLOWCERTDBNameFunc namecb, void *cbarg, PRBool openVolatile)
{
    int rv;

    certdb_InitDBLock(handle);
    
    handle->dbMon = PZ_NewMonitor(nssILockCertDB);
    PORT_Assert(handle->dbMon != NULL);

    rv = nsslowcert_OpenPermCertDB(handle, readOnly, namecb, cbarg);
    if ( rv ) {
	goto loser;
    }

    return (SECSuccess);
    
loser:

    PORT_SetError(SEC_ERROR_BAD_DATABASE);
    return(SECFailure);
}


/*
 * Lookup a certificate in the databases.
 */
static NSSLOWCERTCertificate *
FindCertByKey(NSSLOWCERTCertDBHandle *handle, SECItem *certKey, PRBool lockdb)
{
    SECItem keyitem;
    DBT key;
    SECStatus rv;
    NSSLOWCERTCertificate *cert = NULL;
    PRArenaPool *arena = NULL;
    certDBEntryCert *entry;
    PRBool locked = PR_FALSE;
    
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( arena == NULL ) {
	goto loser;
    }
    
    rv = EncodeDBCertKey(certKey, arena, &keyitem);
    if ( rv != SECSuccess ) {
	goto loser;
    }
    
    key.data = keyitem.data;
    key.size = keyitem.len;
    
    if ( lockdb ) {
	locked = PR_TRUE;
	nsslowcert_LockDB(handle);
    }
	
    /* find in perm database */
    entry = ReadDBCertEntry(handle, certKey);
	
    if ( entry == NULL ) {
 	goto loser;
    }
  
    /* inherit entry */  
    cert = DecodeACert(handle, entry);

loser:
    if ( locked ) {
	nsslowcert_UnlockDB(handle);
    }

    if ( arena ) {
	PORT_FreeArena(arena, PR_FALSE);
    }
    
    return(cert);
}

/*
 * Lookup a certificate in the databases without locking
 */
NSSLOWCERTCertificate *
nsslowcert_FindCertByKey(NSSLOWCERTCertDBHandle *handle, SECItem *certKey)
{
    return(FindCertByKey(handle, certKey, PR_FALSE));
}

/*
 * Generate a key from an issuerAndSerialNumber, and find the
 * associated cert in the database.
 */
NSSLOWCERTCertificate *
nsslowcert_FindCertByIssuerAndSN(NSSLOWCERTCertDBHandle *handle, NSSLOWCERTIssuerAndSN *issuerAndSN)
{
    SECItem certKey;
    SECItem *sn = &issuerAndSN->serialNumber;
    SECItem *issuer = &issuerAndSN->derIssuer;
    NSSLOWCERTCertificate *cert;
    int data_left = sn->len-1;
    int data_len = sn->len;
    int index = 0;

    /* automatically detect DER encoded serial numbers and remove the der
     * encoding since the database expects unencoded data. 
     * if it's DER encoded, there must be at least 3 bytes, tag, len, data */
    if ((sn->len >= 3) && (sn->data[0] == 0x2)) {
	/* remove the der encoding of the serial number before generating the
	 * key.. */
	data_left = sn->len-2;
	data_len = sn->data[1];
	index = 2;

	/* extended length ? (not very likely for a serial number) */
	if (data_len & 0x80) {
	    int len_count = data_len & 0x7f;

	    data_len = 0;
	    data_left -= len_count;
	    if (data_left > 0) {
		while (len_count --) {
		    data_len = (data_len << 8) | sn->data[index++];
		}
	    } 
	}
	/* XXX leaving any leading zeros on the serial number for backwards
	 * compatibility
	 */
	/* not a valid der, must be just an unlucky serial number value */
	if (data_len != data_left) {
	    data_len = sn->len;
	    index = 0;
	}
    }

    certKey.data = (unsigned char*)PORT_Alloc(sn->len + issuer->len);
    certKey.len = data_len + issuer->len;
    
    if ( certKey.data == NULL ) {
	return(0);
    }

    /* first try the serial number as hand-decoded above*/
    /* copy the serialNumber */
    PORT_Memcpy(certKey.data, &sn->data[index], data_len);

    /* copy the issuer */
    PORT_Memcpy( &certKey.data[data_len],issuer->data,issuer->len);

    cert = nsslowcert_FindCertByKey(handle, &certKey);
    if (cert) {
	PORT_Free(certKey.data);
	return (cert);
    }

    /* didn't find it, try by der encoded serial number */
    /* copy the serialNumber */
    PORT_Memcpy(certKey.data, sn->data, sn->len);

    /* copy the issuer */
    PORT_Memcpy( &certKey.data[sn->len], issuer->data, issuer->len);
    certKey.len = sn->len + issuer->len;

    cert = nsslowcert_FindCertByKey(handle, &certKey);
    
    PORT_Free(certKey.data);
    
    return(cert);
}

/*
 * look for the given DER certificate in the database
 */
NSSLOWCERTCertificate *
nsslowcert_FindCertByDERCert(NSSLOWCERTCertDBHandle *handle, SECItem *derCert)
{
    PRArenaPool *arena;
    SECItem certKey;
    SECStatus rv;
    NSSLOWCERTCertificate *cert = NULL;
    
    /* create a scratch arena */
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( arena == NULL ) {
	return(NULL);
    }
    
    /* extract the database key from the cert */
    rv = nsslowcert_KeyFromDERCert(arena, derCert, &certKey);
    if ( rv != SECSuccess ) {
	goto loser;
    }

    /* find the certificate */
    cert = nsslowcert_FindCertByKey(handle, &certKey);
    
loser:
    PORT_FreeArena(arena, PR_FALSE);
    return(cert);
}

static void
DestroyCertificate(NSSLOWCERTCertificate *cert, PRBool lockdb)
{
    int refCount;
    NSSLOWCERTCertDBHandle *handle;
    
    if ( cert ) {

	handle = cert->dbhandle;

	/*
	 * handle may be NULL, for example if the cert was created with
	 * nsslowcert_DecodeDERCertificate.
	 */
	if ( lockdb && handle ) {
	    nsslowcert_LockDB(handle);
	}

        nsslowcert_LockCertRefCount(cert);
	PORT_Assert(cert->referenceCount > 0);
	refCount = --cert->referenceCount;
        nsslowcert_UnlockCertRefCount(cert);

	if ( ( refCount == 0 ) ) {
	    certDBEntryCert *entry  = cert->dbEntry;
	    PRArenaPool *    arena  = cert->arena;

	    if ( entry ) {
		DestroyDBEntry((certDBEntry *)entry);
            }

	    /* zero cert before freeing. Any stale references to this cert
	     * after this point will probably cause an exception.  */
	    PORT_Memset(cert, 0, sizeof *cert);

	    cert = NULL;
	    
	    /* free the arena that contains the cert. */
	    PORT_FreeArena(arena, PR_FALSE);
        }
	if ( lockdb && handle ) {
	    nsslowcert_UnlockDB(handle);
	}
    }

    return;
}

void
nsslowcert_DestroyCertificate(NSSLOWCERTCertificate *cert)
{
    DestroyCertificate(cert, PR_TRUE);
    return;
}

static void
nsslowcert_DestroyCertificateNoLocking(NSSLOWCERTCertificate *cert)
{
    DestroyCertificate(cert, PR_FALSE);
    return;
}

/*
 * Lookup a CRL in the databases. We mirror the same fast caching data base
 *  caching stuff used by certificates....?
 */
SECItem *
nsslowcert_FindCrlByKey(NSSLOWCERTCertDBHandle *handle, SECItem *crlKey, 
		char **url, PRBool isKRL)
{
    SECItem keyitem;
    DBT key;
    SECStatus rv;
    SECItem *crl = NULL;
    PRArenaPool *arena = NULL;
    certDBEntryRevocation *entry = NULL;
    certDBEntryType crlType = isKRL ? certDBEntryTypeKeyRevocation  
					: certDBEntryTypeRevocation;
    
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( arena == NULL ) {
	goto loser;
    }
    
    rv = EncodeDBGenericKey(crlKey, arena, &keyitem, crlType);
    if ( rv != SECSuccess ) {
	goto loser;
    }
    
    key.data = keyitem.data;
    key.size = keyitem.len;

    /* find in perm database */
    entry = ReadDBCrlEntry(handle, crlKey, crlType);
	
    if ( entry == NULL ) {
	goto loser;
    }

    if (url && entry->url) {
	*url = PORT_Strdup(entry->url);
    }
    crl = SECITEM_DupItem(&entry->derCrl);

loser:
    if ( arena ) {
	PORT_FreeArena(arena, PR_FALSE);
    }
    if (entry) {
	DestroyDBEntry((certDBEntry *)entry);
    }
    
    return(crl);
}

/*
 * replace the existing URL in the data base with a new one
 */
SECStatus
nsslowcert_AddCrl(NSSLOWCERTCertDBHandle *handle, SECItem *derCrl, 
			SECItem *crlKey, char *url, PRBool isKRL)
{
    SECStatus rv = SECFailure;
    certDBEntryRevocation *entry = NULL;
    certDBEntryType crlType = isKRL ? certDBEntryTypeKeyRevocation  
					: certDBEntryTypeRevocation;
    DeleteDBCrlEntry(handle, crlKey, crlType);

    /* Write the new entry into the data base */
    entry = NewDBCrlEntry(derCrl, url, crlType, 0);
    if (entry == NULL) goto done;

    rv = WriteDBCrlEntry(handle, entry, crlKey);
    if (rv != SECSuccess) goto done;

done:
    if (entry) {
	DestroyDBEntry((certDBEntry *)entry);
    }
    return rv;
}

SECStatus
nsslowcert_DeletePermCRL(NSSLOWCERTCertDBHandle *handle, SECItem *derName,
								 PRBool isKRL)
{
    SECStatus rv;
    certDBEntryType crlType = isKRL ? certDBEntryTypeKeyRevocation  
					: certDBEntryTypeRevocation;
    
    rv = DeleteDBCrlEntry(handle, derName, crlType);
    if (rv != SECSuccess) goto done;
  
done:
    return rv;
}


PRBool
nsslowcert_hasTrust(NSSLOWCERTCertificate *cert)
{
    NSSLOWCERTCertTrust *trust;

    if (cert->trust == NULL) {
	return PR_FALSE;
    }
    trust = cert->trust;
    return !((trust->sslFlags & CERTDB_TRUSTED_UNKNOWN) && 
		(trust->emailFlags & CERTDB_TRUSTED_UNKNOWN) && 
			(trust->objectSigningFlags & CERTDB_TRUSTED_UNKNOWN));
}

/*
 * This function has the logic that decides if another person's cert and
 * email profile from an S/MIME message should be saved.  It can deal with
 * the case when there is no profile.
 */
SECStatus
nsslowcert_SaveSMimeProfile(NSSLOWCERTCertDBHandle *dbhandle, char *emailAddr, 
	SECItem *derSubject, SECItem *emailProfile, SECItem *profileTime)
{
    certDBEntrySMime *entry = NULL;
    SECStatus rv = SECFailure;;

    /* find our existing entry */
    entry = nsslowcert_ReadDBSMimeEntry(dbhandle, emailAddr);

    if ( entry ) {
	/* keep our old db entry consistant for old applications. */
	if (!SECITEM_ItemsAreEqual(derSubject, &entry->subjectName)) {
	    UpdateSubjectWithEmailAddr(dbhandle, &entry->subjectName, NULL);
	} 
	DestroyDBEntry((certDBEntry *)entry);
	entry = NULL;
    }

    /* now save the entry */
    entry = NewDBSMimeEntry(emailAddr, derSubject, emailProfile,
				profileTime, 0);
    if ( entry == NULL ) {
	rv = SECFailure;
	goto loser;
    }

    nsslowcert_LockDB(dbhandle);

    rv = DeleteDBSMimeEntry(dbhandle, emailAddr);
    /* if delete fails, try to write new entry anyway... */

    /* link subject entry back here */
    rv = UpdateSubjectWithEmailAddr(dbhandle, derSubject, emailAddr);
    if ( rv != SECSuccess ) {
	    nsslowcert_UnlockDB(dbhandle);
	    goto loser;
    }
	
    rv = WriteDBSMimeEntry(dbhandle, entry);
    if ( rv != SECSuccess ) {
	    nsslowcert_UnlockDB(dbhandle);
	    goto loser;
    }

    nsslowcert_UnlockDB(dbhandle);

    rv = SECSuccess;
    
loser:
    if ( entry ) {
	DestroyDBEntry((certDBEntry *)entry);
    }
    return(rv);
}

void
nsslowcert_DestroyGlobalLocks()
{
    if (dbLock) {
	PZ_DestroyLock(dbLock);
	dbLock = NULL;
    }
    if (certRefCountLock) {
	PZ_DestroyLock(certRefCountLock);
	certRefCountLock = NULL;
    }
    if (certTrustLock) {
	PZ_DestroyLock(certTrustLock);
	certTrustLock = NULL;
    }
}
