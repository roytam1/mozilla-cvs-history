/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * The contents of this file are subject to the Netscape Public License
 * Version 1.1 (the "NPL"); you may not use this file except in
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

#include "primpl.h"

void _MD_EarlyInit(void)
{
}

PRWord *_MD_HomeGCRegisters(PRThread *t, int isCurrent, int *np)
{
#ifndef _PR_PTHREADS
    if (isCurrent) {
	(void) setjmp(CONTEXT(t));
    }
    *np = sizeof(CONTEXT(t)) / sizeof(PRWord);
    return (PRWord *) CONTEXT(t);
#else
	*np = 0;
	return NULL;
#endif
}

#ifndef _PR_PTHREADS
void
_MD_SET_PRIORITY(_MDThread *thread, PRUintn newPri)
{
    return;
}

PRStatus
_MD_InitializeThread(PRThread *thread)
{
	return PR_SUCCESS;
}

PRStatus
_MD_WAIT(PRThread *thread, PRIntervalTime ticks)
{
    PR_ASSERT(!(thread->flags & _PR_GLOBAL_SCOPE));
    _PR_MD_SWITCH_CONTEXT(thread);
    return PR_SUCCESS;
}

PRStatus
_MD_WAKEUP_WAITER(PRThread *thread)
{
    if (thread) {
	PR_ASSERT(!(thread->flags & _PR_GLOBAL_SCOPE));
    }
    return PR_SUCCESS;
}

/* These functions should not be called for OSF1 */
void
_MD_YIELD(void)
{
    PR_NOT_REACHED("_MD_YIELD should not be called for OSF1.");
}

PRStatus
_MD_CREATE_THREAD(
    PRThread *thread,
    void (*start) (void *),
    PRThreadPriority priority,
    PRThreadScope scope,
    PRThreadState state,
    PRUint32 stackSize)
{
    PR_NOT_REACHED("_MD_CREATE_THREAD should not be called for OSF1.");
	return PR_FAILURE;
}
#endif /* ! _PR_PTHREADS */

#ifdef _PR_HAVE_ATOMIC_CAS

#include <c_asm.h>

#define _PR_OSF_ATOMIC_LOCK 1

void 
PR_StackPush(PRStack *stack, PRStackElem *stack_elem)
{
long locked;

	do {
		while ((long) stack->prstk_head.prstk_elem_next ==
							_PR_OSF_ATOMIC_LOCK)
			;
		locked = __ATOMIC_EXCH_QUAD(&stack->prstk_head.prstk_elem_next,
								_PR_OSF_ATOMIC_LOCK);	

	} while (locked == _PR_OSF_ATOMIC_LOCK);
	stack_elem->prstk_elem_next = (PRStackElem *) locked;
	/*
	 * memory-barrier instruction
	 */
	asm("mb");
	stack->prstk_head.prstk_elem_next = stack_elem;
}

PRStackElem * 
PR_StackPop(PRStack *stack)
{
PRStackElem *element;
long locked;

	do {
		while ((long)stack->prstk_head.prstk_elem_next == _PR_OSF_ATOMIC_LOCK)
			;
		locked = __ATOMIC_EXCH_QUAD(&stack->prstk_head.prstk_elem_next,
								_PR_OSF_ATOMIC_LOCK);	

	} while (locked == _PR_OSF_ATOMIC_LOCK);

	element = (PRStackElem *) locked;

	if (element == NULL) {
		stack->prstk_head.prstk_elem_next = NULL;
	} else {
		stack->prstk_head.prstk_elem_next =
			element->prstk_elem_next;
	}
	/*
	 * memory-barrier instruction
	 */
	asm("mb");
	return element;
}
#endif /* _PR_HAVE_ATOMIC_CAS */


/*
** thread_suspend and thread_resume are used by the gc code
** in nsprpub/pr/src/pthreads/ptthread.c
**
** These routines are never called for the current thread, and
** there is no check for that - so beware!
*/
int thread_suspend(PRThread *thr_id) {

    extern int pthread_suspend_np (
			pthread_t                       thread,
			__pthreadLongUint_t             *regs,
			void                            *spare);

    __pthreadLongUint_t regs[34];
    int res;

    /*
    ** A return res < 0 indicates that the thread was suspended
    ** but register information could not be obtained
    */

    res = pthread_suspend_np(thr_id->id,&regs[0],0);
    if (res==0)
	thr_id->sp = (void *) regs[30]; 

    thr_id->suspend |= PT_THREAD_SUSPENDED;

    /* Always succeeds */
    return 0;
}

int thread_resume(PRThread *thr_id) {
    extern int pthread_resume_np(pthread_t thread);
    int res;

    res = pthread_resume_np (thr_id->id);
	
    thr_id->suspend |= PT_THREAD_RESUMED;

    return 0;
}


#ifdef AS_IS
/*
** These are here because of the problems we have when
** compiling AS_IS. After much trying to fix this problem
** with macro definitions in _pth.h, I finally gave up
** and put these jackets here. So now these two calls both
** map onto their lowercase version, and the lowercase
** version here just calls the uppercase version.
*/

#undef PTHREAD_MUTEX_INIT
#undef PTHREAD_COND_INIT

int pthread_mutex_init (
	pthread_mutex_t *mutex,
	const pthread_mutexattr_t *attr) {
    return PTHREAD_MUTEX_INIT(mutex,attr);
}

int pthread_cond_init (
        pthread_cond_t                  *cond,
        const pthread_condattr_t        *attr){
    return PTHREAD_COND_INIT (cond,attr);
}
#endif
