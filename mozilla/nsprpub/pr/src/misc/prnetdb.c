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

#include <string.h>

/*
 * On Unix, the error code for gethostbyname() and gethostbyaddr()
 * is returned in the global variable h_errno, instead of the usual
 * errno.
 */
#if defined(XP_UNIX)
#if defined(_PR_NEED_H_ERRNO)
extern int h_errno;
#endif
#define _MD_GETHOST_ERRNO() h_errno
#else
#define _MD_GETHOST_ERRNO() _MD_ERRNO()
#endif

#if defined(_PR_NO_PREEMPT)
#define LOCK_DNS()
#define UNLOCK_DNS()
#else
PRLock *_pr_dnsLock = NULL;
#define LOCK_DNS() PR_Lock(_pr_dnsLock)
#define UNLOCK_DNS() PR_Unlock(_pr_dnsLock)
#endif  /* defined(_PR_NO_PREEMPT) */

#if defined(XP_UNIX)
#include <signal.h>

/*
** Unix's, as a rule, have a bug in their select code: if a timer
** interrupt occurs and you have SA_RESTART set on your signal, select
** forgets how much time has elapsed and restarts the system call from
** the beginning. This can cause a call to select to *never* time out.
**
** Because we aren't certain that select is wrapped properly in this code
** we disable the clock while a dns operation is occuring. This sucks and
** can be tossed when implement our own dns code that calls our own
** PR_Poll.
*/

static sigset_t timer_set;
#define DISABLECLOCK(_set)    sigprocmask(SIG_BLOCK, &timer_set, _set)
#define ENABLECLOCK(_set)    sigprocmask(SIG_SETMASK, _set, 0)

#endif /* XP_UNIX */

/*
 * Some platforms have the reentrant getprotobyname_r() and
 * getprotobynumber_r().  However, they come in two flavors.
 * Some return a pointer to struct protoent, others return
 * an int.
 */

#if defined(SOLARIS) \
	|| (defined(LINUX) && defined(_REENTRANT) \
        && !(defined(__GLIBC__) && __GLIBC__ >= 2))
#define _PR_HAVE_GETPROTO_R
#define _PR_HAVE_GETPROTO_R_POINTER
#endif

#if defined(OSF1) \
        || defined(AIX4_3) || (defined(AIX) && defined(_THREAD_SAFE)) \
	|| (defined(HPUX10_10) && defined(_REENTRANT)) \
        || (defined(HPUX10_20) && defined(_REENTRANT))
#define _PR_HAVE_GETPROTO_R
#define _PR_HAVE_GETPROTO_R_INT
#endif

#if (defined(LINUX) && defined(__GLIBC__) && __GLIBC__ >= 2)
#define _PR_HAVE_GETPROTO_R
#define _PR_HAVE_5_ARG_GETPROTO_R
#endif

#if !defined(_PR_HAVE_GETPROTO_R)
PRLock* _getproto_lock = NULL;
#endif

#if defined(_PR_INET6)
const struct in6_addr _pr_in6addr_any = IN6ADDR_ANY_INIT;
const struct in6_addr _pr_in6addr_loopback = IN6ADDR_LOOPBACK_INIT;

#define _PR_IN6_IS_ADDR_UNSPECIFIED	IN6_IS_ADDR_UNSPECIFIED
#define _PR_IN6_IS_ADDR_LOOPBACK	IN6_IS_ADDR_LOOPBACK
#define _PR_IN6_IS_ADDR_V4MAPPED	IN6_IS_ADDR_V4MAPPED
#define _PR_IN6_IS_ADDR_V4COMPAT	IN6_IS_ADDR_V4COMPAT

#if defined(SOLARIS)
#define _PR_IN6_V4MAPPED_TO_IPADDR(a) ((a)->_S6_un._S6_u32[3])
#else
#define _PR_IN6_V4MAPPED_TO_IPADDR(a) ((a)->s6_addr32[3])
#endif

#else  /* _PR_INET6 */

#define _PR_IN6_IS_ADDR_UNSPECIFIED(a)			\
				(((a)->_pr_s6_addr32[0] == 0) &&	\
				((a)->_pr_s6_addr32[1] == 0) &&		\
				((a)->_pr_s6_addr32[2] == 0) &&		\
				((a)->_pr_s6_addr32[3] == 0))
 
/*
 * For non-ipv6 platforms, loopback address is never passed to the
 * system, so the byte-ordering is not an issue
 */
#define _PR_IN6_IS_ADDR_LOOPBACK(a)				\
               (((a)->_pr_s6_addr32[0] == 0)	&&	\
               ((a)->_pr_s6_addr32[1] == 0)		&&	\
               ((a)->_pr_s6_addr32[2] == 0)		&&	\
               ((a)->_pr_s6_addr32[3] == 0x1))
 
const PRIPv6Addr _pr_in6addr_any = { 0, 0, 0, 0 };
const PRIPv6Addr _pr_in6addr_loopback = { 0, 0, 0, 0x00000001U };
/*
 * The values at bytes 10 and 11 are compared using pointers to
 * 8-bit fields, and not 32-bit fields, to make the comparison work on
 * both big-endian and little-endian systems
 */

#define _PR_IN6_IS_ADDR_V4MAPPED(a)			\
		(((a)->_pr_s6_addr32[0] == 0) 	&&	\
		((a)->_pr_s6_addr32[1] == 0)	&&	\
		((a)->_pr_s6_addr[8] == 0)		&&	\
		((a)->_pr_s6_addr[9] == 0)		&&	\
		((a)->_pr_s6_addr[10] == 0xff)	&&	\
		((a)->_pr_s6_addr[11] == 0xff))

#define _PR_IN6_IS_ADDR_V4COMPAT(a)			\
		(((a)->_pr_s6_addr32[0] == 0) &&	\
		((a)->_pr_s6_addr32[1] == 0) &&		\
		((a)->_pr_s6_addr32[2] == 0))

#define _PR_IN6_V4MAPPED_TO_IPADDR(a) ((a)->_pr_s6_addr32[3])

#endif /* _PR_INET6 */

#if !defined(_PR_INET6)
extern PRStatus _pr_init_ipv6();
#endif

void _PR_InitNet(void)
{
#if defined(XP_UNIX)
#ifdef HAVE_NETCONFIG
	/*
	 * This one-liner prevents the endless re-open's and re-read's of
	 * /etc/netconfig on EACH and EVERY call to accept(), connect(), etc.
	 */
	 (void)setnetconfig();
#endif
	sigemptyset(&timer_set);
	sigaddset(&timer_set, SIGALRM);
#endif
#if !defined(_PR_NO_PREEMPT)
	_pr_dnsLock = PR_NewLock();
#endif
#if !defined(_PR_HAVE_GETPROTO_R)
	_getproto_lock = PR_NewLock();
#endif
#if !defined(_PR_INET6)
	_pr_init_ipv6();
#endif
}

PR_IMPLEMENT(PRStatus) PR_GetHostName(char *name, PRUint32 namelen)
{
#if defined(DEBUG)
    static PRBool warn = PR_TRUE;
    if (warn) warn = _PR_Obsolete("PR_GetHostName()", "PR_GetSystemInfo()");
#endif
    return PR_GetSystemInfo(PR_SI_HOSTNAME, name, namelen);
}

/*
** Allocate space from the buffer, aligning it to "align" before doing
** the allocation. "align" must be a power of 2.
*/
static char *Alloc(PRIntn amount, char **bufp, PRIntn *buflenp, PRIntn align)
{
	char *buf = *bufp;
	PRIntn buflen = *buflenp;

	if (align && ((long)buf & (align - 1))) {
		PRIntn skip = align - ((ptrdiff_t)buf & (align - 1));
		if (buflen < skip) {
			return 0;
		}
		buf += skip;
		buflen -= skip;
	}
	if (buflen < amount) {
		return 0;
	}
	*bufp = buf + amount;
	*buflenp = buflen - amount;
	return buf;
}

typedef enum _PRIPAddrConversion {
    _PRIPAddrNoConversion,
    _PRIPAddrIPv4Mapped,
    _PRIPAddrIPv4Compat
} _PRIPAddrConversion;

/*
** Convert an IPv4 address (v4) to an IPv4-mapped IPv6 address (v6).
*/
static void MakeIPv4MappedAddr(const char *v4, char *v6)
{
    memset(v6, 0, 10);
    memset(v6 + 10, 0xff, 2);
    memcpy(v6 + 12, v4, 4);
    PR_ASSERT(_PR_IN6_IS_ADDR_V4MAPPED(((PRIPv6Addr *) v6)));
}

/*
** Convert an IPv4 address (v4) to an IPv4-compatible IPv6 address (v6).
*/
static void MakeIPv4CompatAddr(const char *v4, char *v6)
{
    memset(v6, 0, 12);
    memcpy(v6 + 12, v4, 4);
    PR_ASSERT(_PR_IN6_IS_ADDR_V4COMPAT(((PRIPv6Addr *) v6)));
}

/*
** Copy a hostent, and all of the memory that it refers to into
** (hopefully) stacked buffers.
*/
static PRStatus CopyHostent(
    struct hostent *from,
    char **buf,
    PRIntn *bufsize,
    _PRIPAddrConversion conversion,
    PRHostEnt *to)
{
	PRIntn len, na;
	char **ap;

	if (conversion != _PRIPAddrNoConversion
			&& from->h_addrtype == AF_INET) {
		PR_ASSERT(from->h_length == 4);
		to->h_addrtype = PR_AF_INET6;
		to->h_length = 16;
	} else {
#if defined(_PR_INET6)
		if (AF_INET6 == from->h_addrtype)
			to->h_addrtype = PR_AF_INET6;
		else
#endif
			to->h_addrtype = from->h_addrtype;
		to->h_length = from->h_length;
	}

	/* Copy the official name */
	if (!from->h_name) return PR_FAILURE;
	len = strlen(from->h_name) + 1;
	to->h_name = Alloc(len, buf, bufsize, 0);
	if (!to->h_name) return PR_FAILURE;
	memcpy(to->h_name, from->h_name, len);

	/* Count the aliases, then allocate storage for the pointers */
	if (!from->h_aliases) {
		na = 1;
	} else {
		for (na = 1, ap = from->h_aliases; *ap != 0; na++, ap++){;} /* nothing to execute */
	}
	to->h_aliases = (char**)Alloc(
	    na * sizeof(char*), buf, bufsize, sizeof(char**));
	if (!to->h_aliases) return PR_FAILURE;

	/* Copy the aliases, one at a time */
	if (!from->h_aliases) {
		to->h_aliases[0] = 0;
	} else {
		for (na = 0, ap = from->h_aliases; *ap != 0; na++, ap++) {
			len = strlen(*ap) + 1;
			to->h_aliases[na] = Alloc(len, buf, bufsize, 0);
			if (!to->h_aliases[na]) return PR_FAILURE;
			memcpy(to->h_aliases[na], *ap, len);
		}
		to->h_aliases[na] = 0;
	}

	/* Count the addresses, then allocate storage for the pointers */
	for (na = 1, ap = from->h_addr_list; *ap != 0; na++, ap++){;} /* nothing to execute */
	to->h_addr_list = (char**)Alloc(
	    na * sizeof(char*), buf, bufsize, sizeof(char**));
	if (!to->h_addr_list) return PR_FAILURE;

	/* Copy the addresses, one at a time */
	for (na = 0, ap = from->h_addr_list; *ap != 0; na++, ap++) {
		to->h_addr_list[na] = Alloc(to->h_length, buf, bufsize, 0);
		if (!to->h_addr_list[na]) return PR_FAILURE;
		if (conversion != _PRIPAddrNoConversion
				&& from->h_addrtype == AF_INET) {
			if (conversion == _PRIPAddrIPv4Mapped) {
				MakeIPv4MappedAddr(*ap, to->h_addr_list[na]);
			} else {
				PR_ASSERT(conversion == _PRIPAddrIPv4Compat);
				MakeIPv4CompatAddr(*ap, to->h_addr_list[na]);
			}
		} else {
			memcpy(to->h_addr_list[na], *ap, to->h_length);
		}
	}
	to->h_addr_list[na] = 0;
	return PR_SUCCESS;
}

#if !defined(_PR_HAVE_GETPROTO_R)
/*
** Copy a protoent, and all of the memory that it refers to into
** (hopefully) stacked buffers.
*/
static PRStatus CopyProtoent(
    struct protoent *from, char *buf, PRIntn bufsize, PRProtoEnt *to)
{
	PRIntn len, na;
	char **ap;

	/* Do the easy stuff */
	to->p_num = from->p_proto;

	/* Copy the official name */
	if (!from->p_name) return PR_FAILURE;
	len = strlen(from->p_name) + 1;
	to->p_name = Alloc(len, &buf, &bufsize, 0);
	if (!to->p_name) return PR_FAILURE;
	memcpy(to->p_name, from->p_name, len);

	/* Count the aliases, then allocate storage for the pointers */
	for (na = 1, ap = from->p_aliases; *ap != 0; na++, ap++){;} /* nothing to execute */
	to->p_aliases = (char**)Alloc(
	    na * sizeof(char*), &buf, &bufsize, sizeof(char**));
	if (!to->p_aliases) return PR_FAILURE;

	/* Copy the aliases, one at a time */
	for (na = 0, ap = from->p_aliases; *ap != 0; na++, ap++) {
		len = strlen(*ap) + 1;
		to->p_aliases[na] = Alloc(len, &buf, &bufsize, 0);
		if (!to->p_aliases[na]) return PR_FAILURE;
		memcpy(to->p_aliases[na], *ap, len);
	}
	to->p_aliases[na] = 0;

	return PR_SUCCESS;
}
#endif /* !defined(_PR_HAVE_GETPROTO_R) */

PR_IMPLEMENT(PRStatus) PR_GetHostByName(
    const char *name, char *buf, PRIntn bufsize, PRHostEnt *hp)
{
	struct hostent *h;
	PRStatus rv = PR_FAILURE;
#ifdef XP_UNIX
	sigset_t oldset;
#endif
#if defined(_PR_INET6) && defined(_PR_HAVE_GETIPNODEBYNAME)
	int error_num;
#endif

    if (!_pr_initialized) _PR_ImplicitInitialization();

#ifdef XP_UNIX
	DISABLECLOCK(&oldset);
#endif
	LOCK_DNS();

#ifdef XP_OS2_VACPP
	h = gethostbyname((char *)name);
#else
    h = gethostbyname(name);
#endif
    
	if (NULL == h)
	{
#if defined(_PR_INET6) && defined(_PR_HAVE_GETIPNODEBYNAME)
	    PR_SetError(PR_DIRECTORY_LOOKUP_ERROR, error_num);
#else
	    PR_SetError(PR_DIRECTORY_LOOKUP_ERROR, _MD_GETHOST_ERRNO());
#endif
	}
	else
	{
		_PRIPAddrConversion conversion = _PRIPAddrNoConversion;
		rv = CopyHostent(h, &buf, &bufsize, conversion, hp);
		if (PR_SUCCESS != rv)
		    PR_SetError(PR_INSUFFICIENT_RESOURCES_ERROR, 0);
#if defined(_PR_INET6) && defined(_PR_HAVE_GETIPNODEBYNAME)
		freehostent(h);
#endif
	}
	UNLOCK_DNS();
#ifdef XP_UNIX
	ENABLECLOCK(&oldset);
#endif
	return rv;
}

PR_IMPLEMENT(PRStatus) PR_GetIPNodeByName(
    const char *name, PRUint16 af, PRIntn flags,
    char *buf, PRIntn bufsize, PRHostEnt *hp)
{
	struct hostent *h;
	PRStatus rv = PR_FAILURE;
#ifdef XP_UNIX
	sigset_t oldset;
#endif
#if defined(_PR_INET6) && defined(_PR_HAVE_GETIPNODEBYNAME)
	int error_num;
	int tmp_flags = 0;
#endif
#if defined(_PR_INET6) && defined(_PR_HAVE_GETHOSTBYNAME2)
    PRBool did_af_inet = PR_FALSE;
    char **new_addr_list;
#endif

    if (!_pr_initialized) _PR_ImplicitInitialization();

    PR_ASSERT(af == PR_AF_INET || af == PR_AF_INET6);
    if (af != PR_AF_INET && af != PR_AF_INET6) {
        PR_SetError(PR_INVALID_ARGUMENT_ERROR, 0);
        return PR_FAILURE;
    }

#ifdef XP_UNIX
	DISABLECLOCK(&oldset);
#endif
	LOCK_DNS();

#ifdef _PR_INET6
#ifdef _PR_HAVE_GETHOSTBYNAME2
    if (af == PR_AF_INET6)
    {
        h = gethostbyname2(name, AF_INET6); 
        if ((NULL == h) && (flags & PR_AI_V4MAPPED))
        {
            did_af_inet = PR_TRUE;
            h = gethostbyname2(name, AF_INET);
        }
    }
    else
    {
        did_af_inet = PR_TRUE;
        h = gethostbyname2(name, af);
    }
#elif defined(_PR_HAVE_GETIPNODEBYNAME)
	if (flags & PR_AI_V4MAPPED)
		tmp_flags |= AI_V4MAPPED;
	if (flags & PR_AI_ADDRCONFIG)
		tmp_flags |= AI_ADDRCONFIG;
	if (flags & PR_AI_ALL)
		tmp_flags |= AI_ALL;
    h = getipnodebyname(name, af, tmp_flags, &error_num);
#else
#error "Unknown name-to-address translation function"
#endif
#else /* _PR_INET6 */
#ifdef XP_OS2_VACPP
	h = gethostbyname((char *)name);
#else
    h = gethostbyname(name);
#endif
#endif /* _PR_INET6 */
    
	if (NULL == h)
	{
#if defined(_PR_INET6) && defined(_PR_HAVE_GETIPNODEBYNAME)
	    PR_SetError(PR_DIRECTORY_LOOKUP_ERROR, error_num);
#else
	    PR_SetError(PR_DIRECTORY_LOOKUP_ERROR, _MD_GETHOST_ERRNO());
#endif
	}
	else
	{
		_PRIPAddrConversion conversion = _PRIPAddrNoConversion;

		if (af == PR_AF_INET6) conversion = _PRIPAddrIPv4Mapped;
		rv = CopyHostent(h, &buf, &bufsize, conversion, hp);
		if (PR_SUCCESS != rv)
		    PR_SetError(PR_INSUFFICIENT_RESOURCES_ERROR, 0);
#if defined(_PR_INET6) && defined(_PR_HAVE_GETIPNODEBYNAME)
		freehostent(h);
#endif
#if defined(_PR_INET6) && defined(_PR_HAVE_GETHOSTBYNAME2)
		if ((flags & PR_AI_V4MAPPED) && (flags & (PR_AI_ALL|PR_AI_ADDRCONFIG))
				&& !did_af_inet && (h = gethostbyname2(name, AF_INET)) != 0) {
			/* Append the V4 addresses to the end of the list */
			PRIntn na, na_old;
			char **ap;
			
			/* Count the addresses, then grow storage for the pointers */
			for (na_old = 0, ap = hp->h_addr_list; *ap != 0; na_old++, ap++)
					{;} /* nothing to execute */
			for (na = na_old + 1, ap = h->h_addr_list; *ap != 0; na++, ap++)
					{;} /* nothing to execute */
			new_addr_list = (char**)Alloc(
				na * sizeof(char*), &buf, &bufsize, sizeof(char**));
			if (!new_addr_list) return PR_FAILURE;

			/* Copy the V6 addresses, one at a time */
			for (na = 0, ap = hp->h_addr_list; *ap != 0; na++, ap++) {
				new_addr_list[na] = hp->h_addr_list[na];
			}
			hp->h_addr_list = new_addr_list;

			/* Copy the V4 addresses, one at a time */
			for (ap = h->h_addr_list; *ap != 0; na++, ap++) {
				hp->h_addr_list[na] = Alloc(hp->h_length, &buf, &bufsize, 0);
				if (!hp->h_addr_list[na]) return PR_FAILURE;
				MakeIPv4MappedAddr(*ap, hp->h_addr_list[na]);
			}
			hp->h_addr_list[na] = 0;
		}
#endif
	}

	UNLOCK_DNS();
#ifdef XP_UNIX
	ENABLECLOCK(&oldset);
#endif
	return rv;
}

PR_IMPLEMENT(PRStatus) PR_GetHostByAddr(
    const PRNetAddr *hostaddr, char *buf, PRIntn bufsize, PRHostEnt *hostentry)
{
	struct hostent *h;
	PRStatus rv = PR_FAILURE;
	const void *addr;
	int addrlen;
	PRInt32 af;
#ifdef XP_UNIX
	sigset_t oldset;
#endif
#if defined(_PR_INET6) && defined(_PR_HAVE_GETIPNODEBYADDR)
	int error_num;
#endif

    if (!_pr_initialized) _PR_ImplicitInitialization();

#ifdef XP_UNIX
	DISABLECLOCK(&oldset);
#endif
	LOCK_DNS();
	if (hostaddr->raw.family == PR_AF_INET6)
	{
		addr = &hostaddr->ipv6.ip;
		addrlen = sizeof(hostaddr->ipv6.ip);
	}
	else
	{
		PR_ASSERT(hostaddr->raw.family == AF_INET);
		addr = &hostaddr->inet.ip;
		addrlen = sizeof(hostaddr->inet.ip);
	}
#if defined(_PR_INET6)
	if (PR_AF_INET6 == hostaddr->raw.family)
		af = AF_INET6;
	else
#endif
		af = hostaddr->raw.family;
#if defined(_PR_INET6) && defined(_PR_HAVE_GETIPNODEBYADDR)
	h = getipnodebyaddr(addr, addrlen, af, &error_num);
#else
#ifdef XP_OS2_VACPP
	h = gethostbyaddr((char *)addr, addrlen, af);
#else
	h = gethostbyaddr(addr, addrlen, af);
#endif
#endif /* _PR_INET6 && _PR_HAVE_GETIPNODEBYADDR */
	if (NULL == h)
	{
#if defined(_PR_INET6) && defined(_PR_HAVE_GETIPNODEBYADDR)
		PR_SetError(PR_DIRECTORY_LOOKUP_ERROR, error_num);
#else
		PR_SetError(PR_DIRECTORY_LOOKUP_ERROR, _MD_GETHOST_ERRNO());
#endif
	}
	else
	{
		_PRIPAddrConversion conversion = _PRIPAddrNoConversion;
#if defined(_PR_INET6)
		if (hostaddr->raw.family == PR_AF_INET6) {
			if (IN6_IS_ADDR_V4MAPPED((struct in6_addr*)addr)) {
				conversion = _PRIPAddrIPv4Mapped;
			} else if (IN6_IS_ADDR_V4COMPAT((struct in6_addr*)addr)) {
				conversion = _PRIPAddrIPv4Compat;
			}
		}
#endif
		rv = CopyHostent(h, &buf, &bufsize, conversion, hostentry);
		if (PR_SUCCESS != rv) {
		    PR_SetError(PR_INSUFFICIENT_RESOURCES_ERROR, 0);
		}
#if defined(_PR_INET6) && defined(_PR_HAVE_GETIPNODEBYADDR)
		freehostent(h);
#endif
	}
	UNLOCK_DNS();
#ifdef XP_UNIX
	ENABLECLOCK(&oldset);
#endif
	return rv;
}

/******************************************************************************/
/*
 * Some systems define a reentrant version of getprotobyname(). Too bad
 * the signature isn't always the same. But hey, they tried. If there
 * is such a definition, use it. Otherwise, grab a lock and do it here.
 */
/******************************************************************************/

#if !defined(_PR_HAVE_GETPROTO_R)
/*
 * This may seem like a silly thing to do, but the compiler SHOULD
 * complain if getprotobyname_r() is implemented on some system and
 * we're not using it. For sure these signatures are different than
 * any usable implementation.
 */

static struct protoent *getprotobyname_r(const char* name)
{
#ifdef XP_OS2_VACPP
	return getprotobyname((char *)name);
#else
	return getprotobyname(name);
#endif
} /* getprotobyname_r */

static struct protoent *getprotobynumber_r(PRInt32 number)
{
	return getprotobynumber(number);
} /* getprotobynumber_r */

#endif /* !defined(_PR_HAVE_GETPROTO_R) */

PR_IMPLEMENT(PRStatus) PR_GetProtoByName(
    const char* name, char* buffer, PRInt32 buflen, PRProtoEnt* result)
{
	PRStatus rv = PR_SUCCESS;
#if defined(_PR_HAVE_GETPROTO_R)
	struct protoent* res = (struct protoent*)result;
#endif

    if (!_pr_initialized) _PR_ImplicitInitialization();

#if defined(_PR_HAVE_GETPROTO_R_INT)
    {
        /*
        ** The protoent_data has a pointer as the first field.
        ** That implies the buffer better be aligned, and char*
        ** doesn't promise much.
        */
        PRUptrdiff aligned = (PRUptrdiff)buffer;
        if (0 != (aligned & (sizeof(struct protoent_data*) - 1)))
        {
            aligned += sizeof(struct protoent_data*) - 1;
            aligned &= ~(sizeof(struct protoent_data*) - 1);
            buflen -= (aligned - (PRUptrdiff)buffer);
            buffer = (char*)aligned;
        }
    }
#endif  /* defined(_PR_HAVE_GETPROTO_R_INT) */

	PR_ASSERT(PR_NETDB_BUF_SIZE <= buflen);
    if (PR_NETDB_BUF_SIZE > buflen)
    {
        PR_SetError(PR_INVALID_ARGUMENT_ERROR, 0);
        return PR_FAILURE;
    }

#if defined(_PR_HAVE_GETPROTO_R_POINTER)
    if (NULL == getprotobyname_r(name, res, buffer, buflen))
    {
        PR_SetError(PR_DIRECTORY_LOOKUP_ERROR, _MD_ERRNO());
        return PR_FAILURE;
    }
#elif defined(_PR_HAVE_GETPROTO_R_INT)
    /*
    ** The buffer needs to be zero'd, and it should be
    ** at least the size of a struct protoent_data.
    */
    memset(buffer, 0, buflen);
	if (-1 == getprotobyname_r(name, res, (struct protoent_data*)buffer))
    {
        PR_SetError(PR_DIRECTORY_LOOKUP_ERROR, _MD_ERRNO());
        return PR_FAILURE;
    }
#elif defined(_PR_HAVE_5_ARG_GETPROTO_R)
    /* The 5th argument for getprotobyname_r() cannot be NULL */
    if (-1 == getprotobyname_r(name, res, buffer, buflen, &res))
    {
        PR_SetError(PR_DIRECTORY_LOOKUP_ERROR, _MD_ERRNO());
        return PR_FAILURE;
    }
#else  /* do it the hard way */
	{
		struct protoent *staticBuf;
		PR_Lock(_getproto_lock);
		staticBuf = getprotobyname_r(name);
		if (NULL == staticBuf)
		{
		    rv = PR_FAILURE;
		    PR_SetError(PR_DIRECTORY_LOOKUP_ERROR, _MD_ERRNO());
        }
		else
		{
			rv = CopyProtoent(staticBuf, buffer, buflen, result);
			if (PR_FAILURE == rv)
			    PR_SetError(PR_INSUFFICIENT_RESOURCES_ERROR, 0);
        }
		PR_Unlock(_getproto_lock);
	}
#endif  /* all that */
    return rv;
}

PR_IMPLEMENT(PRStatus) PR_GetProtoByNumber(
    PRInt32 number, char* buffer, PRInt32 buflen, PRProtoEnt* result)
{
	PRStatus rv = PR_SUCCESS;
#if defined(_PR_HAVE_GETPROTO_R)
	struct protoent* res = (struct protoent*)result;
#endif

    if (!_pr_initialized) _PR_ImplicitInitialization();

#if defined(_PR_HAVE_GETPROTO_R_INT)
    {
        /*
        ** The protoent_data has a pointer as the first field.
        ** That implies the buffer better be aligned, and char*
        ** doesn't promise much.
        */
        PRUptrdiff aligned = (PRUptrdiff)buffer;
        if (0 != (aligned & (sizeof(struct protoent_data*) - 1)))
        {
            aligned += sizeof(struct protoent_data*) - 1;
            aligned &= ~(sizeof(struct protoent_data*) - 1);
            buflen -= (aligned - (PRUptrdiff)buffer);
            buffer = (char*)aligned;
        }
    }
#endif /* defined(_PR_HAVE_GETPROTO_R_INT) */

	PR_ASSERT(PR_NETDB_BUF_SIZE <= buflen);
    if (PR_NETDB_BUF_SIZE > buflen)
    {
        PR_SetError(PR_INVALID_ARGUMENT_ERROR, 0);
        return PR_FAILURE;
    }

#if defined(_PR_HAVE_GETPROTO_R_POINTER)
    if (NULL == getprotobynumber_r(number, res, buffer, buflen))
    {
        PR_SetError(PR_DIRECTORY_LOOKUP_ERROR, _MD_ERRNO());
        return PR_FAILURE;
    }

#elif defined(_PR_HAVE_GETPROTO_R_INT)
    /*
    ** The buffer needs to be zero'd for these OS's.
    */
    memset(buffer, 0, buflen);
	if (-1 == getprotobynumber_r(number, res, (struct protoent_data*)buffer))
    {
        PR_SetError(PR_DIRECTORY_LOOKUP_ERROR, _MD_ERRNO());
        return PR_FAILURE;
    }
#elif defined(_PR_HAVE_5_ARG_GETPROTO_R)
    /* The 5th argument for getprotobynumber_r() cannot be NULL */
    if (-1 == getprotobynumber_r(number, res, buffer, buflen, &res))
    {
        PR_SetError(PR_DIRECTORY_LOOKUP_ERROR, _MD_ERRNO());
        return PR_FAILURE;
    }
#else  /* do it the hard way */
	{
		struct protoent *staticBuf;
		PR_Lock(_getproto_lock);
		staticBuf = getprotobynumber_r(number);
		if (NULL == staticBuf)
		{
		    rv = PR_FAILURE;
		    PR_SetError(PR_DIRECTORY_LOOKUP_ERROR, _MD_ERRNO());
        }
		else
		{
			rv = CopyProtoent(staticBuf, buffer, buflen, result);
			if (PR_FAILURE == rv)
			    PR_SetError(PR_INSUFFICIENT_RESOURCES_ERROR, 0);
        }
		PR_Unlock(_getproto_lock);
	}
#endif  /* all that crap */
    return rv;

}

PR_IMPLEMENT(PRUintn) PR_NetAddrSize(const PRNetAddr* addr)
{
    PRUintn addrsize;

    /*
     * RFC 2553 added a new field (sin6_scope_id) to
     * struct sockaddr_in6.  PRNetAddr's ipv6 member has a
     * scope_id field to match the new field.  In order to
     * work with older implementations supporting RFC 2133,
     * we take the size of struct sockaddr_in6 instead of
     * addr->ipv6.
     */
    if (AF_INET == addr->raw.family)
        addrsize = sizeof(addr->inet);
    else if (PR_AF_INET6 == addr->raw.family)
        addrsize = sizeof(_pr_sockaddr_in6_t);
#if defined(XP_UNIX)
    else if (AF_UNIX == addr->raw.family)
        addrsize = sizeof(addr->local);
#endif
    else addrsize = 0;

    return addrsize;
}  /* PR_NetAddrSize */

PR_IMPLEMENT(PRIntn) PR_EnumerateHostEnt(
    PRIntn enumIndex, const PRHostEnt *hostEnt, PRUint16 port, PRNetAddr *address)
{
    void *addr = hostEnt->h_addr_list[enumIndex++];
    memset(address, 0, sizeof(PRNetAddr));
    if (NULL == addr) enumIndex = 0;
    else
    {
        address->raw.family = hostEnt->h_addrtype;
        if (PR_AF_INET6 == hostEnt->h_addrtype)
        {
            address->ipv6.port = htons(port);
            memcpy(&address->ipv6.ip, addr, hostEnt->h_length);
        }
        else
        {
            PR_ASSERT(AF_INET == hostEnt->h_addrtype);
            address->inet.port = htons(port);
            memcpy(&address->inet.ip, addr, hostEnt->h_length);
        }
    }
    return enumIndex;
}  /* PR_EnumerateHostEnt */

PR_IMPLEMENT(PRStatus) PR_InitializeNetAddr(
    PRNetAddrValue val, PRUint16 port, PRNetAddr *addr)
{
    PRStatus rv = PR_SUCCESS;
    if (!_pr_initialized) _PR_ImplicitInitialization();

	addr->inet.family = AF_INET;
	addr->inet.port = htons(port);
	switch (val)
	{
	case PR_IpAddrNull:
		break;  /* don't overwrite the address */
	case PR_IpAddrAny:
		addr->inet.ip = htonl(INADDR_ANY);
		break;
	case PR_IpAddrLoopback:
		addr->inet.ip = htonl(INADDR_LOOPBACK);
		break;
	default:
		PR_SetError(PR_INVALID_ARGUMENT_ERROR, 0);
		rv = PR_FAILURE;
	}
    return rv;
}  /* PR_InitializeNetAddr */

PR_IMPLEMENT(PRStatus) PR_SetNetAddr(
    PRNetAddrValue val, PRUint16 af, PRUint16 port, PRNetAddr *addr)
{
    PRStatus rv = PR_SUCCESS;
    if (!_pr_initialized) _PR_ImplicitInitialization();

    addr->raw.family = af;
#if defined(_PR_INET6)
    if (af == PR_AF_INET6)
    {
        addr->ipv6.port = htons(port);
        switch (val)
        {
        case PR_IpAddrNull:
            break;  /* don't overwrite the address */
        case PR_IpAddrAny:
            addr->ipv6.ip = _pr_in6addr_any;
            break;
        case PR_IpAddrLoopback:
            addr->ipv6.ip = _pr_in6addr_loopback;
            break;
        default:
            PR_SetError(PR_INVALID_ARGUMENT_ERROR, 0);
            rv = PR_FAILURE;
        }
    }
    else
#else
    if (af == PR_AF_INET6)
    {
		PRUint32 ipv4_in_addr;		
        addr->ipv6.port = htons(port);
        switch (val)
        {
        case PR_IpAddrNull:
            break;  /* don't overwrite the address */
        case PR_IpAddrAny:
            ipv4_in_addr = htonl(INADDR_ANY);
			MakeIPv4MappedAddr((char *) &ipv4_in_addr,
								(char *) addr->ipv6.ip._pr_s6_addr);
            break;
        case PR_IpAddrLoopback:
            ipv4_in_addr = htonl(INADDR_LOOPBACK);
			MakeIPv4MappedAddr((char *) &ipv4_in_addr,
								(char *) addr->ipv6.ip._pr_s6_addr);
            break;
        default:
            PR_SetError(PR_INVALID_ARGUMENT_ERROR, 0);
            rv = PR_FAILURE;
        }
    }
    else
#endif
    {
        addr->inet.port = htons(port);
        switch (val)
        {
        case PR_IpAddrNull:
            break;  /* don't overwrite the address */
        case PR_IpAddrAny:
            addr->inet.ip = htonl(INADDR_ANY);
            break;
        case PR_IpAddrLoopback:
            addr->inet.ip = htonl(INADDR_LOOPBACK);
            break;
        default:
            PR_SetError(PR_INVALID_ARGUMENT_ERROR, 0);
            rv = PR_FAILURE;
        }
    }
    return rv;
}  /* PR_SetNetAddr */

PR_IMPLEMENT(PRBool)
PR_IsNetAddrType(const PRNetAddr *addr, PRNetAddrValue val)
{
    if (addr->raw.family == PR_AF_INET6) {
        if (val == PR_IpAddrAny) {
			if (_PR_IN6_IS_ADDR_UNSPECIFIED((PRIPv6Addr *)&addr->ipv6.ip)) {
            	return PR_TRUE;
			} else if (_PR_IN6_IS_ADDR_V4MAPPED((PRIPv6Addr *)&addr->ipv6.ip)
					&& _PR_IN6_V4MAPPED_TO_IPADDR((PRIPv6Addr *)&addr->ipv6.ip)
							== htonl(INADDR_ANY)) {
            	return PR_TRUE;
			}
        } else if (val == PR_IpAddrLoopback) {
            if (_PR_IN6_IS_ADDR_LOOPBACK((PRIPv6Addr *)&addr->ipv6.ip)) {
            	return PR_TRUE;
			} else if (_PR_IN6_IS_ADDR_V4MAPPED((PRIPv6Addr *)&addr->ipv6.ip)
					&& _PR_IN6_V4MAPPED_TO_IPADDR((PRIPv6Addr *)&addr->ipv6.ip)
							== htonl(INADDR_LOOPBACK)) {
            	return PR_TRUE;
			}
        } else if (val == PR_IpAddrV4Mapped
                && _PR_IN6_IS_ADDR_V4MAPPED((PRIPv6Addr *)&addr->ipv6.ip)) {
            return PR_TRUE;
        }
    } else {
        if (addr->raw.family == AF_INET) {
            if (val == PR_IpAddrAny && addr->inet.ip == htonl(INADDR_ANY)) {
                return PR_TRUE;
            } else if (val == PR_IpAddrLoopback
                    && addr->inet.ip == htonl(INADDR_LOOPBACK)) {
                return PR_TRUE;
            }
        }
    }
    return PR_FALSE;
}

PR_IMPLEMENT(PRNetAddr*) PR_CreateNetAddr(PRNetAddrValue val, PRUint16 port)
{
    PRNetAddr *addr = NULL;
    if ((PR_IpAddrAny == val) || (PR_IpAddrLoopback == val))
    {
        addr = PR_NEWZAP(PRNetAddr);
        if (NULL == addr)
            PR_SetError(PR_OUT_OF_MEMORY_ERROR, 0);
        else
            if (PR_FAILURE == PR_InitializeNetAddr(val, port, addr))
                PR_DELETE(addr);  /* and that will make 'addr' == NULL */
    }
    else
        PR_SetError(PR_INVALID_ARGUMENT_ERROR, 0);
    return addr;
}  /* PR_CreateNetAddr */

PR_IMPLEMENT(PRStatus) PR_DestroyNetAddr(PRNetAddr *addr)
{
    PR_Free(addr);
    return PR_SUCCESS;
}  /* PR_DestroyNetAddr */

PR_IMPLEMENT(PRStatus) PR_StringToNetAddr(const char *string, PRNetAddr *addr)
{
    PRStatus status = PR_SUCCESS;

#if defined(_PR_INET6)
    PRIntn rv;

    rv = inet_pton(AF_INET6, string, &addr->ipv6.ip);
    if (1 == rv)
    {
        addr->raw.family = PR_AF_INET6;
    }
    else
    {
        PR_ASSERT(0 == rv);
        rv = inet_pton(AF_INET, string, &addr->inet.ip);
        if (1 == rv)
        {
            addr->raw.family = AF_INET;
        }
        else
        {
            PR_ASSERT(0 == rv);
            PR_SetError(PR_INVALID_ARGUMENT_ERROR, 0);
            status = PR_FAILURE;
        }
    }
#else /* _PR_INET6 */
    addr->inet.family = AF_INET;
#ifdef XP_OS2_VACPP
    addr->inet.ip = inet_addr((char *)string);
#else
    addr->inet.ip = inet_addr(string);
#endif
    if ((PRUint32) -1 == addr->inet.ip)
    {
        /*
         * The string argument is a malformed address string.
         */
        PR_SetError(PR_INVALID_ARGUMENT_ERROR, 0);
        status = PR_FAILURE;
    }
#endif /* _PR_INET6 */

    return status;
}

PR_IMPLEMENT(PRStatus) PR_NetAddrToString(
    const PRNetAddr *addr, char *string, PRUint32 size)
{
#if defined(_PR_INET6)
    if (PR_AF_INET6 == addr->raw.family)
    {
        if (NULL == inet_ntop(AF_INET6, &addr->ipv6.ip, string, size))
        {
            PR_SetError(PR_INVALID_ARGUMENT_ERROR, errno);
            return PR_FAILURE;
        }
    }
    else
#else
    if (PR_AF_INET6 == addr->raw.family)
    {
		unsigned char *byte = (unsigned char *) addr->ipv6.ip._pr_s6_addr;
		int i, len;

		len = 0;
		for(i=0; i < 15;i++) {
			PR_snprintf(string + len, size - len, "%u.", byte[i]);
			len = strlen(string);
		}
		PR_snprintf(string + len, size - len, "%u", byte[i]);
    }
    else
#endif  /* defined(_PR_INET6) */
    {
        PR_ASSERT(AF_INET == addr->raw.family);
        PR_ASSERT(size >= 16);
        if (size < 16) goto failed;
        if (AF_INET != addr->raw.family) goto failed;
        else
        {
            unsigned char *byte = (unsigned char*)&addr->inet.ip;
            PR_snprintf(string, size, "%u.%u.%u.%u",
                byte[0], byte[1], byte[2], byte[3]);
        }
    }

    return PR_SUCCESS;

failed:
    PR_SetError(PR_INVALID_ARGUMENT_ERROR, 0);
    return PR_FAILURE;

}  /* PR_NetAddrToString */

PR_IMPLEMENT(PRUint16) PR_ntohs(PRUint16 n) { return ntohs(n); }
PR_IMPLEMENT(PRUint32) PR_ntohl(PRUint32 n) { return ntohl(n); }
PR_IMPLEMENT(PRUint16) PR_htons(PRUint16 n) { return htons(n); }
PR_IMPLEMENT(PRUint32) PR_htonl(PRUint32 n) { return htonl(n); }
PR_IMPLEMENT(PRUint64) PR_ntohll(PRUint64 n)
{
    /*
    ** There is currently no attempt to optomize out depending
    ** on the host' byte order. That would be easy enough to
    ** do.
    */
    PRUint64 tmp;
    PRUint32 hi, lo;
    LL_L2UI(lo, n);
    LL_SHR(tmp, n, 32);
    LL_L2UI(hi, tmp);
    hi = PR_ntohl(hi);
    lo = PR_ntohl(lo);
    LL_UI2L(n, hi);
    LL_SHL(n, n, 32);
    LL_UI2L(tmp, lo);
    LL_ADD(n, n, tmp);
    return n;
}  /* ntohll */

PR_IMPLEMENT(PRUint64) PR_htonll(PRUint64 n)
{
    /*
    ** There is currently no attempt to optomize out depending
    ** on the host' byte order. That would be easy enough to
    ** do.
    */
    PRUint64 tmp;
    PRUint32 hi, lo;
    LL_L2UI(lo, n);
    LL_SHR(tmp, n, 32);
    LL_L2UI(hi, tmp);
    hi = htonl(hi);
    lo = htonl(lo);
    LL_UI2L(n, hi);
    LL_SHL(n, n, 32);
    LL_UI2L(tmp, lo);
    LL_ADD(n, n, tmp);
    return n;
}  /* htonll */
