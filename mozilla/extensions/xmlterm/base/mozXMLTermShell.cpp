/*
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "MPL"); you may not use this file
 * except in compliance with the MPL. You may obtain a copy of
 * the MPL at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the MPL is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the MPL for the specific language governing
 * rights and limitations under the MPL.
 * 
 * The Original Code is XMLterm.
 * 
 * The Initial Developer of the Original Code is Ramalingam Saravanan.
 * Portions created by Ramalingam Saravanan <svn@xmlterm.org> are
 * Copyright (C) 1999 Ramalingam Saravanan. All Rights Reserved.
 * 
 * Contributor(s):
 *   Pierre Phaneuf <pp@ludusdesign.com>
 */

// mozXMLTermShell.cpp: implementation of mozIXMLTermShell
// providing an XPCONNECT wrapper to the XMLTerminal interface,
// thus allowing easy (and controlled) access from scripts

#include <stdio.h>

#include "nscore.h"
#include "nsCOMPtr.h"
#include "nsString.h"

#include "nsIDocumentViewer.h"
#include "nsIDocument.h"

#include "nsIWebShell.h"
#include "nsIWebShellWindow.h"
#include "nsIPresShell.h"
#include "nsIPresContext.h"
#include "nsIScriptGlobalObject.h"

#include "nsIServiceManager.h"

#include "nsIAppShellService.h"
#include "nsAppShellCIDs.h"
#include "nsAppCoresCIDs.h"

#include "nsIDOMToolkitCore.h"
#include "nsIDOMDocument.h"
#include "nsIDOMSelection.h"
#include "nsIDOMWindow.h"

#include "mozXMLT.h"
#include "mozXMLTermShell.h"

// Define Class IDs
static NS_DEFINE_IID(kAppShellServiceCID,    NS_APPSHELL_SERVICE_CID);
static NS_DEFINE_CID(kToolkitCoreCID,        NS_TOOLKITCORE_CID);

// Define Interface IDs
static NS_DEFINE_IID(kISupportsIID,          NS_ISUPPORTS_IID);


/////////////////////////////////////////////////////////////////////////
// mozXMLTermShell factory
/////////////////////////////////////////////////////////////////////////

nsresult
NS_NewXMLTermShell(mozIXMLTermShell** aXMLTermShell)
{
    NS_PRECONDITION(aXMLTermShell != nsnull, "null ptr");
    if (! aXMLTermShell)
        return NS_ERROR_NULL_POINTER;

    *aXMLTermShell = new mozXMLTermShell();
    if (! *aXMLTermShell)
        return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(*aXMLTermShell);
    return NS_OK;
}

/////////////////////////////////////////////////////////////////////////
// mozXMLTermShell implementation
/////////////////////////////////////////////////////////////////////////

mozXMLTermShell::mozXMLTermShell() :
  mInitialized(PR_FALSE),
  mContentWindow(nsnull),
  mContentAreaWebShell(nsnull),
  mXMLTerminal(nsnull)
{
  NS_INIT_REFCNT();
}

mozXMLTermShell::~mozXMLTermShell()
{
  if (mInitialized) {
    Finalize();
  }
}


// Implement AddRef and Release
NS_IMPL_ADDREF(mozXMLTermShell)
NS_IMPL_RELEASE(mozXMLTermShell)


NS_IMETHODIMP 
mozXMLTermShell::QueryInterface(REFNSIID aIID,void** aInstancePtr)
{
  if (aInstancePtr == NULL) {
    return NS_ERROR_NULL_POINTER;
  }

  // Always NULL result, in case of failure
  *aInstancePtr = NULL;

  if ( aIID.Equals(kISupportsIID)) {
    *aInstancePtr = NS_STATIC_CAST(nsISupports*,
                                   NS_STATIC_CAST(mozIXMLTermShell*,this));

  } else if ( aIID.Equals(NS_GET_IID(mozIXMLTermShell)) ) {
    *aInstancePtr = NS_STATIC_CAST(mozIXMLTermShell*,this);

  } else {
    return NS_ERROR_NO_INTERFACE;
  }

  NS_ADDREF_THIS();

  return NS_OK;
}


NS_IMETHODIMP mozXMLTermShell::GetCurrentEntryNumber(PRInt32 *aNumber)
{
  if (mXMLTerminal) {
    return mXMLTerminal->GetCurrentEntryNumber(aNumber);
  } else {
    return NS_ERROR_NOT_INITIALIZED;
  }
}


NS_IMETHODIMP mozXMLTermShell::GetHistory(PRInt32 *aHistory)
{
  if (mXMLTerminal) {
    return mXMLTerminal->GetHistory(aHistory);
  } else {
    return NS_ERROR_NOT_INITIALIZED;
  }
}


NS_IMETHODIMP mozXMLTermShell::SetHistory(PRInt32 aHistory)
{
  if (mXMLTerminal) {
    return mXMLTerminal->SetHistory(aHistory);
  } else {
    return NS_ERROR_NOT_INITIALIZED;
  }
}


NS_IMETHODIMP mozXMLTermShell::GetPrompt(PRUnichar **aPrompt)
{
  if (mXMLTerminal) {
    return mXMLTerminal->GetPrompt(aPrompt);
  } else {
    return NS_ERROR_NOT_INITIALIZED;
  }
}


NS_IMETHODIMP mozXMLTermShell::SetPrompt(const PRUnichar* aPrompt)
{
  if (mXMLTerminal) {
    return mXMLTerminal->SetPrompt(aPrompt);
  } else {
    return NS_ERROR_NOT_INITIALIZED;
  }
}


// Initialize XMLTermShell
NS_IMETHODIMP    
mozXMLTermShell::Init(nsIDOMWindow* aContentWin,
                      const PRUnichar* URL,
                      const PRUnichar* args)
{
  nsresult result;

  XMLT_LOG(mozXMLTermShell::Init,10,("\n"));

  if (mInitialized)
    return NS_ERROR_ALREADY_INITIALIZED;

  if (!aContentWin)
      return NS_ERROR_NULL_POINTER;

  mContentWindow = aContentWin;  // no addref

  nsCOMPtr<nsIScriptGlobalObject> globalObj = do_QueryInterface(mContentWindow,
                                                                &result);
  if (NS_FAILED(result) || !globalObj)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIWebShell> webShell;
  globalObj->GetWebShell(getter_AddRefs(webShell));
  if (!webShell)
    return NS_ERROR_FAILURE;
    
  mContentAreaWebShell = webShell;  // SVN: does this assignment addref?

  // Create XMLTerminal
  nsCOMPtr<mozIXMLTerminal> newXMLTerminal;
  result = NS_NewXMLTerminal(getter_AddRefs(newXMLTerminal));

  if(!newXMLTerminal)
    result = NS_ERROR_OUT_OF_MEMORY;

  if (NS_SUCCEEDED(result)) {
    // Initialize XMLTerminal with non-owning reference to us
    result = newXMLTerminal->Init(mContentAreaWebShell, this, URL, args);

    if (NS_SUCCEEDED(result)) {
      mXMLTerminal = newXMLTerminal;
    }
  }

  return result;
}


// De-initialize XMLTermShell and free resources
NS_IMETHODIMP
mozXMLTermShell::Finalize(void)
{
  XMLT_LOG(mozXMLTermShell::Finalize,10,("\n"));

  if (mXMLTerminal) {
    // Finalize and release reference to XMLTerm object owned by us
    mXMLTerminal->Finalize();
    mXMLTerminal = nsnull;
  }

  mContentAreaWebShell = nsnull;
  mContentWindow =       nsnull;

  mInitialized = PR_FALSE;

  return NS_OK;
}


// Poll for readable data from XMLTerminal
NS_IMETHODIMP mozXMLTermShell::Poll(void)
{
  if (!mXMLTerminal)
    return NS_ERROR_NOT_INITIALIZED;

  return mXMLTerminal->Poll();
}


// Send string to LineTerm as if the user had typed it
NS_IMETHODIMP mozXMLTermShell::SendText(const PRUnichar* buf,
                                        const PRUnichar* cookie)
{
  if (!mXMLTerminal)
    return NS_ERROR_FAILURE;

  nsAutoString sendStr (buf);

  XMLT_LOG(mozXMLTermShell::SendText,10,("length=%d\n", sendStr.Length()));

  return mXMLTerminal->SendText(sendStr, cookie);
}


// Create new XMLTerm window with specified argument string
NS_IMETHODIMP
mozXMLTermShell::NewXMLTermWindow(const PRUnichar* args)
{
  nsresult result = NS_OK;

  XMLT_LOG(mozXMLTermShell::NewXMLTermWindow,10,("\n"));

  // Create the toolkit core instance...
  nsIDOMToolkitCore* toolkit = nsnull;
  result = nsServiceManager::GetService(kToolkitCoreCID,
                                        NS_GET_IID(nsIDOMToolkitCore),
                                        (nsISupports**)&toolkit);
  if (NS_FAILED(result))
    return result;

  nsAutoString argStr (args);
  toolkit->ShowWindowWithArgs( "chrome://xmlterm/content/XMLTermFrame.xul",
                               nsnull, argStr );
  
  /* Release the toolkit... */
  if (nsnull != toolkit) {
    nsServiceManager::ReleaseService(kToolkitCoreCID, toolkit);
  }

  return result;
}


// Exit XMLTerm window
NS_IMETHODIMP    
mozXMLTermShell::Exit()
{  
  nsIAppShellService* appShell = nsnull;

  XMLT_LOG(mozXMLTermShell::Exit,10,("\n"));

  // Create the Application Shell instance...
  nsresult result = nsServiceManager::GetService(kAppShellServiceCID,
                                                 NS_GET_IID(nsIAppShellService),
                                                 (nsISupports**)&appShell);
  if (NS_SUCCEEDED(result)) {
    appShell->Shutdown();
    nsServiceManager::ReleaseService(kAppShellServiceCID, appShell);
  } 
  return NS_OK;
}
