/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License.  You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are Copyright (C) 1998
 * Netscape Communications Corporation.  All Rights Reserved.
 */
#include "nscore.h"
#include "nsRepository.h"

#include "nspr.h"
#include "plevent.h"
#include "plstr.h"

#include "nsNetThread.h"

#include "nsIStreamListener.h"
#include "nsIInputStream.h"
#include "nsIURL.h"
#include "nsString.h"

extern "C" {
#include "mkutils.h"
#include "mkgeturl.h"
#include "mktrace.h"
#include "mkstream.h"
#include "cvchunk.h"

#include "fileurl.h"
#include "httpurl.h"
#include "ftpurl.h"
#include "abouturl.h"
#include "gophurl.h"
#include "fileurl.h"
#include "remoturl.h"
#include "netcache.h"

#include "cvactive.h"

void RL_Init();

PUBLIC NET_StreamClass * 
NET_NGLayoutConverter(FO_Present_Types format_out,
                      void *converter_obj,
                      URL_Struct  *URL_s,
                      MWContext   *context);

void net_AddrefContext(MWContext *context);
void net_ReleaseContext(MWContext *context);
}; /* end of extern "C" */

#if defined(XP_PC)
void net_InitAsyncDNS();
#endif /* XP_PC */


PRThread* gNetlibThread = nsnull;

/*
 * Initialize our protocols
 */

extern "C" void NET_ClientProtocolInitialize()
{
    NET_InitFileProtocol();
    NET_InitHTTPProtocol();
#ifdef NU_CACHE
    NET_InitNuCacheProtocol();
#else
    NET_InitMemCacProtocol();
#endif
    NET_InitFTPProtocol();
    NET_InitAboutProtocol();
    NET_InitGopherProtocol();
    NET_InitRemoteProtocol();
}

nsresult NS_InitNetlib(void)
{
    /* Initialize netlib with 32 sockets... */
    NET_InitNetLib(0, 32);

    /* Initialize the file extension -> content-type mappings */
    NET_InitFileFormatTypes(nsnull, nsnull);

    NET_FinishInitNetLib();

    NET_RegisterContentTypeConverter("*", FO_CACHE_AND_NGLAYOUT, NULL,
                                     NET_CacheConverter);
    NET_RegisterContentTypeConverter("*", FO_NGLAYOUT, NULL,
                                     NET_NGLayoutConverter);
    NET_RegisterContentTypeConverter(APPLICATION_HTTP_INDEX, FO_NGLAYOUT,
                                    NULL, NET_HTTPIndexFormatToHTMLConverter);

    NET_RegisterUniversalEncodingConverter("chunked",
                                           NULL,
                                           NET_ChunkedDecoderStream);

    NET_RegisterContentTypeConverter("multipart/x-mixed-replace", FO_NGLAYOUT,
                                     (void *) CVACTIVE_SIGNAL_AT_END_OF_MULTIPART,
                                     CV_MakeMultipleDocumentStream);

    NET_RegisterContentTypeConverter("multipart/mixed", FO_NGLAYOUT,
                                     (void *) CVACTIVE_SIGNAL_AT_END_OF_MULTIPART,
                                     CV_MakeMultipleDocumentStream);

    RL_Init();

#if defined(XP_PC)
    net_InitAsyncDNS();
#endif /* XP_PC */

    return NS_OK;
}


nsNetlibThread::nsNetlibThread()
{
    mIsNetlibThreadRunning = PR_FALSE;
    mNetlibEventQueue = nsnull;
}


nsNetlibThread::~nsNetlibThread()
{
    Stop();
}


nsresult nsNetlibThread::Start(void)
{
    nsresult rv = NS_OK;

#if defined(NETLIB_THREAD)
#if defined(NSPR20) && defined(DEBUG)
    if (NETLIB==NULL) {
        NETLIB = PR_NewLogModule("NETLIB");
    }
#endif
    /*
     * Create the netlib thread and wait for it to initialize Netlib...
     */
    PR_CEnterMonitor(this);

    mThread = PR_CreateThread(PR_USER_THREAD,
                              nsNetlibThread::NetlibThreadMain,
                              this,
                              PR_PRIORITY_NORMAL,
                              PR_GLOBAL_THREAD,
                              PR_JOINABLE_THREAD,
                              4096);

    /*
     * The netlib thread will call PR_Notify on the monitor when Netlib has
     * been initialized...
     */
    PR_CWait(this, PR_INTERVAL_NO_TIMEOUT);
    PR_CExitMonitor(this);
#else
    /*
     * Initialize Netlib...
     */
    NS_InitNetlib();
#endif /* !NETLIB_THREAD */

    return rv;
}


nsresult nsNetlibThread::Stop(void)
{
#if defined(NETLIB_THREAD)
    PR_CEnterMonitor(this);

    if (PR_TRUE == mIsNetlibThreadRunning) {
        mIsNetlibThreadRunning = PR_FALSE;

        PR_CWait(this, PR_INTERVAL_NO_TIMEOUT);
    }
    PR_CExitMonitor(this);

#else
    NET_ShutdownNetLib();
#endif /* !NETLIB_THREAD */

    return NS_OK;
}


void nsNetlibThread::NetlibThreadMain(void *aParam)
{
    nsNetlibThread* me = (nsNetlibThread*)aParam;

    PR_CEnterMonitor(me);

    me->mNetlibEventQueue = PL_CreateEventQueue("Netlib Event Queue", PR_GetCurrentThread());
    if (nsnull == me->mNetlibEventQueue) {
        PR_CNotify(me);
        PR_CExitMonitor(me);

        /* XXX: return error status... */
        return;
    }

    /*
     * Initialize netlib on the netlib thread...
     */
    NS_InitNetlib();
    gNetlibThread = PR_GetCurrentThread();
    
    me->mIsNetlibThreadRunning = PR_TRUE;

    /*
     * Notify the caller thread that Netlib is now initialized...
     */
    PR_CNotify(me);
    PR_CExitMonitor(me);

    /* 
     * Call the platform specific main loop...
     */
    me->NetlibMainLoop();

    /*
     * Netlib is being shutdown... Clean up and exit.
     */
    PR_CEnterMonitor(me);
    me->mIsNetlibThreadRunning = PR_FALSE;

    /* Notify the main thread that the Netlib thread is no longer running...*/
    PR_CNotify(me);
    PR_CExitMonitor(me);

    PL_DestroyEventQueue(me->mNetlibEventQueue);
    me->mNetlibEventQueue = nsnull;

    NET_ShutdownNetLib();
}

/*
 * Platform specific main loop...
 */
#if defined(XP_PC)
void CALLBACK NetlibTimerProc(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime)
{
    (void) NET_PollSockets();
}

void nsNetlibThread::NetlibMainLoop(void)
{
    UINT timerId;

    /*
     * Create a timer to periodically call NET_PollSockets(...)
     */
#if !defined(NO_NETWORK_POLLING)
    timerId = SetTimer(NULL, 0, 10, (TIMERPROC)NetlibTimerProc);
#endif /* NO_NETWORK_POLLING */

    while (PR_TRUE == mIsNetlibThreadRunning) {
      MSG msg;
      BOOL bIsMsg;

      /*
       * Block for network activity...
       *
       * If NET_CallingNetlibAllTheTime(...) is set, then do not block
       * because a non-socket based protocol is being serviced (ie. file)
       */
      if (NET_IsCallNetlibAllTheTimeSet(NULL, NULL)) {
    		NET_ProcessNet(NULL, NET_EVERYTIME_TYPE);
        bIsMsg = PeekMessage(&msg, NULL, 0, 0, PM_REMOVE);
      } else {
        bIsMsg = GetMessage(&msg, NULL, 0, 0);
      }

      if (FALSE != bIsMsg) {
          TranslateMessage(&msg);
          DispatchMessage(&msg);
      }
    }

#if !defined(NO_NETWORK_POLLING)
    KillTimer(NULL, timerId);
#endif /* NO_NETWORK_POLLING */
}

#else
void nsNetlibThread::NetlibMainLoop()
{
    PR_ASSERT(0);
}
#endif /* ! XP_PC */


/*
 * Proxy implementation for the nsIStreamListener interface...
 */

class nsStreamListenerProxy : public nsIStreamListener
{
public:
    nsStreamListenerProxy(nsIStreamListener* aListener);

    NS_DECL_ISUPPORTS

    NS_IMETHOD OnStartBinding(nsIURL* aURL, const char *aContentType);
    NS_IMETHOD OnProgress(nsIURL* aURL, PRInt32 aProgress, PRInt32 aProgressMax);
    NS_IMETHOD OnStatus(nsIURL* aURL, const nsString &aMsg);
    NS_IMETHOD OnStopBinding(nsIURL* aURL, PRInt32 aStatus, const nsString &aMsg);
    NS_IMETHOD GetBindInfo(nsIURL* aURL);
    NS_IMETHOD OnDataAvailable(nsIURL* aURL, nsIInputStream *aIStream, 
                               PRInt32 aLength);

    void     SetStatus(nsresult aStatus);
    nsresult GetStatus();

    nsIStreamListener* mRealListener;

protected:
    virtual ~nsStreamListenerProxy();

private:
    nsresult mStatus;
};


/*-------------------- Base Proxy Class ------------------------------------*/

struct ProxyEvent : public PLEvent 
{
  virtual ~ProxyEvent();
  virtual void InitEvent();
  NS_IMETHOD HandleEvent() = 0;
  void Fire(void);

  static void PR_CALLBACK HandlePLEvent(PLEvent* aEvent);
  static void PR_CALLBACK DestroyPLEvent(PLEvent* aEvent);
};

ProxyEvent::~ProxyEvent()
{
}

void ProxyEvent::InitEvent()
{
  PL_InitEvent(this, nsnull,
               (PLHandleEventProc)  ProxyEvent::HandlePLEvent,
               (PLDestroyEventProc) ProxyEvent::DestroyPLEvent);
}

void PR_CALLBACK ProxyEvent::HandlePLEvent(PLEvent* aEvent)
{
  /*
   * XXX: This is a dangerous cast since it must adjust the pointer 
   *      to compensate for the vtable...
   */
  ProxyEvent *ev = (ProxyEvent*)aEvent;

  ev->HandleEvent();
}

void PR_CALLBACK ProxyEvent::DestroyPLEvent(PLEvent* aEvent)
{
  /*
   * XXX: This is a dangerous cast since it must adjust the pointer 
   *      to compensate for the vtable...
   */
  ProxyEvent *ev = (ProxyEvent*)aEvent;

  delete ev;
}

#if defined(XP_UNIX)
extern PLEventQueue* gWebShell_UnixEventQueue;
#endif

void ProxyEvent::Fire() 
{
  PLEventQueue* eventQueue;
  InitEvent();

#if defined(XP_PC)
  eventQueue = PL_GetMainEventQueue();
#elif defined(XP_UNIX)
  eventQueue = gWebShell_UnixEventQueue;
#endif

  PL_PostEvent(eventQueue, this);
}


/*--------- Base Class for the nsIStreamListenerProxy Class ----------------*/


struct StreamListenerProxyEvent : public ProxyEvent
{
    StreamListenerProxyEvent(nsStreamListenerProxy* aProxy, nsIURL* aURL);
    virtual ~StreamListenerProxyEvent();

    virtual void InitEvent();
    static void PR_CALLBACK HandlePLEvent(PLEvent* aEvent);

    nsStreamListenerProxy* mProxy;
    nsIURL* mURL;
};


StreamListenerProxyEvent::StreamListenerProxyEvent(nsStreamListenerProxy* aProxy, nsIURL* aURL)
{
    mProxy = aProxy;
    mURL   = aURL;

    NS_ADDREF(mProxy);
    NS_ADDREF(mURL);

}

StreamListenerProxyEvent::~StreamListenerProxyEvent()
{
    NS_RELEASE(mProxy);
    NS_RELEASE(mURL);
}

void StreamListenerProxyEvent::InitEvent()
{
  PL_InitEvent(this, nsnull,
               (PLHandleEventProc)  StreamListenerProxyEvent::HandlePLEvent,
               (PLDestroyEventProc) ProxyEvent::DestroyPLEvent);
}

void PR_CALLBACK StreamListenerProxyEvent::HandlePLEvent(PLEvent* aEvent)
{
    /*
     * XXX: This is a dangerous cast since it must adjust the pointer 
     *      to compensate for the vtable...
     */
    nsresult rv;
    StreamListenerProxyEvent *ev = (StreamListenerProxyEvent*)aEvent;

    rv = ev->HandleEvent();
    if (NS_FAILED(rv)) {
        ev->mProxy->SetStatus(rv);
    }
}

/*-------------- OnStartBinding Proxy --------------------------------------*/

struct OnStartBindingProxyEvent : public StreamListenerProxyEvent
{
    OnStartBindingProxyEvent(nsStreamListenerProxy* aProxy, nsIURL* aURL, 
                             const char *aContentType);
    virtual ~OnStartBindingProxyEvent();
    NS_IMETHOD HandleEvent();

    char *mContentType;
};

OnStartBindingProxyEvent::OnStartBindingProxyEvent(nsStreamListenerProxy* aProxy,
                                                   nsIURL* aURL,
                                                   const char *aContentType)
                        : StreamListenerProxyEvent(aProxy, aURL)
{
    mContentType = PL_strdup(aContentType);
}

OnStartBindingProxyEvent::~OnStartBindingProxyEvent()
{
    PR_Free(mContentType);
}

NS_IMETHODIMP
OnStartBindingProxyEvent::HandleEvent()
{
    return mProxy->mRealListener->OnStartBinding(mURL, mContentType);
}


/*-------------- OnProgress Proxy ------------------------------------------*/

struct OnProgressProxyEvent : public StreamListenerProxyEvent
{
    OnProgressProxyEvent(nsStreamListenerProxy* aProxy, nsIURL* aURL, 
                         PRInt32 aProgress, PRInt32 aProgressMax);
    NS_IMETHOD HandleEvent();

    PRInt32 mProgress;
    PRInt32 mProgressMax;
};

OnProgressProxyEvent::OnProgressProxyEvent(nsStreamListenerProxy* aProxy,
                                           nsIURL* aURL,
                                           PRInt32 aProgress,
                                           PRInt32 aProgressMax)
                     : StreamListenerProxyEvent(aProxy, aURL)
{
    mProgress    = aProgress;
    mProgressMax = aProgressMax;
}

NS_IMETHODIMP
OnProgressProxyEvent::HandleEvent()
{
    return mProxy->mRealListener->OnProgress(mURL, mProgress, mProgressMax);
}

/*-------------- OnStatus Proxy --------------------------------------------*/

struct OnStatusProxyEvent : public StreamListenerProxyEvent
{
    OnStatusProxyEvent(nsStreamListenerProxy* aProxy, nsIURL* aURL, 
                       const nsString& aMsg);
    NS_IMETHOD HandleEvent();

    nsString mMsg;
};

OnStatusProxyEvent::OnStatusProxyEvent(nsStreamListenerProxy* aProxy,
                                       nsIURL* aURL,
                                       const nsString& aMsg)
                   : StreamListenerProxyEvent(aProxy, aURL)
{
    mMsg = aMsg;
}

NS_IMETHODIMP
OnStatusProxyEvent::HandleEvent()
{
    return mProxy->mRealListener->OnStatus(mURL, mMsg);
}


/*-------------- OnStopBinding Proxy ---------------------------------------*/

struct OnStopBindingProxyEvent : public StreamListenerProxyEvent
{
    OnStopBindingProxyEvent(nsStreamListenerProxy* aProxy, nsIURL* aURL, 
                            PRInt32 aStatus, const nsString& aMsg);
    NS_IMETHOD HandleEvent();

    PRInt32 mStatus;
    nsString mMsg;
};

OnStopBindingProxyEvent::OnStopBindingProxyEvent(nsStreamListenerProxy* aProxy,
                                                 nsIURL* aURL,
                                                 PRInt32 aStatus,
                                                 const nsString& aMsg)
                       : StreamListenerProxyEvent(aProxy, aURL)
{
    mStatus = aStatus;
    mMsg = aMsg;
}

NS_IMETHODIMP
OnStopBindingProxyEvent::HandleEvent()
{
    return mProxy->mRealListener->OnStopBinding(mURL, mStatus, mMsg);
}


/*-------------- OnDataAvailable Proxy -------------------------------------*/

struct OnDataAvailableProxyEvent : public StreamListenerProxyEvent
{
    OnDataAvailableProxyEvent(nsStreamListenerProxy* aProxy, nsIURL* aURL, 
                              nsIInputStream* aStream, PRInt32 aLength);
    virtual ~OnDataAvailableProxyEvent();
    NS_IMETHOD HandleEvent();

    nsIInputStream* mStream;
    PRInt32 mLength;
};

OnDataAvailableProxyEvent::OnDataAvailableProxyEvent(nsStreamListenerProxy* aProxy,
                                                     nsIURL* aURL,
                                                     nsIInputStream* aStream,
                                                     PRInt32 aLength)
                         : StreamListenerProxyEvent(aProxy, aURL)
{
    mStream = aStream;
    NS_ADDREF(mStream);

    mLength = aLength;
}

OnDataAvailableProxyEvent::~OnDataAvailableProxyEvent()
{
    NS_RELEASE(mStream);
}

NS_IMETHODIMP
OnDataAvailableProxyEvent::HandleEvent()
{
    return mProxy->mRealListener->OnDataAvailable(mURL, mStream, mLength);
}

/*--------------------------------------------------------------------------*/

nsStreamListenerProxy::nsStreamListenerProxy(nsIStreamListener* aListener)
{
    NS_INIT_REFCNT();

    mRealListener = aListener;
    NS_ADDREF(mRealListener);

    mStatus = NS_OK;
}


void nsStreamListenerProxy::SetStatus(nsresult aStatus)
{
    NS_LOCK_INSTANCE();
    mStatus = aStatus;
    NS_UNLOCK_INSTANCE();
}


nsresult nsStreamListenerProxy::GetStatus()
{
    nsresult rv;

    NS_LOCK_INSTANCE();
    rv = mStatus;
    NS_UNLOCK_INSTANCE();

    return rv;
}

/*
 * Implementation of threadsafe ISupports methods...
 */
static NS_DEFINE_IID(kIStreamListenerIID, NS_ISTREAMLISTENER_IID);
NS_IMPL_THREADSAFE_ISUPPORTS(nsStreamListenerProxy, kIStreamListenerIID);

NS_IMETHODIMP 
nsStreamListenerProxy::OnStartBinding(nsIURL* aURL, const char *aContentType)
{
  nsresult rv;

  if (PR_GetCurrentThread() == gNetlibThread) {
    OnStartBindingProxyEvent* ev;

    rv = GetStatus();
    if (NS_SUCCEEDED(rv)) {
      ev = new OnStartBindingProxyEvent(this, aURL, aContentType);
      if (nsnull == ev) {
        rv = NS_ERROR_OUT_OF_MEMORY;
      } else {
        ev->Fire();
      }
    }
  } else {
    rv = mRealListener->OnStartBinding(aURL, aContentType);
  }
  return rv;
}

NS_IMETHODIMP
nsStreamListenerProxy::OnProgress(nsIURL* aURL, PRInt32 aProgress, 
                                  PRInt32 aProgressMax)
{
  nsresult rv;

  if (PR_GetCurrentThread() == gNetlibThread) {
    OnProgressProxyEvent* ev;

    rv = GetStatus();
    if (NS_SUCCEEDED(rv)) {
      ev = new OnProgressProxyEvent(this, aURL, aProgress, aProgressMax);
      if (nsnull == ev) {
        rv = NS_ERROR_OUT_OF_MEMORY;
      } else {
        ev->Fire();
      }
    }
  } else {
    rv = mRealListener->OnProgress(aURL, aProgress, aProgressMax);
  }
  return rv;
}

NS_IMETHODIMP
nsStreamListenerProxy::OnStatus(nsIURL* aURL, const nsString &aMsg)
{
  nsresult rv;

  if (PR_GetCurrentThread() == gNetlibThread) {
    OnStatusProxyEvent* ev;

    rv = GetStatus();
    if (NS_SUCCEEDED(rv)) {
      ev = new OnStatusProxyEvent(this, aURL, aMsg);
      if (nsnull == ev) {
        rv = NS_ERROR_OUT_OF_MEMORY;
      } else {
        ev->Fire();
      }
    }
  } else {
    rv = mRealListener->OnStatus(aURL, aMsg);
  }
  return rv;
}

NS_IMETHODIMP
nsStreamListenerProxy::OnStopBinding(nsIURL* aURL, PRInt32 aStatus, 
                                     const nsString &aMsg)
{
  nsresult rv;

  if (PR_GetCurrentThread() == gNetlibThread) {
    OnStopBindingProxyEvent* ev;

    rv = GetStatus();
    if (NS_SUCCEEDED(rv)) {
      ev = new OnStopBindingProxyEvent(this, aURL, aStatus, aMsg);
      if (nsnull == ev) {
        rv = NS_ERROR_OUT_OF_MEMORY;
      } else {
        ev->Fire();
      }
    }
  } else {
    rv = mRealListener->OnStopBinding(aURL, aStatus, aMsg);
  }
  return rv;
}

/*--------------------------------------------------------------------------*/

NS_IMETHODIMP
nsStreamListenerProxy::GetBindInfo(nsIURL* aURL)
{
  nsresult rv;

  if (PR_GetCurrentThread() == gNetlibThread) {
    PR_ASSERT(0);
    rv = NS_ERROR_FAILURE;
  } else {
    rv = mRealListener->GetBindInfo(aURL);
  }
  return rv;
}

NS_IMETHODIMP
nsStreamListenerProxy::OnDataAvailable(nsIURL* aURL, nsIInputStream *aIStream, 
                                       PRInt32 aLength)
{
  nsresult rv;

  if (PR_GetCurrentThread() == gNetlibThread) {
    OnDataAvailableProxyEvent* ev;

    rv = GetStatus();
    if (NS_SUCCEEDED(rv)) {
      ev = new OnDataAvailableProxyEvent(this, aURL, aIStream, aLength);
      if (nsnull == ev) {
        rv = NS_ERROR_OUT_OF_MEMORY;
      } else {
        ev->Fire();
      }
    }
  } else {
    mRealListener->OnDataAvailable(aURL, aIStream, aLength);
  }
  return rv;
}

/*--------------------------------------------------------------------------*/


nsStreamListenerProxy::~nsStreamListenerProxy()
{
    NS_RELEASE(mRealListener);
}

nsIStreamListener* ns_NewStreamListenerProxy(nsIStreamListener* aListener)
{
    return new nsStreamListenerProxy(aListener);
}


/*----------- Proxy for net_CallExitRoutine(...) ---------------------------*/
extern "C" MODULE_PRIVATE void
net_CallExitRoutine(Net_GetUrlExitFunc* exit_routine,
                    URL_Struct*         URL_s,
                    int                 status,
                    FO_Present_Types    format_out,
                    MWContext*          window_id);


struct CallExitRoutineProxyEvent : public ProxyEvent
{
  CallExitRoutineProxyEvent(Net_GetUrlExitFunc* aExitRoutine,
                            URL_Struct* aURL_s,
                            int aStatus,
                            FO_Present_Types aFormatOut,
                            MWContext* aWindowId);
  virtual ~CallExitRoutineProxyEvent();
  NS_IMETHOD HandleEvent();

  Net_GetUrlExitFunc* exit_routine;
  URL_Struct*         URL_s;
  int                 status;
  FO_Present_Types    format_out;
  MWContext*          window_id;
};

CallExitRoutineProxyEvent::CallExitRoutineProxyEvent(Net_GetUrlExitFunc* aExitRoutine,
                                                     URL_Struct* aURL_s,
                                                     int aStatus,
                                                     FO_Present_Types aFormatOut,
                                                     MWContext* aWindowId)
{
  exit_routine = aExitRoutine;
  URL_s        = aURL_s;
  status       = aStatus;
  format_out   = aFormatOut;
  window_id    = aWindowId;

  net_AddrefContext(window_id);
  NET_HoldURLStruct(URL_s);
}


CallExitRoutineProxyEvent::~CallExitRoutineProxyEvent()
{
  NET_DropURLStruct(URL_s);
  net_ReleaseContext(window_id);
}


NS_IMETHODIMP
CallExitRoutineProxyEvent::HandleEvent()
{
  net_CallExitRoutine(exit_routine, URL_s, status, format_out, window_id);
  return NS_OK;
}


extern "C" MODULE_PRIVATE void
net_CallExitRoutineProxy(Net_GetUrlExitFunc* exit_routine,
                         URL_Struct*         URL_s,
                         int                 status,
                         FO_Present_Types    format_out,
                         MWContext*          window_id)
{

  if (PR_GetCurrentThread() == gNetlibThread) {
    CallExitRoutineProxyEvent* ev;

    ev = new CallExitRoutineProxyEvent(exit_routine, URL_s, status, 
                                       format_out, window_id);
    if (nsnull != ev) {
      ev->Fire();
    }
  } else {
    net_CallExitRoutine(exit_routine, URL_s, status, format_out, window_id);
  }
}