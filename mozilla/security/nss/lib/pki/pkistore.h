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

#ifndef PKISTORE_H
#define PKISTORE_H

#ifdef DEBUG
static const char PKISTORE_CVS_ID[] = "@(#) $RCSfile$ $Revision$ $Date$ $Name$";
#endif /* DEBUG */

#ifndef BASE_H
#include "base.h"
#endif /* BASE_H */

#ifndef NSSPKIT_H
#include "nsspkit.h"
#endif /* NSSPKIT_H */

PR_BEGIN_EXTERN_C

/* 
 * PKI Stores
 *
 * This is a set of routines for managing local stores of PKI objects.
 * Currently, the only application is in crypto contexts, where the
 * certificate store is used.  In the future, methods should be added
 * here for storing local references to keys.
 */

/* 
 * nssCertStore
 *
 * Manages local store of certificate, trust, and S/MIME profile objects.
 * Within a crypto context, mappings of cert to trust and cert to S/MIME
 * profile are always 1-1.  Therefore, it is reasonable to store all objects
 * in a single collection, indexed by the certificate.
 *
 * nssCertStore_Create
 * nssCertStore_Destroy
 * nssCertStore_Add
 * nssCertStore_Remove
 * nssCertStore_FindCertsBySubject
 * nssCertStore_FindCertsByNickname
 * nssCertStore_FindCertsByEmail
 * nssCertStore_FindCertByIssuerAndSerialNumber
 * nssCertStore_FindCertByEncodedCert
 * nssCertStore_AddTrust
 * nssCertStore_FindTrustForCert
 * nssCertStore_AddSMIMEProfile
 * nssCertStore_FindSMIMEProfileForCert
 */

NSS_EXTERN nssCertStore *
nssCertStore_Create (
  NSSArena *arenaOpt
);

NSS_EXTERN void
nssCertStore_Destroy (
  nssCertStore *store
);

NSS_EXTERN PRStatus
nssCertStore_Add (
  nssCertStore *store,
  NSSCert *cert
);

NSS_EXTERN void
nssCertStore_Remove (
  nssCertStore *store,
  NSSCert *cert
);

NSS_EXTERN NSSCert **
nssCertStore_FindCertsBySubject (
  nssCertStore *store,
  NSSDER *subject,
  NSSCert *rvOpt[],
  PRUint32 maximumOpt,
  NSSArena *arenaOpt
);

NSS_EXTERN NSSCert **
nssCertStore_FindCertsByNickname (
  nssCertStore *store,
  NSSUTF8 *nickname,
  NSSCert *rvOpt[],
  PRUint32 maximumOpt,
  NSSArena *arenaOpt
);

NSS_EXTERN NSSCert **
nssCertStore_FindCertsByEmail (
  nssCertStore *store,
  NSSASCII7 *email,
  NSSCert *rvOpt[],
  PRUint32 maximumOpt,
  NSSArena *arenaOpt
);

NSS_EXTERN NSSCert *
nssCertStore_FindCertByIssuerAndSerialNumber (
  nssCertStore *store,
  NSSDER *issuer,
  NSSDER *serial
);

NSS_EXTERN NSSCert *
nssCertStore_FindCertByEncodedCert (
  nssCertStore *store,
  NSSDER *encoding
);

NSS_EXTERN PRStatus
nssCertStore_AddTrust (
  nssCertStore *store,
  nssTrust *trust
);

NSS_EXTERN nssTrust *
nssCertStore_FindTrustForCert (
  nssCertStore *store,
  NSSCert *cert
);

NSS_EXTERN PRStatus
nssCertStore_AddSMIMEProfile (
  nssCertStore *store,
  nssSMIMEProfile *profile
);

NSS_EXTERN nssSMIMEProfile *
nssCertStore_FindSMIMEProfileForCert (
  nssCertStore *store,
  NSSCert *cert
);

NSS_EXTERN void
nssCertStore_DumpStoreInfo (
  nssCertStore *store,
  void (* cert_dump_iter)(const void *, void *, void *),
  void *arg
);

PR_END_EXTERN_C

#endif /* PKISTORE_H */
