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


#include "nsIBlockingNotification.h"
#include "nsIURL.h"
#include "nsNetSupportDialog.h"

#include "nsIEventQueueService.h"
#include "nsIServiceManager.h"
#include "nsXPComFactory.h"

// Class IDs...
static NS_DEFINE_CID(kEventQueueServiceCID,  NS_EVENTQUEUESERVICE_CID);
static NS_DEFINE_CID(kNetSupportDialogCID, NS_NETSUPPORTDIALOG_CID);

// Interface IDs...
static NS_DEFINE_IID(kIEventQueueServiceIID, NS_IEVENTQUEUESERVICE_IID);
static NS_DEFINE_IID(kIBlockingNotificationObserverIID, 
                                             NS_IBLOCKINGNOTIFICATION_OBSERVER_IID);
static NS_DEFINE_IID(kIBlockingNotificationIID, NS_IBLOCKINGNOTIFICATION_IID);


// Forward declarations...
class nsDefaultProtocolHelper;

/*-------------------- Notification Event Class ----------------------------*/

struct NotificationEvent : public PLEvent 
{
  NotificationEvent(nsDefaultProtocolHelper *aSelf,
                    nsIBlockingNotification *aCaller, 
                    nsIURI *aUrl, 
                    PRInt32 aCode, 
                    nsISupports *aExtraInfo);
  ~NotificationEvent();
  PRStatus Fire(nsIEventQueue* aEventQ);

  static void PR_CALLBACK HandlePLEvent(PLEvent* aEvent);
  static void PR_CALLBACK DestroyPLEvent(PLEvent* aEvent);

  nsDefaultProtocolHelper *mSelf;
  nsIBlockingNotification *mCaller;
  nsIURI *mUrl;
  PRInt32 mCode;
  nsISupports *mExtraInfo;
};



/*----------------------------- nsDefaultProtocolHelper ---------------------*/


class nsDefaultProtocolHelper : public nsIBlockingNotificationObserver
{
public:
  nsDefaultProtocolHelper();

  /* nsISupports interface... */
  NS_DECL_ISUPPORTS

  /* nsIBlockingNotificationObserver interface... */
  NS_IMETHOD Notify(nsIBlockingNotification *aCaller,
                    nsIURI *aUrl,
                    PRThread *aThread,
                    PRInt32 aCode,
                    void *aExtraInfo);

  NS_IMETHOD CancelNotify(nsIURI *aUrl);

  /* class methods... */
  nsresult HandleNotification(nsIBlockingNotification *aCaller,
                              nsIURI *aUrl,
                              PRInt32 aCode,
                              void *aExtraInfo);

protected:
  virtual ~nsDefaultProtocolHelper();

private:
  nsIEventQueueService *mEventQService;
};


nsDefaultProtocolHelper::nsDefaultProtocolHelper()
{
  NS_INIT_REFCNT();

  /* 
   * Cache the EventQueue service... 
   *
   * Ignore failure since there is nothing that can be done...
   * Instead, all of the code that uses mEventQService *must*
   * check that it was initialized...
   */
  (void) nsServiceManager::GetService(kEventQueueServiceCID,
                                      kIEventQueueServiceIID,
                                      (nsISupports **)&mEventQService);
}

nsDefaultProtocolHelper::~nsDefaultProtocolHelper()
{
  if (mEventQService) {
    nsServiceManager::ReleaseService(kEventQueueServiceCID, mEventQService);
    mEventQService = nsnull;
  }
}

/* 
 * Implementations of nsISupports interface methods...
 */
NS_IMPL_ADDREF(nsDefaultProtocolHelper);
NS_IMPL_RELEASE(nsDefaultProtocolHelper);
NS_IMPL_QUERY_INTERFACE(nsDefaultProtocolHelper, kIBlockingNotificationObserverIID);



/*
 * Implementations of nsIBlockingNotificationObserver interface methods... 
 */
NS_IMETHODIMP
nsDefaultProtocolHelper::Notify(nsIBlockingNotification *aCaller,
                                nsIURI *aUrl,
                                PRThread *aThread,
                                PRInt32 aCode,
                                void *aExtraInfo)
{
  nsresult rv;
  NotificationEvent *ev;

  /* 
   * Initialize the return code to indicate that the notification was not
   * processed synchronously...
   */
  rv = NS_NOTIFY_BLOCKED;
    
  /*
   * If no thread switch is necessary, then handle the notification
   * immediately...
   */
#if 0 // The mac netlib is on the same thread as the UI thread but you crash if you block right here
  PRThread* currentThread = PR_GetCurrentThread();
  if ( currentThread == aThread) {
    rv = HandleNotification(aCaller, aUrl, aCode, (void *)aExtraInfo);
  }
  else 
#endif 
  {
  
    /*
     * Post a message to the appropriate thread event queue to
     * handle the notification...
     */
    nsIEventQueue *evQ = nsnull;

    /* locate the event queue for the thread... */
    if (mEventQService) {
      mEventQService->GetThreadEventQueue(aThread, &evQ);
    }

    /* Create and dispatch the notification event... */
    if (evQ) {
      ev = new NotificationEvent(this, aCaller, aUrl, aCode, (nsISupports *) aExtraInfo);
      if (ev) {
        PRStatus status;

        /* dispatch the event into the appropriate event queue... */
        status = ev->Fire(evQ);

        if (PR_SUCCESS != status) {
          /* If the event was not dispatched, then clean up... */
          NotificationEvent::DestroyPLEvent(ev);
          rv = NS_ERROR_FAILURE;
        }
      }
      else {
        /* allocation of the Notification event failed... */ 
        rv = NS_ERROR_OUT_OF_MEMORY;
      }

			NS_IF_RELEASE(evQ);
    } else {
      /* No event queue was found! */
      NS_ASSERTION(0, "No Event Queue is available!");
      rv = NS_ERROR_FAILURE;
    }
  }
  return rv;
}


NS_IMETHODIMP
nsDefaultProtocolHelper::CancelNotify(nsIURI *aUrl)
{
  /* XXX: does not support interrupting a notification yet... */
  return NS_ERROR_FAILURE;
}


nsresult nsDefaultProtocolHelper::HandleNotification(nsIBlockingNotification *aCaller,
                                                     nsIURI *aUrl,
                                                     PRInt32 aCode,
                                                     void *aExtraInfo)
{
  /* XXX this definition must match the one in network/protocol/http/mkhttp.c 
     this will go away as netlib cleanup continues */

  typedef struct _NET_AuthClosure {
    char * msg;
    char * user;
    char * pass;
    void * _private;
  } NET_AuthClosure;

  NET_AuthClosure * auth_closure = (NET_AuthClosure *) aExtraInfo;

  nsAutoString aText(auth_closure->msg);
  nsAutoString aUser;
  nsAutoString aPass(auth_closure->pass);

  // create a dialog
	PRBool bResult = PR_FALSE;
	nsString password, user;
 	PRInt32 result;
  nsresult rv;
  
  NS_WITH_SERVICE(nsIPrompt, dialog, kNetSupportDialogCID, &rv);
  
 	if (NS_FAILED(rv))
 		return NS_ERROR_FAILURE;
  if ( dialog ) {
    PRUnichar* usr;
    PRUnichar* pwd;
   	rv = dialog->PromptUsernameAndPassword(aText.GetUnicode(), &usr, &pwd, &result); 
    if (NS_SUCCEEDED(rv)) {
      aUser = usr;
      delete[] usr;
      aPass = pwd;
      delete[] pwd;
    }
  }
  if ( result == 1 )
  	bResult =  NS_NOTIFY_SUCCEEDED;
  else
  	bResult = NS_ERROR_FAILURE;

  auth_closure->user = aUser.ToNewCString();
  auth_closure->pass = aPass.ToNewCString();

  aCaller->Resume(aUrl, (void *) auth_closure);

  return bResult;
  // delete aUser;
  // delete aPass;
  // delete aText;
  // dialog->Release();

  return NS_NOTIFY_BLOCKED;
}



/*-------------------- Notification Event Class ----------------------------*/



NotificationEvent::NotificationEvent(nsDefaultProtocolHelper *aSelf,
                                     nsIBlockingNotification *aCaller,
                                     nsIURI *aUrl,
                                     PRInt32 aCode,
                                     nsISupports *aExtraInfo)
{
  mSelf      = aSelf;
  mCaller    = aCaller;
  mUrl       = aUrl;
  mCode      = aCode;
  mExtraInfo = aExtraInfo;

  NS_IF_ADDREF(mSelf);
  NS_IF_ADDREF(mCaller);
  NS_IF_ADDREF(mUrl);
//  NS_IF_ADDREF(mExtraInfo);
}


NotificationEvent::~NotificationEvent()
{
  NS_IF_RELEASE(mSelf);
  NS_IF_RELEASE(mCaller);
  NS_IF_RELEASE(mUrl);
 // NS_IF_RELEASE(mExtraInfo);
}

void PR_CALLBACK NotificationEvent::HandlePLEvent(PLEvent* aEvent)
{
  NotificationEvent *ev = (NotificationEvent*)aEvent;

  (void)ev->mSelf->HandleNotification(ev->mCaller, ev->mUrl, ev->mCode, 
                                      (void *)ev->mExtraInfo);
}

void PR_CALLBACK NotificationEvent::DestroyPLEvent(PLEvent* aEvent)
{
  NotificationEvent *ev = (NotificationEvent*)aEvent;

  delete ev;
}

PRStatus NotificationEvent::Fire(nsIEventQueue* aEventQ) 
{
  PL_InitEvent(this, nsnull,
               (PLHandleEventProc)  NotificationEvent::HandlePLEvent,
               (PLDestroyEventProc) NotificationEvent::DestroyPLEvent);

  PRStatus status = aEventQ->PostEvent(this);
  return status;
}



//----------------------------------------------------------------------

// Entry point to create nsAppShellService factory instances...
NS_DEF_FACTORY(DefaultProtocolHelper,nsDefaultProtocolHelper)

nsresult NS_NewDefaultProtocolHelperFactory(nsIFactory** aResult)
{
  nsresult rv = NS_OK;
  nsIFactory* inst;
  
  inst = new nsDefaultProtocolHelperFactory;
  if (nsnull == inst) {
    rv = NS_ERROR_OUT_OF_MEMORY;
  }
  else {
    NS_ADDREF(inst);
  }
  *aResult = inst;
  return rv;
}


