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
#include "nsIFocusController.h"
#include "nsIControllers.h"
#include "nsIDOMDocument.h"
#include "nsIDOMXULDocument.h"
#include "nsIDOMHTMLDocument.h"
#include "nsIDOMElement.h"
#include "nsIDOMNSHTMLInputElement.h"
#include "nsIDOMNSHTMLTextAreaElement.h"
#include "nsIDOMUIEvent.h"
#include "nsIDOMWindowInternal.h"
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
#include "nsContentUtils.h"

#ifdef PR_LOGGING
static PRLogModuleInfo* gLog;
#endif

////////////////////////////////////////////////////////////////////////

nsXULCommandDispatcher::nsXULCommandDispatcher(nsIDocument* aDocument)
    : mDocument(aDocument), mFocusController(nsnull), mUpdaters(nsnull)
{
	NS_INIT_REFCNT();

#ifdef PR_LOGGING
  if (! gLog)
    gLog = PR_NewLogModule("nsXULCommandDispatcher");
#endif
}

nsXULCommandDispatcher::~nsXULCommandDispatcher()
{
  while (mUpdaters) {
    Updater* doomed = mUpdaters;
    mUpdaters = mUpdaters->mNext;
    delete doomed;
  }
}

// XPConnect interface list for nsXULCommandDispatcher
NS_CLASSINFO_MAP_BEGIN(XULCommandDispatcher)
    NS_CLASSINFO_MAP_ENTRY(nsIDOMXULCommandDispatcher)
NS_CLASSINFO_MAP_END


// QueryInterface implementation for nsXULCommandDispatcher
NS_INTERFACE_MAP_BEGIN(nsXULCommandDispatcher)
    NS_INTERFACE_MAP_ENTRY(nsIDOMXULCommandDispatcher)
    NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
    NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMXULCommandDispatcher)
    NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(XULCommandDispatcher)
NS_INTERFACE_MAP_END


NS_IMPL_ADDREF(nsXULCommandDispatcher)
NS_IMPL_RELEASE(nsXULCommandDispatcher)


NS_IMETHODIMP
nsXULCommandDispatcher::Create(nsIDocument* aDoc, nsIDOMXULCommandDispatcher** aResult)
{
  nsXULCommandDispatcher* dispatcher = new nsXULCommandDispatcher(aDoc);
  if (!dispatcher)
    return NS_ERROR_OUT_OF_MEMORY;

  *aResult = dispatcher;
  NS_ADDREF(*aResult);
  return NS_OK;
}

void
nsXULCommandDispatcher::EnsureFocusController()
{
  if (!mFocusController) {
    nsCOMPtr<nsIScriptGlobalObject> global;
    mDocument->GetScriptGlobalObject(getter_AddRefs(global));
    nsCOMPtr<nsPIDOMWindow> win(do_QueryInterface(global));
  
    // An inelegant way to retrieve this to be sure, but we are
    // guaranteed that the focus controller outlives us, so it
    // is safe to hold on to it (since we can't die until it has
    // died).
    nsCOMPtr<nsIFocusController> focus;
    win->GetRootFocusController(getter_AddRefs(focus));
    mFocusController = focus; // Store as a weak ptr.
  }
}

////////////////////////////////////////////////////////////////
// nsIDOMXULTracker Interface

NS_IMETHODIMP
nsXULCommandDispatcher::GetFocusedElement(nsIDOMElement** aElement)
{
  EnsureFocusController();
  return mFocusController->GetFocusedElement(aElement);
}

NS_IMETHODIMP
nsXULCommandDispatcher::GetFocusedWindow(nsIDOMWindow** aWindow)
{
  EnsureFocusController();

  nsCOMPtr<nsIDOMWindowInternal> window;
  nsresult rv = mFocusController->GetFocusedWindow(getter_AddRefs(window));
  NS_ENSURE_TRUE(NS_SUCCEEDED(rv) && window, rv);

  return window->QueryInterface(NS_GET_IID(nsIDOMWindow), (void **)aWindow);
}

NS_IMETHODIMP
nsXULCommandDispatcher::SetFocusedElement(nsIDOMElement* aElement)
{
  EnsureFocusController();
  return mFocusController->SetFocusedElement(aElement);
}

NS_IMETHODIMP
nsXULCommandDispatcher::SetFocusedWindow(nsIDOMWindow* aWindow)
{
  EnsureFocusController();
  if (mFocusController) {
    nsCOMPtr<nsIDOMWindowInternal> window(do_QueryInterface(aWindow));

    return mFocusController->SetFocusedWindow(window);
  }

  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsXULCommandDispatcher::AdvanceFocus()
{
  EnsureFocusController();
  if (mFocusController)
    return mFocusController->MoveFocus(PR_TRUE, nsnull);
  return NS_OK;
}

NS_IMETHODIMP
nsXULCommandDispatcher::RewindFocus()
{
  EnsureFocusController();
  if (mFocusController)
    return mFocusController->MoveFocus(PR_FALSE, nsnull);
  return NS_OK;
}

NS_IMETHODIMP
nsXULCommandDispatcher::AdvanceFocusIntoSubtree(nsIDOMElement* aElt)
{
  EnsureFocusController();
  if (mFocusController)
    return mFocusController->MoveFocus(PR_TRUE, aElt);
  return NS_OK;
}

NS_IMETHODIMP
nsXULCommandDispatcher::AddCommandUpdater(nsIDOMElement* aElement,
                                          const nsAReadableString& aEvents,
                                          const nsAReadableString& aTargets)
{
  NS_PRECONDITION(aElement != nsnull, "null ptr");
  if (! aElement)
    return NS_ERROR_NULL_POINTER;

  Updater* updater = mUpdaters;
  Updater** link = &mUpdaters;

  while (updater) {
    if (updater->mElement == aElement) {

#ifdef NS_DEBUG
      nsCAutoString eventsC, targetsC, aeventsC, atargetsC; 
      eventsC.AssignWithConversion(updater->mEvents);
      targetsC.AssignWithConversion(updater->mTargets);
      aeventsC.Assign(NS_ConvertUCS2toUTF8(aEvents));
      atargetsC.Assign(NS_ConvertUCS2toUTF8(aTargets));
      PR_LOG(gLog, PR_LOG_ALWAYS,
             ("xulcmd[%p] replace %p(events=%s targets=%s) with (events=%s targets=%s)",
              this, aElement,
              (const char*) eventsC,
              (const char*) targetsC,
              (const char*) aeventsC,
              (const char*) atargetsC));
#endif

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
#ifdef NS_DEBUG
  nsCAutoString aeventsC, atargetsC; 
  aeventsC.Assign(NS_ConvertUCS2toUTF8(aEvents));
  atargetsC.Assign(NS_ConvertUCS2toUTF8(aTargets));

  PR_LOG(gLog, PR_LOG_ALWAYS,
         ("xulcmd[%p] add     %p(events=%s targets=%s)",
          this, aElement,
          (const char*) aeventsC,
          (const char*) atargetsC));
#endif

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
#ifdef NS_DEBUG
      nsCAutoString eventsC, targetsC; 
      eventsC.AssignWithConversion(updater->mEvents);
      targetsC.AssignWithConversion(updater->mTargets);
      PR_LOG(gLog, PR_LOG_ALWAYS,
             ("xulcmd[%p] remove  %p(events=%s targets=%s)",
              this, aElement,
              (const char*) eventsC,
              (const char*) targetsC));
#endif

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
nsXULCommandDispatcher::UpdateCommands(const nsAReadableString& aEventName)
{
  nsresult rv;

  EnsureFocusController();

  nsAutoString id;
  nsCOMPtr<nsIDOMElement> element;
  mFocusController->GetFocusedElement(getter_AddRefs(element));
  if (element) {
    rv = element->GetAttribute(NS_LITERAL_STRING("id"), id);
    NS_ASSERTION(NS_SUCCEEDED(rv), "unable to get element's id");
    if (NS_FAILED(rv)) return rv;
  }

#if 0
  {
    char*   actionString = aEventName.ToNewCString();
    printf("Doing UpdateCommands(\"%s\")\n", actionString);
    free(actionString);    
  }
#endif
  
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

#ifdef NS_DEBUG
    nsCAutoString aeventnameC; 
    aeventnameC.Assign(NS_ConvertUCS2toUTF8(aEventName));
    PR_LOG(gLog, PR_LOG_ALWAYS,
           ("xulcmd[%p] update %p event=%s",
            this, updater->mElement,
            (const char*) aeventnameC));
#endif

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

PRBool
nsXULCommandDispatcher::Matches(const nsString& aList, 
                                const nsAReadableString& aElement)
{
  if (aList.Equals(NS_LITERAL_STRING("*")))
    return PR_TRUE; // match _everything_!

  PRInt32 indx = aList.Find((const PRUnichar *)PromiseFlatString(aElement).get());
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

NS_IMETHODIMP
nsXULCommandDispatcher::GetControllers(nsIControllers** aResult)
{
  EnsureFocusController();
  return mFocusController->GetControllers(aResult);
}

NS_IMETHODIMP
nsXULCommandDispatcher::GetControllerForCommand(const nsAReadableString& aCommand, nsIController** _retval)
{
  EnsureFocusController();
  return mFocusController->GetControllerForCommand(aCommand, _retval);
}

