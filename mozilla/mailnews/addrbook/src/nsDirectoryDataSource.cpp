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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1999 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 *   Pierre Phaneuf <pp@ludusdesign.com>
 */

#include "nsDirectoryDataSource.h"
#include "nsAbBaseCID.h"
#include "nsAbDirectory.h"
#include "nsIAddrBookSession.h"
#include "nsIAbCard.h"

#include "rdf.h"
#include "nsIRDFService.h"
#include "nsRDFCID.h"
#include "nsIRDFNode.h"
#include "nsEnumeratorUtils.h"
#include "nsIServiceManager.h"

#include "nsString.h"
#include "nsCOMPtr.h"
#include "nsXPIDLString.h"

#include "prprf.h"	 
#include "prlog.h"	 

// this is used for notification of observers using nsVoidArray
typedef struct _nsAbRDFNotification {
  nsIRDFResource *subject;
  nsIRDFResource *property;
  nsIRDFNode *object;
} nsAbRDFNotification;
                                                
static NS_DEFINE_CID(kRDFServiceCID,  NS_RDFSERVICE_CID);

static NS_DEFINE_CID(kAbDirectoryDataSourceCID, NS_ABDIRECTORYDATASOURCE_CID);
static NS_DEFINE_CID(kAbDirectoryCID, NS_ABDIRECTORY_CID); 
static NS_DEFINE_CID(kAddrBookSessionCID, NS_ADDRBOOKSESSION_CID);

nsIRDFResource* nsAbDirectoryDataSource::kNC_Child = nsnull;
nsIRDFResource* nsAbDirectoryDataSource::kNC_DirName = nsnull;
nsIRDFResource* nsAbDirectoryDataSource::kNC_CardChild = nsnull;
nsIRDFResource* nsAbDirectoryDataSource::kNC_DirUri = nsnull;

// commands
nsIRDFResource* nsAbDirectoryDataSource::kNC_Delete = nsnull;
nsIRDFResource* nsAbDirectoryDataSource::kNC_DeleteCards = nsnull;
nsIRDFResource* nsAbDirectoryDataSource::kNC_NewDirectory = nsnull;

#define NC_RDF_CHILD				"http://home.netscape.com/NC-rdf#child"
#define NC_RDF_DIRNAME			    "http://home.netscape.com/NC-rdf#DirName"
#define NC_RDF_CARDCHILD			"http://home.netscape.com/NC-rdf#CardChild"
#define NC_RDF_DIRURI				"http://home.netscape.com/NC-rdf#DirUri"

//Directory Commands
#define NC_RDF_DELETE				"http://home.netscape.com/NC-rdf#Delete"
#define NC_RDF_DELETECARDS			"http://home.netscape.com/NC-rdf#DeleteCards"
#define NC_RDF_NEWDIRECTORY			"http://home.netscape.com/NC-rdf#NewDirectory"

////////////////////////////////////////////////////////////////////////

nsAbDirectoryDataSource::nsAbDirectoryDataSource():
  mInitialized(PR_FALSE),
  mRDFService(nsnull)
{
}

nsAbDirectoryDataSource::~nsAbDirectoryDataSource (void)
{

	if (mRDFService)
	{
		mRDFService->UnregisterDataSource(this);
		nsServiceManager::ReleaseService(kRDFServiceCID, mRDFService); 
		mRDFService = nsnull;
	}
	
	nsresult rv = NS_OK;
	NS_WITH_SERVICE(nsIAddrBookSession, abSession, kAddrBookSessionCID, &rv); 
	if(NS_SUCCEEDED(rv))
		abSession->RemoveAddressBookListener(this);

	nsrefcnt refcnt;
	NS_RELEASE2(kNC_Child, refcnt);
	NS_RELEASE2(kNC_DirName, refcnt);
	NS_RELEASE2(kNC_CardChild, refcnt);
	NS_RELEASE2(kNC_DirUri, refcnt);

	NS_RELEASE2(kNC_Delete, refcnt);
	NS_RELEASE2(kNC_DeleteCards, refcnt);
	NS_RELEASE2(kNC_NewDirectory, refcnt);

	/* free all directories */
	DIR_ShutDown();
}

nsresult
nsAbDirectoryDataSource::Init()
{
	if (mInitialized)
		return NS_ERROR_ALREADY_INITIALIZED;

	nsresult rv = nsServiceManager::GetService(kRDFServiceCID,
											 NS_GET_IID(nsIRDFService),
											 (nsISupports**) &mRDFService); 
	if (NS_FAILED(rv)) return rv;

	NS_WITH_SERVICE(nsIAddrBookSession, abSession, kAddrBookSessionCID, &rv); 
	if (NS_SUCCEEDED(rv))
		abSession->AddAddressBookListener(this);

	mRDFService->RegisterDataSource(this, PR_FALSE);

	if (!kNC_Child)
	{
		mRDFService->GetResource(NC_RDF_CHILD, &kNC_Child);
		mRDFService->GetResource(NC_RDF_DIRNAME, &kNC_DirName);
		mRDFService->GetResource(NC_RDF_CARDCHILD, &kNC_CardChild);
		mRDFService->GetResource(NC_RDF_DIRURI, &kNC_DirUri);

		mRDFService->GetResource(NC_RDF_DELETE, &kNC_Delete);
		mRDFService->GetResource(NC_RDF_DELETECARDS, &kNC_DeleteCards);
		mRDFService->GetResource(NC_RDF_NEWDIRECTORY, &kNC_NewDirectory);
	}

	DIR_GetDirServers();

	mInitialized = PR_TRUE;
	return NS_OK;
}

NS_IMPL_ADDREF_INHERITED(nsAbDirectoryDataSource, nsAbRDFDataSource)
NS_IMPL_RELEASE_INHERITED(nsAbDirectoryDataSource, nsAbRDFDataSource)

NS_IMETHODIMP nsAbDirectoryDataSource::QueryInterface(REFNSIID iid, void** result)
{
  if (! result)
    return NS_ERROR_NULL_POINTER;

	*result = nsnull;
	if(iid.Equals(NS_GET_IID(nsIAbListener)))
	{
		*result = NS_STATIC_CAST(nsIAbListener*, this);
		NS_ADDREF(this);
		return NS_OK;
	}
	else
		return nsAbRDFDataSource::QueryInterface(iid, result);
}

 // nsIRDFDataSource methods
NS_IMETHODIMP nsAbDirectoryDataSource::GetURI(char* *uri)
{
  if ((*uri = nsXPIDLCString::Copy("rdf:addressdirectory")) == nsnull)
    return NS_ERROR_OUT_OF_MEMORY;
  else
    return NS_OK;
}

NS_IMETHODIMP nsAbDirectoryDataSource::GetTarget(nsIRDFResource* source,
                                               nsIRDFResource* property,
                                               PRBool tv,
                                               nsIRDFNode** target)
{
  nsresult rv = NS_RDF_NO_VALUE;
  // we only have positive assertions in the mail data source.
  if (! tv)
    return NS_RDF_NO_VALUE;

  nsCOMPtr<nsIAbDirectory> directory(do_QueryInterface(source, &rv));
  if (NS_SUCCEEDED(rv) && directory) {
    rv = createDirectoryNode(directory, property, target);
  }
  else
	  return NS_RDF_NO_VALUE;
  return rv;
}


NS_IMETHODIMP nsAbDirectoryDataSource::GetTargets(nsIRDFResource* source,
                                                nsIRDFResource* property,    
                                                PRBool tv,
                                                nsISimpleEnumerator** targets)
{
  nsresult rv = NS_RDF_NO_VALUE;
  if(!targets)
	  return NS_ERROR_NULL_POINTER;

  nsCOMPtr<nsIAbDirectory> directory(do_QueryInterface(source, &rv));
  if (NS_SUCCEEDED(rv) && directory)
  {
    if ((kNC_Child == property))
    {
      nsCOMPtr<nsIEnumerator> subDirectories;

      rv = directory->GetChildNodes(getter_AddRefs(subDirectories));
      if (NS_FAILED(rv)) return rv;
      nsAdapterEnumerator* cursor =
        new nsAdapterEnumerator(subDirectories);
      if (cursor == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;
      NS_ADDREF(cursor);
      *targets = cursor;
	  return NS_OK;
	}
    else if((kNC_DirName == property)) 
	{ 
      nsSingletonEnumerator* cursor =
        new nsSingletonEnumerator(property);
      if (cursor == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;
      NS_ADDREF(cursor);
      *targets = cursor;
	  return NS_OK;
    }
    else if((kNC_CardChild == property))
    { 
      nsCOMPtr<nsIEnumerator> cardChild;

      rv = directory->GetChildCards(getter_AddRefs(cardChild));
      if (NS_SUCCEEDED(rv) && cardChild)
	  {
		  nsAdapterEnumerator* cursor =
			new nsAdapterEnumerator(cardChild);
		  if (cursor == nsnull)
			return NS_ERROR_OUT_OF_MEMORY;
		  NS_ADDREF(cursor);
		  *targets = cursor;
		  return NS_OK;
	  }
    }
	else if((kNC_DirUri == property)) 
	{ 
      nsSingletonEnumerator* cursor =
        new nsSingletonEnumerator(property);
      if (cursor == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;
      NS_ADDREF(cursor);
      *targets = cursor;
	  return NS_OK;
    }
  }
  return NS_NewEmptyEnumerator(targets);
}

NS_IMETHODIMP nsAbDirectoryDataSource::Assert(nsIRDFResource* source,
                      nsIRDFResource* property, 
                      nsIRDFNode* target,
                      PRBool tv)
{
	nsresult rv;
	nsCOMPtr<nsIAbDirectory> directory(do_QueryInterface(source, &rv));
	//We don't handle tv = PR_FALSE at the moment.
	if(NS_SUCCEEDED(rv) && tv)
		return DoDirectoryAssert(directory, property, target);
	else
		return NS_ERROR_FAILURE;
}

NS_IMETHODIMP nsAbDirectoryDataSource::HasAssertion(nsIRDFResource* source,
                            nsIRDFResource* property,
                            nsIRDFNode* target,
                            PRBool tv,
                            PRBool* hasAssertion)
{
	nsresult rv;
	nsCOMPtr<nsIAbDirectory> directory(do_QueryInterface(source, &rv));
	if(NS_SUCCEEDED(rv))
		return DoDirectoryHasAssertion(directory, property, target, tv, hasAssertion);
	else
		*hasAssertion = PR_FALSE;
	return NS_OK;
}

NS_IMETHODIMP nsAbDirectoryDataSource::ArcLabelsOut(nsIRDFResource* source,
                                                 nsISimpleEnumerator** labels)
{
  nsCOMPtr<nsISupportsArray> arcs;
  nsresult rv = NS_RDF_NO_VALUE;

  nsCOMPtr<nsIAbDirectory> directory(do_QueryInterface(source, &rv));
  if (NS_SUCCEEDED(rv)) {
    fflush(stdout);
   rv = getDirectoryArcLabelsOut(directory, getter_AddRefs(arcs));
  }
  else {
    // how to return an empty cursor?
    // for now return a 0-length nsISupportsArray
    NS_NewISupportsArray(getter_AddRefs(arcs));
  }

  nsArrayEnumerator* cursor =
    new nsArrayEnumerator(arcs);
  
  if (cursor == nsnull)
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(cursor);
  *labels = cursor;
  
  return NS_OK;
}

nsresult
nsAbDirectoryDataSource::getDirectoryArcLabelsOut(nsIAbDirectory *directory,
                                             nsISupportsArray **arcs)
{
	nsresult rv;
	rv = NS_NewISupportsArray(arcs);
	if(NS_FAILED(rv))
		return rv;
	
	(*arcs)->AppendElement(kNC_DirName);
	(*arcs)->AppendElement(kNC_Child);
	(*arcs)->AppendElement(kNC_CardChild);
	(*arcs)->AppendElement(kNC_DirUri);
	return NS_OK;
}

NS_IMETHODIMP
nsAbDirectoryDataSource::GetAllCommands(nsIRDFResource* source,
                                      nsIEnumerator/*<nsIRDFResource>*/** commands)
{
  nsresult rv;
  nsCOMPtr<nsISupportsArray> cmds;

  nsCOMPtr<nsIAbDirectory> directory(do_QueryInterface(source, &rv));
  if (NS_SUCCEEDED(rv)) {
    rv = NS_NewISupportsArray(getter_AddRefs(cmds));
    if (NS_FAILED(rv)) return rv;
    cmds->AppendElement(kNC_Delete);
    cmds->AppendElement(kNC_DeleteCards);
    cmds->AppendElement(kNC_NewDirectory);
  }

  if (cmds != nsnull)
    return cmds->Enumerate(commands);
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsAbDirectoryDataSource::IsCommandEnabled(nsISupportsArray/*<nsIRDFResource>*/* aSources,
                                        nsIRDFResource*   aCommand,
                                        nsISupportsArray/*<nsIRDFResource>*/* aArguments,
                                        PRBool* aResult)
{
  nsresult rv;
  nsCOMPtr<nsIAbDirectory> directory;

  PRUint32 i, cnt;
  rv = aSources->Count(&cnt);
  for (i = 0; i < cnt; i++) {
    nsCOMPtr<nsISupports> source = getter_AddRefs(aSources->ElementAt(i));
		directory = do_QueryInterface(source, &rv);
    if (NS_SUCCEEDED(rv)) {
      // we don't care about the arguments -- directory commands are always enabled
      if (!((aCommand == kNC_Delete) || (aCommand == kNC_DeleteCards) ||
		    (aCommand == kNC_NewDirectory))) {
        *aResult = PR_FALSE;
        return NS_OK;
      }
    }
  }
  *aResult = PR_TRUE;
  return NS_OK; // succeeded for all sources
}

NS_IMETHODIMP
nsAbDirectoryDataSource::DoCommand(nsISupportsArray/*<nsIRDFResource>*/* aSources,
                                 nsIRDFResource*   aCommand,
                                 nsISupportsArray/*<nsIRDFResource>*/* aArguments)
{
	PRUint32 i, cnt;
	nsresult rv = aSources->Count(&cnt);
	if (NS_FAILED(rv)) return rv;

	if ((aCommand == kNC_Delete))  
		rv = DoDeleteFromDirectory(aSources, aArguments);

	for (i = 0; i < cnt; i++) 
	{
		nsCOMPtr<nsISupports> supports = getter_AddRefs(aSources->ElementAt(i));
		nsCOMPtr<nsIAbDirectory> directory = do_QueryInterface(supports, &rv);
		if (NS_SUCCEEDED(rv)) 
		{
			if ((aCommand == kNC_DeleteCards))  
				rv = DoDeleteCardsFromDirectory(directory, aArguments);
			else if((aCommand == kNC_NewDirectory)) 
				rv = DoNewDirectory(directory, aArguments);
		}
	}
	//for the moment return NS_OK, because failure stops entire DoCommand process.
	return NS_OK;
}

NS_IMETHODIMP nsAbDirectoryDataSource::OnItemAdded(nsISupports *parentDirectory, nsISupports *item)
{
	nsresult rv;
	nsCOMPtr<nsIAbCard> card;
	nsCOMPtr<nsIAbDirectory> directory;
	nsCOMPtr<nsIRDFResource> parentResource;

	if(NS_SUCCEEDED(parentDirectory->QueryInterface(NS_GET_IID(nsIRDFResource), getter_AddRefs(parentResource))))
	{ 
		//If we are adding a card
		if(NS_SUCCEEDED(item->QueryInterface(NS_GET_IID(nsIAbCard), getter_AddRefs(card))))
		{
			nsCOMPtr<nsIRDFNode> itemNode(do_QueryInterface(item, &rv));
			if (NS_SUCCEEDED(rv))
			{
				//Notify directories that a message was added.
				NotifyObservers(parentResource, kNC_CardChild, itemNode, PR_TRUE);
			}
		}
		//If we are adding a directory
		else if(NS_SUCCEEDED(item->QueryInterface(NS_GET_IID(nsIAbDirectory), getter_AddRefs(directory))))
		{
			nsCOMPtr<nsIRDFNode> itemNode(do_QueryInterface(item, &rv));
			if(NS_SUCCEEDED(rv))
			{
				//Notify a directory was added.
				NotifyObservers(parentResource, kNC_Child, itemNode, PR_TRUE);
			}
		}
	}

	return NS_OK;
}

NS_IMETHODIMP nsAbDirectoryDataSource::OnItemRemoved(nsISupports *parentDirectory, nsISupports *item)
{
	nsresult rv;
	nsCOMPtr<nsIAbCard> card;
	nsCOMPtr<nsIAbDirectory> directory;
	nsCOMPtr<nsIRDFResource> parentResource;

	if(NS_SUCCEEDED(parentDirectory->QueryInterface(NS_GET_IID(nsIRDFResource), getter_AddRefs(parentResource))))
	{
		//If we are removing a card
		if(NS_SUCCEEDED(item->QueryInterface(NS_GET_IID(nsIAbCard), getter_AddRefs(card))))
		{
			nsCOMPtr<nsIRDFNode> itemNode(do_QueryInterface(item, &rv));
			if(NS_SUCCEEDED(rv))
			{
				//Notify directories that a card was deleted.
				NotifyObservers(parentResource, kNC_CardChild, itemNode, PR_FALSE);
			}
		}
		//If we are removing a directory
		else if(NS_SUCCEEDED(item->QueryInterface(NS_GET_IID(nsIAbDirectory), getter_AddRefs(directory))))
		{
			nsCOMPtr<nsIRDFNode> itemNode(do_QueryInterface(item, &rv));
			if(NS_SUCCEEDED(rv))
			{
				//Notify a directory was deleted.
				NotifyObservers(parentResource, kNC_Child, itemNode, PR_FALSE);
			}
		}
	}
	return NS_OK;
}

NS_IMETHODIMP nsAbDirectoryDataSource::OnItemPropertyChanged(nsISupports *item, const char *property,
														   const PRUnichar *oldValue, const PRUnichar *newValue)

{
	nsresult rv;
	nsCOMPtr<nsIRDFResource> resource(do_QueryInterface(item, &rv));

	if(NS_SUCCEEDED(rv))
	{
		if(PL_strcmp("DirName", property) == 0)
		{
			NotifyPropertyChanged(resource, kNC_DirName, oldValue, newValue);
		}
	}
	return NS_OK;
}

nsresult nsAbDirectoryDataSource::createDirectoryNode(nsIAbDirectory* directory,
                                                 nsIRDFResource* property,
                                                 nsIRDFNode** target)
{
  nsresult rv = NS_RDF_NO_VALUE;
  
  if ((kNC_DirName == property))
	rv = createDirectoryNameNode(directory, target);
  if ((kNC_DirUri == property))
	rv = createDirectoryUriNode(directory, target);
  if ((kNC_Child == property))
	rv = createDirectoryChildNode(directory, target);
  
  return rv;
}


nsresult nsAbDirectoryDataSource::createDirectoryNameNode(nsIAbDirectory *directory,
                                                     nsIRDFNode **target)
{
	PRUnichar *name = nsnull;
	PRBool bIsMailList = PR_FALSE;
	nsresult rv = NS_OK;

	directory->GetIsMailList(&bIsMailList);
	if (bIsMailList)
		rv = directory->GetListName(&name);
	else
		rv = directory->GetDirName(&name);
	if (NS_FAILED(rv)) return rv;
	nsString nameString(name);
	createNode(nameString, target);
	nsCRT::free(name);
	return NS_OK;
}

nsresult nsAbDirectoryDataSource::createDirectoryUriNode(nsIAbDirectory *directory,
                                                     nsIRDFNode **target)
{
  char *uri;
  nsresult rv = directory->GetDirUri(&uri);
  if (NS_FAILED(rv)) return rv;
  nsString nameString; nameString.AssignWithConversion(uri);
  createNode(nameString, target);
  nsMemory::Free(uri);
  return NS_OK;
}

nsresult
nsAbDirectoryDataSource::createDirectoryChildNode(nsIAbDirectory *directory,
                                             nsIRDFNode **target)
{
	nsCOMPtr<nsISupportsArray> pAddressLists;
	directory->GetAddressLists(getter_AddRefs(pAddressLists));
	if (pAddressLists)
	{
		PRUint32 total = 0;
		pAddressLists->Count(&total);
		
		if (total == 0)
			return NS_RDF_NO_VALUE;
		else
		{
			PRBool bIsMailList = PR_FALSE;
			directory->GetIsMailList(&bIsMailList);
			if (bIsMailList)
				return NS_RDF_NO_VALUE;

			PRUint32 i;
			for (i = 0; i < total; i++)
			{
				nsCOMPtr<nsISupports> mailList;
				mailList = pAddressLists->ElementAt(i);
				if (mailList)
					mailList->QueryInterface(NS_GET_IID(nsIRDFResource), (void**)target);
				else
					return NS_RDF_NO_VALUE;
			}
			return NS_OK;
		}
	}
	else
		return NS_RDF_NO_VALUE;
}

nsresult nsAbDirectoryDataSource::DoDeleteFromDirectory(nsISupportsArray *parentDirs, nsISupportsArray *delDirs)
{
	PRUint32 item, itemCount;
	nsresult rv = parentDirs->Count(&itemCount);
	if (NS_FAILED(rv)) return rv;

	nsCOMPtr<nsISupportsArray> dirArray;
	NS_NewISupportsArray(getter_AddRefs(dirArray));

	for (item = 0; item < itemCount; item++) 
	{
		nsCOMPtr<nsISupports> supports = getter_AddRefs(parentDirs->ElementAt(item));
		nsCOMPtr<nsIAbDirectory> parent = do_QueryInterface(supports, &rv);
		if (NS_SUCCEEDED(rv)) 
		{
			nsCOMPtr<nsISupports> supports = getter_AddRefs(delDirs->ElementAt(item));
			nsCOMPtr<nsIAbDirectory> deletedDir(do_QueryInterface(supports));
			if(deletedDir)
			{
				rv = parent->DeleteDirectory(deletedDir);
			}
		}
	}
	return rv;
}

nsresult nsAbDirectoryDataSource::DoDeleteCardsFromDirectory(nsIAbDirectory *directory, nsISupportsArray *arguments)
{
	nsresult rv = NS_OK;
	PRUint32 itemCount;
	rv = arguments->Count(&itemCount);
	if (NS_FAILED(rv)) return rv;
	
	nsCOMPtr<nsISupportsArray> cardArray;
	NS_NewISupportsArray(getter_AddRefs(cardArray));

	//Split up deleted items into different type arrays to be passed to the folder
	//for deletion.
	PRUint32 item;
	for(item = 0; item < itemCount; item++)
	{
		nsCOMPtr<nsISupports> supports = getter_AddRefs(arguments->ElementAt(item));
		nsCOMPtr<nsIAbCard> deletedCard(do_QueryInterface(supports));
		if (deletedCard)
		{
			cardArray->AppendElement(supports);
		}
	}
	PRUint32 cnt;
	rv = cardArray->Count(&cnt);
	if (NS_FAILED(rv)) return rv;
	if (cnt > 0)
		rv = directory->DeleteCards(cardArray);
	return rv;
}

nsresult nsAbDirectoryDataSource::DoNewDirectory(nsIAbDirectory *directory, nsISupportsArray *arguments)
{
	nsresult rv = NS_OK;
	nsCOMPtr<nsISupports> elem = getter_AddRefs(arguments->ElementAt(0));
	nsCOMPtr<nsIRDFLiteral> literal = do_QueryInterface(elem, &rv);
	if(NS_SUCCEEDED(rv))
	{
		PRUnichar *name;
		literal->GetValue(&name);

		rv = directory->CreateNewDirectory(name, nsnull, PR_FALSE /* migrating */);
		nsMemory::Free(name);
	}
	return rv;
}


nsresult nsAbDirectoryDataSource::DoDirectoryAssert(nsIAbDirectory *directory, nsIRDFResource *property, nsIRDFNode *target)
{
	nsresult rv = NS_ERROR_FAILURE;
	return rv;
}


nsresult nsAbDirectoryDataSource::DoDirectoryHasAssertion(nsIAbDirectory *directory, nsIRDFResource *property, nsIRDFNode *target,
													 PRBool tv, PRBool *hasAssertion)
{
	nsresult rv = NS_OK;
	if (!hasAssertion)
		return NS_ERROR_NULL_POINTER;

	//We're not keeping track of negative assertions on directory.
	if (!tv)
	{
		*hasAssertion = PR_FALSE;
		return NS_OK;
	}

	if ((kNC_CardChild == property))
	{
		nsCOMPtr<nsIAbCard> card(do_QueryInterface(target, &rv));
		if(NS_SUCCEEDED(rv))
			rv = directory->HasCard(card, hasAssertion);
	}
	else if ((kNC_Child == property))
	{
		nsCOMPtr<nsIAbDirectory> newDirectory(do_QueryInterface(target, &rv));
		if(NS_SUCCEEDED(rv))
			rv = directory->HasDirectory(newDirectory, hasAssertion);
	}
	else 
		*hasAssertion = PR_FALSE;

	return rv;

}

nsresult NS_NewAbDirectoryDataSource(const nsIID& iid, void **result)
{
    NS_PRECONDITION(result != nsnull, "null ptr");
    if (! result)
        return NS_ERROR_NULL_POINTER;

    nsAbDirectoryDataSource* datasource = new nsAbDirectoryDataSource();
    if (! datasource)
        return NS_ERROR_OUT_OF_MEMORY;

    nsresult rv;
    rv = datasource->Init();
    if (NS_FAILED(rv)) {
        delete datasource;
        return rv;
    }

	return datasource->QueryInterface(iid, result);
}
