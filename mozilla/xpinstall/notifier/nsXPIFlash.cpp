/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
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
 * The Original Code is Mozilla Communicator client code, 
 * released March 31, 1998. 
 *
 * The Initial Developer of the Original Code is Netscape Communications 
 * Corporation.  Portions created by Netscape are
 * Copyright (C) 1998-1999 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 *   Doug Turner <dougt@netscape.com>
 *   Pierre Phaneuf <pp@ludusdesign.com>
 */
#include "nsISupports.h"
#include "nsIServiceManager.h"
#include "nsCOMPtr.h"
#include "nsIComponentManager.h"
#include "nsIGenericFactory.h"
#include "nsIRDFContainer.h"
#include "nsIRDFDataSource.h"
#include "nsIRDFService.h"
#include "nsIRDFRemoteDataSource.h"
#include "nsRDFCID.h"
#include "nsIRDFXMLSink.h"

#include "VerReg.h"
#include "nsIPref.h"
#include "nsISoftwareUpdate.h"

#define NC_RDF_NAME	         	"http://home.netscape.com/NC-rdf#name"
#define NC_RDF_SOURCE			"http://home.netscape.com/NC-rdf#source"
#define NC_RDF_URL				"http://home.netscape.com/NC-rdf#url"
#define NC_RDF_CHILD			"http://home.netscape.com/NC-rdf#child"

#define NC_RDF_NOTIFICATION_ROOT		"NC:SoftwareNotificationRoot"
#define NC_RDF_NOTIFICATION_LIST		"NC:SoftwareNotificationList"


#define NC_XPI_SOURCES     "NC:SoftwareUpdateDataSources"
#define NC_XPI_PACKAGES    "NC:SoftwarePackages"

#define NC_XPI_TITLE        "http://home.netscape.com/NC-rdf#title"
#define NC_XPI_REGKEY       "http://home.netscape.com/NC-rdf#registryKey"
#define NC_XPI_VERSION      "http://home.netscape.com/NC-rdf#version"
#define NC_XPI_DESCRIPTION	"http://home.netscape.com/NC-rdf#description"

static NS_DEFINE_CID(kRDFServiceCID,   NS_RDFSERVICE_CID);
static NS_DEFINE_CID(kRDFContainerCID, NS_RDFCONTAINER_CID);

class nsXPINotifierImpl : public nsIRDFXMLSinkObserver
{

public:
    static NS_IMETHODIMP New(nsISupports* aOuter, REFNSIID aIID, void** aResult);
    
    // nsISupports interface
    NS_DECL_ISUPPORTS
        
    NS_DECL_NSIRDFXMLSINKOBSERVER
        
protected:
    
    nsXPINotifierImpl();
    virtual ~nsXPINotifierImpl();

    nsresult NotificationEnabled(PRBool* aReturn);
    nsresult Init();
    nsresult OpenRemoteDataSource(const char* aURL, PRBool blocking, nsIRDFDataSource** aResult);
    
    PRBool IsNewerOrUninstalled(const char* regKey, const char* versionString);
    PRInt32 CompareVersions(VERSION *oldversion, VERSION *newVersion);
    void   StringToVersionNumbers(const nsString& version, int32 *aMajor, int32 *aMinor, int32 *aRelease, int32 *aBuild);
    
    nsCOMPtr<nsISupports> mInner;
    nsIRDFService* mRDF;

    static nsIRDFResource* kXPI_NotifierSources;
    static nsIRDFResource* kXPI_NotifierPackages;
    static nsIRDFResource* kXPI_NotifierPackage_Title;
    static nsIRDFResource* kXPI_NotifierPackage_Version;
    static nsIRDFResource* kXPI_NotifierPackage_Description;
    static nsIRDFResource* kXPI_NotifierPackage_RegKey;

    static nsIRDFResource* kNC_NotificationRoot;
    static nsIRDFResource* kNC_NotificationList;
	static nsIRDFResource* kNC_Source;
	static nsIRDFResource* kNC_Name;
	static nsIRDFResource* kNC_URL;
	static nsIRDFResource* kNC_Child;
	
};

nsIRDFResource* nsXPINotifierImpl::kXPI_NotifierSources = nsnull;
nsIRDFResource* nsXPINotifierImpl::kXPI_NotifierPackages = nsnull;
nsIRDFResource* nsXPINotifierImpl::kXPI_NotifierPackage_Title = nsnull;
nsIRDFResource* nsXPINotifierImpl::kXPI_NotifierPackage_Version = nsnull;
nsIRDFResource* nsXPINotifierImpl::kXPI_NotifierPackage_Description = nsnull;
nsIRDFResource* nsXPINotifierImpl::kXPI_NotifierPackage_RegKey = nsnull;

nsIRDFResource* nsXPINotifierImpl::kNC_NotificationRoot = nsnull;
nsIRDFResource* nsXPINotifierImpl::kNC_NotificationList = nsnull;
nsIRDFResource* nsXPINotifierImpl::kNC_Source = nsnull;
nsIRDFResource* nsXPINotifierImpl::kNC_Name = nsnull;
nsIRDFResource* nsXPINotifierImpl::kNC_URL = nsnull;
nsIRDFResource* nsXPINotifierImpl::kNC_Child = nsnull;


nsXPINotifierImpl::nsXPINotifierImpl()
    : mRDF(nsnull)
{
    NS_INIT_REFCNT();


    static NS_DEFINE_CID(kRDFInMemoryDataSourceCID, NS_RDFINMEMORYDATASOURCE_CID);
    
    nsComponentManager::CreateInstance(kRDFInMemoryDataSourceCID,
                                       this, /* the "outer" */
                                       NS_GET_IID(nsISupports),
                                       getter_AddRefs(mInner));
}


nsXPINotifierImpl::~nsXPINotifierImpl()
{
    if (mRDF) 
    {
        nsServiceManager::ReleaseService(kRDFServiceCID, mRDF);
        mRDF = nsnull;
    }

    NS_IF_RELEASE(kXPI_NotifierSources);
    NS_IF_RELEASE(kXPI_NotifierPackages);
    NS_IF_RELEASE(kXPI_NotifierPackage_Title);
    NS_IF_RELEASE(kXPI_NotifierPackage_Version);
    NS_IF_RELEASE(kXPI_NotifierPackage_Description);
    NS_IF_RELEASE(kXPI_NotifierPackage_RegKey);

   	NS_IF_RELEASE(kNC_NotificationRoot);
    NS_IF_RELEASE(kNC_NotificationList);
	NS_IF_RELEASE(kNC_Source);
	NS_IF_RELEASE(kNC_Name);
	NS_IF_RELEASE(kNC_URL);
	NS_IF_RELEASE(kNC_Child);
}


static NS_DEFINE_IID(kPrefsIID, NS_IPREF_IID);
static NS_DEFINE_IID(kPrefsCID,  NS_PREF_CID);

nsresult
nsXPINotifierImpl::NotificationEnabled(PRBool* aReturn)
{
    *aReturn = PR_FALSE;


    nsIPref * prefs;
    nsresult rv = nsServiceManager::GetService(kPrefsCID, 
                                               kPrefsIID,
                                               (nsISupports**) &prefs);

    
    
    if ( NS_SUCCEEDED(rv) )
    {
        PRBool value;
        // check to see if we are on.
        rv = prefs->GetBoolPref( (const char*) XPINSTALL_NOTIFICATIONS_ENABLE, &value);

        if (NS_SUCCEEDED(rv) && value)
        {
            // check to see the last time we did anything.  Since flash does not have a persistant
            // way to do poll invervals longer than a session, we will implemented that here by using the
            // preferences.  

            PRInt32 intervalHours = 0;
            
            PRTime now            = 0;
            PRInt32 nowSec        = 0;

            PRInt32 lastTime      = 0;
            
            rv = prefs->GetIntPref(XPINSTALL_NOTIFICATIONS_INTERVAL, &intervalHours);

            if (NS_FAILED(rv))
            {
                intervalHours = 7*24;  // default at once a week
                rv = prefs->SetIntPref(XPINSTALL_NOTIFICATIONS_INTERVAL, intervalHours);
            }

            rv = prefs->GetIntPref(XPINSTALL_NOTIFICATIONS_LASTDATE, &lastTime);
    
            now = PR_Now();

            // nowSec = now / 1000000
            LL_DIV(nowSec, now, 1000000);

            if (NS_FAILED(rv) || lastTime == 0)
            {
                rv = prefs->SetIntPref(XPINSTALL_NOTIFICATIONS_LASTDATE, nowSec);
                return NS_OK;
            }
            
            if ((lastTime + (intervalHours*60*24)) <= nowSec)
            {
                *aReturn = PR_TRUE;
            }

            NS_RELEASE(prefs);
        }
    }
    
    return NS_OK;
}

nsresult
nsXPINotifierImpl::Init()
{
    PRBool enabled;

    NotificationEnabled(&enabled);

    if (!enabled)
        return NS_ERROR_FAILURE;
    
    if (mInner == nsnull)
        return NS_ERROR_FAILURE;

    nsresult rv;
    nsCOMPtr<nsIRDFDataSource> distributors;
    nsCOMPtr<nsIRDFContainer> distributorsContainer;
    nsCOMPtr <nsISimpleEnumerator> distributorEnumerator;
    PRBool moreElements;
    
    // Read the distributor registry
    rv = nsServiceManager::GetService(kRDFServiceCID, NS_GET_IID(nsIRDFService), (nsISupports**) &mRDF);
    if (NS_FAILED(rv)) return rv;
    
    if (! kXPI_NotifierSources)
	{
	   mRDF->GetResource(NC_XPI_SOURCES,       &kXPI_NotifierSources);
       mRDF->GetResource(NC_XPI_PACKAGES,      &kXPI_NotifierPackages);
       mRDF->GetResource(NC_XPI_TITLE,         &kXPI_NotifierPackage_Title);
       mRDF->GetResource(NC_XPI_VERSION,       &kXPI_NotifierPackage_Version);
       mRDF->GetResource(NC_XPI_DESCRIPTION,   &kXPI_NotifierPackage_Description);
       mRDF->GetResource(NC_XPI_REGKEY,        &kXPI_NotifierPackage_RegKey);

       mRDF->GetResource(NC_RDF_NOTIFICATION_ROOT,  &kNC_NotificationRoot);
       mRDF->GetResource(NC_RDF_NOTIFICATION_LIST,  &kNC_NotificationList);
	   
       mRDF->GetResource(NC_RDF_SOURCE,             &kNC_Source);
	   mRDF->GetResource(NC_RDF_NAME,               &kNC_Name);
	   mRDF->GetResource(NC_RDF_URL,                &kNC_URL);
	   mRDF->GetResource(NC_RDF_CHILD,              &kNC_Child);

	}

    rv = OpenRemoteDataSource("resource:/res/xpinstall/SoftwareUpdates.rdf", PR_TRUE, getter_AddRefs(distributors));
    if (NS_FAILED(rv)) return rv;

    rv = nsComponentManager::CreateInstance(kRDFContainerCID,
                                            nsnull,
                                            NS_GET_IID(nsIRDFContainer),
                                            getter_AddRefs(distributorsContainer));

    if (NS_SUCCEEDED(rv))
    {
        rv = distributorsContainer->Init(distributors, kXPI_NotifierSources);
        
        if (NS_SUCCEEDED(rv))
        {
            rv = distributorsContainer->GetElements(getter_AddRefs(distributorEnumerator));

            if (NS_SUCCEEDED(rv))
            {
                distributorEnumerator->HasMoreElements(&moreElements);
                while (moreElements) 
                {
                    nsCOMPtr<nsISupports> i;
                    rv = distributorEnumerator->GetNext(getter_AddRefs(i));
                    if (NS_FAILED(rv)) break;

                    nsCOMPtr<nsIRDFResource> aDistributor(do_QueryInterface(i, &rv));
                    if (NS_FAILED(rv)) break;

                    char* uri;
                    nsCOMPtr<nsIRDFDataSource> remoteDatasource;
                    aDistributor->GetValue(&uri);

                    rv = OpenRemoteDataSource(uri, PR_FALSE, getter_AddRefs(remoteDatasource));
					nsAllocator::Free(uri);
                    if (NS_FAILED(rv)) break;
                    
                    distributorEnumerator->HasMoreElements(&moreElements);
                }
            }
        }
    }
    return NS_OK;
}


PRBool 
nsXPINotifierImpl::IsNewerOrUninstalled(const char* regKey, const char* versionString)
{
    PRBool needJar = PR_FALSE;

    REGERR status = VR_ValidateComponent( (char*) regKey );

    if ( status == REGERR_NOFIND || status == REGERR_NOFILE )
    {
        // either component is not in the registry or it's a file
        // node and the physical file is missing
        needJar = PR_TRUE;
    }
    else
    {
        VERSION oldVersion;

        status = VR_GetVersion( (char*)regKey, &oldVersion );
        
        if ( status != REGERR_OK )
        {
            needJar = PR_TRUE;
        }
        else 
        {
            VERSION newVersion;

            StringToVersionNumbers(versionString, &(newVersion).major, &(newVersion).minor, &(newVersion).release, &(newVersion).build);
            
            if ( CompareVersions(&oldVersion, &newVersion) < 0 )
                needJar = PR_TRUE;
        }
    }
    return needJar;
}


PRInt32
nsXPINotifierImpl::CompareVersions(VERSION *oldversion, VERSION *newVersion)
{
    PRInt32 diff;
    
    if ( oldversion->major == newVersion->major ) 
    {
        if ( oldversion->minor == newVersion->minor ) 
        {
            if ( oldversion->release == newVersion->release ) 
            {
                if ( oldversion->build == newVersion->build )
                    diff = 0;
                else if ( oldversion->build > newVersion->build )
                    diff = 1;
                else
                    diff = -1;
            }
            else if ( oldversion->release > newVersion->release )
                diff = 1;
            else
                diff = -1;
        }
        else if (  oldversion->minor > newVersion->minor )
            diff = 1;
        else
            diff = -1;
    }
    else if ( oldversion->major > newVersion->major )
        diff = 1;
    else
        diff = -1;

    return diff;
}


void
nsXPINotifierImpl::StringToVersionNumbers(const nsString& version, int32 *aMajor, int32 *aMinor, int32 *aRelease, int32 *aBuild)    
{
    PRInt32 errorCode;

    int dot = version.FindChar('.', PR_FALSE,0);
    
    if ( dot == -1 ) 
    {
        *aMajor = version.ToInteger(&errorCode);
    }
    else  
    {
        nsString majorStr;
        version.Mid(majorStr, 0, dot);
        *aMajor  = majorStr.ToInteger(&errorCode);

        int prev = dot+1;
        dot = version.FindChar('.',PR_FALSE,prev);
        if ( dot == -1 ) 
        {
            nsString minorStr;
            version.Mid(minorStr, prev, version.Length() - prev);
            *aMinor = minorStr.ToInteger(&errorCode);
        }
        else 
        {
            nsString minorStr;
            version.Mid(minorStr, prev, dot - prev);
            *aMinor = minorStr.ToInteger(&errorCode);

            prev = dot+1;
            dot = version.FindChar('.',PR_FALSE,prev);
            if ( dot == -1 ) 
            {
                nsString releaseStr;
                version.Mid(releaseStr, prev, version.Length() - prev);
                *aRelease = releaseStr.ToInteger(&errorCode);
            }
            else 
            {
                nsString releaseStr;
                version.Mid(releaseStr, prev, dot - prev);
                *aRelease = releaseStr.ToInteger(&errorCode);
    
                prev = dot+1;
                if ( version.Length() > dot ) 
                {
                    nsString buildStr;
                    version.Mid(buildStr, prev, version.Length() - prev);
                    *aBuild = buildStr.ToInteger(&errorCode);
               }
            }
        }
    }
}

nsresult
nsXPINotifierImpl::OpenRemoteDataSource(const char* aURL, PRBool blocking, nsIRDFDataSource** aResult)
{
    static NS_DEFINE_CID(kRDFXMLDataSourceCID, NS_RDFXMLDATASOURCE_CID);
    nsresult rv;

    nsCOMPtr<nsIRDFRemoteDataSource> remote;
    rv = nsComponentManager::CreateInstance(kRDFXMLDataSourceCID,
                                            nsnull,
                                            NS_GET_IID(nsIRDFRemoteDataSource),
                                            getter_AddRefs(remote));
    if (NS_FAILED(rv)) return rv;

    rv = remote->Init(aURL);
    if (NS_SUCCEEDED(rv)) 
    {
        if (! blocking)
        {
            nsCOMPtr<nsIRDFXMLSink> sink = do_QueryInterface(remote, &rv);
            if (NS_FAILED(rv)) return rv;

            rv = sink->AddXMLSinkObserver(this);
            if (NS_FAILED(rv)) return rv;
        }

        rv = remote->Refresh(blocking);
        if (NS_FAILED(rv)) return rv;

        nsCOMPtr<nsIRDFDataSource> result = do_QueryInterface(remote, &rv);
        *aResult = result;
        NS_IF_ADDREF(*aResult);
        return rv;
    }
    else 
    {
        // we've already loaded this datasource. use cached copy
        return mRDF->GetDataSource(aURL, aResult);
    }
}


NS_IMETHODIMP
nsXPINotifierImpl::New(nsISupports* aOuter, REFNSIID aIID, void** aResult)
{
    NS_PRECONDITION(aOuter == nsnull, "no aggregation");
    if (aOuter)
        return NS_ERROR_NO_AGGREGATION;

    nsXPINotifierImpl* result = new nsXPINotifierImpl();
    if (! result)
        return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(result); // stabilize

    nsresult rv;
    rv = result->Init();
    if (NS_SUCCEEDED(rv)) {
        rv = result->QueryInterface(aIID, aResult);
    }

    NS_RELEASE(result);
    return rv;
}


NS_IMETHODIMP
nsXPINotifierImpl::OnBeginLoad(nsIRDFXMLSink *aSink)
{
    return NS_OK;
}

NS_IMETHODIMP
nsXPINotifierImpl::OnInterrupt(nsIRDFXMLSink *aSink)
{
    return NS_OK;
}
NS_IMETHODIMP
nsXPINotifierImpl::OnResume(nsIRDFXMLSink *aSink)
{
    return NS_OK;
}

NS_IMETHODIMP
nsXPINotifierImpl::OnEndLoad(nsIRDFXMLSink *aSink)
{
    nsresult rv;

    (void) aSink->RemoveXMLSinkObserver(this);

    nsCOMPtr<nsIRDFDataSource> distributorDataSource = do_QueryInterface(aSink, &rv);
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIRDFContainer> distributorContainer;
    nsCOMPtr <nsISimpleEnumerator> packageEnumerator;
    PRBool moreElements;

    rv = nsComponentManager::CreateInstance(kRDFContainerCID,
                                            nsnull,
                                            NS_GET_IID(nsIRDFContainer),
                                            getter_AddRefs(distributorContainer));
    if (NS_SUCCEEDED(rv))
    {
        rv = distributorContainer->Init(distributorDataSource, kXPI_NotifierPackages);
        if (NS_SUCCEEDED(rv))
        {
            rv = distributorContainer->GetElements(getter_AddRefs(packageEnumerator));
            if (NS_SUCCEEDED(rv))
            {
                packageEnumerator->HasMoreElements(&moreElements);
                while (moreElements) 
                {
                    nsCOMPtr<nsISupports> i;

                    rv = packageEnumerator->GetNext(getter_AddRefs(i));
                    if (NS_FAILED(rv)) break;

                    nsCOMPtr<nsIRDFResource> aPackage(do_QueryInterface(i, &rv));
                    if (NS_FAILED(rv)) break;
    
                    
                    // Get the version information
                    nsCOMPtr<nsIRDFNode> versionNode;
                    distributorDataSource->GetTarget(aPackage, 
                                                     kXPI_NotifierPackage_Version, 
                                                     PR_TRUE, 
                                                     getter_AddRefs(versionNode));

                    nsCOMPtr<nsIRDFLiteral> version(do_QueryInterface(versionNode, &rv));
                    if (NS_FAILED(rv)) break;

                    // Get the regkey information
                    nsCOMPtr<nsIRDFNode> regkeyNode;
                    distributorDataSource->GetTarget(aPackage, 
                                                     kXPI_NotifierPackage_RegKey, 
                                                     PR_TRUE, 
                                                     getter_AddRefs(regkeyNode));

                    nsCOMPtr<nsIRDFLiteral> regkey(do_QueryInterface(regkeyNode, &rv));
                    if (NS_FAILED(rv)) break;

                    // convert them into workable nsAutoStrings
                    PRUnichar* regkeyCString;
                    regkey->GetValue(&regkeyCString);
                    nsString regKeyString(regkeyCString);
                    
                    PRUnichar* versionCString;
                    version->GetValue(&versionCString);
                    nsString versionString(versionCString);
					nsAllocator::Free(versionCString);
					nsAllocator::Free(regkeyCString);

                    // check to see if this software title should be "flashed"
                    if (IsNewerOrUninstalled(nsAutoCString(regKeyString), nsAutoCString(versionString)))
                    {
                        //assert into flash
                        
                        nsCOMPtr<nsIRDFNode> urlNode;
                        distributorDataSource->GetTarget(kXPI_NotifierPackages, 
                                                         kNC_URL, 
                                                         PR_TRUE, 
                                                         getter_AddRefs(urlNode));

                        nsCOMPtr<nsIRDFLiteral> url(do_QueryInterface(urlNode, &rv));
                        if (NS_FAILED(rv)) break;


                        nsCOMPtr<nsIRDFNode> titleNode;
                        distributorDataSource->GetTarget(kXPI_NotifierPackages, 
                                                         kXPI_NotifierPackage_Title, 
                                                         PR_TRUE, 
                                                         getter_AddRefs(titleNode));

                        nsCOMPtr<nsIRDFLiteral> title(do_QueryInterface(titleNode, &rv));
                        if (NS_FAILED(rv)) break;

                        nsCOMPtr<nsIRDFDataSource> ds = do_QueryInterface(mInner);

                        ds->Assert(aPackage, kNC_Name, title, PR_TRUE);
                        ds->Assert(aPackage, kNC_URL, url, PR_TRUE);

                        ds->Assert(kNC_NotificationRoot, kNC_Child, aPackage, PR_TRUE);
                        break;

                    }
                }
            }
        }
    }
	VR_Close();
    return NS_OK;
}


NS_IMETHODIMP
nsXPINotifierImpl::OnError(nsIRDFXMLSink *aSink, nsresult aResult, const PRUnichar* aErrorMsg)
{
    (void) aSink->RemoveXMLSinkObserver(this);
    return NS_OK;
}


////////////////////////////////////////////////////////////////////////
// nsISupports

NS_IMPL_ADDREF(nsXPINotifierImpl);
NS_IMPL_RELEASE(nsXPINotifierImpl);

NS_IMETHODIMP
nsXPINotifierImpl::QueryInterface(REFNSIID aIID, void** aResult)
{
    NS_PRECONDITION(aResult != nsnull, "null ptr");
    if (! aResult)
        return NS_ERROR_NULL_POINTER;

    static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);

    if (aIID.Equals(kISupportsIID)) 
    {
        *aResult = NS_STATIC_CAST(nsISupports*, this);
    }
    else if (aIID.Equals(NS_GET_IID(nsIRDFDataSource))) 
    {
        if (mInner)
            return mInner->QueryInterface(aIID, aResult);
    }
    else if (aIID.Equals(NS_GET_IID(nsIRDFXMLSinkObserver))) 
    {
        *aResult = NS_STATIC_CAST(nsIRDFXMLSinkObserver*, this);
    }
    else 
    {
        *aResult = nsnull;
        return NS_NOINTERFACE;
    }

    NS_ADDREF(this);
    return NS_OK;
}



////////////////////////////////////////////////////////////////////////
// Component Manager Related Exports

// {69FDC800-4050-11d3-BE2F-00104BDE6048}
#define NS_XPIUPDATENOTIFIER_CID \
{ 0x69fdc800, 0x4050, 0x11d3, { 0xbe, 0x2f, 0x0, 0x10, 0x4b, 0xde, 0x60, 0x48 } }

static nsModuleComponentInfo components[] = 
{
    { "XPInstall Update Notifier", 
      NS_XPIUPDATENOTIFIER_CID,
      "component://netscape/rdf/datasource?name=xpinstall-update-notifier", 
      nsXPINotifierImpl::New
    }
};

NS_IMPL_NSGETMODULE("XPInstallUpdateNotifierModule", components);
