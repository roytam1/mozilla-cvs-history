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

#ifndef NSSPKIT_H
#define NSSPKIT_H

#ifdef DEBUG
static const char NSSPKIT_CVS_ID[] = "@(#) $RCSfile$ $Revision$ $Date$ $Name$";
#endif /* DEBUG */

/*
 * nsspkit.h
 *
 * This file defines the types of the top-level PKI objects.
 */

#ifndef NSSBASET_H
#include "nssbaset.h"
#endif /* NSSBASET_H */

PR_BEGIN_EXTERN_C

/*
 * NSSCertificate
 *
 * This is the public representation of a Certificate.  The certificate
 * may be one found on a smartcard or other token, one decoded from data
 * received as part of a protocol, one constructed from constituent
 * parts, etc.  Usually it is associated with ("in") a trust domain; as
 * it can be verified only within a trust domain.  The underlying type
 * of certificate may be of any supported standard, e.g. PKIX, PGP, etc.
 *
 * People speak of "verifying (with) the server's, or correspondant's, 
 * certificate"; for simple operations we support that simplification
 * by implementing public-key crypto operations as methods on this type.
 */

struct NSSCertificateStr;
typedef struct NSSCertificateStr NSSCertificate;

/*
 * NSSUserCertificate
 *
 * A ``User'' certificate is one for which the private key is available.
 * People speak of "using my certificate to sign my email" and "using
 * my certificate to authenticate to (or login to) the server"; for
 * simple operations, we support that simplification by implementing
 * private-key crypto operations as methods on this type.
 *
 * The current design only weakly distinguishes between certificates
 * and user certificates: as far as the compiler goes they're 
 * interchangable; debug libraries only have one common pointer-tracker;
 * etc.  However, attempts to do private-key operations on a certificate
 * for which the private key is not available will fail.
 *
 * Open design question: should these types be more firmly separated?
 */

typedef NSSCertificate NSSUserCertificate;

/*
 * NSSPrivateKey
 *
 * This is the public representation of a Private Key.  In general,
 * the actual value of the key is not available, but operations may
 * be performed with it.
 */

struct NSSPrivateKeyStr;
typedef struct NSSPrivateKeyStr NSSPrivateKey;

/*
 * NSSPublicKey
 *
 */

struct NSSPublicKeyStr;
typedef struct NSSPublicKeyStr NSSPublicKey;

/*
 * NSSSymmetricKey
 *
 */

struct NSSSymmetricKeyStr;
typedef struct NSSSymmetricKeyStr NSSSymmetricKey;

/*
 * NSSTrustDomain
 *
 * A Trust Domain is the field in which certificates may be validated.
 * A trust domain will generally have one or more cryptographic modules
 * open; these modules perform the cryptographic operations, and 
 * provide the basic "root" trust information from which the trust in
 * a specific certificate or key depends.
 *
 * A client program, or a simple server, would typically have one
 * trust domain.  A server supporting multiple "virtual servers" might
 * have a separate trust domain for each virtual server.  The separate
 * trust domains might share some modules (e.g., a hardware crypto
 * accelerator) but not others (e.g., the tokens storing the different
 * servers' private keys, or the databases with each server's trusted
 * root certificates).
 *
 * This object descends from the "permananet database" in the old code.
 */

typedef struct NSSTrustDomainStr NSSTrustDomain;

typedef struct NSSVolatileDomainStr NSSVolatileDomain;

/* special case of volatile domain */
struct NSSCertificateChainStr;
typedef struct NSSCertificateChainStr NSSCertificateChain;

/*
 * NSSCryptoContext
 *
 * A Crypto Context is a short-term, "helper" object which is used
 * for the lifetime of one ongoing "crypto operation."  Such an
 * operation may be the creation of a signed message, the use of an
 * TLS socket connection, etc.  Each crypto context is "in" a
 * specific trust domain, and it may have associated with it a
 * distinguished certificate, public key, private key, and/or
 * symmetric key.  It can also temporarily hold and use temporary
 * data (e.g. intermediate certificates) which is not stored
 * permanently in the trust domain.
 *
 * In OO terms, this interface inherits interfaces from the trust
 * domain, the certificates, and the keys.  It also provides
 * streaming crypto operations.
 *
 * This object descends from the "temporary database" concept in the
 * old code, but it has changed a lot as a result of what we've 
 * learned.
 */

typedef struct NSSCryptoContextStr NSSCryptoContext;

/*
 * NSSCryptoContextMark
 *
 * Holds the state of a crypto context.  The context operation may
 * be continued, and the continuation can be kept by unmarking, or
 * thrown out by releasing the mark.
 */

typedef struct NSSCryptoContextMarkStr NSSCryptoContextMark;

typedef struct NSSCRLStr NSSCRL;

/*
 * fgmr others
 */

/* 
 * NSSTime
 *
 * Unfortunately, we need an "exceptional" value to indicate
 * an error upon return, or "no value" on input.  Note that zero
 * is a perfectly valid value for both time_t and PRTime.
 *
 * If we were to create a "range" object, with two times for
 * Not Before and Not After, we would have an obvious place for
 * the somewhat arbitrary logic involved in comparing them.
 *
 * Failing that, let's have an NSSTime_CompareRanges function.
 */

typedef PRTime NSSTime;

/*
 * NSSUsage
 *
 */

typedef PRUint32 NSSUsage;

#define NSSUsage_SSLClient            0x0001
#define NSSUsage_SSLServer            0x0002
#define NSSUsage_SSLServerWithStepUp  0x0004
#define NSSUsage_EmailSigner          0x0008
#define NSSUsage_EmailRecipient       0x0010
#define NSSUsage_CodeSigner           0x0020
#define NSSUsage_StatusResponder      0x0040
#define NSSUsage_TimeStamper          0x0080
#define NSSUsage_CRLSigner            0x0100

#define NSSUsage_All                                \
  NSSUsage_SSLClient | NSSUsage_SSLServer |         \
  NSSUsage_SSLServerWithStepUp |                    \
  NSSUsage_EmailSigner | NSSUsage_EmailRecipient |  \
  NSSUsage_CodeSigner | NSSUsage_StatusResponder |  \
  NSSUsage_TimeStamper | NSSUsage_CRLSigner

/*
 * NSSUsages
 *
 */

struct NSSUsagesStr
{
  NSSUsage ca;
  NSSUsage peer;
};

typedef struct NSSUsagesStr NSSUsages;

/*
 * NSSPolicies
 *
 * Placeholder, for now.
 */

struct NSSPoliciesStr;
typedef struct NSSPoliciesStr NSSPolicies;

struct NSSPKIXCertificateStr;

PR_END_EXTERN_C

#endif /* NSSPKIT_H */
