/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
 * Original Author: David W. Hyatt (hyatt@netscape.com)
 *
 * Contributor(s): 
 */

/*

  This file provides the implementation for the XUL Command Dispatcher.

 */

#include "nsIContent.h"
#include "nsIControllers.h"
#include "nsIDOMDocument.h"
#include "nsIDOMXULDocument.h"
#include "nsIDOMHTMLDocument.h"
#include "nsIDOMElement.h"
#include "nsIDOMNSHTMLInputElement.h"
#include "nsIDOMNSHTMLTextAreaElement.h"
#include "nsIDOMUIEvent.h"
#include "nsIDOMWindow.h"
#include "nsIDOMXULElement.h"
#include "nsIDocument.h"
#include "nsIPresContext.h"
#include "nsIPresShell.h"
#include "nsIScriptGlobalObject.h"
#include "nsPIDOMWindow.h"
#include "nsRDFCID.h"
#include "nsXULCommandDispatcher.h"
#include "prlog.h"
#include "nsIDOMEventTarget.h"

#ifdef PR_LOGGING
static PRLogModuleInfo* gLog;
#endif

////////////////////////////////////////////////////////////////////////

nsXULCommandDispatcher::nsXULCommandDispatcher(void)
    : mScriptObject(nsnull), mSuppressFocus(PR_FALSE), 
	mActive(PR_FALSE), mFocusInitialized(PR_FALSE), mUpdaters(nsnull)
{
	NS_INIT_REFCNT();

#ifdef PR_LOGGING
    if (! gLog)
        gLog = PR_NewLogModule("nsXULCommandDispatcher");
#endif
}

nsXULCommandDispatcher::~nsXULCommandDispatcher(void)
{
  while (mUpdaters) {
    Updater* doomed = mUpdaters;
    mUpdaters = mUpdaters->mNext;
    delete doomed;
  }
}

NS_IMPL_ADDREF(nsXULCommandDispatcher)
NS_IMPL_RELEASE(nsXULCommandDispatcher)

NS_IMETHODIMP
nsXULCommandDispatcher::QueryInterface(REFNSIID iid, void** result)
{
    if (! result)
        return NS_ERROR_NULL_POINTER;

    *result = nsnull;
    if (iid.Equals(NS_GET_IID(nsISupports)) ||
        iid.Equals(NS_GET_IID(nsIDOMXULCommandDispatcher))) {
        *result = NS_STATIC_CAST(nsIDOMXULCommandDispatcher*, this);
    }
    else if (iid.Equals(NS_GET_IID(nsIDOMFocusListener)) ||
             iid.Equals(NS_GET_IID(nsIDOMEventListener))) {
        *result = NS_STATIC_CAST(nsIDOMFocusListener*, this);
    }
    else if (iid.Equals(NS_GET_IID(nsIScriptObjectOwner))) {
        *result = NS_STATIC_CAST(nsIScriptObjectOwner*, this);
    }
    else if (iid.Equals(NS_GET_IID(nsISupportsWeakReference))) {
        *result = NS_STATIC_CAST(nsISupportsWeakReference*, this);
    }
    else {
        return NS_NOINTERFACE;
    }

    NS_ADDREF_THIS();
    return NS_OK;
}


NS_IMETHODIMP
nsXULCommandDispatcher::Create(nsIDOMXULCommandDispatcher** aResult)
{
    nsXULCommandDispatcher* dispatcher = new nsXULCommandDispatcher();
    if (! dispatcher)
        return NS_ERROR_OUT_OF_MEMORY;

    *aResult = dispatcher;
    NS_ADDREF(*aResult);
    return NS_OK;
}


////////////////////////////////////////////////////////////////
// nsIDOMXULTracker Interface

NS_IMETHODIMP
nsXULCommandDispatcher::GetFocusedElement(nsIDOMElement** aElement)
{
  *aElement = mCurrentElement;
  NS_IF_ADDREF(*aElement);
  return NS_OK;
}

NS_IMETHODIMP
nsXULCommandDispatcher::GetFocusedWindow(nsIDOMWindow** aWindow)
{
  *aWindow = mCurrentWindow;
  NS_IF_ADDREF(*aWindow);
  return NS_OK;
}

NS_IMETHODIMP
nsXULCommandDispatcher::SetFocusedElement(nsIDOMElement* aElement)
{
  mCurrentElement = aElement;
  if (mCurrentElement)
    UpdateCommands(NS_ConvertASCIItoUCS2("focus"));
  return NS_OK;
}

NS_IMETHODIMP
nsXULCommandDispatcher::SetFocusedWindow(nsIDOMWindow* aWindow)
{
  mCurrentWindow = aWindow;
  return NS_OK;
}

NS_IMETHODIMP
nsXULCommandDispatcher::AddCommandUpdater(nsIDOMElement* aElement,
                                            const nsString& aEvents,
                                            const nsString& aTargets)
{
    NS_PRECONDITION(aElement != nsnull, "null ptr");
    if (! aElement)
        return NS_ERROR_NULL_POINTER;

    Updater* updater = mUpdaters;
    Updater** link = &mUpdaters;

    while (updater) {
        if (updater->mElement == aElement) {

            nsCAutoString eventsC, targetsC, aeventsC, atargetsC; 
            eventsC.AssignWithConversion(updater->mEvents);
            targetsC.AssignWithConversion(updater->mTargets);
            aeventsC.AssignWithConversion(aEvents);
            atargetsC.AssignWithConversion(aTargets);
            PR_LOG(gLog, PR_LOG_ALWAYS,
                   ("xulcmd[%p] replace %p(events=%s targets=%s) with (events=%s targets=%s)",
                    this, aElement,
                    (const char*) eventsC,
                    (const char*) targetsC,
                    (const char*) aeventsC,
                    (const char*) atargetsC));

            // If the updater was already in the list, then replace
            // (?) the 'events' and 'targets' filters with the new
            // specification.
            updater->mEvents  = aEvents;
            updater->mTargets = aTargets;
            return NS_OK;
        }

        link = &(updater->mNext);
        updater = updater->mNext;
    }
    nsCAutoString aeventsC, atargetsC; 
    aeventsC.AssignWithConversion(aEvents);
    atargetsC.AssignWithConversion(aTargets);

    PR_LOG(gLog, PR_LOG_ALWAYS,
           ("xulcmd[%p] add     %p(events=%s targets=%s)",
            this, aElement,
            (const char*) aeventsC,
            (const char*) atargetsC));

    // If we get here, this is a new updater. Append it to the list.
    updater = new Updater(aElement, aEvents, aTargets);
    if (! updater)
        return NS_ERROR_OUT_OF_MEMORY;

    *link = updater;
    return NS_OK;
}

NS_IMETHODIMP
nsXULCommandDispatcher::RemoveCommandUpdater(nsIDOMElement* aElement)
{
    NS_PRECONDITION(aElement != nsnull, "null ptr");
    if (! aElement)
        return NS_ERROR_NULL_POINTER;

    Updater* updater = mUpdaters;
    Updater** link = &mUpdaters;

    while (updater) {
        if (updater->mElement == aElement) {
            nsCAutoString eventsC, targetsC; 
            eventsC.AssignWithConversion(updater->mEvents);
            targetsC.AssignWithConversion(updater->mTargets);
            PR_LOG(gLog, PR_LOG_ALWAYS,
                   ("xulcmd[%p] remove  %p(events=%s targets=%s)",
                    this, aElement,
                    (const char*) eventsC,
                    (const char*) targetsC));

            *link = updater->mNext;
            delete updater;
            return NS_OK;
        }

        link = &(updater->mNext);
        updater = updater->mNext;
    }

    // Hmm. Not found. Oh well.
    return NS_OK;
}

NS_IMETHODIMP
nsXULCommandDispatcher::UpdateCommands(const nsString& aEventName)
{
    nsresult rv;

    nsAutoString id;
    if (mCurrentElement) {
        rv = mCurrentElement->GetAttribute(NS_ConvertASCIItoUCS2("id"), id);
        NS_ASSERTION(NS_SUCCEEDED(rv), "unable to get element's id");
        if (NS_FAILED(rv)) return rv;
    }
  
    for (Updater* updater = mUpdaters; updater != nsnull; updater = updater->mNext) {
        // Skip any nodes that don't match our 'events' or 'targets'
        // filters.
        if (! Matches(updater->mEvents, aEventName))
            continue;

        if (! Matches(updater->mTargets, id))
            continue;

        nsCOMPtr<nsIContent> content = do_QueryInterface(updater->mElement);
        NS_ASSERTION(content != nsnull, "not an nsIContent");
        if (! content)
            return NS_ERROR_UNEXPECTED;

        nsCOMPtr<nsIDocument> document;
        rv = content->GetDocument(*getter_AddRefs(document));
        NS_ASSERTION(NS_SUCCEEDED(rv), "unable to get document");
        if (NS_FAILED(rv)) return rv;

        NS_ASSERTION(document != nsnull, "element has no document");
        if (! document)
            continue;

        nsCAutoString aeventnameC; 
        aeventnameC.AssignWithConversion(aEventName);
        PR_LOG(gLog, PR_LOG_ALWAYS,
               ("xulcmd[%p] update %p event=%s",
                this, updater->mElement,
                (const char*) aeventnameC));

        PRInt32 count = document->GetNumberOfShells();
        for (PRInt32 i = 0; i < count; i++) {
            nsCOMPtr<nsIPresShell> shell = dont_AddRef(document->GetShellAt(i));
            if (! shell)
                continue;

            // Retrieve the context in which our DOM event will fire.
            nsCOMPtr<nsIPresContext> context;
            rv = shell->GetPresContext(getter_AddRefs(context));
            if (NS_FAILED(rv)) return rv;

            // Handle the DOM event
            nsEventStatus status = nsEventStatus_eIgnore;
            nsEvent event;
            event.eventStructType = NS_EVENT;
            event.message = NS_XUL_COMMAND_UPDATE; 
            content->HandleDOMEvent(context, &event, nsnull, NS_EVENT_FLAG_INIT, &status);
        }
    }
    return NS_OK;
}

NS_IMETHODIMP
nsXULCommandDispatcher::GetControllers(nsIControllers** aResult)
{
  //XXX: we should fix this so there's a generic interface that describes controllers, 
  //     so this code would have no special knowledge of what object might have controllers.
  if (mCurrentElement) {
    nsCOMPtr<nsIDOMXULElement> xulElement = do_QueryInterface(mCurrentElement);
    if (xulElement)
      return xulElement->GetControllers(aResult);

    nsCOMPtr<nsIDOMNSHTMLTextAreaElement> htmlTextArea = do_QueryInterface(mCurrentElement);
    if (htmlTextArea)
      return htmlTextArea->GetControllers(aResult);

    nsCOMPtr<nsIDOMNSHTMLInputElement> htmlInputElement = do_QueryInterface(mCurrentElement);
    if (htmlInputElement)
      return htmlInputElement->GetControllers(aResult);
  }
  else if (mCurrentWindow) {
    nsCOMPtr<nsIDOMWindow> domWindow = do_QueryInterface(mCurrentWindow);
    if (domWindow)
      return domWindow->GetControllers(aResult);
  }

  *aResult = nsnull;
  return NS_OK;
}

/////
// nsIDOMFocusListener
/////

nsresult 
nsXULCommandDispatcher::Focus(nsIDOMEvent* aEvent)
{
  if (mSuppressFocus)
    return NS_OK;

  nsCOMPtr<nsIDOMEventTarget> t;
  aEvent->GetTarget(getter_AddRefs(t));
  
#ifdef DEBUG_hyatt
  printf("%d : Focus occurred on: ", this);
  nsCOMPtr<nsIDOMElement> domDebugElement = do_QueryInterface(t);
  if (domDebugElement) {
    printf("A Focusable DOM Element");
  }
  nsCOMPtr<nsIDOMDocument> domDebugDocument = do_QueryInterface(t);
  if (domDebugDocument) {
    nsCOMPtr<nsIDOMHTMLDocument> htmlDoc = do_QueryInterface(t);
    if (htmlDoc) {
      printf("Window with an HTML doc (happens twice)");
    }
    else printf("Window with a XUL doc (happens twice)");
  }
  printf("\n");
#endif /* DEBUG_hyatt */

  nsCOMPtr<nsIDOMElement> domElement = do_QueryInterface(t);
  if (domElement && (domElement != mCurrentElement)) {
    SetFocusedElement(domElement);

    // Also set focus to our innermost window.
    // XXX Must be done for the Ender case, since ender causes a blur,
    // but we don't hear the subsequent focus to the Ender window.
    nsCOMPtr<nsIDOMDocument> ownerDoc;
    domElement->GetOwnerDocument(getter_AddRefs(ownerDoc));
    nsCOMPtr<nsIDOMWindow> domWindow;
    GetParentWindowFromDocument(ownerDoc, getter_AddRefs(domWindow));
    if (domWindow)
      SetFocusedWindow(domWindow);
  }
  else {
    // We're focusing a window.  We only want to do an update commands
    // if no element is focused.
    nsCOMPtr<nsIDOMWindow> domWindow;
    nsCOMPtr<nsIDOMDocument> domDoc = do_QueryInterface(t);
    if (domDoc) {
      GetParentWindowFromDocument(domDoc, getter_AddRefs(domWindow));
      if (domWindow) {
        SetFocusedWindow(domWindow);
        if (mCurrentElement) {
          // Make sure this element is in our window. If not, we
          // should clear this field.
          nsCOMPtr<nsIDOMDocument> ownerDoc;
          mCurrentElement->GetOwnerDocument(getter_AddRefs(ownerDoc));
          nsCOMPtr<nsIDOMDocument> windowDoc;
          mCurrentWindow->GetDocument(getter_AddRefs(windowDoc));
          if (ownerDoc != windowDoc)
            mCurrentElement = nsnull;
        }

        if (!mCurrentElement)
          UpdateCommands(NS_ConvertASCIItoUCS2("focus"));
      }
    }
  }

  return NS_OK;
}

nsresult 
nsXULCommandDispatcher::Blur(nsIDOMEvent* aEvent)
{
  if (mSuppressFocus)
    return NS_OK;

  nsCOMPtr<nsIDOMEventTarget> t;
  aEvent->GetTarget(getter_AddRefs(t));

#ifdef DEBUG_hyatt
  printf("%d : Blur occurred on: ", this);
  nsCOMPtr<nsIDOMElement> domDebugElement = do_QueryInterface(t);
  if (domDebugElement) {
    printf("A Focusable DOM Element");
  }
  nsCOMPtr<nsIDOMDocument> domDebugDocument = do_QueryInterface(t);
  if (domDebugDocument) {
    nsCOMPtr<nsIDOMHTMLDocument> htmlDoc = do_QueryInterface(t);
    if (htmlDoc) {
      printf("Window with an HTML doc (happens twice)");
    }
    else printf("Window with a XUL doc (happens twice)");
  }
  printf("\n");
#endif /* DEBUG_hyatt */

  nsCOMPtr<nsIDOMElement> domElement = do_QueryInterface(t);
  if (domElement) {
    SetFocusedElement(nsnull);
  }
  
  nsCOMPtr<nsIDOMWindow> domWindow;
  nsCOMPtr<nsIDOMDocument> domDoc = do_QueryInterface(t);
  if (domDoc) {
    GetParentWindowFromDocument(domDoc, getter_AddRefs(domWindow));
    if (domWindow)
      SetFocusedWindow(nsnull);
  }

  return NS_OK;
}

////////////////////////////////////////////////////////////////////////
// nsIScriptObjectOwner interface
NS_IMETHODIMP
nsXULCommandDispatcher::GetScriptObject(nsIScriptContext *aContext, void** aScriptObject)
{
    nsresult res = NS_OK;
    nsIScriptGlobalObject *global = aContext->GetGlobalObject();

    if (nsnull == mScriptObject) {
        res = NS_NewScriptXULCommandDispatcher(aContext, (nsISupports *)(nsIDOMXULCommandDispatcher*)this, global, (void**)&mScriptObject);
    }
    *aScriptObject = mScriptObject;

    NS_RELEASE(global);
    return res;
}


NS_IMETHODIMP
nsXULCommandDispatcher::SetScriptObject(void *aScriptObject)
{
    mScriptObject = aScriptObject;
    return NS_OK;
}


PRBool
nsXULCommandDispatcher::Matches(const nsString& aList, const nsString& aElement)
{
    if (aList.EqualsWithConversion("*"))
        return PR_TRUE; // match _everything_!

    PRInt32 indx = aList.Find(aElement);
    if (indx == -1)
        return PR_FALSE; // not in the list at all

    // okay, now make sure it's not a substring snafu; e.g., 'ur'
    // found inside of 'blur'.
    if (indx > 0) {
        PRUnichar ch = aList[indx - 1];
        if (! nsCRT::IsAsciiSpace(ch) && ch != PRUnichar(','))
            return PR_FALSE;
    }

    if (indx + aElement.Length() < aList.Length()) {
        PRUnichar ch = aList[indx + aElement.Length()];
        if (! nsCRT::IsAsciiSpace(ch) && ch != PRUnichar(','))
            return PR_FALSE;
    }

    return PR_TRUE;
}


nsresult
nsXULCommandDispatcher::GetParentWindowFromDocument(nsIDOMDocument* aDocument, nsIDOMWindow** aWindow)
{
    nsCOMPtr<nsIDocument> objectOwner = do_QueryInterface(aDocument);
    if(!objectOwner) return NS_OK;

    nsCOMPtr<nsIScriptGlobalObject> globalObject;
    objectOwner->GetScriptGlobalObject(getter_AddRefs(globalObject));
    if(!globalObject) return NS_OK;

    nsCOMPtr<nsIDOMWindow> domWindow = do_QueryInterface(globalObject);
    *aWindow = domWindow;
    NS_IF_ADDREF(*aWindow);
    return NS_OK;
}

NS_IMETHODIMP
nsXULCommandDispatcher::GetControllerForCommand(const nsString& command, nsIController** _retval)
{
    *_retval = nsnull;

    nsCOMPtr<nsIControllers> controllers;
    GetControllers(getter_AddRefs(controllers));
    if(controllers) {
      nsCOMPtr<nsIController> controller;
      controllers->GetControllerForCommand(command.GetUnicode(), getter_AddRefs(controller));
      if(controller) {
        *_retval = controller;
        NS_ADDREF(*_retval);
        return NS_OK;
      }
    }
    
    nsCOMPtr<nsPIDOMWindow> currentWindow;
    if (mCurrentElement) {
      // Move up to the window.
      nsCOMPtr<nsIDOMDocument> domDoc;
      mCurrentElement->GetOwnerDocument(getter_AddRefs(domDoc));
      nsCOMPtr<nsIDOMWindow> domWindow;
      GetParentWindowFromDocument(domDoc, getter_AddRefs(domWindow));
      currentWindow = do_QueryInterface(domWindow);
    }
    else if (mCurrentWindow) {
      nsCOMPtr<nsPIDOMWindow> privateWin = do_QueryInterface(mCurrentWindow);
      privateWin->GetPrivateParent(getter_AddRefs(currentWindow));
    }
    else return NS_OK;

    while(currentWindow) {
      nsCOMPtr<nsIDOMWindow> domWindow = do_QueryInterface(currentWindow);
      if(domWindow) {
        nsCOMPtr<nsIControllers> controllers2;
        domWindow->GetControllers(getter_AddRefs(controllers2));
        if(controllers2) {
          nsCOMPtr<nsIController> controller;
          controllers2->GetControllerForCommand(command.GetUnicode(), getter_AddRefs(controller));
          if(controller) {
            *_retval = controller;
            NS_ADDREF(*_retval);
            return NS_OK;
          }
        }
      } 
      nsCOMPtr<nsPIDOMWindow> parentPWindow = currentWindow;
      parentPWindow->GetPrivateParent(getter_AddRefs(currentWindow));
    }
    
    return NS_OK;
}

NS_IMETHODIMP
nsXULCommandDispatcher::GetSuppressFocusScroll(PRBool* aSuppressFocusScroll)
{
  *aSuppressFocusScroll = mSuppressFocusScroll;
  return NS_OK;
}

NS_IMETHODIMP
nsXULCommandDispatcher::SetSuppressFocusScroll(PRBool aSuppressFocusScroll)
{
  mSuppressFocusScroll = aSuppressFocusScroll;
  return NS_OK;
}

NS_IMETHODIMP
nsXULCommandDispatcher::GetSuppressFocus(PRBool* aSuppressFocus)
{
  *aSuppressFocus = mSuppressFocus;
  return NS_OK;
}

NS_IMETHODIMP
nsXULCommandDispatcher::SetSuppressFocus(PRBool aSuppressFocus)
{
  mSuppressFocus = aSuppressFocus;
  return NS_OK;
}

NS_IMETHODIMP
nsXULCommandDispatcher::GetActive(PRBool* aActive)
{
  //if(!mFocusInitialized)
  //  return PR_TRUE;

  *aActive = mActive;
  return NS_OK;
}

NS_IMETHODIMP
nsXULCommandDispatcher::SetActive(PRBool aActive)
{
  if(!mFocusInitialized)
    mFocusInitialized = PR_TRUE;

  mActive = aActive;
  return NS_OK;
}

