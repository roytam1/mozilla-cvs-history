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

#include <stdio.h>

#include "nsEditorShell.h"
#include "nsIBrowserWindow.h"
#include "nsIWebShell.h"
#include "pratom.h"
#include "prprf.h"
#include "nsIComponentManager.h"
//#include "nsAppCores.h"
#include "nsAppCoresCIDs.h"
#include "nsIDOMAppCoresManager.h"

#include "nsIScriptContext.h"
#include "nsIScriptContextOwner.h"
#include "nsIScriptGlobalObject.h"
#include "nsIDOMDocument.h"
#include "nsIDiskDocument.h"
#include "nsIDocument.h"
#include "nsIDOMWindow.h"

#include "nsIScriptGlobalObject.h"
#include "nsIWebShell.h"
#include "nsIWebShellWindow.h"
#include "nsCOMPtr.h"

#include "nsIServiceManager.h"
#include "nsIURL.h"
#include "nsIWidget.h"
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
#include "nsIDOMToolkitCore.h"
#include "nsIFindComponent.h"

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
#include "nsITextEditor.h"
#include "nsIHTMLEditor.h"
#include "nsEditorCID.h"

#include "nsIComponentManager.h"
#include "nsIServiceManager.h"
#include "nsTextServicesCID.h"
#include "nsITextServicesDocument.h"
#include "nsISpellChecker.h"
///////////////////////////////////////

// Drag & Drop, Clipboard
#include "nsWidgetsCID.h"
#include "nsIClipboard.h"
#include "nsITransferable.h"
#include "nsISupportsArray.h"


/* Define Class IDs */
static NS_DEFINE_IID(kAppShellServiceCID,       NS_APPSHELL_SERVICE_CID);
static NS_DEFINE_IID(kEditorAppCoreCID,         NS_EDITORAPPCORE_CID);
static NS_DEFINE_CID(kHTMLEditorCID,            NS_HTMLEDITOR_CID);
static NS_DEFINE_CID(kTextEditorCID,            NS_TEXTEDITOR_CID);
static NS_DEFINE_CID(kCTextServicesDocumentCID, NS_TEXTSERVICESDOCUMENT_CID);
static NS_DEFINE_CID(kCSpellCheckerCID,         NS_SPELLCHECKER_CID);
static NS_DEFINE_IID(kFileWidgetCID,            NS_FILEWIDGET_CID);

/* Define Interface IDs */
static NS_DEFINE_IID(kINetSupportIID,           NS_INETSUPPORT_IID);
static NS_DEFINE_IID(kISupportsIID,             NS_ISUPPORTS_IID);

#define APP_DEBUG 0 


nsresult
NS_NewEditorShell(nsIEditorShell** aEditorShell)
{
    NS_PRECONDITION(aEditorShell != nsnull, "null ptr");
    if (! aEditorShell)
        return NS_ERROR_NULL_POINTER;

    *aEditorShell = new nsEditorShell();
    if (! *aEditorShell)
        return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(*aEditorShell);
    return NS_OK;
}

/////////////////////////////////////////////////////////////////////////
// nsEditorShell
/////////////////////////////////////////////////////////////////////////

nsEditorShell::nsEditorShell()
{
#ifdef APP_DEBUG
  printf("Created nsEditorShell\n");
#endif

  //mScriptObject         = nsnull;
  mToolbarWindow        = nsnull;
  mToolbarScriptContext = nsnull;
  mContentWindow        = nsnull;
  mContentScriptContext = nsnull;
  mWebShellWin          = nsnull;
  mWebShell             = nsnull;
  mSuggestedWordIndex   = 0;
  
  NS_INIT_REFCNT();
}

nsEditorShell::~nsEditorShell()
{
  NS_IF_RELEASE(mToolbarScriptContext);
  NS_IF_RELEASE(mContentScriptContext);

  //NS_IF_RELEASE(mToolbarWindow);
  //NS_IF_RELEASE(mContentWindow);
  //NS_IF_RELEASE(mWebShellWin);
  //NS_IF_RELEASE(mWebShell);
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
  else if ( aIID.Equals(nsIEditorShell::GetIID()) ) {
    *aInstancePtr = (void*) ((nsIEditorShell*)this);
    AddRef();
    return NS_OK;
  }
  else if ( aIID.Equals(nsIEditorSpellCheck::GetIID()) ) {
    *aInstancePtr = (void*) ((nsIEditorSpellCheck*)this);
    AddRef();
    return NS_OK;
  }
/*
  else if (aIID.Equals(nsINetSupport::GetIID())) {
    *aInstancePtr = (void*) ((nsINetSupport*)this);
    AddRef();
    return NS_OK;
  }
*/
  else if (aIID.Equals(nsIStreamObserver::GetIID())) {
    *aInstancePtr = (void*) ((nsIStreamObserver*)this);
     AddRef();
    return NS_OK;
  }
 
  return NS_ERROR_NO_INTERFACE;
}

#if 0
NS_IMETHODIMP 
nsEditorShell::GetScriptObject(nsIScriptContext *aContext, void** aScriptObject)
{
  NS_PRECONDITION(nsnull != aScriptObject, "null arg");
  nsresult res = NS_OK;
  if (nsnull == mScriptObject) 
  {
      res = NS_NewScriptEditorAppCore(aContext, 
                                (nsISupports *)(nsIEditorShell*)this, 
                                nsnull, 
                                &mScriptObject);
  }

  *aScriptObject = mScriptObject;
  return res;
}

#endif

nsIScriptContext *    
nsEditorShell::GetScriptContext(nsIDOMWindow * aWin)
{
  nsIScriptContext * scriptContext = nsnull;
  if (nsnull != aWin) {
    nsIDOMDocument * domDoc;
    aWin->GetDocument(&domDoc);
    if (nsnull != domDoc) {
      nsIDocument * doc;
      if (NS_OK == domDoc->QueryInterface(nsIDocument::GetIID(),(void**)&doc)) {
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


NS_IMETHODIMP    
nsEditorShell::Init()
{  
  //nsBaseAppCore::Init(aId);

  nsAutoString    editorType = "html";      // default to creating HTML editor
  mEditorTypeString = editorType;
  mEditorTypeString.ToLowerCase();

  return NS_OK;
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
      cv->QueryInterface(nsIDocumentViewer::GetIID(), (void**) &docv);
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
  if (theType == "text")
  {
    mEditorTypeString = theType;
    return NS_OK;
  }
  else if (theType == "html" || theType == "")
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
  
  if (mEditorTypeString == "text")
  {
    nsCOMPtr<nsITextEditor> editor;
    err = nsComponentManager::CreateInstance(kTextEditorCID, nsnull, nsITextEditor::GetIID(), getter_AddRefs(editor));
    if(!editor)
      err = NS_ERROR_OUT_OF_MEMORY;
      
    if (NS_SUCCEEDED(err))
    {
      err = editor->Init(aDoc, aPresShell);
      if (NS_SUCCEEDED(err) && editor)
      {
        mEditor = do_QueryInterface(editor);		// this does the addref that is the owning reference
        mEditorType = ePlainTextEditorType;
      }
    }
  }
  else if (mEditorTypeString == "html" || mEditorTypeString == "")  // empty string default to HTML editor
  {
    nsCOMPtr<nsIHTMLEditor> editor;
    err = nsComponentManager::CreateInstance(kHTMLEditorCID, nsnull, nsIHTMLEditor::GetIID(), getter_AddRefs(editor));
    if(!editor)
      err = NS_ERROR_OUT_OF_MEMORY;
      
    if (NS_SUCCEEDED(err))
    {
      err = editor->Init(aDoc, aPresShell);
      if (NS_SUCCEEDED(err) && editor)
      {
        mEditor = do_QueryInterface(editor);		// this does the addref that is the owning reference
        mEditorType = eHTMLTextEditorType;
      }
    }
  }
  else
  {
    err = NS_ERROR_INVALID_ARG;    // this is not an editor we know about
#if DEBUG
    nsString  errorMsg = "Failed to init editor. Unknown editor type \"";
    errorMsg += mEditorTypeString;
    errorMsg += "\"\n";
    char  *errorMsgCString = errorMsg.ToNewCString();
       NS_WARNING(errorMsgCString);
       delete [] errorMsgCString;
#endif
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

  nsCOMPtr<nsIContentViewer> contViewer;
  aWebShell->GetContentViewer(getter_AddRefs(contViewer));
  if (contViewer)
  {
    nsCOMPtr<nsIDocumentViewer> docViewer;
    if (NS_SUCCEEDED(contViewer->QueryInterface(nsIDocumentViewer::GetIID(), (void**)getter_AddRefs(docViewer))))
    {
      nsCOMPtr<nsIDocument> aDoc;
      docViewer->GetDocument(*getter_AddRefs(aDoc));
      if (aDoc)
      {
        nsCOMPtr<nsIDOMDocument> aDOMDoc;
        if (NS_SUCCEEDED(aDoc->QueryInterface(nsIDOMDocument::GetIID(), (void**)getter_AddRefs(aDOMDoc))))
        {
          nsCOMPtr<nsIPresShell> presShell = dont_AddRef(GetPresShellFor(aWebShell));
          if( presShell )
          {
            err = InstantiateEditor(aDOMDoc, presShell);
          }
        }
      }
    }

#if 0    
// Not sure if this makes sense any more
    PRInt32 i, n;
    aWebShell->GetChildCount(n);
    for (i = 0; i < n; i++) {
      nsIWebShell* mChild;
      aWebShell->ChildAt(i, mChild);
      DoEditorMode(mChild);
      NS_RELEASE(mChild);
    }
#endif
  }
  
  return err;
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

  nsAutoString		attributeStr(attr);
  nsAutoString		valueStr(value);
  
  switch (mEditorType)
  {
    case ePlainTextEditorType:
      {
        // should we allow this?
        nsCOMPtr<nsITextEditor>  textEditor = do_QueryInterface(mEditor);
        if (textEditor)
          err = textEditor->SetTextProperty(styleAtom, &attributeStr, &valueStr);
      }
      break;
    case eHTMLTextEditorType:
      {
        nsCOMPtr<nsIHTMLEditor>  htmlEditor = do_QueryInterface(mEditor);
        if (htmlEditor)
          err = htmlEditor->SetTextProperty(styleAtom, &attributeStr, &valueStr);
      }
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
      {
        // should we allow this?
        nsCOMPtr<nsITextEditor>  textEditor = do_QueryInterface(mEditor);
        if (textEditor)
          err = textEditor->RemoveTextProperty(styleAtom, &aAttr);
      }
      break;
    case eHTMLTextEditorType:
      {
        nsCOMPtr<nsIHTMLEditor>  htmlEditor = do_QueryInterface(mEditor);
        if (htmlEditor)
          err = htmlEditor->RemoveTextProperty(styleAtom, &aAttr);
      }
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
  static const char*  sAllKnownStyles[] = {
    "B",
    "I",
    "U",
    nsnull      // this null is important
  };
  
  nsAutoString  allStr(prop);
  nsAutoString	aAttr(attr);
  
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
nsEditorShell::GetTextProperty(const PRUnichar *prop, const PRUnichar *attr, const PRUnichar *value, PRUnichar **firstHas, PRUnichar **anyHas, PRUnichar **allHas)
{
  nsIAtom    *styleAtom = nsnull;
  nsresult  err = NS_NOINTERFACE;

  PRBool    firstOfSelectionHasProp = PR_FALSE;
  PRBool    anyOfSelectionHasProp = PR_FALSE;
  PRBool    allOfSelectionHasProp = PR_FALSE;
  
  styleAtom = NS_NewAtom(prop);      /// XXX Hack alert! Look in nsIEditProperty.h for this

  nsAutoString  aAttr(attr);
  nsAutoString  aValue(value);
  
  switch (mEditorType)
  {
    case ePlainTextEditorType:
      {
        // should we allow this?
        nsCOMPtr<nsITextEditor>  textEditor = do_QueryInterface(mEditor);
        if (textEditor)
          err = textEditor->GetTextProperty(styleAtom, &aAttr, &aValue, firstOfSelectionHasProp, anyOfSelectionHasProp, allOfSelectionHasProp);
      }
      break;
    case eHTMLTextEditorType:
      {
        nsCOMPtr<nsIHTMLEditor>  htmlEditor = do_QueryInterface(mEditor);
        if (htmlEditor)
          err = htmlEditor->GetTextProperty(styleAtom, &aAttr, &aValue, firstOfSelectionHasProp, anyOfSelectionHasProp, allOfSelectionHasProp);
      }
      break;
    default:
      err = NS_ERROR_NOT_IMPLEMENTED;
  }

  nsAutoString	trueStr("true");
  nsAutoString  falseStr("false");
  
  *firstHas = (firstOfSelectionHasProp) ? trueStr.ToNewUnicode() : falseStr.ToNewUnicode();
  *anyHas = (anyOfSelectionHasProp) ? trueStr.ToNewUnicode() : falseStr.ToNewUnicode();
  *allHas = (allOfSelectionHasProp) ? trueStr.ToNewUnicode() : falseStr.ToNewUnicode();
    
  NS_RELEASE(styleAtom);
  return err;
}

NS_IMETHODIMP nsEditorShell::SetBackgroundColor(const PRUnichar *color)
{
  nsresult result = NS_NOINTERFACE;
  
  nsAutoString aColor(color);
  
  switch (mEditorType)
  {
    case eHTMLTextEditorType:
    {
      nsCOMPtr<nsIHTMLEditor>  htmlEditor = do_QueryInterface(mEditor);
      if (htmlEditor)
        result = htmlEditor->SetBackgroundColor(aColor);
      break;
    }
    default:
      result = NS_ERROR_NOT_IMPLEMENTED;
  }

  return result;
}

NS_IMETHODIMP nsEditorShell::SetBodyAttribute(const PRUnichar *attr, const PRUnichar *value)
{
  nsresult result = NS_NOINTERFACE;
  
  nsAutoString  aAttr(attr);
  nsAutoString  aValue(value);
  
  switch (mEditorType)
  {
    case eHTMLTextEditorType:
    {
      nsCOMPtr<nsIHTMLEditor>  htmlEditor = do_QueryInterface(mEditor);
      if (htmlEditor)
        result = htmlEditor->SetBodyAttribute(aAttr, aValue);
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
nsEditorShell::PrepareDocumentForEditing()
{
  if (!mContentAreaWebShell)
    return NS_ERROR_NOT_INITIALIZED;

  return DoEditorMode(mContentAreaWebShell);
}

NS_IMETHODIMP    
nsEditorShell::SetToolbarWindow(nsIDOMWindow* aWin)
{
  NS_PRECONDITION(aWin != nsnull, "null ptr");
  if (!aWin)
      return NS_ERROR_NULL_POINTER;

  mToolbarWindow = aWin;
  //NS_ADDREF(aWin);
  mToolbarScriptContext = GetScriptContext(aWin);

  return NS_OK;
}

NS_IMETHODIMP    
nsEditorShell::SetContentWindow(nsIDOMWindow* aWin)
{
  NS_PRECONDITION(aWin != nsnull, "null ptr");
  if (!aWin)
      return NS_ERROR_NULL_POINTER;

  mContentWindow = aWin;
  mContentScriptContext = GetScriptContext(mContentWindow);		// XXX does this AddRef?

  nsresult  rv;
  nsCOMPtr<nsIScriptGlobalObject> globalObj = do_QueryInterface(mContentWindow, &rv);
  if (NS_FAILED(rv) || !globalObj)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIWebShell> webShell;
  globalObj->GetWebShell(getter_AddRefs(webShell));
  if (!webShell)
    return NS_ERROR_FAILURE;
    
  mContentAreaWebShell = webShell;			// dont AddRef
  return mContentAreaWebShell->SetDocLoaderObserver((nsIDocumentLoaderObserver *)this);
}


NS_IMETHODIMP    
nsEditorShell::SetWebShellWindow(nsIDOMWindow* aWin)
{
  NS_PRECONDITION(aWin != nsnull, "null ptr");
  if (!aWin)
      return NS_ERROR_NULL_POINTER;

//  if (!mContentWindow) {
//    return NS_ERROR_FAILURE;
//  }
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
  delete[] cstr;
#endif

  nsCOMPtr<nsIWebShellContainer> webShellContainer;
  mWebShell->GetContainer(*getter_AddRefs(webShellContainer));
  if (!webShellContainer)
    return NS_ERROR_NOT_INITIALIZED;

  nsCOMPtr<nsIWebShellWindow> webShellWin = do_QueryInterface(webShellContainer, &rv);
  mWebShellWin = webShellWin;
    
  return rv;
}

NS_IMETHODIMP
nsEditorShell::CreateWindowWithURL(const char* urlStr)
{
  nsresult rv = NS_OK;
  
#if 0
  /*
   * Create the Application Shell instance...
   */
  nsIAppShellService* appShell = nsnull;
  rv = nsServiceManager::GetService(kAppShellServiceCID,
                                    nsIAppShellService::GetIID(),
                                    (nsISupports**)&appShell);
  if (NS_FAILED(rv))
    return rv;

  nsCOMPtr<nsIURL> url = nsnull;
  nsIWebShellWindow* newWindow = nsnull;
  
  rv = NS_NewURL(getter_AddRefs(url), urlStr);
  if (NS_FAILED(rv) || !url)
    goto done;

  appShell->CreateTopLevelWindow(nsnull, url, PR_TRUE, newWindow,
              nsnull, nsnull, 615, 480);
  
done:
  /* Release the shell... */
  if (nsnull != appShell) {
    nsServiceManager::ReleaseService(kAppShellServiceCID, appShell);
  }
#else


  // This code is to ensure that the editor's pseudo-onload handler is always called.
  static NS_DEFINE_CID(kToolkitCoreCID,           NS_TOOLKITCORE_CID);

  /*
   * Create the toolkit core instance...
   */
  nsIDOMToolkitCore* toolkit = nsnull;
  rv = nsServiceManager::GetService(kToolkitCoreCID,
                                    nsIDOMToolkitCore::GetIID(),
                                    (nsISupports**)&toolkit);
  if (NS_FAILED(rv))
    return rv;

  //nsIWebShellWindow* newWindow = nsnull;
  
  toolkit->ShowWindowWithArgs( urlStr, nsnull, "chrome://editor/content/EditorInitPage.html" );
  
  /* Release the toolkit... */
  if (nsnull != toolkit) {
    nsServiceManager::ReleaseService(kToolkitCoreCID, toolkit);
  }

#endif

  return rv;
}

NS_IMETHODIMP    
nsEditorShell::NewWindow()
{  
  return CreateWindowWithURL("chrome://editor/content/");
}

NS_IMETHODIMP    
nsEditorShell::Open()
{
  nsresult  result;
  
  // GetLocalFileURL will do all the nsFileSpec/nsFileURL conversions,
  // and return a "file:///" string
  nsCOMPtr<nsIFileWidget>  fileWidget;

static NS_DEFINE_IID(kCFileWidgetCID, NS_FILEWIDGET_CID);

  result = nsComponentManager::CreateInstance(kCFileWidgetCID, nsnull, nsIFileWidget::GetIID(), getter_AddRefs(fileWidget));
  if (NS_FAILED(result) || !fileWidget)
    return result;
    
  result = NS_NOINTERFACE;
  
  switch (mEditorType)
  {
    case eHTMLTextEditorType:
    {
      nsCOMPtr<nsIHTMLEditor>  htmlEditor = do_QueryInterface(mEditor);

      if (htmlEditor)
      {
        // This was written for all local file getting
        // TODO: NEED TO GET PROPER PARENT WINDOW
        PRUnichar *fileURLString = nsnull;
        nsAutoString filterType("html");
        result = GetLocalFileURL(nsnull, filterType.GetUnicode(), &fileURLString);
        if (NS_FAILED(result) || !fileURLString || !*fileURLString)
          return result;

        // all I want to do is call a method on nsToolkitCore that would normally
        // be static. But I have to go through all this crap. XPCOM sucks so bad.
        static NS_DEFINE_IID(kToolkitCoreCID, NS_TOOLKITCORE_CID);
        nsCOMPtr<nsIDOMToolkitCore>  toolkitCore;
        result = nsComponentManager::CreateInstance(kToolkitCoreCID,
                                          nsnull,
                                          nsIDOMToolkitCore::GetIID(),
                                          getter_AddRefs(toolkitCore));
        if (NS_SUCCEEDED(result) && toolkitCore)
        {
          // at some point we need to be passing nsFileSpecs around. When nsIUrl is fileSpec-
          // savvy, we should use that.
          result = toolkitCore->ShowWindowWithArgs("chrome://editor/content", nsnull, fileURLString/*fileURL.GetAsString()*/);
        }
  
				// delete the string
				
      }
      break;
    }
    default:
      result = NS_ERROR_NOT_IMPLEMENTED;
  }

  return result;

}

NS_IMETHODIMP
nsEditorShell::Save()
{
  nsresult  err = NS_NOINTERFACE;
  
  switch (mEditorType)
  {
    case ePlainTextEditorType:
      {
        nsCOMPtr<nsITextEditor>  textEditor = do_QueryInterface(mEditor);
        if (textEditor)
          err = textEditor->Save();
      }
      break;
    case eHTMLTextEditorType:
      {
        nsCOMPtr<nsIHTMLEditor>  htmlEditor = do_QueryInterface(mEditor);
        if (htmlEditor)
          err = htmlEditor->Save();
      }
      break;
    default:
      err = NS_ERROR_NOT_IMPLEMENTED;
  }

  return err;
}

NS_IMETHODIMP    
nsEditorShell::SaveAs()
{
  nsresult  err = NS_NOINTERFACE;
  
  switch (mEditorType)
  {
    case ePlainTextEditorType:
      {
        nsCOMPtr<nsITextEditor>  textEditor = do_QueryInterface(mEditor);
        if (textEditor)
          err = textEditor->SaveAs(PR_FALSE);
      }
      break;
    case eHTMLTextEditorType:
      {
        nsCOMPtr<nsIHTMLEditor>  htmlEditor = do_QueryInterface(mEditor);
        if (htmlEditor)
          err = htmlEditor->SaveAs(PR_FALSE);
      }
      break;
    default:
      err = NS_ERROR_NOT_IMPLEMENTED;
  }

  return err;
}

NS_IMETHODIMP    
nsEditorShell::CloseWindow()
{
  nsresult rv = NS_OK;
  
  nsCOMPtr<nsIDOMDocument>  theDoc;
  if (NS_SUCCEEDED(GetEditorDocument(getter_AddRefs(theDoc))) && theDoc)
  {
     nsCOMPtr<nsIDiskDocument> diskDoc = do_QueryInterface(theDoc);
    if (diskDoc)
    {
      PRInt32  modCount = 0;
      diskDoc->GetModCount(&modCount);
    
      if (modCount > 0)
      {
        // Show the Save/Dont save dialog, somehow.
      
      }
    }
  
  }

  return rv;
}

NS_IMETHODIMP    
nsEditorShell::Print()
{ 
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP    
nsEditorShell::Exit()
{  
  nsIAppShellService* appShell = nsnull;

  /*
   * Create the Application Shell instance...
   */
  nsresult rv = nsServiceManager::GetService(kAppShellServiceCID,
                                             nsIAppShellService::GetIID(),
                                             (nsISupports**)&appShell);
  if (NS_SUCCEEDED(rv)) {
    appShell->Shutdown();
    nsServiceManager::ReleaseService(kAppShellServiceCID, appShell);
  } 
  return NS_OK;
}

NS_IMETHODIMP
nsEditorShell::GetLocalFileURL(nsIDOMWindow *parent, const PRUnichar *filterType, PRUnichar **_retval)
{
  nsAutoString aFilterType(filterType);
  PRBool htmlFilter = aFilterType.EqualsIgnoreCase("html");
  PRBool imgFilter = aFilterType.EqualsIgnoreCase("img");

  *_retval = nsnull;
  
  // TODO: DON'T ACCEPT NULL PARENT AFTER WIDGET IS FIXED
  if (/*!aParent||*/ !(htmlFilter || imgFilter))
    return NS_ERROR_NOT_INITIALIZED;


  nsCOMPtr<nsIFileWidget>  fileWidget;
  // TODO: WHERE TO WE PUT GLOBAL STRINGS TO BE LOCALIZED?
  nsString title(htmlFilter ? "Open HTML file" : "Select Image File");
  nsFileSpec fileSpec;
  // TODO: GET THE DEFAULT DIRECTORY FOR DIFFERENT TYPES FROM PREFERENCES
  nsFileSpec aDisplayDirectory;

  nsresult res = nsComponentManager::CreateInstance(kFileWidgetCID,
					     nsnull,
					     nsIFileWidget::GetIID(),
					     (void**)&fileWidget);

  if (NS_SUCCEEDED(res))
  {
    nsFileDlgResults dialogResult;
    if (htmlFilter)
    {
      nsAutoString titles[] = {"HTML Files"};
      nsAutoString filters[] = {"*.htm; *.html; *.shtml"};
      fileWidget->SetFilterList(1, titles, filters);
      dialogResult = fileWidget->GetFile(nsnull, title, fileSpec);
    } else {
      nsAutoString titles[] = {"Image Files"};
      nsAutoString filters[] = {"*.gif; *.jpg; *.jpeg; *.png"};
      fileWidget->SetFilterList(1, titles, filters);
      dialogResult = fileWidget->GetFile(nsnull, title, fileSpec);
    }
    // Do this after we get this from preferences
    //fileWidget->SetDisplayDirectory(aDisplayDirectory);
    
    // First param should be Parent window, but type is nsIWidget*
    // Bug is filed to change this to a more suitable window type
    if (dialogResult != nsFileDlgResults_Cancel) 
    {
      // Get the platform-specific format
      // Convert it to the string version of the URL format
      // NOTE: THIS CRASHES IF fileSpec is empty
      nsFileURL url(fileSpec);
      nsString	returnVal = url.GetURLString();
      *_retval = returnVal.ToNewUnicode();
    }
    // TODO: SAVE THIS TO THE PREFS?
    fileWidget->GetDisplayDirectory(aDisplayDirectory);
  }

  return res;
}

NS_IMETHODIMP    
nsEditorShell::Undo()
{ 
  nsresult  err = NS_NOINTERFACE;
  
  switch (mEditorType)
  {
    case ePlainTextEditorType:
      {
        nsCOMPtr<nsITextEditor>  textEditor = do_QueryInterface(mEditor);
        if (textEditor)
          err = textEditor->Undo(1);
      }
      break;
    case eHTMLTextEditorType:
      {
        nsCOMPtr<nsIHTMLEditor>  htmlEditor = do_QueryInterface(mEditor);
        if (htmlEditor)
          err = htmlEditor->Undo(1);
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
      {
        nsCOMPtr<nsITextEditor>  textEditor = do_QueryInterface(mEditor);
        if (textEditor)
          err = textEditor->Redo(1);
      }
      break;
    case eHTMLTextEditorType:
      {
        nsCOMPtr<nsIHTMLEditor>  htmlEditor = do_QueryInterface(mEditor);
        if (htmlEditor)
          err = htmlEditor->Redo(1);
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
      {
        nsCOMPtr<nsITextEditor>  textEditor = do_QueryInterface(mEditor);
        if (textEditor)
          err = textEditor->Cut();
      }
      break;
    case eHTMLTextEditorType:
      {
        nsCOMPtr<nsIHTMLEditor>  htmlEditor = do_QueryInterface(mEditor);
        if (htmlEditor)
          err = htmlEditor->Cut();
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
      {
        nsCOMPtr<nsITextEditor>  textEditor = do_QueryInterface(mEditor);
        if (textEditor)
          err = textEditor->Copy();
      }
      break;
    case eHTMLTextEditorType:
      {
        nsCOMPtr<nsIHTMLEditor>  htmlEditor = do_QueryInterface(mEditor);
        if (htmlEditor)
          err = htmlEditor->Copy();
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
      {
        nsCOMPtr<nsITextEditor>  textEditor = do_QueryInterface(mEditor);
        if (textEditor)
          err = textEditor->Paste();
      }
      break;
    case eHTMLTextEditorType:
      {
        nsCOMPtr<nsIHTMLEditor>  htmlEditor = do_QueryInterface(mEditor);
        if (htmlEditor)
          err = htmlEditor->Paste();
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
      {
        nsCOMPtr<nsITextEditor>  textEditor = do_QueryInterface(mEditor);
        if (textEditor)
          err = textEditor->PasteAsQuotation();
      }
      break;
    case eHTMLTextEditorType:
      {
        nsCOMPtr<nsIHTMLEditor>  htmlEditor = do_QueryInterface(mEditor);
        if (htmlEditor)
          err = htmlEditor->PasteAsQuotation();
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
      {
        nsCOMPtr<nsITextEditor>  textEditor = do_QueryInterface(mEditor);
        if (textEditor)
          err = textEditor->PasteAsQuotation();
      }
      break;
    case eHTMLTextEditorType:
      {
        nsCOMPtr<nsIHTMLEditor>  htmlEditor = do_QueryInterface(mEditor);
        if (htmlEditor)
          err = htmlEditor->PasteAsCitedQuotation(aCiteString);
      }
      break;
    default:
      err = NS_ERROR_NOT_IMPLEMENTED;
  }

  return err;
}

NS_IMETHODIMP    
nsEditorShell::InsertAsQuotation(const PRUnichar *quotedText)
{  
  nsresult  err = NS_NOINTERFACE;
  
  nsAutoString aQuotedText(quotedText);
  
  switch (mEditorType)
  {
    case ePlainTextEditorType:
      {
        nsCOMPtr<nsITextEditor>  textEditor = do_QueryInterface(mEditor);
        if (textEditor)
          err = textEditor->InsertAsQuotation(aQuotedText);
      }
      break;
    case eHTMLTextEditorType:
      {
        nsCOMPtr<nsIHTMLEditor>  htmlEditor = do_QueryInterface(mEditor);
        if (htmlEditor)
          err = htmlEditor->InsertAsQuotation(aQuotedText);
      }
      break;
    default:
      err = NS_ERROR_NOT_IMPLEMENTED;
  }

  return err;
}

NS_IMETHODIMP    
nsEditorShell::InsertAsCitedQuotation(const PRUnichar *quotedText, const PRUnichar *cite)
{  
  nsresult  err = NS_NOINTERFACE;
  
  nsAutoString aQuotedText(quotedText);
  nsAutoString aCiteString(cite);
  
  switch (mEditorType)
  {
    case ePlainTextEditorType:
      {
        nsCOMPtr<nsITextEditor>  textEditor = do_QueryInterface(mEditor);
        if (textEditor)
          err = textEditor->InsertAsQuotation(aQuotedText);
      }
      break;
    case eHTMLTextEditorType:
      {
        nsCOMPtr<nsIHTMLEditor>  htmlEditor = do_QueryInterface(mEditor);
        if (htmlEditor)
          err = htmlEditor->InsertAsCitedQuotation(aQuotedText, aCiteString);
      }
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
      {
        nsCOMPtr<nsITextEditor>  textEditor = do_QueryInterface(mEditor);
        if (textEditor)
          err = textEditor->SelectAll();
      }
      break;
    case eHTMLTextEditorType:
      {
        nsCOMPtr<nsIHTMLEditor>  htmlEditor = do_QueryInterface(mEditor);
        if (htmlEditor)
          err = htmlEditor->SelectAll();
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
  nsIEditor::ECollapsedSelectionAction selectionAction;

  switch(action)
  {
    case nsIEditor::eDeleteRight:
      selectionAction = nsIEditor::eDeleteRight;
      break;
    case nsIEditor::eDeleteLeft:
      selectionAction = nsIEditor::eDeleteLeft;
      break;
    default:
      selectionAction = nsIEditor::eDoNothing;
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
  
  nsAutoString aTextToInsert(textToInsert);
  
  switch (mEditorType)
  {
    case ePlainTextEditorType:
      {
        nsCOMPtr<nsITextEditor>  textEditor = do_QueryInterface(mEditor);
        if (textEditor)
          err = textEditor->InsertText(aTextToInsert);
      }
      break;
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
nsEditorShell::InsertBreak()
{
  nsresult  err = NS_NOINTERFACE;
  
  nsCOMPtr<nsIEditor> editor = do_QueryInterface(mEditor);
  if (editor)
    err = editor->InsertBreak();
  
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
  nsIFindComponent *findComponent;
  nsresult rv = nsServiceManager::GetService( NS_IFINDCOMPONENT_PROGID,
                                     nsIFindComponent::GetIID(),
                                     (nsISupports**)&findComponent );
  if ( NS_SUCCEEDED(rv) && findComponent )
  {
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
    
    // Release the service.
    nsServiceManager::ReleaseService( NS_IFINDCOMPONENT_PROGID, findComponent );
  }
  else
  {
    NS_ASSERTION(0, "GetService failed for find component.");
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
nsEditorShell::GetContentsAsText(PRUnichar * *contentsAsText)
{
  nsresult  err = NS_NOINTERFACE;
  
  nsString aContentsAsText;
  
  switch (mEditorType)
  {
    case ePlainTextEditorType:
      {
        nsCOMPtr<nsITextEditor>  textEditor = do_QueryInterface(mEditor);
        if (textEditor)
          err = textEditor->OutputTextToString(aContentsAsText);
      }
      break;
    case eHTMLTextEditorType:
      {
        nsCOMPtr<nsIHTMLEditor>  htmlEditor = do_QueryInterface(mEditor);
        if (htmlEditor)
          err = htmlEditor->OutputTextToString(aContentsAsText);
      }
      break;
    default:
      err = NS_ERROR_NOT_IMPLEMENTED;
  }

  *contentsAsText = aContentsAsText.ToNewUnicode();
  
  return err;
}

NS_IMETHODIMP
nsEditorShell::GetContentsAsHTML(PRUnichar * *contentsAsHTML)
{
  nsresult  err = NS_NOINTERFACE;
  
  nsString aContentsAsHTML;
  
  switch (mEditorType)
  {
    case ePlainTextEditorType:
      {
        nsCOMPtr<nsITextEditor>  textEditor = do_QueryInterface(mEditor);
        if (textEditor)
          err = textEditor->OutputHTMLToString(aContentsAsHTML);
      }
      break;
    case eHTMLTextEditorType:
      {
        nsCOMPtr<nsIHTMLEditor>  htmlEditor = do_QueryInterface(mEditor);
        if (htmlEditor)
          err = htmlEditor->OutputHTMLToString(aContentsAsHTML);
      }
      break;
    default:
      err = NS_ERROR_NOT_IMPLEMENTED;
  }

  *contentsAsHTML = aContentsAsHTML.ToNewUnicode();
  
  return err;
}

NS_IMETHODIMP
nsEditorShell::GetWrapColumn(PRInt32* aWrapColumn)
{
  nsresult  err = NS_NOINTERFACE;
  
  if (!aWrapColumn)
    return NS_ERROR_NULL_POINTER;
  
  // fill result in case of failure
  *aWrapColumn = 0;
  
  switch (mEditorType)
  {
    case ePlainTextEditorType:
      {
        nsCOMPtr<nsITextEditor>  textEditor = do_QueryInterface(mEditor);
        if (textEditor)
          err = textEditor->GetBodyWrapWidth(aWrapColumn);
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
  nsresult  err = NS_NOINTERFACE;
  
  if (!aWrapColumn)
    return NS_ERROR_NULL_POINTER;
    
  switch (mEditorType)
  {
    case ePlainTextEditorType:
      {
        nsCOMPtr<nsITextEditor>  textEditor = do_QueryInterface(mEditor);
        if (textEditor)
          err = textEditor->SetBodyWrapWidth(aWrapColumn);
      }
      break;
    default:
      err = NS_ERROR_NOT_IMPLEMENTED;
  }

  return err;
}

NS_IMETHODIMP
nsEditorShell::GetParagraphFormat(PRUnichar * *paragraphFormat)
{
  nsresult  err = NS_NOINTERFACE;
  
  nsAutoString aParagraphFormat;
  
  switch (mEditorType)
  {
    case eHTMLTextEditorType:
      {
        nsCOMPtr<nsIHTMLEditor>  htmlEditor = do_QueryInterface(mEditor);
        if (htmlEditor)
          err = htmlEditor->GetParagraphFormat(aParagraphFormat);
      }
      break;
    default:
      err = NS_ERROR_NOT_IMPLEMENTED;
  }

  *paragraphFormat = aParagraphFormat.ToNewUnicode();
  
  return err;
}

NS_IMETHODIMP
nsEditorShell::SetParagraphFormat(PRUnichar * paragraphFormat)
{
  nsresult  err = NS_NOINTERFACE;
  
  nsAutoString aParagraphFormat(paragraphFormat);
  
  switch (mEditorType)
  {
    case eHTMLTextEditorType:
      {
        nsCOMPtr<nsIHTMLEditor>  htmlEditor = do_QueryInterface(mEditor);
        if (htmlEditor)
          err = htmlEditor->SetParagraphFormat(aParagraphFormat);
      }
      break;
    default:
      err = NS_ERROR_NOT_IMPLEMENTED;
  }

  return err;
}


NS_METHOD
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
nsEditorShell::GetEditorSelection(nsIDOMSelection** aEditorSelection)
{

  if (mEditor)
  {
    nsCOMPtr<nsIEditor>  editor = do_QueryInterface(mEditor);
    if (editor)
    {
      return editor->GetSelection(aEditorSelection);
    }
  }
  
  return NS_NOINTERFACE;
}

NS_IMETHODIMP
nsEditorShell::InsertList(const PRUnichar *listType)
{
  nsresult err = NS_NOINTERFACE;

  nsAutoString aListType(listType);
  
  switch (mEditorType)
  {
    case eHTMLTextEditorType:
      {
        nsCOMPtr<nsIHTMLEditor>  htmlEditor = do_QueryInterface(mEditor);
        if (htmlEditor)
          err = htmlEditor->InsertList(aListType);
      }
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
      {
        nsCOMPtr<nsIHTMLEditor>  htmlEditor = do_QueryInterface(mEditor);
        if (htmlEditor)
          err = htmlEditor->Indent(aIndent);
      }
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
      {
        nsCOMPtr<nsIHTMLEditor>  htmlEditor = do_QueryInterface(mEditor);
        if (htmlEditor)
          err = htmlEditor->Align(aAlignType);
      }
      break;
    case ePlainTextEditorType:
    default:
      err = NS_ERROR_NOT_IMPLEMENTED;
  }

  return err;
}

// Pop up the link dialog once we have dialogs ...  for now, hardwire it
NS_IMETHODIMP
nsEditorShell::InsertLink()
{
  nsresult  err = NS_NOINTERFACE;
  nsString tmpString ("http://www.mozilla.org/editor/");

  switch (mEditorType)
  {
    case eHTMLTextEditorType:
      {
        nsCOMPtr<nsIHTMLEditor>  htmlEditor = do_QueryInterface(mEditor);
        if (htmlEditor)
          err = htmlEditor->InsertLink(tmpString);
      }
      break;
    case ePlainTextEditorType:
    default:
      err = NS_ERROR_NOT_IMPLEMENTED;
  }

  return err;
}
// Pop up the image dialog once we have dialogs ...  for now, hardwire it
NS_IMETHODIMP
nsEditorShell::InsertImage()
{
  nsresult  err = NS_NOINTERFACE;
  
  nsString url ("http://www.mozilla.org/editor/images/pensplat.gif");
  nsString width("100");
  nsString height("138");
  nsString hspace("0");
  nsString border("1");
  nsString alt ("[pen splat]");
  nsString align ("left");
  
  switch (mEditorType)
  {
    case eHTMLTextEditorType:
      {
        nsCOMPtr<nsIHTMLEditor>  htmlEditor = do_QueryInterface(mEditor);
        if (htmlEditor)
          err = htmlEditor->InsertImage(url, width, height, hspace, hspace, border, alt, align);
      }
      break;
    case ePlainTextEditorType:
    default:
      err = NS_ERROR_NOT_IMPLEMENTED;
  }

  return err;
}

NS_IMETHODIMP
nsEditorShell::GetSelectedElement(const PRUnichar *tagName, nsIDOMElement **_retval)
{
  if (!_retval)
    return NS_ERROR_NULL_POINTER;

  nsresult  result = NS_NOINTERFACE;
  nsAutoString aTagName(tagName);
  
  switch (mEditorType)
  {
    case eHTMLTextEditorType:
      {
        nsCOMPtr<nsIHTMLEditor>  htmlEditor = do_QueryInterface(mEditor);
        if (htmlEditor)
          return htmlEditor->GetSelectedElement(aTagName, _retval);
      }
      break;
    case ePlainTextEditorType:
    default:
      result = NS_ERROR_NOT_IMPLEMENTED;
  }

  return result;
}

NS_IMETHODIMP
nsEditorShell::CreateElementWithDefaults(const PRUnichar *tagName, nsIDOMElement **_retval)
{
  if (!_retval)
    return NS_ERROR_NULL_POINTER;

  nsresult  result = NS_NOINTERFACE;
  nsAutoString aTagName(tagName);

  switch (mEditorType)
  {
    case eHTMLTextEditorType:
      {
        nsCOMPtr<nsIHTMLEditor>  htmlEditor = do_QueryInterface(mEditor);
        if (htmlEditor)
          return htmlEditor->CreateElementWithDefaults(aTagName, _retval);
      }
      break;
    case ePlainTextEditorType:
    default:
      result = NS_ERROR_NOT_IMPLEMENTED;
  }

  return result;
}


NS_IMETHODIMP
nsEditorShell::InsertElement(nsIDOMElement *element, PRBool deleteSelection)
{
  if (!element)
    return NS_ERROR_NULL_POINTER;

  nsresult  result = NS_NOINTERFACE;
  switch (mEditorType)
  {
    case eHTMLTextEditorType:
      {
        nsCOMPtr<nsIHTMLEditor>  htmlEditor = do_QueryInterface(mEditor);
        if (htmlEditor)
          result = htmlEditor->InsertElement(element, deleteSelection);
      }
      break;
    default:
      result = NS_ERROR_NOT_IMPLEMENTED;
  }

  return result;
}

NS_IMETHODIMP
nsEditorShell::SaveHLineSettings(nsIDOMElement* aElement)
{
  nsresult  result = NS_NOINTERFACE;
  switch (mEditorType)
  {
    case eHTMLTextEditorType:
      {
        nsCOMPtr<nsIHTMLEditor>  htmlEditor = do_QueryInterface(mEditor);
        if (htmlEditor)
          return htmlEditor->SaveHLineSettings(aElement);
      }
      break;
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
      {
        nsCOMPtr<nsIHTMLEditor>  htmlEditor = do_QueryInterface(mEditor);
        if (htmlEditor)
          return htmlEditor->InsertLinkAroundSelection(aAnchorElement);
      }
      break;
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
      {
        nsCOMPtr<nsIHTMLEditor> htmlEditor = do_QueryInterface(mEditor);
        if (htmlEditor)
          result = htmlEditor->SelectElement(aElement);
      }
      break;
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
      {
        nsCOMPtr<nsIHTMLEditor> htmlEditor = do_QueryInterface(mEditor);
        if (htmlEditor)
          result = htmlEditor->SetCaretAfterElement(aElement);
      }
      break;
    default:
      result = NS_ERROR_NOT_IMPLEMENTED;
  }

  return result;
}

NS_IMETHODIMP    
nsEditorShell::StartSpellChecking(PRUnichar **firstMisspelledWord)
{
  nsresult  result = NS_NOINTERFACE;
  nsAutoString aFirstMisspelledWord;
   // We can spell check with any editor type
  if (mEditor)
  {
    nsCOMPtr<nsITextServicesDocument>tsDoc;

    result = nsComponentManager::CreateInstance(
                                 kCTextServicesDocumentCID,
                                 nsnull,
                                 nsITextServicesDocument::GetIID(),
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
                                                nsISpellChecker::GetIID(),
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
    result = mSpellChecker->NextMisspelledWord(&aFirstMisspelledWord, &mSuggestedWordList);
  }
  *firstMisspelledWord = aFirstMisspelledWord.ToNewUnicode();
  return result;
}

NS_IMETHODIMP    
nsEditorShell::GetNextMisspelledWord(PRUnichar **nextMisspelledWord)
{
  nsresult  result = NS_NOINTERFACE;
  nsAutoString aNextMisspelledWord;
  
   // We can spell check with any editor type
  if (mEditor && mSpellChecker)
  {
    DeleteSuggestedWordList();
    result = mSpellChecker->NextMisspelledWord(&aNextMisspelledWord, &mSuggestedWordList);
  }
  *nextMisspelledWord = aNextMisspelledWord.ToNewUnicode();
  return result;
}

NS_IMETHODIMP    
nsEditorShell::GetSuggestedWord(PRUnichar **suggestedWord)
{
  nsresult  result = NS_NOINTERFACE;
  nsAutoString aSuggestedWord;
   // We can spell check with any editor type
  if (mEditor)
  {
    if ( mSuggestedWordIndex < mSuggestedWordList.Count())
    {
      mSuggestedWordList.StringAt(mSuggestedWordIndex, aSuggestedWord);
      mSuggestedWordIndex++;
    } else {
      // A blank string signals that there are no more strings
      aSuggestedWord = "";
    }
    result = NS_OK;
  }
  *suggestedWord = aSuggestedWord.ToNewUnicode();
  return result;
}

NS_IMETHODIMP    
nsEditorShell::CheckCurrentWord(const PRUnichar *suggestedWord, PRBool *aIsMisspelled)
{
  nsresult  result = NS_NOINTERFACE;
  nsAutoString aSuggestedWord(suggestedWord);
   // We can spell check with any editor type
  if (mEditor && mSpellChecker)
  {
    DeleteSuggestedWordList();
    result = mSpellChecker->CheckWord(&aSuggestedWord, aIsMisspelled, &mSuggestedWordList);
  }
  return result;
}

NS_IMETHODIMP    
nsEditorShell::ReplaceWord(const PRUnichar *misspelledWord, const PRUnichar *replaceWord, PRBool allOccurrences)
{
  nsresult  result = NS_NOINTERFACE;
  nsAutoString aMisspelledWord(misspelledWord);
  nsAutoString aReplaceWord(replaceWord);
  if (mEditor && mSpellChecker)
  {
    result = mSpellChecker->Replace(&aMisspelledWord, &aReplaceWord, allOccurrences);
  }
  return result;
}

NS_IMETHODIMP    
nsEditorShell::IgnoreWordAllOccurrences(const PRUnichar *word)
{
  nsresult  result = NS_NOINTERFACE;
  nsAutoString aWord(word);
  if (mEditor && mSpellChecker)
  {
    result = mSpellChecker->IgnoreAll(&aWord);
  }
  return result;
}

NS_IMETHODIMP    
nsEditorShell::AddWordToDictionary(const PRUnichar *word)
{
  nsresult  result = NS_NOINTERFACE;
  nsAutoString aWord(word);
  if (mEditor && mSpellChecker)
  {
    result = mSpellChecker->AddWordToPersonalDictionary(&aWord);
  }
  return result;
}

NS_IMETHODIMP    
nsEditorShell::RemoveWordFromDictionary(const PRUnichar *word)
{
  nsresult  result = NS_NOINTERFACE;
  nsAutoString aWord(word);
  if (mEditor && mSpellChecker)
  {
    result = mSpellChecker->RemoveWordFromPersonalDictionary(&aWord);
  }
  return result;
}

NS_IMETHODIMP    
nsEditorShell::GetPersonalDictionaryWord(const PRUnichar *word, PRUnichar **suggestedWord)
{
  nsresult  result = NS_NOINTERFACE;
  if (mEditor && mSpellChecker)
  {
    printf("GetPersonalDictionaryWord NOT IMPLEMENTED\n");
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
      {
        nsCOMPtr<nsITextEditor>  textEditor = do_QueryInterface(mEditor);
        if (textEditor)
          err = textEditor->BeginTransaction();
      }
      break;
    case eHTMLTextEditorType:
      {
        nsCOMPtr<nsIHTMLEditor>  htmlEditor = do_QueryInterface(mEditor);
        if (htmlEditor)
          err = htmlEditor->BeginTransaction();
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
      {
        nsCOMPtr<nsITextEditor>  textEditor = do_QueryInterface(mEditor);
        if (textEditor)
          err = textEditor->EndTransaction();
      }
      break;
    case eHTMLTextEditorType:
      {
        nsCOMPtr<nsIHTMLEditor>  htmlEditor = do_QueryInterface(mEditor);
        if (htmlEditor)
          err = htmlEditor->EndTransaction();
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
    delete [] str;
  }

}
#endif

NS_IMETHODIMP    
nsEditorShell::ExecuteScript(nsIScriptContext * aContext, const nsString& aScript)
{
  if (nsnull != aContext) {
    const char* url = "";
    PRBool isUndefined = PR_FALSE;
    nsString rVal;

#ifdef APP_DEBUG
    char* script_str = aScript.ToNewCString();
    printf("Executing [%s]\n", script_str);
    delete[] script_str;
#endif

    aContext->EvaluateString(aScript, url, 0, rVal, &isUndefined);
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

#if 1

  switch (mEditorType)
  {
    case ePlainTextEditorType:
      {
        nsCOMPtr<nsITextEditor>  textEditor = do_QueryInterface(mEditor);
        if (textEditor)
          err = textEditor->StartLogging(logFile);
      }
      break;
    case eHTMLTextEditorType:
      {
        nsCOMPtr<nsIHTMLEditor>  htmlEditor = do_QueryInterface(mEditor);
        if (htmlEditor)
          err = htmlEditor->StartLogging(logFile);
      }
      break;
    default:
      err = NS_ERROR_NOT_IMPLEMENTED;
  }

#endif

  return err;
}

NS_IMETHODIMP
nsEditorShell::StopLogging()
{
  nsresult  err = NS_OK;

#if 1

  switch (mEditorType)
  {
    case ePlainTextEditorType:
      {
        nsCOMPtr<nsITextEditor>  textEditor = do_QueryInterface(mEditor);
        if (textEditor)
          err = textEditor->StopLogging();
      }
      break;
    case eHTMLTextEditorType:
      {
        nsCOMPtr<nsIHTMLEditor>  htmlEditor = do_QueryInterface(mEditor);
        if (htmlEditor)
          err = htmlEditor->StopLogging();
      }
      break;
    default:
      err = NS_ERROR_NOT_IMPLEMENTED;
  }

#endif

  return err;
}

#ifdef XP_MAC
#pragma mark -
#endif

// nsIDocumentLoaderObserver methods
NS_IMETHODIMP
nsEditorShell::OnStartDocumentLoad(nsIDocumentLoader* loader, nsIURL* aURL, const char* aCommand)
{
   return NS_OK;
}

NS_IMETHODIMP
nsEditorShell::OnEndDocumentLoad(nsIDocumentLoader* loader, nsIURL *aUrl, PRInt32 aStatus,
								 nsIDocumentLoaderObserver * aObserver)
{
   return PrepareDocumentForEditing();
}

NS_IMETHODIMP
nsEditorShell::OnStartURLLoad(nsIDocumentLoader* loader, 
                                 nsIURL* aURL, const char* aContentType,
                                 nsIContentViewer* aViewer)
{

   return NS_OK;
}

NS_IMETHODIMP
nsEditorShell::OnProgressURLLoad(nsIDocumentLoader* loader, 
                                    nsIURL* aURL, PRUint32 aProgress, 
                                    PRUint32 aProgressMax)
{
  return NS_OK;
}

NS_IMETHODIMP
nsEditorShell::OnStatusURLLoad(nsIDocumentLoader* loader, 
                                  nsIURL* aURL, nsString& aMsg)
{
  return NS_OK;
}

NS_IMETHODIMP
nsEditorShell::OnEndURLLoad(nsIDocumentLoader* loader, 
                               nsIURL* aURL, PRInt32 aStatus)
{
   return NS_OK;
}

NS_IMETHODIMP
nsEditorShell::HandleUnknownContentType(nsIDocumentLoader* loader, 
                                           nsIURL *aURL,
                                           const char *aContentType,
                                           const char *aCommand )
{
   return NS_OK;
}
