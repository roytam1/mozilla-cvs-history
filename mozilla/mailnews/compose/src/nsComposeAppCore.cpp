/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

#include "nsIDOMComposeAppCore.h"
#include "nsComposeAppCore.h"
#include "nsIScriptObjectOwner.h"
#include "nsIDOMBaseAppCore.h"
#include "nsJSComposeAppCore.h"
#include "nsIDOMDocument.h"
#include "nsIScriptContextOwner.h"

/* rhp - for access to webshell */
#include "prmem.h"
#include "plstr.h"
#include "nsCOMPtr.h"
#include "nsIDocument.h"
#include "nsIDOMWindow.h"
#include "nsIWebShell.h"
#include "nsIWebShellWindow.h"
#include "nsIScriptGlobalObject.h"
#include "nsIScriptContext.h"
#include "nsAppShellCIDs.h"
#include "nsIAppShellService.h"
#include "nsIServiceManager.h"
#include "nsIURL.h"
#include "nsMsgCompPrefs.h"

#include "nsMsgCompCID.h"
#include "nsIMsgCompose.h"
#include "nsIMsgCompFields.h"
#include "nsIMsgSend.h"

static NS_DEFINE_IID(kIScriptObjectOwnerIID, NS_ISCRIPTOBJECTOWNER_IID);
static NS_DEFINE_IID(kIDocumentIID, nsIDocument::GetIID());
static NS_DEFINE_IID(kIMsgComposeIID, NS_IMSGCOMPOSE_IID); 
static NS_DEFINE_CID(kMsgComposeCID, NS_MSGCOMPOSE_CID); 

static NS_DEFINE_IID(kIMsgCompFieldsIID, NS_IMSGCOMPFIELDS_IID); 
static NS_DEFINE_CID(kMsgCompFieldsCID, NS_MSGCOMPFIELDS_CID); 

static NS_DEFINE_IID(kIMsgSendIID, NS_IMSGSEND_IID); 
static NS_DEFINE_CID(kMsgSendCID, NS_MSGSEND_CID);

static NS_DEFINE_CID(kAppShellServiceCID, NS_APPSHELL_SERVICE_CID);


NS_BEGIN_EXTERN_C

//nsresult NS_MailNewsLoadUrl(const nsString& urlString, nsISupports * aConsumer);

NS_END_EXTERN_C

// we need this because of an egcs 1.0 (and possibly gcc) compiler bug
// that doesn't allow you to call ::nsISupports::GetIID() inside of a class
// that multiply inherits from nsISupports
static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);

class nsComposeAppCore : public nsIDOMComposeAppCore,
                     public nsIScriptObjectOwner
{
  
public:
	nsComposeAppCore();
	virtual ~nsComposeAppCore();

	NS_DECL_ISUPPORTS
	NS_DECL_IDOMBASEAPPCORE

	// nsIScriptObjectOwner
	NS_IMETHOD GetScriptObject(nsIScriptContext *aContext, void** aScriptObject);
	NS_IMETHOD SetScriptObject(void* aScriptObject);

	// nsIComposeAppCore
	NS_IMETHOD CompleteCallback(nsAutoString& aScript);
	NS_IMETHOD SetWindow(nsIDOMWindow* aWin);
	NS_IMETHOD NewMessage(nsAutoString& aUrl);
	NS_IMETHOD SendMessage(nsAutoString& aAddrTo, nsAutoString& aAddrCc,
		nsAutoString& aAddrBcc, nsAutoString& aSubject, nsAutoString& aMsg);

protected:
  
	nsIScriptContext *	GetScriptContext(nsIDOMWindow * aWin);

	nsString mId;
	nsString mScript;
	void *mScriptObject;
	nsIScriptContext   *mScriptContext;

	/* rhp - need this to drive message display */
	nsIDOMWindow       *mWindow;
	nsIWebShell        *mWebShell;
};

//
// nsComposeAppCore
//
nsComposeAppCore::nsComposeAppCore()
{
  mScriptObject		= nsnull;
  mWebShell			= nsnull; 
  mScriptObject		= nsnull;
  mScriptContext	= nsnull;
  mWindow			= nsnull;

  NS_INIT_REFCNT();
}

nsComposeAppCore::~nsComposeAppCore()
{
  NS_IF_RELEASE(mScriptContext);
  NS_IF_RELEASE(mWindow);
}

nsresult nsComposeAppCore::SetDocumentCharset(class nsString const & aCharset) 
{
	nsresult res = NS_OK;
	if (nsnull != mWindow) 
	{
		nsIDOMDocument* domDoc;
		res = mWindow->GetDocument(&domDoc);
		if (NS_SUCCEEDED(res) && nsnull != domDoc) 
		{
			nsIDocument * doc;
			res = domDoc->QueryInterface(kIDocumentIID,(void**)&doc);
			if (NS_SUCCEEDED(res) && nsnull != doc) 
			{
				nsString *aNewCharset = new nsString(aCharset);
				if (nsnull != aNewCharset) 
				{
					doc->SetDocumentCharacterSet(aNewCharset);
				}
				
				NS_RELEASE(doc);
			}
			
			NS_RELEASE(domDoc);
		}
	}
	
	return res;
}

nsIScriptContext *    
nsComposeAppCore::GetScriptContext(nsIDOMWindow * aWin)
{
  nsIScriptContext * scriptContext = nsnull;
  if (nsnull != aWin) {
    nsIDOMDocument * domDoc;
    aWin->GetDocument(&domDoc);
    if (nsnull != domDoc) {
      nsIDocument * doc;
      if (NS_OK == domDoc->QueryInterface(kIDocumentIID,(void**)&doc)) {
        nsIScriptContextOwner * owner = doc->GetScriptContextOwner();
        if (nsnull != owner) {
          owner->GetScriptContext(&scriptContext);
          NS_RELEASE(owner);
        }
        NS_RELEASE(doc);
      }
      NS_RELEASE(domDoc);
    }
  }
  return scriptContext;
}


//
// nsISupports
//
NS_IMPL_ADDREF(nsComposeAppCore);
NS_IMPL_RELEASE(nsComposeAppCore);

NS_IMETHODIMP
nsComposeAppCore::QueryInterface(REFNSIID aIID,void** aInstancePtr)
{
  if (aInstancePtr == NULL) {
      return NS_ERROR_NULL_POINTER;
  }

  // Always NULL result, in case of failure
  *aInstancePtr = NULL;

  if ( aIID.Equals(kIScriptObjectOwnerIID)) {
      *aInstancePtr = (void*) ((nsIScriptObjectOwner*)this);
      AddRef();
      return NS_OK;
  }
  if ( aIID.Equals(nsIDOMBaseAppCore::GetIID())) {
      *aInstancePtr = (void*) ((nsIDOMBaseAppCore*)this);
      AddRef();
      return NS_OK;
  }
  else if ( aIID.Equals(nsIDOMComposeAppCore::GetIID()) ) {
      *aInstancePtr = (void*) (nsIDOMComposeAppCore*)this;
      AddRef();
      return NS_OK;
  }
  else if ( aIID.Equals(kISupportsIID) ) {
      *aInstancePtr = (void*)(nsISupports*) (nsIScriptObjectOwner *) this;
      AddRef();
      return NS_OK;
  }

  return NS_NOINTERFACE;
}

//
// nsIScriptObjectOwner
//
nsresult
nsComposeAppCore::GetScriptObject(nsIScriptContext *aContext, void **aScriptObject)
{
  NS_PRECONDITION(nsnull != aScriptObject, "null arg");
  nsresult res = NS_OK;
  if (nsnull == mScriptObject) 
  {
      res = NS_NewScriptComposeAppCore(aContext, 
                                   (nsISupports *)(nsIDOMComposeAppCore*)this, 
                                   nsnull, 
                                   &mScriptObject);
  }

  *aScriptObject = mScriptObject;
  return res;
}

nsresult
nsComposeAppCore::SetScriptObject(void* aScriptObject)
{
  
  return NS_OK;
} 

//
// nsIDOMBaseAppCore
//
nsresult
nsComposeAppCore::Init(const nsString& aId)
{
	printf("Init\n");
	mId = aId;
	return NS_OK;
}


nsresult
nsComposeAppCore::GetId(nsString& aId)
{
	printf("GetID\n");
	aId = mId;
	return NS_OK;
}

//
// nsIComposeAppCore
//
NS_IMETHODIMP    
nsComposeAppCore::SetWindow(nsIDOMWindow* aWin)
{
	mWindow = aWin;
	NS_ADDREF(mWindow);
	mScriptContext = GetScriptContext(aWin);
	return NS_OK;
}


NS_IMETHODIMP    
nsComposeAppCore::CompleteCallback(nsAutoString& aScript)
{
	mScript = aScript;
	return NS_OK;
}

NS_IMETHODIMP    
nsComposeAppCore::NewMessage(nsAutoString& aUrl)
{

	char *  urlstr=nsnull;
	nsresult rv;
	nsString controllerCID;

	nsIAppShellService* appShell;
    rv = nsServiceManager::GetService(kAppShellServiceCID,
                                      nsIAppShellService::GetIID(),
                                      (nsISupports**)&appShell);
    if (NS_FAILED(rv)) return rv;

	nsIURL* url;
	nsIWidget* newWindow;
  
	rv = NS_NewURL(&url, aUrl);
	if (NS_FAILED(rv)) {
	  goto done;
	}

	controllerCID = "6B75BB61-BD41-11d2-9D31-00805F8ADDDF";
	appShell->CreateTopLevelWindow(nsnull,      // parent
                                   url,
                                   controllerCID,
                                   newWindow,   // result widget
                                   nsnull,      // observer
                                   nsnull,      // callbacks
                                   615,         // width
                                   650);        // height
done:
	NS_RELEASE(url);
    (void)nsServiceManager::ReleaseService(kAppShellServiceCID, appShell);
	return NS_OK;
}

NS_IMETHODIMP nsComposeAppCore::SendMessage(nsAutoString& aAddrTo, nsAutoString& aAddrCc,
									nsAutoString& aAddrBcc, nsAutoString& aSubject, nsAutoString& aMsg)
{
	nsMsgCompPrefs pCompPrefs;
	char* pUserEmail = nsnull;

	if (nsnull == mScriptContext)
		return NS_ERROR_FAILURE;

#ifdef DEBUG
	  printf("----------------------------\n");
	  printf("-- Sending Mail Message\n");
	  printf("----------------------------\n");
	  printf("To: %s  Cc: %s  Bcc: %s\n", aAddrTo.ToNewCString(), aAddrCc.ToNewCString(), aAddrBcc.ToNewCString());
	  printf("Subject: %s  \nMsg: %s\n", aSubject.ToNewCString(), aMsg.ToNewCString());
	  printf("----------------------------\n");
#endif //DEBUG

//	nsIMsgCompose *pMsgCompose; 
	nsIMsgCompFields *pMsgCompFields;
	nsIMsgSend *pMsgSend;
	nsresult res;
	
	res = nsComponentManager::CreateInstance(kMsgSendCID, 
                                               NULL, 
                                               kIMsgSendIID, 
                                               (void **) &pMsgSend); 

	if (NS_SUCCEEDED(res) && pMsgSend) { 
		printf("We succesfully obtained a nsIMsgSend interface....\n");

		res = nsComponentManager::CreateInstance(kMsgCompFieldsCID, 
													NULL, 
													kIMsgCompFieldsIID, 
													(void **) &pMsgCompFields); 
		if (NS_SUCCEEDED(NS_OK) && pMsgCompFields) { 
			pMsgCompFields->SetFrom((char *)pCompPrefs.GetUserEmail(), NULL);
			pMsgCompFields->SetOrganization((char *)pCompPrefs.GetOrganization(), NULL);
			pMsgCompFields->SetTo(aAddrTo.ToNewCString(), NULL);
			pMsgCompFields->SetCc(aAddrCc.ToNewCString(), NULL);
			pMsgCompFields->SetBcc(aAddrBcc.ToNewCString(), NULL);
			pMsgCompFields->SetSubject(aSubject.ToNewCString(), NULL);
			pMsgCompFields->SetBody(aMsg.ToNewCString(), NULL);

			pMsgSend->SendMessage(pMsgCompFields, NULL);

			pMsgCompFields->Release(); 
		}

		pMsgSend->Release();
	}

	if (nsnull != mScriptContext) {
		const char* url = "";
		PRBool isUndefined = PR_FALSE;
		nsString rVal;
		mScriptContext->EvaluateString(mScript, url, 0, rVal, &isUndefined);
	}

	if (mWindow) {
		mWindow->Close();	//JFD Doesn't work yet because mopener wasn't set!
		NS_RELEASE(mWindow);
	}

	PR_FREEIF(pUserEmail);

	return NS_OK;
}
                              
extern "C"
nsresult
NS_NewComposeAppCore(nsIDOMComposeAppCore **aResult)
{
  if (!aResult) return NS_ERROR_NULL_POINTER;

  nsComposeAppCore *appcore = new nsComposeAppCore();
  if (appcore) {
    return appcore->QueryInterface(nsIDOMComposeAppCore::GetIID(),
                                   (void **)aResult);

  }
  return NS_ERROR_NOT_INITIALIZED;
}

