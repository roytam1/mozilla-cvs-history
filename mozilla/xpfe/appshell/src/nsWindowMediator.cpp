/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
 * Copyright (C) 1999 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

#include "nsString.h"
#include "nsVoidArray.h"
#include "rdf.h"
#include "nsIRDFService.h"
#include "nsIRDFDataSource.h"
#include "nsIRDFNode.h"
#include "nsIRDFObserver.h"
#include "nsRDFCID.h"
#include "nsIRDFContainer.h"
#include "nsIRDFContainerUtils.h"
#include "nsXPIDLString.h"
#include "nsIServiceManager.h"
#include "nsIWindowMediator.h"
#include "nsCOMPtr.h"
#include "nsIWebShell.h"
#include "nsXPComFactory.h"
#include "nsIWebShellWindow.h"
#include "nsIDOMWindow.h"
#include "nsIDOMElement.h"
#include "nsISimpleEnumerator.h"

static NS_DEFINE_IID(kIRDFDataSourceIID,        NS_IRDFDATASOURCE_IID);
static NS_DEFINE_IID(kIRDFServiceIID,           NS_IRDFSERVICE_IID);
static NS_DEFINE_CID(kRDFInMemoryDataSourceCID, NS_RDFINMEMORYDATASOURCE_CID);
static NS_DEFINE_CID(kRDFServiceCID,            NS_RDFSERVICE_CID);
static NS_DEFINE_CID(kRDFContainerUtilsCID,     NS_RDFCONTAINERUTILS_CID);

static NS_DEFINE_CID(kWindowMediatorCID, NS_WINDOWMEDIATOR_CID);
static NS_DEFINE_IID(kIWindowMediatorIID,NS_IWINDOWMEDIATOR_IID);
static NS_DEFINE_IID(kISimpleEnumberatorIID, NS_ISIMPLEENUMERATOR_IID);
static NS_DEFINE_CID(kRDFContainerCID,                    NS_RDFCONTAINER_CID);
static const char kURINC_WindowMediatorRoot[] = "NC:WindowMediatorRoot";


DEFINE_RDF_VOCAB(NC_NAMESPACE_URI, NC, Name);
DEFINE_RDF_VOCAB(NC_NAMESPACE_URI, NC, URL);
static	nsIRDFService* gRDFService = nsnull;

nsresult NS_NewRDFContainer(nsIRDFDataSource* aDataSource,
                   nsIRDFResource* aResource,
                   nsIRDFContainer** aResult)
{
    nsresult rv;
  
    rv = nsComponentManager::CreateInstance( 
                                        kRDFContainerCID, NULL, nsIRDFContainer::GetIID(),  (void**)aResult );
    if (NS_FAILED(rv))
    	return rv;

    rv = (*aResult)->Init(aDataSource, aResource);
    if (NS_FAILED(rv))
    {
    	NS_RELEASE(*aResult);
    }
    return rv;
}

void GetWindowType( nsIWebShellWindow* inWindow, nsString& outType )
{
  nsCOMPtr<nsIDOMElement> webshellElement;

  if (inWindow)
    webshellElement = do_QueryInterface(inWindow);
 
  if (webshellElement )
  	webshellElement->GetAttribute("windowtype", outType );
}

nsresult GetDOMWindow( nsIWebShellWindow* inWindow, nsCOMPtr< nsIDOMWindow>& outDOMWindow )
{
	nsCOMPtr<nsIWebShell> webShell;

	inWindow->GetWebShell( *getter_AddRefs( webShell ) );
	return inWindow->ConvertWebShellToDOMWindow( webShell, getter_AddRefs( outDOMWindow ) );
}

class nsWindowMediator;

struct nsWindowInfo
{
	nsWindowInfo( nsIWebShellWindow* inWindow, PRInt32 inTimeStamp ):
				 mTimeStamp( inTimeStamp ), mWindow( inWindow )
	{
	}
	
	~nsWindowInfo()
	{
	}
	
	nsCOMPtr<nsIRDFResource>	   mRDFID;
	nsCOMPtr<nsIWebShellWindow> mWindow;
	nsString    GetType()
		{ 
			nsString rtnString;
		 	GetWindowType( mWindow, rtnString );
		 	return rtnString;
		}
	PRInt32			  mTimeStamp;
	
};

class nsWindowEnumerator;

class nsWindowMediator : public nsIWindowMediator
{
friend nsWindowEnumerator;

public:
	nsWindowMediator();
	virtual ~nsWindowMediator();
	
	NS_IMETHOD GetEnumerator( nsString* inType, nsISimpleEnumerator** outEnumerator );
	NS_IMETHOD GetMostRecentWindow( nsString* inType, nsIDOMWindow** outWindow );
	
	NS_IMETHOD RegisterWindow( nsIWebShellWindow* inWindow );
	NS_IMETHOD UnregisterWindow( nsIWebShellWindow* inWindow );
	
	NS_IMETHOD UpdateWindowTimeStamp( nsIWebShellWindow* inWindow );
	NS_IMETHOD UpdateWindowTitle( nsIWebShellWindow* inWindow , const nsString& inTitle );
	NS_IMETHOD GetWindowForResource( nsIRDFResource* inResource, nsIDOMWindow** outWindow );
	// COM and RDF 
	NS_DECL_ISUPPORTS	

	// RDF
  // nsIRDFDataSource
  NS_IMETHOD Init(const char* uri);

  NS_IMETHOD GetURI(char* *uri)
  {
      return mInner->GetURI(uri);
  }

  NS_IMETHOD GetSource(nsIRDFResource* property,
                       nsIRDFNode* target,
                       PRBool tv,
                       nsIRDFResource** source)
  {
      return mInner->GetSource(property, target, tv, source);
  }

  NS_IMETHOD GetSources(nsIRDFResource* property,
                        nsIRDFNode* target,
                        PRBool tv,
                        nsISimpleEnumerator** sources)
  {
      return mInner->GetSources(property, target, tv, sources);
  }

  NS_IMETHOD GetTarget(nsIRDFResource* source,
                       nsIRDFResource* property,
                       PRBool tv,
                       nsIRDFNode** target)
 {
   return mInner->GetTarget(source, property, tv, target);
 }

 	NS_IMETHOD GetTargets(nsIRDFResource* source,
                        nsIRDFResource* property,
                        PRBool tv,
                        nsISimpleEnumerator** targets)
  {
      return mInner->GetTargets(source, property, tv, targets);
  }

  NS_IMETHOD Assert(nsIRDFResource* aSource, 
                    nsIRDFResource* aProperty, 
                    nsIRDFNode* aTarget,
                    PRBool aTruthValue);

  NS_IMETHOD Unassert(nsIRDFResource* aSource,
                      nsIRDFResource* aProperty,
                      nsIRDFNode* aTarget);

  NS_IMETHOD HasAssertion(nsIRDFResource* source,
                          nsIRDFResource* property,
                          nsIRDFNode* target,
                          PRBool tv,
                          PRBool* hasAssertion)
  {
      return mInner->HasAssertion(source, property, target, tv, hasAssertion);
  }

  NS_IMETHOD AddObserver(nsIRDFObserver* n)
  {
      return mInner->AddObserver(n);
  }

  NS_IMETHOD RemoveObserver(nsIRDFObserver* n)
  {
      return mInner->RemoveObserver(n);
  }

  NS_IMETHOD ArcLabelsIn( nsIRDFNode* node, nsISimpleEnumerator** labels)
  {
      return mInner->ArcLabelsIn(node, labels);
  }

  NS_IMETHOD ArcLabelsOut(nsIRDFResource* source, nsISimpleEnumerator** labels)
  {
      return mInner->ArcLabelsOut(source, labels);
  }

  NS_IMETHOD GetAllResources(nsISimpleEnumerator** aCursor)
  {
      return mInner->GetAllResources(aCursor);
  }

  NS_IMETHOD Flush(void);

  NS_IMETHOD GetAllCommands(nsIRDFResource* source,
                            nsIEnumerator** commands);

  NS_IMETHOD IsCommandEnabled(nsISupportsArray* aSources,
                              nsIRDFResource*   aCommand,
                              nsISupportsArray* aArguments,
                              PRBool* aResult);

  NS_IMETHOD DoCommand(nsISupportsArray* aSources,
                       nsIRDFResource*   aCommand,
                       nsISupportsArray* aArguments);
private:
	// Helper functions
	nsresult AddWindowToRDF( nsWindowInfo* ioWindowInfo );
	PRInt32 AddEnumerator( nsWindowEnumerator* inEnumerator );
	PRInt32 RemoveEnumerator( nsWindowEnumerator* inEnumerator);
	nsVoidArray mEnumeratorList;
	nsVoidArray	mWindowList;
	PRInt32		mTimeStamp;

  // pseudo-constants for RDF
  static nsIRDFResource* kNC_WindowMediatorRoot;
  static nsIRDFResource* kNC_Name;
  static nsIRDFResource* kNC_URL;
  static PRInt32 gRefCnt;
  static nsIRDFDataSource* mInner;
};


class nsWindowEnumerator : public nsISimpleEnumerator
{

public:
	nsWindowEnumerator ( nsString* inTypeString,  nsWindowMediator& inMediator  );
	virtual ~nsWindowEnumerator();
	NS_IMETHOD HasMoreElements(PRBool *retval);
  NS_IMETHOD GetNext(nsISupports **retval);

  NS_DECL_ISUPPORTS
private:
	friend nsWindowMediator;
	
	PRInt32 FindNext();
	void WindowRemoved( PRInt32 inIndex);
	
	nsCOMPtr<nsWindowMediator> mWindowMediator;
	nsString mType;
	PRInt32 mCurrentPosition;
};



nsIRDFResource	*nsWindowMediator::kNC_WindowMediatorRoot = NULL;
nsIRDFResource	*nsWindowMediator::kNC_Name = NULL;
nsIRDFResource	*nsWindowMediator::kNC_URL = NULL;

PRInt32		nsWindowMediator::gRefCnt;
nsIRDFDataSource *nsWindowMediator::mInner = NULL;


nsWindowMediator::nsWindowMediator() :
	mTimeStamp( 0 ), mEnumeratorList(5), mWindowList(5)
{
	 NS_INIT_REFCNT();

	 if (gRefCnt++ == 0)
	 {
      nsresult rv = nsServiceManager::GetService( kRDFServiceCID, kIRDFServiceIID, (nsISupports**) &gRDFService );

			gRDFService->GetResource( kURINC_WindowMediatorRoot,   &kNC_WindowMediatorRoot );
			gRDFService->GetResource (kURINC_Name, &kNC_Name );
			gRDFService->GetResource( kURINC_URL, &kNC_URL );
    }
}

nsWindowMediator::~nsWindowMediator()
{
    if (--gRefCnt == 0)
    {
    	// Delete data
   		PRInt32 count = mWindowList.Count();
			for ( int32 i = 0; i< count; i++ )
			{
				nsWindowInfo* windowInfo = ( nsWindowInfo*) mWindowList[i];
				UnregisterWindow ( (windowInfo )->mWindow );
			}
		
		// unregister this from the RDF service
			gRDFService->UnregisterDataSource(this);
			nsServiceManager::ReleaseService(kRDFServiceCID, gRDFService);
			gRDFService = nsnull;
		
      NS_RELEASE(kNC_WindowMediatorRoot);
      NS_RELEASE( kNC_Name );
      NS_RELEASE(kNC_URL);
      NS_RELEASE(mInner);
    }
}

       
        
NS_IMETHODIMP nsWindowMediator::RegisterWindow( nsIWebShellWindow* inWindow )
{	
	if ( inWindow == NULL  )
		return NS_ERROR_INVALID_ARG;
	
	mTimeStamp++;
	
	// Create window info struct and add to list of windows
	nsWindowInfo* windowInfo = new nsWindowInfo ( inWindow, mTimeStamp );
	if ( windowInfo == NULL )
		return NS_ERROR_OUT_OF_MEMORY;
		
	if ( !mWindowList.AppendElement( windowInfo ) )
	{
	 	delete windowInfo;
	 	return NS_ERROR_OUT_OF_MEMORY;
	}
	 
	nsresult returnValue;
	if ( NS_FAILED( returnValue = AddWindowToRDF( windowInfo )  ))
	{
	 	mWindowList.RemoveElementAt( mWindowList.Count() );
	 	delete windowInfo;
	 	return returnValue;
	 }
	 
	 return NS_OK;
}

NS_IMETHODIMP nsWindowMediator::UnregisterWindow( nsIWebShellWindow* inWindow )
{
	// Find Window info
	PRInt32 count = mWindowList.Count();
	nsWindowInfo* windowInfo = NULL;
	PRInt32 windowIndex = 0;
	for ( windowIndex = 0; windowIndex< count; windowIndex++ )
	{
		windowInfo = (nsWindowInfo*) mWindowList[windowIndex];
		if ( ( windowInfo )->mWindow.get() == inWindow )
			break;
		windowInfo = NULL;
	}
	
	if ( windowInfo == NULL )	
		return NS_ERROR_INVALID_ARG;
	// Inform the iterators
	PRInt32 index = -1;
  while (++index < mEnumeratorList.Count() ) 
  {
  	((nsWindowEnumerator*)mEnumeratorList[ index ] )->WindowRemoved ( windowIndex );
  }
 
	// Remove from the list and free up 
	mWindowList.RemoveElement( windowInfo );
	
	delete windowInfo;	
	return NS_OK;
}


NS_METHOD nsWindowMediator::GetEnumerator( nsString* inType, nsISimpleEnumerator** outEnumerator )
{
		if ( outEnumerator == NULL )
			return NS_ERROR_INVALID_ARG;
		*outEnumerator = new nsWindowEnumerator( inType, *this );
		if (*outEnumerator )
			return NS_OK;
		
		return NS_ERROR_OUT_OF_MEMORY;
};	
 

PRInt32 nsWindowMediator::AddEnumerator( nsWindowEnumerator* inEnumerator )
{
	return mEnumeratorList.AppendElement( inEnumerator );
}

PRInt32 nsWindowMediator::RemoveEnumerator( nsWindowEnumerator* inEnumerator)
{
	return mEnumeratorList.RemoveElement( inEnumerator );		
}

	
/*
	Returns the window of type inType ( if null return any window type ) which has the most recent
	time stamp
*/
NS_IMETHODIMP nsWindowMediator::GetMostRecentWindow( nsString* inType, nsIDOMWindow** outWindow )
{
	nsWindowInfo* windowInfo = NULL;
	*outWindow = NULL;
	PRInt32 lastTimeStamp = -1;
	PRInt32 count = mWindowList.Count();
	nsIWebShellWindow* mostRecentWindow;
	// Find the most window with the highest time stamp that matches the requested type
	for ( int32 i = 0; i< count; i++ )
	{	
		nsWindowInfo* windowInfo = (nsWindowInfo*) mWindowList[i];
		if ( ( inType == NULL || windowInfo->GetType() == *inType ) && windowInfo->mTimeStamp >= lastTimeStamp )
		{
			mostRecentWindow = windowInfo->mWindow;
			lastTimeStamp = windowInfo->mTimeStamp;
		}
	}
	
	if( mostRecentWindow )
	{
			nsCOMPtr <nsIDOMWindow> DOMWindow;
			if( NS_SUCCEEDED ( GetDOMWindow( mostRecentWindow, DOMWindow  ) ) )
			{	
				*outWindow = DOMWindow;
				return NS_OK;
			}
			else
			{
				return NS_ERROR_FAILURE;
			}
	}
	else
	{
		return NS_ERROR_FAILURE;
	}
}


NS_IMETHODIMP nsWindowMediator::UpdateWindowTimeStamp( nsIWebShellWindow* inWindow )
{
	PRInt32 count = mWindowList.Count();
	for ( int32 i = 0; i< count; i++ )
	{	
		nsWindowInfo* windowInfo = (nsWindowInfo*) mWindowList[i];
		if (  windowInfo->mWindow.get() == inWindow ) 
		{
			mTimeStamp++;
			windowInfo->mTimeStamp = mTimeStamp;
			return NS_OK;
		}
	}
	
	return NS_ERROR_FAILURE;   
}


NS_IMETHODIMP  nsWindowMediator::UpdateWindowTitle( nsIWebShellWindow* inWindow, const nsString& inTitle )
{
	PRInt32 count = mWindowList.Count();
	nsresult rv;
	for ( int32 i = 0; i< count; i++ )
	{	
		nsWindowInfo* windowInfo = (nsWindowInfo*) mWindowList[i];
		if (  windowInfo->mWindow.get() == inWindow ) 
		{
			nsIRDFResource* window = windowInfo->mRDFID;
			// Get rid of the old value
			nsIRDFNode* target = NULL;
			if ( NS_SUCCEEDED(mInner->GetTarget( window, kNC_Name, PR_TRUE, &target) ) )
			{
				mInner->Unassert( window, kNC_Name, target );
				NS_IF_RELEASE( target );
			}
            
            // Add new title             
			nsCOMPtr<nsIRDFLiteral> newTitle;
			if (NS_FAILED(rv = gRDFService->GetLiteral( inTitle.GetUnicode(), getter_AddRefs(newTitle))))
			{
				NS_ERROR("unable to create literal for window name");
				return rv;
			}

			rv = Assert( window , kNC_Name, newTitle, PR_TRUE );
  			if (rv != NS_RDF_ASSERTION_ACCEPTED)
			{
				NS_ERROR("unable to set window name");
			}
			return NS_OK;
		}
	} 
	return NS_ERROR_FAILURE;
}

NS_IMETHODIMP  nsWindowMediator::GetWindowForResource( nsIRDFResource* inResource, nsIDOMWindow** outWindow )
{
	if ( outWindow == NULL )
		return NS_ERROR_INVALID_ARG;
		*outWindow = NULL;
	PRInt32 count = mWindowList.Count();
	// Find the window
	for ( int32 i = 0; i< count; i++ )
	{	
		nsWindowInfo* windowInfo = (nsWindowInfo*) mWindowList[i];
		PRBool result = PR_FALSE;
		if (  windowInfo->mRDFID->EqualsResource( inResource, &result)  && result == PR_TRUE ) 
		{
			nsCOMPtr <nsIDOMWindow> DOMWindow;
			if( NS_SUCCEEDED ( GetDOMWindow( windowInfo->mWindow, DOMWindow  ) ) )
			{	
				*outWindow = DOMWindow;
				return NS_OK;
			}
			else
				return NS_ERROR_FAILURE;
		}
	} 
	return NS_ERROR_FAILURE;
}

// COM
NS_IMPL_ADDREF( nsWindowMediator );
NS_IMPL_RELEASE( nsWindowMediator );

NS_IMETHODIMP nsWindowMediator::QueryInterface(REFNSIID iid, void **result)
{
	if (! result)
		return NS_ERROR_NULL_POINTER;

	*result = nsnull;
	if (iid.Equals(kIRDFDataSourceIID) ||
		iid.Equals(kIWindowMediatorIID))
	{
		*result = NS_STATIC_CAST(nsIWindowMediator *, this);
		AddRef();
		return NS_OK;
	}
	return(NS_NOINTERFACE);
}

// RDF
NS_IMETHODIMP nsWindowMediator::Init(const char* uri)
{
	nsresult rv;

	if (NS_FAILED(rv = nsComponentManager::CreateInstance(kRDFInMemoryDataSourceCID,
	                                                nsnull,
	                                                kIRDFDataSourceIID,
	                                                (void**) &mInner)))
	{
		return rv;
	}

	if (NS_FAILED(rv = mInner->Init(uri)))
	{
	 	NS_ERROR( "unable to init mInner");
		return rv;
	}
	
	NS_WITH_SERVICE(nsIRDFContainerUtils, rdfc, kRDFContainerUtilsCID, &rv);
	if (NS_FAILED(rv))
		return rv;

	rv = rdfc->MakeSeq(mInner, kNC_WindowMediatorRoot, NULL );
	NS_ASSERTION(NS_SUCCEEDED(rv), "Unable to make NC:WindowMediatorRoot a sequence");
	if (NS_FAILED(rv))
		return rv;

  // register this as a named data source with the RDF service
  return gRDFService->RegisterDataSource(this, PR_FALSE);
}

NS_IMETHODIMP nsWindowMediator::Assert(nsIRDFResource* aSource,
                               nsIRDFResource* aProperty,
                               nsIRDFNode* aTarget,
                               PRBool aTruthValue)
{
    // XXX TODO: filter out asserts we don't care about
    return mInner->Assert(aSource, aProperty, aTarget, aTruthValue);
}

NS_IMETHODIMP nsWindowMediator::Unassert(nsIRDFResource* aSource,
                                 nsIRDFResource* aProperty,
                                 nsIRDFNode* aTarget)
{
    // XXX TODO: filter out unasserts we don't care about
    return mInner->Unassert(aSource, aProperty, aTarget);
}


NS_IMETHODIMP nsWindowMediator::Flush(void)
{
    NS_NOTYETIMPLEMENTED("write me!");
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP nsWindowMediator::GetAllCommands(nsIRDFResource* source,
                                       nsIEnumerator/*<nsIRDFResource>*/** commands)
{
    NS_NOTYETIMPLEMENTED("write me!");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsWindowMediator::IsCommandEnabled(nsISupportsArray/*<nsIRDFResource>*/* aSources,
                                         nsIRDFResource*   aCommand,
                                         nsISupportsArray/*<nsIRDFResource>*/* aArguments,
                                         PRBool* aResult)
{
    NS_NOTYETIMPLEMENTED("write me!");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsWindowMediator::DoCommand(nsISupportsArray* aSources,
                                  nsIRDFResource*   aCommand,
                                  nsISupportsArray* aArguments)
{
    NS_NOTYETIMPLEMENTED("write me!");
    return NS_ERROR_NOT_IMPLEMENTED;
}


// Private helpers
nsresult nsWindowMediator::AddWindowToRDF( nsWindowInfo* ioWindowInfo )
{
	if ( ioWindowInfo == NULL || ioWindowInfo->mWindow == NULL )
		return PR_FALSE;
		
	nsCOMPtr<nsIRDFResource> window;
	nsresult rv;	
	
	nsString	windowTitle;
	
	// Make up a unique ID and create the RDF NODE
	nsString uniqueID = "window-";
	uniqueID.Append(windowTitle );
	uniqueID.Append( mTimeStamp, 10 );
	char cID[ 256];
	uniqueID.ToCString( cID, 256);
	if (NS_FAILED(rv = gRDFService->GetResource(cID, getter_AddRefs(window) )))
	{
		NS_ERROR("unable to get window resource");
		return rv;
	}
	ioWindowInfo->mRDFID = window ;
		
	// Get the RDF literal and add it to our node 
	nsCOMPtr<nsIRDFLiteral> windowTitleLiteral;
	if (NS_FAILED(rv = gRDFService->GetLiteral( windowTitle.GetUnicode(), getter_AddRefs(windowTitleLiteral))))
	{
		NS_ERROR("unable to create literal for window name");
		return rv;
	}

	rv = Assert(window, kNC_Name, windowTitleLiteral, PR_TRUE);
    if (rv != NS_RDF_ASSERTION_ACCEPTED)
	{
		NS_ERROR("unable to set window name");
		return rv;
	}
	
	// Add the element to the container
  nsCOMPtr<nsIRDFContainer> container;
  rv = NS_NewRDFContainer(mInner, kNC_WindowMediatorRoot, getter_AddRefs(container));
  if (NS_FAILED(rv))
  	return rv;

	if (NS_FAILED(rv = container->AppendElement( window ) ) /* rdf_ContainerAppendElement(mInner, kNC_WindowMediatorRoot, window) */ )
	{
		NS_ERROR("unable to add window to container");
		return rv;
	}
	return NS_OK;
}


//----------------------------------------------------------------------

// Entry point to create nsAppShellService factory instances...
NS_DEF_FACTORY(WindowMediator, nsWindowMediator)

nsresult NS_NewWindowMediatorFactory(nsIFactory** aResult)
{
  nsresult rv = NS_OK;
  nsIFactory* inst;
  
  inst = new nsWindowMediatorFactory;
  if (nsnull == inst)
  {
    rv = NS_ERROR_OUT_OF_MEMORY;
  }
  else
  {
    NS_ADDREF(inst);
  }
  *aResult = inst;
  return rv;
}

// window Enumerator
nsWindowEnumerator::nsWindowEnumerator ( nsString* inTypeString, nsWindowMediator& inMediator )
	:mWindowMediator( &inMediator )
{
	NS_INIT_REFCNT();
	if ( inTypeString )
	{
		mType = *inTypeString;
	}
	mWindowMediator->AddEnumerator( this );
	NS_ADDREF( this );
}

nsWindowEnumerator::~nsWindowEnumerator()
{
	mWindowMediator->RemoveEnumerator( this );
}

NS_IMETHODIMP nsWindowEnumerator::HasMoreElements(PRBool *retval)
{
	if ( !retval )
		return NS_ERROR_INVALID_ARG;
		
	*retval = PR_FALSE;
	if ( FindNext() != -1 )
		*retval = PR_TRUE;
	return NS_OK;
}
	
NS_IMETHODIMP nsWindowEnumerator::GetNext(nsISupports **retval)
{
	if ( !retval )
		return NS_ERROR_INVALID_ARG;
		
	PRInt32 index = FindNext();
	if ( index >= 0 )
	{
		nsWindowInfo* windowInfo = (nsWindowInfo*) mWindowMediator->mWindowList[index];
		*retval = windowInfo->mWindow;
		NS_ADDREF( *retval );
	}
	return NS_OK;
}

PRInt32 nsWindowEnumerator::FindNext()
{
	// Now find the window
	PRInt32 i = mCurrentPosition;
	PRInt32 numWindows =  mWindowMediator->mWindowList.Count();
	PRBool allWindows = (mType.Length() == 0);
	for ( i++;i < numWindows; i++ )
	{
		nsWindowInfo* windowInfo = (nsWindowInfo*) mWindowMediator->mWindowList[i];
		if (  allWindows  || ( windowInfo->GetType() == mType ) )
		{
			return i;
		}
	}
	return -1;
}

// if a window is being removed adjust the iterator's current position
void nsWindowEnumerator::WindowRemoved( PRInt32 inIndex)
{
	if ( inIndex <= mCurrentPosition )
	{
			mCurrentPosition--;
	}
}

/* 
 * Implementations of nsISupports interface methods...
 */
NS_IMPL_ADDREF(nsWindowEnumerator);
NS_IMPL_RELEASE(nsWindowEnumerator);
NS_IMPL_QUERY_INTERFACE(nsWindowEnumerator, kISimpleEnumberatorIID);
