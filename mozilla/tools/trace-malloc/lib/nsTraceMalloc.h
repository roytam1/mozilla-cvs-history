/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express oqr
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is nsTraceMalloc.c/bloatblame.c code, released
 * April 19, 2000.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 2000 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s):
 *    Brendan Eich, 14-April-2000
 *
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU Public License (the "GPL"), in which case the
 * provisions of the GPL are applicable instead of those above.
 * If you wish to allow use of your version of this file only
 * under the terms of the GPL and not to allow others to use your
 * version of this file under the MPL, indicate your decision by
 * deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL.  If you do not delete
 * the provisions above, a recipient may use your version of this
 * file under either the MPL or the GPL.
 */
#ifndef nsTraceMalloc_h___
#define nsTraceMalloc_h___

#include "prtypes.h"

PR_BEGIN_EXTERN_C

/**
 * Magic "number" at start of a trace-malloc log file.  Inspired by the PNG
 * magic string, which inspired XPCOM's typelib (.xpt) file magic.  See the
 * NS_TraceMalloc comment for magic number differences in log file structure.
 */
#define NS_TRACE_MALLOC_MAGIC           "XPCOM\nTMLog02\r\n\032"
#define NS_TRACE_MALLOC_MAGIC_SIZE      16

/**
 * Trace-malloc stats, traced via the 'Z' event at the end of a log file.
 */
struct nsTMStats {
    uint32 calltree_maxstack;
    uint32 calltree_maxdepth;
    uint32 calltree_parents;
    uint32 calltree_maxkids;
    uint32 calltree_kidhits;
    uint32 calltree_kidmisses;
    uint32 calltree_kidsteps;
    uint32 callsite_recurrences;
    uint32 backtrace_calls;
    uint32 backtrace_failures;
    uint32 btmalloc_failures;
    uint32 dladdr_failures;
    uint32 malloc_calls;
    uint32 malloc_failures;
    uint32 calloc_calls;
    uint32 calloc_failures;
    uint32 realloc_calls;
    uint32 realloc_failures;
    uint32 free_calls;
    uint32 null_free_calls;
};

#define NS_TMSTATS_STATIC_INITIALIZER {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}

/**
 * Call NS_TraceMalloc with a valid log file descriptor to enable logging
 * of compressed malloc traces, including callsite chains.  Integers may be
 * unsigned serial numbers, sizes, or offsets, and require at most 32 bits.
 * They're encoded as follows:
 *   0-127                  0xxxxxxx (binary, one byte)
 *   128-16383              10xxxxxx xxxxxxxx
 *   16384-0x1fffff         110xxxxx xxxxxxxx xxxxxxxx
 *   0x200000-0xfffffff     1110xxxx xxxxxxxx xxxxxxxx xxxxxxxx
 *   0x10000000-0xffffffff  11110000 xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx
 * Strings are NUL-terminated ASCII.
 *
 * Event Operands (magic TMLog01)
 *   'L' library serial, shared object filename string
 *   'N' method serial, library serial, demangled name string
 *   'S' site serial, parent serial, method serial, calling pc offset
 *   'M' site serial, malloc size
 *   'C' site serial, calloc size
 *   'R' site serial, realloc oldsize, realloc size
 *   'F' site serial, free size
 *
 * Event Operands (magic TMLog02)
 *   'Z' serialized struct tmstats (20 unsigned integers),
 *       maxkids parent callsite serial,
 *       maxstack top callsite serial
 *
 * See xpcom/base/bloatblame.c for an example log-file reader.
 */
PR_EXTERN(void) NS_TraceMallocStartup(int logfd, int sitefd);

/**
 * Stop all malloc tracing, flushing any buffered events to the logfile.
 */
PR_EXTERN(void) NS_TraceMallocShutdown(void);

/**
 * Disable malloc tracing.
 */
PR_EXTERN(void) NS_TraceMallocDisable(void);

/**
 * Enable malloc tracing.
 */
PR_EXTERN(void) NS_TraceMallocEnable(void);

/**
 * Change the log file descriptor, flushing any buffered output to the old
 * fd, and writing NS_TRACE_MALLOC_MAGIC to the new file if it is zero length.
 * Return the old fd, so the caller can swap open fds.  Return -2 on failure,
 * which means malloc failure.
 */
PR_EXTERN(int) NS_TraceMallocChangeLogFD(int fd);

PR_END_EXTERN_C

#endif /* nsTraceMalloc_h___ */
