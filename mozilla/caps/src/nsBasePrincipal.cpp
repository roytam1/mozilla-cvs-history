/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1999-2000 Netscape Communications Corporation.  All Rights
 * Reserved.
 * 
 * Contributor(s):
 * Norris Boyd
 */

#include "nsBasePrincipal.h"
#include "nsString.h"
#include "plstr.h"
#include "nsIPref.h"

//////////////////////////


nsBasePrincipal::nsBasePrincipal()
    : mCapabilities(nsnull), mPrefName(nsnull)
{
}

PR_STATIC_CALLBACK(PRBool)
deleteElement(void* aElement, void *aData)
{
    nsHashtable *ht = (nsHashtable *) aElement;
    delete ht;
    return PR_TRUE;
}

nsBasePrincipal::~nsBasePrincipal(void)
{
    mAnnotations.EnumerateForwards(deleteElement, nsnull);
    delete mCapabilities;
    if (mPrefName)
        Recycle(mPrefName);
}

NS_IMETHODIMP
nsBasePrincipal::GetJSPrincipals(JSPrincipals **jsprin)
{
    if (mJSPrincipals.nsIPrincipalPtr == nsnull) {
        mJSPrincipals.nsIPrincipalPtr = this;
        // No need for a ADDREF since it is a self-reference
    }
    *jsprin = &mJSPrincipals;
    JSPRINCIPALS_HOLD(cx, *jsprin);
    return NS_OK;
}

NS_IMETHODIMP 
nsBasePrincipal::CanEnableCapability(const char *capability, PRInt16 *result)
{
    if (!mCapabilities) {
        *result = nsIPrincipal::ENABLE_UNKNOWN;
        return NS_OK;
    }
    nsStringKey key(capability);
    *result = (PRInt16)(PRInt32)mCapabilities->Get(&key);
    if (!*result)
        *result = nsIPrincipal::ENABLE_UNKNOWN;
    return NS_OK;
}

NS_IMETHODIMP 
nsBasePrincipal::SetCanEnableCapability(const char *capability, 
                                        PRInt16 canEnable)
{
    if (!mCapabilities) {
        mCapabilities = new nsHashtable(7);
        if (!mCapabilities)
            return NS_ERROR_OUT_OF_MEMORY;
    }
    nsStringKey key(capability);
    mCapabilities->Put(&key, (void *) canEnable);
    return NS_OK;
}

NS_IMETHODIMP 
nsBasePrincipal::IsCapabilityEnabled(const char *capability, void *annotation, 
                                     PRBool *result)
{
    *result = PR_FALSE;
    nsHashtable *ht = (nsHashtable *) annotation;
    if (ht) {
        nsStringKey key(capability);
        *result = (ht->Get(&key) == (void *) AnnotationEnabled);
    } 
    return NS_OK;
}

NS_IMETHODIMP 
nsBasePrincipal::EnableCapability(const char *capability, void **annotation) 
{
    return SetCapability(capability, annotation, AnnotationEnabled);
}

NS_IMETHODIMP 
nsBasePrincipal::DisableCapability(const char *capability, void **annotation) 
{
    return SetCapability(capability, annotation, AnnotationDisabled);
}

NS_IMETHODIMP 
nsBasePrincipal::RevertCapability(const char *capability, void **annotation) 
{
    if (*annotation) {
        nsHashtable *ht = (nsHashtable *) *annotation;
        nsStringKey key(capability);
        ht->Remove(&key);
    }
    return NS_OK;
}

NS_IMETHODIMP
nsBasePrincipal::SetCapability(const char *capability, void **annotation,
                               AnnotationValue value) 
{
    if (*annotation == nsnull) {
        *annotation = new nsHashtable(5);
        if (!*annotation)
            return NS_ERROR_OUT_OF_MEMORY;
        // This object owns its annotations. Save them so we can release
        // them when we destroy this object.
        mAnnotations.AppendElement(*annotation);
    }
    nsHashtable *ht = (nsHashtable *) *annotation;
    nsStringKey key(capability);
    ht->Put(&key, (void *) value);
    return NS_OK;
}

int nsBasePrincipal::mCapabilitiesOrdinal = 0;

nsresult
nsBasePrincipal::InitFromPersistent(const char *name, const char* data)
{
    // Parses capabilities strings of the form 
    // "Capability=value ..."
    // ie. "UniversalBrowserRead=0 UniversalBrowserWrite=1"
    // where value is from 0 to 3 as defined in nsIPrincipal.idl
    if (mCapabilities)
        mCapabilities->Reset();

    nsCAutoString nameString(name);
    mPrefName = nameString.ToNewCString();

    static const char *prefix = ".X";
    const char *p = PL_strstr(name, prefix);
    if (p) {
        int n = atoi(p + sizeof(prefix)-1);
        if (mCapabilitiesOrdinal <= n)
            mCapabilitiesOrdinal = n+1;
    }
    
    for (;;)
    {
        char* wordEnd = PL_strchr(data, '=');
        if (wordEnd == nsnull)
            break;
        *wordEnd = '\0';
        const char* cap = data;
        data = wordEnd+1; // data is now pointing at the numeric value
        PRInt16 value = (PRInt16)(*data) - (PRInt16)'0';
        nsresult rv = SetCanEnableCapability(cap, value);
        if (NS_FAILED(rv)) return rv;
        if (data[1] == '\0') // End of the data
            break;
        else
            data += 2; // data is now at the beginning of the next capability string
    }
    return NS_OK;
}

PR_STATIC_CALLBACK(PRBool)
AppendCapability(nsHashKey *aKey, void *aData, void *aStr)
{
    char value = (char)((unsigned int)aData) + '0';
    nsCString *capStr = (nsCString*) aStr;    
    capStr->Append(' ');
    capStr->Append(((nsStringKey *) aKey)->GetString());
    capStr->Append('=');
    capStr->Append(value);
    return PR_TRUE;
}   
        

NS_IMETHODIMP
nsBasePrincipal::WriteToPrefs(nsIPref *aPref)
{
    char *streamableForm;
    if (NS_FAILED(ToStreamableForm(&streamableForm))) 
        return NS_ERROR_FAILURE;
    if (!mPrefName) {
        nsCAutoString s("security.principal.X");
        s += mCapabilitiesOrdinal++;
        mPrefName = s.ToNewCString();
    }
    nsresult rv = aPref->SetCharPref(mPrefName, streamableForm);
    Recycle(streamableForm);
    return rv;
}

NS_IMETHODIMP
nsBasePrincipal::ToStreamableForm(char **aResult)
{
    if (NS_FAILED(ToString(aResult)))
        return NS_ERROR_FAILURE;
    if (mCapabilities) {
        nsCAutoString result(*aResult);
        mCapabilities->Enumerate(AppendCapability, (void*)&result);
        Recycle(*aResult);
        *aResult = result.ToNewCString();
    }
    return *aResult ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}


