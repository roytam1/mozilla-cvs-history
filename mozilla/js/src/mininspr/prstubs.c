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
 * This file contains stubs for some functions that are normally implemented
 * in NSPR, but which are not necessary in the single-threaded mini-NSPR.
 * (mini-NSPR is the NSPR subset required to run JS).
 * There is no peer to this file in the full-blown version of NSPR.
 */
#include "prstubs.h"
#include <stdlib.h>
#include <stdio.h>

PRBool _pr_initialized;

/* Stubbed-out functions */
void _PR_ImplicitInitialization(void)
{
    _pr_initialized = PR_TRUE;
}

/* Implementation of PR_Assert() is here to avoid building prlog.c */
PR_IMPLEMENT(void) PR_Assert(const char *s, const char *file, PRIntn ln)
{
#if defined(XP_UNIX) || defined(XP_OS2)
    fprintf(stderr, "Assertion failure: %s, at %s:%d\n", s, file, ln);
#endif
#ifdef XP_MAC
    dprintf("Assertion failure: %s, at %s:%d\n", s, file, ln);
#endif
#ifdef WIN32
    DebugBreak();
#endif
#ifndef XP_MAC
    abort();
#endif
}

