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
 */

#include <stdio.h>

#include "nsEditorShell.h"
#include "nsIPlaintextEditor.h"
#include "nsIWebShell.h"
#include "nsIBaseWindow.h"
#include "nsIContentViewerFile.h"
#include "pratom.h"
#include "prprf.h"
#include "nsIComponentManager.h"

#include "nsIFocusController.h"
#include "nsIScriptContext.h"
#include "nsIScriptGlobalObject.h"
#include "nsIScriptGlobalObjectOwner.h"
#include "nsIDOMDocument.h"
#include "nsIMarkupDocumentViewer.h"
#include "nsIDOMHTMLDocument.h"
#include "nsIDiskDocument.h"
#include "nsIDocument.h"
#include "nsIDOMWindowInternal.h"
#include "nsPIDOMWindow.h"
#include "nsIDOMNode.h"
#include "nsIDOMNodeList.h"
#include "nsICSSLoader.h"
#include "nsICSSStyleSheet.h"
#include "nsIStyleSheet.h"
#include "nsIStyleSet.h"
#include "nsIContent.h"
#include "nsIHTMLContentContainer.h"
#include "nsIURI.h"
#include "nsCURILoader.h"
#include "nsNetUtil.h"
#include "nsIFileChannel.h"
#include "nsILocalFile.h"

#include "nsIScriptGlobalObject.h"
#include "nsIWebNavigation.h"
#include "nsCOMPtr.h"

#include "nsIServiceManager.h"
#include "nsIURL.h"
#include "nsIWidget.h"
#include "nsIWindowMediator.h"
#include "plevent.h"
#include "nsXPIDLString.h"

#include "nsIAppShell.h"
#include "nsIAppShellService.h"
#include "nsAppShellCIDs.h"

#include "nsIDocumentViewer.h"
#include "nsIDOMHTMLImageElement.h"
#include "nsIPresShell.h"
#include "nsIPresContext.h"
#include "nsISelection.h"
#include "nsISelectionPrivate.h"
#include "nsISelectionController.h"

#include "nsIFilePicker.h"
#include "nsIFindComponent.h"
#include "nsIPrompt.h"
#include "nsIDialogParamBlock.h"
#include "nsIPromptService.h"
#include "nsPIPromptService.h"

#include "nsIEditorController.h"
//#include "nsEditorController.h"
#include "nsIControllers.h"
#include "nsIDocShell.h"
#include "nsIDocShellTreeItem.h"
#include "nsIDocShellTreeOwner.h"
#include "nsIDocShellTreeNode.h"
#include "nsITransactionManager.h"
#include "nsIDocumentEncoder.h"

#include "nsIRefreshURI.h"
#include "nsIPref.h"

#include "nsILookAndFeel.h"

#include "nsIChromeRegistry.h"
#include "nsCExternalHandlerService.h"
#include "nsIMIMEService.h"

///////////////////////////////////////
// Editor Includes
///////////////////////////////////////
#include "nsIDOMEventCapturer.h"
#include "nsString.h"
#include "nsIDOMText.h"
#include "nsIDOMElement.h"
#include "nsIDOMWindowCollection.h"
#include "nsIWebProgress.h"

#include "nsIEditor.h"
#include "nsIHTMLEditor.h"
#include "nsIEditorStyleSheets.h"
#include "nsIEditorMailSupport.h"
#include "nsITableEditor.h"
#include "nsIEditorLogging.h"

#include "nsEditorCID.h"

#include "nsIComponentManager.h"
#include "nsTextServicesCID.h"
#include "nsITextServicesDocument.h"
#include "nsISpellChecker.h"
#include "nsInterfaceState.h"

#include "nsEditorShellMouseListener.h"

///////////////////////////////////////

// Drag & Drop, Clipboard
#include "nsWidgetsCID.h"
#include "nsIClipboard.h"
#include "nsITransferable.h"
#include "nsISupportsArray.h"

/* Define Class IDs */
static NS_DEFINE_CID(kHTMLEditorCID,            NS_HTMLEDITOR_CID);
static NS_DEFINE_CID(kCTextServicesDocumentCID, NS_TEXTSERVICESDOCUMENT_CID);
static NS_DEFINE_CID(kCStringBundleServiceCID,  NS_STRINGBUNDLESERVICE_CID);
static NS_DEFINE_CID(kPrefServiceCID,           NS_PREF_CID);
static NS_DEFINE_CID(kChromeRegistryCID,        NS_CHROMEREGISTRY_CID);

#define APP_DEBUG 0 

#define EDITOR_BUNDLE_URL "chrome://editor/locale/editor.properties"
#define EDITOR_DEFAULT_DIR_PREF "editor.default.dir"

enum {
  eEditorController,
  eComposerController
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
,  mDisplayMode(eDisplayModeNormal)
,  mHTMLSourceMode(PR_FALSE)
,  mWebShellWindow(nsnull)
,  mContentWindow(nsnull)
,  mParserObserver(nsnull)
,  mStateMaintainer(nsnull)
,  mEditorController(nsnull)
,  mDocShell(nsnull)
,  mContentAreaDocShell(nsnull)
,  mInitted(PR_FALSE)
,  mCloseWindowWhenLoaded(PR_FALSE)
,  mCantEditReason(eCantEditNoReason)
,  mEditorType(eUninitializedEditorType)
,  mContentMIMEType("text/html")
,  mContentTypeKnown(PR_FALSE)
,  mWrapColumn(0)
,  mSuggestedWordIndex(0)
,  mDictionaryIndex(0)
{
  //TODO:Save last-used display mode in prefs so new window inherits?
  NS_INIT_REFCNT();
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
nsEditorShell::Init()
{
  NS_ASSERTION(!mInitted, "Double init of nsEditorShell detected");
  if (mInitted)
    return NS_OK;
  
  nsAutoString    editorType; editorType.AssignWithConversion("html");      // default to creating HTML editor
  mEditorTypeString = editorType;
  mEditorTypeString.ToLowerCase();

  // Get pointer to our string bundle
  nsresult res;
  nsCOMPtr<nsIStringBundleService> stringBundleService = do_GetService(kCStringBundleServiceCID, &res);
  if (!stringBundleService) { 
    NS_WARNING("ERROR: Failed to get StringBundle Service instance.\n");
    return res;
  }
  res = stringBundleService->CreateBundle(EDITOR_BUNDLE_URL, getter_AddRefs(mStringBundle));

  // XXX: why are we returning NS_OK here rather than res?
  // is it ok to fail to get a string bundle?  if so, it should be documented.
  mInitted = PR_TRUE;
  
  return NS_OK;
}

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

  // Remove our document mouse event listener
  if (mMouseListenerP)
  {
    nsCOMPtr<nsIDOMEventReceiver> erP;
    rv = GetDocumentEventReceiver(getter_AddRefs(erP));
    if (NS_SUCCEEDED(rv))
    {
      if (erP)
      {
        erP->RemoveEventListenerByIID(mMouseListenerP, NS_GET_IID(nsIDOMMouseListener));
        mMouseListenerP = nsnull;
      }
      else rv = NS_ERROR_NULL_POINTER;
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

  // Unload existing stylesheets
  nsCOMPtr<nsIEditorStyleSheets> styleSheets = do_QueryInterface(mEditor);
  if (styleSheets)
  {
    if (mBaseStyleSheet)
    {
      styleSheets->RemoveOverrideStyleSheet(mBaseStyleSheet);
      mBaseStyleSheet = 0;
    }
    if (mEditModeStyleSheet)
    {
      styleSheets->RemoveOverrideStyleSheet(mEditModeStyleSheet);
      mEditModeStyleSheet = 0;
    }
    if (mAllTagsModeStyleSheet)
    {
      styleSheets->RemoveOverrideStyleSheet(mAllTagsModeStyleSheet);
      mAllTagsModeStyleSheet = 0;
    }
    if (mParagraphMarksStyleSheet)
    {
      styleSheets->RemoveOverrideStyleSheet(mParagraphMarksStyleSheet);
      mParagraphMarksStyleSheet = 0;  
    }
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

  // Remove our document mouse event listener
  if (mMouseListenerP)
  {
    nsCOMPtr<nsIDOMEventReceiver> erP;
    rv = GetDocumentEventReceiver(getter_AddRefs(erP));
    if (NS_SUCCEEDED(rv) && erP)
    {
      erP->RemoveEventListenerByIID(mMouseListenerP, NS_GET_IID(nsIDOMMouseListener));
      mMouseListenerP = nsnull;
    }
  }

  // clear this editor out of the controller
  if (mEditorController)
  {
    mEditorController->SetCommandRefCon(nsnull);
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

PRBool
nsEditorShell::IsSupportedTextType(const char* aMIMEType)
{
  PRInt32   i = 0;
  
  while (gSupportedTextTypes[i])
  {
    if (nsCRT::strcmp(gSupportedTextTypes[i], aMIMEType) == 0)
      return PR_TRUE;
  
    i ++;
  }
  
  return PR_FALSE;
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
    presContext->SetImageAnimationMode(eImageAnimation_None);

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
  if (mEditorController)
  {
    nsCOMPtr<nsISupports> editorAsISupports = do_QueryInterface(editor);
    mEditorController->SetCommandRefCon(editorAsISupports);
  }

  if (mEditorType == eHTMLTextEditorType)
  {
    // get a mouse listener for double click on tags
    // We can't use nsEditor listener because core editor shouldn't call UI commands
    rv = NS_NewEditorShellMouseListener(getter_AddRefs(mMouseListenerP), this);
    if (NS_FAILED(rv))
    {
      mMouseListenerP = nsnull;
      return rv;
    }

    // Add mouse listener to document
    nsCOMPtr<nsIDOMEventReceiver> erP;
    rv = GetDocumentEventReceiver(getter_AddRefs(erP));
    if (NS_FAILED(rv))
    {
      mMouseListenerP = nsnull;
      return rv;
    }

    rv = erP->AddEventListenerByIID(mMouseListenerP, NS_GET_IID(nsIDOMMouseListener));
    if (NS_FAILED(rv)) return rv;
  }

  // now all the listeners are set up, we can call PostCreate
  rv = editor->PostCreate();
  if (NS_FAILED(rv)) return rv;
  
  // get the URL of the page we are editing
  if (aUrl)
  {
   
    // if this is a file URL of a file that exists locally, we'll stash the nsIFile
    // in the disk document, so that later saves save back to the same file.
    nsCOMPtr<nsIFileURL> pageFileURL(do_QueryInterface(aUrl));
    PRBool isFile=PR_FALSE;
    rv = aUrl->SchemeIs("file", &isFile);
    if (NS_SUCCEEDED(rv) && isFile && pageFileURL)
    {
      nsCOMPtr<nsIFile> pageFile;
      pageFileURL->GetFile(getter_AddRefs(pageFile));
    
      PRBool  fileExists;
      if (pageFile && NS_SUCCEEDED(pageFile->Exists(&fileExists)) && fileExists)
      {
        nsCOMPtr<nsIDOMDocument>  domDoc;
        editor->GetDocument(getter_AddRefs(domDoc));
        nsCOMPtr<nsIDiskDocument> diskDoc(do_QueryInterface(domDoc));
        if (diskDoc)
          diskDoc->InitDiskDocument(pageFile);
      }
    }
  }

  // Set the editor-specific Window caption
  UpdateWindowTitleAndRecentMenu(PR_TRUE);

  nsCOMPtr<nsIEditorStyleSheets> styleSheets = do_QueryInterface(mEditor);
  if (!styleSheets)
    return NS_NOINTERFACE;

  // Load style sheet with settings that should never
  //  change, even in "Browser" mode
  // We won't unload this, so we don't need to be returned the style sheet pointer
  
  
  styleSheets->ApplyOverrideStyleSheet(NS_ConvertASCIItoUCS2("chrome://editor/content/EditorOverride.css"),
                                       getter_AddRefs(mBaseStyleSheet));

  SetDisplayMode(mDisplayMode);

#ifdef DEBUG
  // Activate the debug menu only in debug builds
  // by removing the "hidden" attribute set "true" in XUL
  nsCOMPtr<nsIDOMElement> elem;
  rv = dDoc->GetElementById(NS_ConvertASCIItoUCS2("debugMenu"), getter_AddRefs(elem));
  if (elem)
    elem->RemoveAttribute(NS_ConvertASCIItoUCS2("hidden"));
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

nsresult nsEditorShell::GetDocumentEventReceiver(nsIDOMEventReceiver **aEventReceiver)
{
  if (!aEventReceiver) return NS_ERROR_NULL_POINTER;
  if (!mContentWindow || !mEditor) return NS_ERROR_NOT_INITIALIZED;

  nsCOMPtr<nsIDOMDocument> domDoc;

  if(!mContentWindow)
    return NS_ERROR_NOT_INITIALIZED;
  nsCOMPtr<nsIDOMWindowInternal> cwP = do_QueryReferent(mContentWindow);
  if (!cwP) return NS_ERROR_NOT_INITIALIZED;
    cwP->GetDocument(getter_AddRefs(domDoc));
  //mContentWindow->GetDocument(getter_AddRefs(domDoc));

  if (!domDoc) return NS_ERROR_NOT_INITIALIZED;

  nsCOMPtr<nsIDOMElement> rootElement;
  nsCOMPtr<nsIEditor> editor = do_QueryInterface(mEditor);
  nsresult rv = editor->GetRootElement(getter_AddRefs(rootElement));

  nsCOMPtr<nsIDOMEventReceiver> erP;
  rv = rootElement->QueryInterface(NS_GET_IID(nsIDOMEventReceiver), getter_AddRefs(erP));

  if (erP)
  {
    *aEventReceiver = erP;
    NS_ADDREF(*aEventReceiver);
  }  
  return rv;
}


NS_IMETHODIMP    
nsEditorShell::SetContentWindow(nsIDOMWindowInternal* aWin)
{
  NS_PRECONDITION(aWin != nsnull, "null ptr");
  if (!aWin)
      return NS_ERROR_NULL_POINTER;

  mContentWindow = getter_AddRefs( NS_GetWeakReference(aWin) );  // weak reference to aWin
  //mContentWindow = aWin;

  nsresult  rv;
  nsCOMPtr<nsIScriptGlobalObject> globalObj = do_QueryReferent(mContentWindow, &rv);
  if (NS_FAILED(rv) || !globalObj)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDocShell> docShell;
  globalObj->GetDocShell(getter_AddRefs(docShell));
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
    rv = webProgress->AddProgressListener(this);
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
  //rv = mContentWindow->GetControllers(getter_AddRefs(controllers));
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
    // the second is a composer controller, and takes an nsIEditorShell as the refCon
    nsCOMPtr<nsIController> controller = do_CreateInstance("@mozilla.org/editor/composercontroller;1", &rv);
    if (NS_FAILED(rv)) return rv;  
    nsCOMPtr<nsIEditorController> editorController = do_QueryInterface(controller);
    
    nsCOMPtr<nsISupports> shellAsISupports = do_QueryInterface((nsIEditorShell*)this);
    rv = editorController->Init(shellAsISupports);
    if (NS_FAILED(rv)) return rv;
    
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

  char* cstr = str.ToNewCString();
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
  theType.ToLowerCase();

  PRBool textMail = theType.EqualsWithConversion("textmail");
  mMailCompose = theType.EqualsWithConversion("htmlmail") || textMail;

  if (mMailCompose ||theType.EqualsWithConversion("text") || theType.EqualsWithConversion("html") || theType.IsEmpty())
  {
    // We don't store a separate type for textmail
    if (textMail)
      mEditorTypeString = NS_ConvertASCIItoUCS2("text");
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

  *_retval = mEditorTypeString.ToNewUnicode();

  return NS_OK;
}

/* attribute string contentsMIMEType; */
NS_IMETHODIMP
nsEditorShell::GetContentsMIMEType(char * *aContentsMIMEType)
{
  NS_ENSURE_ARG_POINTER(aContentsMIMEType);
  *aContentsMIMEType = mContentMIMEType.ToNewCString();
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
  
  if (mEditorTypeString.EqualsWithConversion("text"))
  {
    PRInt16 flags = nsIPlaintextEditor::eEditorPlaintextMask | nsIPlaintextEditor::eEditorEnableWrapHackMask;
    if (mMailCompose) flags |= nsIPlaintextEditor::eEditorMailMask;
    rv = editor->Init(aDoc, aPresShell, nsnull, selCon, flags);
    mEditorType = ePlainTextEditorType;
  }
  else if (mEditorTypeString.EqualsWithConversion("html") || mEditorTypeString.IsEmpty())  // empty string default to HTML editor
  {
    PRUint32    editorFlags = 0;
    EEditorType editorType  = eHTMLTextEditorType;
    
    // if the MIME type of the docment we are editing is text/plain, make a text editor
    if (IsSupportedTextType(mContentMIMEType))
    {
      editorFlags = nsIPlaintextEditor::eEditorPlaintextMask;
      editorType  = ePlainTextEditorType;
    }
    
    rv = editor->Init(aDoc, aPresShell, nsnull, selCon, editorFlags);
    mEditorType = editorType;
  }
  else if (mEditorTypeString.EqualsWithConversion("htmlmail"))  //  HTML editor with special mail rules
  {
    rv = editor->Init(aDoc, aPresShell, nsnull, selCon, nsIPlaintextEditor::eEditorMailMask);
    mEditorType = eHTMLTextEditorType;
  }
  else
  {
    rv = NS_ERROR_INVALID_ARG;    // this is not an editor we know about
#if DEBUG
    nsAutoString  errorMsg; errorMsg.AssignWithConversion("Failed to init editor. Unknown editor type \"");
    errorMsg += mEditorTypeString;
    errorMsg.AppendWithConversion("\"\n");
    char  *errorMsgCString = errorMsg.ToNewCString();
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
                                            nsISelectionController::SELECTION_FOCUS_REGION);
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
 
  switch (mEditorType)
  {
    case ePlainTextEditorType:
        // should we allow this?
    case eHTMLTextEditorType:
      err = mEditor->SetInlineProperty(styleAtom, nsDependentString(attr), nsDependentString(value));
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
  
  allStr.ToLowerCase();
  PRBool    doingAll = (allStr.EqualsWithConversion("all"));
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
  nsresult  err = NS_NOINTERFACE;
  nsCOMPtr<nsIHTMLEditor>  htmlEditor = do_QueryInterface(mEditor);

  if (htmlEditor)
    err = htmlEditor->IncreaseFontSize();
  return err;
}

NS_IMETHODIMP
nsEditorShell::DecreaseFontSize()
{
  nsresult  err = NS_NOINTERFACE;
  nsCOMPtr<nsIHTMLEditor>  htmlEditor = do_QueryInterface(mEditor);

  if (htmlEditor)
    err = htmlEditor->DecreaseFontSize();
  return err;
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
      *_retval = state.ToNewUnicode();
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
        if (bOL) tagStr.AssignWithConversion("ol");
        else if (bUL) tagStr.AssignWithConversion("ul");
        else if (bDL) tagStr.AssignWithConversion("dl");
        *_retval = tagStr.ToNewUnicode();
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
        if (bLI) tagStr.AssignWithConversion("li");
        else if (bDT) tagStr.AssignWithConversion("dt");
        else if (bDD) tagStr.AssignWithConversion("dd");
        *_retval = tagStr.ToNewUnicode();
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
        tagStr.AssignWithConversion("left");
      else if (firstAlign == nsIHTMLEditor::eCenter) 
        tagStr.AssignWithConversion("center");
      else if (firstAlign == nsIHTMLEditor::eRight)  
        tagStr.AssignWithConversion("right");
      else if (firstAlign == nsIHTMLEditor::eJustify)
        tagStr.AssignWithConversion("justify");
      *_retval = tagStr.ToNewUnicode();
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
    result = styleSheets->ApplyStyleSheet(aURL, nsnull);

  return result;
}

// Note: This is not undoable action (on purpose!)
NS_IMETHODIMP 
nsEditorShell::SetDisplayMode(PRInt32 aDisplayMode)
{
  if (aDisplayMode == eDisplayModeSource)
  {
    // We track only display modes that involve style sheet changes
    //   with mDisplayMode, so use a separate bool for source mode
    mHTMLSourceMode = PR_TRUE;
    // The HTML Source display mode is handled in editor.js
    return NS_OK;
  }
  mHTMLSourceMode = PR_FALSE;

  nsCOMPtr<nsIEditorStyleSheets> styleSheets = do_QueryInterface(mEditor);
  if (!styleSheets) return NS_NOINTERFACE;

  nsCOMPtr<nsIStyleSheet> nsISheet;
  nsresult res = NS_OK;

  // Extra style sheets to explain optimization testing:
  // eDisplayModePreview: No extra style sheets
  // eDisplayModePreview: 1 extra sheet:  mEditModeStyleSheet
  // eDisplayModeAllTags: 2 extra sheets: mEditModeStyleSheet and mAllTagsModeStyleSheet

  if (aDisplayMode == eDisplayModePreview)
  {
    // Disable all extra "edit mode" style sheets 
    if (mEditModeStyleSheet)
    {
      nsISheet = do_QueryInterface(mEditModeStyleSheet);
      res = nsISheet->SetEnabled(PR_FALSE);
      if (NS_FAILED(res)) return res;
    }
    // Disable ShowAllTags mode if that was the previous mode
    if (mDisplayMode == eDisplayModeAllTags && mAllTagsModeStyleSheet)
    {
      nsISheet = do_QueryInterface(mAllTagsModeStyleSheet);
      res = nsISheet->SetEnabled(PR_FALSE);
    }
  }
  else if (aDisplayMode == eDisplayModeNormal)
  {
    // Don't need to activate if AllTags was last mode
    if (mDisplayMode != eDisplayModeAllTags)
    {
      // If loaded before, enable the sheet
      if (mEditModeStyleSheet)
      {
        nsISheet = do_QueryInterface(mEditModeStyleSheet);
        res = nsISheet->SetEnabled(PR_TRUE);
      }
      else
      {
    
        //Load the editmode style sheet
        res = styleSheets->ApplyOverrideStyleSheet(NS_ConvertASCIItoUCS2("chrome://editor/content/EditorContent.css"),
                                                   getter_AddRefs(mEditModeStyleSheet));
      }
      if (NS_FAILED(res)) return res;
    }

    // Disable ShowAllTags mode if that was the previous mode
    if (mDisplayMode == eDisplayModeAllTags && mAllTagsModeStyleSheet)
    {
      nsISheet = do_QueryInterface(mAllTagsModeStyleSheet);
      res = nsISheet->SetEnabled(PR_FALSE);
    }
  }
  else if (aDisplayMode == eDisplayModeAllTags)
  {
    // If loaded before, enable the sheet
    if (mAllTagsModeStyleSheet)
    {
      nsISheet = do_QueryInterface(mAllTagsModeStyleSheet);
      res = nsISheet->SetEnabled(PR_TRUE);
    }
    else
    {
      // else load it
      res = styleSheets->ApplyOverrideStyleSheet(NS_ConvertASCIItoUCS2("chrome://editor/content/EditorAllTags.css"),
                                                 getter_AddRefs(mAllTagsModeStyleSheet));
    }     
    if (NS_FAILED(res)) return res;

    // We don't need to activate "normal" mode if that was the previous mode
    if (mDisplayMode != eDisplayModeNormal)
    {
      if (mEditModeStyleSheet)
      {
        nsISheet = do_QueryInterface(mEditModeStyleSheet);
        res = nsISheet->SetEnabled(PR_TRUE);
      }
      else
      {
        res = styleSheets->ApplyOverrideStyleSheet(NS_ConvertASCIItoUCS2("chrome://editor/content/EditorContent.css"),
                                                   getter_AddRefs(mEditModeStyleSheet));
      }
    }
  }

  // Remember the new mode
  if (NS_SUCCEEDED(res)) mDisplayMode = aDisplayMode;
  return res;
}

NS_IMETHODIMP 
nsEditorShell::GetEditMode(PRInt32 *_retval)
{
  if (mHTMLSourceMode)
    *_retval = eDisplayModeSource;
  else
    *_retval = mDisplayMode;

  return NS_OK;
}
  
NS_IMETHODIMP 
nsEditorShell::IsHTMLSourceMode(PRBool *_retval)
{
  *_retval = mHTMLSourceMode;

  return NS_OK;
}

NS_IMETHODIMP 
nsEditorShell::FinishHTMLSource(void)
{
  if (mHTMLSourceMode)
  {
    // Call the JS command to convert and switch to previous edit mode
    return DoControllerCommand(NS_LITERAL_STRING("cmd_FinishHTMLSource"));
  }
  return NS_OK;
}


NS_IMETHODIMP 
nsEditorShell::DisplayParagraphMarks(PRBool aShowMarks)
{
  nsresult  res = NS_OK;

  nsCOMPtr<nsIEditorStyleSheets> styleSheets = do_QueryInterface(mEditor);
  if (!styleSheets) return NS_NOINTERFACE;
  nsCOMPtr<nsIStyleSheet> nsISheet;
  if (aShowMarks)
  {
    // Check if style sheet is already loaded -- just enable it
    if (mParagraphMarksStyleSheet)
    {
      nsISheet = do_QueryInterface(mParagraphMarksStyleSheet);
      return nsISheet->SetEnabled(PR_TRUE);
    }
    //First time used -- load the style sheet
    nsCOMPtr<nsICSSStyleSheet> styleSheet;
    res = styleSheets->ApplyOverrideStyleSheet(NS_ConvertASCIItoUCS2("chrome://editor/content/EditorParagraphMarks.css"),
                                                getter_AddRefs(mParagraphMarksStyleSheet));
  }
  else if (mParagraphMarksStyleSheet)
  {
    // Disable the style sheet
    nsISheet = do_QueryInterface(mParagraphMarksStyleSheet);
    res = nsISheet->SetEnabled(PR_FALSE);
  }
  
  return res;
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
  NS_ENSURE_SUCCESS(webNav->LoadURI(url, nsIWebNavigation::LOAD_FLAGS_NONE), NS_ERROR_FAILURE);

  return NS_OK;
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

NS_IMETHODIMP
nsEditorShell::CheckOpenWindowForURLMatch(const PRUnichar* inFileURL, nsIDOMWindowInternal* inCheckWindow, PRBool *aDidFind)
{
  NS_ENSURE_ARG_POINTER((inCheckWindow && aDidFind));
  
  *aDidFind = PR_FALSE;
  
  
  // It's really hard to compare nsIFiles with file URLs; there seems to be
  // a lot of work here.  Ideally, we should be able to use nsIFile::GetURL,
  // but it is only implemented on Windows.
  nsCAutoString  fileURL; fileURL.AssignWithConversion(inFileURL);
  
  // make a temp URL for testing against
  nsresult rv = NS_OK;
  
  nsCOMPtr<nsILocalFile> urlFile(do_CreateInstance(NS_LOCAL_FILE_CONTRACTID, &rv));
  if (NS_FAILED(rv)) return rv;
  
  rv = urlFile->SetURL(fileURL.get());
  // We fail if inFileURL isn't a "file:" URL, but that's ok.
  //TODO: When publishing is done, we should support checking remote URL as well
  if (NS_FAILED(rv)) return NS_OK;
  
  nsCOMPtr<nsIDOMWindow> contentWindow;
  inCheckWindow->GetContent(getter_AddRefs(contentWindow));
  if (contentWindow)
  {
    // get the content doc
    nsCOMPtr<nsIDOMDocument> contentDoc;          
    contentWindow->GetDocument(getter_AddRefs(contentDoc));
    nsCOMPtr<nsIDiskDocument> diskDoc(do_QueryInterface(contentDoc));   // safe with NULL contentDoc
    if (diskDoc)
    {
      nsCOMPtr<nsIFile> docFileSpec;
      if (NS_SUCCEEDED(diskDoc->GetFileSpec(getter_AddRefs(docFileSpec))) && docFileSpec)
      {
        PRBool    isSameFile;          
        if (NS_SUCCEEDED(docFileSpec->Equals(urlFile, &isSameFile)) && isSameFile)
        {
          *aDidFind = PR_TRUE;
        }
      }
    }
  }

  return NS_OK;
}

// helper function
static nsresult GetExtensionForMIMEType(const char* inMIMEType, nsACString& outExtension)
{
  nsresult  rv;
  nsCOMPtr<nsIMIMEService> mimeService = do_GetService(NS_MIMESERVICE_CONTRACTID, &rv);
  if (NS_FAILED(rv)) return rv;
  
  nsCOMPtr<nsIMIMEInfo>   mimeInfo;
  rv = mimeService->GetFromMIMEType(inMIMEType, getter_AddRefs(mimeInfo));
  if (NS_FAILED(rv)) return rv;

  nsXPIDLCString  fileExtension;
  rv = mimeInfo->FirstExtension(getter_Copies(fileExtension));
  if (NS_FAILED(rv)) return rv;
  
  // the MIME service likes to give back ".htm" for text/html files,
  // so do a special-case fix here.
  nsCAutoString   extensionStr(fileExtension);
  if (extensionStr.EqualsIgnoreCase("htm"))
    extensionStr.Assign("html");

  outExtension.Assign(extensionStr);
  
  return NS_OK;
}


NS_IMETHODIMP 
nsEditorShell::SaveDocument(PRBool aSaveAs, PRBool aSaveCopy, const PRUnichar* aMimeType, PRBool *_retval)
{
  nsresult  res = NS_NOINTERFACE;
  *_retval = PR_FALSE;

  NS_ENSURE_ARG_POINTER((aMimeType));
  if (!mEditor) return NS_ERROR_NOT_INITIALIZED;

  nsCAutoString mimeTypeCStr; mimeTypeCStr.AssignWithConversion(aMimeType);
  PRBool saveAsText = IsSupportedTextType(mimeTypeCStr);

  // Currently, we only understand plain text and html
  if (!mimeTypeCStr.Equals("text/html") && !saveAsText)
    return NS_ERROR_FAILURE;

  nsAutoString    mimeType(aMimeType);
  
  // if we're saving as text, force the text/plain MIME type (because we use this
  // to get a content serializer on save)
  if (saveAsText)
    mimeType.Assign(NS_LITERAL_STRING("text/plain").get());
  
  switch (mEditorType)
  {
    case ePlainTextEditorType:
    case eHTMLTextEditorType:
    {
      nsCOMPtr<nsIEditor> editor = do_QueryInterface(mEditor);
      if (editor)
      {
        // get the document
        nsCOMPtr<nsIDOMDocument> doc;
        res = editor->GetDocument(getter_AddRefs(doc));
        if (NS_FAILED(res)) return res;
        if (!doc) return NS_ERROR_NULL_POINTER;
  
        nsCOMPtr<nsIDiskDocument> diskDoc = do_QueryInterface(doc);
        if (!diskDoc)
          return NS_ERROR_NO_INTERFACE;

        // find out if the doc already has a fileSpec associated with it.
        nsCOMPtr<nsIFile>    docFile;
        PRBool noFileSpec = (diskDoc->GetFileSpec(getter_AddRefs(docFile)) == NS_ERROR_NOT_INITIALIZED);
        PRBool mustShowFileDialog = aSaveAs || noFileSpec;
        PRBool replacing = !aSaveAs;
        PRBool titleChanged = PR_FALSE;
        
        // Get existing document title
        nsAutoString title;
        nsCOMPtr<nsIDOMHTMLDocument> htmlDoc = do_QueryInterface(doc);
        if (!htmlDoc) return NS_ERROR_FAILURE;
        res = htmlDoc->GetTitle(title);
        if (NS_FAILED(res)) return res;
        
        if (mustShowFileDialog)
        {
          // Prompt for title ONLY if existing title is empty and we are saving to HTML
          if (!mMailCompose && (!saveAsText && mEditorType == eHTMLTextEditorType) && (title.Length() == 0))
          {
            // Use a "prompt" common dialog to get title string from user
            nsCOMPtr<nsIPromptService> dialog(do_GetService("@mozilla.org/embedcomp/prompt-service;1"));
            if (dialog)
            { 
              PRUnichar *titleUnicode = ToNewUnicode(title);
              nsAutoString captionStr, msgStr1, msgStr2;
              
              GetBundleString(NS_LITERAL_STRING("DocumentTitle"), captionStr);
              GetBundleString(NS_LITERAL_STRING("NeedDocTitle"), msgStr1); 
              GetBundleString(NS_LITERAL_STRING("DocTitleHelp"), msgStr2);
              msgStr1 += PRUnichar('\n');
              msgStr1 += msgStr2;
              
              PRBool retVal = PR_FALSE;
              if(!mContentWindow)
                return NS_ERROR_NOT_INITIALIZED;
              nsCOMPtr<nsIDOMWindow> cwP = do_QueryReferent(mContentWindow);
              if (!cwP) return NS_ERROR_NOT_INITIALIZED;

              res = dialog->Prompt(cwP, captionStr.get(), msgStr1.get(),
                                   &titleUnicode, 0, 0, &retVal); 
              
              if( retVal == PR_FALSE)
              {
                // This indicates Cancel was used -- don't continue saving
                *_retval = PR_FALSE;
                return NS_OK;
              }
              // This sets title in HTML node
              {
                nsDependentString temp(titleUnicode);
                mEditor->SetDocumentTitle(temp);
                title = temp;
              }
              nsCRT::free(titleUnicode);
              titleChanged = PR_TRUE;
            }
          }

          nsCOMPtr<nsIFilePicker> filePicker = do_CreateInstance("@mozilla.org/filepicker;1", &res);
          if (filePicker)
          {
            nsAutoString fileName;

            nsAutoString  promptString;
            if (saveAsText && mEditorType == eHTMLTextEditorType)
              GetBundleString(NS_LITERAL_STRING("ExportToText"), promptString);
            else
              GetBundleString(NS_LITERAL_STRING("SaveDocumentAs"), promptString);

            // Initialize nsIFilePicker
            nsCOMPtr<nsIDOMWindowInternal> parentWindow(do_QueryReferent(mContentWindow));
            res = filePicker->Init(parentWindow, promptString.get(), nsIFilePicker::modeSave);
            if (NS_FAILED(res))
              return res;
            
            // Set filters according to the type of output
            if (saveAsText)
              filePicker->AppendFilters(nsIFilePicker::filterText);
            else
              filePicker->AppendFilters(nsIFilePicker::filterHTML);

            filePicker->AppendFilters(nsIFilePicker::filterAll);
            
            if (noFileSpec)
            {
              // check the current url, use that file name if possible
              nsString urlstring;
              res = htmlDoc->GetURL(urlstring);

              // if it's not a local file already, grab the current file name
              if ( (urlstring.CompareWithConversion("file", PR_TRUE, 4) != 0 )
                && (urlstring.CompareWithConversion("about:blank", PR_TRUE, -1) != 0) )
              {
                // remove cruft before file name including '/'
                // if the url ends with a '/' then the whole string will be cut
                PRInt32 index = urlstring.RFindChar((PRUnichar)'/', PR_FALSE, -1, -1 );
                if ( index != -1 )
                {
                  urlstring.Cut(0, index + 1);
                  if (urlstring.Length() > 0)
                  {
                    // Then truncate at any existing "#", "?" or "." since we replace with ".html"
                    index = urlstring.RFindChar((PRUnichar)'.', PR_FALSE, -1, -1 );
                    if ( index != -1)
                      urlstring.Truncate(index);
                    if (urlstring.Length() > 0)
                    {
                      index = urlstring.RFindChar((PRUnichar)'#', PR_FALSE, -1, -1 );
                      if ( index != -1)
                        urlstring.Truncate(index);
                      if (urlstring.Length() > 0)
                      {
                        index = urlstring.RFindChar((PRUnichar)'?', PR_FALSE, -1, -1 );
                        if ( index != -1)
                          urlstring.Truncate(index);
                      }
                    }
                    if (urlstring.Length() > 0)
                    {
                      title = urlstring;
                    }
                  }
                }
              }
              
              // Use page title as suggested name for new document
              if (title.IsEmpty())
              {
                title.AppendWithConversion("untitled");
              }
              else
              {
                // Strip out quote character
                PRUnichar quote = (PRUnichar)'\"';
                title.StripChar(quote);

                //Replace "bad" filename characteres with "_"
                title.ReplaceChar(" .\\/@:", (PRUnichar)'_');
              }

              // get the correct extension from the MIME type here
              fileName = title;
              nsCAutoString fileExt(saveAsText ? "txt" : "html");
              GetExtensionForMIMEType(mimeTypeCStr.get(), fileExt);
              fileName.Append(PRUnichar('.'));
              fileName.AppendWithConversion(fileExt);
            } 
            else  // have a file spec
            {
              nsXPIDLString  leafName;
              docFile->GetUnicodeLeafName(getter_Copies(leafName));
              if (leafName.get() && *leafName)
                fileName.Assign(leafName);

              if (saveAsText)
              {
                // Replace html-related extension with "txt"
                PRInt32 index = fileName.RFind(".html", PR_TRUE);
                if (index == -1)
                  index = fileName.RFind(".htm", PR_TRUE);
                if (index == -1)
                  index = fileName.RFind(".shtml", PR_TRUE);
                if (index > 0)
                {
                  nsCAutoString fileExt("txt");
                  GetExtensionForMIMEType(mimeTypeCStr.get(), fileExt);

                  // Truncate after "." and append "txt" extension
                  fileName.SetLength(index+1);
                  fileName.AppendWithConversion(fileExt);
                }
              }

              nsCOMPtr<nsIFile> parentPath;
              if (NS_SUCCEEDED(docFile->GetParent(getter_AddRefs(parentPath))))
              {
                nsCOMPtr<nsILocalFile> localParentPath(do_QueryInterface(parentPath));
                if (localParentPath)
                  filePicker->SetDisplayDirectory(localParentPath);
              }
            }
            
            if (fileName.Length() > 0)
              filePicker->SetDefaultString(fileName.get());

            PRInt16 dialogResult;
            // Finally show the dialog
            res = filePicker->Show(&dialogResult);
            if (NS_FAILED(res))
              return res;

            if (dialogResult == nsIFilePicker::returnCancel)
            {
              // Note that *_retval = PR_FALSE at this point
              if (titleChanged)
                UpdateWindowTitleAndRecentMenu(PR_FALSE);
              return NS_OK;
            }
            replacing = (dialogResult == nsIFilePicker::returnReplace);
            
            nsCOMPtr<nsILocalFile> localFile;
            res = filePicker->GetFile(getter_AddRefs(localFile));
            if (NS_FAILED(res)) return res;
            
            docFile = do_QueryInterface(localFile, &res);
            if (NS_FAILED(res)) return res;
          }
          else
          {
            NS_ASSERTION(0, "Failed to get file widget");
            return res;
          }

          // Set the new URL for the webshell
          if (!aSaveCopy)
          {
            nsCOMPtr<nsIWebShell> webShell(do_QueryInterface(mContentAreaDocShell));
            if (webShell)
            {
              nsXPIDLCString docURLSpec;
              res = docFile->GetURL(getter_Copies(docURLSpec));
              if (NS_FAILED(res)) return res;
            
              nsAutoString  fileURLUnicode; fileURLUnicode.AssignWithConversion(docURLSpec);      
              res = webShell->SetURL(fileURLUnicode.get());
              if (NS_FAILED(res)) return res;
            }
          }
        } // mustShowFileDialog

        res = editor->SaveFile(docFile, replacing, aSaveCopy, mimeType);
        if (NS_FAILED(res))
        {
          nsAutoString saveDocStr, failedStr;
          GetBundleString(NS_LITERAL_STRING("SaveDocument"), saveDocStr);
          GetBundleString(NS_LITERAL_STRING("SaveFileFailed"), failedStr);
          Alert(saveDocStr, failedStr);
        } else {
          // File was saved successfully
          *_retval = PR_TRUE;
        }
        // Update window title to show possibly different filename
        // This also covers problem that after undoing a title change,
        //   window title looses the extra [filename] part that this adds
        UpdateWindowTitleAndRecentMenu(PR_TRUE);
      }
      break;
    }
    default:
      res = NS_ERROR_NOT_IMPLEMENTED;
  }
  return res;
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
    nsCOMPtr<nsIContentViewerFile> viewerFile = do_QueryInterface(viewer);
    if (viewerFile) {
      NS_ENSURE_SUCCESS(viewerFile->Print(PR_FALSE,nsnull), NS_ERROR_FAILURE);
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsEditorShell::GetLocalFileURL(nsIDOMWindowInternal *parent, const PRUnichar *filterType, PRUnichar **_retval)
{
  nsAutoString FilterType(filterType);
  PRBool htmlFilter = FilterType.EqualsIgnoreCase("html");
  PRBool imgFilter = FilterType.EqualsIgnoreCase("img");

  *_retval = nsnull;
  
  // TODO: DON'T ACCEPT NULL PARENT AFTER WIDGET IS FIXED
  if (/*parent||*/ !(htmlFilter || imgFilter))
    return NS_ERROR_NOT_INITIALIZED;


  nsAutoString HTMLTitle;
  GetBundleString(NS_LITERAL_STRING("OpenHTMLFile"), HTMLTitle);

  // An empty string should just result in "Open" for the dialog
  nsAutoString title;
  if (htmlFilter)
  {
    title = HTMLTitle;
  } else
  {
    nsAutoString imageTitle;
    GetBundleString(NS_LITERAL_STRING("SelectImageFile"), imageTitle);

    if (imageTitle.Length() > 0 && imgFilter)
      title = imageTitle;
  }

  nsresult res;
  nsCOMPtr<nsIFilePicker> filePicker = do_CreateInstance("@mozilla.org/filepicker;1", &res);
  if (filePicker)
  {
    res = filePicker->Init(parent, title.get(), nsIFilePicker::modeOpen);
    if (NS_FAILED(res)) return res;
    
    if (htmlFilter)
    {
      filePicker->AppendFilters(nsIFilePicker::filterHTML);
      filePicker->AppendFilters(nsIFilePicker::filterText);
      filePicker->AppendFilters(nsIFilePicker::filterAll);
    }
    else
    {
      filePicker->AppendFilters(nsIFilePicker::filterImages);
      filePicker->AppendFilters(nsIFilePicker::filterAll);      
    }
    
#if 0
    // get default directory from preference
    nsCOMPtr<nsIPref> prefs( do_GetService( NS_PREF_CONTRACTID, &res ) );
    if (prefs)
    {
      nsCOMPtr<nsILocalFile> defaultDir;
      prefs->GetFileXPref(EDITOR_DEFAULT_DIR_PREF, getter_AddRefs(defaultDir));
      if (defaultDir)
      {
        PRBool isValid = PR_FALSE;
        defaultDir->Exists(&isValid);
        if (isValid)
        {
          // Set file picker so startDir is used.
          filePicker->SetDisplayDirectory(defaultDir);
        }
      }
    }
#endif
    
    PRInt16 dialogResult;
    res = filePicker->Show(&dialogResult);
    if (NS_FAILED(res))
      return res;

    if (dialogResult != nsIFilePicker::returnCancel)
    {
      // Get the platform-specific format
      // Convert it to the string version of the URL format
      nsCOMPtr<nsIFileURL> fileURL;
      res = filePicker->GetFileURL(getter_AddRefs(fileURL));
      if (fileURL)
      {
        nsXPIDLCString url;
        res = fileURL->GetSpec(getter_Copies(url));
        if (NS_FAILED(res)) return res;
        
        nsAutoString returnVal;
        returnVal.AssignWithConversion((const char*) url);
        *_retval = returnVal.ToNewUnicode();

        if (!*_retval)
          res = NS_ERROR_OUT_OF_MEMORY;
      }

    }

#if 0
    // save default directory to preference
    if (NS_SUCCEEDED(res) && prefs)
    {
      nsCOMPtr<nsILocalFile> defaultDir;
      filePicker->GetDisplayDirectory(getter_AddRefs(defaultDir));
      if (defaultDir)
      {
        prefs->SetFileXPref(EDITOR_DEFAULT_DIR_PREF, defaultDir);
      }
    }
#endif
  }

  return res;
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
    nsCOMPtr<nsIDOMDocument>  domDoc;
    editor->GetDocument(getter_AddRefs(domDoc));
    if (domDoc)
    {
      nsCOMPtr<nsIDiskDocument> diskDoc = do_QueryInterface(domDoc);
      if (diskDoc)
      {
        // find out if the doc already has a fileSpec associated with it.
        nsCOMPtr<nsIFile> docFileSpec;
        if (NS_SUCCEEDED(diskDoc->GetFileSpec(getter_AddRefs(docFileSpec))))
        {
          nsXPIDLString fileName;
          docFileSpec->GetUnicodeLeafName(getter_Copies(fileName));
          windowCaption.AppendWithConversion(" [");
          windowCaption.Append(fileName);
          windowCaption.AppendWithConversion("]");
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
    res = DoControllerCommand(NS_LITERAL_STRING("cmd_buildRecentPagesMenu"));
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
    *title = titleStr.ToNewUnicode();
  } else {
    // Don't fail, just return an empty string    
    nsAutoString empty;
    *title = empty.ToNewUnicode();
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
  if (mEditorType != eHTMLTextEditorType)
    return NS_ERROR_NOT_IMPLEMENTED;

  res = mEditor->SetDocumentTitle(nsDependentString(title));
  if (NS_FAILED(res)) return res;

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
nsEditorShell::InsertAsQuotation(const PRUnichar *quotedText,
                                 nsIDOMNode** aNodeInserted)
{  
  nsresult  err = NS_NOINTERFACE;
  
  nsAutoString aQuotedText(quotedText);
  
  switch (mEditorType)
  {
    case ePlainTextEditorType:
    case eHTMLTextEditorType:
      {
        nsCOMPtr<nsIEditorMailSupport>  mailEditor = do_QueryInterface(mEditor);
        if (mailEditor)
          err = mailEditor->InsertAsQuotation(aQuotedText, aNodeInserted);
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

  nsAutoString aQuotedText(quotedText);
  nsAutoString aCiteString(cite);
  nsAutoString aCharset(charset);
  
  switch (mEditorType)
  {
    case ePlainTextEditorType:
      err = mailEditor->InsertAsQuotation(aQuotedText, aNodeInserted);
      break;

    case eHTMLTextEditorType:
      err = mailEditor->InsertAsCitedQuotation(aQuotedText, aCiteString,
                                               aInsertHTML,
                                               aCharset, aNodeInserted);
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
  
  nsAutoString sourceToInsert(aSourceToInsert);
  
  switch (mEditorType)
  {
    case ePlainTextEditorType:
    case eHTMLTextEditorType:
      {
        nsCOMPtr<nsIHTMLEditor>  htmlEditor = do_QueryInterface(mEditor);
        if (htmlEditor)
          err = htmlEditor->InsertHTML(sourceToInsert);
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
  
  nsAutoString sourceToInsert(aSourceToInsert);
  nsAutoString charset(aCharset);
  
  switch (mEditorType)
  {
    case ePlainTextEditorType:
    case eHTMLTextEditorType:
      {
        nsCOMPtr<nsIHTMLEditor>  htmlEditor = do_QueryInterface(mEditor);
        if (htmlEditor)
          err = htmlEditor->InsertHTMLWithCharset(sourceToInsert, charset);
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

// Both Find and FindNext call through here.
nsresult
nsEditorShell::DoFind(PRBool aFindNext)
{
  if (!mContentAreaDocShell)
    return NS_ERROR_NOT_INITIALIZED;

  PRBool foundIt = PR_FALSE;
  
  // Get find component.
  nsresult rv;
  nsCOMPtr<nsIFindComponent> findComponent = 
           do_GetService(NS_IFINDCOMPONENT_CONTRACTID, &rv);
  NS_ASSERTION(((NS_SUCCEEDED(rv)) && findComponent), "GetService failed for find component.");
  if (NS_FAILED(rv)) { return rv; }

  // make the search context if we need to
  if (!mSearchContext)
  {
    if(!mContentWindow)
      return NS_ERROR_NOT_INITIALIZED;
    nsCOMPtr<nsIDOMWindowInternal> cwP = do_QueryReferent(mContentWindow);
    if (!cwP) return NS_ERROR_NOT_INITIALIZED;
    rv = findComponent->CreateContext(cwP, this, getter_AddRefs(mSearchContext));
  }
  
  if (NS_SUCCEEDED(rv))
  {
    if (aFindNext)
      rv = findComponent->FindNext(mSearchContext, &foundIt);
    else
      rv = findComponent->Find(mSearchContext, &foundIt);
  }

  return rv;
}

NS_IMETHODIMP
nsEditorShell::Find()
{
  return DoFind(PR_FALSE);
}

NS_IMETHODIMP
nsEditorShell::FindNext()
{
  return DoFind(PR_TRUE);
}

NS_IMETHODIMP
nsEditorShell::Replace()
{
  if (!mContentAreaDocShell)
    return NS_ERROR_NOT_INITIALIZED;

  // Get find component.
  nsresult rv;
  nsCOMPtr<nsIFindComponent> findComponent = 
           do_GetService(NS_IFINDCOMPONENT_CONTRACTID, &rv);
  NS_ASSERTION(((NS_SUCCEEDED(rv)) && findComponent), "GetService failed for find component.");
  if (NS_FAILED(rv)) { return rv; }

  // make the search context if we need to
  if (!mSearchContext)
  {
    if(!mContentWindow)
      return NS_ERROR_NOT_INITIALIZED;
    nsCOMPtr<nsIDOMWindowInternal> cwP = do_QueryReferent(mContentWindow);
    if (!cwP) return NS_ERROR_NOT_INITIALIZED;
    rv = findComponent->CreateContext(cwP, this, getter_AddRefs(mSearchContext));
  }
  
  if (NS_SUCCEEDED(rv))
    rv = findComponent->Replace(mSearchContext);

  return rv;
}

/* Get localized strings for UI from the Editor's string bundle */
// Use this version from JavaScript:
NS_IMETHODIMP
nsEditorShell::GetString(const PRUnichar *stringName, PRUnichar **_retval)
{
  if (!stringName || !_retval)
    return NS_ERROR_NULL_POINTER;

  *_retval = NULL;
  
  NS_ASSERTION(mStringBundle, "No string bundle!");
  if (!mStringBundle) return NS_ERROR_NOT_INITIALIZED;
  
  return mStringBundle->GetStringFromName(stringName, _retval);
}


// Use this version within the shell:
void nsEditorShell::GetBundleString(const nsAReadableString &stringName, nsAWritableString &outString)
{
  outString.Truncate();
  
  nsXPIDLString   tempString;
  if (NS_SUCCEEDED(GetString(PromiseFlatString(stringName).get(), getter_Copies(tempString))) && tempString)
    outString = tempString.get();
}

// Utilities to bring up a Yes/No/Cancel dialog.

// For JavaScript:
NS_IMETHODIMP    
nsEditorShell::ConfirmWithTitle(const PRUnichar *aTitle, const PRUnichar *aQuestion,
                                const PRUnichar *aYesButtonText, const PRUnichar *aNoButtonText, PRInt32 *_retval)
{
  if (!aTitle || !aQuestion || !aYesButtonText || !_retval)
    return NS_ERROR_NULL_POINTER;

  nsAutoString title(aTitle);
  nsAutoString question(aQuestion);
  nsAutoString yesString(aYesButtonText);
  nsAutoString noString(aNoButtonText);

  *_retval = ConfirmWithCancel(title, question, &yesString, &noString);
  
  return NS_OK;
}

nsEditorShell::EConfirmResult
nsEditorShell::ConfirmWithCancel(const nsString& aTitle, const nsString& aQuestion, 
                                 const nsString *aYesString, const nsString *aNoString)
{
  nsEditorShell::EConfirmResult result = nsEditorShell::eCancel;

  nsIDialogParamBlock* block = NULL; 
  nsresult rv = nsComponentManager::CreateInstance("@mozilla.org/embedcomp/dialogparam;1",
                                          0, NS_GET_IID(nsIDialogParamBlock), 
                                          (void**)&block ); 
  if ( NS_SUCCEEDED(rv) )
  { 
    // Stuff in Parameters 
    block->SetString( nsPIPromptService::eMsg, aQuestion.get()); 
    NS_NAMED_LITERAL_STRING(styleClass, "question-icon");
    block->SetString(nsPIPromptService::eIconClass, styleClass.get());

    nsAutoString yesStr, noStr;
    // Default is Yes, No, Cancel
    PRInt32 numberOfButtons = 3;
    if (aYesString)
      yesStr.Assign(*aYesString);
    else
      // We always want a "Yes" string, so supply the default
      GetBundleString(NS_LITERAL_STRING("Yes"), yesStr);

    if (aNoString && aNoString->Length() > 0)
    {
      noStr.Assign(*aNoString);
      block->SetString( nsPIPromptService::eButton2Text, noStr.get() ); 
    }
    else
    {
      // No string for "No" means we only want Yes, Cancel
      numberOfButtons = 2;
    }    
    block->SetInt( nsPIPromptService::eNumberButtons, numberOfButtons ); 

    nsAutoString cancelStr;
    GetBundleString(NS_LITERAL_STRING("Cancel"), cancelStr);

    block->SetString( nsPIPromptService::eDialogTitle, aTitle.get() );
    //Note: "button0" is always Ok or Yes action, "button1" is Cancel
    block->SetString( nsPIPromptService::eButton0Text, yesStr.get() ); 
    block->SetString( nsPIPromptService::eButton1Text, cancelStr.get() ); 

    nsCOMPtr<nsPIPromptService> dialog(do_GetService("@mozilla.org/embedcomp/prompt-service;1"));
    if (dialog)
    { 
      PRInt32 buttonPressed = 0; 
      if(!mContentWindow)
        return result;
      nsCOMPtr<nsIDOMWindow> cwP = do_QueryReferent(mContentWindow);
      if (!cwP) return result;
      rv = dialog->DoDialog( cwP, block, "chrome://global/content/commonDialog.xul" ); 
      block->GetInt( nsPIPromptService::eButtonPressed, &buttonPressed ); 
      // NOTE: If order of buttons changes in nsIPromptService,
      //       then we must change the EConfirmResult enums in nsEditorShell.h
      result = nsEditorShell::EConfirmResult(buttonPressed);
    } 
    NS_IF_RELEASE( block );
  }
  return result;
} 

// Utility to bring up a OK/Cancel dialog.
PRBool    
nsEditorShell::Confirm(const nsString& aTitle, const nsString& aQuestion)
{
  nsresult rv;
  PRBool   result = PR_FALSE;

  nsCOMPtr<nsIPromptService> dialog(do_GetService("@mozilla.org/embedcomp/prompt-service;1"));
  if (dialog)
  {
    if(!mContentWindow)
      return NS_ERROR_NOT_INITIALIZED;
    nsCOMPtr<nsIDOMWindow> cwP = do_QueryReferent(mContentWindow);
    if (!cwP) return NS_ERROR_NOT_INITIALIZED;
    rv = dialog->Confirm(cwP, aTitle.get(), aQuestion.get(), &result);
  }
  return result;
}

NS_IMETHODIMP    
nsEditorShell::AlertWithTitle(const PRUnichar *aTitle, const PRUnichar *aMsg)
{
  if (!aTitle || !aMsg)
    return NS_ERROR_NULL_POINTER;

  nsresult rv = NS_ERROR_FAILURE;
  nsCOMPtr<nsIPromptService> dialog(do_GetService("@mozilla.org/embedcomp/prompt-service;1"));
  if (dialog)
  {
    if(!mContentWindow)
      return NS_ERROR_NOT_INITIALIZED;
    nsCOMPtr<nsIDOMWindow> cwP = do_QueryReferent(mContentWindow);
    if (!cwP) return NS_ERROR_NOT_INITIALIZED;
    rv = dialog->Alert(cwP, aTitle, aMsg);
  }

  return rv;
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
  
  if(NS_SUCCEEDED(res)) {
    nsCOMPtr<nsIScriptGlobalObject> globalObj( do_QueryReferent(mContentWindow));
    if (!globalObj) {
      return NS_ERROR_FAILURE;
    }

    nsCOMPtr<nsIDocShell> docShell;
    globalObj->GetDocShell(getter_AddRefs(docShell));
    if (docShell)
    {
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
    err = editor->OutputToString(contentsAs, aFormat, flags);

  *aContentsAs = contentsAs.ToNewUnicode();
  
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

  *aHeadContents = headContents.ToNewUnicode();
  
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
nsEditorShell::MakeOrChangeList(const PRUnichar *listType, PRBool entireList)
{
  nsresult err = NS_NOINTERFACE;

  if (!mEditor) return NS_ERROR_NOT_INITIALIZED;

  nsAutoString aListType(listType);
  
  switch (mEditorType)
  {
    case eHTMLTextEditorType:
      if (aListType.IsEmpty())
      {
        err = mEditor->RemoveList(NS_ConvertASCIItoUCS2("ol"));
        if(NS_SUCCEEDED(err))
        {
          err = mEditor->RemoveList(NS_ConvertASCIItoUCS2("ul"));
          if(NS_SUCCEEDED(err))
            err = mEditor->RemoveList(NS_ConvertASCIItoUCS2("dl"));
        }
      }
      else
        err = mEditor->MakeOrChangeList(aListType, entireList);
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
        result = tableEditor->GetFirstSelectedCell(aOutElement, nsnull);
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
        result = tableEditor->GetFirstSelectedCellInTable(aOutElement, aRowIndex, aColIndex);
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
        result = tableEditor->GetNextSelectedCell(aOutElement, nsnull);
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
          result = tableEditor->GetCellIndexes(cellElement, *_retval, colIndex);
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
          result = tableEditor->GetCellIndexes(cellElement, rowIndex, *_retval);
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
          result = tableEditor->GetTableSize(tableElement, *_retval, cols);
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
          result = tableEditor->GetTableSize(tableElement, rows, *_retval);
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
                                            *aStartRowIndex, *aStartColIndex, 
                                            *aRowSpan, *aColSpan, 
                                            *aActualRowSpan, *aActualColSpan,
                                            *aIsSelected);
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
          result = tableEditor->GetSelectedOrParentTableElement(*_retval, TagName, *aSelectedCount);
          *aTagName = TagName.ToNewUnicode();
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
          result = tableEditor->GetSelectedCellsType(aElement, *_retval);
      }
      break;
    default:
      result = NS_ERROR_NOT_IMPLEMENTED;
  }
  return result;
}

/* end of table editing */

NS_IMETHODIMP
nsEditorShell::GetEmbeddedObjects(nsISupportsArray **aObjectArray)
{
  if (!aObjectArray)
    return NS_ERROR_NULL_POINTER;

  nsresult result = NS_NOINTERFACE;

  switch (mEditorType)
  {
    case eHTMLTextEditorType:
    {
      nsCOMPtr<nsIEditorMailSupport> mailEditor = do_QueryInterface(mEditor);
      if (mailEditor)
        result = mailEditor->GetEmbeddedObjects(aObjectArray);
    }
    break;

    default:
      result = NS_NOINTERFACE;
  }
  return result;
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

    PRUnichar *dictName = nsnull;

    nsCOMPtr<nsIPref> prefs(do_GetService(kPrefServiceCID, &result));

    if (NS_SUCCEEDED(result) && prefs)
      result = prefs->CopyUnicharPref("spellchecker.dictionary", &dictName);

    if (! dictName || ! *dictName)
    {
      // Prefs didn't give us a dictionary name, so just get the current
      // locale and use that as the default dictionary name!

      if (dictName)
      {
        nsMemory::Free(dictName);
        dictName = nsnull;
      }

      nsCOMPtr<nsIChromeRegistry> chromeRegistry = do_GetService(kChromeRegistryCID, &result);

      if (NS_SUCCEEDED(result) && chromeRegistry)
        result = chromeRegistry->GetSelectedLocale(NS_LITERAL_STRING("navigator").get(), &dictName);
    }

    if (NS_SUCCEEDED(result) && dictName && *dictName)
      result = SetCurrentDictionary(dictName);

    if (dictName)
      nsMemory::Free(dictName);

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
  *aNextMisspelledWord = nextMisspelledWord.ToNewUnicode();
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
  *aSuggestedWord = word.ToNewUnicode();
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
  *aDictionaryWord = word.ToNewUnicode();
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
      tmpPtr[i] = dictStr.ToNewUnicode();
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

    *aDictionary = dictStr.ToNewUnicode();
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

/* void getProtocolHandler (in nsIURI aURI, out nsIProtocolHandler aProtocolHandler); */
NS_IMETHODIMP nsEditorShell::GetProtocolHandler(nsIURI *aURI, nsIProtocolHandler **aProtocolHandler)
{
  NS_ENSURE_ARG_POINTER(aProtocolHandler);
  *aProtocolHandler = nsnull;
  return NS_OK;
}

/* void doContent (in string aContentType, in nsURILoadCommand aCommand, in nsIChannel aOpenedChannel, out nsIStreamListener aContentHandler, out boolean aAbortProcess); */
NS_IMETHODIMP nsEditorShell::DoContent(const char *aContentType, nsURILoadCommand aCommand, nsIRequest* request, nsIStreamListener **aContentHandler, PRBool *aAbortProcess)
{
  NS_ENSURE_ARG_POINTER(aContentHandler);
  NS_ENSURE_ARG_POINTER(aAbortProcess);
  *aContentHandler = nsnull;
  *aAbortProcess = PR_FALSE;
  return NS_OK;
}

/* boolean isPreferred (in string aContentType, in nsURILoadCommand aCommand, out string aDesiredContentType); */
NS_IMETHODIMP nsEditorShell::IsPreferred(const char *aContentType, nsURILoadCommand aCommand, char **aDesiredContentType, PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(aDesiredContentType);
  NS_ENSURE_ARG_POINTER(_retval);
  *aDesiredContentType = nsnull;
  *_retval = PR_FALSE;
  return NS_OK;
}

/* boolean canHandleContent (in string aContentType, in nsURILoadCommand aCommand, out string aDesiredContentType); */
NS_IMETHODIMP nsEditorShell::CanHandleContent(const char *aContentType, nsURILoadCommand aCommand, char **aDesiredContentType, PRBool *_retval)
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
                             PRInt32 aStateFlags,
                             nsresult aStatus)
{
  nsresult    rv = NS_OK;
  
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
  return NS_OK;
}

NS_IMETHODIMP 
nsEditorShell::OnStatusChange(nsIWebProgress* aWebProgress,
                              nsIRequest* aRequest,
                              nsresult aStatus,
                              const PRUnichar* aMessage)
{
  return NS_OK;
}

NS_IMETHODIMP
nsEditorShell::OnSecurityChange(nsIWebProgress *aWebProgress,
                                nsIRequest *aRequest,
                                PRInt32 state)
{
  return NS_OK;
}

nsresult nsEditorShell::StartPageLoad(nsIChannel *aChannel)
{
  nsXPIDLCString  contentType;
  aChannel->GetContentType(getter_Copies(contentType));
  
  // save the original MIME type; we'll use it later
  if (contentType.get())
    mContentMIMEType.Assign(contentType);
  
  if (nsCRT::strcmp(contentType, "text/html") == 0)
  {
    // fine, do nothing
    mContentTypeKnown = PR_TRUE;
  }
  else if (IsSupportedTextType(contentType))
  {
    // set the mime type to text/plain so that it renders as text
    aChannel->SetContentType("text/plain");
    mContentTypeKnown = PR_TRUE;
  }
  else
  {
    // we don't know what this is yet. It might be HTML loaded from 
    // a directory URL (http://www.foo.com/). We'll know the real
    // MIME type later.
    mContentTypeKnown = PR_FALSE;
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
    mParserObserver->RegisterTagToWatch("FRAMESET");
    mParserObserver->RegisterTagToWatch("IFRAME");
    mParserObserver->Start();
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

  // Is this a MIME type we can handle?
  if (aChannel)
  {
    // if we didn't get the content-type at the start of the load, get it now
    if (!mContentTypeKnown)
    {
      nsXPIDLCString  contentType;
      aChannel->GetContentType(getter_Copies(contentType));

      if (contentType.get())
        mContentMIMEType.Assign(contentType);
    }
  }    
    
  if ( !mContentMIMEType.Equals("text/html") && !IsSupportedTextType(mContentMIMEType) )
  {
      mCloseWindowWhenLoaded = PR_TRUE;
      mCantEditReason = eCantEditMimeType;
  }

  nsAutoString doneText;
  GetBundleString(NS_LITERAL_STRING("LoadingDone"), doneText);
  SetChromeAttribute(mDocShell, "statusText", NS_LITERAL_STRING("label"), doneText);

  // Display an Alert dialog if the page cannot be edited...
  if (mCloseWindowWhenLoaded)
  {
    nsAutoString alertLabel, alertMessage;
    GetBundleString(NS_LITERAL_STRING("Alert"), alertLabel);
    
    nsAutoString  stringID;
    switch (mCantEditReason)
    {
      case eCantEditFramesets:
        stringID.AssignWithConversion("CantEditFramesetMsg");
        break;        
      case eCantEditMimeType:
        stringID.AssignWithConversion("CantEditMimeTypeMsg");
        break;
      case eCantEditOther:
        stringID.AssignWithConversion("CantEditDocumentMsg");
        break;
      default:
        // Do nothing.
        break;
    }
    
    GetBundleString(stringID, alertMessage);
    Alert(alertLabel, alertMessage);

    nsCOMPtr<nsIBaseWindow> baseWindow;
    GetTreeOwner(mDocShell, getter_AddRefs(baseWindow));
    NS_ENSURE_TRUE(baseWindow, NS_ERROR_ABORT);
    baseWindow->Destroy();

    return NS_ERROR_ABORT;
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
      scriptContext->SetScriptsEnabled(PR_FALSE);
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


NS_IMETHODIMP
nsEditorShell::HandleMouseClickOnElement(nsIDOMElement *aElement, PRInt32 aClickCount,
                                         PRInt32 x, PRInt32 y, PRBool *_retval)
{
  // Guess it's ok if we don't have an element
  if (!aElement) return NS_OK;
  if (!_retval)  return NS_ERROR_NULL_POINTER;

  *_retval = PR_FALSE;

  // We'll only look at single and double-click
  if (aClickCount > 2) return NS_OK;
  
  nsresult rv = NS_OK;

/*
#if DEBUG_cmanske
  nsAutoString TagName;
  aElement->GetTagName(TagName);
  TagName.ToLowerCase();
  char szTagName[64];
  TagName.ToCString(szTagName, 64);
  printf("***** Element clicked on: %s, x=%d, y=%d\n", szTagName, x, y);
#endif
*/
  if (mDisplayMode == eDisplayModeAllTags) 
  {
    // Always select the element in AllTags mode
    //  in other modes, clicking on images, hline, etc 
    //  already selects them correctly
    // Selection here is used to make clicking on the 
    //  background image in ShowAllTags mode select
    //  contents of a element
    // TODO: It would be great if we could use x, y to restrict
    //  where you click for easier caret placement near border with content,
    //  but:
    //  1. We can get x,y, relative to either screen or "widget" (contentWindow)
    //      origin, but not the element clicked on!
    //  2.  we need to get the size of the element!

    rv = SelectElement(aElement);
    if (NS_SUCCEEDED(rv))
      *_retval = PR_TRUE;
  }

  // For double-click, edit element properties
  if (aClickCount == 2)
  {
    // In "All Tags" mode, use AdvancedProperties,
    //  in others use appriate object property dialog
    if (mDisplayMode == eDisplayModeAllTags)
        rv = DoControllerCommand(NS_LITERAL_STRING("cmd_advancedProperties"));
    else
        rv = DoControllerCommand(NS_LITERAL_STRING("cmd_objectProperties"));
        
    if (NS_SUCCEEDED(rv))
      *_retval = PR_TRUE;
  }

  return rv;
}

nsresult
nsEditorShell::DoControllerCommand(const nsAReadableString& aCommand)
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

