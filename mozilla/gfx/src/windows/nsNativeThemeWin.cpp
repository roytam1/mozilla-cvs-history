/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is 
 *   David Hyatt (hyatt@netscape.com)
 *
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
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

#include <windows.h>
#include "nsNativeThemeWin.h"
#include "nsRenderingContextWin.h"
#include "nsDeviceContextWin.h"
#include "nsRect.h"
#include "nsSize.h"
#include "nsTransform2D.h"
#include "nsThemeConstants.h"
#include "nsIPresShell.h"
#include "nsIPresContext.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsIFrame.h"
#include "nsIEventStateManager.h"
#include "nsINameSpaceManager.h"
#include "nsIPresContext.h"
#include "nsILookAndFeel.h"
#include "nsIDOMHTMLInputElement.h"
#include "nsIComponentManager.h"
#include "nsWidgetsCID.h"
#include "nsIMenuFrame.h"
#include "nsUnicharUtils.h"
#include <malloc.h>

#define THEME_COLOR 204
#define THEME_FONT  210

// Generic state constants
#define TS_NORMAL    1
#define TS_HOVER     2
#define TS_ACTIVE    3
#define TS_DISABLED  4
#define TS_FOCUSED   5

// Button constants
#define BP_BUTTON    1
#define BP_RADIO     2
#define BP_CHECKBOX  3

// Textfield constants
#define TFP_TEXTFIELD 1
#define TFS_READONLY  6

// Treeview/listbox constants
#define TREEVIEW_BODY 1

// Scrollbar constants
#define SP_BUTTON          1
#define SP_THUMBHOR        2
#define SP_THUMBVERT       3
#define SP_TRACKSTARTHOR   4
#define SP_TRACKENDHOR     5
#define SP_TRACKSTARTVERT  6
#define SP_TRACKENDVERT    7
#define SP_GRIPPERHOR      8
#define SP_GRIPPERVERT     9

// Progress bar constants
#define PP_BAR             1
#define PP_BARVERT         2
#define PP_CHUNK           3
#define PP_CHUNKVERT       4

// Tab constants
#define TABP_TAB             4
#define TABP_TAB_SELECTED    5
#define TABP_PANELS          9
#define TABP_PANEL           10

// Tooltip constants
#define TTP_STANDARD         1

// Dropdown constants
#define CBP_DROPMARKER       1

static NS_DEFINE_IID(kLookAndFeelCID, NS_LOOKANDFEEL_CID);

NS_IMPL_ISUPPORTS1(nsNativeThemeWin, nsITheme)

typedef HANDLE (WINAPI*OpenThemeDataPtr)(HWND hwnd, LPCWSTR pszClassList);
typedef HRESULT (WINAPI*CloseThemeDataPtr)(HANDLE hTheme);
typedef HRESULT (WINAPI*DrawThemeBackgroundPtr)(HANDLE hTheme, HDC hdc, int iPartId, 
                                          int iStateId, const RECT *pRect,
                                          const RECT* pClipRect);
typedef HRESULT (WINAPI*GetThemeContentRectPtr)(HANDLE hTheme, HDC hdc, int iPartId,
                                          int iStateId, const RECT* pRect,
                                          RECT* pContentRect);
typedef HRESULT (WINAPI*GetThemePartSizePtr)(HANDLE hTheme, HDC hdc, int iPartId,
                                       int iStateId, RECT* prc, int ts,
                                       SIZE* psz);
typedef HRESULT (WINAPI*GetThemeFontPtr)(HANDLE hTheme, HDC hdc, int iPartId,
                                   int iStateId, int iPropId, OUT LOGFONT* pFont);
typedef HRESULT (WINAPI*GetThemeSysFontPtr)(HANDLE hTheme, int iFontId, OUT LOGFONT* pFont);
typedef HRESULT (WINAPI*GetThemeColorPtr)(HANDLE hTheme, HDC hdc, int iPartId,
                                   int iStateId, int iPropId, OUT COLORREF* pFont);
typedef HRESULT (WINAPI*GetThemeTextMetricsPtr)(HANDLE hTheme, OPTIONAL HDC hdc, int iPartId,
                                          int iStateId, OUT TEXTMETRIC* ptm);

static OpenThemeDataPtr openTheme = NULL;
static CloseThemeDataPtr closeTheme = NULL;
static DrawThemeBackgroundPtr drawThemeBG = NULL;
static GetThemeContentRectPtr getThemeContentRect = NULL;
static GetThemePartSizePtr getThemePartSize = NULL;
static GetThemeFontPtr getThemeFont = NULL;
static GetThemeSysFontPtr getThemeSysFont = NULL;
static GetThemeColorPtr getThemeColor = NULL;
static GetThemeTextMetricsPtr getThemeTextMetrics = NULL;

static const char kThemeLibraryName[] = "uxtheme.dll";

nsNativeThemeWin::nsNativeThemeWin() {
  NS_INIT_ISUPPORTS();
  mThemeDLL = NULL;
  mButtonTheme = NULL;
  mTextFieldTheme = NULL;
  mTooltipTheme = NULL;
  mToolbarTheme = NULL;
  mRebarTheme = NULL;
  mProgressTheme = NULL;
  mScrollbarTheme = NULL;
  mStatusbarTheme = NULL;
  mTabTheme = NULL;
  mTreeViewTheme = NULL;
  mComboBoxTheme = NULL;
  mHeaderTheme = NULL;

  mThemeDLL = ::LoadLibrary(kThemeLibraryName);
  if (mThemeDLL) {
    openTheme = (OpenThemeDataPtr)GetProcAddress(mThemeDLL, "OpenThemeData");
    closeTheme = (CloseThemeDataPtr)GetProcAddress(mThemeDLL, "CloseThemeData");
    drawThemeBG = (DrawThemeBackgroundPtr)GetProcAddress(mThemeDLL, "DrawThemeBackground");
    getThemeContentRect = (GetThemeContentRectPtr)GetProcAddress(mThemeDLL, "GetThemeBackgroundContentRect");
    getThemePartSize = (GetThemePartSizePtr)GetProcAddress(mThemeDLL, "GetThemePartSize");
    getThemeSysFont = (GetThemeSysFontPtr)GetProcAddress(mThemeDLL, "GetThemeSysFont");
    getThemeColor = (GetThemeColorPtr)GetProcAddress(mThemeDLL, "GetThemeColor");

    mCheckedAtom = getter_AddRefs(NS_NewAtom("checked"));
    mInputAtom = getter_AddRefs(NS_NewAtom("input"));
    mInputCheckedAtom = getter_AddRefs(NS_NewAtom("_moz-input-checked"));
    mDisabledAtom = getter_AddRefs(NS_NewAtom("disabled"));
    mSelectedAtom = getter_AddRefs(NS_NewAtom("selected"));
    mTypeAtom = getter_AddRefs(NS_NewAtom("type"));
    mReadOnlyAtom = getter_AddRefs(NS_NewAtom("readonly"));
    mDefaultAtom = getter_AddRefs(NS_NewAtom("default"));
  }
}

nsNativeThemeWin::~nsNativeThemeWin() {
  if (!mThemeDLL)
    return;

  CloseData();
  
  if (mThemeDLL)
    ::FreeLibrary(mThemeDLL);
}

static void GetNativeRect(const nsRect& aSrc, RECT& aDst) 
{
  aDst.top = aSrc.y;
  aDst.bottom = aSrc.y + aSrc.height;
  aDst.left = aSrc.x;
  aDst.right = aSrc.x + aSrc.width;
}

HANDLE
nsNativeThemeWin::GetTheme(PRUint8 aWidgetType)
{ 
  if (!mThemeDLL)
    return NULL;

  switch (aWidgetType) {
    case NS_THEME_BUTTON:
    case NS_THEME_RADIO:
    case NS_THEME_CHECKBOX: {
      if (!mButtonTheme)
        mButtonTheme = openTheme(NULL, L"Button");
      return mButtonTheme;
    }
    case NS_THEME_TEXTFIELD:
    case NS_THEME_DROPDOWN: {
      if (!mTextFieldTheme)
        mTextFieldTheme = openTheme(NULL, L"Edit");
      return mTextFieldTheme;
    }
    case NS_THEME_TOOLTIP: {
      if (!mTooltipTheme)
        mTooltipTheme = openTheme(NULL, L"Tooltip");
      return mTooltipTheme;
    }
    case NS_THEME_TOOLBOX: {
      if (!mRebarTheme)
        mRebarTheme = openTheme(NULL, L"Rebar");
      return mRebarTheme;
    }
    case NS_THEME_TOOLBAR:
    case NS_THEME_TOOLBAR_BUTTON: {
      if (!mToolbarTheme)
        mToolbarTheme = openTheme(NULL, L"Toolbar");
      return mToolbarTheme;
    }
    case NS_THEME_PROGRESSBAR:
    case NS_THEME_PROGRESSBAR_VERTICAL:
    case NS_THEME_PROGRESSBAR_CHUNK:
    case NS_THEME_PROGRESSBAR_CHUNK_VERTICAL: {
      if (!mProgressTheme)
        mProgressTheme = openTheme(NULL, L"Progress");
      return mProgressTheme;
    }
    case NS_THEME_TAB:
    case NS_THEME_TAB_LEFT_EDGE:
    case NS_THEME_TAB_RIGHT_EDGE:
    case NS_THEME_TAB_PANEL:
    case NS_THEME_TAB_PANELS: {
      if (!mTabTheme)
        mTabTheme = openTheme(NULL, L"Tab");
      return mTabTheme;
    }
    case NS_THEME_SCROLLBAR:
    case NS_THEME_SCROLLBAR_TRACK_VERTICAL:
    case NS_THEME_SCROLLBAR_TRACK_HORIZONTAL:
    case NS_THEME_SCROLLBAR_BUTTON_UP:
    case NS_THEME_SCROLLBAR_BUTTON_DOWN:
    case NS_THEME_SCROLLBAR_BUTTON_LEFT:
    case NS_THEME_SCROLLBAR_BUTTON_RIGHT:
    case NS_THEME_SCROLLBAR_THUMB_VERTICAL:
    case NS_THEME_SCROLLBAR_THUMB_HORIZONTAL:
    case NS_THEME_SCROLLBAR_GRIPPER_VERTICAL:
    case NS_THEME_SCROLLBAR_GRIPPER_HORIZONTAL:
    {
      if (!mScrollbarTheme)
        mScrollbarTheme = openTheme(NULL, L"Scrollbar");
      return mScrollbarTheme;
    }
    case NS_THEME_STATUSBAR:
    case NS_THEME_STATUSBAR_PANEL:
    case NS_THEME_STATUSBAR_RESIZER_PANEL:
    case NS_THEME_RESIZER:
    {
      if (!mStatusbarTheme)
        mStatusbarTheme = openTheme(NULL, L"Status");
      return mStatusbarTheme;
    }
    case NS_THEME_DROPDOWN_BUTTON: {
      if (!mComboBoxTheme)
        mComboBoxTheme = openTheme(NULL, L"Combobox");
      return mComboBoxTheme;
    }
    case NS_THEME_TREEVIEW_HEADER_CELL:
    case NS_THEME_TREEVIEW_HEADER_SORTARROW: {
      if (!mHeaderTheme)
        mHeaderTheme = openTheme(NULL, L"Header");
      return mHeaderTheme;
    }
    case NS_THEME_LISTBOX:
    case NS_THEME_LISTBOX_LISTITEM:
    case NS_THEME_TREEVIEW:
    case NS_THEME_TREEVIEW_TWISTY_OPEN:
    case NS_THEME_TREEVIEW_TREEITEM: {
      if (!mTreeViewTheme)
        mTreeViewTheme = openTheme(NULL, L"Listview");
      return mTreeViewTheme;
    }
  }
  return NULL;
}

static void GetPrimaryPresShell(nsIFrame* aFrame, nsIPresShell** aResult)
{
  *aResult = nsnull;

  if (!aFrame)
    return;
 
  nsCOMPtr<nsIDocument> doc;
  nsCOMPtr<nsIContent> content;
  aFrame->GetContent(getter_AddRefs(content));
  content->GetDocument(*getter_AddRefs(doc));
  if (doc)
    doc->GetShellAt(0, aResult); // Addref happens here.
}

static PRInt32 GetContentState(nsIFrame* aFrame)
{
  if (!aFrame)
    return 0;

  nsCOMPtr<nsIPresShell> shell;
  GetPrimaryPresShell(aFrame, getter_AddRefs(shell));
  if (!shell)
    return 0;

  nsCOMPtr<nsIPresContext> context;
  shell->GetPresContext(getter_AddRefs(context));
  nsCOMPtr<nsIEventStateManager> esm;
  context->GetEventStateManager(getter_AddRefs(esm));
  PRInt32 flags = 0;
  nsCOMPtr<nsIContent> content;
  aFrame->GetContent(getter_AddRefs(content));
  esm->GetContentState(content, flags);
  return flags;
}

static PRBool CheckBooleanAttr(nsIFrame* aFrame, nsIAtom* aAtom)
{
  if (!aFrame)
    return PR_FALSE;
  nsCOMPtr<nsIContent> content;
  aFrame->GetContent(getter_AddRefs(content));
  nsAutoString attr;
  nsresult res = content->GetAttr(kNameSpaceID_None, aAtom, attr);
  if (res == NS_CONTENT_ATTR_NO_VALUE ||
      (res != NS_CONTENT_ATTR_NOT_THERE && attr.IsEmpty()))
    return PR_TRUE; // This handles the HTML case (an attr with no value is like a true val)
  return attr.Equals(NS_LITERAL_STRING("true"), // This handles the XUL case.
                     nsCaseInsensitiveStringComparator()); 
}

PRBool nsNativeThemeWin::IsDisabled(nsIFrame* aFrame)
{
  return CheckBooleanAttr(aFrame, mDisabledAtom);
}

PRBool nsNativeThemeWin::IsReadOnly(nsIFrame* aFrame)
{
  return CheckBooleanAttr(aFrame, mReadOnlyAtom);
}

PRBool nsNativeThemeWin::IsChecked(nsIFrame* aFrame)
{
  if (!aFrame)
    return NS_OK;
  nsCOMPtr<nsIContent> content;
  aFrame->GetContent(getter_AddRefs(content));
  nsAutoString checked;
  nsresult res = content->GetAttr(kNameSpaceID_None, mCheckedAtom, checked);
  if (res == NS_CONTENT_ATTR_NO_VALUE)
    return PR_TRUE; // XXXdwh Can the HTML form control's checked property differ
                    // from the checked attribute?  If so, will need an IsContentofType
                    // HTML followed by a QI to nsIDOMHTMLInputElement to grab the prop.
  return checked.Equals(NS_LITERAL_STRING("true"), // This handles the XUL case
                        nsCaseInsensitiveStringComparator()); 
}

PRBool nsNativeThemeWin::IsSelected(nsIFrame* aFrame)
{
  return CheckBooleanAttr(aFrame, mSelectedAtom);
}

nsresult 
nsNativeThemeWin::GetThemePartAndState(nsIFrame* aFrame, PRUint8 aWidgetType, 
                                       PRInt32& aPart, PRInt32& aState)
{
  switch (aWidgetType) {
    case NS_THEME_BUTTON: {
      aPart = BP_BUTTON;
      if (!aFrame) {
        aState = TS_NORMAL;
        return NS_OK;
      }

      if (IsDisabled(aFrame)) {
        aState = TS_DISABLED;
        return NS_OK;
      }
      PRInt32 eventState = GetContentState(aFrame);
      if (eventState & NS_EVENT_STATE_HOVER && eventState & NS_EVENT_STATE_ACTIVE)
        aState = TS_ACTIVE;
      else if (eventState & NS_EVENT_STATE_FOCUS)
        aState = TS_FOCUSED;
      else if (eventState & NS_EVENT_STATE_HOVER)
        aState = TS_HOVER;
      else 
        aState = TS_NORMAL;
      
      // Check for default dialog buttons.  These buttons should always look
      // focused.
      if (aState == TS_NORMAL && CheckBooleanAttr(aFrame, mDefaultAtom))
        aState = TS_FOCUSED;
      return NS_OK;
    }
    case NS_THEME_CHECKBOX:
    case NS_THEME_RADIO: {
      aPart = (aWidgetType == NS_THEME_CHECKBOX) ? BP_CHECKBOX : BP_RADIO; 

      // XXXdwh This check will need to be more complicated, since HTML radio groups
      // use checked, but XUL radio groups use selected.  There will need to be an
      // IsContentOfType test for HTML vs. XUL here.
      nsIAtom* atom = (aWidgetType == NS_THEME_CHECKBOX) ? mCheckedAtom : mSelectedAtom;

      PRBool isHTML = PR_FALSE;
      PRBool isHTMLChecked = PR_FALSE;
      
      if (!aFrame)
        aState = TS_NORMAL;
      else {
        // For XUL checkboxes and radio buttons, the state of the parent
        // determines our state.
        nsCOMPtr<nsIContent> content;
        aFrame->GetContent(getter_AddRefs(content));
        if (content->IsContentOfType(nsIContent::eXUL))
          aFrame->GetParent(&aFrame);
        else {
          // Attempt a QI.
          nsCOMPtr<nsIDOMHTMLInputElement> inputElt(do_QueryInterface(content));
          if (inputElt) {
            inputElt->GetChecked(&isHTMLChecked);
            isHTML = PR_TRUE;
          }
        }

        if (IsDisabled(aFrame))
          aState = TS_DISABLED;
        else {
          PRInt32 eventState = GetContentState(aFrame);
          if (eventState & NS_EVENT_STATE_HOVER && eventState & NS_EVENT_STATE_ACTIVE)
            aState = TS_ACTIVE;
          else if (eventState & NS_EVENT_STATE_HOVER)
            aState = TS_HOVER;
          else 
            aState = TS_NORMAL;
        }
      }

      if (isHTML) {
        if (isHTMLChecked)
          aState += 4;
      }
      else if (CheckBooleanAttr(aFrame, atom))
        aState += 4; // 4 unchecked states, 4 checked states.
      return NS_OK;
    }
    case NS_THEME_TEXTFIELD:
    case NS_THEME_DROPDOWN: {
      aPart = TFP_TEXTFIELD;
      if (!aFrame) {
        aState = TS_NORMAL;
        return NS_OK;
      }

      if (IsDisabled(aFrame)) {
        aState = TS_DISABLED;
        return NS_OK;
      }

      if (IsReadOnly(aFrame)) {
        aState = TFS_READONLY;
        return NS_OK;
      }

      PRInt32 eventState = GetContentState(aFrame);
      if (eventState & NS_EVENT_STATE_HOVER && eventState & NS_EVENT_STATE_ACTIVE)
        aState = TS_ACTIVE;
      else if (eventState & NS_EVENT_STATE_FOCUS)
        aState = TS_FOCUSED;
      else if (eventState & NS_EVENT_STATE_HOVER)
        aState = TS_HOVER;
      else 
        aState = TS_NORMAL;
      
      return NS_OK;
    }
    case NS_THEME_TOOLTIP: {
      aPart = TTP_STANDARD;
      aState = TS_NORMAL;
      return NS_OK;
    }
    case NS_THEME_PROGRESSBAR: {
      aPart = PP_BAR;
      aState = TS_NORMAL;
      return NS_OK;
    }
    case NS_THEME_PROGRESSBAR_CHUNK: {
      aPart = PP_CHUNK;
      aState = TS_NORMAL;
      return NS_OK;
    }
    case NS_THEME_PROGRESSBAR_VERTICAL: {
      aPart = PP_BARVERT;
      aState = TS_NORMAL;
      return NS_OK;
    }
    case NS_THEME_PROGRESSBAR_CHUNK_VERTICAL: {
      aPart = PP_CHUNKVERT;
      aState = TS_NORMAL;
      return NS_OK;
    }
    case NS_THEME_TOOLBAR_BUTTON: {
      aPart = BP_BUTTON;
      if (!aFrame) {
        aState = TS_NORMAL;
        return NS_OK;
      }

      if (IsDisabled(aFrame)) {
        aState = TS_DISABLED;
        return NS_OK;
      }
      PRInt32 eventState = GetContentState(aFrame);
      if (eventState & NS_EVENT_STATE_HOVER && eventState & NS_EVENT_STATE_ACTIVE)
        aState = TS_ACTIVE;
      else if (eventState & NS_EVENT_STATE_HOVER)
        aState = TS_HOVER;
      else 
        aState = TS_NORMAL;
      
      return NS_OK;
    }
    case NS_THEME_SCROLLBAR_BUTTON_UP:
    case NS_THEME_SCROLLBAR_BUTTON_DOWN:
    case NS_THEME_SCROLLBAR_BUTTON_LEFT:
    case NS_THEME_SCROLLBAR_BUTTON_RIGHT: {
      aPart = SP_BUTTON;
      aState = (aWidgetType - NS_THEME_SCROLLBAR_BUTTON_UP)*4;
      if (!aFrame)
        aState += TS_NORMAL;
      else if (IsDisabled(aFrame))
        aState += TS_DISABLED;
      else {
        PRInt32 eventState = GetContentState(aFrame);
        if (eventState & NS_EVENT_STATE_HOVER && eventState & NS_EVENT_STATE_ACTIVE)
          aState += TS_ACTIVE;
        else if (eventState & NS_EVENT_STATE_HOVER)
          aState += TS_HOVER;
        else 
          aState += TS_NORMAL;
      }
      return NS_OK;
    }
    case NS_THEME_SCROLLBAR_TRACK_HORIZONTAL:
    case NS_THEME_SCROLLBAR_TRACK_VERTICAL: {
      aPart = (aWidgetType == NS_THEME_SCROLLBAR_TRACK_HORIZONTAL) ?
              SP_TRACKSTARTHOR : SP_TRACKSTARTVERT;
      aState = TS_NORMAL;
      return NS_OK;
    }
    case NS_THEME_SCROLLBAR_THUMB_HORIZONTAL:
    case NS_THEME_SCROLLBAR_THUMB_VERTICAL: {
      aPart = (aWidgetType == NS_THEME_SCROLLBAR_THUMB_HORIZONTAL) ?
              SP_THUMBHOR : SP_THUMBVERT;
      if (!aFrame)
        aState = TS_NORMAL;
      else if (IsDisabled(aFrame))
        aState = TS_DISABLED;
      else {
        PRInt32 eventState = GetContentState(aFrame);
        if (eventState & NS_EVENT_STATE_ACTIVE) // Hover is not also a requirement for
                                                // the thumb, since the drag is not canceled
                                                // when you move outside the thumb.
          aState = TS_ACTIVE;
        else if (eventState & NS_EVENT_STATE_HOVER)
          aState = TS_HOVER;
        else 
          aState = TS_NORMAL;
      }
      return NS_OK;
    }
    case NS_THEME_SCROLLBAR_GRIPPER_VERTICAL:
    case NS_THEME_SCROLLBAR_GRIPPER_HORIZONTAL: {
      aPart = (aWidgetType == NS_THEME_SCROLLBAR_GRIPPER_HORIZONTAL) ?
              SP_GRIPPERHOR : SP_GRIPPERVERT;
      if (!aFrame)
        aState = TS_NORMAL;
      else if (IsDisabled(aFrame))
        aState = TS_DISABLED;
      else {
        // XXXdwh The gripper needs to get a hover attribute set on it, since it
        // never goes into :hover.
        PRInt32 eventState = GetContentState(aFrame);
        if (eventState & NS_EVENT_STATE_ACTIVE) // Hover is not also a requirement for
                                                // the gripper, since the drag is not canceled
                                                // when you move outside the gripper.
          aState = TS_ACTIVE;
        else if (eventState & NS_EVENT_STATE_HOVER)
          aState = TS_HOVER;
        else 
          aState = TS_NORMAL;
      }
      return NS_OK;
    }
    case NS_THEME_TOOLBOX:
    case NS_THEME_STATUSBAR:
    case NS_THEME_SCROLLBAR: {
      aPart = aState = 0;
      return NS_OK; // These have no part or state.
    }
    case NS_THEME_STATUSBAR_PANEL:
    case NS_THEME_STATUSBAR_RESIZER_PANEL:
    case NS_THEME_RESIZER: {
      aPart = (aWidgetType - NS_THEME_STATUSBAR_PANEL) + 1;
      aState = TS_NORMAL;
      return NS_OK;
    }
    case NS_THEME_TREEVIEW:
    case NS_THEME_LISTBOX: {
      aPart = TREEVIEW_BODY;
      aState = TS_NORMAL;
      return NS_OK;
    }
    case NS_THEME_TAB_PANELS: {
      aPart = TABP_PANELS;
      aState = TS_NORMAL;
      return NS_OK;
    }
    case NS_THEME_TAB_PANEL: {
      aPart = TABP_PANEL;
      aState = TS_NORMAL;
      return NS_OK;
    }
    case NS_THEME_TAB:
    case NS_THEME_TAB_LEFT_EDGE:
    case NS_THEME_TAB_RIGHT_EDGE: {
      aPart = TABP_TAB;
      if (!aFrame) {
        aState = TS_NORMAL;
        return NS_OK;
      }
      
      if (IsDisabled(aFrame)) {
        aState = TS_DISABLED;
        return NS_OK;
      }

      if (IsSelected(aFrame)) {
        aPart = TABP_TAB_SELECTED;
        aState = TS_ACTIVE; // The selected tab is always "pressed".
      }
      else {
        PRInt32 eventState = GetContentState(aFrame);
        if (eventState & NS_EVENT_STATE_HOVER && eventState & NS_EVENT_STATE_ACTIVE)
          aState = TS_ACTIVE;
        else if (eventState & NS_EVENT_STATE_FOCUS)
          aState = TS_FOCUSED;
        else if (eventState & NS_EVENT_STATE_HOVER)
          aState = TS_HOVER;
        else 
          aState = TS_NORMAL;
      }
      
      return NS_OK;
    }
    case NS_THEME_TREEVIEW_HEADER_SORTARROW: {
      // XXX Probably will never work due to a bug in the Luna theme.
      aPart = 4;
      aState = 1;
      return NS_OK;
    }
    case NS_THEME_TREEVIEW_HEADER_CELL: {
      aPart = 1;
      if (!aFrame) {
        aState = TS_NORMAL;
        return NS_OK;
      }
      
      PRInt32 eventState = GetContentState(aFrame);
      if (eventState & NS_EVENT_STATE_HOVER && eventState & NS_EVENT_STATE_ACTIVE)
        aState = TS_ACTIVE;
      else if (eventState & NS_EVENT_STATE_FOCUS)
        aState = TS_FOCUSED;
      else if (eventState & NS_EVENT_STATE_HOVER)
        aState = TS_HOVER;
      else 
        aState = TS_NORMAL;
      
      return NS_OK;
    }
    case NS_THEME_DROPDOWN_BUTTON: {
      aPart = CBP_DROPMARKER;
      if (!aFrame) {
        aState = TS_NORMAL;
        return NS_OK;
      }
      else {
        // For XUL menu lists, the state of the parent
        // determines our state.
        nsIFrame* parentFrame;
        aFrame->GetParent(&parentFrame);
        nsCOMPtr<nsIMenuFrame> menuFrame(do_QueryInterface(parentFrame));
        if (menuFrame)
          aFrame = parentFrame;
      }

      if (IsDisabled(aFrame)) {
        aState = TS_DISABLED;
        return NS_OK;
      }
      PRInt32 eventState = GetContentState(aFrame);
      if (eventState & NS_EVENT_STATE_HOVER && eventState & NS_EVENT_STATE_ACTIVE)
        aState = TS_ACTIVE;
      else if (eventState & NS_EVENT_STATE_HOVER)
        aState = TS_HOVER;
      else 
        aState = TS_NORMAL;
      return NS_OK;
    }
  }

  aPart = 0;
  aState = 0;
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsNativeThemeWin::DrawWidgetBackground(nsIRenderingContext* aContext,
                                       nsIFrame* aFrame,
                                       PRUint8 aWidgetType,
                                       const nsRect& aRect,
                                       const nsRect& aClipRect)
{
  if (!drawThemeBG)
    return NS_ERROR_FAILURE;

  HANDLE theme = GetTheme(aWidgetType);
  if (!theme)
    return NS_ERROR_FAILURE;

  PRInt32 part, state;
  nsresult rv = GetThemePartAndState(aFrame, aWidgetType, part, state);
  if (NS_FAILED(rv))
    return rv;

  nsTransform2D* transformMatrix;
  aContext->GetCurrentTransform(transformMatrix);
  RECT widgetRect;
  RECT clipRect;
	nsRect tr(aRect);
  nsRect cr(aClipRect);
	transformMatrix->TransformCoord(&tr.x,&tr.y,&tr.width,&tr.height);
  GetNativeRect(tr, widgetRect);
  transformMatrix->TransformCoord(&cr.x,&cr.y,&cr.width,&cr.height);
  GetNativeRect(cr, clipRect);
  HDC hdc = ((nsRenderingContextWin*)aContext)->mDC;
  if (!hdc)
    return NS_ERROR_FAILURE;
  
  // For left edge and right edge tabs, we need to adjust the widget
  // rects and clip rects so that the edges don't get drawn.
  if (aWidgetType == NS_THEME_TAB_LEFT_EDGE || aWidgetType == NS_THEME_TAB_RIGHT_EDGE) {
    // HACK ALERT: There appears to be no way to really obtain this value, so we're forced
    // to just use the default value for Luna (which also happens to be correct for
    // all the other skins I've tried).
    PRInt32 edgeSize = 2;
    
    // Armed with the size of the edge, we now need to either shift to the left or to the
    // right.  The clip rect won't include this extra area, so we know that we're
    // effectively shifting the edge out of view (such that it won't be painted).
    if (aWidgetType == NS_THEME_TAB_LEFT_EDGE)
      // The right edge should not be drawn.  Extend our rect by the edge size.
      widgetRect.right += edgeSize;
    else
      // The left edge should not be drawn.  Move the widget rect's left coord back.
      widgetRect.left -= edgeSize;
  }

  drawThemeBG(theme, hdc, part, state, &widgetRect, &clipRect);
  return NS_OK;
}

NS_IMETHODIMP
nsNativeThemeWin::GetWidgetBorder(nsIDeviceContext* aContext, 
                                  nsIFrame* aFrame,
                                  PRUint8 aWidgetType,
                                  nsMargin* aResult)
{
  if (!getThemeContentRect)
    return NS_ERROR_FAILURE;

  (*aResult).top = (*aResult).bottom = (*aResult).left = (*aResult).right = 0;

  if (!WidgetIsContainer(aWidgetType) ||
      aWidgetType == NS_THEME_TOOLBOX || aWidgetType == NS_THEME_TOOLBAR || 
      aWidgetType == NS_THEME_STATUSBAR || 
      aWidgetType == NS_THEME_RESIZER || aWidgetType == NS_THEME_TAB_PANEL)
    return NS_OK; // Don't worry about it.

  HANDLE theme = GetTheme(aWidgetType);
  if (!theme)
    return NS_ERROR_FAILURE;

  PRInt32 part, state;
  nsresult rv = GetThemePartAndState(aFrame, aWidgetType, part, state);
  if (NS_FAILED(rv))
    return rv;

  // Get our info.
  RECT outerRect; // Create a fake outer rect.
  outerRect.top = outerRect.left = 100;
  outerRect.right = outerRect.bottom = 200;
  RECT contentRect(outerRect);
  HRESULT res = getThemeContentRect(theme, NULL, part, state, &outerRect, &contentRect);
  
  if (FAILED(res))
    return NS_ERROR_FAILURE;

  // Now compute the delta in each direction and place it in our
  // nsMargin struct.
  aResult->top = contentRect.top - outerRect.top;
  aResult->bottom = outerRect.bottom - contentRect.bottom;
  aResult->left = contentRect.left - outerRect.left;
  aResult->right = outerRect.right - contentRect.right;

  // Remove the edges for tabs that are before or after the selected tab,
  if (aWidgetType == NS_THEME_TAB_LEFT_EDGE)
    // Remove the right edge, since we won't be drawing it.
    aResult->right = 0;
  else if (aWidgetType == NS_THEME_TAB_RIGHT_EDGE)
    // Remove the left edge, since we won't be drawing it.
    aResult->left = 0;

  if (aFrame && aWidgetType == NS_THEME_TEXTFIELD) {
    nsCOMPtr<nsIContent> content;
    aFrame->GetContent(getter_AddRefs(content));
    if (content && content->IsContentOfType(nsIContent::eHTML)) {
      // We need to pad textfields by 1 pixel, since the caret will draw
      // flush against the edge by default if we don't.
      aResult->left++;
      aResult->right++;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsNativeThemeWin::GetMinimumWidgetSize(nsIRenderingContext* aContext, nsIFrame* aFrame,
                                       PRUint8 aWidgetType,
                                       nsSize* aResult, PRBool* aIsOverridable)
{
  if (!getThemePartSize)
    return NS_ERROR_FAILURE;

  (*aResult).width = (*aResult).height = 0;
  *aIsOverridable = PR_TRUE;

  if (aWidgetType == NS_THEME_TOOLBOX || aWidgetType == NS_THEME_TOOLBAR || 
      aWidgetType == NS_THEME_STATUSBAR || aWidgetType == NS_THEME_PROGRESSBAR_CHUNK ||
      aWidgetType == NS_THEME_PROGRESSBAR_CHUNK_VERTICAL ||
      aWidgetType == NS_THEME_TAB_PANELS || aWidgetType == NS_THEME_TAB_PANEL)
    return NS_OK; // Don't worry about it.

  HANDLE theme = GetTheme(aWidgetType);
  if (!theme)
    return NS_ERROR_FAILURE;

  PRInt32 part, state;
  nsresult rv = GetThemePartAndState(aFrame, aWidgetType, part, state);
  if (NS_FAILED(rv))
    return rv;

  HDC hdc = ((nsRenderingContextWin*)aContext)->mDC;
  if (!hdc)
    return NS_ERROR_FAILURE;

  PRInt32 sizeReq = 1; // Best-fit size.
  if (aWidgetType == NS_THEME_SCROLLBAR_THUMB_VERTICAL ||
      aWidgetType == NS_THEME_SCROLLBAR_THUMB_HORIZONTAL ||
      aWidgetType == NS_THEME_PROGRESSBAR ||
      aWidgetType == NS_THEME_PROGRESSBAR_VERTICAL)
    sizeReq = 0; // Best-fit size for scrollbar thumbs and progress meters is too large for most 
                 // themes.
                 // In our app, we want these widgets to be able to really shrink down,
                 // so use the min-size request value (of 0).
  
  SIZE sz;
  getThemePartSize(theme, hdc, part, state, NULL, sizeReq, &sz);
  aResult->width = sz.cx;
  aResult->height = sz.cy;

  return NS_OK;
}

NS_IMETHODIMP
nsNativeThemeWin::WidgetStateChanged(nsIFrame* aFrame, PRUint8 aWidgetType, 
                                     nsIAtom* aAttribute, PRBool* aShouldRepaint)
{
  // Some widget types just never change state.
  if (aWidgetType == NS_THEME_TOOLBOX || aWidgetType == NS_THEME_TOOLBAR ||
      aWidgetType == NS_THEME_SCROLLBAR_TRACK_VERTICAL || 
      aWidgetType == NS_THEME_SCROLLBAR_TRACK_HORIZONTAL || 
      aWidgetType == NS_THEME_STATUSBAR || aWidgetType == NS_THEME_STATUSBAR_PANEL ||
      aWidgetType == NS_THEME_STATUSBAR_RESIZER_PANEL ||
      aWidgetType == NS_THEME_PROGRESSBAR_CHUNK ||
      aWidgetType == NS_THEME_PROGRESSBAR_CHUNK_VERTICAL ||
      aWidgetType == NS_THEME_PROGRESSBAR ||
      aWidgetType == NS_THEME_PROGRESSBAR_VERTICAL ||
      aWidgetType == NS_THEME_TOOLTIP ||
      aWidgetType == NS_THEME_TAB_PANELS ||
      aWidgetType == NS_THEME_TAB_PANEL) {
    *aShouldRepaint = PR_FALSE;
    return NS_OK;
  }

  // XXXdwh Not sure what can really be done here.  Can at least guess for
  // specific widgets that they're highly unlikely to have certain states.
  // For example, a toolbar doesn't care about any states.
  if (!aAttribute) {
    // Hover/focus/active changed.  Always repaint.
    *aShouldRepaint = PR_TRUE;
  }
  else {
    // Check the attribute to see if it's relevant.  
    // disabled, checked, dlgtype, default, etc.
    *aShouldRepaint = PR_FALSE;
    if (aAttribute == mDisabledAtom || aAttribute == mCheckedAtom ||
        aAttribute == mSelectedAtom || aAttribute == mReadOnlyAtom)
      *aShouldRepaint = PR_TRUE;
  }

  return NS_OK;
}

void
nsNativeThemeWin::CloseData()
{
  if (mToolbarTheme) {
    closeTheme(mToolbarTheme);
    mToolbarTheme = NULL;
  }
  if (mScrollbarTheme) {
    closeTheme(mScrollbarTheme);
    mScrollbarTheme = NULL;
  }
  if (mRebarTheme) {
    closeTheme(mRebarTheme);
    mRebarTheme = NULL;
  }
  if (mProgressTheme) {
    closeTheme(mProgressTheme);
    mProgressTheme = NULL;
  }
  if (mButtonTheme) {
    closeTheme(mButtonTheme);
    mButtonTheme = NULL;
  }
  if (mTextFieldTheme) {
    closeTheme(mTextFieldTheme);
    mTextFieldTheme = NULL;
  }
  if (mTooltipTheme) {
    closeTheme(mTooltipTheme);
    mTooltipTheme = NULL;
  }
  if (mStatusbarTheme) {
    closeTheme(mStatusbarTheme);
    mStatusbarTheme = NULL;
  }
  if (mTabTheme) {
    closeTheme(mTabTheme);
    mTabTheme = NULL;
  }
  if (mTreeViewTheme) {
    closeTheme(mTreeViewTheme);
    mTreeViewTheme = NULL;
  }
  if (mComboBoxTheme) {
    closeTheme(mComboBoxTheme);
    mComboBoxTheme = NULL;
  }
  if (mHeaderTheme) {
    closeTheme(mHeaderTheme);
    mHeaderTheme = NULL;
  }
}

NS_IMETHODIMP
nsNativeThemeWin::ThemeChanged()
{
  CloseData();
  return NS_OK;
}

PRBool 
nsNativeThemeWin::ThemeSupportsWidget(nsIPresContext* aPresContext,
                                      nsIFrame* aFrame,
                                      PRUint8 aWidgetType)
{
  if (aPresContext) {
    nsCOMPtr<nsIPresShell> shell;
    aPresContext->GetShell(getter_AddRefs(shell));
    if (!shell->IsThemeSupportEnabled())
      return PR_FALSE;
  }

  HANDLE theme = NULL;
  if (aWidgetType == NS_THEME_CHECKBOX_CONTAINER)
    theme = GetTheme(NS_THEME_CHECKBOX);
  else if (aWidgetType == NS_THEME_RADIO_CONTAINER)
    theme = GetTheme(NS_THEME_RADIO);
  else
    theme = GetTheme(aWidgetType);
  if (!theme)
    return PR_FALSE;
  
  // Check for specific widgets to see if HTML has overridden the style.
  if (aFrame && (aWidgetType == NS_THEME_BUTTON || aWidgetType == NS_THEME_TEXTFIELD)) {
    nsCOMPtr<nsIContent> content;
    aFrame->GetContent(getter_AddRefs(content));
    if (content->IsContentOfType(nsIContent::eHTML)) {
      nscolor bgColor;
      nscolor disabledBG;

      nsCOMPtr<nsILookAndFeel> lookAndFeel(do_CreateInstance(kLookAndFeelCID));
      
      lookAndFeel->GetColor(nsILookAndFeel::eColor_threedface, disabledBG);

      switch (aWidgetType) {
      case NS_THEME_BUTTON:
        lookAndFeel->GetColor(nsILookAndFeel::eColor_buttonface, bgColor);
        break;
      case NS_THEME_TEXTFIELD:
        lookAndFeel->GetColor(nsILookAndFeel::eColor__moz_field, bgColor);
        break;
      default:
        return PR_TRUE;
      }

      nsCOMPtr<nsIStyleContext> styleContext;
      aFrame->GetStyleContext(getter_AddRefs(styleContext));
      
      const nsStyleBackground* ourBG = (const nsStyleBackground*)styleContext->GetStyleData(eStyleStruct_Background);
      if (ourBG->mBackgroundColor != bgColor && ourBG->mBackgroundColor != disabledBG)
        return PR_FALSE;

      const nsStyleBorder* ourBorder = (const nsStyleBorder*)styleContext->GetStyleData(eStyleStruct_Border);
      //see if any sides are dotted, dashed or solid
      for (PRInt32 cnt = 0; cnt < 4; cnt++) {
        if ((ourBorder->GetBorderStyle(cnt) == NS_STYLE_BORDER_STYLE_DOTTED) || 
            (ourBorder->GetBorderStyle(cnt) == NS_STYLE_BORDER_STYLE_DASHED) ||
            (ourBorder->GetBorderStyle(cnt) == NS_STYLE_BORDER_STYLE_SOLID))
          return PR_FALSE;
      }
    }
  }

  return PR_TRUE;
}

PRBool 
nsNativeThemeWin::WidgetIsContainer(PRUint8 aWidgetType)
{
  // XXXdwh At some point flesh all of this out.
  if (aWidgetType == NS_THEME_DROPDOWN_BUTTON || 
      aWidgetType == NS_THEME_RADIO ||
      aWidgetType == NS_THEME_CHECKBOX)
    return PR_FALSE;
  return PR_TRUE;
}

///////////////////////////////////////////
// Creation Routine
///////////////////////////////////////////
NS_METHOD NS_NewNativeTheme(nsISupports *aOuter, REFNSIID aIID, void **aResult)
{
  if (aOuter)
    return NS_ERROR_NO_AGGREGATION;

  nsNativeThemeWin* theme = new nsNativeThemeWin();
  if (!theme)
    return NS_ERROR_OUT_OF_MEMORY;
  return theme->QueryInterface(aIID, aResult);
}
