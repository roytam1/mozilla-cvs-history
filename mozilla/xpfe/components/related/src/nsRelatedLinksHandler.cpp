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

  Implementation for a Related Links RDF data store.

 */

#include "nsCOMPtr.h"
#include "nsEnumeratorUtils.h"
#include "nsIGenericFactory.h"
#include "nsIInputStream.h"
#include "nsIRDFDataSource.h"
#include "nsIRDFNode.h"
#include "nsIRDFObserver.h"
#include "nsIRDFPurgeableDataSource.h"
#include "nsIRDFService.h"
#include "nsIRelatedLinksHandler.h"
#include "nsIServiceManager.h"
#include "nsIStreamListener.h"
#include "nsIURL.h"

#include "nsNeckoUtil.h"
#include "nsIBufferInputStream.h"

#include "nsRDFCID.h"
#include "nsString.h"
#include "nsVoidArray.h"
#include "nsXPIDLString.h"
#include "nscore.h"
#include "plhash.h"
#include "plstr.h"
#include "prio.h"
#include "prmem.h"
#include "prprf.h"
#include "rdf.h"
#include "xp_core.h"
#include <ctype.h> // for toupper()
#include <stdio.h>

#include "nsICharsetConverterManager.h"
#include "nsICharsetAlias.h"

static NS_DEFINE_CID(kRDFServiceCID,              NS_RDFSERVICE_CID);
static NS_DEFINE_CID(kRDFInMemoryDataSourceCID,   NS_RDFINMEMORYDATASOURCE_CID);
static NS_DEFINE_CID(kRelatedLinksHandlerCID,     NS_RELATEDLINKSHANDLER_CID);
static NS_DEFINE_CID(kComponentManagerCID,        NS_COMPONENTMANAGER_CID);
static NS_DEFINE_CID(kGenericFactoryCID,          NS_GENERICFACTORY_CID);
static NS_DEFINE_CID(kCharsetConverterManagerCID, NS_ICHARSETCONVERTERMANAGER_CID);

static const char kURINC_RelatedLinksRoot[] = "NC:RelatedLinks";


////////////////////////////////////////////////////////////////////////
// RelatedLinksStreamListener
//
//   Until Netcenter produces valid RDF/XML, we'll use this kludge to
//   parse the crap they send us.
//

class RelatedLinksStreamListener : public nsIStreamListener
{
private:
	nsCOMPtr<nsIRDFDataSource> 	mDataSource;
	nsCOMPtr<nsIUnicodeDecoder>	mUnicodeDecoder;
	nsVoidArray			mParentArray;

	// pseudo-constants
	static PRInt32			gRefCnt;
	static nsIRDFService		*gRDFService;
	static nsIRDFResource		*kNC_Child;
	static nsIRDFResource		*kNC_Name;
	static nsIRDFResource		*kNC_loading;
	static nsIRDFResource		*kNC_RelatedLinksRoot;
	static nsIRDFResource		*kNC_BookmarkSeparator;
	static nsIRDFResource		*kRDF_type;

	nsAutoString			mBuffer;

public:

	 NS_DECL_ISUPPORTS

			RelatedLinksStreamListener(nsIRDFDataSource *ds);
	 virtual	~RelatedLinksStreamListener();

	 NS_METHOD	Init();
	 NS_METHOD	CreateAnonymousResource(const nsString& aPrefixURI, nsCOMPtr<nsIRDFResource>* aResult);

	// nsIStreamObserver
	NS_DECL_NSISTREAMOBSERVER

	// nsIStreamListener
	NS_DECL_NSISTREAMLISTENER
};



PRInt32			RelatedLinksStreamListener::gRefCnt;
nsIRDFService		*RelatedLinksStreamListener::gRDFService;

nsIRDFResource		*RelatedLinksStreamListener::kNC_Child;
nsIRDFResource		*RelatedLinksStreamListener::kNC_Name;
nsIRDFResource		*RelatedLinksStreamListener::kNC_loading;
nsIRDFResource		*RelatedLinksStreamListener::kNC_RelatedLinksRoot;
nsIRDFResource		*RelatedLinksStreamListener::kNC_BookmarkSeparator;
nsIRDFResource		*RelatedLinksStreamListener::kRDF_type;



////////////////////////////////////////////////////////////////////////



nsresult
NS_NewRelatedLinksStreamListener(nsIRDFDataSource* aDataSource,
				 nsIStreamListener** aResult)
{
	 RelatedLinksStreamListener* result =
		 new RelatedLinksStreamListener(aDataSource);

	 if (! result)
		 return NS_ERROR_OUT_OF_MEMORY;

	 nsresult rv = result->Init();
	 if (NS_FAILED(rv)) {
		 delete result;
		 return rv;
	 }

	 NS_ADDREF(result);
	 *aResult = result;
	 return NS_OK;
}



RelatedLinksStreamListener::RelatedLinksStreamListener(nsIRDFDataSource *aDataSource)
	 : mDataSource(dont_QueryInterface(aDataSource))
{
	 NS_INIT_REFCNT();
}



RelatedLinksStreamListener::~RelatedLinksStreamListener()
{
	 if (--gRefCnt == 0)
	 {
		 NS_RELEASE(kNC_Child);
		 NS_RELEASE(kNC_Name);
		 NS_RELEASE(kNC_loading);
		 NS_RELEASE(kNC_BookmarkSeparator);
		 NS_RELEASE(kRDF_type);
		 NS_RELEASE(kNC_RelatedLinksRoot);

		 nsServiceManager::ReleaseService(kRDFServiceCID, gRDFService);
	 }
}



NS_METHOD
RelatedLinksStreamListener::Init()
{
	if (gRefCnt++ == 0)
	{
		nsresult rv = nsServiceManager::GetService(kRDFServiceCID,
				    nsIRDFService::GetIID(),
				    (nsISupports**) &gRDFService);

		NS_ASSERTION(NS_SUCCEEDED(rv), "unable to get RDF service");
		if (NS_FAILED(rv))
			return(rv);

		nsICharsetConverterManager	*charsetConv = nsnull;

		rv = nsServiceManager::GetService(kCharsetConverterManagerCID, 
				nsCOMTypeInfo<nsICharsetConverterManager>::GetIID(), 
				(nsISupports**)&charsetConv);
		if (NS_SUCCEEDED(rv) && (charsetConv))
		{
			nsString	utf8("UTF-8");
			rv = charsetConv->GetUnicodeDecoder(&utf8,
				getter_AddRefs(mUnicodeDecoder));
			NS_RELEASE(charsetConv);
		}

		gRDFService->GetResource(NC_NAMESPACE_URI "child", &kNC_Child);
		gRDFService->GetResource(NC_NAMESPACE_URI "Name",  &kNC_Name);
		gRDFService->GetResource(NC_NAMESPACE_URI "loading", &kNC_loading);
		gRDFService->GetResource(NC_NAMESPACE_URI "BookmarkSeparator", &kNC_BookmarkSeparator);
		gRDFService->GetResource(RDF_NAMESPACE_URI "type", &kRDF_type);
		gRDFService->GetResource(kURINC_RelatedLinksRoot, &kNC_RelatedLinksRoot);
	 }

	 mParentArray.AppendElement(kNC_RelatedLinksRoot);
	 return(NS_OK);
}



NS_METHOD
RelatedLinksStreamListener::CreateAnonymousResource(const nsString& aPrefixURI,
						     nsCOMPtr<nsIRDFResource>* aResult)
{
	 static PRInt32 gCounter;

	 nsAutoString uri(aPrefixURI);
	 uri.Append('#');
	 uri.Append(gCounter++, 10);

	 return gRDFService->GetUnicodeResource(uri.GetUnicode(), getter_AddRefs(*aResult));
}



// nsISupports interface
NS_IMPL_ISUPPORTS(RelatedLinksStreamListener, nsIStreamListener::GetIID());



// stream observer methods



NS_IMETHODIMP
RelatedLinksStreamListener::OnStartRequest(nsIChannel* channel, nsISupports *ctxt)
{
	 nsAutoString		trueStr("true");
	 nsIRDFLiteral		*literal = nsnull;
	 nsresult		rv;
	 if (NS_SUCCEEDED(rv = gRDFService->GetLiteral(trueStr.GetUnicode(), &literal)))
	 {
		 mDataSource->Assert(kNC_RelatedLinksRoot, kNC_loading, literal, PR_TRUE);
		 NS_RELEASE(literal);
	 }
	 return(NS_OK);
}



NS_IMETHODIMP
RelatedLinksStreamListener::OnStopRequest(nsIChannel* channel, nsISupports *ctxt,
					nsresult status, const PRUnichar *errorMsg) 
{
	 nsAutoString		trueStr("true");
	 nsIRDFLiteral		*literal = nsnull;
	 nsresult		rv;
	if (NS_SUCCEEDED(rv = gRDFService->GetLiteral(trueStr.GetUnicode(), &literal)))
	{
		mDataSource->Unassert(kNC_RelatedLinksRoot, kNC_loading, literal);
		NS_RELEASE(literal);
	}
	return(NS_OK);
}



// stream listener methods



NS_IMETHODIMP
RelatedLinksStreamListener::OnDataAvailable(nsIChannel* channel, nsISupports *ctxt,
		nsIInputStream *aIStream, PRUint32 sourceOffset, PRUint32 aLength)
{
	nsresult	rv = NS_OK;

	if (aLength < 1)	return(rv);

	PRUint32	count;
	char		*buffer = new char[ aLength ];
	if (!buffer)	return(NS_ERROR_OUT_OF_MEMORY);

	if (NS_FAILED(rv = aIStream->Read(buffer, aLength, &count)) || count == 0)
	{
#ifdef	DEBUG
		printf("Related Links datasource read failure.\n");
#endif
		delete []buffer;
		return(rv);
	}
	if (count != aLength)
	{
#ifdef	DEBUG
		printf("Related Links datasource read # of bytes failure.\n");
#endif
		delete []buffer;
		return(NS_ERROR_UNEXPECTED);
	}

	if (mUnicodeDecoder)
	{
		char			*aBuffer = buffer;
		PRInt32			unicharBufLen = 0;
		mUnicodeDecoder->GetMaxLength(aBuffer, aLength, &unicharBufLen);
		PRUnichar		*unichars = new PRUnichar [ unicharBufLen+1 ];
		do
		{
			PRInt32		srcLength = aLength;
			PRInt32		unicharLength = unicharBufLen;
			rv = mUnicodeDecoder->Convert(aBuffer, &srcLength, unichars, &unicharLength);
			unichars[unicharLength]=0;  //add this since the unicode converters can't be trusted to do so.

			// Move the nsParser.cpp 00 -> space hack to here so it won't break UCS2 file

			// Hack Start
			for(PRInt32 i=0;i<unicharLength;i++)
				if(0x0000 == unichars[i])	unichars[i] = 0x0020;
			// Hack End

			mBuffer.Append(unichars, unicharLength);
			// if we failed, we consume one byte by replace it with U+FFFD
			// and try conversion again.
			if(NS_FAILED(rv))
			{
				mUnicodeDecoder->Reset();
				mBuffer.Append( (PRUnichar)0xFFFD);
				if(((PRUint32) (srcLength + 1)) > aLength)
					srcLength = aLength;
				else 
					srcLength++;
				aBuffer += srcLength;
				aLength -= srcLength;
			}
		} while (NS_FAILED(rv) && (aLength > 0));
		delete [] unichars;
		unichars = nsnull;
	}
	else
	{
		mBuffer.Append(buffer, aLength);
	}
	delete [] buffer;
	buffer = nsnull;

	// parse out any available lines
	while (PR_TRUE)
	{
		PRInt32 eol = mBuffer.FindCharInSet("\r\n");
		if (eol < 0)
		{
			break;
		}

		nsAutoString	oneLiner("");
		if (eol >= 0)
		{
			mBuffer.Left(oneLiner, eol);
			mBuffer.Cut(0, eol+1);
		}
		if (oneLiner.Length() < 1)	break;


//		printf("RL: '%s'\n", oneLiner.ToNewCString());


		// yes, very primitive RDF parsing follows

		nsAutoString	child(""), title("");

		// get href
		PRInt32 theStart = oneLiner.Find("<child href=\"");
		if (theStart == 0)
		{
			// get child href
			theStart += PL_strlen("<child href=\"");
			oneLiner.Cut(0, theStart);
			PRInt32 theEnd = oneLiner.Find("\"");
			if (theEnd > 0)
			{
				oneLiner.Mid(child, 0, theEnd);
			}
			// get child name
			theStart = oneLiner.Find("name=\"");
			if (theStart >= 0)
			{
				theStart += PL_strlen("name=\"");
				oneLiner.Cut(0, theStart);
				theEnd = oneLiner.Find("\"");
				if (theEnd > 0)
				{
					oneLiner.Mid(title, 0, theEnd);
				}
			}
		}
		// check for separator
		else if ((theStart = oneLiner.Find("<child instanceOf=\"Separator1\"/>")) == 0)
		{
			nsCOMPtr<nsIRDFResource>	newSeparator;
			nsAutoString			rlRoot(kURINC_RelatedLinksRoot);
			if (NS_SUCCEEDED(rv = CreateAnonymousResource(rlRoot, &newSeparator)))
			{
				mDataSource->Assert(newSeparator, kRDF_type, kNC_BookmarkSeparator, PR_TRUE);

				PRInt32		numParents = mParentArray.Count();
				if (numParents > 0)
				{
					nsIRDFResource	*parent = (nsIRDFResource *)(mParentArray.ElementAt(numParents - 1));
					mDataSource->Assert(parent, kNC_Child, newSeparator, PR_TRUE);
				}
			}
		}
		else
		{
			theStart = oneLiner.Find("<Topic name=\"");
			if (theStart == 0)
			{
				// get topic name
				theStart += PL_strlen("<Topic name=\"");
				oneLiner.Cut(0, theStart);
				PRInt32 theEnd = oneLiner.Find("\"");
				if (theEnd > 0)
				{
					oneLiner.Mid(title, 0, theEnd);
				}

				nsCOMPtr<nsIRDFResource>	newTopic;
				nsAutoString			rlRoot(kURINC_RelatedLinksRoot);
				if (NS_SUCCEEDED(rv = CreateAnonymousResource(rlRoot, &newTopic)))
				{
					if (title.Length() > 0)
					{
						const PRUnichar		*titleName = title.GetUnicode();
						if (nsnull != titleName)
						{
							nsCOMPtr<nsIRDFLiteral> nameLiteral;
							if (NS_SUCCEEDED(rv = gRDFService->GetLiteral(titleName, getter_AddRefs(nameLiteral))))
							{
								mDataSource->Assert(newTopic, kNC_Name, nameLiteral, PR_TRUE);
							}
						}
					}
					PRInt32		numParents = mParentArray.Count();
					if (numParents > 0)
					{
						nsIRDFResource	*parent = (nsIRDFResource *)(mParentArray.ElementAt(numParents - 1));
						mDataSource->Assert(parent, kNC_Child, newTopic, PR_TRUE);
					}
					mParentArray.AppendElement(newTopic);
				}

			}
			else
			{
				theStart = oneLiner.Find("</Topic>");
				if (theStart == 0)
				{
					PRInt32		numParents = mParentArray.Count();
					if (numParents > 0)
					{
						mParentArray.RemoveElementAt(numParents - 1);
					}
				}
			}
		}

		if (child.Length() > 0)
		{
			const PRUnichar	*url = child.GetUnicode();
			if (nsnull != url)
			{
				nsCOMPtr<nsIRDFResource>	relatedLinksChild;
				PRInt32		numParents = mParentArray.Count();
				if (numParents > 1)
				{
					nsAutoString	rlRoot(kURINC_RelatedLinksRoot);
					rv = CreateAnonymousResource(rlRoot, &relatedLinksChild);
				}
				else
				{
					rv = gRDFService->GetUnicodeResource(url, getter_AddRefs(relatedLinksChild));
				}
				if (NS_SUCCEEDED(rv))
				{					
					if (title.Length() > 0)
					{
						const PRUnichar	*name = title.GetUnicode();
						if (nsnull != name)
						{
							nsCOMPtr<nsIRDFLiteral>	nameLiteral;
							if (NS_SUCCEEDED(rv = gRDFService->GetLiteral(name, getter_AddRefs(nameLiteral))))
							{
								mDataSource->Assert(relatedLinksChild, kNC_Name, nameLiteral, PR_TRUE);
							}
						}
					}
					if (numParents > 0)
					{
						nsIRDFResource	*parent = (nsIRDFResource *)(mParentArray.ElementAt(numParents - 1));
						mDataSource->Assert(parent, kNC_Child, relatedLinksChild, PR_TRUE);
					}
				}
			}
		}
	}
	return(rv);
}



////////////////////////////////////////////////////////////////////////
// RelatedLinksHanlderImpl



class RelatedLinksHandlerImpl : public nsIRelatedLinksHandler,
				public nsIRDFDataSource
{
private:
	char			*mURI;
	char			*mRelatedLinksURL;
	PRBool			mPerformQuery;

   // pseudo-constants
	static PRInt32		gRefCnt;
	static nsIRDFService    *gRDFService;
	static nsIRDFResource	*kNC_RelatedLinksRoot;
	static nsIRDFResource	*kNC_Child;
	static nsIRDFResource	*kNC_Name;
	static nsIRDFResource	*kRDF_type;

	nsCOMPtr<nsIRDFDataSource> mInner;

				RelatedLinksHandlerImpl();
	virtual		~RelatedLinksHandlerImpl();
	nsresult	Init();

	friend NS_IMETHODIMP
	NS_NewRelatedLinksHandler(nsISupports* aOuter, REFNSIID aIID, void** aResult);

public:

	NS_DECL_ISUPPORTS

	// nsIRelatedLinksHandler methods
	NS_DECL_NSIRELATEDLINKSHANDLER

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
	NS_IMETHOD Change(nsIRDFResource* aSource,
					  nsIRDFResource* aProperty,
					  nsIRDFNode* aOldTarget,
					  nsIRDFNode* aNewTarget);
	NS_IMETHOD Move(nsIRDFResource* aOldSource,
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
	NS_IMETHOD	GetAllResources(nsISimpleEnumerator** aCursor);
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
};



PRInt32			RelatedLinksHandlerImpl::gRefCnt;
nsIRDFService		*RelatedLinksHandlerImpl::gRDFService;

nsIRDFResource		*RelatedLinksHandlerImpl::kNC_RelatedLinksRoot;
nsIRDFResource		*RelatedLinksHandlerImpl::kNC_Child;
nsIRDFResource		*RelatedLinksHandlerImpl::kNC_Name;
nsIRDFResource		*RelatedLinksHandlerImpl::kRDF_type;



RelatedLinksHandlerImpl::RelatedLinksHandlerImpl()
	: mURI(nsnull),
	  mRelatedLinksURL(nsnull),
	  mPerformQuery(PR_FALSE)
{
	NS_INIT_REFCNT();
}



RelatedLinksHandlerImpl::~RelatedLinksHandlerImpl()
{
	if (mRelatedLinksURL)
	{
		PL_strfree(mRelatedLinksURL);
		mRelatedLinksURL = nsnull;
	}

	if (--gRefCnt == 0)
	{
		NS_RELEASE(kNC_RelatedLinksRoot);
		NS_RELEASE(kNC_Child);
		NS_RELEASE(kNC_Name);
		NS_RELEASE(kRDF_type);

		nsServiceManager::ReleaseService(kRDFServiceCID, gRDFService);
		gRDFService = nsnull;
	}
}



nsresult
RelatedLinksHandlerImpl::Init()
{
	nsresult rv;

	if (gRefCnt++ == 0)
	{
		rv = nsServiceManager::GetService(kRDFServiceCID,
                                                  nsIRDFService::GetIID(),
                                                  (nsISupports**) &gRDFService);
		if (NS_FAILED(rv)) return rv;

		gRDFService->GetResource(kURINC_RelatedLinksRoot, &kNC_RelatedLinksRoot);
		gRDFService->GetResource(NC_NAMESPACE_URI  "child", &kNC_Child);
		gRDFService->GetResource(NC_NAMESPACE_URI  "Name", &kNC_Name);
		gRDFService->GetResource(RDF_NAMESPACE_URI "type", &kRDF_type);
	}

	rv = nsComponentManager::CreateInstance(kRDFInMemoryDataSourceCID,
											nsnull,
											nsIRDFDataSource::GetIID(),
											getter_AddRefs(mInner));
	if (NS_FAILED(rv)) return rv;

	return NS_OK;
}



NS_IMETHODIMP
NS_NewRelatedLinksHandler(nsISupports* aOuter, REFNSIID aIID, void** aResult)
{
	NS_PRECONDITION(aResult != nsnull, "null ptr");
	if (! aResult)
		return NS_ERROR_NULL_POINTER;

	NS_PRECONDITION(aOuter == nsnull, "no aggregation");
	if (aOuter)
		return NS_ERROR_NO_AGGREGATION;

	nsresult rv = NS_OK;

	RelatedLinksHandlerImpl* result = new RelatedLinksHandlerImpl();
	if (! result)
		return NS_ERROR_OUT_OF_MEMORY;

	rv = result->Init();
	if (NS_SUCCEEDED(rv)) {
		rv = result->QueryInterface(aIID, aResult);
	}

	if (NS_FAILED(rv)) {
		delete result;
		*aResult = nsnull;
		return rv;
	}

	return rv;
}



// nsISupports interface

NS_IMPL_ADDREF(RelatedLinksHandlerImpl);
NS_IMPL_RELEASE(RelatedLinksHandlerImpl);


NS_IMETHODIMP
RelatedLinksHandlerImpl::QueryInterface(REFNSIID aIID, void** aResult)
{
	NS_PRECONDITION(aResult != nsnull, "null ptr");
	if (! aResult)
		return NS_ERROR_NULL_POINTER;

	static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);

	if (aIID.Equals(nsIRelatedLinksHandler::GetIID()) ||
	    aIID.Equals(kISupportsIID))
	{
		*aResult = NS_STATIC_CAST(nsIRelatedLinksHandler*, this);
	}
	else if (aIID.Equals(nsIRDFDataSource::GetIID())) {
		*aResult = NS_STATIC_CAST(nsIRDFDataSource*, this);
	}
	else
	{
		*aResult = nsnull;
		return NS_NOINTERFACE;
	}

	// If we get here, we know the QI succeeded
	NS_ADDREF(this);
	return NS_OK;
}



// nsIRelatedLinksHandler interface

NS_IMETHODIMP
RelatedLinksHandlerImpl::GetURL(char** aURL)
{
	NS_PRECONDITION(aURL != nsnull, "null ptr");
	if (! aURL)
		return NS_ERROR_NULL_POINTER;

	if (mRelatedLinksURL)
	{
		*aURL = nsXPIDLCString::Copy(mRelatedLinksURL);
		return *aURL ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
	}
	else
	{
		*aURL = nsnull;
		return NS_OK;
	}
}



NS_IMETHODIMP
RelatedLinksHandlerImpl::SetURL(const char* aURL)
{
	NS_PRECONDITION(aURL != nsnull, "null ptr");
	if (! aURL)
		return NS_ERROR_NULL_POINTER;

	nsresult rv;

	if (mRelatedLinksURL)
		PL_strfree(mRelatedLinksURL);

	mRelatedLinksURL = PL_strdup(aURL);
	if (! mRelatedLinksURL)
		return NS_ERROR_OUT_OF_MEMORY;

	// Flush the old links. This'll force notifications to propogate, too.
	nsCOMPtr<nsIRDFPurgeableDataSource> purgeable = do_QueryInterface(mInner);
	NS_ASSERTION(purgeable, "uh oh, this datasource isn't purgeable!");
	if (! purgeable)
		return NS_ERROR_UNEXPECTED;

	rv = purgeable->Sweep();
	if (NS_FAILED(rv)) return rv;

	// XXX This should be a parameter
	nsCAutoString	relatedLinksQueryURL("http://www-rl.netscape.com/wtgn?");
	relatedLinksQueryURL += mRelatedLinksURL;

	const char *queryURL = relatedLinksQueryURL.GetBuffer();
	if (! queryURL)
		return NS_ERROR_OUT_OF_MEMORY;

	nsCOMPtr<nsIURI> url;
	rv = NS_NewURI(getter_AddRefs(url), queryURL);

	if (NS_FAILED(rv)) return rv;

	nsCOMPtr<nsIStreamListener> listener;
	rv = NS_NewRelatedLinksStreamListener(mInner, getter_AddRefs(listener));
	if (NS_FAILED(rv)) return rv;

	// XXX: Should there be a LoadGroup?
	rv = NS_OpenURI(listener, nsnull, url, nsnull);
	if (NS_FAILED(rv)) return rv;

	return NS_OK;
}



// nsIRDFDataSource interface


NS_IMETHODIMP
RelatedLinksHandlerImpl::GetURI(char **aURI)
{
	NS_PRECONDITION(aURI != nsnull, "null ptr");
	if (! aURI)
		return NS_ERROR_NULL_POINTER;

	// XXX We could munge in the current URL that we're looking at I
	// suppose. Not critical because this datasource shouldn't be
	// registered with the RDF service.
	*aURI = nsXPIDLCString::Copy("rdf:related-links");
	if (! *aURI)
		return NS_ERROR_OUT_OF_MEMORY;

	return NS_OK;
}



NS_IMETHODIMP
RelatedLinksHandlerImpl::GetSource(nsIRDFResource* aProperty,
				   nsIRDFNode* aTarget,
				   PRBool aTruthValue,
				   nsIRDFResource** aSource)
{
       	return mInner->GetSource(aProperty, aTarget, aTruthValue, aSource);
}



NS_IMETHODIMP
RelatedLinksHandlerImpl::GetSources(nsIRDFResource *aProperty,
				    nsIRDFNode *aTarget,
				    PRBool aTruthValue,
				    nsISimpleEnumerator **aSources)
{
       	return mInner->GetSources(aProperty, aTarget, aTruthValue, aSources);
}



NS_IMETHODIMP
RelatedLinksHandlerImpl::GetTarget(nsIRDFResource *aSource,
				   nsIRDFResource *aProperty,
				   PRBool aTruthValue,
				   nsIRDFNode **aTarget)
{
	return mInner->GetTarget(aSource, aProperty, aTruthValue, aTarget);
}



NS_IMETHODIMP
RelatedLinksHandlerImpl::GetTargets(nsIRDFResource* aSource,
				    nsIRDFResource* aProperty,
				    PRBool aTruthValue,
				    nsISimpleEnumerator** aTargets)
{
	return mInner->GetTargets(aSource, aProperty, aTruthValue, aTargets);
}



NS_IMETHODIMP
RelatedLinksHandlerImpl::Assert(nsIRDFResource *aSource,
				nsIRDFResource *aProperty,
				nsIRDFNode *aTarget,
				PRBool aTruthValue)
{
	return NS_RDF_ASSERTION_REJECTED;
}



NS_IMETHODIMP
RelatedLinksHandlerImpl::Unassert(nsIRDFResource *aSource,
				  nsIRDFResource *aProperty,
				  nsIRDFNode *aTarget)
{
	return NS_RDF_ASSERTION_REJECTED;
}



NS_IMETHODIMP
RelatedLinksHandlerImpl::Change(nsIRDFResource* aSource,
								nsIRDFResource* aProperty,
								nsIRDFNode* aOldTarget,
								nsIRDFNode* aNewTarget)
{
	return NS_RDF_ASSERTION_REJECTED;
}



NS_IMETHODIMP
RelatedLinksHandlerImpl::Move(nsIRDFResource* aOldSource,
							  nsIRDFResource* aNewSource,
							  nsIRDFResource* aProperty,
							  nsIRDFNode* aTarget)
{
	return NS_RDF_ASSERTION_REJECTED;
}



NS_IMETHODIMP
RelatedLinksHandlerImpl::HasAssertion(nsIRDFResource *aSource,
				      nsIRDFResource *aProperty,
				      nsIRDFNode *aTarget,
				      PRBool aTruthValue,
				      PRBool *aResult)
{
	return mInner->HasAssertion(aSource, aProperty, aTarget, aTruthValue, aResult);
}



NS_IMETHODIMP
RelatedLinksHandlerImpl::ArcLabelsIn(nsIRDFNode *aTarget,
				     nsISimpleEnumerator **aLabels)
{
	return mInner->ArcLabelsIn(aTarget, aLabels);
}



NS_IMETHODIMP
RelatedLinksHandlerImpl::ArcLabelsOut(nsIRDFResource *aSource,
				      nsISimpleEnumerator **aLabels)
{
	return mInner->ArcLabelsOut(aSource, aLabels);
}



NS_IMETHODIMP
RelatedLinksHandlerImpl::GetAllResources(nsISimpleEnumerator** aCursor)
{
	return mInner->GetAllResources(aCursor);
}



NS_IMETHODIMP
RelatedLinksHandlerImpl::AddObserver(nsIRDFObserver *aObserver)
{
	return mInner->AddObserver(aObserver);
}



NS_IMETHODIMP
RelatedLinksHandlerImpl::RemoveObserver(nsIRDFObserver *aObserver)
{
	return mInner->RemoveObserver(aObserver);
}



NS_IMETHODIMP
RelatedLinksHandlerImpl::GetAllCommands(nsIRDFResource* aSource,
					nsIEnumerator/*<nsIRDFResource>*/** aCommands)
{
	return mInner->GetAllCommands(aSource, aCommands);
}



NS_IMETHODIMP
RelatedLinksHandlerImpl::GetAllCmds(nsIRDFResource* aSource,
					nsISimpleEnumerator/*<nsIRDFResource>*/** aCommands)
{
	return mInner->GetAllCmds(aSource, aCommands);
}



NS_IMETHODIMP
RelatedLinksHandlerImpl::IsCommandEnabled(nsISupportsArray/*<nsIRDFResource>*/* aSources,
				nsIRDFResource*   aCommand,
				nsISupportsArray/*<nsIRDFResource>*/* aArguments,
                               PRBool* aResult)
{
	return mInner->IsCommandEnabled(aSources, aCommand, aArguments, aResult);
}



NS_IMETHODIMP
RelatedLinksHandlerImpl::DoCommand(nsISupportsArray/*<nsIRDFResource>*/* aSources,
				nsIRDFResource*   aCommand,
				nsISupportsArray/*<nsIRDFResource>*/* aArguments)
{
	return mInner->DoCommand(aSources, aCommand, aArguments);
}



////////////////////////////////////////////////////////////////////////
// Component Exports

extern "C" PR_IMPLEMENT(nsresult)
NSGetFactory(nsISupports* aServiceMgr,
             const nsCID &aClass,
             const char *aClassName,
             const char *aProgID,
             nsIFactory **aFactory)
{
	NS_PRECONDITION(aFactory != nsnull, "null ptr");
	if (! aFactory)
		return NS_ERROR_NULL_POINTER;

	nsIGenericFactory::ConstructorProcPtr constructor;

	if (aClass.Equals(kRelatedLinksHandlerCID)) {
		constructor = NS_NewRelatedLinksHandler;
	}
	else {
		*aFactory = nsnull;
		return NS_NOINTERFACE; // XXX
	}

	nsresult rv;
	NS_WITH_SERVICE1(nsIComponentManager, compMgr, aServiceMgr, kComponentManagerCID, &rv);
	if (NS_FAILED(rv)) return rv;

	nsCOMPtr<nsIGenericFactory> factory;
	rv = compMgr->CreateInstance(kGenericFactoryCID,
				     nsnull,
				     nsIGenericFactory::GetIID(),
				     getter_AddRefs(factory));

	if (NS_FAILED(rv)) return rv;

	rv = factory->SetConstructor(constructor);
	if (NS_FAILED(rv)) return rv;

	*aFactory = factory;
	NS_ADDREF(*aFactory);
	return NS_OK;
}



extern "C" PR_IMPLEMENT(nsresult)
NSRegisterSelf(nsISupports* aServMgr , const char* aPath)
{
	nsresult rv;

	nsCOMPtr<nsIServiceManager> servMgr(do_QueryInterface(aServMgr, &rv));
	if (NS_FAILED(rv)) return rv;

	NS_WITH_SERVICE1(nsIComponentManager, compMgr, servMgr, kComponentManagerCID, &rv);
	if (NS_FAILED(rv)) return rv;

	rv = compMgr->RegisterComponent(kRelatedLinksHandlerCID, "Related Links Handler",
									NS_RELATEDLINKSHANDLER_PROGID,
									aPath, PR_TRUE, PR_TRUE);

	return NS_OK;
}



extern "C" PR_IMPLEMENT(nsresult)
NSUnregisterSelf(nsISupports* aServMgr, const char* aPath)
{
	nsresult rv;

	nsCOMPtr<nsIServiceManager> servMgr(do_QueryInterface(aServMgr, &rv));
	if (NS_FAILED(rv)) return rv;

	NS_WITH_SERVICE1(nsIComponentManager, compMgr, servMgr, kComponentManagerCID, &rv);
	if (NS_FAILED(rv)) return rv;

	rv = compMgr->UnregisterComponent(kRelatedLinksHandlerCID, aPath);

	return NS_OK;
}
