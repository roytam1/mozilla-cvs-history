/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

/*
 * RDF datasource for the account manager
 */

#include "nsMsgAccountManagerDS.h"


#include "rdf.h"
#include "nsRDFCID.h"
#include "nsIRDFDataSource.h"
#include "nsEnumeratorUtils.h"
#include "nsIServiceManager.h"
#include "nsIMsgMailSession.h"

#include "nsXPIDLString.h"

#include "nsMsgRDFUtils.h"
#include "nsIMsgFolder.h"
#include "nsMsgBaseCID.h"

// turn this on to see useful output
#undef DEBUG_amds

static NS_DEFINE_CID(kMsgMailSessionCID, NS_MSGMAILSESSION_CID);
static NS_DEFINE_CID(kRDFServiceCID, NS_RDFSERVICE_CID);

#define NC_RDF_PAGETITLE_MAIN     NC_NAMESPACE_URI "PageTitleMain"
#define NC_RDF_PAGETITLE_SERVER   NC_NAMESPACE_URI "PageTitleServer"
#define NC_RDF_PAGETITLE_COPIES   NC_NAMESPACE_URI "PageTitleCopies"
#define NC_RDF_PAGETITLE_ADVANCED NC_NAMESPACE_URI "PageTitleAdvanced"
#define NC_RDF_PAGETITLE_SMTP     NC_NAMESPACE_URI "PageTitleSMTP"
#define NC_RDF_PAGETAG NC_NAMESPACE_URI "PageTag"

#define NC_RDF_ACCOUNTROOT "msgaccounts:/"

typedef struct _serverCreationParams {
  nsISupportsArray *serverArray;
  nsIRDFService *rdfService;
} serverCreationParams;

// static members
nsIRDFResource* nsMsgAccountManagerDataSource::kNC_Child=nsnull;
nsIRDFResource* nsMsgAccountManagerDataSource::kNC_Name=nsnull;
nsIRDFResource* nsMsgAccountManagerDataSource::kNC_FolderTreeName=nsnull;
nsIRDFResource* nsMsgAccountManagerDataSource::kNC_NameSort=nsnull;
nsIRDFResource* nsMsgAccountManagerDataSource::kNC_FolderTreeNameSort=nsnull;
nsIRDFResource* nsMsgAccountManagerDataSource::kNC_PageTag=nsnull;
nsIRDFResource* nsMsgAccountManagerDataSource::kNC_Settings=nsnull;
nsIRDFResource* nsMsgAccountManagerDataSource::kNC_AccountRoot=nsnull;

// properties corresponding to interfaces
nsIRDFResource* nsMsgAccountManagerDataSource::kNC_Account=nsnull;
nsIRDFResource* nsMsgAccountManagerDataSource::kNC_Server=nsnull;
nsIRDFResource* nsMsgAccountManagerDataSource::kNC_Identity=nsnull;

// individual pages
nsIRDFResource* nsMsgAccountManagerDataSource::kNC_PageTitleMain=nsnull;
nsIRDFResource* nsMsgAccountManagerDataSource::kNC_PageTitleServer=nsnull;
nsIRDFResource* nsMsgAccountManagerDataSource::kNC_PageTitleCopies=nsnull;
nsIRDFResource* nsMsgAccountManagerDataSource::kNC_PageTitleAdvanced=nsnull;
nsIRDFResource* nsMsgAccountManagerDataSource::kNC_PageTitleSMTP=nsnull;

nsrefcnt nsMsgAccountManagerDataSource::gAccountManagerResourceRefCnt = 0;

// RDF to match
#define NC_RDF_ACCOUNT NC_NAMESPACE_URI "Account"
#define NC_RDF_SERVER  NC_NAMESPACE_URI "Server"
#define NC_RDF_IDENTITY NC_NAMESPACE_URI "Identity"
#define NC_RDF_SETTINGS NC_NAMESPACE_URI "Settings"


nsMsgAccountManagerDataSource::nsMsgAccountManagerDataSource():
  mAccountManager(null_nsCOMPtr())
{

#ifdef DEBUG_amds
  printf("nsMsgAccountManagerDataSource() being created\n");
#endif

  // XXX This call should be moved to a NS_NewMsgFooDataSource()
  // method that the factory calls, so that failure to construct
  // will return an error code instead of returning a partially
  // initialized object.
  nsresult rv = Init();
  NS_ASSERTION(NS_SUCCEEDED(rv), "uh oh, initialization failed");
  if (NS_FAILED(rv)) return /* rv */;

  return /* NS_OK */;
}

nsMsgAccountManagerDataSource::~nsMsgAccountManagerDataSource()
{
	if (--gAccountManagerResourceRefCnt == 0)
	{
      NS_IF_RELEASE(kNC_Child);
      NS_IF_RELEASE(kNC_Name);
      NS_IF_RELEASE(kNC_FolderTreeName);
      NS_IF_RELEASE(kNC_NameSort);
      NS_IF_RELEASE(kNC_FolderTreeNameSort);
      NS_IF_RELEASE(kNC_PageTag);
      NS_IF_RELEASE(kNC_Account);
      NS_IF_RELEASE(kNC_Server);
      NS_IF_RELEASE(kNC_Identity);
      NS_IF_RELEASE(kNC_PageTitleMain);
      NS_IF_RELEASE(kNC_PageTitleServer);
      NS_IF_RELEASE(kNC_PageTitleCopies);
      NS_IF_RELEASE(kNC_PageTitleAdvanced);
      NS_IF_RELEASE(kNC_PageTitleSMTP);
      
      NS_IF_RELEASE(kNC_AccountRoot);
      
      // eventually these need to exist in some kind of array
      // that's easily extensible
      NS_IF_RELEASE(kNC_Settings);
	}

}

nsresult
nsMsgAccountManagerDataSource::Init()
{
    nsresult rv=NS_OK;
    
    if (!mAccountManager) {
        mAccountManager = do_GetService(NS_MSGACCOUNTMANAGER_PROGID, &rv);
        if (NS_FAILED(rv)) return rv;
    }
    
	if (gAccountManagerResourceRefCnt++ == 0) {
      getRDFService()->GetResource(NC_RDF_CHILD, &kNC_Child);
      getRDFService()->GetResource(NC_RDF_NAME, &kNC_Name);
      getRDFService()->GetResource(NC_RDF_FOLDERTREENAME, &kNC_FolderTreeName);
      getRDFService()->GetResource(NC_RDF_NAME_SORT, &kNC_NameSort);
      getRDFService()->GetResource(NC_RDF_FOLDERTREENAME_SORT, &kNC_FolderTreeNameSort);
      getRDFService()->GetResource(NC_RDF_PAGETAG, &kNC_PageTag);
      getRDFService()->GetResource(NC_RDF_ACCOUNT, &kNC_Account);
      getRDFService()->GetResource(NC_RDF_SERVER, &kNC_Server);
      getRDFService()->GetResource(NC_RDF_IDENTITY, &kNC_Identity);
      getRDFService()->GetResource(NC_RDF_PAGETITLE_MAIN, &kNC_PageTitleMain);
      getRDFService()->GetResource(NC_RDF_PAGETITLE_SERVER, &kNC_PageTitleServer);
      getRDFService()->GetResource(NC_RDF_PAGETITLE_COPIES, &kNC_PageTitleCopies);
      getRDFService()->GetResource(NC_RDF_PAGETITLE_ADVANCED, &kNC_PageTitleAdvanced);
      getRDFService()->GetResource(NC_RDF_PAGETITLE_SMTP, &kNC_PageTitleSMTP);
      
      getRDFService()->GetResource(NC_RDF_ACCOUNTROOT, &kNC_AccountRoot);
      
      // eventually these need to exist in some kind of array
      // that's easily extensible
      getRDFService()->GetResource(NC_RDF_SETTINGS, &kNC_Settings);
    }
    return NS_OK;
}

void nsMsgAccountManagerDataSource::Close()
{
//	mAccountManager = null_nsCOMPtr();
	nsMsgRDFDataSource::Close();
}

/* nsIRDFNode GetTarget (in nsIRDFResource aSource, in nsIRDFResource property, in boolean aTruthValue); */
NS_IMETHODIMP
nsMsgAccountManagerDataSource::GetTarget(nsIRDFResource *source,
                                         nsIRDFResource *property,
                                         PRBool aTruthValue,
                                         nsIRDFNode **target)
{
  nsresult rv;
  
  
  rv = NS_RDF_NO_VALUE;

  nsAutoString str("");
  if (property == kNC_Name || property == kNC_FolderTreeName) {

    // XXX these should be localized
    if (source == kNC_PageTitleMain)
      str = "Main";
    
    else if (source == kNC_PageTitleServer)
      str = "Server";

    else if (source == kNC_PageTitleCopies)
      str = "Copies and Folders";

    else if (source == kNC_PageTitleAdvanced)
      str = "Advanced";

    else if (source == kNC_PageTitleSMTP)
      str = "Outgoing (SMTP) Server";
    
    else {
      nsCOMPtr<nsIMsgFolder> folder = do_QueryInterface(source, &rv);
      if (NS_SUCCEEDED(rv)) {
        PRBool isServer;
		rv = folder->GetIsServer(&isServer);
		if(NS_SUCCEEDED(rv) && isServer)
		{
			nsXPIDLString prettyName;
			rv = folder->GetPrettyName(getter_Copies(prettyName));
			if (NS_SUCCEEDED(rv))
			  str = prettyName;
		}
      }
    }
  }
  else if (property == kNC_PageTag) {
    // do NOT localize these strings. these are the urls of the XUL files
    if (source == kNC_PageTitleServer)
      str = "am-server.xul";
    else if (source == kNC_PageTitleCopies)
      str = "am-copies.xul";
    else if (source == kNC_PageTitleAdvanced)
      str = "am-advanced.xul";
    else if (source == kNC_PageTitleSMTP) 
      str = "am-smtp.xul";
    else
      str = "am-main.xul";
  }

  // handle sorting of servers
  else if ((property == kNC_NameSort) ||
           (property == kNC_FolderTreeNameSort)) {
    // make sure we're handling a root folder that is a server
    nsCOMPtr<nsIMsgFolder> folder = do_QueryInterface(source,&rv);
    if (NS_FAILED(rv))
      return NS_RDF_NO_VALUE;
    
    PRBool isServer=PR_FALSE;
    folder->GetIsServer(&isServer);
    if (!isServer)
      return NS_RDF_NO_VALUE;

    nsCOMPtr<nsIMsgIncomingServer> server;
    rv = folder->GetServer(getter_AddRefs(server));
    if (NS_FAILED(rv) || !server) return NS_ERROR_FAILURE;

    PRInt32 accountNum;
    rv = mAccountManager->FindServerIndex(server, &accountNum);
    if (NS_FAILED(rv)) return rv;
    
    accountNum += 1000;
    str.Append(accountNum);
  }

  // GetTargets() stuff - need to return a valid answer so that
  // twisties will appear
  else if (property == kNC_Settings) {
    nsCOMPtr<nsIMsgFolder> folder = do_QueryInterface(source,&rv);
    if (NS_FAILED(rv))
      return NS_RDF_NO_VALUE;
    
    PRBool isServer=PR_FALSE;
    folder->GetIsServer(&isServer);
    // no need to localize this!
    if (isServer)
      str = "ServerSettings";
  }
  
  if (str!="")
    rv = createNode(str, target, getRDFService());
  //if we have an empty string and we don't have an error value, then 
  //we don't have a value for RDF.
  else if(NS_SUCCEEDED(rv))
	rv = NS_RDF_NO_VALUE;

  return rv;
}



/* nsISimpleEnumerator GetTargets (in nsIRDFResource aSource, in nsIRDFResource property, in boolean aTruthValue); */
NS_IMETHODIMP
nsMsgAccountManagerDataSource::GetTargets(nsIRDFResource *source,
                                   nsIRDFResource *property,
                                   PRBool aTruthValue,
                                   nsISimpleEnumerator **_retval)
{
  nsresult rv = NS_RDF_NO_VALUE;

  // create array and enumerator
  // even if we're not handling this we need to return something empty?
  nsCOMPtr<nsISupportsArray> nodes;
  rv = NS_NewISupportsArray(getter_AddRefs(nodes));
  if (NS_FAILED(rv)) return rv;
  
  nsISimpleEnumerator* enumerator =
    new nsArrayEnumerator(nodes);
  if (!enumerator) return NS_ERROR_OUT_OF_MEMORY;

  *_retval = enumerator;
  NS_ADDREF(*_retval);

  // if the property is "account" or "child" then return the
  // list of accounts
  // if the property is "server" return a union of all servers
  // in the account (mainly for the folder pane)

#ifdef DEBUG_amds
  nsXPIDLCString source_value;
  rv = source->GetValue(getter_Copies(source_value));

  nsXPIDLCString property_arc;
  rv = property->GetValue(getter_Copies(property_arc));
  if (NS_FAILED(rv)) return rv;
  
  printf("GetTargets(%s with arc %s...)\n",
         (const char*)source_value,
         (const char*)property_arc);
#endif
  
  if (source == kNC_AccountRoot) {

    if (property == kNC_Child ||
        property == kNC_Settings) {
      
      nsCOMPtr<nsISupportsArray> servers;
      rv = mAccountManager->GetAllServers(getter_AddRefs(servers));
      
      // fill up the nodes array with the RDF Resources for the servers
      serverCreationParams params = { nodes, getRDFService() };
      servers->EnumerateForwards(createServerResources, (void*)&params);
#ifdef DEBUG_amds
        PRUint32 nodecount;
        nodes->Count(&nodecount);
      printf("GetTargets(): added %d servers on %s\n", nodecount,
             (const char*)property_arc);
#endif

      // for the "settings" arc, we also want to do an SMTP tag
      if (property == kNC_Settings) {
        nodes->AppendElement(kNC_PageTitleSMTP);
      }
    }
#ifdef DEBUG_amds
    else {
      printf("unknown arc %s on msgaccounts:/\n", (const char*)property_arc);
    }
#endif
    
  } else {
    /* if this is a server, then support the settings */
    nsCOMPtr<nsIMsgFolder> folder = do_QueryInterface(source, &rv);

    if (NS_SUCCEEDED(rv)) {
      // all the Settings - main, server, copies, advanced, etc
      if (property == kNC_Settings) {
        
        if (NS_SUCCEEDED(rv)) nodes->AppendElement(kNC_PageTitleServer);
        if (NS_SUCCEEDED(rv)) nodes->AppendElement(kNC_PageTitleCopies);
        
      }
    } else if (source == kNC_PageTitleSMTP) {
      if (property == kNC_Settings) {
        //        nodes->AppendElement(kNC_PageTitleAdvanced);
      }
    }
    
        
#ifdef DEBUG_amds
    else {
      printf("GetTargets(): Unknown source %s\n", (const char*)source_value);
    }
#endif
  }
  
  return NS_OK;
}

// enumeration function to convert each server (element)
// to an nsIRDFResource and append it to the array (in data)
// always return PR_TRUE to try on every element instead of aborting early
PRBool
nsMsgAccountManagerDataSource::createServerResources(nsISupports *element,
                                                     void *data)
{
  nsresult rv;
  // get parameters out of the data argument
  serverCreationParams *params = (serverCreationParams*)data;
  nsCOMPtr<nsISupportsArray> servers = dont_QueryInterface(params->serverArray);
  nsCOMPtr<nsIRDFService> rdf = dont_QueryInterface(params->rdfService);

  // the server itself is in the element argument
  nsCOMPtr<nsIMsgIncomingServer> server = do_QueryInterface(element, &rv);
  if (NS_FAILED(rv)) return PR_TRUE;

	nsCOMPtr <nsIFolder> serverFolder;
	rv = server->GetRootFolder(getter_AddRefs(serverFolder));
	if(NS_FAILED(rv)) return PR_TRUE;

  if (serverFolder) {
    nsXPIDLString serverName;
    server->GetPrettyName(getter_Copies(serverName));
    serverFolder->SetPrettyName(NS_CONST_CAST(PRUnichar*,
                                              (const PRUnichar*)serverName));
  }

  // add the resource to the array
  nsCOMPtr<nsIRDFResource> serverResource = do_QueryInterface(serverFolder);
	if(!serverResource)
		return PR_TRUE;

  rv = servers->AppendElement(serverResource);
  if (NS_FAILED(rv)) return PR_TRUE;
  
  return PR_TRUE;
}

/* nsISimpleEnumerator ArcLabelsOut (in nsIRDFResource aSource); */
NS_IMETHODIMP
nsMsgAccountManagerDataSource::ArcLabelsOut(nsIRDFResource *source,
                                            nsISimpleEnumerator **_retval)
{
  nsresult rv;
  
  // we have to return something, so always create the array/enumerators
  nsCOMPtr<nsISupportsArray> arcs;
  rv = NS_NewISupportsArray(getter_AddRefs(arcs));

  if (NS_FAILED(rv)) return rv;
  
  nsArrayEnumerator* enumerator =
    new nsArrayEnumerator(arcs);

  if (!enumerator) return NS_ERROR_OUT_OF_MEMORY;
  
  *_retval = enumerator;
  NS_ADDREF(*_retval);
  
  if (source == kNC_AccountRoot) {
    arcs->AppendElement(kNC_Server);
	arcs->AppendElement(kNC_Child);
  }

  arcs->AppendElement(kNC_Settings);
  arcs->AppendElement(kNC_Name);
  arcs->AppendElement(kNC_FolderTreeName);
  arcs->AppendElement(kNC_NameSort);
  arcs->AppendElement(kNC_FolderTreeNameSort);
  arcs->AppendElement(kNC_PageTag);

#ifdef DEBUG_amds_
  printf("GetArcLabelsOut(%s): Adding child, settings, and name arclabels\n", value);
#endif
  
  return NS_OK;
}


