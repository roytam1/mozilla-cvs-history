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

#include <stdio.h>

#include "nsEditorShell.h"
#include "nsIBrowserWindow.h"
#include "nsIWebShell.h"
#include "nsIBaseWindow.h"
#include "nsIContentViewerFile.h"
#include "pratom.h"
#include "prprf.h"
#include "nsIComponentManager.h"

#include "nsIScriptContext.h"
#include "nsIScriptContextOwner.h"
#include "nsIScriptGlobalObject.h"
#include "nsIDOMDocument.h"
#include "nsIDOMXULDocument.h"
#include "nsIDOMHTMLDocument.h"
#include "nsIDiskDocument.h"
#include "nsIDocument.h"
#include "nsIDOMWindow.h"
#include "nsIDOMNode.h"
#include "nsIDOMNodeList.h"
#include "nsICSSLoader.h"
#include "nsICSSStyleSheet.h"
#include "nsIHTMLContentContainer.h"
#include "nsIStyleSet.h"
#include "nsIURI.h"
#include "nsNetUtil.h"

#include "nsIScriptGlobalObject.h"
#include "nsIWebShell.h"
#include "nsIWebShellWindow.h"
#include "nsCOMPtr.h"

#include "nsIServiceManager.h"
#include "nsIURL.h"
#include "nsIWidget.h"
#include "nsIWindowMediator.h"
#include "plevent.h"

#include "nsIAppShell.h"
#include "nsIAppShellService.h"
#include "nsAppShellCIDs.h"

#include "nsIDocumentViewer.h"
#include "nsIDOMHTMLImageElement.h"
#include "nsIPresShell.h"
#include "nsIPresContext.h"
#include "nsIDOMSelection.h"

#include "nsIFileWidget.h"
#include "nsFileSpec.h"
#include "nsIFindComponent.h"
#include "nsIPrompt.h"
#include "nsICommonDialogs.h"

#include "nsIEditorController.h"
#include "nsEditorController.h"
#include "nsIControllers.h"


///////////////////////////////////////
// Editor Includes
///////////////////////////////////////
#include "nsIDOMEventReceiver.h"
#include "nsIDOMEventCapturer.h"
#include "nsString.h"
#include "nsIDOMText.h"
#include "nsIDOMElement.h"
#include "nsIDOMDocument.h"

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

///////////////////////////////////////

// Drag & Drop, Clipboard
#include "nsWidgetsCID.h"
#include "nsIClipboard.h"
#include "nsITransferable.h"
#include "nsISupportsArray.h"

/* Define Class IDs */
static NS_DEFINE_IID(kAppShellServiceCID,       NS_APPSHELL_SERVICE_CID);
static NS_DEFINE_CID(kHTMLEditorCID,            NS_HTMLEDITOR_CID);
static NS_DEFINE_CID(kCTextServicesDocumentCID, NS_TEXTSERVICESDOCUMENT_CID);
static NS_DEFINE_CID(kCSpellCheckerCID,         NS_SPELLCHECKER_CID);
static NS_DEFINE_IID(kCFileWidgetCID,           NS_FILEWIDGET_CID);
static NS_DEFINE_CID(kCStringBundleServiceCID,  NS_STRINGBUNDLESERVICE_CID);
static NS_DEFINE_CID(kCommonDialogsCID,         NS_CommonDialog_CID );
static NS_DEFINE_CID(kDialogParamBlockCID,      NS_DialogParamBlock_CID);
static NS_DEFINE_CID(kEditorControllerCID,      NS_EDITORCONTROLLER_CID);

/* Define Interface IDs */
static NS_DEFINE_IID(kISupportsIID,             NS_ISUPPORTS_IID);

#define APP_DEBUG 0 

#define EDITOR_BUNDLE_URL "chrome://editor/locale/editor.properties"

/////////////////////////////////////////////////////////////////////////
// Utility to extract document from a webshell object.
static nsresult
GetDocument(nsIWebShell *aWebShell, nsIDocument **aDoc ) 
{
  // Get content viewer from the web shell.
  nsCOMPtr<nsIContentViewer> contentViewer;
  nsresult res = (aWebShell && aDoc)? aWebShell->GetContentViewer(getter_AddRefs(contentViewer))
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

// Utility to set and attribute of an element (used for throbber)
static nsresult 
SetChromeAttribute( nsIWebShell *shell, const char *id, 
                    const char *name,  const nsString &value )
{
  nsCOMPtr<nsIDocument> doc;
  nsresult rv = GetDocument( shell, getter_AddRefs(doc) );
  if(NS_SUCCEEDED(rv) && doc)
  {
    // Up-cast.
    nsCOMPtr<nsIDOMXULDocument> xulDoc( do_QueryInterface(doc) );
    if ( xulDoc )
    {
      // Find specified element.
      nsCOMPtr<nsIDOMElement> elem;
      rv = xulDoc->GetElementById( id, getter_AddRefs(elem) );
      if ( elem )
        // Set the text attribute.
        rv = elem->SetAttribute( name, value );
    }
  }
  return rv;
}

/////////////////////////////////////////////////////////////////////////
// nsEditorShell
/////////////////////////////////////////////////////////////////////////

nsEditorShell::nsEditorShell()
:  mToolbarWindow(nsnull)
,  mContentWindow(nsnull)
,  mWebShellWin(nsnull)
,  mWebShell(nsnull)
,  mContentAreaWebShell(nsnull)
,  mEditorType(eUninitializedEditorType)
,  mStateMaintainer(nsnull)
,  mWrapColumn(0)
,  mSuggestedWordIndex(0)
,  mDictionaryIndex(0)
,  mStringBundle(0)
,  mEditModeStyleSheet(0)
{
#ifdef APP_DEBUG
  printf("Created nsEditorShell\n");
#endif

  NS_INIT_REFCNT();
}

nsEditorShell::~nsEditorShell()
{
  NS_IF_RELEASE(mStateMaintainer);
  
  // the only other references we hold are in nsCOMPtrs, so they'll take
  // care of themselves.
}

NS_IMPL_ADDREF(nsEditorShell)
NS_IMPL_RELEASE(nsEditorShell)

NS_IMETHODIMP 
nsEditorShell::QueryInterface(REFNSIID aIID,void** aInstancePtr)
{
  if (aInstancePtr == NULL) {
    return NS_ERROR_NULL_POINTER;
  }

  // Always NULL result, in case of failure
  *aInstancePtr = NULL;

  if ( aIID.Equals(kISupportsIID)) {
    *aInstancePtr = (void*) ((nsISupports*)((nsIEditorShell *)this));
    AddRef();
    return NS_OK;
  }
  else if ( aIID.Equals(NS_GET_IID(nsIEditorShell)) ) {
    *aInstancePtr = (void*) ((nsIEditorShell*)this);
    AddRef();
    return NS_OK;
  }
  else if ( aIID.Equals(NS_GET_IID(nsIEditorSpellCheck)) ) {
    *aInstancePtr = (void*) ((nsIEditorSpellCheck*)this);
    AddRef();
    return NS_OK;
  }
  else if (aIID.Equals(NS_GET_IID(nsIDocumentLoaderObserver))) {
    *aInstancePtr = (void*) ((nsIDocumentLoaderObserver*)this);
     AddRef();
    return NS_OK;
  }
 
  return NS_ERROR_NO_INTERFACE;
}

NS_IMETHODIMP    
nsEditorShell::Init()
{  
  //nsBaseAppCore::Init(aId);

  nsAutoString    editorType = "html";      // default to creating HTML editor
  mEditorTypeString = editorType;
  mEditorTypeString.ToLowerCase();

  // Get pointer to our string bundle
  nsresult res;
  NS_WITH_SERVICE(nsIStringBundleService, service, kCStringBundleServiceCID, &res);
  if (NS_FAILED(res)) { 
    printf("ERROR: Failed to get StringBundle Service instance.\n");
    return res;
  }
  nsILocale* locale = nsnull;
  res = service->CreateBundle(EDITOR_BUNDLE_URL, locale, 
                                 getter_AddRefs(mStringBundle));

  // XXX: why are we returning NS_OK here rather than res?
  // is it ok to fail to get a string bundle?  if so, it should be documented.
  return NS_OK;
}

NS_IMETHODIMP    
nsEditorShell::PrepareDocumentForEditing(nsIURI *aUrl)
{
  if (!mContentAreaWebShell)
    return NS_ERROR_NOT_INITIALIZED;

  if (mEditor)
  {
    // Mmm, we have an editor already. That means that someone loaded more than
    // one URL into the content area. Let's tear down what we have, and rip 'em a
    // new one.

    // first, unregister the selection listener, if there was one
    if (mStateMaintainer)
    {
      nsCOMPtr<nsIDOMSelection> domSelection;
      // using a scoped result, because we don't really care if this fails
      nsresult result = GetEditorSelection(getter_AddRefs(domSelection));
      if (NS_SUCCEEDED(result) && domSelection)
      {
        domSelection->RemoveSelectionListener(mStateMaintainer);
        NS_IF_RELEASE(mStateMaintainer);
      }
    }
    
    mEditorType = eUninitializedEditorType;
    mEditor = 0;  // clear out the nsCOMPtr

    // and tell them that they are doing bad things
    NS_WARNING("Multiple loads of the editor's document detected.");
    // Note that if you registered doc state listeners before the second
    // URL load, they don't get transferred to the new editor.
  }
  
  nsresult rv = DoEditorMode(mContentAreaWebShell);
  if (NS_FAILED(rv)) return rv;
  
  // transfer the doc state listeners to the editor
  rv = TransferDocumentStateListeners();
  if (NS_FAILED(rv)) return rv;
  
  // make the UI state maintainer
  mStateMaintainer = new nsInterfaceState;
  if (!mStateMaintainer) return NS_ERROR_OUT_OF_MEMORY;
  mStateMaintainer->AddRef();      // the owning reference
  rv = mStateMaintainer->Init(mEditor, mWebShell);
  if (NS_FAILED(rv)) return rv;
  
  // set it up as a selection listener
  nsCOMPtr<nsIDOMSelection> domSelection;
  rv = GetEditorSelection(getter_AddRefs(domSelection));
  if (NS_FAILED(rv)) return rv;

  rv = domSelection->AddSelectionListener(mStateMaintainer);
  if (NS_FAILED(rv)) return rv;

  // and set it up as a doc state listener
  nsCOMPtr<nsIEditor> editor = do_QueryInterface(mEditor, &rv);
  if (NS_FAILED(rv)) return rv;
  rv = editor->AddDocumentStateListener(mStateMaintainer);
  if (NS_FAILED(rv)) return rv;
  

  if (NS_SUCCEEDED(rv) && mContentWindow)
  {
    nsCOMPtr<nsIController> controller;
    nsCOMPtr<nsIControllers> controllers;
    rv = nsComponentManager::CreateInstance(kEditorControllerCID, nsnull, NS_GET_IID(nsIController), getter_AddRefs(controller));
    if (NS_SUCCEEDED(rv) && controller)
    {
      rv = mContentWindow->GetControllers(getter_AddRefs(controllers));
      if (NS_SUCCEEDED(rv) && controllers)
      {
        nsCOMPtr<nsIEditorController> ieditcontroller = do_QueryInterface(controller);
        ieditcontroller->SetEditor(editor);//weak link

        rv = controllers->InsertControllerAt(0,controller);
      }
    }
  }

  // now all the listeners are set up, we can call PostCreate
  rv = editor->PostCreate();
  if (NS_FAILED(rv)) return rv;
  
  // get the URL of the page we are editing
  if (aUrl)
  {
    char* pageURLString = nsnull;                                               
    char* pageScheme = nsnull;                                                  
    aUrl->GetScheme(&pageScheme);                                               
    aUrl->GetSpec(&pageURLString);

    // only save the file spec if this is a local file, and is not              
    // about:blank                                                              
    if (nsCRT::strncmp(pageScheme, "file", 4) == 0 &&                           
        nsCRT::strncmp(pageURLString,"about:blank", 11) != 0)                   
    {
      nsFileURL    pageURL(pageURLString);
      nsFileSpec   pageSpec(pageURL);

      nsCOMPtr<nsIDOMDocument>  domDoc;
      editor->GetDocument(getter_AddRefs(domDoc));
    
      if (domDoc)
      {
        nsCOMPtr<nsIDiskDocument> diskDoc = do_QueryInterface(domDoc);
        if (diskDoc)
          diskDoc->InitDiskDocument(&pageSpec);
      }
    }
    if (pageURLString)
      nsCRT::free(pageURLString);
    if (pageScheme)
      nsCRT::free(pageScheme);
  }
  // Set the editor-specific Window caption
  UpdateWindowTitle();

  nsCOMPtr<nsIEditorStyleSheets> styleSheets = do_QueryInterface(mEditor);
  if (!styleSheets)
    return NS_NOINTERFACE;

  // Load style sheet with settings that should never
  //  change, even in "Browser" mode
  styleSheets->ApplyOverrideStyleSheet("chrome://editor/content/EditorOverride.css");

  // Load the edit mode override style sheet
  // This will be remove for "Browser" mode
  SetDisplayMode(eDisplayModeEdit);

  // Force initial focus to the content window -- HOW?
//  mWebShellWin->SetFocus();
  return NS_OK;
}

NS_IMETHODIMP    
nsEditorShell::SetToolbarWindow(nsIDOMWindow* aWin)
{
  NS_PRECONDITION(aWin != nsnull, "null ptr");
  if (!aWin)
      return NS_ERROR_NULL_POINTER;

  mToolbarWindow = aWin;

  return NS_OK;
}

NS_IMETHODIMP    
nsEditorShell::SetContentWindow(nsIDOMWindow* aWin)
{
  NS_PRECONDITION(aWin != nsnull, "null ptr");
  if (!aWin)
      return NS_ERROR_NULL_POINTER;

  mContentWindow = aWin;

  nsresult  rv;
  nsCOMPtr<nsIScriptGlobalObject> globalObj = do_QueryInterface(mContentWindow, &rv);
  if (NS_FAILED(rv) || !globalObj)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIWebShell> webShell;
  globalObj->GetWebShell(getter_AddRefs(webShell));
  if (!webShell)
    return NS_ERROR_FAILURE;
    
  mContentAreaWebShell = webShell;      // dont AddRef
  return mContentAreaWebShell->SetDocLoaderObserver((nsIDocumentLoaderObserver *)this);
}


NS_IMETHODIMP    
nsEditorShell::SetWebShellWindow(nsIDOMWindow* aWin)
{
  NS_PRECONDITION(aWin != nsnull, "null ptr");
  if (!aWin)
      return NS_ERROR_NULL_POINTER;

  nsCOMPtr<nsIScriptGlobalObject> globalObj( do_QueryInterface(aWin) );
  if (!globalObj) {
    return NS_ERROR_FAILURE;
  }

  nsresult  rv = NS_OK;
  
  nsCOMPtr<nsIWebShell> webShell;
  globalObj->GetWebShell(getter_AddRefs(webShell));
  if (!webShell)
    return NS_ERROR_NOT_INITIALIZED;

  mWebShell = webShell;
  //NS_ADDREF(mWebShell);
  
#ifdef APP_DEBUG
  const PRUnichar * name;
  webShell->GetName( &name);
  nsAutoString str(name);

  char* cstr = str.ToNewCString();
  printf("Attaching to WebShellWindow[%s]\n", cstr);
  nsCRT::free(cstr);
#endif

  nsCOMPtr<nsIWebShellContainer> webShellContainer;
  mWebShell->GetContainer(*getter_AddRefs(webShellContainer));
  if (!webShellContainer)
    return NS_ERROR_NOT_INITIALIZED;

  nsCOMPtr<nsIWebShellWindow> webShellWin = do_QueryInterface(webShellContainer, &rv);
  mWebShellWin = webShellWin;
    
  return rv;
}

nsIPresShell*
nsEditorShell::GetPresShellFor(nsIWebShell* aWebShell)
{
  nsIPresShell* shell = nsnull;
  if (nsnull != aWebShell) {
    nsIContentViewer* cv = nsnull;
    aWebShell->GetContentViewer(&cv);
    if (nsnull != cv) {
      nsIDocumentViewer* docv = nsnull;
      cv->QueryInterface(NS_GET_IID(nsIDocumentViewer), (void**) &docv);
      if (nsnull != docv) {
        nsIPresContext* cx;
        docv->GetPresContext(cx);
        if (nsnull != cx) {
          cx->GetShell(&shell);
          NS_RELEASE(cx);
        }
        NS_RELEASE(docv);
      }
      NS_RELEASE(cv);
    }
  }
  return shell;
}


// tell the appcore what type of editor to instantiate
// this must be called before the editor has been instantiated,
// otherwise, an error is returned.
NS_METHOD
nsEditorShell::SetEditorType(const PRUnichar *editorType)
{  
  if (mEditor)
    return NS_ERROR_ALREADY_INITIALIZED;
    
  nsAutoString  theType = editorType;
  theType.ToLowerCase();
  if (theType == "text" || theType == "html" || theType == "" || theType == "htmlmail")
  {
    mEditorTypeString = theType;
    return NS_OK;
  }
  else
  {
    NS_WARNING("Editor type not recognized");
    return NS_ERROR_UNEXPECTED;
  }
}


NS_METHOD
nsEditorShell::InstantiateEditor(nsIDOMDocument *aDoc, nsIPresShell *aPresShell)
{
  NS_PRECONDITION(aDoc && aPresShell, "null ptr");
  if (!aDoc || !aPresShell)
    return NS_ERROR_NULL_POINTER;

  if (mEditor)
    return NS_ERROR_ALREADY_INITIALIZED;

  nsresult err = NS_OK;
  
  nsCOMPtr<nsIEditor> editor;
  err = nsComponentManager::CreateInstance(kHTMLEditorCID, nsnull, NS_GET_IID(nsIEditor), getter_AddRefs(editor));
  if(!editor)
    err = NS_ERROR_OUT_OF_MEMORY;
    
  if (NS_SUCCEEDED(err))
  {
    if (mEditorTypeString == "text")
    {
      err = editor->Init(aDoc, aPresShell, nsIHTMLEditor::eEditorPlaintextMask);
      mEditorType = ePlainTextEditorType;
    }
    else if (mEditorTypeString == "html" || mEditorTypeString == "")  // empty string default to HTML editor
    {
      err = editor->Init(aDoc, aPresShell, 0);
      mEditorType = eHTMLTextEditorType;
    }
    else if (mEditorTypeString == "htmlmail")  //  HTML editor with special mail rules
    {
      err = editor->Init(aDoc, aPresShell, nsIHTMLEditor::eEditorMailMask);
      mEditorType = eHTMLTextEditorType;
    }
    else
    {
      err = NS_ERROR_INVALID_ARG;    // this is not an editor we know about
#if DEBUG
      nsAutoString  errorMsg = "Failed to init editor. Unknown editor type \"";
      errorMsg += mEditorTypeString;
      errorMsg += "\"\n";
      char  *errorMsgCString = errorMsg.ToNewCString();
         NS_WARNING(errorMsgCString);
         nsCRT::free(errorMsgCString);
#endif
    }

    if (NS_SUCCEEDED(err) && editor)
    {
      mEditor = do_QueryInterface(editor);    // this does the addref that is the owning reference
    }
  }
    
  return err;
}


NS_METHOD
nsEditorShell::DoEditorMode(nsIWebShell *aWebShell)
{
  nsresult  err = NS_OK;
  
  NS_PRECONDITION(aWebShell, "Need a webshell here");
  if (!aWebShell)
      return NS_ERROR_NULL_POINTER;

  nsCOMPtr<nsIDocument> Doc;
  err = GetDocument(aWebShell, getter_AddRefs(Doc));
  if (NS_SUCCEEDED(err) && Doc)
  {
    nsCOMPtr<nsIDOMDocument>  DOMDoc;
    if (NS_SUCCEEDED(Doc->QueryInterface(NS_GET_IID(nsIDOMDocument), (void**)getter_AddRefs(DOMDoc))))
    {
      nsCOMPtr<nsIPresShell> presShell = dont_AddRef(GetPresShellFor(aWebShell));
      if( presShell )
        err = InstantiateEditor(DOMDoc, presShell);
    }
  }
  return err;
}


NS_IMETHODIMP
nsEditorShell::UpdateInterfaceState(void)
{
  if (!mStateMaintainer)
    return NS_ERROR_NOT_INITIALIZED;

  return mStateMaintainer->ForceUpdate();
}  

// Deletion routines
nsresult
nsEditorShell::ScrollSelectionIntoView()
{
  nsCOMPtr<nsIEditor> editor = do_QueryInterface(mEditor);
  if (!editor) return NS_ERROR_NOT_INITIALIZED;
  nsCOMPtr<nsIPresShell> presShell;
  nsresult result = editor->GetPresShell(getter_AddRefs(presShell));
  if (NS_FAILED(result))
    return result;
  if (!presShell)
    return NS_ERROR_NULL_POINTER;

  return presShell->ScrollSelectionIntoView(SELECTION_NORMAL,
                                            SELECTION_FOCUS_REGION);
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
    nsAutoString attributeStr(attr);
    nsAutoString valueStr(value);
    result = editor->SetAttribute(element, attributeStr, valueStr); 
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
    nsAutoString attributeStr(attr);
    result = editor->RemoveAttribute(element, attributeStr);
  }
  return result;
}

// the name of the attribute here should be the contents of the appropriate
// tag, e.g. 'b' for bold, 'i' for italics.
NS_IMETHODIMP    
nsEditorShell::SetTextProperty(const PRUnichar *prop, const PRUnichar *attr, const PRUnichar *value)
{
  nsIAtom    *styleAtom = nsnull;
  nsresult  err = NS_NOINTERFACE;

  styleAtom = NS_NewAtom(prop);      /// XXX Hack alert! Look in nsIEditProperty.h for this

  if (! styleAtom)
    return NS_ERROR_OUT_OF_MEMORY;

  // addref it while we're using it
  NS_ADDREF(styleAtom);

  nsAutoString    attributeStr(attr);
  nsAutoString    valueStr(value);
  
  switch (mEditorType)
  {
    case ePlainTextEditorType:
        // should we allow this?
    case eHTMLTextEditorType:
      err = mEditor->SetInlineProperty(styleAtom, &attributeStr, &valueStr);
      break;
    default:
      err = NS_ERROR_NOT_IMPLEMENTED;
  }

  NS_RELEASE(styleAtom);
  return err;
}



NS_IMETHODIMP
nsEditorShell::RemoveOneProperty(const nsString& aProp, const nsString &aAttr)
{
  nsIAtom    *styleAtom = nsnull;
  nsresult  err = NS_NOINTERFACE;

  styleAtom = NS_NewAtom(aProp);      /// XXX Hack alert! Look in nsIEditProperty.h for this

  if (! styleAtom)
    return NS_ERROR_OUT_OF_MEMORY;

  // addref it while we're using it
  NS_ADDREF(styleAtom);

  switch (mEditorType)
  {
    case ePlainTextEditorType:
        // should we allow this?
    case eHTMLTextEditorType:
      err = mEditor->RemoveInlineProperty(styleAtom, &aAttr);
      break;
    default:
      err = NS_ERROR_NOT_IMPLEMENTED;
  }

  NS_RELEASE(styleAtom);
  return err;
}


// the name of the attribute here should be the contents of the appropriate
// tag, e.g. 'b' for bold, 'i' for italics.
NS_IMETHODIMP    
nsEditorShell::RemoveTextProperty(const PRUnichar *prop, const PRUnichar *attr)
{
  // OK, I'm really hacking now. This is just so that we can accept 'all' as input.
  // this logic should live elsewhere.
  //TODO: FIX THIS! IT IS WRONG! IT DOESN'T CONSIDER ALL THE 
  //  OTHER POSSIBLE TEXT ATTRIBUTES!
  static const char*  sAllKnownStyles[] = {
    "B",
    "I",
    "U",
    nsnull      // this null is important
  };
  
  nsAutoString  allStr(prop);
  nsAutoString  aAttr(attr);
  
  allStr.ToLowerCase();
  PRBool    doingAll = (allStr == "all");
  nsresult  err = NS_OK;

  if (doingAll)
  {
    nsAutoString  thisAttr;
    const char    **tagName = sAllKnownStyles;
  
    while (*tagName)
    {
      thisAttr.Truncate(0);
      thisAttr += (char *)(*tagName);
    
      err = RemoveOneProperty(thisAttr, aAttr);

      tagName ++;
    }
  
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
  nsIAtom    *styleAtom = nsnull;
  nsresult  err = NS_NOINTERFACE;

  styleAtom = NS_NewAtom(prop);      /// XXX Hack alert! Look in nsIEditProperty.h for this

  nsAutoString  aAttr(attr);
  nsAutoString  aValue(value);
  
  switch (mEditorType)
  {
    case ePlainTextEditorType:
        // should we allow this?
    case eHTMLTextEditorType:
      err = mEditor->GetInlineProperty(styleAtom, &aAttr, &aValue, *firstHas, *anyHas, *allHas);
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
  
  nsAutoString aColor(color);
  
  result = mEditor->SetBackgroundColor(aColor);

  return result;
}

NS_IMETHODIMP 
nsEditorShell::ApplyStyleSheet(const PRUnichar *url)
{
  nsresult result = NS_NOINTERFACE;
  
  nsAutoString  aURL(url);

  nsCOMPtr<nsIEditorStyleSheets> styleSheets = do_QueryInterface(mEditor);
  if (styleSheets)
    result = styleSheets->ApplyStyleSheet(aURL);

  return result;
}

// Note: This is not undoable action (on purpose!)
NS_IMETHODIMP 
nsEditorShell::SetDisplayMode(PRInt32 aDisplayMode)
{
  // We are already in EditMode
  if (aDisplayMode == eDisplayModeEdit && mEditModeStyleSheet)
    return NS_OK;

  if (!mContentAreaWebShell)
    return NS_ERROR_NOT_INITIALIZED;

  nsCOMPtr<nsIPresShell> presShell = dont_AddRef(GetPresShellFor(mContentAreaWebShell));
  if (!presShell)
    return NS_ERROR_NULL_POINTER;

  nsCOMPtr<nsIDocument> document;
  nsresult rv = presShell->GetDocument(getter_AddRefs(document));
  if (NS_SUCCEEDED(rv))
  {
    if(!document)
      return NS_ERROR_NULL_POINTER;

    nsCOMPtr<nsIStyleSet> styleSet;
    rv = presShell->GetStyleSet(getter_AddRefs(styleSet));
    if (NS_SUCCEEDED(rv))
    {
      if (!styleSet)
        return NS_ERROR_NULL_POINTER;

      nsCOMPtr<nsIStyleSheet> styleSheet;
      if (aDisplayMode == 0)
      {
        // Create and load the style sheet for editor content
        nsAutoString styleURL("chrome://editor/content/EditorContent.css");

        nsCOMPtr<nsIURI>uaURL;
        rv = NS_NewURI(getter_AddRefs(uaURL), styleURL);

        if (NS_SUCCEEDED(rv))
        {
          nsCOMPtr<nsIHTMLContentContainer> container = do_QueryInterface(document);
          if (!container)
            return NS_ERROR_NULL_POINTER;

          nsCOMPtr<nsICSSLoader> cssLoader;
          rv = container->GetCSSLoader(*getter_AddRefs(cssLoader));
          if (NS_SUCCEEDED(rv))
          {
            if (!cssLoader)
              return NS_ERROR_NULL_POINTER;

            nsCOMPtr<nsICSSStyleSheet>cssStyleSheet;
            PRBool complete;

            // We use null for the callback and data pointer because
            //  we MUST ONLY load synchronous local files (no @import)
            rv = cssLoader->LoadAgentSheet(uaURL, *getter_AddRefs(cssStyleSheet), complete, nsnull);
            if (NS_SUCCEEDED(rv))
            {
              // Synchronous loads should ALWAYS return completed
              if (!complete || !cssStyleSheet)
                return NS_ERROR_NULL_POINTER;

              styleSheet = do_QueryInterface(cssStyleSheet);
              if (!styleSheet)
                return NS_ERROR_NULL_POINTER;
            }
          }
        }
      }
      else if (aDisplayMode >= 1)
      {
        if (!mEditModeStyleSheet)
        {
          // The edit mode sheet was not previously loaded
          return NS_OK;
        }
        styleSheet = mEditModeStyleSheet;
      }
      
      if (NS_SUCCEEDED(rv))
      {
        switch (aDisplayMode)
        {
          case eDisplayModeEdit:
            styleSet->AppendOverrideStyleSheet(styleSheet);
            mEditModeStyleSheet = styleSheet;
            break;
          case eDisplayModeBrowserPreview:
            styleSet->RemoveOverrideStyleSheet(mEditModeStyleSheet);
            mEditModeStyleSheet = 0;
            break;
          // Add more modes here, e.g., browser mode with JavaScript turned on?
          default:
            break;
        }
        // This notifies document observers to rebuild all frames
        // (this doesn't affect style sheet because it is not a doc sheet)
        document->SetStyleSheetDisabledState(styleSheet, PR_FALSE);
      }
    }
  }
  return rv;
}

NS_IMETHODIMP 
nsEditorShell::SetBodyAttribute(const PRUnichar *attr, const PRUnichar *value)
{
  nsresult result = NS_NOINTERFACE;
  
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
  if (!mContentAreaWebShell)
    return NS_ERROR_NOT_INITIALIZED;
      
  return mContentAreaWebShell->LoadURL(url);
}


NS_IMETHODIMP    
nsEditorShell::RegisterDocumentStateListener(nsIDocumentStateListener *docListener)
{
  nsresult rv = NS_OK;
  
  if (!docListener)
    return NS_ERROR_NULL_POINTER;
  
  // if we have an editor already, just pass this baby through.
  if (mEditor)
  {
    nsCOMPtr<nsIEditor>  editor = do_QueryInterface(mEditor, &rv);
    if (NS_FAILED(rv))
      return rv;
  
    return editor->AddDocumentStateListener(docListener);
  }
  
  // otherwise, keep it until we create an editor.
  if (!mDocStateListeners)
  {
    rv = NS_NewISupportsArray(getter_AddRefs(mDocStateListeners));
    if (NS_FAILED(rv)) return rv;
  }
  nsCOMPtr<nsISupports> iSupports = do_QueryInterface(docListener, &rv);
  if (NS_FAILED(rv)) return rv;

  // note that this return value is really a PRBool, so be sure to use
  // NS_SUCCEEDED or NS_FAILED to check it.
  return mDocStateListeners->AppendElement(iSupports);
}

NS_IMETHODIMP    
nsEditorShell::UnregisterDocumentStateListener(nsIDocumentStateListener *docListener)
{
  if (!docListener)
    return NS_ERROR_NULL_POINTER;

  nsresult rv = NS_OK;
  
  // if we have an editor already, just pass this baby through.
  if (mEditor)
  {
    nsCOMPtr<nsIEditor>  editor = do_QueryInterface(mEditor, &rv);
    if (NS_FAILED(rv))
      return rv;
  
    return editor->RemoveDocumentStateListener(docListener);
  }

  // otherwise, see if it exists in our list
  if (!mDocStateListeners)
    return (nsresult)PR_FALSE;      // yeah, this sucks, but I'm emulating the behaviour of
                                    // nsISupportsArray::RemoveElement()

  nsCOMPtr<nsISupports> iSupports = do_QueryInterface(docListener, &rv);
  if (NS_FAILED(rv)) return rv;

  // note that this return value is really a PRBool, so be sure to use
  // NS_SUCCEEDED or NS_FAILED to check it.
  return mDocStateListeners->RemoveElement(iSupports);
}

// called after making an editor. Transfer the nsIDOcumentStateListeners
// that we have been stashing in mDocStateListeners to the editor.
NS_IMETHODIMP    
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
  while (NS_SUCCEEDED(mDocStateListeners->Count(&numListeners)) && numListeners > 0)
  {
    nsCOMPtr<nsISupports> iSupports = getter_AddRefs(mDocStateListeners->ElementAt(0));
    nsCOMPtr<nsIDocumentStateListener> docStateListener = do_QueryInterface(iSupports);
    if (docStateListener)
    {
      // this checks for duplicates
      rv = editor->AddDocumentStateListener(docStateListener);
    }
    
    mDocStateListeners->RemoveElementAt(0);
  }
  
  // free the array
  mDocStateListeners = 0;
  return NS_OK;
}

NS_IMETHODIMP
nsEditorShell::CheckOpenWindowForURLMatch(const PRUnichar* inFileURL, nsIDOMWindow* inCheckWindow, PRBool *aDidFind)
{
  if (!inCheckWindow) return NS_ERROR_NULL_POINTER;
  *aDidFind = PR_FALSE;
  
  // get an nsFileSpec from the URL
  nsFileURL    fileURL(inFileURL);
  nsFileSpec   fileSpec(fileURL);
  
  nsCOMPtr<nsIDOMWindow> contentWindow;
  inCheckWindow->GetContent(getter_AddRefs(contentWindow));
  if (contentWindow)
  {
    // get the content doc
    nsCOMPtr<nsIDOMDocument> contentDoc;          
    contentWindow->GetDocument(getter_AddRefs(contentDoc));
    if (contentDoc)
    {
      nsCOMPtr<nsIDiskDocument> diskDoc(do_QueryInterface(contentDoc));
      if (diskDoc)
      {
        nsFileSpec docFileSpec;
        if (NS_SUCCEEDED(diskDoc->GetFileSpec(docFileSpec)))
        {
          // is this the filespec we are looking for?
          if (docFileSpec == fileSpec)
          {
            *aDidFind = PR_TRUE;
          }
        }
      }
    }
  }

  return NS_OK;
}

NS_IMETHODIMP    
nsEditorShell::CheckAndSaveDocument(const PRUnichar *reasonToSave, PRBool *_retval)
{
  *_retval = PR_FALSE;

  nsAutoString ReasonToSave(reasonToSave);
  nsCOMPtr<nsIDOMDocument> theDoc;
  nsresult rv = GetEditorDocument(getter_AddRefs(theDoc));
  if (NS_SUCCEEDED(rv) && theDoc)
  {
    nsCOMPtr<nsIDiskDocument> diskDoc = do_QueryInterface(theDoc);
    if (diskDoc)
    {
      PRInt32  modCount = 0;
      diskDoc->GetModCount(&modCount);

      // Return true unless user cancels an action
      *_retval = PR_TRUE;

      if (modCount > 0)
      {
        // Ask user if they want to save current changes
        nsAutoString tmp1 = GetString("Save");
        nsAutoString tmp2 = GetString("DontSave");
        nsAutoString title;
        GetDocumentTitleString(title);
        nsAutoString saveMsg = GetString("SaveFilePrompt")+" "+"\""+title+"\"";
        if (ReasonToSave.Length() > 0) 
        {
          saveMsg += " ";
          saveMsg += ReasonToSave;
          saveMsg += GetString("QuestionMark");
        } else {
          saveMsg += GetString("QuestionMark");
        }

        EConfirmResult result = ConfirmWithCancel(GetString("SaveDocument"), saveMsg,
                                                  &tmp1, &tmp2);
        if (result == eCancel)
        {
          *_retval = PR_FALSE;
        } else if (result == eYes)
        {
          // Either save to existing file or prompt for name (as for SaveAs)
          // We don't continue if we failed to save file (_retval is set to FALSE)
          rv = SaveDocument(PR_FALSE, PR_FALSE, _retval);
        }
      }
    }
  }
  return rv;
}

NS_IMETHODIMP 
nsEditorShell::SaveDocument(PRBool saveAs, PRBool saveCopy, PRBool *_retval)
{
  nsresult  res = NS_NOINTERFACE;
  *_retval = PR_FALSE;

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
        nsFileSpec    docFileSpec;
        PRBool noFileSpec = (diskDoc->GetFileSpec(docFileSpec) == NS_ERROR_NOT_INITIALIZED);
        PRBool mustShowFileDialog = saveAs || noFileSpec;
        PRBool replacing = !saveAs;
  
        if (mustShowFileDialog)
        {
          PRBool bUpdateWindowTitle = PR_TRUE;

          // Check if the document has a title and prompt for one if missing
          nsCOMPtr<nsIDOMHTMLDocument> HTMLDoc = do_QueryInterface(doc);
          if (HTMLDoc)
          {
            nsString title;
            res = HTMLDoc->GetTitle(title);

            // Prompt for title ONLY if it's empty
            if (NS_SUCCEEDED(res) && title.Length() == 0)
            {
              // Use a "prompt" common dialog to get title string from user
              NS_WITH_SERVICE(nsICommonDialogs, dialog, kCommonDialogsCID, &res); 
              if (NS_SUCCEEDED(res)) 
              { 
                PRUnichar *titleUnicode;
                nsAutoString caption = GetString("DocumentTitle");
                nsAutoString msg = GetString("NeedDocTitle"); 
                PRBool retVal = PR_FALSE;
                res = dialog->Prompt(mContentWindow, caption.GetUnicode(), msg.GetUnicode(),
                                     title.GetUnicode(), &titleUnicode, &retVal); 
                
                if( retVal == PR_FALSE)
                {
                  // This indicates Cancel was used -- don't continue saving
                  *_retval = PR_FALSE;
                  return NS_OK;
                }
                //This will call UpdateWindowTitle
                SetDocumentTitle(titleUnicode);
                title = titleUnicode;
                bUpdateWindowTitle = PR_FALSE;
                nsCRT::free(titleUnicode);
              }
            }

            nsCOMPtr<nsIFileWidget>  fileWidget;
            res = nsComponentManager::CreateInstance(kCFileWidgetCID, nsnull, NS_GET_IID(nsIFileWidget), getter_AddRefs(fileWidget));
            if (NS_SUCCEEDED(res) && fileWidget)
            {
              nsAutoString  promptString = GetString("SaveDocumentAs");

              nsString* titles = nsnull;
              nsString* filters = nsnull;
              nsString* nextTitle;
              nsString* nextFilter;
              nsAutoString HTMLFiles;
              nsAutoString TextFiles;
              nsAutoString fileName;
              nsFileSpec parentPath;

              titles = new nsString[3];
              if (!titles)
              {
                res = NS_ERROR_OUT_OF_MEMORY;
                goto SkipFilters;
              }
              filters = new nsString[3];
              if (!filters)
              {
                res = NS_ERROR_OUT_OF_MEMORY;
                goto SkipFilters;
              }
              nextTitle = titles;
              nextFilter = filters;
              // The names of the file types are localizable
              HTMLFiles = GetString("HTMLFiles");
              TextFiles = GetString("TextFiles");
              if (HTMLFiles.Length() == 0 || TextFiles.Length() == 0)
                goto SkipFilters;
                
              *nextTitle++ = HTMLFiles;
              *nextFilter++ = "*.htm; *.html; *.shtml";
              *nextTitle++ = TextFiles;
              *nextFilter++ = "*.txt";
              *nextTitle++ = GetString("AllFiles");
              *nextFilter++ = "*.*";
              fileWidget->SetFilterList(3, titles, filters);
              
              if (noFileSpec)
              {
                // Use page title as suggested name for new document
                if (title.Length() > 0)
                {
                  //Replace "bad" filename characteres with "_"
                  PRUnichar space = (PRUnichar)' ';
                  PRUnichar dot = (PRUnichar)'.';
                  PRUnichar bslash = (PRUnichar)'\\';
                  PRUnichar fslash = (PRUnichar)'/';
                  PRUnichar at = (PRUnichar)'@';
                  PRUnichar colon = (PRUnichar)':';
                  PRUnichar underscore = (PRUnichar)'_';
                  title = title.ReplaceChar(space, underscore);
                  title = title.ReplaceChar(dot, underscore);
                  title = title.ReplaceChar(bslash, underscore);
                  title = title.ReplaceChar(fslash, underscore);
                  title = title.ReplaceChar(at, underscore);
                  title = title.ReplaceChar(colon, underscore);
                  fileName = title + nsString(".html");
                }
              } else {
                char *leafName = docFileSpec.GetLeafName();
                if (leafName)
                {
                  fileName = leafName;
                  nsCRT::free(leafName);
                }
                docFileSpec.GetParent(parentPath);

                // TODO: CHANGE TO THE DIRECTORY OF THE PARENT PATH?
              }
              if (fileName.Length() > 0)
                fileWidget->SetDefaultString(fileName);
SkipFilters:
              nsFileDlgResults dialogResult;
              // 1ST PARAM SHOULD BE nsIDOMWindow*, not nsIWidget*
              dialogResult = fileWidget->PutFile(nsnull, promptString, docFileSpec);
              delete [] titles;
              delete [] filters;

              if (dialogResult == nsFileDlgResults_Cancel)
              {
                // Note that *_retval = PR_FALSE at this point
                return NS_OK;
              }
              replacing = (dialogResult == nsFileDlgResults_Replace);
            }
            else
            {
               NS_ASSERTION(0, "Failed to get file widget");
              return res;
            }
          }
          // Set the new URL for the webshell
          if (mContentAreaWebShell)
          {
            nsFileURL fileURL(docFileSpec);
            nsAutoString fileURLString(fileURL.GetURLString());
            PRUnichar *fileURLUnicode = fileURLString.ToNewUnicode();
            if (fileURLUnicode)
            {
              mContentAreaWebShell->SetURL(fileURLUnicode);
        			Recycle(fileURLUnicode);
            }
          }         
          
          // Update window caption in case title has changed.
          if (bUpdateWindowTitle)
            UpdateWindowTitle();
        } // mustShowFileDialog

        // TODO: Get the file type (from the extension?) the user set for the file
        // How do we do this in an XP way??? 
        // For now, just save as HTML type
        res = editor->SaveFile(&docFileSpec, replacing, saveCopy, nsIDiskDocument::eSaveFileHTML);
        if (NS_FAILED(res))
        {
          Alert(GetString("SaveDocument"), GetString("SaveFileFailed"));
        } else {
          // File was saved successfully
          *_retval = PR_TRUE;
        }
      }
      break;
    }
    default:
      res = NS_ERROR_NOT_IMPLEMENTED;
  }
  return res;
}

NS_IMETHODIMP    
nsEditorShell::CloseWindow( PRBool *_retval )
{
  nsresult rv = NS_OK;
  
  rv = CheckAndSaveDocument(GetString("BeforeClosing").GetUnicode(),_retval);
 
  // Don't close the window if there was an error saving file or 
  //   user canceled an action along the way
  if (NS_SUCCEEDED(rv) && *_retval)
    mWebShellWin->Close();

  return rv;
}

NS_IMETHODIMP    
nsEditorShell::Print()
{ 
  if (!mContentAreaWebShell)
    return NS_ERROR_NOT_INITIALIZED;

  nsCOMPtr<nsIContentViewer> viewer;    
  mContentAreaWebShell->GetContentViewer(getter_AddRefs(viewer));    
  if (nsnull != viewer) 
  {
    nsCOMPtr<nsIContentViewerFile> viewerFile = do_QueryInterface(viewer);
    if (viewerFile) {
      NS_ENSURE_SUCCESS(viewerFile->Print(), NS_ERROR_FAILURE);
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsEditorShell::GetLocalFileURL(nsIDOMWindow *parent, const PRUnichar *filterType, PRUnichar **_retval)
{
  nsAutoString FilterType(filterType);
  PRBool htmlFilter = FilterType.EqualsIgnoreCase("html");
  PRBool imgFilter = FilterType.EqualsIgnoreCase("img");

  *_retval = nsnull;
  
  // TODO: DON'T ACCEPT NULL PARENT AFTER WIDGET IS FIXED
  if (/*parent||*/ !(htmlFilter || imgFilter))
    return NS_ERROR_NOT_INITIALIZED;


  nsCOMPtr<nsIFileWidget>  fileWidget;
  nsAutoString HTMLTitle = GetString("OpenHTMLFile");

  // An empty string should just result in "Open" for the dialog
  nsAutoString title;
  if (htmlFilter)
  {
    title = HTMLTitle;
  } else {
    nsAutoString ImageTitle = GetString("SelectImageFile");

    if (ImageTitle.Length() > 0 && imgFilter)
      title = ImageTitle;
  }

  nsFileSpec fileSpec;
  // TODO: GET THE DEFAULT DIRECTORY FOR DIFFERENT TYPES FROM PREFERENCES
  nsFileSpec aDisplayDirectory;

  nsresult res = nsComponentManager::CreateInstance(kCFileWidgetCID,
                                                    nsnull,
                                                    NS_GET_IID(nsIFileWidget),
                                                    (void**)&fileWidget);
  if (NS_SUCCEEDED(res))
  {
    nsFileDlgResults dialogResult;
    nsString* titles = nsnull;
    nsString* filters = nsnull;
    nsString* nextTitle;
    nsString* nextFilter;

    if (htmlFilter)
    {
      titles = new nsString[3];
      filters = new nsString[3];
      if (!titles || ! filters)
        return NS_ERROR_OUT_OF_MEMORY;
      nextTitle = titles;
      nextFilter = filters;
      *nextTitle++ = GetString("HTMLFiles");
      *nextTitle++ = GetString("TextFiles");
      *nextFilter++ = "*.htm; *.html; *.shtml";
      *nextFilter++ = "*.txt";
      fileWidget->SetFilterList(3, titles, filters);
    } else {
      titles = new nsString[2];
      filters = new nsString[2];
      if (!titles || ! filters)
        return NS_ERROR_OUT_OF_MEMORY;
      nextTitle = titles;
      nextFilter = filters;
      *nextTitle++ = GetString("IMGFiles");
      *nextFilter++ = "*.gif; *.jpg; *.jpeg; *.png", "*.*";
      fileWidget->SetFilterList(2, titles, filters);
    }
    *nextTitle++ = GetString("AllFiles");
    *nextFilter++ = "*.*";
    // First param should be Parent window, but type is nsIWidget*
    // Bug is filed to change this to a more suitable window type
    dialogResult = fileWidget->GetFile(/*parent*/ nsnull, title, fileSpec);
    delete [] titles;
    delete [] filters;

    // Do this after we get this from preferences
    //fileWidget->SetDisplayDirectory(aDisplayDirectory);
    
    if (dialogResult != nsFileDlgResults_Cancel) 
    {
      // Get the platform-specific format
      // Convert it to the string version of the URL format
      // NOTE: THIS CRASHES IF fileSpec is empty
      nsFileURL url(fileSpec);
      nsAutoString  returnVal = url.GetURLString();
      *_retval = returnVal.ToNewUnicode();
    }
    // TODO: SAVE THIS TO THE PREFS?
    fileWidget->GetDisplayDirectory(aDisplayDirectory);
  }

  return res;
}

NS_IMETHODIMP
nsEditorShell::UpdateWindowTitle()
{
  nsresult res = NS_ERROR_NOT_INITIALIZED;

  if (!mContentAreaWebShell || !mEditor)
    return res;

  nsCOMPtr<nsIEditor> editor = do_QueryInterface(mEditor);
  if (!editor)
    return res;

  nsAutoString windowCaption;
  res = GetDocumentTitleString(windowCaption);

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
        nsFileSpec docFileSpec;
        if (NS_SUCCEEDED(diskDoc->GetFileSpec(docFileSpec)))
        {
          char *name = docFileSpec.GetLeafName();
          if (name)
          {
            windowCaption += " [";
            windowCaption += name;
            windowCaption += "]";
            nsCRT::free(name);
          }
        }
      }
    }
    nsCOMPtr<nsIBaseWindow> contentAreaAsWin(do_QueryInterface(mContentAreaWebShell));
    NS_ASSERTION(contentAreaAsWin, "This object should implement nsIBaseWindow");
    res = contentAreaAsWin->SetTitle(windowCaption.GetUnicode());
  }
  return res;
}

NS_IMETHODIMP
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

    // If title is empty, use "untitled"
    if (title.Length() == 0)
      title = GetString("untitled");
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
    nsAutoString empty("");
    *title = empty.ToNewUnicode();
    res = NS_OK;
  }
  return res;
}

NS_IMETHODIMP
nsEditorShell::SetDocumentTitle(const PRUnichar *title)
{
  nsresult res = NS_ERROR_NOT_INITIALIZED;

  if (!mEditor && !mContentAreaWebShell)
    return res;

  // This should only be allowed for HTML documents
  if (mEditorType != eHTMLTextEditorType)
    return NS_ERROR_NOT_IMPLEMENTED;

  nsCOMPtr<nsIEditor> editor = do_QueryInterface(mEditor);
  if (!editor)
    return NS_ERROR_FAILURE;

  nsAutoString titleStr(title);
  nsCOMPtr<nsIDOMDocument>  domDoc;
  res = editor->GetDocument(getter_AddRefs(domDoc));
  
  if (domDoc)
  {
    // Get existing document title node
    nsCOMPtr<nsIDOMHTMLDocument> HTMLDoc = do_QueryInterface(domDoc);
    if (HTMLDoc)
    {
      // This sets the window title, and saves the title as a member varialble,
      //  but does NOT insert the <title> node.
      HTMLDoc->SetTitle(titleStr);
      
      nsCOMPtr<nsIDOMNodeList> titleList;
      nsCOMPtr<nsIDOMNode>titleNode;
      nsCOMPtr<nsIDOMNode>headNode;
      nsCOMPtr<nsIDOMNode> resultNode;
      res = domDoc->GetElementsByTagName("title", getter_AddRefs(titleList));
      if (NS_SUCCEEDED(res))
      {
        if(titleList)
        {
          /* I'm tempted to just get the 1st title element in the list
             (there should always be just 1). But in case there's > 1,
             I assume the last one will be used, so this finds that one.
          */
          PRUint32 len = 0;
          titleList->GetLength(&len);
          if (len >= 1)
            titleList->Item(len-1, getter_AddRefs(titleNode));

          if (titleNode)
          {
            //Delete existing children
            nsCOMPtr<nsIDOMNodeList> children;
            res = titleNode->GetChildNodes(getter_AddRefs(children));
            if(NS_SUCCEEDED(res) && children)
            {
              PRUint32 count = 0;
              children->GetLength(&count);
              for( PRUint32 i = 0; i < count; i++)
              {
                nsCOMPtr<nsIDOMNode> child;
                res = children->Item(i,getter_AddRefs(child));
                if(NS_SUCCEEDED(res) && child)
                  titleNode->RemoveChild(child,getter_AddRefs(resultNode));
              }
            }
          }
        }
      }
      // Get the <HEAD> node, create a <TITLE> and insert it under the HEAD
      nsCOMPtr<nsIDOMNodeList> headList;
      res = domDoc->GetElementsByTagName("head",getter_AddRefs(headList));
      if (NS_SUCCEEDED(res) && headList) 
      {
        headList->Item(0, getter_AddRefs(headNode));
        if (headNode) 
        {
          PRBool newTitleNode = PR_FALSE;
          if (!titleNode)
          {
            // Didn't find one above: Create a new one
            nsCOMPtr<nsIDOMElement>titleElement;
            res = domDoc->CreateElement("title",getter_AddRefs(titleElement));
            if (NS_SUCCEEDED(res) && titleElement)
            {
              titleNode = do_QueryInterface(titleElement);
              newTitleNode = PR_TRUE;
            }
            // Note: There should ALWAYS be a <title> in any HTML document,
            //       so we will insert the node and not make it undoable
            res = headNode->AppendChild(titleNode, getter_AddRefs(resultNode));
            if (NS_FAILED(res))
              return NS_ERROR_FAILURE;
          }
          // Append a text node under the TITLE
          //  only if the title text isn't empty
          if (titleNode && titleStr.Length() > 0)
          {
            nsCOMPtr<nsIDOMText> textNode;
            res = domDoc->CreateTextNode(titleStr, getter_AddRefs(textNode));
            if (NS_SUCCEEDED(res) && textNode)
            {
              // Go through the editor API so action is undoable
              res = editor->InsertNode(textNode, titleNode, 0);
              // This is the non-undoable code:
              //res = titleNode->AppendChild(textNode,getter_AddRefs(resultNode));
            }
          }
        }
      }
    }
  }
  UpdateWindowTitle();

  return res;
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
  nsresult  rv = NS_NOINTERFACE;
  
  switch (mEditorType)
  {
    case ePlainTextEditorType:
    case eHTMLTextEditorType:
      {
        nsCOMPtr<nsIEditor>  editor = do_QueryInterface(mEditor);
        if (editor)
          rv = editor->NodeIsBlock(node, *_retval);
      }
      break;

    default:
      rv = NS_ERROR_NOT_IMPLEMENTED;
  }

  return rv;
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
nsEditorShell::Paste()
{  
  nsresult  err = NS_NOINTERFACE;
  
  switch (mEditorType)
  {
    case ePlainTextEditorType:
    case eHTMLTextEditorType:
      {
        nsCOMPtr<nsIEditor>  editor = do_QueryInterface(mEditor);
        if (editor)
          err = editor->Paste();
      }
      break;

    default:
      err = NS_ERROR_NOT_IMPLEMENTED;
  }

  return err;
}

NS_IMETHODIMP    
nsEditorShell::PasteAsQuotation()
{  
  nsresult  err = NS_NOINTERFACE;
  
  switch (mEditorType)
  {
    case ePlainTextEditorType:
    case eHTMLTextEditorType:
      {
        nsCOMPtr<nsIEditorMailSupport>  mailEditor = do_QueryInterface(mEditor);
        if (mailEditor)
          err = mailEditor->PasteAsQuotation();
      }
      break;

    default:
      err = NS_ERROR_NOT_IMPLEMENTED;
  }

  return err;
}

NS_IMETHODIMP    
nsEditorShell::PasteAsCitedQuotation(const PRUnichar *cite)
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
          err = mailEditor->PasteAsCitedQuotation(aCiteString);
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
                                               aCharset, aNodeInserted);
      break;

    default:
      err = NS_ERROR_NOT_IMPLEMENTED;
  }

  return err;
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

/* This routine should only be called when playing back a log */
NS_IMETHODIMP
nsEditorShell::TypedText(const PRUnichar *aTextToInsert, PRInt32 aAction)
{
  nsresult  err = NS_NOINTERFACE;
  
  nsAutoString textToInsert(aTextToInsert);
  
  switch (mEditorType)
  {
    case ePlainTextEditorType:
    case eHTMLTextEditorType:
      {
        nsCOMPtr<nsIHTMLEditor>  htmlEditor = do_QueryInterface(mEditor);
        if (htmlEditor)
          err = htmlEditor->TypedText(textToInsert, aAction);
      }
      break;

    default:
      err = NS_ERROR_NOT_IMPLEMENTED;
  }

  return err;
}


NS_IMETHODIMP
nsEditorShell::InsertText(const PRUnichar *textToInsert)
{
  nsresult  err = NS_NOINTERFACE;
  
  nsAutoString aTextToInsert(textToInsert);
  
  switch (mEditorType)
  {
    case ePlainTextEditorType:
    case eHTMLTextEditorType:
      {
        nsCOMPtr<nsIHTMLEditor>  htmlEditor = do_QueryInterface(mEditor);
        if (htmlEditor)
          err = htmlEditor->InsertText(aTextToInsert);
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
nsEditorShell::InsertBreak()
{
  nsresult  err = NS_NOINTERFACE;
  
  if (mEditor)
    err = mEditor->InsertBreak();
  
  return err;
}

// Both Find and FindNext call through here.
NS_IMETHODIMP
nsEditorShell::DoFind(PRBool aFindNext)
{
  if (!mContentAreaWebShell)
    return NS_ERROR_NOT_INITIALIZED;

  PRBool foundIt = PR_FALSE;
  
  // Get find component.
  nsresult rv;
  NS_WITH_SERVICE(nsIFindComponent, findComponent, NS_IFINDCOMPONENT_PROGID, &rv);
  NS_ASSERTION(((NS_SUCCEEDED(rv)) && findComponent), "GetService failed for find component.");
  if (NS_FAILED(rv)) { return rv; }

  // make the search context if we need to
  if (!mSearchContext)
  {
    rv = findComponent->CreateContext( mContentAreaWebShell, nsnull, getter_AddRefs(mSearchContext));
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

/* Get localized strings for UI from the Editor's string bundle */
// Use this version from JavaScript:
NS_IMETHODIMP
nsEditorShell::GetString(const PRUnichar *name, PRUnichar **_retval)
{
  if (!name || !_retval)
    return NS_ERROR_NULL_POINTER;

  // Don't fail, just return an empty string    
  nsAutoString empty("");

  if (mStringBundle)
  {
    if (NS_FAILED(mStringBundle->GetStringFromName(name, _retval)))
      *_retval = empty.ToNewUnicode();

    return NS_OK;
  } else {
    *_retval = empty.ToNewUnicode();
    return NS_ERROR_NOT_INITIALIZED;
  }
}

static nsAutoString *ptmpString = 0;

// Use this version within the shell:
nsString
nsEditorShell::GetString(const nsString& name)
{
  // Initialize upon first use to avoid static constructor
  if (!ptmpString)
    ptmpString = new nsAutoString();

  // Don't fail, just return an empty string    
  *ptmpString = "";
  if (mStringBundle && (name != ""))
  {
    const PRUnichar *ptrtmp = name.GetUnicode();
    PRUnichar *ptrv = nsnull;
    nsresult res = mStringBundle->GetStringFromName(ptrtmp, &ptrv);
    if (NS_SUCCEEDED(res))
      *ptmpString = ptrv;
  }
  return *ptmpString;
}

// Utility to bring up a Yes/No/Cancel dialog.
nsEditorShell::EConfirmResult
nsEditorShell::ConfirmWithCancel(const nsString& aTitle, const nsString& aQuestion, 
                                 const nsString *aYesString, const nsString *aNoString)
{
  nsEditorShell::EConfirmResult result = nsEditorShell::eCancel;
  
  nsIDialogParamBlock* block = NULL; 
  nsresult rv = nsComponentManager::CreateInstance(kDialogParamBlockCID, 0,
                                          NS_GET_IID(nsIDialogParamBlock), 
                                          (void**)&block ); 
  if ( NS_SUCCEEDED(rv) )
  { 
    // Stuff in Parameters 
    block->SetInt( nsICommonDialogs::eNumberButtons,3 ); 
    block->SetString( nsICommonDialogs::eMsg, aQuestion.GetUnicode()); 
    nsAutoString url( "chrome://global/skin/question-icon.gif"  ); 
    block->SetString( nsICommonDialogs::eIconURL, url.GetUnicode()); 

    nsAutoString yes = aYesString ? *aYesString : GetString("Yes");
    nsAutoString no = aNoString ? *aNoString : GetString("No");
    nsAutoString cancel = GetString("Cancel");
    //Note: "button0" is always Ok or Yes action, "button1" is Cancel
    block->SetString( nsICommonDialogs::eButton0Text, yes.GetUnicode() ); 
    block->SetString( nsICommonDialogs::eButton1Text, cancel.GetUnicode() ); 
    block->SetString( nsICommonDialogs::eButton2Text, no.GetUnicode() ); 

    NS_WITH_SERVICE(nsICommonDialogs, dialog, kCommonDialogsCID, &rv); 
    if ( NS_SUCCEEDED( rv ) ) 
    { 
      PRInt32 buttonPressed = 0; 
      rv = dialog->DoDialog( mContentWindow, block, "chrome://global/content/commonDialog.xul" ); 
      block->GetInt( nsICommonDialogs::eButtonPressed, &buttonPressed ); 
      // NOTE: If order of buttons changes in nsICommonDialogs,
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

  NS_WITH_SERVICE(nsICommonDialogs, dialog, kCommonDialogsCID, &rv); 
  if (NS_SUCCEEDED(rv) && dialog)
  {
    rv = dialog->Confirm(mContentWindow, aTitle.GetUnicode(), aQuestion.GetUnicode(), &result);
  }
  return result;
}

void    
nsEditorShell::Alert(const nsString& aTitle, const nsString& aMsg)
{
  nsresult rv;
  NS_WITH_SERVICE(nsICommonDialogs, dialog, kCommonDialogsCID, &rv); 
  if (NS_SUCCEEDED(rv) && dialog)
  {
    rv = dialog->Alert(mContentWindow, aTitle.GetUnicode(), aMsg.GetUnicode());
  }
}

NS_IMETHODIMP
nsEditorShell::GetDocumentCharacterSet(PRUnichar** characterSet)
{
  nsCOMPtr<nsIEditor> editor = do_QueryInterface(mEditor);

  if (editor)
    return editor->GetDocumentCharacterSet(characterSet);

  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsEditorShell::SetDocumentCharacterSet(const PRUnichar* characterSet)
{
  nsCOMPtr<nsIEditor> editor = do_QueryInterface(mEditor);

  if (editor)
    return editor->SetDocumentCharacterSet(characterSet);

  return NS_ERROR_FAILURE;

}

NS_IMETHODIMP
nsEditorShell::GetContentsAs(const PRUnichar *format, PRUint32 flags,
                             PRUnichar **contentsAs)
{
  nsresult  err = NS_NOINTERFACE;

  nsAutoString aFormat (format);
  nsAutoString aContentsAs;

  nsCOMPtr<nsIEditor> editor = do_QueryInterface(mEditor);
  if (editor)
    err = editor->OutputToString(aContentsAs, aFormat, flags);

  *contentsAs = aContentsAs.ToNewUnicode();
  
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
        nsCOMPtr<nsIEditorMailSupport>  mailEditor = do_QueryInterface(mEditor);
        if (mailEditor)
        {
          PRInt32 wc;
          err = mailEditor->GetBodyWrapWidth(&wc);
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
          nsCOMPtr<nsIEditorMailSupport>  mailEditor = do_QueryInterface(mEditor);
          if (mailEditor)
            err = mailEditor->SetBodyWrapWidth(mWrapColumn);
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
nsEditorShell::GetEditorSelection(nsIDOMSelection** aEditorSelection)
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

  nsCOMPtr<nsIPresShell> presShell;
  nsresult rv = editor->GetPresShell(getter_AddRefs(presShell));
  if (NS_FAILED(rv))
    return rv;

  nsCOMPtr<nsISelectionController> selCont (do_QueryInterface(presShell));
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
nsEditorShell::GetDocumentLength(PRInt32 *aDocumentLength)
{
  nsCOMPtr<nsIHTMLEditor> editor = do_QueryInterface(mEditor);
  if (editor)
    return editor->GetDocumentLength(aDocumentLength);

  return NS_NOINTERFACE;
}


NS_IMETHODIMP
nsEditorShell::MakeOrChangeList(const PRUnichar *listType)
{
  nsresult err = NS_NOINTERFACE;

  nsAutoString aListType(listType);
  
  switch (mEditorType)
  {
    case eHTMLTextEditorType:
      if (aListType == "")
      {
        err = mEditor->RemoveList("ol");
        if(NS_SUCCEEDED(err))
        {
          err = mEditor->RemoveList("ul");
          if(NS_SUCCEEDED(err))
            err = mEditor->RemoveList("dl");
        }
      }
      else
        err = mEditor->MakeOrChangeList(aListType);
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

  nsAutoString aListType(listType);
  
  switch (mEditorType)
  {
    case eHTMLTextEditorType:
      if (aListType == "")
      {
        err = mEditor->RemoveList("ol");
        if(NS_SUCCEEDED(err))
        {
          err = mEditor->RemoveList("ul");
          if(NS_SUCCEEDED(err))
            err = mEditor->RemoveList("dl");
        }
      }
      else
        err = mEditor->RemoveList(aListType);
      break;

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

  nsresult  result = NS_NOINTERFACE;
  nsAutoString tagName(aInTagName);
  
  switch (mEditorType)
  {
    case eHTMLTextEditorType:
      result = mEditor->GetSelectedElement(tagName, aOutElement);
      break;

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

  nsresult  result = NS_NOINTERFACE;
  nsAutoString tagName(aInTagName);
  
  switch (mEditorType)
  {
    case eHTMLTextEditorType:
      result = mEditor->GetElementOrParentByTagName(tagName, node, aOutElement);
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
nsEditorShell::InsertElement(nsIDOMElement *element, nsIDOMElement *parent, PRInt32 position)
{
  if (!element || !parent)
    return NS_ERROR_NULL_POINTER;

  nsresult  result = NS_NOINTERFACE;
  nsCOMPtr<nsIEditor> editor = do_QueryInterface(mEditor);
  if (editor) {
    // The nsIEditor::InsertNode() wants nodes as params,
    //   but it actually requires that they are elements!
    nsCOMPtr<nsIDOMNode> node = do_QueryInterface(element);
    nsCOMPtr<nsIDOMNode> parentNode = do_QueryInterface(parent);
    result = editor->InsertNode(node, parentNode, position);
  }
  return result;
}

NS_IMETHODIMP
nsEditorShell::InsertElementAtSelection(nsIDOMElement *element, PRBool deleteSelection)
{
  if (!element)
    return NS_ERROR_NULL_POINTER;

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
          result = tableEditor->InsertTableCell(aNumber, bAfter);
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
          result = tableEditor->DeleteTable();
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
          result = tableEditor->DeleteTableCell(aNumber);
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
          result = tableEditor->DeleteTableCellContents();
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
          result = tableEditor->DeleteTableRow(aNumber);
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
          result = tableEditor->DeleteTableColumn(aNumber);
      }
      break;
    default:
      result = NS_ERROR_NOT_IMPLEMENTED;
  }
  return result;
}

NS_IMETHODIMP    
nsEditorShell::JoinTableCells()
{
  nsresult  result = NS_NOINTERFACE;
  switch (mEditorType)
  {
    case eHTMLTextEditorType:
      {
        nsCOMPtr<nsITableEditor> tableEditor = do_QueryInterface(mEditor);
        if (tableEditor)
          result = tableEditor->JoinTableCells();
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
          result = tableEditor->GetCellAt(tableElement, rowIndex, colIndex, *_retval);
      }
      break;
    default:
      result = NS_ERROR_NOT_IMPLEMENTED;
  }

  return result;
}

// Note that the return param in the IDL must be the LAST out param here,
//   so order of params is different from nsIHTMLEditor
NS_IMETHODIMP    
nsEditorShell::GetCellDataAt(nsIDOMElement *tableElement, PRInt32 rowIndex, PRInt32 colIndex,
                             PRInt32 *aStartRowIndex, PRInt32 *aStartColIndex, 
                             PRInt32 *aRowSpan, PRInt32 *aColSpan, PRBool *aIsSelected, nsIDOMElement **_retval)
{
  if (!_retval || 
      !aStartRowIndex || !aStartColIndex || 
      !aRowSpan || !aColSpan || !aIsSelected )
    return NS_ERROR_NULL_POINTER;

  nsresult  result = NS_NOINTERFACE;
  switch (mEditorType)
  {
    case eHTMLTextEditorType:
      {
        nsCOMPtr<nsITableEditor> tableEditor = do_QueryInterface(mEditor);
        if (tableEditor)
          result = tableEditor->GetCellDataAt(tableElement, rowIndex, colIndex, *_retval,
                                              *aStartRowIndex, *aStartColIndex, *aRowSpan, *aColSpan, *aIsSelected);
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

  nsresult result;

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
nsEditorShell::StartSpellChecking(PRUnichar **aFirstMisspelledWord)
{
  nsresult  result = NS_NOINTERFACE;
  nsAutoString firstMisspelledWord;
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

    result = nsComponentManager::CreateInstance(kCSpellCheckerCID,
                                                nsnull,
                                                NS_GET_IID(nsISpellChecker),
                                                (void **)getter_AddRefs(mSpellChecker));

    if (NS_FAILED(result))
      return result;

    if (!mSpellChecker)
      return NS_ERROR_NULL_POINTER;

    result = mSpellChecker->SetDocument(tsDoc, PR_FALSE);

    if (NS_FAILED(result))
      return result;

    DeleteSuggestedWordList();
    // Return the first misspelled word and initialize the suggested list
    result = mSpellChecker->NextMisspelledWord(&firstMisspelledWord, &mSuggestedWordList);
  }
  *aFirstMisspelledWord = firstMisspelledWord.ToNewUnicode();
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
      word = "";
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
      word = "";
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
nsEditorShell::CloseSpellChecking()
{
  nsresult  result = NS_NOINTERFACE;
   // We can spell check with any editor type
  if (mEditor)
  {
    // Cleanup - kill the spell checker
    DeleteSuggestedWordList();
    mDictionaryList.Clear();
    mDictionaryIndex = 0;
    mSpellChecker = 0;
    result = NS_OK;
  }
  return result;
}

NS_IMETHODIMP    
nsEditorShell::DeleteSuggestedWordList()
{
  mSuggestedWordList.Clear();
  mSuggestedWordIndex = 0;
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

#if 0
//----------------------------------------
void nsEditorShell::SetButtonImage(nsIDOMNode * aParentNode, PRInt32 aBtnNum, const nsString &aResName)
{
  PRInt32 count = 0;
  nsCOMPtr<nsIDOMNode> button(FindNamedDOMNode(nsAutoString("button"), aParentNode, count, aBtnNum)); 
  count = 0;
  nsCOMPtr<nsIDOMNode> img(FindNamedDOMNode(nsAutoString("img"), button, count, 1)); 
  nsCOMPtr<nsIDOMHTMLImageElement> imgElement(do_QueryInterface(img));
  if (imgElement) {
    char * str = aResName.ToNewCString();
    imgElement->SetSrc(str);
    nsCRT::free(str);
  }

}
#endif

// XXXbe why is this needed?  eliminate
NS_IMETHODIMP    
nsEditorShell::ExecuteScript(nsIScriptContext * aContext, const nsString& aScript)
{
  if (nsnull != aContext) {
    const char* url = "";
    PRBool isUndefined = PR_FALSE;
    nsAutoString rVal;

#ifdef APP_DEBUG
    char* script_str = aScript.ToNewCString();
    printf("Executing [%s]\n", script_str);
    nsCRT::free(script_str);
#endif

    aContext->EvaluateString(aScript, url, 0, nsnull, rVal, &isUndefined);
  } 
  return NS_OK;
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
nsEditorShell::StartLogging(nsIFileSpec *logFile)
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

// nsIDocumentLoaderObserver methods
NS_IMETHODIMP
nsEditorShell::OnStartDocumentLoad(nsIDocumentLoader* loader, nsIURI* aURL, const char* aCommand)
{
  // Start the throbber
  // TODO: We should also start/stop it for saving and publishing?
  SetChromeAttribute( mWebShell, "Editor:Throbber", "busy", "true" );
  return NS_OK;
}

NS_IMETHODIMP
nsEditorShell::OnEndDocumentLoad(nsIDocumentLoader* loader, nsIChannel* channel, nsresult aStatus)
{
  // for pages with charsets, this gets called the first time with a 
  // non-zero status value. Don't prepare the editor that time.
  // aStatus will be NS_BINDING_ABORTED then.
  nsresult res = NS_OK;
	if (NS_SUCCEEDED(aStatus))
	{
    nsCOMPtr<nsIURI>  aUrl;
    channel->GetURI(getter_AddRefs(aUrl));
    res = PrepareDocumentForEditing(aUrl);
    SetChromeAttribute( mWebShell, "Editor:Throbber", "busy", "false" );
  }
  return res;
}

NS_IMETHODIMP
nsEditorShell::OnStartURLLoad(nsIDocumentLoader* loader,
                              nsIChannel* channel)
{

   return NS_OK;
}

NS_IMETHODIMP
nsEditorShell::OnProgressURLLoad(nsIDocumentLoader* loader,
                                    nsIChannel* channel, PRUint32 aProgress, 
                                    PRUint32 aProgressMax)
{
  return NS_OK;
}

NS_IMETHODIMP
nsEditorShell::OnStatusURLLoad(nsIDocumentLoader* loader,
                                  nsIChannel* channel, nsString& aMsg)
{
  return NS_OK;
}

NS_IMETHODIMP
nsEditorShell::OnEndURLLoad(nsIDocumentLoader* loader,
                               nsIChannel* channel, nsresult aStatus)
{
   return NS_OK;
}

NS_IMETHODIMP
nsEditorShell::HandleUnknownContentType(nsIDocumentLoader* loader, 
                                           nsIChannel* channel,
                                           const char *aContentType,
                                           const char *aCommand )
{
   return NS_OK;
}
