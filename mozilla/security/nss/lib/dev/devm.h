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

#ifndef DEVM_H
#define DEVM_H

#ifdef DEBUG
static const char DEVM_CVS_ID[] = "@(#) $RCSfile$ $Revision$ $Date$ $Name$";
#endif /* DEBUG */

#ifndef BASE_H
#include "base.h"
#endif /* BASE_H */

#ifndef NSSCKT_H
#include "nssckt.h"
#endif /* NSSCKT_H */

#ifndef DEV_H
#include "dev.h"
#endif /* DEV_H */

#ifndef DEVTM_H
#include "devtm.h"
#endif /* DEVTM_H */

PR_BEGIN_EXTERN_C

/* Shortcut to cryptoki API functions. */
#define CKAPI(epv) \
    ((CK_FUNCTION_LIST_PTR)(epv))

NSS_EXTERN void
nssDevice_AddRef (
 struct nssDeviceBaseStr *device
);

NSS_EXTERN PRBool
nssDevice_Destroy (
 struct nssDeviceBaseStr *device
);

NSS_EXTERN PRStatus
nssModule_DestroyFromSlot (
  NSSModule *mod,
  NSSSlot *slot
);

NSS_EXTERN void *
nssModule_GetCryptokiEPV (
  NSSModule *mod
);

NSS_EXTERN NSSSlot *
nssSlot_Create (
  CK_SLOT_ID slotId,
  NSSModule *parent
);

NSS_EXTERN void *
nssSlot_GetCryptokiEPV (
  NSSSlot *slot
);

NSS_EXTERN NSSToken *
nssToken_Create (
  CK_SLOT_ID slotID,
  NSSSlot *peer
);

NSS_EXTERN void *
nssToken_GetCryptokiEPV (
  NSSToken *token
);

NSS_IMPLEMENT PRBool
nssToken_HasSessionLimit (
  NSSToken *token
);

NSS_EXTERN nssSession *
nssToken_GetDefaultSession (
  NSSToken *token
);

NSS_EXTERN PRBool
nssToken_IsLoginRequired (
  NSSToken *token
);

/* XXX */
NSS_EXTERN void
nssToken_Remove (
  NSSToken *token
);

NSS_EXTERN nssCryptokiObject *
nssCryptokiObject_Create (
  NSSToken *t, 
  nssSession *session,
  CK_OBJECT_HANDLE h
);

NSS_EXTERN CK_MECHANISM_PTR
nssAlgNParam_GetMechanism (
  const NSSAlgNParam *ap
);

NSS_EXTERN PRIntn
nssAlgNParam_SetTemplateValues (
  const NSSAlgNParam *ap,
  CK_ATTRIBUTE_PTR aTemplate,
  CK_ULONG templateSize
);

NSS_EXTERN void
nss_SetGenericDeviceError (
  CK_RV ckrv
);

/* PKCS#11 stores strings in a fixed-length buffer padded with spaces.  This
 * function gets the length of the actual string.
 */
NSS_EXTERN PRUint32
nssPKCS11String_Length (
  CK_CHAR *pkcs11str, 
  PRUint32 bufLen
);

PR_END_EXTERN_C

#endif /* DEV_H */
