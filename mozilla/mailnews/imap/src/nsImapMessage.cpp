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


#include "msgCore.h"    // precompiled header...
#include "nsIMsgFolder.h"
#include "nsImapMessage.h"
#include "nsIRDFService.h"
#include "nsIServiceManager.h"
#include "nsRDFCID.h"
#include "nsXPIDLString.h"
#include "nsImapUtils.h"

static NS_DEFINE_CID(kRDFServiceCID,              NS_RDFSERVICE_CID);

nsImapMessage::nsImapMessage(void)
{

//  NS_INIT_REFCNT(); done by superclass
}

nsImapMessage::~nsImapMessage(void)
{
}

NS_IMPL_ISUPPORTS_INHERITED(nsImapMessage, nsMessage, nsIDBMessage)

NS_IMETHODIMP nsImapMessage::GetMsgFolder(nsIMsgFolder **folder)
{
	nsresult rv;
	if(mFolder)
	{
		*folder = mFolder;
		NS_ADDREF(mFolder);
		rv = NS_OK;
	}
	else
	{
		rv = GetFolderFromURI(folder);
	}
	return rv;


}

nsresult nsImapMessage::GetFolderFromURI(nsIMsgFolder **folder)
{
	nsresult rv;
	nsXPIDLCString uri;
	nsIRDFResource *resource;
	if(NS_SUCCEEDED( rv = QueryInterface(nsIRDFResource::GetIID(), (void**)&resource)))
	{
		resource->GetValue( getter_Copies(uri) );
		nsCAutoString messageFolderURIStr;
		nsMsgKey key;
		nsParseImapMessageURI(uri, messageFolderURIStr, &key);
		nsCAutoString folderOnly, folderURIStr;

		if (messageFolderURIStr.Find(kImapRootURI) != ((PRInt32)-1))
		{
			messageFolderURIStr.Right(folderOnly, messageFolderURIStr.Length() -nsCRT::strlen(kImapRootURI));
			folderURIStr = kImapRootURI;
			folderURIStr+= folderOnly;
			nsIRDFResource *folderResource;
			const char *folderURI = folderURIStr.GetBuffer();

			NS_WITH_SERVICE(nsIRDFService, rdfService, kRDFServiceCID, &rv); 
			if (NS_SUCCEEDED(rv))   // always check this before proceeding 
			{
				rv = rdfService->GetResource(folderURI, &folderResource);
				if(NS_SUCCEEDED(rv))
				{
					rv = NS_SUCCEEDED(folderResource->QueryInterface(nsIMsgFolder::GetIID(), (void**)folder));
					NS_RELEASE(folderResource);
				}
			}
		}

		NS_RELEASE(resource);
	}
	return rv;
}

NS_IMETHODIMP NS_NewImapMessage(nsISupports * aOuter, REFNSIID iid, void ** aResult)
{
  if (!aResult) return NS_ERROR_NULL_POINTER;

  if (aOuter)
  {
      *aResult = nsnull;
      return NS_ERROR_NO_AGGREGATION;
  }

  nsImapMessage * imapMsg = new nsImapMessage();
  if (!imapMsg) return NS_ERROR_OUT_OF_MEMORY;
  return imapMsg->QueryInterface(iid, aResult); 
}
