/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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

#include "msgCore.h"    // precompiled header...

#ifdef XP_PC
#include <windows.h>    // for InterlockedIncrement
#endif

#include "nsINetService.h"
#include "nsPop3Service.h"
#include "nsIMsgMailSession.h"
#include "nsIMsgIdentity.h"

#include "nsPop3Url.h"
#include "nsPop3Sink.h"
#include "nsPop3Protocol.h"

static NS_DEFINE_CID(kCMsgMailSessionCID, NS_MSGMAILSESSION_CID); 

nsPop3Service::nsPop3Service()
{
    NS_INIT_REFCNT();
}

nsPop3Service::~nsPop3Service()
{}

NS_IMPL_THREADSAFE_ISUPPORTS(nsPop3Service, nsIPop3Service::GetIID());

nsresult nsPop3Service::GetNewMail(nsIUrlListener * aUrlListener, nsIURL ** aURL)
{
	NS_LOCK_INSTANCE();
	nsresult rv = NS_OK;
	const char * userName = nsnull;
	const char * popPassword = nsnull;
	const char * mailDirectory = nsnull;
	const char * popServer = nsnull;

	char * url = nsnull;
	nsIMsgMailSession * mailSession = nsnull;
	nsIMsgIdentity * identity = nsnull;
	nsIPop3URL * pop3Url = nsnull;
	nsPop3Protocol * protocol = nsnull; 

	// get the current identity from the mail session....
	rv = nsServiceManager::GetService(kCMsgMailSessionCID,
	    							  nsIMsgMailSession::GetIID(),
                                      (nsISupports **) &mailSession);
	if (NS_SUCCEEDED(rv) && mailSession)
	{
		identity = nsnull;
		rv = mailSession->GetCurrentIdentity(&identity);
		// now release the mail service because we are done with it
		nsServiceManager::ReleaseService(kCMsgMailSessionCID, mailSession);
	}

	if (NS_SUCCEEDED(rv) && identity)
	{
		// load up required identity information
		identity->GetPopName(&userName);
		identity->GetPopPassword(&popPassword);
		identity->GetRootFolderPath(&mailDirectory);
		identity->GetPopServer(&popServer);
	}

	// now construct a pop3 url...
	char * urlSpec = PR_smprintf("pop3://%s", popServer);
	rv = BuildPop3Url(urlSpec, mailDirectory, &pop3Url);
	PR_FREEIF(urlSpec);

	if (NS_SUCCEEDED(rv) && pop3Url) 
	{
		// does the caller want to listen to the url?
		if (aUrlListener)
			pop3Url->RegisterListener(aUrlListener);

		nsPop3Protocol * protocol = new nsPop3Protocol(pop3Url);
		if (protocol)
		{
			protocol->SetUsername(userName);
			protocol->SetPassword(popPassword);
			protocol->Load(pop3Url);
		} // if pop server 
	}

	if (aURL && pop3Url) // we already have a ref count on pop3url...
		*aURL = pop3Url; // transfer ref count to the caller...
	else
		NS_IF_RELEASE(pop3Url); // release our ref...
	
	NS_UNLOCK_INSTANCE();
	return rv;
}

nsresult nsPop3Service::BuildPop3Url(const char * urlSpec, const char * mailDirectory, nsIPop3URL ** aUrl)
{
	nsresult rv = NS_OK;
	// create a sink to run the url with
	nsPop3Sink * pop3Sink = new nsPop3Sink();
	if (pop3Sink)
		pop3Sink->SetMailDirectory(mailDirectory);

	// now create a pop3 url and a protocol instance to run the url....
	nsPop3URL * pop3Url = new nsPop3URL(nsnull, nsnull);
	if (pop3Url)
	{
		pop3Url->SetPop3Sink(pop3Sink);
		pop3Url->ParseURL(urlSpec);
	}

	if (aUrl)
		rv = pop3Url->QueryInterface(nsIPop3URL::GetIID(), (void **) aUrl);
	else  // hmmm delete is protected...what can we do here? no one has a ref cnt on the object...
		NS_IF_RELEASE(pop3Url);	 
//		delete pop3Url;

	return rv;
}
