/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: NPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is 
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Daniel Glazman <glazman@netscape.com>
 *
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the NPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "nsEditorShell.h"
#include "nsIPlaintextEditor.h"
#include "nsIBaseWindow.h"
#include "nsIContentViewerFile.h"
#include "prprf.h"

#include "nsIFocusController.h"
#include "nsIScriptContext.h"
#include "nsIScriptGlobalObject.h"
#include "nsIDOMDocument.h"
#include "nsIMarkupDocumentViewer.h"
#include "nsIDOMHTMLDocument.h"
#include "nsIDocument.h"
#include "nsIDOMWindowInternal.h"
#include "nsPIDOMWindow.h"
#include "nsIDOMNode.h"
#include "nsIDOMNodeList.h"
#include "nsICSSStyleSheet.h"
#include "nsIStyleSheet.h"
#include "nsIURI.h"
#include "nsIFileURL.h"
#include "nsNetUtil.h"

#include "nsIWebNavigation.h"
#include "nsCOMPtr.h"

#include "nsIURL.h"
#include "nsXPIDLString.h"
#include "nsReadableUtils.h"
#include "nsUnicharUtils.h"
#include "nsCRT.h"

#include "nsIDocumentViewer.h"
#include "nsIPresShell.h"
#include "nsIPresContext.h"
#include "nsISelection.h"
#include "nsISelectionPrivate.h"
#include "nsISelectionController.h"

#include "nsIPromptService.h"

#include "imgIContainer.h"

#include "nsIEditorController.h"
#include "nsIControllers.h"
#include "nsIDocShell.h"
#include "nsIDocShellTreeItem.h"
#include "nsIDocShellTreeOwner.h"
#include "nsITransactionManager.h"

#include "nsIRefreshURI.h"
#include "nsIPref.h"
#include "nsILookAndFeel.h"
#include "nsIChromeRegistry.h"

///////////////////////////////////////
// Editor Includes
///////////////////////////////////////
#include "nsString.h"
#include "nsIDOMElement.h"
#include "nsIWebProgress.h"

#include "nsIEditor.h"
#include "nsIHTMLEditor.h"
#include "nsIEditorStyleSheets.h"
#include "nsIEditorMailSupport.h"
#include "nsITableEditor.h"
#include "nsIEditorLogging.h"

#include "nsEditorCID.h"

#include "nsTextServicesCID.h"
#include "nsITextServicesDocument.h"
#include "nsISpellChecker.h"
#include "nsInterfaceState.h"

#include "nsIStringBundle.h"

#include "nsHTMLTags.h"
#include "nsEditorParserObserver.h"
#include "nsIDOMEventReceiver.h"
#include "nsIWebBrowserPrint.h"

///////////////////////////////////////

// Drag & Drop, Clipboard
#include "nsWidgetsCID.h"

#include "nsISupportsArray.h"

/* Define Class IDs */
static NS_DEFINE_CID(kHTMLEditorCID,            NS_HTMLEDITOR_CID);
static NS_DEFINE_CID(kCTextServicesDocumentCID, NS_TEXTSERVICESDOCUMENT_CID);
static NS_DEFINE_CID(kCStringBundleServiceCID,  NS_STRINGBUNDLESERVICE_CID);
static NS_DEFINE_CID(kPrefServiceCID,           NS_PREF_CID);

#define APP_DEBUG 0 

#define EDITOR_BUNDLE_URL "chrome://editor/locale/editor.properties"
#define EDITOR_DEFAULT_DIR_PREF "editor.default.dir"

enum {
  eEditorController,
  eComposerController
};

static eHTMLTags gWatchTags[] = 
{ eHTMLTag_frameset,
  eHTMLTag_iframe,
  eHTMLTag_unknown
};

/////////////////////////////////////////////////////////////////////////
// Utility to extract document from a webshell object.
static nsresult
GetDocument(nsIDocShell *aDocShell, nsIDocument **aDoc ) 
{
  // Get content viewer from the web shell.
  nsCOMPtr<nsIContentViewer> contentViewer;
  nsresult res = (aDocShell && aDoc) ? 
   aDocShell->GetContentViewer(getter_AddRefs(contentViewer))
                   : NS_ERROR_NULL_POINTER;

  if ( NS_SUCCEEDED(res) && contentViewer )
  {
    // Up-cast to a document viewer.
    nsCOMPtr<nsIDocumentViewer> docViewer(do_QueryInterface(contentViewer));
    if ( docViewer )
    {
      // Get the document from the doc viewer.
      res = docViewer->GetDocument(*aDoc);
    }
  }
  return res;
}

// Utility get a UI element
static nsresult 
GetChromeElement(nsIDocShell *aShell, const char *aID, nsIDOMElement **aElement)
{
  if (!aElement) return NS_ERROR_NULL_POINTER;
  *aElement = nsnull;

  nsCOMPtr<nsIDocument> doc;
  nsresult rv = GetDocument( aShell, getter_AddRefs(doc) );
  if(NS_SUCCEEDED(rv) && doc)
  {
    nsCOMPtr<nsIDOMDocument> dDoc( do_QueryInterface(doc) );
    if ( dDoc )
    {
        nsCOMPtr<nsIDOMElement> elem;
        rv = dDoc->GetElementById( NS_ConvertASCIItoUCS2(aID), getter_AddRefs(elem) );
        if (elem)
        {
            *aElement = elem.get();
            NS_ADDREF(*aElement);
        }
    }
  }
  return rv;
}

// Utility to set and attribute of a UI element
static nsresult 
SetChromeAttribute(nsIDocShell *aShell, const char *aID, 
                    const nsAString& aName,  const nsAString &aValue)
{
  nsCOMPtr<nsIDOMElement> elem;
  nsresult rv = GetChromeElement(aShell, aID, getter_AddRefs(elem));

  // Set the text attribute.
  if (NS_SUCCEEDED(rv) && elem)
    rv = elem->SetAttribute(aName, aValue);

  return rv;
}

// Utility to get the treeOwner for a docShell
static nsresult
GetTreeOwner(nsIDocShell* aDocShell, nsIBaseWindow** aBaseWindow)
{
   nsCOMPtr<nsIDocShellTreeItem> docShellAsItem(do_QueryInterface(aDocShell));
   NS_ENSURE_TRUE(docShellAsItem, NS_ERROR_FAILURE);

   nsCOMPtr<nsIDocShellTreeOwner> treeOwner;
   docShellAsItem->GetTreeOwner(getter_AddRefs(treeOwner));
   NS_ENSURE_TRUE(treeOwner, NS_ERROR_FAILURE);

   NS_ENSURE_SUCCESS(CallQueryInterface(treeOwner, aBaseWindow), 
      NS_ERROR_FAILURE);

   return NS_OK;
}

/////////////////////////////////////////////////////////////////////////
// nsEditorShell
/////////////////////////////////////////////////////////////////////////

nsEditorShell::nsEditorShell()
:  mMailCompose(PR_FALSE)
,  mWebShellWindow(nsnull)
,  mContentWindow(nsnull)
,  mParserObserver(nsnull)
,  mStateMaintainer(nsnull)
,  mEditorController(nsnull)
,  mComposerController(nsnull)
,  mDocShell(nsnull)
,  mContentAreaDocShell(nsnull)
,  mCloseWindowWhenLoaded(PR_FALSE)
,  mCantEditReason(eCantEditNoReason)
,  mEditorType(eUninitializedEditorType)
,  mEditorTypeString(NS_LITERAL_STRING("html"))
,  mContentMIMEType("text/html")
,  mContentTypeKnown(PR_FALSE)
,  mWrapColumn(0)
,  mSuggestedWordIndex(0)
,  mDictionaryIndex(0)
{
  //TODO:Save last-used display mode in prefs so new window inherits?
  NS_INIT_ISUPPORTS();
}

nsEditorShell::~nsEditorShell()
{
  NS_IF_RELEASE(mStateMaintainer);
  NS_IF_RELEASE(mParserObserver);
  
  // the only other references we hold are in nsCOMPtrs, so they'll take
  // care of themselves.
}

NS_IMPL_ISUPPORTS5(nsEditorShell, 
                   nsIEditorShell, 
                   nsIWebProgressListener, 
                   nsIURIContentListener, 
                   nsIEditorSpellCheck, 
                   nsISupportsWeakReference);

NS_IMETHODIMP    
nsEditorShell::Shutdown()
{
  nsresult rv = NS_OK;
  
  nsCOMPtr<nsIEditor> editor(do_QueryInterface(mEditor));
  if (editor)
  {
    editor->PreDestroy();
  }

  // Make sure we blow the spellchecker away, just in
  // case it hasn't been destroyed already.
  mSpellChecker = nsnull;
  
  if (mDocShell)
    mDocShell->SetParentURIContentListener(nsnull);

  // Remove our WebProgress listener...
  nsCOMPtr<nsIWebProgress> webProgress;
  if (mContentAreaDocShell) {
    webProgress = do_GetInterface(mContentAreaDocShell);
    if (webProgress) {
      (void) webProgress->RemoveProgressListener(this);
    }
  }
  return rv;
}

nsresult
nsEditorShell::ResetEditingState()
{
  if (!mEditor) return NS_OK;   // nothing to do

  // Mmm, we have an editor already. That means that someone loaded more than
  // one URL into the content area. Let's tear down what we have, and rip 'em a
  // new one.

  nsCOMPtr<nsIEditor> editor(do_QueryInterface(mEditor));
  if (editor)
  {
    editor->PreDestroy();
  }

  nsresult rv;  
  // now, unregister the selection listener, if there was one
  if (mStateMaintainer)
  {
    nsCOMPtr<nsISelection> domSelection;
    // using a scoped result, because we don't really care if this fails
    rv = GetEditorSelection(getter_AddRefs(domSelection));
    if (NS_SUCCEEDED(rv) && domSelection)
    {
      nsCOMPtr<nsISelectionPrivate> selPriv(do_QueryInterface(domSelection));
      selPriv->RemoveSelectionListener(mStateMaintainer);
      NS_IF_RELEASE(mStateMaintainer);
    }
  }

  // clear this editor out of the controller
  if (mEditorController)
  {
    mEditorController->SetCommandRefCon(nsnull);
  }

  if (mComposerController)
  {
    mComposerController->SetCommandRefCon(nsnull);
  }

  mEditorType = eUninitializedEditorType;
  mEditor = 0;  // clear out the nsCOMPtr

  // and tell them that they are doing bad things
  NS_WARNING("Multiple loads of the editor's document detected.");
  // Note that if you registered doc state listeners before the second
  // URL load, they don't get transferred to the new editor.

  return NS_OK;
}


// is this a MIME type that we support the editing of, in plain text mode?
const char* const gSupportedTextTypes[] = {
  "text/plain",
  "text/css",
  "text/rdf",
  "text/xml",
  "text/xsl",
  "text/javascript",    // obsolete type
  "application/x-javascript",
  "text/xul",           // obsolete type
  "application/vnd.mozilla.xul+xml",
  NULL      // IMPORTANT! Null must be at end
};

NS_IMETHODIMP
nsEditorShell::IsSupportedTextType(const char* aMIMEType, PRBool *aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);
  *aResult = PR_FALSE;
  NS_ENSURE_ARG_POINTER(aMIMEType);

  PRInt32 i = 0;
  while (gSupportedTextTypes[i])
  {
    if (nsCRT::strcmp(gSupportedTextTypes[i], aMIMEType) == 0)
    {
      *aResult = PR_TRUE;
      return NS_OK;
    }

    i ++;
  }
  
  return NS_OK;
}

nsresult    
nsEditorShell::PrepareDocumentForEditing(nsIDOMWindow* aDOMWindow, nsIURI *aUrl)
{
  if (!mContentAreaDocShell || !mContentWindow)
    return NS_ERROR_NOT_INITIALIZED;

  NS_ASSERTION(!mEditor, "Should never have an editor here");

  // get the docshell for this DOM Window. Need this, not mContentAreaDocShell, in
  // case we are editing a frameset
  nsCOMPtr<nsIScriptGlobalObject> scriptObject(do_QueryInterface(aDOMWindow));
  nsCOMPtr<nsIDocShell> docshell;

  if (scriptObject) {
    scriptObject->GetDocShell(getter_AddRefs(docshell));
  }
  if (!docshell)
  {
    NS_ASSERTION(0, "Failed to get docShell from DOMWindow");
    return NS_ERROR_UNEXPECTED;
  }
  
  // turn off animated GIFs
  nsCOMPtr<nsIPresContext> presContext;
  docshell->GetPresContext(getter_AddRefs(presContext));
  if (presContext)
    presContext->SetImageAnimationMode(imgIContainer::kDontAnimMode);

  nsresult rv = DoEditorMode(docshell);
  if (NS_FAILED(rv)) return rv;
  
  // transfer the doc state listeners to the editor
  rv = TransferDocumentStateListeners();
  if (NS_FAILED(rv)) return rv;
  
  // make the UI state maintainer
  NS_NEWXPCOM(mStateMaintainer, nsInterfaceState);
  if (!mStateMaintainer) return NS_ERROR_OUT_OF_MEMORY;
  mStateMaintainer->AddRef();      // the owning reference

  // get the Doc from the webshell
  nsCOMPtr<nsIContentViewer> cv;
  rv = mDocShell->GetContentViewer(getter_AddRefs(cv));
  if (NS_FAILED(rv)) return rv;
    
  nsCOMPtr<nsIDocumentViewer> docViewer = do_QueryInterface(cv, &rv);
  if (NS_FAILED(rv)) return rv;

  nsCOMPtr<nsIDocument> chromeDoc;
  rv = docViewer->GetDocument(*getter_AddRefs(chromeDoc));
  if (NS_FAILED(rv)) return rv;
  
  nsCOMPtr<nsIDOMDocument> dDoc = do_QueryInterface(chromeDoc, &rv);
  if (NS_FAILED(rv)) return rv;


  // now init the state maintainer
  rv = mStateMaintainer->Init(mEditor, dDoc);
  if (NS_FAILED(rv)) return rv;
  
  // set it up as a selection listener
  nsCOMPtr<nsISelection> domSelection;
  rv = GetEditorSelection(getter_AddRefs(domSelection));
  if (NS_FAILED(rv)) return rv;

  nsCOMPtr<nsISelectionPrivate> selPriv(do_QueryInterface(domSelection));
  rv = selPriv->AddSelectionListener(mStateMaintainer);
  if (NS_FAILED(rv)) return rv;

  // and set it up as a doc state listener
  nsCOMPtr<nsIEditor> editor = do_QueryInterface(mEditor, &rv);
  if (NS_FAILED(rv)) return rv;
  rv = editor->AddDocumentStateListener(NS_STATIC_CAST(nsIDocumentStateListener*, mStateMaintainer));
  if (NS_FAILED(rv)) return rv;
  
  // and as a transaction listener
  nsCOMPtr<nsITransactionManager> txnMgr;
  editor->GetTransactionManager(getter_AddRefs(txnMgr));
  if (txnMgr)
  {
    txnMgr->AddListener(NS_STATIC_CAST(nsITransactionListener*, mStateMaintainer));
  }
  
  // set the editor in the editor controller  
  nsCOMPtr<nsISupports> editorAsISupports = do_QueryInterface(editor);
  if (mEditorController)
    mEditorController->SetCommandRefCon(editorAsISupports);
  if (mComposerController)
    mComposerController->SetCommandRefCon(editorAsISupports);

  rv = editor->PostCreate();
  if (NS_FAILED(rv)) return rv;

  if (!mMailCompose) {
    // Set the editor-specific Window caption
    UpdateWindowTitleAndRecentMenu(PR_TRUE);
  }

#ifdef DEBUG
  // Activate the debug menu only in debug builds
  // by removing the "hidden" attribute set "true" in XUL
  nsCOMPtr<nsIDOMElement> elem;
  rv = dDoc->GetElementById(NS_LITERAL_STRING("debugMenu"), getter_AddRefs(elem));
  if (elem)
    elem->RemoveAttribute(NS_LITERAL_STRING("hidden"));
#endif

  // Force initial focus to the content window except if in mail compose
  // why aren't we doing this in JS?
  if (!mMailCompose)
  {
    if(!mContentWindow)
      return NS_ERROR_NOT_INITIALIZED;
    nsCOMPtr<nsIDOMWindowInternal> cwP = do_QueryReferent(mContentWindow);
    if (!cwP) return NS_ERROR_NOT_INITIALIZED;
      cwP->Focus();

    //mContentWindow->Focus();
    // Collapse the selection to the begining of the document
    // (this also turns on the caret)
    nsCOMPtr<nsIPlaintextEditor> textEditor (do_QueryInterface(mEditor));
    if (textEditor)
      textEditor->CollapseSelectionToStart();
  }

  // show the caret, if our window is focussed already
  nsCOMPtr<nsIDOMWindowInternal> contentInternal = do_QueryReferent(mContentWindow);
  nsCOMPtr<nsPIDOMWindow> privContent(do_QueryInterface(contentInternal));
  
  if (privContent)
  {
    nsCOMPtr<nsIFocusController> focusController;
    privContent->GetRootFocusController(getter_AddRefs(focusController));
    
    if (focusController)
    {
      nsCOMPtr<nsIDOMWindowInternal> focussedWindow;
      focusController->GetFocusedWindow(getter_AddRefs(focussedWindow));

      if (focussedWindow.get() == contentInternal.get())    // now see if we are focussed
      {
        nsCOMPtr<nsISelectionController> selCon;
        editor->GetSelectionController(getter_AddRefs(selCon));

        PRInt32 pixelWidth = 1;

        static NS_DEFINE_CID(kLookAndFeelCID, NS_LOOKANDFEEL_CID);

        nsCOMPtr<nsILookAndFeel> lookNFeel = do_GetService(kLookAndFeelCID);
        if (lookNFeel)
          lookNFeel->GetMetric(nsILookAndFeel::eMetric_MultiLineCaretWidth, pixelWidth);

        selCon->SetCaretWidth(pixelWidth);
        selCon->SetCaretEnabled(PR_FALSE);
        selCon->SetCaretEnabled(PR_TRUE);   // make damn sure it shows; the last SetVisible
                                            // may have happened when we didn't have focus.
        selCon->SetDisplaySelection(nsISelectionController::SELECTION_ON);
      }
    }
  }

  return NS_OK;
}

nsresult
nsEditorShell::GetDocShellFromContentWindow(nsIDocShell **aDocShell)
{
  if (!aDocShell)
    return NS_ERROR_NULL_POINTER;
  if (!mContentWindow)
    return NS_ERROR_FAILURE;

  nsresult  rv;
  nsCOMPtr<nsIScriptGlobalObject> globalObj = do_QueryReferent(mContentWindow, &rv);
  if (NS_FAILED(rv) || !globalObj)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDocShell> docShell;
  globalObj->GetDocShell(getter_AddRefs(docShell));
  if (!docShell)
  {
    NS_ASSERTION(0, "Failed to get docShell from mContentWindow");
    return NS_ERROR_UNEXPECTED;
  }

  *aDocShell = docShell;
  NS_ADDREF(*aDocShell);
  
  return NS_OK;
}

NS_IMETHODIMP    
nsEditorShell::SetContentWindow(nsIDOMWindowInternal* aWin)
{
  NS_PRECONDITION(aWin != nsnull, "null ptr");
  if (!aWin)
      return NS_ERROR_NULL_POINTER;

  mContentWindow = getter_AddRefs( NS_GetWeakReference(aWin) );  // weak reference to aWin
  //mContentWindow = aWin;


  nsCOMPtr<nsIDocShell> docShell;
  nsresult rv = GetDocShellFromContentWindow(getter_AddRefs(docShell));
  if (NS_FAILED(rv)) return rv;
  if (!docShell)
    return NS_ERROR_FAILURE;
    
  // Remove the WebProgress listener from the old docshell (if any...)
  nsCOMPtr<nsIWebProgress> webProgress;
  if (mContentAreaDocShell) {
    webProgress = do_GetInterface(mContentAreaDocShell, &rv);
    if (webProgress) {
      (void) webProgress->RemoveProgressListener(this);
    }
  }

  // Attach a WebProgress listener to the new docShell
  webProgress = do_GetInterface(docShell, &rv);
  if (webProgress) {
    rv = webProgress->AddProgressListener(this,
                                          (nsIWebProgress::NOTIFY_STATE_NETWORK  | 
                                           nsIWebProgress::NOTIFY_STATE_DOCUMENT |
                                           nsIWebProgress::NOTIFY_LOCATION));
  }
  if (NS_FAILED(rv)) return rv;

  mContentAreaDocShell = docShell;      // dont AddRef
    
  // we make two controllers
  nsCOMPtr<nsIControllers> controllers;      
  if(!mContentWindow)
    return NS_ERROR_NOT_INITIALIZED;
  nsCOMPtr<nsIDOMWindowInternal> cwP = do_QueryReferent(mContentWindow);
  if (!cwP) return NS_ERROR_NOT_INITIALIZED;
    cwP->GetControllers(getter_AddRefs(controllers));

  if (NS_FAILED(rv)) return rv;

  {
    // the first is an editor controller, and takes an nsIEditor as the refCon
    nsCOMPtr<nsIController> controller = do_CreateInstance("@mozilla.org/editor/editorcontroller;1", &rv);
    if (NS_FAILED(rv)) return rv;  
    nsCOMPtr<nsIEditorController> editorController = do_QueryInterface(controller);
    rv = editorController->Init(nsnull);    // we set the editor later when we have one
    if (NS_FAILED(rv)) return rv;
    
    mEditorController = editorController;   // temp weak link, so we can get it and set the editor later
    
    rv = controllers->InsertControllerAt(eEditorController, controller);
    if (NS_FAILED(rv)) return rv;  
  }

  {
    // the second is a composer controller (now also takes nsIEditor as refCon)
    nsCOMPtr<nsIController> controller = do_CreateInstance("@mozilla.org/editor/composercontroller;1", &rv);
    if (NS_FAILED(rv)) return rv;  
    nsCOMPtr<nsIEditorController> editorController = do_QueryInterface(controller);
    nsCOMPtr<nsISupports> editorAsISupports = do_QueryInterface(mEditor);
    rv = editorController->Init(editorAsISupports);    // we set the editor later when we have one
    if (NS_FAILED(rv)) return rv;

    mComposerController = editorController;   // temp weak link, so we can get it and set the editor later

    rv = controllers->InsertControllerAt(eComposerController, controller);
    if (NS_FAILED(rv)) return rv;
  }

  return NS_OK;
}


NS_IMETHODIMP    
nsEditorShell::GetContentWindow(nsIDOMWindowInternal * *aContentWindow)
{
  NS_ENSURE_ARG_POINTER(aContentWindow);

  if(!mContentWindow)
    return NS_ERROR_NOT_INITIALIZED;
  nsCOMPtr<nsIDOMWindowInternal> cwP = do_QueryReferent(mContentWindow);
  if (!cwP) 
    return NS_ERROR_NOT_INITIALIZED;
  
  NS_IF_ADDREF(*aContentWindow = cwP);
  return NS_OK;
}


NS_IMETHODIMP    
nsEditorShell::SetWebShellWindow(nsIDOMWindowInternal* aWin)
{
  NS_PRECONDITION(aWin != nsnull, "null ptr");
  if (!aWin)
      return NS_ERROR_NULL_POINTER;

  mWebShellWindow = aWin;   // no addref
  
  nsCOMPtr<nsIScriptGlobalObject> globalObj( do_QueryInterface(aWin) );
  if (!globalObj) {
    return NS_ERROR_FAILURE;
  }

  nsresult  rv = NS_OK;
  
  nsCOMPtr<nsIDocShell> docShell;
  globalObj->GetDocShell(getter_AddRefs(docShell));
  if (!docShell)
    return NS_ERROR_NOT_INITIALIZED;

  mDocShell = docShell;

  // register as a content listener, so that we can fend off URL
  // loads from sidebar
  rv = mDocShell->SetParentURIContentListener(this);

/*
#ifdef APP_DEBUG
  nsXPIDLString name;
  nsCOMPtr<nsIDocShellTreeItem> docShellAsItem(do_QueryInterface(docShell));
  docShellAsItem->GetName(getter_Copies(name));
  nsAutoString str(name);

  char* cstr = ToNewCString(str);
  printf("Attaching to WebShellWindow[%s]\n", cstr);
  nsCRT::free(cstr);
#endif
*/
    
  return rv;
}

NS_IMETHODIMP    
nsEditorShell::GetWebShellWindow(nsIDOMWindowInternal * *aWebShellWindow)
{
  NS_ENSURE_ARG_POINTER(aWebShellWindow);
  NS_IF_ADDREF(*aWebShellWindow = mWebShellWindow);
  return NS_OK;
}

// tell the appcore what type of editor to instantiate
// this must be called before the editor has been instantiated,
// otherwise, an error is returned.
NS_IMETHODIMP
nsEditorShell::SetEditorType(const PRUnichar *editorType)
{  
  if (mEditor)
    return NS_ERROR_ALREADY_INITIALIZED;
    
  nsAutoString  theType(editorType);
  ToLowerCase(theType);

  PRBool textMail = theType.Equals(NS_LITERAL_STRING("textmail"));
  mMailCompose = textMail || theType.Equals(NS_LITERAL_STRING("htmlmail"));

  if (mMailCompose || theType.Equals(NS_LITERAL_STRING("text")) 
      || theType.Equals(NS_LITERAL_STRING("html")) || theType.IsEmpty())
  {
    // We don't store a separate type for textmail
    if (textMail)
      mEditorTypeString = NS_LITERAL_STRING("text");
    else
      mEditorTypeString = theType;
    return NS_OK;
  }
  else
  {
    NS_WARNING("Editor type not recognized");
    return NS_ERROR_UNEXPECTED;
  }
}

NS_IMETHODIMP
nsEditorShell::GetEditorType(PRUnichar **_retval)
{
  if (!_retval)
    return NS_ERROR_NULL_POINTER;

  *_retval = ToNewUnicode(mEditorTypeString);

  return NS_OK;
}

/* attribute string contentsMIMEType; */
NS_IMETHODIMP
nsEditorShell::GetContentsMIMEType(char * *aContentsMIMEType)
{
  NS_ENSURE_ARG_POINTER(aContentsMIMEType);
  *aContentsMIMEType = ToNewCString(mContentMIMEType);
  return NS_OK;
}

NS_IMETHODIMP
nsEditorShell::SetContentsMIMEType(const char * aContentsMIMEType)
{
  mContentMIMEType.Assign(aContentsMIMEType ? aContentsMIMEType : "");
  return NS_OK;
}

nsresult
nsEditorShell::InstantiateEditor(nsIDOMDocument *aDoc, nsIPresShell *aPresShell)
{
  NS_PRECONDITION(aDoc && aPresShell, "null ptr");
  if (!aDoc || !aPresShell)
    return NS_ERROR_NULL_POINTER;

  if (mEditor)
    return NS_ERROR_ALREADY_INITIALIZED;

  nsresult rv = NS_OK;
  
  nsCOMPtr<nsIEditor> editor;
  rv = nsComponentManager::CreateInstance(kHTMLEditorCID, nsnull, NS_GET_IID(nsIEditor), getter_AddRefs(editor));
  if (NS_FAILED(rv)) return rv;

  nsCOMPtr<nsISelectionController> selCon = do_QueryInterface(aPresShell);
  
  if (mEditorTypeString.Equals(NS_LITERAL_STRING("text")))
  {
    PRInt16 flags = nsIPlaintextEditor::eEditorPlaintextMask | nsIPlaintextEditor::eEditorEnableWrapHackMask;
    if (mMailCompose) flags |= nsIPlaintextEditor::eEditorMailMask;
    rv = editor->Init(aDoc, aPresShell, nsnull, selCon, flags);
    mEditorType = ePlainTextEditorType;
  }
  else if (mEditorTypeString.Equals(NS_LITERAL_STRING("html")) || mEditorTypeString.IsEmpty())  // empty string default to HTML editor
  {
    PRUint32    editorFlags = 0;
    EEditorType editorType  = eHTMLTextEditorType;
    
    // if the MIME type of the docment we are editing is text/plain, make a text editor
    PRBool makeATextEditor;
    IsSupportedTextType(mContentMIMEType.get(), &makeATextEditor);
    if (makeATextEditor)
    {
      editorFlags = nsIPlaintextEditor::eEditorPlaintextMask;
      editorType  = ePlainTextEditorType;
    }
    
    rv = editor->Init(aDoc, aPresShell, nsnull, selCon, editorFlags);
    mEditorType = editorType;
  }
  else if (mEditorTypeString.Equals(NS_LITERAL_STRING("htmlmail")))  //  HTML editor with special mail rules
  {
    rv = editor->Init(aDoc, aPresShell, nsnull, selCon, nsIPlaintextEditor::eEditorMailMask);
    mEditorType = eHTMLTextEditorType;
  }
  else
  {
    rv = NS_ERROR_INVALID_ARG;    // this is not an editor we know about
#if DEBUG
    nsAutoString  errorMsg(NS_LITERAL_STRING("Failed to init editor. Unknown editor type \""));
    errorMsg += mEditorTypeString;
    errorMsg.Append(NS_LITERAL_STRING("\"\n"));
    char  *errorMsgCString = ToNewCString(errorMsg);
    NS_WARNING(errorMsgCString);
    nsCRT::free(errorMsgCString);
#endif
  }

    // disable the preference style sheet so we can override colors
  if (NS_SUCCEEDED(rv))
  {
    rv = aPresShell->EnablePrefStyleRules(PR_FALSE,0);
  }

  if (NS_SUCCEEDED(rv) && editor)
  {
    mEditor = do_QueryInterface(editor);    // this does the addref that is the owning reference
  }
    
  return rv;
}


nsresult
nsEditorShell::DoEditorMode(nsIDocShell *aDocShell)
{
  nsresult  err = NS_OK;
  
  NS_PRECONDITION(aDocShell, "Need a webshell here");
  if (!aDocShell)
      return NS_ERROR_NULL_POINTER;

  nsCOMPtr<nsIDocument> doc;
  err = GetDocument(aDocShell, getter_AddRefs(doc));
  if (NS_FAILED(err)) return err;
  if (!doc) return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDOMDocument>  domDoc = do_QueryInterface(doc, &err);
  if (NS_FAILED(err)) return err;
  if (!domDoc) return NS_ERROR_FAILURE;

  nsCOMPtr<nsIPresShell> presShell;
  err = aDocShell->GetPresShell(getter_AddRefs(presShell));
  if (NS_FAILED(err)) return err;
  if (!presShell) return NS_ERROR_FAILURE;
  
  return InstantiateEditor(domDoc, presShell);
}

// Deletion routines
nsresult
nsEditorShell::ScrollSelectionIntoView()
{
  nsCOMPtr<nsIEditor> editor = do_QueryInterface(mEditor);
  if (!editor) return NS_ERROR_NOT_INITIALIZED;
  nsCOMPtr<nsISelectionController> selCon;
  editor->GetSelectionController(getter_AddRefs(selCon));
  if (!selCon)
    return NS_ERROR_UNEXPECTED;
  return selCon->ScrollSelectionIntoView(nsISelectionController::SELECTION_NORMAL,
                                            nsISelectionController::SELECTION_FOCUS_REGION, PR_TRUE);
}

NS_IMETHODIMP
nsEditorShell::DeleteCharForward()
{
  nsCOMPtr<nsIEditor> editor = do_QueryInterface(mEditor);
  if (!editor) return NS_ERROR_NOT_INITIALIZED;
  nsresult rv = editor->DeleteSelection(nsIEditor::eNext);
  ScrollSelectionIntoView();
  return rv;
}

NS_IMETHODIMP
nsEditorShell::DeleteCharBackward()
{
  nsCOMPtr<nsIEditor> editor = do_QueryInterface(mEditor);
  if (!editor) return NS_ERROR_NOT_INITIALIZED;
  nsresult rv = editor->DeleteSelection(nsIEditor::ePrevious);
  ScrollSelectionIntoView();
  return rv;
}

NS_IMETHODIMP
nsEditorShell::DeleteWordForward()
{
  nsCOMPtr<nsIEditor> editor = do_QueryInterface(mEditor);
  if (!editor) return NS_ERROR_NOT_INITIALIZED;
  nsresult rv = editor->DeleteSelection(nsIEditor::eNextWord);
  ScrollSelectionIntoView();
  return rv;
}

NS_IMETHODIMP
nsEditorShell::DeleteWordBackward()
{
  nsCOMPtr<nsIEditor> editor = do_QueryInterface(mEditor);
  if (!editor) return NS_ERROR_NOT_INITIALIZED;
  nsresult rv = editor->DeleteSelection(nsIEditor::ePreviousWord);
  ScrollSelectionIntoView();
  return rv;
}

NS_IMETHODIMP
nsEditorShell::DeleteToEndOfLine()
{
  nsCOMPtr<nsIEditor> editor = do_QueryInterface(mEditor);
  if (!editor) return NS_ERROR_NOT_INITIALIZED;
  nsresult rv = editor->DeleteSelection(nsIEditor::eToEndOfLine);
  ScrollSelectionIntoView();
  return rv;
}

NS_IMETHODIMP
nsEditorShell::DeleteToBeginningOfLine()
{
  nsCOMPtr<nsIEditor> editor = do_QueryInterface(mEditor);
  if (!editor) return NS_ERROR_NOT_INITIALIZED;
  nsresult rv = editor->DeleteSelection(nsIEditor::eToBeginningOfLine);
  ScrollSelectionIntoView();
  return rv;
}

// Generic attribute setting and removal
NS_IMETHODIMP    
nsEditorShell::SetAttribute(nsIDOMElement *element, const PRUnichar *attr, const PRUnichar *value)
{
  if (!element || !attr || !value)
    return NS_ERROR_NULL_POINTER;

  nsresult  result = NS_NOINTERFACE;
  nsCOMPtr<nsIEditor> editor = do_QueryInterface(mEditor);
  if (editor) {
    result = editor->SetAttribute(element, nsDependentString(attr), nsDependentString(value)); 
  }

  return result;
}

NS_IMETHODIMP    
nsEditorShell::RemoveAttribute(nsIDOMElement *element, const PRUnichar *attr)
{
  if (!element || !attr)
    return NS_ERROR_NULL_POINTER;

  nsresult  result = NS_NOINTERFACE;
  nsCOMPtr<nsIEditor> editor = do_QueryInterface(mEditor);
  if (editor) {
    result = editor->RemoveAttribute(element, nsDependentString(attr));
  }

  return result;
}

// the name of the attribute here should be the contents of the appropriate
// tag, e.g. 'b' for bold, 'i' for italics.
NS_IMETHODIMP    
nsEditorShell::SetTextProperty(const PRUnichar *prop, const PRUnichar *attr, const PRUnichar *value)
{
  nsresult  err = NS_NOINTERFACE;

  if (!mEditor) return NS_ERROR_NOT_INITIALIZED;

  nsCOMPtr<nsIAtom> styleAtom = getter_AddRefs(NS_NewAtom(prop));      /// XXX Hack alert! Look in nsIEditProperty.h for this
  if (! styleAtom) return NS_ERROR_OUT_OF_MEMORY;
 
  static const PRUnichar sEmptyStr = PRUnichar('\0');
  switch (mEditorType)
  {
    case ePlainTextEditorType:
        // should we allow this?
    case eHTMLTextEditorType:
      err = mEditor->SetInlineProperty(styleAtom,
                                    nsDependentString(attr?attr:&sEmptyStr),
                                    nsDependentString(value?value:&sEmptyStr));
      break;
    default:
      err = NS_ERROR_NOT_IMPLEMENTED;
  }

  return err;
}



nsresult
nsEditorShell::RemoveOneProperty(const nsString& aProp, const nsString &aAttr)
{
  nsresult  err = NS_NOINTERFACE;

  if (!mEditor) return NS_ERROR_NOT_INITIALIZED;

  nsCOMPtr<nsIAtom> styleAtom = getter_AddRefs(NS_NewAtom(aProp));      /// XXX Hack alert! Look in nsIEditProperty.h for this
  if (! styleAtom) return NS_ERROR_OUT_OF_MEMORY;

  switch (mEditorType)
  {
    case ePlainTextEditorType:
        // should we allow this?
    case eHTMLTextEditorType:
      err = mEditor->RemoveInlineProperty(styleAtom, aAttr);
      break;
    default:
      err = NS_ERROR_NOT_IMPLEMENTED;
  }

  return err;
}


// the name of the attribute here should be the contents of the appropriate
// tag, e.g. 'b' for bold, 'i' for italics.
NS_IMETHODIMP    
nsEditorShell::RemoveTextProperty(const PRUnichar *prop, const PRUnichar *attr)
{
  if (!mEditor) return NS_ERROR_NOT_INITIALIZED;

  // OK, I'm really hacking now. This is just so that we can accept 'all' as input.  
  nsAutoString  allStr(prop);
  nsAutoString  aAttr(attr);
  
  ToLowerCase(allStr);
  PRBool    doingAll = (allStr.Equals(NS_LITERAL_STRING("all")));
  nsresult  err = NS_OK;

  if (doingAll)
  {
    err = mEditor->RemoveAllInlineProperties();
  }
  else
  {
    nsAutoString  aProp(prop);
    err = RemoveOneProperty(aProp, aAttr);
  }
  
  return err;
}


NS_IMETHODIMP
nsEditorShell::GetTextProperty(const PRUnichar *prop, const PRUnichar *attr, const PRUnichar *value, PRBool *firstHas, PRBool *anyHas, PRBool *allHas)
{
  if (!mEditor) return NS_ERROR_NOT_INITIALIZED;

  nsIAtom    *styleAtom = nsnull;
  nsresult  err = NS_NOINTERFACE;

  styleAtom = NS_NewAtom(prop);      /// XXX Hack alert! Look in nsIEditProperty.h for this

  switch (mEditorType)
  {
    case ePlainTextEditorType:
        // should we allow this?
    case eHTMLTextEditorType:
      err = mEditor->GetInlineProperty(styleAtom, nsDependentString(attr), nsDependentString(value), firstHas, anyHas, allHas);
      break;
    default:
      err = NS_ERROR_NOT_IMPLEMENTED;
  }
  NS_RELEASE(styleAtom);
  return err;
}

NS_IMETHODIMP
nsEditorShell::IncreaseFontSize()
{
  nsCOMPtr<nsIHTMLEditor>  htmlEditor = do_QueryInterface(mEditor);
  if (htmlEditor)
    return htmlEditor->IncreaseFontSize();

  return NS_NOINTERFACE;
}

NS_IMETHODIMP
nsEditorShell::DecreaseFontSize()
{
  nsCOMPtr<nsIHTMLEditor>  htmlEditor = do_QueryInterface(mEditor);
  if (htmlEditor)
    return htmlEditor->DecreaseFontSize();

  return NS_NOINTERFACE;
}


NS_IMETHODIMP 
nsEditorShell::SetBackgroundColor(const PRUnichar *color)
{
  nsresult result = NS_NOINTERFACE;
  
  if (!mEditor) return NS_ERROR_NOT_INITIALIZED;
  
  nsAutoString aColor(color);
  result = mEditor->SetBackgroundColor(aColor);

  return result;
}

NS_IMETHODIMP 
nsEditorShell::GetParagraphState(PRBool *aMixed, PRUnichar **_retval)
{
  if (!aMixed || !_retval) return NS_ERROR_NULL_POINTER;
  *_retval = nsnull;
  *aMixed = PR_FALSE;

  nsresult  err = NS_NOINTERFACE;
  nsCOMPtr<nsIHTMLEditor>  htmlEditor = do_QueryInterface(mEditor);

  if (htmlEditor)
  {
    PRBool bMixed;
    nsAutoString state;
    err = htmlEditor->GetParagraphState(&bMixed, state);
    if (!bMixed)
      *_retval = ToNewUnicode(state);
  }
  return err;
}

NS_IMETHODIMP 
nsEditorShell::GetListState(PRBool *aMixed, PRUnichar **_retval)
{
  if (!aMixed || !_retval) return NS_ERROR_NULL_POINTER;
  *_retval = nsnull;
  *aMixed = PR_FALSE;

  nsresult  err = NS_NOINTERFACE;
  nsCOMPtr<nsIHTMLEditor>  htmlEditor = do_QueryInterface(mEditor);
  if (htmlEditor)
  {
    PRBool bOL, bUL, bDL;
    err = htmlEditor->GetListState(aMixed, &bOL, &bUL, &bDL);
    if (NS_SUCCEEDED(err))
    {
      if (!*aMixed)
      {
        nsAutoString tagStr;
        if (bOL) tagStr.Assign(NS_LITERAL_STRING("ol"));
        else if (bUL) tagStr.Assign(NS_LITERAL_STRING("ul"));
        else if (bDL) tagStr.Assign(NS_LITERAL_STRING("dl"));
        *_retval = ToNewUnicode(tagStr);
      }
    }  
  }
  return err;
}

NS_IMETHODIMP 
nsEditorShell::GetListItemState(PRBool *aMixed, PRUnichar **_retval)
{
  if (!aMixed || !_retval) return NS_ERROR_NULL_POINTER;
  *_retval = nsnull;
  *aMixed = PR_FALSE;

  nsresult  err = NS_NOINTERFACE;
  nsCOMPtr<nsIHTMLEditor>  htmlEditor = do_QueryInterface(mEditor);
  if (htmlEditor)
  {
    PRBool bLI,bDT,bDD;
    err = htmlEditor->GetListItemState(aMixed, &bLI, &bDT, &bDD);
    if (NS_SUCCEEDED(err))
    {
      if (!*aMixed)
      {
        nsAutoString tagStr;
        if (bLI) tagStr.Assign(NS_LITERAL_STRING("li"));
        else if (bDT) tagStr.Assign(NS_LITERAL_STRING("dt"));
        else if (bDD) tagStr.Assign(NS_LITERAL_STRING("dd"));
        *_retval = ToNewUnicode(tagStr);
      }
    }  
  }
  return err;
}

NS_IMETHODIMP 
nsEditorShell::GetAlignment(PRBool *aMixed, PRUnichar **_retval)
{
  if (!aMixed || !_retval) return NS_ERROR_NULL_POINTER;
  *_retval = nsnull;
  *aMixed = PR_FALSE;

  nsresult  err = NS_NOINTERFACE;
  nsCOMPtr<nsIHTMLEditor>  htmlEditor = do_QueryInterface(mEditor);
  if (htmlEditor)
  {
    nsIHTMLEditor::EAlignment firstAlign;
    err = htmlEditor->GetAlignment(aMixed, &firstAlign);
    if (NS_SUCCEEDED(err))
    {
      nsAutoString tagStr;
      if (firstAlign == nsIHTMLEditor::eLeft)        
        tagStr.Assign(NS_LITERAL_STRING("left"));
      else if (firstAlign == nsIHTMLEditor::eCenter) 
        tagStr.Assign(NS_LITERAL_STRING("center"));
      else if (firstAlign == nsIHTMLEditor::eRight)  
        tagStr.Assign(NS_LITERAL_STRING("right"));
      else if (firstAlign == nsIHTMLEditor::eJustify)
        tagStr.Assign(NS_LITERAL_STRING("justify"));
      *_retval = ToNewUnicode(tagStr);
    }  
  }
  return err;
}

NS_IMETHODIMP 
nsEditorShell::ApplyStyleSheet(const PRUnichar *url)
{
  nsresult result = NS_NOINTERFACE;
  
  nsAutoString  aURL(url);

  nsCOMPtr<nsIEditorStyleSheets> styleSheets = do_QueryInterface(mEditor);
  if (styleSheets)
    result = styleSheets->ReplaceStyleSheet(aURL);

  return result;
}

  
NS_IMETHODIMP 
nsEditorShell::SetBodyAttribute(const PRUnichar *attr, const PRUnichar *value)
{
  nsresult result = NS_NOINTERFACE;
  
  if (!mEditor) return NS_ERROR_NOT_INITIALIZED;

  nsAutoString  aAttr(attr);
  nsAutoString  aValue(value);
  
  switch (mEditorType)
  {
    case eHTMLTextEditorType:
    {
      result = mEditor->SetBodyAttribute(aAttr, aValue);
      break;
    }
    default:
      result = NS_ERROR_NOT_IMPLEMENTED;
  }
  return result;
}

NS_IMETHODIMP    
nsEditorShell::LoadUrl(const PRUnichar *url)
{
  if(!mContentAreaDocShell)
    return NS_ERROR_NOT_INITIALIZED;

  nsresult rv = ResetEditingState();
  if (NS_FAILED(rv)) return rv;
  
  nsCOMPtr<nsIWebNavigation> webNav(do_QueryInterface(mContentAreaDocShell));
  rv = webNav->LoadURI(url,                               // uri string
                       nsIWebNavigation::LOAD_FLAGS_BYPASS_CACHE, // load flags
                       nsnull,                            // referrer
                       nsnull,                            // post-data stream
                       nsnull);                           // headers stream

  NS_ASSERTION(NS_SUCCEEDED(rv), "LoadURI failed!");

  return rv;
}


NS_IMETHODIMP    
nsEditorShell::RegisterDocumentStateListener(nsIDocumentStateListener *docListener)
{
  nsresult rv = NS_OK;
  
  if (!docListener)
    return NS_ERROR_NULL_POINTER;
    
  // Make the array
  if (!mDocStateListeners)
  {
    rv = NS_NewISupportsArray(getter_AddRefs(mDocStateListeners));
    if (NS_FAILED(rv)) return rv;
  }
  nsCOMPtr<nsISupports> iSupports = do_QueryInterface(docListener, &rv);
  if (NS_FAILED(rv)) return rv;

  PRBool appended = mDocStateListeners->AppendElement(iSupports);
  NS_ASSERTION(appended, "Append failed");
  
  // if we have an editor already, register this right now.
  if (mEditor)
  {
    nsCOMPtr<nsIEditor>  editor = do_QueryInterface(mEditor, &rv);
    if (NS_FAILED(rv)) return rv;
  
    // this checks for duplicates
    rv = editor->AddDocumentStateListener(docListener);
  }

  return NS_OK;
}

NS_IMETHODIMP    
nsEditorShell::UnregisterDocumentStateListener(nsIDocumentStateListener *docListener)
{
  if (!docListener)
    return NS_ERROR_NULL_POINTER;

  nsresult rv = NS_OK;
  
  // remove it from our list
  if (mDocStateListeners)
  {
    nsCOMPtr<nsISupports> iSupports = do_QueryInterface(docListener, &rv);
    if (NS_FAILED(rv)) return rv;

    mDocStateListeners->RemoveElement(iSupports);
  }
    
  // if we have an editor already, remove it from there too
  if (mEditor)
  {
    nsCOMPtr<nsIEditor>  editor = do_QueryInterface(mEditor, &rv);
    if (NS_FAILED(rv)) return rv;
  
    return editor->RemoveDocumentStateListener(docListener);
  }


  return NS_OK;
}

// called after making an editor. Transfer the nsIDOcumentStateListeners
// that we have been stashing in mDocStateListeners to the editor.
nsresult    
nsEditorShell::TransferDocumentStateListeners()
{
  if (!mDocStateListeners)
    return NS_OK;
   
  if (!mEditor)
    return NS_ERROR_NOT_INITIALIZED;    // called too early.

  nsresult  rv;
  nsCOMPtr<nsIEditor> editor = do_QueryInterface(mEditor, &rv);
  if (NS_FAILED(rv)) return rv;
    
  PRUint32 numListeners;  
  mDocStateListeners->Count(&numListeners);

  for (PRUint32 i = 0; i <  numListeners; i ++)
  {
    nsCOMPtr<nsISupports> iSupports = getter_AddRefs(mDocStateListeners->ElementAt(i));
    nsCOMPtr<nsIDocumentStateListener> docStateListener = do_QueryInterface(iSupports);
    if (docStateListener)
    {
      // this checks for duplicates
      rv = editor->AddDocumentStateListener(docStateListener);
      if (NS_FAILED(rv)) break;
    }
  }
  
  return NS_OK;
}

nsresult
nsEditorShell::GetDocumentURI(nsIDOMDocument *aDoc, nsIURI **aDocumentURI)
{
  nsCOMPtr<nsIDOMHTMLDocument> htmlDoc = do_QueryInterface(aDoc);
  if (!htmlDoc) return NS_ERROR_NULL_POINTER;

  // get the url of the current document
  nsAutoString urlstring;
  nsresult res = htmlDoc->GetURL(urlstring);
  if (NS_FAILED(res)) return res;

  // if it's about:blank, then we haven't saved yet
  if (urlstring.EqualsIgnoreCase("about:blank"))
    return NS_ERROR_NOT_INITIALIZED;

  // create the uri to return
  char *docURLChar = ToNewCString(urlstring);
  if (docURLChar)
  {
    res = NS_NewURI(aDocumentURI, docURLChar);
    nsCRT::free(docURLChar);
    if (NS_FAILED(res)) return res;
  }

  return NS_OK;
}

NS_IMETHODIMP    
nsEditorShell::CloseWindowWithoutSaving()
{
  nsCOMPtr<nsIBaseWindow> baseWindow;
  GetTreeOwner(mDocShell, getter_AddRefs(baseWindow));
  NS_ENSURE_TRUE(baseWindow, NS_ERROR_FAILURE);
  return baseWindow->Destroy();
}

NS_IMETHODIMP    
nsEditorShell::Print()
{ 
  if (!mContentAreaDocShell)
    return NS_ERROR_NOT_INITIALIZED;

  nsCOMPtr<nsIContentViewer> viewer;
  mContentAreaDocShell->GetContentViewer(getter_AddRefs(viewer));    
  if (nsnull != viewer) 
  {
    nsCOMPtr<nsIWebBrowserPrint> webBrowserPrint = do_QueryInterface(viewer);
    if (webBrowserPrint) {
      NS_ENSURE_SUCCESS(webBrowserPrint->Print(nsnull, (nsIWebProgressListener*)nsnull), NS_ERROR_FAILURE);
    }
  }
  return NS_OK;
}

nsresult
nsEditorShell::UpdateWindowTitleAndRecentMenu(PRBool aSaveToPrefs)
{
  nsresult res = NS_ERROR_NOT_INITIALIZED;

  if (!mContentAreaDocShell || !mEditor)
    return res;

  nsCOMPtr<nsIEditor> editor = do_QueryInterface(mEditor);
  if (!editor)
    return res;

  nsAutoString windowCaption;
  res = GetDocumentTitleString(windowCaption);
  // If title is empty, use "untitled"
  if (windowCaption.Length() == 0)
    GetBundleString(NS_LITERAL_STRING("untitled"), windowCaption);

  // Append just the 'leaf' filename to the Doc. Title for the window caption
  if (NS_SUCCEEDED(res))
  {
    // get the dom document
    nsCOMPtr<nsIDOMDocument> doc;
    res = editor->GetDocument(getter_AddRefs(doc));
    if (NS_FAILED(res)) return res;
    if (!doc) return NS_ERROR_NULL_POINTER;

    // find out if the doc already has a fileSpec associated with it.
    nsCOMPtr<nsIURI> docFileSpec;
    if (NS_SUCCEEDED(GetDocumentURI(doc, getter_AddRefs(docFileSpec))))
    {
      nsCOMPtr<nsIURL> url = do_QueryInterface(docFileSpec);
      if (url)
      {
        // Prefix filename with scheme so user can tell if editing a remote vs. local file
        nsCAutoString schemeChar;
        docFileSpec->GetScheme(schemeChar);
        nsCAutoString fileNameChar;
        url->GetFileName(fileNameChar);
        if (fileNameChar.Length() > 0)
        {
          windowCaption += NS_LITERAL_STRING(" [") +
                           NS_ConvertUTF8toUCS2(schemeChar) +
                           NS_LITERAL_STRING(":/.../") +
                           NS_ConvertUTF8toUCS2(fileNameChar) +
                           NS_LITERAL_STRING("]");
        }
      }
    }
    nsCOMPtr<nsIBaseWindow> contentAreaAsWin(do_QueryInterface(mContentAreaDocShell));
    NS_ASSERTION(contentAreaAsWin, "This object should implement nsIBaseWindow");
    res = contentAreaAsWin->SetTitle(windowCaption.get());
  }

  // Rebuild Recent Pages menu and save any changed URLs or titles to editor prefs
  // TODO: We need to pass on aSaveToPrefs to command, but must wait for
  //    new command system before we can do that.
  // For now, don't update the menu at all if aSaveToPrefs is false
  if (aSaveToPrefs)
  {
    res = DoControllerCommand("cmd_buildRecentPagesMenu");
  }
   
  return res;
}

nsresult
nsEditorShell::GetDocumentTitleString(nsString& title)
{
  nsresult res = NS_ERROR_NOT_INITIALIZED;

  if (!mEditor)
    return res;

  nsCOMPtr<nsIEditor> editor = do_QueryInterface(mEditor);
  if (!editor)
    return res;

  nsCOMPtr<nsIDOMDocument>  domDoc;
  res = editor->GetDocument(getter_AddRefs(domDoc));
  if (NS_SUCCEEDED(res) && domDoc)
  {
    // Get the document title
    nsCOMPtr<nsIDOMHTMLDocument> HTMLDoc = do_QueryInterface(domDoc);
    if (HTMLDoc)
      res = HTMLDoc->GetTitle(title);
  }
  return res;
}

// JavaScript version
NS_IMETHODIMP
nsEditorShell::GetDocumentTitle(PRUnichar **title)
{
  if (!title)
    return NS_ERROR_NULL_POINTER;

  nsAutoString titleStr;
  nsresult res = GetDocumentTitleString(titleStr);
  if (NS_SUCCEEDED(res))
  {
    *title = ToNewUnicode(titleStr);
  } else {
    // Don't fail, just return an empty string    
    nsAutoString empty;
    *title = ToNewUnicode(empty);
    res = NS_OK;
  }
  return res;
}

NS_IMETHODIMP
nsEditorShell::SetDocumentTitle(const PRUnichar *title)
{
  nsresult res = NS_ERROR_NOT_INITIALIZED;

  if (!mEditor && !mContentAreaDocShell)
    return res;

  // This should only be allowed for HTML documents
  if (mEditorType == eHTMLTextEditorType)
  {
    res = mEditor->SetDocumentTitle(nsDependentString(title));
    if (NS_FAILED(res)) return res;
  }

  // PR_FALSE means don't save menu to prefs
  return UpdateWindowTitleAndRecentMenu(PR_FALSE);
}


NS_IMETHODIMP
nsEditorShell::CloneAttributes(nsIDOMNode *destNode, nsIDOMNode *sourceNode)
{
  if (!destNode || !sourceNode) { return NS_ERROR_NULL_POINTER; }
  nsresult  rv = NS_NOINTERFACE;
  
  switch (mEditorType)
  {
    case ePlainTextEditorType:
    case eHTMLTextEditorType:
      {
        nsCOMPtr<nsIEditor>  editor = do_QueryInterface(mEditor);
        if (editor)
          rv = editor->CloneAttributes(destNode, sourceNode);
      }
      break;

    default:
      rv = NS_ERROR_NOT_IMPLEMENTED;
  }

  return rv;
}

NS_IMETHODIMP
nsEditorShell::NodeIsBlock(nsIDOMNode *node, PRBool *_retval)
{
  if (!node || !_retval) { return NS_ERROR_NULL_POINTER; }
  if (!mEditor) return NS_ERROR_NOT_INITIALIZED;

  nsresult  rv = NS_NOINTERFACE;
  
  switch (mEditorType)
  {
    case ePlainTextEditorType:
    case eHTMLTextEditorType:
      {
        rv = mEditor->NodeIsBlock(node, _retval);
      }
      break;

    default:
      rv = NS_ERROR_NOT_IMPLEMENTED;
  }

  return rv;
}

NS_IMETHODIMP    
nsEditorShell::GetTransactionManager(nsITransactionManager **aTxnMgr)
{ 
  nsCOMPtr<nsIEditor> editor = do_QueryInterface(mEditor);

  if (!editor)
    return NS_ERROR_FAILURE;

  return editor->GetTransactionManager(aTxnMgr);
}

NS_IMETHODIMP    
nsEditorShell::Undo()
{ 
  nsresult  err = NS_NOINTERFACE;
  
  switch (mEditorType)
  {
    case ePlainTextEditorType:
    case eHTMLTextEditorType:
      {
        nsCOMPtr<nsIEditor>  editor = do_QueryInterface(mEditor);
        if (editor)
          err = editor->Undo(1);
      }
      break;

    default:
      err = NS_ERROR_NOT_IMPLEMENTED;
  }

  return err;
}

NS_IMETHODIMP    
nsEditorShell::Redo()
{  
  nsresult  err = NS_NOINTERFACE;
  
  switch (mEditorType)
  {
    case ePlainTextEditorType:
    case eHTMLTextEditorType:
      {
        nsCOMPtr<nsIEditor>  editor = do_QueryInterface(mEditor);
        if (editor)
          err = editor->Redo(1);
      }
      break;

    default:
      err = NS_ERROR_NOT_IMPLEMENTED;
  }

  return err;
}

NS_IMETHODIMP    
nsEditorShell::Cut()
{  
  nsresult  err = NS_NOINTERFACE;
  
  switch (mEditorType)
  {
    case ePlainTextEditorType:
    case eHTMLTextEditorType:
      {
        nsCOMPtr<nsIEditor>  editor = do_QueryInterface(mEditor);
        if (editor)
          err = editor->Cut();
      }
      break;

    default:
      err = NS_ERROR_NOT_IMPLEMENTED;
  }

  return err;
}

NS_IMETHODIMP    
nsEditorShell::Copy()
{  
  nsresult  err = NS_NOINTERFACE;
  
  switch (mEditorType)
  {
    case ePlainTextEditorType:
    case eHTMLTextEditorType:
      {
        nsCOMPtr<nsIEditor>  editor = do_QueryInterface(mEditor);
        if (editor)
          err = editor->Copy();
      }
      break;

    default:
      err = NS_ERROR_NOT_IMPLEMENTED;
  }

  return err;
}

NS_IMETHODIMP    
nsEditorShell::Paste(PRInt32 aSelectionType)
{  
  nsresult  err = NS_NOINTERFACE;
  
  switch (mEditorType)
  {
    case ePlainTextEditorType:
    case eHTMLTextEditorType:
      {
        nsCOMPtr<nsIEditor>  editor = do_QueryInterface(mEditor);
        if (editor)
          err = editor->Paste(aSelectionType);
      }
      break;

    default:
      err = NS_ERROR_NOT_IMPLEMENTED;
  }

  return err;
}

NS_IMETHODIMP    
nsEditorShell::PasteAsQuotation(PRInt32 aSelectionType)
{  
  nsresult  err = NS_NOINTERFACE;
  
  switch (mEditorType)
  {
    case ePlainTextEditorType:
    case eHTMLTextEditorType:
      {
        nsCOMPtr<nsIEditorMailSupport>  mailEditor = do_QueryInterface(mEditor);
        if (mailEditor)
          err = mailEditor->PasteAsQuotation(aSelectionType);
      }
      break;

    default:
      err = NS_ERROR_NOT_IMPLEMENTED;
  }

  return err;
}

NS_IMETHODIMP    
nsEditorShell::PasteAsCitedQuotation(const PRUnichar *cite, PRInt32 aSelectionType)
{  
  nsresult  err = NS_NOINTERFACE;
  
  nsAutoString aCiteString(cite);

  switch (mEditorType)
  {
    case ePlainTextEditorType:
    case eHTMLTextEditorType:
      {
        nsCOMPtr<nsIEditorMailSupport>  mailEditor = do_QueryInterface(mEditor);
        if (mailEditor)
          err = mailEditor->PasteAsCitedQuotation(aCiteString, aSelectionType);
      }
      break;

    default:
      err = NS_ERROR_NOT_IMPLEMENTED;
  }

  return err;
}

NS_IMETHODIMP    
nsEditorShell::InsertAsQuotation(const PRUnichar *aQuotedText,
                                 nsIDOMNode** aNodeInserted)
{  
  nsresult  err = NS_NOINTERFACE;
  
  switch (mEditorType)
  {
    case ePlainTextEditorType:
    case eHTMLTextEditorType:
      {
        nsCOMPtr<nsIEditorMailSupport>  mailEditor = do_QueryInterface(mEditor);
        if (mailEditor)
          err = mailEditor->InsertAsQuotation(nsDependentString(aQuotedText), aNodeInserted);
      }
      break;

    default:
      err = NS_ERROR_NOT_IMPLEMENTED;
  }

  return err;
}

NS_IMETHODIMP    
nsEditorShell::InsertAsCitedQuotation(const PRUnichar *quotedText,
                                      const PRUnichar *cite,
                                      PRBool aInsertHTML,
                                      const PRUnichar *charset,
                                      nsIDOMNode** aNodeInserted)
{  
  nsresult  err = NS_NOINTERFACE;
  
  nsCOMPtr<nsIEditorMailSupport> mailEditor = do_QueryInterface(mEditor);
  if (!mailEditor)
    return NS_NOINTERFACE;

  switch (mEditorType)
  {
    case ePlainTextEditorType:
      err = mailEditor->InsertAsQuotation(nsDependentString(quotedText), aNodeInserted);
      break;

    case eHTMLTextEditorType:
      err = mailEditor->InsertAsCitedQuotation(nsDependentString(quotedText), nsDependentString(cite),
                                               aInsertHTML,
                                               nsDependentString(charset), aNodeInserted);
      break;

    default:
      err = NS_ERROR_NOT_IMPLEMENTED;
  }

  return err;
}

NS_IMETHODIMP    
nsEditorShell::Rewrap(PRBool aRespectNewlines)
{
  nsCOMPtr<nsIEditorMailSupport> mailEditor = do_QueryInterface(mEditor);
  if (!mailEditor)
    return NS_NOINTERFACE;
  return mailEditor->Rewrap(aRespectNewlines);
}

NS_IMETHODIMP    
nsEditorShell::StripCites()
{
  nsCOMPtr<nsIEditorMailSupport> mailEditor = do_QueryInterface(mEditor);
  if (!mailEditor)
    return NS_NOINTERFACE;
  return mailEditor->StripCites();
}

NS_IMETHODIMP    
nsEditorShell::SelectAll()
{  
  nsresult  err = NS_NOINTERFACE;
  
  switch (mEditorType)
  {
    case ePlainTextEditorType:
    case eHTMLTextEditorType:
      {
        nsCOMPtr<nsIEditor>  editor = do_QueryInterface(mEditor);
        if (editor)
          err = editor->SelectAll();
      }
      break;

    default:
      err = NS_ERROR_NOT_IMPLEMENTED;
  }

  return err;
}


NS_IMETHODIMP    
nsEditorShell::DeleteSelection(PRInt32 action)
{  
  nsresult  err = NS_NOINTERFACE;
  nsIEditor::EDirection selectionAction;

  switch(action)
  {
    case 1:
      selectionAction = nsIEditor::eNext;
      break;
    case 2:
      selectionAction = nsIEditor::ePrevious;
      break;
    default:
      selectionAction = nsIEditor::eNone;
      break;
  }

  nsCOMPtr<nsIEditor> editor = do_QueryInterface(mEditor);
  if (editor)
    err = editor->DeleteSelection(selectionAction);
  
  return err;
}

NS_IMETHODIMP
nsEditorShell::InsertText(const PRUnichar *textToInsert)
{
  nsresult  err = NS_NOINTERFACE;
  
  switch (mEditorType)
  {
    case ePlainTextEditorType:
    case eHTMLTextEditorType:
      {
        nsCOMPtr<nsIPlaintextEditor> textEditor (do_QueryInterface(mEditor));
        if (textEditor)
          err = textEditor->InsertText(nsDependentString(textToInsert));
      }
      break;

    default:
      err = NS_ERROR_NOT_IMPLEMENTED;
  }

  return err;
}

NS_IMETHODIMP
nsEditorShell::InsertSource(const PRUnichar *aSourceToInsert)
{
  nsresult  err = NS_NOINTERFACE;
  
  switch (mEditorType)
  {
    case ePlainTextEditorType:
    case eHTMLTextEditorType:
      {
        nsCOMPtr<nsIHTMLEditor>  htmlEditor = do_QueryInterface(mEditor);
        if (htmlEditor)
          err = htmlEditor->InsertHTML(nsDependentString(aSourceToInsert));
      }
      break;

    default:
      err = NS_NOINTERFACE;
  }

  return err;
}

NS_IMETHODIMP
nsEditorShell::InsertSourceWithCharset(const PRUnichar *aSourceToInsert,
                                       const PRUnichar *aCharset)
{
  nsresult  err = NS_NOINTERFACE;
  
  switch (mEditorType)
  {
    case ePlainTextEditorType:
    case eHTMLTextEditorType:
      {
        nsCOMPtr<nsIHTMLEditor>  htmlEditor = do_QueryInterface(mEditor);
        if (htmlEditor)
          err = htmlEditor->InsertHTMLWithCharset(nsDependentString(aSourceToInsert), nsDependentString(aCharset));
      }
      break;

    default:
      err = NS_NOINTERFACE;
  }

  return err;
}

NS_IMETHODIMP    
nsEditorShell::RebuildDocumentFromSource(const PRUnichar *aSource)
{
  nsresult  err = NS_NOINTERFACE;
  
  nsAutoString source(aSource);
  
  switch (mEditorType)
  {
    case ePlainTextEditorType:
    case eHTMLTextEditorType:
      {
        nsCOMPtr<nsIHTMLEditor>  htmlEditor = do_QueryInterface(mEditor);
        if (htmlEditor)
          err = htmlEditor->RebuildDocumentFromSource(source);
      }
      break;

    default:
      err = NS_NOINTERFACE;
  }

  return err;
}

NS_IMETHODIMP
nsEditorShell::InsertBreak()
{
  nsCOMPtr<nsIPlaintextEditor> textEditor (do_QueryInterface(mEditor));
  if (!textEditor)
    return NS_NOINTERFACE;

  return textEditor->InsertLineBreak();
}

/* Get localized strings for UI from the Editor's string bundle */
// Use this version from JavaScript:
NS_IMETHODIMP
nsEditorShell::GetString(const PRUnichar *stringName, PRUnichar **_retval)
{
  NS_ENSURE_ARG_POINTER(stringName);
  NS_ENSURE_ARG_POINTER(_retval);

  *_retval = NULL;

  if (!mStringBundle)
  {
    nsresult res;
    nsCOMPtr<nsIStringBundleService> stringBundleService = do_GetService(kCStringBundleServiceCID, &res);
    if (NS_FAILED(res))
      return res;
    if (stringBundleService)
      res = stringBundleService->CreateBundle(EDITOR_BUNDLE_URL, getter_AddRefs(mStringBundle));

    NS_ASSERTION(mStringBundle, "No string bundle!");
    if (!mStringBundle) return NS_ERROR_NOT_INITIALIZED;
  }

  return mStringBundle->GetStringFromName(stringName, _retval);
}


// Use this version within the shell:
void nsEditorShell::GetBundleString(const nsAString &stringName, nsAString &outString)
{
  outString.Truncate();
  
  nsXPIDLString   tempString;
  if (NS_SUCCEEDED(GetString(PromiseFlatString(stringName).get(), getter_Copies(tempString))) && tempString)
    outString = tempString.get();
}

void
nsEditorShell::Alert(const nsString& aTitle, const nsString& aMsg)
{
  nsresult rv;
  nsCOMPtr<nsIPromptService> dialog(do_GetService("@mozilla.org/embedcomp/prompt-service;1"));
  if (dialog)
  {
    if(!mContentWindow)
      return;
    nsCOMPtr<nsIDOMWindow> cwP = do_QueryReferent(mContentWindow);
    if (!cwP) return;
    rv = dialog->Alert(cwP, aTitle.get(), aMsg.get());
  }
}

NS_IMETHODIMP
nsEditorShell::GetDocumentCharacterSet(PRUnichar** characterSet)
{
  if (!characterSet)
      return NS_ERROR_NULL_POINTER;
  nsCOMPtr<nsIEditor> editor = do_QueryInterface(mEditor);
  nsAutoString copiedData;
  *characterSet = nsnull;
  if (editor)
  {
    if (NS_SUCCEEDED(editor->GetDocumentCharacterSet(copiedData)))
    {
      *characterSet = ToNewUnicode(copiedData);
      return NS_OK;
    }
  }
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsEditorShell::SetDocumentCharacterSet(const PRUnichar* characterSet)
{
  nsCOMPtr<nsIEditor> editor = do_QueryInterface(mEditor);

  nsresult res = NS_OK;
  if (editor)
    res = editor->SetDocumentCharacterSet(nsAutoString(characterSet));

  nsCOMPtr<nsIDocShell> docShell;
  res = GetDocShellFromContentWindow(getter_AddRefs(docShell));
  if (NS_FAILED(res)) return res;
  if (!docShell) return NS_ERROR_FAILURE;
  
  nsCOMPtr<nsIContentViewer> childCV;
  NS_ENSURE_SUCCESS(docShell->GetContentViewer(getter_AddRefs(childCV)), NS_ERROR_FAILURE);
  if (childCV)
  {
    nsCOMPtr<nsIMarkupDocumentViewer> markupCV = do_QueryInterface(childCV);
    if (markupCV) {
      NS_ENSURE_SUCCESS(markupCV->SetDefaultCharacterSet(characterSet), NS_ERROR_FAILURE);
      NS_ENSURE_SUCCESS(markupCV->SetForceCharacterSet(characterSet), NS_ERROR_FAILURE);
    }
  }
  return res;
}

NS_IMETHODIMP
nsEditorShell::GetContentsAs(const PRUnichar *format, PRUint32 flags,
                             PRUnichar **aContentsAs)
{
  nsresult  err = NS_NOINTERFACE;

  nsAutoString aFormat (format);
  nsAutoString contentsAs;

  nsCOMPtr<nsIEditor> editor = do_QueryInterface(mEditor);
  if (editor)
    err = editor->OutputToString(aFormat, flags, contentsAs);

  *aContentsAs = ToNewUnicode(contentsAs);
  
  return err;
}

NS_IMETHODIMP
nsEditorShell::GetHeadContentsAsHTML(PRUnichar **aHeadContents)
{
  nsresult  err = NS_NOINTERFACE;

  nsAutoString headContents;

  nsCOMPtr<nsIHTMLEditor> editor = do_QueryInterface(mEditor);
  if (editor)
    err = editor->GetHeadContentsAsHTML(headContents);

  *aHeadContents = ToNewUnicode(headContents);
  
  return err;
}

NS_IMETHODIMP
nsEditorShell::ReplaceHeadContentsWithHTML(const PRUnichar *aSourceToInsert)
{
  nsresult  err = NS_NOINTERFACE;
  
  nsAutoString sourceToInsert(aSourceToInsert);
  
  switch (mEditorType)
  {
    case ePlainTextEditorType:
    case eHTMLTextEditorType:
      {
        nsCOMPtr<nsIHTMLEditor>  htmlEditor = do_QueryInterface(mEditor);
        if (htmlEditor)
          err = htmlEditor->ReplaceHeadContentsWithHTML(sourceToInsert);
      }
      break;

    default:
      err = NS_NOINTERFACE;
  }

  return err;
}

NS_IMETHODIMP
nsEditorShell::DumpContentTree()
{
  nsCOMPtr<nsIEditor> editor = do_QueryInterface(mEditor);
  if (!editor)
    return NS_ERROR_NOT_INITIALIZED;
  return editor->DumpContentTree();
}

NS_IMETHODIMP
nsEditorShell::GetWrapColumn(PRInt32* aWrapColumn)
{
  nsresult  err = NS_NOINTERFACE;
  
  if (!aWrapColumn)
    return NS_ERROR_NULL_POINTER;
  
  // fill result in case of failure
  *aWrapColumn = mWrapColumn;

  // If we don't have an editor yet, say we're not initialized
  // even though mWrapColumn may have a value.
  if (!mEditor)
    return NS_ERROR_NOT_INITIALIZED;

  switch (mEditorType)
  {
    case ePlainTextEditorType:
      {
        nsCOMPtr<nsIPlaintextEditor> textEditor = do_QueryInterface(mEditor);
        if (textEditor)
        {
          PRInt32 wc;
          err = textEditor->GetWrapWidth(&wc);
          if (NS_SUCCEEDED(err))
            *aWrapColumn = (PRInt32)wc;
        }
      }
      break;
    default:
      err = NS_ERROR_NOT_IMPLEMENTED;
  }

  return err;
}

NS_IMETHODIMP
nsEditorShell::SetWrapColumn(PRInt32 aWrapColumn)
{
  nsresult  err = NS_OK;

  mWrapColumn = aWrapColumn;

  if (mEditor)
  {
    switch (mEditorType)
    {
        case ePlainTextEditorType:
        {
          nsCOMPtr<nsIPlaintextEditor> textEditor = do_QueryInterface(mEditor);
          if (textEditor)
            err = textEditor->SetWrapWidth(mWrapColumn);
        }
        break;
        default:
          err = NS_ERROR_NOT_IMPLEMENTED;
    }
  }

  return err;
}

NS_IMETHODIMP
nsEditorShell::SetParagraphFormat(const PRUnichar * paragraphFormat)
{
  nsresult  err = NS_NOINTERFACE;
  
  if (!mEditor) return NS_ERROR_NOT_INITIALIZED;

  nsAutoString aParagraphFormat(paragraphFormat);
  
  switch (mEditorType)
  {
    case eHTMLTextEditorType:
      err = mEditor->SetParagraphFormat(aParagraphFormat);
      break;

    default:
      err = NS_ERROR_NOT_IMPLEMENTED;
  }

  return err;
}

NS_IMETHODIMP
nsEditorShell::GetEditorDocument(nsIDOMDocument** aEditorDocument)
{
  if (mEditor)
  {
    nsCOMPtr<nsIEditor>  editor = do_QueryInterface(mEditor);
    if (editor)
    {
      return editor->GetDocument(aEditorDocument);
    }
  }
  return NS_NOINTERFACE;
}

 
NS_IMETHODIMP
nsEditorShell::GetEditor(nsIEditor** aEditor)
{
  if (mEditor)
    return mEditor->QueryInterface(NS_GET_IID(nsIEditor), (void **)aEditor);		// the QI does the addref

  *aEditor = nsnull;
  return NS_ERROR_NOT_INITIALIZED;
}


NS_IMETHODIMP
nsEditorShell::GetEditorSelection(nsISelection** aEditorSelection)
{
  nsCOMPtr<nsIEditor>  editor = do_QueryInterface(mEditor);
  if (editor)
      return editor->GetSelection(aEditorSelection);
  
  return NS_NOINTERFACE;
}

NS_IMETHODIMP
nsEditorShell::GetSelectionController(nsISelectionController** aSelectionController)
{
  nsCOMPtr<nsIEditor> editor = do_QueryInterface(mEditor);
  if (!editor)
    return NS_ERROR_NOT_INITIALIZED;

  nsCOMPtr<nsISelectionController> selCont;
  nsresult rv = editor->GetSelectionController(getter_AddRefs(selCont));
  if (NS_FAILED(rv))
    return rv;

  if (!selCont)
    return NS_ERROR_NO_INTERFACE;
  *aSelectionController = selCont;
  NS_IF_ADDREF(*aSelectionController);
  return NS_OK;
}

NS_IMETHODIMP
nsEditorShell::GetDocumentModified(PRBool *aDocumentModified)
{
  nsCOMPtr<nsIEditor> editor = do_QueryInterface(mEditor);
  if (editor)
    return editor->GetDocumentModified(aDocumentModified);

  return NS_NOINTERFACE;
}


NS_IMETHODIMP
nsEditorShell::GetDocumentIsEmpty(PRBool *aDocumentIsEmpty)
{
  nsCOMPtr<nsIEditor> editor = do_QueryInterface(mEditor);
  if (editor)
    return editor->GetDocumentIsEmpty(aDocumentIsEmpty);

  return NS_NOINTERFACE;
}


NS_IMETHODIMP
nsEditorShell::GetDocumentEditable(PRBool *aDocumentEditable)
{
  *aDocumentEditable = PR_FALSE;  // default return value
  nsCOMPtr<nsIEditor> editor = do_QueryInterface(mEditor);
  if (!editor) return NS_OK;
  
  PRUint32  editorFlags;
  editor->GetFlags(&editorFlags);
  
  if (editorFlags & nsIPlaintextEditor::eEditorReadonlyMask)
    return NS_OK;
  
  nsCOMPtr<nsIDOMDocument> doc;
  editor->GetDocument(getter_AddRefs(doc));
  if (!doc) return NS_OK;

  *aDocumentEditable = PR_TRUE;
  return NS_OK;
}


NS_IMETHODIMP
nsEditorShell::GetDocumentLength(PRInt32 *aDocumentLength)
{
  nsCOMPtr<nsIPlaintextEditor> textEditor = do_QueryInterface(mEditor);
  if (textEditor)
    return textEditor->GetTextLength(aDocumentLength);

  return NS_NOINTERFACE;
}

NS_IMETHODIMP
nsEditorShell::DoAfterSave(PRBool aShouldUpdateURL, const PRUnichar *aURLString)
{
  if (aShouldUpdateURL)
  {
    NS_ENSURE_ARG_POINTER(aURLString);
    nsCOMPtr<nsIURI> uri;
    nsresult rv = NS_NewURI(getter_AddRefs(uri), nsDependentString(aURLString), nsnull);
    NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to create a uri");
    if (NS_SUCCEEDED(rv))
    {
      mContentAreaDocShell->SetCurrentURI(uri);
    }
  }

  // Update window title to show possibly different filename
  // This also covers problem that after undoing a title change,
  //   window title loses the extra [filename] part that this adds
  return UpdateWindowTitleAndRecentMenu(PR_TRUE);
}

NS_IMETHODIMP
nsEditorShell::MakeOrChangeList(const PRUnichar *listType, PRBool entireList,
                                const PRUnichar *bulletType)
{
  nsresult err = NS_NOINTERFACE;

  if (!mEditor) return NS_ERROR_NOT_INITIALIZED;

  nsAutoString aListType(listType);
  nsAutoString aBulletType;
  if (bulletType) {
    aBulletType.Assign(bulletType);
  }

  switch (mEditorType)
  {
    case eHTMLTextEditorType:
      if (aListType.IsEmpty())
      {
        err = mEditor->RemoveList(NS_LITERAL_STRING("ol"));
        if(NS_SUCCEEDED(err))
        {
          err = mEditor->RemoveList(NS_LITERAL_STRING("ul"));
          if(NS_SUCCEEDED(err))
            err = mEditor->RemoveList(NS_LITERAL_STRING("dl"));
        }
      }
      else
        err = mEditor->MakeOrChangeList(aListType, entireList, aBulletType);
      break;

    case ePlainTextEditorType:
    default:
      err = NS_ERROR_NOT_IMPLEMENTED;
  }
  return err;
}


NS_IMETHODIMP
nsEditorShell::RemoveList(const PRUnichar *listType)
{
  nsresult err = NS_NOINTERFACE;

  if (!mEditor) return NS_ERROR_NOT_INITIALIZED;
  
  switch (mEditorType)
  {
    case eHTMLTextEditorType:
    {
      nsAutoString aListType(listType);
      err = mEditor->RemoveList(aListType);
      break;
    }

    case ePlainTextEditorType:
    default:
      err = NS_ERROR_NOT_IMPLEMENTED;
  }

  return err;
}

NS_IMETHODIMP
nsEditorShell::Indent(const PRUnichar *indent)
{
  nsresult err = NS_NOINTERFACE;

  if (!mEditor) return NS_ERROR_NOT_INITIALIZED;

  nsAutoString aIndent(indent);
  
  switch (mEditorType)
  {
    case eHTMLTextEditorType:
      err = mEditor->Indent(aIndent);
      break;

    case ePlainTextEditorType:
    default:
      err = NS_ERROR_NOT_IMPLEMENTED;
  }

  return err;
}

NS_IMETHODIMP
nsEditorShell::Align(const PRUnichar *align)
{
  nsresult err = NS_NOINTERFACE;

  if (!mEditor) return NS_ERROR_NOT_INITIALIZED;

  nsAutoString aAlignType(align);
  
  switch (mEditorType)
  {
    case eHTMLTextEditorType:
      err = mEditor->Align(aAlignType);
      break;

    case ePlainTextEditorType:
    default:
      err = NS_ERROR_NOT_IMPLEMENTED;
  }

  return err;
}

NS_IMETHODIMP
nsEditorShell::GetSelectedElement(const PRUnichar *aInTagName, nsIDOMElement **aOutElement)
{
  if (!aInTagName || !aOutElement)
    return NS_ERROR_NULL_POINTER;

  if (!mEditor) return NS_ERROR_NOT_INITIALIZED;
  
  nsresult  result = NS_NOINTERFACE;
  nsAutoString tagName(aInTagName);
  
  switch (mEditorType)
  {
    case eHTMLTextEditorType:
      result = mEditor->GetSelectedElement(tagName, aOutElement);
      // Don't return NS_EDITOR_ELEMENT_NOT_FOUND (passes NS_SUCCEEDED macro)
      //  to JavaScript
      if(NS_SUCCEEDED(result)) return NS_OK;
      break;

    case ePlainTextEditorType:
    default:
      result = NS_ERROR_NOT_IMPLEMENTED;
  }

  return result;
}

NS_IMETHODIMP
nsEditorShell::GetFirstSelectedCell(nsIDOMElement **aOutElement)
{
  if (!aOutElement)
    return NS_ERROR_NULL_POINTER;

  nsresult  result = NS_NOINTERFACE;
  
  switch (mEditorType)
  {
    case eHTMLTextEditorType:
    {
      nsCOMPtr<nsITableEditor> tableEditor = do_QueryInterface(mEditor);
      if (tableEditor)
      {
        result = tableEditor->GetFirstSelectedCell(nsnull, aOutElement);
        // Don't return NS_EDITOR_ELEMENT_NOT_FOUND (passes NS_SUCCEEDED macro)
        //  to JavaScript
        if(NS_SUCCEEDED(result)) return NS_OK;
      }
      
      break;
    }

    case ePlainTextEditorType:
    default:
      result = NS_ERROR_NOT_IMPLEMENTED;
  }

  return result;
}

NS_IMETHODIMP 
nsEditorShell::GetFirstSelectedCellInTable(PRInt32 *aRowIndex, PRInt32 *aColIndex, nsIDOMElement **aOutElement)
{
  if (!aOutElement || !aRowIndex || !aColIndex)
    return NS_ERROR_NULL_POINTER;

  nsresult  result = NS_NOINTERFACE;
  
  switch (mEditorType)
  {
    case eHTMLTextEditorType:
    {
      nsCOMPtr<nsITableEditor> tableEditor = do_QueryInterface(mEditor);
      if (tableEditor)
      {
        result = tableEditor->GetFirstSelectedCellInTable(aRowIndex, aColIndex, aOutElement);
        // Don't return NS_EDITOR_ELEMENT_NOT_FOUND (passes NS_SUCCEEDED macro)
        //  to JavaScript
        if(NS_SUCCEEDED(result)) return NS_OK;
      }
      
      break;
    }

    case ePlainTextEditorType:
    default:
      result = NS_ERROR_NOT_IMPLEMENTED;
  }

  return result;
}

NS_IMETHODIMP
nsEditorShell::GetNextSelectedCell(nsIDOMElement **aOutElement)
{
  if (!aOutElement)
    return NS_ERROR_NULL_POINTER;

  nsresult  result = NS_NOINTERFACE;
  
  switch (mEditorType)
  {
    case eHTMLTextEditorType:
    {
      nsCOMPtr<nsITableEditor> tableEditor = do_QueryInterface(mEditor);
      if (tableEditor)
        result = tableEditor->GetNextSelectedCell(nsnull, aOutElement);
      break;
    }
    case ePlainTextEditorType:
    default:
      result = NS_ERROR_NOT_IMPLEMENTED;
  }

  return result;
}


NS_IMETHODIMP
nsEditorShell::GetElementOrParentByTagName(const PRUnichar *aInTagName, nsIDOMNode *node, nsIDOMElement **aOutElement)
{
  //node can be null -- this signals using the selection anchorNode
  if (!aInTagName || !aOutElement)
    return NS_ERROR_NULL_POINTER;

  if (!mEditor) return NS_ERROR_NOT_INITIALIZED;

  nsresult  result = NS_NOINTERFACE;
  nsAutoString tagName(aInTagName);
  
  switch (mEditorType)
  {
    case eHTMLTextEditorType:
      result = mEditor->GetElementOrParentByTagName(tagName, node, aOutElement);
      // Don't return NS_EDITOR_ELEMENT_NOT_FOUND (passes NS_SUCCEEDED macro)
      //  to JavaScript
      if(NS_SUCCEEDED(result)) return NS_OK;
      break;

    case ePlainTextEditorType:
    default:
      result = NS_ERROR_NOT_IMPLEMENTED;
  }

  return result;
}

NS_IMETHODIMP
nsEditorShell::CreateElementWithDefaults(const PRUnichar *aInTagName, nsIDOMElement **aOutElement)
{
  if (!aOutElement)
    return NS_ERROR_NULL_POINTER;

  if (!mEditor) return NS_ERROR_NOT_INITIALIZED;

  nsresult  result = NS_NOINTERFACE;
  nsAutoString tagName(aInTagName);

  switch (mEditorType)
  {
    case eHTMLTextEditorType:
      result = mEditor->CreateElementWithDefaults(tagName, aOutElement);
      break;

    case ePlainTextEditorType:
    default:
      result = NS_ERROR_NOT_IMPLEMENTED;
  }

  return result;
}

NS_IMETHODIMP
nsEditorShell::DeleteElement(nsIDOMElement *element)
{
  if (!element)
    return NS_ERROR_NULL_POINTER;

  nsresult  result = NS_NOINTERFACE;
  nsCOMPtr<nsIEditor> editor = do_QueryInterface(mEditor);
  if (editor) {
    // The nsIEditor::DeleteNode() wants a node
    //   but it actually requires that it is an element!
    nsCOMPtr<nsIDOMNode> node = do_QueryInterface(element);
    result = editor->DeleteNode(node);
  }
  return result;
}

NS_IMETHODIMP
nsEditorShell::InsertElement(nsIDOMElement *element, nsIDOMElement *parent, PRInt32 position, PRBool dontChangeSelection)
{
  if (!element || !parent)
    return NS_ERROR_NULL_POINTER;

  nsresult  result = NS_NOINTERFACE;
  nsCOMPtr<nsIEditor> editor = do_QueryInterface(mEditor);
  if (editor) 
  {
    // Set flag so InsertElementTxn doesn't change the selection
    if (dontChangeSelection)
      editor->SetShouldTxnSetSelection(PR_FALSE);

    // The nsIEditor::InsertNode() wants nodes as params,
    //   but it actually requires that they are elements!
    nsCOMPtr<nsIDOMNode> node = do_QueryInterface(element);
    nsCOMPtr<nsIDOMNode> parentNode = do_QueryInterface(parent);
    result = editor->InsertNode(node, parentNode, position);

    if (dontChangeSelection)
      editor->SetShouldTxnSetSelection(PR_TRUE);
  }
  return result;
}

NS_IMETHODIMP
nsEditorShell::InsertElementAtSelection(nsIDOMElement *element, PRBool deleteSelection)
{
  if (!element)
    return NS_ERROR_NULL_POINTER;

  if (!mEditor) return NS_ERROR_NOT_INITIALIZED;

  nsresult  result = NS_NOINTERFACE;
  switch (mEditorType)
  {
    case eHTMLTextEditorType:
      result = mEditor->InsertElementAtSelection(element, deleteSelection);
      break;

    case ePlainTextEditorType:
    default:
      result = NS_ERROR_NOT_IMPLEMENTED;
  }

  return result;
}

NS_IMETHODIMP
nsEditorShell::InsertLinkAroundSelection(nsIDOMElement* aAnchorElement)
{
  nsresult  result = NS_NOINTERFACE;

  if (!mEditor) return NS_ERROR_NOT_INITIALIZED;

  switch (mEditorType)
  {
    case eHTMLTextEditorType:
      result = mEditor->InsertLinkAroundSelection(aAnchorElement);
      break;

    case ePlainTextEditorType:
    default:
      result = NS_ERROR_NOT_IMPLEMENTED;
  }
  return result;
}

NS_IMETHODIMP    
nsEditorShell::SelectElement(nsIDOMElement* aElement)
{
  if (!aElement)
    return NS_ERROR_NULL_POINTER;

  if (!mEditor) return NS_ERROR_NOT_INITIALIZED;

  nsresult  result = NS_NOINTERFACE;
  switch (mEditorType)
  {
    case eHTMLTextEditorType:
      result = mEditor->SelectElement(aElement);
      break;

    case ePlainTextEditorType:
    default:
      result = NS_ERROR_NOT_IMPLEMENTED;
  }

  return result;
}

NS_IMETHODIMP    
nsEditorShell::SetSelectionAfterElement(nsIDOMElement* aElement)
{
  if (!aElement)
    return NS_ERROR_NULL_POINTER;

  if (!mEditor) return NS_ERROR_NOT_INITIALIZED;

  nsresult  result = NS_NOINTERFACE;
  switch (mEditorType)
  {
    case eHTMLTextEditorType:
      result = mEditor->SetCaretAfterElement(aElement);
      break;

    case ePlainTextEditorType:
    default:
      result = NS_ERROR_NOT_IMPLEMENTED;
  }

  return result;
}

/* Table Editing */
NS_IMETHODIMP    
nsEditorShell::InsertTableRow(PRInt32 aNumber, PRBool bAfter)
{
  nsresult  result = NS_NOINTERFACE;
  switch (mEditorType)
  {
    case eHTMLTextEditorType:
      {
        nsCOMPtr<nsITableEditor> tableEditor = do_QueryInterface(mEditor);
        if (tableEditor)
          result = tableEditor->InsertTableRow(aNumber,bAfter);
      }
      break;

    case ePlainTextEditorType:
    default:
      result = NS_ERROR_NOT_IMPLEMENTED;
  }
  return result;
}

NS_IMETHODIMP    
nsEditorShell::InsertTableColumn(PRInt32 aNumber, PRBool bAfter)
{
  nsresult  result = NS_NOINTERFACE;
  switch (mEditorType)
  {
    case eHTMLTextEditorType:
      {
        nsCOMPtr<nsITableEditor> tableEditor = do_QueryInterface(mEditor);
        if (tableEditor)
          result = tableEditor->InsertTableColumn(aNumber,bAfter);
      }
      break;
    default:
      result = NS_ERROR_NOT_IMPLEMENTED;
  }
  return result;
}

NS_IMETHODIMP    
nsEditorShell::InsertTableCell(PRInt32 aNumber, PRBool bAfter)
{
  nsresult  result = NS_NOINTERFACE;
  switch (mEditorType)
  {
    case eHTMLTextEditorType:
      {
        nsCOMPtr<nsITableEditor> tableEditor = do_QueryInterface(mEditor);
        if (tableEditor)
        {
          BeginBatchChanges();
          result = tableEditor->InsertTableCell(aNumber, bAfter);
          if (NS_SUCCEEDED(result))
          {
            // Fix disturbances in table layout because of inserted cells
            result = CheckPrefAndNormalizeTable();
          }
          EndBatchChanges();
          return result;
        }
      }
      break;

    case ePlainTextEditorType:
    default:
      result = NS_ERROR_NOT_IMPLEMENTED;
  }
  return result;
}

NS_IMETHODIMP    
nsEditorShell::DeleteTable()
{
  nsresult  result = NS_NOINTERFACE;
  switch (mEditorType)
  {
    case eHTMLTextEditorType:
      {
        nsCOMPtr<nsITableEditor> tableEditor = do_QueryInterface(mEditor);
        if (tableEditor)
        {
          result = tableEditor->DeleteTable();
          // Don't return NS_EDITOR_ELEMENT_NOT_FOUND (passes NS_SUCCEEDED macro)
          //  to JavaScript
          if(NS_SUCCEEDED(result)) return NS_OK;
        }
      }
      break;
    default:
      result = NS_ERROR_NOT_IMPLEMENTED;
  }
  return result;
}

NS_IMETHODIMP    
nsEditorShell::DeleteTableCell(PRInt32 aNumber)
{
  nsresult  result = NS_NOINTERFACE;
  switch (mEditorType)
  {
    case eHTMLTextEditorType:
      {
        nsCOMPtr<nsITableEditor> tableEditor = do_QueryInterface(mEditor);
        if (tableEditor)
        {
          BeginBatchChanges();
          result = tableEditor->DeleteTableCell(aNumber);
          if(NS_SUCCEEDED(result))
          {
            // Fix disturbances in table layout because of deleted cells
            result = CheckPrefAndNormalizeTable();
          }
          EndBatchChanges();
          return result;
        }
      }
      break;
    default:
      result = NS_ERROR_NOT_IMPLEMENTED;
  }
  return result;
}

NS_IMETHODIMP    
nsEditorShell::DeleteTableCellContents()
{
  nsresult  result = NS_NOINTERFACE;
  switch (mEditorType)
  {
    case eHTMLTextEditorType:
      {
        nsCOMPtr<nsITableEditor> tableEditor = do_QueryInterface(mEditor);
        if (tableEditor)
        {
          result = tableEditor->DeleteTableCellContents();
          // Don't return NS_EDITOR_ELEMENT_NOT_FOUND (passes NS_SUCCEEDED macro)
          //  to JavaScript
          if(NS_SUCCEEDED(result)) return NS_OK;
        }
      }
      break;
    default:
      result = NS_ERROR_NOT_IMPLEMENTED;
  }
  return result;
}

NS_IMETHODIMP    
nsEditorShell::DeleteTableRow(PRInt32 aNumber)
{
  nsresult  result = NS_NOINTERFACE;
  switch (mEditorType)
  {
    case eHTMLTextEditorType:
      {
        nsCOMPtr<nsITableEditor> tableEditor = do_QueryInterface(mEditor);
        if (tableEditor)
        {
          result = tableEditor->DeleteTableRow(aNumber);
          // Don't return NS_EDITOR_ELEMENT_NOT_FOUND (passes NS_SUCCEEDED macro)
          //  to JavaScript
          if(NS_SUCCEEDED(result)) return NS_OK;
        }
      }
      break;
    default:
      result = NS_ERROR_NOT_IMPLEMENTED;
  }
  return result;
}


NS_IMETHODIMP    
nsEditorShell::DeleteTableColumn(PRInt32 aNumber)
{
  nsresult  result = NS_NOINTERFACE;
  switch (mEditorType)
  {
    case eHTMLTextEditorType:
      {
        nsCOMPtr<nsITableEditor> tableEditor = do_QueryInterface(mEditor);
        if (tableEditor)
        {
          result = tableEditor->DeleteTableColumn(aNumber);
          // Don't return NS_EDITOR_ELEMENT_NOT_FOUND (passes NS_SUCCEEDED macro)
          //  to JavaScript
          if(NS_SUCCEEDED(result)) return NS_OK;
        }
      }
      break;
    default:
      result = NS_ERROR_NOT_IMPLEMENTED;
  }
  return result;
}

NS_IMETHODIMP 
nsEditorShell::SwitchTableCellHeaderType(nsIDOMElement *aSourceCell, nsIDOMElement **aNewCell)
{
  nsresult  result = NS_NOINTERFACE;
  switch (mEditorType)
  {
    case eHTMLTextEditorType:
      {
        nsCOMPtr<nsITableEditor> tableEditor = do_QueryInterface(mEditor);
        if (tableEditor)
          result = tableEditor->SwitchTableCellHeaderType(aSourceCell, aNewCell);
      }
      break;
    default:
      result = NS_ERROR_NOT_IMPLEMENTED;
  }
  return result;
}


NS_IMETHODIMP    
nsEditorShell::JoinTableCells(PRBool aMergeNonContiguousContents)
{
  nsresult  result = NS_NOINTERFACE;
  switch (mEditorType)
  {
    case eHTMLTextEditorType:
      {
        nsCOMPtr<nsITableEditor> tableEditor = do_QueryInterface(mEditor);
        if (tableEditor)
          result = tableEditor->JoinTableCells(aMergeNonContiguousContents);
      }
      break;
    default:
      result = NS_ERROR_NOT_IMPLEMENTED;
  }
  return result;
}

NS_IMETHODIMP    
nsEditorShell::SplitTableCell()
{
  nsresult  result = NS_NOINTERFACE;
  switch (mEditorType)
  {
    case eHTMLTextEditorType:
      {
        nsCOMPtr<nsITableEditor> tableEditor = do_QueryInterface(mEditor);
        if (tableEditor)
          result = tableEditor->SplitTableCell();
      }
      break;
    default:
      result = NS_ERROR_NOT_IMPLEMENTED;
  }
  return result;
}

NS_IMETHODIMP    
nsEditorShell::SelectTableCell()
{
  nsresult  result = NS_NOINTERFACE;
  switch (mEditorType)
  {
    case eHTMLTextEditorType:
      {
        nsCOMPtr<nsITableEditor> tableEditor = do_QueryInterface(mEditor);
        if (tableEditor)
          result = tableEditor->SelectTableCell();
      }
      break;
    default:
      result = NS_ERROR_NOT_IMPLEMENTED;
  }
  return result;
}

NS_IMETHODIMP    
nsEditorShell::SelectBlockOfCells(nsIDOMElement *aStartCell, nsIDOMElement *aEndCell)
{
  nsresult  result = NS_NOINTERFACE;
  switch (mEditorType)
  {
    case eHTMLTextEditorType:
      {
        nsCOMPtr<nsITableEditor> tableEditor = do_QueryInterface(mEditor);
        if (tableEditor)
          result = tableEditor->SelectBlockOfCells(aStartCell, aEndCell);
      }
      break;
    default:
      result = NS_ERROR_NOT_IMPLEMENTED;
  }
  return result;
}


NS_IMETHODIMP    
nsEditorShell::SelectTableRow()
{
  nsresult  result = NS_NOINTERFACE;
  switch (mEditorType)
  {
    case eHTMLTextEditorType:
      {
        nsCOMPtr<nsITableEditor> tableEditor = do_QueryInterface(mEditor);
        if (tableEditor)
          result = tableEditor->SelectTableRow();
      }
      break;
    default:
      result = NS_ERROR_NOT_IMPLEMENTED;
  }
  return result;
}

NS_IMETHODIMP    
nsEditorShell::SelectTableColumn()
{
  nsresult  result = NS_NOINTERFACE;
  switch (mEditorType)
  {
    case eHTMLTextEditorType:
      {
        nsCOMPtr<nsITableEditor> tableEditor = do_QueryInterface(mEditor);
        if (tableEditor)
          result = tableEditor->SelectTableColumn();
      }
      break;
    default:
      result = NS_ERROR_NOT_IMPLEMENTED;
  }
  return result;
}

NS_IMETHODIMP    
nsEditorShell::SelectTable()
{
  nsresult  result = NS_NOINTERFACE;
  switch (mEditorType)
  {
    case eHTMLTextEditorType:
      {
        nsCOMPtr<nsITableEditor> tableEditor = do_QueryInterface(mEditor);
        if (tableEditor)
          result = tableEditor->SelectTable();
      }
      break;
    default:
      result = NS_ERROR_NOT_IMPLEMENTED;
  }
  return result;
}

NS_IMETHODIMP    
nsEditorShell::SelectAllTableCells()
{
  nsresult  result = NS_NOINTERFACE;
  switch (mEditorType)
  {
    case eHTMLTextEditorType:
      {
        nsCOMPtr<nsITableEditor> tableEditor = do_QueryInterface(mEditor);
        if (tableEditor)
          result = tableEditor->SelectAllTableCells();
      }
      break;
    default:
      result = NS_ERROR_NOT_IMPLEMENTED;
  }
  return result;
}

NS_IMETHODIMP 
nsEditorShell::NormalizeTable(nsIDOMElement *aTable)
{
  nsresult  result = NS_NOINTERFACE;
  switch (mEditorType)
  {
    case eHTMLTextEditorType:
      {
        nsCOMPtr<nsITableEditor> tableEditor = do_QueryInterface(mEditor);
        if (tableEditor)
          result = tableEditor->NormalizeTable(aTable);
      }
      break;
    default:
      result = NS_ERROR_NOT_IMPLEMENTED;
  }
  return result;
}

// The next four methods are factored to return single items 
//  separately for row and column. 
//  Underlying implementation gets both at the same time for efficiency.

NS_IMETHODIMP    
nsEditorShell::GetRowIndex(nsIDOMElement *cellElement, PRInt32 *_retval)
{
  if (!_retval)
    return NS_ERROR_NULL_POINTER;

  nsresult  result = NS_NOINTERFACE;
  switch (mEditorType)
  {
    case eHTMLTextEditorType:
      {
        nsCOMPtr<nsITableEditor> tableEditor = do_QueryInterface(mEditor);
        if (tableEditor)
        {
          // Get both row and column indexes - return just row
          PRInt32 colIndex;
          result = tableEditor->GetCellIndexes(cellElement, _retval, &colIndex);
        }
      }
      break;
    default:
      result = NS_ERROR_NOT_IMPLEMENTED;
  }
  return result;
}

NS_IMETHODIMP    
nsEditorShell::GetColumnIndex(nsIDOMElement *cellElement, PRInt32 *_retval)
{
  if (!_retval)
    return NS_ERROR_NULL_POINTER;

  nsresult  result = NS_NOINTERFACE;
  switch (mEditorType)
  {
    case eHTMLTextEditorType:
      {
        nsCOMPtr<nsITableEditor> tableEditor = do_QueryInterface(mEditor);
        if (tableEditor)
        {
          // Get both row and column indexes - return just column
          PRInt32 rowIndex;
          result = tableEditor->GetCellIndexes(cellElement, &rowIndex, _retval);
        }
      }
      break;
    default:
      result = NS_ERROR_NOT_IMPLEMENTED;
  }

  return result;
}

NS_IMETHODIMP    
nsEditorShell::GetTableRowCount(nsIDOMElement *tableElement, PRInt32 *_retval)
{
  if (!_retval)
    return NS_ERROR_NULL_POINTER;

  nsresult  result = NS_NOINTERFACE;
  switch (mEditorType)
  {
    case eHTMLTextEditorType:
      {
        nsCOMPtr<nsITableEditor> tableEditor = do_QueryInterface(mEditor);
        if (tableEditor)
        {
          // This returns both the number of rows and columns: return just rows
          PRInt32 cols;
          result = tableEditor->GetTableSize(tableElement, _retval, &cols);
        }
      }
      break;
    default:
      result = NS_ERROR_NOT_IMPLEMENTED;
  }

  return result;
}

NS_IMETHODIMP    
nsEditorShell::GetTableColumnCount(nsIDOMElement *tableElement, PRInt32 *_retval)
{
  if (!_retval)
    return NS_ERROR_NULL_POINTER;

  nsresult  result = NS_NOINTERFACE;
  switch (mEditorType)
  {
    case eHTMLTextEditorType:
      {
        nsCOMPtr<nsITableEditor> tableEditor = do_QueryInterface(mEditor);
        if (tableEditor)
        {
          // This returns both the number of rows and columns: return just columns
          PRInt32 rows;
          result = tableEditor->GetTableSize(tableElement, &rows, _retval);
        }
      }
      break;
    default:
      result = NS_ERROR_NOT_IMPLEMENTED;
  }

  return result;
}

NS_IMETHODIMP    
nsEditorShell::GetCellAt(nsIDOMElement *tableElement, PRInt32 rowIndex, PRInt32 colIndex, nsIDOMElement **_retval)
{
  if (!_retval)
    return NS_ERROR_NULL_POINTER;

  nsresult  result = NS_NOINTERFACE;
  switch (mEditorType)
  {
    case eHTMLTextEditorType:
      {
        nsCOMPtr<nsITableEditor> tableEditor = do_QueryInterface(mEditor);
        if (tableEditor)
        {
          result = tableEditor->GetCellAt(tableElement, rowIndex, colIndex, _retval);
          // Don't return NS_EDITOR_ELEMENT_NOT_FOUND (passes NS_SUCCEEDED macro)
          //  to JavaScript
          if(NS_SUCCEEDED(result)) return NS_OK;
        }
      }
      break;
    default:
      result = NS_ERROR_NOT_IMPLEMENTED;
  }

  return result;
}

// Note that the return param in the IDL must be the LAST out param here,
//   so order of params is different from nsITableEditor
NS_IMETHODIMP    
nsEditorShell::GetCellDataAt(nsIDOMElement *tableElement, PRInt32 rowIndex, PRInt32 colIndex,
                             PRInt32 *aStartRowIndex, PRInt32 *aStartColIndex, 
                             PRInt32 *aRowSpan, PRInt32 *aColSpan, 
                             PRInt32 *aActualRowSpan, PRInt32 *aActualColSpan, 
                             PRBool *aIsSelected, nsIDOMElement **_retval)
{
  if (!_retval || 
      !aStartRowIndex || !aStartColIndex || 
      !aRowSpan || !aColSpan || 
      !aActualRowSpan || !aActualColSpan ||
      !aIsSelected )
    return NS_ERROR_NULL_POINTER;

  nsresult  result = NS_NOINTERFACE;
  switch (mEditorType)
  {
    case eHTMLTextEditorType:
    {
      nsCOMPtr<nsITableEditor> tableEditor = do_QueryInterface(mEditor);
      if (tableEditor)
        result = tableEditor->GetCellDataAt(tableElement, rowIndex, colIndex, _retval,
                                            aStartRowIndex, aStartColIndex, 
                                            aRowSpan, aColSpan, 
                                            aActualRowSpan, aActualColSpan,
                                            aIsSelected);
      // Don't return NS_EDITOR_ELEMENT_NOT_FOUND (passes NS_SUCCEEDED macro)
      //  to JavaScript
      if(NS_SUCCEEDED(result)) return NS_OK;
    }
    break;
    default:
      result = NS_ERROR_NOT_IMPLEMENTED;
  }

  return result;
}

NS_IMETHODIMP
nsEditorShell::GetFirstRow(nsIDOMElement *aTableElement, nsIDOMNode **_retval)
{
  if (!_retval || !aTableElement)
    return NS_ERROR_NULL_POINTER;

  nsresult  result = NS_NOINTERFACE;
  switch (mEditorType)
  {
    case eHTMLTextEditorType:
      {
        nsCOMPtr<nsITableEditor> tableEditor = do_QueryInterface(mEditor);
        if (tableEditor)
          result = tableEditor->GetFirstRow(aTableElement, _retval);
      }
      break;
    default:
      result = NS_ERROR_NOT_IMPLEMENTED;
  }
  return result;
}

NS_IMETHODIMP
nsEditorShell::GetNextRow(nsIDOMNode *aCurrentRow, nsIDOMNode **_retval)
{
  if (!_retval || !*_retval || !aCurrentRow)
    return NS_ERROR_NULL_POINTER;

  nsresult  result = NS_NOINTERFACE;
  
  switch (mEditorType)
  {
    case eHTMLTextEditorType:
      {
        nsCOMPtr<nsITableEditor> tableEditor = do_QueryInterface(mEditor);
        if (tableEditor)
          result = tableEditor->GetNextRow(aCurrentRow, _retval);
      }
      break;
    default:
      result = NS_ERROR_NOT_IMPLEMENTED;
  }
  return result;
}

NS_IMETHODIMP
nsEditorShell::GetSelectedOrParentTableElement(PRUnichar **aTagName, PRInt32 *aSelectedCount, nsIDOMElement **_retval)
{
  if (!_retval || !aTagName || !aSelectedCount)
    return NS_ERROR_NULL_POINTER;

  nsresult  result = NS_NOINTERFACE;
  switch (mEditorType)
  {
    case eHTMLTextEditorType:
      {
        nsCOMPtr<nsITableEditor> tableEditor = do_QueryInterface(mEditor);
        nsAutoString TagName(*aTagName);
        if (tableEditor)
          result = tableEditor->GetSelectedOrParentTableElement(TagName,
                                                                aSelectedCount,
                                                                _retval);
          *aTagName = ToNewUnicode(TagName);
      }
      break;
    default:
      result = NS_ERROR_NOT_IMPLEMENTED;
  }
  return result;
}

NS_IMETHODIMP
nsEditorShell::GetSelectedCellsType(nsIDOMElement *aElement, PRUint32 *_retval)
{
  if (!_retval)
    return NS_ERROR_NULL_POINTER;

  nsresult  result = NS_NOINTERFACE;
  switch (mEditorType)
  {
    case eHTMLTextEditorType:
      {
        nsCOMPtr<nsITableEditor> tableEditor = do_QueryInterface(mEditor);
        if (tableEditor)
          result = tableEditor->GetSelectedCellsType(aElement, _retval);
      }
      break;
    default:
      result = NS_ERROR_NOT_IMPLEMENTED;
  }
  return result;
}

/* end of table editing */

NS_IMETHODIMP
nsEditorShell::GetLinkedObjects(nsISupportsArray **aObjectArray)
{
  if (!aObjectArray)
    return NS_ERROR_NULL_POINTER;

  if (mEditorType == eHTMLTextEditorType)
    return mEditor->GetLinkedObjects(aObjectArray);

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsEditorShell::GetEmbeddedObjects(nsISupportsArray **aObjectArray)
{
  if (!aObjectArray)
    return NS_ERROR_NULL_POINTER;

  if (mEditorType == eHTMLTextEditorType)
  {
    nsCOMPtr<nsIEditorMailSupport> mailEditor = do_QueryInterface(mEditor);
    if (mailEditor)
      return mailEditor->GetEmbeddedObjects(aObjectArray);
  }

  return NS_NOINTERFACE;
}

NS_IMETHODIMP    
nsEditorShell::InitSpellChecker()
{
  nsresult  result = NS_NOINTERFACE;

   // We can spell check with any editor type
  if (mEditor)
  {
    nsCOMPtr<nsITextServicesDocument>tsDoc;

    result = nsComponentManager::CreateInstance(
                                 kCTextServicesDocumentCID,
                                 nsnull,
                                 NS_GET_IID(nsITextServicesDocument),
                                 (void **)getter_AddRefs(tsDoc));

    if (NS_FAILED(result))
      return result;

    if (!tsDoc)
      return NS_ERROR_NULL_POINTER;

    // Pass the editor to the text services document
    nsCOMPtr<nsIEditor>  editor = do_QueryInterface(mEditor);
    if (!editor)
      return NS_NOINTERFACE;

    result = tsDoc->InitWithEditor(editor);

    if (NS_FAILED(result))
      return result;

    result = nsComponentManager::CreateInstance(NS_SPELLCHECKER_CONTRACTID,
                                                nsnull,
                                                NS_GET_IID(nsISpellChecker),
                                                (void **)getter_AddRefs(mSpellChecker));

    if (NS_FAILED(result))
      return result;

    if (!mSpellChecker)
      return NS_ERROR_NULL_POINTER;

    result = mSpellChecker->SetDocument(tsDoc, PR_TRUE);

    if (NS_FAILED(result))
      return result;

    // Tell the spellchecker what dictionary to use:

    nsXPIDLString dictName;

    nsCOMPtr<nsIPref> prefs(do_GetService(kPrefServiceCID, &result));

    if (NS_SUCCEEDED(result) && prefs)
      result = prefs->CopyUnicharPref("spellchecker.dictionary",
                                      getter_Copies(dictName));

    if (NS_FAILED(result) || dictName.IsEmpty())
    {
      // Prefs didn't give us a dictionary name, so just get the current
      // locale and use that as the default dictionary name!

      nsCOMPtr<nsIXULChromeRegistry> packageRegistry =
        do_GetService(NS_CHROMEREGISTRY_CONTRACTID, &result);

      if (NS_SUCCEEDED(result) && packageRegistry) {
        nsCAutoString utf8DictName;
        result = packageRegistry->GetSelectedLocale(NS_LITERAL_CSTRING("navigator"), utf8DictName);
        dictName = NS_ConvertUTF8toUCS2(utf8DictName);
      }
    }

    if (NS_SUCCEEDED(result) && !dictName.IsEmpty())
      result = SetCurrentDictionary(dictName.get());

    // If an error was thrown while checking the dictionary pref, just
    // fail silently so that the spellchecker dialog is allowed to come
    // up. The user can manually reset the language to their choice on
    // the dialog if it is wrong.

    result = NS_OK;

    DeleteSuggestedWordList();
  }

  return result;
}

NS_IMETHODIMP    
nsEditorShell::GetNextMisspelledWord(PRUnichar **aNextMisspelledWord)
{
  nsresult  result = NS_NOINTERFACE;
  nsAutoString nextMisspelledWord;
  
   // We can spell check with any editor type
  if (mEditor && mSpellChecker)
  {
    DeleteSuggestedWordList();
    result = mSpellChecker->NextMisspelledWord(&nextMisspelledWord, &mSuggestedWordList);
  }
  *aNextMisspelledWord = ToNewUnicode(nextMisspelledWord);
  return result;
}

NS_IMETHODIMP    
nsEditorShell::GetSuggestedWord(PRUnichar **aSuggestedWord)
{
  nsresult  result = NS_NOINTERFACE;
  nsAutoString word;
   // We can spell check with any editor type
  if (mEditor)
  {
    if ( mSuggestedWordIndex < mSuggestedWordList.Count())
    {
      mSuggestedWordList.StringAt(mSuggestedWordIndex, word);
      mSuggestedWordIndex++;
    } else {
      // A blank string signals that there are no more strings
      word.SetLength(0);
    }
    result = NS_OK;
  }
  *aSuggestedWord = ToNewUnicode(word);
  return result;
}

NS_IMETHODIMP    
nsEditorShell::CheckCurrentWord(const PRUnichar *aSuggestedWord, PRBool *aIsMisspelled)
{
  nsresult  result = NS_NOINTERFACE;
  nsAutoString suggestedWord(aSuggestedWord);
   // We can spell check with any editor type
  if (mEditor && mSpellChecker)
  {
    DeleteSuggestedWordList();
    result = mSpellChecker->CheckWord(&suggestedWord, aIsMisspelled, &mSuggestedWordList);
  }
  return result;
}

NS_IMETHODIMP    
nsEditorShell::ReplaceWord(const PRUnichar *aMisspelledWord, const PRUnichar *aReplaceWord, PRBool allOccurrences)
{
  nsresult  result = NS_NOINTERFACE;
  nsAutoString misspelledWord(aMisspelledWord);
  nsAutoString replaceWord(aReplaceWord);
  if (mEditor && mSpellChecker)
  {
    result = mSpellChecker->Replace(&misspelledWord, &replaceWord, allOccurrences);
  }
  return result;
}

NS_IMETHODIMP    
nsEditorShell::IgnoreWordAllOccurrences(const PRUnichar *aWord)
{
  nsresult  result = NS_NOINTERFACE;
  nsAutoString word(aWord);
  if (mEditor && mSpellChecker)
  {
    result = mSpellChecker->IgnoreAll(&word);
  }
  return result;
}

NS_IMETHODIMP    
nsEditorShell::GetPersonalDictionary()
{
  nsresult  result = NS_NOINTERFACE;
   // We can spell check with any editor type
  if (mEditor && mSpellChecker)
  {
    mDictionaryList.Clear();
    mDictionaryIndex = 0;
    result = mSpellChecker->GetPersonalDictionary(&mDictionaryList);
  }
  return result;
}

NS_IMETHODIMP    
nsEditorShell::GetPersonalDictionaryWord(PRUnichar **aDictionaryWord)
{
  nsresult  result = NS_NOINTERFACE;
  nsAutoString word;
  if (mEditor)
  {
    if ( mDictionaryIndex < mDictionaryList.Count())
    {
      mDictionaryList.StringAt(mDictionaryIndex, word);
      mDictionaryIndex++;
    } else {
      // A blank string signals that there are no more strings
      word.SetLength(0);
    }
    result = NS_OK;
  }
  *aDictionaryWord = ToNewUnicode(word);
  return result;
}

NS_IMETHODIMP    
nsEditorShell::AddWordToDictionary(const PRUnichar *aWord)
{
  nsresult  result = NS_NOINTERFACE;
  nsAutoString word(aWord);
  if (mEditor && mSpellChecker)
  {
    result = mSpellChecker->AddWordToPersonalDictionary(&word);
  }
  return result;
}

NS_IMETHODIMP    
nsEditorShell::RemoveWordFromDictionary(const PRUnichar *aWord)
{
  nsresult  result = NS_NOINTERFACE;
  nsAutoString word(aWord);
  if (mEditor && mSpellChecker)
  {
    result = mSpellChecker->RemoveWordFromPersonalDictionary(&word);
  }
  return result;
}

NS_IMETHODIMP    
nsEditorShell::GetDictionaryList(PRUnichar ***aDictionaryList, PRUint32 *aCount)
{
  nsresult  result = NS_ERROR_NOT_IMPLEMENTED;

  if (!aDictionaryList || !aCount)
    return NS_ERROR_NULL_POINTER;

  *aDictionaryList = 0;
  *aCount          = 0;

  if (mEditor && mSpellChecker)
  {
    nsStringArray dictList;

    result = mSpellChecker->GetDictionaryList(&dictList);

    if (NS_FAILED(result))
      return result;

    PRUnichar **tmpPtr = 0;

    if (dictList.Count() < 1)
    {
      // If there are no dictionaries, return an array containing
      // one element and a count of one.

      tmpPtr = (PRUnichar **)nsMemory::Alloc(sizeof(PRUnichar *));

      if (!tmpPtr)
        return NS_ERROR_OUT_OF_MEMORY;

      *tmpPtr          = 0;
      *aDictionaryList = tmpPtr;
      *aCount          = 0;

      return NS_OK;
    }

    tmpPtr = (PRUnichar **)nsMemory::Alloc(sizeof(PRUnichar *) * dictList.Count());

    if (!tmpPtr)
      return NS_ERROR_OUT_OF_MEMORY;

    *aDictionaryList = tmpPtr;
    *aCount          = dictList.Count();

    nsAutoString dictStr;

    PRUint32 i;

    for (i = 0; i < *aCount; i++)
    {
      dictList.StringAt(i, dictStr);
      tmpPtr[i] = ToNewUnicode(dictStr);
    }
  }

  return result;
}

NS_IMETHODIMP    
nsEditorShell::GetCurrentDictionary(PRUnichar **aDictionary)
{
  nsresult  result = NS_ERROR_NOT_INITIALIZED;

  if (!aDictionary)
    return NS_ERROR_NULL_POINTER;

  *aDictionary = 0;

  if (mEditor && mSpellChecker)
  {
    nsAutoString dictStr;
    result = mSpellChecker->GetCurrentDictionary(&dictStr);

    if (NS_FAILED(result))
      return result;

    *aDictionary = ToNewUnicode(dictStr);
  }

  return result;
}

NS_IMETHODIMP    
nsEditorShell::SetCurrentDictionary(const PRUnichar *aDictionary)
{
  nsresult  result = NS_ERROR_NOT_INITIALIZED;

  if (!aDictionary)
    return NS_ERROR_NULL_POINTER;

  if (mEditor && mSpellChecker)
  {
    nsAutoString dictStr(aDictionary);
    result = mSpellChecker->SetCurrentDictionary(&dictStr);
  }

  return result;
}

NS_IMETHODIMP    
nsEditorShell::UninitSpellChecker()
{
  nsresult  result = NS_NOINTERFACE;
   // We can spell check with any editor type
  if (mEditor)
  {
    // Save the last used dictionary to the user's preferences.
    nsCOMPtr<nsIPref> prefs(do_GetService(kPrefServiceCID, &result));

    if (NS_SUCCEEDED(result) && prefs)
    {
      PRUnichar *dictName = nsnull;

      result = GetCurrentDictionary(&dictName);

      if (NS_SUCCEEDED(result) && dictName && *dictName)
        result = prefs->SetUnicharPref("spellchecker.dictionary", dictName);

      if (dictName)
        nsMemory::Free(dictName);
    }

    // Cleanup - kill the spell checker
    DeleteSuggestedWordList();
    mDictionaryList.Clear();
    mDictionaryIndex = 0;
    mSpellChecker = 0;
    result = NS_OK;
  }
  return result;
}

nsresult    
nsEditorShell::DeleteSuggestedWordList()
{
  mSuggestedWordList.Clear();
  mSuggestedWordIndex = 0;
  return NS_OK;
}

#ifdef XP_MAC
#pragma mark -
#endif

/* void onStartURIOpen (in nsIURI aURI, out boolean aAbortOpen); */
NS_IMETHODIMP nsEditorShell::OnStartURIOpen(nsIURI *aURI, PRBool *aAbortOpen)
{
  return NS_OK;
}

/* void doContent (in string aContentType, in boolean aIsContentPreferred, in nsIChannel aOpenedChannel, out nsIStreamListener aContentHandler, out boolean aAbortProcess); */
NS_IMETHODIMP nsEditorShell::DoContent(const char *aContentType, PRBool aIsContentPreferred, nsIRequest* request, nsIStreamListener **aContentHandler, PRBool *aAbortProcess)
{
  NS_ENSURE_ARG_POINTER(aContentHandler);
  NS_ENSURE_ARG_POINTER(aAbortProcess);
  *aContentHandler = nsnull;
  *aAbortProcess = PR_FALSE;
  return NS_OK;
}

/* boolean isPreferred (in string aContentType, out string aDesiredContentType); */
NS_IMETHODIMP nsEditorShell::IsPreferred(const char *aContentType, char **aDesiredContentType, PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(aDesiredContentType);
  NS_ENSURE_ARG_POINTER(_retval);
  *aDesiredContentType = nsnull;
  *_retval = PR_FALSE;
  return NS_OK;
}

/* boolean canHandleContent (in string aContentType, in boolean aIsContentPreferred, out string aDesiredContentType); */
NS_IMETHODIMP nsEditorShell::CanHandleContent(const char *aContentType, PRBool aIsContentPreferred, char **aDesiredContentType, PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(aDesiredContentType);
  NS_ENSURE_ARG_POINTER(_retval);
  *aDesiredContentType = nsnull;
  *_retval = PR_FALSE;
  return NS_OK;
}

/* attribute nsISupports loadCookie; */
NS_IMETHODIMP nsEditorShell::GetLoadCookie(nsISupports * *aLoadCookie)
{
  NS_ENSURE_ARG_POINTER(aLoadCookie);
  *aLoadCookie = nsnull;
  return NS_OK;
}
NS_IMETHODIMP nsEditorShell::SetLoadCookie(nsISupports * aLoadCookie)
{
  return NS_OK;
}

/* attribute nsIURIContentListener parentContentListener; */
NS_IMETHODIMP nsEditorShell::GetParentContentListener(nsIURIContentListener * *aParentContentListener)
{
  NS_ENSURE_ARG_POINTER(aParentContentListener);
  *aParentContentListener = nsnull;
  return NS_OK;
}
NS_IMETHODIMP nsEditorShell::SetParentContentListener(nsIURIContentListener * aParentContentListener)
{
  return NS_OK;
}

#ifdef XP_MAC
#pragma mark -
#endif


NS_IMETHODIMP
nsEditorShell::BeginBatchChanges()
{
  nsresult  err = NS_NOINTERFACE;
  
  switch (mEditorType)
  {
    case ePlainTextEditorType:
    case eHTMLTextEditorType:
      {
        nsCOMPtr<nsIEditor>  editor = do_QueryInterface(mEditor);
        if (editor)
          err = editor->BeginTransaction();
      }
      break;

    default:
      err = NS_ERROR_NOT_IMPLEMENTED;
  }

  return err;
}

NS_IMETHODIMP
nsEditorShell::EndBatchChanges()
{
  nsresult  err = NS_NOINTERFACE;
  
  switch (mEditorType)
  {
    case ePlainTextEditorType:
    case eHTMLTextEditorType:
      {
        nsCOMPtr<nsIEditor>  editor = do_QueryInterface(mEditor);
        if (editor)
          err = editor->EndTransaction();
      }
      break;

    default:
      err = NS_ERROR_NOT_IMPLEMENTED;
  }

  return err;
}

NS_IMETHODIMP
nsEditorShell::RunUnitTests()
{
  PRInt32  numTests = 0;
  PRInt32  numTestsFailed = 0;
  
  nsresult err = NS_OK;
  nsCOMPtr<nsIEditor>  editor = do_QueryInterface(mEditor);
  if (editor)
    err = editor->DebugUnitTests(&numTests, &numTestsFailed);

#ifdef APP_DEBUG
  printf("\nRan %ld tests, of which %ld failed\n", (long)numTests, (long)numTestsFailed);
#endif

  return NS_OK;
}

NS_IMETHODIMP
nsEditorShell::StartLogging(nsIFile *logFile)
{
  nsresult  err = NS_OK;

  switch (mEditorType)
  {
    case ePlainTextEditorType:
    case eHTMLTextEditorType:
      {
        nsCOMPtr<nsIEditorLogging>  logger = do_QueryInterface(mEditor);
        if (logger)
          err = logger->StartLogging(logFile);
      }
      break;

    default:
      err = NS_ERROR_NOT_IMPLEMENTED;
  }

  return err;
}

NS_IMETHODIMP
nsEditorShell::StopLogging()
{
  nsresult  err = NS_OK;

  switch (mEditorType)
  {
    case ePlainTextEditorType:
    case eHTMLTextEditorType:
      {
        nsCOMPtr<nsIEditorLogging>  logger = do_QueryInterface(mEditor);
        if (logger)
          err = logger->StopLogging();
      }
      break;

    default:
      err = NS_ERROR_NOT_IMPLEMENTED;
  }

  return err;
}

#ifdef XP_MAC
#pragma mark -
#endif

//----------------------------------------
// nsIWebProgessListener implementation
//----------------------------------------
NS_IMETHODIMP
nsEditorShell::OnProgressChange(nsIWebProgress *aProgress,
                                nsIRequest *aRequest,
                                PRInt32 aCurSelfProgress,
                                PRInt32 aMaxSelfProgress,
                                PRInt32 aCurTotalProgress,
                                PRInt32 aMaxTotalProgress)
{
  if (mParserObserver)
  {
    PRBool cancelEdit;
    mParserObserver->GetBadTagFound(&cancelEdit);
    if (cancelEdit)
    {
      mParserObserver->End();
      NS_RELEASE(mParserObserver);

        mCloseWindowWhenLoaded = PR_TRUE;
        mCantEditReason = eCantEditFramesets;
      }
    }
  return NS_OK;
}

NS_IMETHODIMP
nsEditorShell::OnStateChange(nsIWebProgress *aProgress,
                             nsIRequest *aRequest,
                             PRUint32 aStateFlags,
                             nsresult aStatus)
{
  NS_ENSURE_ARG_POINTER(aProgress);

  //
  // A Request has started...
  //
  if (aStateFlags & nsIWebProgressListener::STATE_START)
  {
    // Page level notification...
    if (aStateFlags & nsIWebProgressListener::STATE_IS_NETWORK)
    {
      nsCOMPtr<nsIChannel> channel(do_QueryInterface(aRequest));
      StartPageLoad(channel);
    }
    // Document level notification...
    if (aStateFlags & nsIWebProgressListener::STATE_IS_DOCUMENT)
    {
      nsCOMPtr<nsIDOMWindow> domWindow;

      // Get the DOMWindow where the state change occurred...
      aProgress->GetDOMWindow(getter_AddRefs(domWindow));
      if (domWindow)
      {
        (void) StartDocumentLoad(domWindow);
      }
    }
  }
  //
  // A network or document Request as finished...
  //
  else if ((aStateFlags & nsIWebProgressListener::STATE_STOP) &&
          ((aStateFlags & nsIWebProgressListener::STATE_IS_DOCUMENT) ||
           (aStateFlags & nsIWebProgressListener::STATE_IS_NETWORK)))
  {
    nsCOMPtr<nsIChannel> channel(do_QueryInterface(aRequest));
    nsCOMPtr<nsIDOMWindow> domWindow;

    // Get the DOMWindow where the state change occurred...
    aProgress->GetDOMWindow(getter_AddRefs(domWindow));

    if (domWindow)
    {
      // Document level notification...
      if (aStateFlags & nsIWebProgressListener::STATE_IS_DOCUMENT)
      {
        (void) EndDocumentLoad(domWindow, channel, aStatus);
      }
      // Page level notification...
      if (aStateFlags & nsIWebProgressListener::STATE_IS_NETWORK)
      {
        (void) EndPageLoad(domWindow, channel, aStatus);
      }
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsEditorShell::OnLocationChange(nsIWebProgress *aProgress,
                                nsIRequest *aRequest,
                                nsIURI *aURI)
{
  nsCOMPtr<nsIDocShell> docShell;
  nsresult rv = GetDocShellFromContentWindow(getter_AddRefs(docShell));
  if (!docShell) return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDocument> doc;
  rv = GetDocument( docShell, getter_AddRefs(doc) );
  if (NS_FAILED(rv)) return rv;
  if (!doc) return NS_ERROR_FAILURE;

  // Set the new document URL
  rv = doc->SetDocumentURL(aURI);
  if (NS_FAILED(rv)) return rv;

  nsCOMPtr<nsIDOMDocument> domDoc = do_QueryInterface(doc);
  if (!doc) return NS_ERROR_FAILURE;

  // Look for an HTML <base> tag
  nsCOMPtr<nsIDOMNodeList> nodeList;
  rv = domDoc->GetElementsByTagName(NS_LITERAL_STRING("base"), getter_AddRefs(nodeList));
  if (NS_FAILED(rv)) return rv;
  if (!doc) return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDOMNode> baseNode;
  if (nodeList)
  {
    PRUint32 count;
    nodeList->GetLength(&count);
    if (count >= 1)
    {
      rv = nodeList->Item(0, getter_AddRefs(baseNode));
      if (NS_FAILED(rv)) return rv;
    }
  }
  // If no base tag, then set baseURL to the document's URL
  // This is very important, else relative URLs for links and images are busted!
  if (!baseNode)
    rv = doc->SetBaseURL(aURI);

  return rv;
}

NS_IMETHODIMP 
nsEditorShell::OnStatusChange(nsIWebProgress* aWebProgress,
                              nsIRequest* aRequest,
                              nsresult aStatus,
                              const PRUnichar* aMessage)
{
  NS_NOTREACHED("notification excluded in AddProgressListener(...)");
  return NS_OK;
}

NS_IMETHODIMP
nsEditorShell::OnSecurityChange(nsIWebProgress *aWebProgress,
                                nsIRequest *aRequest,
                                PRUint32 state)
{
  NS_NOTREACHED("notification excluded in AddProgressListener(...)");
  return NS_OK;
}

nsresult nsEditorShell::StartPageLoad(nsIChannel *aChannel)
{
  nsCAutoString contentType;
  aChannel->GetContentType(contentType);
  
  // save the original MIME type; we'll use it later
  if (!contentType.IsEmpty())
    mContentMIMEType.Assign(contentType);
  
  if (contentType.Equals(NS_LITERAL_CSTRING("text/html")))
  {
    // fine, do nothing
    mContentTypeKnown = PR_TRUE;
  }
  else
  {
    PRBool canBeText;
    IsSupportedTextType(contentType.get(), &canBeText);
    if (canBeText)
    {
      // set the mime type to text/plain so that it renders as text
      aChannel->SetContentType(NS_LITERAL_CSTRING("text/plain"));
      mContentTypeKnown = PR_TRUE;
    }
    else
    {
      // we don't know what this is yet. It might be HTML loaded from 
      // a directory URL (http://www.foo.com/). We'll know the real
      // MIME type later.
      mContentTypeKnown = PR_FALSE;
    }
  }

  // Start the throbber
  // TODO: We should also start/stop it for saving and publishing?
  SetChromeAttribute( mDocShell, "Editor:Throbber", NS_LITERAL_STRING("busy"), NS_LITERAL_STRING("true") );

  // set up a parser observer
  if (!mParserObserver)
  {
    mParserObserver = new nsEditorParserObserver();
    if (!mParserObserver) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    NS_ADDREF(mParserObserver);
    mParserObserver->Start(gWatchTags);
  }

  return NS_OK;
}

// Called when the entire document, including any sub-documents (ie. frames)
// have been loaded...
nsresult nsEditorShell::EndPageLoad(nsIDOMWindow *aDOMWindow,
                                    nsIChannel *aChannel,
                                    nsresult aStatus)
{
  // Make sure the ParserObserver is active through any frameset loads?
  if (mParserObserver)
  {
    // if we didn't already get the bad tag flag from the parser observer (i.e.
    // we never got an OnProgress), then look again here.
    PRBool cancelEdit;
    mParserObserver->GetBadTagFound(&cancelEdit);
    if (cancelEdit && !mCloseWindowWhenLoaded)
    {
        mCloseWindowWhenLoaded = PR_TRUE;
        mCantEditReason = eCantEditFramesets;
    }
    
    mParserObserver->End();
    NS_RELEASE(mParserObserver);
  }

  SetChromeAttribute( mDocShell, "Editor:Throbber", NS_LITERAL_STRING("busy"), NS_LITERAL_STRING("false") );

  if (aStatus == NS_ERROR_FILE_NOT_FOUND)
  {
    // For this case, network code popped up an alert dialog,
    //  so we don't need to
    mCloseWindowWhenLoaded = PR_TRUE;
    mCantEditReason = eCantEditFileNotFound;
  }
  else
  {
    // Is this a MIME type we can handle?
    if (aChannel)
    {
      // if we didn't get the content-type at the start of the load, get it now
      if (!mContentTypeKnown)
      {
        nsCAutoString  contentType;
        aChannel->GetContentType(contentType);

        if (!contentType.IsEmpty())
          mContentMIMEType.Assign(contentType);
      }
    }    

    PRBool canBeText;
    IsSupportedTextType(mContentMIMEType.get(), &canBeText);
    if (!mContentMIMEType.Equals("text/html") && !canBeText)
    {
        mCloseWindowWhenLoaded = PR_TRUE;
        mCantEditReason = eCantEditMimeType;
    }

    // Display an Alert dialog if the page cannot be edited...
    if (mCloseWindowWhenLoaded)
    {
      nsAutoString alertLabel, alertMessage;
      GetBundleString(NS_LITERAL_STRING("Alert"), alertLabel);
    
      nsAutoString  stringID;
      switch (mCantEditReason)
      {
        case eCantEditFramesets:
          stringID.Assign(NS_LITERAL_STRING("CantEditFramesetMsg"));
          break;        
        case eCantEditMimeType:
          stringID.Assign(NS_LITERAL_STRING("CantEditMimeTypeMsg"));
          break;
        case eCantEditOther:
        default:
          stringID.Assign(NS_LITERAL_STRING("CantEditDocumentMsg"));
          break;
      }
      GetBundleString(stringID, alertMessage);
      Alert(alertLabel, alertMessage);
    }
  }

  // If we had any errors, close the window
  if (mCloseWindowWhenLoaded)
  {
    //TODO: Would it be possible to simply change channel URL to "about:blank"
    //      so we leave window up with empty page instead of closing it?
    nsCOMPtr<nsIBaseWindow> baseWindow;
    GetTreeOwner(mDocShell, getter_AddRefs(baseWindow));
    NS_ENSURE_TRUE(baseWindow, NS_ERROR_ABORT);
    baseWindow->Destroy();

    return NS_ERROR_ABORT;
  }

  if (!mMailCompose) {
    nsAutoString doneText;
    GetBundleString(NS_LITERAL_STRING("LoadingDone"), doneText);
    SetChromeAttribute(mDocShell, "statusText", NS_LITERAL_STRING("label"), doneText);
  }

  //
  // By this time, we know that the page did not contain any frames
  // (since mCloseWindowWhenLoaded was PR_FALSE)...  So, make an
  // editor (assuming that the load succeeded)...
  //
  // for pages with charsets, this gets called the first time with a 
  // non-zero status value. Don't prepare the editor that time.
  // aStatus will be NS_BINDING_ABORTED then.
  //
  if (NS_SUCCEEDED(aStatus))
  {
    nsCOMPtr<nsIURI>  url;
    aChannel->GetURI(getter_AddRefs(url));

    nsresult rv = PrepareDocumentForEditing(aDOMWindow, url);

    // if we return this error, it will never get reported, so we should
    // report it here.
    NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to setup document for editing");
  }
  
  return NS_OK;
}

// Called when each new document, or sub-document (ie. frame) starts to
// load.
nsresult nsEditorShell::StartDocumentLoad(nsIDOMWindow *aDOMWindow)
{
  // Disable JavaScript in this document:
  nsCOMPtr<nsIScriptGlobalObject> sgo (do_QueryInterface(aDOMWindow));

  if (sgo)
  {
    nsCOMPtr<nsIScriptContext> scriptContext;
    sgo->GetContext(getter_AddRefs(scriptContext));
    if (scriptContext)
      scriptContext->SetScriptsEnabled(PR_FALSE, PR_TRUE);
  }
  
  return NS_OK;
}

// Called when each document, or sub-document (ie. frame) has finished
// loading...
nsresult nsEditorShell::EndDocumentLoad(nsIDOMWindow *aDOMWindow,
                                        nsIChannel* aChannel,
                                        nsresult aStatus)
{
  // for pages with charsets, this gets called the first time with a 
  // non-zero status value. Don't prepare the editor that time.
  // aStatus will be NS_BINDING_ABORTED then.
  if (aStatus == NS_BINDING_ABORTED)
    return NS_OK;

  // note that we continue with other non-success status codes.
	
  // Disable meta-refresh
  nsCOMPtr<nsIRefreshURI> refreshURI = do_QueryInterface(mContentAreaDocShell);
  if (refreshURI)
    refreshURI->CancelRefreshURITimers();
  
  return NS_OK;
}


#ifdef XP_MAC
#pragma mark -
#endif

nsresult
nsEditorShell::CheckPrefAndNormalizeTable()
{
  nsresult  res = NS_NOINTERFACE;
  nsCOMPtr<nsIHTMLEditor>  htmlEditor = do_QueryInterface(mEditor);

  if (htmlEditor)
  {
    nsCOMPtr<nsIPref> prefs(do_GetService(kPrefServiceCID, &res));
    if (NS_FAILED(res)) return NS_OK;

    PRBool normalizeTable = PR_FALSE;
    if (NS_SUCCEEDED(prefs->GetBoolPref("editor.table.maintain_structure", &normalizeTable)) && normalizeTable)
      return NormalizeTable(nsnull);

    return NS_OK;
  }
  return res;
}


nsresult
nsEditorShell::DoControllerCommand(const char *aCommand)
{
  // Get the list of controllers...
  nsCOMPtr<nsIControllers> controllers;      
  if(!mContentWindow)
    return NS_ERROR_NOT_INITIALIZED;

  nsCOMPtr<nsIDOMWindowInternal> cwP = do_QueryReferent(mContentWindow);
  if (!cwP) return NS_ERROR_NOT_INITIALIZED;
  nsresult rv = cwP->GetControllers(getter_AddRefs(controllers));      
  if (NS_FAILED(rv)) return rv;
  if (!controllers) return NS_ERROR_NULL_POINTER;

  //... then find the specific controller supporting desired command
  nsCOMPtr<nsIController> controller; 

  rv = controllers->GetControllerForCommand(aCommand, getter_AddRefs(controller));
  
  if (NS_SUCCEEDED(rv))
  {
    if (!controller) return NS_ERROR_FAILURE;

    nsCOMPtr<nsIEditorController> composerController = do_QueryInterface(controller);
    // Execute the command
    rv = composerController->DoCommand(aCommand);
  }
  return rv;
}

