/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: NPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is 
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998-2000
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 * Mitch Stoltz <mstoltz@netscape.com>
 *
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the NPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

/*describes principals which combine one or more principals*/
#include "nsAggregatePrincipal.h"
#include "nsIURI.h"

static NS_DEFINE_IID(kIAggregatePrincipalIID, NS_IAGGREGATEPRINCIPAL_IID);

NS_IMPL_QUERY_INTERFACE5_CI(nsAggregatePrincipal, nsIAggregatePrincipal, 
                            nsICertificatePrincipal, nsICodebasePrincipal,
                            nsIPrincipal, nsISerializable)
NS_IMPL_CI_INTERFACE_GETTER5(nsAggregatePrincipal, nsIAggregatePrincipal, 
                             nsICertificatePrincipal, nsICodebasePrincipal,
                             nsIPrincipal, nsISerializable)

NSBASEPRINCIPALS_ADDREF(nsAggregatePrincipal);
NSBASEPRINCIPALS_RELEASE(nsAggregatePrincipal);

//////////////////////////////////////////////////
// Methods implementing nsICertificatePrincipal //
//////////////////////////////////////////////////
NS_IMETHODIMP 
nsAggregatePrincipal::GetCertificateID(char** aCertificateID)
{
    if (!mCertificate)
    {
        *aCertificateID = nsnull;
        return NS_OK;
    }

    nsCOMPtr<nsICertificatePrincipal> certificate = do_QueryInterface(mCertificate);
    return certificate->GetCertificateID(aCertificateID);
}

NS_IMETHODIMP 
nsAggregatePrincipal::GetCommonName(char** aCommonName)
{
    if (!mCertificate)
    {
        *aCommonName = nsnull;
        return NS_OK;
    }

    nsCOMPtr<nsICertificatePrincipal> certificate = do_QueryInterface(mCertificate);
    return certificate->GetCommonName(aCommonName);
}

NS_IMETHODIMP 
nsAggregatePrincipal::SetCommonName(const char* aCommonName)
{
    if (!mCertificate)
        return NS_ERROR_FAILURE;

    nsCOMPtr<nsICertificatePrincipal> certificate = do_QueryInterface(mCertificate);
    return certificate->SetCommonName(aCommonName);
}

///////////////////////////////////////////////
// Methods implementing nsICodebasePrincipal //
///////////////////////////////////////////////
NS_IMETHODIMP 
nsAggregatePrincipal::GetURI(nsIURI** aURI)
{
    if (!mCodebase)
    {
        *aURI = nsnull;
        return NS_ERROR_FAILURE;
    }

    nsCOMPtr<nsICodebasePrincipal> codebase = do_QueryInterface(mCodebase);
    return codebase->GetURI(aURI);
}

NS_IMETHODIMP 
nsAggregatePrincipal::GetOrigin(char** aOrigin)
{
    if (!mCodebase)
    {
        *aOrigin = nsnull;
        return NS_ERROR_FAILURE;
    }

    nsCOMPtr<nsICodebasePrincipal> codebase = do_QueryInterface(mCodebase);
    return codebase->GetOrigin(aOrigin);
}

NS_IMETHODIMP 
nsAggregatePrincipal::GetSpec(char** aSpec)
{
    if (!mCodebase)
    {
        *aSpec = nsnull;
        return NS_ERROR_FAILURE;
    }

    nsCOMPtr<nsICodebasePrincipal> codebase = do_QueryInterface(mCodebase);
    return codebase->GetSpec(aSpec);
}

////////////////////////////////////////////////
// Methods implementing nsIAggregatePrincipal //
////////////////////////////////////////////////
NS_IMETHODIMP 
nsAggregatePrincipal::GetCertificate(nsIPrincipal** result)
{
    *result = mCertificate;
    NS_IF_ADDREF(*result);
    return NS_OK;
}

NS_IMETHODIMP 
nsAggregatePrincipal::GetCodebase(nsIPrincipal** result)
{
    *result = mCodebase;
    NS_IF_ADDREF(*result);
    return NS_OK;
}

NS_IMETHODIMP 
nsAggregatePrincipal::SetCertificate(nsIPrincipal* aCertificate)
{
    nsresult rv;
    //-- Make sure this really is a certificate principal
    if (aCertificate)
    {
        nsCOMPtr<nsICertificatePrincipal> tempCertificate = 
            do_QueryInterface(aCertificate, &rv);
        if (NS_FAILED(rv))
            return NS_ERROR_FAILURE;
    }

    //-- If aCertificate is an aggregate, get its underlying certificate
    nsCOMPtr<nsIAggregatePrincipal> agg = 
        do_QueryInterface(aCertificate, &rv);
    if (NS_SUCCEEDED(rv))
    {
        nsCOMPtr<nsIPrincipal> underlying;
        rv = agg->GetCertificate(getter_AddRefs(underlying));
        if (NS_FAILED(rv)) return NS_ERROR_FAILURE;
        mCertificate = underlying.get();
    }
    else
        mCertificate = aCertificate;
    return NS_OK;
}

NS_IMETHODIMP 
nsAggregatePrincipal::SetCodebase(nsIPrincipal* aCodebase)
{
    nsresult rv;
    nsCOMPtr<nsIPrincipal> newCodebase(aCodebase);

    //-- If newCodebase is an aggregate, get its underlying codebase
    nsCOMPtr<nsIAggregatePrincipal> agg = 
        do_QueryInterface(newCodebase, &rv);
    if (NS_SUCCEEDED(rv))
    {
        rv = agg->GetCodebase(getter_AddRefs(newCodebase));
        if (NS_FAILED(rv)) return NS_ERROR_FAILURE;
    }
    else
    { //-- Make sure this really is a codebase principal
        nsCOMPtr<nsICodebasePrincipal> tempCodebase = 
            do_QueryInterface(newCodebase, &rv);
        if (NS_FAILED(rv))
            return NS_ERROR_FAILURE;
    }

    mCodebase = newCodebase;

    //-- If this is the first codebase set, remember it.
    //   If not, remember that the codebase was explicitly set
    if (!mOriginalCodebase)
        mOriginalCodebase = newCodebase;
    else
        mCodebaseWasChanged = PR_TRUE;

    return NS_OK;
}

NS_IMETHODIMP 
nsAggregatePrincipal::GetOriginalCodebase(nsIPrincipal** aOriginalCodebase)
{
    NS_ENSURE_ARG_POINTER(aOriginalCodebase);

    *aOriginalCodebase = mOriginalCodebase;
    NS_IF_ADDREF(*aOriginalCodebase);

    return NS_OK;
}

NS_IMETHODIMP
nsAggregatePrincipal::GetPrimaryChild(nsIPrincipal** aPrimaryChild)
{
    //-- If a certificate is present, then that's the PrimaryChild principal.
    //   Otherwise we use the codebase.
    if (mCertificate)
        *aPrimaryChild = mCertificate.get();
    else if (mCodebase)
        *aPrimaryChild = mCodebase.get();
    else
    {
        *aPrimaryChild = nsnull;
        return NS_ERROR_FAILURE;
    }

    NS_IF_ADDREF(*aPrimaryChild);
    return NS_OK;
}

NS_IMETHODIMP 
nsAggregatePrincipal::Intersect(nsIPrincipal* other)
{
    NS_ASSERTION(mCodebase, "Principal without codebase");

    if (mCertificate)
    {
        PRBool sameCert = PR_FALSE;
        if (NS_FAILED(mCertificate->Equals(other, &sameCert)))
            return NS_ERROR_FAILURE;
        if (!sameCert)
            SetCertificate(nsnull);
    }
    return NS_OK;
}

NS_IMETHODIMP 
nsAggregatePrincipal::WasCodebaseChanged(PRBool* changed)
{
    *changed = mCodebaseWasChanged;
    return NS_OK;
}

///////////////////////////////////////
// Methods implementing nsIPrincipal //
///////////////////////////////////////
NS_IMETHODIMP 
nsAggregatePrincipal::ToString(char **result)
{
    nsCOMPtr<nsIPrincipal> PrimaryChild;
    if (NS_FAILED(GetPrimaryChild(getter_AddRefs(PrimaryChild))))
        return NS_ERROR_FAILURE;
    return PrimaryChild->ToString(result);
}

NS_IMETHODIMP 
nsAggregatePrincipal::ToUserVisibleString(char **result)
{
    nsCOMPtr<nsIPrincipal> PrimaryChild;
    if (NS_FAILED(GetPrimaryChild(getter_AddRefs(PrimaryChild))))
        return NS_ERROR_FAILURE;
    return PrimaryChild->ToUserVisibleString(result);
}

NS_IMETHODIMP
nsAggregatePrincipal::Equals(nsIPrincipal * other, PRBool * result)
{
    *result = PR_FALSE;
    if (this == other) {
        *result = PR_TRUE;
        return NS_OK;
    }
    if (!other)
        return NS_OK;
    
    nsresult rv;
    nsCOMPtr<nsIAggregatePrincipal> otherAgg = 
        do_QueryInterface(other, &rv);
    if (NS_FAILED(rv))
        return NS_OK;
       //-- Two aggregates are equal if both codebase and certificate are equal
    PRBool certEqual = PR_TRUE;
    if (mCertificate)
    {
        rv = mCertificate->Equals(other, &certEqual);
        if(NS_FAILED(rv)) return rv;
    }
    PRBool cbEqual = PR_TRUE;
    if (mCodebase)
    {
        rv = mCodebase->Equals(other, &cbEqual);
        if(NS_FAILED(rv)) return rv;
    }
    if (mCertificate || mCodebase) // At least one must be present
        *result = certEqual && cbEqual;
    return NS_OK;
}

NS_IMETHODIMP
nsAggregatePrincipal::HashValue(PRUint32 *result)
{
    nsCOMPtr<nsIPrincipal> PrimaryChild;
    if (NS_FAILED(GetPrimaryChild(getter_AddRefs(PrimaryChild))))
        return NS_ERROR_FAILURE;
    return PrimaryChild->HashValue(result);
}

NS_IMETHODIMP 
nsAggregatePrincipal::CanEnableCapability(const char *capability,
                                          PRInt16 *result)
{
    nsCOMPtr<nsIPrincipal> PrimaryChild;
    if (NS_FAILED(GetPrimaryChild(getter_AddRefs(PrimaryChild))))
        return NS_ERROR_FAILURE;
    return PrimaryChild->CanEnableCapability(capability, result);
}

NS_IMETHODIMP
nsAggregatePrincipal::SetCanEnableCapability(const char *capability, 
                                             PRInt16 canEnable)
{
    nsCOMPtr<nsIPrincipal> PrimaryChild;
    if (NS_FAILED(GetPrimaryChild(getter_AddRefs(PrimaryChild))))
        return NS_ERROR_FAILURE;
    return PrimaryChild->SetCanEnableCapability(capability, canEnable);
}   

NS_IMETHODIMP
nsAggregatePrincipal::IsCapabilityEnabled(const char *capability, void *annotation, 
                                          PRBool *result)
{
    nsCOMPtr<nsIPrincipal> PrimaryChild;
    if (NS_FAILED(GetPrimaryChild(getter_AddRefs(PrimaryChild))))
        return NS_ERROR_FAILURE;
    return PrimaryChild->IsCapabilityEnabled(capability, annotation, result);
}

NS_IMETHODIMP
nsAggregatePrincipal::EnableCapability(const char *capability, void **annotation)
{
    nsCOMPtr<nsIPrincipal> PrimaryChild;
    if (NS_FAILED(GetPrimaryChild(getter_AddRefs(PrimaryChild))))
        return NS_ERROR_FAILURE;
    return PrimaryChild->EnableCapability(capability, annotation);
}

NS_IMETHODIMP
nsAggregatePrincipal::RevertCapability(const char *capability, void **annotation)
{
    nsCOMPtr<nsIPrincipal> PrimaryChild;
    if (NS_FAILED(GetPrimaryChild(getter_AddRefs(PrimaryChild))))
        return NS_ERROR_FAILURE;
    return PrimaryChild->RevertCapability(capability, annotation);
}

NS_IMETHODIMP
nsAggregatePrincipal::DisableCapability(const char *capability, void **annotation)
{
    nsCOMPtr<nsIPrincipal> PrimaryChild;
    if (NS_FAILED(GetPrimaryChild(getter_AddRefs(PrimaryChild))))
        return NS_ERROR_FAILURE;
    return PrimaryChild->DisableCapability(capability, annotation);
}

NS_IMETHODIMP
nsAggregatePrincipal::GetPreferences(char** aPrefName, char** aID, 
                                     char** aGrantedList, char** aDeniedList)
{
    nsCOMPtr<nsIPrincipal> PrimaryChild;
    if (NS_FAILED(GetPrimaryChild(getter_AddRefs(PrimaryChild))))
        return NS_ERROR_FAILURE;
    return PrimaryChild->GetPreferences(aPrefName, aID, 
                                          aGrantedList, aDeniedList);
}

//////////////////////////////////////////
// Methods implementing nsISerializable //
//////////////////////////////////////////

NS_IMETHODIMP
nsAggregatePrincipal::Read(nsIObjectInputStream* aStream)
{
    nsresult rv;

    rv = nsBasePrincipal::Read(aStream);
    if (NS_FAILED(rv)) return rv;

    rv = NS_ReadOptionalObject(aStream, PR_TRUE, getter_AddRefs(mCertificate));
    if (NS_FAILED(rv)) return rv;

    rv = NS_ReadOptionalObject(aStream, PR_TRUE, getter_AddRefs(mCodebase));
    if (NS_FAILED(rv)) return rv;

    return NS_OK;
}

NS_IMETHODIMP
nsAggregatePrincipal::Write(nsIObjectOutputStream* aStream)
{
    nsresult rv;

    rv = nsBasePrincipal::Write(aStream);
    if (NS_FAILED(rv)) return rv;

    rv = NS_WriteOptionalObject(aStream, mCertificate, PR_TRUE);
    if (NS_FAILED(rv)) return rv;

    rv = NS_WriteOptionalCompoundObject(aStream, mCodebase, NS_GET_IID(nsIPrincipal), PR_TRUE);
    if (NS_FAILED(rv)) return rv;

    return NS_OK;
}

/////////////////////////////////////////////
// Constructor, Destructor, initialization //
/////////////////////////////////////////////

nsAggregatePrincipal::nsAggregatePrincipal() : mCodebaseWasChanged(PR_FALSE)
{
    NS_INIT_ISUPPORTS();
}

nsAggregatePrincipal::~nsAggregatePrincipal()
{
}
