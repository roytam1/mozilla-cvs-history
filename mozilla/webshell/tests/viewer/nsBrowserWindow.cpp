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

#include "nsIPref.h" 
#include "prmem.h"

#ifdef XP_MAC
#include "nsBrowserWindow.h"
#define NS_IMPL_IDS
#else
#define NS_IMPL_IDS
#include "nsBrowserWindow.h"
#endif
#include "nsIStreamListener.h"
#include "nsIAppShell.h"
#include "nsIWidget.h"
#include "nsITextWidget.h"
#include "nsIButton.h"
#include "nsIImageGroup.h"
#include "nsITimer.h"
#include "nsIDOMDocument.h"
#include "nsIURL.h"
#include "nsIFileWidget.h"
#include "nsILookAndFeel.h"
#include "nsIComponentManager.h"
#include "nsIFactory.h"
#include "nsCRT.h"
#include "nsWidgetsCID.h"
#include "nsViewerApp.h"
#include "prprf.h"
#include "nsIComponentManager.h"
#include "nsParserCIID.h"
#include "nsIEnumerator.h"
#include "nsCOMPtr.h"
#include "nsIServiceManager.h"
#include "nsIStringBundle.h"
#include "nsLayoutCID.h"
#include "nsIDocumentViewer.h"
#include "nsIContentViewer.h"
#include "nsIContentViewerFile.h"
#include "nsIPresShell.h"
#include "nsIPresContext.h"
#include "nsIDocument.h"
#include "nsILayoutDebugger.h"
#include "nsThrobber.h"
#include "nsIDocumentLoader.h"
#include "nsIDocShellTreeItem.h"
#include "nsIDocShellTreeNode.h"
#include "nsIWebNavigation.h"
#include "nsIBaseWindow.h"
#include "nsXPIDLString.h"

#include "nsCWebBrowser.h"

#include "nsXIFDTD.h"
#include "nsIParser.h"
#include "nsHTMLContentSinkStream.h"
#include "nsEditorMode.h"

// Needed for "Find" GUI
#include "nsICheckButton.h"
#include "nsILabel.h"
#include "nsWidgetSupport.h"

#include "nsXPBaseWindow.h"

#include "resources.h"

#if defined(WIN32)
#include <windows.h>
#endif

#include <ctype.h> // tolower

// For Copy
#include "nsIDOMSelection.h"

// XXX For font setting below
#include "nsFont.h"
#include "nsUnitConversion.h"
#include "nsIDeviceContext.h"

#if defined(CookieManagement) || defined(SingleSignon) || defined(ClientWallet)
#include "nsIServiceManager.h"
#endif

#ifdef CookieManagement

#include "nsIURL.h"

#endif
#include "nsIIOService.h"
static NS_DEFINE_CID(kIOServiceCID, NS_IOSERVICE_CID);

#if defined(ClientWallet) || defined(SingleSignon)
#include "nsIWalletService.h"
static NS_DEFINE_IID(kIWalletServiceIID, NS_IWALLETSERVICE_IID);
static NS_DEFINE_CID(kWalletServiceCID, NS_WALLETSERVICE_CID);
#endif

#ifdef PURIFY
#include "pure.h"
#endif

#define THROBBING_N

// XXX greasy constants
#ifdef THROBBING_N
#define THROBBER_WIDTH 32
#define THROBBER_HEIGHT 32
#define THROBBER_AT "resource:/res/throbber/anims%02d.gif"
#define THROB_NUM 29
#else
#define THROBBER_WIDTH 42
#define THROBBER_HEIGHT 42
#define THROBBER_AT "resource:/res/throbber/LargeAnimation%02d.gif"
#define THROB_NUM 38
#endif
#define BUTTON_WIDTH 90
#define BUTTON_HEIGHT THROBBER_HEIGHT

#ifdef INSET_WEBSHELL
#define WEBSHELL_LEFT_INSET 5
#define WEBSHELL_RIGHT_INSET 5
#define WEBSHELL_TOP_INSET 5
#define WEBSHELL_BOTTOM_INSET 5
#else
#define WEBSHELL_LEFT_INSET 0
#define WEBSHELL_RIGHT_INSET 0
#define WEBSHELL_TOP_INSET 0
#define WEBSHELL_BOTTOM_INSET 0
#endif

static NS_DEFINE_CID(kLookAndFeelCID, NS_LOOKANDFEEL_CID);
static NS_DEFINE_CID(kBrowserWindowCID, NS_BROWSER_WINDOW_CID);
static NS_DEFINE_CID(kButtonCID, NS_BUTTON_CID);
static NS_DEFINE_CID(kFileWidgetCID, NS_FILEWIDGET_CID);
static NS_DEFINE_CID(kTextFieldCID, NS_TEXTFIELD_CID);
static NS_DEFINE_CID(kWebShellCID, NS_WEB_SHELL_CID);
static NS_DEFINE_CID(kWindowCID, NS_WINDOW_CID);
static NS_DEFINE_CID(kDialogCID, NS_DIALOG_CID);
static NS_DEFINE_CID(kCheckButtonCID, NS_CHECKBUTTON_CID);
static NS_DEFINE_CID(kRadioButtonCID, NS_RADIOBUTTON_CID);
static NS_DEFINE_CID(kLabelCID, NS_LABEL_CID);

static NS_DEFINE_IID(kIXPBaseWindowIID, NS_IXPBASE_WINDOW_IID);
static NS_DEFINE_IID(kILookAndFeelIID, NS_ILOOKANDFEEL_IID);
static NS_DEFINE_IID(kIBrowserWindowIID, NS_IBROWSER_WINDOW_IID);
static NS_DEFINE_IID(kIButtonIID, NS_IBUTTON_IID);
static NS_DEFINE_IID(kIDOMDocumentIID, NS_IDOMDOCUMENT_IID);
static NS_DEFINE_IID(kIFactoryIID, NS_IFACTORY_IID);
static NS_DEFINE_IID(kIFileWidgetIID, NS_IFILEWIDGET_IID);
static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);
static NS_DEFINE_IID(kITextWidgetIID, NS_ITEXTWIDGET_IID);
static NS_DEFINE_IID(kIWebShellContainerIID, NS_IWEB_SHELL_CONTAINER_IID);
static NS_DEFINE_IID(kIWidgetIID, NS_IWIDGET_IID);
static NS_DEFINE_IID(kICheckButtonIID, NS_ICHECKBUTTON_IID);
static NS_DEFINE_IID(kILabelIID, NS_ILABEL_IID);
static NS_DEFINE_IID(kIPromptIID,         NS_IPROMPT_IID);
static NS_DEFINE_IID(kIDocumentViewerIID, NS_IDOCUMENT_VIEWER_IID);
static NS_DEFINE_CID(kXPBaseWindowCID, NS_XPBASE_WINDOW_CID);
static NS_DEFINE_IID(kIStringBundleServiceIID, NS_ISTRINGBUNDLESERVICE_IID);

static NS_DEFINE_CID(kStringBundleServiceCID, NS_STRINGBUNDLESERVICE_CID);

static NS_DEFINE_IID(kILayoutDebuggerIID, NS_ILAYOUT_DEBUGGER_IID);
static NS_DEFINE_CID(kLayoutDebuggerCID, NS_LAYOUT_DEBUGGER_CID);

static NS_DEFINE_IID(kIDocumentLoaderObserverIID, NS_IDOCUMENTLOADEROBSERVER_IID);

#define FILE_PROTOCOL "file://"

#ifdef USE_LOCAL_WIDGETS
  extern nsresult NS_NewButton(nsIButton** aControl);
  extern nsresult NS_NewLabel(nsILabel** aControl);
  extern nsresult NS_NewTextWidget(nsITextWidget** aControl);
  extern nsresult NS_NewCheckButton(nsICheckButton** aControl);
#endif

// prototypes
#if 0
static nsEventStatus PR_CALLBACK HandleEvent(nsGUIEvent *aEvent);
#endif
static void* GetItemsNativeData(nsISupports* aObject);

//----------------------------------------------------------------------

static
nsIPresShell*
GetPresShellFor(nsIDocShell* aDocShell)
{
  nsIPresShell* shell = nsnull;
  if (nsnull != aDocShell) {
    nsIContentViewer* cv = nsnull;
    aDocShell->GetContentViewer(&cv);
    if (nsnull != cv) {
      nsIDocumentViewer* docv = nsnull;
      cv->QueryInterface(kIDocumentViewerIID, (void**) &docv);
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

nsVoidArray nsBrowserWindow::gBrowsers;

nsBrowserWindow*
nsBrowserWindow::FindBrowserFor(nsIWidget* aWidget, PRIntn aWhich)
{
  nsIWidget*        widget;
  nsBrowserWindow*  result = nsnull;

  PRInt32 i, n = gBrowsers.Count();
  for (i = 0; i < n; i++) {
    nsBrowserWindow* bw = (nsBrowserWindow*) gBrowsers.ElementAt(i);
    if (nsnull != bw) {
      switch (aWhich) {
      case FIND_WINDOW:
        if (bw->mWindow) {
          bw->mWindow->QueryInterface(kIWidgetIID, (void**) &widget);
          if (widget == aWidget) {
            result = bw;
          }
          NS_IF_RELEASE(widget);
        }
        break;
      case FIND_BACK:
        if (bw->mBack) {
          bw->mBack->QueryInterface(kIWidgetIID, (void**) &widget);
          if (widget == aWidget) {
            result = bw;
          }
          NS_IF_RELEASE(widget);
        }
        break;
      case FIND_FORWARD:
        if (bw->mForward) {
          bw->mForward->QueryInterface(kIWidgetIID, (void**) &widget);
          if (widget == aWidget) {
            result = bw;
          }
          NS_IF_RELEASE(widget);
        }
        break;
      case FIND_LOCATION:
        if (bw->mLocation) {
          bw->mLocation->QueryInterface(kIWidgetIID, (void**) &widget);
          if (widget == aWidget) {
            result = bw;
          }
          NS_IF_RELEASE(widget);
        }
        break;
      }
    }
  }
  if (nsnull != result) {
    NS_ADDREF(result);
  }
  return result;
}

void
nsBrowserWindow::AddBrowser(nsBrowserWindow* aBrowser)
{
  gBrowsers.AppendElement(aBrowser);
  NS_ADDREF(aBrowser);
}

void
nsBrowserWindow::RemoveBrowser(nsBrowserWindow* aBrowser)
{
  //nsViewerApp* app = aBrowser->mApp;
  gBrowsers.RemoveElement(aBrowser);
  NS_RELEASE(aBrowser);
}

void
nsBrowserWindow::CloseAllWindows()
{
  while (0 != gBrowsers.Count()) {
    nsBrowserWindow* bw = (nsBrowserWindow*) gBrowsers.ElementAt(0);
    NS_ADDREF(bw);
    bw->Close();
    NS_RELEASE(bw);
  }
  gBrowsers.Clear();
}

static nsEventStatus PR_CALLBACK
HandleBrowserEvent(nsGUIEvent *aEvent)
{ 
  nsEventStatus result = nsEventStatus_eIgnore;
  nsBrowserWindow* bw =
    nsBrowserWindow::FindBrowserFor(aEvent->widget, FIND_WINDOW);
  if (nsnull != bw) {
    nsSizeEvent* sizeEvent;
    switch(aEvent->message) {
    case NS_SIZE:
      sizeEvent = (nsSizeEvent*)aEvent;  
      bw->Layout(sizeEvent->windowSize->width,
                 sizeEvent->windowSize->height);
      result = nsEventStatus_eConsumeNoDefault;
      break;

    case NS_XUL_CLOSE:
    case NS_DESTROY:
    {
      nsViewerApp* app = bw->mApp;
      app->CloseWindow(bw);
      result = nsEventStatus_eConsumeDoDefault;

#ifndef XP_MAC
      // XXX Really shouldn't just exit, we should just notify somebody...
      if (0 == nsBrowserWindow::gBrowsers.Count()) {
        app->Exit();
      }
#endif
    }
    return result;

    case NS_MENU_SELECTED:
      result = bw->DispatchMenuItem(((nsMenuEvent*)aEvent)->mCommand);
      break;

    default:
      break;
    }
    NS_RELEASE(bw);
  }
  return result;
}

static nsEventStatus PR_CALLBACK
HandleBackEvent(nsGUIEvent *aEvent)
{
  nsEventStatus result = nsEventStatus_eIgnore;
  nsBrowserWindow* bw =
    nsBrowserWindow::FindBrowserFor(aEvent->widget, FIND_BACK);
  if (nsnull != bw) {
    switch(aEvent->message) {
    case NS_MOUSE_LEFT_BUTTON_UP:
      bw->Back();
      break;
    }
    NS_RELEASE(bw);
  }
  return result;
}

static nsEventStatus PR_CALLBACK
HandleForwardEvent(nsGUIEvent *aEvent)
{
  nsEventStatus result = nsEventStatus_eIgnore;
  nsBrowserWindow* bw =
    nsBrowserWindow::FindBrowserFor(aEvent->widget, FIND_FORWARD);
  if (nsnull != bw) {
    switch(aEvent->message) {
    case NS_MOUSE_LEFT_BUTTON_UP:
      bw->Forward();
      break;
    }
    NS_RELEASE(bw);
  }
  return result;
}

static nsEventStatus PR_CALLBACK
HandleLocationEvent(nsGUIEvent *aEvent)
{
  nsEventStatus result = nsEventStatus_eIgnore;
  nsBrowserWindow* bw =
    nsBrowserWindow::FindBrowserFor(aEvent->widget, FIND_LOCATION);
  if (nsnull != bw) {
    switch (aEvent->message) {
    case NS_KEY_UP:
      if (NS_VK_RETURN == ((nsKeyEvent*)aEvent)->keyCode) {
        nsAutoString text;
        PRUint32 size;
        bw->mLocation->GetText(text, 1000, size);
        bw->GoTo(text.GetUnicode());
      }
      break;

    case NS_DRAGDROP_EVENT: {
      /*printf("Drag & Drop Event\n");
      nsDragDropEvent * ev = (nsDragDropEvent *)aEvent;
      nsAutoString fileURL;
      BuildFileURL(ev->mURL, fileURL);
      nsAutoString fileName(ev->mURL);
      char * str = fileName.ToNewCString();

      PRInt32 len = strlen(str);
      PRInt32 sum = len + sizeof(FILE_PROTOCOL);
      char* lpszFileURL = new char[sum];
  
      // Translate '\' to '/'
      for (PRInt32 i = 0; i < len; i++) {
        if (str[i] == '\\') {
          str[i] = '/';
        }
      }

      // Build the file URL
      PR_snprintf(lpszFileURL, sum, "%s%s", FILE_PROTOCOL, str);

      // Ask the Web widget to load the file URL
      nsString urlStr(lpszFileURL);
      const PRUnichar * uniStr = fileURL.GetUnicode();
      bw->GoTo(uniStr);
      //delete [] lpszFileURL;
      //delete [] str;*/
      } break;

    default:
      break;
    }
    NS_RELEASE(bw);
  }
  return result;
}

#ifdef PURIFY
static void
DispatchPurifyEvent(PRInt32 aID)
{
  if (!PurifyIsRunning()) {
      printf("!!! Re-run viewer under Purify to use this menu item.\n");
      fflush(stdout);
      return;
  }

  switch (aID) {
  case VIEWER_PURIFY_SHOW_NEW_LEAKS:
    PurifyPrintf("viewer: new leaks");
    PurifyNewLeaks();
    break;
  case VIEWER_PURIFY_SHOW_ALL_LEAKS:
    PurifyPrintf("viewer: all leaks");
    PurifyAllLeaks();
    break;
  case VIEWER_PURIFY_CLEAR_ALL_LEAKS:
    PurifyPrintf("viewer: clear leaks");
    PurifyClearLeaks();
    break;
  case VIEWER_PURIFY_SHOW_ALL_HANDLES_IN_USE:
    PurifyPrintf("viewer: all handles");
    PurifyAllHandlesInuse();
    break;
  case VIEWER_PURIFY_SHOW_NEW_IN_USE:
    PurifyPrintf("viewer: new in-use");
    PurifyNewInuse();
    break;
  case VIEWER_PURIFY_SHOW_ALL_IN_USE:
    PurifyPrintf("viewer: all in-use");
    PurifyAllInuse();
    break;
  case VIEWER_PURIFY_CLEAR_ALL_IN_USE:
    PurifyPrintf("viewer: clear in-use");
    PurifyClearInuse();
    break;
  case VIEWER_PURIFY_HEAP_VALIDATE:
    PurifyPrintf("viewer: heap validate");
    PurifyHeapValidate(PURIFY_HEAP_ALL, PURIFY_HEAP_BLOCKS_ALL, NULL);
    break;
  }
}
#endif

nsEventStatus
nsBrowserWindow::DispatchMenuItem(PRInt32 aID)
{
#if defined(CookieManagement) || defined(SingleSignon) || defined(ClientWallet)
  nsresult res;
#if defined(ClientWallet) || defined(SingleSignon)
  nsIWalletService *walletservice;
#endif
#ifdef ClientWallet
#define WALLET_EDITOR_URL "file:///y|/walleted.html"
  nsAutoString urlString(WALLET_EDITOR_URL);
#endif
#endif

  nsEventStatus result;
#ifdef NS_DEBUG
  result = DispatchDebugMenu(aID);
  if (nsEventStatus_eIgnore != result) {
    return result;
  }
#endif
  result = DispatchStyleMenu(aID);
  if (nsEventStatus_eIgnore != result) {
    return result;
  }

  switch (aID) {
  case VIEWER_EXIT:
    mApp->Exit();
    return nsEventStatus_eConsumeNoDefault;

  case VIEWER_WINDOW_OPEN:
    mApp->OpenWindow();
    break;
  
  case VIEWER_FILE_OPEN:
    DoFileOpen();
    break;

  case VIEW_SOURCE:
    {
      PRInt32 theIndex;
      nsCOMPtr<nsIWebShell> webShell(do_QueryInterface(mDocShell));
      webShell->GetHistoryIndex(theIndex);
      const PRUnichar* theURL;
      webShell->GetURL(theIndex,&theURL);
      nsAutoString theString(theURL);
      mApp->ViewSource(theString);
      //XXX Find out how the string is allocated, and perhaps delete it...
    }
    break;
  
  case VIEWER_EDIT_COPY:
    DoCopy();
    break;

  case VIEWER_EDIT_PASTE:
    DoPaste();
    break;

  case VIEWER_EDIT_FINDINPAGE:
    DoFind();
    break;

  case VIEWER_DEMO0:
  case VIEWER_DEMO1:
  case VIEWER_DEMO2:
  case VIEWER_DEMO3:
  case VIEWER_DEMO4:
  case VIEWER_DEMO5:
  case VIEWER_DEMO6:
  case VIEWER_DEMO7:
  case VIEWER_DEMO8: 
  case VIEWER_DEMO9: 
  case VIEWER_DEMO10: 
  case VIEWER_DEMO11: 
  case VIEWER_DEMO12: 
  case VIEWER_DEMO13:
  case VIEWER_DEMO14:
  case VIEWER_DEMO15:
  case VIEWER_DEMO16:
  case VIEWER_DEMO17:
    {
      PRIntn ix = aID - VIEWER_DEMO0;
      nsAutoString url(SAMPLES_BASE_URL);
      url.Append("/test");
      url.Append(ix, 10);
      url.Append(".html");
      nsCOMPtr<nsIWebShell> webShell(do_QueryInterface(mDocShell));
      webShell->LoadURL(url.GetUnicode());
    }
    break;

  case VIEWER_XPTOOLKITTOOLBAR1:
    {
      nsAutoString url(SAMPLES_BASE_URL);
      url.Append("/toolbarTest1.xul");
      nsCOMPtr<nsIWebShell> webShell(do_QueryInterface(mDocShell));
      webShell->LoadURL(url.GetUnicode());
      break;
    }
  case VIEWER_XPTOOLKITTREE1:
    {
      nsAutoString url(SAMPLES_BASE_URL);
      url.Append("/treeTest1.xul");
      nsCOMPtr<nsIWebShell> webShell(do_QueryInterface(mDocShell));
      webShell->LoadURL(url.GetUnicode());
      break;
    }
  
  case JS_CONSOLE:
    DoJSConsole();
    break;

  case VIEWER_PREFS:
    DoPrefs();
    break;

  case EDITOR_MODE:
    DoEditorMode(mDocShell);
    break;

  case VIEWER_ONE_COLUMN:
  case VIEWER_TWO_COLUMN:
  case VIEWER_THREE_COLUMN:
    ShowPrintPreview(aID);
    break;

  case VIEWER_PRINT:
    DoPrint();
    break;

  case VIEWER_PRINT_SETUP:
    DoPrintSetup();
    break;

  case VIEWER_TABLE_INSPECTOR:
    DoTableInspector();
    break;

  case VIEWER_IMAGE_INSPECTOR:
    DoImageInspector();
    break;

#ifdef PURIFY
  case VIEWER_PURIFY_SHOW_NEW_LEAKS:
  case VIEWER_PURIFY_SHOW_ALL_LEAKS:
  case VIEWER_PURIFY_CLEAR_ALL_LEAKS:
  case VIEWER_PURIFY_SHOW_ALL_HANDLES_IN_USE:
  case VIEWER_PURIFY_SHOW_NEW_IN_USE:
  case VIEWER_PURIFY_SHOW_ALL_IN_USE:
  case VIEWER_PURIFY_CLEAR_ALL_IN_USE:
  case VIEWER_PURIFY_HEAP_VALIDATE:
    DispatchPurifyEvent(aID);
    break;
#endif

  case VIEWER_ZOOM_500:
  case VIEWER_ZOOM_300:
  case VIEWER_ZOOM_200:
  case VIEWER_ZOOM_100:
  case VIEWER_ZOOM_070:
  case VIEWER_ZOOM_050:
  case VIEWER_ZOOM_030:
  case VIEWER_ZOOM_020:
    mDocShell->SetZoom((aID - VIEWER_ZOOM_BASE) / 10.0f);
    break;

#ifdef ClientWallet
  case PRVCY_PREFILL:
  case PRVCY_QPREFILL:
  nsIPresShell* shell;
  shell = nsnull;
  shell = GetPresShell();
  res = nsServiceManager::GetService(kWalletServiceCID,
                                     kIWalletServiceIID,
                                     (nsISupports **)&walletservice);
  if ((NS_OK == res) && (nsnull != walletservice)) {
    nsString urlString2 = nsString("");
    res = walletservice->WALLET_Prefill(shell, (PRVCY_QPREFILL == aID));
    NS_RELEASE(walletservice);
  }

#ifndef HTMLDialogs 
  if (aID == PRVCY_PREFILL) {
    nsAutoString url("file:///y|/htmldlgs.htm");
    nsIBrowserWindow* bw = nsnull;
    mApp->OpenWindow(PRUint32(~0), bw);
    bw->Show();
    ((nsBrowserWindow *)bw)->GoTo(url.GetUnicode());
    NS_RELEASE(bw);
  }
#endif
  break;

  case PRVCY_DISPLAY_WALLET:
  {


  /* set a cookie for the javascript wallet editor */
  res = nsServiceManager::GetService(kWalletServiceCID,
                                     kIWalletServiceIID,
                                     (nsISupports **)&walletservice);
  if ((NS_OK == res) && (nsnull != walletservice)) {
    nsIURI * url;
    NS_WITH_SERVICE(nsIIOService, service, kIOServiceCID, &res);
    if (NS_FAILED(res)) return nsEventStatus_eIgnore;

    nsIURI *uri = nsnull;
    res = service->NewURI(WALLET_EDITOR_URL, nsnull, &uri);
    if (NS_FAILED(res)) return nsEventStatus_eIgnore;

    res = uri->QueryInterface(NS_GET_IID(nsIURI), (void**)&url);
    NS_RELEASE(uri);
    if (!NS_FAILED(res)) {
//      res = walletservice->WALLET_PreEdit(url);
      NS_RELEASE(walletservice);
    }
  }

  /* invoke the javascript wallet editor */
  nsCOMPtr<nsIWebShell> webShell(do_QueryInterface(mDocShell));
  webShell->LoadURL(urlString.GetUnicode());
  }
  break;
#endif

#if defined(CookieManagement)
  case PRVCY_DISPLAY_COOKIES:
  {
      break;
  }
#endif

#if defined(SingleSignon)
  case PRVCY_DISPLAY_SIGNONS:
  res = nsServiceManager::GetService(kWalletServiceCID,
                                     kIWalletServiceIID,
                                     (nsISupports **)&walletservice);
  if ((NS_OK == res) && (nsnull != walletservice)) {
//    res = walletservice->SI_DisplaySignonInfoAsHTML();
    NS_RELEASE(walletservice);
  }
  break;
#endif

  }

  // Any menu IDs that the editor uses will be processed here
  DoEditorTest(mDocShell, aID);

  return nsEventStatus_eIgnore;
}

void
nsBrowserWindow::Back()
{
  nsCOMPtr<nsIWebNavigation> webNav(do_QueryInterface(mWebBrowser));
  webNav->GoBack();
}

void
nsBrowserWindow::Forward()
{
  nsCOMPtr<nsIWebNavigation> webNav(do_QueryInterface(mWebBrowser));
  webNav->GoForward();
}

void
nsBrowserWindow::GoTo(const PRUnichar* aURL,const char* aCommand)
{
  nsCOMPtr<nsIWebShell> webShell(do_QueryInterface(mDocShell));
  webShell->LoadURL(aURL, aCommand, nsnull);
}


static PRBool GetFileFromFileSelector(nsIWidget* aParentWindow,
                                      nsFileSpec& aFileSpec,
                                      nsFileSpec& aDisplayDirectory)
{
  PRBool selectedFileName = PR_FALSE;
  nsIFileWidget *fileWidget;
  nsString title("Open HTML");
  nsresult rv = nsComponentManager::CreateInstance(kFileWidgetCID,
                                                   nsnull,
                                                   kIFileWidgetIID,
                                                   (void**)&fileWidget);
  if (NS_OK == rv) {
    nsString titles[] = {"All Readable Files", "HTML Files",
                         "XML Files", "Image Files", "All Files"};
    nsString filters[] = {"*.htm; *.html; *.xml; *.gif; *.jpg; *.jpeg; *.png",
                          "*.htm; *.html",
                          "*.xml",
                          "*.gif; *.jpg; *.jpeg; *.png",
                          "*.*"};
    fileWidget->SetFilterList(5, titles, filters);

    fileWidget->SetDisplayDirectory(aDisplayDirectory);
    fileWidget->Create(aParentWindow,
                       title,
                       eMode_load,
                       nsnull,
                       nsnull);

    PRUint32 result = fileWidget->Show();
    if (result) {
      fileWidget->GetFile(aFileSpec);
      selectedFileName = PR_TRUE;
    }
 
    fileWidget->GetDisplayDirectory(aDisplayDirectory);
    NS_RELEASE(fileWidget);
  }

  return selectedFileName;
}

void
nsBrowserWindow::DoFileOpen()
{
  nsFileSpec fileSpec;
  if (GetFileFromFileSelector(mWindow, fileSpec, mOpenFileDirectory)) {
    nsFileURL fileURL(fileSpec);
    // Ask the Web widget to load the file URL
    nsCOMPtr<nsIWebShell> webShell(do_QueryInterface(mDocShell));
    webShell->LoadURL(nsString(fileURL.GetURLString()).GetUnicode());
    Show();
  }
}

#define DIALOG_FONT      "Helvetica"
#define DIALOG_FONT_SIZE 10

/**--------------------------------------------------------------------------------
 * Main Handler
 *--------------------------------------------------------------------------------
 */
#if 0
nsEventStatus PR_CALLBACK HandleEvent(nsGUIEvent *aEvent)
{
  //printf("HandleEvent aEvent->message %d\n", aEvent->message);
  nsEventStatus result = nsEventStatus_eIgnore;
  if (aEvent == nsnull ||  aEvent->widget == nsnull) {
    return result;
  }

  if (aEvent->message == 301 || aEvent->message == 302) {
    //int x = 0;
  }

  void * data;
  aEvent->widget->GetClientData(data);

  if (data == nsnull) {
    nsIWidget * parent = aEvent->widget->GetParent();
    if (parent != nsnull) {
      parent->GetClientData(data);
      NS_RELEASE(parent);
    }
  }
  
  if (data != nsnull) {
    nsBrowserWindow * browserWindow = (nsBrowserWindow *)data;
    result = browserWindow->ProcessDialogEvent(aEvent);
  }

  return result;
}
#endif

static void* GetItemsNativeData(nsISupports* aObject)
{
  void*                   result = nsnull;
  nsIWidget*      widget;
  if (NS_OK == aObject->QueryInterface(kIWidgetIID,(void**)&widget))
  {
    result = widget->GetNativeData(NS_NATIVE_WIDGET);
    NS_RELEASE(widget);
  }
  return result;
}

//---------------------------------------------------------------
NS_IMETHODIMP nsBrowserWindow::FindNext(const nsString &aSearchStr, PRBool aMatchCase, PRBool aSearchDown, PRBool &aIsFound)
{
  nsIPresShell* shell = GetPresShell();
  if (nsnull != shell) {
    nsCOMPtr<nsIDocument> doc;
    shell->GetDocument(getter_AddRefs(doc));
    if (doc) {
      //PRBool foundIt = PR_FALSE;
      doc->FindNext(aSearchStr, aMatchCase, aSearchDown, aIsFound);
      if (!aIsFound) {
        // Display Dialog here
      }
      ForceRefresh();
    }
    NS_RELEASE(shell);
  }
  return NS_OK;
}

//---------------------------------------------------------------
NS_IMETHODIMP nsBrowserWindow::ForceRefresh()
{
  nsIPresShell* shell = GetPresShell();
  if (nsnull != shell) {
    nsCOMPtr<nsIViewManager> vm;
    shell->GetViewManager(getter_AddRefs(vm));
    if (vm) {
      nsIView* root;
      vm->GetRootView(root);
      if (nsnull != root) {
        vm->UpdateView(root, NS_VMREFRESH_IMMEDIATE);
      }
    }
    NS_RELEASE(shell);
  }
  return NS_OK;
}


/**--------------------------------------------------------------------------------
 * Main Handler
 *--------------------------------------------------------------------------------
 */

 
 
nsEventStatus nsBrowserWindow::ProcessDialogEvent(nsGUIEvent *aEvent)
{ 
  nsEventStatus result = nsEventStatus_eIgnore;

  //printf("aEvent->message %d\n", aEvent->message);
  switch(aEvent->message) {

    case NS_KEY_DOWN: {
    } break;

    case NS_MOUSE_LEFT_BUTTON_UP: {
    } break;

    case NS_PAINT: 
      break;
    default:
      result = nsEventStatus_eIgnore;
  }
    //printf("result: %d = %d\n", result, PR_FALSE);

    return result;
}

void
nsBrowserWindow::DoFind()
{
}


//----------------------------------------------------------------------

#define VIEWER_BUNDLE_URL "resource:/res/viewer.properties"

static nsString* gTitleSuffix = nsnull;

#if XXX
static nsString*
GetTitleSuffix(void)
{
  nsString* suffix = new nsString(" - Failed");
  nsIStringBundleService* service = nsnull;
  nsresult ret = nsServiceManager::GetService(kStringBundleServiceCID,
    kIStringBundleServiceIID, (nsISupports**) &service);
  if (NS_FAILED(ret)) {
    return suffix;
  }
  nsIURI* url = nsnull;
    NS_WITH_SERVICE(nsIIOService, service, kIOServiceCID, &ret);
    if (NS_FAILED(ret)) return ret;

    nsIURI *uri = nsnull;
    ret = service->NewURI(VIEWER_BUNDLE_URL, nsnull, &uri);
    if (NS_FAILED(ret)) return ret;

    ret = uri->QueryInterface(NS_GET_IID(nsIURI), (void**)&url);
    NS_RELEASE(uri);
  if (NS_FAILED(ret)) {
    NS_RELEASE(service);
    return suffix;
  }
  nsILocale* locale = nsnull;
  nsIStringBundle* bundle = nsnull;
  ret = service->CreateBundle(url, locale, &bundle);
  NS_RELEASE(url);
  if (NS_FAILED(ret)) {
    NS_RELEASE(service);
    return suffix;
  }
  ret = bundle->GetStringFromID(1, *suffix);
  NS_RELEASE(bundle);
  NS_RELEASE(service);

  return suffix;
}
#endif

// Note: operator new zeros our memory
nsBrowserWindow::nsBrowserWindow()
{
  NS_INIT_REFCNT();
  if (!gTitleSuffix) {
#if XXX
    gTitleSuffix = GetTitleSuffix();
#endif
    gTitleSuffix = new nsString(" - Raptor");
  }
  AddBrowser(this);
}

nsBrowserWindow::~nsBrowserWindow()
{
  NS_IF_RELEASE(mAppShell);
  NS_IF_RELEASE(mTableInspectorDialog);
  NS_IF_RELEASE(mImageInspectorDialog);
  NS_IF_RELEASE(mWebCrawler);

  if (nsnull != mTableInspector) {
    delete mTableInspector;
  }
  if (nsnull != mImageInspector) {
    delete mImageInspector;
  }
  if(mWebBrowserChrome)
   {
   mWebBrowserChrome->BrowserWindow(nsnull);
   NS_RELEASE(mWebBrowserChrome);
   }

}

NS_IMPL_ADDREF(nsBrowserWindow)
NS_IMPL_RELEASE(nsBrowserWindow)

nsresult
nsBrowserWindow::QueryInterface(const nsIID& aIID,
                                void** aInstancePtrResult)
{
  NS_PRECONDITION(nsnull != aInstancePtrResult, "null pointer");
  if (nsnull == aInstancePtrResult) {
    return NS_ERROR_NULL_POINTER;
  }

  *aInstancePtrResult = NULL;

  if (aIID.Equals(kIBrowserWindowIID)) {
    *aInstancePtrResult = (void*) ((nsIBrowserWindow*)this);
    NS_ADDREF_THIS();
    return NS_OK;
  }
  if (aIID.Equals(kIDocumentLoaderObserverIID)) {
    *aInstancePtrResult = (void*) ((nsIDocumentLoaderObserver*)this);
    NS_ADDREF_THIS();
    return NS_OK;
  }
  if (aIID.Equals(kIWebShellContainerIID)) {
    *aInstancePtrResult = (void*) ((nsIWebShellContainer*)this);
    NS_ADDREF_THIS();
    return NS_OK;
  }
  if (aIID.Equals(NS_GET_IID(nsIProgressEventSink))) {
    *aInstancePtrResult = (void*) ((nsIProgressEventSink*)this);
    NS_ADDREF_THIS();
    return NS_OK;
  }
  if (aIID.Equals(kIPromptIID)) {
    *aInstancePtrResult = (void*) ((nsIPrompt*)this);
    NS_ADDREF_THIS();
    return NS_OK;
  }
  if (aIID.Equals(kISupportsIID)) {
    *aInstancePtrResult = (void*) ((nsISupports*)((nsIBrowserWindow*)this));
    NS_ADDREF_THIS();
    return NS_OK;
  }
  return NS_NOINTERFACE;
}

nsresult
nsBrowserWindow::Init(nsIAppShell* aAppShell,
                      const nsRect& aBounds,
                      PRUint32 aChromeMask,
                      PRBool aAllowPlugins)
{
  mChromeMask = aChromeMask;
  mAppShell = aAppShell;
  NS_IF_ADDREF(mAppShell);
  mAllowPlugins = aAllowPlugins;

  // Create top level window
  nsresult rv = nsComponentManager::CreateInstance(kWindowCID, nsnull,
                                                   kIWidgetIID,
                                                   (void**)&mWindow);
  if (NS_OK != rv) {
    return rv;
  }
  nsWidgetInitData initData;
  initData.mWindowType = eWindowType_toplevel;
  initData.mBorderStyle = eBorderStyle_default;

  nsRect r(0, 0, aBounds.width, aBounds.height);
  mWindow->Create((nsIWidget*)NULL, r, HandleBrowserEvent,
                  nsnull, aAppShell, nsnull, &initData);
  mWindow->GetClientBounds(r);

  // Create web shell
  mWebBrowser = do_CreateInstance(NS_WEBBROWSER_PROGID, &rv);
/*  rv = nsComponentManager::CreateInstance(kWebShellCID, nsnull,
                                          NS_GET_IID(nsIDocShell),
                                          (void**)&mDocShell);
*/  if (NS_OK != rv) {
    return rv;
  }
  r.x = r.y = 0;
  nsCOMPtr<nsIBaseWindow> webBrowserWin(do_QueryInterface(mWebBrowser));
  rv = webBrowserWin->InitWindow(mWindow->GetNativeData(NS_NATIVE_WIDGET), nsnull, r.x, r.y, r.width, r.height);
  webBrowserWin->Create();
  mWebBrowser->GetDocShell(&mDocShell);
  mDocShell->SetAllowPlugins(aAllowPlugins);
  nsCOMPtr<nsIDocumentLoader> docLoader;
  nsCOMPtr<nsIWebShell> webShell(do_QueryInterface(mDocShell));
  webShell->SetContainer((nsIWebShellContainer*) this);

  nsCOMPtr<nsIDocShellTreeItem> docShellAsItem(do_QueryInterface(mDocShell));
  EnsureWebBrowserChrome();
  docShellAsItem->SetTreeOwner(mWebBrowserChrome);

  webShell->GetDocumentLoader(*getter_AddRefs(docLoader));
  if (docLoader) {
    docLoader->AddObserver(this);
  }
  webBrowserWin->SetVisibility(PR_TRUE);

  if (NS_CHROME_MENU_BAR_ON & aChromeMask) {
    rv = CreateMenuBar(r.width);
    if (NS_OK != rv) {
      return rv;
    }
    mWindow->GetClientBounds(r);
    r.x = r.y = 0;
  }

  if (NS_CHROME_TOOL_BAR_ON & aChromeMask) {
    rv = CreateToolBar(r.width);
    if (NS_OK != rv) {
      return rv;
    }
  }

  if (NS_CHROME_STATUS_BAR_ON & aChromeMask) {
    rv = CreateStatusBar(r.width);
    if (NS_OK != rv) {
      return rv;
    }
  }

  // Give the embedding app a chance to do platforms-specific window setup
  InitNativeWindow();

  // Now lay it all out
  Layout(r.width, r.height);

  return NS_OK;
}

nsresult
nsBrowserWindow::Init(nsIAppShell* aAppShell,
                      const nsRect& aBounds,
                      PRUint32 aChromeMask,
                      PRBool aAllowPlugins,
                      nsIDocumentViewer* aDocumentViewer,
                      nsIPresContext* aPresContext)
{
  mChromeMask = aChromeMask;
  mAppShell = aAppShell;
  NS_IF_ADDREF(mAppShell);
  mAllowPlugins = aAllowPlugins;

  // Create top level window
  nsresult rv = nsComponentManager::CreateInstance(kWindowCID, nsnull,
                                                   kIWidgetIID,
                                                   (void**)&mWindow);
  if (NS_OK != rv) {
    return rv;
  }
  nsRect r(0, 0, aBounds.width, aBounds.height);
  mWindow->Create((nsIWidget*)NULL, r, HandleBrowserEvent,
                  nsnull, aAppShell);
  mWindow->GetClientBounds(r);

  // Create web shell
  rv = nsComponentManager::CreateInstance(kWebShellCID, nsnull,
                                          NS_GET_IID(nsIDocShell),
                                          (void**)&mDocShell);
  if (NS_OK != rv) {
    return rv;
  }
  nsCOMPtr<nsIDocumentLoader> docLoader;
  r.x = r.y = 0;
  //nsRect ws = r;
  nsCOMPtr<nsIWebShell> webShell(do_QueryInterface(mDocShell));
  mDocShell->SetAllowPlugins(aAllowPlugins);
  nsCOMPtr<nsIBaseWindow> webBrowserWin(do_QueryInterface(mWebBrowser));
  rv = webBrowserWin->InitWindow(mWindow->GetNativeData(NS_NATIVE_WIDGET), nsnull, r.x, r.y, r.width, r.height);
  webBrowserWin->Create();
  webShell->SetContainer((nsIWebShellContainer*) this);
  webShell->GetDocumentLoader(*getter_AddRefs(docLoader));
  if (docLoader) {
    docLoader->AddObserver(this);
  }

  if (NS_CHROME_MENU_BAR_ON & aChromeMask) {
    rv = CreateMenuBar(r.width);
    if (NS_OK != rv) {
      return rv;
    }
    mWindow->GetClientBounds(r);
    r.x = r.y = 0;
  }

  if (NS_CHROME_TOOL_BAR_ON & aChromeMask) {
    rv = CreateToolBar(r.width);
    if (NS_OK != rv) {
      return rv;
    }
  }

  if (NS_CHROME_STATUS_BAR_ON & aChromeMask) {
    rv = CreateStatusBar(r.width);
    if (NS_OK != rv) {
      return rv;
    }
  }

  // Give the embedding app a chance to do platforms-specific window setup
  InitNativeWindow();
  
  // Now lay it all out
  Layout(r.width, r.height);

  // Create a document viewer and bind it to the webshell
  nsIDocumentViewer* docv;
  aDocumentViewer->CreateDocumentViewerUsing(aPresContext, docv);
  webShell->Embed(docv, "duh", nsnull);
  webBrowserWin->SetVisibility(PR_TRUE);
  NS_RELEASE(docv);

  return NS_OK;
}

void
nsBrowserWindow::SetWebCrawler(nsWebCrawler* aCrawler)
{
  if (mWebCrawler) {
    if (mDocShell) {
      mDocShell->SetDocLoaderObserver(nsnull);
    }
    NS_RELEASE(mWebCrawler);
  }
  if (aCrawler) {
    mWebCrawler = aCrawler;
    /* Nisheeth: the crawler registers as a document loader observer with
     * the webshell when nsWebCrawler::Start() is called.
    if (mDocShell) {
      mDocShell->SetDocLoaderObserver(aCrawler);
    }
    */
    NS_ADDREF(aCrawler);
  }
}

// XXX This sort of thing should be in a resource
#define TOOL_BAR_FONT      "Helvetica"
#define TOOL_BAR_FONT_SIZE 12
#define STATUS_BAR_FONT      "Helvetica"
#define STATUS_BAR_FONT_SIZE 10

nsresult
nsBrowserWindow::CreateToolBar(PRInt32 aWidth)
{
  nsresult rv;

  nsIDeviceContext* dc = mWindow->GetDeviceContext();
  float t2d;
  dc->GetTwipsToDevUnits(t2d);
  float d2a;
  dc->GetDevUnitsToAppUnits(d2a);
  nsFont font(TOOL_BAR_FONT, NS_FONT_STYLE_NORMAL, NS_FONT_VARIANT_NORMAL,
              NS_FONT_WEIGHT_NORMAL, 0,
              nscoord(NSIntPointsToTwips(TOOL_BAR_FONT_SIZE) * t2d * d2a));
  NS_RELEASE(dc);

#ifdef USE_LOCAL_WIDGETS
  rv = NS_NewButton(&mBack);
#else
  // Create and place back button
  rv = nsComponentManager::CreateInstance(kButtonCID, nsnull, kIButtonIID,
                                          (void**)&mBack);
#endif

  if (NS_OK != rv) {
    return rv;
  }
  nsRect r(0, 0, BUTTON_WIDTH, BUTTON_HEIGHT);

  nsIWidget* widget = nsnull;
  NS_CreateButton(mWindow,mBack,r,HandleBackEvent,&font);
  if (NS_OK == mBack->QueryInterface(kIWidgetIID,(void**)&widget))
  {
    nsAutoString back("Back");
    mBack->SetLabel(back);
    NS_RELEASE(widget);
  }


  // Create and place forward button
  r.SetRect(BUTTON_WIDTH, 0, BUTTON_WIDTH, BUTTON_HEIGHT);
  
#ifdef USE_LOCAL_WIDGETS
  rv = NS_NewButton(&mForward);
#else
  rv = nsComponentManager::CreateInstance(kButtonCID, nsnull, kIButtonIID,
                                          (void**)&mForward);
#endif
  if (NS_OK != rv) {
    return rv;
  }
  if (NS_OK == mForward->QueryInterface(kIWidgetIID,(void**)&widget))
  {
    widget->Create(mWindow, r, HandleForwardEvent, NULL);
    widget->SetFont(font);
    widget->Show(PR_TRUE);
    nsAutoString forward("Forward");
    mForward->SetLabel(forward);
    NS_RELEASE(widget);
  }


  // Create and place location bar
  r.SetRect(2*BUTTON_WIDTH, 0,
            aWidth - 2*BUTTON_WIDTH - THROBBER_WIDTH,
            BUTTON_HEIGHT);

#ifdef USE_LOCAL_WIDGETS
  rv = NS_NewTextWidget(&mLocation);
#else
  rv = nsComponentManager::CreateInstance(kTextFieldCID, nsnull, kITextWidgetIID,
                                          (void**)&mLocation);
#endif

  if (NS_OK != rv) {
    return rv;
  }

  NS_CreateTextWidget(mWindow,mLocation,r,HandleLocationEvent,&font);
  if (NS_OK == mLocation->QueryInterface(kIWidgetIID,(void**)&widget))
  { 
    widget->SetForegroundColor(NS_RGB(0, 0, 0));
    widget->SetBackgroundColor(NS_RGB(255, 255, 255));
    PRUint32 size;
    nsAutoString empty;
    mLocation->SetText(empty, size);
    NS_RELEASE(widget);
  }

  // Create and place throbber
  r.SetRect(aWidth - THROBBER_WIDTH, 0,
            THROBBER_WIDTH, THROBBER_HEIGHT);
  mThrobber = nsThrobber::NewThrobber();
  nsString throbberURL(THROBBER_AT);
  mThrobber->Init(mWindow, r, throbberURL, THROB_NUM);
  mThrobber->Show();
  return NS_OK;
}

// Overload this method in your nsNativeBrowserWindow if you need to 
// have the logic in nsBrowserWindow::Layout() offset the menu within
// the parent window.
nsresult
nsBrowserWindow::GetMenuBarHeight(PRInt32 * aHeightOut)
{
  NS_ASSERTION(nsnull != aHeightOut,"null out param.");

  *aHeightOut = 0;

  return NS_OK;
}

nsresult
nsBrowserWindow::CreateStatusBar(PRInt32 aWidth)
{
  nsresult rv;

  nsIDeviceContext* dc = mWindow->GetDeviceContext();
  float t2d;
  dc->GetTwipsToDevUnits(t2d);
  nsFont font(STATUS_BAR_FONT, NS_FONT_STYLE_NORMAL, NS_FONT_VARIANT_NORMAL,
              NS_FONT_WEIGHT_NORMAL, 0,
              nscoord(t2d * NSIntPointsToTwips(STATUS_BAR_FONT_SIZE)));
  NS_RELEASE(dc);

  nsRect r(0, 0, aWidth, THROBBER_HEIGHT);

#ifdef USE_LOCAL_WIDGETS
  rv = NS_NewTextWidget(&mStatus);
#else
  rv = nsComponentManager::CreateInstance(kTextFieldCID, nsnull, kITextWidgetIID,
                                          (void**)&mStatus);
#endif

  if (NS_OK != rv) {
    return rv;
  }

  nsIWidget* widget = nsnull;
  NS_CreateTextWidget(mWindow,mStatus,r,HandleLocationEvent,&font);
  if (NS_OK == mStatus->QueryInterface(kIWidgetIID,(void**)&widget))
  {
    widget->SetForegroundColor(NS_RGB(0, 0, 0));
    PRUint32 size;
    mStatus->SetText("",size);

    nsITextWidget* textWidget = nsnull;
    if (NS_OK == mStatus->QueryInterface(kITextWidgetIID,(void**)&textWidget))
    {
      PRBool  wasReadOnly;
      textWidget->SetReadOnly(PR_TRUE, wasReadOnly);
      NS_RELEASE(textWidget);
    }

    NS_RELEASE(widget);
  }

  return NS_OK;
}

void
nsBrowserWindow::Layout(PRInt32 aWidth, PRInt32 aHeight)
{
  nscoord txtHeight;
  nscoord menuBarHeight;
  nsILookAndFeel * lookAndFeel;
  if (NS_OK == nsComponentManager::CreateInstance(kLookAndFeelCID, nsnull, kILookAndFeelIID, (void**)&lookAndFeel)) {
    lookAndFeel->GetMetric(nsILookAndFeel::eMetric_TextFieldHeight, txtHeight);
    NS_RELEASE(lookAndFeel);
  } else {
    txtHeight = 24;
  }

  // Find out the menubar height
  GetMenuBarHeight(&menuBarHeight);

  nsRect rr(0, 0, aWidth, aHeight);

  // position location bar (it's stretchy)
  if (NS_CHROME_TOOL_BAR_ON & mChromeMask) {
    nsIWidget* locationWidget = nsnull;
    if (mLocation &&
        NS_SUCCEEDED(mLocation->QueryInterface(kIWidgetIID,
                                               (void**)&locationWidget))) {
      if (mThrobber) {
        PRInt32 width = PR_MAX(aWidth - (2*BUTTON_WIDTH + THROBBER_WIDTH), 0);
      
        locationWidget->Resize(2*BUTTON_WIDTH, menuBarHeight,
                               width,
                               BUTTON_HEIGHT,
                               PR_TRUE);
        mThrobber->MoveTo(aWidth - THROBBER_WIDTH, menuBarHeight);
      }
      else {
        PRInt32 width = PR_MAX(aWidth - 2*BUTTON_WIDTH, 0);
        locationWidget->Resize(2*BUTTON_WIDTH, menuBarHeight,
                               width,
                               BUTTON_HEIGHT,
                               PR_TRUE);
      }

      locationWidget->Show(PR_TRUE);
      NS_RELEASE(locationWidget);

      nsIWidget* w = nsnull;
      if (mBack &&
          NS_SUCCEEDED(mBack->QueryInterface(kIWidgetIID, (void**)&w))) {
        w->Move(0, menuBarHeight);
        w->Show(PR_TRUE);
        NS_RELEASE(w);
      }
      if (mForward &&
          NS_SUCCEEDED(mForward->QueryInterface(kIWidgetIID, (void**)&w))) {
        w->Move(BUTTON_WIDTH, menuBarHeight);
        w->Show(PR_TRUE);
        NS_RELEASE(w);
      }
    }
  }
  else {
    nsIWidget* w = nsnull;
    if (mLocation &&
        NS_SUCCEEDED(mLocation->QueryInterface(kIWidgetIID, (void**)&w))) {
      w->Show(PR_FALSE);
      NS_RELEASE(w);
    }
    if (mBack &&
        NS_SUCCEEDED(mBack->QueryInterface(kIWidgetIID, (void**)&w))) {
      w->Move(0, menuBarHeight);
      w->Show(PR_FALSE);
      NS_RELEASE(w);
    }
    if (mForward &&
        NS_SUCCEEDED(mForward->QueryInterface(kIWidgetIID, (void**)&w))) {
      w->Move(BUTTON_WIDTH, menuBarHeight);
      w->Show(PR_FALSE);
      NS_RELEASE(w);
    }
    if (mThrobber) {
      mThrobber->Hide();
    }
  }
  
  nsIWidget* statusWidget = nsnull;

  if (mStatus && NS_OK == mStatus->QueryInterface(kIWidgetIID,(void**)&statusWidget)) {
    if (mChromeMask & NS_CHROME_STATUS_BAR_ON) {
      statusWidget->Resize(0, aHeight - txtHeight,
                           aWidth, txtHeight,
                           PR_TRUE);

      //Since allowing a negative height is a bad idea, let's condition this...
      rr.height = PR_MAX(0,rr.height-txtHeight);
      statusWidget->Show(PR_TRUE);
    }
    else {
      statusWidget->Show(PR_FALSE);
    }
    NS_RELEASE(statusWidget);
  }

  // inset the web widget

  if (NS_CHROME_TOOL_BAR_ON & mChromeMask) {
    rr.height -= BUTTON_HEIGHT;
    rr.y += BUTTON_HEIGHT;
  }

  rr.x += WEBSHELL_LEFT_INSET;
  rr.y += WEBSHELL_TOP_INSET + menuBarHeight;
  rr.width -= WEBSHELL_LEFT_INSET + WEBSHELL_RIGHT_INSET;
  rr.height -= WEBSHELL_TOP_INSET + WEBSHELL_BOTTOM_INSET + menuBarHeight;

  //Since allowing a negative height is a bad idea, let's condition this...
  if(rr.height<0)
    rr.height=0;

  nsCOMPtr<nsIBaseWindow> webBrowserWin(do_QueryInterface(mWebBrowser));
  if (webBrowserWin) {
    webBrowserWin->SetPositionAndSize(rr.x, rr.y, rr.width, rr.height, PR_FALSE);
  }
}

NS_IMETHODIMP
nsBrowserWindow::MoveTo(PRInt32 aX, PRInt32 aY)
{
  NS_PRECONDITION(nsnull != mWindow, "null window");
  mWindow->Move(aX, aY);
  return NS_OK;
}

NS_IMETHODIMP
nsBrowserWindow::SizeContentTo(PRInt32 aWidth, PRInt32 aHeight)
{
  NS_PRECONDITION(nsnull != mWindow, "null window");

  // XXX We want to do this in one shot
  mWindow->Resize(aWidth, aHeight, PR_FALSE);
  Layout(aWidth, aHeight);

  return NS_OK;
}

NS_IMETHODIMP
nsBrowserWindow::SizeWindowTo(PRInt32 aWidth, PRInt32 aHeight,
                              PRBool /*aWidthTransient*/,
                              PRBool /*aHeightTransient*/)
{
  return SizeContentTo(aWidth, aHeight);
}

NS_IMETHODIMP
nsBrowserWindow::GetContentBounds(nsRect& aBounds)
{
  mWindow->GetClientBounds(aBounds);
  return NS_OK;
}

NS_IMETHODIMP
nsBrowserWindow::GetWindowBounds(nsRect& aBounds)
{
  mWindow->GetBounds(aBounds);
  return NS_OK;
}

NS_IMETHODIMP
nsBrowserWindow::Show()
{
  NS_PRECONDITION(nsnull != mWindow, "null window");
  mWindow->Show(PR_TRUE);
  return NS_OK;
}

NS_IMETHODIMP
nsBrowserWindow::Hide()
{
  NS_PRECONDITION(nsnull != mWindow, "null window");
  mWindow->Show(PR_FALSE);
  return NS_OK;
}

static void DestroyWidget(nsISupports* aWidget)
{
  if (aWidget) {
    nsIWidget* w;
    nsresult rv = aWidget->QueryInterface(kIWidgetIID, (void**) &w);
    if (NS_SUCCEEDED(rv)) {
      w->Destroy();
      NS_RELEASE(w);
    }
    NS_RELEASE(aWidget);
  }
}

NS_IMETHODIMP
nsBrowserWindow::Close()
{
  RemoveBrowser(this);

  nsCOMPtr<nsIWebShell> webShell(do_QueryInterface(mDocShell));
  if (nsnull != webShell) {
    nsCOMPtr<nsIDocumentLoader> docLoader;
    webShell->GetDocumentLoader(*getter_AddRefs(docLoader));
    if (docLoader) {
      docLoader->RemoveObserver(this);
    }
    nsCOMPtr<nsIBaseWindow> docShellWin(do_QueryInterface(mDocShell));
    docShellWin->Destroy();
    NS_RELEASE(mDocShell);
  }

  DestroyWidget(mBack);         mBack = nsnull;
  DestroyWidget(mForward);      mForward = nsnull;
  DestroyWidget(mLocation);     mLocation = nsnull;
  DestroyWidget(mStatus);       mStatus = nsnull;

  if (mThrobber) {
    mThrobber->Destroy();
    NS_RELEASE(mThrobber);
    mThrobber = nsnull;
  }

  DestroyWidget(mWindow);       mWindow = nsnull;

  return NS_OK;
}


NS_IMETHODIMP
nsBrowserWindow::ShowModally(PRBool aPrepare)
{
  // unsupported by viewer
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsBrowserWindow::SetChrome(PRUint32 aChromeMask)
{
  mChromeMask = aChromeMask;
  nsRect r;
  mWindow->GetClientBounds(r);
  Layout(r.width, r.height);

  return NS_OK;
}

NS_IMETHODIMP
nsBrowserWindow::GetChrome(PRUint32& aChromeMaskResult)
{
  aChromeMaskResult = mChromeMask;
  return NS_OK;
}

NS_IMETHODIMP
nsBrowserWindow::GetWebShell(nsIWebShell*& aResult)
{
  nsCOMPtr<nsIWebShell> webShell(do_QueryInterface(mDocShell));
  aResult = webShell;
  NS_IF_ADDREF(aResult);
  return NS_OK;
}

NS_IMETHODIMP
nsBrowserWindow::GetContentWebShell(nsIWebShell **aResult)
{
  nsCOMPtr<nsIWebShell> webShell(do_QueryInterface(mDocShell));
  *aResult = webShell;
  NS_IF_ADDREF(*aResult);
  return NS_OK;
}

//----------------------------------------
NS_IMETHODIMP
nsBrowserWindow::IsIntrinsicallySized(PRBool& aResult)
{
  aResult = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP
nsBrowserWindow::SetTitle(const PRUnichar* aTitle)
{
   EnsureWebBrowserChrome();
   return mWebBrowserChrome->SetTitle(aTitle);
}

NS_IMETHODIMP
nsBrowserWindow::GetTitle(PRUnichar** aResult)
{
  *aResult = mTitle.ToNewUnicode();
  return NS_OK;
}

NS_IMETHODIMP
nsBrowserWindow::SetStatus(const PRUnichar* aStatus)
{
  if (nsnull != mStatus) {
    PRUint32 size;
    mStatus->SetText(aStatus,size);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsBrowserWindow::GetStatus(const PRUnichar** aResult)
{
  return NS_OK;
}

NS_IMETHODIMP
nsBrowserWindow::SetDefaultStatus(const PRUnichar* aStatus)
{
  return NS_OK;
}

NS_IMETHODIMP
nsBrowserWindow::GetDefaultStatus(const PRUnichar** aResult)
{
  return NS_OK;
}

NS_IMETHODIMP
nsBrowserWindow::SetProgress(PRInt32 aProgress, PRInt32 aProgressMax)
{
  return NS_OK;
}

NS_IMETHODIMP
nsBrowserWindow::ShowMenuBar(PRBool aShow)
{
  return NS_OK;
}

NS_IMETHODIMP
nsBrowserWindow::WillLoadURL(nsIWebShell* aShell, const PRUnichar* aURL,
                             nsLoadType aReason)
{
  nsCOMPtr<nsIWebShell> webShell(do_QueryInterface(mDocShell));
  if (aShell == webShell.get()) {
    if (mStatus) {
      nsAutoString url("Connecting to ");
      url.Append(aURL);
      PRUint32 size;
      mStatus->SetText(url,size);
      mLoadStartTime = PR_Now();
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsBrowserWindow::BeginLoadURL(nsIWebShell* aShell, const PRUnichar* aURL)
{
  nsCOMPtr<nsIWebShell> webShell(do_QueryInterface(mDocShell));
  if (aShell == webShell.get()) {
    if (mThrobber) {
      mThrobber->Start();
      PRUint32 size;
      nsAutoString tmp(aURL);
      mLocation->SetText(tmp,size);
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsBrowserWindow::ProgressLoadURL(nsIWebShell* aShell,
                                 const PRUnichar* aURL,
                                 PRInt32 aProgress,
                                 PRInt32 aProgressMax)
{
  return NS_OK;
}

NS_IMETHODIMP
nsBrowserWindow::EndLoadURL(nsIWebShell* aShell,
                            const PRUnichar* aURL,
                            nsresult aStatus)
{
  nsCOMPtr<nsIWebShell> webShell(do_QueryInterface(mDocShell));
  if (aShell == webShell.get()) {
    PRTime endLoadTime = PR_Now();
    if (mShowLoadTimes) {
      nsAutoString msg(aURL);
      printf("Loading ");
      fputs(msg, stdout);
      PRTime delta;
      LL_SUB(delta, endLoadTime, mLoadStartTime);
      double usecs;
      LL_L2D(usecs, delta);
      printf(" took %g milliseconds\n", usecs / 1000.0);
    }
    if (mThrobber) {
      mThrobber->Stop();
    }
    if (nsnull != mStatus) {
      nsAutoString msg(aURL);
      PRUint32 size;

      msg.Append(" done.");

      mStatus->SetText(msg, size);
    }
  }
  return NS_OK;
}


NS_IMETHODIMP
nsBrowserWindow::NewWebShell(PRUint32 aChromeMask,
                             PRBool aVisible,
                             nsIWebShell*& aNewWebShell)
{
  nsresult rv = NS_OK;

  if (mWebCrawler) {
    if (mWebCrawler->Crawling() || mWebCrawler->LoadingURLList()) {
      // Do not fly javascript popups when we are crawling
      aNewWebShell = nsnull;
      return NS_ERROR_NOT_IMPLEMENTED;
    }
  }

  // Create new window. By default, the refcnt will be 1 because of
  // the registration of the browser window in gBrowsers.
  nsNativeBrowserWindow* browser;
  NS_NEWXPCOM(browser, nsNativeBrowserWindow);

  if (nsnull != browser)
  {
    nsRect  bounds;
    GetContentBounds(bounds);

    browser->SetApp(mApp);

    // Assume no controls for now
    rv = browser->Init(mAppShell, bounds, aChromeMask, mAllowPlugins);
    if (NS_OK == rv)
    {
      // Default is to startup hidden
      if (aVisible) {
        browser->Show();
      }
      nsIWebShell *shell;
      rv = browser->GetWebShell(shell);
      aNewWebShell = shell;
    }
    else
    {
      browser->Close();
    }
  }
  else
    rv = NS_ERROR_OUT_OF_MEMORY;

  return rv;
}


NS_IMETHODIMP
nsBrowserWindow::ContentShellAdded(nsIWebShell* aChildShell, nsIContent* frameNode)
{
  return NS_OK;
}

NS_IMETHODIMP
nsBrowserWindow::FindWebShellWithName(const PRUnichar* aName, nsIWebShell*& aResult)
{
  PRInt32 i, n = gBrowsers.Count();

  aResult = nsnull;
  nsString aNameStr(aName);

  for (i = 0; i < n; i++) {
    nsBrowserWindow* bw = (nsBrowserWindow*) gBrowsers.ElementAt(i);
    nsCOMPtr<nsIWebShell> webShell;
    
    if (NS_OK == bw->GetWebShell(*getter_AddRefs(webShell))) {
      nsXPIDLString name;
      nsCOMPtr<nsIDocShellTreeItem> docShellAsItem(do_QueryInterface(mDocShell));
      if (NS_OK == docShellAsItem->GetName(getter_Copies(name))) {
        if (aNameStr.Equals(name)) {
          aResult = webShell;
          NS_ADDREF(aResult);
          return NS_OK;
        }
      }      

      nsCOMPtr<nsIDocShellTreeNode> docShellAsNode(do_QueryInterface(mDocShell));

      nsCOMPtr<nsIDocShellTreeItem> result;
      if (NS_OK == docShellAsNode->FindChildWithName(aName, PR_TRUE, PR_FALSE, nsnull,
         getter_AddRefs(result))) {
        if (result) {
          CallQueryInterface(result, &aResult);
          return NS_OK;
        }
      }
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsBrowserWindow::FocusAvailable(nsIWebShell* aFocusedWebShell, PRBool& aFocusTaken)
{
  return NS_OK;
}


//----------------------------------------

// document loader observer implementation
NS_IMETHODIMP
nsBrowserWindow::OnStartDocumentLoad(nsIDocumentLoader* loader, 
                                     nsIURI* aURL, 
                                     const char* aCommand)
{
  return NS_OK;
}

NS_IMETHODIMP
nsBrowserWindow::OnEndDocumentLoad(nsIDocumentLoader* loader,
                                   nsIChannel* channel,
                                   nsresult aStatus)
{
  return NS_OK;
}

NS_IMETHODIMP
nsBrowserWindow::OnStartURLLoad(nsIDocumentLoader* loader,
                                nsIChannel* channel)
{
  nsresult rv;

  nsCOMPtr<nsIURI> aURL;
  rv = channel->GetURI(getter_AddRefs(aURL));
  if (NS_FAILED(rv)) return rv;
  
  if (mStatus) {
    nsAutoString url;
    if (nsnull != aURL) {
      char* str;
      aURL->GetSpec(&str);
      url = str;
      nsCRT::free(str);
    }
    url.Append(": start");
    PRUint32 size;
    mStatus->SetText(url,size);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsBrowserWindow::OnProgressURLLoad(nsIDocumentLoader* loader,
                                   nsIChannel* channel,
                                   PRUint32 aProgress, 
                                   PRUint32 aProgressMax)
{
  return NS_OK;
}

NS_IMETHODIMP
nsBrowserWindow::OnStatusURLLoad(nsIDocumentLoader* loader,
                                 nsIChannel* channel,
                                 nsString& aMsg)
{
  return NS_OK;
}

NS_IMETHODIMP
nsBrowserWindow::OnEndURLLoad(nsIDocumentLoader* loader,
                              nsIChannel* channel,
                              nsresult aStatus)
{
  nsresult rv;

  nsCOMPtr<nsIURI> aURL;
  rv = channel->GetURI(getter_AddRefs(aURL));
  if (NS_FAILED(rv)) return rv;
  
  if (mStatus) {
    nsAutoString url;
    if (nsnull != aURL) {
      char* str;
      aURL->GetSpec(&str);
      url = str;
      nsCRT::free(str);
    }
    url.Append(": stop");
    PRUint32 size;
    mStatus->SetText(url,size);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsBrowserWindow::OnProgress(nsIChannel* channel, nsISupports *ctxt,
                            PRUint32 aProgress, PRUint32 aProgressMax)
{
  nsresult rv;

  nsCOMPtr<nsIURI> aURL;
  rv = channel->GetURI(getter_AddRefs(aURL));
  if (NS_FAILED(rv)) return rv;
  
  if (mStatus) {
    nsAutoString url;
    if (nsnull != aURL) {
      char* str;
      aURL->GetSpec(&str);
      url = str;
      nsCRT::free(str);
    }
    url.Append(": progress ");
    url.Append(aProgress, 10);
    if (0 != aProgressMax) {
      url.Append(" (out of ");
      url.Append(aProgressMax, 10);
      url.Append(")");
    }
    PRUint32 size;
    mStatus->SetText(url,size);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsBrowserWindow::OnStatus(nsIChannel* channel, nsISupports *ctxt, const PRUnichar *aMsg)
{
  if (mStatus) {
    PRUint32 size;
    mStatus->SetText(aMsg,size);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsBrowserWindow::Alert(const PRUnichar *text)
{
  nsCAutoString str(text);
  printf("%cBrowser Window Alert: %s\n", '\007', str.GetBuffer());

  return NS_OK;
}

NS_IMETHODIMP
nsBrowserWindow::Confirm(const PRUnichar *text,
                         PRBool *result)
{
  nsCAutoString str(text);
  const char* msg= nsnull;

  msg = str.GetBuffer();
  if (nsnull != msg) {
    printf("Browser Window Confirm: %s (y/n)? ", msg);
    char c;
    for (;;) {
      c = getchar();
      if (tolower(c) == 'y') {
        *result = PR_TRUE;
      }
      if (tolower(c) == 'n') {
        *result = PR_FALSE;
      }
    }
  }
  *result = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP
nsBrowserWindow::ConfirmCheck(const PRUnichar *text,
                              const PRUnichar *checkMsg,
                              PRBool *checkValue,
                              PRBool *result)
{
  return Confirm(text, result);
}

NS_IMETHODIMP
nsBrowserWindow::Prompt(const PRUnichar *text,
                        const PRUnichar *defaultText,
                        PRUnichar **result,
                        PRBool *_retval)
{
  nsCAutoString str(text);
  const char* msg= nsnull;
  char buf[256];

  msg = str.GetBuffer();
  if (nsnull != msg) {
    printf("Browser Window: %s\n", msg);

    printf("%cPrompt: ", '\007');
    scanf("%s", buf);
    nsAutoString response(buf);
    *result = response.ToNewUnicode();
  }
  
  *_retval = (nsCRT::strlen(buf) > 0);
  return NS_OK;
}

NS_IMETHODIMP
nsBrowserWindow::PromptUsernameAndPassword(const PRUnichar *text,
                                           PRUnichar **user,
                                           PRUnichar **pwd,
                                           PRBool *_retval)
{
  nsCAutoString str(text);
  const char* msg = nsnull;
  char buf[256];

  msg = str.GetBuffer();
  if (nsnull != msg) {
    nsAutoString response;
    printf("Browser Window: %s\n", msg);

    printf("%cUser: ", '\007');
    scanf("%s", buf);
    response.SetString(buf);
    *user = response.ToNewUnicode();

    printf("%cPassword: ", '\007');
    scanf("%s", buf);
    response.SetString(buf);
    *pwd = response.ToNewUnicode();
  }

  *_retval = (nsCRT::strlen(*user) > 0);
  return NS_OK;
}

NS_IMETHODIMP
nsBrowserWindow::PromptPassword(const PRUnichar *text,
				const PRUnichar *title,
                                PRUnichar **pwd,
                                PRBool *_retval)
{
  nsCAutoString str(text);
  const char* msg = nsnull;
  char buf[256];

  msg = str.GetBuffer();
  if (nsnull != msg) {
    printf("Browser Window: %s\n", msg);
    printf("%cPassword: ", '\007');
    scanf("%s", buf);
    nsAutoString response(buf);
    *pwd = response.ToNewUnicode();
  }
 
  *_retval = (nsCRT::strlen(*pwd) > 0);
  return NS_OK;
}

nsresult nsBrowserWindow::Select(const PRUnichar *, const PRUnichar *, PRUint32 , const PRUnichar **, PRInt32 *, PRBool *_retval)
{
	return NS_OK;
}

NS_IMETHODIMP nsBrowserWindow::UniversalDialog
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
	return NS_OK;
}


//----------------------------------------

// Toolbar support

void
nsBrowserWindow::StartThrobber()
{
}

void
nsBrowserWindow::StopThrobber()
{
}

void
nsBrowserWindow::LoadThrobberImages()
{
}

void
nsBrowserWindow::DestroyThrobberImages()
{
}

nsIPresShell*
nsBrowserWindow::GetPresShell()
{
  return GetPresShellFor(mDocShell);
}


void
nsBrowserWindow::DoCopy()
{
  nsIPresShell* shell = GetPresShell();
  if (nsnull != shell) {
    shell->DoCopy();
    NS_RELEASE(shell);
  }
}

void
nsBrowserWindow::DoPaste()
{
  nsIPresShell* shell = GetPresShell();
  if (nsnull != shell) {

    printf("nsBrowserWindow::DoPaste()\n");

    NS_RELEASE(shell);
  }
}

//----------------------------------------------------------------------

void
nsBrowserWindow::ShowPrintPreview(PRInt32 aID)
{
  static NS_DEFINE_CID(kPrintPreviewContextCID, NS_PRINT_PREVIEW_CONTEXT_CID);
  static NS_DEFINE_IID(kIPresContextIID, NS_IPRESCONTEXT_IID);
  nsIContentViewer* cv = nsnull;
  if (nsnull != mDocShell) {
    if ((NS_OK == mDocShell->GetContentViewer(&cv)) && (nsnull != cv)) {
      nsIDocumentViewer* docv = nsnull;
      if (NS_OK == cv->QueryInterface(kIDocumentViewerIID, (void**)&docv)) {
        nsIPresContext* printContext;
        nsresult rv =
          nsComponentManager::CreateInstance(kPrintPreviewContextCID,
                                             nsnull,
                                             kIPresContextIID,
                                             (void **)&printContext);
        if (NS_SUCCEEDED(rv)) {
          // Prepare new printContext for print-preview
          nsCOMPtr<nsIDeviceContext> dc;
          nsIPresContext* presContext;
          docv->GetPresContext(presContext);
          presContext->GetDeviceContext(getter_AddRefs(dc));
          printContext->Init(dc);
          NS_RELEASE(presContext);

          // Make a window using that content viewer
          // XXX Some piece of code needs to properly hold the reference to this
          // browser window. For the time being the reference is released by the
          // browser event handling code during processing of the NS_DESTROY event...
          nsBrowserWindow* bw = new nsNativeBrowserWindow;
          bw->SetApp(mApp);
          bw->Init(mAppShell, nsRect(0, 0, 600, 400),
                   NS_CHROME_MENU_BAR_ON, PR_TRUE, docv, printContext);
          bw->Show();

          NS_RELEASE(printContext);
        }
        NS_RELEASE(docv);
      }
      NS_RELEASE(cv);
    }
  }
}

void nsBrowserWindow::DoPrint(void)
{
  nsCOMPtr <nsIContentViewer> viewer;

  mDocShell->GetContentViewer(getter_AddRefs(viewer));

  if (viewer)
  {
    nsCOMPtr<nsIContentViewerFile> viewerFile = do_QueryInterface(viewer);
    if (viewerFile) {
      viewerFile->Print(PR_FALSE,0);
    }
  }
}

//---------------------------------------------------------------
void nsBrowserWindow::DoPrintSetup()
{
  if (mXPDialog) {
    NS_RELEASE(mXPDialog);
    //mXPDialog->SetVisible(PR_TRUE);
    //return;
  }

  nsString printHTML("resource:/res/samples/printsetup.html");
  nsRect rect(0, 0, 375, 510);
  nsString title("Print Setup");

  nsXPBaseWindow * dialog = nsnull;
  nsresult rv = nsComponentManager::CreateInstance(kXPBaseWindowCID, nsnull,
                                             kIXPBaseWindowIID,
                                             (void**) &dialog);
  if (rv == NS_OK) {
    dialog->Init(eXPBaseWindowType_dialog, mAppShell, printHTML,
                 title, rect, PRUint32(~0), PR_FALSE);
    dialog->SetVisible(PR_TRUE);
    if (NS_OK == dialog->QueryInterface(kIXPBaseWindowIID, (void**)&mXPDialog)) {
    }
  }

  mPrintSetupInfo.mPortrait         = PR_TRUE;
  mPrintSetupInfo.mBevelLines       = PR_TRUE;
  mPrintSetupInfo.mBlackText        = PR_FALSE;
  mPrintSetupInfo.mBlackLines       = PR_FALSE;
  mPrintSetupInfo.mLastPageFirst    = PR_FALSE;
  mPrintSetupInfo.mPrintBackgrounds = PR_FALSE;
  mPrintSetupInfo.mTopMargin        = 0.50;
  mPrintSetupInfo.mBottomMargin     = 0.50;
  mPrintSetupInfo.mLeftMargin       = 0.50;
  mPrintSetupInfo.mRightMargin      = 0.50;

  mPrintSetupInfo.mDocTitle         = PR_TRUE;
  mPrintSetupInfo.mDocLocation      = PR_TRUE;

  mPrintSetupInfo.mHeaderText       = "Header Text";
  mPrintSetupInfo.mFooterText       = "Footer Text";

  mPrintSetupInfo.mPageNum          = PR_TRUE;
  mPrintSetupInfo.mPageTotal        = PR_TRUE;
  mPrintSetupInfo.mDatePrinted      = PR_TRUE;


  nsPrintSetupDialog * printSetupDialog = new nsPrintSetupDialog(this);
  if (nsnull != printSetupDialog) {
    dialog->AddWindowListener(printSetupDialog);
  }
  printSetupDialog->SetSetupInfo(mPrintSetupInfo);
  //NS_IF_RELEASE(dialog);

}

//---------------------------------------------------------------
nsIDOMDocument* nsBrowserWindow::GetDOMDocument(nsIDocShell *aDocShell)
{
  nsIDOMDocument* domDoc = nsnull;
  if (nsnull != aDocShell) {
    nsIContentViewer* mCViewer;
    aDocShell->GetContentViewer(&mCViewer);
    if (nsnull != mCViewer) {
      nsIDocumentViewer* mDViewer;
      if (NS_OK == mCViewer->QueryInterface(kIDocumentViewerIID, (void**) &mDViewer)) {
        nsIDocument* mDoc;
        mDViewer->GetDocument(mDoc);
        if (nsnull != mDoc) {
          if (NS_OK == mDoc->QueryInterface(kIDOMDocumentIID, (void**) &domDoc)) {
          }
          NS_RELEASE(mDoc);
        }
        NS_RELEASE(mDViewer);
      }
      NS_RELEASE(mCViewer);
    }
  }
  return domDoc;
}

//---------------------------------------------------------------

void nsBrowserWindow::DoTableInspector()
{
  if (mTableInspectorDialog) {
    mTableInspectorDialog->SetVisible(PR_TRUE);
    return;
  }
  nsIDOMDocument* domDoc = GetDOMDocument(mDocShell);

  if (nsnull != domDoc) {
    nsString printHTML("resource:/res/samples/printsetup.html");
    nsRect rect(0, 0, 375, 510);
    nsString title("Table Inspector");

    nsXPBaseWindow * xpWin = nsnull;
    nsresult rv = nsComponentManager::CreateInstance(kXPBaseWindowCID, nsnull,
                                                     kIXPBaseWindowIID,
                                                     (void**) &xpWin);
    if (rv == NS_OK) {
      xpWin->Init(eXPBaseWindowType_dialog, mAppShell, printHTML, title, rect, PRUint32(~0), PR_FALSE);
      xpWin->SetVisible(PR_TRUE);
      if (NS_OK == xpWin->QueryInterface(kIXPBaseWindowIID, (void**) &mTableInspectorDialog)) {
        mTableInspector = new nsTableInspectorDialog(this, domDoc); // ref counts domDoc
        if (nsnull != mTableInspector) {
          xpWin->AddWindowListener(mTableInspector); 
        }
      }
      NS_RELEASE(xpWin);
    }
    NS_RELEASE(domDoc);
  }

}
void nsBrowserWindow::DoImageInspector()
{
  if (mImageInspectorDialog) {
    mImageInspectorDialog->SetVisible(PR_TRUE);
    return;
  }

  nsIDOMDocument* domDoc = GetDOMDocument(mDocShell);

  if (nsnull != domDoc) {
    nsString printHTML("resource:/res/samples/image_props.html");
    nsRect rect(0, 0, 485, 124);
    nsString title("Image Inspector");

    nsXPBaseWindow * xpWin = nsnull;
    nsresult rv = nsComponentManager::CreateInstance(kXPBaseWindowCID, nsnull, kIXPBaseWindowIID, (void**) &xpWin);
    if (rv == NS_OK) {
      xpWin->Init(eXPBaseWindowType_dialog, mAppShell, printHTML, title, rect, PRUint32(~0), PR_FALSE);
      xpWin->SetVisible(PR_TRUE);
      if (NS_OK == xpWin->QueryInterface(kIXPBaseWindowIID, (void**) &mImageInspectorDialog)) {
        mImageInspector = new nsImageInspectorDialog(this, domDoc); // ref counts domDoc
        if (nsnull != mImageInspector) {
          xpWin->AddWindowListener(mImageInspector); 
        }
      }
      NS_RELEASE(xpWin);
    }
    NS_RELEASE(domDoc);
  }

}

//----------------------------------------------------------------------

void
nsBrowserWindow::DoJSConsole()
{
  mApp->CreateJSConsole(this);
}

void
nsBrowserWindow::DoPrefs()
{
  mApp->DoPrefs(this);
}

void
nsBrowserWindow::DoEditorMode(nsIDocShell *aDocShell)
{
  PRInt32 i, n;
  if (nsnull != aDocShell) {
    nsIContentViewer* mCViewer;
    aDocShell->GetContentViewer(&mCViewer);
    if (nsnull != mCViewer) {
      nsIDocumentViewer* mDViewer;
      if (NS_OK == mCViewer->QueryInterface(kIDocumentViewerIID, (void**) &mDViewer)) 
      {
        nsIDocument* mDoc;
        mDViewer->GetDocument(mDoc);
        if (nsnull != mDoc) {
          nsIDOMDocument* mDOMDoc;
          if (NS_OK == mDoc->QueryInterface(kIDOMDocumentIID, (void**) &mDOMDoc)) 
          {
            nsIPresShell* shell = GetPresShellFor(aDocShell);
            NS_InitEditorMode(mDOMDoc, shell);
            NS_RELEASE(mDOMDoc);
            NS_IF_RELEASE(shell);
          }
          NS_RELEASE(mDoc);
        }
        NS_RELEASE(mDViewer);
      }
      NS_RELEASE(mCViewer);
    }
    
    nsCOMPtr<nsIDocShellTreeNode> docShellAsNode(do_QueryInterface(aDocShell));
    docShellAsNode->GetChildCount(&n);
    for (i = 0; i < n; i++) {
      nsCOMPtr<nsIDocShellTreeItem> child;
      docShellAsNode->GetChildAt(i, getter_AddRefs(child));
      nsCOMPtr<nsIDocShell> childAsShell(do_QueryInterface(child));
      DoEditorMode(childAsShell);
    }
  }
}

// Same as above, but calls NS_DoEditorTest instead of starting an editor
void
nsBrowserWindow::DoEditorTest(nsIDocShell *aDocShell, PRInt32 aCommandID)
{
  if (nsnull != aDocShell) {
    nsIContentViewer* mCViewer;
    aDocShell->GetContentViewer(&mCViewer);
    if (nsnull != mCViewer) {
      nsIDocumentViewer* mDViewer;
      if (NS_OK == mCViewer->QueryInterface(kIDocumentViewerIID, (void**) &mDViewer)) 
      {
        nsIDocument* mDoc;
        mDViewer->GetDocument(mDoc);
        if (nsnull != mDoc) {
          nsIDOMDocument* mDOMDoc;
          if (NS_OK == mDoc->QueryInterface(kIDOMDocumentIID, (void**) &mDOMDoc)) 
          {
            NS_DoEditorTest(aCommandID);
            NS_RELEASE(mDOMDoc);
          }
          NS_RELEASE(mDoc);
        }
        NS_RELEASE(mDViewer);
      }
      NS_RELEASE(mCViewer);
    }
    // Do we need to do all the children as in DoEditorMode?
    // Its seems to work if we don't do that
  }
}




#ifdef NS_DEBUG
#include "nsIContent.h"
#include "nsIFrame.h"
#include "nsIFrameDebug.h"
#include "nsIStyleContext.h"
#include "nsIStyleSet.h"


static void DumpAWebShell(nsIDocShellTreeItem* aShellItem, FILE* out, PRInt32 aIndent)
{
  nsXPIDLString name;
  nsAutoString str;
  nsCOMPtr<nsIDocShellTreeItem> parent;
  PRInt32 i, n;

  for (i = aIndent; --i >= 0; ) fprintf(out, "  ");

  fprintf(out, "%p '", aShellItem);
  aShellItem->GetName(getter_Copies(name));
  aShellItem->GetSameTypeParent(getter_AddRefs(parent));
  str = name;
  fputs(str, out);
  fprintf(out, "' parent=%p <\n", parent.get());

  aIndent++;
  nsCOMPtr<nsIDocShellTreeNode> shellAsNode(do_QueryInterface(aShellItem));
  shellAsNode->GetChildCount(&n);
  for (i = 0; i < n; i++) {
    nsCOMPtr<nsIDocShellTreeItem> child;
    shellAsNode->GetChildAt(i, getter_AddRefs(child));
    if (child) {
      DumpAWebShell(child, out, aIndent);
    }
  }
  aIndent--;
  for (i = aIndent; --i >= 0; ) fprintf(out, "  ");
  fputs(">\n", out);
}

void
nsBrowserWindow::DumpWebShells(FILE* out)
{
  nsCOMPtr<nsIDocShellTreeItem> shellAsItem(do_QueryInterface(mDocShell));
  DumpAWebShell(shellAsItem, out, 0);
}

static
void 
DumpMultipleWebShells(nsBrowserWindow& aBrowserWindow, nsIDocShell* aDocShell, FILE* aOut)
{ 
  PRInt32 count;
  nsCOMPtr<nsIDocShellTreeNode> docShellAsNode(do_QueryInterface(aDocShell));
  if (docShellAsNode) {
    docShellAsNode->GetChildCount(&count);
    if (count > 0) {
      fprintf(aOut, "webshells= \n");
      aBrowserWindow.DumpWebShells(aOut);
    }
  }
}

static
void
DumpContentRecurse(nsIDocShell* aDocShell, FILE* out)
{
  if (nsnull != aDocShell) {
    fprintf(out, "docshell=%p \n", aDocShell);
    nsIPresShell* shell = GetPresShellFor(aDocShell);
    if (nsnull != shell) {
      nsCOMPtr<nsIDocument> doc;
      shell->GetDocument(getter_AddRefs(doc));
      if (doc) {
        nsIContent* root = doc->GetRootContent();
        if (nsnull != root) {
          root->List(out);
          NS_RELEASE(root);
        }
      }
      NS_RELEASE(shell);
    }
    else {
      fputs("null pres shell\n", out);
    }
    // dump the frames of the sub documents
    PRInt32 i, n;
    nsCOMPtr<nsIDocShellTreeNode> docShellAsNode(do_QueryInterface(aDocShell));
    docShellAsNode->GetChildCount(&n);
    for (i = 0; i < n; i++) {
      nsCOMPtr<nsIDocShellTreeItem> child;
      docShellAsNode->GetChildAt(i, getter_AddRefs(child));
      nsCOMPtr<nsIDocShell> childAsShell(do_QueryInterface(child));
      if (nsnull != child) {
        DumpContentRecurse(childAsShell, out);
      }
    }
  }
}

void
nsBrowserWindow::DumpContent(FILE* out)
{
  DumpContentRecurse(mDocShell, out);
  DumpMultipleWebShells(*this, mDocShell, out);
}

static void
DumpFramesRecurse(nsIDocShell* aDocShell, FILE* out, nsString *aFilterName)
{
  if (nsnull != aDocShell) {
    fprintf(out, "webshell=%p \n", aDocShell);
    nsIPresShell* shell = GetPresShellFor(aDocShell);
    if (nsnull != shell) {
      nsIFrame* root;
      shell->GetRootFrame(&root);
      if (nsnull != root) {
        nsIPresContext* presContext;
        shell->GetPresContext(&presContext);

        nsIFrameDebug* fdbg;
        if (NS_SUCCEEDED(root->QueryInterface(NS_GET_IID(nsIFrameDebug), (void**)&fdbg))) {
          fdbg->List(presContext, out, 0);
        }
        NS_IF_RELEASE(presContext);
      }
      NS_RELEASE(shell);
    }
    else {
      fputs("null pres shell\n", out);
    }

    // dump the frames of the sub documents
    PRInt32 i, n;
    nsCOMPtr<nsIDocShellTreeNode> docShellAsNode(do_QueryInterface(aDocShell));
    docShellAsNode->GetChildCount(&n);
    for (i = 0; i < n; i++) {
      nsCOMPtr<nsIDocShellTreeItem> child;
      docShellAsNode->GetChildAt(i, getter_AddRefs(child));
      nsCOMPtr<nsIDocShell> childAsShell(do_QueryInterface(child));
      if (childAsShell) {
        DumpFramesRecurse(childAsShell, out, aFilterName);
      }
    }
  }
}

void
nsBrowserWindow::DumpFrames(FILE* out, nsString *aFilterName)
{
  DumpFramesRecurse(mDocShell, out, aFilterName);
  DumpMultipleWebShells(*this, mDocShell, out);
}

//----------------------------------------------------------------------

#include "nsISizeOfHandler.h"
#include "nsQuickSort.h"

struct SizeReportEntry {
  nsIAtom* mKey;
  PRUint32 mCount;
  PRUint32 mTotalSize;
  PRUint32 mMinSize;
  PRUint32 mMaxSize;
};

static void
GatherSizeReportData(nsISizeOfHandler* aHandler,
                     nsIAtom* aKey,
                     PRUint32 aCount,
                     PRUint32 aTotalSize,
                     PRUint32 aMinSize,
                     PRUint32 aMaxSize,
                     void* arg)
{
  if (aKey) {
    nsVoidArray* array = (nsVoidArray*) arg;
    SizeReportEntry* entry = new SizeReportEntry();
    entry->mKey = aKey;
    entry->mCount = aCount;
    entry->mTotalSize = aTotalSize;
    entry->mMinSize = aMinSize;
    entry->mMaxSize = aMaxSize;
    array->AppendElement((void*) entry);
  }
}

static int CompareEntries(const void* ve1, const void* ve2, void* closure)
{
  SizeReportEntry* e1 = (SizeReportEntry*) ve1;
  SizeReportEntry* e2 = (SizeReportEntry*) ve2;
  if (e1->mKey == e2->mKey) {
    return 0;
  }
  nsAutoString k1, k2;
  e1->mKey->ToString(k1);
  e2->mKey->ToString(k2);
  if (k1 < k2) {
    return -1;
  }
  return 1;
}

static SizeReportEntry*
SortFrameSizeList(nsVoidArray& sizeReportData)
{
  // Convert data in void array into a real array, freeing the old data
  PRInt32 i, count = sizeReportData.Count();
  SizeReportEntry* all = new SizeReportEntry[count];
  SizeReportEntry* to = all;
  for (i = 0; i < count; i++) {
    SizeReportEntry* e = (SizeReportEntry*) sizeReportData[i];
    *to = *e;
    to++;
    sizeReportData.ReplaceElementAt(nsnull, i);
    delete e;
  }

  // Now sort the array
  NS_QuickSort(all, count, sizeof(SizeReportEntry), CompareEntries, nsnull);

  return all;
}

static void ShowReport(FILE* out, nsISizeOfHandler* aHandler)
{
  nsVoidArray sizeReportData;
  aHandler->Report(GatherSizeReportData, (void*) &sizeReportData);

  if (sizeReportData.Count()) {
    PRInt32 count = sizeReportData.Count();
    SizeReportEntry* entries = SortFrameSizeList(sizeReportData);

    // Display headers
    fprintf(out, "%-20s %-5s %-9s %-7s %-7s %-7s\n",
            "Frame Type", "Count", "TotalSize",
            "MinSize", "MaxSize", "AvgSize");
    fprintf(out, "%-20s %-5s %-9s %-7s %-7s %-7s\n",
            "----------", "-----", "---------",
            "-------", "-------", "-------");

    SizeReportEntry* entry = entries;
    SizeReportEntry* end = entries + count;
    for (; entry < end; entry++) {
      nsAutoString type;
      entry->mKey->ToString(type);
      char cbuf[20];
      type.ToCString(cbuf, sizeof(cbuf));

      fprintf(out, "%-20s %5d %9d %7d %7d %7d\n",
              cbuf, entry->mCount, entry->mTotalSize,
              entry->mMinSize, entry->mMaxSize,
              entry->mCount ? entry->mTotalSize / entry->mCount : 0);
    }
    delete entries;

    // Display totals and trailer
    PRUint32 totalCount, totalSize;
    aHandler->GetTotals(&totalCount, &totalSize);
    fprintf(out, "%-20s %5d %9d\n",
            "*** Total ***", totalCount, totalSize);
  }
}

//----------------------------------------

static void
GatherFrameDataSizes(nsIPresContext* aPresContext, nsISizeOfHandler* aHandler, nsIFrame* aFrame)
{
  if (aFrame) {
    // Add in the frame
    nsCOMPtr<nsIAtom> frameType;
    aFrame->GetFrameType(getter_AddRefs(frameType));
    PRUint32 frameDataSize;
    nsIFrameDebug* fdbg;
    if (NS_SUCCEEDED(aFrame->QueryInterface(NS_GET_IID(nsIFrameDebug), (void**) &fdbg))) {
      fdbg->SizeOf(aHandler, &frameDataSize);
      aHandler->AddSize(frameType, frameDataSize);
    }

    // And all of its children
    PRInt32 listIndex = 0;
    nsIAtom* listName = nsnull;
    do {
      nsIFrame* kid;
      aFrame->FirstChild(aPresContext, listName, &kid);
      while (kid) {
        GatherFrameDataSizes(aPresContext, aHandler, kid);
        kid->GetNextSibling(&kid);
      }
      NS_IF_RELEASE(listName);
      aFrame->GetAdditionalChildListName(listIndex, &listName);
      listIndex++;
    } while (nsnull != listName);
  }
}

static void
GatherFrameDataSizes(nsISizeOfHandler* aHandler, nsIDocShell* aDocShell)
{
  if (nsnull != aDocShell) {
    nsIPresShell* shell = GetPresShellFor(aDocShell);
    if (nsnull != shell) {
      nsCOMPtr<nsIPresContext> presContext;
      nsIFrame* root;
      shell->GetRootFrame(&root);
      shell->GetPresContext(getter_AddRefs(presContext));
      if (nsnull != root) {
        GatherFrameDataSizes(presContext, aHandler, root);
      }
      NS_RELEASE(shell);
    }

    // dump the frames of the sub documents
    PRInt32 i, n;
    nsCOMPtr<nsIDocShellTreeNode> docShellAsNode(do_QueryInterface(aDocShell));
    docShellAsNode->GetChildCount(&n);
    for (i = 0; i < n; i++) {
      nsCOMPtr<nsIDocShellTreeItem> child;
      docShellAsNode->GetChildAt(i, getter_AddRefs(child));
      nsCOMPtr<nsIDocShell> childAsShell(do_QueryInterface(child));
      if (childAsShell) {
        GatherFrameDataSizes(aHandler, childAsShell);
      }
    }
  }
}

void
nsBrowserWindow::ShowFrameSize(FILE* out)
{
  nsCOMPtr<nsISizeOfHandler> handler;
  nsresult rv = NS_NewSizeOfHandler(getter_AddRefs(handler));
  if (NS_SUCCEEDED(rv) && handler) {
    GatherFrameDataSizes(handler, mDocShell);
    ShowReport(out, handler);
  }
}

//----------------------------------------------------------------------

static
void
DumpViewsRecurse(nsIDocShell* aDocShell, FILE* out)
{
  if (nsnull != aDocShell) {
    fprintf(out, "docshell=%p \n", aDocShell);
    nsIPresShell* shell = GetPresShellFor(aDocShell);
    if (nsnull != shell) {
      nsCOMPtr<nsIViewManager> vm;
      shell->GetViewManager(getter_AddRefs(vm));
      if (vm) {
        nsIView* root;
        vm->GetRootView(root);
        if (nsnull != root) {
          root->List(out);
        }
      }
      NS_RELEASE(shell);
    }
    else {
      fputs("null pres shell\n", out);
    }

    // dump the views of the sub documents
    PRInt32 i, n;
    nsCOMPtr<nsIDocShellTreeNode> docShellAsNode(do_QueryInterface(aDocShell));
    docShellAsNode->GetChildCount(&n);
    for (i = 0; i < n; i++) {
      nsCOMPtr<nsIDocShellTreeItem> child;
      docShellAsNode->GetChildAt(i, getter_AddRefs(child));
      nsCOMPtr<nsIDocShell> childAsShell(do_QueryInterface(child));
      if (childAsShell) {
        DumpViewsRecurse(childAsShell, out);
      }
    }
  }
}

void
nsBrowserWindow::DumpViews(FILE* out)
{
  DumpViewsRecurse(mDocShell, out);
  DumpMultipleWebShells(*this, mDocShell, out);
}

void
nsBrowserWindow::DumpStyleSheets(FILE* out)
{
  nsIPresShell* shell = GetPresShell();
  if (nsnull != shell) {
    nsCOMPtr<nsIStyleSet> styleSet;
    shell->GetStyleSet(getter_AddRefs(styleSet));
    if (!styleSet) {
      fputs("null style set\n", out);
    } else {
      styleSet->List(out);
    }
    NS_RELEASE(shell);
  }
  else {
    fputs("null pres shell\n", out);
  }
}

void
nsBrowserWindow::DumpStyleContexts(FILE* out)
{
  nsIPresShell* shell = GetPresShell();
  if (nsnull != shell) {
    nsCOMPtr<nsIStyleSet> styleSet;
    shell->GetStyleSet(getter_AddRefs(styleSet));
    if (!styleSet) {
      fputs("null style set\n", out);
    } else {
      nsIFrame* root;
      shell->GetRootFrame(&root);
      if (nsnull == root) {
        fputs("null root frame\n", out);
      } else {
        nsIStyleContext* rootContext;
        root->GetStyleContext(&rootContext);
        if (nsnull != rootContext) {
          styleSet->ListContexts(rootContext, out);
          NS_RELEASE(rootContext);
        }
        else {
          fputs("null root context", out);
        }
      }
    }
    NS_RELEASE(shell);
  } else {
    fputs("null pres shell\n", out);
  }
}

void
nsBrowserWindow::ToggleFrameBorders()
{
  nsILayoutDebugger* ld;
  nsresult rv = nsComponentManager::CreateInstance(kLayoutDebuggerCID,
                                                   nsnull,
                                                   kILayoutDebuggerIID,
                                                   (void **)&ld);
  if (NS_SUCCEEDED(rv)) {
    PRBool showing;
    ld->GetShowFrameBorders(&showing);
    ld->SetShowFrameBorders(!showing);
    ForceRefresh();
    NS_RELEASE(ld);
  }
}

void
nsBrowserWindow::ToggleVisualEventDebugging()
{
  nsILayoutDebugger* ld;
  nsresult rv = nsComponentManager::CreateInstance(kLayoutDebuggerCID,
                                                   nsnull,
                                                   kILayoutDebuggerIID,
                                                   (void **)&ld);
  if (NS_SUCCEEDED(rv)) {
    PRBool showing;
    ld->GetShowEventTargetFrameBorder(&showing);
    ld->SetShowEventTargetFrameBorder(!showing);
    ForceRefresh();
    NS_RELEASE(ld);
  }
}

void
nsBrowserWindow::ToggleBoolPrefAndRefresh(const char * aPrefName)
{
  NS_ASSERTION(nsnull != aPrefName,"null pref name");

  nsCOMPtr<nsIPref> prefs(do_GetService(NS_PREF_PROGID));
  if (prefs && nsnull != aPrefName)
  {
    PRBool value;
    prefs->GetBoolPref(aPrefName,&value);
    prefs->SetBoolPref(aPrefName,!value);
    prefs->SavePrefFile();
    
    ForceRefresh();
  }
}

//----------------------------------------------------------------------

static void
GatherContentDataSizes(nsISizeOfHandler* aHandler, nsIContent* aContent)
{
  if (aContent) {
    // Add in the content
    nsCOMPtr<nsIAtom> tag;
    aContent->GetTag(*getter_AddRefs(tag));
    if (!tag) {
      tag = getter_AddRefs(NS_NewAtom(":content-no-tag"));
    }
    PRUint32 contentDataSize;
    aContent->SizeOf(aHandler, &contentDataSize);
    aHandler->AddSize(tag, contentDataSize);

    // And all of its children
    PRInt32 i, childCount;
    aContent->ChildCount(childCount);
    for (i = 0; i < childCount; i++) {
      nsIContent* kid;
      aContent->ChildAt(i, kid);
      if (kid) {
        GatherContentDataSizes(aHandler, kid);
        NS_RELEASE(kid);
      }
    }
  }
}

static void
GatherContentDataSizes(nsISizeOfHandler* aHandler, nsIDocShell* aDocShell)
{
  if (nsnull != aDocShell) {
    nsIPresShell* shell = GetPresShellFor(aDocShell);
    if (nsnull != shell) {
      nsIDocument* doc;
      shell->GetDocument(&doc);
      if (doc) {
        nsCOMPtr<nsIContent> rootContent = getter_AddRefs(
          doc->GetRootContent()
          );
        if (rootContent) {
          GatherContentDataSizes(aHandler, rootContent.get());
        }
        NS_RELEASE(doc);
      }
      NS_RELEASE(shell);
    }

    // dump the frames of the sub documents
    PRInt32 i, n;
    nsCOMPtr<nsIDocShellTreeNode> docShellAsNode(do_QueryInterface(aDocShell));
    docShellAsNode->GetChildCount(&n);
    for (i = 0; i < n; i++) {
      nsCOMPtr<nsIDocShellTreeItem> child;
      docShellAsNode->GetChildAt(i, getter_AddRefs(child));
      nsCOMPtr<nsIDocShell> childAsShell(do_QueryInterface(child));
      if (childAsShell) {
        GatherContentDataSizes(aHandler, childAsShell);
      }
    }
  }
}

void
nsBrowserWindow::ShowContentSize(FILE* out)
{
  nsCOMPtr<nsISizeOfHandler> handler;
  nsresult rv = NS_NewSizeOfHandler(getter_AddRefs(handler));
  if (NS_SUCCEEDED(rv) && handler) {
    GatherContentDataSizes(handler, mDocShell);
    ShowReport(out, handler);
  }
}

//----------------------------------------------------------------------

void
nsBrowserWindow::ShowStyleSize()
{
  // XXX not yet implemented
}

void
nsBrowserWindow::DoDebugSave()
{
  /* Removed 04/01/99 -- gpk */
}

void 
nsBrowserWindow::DoToggleSelection()
{
  nsIPresShell* shell = GetPresShell();
  if (nsnull != shell) {
    nsCOMPtr<nsIDocument> doc;
    shell->GetDocument(getter_AddRefs(doc));
    if (doc) {
      PRBool  current = doc->GetDisplaySelection();
      doc->SetDisplaySelection(!current);
      ForceRefresh();
    }
    NS_RELEASE(shell);
  }
}

void
nsBrowserWindow::DoDebugRobot()
{
  mApp->CreateRobot(this);
}

void
nsBrowserWindow::DoSiteWalker()
{
  mApp->CreateSiteWalker(this);
}

nsEventStatus
nsBrowserWindow::DispatchDebugMenu(PRInt32 aID)
{
  nsEventStatus result = nsEventStatus_eIgnore;

  switch(aID) {
    case VIEWER_VISUAL_DEBUGGING:
      ToggleFrameBorders();
      result = nsEventStatus_eConsumeNoDefault;
      break;

    case VIEWER_VISUAL_EVENT_DEBUGGING:
      ToggleVisualEventDebugging();
      result = nsEventStatus_eConsumeNoDefault;
      break;

    case VIEWER_TOGGLE_PAINT_FLASHING:
      ToggleBoolPrefAndRefresh("nglayout.debug.paint_flashing");
      result = nsEventStatus_eConsumeNoDefault;
      break;

    case VIEWER_TOGGLE_PAINT_DUMPING:
      ToggleBoolPrefAndRefresh("nglayout.debug.paint_dumping");
      result = nsEventStatus_eConsumeNoDefault;
      break;

    case VIEWER_TOGGLE_INVALIDATE_DUMPING:
      ToggleBoolPrefAndRefresh("nglayout.debug.invalidate_dumping");
      result = nsEventStatus_eConsumeNoDefault;
      break;

    case VIEWER_TOGGLE_EVENT_DUMPING:
      ToggleBoolPrefAndRefresh("nglayout.debug.event_dumping");
      result = nsEventStatus_eConsumeNoDefault;
      break;

    case VIEWER_TOGGLE_MOTION_EVENT_DUMPING:
      ToggleBoolPrefAndRefresh("nglayout.debug.motion_event_dumping");
      result = nsEventStatus_eConsumeNoDefault;
      break;

    case VIEWER_TOGGLE_CROSSING_EVENT_DUMPING:
      ToggleBoolPrefAndRefresh("nglayout.debug.crossing_event_dumping");
      result = nsEventStatus_eConsumeNoDefault;
      break;

    case VIEWER_DUMP_CONTENT:
      DumpContent();
      result = nsEventStatus_eConsumeNoDefault;
      break;

    case VIEWER_DUMP_FRAMES:
      DumpFrames();
      result = nsEventStatus_eConsumeNoDefault;
      break;

    case VIEWER_DUMP_VIEWS:
      DumpViews();
      result = nsEventStatus_eConsumeNoDefault;
      break;

    case VIEWER_DUMP_STYLE_SHEETS:
      DumpStyleSheets();
      result = nsEventStatus_eConsumeNoDefault;
      break;

    case VIEWER_DUMP_STYLE_CONTEXTS:
      DumpStyleContexts();
      result = nsEventStatus_eConsumeNoDefault;
      break;

    case VIEWER_SHOW_CONTENT_SIZE:
      ShowContentSize();
      result = nsEventStatus_eConsumeNoDefault;
      break;

    case VIEWER_SHOW_FRAME_SIZE:
      ShowFrameSize();
      result = nsEventStatus_eConsumeNoDefault;
      break;

    case VIEWER_SHOW_STYLE_SIZE:
      ShowStyleSize();
      result = nsEventStatus_eConsumeNoDefault;
      break;

    case VIEWER_SHOW_CONTENT_QUALITY:
      if (nsnull != mDocShell) {
        nsIPresShell   *ps = GetPresShellFor(mDocShell);
        nsIViewManager *vm = nsnull;
        PRBool         qual;

        if (ps) {
          ps->GetViewManager(&vm);

          if (vm) {
            vm->GetShowQuality(qual);
            vm->ShowQuality(!qual);

            NS_RELEASE(vm);
          }

          NS_RELEASE(ps);
        }
      }

      result = nsEventStatus_eConsumeNoDefault;
      break;

    case VIEWER_DEBUGSAVE:
      DoDebugSave();
      break;

    case VIEWER_TOGGLE_SELECTION:
      DoToggleSelection();
      break;


    case VIEWER_DEBUGROBOT:
      DoDebugRobot();
      break;

    case VIEWER_TOP100:
      DoSiteWalker();
      break;
  }
  return(result);
}

#endif // NS_DEBUG

void 
nsBrowserWindow::SetCompatibilityMode(PRBool aIsStandard)
{
  nsCOMPtr<nsIPref> pref(do_GetService(NS_PREF_PROGID));
  if (pref) { 
    int32 prefInt = (aIsStandard) ? eCompatibility_Standard : eCompatibility_NavQuirks;
    pref->SetIntPref("nglayout.compatibility.mode", prefInt);
    pref->SavePrefFile();
  }
}

nsEventStatus
nsBrowserWindow::DispatchStyleMenu(PRInt32 aID)
{
  nsEventStatus result = nsEventStatus_eIgnore;

  switch(aID) {
  case VIEWER_SELECT_STYLE_LIST:
    {
      nsIPresShell* shell = GetPresShell();
      if (nsnull != shell) {
        nsAutoString  defaultStyle;
        nsCOMPtr<nsIDocument> doc;
        shell->GetDocument(getter_AddRefs(doc));
        if (doc) {
          nsIAtom* defStyleAtom = NS_NewAtom("default-style");
          doc->GetHeaderData(defStyleAtom, defaultStyle);
          NS_RELEASE(defStyleAtom);
        }

        nsStringArray titles;
        shell->ListAlternateStyleSheets(titles);
        nsAutoString  current;
        shell->GetActiveAlternateStyleSheet(current);

        PRInt32 count = titles.Count();
        fprintf(stdout, "There %s %d alternate style sheet%s\n",  
                ((1 == count) ? "is" : "are"),
                count,
                ((1 == count) ? "" : "s"));
        PRInt32 index;
        for (index = 0; index < count; index++) {
          fprintf(stdout, "%d: \"", index + 1);
          nsAutoString title;
          titles.StringAt(index, title);
          fputs(title, stdout);
          fprintf(stdout, "\" %s%s\n", 
                  ((title.EqualsIgnoreCase(current)) ? "<< current " : ""), 
                  ((title.EqualsIgnoreCase(defaultStyle)) ? "** default" : ""));
        }
        NS_RELEASE(shell);
      }
    }
    result = nsEventStatus_eConsumeNoDefault;
    break;

  case VIEWER_SELECT_STYLE_DEFAULT:
    {
      nsIPresShell* shell = GetPresShell();
      if (nsnull != shell) {
        nsCOMPtr<nsIDocument> doc;
        shell->GetDocument(getter_AddRefs(doc));
        if (doc) {
          nsAutoString  defaultStyle;
          nsIAtom* defStyleAtom = NS_NewAtom("default-style");
          doc->GetHeaderData(defStyleAtom, defaultStyle);
          NS_RELEASE(defStyleAtom);
          fputs("Selecting default style sheet \"", stdout);
          fputs(defaultStyle, stdout);
          fputs("\"\n", stdout);
          shell->SelectAlternateStyleSheet(defaultStyle);
        }
        NS_RELEASE(shell);
      }
    }
    result = nsEventStatus_eConsumeNoDefault;
    break;

  case VIEWER_SELECT_STYLE_ONE:
  case VIEWER_SELECT_STYLE_TWO:
  case VIEWER_SELECT_STYLE_THREE:
  case VIEWER_SELECT_STYLE_FOUR:
    {
      nsIPresShell* shell = GetPresShell();
      if (nsnull != shell) {
        nsStringArray titles;
        shell->ListAlternateStyleSheets(titles);
        nsAutoString  title;
        titles.StringAt(aID - VIEWER_SELECT_STYLE_ONE, title);
        fputs("Selecting alternate style sheet \"", stdout);
        fputs(title, stdout);
        fputs("\"\n", stdout);
        shell->SelectAlternateStyleSheet(title);
        NS_RELEASE(shell);
      }
    }
    result = nsEventStatus_eConsumeNoDefault;
    break;

  case VIEWER_NAV_QUIRKS_MODE:
  case VIEWER_STANDARD_MODE:
    SetCompatibilityMode(VIEWER_STANDARD_MODE == aID);
    result = nsEventStatus_eConsumeNoDefault;
    break;
  }
  return result;
}

NS_IMETHODIMP nsBrowserWindow::EnsureWebBrowserChrome()
{
   if(mWebBrowserChrome)
      return NS_OK;

   mWebBrowserChrome = new nsWebBrowserChrome();
   NS_ENSURE_TRUE(mWebBrowserChrome, NS_ERROR_OUT_OF_MEMORY);

   NS_ADDREF(mWebBrowserChrome);
   mWebBrowserChrome->BrowserWindow(this);

   return NS_OK;
}

//----------------------------------------------------------------------

// Factory code for creating nsBrowserWindow's

class nsBrowserWindowFactory : public nsIFactory
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIFACTORY

  nsBrowserWindowFactory();
  virtual ~nsBrowserWindowFactory();
};

nsBrowserWindowFactory::nsBrowserWindowFactory()
{
  NS_INIT_REFCNT();
}

nsBrowserWindowFactory::~nsBrowserWindowFactory()
{
}

nsresult
nsBrowserWindowFactory::QueryInterface(const nsIID &aIID, void **aResult)
{
  if (aResult == NULL) {
    return NS_ERROR_NULL_POINTER;
  }

  // Always NULL result, in case of failure
  *aResult = NULL;

  if (aIID.Equals(kISupportsIID)) {
    *aResult = (void *)(nsISupports*)this;
  } else if (aIID.Equals(kIFactoryIID)) {
    *aResult = (void *)(nsIFactory*)this;
  }

  if (*aResult == NULL) {
    return NS_NOINTERFACE;
  }

  NS_ADDREF_THIS(); // Increase reference count for caller
  return NS_OK;
}

NS_IMPL_ADDREF(nsBrowserWindowFactory);
NS_IMPL_RELEASE(nsBrowserWindowFactory);

nsresult
nsBrowserWindowFactory::CreateInstance(nsISupports *aOuter,
                                       const nsIID &aIID,
                                       void **aResult)
{
  nsresult rv;
  nsBrowserWindow *inst;

  if (aResult == NULL) {
    return NS_ERROR_NULL_POINTER;
  }
  *aResult = NULL;
  if (nsnull != aOuter) {
    rv = NS_ERROR_NO_AGGREGATION;
    goto done;
  }

  NS_NEWXPCOM(inst, nsNativeBrowserWindow);
  if (inst == NULL) {
    rv = NS_ERROR_OUT_OF_MEMORY;
    goto done;
  }

  NS_ADDREF(inst);
  rv = inst->QueryInterface(aIID, aResult);
  NS_RELEASE(inst);

done:
  return rv;
}

nsresult
nsBrowserWindowFactory::LockFactory(PRBool aLock)
{
  // Not implemented in simplest case.
  return NS_OK;
}

nsresult NS_NewBrowserWindowFactory(nsIFactory** aFactory);
nsresult
NS_NewBrowserWindowFactory(nsIFactory** aFactory)
{
  nsresult rv = NS_OK;
  nsBrowserWindowFactory* inst;
  NS_NEWXPCOM(inst, nsBrowserWindowFactory);
  if (nsnull == inst) {
    rv = NS_ERROR_OUT_OF_MEMORY;
  }
  else {
    NS_ADDREF(inst);
  }
  *aFactory = inst;
  return rv;
}
