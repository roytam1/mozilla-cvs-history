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

#ifndef PKIT_H
#include "pkit.h"
#endif /* PKIT_H */

#ifndef NSSDEVT_H
#include "nssdevt.h"
#endif /* NSSDEVT_H */

PR_BEGIN_EXTERN_C

NSS_EXTERN NSSCertificate *
nssCertificate_AddRef
(
  NSSCertificate *c
);

NSS_EXTERN NSSUTF8 *
NSSCertificate_GetNickname
(
  NSSCertificate *c,
  NSSToken *tokenOpt
);

/* putting here for now, needs more thought */
NSS_EXTERN PRStatus
nssCryptoContext_ImportTrust
(
  NSSCryptoContext *cc,
  NSSTrust *trust
);

NSS_EXTERN NSSTrust *
nssCryptoContext_FindTrustForCertificate
(
  NSSCryptoContext *cc,
  NSSCertificate *cert
);

NSS_EXTERN PRStatus
nssCryptoContext_ImportSMIMEProfile
(
  NSSCryptoContext *cc,
  nssSMIMEProfile *profile
);

NSS_EXTERN nssSMIMEProfile *
nssCryptoContext_FindSMIMEProfileForCertificate
(
  NSSCryptoContext *cc,
  NSSCertificate *cert
);

NSS_EXTERN NSSTrust *
nssTrust_AddRef
(
  NSSTrust *trust
);

NSS_EXTERN PRStatus
nssTrust_Destroy
(
  NSSTrust *trust
);

NSS_EXTERN nssSMIMEProfile *
nssSMIMEProfile_AddRef
(
  nssSMIMEProfile *profile
);

NSS_EXTERN PRStatus
nssSMIMEProfile_Destroy
(
  nssSMIMEProfile *profile
);

NSS_EXTERN nssSMIMEProfile *
nssSMIMEProfile_Create
(
  NSSCertificate *cert,
  NSSItem *profileTime,
  NSSItem *profileData
);

PR_END_EXTERN_C

#endif /* PKI_H */
