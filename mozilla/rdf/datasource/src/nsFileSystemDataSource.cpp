/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
  Implementation for a file system RDF data store.
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
#include "nsIRDFService.h"
#include "xp_core.h"
#include "plhash.h"
#include "plstr.h"
#include "prmem.h"
#include "prprf.h"
#include "prio.h"
#include "nsFileSpec.h"
#include "nsIRDFFileSystem.h"
#include "nsFileSystemDataSource.h"
#include "nsFileSpec.h"


#ifdef	XP_WIN
#include "windef.h"
#include "winbase.h"
#endif



static NS_DEFINE_CID(kRDFServiceCID,               NS_RDFSERVICE_CID);
static NS_DEFINE_IID(kIRDFServiceIID,              NS_IRDFSERVICE_IID);
static NS_DEFINE_IID(kIRDFFileSystemDataSourceIID, NS_IRDFFILESYSTEMDATAOURCE_IID);
static NS_DEFINE_IID(kIRDFFileSystemIID,           NS_IRDFFILESYSTEM_IID);
static NS_DEFINE_IID(kIRDFAssertionCursorIID,      NS_IRDFASSERTIONCURSOR_IID);
static NS_DEFINE_IID(kIRDFCursorIID,               NS_IRDFCURSOR_IID);
static NS_DEFINE_IID(kIRDFArcsOutCursorIID,        NS_IRDFARCSOUTCURSOR_IID);
static NS_DEFINE_IID(kISupportsIID,                NS_ISUPPORTS_IID);
static NS_DEFINE_IID(kIRDFResourceIID,             NS_IRDFRESOURCE_IID);
static NS_DEFINE_IID(kIRDFResourceFactoryIID,      NS_IRDFRESOURCEFACTORY_IID);
static NS_DEFINE_IID(kIRDFNodeIID,                 NS_IRDFNODE_IID);



static const char kURINC_FileSystemRoot[] = "NC:FilesRoot";

DEFINE_RDF_VOCAB(NC_NAMESPACE_URI, NC, child);
DEFINE_RDF_VOCAB(NC_NAMESPACE_URI, NC, Name);
DEFINE_RDF_VOCAB(NC_NAMESPACE_URI, NC, URL);
DEFINE_RDF_VOCAB(NC_NAMESPACE_URI, NC, Columns);

DEFINE_RDF_VOCAB(RDF_NAMESPACE_URI, RDF, instanceOf);
DEFINE_RDF_VOCAB(RDF_NAMESPACE_URI, RDF, Bag);



static	nsIRDFService		*gRDFService = nsnull;
static	FileSystemDataSource	*gFileSystemDataSource = nsnull;



static PRBool
peq(nsIRDFResource* r1, nsIRDFResource* r2)
{
	PRBool		retVal=PR_FALSE, result;

	if (NS_SUCCEEDED(r1->EqualsResource(r2, &result)))
	{
		if (result)
		{
			retVal = PR_TRUE;
		}
	}
	return(retVal);
}


static PRBool
isFileURI(nsIRDFResource *r)
{
	PRBool		isFileURI = PR_FALSE;
	const char	*uri;
	
	r->GetValue(&uri);
	if (!strncmp(uri, "file://", 7))
	{
		// XXX HACK HACK HACK
		if (!strchr(uri, '#'))
		{
			isFileURI = PR_TRUE;
		}
	}
	return(isFileURI);
}



FileSystemDataSource::FileSystemDataSource(void)
	: mURI(nsnull),
	  mObservers(nsnull)
{
	NS_INIT_REFCNT();
	nsresult rv = nsServiceManager::GetService(kRDFServiceCID,
		kIRDFServiceIID, (nsISupports**) &gRDFService);
	PR_ASSERT(NS_SUCCEEDED(rv));
	gFileSystemDataSource = this;
}



FileSystemDataSource::~FileSystemDataSource (void)
{
	gRDFService->UnregisterDataSource(this);

	PL_strfree(mURI);
	if (nsnull != mObservers)
	{
		for (PRInt32 i = mObservers->Count(); i >= 0; --i)
		{
			nsIRDFObserver* obs = (nsIRDFObserver*) mObservers->ElementAt(i);
			NS_RELEASE(obs);
		}
		delete mObservers;
		mObservers = nsnull;
	}

	nsrefcnt	refcnt;
	NS_RELEASE2(kNC_FileSystemRoot, refcnt);
	NS_RELEASE2(kNC_Child, refcnt);
	NS_RELEASE2(kNC_Name, refcnt);
	NS_RELEASE2(kNC_URL, refcnt);
	NS_RELEASE2(kNC_Columns, refcnt);

	NS_RELEASE2(kRDF_InstanceOf, refcnt);
	NS_RELEASE2(kRDF_Bag, refcnt);

	gFileSystemDataSource = nsnull;
	nsServiceManager::ReleaseService(kRDFServiceCID, gRDFService);
	gRDFService = nsnull;
}



NS_IMPL_ISUPPORTS(FileSystemDataSource, kIRDFFileSystemDataSourceIID);



NS_IMETHODIMP
FileSystemDataSource::Init(const char *uri)
{
	nsresult	rv = NS_ERROR_OUT_OF_MEMORY;

	if ((mURI = PL_strdup(uri)) == nsnull)
		return rv;

	gRDFService->GetResource(kURINC_FileSystemRoot, &kNC_FileSystemRoot);
	gRDFService->GetResource(kURINC_child, &kNC_Child);
	gRDFService->GetResource(kURINC_Name, &kNC_Name);
	gRDFService->GetResource(kURINC_URL, &kNC_URL);
	gRDFService->GetResource(kURINC_Columns, &kNC_Columns);

	gRDFService->GetResource(kURIRDF_instanceOf, &kRDF_InstanceOf);
	gRDFService->GetResource(kURIRDF_Bag, &kRDF_Bag);

	//   if (NS_FAILED(rv = AddColumns()))
	//       return rv;

	// register this as a named data source with the service manager
	if (NS_FAILED(rv = gRDFService->RegisterDataSource(this)))
		return rv;

	return NS_OK;
}



NS_IMETHODIMP
FileSystemDataSource::GetURI(const char **uri) const
{
	*uri = mURI;
	return NS_OK;
}



NS_IMETHODIMP
FileSystemDataSource::GetSource(nsIRDFResource* property,
                          nsIRDFNode* target,
                          PRBool tv,
                          nsIRDFResource** source /* out */)
{
	nsresult rv = NS_ERROR_RDF_NO_VALUE;
	return rv;
}



NS_IMETHODIMP
FileSystemDataSource::GetSources(nsIRDFResource *property,
                           nsIRDFNode *target,
			   PRBool tv,
                           nsIRDFAssertionCursor **sources /* out */)
{
	PR_ASSERT(0);
	return NS_ERROR_NOT_IMPLEMENTED;
}



nsresult	GetVolumeList(nsVoidArray **array);
nsresult	GetFolderList(nsIRDFResource *source, nsVoidArray **array /* out */);
nsresult	GetName(nsIRDFResource *source, nsVoidArray **array);
nsresult	GetURL(nsIRDFResource *source, nsVoidArray **array);



NS_IMETHODIMP
FileSystemDataSource::GetTarget(nsIRDFResource *source,
                          nsIRDFResource *property,
                          PRBool tv,
                          nsIRDFNode **target /* out */)
{
	nsresult		rv = NS_ERROR_RDF_NO_VALUE;

	// we only have positive assertions in the file system data source.
	if (! tv)
		return rv;

	if (peq(source, kNC_FileSystemRoot) || isFileURI(source))
	{
		nsVoidArray		*array = nsnull;

		if (peq(property, kNC_Name))
		{
			rv = GetName(source, &array);
		}
		else if (peq(property, kNC_URL))
		{
			rv = GetURL(source, &array);
		}

		if (array != nsnull)
		{
			nsIRDFLiteral *literal = (nsIRDFLiteral *)(array->ElementAt(0));
			*target = (nsIRDFNode *)literal;
			delete array;
			rv = NS_OK;
		}
		else
		{
			rv = NS_ERROR_RDF_NO_VALUE;
		}
	}
	return(rv);
}



NS_IMETHODIMP
FileSystemDataSource::GetTargets(nsIRDFResource *source,
                           nsIRDFResource *property,
                           PRBool tv,
                           nsIRDFAssertionCursor **targets /* out */)
{
	nsVoidArray		*array = nsnull;
	nsresult		rv = NS_ERROR_FAILURE;

	if (peq(source, kNC_FileSystemRoot))
	{
		if (peq(property, kNC_Child))
		{
			rv = GetVolumeList(&array);
		}
	}
	else if (isFileURI(source))
	{
		if (peq(property, kNC_Child))
		{
			rv = GetFolderList(source, &array);
		}
		else if (peq(property, kNC_Name))
		{
			rv = GetName(source, &array);
		}
		else if (peq(property, kNC_URL))
		{
			rv = GetURL(source, &array);
		}
	}
	if ((rv == NS_OK) && (nsnull != array))
	{
		*targets = new FileSystemCursor(source, property, array);
		NS_ADDREF(*targets);
	}
	return(rv);
}



NS_IMETHODIMP
FileSystemDataSource::Assert(nsIRDFResource *source,
                       nsIRDFResource *property,
                       nsIRDFNode *target,
                       PRBool tv)
{
	PR_ASSERT(0);
	return NS_ERROR_NOT_IMPLEMENTED;
}



NS_IMETHODIMP
FileSystemDataSource::Unassert(nsIRDFResource *source,
                         nsIRDFResource *property,
                         nsIRDFNode *target)
{
	PR_ASSERT(0);
	return NS_ERROR_NOT_IMPLEMENTED;
}



NS_IMETHODIMP
FileSystemDataSource::HasAssertion(nsIRDFResource *source,
                             nsIRDFResource *property,
                             nsIRDFNode *target,
                             PRBool tv,
                             PRBool *hasAssertion /* out */)
{
	PRBool			retVal = PR_FALSE;
	nsresult		rv = NS_ERROR_FAILURE;

	*hasAssertion = PR_FALSE;
	if (isFileURI(source))
	{
		if (peq(property, kRDF_InstanceOf))
		{
			if (peq((nsIRDFResource *)target, kRDF_Bag))
			{
				*hasAssertion = PR_TRUE;
				rv = NS_OK;
			}
		}
	}
	return (rv);
}



NS_IMETHODIMP
FileSystemDataSource::ArcLabelsIn(nsIRDFNode *node,
                            nsIRDFArcsInCursor ** labels /* out */)
{
	PR_ASSERT(0);
	return NS_ERROR_NOT_IMPLEMENTED;
}



NS_IMETHODIMP
FileSystemDataSource::ArcLabelsOut(nsIRDFResource *source,
                             nsIRDFArcsOutCursor **labels /* out */)
{
	nsresult		rv = NS_ERROR_RDF_NO_VALUE;

	*labels = nsnull;

	if (peq(source, kNC_FileSystemRoot))
	{
		nsVoidArray *temp = new nsVoidArray();
		if (nsnull == temp)
			return NS_ERROR_OUT_OF_MEMORY;

		temp->AppendElement(kNC_Child);
		*labels = new FileSystemCursor(source, kNC_Child, temp);
		if (nsnull != *labels)
		{
			NS_ADDREF(*labels);
			rv = NS_OK;
		}
	}
	else if (isFileURI(source))
	{
		nsVoidArray *temp = new nsVoidArray();
		if (nsnull == temp)
			return NS_ERROR_OUT_OF_MEMORY;

		temp->AppendElement(kNC_Child);
//		temp->AppendElement(kNC_Name);
//		temp->AppendElement(kNC_URL);
//		temp->AppendElement(kNC_Columns);
		*labels = new FileSystemCursor(source, kNC_Child, temp);
		if (nsnull != *labels)
		{
			NS_ADDREF(*labels);
			rv = NS_OK;
		}
	}
	return(rv);

}



NS_IMETHODIMP
FileSystemDataSource::GetAllResources(nsIRDFResourceCursor** aCursor)
{
	NS_NOTYETIMPLEMENTED("sorry!");
	return NS_ERROR_NOT_IMPLEMENTED;
}



NS_IMETHODIMP
FileSystemDataSource::AddObserver(nsIRDFObserver *n)
{
	if (nsnull == mObservers)
	{
		if ((mObservers = new nsVoidArray()) == nsnull)
			return NS_ERROR_OUT_OF_MEMORY;
	}
	mObservers->AppendElement(n);
	return NS_OK;
}



NS_IMETHODIMP
FileSystemDataSource::RemoveObserver(nsIRDFObserver *n)
{
	if (nsnull == mObservers)
		return NS_OK;
	mObservers->RemoveElement(n);
	return NS_OK;
}



NS_IMETHODIMP
FileSystemDataSource::Flush()
{
	PR_ASSERT(0);
	return NS_ERROR_NOT_IMPLEMENTED;
}



NS_IMETHODIMP
FileSystemDataSource::IsCommandEnabled(const char* aCommand,
                                 nsIRDFResource* aCommandTarget,
                                 PRBool* aResult)
{
	return NS_OK;
}



NS_IMETHODIMP
FileSystemDataSource::DoCommand(const char* aCommand,
                          nsIRDFResource* aCommandTarget)
{
	return NS_OK;
}



nsresult
NS_NewRDFFileSystemDataSource(nsIRDFDataSource **result)
{
	if (!result)
		return NS_ERROR_NULL_POINTER;

	// only one file system data source
	if (nsnull == gFileSystemDataSource)
	{
		if ((gFileSystemDataSource = new FileSystemDataSource()) == nsnull)
		{
			return NS_ERROR_OUT_OF_MEMORY;
		}
	}
	NS_ADDREF(gFileSystemDataSource);
	*result = gFileSystemDataSource;
	return NS_OK;
}



nsresult
GetVolumeList(nsVoidArray **array)
{
	nsVoidArray		*volumes = new nsVoidArray();

	*array = volumes;
	if (nsnull == *array)
	{
		return(NS_ERROR_OUT_OF_MEMORY);
	}

	nsIRDFResource		*vol;

#ifdef	XP_MAC
	FSSpec			fss;
	OSErr			err;
	ParamBlockRec		pb;
	char			*url;
	int16			volNum = 1;

	do
	{
		pb.volumeParam.ioCompletion = NULL;
		pb.volumeParam.ioVolIndex = volNum++;
		pb.volumeParam.ioNamePtr = (StringPtr)fss.name;
		if (!(err = PBGetVInfo(&pb,FALSE)))
		{
			fss.vRefNum = pb.volumeParam.ioVRefNum;
			fss.parID = 2L;
			fss.name[0] = '\0';

			// even though we have the volume name, create a
			// nativeFileSpec and then get name out of it so
			// that if will be fully encoded

			nsNativeFileSpec	nativeSpec(fss);
			nsFilePath		filePath(nativeSpec);
			char			*volURL = filePath;
			if (volURL != nsnull)
			{
				nsAutoString	pathname("file://");
				pathname += volURL;
				if (nativeSpec.IsDirectory())
				{
					pathname += "/";
				}
				char		*filename = pathname.ToNewCString();
				gRDFService->GetResource(filename, (nsIRDFResource **)&vol);
				NS_ADDREF(vol);
				volumes->AppendElement(vol);
				delete filename;
			}
		}
	} while (!err);
#endif

#ifdef	XP_WIN
	PRInt32			driveType;
	char			drive[32];
	PRInt32			volNum;
	char			*url;

	for (volNum = 0; volNum < 26; volNum++)
	{
		sprintf(drive, "%c:\\", volNum + 'A');
		driveType = GetDriveType(drive);
		if (driveType != DRIVE_UNKNOWN && driveType != DRIVE_NO_ROOT_DIR)
		{
			if (nsnull != (url = PR_smprintf("file:///%c|/", volNum + 'A')))
			{
				gRDFService->GetResource(url, (nsIRDFResource **)&vol);
				NS_ADDREF(vol);
				volumes->AppendElement(vol);
				PR_Free(url);
			}
		}
	}
#endif

#ifdef	XP_UNIX
	gRDFService->GetResource("file:///", (nsIRDFResource **)&vol);
	NS_ADDREF(vol);
	volumes->AppendElement(vol);
#endif

	return NS_OK;
}



FileSystemCursor::FileSystemCursor(nsIRDFResource *source,
				nsIRDFResource *property,
				nsVoidArray *array)
{
	NS_INIT_REFCNT();

	mSource = source;
	mProperty = property;
	mArray = array;
	NS_ADDREF(mSource);
	NS_ADDREF(mProperty);
	mCount = 0;
	mTarget = nsnull;
	mValue = nsnull;
}



FileSystemCursor::~FileSystemCursor(void)
{
	NS_IF_RELEASE(mSource);
	NS_IF_RELEASE(mValue);
	NS_IF_RELEASE(mProperty);
	NS_IF_RELEASE(mTarget);
}



NS_IMETHODIMP
FileSystemCursor::Advance(void)
{
	if (!mArray)
		return NS_ERROR_NULL_POINTER;
	if (mArray->Count() <= mCount)
		return NS_ERROR_RDF_CURSOR_EMPTY;
	NS_IF_RELEASE(mValue);
	mTarget = mValue = (nsIRDFNode *)mArray->ElementAt(mCount++);
	NS_ADDREF(mValue);
	return NS_OK;
}



NS_IMETHODIMP
FileSystemCursor::GetValue(nsIRDFNode **aValue)
{
	if (nsnull == mValue)
		return NS_ERROR_NULL_POINTER;
	NS_ADDREF(mValue);
	*aValue = mValue;
	return NS_OK;
}



NS_IMETHODIMP
FileSystemCursor::GetDataSource(nsIRDFDataSource **aDataSource)
{
	NS_ADDREF(gFileSystemDataSource);
	*aDataSource = gFileSystemDataSource;
	return NS_OK;
}



NS_IMETHODIMP
FileSystemCursor::GetSubject(nsIRDFResource **aResource)
{
	NS_ADDREF(mSource);
	*aResource = mSource;
	return NS_OK;
}



NS_IMETHODIMP
FileSystemCursor::GetPredicate(nsIRDFResource **aPredicate)
{
	NS_ADDREF(mProperty);
	*aPredicate = mProperty;
	return NS_OK;
}



NS_IMETHODIMP
FileSystemCursor::GetObject(nsIRDFNode **aObject)
{
	if (nsnull != mTarget)
		NS_ADDREF(mTarget);
	*aObject = mTarget;
	return NS_OK;
}



NS_IMETHODIMP
FileSystemCursor::GetTruthValue(PRBool *aTruthValue)
{
	*aTruthValue = 1;
	return NS_OK;
}



NS_IMPL_ADDREF(FileSystemCursor);
NS_IMPL_RELEASE(FileSystemCursor);



NS_IMETHODIMP
FileSystemCursor::QueryInterface(REFNSIID iid, void **result)
{
	if (! result)
		return NS_ERROR_NULL_POINTER;

	*result = nsnull;
	if (iid.Equals(kIRDFAssertionCursorIID) ||
		iid.Equals(kIRDFCursorIID) ||
		iid.Equals(kIRDFArcsOutCursorIID) ||
		iid.Equals(kISupportsIID))
	{
		*result = NS_STATIC_CAST(nsIRDFAssertionCursor *, this);
		AddRef();
		return NS_OK;
	}
	return(NS_NOINTERFACE);
}



PRBool
isVisible(nsNativeFileSpec file)
{
	PRBool		isVisible = PR_TRUE;

#ifdef	XP_MAC

	FInfo		fndrInfo;
	FSSpec		fss = file;
	OSErr		err;

	CInfoPBRec	cInfo;

	cInfo.hFileInfo.ioCompletion = NULL;
	cInfo.hFileInfo.ioVRefNum = fss.vRefNum;
	cInfo.hFileInfo.ioDirID = fss.parID;
	cInfo.hFileInfo.ioNamePtr = (StringPtr)fss.name;
	cInfo.hFileInfo.ioFVersNum = 0;
	if (!(err = PBGetCatInfoSync(&cInfo)))
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
		delete []basename;
	}
#endif

	return(isVisible);
}



nsresult
GetFolderList(nsIRDFResource *source, nsVoidArray **array /* out */)
{
	nsVoidArray	*nameArray = new nsVoidArray();
	*array = nameArray;
	if (nsnull == nameArray)
	{
		return(NS_ERROR_OUT_OF_MEMORY);
	}

	const char		*uri;
	source->GetValue(&uri);
	if (nsnull == uri)
	{
		return(NS_ERROR_FAILURE);
	}

	nsFileURL 		parentDir(uri);
	nsNativeFileSpec 	nativeDir(parentDir);
	for (nsDirectoryIterator i(nativeDir); i; i++)
	{
		nsNativeFileSpec	nativeSpec = (const nsNativeFileSpec &)i;
		if (!isVisible(nativeSpec))	continue;
		nsFilePath		filePath(nativeSpec);
		char			*childURL = filePath;
		if (childURL != nsnull)
		{
			nsAutoString	pathname("file://");
			pathname += childURL;
			if (nativeSpec.IsDirectory())
			{
				pathname += "/";
			}
			char		*filename = pathname.ToNewCString();
			nsIRDFResource	*file;
			gRDFService->GetResource(filename, (nsIRDFResource **)&file);
			nameArray->AppendElement(file);
			delete filename;
		}
	}
	return(NS_OK);
}



nsresult
GetName(nsIRDFResource *source, nsVoidArray **array)
{
	nsVoidArray *nameArray = new nsVoidArray();
	*array = nameArray;
	if (nsnull == nameArray)
	{
		return(NS_ERROR_OUT_OF_MEMORY);
	}

	const char		*uri;
	source->GetValue(&uri);
	nsFileURL		url(uri);
	nsNativeFileSpec	native(url);
	char			*basename = native.GetLeafName();
	if (basename)
	{
		nsString *name = new nsString(basename);
		if (nsnull != name)
		{
			nsIRDFLiteral *literal;
			gRDFService->GetLiteral(*name, &literal);
			nameArray->AppendElement(literal);
			delete name;
		}
	}
	return(NS_OK);
}



nsresult
GetURL(nsIRDFResource *source, nsVoidArray **array)
{
	nsVoidArray *urlArray = new nsVoidArray();
	*array = urlArray;
	if (nsnull == urlArray)
	{
		return(NS_ERROR_OUT_OF_MEMORY);
	}

	const char	*uri;
	source->GetValue(&uri);
	nsAutoString	url(uri);

	nsIRDFLiteral	*literal;
	gRDFService->GetLiteral(url, &literal);
	urlArray->AppendElement(literal);
	return(NS_OK);
}
