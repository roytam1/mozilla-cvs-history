/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4; c-file-style: "stroustrup" -*-
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

/*
  Implementation for an internet search RDF data store.
 */

#include <ctype.h> // for toupper()
#include <stdio.h>
#include "nscore.h"
#include "nsCOMPtr.h"
#include "nsIEnumerator.h"
#include "nsIRDFDataSource.h"
#include "nsIRDFNode.h"
#include "nsIRDFObserver.h"
#include "nsIServiceManager.h"
#include "nsString.h"
#include "nsVoidArray.h"  // XXX introduces dependency on raptorbase
#include "nsXPIDLString.h"
#include "nsRDFCID.h"
#include "rdfutil.h"
#include "nsIRDFService.h"
#include "xp_core.h"
#include "plhash.h"
#include "plstr.h"
#include "prmem.h"
#include "prprf.h"
#include "prio.h"
#include "rdf.h"
#include "nsFileSpec.h"
#include "nsFileStream.h"
#include "nsSpecialSystemDirectory.h"
#include "nsEnumeratorUtils.h"

#include "nsEscape.h"

#include "nsIPostToServer.h"
#include "nsIURL.h"
#ifdef NECKO
#include "nsIIOService.h"
#include "nsIURL.h"
static NS_DEFINE_CID(kIOServiceCID, NS_IOSERVICE_CID);
#endif // NECKO
#include "nsIBuffer.h"
#include "nsIInputStream.h"
#include "nsIBufferInputStream.h"
#include "nsIStreamListener.h"
#include "nsIRDFSearch.h"

#ifdef	XP_MAC
#include "Files.h"
#endif

#ifdef	XP_WIN
#include "windef.h"
#include "winbase.h"
#endif


#define	POSTHEADER_PREFIX	"Content-type: application/x-www-form-urlencoded; charset=ISO-8859-1\r\nContent-Length: "
#define	POSTHEADER_SUFFIX	"\r\n\r\n"


static NS_DEFINE_CID(kRDFServiceCID,               NS_RDFSERVICE_CID);
static NS_DEFINE_CID(kRDFInMemoryDataSourceCID,    NS_RDFINMEMORYDATASOURCE_CID);
static NS_DEFINE_IID(kISupportsIID,                NS_ISUPPORTS_IID);

static const char kURINC_SearchRoot[] = "NC:SearchEngineRoot";



class SearchDataSourceCallback : public nsIStreamListener
{
private:
	nsIRDFDataSource	*mDataSource;
	nsIRDFResource		*mParent;
	nsIRDFResource		*mEngine;
	static PRInt32		gRefCnt;

    // pseudo-constants
	static nsIRDFResource	*kNC_loading;
	static nsIRDFResource	*kNC_Child;
	static nsIRDFResource	*kNC_Name;
	static nsIRDFResource	*kNC_Data;
	static nsIRDFResource	*kNC_Relevance;
	static nsIRDFResource	*kNC_RelevanceSort;
	static nsIRDFResource	*kNC_Site;
	static nsIRDFResource	*kNC_Engine;

	char			*mLine;

public:

	NS_DECL_ISUPPORTS

			SearchDataSourceCallback(nsIRDFDataSource *ds, nsIRDFResource *parent, nsIRDFResource *engine);
	virtual		~SearchDataSourceCallback(void);

	// stream observer

	NS_IMETHOD	OnStartBinding(nsIURI *aURL, const char *aContentType);
	NS_IMETHOD	OnProgress(nsIURI* aURL, PRUint32 aProgress, PRUint32 aProgressMax);
	NS_IMETHOD	OnStatus(nsIURI* aURL, const PRUnichar* aMsg);
	NS_IMETHOD	OnStopBinding(nsIURI* aURL, nsresult aStatus, const PRUnichar* aMsg);

	// stream listener
	NS_IMETHOD	GetBindInfo(nsIURI* aURL, nsStreamBindingInfo* aInfo);
	NS_IMETHOD	OnDataAvailable(nsIURI* aURL, nsIInputStream *aIStream, 
                               PRUint32 aLength);
};



class SearchDataSource : public nsIRDFSearchDataSource
{
private:
	static PRInt32		gRefCnt;

    // pseudo-constants
	static nsIRDFResource		*kNC_SearchRoot;
	static nsIRDFResource		*kNC_Child;
	static nsIRDFResource		*kNC_Data;
	static nsIRDFResource		*kNC_Name;
	static nsIRDFResource		*kNC_URL;
	static nsIRDFResource		*kRDF_InstanceOf;
	static nsIRDFResource		*kRDF_type;

protected:
	static nsIRDFDataSource		*mInner;

public:

friend	class SearchDataSourceCallback;

	NS_DECL_ISUPPORTS

				SearchDataSource(void);
	virtual		~SearchDataSource(void);
	nsresult	Init();

	// nsIRDFDataSource methods

	NS_IMETHOD	GetURI(char **uri);
	NS_IMETHOD	GetSource(nsIRDFResource *property,
				nsIRDFNode *target,
				PRBool tv,
				nsIRDFResource **source /* out */);
	NS_IMETHOD	GetSources(nsIRDFResource *property,
				nsIRDFNode *target,
				PRBool tv,
				nsISimpleEnumerator **sources /* out */);
	NS_IMETHOD	GetTarget(nsIRDFResource *source,
				nsIRDFResource *property,
				PRBool tv,
				nsIRDFNode **target /* out */);
	NS_IMETHOD	GetTargets(nsIRDFResource *source,
				nsIRDFResource *property,
				PRBool tv,
				nsISimpleEnumerator **targets /* out */);
	NS_IMETHOD	Assert(nsIRDFResource *source,
				nsIRDFResource *property,
				nsIRDFNode *target,
				PRBool tv);
	NS_IMETHOD	Unassert(nsIRDFResource *source,
				nsIRDFResource *property,
				nsIRDFNode *target);
	NS_IMETHOD	Change(nsIRDFResource* aSource,
				nsIRDFResource* aProperty,
				nsIRDFNode* aOldTarget,
				nsIRDFNode* aNewTarget);
	NS_IMETHOD	Move(nsIRDFResource* aOldSource,
				nsIRDFResource* aNewSource,
				nsIRDFResource* aProperty,
				nsIRDFNode* aTarget);
	NS_IMETHOD	HasAssertion(nsIRDFResource *source,
				nsIRDFResource *property,
				nsIRDFNode *target,
				PRBool tv,
				PRBool *hasAssertion /* out */);
	NS_IMETHOD	ArcLabelsIn(nsIRDFNode *node,
				nsISimpleEnumerator **labels /* out */);
	NS_IMETHOD	ArcLabelsOut(nsIRDFResource *source,
				nsISimpleEnumerator **labels /* out */);
	NS_IMETHOD	GetAllResources(nsISimpleEnumerator** aResult);
	NS_IMETHOD	AddObserver(nsIRDFObserver *n);
	NS_IMETHOD	RemoveObserver(nsIRDFObserver *n);
	NS_IMETHOD	GetAllCommands(nsIRDFResource* source,
				nsIEnumerator/*<nsIRDFResource>*/** commands);
	NS_IMETHOD	GetAllCmds(nsIRDFResource* source,
				nsISimpleEnumerator/*<nsIRDFResource>*/** commands);

	NS_IMETHOD	IsCommandEnabled(nsISupportsArray/*<nsIRDFResource>*/* aSources,
				nsIRDFResource*   aCommand,
				nsISupportsArray/*<nsIRDFResource>*/* aArguments,
				PRBool* aResult);

	NS_IMETHOD	DoCommand(nsISupportsArray/*<nsIRDFResource>*/* aSources,
				nsIRDFResource*   aCommand,
				nsISupportsArray/*<nsIRDFResource>*/* aArguments);


	// helper methods
static PRBool		isEngineURI(nsIRDFResource* aResource);
static PRBool		isSearchURI(nsIRDFResource* aResource);
static nsresult		BeginSearchRequest(nsIRDFResource *source);
static nsresult		DoSearch(nsIRDFResource *source, nsIRDFResource *engine, nsString text);
static nsresult		GetSearchEngineList();
static nsresult		ReadFileContents(char *basename, nsString & sourceContents);
static nsresult		GetData(nsString data, char *sectionToFind, char *attribToFind, nsString &value);
static nsresult		GetInputs(nsString data, nsString text, nsString &input);
static nsresult		GetURL(nsIRDFResource *source, nsIRDFLiteral** aResult);
static PRBool		isVisible(const nsNativeFileSpec& file);

};



static	nsIRDFService		*gRDFService = nsnull;
static	SearchDataSource	*gSearchDataSource = nsnull;

PRInt32				SearchDataSource::gRefCnt;
nsIRDFDataSource		*SearchDataSource::mInner = nsnull;

nsIRDFResource			*SearchDataSource::kNC_SearchRoot;
nsIRDFResource			*SearchDataSource::kNC_Child;
nsIRDFResource			*SearchDataSource::kNC_Data;
nsIRDFResource			*SearchDataSource::kNC_Name;
nsIRDFResource			*SearchDataSource::kNC_URL;
nsIRDFResource			*SearchDataSource::kRDF_InstanceOf;
nsIRDFResource			*SearchDataSource::kRDF_type;

PRInt32				SearchDataSourceCallback::gRefCnt;

nsIRDFResource			*SearchDataSourceCallback::kNC_Child;
nsIRDFResource			*SearchDataSourceCallback::kNC_Name;
nsIRDFResource			*SearchDataSourceCallback::kNC_Data;
nsIRDFResource			*SearchDataSourceCallback::kNC_Relevance;
nsIRDFResource			*SearchDataSourceCallback::kNC_RelevanceSort;
nsIRDFResource			*SearchDataSourceCallback::kNC_Site;
nsIRDFResource			*SearchDataSourceCallback::kNC_Engine;
nsIRDFResource			*SearchDataSourceCallback::kNC_loading;



PRBool
SearchDataSource::isEngineURI(nsIRDFResource *r)
{
	PRBool		isEngineURIFlag = PR_FALSE;
        nsXPIDLCString	uri;
	
	r->GetValue( getter_Copies(uri) );
	if (!strncmp(uri, "engine://", 9))
	{
		isEngineURIFlag = PR_TRUE;
	}
	return(isEngineURIFlag);
}



PRBool
SearchDataSource::isSearchURI(nsIRDFResource *r)
{
	PRBool		isSearchURIFlag = PR_FALSE;
        nsXPIDLCString	uri;
	
	r->GetValue( getter_Copies(uri) );
	if (!strncmp(uri, "internetsearch:", strlen("internetsearch:")))
	{
		isSearchURIFlag = PR_TRUE;
	}
	return(isSearchURIFlag);
}



SearchDataSource::SearchDataSource(void)
{
	NS_INIT_REFCNT();

	if (gRefCnt++ == 0)
	{
		nsresult rv = nsServiceManager::GetService(kRDFServiceCID,
			nsIRDFService::GetIID(), (nsISupports**) &gRDFService);

		PR_ASSERT(NS_SUCCEEDED(rv));

		gRDFService->GetResource(kURINC_SearchRoot,                   &kNC_SearchRoot);
		gRDFService->GetResource(NC_NAMESPACE_URI "child",            &kNC_Child);
		gRDFService->GetResource(NC_NAMESPACE_URI "data",             &kNC_Data);
		gRDFService->GetResource(NC_NAMESPACE_URI "Name",             &kNC_Name);
		gRDFService->GetResource(NC_NAMESPACE_URI "URL",              &kNC_URL);

		gRDFService->GetResource(RDF_NAMESPACE_URI "instanceOf",      &kRDF_InstanceOf);
		gRDFService->GetResource(RDF_NAMESPACE_URI "type",            &kRDF_type);

		gSearchDataSource = this;
	}
}



SearchDataSource::~SearchDataSource (void)
{
	gRDFService->UnregisterDataSource(this);

	if (--gRefCnt == 0)
	{
		NS_RELEASE(kNC_SearchRoot);
		NS_RELEASE(kNC_Child);
		NS_RELEASE(kNC_Data);
		NS_RELEASE(kNC_Name);
		NS_RELEASE(kNC_URL);
		NS_RELEASE(kRDF_InstanceOf);
		NS_RELEASE(kRDF_type);

		NS_IF_RELEASE(mInner);

		gSearchDataSource = nsnull;
		nsServiceManager::ReleaseService(kRDFServiceCID, gRDFService);
		gRDFService = nsnull;
	}
}



NS_IMPL_ISUPPORTS(SearchDataSource, nsIRDFDataSource::GetIID());



nsresult
SearchDataSource::Init()
{
	nsresult	rv = NS_ERROR_OUT_OF_MEMORY;

	if (NS_FAILED(rv = nsComponentManager::CreateInstance(kRDFInMemoryDataSourceCID,
		nsnull, nsIRDFDataSource::GetIID(), (void **)&mInner)))
		return(rv);

	// register this as a named data source with the service manager
	if (NS_FAILED(rv = gRDFService->RegisterDataSource(this, PR_FALSE)))
		return(rv);

	// get available search engines
	if (NS_FAILED(rv = GetSearchEngineList()))
		return(rv);

	return NS_OK;
}



NS_IMETHODIMP
SearchDataSource::GetURI(char **uri)
{
	NS_PRECONDITION(uri != nsnull, "null ptr");
	if (! uri)
		return NS_ERROR_NULL_POINTER;

	if ((*uri = nsXPIDLCString::Copy("rdf:search")) == nsnull)
		return NS_ERROR_OUT_OF_MEMORY;

	return NS_OK;
}



NS_IMETHODIMP
SearchDataSource::GetSource(nsIRDFResource* property,
                                nsIRDFNode* target,
                                PRBool tv,
                                nsIRDFResource** source /* out */)
{
	NS_PRECONDITION(property != nsnull, "null ptr");
	if (! property)
		return NS_ERROR_NULL_POINTER;

	NS_PRECONDITION(target != nsnull, "null ptr");
	if (! target)
		return NS_ERROR_NULL_POINTER;

	NS_PRECONDITION(source != nsnull, "null ptr");
	if (! source)
		return NS_ERROR_NULL_POINTER;

	*source = nsnull;
	return NS_RDF_NO_VALUE;
}



NS_IMETHODIMP
SearchDataSource::GetSources(nsIRDFResource *property,
                                 nsIRDFNode *target,
                                 PRBool tv,
                                 nsISimpleEnumerator **sources /* out */)
{
	NS_NOTYETIMPLEMENTED("write me");
	return NS_ERROR_NOT_IMPLEMENTED;
}



NS_IMETHODIMP
SearchDataSource::GetTarget(nsIRDFResource *source,
                                nsIRDFResource *property,
                                PRBool tv,
                                nsIRDFNode **target /* out */)
{
	NS_PRECONDITION(source != nsnull, "null ptr");
	if (! source)
		return NS_ERROR_NULL_POINTER;

	NS_PRECONDITION(property != nsnull, "null ptr");
	if (! property)
		return NS_ERROR_NULL_POINTER;

	NS_PRECONDITION(target != nsnull, "null ptr");
	if (! target)
		return NS_ERROR_NULL_POINTER;

	nsresult		rv = NS_RDF_NO_VALUE;

	// we only have positive assertions in the internet search data source.
	if (! tv)
		return(rv);
	
	if (mInner)
	{
		rv = mInner->GetTarget(source, property, tv, target);
	}
	return(rv);
}


NS_IMETHODIMP
SearchDataSource::GetTargets(nsIRDFResource *source,
                           nsIRDFResource *property,
                           PRBool tv,
                           nsISimpleEnumerator **targets /* out */)
{
	NS_PRECONDITION(source != nsnull, "null ptr");
	if (! source)
		return NS_ERROR_NULL_POINTER;

	NS_PRECONDITION(property != nsnull, "null ptr");
	if (! property)
		return NS_ERROR_NULL_POINTER;

	NS_PRECONDITION(targets != nsnull, "null ptr");
	if (! targets)
		return NS_ERROR_NULL_POINTER;

	nsresult rv = NS_RDF_NO_VALUE;

	// we only have positive assertions in the internet search data source.
	if (! tv)
		return(rv);

	if (source == kNC_SearchRoot)
	{
		if (property == kNC_Child)
		{
			if (mInner)
			{
				rv = mInner->GetTargets(source, property,tv, targets);
			}
			return(rv);
		}
	}
	else if (isSearchURI(source))
	{
		if (property == kNC_Child)
		{
			if (mInner)
			{
				rv = mInner->GetTargets(source, property,tv, targets);
			}
			if (NS_SUCCEEDED(rv) && (targets))
			{
				// check and see if we already have data for the search in question;
				// if we do, don't bother doing the search again, otherwise kickstart it
				PRBool		hasResults = PR_FALSE;
				if (NS_FAILED((*targets)->HasMoreElements(&hasResults)) || (hasResults == PR_FALSE))
				{
					BeginSearchRequest(source);
				}
			}
			return(rv);
		}
	}
	return NS_NewEmptyEnumerator(targets);
}



NS_IMETHODIMP
SearchDataSource::Assert(nsIRDFResource *source,
                       nsIRDFResource *property,
                       nsIRDFNode *target,
                       PRBool tv)
{
	return NS_RDF_ASSERTION_REJECTED;
}



NS_IMETHODIMP
SearchDataSource::Unassert(nsIRDFResource *source,
                         nsIRDFResource *property,
                         nsIRDFNode *target)
{
	return NS_RDF_ASSERTION_REJECTED;
}

NS_IMETHODIMP
SearchDataSource::Change(nsIRDFResource* aSource,
						 nsIRDFResource* aProperty,
						 nsIRDFNode* aOldTarget,
						 nsIRDFNode* aNewTarget)
{
	return NS_RDF_ASSERTION_REJECTED;
}


NS_IMETHODIMP
SearchDataSource::Move(nsIRDFResource* aOldSource,
					   nsIRDFResource* aNewSource,
					   nsIRDFResource* aProperty,
					   nsIRDFNode* aTarget)
{
	return NS_RDF_ASSERTION_REJECTED;
}


NS_IMETHODIMP
SearchDataSource::HasAssertion(nsIRDFResource *source,
                             nsIRDFResource *property,
                             nsIRDFNode *target,
                             PRBool tv,
                             PRBool *hasAssertion /* out */)
{
	NS_PRECONDITION(source != nsnull, "null ptr");
	if (! source)
		return NS_ERROR_NULL_POINTER;

	NS_PRECONDITION(property != nsnull, "null ptr");
	if (! property)
		return NS_ERROR_NULL_POINTER;

	NS_PRECONDITION(target != nsnull, "null ptr");
	if (! target)
		return NS_ERROR_NULL_POINTER;

	NS_PRECONDITION(hasAssertion != nsnull, "null ptr");
	if (! hasAssertion)
		return NS_ERROR_NULL_POINTER;

	// we only have positive assertions in the internet search data source.
	if (! tv)
	{
		*hasAssertion = PR_FALSE;
		return NS_OK;
        }
        nsresult	rv = NS_RDF_NO_VALUE;
        
        if (mInner)
        {
		mInner->HasAssertion(source, property, target, tv, hasAssertion);
	}
        return(rv);
}



NS_IMETHODIMP
SearchDataSource::ArcLabelsIn(nsIRDFNode *node,
                            nsISimpleEnumerator ** labels /* out */)
{
	NS_NOTYETIMPLEMENTED("write me");
	return NS_ERROR_NOT_IMPLEMENTED;
}



NS_IMETHODIMP
SearchDataSource::ArcLabelsOut(nsIRDFResource *source,
                             nsISimpleEnumerator **labels /* out */)
{
	NS_PRECONDITION(source != nsnull, "null ptr");
	if (! source)
		return NS_ERROR_NULL_POINTER;

	NS_PRECONDITION(labels != nsnull, "null ptr");
	if (! labels)
		return NS_ERROR_NULL_POINTER;

	nsresult rv;

	if ((source == kNC_SearchRoot) || isSearchURI(source))
	{
            nsCOMPtr<nsISupportsArray> array;
            rv = NS_NewISupportsArray(getter_AddRefs(array));
            if (NS_FAILED(rv)) return rv;

            array->AppendElement(kNC_Child);

            nsISimpleEnumerator* result = new nsArrayEnumerator(array);
            if (! result)
                return NS_ERROR_OUT_OF_MEMORY;

            NS_ADDREF(result);
            *labels = result;
            return NS_OK;
	}

	return NS_NewEmptyEnumerator(labels);
}



NS_IMETHODIMP
SearchDataSource::GetAllResources(nsISimpleEnumerator** aCursor)
{
	return mInner->GetAllResources(aCursor);
}



NS_IMETHODIMP
SearchDataSource::AddObserver(nsIRDFObserver *aObserver)
{
	return mInner->AddObserver(aObserver);
}



NS_IMETHODIMP
SearchDataSource::RemoveObserver(nsIRDFObserver *aObserver)
{
	return mInner->RemoveObserver(aObserver);
}



NS_IMETHODIMP
SearchDataSource::GetAllCommands(nsIRDFResource* source,
                                     nsIEnumerator/*<nsIRDFResource>*/** commands)
{
	NS_NOTYETIMPLEMENTED("write me!");
	return NS_ERROR_NOT_IMPLEMENTED;
}



NS_IMETHODIMP
SearchDataSource::GetAllCmds(nsIRDFResource* source,
                                     nsISimpleEnumerator/*<nsIRDFResource>*/** commands)
{
	NS_NOTYETIMPLEMENTED("write me!");
	return NS_ERROR_NOT_IMPLEMENTED;
}



NS_IMETHODIMP
SearchDataSource::IsCommandEnabled(nsISupportsArray/*<nsIRDFResource>*/* aSources,
                                       nsIRDFResource*   aCommand,
                                       nsISupportsArray/*<nsIRDFResource>*/* aArguments,
                                       PRBool* aResult)
{
	NS_NOTYETIMPLEMENTED("write me!");
	return NS_ERROR_NOT_IMPLEMENTED;
}



NS_IMETHODIMP
SearchDataSource::DoCommand(nsISupportsArray/*<nsIRDFResource>*/* aSources,
                                nsIRDFResource*   aCommand,
                                nsISupportsArray/*<nsIRDFResource>*/* aArguments)
{
	NS_NOTYETIMPLEMENTED("write me!");
	return NS_ERROR_NOT_IMPLEMENTED;
}



nsresult
NS_NewRDFSearchDataSource(nsIRDFDataSource **result)
{
	if (!result)
		return NS_ERROR_NULL_POINTER;

	// only one search data source
	if (nsnull == gSearchDataSource)
	{
		if ((gSearchDataSource = new SearchDataSource()) == nsnull)
		{
			return NS_ERROR_OUT_OF_MEMORY;
		}

		nsresult rv = gSearchDataSource->Init();
		if (NS_FAILED(rv)) {
			delete gSearchDataSource;
			gSearchDataSource = nsnull;
			return rv;
		}
	}
	NS_ADDREF(gSearchDataSource);
	*result = gSearchDataSource;
	return NS_OK;
}



nsresult
SearchDataSource::BeginSearchRequest(nsIRDFResource *source)
{
        nsresult		rv = NS_OK;
	char			*sourceURI = nsnull;

	if (NS_FAILED(rv = source->GetValue(&sourceURI)))
		return(rv);
	nsAutoString		uri(sourceURI);
	if (uri.Find("internetsearch:") != 0)
		return(NS_ERROR_FAILURE);
	uri.Cut(0, strlen("internetsearch:"));

	nsVoidArray	*engineArray = new nsVoidArray;
	if (!engineArray)
		return(NS_ERROR_FAILURE);

	nsAutoString	text("");

	// parse up attributes

	while(uri.Length() > 0)
	{
		nsAutoString	item("");

		PRInt32 andOffset = uri.Find("&");
		if (andOffset >= 0)
		{
			uri.Left(item, andOffset);
			uri.Cut(0, andOffset + 1);
		}
		else
		{
			item = uri;
			uri.Truncate();
		}

		PRInt32 equalOffset = item.Find("=");
		if (equalOffset < 0)	break;
		
		nsAutoString	attrib(""), value("");
		item.Left(attrib, equalOffset);
		value = item;
		value.Cut(0, equalOffset + 1);
		
		if ((attrib.Length() > 0) && (value.Length() > 0))
		{
			if (attrib.EqualsIgnoreCase("engine"))
			{
				if (value.Find("engine://") == 0)
				{
					char	*val = value.ToNewCString();
					if (val)
					{
						engineArray->AppendElement(val);
					}
				}
			}
			else if (attrib.EqualsIgnoreCase("text"))
			{
				text = value;
			}
		}
	}

	// loop over specified search engines
	while (engineArray->Count() > 0)
	{
		char *basename = (char *)(engineArray->ElementAt(0));
		engineArray->RemoveElementAt(0);
		if (!basename)	continue;

#ifdef	DEBUG
		printf("Search engine to query: '%s'\n", basename);
#endif

		nsCOMPtr<nsIRDFResource>	engine;
		gRDFService->GetResource(basename, getter_AddRefs(engine));
		delete [] basename;
		basename = nsnull;
		if (!engine)	continue;
		DoSearch(source, engine, text);
	}
	
	delete engineArray;
	engineArray = nsnull;

	return(rv);
}



nsresult
SearchDataSource::DoSearch(nsIRDFResource *source, nsIRDFResource *engine, nsString text)
{
	nsresult	rv;

	if (!source)
		return(NS_ERROR_NULL_POINTER);
	if (!engine)
		return(NS_ERROR_NULL_POINTER);

	if (!mInner)
	{
		return(NS_RDF_NO_VALUE);
	}

	// get data
	nsAutoString		data("");
	nsCOMPtr<nsIRDFNode>	dataTarget = nsnull;
	if (NS_SUCCEEDED((rv = mInner->GetTarget(engine, kNC_Data, PR_TRUE, getter_AddRefs(dataTarget)))) && (dataTarget))
	{
		nsCOMPtr<nsIRDFLiteral>	dataLiteral = do_QueryInterface(dataTarget);
		if (!dataLiteral)
			return(rv);
		PRUnichar	*dataUni;
		if (NS_FAILED(rv = dataLiteral->GetValue(&dataUni)))
			return(rv);
		data = dataUni;
	}
	else
	{
		nsXPIDLCString	engineURI;
		if (NS_FAILED(rv = engine->GetValue(getter_Copies(engineURI))))
			return(rv);
		nsAutoString	engineStr(engineURI);
		if (engineStr.Find("engine://") != 0)
			return(rv);
		engineStr.Cut(0, strlen("engine://"));
		char	*basename = engineStr.ToNewCString();
		if (!basename)
			return(rv);
		basename = nsUnescape(basename);
		if (!basename)
			return(rv);
		rv = ReadFileContents(basename, data);
		delete [] basename;
		basename = nsnull;
		if (NS_FAILED(rv))
		{
			return(rv);
		}

		// save file contents
		nsCOMPtr<nsIRDFLiteral>	dataLiteral;
		if (NS_SUCCEEDED(rv = gRDFService->GetLiteral(data.GetUnicode(), getter_AddRefs(dataLiteral))))
		{
			if (mInner)
			{
				mInner->Assert(engine, kNC_Data, dataLiteral, PR_TRUE);
			}
		}
	}
	if (data.Length() < 1)
		return(NS_RDF_NO_VALUE);
	
	nsAutoString	action, method, input;

	if (NS_FAILED(rv = GetData(data, "search", "action", action)))
		return(rv);
	if (NS_FAILED(rv = GetData(data, "search", "method", method)))
		return(rv);
	if (NS_FAILED(rv = GetInputs(data, text, input)))
		return(rv);

#ifdef	DEBUG
	char *cAction = action.ToNewCString();
	char *cMethod = method.ToNewCString();
	char *cInput = input.ToNewCString();
	printf("Search Action: '%s'\n", cAction);
	printf("Search Method: '%s'\n", cMethod);
	printf(" Search Input: '%s'\n\n", cInput);
	if (cAction)
	{
		delete [] cAction;
		cAction = nsnull;
	}
	if (cMethod)
	{
		delete [] cMethod;
		cMethod = nsnull;
	}
	if (cInput)
	{
		delete [] cInput;
		cInput = nsnull;
	}
#endif

	if (input.Length() < 1)
		return(NS_ERROR_UNEXPECTED);

	if (method.EqualsIgnoreCase("get"))
	{
		// HTTP Get method support
		action += "?";
		action += input;
	}
	char	*searchURL = action.ToNewCString();
	if (searchURL)
	{
		nsIURI		*url = nsnull;
#ifndef NECKO
        rv = NS_NewURL(&url, (const char*) searchURL);
#else
        NS_WITH_SERVICE(nsIIOService, service, kIOServiceCID, &rv);
        if (NS_FAILED(rv)) return rv;

        nsIURI *uri = nsnull;
        rv = service->NewURI((const char*) searchURL, nsnull, &uri);
        if (NS_FAILED(rv)) return rv;

        rv = uri->QueryInterface(nsIURI::GetIID(), (void**)&url);
        NS_RELEASE(uri);
#endif // NECKO
		if (NS_SUCCEEDED(rv))
		{
			if (method.EqualsIgnoreCase("post"))
			{
				// HTTP Post method support
			        nsCOMPtr<nsIPostToServer>	pts = do_QueryInterface(url);
				if (pts)
			        {
			        	// construct post data to send
			        	nsAutoString	postStr(POSTHEADER_PREFIX);
			        	postStr.Append(input.Length(), 10);
			        	postStr += POSTHEADER_SUFFIX;
			        	postStr += input;
					char	*postData = postStr.ToNewCString();
					if (postData)
					{
						rv = pts->SendData(postData, (PRUint32)strlen(postData));
						delete []postData;
						postData = nsnull;
					}
			        }
			}

			SearchDataSourceCallback *callback = new SearchDataSourceCallback(mInner, source, engine);
			if (nsnull != callback)
			{
				rv = NS_OpenURL(url, NS_STATIC_CAST(nsIStreamListener *, callback));
			}
		}
		delete [] searchURL;
		searchURL = nsnull;
	}
	return(NS_OK);
}



nsresult
SearchDataSource::GetSearchEngineList()
{
        nsresult			rv = NS_OK;

	if (!mInner)
	{
		return(NS_RDF_NO_VALUE);
	}

#ifdef	XP_MAC
	// on Mac, use system's search files
	nsSpecialSystemDirectory	searchSitesDir(nsSpecialSystemDirectory::Mac_InternetSearchDirectory);
	nsFileSpec 			nativeDir(searchSitesDir);
#else
	// on other platforms, use our search files
	nsSpecialSystemDirectory	searchSitesDir(nsSpecialSystemDirectory::OS_CurrentProcessDirectory);
	// XXX we should get this from prefs?
	searchSitesDir += "res";
	searchSitesDir += "rdf";
	searchSitesDir += "search";
	nsFileSpec 			nativeDir(searchSitesDir);
#endif
	for (nsDirectoryIterator i(nativeDir); i.Exists(); i++)
	{
		const nsFileSpec	fileSpec = (const nsFileSpec &)i;
		const char		*childURL = fileSpec;
		if (!isVisible(fileSpec))	continue;
		if (childURL != nsnull)
		{
			nsAutoString	uri(childURL);
			PRInt32		len = uri.Length();
			if (len > 4)
			{
#ifdef	XP_MAC
				// be sure to resolve aliases in case we encounter one
				CInfoPBRec	cInfo;
				OSErr		err;
				PRBool		wasAliased = PR_FALSE;
				fileSpec.ResolveAlias(wasAliased);
				err = fileSpec.GetCatInfo(cInfo);
				if ((!err) && (cInfo.hFileInfo.ioFlFndrInfo.fdType == 'issp') &&
					(cInfo.hFileInfo.ioFlFndrInfo.fdCreator == 'fndf'))
#else
				nsAutoString	extension;
				if ((uri.Right(extension, 4) == 4) && (extension.EqualsIgnoreCase(".src")))
#endif
				{
#ifdef	XP_MAC
					PRInt32	separatorOffset = uri.RFind(":");
#else
					PRInt32	separatorOffset = uri.RFind("/");
#endif
					if (separatorOffset > 0)
					{
						uri.Cut(0, separatorOffset+1);
						
						nsAutoString	searchURL("engine://");
						searchURL += uri;

						char *basename = uri.ToNewCString();
						if (!basename)	continue;
						basename = nsUnescape(basename);
						if (!basename)	continue;

						nsAutoString	data("");
						rv = ReadFileContents(basename, data);
						delete [] basename;
						basename = nsnull;
						if (NS_FAILED(rv))	continue;

						nsCOMPtr<nsIRDFResource>	searchRes;
						char		*searchURI = searchURL.ToNewCString();
						if (searchURI)
						{
							if (NS_SUCCEEDED(rv = gRDFService->GetResource(searchURI, getter_AddRefs(searchRes))))
							{
								mInner->Assert(kNC_SearchRoot, kNC_Child, searchRes, PR_TRUE);

#if 0
								// Note: don't save file contents here. Save them when a sort actually begins.
								// This means we use a lot less memory as we only save datasets that we are using.

								// save file contents
								nsCOMPtr<nsIRDFLiteral>	dataLiteral;
								if (NS_SUCCEEDED(rv = gRDFService->GetLiteral(data.GetUnicode(), getter_AddRefs(dataLiteral))))
								{
									mInner->Assert(searchRes, kNC_Data, dataLiteral, PR_TRUE);
								}
#endif

								// save name of search engine (as specified in file)
								nsAutoString	nameValue;
								if (NS_SUCCEEDED(rv = GetData(data, "search", "name", nameValue)))
								{
									nsCOMPtr<nsIRDFLiteral>	nameLiteral;
									if (NS_SUCCEEDED(rv = gRDFService->GetLiteral(nameValue.GetUnicode(), getter_AddRefs(nameLiteral))))
									{
										mInner->Assert(searchRes, kNC_Name, nameLiteral, PR_TRUE);
									}
								}
							}
							delete [] searchURI;
							searchURI = nsnull;
						}
						delete [] basename;
						basename = nsnull;
					}
				}
			}
		}
	}
	return(rv);
}


PRBool
SearchDataSource::isVisible(const nsNativeFileSpec& file)
{
	PRBool		isVisible = PR_TRUE;

#ifdef	XP_MAC
	CInfoPBRec	cInfo;
	OSErr		err;

	nsFileSpec	fileSpec(file);
	if (!(err = fileSpec.GetCatInfo(cInfo)))
	{
		if (cInfo.hFileInfo.ioFlFndrInfo.fdFlags & kIsInvisible)
		{
			isVisible = PR_FALSE;
		}
	}
#else
	char		*basename = file.GetLeafName();
	if (nsnull != basename)
	{
		if ((!strcmp(basename, ".")) || (!strcmp(basename, "..")))
		{
			isVisible = PR_FALSE;
		}
		nsCRT::free(basename);
	}
#endif

	return(isVisible);
}



nsresult
SearchDataSource::ReadFileContents(char *basename, nsString& sourceContents)
{
	nsresult			rv = NS_OK;

#ifdef	XP_MAC
	nsSpecialSystemDirectory	searchEngine(nsSpecialSystemDirectory::Mac_InternetSearchDirectory);
#else
	nsSpecialSystemDirectory	searchEngine(nsSpecialSystemDirectory::OS_CurrentProcessDirectory);
	// XXX we should get this from prefs.
	searchEngine += "res";
	searchEngine += "rdf";
	searchEngine += "search";
#endif
	searchEngine += basename;

#ifdef	XP_MAC
	// be sure to resolve aliases in case we encounter one
	PRBool	wasAliased = PR_FALSE;
	searchEngine.ResolveAlias(wasAliased);
#endif

	nsInputFileStream		searchFile(searchEngine);

#ifdef	XP_MAC
	if (!searchFile.is_open())
	{
		// on Mac, nsDirectoryIterator resolves aliases before returning them currently;
		// so, if we can't open the file directly, walk the directory and see if we then
		// find a match

		// on Mac, use system's search files
		nsSpecialSystemDirectory	searchSitesDir(nsSpecialSystemDirectory::Mac_InternetSearchDirectory);
		nsFileSpec 			nativeDir(searchSitesDir);
		for (nsDirectoryIterator i(nativeDir); i.Exists(); i++)
		{
			const nsFileSpec	fileSpec = (const nsFileSpec &)i;
			const char		*childURL = fileSpec;
			if (!isVisible(fileSpec))	continue;
			if (childURL != nsnull)
			{
				// be sure to resolve aliases in case we encounter one
				CInfoPBRec	cInfo;
				PRBool		wasAliased = PR_FALSE;
				fileSpec.ResolveAlias(wasAliased);
				nsAutoString	childPath(childURL);
				PRInt32		separatorOffset = childPath.RFind(":");
				if (separatorOffset > 0)
				{
						childPath.Cut(0, separatorOffset+1);
				}
				if (childPath.EqualsIgnoreCase(basename))
				{
					searchFile = fileSpec;
				}
			}
		}
	}
#endif

	if (searchFile.is_open())
	{
		nsRandomAccessInputStream	stream(searchFile);
		char				buffer[1024];
		while (!stream.eof())
		{
			stream.readline(buffer, sizeof(buffer)-1 );
			sourceContents += buffer;
			sourceContents += "\n";
		}
	}
	else
	{
		rv = NS_ERROR_FAILURE;
	}
	return(rv);
}



nsresult
SearchDataSource::GetData(nsString data, char *sectionToFind, char *attribToFind, nsString &value)
{
	nsAutoString	buffer(data);	
	nsresult	rv = NS_RDF_NO_VALUE;
	PRBool		inSection = PR_FALSE;

	while(buffer.Length() > 0)
	{
		PRInt32 eol = buffer.FindCharInSet("\r\n");
		if (eol < 0)	break;
		nsAutoString	line("");
		if (eol > 0)
		{
			buffer.Left(line, eol);
		}
		buffer.Cut(0, eol+1);
		if (line.Length() < 1)	continue;		// skip empty lines
		if (line[0] == PRUnichar('#'))	continue;	// skip comments
		line = line.Trim(" \t");
		if (inSection == PR_FALSE)
		{
			nsAutoString	section("<");
			section += sectionToFind;
			PRInt32	sectionOffset = line.Find(section, PR_TRUE);
			if (sectionOffset < 0)	continue;
			line.Cut(0, sectionOffset + section.Length() + 1);
			inSection = PR_TRUE;
			
		}
		line = line.Trim(" \t");
		PRInt32	len = line.Length();
		if (len > 0)
		{
			if (line[len-1] == PRUnichar('>'))
			{
				inSection = PR_FALSE;
				line.SetLength(len-1);
			}
		}
		PRInt32 equal = line.Find("=");
		if (equal < 0)	continue;			// skip lines with no equality
		
		nsAutoString	attrib("");
		if (equal > 0)
		{
			line.Left(attrib, equal /* - 1 */);
		}
		attrib = attrib.Trim(" \t");
		if (attrib.EqualsIgnoreCase(attribToFind))
		{
			line.Cut(0, equal+1);
			value = line.Trim(" \t");

			// strip of any enclosing quotes
			if (value[0] == PRUnichar('\"'))
			{
				value.Cut(0,1);
			}
			len = value.Length();
			if (len > 0)
			{
				if (value[len-1] == PRUnichar('\"'))
				{
					value.SetLength(len-1);
				}
			}
			rv = NS_OK;
			break;
		}
	}
	return(rv);
}



nsresult
SearchDataSource::GetInputs(nsString data, nsString text, nsString &input)
{
	nsAutoString	buffer(data);	
	nsresult	rv = NS_OK;
	PRBool		inSection = PR_FALSE;

	while(buffer.Length() > 0)
	{
		PRInt32 eol = buffer.FindCharInSet("\r\n");
		if (eol < 0)	break;
		nsAutoString	line("");
		if (eol > 0)
		{
			buffer.Left(line, eol);
		}
		buffer.Cut(0, eol+1);
		if (line.Length() < 1)	continue;		// skip empty lines
		if (line[0] == PRUnichar('#'))	continue;	// skip comments
		line = line.Trim(" \t");
		if (inSection == PR_FALSE)
		{
			nsAutoString	section("<");
			PRInt32	sectionOffset = line.Find(section, PR_TRUE);
			if (sectionOffset < 0)	continue;
			if (sectionOffset == 0)
			{
				line.Cut(0, sectionOffset + section.Length());
				inSection = PR_TRUE;
			}
		}
		PRInt32	len = line.Length();
		if (len > 0)
		{
			if (line[len-1] == PRUnichar('>'))
			{
				inSection = PR_FALSE;
				line.SetLength(len-1);
			}
		}
		if (inSection == PR_TRUE)	continue;

		// look for inputs
		if (line.Find("input", PR_TRUE) == 0)
		{
			line.Cut(0, 6);
			line = line.Trim(" \t");
			
			// first look for name attribute
			nsAutoString	nameAttrib("");

			PRInt32	nameOffset = line.Find("name", PR_TRUE);
			if (nameOffset >= 0)
			{
				PRInt32 equal = line.Find(PRUnichar('='), nameOffset);
				if (equal >= 0)
				{
					PRInt32	startQuote = line.Find(PRUnichar('\"'), equal + 1);
					if (startQuote >= 0)
					{
						PRInt32	endQuote = line.Find(PRUnichar('\"'), startQuote + 1);
						if (endQuote >= 0)
						{
							line.Mid(nameAttrib, startQuote+1, endQuote-startQuote-1);
						}
					}
					else
					{
						nameAttrib = line;
						nameAttrib.Cut(0, equal+1);
						nameAttrib = nameAttrib.Trim(" \t");
						PRInt32 space = nameAttrib.FindCharInSet(" \t");
						if (space > 0)
						{
							nameAttrib.Truncate(space);
						}
					}
				}
			}
			if (nameAttrib.Length() <= 0)	continue;

			// first look for value attribute
			nsAutoString	valueAttrib("");

			PRInt32	valueOffset = line.Find("value", PR_TRUE);
			if (valueOffset >= 0)
			{
				PRInt32 equal = line.Find(PRUnichar('='), valueOffset);
				if (equal >= 0)
				{
					PRInt32	startQuote = line.Find(PRUnichar('\"'), equal + 1);
					if (startQuote >= 0)
					{
						PRInt32	endQuote = line.Find(PRUnichar('\"'), startQuote + 1);
						if (endQuote >= 0)
						{
							line.Mid(valueAttrib, startQuote+1, endQuote-startQuote-1);
						}
					}
					else
					{
						// if value attribute's "value" isn't quoted, get the first word... ?
/*
						PRInt32 theEnd = line.FindCharInSet(" >\t", equal);
						if (theEnd >= 0)
						{
							line.Mid(valueAttrib, equal+1, theEnd-equal-1);
							valueAttrib.Trim(" \t");
						}
*/
						valueAttrib = line;
						valueAttrib.Cut(0, equal+1);
						valueAttrib = valueAttrib.Trim(" \t");
						PRInt32 space = valueAttrib.FindCharInSet(" \t>");
						if (space > 0)
						{
							valueAttrib.Truncate(space);
						}
					}
				}
			}
			else if (line.Find("user", PR_TRUE) >= 0)
			{
				valueAttrib = text;
			}
			
			// XXX should ignore if  mode=browser  is specified
			// XXX need to do this better
			if (line.RFind("mode=browser", PR_TRUE) >= 0)
				continue;

			if ((nameAttrib.Length() > 0) && (valueAttrib.Length() > 0))
			{
				if (input.Length() > 0)
				{
					input += "&";
				}
				input += nameAttrib;
				input += "=";
				input += valueAttrib;
			}
		}
	}
	return(rv);
}



nsresult
SearchDataSource::GetURL(nsIRDFResource *source, nsIRDFLiteral** aResult)
{
        nsXPIDLCString	uri;
	source->GetValue( getter_Copies(uri) );
	nsAutoString	url(uri);
	nsIRDFLiteral	*literal;
	gRDFService->GetLiteral(url.GetUnicode(), &literal);
        *aResult = literal;
        return NS_OK;
}



// Search class for Netlib callback



SearchDataSourceCallback::SearchDataSourceCallback(nsIRDFDataSource *ds, nsIRDFResource *parent, nsIRDFResource *engine)
	: mDataSource(ds),
	  mParent(parent),
	  mEngine(engine),
	  mLine(nsnull)
{
	NS_INIT_REFCNT();
	NS_ADDREF(mDataSource);
	NS_ADDREF(mParent);

	if (gRefCnt++ == 0)
	{
		nsresult rv = nsServiceManager::GetService(kRDFServiceCID,
                                                   nsIRDFService::GetIID(),
                                                   (nsISupports**) &gRDFService);

		PR_ASSERT(NS_SUCCEEDED(rv));

		gRDFService->GetResource(NC_NAMESPACE_URI "child", &kNC_Child);
		gRDFService->GetResource(NC_NAMESPACE_URI "Name", &kNC_Name);
		gRDFService->GetResource(NC_NAMESPACE_URI "data", &kNC_Data);
		gRDFService->GetResource(NC_NAMESPACE_URI "Relevance", &kNC_Relevance);
		gRDFService->GetResource(NC_NAMESPACE_URI "Relevance?sort=true", &kNC_RelevanceSort);
		gRDFService->GetResource(NC_NAMESPACE_URI "Site", &kNC_Site);
		gRDFService->GetResource(NC_NAMESPACE_URI "Engine", &kNC_Engine);
		gRDFService->GetResource(NC_NAMESPACE_URI "loading", &kNC_loading);
	}
}



SearchDataSourceCallback::~SearchDataSourceCallback()
{
	NS_IF_RELEASE(mDataSource);
	NS_IF_RELEASE(mParent);
	NS_IF_RELEASE(mEngine);

	if (mLine)
	{
		delete [] mLine;
		mLine = nsnull;
	}

	if (--gRefCnt == 0)
	{
		NS_RELEASE(kNC_Child);
		NS_RELEASE(kNC_Name);
		NS_RELEASE(kNC_Data);
		NS_RELEASE(kNC_Relevance);
		NS_RELEASE(kNC_RelevanceSort);
		NS_RELEASE(kNC_Site);
		NS_RELEASE(kNC_Engine);
		NS_RELEASE(kNC_loading);
	}
}



// stream observer methods



NS_IMETHODIMP
SearchDataSourceCallback::OnStartBinding(nsIURI *aURL, const char *aContentType)
{
	nsAutoString		trueStr("true");
	nsIRDFLiteral		*literal = nsnull;
	nsresult		rv;

#ifdef	DEBUG
	printf("SearchDataSourceCallback::OnStartBinding entered.\n");
#endif

	if (NS_SUCCEEDED(rv = gRDFService->GetLiteral(trueStr.GetUnicode(), &literal)))
	{
		mDataSource->Assert(mParent, kNC_loading, literal, PR_TRUE);
		NS_RELEASE(literal);
	}
	return(NS_OK);
}



NS_IMETHODIMP
SearchDataSourceCallback::OnProgress(nsIURI* aURL, PRUint32 aProgress, PRUint32 aProgressMax) 
{
	return(NS_OK);
}



NS_IMETHODIMP
SearchDataSourceCallback::OnStatus(nsIURI* aURL, const PRUnichar* aMsg)
{
	return(NS_OK);
}



NS_IMETHODIMP
SearchDataSourceCallback::OnStopBinding(nsIURI* aURL, nsresult aStatus, const PRUnichar* aMsg) 
{
	nsAutoString		trueStr("true");
	nsIRDFLiteral		*literal = nsnull;
	nsresult		rv;

	if (NS_SUCCEEDED(rv = gRDFService->GetLiteral(trueStr.GetUnicode(), &literal)))
	{
		mDataSource->Unassert(mParent, kNC_loading, literal);
		NS_RELEASE(literal);
	}

	if (!mLine)
	{
#ifdef	DEBUG
		printf(" *** SearchDataSourceCallback::OnStopBinding:  no data.\n\n");
#endif

		return(NS_OK);
	}

	nsAutoString	htmlResults(mLine);
	delete [] mLine;
	mLine = nsnull;

	// get data out of graph
	nsAutoString		data("");
	nsCOMPtr<nsIRDFNode>	dataNode;
	if (NS_FAILED(rv = mDataSource->GetTarget(mEngine, kNC_Data, PR_TRUE, getter_AddRefs(dataNode))))
	{
		return(rv);
	}
	nsCOMPtr<nsIRDFLiteral>	dataLiteral = do_QueryInterface(dataNode);
	if (!dataLiteral)	return(NS_ERROR_NULL_POINTER);

	PRUnichar	*dataUni = nsnull;
	if (NS_FAILED(rv = dataLiteral->GetValue(&dataUni)))
		return(rv);
	if (!dataUni)	return(NS_ERROR_NULL_POINTER);
	data = dataUni;
	if (data.Length() < 1)
		return(rv);

	nsAutoString	resultListStartStr(""), resultListEndStr("");
	nsAutoString	resultItemStartStr(""), resultItemEndStr("");
	nsAutoString	relevanceStartStr(""), relevanceEndStr("");

	SearchDataSource::GetData(data, "interpret", "resultListStart", resultListStartStr);
	SearchDataSource::GetData(data, "interpret", "resultListEnd", resultListEndStr);
	SearchDataSource::GetData(data, "interpret", "resultItemStart", resultItemStartStr);
	SearchDataSource::GetData(data, "interpret", "resultItemEnd", resultItemEndStr);
	SearchDataSource::GetData(data, "interpret", "relevanceStart", relevanceStartStr);
	SearchDataSource::GetData(data, "interpret", "relevanceEnd", relevanceEndStr);

#ifdef	DEBUG
	char *cStr;
	cStr = resultListStartStr.ToNewCString();
	if (cStr)
	{
		printf("resultListStart: '%s'\n", cStr);
		delete [] cStr;
		cStr = nsnull;
	}
	cStr = resultListEndStr.ToNewCString();
	if (cStr)
	{
		printf("resultListEnd: '%s'\n", cStr);
		delete [] cStr;
		cStr = nsnull;
	}
	cStr = resultItemStartStr.ToNewCString();
	if (cStr)
	{
		printf("resultItemStart: '%s'\n", cStr);
		delete [] cStr;
		cStr = nsnull;
	}
	cStr = resultItemEndStr.ToNewCString();
	if (cStr)
	{
		printf("resultItemEnd: '%s'\n", cStr);
		delete [] cStr;
		cStr = nsnull;
	}
	cStr = relevanceStartStr.ToNewCString();
	if (cStr)
	{
		printf("relevanceStart: '%s'\n", cStr);
		delete [] cStr;
		cStr = nsnull;
	}
	cStr = relevanceEndStr.ToNewCString();
	if (cStr)
	{
		printf("relevanceEnd: '%s'\n", cStr);
		delete [] cStr;
		cStr = nsnull;
	}
#endif

	if (resultListStartStr.Length() > 0)
	{
		PRInt32	resultListStart = htmlResults.Find(resultListStartStr, PR_TRUE);
		if (resultListStart >= 0)
		{
			htmlResults.Cut(0, resultListStart + resultListStartStr.Length());
		}
	}
	if (resultListEndStr.Length() > 0)
	{
		PRInt32	resultListEnd = htmlResults.Find(resultListEndStr, PR_TRUE);
		if (resultListEnd >= 0)
		{
			htmlResults.Truncate(resultListEnd);
		}
	}

	// if resultItemEndStr is not specified, try making it the same as resultItemStartStr
	if (resultItemEndStr.Length() < 1)
	{
		resultItemEndStr = resultItemStartStr;
	}

	PRInt32	resultItemStart;
	while((resultItemStart = htmlResults.Find(resultItemStartStr, PR_TRUE)) >= 0)
	{
		htmlResults.Cut(0, resultItemStart + resultItemStartStr.Length());

		PRInt32	resultItemEnd = htmlResults.Find(resultItemEndStr, PR_TRUE );
		if (resultItemEnd < 0)
		{
			resultItemEnd = htmlResults.Length()-1;
		}

		nsAutoString	resultItem("");
		htmlResults.Left(resultItem, resultItemEnd);

		if (resultItem.Length() < 1)	break;
		htmlResults.Cut(0, resultItemEnd + resultItemEndStr.Length());

#if 0
		char	*results = resultItem.ToNewCString();
		if (results)
		{
			printf("-----\nResult: %s\n", results);
			delete [] results;
			results = nsnull;
		}
#endif

		// look for href
		PRInt32	hrefOffset = resultItem.Find("<A HREF=", PR_TRUE);
		if (hrefOffset < 0)
		{
			continue;
		}

		nsAutoString	hrefStr("");
		PRInt32 quoteStartOffset = resultItem.FindCharInSet("\"\'>", hrefOffset);
		if (quoteStartOffset < hrefOffset)	continue;
		PRInt32		quoteEndOffset;
		if (resultItem[quoteStartOffset] == PRUnichar('>'))
		{
			// handle case where HREF isn't quoted
			quoteEndOffset = quoteStartOffset;
			quoteStartOffset = hrefOffset + strlen("<A HREF=") -1;
		}
		else
		{
			quoteEndOffset = resultItem.FindCharInSet("\"\'", quoteStartOffset + 1);
			if (quoteEndOffset < hrefOffset)	continue;
		}
		resultItem.Mid(hrefStr, quoteStartOffset + 1, quoteEndOffset - quoteStartOffset - 1);
		if (hrefStr.Length() < 1)	continue;

		// check to see if this needs to be an absolute URL
		if (hrefStr[0] == PRUnichar('/'))
		{
			const char	*host = nsnull, *protocol = nsnull;
			aURL->GetHost(&host);
			aURL->GetProtocol(&protocol);
			if (host && protocol)
			{
				nsAutoString	temp;
				temp += protocol;
				temp += "://";
				temp += host;
				temp += hrefStr;

				hrefStr = temp;
			}
		}
		
		char	*href = hrefStr.ToNewCString();
		if (!href)	continue;

		nsAutoString	site(href);

#ifdef	DEBUG
		printf("HREF: '%s'\n", href);
#endif

		nsCOMPtr<nsIRDFResource>	res;
		rv = gRDFService->GetResource(href, getter_AddRefs(res));
		delete [] href;
		href = nsnull;
		if (NS_FAILED(rv))	continue;

		// look for Site (if it isn't already set)
		nsCOMPtr<nsIRDFNode>		oldSiteRes = nsnull;
		mDataSource->GetTarget(res, kNC_Site, PR_TRUE, getter_AddRefs(oldSiteRes));
		if (!oldSiteRes)
		{
			PRInt32	protocolOffset = site.FindCharInSet(":");
			if (protocolOffset >= 0)
			{
				site.Cut(0, protocolOffset+1);
				while (site[0] == PRUnichar('/'))
				{
					site.Cut(0, 1);
				}
				PRInt32	slashOffset = site.FindCharInSet("/");
				if (slashOffset >= 0)
				{
					site.Truncate(slashOffset);
				}
				if (site.Length() > 0)
				{
					const PRUnichar	*siteUni = site.GetUnicode();
					if (siteUni)
					{
						nsCOMPtr<nsIRDFLiteral>	siteLiteral;
						if (NS_SUCCEEDED(rv = gRDFService->GetLiteral(siteUni, getter_AddRefs(siteLiteral))))
						{
							if (siteLiteral)
							{
								mDataSource->Assert(res, kNC_Site, siteLiteral, PR_TRUE);
							}
						}
					}
				}
			}
		}

		// look for name
		PRInt32	anchorEnd = resultItem.FindCharInSet(">", quoteEndOffset);
		if (anchorEnd < quoteEndOffset)	continue;
		PRInt32	anchorStop = resultItem.Find("</A>", PR_TRUE);
		if (anchorStop < anchorEnd)	continue;
		
		nsAutoString	nameStr;
		resultItem.Mid(nameStr, anchorEnd + 1, anchorStop - anchorEnd - 1);

		// munge any "&quot;" in name
		PRInt32	quotOffset;
		while ((quotOffset = nameStr.Find("&quot;", PR_TRUE)) >= 0)
		{
			nameStr.Cut(quotOffset, strlen("&quot;"));
			nameStr.Insert(PRUnichar('\"'), quotOffset);
		}
		
		// munge any "&amp;" in name
		PRInt32	ampOffset;
		while ((ampOffset = nameStr.Find("&amp;", PR_TRUE)) >= 0)
		{
			nameStr.Cut(ampOffset, strlen("&amp;"));
			nameStr.Insert(PRUnichar('&'), ampOffset);
		}
		
		// munge any "&nbsp;" in name
		PRInt32	nbspOffset;
		while ((nbspOffset = nameStr.Find("&nbsp;", PR_TRUE)) >= 0)
		{
			nameStr.Cut(nbspOffset, strlen("&nbsp;"));
			nameStr.Insert(PRUnichar(' '), nbspOffset);
		}

		// munge any "&lt;" in name
		PRInt32	ltOffset;
		while ((ltOffset = nameStr.Find("&lt;", PR_TRUE)) >= 0)
		{
			nameStr.Cut(ltOffset, strlen("&lt;"));
			nameStr.Insert(PRUnichar('<'), ltOffset);
		}

		// munge any "&gt;" in name
		PRInt32	gtOffset;
		while ((gtOffset = nameStr.Find("&gt;", PR_TRUE)) >= 0)
		{
			nameStr.Cut(gtOffset, strlen("&gt;"));
			nameStr.Insert(PRUnichar('>'), gtOffset);
		}
		
		// munge out anything inside of HTML "<" / ">" tags
		PRInt32 tagStartOffset;
		while ((tagStartOffset = nameStr.FindCharInSet("<")) >= 0)
		{
			PRInt32	tagEndOffset = nameStr.FindCharInSet(">", tagStartOffset);
			if (tagEndOffset <= tagStartOffset)	break;
			nameStr.Cut(tagStartOffset, tagEndOffset - tagStartOffset + 1);
		}

		// cut out any CRs or LFs
		PRInt32	eolOffset;
		while ((eolOffset = nameStr.FindCharInSet("\n\r")) >= 0)
		{
			nameStr.Cut(eolOffset, 1);
		}
		// and trim name
		nameStr = nameStr.Trim(" \t");

		// look for Name (if it isn't already set)
		nsCOMPtr<nsIRDFNode>		oldNameRes = nsnull;
		mDataSource->GetTarget(res, kNC_Name, PR_TRUE, getter_AddRefs(oldNameRes));
		if (!oldNameRes)
		{
			if (nameStr.Length() > 0)
			{
				const PRUnichar	*nameUni = nameStr.GetUnicode();
				if (nameUni)
				{
					nsCOMPtr<nsIRDFLiteral>	nameLiteral;
					if (NS_SUCCEEDED(rv = gRDFService->GetLiteral(nameUni, getter_AddRefs(nameLiteral))))
					{
						if (nameLiteral)
						{
							mDataSource->Assert(res, kNC_Name, nameLiteral, PR_TRUE);
						}
					}
				}
			}
		}

		// look for relevance
		nsAutoString	relItem("-");
		PRInt32		relStart;
		if ((relStart = resultItem.Find(relevanceStartStr /* , PR_TRUE */)) >= 0)
		{
			PRInt32	relEnd = resultItem.Find(relevanceEndStr /* , PR_TRUE */);
			if (relEnd > relStart)
			{
				resultItem.Mid(relItem, relStart + relevanceStartStr.Length(),
					relEnd - relStart - relevanceStartStr.Length());
			}
		}

		// look for Relevance (if it isn't already set)
		nsCOMPtr<nsIRDFNode>		oldRelRes = nsnull;
		mDataSource->GetTarget(res, kNC_Relevance, PR_TRUE, getter_AddRefs(oldRelRes));
		if (!oldRelRes)
		{
			if (relItem.Length() > 0)
			{
				// save real relevance
				const PRUnichar	*relUni = relItem.GetUnicode();
				if (relUni)
				{
					nsCOMPtr<nsIRDFLiteral>	relLiteral;
					if (NS_SUCCEEDED(rv = gRDFService->GetLiteral(relUni, getter_AddRefs(relLiteral))))
					{
						if (relLiteral)
						{
							mDataSource->Assert(res, kNC_Relevance, relLiteral, PR_TRUE);
						}
					}
				}

				// If its a percentage, remove "%", left-pad with "0"s and set special sorting value
				if (relItem[relItem.Length()-1] == PRUnichar('%'))
				{
					relItem.Cut(relItem.Length()-1, 1);

					nsAutoString	zero("000");
					if (relItem.Length() < 3)
					{
						relItem.Insert(zero, 0, 3-relItem.Length()); 
					}

					const PRUnichar	*relSortUni = relItem.GetUnicode();
					if (relSortUni)
					{
						nsCOMPtr<nsIRDFLiteral>	relSortLiteral;
						if (NS_SUCCEEDED(rv = gRDFService->GetLiteral(relSortUni, getter_AddRefs(relSortLiteral))))
						{
							if (relSortLiteral)
							{
								mDataSource->Assert(res, kNC_RelevanceSort, relSortLiteral, PR_TRUE);
							}
						}
					}
				}
			}
		}

		// set reference to engine this came from (if it isn't already set)
		nsCOMPtr<nsIRDFNode>		oldEngineRes = nsnull;
		mDataSource->GetTarget(res, kNC_Engine, PR_TRUE, getter_AddRefs(oldEngineRes));
		if (!oldEngineRes)
		{
			nsAutoString	engineStr;
			if (NS_SUCCEEDED(rv = SearchDataSource::GetData(data, "search", "name", engineStr)))
			{
				const PRUnichar		*engineUni = engineStr.GetUnicode();
				nsCOMPtr<nsIRDFLiteral>	engineLiteral;
				if (NS_SUCCEEDED(rv = gRDFService->GetLiteral(engineUni, getter_AddRefs(engineLiteral))))
				{
					if (engineLiteral)
					{
						mDataSource->Assert(res, kNC_Engine, engineLiteral, PR_TRUE);
					}
				}
			}
		}

		// Note: always add in parent-child relationship last!  (if it isn't already set)
		PRBool		parentHasChildFlag = PR_FALSE;
		mDataSource->HasAssertion(mParent, kNC_Child, res, PR_TRUE, &parentHasChildFlag);
		if (parentHasChildFlag == PR_FALSE)
		{
			rv = mDataSource->Assert(mParent, kNC_Child, res, PR_TRUE);
		}
	}
	return(NS_OK);
}



// stream listener methods



NS_IMPL_ISUPPORTS(SearchDataSourceCallback, nsIRDFSearchDataSourceCallback::GetIID());



NS_IMETHODIMP
SearchDataSourceCallback::GetBindInfo(nsIURI* aURL, nsStreamBindingInfo* aInfo)
{
	return(NS_OK);
}



NS_IMETHODIMP
SearchDataSourceCallback::OnDataAvailable(nsIURI* aURL, nsIInputStream *aIStream, PRUint32 aLength)
{
	nsresult	rv = NS_OK;

	if (aLength > 0)
	{
		nsAutoString	line;
		if (mLine)
		{
			line += mLine;
			delete	[]mLine;
			mLine = nsnull;
		}

		char	buffer[257];
		while (aLength > 0)
		{
			PRUint32	count=0, numBytes = (aLength > sizeof(buffer)-1 ? sizeof(buffer)-1 : aLength);
			if (NS_FAILED(rv = aIStream->Read(buffer, numBytes, &count)))
			{
				printf("Search datasource read failure.\n");
				break;
			}
			if (numBytes != count)
			{
				printf("Search datasource read # of bytes failure.\n");
				break;
			}
			buffer[count] = '\0';
			line += buffer;
			aLength -= count;
		}
		mLine = line.ToNewCString();
	}
	return(rv);
}
