
/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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

#include "nsRDFCore.h"
#include "nsIBrowserWindow.h"
#include "nsIWebShell.h"
#include "pratom.h"
#include "nsRepository.h"
#include "nsAppCores.h"
#include "nsAppCoresCIDs.h"
#include "nsAppCoresManager.h"

#include "nsIScriptContext.h"
#include "nsIScriptContextOwner.h"
#include "nsIDOMDocument.h"
#include "nsIDocument.h"
#include "nsIDOMWindow.h"


// Globals
static NS_DEFINE_IID(kISupportsIID,              NS_ISUPPORTS_IID);
static NS_DEFINE_IID(kIRDFCoreIID,               NS_IDOMRDFCORE_IID);

static NS_DEFINE_IID(kIDOMDocumentIID,           nsIDOMDocument::IID());
static NS_DEFINE_IID(kIDocumentIID,              nsIDocument::IID());

static NS_DEFINE_IID(kRDFCoreCID,                NS_RDFCORE_CID);
static NS_DEFINE_IID(kBrowserWindowCID,          NS_BROWSER_WINDOW_CID);


/////////////////////////////////////////////////////////////////////////
// nsRDFCore
/////////////////////////////////////////////////////////////////////////

nsRDFCore::nsRDFCore()
{
  printf("Created nsRDFCore\n");
  mScriptObject   = nsnull;
  mScriptContext  = nsnull;
/*
  mWindow         = nsnull;
*/
  IncInstanceCount();
  NS_INIT_REFCNT();
}

nsRDFCore::~nsRDFCore()
{
  NS_IF_RELEASE(mScriptContext);
/*
  NS_IF_RELEASE(mWindow);
*/
  DecInstanceCount();
}


NS_IMPL_ADDREF(nsRDFCore)
NS_IMPL_RELEASE(nsRDFCore)


NS_IMETHODIMP 
nsRDFCore::QueryInterface(REFNSIID aIID,void** aInstancePtr)
{
  if (aInstancePtr == NULL) {
    return NS_ERROR_NULL_POINTER;
  }

  // Always NULL result, in case of failure
  *aInstancePtr = NULL;

printf("About to compare RDFCOREIIDs\n");

  if ( aIID.Equals(kIRDFCoreIID) ) {
    *aInstancePtr = (void*) ((nsIDOMRDFCore*)this);
    AddRef();
    return NS_OK;
  }

printf("RDFCOREIIDs did not matched.\n");

  return nsBaseAppCore::QueryInterface(aIID, aInstancePtr);
}


NS_IMETHODIMP 
nsRDFCore::GetScriptObject(nsIScriptContext *aContext, void** aScriptObject)
{
  NS_PRECONDITION(nsnull != aScriptObject, "null arg");
  nsresult res = NS_OK;
  if (nsnull == mScriptObject) 
  {
      res = NS_NewScriptRDFCore(aContext, 
                                (nsISupports *)(nsIDOMRDFCore*)this, 
                                nsnull, 
                                &mScriptObject);
  }

  *aScriptObject = mScriptObject;
  return res;
}

NS_IMETHODIMP    
nsRDFCore::Init(const nsString& aId)
{
   
  nsBaseAppCore::Init(aId);

	nsAppCoresManager* sdm = new nsAppCoresManager();
        sdm->Add((nsIDOMBaseAppCore *)(nsBaseAppCore *)this);
	delete sdm;

	return NS_OK;
}

NS_IMETHODIMP    
nsRDFCore::DoSort(nsIDOMNode* node, const nsString& sortResource)
{
  printf("nsRDFCore::DoSort entered!!!\n");

/*
  if (nsnull == mScriptContext) {
    return NS_ERROR_FAILURE;
  }
*/

  printf("----------------------------\n");
  printf("-- Sort \n");
  printf("----------------------------\n");
  printf("Column: %s  \n", sortResource.ToNewCString());
  printf("----------------------------\n");
/*
  if (nsnull != mScriptContext) {
    const char* url = "";
    PRBool isUndefined = PR_FALSE;
    nsString rVal;
    mScriptContext->EvaluateString(mScript, url, 0, rVal, &isUndefined);
  }
*/
  return NS_OK;
}




