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
 * Copyright (C) 1999 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s):
 * Alec Flett <alecf@netscape.com>
 */

#include "nsMsgSearchDataSource.h"
#include "nsIRDFService.h"
#include "nsMsgRDFUtils.h"

#include "nsIMessage.h"
#include "nsIMsgHdr.h"
#include "nsIMsgSearchSession.h"
#include "nsEnumeratorUtils.h"

#ifdef DEBUG_alecf
#include "nsXPIDLString.h"
#endif

nsCOMPtr<nsIRDFResource> nsMsgSearchDataSource::kNC_MessageChild;
nsrefcnt nsMsgSearchDataSource::gInstanceCount = 0;
PRUint32 nsMsgSearchDataSource::gCurrentURINum = 0;


nsMsgSearchDataSource::nsMsgSearchDataSource()
{
    NS_INIT_REFCNT();

}

nsresult
nsMsgSearchDataSource::Init()
{
    if (gInstanceCount++ == 0) {

        getRDFService()->GetResource(NC_RDF_MESSAGECHILD, getter_AddRefs(kNC_MessageChild));
    }

    mURINum = gCurrentURINum++;
    
    nsCAutoString uri("mailsearch:#");
    uri.AppendInt(mURINum);
    getRDFService()->GetResource(uri.GetBuffer(), getter_AddRefs(mSearchRoot));

    NS_NewISupportsArray(getter_AddRefs(mSearchResults));
    return NS_OK;
}

nsMsgSearchDataSource::~nsMsgSearchDataSource()
{
    if (--gInstanceCount == 0) {
        kNC_MessageChild = nsnull;
    }
}

NS_IMPL_ISUPPORTS2(nsMsgSearchDataSource,
                   nsIRDFDataSource,
                   nsIMsgSearchNotify)

    NS_IMETHODIMP
nsMsgSearchDataSource::OnSearchHit(nsIMsgDBHdr* aMsgHdr, nsIMsgFolder *folder)
{
    nsresult rv;

#ifdef DEBUG
    printf("nsMsgSearchDataSource::OnSearchHit!!\n");
#endif
    
    nsCOMPtr<nsIMessage> message;
    folder->CreateMessageFromMsgDBHdr(aMsgHdr, getter_AddRefs(message));
    
    // this probably wont work. Need to convert nsMsgDBHdr -> nsMessage
    // probably through a URI or something
    nsCOMPtr<nsIRDFResource> messageResource =
      do_QueryInterface(message, &rv);

    // remember that we got this, so we can later answer HasAssertion()
    mSearchResults->AppendElement(NS_STATIC_CAST(nsISupports*,messageResource.get()));
    
    // should probably try to cache this with an in-memory datasource
    // or something
    NotifyObservers(mSearchRoot, kNC_MessageChild, messageResource, PR_TRUE, PR_FALSE);
    return NS_OK;
}

NS_IMETHODIMP
nsMsgSearchDataSource::OnSearchDone(nsresult status)
{
  return NS_OK;
}


// for now also acts as a way of resetting the search datasource
NS_IMETHODIMP
nsMsgSearchDataSource::OnNewSearch()
{
    mSearchResults->Clear();
    return NS_OK;
}


/* readonly attribute string URI; */
NS_IMETHODIMP
nsMsgSearchDataSource::GetURI(char * *aURI)
{
    return mSearchRoot->GetValue(aURI);
}


/* nsISimpleEnumerator GetTargets (in nsIRDFResource aSource, in nsIRDFResource aProperty, in boolean aTruthValue); */
NS_IMETHODIMP
nsMsgSearchDataSource::GetTargets(nsIRDFResource *aSource,
                                  nsIRDFResource *aProperty,
                                  PRBool aTruthValue,
                                  nsISimpleEnumerator **aResult)
{
    return NS_NewArrayEnumerator(aResult, mSearchResults);
}


/* boolean HasAssertion (in nsIRDFResource aSource, in nsIRDFResource aProperty, in nsIRDFNode aTarget, in boolean aTruthValue); */
NS_IMETHODIMP
nsMsgSearchDataSource::HasAssertion(nsIRDFResource *aSource,
                                    nsIRDFResource *aProperty,
                                    nsIRDFNode *aTarget,
                                    PRBool aTruthValue,
                                    PRBool *aResult)
{
    NS_ENSURE_ARG_POINTER(aResult);

    if (!aTruthValue) return NS_RDF_NO_VALUE;
    nsresult rv;
    
#ifdef DEBUG_alecf
    nsXPIDLCString sourceVal;
    aSource->GetValueConst(getter_Shares(sourceVal));

    nsXPIDLCString propVal;
    aProperty->GetValueConst(getter_Shares(propVal));

    nsCOMPtr<nsIRDFResource> targetRes(do_QueryInterface(aTarget, &rv));
    nsXPIDLCString targetVal;
    if (NS_SUCCEEDED(rv))
        targetRes->GetValueConst(getter_Shares(targetVal));

    printf("HasAssertion(%s, %s, %s..)?\n",
           (const char*)sourceVal,
           (const char*)propVal,
           (const char*)targetVal ? (const char*)targetVal : "(null)");
        
#endif

    if (aSource == mSearchRoot.get() &&
        aProperty == kNC_MessageChild.get()) {
#ifdef DEBUG_alecf
        printf("I care about this one.\n");
#endif

        PRInt32 indexOfResource;
        mSearchResults->GetIndexOf(NS_STATIC_CAST(nsISupports*,aTarget),
                                    &indexOfResource);

        if (indexOfResource != -1) {
#ifdef DEBUG_alecf
            printf("And it's a success!\n");
#endif
            *aResult = PR_TRUE;
            return NS_OK;
        }
    }
    
    
    return NS_RDF_NO_VALUE;
}


/* nsISimpleEnumerator ArcLabelsOut (in nsIRDFResource aSource); */
NS_IMETHODIMP
nsMsgSearchDataSource::ArcLabelsOut(nsIRDFResource *aSource,
                                    nsISimpleEnumerator **aResult)
{
    nsresult rv;
    
    if (aSource == mSearchRoot.get()) {
        rv = NS_NewSingletonEnumerator(aResult, kNC_MessageChild);
    }

    else
        rv = NS_NewEmptyEnumerator(aResult);

    return rv;
}

