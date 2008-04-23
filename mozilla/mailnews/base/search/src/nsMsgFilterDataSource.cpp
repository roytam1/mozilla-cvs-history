/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Alec Flett <alecf@netscape.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either of the GNU General Public License Version 2 or later (the "GPL"),
 * or the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "nsMsgFilterDataSource.h"
#include "nsMsgRDFUtils.h"
#include "nsEnumeratorUtils.h"

#include "nsIMsgFilter.h"
#include "nsIMsgFilterList.h"

#define NC_RDF_ENABLED NC_NAMESPACE_URI "Enabled"

nsrefcnt nsMsgFilterDataSource::mGlobalRefCount = 0;
nsCOMPtr<nsIRDFResource> nsMsgFilterDataSource::kNC_Child;
nsCOMPtr<nsIRDFResource> nsMsgFilterDataSource::kNC_Name;
nsCOMPtr<nsIRDFResource> nsMsgFilterDataSource::kNC_Enabled;

nsCOMPtr<nsIRDFLiteral> nsMsgFilterDataSource::kTrueLiteral;

nsCOMPtr<nsISupportsArray> nsMsgFilterDataSource::mFilterListArcsOut;
nsCOMPtr<nsISupportsArray> nsMsgFilterDataSource::mFilterArcsOut;



nsMsgFilterDataSource::nsMsgFilterDataSource()
{
    if (mGlobalRefCount++ == 0)
        initGlobalObjects(getRDFService());
}

nsMsgFilterDataSource::~nsMsgFilterDataSource()
{
    mGlobalRefCount--;
    if (mGlobalRefCount == 0)
        cleanupGlobalObjects();
}

nsresult
nsMsgFilterDataSource::cleanupGlobalObjects()
{
    mFilterListArcsOut = nsnull;
    mFilterArcsOut = nsnull;
    kNC_Child = nsnull;
    kNC_Name = nsnull;
    kNC_Enabled = nsnull;
    kTrueLiteral = nsnull;
    return NS_OK;
}

nsresult
nsMsgFilterDataSource::initGlobalObjects(nsIRDFService *rdf)
{
    rdf->GetLiteral(NS_LITERAL_STRING("true").get(),
                    getter_AddRefs(kTrueLiteral));
    
    rdf->GetResource(NS_LITERAL_CSTRING(NC_RDF_CHILD), getter_AddRefs(kNC_Child));
    rdf->GetResource(NS_LITERAL_CSTRING(NC_RDF_NAME), getter_AddRefs(kNC_Name));
    rdf->GetResource(NS_LITERAL_CSTRING(NC_RDF_ENABLED), getter_AddRefs(kNC_Enabled));

    NS_NewISupportsArray(getter_AddRefs(mFilterListArcsOut));
    mFilterListArcsOut->AppendElement(kNC_Child);
  
    NS_NewISupportsArray(getter_AddRefs(mFilterArcsOut));
    mFilterArcsOut->AppendElement(kNC_Name);
    mFilterArcsOut->AppendElement(kNC_Enabled);
  
    return NS_OK;
}

NS_IMETHODIMP
nsMsgFilterDataSource::GetTargets(nsIRDFResource *aSource,
                                  nsIRDFResource *aProperty,
                                  PRBool aTruthValue,
                                  nsISimpleEnumerator **aResult)
{
    nsresult rv;
    
    nsCOMPtr<nsISupportsArray> resourceList;
    rv = NS_NewISupportsArray(getter_AddRefs(resourceList));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsISupports> filterDelegate;
    
    // first see if it's a filter list
    rv = aSource->GetDelegate("filter", NS_GET_IID(nsISupports),
                             (void **)getter_AddRefs(filterDelegate));

    if (NS_FAILED(rv)) {
        // no filter delegate
        return NS_NewSingletonEnumerator(aResult, nsnull);
    }
        

    //
    // nsIMsgFilterList
    //
    nsCOMPtr<nsIMsgFilterList> filterList =
        do_QueryInterface(filterDelegate, &rv);
    
    if (NS_SUCCEEDED(rv)) {
        rv = getFilterListTargets(filterList, aSource, aProperty,
                                  aTruthValue, resourceList);
    }
    else {

        //
        // nsIMsgFilter
        //
        nsCOMPtr<nsIMsgFilter> filter =
            do_QueryInterface(filterDelegate, &rv);

        if (NS_SUCCEEDED(rv)) {
            // filters do not have multiple targets, right?
            // do we have to call GetTarget and return the result?
        }

        else {
            NS_WARNING("ArcLabelsOut(): unknown filter delegate!\n");
        }
    }
    
    return NS_NewArrayEnumerator(aResult, resourceList);
}

NS_IMETHODIMP
nsMsgFilterDataSource::GetTarget(nsIRDFResource *aSource,
                                 nsIRDFResource *aProperty,
                                 PRBool aTruthValue,
                                 nsIRDFNode **aResult)
{
    nsresult rv;
    *aResult = nsnull;

    
    nsCOMPtr<nsISupports> filterDelegate;
    aSource->GetDelegate("filter", NS_GET_IID(nsISupports),
                         (void **)getter_AddRefs(filterDelegate));

    //
    // nsIMsgFilterList
    //
    nsCOMPtr<nsIMsgFilterList> filterList =
        do_QueryInterface(filterDelegate, &rv);

    if (NS_SUCCEEDED(rv))
        rv = getFilterListTarget(filterList, aProperty, aTruthValue, aResult);
    else {

        //
        // nsIMsgFilter
        //
        nsCOMPtr<nsIMsgFilter> filter =
            do_QueryInterface(filterDelegate, &rv);
        if (NS_SUCCEEDED(rv))
            rv = getFilterTarget(filter, aProperty, aTruthValue, aResult);
        
        else {
            NS_WARNING("GetTarget(): unknown filter delegate!\n");
        }
    }

    if (*aResult)
        return NS_OK;

    return NS_RDF_NO_VALUE;
}


NS_IMETHODIMP
nsMsgFilterDataSource::ArcLabelsOut(nsIRDFResource *aSource,
                                    nsISimpleEnumerator **aResult)
{
    nsresult rv;
    nsCOMPtr<nsISupportsArray> arcs;

    nsCOMPtr<nsISupports> filterDelegate;
    
    rv = aSource->GetDelegate("filter", NS_GET_IID(nsISupports),
                              (void **)getter_AddRefs(filterDelegate));
    
    if (NS_FAILED(rv)) return NS_RDF_NO_VALUE;

    //
    // nsIMsgFilterList
    //
    nsCOMPtr<nsIMsgFilterList> filterList =
        do_QueryInterface(filterDelegate, &rv);
    
    if (NS_SUCCEEDED(rv)) {
        arcs = mFilterListArcsOut;
    
    } else {

        //
        // nsIMsgFilter
        //
        nsCOMPtr<nsIMsgFilter> filter =
            do_QueryInterface(filterDelegate, &rv);
        
        if (NS_SUCCEEDED(rv))
            arcs = mFilterArcsOut;
        
        else {
            NS_WARNING("GetTargets(): unknown filter delegate!\n");

        }
    }

    if (!arcs) {
        *aResult = nsnull;
        return NS_RDF_NO_VALUE;
    }

    return NS_NewArrayEnumerator(aResult, arcs);
}

// takes a base resource, like mailbox://username@host/folder and returns
// the array that corresponds to all the filters.
// it does this quickly because it knows the sub-filters will be in the form
//
// mailbox://username@host/folder;filterName=abc
// mailbox://username@host/folder;filterName=cde
// mailbox://username@host/folder;filterName=efg
// mailbox://username@host/folder;filterName=ghi
//
// and so forth
nsresult
nsMsgFilterDataSource::getFilterListTargets(nsIMsgFilterList *aFilterList,
                                            nsIRDFResource *aSource,
                                            nsIRDFResource *aProperty,
                                            PRBool aTruthValue,
                                            nsISupportsArray *aResult)
{
    nsresult rv;

    // get the URI of the source resource, say mailbox://username@host/folder
    // from there we'll append ";filter", and then enumerate the
    // filters to get all the resources like
    // mailbox://username@host/folder;filterName=abcd

    // a better way to do this might be to ask the filter
    // what folder it's in. Then we wouldn't need aSource
    const char* filterListUri;
    aSource->GetValueConst(&filterListUri);

    nsCAutoString filterUri(filterListUri);
    filterUri.Append(";filterName=");

    // we'll use the length of this base string to truncate the string later
    PRInt32 baseFilterUriLength = filterUri.Length();

    // now start looping through the filters
    // we're not actually touching any filters here, just creating
    // resources that correspond to them.
    PRUint32 filterCount;
    rv = aFilterList->GetFilterCount(&filterCount);
    if (NS_FAILED(rv)) return rv;

    PRUint32 i;
    for (i=0; i<filterCount; i++) {
        nsCOMPtr<nsIMsgFilter> filter;
        rv = aFilterList->GetFilterAt(i, getter_AddRefs(filter));
        if (NS_FAILED(rv)) continue;

        PRBool isTemporary;
        rv = filter->GetTemporary(&isTemporary);
        NS_ASSERTION(NS_SUCCEEDED(rv), "GetTemporary() failed");
        if (isTemporary)
          continue;

        nsString filterName;
        rv = filter->GetFilterName(filterName);
        if (NS_FAILED(rv)) return rv;

        filterUri.Append(NS_ConvertUTF16toUTF8(filterName));
        
        nsCOMPtr<nsIRDFResource> filterResource;
        rv = getRDFService()->GetResource(filterUri,
                              getter_AddRefs(filterResource));
        if (NS_SUCCEEDED(rv))
            aResult->AppendElement(filterResource);

        // now reduce the URI back to the base uri
        filterUri.SetLength(baseFilterUriLength);
    }
    
    return NS_OK;
}

nsresult
nsMsgFilterDataSource::getFilterListTarget(nsIMsgFilterList *aFilterList,
                                           nsIRDFResource *aProperty,
                                           PRBool aTruthValue,
                                           nsIRDFNode **aResult)
{

    // here we probably need to answer to the #child property
    // so that RDF realizes this is a container
    return NS_ERROR_NOT_IMPLEMENTED;
}


nsresult
nsMsgFilterDataSource::getFilterTarget(nsIMsgFilter *aFilter,
                                       nsIRDFResource *aProperty,
                                       PRBool aTruthValue,
                                       nsIRDFNode **aResult)
{

    if (aProperty == kNC_Name.get()) {
        nsString filterName;
        aFilter->GetFilterName(filterName);
        return createNode(filterName.get(), aResult,
                          getRDFService());
        
    } else if (aProperty == kNC_Enabled.get()) {
        PRBool enabled;
        aFilter->GetEnabled(&enabled);
        if (enabled) {
            *aResult = kTrueLiteral;
            NS_ADDREF(*aResult);
            return NS_OK;
        }
    }
    
    return NS_RDF_NO_VALUE;
}


