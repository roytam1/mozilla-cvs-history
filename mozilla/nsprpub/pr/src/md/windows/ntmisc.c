/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
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

/*
 * ntmisc.c
 *
 */

#include "primpl.h"

char *_PR_MD_GET_ENV(const char *name)
{
    return getenv(name);
}

PRIntn _PR_MD_PUT_ENV(const char *name)
{
    return putenv(name);
}


/*
 **************************************************************************
 **************************************************************************
 **
 **     Date and time routines
 **
 **************************************************************************
 **************************************************************************
 */

#include <sys/timeb.h>

/*
 *-----------------------------------------------------------------------
 *
 * PR_Now --
 *
 *     Returns the current time in microseconds since the epoch.
 *     The epoch is midnight January 1, 1970 GMT.
 *     The implementation is machine dependent.  This is the
 *     implementation for Windows.
 *     Cf. time_t time(time_t *tp)
 *
 *-----------------------------------------------------------------------
 */

PRTime
PR_Now(void)
{
    PRInt64 s, ms, ms2us, s2us;
    struct timeb b;

    ftime(&b);
    LL_I2L(ms2us, PR_USEC_PER_MSEC);
    LL_I2L(s2us, PR_USEC_PER_SEC);
    LL_I2L(s, b.time);
    LL_I2L(ms, b.millitm);
    LL_MUL(ms, ms, ms2us);
    LL_MUL(s, s, s2us);
    LL_ADD(s, s, ms);
    return s;       
}

/*
 * The following code works around a bug in NT (Netscape Bugsplat
 * Defect ID 47942).
 *
 * In Windows NT 3.51 and 4.0, if the local time zone does not practice
 * daylight savings time, e.g., Arizona, Taiwan, and Japan, the global
 * variables that _ftime() and localtime() depend on have the wrong
 * default values:
 *     _tzname[0]  "PST"
 *     _tzname[1]  "PDT"
 *     _daylight   1
 *     _timezone   28800
 *
 * So at startup time, we need to invoke _PR_Win32InitTimeZone(), which
 * on NT sets these global variables to the correct values (obtained by
 * calling GetTimeZoneInformation().
 */

#include <time.h>     /* for _tzname, _daylight, _timezone */

void
_PR_Win32InitTimeZone(void)
{
    OSVERSIONINFO version;
    TIME_ZONE_INFORMATION tzinfo;

    version.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if (GetVersionEx(&version) != FALSE) {
        /* Only Windows NT needs this hack */
        if (version.dwPlatformId != VER_PLATFORM_WIN32_NT) {
            return;
        }
    }

    if (GetTimeZoneInformation(&tzinfo) == 0xffffffff) {
        return;  /* not much we can do if this failed */
    }
 
    /* 
     * I feel nervous about modifying these globals.  I hope that no
     * other thread is reading or modifying these globals simultaneously
     * during nspr initialization.
     *
     * I am assuming that _tzname[0] and _tzname[1] point to static buffers
     * and that the buffers are at least 32 byte long.  My experiments show
     * this is true, but of course this is undocumented.  --wtc
     *
     * Convert time zone names from WCHAR to CHAR and copy them to
     * the static buffers pointed to by _tzname[0] and _tzname[1].
     * Ignore conversion errors, because it is _timezone and _daylight
     * that _ftime() and localtime() really depend on.
     */

    WideCharToMultiByte(CP_ACP, 0, tzinfo.StandardName, -1, _tzname[0],
            32, NULL, NULL);
    WideCharToMultiByte(CP_ACP, 0, tzinfo.DaylightName, -1, _tzname[1],
            32, NULL, NULL);

    /* _timezone is in seconds.  tzinfo.Bias is in minutes. */

    _timezone = tzinfo.Bias * 60;
    _daylight = tzinfo.DaylightBias ? 1 : 0;
    return;
}

/*
 ***********************************************************************
 ***********************************************************************
 *
 * Process creation routines
 *
 ***********************************************************************
 ***********************************************************************
 */

/*
 * Assemble the command line by concatenating the argv array.
 * On success, this function returns 0 and the resulting command
 * line is returned in *cmdLine.  On failure, it returns -1.
 */
static int assembleCmdLine(char *const *argv, char **cmdLine)
{
    char *const *arg;
    char *p, *q;
    int cmdLineSize;
    int numBackslashes;
    int i;
    int argNeedQuotes;

    /*
     * Find out how large the command line buffer should be.
     */
    cmdLineSize = 0;
    for (arg = argv; *arg; arg++) {
        /*
         * \ and " need to be escaped by a \.  In the worst case,
         * every character is a \ or ", so the string of length
         * may double.  If we quote an argument, that needs two ".
         * Finally, we need a space between arguments, and
         * a null byte at the end of command line.
         */
        cmdLineSize += 2 * strlen(*arg)  /* \ and " need to be escaped */
                + 2                      /* we quote every argument */
                + 1;                     /* space in between, or final null */
    }
    p = *cmdLine = PR_MALLOC(cmdLineSize);
    if (p == NULL) {
        return -1;
    }

    for (arg = argv; *arg; arg++) {
        /* Add a space to separates the arguments */
        if (arg != argv) {
            *p++ = ' '; 
        }
        q = *arg;
        numBackslashes = 0;
        argNeedQuotes = 0;

        /* If the argument contains white space, it needs to be quoted. */
        if (strpbrk(*arg, " \f\n\r\t\v")) {
            argNeedQuotes = 1;
        }

        if (argNeedQuotes) {
            *p++ = '"';
        }
        while (*q) {
            if (*q == '\\') {
                numBackslashes++;
                q++;
            } else if (*q == '"') {
                if (numBackslashes) {
                    /*
                     * Double the backslashes since they are followed
                     * by a quote
                     */
                    for (i = 0; i < 2 * numBackslashes; i++) {
                        *p++ = '\\';
                    }
                    numBackslashes = 0;
                }
                /* To escape the quote */
                *p++ = '\\';
                *p++ = *q++;
            } else {
                if (numBackslashes) {
                    /*
                     * Backslashes are not followed by a quote, so
                     * don't need to double the backslashes.
                     */
                    for (i = 0; i < numBackslashes; i++) {
                        *p++ = '\\';
                    }
                    numBackslashes = 0;
                }
                *p++ = *q++;
            }
        }

        /* Now we are at the end of this argument */
        if (numBackslashes) {
            /*
             * Double the backslashes if we have a quote string
             * delimiter at the end.
             */
            if (argNeedQuotes) {
                numBackslashes *= 2;
            }
            for (i = 0; i < numBackslashes; i++) {
                *p++ = '\\';
            }
        }
        if (argNeedQuotes) {
            *p++ = '"';
        }
    } 

    *p = '\0';
    return 0;
}

/*
 * Assemble the environment block by concatenating the envp array
 * (preserving the terminating null byte in each array element)
 * and adding a null byte at the end.
 *
 * Returns 0 on success.  The resulting environment block is returned
 * in *envBlock.  Note that if envp is NULL, a NULL pointer is returned
 * in *envBlock.  Returns -1 on failure.
 */
static int assembleEnvBlock(char **envp, char **envBlock)
{
    char *p;
    char *q;
    char **env;
    char *curEnv;
    char *cwdStart, *cwdEnd;
    int envBlockSize;

    if (envp == NULL) {
        *envBlock = NULL;
        return 0;
    }

    curEnv = GetEnvironmentStrings();

    cwdStart = curEnv;
    while (*cwdStart) {
        if (cwdStart[0] == '=' && cwdStart[1] != '\0'
                && cwdStart[2] == ':' && cwdStart[3] == '=') {
            break;
        }
        cwdStart += strlen(cwdStart) + 1;
    }
    cwdEnd = cwdStart;
    if (*cwdEnd) {
        cwdEnd += strlen(cwdEnd) + 1;
        while (*cwdEnd) {
            if (cwdEnd[0] != '=' || cwdEnd[1] == '\0'
                    || cwdEnd[2] != ':' || cwdEnd[3] != '=') {
                break;
            }
            cwdEnd += strlen(cwdEnd) + 1;
        }
    }
    envBlockSize = cwdEnd - cwdStart;

    for (env = envp; *env; env++) {
        envBlockSize += strlen(*env) + 1;
    }
    envBlockSize++;

    p = *envBlock = PR_MALLOC(envBlockSize);
    if (p == NULL) {
        FreeEnvironmentStrings(curEnv);
        return -1;
    }

    q = cwdStart;
    while (q < cwdEnd) {
        *p++ = *q++;
    }
    FreeEnvironmentStrings(curEnv);

    for (env = envp; *env; env++) {
        q = *env;
        while (*q) {
            *p++ = *q++;
        }
        *p++ = '\0';
    }
    *p = '\0';
    return 0;
}

/*
 * For qsort.  We sort (case-insensitive) the environment strings
 * before generating the environment block.
 */
static int compare(const void *arg1, const void *arg2)
{
    return _stricmp(* (char**)arg1, * (char**)arg2);
}

PRProcess * _PR_CreateWindowsProcess(
    const char *path,
    char *const *argv,
    char *const *envp,
    const PRProcessAttr *attr)
{
    STARTUPINFO startupInfo;
    PROCESS_INFORMATION procInfo;
    BOOL retVal;
    char *cmdLine = NULL;
    char *envBlock = NULL;
    char **newEnvp;
    PRProcess *proc = NULL;

    proc = PR_NEW(PRProcess);
    if (!proc) {
        PR_SetError(PR_OUT_OF_MEMORY_ERROR, 0);
        goto errorExit;
    }

    if (assembleCmdLine(argv, &cmdLine) == -1) {
        PR_SetError(PR_OUT_OF_MEMORY_ERROR, 0);
        goto errorExit;
    }

    if (envp == NULL) {
        newEnvp = NULL;
    } else {
        int i;
        int numEnv = 0;
        while (envp[numEnv]) {
            numEnv++;
        }
        newEnvp = (char **) PR_MALLOC((numEnv+1) * sizeof(char *));
        for (i = 0; i <= numEnv; i++) {
            newEnvp[i] = envp[i];
        }
        qsort((void *) newEnvp, (size_t) numEnv, sizeof(char *), compare);
    }
    if (assembleEnvBlock(newEnvp, &envBlock) == -1) {
        PR_SetError(PR_OUT_OF_MEMORY_ERROR, 0);
        goto errorExit;
    }

    ZeroMemory(&startupInfo, sizeof(startupInfo));
    startupInfo.cb = sizeof(startupInfo);

    if (attr) {
        PRBool redirected = PR_FALSE;

        /*
         * XXX the default value for stdin, stdout, and stderr
         * should probably be the console input and output, not
         * those of the parent process.
         */
        startupInfo.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
        startupInfo.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
        startupInfo.hStdError = GetStdHandle(STD_ERROR_HANDLE);
        if (attr->stdinFd) {
            startupInfo.hStdInput = (HANDLE) attr->stdinFd->secret->md.osfd;
            redirected = PR_TRUE;
        }
        if (attr->stdoutFd) {
            startupInfo.hStdOutput = (HANDLE) attr->stdoutFd->secret->md.osfd;
            redirected = PR_TRUE;
        }
        if (attr->stderrFd) {
            startupInfo.hStdError = (HANDLE) attr->stderrFd->secret->md.osfd;
            redirected = PR_TRUE;
        }
        if (redirected) {
            startupInfo.dwFlags |= STARTF_USESTDHANDLES;
        }
    }

    retVal = CreateProcess(NULL,
                           cmdLine,
                           NULL,  /* security attributes for the new
                                   * process */
                           NULL,  /* security attributes for the primary
                                   * thread in the new process */
                           TRUE,  /* inherit handles */
                           0,     /* creation flags */
                           envBlock,  /* an environment block, consisting
                                       * of a null-terminated block of
                                       * null-terminated strings.  Each
                                       * string is in the form:
                                       *     name=value
                                       * XXX: usually NULL */
                           NULL,  /* current drive and directory */
                           &startupInfo,
                           &procInfo
                          );
    if (retVal == FALSE) {
        /* XXX what error code? */
        PR_SetError(PR_UNKNOWN_ERROR, GetLastError());
        goto errorExit;
    }

    CloseHandle(procInfo.hThread);
    proc->md.handle = procInfo.hProcess;
    proc->md.id = procInfo.dwProcessId;

    PR_DELETE(cmdLine);
    if (envBlock) {
        PR_DELETE(envBlock);
    }
    return proc;

errorExit:
    if (cmdLine) {
        PR_DELETE(cmdLine);
    }
    if (envBlock) {
        PR_DELETE(envBlock);
    }
    if (proc) {
        PR_DELETE(proc);
    }
    return NULL;
}  /* _PR_CreateWindowsProcess */

PRStatus _PR_DetachWindowsProcess(PRProcess *process)
{
    CloseHandle(process->md.handle);
    PR_DELETE(process);
    return PR_SUCCESS;
}

/*
 * XXX: This implementation is a temporary quick solution.
 * It can be called by native threads only (not by fibers).
 */
PRStatus _PR_WaitWindowsProcess(PRProcess *process,
    PRInt32 *exitCode)
{
    DWORD dwRetVal;

    dwRetVal = WaitForSingleObject(process->md.handle, INFINITE);
    if (dwRetVal == WAIT_FAILED) {
        PR_SetError(PR_UNKNOWN_ERROR, GetLastError());
        return PR_FAILURE;
    }
    PR_ASSERT(dwRetVal == WAIT_OBJECT_0);
    if (exitCode != NULL &&
            GetExitCodeProcess(process->md.handle, exitCode) == FALSE) {
        PR_SetError(PR_UNKNOWN_ERROR, GetLastError());
        return PR_FAILURE;
    }
    CloseHandle(process->md.handle);
    PR_DELETE(process);
    return PR_SUCCESS;
}

PRStatus _PR_KillWindowsProcess(PRProcess *process)
{
    /*
     * On Unix, if a process terminates normally, its exit code is
     * between 0 and 255.  So here on Windows, we use the exit code
     * 256 to indicate that the process is killed.
     */
    if (TerminateProcess(process->md.handle, 256)) {
	return PR_SUCCESS;
    }
    PR_SetError(PR_UNKNOWN_ERROR, GetLastError());
    return PR_FAILURE;
}

PRStatus _MD_WindowsGetHostName(char *name, PRUint32 namelen)
{
    PRIntn rv;
    PRInt32 syserror;

    rv = gethostname(name, (PRInt32) namelen);
    if (0 == rv) {
        return PR_SUCCESS;
    }
    syserror = WSAGetLastError();
    PR_ASSERT(WSANOTINITIALISED != syserror);
	_PR_MD_MAP_GETHOSTNAME_ERROR(syserror);
    return PR_FAILURE;
}

/*
 **********************************************************************
 *
 * Memory-mapped files
 *
 **********************************************************************
 */

PRStatus _MD_CreateFileMap(PRFileMap *fmap, PRInt64 size)
{
    DWORD dwHi, dwLo;
    DWORD flProtect;

    dwLo = (DWORD) (size & 0xffffffff);
    dwHi = (DWORD) (((PRUint64) size >> 32) & 0xffffffff);

    if (fmap->prot == PR_PROT_READONLY) {
        flProtect = PAGE_READONLY;
        fmap->md.dwAccess = FILE_MAP_READ;
    } else if (fmap->prot == PR_PROT_READWRITE) {
        flProtect = PAGE_READWRITE;
        fmap->md.dwAccess = FILE_MAP_WRITE;
    } else {
        PR_ASSERT(fmap->prot == PR_PROT_WRITECOPY);
        flProtect = PAGE_WRITECOPY;
        fmap->md.dwAccess = FILE_MAP_COPY;
    }

    fmap->md.hFileMap = CreateFileMapping(
        (HANDLE) fmap->fd->secret->md.osfd,
        NULL,
        flProtect,
        dwHi,
        dwLo,
        NULL);

    if (fmap->md.hFileMap == NULL) {
        PR_SetError(PR_UNKNOWN_ERROR, GetLastError());
        return PR_FAILURE;
    }
    return PR_SUCCESS;
}

void * _MD_MemMap(
    PRFileMap *fmap,
    PRInt64 offset,
    PRUint32 len)
{
    DWORD dwHi, dwLo;
    void *addr;

    dwLo = (DWORD) (offset & 0xffffffff);
    dwHi = (DWORD) (((PRUint64) offset >> 32) & 0xffffffff);
    if ((addr = MapViewOfFile(fmap->md.hFileMap, fmap->md.dwAccess,
            dwHi, dwLo, len)) == NULL) {
        PR_SetError(PR_UNKNOWN_ERROR, GetLastError());
    }
    return addr;
}

PRStatus _MD_MemUnmap(void *addr, PRUint32 len)
{
    if (UnmapViewOfFile(addr)) {
        return PR_SUCCESS;
    } else {
        PR_SetError(PR_UNKNOWN_ERROR, GetLastError());
        return PR_FAILURE;
    }
}

PRStatus _MD_CloseFileMap(PRFileMap *fmap)
{
    CloseHandle(fmap->md.hFileMap);
    PR_DELETE(fmap);
    return PR_SUCCESS;
}
