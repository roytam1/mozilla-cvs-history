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
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

#include <ctype.h> // for toupper()
#include <stdio.h>
#include "nscore.h"
#include "nsIRDFCursor.h"
#include "nsIRDFDataSource.h"
#include "nsIRDFNode.h"
#include "nsIRDFObserver.h"
#include "nsIRDFResourceFactory.h"
#include "nsIServiceManager.h"
#include "nsString.h"
#include "nsVoidArray.h"  // XXX introduces dependency on raptorbase
#include "nsRDFCID.h"
#include "rdfutil.h"
#include "nsIRDFHistory.h"
#include "nsIRDFService.h"
#include "plhash.h"
#include "plstr.h"
#include "prmem.h"
#include "prprf.h"
#include "prio.h"
#include "prtime.h"


////////////////////////////////////////////////////////////////////////
// Interface IDs

static NS_DEFINE_IID(kIRDFArcsInCursorIID,         NS_IRDFARCSINCURSOR_IID);
static NS_DEFINE_IID(kIRDFArcsOutCursorIID,        NS_IRDFARCSOUTCURSOR_IID);
static NS_DEFINE_IID(kIRDFAssertionCursorIID,      NS_IRDFASSERTIONCURSOR_IID);
static NS_DEFINE_IID(kIRDFCursorIID,               NS_IRDFCURSOR_IID);
static NS_DEFINE_IID(kIRDFDataSourceIID,           NS_IRDFDATASOURCE_IID);
static NS_DEFINE_IID(kIRDFLiteralIID,              NS_IRDFLITERAL_IID);
static NS_DEFINE_IID(kIRDFHistoryDataSourceIID,    NS_IRDFHISTORYDATASOURCE_IID);
static NS_DEFINE_IID(kIRDFNodeIID,                 NS_IRDFNODE_IID);
static NS_DEFINE_IID(kIRDFResourceIID,             NS_IRDFRESOURCE_IID);
static NS_DEFINE_IID(kIRDFServiceIID,              NS_IRDFSERVICE_IID);
static NS_DEFINE_IID(kISupportsIID,                NS_ISUPPORTS_IID);
static NS_DEFINE_CID(kRDFServiceCID,               NS_RDFSERVICE_CID);
static NS_DEFINE_CID(kRDFInMemoryDataSourceCID,    NS_RDFINMEMORYDATASOURCE_CID);

////////////////////////////////////////////////////////////////////////
// RDF property & resource declarations

static const char kURIHistoryRoot[]  = "HistoryRoot";
static nsIRDFService* gRDFService = nsnull;


DEFINE_RDF_VOCAB(RDF_NAMESPACE_URI, RDF, instanceOf);

DEFINE_RDF_VOCAB(NC_NAMESPACE_URI, NC, child);
DEFINE_RDF_VOCAB(NC_NAMESPACE_URI, NC, Name);
DEFINE_RDF_VOCAB(NC_NAMESPACE_URI, NC, Folder);
DEFINE_RDF_VOCAB(NC_NAMESPACE_URI, NC, Title);
DEFINE_RDF_VOCAB(NC_NAMESPACE_URI, NC, Page);
DEFINE_RDF_VOCAB(NC_NAMESPACE_URI, NC, Date);
DEFINE_RDF_VOCAB(NC_NAMESPACE_URI, NC, Referer);

DEFINE_RDF_VOCAB(WEB_NAMESPACE_URI, WEB, LastVisitDate);
DEFINE_RDF_VOCAB(WEB_NAMESPACE_URI, WEB, LastModifiedDate);

typedef struct _HistoryEntry {
    PRTime date;
    char* url;
    char* referer;
} HistoryEntry;

typedef HistoryEntry* HE;

static PLHashNumber
rdf_HashPointer(const void* key)
{
    return (PLHashNumber) key;
}

class nsHistoryDataSource : public nsIRDFHistoryDataSource {
protected:
    nsIRDFDataSource*  mInner;
    nsVoidArray        mFiles;
    char*              mCurrentFilePath;
    FILE*              mCurrentFile;
    nsVoidArray        mPendingWrites; 
    PLHashTable*       mLastVisitDateHash; 
    PRExplodedTime     mSessionTime;

    nsIRDFResource*    mResourcePage;
    nsIRDFResource*    mResourceDate;    
    nsIRDFResource*    mResourceTitle;
    nsIRDFResource*    mResourceReferer;
    nsIRDFResource*    mResourceChild;
    nsIRDFResource*    mResourceHistoryBySite;

    nsresult ReadHistory(void);
    nsresult ReadOneHistoryFile(const char* uri);
    nsresult AddPageToGraph(char* url, char* title, char* referer,  PRTime date);
    nsresult AddToDateHierarchy (PRTime date, nsIRDFResource* resource) ;
    nsresult getSiteOfURL(char* url, nsIRDFResource** resource) ;

public:
    // nsISupports
    NS_DECL_ISUPPORTS

    nsHistoryDataSource(void) {
        NS_INIT_REFCNT();
    }

    virtual ~nsHistoryDataSource(void) {
        NS_RELEASE(mInner);
        nsServiceManager::ReleaseService(kRDFServiceCID, gRDFService);
    }

    NS_IMETHOD Init(const char* uri) ;

    NS_IMETHOD GetURI(const char* *uri) {
        return mInner->GetURI(uri);
    }

    NS_IMETHOD GetSource(nsIRDFResource* property,
                         nsIRDFNode* target,
                         PRBool tv, nsIRDFResource** source) {
        return mInner->GetSource(property, target, tv, source);
    }

    NS_IMETHOD GetSources(nsIRDFResource* property,
                          nsIRDFNode* target,  PRBool tv,
                          nsIRDFAssertionCursor** sources) {
        return mInner->GetSources(property, target, tv, sources);
    }

    NS_IMETHOD GetTarget(nsIRDFResource* source,
                         nsIRDFResource* property,  PRBool tv,
                         nsIRDFNode** target) {
        return mInner->GetTarget(source, property, tv, target);
    }

    NS_IMETHOD GetTargets(nsIRDFResource* source,
                          nsIRDFResource* property,  PRBool tv,
                          nsIRDFAssertionCursor** targets) {
        return mInner->GetTargets(source, property, tv, targets);
    }

    NS_IMETHOD Assert(nsIRDFResource* source, 
                      nsIRDFResource* property, 
                      nsIRDFNode* target, PRBool tv) {
        return mInner->Assert(source, property, target, tv);
    }

    NS_IMETHOD Unassert(nsIRDFResource* source,
                        nsIRDFResource* property, nsIRDFNode* target) {
        return mInner->Unassert(source, property, target);
    }

    NS_IMETHOD HasAssertion(nsIRDFResource* source,
                            nsIRDFResource* property,
                            nsIRDFNode* target, PRBool tv,
                            PRBool* hasAssertion) {
        return mInner->HasAssertion(source, property, target, tv, hasAssertion);
    }

    NS_IMETHOD AddObserver(nsIRDFObserver* n) {
        return mInner->AddObserver(n);
    }

    NS_IMETHOD RemoveObserver(nsIRDFObserver* n) {
        return mInner->RemoveObserver(n);
    }

    NS_IMETHOD ArcLabelsIn(nsIRDFNode* node,
                           nsIRDFArcsInCursor** labels) {
        return mInner->ArcLabelsIn(node, labels);
    }

    NS_IMETHOD ArcLabelsOut(nsIRDFResource* source,
                            nsIRDFArcsOutCursor** labels) {
        return mInner->ArcLabelsOut(source, labels);
    }

    NS_IMETHOD Flush() {
        return mInner->Flush();
    }

    NS_IMETHOD IsCommandEnabled(const char* aCommand,
                                nsIRDFResource* aCommandTarget,
                                PRBool* aResult) {
        return mInner->IsCommandEnabled(aCommand, aCommandTarget, aResult);
    }

    NS_IMETHOD DoCommand(const char* aCommand,
                         nsIRDFResource* aCommandTarget) {
        return mInner->DoCommand(aCommand, aCommandTarget);
    }

    NS_IMETHOD GetURI(const char* *uri) const {
        return mInner->GetURI(uri);
    }

    NS_IMETHOD GetAllResources(nsIRDFResourceCursor** aCursor) {
        return mInner->GetAllResources(aCursor);
    }

    NS_IMETHOD AddPage (const char* uri, const char* referer, PRTime time);

    NS_IMETHOD SetPageTitle (const char* uri, PRUnichar* title) ;

    NS_IMETHOD RemovePage (nsIRDFResource* page) {
        return NS_OK;
    }

    NS_IMETHOD LastVisitDate (const char* uri, uint32 *date) {
        nsIRDFResource* res;
        PRBool found = 0;
        *date = 0;
        gRDFService->FindResource(uri, &res, &found);
        if (found) *date = (uint32)PL_HashTableLookup(mLastVisitDateHash, res);    
        return NS_OK;
    }

    nsresult UpdateLastVisitDate (nsIRDFResource* res, uint32 date) {
        uint32 edate = (uint32) PL_HashTableLookup(mLastVisitDateHash, res);    
        if (edate && (edate > date)) return NS_OK;
        PL_HashTableAdd(mLastVisitDateHash, res, (void*)date);
        return NS_OK;
    }

    NS_IMETHOD CompleteURL (const char* prefix, char** preferredCompletion) {
        return NS_OK;
    }

};

NS_IMPL_ADDREF(nsHistoryDataSource);
NS_IMPL_RELEASE(nsHistoryDataSource);

NS_IMETHODIMP
nsHistoryDataSource::QueryInterface(REFNSIID aIID, void** aResult)
{
    NS_PRECONDITION(aResult != nsnull, "null ptr");
    if (! aResult)
        return NS_ERROR_NULL_POINTER;

    if (aIID.Equals(nsIRDFHistoryDataSource::GetIID()) ||
        aIID.Equals(nsIRDFDataSource::GetIID()) ||
        aIID.Equals(nsISupports::GetIID())) {
        *aResult = NS_STATIC_CAST(nsIRDFHistoryDataSource*, this);
        NS_ADDREF(this);
        return NS_OK;
    }
    else {
        *aResult = nsnull;
        return NS_NOINTERFACE;
    }
}

NS_IMETHODIMP
nsHistoryDataSource::Init(const char* uri)
{
    nsresult rv;
    if (NS_FAILED(rv = nsRepository::CreateInstance(kRDFInMemoryDataSourceCID,
                                                    nsnull, kIRDFDataSourceIID,
                                                    (void**) &mInner)))
        return rv;

    if (NS_FAILED(rv = mInner->Init(uri)))
        return rv;


    rv = nsServiceManager::GetService(kRDFServiceCID,
                                               kIRDFServiceIID,
                                               (nsISupports**) &gRDFService);

    PR_ASSERT(NS_SUCCEEDED(rv));

    // register this as a named data source with the RDF service
    gRDFService->RegisterDataSource(this);
    
    gRDFService->GetResource(kURINC_Page,    &mResourcePage);
    gRDFService->GetResource(kURINC_Date,    &mResourceDate);
    gRDFService->GetResource(kURINC_Name,   &mResourceTitle);
    gRDFService->GetResource(kURINC_Referer, &mResourceReferer);
    gRDFService->GetResource(kURINC_child,   &mResourceChild);
    gRDFService->GetResource("NC:HistoryBySite",   &mResourceHistoryBySite);

    mLastVisitDateHash = PL_NewHashTable(400, rdf_HashPointer,
                                         PL_CompareValues,
                                         PL_CompareValues,
                                         nsnull, nsnull);
 
    PR_ExplodeTime(PR_Now(), PR_LocalTimeParameters, &mSessionTime);
    
    if (NS_FAILED(rv = ReadHistory()))
        return rv;

    if (!mCurrentFile) {
        char fileurl[256];
        PR_snprintf(fileurl, sizeof(fileurl), "res\\rdf\\History\\%i.hst",  PR_Now());
    }
    return NS_OK;
}

NS_IMETHODIMP 
nsHistoryDataSource::AddPage (const char* uri, const char* referer, PRTime time) {
    HE he = (HE) PR_Malloc(sizeof(HistoryEntry));
    he->date = time;
    he->url = (char*) uri;
    he->referer = (char*) referer;
    mPendingWrites.AppendElement(he);
    return NS_OK;
}

NS_IMETHODIMP 
nsHistoryDataSource::SetPageTitle (const char* uri, PRUnichar* title) {
    for (PRInt32 i = mPendingWrites.Count() - 1; i >= 0; --i) {
        HE wr = (HE) mPendingWrites.ElementAt(i);
        if (strcmp(wr->url, uri) == 0) {
            char timeBuffer[256];
            PRExplodedTime etime;
            PR_ExplodeTime(wr->date, PR_LocalTimeParameters, &etime);
            PR_FormatTimeUSEnglish(timeBuffer, 256, "%a %b %d %H:%M:%S %Y", &etime);
            fprintf(mCurrentFile, "%s\t%s\t%s\t%s\n", wr->url, title, wr->referer, timeBuffer);
            mPendingWrites.RemoveElement(wr);
            AddPageToGraph(wr->url,  (char*) title, wr->referer,  PR_Now()) ;
            PR_Free(wr);
            return NS_OK;
        }    
    }
    return NS_OK;
}



PRBool
endsWith (const char* pattern, const char* uuid)
{
  short l1 = strlen(pattern);
  short l2 = strlen(uuid);
  short index;
  if (l2 < l1) return false;  
  for (index = 1; index <= l1; index++) {
    if (pattern[l1-index] != uuid[l2-index]) return false;
  }  
  return true;
}

nsresult
nsHistoryDataSource::ReadHistory(void)
{
    nsresult rv = NS_ERROR_FAILURE;
    char fileurl[256];
    PRInt32 n = PR_SKIP_BOTH;
    PRDirEntry	*de;
    PRDir* dir;
    PR_snprintf(fileurl, sizeof(fileurl), "res\\rdf\\History");
    dir =  PR_OpenDir(fileurl);
    while ((dir != NULL) && ((de = PR_ReadDir(dir, (PRDirFlags)(n++))) != NULL)) {
        if (endsWith(".hst", de->name)) ReadOneHistoryFile(de->name);
    }
    if (dir) PR_CloseDir(dir);
    return NS_OK;
}


nsresult
nsHistoryDataSource::ReadOneHistoryFile(const char* fileName)
{
    nsresult rv = NS_ERROR_FAILURE;
    char fileurl[256];
    FILE* f;
    PR_snprintf(fileurl, sizeof(fileurl), "res\\rdf\\History\\%s", fileName);
    if ((f = fopen(fileurl, "rw")) != nsnull) {
        char* buffer = (char*) PR_Malloc(1024 * 4);
        int n = 0;
        char *title, *url, *referer, *date;
        PRTime time;
        PRTime fileTime = LL_ZERO;
        while (fgets(buffer, 1024 * 4, f) != 0) {
            // url<tab>title<tab>referer<tab>date
            url = buffer;
			buffer[strlen(buffer)-1] = '\0';
            title = strchr(buffer, '\t') + 1;
            *(title - 1) = '\0';
            referer = strchr(title, '\t') + 1;
            *(referer - 1) = '\0';
            date = strchr(referer, '\t') + 1;         
			*(date -1 ) = '\0';
            PR_ParseTimeString (date, 0, &time);
            if (LL_IS_ZERO(fileTime)) { 
                PRExplodedTime etime;
                fileTime = time;
                PR_ExplodeTime(time, PR_LocalTimeParameters, &etime);
                if (etime.tm_yday == mSessionTime.tm_yday) mCurrentFile = f;
            }
            AddPageToGraph(url, title, referer,  time);
        }        
        PR_Free(buffer);  
		if (f != mCurrentFile) fclose(f);
    }
    return rv;
}

nsresult 
nsHistoryDataSource::getSiteOfURL(char* url, nsIRDFResource** resource) {
    char* str = strstr(url, "://");
    char buff[256];
	 nsIRDFLiteral  *titleLiteral;
    PRBool foundp;
    if (!str) return NS_ERROR_FAILURE;
    str = str + 3;
    if (str[0] == '/') str++;
    char* estr = strchr(str, '/');
    if (!estr) estr = str + strlen(str);
    if ((estr - str) > 256) return NS_ERROR_FAILURE;
	memset(buff, '\0', 256);
    static const char kHistoryURLPrefix[] = "hst:site?";
    strcpy(buff, kHistoryURLPrefix);
    strncat(buff, str, (estr-str));
    if (NS_OK != gRDFService->FindResource(buff, resource, &foundp)) return NS_ERROR_FAILURE;
    if (foundp) return NS_OK;
    if (NS_OK != gRDFService->GetResource(buff, resource)) return NS_ERROR_FAILURE;
    mInner->Assert(mResourceHistoryBySite,  mResourceChild, *resource, 1);
    nsAutoString ptitle(buff + sizeof(kHistoryURLPrefix) - 1);	
    gRDFService->GetLiteral(ptitle, &titleLiteral);
	mInner->Assert(*resource, mResourceTitle, titleLiteral, 1);
    return NS_OK;
}

nsresult 
nsHistoryDataSource::AddPageToGraph(char* url, char* title, 
                                    char* referer,  PRTime date) {
    char* histURL = (char*) PR_Malloc(strlen(url) + 7);    
    nsIRDFResource *pageResource, *refererResource, *histResource, *siteResource;    
    nsIRDFLiteral  *titleLiteral;
    nsIRDFDate     *dateLiteral;
    PR_snprintf(histURL, strlen(histURL), "hst://%s", url);
    nsresult rv = getSiteOfURL(url, &siteResource);
    if (rv != NS_OK) return rv;
    gRDFService->GetResource(url, &pageResource);
    gRDFService->GetResource(histURL, &histResource);
    gRDFService->GetResource(referer, &refererResource);
    gRDFService->GetDateLiteral(date, &dateLiteral);
    nsAutoString ptitle(title);
    gRDFService->GetLiteral(ptitle, &titleLiteral);
    mInner->Assert(histResource,  mResourcePage, pageResource, 1);
    mInner->Assert(histResource,  mResourceDate, dateLiteral, 1);
    mInner->Assert(pageResource,  mResourceTitle, titleLiteral, 1);
    mInner->Assert(histResource,  mResourceReferer, refererResource, 1);
    mInner->Assert(siteResource,  mResourceChild, pageResource, 1);
    AddToDateHierarchy(date, histResource);
    UpdateLastVisitDate (pageResource, 1/* date */) ;
    NS_RELEASE(histResource);
    NS_RELEASE(siteResource);
    NS_RELEASE(pageResource);
    NS_RELEASE(refererResource);
    NS_RELEASE(dateLiteral);
    NS_RELEASE(titleLiteral);
    PR_Free(histURL);
    return NS_OK;
}

nsresult
nsHistoryDataSource::AddToDateHierarchy (PRTime date, nsIRDFResource* resource) {
    return NS_OK;
}

nsresult
NS_NewRDFHistoryDataSource(nsIRDFDataSource** result)
{
    nsHistoryDataSource* ds = new nsHistoryDataSource();

    if (! ds)
        return NS_ERROR_NULL_POINTER;

    *result = ds;
    NS_ADDREF(*result);
    return NS_OK;
}
    
    










