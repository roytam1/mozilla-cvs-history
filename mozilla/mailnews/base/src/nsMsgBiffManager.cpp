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
 * Copyright (C) 1999 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

#include "nsMsgBiffManager.h"
#include "nsCRT.h"
#include "nsIMsgMailSession.h"
#include "nsIMsgAccountManager.h"
#include "nsMsgBaseCID.h"
#include "nsIObserverService.h"

static NS_DEFINE_CID(kMsgAccountManagerCID, NS_MSGACCOUNTMANAGER_CID);

NS_IMPL_ISUPPORTS4(nsMsgBiffManager, nsIMsgBiffManager, nsIIncomingServerListener, nsIObserver, nsISupportsWeakReference)

void OnBiffTimer(nsITimer *timer, void *aBiffManager)
{
	nsMsgBiffManager *biffManager = (nsMsgBiffManager*)aBiffManager;
	biffManager->PerformBiff();		
}

nsMsgBiffManager::nsMsgBiffManager()
{
	NS_INIT_REFCNT();

	mBiffTimer = nsnull;
	mBiffArray = nsnull;
	mHaveShutdown = PR_FALSE;
}

nsMsgBiffManager::~nsMsgBiffManager()
{
	nsresult rv;

	if (mBiffTimer) {
		mBiffTimer->Cancel();
	}
	NS_IF_RELEASE(mBiffTimer);

	PRInt32 count = mBiffArray->Count();
    PRInt32 i;
	for(i=0; i < count; i++)
	{
		nsBiffEntry *biffEntry = (nsBiffEntry*)mBiffArray->ElementAt(i);
		delete biffEntry;
	}
	delete mBiffArray;

	if(!mHaveShutdown)
	{
		Shutdown();
		//Don't remove from Observer service in Shutdown because Shutdown also gets called
		//from xpcom shutdown observer.  And we don't want to remove from the service in that case.
		NS_WITH_SERVICE (nsIObserverService, observerService, NS_OBSERVERSERVICE_PROGID, &rv);
		if (NS_SUCCEEDED(rv))
		{    
			nsAutoString topic(NS_XPCOM_SHUTDOWN_OBSERVER_ID);
			observerService->RemoveObserver(this, topic.GetUnicode());
		}
	}
}

nsresult nsMsgBiffManager::Init()
{
	nsresult rv;

	mBiffArray = new nsVoidArray();
	if(!mBiffArray)
		return NS_ERROR_OUT_OF_MEMORY;

	NS_WITH_SERVICE(nsIMsgAccountManager, accountManager, kMsgAccountManagerCID, &rv);
	if (NS_SUCCEEDED(rv))
	{
		accountManager->AddIncomingServerListener(this);
	}

	NS_WITH_SERVICE (nsIObserverService, observerService, NS_OBSERVERSERVICE_PROGID, &rv);
	if (NS_SUCCEEDED(rv))
	{    
		nsAutoString topic(NS_XPCOM_SHUTDOWN_OBSERVER_ID);
		observerService->AddObserver(this, topic.GetUnicode());
	}

	return NS_OK;
}

nsresult nsMsgBiffManager::Shutdown()
{
	nsresult rv;
	NS_WITH_SERVICE(nsIMsgAccountManager, accountManager, kMsgAccountManagerCID, &rv);
	if (NS_SUCCEEDED(rv))
	{
		accountManager->RemoveIncomingServerListener(this);
	}

	mHaveShutdown = PR_TRUE;
	return NS_OK;
}

NS_IMETHODIMP nsMsgBiffManager::AddServerBiff(nsIMsgIncomingServer *server)
{
	nsresult rv;
	PRInt32 biffMinutes;

	rv = server->GetBiffMinutes(&biffMinutes);
	if(NS_FAILED(rv))
		return rv;

	//Don't add if biffMinutes isn't > 0
	if(biffMinutes > 0)
	{
		PRInt32 serverIndex = FindServer(server);
		//Only add it if it hasn't been added already.
		if(serverIndex == -1)
		{
			nsBiffEntry *biffEntry = new nsBiffEntry;
			if(!biffEntry)
				return NS_ERROR_OUT_OF_MEMORY;
			biffEntry->server = server;
			nsTime currentTime;
			rv = SetNextBiffTime(biffEntry, currentTime);
			if(NS_FAILED(rv))
				return rv;

			AddBiffEntry(biffEntry);
			SetupNextBiff();
		}
	}
	return NS_OK;
}

NS_IMETHODIMP nsMsgBiffManager::RemoveServerBiff(nsIMsgIncomingServer *server)
{
	PRInt32 pos = FindServer(server);
	if(pos != -1)
	{
		nsBiffEntry *biffEntry = (nsBiffEntry*)mBiffArray->ElementAt(pos);
		mBiffArray->RemoveElementAt(pos);
		delete biffEntry;
	}

	//Should probably reset biff time if this was the server that gets biffed next.
	return NS_OK;
}


NS_IMETHODIMP nsMsgBiffManager::ForceBiff(nsIMsgIncomingServer *server)
{
	return NS_OK;
}

NS_IMETHODIMP nsMsgBiffManager::ForceBiffAll()
{
	return NS_OK;
}

NS_IMETHODIMP nsMsgBiffManager::OnServerLoaded(nsIMsgIncomingServer *server)
{
	nsresult rv;
	PRBool doBiff = PR_FALSE;

    rv = server->GetDoBiff(&doBiff);

	if(NS_SUCCEEDED(rv) && doBiff)
	{
		rv = AddServerBiff(server);
	}

	return rv;
}

NS_IMETHODIMP nsMsgBiffManager::OnServerUnloaded(nsIMsgIncomingServer *server)
{
	nsresult rv;
	PRBool doBiff = PR_FALSE;

    rv = server->GetDoBiff(&doBiff);

	if(NS_SUCCEEDED(rv) && doBiff)
	{
		rv = RemoveServerBiff(server);
	}

	return rv;
}

NS_IMETHODIMP nsMsgBiffManager::Observe(nsISupports *aSubject, const PRUnichar *aTopic, const PRUnichar *someData)
{
	nsAutoString topicString(aTopic);
	nsAutoString shutdownString(NS_XPCOM_SHUTDOWN_OBSERVER_ID);

	if(topicString == shutdownString)
	{
		Shutdown();
	}
	
	return NS_OK;
}

PRInt32 nsMsgBiffManager::FindServer(nsIMsgIncomingServer *server)
{
	PRInt32 count = mBiffArray->Count();
	for(PRInt32 i = 0; i < count; i++)
	{
		nsBiffEntry *biffEntry = (nsBiffEntry*)mBiffArray->ElementAt(i);
		if(server == biffEntry->server.get())
			return i;
	}
	return -1;
}

nsresult nsMsgBiffManager::AddBiffEntry(nsBiffEntry *biffEntry)
{
	PRInt32 i;
	PRInt32 count = mBiffArray->Count();
	for(i = 0; i < count; i++)
	{
		nsBiffEntry *current = (nsBiffEntry*)mBiffArray->ElementAt(i);
		if(biffEntry->nextBiffTime < current->nextBiffTime)
			break;

	}
	mBiffArray->InsertElementAt(biffEntry, i);
	return NS_OK;
}

nsresult nsMsgBiffManager::SetNextBiffTime(nsBiffEntry *biffEntry, nsTime startTime)
{
	nsresult rv;
	nsIMsgIncomingServer *server = biffEntry->server;

	if(!server)
		return NS_ERROR_FAILURE;

	PRInt32 biffInterval;
	rv = server->GetBiffMinutes(&biffInterval);
	if(NS_FAILED(rv))
		return rv;
	//Add 60 secs/minute in microseconds to current time. biffEntry->nextBiffTime's
	//constructor makes it the current time.
	biffEntry->nextBiffTime = startTime;
	biffEntry->nextBiffTime += (60000000 * biffInterval);
	return NS_OK;
}

nsresult nsMsgBiffManager::SetupNextBiff()
{
	
	if(mBiffArray->Count() > 0)
	{
		
		//Get the next biff entry
		nsBiffEntry *biffEntry = (nsBiffEntry*)mBiffArray->ElementAt(0);
		nsTime currentTime;
		nsInt64 biffDelay;
		nsInt64 ms(1000);
		if(currentTime > biffEntry->nextBiffTime)
			biffDelay = 1;
		else
			biffDelay = biffEntry->nextBiffTime - currentTime;
		//Convert biffDelay into milliseconds
		nsInt64 timeInMS = biffDelay / ms;
		PRUint32 timeInMSUint32 = (PRUint32)timeInMS;
		//Can't currently reset a timer when it's in the process of
		//calling Notify. So, just release the timer here and create a new one.
		if(mBiffTimer)
		{
			mBiffTimer->Cancel();
			NS_RELEASE(mBiffTimer);
		}
		NS_NewTimer(&mBiffTimer);
		mBiffTimer->Init(OnBiffTimer, (void*)this, timeInMSUint32);
		
	}
	return NS_OK;
}

//This is the function that does a biff on all of the servers whose time it is to biff.
nsresult nsMsgBiffManager::PerformBiff()
{
	nsTime currentTime;
	for(PRInt32 i = 0; i < mBiffArray->Count(); i++)
	{
		nsBiffEntry *current = (nsBiffEntry*)mBiffArray->ElementAt(i);
		if(current->nextBiffTime < currentTime)
		{
			PRBool serverBusy = PR_FALSE;
			PRBool serverRequiresPassword = PR_TRUE;
			char *password = nsnull;
			// we don't want to prompt the user for password UI so pass in false to
			// the server->GetPassword method. If we don't already know the passsword then 
			// we just won't biff this server
			current->server->GetPassword(&password);
			current->server->GetServerBusy(&serverBusy);
			current->server->GetServerRequiresPasswordForBiff(&serverRequiresPassword);
			//Make sure we're logged on before doing a biff
			// and make sure the server isn't already in the middle of downloading new messages
			if(!serverBusy && (!serverRequiresPassword || (password && (nsCRT::strcmp(password, "") != 0))))
				current->server->PerformBiff();
			mBiffArray->RemoveElementAt(i);
			i--; //Because we removed it we need to look at the one that just moved up.
			SetNextBiffTime(current, currentTime);
			AddBiffEntry(current);
		}
		else
			//since we're in biff order, there's no reason to keep checking
			break;
	}
	SetupNextBiff();
	return NS_OK;
}


