/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All 
 * Rights Reserved.
 *
 * Contributor(s):
 */

/* describes principals for use with signed scripts */

#ifndef _NS_CERTIFICATE_PRINCIPAL_H_
#define _NS_CERTIFICATE_PRINCIPAL_H_
#include "jsapi.h"
#include "nsICertificatePrincipal.h"
#include "nsBasePrincipal.h"

#define NS_CERTIFICATEPRINCIPALMANAGER_CID \
{ 0x7ee2a4c0, 0x4b91, 0x11d3, \
{ 0xba, 0x18, 0x00, 0x60, 0xb0, 0xf1, 0x99, 0xa2 }}

class nsCertificatePrincipal : public nsICertificatePrincipal, public nsBasePrincipal {
public:

    NS_DEFINE_STATIC_CID_ACCESSOR(NS_CERTIFICATEPRINCIPALMANAGER_CID)
    NS_DECL_ISUPPORTS
    NS_DECL_NSICERTIFICATEPRINCIPAL

    NS_IMETHOD ToString(char **result);

    NS_IMETHOD Equals(nsIPrincipal *other, PRBool *result);

    NS_IMETHOD HashValue(PRUint32 *result);

    NS_IMETHOD CanEnableCapability(const char *capability, PRInt16 *result);

    NS_IMETHOD SetCanEnableCapability(const char *capability, 
                                      PRInt16 canEnable);

	nsCertificatePrincipal(PRInt16 type, const char * key);
	nsCertificatePrincipal(PRInt16 type, const unsigned char ** certChain, PRUint32 * certChainLengths, PRUint32 noOfCerts);
	virtual ~nsCertificatePrincipal(void);

protected:
	PRInt16 itsType;
	const char * itsKey;
	char * itsCompanyName;
	char * itsCertificateAuthority;
	char * itsSerialNumber;
	char * itsExpirationDate;
	char * itsFingerPrint;	
	char * itsNickname;
	char * itsString;
};

#endif // _NS_CERTIFICATE_PRINCIPAL_H_
