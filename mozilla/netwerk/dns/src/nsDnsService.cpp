/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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

#include "nsDnsService.h"
#include "nsIDNSListener.h"
#include "nsIRequest.h"
#include "nsError.h"
#include "prnetdb.h"
#include "nsString.h"
#include "nsIServiceManager.h"
#include "netCore.h"
#include "nsAutoLock.h"
#include "nsIRequestObserver.h"
#include "nsTime.h"
#ifdef DNS_TIMING
#include "prinrval.h"
#include "prtime.h"
#endif
#include "prsystem.h"


/******************************************************************************
 *  Platform specific defines and includes
 *****************************************************************************/

/**
 *  XP_MAC
 */
#if defined(XP_MAC)
#include "pprthred.h"

#if TARGET_CARBON
#define INIT_OPEN_TRANSPORT()	InitOpenTransportInContext(kInitOTForExtensionMask, &mClientContext)
#define OT_OPEN_INTERNET_SERVICES(config, flags, err)	OTOpenInternetServicesInContext(config, flags, err, mClientContext)
#define OT_OPEN_ENDPOINT(config, flags, info, err)		OTOpenEndpointInContext(config, flags, info, err, mClientContext)
#define CLOSE_OPEN_TRANSPORT() do { if (mClientContext) CloseOpenTransportInContext(mClientContext); } while (0)
#else
#define INIT_OPEN_TRANSPORT()	InitOpenTransport()
#define OT_OPEN_INTERNET_SERVICES(config, flags, err)	OTOpenInternetServices(config, flags, err)
#define OT_OPEN_ENDPOINT(config, flags, info, err)		OTOpenEndpoint(config, flags, info, err)
#define CLOSE_OPEN_TRANSPORT() CloseOpenTransport()
#endif /* TARGET_CARBON */

#endif /* XP_MAC */

#if defined(XP_BEOS) && defined(BONE_VERSION)
#include <arpa/inet.h>
#endif

/**
 *  XP_UNIX
 */


/**
 *  XP_WIN
 */
#if defined(XP_WIN)
#define WM_DNS_SHUTDOWN         (WM_USER + 200)
#endif /* XP_WIN */


/******************************************************************************
 *  nsDNSRequest
 *****************************************************************************/
#ifdef XP_MAC
#pragma mark CLASSES
#pragma mark nsDNSRequest
#endif

class nsDNSRequest
            : public PRCList
            , public nsIRequest
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIREQUEST

    nsDNSRequest(nsDNSLookup *     lookup,
                 nsIDNSListener *  userListener,
                 nsISupports *     userContext)
        : mUserListener(userListener)
        , mUserContext(userContext)
        , mLookup(lookup)
        , mStatus(NS_OK)
#ifdef DNS_TIMING
        , mStartTime(PR_IntervalNow())
#endif
    {
        NS_INIT_REFCNT(); 
        PR_INIT_CLIST(this);
    }

    virtual ~nsDNSRequest()
    {
        if (!PR_CLIST_IS_EMPTY(this)) {
            nsDNSService::LockDNSService();
            PR_REMOVE_AND_INIT_LINK(this);
            nsDNSService::UnlockDNSService();
        }
    }

    nsresult FireStart();
    nsresult FireStop(nsresult status);

protected:
    nsCOMPtr<nsIDNSListener>    mUserListener;
    nsCOMPtr<nsISupports>       mUserContext;
    nsDNSLookup*                mLookup;        // weak ref
    nsresult                    mStatus;
#ifdef DNS_TIMING
    PRIntervalTime              mStartTime;
#endif
};


/******************************************************************************
 *  nsDNSLookup
 *****************************************************************************/

#ifdef XP_MAC
#pragma mark nsDNSLookup
#endif
class nsDNSLookup
            : public PRCList
            , public nsISupports
{
public:
    NS_DECL_ISUPPORTS

    static
    nsDNSLookup *       Create(const char * hostName);

    virtual             ~nsDNSLookup();    

    const char *        HostName()   { return mHostName; }
    nsHostEnt *         HostEntry()  { return &mHostEntry; }

    PRBool              IsComplete() { return mState == LOOKUP_COMPLETE; }
    PRBool              IsNotCacheable() { return !mCacheable; }
    PRBool              IsExpired();
    
    nsresult            Status()     { return mStatus; }
    void                SetStatus( nsresult  status) { mStatus = status; }
    
    void                Reset();
    PRBool              HostnameIsIPAddress();
    nsresult            InitiateLookup();
    void                MarkComplete( nsresult  status);

    nsresult            EnqueueRequest( nsDNSRequest *  request);
    void                ProcessRequests();

private:
	friend class nsDNSService;
    nsDNSLookup();

    PRCList             mRequestQ;    
    char *              mHostName;

    // Result of the DNS Lookup
    nsHostEnt           mHostEntry;
    nsresult            mStatus;
    PRUint32            mState;
    enum {
        LOOKUP_NEW      = 0,
        LOOKUP_PENDING  = 1,
        LOOKUP_COMPLETE = 2
    };
    
	PRBool              mCacheable;
    nsTime              mExpires;    

    // Platform specific portions
public:
#if defined(XP_MAC)
    friend pascal
    void                nsDnsServiceNotifierRoutine(void *       contextPtr,
                                                    OTEventCode  code,
                                                    OTResult     result,
                                                    void *       cookie);
public:
    void               ConvertHostEntry();
    PRBool              mStringToAddrComplete;
private:
    InetHostInfo        mInetHostInfo;

#define GET_LOOKUP_FROM_ELEM(_this) \
    ((nsDNSLookup*)((char*)(_this) - offsetof(nsDNSLookup, mLookupElement)))
#define GET_LOOKUP_FROM_HOSTINFO(_this) \
    ((nsDNSLookup*)((char*)(_this) - offsetof(nsDNSLookup, mInetHostInfo)))
#endif


#if defined(XP_WIN)
    friend static
    LRESULT CALLBACK    nsDNSEventProc(HWND    hwnd,
                                       UINT    uMsg,
                                       WPARAM  wParam,
                                       LPARAM  lParam);

    HANDLE              mLookupHandle;
    PRUint32            mMsgID;
#endif


#if defined(XP_UNIX) || defined(XP_BEOS) || defined(XP_OS2)

    void                DoSyncLookup();
#endif
};


/******************************************************************************
 *  utility routines:
 *****************************************************************************/
#ifdef XP_MAC
#pragma mark -
#pragma mark UTILITY ROUTINES
#endif

/**
 *  BufAlloc
 *
 *  Allocate space from the buffer, aligning it to "align" before doing
 *  the allocation. "align" must be a power of 2.
 *  NOTE: this code was taken from NSPR.
 */
static char *
BufAlloc(PRIntn amount, char **bufp, PRIntn *buflenp, PRIntn align) {
    char *buf = *bufp;
    PRIntn buflen = *buflenp;

    if (align && ((long)buf & (align - 1))) {
        PRIntn skip = align - ((ptrdiff_t)buf & (align - 1));
        if (buflen < skip) {
            return 0;
        }
        buf += skip;
        buflen -= skip;
    }
    if (buflen < amount) {
        return 0;
    }
    *bufp = buf + amount;
    *buflenp = buflen - amount;
    return buf;
}


struct DNSHashTableEntry : PLDHashEntryHdr {
    nsDNSLookup *  mLookup;
};

static const void * PR_CALLBACK
GetKey(PLDHashTable * /*table*/, PLDHashEntryHdr * header)
{
    return (void*) ((DNSHashTableEntry *)header)->mLookup->HostName();
}

static PRBool PR_CALLBACK
MatchEntry(PLDHashTable *              /* table */,
            const PLDHashEntryHdr *       header,
            const void *                  key)
{
    const char * hostName = ((DNSHashTableEntry *) header)->mLookup->HostName();
    return nsCRT::strcmp(hostName, (const char *)key) == 0;
}

static void PR_CALLBACK
MoveEntry(PLDHashTable *           /* table */,
          const PLDHashEntryHdr *     src,
          PLDHashEntryHdr       *     dst)
{
    ((DNSHashTableEntry *)dst)->mLookup = ((DNSHashTableEntry *)src)->mLookup;
}

static void PR_CALLBACK
ClearEntry(PLDHashTable *      /* table */,
           PLDHashEntryHdr *      header)
{
    ((DNSHashTableEntry *)header)->mLookup = nsnull;
}


#ifdef XP_MAC
#pragma mark -
#pragma mark nsDNSRequest
#endif
/******************************************************************************
 *  nsDNSRequest methods:
 *****************************************************************************/

NS_IMPL_THREADSAFE_ISUPPORTS1(nsDNSRequest, nsIRequest);


nsresult
nsDNSRequest::FireStart()
{
    nsresult rv;
    NS_ASSERTION(mUserListener, "calling FireStart more than once");
    if (mUserListener == nsnull)
        return NS_ERROR_FAILURE;
    rv = mUserListener->OnStartLookup(mUserContext, mLookup->HostName());
    return rv;
}


nsresult
nsDNSRequest::FireStop(nsresult  status)
{
    nsresult rv;
    const char *  hostName = nsnull;
    nsHostEnt *   hostEnt  = nsnull;
    mStatus = status;
    NS_ASSERTION(mLookup, "FireStop called with no mLookup.");
    if (mLookup) {
        hostName = mLookup->HostName();
        hostEnt  = mLookup->HostEntry();
    } else if (NS_SUCCEEDED(mStatus)) {
        mStatus = NS_ERROR_FAILURE;  // skip calling OnFound()
    }
    mLookup  = nsnull;
    
    NS_ASSERTION(mUserListener, "calling FireStop more than once");
    if (mUserListener == nsnull)
        return NS_ERROR_FAILURE;

    if (NS_SUCCEEDED(mStatus)) {
        rv = mUserListener->OnFound(mUserContext, hostName, hostEnt);
        NS_ASSERTION(NS_SUCCEEDED(rv), "OnFound failed");
    }
    rv = mUserListener->OnStopLookup(mUserContext, hostName, mStatus);
    NS_ASSERTION(NS_SUCCEEDED(rv), "OnStopLookup failed");

    mUserListener = nsnull;
    mUserContext  = nsnull;
#ifdef DNS_TIMING
    if (nsDNSService::gService->mOut) {
        PRIntervalTime stopTime = PR_IntervalNow();
        double duration = PR_IntervalToMicroseconds(stopTime - mStartTime);
        nsDNSService::gService->mCount++;
        nsDNSService::gService->mTimes += duration;
        nsDNSService::gService->mSquaredTimes += duration * duration;
        fprintf(nsDNSService::gService->mOut, "DNS time #%d: %u us for %s\n", 
                (PRInt32)nsDNSService::gService->mCount,
                (PRInt32)duration, mLookup->HostName());
    }
#endif
    return NS_OK;
}


NS_IMETHODIMP
nsDNSRequest::GetName(PRUnichar **  result)
{
    NS_NOTREACHED("nsDNSRequest::GetName");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsDNSRequest::IsPending(PRBool *  result)
{
    if (mLookup)  *result = !mLookup->IsComplete();
    else          *result = PR_FALSE;  // XXX or error
    return NS_OK;
}

NS_IMETHODIMP
nsDNSRequest::GetStatus(nsresult *  status)
{
    *status = mStatus;
    return NS_OK;
}

NS_IMETHODIMP
nsDNSRequest::Cancel(nsresult  status)
{
    NS_ASSERTION(NS_FAILED(status), "shouldn't cancel with a success code");
    nsresult  rv = NS_OK;
    mStatus = status;
    
    NS_ASSERTION(!PR_CLIST_IS_EMPTY(this), "request is not queue on lookup");
    if (!PR_CLIST_IS_EMPTY(this)) {
        nsDNSService::LockDNSService();
        PR_REMOVE_AND_INIT_LINK(this);
        nsDNSService::UnlockDNSService();
    }

    if (mUserListener)  rv = FireStop(status);
    mLookup = nsnull;
    return rv;
}

NS_IMETHODIMP
nsDNSRequest::Suspend()
{
    NS_NOTYETIMPLEMENTED("Suspend() not supported by nsDNSRequest");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsDNSRequest::Resume()
{
    NS_NOTYETIMPLEMENTED("Resume() not supported by nsDNSRequest");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsDNSRequest::GetLoadGroup(nsILoadGroup **  loadGroup)
{
    *loadGroup = nsnull;
    return NS_OK;
}

NS_IMETHODIMP
nsDNSRequest::SetLoadGroup(nsILoadGroup *  loadGroup)
{
    NS_NOTYETIMPLEMENTED("SetLoadGroup() not supported by nsDNSRequest");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsDNSRequest::GetLoadFlags(nsLoadFlags *  loadFlags)
{
    *loadFlags = nsIRequest::LOAD_NORMAL;
    return NS_OK;
}

NS_IMETHODIMP
nsDNSRequest::SetLoadFlags(nsLoadFlags  loadFlags)
{
    NS_NOTYETIMPLEMENTED("SetLoadFlags() not supported by nsDNSRequest");
    return NS_ERROR_NOT_IMPLEMENTED;
}


/******************************************************************************
 *  nsDNSLookup methods:
 *****************************************************************************/
#ifdef XP_MAC
#pragma mark -
#pragma mark nsDNSLookup
#endif

NS_IMPL_THREADSAFE_ISUPPORTS0(nsDNSLookup);

nsDNSLookup::nsDNSLookup()
    : mHostName(nsnull)
    , mStatus(NS_OK)
    , mState(LOOKUP_NEW)
    , mCacheable(PR_TRUE)
    , mExpires(0)
#if defined(XP_UNIX)
    , mNextLookup(nsnull)
#endif
{
	NS_INIT_REFCNT();
	MOZ_COUNT_CTOR(nsDNSLookup);
	PR_INIT_CLIST(this);
    PR_INIT_CLIST(&mRequestQ);
    Reset();
}



nsDNSLookup *
nsDNSLookup::Create(const char * hostName)
{
    nsDNSLookup *  lookup = new nsDNSLookup;
    if (!lookup)  return nsnull;
    
    lookup->mHostName = nsCRT::strdup(hostName);
    if (lookup->mHostName == nsnull) {
        delete lookup;
        return nsnull;
    }
    NS_ADDREF(lookup);
    return lookup;
}


#define EXPIRATION_INTERVAL  (5*60*1000000)         // 5 min worth of microseconds

void
nsDNSLookup::Reset()
{
    PR_REMOVE_AND_INIT_LINK(this);  // remove us from eviction queue
    
    // Initialize result holders
    mHostEntry.bufLen = PR_NETDB_BUF_SIZE;
    mHostEntry.bufPtr = mHostEntry.buffer;

    mState    = LOOKUP_NEW;
    mStatus   = NS_OK;
    mExpires  = nsTime() + nsInt64(EXPIRATION_INTERVAL);      // now + 5 minutes

#if defined(XP_MAC)
    mStringToAddrComplete = PR_FALSE;
#endif

#if defined(XP_WIN)
    mLookupHandle = nsnull;
    mMsgID        = 0;
#endif
}


nsDNSLookup::~nsDNSLookup()
{
    MOZ_COUNT_DTOR(nsDNSLookup);
    NS_ASSERTION(PR_CLIST_IS_EMPTY(this), "lookup deleted while on list");
    NS_ASSERTION(PR_CLIST_IS_EMPTY(&mRequestQ), "lookup deleted while requests pending");
    if (mHostName)  nsCRT::free(mHostName);
}


#if defined(XP_MAC)
void
nsDNSLookup::ConvertHostEntry()
{
    PRIntn  len, count, i;

    // convert InetHostInfo to PRHostEnt
    
    // expect the worse
    mStatus = NS_ERROR_OUT_OF_MEMORY;
    
    // copy name
    len = nsCRT::strlen(mInetHostInfo.name);
    mHostEntry.hostEnt.h_name = BufAlloc(len + 1, &mHostEntry.bufPtr, &mHostEntry.bufLen, 0);
    NS_ASSERTION(nsnull != mHostEntry.hostEnt.h_name,"nsHostEnt buffer full.");
    if (mHostEntry.hostEnt.h_name == nsnull)  return;
    nsCRT::memcpy(mHostEntry.hostEnt.h_name, mInetHostInfo.name, len + 1);
    
    // set aliases to nsnull
    mHostEntry.hostEnt.h_aliases = (char **)BufAlloc(sizeof(char *), &mHostEntry.bufPtr, &mHostEntry.bufLen, sizeof(char *));
    NS_ASSERTION(nsnull != mHostEntry.hostEnt.h_aliases,"nsHostEnt buffer full.");
    if (mHostEntry.hostEnt.h_aliases == nsnull)  return;
    *mHostEntry.hostEnt.h_aliases = nsnull;
    
    // fill in address type
    mHostEntry.hostEnt.h_addrtype = PR_AF_INET6;
    
    // set address length
    mHostEntry.hostEnt.h_length = sizeof(PRIPv6Addr);
    
    // count addresses and allocate storage for the pointers
    for (count = 0; count < kMaxHostAddrs && mInetHostInfo.addrs[count] != nsnull; count++){}
    count++; // for terminating null address
    mHostEntry.hostEnt.h_addr_list = (char **)BufAlloc(count * sizeof(char *), &mHostEntry.bufPtr, &mHostEntry.bufLen, sizeof(char *));
    NS_ASSERTION(nsnull != mHostEntry.hostEnt.h_addr_list,"nsHostEnt buffer full.");
    if (mHostEntry.hostEnt.h_addr_list == nsnull)  return;

    // copy addresses one at a time
    len = mHostEntry.hostEnt.h_length;
    for (i = 0; i < kMaxHostAddrs && mInetHostInfo.addrs[i] != nsnull; i++)
    {
        mHostEntry.hostEnt.h_addr_list[i] = BufAlloc(len, &mHostEntry.bufPtr, &mHostEntry.bufLen, len);
        NS_ASSERTION(nsnull != mHostEntry.hostEnt.h_addr_list[i],"nsHostEnt buffer full.");
        if (mHostEntry.hostEnt.h_addr_list[i] == nsnull) {
        	if (i > 0) break;  // if we have at least one address, then don't fail completely
            return;
        }
        PR_ConvertIPv4AddrToIPv6(mInetHostInfo.addrs[i],
                                 (PRIPv6Addr *)mHostEntry.hostEnt.h_addr_list[i]);
    }
    mHostEntry.hostEnt.h_addr_list[i] = nsnull;

    // hey! we made it.
    mStatus  = NS_OK;
    mState   = LOOKUP_COMPLETE;
}
#endif


PRBool
nsDNSLookup::HostnameIsIPAddress()
{    
    // try converting hostname to an IP-Address
    PRNetAddr *netAddr = (PRNetAddr*)nsMemory::Alloc(sizeof(PRNetAddr));    
    if (!netAddr) {
        return PR_FALSE; // couldn't allocate memory
    }

    PRStatus  status = PR_StringToNetAddr(mHostName, netAddr);
    if (PR_SUCCESS != status) {
        nsMemory::Free(netAddr);
        return PR_FALSE;
    }
    
    // slam the IP in and move on.
    mHostEntry.bufLen = PR_NETDB_BUF_SIZE;

    PRUint32 hostNameLen = nsCRT::strlen(mHostName);
    mHostEntry.hostEnt.h_name = (char*)BufAlloc(hostNameLen + 1,
                                                (char**)&mHostEntry.bufPtr,
                                                &mHostEntry.bufLen,
                                                0);
    memcpy(mHostEntry.hostEnt.h_name, mHostName, hostNameLen + 1);

    mHostEntry.hostEnt.h_aliases = (char**)BufAlloc(1 * sizeof(char*),
                                                    (char**)&mHostEntry.bufPtr,
                                                    &mHostEntry.bufLen,
                                                    sizeof(char **));
    mHostEntry.hostEnt.h_aliases[0] = nsnull;

    mHostEntry.hostEnt.h_addrtype = PR_AF_INET6;
    mHostEntry.hostEnt.h_length = sizeof(PRIPv6Addr);
    mHostEntry.hostEnt.h_addr_list = (char**)BufAlloc(2 * sizeof(char*),
                                                      (char**)&mHostEntry.bufPtr,
                                                      &mHostEntry.bufLen,
                                                      sizeof(char **));
    mHostEntry.hostEnt.h_addr_list[0] = (char*)BufAlloc(mHostEntry.hostEnt.h_length,
                                                        (char**)&mHostEntry.bufPtr,
                                                        &mHostEntry.bufLen,
                                                        0);
    if (PR_NetAddrFamily(netAddr) == PR_AF_INET6) {
        memcpy(mHostEntry.hostEnt.h_addr_list[0], &netAddr->ipv6.ip, mHostEntry.hostEnt.h_length);
    } else {
        PR_ConvertIPv4AddrToIPv6(netAddr->inet.ip,
                                 (PRIPv6Addr *)mHostEntry.hostEnt.h_addr_list[0]);
    }
    mHostEntry.hostEnt.h_addr_list[1] = nsnull;

    MarkComplete(NS_OK);
    mCacheable = PR_FALSE;  // no need to cache ip address strings

    nsMemory::Free(netAddr);
    return PR_TRUE;
}


nsresult
nsDNSLookup::InitiateLookup()
{
    NS_ASSERTION(PR_CLIST_IS_EMPTY(this), "lookup being initiated while on queue");
    if (HostnameIsIPAddress())  return NS_OK;
   
#if defined(XP_BEOS) || defined(XP_OS2)

    DoSyncLookup();
    return NS_OK;

#elif defined(XP_MAC)
    
    nsDNSService::gService->EnqueuePendingQ(this);
    OSErr err = OTInetStringToAddress(nsDNSService::gService->mServiceRef, mHostName, &mInetHostInfo);
    if (err == noErr)  return NS_OK;
    
    PR_REMOVE_AND_INIT_LINK(this);
    return NS_ERROR_UNEXPECTED;
    
#elif defined(XP_WIN)

    mMsgID = nsDNSService::gService->AllocMsgID();
    if (mMsgID == 0)  return NS_ERROR_UNEXPECTED;
        
    nsDNSService::gService->EnqueuePendingQ(this);
    mLookupHandle = WSAAsyncGetHostByName(nsDNSService::gService->mDNSWindow, mMsgID,
                                      mHostName, (char *)&mHostEntry.hostEnt, PR_NETDB_BUF_SIZE);
    // check for error conditions
    if (mLookupHandle == nsnull) {
        NS_WARNING("WSAAsyncGetHostByName return null\n");
        PR_REMOVE_AND_INIT_LINK(this);
        return NS_ERROR_UNEXPECTED;  // or call WSAGetLastError() for details;
	    // While we would like to use mLookupHandle to allow for canceling the request in
	    // the future, there is a bug with winsock2 on win95 that causes WSAAsyncGetHostByName
	    // to always return the same value.  We have worked around this problem by using the
	    // msgID to identify which lookup has completed, rather than the handle, however, we
	    // still need to identify when this is a problem (by comparing the return values of
	    // the first two calls made to WSAAsyncGetHostByName), to avoid using the handle on
	    // those systems.  For more info, see bug 23709.
    }
    return NS_OK;
    
#elif defined(XP_UNIX)

    // Add to pending lookup queue
    nsDNSService::gService->EnqueuePendingQ(this);
    return NS_OK;
    
#endif
}


void
nsDNSLookup::MarkComplete(nsresult status)
{
    mStatus = status;
    mState  = LOOKUP_COMPLETE;
}


nsresult
nsDNSLookup::EnqueueRequest(nsDNSRequest * request)
{
    // mDNSServiceLock must be held, this method releases & reaquires it.
    // must guarantee lookup will not be deallocated for duration of method
    nsresult  rv;
    
    nsDNSService::UnlockDNSService();   // can't hold locks during callback
    rv = request->FireStart();
    nsDNSService::LockDNSService();

    if (NS_FAILED(rv)) return rv;

    PR_APPEND_LINK(request, &mRequestQ);

    if (mState == LOOKUP_NEW) {
        // we need to kick off the lookup
        mState = LOOKUP_PENDING;
        rv = InitiateLookup();
        if (NS_FAILED(rv))  MarkComplete(rv);
    }
    return NS_OK;
}


void
nsDNSLookup::ProcessRequests()
{
    // mDNSServiceLock must be held, this method releases & reaquires it.
    // must guarantee lookup will not be deallocated for duration of method
    
    nsDNSRequest * request = (nsDNSRequest *)PR_LIST_HEAD(&mRequestQ);
    while (request != &mRequestQ) {
        PR_REMOVE_AND_INIT_LINK(request);
        NS_ADDREF(request);                 // hold onto request while calling FireStop
        nsDNSService::UnlockDNSService();   // can't hold lock during callback
        nsresult  rv = request->FireStop(mStatus);
        NS_ASSERTION(NS_SUCCEEDED(rv), "request->FireStop() failed.");
        NS_RELEASE(request);
        nsDNSService::LockDNSService();
        request = (nsDNSRequest *)PR_LIST_HEAD(&mRequestQ);
    }
}

	
PRBool
nsDNSLookup::IsExpired()
{
#ifdef xDEBUG
    char buf[256];
    PRExplodedTime et;

    PR_ExplodeTime(mExpires, PR_LocalTimeParameters, &et);
    PR_FormatTimeUSEnglish(buf, sizeof(buf), "%c", &et);
    fprintf(stderr, "\nDNS %s expires %s\n", mHostName, buf);

    PR_ExplodeTime(nsTime(), PR_LocalTimeParameters, &et);
    PR_FormatTimeUSEnglish(buf, sizeof(buf), "%c", &et);
    fprintf(stderr, "now %s ==> %s\n", buf,
            mExpires < nsTime()
            ? "expired" : "valid");
    fflush(stderr);
#endif
     return mExpires < nsTime();
}


#if defined(XP_UNIX) || defined(XP_BEOS) || defined(XP_OS2)

void
nsDNSLookup::DoSyncLookup()
{
    PRStatus status;
    nsresult rv = NS_OK;

#if defined(XP_UNIX) || defined(XP_BEOS)
    status = PR_GetIPNodeByName(mHostName,
                                PR_AF_INET6,
                                PR_AI_DEFAULT,
                                mHostEntry.buffer,
                                PR_NETDB_BUF_SIZE,
                                &(mHostEntry.hostEnt));
#elif defined(XP_OS2)
    status = PR_GetHostByName(mHostName, 
                              mHostEntry.buffer, 
                              PR_NETDB_BUF_SIZE, 
                              &(mHostEntry.hostEnt));
#else
    NS_NOTREACHED("platform requires async interface");
#endif

    if (PR_SUCCESS != status)
        rv = NS_ERROR_UNKNOWN_HOST;

    MarkComplete(rv);
}

#endif /* XP_UNIX || XP_BEOS || XP_OS2*/


/******************************************************************************
 *  nsDNSService
 *****************************************************************************/
#ifdef XP_MAC
#pragma mark -
#pragma mark nsDNSService
#endif

nsDNSService *   nsDNSService::gService = nsnull;
PRBool           nsDNSService::gNeedLateInitialization = PR_FALSE;

PLDHashTableOps nsDNSService::gHashTableOps =
{
    PL_DHashAllocTable,
    PL_DHashFreeTable,
    GetKey,
    PL_DHashStringKey,
    MatchEntry,
    MoveEntry,
    ClearEntry,
    PL_DHashFinalizeStub
};


nsDNSService::nsDNSService()
    : mDNSServiceLock(nsnull)
    , mDNSCondVar(nsnull)
    , mEvictionQCount(0)
    , mMyIPAddress(0)
    , mState(DNS_NOT_INITIALIZED)
#if defined(XP_MAC)
    , mServiceRef(nsnull)
#endif
#ifdef DNS_TIMING
    , mCount(0)
    , mTimes(0)
    , mSquaredTimes(0)
    , mOut(nsnull)
#endif
{
    NS_INIT_REFCNT();
    
    NS_ASSERTION(gService==nsnull,"multiple nsDNSServices allocated!");
    gService    = this;
    
    PR_INIT_CLIST(&mPendingQ);
    PR_INIT_CLIST(&mEvictionQ);

#if defined(XP_MAC)
    gNeedLateInitialization = PR_TRUE;
    
#if TARGET_CARBON
    mClientContext = nsnull;
#endif /* TARGET_CARBON */
#endif /* defined(XP_MAC) */

#if defined(XP_WIN)
    // initialize bit vector for allocating message IDs.
	int i;
	for (i=0; i<4; i++)
		mMsgIDBitVector[i] = 0;
#endif

#ifdef DNS_TIMING
    if (getenv("DNS_TIMING")) {
        mOut = fopen("dns-timing.txt", "a");
        if (mOut) {
            PRTime now = PR_Now();
            PRExplodedTime time;
            PR_ExplodeTime(now, PR_LocalTimeParameters, &time);
            char buf[128];
            PR_FormatTimeUSEnglish(buf, sizeof(buf), "%c", &time);
            fprintf(mOut, "############### DNS starting new run: %s\n", buf);
        }
    }
#endif
}


NS_IMETHODIMP
nsDNSService::Init()
{
    nsresult rv     = NS_OK;
	PRStatus status = PR_SUCCESS;

    NS_ASSERTION(mDNSServiceLock == nsnull, "nsDNSService not shut down");
    if (mDNSServiceLock)  return NS_ERROR_ALREADY_INITIALIZED;

#if defined(XP_MAC)
    OSStatus errOT = INIT_OPEN_TRANSPORT();
    NS_ASSERTION(errOT == kOTNoError, "InitOpenTransport failed.");
    if (errOT != kOTNoError)  return NS_ERROR_UNEXPECTED;

    nsDnsServiceNotifierRoutineUPP	=  NewOTNotifyUPP(nsDnsServiceNotifierRoutine);
    if (nsDnsServiceNotifierRoutineUPP == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;
#endif

    
    PRBool initialized = PL_DHashTableInit(&mHashTable, &gHashTableOps, nsnull,
                                           sizeof(DNSHashTableEntry), 512);
    if (!initialized)  return NS_ERROR_OUT_OF_MEMORY;
        
    

    mDNSServiceLock = PR_NewLock();
    if (mDNSServiceLock == nsnull)  return NS_ERROR_OUT_OF_MEMORY;

#if defined(XP_UNIX) || defined(XP_WIN)
    mDNSCondVar = PR_NewCondVar(mDNSServiceLock);
    if (!mDNSCondVar) {
        rv = NS_ERROR_OUT_OF_MEMORY;
        goto error_exit;
    }
#endif

#if defined(XP_WIN)
	{
    nsAutoLock  dnsLock(mDNSServiceLock);
#endif

#if defined(XP_MAC) || defined(XP_WIN) || defined(XP_UNIX)
    // create DNS thread
    NS_ASSERTION(mThread == nsnull, "nsDNSService not shut down");
    rv = NS_NewThread(getter_AddRefs(mThread), this, 0, PR_JOINABLE_THREAD);
    NS_ASSERTION(NS_SUCCEEDED(rv), "NS_NewThread failed.");
    if (NS_FAILED(rv))  goto error_exit;
#endif

#if defined(XP_WIN)
    // sync with DNS thread to allow it to create the DNS window
    status = PR_WaitCondVar(mDNSCondVar, PR_INTERVAL_NO_TIMEOUT);
    NS_ASSERTION(status == PR_SUCCESS, "PR_WaitCondVar failed.");
#endif

    mState = DNS_RUNNING;
    return NS_OK;

#if defined(XP_WIN)
    }
#endif
error_exit:

#if defined(XP_UNIX) || defined(XP_WIN)
    if (mDNSCondVar)  PR_DestroyCondVar(mDNSCondVar);
    mDNSCondVar = nsnull;
#endif
    
    if (mDNSServiceLock)  PR_DestroyLock(mDNSServiceLock);
    mDNSServiceLock = nsnull;

    return rv;
}


nsresult
nsDNSService::LateInit()
{
#if defined(XP_MAC)
//    create Open Transport Service Provider for DNS Lookups
    OSStatus    errOT;

    if (mServiceRef) {
        NS_ASSERTION(gNeedLateInitialization == PR_TRUE, "dns badly initialized");
        return (gNeedLateInitialization == PR_TRUE) ? NS_OK : NS_ERROR_UNEXPECTED;
    }

    mServiceRef = OT_OPEN_INTERNET_SERVICES(kDefaultInternetServicesPath, NULL, &errOT);
    NS_ASSERTION((mServiceRef != nsnull) && (errOT == kOTNoError), "error opening OT service.");
    if (errOT != kOTNoError) return NS_ERROR_UNEXPECTED;    // no network -- oh well

    // Install notify function for DNR Address To String completion
    errOT = OTInstallNotifier(mServiceRef, nsDnsServiceNotifierRoutineUPP, this);
    NS_ASSERTION(errOT == kOTNoError, "error installing dns notification routine.");

    // Put us into async mode
    if (errOT == kOTNoError) {
        errOT = OTSetAsynchronous(mServiceRef);
        NS_ASSERTION(errOT == kOTNoError, "error setting service to async mode.");
    }
    
    // if either of the two previous calls failed then dealloc service ref and fail
    if (errOT != kOTNoError) {
        OSStatus status = OTCloseProvider((ProviderRef)mServiceRef);
        mServiceRef = nsnull;
        return NS_ERROR_UNEXPECTED;
    }
#endif

    gNeedLateInitialization = PR_FALSE;
    return NS_OK;
}


nsDNSService::~nsDNSService()
{
    nsresult rv = Shutdown();
    NS_ASSERTION(NS_SUCCEEDED(rv), "DNS shutdown failed");
    NS_ASSERTION(mThread == nsnull, "DNS shutdown failed");
    NS_ASSERTION(mDNSServiceLock == nsnull, "DNS shutdown failed");
    NS_ASSERTION(PR_CLIST_IS_EMPTY(&mPendingQ),  "didn't clean up lookups");
    NS_ASSERTION(PR_CLIST_IS_EMPTY(&mEvictionQ), "didn't clean up lookups");

#if defined(XP_UNIX) || defined(XP_WIN)
    NS_ASSERTION(mDNSCondVar == nsnull, "DNS condition variable not destroyed");
#endif

    NS_ASSERTION(gService==this,"multiple nsDNSServices allocated.");
    gService = nsnull;

#ifdef DNS_TIMING
    if (mOut) {
        double mean;
        double stddev;
        NS_MeanAndStdDev(mCount, mTimes, mSquaredTimes, &mean, &stddev);
        fprintf(mOut, "DNS lookup time: %.2f +/- %.2f us (%d lookups)\n",
                mean, stddev, (PRInt32)mCount);
        fclose(mOut);
        mOut = nsnull;
    }
#endif
    CRTFREEIF(mMyIPAddress);
}


NS_IMPL_THREADSAFE_ISUPPORTS2(nsDNSService, nsIDNSService, nsIRunnable);

NS_METHOD
nsDNSService::Create(nsISupports* aOuter, const nsIID& aIID, void* *aResult)
{
    nsresult rv;
    
    if (aOuter != nsnull)
    	return NS_ERROR_NO_AGGREGATION;
    
    nsDNSService* dnsService = new nsDNSService();
    if (dnsService == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(dnsService);
    rv = dnsService->Init();
    if (NS_SUCCEEDED(rv)) {
        rv = dnsService->QueryInterface(aIID, aResult);
    }
    NS_RELEASE(dnsService);
    return rv;
}


void
nsDNSService::LockDNSService()
{
    NS_ASSERTION(gService, "LockDNSService: No DNS Service");
    if (gService && gService->mDNSServiceLock)  PR_Lock(gService->mDNSServiceLock);
}


void
nsDNSService::UnlockDNSService()
{
    if (gService && gService->mDNSServiceLock) {
        PRStatus status = PR_Unlock(gService->mDNSServiceLock);
        NS_ASSERTION(status == PR_SUCCESS, "incorrect unlock of mDNSServiceLock");
    }
}


nsresult
nsDNSService::InitDNSThread()
{
#if defined(XP_WIN)
    WNDCLASS    wc;
    char *      windowClass = "Mozilla:DNSWindowClass";

    // register window class for DNS event receiver window
    memset(&wc, 0, sizeof(wc));
    wc.style            = 0;
    wc.lpfnWndProc      = nsDNSEventProc;
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 0;
    wc.hInstance        = NULL;
    wc.hIcon            = NULL;
    wc.hbrBackground    = (HBRUSH) NULL;
    wc.lpszMenuName     = (LPCSTR) NULL;
    wc.lpszClassName    = windowClass;
    RegisterClass(&wc);

    // create DNS event receiver window
    mDNSWindow = CreateWindow(windowClass, "Mozilla:DNSWindow",
                              0, 0, 0, 10, 10, NULL, NULL, NULL, NULL);

    // sync with Create thread
    nsAutoLock dnsLock(mDNSServiceLock);
    PRStatus  status = PR_NotifyCondVar(mDNSCondVar);
    NS_ASSERTION(status == PR_SUCCESS, "PR_NotifyCondVar failed.");
#endif /* XP_WIN */

    return NS_OK;
}


#if defined(XP_MAC)

NS_IMETHODIMP
nsDNSService::Run()
{
    while (mState != DNS_SHUTTING_DOWN) {
		
		// can't use a PRCondVar because we need to notify from an interrupt
        PR_Mac_WaitForAsyncNotify(PR_INTERVAL_NO_TIMEOUT);
        // check queue for completed DNS lookups
        PR_Lock(mDNSServiceLock);

        nsDNSLookup * lookup;
        while ((lookup = DequeuePendingQ())) {

            if (NS_SUCCEEDED(lookup->Status())) {
                // convert InetHostInfo to nsHostEnt
                lookup->ConvertHostEntry();
            }
            lookup->ProcessRequests();
            AddToEvictionQ(lookup);
        }
        
        PR_Unlock(mDNSServiceLock);
	}

    return NS_OK;
}


#elif defined(XP_UNIX)

NS_IMETHODIMP
nsDNSService::Run()
{
    nsAutoLock  dnsLock(mDNSServiceLock);

    while (mState != DNS_SHUTTING_DOWN) {

        nsDNSLookup * lookup = DequeuePendingQ();
        if (lookup) {
            // Got a request!!
            lookup->DoSyncLookup();
            lookup->ProcessRequests();
            AddToEvictionQ(lookup);
        } else
            // Woken up without a request --> shutdown
            NS_ASSERTION(mState == DNS_SHUTTING_DOWN, "bad DNS shutdown state");
            break;
    }

    return NS_OK;
}

#elif defined(XP_WIN)

NS_IMETHODIMP
nsDNSService::Run()
{
    nsresult rv = InitDNSThread();
    if (NS_FAILED(rv)) return rv;

    MSG msg;
    
    while(GetMessage(&msg, mDNSWindow, 0, 0)) {
        // no TranslateMessage() because we're not expecting input
        DispatchMessage(&msg);
    }

    return rv;
}

#endif

    
/******************************************************************************
 *  nsIDNSService methods
 *****************************************************************************/
NS_IMETHODIMP
nsDNSService::Lookup(const char*     hostName,
                     nsIDNSListener* userListener,
                     nsISupports*    userContext,
                     nsIRequest*     *result)
{
    nsresult rv;
    nsDNSRequest * request = nsnull;
    *result = nsnull;
    
    if (!mDNSServiceLock || (mState != DNS_RUNNING))  return NS_ERROR_OFFLINE;
    else {
        nsAutoLock  dnsLock(mDNSServiceLock);
        
        if (gNeedLateInitialization) {
            rv = LateInit();
            if (NS_FAILED(rv)) return rv;
        }

#if !defined(XP_OS2) && !defined(XP_BEOS)
        if (mThread == nsnull)
            return NS_ERROR_OFFLINE;
#endif

        nsDNSLookup * lookup = FindOrCreateLookup(hostName);
        if (!lookup) return NS_ERROR_OUT_OF_MEMORY;

        request = new nsDNSRequest(lookup, userListener, userContext);
        if (!request)  return NS_ERROR_OUT_OF_MEMORY;
        NS_ADDREF(request); // for caller
 //       NS_ADDREF(request); // XXX debug leak

        NS_ADDREF(lookup);  // keep it around for life of this method.
        rv = lookup->EnqueueRequest(request);    // releases dns lock
        if (NS_FAILED(rv)) {
            NS_RELEASE(lookup);
            return rv;
        }

        if (lookup->IsComplete()) {
            lookup->ProcessRequests();      // releases dns lock
            if (lookup->IsNotCacheable()) {
                // non-cacheable lookups are released here.
                EvictLookup(lookup);
            }
        }
        NS_RELEASE(lookup); // it's either on the pending queue, or we're done with it
    }

    if (NS_SUCCEEDED(rv)) {
        *result = request;
    }
    return rv;
}


nsDNSLookup *
nsDNSService::FindOrCreateLookup(const char* hostName)
{
    // find hostname in hashtable
    PLDHashEntryHdr *  hashEntry;
    nsDNSLookup *      lookup = nsnull;

    hashEntry = PL_DHashTableOperate(&mHashTable, hostName, PL_DHASH_LOOKUP);
    if (PL_DHASH_ENTRY_IS_BUSY(hashEntry)) {
        lookup = ((DNSHashTableEntry *)hashEntry)->mLookup;
        if (lookup->IsComplete() && lookup->IsExpired()) {
            lookup->Reset();
        }
        return lookup;
    }

    // no lookup entry exists for this request 
    lookup = nsDNSLookup::Create(hostName);
    if (lookup == nsnull)  return nsnull;

    // insert in hash table
    hashEntry = PL_DHashTableOperate(&mHashTable, lookup->HostName(), PL_DHASH_ADD);
    if (!hashEntry) {
        NS_RELEASE(lookup);
        return nsnull;
    }
    ((DNSHashTableEntry *)hashEntry)->mLookup = lookup;

    return lookup;
}


void
nsDNSService::EnqueuePendingQ(nsDNSLookup * lookup)
{
    PR_APPEND_LINK(lookup, &mPendingQ);

#if defined(XP_UNIX)
    // Notify the worker thread that a request has been queued.
    PRStatus  status = PR_NotifyCondVar(mDNSCondVar);
    NS_ASSERTION(status == PR_SUCCESS, "PR_NotifyCondVar failed.");
#endif
}


nsDNSLookup *
nsDNSService::DequeuePendingQ()
{
#if defined(XP_UNIX)
    // Wait for notification of a queued request, unless  we're shutting down.
    if (PR_CLIST_IS_EMPTY(&mPendingQ) && (mState != DNS_SHUTTING_DOWN)) {
        PRStatus  status = PR_WaitCondVar(mDNSCondVar, PR_INTERVAL_NO_TIMEOUT);
        NS_ASSERTION(status == PR_SUCCESS, "PR_WaitCondVar failed.");
    }
#endif

    nsDNSLookup * lookup = (nsDNSLookup *)PR_LIST_HEAD(&mPendingQ);

#if defined(XP_MAC)
    // dequeue only completed lookups
    while (lookup != &mPendingQ) {
        if (lookup->mStringToAddrComplete)  break;
        lookup = (nsDNSLookup *)PR_NEXT_LINK(lookup);
    }
#endif

    if (lookup == &mPendingQ)  return nsnull;
    
    PR_REMOVE_AND_INIT_LINK(lookup);
    return lookup;
}


void
nsDNSService::EvictLookup(nsDNSLookup * lookup)
{
    // remove from hashtable
    (void) PL_DHashTableOperate(&mHashTable, lookup->HostName(), PL_DHASH_REMOVE);
    NS_RELEASE(lookup);
}


void
nsDNSService::AddToEvictionQ(nsDNSLookup * lookup)
{
    PR_APPEND_LINK(lookup, &mEvictionQ);
    ++mEvictionQCount;
    
    while (mEvictionQCount > kMaxCachedLookups) {
        // evict oldest lookup
        PRCList * elem = PR_LIST_HEAD(&mEvictionQ);
        if (elem == &mEvictionQ) {
            NS_ASSERTION(mEvictionQCount == 0, "dns eviction count out of sync");
            mEvictionQCount = 0;
            break;
        }
        
        nsDNSLookup * lookup = (nsDNSLookup *)elem;
        PR_REMOVE_AND_INIT_LINK(lookup);
        --mEvictionQCount;
       EvictLookup(lookup);
    }
}


void
nsDNSService::AbortLookups()
{
    nsDNSLookup * lookup;
    
    // walk pending queue, aborting lookups
    lookup = (nsDNSLookup *)PR_LIST_HEAD(&mPendingQ);
    while(lookup != &mPendingQ) {
        PR_REMOVE_AND_INIT_LINK(lookup);
        lookup->MarkComplete(NS_BINDING_ABORTED);
        lookup->ProcessRequests();
        EvictLookup(lookup);
        lookup = (nsDNSLookup *)PR_LIST_HEAD(&mPendingQ);
    }
    
    // walk pending queue, aborting lookups
    lookup = (nsDNSLookup *)PR_LIST_HEAD(&mEvictionQ);
    while(lookup != &mEvictionQ) {
        PR_REMOVE_AND_INIT_LINK(lookup);
        EvictLookup(lookup);
        --mEvictionQCount;
        lookup = (nsDNSLookup *)PR_LIST_HEAD(&mEvictionQ);
    }
    NS_ASSERTION(mEvictionQCount == 0, "bad eviction queue count");
}


NS_IMETHODIMP
nsDNSService::Resolve(const char *i_hostname, char **o_ip)
{
    // Note that this does not check for an IP address already in hostname
    // So beware!
    NS_ENSURE_ARG_POINTER(o_ip);
    *o_ip = 0;
    NS_ENSURE_ARG_POINTER(i_hostname);

    PRHostEnt he;
    char netdbbuf[PR_NETDB_BUF_SIZE];

    if (PR_SUCCESS == PR_GetHostByName(i_hostname, 
                netdbbuf, 
                sizeof(netdbbuf), 
                &he))
    {
        struct in_addr in;
        memcpy(&in.s_addr, he.h_addr, he.h_length);
        char* ip = 0;

        ip = inet_ntoa(in);
        
        if (ip)
        {
            return (*o_ip = nsCRT::strdup(ip)) ? 
                NS_OK: NS_ERROR_OUT_OF_MEMORY;
        }
    }
    return NS_ERROR_FAILURE;
}


NS_IMETHODIMP
nsDNSService::GetMyIPAddress(char * *o_ip)
{
    NS_ENSURE_ARG_POINTER(o_ip);
    static PRBool readOnce = PR_FALSE;
    if (!readOnce || !mMyIPAddress)
    {
        readOnce = PR_TRUE;
        char name[100];
        if (PR_GetSystemInfo(PR_SI_HOSTNAME, 
                    name, 
                    sizeof(name)) == PR_SUCCESS)
        {
            char* hostname = nsCRT::strdup(name);
            if (NS_SUCCEEDED(Resolve(hostname, &mMyIPAddress)))
            {
                CRTFREEIF(hostname);
            }
            else
            {
                CRTFREEIF(hostname);
                return NS_ERROR_FAILURE;
            }
        }
    }
    *o_ip = nsCRT::strdup(mMyIPAddress);
    return *o_ip ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}


// a helper function to convert an IP address to long value. 
// used by nsDNSService::IsInNet
static unsigned long convert_addr(const char* ip)
{
    char *p, *q, *buf = 0;
    int i;
    unsigned char b[4];
    unsigned long addr = 0L;

    p = buf = PL_strdup(ip);
    if (ip && p) 
    {
        for (i=0; p && i<4 ; ++i)
        {
            q = PL_strchr(p, '.');
            if (q) 
                *q = '\0';
            b[i] = atoi(p) & 0xff;
            if (q)
                p = q+1;
        }
        addr = (((unsigned long)b[0] << 24) |
                ((unsigned long)b[1] << 16) |
                ((unsigned long)b[2] << 8) |
                ((unsigned long)b[3]));
        PL_strfree(buf);
    }
    return htonl(addr);
};


NS_IMETHODIMP
nsDNSService::IsInNet(const char * ipaddr, 
                      const char * pattern, 
                      const char * maskstr, 
                      PRBool *     o_Result)
{
    // Note -- should check that ipaddr is truly digits only! 
    // TODO
    NS_ENSURE_ARG_POINTER(o_Result);
    NS_ENSURE_ARG_POINTER(ipaddr && pattern && maskstr);
    *o_Result = PR_FALSE;

    unsigned long host = convert_addr(ipaddr);
    unsigned long pat = convert_addr(pattern);
    unsigned long mask = convert_addr(maskstr);

    *o_Result = ((mask & host) == (mask & pat));
    return NS_OK;
}


NS_IMETHODIMP
nsDNSService::Shutdown()
{
    nsresult rv = NS_OK;

#if !defined(XP_OS2) && !defined(XP_BEOS)
    if (mThread == nsnull) return rv;
#endif

    if (!mDNSServiceLock) // already shutdown or not init'ed as yet. 
        return NS_ERROR_NOT_AVAILABLE; 

    PR_Lock(mDNSServiceLock);
    mState = DNS_SHUTTING_DOWN;
    
#if defined(XP_MAC)

	// let's shutdown Open Transport so outstanding lookups won't complete while we're cleaning them up
    if (mServiceRef) {
        (void) OTCloseProvider((ProviderRef)mServiceRef);
        mServiceRef = nsnull;
        gNeedLateInitialization = PR_TRUE;
    }
    CLOSE_OPEN_TRANSPORT();           // terminate routine should check flag and do this if Shutdown() is bypassed somehow
    DisposeOTNotifyUPP(nsDnsServiceNotifierRoutineUPP);


    PRThread* dnsServiceThread;
    rv = mThread->GetPRThread(&dnsServiceThread);
    if (dnsServiceThread)
        PR_Mac_PostAsyncNotify(dnsServiceThread);

#elif defined(XP_WIN)

    SendMessage(mDNSWindow, WM_DNS_SHUTDOWN, 0, 0);

#endif

#if defined(XP_MAC) || defined(XP_UNIX) || defined(XP_WIN)
    PR_Unlock(mDNSServiceLock);  // so dns thread can aquire it.
    rv = mThread->Join();        // wait for dns thread to quit
    NS_ASSERTION(NS_SUCCEEDED(rv), "dns thread join failed in shutdown.");
#endif

    // clean up outstanding lookups
    //   - wait until DNS thread is gone so we don't collide while calling requests
    PR_Lock(mDNSServiceLock);
    AbortLookups();

    // reset hashtable
    // XXX assert hashtable is empty
    PL_DHashTableFinish(&mHashTable);

    // Have to break the cycle here, otherwise nsDNSService holds onto the thread
    // and the thread holds onto the nsDNSService via its mRunnable
    mThread = nsnull;

#if defined(XP_UNIX) || defined(XP_WIN)
    PR_DestroyCondVar(mDNSCondVar);
    mDNSCondVar = nsnull;
#endif

    PR_Unlock(mDNSServiceLock);
    PR_DestroyLock(mDNSServiceLock);
    mDNSServiceLock = nsnull;
    mState = DNS_SHUTDOWN;
    
    return rv;
}


/******************************************************************************
 *  XP_MAC methods
 *****************************************************************************/
#if defined(XP_MAC)
pascal void
nsDnsServiceNotifierRoutine(void * contextPtr, OTEventCode code, 
                            OTResult result, void * cookie)
{
	OSStatus        errOT;
    nsDNSService *  dnsService = (nsDNSService *)contextPtr;
    nsDNSLookup *   lookup = GET_LOOKUP_FROM_HOSTINFO(cookie);
    PRThread *      thread;
        
    dnsService->mThread->GetPRThread(&thread);
	
    switch (code) {

    	case T_DNRSTRINGTOADDRCOMPLETE:
                // mark lookup complete and wake dns thread
                nsresult rv = (result == kOTNoError) ? NS_OK : NS_ERROR_UNKNOWN_HOST;
                lookup->mStatus = rv;
                lookup->mStringToAddrComplete = PR_TRUE;
                if (thread)  PR_Mac_PostAsyncNotify(thread);
                break;
        
        case kOTProviderWillClose:
                errOT = OTSetSynchronous(dnsService->mServiceRef);
                // fall through to case kOTProviderIsClosed

        case kOTProviderIsClosed:
                 errOT = OTCloseProvider((ProviderRef)dnsService->mServiceRef);
                 dnsService->mServiceRef = nsnull;
                 nsDNSService::gNeedLateInitialization = PR_TRUE;
                 // set flag to iterate outstanding lookups and cancel
                 break;

#if DEBUG
        default: // or else we don't handle the event
	        DebugStr("\punknown OT event in nsDnsServiceNotifier!");
#endif
    }
}
#endif /* XP_MAC */


/******************************************************************************
 *  XP_WIN methods
 *****************************************************************************/
#if defined(XP_WIN)

static LRESULT CALLBACK
nsDNSEventProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT result = nsnull;
    int     error = nsnull;

 	if ((uMsg >= WM_USER) && (uMsg < WM_USER+128)) {
        result = nsDNSService::gService->ProcessLookup(hWnd, uMsg, wParam, lParam);
    }
    else if (uMsg == WM_DNS_SHUTDOWN) {
        // dispose DNS EventHandler Window
        (void) DestroyWindow(nsDNSService::gService->mDNSWindow);
        PostQuitMessage(0);
        result = 0;
    }
    else {
        result = DefWindowProc(hWnd, uMsg, wParam, lParam);
    }

    return result;
}


LRESULT
nsDNSService::ProcessLookup(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    nsAutoLock    dnsLock(mDNSServiceLock);

    // walk pending queue to find lookup for this (HANDLE)wParam
    nsDNSLookup * lookup = (nsDNSLookup *)PR_LIST_HEAD(&mPendingQ);
    while (lookup != &mPendingQ) {
        if (lookup->mMsgID == uMsg) break;
        lookup = (nsDNSLookup *)PR_NEXT_LINK(lookup);
    }
    
    if (lookup == &mPendingQ)  return -1;   // XXX right result?
    
    
    int error = WSAGETASYNCERROR(lParam);
    lookup->MarkComplete(error ? NS_ERROR_UNKNOWN_HOST : NS_OK);
    FreeMsgID(lookup->mMsgID);

    PR_REMOVE_AND_INIT_LINK(lookup);
    lookup->ProcessRequests();
    AddToEvictionQ(lookup);

    return error ? -1 : 0;
}


PRUint32
nsDNSService::AllocMsgID()
{
	PRUint32 i   = 0;
	PRUint32 bit = 0;
	PRUint32 element;

	while (i < 4) {
		if (~mMsgIDBitVector[i] != 0)	// break if any free bits in this element
			break;
		i++;
	}
	if (i >= 4)
		return 0;

	element = ~mMsgIDBitVector[i]; // element contains free bits marked as 1

	if ((element & 0xFFFF) == 0) {	bit |= 16;	element >>= 16;	}
	if ((element & 0x00FF) == 0) {	bit |=  8;	element >>= 8;	}
	if ((element & 0x000F) == 0) {	bit |=  4;	element >>= 4;	}
	if ((element & 0x0003) == 0) {	bit |=  2;	element >>= 2;	}
	if ((element & 0x0001) == 0)	bit |=  1;

	mMsgIDBitVector[i] |= (PRUint32)1 << bit;

	return WM_USER + i * 32 + bit;
}

void
nsDNSService::FreeMsgID(PRUint32 msgID)
{
	PRInt32 bit = msgID - WM_USER;

	NS_ASSERTION((bit >= 0) && (bit < 128),"nsDNSService::FreeMsgID reports invalid msgID.");

	mMsgIDBitVector[bit>>5] &= ~((PRUint32)1 << (bit & 0x1f));
}

#endif
