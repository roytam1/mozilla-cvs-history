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
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */
#include "nsIDOMWindow.h"
#include "nsICommonDialogs.h"
#include "nsCOMPtr.h"
#include "nsIScriptGlobalObject.h"
#include "nsXPComFactory.h"
#include "nsIComponentManager.h"
#include "nsIServiceManager.h"
static NS_DEFINE_CID( kDialogParamBlockCID,          NS_DialogParamBlock_CID);

#if 0
nsresult FE_Select( nsIDOMWindow* inParent, const PRUnichar* inTitle, const PRUnichar* inMsg, PRUnichar** inList , PRInt32& ioCount, PRInt32* _retval )
{	
	nsresult rv;
	const PRInt32 eSelection = 2 ;
	nsIDialogParamBlock* block = NULL;
	rv = nsComponentManager::CreateInstance( kDialogParamBlockCID,
                                                      0,
                                                      nsIDialogParamBlock::GetIID(),
                                                      (void**)&block );
      
	if ( NS_FAILED( rv ) )
		return rv;
	block->SetNumberStrings( ioCount + 2 );
	block->SetString( 0, inMsg );
	block->SetString(1, inMsg );
	block->SetInt( eSelection, ioCount );
	for ( int32 i =2; i<= ioCount+1; i++ )
	{
		block->SetString( i, inList[i-2] );
	}
	static NS_DEFINE_CID(	kCommonDialogsCID, NS_CommonDialog_CID );
	NS_WITH_SERVICE(nsICommonDialogs, dialog, kCommonDialogsCID, &rv);
	 if ( NS_SUCCEEDED( rv ) )
	 {
 		rv = dialog->DoDialog( inParent, block, "chrome://global/content/selectDialog.xul" );
	
		PRInt32 buttonPressed = 0;
		block->GetInt( nsICommonDialogs::eButtonPressed, &buttonPressed );
		block->GetInt( eSelection, &ioCount );
		*_retval = buttonPressed ? PR_FALSE : PR_TRUE;
		NS_IF_RELEASE( block );
	}
	return rv;
}
#endif 


class nsCommonDialogs: public nsICommonDialogs
{
public:
			nsCommonDialogs();
  virtual	~nsCommonDialogs();

  NS_DECL_NSICOMMONDIALOGS
  NS_DECL_ISUPPORTS
private:
};

const char* kPromptURL="chrome://global/content/commonDialog.xul";

const char* kQuestionIconURL ="chrome://global/skin/question-icon.gif";
const char* kAlertIconURL ="chrome://global/skin/alert-icon.gif";
const char* kWarningIconURL ="chrome://global/skin/message-icon.gif";

nsCommonDialogs::nsCommonDialogs()
{
	NS_INIT_REFCNT();
}

nsCommonDialogs::~nsCommonDialogs()
{
}

NS_IMETHODIMP nsCommonDialogs::Alert(nsIDOMWindow *inParent,  const PRUnichar *inWindowTitle, const PRUnichar *inMsg)
{
	nsresult rv;
	nsIDialogParamBlock* block = NULL;
	rv = nsComponentManager::CreateInstance( kDialogParamBlockCID,
                                                      0,
                                                      nsIDialogParamBlock::GetIID(),
                                                      (void**)&block );
      
	if ( NS_FAILED( rv ) )
		return rv;
	// Stuff in Parameters
	block->SetInt( eNumberButtons, 1 );
	block->SetString( eMsg, inMsg );

	block->SetString( eDialogTitle,inWindowTitle );
	nsString url( kAlertIconURL );
	block->SetString( eIconURL, url.GetUnicode());
	
	rv = DoDialog( inParent, block, kPromptURL );
	
	NS_IF_RELEASE( block );
	return rv;
}

NS_IMETHODIMP nsCommonDialogs::Confirm(nsIDOMWindow *inParent, const PRUnichar *inWindowTitle, const PRUnichar *inMsg, PRBool *_retval)
{
	nsresult rv;
	nsIDialogParamBlock* block = NULL;
	rv = nsComponentManager::CreateInstance( kDialogParamBlockCID,
                                                      0,
                                                      nsIDialogParamBlock::GetIID(),
                                                      (void**)&block );
      
	if ( NS_FAILED( rv ) )
		return rv;
	// Stuff in Parameters
	block->SetInt( eNumberButtons,2 );
	block->SetString( eMsg, inMsg );
	block->SetString( eDialogTitle, inWindowTitle );
	nsString url( kQuestionIconURL );
	block->SetString( eIconURL, url.GetUnicode());
	
	rv = DoDialog( inParent, block, kPromptURL );
	
	PRInt32 buttonPressed = 0;
	block->GetInt( eButtonPressed, &buttonPressed );
	*_retval = buttonPressed ? PR_FALSE : PR_TRUE;
	NS_IF_RELEASE( block );
	return rv;
}

NS_IMETHODIMP nsCommonDialogs::ConfirmCheck(nsIDOMWindow *inParent,  const PRUnichar *inWindowTitle,const PRUnichar *inMsg, const PRUnichar *inCheckMsg, PRBool *outCheckValue, PRBool *_retval)
{
	nsresult rv;
	nsIDialogParamBlock* block = NULL;
	rv = nsComponentManager::CreateInstance( kDialogParamBlockCID,
                                                      0,
                                                      nsIDialogParamBlock::GetIID(),
                                                      (void**)&block );
      
	if ( NS_FAILED( rv ) )
		return rv;
	// Stuff in Parameters
	block->SetInt( eNumberButtons,2 );
	block->SetString( eMsg, inMsg );
	block->SetString( eDialogTitle, inWindowTitle );
	nsString url( kQuestionIconURL );
	block->SetString( eIconURL, url.GetUnicode());
	block->SetString( eCheckboxMsg, inCheckMsg );
	block->SetInt(eCheckboxState, *outCheckValue );
	
	rv = DoDialog( inParent, block, kPromptURL );
	PRInt32 tempInt = 0;
	block->GetInt( eButtonPressed, &tempInt );
	*_retval = tempInt ? PR_FALSE : PR_TRUE;
	
	block->GetInt(eCheckboxState, & tempInt  );
	*outCheckValue = PRBool( tempInt );
	
	NS_IF_RELEASE( block );
	return rv;
}

NS_IMETHODIMP nsCommonDialogs::Prompt(nsIDOMWindow *inParent, const PRUnichar *inWindowTitle, const PRUnichar *inMsg, const PRUnichar *inDefaultText, PRUnichar **result, PRBool *_retval)
{
	nsresult rv;
	nsIDialogParamBlock* block = NULL;
	rv = nsComponentManager::CreateInstance( kDialogParamBlockCID,
                                                      0,
                                                      nsIDialogParamBlock::GetIID(),
                                                      (void**)&block );
      
	if ( NS_FAILED( rv ) )
		return rv;
	// Stuff in Parameters
	block->SetInt( eNumberButtons,2 );
	block->SetString( eMsg, inMsg );
	block->SetString( eDialogTitle, inWindowTitle );
	nsString url( kQuestionIconURL );
	block->SetString( eIconURL, url.GetUnicode());
	block->SetInt( eNumberEditfields, 1 );
	block->SetString( eEditfield1Value, inDefaultText );
	
	rv = DoDialog( inParent, block, kPromptURL );
	
	
	block->GetString( eEditfield1Value, result );
	PRInt32 tempInt = 0;
	block->GetInt( eButtonPressed, &tempInt );
	*_retval = tempInt ? PR_FALSE : PR_TRUE;
	
	NS_IF_RELEASE( block );
	return rv;
}

NS_IMETHODIMP nsCommonDialogs::PromptUsernameAndPassword(nsIDOMWindow *inParent, const PRUnichar *inWindowTitle, const PRUnichar *inMsg, PRUnichar **outUser, PRUnichar **outPassword, PRBool *_retval)
{
	nsresult rv;
	nsIDialogParamBlock* block = NULL;
	rv = nsComponentManager::CreateInstance( kDialogParamBlockCID,
                                                      0,
                                                      nsIDialogParamBlock::GetIID(),
                                                      (void**)&block );
      
	if ( NS_FAILED( rv ) )
		return rv;
	// Stuff in Parameters
	block->SetInt( eNumberButtons,2 );
	block->SetString( eMsg, inMsg );
	block->SetString( eDialogTitle, inWindowTitle );
	nsString url( kQuestionIconURL );
	block->SetString( eIconURL, url.GetUnicode());
	block->SetInt( eNumberEditfields, 2 );
	block->SetString( eEditfield1Value, *outUser );
	block->SetString( eEditfield2Value, *outPassword );
	
	
	rv = DoDialog( inParent, block, kPromptURL );
	
	block->GetString( eEditfield1Value, outUser );
	block->GetString( eEditfield2Value, outPassword );
	PRInt32 tempInt = 0;
	block->GetInt( eButtonPressed, &tempInt );
	*_retval = tempInt ? PR_FALSE : PR_TRUE;
	NS_IF_RELEASE( block );
	return rv;
}

NS_IMETHODIMP nsCommonDialogs::PromptPassword(nsIDOMWindow *inParent,  const PRUnichar *inWindowTitle, const PRUnichar *inMsg, PRUnichar **outPassword, PRBool *_retval)
{	
	nsresult rv;
	nsIDialogParamBlock* block = NULL;
	rv = nsComponentManager::CreateInstance( kDialogParamBlockCID,
                                                      0,
                                                      nsIDialogParamBlock::GetIID(),
                                                      (void**)&block );
      
	if ( NS_FAILED( rv ) )
		return rv;
	// Stuff in Parameters
	block->SetInt( eNumberButtons,2 );
	block->SetString( eMsg, inMsg );
	block->SetString( eDialogTitle, inWindowTitle );
	nsString url( kQuestionIconURL );
	block->SetString( eIconURL, url.GetUnicode());
	block->SetInt( eNumberEditfields, 1 );
	block->SetInt( eEditField1Password, 1 );
	rv = DoDialog( inParent, block, kPromptURL );
	block->GetString( eEditfield2Value, outPassword );
	PRInt32 tempInt = 0;
	block->GetInt( eButtonPressed, &tempInt );
	*_retval = tempInt ? PR_FALSE : PR_TRUE;
	
	NS_IF_RELEASE( block );
	return rv;
}


 NS_IMETHODIMP nsCommonDialogs::DoDialog(nsIDOMWindow* inParent, nsIDialogParamBlock *ioParamBlock, const char *inChromeURL)
 {
  nsresult rv = NS_OK;

    if ( inParent && ioParamBlock &&inChromeURL )
    {
        // Get JS context from parent window.
        nsCOMPtr<nsIScriptGlobalObject> sgo = do_QueryInterface( inParent, &rv );
        if ( NS_SUCCEEDED( rv ) && sgo )
        {
            nsCOMPtr<nsIScriptContext> context;
            sgo->GetContext( getter_AddRefs( context ) );
            if ( context )
            {
                JSContext *jsContext = (JSContext*)context->GetNativeContext();
                if ( jsContext ) {
                    void *stackPtr;
                    jsval *argv = JS_PushArguments( jsContext,
                                                    &stackPtr,
                                                    "svs%ip",
                                                    inChromeURL,
                                                    JSVAL_NULL,
                                                    "chrome,modal",
                                                    (const nsIID*)(&nsIDialogParamBlock::GetIID()),
                                                    (nsISupports*)ioParamBlock
                                                  );
                    if ( argv ) {
                        nsIDOMWindow *newWindow;
                        rv = inParent->OpenDialog( jsContext, argv, 4, &newWindow );
                        if ( NS_SUCCEEDED( rv ) )
                        {
    //                        newWindow->Release();
                        } else
                        {
                        }
                        JS_PopArguments( jsContext, stackPtr );
                    }
                    else
                    {
                    	
                        NS_WARNING( "JS_PushArguments failed\n" );
                        rv = NS_ERROR_FAILURE;
                    }
                }
                else
                {
                    NS_WARNING(" GetNativeContext failed\n" );
                    rv = NS_ERROR_FAILURE;
                }
            }
            else
            {
                NS_WARNING( "GetContext failed\n" );
                rv = NS_ERROR_FAILURE;
            }
        }
        else
        {
            NS_WARNING( "QueryInterface (for nsIScriptGlobalObject) failed \n" );
        }
    }
    else
    {
        NS_WARNING( " OpenDialogWithArg was passed a null pointer!\n" );
        rv = NS_ERROR_NULL_POINTER;
    }
    return rv;
 }
 
 
 static NS_DEFINE_IID(kICommonDialogs, nsICommonDialogs::GetIID() );
NS_IMPL_ADDREF(nsCommonDialogs);
NS_IMPL_RELEASE(nsCommonDialogs);
NS_IMPL_QUERY_INTERFACE(nsCommonDialogs, kICommonDialogs);

// Entry point to create nsAppShellService factory instances...
NS_DEF_FACTORY(CommonDialogs, nsCommonDialogs)

nsresult NS_NewCommonDialogsFactory(nsIFactory** aResult)
{
  nsresult rv = NS_OK;
  nsIFactory* inst;
  
  inst = new nsCommonDialogsFactory;
  if (nsnull == inst)
  {
    rv = NS_ERROR_OUT_OF_MEMORY;
  }
  else
  {
    NS_ADDREF(inst);
  }
  *aResult = inst;
  return rv;
}

