/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* 
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is the Netscape Portable Runtime (NSPR).
 * 
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are 
 * Copyright (C) 1998-2000 Netscape Communications Corporation.  All
 * Rights Reserved.
 * 
 * Contributor(s):
 *  Garrett Arch Blythe, 02/05/2002
 * 
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU General Public License Version 2 or later (the
 * "GPL"), in which case the provisions of the GPL are applicable 
 * instead of those above.  If you wish to allow use of your 
 * version of this file only under the terms of the GPL and not to
 * allow others to use your version of this file under the MPL,
 * indicate your decision by deleting the provisions above and
 * replace them with the notice and other provisions required by
 * the GPL.  If you do not delete the provisions above, a recipient
 * may use your version of this file under either the MPL or the
 * GPL.
 */

/*
 * w32unicode.c
 *
 * This file exists mainly to provide easy ways to convert internal
 *  multibyte string representations into their wide character
 *  counterparts.
 *
 * FYI:
 *
 * WinCE only has the UNICODE Win32 API (funcW vs. funcA), and so there
 *  is no choice as to which API to utilize (the main reason this file
 *  exists).
 * WinNT functions as a UNICODE Win32 API with automatic conversions
 *  functioning in the funcA (using funcW would be faster for NT).
 * Win95 functions as a multibyte Win32 API with automatic conversions
 *  functioning in the funcW (using funcA would be faster for win9x).
 */

#include "primpl.h"

/*
 * _PR_MD_MALLOC_A2W
 *
 * Automatically PR_Malloc a wide char string and return it based on the
 *  ANSI (multi byte, ansi code page) string passed in.
 *
 * Caller must PR_Free the return value if non-NULL.
 */
LPWSTR _PR_MD_MALLOC_A2W(LPCSTR inString)
{
    LPWSTR retval = NULL;

    if(NULL != inString)
    {
        int neededWChars = 0;

        neededWChars = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, inString, -1, NULL, 0);
        if(0 < neededWChars)
        {
            LPWSTR wstr = NULL;

            wstr = (LPWSTR)PR_Malloc(sizeof(WCHAR) * neededWChars);
            if(NULL != wstr)
            {
                int convertRes = 0;

                convertRes = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, inString, -1, wstr, neededWChars);
                if(0 == convertRes)
                {
                    PR_Free(wstr);
                }
                else
                {
                    retval = wstr;
                }
            }
        }
    }

    return retval;
}

/*
 * _PR_MD_A2W
 *
 * Non-mallocing version to return a wide char string based on the
 *  ANSI (multi byte, ansi code page) string passed in.
 *
 * NOTE:  inWideStringChars is number of wide characters in outWideString,
 *          NOT the number of bytes....
 */
LPWSTR _PR_MD_A2W(LPCSTR inString, LPWSTR outWideString, int inWideStringChars)
{
    LPWSTR retval = outWideString;

    if(NULL != outWideString)
    {
        int convertRes = 0;

        convertRes = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, inString, -1, outWideString, inWideStringChars);
        if(0 == convertRes)
        {
            retval = NULL;
        }
    }

    return retval;
}

#if defined(WINCE)

/*
** WINCE Only
**
** This entire section of code dedicated to doing conversions between
**  ANSI code page APIs and UNICODE APIs (funcA to funcW).
** This is similar to what NT does to support the funcAs.
*/

VOID
WINAPI
OutputDebugStringA(
    LPCSTR lpOutputString
    )
{
    LPWSTR str = NULL;

    str = _PR_MD_MALLOC_A2W(lpOutputString);
    if(NULL != str)
    {
        OutputDebugStringW(str);
        PR_Free(str);
    }
    else
    {
        PR_SetError(PR_OUT_OF_MEMORY_ERROR, 0);
    }
}

HINSTANCE
WINAPI
LoadLibraryA(
    LPCSTR lpLibFileName
    )
{
    HINSTANCE retval = NULL;
    LPWSTR wideStr = NULL;
    WCHAR widePath[MAX_PATH + 1];

    wideStr = _PR_MD_A2W(lpLibFileName, widePath, sizeof(widePath) / sizeof(WCHAR));
    if(NULL != wideStr)
    {
        retval = LoadLibraryW(wideStr);
    }
    else
    {
        PR_SetError(PR_NAME_TOO_LONG_ERROR, 0);
    }

    return retval;
}

BOOL
WINAPI
CreateProcessA (
    LPCSTR pszImageName,
    LPCSTR pszCmdLine,
    LPSECURITY_ATTRIBUTES psaProcess,
    LPSECURITY_ATTRIBUTES psaThread,
    BOOL fInheritHandles,
    DWORD fdwCreate,
    LPVOID pvEnvironment,
    LPSTR pszCurDir,
    LPSTARTUPINFO psiStartInfo,
    LPPROCESS_INFORMATION pProcInfo
    )
{
    BOOL retval = FALSE;
    LPWSTR wideImageName = NULL;

    wideImageName = _PR_MD_MALLOC_A2W(pszImageName);
    if(NULL != wideImageName)
    {
        LPWSTR wideCmdLine = NULL;

        wideCmdLine = _PR_MD_MALLOC_A2W(pszCmdLine);
        if(NULL == pszCmdLine || NULL != wideCmdLine)
        {
            LPWSTR wideCurDir = NULL;
            WCHAR widePath[MAX_PATH + 1];

            wideCurDir = _PR_MD_A2W(pszCurDir, widePath, sizeof(widePath) / sizeof(WCHAR));
            retval = CreateProcessW(
                wideImageName,
                wideCmdLine,
                psaProcess,
                psaThread,
                fInheritHandles,
                fdwCreate,
                pvEnvironment,
                wideCurDir,
                psiStartInfo,
                pProcInfo);

            if(NULL != wideCmdLine)
            {
                PR_Free(wideCmdLine);
            }
        }
        else
        {
            PR_SetError(PR_OUT_OF_MEMORY_ERROR, 0);
        }

        PR_Free(wideImageName);
    }
    else
    {
        PR_SetError(PR_OUT_OF_MEMORY_ERROR, 0);
    }

    return retval;
}

HANDLE
WINAPI
_MD_CreateFileA(
    LPCSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile
    )
{
    HANDLE retval = (HANDLE)INVALID_HANDLE_VALUE;
    LPWSTR wideStr = NULL;
    WCHAR widePath[MAX_PATH + 1];

    wideStr = _PR_MD_A2W(lpFileName, widePath, sizeof(widePath) / sizeof(WCHAR));
    if(NULL != wideStr)
    {
        retval = CreateFileW(
            wideStr,
            dwDesiredAccess,
            dwShareMode,
            lpSecurityAttributes,
            dwCreationDisposition,
            dwFlagsAndAttributes,
            hTemplateFile
            );
    }
    else
    {
        PR_SetError(PR_NAME_TOO_LONG_ERROR, 0);
    }

    return retval;
}

BOOL
WINAPI
_MD_DeleteFileA(
    LPCSTR lpFileName
    )
{
    BOOL retval = FALSE;
    LPWSTR wideStr = NULL;
    WCHAR widePath[MAX_PATH + 1];

    wideStr = _PR_MD_A2W(lpFileName, widePath, sizeof(widePath) / sizeof(WCHAR));
    if(NULL != wideStr)
    {
        retval = DeleteFileW(
            wideStr
            );
    }
    else
    {
        PR_SetError(PR_NAME_TOO_LONG_ERROR, 0);
    }

    return retval;
}

BOOL
WINAPI
_MD_MoveFileA(
    LPCSTR lpExistingFileName,
    LPCSTR lpNewFileName
    )
{
    BOOL retval = FALSE;
    LPWSTR wideStr = NULL;
    WCHAR widePath[MAX_PATH + 1];
    LPWSTR wideNewStr = NULL;
    WCHAR wideNewPath[MAX_PATH + 1];

    wideStr = _PR_MD_A2W(lpExistingFileName, widePath, sizeof(widePath) / sizeof(WCHAR));
    wideNewStr = _PR_MD_A2W(lpNewFileName, wideNewPath, sizeof(wideNewPath) / sizeof(WCHAR));
    if(NULL != wideStr && NULL != wideNewStr)
    {
        retval = MoveFileW(
            wideStr,
            wideNewStr
            );
    }
    else
    {
        PR_SetError(PR_NAME_TOO_LONG_ERROR, 0);
    }

    return retval;
}

DWORD
WINAPI
_MD_GetFileAttributesA(
    LPCSTR lpFileName
    )
{
    DWORD retval = (DWORD)-1;
    LPWSTR wideStr = NULL;
    WCHAR widePath[MAX_PATH + 1];

    wideStr = _PR_MD_A2W(lpFileName, widePath, sizeof(widePath) / sizeof(WCHAR));
    if(NULL != wideStr)
    {
        retval = GetFileAttributesW(
            wideStr
            );
    }
    else
    {
        PR_SetError(PR_NAME_TOO_LONG_ERROR, 0);
    }

    return retval;
}

BOOL
WINAPI
_MD_CreateDirectoryA(
    LPCSTR lpPathName,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes
    )
{
    BOOL retval = FALSE;
    LPWSTR wideStr = NULL;
    WCHAR widePath[MAX_PATH + 1];

    wideStr = _PR_MD_A2W(lpPathName, widePath, sizeof(widePath) / sizeof(WCHAR));
    if(NULL != wideStr)
    {
        retval = CreateDirectoryW(
            wideStr,
            lpSecurityAttributes
            );
    }
    else
    {
        PR_SetError(PR_NAME_TOO_LONG_ERROR, 0);
    }

    return retval;
}

#endif /* WINCE */