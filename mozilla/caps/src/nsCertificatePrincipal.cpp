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
 * Copyright (C) 1998-2000 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 * Norris Boyd
 * Mitch Stoltz <mstoltz@netscape.com>
 */

/*describes principals for use in signed scripts*/
#include "nsCertificatePrincipal.h"
#include "prmem.h"
#include "nsCOMPtr.h"

static NS_DEFINE_IID(kICertificatePrincipalIID, NS_ICERTIFICATEPRINCIPAL_IID);

NS_IMPL_QUERY_INTERFACE2(nsCertificatePrincipal, nsICertificatePrincipal, nsIPrincipal)

NSBASEPRINCIPALS_ADDREF(nsCertificatePrincipal);
NSBASEPRINCIPALS_RELEASE(nsCertificatePrincipal);

//////////////////////////////////////////////////
// Methods implementing nsICertificatePrincipal //
//////////////////////////////////////////////////
NS_IMETHODIMP
nsCertificatePrincipal::GetCertificateID(char** aCertificateID) 
{
    *aCertificateID = nsCRT::strdup(mCertificateID);
    return *mCertificateID ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

NS_IMETHODIMP
nsCertificatePrincipal::GetCommonName(char** aCommonName)
{
    *aCommonName = nsCRT::strdup(mCommonName);
    return *aCommonName ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

NS_IMETHODIMP
nsCertificatePrincipal::SetCommonName(const char* aCommonName)
{
    PR_FREEIF(mCommonName);
    mCommonName = nsCRT::strdup(aCommonName);
    return * mCommonName ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

///////////////////////////////////////
// Methods implementing nsIPrincipal //
///////////////////////////////////////
NS_IMETHODIMP 
nsCertificatePrincipal::CanEnableCapability(const char *capability,
                                            PRInt16 *result)
{
    if(NS_FAILED(nsBasePrincipal::CanEnableCapability(capability, result)))
        return NS_ERROR_FAILURE;
    if (*result == nsIPrincipal::ENABLE_UNKNOWN)
        *result = ENABLE_WITH_USER_PERMISSION;
    return NS_OK;
}

NS_IMETHODIMP 
nsCertificatePrincipal::ToString(char **result)
{
    return GetCertificateID(result);
}

NS_IMETHODIMP 
nsCertificatePrincipal::ToUserVisibleString(char **result)
{
    return GetCommonName(result);
}

NS_IMETHODIMP 
nsCertificatePrincipal::GetPreferences(char** aPrefName, char** aID, 
                                       char** aGrantedList, char** aDeniedList)
{
    if (!mPrefName) {
        nsCAutoString s;
        s.Assign("capability.principal.certificate.p");
        s.AppendInt(mCapabilitiesOrdinal++);
        s.Append(".id");
        mPrefName = s.ToNewCString();
    }
    return nsBasePrincipal::GetPreferences(aPrefName, aID, 
                                           aGrantedList, aDeniedList);
}

NS_IMETHODIMP
nsCertificatePrincipal::Equals(nsIPrincipal * other, PRBool * result)
{
    *result = PR_FALSE;
    if (this == other) {
        *result = PR_TRUE;
        return NS_OK;
    }
    if (!other)
        return NS_OK;
    nsresult rv;
    nsCOMPtr<nsICertificatePrincipal> otherCertificate = 
        do_QueryInterface(other, &rv);
    if (NS_FAILED(rv))
        return NS_OK;
    //-- Compare cert ID's
    char* otherID;
    rv = otherCertificate->GetCertificateID(&otherID);
    if (NS_FAILED(rv)) 
    {
        PR_FREEIF(otherID);
        return rv;
    }
    *result = (PL_strcmp(mCertificateID, otherID) == 0);
    PR_FREEIF(otherID);
    return NS_OK;
}

NS_IMETHODIMP
nsCertificatePrincipal::HashValue(PRUint32 *result)
{
    char* str;
    if (NS_FAILED(ToString(&str)) || !str) return NS_ERROR_FAILURE;
    *result = nsCRT::HashCode(str, nsnull);
    nsCRT::free(str);
    return NS_OK;
}

//////////////////////////////////////////
// Methods implementing nsISerializable //
//////////////////////////////////////////

NS_IMETHODIMP
nsCertificatePrincipal::Read(nsIObjectInputStream* aStream)
{
    nsresult rv;

    rv = nsBasePrincipal::Read(aStream);
    if (NS_FAILED(rv)) return rv;

    rv = aStream->ReadStringZ(&mCertificateID);
    if (NS_FAILED(rv)) return rv;

    PRBool hasCommonName;
    rv = aStream->ReadBoolean(&hasCommonName);
    if (NS_SUCCEEDED(rv) && hasCommonName)
        rv = aStream->ReadStringZ(&mCommonName);
    if (NS_FAILED(rv)) return rv;

    return NS_OK;
}

NS_IMETHODIMP
nsCertificatePrincipal::Write(nsIObjectOutputStream* aStream)
{
    nsresult rv;

    rv = nsBasePrincipal::Write(aStream);
    if (NS_FAILED(rv)) return rv;

    rv = aStream->WriteStringZ(mCertificateID);
    if (NS_FAILED(rv)) return rv;

    PRBool hasCommonName = (mCommonName != nsnull);
    rv = aStream->WriteBoolean(hasCommonName);
    if (NS_SUCCEEDED(rv) && hasCommonName)
        rv = aStream->WriteStringZ(mCommonName);
    if (NS_FAILED(rv)) return rv;

    return NS_OK;
}

NS_IMETHODIMP
nsCertificatePrincipal::GetCID(nsCID *aResult)
{
    nsCID myCID = NS_CERTIFICATEPRINCIPAL_CID;

    *aResult = myCID;
    return NS_OK;
}

/////////////////////////////////////////////
// Constructor, Destructor, initialization //
/////////////////////////////////////////////
nsresult
nsCertificatePrincipal::InitFromPersistent(const char* aPrefName, const char*  aCertID, 
                                           const char* aGrantedList, const char* aDeniedList)
{
    if (NS_FAILED(Init(aCertID))) 
        return NS_ERROR_FAILURE;
    
    return nsBasePrincipal::InitFromPersistent(aPrefName, aCertID, 
                                               aGrantedList, aDeniedList);
}

NS_IMETHODIMP
nsCertificatePrincipal::Init(const char* aCertificateID)
{
    mCertificateID = nsCRT::strdup(aCertificateID);
    if (!mCertificateID) return NS_ERROR_OUT_OF_MEMORY;
    return NS_OK;
}

nsCertificatePrincipal::nsCertificatePrincipal() : mCertificateID(nsnull),
                                                   mCommonName(nsnull)
{
    NS_INIT_ISUPPORTS();
}

nsCertificatePrincipal::~nsCertificatePrincipal()
{
    PR_FREEIF(mCertificateID);
    PR_FREEIF(mCommonName);
}
