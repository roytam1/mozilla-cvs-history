/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express oqr
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is Mozilla Communicator client code, released
 * March 31, 1998.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1999 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 *   John Bandhauer <jband@netscape.com>
 *   Garrett Arch Blythe <blythe@netscape.com>
 *          03/19/2002  Unicode enabled for WinCE
 *
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU Public License (the "GPL"), in which case the
 * provisions of the GPL are applicable instead of those above.
 * If you wish to allow use of your version of this file only
 * under the terms of the GPL and not to allow others to use your
 * version of this file under the NPL, indicate your decision by
 * deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL.  If you do not delete
 * the provisions above, a recipient may use your version of this
 * file under either the NPL or the GPL.
 */

/* Windows only app to show a modal debug dialog - launched by nsDebug.cpp */

#include <windows.h>

int WINAPI 
WinMain(HINSTANCE  hInstance, HINSTANCE  hPrevInstance, 
        LPTSTR  lpszCmdLine, int  nCmdShow)
{
    static TCHAR msg[4048];

    wsprintf(msg,
             _T("%s\n\nClick Abort to exit the Application.\n")
             _T("Click Retry to Debug the Application..\n")
             _T("Click Ignore to continue running the Application."), 
             lpszCmdLine);
             
    return MessageBox(NULL, msg, _T("nsDebug::Assertion"),
                      MB_ICONSTOP |
#if !defined(WINCE)
                      MB_SYSTEMMODAL |
#else
                      MB_APPLMODAL | MB_TOPMOST | MB_SETFOREGROUND |
#endif
                      MB_ABORTRETRYIGNORE | MB_DEFBUTTON3);
}
