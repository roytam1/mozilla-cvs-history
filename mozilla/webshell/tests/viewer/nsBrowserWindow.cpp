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
#include "nsIThrobber.h"
#include "nsIDOMDocument.h"
#include "nsIURL.h"
#include "nsIFileWidget.h"
#include "nsILookAndFeel.h"
#include "nsRepository.h"
#include "nsIFactory.h"
#include "nsCRT.h"
#include "nsWidgetsCID.h"
#include "nsViewerApp.h"
#include "prprf.h"
#include "nsRepository.h"
#include "nsParserCIID.h"

#include "nsIDocument.h"
#include "nsIPresContext.h"
#include "nsIDocumentViewer.h"
#include "nsIContentViewer.h"
#include "nsIPresShell.h"
#include "nsIDocument.h"
#include "nsXIFDTD.h"
#include "nsIParser.h"
#include "nsHTMLContentSinkStream.h"
#include "nsEditorMode.h"

// Needed for "Find" GUI
#include "nsIDialog.h"
#include "nsICheckButton.h"
#include "nsIRadioButton.h"
#include "nsILabel.h"
#include "nsWidgetSupport.h"


#include "resources.h"

#if defined(WIN32)
#include <strstrea.h>
#else
#if defined(XP_MAC)
#include "ostrstream.h"
#else
#include <strstream.h>
#endif
#endif

#if defined(WIN32)
#include <windows.h>
#endif


// XXX For font setting below
#include "nsFont.h"
#include "nsUnitConversion.h"
#include "nsIDeviceContext.h"

// XXX greasy constants
#define THROBBER_WIDTH 32
#define THROBBER_HEIGHT 32
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

static NS_DEFINE_IID(kLookAndFeelCID, NS_LOOKANDFEEL_CID);
static NS_DEFINE_IID(kBrowserWindowCID, NS_BROWSER_WINDOW_CID);
static NS_DEFINE_IID(kButtonCID, NS_BUTTON_CID);
static NS_DEFINE_IID(kFileWidgetCID, NS_FILEWIDGET_CID);
static NS_DEFINE_IID(kTextFieldCID, NS_TEXTFIELD_CID);
static NS_DEFINE_IID(kThrobberCID, NS_THROBBER_CID);
static NS_DEFINE_IID(kWebShellCID, NS_WEB_SHELL_CID);
static NS_DEFINE_IID(kWindowCID, NS_WINDOW_CID);
static NS_DEFINE_IID(kDialogCID, NS_DIALOG_CID);
static NS_DEFINE_IID(kCheckButtonCID, NS_CHECKBUTTON_CID);
static NS_DEFINE_IID(kRadioButtonCID, NS_RADIOBUTTON_CID);
static NS_DEFINE_IID(kLabelCID, NS_LABEL_CID);

static NS_DEFINE_IID(kILookAndFeelIID, NS_ILOOKANDFEEL_IID);
static NS_DEFINE_IID(kIBrowserWindowIID, NS_IBROWSER_WINDOW_IID);
static NS_DEFINE_IID(kIButtonIID, NS_IBUTTON_IID);
static NS_DEFINE_IID(kIDOMDocumentIID, NS_IDOMDOCUMENT_IID);
static NS_DEFINE_IID(kIFactoryIID, NS_IFACTORY_IID);
static NS_DEFINE_IID(kIFileWidgetIID, NS_IFILEWIDGET_IID);
static NS_DEFINE_IID(kIStreamObserverIID, NS_ISTREAMOBSERVER_IID);
static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);
static NS_DEFINE_IID(kITextWidgetIID, NS_ITEXTWIDGET_IID);
static NS_DEFINE_IID(kIThrobberIID, NS_ITHROBBER_IID);
static NS_DEFINE_IID(kIWebShellIID, NS_IWEB_SHELL_IID);
static NS_DEFINE_IID(kIWebShellContainerIID, NS_IWEB_SHELL_CONTAINER_IID);
static NS_DEFINE_IID(kIWidgetIID, NS_IWIDGET_IID);
static NS_DEFINE_IID(kIDialogIID, NS_IDIALOG_IID);
static NS_DEFINE_IID(kICheckButtonIID, NS_ICHECKBUTTON_IID);
static NS_DEFINE_IID(kIRadioButtonIID, NS_IRADIOBUTTON_IID);
static NS_DEFINE_IID(kILabelIID, NS_ILABEL_IID);


static const char* gsAOLFormat = "AOLMAIL";
static const char* gsHTMLFormat = "text/html";


// prototypes
static nsEventStatus PR_CALLBACK HandleEvent(nsGUIEvent *aEvent);
static void* GetItemsNativeData(nsISupports* aObject);

//----------------------------------------------------------------------

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
        bw->mWindow->QueryInterface(kIWidgetIID, (void**) &widget);
        if (widget == aWidget) {
          result = bw;
        }
        NS_IF_RELEASE(widget);
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
  nsViewerApp* app = aBrowser->mApp;
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

    case NS_DESTROY:
      {
        nsViewerApp* app = bw->mApp;
        result = nsEventStatus_eConsumeDoDefault;
        bw->Close();
        NS_RELEASE(bw);

        // XXX Really shouldn't just exit, we should just notify somebody...
        if (0 == nsBrowserWindow::gBrowsers.Count()) {
          app->Exit();
        }
      }
      return result;

    case NS_MENU_SELECTED:
      result = bw->DispatchMenuItem(((nsMenuEvent*)aEvent)->menuItem);
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
        bw->GoTo(text);
      }
      break;
    default:
      break;
    }
    NS_RELEASE(bw);
  }
  return result;
}

nsEventStatus
nsBrowserWindow::DispatchMenuItem(PRInt32 aID)
{
#ifdef NS_DEBUG
  nsEventStatus result = DispatchDebugMenu(aID);
  if (nsEventStatus_eIgnore != result) {
    return result;
  }
#endif
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
  
  case VIEWER_EDIT_COPY:
    DoCopy();
    break;

  case VIEWER_EDIT_SELECTALL:
    DoSelectAll();
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
    {
      PRIntn ix = aID - VIEWER_DEMO0;
      nsAutoString url(SAMPLES_BASE_URL);
      url.Append("/test");
      url.Append(ix, 10);
      url.Append(".html");
      mWebShell->LoadURL(url);
    }
    break;
  case JS_CONSOLE:
    DoJSConsole();
    break;

  case EDITOR_MODE:
    DoEditorMode(mWebShell);
    break;
  }

  return nsEventStatus_eIgnore;
}

void
nsBrowserWindow::Back()
{
  mWebShell->Back();
}

void
nsBrowserWindow::Forward()
{
  mWebShell->Forward();
}

void
nsBrowserWindow::GoTo(const PRUnichar* aURL)
{
  mWebShell->LoadURL(aURL, nsnull);
}

#define FILE_PROTOCOL "file://"

static PRBool GetFileNameFromFileSelector(nsIWidget* aParentWindow,
                                          nsString* aFileName)
{
  PRBool selectedFileName = PR_FALSE;
  nsIFileWidget *fileWidget;
  nsString title("Open HTML");
  nsresult rv = nsRepository::CreateInstance(kFileWidgetCID,
                                             nsnull,
                                             kIFileWidgetIID,
                                             (void**)&fileWidget);
  if (NS_OK == rv) {
    nsString titles[] = {"all files","html" };
    nsString filters[] = {"*.*", "*.html"};
    fileWidget->SetFilterList(2, titles, filters);
    fileWidget->Create(aParentWindow,
                       title,
                       eMode_load,
                       nsnull,
                       nsnull);

    PRUint32 result = fileWidget->Show();
    if (result) {
      fileWidget->GetFile(*aFileName);
      selectedFileName = PR_TRUE;
    }
 
    NS_RELEASE(fileWidget);
  }

  return selectedFileName;
}

void
nsBrowserWindow::DoFileOpen()
{
  nsAutoString fileName;
  char szFile[1000];
  if (GetFileNameFromFileSelector(mWindow, &fileName)) {
    fileName.ToCString(szFile, sizeof(szFile));
    PRInt32 len = strlen(szFile);
    PRInt32 sum = len + sizeof(FILE_PROTOCOL);
    char* lpszFileURL = new char[sum];
    
    // Translate '\' to '/'
    for (PRInt32 i = 0; i < len; i++) {
      if (szFile[i] == '\\') {
        szFile[i] = '/';
      }
    }

    // Build the file URL
    PR_snprintf(lpszFileURL, sum, "%s%s", FILE_PROTOCOL, szFile);

    // Ask the Web widget to load the file URL
    mWebShell->LoadURL(nsString(lpszFileURL));
    delete lpszFileURL;
  }
}

#define DIALOG_FONT      "Helvetica"
#define DIALOG_FONT_SIZE 10

/**--------------------------------------------------------------------------------
 * Main Handler
 *--------------------------------------------------------------------------------
 */
nsEventStatus PR_CALLBACK HandleEvent(nsGUIEvent *aEvent)
{
  //printf("HandleEvent aEvent->message %d\n", aEvent->message);
  nsEventStatus result = nsEventStatus_eIgnore;
  if (aEvent == nsnull ||  aEvent->widget == nsnull) {
    return result;
  }

  if (aEvent->message == 301 || aEvent->message == 302) {
    int x = 0;
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



static void* GetItemsNativeData(nsISupports* aObject)
{
	void* 			result = nsnull;
	nsIWidget* 	widget;
	if (NS_OK == aObject->QueryInterface(kIWidgetIID,(void**)&widget))
	{
		result = widget->GetNativeData(NS_NATIVE_WIDGET);
		NS_RELEASE(widget);
	}
	return result;
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
          nsKeyEvent* keyEvent = (nsKeyEvent*)aEvent;
          if (NS_VK_RETURN == keyEvent->keyCode) {
            PRBool matchCase   = PR_FALSE;
            mMatchCheckBtn->GetState(matchCase);
            PRBool findDwn     = PR_FALSE;
            mDwnRadioBtn->GetState(findDwn);
            nsString searchStr;
            PRUint32 actualSize;
            mTextField->GetText(searchStr, 255,actualSize);

            nsIPresShell* shell = GetPresShell();
            if (nsnull != shell) {
              nsIDocument* doc = shell->GetDocument();
              if (nsnull != doc) {
                PRBool foundIt = PR_FALSE;
                doc->FindNext(searchStr, matchCase, findDwn, foundIt);
                if (!foundIt) {
                  // Display Dialog here
                }
                ForceRefresh();
                NS_RELEASE(doc);
              }
              NS_RELEASE(shell);
            }
          }
        } break;

        case NS_MOUSE_LEFT_BUTTON_UP: {
        	nsIWidget* dialogWidget = nsnull;        	
         	if (NS_OK !=  mDialog->QueryInterface(kIWidgetIID,(void**)&dialogWidget))
         		break;
 				
          if (aEvent->widget->GetNativeData(NS_NATIVE_WIDGET) == GetItemsNativeData(mCancelBtn)) {
            dialogWidget->Show(PR_FALSE);
          } else if (aEvent->widget->GetNativeData(NS_NATIVE_WIDGET) == GetItemsNativeData(mFindBtn)) {

            PRBool matchCase   = PR_FALSE;
            mMatchCheckBtn->GetState(matchCase);
            PRBool findDwn     = PR_FALSE;
            mDwnRadioBtn->GetState(findDwn);
            PRUint32 actualSize;
            nsString searchStr;
            mTextField->GetText(searchStr, 255,actualSize);

            nsIPresShell* shell = GetPresShell();
            if (nsnull != shell) {
              nsIDocument* doc = shell->GetDocument();
              if (nsnull != doc) {
                PRBool foundIt = PR_FALSE;
                doc->FindNext(searchStr, matchCase, findDwn, foundIt);
                if (!foundIt) {
                  // Display Dialog here
                }
                ForceRefresh();
                NS_RELEASE(doc);
              }
              NS_RELEASE(shell);
            }

          } else if (aEvent->widget->GetNativeData(NS_NATIVE_WIDGET) == GetItemsNativeData(mUpRadioBtn)) {
            mUpRadioBtn->SetState(PR_TRUE);
            mDwnRadioBtn->SetState(PR_FALSE);
          } else if (aEvent->widget->GetNativeData(NS_NATIVE_WIDGET) == GetItemsNativeData(mDwnRadioBtn)) {
            mDwnRadioBtn->SetState(PR_TRUE);
            mUpRadioBtn->SetState(PR_FALSE);
          } else if (aEvent->widget->GetNativeData(NS_NATIVE_WIDGET) == GetItemsNativeData(mMatchCheckBtn)) {
            PRBool state = PR_FALSE;
          	mMatchCheckBtn->GetState(state);
            mMatchCheckBtn->SetState(!state);
          }
          } break;
        
        case NS_PAINT: 
#ifndef XP_UNIX
              // paint the background
            if (aEvent->widget->GetNativeData(NS_NATIVE_WIDGET) == GetItemsNativeData(mDialog)) {
                nsIRenderingContext *drawCtx = ((nsPaintEvent*)aEvent)->renderingContext;
                drawCtx->SetColor(aEvent->widget->GetBackgroundColor());
                drawCtx->FillRect(*(((nsPaintEvent*)aEvent)->rect));

                return nsEventStatus_eIgnore;
            }
#endif
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
  if (mDialog == nsnull) {
    nscoord txtHeight   = 24;
    nscolor textBGColor = NS_RGB(0, 0, 0);
    nscolor textFGColor = NS_RGB(255, 255, 255);

    nsILookAndFeel * lookAndFeel;
    if (NS_OK == nsRepository::CreateInstance(kLookAndFeelCID, nsnull, kILookAndFeelIID, (void**)&lookAndFeel)) {
       lookAndFeel->GetMetric(nsILookAndFeel::eMetric_TextFieldHeight, txtHeight);
       lookAndFeel->GetColor(nsILookAndFeel::eColor_TextBackground, textBGColor);
       lookAndFeel->GetColor(nsILookAndFeel::eColor_TextForeground, textFGColor);
       NS_RELEASE(lookAndFeel);
    }


    nsIDeviceContext* dc = mWindow->GetDeviceContext();
    float t2d;
    dc->GetTwipsToDevUnits(t2d);
    nsFont font(DIALOG_FONT, NS_FONT_STYLE_NORMAL, NS_FONT_VARIANT_NORMAL,
                NS_FONT_WEIGHT_NORMAL, 0,
                nscoord(t2d * NSIntPointsToTwips(DIALOG_FONT_SIZE)));
    NS_RELEASE(dc);

    // create a Dialog
    //
    nsRect rect;
    rect.SetRect(0, 0, 380, 110);  

    nsRepository::CreateInstance(kDialogCID, nsnull, kIDialogIID, (void**)&mDialog);
    nsIWidget* widget = nsnull;
    NS_CreateDialog(mWindow,mDialog,rect,HandleEvent,&font);
    if (NS_OK == mDialog->QueryInterface(kIWidgetIID,(void**)&widget))
    {
    	widget->SetClientData(this);
    	NS_RELEASE(widget);
    }
    mDialog->SetLabel("Find");

    nscoord xx = 5;
    // Create Label
    rect.SetRect(xx, 8, 75, 24);  
    nsRepository::CreateInstance(kLabelCID, nsnull, kILabelIID, (void**)&mLabel);
    NS_CreateLabel(mDialog,mLabel,rect,HandleEvent,&font);
    if (NS_OK == mLabel->QueryInterface(kIWidgetIID,(void**)&widget))
    {
    	widget->SetClientData(this);
    	mLabel->SetAlignment(eAlign_Right);
    	mLabel->SetLabel("Find what:");
    	NS_RELEASE(widget);
    }
    xx += 75 + 5;

    // Create TextField
    rect.SetRect(xx, 5, 200, txtHeight);  
    nsRepository::CreateInstance(kTextFieldCID, nsnull, kITextWidgetIID, (void**)&mTextField);
    NS_CreateTextWidget(mDialog,mTextField,rect,HandleEvent,&font);
    if (NS_OK == mTextField->QueryInterface(kIWidgetIID,(void**)&widget))
    {
      widget->SetBackgroundColor(textBGColor);
      widget->SetForegroundColor(textFGColor);
      widget->SetClientData(this);
      widget->SetFocus();
    	NS_RELEASE(widget);
    }
    xx += 200 + 5;
  
    nscoord w = 65;
    nscoord x = 205+80-w;
    nscoord y = txtHeight + 10;
    nscoord h = 19;

    // Create Up RadioButton
    rect.SetRect(x, y, w, h);  
    nsRepository::CreateInstance(kRadioButtonCID, nsnull, kIRadioButtonIID, (void**)&mUpRadioBtn);
    NS_CreateRadioButton(mDialog,mUpRadioBtn,rect,HandleEvent,&font);
    if (NS_OK == mUpRadioBtn->QueryInterface(kIWidgetIID,(void**)&widget))
    {
      widget->SetClientData(this);
      mUpRadioBtn->SetLabel("Up");
    	NS_RELEASE(widget);
    }
    y += h + 2;
  
    // Create Up RadioButton
    rect.SetRect(x, y, w, h);  
    nsRepository::CreateInstance(kRadioButtonCID, nsnull, kIRadioButtonIID, (void**)&mDwnRadioBtn);
    NS_CreateRadioButton(mDialog,mDwnRadioBtn,rect,HandleEvent,&font);
    if (NS_OK == mDwnRadioBtn->QueryInterface(kIWidgetIID,(void**)&widget))
    {
	    widget->SetClientData(this);
	    mDwnRadioBtn->SetLabel("Down");
    	NS_RELEASE(widget);
    }
  
    // Create Match CheckButton
    rect.SetRect(5, y, 125, 24);  
    nsRepository::CreateInstance(kCheckButtonCID, nsnull, kICheckButtonIID, (void**)&mMatchCheckBtn);
    NS_CreateCheckButton(mDialog,mMatchCheckBtn,rect,HandleEvent,&font);
    if (NS_OK == mMatchCheckBtn->QueryInterface(kIWidgetIID,(void**)&widget))
    {
	    widget->SetClientData(this);
	    mMatchCheckBtn->SetLabel("Match Case");
    	NS_RELEASE(widget);
    }

    mUpRadioBtn->SetState(PR_FALSE);
    mDwnRadioBtn->SetState(PR_TRUE);
  
    // Create Find Next Button
    rect.SetRect(xx, 5, 75, 24);  
    nsRepository::CreateInstance(kButtonCID, nsnull, kIButtonIID, (void**)&mFindBtn);
    NS_CreateButton(mDialog,mFindBtn,rect,HandleEvent,&font);
    if (NS_OK == mFindBtn->QueryInterface(kIWidgetIID,(void**)&widget))
    {
	    widget->SetClientData(this);
	    mFindBtn->SetLabel("Find Next");
    	NS_RELEASE(widget);
    }
  
    // Create Cancel Button
    rect.SetRect(xx, 35, 75, 24);  
    nsRepository::CreateInstance(kButtonCID, nsnull, kIButtonIID, (void**)&mCancelBtn);
    NS_CreateButton(mDialog,mCancelBtn,rect,HandleEvent,&font);
    if (NS_OK == mCancelBtn->QueryInterface(kIWidgetIID,(void**)&widget))
    {
	    widget->SetClientData(this);
	    mCancelBtn->SetLabel("Cancel");
    	NS_RELEASE(widget);
    }  
  }
  mTextField->SelectAll();

}

void
nsBrowserWindow::DoSelectAll()
{

  nsIPresShell* shell = GetPresShell();
  if (nsnull != shell) {
    nsIDocument* doc = shell->GetDocument();
    if (nsnull != doc) {
      doc->SelectAll();
      ForceRefresh();
      NS_RELEASE(doc);
    }
    NS_RELEASE(shell);
  }
}

void
nsBrowserWindow::ForceRefresh()
{
  nsIPresShell* shell = GetPresShell();
  if (nsnull != shell) {
    nsIViewManager* vm = shell->GetViewManager();
    if (nsnull != vm) {
      nsIView* root;
      vm->GetRootView(root);
      if (nsnull != root) {
        vm->UpdateView(root, (nsIRegion*)nsnull, NS_VMREFRESH_IMMEDIATE);
      }
      NS_RELEASE(vm);
    }
    NS_RELEASE(shell);
  }
}


//----------------------------------------------------------------------

// Note: operator new zeros our memory
nsBrowserWindow::nsBrowserWindow()
{
  AddBrowser(this);
}

nsBrowserWindow::~nsBrowserWindow()
{
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
  if (aIID.Equals(kIStreamObserverIID)) {
    *aInstancePtrResult = (void*) ((nsIStreamObserver*)this);
    NS_ADDREF_THIS();
    return NS_OK;
  }
  if (aIID.Equals(kIWebShellContainerIID)) {
    *aInstancePtrResult = (void*) ((nsIWebShellContainer*)this);
    NS_ADDREF_THIS();
    return NS_OK;
  }
  if (aIID.Equals(kINetSupportIID)) {
    *aInstancePtrResult = (void*) ((nsINetSupport*)this);
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
                      nsIPref* aPrefs,
                      const nsRect& aBounds,
                      PRUint32 aChromeMask,
                      PRBool aAllowPlugins)
{
  mChromeMask = aChromeMask;
  mAppShell = aAppShell;
  mPrefs = aPrefs;
  mAllowPlugins = aAllowPlugins;

  // Create top level window
  nsresult rv = nsRepository::CreateInstance(kWindowCID, nsnull, kIWidgetIID,
                                             (void**)&mWindow);
  if (NS_OK != rv) {
    return rv;
  }
  nsRect r(0, 0, aBounds.width, aBounds.height);
  mWindow->Create((nsIWidget*)NULL, r, HandleBrowserEvent,
                  nsnull, aAppShell);
  mWindow->GetBounds(r);

  // Create web shell
  rv = nsRepository::CreateInstance(kWebShellCID, nsnull,
                                    kIWebShellIID,
                                    (void**)&mWebShell);
  if (NS_OK != rv) {
    return rv;
  }
  r.x = r.y = 0;
  rv = mWebShell->Init(mWindow->GetNativeData(NS_NATIVE_WIDGET), 
                       r.x, r.y, r.width, r.height,
                       nsScrollPreference_kAuto, aAllowPlugins);
  mWebShell->SetContainer((nsIWebShellContainer*) this);
  mWebShell->SetObserver((nsIStreamObserver*)this);
  mWebShell->SetPrefs(aPrefs);
  mWebShell->Show();

  if (NS_CHROME_MENU_BAR_ON & aChromeMask) {
    rv = CreateMenuBar(r.width);
    if (NS_OK != rv) {
      return rv;
    }
    mWindow->GetBounds(r);
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

  // Now lay it all out
  Layout(r.width, r.height);


  return NS_OK;
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
  nsFont font(TOOL_BAR_FONT, NS_FONT_STYLE_NORMAL, NS_FONT_VARIANT_NORMAL,
              NS_FONT_WEIGHT_NORMAL, 0,
              nscoord(t2d * NSIntPointsToTwips(TOOL_BAR_FONT_SIZE)));
  NS_RELEASE(dc);

  // Create and place back button
  rv = nsRepository::CreateInstance(kButtonCID, nsnull, kIButtonIID,
                                    (void**)&mBack);
  if (NS_OK != rv) {
    return rv;
  }
  nsRect r(0, 0, BUTTON_WIDTH, BUTTON_HEIGHT);
	
	nsIWidget* widget = nsnull;

  NS_CreateButton(mWindow,mBack,r,HandleBackEvent,&font);
  if (NS_OK == mBack->QueryInterface(kIWidgetIID,(void**)&widget))
	{
    mBack->SetLabel("Back");
		NS_RELEASE(widget);
	}


  // Create and place forward button
  r.SetRect(BUTTON_WIDTH, 0, BUTTON_WIDTH, BUTTON_HEIGHT);  
  rv = nsRepository::CreateInstance(kButtonCID, nsnull, kIButtonIID,
                                    (void**)&mForward);
  if (NS_OK != rv) {
    return rv;
  }
	if (NS_OK == mForward->QueryInterface(kIWidgetIID,(void**)&widget))
	{
    widget->Create(mWindow, r, HandleBackEvent, NULL);
    widget->SetFont(font);
    widget->Show(PR_TRUE);
    mForward->SetLabel("Forward");
		NS_RELEASE(widget);
	}


  // Create and place location bar
  r.SetRect(2*BUTTON_WIDTH, 0,
            aWidth - 2*BUTTON_WIDTH - THROBBER_WIDTH,
            BUTTON_HEIGHT);
  rv = nsRepository::CreateInstance(kTextFieldCID, nsnull, kITextWidgetIID,
                                    (void**)&mLocation);
  if (NS_OK != rv) {
    return rv;
  }

  NS_CreateTextWidget(mWindow,mLocation,r,HandleLocationEvent,&font);
	if (NS_OK == mLocation->QueryInterface(kIWidgetIID,(void**)&widget))
  { 
    widget->SetForegroundColor(NS_RGB(0, 0, 0));
    widget->SetBackgroundColor(NS_RGB(255, 255, 255));
    PRUint32 size;
    mLocation->SetText("",size);
		NS_RELEASE(widget);
  }

  // Create and place throbber
  r.SetRect(aWidth - THROBBER_WIDTH, 0,
            THROBBER_WIDTH, THROBBER_HEIGHT);
  rv = nsRepository::CreateInstance(kThrobberCID, nsnull, kIThrobberIID,
                                    (void**)&mThrobber);
  if (NS_OK != rv) {
    return rv;
  }
  mThrobber->Init(mWindow, r);
  mThrobber->Show();

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
  rv = nsRepository::CreateInstance(kTextFieldCID, nsnull, kITextWidgetIID,
                                    (void**)&mStatus);
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
    NS_RELEASE(widget);
  }

  return NS_OK;
}

void
nsBrowserWindow::Layout(PRInt32 aWidth, PRInt32 aHeight)
{
  nscoord txtHeight;
  nsILookAndFeel * lookAndFeel;
  if (NS_OK == nsRepository::CreateInstance(kLookAndFeelCID, nsnull, kILookAndFeelIID, (void**)&lookAndFeel)) {
    lookAndFeel->GetMetric(nsILookAndFeel::eMetric_TextFieldHeight, txtHeight);
    NS_RELEASE(lookAndFeel);
  } else {
    txtHeight = 24;
  }

  nsIWidget* locationWidget = nsnull;

  if (NS_OK == mLocation->QueryInterface(kIWidgetIID,(void**)&locationWidget))
  {
    // position location bar (it's stretchy)
    if (mLocation) {
      if (mThrobber) {
        locationWidget->Resize(2*BUTTON_WIDTH, 0,
                          aWidth - (2*BUTTON_WIDTH + THROBBER_WIDTH),
                          BUTTON_HEIGHT,
                          PR_TRUE);
      }
      else {
        locationWidget->Resize(2*BUTTON_WIDTH, 0,
                          aWidth - 2*BUTTON_WIDTH,
                          BUTTON_HEIGHT,
                          PR_TRUE);
      }
    }

    if (mThrobber) {
      mThrobber->MoveTo(aWidth - THROBBER_WIDTH, 0);
    }

    nsRect rr(0, 0, aWidth, aHeight);

    if (mLocation) {
      rr.y += BUTTON_HEIGHT;
      rr.height -= BUTTON_HEIGHT;
    }

    nsIWidget* statusWidget = nsnull;

    if (NS_OK == mStatus->QueryInterface(kIWidgetIID,(void**)&statusWidget))
    {
      if (mStatus) {
        statusWidget->Resize(0, aHeight - txtHeight,
                        aWidth, txtHeight,
                        PR_TRUE);
        rr.height -= txtHeight;
      }
    }
    // inset the web widget
    rr.x += WEBSHELL_LEFT_INSET;
    rr.y += WEBSHELL_TOP_INSET;
    rr.width -= WEBSHELL_LEFT_INSET + WEBSHELL_RIGHT_INSET;
    rr.height -= WEBSHELL_TOP_INSET + WEBSHELL_BOTTOM_INSET;
    mWebShell->SetBounds(rr.x, rr.y, rr.width, rr.height);
    NS_RELEASE(locationWidget);
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
nsBrowserWindow::SizeTo(PRInt32 aWidth, PRInt32 aHeight)
{
  NS_PRECONDITION(nsnull != mWindow, "null window");

  // XXX We want to do this in one shot
  mWindow->Resize(aWidth, aHeight, PR_FALSE);
  Layout(aWidth, aHeight);

  return NS_OK;
}

NS_IMETHODIMP
nsBrowserWindow::GetBounds(nsRect& aBounds)
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

NS_IMETHODIMP
nsBrowserWindow::Close()
{
  RemoveBrowser(this);

  if (nsnull != mWebShell) {
    mWebShell->Destroy();
    NS_RELEASE(mWebShell);
  }

//  NS_IF_RELEASE(mWindow);
  if (nsnull != mWindow) {
    nsIWidget* w = mWindow;
    NS_RELEASE(w);
  }

  NS_IF_RELEASE(mBack);
  NS_IF_RELEASE(mForward);
  NS_IF_RELEASE(mLocation);
  NS_IF_RELEASE(mThrobber);
  NS_IF_RELEASE(mStatus);

  return NS_OK;
}


NS_IMETHODIMP
nsBrowserWindow::SetChrome(PRUint32 aChromeMask)
{
  // XXX write me
  mChromeMask = aChromeMask;
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
  aResult = mWebShell;
  NS_IF_ADDREF(mWebShell);
  return NS_OK;
}

//----------------------------------------

NS_IMETHODIMP
nsBrowserWindow::SetTitle(const PRUnichar* aTitle)
{
  NS_PRECONDITION(nsnull != mWindow, "null window");
  mTitle = aTitle;
  mWindow->SetTitle(aTitle);
  return NS_OK;
}

NS_IMETHODIMP
nsBrowserWindow::GetTitle(PRUnichar** aResult)
{
  *aResult = mTitle;
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
nsBrowserWindow::GetStatus(PRUnichar** aResult)
{
  return NS_OK;
}

NS_IMETHODIMP
nsBrowserWindow::SetProgress(PRInt32 aProgress, PRInt32 aProgressMax)
{
  return NS_OK;
}

NS_IMETHODIMP
nsBrowserWindow::WillLoadURL(nsIWebShell* aShell, const PRUnichar* aURL, nsLoadType aReason)
{
  if (mStatus) {
    nsAutoString url("Connecting to ");
    url.Append(aURL);
    PRUint32 size;
    mStatus->SetText(url,size);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsBrowserWindow::BeginLoadURL(nsIWebShell* aShell, const PRUnichar* aURL)
{
  if (mThrobber) {
    mThrobber->Start();
    PRUint32 size;
    mLocation->SetText(aURL,size);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsBrowserWindow::ProgressLoadURL(nsIWebShell* aShell, const PRUnichar* aURL, PRInt32 aProgress, PRInt32 aProgressMax)
{
  return NS_OK;
}

NS_IMETHODIMP
nsBrowserWindow::EndLoadURL(nsIWebShell* aShell, const PRUnichar* aURL, PRInt32 aStatus)
{
  if (mThrobber) {
    mThrobber->Stop();
  }
  return NS_OK;
}


NS_IMETHODIMP
nsBrowserWindow::NewWebShell(nsIWebShell*& aNewWebShell)
{
  nsresult rv = NS_OK;

  // Create new window. By default, the refcnt will be 1 because of
  // the registration of the browser window in gBrowsers.
  nsNativeBrowserWindow* browser;
  NS_NEWXPCOM(browser, nsNativeBrowserWindow);

  if (nsnull != browser)
  {
    nsRect  bounds;
    GetBounds(bounds);

    browser->SetApp(mApp);
    rv = browser->Init(mAppShell, mPrefs, bounds, mChromeMask, mAllowPlugins);
    if (NS_OK == rv)
    {
      browser->Show();
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

//----------------------------------------

// Stream observer implementation

NS_IMETHODIMP
nsBrowserWindow::OnProgress(nsIURL* aURL,
                            PRInt32 aProgress,
                            PRInt32 aProgressMax)
{
  if (mStatus) {
    nsAutoString url;
    if (nsnull != aURL) {
      aURL->ToString(url);
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
nsBrowserWindow::OnStatus(nsIURL* aURL, const nsString& aMsg)
{
  if (mStatus) {
    PRUint32 size;
    mStatus->SetText(aMsg,size);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsBrowserWindow::OnStartBinding(nsIURL* aURL, const char *aContentType)
{
  if (mStatus) {
    nsAutoString url;
    if (nsnull != aURL) {
      aURL->ToString(url);
    }
    url.Append(": start");
    PRUint32 size;
    mStatus->SetText(url,size);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsBrowserWindow::OnStopBinding(nsIURL* aURL,
                               PRInt32 status,
                               const nsString& aMsg)
{
  if (mThrobber) {
    mThrobber->Stop();
  }
  if (mStatus) {
    nsAutoString url;
    if (nsnull != aURL) {
      aURL->ToString(url);
    }
    url.Append(": stop");
    PRUint32 size;
    mStatus->SetText(url,size);
  }
  return NS_OK;
}

NS_IMETHODIMP_(void)
nsBrowserWindow::Alert(const nsString &aText)
{
  nsAutoString str(aText);
  printf("Browser Window Alert: %s\n", str);
}

NS_IMETHODIMP_(PRBool)
nsBrowserWindow::Confirm(const nsString &aText)
{
  nsAutoString str(aText);
  printf("Browser Window Confirm: %s (returning false)\n", str);

  return PR_FALSE;
}

NS_IMETHODIMP_(PRBool)
nsBrowserWindow::Prompt(const nsString &aText,
                        const nsString &aDefault,
                        nsString &aResult)
{
  nsAutoString str(aText);
  char buf[256];
  printf("Browser Window: %s\n", str);
  printf("Prompt: ");
  scanf("%s", buf);
  aResult = buf;
  
  return (aResult.Length() > 0);
}

NS_IMETHODIMP_(PRBool) 
nsBrowserWindow::PromptUserAndPassword(const nsString &aText,
                                       nsString &aUser,
                                       nsString &aPassword)
{
  nsAutoString str(aText);
  char buf[256];
  printf("Browser Window: %s\n", str);
  printf("User: ");
  scanf("%s", buf);
  aUser = buf;
  printf("Password: ");
  scanf("%s", buf);
  aPassword = buf;
  
  return (aUser.Length() > 0);
}

NS_IMETHODIMP_(PRBool) 
nsBrowserWindow::PromptPassword(const nsString &aText,
                                nsString &aPassword)
{
  nsAutoString str(aText);
  char buf[256];
  printf("Browser Window: %s\n", str);
  printf("Password: ");
  scanf("%s", buf);
  aPassword = buf;
 
  return PR_TRUE;
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

static NS_DEFINE_IID(kIDocumentViewerIID, NS_IDOCUMENT_VIEWER_IID);

nsIPresShell*
nsBrowserWindow::GetPresShell()
{
  nsIPresShell* shell = nsnull;
  if (nsnull != mWebShell) {
    nsIContentViewer* cv = nsnull;
    mWebShell->GetContentViewer(cv);
    if (nsnull != cv) {
      nsIDocumentViewer* docv = nsnull;
      cv->QueryInterface(kIDocumentViewerIID, (void**) &docv);
      if (nsnull != docv) {
        nsIPresContext* cx;
        docv->GetPresContext(cx);
        if (nsnull != cx) {
          shell = cx->GetShell();
          NS_RELEASE(cx);
        }
        NS_RELEASE(docv);
      }
      NS_RELEASE(cv);
    }
  }
  return shell;
}

#ifdef WIN32
void PlaceHTMLOnClipboard(PRUint32 aFormat, char* aData, int aLength)
{
  HGLOBAL     hGlobalMemory;
  PSTR        pGlobalMemory;

  PRUint32    cf_aol = RegisterClipboardFormat(gsAOLFormat);
  PRUint32    cf_html = RegisterClipboardFormat(gsHTMLFormat);

  char*       preamble = "";
  char*       postamble = "";

  if (aFormat == cf_aol || aFormat == CF_TEXT)
  {
    preamble = "<HTML>";
    postamble = "</HTML>";
  }

  PRInt32 size = aLength + 1 + strlen(preamble) + strlen(postamble);


  if (aLength)
  {
    // Copy text to Global Memory Area
    hGlobalMemory = (HGLOBAL)GlobalAlloc(GHND, size);
    if (hGlobalMemory != NULL) 
    {
      pGlobalMemory = (PSTR) GlobalLock(hGlobalMemory);

      int i;

      // AOL requires HTML prefix/postamble
      char*     s  = preamble;
      PRInt32   len = strlen(s); 
      for (i=0; i < len; i++)
      {
        *pGlobalMemory++ = *s++;
      }

      s  = aData;
      len = aLength;
      for (i=0;i< len;i++) {
        *pGlobalMemory++ = *s++;
      }


      s = postamble;
      len = strlen(s); 
      for (i=0; i < len; i++)
      {
        *pGlobalMemory++ = *s++;
      }
      
      // Put data on Clipboard
      GlobalUnlock(hGlobalMemory);
      SetClipboardData(aFormat, hGlobalMemory);
    }
  }  
}
#endif



void
nsBrowserWindow::DoCopy()
{
  nsIPresShell* shell = GetPresShell();
  if (nsnull != shell) {
    nsIDocument* doc = shell->GetDocument();
    if (nsnull != doc) {
      nsString buffer;

      doc->CreateXIF(buffer,PR_TRUE);

      nsIParser* parser;

      static NS_DEFINE_IID(kCParserIID, NS_IPARSER_IID);
      static NS_DEFINE_IID(kCParserCID, NS_PARSER_IID);

      nsresult rv = nsRepository::CreateInstance(kCParserCID, 
                                                 nsnull, 
                                                 kCParserIID, 
                                                 (void **)&parser);

      if (NS_OK == rv) {
        nsIHTMLContentSink* sink = nsnull;
        
        rv = NS_New_HTML_ContentSinkStream(&sink,PR_FALSE,PR_FALSE);

        ostrstream  data;
        ((nsHTMLContentSinkStream*)sink)->SetOutputStream(data);

        if (NS_OK == rv) {
          parser->SetContentSink(sink);
          
          nsIDTD* dtd = nsnull;
          rv = NS_NewXIFDTD(&dtd);
          if (NS_OK == rv) 
          {
            parser->RegisterDTD(dtd);
            dtd->SetContentSink(sink);
            dtd->SetParser(parser);
            parser->Parse(buffer, PR_FALSE);           
          }
          NS_IF_RELEASE(dtd);
          NS_IF_RELEASE(sink);
          char* str = data.str();

#if defined(WIN32)
          PRUint32 cf_aol = RegisterClipboardFormat(gsAOLFormat);
          PRUint32 cf_html = RegisterClipboardFormat(gsHTMLFormat);
         
          PRInt32     len = data.pcount();
          if (len)
          {   
            OpenClipboard(NULL);
            EmptyClipboard();
        
            PlaceHTMLOnClipboard(cf_aol,str,len);
            PlaceHTMLOnClipboard(cf_html,str,len);
            PlaceHTMLOnClipboard(CF_TEXT,str,len);            
                        
            CloseClipboard();
          }
          // in ostrstreams if you cal the str() function
          // then you are responsible for deleting the string
#endif
          if (str) delete str;

        }
        NS_RELEASE(parser);
      }
      NS_RELEASE(doc);
    }
    NS_RELEASE(shell);
  }
}

//----------------------------------------------------------------------

void
nsBrowserWindow::DoJSConsole()
{
  mApp->CreateJSConsole(this);
}

void
nsBrowserWindow::DoEditorMode(nsIWebShell *aWebShell)
{
  PRInt32 i, n;
  if (nsnull != aWebShell) {
    nsIContentViewer* mCViewer;
    aWebShell->GetContentViewer(mCViewer);
    if (nsnull != mCViewer) {
      nsIDocumentViewer* mDViewer;
      if (NS_OK == mCViewer->QueryInterface(kIDocumentViewerIID, (void**) &mDViewer)) {
        nsIDocument* mDoc;
        mDViewer->GetDocument(mDoc);
        if (nsnull != mDoc) {
          nsIDOMDocument* mDOMDoc;
          if (NS_OK == mDoc->QueryInterface(kIDOMDocumentIID, (void**) &mDOMDoc)) {
            NS_InitEditorMode(mDOMDoc);
            NS_RELEASE(mDOMDoc);
          }
          NS_RELEASE(mDoc);
        }
        NS_RELEASE(mDViewer);
      }
      NS_RELEASE(mCViewer);
    }
    
    aWebShell->GetChildCount(n);
    for (i = 0; i < n; i++) {
      nsIWebShell* mChild;
      aWebShell->ChildAt(i, mChild);
      DoEditorMode(mChild);
      NS_RELEASE(mChild);
    }
  }
}

#ifdef NS_DEBUG
#include "nsIContent.h"
#include "nsIFrame.h"
#include "nsIStyleContext.h"
#include "nsISizeOfHandler.h"
#include "nsIStyleSet.h"


void
nsBrowserWindow::DumpContent(FILE* out)
{
  nsIPresShell* shell = GetPresShell();
  if (nsnull != shell) {
    nsIDocument* doc = shell->GetDocument();
    if (nsnull != doc) {
      nsIContent* root = doc->GetRootContent();
      if (nsnull != root) {
        root->List(out);
        NS_RELEASE(root);
      }
      NS_RELEASE(doc);
    }
    NS_RELEASE(shell);
  }
  else {
    fputs("null pres shell\n", out);
  }
}

void
nsBrowserWindow::DumpFrames(FILE* out, nsString *aFilterName)
{
  nsIPresShell* shell = GetPresShell();
  if (nsnull != shell) {
    nsIFrame* root = shell->GetRootFrame();
    if (nsnull != root) {
      nsIListFilter *filter = nsIFrame::GetFilter(aFilterName);
      root->List(out, 0, filter);
    }
    NS_RELEASE(shell);
  }
  else {
    fputs("null pres shell\n", out);
  }
}

void
DumpViewsRecurse(nsBrowserWindow* aBrowser, nsIWebShell* aWebShell, FILE* out)
{
  if (nsnull != aWebShell) {
    nsIPresShell* shell = aBrowser->GetPresShell();
    if (nsnull != shell) {
      nsIViewManager* vm = shell->GetViewManager();
      if (nsnull != vm) {
        nsIView* root;
        vm->GetRootView(root);
        if (nsnull != root) {
          root->List(out);
        }
        NS_RELEASE(vm);
      }
      NS_RELEASE(shell);
    }
    else {
      fputs("null pres shell\n", out);
    }
    // dump the views of the sub documents
    PRInt32 i, n;
    aWebShell->GetChildCount(n);
    for (i = 0; i < n; i++) {
      nsIWebShell* child;
      aWebShell->ChildAt(i, child);
      if (nsnull != child) {
        DumpViewsRecurse(aBrowser, child, out);
      }
    }
  }
}

void
nsBrowserWindow::DumpViews(FILE* out)
{
  DumpViewsRecurse(this, mWebShell, out);
}

static void DumpAWebShell(nsIWebShell* aShell, FILE* out, PRInt32 aIndent)
{
  PRUnichar *name;
  nsAutoString str;
  nsIWebShell* parent;
  PRInt32 i, n;

  for (i = aIndent; --i >= 0; ) fprintf(out, "  ");

  fprintf(out, "%p '", aShell);
  aShell->GetName(&name);
  aShell->GetParent(parent);
  str = name;
  fputs(str, out);
  fprintf(out, "' parent=%p <\n", parent);
  NS_IF_RELEASE(parent);

  aIndent++;
  aShell->GetChildCount(n);
  for (i = 0; i < n; i++) {
    nsIWebShell* child;
    aShell->ChildAt(i, child);
    if (nsnull != child) {
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
  DumpAWebShell(mWebShell, out, 0);
}

void
nsBrowserWindow::DumpStyleSheets(FILE* out)
{
  nsIPresShell* shell = GetPresShell();
  if (nsnull != shell) {
    nsIStyleSet* styleSet = shell->GetStyleSet();
    if (nsnull == styleSet) {
      fputs("null style set\n", out);
    } else {
      styleSet->List(out);
      NS_RELEASE(styleSet);
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
    nsIPresContext* cx = shell->GetPresContext();
    nsIStyleSet* styleSet = shell->GetStyleSet();
    if (nsnull == styleSet) {
      fputs("null style set\n", out);
    } else {
      nsIFrame* root = shell->GetRootFrame();
      if (nsnull == root) {
        fputs("null root frame\n", out);
      } else {
        nsIStyleContext* rootContext;
        root->GetStyleContext(cx, rootContext);
        if (nsnull != rootContext) {
          styleSet->ListContexts(rootContext, out);
          NS_RELEASE(rootContext);
        }
        else {
          fputs("null root context", out);
        }
      }
      NS_RELEASE(styleSet);
    }
    NS_IF_RELEASE(cx);
    NS_RELEASE(shell);
  } else {
    fputs("null pres shell\n", out);
  }
}

void
nsBrowserWindow::ToggleFrameBorders()
{
  PRBool showing = nsIFrame::GetShowFrameBorders();
  nsIFrame::ShowFrameBorders(!showing);
  ForceRefresh();
}

void
nsBrowserWindow::ShowContentSize()
{
  nsISizeOfHandler* szh;
  if (NS_OK != NS_NewSizeOfHandler(&szh)) {
    return;
  }

  nsIPresShell* shell = GetPresShell();
  if (nsnull != shell) {
    nsIDocument* doc = shell->GetDocument();
    if (nsnull != doc) {
      nsIContent* content = doc->GetRootContent();
      if (nsnull != content) {
        content->SizeOf(szh);
        PRUint32 totalSize;
        szh->GetSize(totalSize);
        printf("Content model size is approximately %d bytes\n", totalSize);
        NS_RELEASE(content);
      }
      NS_RELEASE(doc);
    }
    NS_RELEASE(shell);
  }
  NS_RELEASE(szh);
}

void
nsBrowserWindow::ShowFrameSize()
{
  nsIPresShell* shell0 = GetPresShell();
  if (nsnull != shell0) {
    nsIDocument* doc = shell0->GetDocument();
    if (nsnull != doc) {
      PRInt32 i, shells = doc->GetNumberOfShells();
      for (i = 0; i < shells; i++) {
        nsIPresShell* shell = doc->GetShellAt(i);
        if (nsnull != shell) {
          nsISizeOfHandler* szh;
          if (NS_OK != NS_NewSizeOfHandler(&szh)) {
            return;
          }
          nsIFrame* root;
          root = shell->GetRootFrame();
          if (nsnull != root) {
            root->SizeOf(szh);
            PRUint32 totalSize;
            szh->GetSize(totalSize);
            printf("Frame model for shell=%p size is approximately %d bytes\n",
                   shell, totalSize);
          }
          NS_RELEASE(szh);
          NS_RELEASE(shell);
        }
      }
      NS_RELEASE(doc);
    }
    NS_RELEASE(shell0);
  }
}

void
nsBrowserWindow::ShowStyleSize()
{
}




static PRBool GetSaveFileNameFromFileSelector(nsIWidget* aParentWindow,
                                              nsString&  aFileName)
{
  PRInt32 offset = aFileName.RFind('/');
  if (offset != -1)
    aFileName.Cut(0,offset+1);

  PRBool selectedFileName = PR_FALSE;
  nsIFileWidget *fileWidget;
  nsString title("Save HTML");
  nsresult rv = nsRepository::CreateInstance(kFileWidgetCID,
                                             nsnull,
                                             kIFileWidgetIID,
                                             (void**)&fileWidget);
  if (NS_OK == rv) {
    nsString titles[] = {"html","txt"};
    nsString filters[] = {"*.html", "*.txt"};
    fileWidget->SetFilterList(2, titles, filters);
    fileWidget->Create(aParentWindow,
                       title,
                       eMode_save,
                       nsnull,
                       nsnull);
    fileWidget->SetDefaultString(aFileName);

    PRUint32 result = fileWidget->Show();
    if (result) {
      fileWidget->GetFile(aFileName);
      selectedFileName = PR_TRUE;
    }
 
    NS_RELEASE(fileWidget);
  }

  return selectedFileName;
}




void
nsBrowserWindow::DoDebugSave()
{
  PRBool    doSave = PR_FALSE;
  nsString  path;

  PRUnichar *urlString;
  mWebShell->GetURL(0,&urlString);
  nsIURL* url;
  nsresult rv = NS_NewURL(&url, urlString);
  
  if (rv == NS_OK)
  {
    const char* name = url->GetFile();
    path = name;

    doSave = GetSaveFileNameFromFileSelector(mWindow, path);
    NS_RELEASE(url);

  }
  if (!doSave)
    return;


  nsIPresShell* shell = GetPresShell();
  if (nsnull != shell) {
    nsIDocument* doc = shell->GetDocument();
    if (nsnull != doc) {
      nsString buffer;

      doc->CreateXIF(buffer,PR_FALSE);

      nsIParser* parser;

      static NS_DEFINE_IID(kCParserIID, NS_IPARSER_IID);
      static NS_DEFINE_IID(kCParserCID, NS_PARSER_IID);

      nsresult rv = nsRepository::CreateInstance(kCParserCID, 
                                                 nsnull, 
                                                 kCParserIID, 
                                                 (void **)&parser);

      if (NS_OK == rv) {
        nsIHTMLContentSink* sink = nsnull;

        rv = NS_New_HTML_ContentSinkStream(&sink);
        
#ifdef WIN32
#define   BUFFER_SIZE MAX_PATH
#else
#define   BUFFER_SIZE 1024
#endif
        char filename[BUFFER_SIZE];
        path.ToCString(filename,BUFFER_SIZE);
        ofstream    out(filename);
        ((nsHTMLContentSinkStream*)sink)->SetOutputStream(out);

        if (NS_OK == rv) {
          parser->SetContentSink(sink);
          
          nsIDTD* dtd = nsnull;
          rv = NS_NewXIFDTD(&dtd);
          if (NS_OK == rv) 
          {
            parser->RegisterDTD(dtd);
            dtd->SetContentSink(sink);
            dtd->SetParser(parser);
            parser->Parse(buffer, PR_FALSE);           
          }
          out.close();

          NS_IF_RELEASE(dtd);
          NS_IF_RELEASE(sink);
        }
        NS_RELEASE(parser);
      }
      NS_RELEASE(doc);
    }
    NS_RELEASE(shell);
  }
}

void 
nsBrowserWindow::DoToggleSelection()
{
  nsIPresShell* shell = GetPresShell();
  if (nsnull != shell) {
    nsIDocument* doc = shell->GetDocument();
    if (nsnull != doc) {
      PRBool  current = doc->GetDisplaySelection();
      doc->SetDisplaySelection(!current);
      ForceRefresh();
      NS_RELEASE(doc);
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

  case VIEWER_DUMP_CONTENT:
    DumpContent();
    DumpWebShells();
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
#if XXX_fix_me
    if ((nsnull != wd) && (nsnull != wd->observer)) {
      nsIPresContext *px = wd->observer->mWebWidget->GetPresContext();
      nsIPresShell   *ps = px->GetShell();
      nsIViewManager *vm = ps->GetViewManager();

      vm->ShowQuality(!vm->GetShowQuality());

      NS_RELEASE(vm);
      NS_RELEASE(ps);
      NS_RELEASE(px);
    }
#endif
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

//----------------------------------------------------------------------

// Factory code for creating nsBrowserWindow's

class nsBrowserWindowFactory : public nsIFactory
{
public:
  nsBrowserWindowFactory();
  ~nsBrowserWindowFactory();

  // nsISupports methods
  NS_IMETHOD QueryInterface(const nsIID &aIID, void **aResult);
  NS_IMETHOD_(nsrefcnt) AddRef(void);
  NS_IMETHOD_(nsrefcnt) Release(void);

  // nsIFactory methods
  NS_IMETHOD CreateInstance(nsISupports *aOuter,
                            const nsIID &aIID,
                            void **aResult);

  NS_IMETHOD LockFactory(PRBool aLock);

private:
  nsrefcnt  mRefCnt;
};

nsBrowserWindowFactory::nsBrowserWindowFactory()
{
  mRefCnt = 0;
}

nsBrowserWindowFactory::~nsBrowserWindowFactory()
{
  NS_ASSERTION(mRefCnt == 0, "non-zero refcnt at destruction");
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

nsrefcnt
nsBrowserWindowFactory::AddRef()
{
  return ++mRefCnt;
}

nsrefcnt
nsBrowserWindowFactory::Release()
{
  if (--mRefCnt == 0) {
    delete this;
    return 0; // Don't access mRefCnt after deleting!
  }
  return mRefCnt;
}

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
