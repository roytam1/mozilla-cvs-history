/* -*- Mode: C; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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
 * The Original Code is Mozilla Communicator client code,
 * released March 31, 1998.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 *     Sean Su <ssu@netscape.com>
 */

#include "extern.h"
#include "extra.h"
#include "dialogs.h"
#include "ifuncns.h"
#include "xpistub.h"
#include "xpi.h"
#include <shlobj.h>

static WNDPROC OldListBoxWndProc;
static BOOL    gbProcessingXpnstallFiles;
static DWORD   gdwACFlag;

void AskCancelDlg(HWND hDlg)
{
  char szDlgQuitTitle[MAX_BUF];
  char szDlgQuitMsg[MAX_BUF];

  if((sgProduct.dwMode != SILENT) && (sgProduct.dwMode != AUTO))
  {
    if(NS_LoadString(hSetupRscInst, IDS_DLGQUITTITLE, szDlgQuitTitle, MAX_BUF) != WIZ_OK)
      PostQuitMessage(1);
    else if(NS_LoadString(hSetupRscInst, IDS_DLGQUITMSG, szDlgQuitMsg, MAX_BUF) != WIZ_OK)
      PostQuitMessage(1);
    else if(MessageBox(hDlg, szDlgQuitMsg, szDlgQuitTitle, MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2 | MB_APPLMODAL | MB_SETFOREGROUND) == IDYES)
    {
      DestroyWindow(hDlg);
      PostQuitMessage(0);
    }
  }
} 

void PaintGradientShade(HWND hWnd, HDC hdc)
{
  RECT    rectClient;        // Rectangle for entire client area
  RECT    rectFill;          // Rectangle for filling band
  HBRUSH  brush;
  float   fStep;            // How large is each band?
  int     iOnBand;  // Loop index

  GetClientRect(hWnd, &rectClient);

  // Determine how large each band should be in order to cover the
  // client with 256 bands (one for every color intensity level)
  fStep = (float)rectClient.bottom / 256.0f;

  // Start filling bands
  for (iOnBand = 0; iOnBand < 256; iOnBand++)
  {

    // Set the location of the current band
    SetRect(&rectFill,
            0,                           // Upper left X
            (int)(iOnBand * fStep),      // Upper left Y
            rectClient.right+1,          // Lower right X
            (int)((iOnBand+1) * fStep)); // Lower right Y

    // Create a brush with the appropriate color for this band
    brush = CreateSolidBrush(RGB(0, 0, (255 - iOnBand)));

    // Fill the rectangle
    FillRect(hdc, &rectFill, brush);

    // Get rid of the brush we created
    DeleteObject(brush);
  };
}

LRESULT CALLBACK DlgProcMain(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  HDC           hdc;
  PAINTSTRUCT   ps;
  BOOL          bReturn = FALSE;

  switch(msg)
  {
    case WM_CREATE:
      hWndMain = hWnd;
      bReturn = FALSE;
      break;

    case WM_COMMAND:
      switch(LOWORD(wParam))
      {
        case IDWIZNEXT:
          DlgSequenceNext();
          bReturn = FALSE;
          break;

        case IDWIZBACK:
          DlgSequencePrev();
          bReturn = FALSE;
          break;

        default:
          bReturn = FALSE;
          break;
      }
      break;

    case WM_PAINT:
      hdc = BeginPaint(hWnd, &ps);
      // Add any drawing code here...

      PaintGradientShade(hWnd, hdc);
      OutputSetupTitle(hdc);

      EndPaint(hWnd, &ps);
      bReturn = FALSE;
      break;

    case WM_CLOSE:
      if(gbProcessingXpnstallFiles == FALSE)
        AskCancelDlg(hWnd);
      bReturn = FALSE;
      break;

    default:
      return(DefWindowProc(hWnd, msg, wParam, lParam));
  }
  return(bReturn);
}

LRESULT CALLBACK DlgProcWelcome(HWND hDlg, UINT msg, WPARAM wParam, LONG lParam)
{
  char szBuf[MAX_BUF];
  RECT rDlg;
  
  switch(msg)
  {
    case WM_INITDIALOG:
      SetWindowText(hDlg, diWelcome.szTitle);

      wsprintf(szBuf, diWelcome.szMessage0, sgProduct.szProductName);
      SetDlgItemText(hDlg, IDC_STATIC0, szBuf);
      SetDlgItemText(hDlg, IDC_STATIC1, diWelcome.szMessage1);
      SetDlgItemText(hDlg, IDC_STATIC2, diWelcome.szMessage2);

      if(GetClientRect(hDlg, &rDlg))
        SetWindowPos(hDlg, HWND_TOP, (dwScreenX/2)-(rDlg.right/2), (dwScreenY/2)-(rDlg.bottom/2), 0, 0, SWP_NOSIZE);
      break;

    case WM_COMMAND:
      switch(LOWORD(wParam))
      {
        case IDWIZNEXT:
          DestroyWindow(hDlg);
          PostMessage(hWndMain, WM_COMMAND, IDWIZNEXT, 0);
          break;

        case IDWIZBACK:
          DestroyWindow(hDlg);
          PostMessage(hWndMain, WM_COMMAND, IDWIZBACK, 0);
          break;

        case IDCANCEL:
          AskCancelDlg(hDlg);
          break;

        default:
          break;
      }
      break;
  }
  return(0);
}

LRESULT CALLBACK DlgProcLicense(HWND hDlg, UINT msg, WPARAM wParam, LONG lParam)
{
  char            szBuf[MAX_BUF];
  LPSTR           szLicenseFilenameBuf = NULL;
  WIN32_FIND_DATA wfdFindFileData;
  DWORD           dwFileSize;
  DWORD           dwBytesRead;
  HANDLE          hFLicense;
  FILE            *fLicense;
  RECT            rDlg;

  switch(msg)
  {
    case WM_INITDIALOG:
      SetWindowText(hDlg, diLicense.szTitle);
      SetDlgItemText(hDlg, IDC_MESSAGE0, diLicense.szMessage0);
      SetDlgItemText(hDlg, IDC_MESSAGE1, diLicense.szMessage1);

      lstrcpy(szBuf, szSetupDir);
      lstrcat(szBuf, "\\");
      lstrcat(szBuf, diLicense.szLicenseFilename);

      if((hFLicense = FindFirstFile(szBuf, &wfdFindFileData)) != INVALID_HANDLE_VALUE)
      {
        dwFileSize = (wfdFindFileData.nFileSizeHigh * MAXDWORD) + wfdFindFileData.nFileSizeLow;
        FindClose(hFLicense);
        if((szLicenseFilenameBuf = NS_GlobalAlloc(dwFileSize)) != NULL)
        {
          if((fLicense = fopen(szBuf, "r+b")) != NULL)
          {
            dwBytesRead = fread(szLicenseFilenameBuf, 1, dwFileSize, fLicense);
            fclose(fLicense);
            SetDlgItemText(hDlg, IDC_EDIT_LICENSE, szLicenseFilenameBuf);
          }

          FreeMemory(&szLicenseFilenameBuf);
        }
      }

      if(GetClientRect(hDlg, &rDlg))
        SetWindowPos(hDlg, HWND_TOP, (dwScreenX/2)-(rDlg.right/2), (dwScreenY/2)-(rDlg.bottom/2), 0, 0, SWP_NOSIZE);

      break;

    case WM_COMMAND:
      switch(LOWORD(wParam))
      {
        case IDWIZNEXT:
          DestroyWindow(hDlg);
          PostMessage(hWndMain, WM_COMMAND, IDWIZNEXT, 0);
          break;

        case IDWIZBACK:
          DestroyWindow(hDlg);
          PostMessage(hWndMain, WM_COMMAND, IDWIZBACK, 0);
          break;

        case IDCANCEL:
          AskCancelDlg(hDlg);
          break;

        default:
          break;
      }
      break;
  }
  return(0);
}

LRESULT CALLBACK BrowseHookProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
  DWORD dwIndex;
  DWORD dwLoop;
  RECT  rDlg;
  char  szBuf[MAX_BUF];
  char  szBufIndex[MAX_BUF];
  char  szPath[MAX_BUF];
  char  szTempPath[MAX_BUF];

  switch(message)
  {
    case WM_INITDIALOG:
      SetDlgItemText(hDlg, IDC_EDIT_DESTINATION, szTempSetupPath);

      if(GetClientRect(hDlg, &rDlg))
        SetWindowPos(hDlg, HWND_TOP, (dwScreenX/2)-(rDlg.right/2), (dwScreenY/2)-(rDlg.bottom/2), 0, 0, SWP_NOSIZE);
      break;

    case WM_COMMAND:
      switch(LOWORD(wParam))
      {
        case 1121:
          if(HIWORD(wParam) == LBN_DBLCLK)
          {
            SendDlgItemMessage(hDlg, 1121, LB_GETTEXT, 0, (LPARAM)szBuf);
            lstrcpy(szPath, szBuf);
            dwIndex = SendDlgItemMessage(hDlg, 1121, LB_GETCURSEL, 0, (LPARAM)0);
            for(dwLoop = 1; dwLoop <= dwIndex; dwLoop++)
            {
              SendDlgItemMessage(hDlg, 1121, LB_GETTEXT, dwIndex, (LPARAM)szBufIndex);
              lstrcpy(szTempPath, szPath);
              AppendBackSlash(szTempPath, sizeof(szTempPath));
              lstrcat(szTempPath, szBufIndex);
              if(FileExists(szTempPath))
              {
                AppendBackSlash(szPath, sizeof(szPath));
                lstrcpy(szPath, szTempPath);
                break;
              }
              else
              {
                SendDlgItemMessage(hDlg, 1121, LB_GETTEXT, dwLoop, (LPARAM)szBuf);
                lstrcpy(szTempPath, szPath);
                AppendBackSlash(szTempPath, sizeof(szTempPath));
                lstrcat(szTempPath, szBuf);

                if(FileExists(szTempPath))
                {
                  AppendBackSlash(szPath, sizeof(szPath));
                  lstrcpy(szPath, szTempPath);
                }
              }
            }
            SetDlgItemText(hDlg, IDC_EDIT_DESTINATION, szPath);
          }
          break;

        case IDOK:
          GetDlgItemText(hDlg, IDC_EDIT_DESTINATION, szBuf, MAX_BUF);
          if(*szBuf == '\0')
          {
            char szEDestinationPath[MAX_BUF];

            NS_LoadString(hSetupRscInst, IDS_ERROR_DESTINATION_PATH, szEDestinationPath, MAX_BUF);
            MessageBox(hDlg, szEDestinationPath, NULL, MB_OK | MB_ICONEXCLAMATION);
            break;
          }

          AppendBackSlash(szBuf, sizeof(szBuf));
          if(FileExists(szBuf) == FALSE)
          {
            char szMsgCreateDirectory[MAX_BUF];
            char szStrCreateDirectory[MAX_BUF];
            char szBufTemp[MAX_BUF];
            char szBufTemp2[MAX_BUF];

            NS_LoadString(hSetupRscInst, IDS_STR_CREATE_DIRECTORY, szStrCreateDirectory, MAX_BUF);
            if(NS_LoadString(hSetupRscInst, IDS_MSG_CREATE_DIRECTORY, szMsgCreateDirectory, MAX_BUF) == WIZ_OK)
            {
              lstrcpy(szBufTemp, "\n\n    ");
              lstrcat(szBufTemp, szBuf);
              RemoveBackSlash(szBufTemp);
              lstrcat(szBufTemp, "\n\n");
              wsprintf(szBufTemp2, szMsgCreateDirectory, szBufTemp);
            }

            if(MessageBox(hDlg, szBufTemp2, szStrCreateDirectory, MB_YESNO | MB_ICONQUESTION) == IDYES)
            {
              if(CreateDirectoriesAll(szBuf, TRUE) == FALSE)
              {
                char szECreateDirectory[MAX_BUF];

                lstrcpy(szBufTemp, "\n\n    ");
                lstrcat(szBufTemp, sgProduct.szPath);
                RemoveBackSlash(szBufTemp);
                lstrcat(szBufTemp, "\n\n");

                if(NS_LoadString(hSetupRscInst, IDS_ERROR_CREATE_DIRECTORY, szECreateDirectory, MAX_BUF) == WIZ_OK)
                  wsprintf(szBuf, szECreateDirectory, szBufTemp);

                MessageBox(hDlg, szBuf, "", MB_OK | MB_ICONERROR);
                break;
              }
              bCreateDestinationDir = TRUE;
            }
            else
            {
              break;
            }
          }

          lstrcpy(szTempSetupPath, szBuf);
          DestroyWindow(hDlg);
          break;
      }
      break;
  }
  return(0);
}

BOOL BrowseForDirectory(HWND hDlg, char *szCurrDir)
{ 
  OPENFILENAME   of;
  char           ftitle[MAX_PATH];
  char           fname[MAX_PATH];
  char           szCDir[MAX_BUF];
  char           szBuf[MAX_BUF];
  char           szSearchPathBuf[MAX_BUF];
  char           szDlgBrowseTitle[MAX_BUF];
  BOOL           bRet;

  /* save the current directory */
  GetCurrentDirectory(MAX_BUF, szCDir);

  ZeroMemory(szDlgBrowseTitle, sizeof(szDlgBrowseTitle));
  NS_LoadString(hSetupRscInst, IDS_DLGBROWSETITLE, szDlgBrowseTitle, MAX_BUF);

  lstrcpy(szSearchPathBuf, szCurrDir);
  RemoveBackSlash(szSearchPathBuf);
  while(FileExists(szSearchPathBuf) == FALSE)
  {
    RemoveBackSlash(szSearchPathBuf);
    ParsePath(szSearchPathBuf, szBuf, sizeof(szBuf), PP_PATH_ONLY);
    lstrcpy(szSearchPathBuf, szBuf);
  }

  ZeroMemory(ftitle, sizeof(ftitle));
  strcpy(fname, "*.*");
  of.lStructSize        = sizeof(OPENFILENAME);
  of.hwndOwner          = hDlg;
  of.hInstance          = hSetupRscInst;
  of.lpstrFilter        = NULL;
  of.lpstrCustomFilter  = NULL;
  of.nMaxCustFilter     = 0;
  of.nFilterIndex       = 0;
  of.lpstrFile          = fname;
  of.nMaxFile           = MAX_PATH;
  of.lpstrFileTitle     = ftitle;
  of.nMaxFileTitle      = MAX_PATH;
  of.lpstrInitialDir    = szSearchPathBuf;
  of.lpstrTitle         = szDlgBrowseTitle;
  of.Flags              = OFN_NONETWORKBUTTON |
                          OFN_ENABLEHOOK      |
                          OFN_NOCHANGEDIR  |
                          OFN_ENABLETEMPLATE;
  of.nFileOffset        = 0;
  of.nFileExtension     = 0;
  of.lpstrDefExt        = NULL;
  of.lCustData          = 0;
  of.lpfnHook           = BrowseHookProc;
  of.lpTemplateName     = MAKEINTRESOURCE(DLG_BROWSE_DIR);

  if(GetOpenFileName(&of))
    bRet = TRUE;
  else
    bRet = FALSE;

  /* restore the current directory */
  SetCurrentDirectory(szCDir);
  return(bRet);
}

LRESULT CALLBACK DlgProcSetupType(HWND hDlg, UINT msg, WPARAM wParam, LONG lParam)
{
  HWND          hRadioSt0;
  HWND          hStaticSt0;
  HWND          hRadioSt1;
  HWND          hStaticSt1;
  HWND          hRadioSt2;
  HWND          hStaticSt2;
  HWND          hRadioSt3;
  HWND          hStaticSt3;
  HWND          hReadme;
  RECT          rDlg;
  char          szBuf[MAX_BUF];
  char          szBufTemp[MAX_BUF];
  char          szBufTemp2[MAX_BUF];

  hRadioSt0   = GetDlgItem(hDlg, IDC_RADIO_ST0);
  hStaticSt0  = GetDlgItem(hDlg, IDC_STATIC_ST0_DESCRIPTION);
  hRadioSt1   = GetDlgItem(hDlg, IDC_RADIO_ST1);
  hStaticSt1  = GetDlgItem(hDlg, IDC_STATIC_ST1_DESCRIPTION);
  hRadioSt2   = GetDlgItem(hDlg, IDC_RADIO_ST2);
  hStaticSt2  = GetDlgItem(hDlg, IDC_STATIC_ST2_DESCRIPTION);
  hRadioSt3   = GetDlgItem(hDlg, IDC_RADIO_ST3);
  hStaticSt3  = GetDlgItem(hDlg, IDC_STATIC_ST3_DESCRIPTION);
  hReadme     = GetDlgItem(hDlg, IDC_README);

  switch(msg)
  {
    case WM_INITDIALOG:
      SetWindowText(hDlg, diSetupType.szTitle);

      SetDlgItemText(hDlg, IDC_EDIT_DESTINATION, szTempSetupPath);
      SetDlgItemText(hDlg, IDC_STATIC_MSG0, diSetupType.szMessage0);

      if(diSetupType.stSetupType0.bVisible)
      {
        SetDlgItemText(hDlg, IDC_RADIO_ST0, diSetupType.stSetupType0.szDescriptionShort);
        SetDlgItemText(hDlg, IDC_STATIC_ST0_DESCRIPTION, diSetupType.stSetupType0.szDescriptionLong);
        ShowWindow(hRadioSt0, SW_SHOW);
        ShowWindow(hStaticSt0, SW_SHOW);
      }
      else
      {
        ShowWindow(hRadioSt0, SW_HIDE);
        ShowWindow(hStaticSt0, SW_HIDE);
      }

      if(diSetupType.stSetupType1.bVisible)
      {
        SetDlgItemText(hDlg, IDC_RADIO_ST1, diSetupType.stSetupType1.szDescriptionShort);
        SetDlgItemText(hDlg, IDC_STATIC_ST1_DESCRIPTION, diSetupType.stSetupType1.szDescriptionLong);
        ShowWindow(hRadioSt1, SW_SHOW);
        ShowWindow(hStaticSt1, SW_SHOW);
      }
      else
      {
        ShowWindow(hRadioSt1, SW_HIDE);
        ShowWindow(hStaticSt1, SW_HIDE);
      }

      if(diSetupType.stSetupType2.bVisible)
      {
        SetDlgItemText(hDlg, IDC_RADIO_ST2, diSetupType.stSetupType2.szDescriptionShort);
        SetDlgItemText(hDlg, IDC_STATIC_ST2_DESCRIPTION, diSetupType.stSetupType2.szDescriptionLong);
        ShowWindow(hRadioSt2, SW_SHOW);
        ShowWindow(hStaticSt2, SW_SHOW);
      }
      else
      {
        ShowWindow(hRadioSt2, SW_HIDE);
        ShowWindow(hStaticSt2, SW_HIDE);
      }

      if(diSetupType.stSetupType3.bVisible)
      {
        SetDlgItemText(hDlg, IDC_RADIO_ST3, diSetupType.stSetupType3.szDescriptionShort);
        SetDlgItemText(hDlg, IDC_STATIC_ST3_DESCRIPTION, diSetupType.stSetupType3.szDescriptionLong);
        ShowWindow(hRadioSt3, SW_SHOW);
        ShowWindow(hStaticSt3, SW_SHOW);
      }
      else
      {
        ShowWindow(hRadioSt3, SW_HIDE);
        ShowWindow(hStaticSt3, SW_HIDE);
      }

      /* enable the appropriate radio button */
      switch(dwTempSetupType)
      {
        case ST_RADIO0:
          CheckDlgButton(hDlg, IDC_RADIO_ST0, BST_CHECKED);
          SetFocus(hRadioSt0);
          break;

        case ST_RADIO1:
          CheckDlgButton(hDlg, IDC_RADIO_ST1, BST_CHECKED);
          SetFocus(hRadioSt1);
          break;

        case ST_RADIO2:
          CheckDlgButton(hDlg, IDC_RADIO_ST2, BST_CHECKED);
          SetFocus(hRadioSt2);
          break;

        case ST_RADIO3:
          CheckDlgButton(hDlg, IDC_RADIO_ST3, BST_CHECKED);
          SetFocus(hRadioSt3);
          break;
      }

      if(GetClientRect(hDlg, &rDlg))
        SetWindowPos(hDlg, HWND_TOP, (dwScreenX/2)-(rDlg.right/2), (dwScreenY/2)-(rDlg.bottom/2), 0, 0, SWP_NOSIZE);

      if((*diSetupType.szReadmeFilename == '\0') || (FileExists(diSetupType.szReadmeFilename) == FALSE))
        ShowWindow(hReadme, SW_HIDE);
      else
        ShowWindow(hReadme, SW_SHOW);

      break;

    case WM_COMMAND:
      switch(LOWORD(wParam))
      {
/*
        BROWSEINFO    biBrowseInfo;
        LPITEMIDLIST  lppidlPath;
        int           iImageId = 0;
        char          szDisplayName[MAX_BUF];
*/
        case IDC_BUTTON_BROWSE:
          if(IsDlgButtonChecked(hDlg, IDC_RADIO_ST0)      == BST_CHECKED)
            dwTempSetupType = ST_RADIO0;
          else if(IsDlgButtonChecked(hDlg, IDC_RADIO_ST1) == BST_CHECKED)
            dwTempSetupType = ST_RADIO1;
          else if(IsDlgButtonChecked(hDlg, IDC_RADIO_ST2) == BST_CHECKED)
            dwTempSetupType = ST_RADIO2;
          else if(IsDlgButtonChecked(hDlg, IDC_RADIO_ST3) == BST_CHECKED)
            dwTempSetupType = ST_RADIO3;

/*
          biBrowseInfo.hwndOwner        = hDlg;
          biBrowseInfo.pidlRoot         = NULL;
          biBrowseInfo.pszDisplayName   = szDisplayName;
          biBrowseInfo.lpszTitle        = "Title of BrowseForFolder()";
          biBrowseInfo.ulFlags          = BIF_EDITBOX |
                                          BIF_STATUSTEXT |
                                          BIF_DONTGOBELOWDOMAIN;
          biBrowseInfo.lpfn             = NULL;
          biBrowseInfo.lParam           = 0;
          biBrowseInfo.iImage           = iImageId;

          lppidlPath = SHBrowseForFolder(&biBrowseInfo);
*/

          GetDlgItemText(hDlg, IDC_EDIT_DESTINATION, szTempSetupPath, MAX_PATH);
          BrowseForDirectory(hDlg, szTempSetupPath);

          /* fix: hack to work around bug with this dlg proc no longer being called
             after returning from BrowseForDirectory() */
          DestroyWindow(hDlg);
          dwWizardState = DLG_LICENSE;
          DlgSequenceNext();
          break;

        case IDC_README:
          if(*diSetupType.szReadmeApp == '\0')
            WinSpawn(diSetupType.szReadmeFilename, NULL, szSetupDir, SW_SHOWNORMAL, FALSE);
          else
            WinSpawn(diSetupType.szReadmeApp, diSetupType.szReadmeFilename, szSetupDir, SW_SHOWNORMAL, FALSE);
          break;

        case IDWIZNEXT:
          GetDlgItemText(hDlg, IDC_EDIT_DESTINATION, szTempSetupPath, MAX_PATH);
          lstrcpy(sgProduct.szPath, szTempSetupPath);

          /* check for legacy file.  We're trying not to install over an old incompatible version */
          if(CheckLegacy(hDlg) == TRUE)
          {
            SetForegroundWindow(hDlg);
            break;
          }

          /* append a backslash to the path because CreateDirectoriesAll()
             uses a backslash to determine directories */
          lstrcpy(szBuf, sgProduct.szPath);
          AppendBackSlash(szBuf, sizeof(szBuf));

          if(FileExists(szBuf) == FALSE)
          {
            char szMsgCreateDirectory[MAX_BUF];
            char szStrCreateDirectory[MAX_BUF];

            NS_LoadString(hSetupRscInst, IDS_STR_CREATE_DIRECTORY, szStrCreateDirectory, MAX_BUF);
            if(NS_LoadString(hSetupRscInst, IDS_MSG_CREATE_DIRECTORY, szMsgCreateDirectory, MAX_BUF) == WIZ_OK)
            {
              lstrcpy(szBufTemp, "\n\n    ");
              lstrcat(szBufTemp, szBuf);
              RemoveBackSlash(szBufTemp);
              lstrcat(szBufTemp, "\n\n");
              wsprintf(szBufTemp2, szMsgCreateDirectory, szBufTemp);
            }

            if(MessageBox(hDlg, szBufTemp2, szStrCreateDirectory, MB_YESNO | MB_ICONQUESTION) == IDYES)
            {
              if(CreateDirectoriesAll(szBuf, TRUE) == FALSE)
              {
                char szECreateDirectory[MAX_BUF];

                lstrcpy(szBufTemp, "\n\n    ");
                lstrcat(szBufTemp, sgProduct.szPath);
                RemoveBackSlash(szBufTemp);
                lstrcat(szBufTemp, "\n\n");

                if(NS_LoadString(hSetupRscInst, IDS_ERROR_CREATE_DIRECTORY, szECreateDirectory, MAX_BUF) == WIZ_OK)
                  wsprintf(szBuf, szECreateDirectory, szBufTemp);

                MessageBox(hDlg, szBuf, "", MB_OK | MB_ICONERROR);
                break;
              }
              bCreateDestinationDir = TRUE;
            }
            else
            {
              break;
            }
          }

          /* retrieve and save the state of the selected radio button */
          if(IsDlgButtonChecked(hDlg, IDC_RADIO_ST0)      == BST_CHECKED)
          {
            dwSetupType     = ST_RADIO0;
            dwTempSetupType = dwSetupType;
            SiCNodeSetItemsSelected(diSetupType.stSetupType0.dwCItems, diSetupType.stSetupType0.dwCItemsSelected);
          }
          else if(IsDlgButtonChecked(hDlg, IDC_RADIO_ST1) == BST_CHECKED)
          {
            dwSetupType     = ST_RADIO1;
            dwTempSetupType = dwSetupType;
            SiCNodeSetItemsSelected(diSetupType.stSetupType1.dwCItems, diSetupType.stSetupType1.dwCItemsSelected);
          }
          else if(IsDlgButtonChecked(hDlg, IDC_RADIO_ST2) == BST_CHECKED)
          {
            dwSetupType     = ST_RADIO2;
            dwTempSetupType = dwSetupType;
            SiCNodeSetItemsSelected(diSetupType.stSetupType2.dwCItems, diSetupType.stSetupType2.dwCItemsSelected);
          }
          else if(IsDlgButtonChecked(hDlg, IDC_RADIO_ST3) == BST_CHECKED)
          {
            dwSetupType     = ST_RADIO3;
            dwTempSetupType = dwSetupType;
            SiCNodeSetItemsSelected(diSetupType.stSetupType3.dwCItems, diSetupType.stSetupType3.dwCItemsSelected);
          }

          /* set the next dialog to be shown depending on the 
             what the user selected */
          dwWizardState = DLG_SETUP_TYPE;
          CheckWizardStateCustom(DLG_WINDOWS_INTEGRATION);

          DestroyWindow(hDlg);
          PostMessage(hWndMain, WM_COMMAND, IDWIZNEXT, 0);
          break;

        case IDWIZBACK:
          dwTempSetupType = dwSetupType;
          lstrcpy(szTempSetupPath, sgProduct.szPath);
          DestroyWindow(hDlg);
          PostMessage(hWndMain, WM_COMMAND, IDWIZBACK, 0);
          break;

        case IDCANCEL:
          AskCancelDlg(hDlg);
          break;

        default:
          break;
      }
      break;
  }
  return(0);
}

void DrawCheck(LPDRAWITEMSTRUCT lpdis, DWORD dwACFlag)
{
  siC     *siCTemp  = NULL;
  HDC     hdcMem;
  HBITMAP hbmpCheckBox;

  siCTemp = SiCNodeGetObject(lpdis->itemID, FALSE, dwACFlag);
  if(siCTemp != NULL)
  {
    if(siCTemp->dwAttributes & SIC_DISABLED)
      hbmpCheckBox = hbmpBoxCheckedDisabled;
    else if(siCTemp->dwAttributes & SIC_SELECTED)
      hbmpCheckBox = hbmpBoxChecked;
    else
      hbmpCheckBox = hbmpBoxUnChecked;

    SendMessage(lpdis->hwndItem, LB_SETITEMDATA, lpdis->itemID, (LPARAM)hbmpCheckBox);
    if((hdcMem = CreateCompatibleDC(lpdis->hDC)) != NULL)
    {
      SelectObject(hdcMem, hbmpCheckBox);

      // BitBlt() is used to prepare the checkbox icon into the list box item's device context.
      // The SendMessage() function using LB_SETITEMDATA performs the drawing.
      BitBlt(lpdis->hDC,
             lpdis->rcItem.left + 2,
             lpdis->rcItem.top + 2,
             lpdis->rcItem.right - lpdis->rcItem.left,
             lpdis->rcItem.bottom - lpdis->rcItem.top,
             hdcMem,
             0,
             0,
             SRCCOPY);

      DeleteDC(hdcMem);
    }
  }
}

void lbAddItem(HWND hList, siC *siCComponent)
{
  DWORD dwItem;

  dwItem = SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)siCComponent->szDescriptionShort);
  if(siCComponent->dwAttributes & SIC_DISABLED)
    SendMessage(hList, LB_SETITEMDATA, dwItem, (LPARAM)hbmpBoxCheckedDisabled);
  else if(siCComponent->dwAttributes & SIC_SELECTED)
    SendMessage(hList, LB_SETITEMDATA, dwItem, (LPARAM)hbmpBoxChecked);
  else
    SendMessage(hList, LB_SETITEMDATA, dwItem, (LPARAM)hbmpBoxUnChecked);
} 

void InvalidateLBCheckbox(HWND hwndListBox)
{
  RECT rcCheckArea;

  // retrieve the rectangle of all list items to update.
  GetClientRect(hwndListBox, &rcCheckArea);

  // set the right coordinate of the rectangle to be the same
  // as the right edge of the bitmap drawn.
  rcCheckArea.right = CX_CHECKBOX;

  // It then invalidates the checkbox region to be redrawn.
  // Invalidating the region sends a WM_DRAWITEM message to
  // the dialog, which redraws the region given the
  // node attirbute, in this case it is a bitmap of a
  // checked/unchecked checkbox.
  InvalidateRect(hwndListBox, &rcCheckArea, TRUE);
}
  
void SunJavaDependencyHack(DWORD dwIndex, BOOL bSelected, DWORD dwACFlag)
{
  DWORD dwPatchIndex;
  siC   *siCTemp = NULL;

  siCTemp = SiCNodeGetObject(dwIndex, FALSE, dwACFlag);
  if(lstrcmpi("Sun Java 2", siCTemp->szDescriptionShort) == 0)
  {
    if((dwPatchIndex = SiCNodeGetIndexDS("Sun Java 2 Patch")) != -1)
      SiCNodeSetAttributes(dwPatchIndex, SIC_SELECTED, bSelected, TRUE, AC_ALL);
  }
}

void ToggleCheck(HWND hwndListBox, DWORD dwIndex, DWORD dwACFlag)
{
  BOOL  bMoreToResolve;
  LPSTR szToggledDescriptionShort = NULL;
  DWORD dwAttributes;

  // Checks to see if the checkbox is checked or not checked, and
  // toggles the node attributes appropriately.
  dwAttributes = SiCNodeGetAttributes(dwIndex, FALSE, dwACFlag);
  if(!(dwAttributes & SIC_DISABLED))
  {
    if(dwAttributes & SIC_SELECTED)
    {
      SiCNodeSetAttributes(dwIndex, SIC_SELECTED, FALSE, FALSE, dwACFlag);
      szToggledDescriptionShort = SiCNodeGetDescriptionShort(dwIndex, FALSE, dwACFlag);
      ResolveDependees(szToggledDescriptionShort);
      SunJavaDependencyHack(dwIndex, FALSE, dwACFlag);
    }
    else
    {
      SiCNodeSetAttributes(dwIndex, SIC_SELECTED, TRUE, FALSE, dwACFlag);
      bMoreToResolve = ResolveDependencies(dwIndex);

      while(bMoreToResolve)
        bMoreToResolve = ResolveDependencies(-1);

      szToggledDescriptionShort = SiCNodeGetDescriptionShort(dwIndex, FALSE, dwACFlag);
      ResolveDependees(szToggledDescriptionShort);
      SunJavaDependencyHack(dwIndex, TRUE, dwACFlag);
    }

    InvalidateLBCheckbox(hwndListBox);
  }
}

// ************************************************************************
// FUNCTION : SubclassWindow( HWND, WNDPROC )
// PURPOSE  : Subclasses a window procedure
// COMMENTS : Returns the old window procedure
// ************************************************************************
WNDPROC SubclassWindow( HWND hWnd, WNDPROC NewWndProc)
{
  WNDPROC OldWndProc;

  OldWndProc = (WNDPROC)GetWindowLong(hWnd, GWL_WNDPROC);
  SetWindowLong(hWnd, GWL_WNDPROC, (LONG) NewWndProc);

  return OldWndProc;
}

// ************************************************************************
// FUNCTION : NewListBoxWndProc( HWND, UINT, WPARAM, LPARAM )
// PURPOSE  : Processes messages for "LISTBOX" class.
// COMMENTS : Prevents the user from moving the window
//            by dragging the titlebar.
// ************************************************************************
LRESULT CALLBACK NewListBoxWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  DWORD               dwPosX;
  DWORD               dwPosY;
  DWORD               dwIndex;
#ifdef XXX_SSU_DISABLED_FOR_NOW
  LPDRAWITEMSTRUCT    lpdis;
  PAINTSTRUCT         ps;
  HDC                 hdc;
#endif

  switch(uMsg)
  {
#ifdef XXX_SSU_DISABLED_FOR_NOW
    case WM_DRAWITEM:
      lpdis = (LPDRAWITEMSTRUCT)lParam;

      break;

    case WM_PAINT:
      hdc = BeginPaint(hWnd, &ps);
      // Add any drawing code here...

//      PaintGradientShade(hWnd, hdc);
//      OutputSetupTitle(hdc);

      EndPaint(hWnd, &ps);
//      bReturn = FALSE;
      break;
#endif

    case WM_CHAR:
      /* check for the space key */
      if((TCHAR)wParam == 32)
      {
        dwIndex = SendMessage(hWnd,
                              LB_GETCURSEL,
                              0,
                              0);
        ToggleCheck(hWnd, dwIndex, gdwACFlag);
      }
      break;

    case WM_LBUTTONDOWN:
      if(wParam == MK_LBUTTON)
      {
        dwPosX = LOWORD(lParam); // x pos
        dwPosY = HIWORD(lParam); // y pos

        if((dwPosX > 1) && (dwPosX <= CX_CHECKBOX))
        {
          dwIndex = LOWORD(SendMessage(hWnd,
                                       LB_ITEMFROMPOINT,
                                       0,
                                       (LPARAM)MAKELPARAM(dwPosX, dwPosY)));
          ToggleCheck(hWnd, dwIndex, gdwACFlag);
        }
      }
      break;
  }

  return(CallWindowProc(OldListBoxWndProc, hWnd, uMsg, wParam, lParam));
}

LRESULT CALLBACK DlgProcSelectComponents(HWND hDlg, UINT msg, WPARAM wParam, LONG lParam)
{
  BOOL                bReturn = FALSE;
  siC                 *siCTemp;
  DWORD               dwIndex;
  DWORD               dwItems = MAX_BUF;
  HWND                hwndLBComponents;
  RECT                rDlg;
  TCHAR               tchBuffer[MAX_BUF];
  TEXTMETRIC          tm;
  DWORD               y;
  LPDRAWITEMSTRUCT    lpdis;
  ULONGLONG           ullDSBuf;
  char                szBuf[MAX_BUF];

#ifdef XXX_SSU_COMPONENT_SIZE
  RECT                rTemp;
  HDC                 hdcComponentSize;
  RECT                rLBComponentSize;
  RECT                rListBox;
#endif

  hwndLBComponents  = GetDlgItem(hDlg, IDC_LIST_COMPONENTS);

  switch(msg)
  {
    case WM_INITDIALOG:
      SetWindowText(hDlg, diSelectComponents.szTitle);
      SetDlgItemText(hDlg, IDC_MESSAGE0, diSelectComponents.szMessage0);

      siCTemp = siComponents;
      if(siCTemp != NULL)
      {
        if((!(siCTemp->dwAttributes & SIC_INVISIBLE)) && (!(siCTemp->dwAttributes & SIC_ADDITIONAL)))
          lbAddItem(hwndLBComponents, siCTemp);

        siCTemp = siCTemp->Next;
        while((siCTemp != siComponents) && (siCTemp != NULL))
        {
          if((!(siCTemp->dwAttributes & SIC_INVISIBLE)) && (!(siCTemp->dwAttributes & SIC_ADDITIONAL)))
            lbAddItem(hwndLBComponents, siCTemp);

          siCTemp = siCTemp->Next;
        }
        SetFocus(hwndLBComponents);
        SendMessage(hwndLBComponents, LB_SETCURSEL, 0, 0);
        SetDlgItemText(hDlg, IDC_STATIC_DESCRIPTION, SiCNodeGetDescriptionLong(0, FALSE, AC_COMPONENTS));
      }

      if(GetClientRect(hDlg, &rDlg))
        SetWindowPos(hDlg, HWND_TOP, (dwScreenX/2)-(rDlg.right/2), (dwScreenY/2)-(rDlg.bottom/2), 0, 0, SWP_NOSIZE);

      /* update the disk space available info in the dialog.  GetDiskSpaceAvailable()
         returns value in kbytes */
      ullDSBuf = GetDiskSpaceAvailable(sgProduct.szPath);
      _ui64toa(ullDSBuf, tchBuffer, 10);
      ParsePath(sgProduct.szPath, szBuf, sizeof(szBuf), PP_ROOT_ONLY);
      RemoveBackSlash(szBuf);
      lstrcat(szBuf, " - ");
      lstrcat(szBuf, tchBuffer);
      lstrcat(szBuf, " K");
      SetDlgItemText(hDlg, IDC_SPACE_AVAILABLE, szBuf);

      gdwACFlag = AC_COMPONENTS;
      OldListBoxWndProc = SubclassWindow(hwndLBComponents, (WNDPROC)NewListBoxWndProc);
      break;

    case WM_DRAWITEM:
      lpdis = (LPDRAWITEMSTRUCT)lParam;

      // If there are no list box items, skip this message.
      if(lpdis->itemID == -1)
        break;

      SendMessage(lpdis->hwndItem, LB_GETTEXT, lpdis->itemID, (LPARAM)tchBuffer);
#ifdef XXX_SSU_COMPONENT_SIZE
      GetClientRect(lpdis->hwndItem, &rTemp);
      hdcComponentSize = GetDC(lpdis->hwndItem);
      SelectObject(hdcComponentSize, GetCurrentObject(lpdis->hDC, OBJ_FONT));
#endif

      if((lpdis->itemAction & ODA_FOCUS) && (lpdis->itemState & ODS_SELECTED))
      {
        // remove the focus rect on the previous selected item
        DrawFocusRect(lpdis->hDC, &(lpdis->rcItem));
      }

      if(lpdis->itemAction & ODA_FOCUS)
      {
        siCTemp = SiCNodeGetObject(lpdis->itemID, FALSE, AC_COMPONENTS);

        if((lpdis->itemState & ODS_SELECTED) &&
          !(lpdis->itemState & ODS_FOCUS))
        {
          if(siCTemp->dwAttributes & SIC_DISABLED)
            SetTextColor(lpdis->hDC,        GetSysColor(COLOR_GRAYTEXT));
          else
          {
            SetTextColor(lpdis->hDC,        GetSysColor(COLOR_WINDOWTEXT));
            SetBkColor(lpdis->hDC,          GetSysColor(COLOR_WINDOW));

#ifdef XXX_SSU_COMPONENT_SIZE
            SetTextColor(hdcComponentSize,  GetSysColor(COLOR_WINDOWTEXT));
            SetBkColor(hdcComponentSize,    GetSysColor(COLOR_WINDOW));
#endif
          }
        }
        else
        {
          if(siCTemp->dwAttributes & SIC_DISABLED)
            SetTextColor(lpdis->hDC,        GetSysColor(COLOR_GRAYTEXT));
          else
          {
            SetTextColor(lpdis->hDC,        GetSysColor(COLOR_HIGHLIGHTTEXT));
            SetBkColor(lpdis->hDC,          GetSysColor(COLOR_HIGHLIGHT));

#ifdef XXX_SSU_COMPONENT_SIZE
            SetTextColor(hdcComponentSize,  GetSysColor(COLOR_HIGHLIGHTTEXT));
            SetBkColor(hdcComponentSize,    GetSysColor(COLOR_HIGHLIGHT));
#endif
          }
        }
      }

      if(lpdis->itemAction & (ODA_DRAWENTIRE | ODA_FOCUS))
      {
        // Display the text associated with the item.
        GetTextMetrics(lpdis->hDC, &tm);
        y = (lpdis->rcItem.bottom + lpdis->rcItem.top - tm.tmHeight) / 2;

        ExtTextOut(lpdis->hDC,
                   CX_CHECKBOX + 5,
                   y,
                   ETO_OPAQUE | ETO_CLIPPED,
                   &(lpdis->rcItem),
                   tchBuffer,
                   strlen(tchBuffer),
                   NULL);

#ifdef XXX_SSU_COMPONENT_SIZE
        siCTemp = SiCNodeGetObject(lpdis->itemID, FALSE, AC_COMPONENTS);
        _ui64toa(siCTemp->ullInstallSizeArchive, tchBuffer, 10);
        lstrcat(tchBuffer, " K");

        /* calculate clipping region.  The region being the entire listbox window */
        GetClientRect(hwndLBComponents, &rListBox);
        if(lpdis->rcItem.bottom > rListBox.bottom)
          rLBComponentSize.bottom = rListBox.bottom - 1;
        else
          rLBComponentSize.bottom = lpdis->rcItem.bottom - 1;

        rLBComponentSize.left  = lpdis->rcItem.right - 50;
        rLBComponentSize.right = lpdis->rcItem.right;
        if(lpdis->rcItem.top < rListBox.top)
          rLBComponentSize.top = rListBox.top + 1;
        else
          rLBComponentSize.top = lpdis->rcItem.top + 1;

        /* set text alignment */
        SetTextAlign(hdcComponentSize, TA_RIGHT);
        /* output string */
        ExtTextOut(hdcComponentSize,
                   lpdis->rcItem.right - 3,
                   y,
                   ETO_OPAQUE | ETO_CLIPPED,
                   &(rLBComponentSize),
                   tchBuffer,
                   strlen(tchBuffer),
                   NULL);
#endif
      }
      
      DrawCheck(lpdis, AC_COMPONENTS);

      // draw the focus rect on the selected item
      if((lpdis->itemAction & ODA_FOCUS) &&
         (lpdis->itemState & ODS_FOCUS))
      {
        DrawFocusRect(lpdis->hDC, &(lpdis->rcItem));
      }

#ifdef XXX_SSU_COMPONENT_SIZE
      ReleaseDC(lpdis->hwndItem, hdcComponentSize);
#endif

      bReturn = TRUE;

      /* update the disk space required info in the dialog.  It is already
         in Kilobytes */
      ullDSBuf = GetDiskSpaceRequired(DSR_DOWNLOAD_SIZE);
      _ui64toa(ullDSBuf, tchBuffer, 10);
//      ParsePath(sgProduct.szPath, szBuf, sizeof(szBuf), PP_ROOT_ONLY);
//      RemoveBackSlash(szBuf);
//      lstrcat(szBuf, " - ");
      lstrcpy(szBuf, tchBuffer);
      lstrcat(szBuf, " K");
      
      SetDlgItemText(hDlg, IDC_DOWNLOAD_SIZE, szBuf);
      break;

    case WM_COMMAND:
      switch(LOWORD(wParam))
      {
        case IDC_LIST_COMPONENTS:
          /* to update the long description for each component the user selected */
          if((dwIndex = SendMessage(hwndLBComponents, LB_GETCURSEL, 0, 0)) != LB_ERR)
            SetDlgItemText(hDlg, IDC_STATIC_DESCRIPTION, SiCNodeGetDescriptionLong(dwIndex, FALSE, AC_COMPONENTS));
          break;

        case IDWIZNEXT:
          DestroyWindow(hDlg);
          PostMessage(hWndMain, WM_COMMAND, IDWIZNEXT, 0);
          break;

        case IDWIZBACK:
          DestroyWindow(hDlg);
          PostMessage(hWndMain, WM_COMMAND, IDWIZBACK, 0);
          break;

        case IDCANCEL:
          AskCancelDlg(hDlg);
          break;

        default:
          break;
      }
      break;
  }

  return(bReturn);
}

LRESULT CALLBACK DlgProcSelectAdditionalComponents(HWND hDlg, UINT msg, WPARAM wParam, LONG lParam)
{
  BOOL                bReturn = FALSE;
  siC                 *siCTemp;
  DWORD               dwIndex;
  DWORD               dwItems = MAX_BUF;
  HWND                hwndLBComponents;
  RECT                rDlg;
  TCHAR               tchBuffer[MAX_BUF];
  TEXTMETRIC          tm;
  DWORD               y;
  LPDRAWITEMSTRUCT    lpdis;
  ULONGLONG           ullDSBuf;
  char                szBuf[MAX_BUF];

  hwndLBComponents  = GetDlgItem(hDlg, IDC_LIST_COMPONENTS);

  switch(msg)
  {
    case WM_INITDIALOG:
      SetWindowText(hDlg, diSelectAdditionalComponents.szTitle);
      SetDlgItemText(hDlg, IDC_MESSAGE0, diSelectAdditionalComponents.szMessage0);

      siCTemp = siComponents;
      if(siCTemp != NULL)
      {
        if((!(siCTemp->dwAttributes & SIC_INVISIBLE)) && (siCTemp->dwAttributes & SIC_ADDITIONAL))
          lbAddItem(hwndLBComponents, siCTemp);

        siCTemp = siCTemp->Next;
        while((siCTemp != siComponents) && (siCTemp != NULL))
        {
          if((!(siCTemp->dwAttributes & SIC_INVISIBLE)) && (siCTemp->dwAttributes & SIC_ADDITIONAL))
            lbAddItem(hwndLBComponents, siCTemp);

          siCTemp = siCTemp->Next;
        }
        SetFocus(hwndLBComponents);
        SendMessage(hwndLBComponents, LB_SETCURSEL, 0, 0);
        SetDlgItemText(hDlg, IDC_STATIC_DESCRIPTION, SiCNodeGetDescriptionLong(0, FALSE, AC_ADDITIONAL_COMPONENTS));
      }

      if(GetClientRect(hDlg, &rDlg))
        SetWindowPos(hDlg, HWND_TOP, (dwScreenX/2)-(rDlg.right/2), (dwScreenY/2)-(rDlg.bottom/2), 0, 0, SWP_NOSIZE);

      /* update the disk space available info in the dialog.  GetDiskSpaceAvailable()
         returns value in kbytes */
      ullDSBuf = GetDiskSpaceAvailable(sgProduct.szPath);
      _ui64toa(ullDSBuf, tchBuffer, 10);
      ParsePath(sgProduct.szPath, szBuf, sizeof(szBuf), PP_ROOT_ONLY);
      RemoveBackSlash(szBuf);
      lstrcat(szBuf, " - ");
      lstrcat(szBuf, tchBuffer);
      lstrcat(szBuf, " K");
      SetDlgItemText(hDlg, IDC_SPACE_AVAILABLE, szBuf);

      gdwACFlag = AC_ADDITIONAL_COMPONENTS;
      OldListBoxWndProc = SubclassWindow(hwndLBComponents, (WNDPROC)NewListBoxWndProc);
      break;

    case WM_DRAWITEM:
      lpdis = (LPDRAWITEMSTRUCT)lParam;

      // If there are no list box items, skip this message.
      if(lpdis->itemID == -1)
        break;

      SendMessage(lpdis->hwndItem, LB_GETTEXT, lpdis->itemID, (LPARAM)tchBuffer);
      if((lpdis->itemAction & ODA_FOCUS) && (lpdis->itemState & ODS_SELECTED))
      {
        // remove the focus rect on the previous selected item
        DrawFocusRect(lpdis->hDC, &(lpdis->rcItem));
      }

      if(lpdis->itemAction & ODA_FOCUS)
      {
        siCTemp = SiCNodeGetObject(lpdis->itemID, FALSE, AC_ADDITIONAL_COMPONENTS);

        if((lpdis->itemState & ODS_SELECTED) &&
          !(lpdis->itemState & ODS_FOCUS))
        {
          if(siCTemp->dwAttributes & SIC_DISABLED)
            SetTextColor(lpdis->hDC,        GetSysColor(COLOR_GRAYTEXT));
          else
          {
            SetTextColor(lpdis->hDC,        GetSysColor(COLOR_WINDOWTEXT));
            SetBkColor(lpdis->hDC,          GetSysColor(COLOR_WINDOW));
          }
        }
        else
        {
          if(siCTemp->dwAttributes & SIC_DISABLED)
            SetTextColor(lpdis->hDC,        GetSysColor(COLOR_GRAYTEXT));
          else
          {
            SetTextColor(lpdis->hDC,        GetSysColor(COLOR_HIGHLIGHTTEXT));
            SetBkColor(lpdis->hDC,          GetSysColor(COLOR_HIGHLIGHT));
          }
        }
      }

      if(lpdis->itemAction & (ODA_DRAWENTIRE | ODA_FOCUS))
      {
        // Display the text associated with the item.
        GetTextMetrics(lpdis->hDC, &tm);
        y = (lpdis->rcItem.bottom + lpdis->rcItem.top - tm.tmHeight) / 2;

        ExtTextOut(lpdis->hDC,
                   CX_CHECKBOX + 5,
                   y,
                   ETO_OPAQUE | ETO_CLIPPED,
                   &(lpdis->rcItem),
                   tchBuffer,
                   strlen(tchBuffer),
                   NULL);
      }
      
      DrawCheck(lpdis, AC_ADDITIONAL_COMPONENTS);

      // draw the focus rect on the selected item
      if((lpdis->itemAction & ODA_FOCUS) &&
         (lpdis->itemState & ODS_FOCUS))
      {
        DrawFocusRect(lpdis->hDC, &(lpdis->rcItem));
      }

      bReturn = TRUE;

      /* update the disk space required info in the dialog.  It is already
         in Kilobytes */
      ullDSBuf = GetDiskSpaceRequired(DSR_DOWNLOAD_SIZE);
      _ui64toa(ullDSBuf, tchBuffer, 10);
      lstrcpy(szBuf, tchBuffer);
      lstrcat(szBuf, " K");
      
      SetDlgItemText(hDlg, IDC_DOWNLOAD_SIZE, szBuf);
      break;

    case WM_COMMAND:
      switch(LOWORD(wParam))
      {
        case IDC_LIST_COMPONENTS:
          /* to update the long description for each component the user selected */
          if((dwIndex = SendMessage(hwndLBComponents, LB_GETCURSEL, 0, 0)) != LB_ERR)
            SetDlgItemText(hDlg, IDC_STATIC_DESCRIPTION, SiCNodeGetDescriptionLong(dwIndex, FALSE, AC_ADDITIONAL_COMPONENTS));
          break;

        case IDWIZNEXT:
          DestroyWindow(hDlg);
          PostMessage(hWndMain, WM_COMMAND, IDWIZNEXT, 0);
          break;

        case IDWIZBACK:
          DestroyWindow(hDlg);
          PostMessage(hWndMain, WM_COMMAND, IDWIZBACK, 0);
          break;

        case IDCANCEL:
          AskCancelDlg(hDlg);
          break;

        default:
          break;
      }
      break;
  }

  return(bReturn);
}

LRESULT CALLBACK DlgProcWindowsIntegration(HWND hDlg, UINT msg, WPARAM wParam, LONG lParam)
{
  HWND hcbCheck0;
  HWND hcbCheck1;
  HWND hcbCheck2;
  HWND hcbCheck3;
  RECT rDlg;

  hcbCheck0 = GetDlgItem(hDlg, IDC_CHECK0);
  hcbCheck1 = GetDlgItem(hDlg, IDC_CHECK1);
  hcbCheck2 = GetDlgItem(hDlg, IDC_CHECK2);
  hcbCheck3 = GetDlgItem(hDlg, IDC_CHECK3);

  switch(msg)
  {
    case WM_INITDIALOG:
      SetWindowText(hDlg, diWindowsIntegration.szTitle);
      SetDlgItemText(hDlg, IDC_MESSAGE0, diWindowsIntegration.szMessage0);
      SetDlgItemText(hDlg, IDC_MESSAGE1, diWindowsIntegration.szMessage1);

      if(diWindowsIntegration.wiCB0.bEnabled)
      {
        ShowWindow(hcbCheck0, SW_SHOW);
        CheckDlgButton(hDlg, IDC_CHECK0, diWindowsIntegration.wiCB0.bCheckBoxState);
        SetDlgItemText(hDlg, IDC_CHECK0, diWindowsIntegration.wiCB0.szDescription);
      }
      else
        ShowWindow(hcbCheck0, SW_HIDE);

      if(diWindowsIntegration.wiCB1.bEnabled)
      {
        ShowWindow(hcbCheck1, SW_SHOW);
        CheckDlgButton(hDlg, IDC_CHECK1, diWindowsIntegration.wiCB1.bCheckBoxState);
        SetDlgItemText(hDlg, IDC_CHECK1, diWindowsIntegration.wiCB1.szDescription);
      }
      else
        ShowWindow(hcbCheck1, SW_HIDE);

      if(diWindowsIntegration.wiCB2.bEnabled)
      {
        ShowWindow(hcbCheck2, SW_SHOW);
        CheckDlgButton(hDlg, IDC_CHECK2, diWindowsIntegration.wiCB2.bCheckBoxState);
        SetDlgItemText(hDlg, IDC_CHECK2, diWindowsIntegration.wiCB2.szDescription);
      }
      else
        ShowWindow(hcbCheck2, SW_HIDE);

      if(diWindowsIntegration.wiCB3.bEnabled)
      {
        ShowWindow(hcbCheck3, SW_SHOW);
        CheckDlgButton(hDlg, IDC_CHECK3, diWindowsIntegration.wiCB3.bCheckBoxState);
        SetDlgItemText(hDlg, IDC_CHECK3, diWindowsIntegration.wiCB3.szDescription);
      }
      else
        ShowWindow(hcbCheck3, SW_HIDE);

      if(GetClientRect(hDlg, &rDlg))
        SetWindowPos(hDlg, HWND_TOP, (dwScreenX/2)-(rDlg.right/2), (dwScreenY/2)-(rDlg.bottom/2), 0, 0, SWP_NOSIZE);

      break;

    case WM_COMMAND:
      switch(LOWORD(wParam))
      {
        case IDWIZNEXT:
          if(IsDlgButtonChecked(hDlg, IDC_CHECK0) == BST_CHECKED)
          {
          }

          if(diWindowsIntegration.wiCB0.bEnabled)
          {
            if(IsDlgButtonChecked(hDlg, IDC_CHECK0) == BST_CHECKED)
              diWindowsIntegration.wiCB0.bCheckBoxState = TRUE;
            else
              diWindowsIntegration.wiCB0.bCheckBoxState = FALSE;
          }

          if(diWindowsIntegration.wiCB1.bEnabled)
          {
            if(IsDlgButtonChecked(hDlg, IDC_CHECK1) == BST_CHECKED)
              diWindowsIntegration.wiCB1.bCheckBoxState = TRUE;
            else
              diWindowsIntegration.wiCB1.bCheckBoxState = FALSE;
          }

          if(diWindowsIntegration.wiCB2.bEnabled)
          {
            if(IsDlgButtonChecked(hDlg, IDC_CHECK2) == BST_CHECKED)
              diWindowsIntegration.wiCB2.bCheckBoxState = TRUE;
            else
              diWindowsIntegration.wiCB2.bCheckBoxState = FALSE;
          }

          if(diWindowsIntegration.wiCB3.bEnabled)
          {
            if(IsDlgButtonChecked(hDlg, IDC_CHECK3) == BST_CHECKED)
              diWindowsIntegration.wiCB3.bCheckBoxState = TRUE;
            else
              diWindowsIntegration.wiCB3.bCheckBoxState = FALSE;
          }

          DestroyWindow(hDlg);
          PostMessage(hWndMain, WM_COMMAND, IDWIZNEXT, 0);
          break;

        case IDWIZBACK:
          dwWizardState = DLG_WINDOWS_INTEGRATION;
          CheckWizardStateCustom(DLG_SETUP_TYPE);

          DestroyWindow(hDlg);
          PostMessage(hWndMain, WM_COMMAND, IDWIZBACK, 0);
          break;

        case IDCANCEL:
          AskCancelDlg(hDlg);
          break;

        default:
          break;
      }
      break;
  }
  return(0);
}

LRESULT CALLBACK DlgProcProgramFolder(HWND hDlg, UINT msg, WPARAM wParam, LONG lParam)
{
  char            szBuf[MAX_BUF];
  HANDLE          hDir;
  DWORD           dwIndex;
  WIN32_FIND_DATA wfdFindFileData;
  RECT            rDlg;

  switch(msg)
  {
    case WM_INITDIALOG:
      SetWindowText(hDlg, diProgramFolder.szTitle);
      SetDlgItemText(hDlg, IDC_MESSAGE0, diProgramFolder.szMessage0);
      SetDlgItemText(hDlg, IDC_EDIT_PROGRAM_FOLDER, sgProduct.szProgramFolderName);

      lstrcpy(szBuf, sgProduct.szProgramFolderPath);
      lstrcat(szBuf, "\\*.*");
      if((hDir = FindFirstFile(szBuf , &wfdFindFileData)) != INVALID_HANDLE_VALUE)
      {
        if((wfdFindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && (lstrcmpi(wfdFindFileData.cFileName, ".") != 0) && (lstrcmpi(wfdFindFileData.cFileName, "..") != 0))
        {
          SendDlgItemMessage(hDlg, IDC_LIST, LB_ADDSTRING, 0, (LPARAM)wfdFindFileData.cFileName);
        }

        while(FindNextFile(hDir, &wfdFindFileData))
        {
          if((wfdFindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && (lstrcmpi(wfdFindFileData.cFileName, ".") != 0) && (lstrcmpi(wfdFindFileData.cFileName, "..") != 0))
            SendDlgItemMessage(hDlg, IDC_LIST, LB_ADDSTRING, 0, (LPARAM)wfdFindFileData.cFileName);
        }
        FindClose(hDir);
      }

      if(GetClientRect(hDlg, &rDlg))
        SetWindowPos(hDlg, HWND_TOP, (dwScreenX/2)-(rDlg.right/2), (dwScreenY/2)-(rDlg.bottom/2), 0, 0, SWP_NOSIZE);

      break;

    case WM_COMMAND:
      switch(LOWORD(wParam))
      {
        case IDWIZNEXT:
          GetDlgItemText(hDlg, IDC_EDIT_PROGRAM_FOLDER, szBuf, MAX_BUF);
          if(*szBuf == '\0')
          {
            char szEProgramFolderName[MAX_BUF];

            NS_LoadString(hSetupRscInst, IDS_ERROR_PROGRAM_FOLDER_NAME, szEProgramFolderName, MAX_BUF);
            MessageBox(hDlg, szEProgramFolderName, NULL, MB_OK | MB_ICONEXCLAMATION);
            break;
          }
          lstrcpy(sgProduct.szProgramFolderName, szBuf);
          dwWizardState = DLG_ADVANCED_SETTINGS;

          DestroyWindow(hDlg);
          PostMessage(hWndMain, WM_COMMAND, IDWIZNEXT, 0);
          break;

        case IDWIZBACK:
          DestroyWindow(hDlg);
          PostMessage(hWndMain, WM_COMMAND, IDWIZBACK, 0);
          break;

        case IDC_LIST:
          if((dwIndex = SendDlgItemMessage(hDlg, IDC_LIST, LB_GETCURSEL, 0, 0)) != LB_ERR)
          {
            SendDlgItemMessage(hDlg, IDC_LIST, LB_GETTEXT, dwIndex, (LPARAM)szBuf);
            SetDlgItemText(hDlg, IDC_EDIT_PROGRAM_FOLDER, szBuf);
          }
          break;

        case IDCANCEL:
          AskCancelDlg(hDlg);
          break;

        default:
          break;
      }
      break;
  }
  return(0);
}

LRESULT CALLBACK DlgAdvancedSettings(HWND hDlg, UINT msg, WPARAM wParam, LONG lParam)
{
  RECT  rDlg;
  HWND  hwndCBSiteSelector;
  int   iIndex;
  ssi   *ssiTemp;
  char  szCBDefault[MAX_BUF];

  hwndCBSiteSelector = GetDlgItem(hDlg, IDC_LIST_SITE_SELECTOR);

  switch(msg)
  {
    case WM_INITDIALOG:
      SetWindowText(hDlg, diAdvancedSettings.szTitle);
      SetDlgItemText(hDlg, IDC_MESSAGE0, diAdvancedSettings.szMessage0);
      SetDlgItemText(hDlg, IDC_MESSAGE1, diAdvancedSettings.szMessage1);

      if(GetClientRect(hDlg, &rDlg))
        SetWindowPos(hDlg, HWND_TOP, (dwScreenX/2)-(rDlg.right/2), (dwScreenY/2)-(rDlg.bottom/2), 0, 0, SWP_NOSIZE);

      ssiTemp = ssiSiteSelector;
      do
      {
        if(ssiTemp == NULL)
          break;

        SendMessage(hwndCBSiteSelector, CB_ADDSTRING, 0, (LPARAM)(ssiTemp->szDescription));
        ssiTemp = ssiTemp->Next;
      } while(ssiTemp != ssiSiteSelector);

      if((szSiteSelectorDescription == NULL) || (*szSiteSelectorDescription == '\0'))
      {
        if((NS_LoadString(hSetupRscInst, IDS_CB_DEFAULT, szCBDefault, MAX_BUF) == WIZ_OK) &&
          ((iIndex = SendMessage(hwndCBSiteSelector, CB_SELECTSTRING, -1, (LPARAM)szCBDefault)) != CB_ERR))
          SendMessage(hwndCBSiteSelector, CB_SETCURSEL, (WPARAM)iIndex, 0);
        else
          SendMessage(hwndCBSiteSelector, CB_SETCURSEL, 0, 0);
      }
      else if((iIndex = SendMessage(hwndCBSiteSelector, CB_SELECTSTRING, -1, (LPARAM)szSiteSelectorDescription)) != CB_ERR)
        SendMessage(hwndCBSiteSelector, CB_SETCURSEL, (WPARAM)iIndex, 0);
      else
        SendMessage(hwndCBSiteSelector, CB_SETCURSEL, 0, 0);

      if(bSaveInstallerFiles)
        CheckDlgButton(hDlg, IDC_CHECK_SAVE_INSTALLER_FILES, BST_CHECKED);
      else
        CheckDlgButton(hDlg, IDC_CHECK_SAVE_INSTALLER_FILES, BST_UNCHECKED);
      break;

    case WM_COMMAND:
      switch(LOWORD(wParam))
      {
        case IDWIZNEXT:
          /* get selected item from the site selector's pull down list */
          iIndex = SendMessage(hwndCBSiteSelector, CB_GETCURSEL, 0, 0);
          SendMessage(hwndCBSiteSelector, CB_GETLBTEXT, (WPARAM)iIndex, (LPARAM)szSiteSelectorDescription);
          dwWizardState = DLG_ADVANCED_SETTINGS;

          /* get the state of the Save Installer Files checkbox */
          if(IsDlgButtonChecked(hDlg, IDC_CHECK_SAVE_INSTALLER_FILES) == BST_CHECKED)
            bSaveInstallerFiles = TRUE;
          else
            bSaveInstallerFiles = FALSE;

          /* get the proxy server and port information */
          GetDlgItemText(hDlg, IDC_EDIT_PROXY_SERVER, sgProduct.szProxyServer, MAX_BUF);
          GetDlgItemText(hDlg, IDC_EDIT_PROXY_PORT,   sgProduct.szProxyPort,   MAX_BUF);
          if((*sgProduct.szProxyServer == '\0') || (*sgProduct.szProxyPort == '\0'))
          {
            ZeroMemory(sgProduct.szProxyServer, MAX_BUF);
            ZeroMemory(sgProduct.szProxyPort,   MAX_BUF);
          }

          DestroyWindow(hDlg);
          PostMessage(hWndMain, WM_COMMAND, IDWIZNEXT, 0);
          break;

        case IDWIZBACK:
          iIndex = SendMessage(hwndCBSiteSelector, CB_GETCURSEL, 0, 0);
          SendMessage(hwndCBSiteSelector, CB_GETLBTEXT, (WPARAM)iIndex, (LPARAM)szSiteSelectorDescription);
          DestroyWindow(hDlg);
          PostMessage(hWndMain, WM_COMMAND, IDWIZBACK, 0);
          break;

        case IDCANCEL:
          AskCancelDlg(hDlg);
          break;

        default:
          break;
      }
      break;
  }
  return(0);
}

LRESULT CALLBACK DlgProcStartInstall(HWND hDlg, UINT msg, WPARAM wParam, LONG lParam)
{
  RECT rDlg;

  switch(msg)
  {
    case WM_INITDIALOG:
      SetWindowText(hDlg, diStartInstall.szTitle);
      SetDlgItemText(hDlg, IDC_MESSAGE0, diStartInstall.szMessage0);

      if(GetClientRect(hDlg, &rDlg))
        SetWindowPos(hDlg, HWND_TOP, (dwScreenX/2)-(rDlg.right/2), (dwScreenY/2)-(rDlg.bottom/2), 0, 0, SWP_NOSIZE);

      if((diAdvancedSettings.bShowDialog == FALSE) || (GetTotalArchivesToDownload() == 0))
        ShowWindow(GetDlgItem(hDlg, IDC_BUTTON_SITE_SELECTOR), SW_HIDE);
      else
        ShowWindow(GetDlgItem(hDlg, IDC_BUTTON_SITE_SELECTOR), SW_SHOW);

      break;

    case WM_COMMAND:
      switch(LOWORD(wParam))
      {
        case IDWIZNEXT:
          DestroyWindow(hDlg);
          PostMessage(hWndMain, WM_COMMAND, IDWIZNEXT, 0);
          break;

        case IDWIZBACK:
          dwWizardState = DLG_ADVANCED_SETTINGS;
          DestroyWindow(hDlg);
          PostMessage(hWndMain, WM_COMMAND, IDWIZBACK, 0);
          break;

        case IDC_BUTTON_SITE_SELECTOR:
          dwWizardState = DLG_PROGRAM_FOLDER;
          DestroyWindow(hDlg);
          PostMessage(hWndMain, WM_COMMAND, IDWIZNEXT, 0);
          break;

        case IDCANCEL:
          AskCancelDlg(hDlg);
          break;

        default:
          break;
      }
      break;
  }
  return(0);
}

LRESULT CALLBACK DlgProcReboot(HWND hDlg, UINT msg, WPARAM wParam, LONG lParam)
{
  HANDLE            hToken;
  TOKEN_PRIVILEGES  tkp;
  HWND              hRadioYes;
  RECT              rDlg;

  hRadioYes = GetDlgItem(hDlg, IDC_RADIO_YES);

  switch(msg)
  {
    case WM_INITDIALOG:
      CheckDlgButton(hDlg, IDC_RADIO_YES, BST_CHECKED);
      SetFocus(hRadioYes);

      if(GetClientRect(hDlg, &rDlg))
        SetWindowPos(hDlg, HWND_TOP, (dwScreenX/2)-(rDlg.right/2), (dwScreenY/2)-(rDlg.bottom/2), 0, 0, SWP_NOSIZE);

      break;

    case WM_COMMAND:
      switch(LOWORD(wParam))
      {
        case IDOK:
          if(IsDlgButtonChecked(hDlg, IDC_RADIO_YES) == BST_CHECKED)
          {
            // Get a token for this process.
            OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken);

            // Get the LUID for the shutdown privilege.
            LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid);
            tkp.PrivilegeCount = 1;  // one privilege to set
            tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

            // Get the shutdown privilege for this process.
            AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES)NULL, 0);

            DestroyWindow(hDlg);
            PostQuitMessage(0);
            DestroyWindow(hWndMain);

            // Reboot the system and force all applications to close.
            ExitWindowsEx(EWX_REBOOT, 0);
          }
          else
          {
            DestroyWindow(hDlg);
            PostQuitMessage(0);
          }
          break;

        case IDCANCEL:
          DestroyWindow(hDlg);
          PostQuitMessage(0);
          break;

        default:
          break;
      }
      break;
  }
  return(0);
}

LRESULT CALLBACK DlgProcMessage(HWND hDlg, UINT msg, WPARAM wParam, LONG lParam)
{
  RECT rDlg;
  HWND hSTMessage = GetDlgItem(hDlg, IDC_MESSAGE); /* handle to the Static Text message window */
  HDC  hdcSTMessage;
  SIZE sizeString;
  int  iLen;
//  int  iCount;
//  int  iCharWidth;
//  UINT uiTotalWidth;

  switch(msg)
  {
    case WM_INITDIALOG:
      break;

    case WM_COMMAND:
      switch(LOWORD(wParam))
      {
        case IDC_MESSAGE:
          hdcSTMessage = GetWindowDC(hSTMessage);
          iLen = lstrlen((LPSTR)lParam);
          GetTextExtentPoint32(hdcSTMessage, (LPSTR)lParam, iLen, &sizeString);

/*          uiTotalWidth = 0;
          for(iCount = 0; iCount < iLen; iCount ++)
          {
            GetCharWidth32(hdcSTMessage, ((LPSTR)lParam)[iCount], ((LPSTR)lParam)[iCount], &iCharWidth);
            uiTotalWidth += iCharWidth;
          }
*/

          ReleaseDC(hSTMessage, hdcSTMessage);

          SetWindowPos(hDlg, HWND_TOP,
                      (dwScreenX/2)-((sizeString.cx - (iLen * 1))/2), (dwScreenY/2)-((sizeString.cy + 50)/2),
                      (sizeString.cx - (iLen * 1)), sizeString.cy + 50,
                       SWP_SHOWWINDOW);

          if(GetClientRect(hDlg, &rDlg))
            SetWindowPos(hSTMessage, HWND_TOP,
                         rDlg.left, rDlg.top,
                         (sizeString.cx - (iLen * 1)), rDlg.bottom,
                         SWP_SHOWWINDOW);

          SetDlgItemText(hDlg, IDC_MESSAGE, (LPSTR)lParam);
          break;
      }
      break;
  }
  return(0);
}

void ProcessWindowsMessages()
{
  MSG msg;

  while(PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
  {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
}

void ShowMessage(LPSTR szMessage, BOOL bShow)
{
  char szBuf[MAX_BUF];

  if(sgProduct.dwMode != SILENT)
  {
    if((bShow) && (hDlgMessage == NULL))
    {
      ZeroMemory(szBuf, sizeof(szBuf));
      NS_LoadString(hSetupRscInst, IDS_MB_MESSAGE_STR, szBuf, MAX_BUF);
      InstantiateDialog(DLG_MESSAGE, szBuf, DlgProcMessage);
      SendMessage(hDlgMessage, WM_COMMAND, IDC_MESSAGE, (LPARAM)szMessage);
    }
    else if(!bShow && hDlgMessage)
    {
      DestroyWindow(hDlgMessage);
      hDlgMessage = NULL;
    }
  }
}

void InstantiateDialog(DWORD dwDlgID, LPSTR szTitle, WNDPROC wpDlgProc)
{
  char szBuf[MAX_BUF];

  if((hDlg = CreateDialog(hSetupRscInst, MAKEINTRESOURCE(dwDlgID), hWndMain, wpDlgProc)) == NULL)
  {
    char szEDialogCreate[MAX_BUF];

    if(NS_LoadString(hSetupRscInst, IDS_ERROR_DIALOG_CREATE, szEDialogCreate, MAX_BUF) == WIZ_OK)
    {
      wsprintf(szBuf, szEDialogCreate, szTitle);
      PrintError(szBuf, ERROR_CODE_SHOW);
    }
    PostQuitMessage(1);
  }
  else if(dwDlgID == DLG_MESSAGE)
  {
    hDlgMessage = hDlg;
  }
}

void CheckWizardStateCustom(DWORD dwDefault)
{
  if(sgProduct.dwCustomType != dwSetupType)
    dwWizardState = dwDefault;
}

void DlgSequenceNext()
{
  HRESULT hrValue;
  HRESULT hrErr;

  switch(dwWizardState)
  {
    case DLG_NONE:
      dwWizardState = DLG_WELCOME;
      gbProcessingXpnstallFiles = FALSE;
      if(diWelcome.bShowDialog)
        InstantiateDialog(dwWizardState, diWelcome.szTitle, DlgProcWelcome);
      else
        PostMessage(hWndMain, WM_COMMAND, IDWIZNEXT, 0);
      break;

    case DLG_WELCOME:
      dwWizardState = DLG_LICENSE;
      gbProcessingXpnstallFiles = FALSE;
      if(diLicense.bShowDialog)
        InstantiateDialog(dwWizardState, diLicense.szTitle, DlgProcLicense);
      else
        PostMessage(hWndMain, WM_COMMAND, IDWIZNEXT, 0);

      break;

    case DLG_LICENSE:
      dwWizardState = DLG_SETUP_TYPE;
      gbProcessingXpnstallFiles = FALSE;
      if(diSetupType.bShowDialog)
        InstantiateDialog(dwWizardState, diSetupType.szTitle, DlgProcSetupType);
      else
      {
        CheckWizardStateCustom(DLG_SELECT_ADDITIONAL_COMPONENTS);
        PostMessage(hWndMain, WM_COMMAND, IDWIZNEXT, 0);
      }
      break;

    case DLG_SETUP_TYPE:
      dwWizardState = DLG_SELECT_COMPONENTS;
      gbProcessingXpnstallFiles = FALSE;
      if(diSelectComponents.bShowDialog)
        InstantiateDialog(dwWizardState, diSelectComponents.szTitle, DlgProcSelectComponents);
      else
        PostMessage(hWndMain, WM_COMMAND, IDWIZNEXT, 0);
      break;

    case DLG_SELECT_COMPONENTS:
      dwWizardState = DLG_SELECT_ADDITIONAL_COMPONENTS;
      gbProcessingXpnstallFiles = FALSE;
      if((diSelectAdditionalComponents.bShowDialog) && (GetAdditionalComponentsCount() > 0))
        InstantiateDialog(dwWizardState, diSelectAdditionalComponents.szTitle, DlgProcSelectAdditionalComponents);
      else
        PostMessage(hWndMain, WM_COMMAND, IDWIZNEXT, 0);
      break;

    case DLG_SELECT_ADDITIONAL_COMPONENTS:
      dwWizardState = DLG_WINDOWS_INTEGRATION;
      gbProcessingXpnstallFiles = FALSE;
      if(diWindowsIntegration.bShowDialog)
        InstantiateDialog(dwWizardState, diWindowsIntegration.szTitle, DlgProcWindowsIntegration);
      else
        PostMessage(hWndMain, WM_COMMAND, IDWIZNEXT, 0);
      break;

    case DLG_WINDOWS_INTEGRATION:
      dwWizardState = DLG_PROGRAM_FOLDER;
      gbProcessingXpnstallFiles = FALSE;
      do
      {
        hrValue = VerifyDiskSpace();
        if(hrValue == IDOK)
        {
          /* show previous visible window */
          DlgSequencePrev();
          break;
        }
        else if(hrValue == IDCANCEL)
        {
          AskCancelDlg(hWndMain);
          hrValue = IDRETRY;
        }
      }while(hrValue == IDRETRY);

      if(hrValue == IDOK)
      {
        /* break out of this case because we need to show the previous dialog */
        break;
      }

      if(diProgramFolder.bShowDialog)
        InstantiateDialog(dwWizardState, diProgramFolder.szTitle, DlgProcProgramFolder);
      else
      {
        dwWizardState = DLG_ADVANCED_SETTINGS;
        PostMessage(hWndMain, WM_COMMAND, IDWIZNEXT, 0);
      }
      break;

    case DLG_PROGRAM_FOLDER:
      dwWizardState = DLG_ADVANCED_SETTINGS;
      gbProcessingXpnstallFiles = FALSE;
      if(diAdvancedSettings.bShowDialog)
        InstantiateDialog(dwWizardState, diAdvancedSettings.szTitle, DlgAdvancedSettings);
      else
      {
        dwWizardState = DLG_ADVANCED_SETTINGS;
        PostMessage(hWndMain, WM_COMMAND, IDWIZNEXT, 0);
      }
      break;

    case DLG_ADVANCED_SETTINGS:
      dwWizardState = DLG_START_INSTALL;
      gbProcessingXpnstallFiles = FALSE;
      if(diStartInstall.bShowDialog)
        InstantiateDialog(dwWizardState, diStartInstall.szTitle, DlgProcStartInstall);
      else
        PostMessage(hWndMain, WM_COMMAND, IDWIZNEXT, 0);
      break;

    default:
      dwWizardState = DLG_START_INSTALL;
      gbProcessingXpnstallFiles = TRUE;

      /* PRE_DOWNLOAD process file manipulation functions */
      ProcessFileOps(T_PRE_DOWNLOAD);

      if(RetrieveArchives() == WIZ_OK)
      {
        /* POST_DOWNLOAD process file manipulation functions */
        ProcessFileOps(T_POST_DOWNLOAD);
        /* PRE_XPCOM process file manipulation functions */
        ProcessFileOps(T_PRE_XPCOM);

        ProcessXpcomFile();

        /* POST_XPCOM process file manipulation functions */
        ProcessFileOps(T_POST_XPCOM);
        /* PRE_SMARTUPDATE process file manipulation functions */
        ProcessFileOps(T_PRE_SMARTUPDATE);

        /* save the installer files in the local machine */
        if(bSaveInstallerFiles)
          SaveInstallerFiles();

        hrErr = SmartUpdateJars();
        if((hrErr == WIZ_OK) || (hrErr == 999))
        {
          /* POST_SMARTUPDATE process file manipulation functions */
          ProcessFileOps(T_POST_SMARTUPDATE);
          /* PRE_LAUNCHAPP process file manipulation functions */
          ProcessFileOps(T_PRE_LAUNCHAPP);

          LaunchApps();

          /* POST_LAUNCHAPP process file manipulation functions */
          ProcessFileOps(T_POST_LAUNCHAPP);
          /* DEPEND_REBOOT process file manipulation functions */
          ProcessFileOps(T_DEPEND_REBOOT);
          ProcessProgramFolderShowCmd();

          if(NeedReboot())
          {
            CleanupXpcomFile();
            InstantiateDialog(DLG_RESTART, diReboot.szTitle, DlgProcReboot);
          }
          else
          {
            CleanupXpcomFile();
            PostQuitMessage(0);
          }
        }
        else
        {
          CleanupXpcomFile();
          PostQuitMessage(0);
        }
      }
      else
      {
        bSDUserCanceled = TRUE;
        CleanupXpcomFile();
        PostQuitMessage(0);
      }
      gbProcessingXpnstallFiles = FALSE;

      break;
  }
}

void DlgSequencePrev()
{
  switch(dwWizardState)
  {
    case DLG_START_INSTALL:
      dwWizardState = DLG_ADVANCED_SETTINGS;
      gbProcessingXpnstallFiles = FALSE;
      if(diAdvancedSettings.bShowDialog)
        InstantiateDialog(dwWizardState, diAdvancedSettings.szTitle, DlgAdvancedSettings);
      else
        PostMessage(hWndMain, WM_COMMAND, IDWIZBACK, 0);
      break;

    case DLG_ADVANCED_SETTINGS:
      dwWizardState = DLG_PROGRAM_FOLDER;
      gbProcessingXpnstallFiles = FALSE;
      if(diProgramFolder.bShowDialog)
        InstantiateDialog(dwWizardState, diProgramFolder.szTitle, DlgProcProgramFolder);
      else
        PostMessage(hWndMain, WM_COMMAND, IDWIZBACK, 0);
      break;

    case DLG_PROGRAM_FOLDER:
      dwWizardState = DLG_WINDOWS_INTEGRATION;
      gbProcessingXpnstallFiles = FALSE;
      if(diWindowsIntegration.bShowDialog)
        InstantiateDialog(dwWizardState, diWindowsIntegration.szTitle, DlgProcWindowsIntegration);
      else
      {
        CheckWizardStateCustom(DLG_SELECT_COMPONENTS);
        PostMessage(hWndMain, WM_COMMAND, IDWIZBACK, 0);
      }
      break;

    case DLG_WINDOWS_INTEGRATION:
      dwWizardState = DLG_SELECT_ADDITIONAL_COMPONENTS;
      gbProcessingXpnstallFiles = FALSE;
      if((diSelectAdditionalComponents.bShowDialog) && (GetAdditionalComponentsCount() > 0))
        InstantiateDialog(dwWizardState, diSelectAdditionalComponents.szTitle, DlgProcSelectAdditionalComponents);
      else
        PostMessage(hWndMain, WM_COMMAND, IDWIZBACK, 0);
      break;

    case DLG_SELECT_ADDITIONAL_COMPONENTS:
      dwWizardState = DLG_SELECT_COMPONENTS;
      gbProcessingXpnstallFiles = FALSE;
      if(diSelectComponents.bShowDialog)
        InstantiateDialog(dwWizardState, diSelectComponents.szTitle, DlgProcSelectComponents);
      else
        PostMessage(hWndMain, WM_COMMAND, IDWIZBACK, 0);
      break;

    case DLG_SELECT_COMPONENTS:
      dwWizardState = DLG_SETUP_TYPE;
      gbProcessingXpnstallFiles = FALSE;
      if(diSetupType.bShowDialog)
        InstantiateDialog(dwWizardState, diSetupType.szTitle, DlgProcSetupType);
      else
        PostMessage(hWndMain, WM_COMMAND, IDWIZBACK, 0);
      break;

    case DLG_SETUP_TYPE:
      dwWizardState = DLG_LICENSE;
      gbProcessingXpnstallFiles = FALSE;
      if(diLicense.bShowDialog)
        InstantiateDialog(dwWizardState, diLicense.szTitle, DlgProcLicense);
      else
        PostMessage(hWndMain, WM_COMMAND, IDWIZBACK, 0);
      break;

    case DLG_LICENSE:
      dwWizardState = DLG_WELCOME;
      gbProcessingXpnstallFiles = FALSE;
      if(diWelcome.bShowDialog)
        InstantiateDialog(dwWizardState, diWelcome.szTitle, DlgProcWelcome);
      else
        PostMessage(hWndMain, WM_COMMAND, IDWIZBACK, 0);
      break;

    default:
      dwWizardState = DLG_WELCOME;
      gbProcessingXpnstallFiles = FALSE;
      if(diWelcome.bShowDialog)
        InstantiateDialog(DLG_WELCOME, diWelcome.szTitle, DlgProcWelcome);
      else
        PostMessage(hWndMain, WM_COMMAND, IDWIZBACK, 0);
      break;
  }
}
