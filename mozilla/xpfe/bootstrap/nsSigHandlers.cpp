/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 *  Ramiro
 *  Jerry.Kirk@Nexwarecorp.com
 */

/*
 * This module is supposed to abstract signal handling away from the other
 * platforms that do not support it.
 */

#include <signal.h>
#include <stdio.h>
#include "prthread.h"
#include "plstr.h"
#include "prenv.h"

#if defined(LINUX)
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#endif

#ifdef XP_BEOS
#include <string.h>
extern "C" const char * strsignal(int);
#else
extern "C" char * strsignal(int);
#endif

#ifdef NTO
#include <photon/PhProto.h>
#include <sys/mman.h>			/* for munlockall() */
#endif

static char _progname[1024] = "huh?";

//#ifdef DEBUG_ramiro
#if 0
#define CRAWL_STACK_ON_SIGSEGV
#endif // DEBUG_ramiro
 
#ifdef NTO
void abnormal_exit_handler(int signum)
{
  /* Free any shared memory that has been allocated */
  PgShmemCleanup();

#if defined(DEBUG)
  if (    (signum == SIGSEGV)
       || (signum == SIGILL)
	   || (signum == SIGABRT)
	 )
  {
    PR_CurrentThread();
    printf("prog = %s\npid = %d\nsignal = %s\n", 
	  _progname, getpid(), strsignal(signum));

    printf("Sleeping for 5 minutes.\n");
    printf("Type 'gdb %s %d' to attatch your debugger to this thread.\n",
	  _progname, getpid());

    sleep(300);

    printf("Done sleeping...\n");
  }
#endif

  _exit(1);
} 
#elif defined(CRAWL_STACK_ON_SIGSEGV)

#include <unistd.h>
#include "nsTraceRefcnt.h"

void
ah_crap_handler(int signum)
{
  PR_CurrentThread();

  printf("prog = %s\npid = %d\nsignal = %s\n",
         _progname,
         getpid(),
         strsignal(signum));
  
  printf("stack logged to someplace\n");
  nsTraceRefcnt::WalkTheStack(stdout);

  printf("Sleeping for 5 minutes.\n");
  printf("Type 'gdb %s %d' to attatch your debugger to this thread.\n",
         _progname,
         getpid());

  sleep(300);

  printf("Done sleeping...\n");
} 
#endif // CRAWL_STACK_ON_SIGSEGV



void InstallUnixSignalHandlers(const char *ProgramName)
{

  PL_strncpy(_progname,ProgramName, (sizeof(_progname)-1) );

#if defined(NTO)
 /* Neutrino need this to free shared memory in case of a crash */
  signal(SIGTERM, abnormal_exit_handler);
  signal(SIGQUIT, abnormal_exit_handler);
  signal(SIGINT,  abnormal_exit_handler);
  signal(SIGHUP,  abnormal_exit_handler);
  signal(SIGSEGV, abnormal_exit_handler);
  signal(SIGILL,  abnormal_exit_handler);
  signal(SIGABRT, abnormal_exit_handler);

/* Tell the OS it can page any part of this program to virtual memory */
  munlockall();
#elif defined(CRAWL_STACK_ON_SIGSEGV)
  signal(SIGSEGV, ah_crap_handler);
  signal(SIGILL, ah_crap_handler);
  signal(SIGABRT, ah_crap_handler);
#endif // CRAWL_STACK_ON_SIGSEGV

#if defined(DEBUG) && defined(LINUX)
  char *text = PR_GetEnv("MOZ_MEM_LIMIT");
  if (text) 
  {
    long m = atoi(text);
    m *= (1024*1024);    
    struct rlimit r;
    r.rlim_cur = m;
    r.rlim_max = m;
    setrlimit(RLIMIT_AS, &r);
  }
#endif
}
