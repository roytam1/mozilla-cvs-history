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

#ifndef DEV_H
#define DEV_H

#ifdef DEBUG
static const char DEV_CVS_ID[] = "@(#) $RCSfile$ $Revision$ $Date$ $Name$";
#endif /* DEBUG */

#ifndef DEVT_H
#include "devt.h"
#endif /* DEVT_H */

#ifndef NSSCKT_H
#include "nssckt.h"
#endif /* NSSCKT_H */

#ifndef NSSPKIT_H
#include "nsspkit.h"
#endif /* NSSPKIT_H */

/*
 * nssdev.h
 *
 * This file prototypes the methods of the low-level cryptoki devices.
 *
 *  |-----------|---> NSSSlot <--> NSSToken
 *  | NSSModule |---> NSSSlot <--> NSSToken
 *  |-----------|---> NSSSlot <--> NSSToken
 */

#ifndef DEVT_H
#include "devt.h"
#endif /* DEVT_H */

PR_BEGIN_EXTERN_C

NSS_EXTERN NSSModule *
NSSModule_Create
(
  NSSUTF8 *moduleOpt,
  NSSUTF8 *uriOpt,
  NSSUTF8 *opaqueOpt, /* XXX is this where the mech flags go??? */
  void    *reserved
  /* XXX more?  */
);

/* This is to use the new loading mechanism. */
NSS_EXTERN NSSModule *
NSSModule_CreateFromSpec
(
  NSSUTF8 *moduleSpec
);

NSS_EXTERN PRStatus
NSSModule_Destroy
(
  NSSModule *mod
);

NSS_EXTERN PRStatus
NSSModule_Load
(
  NSSModule *mod
);

NSS_EXTERN PRStatus
NSSModule_Unload
(
  NSSModule *mod
);

NSS_EXTERN PRStatus
NSSModule_LogoutAllSlots
(
  NSSModule *mod
);

NSS_EXTERN NSSSlot **
NSSModule_GetSlots
(
  NSSModule *mod
);

NSS_EXTERN NSSSlot *
NSSModule_FindSlotByName
(
  NSSModule *mod,
  NSSUTF8 *slotName
);

/* This descends from NSSTrustDomain_TraverseCertificates, a questionable
 * function.  Do we want NSS to have access to this at the module level?
 */
NSS_EXTERN PRStatus *
NSSModule_TraverseCertificates
(
  NSSModule *mod,
  PRStatus (*callback)(NSSCertificate *c, void *arg),
  void *arg
);

NSS_EXTERN NSSSlot *
NSSSlot_Create
(
  NSSArena *arenaOpt,
  CK_SLOT_ID slotId,
  NSSModule *parent
);

NSS_EXTERN PRStatus
NSSSlot_Destroy
(
  NSSSlot *slot
);

NSS_EXTERN PRStatus
NSSSlot_Login
(
  NSSSlot *slot
);

NSS_EXTERN PRStatus
NSSSlot_Logout
(
  NSSSlot *slot
);

/*
 * NSSSlot_IsLoggedIn
 * NSSSlot_ChangePassword
 */

NSS_EXTERN NSSToken *
NSSToken_Create
(
  NSSArena *arenaOpt,
  CK_SLOT_ID slotID,
  NSSSlot *parent
);

NSS_EXTERN PRStatus
NSSToken_Destroy
(
  NSSToken *tok
);

/* Given a raw attribute template, import an object 
 * (certificate, public key, private key, symmetric key)
 * Return the object as an NSS type.
 */
NSS_EXTERN NSSCertificate *
NSSToken_ImportCertificate
(
  NSSToken *tok,
  CK_ATTRIBUTE *cktemplate
);

NSS_EXTERN NSSPublicKey *
NSSToken_ImportPublicKey
(
  NSSToken *tok,
  CK_ATTRIBUTE *cktemplate
);

NSS_EXTERN NSSPrivateKey *
NSSToken_ImportPrivateKey
(
  NSSToken *tok,
  CK_ATTRIBUTE *cktemplate
);

NSS_EXTERN NSSSymmetricKey *
NSSToken_ImportSymmetricKey
(
  NSSToken *tok,
  CK_ATTRIBUTE *cktemplate
);

NSS_EXTERN NSSPublicKey *
NSSToken_GenerateKeyPair
(
  NSSToken *tok
  /* algorithm and parameters */
);

NSS_EXTERN NSSSymmetricKey *
NSSToken_GenerateSymmetricKey
(
  NSSToken *tok
  /* algorithm and parameters */
);

/* Permanently remove an object from the token. */
NSS_EXTERN PRStatus
NSSToken_DeleteStoredObject
(
  NSSToken *tok,
  CK_OBJECT_HANDLE object
);

/* again, a questionable function.  maybe some tokens allow this? */
NSS_EXTERN PRStatus *
NSSToken_TraverseCertificates
(
  NSSToken *tok,
  PRStatus (*callback)(NSSCertificate *c, void *arg),
  void *arg
);

#ifdef DEBUG
void NSSModule_Debug(NSSModule *m);
#endif

PR_END_EXTERN_C

#endif /* DEV_H */
