/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */
#define NEEDPROTOS
#define NEEDGETOPT
#define FILTERFILE	"ldapfilter.conf"
#define TEMPLATEFILE	"ldaptemplates.conf"

#define MOZILLA_CLIENT 1
#define NET_SSL 1
#define LDAP_SSLIO_HOOKS 1

#ifdef __powerc
#define SUPPORT_OPENTRANSPORT	1
#define OTUNIXERRORS 1
#endif


/* Read standard Mac prefix header */
#include "MacPrefix.h"

#if 0 /* These are picked up from the MacPrefix file */
#ifndef macintosh
#define macintosh
#endif 

#define XP_MAC

#include "IDE_Options.h"
// #define NO_USERINTERFACE
// #define LDAP_DEBUG
#endif /* 0 */
