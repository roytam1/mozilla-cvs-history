/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 *   Pierre Phaneuf <pp@ludusdesign.com>
 */

#include "nsCOMPtr.h"
#include "nsWalletService.h"
#include "nsIServiceManager.h"
#include "wallet.h"
#include "singsign.h"
#include "nsPassword.h"
#include "nsIObserverService.h"
#include "nsIDOMHTMLDocument.h"
#include "nsIDOMHTMLCollection.h"
#include "nsIDOMHTMLFormElement.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsIDocumentLoader.h"
#include "nsCURILoader.h"
#include "nsIWebShell.h"
#include "nsIDocumentViewer.h"
#include "nsIDOMHTMLInputElement.h"
#include "nsIFormControl.h"
#include "nsIDocShell.h"
#include "nsIDOMWindowInternal.h"
#include "nsIInterfaceRequestor.h"
#include "nsIPrompt.h"
#include "nsIChannel.h"
#include "nsIWindowWatcher.h"
#include "nsIWebProgress.h"
#include "nsXPIDLString.h"
#include "nsICategoryManager.h"

// for making the leap from nsIDOMWindowInternal -> nsIPresShell
#include "nsIScriptGlobalObject.h"

static NS_DEFINE_IID(kDocLoaderServiceCID, NS_DOCUMENTLOADER_SERVICE_CID);


nsWalletlibService::nsWalletlibService()
{
  NS_INIT_REFCNT();
}

nsWalletlibService::~nsWalletlibService()
{
#ifdef DEBUG_dp
  printf("Wallet Service destroyed successfully.\n");
#endif /* DEBUG_dp */
  Wallet_ReleaseAllLists();
  SI_ClearUserData();
}

NS_IMPL_THREADSAFE_ISUPPORTS5(nsWalletlibService,
                              nsIWalletService,
                              nsIObserver,
                              nsIFormSubmitObserver,
                              nsIWebProgressListener,
                              nsISupportsWeakReference)

NS_IMETHODIMP nsWalletlibService::WALLET_PreEdit(nsAutoString& walletList) {
  ::WLLT_PreEdit(walletList);
  return NS_OK;
}

NS_IMETHODIMP nsWalletlibService::WALLET_PostEdit(nsAutoString walletList) {
  ::WLLT_PostEdit(walletList);
  return NS_OK;
}

NS_IMETHODIMP nsWalletlibService::WALLET_ChangePassword(PRBool* status) {
  ::WLLT_ChangePassword(status);
  return NS_OK;
}

NS_IMETHODIMP nsWalletlibService::WALLET_DeleteAll() {
  ::WLLT_DeleteAll();
  return NS_OK;
}

NS_IMETHODIMP
nsWalletlibService::WALLET_RequestToCapture(nsIDOMWindowInternal* aWin,
                                            PRUint32* status)
{

  nsCOMPtr<nsIScriptGlobalObject> scriptGlobalObject; 
  scriptGlobalObject = do_QueryInterface(aWin);
  nsCOMPtr<nsIDocShell> docShell; 
  scriptGlobalObject->GetDocShell(getter_AddRefs(docShell)); 

  nsCOMPtr<nsIPresShell> presShell;
  if(docShell) 
   docShell->GetPresShell(getter_AddRefs(presShell));
  
  ::WLLT_RequestToCapture(presShell, aWin, status);
  return NS_OK;
}

NS_IMETHODIMP
nsWalletlibService::WALLET_PrefillOneElement
    (nsIDOMWindowInternal* aWin, nsIDOMNode* elementNode, PRUnichar **value)
{
  nsAutoString compositeValue;
  nsresult rv = ::WLLT_PrefillOneElement(aWin, elementNode, compositeValue);
  *value = compositeValue.ToNewUnicode();
  return rv;
}

NS_IMETHODIMP
nsWalletlibService::WALLET_Prefill(PRBool quick,
                                   nsIDOMWindowInternal* aWin,
                                   PRBool* status)
{
  nsCOMPtr<nsIScriptGlobalObject> scriptGlobalObject;
  scriptGlobalObject = do_QueryInterface(aWin);
  nsCOMPtr<nsIDocShell> docShell; 
  scriptGlobalObject->GetDocShell(getter_AddRefs(docShell)); 
  
  nsCOMPtr<nsIPresShell> presShell;
  if(docShell)
    docShell->GetPresShell(getter_AddRefs(presShell));

  return ::WLLT_Prefill(presShell, quick, aWin);
}

NS_IMETHODIMP nsWalletlibService::WALLET_PrefillReturn(nsAutoString results){
  ::WLLT_PrefillReturn(results);
  return NS_OK;
}

NS_IMETHODIMP nsWalletlibService::WALLET_FetchFromNetCenter(){
  ::WLLT_FetchFromNetCenter();
  return NS_OK;
}

NS_IMETHODIMP nsWalletlibService::WALLET_ExpirePassword(PRBool* status){
  ::WLLT_ExpirePassword(status);
  return NS_OK;
}

NS_IMETHODIMP nsWalletlibService::WALLET_InitReencryptCallback(nsIDOMWindowInternal* window){
  /* register callback to be used when encryption pref changes */
  ::WLLT_InitReencryptCallback(window);
  return NS_OK;
}

NS_IMETHODIMP nsWalletlibService::SI_RemoveUser(const char *key, const PRUnichar *userName) {
  ::SINGSIGN_RemoveUser(key, userName);
  return NS_OK;
}

NS_IMETHODIMP nsWalletlibService::SI_StorePassword(const char *key, const PRUnichar *userName, const PRUnichar *password) {
  ::SINGSIGN_StorePassword(key, userName, password);
  return NS_OK;
}


NS_IMETHODIMP nsWalletlibService::WALLET_GetNopreviewListForViewer(nsAutoString& aNopreviewList){
  ::WLLT_GetNopreviewListForViewer(aNopreviewList);
  return NS_OK;
}

NS_IMETHODIMP nsWalletlibService::WALLET_GetNocaptureListForViewer(nsAutoString& aNocaptureList){
  ::WLLT_GetNocaptureListForViewer(aNocaptureList);
  return NS_OK;
}

NS_IMETHODIMP nsWalletlibService::WALLET_GetPrefillListForViewer(nsAutoString& aPrefillList){
  ::WLLT_GetPrefillListForViewer(aPrefillList);
  return NS_OK;
}

NS_IMETHODIMP nsWalletlibService::SI_SignonViewerReturn(nsAutoString results){
  ::SINGSIGN_SignonViewerReturn(results);
  return NS_OK;
}

NS_IMETHODIMP nsWalletlibService::Observe(nsISupports*, const PRUnichar *aTopic, const PRUnichar *someData) 
{
  if (!nsCRT::strcmp(aTopic, NS_LITERAL_STRING("profile-before-change").get())) {
    WLLT_ClearUserData();
    if (!nsCRT::strcmp(someData, NS_LITERAL_STRING("shutdown-cleanse").get())) {
      WLLT_DeletePersistentUserData();
    }
  }
  return NS_OK;
}

#define CRLF "\015\012"   
NS_IMETHODIMP nsWalletlibService::Notify(nsIContent* formNode, nsIDOMWindowInternal* window, nsIURI* actionURL, PRBool* cancelSubmit)
{
  if (!formNode) {
    return NS_ERROR_FAILURE;
  }
  ::WLLT_OnSubmit(formNode, window);
  return NS_OK;
}

NS_IMETHODIMP
nsWalletlibService::RegisterProc(nsIComponentManager *aCompMgr,
                                 nsIFile *aPath,
                                 const char *registryLocation,
                                 const char *componentType,
                                 const nsModuleComponentInfo *info)
{
  // Register ourselves into the NS_CATEGORY_HTTP_STARTUP
  nsresult rv;
  nsCOMPtr<nsICategoryManager> catman = do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);
  if (NS_FAILED(rv)) return rv;

  nsXPIDLCString prevEntry;
  catman->AddCategoryEntry(NS_FIRST_FORMSUBMIT_CATEGORY, "Form Manager", NS_WALLETSERVICE_CONTRACTID,
                           PR_TRUE, PR_TRUE, getter_Copies(prevEntry));
  
  catman->AddCategoryEntry(NS_PASSWORDMANAGER_CATEGORY, "Password Manager", NS_WALLETSERVICE_CONTRACTID,
                           PR_TRUE, PR_TRUE, getter_Copies(prevEntry));
  return NS_OK;
}

NS_IMETHODIMP
nsWalletlibService::UnregisterProc(nsIComponentManager *aCompMgr,
                                   nsIFile *aPath,
                                   const char *registryLocation,
                                   const nsModuleComponentInfo *info)
{
  nsresult rv;
  nsCOMPtr<nsICategoryManager> catman = do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);
  if (NS_FAILED(rv)) return rv;
  
  catman->DeleteCategoryEntry(NS_FIRST_FORMSUBMIT_CATEGORY,
                              NS_WALLETSERVICE_CONTRACTID, PR_TRUE);
  
  catman->DeleteCategoryEntry(NS_PASSWORDMANAGER_CATEGORY,
                              NS_WALLETSERVICE_CONTRACTID, PR_TRUE);

  // Return value is not used from this function.
  return NS_OK;
}

nsresult nsWalletlibService::Init() 
{
  nsresult rv;

  nsCOMPtr<nsIObserverService> svc = 
           do_GetService(NS_OBSERVERSERVICE_CONTRACTID, &rv);
  if (NS_SUCCEEDED(rv) && svc) {
    // Register as an observer of form submission
    nsAutoString  topic; topic.AssignWithConversion(NS_FORMSUBMIT_SUBJECT);
    svc->AddObserver(this, topic.get());
    // Register as an observer of profile changes
    svc->AddObserver(this, NS_LITERAL_STRING("profile-before-change").get());
  }
  else
    NS_ASSERTION(PR_FALSE, "Could not get nsIObserverService");

  // Get the global document loader service...  
  nsCOMPtr<nsIDocumentLoader> docLoaderService = 
           do_GetService(kDocLoaderServiceCID, &rv);
  if (NS_SUCCEEDED(rv) && docLoaderService) {
    nsCOMPtr<nsIWebProgress> progress(do_QueryInterface(docLoaderService, &rv));
    if (NS_SUCCEEDED(rv))
        (void) progress->AddProgressListener((nsIWebProgressListener*)this);
  }
  else
    NS_ASSERTION(PR_FALSE, "Could not get nsIDocumentLoader");
  
  return NS_OK;
}

// nsIWebProgressListener implementation
NS_IMETHODIMP
nsWalletlibService::OnStateChange(nsIWebProgress* aWebProgress, 
                                  nsIRequest *aRequest, 
                                  PRInt32 progressStateFlags, 
                                  nsresult aStatus)
{
    nsresult rv = NS_OK;

    // If the load failed, do not try to prefill...
    if (NS_FAILED(aStatus)) {
       return NS_OK;
    }

    if (progressStateFlags & nsIWebProgressListener::STATE_IS_DOCUMENT) {
        if (progressStateFlags & nsIWebProgressListener::STATE_STOP) {

          nsCOMPtr<nsIDOMWindow> domWin;
          rv = aWebProgress->GetDOMWindow(getter_AddRefs(domWin));
          if (NS_FAILED(rv)) return rv;

          nsCOMPtr<nsIDOMDocument> domDoc;
          rv = domWin->GetDocument(getter_AddRefs(domDoc));
          if (NS_FAILED(rv)) return rv;

          // we only want to handle HTML documents as they're the
          // only one's that can have forms which we might want to
          // pre-fill.
          nsCOMPtr<nsIDOMHTMLDocument> htmldoc(do_QueryInterface(domDoc, &rv));
          if (NS_FAILED(rv)) return NS_OK;
              
          nsCOMPtr<nsIDocument> doc(do_QueryInterface(htmldoc, &rv));
          if (NS_FAILED(rv)) {
            NS_ASSERTION(0, "no document available");
            return NS_OK;
          }
  
          nsCOMPtr<nsIURI> uri;
          doc->GetDocumentURL(getter_AddRefs(uri));
          if (!uri) {
            NS_ASSERTION(0, "no URI available");
            return NS_OK;
          }

          nsXPIDLCString spec;
          rv = uri->GetSpec(getter_Copies(spec));
          if (NS_FAILED(rv)) return rv;

          nsCOMPtr<nsIDOMHTMLCollection> forms;
          rv = htmldoc->GetForms(getter_AddRefs(forms));
          if (NS_FAILED(rv) || (forms == nsnull)) return rv;

          PRUint32 elementNumber = 0;
          PRUint32 numForms;
          forms->GetLength(&numForms);
          for (PRUint32 formX = 0; formX < numForms; formX++) {
            nsCOMPtr<nsIDOMNode> formNode;
            forms->Item(formX, getter_AddRefs(formNode));
            if (nsnull != formNode) {
              nsCOMPtr<nsIDOMHTMLFormElement> formElement(do_QueryInterface(formNode));
              if ((nsnull != formElement)) {
                nsCOMPtr<nsIDOMHTMLCollection> elements;
                rv = formElement->GetElements(getter_AddRefs(elements));
                if ((NS_SUCCEEDED(rv)) && (nsnull != elements)) {
                  /* got to the form elements at long last */ 
                  PRUint32 numElements;
                  elements->GetLength(&numElements);
                  /* get number of passwords on form */
                  PRInt32 passwordCount = 0;
                  for (PRUint32 elementXX = 0; elementXX < numElements; elementXX++) {
                    nsCOMPtr<nsIDOMNode> elementNode;
                    elements->Item(elementXX, getter_AddRefs(elementNode));
                    if (nsnull != elementNode) {
                      nsCOMPtr<nsIDOMHTMLInputElement> inputElement(do_QueryInterface(elementNode));
                      if ((NS_SUCCEEDED(rv)) && (nsnull != inputElement)) {
                        nsAutoString type;
                        rv = inputElement->GetType(type);
                        if (NS_SUCCEEDED(rv)) {
                          if (type.CompareWithConversion("password", PR_TRUE) == 0) {
                            passwordCount++;
                          }
                        }
                      }
                    }
                  }
                  /* don't prefill if there were no passwords on the form */
                  if (passwordCount == 0) {
                    continue;
                  }
                  for (PRUint32 elementX = 0; elementX < numElements; elementX++) {
                    nsCOMPtr<nsIDOMNode> elementNode;
                    elements->Item(elementX, getter_AddRefs(elementNode));
                    if (nsnull != elementNode) {
                      nsCOMPtr<nsIDOMHTMLInputElement> inputElement(do_QueryInterface(elementNode));
                      if ((NS_SUCCEEDED(rv)) && (nsnull != inputElement)) {
                        nsAutoString type;
                        rv = inputElement->GetType(type);
                        if (NS_SUCCEEDED(rv)) {
                          if ((type.IsEmpty()) || (type.CompareWithConversion("text", PR_TRUE) == 0) ||
                            (type.CompareWithConversion("password", PR_TRUE) == 0)) {
                            nsAutoString field;
                            rv = inputElement->GetName(field);
                            if (NS_SUCCEEDED(rv)) {
                              PRUnichar* nameString = field.ToNewUnicode();
                              if (nameString) {
                                /* note: we do not want to prefill if there is a default value */
                                nsAutoString value;
                                rv = inputElement->GetValue(value);
                                if (NS_FAILED(rv) || value.Length() == 0) {
                                  PRUnichar* valueString = NULL;
                                  nsCOMPtr<nsIInterfaceRequestor> interfaces;
                                  nsCOMPtr<nsIPrompt> prompter;

                                  nsCOMPtr<nsIChannel> channel = do_QueryInterface(aRequest);
                                  if (channel)
                                    channel->GetNotificationCallbacks(getter_AddRefs(interfaces));
                                  if (interfaces)
                                    interfaces->GetInterface(NS_GET_IID(nsIPrompt), getter_AddRefs(prompter));
                                  if (!prompter) {
                                    nsCOMPtr<nsIWindowWatcher> wwatch(do_GetService("@mozilla.org/embedcomp/window-watcher;1"));
                                    if (wwatch)
                                      wwatch->GetNewPrompter(0, getter_AddRefs(prompter));
                                  }
                                  if (prompter) {
                                    SINGSIGN_RestoreSignonData(prompter, spec, nameString, &valueString, elementNumber++);
                                  }
                                  if (valueString) {
                                    value = valueString;
                                    rv = inputElement->SetValue(value);
                                    // warning! don't delete valueString
                                  }
                                }
                                Recycle(nameString);
                              }
                            }
                          }
                        }
                      }
                    }
                  }
                }
              }
            }
          }
        }
    }
    return rv;
}

NS_IMETHODIMP
nsWalletlibService::OnProgressChange(nsIWebProgress *aWebProgress,
                                     nsIRequest *aRequest,
                                     PRInt32 aCurSelfProgress,
                                     PRInt32 aMaxSelfProgress,
                                     PRInt32 aCurTotalProgress,
                                     PRInt32 aMaxTotalProgress)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsWalletlibService::OnLocationChange(nsIWebProgress* aWebProgress,
                                     nsIRequest* aRequest,
                                     nsIURI *location)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsWalletlibService::OnStatusChange(nsIWebProgress* aWebProgress,
                                   nsIRequest* aRequest,
                                   nsresult aStatus,
                                   const PRUnichar* aMessage)
{
    return NS_ERROR_NOT_IMPLEMENTED;    
}


NS_IMETHODIMP
nsWalletlibService::OnSecurityChange(nsIWebProgress *aWebProgress, 
                                     nsIRequest *aRequest, 
                                     PRInt32 state)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsWalletlibService::GetPassword(PRUnichar **password)
{
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsWalletlibService::HaveData(nsIPrompt* dialog, const char *key, const PRUnichar *userName, PRBool *_retval)
{
  return ::SINGSIGN_HaveData(dialog, key, userName, _retval);
}

NS_IMETHODIMP
nsWalletlibService::WALLET_Encrypt (const PRUnichar *text, char **crypt) {
  nsAutoString textAutoString( text );
  nsAutoString cryptAutoString;
  PRBool rv = ::Wallet_Encrypt(textAutoString, cryptAutoString);
  *crypt = cryptAutoString.ToNewCString();
  return rv;
}

NS_IMETHODIMP
nsWalletlibService::WALLET_Decrypt (const char *crypt, PRUnichar **text) {
  nsAutoString cryptAutoString; cryptAutoString.AssignWithConversion(crypt);
  nsAutoString textAutoString;
  PRBool rv = ::Wallet_Decrypt(cryptAutoString, textAutoString);
  *text = textAutoString.ToNewUnicode();
  return rv;
}

////////////////////////////////////////////////////////////////////////////////
// nsSingleSignOnProfileObserver
// nsSingleSignOnPrompt itself can't be a profile change observer because, if the
// last instance has been destroyed at the time of a profile change, there's
// nothing to get the notification.

class nsSingleSignOnProfileObserver : public nsIObserver
{
public:
    nsSingleSignOnProfileObserver() { NS_INIT_REFCNT(); }
    virtual ~nsSingleSignOnProfileObserver() {}
    
    NS_DECL_ISUPPORTS
    
    NS_IMETHODIMP Observe(nsISupports*, const PRUnichar *aTopic, const PRUnichar *someData) 
    {
        if (!nsCRT::strcmp(aTopic, NS_LITERAL_STRING("profile-before-change").get())) {
            SI_ClearUserData();
        if (!nsCRT::strcmp(someData, NS_LITERAL_STRING("shutdown-cleanse").get()))
            SI_DeletePersistentUserData();
        }
        return NS_OK;
    }
};
NS_IMPL_THREADSAFE_ISUPPORTS1(nsSingleSignOnProfileObserver, nsIObserver)

////////////////////////////////////////////////////////////////////////////////
// nsSingleSignOnPrompt

PRBool nsSingleSignOnPrompt::mgRegisteredObserver = PR_FALSE;

NS_IMPL_THREADSAFE_ISUPPORTS2(nsSingleSignOnPrompt,
                              nsISingleSignOnPrompt,
                              nsIAuthPrompt)

nsresult
nsSingleSignOnPrompt::Init()
{
  if (!mgRegisteredObserver) {
    nsSingleSignOnProfileObserver *observer = new nsSingleSignOnProfileObserver;
    if (!observer)
        return NS_ERROR_OUT_OF_MEMORY;
    nsCOMPtr<nsIObserverService> svc(do_GetService(NS_OBSERVERSERVICE_CONTRACTID));
    if (!svc) return NS_ERROR_FAILURE;
    // The observer service holds the only ref to the observer
    // It thus has the lifespan of the observer service
    svc->AddObserver(observer, NS_LITERAL_STRING("profile-before-change").get());
    mgRegisteredObserver = PR_TRUE;
  }
  return NS_OK;
}


NS_IMETHODIMP
nsSingleSignOnPrompt::Prompt(const PRUnichar *dialogTitle, const PRUnichar *text, 
                             const PRUnichar *passwordRealm, PRUint32 savePassword,
                             const PRUnichar *defaultText, PRUnichar **result, PRBool *_retval)
{
  nsresult rv;
  nsCAutoString realm;
  realm.AssignWithConversion(passwordRealm);     // XXX should be PRUnichar*
  rv = SINGSIGN_Prompt(dialogTitle, text, defaultText, result, realm.get(), mPrompt, _retval, savePassword);
  return rv;
}

NS_IMETHODIMP
nsSingleSignOnPrompt::PromptUsernameAndPassword(const PRUnichar *dialogTitle, const PRUnichar *text, 
                                                const PRUnichar *passwordRealm, PRUint32 savePassword, 
                                                PRUnichar **user, PRUnichar **pwd, PRBool *_retval)
{
  nsresult rv;
  nsCAutoString realm;
  realm.AssignWithConversion(passwordRealm);     // XXX should be PRUnichar*
  rv = SINGSIGN_PromptUsernameAndPassword(dialogTitle, text, user, pwd,
                                          realm.get(), mPrompt, _retval, savePassword);
  return rv;
}

NS_IMETHODIMP
nsSingleSignOnPrompt::PromptPassword(const PRUnichar *dialogTitle, const PRUnichar *text, 
                                     const PRUnichar *passwordRealm, PRUint32 savePassword, 
                                     PRUnichar **pwd, PRBool *_retval)
{
  nsresult rv;
  nsCAutoString realm;
  realm.AssignWithConversion(passwordRealm);     // XXX should be PRUnichar*
  rv = SINGSIGN_PromptPassword(dialogTitle, text, pwd,
                               realm.get(), mPrompt, _retval, savePassword);
  return rv;
}
  
// nsISingleSignOnPrompt methods:

NS_IMETHODIMP
nsSingleSignOnPrompt::SetPromptDialogs(nsIPrompt* dialogs)
{
  mPrompt = dialogs;
  return NS_OK;
}

////////////////////////////////////////////////////////////////////////////////
