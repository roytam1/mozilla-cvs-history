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

#ifndef PKI_H
#define PKI_H

#ifdef DEBUG
static const char PKI_CVS_ID[] = "@(#) $RCSfile$ $Revision$ $Date$ $Name$";
#endif /* DEBUG */

#ifndef NSSDEVT_H
#include "nssdevt.h"
#endif /* NSSDEVT_H */

#ifndef NSSPKI_H
#include "nsspki.h"
#endif /* NSSPKI_H */

#ifndef PKIT_H
#include "pkit.h"
#endif /* PKIT_H */

PR_BEGIN_EXTERN_C

NSS_EXTERN NSSToken *
nssTrustDomain_FindTokenForAlgorithmAndParameters (
  NSSTrustDomain *td,
  const NSSAlgorithmAndParameters *ap
);

NSS_EXTERN NSSToken *
nssTrustDomain_FindTokenForAlgorithm (
  NSSTrustDomain *td,
  const NSSOID *algorithm
);

NSS_EXTERN NSSCallback *
nssTrustDomain_GetDefaultCallback (
  NSSTrustDomain *td,
  PRStatus *statusOpt
);

NSS_EXTERN NSSCertificate **
nssTrustDomain_FindCertificatesByNickname (
  NSSTrustDomain *td,
  NSSUTF8 *name,
  NSSCertificate *rvOpt[],
  PRUint32 maximumOpt, /* 0 for no max */
  NSSArena *arenaOpt
);

NSS_EXTERN NSSCertificate **
nssTrustDomain_FindCertificatesBySubject (
  NSSTrustDomain *td,
  NSSDER *subject,
  NSSCertificate *rvOpt[],
  PRUint32 maximumOpt,
  NSSArena *arenaOpt
);

NSS_EXTERN NSSCertificate *
nssTrustDomain_FindCertificateByIssuerAndSerialNumber (
  NSSTrustDomain *td,
  NSSDER *issuer,
  NSSDER *serialNumber
);

NSS_EXTERN NSSCertificate **
nssTrustDomain_FindCertificatesByEmail (
  NSSTrustDomain *td,
  NSSASCII7 *email,
  NSSCertificate *rvOpt[],
  PRUint32 maximumOpt, /* 0 for no max */
  NSSArena *arenaOpt
);

NSS_EXTERN NSSCertificate *
nssTrustDomain_FindCertificateByEncodedCertificate (
  NSSTrustDomain *td,
  NSSBER *encodedCertificate
);

NSS_EXTERN PRStatus *
nssTrustDomain_TraverseCertificates (
  NSSTrustDomain *td,
  PRStatus (*callback)(NSSCertificate *c, void *arg),
  void *arg
);

NSS_EXTERN nssTrust *
nssTrustDomain_FindTrustForCertificate (
  NSSTrustDomain *td,
  NSSCertificate *c
);

NSS_EXTERN NSSCertificate *
nssCertificate_Decode (
  NSSBER *ber
);

NSS_EXTERN NSSCertificate *
nssCertificate_AddRef (
  NSSCertificate *c
);

NSS_EXTERN PRStatus
nssCertificate_Destroy (
  NSSCertificate *c
);

NSS_EXTERN NSSDER *
nssCertificate_GetEncoding (
  NSSCertificate *c
);

NSS_EXTERN NSSDER *
nssCertificate_GetIssuer (
  NSSCertificate *c
);

NSS_EXTERN NSSDER *
nssCertificate_GetSerialNumber (
  NSSCertificate *c
);

NSS_EXTERN NSSDER *
nssCertificate_GetSubject (
  NSSCertificate *c
);

NSS_EXTERN PRStatus
nssCertificate_SetNickname (
  NSSCertificate *c,
  NSSToken *tokenOpt,
  NSSUTF8 *nickname
);

NSS_EXTERN NSSUTF8 *
nssCertificate_GetNickname (
  NSSCertificate *c,
  NSSToken *tokenOpt
);

NSS_EXTERN NSSASCII7 *
nssCertificate_GetEmailAddress (
  NSSCertificate *c
);

NSS_EXTERN PRBool
nssCertificate_IssuerAndSerialEqual (
  NSSCertificate *c1,
  NSSCertificate *c2
);

NSS_EXTERN NSSPublicKey *
nssCertificate_GetPublicKey (
  NSSCertificate *c
);

NSS_EXTERN NSSPrivateKey *
nssCertificate_FindPrivateKey (
  NSSCertificate *c,
  NSSCallback *uhh
);

NSS_EXTERN PRBool
nssCertificate_IsPrivateKeyAvailable (
  NSSCertificate *c,
  NSSCallback *uhh,
  PRStatus *statusOpt
);

NSS_EXTERN NSSUsages *
nssCertificate_GetUsages (
  NSSCertificate *c,
  PRStatus *statusOpt
);

NSS_EXTERN PRBool
nssCertificate_IsValidAtTime (
  NSSCertificate *c,
  NSSTime time,
  PRStatus *statusOpt
);

NSS_EXTERN PRBool
nssCertificate_IsNewer (
  NSSCertificate *c1,
  NSSCertificate *c2,
  PRStatus *statusOpt
);

NSS_EXTERN NSSCertificate **
nssCertificate_BuildChain (
  NSSCertificate *c,
  NSSTime time,
  NSSUsages *usagesOpt,
  NSSPolicies *policiesOpt,
  NSSCertificate **rvOpt,
  PRUint32 rvLimit,
  NSSArena *arenaOpt,
  PRStatus *statusOpt
);

NSS_EXTERN NSSPrivateKey *
nssPrivateKey_AddRef (
  NSSPrivateKey *vk
);

NSS_EXTERN NSSPrivateKey *
nssPrivateKey_Decode (
  NSSBER *ber,
  NSSOID *keyPairAlg,
  NSSOperations operations,
  NSSProperties properties,
  NSSUTF8 *passwordOpt,
  NSSCallback *uhhOpt,
  NSSToken *destination,
  NSSTrustDomain *td,
  NSSVolatileDomain *vdOpt
);

NSS_EXTERN PRStatus
nssPrivateKey_Destroy (
  NSSPrivateKey *vk
);

NSS_EXTERN NSSItem *
nssPrivateKey_GetID (
  NSSPrivateKey *vk
);

NSS_EXTERN NSSUTF8 *
nssPrivateKey_GetNickname (
  NSSPrivateKey *vk,
  NSSToken *tokenOpt
);

NSS_EXTERN NSSPublicKey *
nssPublicKey_AddRef (
  NSSPublicKey *bk
);

NSS_EXTERN PRStatus
nssPublicKey_Destroy (
  NSSPublicKey *bk
);

NSS_EXTERN NSSItem *
nssPublicKey_GetID (
  NSSPublicKey *vk
);

NSS_EXTERN NSSItem *
nssPublicKey_WrapSymmetricKey (
  NSSPublicKey *bk,
  const NSSAlgorithmAndParameters *ap,
  NSSSymmetricKey *keyToWrap,
  NSSCallback *uhh,
  NSSItem *rvOpt,
  NSSArena *arenaOpt
);

NSS_EXTERN NSSSymmetricKey *
nssSymmetricKey_AddRef (
  NSSSymmetricKey *mk
);

NSS_EXTERN PRStatus
nssSymmetricKey_DeriveSSLSessionKeys (
  NSSSymmetricKey *masterSecret,
  const NSSAlgorithmAndParameters *ap,
  NSSSymmetricKeyType bulkKeyType,
  NSSOperations operations,
  NSSProperties properties,
  PRUint32 keySize,
  NSSSymmetricKey **sessionKeys
);

NSS_EXTERN NSSVolatileDomain *
nssVolatileDomain_Create (
  NSSTrustDomain *td,
  NSSCallback *uhhOpt
);

NSS_EXTERN NSSCertificate **
nssVolatileDomain_FindCertificatesBySubject (
  NSSVolatileDomain *vd,
  NSSDER *subject,
  NSSCertificate *rvOpt[],
  PRUint32 maximumOpt, /* 0 for no max */
  NSSArena *arenaOpt
);

NSS_EXTERN void
nssPublicKeyArray_Destroy (
  NSSPublicKey **bkeys
);

NSS_EXTERN void
nssPrivateKeyArray_Destroy (
  NSSPrivateKey **vkeys
);

NSS_EXTERN void
nssSymmetricKeyArray_Destroy (
  NSSSymmetricKey **mkeys
);

NSS_EXTERN nssTrust *
nssTrust_AddRef (
  nssTrust *trust
);

NSS_EXTERN PRStatus
nssTrust_Destroy (
  nssTrust *trust
);

NSS_EXTERN nssSMIMEProfile *
nssSMIMEProfile_AddRef (
  nssSMIMEProfile *profile
);

NSS_EXTERN PRStatus
nssSMIMEProfile_Destroy (
  nssSMIMEProfile *profile
);

NSS_EXTERN nssSMIMEProfile *
nssSMIMEProfile_Create (
  NSSCertificate *cert,
  NSSItem *profileTime,
  NSSItem *profileData
);

NSS_EXTERN PRBool
nssTime_WithinRange (
  NSSTime time,
  NSSTime start,
  NSSTime finish
);

NSS_EXTERN PRBool
nssTime_IsAfter (
  NSSTime time,
  NSSTime compareTime
);

PR_END_EXTERN_C

#endif /* PKI_H */
