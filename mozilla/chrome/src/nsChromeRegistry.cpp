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

#include "nsCOMPtr.h"
#include "nsFileSpec.h"
#include "nsSpecialSystemDirectory.h"
#include "nsIChromeRegistry.h"
#include "nsIRDFDataSource.h"
#include "nsIRDFObserver.h"
#include "nsIRDFRemoteDataSource.h"
#include "nsCRT.h"
#include "rdf.h"
#include "nsIServiceManager.h"
#include "nsIRDFService.h"
#include "nsRDFCID.h"
#include "nsIRDFResource.h"
#include "nsIRDFDataSource.h"
#include "nsHashtable.h"
#include "nsString.h"
#include "nsXPIDLString.h"

static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);
static NS_DEFINE_IID(kIRDFResourceIID, NS_IRDFRESOURCE_IID);
static NS_DEFINE_IID(kIRDFLiteralIID, NS_IRDFLITERAL_IID);
static NS_DEFINE_IID(kIRDFServiceIID, NS_IRDFSERVICE_IID);
static NS_DEFINE_IID(kIRDFDataSourceIID, NS_IRDFDATASOURCE_IID);
static NS_DEFINE_IID(kIRDFIntIID, NS_IRDFINT_IID);
static NS_DEFINE_CID(kRDFServiceCID, NS_RDFSERVICE_CID);
static NS_DEFINE_CID(kRDFXMLDataSourceCID, NS_RDFXMLDATASOURCE_CID);

#define CHROME_NAMESPACE_URI "http://chrome.mozilla.org/rdf#"
DEFINE_RDF_VOCAB(CHROME_NAMESPACE_URI, CHROME, chrome);
DEFINE_RDF_VOCAB(CHROME_NAMESPACE_URI, CHROME, skin);
DEFINE_RDF_VOCAB(CHROME_NAMESPACE_URI, CHROME, content);
DEFINE_RDF_VOCAB(CHROME_NAMESPACE_URI, CHROME, locale);
DEFINE_RDF_VOCAB(CHROME_NAMESPACE_URI, CHROME, base);
DEFINE_RDF_VOCAB(CHROME_NAMESPACE_URI, CHROME, main);
DEFINE_RDF_VOCAB(CHROME_NAMESPACE_URI, CHROME, archive);
DEFINE_RDF_VOCAB(CHROME_NAMESPACE_URI, CHROME, displayname);
DEFINE_RDF_VOCAB(CHROME_NAMESPACE_URI, CHROME, name);

// This nasty function should disappear when we land Necko completely and 
// change chrome://global/skin/foo to chrome://skin@global/foo
//
void BreakProviderAndRemainingFromPath(const char* i_path, char** o_provider, char** o_remaining);

////////////////////////////////////////////////////////////////////////////////

class nsChromeRegistry : public nsIChromeRegistry,
                         public nsIRDFDataSource,
                         public nsIRDFRemoteDataSource
{
public:
    NS_DECL_ISUPPORTS

    // nsIChromeRegistry methods:
    NS_IMETHOD InitRegistry();  
    NS_IMETHOD ConvertChromeURL(nsIURI* aChromeURL);

    // nsIRDFDataSource methods
    NS_IMETHOD GetURI(char** uri);
    NS_IMETHOD GetSource(nsIRDFResource* property,
                         nsIRDFNode* target,
                         PRBool tv,
                         nsIRDFResource** source /* out */)  ;
    NS_IMETHOD GetSources(nsIRDFResource* property,
                          nsIRDFNode* target,
                          PRBool tv,
                          nsISimpleEnumerator** sources /* out */)  ;
    NS_IMETHOD GetTarget(nsIRDFResource* source,
                         nsIRDFResource* property,
                         PRBool tv,
                         nsIRDFNode** target /* out */)  ;
    NS_IMETHOD GetTargets(nsIRDFResource* source,
                          nsIRDFResource* property,
                          PRBool tv,
                          nsISimpleEnumerator** targets /* out */)  ;
    NS_IMETHOD Assert(nsIRDFResource* source, 
                      nsIRDFResource* property, 
                      nsIRDFNode* target,
                      PRBool tv)  ;
    NS_IMETHOD Unassert(nsIRDFResource* source,
                        nsIRDFResource* property,
                        nsIRDFNode* target)  ;
    NS_IMETHOD Change(nsIRDFResource* aSource,
                      nsIRDFResource* aProperty,
                      nsIRDFNode* aOldTarget,
                      nsIRDFNode* aNewTarget);
    NS_IMETHOD Move(nsIRDFResource* aOldSource,
                    nsIRDFResource* aNewSource,
                    nsIRDFResource* aProperty,
                    nsIRDFNode* aTarget);
    NS_IMETHOD HasAssertion(nsIRDFResource* source,
                            nsIRDFResource* property,
                            nsIRDFNode* target,
                            PRBool tv,
                            PRBool* hasAssertion /* out */)  ;
    NS_IMETHOD AddObserver(nsIRDFObserver* n)  ;
    NS_IMETHOD RemoveObserver(nsIRDFObserver* n)  ;
    NS_IMETHOD ArcLabelsIn(nsIRDFNode* node,
                           nsISimpleEnumerator** labels /* out */)  ;
    NS_IMETHOD ArcLabelsOut(nsIRDFResource* source,
                            nsISimpleEnumerator** labels /* out */)  ;
    NS_IMETHOD GetAllResources(nsISimpleEnumerator** aResult)  ;
    NS_IMETHOD GetAllCommands(nsIRDFResource* source,
                              nsIEnumerator/*<nsIRDFResource>*/** commands)  ;
    NS_IMETHOD GetAllCmds(nsIRDFResource* source,
                              nsISimpleEnumerator/*<nsIRDFResource>*/** commands)  ;
    NS_IMETHOD IsCommandEnabled(nsISupportsArray/*<nsIRDFResource>*/* aSources,
                                nsIRDFResource*   aCommand,
                                nsISupportsArray/*<nsIRDFResource>*/* aArguments,
                                PRBool* retVal)  ;
    NS_IMETHOD DoCommand(nsISupportsArray/*<nsIRDFResource>*/* aSources,
                         nsIRDFResource*   aCommand,
                         nsISupportsArray/*<nsIRDFResource>*/* aArguments)  ;

    // nsIRDFRemoteDataSource methods
    NS_IMETHOD Init(const char* aURI);
    NS_IMETHOD Refresh(PRBool aBlocking);
    NS_IMETHOD Flush();

    // nsChromeRegistry methods:
    nsChromeRegistry();
    virtual ~nsChromeRegistry();

    static PRUint32 gRefCnt;
    static nsIRDFService* gRDFService;
    static nsIRDFResource* kCHROME_chrome;
    static nsIRDFResource* kCHROME_skin;
    static nsIRDFResource* kCHROME_content;
    static nsIRDFResource* kCHROME_locale;
    static nsIRDFResource* kCHROME_base;
    static nsIRDFResource* kCHROME_main;
    static nsIRDFResource* kCHROME_archive;
    static nsIRDFResource* kCHROME_name;
    static nsIRDFResource* kCHROME_displayname;
    static nsIRDFDataSource* mInner;

protected:
    nsresult GetPackageTypeResource(const nsString& aChromeType, nsIRDFResource** aResult);
    nsresult GetChromeResource(nsString& aResult, nsIRDFResource* aChromeResource,
                               nsIRDFResource* aProperty);    
};

PRUint32 nsChromeRegistry::gRefCnt  ;
nsIRDFService* nsChromeRegistry::gRDFService = nsnull;

nsIRDFResource* nsChromeRegistry::kCHROME_chrome = nsnull;
nsIRDFResource* nsChromeRegistry::kCHROME_skin = nsnull;
nsIRDFResource* nsChromeRegistry::kCHROME_content = nsnull;
nsIRDFResource* nsChromeRegistry::kCHROME_locale = nsnull;
nsIRDFResource* nsChromeRegistry::kCHROME_base = nsnull;
nsIRDFResource* nsChromeRegistry::kCHROME_main = nsnull;
nsIRDFResource* nsChromeRegistry::kCHROME_archive = nsnull;
nsIRDFResource* nsChromeRegistry::kCHROME_name = nsnull;
nsIRDFResource* nsChromeRegistry::kCHROME_displayname = nsnull;
nsIRDFDataSource* nsChromeRegistry::mInner = nsnull;

////////////////////////////////////////////////////////////////////////////////

nsChromeRegistry::nsChromeRegistry()
{
	NS_INIT_REFCNT();
  
}

nsChromeRegistry::~nsChromeRegistry()
{
    
    --gRefCnt;
    if (gRefCnt == 0) {

        // Release our inner data source
        NS_IF_RELEASE(mInner);

        // release all the properties:
        NS_IF_RELEASE(kCHROME_chrome);
        NS_IF_RELEASE(kCHROME_skin);
        NS_IF_RELEASE(kCHROME_content);
        NS_IF_RELEASE(kCHROME_locale);
        NS_IF_RELEASE(kCHROME_base);
        NS_IF_RELEASE(kCHROME_main);
        NS_IF_RELEASE(kCHROME_archive);
        NS_IF_RELEASE(kCHROME_displayname);
        NS_IF_RELEASE(kCHROME_name);
       
        if (gRDFService) {
            nsServiceManager::ReleaseService(kRDFServiceCID, gRDFService);
            gRDFService = nsnull;
        }
    }
}

NS_IMPL_ADDREF(nsChromeRegistry)
NS_IMPL_RELEASE(nsChromeRegistry)

NS_IMETHODIMP
nsChromeRegistry::QueryInterface(REFNSIID aIID, void** aResult)
{
    NS_PRECONDITION(aResult != nsnull, "null ptr");
    if (! aResult)
        return NS_ERROR_NULL_POINTER;

    if (aIID.Equals(nsCOMTypeInfo<nsIChromeRegistry>::GetIID()) ||
        aIID.Equals(kISupportsIID)) {
        *aResult = NS_STATIC_CAST(nsIChromeRegistry*, this);
    }
    else if (aIID.Equals(nsCOMTypeInfo<nsIRDFDataSource>::GetIID())) {
        *aResult = NS_STATIC_CAST(nsIRDFDataSource*, this);
    }
    else if (aIID.Equals(nsCOMTypeInfo<nsIRDFRemoteDataSource>::GetIID())) {
        *aResult = NS_STATIC_CAST(nsIRDFRemoteDataSource*, this);
    }
    else {
        *aResult = nsnull;
        return NS_NOINTERFACE;
    }

    NS_ADDREF(this);
    return NS_OK;
}

////////////////////////////////////////////////////////////////////////////////
// nsIChromeRegistry methods:
/*
    The ConvertChromeURL takes a chrome URL and converts it into a resource: or 
    an HTTP: url type with certain rules. Here are the current portions of a 
    chrome: url that make up the chrome-

            chrome://global/skin/foo?bar
            \------/ \----/\---/ \-----/
                |       |     |     |
                |       |     |     `-- RemainingPortion
                |       |     |
                |       |     `-- Provider 
                |       |
                |       `-- Package
                |
                '-- Always "chrome://"


    Sometime in future when Necko lands completely this will change to the 
    following syntax-

            chrome://skin@global/foo?bar

    This will make the parsing simpler and quicker (since the URL parsing already
    takes this into account)

*/
NS_IMETHODIMP
nsChromeRegistry::ConvertChromeURL(nsIURI* aChromeURL)
{
    nsresult rv = NS_OK;
    NS_ASSERTION(aChromeURL, "null url!");
    if (!aChromeURL)
        return NS_ERROR_NULL_POINTER;

#ifdef NS_DEBUG
    //Ensure that we got a chrome url!
    nsXPIDLCString scheme;
    rv = aChromeURL->GetScheme(getter_Copies(scheme));
    if (NS_FAILED(rv)) return rv;
    NS_ASSERTION((0 == PL_strncmp(scheme, "chrome", 6)), 
        "Bad scheme URL in chrome URL conversion!");
    if (0 != PL_strncmp(scheme, "chrome", 6))
        return NS_ERROR_FAILURE;
#endif

    // Obtain the package, provider and remaining from the URL
    nsXPIDLCString package, provider, remaining;

#if 0 // This change happens when we switch to using chrome://skin@global/foo..
    rv = aChromeURL->GetHost(getter_Copies(package));
    if (NS_FAILED(rv)) return rv;
    rv = aChromeURL->GetPreHost(getter_Copies(provider));
    if (NS_FAILED(rv)) return rv;
    rv = aChromeURL->GetPath(getter_Copies(remaining));
    if (NS_FAILED(rv)) return rv;
#else // For now however...

    rv = aChromeURL->GetHost(getter_Copies(package));
    if (NS_FAILED(rv)) return rv;
    nsXPIDLCString tempPath;
    rv = aChromeURL->GetPath(getter_Copies(tempPath));
    if (NS_FAILED(rv)) return rv;

    BreakProviderAndRemainingFromPath(
        (const char*)tempPath, 
        getter_Copies(provider), 
        getter_Copies(remaining));

#endif

    // Construct the lookup string-
    // which is basically chrome:// + package + provider
    
    nsAutoString lookup("chrome://");

    lookup += package; // no trailing slash here
    
    NS_ASSERTION(*provider == '/', "No leading slash here!");
    
    //definitely have a leading slash...
    if (*provider != '/')
        lookup += '/';
    lookup += provider; 
    
    // end it on a slash if none is present
    if (lookup.CharAt(lookup.Length()-1) != '/')
        lookup += '/';

    // Get the chromeResource from this lookup string
    nsCOMPtr<nsIRDFResource> chromeResource;
    if (NS_FAILED(rv = GetPackageTypeResource(lookup, getter_AddRefs(chromeResource)))) {
//        NS_ERROR("Unable to retrieve the resource corresponding to the chrome skin or content.");
        //return rv;
    }
    
    // Using this chrome resource get the three basic things of a chrome entry-
    // base, name, main. and don't bail if they don't exist.

    nsAutoString base, name, main;

    rv = GetChromeResource(name, chromeResource, kCHROME_name);
    if (NS_FAILED (rv)) {
        if (PL_strcmp(provider,"/locale") == 0)
          name = "en-US";
        else
          name = "default";
    }

    rv = GetChromeResource(base, chromeResource, kCHROME_base);
    if (NS_FAILED(rv))
    {
        // No base entry was found, default it to our cache.
        base = "resource:/chrome/";
        base += package;
        if ((base.CharAt(base.Length()-1) != '/') && *provider != '/')
            base += '/';
        base += provider;
        if (base.CharAt(base.Length()-1) != '/') 
            base += '/';
        if (name.Length())
            base += name;
        if (base.CharAt(base.Length()-1) != '/')
            base += '/';
    }

    NS_ASSERTION(base.CharAt(base.Length()-1) == '/', "Base doesn't end in a slash!");
    if ('/' != base.CharAt(base.Length()-1))
        base += '/';
  
    // Now we construct our finalString
    nsAutoString finalString(base);

    if (!remaining || (0 == PL_strlen(remaining)))
    {
        rv = GetChromeResource(main, chromeResource, kCHROME_main);
        if (NS_FAILED(rv))
        {
            //we'd definitely need main for an empty remaining
            //NS_ERROR("Unable to retrieve the main file registry entry for a chrome URL.");
            //return rv;
            main = package;
            if (PL_strcmp(provider, "/skin") == 0)
              main += ".css";
            else if (PL_strcmp(provider, "/content") == 0)
              main += ".xul";
            else if (PL_strcmp(provider, "/locale") == 0)
              main += ".dtd";
        }
        finalString += main;
    }
    else
        finalString += remaining;

    char* finalURI = finalString.ToNewCString();
    aChromeURL->SetSpec(finalURI);

    nsCRT::free(finalURI);
    return NS_OK;
}


NS_IMETHODIMP
nsChromeRegistry::InitRegistry()
{
    gRefCnt++;
    if (gRefCnt == 1) {
        nsresult rv;
        rv = nsServiceManager::GetService(kRDFServiceCID,
                                          kIRDFServiceIID,
                                          (nsISupports**)&gRDFService);
        NS_ASSERTION(NS_SUCCEEDED(rv), "unable to get RDF service");
        if (NS_FAILED(rv)) return rv;

        // get all the properties we'll need:
        rv = gRDFService->GetResource(kURICHROME_chrome, &kCHROME_chrome);
        NS_ASSERTION(NS_SUCCEEDED(rv), "unable to get resource");
        if (NS_FAILED(rv)) return rv;

        rv = gRDFService->GetResource(kURICHROME_skin, &kCHROME_skin);
        NS_ASSERTION(NS_SUCCEEDED(rv), "unable to get resource");
        if (NS_FAILED(rv)) return rv;

        rv = gRDFService->GetResource(kURICHROME_content, &kCHROME_content);
        NS_ASSERTION(NS_SUCCEEDED(rv), "unable to get resource");
        if (NS_FAILED(rv)) return rv;

        rv = gRDFService->GetResource(kURICHROME_locale, &kCHROME_locale);
        NS_ASSERTION(NS_SUCCEEDED(rv), "unable to get resource");
        if (NS_FAILED(rv)) return rv;

        rv = gRDFService->GetResource(kURICHROME_base, &kCHROME_base);
        NS_ASSERTION(NS_SUCCEEDED(rv), "unable to get resource");
        if (NS_FAILED(rv)) return rv;

        rv = gRDFService->GetResource(kURICHROME_main, &kCHROME_main);
        NS_ASSERTION(NS_SUCCEEDED(rv), "unable to get resource");
        if (NS_FAILED(rv)) return rv;

        rv = gRDFService->GetResource(kURICHROME_archive, &kCHROME_archive);
        NS_ASSERTION(NS_SUCCEEDED(rv), "unable to get resource");
        if (NS_FAILED(rv)) return rv;

        rv = gRDFService->GetResource(kURICHROME_name, &kCHROME_name);
        NS_ASSERTION(NS_SUCCEEDED(rv), "unable to get resource");
        if (NS_FAILED(rv)) return rv;

        rv = gRDFService->GetResource(kURICHROME_displayname, &kCHROME_displayname);
        NS_ASSERTION(NS_SUCCEEDED(rv), "unable to get resource");
        if (NS_FAILED(rv)) return rv;


        /*
        rv = nsComponentManager::CreateInstance(kRDFXMLDataSourceCID,
                                                nsnull,
                                                nsIRDFDataSource::GetIID(),
                                                (void**) &mInner);
        if (NS_FAILED(rv)) return rv;

        nsCOMPtr<nsIRDFRemoteDataSource> remote = do_QueryInterface(mInner);
        if (! remote)
            return NS_ERROR_UNEXPECTED;

        // Retrieve the mInner data source.
        nsSpecialSystemDirectory chromeFile(nsSpecialSystemDirectory::OS_CurrentProcessDirectory);
        chromeFile += "chrome";
        chromeFile += "registry.rdf";

        nsFileURL chromeURL(chromeFile);
        const char* innerURI = chromeURL.GetAsString();

        rv = remote->Init(innerURI);
        if (NS_FAILED(rv)) return rv;
        */
    }
    /*
    nsCOMPtr<nsIRDFRemoteDataSource> remote = do_QueryInterface(mInner);
    if (! remote)
        return NS_ERROR_UNEXPECTED;

    // We need to read this synchronously.
    nsresult rv = remote->Refresh(PR_TRUE);
    if (NS_FAILED(rv)) return rv;
    */

    return NS_OK;
}

////////////////////////////////////////////////////////////////////////////////
// nsIRDFObserver methods:


////////////////////////////////////////////////////////////////////////////////

nsresult
nsChromeRegistry::GetPackageTypeResource(const nsString& aChromeType,
                                           nsIRDFResource** aResult)
{
    nsresult rv = NS_OK;
    char* url = aChromeType.ToNewCString();
    if (NS_FAILED(rv = gRDFService->GetResource(url, aResult))) {
        NS_ERROR("Unable to retrieve a resource for this package type.");
        *aResult = nsnull;
        nsCRT::free(url);
        return rv;
    }
    nsCRT::free(url);
    return NS_OK;
}

nsresult 
nsChromeRegistry::GetChromeResource(nsString& aResult, 
                                    nsIRDFResource* aChromeResource,
                                    nsIRDFResource* aProperty)
{
    nsresult rv = NS_OK;

    if (mInner == nsnull)
        return NS_ERROR_FAILURE; // Must have a DB to attempt this operation.

    nsCOMPtr<nsIRDFNode> chromeBase;
    if (NS_FAILED(rv = GetTarget(aChromeResource, aProperty, PR_TRUE, getter_AddRefs(chromeBase)))) {
        NS_ERROR("Unable to obtain a base resource.");
        return rv;
    }

    if (chromeBase == nsnull)
        return NS_ERROR_FAILURE;

    nsCOMPtr<nsIRDFResource> resource;
    nsCOMPtr<nsIRDFLiteral> literal;

    if (NS_SUCCEEDED(rv = chromeBase->QueryInterface(kIRDFResourceIID,
                                                     (void**) getter_AddRefs(resource)))) {
        nsXPIDLCString uri;
        resource->GetValue( getter_Copies(uri) );
        aResult = uri;
    }
    else if (NS_SUCCEEDED(rv = chromeBase->QueryInterface(kIRDFLiteralIID,
                                                      (void**) getter_AddRefs(literal)))) {
        nsXPIDLString s;
        literal->GetValue( getter_Copies(s) );
        aResult = s;
    }
    else {
        // This should _never_ happen.
        NS_ERROR("uh, this isn't a resource or a literal!");
        return NS_ERROR_UNEXPECTED;
    }

    return NS_OK;
}


nsresult
NS_NewChromeRegistry(nsIChromeRegistry** aResult)
{
    NS_PRECONDITION(aResult != nsnull, "null ptr");
    if (! aResult)
        return NS_ERROR_NULL_POINTER;

    nsChromeRegistry* chromeRegistry = new nsChromeRegistry();
    if (chromeRegistry == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(chromeRegistry);
    *aResult = chromeRegistry;
    return NS_OK;
}

////////////////////////////////////////////////////////////////////////////////

// nsIRDFDataSource methods
NS_IMETHODIMP 
nsChromeRegistry::GetURI(char** uri)
{
    *uri = nsXPIDLCString::Copy("rdf:chrome");
    if (! *uri)
        return NS_ERROR_OUT_OF_MEMORY;

    return NS_OK;
}

NS_IMETHODIMP 
nsChromeRegistry::GetSource(nsIRDFResource* property,
                     nsIRDFNode* target,
                     PRBool tv,
                     nsIRDFResource** source /* out */)
{
  return mInner->GetSource(property, target, tv, source);
}

NS_IMETHODIMP 
nsChromeRegistry::GetSources(nsIRDFResource* property,
                      nsIRDFNode* target,
                      PRBool tv,
                      nsISimpleEnumerator** sources /* out */)
{
  return mInner->GetSources(property, target, tv, sources);
}

NS_IMETHODIMP 
nsChromeRegistry::GetTarget(nsIRDFResource* source,
                     nsIRDFResource* property,
                     PRBool tv,
                     nsIRDFNode** target /* out */)
{

  return mInner->GetTarget(source, property, tv, target);
}

NS_IMETHODIMP 
nsChromeRegistry::GetTargets(nsIRDFResource* source,
                      nsIRDFResource* property,
                      PRBool tv,
                      nsISimpleEnumerator** targets /* out */)
{
  return mInner->GetTargets(source, property, tv, targets);
}

NS_IMETHODIMP 
nsChromeRegistry::Assert(nsIRDFResource* source, 
                  nsIRDFResource* property, 
                  nsIRDFNode* target,
                  PRBool tv)
{
  return mInner->Assert(source, property, target, tv);
}

NS_IMETHODIMP 
nsChromeRegistry::Unassert(nsIRDFResource* source,
                    nsIRDFResource* property,
                    nsIRDFNode* target)
{
  return mInner->Unassert(source, property, target);
}

NS_IMETHODIMP
nsChromeRegistry::Change(nsIRDFResource* aSource,
                         nsIRDFResource* aProperty,
                         nsIRDFNode* aOldTarget,
                         nsIRDFNode* aNewTarget)
{
    return mInner->Change(aSource, aProperty, aOldTarget, aNewTarget);
}

NS_IMETHODIMP
nsChromeRegistry::Move(nsIRDFResource* aOldSource,
                       nsIRDFResource* aNewSource,
                       nsIRDFResource* aProperty,
                       nsIRDFNode* aTarget)
{
    return mInner->Move(aOldSource, aNewSource, aProperty, aTarget);
}

NS_IMETHODIMP 
nsChromeRegistry::HasAssertion(nsIRDFResource* source,
                        nsIRDFResource* property,
                        nsIRDFNode* target,
                        PRBool tv,
                        PRBool* hasAssertion /* out */)
{
  return mInner->HasAssertion(source, property, target, tv, hasAssertion);
}

NS_IMETHODIMP nsChromeRegistry::AddObserver(nsIRDFObserver* n)
{
  return mInner->AddObserver(n);
}

NS_IMETHODIMP nsChromeRegistry::RemoveObserver(nsIRDFObserver* n)
{
  return mInner->RemoveObserver(n);
}

NS_IMETHODIMP nsChromeRegistry::ArcLabelsIn(nsIRDFNode* node,
                       nsISimpleEnumerator** labels /* out */)
{
  return mInner->ArcLabelsIn(node, labels);
}

NS_IMETHODIMP nsChromeRegistry::ArcLabelsOut(nsIRDFResource* source,
                        nsISimpleEnumerator** labels /* out */) 
{
  return mInner->ArcLabelsOut(source, labels);
}

NS_IMETHODIMP nsChromeRegistry::GetAllResources(nsISimpleEnumerator** aCursor)
{
  return mInner->GetAllResources(aCursor);
}

NS_IMETHODIMP 
nsChromeRegistry::GetAllCommands(nsIRDFResource* source,
                          nsIEnumerator/*<nsIRDFResource>*/** commands)
{
  return mInner->GetAllCommands(source, commands);
}

NS_IMETHODIMP 
nsChromeRegistry::GetAllCmds(nsIRDFResource* source,
                          nsISimpleEnumerator/*<nsIRDFResource>*/** commands)
{
  return mInner->GetAllCmds(source, commands);
}

NS_IMETHODIMP 
nsChromeRegistry::IsCommandEnabled(nsISupportsArray/*<nsIRDFResource>*/* aSources,
                            nsIRDFResource*   aCommand,
                            nsISupportsArray/*<nsIRDFResource>*/* aArguments,
                            PRBool* retVal)
{
  return mInner->IsCommandEnabled(aSources, aCommand, aArguments, retVal);
}

NS_IMETHODIMP 
nsChromeRegistry::DoCommand(nsISupportsArray/*<nsIRDFResource>*/* aSources,
                     nsIRDFResource*   aCommand,
                     nsISupportsArray/*<nsIRDFResource>*/* aArguments)
{
  return mInner->DoCommand(aSources, aCommand, aArguments);
}


////////////////////////////////////////////////////////////////////////
// nsIRDFRemoteDataSource methods

NS_IMETHODIMP
nsChromeRegistry::Init(const char* aURI)
{
    return NS_OK;
}


NS_IMETHODIMP
nsChromeRegistry::Refresh(PRBool aBlocking)
{
    nsCOMPtr<nsIRDFRemoteDataSource> remote = do_QueryInterface(mInner);
    if (! remote)
        return NS_ERROR_UNEXPECTED;

    return remote->Refresh(aBlocking);
}


NS_IMETHODIMP
nsChromeRegistry::Flush()
{
    nsCOMPtr<nsIRDFRemoteDataSource> remote = do_QueryInterface(mInner);
    if (! remote)
        return NS_ERROR_UNEXPECTED;

    return remote->Flush();
}

////////////////////////////////////////////////////////////////////////

//
// Path = provider/remaining
// 
void BreakProviderAndRemainingFromPath(const char* i_path, char** o_provider, char** o_remaining)
{
    if (!i_path || !o_provider || !o_remaining)
        return;
    int len = PL_strlen(i_path);
    NS_ASSERTION(len>1, "path is messed up!");
    char* slash = PL_strchr(i_path+1, '/'); // +1 to skip the leading slash if any
    if (slash)
    {
        *o_provider = PL_strndup(i_path, (slash - i_path)); // dont include the trailing slash
        if (slash != (i_path + len-1)) // if that was not the last trailing slash...
        {
            // don't include the leading slash here as well...
            *o_remaining = PL_strndup(slash+1, len - (slash-i_path + 1)); 
        }
        else
            *o_remaining = nsnull;
    }
    else // everything is just the provider
        *o_provider = PL_strndup(i_path, len);
}
