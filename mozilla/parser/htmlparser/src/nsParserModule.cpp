/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 *   Pierre Phaneuf <pp@ludusdesign.com>
 */

#include "nsString.h"
#include "nspr.h"
#include "nsCOMPtr.h"
#include "nsIGenericFactory.h"
#include "nsIModule.h"
#include "nsParserCIID.h"
#include "nsParser.h"
#include "nsLoggingSink.h"
#include "nsWellFormedDTD.h"
#include "CNavDTD.h"
#include "COtherDTD.h"
#include "COtherDTD.h"
#include "nsViewSourceHTML.h"
#include "nsHTMLContentSinkStream.h"
#include "nsHTMLEntities.h"
#include "nsHTMLTokenizer.h"
#include "nsXMLTokenizer.h"
//#include "nsTextTokenizer.h"
#include "nsExpatTokenizer.h"
#include "nsIParserService.h"
#include "nsElementTable.h"

static NS_DEFINE_IID(kIParserServiceIID, NS_IPARSERSERVICE_IID);

class nsParserService : public nsIParserService {
public:
  nsParserService();
  virtual ~nsParserService();

  NS_DECL_ISUPPORTS

  NS_IMETHOD HTMLStringTagToId(const nsString &aTag, PRInt32* aId) const;

  NS_IMETHOD HTMLIdToStringTag(PRInt32 aId, nsString& aTag) const;
  
  NS_IMETHOD HTMLConvertEntityToUnicode(const nsString& aEntity, 
                                        PRInt32* aUnicode) const;
  NS_IMETHOD HTMLConvertUnicodeToEntity(PRInt32 aUnicode,
                                        nsCString& aEntity) const;
  NS_IMETHOD IsContainer(PRInt32 aId, PRBool& aIsContainer) const;
  NS_IMETHOD IsBlock(PRInt32 aId, PRBool& aIsBlock) const;
};

nsParserService::nsParserService()
{
  NS_INIT_ISUPPORTS();
}

nsParserService::~nsParserService()
{
}

NS_IMPL_ISUPPORTS(nsParserService, kIParserServiceIID)
  
NS_IMETHODIMP 
nsParserService::HTMLStringTagToId(const nsString &aTag, PRInt32* aId) const
{
  *aId = nsHTMLTags::LookupTag(aTag);
  return NS_OK;
}

NS_IMETHODIMP 
nsParserService::HTMLIdToStringTag(PRInt32 aId, nsString& aTag) const
{
  aTag.AssignWithConversion( nsHTMLTags::GetStringValue((nsHTMLTag)aId) );
  return NS_OK;
}
  
NS_IMETHODIMP 
nsParserService::HTMLConvertEntityToUnicode(const nsString& aEntity, 
                                            PRInt32* aUnicode) const
{
  *aUnicode = nsHTMLEntities::EntityToUnicode(aEntity);
  return NS_OK;
}

NS_IMETHODIMP 
nsParserService::HTMLConvertUnicodeToEntity(PRInt32 aUnicode,
                                            nsCString& aEntity) const
{
  const nsCString& str = nsHTMLEntities::UnicodeToEntity(aUnicode);
  if (str.Length() > 0) {
    aEntity.Assign(str);
  }
  return NS_OK;
}

NS_IMETHODIMP 
nsParserService::IsContainer(PRInt32 aId, PRBool& aIsContainer) const
{
  aIsContainer = nsHTMLElement::IsContainer((eHTMLTags)aId);
  return NS_OK;
}

NS_IMETHODIMP 
nsParserService::IsBlock(PRInt32 aId, PRBool& aIsBlock) const
{
  if((aId>eHTMLTag_unknown) && (aId<eHTMLTag_userdefined)) {
    aIsBlock=((gHTMLElements[aId].IsMemberOf(kBlock))       || 
              (gHTMLElements[aId].IsMemberOf(kBlockEntity)) || 
              (gHTMLElements[aId].IsMemberOf(kHeading))     || 
              (gHTMLElements[aId].IsMemberOf(kPreformatted))|| 
              (gHTMLElements[aId].IsMemberOf(kList))); 
  }
  else {
    aIsBlock = PR_FALSE;
  }

  return NS_OK;
}

//----------------------------------------------------------------------

static NS_DEFINE_CID(kParserCID, NS_PARSER_CID);
static NS_DEFINE_CID(kLoggingSinkCID, NS_LOGGING_SINK_CID);
static NS_DEFINE_CID(kWellFormedDTDCID, NS_WELLFORMEDDTD_CID);
static NS_DEFINE_CID(kNavDTDCID, NS_CNAVDTD_CID);
static NS_DEFINE_CID(kCOtherDTDCID, NS_COTHER_DTD_CID);
static NS_DEFINE_CID(kCTransitionalDTDCID, NS_CTRANSITIONAL_DTD_CID);
static NS_DEFINE_CID(kViewSourceDTDCID, NS_VIEWSOURCE_DTD_CID);
static NS_DEFINE_CID(kHTMLContentSinkStreamCID, NS_HTMLCONTENTSINKSTREAM_CID);
static NS_DEFINE_CID(kParserServiceCID, NS_PARSERSERVICE_CID);

struct Components {
  const char* mDescription;
  const nsID* mCID;
};

static Components gComponents[] = {
  { "Parser", &kParserCID },
  { "Logging sink", &kLoggingSinkCID },
  { "Well formed DTD", &kWellFormedDTDCID },
  { "Navigator HTML DTD", &kNavDTDCID },
  { "OTHER DTD", &kCOtherDTDCID },
  { "Transitional DTD", &kCTransitionalDTDCID },
  { "ViewSource DTD", &kViewSourceDTDCID },
  { "HTML Content Sink Stream", &kHTMLContentSinkStreamCID },
  { "ParserService", &kParserServiceCID },
};

#define NUM_COMPONENTS (sizeof(gComponents) / sizeof(gComponents[0]));

NS_GENERIC_FACTORY_CONSTRUCTOR(nsParser)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsLoggingSink)
NS_GENERIC_FACTORY_CONSTRUCTOR(CWellFormedDTD)
NS_GENERIC_FACTORY_CONSTRUCTOR(CNavDTD)
NS_GENERIC_FACTORY_CONSTRUCTOR(COtherDTD)
NS_GENERIC_FACTORY_CONSTRUCTOR(CTransitionalDTD)
NS_GENERIC_FACTORY_CONSTRUCTOR(CViewSourceHTML)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsHTMLContentSinkStream)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsParserService)

//----------------------------------------------------------------------

class nsParserModule : public nsIModule {
public:
  nsParserModule();
  virtual ~nsParserModule();

  NS_DECL_ISUPPORTS

  NS_DECL_NSIMODULE

  nsresult Initialize();

protected:
  void Shutdown();

  PRBool mInitialized;
  nsCOMPtr<nsIGenericFactory> mParserFactory;
  nsCOMPtr<nsIGenericFactory> mParserNodeFactory;
  nsCOMPtr<nsIGenericFactory> mLoggingSinkFactory;
  nsCOMPtr<nsIGenericFactory> mWellFormedDTDFactory;
  nsCOMPtr<nsIGenericFactory> mNavHTMLDTDFactory;
  nsCOMPtr<nsIGenericFactory> mOtherHTMLDTDFactory;
  nsCOMPtr<nsIGenericFactory> mTransitionalHTMLDTDFactory;
  nsCOMPtr<nsIGenericFactory> mViewSourceHTMLDTDFactory;
  nsCOMPtr<nsIGenericFactory> mHTMLContentSinkStreamFactory;
  nsCOMPtr<nsIGenericFactory> mParserServiceFactory;
};

static NS_DEFINE_IID(kIModuleIID, NS_IMODULE_IID);

nsParserModule::nsParserModule()
  : mInitialized(PR_FALSE)
{
  NS_INIT_ISUPPORTS();
}

nsParserModule::~nsParserModule()
{
  Shutdown();
}

NS_IMPL_ISUPPORTS(nsParserModule, kIModuleIID)

nsresult
nsParserModule::Initialize()
{
  if (!mInitialized) {
    nsHTMLTags::AddRefTable();
    nsHTMLEntities::AddRefTable();
    mInitialized = PR_TRUE;
    InitializeElementTable();
    CNewlineToken::AllocNewline();
  }
  return NS_OK;
}

void
nsParserModule::Shutdown()
{
  if (mInitialized) {
    nsHTMLTags::ReleaseTable();
    nsHTMLEntities::ReleaseTable();
    nsDTDContext::ReleaseGlobalObjects();
    nsParser::FreeSharedObjects();
    mInitialized = PR_FALSE;
    DeleteElementTable();
    CNewlineToken::FreeNewline();
  }
}

NS_IMETHODIMP
nsParserModule::GetClassObject(nsIComponentManager *aCompMgr,
                               const nsCID& aClass,
                               const nsIID& aIID,
                               void** r_classObj)
{
  nsresult rv=NS_OK;

  if (!mInitialized) {
    rv = Initialize();
    if (NS_FAILED(rv)) {
      return rv;
    }
    mInitialized = PR_TRUE;
  }

  nsCOMPtr<nsIGenericFactory> fact;

  if (aClass.Equals(kParserCID)) {
    if (!mParserFactory) {
      rv = NS_NewGenericFactory(getter_AddRefs(mParserFactory), 
                                &nsParserConstructor);
    }
    fact = mParserFactory;
  }
  else if (aClass.Equals(kLoggingSinkCID)) {
    if (!mLoggingSinkFactory) {
      rv = NS_NewGenericFactory(getter_AddRefs(mLoggingSinkFactory), 
                                &nsLoggingSinkConstructor);
    }
    fact = mLoggingSinkFactory;
  }
  else if (aClass.Equals(kWellFormedDTDCID)) {
    if (!mWellFormedDTDFactory) {
      rv = NS_NewGenericFactory(getter_AddRefs(mWellFormedDTDFactory), 
                                &CWellFormedDTDConstructor);
    }
    fact = mWellFormedDTDFactory;
  }
  else if (aClass.Equals(kNavDTDCID)) {
    if (!mNavHTMLDTDFactory) {
      rv = NS_NewGenericFactory(getter_AddRefs(mNavHTMLDTDFactory), 
                                &CNavDTDConstructor);
    }
    fact = mNavHTMLDTDFactory;
  }
  else if (aClass.Equals(kCOtherDTDCID)) {
    if (!mOtherHTMLDTDFactory) {
      rv = NS_NewGenericFactory(getter_AddRefs(mOtherHTMLDTDFactory), 
                                &COtherDTDConstructor);
    }
    fact = mOtherHTMLDTDFactory;
  }
  else if (aClass.Equals(kCTransitionalDTDCID)) {
    if (!mTransitionalHTMLDTDFactory) {
      rv = NS_NewGenericFactory(getter_AddRefs(mTransitionalHTMLDTDFactory), 
                                &CTransitionalDTDConstructor);
    }
    fact = mTransitionalHTMLDTDFactory;
  }
  else if (aClass.Equals(kViewSourceDTDCID)) {
    if (!mViewSourceHTMLDTDFactory) {
      rv = NS_NewGenericFactory(getter_AddRefs(mViewSourceHTMLDTDFactory), 
                                &CViewSourceHTMLConstructor);
    }
    fact = mViewSourceHTMLDTDFactory;
  }
  else if (aClass.Equals(kHTMLContentSinkStreamCID)) {
    if (!mHTMLContentSinkStreamFactory) {
      rv = NS_NewGenericFactory(getter_AddRefs(mHTMLContentSinkStreamFactory), 
                                &nsHTMLContentSinkStreamConstructor);
    }
    fact = mHTMLContentSinkStreamFactory;
  }
  else if (aClass.Equals(kParserServiceCID)) {
    if (!mParserServiceFactory) {
      rv = NS_NewGenericFactory(getter_AddRefs(mParserServiceFactory), 
                                &nsParserServiceConstructor);
    }
    fact = mParserServiceFactory;
  }
  else {
		rv = NS_ERROR_FACTORY_NOT_REGISTERED;
  }

  if (fact) {
    rv = fact->QueryInterface(aIID, r_classObj);
  }

  return rv;
}

NS_IMETHODIMP
nsParserModule::RegisterSelf(nsIComponentManager *aCompMgr,
                             nsIFile* aPath,
                             const char* registryLocation,
                             const char* componentType)
{
  nsresult rv = NS_OK;
  Components* cp = gComponents;
  Components* end = cp + NUM_COMPONENTS;
  while (cp < end) {
    rv = aCompMgr->RegisterComponentSpec(*cp->mCID, cp->mDescription,
                                         nsnull, aPath, PR_TRUE, PR_TRUE);
    if (NS_FAILED(rv)) {
#ifdef DEBUG
      printf("nsParserModule: unable to register %s component => %x\n",
             cp->mDescription, rv);
#endif
      break;
    }
    cp++;
  }
  return rv;
}

NS_IMETHODIMP
nsParserModule::UnregisterSelf(nsIComponentManager *aCompMgr,
                               nsIFile* aPath,
                               const char* registryLocation)
{
  Components* cp = gComponents;
  Components* end = cp + NUM_COMPONENTS;
  while (cp < end) {
    nsresult rv = aCompMgr->UnregisterComponentSpec(*cp->mCID, aPath);
    if (NS_FAILED(rv)) {
#ifdef DEBUG
      printf("nsParserModule: unable to unregister %s component => %x\n",
             cp->mDescription, rv);
#endif
    }
    cp++;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsParserModule::CanUnload(nsIComponentManager *aCompMgr, PRBool *okToUnload)
{
  if (!okToUnload) {
    return NS_ERROR_INVALID_POINTER;
  }
  *okToUnload = PR_FALSE;
  return NS_ERROR_FAILURE;
}

//----------------------------------------------------------------------

static nsParserModule *gModule = NULL;

extern "C" NS_EXPORT nsresult NSGetModule(nsIComponentManager *servMgr,
                                          nsIFile* location,
                                          nsIModule** return_cobj)
{
  nsresult rv = NS_OK;

  NS_ASSERTION(return_cobj, "Null argument");
  NS_ASSERTION(gModule == NULL, "nsParserModule: Module already created.");

  // Create an initialize the layout module instance
  nsParserModule *m = new nsParserModule();
  if (!m) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  // Increase refcnt and store away nsIModule interface to m in return_cobj
  rv = m->QueryInterface(NS_GET_IID(nsIModule), (void**)return_cobj);
  if (NS_FAILED(rv)) {
    delete m;
    m = nsnull;
  }
  gModule = m;                  // WARNING: Weak Reference
  return rv;
}
