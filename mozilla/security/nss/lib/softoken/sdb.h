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
 * The Original Code is Red Hat, Inc.
 *
 * The Initial Developer of the Original Code is
 * Red Hat, Inc.
 * Portions created by the Initial Developer are Copyright (C) 2005
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Robert Relyea (rrelyea@redhat.com)
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
/*
 * This file implements PKCS 11 on top of our existing security modules
 *
 * For more information about PKCS 11 See PKCS 11 Token Inteface Standard.
 *   This implementation has two slots:
 *	slot 1 is our generic crypto support. It does not require login.
 *   It supports Public Key ops, and all they bulk ciphers and hashes. 
 *   It can also support Private Key ops for imported Private keys. It does 
 *   not have any token storage.
 *	slot 2 is our private key support. It requires a login before use. It
 *   can store Private Keys and Certs as token objects. Currently only private
 *   keys and their associated Certificates are saved on the token.
 *
 *   In this implementation, session objects are only visible to the session
 *   that created or generated them.
 */

/*
 * the following data structures should be moved to a 'rdb.h'.
 */

#ifndef _SDB_H
#define _SDB_H 1
#include "pkcs11t.h"
#include "secitem.h"
#include <sqlite3.h>

#define STATIC_CMD_SIZE 2048

typedef struct SDBFindStr SDBFind;
typedef struct SDBStr SDB;
typedef struct SDBPasswordEntryStr SDBPasswordEntry;

struct SDBStr {
    void *private;
    int  sdb_type;
    int  sdb_flags;
    CK_RV (*sdb_FindObjectsInit)(SDB *sdb, const CK_ATTRIBUTE *template, 
				 int count, SDBFind **find);
    CK_RV (*sdb_FindObjects)(SDB *sdb, SDBFind *find, CK_OBJECT_HANDLE *ids, 
				int arraySize, int *count);
    CK_RV (*sdb_FindObjectsFinal)(SDB *sdb, SDBFind *find);
    CK_RV (*sdb_GetAttributeValue)(SDB *sdb, CK_OBJECT_HANDLE object, 
				CK_ATTRIBUTE *template, int count);
    CK_RV (*sdb_SetAttributeValue)(SDB *sdb, CK_OBJECT_HANDLE object, 
				const CK_ATTRIBUTE *template, int count);
    CK_RV (*sdb_CreateObject)(SDB *sdb, CK_OBJECT_HANDLE *object, 
				const CK_ATTRIBUTE *template, int count);
    CK_RV (*sdb_DestroyObject)(SDB *sdb, CK_OBJECT_HANDLE object);
    CK_RV (*sdb_GetPWEntry)(SDB *sdb, SDBPasswordEntry *entry);
    CK_RV (*sdb_PutPWEntry)(SDB *sdb, SDBPasswordEntry *entry);
    CK_RV (*sdb_Begin)(SDB *sdb);
    CK_RV (*sdb_Commit)(SDB *sdb);
    CK_RV (*sdb_Abort)(SDB *sdb);
    CK_RV (*sdb_Close)(SDB *sdb);
};

struct SDBPasswordEntryStr {
    SECItem salt;
    SECItem value;
    unsigned char data[128];
};

#define SDB_INTERNAL 1

CK_RV s_open(const char *directory, char *certPrefix, char *keyPrefix,
	     int cert_version, int key_version, 
	     int flags, SDB **certdb, SDB **keydb);


/* flags */
#define SDB_RDONLY      1
#define SDB_RDWR        2
#define SDB_CREATE      4

#endif
