/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
 */

//#define DONT_INFORM_WEBSHELL

#include "nsIServiceManager.h"
#include "nsIDocumentLoader.h"
#include "nsIWebShellServices.h"
#include "nsIContentViewerContainer.h"
#include "nsCURILoader.h"
#include "nsObserverBase.h"

static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);

static NS_DEFINE_IID(kDocLoaderServiceCID, NS_DOCUMENTLOADER_SERVICE_CID);
static NS_DEFINE_IID(kIDocumentLoaderIID, NS_IDOCUMENTLOADER_IID);
static NS_DEFINE_IID(kIWebShellServicesIID, NS_IWEB_SHELL_SERVICES_IID);

//-------------------------------------------------------------------------
NS_IMETHODIMP nsObserverBase::NotifyWebShell(
  PRUint32 aDocumentID, const char* charset, nsCharsetSource source)
{
   nsresult res = NS_OK;
   nsresult rv = NS_OK;
   // should docLoader a member to increase performance ???
   nsIDocumentLoader * docLoader = nsnull;
   nsIContentViewerContainer * cvc  = nsnull;
   nsIWebShellServices* wss = nsnull;

   if(NS_FAILED(rv =nsServiceManager::GetService(kDocLoaderServiceCID,
                                                 kIDocumentLoaderIID,
                                                 (nsISupports**)&docLoader)))
     goto done;
   
   if(NS_FAILED(rv =docLoader->GetContentViewerContainer(aDocumentID, &cvc)))
     goto done;

   /* sspitzer:  this was necessary to get printing of mail to work (sort of)
    */
   NS_ASSERTION(cvc,"GetContentViewerContainer didn't fail, but cvc is null");
   if (!cvc) {
	goto done;
   }

   if(NS_FAILED( rv = cvc->QueryInterface(kIWebShellServicesIID, (void**)&wss)))
     goto done;

   if (!wss) {
 	goto done;
   }

#ifndef DONT_INFORM_WEBSHELL
   // ask the webshellservice to load the URL
   if(NS_FAILED( rv = wss->SetRendering(PR_FALSE) ))
     goto done;

   // XXX nisheeth, uncomment the following two line to see the reent problem

   if(NS_FAILED(rv = wss->StopDocumentLoad())){
	  rv = wss->SetRendering(PR_TRUE); // turn on the rendering so at least we will see something.
      goto done;
   }

   if(NS_FAILED(rv = wss->ReloadDocument(charset, source))) {
	  rv = wss->SetRendering(PR_TRUE); // turn on the rendering so at least we will see something.
      goto done;
   }
   res = NS_ERROR_HTMLPARSER_STOPPARSING;
#endif

done:
   if(docLoader) {
      nsServiceManager::ReleaseService(kDocLoaderServiceCID,docLoader);
   }
   NS_IF_RELEASE(cvc);
   NS_IF_RELEASE(wss);
   return res;
}
