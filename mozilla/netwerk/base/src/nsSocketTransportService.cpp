/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: NPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is 
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or 
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the NPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "nsILoadGroup.h"
#include "netCore.h"
#include "nsSocketTransportService.h"
#include "nsSocketTransport.h"
#include "nsAutoLock.h"
#include "nsIIOService.h"
#include "nsIServiceManager.h"
#include "nsICategoryManager.h"
#include "nsProxiedService.h"
#include "nsString.h"
#include "nsPrintfCString.h"
#include "nsReadableUtils.h"
#include "nsNetCID.h"
#include "prlog.h"
#include "plstr.h"
#include "nsProtocolProxyService.h"

#if defined(PR_LOGGING)
static PRLogModuleInfo *gSocketTransportServiceLog = nsnull;
#endif

#define LOG(args) PR_LOG(gSocketTransportServiceLog, PR_LOG_DEBUG, args)

#ifdef DEBUG
// in debug builds this will be valid while the socket transport service is active.
PRThread *NS_SOCKET_THREAD = 0;
#endif

static NS_DEFINE_CID(kIOServiceCID, NS_IOSERVICE_CID);
static NS_DEFINE_CID(kStringBundleServiceCID, NS_STRINGBUNDLESERVICE_CID);
static NS_DEFINE_CID(kDNSService, NS_DNSSERVICE_CID);

nsSocketTransportService::nsSocketTransportService ()   :
    mAutodialEnabled(PR_FALSE),
    mConnectedTransports (0),
    mTotalTransports (0)
{
#if defined(PR_LOGGING)
  if (!gSocketTransportServiceLog)
    gSocketTransportServiceLog = PR_NewLogModule("nsSocketTransportService");
#endif
  LOG(("nsSocketTransportService::nsSocketTransportService"));
    
  NS_INIT_ISUPPORTS();

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
  LOG(("nsSocketTransportService::~nsSocketTransportService"));
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
    mSelectFDSet = (PRPollDesc*)PR_Calloc(MAX_OPEN_CONNECTIONS, sizeof(PRPollDesc));
    if (!mSelectFDSet) {
      rv = NS_ERROR_OUT_OF_MEMORY;
    }
  }

  //
  // Create the list of Active transport objects...  This list contains the 
  // nsSocketTransport corresponding to each PRFileDesc* in the mSelectFDSet
  //
  if (NS_SUCCEEDED(rv) && !mActiveTransportList) {
    mActiveTransportList = (nsSocketTransport**)PR_Calloc(MAX_OPEN_CONNECTIONS, sizeof(nsSocketTransport*));
    if (!mActiveTransportList) {
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

  //
  // Cache the DNS service...
  //
  if (NS_SUCCEEDED(rv) && !mDNSService) {
    mDNSService = do_GetService(kDNSService);
    if (!mDNSService) {
      rv = NS_ERROR_UNEXPECTED;
    }
  }

  //
  // Cache the Event Queue service...
  //
  if (NS_SUCCEEDED(rv) && !mEventQService) {
    mEventQService = do_GetService(NS_EVENTQUEUESERVICE_CONTRACTID);
    if (!mEventQService) {
      rv = NS_ERROR_UNEXPECTED;
    }
  }

  //
  // Initialize hostname database
  //
  PL_DHashTableInit(&mHostDB, &ops, nsnull, sizeof(nsHostEntry), 0);

  return rv;
}

nsresult nsSocketTransportService::AddToWorkQ(nsSocketTransport* aTransport)
{
  PRStatus status = PR_SUCCESS;
  PRBool bFireEvent = PR_FALSE;
  nsresult rv = NS_OK;
  PRCList* qp;

  NS_ASSERTION(mThreadRunning, "adding transport after service is shut down");

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
      LOG(("nsSocketTransportService::AddToWorkQ:  Adding transport %p.",
           aTransport));
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

    LOG(("nsSocketTransportService::ProcessWorkQ:  Processing transport %p.",
         transport));
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
    pfd->fd        = aTransport->GetSocket();
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
#if defined(USE_POLLABLE_EVENT) && defined(XP_BEOS)
  PRBool hadThreadEvent = mThreadEvent ? PR_TRUE : PR_FALSE;
#endif

#ifdef DEBUG
  NS_SOCKET_THREAD = PR_GetCurrentThread();
#endif

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

#if defined(USE_POLLABLE_EVENT) && defined(XP_BEOS)

    // If we lose the pollable event while running,
    // attempt to get it back

    if (hadThreadEvent && !mThreadEvent) {
      mThreadEvent = PR_NewPollableEvent();
      mSelectFDSet[0].fd = mThreadEvent;

      if (!mThreadEvent) {
        NS_WARNING("Failed to recreate mThreadEvent. Falling back to non-pollable events.");
        mSelectFDSet[0].in_flags = 0;
        pollTimeout = PR_MillisecondsToInterval(5);
      } else {
        NS_WARNING("Finally recreated pollable event.");
        mSelectFDSet[0].in_flags = PR_POLL_READ;
        pollTimeout = PR_MillisecondsToInterval (DEFAULT_POLL_TIMEOUT_IN_MS);
      }
    }
#endif

    count = PR_Poll(mSelectFDSet, mSelectFDSetCount, pollTimeout);

    if (-1 == count) {
      // XXX: PR_Poll failed...  What should happen?
#if defined(USE_POLLABLE_EVENT) && defined(XP_BEOS)
      // If PR_Poll() failed, chances are that something catastrophic 
      // happened to the ip stack.....like net_server was restarted ;-P
      // Delete the sockets and start over

      NS_WARNING("PR_Poll() failed.");
      if (mThreadEvent) {
        PRStatus status = PR_DestroyPollableEvent(mThreadEvent);
#ifdef DEBUG
        fprintf(stderr, "PR_DestroyPollableEvent(%p) = %d\n", mThreadEvent, status);
#endif
      }
      mThreadEvent = 0;
      continue;
#endif
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
#ifdef DEBUG
  NS_SOCKET_THREAD = 0;
#endif
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
                                          nsIProxyInfo* proxyInfo,
                                          PRUint32 bufferSegmentSize,
                                          PRUint32 bufferMaxSize, 
                                          nsITransport** aResult)
{
    return CreateTransportOfTypes(0, nsnull, aHost, aPort, proxyInfo,
				  bufferSegmentSize, bufferMaxSize, aResult);
}

NS_IMETHODIMP
nsSocketTransportService::CreateTransportOfType(const char* aSocketType,
                                                const char* aHost, 
                                                PRInt32 aPort,
                                                nsIProxyInfo* proxyInfo,
                                                PRUint32 bufferSegmentSize,
                                                PRUint32 bufferMaxSize,
                                                nsITransport** aResult)
{
    const char * types[] = { aSocketType };
    return CreateTransportOfTypes(1, types, aHost, aPort, proxyInfo,
				  bufferSegmentSize, bufferMaxSize, aResult);
}

NS_IMETHODIMP
nsSocketTransportService::CreateTransportOfTypes(PRUint32 socketTypeCount,
						 const char* *aSocketTypes,
						 const char* aHost,
						 PRInt32 aPort,
						 nsIProxyInfo* aProxyInfo,
						 PRUint32 bufferSegmentSize,
						 PRUint32 bufferMaxSize,
						 nsITransport** aResult)
{
    nsresult rv = NS_OK;
    
    nsCOMPtr<nsIIOService> ios(do_GetService(kIOServiceCID, &rv));
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
                             aProxyInfo, bufferSegmentSize, bufferMaxSize);
    	if (NS_FAILED(rv)) {
	        delete transport;
	        transport = nsnull;
	    }
    } else {
	    rv = NS_ERROR_OUT_OF_MEMORY;
    }
    
    // Set the reference count to one...
    if (NS_SUCCEEDED(rv))
    	NS_ADDREF(transport);
 
    *aResult = transport;
    
    return rv;
}

NS_IMETHODIMP
nsSocketTransportService::ReuseTransport(nsITransport* i_Transport, 
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
nsSocketTransportService::Wakeup (nsITransport* i_Transport)
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
  int i;
  
  LOG(("nsSocketTransportService::Shutdown BEGIN"));

  if (mThread) {
    // Cancel all remaining transports.
    for (i=0; i<mSelectFDSetCount; i++) {
      if (mActiveTransportList[i])
          mActiveTransportList[i]->Cancel(NS_BINDING_ABORTED);
    }

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
    
    for (i=0; i<mSelectFDSetCount; i++)
      NS_IF_RELEASE(mActiveTransportList[i]);

    // clear the hostname database (NOTE: this runs when the browser
    // enters the offline state).
    PL_DHashTableFinish(&mHostDB);
  } else {
    NS_NOTREACHED("Should have thread when shutting down.");
    rv = NS_ERROR_FAILURE;
  }


  LOG(("nsSocketTransportService::Shutdown END"));

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

    PRUint32    inUseTransports = (PRUint32)mSelectFDSetCount;
    // The first item in the FDSet is *always* the pollable event, if we are using them
    if (mThreadEvent && inUseTransports > 0)
        --inUseTransports;
    
    *o_TransCount = inUseTransports;
    return NS_OK;
}

nsresult
nsSocketTransportService::GetNeckoStringByName (const char *aName, PRUnichar **aString)
{
	nsresult res;
	nsAutoString	resultString; resultString.AssignWithConversion(aName);

    if (!mStringBundle) {
        const char propertyURL[] = NECKO_MSGS_URL;
        // make sure that we get this service on the UI thread.
        NS_WITH_PROXIED_SERVICE(nsIStringBundleService, sBundleService, kStringBundleServiceCID, 
                                NS_UI_THREAD_EVENTQ, &res);
        if (NS_SUCCEEDED (res) && (nsnull != sBundleService)) {
            res = sBundleService->CreateBundle(propertyURL, getter_AddRefs(mStringBundle));
        }
    }

    if (mStringBundle)
	{
		nsAutoString unicodeName; unicodeName.AssignWithConversion(aName);

		PRUnichar *ptrv = nsnull;
		res = mStringBundle->GetStringFromName(unicodeName.get(), &ptrv);

		if (NS_FAILED(res)) 
		{
			resultString.Assign(NS_LITERAL_STRING("[StringName"));
			resultString.AppendWithConversion(aName);
			resultString.Append(NS_LITERAL_STRING("?]"));
			*aString = ToNewUnicode(resultString);
		}
		else
		{
			*aString = ptrv;
		}
	}
	else
	{
		res = NS_OK;
		*aString = ToNewUnicode(resultString);
	}

	return res;
}

//-----------------------------------------------------------------------------
// hostname database impl
//-----------------------------------------------------------------------------

PLDHashTableOps nsSocketTransportService::ops =
{
    PL_DHashAllocTable,
    PL_DHashFreeTable,
    PL_DHashGetKeyStub,
    PL_DHashStringKey,
    nsSocketTransportService::MatchEntry,
    PL_DHashMoveEntryStub,
    nsSocketTransportService::ClearEntry,
    PL_DHashFinalizeStub,
    nsnull
};

PRBool PR_CALLBACK
nsSocketTransportService::MatchEntry(PLDHashTable *table,
                                     const PLDHashEntryHdr *entry,
                                     const void *key)
{
    const nsSocketTransportService::nsHostEntry *he =
        NS_REINTERPRET_CAST(const nsSocketTransportService::nsHostEntry *, entry);

    return !strcmp(he->hostport(), (const char *) key);
}

void PR_CALLBACK
nsSocketTransportService::ClearEntry(PLDHashTable *table,
                                     PLDHashEntryHdr *entry)
{
    nsSocketTransportService::nsHostEntry *he =
        NS_REINTERPRET_CAST(nsSocketTransportService::nsHostEntry *, entry);

    PL_strfree((char *) he->key);
    he->key = nsnull;
    memset(&he->addr, 0, sizeof(he->addr));
}

PRBool
nsSocketTransportService::LookupHost(const char *host, PRInt32 port, PRIPv6Addr *addr)
{
    NS_ASSERTION(host, "null host");
    NS_ASSERTION(addr, "null addr");

    PLDHashEntryHdr *hdr;

    nsCAutoString hostport(nsDependentCString(host) + nsPrintfCString(":%d", port));

    hdr = PL_DHashTableOperate(&mHostDB, hostport.get(), PL_DHASH_LOOKUP);
    if (PL_DHASH_ENTRY_IS_BUSY(hdr)) {
        // found match
        nsHostEntry *ent = NS_REINTERPRET_CAST(nsHostEntry *, hdr);
        memcpy(addr, &ent->addr, sizeof(ent->addr));
        return PR_TRUE;
    }

    return PR_FALSE;
}

void
nsSocketTransportService::OnTransportConnected(const char *host, PRInt32 port, PRNetAddr *addr)
{
    // remember hostname

    PLDHashEntryHdr *hdr;

    nsCAutoString hostport(nsDependentCString(host) + nsPrintfCString(":%d", port));

    hdr = PL_DHashTableOperate(&mHostDB, hostport.get(), PL_DHASH_ADD);
    if (!hdr)
        return;

    NS_ASSERTION(PL_DHASH_ENTRY_IS_BUSY(hdr), "entry not busy");

    nsHostEntry *ent = NS_REINTERPRET_CAST(nsHostEntry *, hdr);
    if (ent->key == nsnull) {
        ent->key = (const void *) ToNewCString(hostport);
        memcpy(&ent->addr, &addr->ipv6.ip, sizeof(ent->addr));
    }
#ifdef DEBUG
    else {
        // verify that the existing entry is in fact a perfect match
        NS_ASSERTION(PL_strcmp(ent->hostport(), hostport.get()) == 0, "bad match");
        NS_ASSERTION(memcmp(&ent->addr, &addr->ipv6.ip, sizeof(ent->addr)) == 0, "bad match");
    }
#endif
}


NS_IMETHODIMP
nsSocketTransportService::GetAutodialEnabled(PRBool *enabled)
{
    *enabled = mAutodialEnabled;
    return NS_OK;
}

NS_IMETHODIMP
nsSocketTransportService::SetAutodialEnabled(PRBool enabled)
{
    mAutodialEnabled = enabled;
    return NS_OK;
}
