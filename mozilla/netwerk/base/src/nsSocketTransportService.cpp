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
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

#include "nsILoadGroup.h"
#include "netCore.h"
#include "nsSocketTransportService.h"
#include "nsSocketTransport.h"
#include "nsAutoLock.h"
#include "nsIIOService.h"
#include "nsIServiceManager.h"
#include "nsString.h"

static NS_DEFINE_CID(kIOServiceCID, NS_IOSERVICE_CID);
static NS_DEFINE_CID(kStringBundleServiceCID, NS_STRINGBUNDLESERVICE_CID);

nsSocketTransportService::nsSocketTransportService ()   :
    mConnectedTransports (0),
    mTotalTransports (0)
{
  NS_INIT_REFCNT();

  PR_INIT_CLIST(&mWorkQ);

  mThread      = nsnull;
  mThreadEvent = nsnull;
  mThreadLock  = nsnull;

  mSelectFDSet      = nsnull;
  mSelectFDSetCount = 0;

  mActiveTransportList = nsnull;

  mThreadRunning = PR_FALSE;
}


nsSocketTransportService::~nsSocketTransportService()
{
  //
  // It is impossible for the nsSocketTransportService to be deleted while
  // the transport thread is running because it holds a reference to the
  // nsIRunnable (ie. the nsSocketTransportService instance)...
  //
  NS_ASSERTION(!mThread && !mThreadRunning, 
               "The socket transport thread is still running...");

  if (mSelectFDSet) {
    PR_Free(mSelectFDSet);
    mSelectFDSet = nsnull;
  }

  if (mActiveTransportList) {
    PR_Free(mActiveTransportList);
    mActiveTransportList = nsnull;
  }

  if (mThreadEvent)
  {
    PR_DestroyPollableEvent(mThreadEvent);
    mThreadEvent = nsnull;
  }

  if (mThreadLock) {
    PR_DestroyLock(mThreadLock);
    mThreadLock = nsnull;
  }
}

NS_METHOD
nsSocketTransportService::Create(nsISupports *aOuter, REFNSIID aIID, void **aResult)
{
    nsresult rv;

    if (aOuter)
        return NS_ERROR_NO_AGGREGATION;

    nsSocketTransportService* trans = new nsSocketTransportService();
    if (trans == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(trans);
    rv = trans->Init();
    if (NS_SUCCEEDED(rv)) {
        rv = trans->QueryInterface(aIID, aResult);
    }
    NS_RELEASE(trans);
    return rv;
}

NS_IMETHODIMP
nsSocketTransportService::Init(void)
{
  nsresult rv = NS_OK;

  NS_ASSERTION(!mThread, "Socket transport thread has already been created!.");

  //
  // Create FDSET list used by PR_Poll(...)
  //
  if (!mSelectFDSet) {
    mSelectFDSet = (PRPollDesc*)PR_Malloc(sizeof(PRPollDesc)*MAX_OPEN_CONNECTIONS);
    if (mSelectFDSet) {
      memset(mSelectFDSet, 0, sizeof(PRPollDesc)*MAX_OPEN_CONNECTIONS);
    } else {
      rv = NS_ERROR_OUT_OF_MEMORY;
    }
  }

  //
  // Create the list of Active transport objects...  This list contains the 
  // nsSocketTransport corresponding to each PRFileDesc* in the mSelectFDSet
  //
  if (NS_SUCCEEDED(rv) && !mActiveTransportList) {
    mActiveTransportList = (nsSocketTransport**)PR_Malloc(sizeof(nsSocketTransport*)*MAX_OPEN_CONNECTIONS);
    if (mActiveTransportList) {
      memset(mActiveTransportList, 0, sizeof(nsSocketTransport*)*MAX_OPEN_CONNECTIONS);
    } else {
      rv = NS_ERROR_OUT_OF_MEMORY;
    }
  }

  //
  // Create the pollable event used to immediately wake up the transport 
  // thread when it is blocked in PR_Poll(...)
  //
#ifdef USE_POLLABLE_EVENT
  if (NS_SUCCEEDED(rv) && !mThreadEvent)
    mThreadEvent = PR_NewPollableEvent();
#endif
  //
  // Create the synchronization lock for the transport thread...
  //
  if (NS_SUCCEEDED(rv) && !mThreadLock) {
    mThreadLock = PR_NewLock();
    if (!mThreadLock) {
      rv = NS_ERROR_OUT_OF_MEMORY;
    }
  }

  //
  // Create the transport thread...
  //
  if (NS_SUCCEEDED(rv) && !mThread) {
    mThreadRunning = PR_TRUE;
    rv = NS_NewThread(&mThread, this, 0, PR_JOINABLE_THREAD);
  }
  return rv;
}

nsresult nsSocketTransportService::AddToWorkQ(nsSocketTransport* aTransport)
{
  PRStatus status = PR_SUCCESS;
  PRBool bFireEvent = PR_FALSE;
  nsresult rv = NS_OK;
  PRCList* qp;

  {
    nsAutoLock lock(mThreadLock);
    //
    // Only add the transport if it is *not* already on the list...
    //
    qp = aTransport->GetListNode();
    if (PR_CLIST_IS_EMPTY(qp)) {
      NS_ADDREF(aTransport);
      bFireEvent = PR_CLIST_IS_EMPTY(&mWorkQ);
      PR_APPEND_LINK(qp, &mWorkQ);
    }
  }
  //
  // Only fire an event if this is the first entry in the workQ.  Otherwise,
  // the event has already been fired and the transport thread will process
  // all of the entries at once...
  //
  if (bFireEvent) {
    if (mThreadEvent)
        status = PR_SetPollableEvent(mThreadEvent);

    if (PR_FAILURE == status) {
      rv = NS_ERROR_FAILURE;
    }
  }
  return rv;
}


nsresult nsSocketTransportService::ProcessWorkQ(void)
{
  nsresult rv = NS_OK;
  PRCList* qp;

  //
  // Only process pending operations while there is space available in the
  // select list...
  //
  // XXX:  Need a way to restart the ProcessWorkQ(...) when space becomes
  //       available in the select set...
  //
  PR_Lock(mThreadLock);
  NS_ASSERTION(MAX_OPEN_CONNECTIONS > mSelectFDSetCount, "reached max open connections");
  while (!PR_CLIST_IS_EMPTY(&mWorkQ) && 
         (MAX_OPEN_CONNECTIONS > mSelectFDSetCount)) {
    nsSocketTransport* transport;

    // Get the next item off of the workQ...
    qp = PR_LIST_HEAD(&mWorkQ);

    transport = nsSocketTransport::GetInstance(qp);
    PR_REMOVE_AND_INIT_LINK(qp);
    //
    // Make sure that the transport is not already on the select list.
    // It will be added (if necessary) after Process() is called...
    //
    RemoveFromSelectList(transport);

    // Try to perform the operation...  
    //
    // Do not process the transport while holding the transport service 
    // lock...  A deadlock could occur if another thread is holding the 
    // transport lock and tries to add the transport to the service's WorkQ...
    //
    // Do not pass any select flags...
    PR_Unlock(mThreadLock);
    rv = transport->Process(0);
    PR_Lock(mThreadLock);
    //
    // If the operation would block, then add it to the select list for
    // later processing when the data arrives...
    //
    if (NS_BASE_STREAM_WOULD_BLOCK == rv) {
      rv = AddToSelectList(transport);
    }
    // Release the transport object (since it is no longer on the WorkQ).
    NS_RELEASE(transport);
  }
  PR_Unlock(mThreadLock);

  return rv;
}

nsresult nsSocketTransportService::AddToSelectList(nsSocketTransport* aTransport)
{
  nsresult rv = NS_OK;

  NS_ASSERTION(MAX_OPEN_CONNECTIONS > mSelectFDSetCount, "reached max open connections");
  if (aTransport && (MAX_OPEN_CONNECTIONS > mSelectFDSetCount) ) {
    PRPollDesc* pfd;
    int i;
    //
    // Check to see if the transport is already in the list...
    //
    // If the first FD is the Pollable Event, it will be ignored since
    // its corresponding entry in the ActiveTransportList is nsnull.
    //
    for (i=0; i<mSelectFDSetCount; i++) {
      if (mActiveTransportList[i] == aTransport) {
        break;
      }
    }
    // Initialize/update the info in the entry...
    pfd = &mSelectFDSet[i];
    pfd->fd        = aTransport->GetSocket();;
    pfd->in_flags  = aTransport->GetSelectFlags();
    pfd->out_flags = 0;
    // Add the FileDesc to the PRPollDesc list...
    if (i == mSelectFDSetCount) {
      // Add the transport instance to the corresponding active transport list...
      NS_ADDREF(aTransport);
      mActiveTransportList[mSelectFDSetCount] = aTransport;
      mSelectFDSetCount += 1;
    }
  }

  return rv;
}


nsresult nsSocketTransportService::RemoveFromSelectList(nsSocketTransport* aTransport)
{
  int i;
  nsresult rv = NS_ERROR_FAILURE;

  if (!aTransport) return rv;

  //
  // Remove the transport from SelectFDSet and ActiveTransportList...
  //
  // If the first FD is the Pollable Event, it will be ignored since
  // its corresponding entry in the ActiveTransportList is nsnull.
  //
  for (i=0; i<mSelectFDSetCount; i++) {
    if (mActiveTransportList[i] == aTransport) {
      int last = mSelectFDSetCount-1;

      NS_RELEASE(mActiveTransportList[i]);

      // Move the last element in the array into the new empty slot...
      if (i != last) {
        memcpy(&mSelectFDSet[i], &mSelectFDSet[last], sizeof(mSelectFDSet[0]));
        mSelectFDSet[last].fd = nsnull;

        mActiveTransportList[i]    = mActiveTransportList[last];
        mActiveTransportList[last] = nsnull;
      } else {
        mSelectFDSet[i].fd      = nsnull;
        mActiveTransportList[i] = nsnull;
      }
      mSelectFDSetCount -= 1;
      rv = NS_OK;
      break;
    }
  }

  return rv;
}


//
// --------------------------------------------------------------------------
// nsISupports implementation...
// --------------------------------------------------------------------------
//
NS_IMPL_THREADSAFE_ISUPPORTS2(nsSocketTransportService,
                              nsISocketTransportService,
                              nsIRunnable)

//
// --------------------------------------------------------------------------
// nsIRunnable implementation...
// --------------------------------------------------------------------------
//
NS_IMETHODIMP
nsSocketTransportService::Run(void)
{
  PRIntervalTime pollTimeout;

  if (mThreadEvent)
  {
    //
    // Initialize the FDSET used by PR_Poll(...).  The first item in the FDSet
    // is *always* the pollable event (ie. mThreadEvent).
    //
    mSelectFDSet[0].fd       = mThreadEvent;
    mSelectFDSet[0].in_flags = PR_POLL_READ;
    mSelectFDSetCount = 1;
    pollTimeout = PR_MillisecondsToInterval (DEFAULT_POLL_TIMEOUT_IN_MS);
  }
  else
  {
    //
    // For now, rather than breaking out of the call to PR_Poll(...) just set
    // the time out small enough...
    //
    // This means that new transports will only be processed once a timeout
    // occurs...
    //
    mSelectFDSetCount = 0;
    pollTimeout = PR_MillisecondsToInterval(5);
  }

  while (mThreadRunning) {
    nsresult rv;
    PRInt32 count;
    PRIntervalTime intervalNow;
    nsSocketTransport* transport;
    int i;

    count = PR_Poll(mSelectFDSet, mSelectFDSetCount, pollTimeout);

    if (-1 == count) {
      // XXX: PR_Poll failed...  What should happen?
    }

    intervalNow = PR_IntervalNow();
    //
    // See if any sockets have data...
    //
    // Walk the list of active transports backwards to avoid missing
    // elements when a transport is removed...
    //
    for (i=mSelectFDSetCount-1; i>=0; i--) {
      PRPollDesc* pfd;
      PRInt16 out_flags;

      transport = mActiveTransportList[i];
      pfd = &mSelectFDSet[i];
  
      /* Process any sockets with data first... */
      //
      // XXX: PR_Poll(...) has the unpleasent behavior of ONLY setting the
      //      out_flags if one or more FDs are ready.  So, DO NOT look at
      //      the out_flags unless count > 0.
      //
      if ((count > 0) && pfd->out_flags) {
        // Clear the out_flags for next time...
        out_flags = pfd->out_flags;
        pfd->out_flags = 0;

        if (transport) {
          rv = transport->Process(out_flags);
          if (NS_BASE_STREAM_WOULD_BLOCK == rv) {
            // Update the select flags...
            pfd->in_flags = transport->GetSelectFlags();
          }
          //
          // If the operation completed, then remove the entry from the
          // select list...
          //
          else {
            rv = RemoveFromSelectList(transport);
          }
        }
        else {
          if (mThreadEvent)
          {
            /* Process any pending operations on the mWorkQ... */
            NS_ASSERTION(0 == i, "Null transport in active list...");
            if (0 == i) {
              //
              // Clear the pollable event...  This call should *never* block since 
              // PR_Poll(...) said that it had been fired...
              //
              NS_ASSERTION(!(mSelectFDSet[0].out_flags & PR_POLL_EXCEPT), 
                         "Exception on Pollable event.");
              PR_WaitForPollableEvent(mThreadEvent);

              rv = ProcessWorkQ();
            }
          }
          else
          {
            //
            // The pollable event should be the *only* null transport
            // in the active transport list.
            //
            NS_ASSERTION(transport, "Null transport in active list...");
          }
        }
      //
      // Check to see if the transport has timed out...
      //
      } else {
        if (transport) {
          rv = transport->CheckForTimeout(intervalNow);
          if (NS_ERROR_NET_TIMEOUT == rv) {
            // Process the timeout...
            rv = transport->Process(0);
            //
            // The operation has completed.  Remove the entry from the
            // select list///
            //
            rv = RemoveFromSelectList(transport);
          }
        }
      }
    } // end-for

    if (!mThreadEvent)
        /* Process any pending operations on the mWorkQ... */
        rv = ProcessWorkQ();
  }

  return NS_OK;
}


//
// --------------------------------------------------------------------------
// nsISocketTransportService implementation...
// --------------------------------------------------------------------------
//
NS_IMETHODIMP
nsSocketTransportService::CreateTransport(const char* aHost, 
                                          PRInt32 aPort,
                                          const char* proxyHost, 
                                          PRInt32 proxyPort, 
                                          PRUint32 bufferSegmentSize,
                                          PRUint32 bufferMaxSize, 
                                          nsIChannel** aResult)
{
    return CreateTransportOfTypes(0, nsnull, aHost, aPort, proxyHost, proxyPort,
				  bufferSegmentSize, bufferMaxSize, aResult);
}

NS_IMETHODIMP
nsSocketTransportService::CreateTransportOfType(const char* aSocketType,
                                                const char* aHost, 
                                                PRInt32 aPort,
                                                const char* proxyHost, 
                                                PRInt32 proxyPort, 
                                                PRUint32 bufferSegmentSize,
                                                PRUint32 bufferMaxSize,
                                                nsIChannel** aResult)
{
    const char * types[] = { aSocketType };
    return CreateTransportOfTypes(1, types, aHost, aPort, proxyHost, proxyPort,
				  bufferSegmentSize, bufferMaxSize, aResult);
}

NS_IMETHODIMP
nsSocketTransportService::CreateTransportOfTypes(PRUint32 socketTypeCount,
						 const char* *aSocketTypes,
						 const char* aHost,
						 PRInt32 aPort,
						 const char* proxyHost,
						 PRInt32 proxyPort,
						 PRUint32 bufferSegmentSize,
						 PRUint32 bufferMaxSize,
						 nsIChannel** aResult)
{
    nsresult rv = NS_OK;
    
    NS_WITH_SERVICE(nsIIOService, ios, kIOServiceCID, &rv);
    if (NS_FAILED(rv)) return rv;
    PRBool offline;
    rv = ios->GetOffline(&offline);
    if (NS_FAILED(rv)) return rv;
    if (offline) return NS_ERROR_OFFLINE;
    
    nsSocketTransport* transport = nsnull;
    
    // Parameter validation...
    NS_ASSERTION(aResult, "aResult == nsnull.");
    if (!aResult) {
	return NS_ERROR_NULL_POINTER;
    }
    
    // Create and initialize a new connection object...
    NS_NEWXPCOM(transport, nsSocketTransport);
    if (transport) {
	rv = transport->Init(this, aHost, aPort, socketTypeCount, aSocketTypes, 
			     proxyHost, proxyPort, bufferSegmentSize, bufferMaxSize);
	if (NS_FAILED(rv)) {
	    delete transport;
	    transport = nsnull;
	}
    } 
    else {
	rv = NS_ERROR_OUT_OF_MEMORY;
    }
    
    // Set the reference count to one...
    if (NS_SUCCEEDED(rv)) {
	NS_ADDREF(transport);
    } 
    *aResult = transport;
    
    return rv;
}

NS_IMETHODIMP
nsSocketTransportService::ReuseTransport(nsIChannel* i_Transport, 
        PRBool * o_Reuse)
{
    nsresult rv = NS_ERROR_FAILURE;
    if (!i_Transport)
        return NS_ERROR_NULL_POINTER;
    nsSocketTransport* trans = NS_STATIC_CAST(nsSocketTransport*, 
        i_Transport);
    if (!trans) return rv;
    *o_Reuse = trans->CanBeReused();
    return NS_OK;
}

/**
 * wakeup the transport from PR_Poll ()
 */

NS_IMETHODIMP
nsSocketTransportService::Wakeup (nsIChannel* i_Transport)
{
    nsSocketTransport *transport = NS_STATIC_CAST (nsSocketTransport *, i_Transport);

    if (transport == NULL)
        return NS_ERROR_NULL_POINTER;

    AddToWorkQ (transport);

    if (mThreadEvent)
        PR_SetPollableEvent (mThreadEvent);

    // else
    // XXX/ruslan: normally we would call PR_Interrupt (), but since it did work
    // wait till NSPR fixes it one day

    return NS_OK;
}

NS_IMETHODIMP
nsSocketTransportService::Shutdown(void)
{
  PRStatus status;
  nsresult rv = NS_OK;

  if (mThread) {
    //
    // Clear the running flag and wake up the transport thread...
    //
    mThreadRunning = PR_FALSE;

    if (mThreadEvent)
    {
        status = PR_SetPollableEvent(mThreadEvent);
        // XXX: what should happen if this fails?
        NS_ASSERTION(PR_SUCCESS == status, "Unable to wake up the transport thread.");
    }
    else
        status = PR_SUCCESS;
    
    // Wait for the transport thread to exit nsIRunnable::Run()
    if (PR_SUCCESS == status) {
      mThread->Join();
    } 

    NS_RELEASE(mThread);
  } else {
    rv = NS_ERROR_FAILURE;
  }

  return rv;
}

NS_IMETHODIMP
nsSocketTransportService::GetTotalTransportCount    (PRUint32 * o_TransCount)
{
    if (!o_TransCount)
        return NS_ERROR_NULL_POINTER;

    *o_TransCount = (PRUint32) mTotalTransports;
    return NS_OK;
}

NS_IMETHODIMP
nsSocketTransportService::GetConnectedTransportCount (PRUint32 * o_TransCount)
{
    if (!o_TransCount)
        return NS_ERROR_NULL_POINTER;

    *o_TransCount = (PRUint32) mConnectedTransports;
    return NS_OK;
}

NS_IMETHODIMP
nsSocketTransportService::GetInUseTransportCount     (PRUint32 * o_TransCount)
{
    if (!o_TransCount)
        return NS_ERROR_NULL_POINTER;

    *o_TransCount = (PRUint32) mSelectFDSetCount;
    return NS_OK;
}

nsresult
nsSocketTransportService::GetNeckoStringByName (const char *aName, PRUnichar **aString)
{
	nsresult res;
	nsAutoString	resultString; resultString.AssignWithConversion(aName);

    if (!m_stringBundle) {
        char*  propertyURL = NECKO_MSGS_URL;

        NS_WITH_SERVICE(nsIStringBundleService, sBundleService, kStringBundleServiceCID, &res);
        if (NS_SUCCEEDED (res) && (nsnull != sBundleService)) {
            nsILocale   *locale = nsnull;
            res = sBundleService->CreateBundle(propertyURL, locale, getter_AddRefs(m_stringBundle));
        }
    }

    if (m_stringBundle)
	{
		nsAutoString unicodeName; unicodeName.AssignWithConversion(aName);

		PRUnichar *ptrv = nsnull;
		res = m_stringBundle->GetStringFromName(unicodeName.GetUnicode(), &ptrv);

		if (NS_FAILED(res)) 
		{
			resultString.AssignWithConversion("[StringName");
			resultString.AppendWithConversion(aName);
			resultString.AppendWithConversion("?]");
			*aString = resultString.ToNewUnicode();
		}
		else
		{
			*aString = ptrv;
		}
	}
	else
	{
		res = NS_OK;
		*aString = resultString.ToNewUnicode();
	}

	return res;
}
