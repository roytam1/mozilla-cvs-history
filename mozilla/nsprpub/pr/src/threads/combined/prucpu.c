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

#include "primpl.h"

_PRCPU *_pr_primordialCPU = NULL;

PRInt32 _pr_md_idle_cpus;       /* number of idle cpus */
/*
 * The idle threads in MxN models increment/decrement _pr_md_idle_cpus.
 * If _PR_HAVE_ATOMIC_OPS is not defined, they can't use the atomic
 * increment/decrement routines (which are based on PR_Lock/PR_Unlock),
 * because PR_Lock asserts that the calling thread is not an idle thread.
 * So we use a _MDLock to protect _pr_md_idle_cpus.
 */
#if !defined(_PR_LOCAL_THREADS_ONLY) && !defined(_PR_GLOBAL_THREADS_ONLY)
#ifndef _PR_HAVE_ATOMIC_OPS
static _MDLock _pr_md_idle_cpus_lock;
#endif
#endif
PRUintn _pr_numCPU;
PRInt32 _pr_cpus_exit;
PRInt32 _pr_cpu_affinity_mask = 0;

#if !defined (_PR_GLOBAL_THREADS_ONLY)

static PRUintn _pr_cpuID;

static void PR_CALLBACK _PR_CPU_Idle(void *);

static _PRCPU *_PR_CreateCPU(PRThread *thread, PRBool needQueue);

#if !defined(_PR_LOCAL_THREADS_ONLY)
static void _PR_RunCPU(void *arg);
#endif

void  _PR_InitCPUs()
{
    PRThread *me = _PR_MD_CURRENT_THREAD();

    _pr_cpuID = 0;
    _MD_NEW_LOCK( &_pr_cpuLock);
#if !defined(_PR_LOCAL_THREADS_ONLY) && !defined(_PR_GLOBAL_THREADS_ONLY)
#ifndef _PR_HAVE_ATOMIC_OPS
    _MD_NEW_LOCK(&_pr_md_idle_cpus_lock);
#endif
#endif

#ifdef _PR_LOCAL_THREADS_ONLY

#ifdef HAVE_CUSTOM_USER_THREADS
	if (!_native_threads_only)
    	_PR_MD_CREATE_PRIMORDIAL_USER_THREAD(me);
#endif

    /* Now start the first CPU. */
    _pr_primordialCPU = _PR_CreateCPU(me, PR_TRUE);
    _pr_numCPU = 1;

    _PR_MD_SET_CURRENT_CPU(_pr_primordialCPU);

    /* Initialize cpu for current thread (could be different from me) */
    _PR_MD_CURRENT_THREAD()->cpu = _pr_primordialCPU;

    _PR_MD_SET_LAST_THREAD(me);

#else /* Combined MxN model */

    _PR_CreateThread(PR_SYSTEM_THREAD,
                     _PR_RunCPU,
                     me,
                     PR_PRIORITY_NORMAL,
                     PR_GLOBAL_THREAD,
                     PR_UNJOINABLE_THREAD,
                     0,
                     _PR_IDLE_THREAD);
    _PR_MD_WAIT(me, PR_INTERVAL_NO_TIMEOUT);

#endif /* _PR_LOCAL_THREADS_ONLY */

    _PR_MD_INIT_CPUS();
}


static _PRCPUQueue *_PR_CreateCPUQueue(void)
{
    PRInt32 index;
    _PRCPUQueue *cpuQueue;
    cpuQueue = PR_NEWZAP(_PRCPUQueue);
 
    _MD_NEW_LOCK( &cpuQueue->runQLock );
    _MD_NEW_LOCK( &cpuQueue->sleepQLock );
    _MD_NEW_LOCK( &cpuQueue->miscQLock );

    for (index = 0; index < PR_PRIORITY_LAST + 1; index++)
        PR_INIT_CLIST( &(cpuQueue->runQ[index]) );
    PR_INIT_CLIST( &(cpuQueue->sleepQ) );
    PR_INIT_CLIST( &(cpuQueue->pauseQ) );
    PR_INIT_CLIST( &(cpuQueue->suspendQ) );
    PR_INIT_CLIST( &(cpuQueue->waitingToJoinQ) );

    cpuQueue->numCPUs = 1;

    return cpuQueue;
}

/*
 * Create a new CPU.
 */
static _PRCPU *_PR_CreateCPU(PRThread *thread, PRBool needQueue)
{
    _PRCPU *cpu;

    /*
    ** Create a new cpu. The assumption this code makes is that the
    ** underlying operating system creates a stack to go with the new
    ** native thread. That stack will be used by the cpu when pausing.
    */
    cpu = PR_NEWZAP(_PRCPU);
    if (cpu) {
        cpu->last_clock = PR_IntervalNow();

        if (needQueue == PR_TRUE)
            cpu->queue = _PR_CreateCPUQueue();
        else 
            cpu->queue = _PR_MD_CURRENT_CPU()->queue;
   
        if (!cpu->queue) {
            PR_DELETE(cpu);
            return NULL;
        }

        /* Before we create any threads on this CPU we have to
         * set the current CPU 
         */
        _PR_MD_SET_CURRENT_CPU(cpu);
        _PR_MD_INIT_RUNNING_CPU(cpu);
        thread->cpu = cpu;

		if (!_native_threads_only) {
			
			cpu->idle_thread = _PR_CreateThread(PR_SYSTEM_THREAD,
											   _PR_CPU_Idle,
											   (void *)cpu,
											   PR_PRIORITY_NORMAL,
											   PR_LOCAL_THREAD,
											   PR_UNJOINABLE_THREAD,
											   0,
											   _PR_IDLE_THREAD);

			if (!cpu->idle_thread) {
				/* didn't clean up CPU queue XXXMB */
				PR_DELETE(cpu);
				return NULL;
			} 
			cpu->idle_thread->cpu = cpu;

			cpu->idle_thread->no_sched = 0;
		}

        cpu->thread = thread;

        if (_pr_cpu_affinity_mask)
            PR_SetThreadAffinityMask(thread, _pr_cpu_affinity_mask);

	    /* Created a new CPU */
	_PR_CPU_LIST_LOCK();
        cpu->id = _pr_cpuID++;
        PR_APPEND_LINK(&cpu->links, &_PR_CPUQ());
	_PR_CPU_LIST_UNLOCK();
   }
    return cpu;
}

#if !defined(_PR_GLOBAL_THREADS_ONLY) && !defined(_PR_LOCAL_THREADS_ONLY)
/*
** This code is used during a cpu's initial creation.
*/
static void _PR_RunCPU(void *arg)
{
    _PRCPU *cpu;
    PRThread *me = _PR_MD_CURRENT_THREAD();
    PRThread *waiter = (PRThread *) arg;

    PR_ASSERT(NULL != me);

    /*
     * _PR_CreateCPU calls _PR_CreateThread to create the
     * idle thread.  Because _PR_CreateThread calls PR_Lock,
     * the current thread has to remain a global thread
     * during the _PR_CreateCPU call so that it can wait for
     * the lock if the lock is held by another thread.  If
     * we clear the _PR_GLOBAL_SCOPE flag in
     * _PR_MD_CREATE_PRIMORDIAL_THREAD, the current thread
     * will be treated as a local thread and have trouble
     * waiting for the lock because the CPU is not fully
     * constructed yet.
     *
     * After the CPU is created, it is safe to mark the
     * current thread as a local thread.
     */

#ifdef HAVE_CUSTOM_USER_THREADS
    _PR_MD_CREATE_PRIMORDIAL_USER_THREAD(me);
#endif

    me->no_sched = 1;
    cpu = _PR_CreateCPU(me, PR_TRUE);

#ifdef HAVE_CUSTOM_USER_THREADS
    me->flags &= (~_PR_GLOBAL_SCOPE);
#endif

    _PR_MD_SET_CURRENT_CPU(cpu);
    _PR_MD_SET_CURRENT_THREAD(cpu->thread);
    me->cpu = cpu;

    if (waiter) {
        _pr_primordialCPU = cpu;
        _pr_numCPU = 1;
        _PR_MD_WAKEUP_WAITER(waiter);
    }

    while(1) {
        PRInt32 is;
        if (!_PR_IS_NATIVE_THREAD(me)) _PR_INTSOFF(is);
	    _PR_MD_START_INTERRUPTS();
        _PR_MD_SWITCH_CONTEXT(me);
    }
}
#endif

static void PR_CALLBACK _PR_CPU_Idle(void *_cpu)
{
    _PRCPU *cpu = (_PRCPU *)_cpu;
    PRThread *me = _PR_MD_CURRENT_THREAD();

    PR_ASSERT(NULL != me);

    me->cpu = cpu;
    cpu->idle_thread = me;
    if (_MD_LAST_THREAD())
        _MD_LAST_THREAD()->no_sched = 0;
    if (!_PR_IS_NATIVE_THREAD(me)) _PR_MD_SET_INTSOFF(0);
    while(1) {
        PRInt32 is;
        PRIntervalTime timeout;
        if (!_PR_IS_NATIVE_THREAD(me)) _PR_INTSOFF(is);

        _PR_RUNQ_LOCK(cpu);
#if !defined(_PR_LOCAL_THREADS_ONLY) && !defined(_PR_GLOBAL_THREADS_ONLY)
#ifdef _PR_HAVE_ATOMIC_OPS
        _PR_MD_ATOMIC_INCREMENT(&_pr_md_idle_cpus);
#else
        _PR_MD_LOCK(&_pr_md_idle_cpus_lock);
        _pr_md_idle_cpus++;
        _PR_MD_UNLOCK(&_pr_md_idle_cpus_lock);
#endif /* _PR_HAVE_ATOMIC_OPS */
#endif
        /* If someone on runq; do a nonblocking PAUSECPU */
        if (_PR_RUNQREADYMASK(me->cpu) != 0) {
            _PR_RUNQ_UNLOCK(cpu);
            timeout = PR_INTERVAL_NO_WAIT;
        } else {
            _PR_RUNQ_UNLOCK(cpu);

            _PR_SLEEPQ_LOCK(cpu);
            if (PR_CLIST_IS_EMPTY(&_PR_SLEEPQ(me->cpu))) {
                timeout = PR_INTERVAL_NO_TIMEOUT;
            } else {
                PRThread *wakeThread;
                wakeThread = _PR_THREAD_PTR(_PR_SLEEPQ(me->cpu).next);
                timeout = wakeThread->sleep;
            }
            _PR_SLEEPQ_UNLOCK(cpu);
        }

        /* Wait for an IO to complete */
        (void)_PR_MD_PAUSE_CPU(timeout);


#if !defined(_PR_LOCAL_THREADS_ONLY) && !defined(_PR_GLOBAL_THREADS_ONLY)
#ifdef _PR_HAVE_ATOMIC_OPS
        _PR_MD_ATOMIC_DECREMENT(&_pr_md_idle_cpus);
#else
        _PR_MD_LOCK(&_pr_md_idle_cpus_lock);
        _pr_md_idle_cpus--;
        _PR_MD_UNLOCK(&_pr_md_idle_cpus_lock);
#endif /* _PR_HAVE_ATOMIC_OPS */
#endif

		_PR_ClockInterrupt();

		/* Now schedule any thread that is on the runq
		 * INTS must be OFF when calling PR_Schedule()
		 */
		me->state = _PR_RUNNABLE;
		_PR_MD_SWITCH_CONTEXT(me);
		if (!_PR_IS_NATIVE_THREAD(me)) _PR_FAST_INTSON(is);
    }
}
#endif /* _PR_GLOBAL_THREADS_ONLY */

PR_IMPLEMENT(void) PR_SetConcurrency(PRUintn numCPUs)
{
#if defined(_PR_GLOBAL_THREADS_ONLY) || defined(_PR_LOCAL_THREADS_ONLY)
#ifdef XP_MAC 
#pragma unused(numCPUs) 
#endif

    /* do nothing */

#else /* combined, MxN thread model */

    PRUintn newCPU;
    PRThread *cpu;


    if (!_pr_initialized) _PR_ImplicitInitialization();

	if (_native_threads_only)
		return;
    
    _PR_CPU_LIST_LOCK();
    if (_pr_numCPU < numCPUs) {
        newCPU = numCPUs - _pr_numCPU;
        _pr_numCPU = numCPUs;
    } else newCPU = 0;
    _PR_CPU_LIST_UNLOCK();

    for (; newCPU; newCPU--) {
        cpu = _PR_CreateThread(PR_SYSTEM_THREAD,
                              _PR_RunCPU,
                              NULL,
                              PR_PRIORITY_NORMAL,
                              PR_GLOBAL_THREAD,
                              PR_UNJOINABLE_THREAD,
                              0,
                              _PR_IDLE_THREAD);
    }
#endif
}

PR_IMPLEMENT(_PRCPU *) _PR_GetPrimordialCPU(void)
{
    if (_pr_primordialCPU)
        return _pr_primordialCPU;
    else
        return _PR_MD_CURRENT_CPU();
}
