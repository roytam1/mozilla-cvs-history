/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

#include "stdafx.h"
#include "usertlbr.h"
#include "shcut.h"
#include "dropmenu.h"
#include "prefapi.h"
#include "rdfliner.h"

extern "C" {
#include "xpgetstr.h"
};

// The Nav Center vocab element
extern "C" RDF_NCVocab gNavCenter;

#define TEXT_CHARACTERS_SHOWN 9
#define BORDERSIZE 2
#define TEXTVERTMARGIN 1
#define TEXTONLYVERTMARGIN 2
#define BITMAPVERTMARGIN 2
#define TEXT_BITMAPVERTMARGIN 1
#define HORIZMARGINSIZE 4

#define LEFT_TOOLBAR_MARGIN 10
#define RIGHT_TOOLBAR_MARGIN 20
#define SPACE_BETWEEN_BUTTONS 2

#define MAX_TOOLBAR_BUTTONS 60
#define MAX_TOOLBAR_ROWS 3

int CRDFToolbar::m_nMinToolbarButtonChars = 15;
int CRDFToolbar::m_nMaxToolbarButtonChars = 30;

//////////////////////////////////////////////////////////////////////////////////////
//							CRDFToolbarButtonDropTarget
//////////////////////////////////////////////////////////////////////////////////////

DROPEFFECT CRDFToolbarButtonDropTarget::ProcessDragEnter(CWnd *pWnd, COleDataObject *pDataObject, 
			DWORD dwKeyState, CPoint point)
{

	// Drop target is only used for drops directly onto folder personal toolbar buttons.
	// This drop target is NOT used for the Aurora selector bar drops.
	return ProcessDragOver(pWnd, pDataObject, dwKeyState, point);
}

DROPEFFECT CRDFToolbarButtonDropTarget::ProcessDragOver(CWnd *pWnd, COleDataObject *pDataObject, 
			DWORD dwKeyState, CPoint point)
{
	CRDFToolbarButton* pButton = (CRDFToolbarButton*)m_pButton;

	// Treat as a drop onto the folder.
	return RDFGLOBAL_TranslateDropAction(pButton->GetNode(), pDataObject, 2);
}

BOOL CRDFToolbarButtonDropTarget::ProcessDrop(CWnd *pWnd, COleDataObject *pDataObject, 
			DROPEFFECT dropEffect, CPoint point)
{
	// Treat as a drop onto the folder.
	CRDFToolbarButton* pButton = (CRDFToolbarButton*)m_pButton;
	RDFGLOBAL_PerformDrop(pDataObject, pButton->GetNode(), 2);
	return TRUE;
}

//////////////////////////////////////////////////////////////////////////////////////
//							CRDFToolbarButton
//////////////////////////////////////////////////////////////////////////////////////
static HBITMAP m_hbmpBM = NULL;
static HBITMAP m_hbmpBMSelected = NULL;
static HBITMAP m_hbmpFolder = NULL;
static HBITMAP m_hbmpSelectedFolder = NULL;
static HBITMAP m_hbmpFolderOpen =  NULL;
static int nBitmapRefCount = 0;

CRDFToolbarButton::CRDFToolbarButton()
{
	m_bShouldShowRMMenu = TRUE;

	m_Node = NULL;

	m_uCurBMID = 0;

    currentRow = 0;
}


CRDFToolbarButton::~CRDFToolbarButton()
{
	if(m_Node && HT_IsContainer(m_Node))
	{
		nBitmapRefCount--;

		if(nBitmapRefCount == 0)
		{
			if(m_hbmpBM)
			{
				DeleteObject(m_hbmpBM);
			}

			if(m_hbmpFolder)
			{
				DeleteObject(m_hbmpFolder);
			}

			if(m_hbmpSelectedFolder)
			{
				DeleteObject(m_hbmpSelectedFolder);
			}

			if(m_hbmpFolderOpen)
			{
				DeleteObject(m_hbmpFolderOpen);
			}

			if(m_hbmpBMSelected)
			{
				DeleteObject(m_hbmpBMSelected);
			}
		}

	}
}

int CRDFToolbarButton::Create(CWnd *pParent, int nToolbarStyle, CSize noviceButtonSize, CSize advancedButtonSize,
								LPCTSTR pButtonText, LPCTSTR pToolTipText,
								LPCTSTR pStatusText, 
								CSize bitmapSize, int nMaxTextChars, int nMinTextChars, BOOKMARKITEM bookmark,
								HT_Resource pNode, DWORD dwButtonStyle )
{
	m_bookmark = bookmark;

	BOOL bResult = CToolbarButton::Create(pParent, nToolbarStyle, noviceButtonSize, advancedButtonSize,
		pButtonText, pToolTipText, pStatusText, 0, 0,
		bitmapSize, TRUE, 0, nMaxTextChars, nMinTextChars, dwButtonStyle);

	if(bResult)
	{
		SetNode(pNode);
		UpdateIconInfo();		
		if (m_menu.m_hMenu == NULL || (m_menu.m_hMenu != NULL && !IsMenu(m_menu.m_hMenu)))
          m_menu.CreatePopupMenu();
	}

	return bResult;
}

void CRDFToolbarButton::UpdateIconInfo()
{
	m_nIconType = DetermineIconType(m_Node, UseLargeIcons());
	UINT oldBitmapID = m_nBitmapID;
	m_nBitmapID = GetBitmapID();
	if (m_nBitmapID != oldBitmapID)
	{
		HDC hDC = ::GetDC(m_hWnd);
		HPALETTE hPalette = WFE_GetUIPalette(GetParentFrame());
		HBITMAP hBitmap = WFE_LookupLoadAndEnterBitmap(hDC, m_nBitmapID, TRUE, hPalette,
													   sysInfo.m_clrBtnFace, RGB(255, 0, 255));
		::ReleaseDC(m_hWnd, hDC);
		SetBitmap(hBitmap, TRUE);
		if (m_nBitmapID == IDB_PICTURES)
			SetBitmapSize(CSize(23,21));	// Command buttons
		else SetBitmapSize(CSize(23,17)); // Personal toolbar buttons
	}

	m_nBitmapIndex = GetBitmapIndex(); 
}

CSize CRDFToolbarButton::GetMinimalButtonSize(void)
{
    CString szText(HT_GetNodeName(m_Node));
    return GetButtonSizeFromChars(szText, m_nMinTextChars);
}

CSize CRDFToolbarButton::GetMaximalButtonSize(void)
{
    CString szText(HT_GetNodeName(m_Node));
    return GetButtonSizeFromChars(szText, m_nMaxTextChars);
}

void CRDFToolbarButton::OnAction(void)
{
	if(m_Node)
	{
		char* url = HT_GetNodeURL(m_Node);
		if (strncmp(url, "command:", 8) == 0)
		{
			// We're a command, baby.  Look up our FE command and execute it.
			UINT nCommand = theApp.m_pBrowserCommandMap->GetFEResource(url);
			WFE_GetOwnerFrame(this)->PostMessage(WM_COMMAND, MAKEWPARAM(nCommand, nCommand), 0);
		}
		else if (!HT_IsContainer(m_Node))
		{
			CAbstractCX * pCX = FEU_GetLastActiveFrameContext();
			ASSERT(pCX != NULL);
			if (pCX != NULL)
			{
				if (!HT_Launch(m_Node, pCX->GetContext()))
					pCX->NormalGetUrl((LPTSTR)HT_GetNodeURL(m_Node));
			}
		}
	}
}

void CRDFToolbarButton::FillInOleDataSource(COleDataSource *pDataSource)
{
	CLIPFORMAT cfHTNode = (CLIPFORMAT)RegisterClipboardFormat(NETSCAPE_HTNODE_FORMAT);
    HANDLE hString = GlobalAlloc(GMEM_MOVEABLE|GMEM_DDESHARE,1);
    pDataSource->CacheGlobalData(cfHTNode, hString);

	// Need to "select" the button in the view.  Hack.
	HT_View theView = HT_GetView(m_Node);
	HT_SetSelectedView(HT_GetPane(theView), theView); // Make sure this view is selected in the pane.
	HT_SetSelection(m_Node);

	// Now the view is sufficient
	RDFGLOBAL_BeginDrag(pDataSource, HT_GetView(m_Node));
	
}

BOOKMARKITEM CRDFToolbarButton::GetBookmarkItem(void)
{
	return m_bookmark;
}

void CRDFToolbarButton::SetBookmarkItem(BOOKMARKITEM bookmark)
{
/*	m_bookmark = bookmark;
    CString strText(bookmark.szText);
	CToolbarButton::SetText(strText);
	*/
}

void CRDFToolbarButton::SetNode(HT_Resource pNode, BOOL bAddRef)
{
	if (pNode == NULL)
		return;

	m_Node = pNode;
	
	// if it's a header and we haven't already loaded the bitmaps, load the bitmaps
	if(HT_IsContainer(pNode) && bAddRef)
	{	
		HDC hDC = ::GetDC(m_hWnd);

		HINSTANCE hInstance = AfxGetResourceHandle();
		WFE_InitializeUIPalette(hDC);
		HPALETTE hPalette = WFE_GetUIPalette(GetParentFrame());
		if(nBitmapRefCount == 0)
		{
			m_hbmpBM = WFE_LoadTransparentBitmap(hInstance, hDC, sysInfo.m_clrMenu, RGB(255, 0, 255),
												 hPalette, IDB_BOOKMARK_ITEM);
			m_hbmpBMSelected = WFE_LoadTransparentBitmap(hInstance, hDC,sysInfo.m_clrHighlight,
														 RGB(255, 0, 255), hPalette, IDB_BOOKMARK_ITEM);
			m_hbmpFolder = WFE_LoadTransparentBitmap(hInstance, hDC,sysInfo.m_clrMenu, RGB(255, 0, 255),
													 hPalette, IDB_BOOKMARK_FOLDER2);

			m_hbmpSelectedFolder = WFE_LoadTransparentBitmap(hInstance, hDC, sysInfo.m_clrHighlight, RGB(255, 0, 255),
													 hPalette, IDB_BOOKMARK_FOLDER2);

			m_hbmpFolderOpen = WFE_LoadTransparentBitmap(hInstance, hDC, sysInfo.m_clrHighlight,
														 RGB(255, 0, 255), hPalette, IDB_BOOKMARK_FOLDER_OPEN);
		}
		nBitmapRefCount++;

		::ReleaseDC(m_hWnd, hDC);
	}

}

void CRDFToolbarButton::EditTextChanged(char *pText)
{
	if (pText)
	{
		HT_SetNodeName(m_Node, pText);
		SetText(pText);	
		SetToolTipText(pText);
		delete []pText;
	}
    RemoveTextEdit();
	
	if (foundOnRDFToolbar())
		((CRDFToolbar*)GetParent())->LayoutButtons(-1);
}

void CRDFToolbarButton::DrawPicturesAndTextMode(HDC hDC, CRect rect)
{
	if (foundOnRDFToolbar())
	{
		CRDFToolbar* theToolbar = (CRDFToolbar*)GetParent();
		void* data;
		HT_GetNodeData(HT_TopNode(theToolbar->GetHTView()), gNavCenter->toolbarBitmapPosition, HT_COLUMN_STRING, &data);
		if (data)
		{
			CString position((char*)data);
			if (position == "top")
			{
				DrawBitmapOnTop(hDC, rect);
				return;
			}
		}
	}
	
	DrawBitmapOnSide(hDC, rect);
}

void CRDFToolbarButton::DrawPicturesMode(HDC hDC, CRect rect)
{
	if (foundOnRDFToolbar())
	{
		CRDFToolbar* theToolbar = (CRDFToolbar*)GetParent();
		void* data;
		HT_GetNodeData(HT_TopNode(theToolbar->GetHTView()), gNavCenter->toolbarBitmapPosition, HT_COLUMN_STRING, &data);
		if (data)
		{
			CString position((char*)data);
			if (position == "top")
			{
				DrawBitmapOnTop(hDC, rect);
				return;
			}
		}
	}

	DrawBitmapOnSide(hDC, rect);
}

void CRDFToolbarButton::DrawButtonText(HDC hDC, CRect rcTxt, CSize sizeTxt, CString strTxt)
{
	if (foundOnRDFToolbar())
	{
		hasCustomTextColor = TRUE;
		switch (m_eState)
		{
			case eDISABLED:
				customTextColor = ((CRDFToolbar*)GetParent())->GetDisabledColor();
				if (customTextColor == -1)
				{
					hasCustomTextColor = FALSE;
				}
				break;
			case eBUTTON_UP:
				customTextColor = ((CRDFToolbar*)GetParent())->GetRolloverColor();
				break;
			case eBUTTON_DOWN:
				customTextColor = ((CRDFToolbar*)GetParent())->GetPressedColor();
				break;
			case eNORMAL:
				customTextColor = ((CRDFToolbar*)GetParent())->GetForegroundColor();
				break;
		}
	}

    CToolbarButton::DrawButtonText(hDC, rcTxt, sizeTxt, strTxt);
}

CSize CRDFToolbarButton::GetButtonSizeFromChars(CString s, int c)
{
    if(m_nToolbarStyle != TB_TEXT)
	{
		if (foundOnRDFToolbar())
		{
			CRDFToolbar* theToolbar = (CRDFToolbar*)GetParent();
			void* data;
			HT_GetNodeData(HT_TopNode(theToolbar->GetHTView()), gNavCenter->toolbarBitmapPosition, HT_COLUMN_STRING, &data);
			if (data)
			{
				CString position((char*)data);
				if (position == "top")
				{
					return GetBitmapOnTopSize(s, c);
				}
			}
		}
		return(GetBitmapOnSideSize(s, c));
	}
	else
		return(GetTextOnlySize(s, c));
}

void CRDFToolbarButton::GetPicturesAndTextModeTextRect(CRect &rect)
{
	if (foundOnRDFToolbar())
	{
		CRDFToolbar* theToolbar = (CRDFToolbar*)GetParent();
		void* data;
		HT_GetNodeData(HT_TopNode(theToolbar->GetHTView()), gNavCenter->toolbarBitmapPosition, HT_COLUMN_STRING, &data);
		if (data)
		{
			CString position((char*)data);
			if (position == "top")
			{
				GetBitmapOnTopTextRect(rect);
				return;
			}
		}
	}

	GetBitmapOnSideTextRect(rect);
}

void CRDFToolbarButton::GetPicturesModeTextRect(CRect &rect)
{
	if (foundOnRDFToolbar())
	{
		CRDFToolbar* theToolbar = (CRDFToolbar*)GetParent();
		void* data;
		HT_GetNodeData(HT_TopNode(theToolbar->GetHTView()), gNavCenter->toolbarBitmapPosition, HT_COLUMN_STRING, &data);
		if (data)
		{
			CString position((char*)data);
			if (position == "top")
			{
				GetBitmapOnTopTextRect(rect);
				return;
			}
		}
	}

	GetBitmapOnSideTextRect(rect);
}


BOOL CRDFToolbarButton::CreateRightMouseMenu(void)
{
	if (m_bShouldShowRMMenu)
	{
		m_MenuCommandMap.Clear();
		HT_View theView = HT_GetView(m_Node);
		HT_SetSelectedView(HT_GetPane(theView), theView); // Make sure this view is selected in the pane.
		HT_SetSelection(m_Node); // Make sure the node is the selection in the view.
		
		HT_Cursor theCursor = HT_NewContextualMenuCursor(theView, PR_FALSE, PR_FALSE);
		if (theCursor != NULL)
		{
			// We have a cursor. Attempt to iterate
			HT_MenuCmd theCommand; 
			while (HT_NextContextMenuItem(theCursor, &theCommand))
			{
				char* menuName = HT_GetMenuCmdName(theCommand);
				if (theCommand == HT_CMD_SEPARATOR)
					m_menu.AppendMenu(MF_SEPARATOR);
				else
				{
					// Add the command to our command map
					CRDFMenuCommand* rdfCommand = new CRDFMenuCommand(menuName, theCommand);
					int index = m_MenuCommandMap.AddCommand(rdfCommand);
					m_menu.AppendMenu(MF_ENABLED, index+FIRST_HT_MENU_ID, menuName);
				}
			}
			HT_DeleteCursor(theCursor);
		}
	}
	return m_bShouldShowRMMenu;
}

CWnd* CRDFToolbarButton::GetMenuParent(void)
{
	return this;
}

BOOL CRDFToolbarButton::OnCommand(UINT wParam, LONG lParam)
{
	BOOL bRtn = TRUE;
	
	if (wParam >= FIRST_HT_MENU_ID && wParam <= LAST_HT_MENU_ID)
	{
		// A selection was made from the context menu.
		// Use the menu map to get the HT command value
		CRDFMenuCommand* theCommand = (CRDFMenuCommand*)(m_MenuCommandMap.GetCommand((int)wParam-FIRST_HT_MENU_ID));
		if (theCommand)
		{
			HT_MenuCmd htCommand = theCommand->GetHTCommand();
			HT_DoMenuCmd(HT_GetPane(GetHTView()), htCommand);
		}

		return bRtn;
	}

	if(wParam == ID_HOTLIST_VIEW || wParam == ID_HOTLIST_ADDCURRENTTOHOTLIST)
	{
		GetParentFrame()->SendMessage(WM_COMMAND, wParam, lParam);
	}
	else if(wParam == ID_DELETE_BUTTON)
	{
		HT_RemoveChild(HT_GetParent(m_Node), m_Node);
	}
	else if(wParam == ID_BUTTON_PROPERTIES)
	{
		// PROPERTIES
	}
	else if(wParam == ID_RENAME_BUTTON)
	{
		AddTextEdit();
		SetTextEditText(m_pButtonText);
	}
	// Only interested in commands from bookmark quick file menu
	else if ((wParam >= FIRST_BOOKMARK_MENU_ID))
	{
		void * pBookmark = NULL;
		m_BMMenuMap.Lookup(wParam, pBookmark);
		if (pBookmark != NULL)
		{
			HT_Resource theNode = (HT_Resource)pBookmark;
			CAbstractCX *pCX = FEU_GetLastActiveFrameContext();
			ASSERT(pCX != NULL);
			if (pCX != NULL)
			{
				if (!HT_Launch(theNode, pCX->GetContext()))
					pCX->NormalGetUrl((LPTSTR)HT_GetNodeURL(theNode));
			}
		}
	}
	else 
	{
		bRtn = CRDFToolbarButtonBase::OnCommand(wParam, lParam);
	}
	
	return(bRtn);
	
} 

///////////////////////////////////////////////////////////////////////////////////
//									CRDFToolbarButton Messages
///////////////////////////////////////////////////////////////////////////////////

BEGIN_MESSAGE_MAP(CRDFToolbarButton, CRDFToolbarButtonBase)
	//{{AFX_MSG_MAP(CRDFToolbarButton)
	ON_MESSAGE(NSDRAGMENUOPEN, OnDragMenuOpen) 
	ON_MESSAGE(DM_FILLINMENU, OnFillInMenu)
	ON_MESSAGE(DT_DROPOCCURRED, OnDropMenuDropOccurred)
	ON_MESSAGE(DT_DRAGGINGOCCURRED, OnDropMenuDraggingOccurred)
	ON_MESSAGE(DM_MENUCLOSED, OnDropMenuClosed)
	ON_WM_SYSCOLORCHANGE()
	ON_WM_PAINT()
		//}}AFX_MSG_MAP

END_MESSAGE_MAP()



void CRDFToolbarButton::OnPaint()
{
	CRect updateRect;

	GetUpdateRect(&updateRect);

	CPaintDC dcPaint(this);	// device context for painting
	CRect rcClient;
	GetClientRect(&rcClient);
	
	HDC hSrcDC = dcPaint.m_hDC;

	HPALETTE hPalette;
	HPALETTE hOldPalette;
	CFrameWnd* pParent = WFE_GetOwnerFrame(this); 
	hPalette = WFE_GetUIPalette(pParent);
	hOldPalette= ::SelectPalette(hSrcDC, hPalette, FALSE);
	HDC hMemDC = ::CreateCompatibleDC(hSrcDC);

	if(hMemDC)
	{
		HPALETTE hOldMemPalette = ::SelectPalette(hMemDC, hPalette, FALSE);

		HBITMAP hbmMem = CreateCompatibleBitmap(hSrcDC,
										rcClient.Width(),
										rcClient.Height());

		//
		// Select the bitmap into the off-screen DC.
		//
		HBITMAP hbmOld = (HBITMAP)::SelectObject(hMemDC, hbmMem);

		// if we are not enabled then must make sure that button state is normal
		if(!m_bEnabled)
		{
			m_eState = eNORMAL;
		}

		CRect innerRect = rcClient;

		innerRect.InflateRect(-BORDERSIZE, -BORDERSIZE);

		int oldMode = ::SetBkMode(hMemDC, TRANSPARENT);
		if (foundOnRDFToolbar())
		{
			CRDFToolbar* pToolbar = (CRDFToolbar*)GetParent();
			if (pToolbar->GetBackgroundImage() == NULL ||
				(pToolbar->GetBackgroundImage() != NULL && 
				 !pToolbar->GetBackgroundImage()->FrameSuccessfullyLoaded()))
			{
				// Fill with our background color.
				HBRUSH hRegBrush = (HBRUSH) ::CreateSolidBrush(pToolbar->GetBackgroundColor());
				::FillRect(hMemDC, rcClient, hRegBrush);
			}
			else 
			{
				// There is a background. Let's do a tile on the rect corresponding to our position
				// in the parent toolbar.
				CWnd* pGrandParent = pToolbar->GetParent();
				CRect offsetRect(rcClient);
				MapWindowPoints(pGrandParent, &offsetRect);

				// Now we want to fill the given rectangle.
				PaintBackground(hMemDC, rcClient, pToolbar->GetBackgroundImage(), offsetRect.left, offsetRect.top);
			}
		}
		else
		{
			::FillRect(hMemDC, rcClient, sysInfo.m_hbrBtnFace);
		}

		if(m_nToolbarStyle == TB_PICTURESANDTEXT)
		{
			DrawPicturesAndTextMode(hMemDC, innerRect);
		}
		else if(m_nToolbarStyle == TB_PICTURES)
		{
			DrawPicturesMode(hMemDC, innerRect);
		}
		else
		{
			DrawTextMode(hMemDC, innerRect);
		}

		// Now, draw 3d visual button effects, depending on our state
		switch (m_eState)
		{
			case eBUTTON_UP:
			{
                if( m_nChecked == 0 )
    				DrawUpButton(hMemDC, rcClient);
                else
    			    DrawDownButton(hMemDC, rcClient);
			}
			break;
			
			case eBUTTON_CHECKED:
			{
				// A checked button but NOT mousing over - no black border
                DrawCheckedButton(hMemDC, rcClient);
			}
			break;

			case eBUTTON_DOWN:
			{
				DrawDownButton(hMemDC, rcClient);
			}
			break;

			case eDISABLED:
			{
				if(m_nChecked == 2)
					DrawCheckedButton(hMemDC, rcClient);

			}
			break;

			case eNORMAL:
			{
				if (m_bDepressed)
					DrawDownButton(hMemDC, rcClient); // Looks like it's locked down.
			}
		}

		::SetBkMode(hMemDC, oldMode);

		::BitBlt(hSrcDC, 0, 0, rcClient.Width(), rcClient.Height(), hMemDC, 0, 0,
						SRCCOPY);
	
		::SelectPalette(hMemDC, hOldMemPalette, FALSE);
		::SelectPalette(hSrcDC, hOldPalette, FALSE);

		::SelectObject(hMemDC, hbmOld);
		::DeleteObject(hbmMem);
 
		::DeleteDC(hMemDC);
	}
}

LRESULT CRDFToolbarButton::OnDragMenuOpen(WPARAM wParam, LPARAM lParam)
{
// Set our drop menu's user data.
	if (m_pDropMenu)
		m_pDropMenu->SetUserData(m_Node);

	if(m_Node == NULL || !HT_IsContainer(m_Node))
		return 1;
	m_uCurBMID = FIRST_BOOKMARK_MENU_ID;

	m_pCachedDropMenu = (CDropMenu *)lParam;  // Set our drop menu
	PRBool isOpen;

	HT_Resource theNode = (HT_Resource)m_pCachedDropMenu->GetUserData();
	HT_GetOpenState(theNode, &isOpen);
	if (isOpen)
		FillInMenu(theNode);
	else HT_SetOpenState(theNode, (PRBool)TRUE);

	return 1;

}

LRESULT CRDFToolbarButton::OnFillInMenu(WPARAM wParam, LPARAM lParam)
{
// Set our drop menu's user data.
	if (m_pDropMenu)
		m_pDropMenu->SetUserData(m_Node);

	m_pCachedDropMenu = (CDropMenu *)lParam;  // Set our drop menu
	PRBool isOpen;
	HT_Resource theNode = (HT_Resource)m_pCachedDropMenu->GetUserData();
	HT_GetOpenState(theNode, &isOpen);
	if (isOpen)
		FillInMenu(theNode);
	else HT_SetOpenState(theNode, (PRBool)TRUE);
	
	return 1;
}

LRESULT CRDFToolbarButton::OnDropMenuDraggingOccurred(WPARAM wParam, LPARAM lParam)
{
	CDropMenuDragData* pData = (CDropMenuDragData*)lParam;
	UINT nCommand = pData->m_nCommand;
	MenuSelectionType eSelType = pData->eSelType;

	void * pBookmarkInsertAfter = NULL;
	m_BMMenuMap.Lookup(nCommand, pBookmarkInsertAfter);
	HT_Resource theNode = (HT_Resource)pBookmarkInsertAfter;

	int dragFraction = 2;
	if (eSelType == eON)
		dragFraction = 2;
	else if (eSelType == eBELOW)
		dragFraction = 3;
	else if (eSelType == eABOVE)
		dragFraction = 1;

	// Next we get the result.
	DROPEFFECT answer = RDFGLOBAL_TranslateDropAction(theNode, pData->m_pDataSource, dragFraction);
	
	// Place the result into our data structure for the droptarget to use.
	pData->m_DropEffect = answer;
	
	return 1;

}

LRESULT CRDFToolbarButton::OnDropMenuDropOccurred(WPARAM wParam, LPARAM lParam)
{
	CDropMenuDragData* pData = (CDropMenuDragData*)lParam;
	UINT nCommand = pData->m_nCommand;
	MenuSelectionType eSelType = pData->eSelType;

	void * pBookmarkInsertAfter = NULL;
	m_BMMenuMap.Lookup(nCommand, pBookmarkInsertAfter);
	HT_Resource theNode = (HT_Resource)pBookmarkInsertAfter;

	int dragFraction = 2;
	if (eSelType == eON)
		dragFraction = 2;
	else if (eSelType == eBELOW)
		dragFraction = 3;
	else if (eSelType == eABOVE)
		dragFraction = 1;

	RDFGLOBAL_PerformDrop(pData->m_pDataSource, theNode, dragFraction);
	
	delete pData; // Clean this structure up.

	return 1;
}
	
LRESULT CRDFToolbarButton::OnDropMenuClosed(WPARAM wParam, LPARAM lParam)
{
	int nCount;
	if(m_pDropMenu != NULL)
	{
		HT_Resource theNode = (HT_Resource)m_pDropMenu->GetUserData();
		nCount = m_pDropMenu->GetMenuItemCount();

		// clean out the menu 
		for(int i = nCount - 1; i >= 0; i--)
		{
			m_pDropMenu->DeleteMenu(i, MF_BYPOSITION);
		}
		m_pDropMenu->DestroyDropMenu();
		delete m_pDropMenu;
		m_pDropMenu = NULL;

		if (theNode != NULL)
			HT_SetOpenState(theNode, (PRBool)FALSE);
	}
	
	return 1;
}

void CRDFToolbarButton::OnSysColorChange( )
{
	if(nBitmapRefCount > 0)
	{
		VERIFY(::DeleteObject(m_hbmpBM));
		VERIFY(::DeleteObject(m_hbmpBMSelected));
		VERIFY(::DeleteObject(m_hbmpFolder));
		VERIFY(::DeleteObject(m_hbmpSelectedFolder));
		VERIFY(::DeleteObject(m_hbmpFolderOpen));
		
		HINSTANCE hInstance = AfxGetResourceHandle();
		HDC hDC = ::GetDC(m_hWnd);
		WFE_InitializeUIPalette(hDC);
		HPALETTE hPalette = WFE_GetUIPalette(GetParentFrame());

		m_hbmpBM = WFE_LoadTransparentBitmap(hInstance, hDC, sysInfo.m_clrMenu, RGB(255, 0, 255),
											 hPalette, IDB_BOOKMARK_ITEM);
		m_hbmpBMSelected = WFE_LoadTransparentBitmap(hInstance, hDC,sysInfo.m_clrHighlight,
													 RGB(255, 0, 255), hPalette, IDB_BOOKMARK_ITEM);
		m_hbmpFolder = WFE_LoadTransparentBitmap(hInstance, hDC,sysInfo.m_clrMenu, RGB(255, 0, 255),
												 hPalette, IDB_BOOKMARK_FOLDER2);

		m_hbmpSelectedFolder = WFE_LoadTransparentBitmap(hInstance, hDC,sysInfo.m_clrHighlight, RGB(255, 0, 255),
												 hPalette, IDB_BOOKMARK_FOLDER2);

		m_hbmpFolderOpen = WFE_LoadTransparentBitmap(hInstance, hDC, sysInfo.m_clrHighlight,
														 RGB(255, 0, 255), hPalette, IDB_BOOKMARK_FOLDER_OPEN);

		::ReleaseDC(m_hWnd, hDC);
	}

	if(m_bBitmapFromParent)
	{
		HDC hDC = ::GetDC(m_hWnd);

		HPALETTE hPalette = WFE_GetUIPalette(GetParentFrame());

		HBITMAP hBitmap = WFE_LookupLoadAndEnterBitmap(hDC, m_nBitmapID, TRUE, hPalette,
													   sysInfo.m_clrBtnFace, RGB(255, 0, 255));
		::ReleaseDC(m_hWnd, hDC);
		SetBitmap(hBitmap, TRUE);
	}
	CToolbarButton::OnSysColorChange();
}

///////////////////////////////////////////////////////////////////////////
//							CRDFToolbarButton Helpers
///////////////////////////////////////////////////////////////////////////

void CRDFToolbarButton::FillInMenu(HT_Resource theNode)
{
	m_pCachedDropMenu->CreateOverflowMenuItem(ID_HOTLIST_VIEW, CString(szLoadString(IDS_MORE_BOOKMARKS)), NULL, NULL );

	HT_Cursor theCursor = HT_NewCursor(theNode);
	HT_Resource theItem = NULL;
	while (theItem = HT_GetNextItem(theCursor))
	{
		IconType nIconType = DetermineIconType(theItem, FALSE);
		void* pCustomIcon = NULL;
		if (nIconType == LOCAL_FILE)
			pCustomIcon = FetchLocalFileIcon(theItem);
		else if (nIconType == ARBITRARY_URL)
			pCustomIcon = FetchCustomIcon(theItem, this, FALSE);

		HT_SetNodeFEData(theItem, this);

		if (HT_IsContainer(theItem))
		{
			CDropMenu *pSubMenu = new CDropMenu;
			pSubMenu->SetUserData(theItem);

			CString csAmpersandString = FEU_EscapeAmpersand(HT_GetNodeName(theItem));

			m_pCachedDropMenu->AppendMenu(MF_POPUP, m_uCurBMID, pSubMenu, FALSE, 
				csAmpersandString, TRUE, m_hbmpFolder, m_hbmpFolderOpen, pCustomIcon, nIconType);
			
			m_BMMenuMap.SetAt(m_uCurBMID, theItem);
			m_uCurBMID++;
		}
		else if (!HT_IsSeparator(theItem))
		{
			CString csAmpersandString = FEU_EscapeAmpersand(HT_GetNodeName(theItem));
			m_pCachedDropMenu->AppendMenu(MF_STRING, m_uCurBMID, csAmpersandString, TRUE, 
				m_hbmpBM, m_hbmpBMSelected, pCustomIcon, nIconType);
			m_BMMenuMap.SetAt(m_uCurBMID, theItem);
			m_uCurBMID++;
		}
		else
		{
			m_pCachedDropMenu->AppendMenu(MF_SEPARATOR, 0, TRUE, NULL, NULL);
		}
	}
	
	HT_DeleteCursor(theCursor);
	m_pCachedDropMenu = NULL;
}

UINT CRDFToolbarButton::GetBitmapID(void)
{
	if (m_Node)
	{
		if (strncmp(HT_GetNodeURL(m_Node), "command:", 8) == 0)
		{
			// We're an internal command.
			return IDB_PICTURES;
		}
		else if (HT_IsContainer(m_Node))
			return IDB_BUTTON_FOLDER;
		else return IDB_USERBTN;
	}

	return IDB_USERBTN;
}

UINT CRDFToolbarButton::GetBitmapIndex(void)
{
	if (m_Node)
	{
		if (strncmp(HT_GetNodeURL(m_Node), "command:", 8) == 0)
		{
			// We're an internal command.  Access the command bitmap strip.
			return theApp.m_pCommandToolbarIndices->GetFEResource(HT_GetNodeURL(m_Node));
		}
	}
	return 0;
}

void CRDFToolbarButton::SetTextWithoutResize(CString text)
{
    if(m_pButtonText != NULL)
	{
		XP_FREE(m_pButtonText);
	}

	m_pButtonText = (LPTSTR)XP_ALLOC(text.GetLength() +1);
	XP_STRCPY(m_pButtonText, text);
}

BOOL CRDFToolbarButton::NeedsUpdate()
{
	// Only internal commands need updating via the OnUpdateCmdUI handler.
	// All other buttons are enabled/disabled using the appropriate HT calls.
	if (m_Node && strncmp(HT_GetNodeURL(m_Node), "command:", 8) == 0)
	{
		m_nCommand = theApp.m_pBrowserCommandMap->GetFEResource(HT_GetNodeURL(m_Node));
		return TRUE;
	}
	return FALSE;
}

void CRDFToolbarButton::LoadComplete(HT_Resource r)
{
	Invalidate();
}

void CRDFToolbarButton::DrawCustomIcon(HDC hDC, int x, int y)
{
	CRDFImage* pCustomImage = DrawArbitraryURL(m_Node, x, y, m_bitmapSize.cx, m_bitmapSize.cy, hDC, 
					m_bDepressed ? (::GetSysColor(COLOR_BTNSHADOW)) :  (::GetSysColor(COLOR_BTNFACE)), 
					 this, UseLargeIcons());
	
	if (foundOnRDFToolbar() && pCustomImage->FrameSuccessfullyLoaded())
	{
		CRDFToolbar* pToolbar = (CRDFToolbar*)GetParent();
				
		// Adjust the toolbar button's width and height.
		long width = pCustomImage->bmpInfo->bmiHeader.biWidth;
		long height = pCustomImage->bmpInfo->bmiHeader.biHeight;
		
		if (width > m_bitmapSize.cx || height > m_bitmapSize.cy)
		{
			SetBitmapSize(CSize(width, height));

			CSize buttonSize = GetMinimalButtonSize(); // Only care about height.
	
			// Grow the toolbar if necessary.
			if (buttonSize.cy > pToolbar->GetRowHeight())
			{
				pToolbar->SetRowHeight(buttonSize.cy);
				pToolbar->LayoutButtons(-1);
				GetParentFrame()->RecalcLayout();
			}
			else
			{
				// We're too small.  Need to adjust our bitmap height.
				int diff = pToolbar->GetRowHeight() - buttonSize.cy;
				SetBitmapSize(CSize(width, height + diff));
			}
		}
	}
}


void CRDFToolbarButton::DrawLocalIcon(HDC hDC, int x, int y)
{
	if (m_Node)
		DrawLocalFileIcon(m_Node, x, y, hDC);
}

void CRDFToolbarButton::DrawButtonBitmap(HDC hDC, CRect rcImg)
{
	BTN_STATE eState = m_eState;

	if(m_eState == eBUTTON_CHECKED)
		// A checked button has same bitmap as the normal state with no mouse-over
		eState = eNORMAL;
	else if(m_eState == eBUTTON_UP && m_nChecked == 2)
		// if we are in the mouse over mode, but indeterminate we want our bitmap to have a disabled look
		eState = eDISABLED;

	UpdateIconInfo();
	if(m_hBmpImg != NULL)
	{
		// Create a scratch DC and select our bitmap into it.
		HDC pBmpDC  = ::CreateCompatibleDC(hDC);
		HPALETTE hPalette = WFE_GetUIPalette(GetParentFrame());
		CBitmap BmpImg;
		CPoint ptDst;
		HINSTANCE hInst = AfxGetResourceHandle();
		HBITMAP hBmpImg;
		hBmpImg = m_hBmpImg;
		HBITMAP hOldBmp = (HBITMAP)::SelectObject(pBmpDC, hBmpImg);
		HPALETTE hOldPal = ::SelectPalette(pBmpDC, WFE_GetUIPalette(NULL), TRUE);
		::RealizePalette(pBmpDC);
		// Get the image dimensions
		CSize sizeImg;
		BITMAP bmp;
		::GetObject(hBmpImg, sizeof(bmp), &bmp);
		sizeImg.cx = bmp.bmWidth;
		sizeImg.cy = bmp.bmHeight;

		int realBitmapHeight;
		if(m_nIconType == LOCAL_FILE)
		{
			realBitmapHeight = 16;
			m_bitmapSize.cx = 16;
		}
		else if (m_nIconType == ARBITRARY_URL)
		{
			realBitmapHeight = m_bitmapSize.cy;
		}
		else 
		{
			if (m_nBitmapID == IDB_PICTURES)
				realBitmapHeight = 21;	// Height of command buttons
			else realBitmapHeight = 17; // Height of personal toolbar button bitmaps.
		}

		// Center the image within the button	
		ptDst.x = (rcImg.Width() >= m_bitmapSize.cx) ?
			rcImg.left + (((rcImg.Width() - m_bitmapSize.cx) + 1) / 2) : rcImg.left;
		
		ptDst.y = (rcImg.Height() >= realBitmapHeight) ?
			rcImg.top + (((rcImg.Height() - realBitmapHeight) + 1) / 2) : rcImg.top;

		// If we're in the checked state, shift the image one pixel
		if (m_eState == eBUTTON_CHECKED || (m_eState == eBUTTON_UP && m_nChecked == 1))
		{
			ptDst.x += 1;
			ptDst.y += 1;
		}
	
		// Call the handy transparent blit function to paint the bitmap over
		// whatever colors exist.
		
		CPoint bitmapStart;

        if(m_bIsResourceID)
			bitmapStart = CPoint(m_nBitmapIndex * m_bitmapSize.cx, m_bEnabled ? realBitmapHeight * eState : realBitmapHeight);

		if(m_bIsResourceID)  
		{
			
			if(m_nIconType == LOCAL_FILE)
			{
				DrawLocalIcon(hDC, ptDst.x, ptDst.y);
			}
			else if (m_nIconType == ARBITRARY_URL)
			{
				DrawCustomIcon(hDC, ptDst.x, ptDst.y);
			}
			else 
				FEU_TransBlt( hDC, ptDst.x, ptDst.y, m_bitmapSize.cx, realBitmapHeight,
					pBmpDC, bitmapStart.x, bitmapStart.y ,WFE_GetUIPalette(NULL), 
					GetSysColor(COLOR_BTNFACE));   
                //::BitBlt(hDC, ptDst.x, ptDst.y, m_bitmapSize.cx, realBitmapHeight, 
				//	 pBmpDC, bitmapStart.x, bitmapStart.y, SRCCOPY);
		}
		else
		{
			CSize destSize;

			if(sizeImg.cx > sizeImg.cy)
			{
				destSize.cx = m_bitmapSize.cx;
				destSize.cy = (int)(realBitmapHeight * ((double)sizeImg.cy / sizeImg.cx));
			}
			else
			{
				destSize.cx = (int)(m_bitmapSize.cx * ((double)sizeImg.cx/ sizeImg.cy));
				destSize.cy = m_bitmapSize.cy;
			}
			StretchBlt(hDC, ptDst.x, ptDst.y, destSize.cx, destSize.cy,
						pBmpDC, 0, 0, sizeImg.cx, sizeImg.cy, SRCCOPY);
		}

		// Cleanup

		::SelectObject(pBmpDC, hOldBmp);
		::SelectPalette(pBmpDC, hOldPal, TRUE);
		::DeleteDC(pBmpDC);

	}
}
///////////////////////////////////////////////////////////////////////////
//							Class CRDFToolbarDropTarget
///////////////////////////////////////////////////////////////////////////

void CRDFToolbarDropTarget::Toolbar(CRDFToolbar *pToolbar)
{
	m_pToolbar = pToolbar;
}

DROPEFFECT CRDFToolbarDropTarget::OnDragEnter(CWnd * pWnd,	COleDataObject * pDataObject,
											   DWORD dwKeyState, CPoint point)
{
	return OnDragOver(pWnd, pDataObject, dwKeyState, point);	
}

DROPEFFECT CRDFToolbarDropTarget::OnDragOver(CWnd * pWnd, COleDataObject * pDataObject,
											  DWORD dwKeyState, CPoint point )
{
	int nStartX = 0;

	RECT buttonRect;

    int currentRow = 0;

	CRDFToolbarButton* pButton = NULL;

	for(int i = 0; i < m_pToolbar->GetNumButtons(); i++)
	{
        pButton = (CRDFToolbarButton*)(m_pToolbar->GetNthButton(i));
		pButton->GetClientRect(&buttonRect);
	    pButton->MapWindowPoints(m_pToolbar, &buttonRect);
        
		nStartX += (buttonRect.right - buttonRect.left) + SPACE_BETWEEN_BUTTONS;
        if (currentRow != pButton->GetRow())
        {
            currentRow++;
            nStartX = LEFT_TOOLBAR_MARGIN + (buttonRect.right - buttonRect.left) + SPACE_BETWEEN_BUTTONS;
        }

		if(point.x < nStartX && (point.y >= buttonRect.top && point.y <= buttonRect.bottom))
           break;

		pButton = NULL;
	}
	
	HT_Resource theNode = pButton ? pButton->GetNode() : HT_TopNode(m_pToolbar->GetHTView());
	
	m_pToolbar->SetDragButton(pButton);

	if (pButton == NULL)
		m_pToolbar->SetDragFraction(2);
	else 
	{
		// Do the whole computation of drag fraction.  Cache our drag fraction and button.
		CRect rect;
		pButton->GetClientRect(&rect);
		if (HT_IsContainer(pButton->GetNode()))
		{
			if (point.x <= rect.Width()/3)
				m_pToolbar->SetDragFraction(1);
			else if (point.x <= 2*(rect.Width()/3))
				m_pToolbar->SetDragFraction(2);
			else m_pToolbar->SetDragFraction(3);
		}
		else if (point.x <= rect.Width()/2)
			m_pToolbar->SetDragFraction(1);
		else m_pToolbar->SetDragFraction(2);
	}

	return RDFGLOBAL_TranslateDropAction(theNode, pDataObject, m_pToolbar->GetDragFraction());
}

BOOL CRDFToolbarDropTarget::OnDrop(CWnd * pWnd, COleDataObject * pDataObject,
			DROPEFFECT dropEffect, CPoint point)
{
	HT_Resource theNode = HT_TopNode(m_pToolbar->GetHTView());
	if (m_pToolbar->GetDragButton())
	  theNode = m_pToolbar->GetDragButton()->GetNode();

	RDFGLOBAL_PerformDrop(pDataObject, theNode, m_pToolbar->GetDragFraction());
	
	return TRUE;
}

// End CRDFToolbarDropTarget implementation

///////////////////////////////////////////////////////////////////////////
//							Class CRDFToolbar
///////////////////////////////////////////////////////////////////////////
#define LINKTOOLBARHEIGHT 21
#define COMMANDTOOLBARHEIGHT 42
#define SPACE_BETWEEN_ROWS 2

// The Event Handler for HT notifications on the toolbars
static void toolbarNotifyProcedure (HT_Notification ns, HT_Resource n, HT_Event whatHappened) 
{
	static int toolbarIDCounter = 0;

	CRDFToolbarHolder* theToolbarHolder = (CRDFToolbarHolder*)ns->data;
	if (theToolbarHolder == NULL)
		return;

	HT_View theView = HT_GetView(n);
	
	// The pane has to handle some events.  These will go here.
	if (whatHappened == HT_EVENT_VIEW_SELECTED)
	{
		
	}
	
	if (theView == NULL)
		return;

	if (whatHappened == HT_EVENT_VIEW_ADDED) 
	{
		CRDFToolbar* theNewToolbar = CRDFToolbar::CreateUserToolbar(theView, theToolbarHolder->GetCachedParentWindow());
		CButtonToolbarWindow *pWindow = new CButtonToolbarWindow(theNewToolbar, 
										theApp.m_pToolbarStyle, 43, 27, eSMALL_HTAB);
		
		theToolbarHolder->AddNewWindow(ID_PERSONAL_TOOLBAR+toolbarIDCounter, pWindow, toolbarIDCounter, 43, 27, 1, 
				HT_GetNodeName(HT_TopNode(theNewToolbar->GetHTView())),theApp.m_pToolbarStyle, TRUE, FALSE);
		toolbarIDCounter++;
		theToolbarHolder->GetCachedParentWindow()->RecalcLayout();
	}
	else if (whatHappened == HT_EVENT_VIEW_DELETED)
	{
		CRDFToolbar* pToolbar = (CRDFToolbar*)HT_GetViewFEData(theView);
		pToolbar->SetHTView(NULL);
		delete pToolbar;
		HT_SetViewFEData(theView, NULL);
	}
	else if (whatHappened == HT_EVENT_NODE_VPROP_CHANGED && HT_TopNode(theView) == n)
	{
	}
	else if (whatHappened == HT_EVENT_NODE_EDIT && HT_TopNode(theView) == n)
	{
		// Edit being performed on a selector bar item. (STILL TO DO)
	}
	else if (whatHappened == HT_EVENT_VIEW_WORKSPACE_REFRESH)
	{
	}
	// If the pane doesn't handle the event, then a view does.
	else 
	{
		CRDFToolbar* pToolbar = (CRDFToolbar*)HT_GetViewFEData(theView);
		if (pToolbar != NULL)
			pToolbar->HandleEvent(ns, n, whatHappened);
	}
}

void CRDFToolbar::HandleEvent(HT_Notification ns, HT_Resource n, HT_Event whatHappened)
{
	HT_View theView = m_ToolbarView;
	if (theView != NULL)
	{
		if (whatHappened == HT_EVENT_NODE_OPENCLOSE_CHANGED)
		{
			PRBool openState;
			HT_GetOpenState(n, &openState);
			if (openState)
			{
				if (n == HT_TopNode(theView))
				{
					// Initial population of the toolbar. We should only receive this event once.
					FillInToolbar();
				}
				else 
				{
					// Toolbar button menu.  Populate it.
					CRDFToolbarButton* theButton = (CRDFToolbarButton*)(HT_GetNodeFEData(n));
					theButton->FillInMenu(n);
				}
			}
		}
		else if (whatHappened == HT_EVENT_VIEW_REFRESH)
		{
			LayoutButtons(-1);
		}
		else if (HT_TopNode(theView) == HT_GetParent(n))
		{
			// Aside from the opening/closing, we only respond to events that occurred on the top node of
			// the view or the immediate children (the buttons on the toolbar).
			if (whatHappened == HT_EVENT_NODE_DELETED_DATA ||
				 whatHappened == HT_EVENT_NODE_DELETED_NODATA)
			{
				// Delete the button
				if (HT_EVENT_NODE_DELETED_DATA == whatHappened)
				{
					// Destroy the toolbar button
					CRDFToolbarButton* pButton = (CRDFToolbarButton*)HT_GetNodeFEData(n);
					if (m_hWnd)
					  RemoveButton(pButton);
					else DecrementButtonCount();
					delete pButton;

				}
			}
			else if (whatHappened == HT_EVENT_NODE_ADDED)
			{
				AddHTButton(n);
				LayoutButtons(-1);
			}
			else if (whatHappened == HT_EVENT_NODE_VPROP_CHANGED)
			{
				CRDFToolbarButton* pButton = (CRDFToolbarButton*)HT_GetNodeFEData(n);
				pButton->SetText(HT_GetNodeName(n)); // Update our name.
				LayoutButtons(-1);
			}
		}
	}
}

CRDFToolbar::CRDFToolbar(HT_View htView, int nMaxButtons, int nToolbarStyle, int nPicturesAndTextHeight, 
						 int nPicturesHeight, int nTextHeight)
	 : CNSToolbar2(nMaxButtons, nToolbarStyle, nPicturesAndTextHeight, nPicturesHeight, nTextHeight)
{
	// Set our view and point HT at us.
	m_pBackgroundImage = NULL;
	m_ToolbarView = htView;
	HT_SetViewFEData(htView, this);

	m_nNumberOfRows = 1;
	m_nRowHeight = LINKTOOLBARHEIGHT;
	void* data;
	HT_GetNodeData(HT_TopNode(GetHTView()), gNavCenter->toolbarBitmapPosition, HT_COLUMN_STRING, &data);
	if (data)
	{
		CString position((char*)data);
		if (position == "top")
			m_nRowHeight = COMMANDTOOLBARHEIGHT;
	}
}

CRDFToolbar* CRDFToolbar::CreateUserToolbar(HT_View theView, CWnd* pParent)
{
	CRDFToolbar* pToolbar = new CRDFToolbar(theView, MAX_TOOLBAR_BUTTONS, theApp.m_pToolbarStyle, 43, 27, 27);

	if (pToolbar->Create(pParent))
	{
		// Top node is already open.  Fill it in.
		PRBool openState;
		HT_Resource topNode = HT_TopNode(theView);
		HT_GetOpenState(topNode, &openState);
		if (openState)
			pToolbar->FillInToolbar();
		else HT_SetOpenState(topNode, PR_TRUE); // Let the callback kick in.
	}

	return pToolbar;
}

CRDFToolbar::~CRDFToolbar()
{
	m_ToolbarView = NULL;
}

int CRDFToolbar::Create(CWnd *pParent)
{

	int result = CNSToolbar2::Create(pParent);

	if(!result)
		return FALSE;

	m_DropTarget.Register(this);
	m_DropTarget.Toolbar(this);

	DragAcceptFiles(FALSE);

	HT_Resource topNode = HT_TopNode(GetHTView());
	BOOL fixedSize = FALSE;

	void* data;
	HT_GetNodeData(topNode, gNavCenter->toolbarButtonsFixedSize, HT_COLUMN_STRING, &data);
	if (data)
	{
		CString answer((char*)data);
		if (answer.GetLength() > 0 && (answer.GetAt(0) == 'y' || answer.GetAt(0) == 'Y'))
			fixedSize = TRUE;
	}
	SetButtonsSameWidth(fixedSize);

	return result;
}

void CRDFToolbar::FillInToolbar()
{
	if (!m_ToolbarView)
		return;

	HT_Resource top = HT_TopNode(m_ToolbarView);
	HT_Cursor cursor = HT_NewCursor(top);
	if (cursor == NULL)
		return;

	HT_Resource item = NULL;
	while (item = HT_GetNextItem(cursor))
		AddHTButton(item);

	HT_DeleteCursor(cursor);

	LayoutButtons(-1);
}

void CRDFToolbar::AddHTButton(HT_Resource item)
{
	if (HT_IsSeparator(item))
		return;

	CRDFToolbarButton* pButton = new CRDFToolbarButton;
	BOOKMARKITEM bookmark;

	XP_STRCPY(bookmark.szText, HT_GetNodeName(item));
	XP_STRCPY(bookmark.szAnchor, HT_GetNodeURL(item));

	CString tooltipText(bookmark.szText);  // Default is to use the name for the tooltip
	CString statusBarText(bookmark.szAnchor); // and the URL for the status bar text.

	// Fetch the button's tooltip and status bar text.
	void* data;
	HT_GetNodeData(item, gNavCenter->buttonTooltipText, HT_COLUMN_STRING, &data);
	if (data)
		tooltipText = (char*)data;
	HT_GetNodeData(item, gNavCenter->buttonStatusbarText, HT_COLUMN_STRING, &data);
	if (data)
		statusBarText = (char*)data;

	CString csAmpersandString = FEU_EscapeAmpersand(CString(bookmark.szText));
	pButton->Create(this, theApp.m_pToolbarStyle, CSize(60,42), CSize(85, 25), csAmpersandString,
					tooltipText, statusBarText, CSize(23,17), 
					m_nMaxToolbarButtonChars, m_nMinToolbarButtonChars, bookmark,
					item, (HT_IsContainer(item) ? TB_HAS_DRAGABLE_MENU | TB_HAS_IMMEDIATE_MENU : 0));
		
	if(HT_IsContainer(item))
	{
		CRDFToolbarButtonDropTarget *pDropTarget = new CRDFToolbarButtonDropTarget;
		pButton->SetDropTarget(pDropTarget);
	}

	HT_SetNodeFEData(item, pButton);
	CSize buttonSize = pButton->GetMinimalButtonSize(); // Only care about height.
	
	if (buttonSize.cy > m_nRowHeight)
		m_nRowHeight = buttonSize.cy;
	else if (buttonSize.cy < m_nRowHeight)
	{
		CSize size = pButton->GetBitmapSize();
		pButton->SetBitmapSize(CSize(size.cx, size.cy + (m_nRowHeight - buttonSize.cy)));
	}

	AddButtonAtIndex(pButton); // Have to put the button in the array, since the toolbar base class depends on it.

	// Update the button if it's a command.
	if (pButton->NeedsUpdate())
	{
		pButton->OnUpdateCmdUI(GetTopLevelFrame(), FALSE);
	}
}

int CRDFToolbar::GetHeight(void)
{
    return m_nNumberOfRows * (m_nRowHeight + SPACE_BETWEEN_ROWS) + SPACE_BETWEEN_ROWS;
}



void CRDFToolbar::SetMinimumRows(int rowWidth)
{
    int rowCount = 1;
    int totalLine = 0;
    int rowSpace = rowWidth - RIGHT_TOOLBAR_MARGIN - LEFT_TOOLBAR_MARGIN;

    if (rowSpace <= 0)
    {
        SetRows(rowCount);
        return;
    }

	HT_Cursor cursor = HT_NewCursor(HT_TopNode(m_ToolbarView));
	if (!cursor)
		return;
	HT_Resource item;
    while (rowCount < MAX_TOOLBAR_ROWS && (item = HT_GetNextItem(cursor)))
	{
        // Get the current button
		CRDFToolbarButton* pButton = (CRDFToolbarButton*)(HT_GetNodeFEData(item));
        if (!pButton) // Separator
			continue;

        CSize s = pButton->GetMinimalButtonSize();
        int tempTotal = totalLine + s.cx;
        if (tempTotal > rowSpace)
        {
            rowCount++;
            totalLine = s.cx;
        }
        else totalLine = tempTotal;

        totalLine += SPACE_BETWEEN_BUTTONS;
    }
	HT_DeleteCursor(cursor);

    SetRows(rowCount); 
}
    
void CRDFToolbar::ComputeLayoutInfo(CRDFToolbarButton* pButton, int numChars, int rowWidth, int& usedSpace)
{
   CString originalText = HT_GetNodeName(pButton->GetNode());

   int currCount = originalText.GetLength();
        
   // Start off at the maximal string
   CString strTxt = originalText;

   if (currCount > numChars)
   {
       strTxt = originalText.Left(numChars-3) + "...";
   }
   
   pButton->SetTextWithoutResize(strTxt);

   if (!m_bButtonsSameWidth)
	  pButton->SetButtonSize(pButton->GetButtonSizeFromChars(strTxt, numChars));

// Determine how much additional padding we'll use to fill out a row if this button doesn't fit.
    int rowUsage = usedSpace % rowWidth;
    if (rowUsage == 0)
        rowUsage = rowWidth;
    int additionalPadding = rowWidth - rowUsage;
        
    int tempTotal = rowUsage + pButton->GetButtonSize().cx;

// The button doesn't fit.  Flesh out this row and start a new one.    
    if (tempTotal > rowWidth)
      usedSpace += additionalPadding;
    
// Add this button to the row.
    usedSpace += pButton->GetButtonSize().cx + SPACE_BETWEEN_BUTTONS;

// Set this button's row information, so it knows which row it is currently residing on.
    int currentRow = usedSpace/rowWidth;
    if (usedSpace % rowWidth == 0)
        currentRow--;

    pButton->SetRow(currentRow);

}

void CRDFToolbar::LayoutButtons(int nIndex)
{
    int width = m_nWidth;

    if (width <= 0)
    {
        CRect rect;
        GetParentFrame()->GetClientRect(&rect);
        width = rect.Width();
    }
    
    int numButtonsAtMin = 0;
    int numButtonsAtMax = 0;
    int idealSpace = 0;
   
// First quickly determine what the minimum # of rows we consume is.  This is our allowed space.
    int oldRows = GetRows();

    int rowWidth = width-RIGHT_TOOLBAR_MARGIN-LEFT_TOOLBAR_MARGIN;
    
    if (rowWidth <= 0 && m_nWidth > 0)
        rowWidth = m_nWidth - RIGHT_TOOLBAR_MARGIN - LEFT_TOOLBAR_MARGIN;

    SetMinimumRows(rowWidth);

    int newRows = GetRows();

    int allowedSpace = rowWidth * GetRows(); // Toolbar width * numRows
    int usedSpace = 0;

    int numChars = 0; // Start off trying to fit the whole thing on the toolbar.
    int minChars = 0;
	
	HT_Cursor cursor = HT_NewCursor(HT_TopNode(m_ToolbarView));
	if (!cursor)
		return;
	HT_Resource item;
    while ((item = HT_GetNextItem(cursor)))
	{
        // Get the current button
		CRDFToolbarButton* pButton = (CRDFToolbarButton*)(HT_GetNodeFEData(item));
		if (!pButton)
			continue;

        if (numChars == 0)
        {
            numChars = pButton->GetMaxTextCharacters();
            minChars = pButton->GetMinTextCharacters();
        }

        // See how much this num chars takes up
        ComputeLayoutInfo(pButton, numChars, rowWidth, usedSpace);
    }
	HT_DeleteCursor(cursor);

    while (usedSpace > allowedSpace && numChars > minChars)
    {
        usedSpace = 0;
        numChars--;
        // Let's see what we can fit.
        HT_Cursor cursor = HT_NewCursor(HT_TopNode(m_ToolbarView));
		if (!cursor)
			return;
		HT_Resource item;
		while ((item = HT_GetNextItem(cursor)))
		{
			// Get the current button
			CRDFToolbarButton* pButton = (CRDFToolbarButton*)(HT_GetNodeFEData(item));
			if (!pButton)  // Separator
				continue;

            // See how much this num chars takes up
            ComputeLayoutInfo(pButton, numChars, rowWidth, usedSpace);
        }
		HT_DeleteCursor(cursor);
    }

    // That's it.  lay them out with this number of characters.
    
    int nStartX = LEFT_TOOLBAR_MARGIN;
    int nStartY = SPACE_BETWEEN_ROWS;

    int row = 0;

	CSize buttonSize;
    CString strTxt;

	cursor = HT_NewCursor(HT_TopNode(m_ToolbarView));
	if (!cursor)
		return;
	while ((item = HT_GetNextItem(cursor)))
	{
        // Get the current button
		CRDFToolbarButton* pButton = (CRDFToolbarButton*)(HT_GetNodeFEData(item));
        if (!pButton)  // Separator
			continue;

        buttonSize = pButton->GetButtonSize();  // The size we must be
        
        int tempTotal = nStartX + buttonSize.cx;
        if (tempTotal > (width - RIGHT_TOOLBAR_MARGIN))
        {
            nStartX = LEFT_TOOLBAR_MARGIN;
            nStartY += m_nRowHeight + SPACE_BETWEEN_ROWS;
        }

		if (buttonSize.cy < m_nRowHeight)
		{
			CSize size = pButton->GetBitmapSize();
			pButton->SetBitmapSize(CSize(size.cx, size.cy + (m_nRowHeight - buttonSize.cy)));
		}

		pButton->MoveWindow(nStartX, nStartY,
									  buttonSize.cx, m_nRowHeight);

		nStartX += buttonSize.cx + SPACE_BETWEEN_BUTTONS;
	}
	HT_DeleteCursor(cursor);

	//record the width of our toolbar
    //if (nStartY == SPACE_BETWEEN_ROWS && nStartX < (width - RIGHT_TOOLBAR_MARGIN))
	//  m_nWidth = nStartX;
    //else 
       m_nWidth = width;

    if (oldRows != newRows)
        GetParentFrame()->RecalcLayout();    
}

void CRDFToolbar::WidthChanged(int animWidth)
{
    CRect rect;
   
    GetParentFrame()->GetClientRect(&rect);
    int width = rect.Width() - animWidth;
    
    int numButtonsAtMin = 0;
    int numButtonsAtMax = 0;
    int idealSpace = 0;
   
// First quickly determine what the minimum # of rows we consume is.  This is our allowed space.
    int oldRows = GetRows();

    int rowWidth = width-RIGHT_TOOLBAR_MARGIN-LEFT_TOOLBAR_MARGIN;
    
    if (rowWidth <= 0 && m_nWidth > 0)
        rowWidth = m_nWidth - RIGHT_TOOLBAR_MARGIN - LEFT_TOOLBAR_MARGIN;

    SetMinimumRows(rowWidth);

    int newRows = GetRows();

    int allowedSpace = rowWidth * GetRows(); // Toolbar width * numRows
    int usedSpace = 0;

    int numChars = 0; // Start off trying to fit the whole thing on the toolbar.
    int minChars = 0;
	HT_Cursor cursor = HT_NewCursor(HT_TopNode(m_ToolbarView));
	if (!cursor)
		return;
	HT_Resource item;
    while ((item = HT_GetNextItem(cursor)))
	{
        // Get the current button
		CRDFToolbarButton* pButton = (CRDFToolbarButton*)(HT_GetNodeFEData(item));
        if (!pButton)  // Separator
			continue;

        if (numChars == 0)
        {
            numChars = pButton->GetMaxTextCharacters();
            minChars = pButton->GetMinTextCharacters();
        }

        // See how much this num chars takes up
        ComputeLayoutInfo(pButton, numChars, rowWidth, usedSpace);
    }
	HT_DeleteCursor(cursor);

    while (usedSpace > allowedSpace && numChars > minChars)
    {
        usedSpace = 0;
        numChars--;
        // Let's see what we can fit.
        HT_Cursor cursor = HT_NewCursor(HT_TopNode(m_ToolbarView));
		if (!cursor)
			return;
		HT_Resource item;
		while ((item = HT_GetNextItem(cursor)))
		{
			// Get the current button
			CRDFToolbarButton* pButton = (CRDFToolbarButton*)(HT_GetNodeFEData(item));
			if (!pButton)
				continue;

            // See how much this num chars takes up
            ComputeLayoutInfo(pButton, numChars, rowWidth, usedSpace);
        }
		HT_DeleteCursor(cursor);
    }

    // That's it.  lay them out with this number of characters.
    
    int nStartX = LEFT_TOOLBAR_MARGIN;
    int nStartY = SPACE_BETWEEN_ROWS;

    int row = 0;

	CSize buttonSize;
    CString strTxt;

	cursor = HT_NewCursor(HT_TopNode(m_ToolbarView));
	if (!cursor)
		return;
	
    while ((item = HT_GetNextItem(cursor)))
	{
        // Get the current button
		CRDFToolbarButton* pButton = (CRDFToolbarButton*)(HT_GetNodeFEData(item));
        if (!pButton)
			continue;

        buttonSize = pButton->GetButtonSize();  // The size we must be
        
        int tempTotal = nStartX + buttonSize.cx;
        if (tempTotal > (width - RIGHT_TOOLBAR_MARGIN))
        {
            nStartX = LEFT_TOOLBAR_MARGIN;
            nStartY += m_nRowHeight + SPACE_BETWEEN_ROWS;
        }

		if (buttonSize.cy < m_nRowHeight)
		{
			CSize size = pButton->GetBitmapSize();
			pButton->SetBitmapSize(CSize(size.cx, size.cy + (m_nRowHeight - buttonSize.cy)));
		}

		pButton->MoveWindow(nStartX, nStartY,
									  buttonSize.cx, m_nRowHeight);

		nStartX += buttonSize.cx + SPACE_BETWEEN_BUTTONS;
	}
	HT_DeleteCursor(cursor);


	m_nWidth = width;

    if (oldRows != newRows)
        GetParentFrame()->RecalcLayout();
}

void CRDFToolbar::OnPaint(void)
{
	CRect rcClient, updateRect, buttonRect, intersectRect;
	
	GetClientRect(&rcClient);
	GetUpdateRect(&updateRect);

	CPaintDC dcPaint(this);
	
	// background color
	HT_Resource top = HT_TopNode(GetHTView());
	void* data;
	COLORREF backgroundColor = GetSysColor(COLOR_BTNFACE);
	HT_GetNodeData(top, gNavCenter->viewBGColor, HT_COLUMN_STRING, &data);
	if (data)
	{
		WFE_ParseColor((char*)data, &backgroundColor);
	}
	SetBackgroundColor(backgroundColor);

	// Foreground color
	COLORREF foregroundColor = GetSysColor(COLOR_BTNTEXT);
	HT_GetNodeData(top, gNavCenter->viewFGColor, HT_COLUMN_STRING, &data);
	if (data)
	{
		WFE_ParseColor((char*)data, &foregroundColor);
	}
	SetForegroundColor(foregroundColor);

	// Rollover color
	COLORREF rolloverColor = RGB(0, 0, 255);
	HT_GetNodeData(top, gNavCenter->viewRolloverColor, HT_COLUMN_STRING, &data);
	if (data)
	{
		WFE_ParseColor((char*)data, &rolloverColor);
	}
	SetRolloverColor(rolloverColor);

	// Pressed color
	COLORREF pressedColor = RGB(0, 0, 128);
	HT_GetNodeData(top, gNavCenter->viewPressedColor, HT_COLUMN_STRING, &data);
	if (data)
	{
		WFE_ParseColor((char*)data, &pressedColor);
	}
	SetPressedColor(pressedColor);

	// Disabled color
	COLORREF disabledColor = -1;
	HT_GetNodeData(top, gNavCenter->viewDisabledColor, HT_COLUMN_STRING, &data);
	if (data)
	{
		WFE_ParseColor((char*)data, &disabledColor);
	}
	SetDisabledColor(disabledColor);

	// Background image URL
	CString backgroundImageURL = "";
	HT_GetNodeData(top, gNavCenter->viewBGURL, HT_COLUMN_STRING, &data);
	if (data)
		backgroundImageURL = (char*)data;
	SetBackgroundImage(NULL); // Clear out the BG image.

	HBRUSH hRegBrush = (HBRUSH) ::CreateSolidBrush(backgroundColor); 
	if (backgroundImageURL != "")
	{
		// There's a background that needs to be drawn.
		SetBackgroundImage(LookupImage(backgroundImageURL, NULL));
	}

	if (GetBackgroundImage() && 
		GetBackgroundImage()->FrameSuccessfullyLoaded())
	{
		PaintBackground(dcPaint.m_hDC, &rcClient, GetBackgroundImage(), 0);
	}
	else
	{
		::FillRect(dcPaint.m_hDC, &rcClient, hRegBrush);
	}

	for (int i = 0; i < m_nNumButtons; i++)
	{
		m_pButtonArray[i]->GetClientRect(&buttonRect);

		m_pButtonArray[i]->MapWindowPoints(this, &buttonRect);

		if(intersectRect.IntersectRect(updateRect, buttonRect))
		{
			MapWindowPoints(m_pButtonArray[i], &intersectRect);
			m_pButtonArray[i]->RedrawWindow(&intersectRect);
		}

	}

}

///////////////////////////////////////////////////////////////////////////////////
//									CRDFToolbar Messages
///////////////////////////////////////////////////////////////////////////////////

BEGIN_MESSAGE_MAP(CRDFToolbar, CNSToolbar2)
	//{{AFX_MSG_MAP(CNSToolbar2)
	ON_WM_RBUTTONDOWN()
	ON_WM_PAINT()
//}}AFX_MSG_MAP

END_MESSAGE_MAP()

void CRDFToolbar::OnRButtonDown(UINT nFlags, CPoint point)
{
	m_MenuCommandMap.Clear();
	HT_Cursor theCursor = HT_NewContextualMenuCursor(m_ToolbarView, PR_FALSE, PR_TRUE);
	CMenu menu;
	ClientToScreen(&point);
	if (menu.CreatePopupMenu() != 0 && theCursor != NULL)
	{
		// We have a cursor. Attempt to iterate
		HT_MenuCmd theCommand; 
		while (HT_NextContextMenuItem(theCursor, &theCommand))
		{
			char* menuName = HT_GetMenuCmdName(theCommand);
			if (theCommand == HT_CMD_SEPARATOR)
				menu.AppendMenu(MF_SEPARATOR);
			else
			{
				// Add the command to our command map
				CRDFMenuCommand* rdfCommand = new CRDFMenuCommand(menuName, theCommand);
				int index = m_MenuCommandMap.AddCommand(rdfCommand);
				menu.AppendMenu(MF_ENABLED, index+FIRST_HT_MENU_ID, menuName);
			}
		}
		HT_DeleteCursor(theCursor);

		menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, this, NULL);	

		menu.DestroyMenu();
	}
}

BOOL CRDFToolbar::OnCommand( WPARAM wParam, LPARAM lParam )
{
	if (wParam >= FIRST_HT_MENU_ID && wParam <= LAST_HT_MENU_ID)
	{
		// A selection was made from the context menu.
		// Use the menu map to get the HT command value
		CRDFMenuCommand* theCommand = (CRDFMenuCommand*)(m_MenuCommandMap.GetCommand((int)wParam-FIRST_HT_MENU_ID));
		if (theCommand)
		{
			HT_MenuCmd htCommand = theCommand->GetHTCommand();
			HT_DoMenuCmd(HT_GetPane(m_ToolbarView), htCommand);
		}
		return TRUE;
	}
	return((BOOL)GetParentFrame()->SendMessage(WM_COMMAND, wParam, lParam));
}

// ==========================================================
// CRDFDragToolbar
// Contains a single RDF toolbar.
// Handles drawing of backgrounds etc. etc.
// ==========================================================

BEGIN_MESSAGE_MAP(CRDFDragToolbar, CDragToolbar)
	//{{AFX_MSG_MAP(CRDFToolbarButton)
	ON_WM_PAINT()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

#define OPEN_BUTTON_WIDTH 9
#define CLOSED_BUTTON_WIDTH 40
#define CLOSED_BUTTON_HEIGHT 10
#define DT_RIGHT_MARGIN 1		//Margin between end of toolbar and right border of Navigator
#define DRAGGING_BORDER_HEIGHT 2
#define NOTOOL -1

#define HTAB_TOP_START 0
#define HTAB_TOP_HEIGHT 7
#define HTAB_MIDDLE_START 8
#define HTAB_MIDDLE_HEIGHT 6
#define HTAB_BOTTOM_START 15
#define HTAB_BOTTOM_HEIGHT 3

extern HBITMAP				m_hTabBitmap;

void CRDFDragToolbar::OnPaint(void)
{
	CPaintDC dcPaint(this);	// device context for painting
	CRect rect;

	GetClientRect(&rect);

	CDC *pDC = &dcPaint;
	HPALETTE hOldPal = ::SelectPalette(pDC->m_hDC, WFE_GetUIPalette(GetParentFrame()), FALSE);

	if(m_hTabBitmap != NULL)
	{

		// Create a scratch DC and select our bitmap into it.
		CDC * pBmpDC = new CDC;
		pBmpDC->CreateCompatibleDC(pDC);


		HBITMAP hOldBmp = (HBITMAP)::SelectObject(pBmpDC->m_hDC ,m_hTabBitmap);
		HPALETTE hOldPalette = ::SelectPalette(pBmpDC->m_hDC, WFE_GetUIPalette(NULL), TRUE);
		::RealizePalette(pBmpDC->m_hDC);
		CPoint bitmapStart(!m_bMouseInTab ? 0 : OPEN_BUTTON_WIDTH ,HTAB_TOP_START);

		//First do top of the tab

		::BitBlt(pDC->m_hDC, 0, 0, OPEN_BUTTON_WIDTH, HTAB_TOP_HEIGHT,
	  			 pBmpDC->m_hDC, bitmapStart.x, bitmapStart.y, SRCCOPY);
	
		//Now do the middle portion of the tab
		int y = HTAB_TOP_HEIGHT;

		bitmapStart.y = HTAB_MIDDLE_START;

		while(y < rect.bottom - HTAB_BOTTOM_HEIGHT)
		{
			::BitBlt(pDC->m_hDC, 0, y, OPEN_BUTTON_WIDTH, HTAB_MIDDLE_HEIGHT,
		  			 pBmpDC->m_hDC, bitmapStart.x, bitmapStart.y, SRCCOPY);

			y += HTAB_MIDDLE_HEIGHT;

		}

		// Now do the bottom of the tab
		y = rect.bottom - HTAB_BOTTOM_HEIGHT;

		bitmapStart.y = HTAB_BOTTOM_START;

		::BitBlt(pDC->m_hDC, 0, y, OPEN_BUTTON_WIDTH, HTAB_BOTTOM_HEIGHT,
	  			 pBmpDC->m_hDC, bitmapStart.x, bitmapStart.y, SRCCOPY);


		// Cleanup
		::SelectObject(pBmpDC->m_hDC, hOldBmp);
		::SelectPalette(pBmpDC->m_hDC, hOldPalette, TRUE);
		::SelectPalette(pDC->m_hDC, hOldPal, TRUE);
		pBmpDC->DeleteDC();
		delete pBmpDC;
	}

	if(m_bDragging)
	{
		CBrush brush(RGB(0, 0, 0));
		CBrush *pOldBrush = (CBrush*)dcPaint.SelectObject(&brush);

		//rect.left += OPEN_BUTTON_WIDTH;

		dcPaint.FrameRect(&rect, &brush);

		dcPaint.SelectObject(pOldBrush);
		brush.DeleteObject();
	}
}

// ==========================================================
// CRDFToolbarHolder
// The container of all the toolbars
// ==========================================================

CRDFToolbarHolder::CRDFToolbarHolder(int maxToolbars, CFrameWnd* pParentWindow)
:CCustToolbar(maxToolbars)
{
	m_pCachedParentWindow = pParentWindow;
}

CRDFToolbarHolder::~CRDFToolbarHolder()
{
	if (m_ToolbarPane)
	{
		HT_Pane oldPane = m_ToolbarPane;
		m_ToolbarPane = NULL;
		HT_DeletePane(oldPane);
	}
}

void CRDFToolbarHolder::InitializeRDFData()
{
	HT_Notification ns = new HT_NotificationStruct;
	ns->notifyProc = toolbarNotifyProcedure;
	ns->data = this;
	
	// Construct the pane and give it our notification struct
	HT_Pane newPane = HT_NewToolbarPane(ns);
	if (newPane)
	{
		SetHTPane(newPane);
		HT_SetPaneFEData(newPane, this);
	}
}
	
CIsomorphicCommandMap* CIsomorphicCommandMap::InitializeCommandMap(const CString& initType) 
{
	CIsomorphicCommandMap* result = new CIsomorphicCommandMap();

	if (initType == "Browser Commands")
	{
		// Enter the builtin browser commands into the map.
		result->AddItem("command:back", ID_NAVIGATE_BACK);
		result->AddItem("command:forward", ID_NAVIGATE_FORWARD);
		result->AddItem("command:reload", ID_NAVIGATE_RELOAD);
		result->AddItem("command:home", ID_GO_HOME);
		result->AddItem("command:print", ID_FILE_PRINT);
		result->AddItem("command:stop", ID_NAVIGATE_INTERRUPT);
	}
	else if (initType == "Command Toolbar Bitmap Indices")
	{
		result->AddItem("command:back", 0);
		result->AddItem("command:forward", 1);
		result->AddItem("command:reload", 2);
		result->AddItem("command:home", 3);
		result->AddItem("command:print", 7);
		result->AddItem("command:stop", 11);
	}
	return result;
}

void CIsomorphicCommandMap::AddItem(CString xpItem, UINT feResource)
{
	char buffer[20];
	_itoa((int)feResource, buffer, 10);
	mapFromXPToFE.SetAt(xpItem, CString(buffer));
	mapFromFEToXP.SetAt(CString(buffer), xpItem);
}

void CIsomorphicCommandMap::RemoveXPItem(CString xpItem)
{
	CString result;
	mapFromXPToFE.Lookup(xpItem, result);
	mapFromXPToFE.RemoveKey(xpItem);
	mapFromFEToXP.RemoveKey(result);
}

void CIsomorphicCommandMap::RemoveFEItem(UINT feResource)
{
	char buffer[20];
	_itoa((int)feResource, buffer, 10);
	CString resource(buffer);
	CString result;
	mapFromFEToXP.Lookup(resource, result);
	mapFromXPToFE.RemoveKey(result);
	mapFromFEToXP.RemoveKey(resource);
}

UINT CIsomorphicCommandMap::GetFEResource(CString xpItem)
{
	CString result;
	mapFromXPToFE.Lookup(xpItem, result);
	return (atoi(result));
}

CString CIsomorphicCommandMap::GetXPResource(UINT feResource)
{
	char buffer[20];
	_itoa((int)feResource, buffer, 10);
	CString resource(buffer);
	CString result;
	mapFromFEToXP.Lookup(resource, result);
	return result;
}