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

/*
 * This file implements _PR_MD_PR_POLL for OS/2.
 */

#ifdef XP_OS2_EMX
	#include <sys/time.h> /* For timeval. */
#endif

#include "primpl.h"

PRInt32 _PR_MD_PR_POLL(
    PRPollDesc *pds, PRIntn npds, PRIntervalTime timeout)
{
    PRInt32 osfd;
    int maxfd = -1;
    fd_set rd, wt, ex;
    PRFileDesc *bottom;
    PRPollDesc *pd, *epd;
    PRInt32 ready, err;
	PRThread *me = _PR_MD_CURRENT_THREAD();
    struct timeval tv, *tvp = NULL;

    /*
     * For restarting _MD_SELECT() if it is interrupted by a signal.
     * We use these variables to figure out how much time has elapsed
     * and how much of the timeout still remains.
     */
    PRIntervalTime start, elapsed, remaining;

    if (_PR_PENDING_INTERRUPT(me))
    {
        me->flags &= ~_PR_INTERRUPT;
        PR_SetError(PR_PENDING_INTERRUPT_ERROR, 0);
        return -1;
    }

    /*
    ** Is it an empty set? If so, just sleep for the timeout and return
    */
    if (0 == npds)
    {
        PR_Sleep(timeout);
        return 0;
    }

    remaining = timeout;
    start = PR_IntervalNow();

  	FD_ZERO(&rd);
  	FD_ZERO(&wt);
    FD_ZERO(&ex);

	ready = 0;
    for (pd = pds, epd = pd + npds; pd < epd; pd++)
    {
        PRInt16 in_flags_read = 0, in_flags_write = 0;
        PRInt16 out_flags_read = 0, out_flags_write = 0;

        if ((NULL != pd->fd) && (0 != pd->in_flags))
        {
            if (pd->in_flags & PR_POLL_READ)
            {
                in_flags_read = (pd->fd->methods->poll)(
                    pd->fd, (PRInt16)(pd->in_flags & ~PR_POLL_WRITE),
                    &out_flags_read);
            }
            if (pd->in_flags & PR_POLL_WRITE)
            {
                in_flags_write = (pd->fd->methods->poll)(
                    pd->fd, (PRInt16)(pd->in_flags & ~PR_POLL_READ),
                    &out_flags_write);
            }
            if ((0 != (in_flags_read & out_flags_read))
            || (0 != (in_flags_write & out_flags_write)))
            {
                /* this one's ready right now (buffered input) */
                if (0 == ready)
                {
                    /*
                     * We will have to return without calling the
                     * system poll/select function.  So zero the
                     * out_flags fields of all the poll descriptors
                     * before this one.
                     */
                    PRPollDesc *prev;
                    for (prev = pds; prev < pd; prev++)
                    {
                        prev->out_flags = 0;
                    }
                }
                ready += 1;
                pd->out_flags = out_flags_read | out_flags_write;
            }
            else
            {
                pd->out_flags = 0;  /* pre-condition */
                /* make sure this is an NSPR supported stack */
                bottom = PR_GetIdentitiesLayer(pd->fd, PR_NSPR_IO_LAYER);
                PR_ASSERT(NULL != bottom);  /* what to do about that? */
                if ((NULL != bottom)
                && (_PR_FILEDESC_OPEN == bottom->secret->state))
                {
                    if (0 == ready)
                    {
                        osfd = bottom->secret->md.osfd;
    	                if (osfd > maxfd) maxfd = osfd;
                        if (in_flags_read & PR_POLL_READ)
                        {
                            pd->out_flags |= _PR_POLL_READ_SYS_READ;
                            FD_SET(osfd, &rd);
                        }
                        if (in_flags_read & PR_POLL_WRITE)
                        {
                            pd->out_flags |= _PR_POLL_READ_SYS_WRITE;
                            FD_SET(osfd, &wt);
                        }
                        if (in_flags_write & PR_POLL_READ)
                        {
                            pd->out_flags |= _PR_POLL_WRITE_SYS_READ;
                            FD_SET(osfd, &rd);
                        }
                        if (in_flags_write & PR_POLL_WRITE)
                        {
                            pd->out_flags |= _PR_POLL_WRITE_SYS_WRITE;
                            FD_SET(osfd, &wt);
                        }
                        if (pd->in_flags & PR_POLL_EXCEPT) FD_SET(osfd, &ex);
                    }
                }
                else
                {
                    if (0 == ready)
                    {
                        PRPollDesc *prev;
                        for (prev = pds; prev < pd; prev++)
                        {
                            prev->out_flags = 0;
                        }
                    }
                    ready += 1;  /* this will cause an abrupt return */
                    pd->out_flags = PR_POLL_NVAL;  /* bogii */
                }
            }
        }
    }

    if (0 != ready) return ready;  /* no need to block */

retry:
    if (timeout != PR_INTERVAL_NO_TIMEOUT)
    {
        PRInt32 ticksPerSecond = PR_TicksPerSecond();
        tv.tv_sec = remaining / ticksPerSecond;
        tv.tv_usec = remaining - (ticksPerSecond * tv.tv_sec);
        tv.tv_usec = (PR_USEC_PER_SEC * tv.tv_usec) / ticksPerSecond;
        tvp = &tv;
    }

    ready = _MD_SELECT(maxfd + 1, &rd, &wt, &ex, tvp);
    if (ready == -1 && errno == EINTR)
    {
        if (timeout == PR_INTERVAL_NO_TIMEOUT) goto retry;
     	else
        {
     		elapsed = (PRIntervalTime) (PR_IntervalNow() - start);
  	   	    if (elapsed > timeout) ready = 0;  /* timed out */
     	    else
            {
        		remaining = timeout - elapsed;
     	   	    goto retry;
            }
  	    }
    }

    if (ready > 0)
    {
        ready = 0;
        for (pd = pds, epd = pd + npds; pd < epd; pd++)
        {
            PRInt16 out_flags = 0;
            if ((NULL != pd->fd) && (0 != pd->in_flags))
            {
                bottom = PR_GetIdentitiesLayer(pd->fd, PR_NSPR_IO_LAYER);
                PR_ASSERT(NULL != bottom);

                osfd = bottom->secret->md.osfd;

                if (FD_ISSET(osfd, &rd))
                {
                    if (pd->out_flags & _PR_POLL_READ_SYS_READ)
                        out_flags |= PR_POLL_READ;
                    if (pd->out_flags & _PR_POLL_WRITE_SYS_READ)
                        out_flags |= PR_POLL_WRITE;
                }
                if (FD_ISSET(osfd, &wt))
                {
                    if (pd->out_flags & _PR_POLL_READ_SYS_WRITE)
                        out_flags |= PR_POLL_READ;
                    if (pd->out_flags & _PR_POLL_WRITE_SYS_WRITE)
                        out_flags |= PR_POLL_WRITE;
                }
                if (FD_ISSET(osfd, &ex)) out_flags |= PR_POLL_EXCEPT;
            }
            pd->out_flags = out_flags;
            if (out_flags) ready++;
        }
        PR_ASSERT(ready > 0);
    }
    else if (ready < 0)
    {
   	    err = _MD_ERRNO();
   	    if (err == EBADF)
        {
   		    /* Find the bad fds */
   		    ready = 0;
   		    for (pd = pds, epd = pd + npds; pd < epd; pd++)
            {
                int optval;
                int optlen = sizeof(optval);
			    pd->out_flags = 0;
   			    if ((NULL == pd->fd) || (pd->in_flags == 0)) continue;
   			    bottom = PR_GetIdentitiesLayer(pd->fd, PR_NSPR_IO_LAYER);
                if (getsockopt(bottom->secret->md.osfd, SOL_SOCKET,
                    SO_TYPE, (char *) &optval, &optlen) == -1)
                {
                    PR_ASSERT(_MD_ERRNO() == ENOTSOCK);
                    if (_MD_ERRNO() == ENOTSOCK)
                    {
                        pd->out_flags = PR_POLL_NVAL;
                        ready++;
                    }
                }
   		    }
   		    PR_ASSERT(ready > 0);
   	    }
        else
        {
   		    PR_ASSERT(err != EINTR);  /* should have been handled above */
   		    _PR_MD_MAP_SELECT_ERROR(err);
        }
   }
   return ready;
 }

#ifdef XP_OS2_EMX
HMTX thread_select_mutex = 0;	/* because EMX's select is not thread safe - duh! */

typedef struct _thread_select_st {
	int		nfds;
	int		isrdfds;
	struct _fd_set *readfds;
	int		iswrfds;
	struct _fd_set *writefds;
	int		isexfds;
	struct _fd_set *exceptfds;
	int		istimeout;
	struct timeval	timeout;
	volatile HEV	event;
	int		result;
	int		select_errno;
	volatile int	done;
} *pthread_select_t;
	
void _thread_select(void * arg)
{
	pthread_select_t	self = arg;
	int			result, chkstdin;
	struct _fd_set		readfds;
	struct _fd_set		writefds;
	struct _fd_set		exceptfds;
	HEV			event = self->event;

	chkstdin = (self->isrdfds && FD_ISSET(0,self->readfds))?1:0;

	do {
		struct timeval	timeout = {0L,0L};


		if (self->isrdfds) readfds = *self->readfds;
		if (self->iswrfds) writefds = *self->writefds;
		if (self->isexfds) exceptfds = *self->exceptfds;
		
		if (chkstdin) FD_CLR(0,&readfds);

		if (!thread_select_mutex) 
			DosCreateMutexSem(NULL,&thread_select_mutex,0,1);
		else
			DosRequestMutexSem(thread_select_mutex,SEM_INDEFINITE_WAIT);
		result = select(
			self->nfds, 
			self->isrdfds?&readfds:NULL,
			self->iswrfds?&writefds:NULL,
			self->isexfds?&exceptfds:NULL,
			&timeout);
		DosReleaseMutexSem(thread_select_mutex);

		if (chkstdin) {
			int charcount = 0, res;
			res = ioctl(0,FIONREAD,&charcount);
			if (res==0 && charcount>0) FD_SET(0,&readfds);
		}
				
		if (result>0) {
			self->done++;
			if (self->isrdfds) *self->readfds = readfds;
			if (self->iswrfds) *self->writefds = writefds;
			if (self->isexfds) *self->exceptfds = exceptfds;
		} else
		if (result) self->done++;
		else DosSleep(1);

	} while (self->event!=0 && self->done==0);

	if (self->event) {
		self->select_errno = (result < 0)?errno:0;
		self->result = result;
		self->done = 3;
		DosPostEventSem(event);
	} else {
		self->done = 3;
		free(self);
	}

}

PRInt32
_MD_SELECT(int nfds, fd_set *readfds, fd_set *writefds,
                  fd_set *exceptfds, struct timeval *timeout)
{
	pthread_select_t sel;
	HEV		ev = 0;
	HTIMER		timer = 0;
	int		result = 0;
	APIRET		rc;
	unsigned long	msecs = SEM_INDEFINITE_WAIT;

	if (timeout) {
		if (timeout->tv_sec != 0 || timeout->tv_usec != 0) 
			msecs = (timeout->tv_sec * 1000L) + (timeout->tv_usec / 1000L);
		else
			msecs = SEM_IMMEDIATE_RETURN;
	};

	if (!(sel = (pthread_select_t) malloc(sizeof(struct _thread_select_st)))) {
		result = -1;
		errno = ENOMEM;
	} else {
		sel->nfds = nfds;
		sel->isrdfds = readfds?1:0;
		if (sel->isrdfds) sel->readfds = readfds;
		sel->iswrfds = writefds?1:0;
		if (sel->iswrfds) sel->writefds = writefds;
		sel->isexfds = exceptfds?1:0;
		if (sel->isexfds) sel->exceptfds = exceptfds;
		sel->istimeout = timeout?1:0;
		if (sel->istimeout) sel->timeout = *timeout;
	
		rc = DosCreateEventSem(NULL,&ev,0,FALSE);

		sel->event = ev;
		if (msecs == SEM_IMMEDIATE_RETURN)
			sel->done = 1;
		else
			sel->done = 0;

		if (_beginthread(_thread_select,NULL,65536,(void *)sel) == -1) {
			result = -1; sel->event = 0;
			DosCloseEventSem(ev);
		} else {
			rc = DosWaitEventSem(ev,msecs);
			if ((!sel->done) && (msecs != SEM_IMMEDIATE_RETURN)) {	/* Interrupted by other thread or timeout */
				sel->event = 0;
				result = 0;
				errno = ETIMEDOUT;
				
			} else {
				while (sel->done && sel->done != 3) {
					DosSleep(1);
				}
				sel->event = 0;
				result = sel->result;
				if (sel->select_errno) errno = sel->select_errno;
				free(sel);
			}
			rc = DosCloseEventSem(ev);
		}
	}

	return (result);
}

#endif
