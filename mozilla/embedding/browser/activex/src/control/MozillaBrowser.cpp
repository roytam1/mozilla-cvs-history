/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
 *   Pierre Phaneuf <pp@ludusdesign.com>
 */

#include "stdafx.h"
#include <string.h>
#include <string>
#include <objidl.h>
//#include <comdef.h>

#include "MozillaControl.h"
#include "MozillaBrowser.h"
#include "IEHtmlDocument.h"

#include "nsCWebBrowser.h"
#include "nsFileSpec.h"
#include "nsILocalFile.h"
#include "nsIContentViewerFile.h"

static NS_DEFINE_CID(kWindowCID, NS_WINDOW_CID);
static NS_DEFINE_IID(kIWidgetIID, NS_IWIDGET_IID);

static const TCHAR *c_szInvalidArg = _T("Invalid parameter");
static const TCHAR *c_szUninitialized = _T("Method called while control is uninitialized");

#define RETURN_ERROR(message, hr) \
	SetErrorInfo(message, hr); \
	return hr;

#define RETURN_E_INVALIDARG() \
	RETURN_ERROR(c_szInvalidArg, E_INVALIDARG);

#define RETURN_E_UNEXPECTED() \
	RETURN_ERROR(c_szUninitialized, E_UNEXPECTED);


extern "C" void NS_SetupRegistry();

static const std::string c_szPrefsHomePage = "browser.startup.homepage";
static const std::string c_szDefaultPage   = "resource:/res/MozillaControl.html";

static const tstring c_szHelpKey = _T("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Browser Helper Objects");

#define MOZ_CONTROL_REG_KEY                  _T("Software\\Mozilla\\")
#define MOZ_CONTROL_REG_VALUE_DIR            _T("Dir")
#define MOZ_CONTROL_REG_VALUE_BIN_DIRECTORY_PATH _T("BinDirectoryPath")

// Some recent SDKs define these IOleCommandTarget groups, so they're
// postfixed with _Moz to prevent linker errors.

GUID CGID_IWebBrowser_Moz =
	{ 0xED016940L, 0xBD5B, 0x11cf, {0xBA, 0x4E, 0x00, 0xC0, 0x4F, 0xD7, 0x08, 0x16} };

GUID CGID_MSHTML_Moz =
	{ 0xED016940L, 0xBD5B, 0x11cf, {0xBA, 0x4E, 0x00, 0xC0, 0x4F, 0xD7, 0x08, 0x16} };

/////////////////////////////////////////////////////////////////////////////
// CMozillaBrowser

// Initialise static member variables
BOOL CMozillaBrowser::m_bRegistryInitialized = FALSE;
#ifdef HACK_AROUND_NONREENTRANT_INITXPCOM
BOOL CMozillaBrowser::m_bXPCOMInitialised = FALSE;
#endif

//
// Constructor
//
CMozillaBrowser::CMozillaBrowser()
{
	NG_TRACE_METHOD(CMozillaBrowser::CMozillaBrowser);

	// ATL flags ensures the control opens with a window
	m_bWindowOnly = TRUE;
	m_bWndLess = FALSE;

	// Initialize layout interfaces
	m_pIDocShell = nsnull;
	m_pIWebShellWin = nsnull;
	m_pIPref = nsnull;
    m_pIServiceManager = nsnull;
	m_bValidBrowser = FALSE;

	// Ready state of control
	m_nBrowserReadyState = READYSTATE_UNINITIALIZED;
	
	// Create the container that handles some things for us
	m_pWebShellContainer = NULL;
	m_pEditor = NULL;

	// Controls starts off unbusy
	m_bBusy = FALSE;

	// Control starts off in non-edit mode
	m_bEditorMode = FALSE;

	// Control starts off without being a drop target
	m_bDropTarget = FALSE;

 	// the IHTMLDocument, lazy allocation.
 	m_pDocument = NULL;

	// Open registry keys
	m_SystemKey.Create(HKEY_LOCAL_MACHINE, MOZ_CONTROL_REG_KEY);
	m_UserKey.Create(HKEY_CURRENT_USER, MOZ_CONTROL_REG_KEY);

	// Initialise the web shell
	Initialize();
}


//
// Destructor
//
CMozillaBrowser::~CMozillaBrowser()
{
	NG_TRACE_METHOD(CMozillaBrowser::~CMozillaBrowser);
	
	// Close the web shell
	Terminate();

	// Close registry keys
	m_SystemKey.Close();
	m_UserKey.Close();
}


STDMETHODIMP CMozillaBrowser::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_IWebBrowser,
		&IID_IWebBrowser2,
		&IID_IWebBrowserApp
	};
	for (int i=0;i<(sizeof(arr)/sizeof(arr[0]));i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}


//
// Displays a message box to the user. If the container provides
// a IDocHostShowUI interface we use that to display messages, otherwise
// a simple message box is shown.
//
int CMozillaBrowser::MessageBox(LPCTSTR lpszText, LPCTSTR lpszCaption, UINT nType)
{
	// Let the doc host display it's own message box if it can
	CIPtr(IDocHostShowUI) spIDocHostShowUI = m_spClientSite;
	if (spIDocHostShowUI)
	{
		USES_CONVERSION;
		LRESULT lResult = 0;
		HRESULT hr = spIDocHostShowUI->ShowMessage(m_hWnd,
						T2OLE(lpszText), T2OLE(lpszCaption), nType, NULL, 0, &lResult);
		if (hr == S_OK)
		{
			return lResult;
		}
	}

	// Do the default message box
	return CWindow::MessageBox(lpszText, lpszCaption, nType);
}


//
// Sets error information for VB programmers who want to know why
// something failed.
//
HRESULT CMozillaBrowser::SetErrorInfo(LPCTSTR lpszDesc, HRESULT hr)
{
	USES_CONVERSION;
	return AtlSetErrorInfo(GetObjectCLSID(), T2OLE(lpszDesc), 0, NULL, GUID_NULL, hr, NULL);
}


///////////////////////////////////////////////////////////////////////////////
// Message handlers


// Handle WM_CREATE windows message
LRESULT CMozillaBrowser::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	NG_TRACE_METHOD(CMozillaBrowser::OnCreate);

	// Clip the child windows out of paint operations
	SetWindowLong(GWL_STYLE, GetWindowLong(GWL_STYLE) | WS_CLIPCHILDREN);

    // Create the NGLayout WebShell
    CreateBrowser();

	// TODO create and register a drop target

	// Control is ready
	m_nBrowserReadyState = READYSTATE_LOADED;

	// Load browser helpers
	LoadBrowserHelpers();

	// Browse to a default page - if in design mode
    BOOL bUserMode = FALSE;
    if (SUCCEEDED(GetAmbientUserMode(bUserMode)))
    {
		if (!bUserMode)
		{
			USES_CONVERSION;
			Navigate(A2OLE(c_szDefaultPage.c_str()), NULL, NULL, NULL, NULL);
		}
	}

	return 0;
}


// Handle WM_DESTROY window message
LRESULT CMozillaBrowser::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	NG_TRACE_METHOD(CMozillaBrowser::OnDestroy);

	// Unload browser helpers
	UnloadBrowserHelpers();

	// Clean up the browser
	DestroyBrowser();

	return 0;
}


// Handle WM_SIZE windows message
LRESULT CMozillaBrowser::OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	NG_TRACE_METHOD(CMozillaBrowser::OnSize);

    // Pass resize information down to the WebShell...
    if (m_pIWebShellWin)
	{
		m_pIWebShellWin->SetPosition(0, 0);
		m_pIWebShellWin->SetSize(LOWORD(lParam), HIWORD(lParam), PR_TRUE);
	}
	return 0;
}


// Handle WM_PAINT windows message (and IViewObject::Draw) 
HRESULT CMozillaBrowser::OnDraw(ATL_DRAWINFO& di)
{
	NG_TRACE_METHOD(CMozillaBrowser::OnDraw);

	if (!IsValid())
	{
		RECT& rc = *(RECT*)di.prcBounds;
		Rectangle(di.hdcDraw, rc.left, rc.top, rc.right, rc.bottom);
		DrawText(di.hdcDraw, m_sErrorMessage.c_str(), -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	}

	return S_OK;
}

// Handle ID_PAGESETUP command
LRESULT CMozillaBrowser::OnPageSetup(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	NG_TRACE_METHOD(CMozillaBrowser::OnPageSetup);
	MessageBox(_T("No page setup yet!"), _T("Control Message"), MB_OK);

	// TODO show the page setup dialog

	return 0;
}

// Handle ID_PRINT command
LRESULT CMozillaBrowser::OnPrint(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	NG_TRACE_METHOD(CMozillaBrowser::OnPrint);
	if (IsValid())
	{
		return 0;
	}

	// Print the contents
	nsIContentViewer *pContentViewer = nsnull;
	nsresult res = m_pIDocShell->GetContentViewer(&pContentViewer);
	if (NS_SUCCEEDED(res))
	{
        nsCOMPtr<nsIContentViewerFile> spContentViewerFile = do_QueryInterface(pContentViewer);
		spContentViewerFile->Print(PR_TRUE, nsnull);
		NS_RELEASE(pContentViewer);
	}
	
	return 0;
}

LRESULT CMozillaBrowser::OnSaveAs(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	NG_TRACE_METHOD(CMozillaBrowser::OnSaveAs);

	OPENFILENAME SaveFileName;

	char szFile[256];
	char szFileTitle[256];
	BSTR pageName = NULL;

	SaveFileName.lStructSize = sizeof(SaveFileName);
	SaveFileName.hwndOwner = m_hWnd;
	SaveFileName.hInstance = NULL;
	SaveFileName.lpstrFilter = "Web Page, HTML Only (*.htm;*.html)\0*.htm;*.html\0Text File (*.txt)\0*.txt\0"; 
	//TODO:	The IE control allows you to also save as "Web Page, complete" where all of the page's images are saved
	//		in a directory along with the web page.  This doesn't appear to be directly supported by Mozilla, but
	//		could be implemented here if deemed necessary.  (Web Page, complete (*.htm;*.html)\0*.htm;*.html\0)
	SaveFileName.lpstrCustomFilter = NULL; 
	SaveFileName.nMaxCustFilter = NULL; 
	SaveFileName.nFilterIndex = 1; 
	SaveFileName.lpstrFile = szFile; 
	SaveFileName.nMaxFile = sizeof(szFile); 
	SaveFileName.lpstrFileTitle = szFileTitle;
	SaveFileName.nMaxFileTitle = sizeof(szFileTitle); 
	SaveFileName.lpstrInitialDir = NULL; 
	SaveFileName.lpstrTitle = NULL; 
	SaveFileName.Flags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT; 
	SaveFileName.nFileOffset = NULL; 
	SaveFileName.nFileExtension = NULL; 
	SaveFileName.lpstrDefExt = "htm"; 
	SaveFileName.lCustData = NULL; 
	SaveFileName.lpfnHook = NULL; 
	SaveFileName.lpTemplateName = NULL; 


	//Get the title of the current web page to set as the default filename.
	USES_CONVERSION;
	char szTmp[256];
	get_LocationName(&pageName);
	strcpy(szTmp, OLE2A(pageName));
	
	int j = 0;									//The SaveAs dialog will fail if szFile contains any "bad" characters.
	for (int i=0; szTmp[i]!='\0'; i++) {		//This hunk of code attempts to mimick the IE way of replacing "bad"
		switch(szTmp[i]) {						//characters with "good" characters.
			case '\\':
			case '*':
			case '|':
			case ':':
			case '"':
			case '>':
			case '<':
			case '?':
				break;
			case '.':
				if (szTmp[i+1] != '\0') {
					szFile[j] = '_';
					j++;
				}
				break;
			case '/':
				szFile[j] = '-';
				j++;
				break;
			default:
				szFile[j] = szTmp[i];
				j++;
		}
	}
	szFile[j] = '\0';

	HRESULT res = S_OK;
	if ( GetSaveFileName(&SaveFileName) ) {		
		//Get the current DOM document
		nsIDOMDocument* pDocument = nsnull;
		res = GetDOMDocument(&pDocument);
		if ( FAILED(res) )
			return res;
		
		//Get an nsIDiskDocument interface to the DOM document
		nsCOMPtr<nsIDiskDocument> diskDoc = do_QueryInterface(pDocument);
		if (!diskDoc)
			return E_NOINTERFACE;
		
		//Set the file type specified by the user
		//Add the correct file extension if none is specified
		nsIDiskDocument::ESaveFileType saveFileType;
		if ( SaveFileName.nFilterIndex == 2 )		//SaveAs text file
			saveFileType = nsIDiskDocument::eSaveFileText;
		else										//SaveAs html file
			saveFileType = nsIDiskDocument::eSaveFileHTML;

		//Create an nsFilelSpec from the selected file path.
		nsFileSpec fileSpec(szFile, PR_FALSE);
		
		//Save the file.
		res = diskDoc->SaveFile(&fileSpec, PR_TRUE, PR_TRUE, saveFileType, "");
	}
	return res;
}

LRESULT CMozillaBrowser::OnProperties(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	NG_TRACE_METHOD(CMozillaBrowser::OnProperties);

	MessageBox(_T("No Properties Yet!"), _T("Control Message"), MB_OK);
	// TODO show the properties dialog
	return 0;
}

LRESULT CMozillaBrowser::OnCut(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	NG_TRACE_METHOD(CMozillaBrowser::OnCut);

	nsresult res;
	
	nsIPresShell* pIPresShell = nsnull;
	res = GetPresShell(&pIPresShell);		//Get the presentation shell for the document
	if (NS_FAILED(res) || pIPresShell == nsnull)
	{
		return NS_ERROR_NOT_INITIALIZED;
	}

	nsCOMPtr<nsIDOMSelection> selection;						//Get a pointer to the DOM selection
	res = pIPresShell->GetSelection(SELECTION_NORMAL, getter_AddRefs(selection));
	if ( NS_FAILED(res) ) return res;
	
	res = pIPresShell->DoCopy();									//Copy the selection to the clipboard
	if ( NS_SUCCEEDED(res) )
	{
		res = selection->DeleteFromDocument();					//Delete the selection from the document
	}
	NS_RELEASE(pIPresShell);

	return res;
}

LRESULT CMozillaBrowser::OnCopy(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	NG_TRACE_METHOD(CMozillaBrowser::OnCopy);

	nsresult res;
	
	nsIPresShell* pIPresShell = nsnull;
	res = GetPresShell(&pIPresShell);		//Get the presentation shell for the document
	if (NS_FAILED(res) || pIPresShell == nsnull)
	{
		return NS_ERROR_NOT_INITIALIZED;
	}

	res = pIPresShell->DoCopy();
	NS_RELEASE(pIPresShell);

	return res;
}

LRESULT CMozillaBrowser::OnPaste(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	NG_TRACE_METHOD(CMozillaBrowser::OnPaste);

	MessageBox(_T("No Paste Yet!"), _T("Control Message"), MB_OK);
	// TODO enable pasting
	return 0;
}

LRESULT CMozillaBrowser::OnSelectAll(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	NG_TRACE_METHOD(CMozillaBrowser::OnSelectAll);

	MessageBox(_T("No Select All Yet!"), _T("Control Message"), MB_OK);
	// TODO enable Select All
	return 0;
}

// Handle ID_VIEWSOURCE command
LRESULT CMozillaBrowser::OnViewSource(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	NG_TRACE_METHOD(CMozillaBrowser::OnViewSource);

	if (m_pWebShellContainer->m_pCurrentURI)
	{
		// No URI to view!
		NG_ASSERT(0);
		return 0;
	}

	// Get the current URI
	nsXPIDLCString aURI;
	m_pWebShellContainer->m_pCurrentURI->GetSpec(getter_Copies(aURI));
	
	nsString strUrl = nsString("view-source:") + nsString(aURI);

	CIPtr(IDispatch) spDispNew;
	VARIANT_BOOL bCancel = VARIANT_FALSE;
	Fire_NewWindow2(&spDispNew, &bCancel);

	if ((bCancel == VARIANT_FALSE) && spDispNew)
	{
		CIPtr(IWebBrowser2) spOther = spDispNew;;
		if (spOther)
		{
			// tack in the viewsource command
			BSTR bstrURL = SysAllocString(strUrl.GetUnicode());
			CComVariant vURL(bstrURL);
			VARIANT vNull;
			vNull.vt = VT_NULL;

			spOther->Navigate2(&vURL, &vNull, &vNull, &vNull, &vNull);			

			// when and if we can get the container we should
			// be able to tell the other windows container not to show toolbars, menus, etc.
			// we would also be able to show the window.  one fix would be to 
			// change the navigate method to set a flag if it's viewing source, and to 
			// have it's document complete method tell it's container to hide toolbars and menu.
			/*
			IDispatch *pOtherContainer = NULL;
			pOther->get_Container(&pOtherContainer);
			if(pOtherContainer != NULL)
			{
				DWebBrowserEvents2 *pOtherEventSink;
				if(SUCCEEDED(pOtherContainer->QueryInterface(IID_IDWebBrowserEvents2,(void**)&pOtherEventSink)))
				{
					__asm int 3

					pOtherEventSink->Release();
				}

				pOtherContainer->Release();
			}
			*/

			SysFreeString(bstrURL);
		}
	}

	bHandled = TRUE;
	return 0;
}


// Test if the browser is in a valid state
BOOL CMozillaBrowser::IsValid()
{
	NG_TRACE_METHOD(CMozillaBrowser::IsValid);
	return m_bValidBrowser;
}

/////////////////////////////////////////////////////////////////////////////

static NS_DEFINE_IID(kIWebShellIID, NS_IWEB_SHELL_IID);
static NS_DEFINE_IID(kWebShellCID, NS_WEB_SHELL_CID);
static NS_DEFINE_IID(kIPrefIID, NS_IPREF_IID);
static NS_DEFINE_CID(kPrefCID, NS_PREF_CID);

// Initialises the web shell engine
HRESULT CMozillaBrowser::Initialize()
{
	// Initialise XPCOM
	TCHAR szBinDirPath[MAX_PATH];
	DWORD dwBinDirPath = sizeof(szBinDirPath) / sizeof(szBinDirPath[0]);

#ifdef HACK_AROUND_NONREENTRANT_INITXPCOM
	// Can't call NS_InitXPCom more than once or things go boom!
	if (!m_bXPCOMInitialised)
#endif
	{
		memset(szBinDirPath, 0, sizeof(szBinDirPath));
		if (m_SystemKey.QueryValue(szBinDirPath, MOZ_CONTROL_REG_VALUE_BIN_DIRECTORY_PATH, &dwBinDirPath) == ERROR_SUCCESS)
		{
			USES_CONVERSION;
			nsILocalFile *pBinDirPath = nsnull;
			nsresult res = NS_NewLocalFile(T2A(szBinDirPath), &pBinDirPath);
			NS_InitXPCOM(&m_pIServiceManager, pBinDirPath);
			NS_RELEASE(pBinDirPath);
		}
		else
		{
			NS_InitXPCOM(&m_pIServiceManager, nsnull);
		}
#ifdef HACK_AROUND_NONREENTRANT_INITXPCOM
		m_bXPCOMInitialised = TRUE;
#endif
	}

	// Register components
	if (!m_bRegistryInitialized)
	{
		NS_SetupRegistry();
		m_bRegistryInitialized = TRUE;
	}

	// Create the Event Queue for the UI thread...
	//
	// If an event queue already exists for the thread, then 
	// CreateThreadEventQueue(...) will fail...
	nsIEventQueueService* eventQService = NULL;
	nsresult rv = nsServiceManager::GetService(kEventQueueServiceCID,
								kIEventQueueServiceIID,
								(nsISupports **)&eventQService);
	if (NS_SUCCEEDED(rv))
	{
		rv = eventQService->CreateThreadEventQueue();
		nsServiceManager::ReleaseService(kEventQueueServiceCID, eventQService);
	}

	return S_OK;
}

// Terminates the web shell engine
HRESULT CMozillaBrowser::Terminate()
{
	// Destroy the event queue
	nsIEventQueueService* eventQService = NULL;
	nsresult rv = nsServiceManager::GetService(kEventQueueServiceCID,
								kIEventQueueServiceIID,
								(nsISupports **)&eventQService);
	if (NS_SUCCEEDED(rv))
	{
		rv = eventQService->DestroyThreadEventQueue();
		nsServiceManager::ReleaseService(kEventQueueServiceCID, eventQService);
	}

	// Terminate XPCOM & cleanup
#ifndef HACK_AROUND_NONREENTRANT_INITXPCOM
	NS_ShutdownXPCOM(m_pIServiceManager);
#endif
	m_pIServiceManager = nsnull;

	return S_OK;
}

// Create and initialise the web shell
HRESULT CMozillaBrowser::CreateBrowser() 
{	
	NG_TRACE_METHOD(CMozillaBrowser::CreateBrowser);

	if (m_pIDocShell != nsnull)
	{
		NG_ASSERT(0);
		RETURN_E_UNEXPECTED();
	}

	RECT rcLocation;
	GetClientRect(&rcLocation);
	if (IsRectEmpty(&rcLocation))
	{
		rcLocation.bottom++;
		rcLocation.top++;
	}

	nsRect r;
	r.x = 0;
	r.y = 0;
	r.width  = rcLocation.right  - rcLocation.left;
	r.height = rcLocation.bottom - rcLocation.top;

	nsresult rv;

	// Load preferences service
	rv = nsServiceManager::GetService(kPrefCID, 
									NS_GET_IID(nsIPref), 
									(nsISupports **)&m_pIPref);
	if (NS_FAILED(rv))
	{
		NG_ASSERT(0);
		NG_TRACE(_T("Could not create preference object rv=%08x\n"), (int) rv);
		m_sErrorMessage = _T("Error - could not create preference object");
		return E_FAIL;
	}

	rv = m_pIPref->StartUp();		//Initialize the preference service
	rv = m_pIPref->ReadUserPrefs();	//Reads from default_prefs.js
	
	PRBool aAllowPlugins = PR_TRUE;

	// Create web shell
	m_pIWebBrowser = do_CreateInstance(NS_WEBBROWSER_PROGID, &rv);
	if (NS_FAILED(rv))
	{
		return rv;
	}

	m_pIWebBrowser->QueryInterface(NS_GET_IID(nsIBaseWindow), (void **) &m_pIWebShellWin);
	rv = m_pIWebShellWin->InitWindow(nsNativeWidget(m_hWnd), nsnull, r.x, r.y, r.width, r.height);
	m_pIWebShellWin->Create();
	m_pIWebBrowser->GetDocShell(&m_pIDocShell);
	m_pIDocShell->SetAllowPlugins(aAllowPlugins);
	nsCOMPtr<nsIDocumentLoader> docLoader;

	// Create the container object
	m_pWebShellContainer = new CWebShellContainer(this);
	m_pWebShellContainer->AddRef();

	// Set up the web shell
	m_pIWebBrowser->SetParentURIContentListener(m_pWebShellContainer);

	nsCOMPtr<nsIDocShellTreeItem> docShellAsItem(do_QueryInterface(m_pIDocShell));
	docShellAsItem->SetTreeOwner(m_pWebShellContainer);

	m_pIDocShell->SetDocLoaderObserver((nsIDocumentLoaderObserver*) m_pWebShellContainer);
	m_pIWebShellWin->SetVisibility(PR_TRUE);

	m_bValidBrowser = TRUE;

	return S_OK;
}

// Clean up the browser
HRESULT CMozillaBrowser::DestroyBrowser()
{
	// TODO unregister drop target

	m_bValidBrowser = FALSE;

 	// Destroy the htmldoc
 	if (m_pDocument != NULL)
 	{
 		m_pDocument->Release();
 		m_pDocument = NULL;
 	}

    // Destroy layout...
	if (m_pIWebShellWin != nsnull)
	{
		m_pIWebShellWin->Destroy();
		NS_RELEASE(m_pIWebShellWin);
	}

	if (m_pIDocShell != nsnull)
	{
		NS_RELEASE(m_pIDocShell);
	}

	if (m_pWebShellContainer)
	{
		m_pWebShellContainer->Release();
		m_pWebShellContainer = NULL;
	}

	if (m_pIPref)
	{
		NS_RELEASE(m_pIPref);
	}
	
	return S_OK;
}


// Turns the editor mode on or off
HRESULT CMozillaBrowser::SetEditorMode(BOOL bEnabled)
{
	NG_TRACE_METHOD(CMozillaBrowser::SetEditorMode);

	m_bEditorMode = FALSE;
	if (bEnabled && m_pEditor == nsnull)
	{
		if (!IsValid())
		{
			return E_UNEXPECTED;
		}

		nsresult result = nsComponentManager::CreateInstance(kHTMLEditorCID,
										nsnull,
										NS_GET_IID(nsIEditor),
										(void **) &m_pEditor);
		if (NS_FAILED(result))
		{
			return result;
		}
		if (!m_pEditor)
		{
			return E_OUTOFMEMORY;
		}

		nsIDOMDocument *pIDOMDocument = nsnull;
		if (FAILED(GetDOMDocument(&pIDOMDocument)) || pIDOMDocument == nsnull)
		{
			return E_UNEXPECTED;
		}

		nsIPresShell* pIPresShell = nsnull;
		if (FAILED(GetPresShell(&pIPresShell)) || pIPresShell == nsnull)
		{
			return E_UNEXPECTED;
		}

		result = m_pEditor->Init(pIDOMDocument, pIPresShell, 0);
		if (NS_SUCCEEDED(result))
		{
			m_bEditorMode = TRUE;
		}

		NS_RELEASE(pIPresShell);
		NS_RELEASE(pIDOMDocument);
	}
	else
	{
		if (m_pEditor)
		{
			m_pEditor->Release();
			m_pEditor = nsnull;
		}
	}

	return S_OK;
}


HRESULT CMozillaBrowser::OnEditorCommand(DWORD nCmdID)
{
	NG_TRACE_METHOD(CMozillaBrowser::OnEditorCommand);

	static nsIAtom * propB = NS_NewAtom("b");       
	static nsIAtom * propI = NS_NewAtom("i");     
	static nsIAtom * propU = NS_NewAtom("u");     

	if (!m_bEditorMode)
	{
		return E_UNEXPECTED;
	}
	if (!m_pEditor)
	{
		NG_ASSERT(0);
		return E_UNEXPECTED;
	}

	nsCOMPtr<nsIHTMLEditor> pHtmlEditor = do_QueryInterface(m_pEditor);

	bool bToggleInlineProperty = false;
	nsIAtom *pInlineProperty = nsnull;

	switch (nCmdID)
	{
	case IDM_BOLD:
		pInlineProperty = propB;
		bToggleInlineProperty = true;
		break;
	case IDM_ITALIC:
		pInlineProperty = propI;
		bToggleInlineProperty = true;
		break;
	case IDM_UNDERLINE:
		pInlineProperty = propU;
		bToggleInlineProperty = true;
		break;
	
	// TODO add the rest!
	
	default:
		// DO NOTHING
		break;
	}

	// Does the instruction involve toggling something? e.g. B, U, I
	if (bToggleInlineProperty)
	{
		PRBool bFirst = PR_TRUE;
		PRBool bAny = PR_TRUE;
		PRBool bAll = PR_TRUE;

		// Set or remove
		pHtmlEditor->GetInlineProperty(pInlineProperty, nsnull, nsnull, bFirst, bAny, bAll);
		if (bAny)
		{
			pHtmlEditor->RemoveInlineProperty(pInlineProperty, nsnull);
		}
		else
		{
			pHtmlEditor->SetInlineProperty(pInlineProperty, nsnull, nsnull);
		}
	}

	return S_OK;
}


// Returns the presentation shell
HRESULT CMozillaBrowser::GetPresShell(nsIPresShell **pPresShell)
{
	NG_TRACE_METHOD(CMozillaBrowser::GetPresShell);

	nsresult res;
	
	// Test for stupid args
	if (pPresShell == NULL)
	{
		NG_ASSERT(0);
		return E_INVALIDARG;
	}

	*pPresShell = nsnull;

	if (!IsValid())
	{
		NG_ASSERT(0);
		return E_UNEXPECTED;
	}
	
	nsIContentViewer* pIContentViewer = nsnull;
	res = m_pIDocShell->GetContentViewer(&pIContentViewer);
	if (NS_SUCCEEDED(res) && pIContentViewer)
	{
		nsIDocumentViewer* pIDocViewer = nsnull;
		res = pIContentViewer->QueryInterface(kIDocumentViewerIID, (void**) &pIDocViewer);
		if (NS_SUCCEEDED(res) && pIDocViewer)
		{
			nsIPresContext * pIPresContent = nsnull;
			res = pIDocViewer->GetPresContext(pIPresContent);
			if (NS_SUCCEEDED(res) && pIPresContent)
			{
				res = pIPresContent->GetShell(pPresShell);
				NS_RELEASE(pIPresContent);
			}
			NS_RELEASE(pIDocViewer);
		}
		NS_RELEASE(pIContentViewer);
	}

	return res;
}


// Return the root DOM document
HRESULT CMozillaBrowser::GetDOMDocument(nsIDOMDocument **pDocument)
{
	NG_TRACE_METHOD(CMozillaBrowser::GetDOMDocument);

	nsresult res;

	// Test for stupid args
	if (pDocument == NULL)
	{
		NG_ASSERT(0);
		return E_INVALIDARG;
	}

	*pDocument = nsnull;

	if (!IsValid())
	{
		NG_ASSERT(0);
		return E_UNEXPECTED;
	}
	
	nsIContentViewer * pCViewer = nsnull;
	res = m_pIDocShell->GetContentViewer(&pCViewer);
	if (NS_SUCCEEDED(res) && pCViewer)
	{
		nsIDocumentViewer * pDViewer = nsnull;
		res = pCViewer->QueryInterface(kIDocumentViewerIID, (void**) &pDViewer);
		if (NS_SUCCEEDED(res) && pDViewer)
		{
			nsIDocument * pDoc = nsnull;
			res = pDViewer->GetDocument(pDoc);
			if (NS_SUCCEEDED(res) && pDoc)
			{
				res = pDoc->QueryInterface(kIDOMDocumentIID, (void**) pDocument);
				NS_RELEASE(pDoc);
			}
			NS_RELEASE(pDViewer);
		}
		NS_RELEASE(pCViewer);
	}

	return res;
}


// Load any browser helpers
HRESULT CMozillaBrowser::LoadBrowserHelpers()
{
	NG_TRACE_METHOD(CMozillaBrowser::LoadBrowserHelpers);

	// IE loads browser helper objects from a branch of the registry
	// Search the branch looking for objects to load with the control.

	CRegKey cKey;
	if (cKey.Open(HKEY_LOCAL_MACHINE, c_szHelpKey.c_str(), KEY_ENUMERATE_SUB_KEYS) != ERROR_SUCCESS)
	{
		NG_TRACE(_T("No browser helper key found\n"));
		return S_FALSE;
	}

	std::vector<CLSID> cClassList;

	DWORD nKey = 0;
	LONG nResult = ERROR_SUCCESS;
	while (nResult == ERROR_SUCCESS)
	{
		TCHAR szCLSID[50];
		DWORD dwCLSID = sizeof(szCLSID) / sizeof(TCHAR);
		FILETIME cLastWrite;
		
		// Read next subkey
		nResult = RegEnumKeyEx(cKey, nKey++, szCLSID, &dwCLSID, NULL, NULL, NULL, &cLastWrite);
		if (nResult != ERROR_SUCCESS)
		{
			break;
		}

		NG_TRACE(_T("Reading helper object \"%s\"\n"), szCLSID);

		// Turn the key into a CLSID
		USES_CONVERSION;
		CLSID clsid;
		if (CLSIDFromString(T2OLE(szCLSID), &clsid) != NOERROR)
		{
			continue;
		}

		cClassList.push_back(clsid);
	}

	// Empty list?
	if (cClassList.empty())
	{
		NG_TRACE(_T("No browser helper objects found\n"));
		return S_FALSE;
	}

	// Create each object in turn
	for (std::vector<CLSID>::const_iterator i = cClassList.begin(); i != cClassList.end(); i++)
	{
		CLSID clsid = *i;
		HRESULT hr;
		CComQIPtr<IObjectWithSite, &IID_IObjectWithSite> cpObjectWithSite;

		hr = CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER, IID_IObjectWithSite, (LPVOID *) &cpObjectWithSite);
		if (FAILED(hr))
		{
			NG_TRACE(_T("Registered browser helper object cannot be created\n"));;
		}

		// Set the object to point at the browser
		cpObjectWithSite->SetSite((IWebBrowser2 *) this);

		// Store in the list
		CComUnkPtr cpUnk = cpObjectWithSite;
		m_cBrowserHelperList.push_back(cpUnk);
	}
		
	return S_OK;
}

// Release browser helpers
HRESULT CMozillaBrowser::UnloadBrowserHelpers()
{
	NG_TRACE_METHOD(CMozillaBrowser::UnloadBrowserHelpers);

	for (ObjectList::const_iterator i = m_cBrowserHelperList.begin(); i != m_cBrowserHelperList.end(); i++)
	{
		CComUnkPtr cpUnk = *i;
		CComQIPtr<IObjectWithSite, &IID_IObjectWithSite> cpObjectWithSite = cpUnk;
		if (cpObjectWithSite)
		{
			cpObjectWithSite->SetSite(NULL);
		}
	}
	m_cBrowserHelperList.clear();

	return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
// IOleObject overrides

// This is an almost verbatim copy of the standard ATL implementation of
// IOleObject::InPlaceActivate but with a few lines commented out.

HRESULT CMozillaBrowser::InPlaceActivate(LONG iVerb, const RECT* prcPosRect)
{
	NG_TRACE_METHOD(CMozillaBrowser::InPlaceActivate);

	HRESULT hr;

	if (m_spClientSite == NULL)
		return S_OK;

	CComPtr<IOleInPlaceObject> pIPO;
	ControlQueryInterface(IID_IOleInPlaceObject, (void**)&pIPO);
	_ASSERTE(pIPO != NULL);
	if (prcPosRect != NULL)
		pIPO->SetObjectRects(prcPosRect, prcPosRect);

	if (!m_bNegotiatedWnd)
	{
		if (!m_bWindowOnly)
			// Try for windowless site
			hr = m_spClientSite->QueryInterface(IID_IOleInPlaceSiteWindowless, (void **)&m_spInPlaceSite);

		if (m_spInPlaceSite)
		{
			m_bInPlaceSiteEx = TRUE;
			m_bWndLess = SUCCEEDED(m_spInPlaceSite->CanWindowlessActivate());
			m_bWasOnceWindowless = TRUE;
		}
		else
		{
			m_spClientSite->QueryInterface(IID_IOleInPlaceSiteEx, (void **)&m_spInPlaceSite);
			if (m_spInPlaceSite)
				m_bInPlaceSiteEx = TRUE;
			else
				hr = m_spClientSite->QueryInterface(IID_IOleInPlaceSite, (void **)&m_spInPlaceSite);
		}
	}

	_ASSERTE(m_spInPlaceSite);
	if (!m_spInPlaceSite)
		return E_FAIL;

	m_bNegotiatedWnd = TRUE;

	if (!m_bInPlaceActive)
	{

		BOOL bNoRedraw = FALSE;
		if (m_bWndLess)
			m_spInPlaceSite->OnInPlaceActivateEx(&bNoRedraw, ACTIVATE_WINDOWLESS);
		else
		{
			if (m_bInPlaceSiteEx)
				m_spInPlaceSite->OnInPlaceActivateEx(&bNoRedraw, 0);
			else
			{
				HRESULT hr = m_spInPlaceSite->CanInPlaceActivate();
				if (FAILED(hr))
					return hr;
				m_spInPlaceSite->OnInPlaceActivate();
			}
		}
	}

	m_bInPlaceActive = TRUE;

	// get location in the parent window,
	// as well as some information about the parent
	//
	OLEINPLACEFRAMEINFO frameInfo;
	RECT rcPos, rcClip;
	CComPtr<IOleInPlaceFrame> spInPlaceFrame;
	CComPtr<IOleInPlaceUIWindow> spInPlaceUIWindow;
	frameInfo.cb = sizeof(OLEINPLACEFRAMEINFO);
	HWND hwndParent;
	if (m_spInPlaceSite->GetWindow(&hwndParent) == S_OK)
	{
		m_spInPlaceSite->GetWindowContext(&spInPlaceFrame,
			&spInPlaceUIWindow, &rcPos, &rcClip, &frameInfo);

		if (!m_bWndLess)
		{
			if (m_hWndCD)
			{
				::ShowWindow(m_hWndCD, SW_SHOW);
				::SetFocus(m_hWndCD);
			}
			else
			{
				HWND h = CreateControlWindow(hwndParent, rcPos);
				_ASSERTE(h == m_hWndCD);
			}
		}

		pIPO->SetObjectRects(&rcPos, &rcClip);
	}

	CComPtr<IOleInPlaceActiveObject> spActiveObject;
	ControlQueryInterface(IID_IOleInPlaceActiveObject, (void**)&spActiveObject);

	// Gone active by now, take care of UIACTIVATE
	if (DoesVerbUIActivate(iVerb))
	{
		if (!m_bUIActive)
		{
			m_bUIActive = TRUE;
			hr = m_spInPlaceSite->OnUIActivate();
			if (FAILED(hr))
				return hr;

			SetControlFocus(TRUE);
			// set ourselves up in the host.
			//
			if (spActiveObject)
			{
				if (spInPlaceFrame)
					spInPlaceFrame->SetActiveObject(spActiveObject, NULL);
				if (spInPlaceUIWindow)
					spInPlaceUIWindow->SetActiveObject(spActiveObject, NULL);
			}

// These lines are deliberately commented out to demonstrate what breaks certain
// control containers.

//			if (spInPlaceFrame)
//				spInPlaceFrame->SetBorderSpace(NULL);
//			if (spInPlaceUIWindow)
//				spInPlaceUIWindow->SetBorderSpace(NULL);
		}
	}

	m_spClientSite->ShowObject();

	return S_OK;
}


HRESULT STDMETHODCALLTYPE CMozillaBrowser::GetClientSite(IOleClientSite **ppClientSite)
{
	NG_TRACE_METHOD(CMozillaBrowser::GetClientSite);

	NG_ASSERT(ppClientSite);

	// This fixes a problem in the base class which asserts if the client
	// site has not been set when this method is called. 

	HRESULT hRes = E_POINTER;
	if (ppClientSite != NULL)
	{
		*ppClientSite = NULL;
		if (m_spClientSite == NULL)
		{
			return S_OK;
		}
	}

	return IOleObjectImpl<CMozillaBrowser>::GetClientSite(ppClientSite);
}


/////////////////////////////////////////////////////////////////////////////
// IWebBrowser

HRESULT STDMETHODCALLTYPE CMozillaBrowser::GoBack(void)
{
	NG_TRACE_METHOD(CMozillaBrowser::GoBack);

    if (!IsValid())
	{
		NG_ASSERT(0);
		RETURN_E_UNEXPECTED();
	}
	nsCOMPtr<nsIWebNavigation> webNav(do_QueryInterface(m_pIWebBrowser));
	PRBool aCanGoBack = PR_FALSE;
	webNav->GetCanGoBack(&aCanGoBack);
	if (aCanGoBack == PR_TRUE)
	{
		webNav->GoBack();
	}

	return S_OK;
}


HRESULT STDMETHODCALLTYPE CMozillaBrowser::GoForward(void)
{
	NG_TRACE_METHOD(CMozillaBrowser::GoForward);

    if (!IsValid())
	{
		NG_ASSERT(0);
		RETURN_E_UNEXPECTED();
	}

	nsCOMPtr<nsIWebNavigation> webNav(do_QueryInterface(m_pIWebBrowser));
	PRBool aCanGoForward = PR_FALSE;
	webNav->GetCanGoForward(&aCanGoForward);
	if (aCanGoForward == PR_TRUE)
	{
		webNav->GoForward();
	}

	return S_OK;
}


HRESULT STDMETHODCALLTYPE CMozillaBrowser::GoHome(void)
{
	NG_TRACE_METHOD(CMozillaBrowser::GoHome);

    if (!IsValid())
	{
		NG_ASSERT(0);
		RETURN_E_UNEXPECTED();
	}

	USES_CONVERSION;

	TCHAR * sUrl = _T("http://home.netscape.com");

	// Find the home page stored in prefs
	if (m_pIPref)
	{
		char *szBuffer;
		nsresult rv;
		rv = m_pIPref->CopyCharPref(c_szPrefsHomePage.c_str(), &szBuffer);
		if (rv == NS_OK)
		{
			sUrl = A2T(szBuffer);
		}
	}
	// Navigate to the home page
	Navigate(T2OLE(sUrl), NULL, NULL, NULL, NULL);
	
	return S_OK;
}


HRESULT STDMETHODCALLTYPE CMozillaBrowser::GoSearch(void)
{
	NG_TRACE_METHOD(CMozillaBrowser::GoSearch);

    if (!IsValid())
	{
		NG_ASSERT(0);
		RETURN_E_UNEXPECTED();
	}

	USES_CONVERSION;

	TCHAR * sUrl = _T("http://search.netscape.com");
	// Find the home page stored in prefs
	if (m_pIPref)
	{
		// TODO find and navigate to the search page stored in prefs
		//      and not this hard coded address
	}
	Navigate(T2OLE(sUrl), NULL, NULL, NULL, NULL);
	
	return S_OK;
}


HRESULT STDMETHODCALLTYPE CMozillaBrowser::Navigate(BSTR URL, VARIANT __RPC_FAR *Flags, VARIANT __RPC_FAR *TargetFrameName, VARIANT __RPC_FAR *PostData, VARIANT __RPC_FAR *Headers)
{
	NG_TRACE_METHOD(CMozillaBrowser::Navigate);

	//Make sure browser is valid
    if (!IsValid())
	{
		NG_ASSERT(0);
		RETURN_E_UNEXPECTED();
	}

	// Extract the URL parameter
 	nsString sCommand("view");
	nsString sUrl;
	if (URL == NULL)
	{
		NG_ASSERT(0);
		RETURN_E_INVALIDARG();
	}
	else
	{
		USES_CONVERSION;
		sUrl = OLE2A(URL);
	}

	// Check for a view-source op - this is a bit kludgy
	// TODO
	if (sUrl.Compare(L"view-source:", PR_TRUE, 12) == 0)
 	{
 		sUrl.Left(sCommand, 11);
 		sUrl.Cut(0,12);
 	}

	// Extract the launch flags parameter
	LONG lFlags = 0;
	if (Flags &&
		Flags->vt != VT_ERROR &&
		Flags->vt != VT_EMPTY &&
		Flags->vt != VT_NULL)
	{
		CComVariant vFlags;
		if ( vFlags.ChangeType(VT_I4, Flags) != S_OK )
		{
			NG_ASSERT(0);
			RETURN_E_INVALIDARG();
		}
		lFlags = vFlags.lVal;
	}


	// Extract the target frame parameter
	nsString sTargetFrame;
	if (TargetFrameName &&
		TargetFrameName->vt == VT_BSTR)
	{
		USES_CONVERSION;
		sTargetFrame = nsString(OLE2A(TargetFrameName->bstrVal));
	}

	// Extract the post data parameter
	m_vLastPostData.Clear();
	if (PostData)
	{
		m_vLastPostData.Copy(PostData);
	}

	nsIPostData *pIPostData = nsnull;
	if (PostData && PostData->vt == VT_BSTR)
	{
		USES_CONVERSION;
		char *szPostData = OLE2A(PostData->bstrVal);
#if 0
		// Create post data from string
		NS_NewPostData(PR_FALSE, szPostData, &pIPostData);
#endif
	}

	// TODO Extract the headers parameter
	PRBool bModifyHistory = PR_TRUE;

	// Check the navigation flags
	if (lFlags & navOpenInNewWindow)
	{
		CIPtr(IDispatch) spDispNew;
		VARIANT_BOOL bCancel = VARIANT_FALSE;
		
		// Test if the event sink can give us a new window to navigate into
		Fire_NewWindow2(&spDispNew, &bCancel);

		lFlags &= ~(navOpenInNewWindow);
		if ((bCancel == VARIANT_FALSE) && spDispNew)
		{
			CIPtr(IWebBrowser2) spOther = spDispNew;
			if (spOther)
			{
				CComVariant vURL(URL);
				CComVariant vFlags(lFlags);
				return spOther->Navigate2(&vURL, &vFlags, TargetFrameName, PostData, Headers);
			}
		}

		// Can't open a new window without client suppot
		return E_NOTIMPL;
	}
	if (lFlags & navNoHistory)
	{
		// Disable history
		bModifyHistory = PR_FALSE;
	}
	if (lFlags & navNoReadFromCache)
	{
		// TODO disable read from cache
	}
	if (lFlags & navNoWriteToCache)
	{
		// TODO disable write to cache
	}

	// TODO find the correct target frame

	// Load the URL	
	nsresult res = NS_ERROR_FAILURE;

	nsCOMPtr<nsIWebNavigation> spIWebNavigation = do_QueryInterface(m_pIDocShell);
	if (spIWebNavigation)
	{
		res = spIWebNavigation->LoadURI(sUrl.GetUnicode());
	}

	return res;
}


HRESULT STDMETHODCALLTYPE CMozillaBrowser::Refresh(void)
{
	NG_TRACE_METHOD(CMozillaBrowser::Refresh);

//Uneccessary since this gets called again in Refresh2
#if 0
	if (!IsValid())
	{
		NG_ASSERT(0);
		RETURN_E_UNEXPECTED();
	}
#endif

	// Reload the page
	CComVariant vRefreshType(REFRESH_NORMAL);
	return Refresh2(&vRefreshType);
}


HRESULT STDMETHODCALLTYPE CMozillaBrowser::Refresh2(VARIANT __RPC_FAR *Level)
{
	NG_TRACE_METHOD(CMozillaBrowser::Refresh2);

    if (!IsValid())
	{
		NG_ASSERT(0);
		RETURN_E_UNEXPECTED();
	}

	NG_ASSERT_NULL_OR_POINTER(Level, VARIANT);

	// Check the requested refresh type
	OLECMDID_REFRESHFLAG iRefreshLevel = OLECMDIDF_REFRESH_NORMAL;
	if (Level)
	{
		CComVariant vLevelAsInt;
		if ( vLevelAsInt.ChangeType(VT_I4, Level) != S_OK )
		{
			NG_ASSERT(0);
			RETURN_E_UNEXPECTED();
		}
		iRefreshLevel = (OLECMDID_REFRESHFLAG) vLevelAsInt.iVal;
	}

	// Turn the IE refresh type into the nearest NG equivalent
	nsLoadFlags type = nsIChannel::LOAD_NORMAL;
	switch (iRefreshLevel & OLECMDIDF_REFRESH_LEVELMASK)
	{
	case OLECMDIDF_REFRESH_NORMAL:
	case OLECMDIDF_REFRESH_IFEXPIRED:
	case OLECMDIDF_REFRESH_CONTINUE:
	case OLECMDIDF_REFRESH_NO_CACHE:
	case OLECMDIDF_REFRESH_RELOAD:
	    type = nsIChannel::LOAD_NORMAL;
		break;
	case OLECMDIDF_REFRESH_COMPLETELY:
        type = nsIHTTPChannel::BYPASS_CACHE | nsIHTTPChannel::BYPASS_PROXY;
		break;
	default:
		// No idea what refresh type this is supposed to be
		NG_ASSERT(0);
		RETURN_E_UNEXPECTED();
	}

	nsCOMPtr<nsIWebNavigation> spIWebNavigation = do_QueryInterface(m_pIDocShell);
	if (spIWebNavigation)
	{
		spIWebNavigation->Reload(type);
	}
	
	return S_OK;
}


HRESULT STDMETHODCALLTYPE CMozillaBrowser::Stop()
{
	NG_TRACE_METHOD(CMozillaBrowser::Stop);

    if (!IsValid())
	{
		NG_ASSERT(0);
		RETURN_E_UNEXPECTED();
	}

	nsCOMPtr<nsIWebNavigation> spIWebNavigation = do_QueryInterface(m_pIDocShell);
	if (spIWebNavigation)
	{
		spIWebNavigation->Stop();
	}
	
	return S_OK;
}


HRESULT STDMETHODCALLTYPE CMozillaBrowser::get_Application(IDispatch __RPC_FAR *__RPC_FAR *ppDisp)
{
	NG_TRACE_METHOD(CMozillaBrowser::get_Application);

    if (!IsValid())
	{
		NG_ASSERT(0);
		RETURN_E_UNEXPECTED();
	}

	if (!NgIsValidAddress(ppDisp, sizeof(IDispatch *)))
	{
		NG_ASSERT(0);
		RETURN_E_INVALIDARG();
	}

	// Return a pointer to this controls dispatch interface
	*ppDisp = (IDispatch *) this;
	(*ppDisp)->AddRef();
	return S_OK;
}


HRESULT STDMETHODCALLTYPE CMozillaBrowser::get_Parent(IDispatch __RPC_FAR *__RPC_FAR *ppDisp)
{
	NG_TRACE_METHOD(CMozillaBrowser::get_Parent);

    if (!IsValid())
	{
		NG_ASSERT(0);
		RETURN_E_UNEXPECTED();
	}

	if (!NgIsValidAddress(ppDisp, sizeof(IDispatch *)))
	{
		NG_ASSERT(0);
		RETURN_E_INVALIDARG();
	}

	// Attempt to get the parent object of this control
	HRESULT hr = E_FAIL;
	if (m_spClientSite)
	{
		hr = m_spClientSite->QueryInterface(IID_IDispatch, (void **) ppDisp);
	}

	return (SUCCEEDED(hr)) ? S_OK : E_NOINTERFACE;
}


HRESULT STDMETHODCALLTYPE CMozillaBrowser::get_Container(IDispatch __RPC_FAR *__RPC_FAR *ppDisp)
{
	NG_TRACE_METHOD(CMozillaBrowser::get_Container);

    if (!IsValid())
	{
		NG_ASSERT(0);
		RETURN_E_UNEXPECTED();
	}

	if (!NgIsValidAddress(ppDisp, sizeof(IDispatch *)))
	{
		NG_ASSERT(0);
		RETURN_E_INVALIDARG();
	}

	//TODO: Implement get_Container: Retrieve a pointer to the IDispatch interface of the container.
	*ppDisp = NULL;
	RETURN_E_UNEXPECTED();
}


HRESULT STDMETHODCALLTYPE CMozillaBrowser::get_Document(IDispatch __RPC_FAR *__RPC_FAR *ppDisp)
{
	NG_TRACE_METHOD(CMozillaBrowser::get_Document);

    if (!IsValid())
	{
		NG_ASSERT(0);
		RETURN_E_UNEXPECTED();
	}

	if (!NgIsValidAddress(ppDisp, sizeof(IDispatch *)))
	{
		NG_ASSERT(0);
		RETURN_E_UNEXPECTED();
	}

	*ppDisp = NULL;

	// Get hold of the DOM document
	nsIDOMDocument *pIDOMDocument = nsnull;
	if (FAILED(GetDOMDocument(&pIDOMDocument)) || pIDOMDocument == nsnull)
	{
		RETURN_E_UNEXPECTED();
	}

	if (m_pDocument == NULL)
 	{	
 		CIEHtmlDocumentInstance::CreateInstance(&m_pDocument);
  		if (m_pDocument == NULL)
 		{
			RETURN_ERROR(_T("Refresh2: called while browser is invalid"), E_OUTOFMEMORY);
 		}
 		
		// addref it so it doesn't go away on us.
 		m_pDocument->AddRef();
 
 		// give it a pointer to us.  note that we shouldn't be addref'd by this call, or it would be 
 		// a circular reference.
 		m_pDocument->SetParent(this);
	}

	m_pDocument->QueryInterface(IID_IDispatch, (void **) ppDisp);
	m_pDocument->SetDOMNode(pIDOMDocument);
	pIDOMDocument->Release();

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CMozillaBrowser::get_TopLevelContainer(VARIANT_BOOL __RPC_FAR *pBool)
{
	NG_TRACE_METHOD(CMozillaBrowser::get_TopLevelContainer);

    if (!IsValid())
	{
		NG_ASSERT(0);
		RETURN_E_UNEXPECTED();
	}

	if (!NgIsValidAddress(pBool, sizeof(VARIANT_BOOL)))
	{
		NG_ASSERT(0);
		RETURN_E_INVALIDARG();
	}

	//TODO:  Implement get_TopLevelContainer
	*pBool = VARIANT_FALSE;
	return S_OK;
}


HRESULT STDMETHODCALLTYPE CMozillaBrowser::get_Type(BSTR __RPC_FAR *Type)
{
	NG_TRACE_METHOD(CMozillaBrowser::get_Type);

    if (!IsValid())
	{
		NG_ASSERT(0);
		RETURN_E_UNEXPECTED();
	}

	//NOTE:	This code should work in theory, but can't be verified because GetDoctype
	//		has not been implemented yet.
#if 0
	nsIDOMDocument *pIDOMDocument = nsnull;
	if ( SUCCEEDED(GetDOMDocument(&pIDOMDocument)) )
	{
		nsIDOMDocumentType *pIDOMDocumentType = nsnull;
		if ( SUCCEEDED(pIDOMDocument->GetDoctype(&pIDOMDocumentType)) )
		{
			nsString docName;
			pIDOMDocumentType->GetName(docName);
			//NG_TRACE("pIDOMDocumentType returns: %s", docName);
			//Still need to manipulate docName so that it goes into *Type param of this function.
		}
	}
#endif

	//TODO: Implement get_Type
	RETURN_ERROR(_T("get_Type: failed"), E_FAIL);
}


HRESULT STDMETHODCALLTYPE CMozillaBrowser::get_Left(long __RPC_FAR *pl)
{
	NG_TRACE_METHOD(CMozillaBrowser::get_Left);

    if (!IsValid())
	{
		NG_ASSERT(0);
		RETURN_E_UNEXPECTED();
	}

	if (pl == NULL)
	{
		NG_ASSERT(0);
		RETURN_E_INVALIDARG();
	}

	//TODO: Implement get_Left - Should return the left position of this control.
	*pl = 0;
	return S_OK;
}


HRESULT STDMETHODCALLTYPE CMozillaBrowser::put_Left(long Left)
{
	NG_TRACE_METHOD(CMozillaBrowser::put_Left);

    if (!IsValid())
	{
		NG_ASSERT(0);
		RETURN_E_UNEXPECTED();
	}
	
	//TODO: Implement put_Left - Should set the left position of this control.
	return S_OK;
}


HRESULT STDMETHODCALLTYPE CMozillaBrowser::get_Top(long __RPC_FAR *pl)
{
	NG_TRACE_METHOD(CMozillaBrowser::get_Top);

    if (!IsValid())
	{
		NG_ASSERT(0);
		RETURN_E_UNEXPECTED();
	}

	if (pl == NULL)
	{
		NG_ASSERT(0);
		RETURN_E_INVALIDARG();
	}

	//TODO: Implement get_Top - Should return the top position of this control.
	*pl = 0;
	return S_OK;
}


HRESULT STDMETHODCALLTYPE CMozillaBrowser::put_Top(long Top)
{
	NG_TRACE_METHOD(CMozillaBrowser::put_Top);

    if (!IsValid())
	{
		NG_ASSERT(0);
		RETURN_E_UNEXPECTED();
	}

	//TODO: Implement set_Top - Should set the top position of this control.
	return S_OK;
}


HRESULT STDMETHODCALLTYPE CMozillaBrowser::get_Width(long __RPC_FAR *pl)
{
	NG_TRACE_METHOD(CMozillaBrowser::get_Width);

    if (!IsValid())
	{
		NG_ASSERT(0);
		RETURN_E_UNEXPECTED();
	}

	if (pl == NULL)
	{
		NG_ASSERT(0);
		RETURN_E_INVALIDARG();
	}

	//TODO: Implement get_Width- Should return the width of this control.
	*pl = 0;
	return S_OK;
}


HRESULT STDMETHODCALLTYPE CMozillaBrowser::put_Width(long Width)
{
	NG_TRACE_METHOD(CMozillaBrowser::put_Width);

    if (!IsValid())
	{
		NG_ASSERT(0);
		RETURN_E_UNEXPECTED();
	}

	//TODO: Implement put_Width - Should set the width of this control.
	return S_OK;
}


HRESULT STDMETHODCALLTYPE CMozillaBrowser::get_Height(long __RPC_FAR *pl)
{
	NG_TRACE_METHOD(CMozillaBrowser::get_Height);

    if (!IsValid())
	{
		RETURN_E_UNEXPECTED();
	}

	if (pl == NULL)
	{
		RETURN_E_INVALIDARG();
	}

	//TODO: Implement get_Height - Should return the hieght of this control.
	*pl = 0;
	return S_OK;
}


HRESULT STDMETHODCALLTYPE CMozillaBrowser::put_Height(long Height)
{
	NG_TRACE_METHOD(CMozillaBrowser::put_Height);

    if (!IsValid())
	{
		NG_ASSERT(0);
		RETURN_E_UNEXPECTED();
	}

	//TODO: Implement put_Height - Should set the height of this control.
	return S_OK;
}


HRESULT STDMETHODCALLTYPE CMozillaBrowser::get_LocationName(BSTR __RPC_FAR *LocationName)
{
	NG_TRACE_METHOD(CMozillaBrowser::get_LocationName);

    if (!IsValid())
	{
		NG_ASSERT(0);
		RETURN_E_UNEXPECTED();
	}

	if (LocationName == NULL)
	{
		RETURN_E_INVALIDARG();
	}

	// Get the url from the web shell
	nsXPIDLString szLocationName;
	m_pIWebShellWin->GetTitle(getter_Copies(szLocationName));
	if (nsnull == (const PRUnichar *) szLocationName)
	{
		RETURN_E_UNEXPECTED();
	}

	// Convert the string to a BSTR
	USES_CONVERSION;
	LPCOLESTR pszConvertedLocationName = W2COLE((const PRUnichar *) szLocationName);
	*LocationName = SysAllocString(pszConvertedLocationName);

	return S_OK;
}


HRESULT STDMETHODCALLTYPE CMozillaBrowser::get_LocationURL(BSTR __RPC_FAR *LocationURL)
{
	NG_TRACE_METHOD(CMozillaBrowser::get_LocationURL);

    if (!IsValid())
	{
		NG_ASSERT(0);
		RETURN_E_UNEXPECTED();
	}

	if (LocationURL == NULL)
	{
		NG_ASSERT(0);
		RETURN_E_INVALIDARG();
	}

	// Get the url from the web shell
	if (m_pWebShellContainer->m_pCurrentURI != nsnull)
	{
		USES_CONVERSION;
		nsXPIDLCString aURI;
		m_pWebShellContainer->m_pCurrentURI->GetSpec(getter_Copies(aURI));
		*LocationURL = SysAllocString(A2OLE((const char *) aURI));
	}
	else
	{
		*LocationURL = NULL;
	}

	return S_OK;
}


HRESULT STDMETHODCALLTYPE CMozillaBrowser::get_Busy(VARIANT_BOOL __RPC_FAR *pBool)
{
	NG_TRACE_METHOD(CMozillaBrowser::get_Busy);

    if (!IsValid())
	{
		NG_ASSERT(0);
		RETURN_E_UNEXPECTED();
	}

	if (!NgIsValidAddress(pBool, sizeof(*pBool)))
	{
		NG_ASSERT(0);
		RETURN_E_INVALIDARG();
	}

	*pBool = (m_bBusy) ? VARIANT_TRUE : VARIANT_FALSE;

	return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
// IWebBrowserApp

HRESULT STDMETHODCALLTYPE CMozillaBrowser::Quit(void)
{
	NG_TRACE_METHOD(CMozillaBrowser::Quit);

    if (!IsValid())
	{
		NG_ASSERT(0);
		RETURN_E_UNEXPECTED();
	}

	//This generates an exception in the IE control.
	// TODO fire quit event
	return S_OK;
}


HRESULT STDMETHODCALLTYPE CMozillaBrowser::ClientToWindow(int __RPC_FAR *pcx, int __RPC_FAR *pcy)
{
	NG_TRACE_METHOD(CMozillaBrowser::ClientToWindow);

    if (!IsValid())
	{
		NG_ASSERT(0);
		RETURN_E_UNEXPECTED();
	}

	//This generates an exception in the IE control.
	// TODO convert points to be relative to browser
	return S_OK;
}


HRESULT STDMETHODCALLTYPE CMozillaBrowser::PutProperty(BSTR szProperty, VARIANT vtValue)
{
	NG_TRACE_METHOD(CMozillaBrowser::PutProperty);

    if (!IsValid())
	{
		NG_ASSERT(0);
		RETURN_E_UNEXPECTED();
	}

	if (szProperty == NULL)
	{
		RETURN_E_INVALIDARG();
	}
	PropertyList::iterator i;
	for (i = m_PropertyList.begin(); i != m_PropertyList.end(); i++)
	{
		// Is the property already in the list?
		if (wcscmp((*i).szName, szProperty) == 0)
		{
			// Copy the new value
			(*i).vValue = CComVariant(vtValue);
			return S_OK;
		}
	}

	Property p;
	p.szName = CComBSTR(szProperty);
	p.vValue = vtValue;

	m_PropertyList.push_back(p);
	return S_OK;
}


HRESULT STDMETHODCALLTYPE CMozillaBrowser::GetProperty(BSTR Property, VARIANT __RPC_FAR *pvtValue)
{
	NG_TRACE_METHOD(CMozillaBrowser::GetProperty);

    if (!IsValid())
	{
		NG_ASSERT(0);
		RETURN_E_UNEXPECTED();
	}

	NG_ASSERT(Property);
	NG_ASSERT_POINTER(pvtValue, VARIANT);
	
	if (Property == NULL || pvtValue == NULL)
	{
		RETURN_E_INVALIDARG();
	}
	
	VariantInit(pvtValue);
	PropertyList::iterator i;
	for (i = m_PropertyList.begin(); i != m_PropertyList.end(); i++)
	{
		// Is the property already in the list?
		if (wcscmp((*i).szName, Property) == 0)
		{
			// Copy the new value
			VariantCopy(pvtValue, &(*i).vValue);
			return S_OK;
		}
	}
	return S_OK;
}


HRESULT STDMETHODCALLTYPE CMozillaBrowser::get_Name(BSTR __RPC_FAR *Name)
{
	NG_TRACE_METHOD(CMozillaBrowser::get_Name);

    if (!IsValid())
	{
		NG_ASSERT(0);
		RETURN_E_UNEXPECTED();
	}

	NG_ASSERT_POINTER(Name, BSTR);
	if (Name == NULL)
	{
		RETURN_E_INVALIDARG();
	}

	// TODO: Implement get_Name (get Mozilla's executable name)
	*Name = SysAllocString(L"Mozilla Web Browser Control");
	return S_OK;
}


HRESULT STDMETHODCALLTYPE CMozillaBrowser::get_HWND(long __RPC_FAR *pHWND)
{
	NG_TRACE_METHOD(CMozillaBrowser::get_HWND);

    if (!IsValid())
	{
		NG_ASSERT(0);
		RETURN_E_UNEXPECTED();
	}

	NG_ASSERT_POINTER(pHWND, HWND);
	if (pHWND == NULL)
	{
		RETURN_E_INVALIDARG();
	}

	//This is supposed to return a handle to the IE main window.  Since that doesn't exist
	//in the control's case, this shouldn't do anything.
	*pHWND = NULL;
	return S_OK;
}


HRESULT STDMETHODCALLTYPE CMozillaBrowser::get_FullName(BSTR __RPC_FAR *FullName)
{
	NG_TRACE_METHOD(CMozillaBrowser::get_FullName);

    if (!IsValid())
	{
		NG_ASSERT(0);
		RETURN_E_UNEXPECTED();
	}

	NG_ASSERT_POINTER(FullName, BSTR);
	if (FullName == NULL)
	{
		RETURN_E_INVALIDARG();
	}

	// TODO: Implement get_FullName (Return the full path of the executable containing this control)
	*FullName = SysAllocString(L""); // TODO get Mozilla's executable name
	return S_OK;
}


HRESULT STDMETHODCALLTYPE CMozillaBrowser::get_Path(BSTR __RPC_FAR *Path)
{
	NG_TRACE_METHOD(CMozillaBrowser::get_Path);

    if (!IsValid())
	{
		NG_ASSERT(0);
		RETURN_E_UNEXPECTED();
	}

	NG_ASSERT_POINTER(Path, BSTR);
	if (Path == NULL)
	{
		RETURN_E_INVALIDARG();
	}

	// TODO: Implement get_Path (get Mozilla's path)
	*Path = SysAllocString(L"");
	return S_OK;
}


HRESULT STDMETHODCALLTYPE CMozillaBrowser::get_Visible(VARIANT_BOOL __RPC_FAR *pBool)
{
	NG_TRACE_METHOD(CMozillaBrowser::get_Visible);

    if (!IsValid())
	{
		NG_ASSERT(0);
		RETURN_E_UNEXPECTED();
	}

	NG_ASSERT_POINTER(pBool, int);
	if (pBool == NULL)
	{
		RETURN_E_INVALIDARG();
	}

	//TODO: Implement get_Visible
	*pBool = VARIANT_FALSE;
	return S_OK;
}


HRESULT STDMETHODCALLTYPE CMozillaBrowser::put_Visible(VARIANT_BOOL Value)
{
	NG_TRACE_METHOD(CMozillaBrowser::put_Visible);

    if (!IsValid())
	{
		NG_ASSERT(0);
		RETURN_E_UNEXPECTED();
	}

	//TODO: Implement put_Visible
	return S_OK;
}


HRESULT STDMETHODCALLTYPE CMozillaBrowser::get_StatusBar(VARIANT_BOOL __RPC_FAR *pBool)
{
	NG_TRACE_METHOD(CMozillaBrowser::get_StatusBar);

    if (!IsValid())
	{
		NG_ASSERT(0);
		RETURN_E_UNEXPECTED();
	}

	NG_ASSERT_POINTER(pBool, int);
	if (pBool == NULL)
	{
		RETURN_E_INVALIDARG();
	}

	//There is no StatusBar in this control.
	*pBool = VARIANT_FALSE;
	return S_OK;
}


HRESULT STDMETHODCALLTYPE CMozillaBrowser::put_StatusBar(VARIANT_BOOL Value)
{
	NG_TRACE_METHOD(CMozillaBrowser::put_StatusBar);

    if (!IsValid())
	{
		NG_ASSERT(0);
		RETURN_E_UNEXPECTED();
	}

	//There is no StatusBar in this control.
	return S_OK;
}


HRESULT STDMETHODCALLTYPE CMozillaBrowser::get_StatusText(BSTR __RPC_FAR *StatusText)
{
	NG_TRACE_METHOD(CMozillaBrowser::get_StatusText);

    if (!IsValid())
	{
		NG_ASSERT(0);
		RETURN_E_UNEXPECTED();
	}

	NG_ASSERT_POINTER(StatusText, BSTR);
	if (StatusText == NULL)
	{
		RETURN_E_INVALIDARG();
	}

	//TODO: Implement get_StatusText
	//NOTE: This function is related to the MS status bar which doesn't exist in this control.  Needs more
	//		investigation, but probably doesn't apply.
	*StatusText = SysAllocString(L"");
	return S_OK;
}


HRESULT STDMETHODCALLTYPE CMozillaBrowser::put_StatusText(BSTR StatusText)
{
	NG_TRACE_METHOD(CMozillaBrowser::put_StatusText);

    if (!IsValid())
	{
		NG_ASSERT(0);
		RETURN_E_UNEXPECTED();
	}

	//TODO: Implement put_StatusText
	//NOTE: This function is related to the MS status bar which doesn't exist in this control.  Needs more
	//		investigation, but probably doesn't apply.
	return S_OK;
}


HRESULT STDMETHODCALLTYPE CMozillaBrowser::get_ToolBar(int __RPC_FAR *Value)
{
	NG_TRACE_METHOD(CMozillaBrowser::get_ToolBar);

    if (!IsValid())
	{
		NG_ASSERT(0);
		RETURN_E_UNEXPECTED();
	}

	NG_ASSERT_POINTER(Value, int);
	if (Value == NULL)
	{
		RETURN_E_INVALIDARG();
	}

	//There is no ToolBar in this control.
	*Value = FALSE;
	return S_OK;
}


HRESULT STDMETHODCALLTYPE CMozillaBrowser::put_ToolBar(int Value)
{
	NG_TRACE_METHOD(CMozillaBrowser::put_ToolBar);

    if (!IsValid())
	{
		NG_ASSERT(0);
		RETURN_E_UNEXPECTED();
	}

	//There is no ToolBar in this control.
	return S_OK;
}


HRESULT STDMETHODCALLTYPE CMozillaBrowser::get_MenuBar(VARIANT_BOOL __RPC_FAR *Value)
{
	NG_TRACE_METHOD(CMozillaBrowser::get_MenuBar);

    if (!IsValid())
	{
		NG_ASSERT(0);
		RETURN_E_UNEXPECTED();
	}

	NG_ASSERT_POINTER(Value, int);
	if (Value == NULL)
	{
		RETURN_E_INVALIDARG();
	}

	//There is no MenuBar in this control.
	*Value = VARIANT_FALSE;
	return S_OK;
}


HRESULT STDMETHODCALLTYPE CMozillaBrowser::put_MenuBar(VARIANT_BOOL Value)
{
	NG_TRACE_METHOD(CMozillaBrowser::put_MenuBar);

    if (!IsValid())
	{
		NG_ASSERT(0);
		RETURN_E_UNEXPECTED();
	}

	//There is no MenuBar in this control.
	return S_OK;
}


HRESULT STDMETHODCALLTYPE CMozillaBrowser::get_FullScreen(VARIANT_BOOL __RPC_FAR *pbFullScreen)
{
	NG_TRACE_METHOD(CMozillaBrowser::get_FullScreen);

    if (!IsValid())
	{
		NG_ASSERT(0);
		RETURN_E_UNEXPECTED();
	}

	NG_ASSERT_POINTER(pbFullScreen, VARIANT_BOOL);
	if (pbFullScreen == NULL)
	{
		RETURN_E_INVALIDARG();
	}

	//FullScreen mode doesn't really apply to this control.
	*pbFullScreen = VARIANT_FALSE;
	return S_OK;
}


HRESULT STDMETHODCALLTYPE CMozillaBrowser::put_FullScreen(VARIANT_BOOL bFullScreen)
{
	NG_TRACE_METHOD(CMozillaBrowser::put_FullScreen);

    if (!IsValid())
	{
		NG_ASSERT(0);
		RETURN_E_UNEXPECTED();
	}

	//FullScreen mode doesn't really apply to this control.
	return S_OK;
}



/////////////////////////////////////////////////////////////////////////////
// IWebBrowser2

HRESULT STDMETHODCALLTYPE CMozillaBrowser::Navigate2(VARIANT __RPC_FAR *URL, VARIANT __RPC_FAR *Flags, VARIANT __RPC_FAR *TargetFrameName, VARIANT __RPC_FAR *PostData, VARIANT __RPC_FAR *Headers)
{
	NG_TRACE_METHOD(CMozillaBrowser::Navigate2);

//Redundant code... taken care of in CMozillaBrowser::Navigate
#if 0
    if (!IsValid())
	{
		NG_ASSERT(0);
		RETURN_E_UNEXPECTED();
	}

	if (URL == NULL)
	{
		NG_ASSERT(0);
		RETURN_E_INVALIDARG();
	}

#endif

	CComVariant vURLAsString;
	if ( vURLAsString.ChangeType(VT_BSTR, URL) != S_OK )
	{
		RETURN_E_INVALIDARG();
	}

	return Navigate(vURLAsString.bstrVal, Flags, TargetFrameName, PostData, Headers);
}


HRESULT STDMETHODCALLTYPE CMozillaBrowser::QueryStatusWB(OLECMDID cmdID, OLECMDF __RPC_FAR *pcmdf)
{
	NG_TRACE_METHOD(CMozillaBrowser::QueryStatusWB);

    if (!IsValid())
	{
		NG_ASSERT(0);
		RETURN_E_UNEXPECTED();
	}

	if (pcmdf == NULL)
	{
		RETURN_E_INVALIDARG();
	}

	// Call through to IOleCommandTarget::QueryStatus
	OLECMD cmd;
	HRESULT hr;
	
	cmd.cmdID = cmdID;
	cmd.cmdf = 0;
	hr = QueryStatus(NULL, 1, &cmd, NULL);
	if (SUCCEEDED(hr))
	{
		*pcmdf = (OLECMDF) cmd.cmdf;
	}

	return hr;
}


HRESULT STDMETHODCALLTYPE CMozillaBrowser::ExecWB(OLECMDID cmdID, OLECMDEXECOPT cmdexecopt, VARIANT __RPC_FAR *pvaIn, VARIANT __RPC_FAR *pvaOut)
{
	NG_TRACE_METHOD(CMozillaBrowser::ExecWB);

    if (!IsValid())
	{
		NG_ASSERT(0);
		RETURN_E_UNEXPECTED();
	}

	// Call through to IOleCommandTarget::Exec
	HRESULT hr;
	hr = Exec(NULL, cmdID, cmdexecopt, pvaIn, pvaOut);
	return hr;
}


HRESULT STDMETHODCALLTYPE CMozillaBrowser::ShowBrowserBar(VARIANT __RPC_FAR *pvaClsid, VARIANT __RPC_FAR *pvarShow, VARIANT __RPC_FAR *pvarSize)
{
	NG_TRACE_METHOD(CMozillaBrowser::ShowBrowserBar);

    if (!IsValid())
	{
		NG_ASSERT(0);
		RETURN_E_UNEXPECTED();
	}

	return S_OK;
}


HRESULT STDMETHODCALLTYPE CMozillaBrowser::get_ReadyState(READYSTATE __RPC_FAR *plReadyState)
{
	NG_TRACE_METHOD(CMozillaBrowser::get_ReadyState);

	if (plReadyState == NULL)
	{
		NG_ASSERT(0);
		RETURN_E_INVALIDARG();
	}

	*plReadyState = m_nBrowserReadyState;

	return S_OK;
}


HRESULT STDMETHODCALLTYPE CMozillaBrowser::get_Offline(VARIANT_BOOL __RPC_FAR *pbOffline)
{
	NG_TRACE_METHOD(CMozillaBrowser::get_Offline);

    if (!IsValid())
	{
		NG_ASSERT(0);
		RETURN_E_UNEXPECTED();
	}

	if (pbOffline == NULL)
	{
		NG_ASSERT(0);
		RETURN_E_INVALIDARG();
	}

	//TODO: Implement get_Offline
	*pbOffline = VARIANT_FALSE;
	return S_OK;
}


HRESULT STDMETHODCALLTYPE CMozillaBrowser::put_Offline(VARIANT_BOOL bOffline)
{
	NG_TRACE_METHOD(CMozillaBrowser::put_Offline);

    if (!IsValid())
	{
		NG_ASSERT(0);
		RETURN_E_UNEXPECTED();
	}

	//TODO: Implement get_Offline
	return S_OK;
}


HRESULT STDMETHODCALLTYPE CMozillaBrowser::get_Silent(VARIANT_BOOL __RPC_FAR *pbSilent)
{
	NG_TRACE_METHOD(CMozillaBrowser::get_Silent);

    if (!IsValid())
	{
		NG_ASSERT(0);
		RETURN_E_UNEXPECTED();
	}

	if (pbSilent == NULL)
	{
		RETURN_E_INVALIDARG();
	}

	//Only really applies to the IE app, not a control
	*pbSilent = VARIANT_FALSE;
	return S_OK;
}


HRESULT STDMETHODCALLTYPE CMozillaBrowser::put_Silent(VARIANT_BOOL bSilent)
{
	NG_TRACE_METHOD(CMozillaBrowser::put_Silent);

    if (!IsValid())
	{
		NG_ASSERT(0);
		RETURN_E_UNEXPECTED();
	}

	//Only really applies to the IE app, not a control
	return S_OK;
}


HRESULT STDMETHODCALLTYPE CMozillaBrowser::get_RegisterAsBrowser(VARIANT_BOOL __RPC_FAR *pbRegister)
{
	NG_TRACE_METHOD(CMozillaBrowser::get_RegisterAsBrowser);

    if (!IsValid())
	{
		NG_ASSERT(0);
		RETURN_E_UNEXPECTED();
	}

	if (pbRegister == NULL)
	{
		NG_ASSERT(0);
		RETURN_E_INVALIDARG();
	}

	//TODO: Implement get_RegisterAsBrowser
	*pbRegister = VARIANT_FALSE;
	return S_OK;
}


HRESULT STDMETHODCALLTYPE CMozillaBrowser::put_RegisterAsBrowser(VARIANT_BOOL bRegister)
{
	NG_TRACE_METHOD(CMozillaBrowser::put_RegisterAsBrowser);

    if (!IsValid())
	{
		NG_ASSERT(0);
		RETURN_E_UNEXPECTED();
	}

	//TODO: Implement put_RegisterAsBrowser
	return S_OK;
}


HRESULT STDMETHODCALLTYPE CMozillaBrowser::get_RegisterAsDropTarget(VARIANT_BOOL __RPC_FAR *pbRegister)
{
	NG_TRACE_METHOD(CMozillaBrowser::get_RegisterAsDropTarget);

    if (!IsValid())
	{
		NG_ASSERT(0);
		RETURN_E_UNEXPECTED();
	}

	if (pbRegister == NULL)
	{
		NG_ASSERT(0);
		RETURN_E_INVALIDARG();
	}

	*pbRegister = m_bDropTarget ? VARIANT_TRUE : VARIANT_FALSE;
	return S_OK;
}



static BOOL CALLBACK EnumChildProc(HWND hwnd, LPARAM lParam)
{
	::RevokeDragDrop(hwnd);
	return TRUE;
}
 

HRESULT STDMETHODCALLTYPE CMozillaBrowser::put_RegisterAsDropTarget(VARIANT_BOOL bRegister)
{
	NG_TRACE_METHOD(CMozillaBrowser::put_RegisterAsDropTarget);

    if (!IsValid())
	{
		NG_ASSERT(0);
		RETURN_E_UNEXPECTED();
	}

	// Register the window as a drop target
	if (bRegister == VARIANT_TRUE)
	{
		if (!m_bDropTarget)
		{
			CDropTargetInstance *pDropTarget = NULL;
			CDropTargetInstance::CreateInstance(&pDropTarget);
			if (pDropTarget)
			{
				pDropTarget->AddRef();
				pDropTarget->SetOwner(this);


				// Ask the site if it wants to replace this drop target for another one
				CIPtr(IDropTarget) spDropTarget;
				CIPtr(IDocHostUIHandler) spDocHostUIHandler = m_spClientSite;
				if (spDocHostUIHandler)
				{
					//if (spDocHostUIHandler->GetDropTarget(pDropTarget, &spDropTarget) != S_OK)
					if (spDocHostUIHandler->GetDropTarget(pDropTarget, &spDropTarget) == S_OK)
					{
						m_bDropTarget = TRUE;
						::RegisterDragDrop(m_hWnd, spDropTarget);
						//spDropTarget = pDropTarget;
					}
				}
				else
				//if (spDropTarget)
				{
					m_bDropTarget = TRUE;
					::RegisterDragDrop(m_hWnd, pDropTarget);
					//::RegisterDragDrop(m_hWnd, spDropTarget);
				}
				pDropTarget->Release();
			}
			// Now revoke any child window drop targets and pray they aren't
			// reset by the layout engine.
			::EnumChildWindows(m_hWnd, EnumChildProc, (LPARAM) this);
		}
	}
	else
	{
		if (m_bDropTarget)
		{
			m_bDropTarget = FALSE;
			::RevokeDragDrop(m_hWnd);
		}
	}

	return S_OK;
}


HRESULT STDMETHODCALLTYPE CMozillaBrowser::get_TheaterMode(VARIANT_BOOL __RPC_FAR *pbRegister)
{
	NG_TRACE_METHOD(CMozillaBrowser::get_TheaterMode);

    if (!IsValid())
	{
		NG_ASSERT(0);
		RETURN_E_UNEXPECTED();
	}

	if (pbRegister == NULL)
	{
		NG_ASSERT(0);
		RETURN_E_INVALIDARG();
	}

	//TheaterMode doesn't apply to this control.
	*pbRegister = VARIANT_FALSE;

	return S_OK;
}


HRESULT STDMETHODCALLTYPE CMozillaBrowser::put_TheaterMode(VARIANT_BOOL bRegister)
{
	NG_TRACE_METHOD(CMozillaBrowser::put_TheaterMode);

    if (!IsValid())
	{
		NG_ASSERT(0);
		RETURN_E_UNEXPECTED();
	}

	//TheaterMode doesn't apply to this control.
	return S_OK;
}


HRESULT STDMETHODCALLTYPE CMozillaBrowser::get_AddressBar(VARIANT_BOOL __RPC_FAR *Value)
{
	NG_TRACE_METHOD(CMozillaBrowser::get_AddressBar);

    if (!IsValid())
	{
		NG_ASSERT(0);
		RETURN_E_UNEXPECTED();
	}

	if (Value == NULL)
	{
		RETURN_E_INVALIDARG();
	}

	//There is no AddressBar in this control.
	*Value = VARIANT_FALSE;
	return S_OK;
}


HRESULT STDMETHODCALLTYPE CMozillaBrowser::put_AddressBar(VARIANT_BOOL Value)
{
	NG_TRACE_METHOD(CMozillaBrowser::put_AddressBar);

    if (!IsValid())
	{
		NG_ASSERT(0);
		RETURN_E_UNEXPECTED();
	}

	//There is no AddressBar in this control.
	return S_OK;
}


HRESULT STDMETHODCALLTYPE CMozillaBrowser::get_Resizable(VARIANT_BOOL __RPC_FAR *Value)
{
	NG_TRACE_METHOD(CMozillaBrowser::get_Resizable);

    if (!IsValid())
	{
		NG_ASSERT(0);
		RETURN_E_UNEXPECTED();
	}

	if (Value == NULL)
	{
		NG_ASSERT(0);
		RETURN_E_INVALIDARG();
	}

	//TODO:  Not sure if this should actually be implemented or not.
	*Value = VARIANT_FALSE;
	return S_OK;
}


HRESULT STDMETHODCALLTYPE CMozillaBrowser::put_Resizable(VARIANT_BOOL Value)
{
	NG_TRACE_METHOD(CMozillaBrowser::put_Resizable);

    if (!IsValid())
	{
		NG_ASSERT(0);
		RETURN_E_UNEXPECTED();
	}

	//TODO:  Not sure if this should actually be implemented or not.
	return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
// Ole Command Handlers


HRESULT _stdcall CMozillaBrowser::EditModeHandler(CMozillaBrowser *pThis, const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
	BOOL bEditorMode = (nCmdID == IDM_EDITMODE) ? TRUE : FALSE;
	pThis->SetEditorMode(bEditorMode);
	return S_OK;
}


HRESULT _stdcall CMozillaBrowser::EditCommandHandler(CMozillaBrowser *pThis, const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
	pThis->OnEditorCommand(nCmdID);
	return S_OK;
}
