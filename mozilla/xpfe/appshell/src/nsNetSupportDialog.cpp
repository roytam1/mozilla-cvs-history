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
#include "nsNetSupportDialog.h"
#include "nsCOMPtr.h"

#include "nsIAppShellService.h"
#include "nsIServiceManager.h"
#include "nsAppShellCIDs.h"
#include "nsIWebShellWindow.h"
#include "nsICommonDialogs.h"
#include "nsIDOMWindow.h"
#include "nsXPComFactory.h"

/* Define Class IDs */
static NS_DEFINE_IID(kAppShellServiceCID, NS_APPSHELL_SERVICE_CID);


/* Define Interface IDs */
static NS_DEFINE_IID(kIAppShellServiceIID,       NS_IAPPSHELL_SERVICE_IID);


static NS_DEFINE_IID(kIFactoryIID,         NS_IFACTORY_IID);
static NS_DEFINE_IID(kISupportsIID,         NS_ISUPPORTS_IID);

static NS_DEFINE_CID( kCommonDialogsCID,          NS_CommonDialog_CID);


PRBool GetNSIPrompt( nsCOMPtr<nsIPrompt> & outPrompt )
{
  nsresult rv;
  NS_WITH_SERVICE(nsIAppShellService, appshellservice, kAppShellServiceCID, &rv);
  if(NS_FAILED(rv)) {
    return PR_FALSE;
  }
  nsCOMPtr<nsIWebShellWindow> webshellwindow;
  appshellservice->GetHiddenWindow(getter_AddRefs(webshellwindow));
  outPrompt = do_QueryInterface(webshellwindow);
  if ( outPrompt.get() != NULL )
  	return PR_TRUE;
  return PR_FALSE;
} 

nsNetSupportDialog::nsNetSupportDialog()
{
	NS_INIT_REFCNT();
}

nsNetSupportDialog::~nsNetSupportDialog()
{
}



NS_IMETHODIMP nsNetSupportDialog::Alert(const PRUnichar *text)
{

	 nsresult rv = NS_ERROR_FAILURE;
	 nsCOMPtr< nsIPrompt> dialogService;
     if( GetNSIPrompt( dialogService ) )
        rv = dialogService->Alert(  text );

	 return rv;
}

NS_IMETHODIMP nsNetSupportDialog::Confirm(const PRUnichar *text, PRBool *returnValue)
{

	nsresult rv = NS_ERROR_FAILURE;
	nsCOMPtr< nsIPrompt> dialogService;
    if( GetNSIPrompt( dialogService ) )
    	rv = dialogService->Confirm(  text, returnValue );

	return rv;
}

NS_IMETHODIMP	nsNetSupportDialog::ConfirmCheck(const PRUnichar *text, 
                                               const PRUnichar *checkMsg, 
                                               PRBool *checkValue, 
                                               PRBool *returnValue)
{

	nsresult rv = NS_ERROR_FAILURE;
	nsCOMPtr< nsIPrompt> dialogService;
    if( GetNSIPrompt( dialogService ) )
    	rv = dialogService->ConfirmCheck(  text, checkMsg, checkValue, returnValue );

	return rv;
}



NS_IMETHODIMP	nsNetSupportDialog::UniversalDialog
	(const PRUnichar *inTitleMessage,
	const PRUnichar *inDialogTitle, /* e.g., alert, confirm, prompt, prompt password */
	const PRUnichar *inMsg, /* main message for dialog */
	const PRUnichar *inCheckboxMsg, /* message for checkbox */
	const PRUnichar *inButton0Text, /* text for first button */
	const PRUnichar *inButton1Text, /* text for second button */
	const PRUnichar *inButton2Text, /* text for third button */
	const PRUnichar *inButton3Text, /* text for fourth button */
	const PRUnichar *inEditfield1Msg, /*message for first edit field */
	const PRUnichar *inEditfield2Msg, /* message for second edit field */
	PRUnichar **inoutEditfield1Value, /* initial and final value for first edit field */
	PRUnichar **inoutEditfield2Value, /* initial and final value for second edit field */
	const PRUnichar *inIConURL, /* url of icon to be displayed in dialog */
		/* examples are
		   "chrome://global/skin/question-icon.gif" for question mark,
		   "chrome://global/skin/alert-icon.gif" for exclamation mark
		*/
	PRBool *inoutCheckboxState, /* initial and final state of check box */
	PRInt32 inNumberButtons, /* total number of buttons (0 to 4) */
	PRInt32 inNumberEditfields, /* total number of edit fields (0 to 2) */
	PRInt32 inEditField1Password, /* is first edit field a password field */
	PRInt32 *outButtonPressed) /* number of button that was pressed (0 to 3) */
{
	nsresult rv = NS_ERROR_FAILURE;
	nsCOMPtr< nsIPrompt> dialogService;
    if( GetNSIPrompt( dialogService ) )
    {
        	rv = dialogService->UniversalDialog(
                        inTitleMessage, inDialogTitle, inMsg, inCheckboxMsg,
                        inButton0Text, inButton1Text, inButton2Text, inButton3Text,
                        inEditfield1Msg, inEditfield2Msg, inoutEditfield1Value,
                        inoutEditfield2Value, inIConURL, inoutCheckboxState, inNumberButtons,
                        inNumberEditfields, inEditField1Password, outButtonPressed);
	 }
	 return rv;
}

NS_IMETHODIMP nsNetSupportDialog::Prompt(const PRUnichar *text,
                                         const PRUnichar *defaultText, 
                                         PRUnichar **resultText,
                                         PRBool *returnValue)
{

	nsresult rv = NS_ERROR_FAILURE;
	nsCOMPtr< nsIPrompt> dialogService;
    if( GetNSIPrompt( dialogService ) )
		rv = dialogService->Prompt( text, defaultText, resultText, returnValue );
	 
	return rv;	
}

NS_IMETHODIMP nsNetSupportDialog::PromptUsernameAndPassword(const PRUnichar *text,
                                                            PRUnichar **user,
                                                            PRUnichar **pwd,
                                                            PRBool *returnValue)
{

	nsresult rv = NS_ERROR_FAILURE;
	nsCOMPtr< nsIPrompt> dialogService;
    if( GetNSIPrompt( dialogService ) )
    	rv = dialogService->PromptUsernameAndPassword(  text, user, pwd, returnValue );
	return rv;	
}

NS_IMETHODIMP nsNetSupportDialog::PromptPassword(const PRUnichar *text, const PRUnichar *title, PRUnichar **pwd, PRBool *_retval)
{
	nsresult rv = NS_ERROR_FAILURE;
	nsCOMPtr< nsIPrompt> dialogService;
    if( GetNSIPrompt( dialogService ) )
    	rv = dialogService->PromptPassword(  text, title, pwd, _retval );
	 
	return rv;	
}


nsresult nsNetSupportDialog::Select(const PRUnichar *inDialogTitle, const PRUnichar *inMsg, PRUint32 inCount, const PRUnichar **inList, PRInt32 *outSelection, PRBool *_retval)
{
	nsresult rv = NS_ERROR_FAILURE;
	nsCOMPtr< nsIPrompt> dialogService;
    if( GetNSIPrompt( dialogService ) )
    	rv = dialogService->Select( inDialogTitle, inMsg,  inCount, inList, outSelection, _retval);
	 
	return rv;	
}




// COM Fluff

NS_IMPL_ISUPPORTS1( nsNetSupportDialog, nsIPrompt );
NS_DEF_FACTORY(NetSupportDialog, nsNetSupportDialog)



nsresult NS_NewNetSupportDialogFactory(nsIFactory** aFactory)
{
  nsresult rv = NS_OK;
  nsIFactory* inst = new nsNetSupportDialogFactory();
  if (nsnull == inst)
  {
  	rv = NS_ERROR_OUT_OF_MEMORY;
  }
  else
  {
    NS_ADDREF(inst);
  }
  *aFactory = inst;
  return rv;
}

