/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
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
 *   Pierre Phaneuf <pp@ludusdesign.com>
 */
#include "nsIUnkContentTypeHandler.h"
#include "nsIHelperAppLauncherDialog.h"

#include "nsString.h"
#include "nsIDOMWindowInternal.h"
#include "nsIScriptGlobalObject.h"
#include "nsIChannel.h"
#include "nsIURI.h"
#include "nsIRequestObserver.h"
#include "nsIHTTPChannel.h"
#include "nsXPIDLString.h"
#include "nsIInterfaceRequestor.h"
#include "nsIExternalHelperAppService.h"
#include "nsIStringBundle.h"
#include "nsIFilePicker.h"
#include "nsIPref.h"

#include "nsIGenericFactory.h"

static NS_DEFINE_CID(kStringBundleServiceCID, NS_STRINGBUNDLESERVICE_CID);
#define HELPERAPP_DIALOG_URL       "chrome://global/locale/helperAppLauncher.properties"

// {42770B50-03E9-11d3-8068-00600811A9C3}
#define NS_UNKNOWNCONTENTTYPEHANDLER_CID \
    { 0x42770b50, 0x3e9, 0x11d3, { 0x80, 0x68, 0x0, 0x60, 0x8, 0x11, 0xa9, 0xc3 } }

class nsUnknownContentTypeHandler : public nsIUnknownContentTypeHandler,
                                    public nsIHelperAppLauncherDialog {
public:
    NS_DEFINE_STATIC_CID_ACCESSOR( NS_UNKNOWNCONTENTTYPEHANDLER_CID );

    // ctor/dtor
    nsUnknownContentTypeHandler() {
        NS_INIT_REFCNT();
    }
    virtual ~nsUnknownContentTypeHandler() {
    }

    // This class implements the nsISupports interface functions.
    NS_DECL_ISUPPORTS

    // This class implements the nsIUnknownContentTypeHandler interface functions.
    NS_DECL_NSIUNKNOWNCONTENTTYPEHANDLER

    // This class implements the nsIHelperAppLauncherDialog interface functions.
    NS_DECL_NSIHELPERAPPLAUNCHERDIALOG

}; // nsUnknownContentTypeHandler

// HandleUnknownContentType (from nsIUnknownContentTypeHandler) implementation.
// XXX We can get the content type from the channel now so that arg could be dropped.
NS_IMETHODIMP
nsUnknownContentTypeHandler::HandleUnknownContentType( nsIRequest *request,
                                                       const char *aContentType,
                                                       nsIDOMWindowInternal *aWindow ) {
    nsresult rv = NS_OK;

    nsCOMPtr<nsIChannel> aChannel;
    nsCOMPtr<nsISupports> channel;
    nsCAutoString         contentDisp;
    

    if ( request ) {
        
      aChannel = do_QueryInterface(request);

        // Need root nsISupports for later JS_PushArguments call.
        channel = do_QueryInterface( aChannel );

        // Try to get HTTP channel.
        nsCOMPtr<nsIHTTPChannel> httpChannel = do_QueryInterface( aChannel );
        if ( httpChannel ) {
            // Get content-disposition response header.
            nsCOMPtr<nsIAtom> atom = dont_AddRef(NS_NewAtom( "content-disposition" ));
            if ( atom ) {
                nsXPIDLCString disp; 
                rv = httpChannel->GetResponseHeader( atom, getter_Copies( disp ) );
                if ( NS_SUCCEEDED( rv ) && disp ) {
                    contentDisp = disp; // Save the response header to pass to dialog.
                }
            }
        }

        // Cancel input channel now.
        rv = request->Cancel(NS_BINDING_ABORTED);
        if ( NS_FAILED( rv ) ) {
          NS_WARNING("Cancel failed");
        }
    }

    if ( NS_SUCCEEDED( rv ) && channel && aContentType && aWindow ) {
        // Open "Unknown content type" dialog.
        // We pass in the channel, the content type, and the content disposition.
        // Note that the "parent" browser window will be window.opener within the
        // new dialog.
    
        // Get JS context from parent window.
        nsCOMPtr<nsIScriptGlobalObject> sgo = do_QueryInterface( aWindow, &rv );
        if ( NS_SUCCEEDED( rv ) && sgo ) {
            nsCOMPtr<nsIScriptContext> context;
            sgo->GetContext( getter_AddRefs( context ) );
            if ( context ) {
                JSContext *jsContext = (JSContext*)context->GetNativeContext();
                if ( jsContext ) {
                    void *stackPtr;
                    jsval *argv = JS_PushArguments( jsContext,
                                                    &stackPtr,
                                                    "sss%ipss",
                                                    "chrome://global/content/unknownContent.xul",
                                                    "_blank",
                                                    "chrome,titlebar",
                                                    (const nsIID*)(&NS_GET_IID(nsIChannel)),
                                                    (nsISupports*)channel.get(),
                                                    aContentType,
                                                    contentDisp.get() );
                    if ( argv ) {
                        nsCOMPtr<nsIDOMWindowInternal> newWindow;
                        rv = aWindow->OpenDialog( jsContext, argv, 6, getter_AddRefs( newWindow ) );
                        NS_ASSERTION(NS_SUCCEEDED(rv), "OpenDialog failed");
                        JS_PopArguments( jsContext, stackPtr );
                    } else {
                        NS_ASSERTION(0, "JS_PushArguments failed");
                        rv = NS_ERROR_FAILURE;
                    }
                } else {
                    NS_ASSERTION(0, "GetNativeContext failed");
                    rv = NS_ERROR_FAILURE;
                }
            } else {
                NS_ASSERTION(0, "GetContext failed");
                rv = NS_ERROR_FAILURE;
            }
        } else {
            NS_ASSERTION(0, "QueryInterface (for nsIScriptGlobalObject) failed");
        }
    } else {
        // If no error recorded so far, set one now.
        if ( NS_SUCCEEDED( rv ) ) {
            rv = NS_ERROR_NULL_POINTER;
        }
    }

    return rv;
}

NS_IMETHODIMP
nsUnknownContentTypeHandler::ShowProgressDialog(nsIHelperAppLauncher *aLauncher, nsISupports *aContext ) {
    nsresult rv = NS_ERROR_FAILURE;

    // Get parent window (from context).
    nsCOMPtr<nsIDOMWindowInternal> parent( do_GetInterface( aContext ) );
    if ( parent ) {
        // Get JS context from parent window.
        nsCOMPtr<nsIScriptGlobalObject> sgo = do_QueryInterface( parent, &rv );
        if ( NS_SUCCEEDED( rv ) && sgo ) {
            nsCOMPtr<nsIScriptContext> context;
            sgo->GetContext( getter_AddRefs( context ) );
            if ( context ) {
                // Get native context.
                JSContext *jsContext = (JSContext*)context->GetNativeContext();
                if ( jsContext ) {
                    // Set up window.arguments[0]...
                    void *stackPtr;
                    jsval *argv = JS_PushArguments( jsContext,
                                                    &stackPtr,
                                                    "sss%ip",
                                                    "chrome://global/content/helperAppDldProgress.xul",
                                                    "_blank",
                                                    "chrome,titlebar,minimizable",
                                                    (const nsIID*)(&NS_GET_IID(nsIHelperAppLauncher)),
                                                    (nsISupports*)aLauncher );
                    if ( argv ) {
                        // Open the dialog.
                        nsCOMPtr<nsIDOMWindowInternal> dialog;
                        rv = parent->OpenDialog( jsContext, argv, 4, getter_AddRefs( dialog ) );
                        // Pop arguments.
                        JS_PopArguments( jsContext, stackPtr );
                    }
                }
            }
        }
    }
    return rv;
}

// Show the helper app launch confirmation dialog as instructed.
NS_IMETHODIMP
nsUnknownContentTypeHandler::Show( nsIHelperAppLauncher *aLauncher, nsISupports *aContext ) {
    nsresult rv = NS_ERROR_FAILURE;

    // Get parent window (from context).
    nsCOMPtr<nsIDOMWindowInternal> parent( do_GetInterface( aContext ) );
    if ( parent ) {
        // Get JS context from parent window.
        nsCOMPtr<nsIScriptGlobalObject> sgo = do_QueryInterface( parent, &rv );
        if ( NS_SUCCEEDED( rv ) && sgo ) {
            nsCOMPtr<nsIScriptContext> context;
            sgo->GetContext( getter_AddRefs( context ) );
            if ( context ) {
                // Get native context.
                JSContext *jsContext = (JSContext*)context->GetNativeContext();
                if ( jsContext ) {
                    // Set up window.arguments[0]...
                    void *stackPtr;
                    jsval *argv = JS_PushArguments( jsContext,
                                                    &stackPtr,
                                                    "sss%ip",
                                                    "chrome://global/content/helperAppLauncher.xul",
                                                    "_blank",
                                                    "chrome,titlebar",
                                                    (const nsIID*)(&NS_GET_IID(nsIHelperAppLauncher)),
                                                    (nsISupports*)aLauncher );
                    if ( argv ) {
                        // Open the dialog.
                        nsCOMPtr<nsIDOMWindowInternal> dialog;
                        rv = parent->OpenDialog( jsContext, argv, 4, getter_AddRefs( dialog ) );
                        // Pop arguments.
                        JS_PopArguments( jsContext, stackPtr );
                    }
                }
            }
        }
    }
    return rv;
}

// prompt the user for a file name to save the unknown content to as instructed
NS_IMETHODIMP
nsUnknownContentTypeHandler::PromptForSaveToFile(nsISupports * aWindowContext, const PRUnichar * aDefaultFile, const PRUnichar * aSuggestedFileExtension, nsILocalFile ** aNewFile)
{
  nsresult rv = NS_OK;
  nsCOMPtr<nsIFilePicker> filePicker = do_CreateInstance("@mozilla.org/filepicker;1", &rv);
  if (filePicker)
  {
    nsCOMPtr<nsIStringBundleService> stringService = do_GetService(kStringBundleServiceCID);
    nsCOMPtr<nsIStringBundle> stringBundle;
    NS_ENSURE_TRUE(stringService, NS_ERROR_FAILURE);

    NS_ENSURE_SUCCESS(stringService->CreateBundle(HELPERAPP_DIALOG_URL, nsnull, getter_AddRefs(stringBundle)), 
                    NS_ERROR_FAILURE);

    nsXPIDLString windowTitle;
    stringBundle->GetStringFromName(NS_LITERAL_STRING("saveDialogTitle").get(), getter_Copies(windowTitle));

    nsCOMPtr<nsIDOMWindowInternal> parent( do_GetInterface( aWindowContext ) );
    filePicker->Init(parent, windowTitle, nsIFilePicker::modeSave);
    filePicker->SetDefaultString(aDefaultFile);
    nsAutoString wildCardExtension (NS_LITERAL_STRING("*").get());
    if (aSuggestedFileExtension) {
      wildCardExtension.Append(aSuggestedFileExtension);
      filePicker->AppendFilter(wildCardExtension.GetUnicode(), wildCardExtension.GetUnicode());
    }

    filePicker->AppendFilters(nsIFilePicker::filterAll);

    nsCOMPtr<nsILocalFile> startDir;
    // Pull in the user's preferences and get the default download directory.
    nsCOMPtr<nsIPref> prefs (do_GetService(NS_PREF_CONTRACTID));
    if ( prefs ) 
    {
      rv = prefs->GetFileXPref( "browser.download.dir", getter_AddRefs( startDir ) );
      if ( NS_SUCCEEDED(rv) && startDir ) 
      {
        PRBool isValid = PR_FALSE;
        startDir->Exists( &isValid );
        if ( isValid )  // Set file picker so startDir is used.
          filePicker->SetDisplayDirectory( startDir );
      }
    }


    PRInt16 dialogResult;
    filePicker->Show(&dialogResult);
    if (dialogResult == nsIFilePicker::returnCancel)
      rv = NS_ERROR_FAILURE;
    else          
    {
      // be sure to save the directory the user chose as the new browser.download.dir
      rv = filePicker->GetFile(aNewFile);
      if (*aNewFile)
      {
        nsCOMPtr<nsIFile> newDirectory;
        (*aNewFile)->GetParent(getter_AddRefs(newDirectory));
        nsCOMPtr<nsILocalFile> newLocalDirectory (do_QueryInterface(newDirectory));

        if (newLocalDirectory)
         prefs->SetFileXPref( "browser.download.dir", newLocalDirectory);
      }

    }
  }

  return rv;
}

NS_GENERIC_FACTORY_CONSTRUCTOR(nsUnknownContentTypeHandler)

static nsModuleComponentInfo components[] = {
  { NS_IUNKNOWNCONTENTTYPEHANDLER_CLASSNAME, 
    NS_UNKNOWNCONTENTTYPEHANDLER_CID, 
    NS_IUNKNOWNCONTENTTYPEHANDLER_CONTRACTID,
    nsUnknownContentTypeHandlerConstructor },
  { NS_IHELPERAPPLAUNCHERDLG_CLASSNAME, 
    NS_UNKNOWNCONTENTTYPEHANDLER_CID, 
    NS_IHELPERAPPLAUNCHERDLG_CONTRACTID, 
    nsUnknownContentTypeHandlerConstructor },
};

NS_IMPL_NSGETMODULE(nsUnknownContentTypeHandler, components )

/* nsISupports Implementation for the class */
NS_IMPL_ISUPPORTS2(nsUnknownContentTypeHandler,
                   nsIUnknownContentTypeHandler,
                   nsIHelperAppLauncherDialog)

